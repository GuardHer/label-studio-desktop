#pragma once
#include "annotationdocument.h"
#include "accountmanager.h"
#include "projectcatalog.h"
#include "datamanager.h"
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QSet>
#include <functional>

class ApiClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(DataManager* dataManager READ dataManager CONSTANT)
    Q_PROPERTY(bool allResultsSelected READ allResultsSelected NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(AccountManager* account READ account CONSTANT)
    Q_PROPERTY(ProjectCatalog* catalog READ catalog CONSTANT)
    Q_PROPERTY(bool samplingQueue READ samplingQueue NOTIFY changed)
    Q_PROPERTY(int samplingPosition READ samplingPosition NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool demo READ demo NOTIFY changed)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool messageIsError READ messageIsError NOTIFY changed)
    Q_PROPERTY(QVariantList projects READ projects NOTIFY changed)
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY changed)
    Q_PROPERTY(QVariantList annotations READ annotations NOTIFY changed)
    Q_PROPERTY(QVariantMap project READ project NOTIFY changed)
    Q_PROPERTY(QVariantMap taskData READ taskData NOTIFY changed)
    Q_PROPERTY(QVariantMap taskSnapshot READ taskSnapshot NOTIFY changed)
    Q_PROPERTY(int taskId READ taskId NOTIFY changed)
    Q_PROPERTY(int page READ page NOTIFY changed)
    Q_PROPERTY(int totalTasks READ totalTasks NOTIFY changed)
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY changed)
    Q_PROPERTY(QVariantMap user READ user NOTIFY changed)
    Q_PROPERTY(QVariantMap thumbnails READ thumbnails NOTIFY thumbnailsChanged)
    Q_PROPERTY(QString taskSearch READ taskSearch NOTIFY changed)
    Q_PROPERTY(int taskStatus READ taskStatus NOTIFY changed)
    Q_PROPERTY(bool taskDescending READ taskDescending NOTIFY changed)
    Q_PROPERTY(QVariantList selectedTasks READ selectedTasks NOTIFY changed)
    Q_PROPERTY(int selectedTaskCount READ selectedTaskCount NOTIFY changed)
    Q_PROPERTY(bool groundTruth READ groundTruth NOTIFY changed)
public:
    DataManager *dataManager() const { return m_dataManager; }
    bool allResultsSelected() const { return m_allSelected; }
    Q_INVOKABLE bool isTaskSelected(int id) const { return m_allSelected ? !m_excludedTasks.contains(id) : m_selectedTasks.contains(id); }
    Q_INVOKABLE void selectAllResults();
    AccountManager *account() const { return m_account; }
    ProjectCatalog *catalog() const { return m_catalog; }
    bool samplingQueue() const { return m_samplingQueue; }
    int samplingPosition() const { return m_samplingIndex+1; }
    explicit ApiClient(AnnotationDocument *document, QObject *parent = nullptr);
    bool busy() const { return m_pending > 0; }
    bool connected() const { return m_connected; }
    bool demo() const { return m_demo; }
    QString serverUrl() const { return m_base.toString(); }
    QString message() const { return m_message; }
    bool messageIsError() const { return m_messageIsError; }
    QVariantList projects() const { return m_projects; }
    QVariantList tasks() const { return m_tasks; }
    QVariantList annotations() const;
    QVariantMap project() const { return m_project.toVariantMap(); }
    QVariantMap taskData() const { return m_task.value("data").toObject().toVariantMap(); }
    QVariantMap taskSnapshot() const { return m_task.toVariantMap(); }
    int taskId() const { return m_taskId; }
    int page() const { return m_page; }
    int totalTasks() const { return m_totalTasks; }
    QString imageSource() const { return m_imageSource; }
    QVariantMap user() const { return m_user.toVariantMap(); }
    QVariantMap thumbnails() const { return m_thumbnails; }
    Q_INVOKABLE void requestThumbnail(int taskId, const QString &field = QString());
    Q_INVOKABLE void login(const QString &url, const QString &email, const QString &password);
    Q_INVOKABLE void signup(const QString &url, const QString &email, const QString &password, const QString &invitation, bool newsletters);
    Q_INVOKABLE void disconnectServer();
    Q_INVOKABLE void createProject(const QString &title, const QString &description, const QString &config);
    Q_INVOKABLE void updateProject(const QString &title, const QString &description, const QString &config);
    Q_INVOKABLE void updateProjectDetails(const QString &title,const QString &description,const QString &config,const QString &color,const QString &sampling);
    Q_INVOKABLE void deleteProject();
    Q_INVOKABLE void deleteProjectById(int id);
    Q_INVOKABLE void resetProjectCache();
    Q_INVOKABLE void dropProjectTabs();
    Q_INVOKABLE void startLabeling();
    Q_INVOKABLE void importFiles(const QVariantList &files);
    Q_INVOKABLE void importUrl(const QString &url);
    Q_INVOKABLE void exportProject(const QUrl &file);
    Q_INVOKABLE void connectTo(const QString &url, const QString &token, int authMode);
    Q_INVOKABLE void refreshProjects();
    Q_INVOKABLE void openProject(int id);
    Q_INVOKABLE void loadPage(int page);
    Q_INVOKABLE void openTask(int id);
    Q_INVOKABLE void selectAnnotation(int id);
    Q_INVOKABLE void save();
    QString taskSearch() const { return m_taskSearch; }
    int taskStatus() const { return m_taskStatus; }
    bool taskDescending() const { return m_taskDescending; }
    QVariantList selectedTasks() const;
    int selectedTaskCount() const { return m_allSelected ? qMax(0,m_totalTasks-m_excludedTasks.size()) : m_selectedTasks.size(); }
    bool groundTruth() const;
    Q_INVOKABLE bool setTaskQuery(const QString &search, int status, bool descending);
    Q_INVOKABLE void moveTask(int delta);
    Q_INVOKABLE void saveAndNext();
    Q_INVOKABLE void skipTask();
    Q_INVOKABLE void selectTask(int id, bool selected);
    Q_INVOKABLE void selectPage(bool selected);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void runTaskAction(const QString &action);
    Q_INVOKABLE void deleteAnnotation();
    Q_INVOKABLE void setGroundTruth(bool enabled);
    Q_INVOKABLE void discardChanges();
    Q_INVOKABLE void loadDemo();
    Q_INVOKABLE void clearMessage();
