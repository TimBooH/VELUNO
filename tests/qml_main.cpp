#include "PlayerFixture.h"
#include "backup/BackupService.h"
#include "core/AppSettings.h"
#include "core/LanguageController.h"
#include "library/LibraryController.h"
#include "metadata/MetadataController.h"
#include "player/MpvEngine.h"
#include "player/PlayerSettings.h"
#include "player/VideoItem.h"
#include "profiles/PersonalController.h"
#include "updater/UpdateService.h"
#include <QPainter>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QWindow>
#include <QtQuickTest/quicktest.h>
#include <QtTest/QTest>
#include <clocale>
#include <memory>
class Setup : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl videoFixture READ videoFixture CONSTANT)
    Q_PROPERTY(QUrl libraryFolder READ libraryFolder CONSTANT)
    Q_PROPERTY(qint64 unknownFileId READ unknownFileId)
    QTemporaryDir m_directory;
    std::unique_ptr<veluno::Database> m_database;
    std::unique_ptr<veluno::AppSettings> m_settings;
    std::unique_ptr<veluno::LanguageController> m_localization;
    std::unique_ptr<veluno::MpvEngine> m_player;
    std::unique_ptr<veluno::PlayerSettings> m_playerSettings;
    std::unique_ptr<veluno::LibraryController> m_library;
    std::unique_ptr<veluno::PersonalController> m_personal;
    std::unique_ptr<veluno::BackupService> m_backups;
    std::unique_ptr<veluno::MetadataController> m_metadata;
    std::unique_ptr<veluno::UpdateService> m_updates;

  public:
    QUrl libraryFolder() const {
        return QUrl::fromLocalFile(m_directory.filePath("library-media"));
    }
    qint64 unknownFileId() const {
        veluno::LibraryRepository repo(*m_database);
        return repo.file(libraryFolder().toLocalFile() + "/clip001.mp4").value("id").toLongLong();
    }
    QUrl videoFixture() const {
        return QUrl::fromLocalFile(m_directory.filePath("ui-video.mp4"));
    }
    Q_INVOKABLE QString artworkFixture(int index) {
        // Entirely synthetic test art; no provider or network access.
        const QString relative = QString("images/test-artwork-%1.png").arg(index);
        const QString path = m_directory.filePath("app/" + relative);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QImage image(360, 540, QImage::Format_RGB32);
        QPainter painter(&image);
        QLinearGradient gradient(0, 0, 360, 540);
        gradient.setColorAt(0, QColor::fromHsv((index * 47 + 195) % 360, 160, 110));
        gradient.setColorAt(1, QColor("#060C18"));
        painter.fillRect(image.rect(), gradient);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor::fromHsv((index * 47 + 195) % 360, 100, 235), 3));
        for (int ring = 0; ring < 7; ++ring)
            painter.drawEllipse(QPointF(180, 220), 35 + ring * 20, 35 + ring * 20);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Sans Serif", 26, QFont::Bold));
        painter.drawText(
            QRect(24, 390, 312, 100), Qt::AlignCenter | Qt::TextWordWrap,
            QStringList{"ORBIT", "NACHTZUG", "GEZEITEN", "FERNWEH", "LICHT", "MORGEN"}.at(index %
                                                                                          6));
        painter.end();
        return image.save(path) ? relative : QString{};
    }
    Q_INVOKABLE QUrl artworkFixtureUrl(int index) {
        return QUrl::fromLocalFile(m_directory.filePath("app/" + artworkFixture(index)));
    }
    Q_INVOKABLE qint64 detailSeriesFixture() {
        const auto folder = m_directory.filePath("detail-series");
        QDir().mkpath(folder);
        veluno::LibraryRepository repo(*m_database);
        const auto source = repo.addSource(folder, "series");
        qint64 title = 0;
        for (int number = 0; number < 3; ++number) {
            veluno::ScannedFile file;
            file.sourceId = source;
            file.path =
                folder +
                QString("/Orbit.S%1E%2.mp4").arg(number == 2 ? 2 : 1).arg(number == 1 ? 2 : 1);
            if (!QFileInfo::exists(file.path))
                QFile::copy(videoFixture().toLocalFile(), file.path);
            file.kind = "series";
            file.title = "Orbit";
            file.season = number == 2 ? 2 : 1;
            file.episode = number == 1 ? 2 : 1;
            file.width = 96;
            file.height = 64;
            file.duration = 14;
            file.available = true;
            repo.record(file, "detail-test");
            title = repo.file(file.path).value("title_id").toLongLong();
        }
        m_library->reload();
        return title;
    }
    Q_INVOKABLE void seedDetailProgress(qint64 title, int season, int episode, double position) {
        const auto files = m_database->query("SELECT id FROM media_files WHERE title_id=? AND "
                                             "season=? AND episode=? ORDER BY height DESC LIMIT 1",
                                             {title, season, episode});
        if (files.isEmpty())
            qFatal("Missing detail fixture file");
        m_database->query("INSERT INTO "
                          "playback_progress(profile_id,title_id,season,episode,file_id,position,"
                          "duration,seen,updated_at) VALUES(?,?,?,?,?,?,14,0,datetime('now')) ON "
                          "CONFLICT(profile_id,title_id,season,episode) DO UPDATE SET "
                          "file_id=excluded.file_id,position=excluded.position,duration=14,seen=0,"
                          "updated_at=excluded.updated_at",
                          {m_database->setting("active_profile", "1").toLongLong(), title, season,
                           episode, files.first().toMap().value("id"), position});
        m_personal->refresh();
    }
    Q_INVOKABLE void resetSetupForTests() {
        m_localization->setLanguage("de");
        m_database->setSetting("setup_complete", "1");
        m_database->setSetting("online_consent", "denied");
        m_library->reload();
    }
    Q_INVOKABLE void restartSetupForTests() {
        m_database->setSetting("setup_complete", "0");
        m_database->setSetting("wizard_step", "0");
        m_database->setSetting("online_consent", "undecided");
        m_library->reload();
    }
    Q_INVOKABLE void pressKey(QWindow *window, int key) {
        QTest::keyClick(window, static_cast<Qt::Key>(key));
    }
  public slots:
    void applicationAvailable() {
        QQuickStyle::setStyle("Basic");
        std::setlocale(LC_NUMERIC, "C");
        qmlRegisterUncreatableType<veluno::MpvEngine>("Veluno.Player", 1, 0, "Playback",
                                                      "Provided by the application");
        qmlRegisterType<veluno::VideoItem>("Veluno.Player", 1, 0, "VideoView");
        m_player = std::make_unique<veluno::MpvEngine>(nullptr, true);
        if (!createPlayerFixture(videoFixture().toLocalFile()))
            qFatal("UI test fixture generation failed");
        QDir().mkpath(libraryFolder().toLocalFile());
        QFile::copy(videoFixture().toLocalFile(),
                    libraryFolder().toLocalFile() + "/Example.Movie.2020.mp4");
        QFile::copy(videoFixture().toLocalFile(), libraryFolder().toLocalFile() + "/clip001.mp4");
        m_database = std::make_unique<veluno::Database>(m_directory.filePath("app/veluno.sqlite3"));
        m_settings = std::make_unique<veluno::AppSettings>(*m_database);
        m_localization = std::make_unique<veluno::LanguageController>(*m_database);
        m_playerSettings = std::make_unique<veluno::PlayerSettings>(*m_database, *m_player);
        m_database->setSetting("setup_complete", "1");
        m_database->setSetting("online_consent", "denied");
        m_library = std::make_unique<veluno::LibraryController>(*m_database, false, false);
        m_personal =
            std::make_unique<veluno::PersonalController>(*m_database, *m_library, *m_player);
        m_backups = std::make_unique<veluno::BackupService>(*m_database, false);
        m_metadata = std::make_unique<veluno::MetadataController>(
            *m_database, *m_library, *m_personal, nullptr, QUrl("http://127.0.0.1:1"));
        m_updates = std::make_unique<veluno::UpdateService>(*m_database, *m_backups, false);
    }
    void qmlEngineAvailable(QQmlEngine *engine) {
        connect(m_localization.get(), &veluno::LanguageController::languageChanged, engine,
                &QQmlEngine::retranslate);
        engine->rootContext()->setContextProperty("localization", m_localization.get());
        engine->rootContext()->setContextProperty("updates", m_updates.get());
        engine->rootContext()->setContextProperty("metadata", m_metadata.get());
        engine->rootContext()->setContextProperty("playerSettings", m_playerSettings.get());
        engine->rootContext()->setContextProperty("backups", m_backups.get());
        engine->rootContext()->setContextProperty("personal", m_personal.get());
        engine->rootContext()->setContextProperty("library", m_library.get());
        engine->rootContext()->setContextProperty("playback", m_player.get());
        engine->rootContext()->setContextProperty("testInput", this);
        engine->rootContext()->setContextProperty("appSettings", m_settings.get());
        engine->rootContext()->setContextProperty("screenshotDirectory",
                                                  qEnvironmentVariable("VELUNO_TEST_SCREENSHOTS"));
    }
    void cleanupTestCase() {
        m_updates.reset();
        m_metadata.reset();
        m_backups.reset();
        m_personal.reset();
        m_library.reset();
        m_playerSettings.reset();
        m_player.reset();
        m_localization.reset();
        m_settings.reset();
        m_database.reset();
    }
};
QUICK_TEST_MAIN_WITH_SETUP(veluno, Setup)
#include "qml_main.moc"
