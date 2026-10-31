#include <clocale>
#include <QtTest>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QSGRendererInterface>
#include <QImage>
#include <QScopeGuard>
#include <QPainter>
#include "player/MpvEngine.h"
#include "player/VideoItem.h"
#include "PlayerFixture.h"
class PlayerRenderTest : public QObject {
    Q_OBJECT
private slots:
    void softwareFramesHaveOpaqueStorage() {
        QTemporaryDir directory;
        const auto video = directory.filePath("opaque.mp4");
        QVERIFY(createPlayerFixture(video, false));
        veluno::MpvEngine player;
        auto mailbox = std::make_shared<veluno::FrameMailbox>();
        veluno::VideoWorker worker(player.session(), mailbox);
        const auto cleanup = qScopeGuard([&] {
            worker.shutdown();
            player.rendererReleased();
        });
        connect(&worker, &veluno::VideoWorker::initialized, &player,
                &veluno::MpvEngine::rendererInitialized);
        connect(&worker, &veluno::VideoWorker::failed, &player,
                &veluno::MpvEngine::rendererFailed);
        worker.initialize();
        worker.resize(QSize(320, 240));
        player.openFile(QUrl::fromLocalFile(video));
        QTRY_VERIFY_WITH_TIMEOUT(player.position() > 0.1 || !player.error().isEmpty(), 15000);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        QTRY_VERIFY(!mailbox->latest.isNull());
        const auto frame = mailbox->latest;
        QCOMPARE(frame.format(), QImage::Format_RGB32);
        // pixelColor() masks the unused byte for RGB32; inspect the stored pixels.
        // Qt's painter/texture path must receive opaque words, including letterboxes.
        for (int y = 0; y < frame.height(); ++y) {
            const auto *pixels = reinterpret_cast<const QRgb *>(frame.constScanLine(y));
            for (int x = 0; x < frame.width(); ++x)
                QVERIFY2(qAlpha(pixels[x]) == 255, "mpv padding reached Qt as non-opaque alpha");
        }
    }
    void neutralVideoDoesNotBlendWithWindow() {
        QTemporaryDir directory;
        const auto video = directory.filePath("neutral.mp4");
        QProcess ffmpeg;
        ffmpeg.start("ffmpeg", {"-hide_banner", "-loglevel", "error", "-y", "-f", "lavfi",
                               "-i", "color=c=0x808080:s=160x90:r=20:d=14", "-an",
                               "-c:v", "mpeg4", "-q:v", "2", video});
        QVERIFY(ffmpeg.waitForFinished(30000));
        QCOMPARE(ffmpeg.exitCode(), 0);
        veluno::MpvEngine player;
        QQuickWindow window;
        window.setColor(QColor("#D020E0"));
        window.resize(480, 320);
        auto *item = new veluno::VideoItem(window.contentItem());
        item->setPlayer(&player);
        item->setSize(QSizeF(480, 320));
        connect(&window, &QWindow::widthChanged, item, [item](int width) { item->setWidth(width); });
        connect(&window, &QWindow::heightChanged, item, [item](int height) { item->setHeight(height); });
        window.show();
        player.openFile(QUrl::fromLocalFile(video));
        QTRY_VERIFY_WITH_TIMEOUT(player.position() > 0.1 || !player.error().isEmpty(), 15000);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        const auto neutral = [&] {
            const auto frame = window.grabWindow();
            if (frame.isNull()) return false;
            const auto pixel = frame.pixelColor(frame.width() / 2, frame.height() / 2);
            return qAbs(pixel.red() - 128) < 12 && qAbs(pixel.green() - 128) < 12 &&
                   qAbs(pixel.blue() - 128) < 12;
        };
        const auto opaquePaint = [&] {
            QImage layer(item->size().toSize(), QImage::Format_ARGB32_Premultiplied);
            layer.fill(Qt::transparent);
            QPainter painter(&layer);
            item->paint(&painter);
            painter.end();
            const auto pixel = layer.pixel(layer.width() / 2, layer.height() / 2);
            return qAlpha(pixel) == 255 && qAbs(qRed(pixel) - 128) < 12 &&
                   qAbs(qGreen(pixel) - 128) < 12 && qAbs(qBlue(pixel) - 128) < 12;
        };
        QTRY_VERIFY_WITH_TIMEOUT(neutral(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(opaquePaint(), 5000);
        player.togglePause();
        QTRY_VERIFY(player.paused());
        window.setColor(QColor("#20E030"));
        QTRY_VERIFY(neutral());
        item->setVisible(false);
        QTRY_VERIFY(window.grabWindow().pixelColor(240, 160).green() > 200);
        item->setVisible(true);
        QTRY_VERIFY(neutral());
        player.stop();
        window.close();
    }
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
