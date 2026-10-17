#include "apiclient.h"
#include <QNetworkReply>
#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QSettings>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

ApiClient::ApiClient(AnnotationDocument *document, QObject *parent) : QObject(parent), m_dataManager(new DataManager(this)), m_account(new AccountManager(this)), m_catalog(new ProjectCatalog(this)), m_document(document) {
    connect(document,&AnnotationDocument::activeImageChanged,this,[this]{
        if(!m_loadingTask && !m_demo && m_taskId>=0) loadActiveImage();
    });
}

void ApiClient::notify(const QString &message, bool error) {
    m_message = message; m_messageIsError = error; emit changed();
}
void ApiClient::clearMessage() { m_message.clear(); emit changed(); }
bool ApiClient::mayNavigate() {
    if (busy()) return false;
    if (m_document->dirty()) { notify(tr("You have unsaved changes. Save or discard them before continuing."), true); return false; }
    return true;
}
bool ApiClient::sameOrigin(const QUrl &url) const {
    return url.scheme() == m_base.scheme() && url.host() == m_base.host() &&
           url.port(url.scheme() == "https" ? 443 : 80) == m_base.port(m_base.scheme() == "https" ? 443 : 80);
}
QUrl ApiClient::endpoint(const QString &path) const {
    QUrl relative(path);
    if (!relative.isRelative()) return relative;
    QString suffix = path; while (suffix.startsWith('/')) suffix.remove(0, 1);
    return m_base.resolved(QUrl(suffix));
}

void ApiClient::request(const QByteArray &method, const QString &path, const QJsonObject &body, JsonCallback callback, bool retry, ErrorCallback failure) {
    requestPayload(method, path, QJsonDocument(body).toJson(QJsonDocument::Compact), "application/json", callback, retry, failure);
}
void ApiClient::requestPayload(const QByteArray &method, const QString &path, const QByteArray &body, const QByteArray &contentType, JsonCallback callback, bool retry, ErrorCallback failure) {
    const auto url = endpoint(path);
    if (!sameOrigin(url)) { notify(tr("The API address must have the same origin as the server."), true); return; }
    QNetworkRequest req(url);
    req.setTransferTimeout(30000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setRawHeader("Accept", "application/json");
    const bool refreshing = path == "/api/token/refresh/";
    if (!refreshing && !m_credential.isEmpty()) req.setRawHeader("Authorization", m_credential);
    if (method != "GET") {
        req.setRawHeader("X-CSRFToken", csrfToken());
        req.setRawHeader("Referer", m_base.toEncoded());
    }
    QNetworkReply *reply;
    if (method == "GET") reply = m_network.get(req);
    else {
        req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
        reply = m_network.sendCustomRequest(req, method, body);
    }
    ++m_pending; emit changed();
    connect(reply, &QNetworkReply::finished, this, [=] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto bytes = reply->readAll();
        QJsonParseError parseError;
        const auto json = QJsonDocument::fromJson(bytes, &parseError);
        if (status == 401 && retry && !refreshing && !m_refreshToken.isEmpty()) {
            refreshAccess([=] { requestPayload(method, path, body, contentType, callback, false, failure); }, [=] { if (failure) failure(401); });
        } else if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            QString detail = json.object().value("detail").toString();
            if (detail.isEmpty()) detail = json.isNull() ? reply->errorString() : QString::fromUtf8(json.toJson(QJsonDocument::Compact)).left(400);
            if (!failure || !failure(status)) notify(tr("Request failed (HTTP %1): %2").arg(status).arg(detail), true);
        } else if (status == 204 || bytes.isEmpty()) callback(QJsonDocument(QJsonObject{}));
        else if (parseError.error != QJsonParseError::NoError) {
            if (!failure || !failure(-1)) notify(tr("The server did not return valid JSON. Check that the address points to Label Studio."), true);
        } else callback(json);
        --m_pending; reply->deleteLater(); emit changed();
    });
}

