#include "PlayerFixture.h"
#include "player/MpvEngine.h"
#include <QTemporaryDir>
#include <QtTest>
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
        const auto before = fixtureHash(m_video);
        QVERIFY(!before.isEmpty());
        veluno::MpvEngine player(nullptr, true);
        player.openFile(QUrl::fromLocalFile(m_video));
        QVERIFY(player.active());
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        QTRY_VERIFY(player.duration() > 13);
        QTRY_VERIFY(!player.diagnostics().value("video-codec").toString().isEmpty());
        QTRY_VERIFY(!player.diagnostics().value("audio-codec-name").toString().isEmpty());
        QCOMPARE(player.diagnostics().value("rendering").toString(),
                 QString("Null-Ausgabe (Test)"));
        QTRY_VERIFY(player.position() > 0.15);
        player.togglePause();
        QTRY_VERIFY(player.paused());
        QTest::qWait(100);
        const double pausedAt = player.position();
        QTest::qWait(200);
        QVERIFY(std::abs(player.position() - pausedAt) < 0.15);
        player.seek(6);
        QTRY_VERIFY(player.position() > 5.8 && player.position() < 6.3);
        player.togglePause();
        QTRY_VERIFY(!player.paused());
        player.seekRelative(2);
        QTRY_VERIFY(player.position() > 7.8);
        player.setVolume(150);
        QTRY_COMPARE(player.volume(), 100.0);
        player.setVolume(-2);
        QTRY_COMPARE(player.volume(), 0.0);
        player.setSpeed(3);
        QTRY_COMPARE(player.speed(), 2.0);
        player.setSpeed(0.1);
        QTRY_COMPARE(player.speed(), 0.5);
        player.setMuted(true);
        QTRY_VERIFY(player.muted());
        player.stop();
        QVERIFY(!player.active());
        QCOMPARE(fixtureHash(m_video), before);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
    }
    void failuresAndRecovery() {
        veluno::MpvEngine player(nullptr, true);
        player.openFile(QUrl("https://example.invalid/video.mp4"));
        QVERIFY(!player.active());
        QVERIFY(!player.error().isEmpty());
        player.openFile(QUrl::fromLocalFile(m_directory.filePath("missing.mp4")));
        QVERIFY(!player.active());
        QVERIFY(!player.error().isEmpty());
        const auto corrupt = m_directory.filePath("broken.mp4");
        QFile f(corrupt);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not a movie");
        f.close();
        player.openFile(QUrl::fromLocalFile(corrupt));
        QTRY_VERIFY_WITH_TIMEOUT(!player.active(), 10000);
        QVERIFY(!player.error().isEmpty());
        player.openFile(QUrl::fromLocalFile(m_video));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QVERIFY(player.active());
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        player.stop();
    }
    void externalSubtitlesAndTrackSelection() {
        const auto subtitle = m_directory.filePath("external.srt");
        QFile file(subtitle);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("1\n00:00:00,100 --> 00:00:05,000\nVELUNO subtitle test\n");
        file.close();
        const auto before = fixtureHash(m_video);
        veluno::MpvEngine player(nullptr, true);
        player.openFile(QUrl::fromLocalFile(m_video));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_VERIFY(!player.audioTracks().isEmpty());
        player.selectAudio(player.audioTracks().first().toMap().value("id").toInt());
        player.loadSubtitle(QUrl::fromLocalFile(subtitle));
        QTRY_VERIFY_WITH_TIMEOUT(!player.subtitleTracks().isEmpty(), 5000);
        const auto selected = [&player] {
            for (const auto &v : player.subtitleTracks())
                if (v.toMap().value("selected").toBool())
                    return true;
            return false;
        };
        QTRY_VERIFY(selected());
        player.selectSubtitle(0);
        QTRY_VERIFY(!selected());
        player.openFile(QUrl::fromLocalFile(m_video));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_VERIFY(!selected());
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        player.stop();
        QCOMPARE(fixtureHash(m_video), before);
    }
    void germanAudioPreferenceResetsForNewFile() {
        const auto path = m_directory.filePath("multiple-languages.mkv");
        QProcess ffmpeg;
        ffmpeg.start("ffmpeg", {"-hide_banner",
                                "-loglevel",
                                "error",
                                "-y",
                                "-i",
                                m_video,
                                "-map",
                                "0:v:0",
                                "-map",
                                "0:a:0",
                                "-map",
                                "0:a:0",
                                "-c",
                                "copy",
                                "-metadata:s:a:0",
                                "language=eng",
                                "-metadata:s:a:1",
                                "language=deu",
                                "-disposition:a:0",
                                "default",
                                "-disposition:a:1",
                                "0",
                                path});
        QVERIFY(ffmpeg.waitForFinished(30000));
        QCOMPARE(ffmpeg.exitCode(), 0);
        const auto hash = fixtureHash(path);
        veluno::MpvEngine player(nullptr, true);
        const auto selectedLanguage = [&player] {
            for (const auto &track : player.audioTracks())
                if (track.toMap().value("selected").toBool())
                    return track.toMap().value("lang").toString();
            return QString{};
        };
        player.openFile(QUrl::fromLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_COMPARE(player.audioTracks().size(), 2);
        QTRY_COMPARE(selectedLanguage(), QString("deu"));
        int english = -1;
        for (const auto &track : player.audioTracks())
            if (track.toMap().value("lang") == "eng")
                english = track.toMap().value("id").toInt();
        QVERIFY(english > 0);
        player.selectAudio(english);
        QTRY_COMPARE(selectedLanguage(), QString("eng"));
        player.openFile(QUrl::fromLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_COMPARE(selectedLanguage(), QString("deu"));
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        player.stop();
        QCOMPARE(fixtureHash(path), hash);
    }
    void repeatedOpenRetainsMediaInformation() {
        veluno::MpvEngine player(nullptr, true);
        for (int attempt = 0; attempt < 12; ++attempt) {
            player.openFile(QUrl::fromLocalFile(m_video));
            QTest::qSleep(150); // Let mpv coalesce notifications while the UI is busy.
            QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
            QTRY_VERIFY_WITH_TIMEOUT(player.duration() > 13, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!player.audioTracks().isEmpty(), 3000);
        }
        player.stop();
    }
    void repeatedOpenAndStop() {
        veluno::MpvEngine player(nullptr, true);
        for (int i = 0; i < 3; ++i) {
            player.openFile(QUrl::fromLocalFile(m_video));
            QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
            QVERIFY(player.active());
            player.stop();
            QTest::qWait(80);
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
