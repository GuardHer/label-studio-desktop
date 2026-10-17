#pragma once
#include <QObject>
#include <QVariantList>
class ApiClient;

class ProjectCatalog : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY changed)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY changed)
    Q_PROPERTY(QString order READ order WRITE setOrder NOTIFY changed)
public:
    explicit ProjectCatalog(ApiClient *api);
    QVariantList items() const;
    QVariantList recent() const;
    QString search() const { return m_search; }
    QString order() const { return m_order; }
    void setSearch(const QString &value);
    void setOrder(const QString &value);
    void recordVisit(int id);
    void forget(int id);
signals:
    void changed();
private:
    QString historyKey() const;
    ApiClient *m_api;
    QString m_search, m_order="newest";
};
