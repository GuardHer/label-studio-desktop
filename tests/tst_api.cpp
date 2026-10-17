#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QBuffer>
#include <QUrlQuery>
#include <QTemporaryDir>
#include <QFile>
#include "apiclient.h"
#include "accountmanager.h"

static const QString config = QStringLiteral(R"(<View><Image name="image" value="$image"/><RectangleLabels name="bbox" toName="image"><Label value="Car"/></RectangleLabels></View>)");

class HttpFixture : public QTcpServer {
public:
    struct Request { QByteArray method, path, auth, body, cookie, csrf, contentType; };
    QList<Request> requests;
    bool failSave = false;
    bool failLogin = false;
    bool workflow = false;
    bool failAction = false;
    bool failQuery = false;
    bool shrinkOnAction = false;
    int workflowTotal = 51;
    int failTask = -1;
    QString mediaSource = "/media.png";
    QString projectConfig = config;
    QJsonObject extraTaskData;
    bool orientedJpeg = false;
    QJsonArray taskAnnotations;
    QJsonObject accountProfile{{"id",1},{"email","user@example.com"},{"first_name","Test"},{"last_name",""},{"phone",""},{"active_organization",3},{"allow_newsletters",false}};
    QJsonObject organization{{"id",3},{"title","Test org"},{"contact_info","contact@example.com"},{"created_by",1}};
    QJsonObject policy{{"api_tokens_enabled",true},{"legacy_api_tokens_enabled",false}};
    QJsonObject globalKeys, projectKeys;
    bool failAccountSave=false;
    bool expireNextAccountWrite=false;
    bool failProjectMaintenance=false;
    bool failProjectList=false;
    bool failViewSave=false;
    QJsonArray dataViews;
    int sampledTasks=0;
    QJsonObject managedProject{{"id",7},{"title","Managed"},{"description",""},{"label_config",config},{"color","#FFFFFF"},{"sampling","Sequential sampling"}};
    HttpFixture() {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto *s = nextPendingConnection();
                connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
                connect(s, &QTcpSocket::readyRead, this, [this, s] {
                    auto data = s->property("buffer").toByteArray() + s->readAll(); s->setProperty("buffer", data);
                    const auto split = data.indexOf("\r\n\r\n");
                    if (split < 0 || s->property("done").toBool()) return;
                    const auto headers = data.left(split).split('\n'); Request r;
                    const auto first = headers.first().split(' '); r.method = first.value(0); r.path = first.value(1);
                    int length = 0;
                    for (const auto &line : headers) {
                        if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                        if (line.toLower().startsWith("authorization:")) r.auth = line.mid(14).trimmed();
                        if (line.toLower().startsWith("cookie:")) r.cookie = line.mid(7).trimmed();
                        if (line.toLower().startsWith("x-csrftoken:")) r.csrf = line.mid(12).trimmed();
                        if (line.toLower().startsWith("content-type:")) r.contentType = line.mid(13).trimmed();
                    }
                    if (data.size() < split+4+length) return;
                    s->setProperty("done", true); r.body = data.mid(split+4, length); requests.append(r);
                    int status = 200; QByteArray mime = "application/json", payload, extra;
                    if ((r.path == "/user/login/" || r.path.startsWith("/user/signup/")) && r.method == "GET") {
                        mime = "text/html"; payload = "<form>Login</form>";
                        extra = "Set-Cookie: csrftoken=csrf-fixture; Path=/\r\n";
                    } else if ((r.path == "/user/login/" || r.path.startsWith("/user/signup/")) && r.method == "POST") {
                        mime = "text/html";
                        if (failLogin) payload = "<form>Invalid credentials</form>";
                        else { status = 302; extra = "Location: /\r\nSet-Cookie: sessionid=session-fixture; Path=/; HttpOnly\r\n"; }
                    } else if (r.path == "/api/current-user/whoami") payload = QJsonDocument(accountProfile).toJson();
                    else if (r.path.startsWith("/api/current-user/hotkeys/")) {
                        auto &keys=r.path.contains("project=")?projectKeys:globalKeys;
                        if(r.method=="PATCH")keys=QJsonDocument::fromJson(r.body).object().value("custom_hotkeys").toObject();
                        payload=QJsonDocument(QJsonObject{{"custom_hotkeys",keys}}).toJson();
                    } else if (r.path=="/api/users/1/" && r.method=="PATCH") {
                        if(expireNextAccountWrite){expireNextAccountWrite=false;status=401;payload=R"({"detail":"Token expired"})";}
                        else if(failAccountSave){status=400;payload=R"({"phone":["Invalid phone"]})";}
                        else {const auto changes=QJsonDocument::fromJson(r.body).object();for(auto i=changes.begin();i!=changes.end();++i)accountProfile[i.key()]=i.value();payload=QJsonDocument(accountProfile).toJson();}
                    } else if(r.path=="/api/organizations/3") {
                        if(r.method=="PATCH"){const auto changes=QJsonDocument::fromJson(r.body).object();for(auto i=changes.begin();i!=changes.end();++i)organization[i.key()]=i.value();}
                        payload=QJsonDocument(organization).toJson();
                    } else if(r.path.startsWith("/api/organizations/3/memberships?"))payload=QJsonDocument(QJsonObject{{"count",1},{"results",QJsonArray{QJsonObject{{"id",91},{"user",accountProfile}}}}}).toJson();
                    else if(r.path.startsWith("/api/organizations/3/memberships/1/"))payload=R"({"user":1,"organization":3,"annotations_count":8,"contributed_projects_count":2})";
                    else if(r.path=="/api/invite")payload=R"({"invite_url":"/user/signup/?token=invite-test"})";
                    else if(r.path=="/api/invite/reset-token")payload=R"({"invite_url":"/user/signup/?token=new-invite"})";
                    else if(r.path=="/api/jwt/settings"){
                        if(r.method=="POST")policy=QJsonDocument::fromJson(r.body).object();payload=QJsonDocument(policy).toJson();
                    } else if(r.path=="/api/token/" && r.method=="GET")payload="[]";
                    else if(r.path=="/api/token/" && r.method=="POST")payload=R"({"token":"new-personal-secret"})";
                    else if(r.path=="/api/token/blacklist/"){status=204;}
                    else if(r.path=="/logout/"){status=302;extra="Location: /user/login/\r\n";}
                    else if (r.path == "/api/projects/" && r.method == "POST") {
                        auto project = QJsonDocument::fromJson(r.body).object(); project["id"] = 7;
                        managedProject=project;payload = QJsonDocument(project).toJson();
                    } else if (r.path == "/api/projects/7/" && r.method == "PATCH") {
                        const auto changes=QJsonDocument::fromJson(r.body).object();for(auto i=changes.begin();i!=changes.end();++i)managedProject[i.key()]=i.value();
                        payload = QJsonDocument(managedProject).toJson();
                    } else if(r.path=="/api/projects/7/" && r.method=="GET")payload=QJsonDocument(managedProject).toJson();
                    else if(r.path=="/api/projects/7/summary/reset/" || r.path=="/api/dm/views/reset/"){
                        status=failProjectMaintenance?403:r.method=="DELETE"?204:200;
                        if(failProjectMaintenance)payload=R"({"detail":"Permission denied"})";
                    } else if(r.path.startsWith("/api/projects/7/next/")){
                        payload=QJsonDocument(QJsonObject{{"id",9+sampledTasks++},{"data",QJsonObject{{"image","/media.png"}}},{"annotations",QJsonArray{}}}).toJson();
                    } else if (r.path == "/api/projects/7/" && r.method == "DELETE") { status = 204; }
                    else if (r.path.startsWith("/api/projects/7/import")) payload = R"({"task_count":1})";
                    else if (r.path.startsWith("/api/projects/7/export")) payload = R"([{"id":9,"annotations":[]}])";
                    else if (r.path.startsWith("/api/token/refresh/")) payload = R"({"access":"access-token"})";
                    else if (r.path.startsWith("/api/projects/?")) {
                        const bool second = QUrlQuery(QUrl(QString::fromUtf8(r.path))).queryItemValue("page") == "2";
                        if(failProjectList&&second){status=500;payload=R"({"detail":"List unavailable"})";}
                        else payload = QJsonDocument(QJsonObject{{"count", 2}, {"results", QJsonArray{QJsonObject{{"id", second ? 2 : 1}, {"title", second ? "Other" : "Cars"}}}}}).toJson();
                    } else if (r.path == "/api/projects/1/") payload = QJsonDocument(QJsonObject{{"id", 1}, {"title", "Cars"}, {"label_config", projectConfig}}).toJson();
                    else if(r.path.startsWith("/api/dm/columns/"))payload=R"({"columns":[{"id":"id","title":"ID","type":"Number","target":"tasks","visibility_defaults":{"explore":true}},{"id":"created_at","title":"Created at","type":"Datetime","target":"tasks","visibility_defaults":{"explore":false}},{"id":"total_annotations","title":"Annotations","type":"Number","target":"tasks","visibility_defaults":{"explore":true}},{"id":"annotators","title":"Annotated by","type":"List","target":"tasks","visibility_defaults":{"explore":true}},{"id":"image","title":"image","type":"Image","target":"tasks","parent":"data","visibility_defaults":{"explore":true}}]})";
                    else if(r.path.startsWith("/api/dm/projects/"))payload=R"({"results":[{"id":1,"email":"user@example.com"}]})";
                    else if(r.path.startsWith("/api/dm/views/?")&&r.method=="GET")payload=QJsonDocument(dataViews).toJson();
                    else if(r.path.startsWith("/api/dm/views/")&&r.method!="GET"){
                        if(failViewSave){status=403;payload=R"({"detail":"Read only view"})";}
                        else if(r.method=="DELETE"){dataViews={};status=204;}
                        else{auto v=QJsonDocument::fromJson(r.body).object();v["id"]=8;dataViews=QJsonArray{v};payload=QJsonDocument(v).toJson();}
                    }
                    else if(r.path.startsWith("/api/dm/actions/")&&r.path.contains("/form/"))payload=R"([{"fields":[{"name":"annotator","type":"select","label":"Annotator","options":[{"value":"1","label":"user@example.com"}]}]}])";
                    else if(r.path.startsWith("/api/dm/actions/?")&&r.method=="GET")payload=R"([{"id":"delete_tasks","title":"Delete Tasks"},{"id":"delete_tasks_annotations","title":"Delete Annotations","dialog":{"form":true}},{"id":"retrieve_tasks_predictions","title":"Retrieve Predictions"},{"id":"predictions_to_annotations","title":"Create Annotations From Predictions"},{"id":"remove_duplicates","title":"Remove Duplicated Tasks"}])";
                    else if (workflow && r.path.startsWith("/api/dm/actions/")) { status = failAction ? 403 : 200; if (!failAction && shrinkOnAction) workflowTotal=50; payload = failAction ? R"({"detail":"Permission denied"})" : R"({"processed_items":1})"; }
                    else if (workflow && r.path.startsWith("/api/tasks/?")) {
                        const auto requestQuery=QUrlQuery(QUrl(QString::fromUtf8(r.path)));const int page = requestQuery.queryItemValue("page").toInt();const int size=requestQuery.queryItemValue("page_size").toInt();
                        QJsonArray tasks;
                        for (int id=(page-1)*size+1; id<=qMin(page*size,workflowTotal); ++id) tasks.append(QJsonObject{{"id",id},{"data",QJsonObject{{"image",mediaSource}}}});
                        if (failQuery) { status=400; payload=R"({"detail":"Bad query"})"; }
                        else if (page>qMax(1,(workflowTotal+size-1)/size)) { status=404; payload=R"({"detail":"Invalid page."})"; }
                        else payload=QJsonDocument(QJsonObject{{"total",workflowTotal},{"tasks",tasks}}).toJson();
                    }
                    else if (workflow && r.method=="GET" && QRegularExpression("^/api/tasks/[0-9]+/$").match(QString::fromUtf8(r.path)).hasMatch()) {
                        const int id=r.path.split('/').value(3).toInt();
                        if (id==failTask) { status=500; payload=R"({"detail":"Task unavailable"})"; }
                        else payload=QJsonDocument(QJsonObject{{"id",id},{"data",QJsonObject{{"image",mediaSource}}},{"annotations",taskAnnotations}}).toJson();
                    }
                    else if (r.path.startsWith("/api/tasks/?")) {auto taskData=extraTaskData;taskData["image"]=mediaSource;payload = QJsonDocument(QJsonObject{{"total",51},{"tasks",QJsonArray{QJsonObject{{"id",9},{"is_labeled",false},{"data",taskData}}}}}).toJson();}
                    else if (r.path == "/api/tasks/9/" && r.method == "GET") { auto taskData=extraTaskData;taskData["image"]=mediaSource;payload = QJsonDocument(QJsonObject{{"id", 9}, {"data", taskData}, {"annotations", taskAnnotations}}).toJson(); }
                    else if (r.path == "/media.png" || r.path == "/media-other.png") {
                        mime = "image/png"; QImage image(120, 80, QImage::Format_RGB32); image.fill(r.path=="/media.png"?Qt::cyan:Qt::magenta);
                        QBuffer buffer(&payload); buffer.open(QIODevice::WriteOnly); image.save(&buffer, orientedJpeg ? "JPEG" : "PNG");
                        if (orientedJpeg) {
                            mime = "image/jpeg";
                            payload.insert(2, QByteArray::fromHex("ffe1002245786966000049492a0008000000010012010300010000000600000000000000"));
                        }
                    } else if (workflow && (r.path.contains("/annotations/") || r.path.startsWith("/api/annotations/"))) {
                        status = failSave ? 400 : r.method=="DELETE" ? 204 : 200;
                        payload = failSave ? R"({"detail":"Validation rejected"})" : R"({"id":84})";
                    } else if (r.path == "/api/tasks/9/annotations/" || r.path == "/api/annotations/84/") {
                        status = failSave ? 400 : 200;
                        payload = failSave ? R"({"detail":"Validation rejected"})" : R"({"id":84})";
                    } else { status = 404; payload = R"({"detail":"unknown"})"; }
                    s->write("HTTP/1.1 " + QByteArray::number(status) + " Response\r\n" + extra + "Content-Type: " + mime + "\r\nContent-Length: " + QByteArray::number(payload.size()) + "\r\nConnection: close\r\n\r\n" + payload);
                    s->disconnectFromHost();
                });
            }
        });
    }
    QString url() const { return QString("http://127.0.0.1:%1").arg(serverPort()); }
};

class ApiTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){QCoreApplication::setOrganizationName("LabelStudioDesktopTests");}
    void dataColumnsUseTheirOwnAuthenticatedThumbnails() {
        HttpFixture server;server.extraTaskData={{"other","/media-other.png"}};AnnotationDocument doc;ApiClient api(&doc);api.connectTo(server.url(),"test-secret",0);QTRY_VERIFY(!api.busy());api.openProject(1);QTRY_VERIFY(!api.busy());
        api.requestThumbnail(9,"data.image");api.requestThumbnail(9,"data.other");QTRY_VERIFY(api.thumbnails().contains("9:data.image")&&api.thumbnails().contains("9:data.other"));
        QVERIFY(api.thumbnails().value("9:data.image")!=api.thumbnails().value("9:data.other"));int mediaCount=0;for(const auto&r:server.requests)if(r.path=="/media.png"||r.path=="/media-other.png"){++mediaCount;QCOMPARE(r.auth,"Token test-secret");}QCOMPARE(mediaCount,2);
    }
    void nativeTextTaskValidatesAndSavesWithoutImage() {
        HttpFixture server;server.projectConfig="<View><Text name='body' value='$text'/><Choices name='sentiment' toName='body' required='true'><Choice value='Good'/><Choice value='Bad'/></Choices><TextArea name='note' toName='body'/></View>";
        server.extraTaskData={{"text","A native text task"}};
        AnnotationDocument doc;ApiClient api(&doc);api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(1);QTRY_VERIFY(!api.busy());api.openTask(9);QTRY_VERIFY(!api.busy());
        QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QVERIFY(doc.editable());QVERIFY(doc.imageFrames().isEmpty());
        QSignalSpy saved(&api,&ApiClient::saved);api.save();QTRY_VERIFY(!api.busy());QCOMPARE(saved.size(),0);QVERIFY(!doc.validationError().isEmpty());
        doc.setControlValue("sentiment",QStringList{"Good"});doc.setControlValue("note",QStringList{"Works without an image"});api.save();QTRY_VERIFY(!api.busy());QCOMPARE(saved.size(),1);QVERIFY(!doc.dirty());QCOMPARE(doc.results().size(),2);
    }
    void nativeImageFramesResolveTaskDataAndReloadOnSelection() {
        HttpFixture server;server.projectConfig="<View><Image name='photos' valueList='$photos'/><RectangleLabels name='boxes' toName='photos'><Label value='Car'/></RectangleLabels></View>";
        server.extraTaskData={{"photos",QJsonArray{"/media.png","/media.png?frame=2"}}};
        AnnotationDocument doc;ApiClient api(&doc);api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(1);QTRY_VERIFY(!api.busy());api.openTask(9);QTRY_VERIFY(!api.busy());
        QVERIFY2(!api.messageIsError(),qPrintable(api.message()));QCOMPARE(doc.imageFrames().size(),2);QCOMPARE(doc.activeImageSource(),"/media.png");QVERIFY(!api.imageSource().isEmpty());
        const auto previous=api.imageSource();doc.selectImage(1);QTRY_VERIFY(!api.busy());QCOMPARE(doc.activeImageSource(),"/media.png?frame=2");
        QVERIFY(api.imageSource().isEmpty()||api.imageSource()!=previous);
    }
    void dataManagerViewsColumnsAndTypedFilters() {
        HttpFixture server;server.workflow=true;AnnotationDocument doc;ApiClient api(&doc);api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(1);QTRY_VERIFY(!api.busy());auto *dm=api.dataManager();
        QCOMPARE(dm->columns().size(),5);QCOMPARE(dm->visibleColumns().size(),4);
        QCOMPARE(dm->makeFilter("data.image","contains","upload").value("type").toString(),"String");
        const auto numericData=dm->makeFilter("data.image","greater","50",{},"Number");QCOMPARE(numericData.value("type").toString(),"Number");QCOMPARE(numericData.value("value").toDouble(),50.0);QVERIFY(dm->makeFilter("data.image","greater","invalid",{},"Number").isEmpty());bool numericOperator=false;for(const auto &op:dm->operators("data.image","Number"))if(op.toMap().value("key")=="greater")numericOperator=true;QVERIFY(numericOperator);QVERIFY(dm->makeFilter("data.image","equal","invalid",{},"Boolean").isEmpty());QCOMPARE(dm->makeFilter("data.image","equal","false",{},"Boolean").value("value").toBool(),false);
        QCOMPARE(dm->makeFilter("total_annotations","in","1","3").value("value").toMap(),(QVariantMap{{"min",1.0},{"max",3.0}}));QVERIFY(dm->makeFilter("total_annotations","equal","bad").isEmpty());
        QCOMPARE(dm->makeFilter("annotators","contains","1,2").value("value").toStringList(),(QStringList{"1","2"}));QVERIFY(dm->makeFilter("created_at","greater","not a date").isEmpty());
        dm->setColumnVisible("created_at",true);QTRY_VERIFY(!api.busy());dm->setColumnWidth("created_at",300);dm->moveColumn("created_at",-1);dm->setCompact(true);
        dm->setPageSize(10);QTRY_VERIFY(!api.busy());QCOMPARE(api.tasks().size(),10);api.loadPage(6);QTRY_VERIFY(!api.busy());QCOMPARE(api.tasks().size(),1);
        const QVariantMap filter{{"conjunction","and"},{"items",QVariantList{dm->makeFilter("total_annotations","greater","2")}}};dm->applyQuery(filter,{"-tasks:created_at"});QTRY_VERIFY(!api.busy());
        dm->createView("My view");QTRY_VERIFY(!api.busy());QCOMPARE(dm->activeView(),8);QCOMPARE(dm->views().size(),1);QVERIFY(!dm->viewDirty());
        dm->setCompact(false);server.failViewSave=true;dm->saveView();QTRY_VERIFY(!api.busy());QVERIFY(dm->viewDirty());server.failViewSave=false;dm->saveView();QTRY_VERIFY(!api.busy());QVERIFY(!dm->viewDirty());
        dm->activateView(0);QTRY_VERIFY(!api.busy());dm->activateView(8);QTRY_VERIFY(!api.busy());QCOMPARE(dm->filters(),filter);QCOMPARE(dm->pageSize(),10);QVERIFY(!dm->compact());
        dm->renameView("Renamed");QTRY_VERIFY(!api.busy());QCOMPARE(dm->views().first().toMap().value("data").toMap().value("title").toString(),"Renamed");
        api.selectTask(1,true);QSignalSpy ready(dm,&DataManager::actionReady);dm->prepareAction("delete_tasks_annotations");QTRY_VERIFY(!api.busy());QCOMPARE(ready.size(),1);QCOMPARE(dm->actionFields().size(),1);
        dm->executeAction("delete_tasks_annotations",{{"annotator","1"}});QTRY_VERIFY(!api.busy());bool sent=false;for(const auto&r:server.requests)if(r.method=="POST"&&r.path.contains("id=delete_tasks_annotations")){sent=true;QCOMPARE(QJsonDocument::fromJson(r.body).object().value("annotator").toString(),"1");}QVERIFY(sent);
        dm->deleteView();QTRY_VERIFY(!api.busy());QCOMPARE(dm->activeView(),0);QVERIFY(dm->views().isEmpty());
    }
    void selectedSamplingQueueIncludesOnlySelectionAndExcludesVisited() {
        HttpFixture server;server.workflow=true;AnnotationDocument doc;ApiClient api(&doc);api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(7);QTRY_VERIFY(!api.busy());api.selectTask(9,true);api.selectTask(10,true);api.startLabeling();QTRY_VERIFY(!api.busy());api.moveTask(1);QTRY_VERIFY(!api.busy());
        QList<QJsonObject> queries;for(const auto&r:server.requests)if(r.path.startsWith("/api/projects/7/next/"))queries.append(QJsonDocument::fromJson(QUrlQuery(QUrl(QString::fromUtf8(r.path))).queryItemValue("query",QUrl::FullyDecoded).toUtf8()).object());
        QCOMPARE(queries.size(),2);QCOMPARE(queries[0].value("selectedItems").toObject(),(QJsonObject{{"all",false},{"included",QJsonArray{9,10}}}));QCOMPARE(queries[1].value("selectedItems").toObject().value("included").toArray(),QJsonArray{10});QSignalSpy finished(&api,&ApiClient::workflowFinished);api.moveTask(1);QCOMPARE(finished.size(),1);QCOMPARE(server.sampledTasks,2);
    }
    void dataManagerProtocolAndFailurePreservation() {
        HttpFixture server;server.workflow=true;AnnotationDocument doc;ApiClient api(&doc);
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(1);QTRY_VERIFY(!api.busy());
        QObject *dm=api.property("dataManager").value<QObject*>();QVERIFY2(dm,"Data Manager controller missing");
        const QVariantMap filters{{"conjunction","or"},{"items",QVariantList{QVariantMap{{"filter","filter:tasks:total_annotations"},{"type","Number"},{"operator","greater"},{"value",2}}}}};
        QVERIFY(QMetaObject::invokeMethod(dm,"applyQuery",Q_ARG(QVariantMap,filters),Q_ARG(QStringList,QStringList{"-tasks:created_at"})));QTRY_VERIFY(!api.busy());
        const auto lastQuery=[&] {for(auto i=server.requests.crbegin();i!=server.requests.crend();++i)if(i->path.startsWith("/api/tasks/?"))return QJsonDocument::fromJson(QUrlQuery(QUrl(QString::fromUtf8(i->path))).queryItemValue("query",QUrl::FullyDecoded).toUtf8()).object();return QJsonObject{};};
        QCOMPARE(lastQuery().value("filters").toObject(),QJsonObject::fromVariantMap(filters));QCOMPARE(lastQuery().value("ordering").toArray(),QJsonArray{"-tasks:created_at"});
        QVERIFY(QMetaObject::invokeMethod(&api,"selectAllResults"));QCOMPARE(api.selectedTaskCount(),51);api.selectTask(1,false);QCOMPARE(api.selectedTaskCount(),50);
        server.failAction=true;api.runTaskAction("delete_tasks");QTRY_VERIFY(!api.busy());QCOMPARE(api.selectedTaskCount(),50);
        server.failAction=false;api.runTaskAction("delete_tasks");QTRY_VERIFY(!api.busy());
        for(auto i=server.requests.crbegin();i!=server.requests.crend();++i)if(i->method=="POST"&&i->path.startsWith("/api/dm/actions/")){
            const auto body=QJsonDocument::fromJson(i->body).object();QCOMPARE(body.value("filters").toObject(),QJsonObject::fromVariantMap(filters));QCOMPARE(body.value("selectedItems").toObject(),(QJsonObject{{"all",true},{"excluded",QJsonArray{1}}}));break;
        }
        QCOMPARE(api.selectedTaskCount(),0);server.failQuery=true;
        QVERIFY(QMetaObject::invokeMethod(dm,"applyQuery",Q_ARG(QVariantMap,(QVariantMap{{"conjunction","and"},{"items",QVariantList{}}})),Q_ARG(QStringList,QStringList{"tasks:id"})));QTRY_VERIFY(!api.busy());
        QCOMPARE(dm->property("filters").toMap(),filters);
    }
    void projectCatalogFiltersSortsAndPreservesRefreshOnFailure() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());
        auto *catalog=api.catalog();QCOMPARE(catalog->items().size(),2);catalog->setSearch("cAr");QCOMPARE(catalog->items().size(),1);QCOMPARE(catalog->items().first().toMap().value("id").toInt(),1);
        catalog->setSearch("2");QCOMPARE(catalog->items().size(),1);catalog->setSearch("");catalog->setOrder("title");QCOMPARE(catalog->items().first().toMap().value("title").toString(),"Cars");
        catalog->forget(1);catalog->forget(2);catalog->recordVisit(2);catalog->recordVisit(1);QCOMPARE(catalog->recent().first().toMap().value("id").toInt(),1);
        catalog->setOrder("recent");QCOMPARE(catalog->items().first().toMap().value("id").toInt(),1);catalog->forget(1);QCOMPARE(catalog->recent().first().toMap().value("id").toInt(),2);
        const auto previous=api.projects();server.failProjectList=true;api.refreshProjects();QTRY_VERIFY(!api.busy());QCOMPARE(api.projects(),previous);QVERIFY(api.messageIsError());
    }
    void projectManagementControllerAndMaintenance() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());
        QVERIFY2(api.metaObject()->indexOfProperty("catalog")>=0,"Project catalog is missing");
        api.createProject("Empty project","","");QTRY_VERIFY(!api.busy());QCOMPARE(api.project().value("label_config").toString(),"<View></View>");QVERIFY(!doc.editable());
        QVERIFY(QMetaObject::invokeMethod(&api,"updateProjectDetails",Q_ARG(QString,"Colored"),Q_ARG(QString,"Description"),Q_ARG(QString,config),Q_ARG(QString,"#ff8844"),Q_ARG(QString,"Uniform sampling")));
        QTRY_VERIFY(!api.busy());QCOMPARE(api.project().value("color").toString(),"#ff8844");QCOMPARE(api.project().value("sampling").toString(),"Uniform sampling");
        QVERIFY(QMetaObject::invokeMethod(&api,"resetProjectCache"));QTRY_VERIFY(!api.busy());QVERIFY2(!api.messageIsError(),qPrintable(api.message()));
        api.setTaskQuery("9",2,true);QTRY_VERIFY(!api.busy());server.failProjectMaintenance=true;
        QVERIFY(QMetaObject::invokeMethod(&api,"dropProjectTabs"));QTRY_VERIFY(!api.busy());QCOMPARE(api.taskSearch(),"9");QVERIFY(api.messageIsError());
        server.failProjectMaintenance=false;QVERIFY(QMetaObject::invokeMethod(&api,"dropProjectTabs"));QTRY_VERIFY(!api.busy());QCOMPARE(api.taskSearch(),"");QCOMPARE(api.taskStatus(),0);QVERIFY(!api.taskDescending());
        for(const auto &request:server.requests)if(request.path=="/api/dm/views/reset/"){QCOMPARE(request.method,"DELETE");QCOMPARE(QJsonDocument::fromJson(request.body).object().value("project").toInt(),7);QCOMPARE(request.csrf,"csrf-fixture");}
    }
    void projectSamplingUsesBackendNextWithoutChangingListNavigation() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(7);QTRY_VERIFY(!api.busy());
        QVERIFY2(QMetaObject::invokeMethod(&api,"startLabeling"),"Native sampling queue is missing");QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),9);
        QVERIFY(api.property("samplingQueue").toBool());
        QVERIFY(std::any_of(server.requests.begin(),server.requests.end(),[](const auto &r){return r.path.startsWith("/api/projects/7/next/");}));
        api.openTask(9);QTRY_VERIFY(!api.busy());QVERIFY(!api.property("samplingQueue").toBool());
    }
    void samplingSubmitAndSkipPreserveForwardHistory() {
        HttpFixture server;server.workflow=true;AnnotationDocument doc;ApiClient api(&doc);
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());api.openProject(7);QTRY_VERIFY(!api.busy());
        api.startLabeling();QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),9);
        api.moveTask(1);QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),10);QCOMPARE(server.sampledTasks,2);
        api.moveTask(-1);QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),9);
        doc.addRectangle(10,10,30,30,"Car");api.saveAndNext();QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),10);QCOMPARE(server.sampledTasks,2);
        api.moveTask(-1);QTRY_VERIFY(!api.busy());api.skipTask();QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),10);QCOMPARE(server.sampledTasks,2);
        api.moveTask(1);QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),11);QCOMPARE(server.sampledTasks,3);
        const auto it=std::find_if(server.requests.crbegin(),server.requests.crend(),[](const auto &r){return r.path.startsWith("/api/projects/7/next/");});QVERIFY(it!=server.requests.crend());
        const auto query=QJsonDocument::fromJson(QUrlQuery(QUrl(QString::fromUtf8(it->path))).queryItemValue("query",QUrl::FullyDecoded).toUtf8()).object();
        QCOMPARE(query.value("selectedItems").toObject().value("excluded").toArray(),(QJsonArray{9,10}));
        server.failTask=12;api.startLabeling();QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),11);QCOMPARE(api.samplingPosition(),3);
        api.moveTask(-1);QTRY_VERIFY(!api.busy());QCOMPARE(api.taskId(),10);
    }
    void accountMutationRefreshesExpiredAccessToken() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);
        api.connectTo(server.url(),"refresh-secret",1);QTRY_VERIFY(!api.busy());QVERIFY(api.connected());
        api.account()->load();QTRY_VERIFY(!api.busy());server.requests.clear();server.expireNextAccountWrite=true;
        api.account()->saveProfile("Refreshed","User","");QTRY_VERIFY(!api.busy());
        QCOMPARE(api.account()->profile().value("first_name").toString(),"Refreshed");QVERIFY(!api.messageIsError());
        int writes=0,refreshes=0;for(const auto &request:server.requests){if(request.path=="/api/users/1/")++writes;if(request.path=="/api/token/refresh/")++refreshes;}
        QCOMPARE(writes,2);QCOMPARE(refreshes,1);
    }
    void nativeSignupPreservesInviteAndFormEncoding() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);
        api.signup(server.url(),"new+person@example.com","safe+password&one",server.url()+"/user/signup/?token=invite%2Btest",true);
        QTRY_VERIFY(!api.busy());QVERIFY2(api.connected(),qPrintable(api.message()));
        bool posted=false;
        for(const auto &r:server.requests)if(r.method=="POST"&&r.path.startsWith("/user/signup/")){
            posted=true;QVERIFY(r.path.contains("token=invite%2Btest"));const QUrlQuery form(QString::fromUtf8(r.body));
            QCOMPARE(form.queryItemValue("email",QUrl::FullyDecoded),"new+person@example.com");
            QCOMPARE(form.queryItemValue("password",QUrl::FullyDecoded),"safe+password&one");
            QCOMPARE(form.queryItemValue("allow_newsletters"),"on");QVERIFY(!r.body.contains('+'));QVERIFY(r.cookie.contains("csrftoken=csrf-fixture"));
        }QVERIFY(posted);
    }
    void nativeAccountReadsAndWritesWithoutGeneratingSecretsOnLoad() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());QVERIFY(api.connected());
        auto *account=api.account();account->load();QTRY_VERIFY(!api.busy());
        QCOMPARE(account->organization().value("title").toString(),"Test org");QCOMPARE(account->memberCount(),1);
        QVERIFY(account->secret().isEmpty());QVERIFY(account->inviteUrl().isEmpty());
        for(const auto &r:server.requests)QVERIFY(r.method=="GET" || r.path=="/user/login/");
        account->saveProfile("Native","User","123");QTRY_VERIFY(!api.busy());
        QCOMPARE(api.user().value("first_name").toString(),"Native");
        server.failAccountSave=true;account->saveProfile("Lost","User","bad");QTRY_VERIFY(!api.busy());
        QCOMPARE(account->profile().value("first_name").toString(),"Native");QVERIFY(api.messageIsError());
        account->saveOrganization("New organization","new@example.com");QTRY_VERIFY(!api.busy());
        QCOMPARE(account->organization().value("title").toString(),"New organization");
        account->loadMember(1);QTRY_VERIFY(!api.busy());QCOMPARE(account->memberDetail().value("annotations_count").toInt(),8);
        account->loadInvite();QTRY_VERIFY(!api.busy());QVERIFY(account->inviteUrl().startsWith(server.url()));
        account->createToken();QTRY_VERIFY(!api.busy());QCOMPARE(account->secret(),"new-personal-secret");
        account->revokeToken(account->secret());QTRY_VERIFY(!api.busy());QVERIFY(account->secret().isEmpty());
        for(const auto &r:server.requests)if(r.method!="GET" && r.path.startsWith("/api/")){QCOMPARE(r.csrf,"csrf-fixture");QVERIFY(r.cookie.contains("sessionid=session-fixture"));}
        api.disconnectServer();QTRY_VERIFY(!api.busy());QVERIFY(!api.connected());QVERIFY(account->profile().isEmpty());QVERIFY(api.user().isEmpty());
        QVERIFY(std::any_of(server.requests.begin(),server.requests.end(),[](const auto &r){return r.path=="/logout/";}));
    }
    void hotkeysScopeInheritanceAndFileRoundTrip() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());
        auto *account=api.account();account->load();QTRY_VERIFY(!api.busy());
        account->saveHotkeys(R"({"custom_hotkeys":{"editor:save":{"key":"ctrl+shift+s","active":true}}})");QTRY_VERIFY(!api.busy());
        QCOMPARE(account->keySequence("editor:save","Ctrl+S"),"Ctrl+Shift+S");
        account->activateProject(7);QTRY_VERIFY(!api.busy());account->loadHotkeys(7);QTRY_VERIFY(!api.busy());
        QCOMPARE(account->keySequence("editor:save","Ctrl+S"),"Ctrl+Shift+S");
        account->saveHotkeys(R"({"custom_hotkeys":{"editor:save":{"key":"ctrl+s","active":false}}})");QTRY_VERIFY(!api.busy());
        QVERIFY(account->keySequence("editor:save","Ctrl+S").isEmpty());
        QTemporaryDir directory;const auto url=QUrl::fromLocalFile(directory.filePath("keys.json"));
        account->exportHotkeys(url,account->hotkeyJson());QSignalSpy imported(account,&AccountManager::hotkeysImported);account->importHotkeys(url);QCOMPARE(imported.size(),1);
        QCOMPARE(imported.first().first().toString(),account->hotkeyJson());
        const int count=server.requests.size();account->saveHotkeys("not json");QCOMPARE(server.requests.size(),count);QVERIFY(api.messageIsError());
        account->saveHotkeys(R"({"custom_hotkeys":{}})");QTRY_VERIFY(!api.busy());QCOMPARE(account->keySequence("editor:save","Ctrl+S"),"Ctrl+Shift+S");
    }
    void nativeAccountControllerIsAvailable() {
        AnnotationDocument doc; ApiClient api(&doc);
        QVERIFY2(api.metaObject()->indexOfProperty("account") >= 0, "Native account controller is missing");
    }
    void webHotkeysAndEditorScopeDoNotReplaceRuntimeProject() {
        HttpFixture server;AnnotationDocument doc;ApiClient api(&doc);
        server.globalKeys={{"annotation:annotation:undo",QJsonObject{{"key","ctrl+alt+z"},{"active",true}}}};
        server.projectKeys={{"regions:region:delete",QJsonObject{{"key","backspace"},{"active",false}}}};
        api.login(server.url(),"user@example.com","secret");QTRY_VERIFY(!api.busy());
        auto *account=api.account();account->activateProject(7);QTRY_VERIFY(!api.busy());
        QCOMPARE(account->keySequence("annotation:undo","Ctrl+Z"),"Ctrl+Alt+Z");
        QVERIFY(account->keySequence("region:delete","Delete").isEmpty());
        server.projectKeys={{"regions:region:delete",QJsonObject{{"key","ctrl+d"},{"active",true}}}};
        account->loadHotkeys(8);QTRY_VERIFY(!api.busy());
        QVERIFY(account->keySequence("region:delete","Delete").isEmpty());
        account->load();QTRY_VERIFY(!api.busy());
        QCOMPARE(account->keySequence("annotation:undo","Ctrl+Z"),"Ctrl+Alt+Z");
    }
    void savedCrossPageFailureKeepsSavedTaskNavigable() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy()); api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(50); QTRY_VERIFY(!api.busy()); doc.addRectangle(1,2,30,40,"Car");
        server.failTask=51; api.saveAndNext(); QTRY_VERIFY(!api.busy());
        QVERIFY(!doc.dirty()); QCOMPARE(doc.annotationId(),84); QCOMPARE(api.taskId(),50); QCOMPARE(api.page(),1);
        server.failTask=-1; api.moveTask(1); QTRY_VERIFY(!api.busy()); QCOMPARE(api.taskId(),51);
    }
    void groundTruthUniquenessAndFailedCrossPageOpen() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        server.taskAnnotations=QJsonArray{QJsonObject{{"id",84},{"result",QJsonArray{}},{"ground_truth",true}}, QJsonObject{{"id",85},{"result",QJsonArray{}}}};
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy()); api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(50); QTRY_VERIFY(!api.busy()); QVERIFY(api.groundTruth());
        api.selectAnnotation(85); api.setGroundTruth(true); QTRY_VERIFY(!api.busy()); QVERIFY(api.groundTruth());
        api.selectAnnotation(84); QVERIFY2(!api.groundTruth(),"Django permits one ground truth per task");
    }
    void failedCrossPageOpenCanRetry() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy()); api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(50); QTRY_VERIFY(!api.busy());
        server.failTask=51; api.moveTask(1); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),50); QCOMPARE(api.page(),1); QCOMPARE(api.tasks().last().toMap().value("id").toInt(),50);
        server.failTask=-1; api.moveTask(1); QTRY_VERIFY(!api.busy()); QCOMPARE(api.taskId(),51); QCOMPARE(api.page(),2);
    }
    void deletedLastPageAndFailedQueryPreserveState() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy()); api.openProject(1); QTRY_VERIFY(!api.busy());
        api.loadPage(2); QTRY_VERIFY(!api.busy()); QCOMPARE(api.page(),2);
        api.selectTask(51,true); server.shrinkOnAction=true;
        api.runTaskAction("delete_tasks"); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.page(),1); QCOMPARE(api.tasks().size(),50); QVERIFY(!api.messageIsError());
        QSignalSpy failedQuery(&api,&ApiClient::taskQueryFailed);
        server.failQuery=true; api.setTaskQuery("reject",2,true); QTRY_VERIFY(!api.busy());
        QCOMPARE(failedQuery.count(),1);
        QVERIFY(api.messageIsError()); QCOMPARE(api.taskSearch(),QString()); QCOMPARE(api.taskStatus(),0); QVERIFY(!api.taskDescending());
        QCOMPARE(api.tasks().size(),50);
    }
    void serverQueryAndCrossPageWorkflow() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        QVERIFY2(api.metaObject()->indexOfMethod("setTaskQuery(QString,int,bool)")>=0,"Whole-project task query required");
        QVERIFY(QMetaObject::invokeMethod(&api,"setTaskQuery",Q_ARG(QString,QStringLiteral("car+red")),Q_ARG(int,2),Q_ARG(bool,true)));
        QTRY_VERIFY(!api.busy());
        QVERIFY2(!server.requests.last().path.contains('+'),"Django query parsing must preserve literal plus signs");
        const auto query=QUrlQuery(QUrl(QString::fromUtf8(server.requests.last().path))).queryItemValue("query",QUrl::FullyDecoded);
        const auto json=QJsonDocument::fromJson(query.toUtf8()).object();
        QCOMPARE(json["ordering"].toArray().first().toString(),"-tasks:id");
        const auto filters=json["filters"].toObject()["items"].toArray(); QCOMPARE(filters.size(),2);
        QCOMPARE(filters[1].toObject()["value"].toString(),"car+red");
        api.openTask(50); QTRY_VERIFY(!api.busy());
        QVERIFY(QMetaObject::invokeMethod(&api,"moveTask",Q_ARG(int,1))); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),51); QCOMPARE(api.page(),2);
        QVERIFY(QMetaObject::invokeMethod(&api,"moveTask",Q_ARG(int,-1))); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),50); QCOMPARE(api.page(),1);
    }
    void submitNextFailureAndRealSkip() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy()); api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(1); QTRY_VERIFY(!api.busy()); doc.addRectangle(1,2,30,40,"Car");
        server.failSave=true;
        QVERIFY2(QMetaObject::invokeMethod(&api,"saveAndNext"),"Submit-and-next required"); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),1); QVERIFY(doc.dirty());
        server.failSave=false; QVERIFY(QMetaObject::invokeMethod(&api,"saveAndNext")); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.taskId(),2); QVERIFY(!doc.dirty());
        QVERIFY(QMetaObject::invokeMethod(&api,"skipTask")); QTRY_VERIFY(!api.busy()); QCOMPARE(api.taskId(),3);
        bool skipped=false;
        for(const auto &r:server.requests) if(r.path=="/api/tasks/2/annotations/") {
            const auto body=QJsonDocument::fromJson(r.body).object(); QVERIFY(body["was_cancelled"].toBool()); QVERIFY(body["result"].toArray().isEmpty()); skipped=true;
        }
        QVERIFY(skipped);
    }
    void explicitBulkAndAnnotationManagement() {
        HttpFixture server; server.workflow=true; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(),"secret",0); QTRY_VERIFY(!api.busy()); api.openProject(1); QTRY_VERIFY(!api.busy());
        QVERIFY2(QMetaObject::invokeMethod(&api,"selectTask",Q_ARG(int,1),Q_ARG(bool,true)),"Task selection required");
        server.failAction=true;
        QVERIFY(QMetaObject::invokeMethod(&api,"runTaskAction",Q_ARG(QString,QStringLiteral("delete_tasks")))); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.property("selectedTaskCount").toInt(),1);
        server.failAction=false;
        QVERIFY(QMetaObject::invokeMethod(&api,"runTaskAction",Q_ARG(QString,QStringLiteral("delete_tasks_annotations")))); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.property("selectedTaskCount").toInt(),0);
        bool checked=false;
        for(const auto &r:server.requests) if(r.method=="POST"&&r.path.contains("/api/dm/actions/")) {
            const auto selection=QJsonDocument::fromJson(r.body).object()["selectedItems"].toObject();
            QVERIFY(!selection["all"].toBool()); QCOMPARE(selection["included"].toArray(),QJsonArray{1}); checked=true;
        }
        QVERIFY(checked);
        server.taskAnnotations=QJsonArray{QJsonObject{{"id",84},{"result",QJsonArray{}}}};
        api.openTask(1); QTRY_VERIFY(!api.busy()); QCOMPARE(doc.annotationId(),84);
        QVERIFY(QMetaObject::invokeMethod(&api,"setGroundTruth",Q_ARG(bool,true))); QTRY_VERIFY(!api.busy());
        QVERIFY(api.property("groundTruth").toBool());
        QVERIFY(QMetaObject::invokeMethod(&api,"deleteAnnotation")); QTRY_VERIFY(!api.busy()); QCOMPARE(doc.annotationId(),-1);
        bool deleted=false; for(const auto &r:server.requests) if(r.path=="/api/annotations/84/" && r.method=="DELETE") deleted=true;
        QVERIFY(deleted);
    }
    void sessionLoginAndCsrfProtectedManagement() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        QVERIFY2(api.metaObject()->indexOfMethod("login(QString,QString,QString)") >= 0,
                 "Native desktop must support Django email/password login");
        QVERIFY(QMetaObject::invokeMethod(&api, "login", Q_ARG(QString, server.url()), Q_ARG(QString, QStringLiteral("user@example.com")), Q_ARG(QString, QStringLiteral("fixture-password"))));
        QTRY_VERIFY(!api.busy()); QVERIFY2(api.connected(), qPrintable(api.message()));
        QCOMPARE(api.property("user").toMap().value("email").toString(), "user@example.com");
        QCOMPARE(server.requests[1].path, "/user/login/");
        QVERIFY(server.requests[1].body.contains("csrfmiddlewaretoken=csrf-fixture"));
        QVERIFY(server.requests[1].cookie.contains("csrftoken=csrf-fixture"));
        QVERIFY(QMetaObject::invokeMethod(&api, "createProject", Q_ARG(QString, QStringLiteral("Created")), Q_ARG(QString, QStringLiteral("Description")), Q_ARG(QString, config)));
        QTRY_VERIFY(!api.busy()); QCOMPARE(api.project().value("id").toInt(), 7);
        bool creationChecked = false;
        for (const auto &r : server.requests) if (r.path == "/api/projects/" && r.method == "POST") {
            QVERIFY(r.cookie.contains("sessionid=session-fixture")); QCOMPARE(r.csrf, "csrf-fixture");
            QCOMPARE(QJsonDocument::fromJson(r.body).object()["label_config"].toString(), config); creationChecked = true;
        }
        QVERIFY(creationChecked);
        QVERIFY(QMetaObject::invokeMethod(&api, "updateProject", Q_ARG(QString, QStringLiteral("Renamed")), Q_ARG(QString, QStringLiteral("Description")), Q_ARG(QString, config)));
        QTRY_VERIFY(!api.busy()); QCOMPARE(api.project().value("title").toString(), "Renamed");
        QTemporaryDir temp; const auto imagePath = temp.filePath("upload.png");
        QImage image(20,20,QImage::Format_RGB32); image.fill(Qt::red); QVERIFY(image.save(imagePath));
        QVERIFY(QMetaObject::invokeMethod(&api, "importFiles", Q_ARG(QVariantList, QVariantList{QUrl::fromLocalFile(imagePath)})));
        QTRY_VERIFY(!api.busy());
        bool importChecked = false;
        for (const auto &r : server.requests) if (r.path.startsWith("/api/projects/7/import")) {
            QVERIFY(r.body.contains("filename=\"upload.png\"")); QVERIFY(r.body.contains("\x89PNG")); QCOMPARE(r.csrf, "csrf-fixture"); importChecked = true;
        }
        QVERIFY(importChecked);
        const auto exported = QUrl::fromLocalFile(temp.filePath("export.json"));
        QVERIFY(QMetaObject::invokeMethod(&api, "exportProject", Q_ARG(QUrl, exported)));
        QTRY_VERIFY(!api.busy()); QFile file(exported.toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(QJsonDocument::fromJson(file.readAll()).isArray());
        QVERIFY(QMetaObject::invokeMethod(&api, "deleteProject")); QTRY_VERIFY(!api.busy());
        QVERIFY(api.project().isEmpty());
    }
    void invalidSessionLoginDoesNotConnect() {
        HttpFixture server; server.failLogin = true; AnnotationDocument doc; ApiClient api(&doc);
        QVERIFY2(api.metaObject()->indexOfMethod("login(QString,QString,QString)") >= 0, "Login is required");
        QVERIFY(QMetaObject::invokeMethod(&api, "login", Q_ARG(QString, server.url()), Q_ARG(QString, QStringLiteral("user@example.com")), Q_ARG(QString, QStringLiteral("wrong"))));
        QTRY_VERIFY(!api.busy()); QVERIFY(!api.connected()); QVERIFY(api.messageIsError());
        QCOMPARE(server.requests.size(), 2);
    }
    void multiFileImportUsesDistinctFields() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(), "secret", 0); QTRY_VERIFY(!api.busy());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        QTemporaryDir temp;
        QDir().mkpath(temp.filePath("a")); QDir().mkpath(temp.filePath("b"));
        QImage image(20,20,QImage::Format_RGB32); image.fill(Qt::red);
        QVERIFY(image.save(temp.filePath("a/same.png"))); QVERIFY(image.save(temp.filePath("b/same.png")));
        api.importFiles({QUrl::fromLocalFile(temp.filePath("a/same.png")),QUrl::fromLocalFile(temp.filePath("b/same.png"))});
        QTRY_VERIFY(!api.busy());
        QList<QByteArray> fields;
        for (const auto &r : server.requests) if (r.method == "POST" && r.path.contains("/import")) {
            for (const auto &line : r.body.split('\n')) if (line.startsWith("Content-Disposition:")) fields.append(line.split(';').value(1).trimmed());
        }
        QCOMPARE(fields.size(), 2); QVERIFY2(fields[0] != fields[1], "Django request.FILES.items() keeps only the last file for each field");
    }
    void urlImportUsesDjangoFormEncoding() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(), "secret", 0); QTRY_VERIFY(!api.busy());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        const QString address = "https://example.com/data+set.json?q=a+b&v=1";
        api.importUrl(address); QTRY_VERIFY(!api.busy());
        bool checked = false;
        for (const auto &r : server.requests) if (r.method == "POST" && r.path.contains("/import")) {
            QCOMPARE(r.contentType, "application/x-www-form-urlencoded");
            QCOMPARE(QUrlQuery(QString::fromUtf8(r.body)).queryItemValue("url", QUrl::FullyDecoded), address);
            QVERIFY(!r.body.contains('+')); checked = true;
        }
        QVERIFY(checked);
    }
    void sessionThumbnailsUseOwnOriginCookiesAndClearOnLogout() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        api.login(server.url(), "user@example.com", "fixture-password");
        QTRY_VERIFY(!api.busy()); QVERIFY(api.connected());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        api.requestThumbnail(9);
        QTRY_VERIFY(api.thumbnails().contains("9"));
        QVERIFY(api.thumbnails()["9"].toString().startsWith("data:image/png;base64,"));
        QVERIFY(server.requests.last().cookie.contains("sessionid=session-fixture"));
        QVERIFY(api.imageSource().isEmpty());
        api.disconnectServer(); QTRY_VERIFY(!api.busy()); QVERIFY(api.thumbnails().isEmpty());
    }
    void sessionThumbnailsAreAnonymousAcrossOrigins() {
        HttpFixture server, foreign; AnnotationDocument doc; ApiClient api(&doc);
        server.mediaSource = foreign.url() + "/media.png";
        api.login(server.url(), "user@example.com", "fixture-password");
        QTRY_VERIFY(!api.busy()); QVERIFY(api.connected());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        api.requestThumbnail(9);
        QTRY_VERIFY(api.thumbnails().contains("9"));
        QCOMPARE(foreign.requests.size(), 1);
        QVERIFY(foreign.requests.first().cookie.isEmpty());
        QVERIFY(foreign.requests.first().auth.isEmpty());
        QVERIFY(api.imageSource().isEmpty());
    }
    void tokenPaginationAndSaveLifecycle() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        QSignalSpy images(&api, &ApiClient::imageReady), saved(&api, &ApiClient::saved);
        api.connectTo(server.url(), "secret", 0);
        QTRY_VERIFY(api.connected()); QTRY_VERIFY(!api.busy());
        QCOMPARE(api.projects().size(), 2); QCOMPARE(server.requests.first().auth, "Token secret");
        api.openProject(1); QTRY_VERIFY(!api.busy());
        QVERIFY(doc.editable()); QCOMPARE(api.totalTasks(), 51);
        api.openTask(9); QTRY_VERIFY(!api.busy());
        QCOMPARE(images.count(), 1); QCOMPARE(server.requests.last().auth, "Token secret");
        doc.addRectangle(10, 10, 30, 40, "Car"); QVERIFY(doc.dirty());
        api.save(); QTRY_COMPARE(saved.count(), 1); QTRY_VERIFY(!api.busy());
        QCOMPARE(server.requests.last().method, "POST"); QCOMPARE(doc.annotationId(), 84); QVERIFY(!doc.dirty());
        const auto sent = QJsonDocument::fromJson(server.requests.last().body).object()["result"].toArray();
        QCOMPARE(sent[0].toObject()["original_width"].toInt(), 120);
        doc.updateRectangle(0, 30, 20, 20, 30); server.failSave = true; api.save(); QTRY_VERIFY(!api.busy());
        QVERIFY(doc.dirty()); QVERIFY(api.messageIsError());
        QCOMPARE(server.requests.last().method, "PATCH"); QCOMPARE(server.requests.last().path, "/api/annotations/84/");
        api.openTask(9); QVERIFY(doc.dirty()); api.discardChanges(); QVERIFY(!doc.dirty());
    }
    void jwtRefreshExchange() {
        HttpFixture server; AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(), "refresh-secret", 1);
        QTRY_VERIFY(api.connected()); QTRY_VERIFY(!api.busy());
        QCOMPARE(server.requests[0].path, "/api/token/refresh/"); QVERIFY(server.requests[0].auth.isEmpty());
        QCOMPARE(QJsonDocument::fromJson(server.requests[0].body).object()["refresh"].toString(), "refresh-secret");
        QCOMPARE(server.requests[1].auth, "Bearer access-token");
    }
    void invalidAddressAndDemo() {
        AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo("file:///tmp", "secret", 0); QVERIFY(!api.connected()); QVERIFY(api.messageIsError());
        api.loadDemo(); QVERIFY(api.demo()); QVERIFY(doc.editable()); QVERIFY(!doc.dirty()); QCOMPARE(doc.rectangles().size(), 3);
        doc.addRectangle(1, 1, 10, 10, "车辆"); QVERIFY(doc.dirty());
        api.discardChanges(); QCOMPARE(doc.rectangles().size(), 3); QVERIFY(!doc.dirty());
    }
    void foreignMediaDoesNotReceiveCredentials() {
        HttpFixture server, foreign;
        server.mediaSource = foreign.url() + "/media.png";
        AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(), "private-token", 0);
        QTRY_VERIFY(api.connected()); QTRY_VERIFY(!api.busy());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(9); QTRY_VERIFY(!api.busy());
        QCOMPARE(foreign.requests.size(), 1);
        QVERIFY(foreign.requests.first().auth.isEmpty());
        QVERIFY(!api.imageSource().isEmpty());
    }
    void jpegExifOrientationMatchesBrowserCoordinates() {
        HttpFixture server; server.orientedJpeg = true;
        AnnotationDocument doc; ApiClient api(&doc); QSignalSpy images(&api, &ApiClient::imageReady);
        api.connectTo(server.url(), "secret", 0); QTRY_VERIFY(api.connected()); QTRY_VERIFY(!api.busy());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(9); QTRY_VERIFY(!api.busy());
        QCOMPARE(images.count(), 1);
        QCOMPARE(qvariant_cast<QImage>(images.first().first()).size(), QSize(80,120));
        doc.addRectangle(0,0,50,50,"Car");
        QCOMPARE(doc.results()[0].toObject()["original_width"].toInt(), 80);
    }
    void malformedExistingResultCannotBeOverwritten() {
        HttpFixture server;
        server.taskAnnotations = QJsonArray{QJsonObject{{"id", 84}, {"result", "invalid-array"}}};
        AnnotationDocument doc; ApiClient api(&doc);
        api.connectTo(server.url(), "secret", 0); QTRY_VERIFY(api.connected()); QTRY_VERIFY(!api.busy());
        api.openProject(1); QTRY_VERIFY(!api.busy());
        api.openTask(9); QTRY_VERIFY(!api.busy());
        QVERIFY(!doc.editable());
        const int count = server.requests.size(); api.save(); QCOMPARE(server.requests.size(), count);
    }
};
QTEST_GUILESS_MAIN(ApiTest)
#include "tst_api.moc"
