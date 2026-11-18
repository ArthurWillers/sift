// sift — fast keyboard-and-swipe photo triage for Omarchy.

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "portalfilepicker.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("sift");
    app.setApplicationVersion("0.1.0");
    app.setDesktopFileName("sift");
    app.setWindowIcon(QIcon::fromTheme("sift"));

    QQuickStyle::setStyle("Material");

    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    PortalFilePicker picker;
    Backend backend(&picker);

    const QStringList args = app.arguments();
    if (args.size() > 1)
        backend.openFolder(args.at(1));

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
