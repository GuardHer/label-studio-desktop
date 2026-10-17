#include "apiclient.h"
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QSaveFile>
#include <QSettings>
#include <QUrlQuery>
#include <QUuid>
#include <QBuffer>
#include <QImageReader>
#include <QRegularExpression>
#include <QTextDocumentFragment>

QByteArray ApiClient::csrfToken() const {
    for (const auto &cookie : m_network.cookieJar()->cookiesForUrl(m_base))
        if (cookie.name() == "csrftoken") return cookie.value();
    return {};
}

bool ApiClient::prepareServer(const QString &address) {
    QUrl url(address.trimmed());
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
        (url.scheme() != "http" && url.scheme() != "https")) {
        notify(tr("Enter a valid http(s) Label Studio server address."), true); return false;
    }
    url.setQuery(QString()); url.setFragment(QString());
    if (!url.path().endsWith('/')) url.setPath(url.path() + '/');
    resetTaskQuery(); clearThumbnails(); m_account->clear(); m_base = url;
    m_network.setCookieJar(new QNetworkCookieJar(&m_network));
    m_credential.clear(); m_refreshToken.clear(); m_user = {};
    m_connected = false; m_demo = false;
    m_projects.clear(); m_tasks.clear(); m_project = {}; m_task = {}; m_annotations = {};
    m_taskId = -1; m_totalTasks = 0; m_imageSource.clear(); m_document->configure({});
    clearMessage(); QSettings().setValue("serverUrl", url.toString());
    return true;
}

void ApiClient::login(const QString &address, const QString &email, const QString &password) {
    authenticateForm(address,email,password,{},false,false);
}
void ApiClient::signup(const QString &address,const QString &email,const QString &password,const QString &invitation,bool newsletters) {
    authenticateForm(address,email,password,invitation,newsletters,true);
}
void ApiClient::authenticateForm(const QString &address,const QString &email,const QString &password,const QString &invitation,bool newsletters,bool registering) {
    if (!mayNavigate()) return;
    if (email.trimmed().isEmpty() || password.isEmpty()) { notify(tr("Enter your email and password."), true); return; }
    if (!prepareServer(address)) return;
    QString path=registering?"/user/signup/":"/user/login/";
    if(registering&&!invitation.trimmed().isEmpty()) {
        const QUrl link(invitation.trimmed());
        const auto token=link.hasQuery()?QUrlQuery(link).queryItemValue("token",QUrl::FullyDecoded):invitation.trimmed();
        path+="?token="+QString::fromLatin1(QUrl::toPercentEncoding(token));
    }
    QNetworkRequest req(endpoint(path));
    req.setTransferTimeout(30000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = m_network.get(req); ++m_pending; emit changed();
    connect(reply, &QNetworkReply::finished, this, [=] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status != 200 || csrfToken().isEmpty()) {
            notify(tr("Unable to load the Django login form and CSRF cookie (HTTP %1).").arg(status), true);
        } else {
            QUrlQuery form;
            form.addQueryItem("email", email.trimmed()); form.addQueryItem("password", password);
            form.addQueryItem("csrfmiddlewaretoken", QString::fromUtf8(csrfToken()));
            if(registering&&newsletters)form.addQueryItem("allow_newsletters","on");
            QNetworkRequest post(endpoint(path));
            post.setTransferTimeout(30000);
            post.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
            post.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            post.setRawHeader("Referer", endpoint(path).toEncoded());
            // QUrlQuery retains '+'; form encoding must represent it as %2B.
            const auto body = form.toString(QUrl::FullyEncoded).replace("+", "%2B").toUtf8();
            auto *auth = m_network.post(post, body); ++m_pending;
            connect(auth, &QNetworkReply::finished, this, [=] {
                const int code = auth->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                const auto location = auth->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                if ((code == 302 || code == 303) && sameOrigin(auth->url().resolved(location))) {
                    request("GET", "/api/current-user/whoami", {}, [this](const QJsonDocument &json) {
                        m_user = json.object();
                        if (!m_user.value("id").isDouble()) { notify(tr("The login session is not authenticated."), true); return; }
                        fetchProjects(1);
                    });
                } else if(registering) {
                    const auto html=QString::fromUtf8(auth->readAll());
                    const auto match=QRegularExpression("<ul[^>]*class=[\"'][^\"']*errorlist[^\"']*[\"'][^>]*>([\\s\\S]*?)</ul>").match(html);
                    const auto detail=match.hasMatch()?QTextDocumentFragment::fromHtml(match.captured(1)).toPlainText().trimmed():tr("Check the invitation link, email, password requirements and server signup policy.");
                    notify(tr("Registration failed (HTTP %1): %2").arg(code).arg(detail),true);
                } else notify(code == 200 ? tr("Email or password is incorrect.") : tr("Login failed (HTTP %1).").arg(code), true);
                --m_pending; auth->deleteLater(); emit changed();
            });
        }
        --m_pending; reply->deleteLater(); emit changed();
    });
}

