#include "VideoItem.h"
#include <mpv/render.h>
#include <mpv/client.h>
#include <QPainter>
#include <QQuickWindow>
#include <QMutexLocker>
namespace veluno {
VideoWorker::VideoWorker(std::shared_ptr<MpvSession> session, std::shared_ptr<FrameMailbox> mailbox) : m_session(std::move(session)), m_mailbox(std::move(mailbox)) {}
void VideoWorker::initialize() {
    mpv_render_param params[] = {{MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_SW)}, {MPV_RENDER_PARAM_INVALID, nullptr}};
    const int result = mpv_render_context_create(&m_context, m_session->handle, params);
    if (result < 0) { emit failed(QString("Videoausgabe konnte nicht initialisiert werden: %1").arg(QString::fromUtf8(mpv_error_string(result)))); return; }
    mpv_render_context_set_update_callback(m_context, [](void *worker) { static_cast<VideoWorker *>(worker)->requestFrame(); }, this);
    emit initialized();
}
void VideoWorker::requestFrame() {
    if (!m_scheduled.exchange(true)) QMetaObject::invokeMethod(this, [this] { m_scheduled = false; renderFrame(); }, Qt::QueuedConnection);
}
void VideoWorker::resize(const QSize &size) {
    if (size == m_size) return;
    m_size = size; requestFrame();
}
void VideoWorker::renderFrame() {
    if (!m_context || m_size.isEmpty()) return;
    mpv_render_context_update(m_context);
    QImage image(m_size, QImage::Format_RGB32);
    if (image.isNull()) { emit failed("Bildpuffer konnte nicht erstellt werden. Prüfe den verfügbaren Arbeitsspeicher."); return; }
    image.fill(Qt::black);
    int dimensions[] = {image.width(), image.height()};
    size_t stride = image.bytesPerLine();
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    const char *format = "bgr0";
#else
    const char *format = "0rgb";
#endif
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_SW_SIZE, dimensions}, {MPV_RENDER_PARAM_SW_FORMAT, const_cast<char *>(format)},
        {MPV_RENDER_PARAM_SW_STRIDE, &stride}, {MPV_RENDER_PARAM_SW_POINTER, image.bits()}, {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    const int result = mpv_render_context_render(m_context, params);
    if (result < 0) { emit failed(QString("Videobild konnte nicht gerendert werden: %1").arg(QString::fromUtf8(mpv_error_string(result)))); return; }
    // mpv's bgr0/0rgb padding is not an alpha channel. Qt's RGB32 contract
    // requires 0xffRRGGBB, including when the image reaches a GPU texture.
    // Leaving padding at zero causes blending/overbright output on that path.
    for (int y = 0; y < image.height(); ++y) {
        auto *pixels = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x)
            pixels[x] |= 0xff000000u;
    }
    {
        QMutexLocker lock(&m_mailbox->mutex);
        m_mailbox->latest = std::move(image);
    }
    // At most one queued GUI notification; old frames never accumulate.
    if (!m_mailbox->notificationPending.exchange(true)) emit frameAvailable();
}
void VideoWorker::shutdown() {
    if (m_context) {
        mpv_render_context_set_update_callback(m_context, nullptr, nullptr);
        mpv_render_context_free(m_context); m_context = nullptr;
    }
}
VideoItem::VideoItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setOpaquePainting(true);
    connect(this, &QQuickItem::windowChanged, this, [this] { resizeWorker(); });
}
VideoItem::~VideoItem() { stopWorker(); }
void VideoItem::stopWorker() {
    if (!m_worker) return;
    QMetaObject::invokeMethod(m_worker, [worker = m_worker] { worker->shutdown(); }, Qt::BlockingQueuedConnection);
    m_thread.quit(); m_thread.wait(); m_worker = nullptr;
    if (m_player) m_player->rendererReleased();
}
void VideoItem::setPlayer(MpvEngine *player) {
    if (m_player == player) return;
    stopWorker(); m_player = player;
    if (player && player->requiresRenderer()) {
        m_mailbox = std::make_shared<FrameMailbox>();
        m_worker = new VideoWorker(player->session(), m_mailbox);
        m_worker->moveToThread(&m_thread);
        connect(&m_thread, &QThread::started, m_worker, &VideoWorker::initialize);
        connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
        connect(m_worker, &VideoWorker::initialized, player, &MpvEngine::rendererInitialized, Qt::QueuedConnection);
        connect(m_worker, &VideoWorker::failed, player, &MpvEngine::rendererFailed, Qt::QueuedConnection);
        connect(m_worker, &VideoWorker::frameAvailable, this, [this, mailbox = m_mailbox] {
            if (mailbox != m_mailbox || !m_worker) return;
            m_mailbox->notificationPending = false;
            {
                QMutexLocker lock(&m_mailbox->mutex);
                m_frame = m_mailbox->latest;
            }
            resizeWorker(); update();
        }, Qt::QueuedConnection);
        m_thread.start(); resizeWorker();
    }
    emit playerChanged(); update();
}
void VideoItem::resizeWorker() {
    if (!m_worker || width() <= 0 || height() <= 0) return;
    const auto scale = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const QSize size(qMax(1, qRound(width()*scale)), qMax(1, qRound(height()*scale)));
    if (size == m_requestedSize) return;
    m_requestedSize = size;
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, size] { worker->resize(size); }, Qt::QueuedConnection);
}
void VideoItem::geometryChange(const QRectF &geometry, const QRectF &oldGeometry) {
    QQuickPaintedItem::geometryChange(geometry, oldGeometry);
    if (geometry.size() != oldGeometry.size()) resizeWorker();
}
void VideoItem::paint(QPainter *painter) {
    painter->fillRect(boundingRect(), Qt::black);
    if (!m_frame.isNull()) painter->drawImage(boundingRect(), m_frame);
}
}
