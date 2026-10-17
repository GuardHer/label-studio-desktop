#pragma once
#include <QObject>
#include <QVariantList>
#include <QDomDocument>
#include <QQuickTextDocument>

class LabelingConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString xml READ xml WRITE setXml NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY changed)
    Q_PROPERTY(QVariantList controls READ controls NOTIFY changed)
    Q_PROPERTY(QVariantList objects READ objects NOTIFY changed)
    Q_PROPERTY(QVariantList labels READ labels NOTIFY changed)
    Q_PROPERTY(int selectedControl READ selectedControl WRITE setSelectedControl NOTIFY changed)
    Q_PROPERTY(QVariantList templates READ templates CONSTANT)
    Q_PROPERTY(QStringList groups READ groups CONSTANT)
public:
    explicit LabelingConfig(QObject *parent=nullptr);
    QString xml()const{return m_xml;}
    QString error()const{return m_error;}
    QString message()const{return m_message;}
    QString serverStatus()const{return m_serverStatus;}
    QVariantList controls()const;
    QVariantList objects()const;
    QVariantList labels()const;
    QVariantList templates()const{return m_templates;}
    QStringList groups()const{return m_groups;}
    int selectedControl()const{return m_selected;}
    void setXml(const QString &xml);
    void setSelectedControl(int index);
    Q_INVOKABLE void setLabel(int index,const QString &value,const QString &color,const QString &alias,const QString &hotkey);
    Q_INVOKABLE void removeLabel(int index);
    Q_INVOKABLE void addLabels(const QString &lines);
    Q_INVOKABLE void setObjectValue(int index,const QString &value);
    Q_INVOKABLE void setControlAttribute(const QString &key,const QString &value);
    Q_INVOKABLE void formatXml();
    Q_INVOKABLE QStringList completions(const QString &prefix)const;
    Q_INVOKABLE QStringList attributeCompletions(const QString &tag,const QString &prefix)const;
    Q_INVOKABLE QString templateTitle(const QString &title)const;
    Q_INVOKABLE void validateOnServer(QObject *api,bool existingProject);
    Q_INVOKABLE QVariantMap sampleData()const;
signals:
    void changed();
private:
    void commit();
    QList<QDomElement> elements(const QStringList &tags)const;
    QDomElement selected()const;
    QList<QDomElement> labelElements()const;
    QString m_xml,m_error,m_message,m_serverStatus;
    QDomDocument m_doc;
    QVariantList m_templates;
    QStringList m_groups;
    int m_selected=0;
};

class XmlSyntax : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* document READ document WRITE setDocument NOTIFY documentChanged)
public:
    explicit XmlSyntax(QObject *parent=nullptr):QObject(parent){}
    QQuickTextDocument *document()const{return m_document;}
    void setDocument(QQuickTextDocument *document);
signals:
    void documentChanged();
private:
    QQuickTextDocument *m_document=nullptr;
    QObject *m_highlighter=nullptr;
};
void registerLabelingTypes();
