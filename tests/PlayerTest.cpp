#include <QtTest>
#include <QTemporaryDir>
#include "player/MpvEngine.h"
#include "PlayerFixture.h"
class PlayerTest : public QObject {
    Q_OBJECT
    QTemporaryDir m_directory;
    QString m_video;
private slots:
    void initTestCase() {
        m_video = m_directory.filePath(QString::fromUtf8("Film mit Leerzeichen – Ä.mp4"));
        QVERIFY(createPlayerFixture(m_video));
    }
    void playbackControlsAndSourceIntegrity() {
        const auto before = fixtureHash(m_video); QVERIFY(!before.isEmpty());
        veluno::MpvEngine player(nullptr, true);
        player.openFile(QUrl::fromLocalFile(m_video));
        QVERIFY(player.active());
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        QTRY_VERIFY(player.duration() > 13);
        QTRY_VERIFY(player.position() > 0.15);
        player.togglePause(); QTRY_VERIFY(player.paused());
        QTest::qWait(100);
        const double pausedAt = player.position(); QTest::qWait(200);
        QVERIFY(std::abs(player.position() - pausedAt) < 0.15);
        player.seek(6); QTRY_VERIFY(player.position() > 5.8 && player.position() < 6.3);
        player.togglePause(); QTRY_VERIFY(!player.paused());
        player.seekRelative(2); QTRY_VERIFY(player.position() > 7.8);
        player.setVolume(150); QTRY_COMPARE(player.volume(), 100.0);
        player.setVolume(-2); QTRY_COMPARE(player.volume(), 0.0);
        player.setSpeed(3); QTRY_COMPARE(player.speed(), 2.0);
        player.setSpeed(0.1); QTRY_COMPARE(player.speed(), 0.5);
        player.setMuted(true); QTRY_VERIFY(player.muted());
        player.stop(); QVERIFY(!player.active());
        QCOMPARE(fixtureHash(m_video), before);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
    }
    void failuresAndRecovery() {
        veluno::MpvEngine player(nullptr, true);
        player.openFile(QUrl("https://example.invalid/video.mp4"));
        QVERIFY(!player.active()); QVERIFY(!player.error().isEmpty());
        player.openFile(QUrl::fromLocalFile(m_directory.filePath("missing.mp4")));
        QVERIFY(!player.active()); QVERIFY(!player.error().isEmpty());
        const auto corrupt = m_directory.filePath("broken.mp4");
        QFile f(corrupt); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("not a movie"); f.close();
        player.openFile(QUrl::fromLocalFile(corrupt));
        QTRY_VERIFY_WITH_TIMEOUT(!player.active(), 10000);
        QVERIFY(!player.error().isEmpty());
        player.openFile(QUrl::fromLocalFile(m_video));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QVERIFY(player.active()); QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        player.stop();
    }
    void repeatedOpenAndStop() {
        veluno::MpvEngine player(nullptr, true);
        for (int i = 0; i < 3; ++i) {
            player.openFile(QUrl::fromLocalFile(m_video));
            QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
            QVERIFY(player.active());
            player.stop(); QTest::qWait(80);
            QVERIFY(!player.active());
        }
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
    }
    void endAndReplay() {
        veluno::MpvEngine player(nullptr, true);
        player.openFile(QUrl::fromLocalFile(m_video));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_VERIFY(player.duration() > 13);
        player.seek(13.7);
        QTRY_VERIFY_WITH_TIMEOUT(player.ended(), 5000);
        QTRY_VERIFY(player.paused());
        player.togglePause();
        QTRY_VERIFY(!player.paused());
        QTRY_VERIFY(player.position() < 2);
        player.stop();
    }
};
QTEST_GUILESS_MAIN(PlayerTest)
#include "PlayerTest.moc"