void ApiClient::refreshAccess(std::function<void()> callback, std::function<void()> failed) {
    request("POST", "/api/token/refresh/", {{"refresh", m_refreshToken}}, [=](const QJsonDocument &json) {
        const auto token = json.object().value("access").toString();
        if (token.isEmpty()) { if (failed) failed(); notify(tr("The JWT refresh response has no access token."), true); return; }
        m_credential = "Bearer " + token.toUtf8();
        const auto rotated = json.object().value("refresh").toString();
        if (!rotated.isEmpty()) m_refreshToken = rotated;
        callback();
    }, false, [=](int) { if (failed) failed(); return false; });
}

void ApiClient::connectTo(const QString &address, const QString &token, int mode) {
    if (!mayNavigate()) return;
    QUrl url(address.trimmed());
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "http" && url.scheme() != "https") || !url.userInfo().isEmpty() || token.trimmed().isEmpty()) {
        notify(tr("Enter an http(s) server address and a valid token."), true); return;
    }
    url.setQuery(QString()); url.setFragment(QString());
    if (!url.path().endsWith('/')) url.setPath(url.path() + '/');
    resetTaskQuery(); clearThumbnails(); m_account->clear();
    m_base = url; m_refreshToken.clear(); m_credential.clear(); m_user = {};
    m_network.setCookieJar(new QNetworkCookieJar(&m_network));
    m_connected = false; m_demo = false;
    m_projects.clear(); m_tasks.clear(); m_project = {}; m_task = {}; m_annotations = {}; m_taskId = -1;
    m_imageSource.clear(); m_document->configure(QString());
    clearMessage();
    QSettings().setValue("serverUrl", address.trimmed());
    const auto authenticated = [this] { request("GET", "/api/current-user/whoami", {}, [this](const QJsonDocument &json) { m_user = json.object(); fetchProjects(1); }); };
    if (mode == 1) { m_refreshToken = token.trimmed(); refreshAccess(authenticated); }
    else { m_credential = (mode == 2 ? "Bearer " : "Token ") + token.trimmed().toUtf8(); authenticated(); }
}

