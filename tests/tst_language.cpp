#include <QtTest>
#include <QQmlEngine>
#include <QSettings>
#include <QTemporaryDir>
#include "languagemanager.h"
#include "apiclient.h"

class LanguageTest : public QObject {
    Q_OBJECT
private slots:
    void persistsChoiceAndTranslatesNativeErrors() {
        QTemporaryDir directory;
        const auto path = directory.filePath("preferences.ini");
        {
            QSettings settings(path, QSettings::IniFormat);
            LanguageManager language(nullptr, &settings);
            QVERIFY(!settings.contains("language"));
            const auto systemDefault = QLocale::system().language() == QLocale::Chinese ? "zh_CN" : "en";
            QCOMPARE(language.code(), systemDefault);
            QVERIFY(language.setLanguage("en"));
            const auto previous = settings.value("language");
            QVERIFY(!language.setLanguage("unsupported"));
            QCOMPARE(settings.value("language"), previous);
            QVERIFY(language.setLanguage("zh_CN"));
            QCOMPARE(settings.value("language").toString(), "zh_CN");
            AnnotationDocument document; ApiClient api(&document);
            api.login("http://localhost:8080", "", "");
            QCOMPARE(api.message(), QStringLiteral("请输入邮箱和密码。"));
            QVERIFY(!document.configure("<View><Unknown/></View>"));
            QVERIFY(document.error().contains(QStringLiteral("已禁用编辑和保存")));
            QCOMPARE(QCoreApplication::translate("QPlatformTheme", "Cancel"), QStringLiteral("取消"));
        }
        {
            QSettings settings(path, QSettings::IniFormat);
            LanguageManager restarted(nullptr, &settings);
            QCOMPARE(restarted.code(), "zh_CN");
            QVERIFY(restarted.setLanguage("en"));
            AnnotationDocument document; ApiClient api(&document);
            api.login("http://localhost:8080", "", "");
            QCOMPARE(api.message(), "Enter your email and password.");
            QVERIFY(!document.configure("<View><Unknown/></View>"));
            QVERIFY(document.error().startsWith("Unsupported annotation control"));
            QCOMPARE(QCoreApplication::translate("QPlatformTheme", "Cancel"), "Cancel");
        }
        {
            QSettings settings(path, QSettings::IniFormat);
            LanguageManager restarted(nullptr, &settings);
            QCOMPARE(restarted.code(), "en");
        }
    }
};
QTEST_GUILESS_MAIN(LanguageTest)
#include "tst_language.moc"
