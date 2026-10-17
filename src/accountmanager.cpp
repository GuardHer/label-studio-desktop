#include "accountmanager.h"
#include "apiclient.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QMimeDatabase>
#include <QKeySequence>
#include <QUrlQuery>
#include <QUuid>
#include <QDateTime>

AccountManager::AccountManager(ApiClient *api) : QObject(api), m_api(api) {}
bool AccountManager::ready() const { return m_api->connected() && !m_api->demo(); }
int AccountManager::orgId() const { return m_profile.value("active_organization").toInt(); }
void AccountManager::clear() {
    m_profile={}; m_org={}; m_detail={}; m_settings={}; m_hotkeys={}; m_globalKeys={}; m_projectKeys={};
    m_members.clear(); m_tokens.clear(); m_invite.clear(); m_secret.clear(); m_legacy.clear();
    m_loaded=false; m_memberPage=1; m_memberCount=0; m_hotkeyProject=0; m_activeProject=0; ++m_keyGeneration;
    emit changed(); emit hotkeysChanged();
}
void AccountManager::fetchProfile(std::function<void()> done) {
    m_api->request("GET", "/api/current-user/whoami", {}, [this,done](const QJsonDocument &json) {
        m_profile=json.object(); m_api->m_user=m_profile; emit changed(); if (done) done();
    });
}
void AccountManager::load() {
    if (!ready() || m_api->busy()) return;
    m_profile={};m_org={};m_detail={};m_settings={};m_members.clear();m_tokens.clear();m_secret.clear();m_legacy.clear();m_invite.clear();m_loaded=false;
    emit changed();
    fetchProfile([this] {
        m_loaded=true;
        if (orgId()>0) {
            m_api->request("GET",QString("/api/organizations/%1").arg(orgId()),{},[this](const QJsonDocument &json){m_org=json.object();emit changed();});
            loadMembers();
        }
        m_api->request("GET","/api/jwt/settings",{},[this](const QJsonDocument &json){m_settings=json.object();emit changed();});
        loadTokens(); loadHotkeys(); emit changed();
    });
}
void AccountManager::saveProfile(const QString &first,const QString &last,const QString &phone) {
    if (!ready() || m_api->busy() || m_profile.isEmpty()) return;
    m_api->request("PATCH",QString("/api/users/%1/").arg(m_profile.value("id").toInt()),
        {{"first_name",first},{"last_name",last},{"phone",phone}},[this](const QJsonDocument &){
            fetchProfile([this]{emit profileSaved();m_api->notify(tr("Profile saved."));});
        });
}
void AccountManager::saveEmailPreferences(bool allowed) {
    if (!ready() || m_api->busy() || m_profile.isEmpty()) return;
    m_api->request("PATCH",QString("/api/users/%1/").arg(m_profile.value("id").toInt()),{{"allow_newsletters",allowed}},[this](const QJsonDocument &){
        fetchProfile([this]{emit profileSaved();m_api->notify(tr("Email preferences saved."));});
    });
}
void AccountManager::uploadAvatar(const QUrl &url) {
    if (!ready() || m_api->busy() || m_profile.isEmpty()) return;
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size()>1024*1024) {m_api->notify(tr("Choose an avatar image up to 1 MB and 1200 × 1200 pixels."),true);return;}
    const auto name=QFileInfo(file).fileName().replace('"','_').replace('\r','_').replace('\n','_').toUtf8();
    const auto boundary="QmlAvatar"+QUuid::createUuid().toByteArray(QUuid::Id128);
    QByteArray body="--"+boundary+"\r\nContent-Disposition: form-data; name=\"avatar\"; filename=\""+name+"\"\r\nContent-Type: "+QMimeDatabase().mimeTypeForFile(file.fileName()).name().toUtf8()+"\r\n\r\n";
    body+=file.readAll()+"\r\n--"+boundary+"--\r\n";
    m_api->requestPayload("POST",QString("/api/users/%1/avatar/").arg(m_profile.value("id").toInt()),body,"multipart/form-data; boundary="+boundary,[this](const QJsonDocument &){fetchProfile([this]{emit profileSaved();m_api->notify(tr("Avatar updated."));});});
}
void AccountManager::removeAvatar() {
    if (!ready() || m_api->busy() || m_profile.isEmpty()) return;
    m_api->request("DELETE",QString("/api/users/%1/avatar/").arg(m_profile.value("id").toInt()),{},[this](const QJsonDocument &){fetchProfile([this]{emit profileSaved();});});
}
void AccountManager::saveOrganization(const QString &title,const QString &contact) {
    if (!ready() || m_api->busy() || orgId()<1) return;
    if (title.trimmed().isEmpty()) {m_api->notify(tr("Organization name is required."),true);return;}
    m_api->request("PATCH",QString("/api/organizations/%1").arg(orgId()),{{"title",title.trimmed()},{"contact_info",contact}},[this](const QJsonDocument &json){m_org=json.object();emit changed();emit organizationSaved();m_api->notify(tr("Organization saved."));});
}
void AccountManager::loadMembers(int page,const QString &search) {
    if (!ready() || orgId()<1 || page<1) return;
    const auto path=QString("/api/organizations/%1/memberships?page=%2&page_size=20&contributed_to_projects=true&active=true&search=%3").arg(orgId()).arg(page).arg(QString::fromLatin1(QUrl::toPercentEncoding(search)));
    m_api->request("GET",path,{},[this,page](const QJsonDocument &json){
        const auto obj=json.object(); m_members=(json.isArray()?json.array():obj.value("results").toArray()).toVariantList();
        m_memberCount=obj.value("count").toInt(m_members.size());m_memberPage=page;emit changed();
    });
}
void AccountManager::loadMember(int userId) {
    if (!ready() || orgId()<1) return;
    m_detail={};emit changed();
    m_api->request("GET",QString("/api/organizations/%1/memberships/%2/?contributed_to_projects=true").arg(orgId()).arg(userId),{},[this](const QJsonDocument &json){m_detail=json.object();emit changed();});
}
void AccountManager::removeMember(int userId) {
    if (!ready() || m_api->busy() || userId==m_profile.value("id").toInt() || userId==m_org.value("created_by").toInt()) return;
    m_api->request("DELETE",QString("/api/organizations/%1/memberships/%2/").arg(orgId()).arg(userId),{},[this](const QJsonDocument &){loadMembers();m_api->notify(tr("Member removed from the organization."));});
}
void AccountManager::setInvite(const QJsonObject &data) {
    m_invite=m_api->endpoint(data.value("invite_url").toString()).toString();emit changed();
}
void AccountManager::loadInvite() { if(ready())m_api->request("GET","/api/invite",{},[this](const QJsonDocument &json){setInvite(json.object());}); }
void AccountManager::resetInvite() { if(ready()&&!m_api->busy())m_api->request("POST","/api/invite/reset-token",{},[this](const QJsonDocument &json){setInvite(json.object());m_api->notify(tr("Invitation link reset. Previous links no longer work."));}); }
void AccountManager::loadTokens() { if(ready())m_api->request("GET","/api/token/",{},[this](const QJsonDocument &json){m_tokens=(json.isArray()?json.array():json.object().value("results").toArray()).toVariantList();emit changed();}); }
void AccountManager::createToken() {
    if(!ready()||m_api->busy())return;
    m_api->request("POST","/api/token/",{},[this](const QJsonDocument &json){m_secret=json.object().value("token").toString();emit changed();loadTokens();m_api->notify(tr("Copy the new token now. Its signature cannot be retrieved later."));});
}
void AccountManager::revokeToken(const QString &token) {
    if(!ready()||m_api->busy()||token.isEmpty())return;
    m_api->request("POST","/api/token/blacklist/",{{"refresh",token}},[this](const QJsonDocument &){clearSecret();loadTokens();m_api->notify(tr("Token revoked."));});
}
void AccountManager::rotateToken(const QString &token) {
    if(!ready()||m_api->busy()||token.isEmpty())return;
    m_api->request("POST","/api/token/rotate/",{{"refresh",token}},[this,token](const QJsonDocument &json){m_secret=json.object().value("refresh").toString();if(m_api->m_refreshToken==token)m_api->m_refreshToken=m_secret;emit changed();loadTokens();m_api->notify(tr("Token rotated. Copy the new token now."));});
}
void AccountManager::clearSecret(){m_secret.clear();emit changed();}
void AccountManager::loadLegacyToken(){if(ready())m_api->request("GET","/api/current-user/token",{},[this](const QJsonDocument &json){m_legacy=json.object().value("token").toString();emit changed();});}
void AccountManager::resetLegacyToken(){if(ready()&&!m_api->busy())m_api->request("POST","/api/current-user/reset-token/",{},[this](const QJsonDocument &json){m_legacy=json.object().value("token").toString();if(m_api->m_credential.startsWith("Token "))m_api->m_credential="Token "+m_legacy.toUtf8();emit changed();m_api->notify(tr("Legacy token reset. Previous credentials no longer work."));});}
void AccountManager::saveTokenSettings(bool personal,bool legacy){
    if(!ready()||m_api->busy())return;
    m_api->request("POST","/api/jwt/settings",{{"api_tokens_enabled",personal},{"legacy_api_tokens_enabled",legacy}},[this](const QJsonDocument &json){m_settings=json.object();if(!m_settings.value("legacy_api_tokens_enabled").toBool())m_legacy.clear();emit changed();emit tokenPolicySaved();m_api->notify(tr("Organization token policy saved."));});
}
void AccountManager::activateProject(int projectId) {
    if(!ready())return;
    m_activeProject=projectId;const int generation=++m_keyGeneration;
    m_projectKeys={};emit hotkeysChanged();
    m_api->request("GET","/api/current-user/hotkeys/",{},[this,generation](const QJsonDocument &json){if(generation!=m_keyGeneration)return;m_globalKeys=json.object().value("custom_hotkeys").toObject();emit hotkeysChanged();});
    if(projectId>0)m_api->request("GET",QString("/api/current-user/hotkeys/?project=%1").arg(projectId),{},[this,generation](const QJsonDocument &json){if(generation!=m_keyGeneration)return;m_projectKeys=json.object().value("custom_hotkeys").toObject();emit hotkeysChanged();});
}
QString AccountManager::hotkeyJson()const{return QString::fromUtf8(QJsonDocument(QJsonObject{{"custom_hotkeys",m_hotkeys}}).toJson(QJsonDocument::Indented));}
void AccountManager::loadHotkeys(int projectId){
    if(!ready()||projectId<0)return;
    const QString path="/api/current-user/hotkeys/"+(projectId?QString("?project=%1").arg(projectId):QString());
    m_api->request("GET",path,{},[this,projectId](const QJsonDocument &json){m_hotkeys=json.object().value("custom_hotkeys").toObject();m_hotkeyProject=projectId;if(projectId==0)m_globalKeys=m_hotkeys;else if(projectId==m_activeProject)m_projectKeys=m_hotkeys;emit hotkeysChanged();});
}
static bool parseKeys(const QString &text,QJsonObject &obj){
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(text.toUtf8(),&error);
    if(error.error!=QJsonParseError::NoError)return false;
    if(doc.isObject()&&doc.object().value("custom_hotkeys").isObject()){obj=QJsonObject{{"custom_hotkeys",doc.object().value("custom_hotkeys")}};return true;}
    if(!doc.isArray()&&(!doc.isObject()||!doc.object().value("hotkeys").isArray()))return false;
    const auto list=doc.isArray()?doc.array():doc.object().value("hotkeys").toArray();QJsonObject customs;
    for(const auto &entry:list){
        const auto row=entry.toObject();const auto section=row.value("section").toString(),element=row.value("element").toString();
        if(section.isEmpty()||element.isEmpty()||!row.value("key").isString())return false;
        QJsonObject value{{"key",row.value("key")},{"active",row.value("active").toBool(true)}};
        if(row.value("description").isString())value["description"]=row.value("description");customs[section+":"+element]=value;
    }obj=QJsonObject{{"custom_hotkeys",customs}};return true;
}
void AccountManager::saveHotkeys(const QString &json){
    if(!ready()||m_api->busy())return;
    QJsonObject obj;if(!parseKeys(json,obj)){m_api->notify(tr("Enter a JSON object containing custom_hotkeys."),true);return;}
    const int scope=m_hotkeyProject;
    m_api->request("PATCH","/api/current-user/hotkeys/"+(scope?QString("?project=%1").arg(scope):QString()),obj,[this,scope](const QJsonDocument &data){m_hotkeys=data.object().value("custom_hotkeys").toObject();if(scope==0)m_globalKeys=m_hotkeys;else if(scope==m_activeProject)m_projectKeys=m_hotkeys;emit hotkeysChanged();emit hotkeysSaved();m_api->notify(tr("Hotkeys saved."));});
}
void AccountManager::importHotkeys(const QUrl &url){
    QFile file(url.toLocalFile());if(!url.isLocalFile()||!file.open(QIODevice::ReadOnly)||file.size()>1024*1024){m_api->notify(tr("Cannot read hotkeys JSON file (maximum 1 MB)."),true);return;}
    const auto json=QString::fromUtf8(file.readAll());QJsonObject obj;if(!parseKeys(json,obj)){m_api->notify(tr("Enter a JSON object containing custom_hotkeys."),true);return;}emit hotkeysImported(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented)));
}
void AccountManager::exportHotkeys(const QUrl &url,const QString &json){
    QJsonObject obj;if(!parseKeys(json,obj)){m_api->notify(tr("Enter a JSON object containing custom_hotkeys."),true);return;}
    QJsonArray webKeys;const auto customs=obj.value("custom_hotkeys").toObject();
    for(auto it=customs.begin();it!=customs.end();++it){
        QString id=it.key();
        if(id.startsWith("annotation:")&&!id.startsWith("annotation:annotation:"))id="annotation:"+id;
        else if(id.startsWith("region:"))id="regions:"+id;
        else if(id.startsWith("tool:"))id="tools:"+id;
        const int split=id.indexOf(':');if(split<1)continue;auto row=it.value().toObject();row["section"]=id.left(split);row["element"]=id.mid(split+1);row["id"]=id;row["label"]=id;webKeys.append(row);
    }
    obj["hotkeys"]=webKeys;obj["settings"]=QJsonObject{};obj["version"]="1.0";obj["exportedAt"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSaveFile file(url.toLocalFile());const auto data=QJsonDocument(obj).toJson(QJsonDocument::Indented);
    if(!url.isLocalFile()||!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit()){m_api->notify(tr("Cannot save hotkeys JSON file."),true);return;}m_api->notify(tr("Hotkeys exported."));
}
QString AccountManager::keySequence(const QString &action,const QString &fallback)const{
    QString canonical=action;
    if(action.startsWith("annotation:"))canonical="annotation:"+action;
    else if(action.startsWith("region:"))canonical="regions:"+action;
    else if(action.startsWith("tool:"))canonical="tools:"+action;
    const auto lookup=[&](const QJsonObject &keys){return keys.value(keys.contains(canonical)?canonical:action).toObject();};
    auto value=lookup(m_projectKeys);if(value.isEmpty())value=lookup(m_globalKeys);
    if(value.isEmpty())return fallback;if(!value.value("active").toBool(true))return {};
    QString key=value.value("key").toString();key.replace("cmd+","Meta+",Qt::CaseInsensitive);key.replace("command+","Meta+",Qt::CaseInsensitive);key.replace("option+","Alt+",Qt::CaseInsensitive);
    const auto sequence=QKeySequence::fromString(key,QKeySequence::PortableText);return sequence.isEmpty()?fallback:sequence.toString(QKeySequence::PortableText);
}
void AccountManager::copyText(const QString &text){QGuiApplication::clipboard()->setText(text);}
void AccountManager::changePassword(const QString &oldPassword,const QString &newPassword){
    if(!ready()||m_api->busy())return;
    if(oldPassword.isEmpty()||newPassword.isEmpty()){m_api->notify(tr("Enter your current and new passwords."),true);return;}
    m_api->request("POST","/api/current-user/password/",{{"old_password",oldPassword},{"new_password",newPassword}},[this](const QJsonDocument &){emit passwordChanged();m_api->notify(tr("Password changed."));});
}
