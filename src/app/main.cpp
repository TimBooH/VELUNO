#include "backup/BackupService.h"
#include "core/AppSettings.h"
#include "core/LanguageController.h"
#include "library/LibraryController.h"
#include "metadata/MetadataController.h"
#include "metadata/MetadataProvider.h"
#include "player/MpvEngine.h"
#include "player/PlayerSettings.h"
#include "player/VideoItem.h"
#include "profiles/PersonalController.h"
#include "scanner/Scanner.h"
#include "updater/UpdateService.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTimer>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <exception>
int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--build-info") == 0) {
            QCoreApplication info(argc, argv);
            const auto bytes =
                QJsonDocument::fromVariant(veluno::UpdateService::buildInfo()).toJson();
            std::fwrite(bytes.constData(), 1, bytes.size(), stdout);
            return 0;
        }
    QGuiApplication app(argc, argv);
    std::setlocale(LC_NUMERIC, "C");
    QCoreApplication::setOrganizationName("VELUNO");
    QCoreApplication::setApplicationName("VELUNO");
    QCoreApplication::setApplicationVersion("0.1.0-dev");
    QQuickStyle::setStyle("Basic");
    try {
        if (app.arguments().contains("--smoke-test")) {
            if (!QSslSocket::setActiveBackend("openssl") || !QSslSocket::supportsSsl())
                throw std::runtime_error("OpenSSL/TLS-Backend ist nicht verfügbar.");
            qInfo() << "TLS backend ready:" << QSslSocket::activeBackend();
        }
        const auto path =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/veluno.sqlite3";
        const auto appData = QFileInfo(path).absolutePath();
        if (!QDir().mkpath(QFileInfo(appData).absolutePath()))
            throw std::runtime_error("Anwendungsordner nicht erreichbar.");
        QLockFile instance(appData + ".lock");
        instance.setStaleLockTime(0);
        if (!instance.tryLock(100))
            throw std::runtime_error(
                "VELUNO läuft bereits. Bitte die vorhandene Instanz verwenden.");
        veluno::BackupService::applyPendingRestore(path);
        veluno::BackupService::protectBeforeMigration(path);
        veluno::Database database(path);
        veluno::AppSettings settings(database);
        veluno::LanguageController localization(database);
        veluno::MpvEngine player(nullptr, app.arguments().contains("--smoke-test"));
        veluno::PlayerSettings playerSettings(database, player);
        player.setVolume(database.setting("player_volume", "70").toDouble());
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &settings, [&] {
            try {
                database.setSetting("player_volume", QString::number(player.volume()));
            } catch (const std::exception &e) {
                qWarning() << "Lautstärke konnte nicht gespeichert werden:" << e.what();
            }
        });
        qmlRegisterUncreatableType<veluno::MpvEngine>("Veluno.Player", 1, 0, "Playback",
                                                      "Provided by the application");
        qmlRegisterType<veluno::VideoItem>("Veluno.Player", 1, 0, "VideoView");
        const int scanArgument = app.arguments().indexOf("--smoke-scan");
        const bool scanSmoke = app.arguments().contains("--smoke-test") && scanArgument >= 0;
        veluno::LibraryController library(database, !scanSmoke, !scanSmoke);
        veluno::PersonalController personal(database, library, player);
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &personal,
                         &veluno::PersonalController::saveProgress);
        veluno::BackupService backups(database, !app.arguments().contains("--smoke-test"));
        veluno::MetadataController metadata(database, library, personal);
        veluno::UpdateService updates(database, backups, !app.arguments().contains("--smoke-test"));
        QQmlApplicationEngine engine;
        QObject::connect(&localization, &veluno::LanguageController::languageChanged, &engine,
                         &QQmlApplicationEngine::retranslate);
        engine.rootContext()->setContextProperty("localization", &localization);
        engine.rootContext()->setContextProperty("appSettings", &settings);
        engine.rootContext()->setContextProperty("playback", &player);
        engine.rootContext()->setContextProperty("library", &library);
        engine.rootContext()->setContextProperty("personal", &personal);
        engine.rootContext()->setContextProperty("backups", &backups);
        engine.rootContext()->setContextProperty("playerSettings", &playerSettings);
        engine.rootContext()->setContextProperty("metadata", &metadata);
        engine.rootContext()->setContextProperty("updates", &updates);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        if (engine.rootObjects().isEmpty())
            throw std::runtime_error("Die Benutzeroberfläche konnte nicht geladen werden.");
        qInfo() << "VELUNO ready; platform:" << QGuiApplication::platformName()
                << "database:" << path << "mpv API:" << player.apiVersion();
        // Bounded startup verification used by CI; close through the real window.
        if (scanSmoke) {
            if (scanArgument + 1 >= app.arguments().size())
                return 2;
            QObject::connect(&library, &veluno::LibraryController::scanCompleted, &app,
                             [&](bool interrupted) {
                                 const bool success = !interrupted && library.movieCount() > 0 &&
                                                      library.errorCount() == 0;
                                 qInfo() << "Library scan smoke test:" << success
                                         << "movies:" << library.movieCount()
                                         << "errors:" << library.errorCount();
                                 QCoreApplication::exit(success ? 0 : 2);
                             });
            QObject::connect(&library, &veluno::LibraryController::errorChanged, &app, [&] {
                if (!library.error().isEmpty()) {
                    qCritical().noquote() << library.error();
                    QCoreApplication::exit(2);
                }
            });
            QTimer::singleShot(20000, &app, [] { QCoreApplication::exit(3); });
            const auto folder = QUrl::fromLocalFile(app.arguments().at(scanArgument + 1));
            QTimer::singleShot(0, &library, [&, folder] {
                library.addSource(folder, "movie");
                library.scan();
            });
        } else if (app.arguments().contains("--smoke-test")) {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            if (!window)
                return 1;
            if (app.arguments().contains("--smoke-dialog")) {
                QTimer::singleShot(0, &engine, [&] {
                    auto *dialog =
                        engine.rootObjects().first()->findChild<QObject *>("mediaFolderPicker");
                    if (!dialog || !QMetaObject::invokeMethod(dialog, "open")) {
                        QCoreApplication::exit(2);
                        return;
                    }
                    QTimer::singleShot(200, &app, [dialog] {
                        const bool visible = dialog->property("visible").toBool();
                        qInfo() << "Folder dialog smoke test:" << visible;
                        if (!visible) {
                            QCoreApplication::exit(2);
                            return;
                        }
                        QMetaObject::invokeMethod(dialog, "reject");
                    });
                });
            }
            QTimer::singleShot(800, window, &QWindow::close);
        }
        return app.exec();
    } catch (const std::exception &e) {
        qCritical().noquote() << "VELUNO kann nicht starten:" << e.what();
        if (!app.arguments().contains("--smoke-test")) {
            QQmlApplicationEngine failure;
            failure.rootContext()->setContextProperty("startupError", QString::fromUtf8(e.what()));
            failure.rootContext()->setContextProperty(
                "dataFolder", QUrl::fromLocalFile(QStandardPaths::writableLocation(
                                  QStandardPaths::AppDataLocation)));
            failure.loadData(R"QML(
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    visible: true; width: 640; height: 320; title: "VELUNO"; color: "#0B1424"
    ColumnLayout { anchors.fill: parent; anchors.margins: 28; spacing: 20
        Label { text: "VELUNO kann nicht starten"; color: "#EFF5FF"; font.pixelSize: 24 }
        Label { text: startupError; textFormat: Text.PlainText; color: "#EFF5FF"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Layout.fillHeight: true }
        Label { text: "Bestehende Daten wurden nicht automatisch ersetzt."; color: "#A5B3C7"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        RowLayout { Button { text: "Datenordner öffnen"; onClicked: Qt.openUrlExternally(dataFolder) } Item { Layout.fillWidth: true } Button { text: "Schließen"; onClicked: Qt.quit() } }
    }
}
)QML");
            if (!failure.rootObjects().isEmpty())
                app.exec();
        }
        return 1;
    }
}
