#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <QVariantList>
#include <QStringList>
class ApiClient;
class DataManager:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList columns READ columns NOTIFY changed)
    Q_PROPERTY(QStringList columnOrder READ columnOrder NOTIFY changed)
    Q_PROPERTY(QVariantList visibleColumns READ visibleColumns NOTIFY changed)
    Q_PROPERTY(QVariantList views READ views NOTIFY changed)
    Q_PROPERTY(QVariantList actions READ actions NOTIFY changed)
    Q_PROPERTY(QVariantList actors READ actors NOTIFY changed)
    Q_PROPERTY(QVariantList actionFields READ actionFields NOTIFY changed)
    Q_PROPERTY(QVariantMap filters READ filters NOTIFY changed)
    Q_PROPERTY(QStringList ordering READ ordering NOTIFY changed)
    Q_PROPERTY(int activeView READ activeView NOTIFY changed)
    Q_PROPERTY(int pageSize READ pageSize NOTIFY changed)
    Q_PROPERTY(bool compact READ compact WRITE setCompact NOTIFY changed)
    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY changed)
    Q_PROPERTY(bool viewDirty READ viewDirty NOTIFY changed)
    Q_PROPERTY(QString actionId READ actionId NOTIFY changed)
    Q_PROPERTY(int totalAnnotations READ totalAnnotations NOTIFY changed)
    Q_PROPERTY(int totalPredictions READ totalPredictions NOTIFY changed)
public:
    explicit DataManager(ApiClient*);
    QStringList columnOrder()const{return m_columnOrder;}
    QVariantList columns() const{return m_columns;} QVariantList visibleColumns() const;
    QVariantList views()const{return m_views;} QVariantList actions()const{return m_actions;} QVariantList actors()const{return m_actors;}
    QVariantList actionFields()const{return m_actionFields;}
    QVariantMap filters()const{return m_filters.toVariantMap();} QStringList ordering()const{return m_ordering;}
    int activeView()const{return m_activeView;} int pageSize()const{return m_pageSize;}
    bool compact()const{return m_compact;} bool pinned()const{return m_pinned;} bool viewDirty()const{return m_dirty;}
    QString actionId()const{return m_actionId;}
    int totalAnnotations()const{return m_totalAnnotations;} int totalPredictions()const{return m_totalPredictions;}
    void reset();void load();QJsonObject query()const;void totals(int annotations,int predictions);
    Q_INVOKABLE bool applyQuery(const QVariantMap &filters,const QStringList &ordering);
    Q_INVOKABLE void setPageSize(int size);
    Q_INVOKABLE void activateView(int id);
    Q_INVOKABLE void createView(const QString &title);
    Q_INVOKABLE void saveView();Q_INVOKABLE void renameView(const QString &title);Q_INVOKABLE void deleteView();
    Q_INVOKABLE void prepareAction(const QString &id);
    Q_INVOKABLE void executeAction(const QString &id,const QVariantMap &values);
    Q_INVOKABLE void setColumnVisible(const QString &key,bool visible);
    Q_INVOKABLE void moveColumn(const QString &key,int delta);
    Q_INVOKABLE void setColumnWidth(const QString &key,int width);
    Q_INVOKABLE QVariantList operators(const QString &key,const QString &type=QString())const;
    Q_INVOKABLE QVariantMap makeFilter(const QString &key,const QString &op,const QString &value,const QString &end=QString(),const QString &type=QString())const;
    Q_INVOKABLE QString cellText(const QVariantMap &task,const QVariantMap &column)const;
    Q_INVOKABLE QString filtersJson()const;
    Q_INVOKABLE void copyFilters();Q_INVOKABLE void pasteFilters();
    Q_INVOKABLE bool applyFiltersJson(const QString &json);
    void setCompact(bool);void setPinned(bool);
signals:
    void changed();void actionReady();void queryRestored();
private:
    friend class ApiClient;
    ApiClient *m_api;QVariantList m_columns,m_views,m_actions,m_actors,m_actionFields;
    QJsonObject m_filters{{"conjunction","and"},{"items",QJsonArray{}}};
    QStringList m_ordering{"tasks:id"},m_columnOrder;QVariantMap m_visible,m_widths;
    int m_activeView=0,m_pageSize=50,m_generation=0,m_totalAnnotations=0,m_totalPredictions=0;
    bool m_compact=false,m_pinned=false,m_dirty=false,m_advanced=false;QString m_actionId;
    void fetchActors(const QString &column,int page,int generation);
    void fetchViews(std::function<void()> done={});QJsonObject viewData()const;void applyViewData(const QJsonObject&);
    QString key(const QVariantMap&)const;QVariantMap column(const QString&)const;void layoutChanged();
};
