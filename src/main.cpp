#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>
#include <QFontDatabase>
#include "apiclient.h"
#include "languagemanager.h"
#include "imageprovider.h"
#include "labelingconfig.h"

int main(int argc, char **argv) {
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    QGuiApplication app(argc, argv);
    app.setOrganizationName("LabelStudioNative");
    app.setApplicationName("LabelStudioDesktop");
    app.setApplicationVersion("0.1.0");
    QQuickStyle::setStyle("Material");
    registerLabelingTypes();
    QFontDatabase::addApplicationFont(":/assets/Figtree-Regular.ttf");
    QFontDatabase::addApplicationFont(":/assets/Figtree-SemiBold.ttf");
    app.setFont(QFont("Figtree", 10));
    AnnotationDocument document;
    ApiClient backend(&document);
    QQmlApplicationEngine engine;
    auto *language = new LanguageManager(&engine, nullptr, &engine);
    engine.rootContext()->setContextProperty("languageManager", language);
    auto *provider = new TaskImageProvider;
    engine.addImageProvider("task", provider);
    QObject::connect(&backend, &ApiClient::imageReady, &backend, [=](const QImage &image) { provider->setImage(image); });
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("annotationDocument", &document);
    engine.rootContext()->setContextProperty("initialServer", QSettings().value("serverUrl", "http://127.0.0.1:8080").toString());
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    const auto arguments = app.arguments();
    if (arguments.contains("--demo")) backend.loadDemo();
    const int page = arguments.indexOf("--page");
    if (page >= 0 && page + 1 < arguments.size()) {
        const auto view = arguments[page + 1];
        engine.rootObjects().first()->setProperty("page", view == "create" ? "home" : view);
        if (view == "create") {
            auto *dialog = engine.rootObjects().first()->findChild<QObject *>("createProjectDialog");
            if (dialog) QMetaObject::invokeMethod(dialog, "open");
        }
    }
    const int screenshot = arguments.indexOf("--screenshot");
    if (screenshot >= 0 && screenshot+1 < arguments.size()) {
        const auto path = arguments[screenshot+1];
        QTimer::singleShot(1500, &app, [&, path] {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            app.exit(window && window->grabWindow().save(path) ? 0 : 2);
        });
    }
    return app.exec();
}