static QJsonArray pageItems(const QJsonDocument &json) {
    if (json.isArray()) return json.array();
    auto object = json.object();
    if (object.contains("results")) return object.value("results").toArray();
    return object.value("tasks").toArray();
}
void ApiClient::fetchProjects(int page,QVariantList accumulated) {
    request("GET", QString("/api/projects/?page=%1&page_size=100").arg(page), {}, [=](const QJsonDocument &json) {
        const auto items = pageItems(json);
        auto collected=accumulated;for(const auto &item:items)collected.append(item.toObject().toVariantMap());
        const int count=json.object().value("count").toInt(collected.size());
        if(!items.isEmpty()&&collected.size()<count&&page<1000)fetchProjects(page+1,collected);
        else {
            if(collected.size()<count){notify(tr("The project list is incomplete. Refresh to retry."),true);return;}
            m_projects=collected;
            const bool firstConnection = !m_connected; m_connected = true;
            if (firstConnection) m_account->activateProject(0);
            if (firstConnection) notify(tr("Connected to Label Studio."));
            else emit changed();
        }
    });
}
void ApiClient::refreshProjects() { if (mayNavigate() && m_connected && !m_demo) fetchProjects(1); }
void ApiClient::openProject(int id) {
    if (!mayNavigate() || !m_connected || m_demo) return;
    request("GET", QString("/api/projects/%1/").arg(id), {}, [=](const QJsonDocument &json) {
        resetTaskQuery(); m_project = json.object(); m_tasks.clear(); m_task = {}; m_annotations = {}; m_taskId = -1;
        m_imageSource.clear(); m_totalTasks = 0; m_page = 1;
        m_document->configure(m_project.value("label_config").toString());
        clearMessage();m_catalog->recordVisit(id); m_account->activateProject(id);m_dataManager->load(); fetchTasks(1, [this] { emit projectOpened(); });
    });
}
void ApiClient::fetchTasks(int page, std::function<void()> done, std::function<void()> failed) {
    request("GET", tasksPath(page), {}, [=](const QJsonDocument &json) {
        const auto items = pageItems(json);
        const int total = json.object().value("total").toInt(json.object().value("count").toInt(items.size()));
        const int lastPage = qMax(1, (total + m_dataManager->pageSize()-1) / m_dataManager->pageSize());
        if (page > lastPage) { fetchTasks(lastPage, done, failed); return; }
        clearThumbnails(); m_tasks = items.toVariantList(); m_totalTasks = total; m_page = page;m_dataManager->totals(json.object().value("total_annotations").toInt(),json.object().value("total_predictions").toInt());
        emit changed(); if (done) done();
    }, true, [=](int status) {
        // Django rejects out-of-range pages before returning a total. Recover
        // from a shrinking final page using a fresh first-page count.
        if (status == 404 && page > 1) {
            fetchTasks(1, [=] {
                const int target = qMin(page, qMax(1, (m_totalTasks + m_dataManager->pageSize()-1) / m_dataManager->pageSize()));
                if (target > 1) fetchTasks(target, done, failed); else if (done) done();
            }, failed);
            return true;
        }
        if (failed) failed();
        return false;
    });
}
void ApiClient::loadPage(int page) {
    if (!mayNavigate() || m_demo || m_project.isEmpty() || page < 1 || (page-1)*m_dataManager->pageSize() >= m_totalTasks) return;
    fetchTasks(page);
}
void ApiClient::openTask(int id) {
    if (!mayNavigate() || m_demo || m_project.isEmpty()) return;
    m_samplingQueue=false;m_samplingHistory.clear();m_samplingIndex=-1;
    fetchTask(id);
}
void ApiClient::fetchTask(int id, std::function<void()> failed) {
    request("GET", QString("/api/tasks/%1/").arg(id), {}, [=](const QJsonDocument &json) {
        m_task = json.object(); m_taskId = m_task.value("id").toInt(); m_annotations = {};
        for (const auto &value : m_task.value("annotations").toArray())
            if (!value.toObject().value("was_cancelled").toBool()) m_annotations.append(value);
        m_loadingTask=true; ++m_mediaEpoch; m_imageSource.clear();
        if (!m_document->configure(m_project.value("label_config").toString())) { m_loadingTask=false; emit taskOpened(); emit changed(); return; }
        m_document->setTaskData(taskData());
        if (!m_annotations.isEmpty()) {
            const auto annotation = m_annotations.first().toObject();
            m_document->loadAnnotation(annotation);
        }
        m_loadingTask=false; clearMessage(); emit taskOpened(); loadActiveImage(); emit changed();
    }, true, [=](int) { if (failed) failed(); return false; });
}
QVariantList ApiClient::annotations() const {
    QVariantList list;
    for (const auto &item : m_annotations) {
        const auto a = item.toObject();
        const auto author = a.value("completed_by");
        const int userId = author.isObject() ? author.toObject().value("id").toInt() : author.toInt();
        const QString user = m_demo ? tr("Demo") : userId > 0 ? tr("User %1").arg(userId) : tr("Manual");
        list.append(QVariantMap{{"id", a.value("id").toInt()}, {"title", tr("Annotation #%1 · %2%3").arg(a.value("id").toInt()).arg(user).arg(a.value("ground_truth").toBool() ? tr(" · Ground truth") : "")}});
    }
    list.append(QVariantMap{{"id", -1}, {"title", tr("New annotation")}});
    return list;
}
void ApiClient::selectAnnotation(int id) {
    if (!mayNavigate()) return;
    if (id < 0) m_document->load({});
    else for (const auto &a : m_annotations) if (a.toObject().value("id").toInt() == id) {
        m_document->loadAnnotation(a.toObject()); break;
    }
    emit changed();
}
void ApiClient::discardChanges() {
    const int id = m_document->annotationId();
    if (id < 0) m_document->load({});
    else for (const auto &a : m_annotations) if (a.toObject().value("id").toInt() == id) m_document->loadAnnotation(a.toObject());
}
void ApiClient::save() { saveCurrent({}); }
void ApiClient::saveCurrent(std::function<void()> done) {
    if (busy() || !m_document->editable() || m_taskId < 0 || (!m_document->imageFrames().isEmpty() && m_imageSource.isEmpty())) return;
    if(!m_document->validate()){ notify(m_document->validationError(),true); return; }
    const auto results = m_document->results();
    const int previousId = m_document->annotationId();
    const auto update = [=](int id) {
        QJsonObject a{{"id", id}, {"result", results}, {"completed_by", m_user}};
        bool found = false;
        for (int i = 0; i < m_annotations.size(); ++i) if (m_annotations[i].toObject().value("id").toInt() == id) {
            a = m_annotations[i].toObject(); a["result"] = results; m_annotations[i] = a; found = true; break;
        }
        if (!found) m_annotations.append(a);
        m_document->markSaved(id);
        for (auto &item : m_tasks) if (item.toMap().value("id").toInt() == m_taskId) { auto row = item.toMap(); row["is_labeled"] = true; item = row; }
        notify(m_demo ? tr("Demo annotation saved for this session.") : tr("Annotation #%1 saved to Django.").arg(id));
        emit saved(); if (done) done();
    };
    if (m_demo) { update(previousId < 0 ? 1 : previousId); return; }
    clearMessage();
    request(previousId < 0 ? "POST" : "PATCH", previousId < 0 ? QString("/api/tasks/%1/annotations/").arg(m_taskId) : QString("/api/annotations/%1/").arg(previousId), {{"result", results}}, [=](const QJsonDocument &json) {
        const int id = json.object().value("id").toInt(previousId);
        if (id < 0) { notify(tr("The save response has no annotation ID. Your changes are preserved."), true); return; }
        update(id);
    });
}

