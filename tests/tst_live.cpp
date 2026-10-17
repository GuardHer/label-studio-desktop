#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QFontDatabase>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include "apiclient.h"
#include "languagemanager.h"
#include "imageprovider.h"
#include "labelingconfig.h"

// ListView delegates belong to its visual tree, which can differ from
// QObject ownership after creation/reparenting by Qt Quick.
static QQuickItem *visualItem(QQuickItem *root, const QString &name) {
    if (root->objectName() == name) return root;
    for (auto *child : root->childItems()) if (auto *found = visualItem(child,name)) return found;
    return nullptr;
}

class LiveTest : public QObject {
    Q_OBJECT
private slots:
    void nativeDataManagerRoundTrip() {
        if(qEnvironmentVariable("QML_EDITOR_DATA")!="1"||qEnvironmentVariable("QML_ACCOUNT_ISOLATED")!="1")QSKIP("Use run-editor-data-live.ps1 for isolated Data Manager tests.");
        AnnotationDocument doc;ApiClient api(&doc);const auto server=qEnvironmentVariable("QML_LIVE_SERVER","http://localhost:8080");
        api.login(server,qEnvironmentVariable("QML_ACCOUNT_LIVE_EMAIL"),qEnvironmentVariable("QML_ACCOUNT_LIVE_PASSWORD"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.connected(),qPrintable(api.message()));
        int pid=0;for(const auto &row:api.projects())if(row.toMap().value("title").toString()=="QML management QA data manager")pid=row.toMap().value("id").toInt();QVERIFY(pid>0);api.openProject(pid);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(api.totalTasks(),65);
        auto *dm=api.dataManager();QVERIFY(dm->columns().size()>20);QVERIFY(dm->actions().size()>=6);QVERIFY(dm->actors().size()>=2);
        QQmlApplicationEngine engine;auto *provider=new TaskImageProvider;engine.addImageProvider("task",provider);connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer",server);
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->setProperty("page","project");auto *page=window->findChild<QObject*>("projectPage");QVERIFY(page);
        const auto shots=qEnvironmentVariable("QML_EDITOR_DATA_SCREENSHOTS");const auto capture=[&](const QString &name){if(shots.isEmpty())return;QDir().mkpath(shots);QTest::qWait(150);QVERIFY(window->grabWindow().save(shots+"/"+name+".png"));};
        QTRY_VERIFY_WITH_TIMEOUT(api.thumbnails().size()>=qMin(6,api.tasks().size()),15000);capture("data-manager-en");language->setLanguage("zh_CN",false);capture("data-manager-zh");window->resize(1060,700);capture("data-manager-minimum-zh");language->setLanguage("en",false);
        page->setProperty("filtersOpen",true);auto *filtersPanel=window->findChild<QObject*>("popupDataFilters");QVERIFY(filtersPanel);QVERIFY(QMetaObject::invokeMethod(filtersPanel,"addFilter"));QTest::qWait(100);capture("data-manager-add-filter-en");QVERIFY(QMetaObject::invokeMethod(filtersPanel,"reload"));page->setProperty("filtersOpen",false);
        dm->setPageSize(10);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.tasks().size(),10);api.loadPage(7);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.tasks().size(),5);
        const QVariantMap rank{{"filter","filter:tasks:data.rank"},{"operator","greater"},{"type","Number"},{"value",50}};
        const QVariantMap group{{"filter","filter:tasks:data.group"},{"operator","equal"},{"type","String"},{"value","A"}};
        const QVariantMap andFilters{{"conjunction","and"},{"items",QVariantList{rank,group}}};QVERIFY(dm->applyQuery(andFilters,{"-tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(api.totalTasks(),6);
        const auto filtered=api.tasks();for(const auto &row:filtered){const auto data=row.toMap().value("data").toMap();QVERIFY(data.value("rank").toInt()>50);QCOMPARE(data.value("group").toString(),"A");}
        const QVariantMap orFilters{{"conjunction","or"},{"items",QVariantList{QVariantMap{{"filter","filter:tasks:data.rank"},{"operator","greater"},{"type","Number"},{"value",60}},group}}};QVERIFY(dm->applyQuery(orFilters,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),35);
        const QVariantMap empty{{"conjunction","and"},{"items",QVariantList{}}};
        for(const auto &field:QStringList{"predictions_results","annotations_results"}){
            const QVariantMap f{{"conjunction","and"},{"items",QVariantList{dm->makeFilter(field,"contains","Car")}}};QVERIFY(dm->applyQuery(f,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(api.totalTasks(),field=="predictions_results"?3:2);
        }
        int member=0;for(const auto &row:dm->actors())if(row.toMap().value("id").toInt()!=api.user().value("id").toInt())member=row.toMap().value("id").toInt();QVERIFY(member>0);
        QVariantMap actorFilter{{"conjunction","and"},{"items",QVariantList{dm->makeFilter("annotators","contains",QString::number(member))}}};QVERIFY(dm->applyQuery(actorFilter,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),2);
        QVariantMap dateFilter{{"conjunction","and"},{"items",QVariantList{dm->makeFilter("created_at","greater","2000-01-01T00:00:00Z")}}};QVERIFY(dm->applyQuery(dateFilter,{"-tasks:created_at"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),65);
        QVariantMap scoreFilter{{"conjunction","and"},{"items",QVariantList{dm->makeFilter("predictions_score","greater","0.5")}}};QVERIFY(dm->applyQuery(scoreFilter,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),3);
        QVERIFY(dm->applyQuery(andFilters,{"-tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);dm->setColumnVisible("created_at",true);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);dm->setColumnWidth("created_at",245);dm->moveColumn("created_at",-1);dm->setCompact(true);dm->setPinned(true);page->setProperty("filtersOpen",true);capture("data-manager-pinned-filters-en");
        dm->createView("QA filtered view");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(dm->activeView()>0,qPrintable(api.message()));const int viewId=dm->activeView();dm->renameView("Renamed QA view");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);dm->saveView();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(!dm->viewDirty());dm->activateView(0);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),65);dm->activateView(viewId);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),6);QCOMPARE(dm->pageSize(),10);QVERIFY(dm->compact());QVERIFY(dm->pinned());capture("data-manager-saved-tab-en");
        auto *pinnedPanel=window->findChild<QObject*>("pinnedDataFilters");QVERIFY(pinnedPanel);QVERIFY(QMetaObject::invokeMethod(pinnedPanel,"reload"));QVariant draftRows;QVERIFY(QMetaObject::invokeMethod(pinnedPanel,"draftFilters",Q_RETURN_ARG(QVariant,draftRows)));const auto restoredRows=draftRows.metaType().id()==qMetaTypeId<QJSValue>()?draftRows.value<QJSValue>().toVariant().toList():draftRows.toList();QCOMPARE(restoredRows.size(),2);QCOMPARE(restoredRows[0].toMap().value("type").toString(),"Number");QCOMPARE(restoredRows[0].toMap().value("operator").toString(),"greater");QCOMPARE(restoredRows[0].toMap().value("value").toDouble(),50.0);
        language->setLanguage("zh_CN",false);capture("data-manager-saved-tab-zh");page->setProperty("grid",true);capture("data-manager-grid-zh");page->setProperty("grid",false);language->setLanguage("en",false);page->setProperty("filtersOpen",false);
        api.selectAllResults();QCOMPARE(api.selectedTaskCount(),6);const int excluded=api.tasks().first().toMap().value("id").toInt();api.selectTask(excluded,false);QCOMPARE(api.selectedTaskCount(),5);QVERIFY(!api.isTaskSelected(excluded));api.clearSelection();
        const int selected1=api.tasks()[0].toMap().value("id").toInt(),selected2=api.tasks()[1].toMap().value("id").toInt();api.selectTask(selected1,true);api.selectTask(selected2,true);api.startLabeling();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.samplingQueue(),qPrintable(api.message()));QVERIFY(api.taskId()==selected1||api.taskId()==selected2);const int first=api.taskId();doc.addRectangle(10,10,20,25,"Car");api.saveAndNext();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QVERIFY(api.taskId()!=first);QVERIFY(api.taskId()==selected1||api.taskId()==selected2);QSignalSpy finished(&api,&ApiClient::workflowFinished);api.skipTask();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(finished.size()>0);api.clearSelection();window->setProperty("page","project");
        QVERIFY(dm->applyQuery(empty,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);api.selectAllResults();auto *actionDialog=window->findChild<QObject*>("taskActionConfirm");QVERIFY(actionDialog);
        QVERIFY(QMetaObject::invokeMethod(page,"taskActionRequested",Q_ARG(QString,"delete_tasks_annotations")));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(dm->actionFields().size()>0);QVERIFY(actionDialog->setProperty("values",QVariantMap{{"annotator",QString::number(member)}}));capture("data-manager-delete-by-actor-en");QVERIFY(QMetaObject::invokeMethod(actionDialog,"accept"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        QVERIFY(dm->applyQuery(actorFilter,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),0);QVERIFY(dm->applyQuery(empty,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);api.selectAllResults();
        dm->prepareAction("predictions_to_annotations");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(dm->actionFields().size()>0);actionDialog->setProperty("values",QVariantMap{{"model_version","qa-v1"}});QVERIFY(QMetaObject::invokeMethod(actionDialog,"accept"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        api.selectAllResults();dm->executeAction("delete_tasks_predictions",{});QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(dm->totalPredictions(),0);
        api.selectAllResults();dm->executeAction("retrieve_tasks_predictions",{});QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        api.selectAllResults();dm->executeAction("remove_duplicates",{});QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QElapsedTimer dedupeWait;dedupeWait.start();while(api.totalTasks()!=64&&dedupeWait.elapsed()<30000){QTest::qWait(300);api.loadPage(1);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);}QCOMPARE(api.totalTasks(),64);
        const QVariantMap finalFilter{{"conjunction","and"},{"items",QVariantList{QVariantMap{{"filter","filter:tasks:data.rank"},{"operator","greater"},{"type","Number"},{"value",60}}}}};QVERIFY(dm->applyQuery(finalFilter,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),3);api.selectAllResults();dm->executeAction("delete_tasks",{});QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),0);QVERIFY(dm->applyQuery(empty,{"tasks:id"}));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),61);
        if(!warnings.isEmpty())for(const auto &event:warnings)for(const auto &warning:qvariant_cast<QList<QQmlError>>(event.first()))qWarning().noquote()<<warning.toString();
        QVERIFY2(warnings.isEmpty(),"Native Data Manager must run without QML warnings.");api.disconnectServer();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
    }
    void nativeLabelingConfigRoundTrip() {
        if(qEnvironmentVariable("QML_EDITOR_DATA")!="1"||qEnvironmentVariable("QML_ACCOUNT_ISOLATED")!="1")QSKIP("Use run-editor-data-live.ps1 for isolated editor tests.");
        AnnotationDocument doc;ApiClient api(&doc);const auto server=qEnvironmentVariable("QML_LIVE_SERVER","http://localhost:8080");
        api.login(server,qEnvironmentVariable("QML_ACCOUNT_LIVE_EMAIL"),qEnvironmentVariable("QML_ACCOUNT_LIVE_PASSWORD"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.connected(),qPrintable(api.message()));
        QQmlApplicationEngine engine;auto *provider=new TaskImageProvider;engine.addImageProvider("task",provider);connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer",server);
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        const auto shots=qEnvironmentVariable("QML_EDITOR_DATA_SCREENSHOTS");const auto capture=[&](const QString &name){if(shots.isEmpty())return;QDir().mkpath(shots);QTest::qWait(400);QVERIFY(window->grabWindow().save(shots+"/"+name+".png"));};
        const QString config="<View><Header value='Review the text'/><Text name='body' value='$text'/><Choices name='sentiment' toName='body' required='true'><Choice value='Good'/><Choice value='Bad'/></Choices><TextArea name='note' toName='body'/><Rating name='score' toName='body' maxRating='5'/><Number name='count' toName='body' min='-5' max='10'/></View>";
        api.createProject("QML management QA native config","Native configuration integration",config);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));const int pid=api.project().value("id").toInt();QVERIFY(pid>0);
        auto *settings=window->findChild<QObject*>("projectSettings");QVERIFY(settings);QVERIFY(QMetaObject::invokeMethod(settings,"load"));settings->setProperty("section",1);window->setProperty("page","settings");
        auto *setup=settings->findChild<QObject*>("settingsLabelingSetup");QVERIFY(setup);auto *editor=qobject_cast<LabelingConfig*>(setup->findChild<QObject*>("configEditorModel"));QVERIFY(editor);QCOMPARE(editor->groups().size(),11);QCOMPARE(editor->templates().size(),69);
        editor->setSelectedControl(0);editor->setLabel(0,"Good","#008000","Positive","1");QVERIFY(settings->property("dirty").toBool());editor->validateOnServer(&api,true);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QVERIFY(!editor->serverStatus().isEmpty());
        capture("config-visual-en");language->setLanguage("zh_CN",false);capture("config-visual-zh");setup->setProperty("codeMode",true);capture("config-code-zh");setup->setProperty("browsing",true);capture("config-templates-zh");setup->setProperty("browsing",false);setup->setProperty("codeMode",false);window->resize(1060,700);capture("config-minimum-zh");
        QVERIFY(QMetaObject::invokeMethod(settings,"save"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QVERIFY(!settings->property("dirty").toBool());QVERIFY(api.project().value("label_config").toString().contains("Positive"));
        QTemporaryDir temporary;QFile json(temporary.filePath("text-tasks.json"));QVERIFY(json.open(QIODevice::WriteOnly));json.write("[{\"data\":{\"text\":\"A useful native labeling task\"}}]");json.close();api.importFiles({QUrl::fromLocalFile(json.fileName())});QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(api.totalTasks(),1);
        const int tid=api.tasks().first().toMap().value("id").toInt();api.openTask(tid);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(doc.editable(),qPrintable(doc.error()));QVERIFY(doc.imageFrames().isEmpty());
        doc.setControlValue("sentiment",QStringList{"Positive"});doc.setControlValue("note",QStringList{"Saved by native Qt Quick"});doc.setControlValue("score",4);doc.setControlValue("count",-2);api.save();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QVERIFY(doc.annotationId()>0);const auto expected=doc.results();QCOMPARE(expected.size(),4);capture("config-text-task-zh");
        api.openTask(tid);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(doc.results(),expected);QCOMPARE(doc.controlValues().value("sentiment").toStringList(),QStringList{"Positive"});doc.setControlValue("count",QVariant{});api.save();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(doc.results().size(),3);api.openTask(tid);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(!doc.controlValues().contains("count"));
        window->setProperty("page","project");api.openProject(pid);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(api.project().value("label_config").toString().contains("Positive"));
        int imagesFixture=0;for(const auto &row:api.projects())if(row.toMap().value("title").toString()=="QML management QA data manager")imagesFixture=row.toMap().value("id").toInt();QVERIFY(imagesFixture>0);api.openProject(imagesFixture);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);const auto imageUrl=api.tasks().first().toMap().value("data").toMap().value("image").toString();QVERIFY(!imageUrl.isEmpty());
        const QString mixedConfig="<View><Image name='photos' valueList='$photos'/><RectangleLabels name='boxes' toName='photos'><Label value='Car' alias='car' background='#008000'/></RectangleLabels><PolygonLabels name='areas' toName='photos'><Label value='Road' background='#0000ff'/></PolygonLabels></View>";
        api.createProject("QML management QA native images","Mixed controls and frames",mixedConfig);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        QFile imageJson(temporary.filePath("image-tasks.json"));QVERIFY(imageJson.open(QIODevice::WriteOnly));imageJson.write(QJsonDocument(QJsonArray{QJsonObject{{"data",QJsonObject{{"photos",QJsonArray{imageUrl,imageUrl}}}}}}).toJson());imageJson.close();api.importFiles({QUrl::fromLocalFile(imageJson.fileName())});QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(api.totalTasks(),1);
        const int imageTask=api.tasks().first().toMap().value("id").toInt();api.openTask(imageTask);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(doc.editable(),qPrintable(doc.error()));QCOMPARE(doc.imageFrames().size(),2);QVERIFY(!api.imageSource().isEmpty());
        doc.selectControl(0);doc.addRectangle(10,10,30,30,"Car");doc.selectControl(1);doc.addPolygon({QVariantList{40,40},QVariantList{70,40},QVariantList{65,70}},"Road");QCOMPARE(doc.rectangles().size(),2);
        doc.selectImage(1);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);doc.selectControl(0);doc.addRectangle(45,15,65,35,"Car");api.save();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(doc.results().size(),3);const auto mixedResults=doc.results();capture("config-multiple-images-zh");
        api.openTask(imageTask);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(doc.results(),mixedResults);QCOMPARE(doc.rectangles().size(),2);language->setLanguage("en",false);capture("config-mixed-regions-en");
        window->setProperty("page","project");window->resize(1440,900);QVERIFY(QMetaObject::invokeMethod(window,"openTask",Q_ARG(QVariant,imageTask)));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(window->property("quickView").toBool());capture("data-manager-quickview-en");
        auto *sourceButton=window->findChild<QObject*>("taskSourceButton");QVERIFY(sourceButton);QVERIFY(QMetaObject::invokeMethod(sourceButton,"clicked"));auto *source=window->findChild<QObject*>("taskSourceJson");QVERIFY(source);QVERIFY(source->property("readOnly").toBool());const auto snapshot=QJsonDocument::fromJson(source->property("text").toString().toUtf8()).object();QCOMPARE(snapshot.value("id").toInt(),imageTask);QCOMPARE(snapshot.value("annotations").toArray().size(),1);capture("data-manager-task-source-en");
        QVERIFY(QMetaObject::invokeMethod(window->findChild<QObject*>("taskSourceDialog"),"close"));language->setLanguage("zh_CN",false);window->resize(1060,700);capture("data-manager-quickview-minimum-zh");
        QVERIFY2(warnings.isEmpty(),"Native configuration editor and preview must run without QML warnings.");api.disconnectServer();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
    }
    void nativeProjectManagementRoundTrip() {
        if(qEnvironmentVariable("QML_PROJECT_MANAGEMENT")!="1"||qEnvironmentVariable("QML_ACCOUNT_ISOLATED")!="1")QSKIP("Use run-project-live.ps1 for isolated project integration tests.");
        AnnotationDocument doc;ApiClient api(&doc);const auto server=qEnvironmentVariable("QML_LIVE_SERVER","http://localhost:8080");
        api.login(server,qEnvironmentVariable("QML_ACCOUNT_LIVE_EMAIL"),qEnvironmentVariable("QML_ACCOUNT_LIVE_PASSWORD"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.connected(),qPrintable(api.message()));
        int fixtureId=0;for(const auto &row:api.projects())if(row.toMap().value("title").toString()=="QML management QA fixture")fixtureId=row.toMap().value("id").toInt();QVERIFY(fixtureId>0);
        QQmlApplicationEngine engine;auto *provider=new TaskImageProvider;engine.addImageProvider("task",provider);connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer",server);
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->setProperty("page","projects");
        const auto shots=qEnvironmentVariable("QML_PROJECT_SCREENSHOTS");
        const auto capture=[&](const QString &name){if(shots.isEmpty())return;QDir().mkpath(shots);QTest::qWait(150);QVERIFY(window->grabWindow().save(shots+"/"+name+".png"));};
        capture("projects-en");language->setLanguage("zh_CN",false);capture("projects-zh");window->resize(1060,700);capture("projects-minimum");
        auto *creation=window->findChild<QObject*>("createProjectDialog");QVERIFY(creation);QVERIFY(QMetaObject::invokeMethod(creation,"open"));
        creation->findChild<QObject*>("projectTitle")->setProperty("text","QML management QA empty");
        QVERIFY(QMetaObject::invokeMethod(creation->findChild<QObject*>("createProjectSave"),"clicked"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(api.project().value("id").toInt()>0,qPrintable(api.message()));QVERIFY(!doc.editable());QVERIFY(!api.project().value("label_config").toString().contains("<Image"));const int emptyId=api.project().value("id").toInt();
        auto *projectsPage=window->findChild<QObject*>("projectsPage");QVERIFY(projectsPage);window->setProperty("page","projects");projectsPage->setProperty("pendingDelete",api.project());
        auto *deleteConfirm=window->findChild<QObject*>("deleteCardProjectConfirm");QVERIFY(deleteConfirm);QVERIFY(QMetaObject::invokeMethod(deleteConfirm,"open"));QVERIFY(QMetaObject::invokeMethod(deleteConfirm,"accept"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(api.project().isEmpty());
        for(const auto &row:api.projects())QVERIFY(row.toMap().value("id").toInt()!=emptyId);
        QVERIFY(QMetaObject::invokeMethod(window,"openProjectSettings",Q_ARG(QVariant,fixtureId)));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(window->property("page").toString(),"settings");
        auto *settings=window->findChild<QObject*>("projectSettings");QVERIFY(settings);auto *color=settings->findChild<QObject*>("settingsColor");QVERIFY(color);color->setProperty("text","#ff8844");
        const QString config="<View><Image name='image' value='$image'/><RectangleLabels name='bbox' toName='image'><Label value='Car'/></RectangleLabels></View>";
        auto *setup=settings->findChild<QObject*>("settingsLabelingSetup");QVERIFY(setup);QVERIFY(QMetaObject::invokeMethod(setup,"setConfig",Q_ARG(QVariant,config)));
        QVERIFY(settings->property("dirty").toBool());language->setLanguage("en",false);QCOMPARE(color->property("text").toString(),"#ff8844");QVERIFY(settings->property("dirty").toBool());
        capture("settings-en-minimum");language->setLanguage("zh_CN",false);capture("settings-zh-minimum");
        auto *general=settings->findChild<QObject*>("settingsGeneralScroll");QVERIFY(general);auto *flickable=qvariant_cast<QObject*>(general->property("contentItem"));QVERIFY(flickable);
        QVERIFY(flickable->property("contentHeight").toReal()>flickable->property("height").toReal());
        flickable->setProperty("contentY",flickable->property("contentHeight").toReal()-flickable->property("height").toReal());QTest::qWait(100);
        auto *saveSettings=settings->findChild<QQuickItem*>("saveProjectSettings");QVERIFY(saveSettings);const auto savePoint=saveSettings->mapToScene(QPointF(saveSettings->width()/2,saveSettings->height()/2));QVERIFY(savePoint.y()>48&&savePoint.y()<window->height());
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,savePoint.toPoint());QTRY_VERIFY_WITH_TIMEOUT(!api.busy()&&!settings->property("dirty").toBool(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));capture("settings-save-visible-zh");
        QTemporaryDir temporary;QVariantList files;for(int i=0;i<3;++i){const auto path=temporary.filePath(QString("image-%1.png").arg(i));QVERIFY(QFile::copy(":/assets/bounding-boxes.png",path));files.append(QUrl::fromLocalFile(path));}
        api.importFiles(files);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.totalTasks(),3);api.setTaskQuery("",0,true);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        auto *taskPage=window->findChild<QObject*>("projectPage");QVERIFY(taskPage);window->setProperty("page","project");
        auto *labelButton=window->findChild<QObject*>("startLabeling");QVERIFY(labelButton);taskPage->setProperty("descending",true);QVERIFY(!labelButton->property("enabled").toBool());QTRY_VERIFY_WITH_TIMEOUT(labelButton->property("enabled").toBool(),60000);
        const int firstId=api.tasks().first().toMap().value("id").toInt();
        QVERIFY(QMetaObject::invokeMethod(labelButton,"clicked"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.samplingQueue(),qPrintable(api.message()));QCOMPARE(api.taskId(),firstId);QVERIFY(!api.imageSource().isEmpty());
        doc.addRectangle(10,10,30,30,"Car");api.saveAndNext();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QVERIFY(api.taskId()!=firstId);const int secondId=api.taskId();
        api.moveTask(-1);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.taskId(),firstId);api.saveAndNext();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.taskId(),secondId);
        api.skipTask();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(api.taskId()!=secondId);capture("sampling-queue-zh");
        api.updateProjectDetails("QML management QA fixture","Verified metadata",config,"#ff8844","Uniform sampling");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(api.project().value("sampling").toString(),"Uniform sampling");
        api.setTaskQuery("",0,false);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);api.startLabeling();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.samplingQueue(),qPrintable(api.message()));QVERIFY(!api.imageSource().isEmpty());
        api.openTask(firstId);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(!api.samplingQueue());QVERIFY(doc.annotationId()>0);
        api.setTaskQuery(QString::number(firstId),0,false);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);settings->setProperty("section",2);window->setProperty("page","settings");
        QVERIFY(QMetaObject::invokeMethod(settings,"load"));settings->setProperty("section",2);capture("danger-zone-zh");
        QVERIFY(QMetaObject::invokeMethod(settings->findChild<QObject*>("resetProjectCache"),"clicked"));auto *maintenance=window->findChild<QObject*>("projectMaintenanceConfirm");QVERIFY(maintenance);QVERIFY(QMetaObject::invokeMethod(maintenance,"accept"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        QVERIFY(QMetaObject::invokeMethod(settings->findChild<QObject*>("dropProjectTabs"),"clicked"));QVERIFY(QMetaObject::invokeMethod(maintenance,"accept"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(api.taskSearch(),"");QCOMPARE(api.totalTasks(),3);
        QCOMPARE(api.catalog()->recent().first().toMap().value("id").toInt(),fixtureId);api.catalog()->setSearch("nonexistent-qa-search");QCOMPARE(api.catalog()->items().size(),0);api.catalog()->setSearch("Verified metadata");QCOMPARE(api.catalog()->items().size(),1);api.catalog()->setSearch("");
        QVERIFY2(warnings.isEmpty(),"Project management pages must run without QML warnings.");api.disconnectServer();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
    }
    void nativeAccountRoundTrip() {
        if(qEnvironmentVariable("QML_ACCOUNT_ISOLATED")!="1" || qEnvironmentVariableIsEmpty("QML_ACCOUNT_LIVE_EMAIL") || qEnvironmentVariableIsEmpty("QML_ACCOUNT_LIVE_PASSWORD"))
            QSKIP("Set isolated QML_ACCOUNT_LIVE_EMAIL/PASSWORD for account integration tests.");
        const auto email=qEnvironmentVariable("QML_ACCOUNT_LIVE_EMAIL"), password=qEnvironmentVariable("QML_ACCOUNT_LIVE_PASSWORD");
        const auto server=qEnvironmentVariable("QML_LIVE_SERVER","http://localhost:8080");
        AnnotationDocument doc;ApiClient api(&doc);api.login(server,email,password);
        QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.connected(),qPrintable(api.message()));
        auto *account=api.account();account->load();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(account->loaded(),qPrintable(api.message()));QVERIFY(account->organization().value("id").toInt()>0);
        QVERIFY(account->memberCount()>0);QVERIFY(account->secret().isEmpty());
        QQmlApplicationEngine engine;auto *provider=new TaskImageProvider;engine.addImageProvider("task",provider);
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer",server);
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        window->setProperty("page","account");auto *page=window->findChild<QObject*>("accountPage");QVERIFY(page);
        QVERIFY(QMetaObject::invokeMethod(page,"open",Q_ARG(QVariant,0)));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        auto *first=page->findChild<QObject*>("accountFirstName");auto *save=page->findChild<QObject*>("accountSaveProfile");QVERIFY(first);QVERIFY(save);
        first->setProperty("text","Native QA");QVERIFY(page->property("dirty").toBool());
        language->setLanguage("zh_CN",false);QCOMPARE(first->property("text").toString(),"Native QA");QVERIFY(page->property("dirty").toBool());
        QVERIFY(QMetaObject::invokeMethod(save,"clicked"));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(api.user().value("first_name").toString(),"Native QA");QVERIFY(!page->property("dirty").toBool());
        const auto shots=qEnvironmentVariable("QML_ACCOUNT_SCREENSHOTS");
        const auto capture=[&](const QString &name){if(shots.isEmpty())return;QDir().mkpath(shots);QTest::qWait(120);QVERIFY(window->grabWindow().save(shots+"/"+name+".png"));};
        capture("account-profile-zh");language->setLanguage("en",false);capture("account-profile-en");
        window->resize(1060,700);capture("account-profile-minimum");
        for(int section=1;section<9;++section){page->setProperty("section",section);if(section==5){account->loadMember(api.user().value("id").toInt());QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);}capture(QString("account-section-%1").arg(section));}
        page->setProperty("section",0);
        account->saveEmailPreferences(true);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->profile().value("allow_newsletters").toBool());
        QTemporaryDir temporary;QImage image(100,100,QImage::Format_RGB32);image.fill(Qt::blue);QVERIFY(image.save(temporary.filePath("avatar.png")));
        account->uploadAvatar(QUrl::fromLocalFile(temporary.filePath("avatar.png")));QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!account->profile().value("avatar").toString().isEmpty(),qPrintable(api.message()));
        account->removeAvatar();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->profile().value("avatar").toString().isEmpty());
        account->loadMember(api.user().value("id").toInt());QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->memberDetail().contains("annotations_count"));
        account->loadInvite();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->inviteUrl().contains("token="));
        const auto oldInvite=account->inviteUrl();account->resetInvite();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->inviteUrl()!=oldInvite);
        account->saveOrganization("QML account isolated QA updated","qa@example.invalid");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(account->organization().value("title").toString(),"QML account isolated QA updated");
        account->saveTokenSettings(true,true);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->tokenSettings().value("legacy_api_tokens_enabled").toBool());
        account->loadLegacyToken();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(!account->legacyToken().isEmpty());
        const auto oldLegacy=account->legacyToken();account->resetLegacyToken();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->legacyToken()!=oldLegacy);
        for(const auto &row:account->members()){
            const auto member=row.toMap().value("user").toMap();
            if(member.value("id").toInt()!=api.user().value("id").toInt()){
                account->removeMember(member.value("id").toInt());QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QCOMPARE(account->memberCount(),1);break;
            }
        }
        account->saveHotkeys(R"({"custom_hotkeys":{"editor:save":{"key":"ctrl+shift+s","active":true}}})");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(account->keySequence("editor:save","Ctrl+S"),"Ctrl+Shift+S");
        if(account->tokenSettings().value("api_tokens_enabled").toBool()){
            account->createToken();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!account->secret().isEmpty(),qPrintable(api.message()));
            const auto token=account->secret();account->rotateToken(token);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->secret()!=token);
            account->revokeToken(account->secret());QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->tokens().isEmpty());
        }
        const auto newPassword=password+"X";account->changePassword(password,newPassword);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        account->load();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(account->loaded());
        api.disconnectServer();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY(!api.connected());
        api.login(server,email,newPassword);QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);QVERIFY2(api.connected(),qPrintable(api.message()));
        QVERIFY2(warnings.isEmpty(),"Account UI must load and switch languages without QML warnings.");
        api.disconnectServer();QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
    }
    void realDjangoRoundTrip() {
        if (qEnvironmentVariableIsEmpty("QML_LIVE_EMAIL") || qEnvironmentVariableIsEmpty("QML_LIVE_PASSWORD"))
            QSKIP("Set QML_LIVE_EMAIL and QML_LIVE_PASSWORD for the optional live Django test.");
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider = new TaskImageProvider;
        engine.addImageProvider("task", provider);
        connect(&api, &ApiClient::imageReady, &api, [=](const QImage &image) { provider->setImage(image); });
        const auto server = qEnvironmentVariable("QML_LIVE_SERVER", "http://localhost:8080");
        auto *language = new LanguageManager(&engine, nullptr, &engine); language->setLanguage(qEnvironmentVariable("QML_LIVE_LANGUAGE", "en"),false);
        engine.rootContext()->setContextProperty("languageManager", language);
        engine.rootContext()->setContextProperty("backend", &api);
        engine.rootContext()->setContextProperty("annotationDocument", &doc);
        engine.rootContext()->setContextProperty("initialServer", server);
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        const auto screenshots = qEnvironmentVariable("QML_LIVE_SCREENSHOTS");
        const auto capture = [&](const QString &name) {
            if (screenshots.isEmpty()) return;
            QDir().mkpath(screenshots); QTest::qWait(180);
            QVERIFY(window->grabWindow().save(screenshots + "/" + name + ".png"));
            const auto originalLanguage = language->code();
            QVERIFY(language->setLanguage(originalLanguage == "zh_CN" ? "en" : "zh_CN", false));
            QTest::qWait(100);
            QVERIFY(window->grabWindow().save(screenshots + "/" + name + "-other-language.png"));
            const auto originalSize = window->size();
            window->resize(1060, 700); QTest::qWait(100);
            QVERIFY(window->grabWindow().save(screenshots + "/" + name + "-minimum.png"));
            QVERIFY(language->setLanguage(originalLanguage, false));
            QTest::qWait(100);
            QVERIFY(window->grabWindow().save(screenshots + "/" + name + "-minimum-original-language.png"));
            window->resize(originalSize); QTest::qWait(100);
        };
        capture("native-login");
        auto *email = window->findChild<QObject *>("loginEmail"); QVERIFY(email);
        auto *password = window->findChild<QObject *>("loginPassword"); QVERIFY(password);
        email->setProperty("text", qEnvironmentVariable("QML_LIVE_EMAIL"));
        password->setProperty("text", qEnvironmentVariable("QML_LIVE_PASSWORD"));
        auto *loginButton = window->findChild<QObject *>("loginSubmit"); QVERIFY(loginButton);
        QVERIFY(QMetaObject::invokeMethod(loginButton, "clicked"));
        QVERIFY(password->property("text").toString().isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QVERIFY2(api.connected(), qPrintable(api.message()));
        QVERIFY(!api.user().value("email").toString().isEmpty());
        capture("native-home");
        window->setProperty("page", "projects"); capture("native-projects");
        const auto referenceId = qEnvironmentVariableIntValue("QML_LIVE_REFERENCE_PROJECT");
        if (referenceId > 0) {
            api.openProject(referenceId); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
            window->setProperty("page", "project");
            auto *projectPage = window->findChild<QObject *>("projectPage"); QVERIFY(projectPage);
            if (!api.tasks().isEmpty()) {
                const int referenceTask = api.tasks().first().toMap().value("id").toInt();
                for (const auto &row : api.tasks()) api.requestThumbnail(row.toMap().value("id").toInt());
                QTRY_COMPARE_WITH_TIMEOUT(api.thumbnails().size(), api.tasks().size(), 30000);
                capture("native-reference-tasks");
                projectPage->setProperty("grid", true); capture("native-reference-grid");
                projectPage->setProperty("grid", false);
                api.openTask(referenceTask); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
                QVERIFY2(!api.imageSource().isEmpty(),qPrintable(api.message()));
                window->setProperty("page", "annotate"); capture("native-reference-annotation");
            }
        }
        window->setProperty("page", "home");
        auto *creation = window->findChild<QObject *>("createProjectDialog"); QVERIFY(creation);
        QVERIFY(QMetaObject::invokeMethod(creation, "open"));
        QTRY_VERIFY(creation->property("opened").toBool()); capture("native-create");
        creation->setProperty("step", 1); capture("native-create-import");
        creation->setProperty("step", 2); capture("native-create-setup");
        creation->setProperty("step", 0);
        auto *title = creation->findChild<QObject *>("projectTitle"); QVERIFY(title);
        const auto projectName = "QML verification " + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmsszzz");
        auto *creationSetup=creation->findChild<QObject*>("creationLabelingSetup");QVERIFY(creationSetup);
        creationSetup->setProperty("config","<View><Image name='image' value='$image'/><RectangleLabels name='bbox' toName='image'><Label value='Car'/></RectangleLabels></View>");
        title->setProperty("text", projectName);
        auto *save = creation->findChild<QObject *>("createProjectSave"); QVERIFY(save);
        QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
        QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QVERIFY2(api.project().value("id").toInt() > 0, qPrintable(api.message()));
        const auto disposableId = api.project().value("id").toInt();
        struct Cleanup {
            ApiClient *api; int id;
            ~Cleanup() {
                if (api->project().value("id").toInt() != id) return;
                if (api->busy()) { QElapsedTimer timer; timer.start(); while (api->busy() && timer.elapsed() < 30000) QTest::qWait(20); }
                api->discardChanges(); api->deleteProject();
                QElapsedTimer timer; timer.start(); while (api->busy() && timer.elapsed() < 30000) QTest::qWait(20);
            }
        } cleanup{&api, disposableId};
        QCOMPARE(api.project().value("title").toString(), projectName);
        capture("native-tasks-empty");
        QTemporaryDir temp;
        QVERIFY(QFile::copy(":/assets/bounding-boxes.png", temp.filePath("sample.png")));
        QVERIFY(QFile::copy(":/assets/bounding-boxes.png", temp.filePath("sample-second.png")));
        api.importFiles({QUrl::fromLocalFile(temp.filePath("sample.png")), QUrl::fromLocalFile(temp.filePath("sample-second.png"))});
        QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QVERIFY2(api.totalTasks() == 2, qPrintable(api.message()));
        QCOMPARE(api.tasks().size(), 2); capture("native-tasks");
        const int task = api.tasks().first().toMap().value("id").toInt();
        auto *settings = window->findChild<QObject *>("projectSettings"); QVERIFY(settings);
        QVERIFY(QMetaObject::invokeMethod(settings, "load"));
        window->setProperty("page", "settings"); capture("native-settings");
        window->setProperty("page", "project");
        api.openTask(task); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QVERIFY2(!api.imageSource().isEmpty(), qPrintable(api.message())); QVERIFY(doc.editable());
        window->setProperty("page", "annotate");
        doc.addRectangle(12,15,36,42,"Car"); QVERIFY(doc.dirty());
        api.save(); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QVERIFY2(!doc.dirty(), qPrintable(api.message())); QVERIFY(doc.annotationId() > 0);
        capture("native-annotation");
        api.openTask(task); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QCOMPARE(doc.rectangles().size(), 1);
        QCOMPARE(doc.rectangles()[0].toMap().value("label").toString(), "Car");
        const auto exportPath = temp.filePath("annotations.json");
        api.exportProject(QUrl::fromLocalFile(exportPath)); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(), 60000);
        QFile exported(exportPath); QVERIFY2(exported.open(QIODevice::ReadOnly), qPrintable(api.message()));
        const auto json = QJsonDocument::fromJson(exported.readAll()); QVERIFY(json.isArray());
        QCOMPARE(json.array().size(), 2);
        bool annotationExported = false;
        for (const auto &item : json.array())
            if (item.toObject().value("id").toInt() == task)
                annotationExported = !item.toObject().value("annotations").toArray().isEmpty();
        QVERIFY(annotationExported);
        // Every mutation below is confined to this test's disposable project.
        const auto originalConfig = api.project().value("label_config").toString();
        QVERIFY(QMetaObject::invokeMethod(settings, "load"));
        window->setProperty("page", "settings");
        auto *settingsName = settings->findChild<QObject *>("settingsName"); QVERIFY(settingsName);
        settingsName->setProperty("text", projectName + " renamed");
        QVERIFY(settings->property("dirty").toBool());
        auto *settingsSave = settings->findChild<QObject *>("saveProjectSettings"); QVERIFY(settingsSave);
        QVERIFY(QMetaObject::invokeMethod(settingsSave,"clicked")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(!settings->property("dirty").toBool(),qPrintable(api.message()));
        api.openProject(disposableId); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(api.project().value("title").toString(),projectName + " renamed");
        QCOMPARE(api.project().value("label_config").toString(),originalConfig);
        const auto media=api.tasks().first().toMap().value("data").toMap().value("image").toString();
        QJsonArray many;
        for(int i=0;i<51;++i) many.append(QJsonObject{{"image",media}});
        QFile dataset(temp.filePath("batch.json")); QVERIFY(dataset.open(QIODevice::WriteOnly));
        dataset.write(QJsonDocument(many).toJson()); dataset.close();
        api.importFiles({QUrl::fromLocalFile(dataset.fileName())}); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(api.totalTasks()==53,qPrintable(api.message())); QCOMPARE(api.tasks().size(),50);
        api.setTaskQuery("",0,true); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        const int firstDescending=api.tasks().first().toMap().value("id").toInt();
        const int boundaryTask=api.tasks().last().toMap().value("id").toInt();
        QVERIFY(firstDescending>boundaryTask);
        api.openTask(boundaryTask); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        api.moveTask(1); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(api.page(),2);
        QVERIFY(api.taskId()<boundaryTask); QCOMPARE(api.tasks().size(),3);
        api.moveTask(-1); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(api.taskId(),boundaryTask); QCOMPARE(api.page(),1);
        api.setTaskQuery(QString::number(firstDescending),0,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(api.totalTasks(),1); QCOMPARE(api.tasks().first().toMap().value("id").toInt(),firstDescending);
        api.setTaskQuery("",1,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(api.totalTasks()==1,qPrintable(api.message()));
        api.setTaskQuery("",2,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(api.totalTasks(),52);
        const int nextUnlabeled=api.tasks().first().toMap().value("id").toInt();
        api.openTask(nextUnlabeled); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        doc.addRectangle(10,10,25,25,"Car");
        auto *submit=window->findChild<QObject *>("submitAnnotation"); QVERIFY(submit);
        QVERIFY(QMetaObject::invokeMethod(submit,"clicked")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(api.taskId()!=nextUnlabeled,qPrintable(api.message())); QCOMPARE(api.totalTasks(),51);
        const int skippedTask=api.taskId();
        auto *skip=window->findChild<QObject *>("skipTask"); QVERIFY(skip);
        QVERIFY(QMetaObject::invokeMethod(skip,"clicked")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(api.taskId()!=skippedTask,qPrintable(api.message()));
        api.setTaskQuery(QString::number(skippedTask),0,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(api.tasks().first().toMap().value("cancelled_annotations").toInt(),1);
        api.openTask(nextUnlabeled); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QVERIFY(doc.annotationId()>0);
        auto *truth=window->findChild<QObject *>("groundTruth"); QVERIFY(truth);
        QVERIFY(QMetaObject::invokeMethod(truth,"clicked")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QVERIFY2(api.groundTruth(),qPrintable(api.message()));
        api.openTask(nextUnlabeled); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QVERIFY(api.groundTruth());
        auto *deleteButton=window->findChild<QObject *>("deleteAnnotation"); QVERIFY(deleteButton);
        QVERIFY(QMetaObject::invokeMethod(deleteButton,"clicked"));
        auto *deleteConfirm=window->findChild<QObject *>("deleteAnnotationConfirm"); QVERIFY(deleteConfirm);
        QVERIFY(QMetaObject::invokeMethod(deleteConfirm,"accept")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(doc.annotationId(),-1);
        api.openTask(nextUnlabeled); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(doc.annotationId(),-1);
        api.setTaskQuery(QString::number(task),0,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        api.selectTask(task,true);
        auto *projectPage=window->findChild<QObject *>("projectPage"); QVERIFY(projectPage);
        window->setProperty("page","project");
        api.dataManager()->prepareAction("delete_tasks_annotations");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        auto *actionConfirm=window->findChild<QObject *>("taskActionConfirm"); QVERIFY(actionConfirm);
        QVERIFY(QMetaObject::invokeMethod(actionConfirm,"accept")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY2(!api.messageIsError(),qPrintable(api.message())); QCOMPARE(api.selectedTaskCount(),0);
        api.openTask(task); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(doc.annotationId(),-1);
        api.setTaskQuery(QString::number(task),0,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        api.selectTask(task,true); api.dataManager()->prepareAction("delete_tasks");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY(QMetaObject::invokeMethod(actionConfirm,"accept")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(api.totalTasks(),0); QCOMPARE(api.selectedTaskCount(),0);
        api.setTaskQuery("",0,false); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(api.totalTasks(),52);
        window->setProperty("page","project");
        QTRY_VERIFY_WITH_TIMEOUT(api.thumbnails().size()>=qMin(8,api.tasks().size()),10000);
        capture("native-functional-tasks");
        auto *selectPage = window->findChild<QQuickItem *>("selectPageTasks"); QVERIFY(selectPage);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,selectPage->mapToScene(QPointF(selectPage->width()/2,selectPage->height()/2)).toPoint());
        QTRY_COMPARE(api.selectedTaskCount(),50);
        api.loadPage(2); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(api.tasks().size(),2);
        const int lastPageTask=api.tasks().first().toMap().value("id").toInt();
        QTRY_VERIFY(visualItem(window->contentItem(),"selectTask"+QString::number(lastPageTask)));
        auto *selectLast=visualItem(window->contentItem(),"selectTask"+QString::number(lastPageTask));
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,selectLast->mapToScene(QPointF(selectLast->width()/2,selectLast->height()/2)).toPoint());
        QTRY_COMPARE(api.selectedTaskCount(),51);
        api.dataManager()->prepareAction("delete_tasks");QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY(QMetaObject::invokeMethod(actionConfirm,"accept")); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(api.page(),1); QCOMPARE(api.totalTasks(),1); QCOMPARE(api.tasks().size(),1);
        QVERIFY2(!api.messageIsError(),qPrintable(api.message()));

        api.deleteProject(); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QVERIFY(api.project().isEmpty());
        api.createProject("QML polygon verification " + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmsszzz"), "Temporary polygon round trip", "<View><Image name='image' value='$image'/><PolygonLabels name='outline' toName='image'><Label value='Car'/></PolygonLabels></View>");
        QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QVERIFY(api.project().value("id").toInt()>0);
        Cleanup polygonCleanup{&api,api.project().value("id").toInt()};
        api.importFiles({QUrl::fromLocalFile(temp.filePath("sample.png"))});
        QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QCOMPARE(api.tasks().size(),1);
        const int polygonTask=api.tasks().first().toMap().value("id").toInt();
        api.openTask(polygonTask); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QVERIFY(doc.editable());
        const QVariantList points{QVariantList{10.,20.},QVariantList{70.,20.},QVariantList{60.,75.},QVariantList{20.,85.}};
        doc.addPolygon(points,"Car"); QVERIFY(doc.dirty());
        api.save(); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000); QVERIFY(!doc.dirty()); QVERIFY(doc.annotationId()>0);
        window->setProperty("page","annotate"); capture("native-polygon-annotation");
        api.openTask(polygonTask); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QCOMPARE(doc.results().size(),1);
        QCOMPARE(doc.results()[0].toObject()["value"].toObject()["points"].toArray(),QJsonArray::fromVariantList(points));
        api.exportProject(QUrl::fromLocalFile(temp.filePath("polygons.json"))); QTRY_VERIFY_WITH_TIMEOUT(!api.busy(),60000);
        QFile polygonExport(temp.filePath("polygons.json")); QVERIFY(polygonExport.open(QIODevice::ReadOnly));
        const auto polygonJson=QJsonDocument::fromJson(polygonExport.readAll()).array(); QCOMPARE(polygonJson.size(),1);
        QCOMPARE(polygonJson[0].toObject()["annotations"].toArray()[0].toObject()["result"].toArray()[0].toObject()["type"].toString(),"polygonlabels");
        window->setProperty("page", "project");
        QVERIFY2(warnings.isEmpty(), "Native pages must run without QML warnings.");
    }
};
int main(int argc, char **argv) {
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    QGuiApplication app(argc, argv);
    app.setOrganizationName("LabelStudioDesktopTests"); app.setApplicationName("Live");
    QQuickStyle::setStyle("Material");
    registerLabelingTypes();
    QFontDatabase::addApplicationFont(":/assets/Figtree-Regular.ttf");
    QFontDatabase::addApplicationFont(":/assets/Figtree-SemiBold.ttf");
    QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR")+"/Fonts/msyh.ttc");
    QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR")+"/Fonts/msyhbd.ttc");
    LiveTest test; return QTest::qExec(&test, argc, argv);
}
#include "tst_live.moc"
