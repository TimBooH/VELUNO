#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>
#include "core/AppSettings.h"
#include "player/MpvEngine.h"
#include "scanner/Scanner.h"
#include "metadata/MetadataProvider.h"
#include <exception>
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VELUNO");
    QCoreApplication::setApplicationName("VELUNO");
    QCoreApplication::setApplicationVersion("0.1.0-dev");
    QQuickStyle::setStyle("Basic");
    try {
        const auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/veluno.sqlite3";
        veluno::Database database(path);
        veluno::AppSettings settings(database);
        veluno::MpvEngine player;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("appSettings", &settings);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        if (engine.rootObjects().isEmpty()) return 1;
        qInfo() << "VELUNO ready; platform:" << QGuiApplication::platformName()
                << "database:" << path << "mpv API:" << player.apiVersion();
        // Bounded startup verification used by CI; close through the real window.
        if (app.arguments().contains("--smoke-test")) {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            if (!window) return 1;
            QTimer::singleShot(800, window, &QWindow::close);
        }
        return app.exec();
    } catch (const std::exception &e) {
        qCritical().noquote() << "VELUNO kann nicht starten:" << e.what();
        return 1;
    }
}
