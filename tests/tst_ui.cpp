#include <QtTest>
#include <functional>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQmlProperty>
#include "apiclient.h"
#include "languagemanager.h"
#include "imageprovider.h"
#include "labelingconfig.h"

static QQuickItem *visualAccountItem(QQuickItem *root,const QString &name) {
    if(root->objectName()==name)return root;
    for(auto *child:root->childItems())if(auto *found=visualAccountItem(child,name))return found;
    return nullptr;
}

class UiTest : public QObject {
    Q_OBJECT
private slots:
    void taskQuickViewSharesWorkspaceAndProtectsUnsavedClose() {
        AnnotationDocument doc;ApiClient api(&doc);QQmlApplicationEngine engine;
        auto *provider=new TaskImageProvider;engine.addImageProvider("task",provider);connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);api.loadDemo();
        QCOMPARE(window->findChildren<QObject*>("annotationWorkspace").size(),1);
        window->setProperty("page","project");QVERIFY(QMetaObject::invokeMethod(window,"openTask",Q_ARG(QVariant,1001)));
        QTRY_COMPARE(window->property("page").toString(),"annotate");QVERIFY(window->property("quickView").toBool());
        auto *project=window->findChild<QQuickItem*>("projectPage");auto *workspace=window->findChild<QQuickItem*>("annotationWorkspace");QVERIFY(project);QVERIFY(workspace);QVERIFY(project->isVisible());QVERIFY(workspace->isVisible());
        QTRY_VERIFY(project->width()>=200);QTRY_VERIFY(workspace->width()>=760);
        auto *sourceButton=window->findChild<QObject*>("taskSourceButton");QVERIFY(sourceButton);QVERIFY(QMetaObject::invokeMethod(sourceButton,"clicked"));
        auto *source=window->findChild<QObject*>("taskSourceJson");QVERIFY(source);QVERIFY(source->property("text").toString().contains("data"));QVERIFY(source->property("readOnly").toBool());
        auto *sourceDialog=window->findChild<QObject*>("taskSourceDialog");QVERIFY(sourceDialog);QVERIFY(QMetaObject::invokeMethod(sourceDialog,"close"));
        doc.addRectangle(1,2,20,30,doc.labels()[0].toMap()["name"].toString());QVERIFY(doc.dirty());
        auto *close=window->findChild<QObject*>("closeTaskQuickView");QVERIFY(close);QVERIFY(QMetaObject::invokeMethod(close,"clicked"));
        auto *discard=window->findChild<QObject*>("navigationDiscardDialog");QVERIFY(discard);QVERIFY(discard->property("visible").toBool());QCOMPARE(window->property("page").toString(),"annotate");
        QVERIFY(QMetaObject::invokeMethod(discard,"reject"));QVERIFY(window->property("quickView").toBool());
        api.discardChanges();QVERIFY(QMetaObject::invokeMethod(close,"clicked"));QCOMPARE(window->property("page").toString(),"project");
        QVERIFY(QMetaObject::invokeMethod(window,"openTask",Q_ARG(QVariant,1001)));doc.addRectangle(2,3,21,31,doc.labels()[0].toMap()["name"].toString());QVERIFY(doc.dirty());const auto before=doc.results();auto *expand=window->findChild<QObject*>("expandTaskQuickView");QVERIFY(expand);QVERIFY(QMetaObject::invokeMethod(expand,"clicked"));QVERIFY(!window->property("quickView").toBool());QCOMPARE(window->property("page").toString(),"annotate");QCOMPARE(doc.results(),before);QVERIFY(doc.dirty());
        api.discardChanges();window->setProperty("quickView",true);
        auto *canvas=window->findChild<QQuickItem*>("annotationCanvas");QVERIFY(canvas);
        const QVariantList draft{QVariantList{10,10},QVariantList{20,20}};canvas->setProperty("polygonDraft",draft);
        QVERIFY(!doc.dirty());QVERIFY(workspace->property("hasDraft").toBool());
        QVERIFY(QMetaObject::invokeMethod(project,"taskActionRequested",Q_ARG(QString,"delete_tasks_predictions")));
        QVERIFY(discard->property("visible").toBool());QVERIFY(QMetaObject::invokeMethod(discard,"reject"));QVERIFY(workspace->property("hasDraft").toBool());
    }
    void runtimeTextAnnotationKeepsTypingFocusAcrossLanguageChanges() {
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider=new TaskImageProvider; engine.addImageProvider("task",provider);
        connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine); language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language); engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc); engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml")); QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window); api.loadDemo();
        QVERIFY(doc.configure("<View><Header value='Review text'/><Text name='text' value='Native text task'/><TextArea name='notes' toName='text' required='true'/><Choices name='class' toName='text'><Choice value='Good'/></Choices></View>"));
        doc.setTaskData({}); emit api.taskOpened(); window->setProperty("page","annotate");
        auto *workspace=window->findChild<QObject*>("annotationWorkspace"); QVERIFY(workspace);
        QTRY_VERIFY(workspace->property("hasRuntimeLayout").toBool());
        QTRY_VERIFY(visualAccountItem(window->contentItem(),"runtimeConfigurationControls"));
        auto *layout=visualAccountItem(window->contentItem(),"runtimeConfigurationControls");
        QQuickItem *field=nullptr;
        std::function<void(QQuickItem*)> findEditor=[&](QQuickItem *item){if(item->metaObject()->indexOfProperty("cursorPosition")>=0&&item->metaObject()->indexOfProperty("text")>=0&&item->isVisible())field=item;for(auto *child:item->childItems())findEditor(child);};
        QTRY_VERIFY(([&]{findEditor(layout);return field!=nullptr;})());
        field->forceActiveFocus(); QTRY_COMPARE(window->activeFocusItem(),field);
        for(const char character:QByteArray("Review 123 rvh")) QTest::keyClick(window,character);
        QTRY_COMPARE(doc.controlValues().value("notes").toStringList(),QStringList{"Review 123 rvh"});
        QCOMPARE(workspace->property("tool").toString(),"draw"); QCOMPARE(window->activeFocusItem(),field);
        language->setLanguage("zh_CN",false); QCoreApplication::processEvents();
        QCOMPARE(window->activeFocusItem(),field); QCOMPARE(field->property("text").toString(),"Review 123 rvh");
        QTest::keyClick(window,Qt::Key_A); QTRY_COMPARE(doc.controlValues().value("notes").toStringList(),QStringList{"Review 123 rvha"});
        auto *submit=window->findChild<QObject*>("submitAnnotation"); QVERIFY(submit); QVERIFY(submit->property("enabled").toBool());
    }
    void textAreaEditingPreservesSeparateSubmissions() {
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider=new TaskImageProvider; engine.addImageProvider("task",provider);
        connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine); language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language); engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc); engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml")); QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window); api.loadDemo();
        QVERIFY(doc.configure("<View><Text name='body' value='Text'/><TextArea name='notes' toName='body' maxSubmissions='3'/></View>"));
        doc.setTaskData({}); doc.setControlValue("notes",QStringList{"First entry","Second entry"});
        emit api.taskOpened(); window->setProperty("page","annotate");
        QTRY_VERIFY(visualAccountItem(window->contentItem(),"runtimeTextnotes_1"));
        auto *second=visualAccountItem(window->contentItem(),"runtimeTextnotes_1"); second->forceActiveFocus();
        QTest::keyClick(window,Qt::Key_End); QTest::keyClick(window,Qt::Key_X);
        QTRY_COMPARE(doc.controlValues().value("notes").toStringList(),(QStringList{"First entry","Second entryx"}));
        language->setLanguage("zh_CN",false);
        QCOMPARE(visualAccountItem(window->contentItem(),"runtimeTextnotes_0")->property("text").toString(),"First entry");
        QCOMPARE(second->property("text").toString(),"Second entryx");
    }
    void labelingConfigVisualEditsPreserveOtherNodes() {
        AnnotationDocument doc;ApiClient api(&doc);QQmlApplicationEngine engine;
        auto *language=new LanguageManager(&engine,nullptr,&engine);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto *setup=engine.rootObjects().first()->findChild<QObject*>("creationLabelingSetup");QVERIFY(setup);
        const QString source="<View><Header value='Keep title'/><Image name='photo' value='$photo'/><RectangleLabels name='boxes' toName='photo'><Label value='Cat' background='#ff0000'/></RectangleLabels><Choices name='quality' toName='photo'><Choice value='Good'/></Choices></View>";
        QVERIFY(QMetaObject::invokeMethod(setup,"setConfig",Q_ARG(QVariant,source)));
        auto *model=setup->findChild<QObject*>("configEditorModel");QVERIFY2(model,"Native XML visual editor model is missing");
        QCOMPARE(model->property("controls").toList().size(),2);
        QVERIFY(QMetaObject::invokeMethod(model,"setLabel",Q_ARG(int,0),Q_ARG(QString,"Kitten"),Q_ARG(QString,"#33aa55"),Q_ARG(QString,"Small cat"),Q_ARG(QString,"1")));
        const auto result=setup->property("config").toString();QVERIFY(result.contains("Keep title"));QVERIFY(result.contains("quality"));QVERIFY(result.contains("Small cat"));QVERIFY(result.contains("#33aa55"));
        QVERIFY(QMetaObject::invokeMethod(model,"setObjectValue",Q_ARG(int,0),Q_ARG(QString,"$picture")));QVERIFY(setup->property("config").toString().contains("$picture"));
        QVERIFY(model->property("templates").toList().size()>=60);
        auto *configModel=qobject_cast<LabelingConfig*>(model);QVERIFY(configModel);
        configModel->setXml("<View><Image name='image' value='$payload.image'/><RectangleLabels name='boxes' toName='image'><Label value='Cat'/></RectangleLabels></View>");
        QVERIFY2(configModel->sampleData().value("payload").toMap().contains("image"),"Nested mappings need nested preview data");
        model->setProperty("xml","<View><Image></View>");QVERIFY(!model->property("error").toString().isEmpty());
    }
    void projectCreationClearsTemplateBeforeOpeningAnimation() {
        AnnotationDocument doc;ApiClient api(&doc);QQmlApplicationEngine engine;
        auto *language=new LanguageManager(&engine,nullptr,&engine);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto *creation=engine.rootObjects().first()->findChild<QObject*>("createProjectDialog");QVERIFY(creation);
        auto *setup=creation->findChild<QObject*>("creationLabelingSetup");QVERIFY(setup);
        setup->setProperty("config","<View>stale template</View>");
        QVERIFY(QMetaObject::invokeMethod(creation,"open"));QCOMPARE(setup->property("config").toString(),QString());
    }
    void accountHotkeyTypingSurvivesLanguageSwitch() {
        AnnotationDocument doc;ApiClient api(&doc);QQmlApplicationEngine engine;
        auto *provider=new TaskImageProvider;engine.addImageProvider("task",provider);connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){provider->setImage(image);});
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);api.loadDemo();
        window->setProperty("page","account");auto *page=window->findChild<QObject*>("accountPage");QVERIFY(page);page->setProperty("section",1);
        auto *json=page->findChild<QObject*>("accountHotkeys");QVERIFY(json);json->setProperty("text",QStringLiteral("{\"custom_hotkeys\":{}}"));
        auto *field=visualAccountItem(window->contentItem(),"accountHotkeyeditor:save");QVERIFY(field);field->forceActiveFocus();
        QTRY_COMPARE(window->activeFocusItem(),field);
        QTest::keyClick(window,Qt::Key_A,Qt::ControlModifier);for(const char character:QByteArray("ctrl+alt+s"))QTest::keyClick(window,character);
        const auto edited=json->property("text").toString();QVERIFY(edited.contains("ctrl+alt+s"));QVERIFY(page->property("dirty").toBool());
        language->setLanguage("zh_CN",false);QCOMPARE(json->property("text").toString(),edited);
        field=visualAccountItem(window->contentItem(),"accountHotkeyeditor:save");QVERIFY(field);QCOMPARE(field->property("text").toString(),"ctrl+alt+s");
    }
    void signupConfirmationCannotBeBypassedWithEnter() {
        AnnotationDocument doc;ApiClient api(&doc);QQmlApplicationEngine engine;
        auto *language=new LanguageManager(&engine,nullptr,&engine);language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language);engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        engine.load(QUrl("qrc:/qml/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());auto *window=engine.rootObjects().first();
        auto *login=window->findChild<QObject*>("loginPage");QVERIFY(login);login->setProperty("registering",true);
        window->findChild<QObject*>("loginEmail")->setProperty("text","confirmation@example.invalid");
        auto *password=window->findChild<QObject*>("loginPassword");password->setProperty("text","CandidatePass!123");
        auto *submit=window->findChild<QObject*>("loginSubmit");QVERIFY(!submit->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(password,"accepted"));QVERIFY(!api.busy());
        QVERIFY(QMetaObject::invokeMethod(submit,"clicked"));QVERIFY(!api.busy());
    }
    void translatedModelsPreserveSelection() {
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider = new TaskImageProvider; engine.addImageProvider("task",provider);
        connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){ provider->setImage(image); });
        auto *language = new LanguageManager(&engine,nullptr,&engine); language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager",language);
        engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);
        engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl("qrc:/qml/Main.qml")); QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        api.loadDemo(); api.selectAnnotation(-1);
        auto *annotations=window->findChild<QObject *>("annotationSelector"); QVERIFY(annotations);
        QTRY_COMPARE(annotations->property("currentValue").toInt(),-1);
        QVERIFY(language->setLanguage("zh_CN",false));
        QTRY_COMPARE(annotations->property("currentValue").toInt(),-1);
        QCOMPARE(annotations->property("displayText").toString(),QStringLiteral("新建标注"));
        auto *dialog=window->findChild<QObject *>("connectionDialog"); QVERIFY(dialog);
        QVERIFY(QMetaObject::invokeMethod(dialog,"open"));
        auto *auth=dialog->findChild<QObject *>("authMode"); QVERIFY(auth);
        auth->setProperty("currentIndex",2);
        QVERIFY(language->setLanguage("en",false));
        QTRY_COMPARE(auth->property("currentIndex").toInt(),2);
        QVERIFY(language->setLanguage("zh_CN",false));
        QTRY_COMPARE(auth->property("currentIndex").toInt(),2);
        QVERIFY(QMetaObject::invokeMethod(dialog,"close"));
        QVERIFY2(warnings.isEmpty(),"Translated models must preserve selection without warnings");
    }
    void switchesLanguageWithoutLosingEdits() {
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider = new TaskImageProvider; engine.addImageProvider("task",provider);
        connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){ provider->setImage(image); });
        auto *language = new LanguageManager(&engine, nullptr, &engine); language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager", language);
        engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);
        engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl("qrc:/qml/Main.qml")); QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        auto *loginSelector=window->findChild<QObject *>("loginLanguageSelector");
        QVERIFY2(loginSelector,"Login page must provide Chinese / English switching");
        auto *submit=window->findChild<QObject *>("loginSubmit"); QVERIFY(submit);
        auto *email=window->findChild<QObject *>("loginEmail"); email->setProperty("text","example@example.com");
        loginSelector->setProperty("currentIndex",0);
        QVERIFY(QMetaObject::invokeMethod(loginSelector,"activated",Q_ARG(int,0)));
        QTRY_COMPARE(submit->property("text").toString(),QStringLiteral("登录"));
        QCOMPARE(email->property("text").toString(),"example@example.com");
        api.loadDemo(); doc.addRectangle(1,2,10,20,"车辆"); const auto results=doc.results();
        const auto page=window->property("page").toString(); QVERIFY(doc.dirty());
        auto *headerSelector=window->findChild<QObject *>("headerLanguageSelector"); QVERIFY(headerSelector);
        headerSelector->setProperty("currentIndex",1);
        QVERIFY(QMetaObject::invokeMethod(headerSelector,"activated",Q_ARG(int,1)));
        QTRY_COMPARE(submit->property("text").toString(),"Log in");
        QCOMPARE(window->property("page").toString(),page); QCOMPARE(doc.results(),results); QVERIFY(doc.dirty()); QVERIFY(doc.canUndo());
        auto *creation=window->findChild<QObject *>("createProjectDialog"); QVERIFY(creation);
        QVERIFY(QMetaObject::invokeMethod(creation,"open")); creation->setProperty("step",2);
        auto *title=creation->findChild<QObject *>("projectTitle"); QVERIFY(title);
        title->setProperty("text",QStringLiteral("我的 Project"));
        const auto config=creation->findChild<QObject *>("labelConfig")->property("text");
        QVERIFY(language->setLanguage("zh_CN",false));
        QCOMPARE(creation->property("step").toInt(),2); QCOMPARE(title->property("text").toString(),QStringLiteral("我的 Project"));
        QCOMPARE(creation->findChild<QObject *>("labelConfig")->property("text"),config);
        QCOMPARE(doc.results(),results); QVERIFY(doc.dirty());
        QVERIFY(QMetaObject::invokeMethod(creation,"close"));
        QVERIFY(language->setLanguage("en",false));
        QVERIFY2(warnings.isEmpty(),"Switching languages must not cause QML warnings");
    }
    void polygonDrawingVertexEditingAndUndo() {
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider = new TaskImageProvider; engine.addImageProvider("task",provider);
        connect(&api,&ApiClient::imageReady,&api,[=](const QImage &image){ provider->setImage(image); });
        auto *language = new LanguageManager(&engine, nullptr, &engine); language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager", language);
        engine.rootContext()->setContextProperty("backend",&api);
        engine.rootContext()->setContextProperty("annotationDocument",&doc);
        engine.rootContext()->setContextProperty("initialServer","http://127.0.0.1:8080");
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl("qrc:/qml/Main.qml")); QVERIFY(!engine.rootObjects().isEmpty());
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        api.loadDemo();
        QVERIFY(doc.configure("<View><Image name='image' value='$image'/><PolygonLabels name='outline' toName='image'><Label value='Car'/></PolygonLabels></View>"));
        doc.setImageSize(1200,760);
        auto *canvas=window->findChild<QQuickItem *>("annotationCanvas"); QVERIFY(canvas);
        auto *plane=canvas->findChild<QQuickItem *>("imagePlane"); QVERIFY(plane);
        QTRY_VERIFY(plane->width()>100);
        const auto at=[&](double x,double y){ return plane->mapToScene(QPointF(plane->width()*x/100,plane->height()*y/100)).toPoint(); };
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,at(10,10));
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,at(60,10));
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,at(40,60));
        QTest::keyClick(window,Qt::Key_Return);
        QTRY_COMPARE(doc.results().size(),1);
        QCOMPARE(doc.results()[0].toObject()["value"].toObject()["points"].toArray().size(),3);
        const auto original=doc.results(); canvas->setProperty("tool","select");
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,at(10,10));
        QTest::mouseMove(window,at(15,20),30);
        QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,at(15,20));
        const auto changed=doc.results()[0].toObject()["value"].toObject()["points"].toArray()[0].toArray();
        QVERIFY(qAbs(changed[0].toDouble()-15)<0.5); QVERIFY(qAbs(changed[1].toDouble()-20)<0.5);
        doc.undo(); QCOMPARE(doc.results(),original);
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,at(35,30));
        QTest::mouseMove(window,at(40,35),30);
        QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,at(40,35));
        const auto moved=doc.results()[0].toObject()["value"].toObject()["points"].toArray()[0].toArray();
        QVERIFY(qAbs(moved[0].toDouble()-15)<0.5); QVERIFY(qAbs(moved[1].toDouble()-15)<0.5);
        doc.undo(); QCOMPARE(doc.results(),original); doc.markSaved(77);
        canvas->setProperty("tool","draw");
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,at(75,75));
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,at(90,75));
        QVERIFY(!doc.dirty()); QVERIFY(!window->close());
        QTest::keyClick(window,Qt::Key_Escape);
        QVERIFY(QMetaObject::invokeMethod(canvas,"cancelDraft"));
        QVERIFY2(warnings.isEmpty(),"Polygon UI must run without QML warnings");
    }
    void nativeCanvasEditsAndUndo() {
        AnnotationDocument doc; ApiClient api(&doc); QQmlApplicationEngine engine;
        auto *provider = new TaskImageProvider;
        engine.addImageProvider("task", provider);
        connect(&api, &ApiClient::imageReady, &api, [=](const QImage &image) { provider->setImage(image); });
        auto *language = new LanguageManager(&engine, nullptr, &engine); language->setLanguage("en",false);
        engine.rootContext()->setContextProperty("languageManager", language);
        engine.rootContext()->setContextProperty("backend", &api);
        engine.rootContext()->setContextProperty("annotationDocument", &doc);
        engine.rootContext()->setContextProperty("initialServer", "http://127.0.0.1:8080");
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        api.loadDemo();
        auto *canvas = window->findChild<QQuickItem *>("annotationCanvas");
        QVERIFY2(canvas, "Native annotation canvas must be present");
        auto *plane = canvas->findChild<QQuickItem *>("imagePlane");
        QVERIFY(plane);
        QTRY_VERIFY(plane->width() > 100);
        const auto at = [&](double x, double y) { return plane->mapToScene(QPointF(plane->width()*x/100, plane->height()*y/100)).toPoint(); };
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at(10,10));
        QTest::mouseMove(window, at(30,30), 30);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at(30,30));
        QTRY_COMPARE(doc.rectangles().size(), 4);
        QVERIFY(doc.dirty());
        QTest::keyClick(window, Qt::Key_Z, Qt::ControlModifier);
        QTRY_COMPARE(doc.rectangles().size(), 3);
        QTest::keyClick(window, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(doc.rectangles().size(), 4);
        canvas->setProperty("tool", "select");
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at(20,20));
        QTest::mouseMove(window, at(25,25), 30);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at(25,25));
        QVERIFY(qAbs(doc.rectangles().last().toMap()["x"].toDouble()-15.0) < 0.5);
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at(35,35));
        QTest::mouseMove(window, at(40,45), 30);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at(40,45));
        QVERIFY(qAbs(doc.rectangles().last().toMap()["width"].toDouble()-25.0) < 0.7);
        QVERIFY(!window->close());
        QVERIFY(doc.dirty());
        QTest::keyClick(window, Qt::Key_Escape);
        QQmlProperty(window, "activeLabel").write("车辆");
        QVERIFY(doc.configure("<View><Image name='image' value='$image'/><RectangleLabels name='bbox' toName='image'><Label value='Tree'/></RectangleLabels></View>"));
        QCOMPARE(window->property("activeLabel").toString(), "Tree");
        auto *connection = window->findChild<QObject *>("connectionDialog");
        QVERIFY(connection);
        QVERIFY(QMetaObject::invokeMethod(connection, "open"));
        QTRY_VERIFY(connection->property("opened").toBool());
        auto *token = connection->findChild<QObject *>("serverToken");
        QVERIFY(token);
        token->setProperty("text", "dialog-test-token");
        QVERIFY(QMetaObject::invokeMethod(connection, "accept"));
        QVERIFY2(api.busy(), qPrintable("Accepting credentials: " + api.message()));
        QTRY_VERIFY(!api.busy());
        QTRY_VERIFY(token->property("text").toString().isEmpty());
        QVERIFY2(warnings.isEmpty(), "QML must run without warnings");
    }
};

int main(int argc, char **argv) {
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    QGuiApplication app(argc, argv);
    app.setOrganizationName("LabelStudioDesktopTests"); app.setApplicationName("UI");
    QQuickStyle::setStyle("Material");
    registerLabelingTypes();
    UiTest test; return QTest::qExec(&test, argc, argv);
}
#include "tst_ui.moc"
