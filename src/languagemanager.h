#pragma once
#include <QObject>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QPointer>
#include <QQmlEngine>

class LanguageManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString code READ code NOTIFY languageChanged)
    Q_PROPERTY(QString localeName READ localeName NOTIFY languageChanged)
public:
    explicit LanguageManager(QQmlEngine *engine, QSettings *settings = nullptr, QObject *parent = nullptr);
    ~LanguageManager() override;
    QString code() const { return m_code; }
    QString localeName() const { return m_code == "zh_CN" ? "zh_CN" : "en_US"; }
    Q_INVOKABLE bool setLanguage(const QString &code, bool persist = true);
signals:
    void languageChanged();
private:
    QPointer<QQmlEngine> m_engine;
    QSettings m_defaultSettings;
    QSettings *m_settings;
    QTranslator m_application, m_qtBase, m_qtQuick;
    QString m_code;
    QLocale m_previousLocale;
};
