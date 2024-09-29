#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("org.kde.breeze"));
    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName("Koto");
    app.setDesktopFileName("com.github.joshstrobl.koto.desktop");

    QQmlApplicationEngine engine;

    engine.loadFromModule("com.github.joshstrobl.koto", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
