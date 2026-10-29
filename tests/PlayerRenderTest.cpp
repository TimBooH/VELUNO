#include <clocale>
#include <QtTest>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QSGRendererInterface>
#include <QImage>
#include "player/MpvEngine.h"
#include "player/VideoItem.h"
#include "PlayerFixture.h"
class PlayerRenderTest : public QObject {
    Q_OBJECT
private slots:
    void decodedPixelsResizeAndShutdown() {
        QTemporaryDir directory;
        const auto video = directory.filePath("render.mp4");
        QVERIFY(createPlayerFixture(video, false));
        const auto before = fixtureHash(video);
        veluno::MpvEngine player;
        {
            QQuickWindow window;
            window.resize(480, 320);
            auto *item = new veluno::VideoItem(window.contentItem());
            item->setPlayer(&player);
            item->setSize(QSizeF(window.width(), window.height()));
            connect(&window, &QWindow::widthChanged, item, [item](int width) { item->setWidth(width); });
            connect(&window, &QWindow::heightChanged, item, [item](int height) { item->setHeight(height); });
            window.show();
            player.openFile(QUrl::fromLocalFile(video));
            QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 15000);
            QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
            QTRY_VERIFY(player.position() > 0.1);
            const auto hasExpectedFrame = [&] {
                const auto frame = window.grabWindow();
                if (frame.isNull()) return false;
                const auto ratio = window.devicePixelRatio();
                const int videoWidth = qRound(item->width()*ratio);
                const int videoHeight = qRound(item->height()*ratio);
                int correct = 0, total = 0;
                for (int y = videoHeight/4; y < videoHeight*2/5; ++y) {
                    for (int x = videoWidth/10; x < videoWidth*9/10; ++x) {
                        const auto red = frame.pixelColor(x, y);
                        const auto blue = frame.pixelColor(x, videoHeight-1-y);
                        correct += red.red() > 150 && red.blue() < 80 && blue.blue() > 150 && blue.red() < 80;
                        ++total;
                    }
                }
                return total > 0 && correct > total * 0.98;
            };
            QTest::qWait(200);
            if (const auto dir = qEnvironmentVariable("VELUNO_TEST_SCREENSHOTS"); !dir.isEmpty()) window.grabWindow().save(dir + "/video-first-frame.png");
            QTRY_VERIFY_WITH_TIMEOUT(hasExpectedFrame() || !player.error().isEmpty(), 5000);
            QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
            QVERIFY(hasExpectedFrame());
            player.togglePause(); QTRY_VERIFY(player.paused());
            window.resize(640, 400);
            item->setSize(QSizeF(window.width()*0.75, window.height()*0.75));
            QTRY_VERIFY_WITH_TIMEOUT(hasExpectedFrame(), 10000);
            const auto screenshot = qEnvironmentVariable("VELUNO_TEST_SCREENSHOTS");
            if (!screenshot.isEmpty()) QVERIFY(window.grabWindow().save(screenshot + "/video-render.png"));
            // Destroy a still active scene graph before releasing the mpv handle.
            window.close();
        }
        player.stop();
        QCOMPARE(fixtureHash(video), before);
    }
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    std::setlocale(LC_NUMERIC, "C");
    PlayerRenderTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "PlayerRenderTest.moc"
