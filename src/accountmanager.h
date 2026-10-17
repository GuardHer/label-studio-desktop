#pragma once
#include <QObject>
#include <QJsonObject>
#include <QVariantList>
#include <QUrl>
#include <functional>
class ApiClient;

class AccountManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY changed)
    Q_PROPERTY(QVariantMap organization READ organization NOTIFY changed)
    Q_PROPERTY(QVariantList members READ members NOTIFY changed)
    Q_PROPERTY(int memberPage READ memberPage NOTIFY changed)
    Q_PROPERTY(int memberCount READ memberCount NOTIFY changed)
    Q_PROPERTY(QVariantMap memberDetail READ memberDetail NOTIFY changed)
    Q_PROPERTY(QString inviteUrl READ inviteUrl NOTIFY changed)
    Q_PROPERTY(QVariantMap tokenSettings READ tokenSettings NOTIFY changed)
    Q_PROPERTY(QVariantList tokens READ tokens NOTIFY changed)
    Q_PROPERTY(QString secret READ secret NOTIFY changed)
    Q_PROPERTY(QString legacyToken READ legacyToken NOTIFY changed)
    Q_PROPERTY(QString hotkeyJson READ hotkeyJson NOTIFY hotkeysChanged)
    Q_PROPERTY(int hotkeyProject READ hotkeyProject NOTIFY hotkeysChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
public:
    explicit AccountManager(ApiClient *api);
    QVariantMap profile() const { return m_profile.toVariantMap(); }
    QVariantMap organization() const { return m_org.toVariantMap(); }
    QVariantList members() const { return m_members; }
    int memberPage() const { return m_memberPage; }
    int memberCount() const { return m_memberCount; }
    QVariantMap memberDetail() const { return m_detail.toVariantMap(); }
    QString inviteUrl() const { return m_invite; }
    QVariantMap tokenSettings() const { return m_settings.toVariantMap(); }
    QVariantList tokens() const { return m_tokens; }
    QString secret() const { return m_secret; }
    QString legacyToken() const { return m_legacy; }
    QString hotkeyJson() const;
    int hotkeyProject() const { return m_hotkeyProject; }
    bool loaded() const { return m_loaded; }
    void clear();
    void activateProject(int projectId);
    Q_INVOKABLE void load();
    Q_INVOKABLE void saveProfile(const QString &first, const QString &last, const QString &phone);
    Q_INVOKABLE void saveEmailPreferences(bool allowed);
    Q_INVOKABLE void uploadAvatar(const QUrl &file);
    Q_INVOKABLE void removeAvatar();
    Q_INVOKABLE void saveOrganization(const QString &title, const QString &contact);
    Q_INVOKABLE void loadMembers(int page = 1, const QString &search = QString());
    Q_INVOKABLE void loadMember(int userId);
    Q_INVOKABLE void removeMember(int userId);
    Q_INVOKABLE void loadInvite();
    Q_INVOKABLE void resetInvite();
    Q_INVOKABLE void loadTokens();
    Q_INVOKABLE void createToken();
    Q_INVOKABLE void revokeToken(const QString &token);
    Q_INVOKABLE void rotateToken(const QString &token);
    Q_INVOKABLE void clearSecret();
    Q_INVOKABLE void loadLegacyToken();
    Q_INVOKABLE void resetLegacyToken();
    Q_INVOKABLE void saveTokenSettings(bool personal, bool legacy);
    Q_INVOKABLE void loadHotkeys(int projectId = 0);
    Q_INVOKABLE void saveHotkeys(const QString &json);
    Q_INVOKABLE void importHotkeys(const QUrl &file);
    Q_INVOKABLE void exportHotkeys(const QUrl &file, const QString &json);
    Q_INVOKABLE QString keySequence(const QString &action, const QString &fallback) const;
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void changePassword(const QString &oldPassword, const QString &newPassword);
signals:
    void changed();
    void profileSaved();
    void organizationSaved();
    void hotkeysChanged();
    void hotkeysSaved();
    void hotkeysImported(const QString &json);
    void passwordChanged();
    void tokenPolicySaved();
private:
    bool ready() const;
    int orgId() const;
    void fetchProfile(std::function<void()> done = {});
    void setInvite(const QJsonObject &data);
    ApiClient *m_api;
    QJsonObject m_profile, m_org, m_detail, m_settings, m_hotkeys, m_globalKeys, m_projectKeys;
    QVariantList m_members, m_tokens;
    QString m_invite, m_secret, m_legacy;
    int m_memberPage = 1, m_memberCount = 0, m_hotkeyProject = 0;
    int m_activeProject = 0, m_keyGeneration = 0;
    bool m_loaded = false;
};