void ApiClient::acceptImage(const QImage &image) {
    if (image.isNull()) { notify(tr("Unable to decode the image."), true); return; }
    m_document->setImageSize(image.width(), image.height());
    emit imageReady(image);
    m_imageSource = QString("image://task/%1").arg(++m_imageRevision); emit changed();
}
void ApiClient::loadActiveImage() {
    ++m_mediaEpoch; m_imageSource.clear(); emit changed();
    if(m_document->imageFrames().isEmpty()) return;
    const auto source=m_document->activeImageSource();
    if(source.isEmpty()){notify(tr("The task has no image field %1.").arg(m_document->imageKey()),true);return;}
    QUrl media(source);
    if(media.isRelative())media=source.startsWith('/')?m_base.resolved(media):endpoint(source);
    fetchImage(media);
}
void ApiClient::fetchImage(const QUrl &url, int redirects) {
    if ((url.scheme() != "http" && url.scheme() != "https") || redirects > 5) { notify(tr("Invalid image address or redirect."), true); return; }
    QNetworkRequest req(url); req.setTransferTimeout(30000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    if (sameOrigin(url)) req.setRawHeader("Authorization", m_credential);
    auto *reply = (sameOrigin(url) ? m_network : m_publicNetwork).get(req); ++m_pending; emit changed();
    const int epoch=m_mediaEpoch;
    connect(reply, &QNetworkReply::finished, this, [=] {
        if(epoch!=m_mediaEpoch){--m_pending;reply->deleteLater();emit changed();return;}
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (status >= 300 && status < 400 && !redirect.isEmpty()) {
            const auto next = url.resolved(redirect);
            if (url.scheme() == "https" && next.scheme() != "https") notify(tr("The image redirect downgraded HTTPS. Loading was stopped."), true);
            else fetchImage(next, redirects+1);
        } else if (reply->error() != QNetworkReply::NoError) notify(tr("Image loading failed (HTTP %1): %2").arg(status).arg(reply->errorString()), true);
        else {
            QByteArray bytes = reply->readAll();
            QBuffer buffer(&bytes); buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer); reader.setAutoTransform(true);
            acceptImage(reader.read());
        }
        --m_pending; reply->deleteLater(); emit changed();
    });
}

