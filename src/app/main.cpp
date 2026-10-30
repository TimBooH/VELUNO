#include <clocale>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>
#include "core/AppSettings.h"
#include "library/LibraryController.h"
#include "player/MpvEngine.h"
#include "player/VideoItem.h"
#include <QSGRendererInterface>
#include "scanner/Scanner.h"
#include "metadata/MetadataProvider.h"
#include <exception>
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    std::setlocale(LC_NUMERIC, "C");
    QCoreApplication::setOrganizationName("VELUNO");
    QCoreApplication::setApplicationName("VELUNO");
    QCoreApplication::setApplicationVersion("0.1.0-dev");
    QQuickStyle::setStyle("Basic");
    try {
        const auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/veluno.sqlite3";
        veluno::Database database(path);
        veluno::AppSettings settings(database);
        veluno::MpvEngine player;
        player.setVolume(database.setting("player_volume", "70").toDouble());
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
            try { database.setSetting("player_volume", QString::number(player.volume())); }
            catch (const std::exception &e) { qWarning() << "Lautstärke konnte nicht gespeichert werden:" << e.what(); }
        });
        qmlRegisterUncreatableType<veluno::MpvEngine>("Veluno.Player", 1, 0, "Playback", "Provided by the application");
        qmlRegisterType<veluno::VideoItem>("Veluno.Player", 1, 0, "VideoView");
        const int scanArgument = app.arguments().indexOf("--smoke-scan");
        const bool scanSmoke = app.arguments().contains("--smoke-test") && scanArgument >= 0;
        veluno::LibraryController library(database,!scanSmoke,!scanSmoke);
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("appSettings", &settings);
        engine.rootContext()->setContextProperty("playback", &player);
        engine.rootContext()->setContextProperty("library", &library);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        if (engine.rootObjects().isEmpty()) return 1;
        qInfo() << "VELUNO ready; platform:" << QGuiApplication::platformName()
                << "database:" << path << "mpv API:" << player.apiVersion();
        // Bounded startup verification used by CI; close through the real window.
        if (scanSmoke) {
            if (scanArgument + 1 >= app.arguments().size()) return 2;
            QObject::connect(&library,&veluno::LibraryController::scanCompleted,&app,[&](bool interrupted) {
                const bool success = !interrupted && library.movieCount()>0 && library.errorCount()==0;
                qInfo() << "Library scan smoke test:" << success << "movies:" << library.movieCount() << "errors:" << library.errorCount();
                QCoreApplication::exit(success ? 0 : 2);
            });
            QObject::connect(&library,&veluno::LibraryController::errorChanged,&app,[&] {
                if (!library.error().isEmpty()) { qCritical().noquote() << library.error(); QCoreApplication::exit(2); }
            });
            QTimer::singleShot(20000,&app,[]{ QCoreApplication::exit(3); });
            const auto folder=QUrl::fromLocalFile(app.arguments().at(scanArgument+1));
            QTimer::singleShot(0,&library,[&,folder]{ library.addSource(folder,"movie"); library.scan(); });
        } else if (app.arguments().contains("--smoke-test")) {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            if (!window) return 1;
            if (app.arguments().contains("--smoke-dialog")) {
                QTimer::singleShot(0, &engine, [&] {
                    auto *dialog=engine.rootObjects().first()->findChild<QObject *>("mediaFolderPicker");
                    if (!dialog || !QMetaObject::invokeMethod(dialog,"open")) { QCoreApplication::exit(2); return; }
                    QTimer::singleShot(200, &app, [dialog] {
                        const bool visible=dialog->property("visible").toBool();
                        qInfo() << "Folder dialog smoke test:" << visible;
                        if (!visible) { QCoreApplication::exit(2); return; }
                        QMetaObject::invokeMethod(dialog,"reject");
                    });
                });
            }
            QTimer::singleShot(800, window, &QWindow::close);
        }
        return app.exec();
    } catch (const std::exception &e) {
        qCritical().noquote() << "VELUNO kann nicht starten:" << e.what();
        return 1;
    }
}
