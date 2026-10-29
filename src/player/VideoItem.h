#pragma once
#include <QQuickPaintedItem>
#include <QThread>
#include <QMutex>
#include <QImage>
#include <QPointer>
#include <atomic>
#include "MpvEngine.h"
struct mpv_render_context;
namespace veluno {
struct FrameMailbox {
    QMutex mutex;
    QImage latest;
    std::atomic_bool notificationPending{false};
};
// Render API calls live on this worker. No decoding/scaling runs on the UI thread.
class VideoWorker final : public QObject {
    Q_OBJECT
public:
    VideoWorker(std::shared_ptr<MpvSession> session, std::shared_ptr<FrameMailbox> mailbox);
    void initialize();
    void resize(const QSize &size);
    void shutdown();
signals:
    void initialized();
    void failed(const QString &message);
    void frameAvailable();
private:
    void requestFrame();
    void renderFrame();
    std::shared_ptr<MpvSession> m_session;
    std::shared_ptr<FrameMailbox> m_mailbox;
    mpv_render_context *m_context = nullptr;
    QSize m_size{640, 360};
    std::atomic_bool m_scheduled{false};
};
class VideoItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(veluno::MpvEngine *player READ player WRITE setPlayer NOTIFY playerChanged)
public:
    explicit VideoItem(QQuickItem *parent = nullptr);
    ~VideoItem() override;
    MpvEngine *player() const { return m_player; }
    void setPlayer(MpvEngine *player);
    void paint(QPainter *painter) override;
signals:
    void playerChanged();
protected:
    void geometryChange(const QRectF &geometry, const QRectF &oldGeometry) override;
private:
    void resizeWorker();
    void stopWorker();
    QPointer<MpvEngine> m_player;
    QThread m_thread;
    VideoWorker *m_worker = nullptr;
    std::shared_ptr<FrameMailbox> m_mailbox;
    QImage m_frame;
    QSize m_requestedSize;
};
}
