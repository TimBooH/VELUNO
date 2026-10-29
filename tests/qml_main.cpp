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
class Setup : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl videoFixture READ videoFixture CONSTANT)
    QTemporaryDir m_directory;
    std::unique_ptr<veluno::Database> m_database;
    std::unique_ptr<veluno::AppSettings> m_settings;
    std::unique_ptr<veluno::MpvEngine> m_player;
public:
    QUrl videoFixture() const { return QUrl::fromLocalFile(m_directory.filePath("ui-video.mp4")); }
    Q_INVOKABLE void pressKey(QWindow *window, int key) { QTest::keyClick(window, static_cast<Qt::Key>(key)); }
public slots:
    void applicationAvailable() {
        QQuickStyle::setStyle("Basic");
    std::setlocale(LC_NUMERIC, "C");
        qmlRegisterUncreatableType<veluno::MpvEngine>("Veluno.Player", 1, 0, "Playback", "Provided by the application");
        qmlRegisterType<veluno::VideoItem>("Veluno.Player", 1, 0, "VideoView");
        m_player = std::make_unique<veluno::MpvEngine>(nullptr, true);
        if (!createPlayerFixture(videoFixture().toLocalFile())) qFatal("UI test fixture generation failed");
        m_database = std::make_unique<veluno::Database>(m_directory.filePath("test.sqlite3"));
        m_settings = std::make_unique<veluno::AppSettings>(*m_database);
    }
    void qmlEngineAvailable(QQmlEngine *engine) { engine->rootContext()->setContextProperty("playback", m_player.get()); engine->rootContext()->setContextProperty("testInput", this); engine->rootContext()->setContextProperty("appSettings", m_settings.get()); engine->rootContext()->setContextProperty("screenshotDirectory", qEnvironmentVariable("VELUNO_TEST_SCREENSHOTS")); }
    void cleanupTestCase() { m_player.reset(); m_settings.reset(); m_database.reset(); }
};
QUICK_TEST_MAIN_WITH_SETUP(veluno, Setup)
#include "qml_main.moc"
