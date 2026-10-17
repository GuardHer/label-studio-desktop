#include "languagemanager.h"
#include <QCoreApplication>

LanguageManager::LanguageManager(QQmlEngine *engine, QSettings *settings, QObject *parent)
    : QObject(parent), m_engine(engine), m_settings(settings ? settings : &m_defaultSettings), m_previousLocale() {
    Q_INIT_RESOURCE(desktop_translations);
    const QString systemDefault = QLocale::system().language() == QLocale::Chinese ? "zh_CN" : "en";
    const auto saved = m_settings->value("language", systemDefault).toString();
    setLanguage(saved == "zh_CN" || saved == "en" ? saved : systemDefault, false);
}

LanguageManager::~LanguageManager() {
    QCoreApplication::removeTranslator(&m_application);
    QCoreApplication::removeTranslator(&m_qtQuick);
    QCoreApplication::removeTranslator(&m_qtBase);
    QLocale::setDefault(m_previousLocale);
}

bool LanguageManager::setLanguage(const QString &code, bool persist) {
    if (code != "zh_CN" && code != "en") return false;
    if (code != m_code) {
        // Load every catalog first, so missing deployment resources cannot leave
        // the application half translated or reset any active editing state.
        if (code == "zh_CN" &&
            (!m_application.load(":/i18n/desktop_zh_CN.qm") ||
             !m_qtBase.load(":/i18n/qtbase_zh_CN.qm") ||
             !m_qtQuick.load(":/i18n/qtdeclarative_zh_CN.qm"))) return false;
        QCoreApplication::removeTranslator(&m_application);
        QCoreApplication::removeTranslator(&m_qtQuick);
        QCoreApplication::removeTranslator(&m_qtBase);
        if (code == "zh_CN") {
            QCoreApplication::installTranslator(&m_qtBase);
            QCoreApplication::installTranslator(&m_qtQuick);
            QCoreApplication::installTranslator(&m_application);
        }
        m_code = code;
        QLocale::setDefault(QLocale(localeName()));
        if (m_engine) m_engine->retranslate();
        emit languageChanged();
    }
    if (persist) { m_settings->setValue("language", code); m_settings->sync(); }
    return true;
}