signals:
    void changed();
    void projectOpened();
    void taskQueryFailed();
    void taskOpened();
    void workflowFinished();
    void imageReady(const QImage &image);
    void saved();
    void projectCreated(int id);
    void projectDeleted();
    void imported();
    void projectUpdated();
    void projectTabsReset();
    void thumbnailsChanged();
private:
    friend class AccountManager;
    friend class LabelingConfig;
    friend class DataManager;
    DataManager *m_dataManager;
    bool m_allSelected=false;
    QSet<int> m_excludedTasks;
    QJsonObject m_queueSelection;
    QJsonObject selectionQuery() const;
    void clearConnection();
    void authenticateForm(const QString &address, const QString &email, const QString &password, const QString &invitation, bool newsletters, bool signup);
    AccountManager *m_account;
    ProjectCatalog *m_catalog;
    using ErrorCallback = std::function<bool(int)>;
    using JsonCallback = std::function<void(const QJsonDocument &)>;
    void request(const QByteArray &method, const QString &path, const QJsonObject &body, JsonCallback callback, bool retry = true, ErrorCallback failure = {});
    void requestPayload(const QByteArray &method, const QString &path, const QByteArray &body, const QByteArray &contentType, JsonCallback callback, bool retry = true, ErrorCallback failure = {});
    bool prepareServer(const QString &address);
    QByteArray csrfToken() const;
    void clearThumbnails();
    void fetchThumbnail(int id, const QUrl &url, int epoch, int redirects = 0, const QString &cacheKey = QString());
    void refreshAccess(std::function<void()> callback, std::function<void()> failed = {});
    void fetchProjects(int page,QVariantList accumulated = {});
    void fetchSampledTask(bool restart = false);
    void fetchTasks(int page, std::function<void()> done = {}, std::function<void()> failed = {});
    void fetchTask(int id, std::function<void()> failed = {});
    void saveCurrent(std::function<void()> done);
    void advanceAfterWrite(int previousIndex);
    void refreshProjectSummary();
    void resetTaskQuery();
    QJsonObject taskQuery() const;
    QString tasksPath(int page) const;
    void fetchImage(const QUrl &url, int redirects = 0);
    void loadActiveImage();
    void acceptImage(const QImage &image);
    void notify(const QString &message, bool error = false);
    bool mayNavigate();
    bool sameOrigin(const QUrl &url) const;
    QUrl endpoint(const QString &path) const;
    AnnotationDocument *m_document;
    QNetworkAccessManager m_network;
    QNetworkAccessManager m_publicNetwork;
    QUrl m_base;
    QByteArray m_credential;
    QString m_refreshToken, m_message, m_imageSource;
    QVariantList m_projects, m_tasks;
    QJsonObject m_project, m_task, m_user;
    QJsonArray m_annotations;
    QVariantMap m_thumbnails;
    QSet<int> m_thumbnailIds;
    QSet<QString> m_thumbnailKeys;
    QSet<QNetworkReply *> m_thumbnailReplies;
    int m_thumbnailEpoch = 0;
    int m_pending = 0, m_taskId = -1, m_page = 1, m_totalTasks = 0, m_imageRevision = 0;
    QString m_taskSearch;
    int m_taskStatus = 0;
    bool m_taskDescending = false;
    QSet<int> m_selectedTasks;
    bool m_samplingQueue=false;
    QList<int> m_samplingHistory;
    int m_samplingIndex=-1;
    bool m_connected = false, m_demo = false, m_messageIsError = false;
    bool m_loadingTask = false;
    int m_mediaEpoch = 0;
};
