#pragma once
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <memory>
struct mpv_handle;
namespace veluno {
// Lives as long as both controller and render-thread adapter need the handle.
struct MpvSession final {
    explicit MpvSession(bool nullOutput);
    ~MpvSession();
    mpv_handle *handle = nullptr;
};
class MpvEngine final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList audioDevices READ audioDevices NOTIFY mediaInfoChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY mediaInfoChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY mediaInfoChanged)
    Q_PROPERTY(double creditsStart READ creditsStart NOTIFY mediaInfoChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(bool ended READ ended NOTIFY stateChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY stateChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  public:
    explicit MpvEngine(QObject *parent = nullptr, bool nullOutput = false);
    ~MpvEngine() override;
    static unsigned long apiVersion();
    QVariantList audioDevices() const {
        return m_audioDevices;
    }
    QVariantList audioTracks() const {
        return m_audioTracks;
    }
    QVariantList subtitleTracks() const {
        return m_subtitleTracks;
    }
    double creditsStart() const {
        return m_creditsStart;
    }
    void configure(const QVariantMap &options);
    Q_INVOKABLE void selectAudio(int id);
    Q_INVOKABLE void selectSubtitle(int id);
    Q_INVOKABLE void loadSubtitle(const QUrl &url);
    bool requiresRenderer() const {
        return !m_nullOutput;
    }
    bool active() const {
        return m_active;
    }
    bool loading() const {
        return m_loading;
    }
    bool paused() const {
        return m_paused;
    }
    bool ended() const {
        return m_ended;
    }
    double position() const {
        return m_position;
    }
    double duration() const {
        return m_duration;
    }
    double volume() const {
        return m_volume;
    }
    bool muted() const {
        return m_muted;
    }
    double speed() const {
        return m_speed;
    }
    QString title() const {
        return m_title;
    }
    QString error() const {
        return m_error;
    }
    void setVolume(double volume);
    void setMuted(bool muted);
    void setSpeed(double speed);
    Q_INVOKABLE void openFile(const QUrl &url, bool resolvedNetworkFile = false);
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void seek(double seconds);
    Q_INVOKABLE void seekRelative(double seconds);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void cycleAudio();
    Q_INVOKABLE void cycleSubtitles();
    std::shared_ptr<MpvSession> session() const {
        return m_session;
    }
  public slots:
    void rendererInitialized();
    void rendererReleased();
    void rendererFailed(const QString &message);
  signals:
    void aboutToStop();
    void mediaInfoChanged();
    void stateChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void speedChanged();
    void errorChanged();

  private:
    QVariantList m_audioDevices, m_audioTracks, m_subtitleTracks;
    double m_creditsStart = -1;
    void pollEvents();
    void loadPending();
    bool command(const QList<QByteArray> &arguments);
    bool setFlag(const char *name, bool value);
    bool setNumber(const char *name, double value);
    void fail(const QString &message);
    std::shared_ptr<MpvSession> m_session;
    QTimer m_events;
    QString m_pending;
    QString m_title;
    QString m_error;
    QString m_renderFailure;
    bool m_nullOutput = false;
    bool m_rendererReady = false;
    bool m_active = false;
    bool m_loading = false;
    bool m_paused = false;
    bool m_ended = false;
    bool m_muted = false;
    double m_position = 0;
    double m_duration = 0;
    double m_volume = 70;
    double m_speed = 1;
};
} // namespace veluno
