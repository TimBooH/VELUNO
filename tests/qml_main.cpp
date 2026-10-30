#include <clocale>
#include <QtQuickTest/quicktest.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QtTest/QTest>
#include <QWindow>
#include <memory>
#include "core/AppSettings.h"
#include "player/MpvEngine.h"
#include "player/VideoItem.h"
#include "PlayerFixture.h"
#include "library/LibraryController.h"
class Setup : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl videoFixture READ videoFixture CONSTANT)
    Q_PROPERTY(QUrl libraryFolder READ libraryFolder CONSTANT)
    Q_PROPERTY(qint64 unknownFileId READ unknownFileId)
    QTemporaryDir m_directory;
    std::unique_ptr<veluno::Database> m_database;
    std::unique_ptr<veluno::AppSettings> m_settings;
    std::unique_ptr<veluno::MpvEngine> m_player;
    std::unique_ptr<veluno::LibraryController> m_library;
public:
    QUrl libraryFolder() const { return QUrl::fromLocalFile(m_directory.filePath("library-media")); }
    qint64 unknownFileId() const { veluno::LibraryRepository repo(*m_database); return repo.file(libraryFolder().toLocalFile()+"/clip001.mp4").value("id").toLongLong(); }
    QUrl videoFixture() const { return QUrl::fromLocalFile(m_directory.filePath("ui-video.mp4")); }
    Q_INVOKABLE void resetSetupForTests() { m_database->setSetting("setup_complete","1"); m_database->setSetting("online_consent","denied"); m_library->reload(); }
    Q_INVOKABLE void restartSetupForTests() { m_database->setSetting("setup_complete","0"); m_database->setSetting("wizard_step","0"); m_database->setSetting("online_consent","undecided"); m_library->reload(); }
    Q_INVOKABLE void pressKey(QWindow *window, int key) { QTest::keyClick(window, static_cast<Qt::Key>(key)); }
public slots:
    void applicationAvailable() {
        QQuickStyle::setStyle("Basic");
    std::setlocale(LC_NUMERIC, "C");
        qmlRegisterUncreatableType<veluno::MpvEngine>("Veluno.Player", 1, 0, "Playback", "Provided by the application");
        qmlRegisterType<veluno::VideoItem>("Veluno.Player", 1, 0, "VideoView");
        m_player = std::make_unique<veluno::MpvEngine>(nullptr, true);
        if (!createPlayerFixture(videoFixture().toLocalFile())) qFatal("UI test fixture generation failed");
        QDir().mkpath(libraryFolder().toLocalFile());
        QFile::copy(videoFixture().toLocalFile(),libraryFolder().toLocalFile()+"/Example.Movie.2020.mp4");
        QFile::copy(videoFixture().toLocalFile(),libraryFolder().toLocalFile()+"/clip001.mp4");
        m_database = std::make_unique<veluno::Database>(m_directory.filePath("test.sqlite3"));
        m_settings = std::make_unique<veluno::AppSettings>(*m_database);
        m_database->setSetting("setup_complete","1");
        m_database->setSetting("online_consent","denied");
        m_library = std::make_unique<veluno::LibraryController>(*m_database,false,false);
    }
    void qmlEngineAvailable(QQmlEngine *engine) { engine->rootContext()->setContextProperty("library", m_library.get()); engine->rootContext()->setContextProperty("playback", m_player.get()); engine->rootContext()->setContextProperty("testInput", this); engine->rootContext()->setContextProperty("appSettings", m_settings.get()); engine->rootContext()->setContextProperty("screenshotDirectory", qEnvironmentVariable("VELUNO_TEST_SCREENSHOTS")); }
    void cleanupTestCase() { m_library.reset(); m_player.reset(); m_settings.reset(); m_database.reset(); }
};
QUICK_TEST_MAIN_WITH_SETUP(veluno, Setup)
#include "qml_main.moc"