void ApiClient::loadDemo() {
    if (!mayNavigate()) return;
    resetTaskQuery(); m_demo = true; m_connected = true; m_credential.clear(); m_refreshToken.clear();
    m_project = {{"id", 0}, {"title", QStringLiteral("城市交通 · 示例项目")}};
    m_projects = {m_project.toVariantMap()}; m_taskId = 1001; m_page = 1; m_totalTasks = 1;
    m_tasks = {QVariantMap{{"id", 1001}, {"is_labeled", true}, {"data", QVariantMap{{"image", "city-street.png"}}}}};
    m_annotations = {};
    m_document->configure(QStringLiteral(R"(<View><Image name="image" value="$image"/><RectangleLabels name="bbox" toName="image"><Label value="车辆" background="#8B7EFF"/><Label value="行人" background="#F5B85A"/><Label value="交通灯" background="#55CBAA"/></RectangleLabels></View>)"));
    QImage image(1200, 800, QImage::Format_RGB32);
    QPainter p(&image); p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient sky(0, 0, 0, 800); sky.setColorAt(0, QColor("#BFD4DF")); sky.setColorAt(1, QColor("#EDF0E4")); p.fillRect(image.rect(), sky);
    const QList<QColor> colors{QColor("#A8B6B5"), QColor("#849DA9"), QColor("#C2C5B7"), QColor("#7F99A7")};
    for (int i=0; i<7; ++i) {
        int x=i*200-50, y=100+(i%3)*60; p.fillRect(x,y,175,470-y,colors[i%4]);
        p.setBrush(QColor("#D8E5DD")); p.setPen(Qt::NoPen);
        for(int row=0; row<5; ++row) for(int col=0; col<4; ++col) p.drawRect(x+18+col*38,y+24+row*50,20,29);
    }
    p.fillRect(0,435,1200,65,QColor("#BAC2BA")); p.fillRect(0,500,1200,300,QColor("#59666D"));
    p.setPen(QPen(QColor("#E3DFCF"),7,Qt::DashLine)); p.drawLine(0,700,1200,700);
    auto car = [&](QRectF r, QColor c) {
        p.setPen(Qt::NoPen); p.setBrush(c); p.drawRoundedRect(QRectF(r.x(),r.y()+r.height()*.25,r.width(),r.height()*.6),18,18);
        p.drawRoundedRect(QRectF(r.x()+r.width()*.16,r.y(),r.width()*.65,r.height()*.5),24,24);
        p.setBrush(QColor("#354C5A")); p.drawRoundedRect(QRectF(r.x()+r.width()*.22,r.y()+10,r.width()*.5,r.height()*.3),10,10);
        p.setBrush(QColor("#26323B")); p.drawEllipse(QRectF(r.x()+24,r.bottom()-43,55,55)); p.drawEllipse(QRectF(r.right()-83,r.bottom()-43,55,55));
        p.setBrush(QColor("#B1B8BB")); p.drawEllipse(QRectF(r.x()+40,r.bottom()-27,23,23)); p.drawEllipse(QRectF(r.right()-67,r.bottom()-27,23,23));
    };
    car(QRectF(168,440,276,176),QColor("#DE705E")); car(QRectF(696,384,324,176),QColor("#D9D7CB"));
    p.setBrush(QColor("#354F5D")); p.setPen(Qt::NoPen); p.drawRoundedRect(561,392,50,111,12,12);
    p.setBrush(QColor("#C49579")); p.drawEllipse(570,351,32,38);
    p.setPen(QPen(QColor("#30434D"),15,Qt::SolidLine,Qt::RoundCap)); p.drawLine(574,498,565,566); p.drawLine(598,498,612,566); p.end();
    acceptImage(image);
    m_document->addRectangle(14,55,37,77,"车辆"); m_document->addRectangle(58,48,85,70,"车辆"); m_document->addRectangle(46,43,52,72,"行人");
    m_annotations = QJsonArray{QJsonObject{{"id", 1}, {"result", m_document->results()}}};
    m_task=QJsonObject::fromVariantMap(m_tasks.first().toMap());m_task["annotations"]=m_annotations;
    m_document->load(m_document->results(), 1);
    emit taskOpened();
    notify(tr("Demo mode · Annotations are saved for this session. Connect to Django to work on real tasks."));
}
