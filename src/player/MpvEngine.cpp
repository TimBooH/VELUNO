#include "MpvEngine.h"
#include <mpv/client.h>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace veluno {
MpvSession::MpvSession(bool nullOutput) {
    handle = mpv_create();
    if (!handle) throw std::runtime_error("libmpv konnte nicht erstellt werden.");
    try {
        const auto option = [this](const char *name, const char *value) {
            const int result = mpv_set_option_string(handle, name, value);
            if (result < 0) throw std::runtime_error(std::string(name) + ": " + mpv_error_string(result));
        };
        option("config", "no");
        option("load-scripts", "no");
        option("terminal", "no");
        option("input-default-bindings", "no");
        option("input-vo-keyboard", "no");
        option("osc", "no");
        option("ytdl", "no");
        option("idle", "yes");
        option("keep-open", "yes");
        option("vo", nullOutput ? "null" : "libmpv");
        if (nullOutput) option("ao", "null");
        option("hwdec", nullOutput ? "no" : "auto-safe");
        option("alang", "de,deu,ger");
        option("sid", "no");
        option("volume", "70");
        const int result = mpv_initialize(handle);
        if (result < 0) throw std::runtime_error(mpv_error_string(result));
        mpv_request_log_messages(handle, "warn");
    } catch (...) { mpv_terminate_destroy(handle); handle = nullptr; throw; }
}
MpvSession::~MpvSession() { if (handle) mpv_terminate_destroy(handle); }
MpvEngine::MpvEngine(QObject *parent, bool nullOutput) : QObject(parent), m_session(std::make_shared<MpvSession>(nullOutput)), m_nullOutput(nullOutput), m_rendererReady(nullOutput) {
    for (const char *name : {"time-pos", "duration", "volume", "speed"})
        mpv_observe_property(m_session->handle, 0, name, MPV_FORMAT_DOUBLE);
    for (const char *name : {"pause", "mute", "eof-reached"})
        mpv_observe_property(m_session->handle, 0, name, MPV_FORMAT_FLAG);
    // No callbacks point into a dying QObject: drain events without blocking.
    m_events.setInterval(30);
    connect(&m_events, &QTimer::timeout, this, &MpvEngine::pollEvents);
    m_events.start();
}
MpvEngine::~MpvEngine() {
    m_events.stop();
    const char *arguments[] = {"stop", nullptr};
    mpv_command_async(m_session->handle, 0, arguments);
}
unsigned long MpvEngine::apiVersion() { return mpv_client_api_version(); }
bool MpvEngine::command(const QList<QByteArray> &arguments) {
    QList<const char *> values;
    for (const auto &argument : arguments) values.push_back(argument.constData());
    values.push_back(nullptr);
    const int result = mpv_command_async(m_session->handle, 0, values.data());
    if (result < 0) { fail(QString::fromUtf8(mpv_error_string(result))); return false; }
    return true;
}
bool MpvEngine::setFlag(const char *name, bool value) {
    int flag = value;
    const int result = mpv_set_property_async(m_session->handle, 0, name, MPV_FORMAT_FLAG, &flag);
    if (result < 0) { fail(QString::fromUtf8(mpv_error_string(result))); return false; }
    return true;
}
bool MpvEngine::setNumber(const char *name, double value) {
    const int result = mpv_set_property_async(m_session->handle, 0, name, MPV_FORMAT_DOUBLE, &value);
    if (result < 0) { fail(QString::fromUtf8(mpv_error_string(result))); return false; }
    return true;
}
void MpvEngine::openFile(const QUrl &url) {
    if (!m_renderFailure.isEmpty()) { fail(m_renderFailure); return; }
    if (!url.isLocalFile()) { fail("Bitte eine lokale Videodatei auswählen."); return; }
    const QFileInfo file(url.toLocalFile());
    if (!file.exists() || !file.isFile() || !file.isReadable()) {
        fail("Datei nicht lesbar. Prüfe Dateipfad, Zugriffsrechte und angeschlossene Laufwerke."); return;
    }
    m_pending = file.absoluteFilePath();
    m_title = file.fileName();
    m_error.clear(); emit errorChanged();
    m_active = true; m_loading = true; m_ended = false; m_paused = false;
    m_position = 0; m_duration = 0;
    emit positionChanged(); emit durationChanged(); emit stateChanged();
    if (m_rendererReady) loadPending();
}
void MpvEngine::loadPending() {
    if (m_pending.isEmpty()) return;
    // Native absolute path (no URL decoding by mpv). Original file is read-only.
    const auto path = m_pending.toUtf8(); m_pending.clear();
    command({"set", "sid", "no"});
    setFlag("pause", false);
    command({"loadfile", path, "replace"});
}
void MpvEngine::rendererInitialized() { m_renderFailure.clear(); m_rendererReady = true; loadPending(); }
void MpvEngine::rendererReleased() {
    m_rendererReady = m_nullOutput;
    if (m_active) fail("Videoausgabe wurde unterbrochen. Bitte die Datei erneut öffnen.");
}
void MpvEngine::rendererFailed(const QString &message) { m_rendererReady = false; m_renderFailure = message; fail(message); }
void MpvEngine::togglePause() {
    if (!m_active || m_loading) return;
    if (m_ended) { seek(0); m_ended = false; }
    if (setFlag("pause", !m_paused)) { m_paused = !m_paused; emit stateChanged(); }
}
void MpvEngine::seek(double seconds) {
    if (!m_active || m_duration <= 0 || !std::isfinite(seconds)) return;
    command({"seek", QByteArray::number(std::clamp(seconds, 0.0, m_duration), 'f', 3), "absolute+exact"});
}
void MpvEngine::seekRelative(double seconds) {
    if (m_active && !m_loading && std::isfinite(seconds))
        command({"seek", QByteArray::number(seconds, 'f', 3), "relative+exact"});
}
void MpvEngine::setVolume(double volume) {
    if (!std::isfinite(volume)) return;
    volume = std::clamp(volume, 0.0, 100.0);
    if (setNumber("volume", volume)) { m_volume = volume; emit volumeChanged(); }
}
void MpvEngine::setMuted(bool muted) {
    if (setFlag("mute", muted)) { m_muted = muted; emit stateChanged(); }
}
void MpvEngine::setSpeed(double speed) {
    if (!std::isfinite(speed)) return;
    speed = std::clamp(speed, 0.5, 2.0);
    if (setNumber("speed", speed)) { m_speed = speed; emit speedChanged(); }
}
void MpvEngine::cycleAudio() { if (m_active) command({"cycle", "audio"}); }
void MpvEngine::cycleSubtitles() { if (m_active) command({"cycle", "sub"}); }
void MpvEngine::stop() {
    m_pending.clear(); command({"stop"});
    m_active = false; m_loading = false; m_paused = false; m_ended = false;
    m_position = 0; m_duration = 0;
    emit positionChanged(); emit durationChanged(); emit stateChanged();
}
void MpvEngine::fail(const QString &message) {
    m_pending.clear();
    const char *stopArguments[] = {"stop", nullptr};
    mpv_command_async(m_session->handle, 0, stopArguments);
    m_active = false; m_loading = false;
    m_error = message;
    emit errorChanged(); emit stateChanged();
}
void MpvEngine::pollEvents() {
    for (int i = 0; i < 100; ++i) {
        const auto *event = mpv_wait_event(m_session->handle, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
            const auto *log = static_cast<mpv_event_log_message *>(event->data);
            qWarning().noquote() << "libmpv:" << log->prefix << QString::fromUtf8(log->text).trimmed();
        } else if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            const auto *property = static_cast<mpv_event_property *>(event->data);
            if (!property->data) continue;
            const QByteArray name(property->name);
            if (property->format == MPV_FORMAT_DOUBLE) {
                const double value = *static_cast<double *>(property->data);
                if (!std::isfinite(value)) continue;
                if (name == "time-pos") { m_position = value; emit positionChanged(); }
                else if (name == "duration") { m_duration = value; emit durationChanged(); }
                else if (name == "volume") { m_volume = value; emit volumeChanged(); }
                else if (name == "speed") { m_speed = value; emit speedChanged(); }
            } else if (property->format == MPV_FORMAT_FLAG) {
                const bool value = *static_cast<int *>(property->data);
                if (name == "pause") m_paused = value;
                else if (name == "mute") m_muted = value;
                else if (name == "eof-reached") m_ended = value;
                emit stateChanged();
            }
        } else if (event->event_id == MPV_EVENT_FILE_LOADED) {
            m_loading = false; emit stateChanged();
        } else if (event->event_id == MPV_EVENT_END_FILE) {
            const auto *end = static_cast<mpv_event_end_file *>(event->data);
            if (end->reason == MPV_END_FILE_REASON_ERROR) {
                fail(QString("Wiedergabe fehlgeschlagen: %1. Prüfe die Datei und verfügbare Codecs.").arg(QString::fromUtf8(mpv_error_string(end->error))));
            }
        } else if ((event->event_id == MPV_EVENT_COMMAND_REPLY || event->event_id == MPV_EVENT_SET_PROPERTY_REPLY) && event->error < 0) {
            fail(QString("Player-Befehl fehlgeschlagen: %1").arg(QString::fromUtf8(mpv_error_string(event->error))));
        }
    }
}
}