void ApiClient::disconnectServer() {
    if (!mayNavigate()) return;
    if(m_connected&&!m_demo&&m_credential.isEmpty()) {
        QNetworkRequest req(endpoint("/logout/"));req.setTransferTimeout(30000);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
        auto *reply=m_network.get(req);++m_pending;emit changed();
        connect(reply,&QNetworkReply::finished,this,[this,reply]{
            const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            --m_pending;
            if(status>=200&&status<400)clearConnection();
            else notify(tr("Server logout failed (HTTP %1). Please retry.").arg(status),true);
            reply->deleteLater();emit changed();
        });return;
    }
    clearConnection();
}
void ApiClient::clearConnection() {
    m_account->clear();
    resetTaskQuery(); clearThumbnails();
    m_network.setCookieJar(new QNetworkCookieJar(&m_network));
    m_credential.clear(); m_refreshToken.clear(); m_user = {};
    m_connected = false; m_demo = false; m_projects.clear(); m_tasks.clear();
    m_project = {}; m_task = {}; m_annotations = {}; m_taskId = -1; m_totalTasks = 0;
    m_imageSource.clear(); m_document->configure({}); clearMessage();
}

void ApiClient::clearThumbnails() {
    ++m_thumbnailEpoch;
    const auto replies = m_thumbnailReplies;
    for (auto *reply : replies) reply->abort();
    m_thumbnailReplies.clear(); m_thumbnailIds.clear();m_thumbnailKeys.clear(); m_thumbnails.clear();
    emit thumbnailsChanged();
}

void ApiClient::requestThumbnail(int id,const QString &field) {
    const QString cacheKey=QString::number(id)+(field.isEmpty()?QString():":"+field);
    if(!m_connected||m_demo||m_thumbnailKeys.contains(cacheKey))return;
    for(const auto &item:m_tasks){
        const auto task=item.toMap();if(task.value("id").toInt()!=id)continue;
        const auto data=task.value("data").toMap();
        const QString dataKey=field.isEmpty()?(m_document->imageKey().isEmpty()?"image":m_document->imageKey()):(field.startsWith("data.")?field.mid(5):field);
        auto value=data.value(dataKey);if(value.typeId()==QMetaType::QVariantList&&!value.toList().isEmpty())value=value.toList().first();
        const QString source=value.toString();if(source.isEmpty())return;
        m_thumbnailKeys.insert(cacheKey);if(field.isEmpty())m_thumbnailIds.insert(id);
        const QUrl relative(source);fetchThumbnail(id,relative.isRelative()?m_base.resolved(relative):relative,m_thumbnailEpoch,0,cacheKey);return;
    }
}

void ApiClient::fetchThumbnail(int id, const QUrl &url, int epoch, int redirects,const QString &cacheKey) {
    if (epoch != m_thumbnailEpoch || redirects > 5 || (url.scheme() != "http" && url.scheme() != "https")) return;
    QNetworkRequest req(url); req.setTransferTimeout(15000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    if (sameOrigin(url) && !m_credential.isEmpty()) req.setRawHeader("Authorization", m_credential);
    auto *reply = (sameOrigin(url) ? m_network : m_publicNetwork).get(req);
    m_thumbnailReplies.insert(reply);
    connect(reply, &QNetworkReply::finished, this, [=] {
        m_thumbnailReplies.remove(reply);
        if (epoch == m_thumbnailEpoch) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const auto target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (status >= 300 && status < 400 && !target.isEmpty()) {
                const auto next = url.resolved(target);
                if (url.scheme() != "https" || next.scheme() == "https") fetchThumbnail(id, next, epoch, redirects + 1,cacheKey);
            } else if (reply->error() == QNetworkReply::NoError) {
                auto bytes = reply->readAll(); QBuffer input(&bytes); input.open(QIODevice::ReadOnly);
                QImageReader reader(&input); reader.setAutoTransform(true);
                const auto image = reader.read();
                if (!image.isNull()) {
                    QByteArray png; QBuffer output(&png); output.open(QIODevice::WriteOnly);
                    image.scaled(240,160,Qt::KeepAspectRatio,Qt::SmoothTransformation).save(&output,"PNG");
                    m_thumbnails[cacheKey.isEmpty()?QString::number(id):cacheKey] = "data:image/png;base64," + QString::fromLatin1(png.toBase64());
                    emit thumbnailsChanged();
                }
            }
        }
        reply->deleteLater();
    });
}

void ApiClient::createProject(const QString &title, const QString &description, const QString &config) {
    if (!mayNavigate() || !m_connected || m_demo) return;
    if (title.trimmed().isEmpty()) { notify(tr("Project name is required."), true); return; }
    const auto labelingConfig=config.trimmed().isEmpty()?QStringLiteral("<View></View>"):config;
    request("POST", "/api/projects/", {{"title", title.trimmed()}, {"description", description}, {"label_config", labelingConfig}}, [=](const QJsonDocument &json) {
        resetTaskQuery(); m_project = json.object();
        if (!m_project.value("id").isDouble()) { notify(tr("Project response has no ID."), true); return; }
        m_tasks.clear(); m_taskId = -1; m_imageSource.clear(); m_annotations = {}; m_totalTasks = 0; m_page = 1;
        m_document->configure(m_project.value("label_config").toString());
        m_dataManager->load();m_catalog->recordVisit(m_project.value("id").toInt());m_account->activateProject(m_project.value("id").toInt());
        emit projectCreated(m_project.value("id").toInt()); fetchProjects(1); fetchTasks(1);
    });
}

void ApiClient::updateProject(const QString &title, const QString &description, const QString &config) {
    updateProjectDetails(title,description,config,m_project.value("color").toString("#FFFFFF"),m_project.value("sampling").toString("Sequential sampling"));
}

void ApiClient::deleteProject() {
    deleteProjectById(m_project.value("id").toInt());
}

void ApiClient::importFiles(const QVariantList &files) {
    if (!mayNavigate() || m_project.isEmpty() || m_demo || files.isEmpty()) return;
    const auto boundary = QByteArray("QmlStudio") + QUuid::createUuid().toByteArray(QUuid::Id128);
    QByteArray body;
    int field = 0;
    for (const auto &value : files) {
        QUrl url = value.toUrl();
        if (!url.isLocalFile()) { notify(tr("Choose a local file to upload."), true); return; }
        QFile file(url.toLocalFile());
        if (!file.open(QIODevice::ReadOnly)) { notify(tr("Cannot read %1.").arg(file.fileName()), true); return; }
        if (file.size() > 64*1024*1024 || body.size() + file.size() > 64*1024*1024) { notify(tr("Upload up to 64 MB at a time."), true); return; }
        const auto name = QFileInfo(file).fileName().replace('"', '_').replace('\r', '_').replace('\n', '_').toUtf8();
        // Django iterates request.FILES.items(); repeated field names lose files.
        body += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"file_" + QByteArray::number(field++) + "\"; filename=\"" + name + "\"\r\nContent-Type: " + QMimeDatabase().mimeTypeForFile(file.fileName()).name().toUtf8() + "\r\n\r\n";
        body += file.readAll(); body += "\r\n";
    }
    body += "--" + boundary + "--\r\n";
    requestPayload("POST", QString("/api/projects/%1/import?commit_to_project=true").arg(m_project.value("id").toInt()), body,
        "multipart/form-data; boundary=" + boundary, [this](const QJsonDocument &) {
            notify(tr("Data imported.")); emit imported();m_dataManager->load(); fetchTasks(1); fetchProjects(1);
        });
}

void ApiClient::importUrl(const QString &address) {
    if (!mayNavigate() || m_project.isEmpty() || m_demo) return;
    const QUrl url(address.trimmed());
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "http" && url.scheme() != "https")) {
        notify(tr("Enter an http(s) dataset URL."), true); return;
    }
    QUrlQuery form; form.addQueryItem("url", url.toString());
    const auto body = form.toString(QUrl::FullyEncoded).replace("+", "%2B").toUtf8();
    requestPayload("POST", QString("/api/projects/%1/import?commit_to_project=true").arg(m_project.value("id").toInt()), body, "application/x-www-form-urlencoded", [this](const QJsonDocument &) {
        notify(tr("Data imported.")); emit imported();m_dataManager->load(); fetchTasks(1); fetchProjects(1);
    });
}

void ApiClient::exportProject(const QUrl &url) {
    if (!mayNavigate() || m_project.isEmpty() || m_demo) return;
    if (!url.isLocalFile()) { notify(tr("Choose a local export filename."), true); return; }
    request("GET", QString("/api/projects/%1/export?exportType=JSON&download_all_tasks=true").arg(m_project.value("id").toInt()), {}, [=](const QJsonDocument &json) {
        QSaveFile file(url.toLocalFile());
        const auto bytes = json.toJson(QJsonDocument::Indented);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
            notify(tr("Cannot save the exported JSON file."), true); return;
        }
        notify(tr("Export saved to %1").arg(url.toLocalFile()));
    });
}
