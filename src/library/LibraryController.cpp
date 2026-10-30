#include "LibraryController.h"
#include "platform/MediaLocation.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <algorithm>
namespace veluno {
LibraryController::LibraryController(Database &db, bool autoStart, bool notifications,
                                     QObject *parent)
    : QObject(parent), m_database(db), m_repository(db), m_worker(new ScannerWorker(db.path())),
      m_notifications(notifications) {
    m_autoStart = autoStart;
    for (auto *proxy : {&m_movies, &m_series, &m_recent, &m_randomMovies, &m_randomSeries}) {
        proxy->setSourceModel(&m_titles);
        proxy->setDynamicSortFilter(true);
        proxy->setFilterRole(Qt::UserRole + 1);
        proxy->setSortRole(Qt::UserRole + 1);
    }
    m_movies.kind = "movie";
    m_series.kind = "series";
    m_recent.order = "recent";
    m_randomMovies.kind = "movie";
    m_randomSeries.kind = "series";
    m_randomMovies.order = m_randomSeries.order = "random";
    m_randomMovies.seed = m_randomSeries.seed = QRandomGenerator::global()->generate();
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_worker, &ScannerWorker::initialize);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ScannerWorker::snapshotReady, this, &LibraryController::applySnapshot);
    connect(m_worker, &ScannerWorker::failed, this,
            [this](const QString &message) { report(message); });
    connect(m_worker, &ScannerWorker::ready, this, [this, autoStart] {
        m_ready = true;
        emit ready();
        if ((m_rescan || (autoStart && (m_setupComplete || m_wizardStep >= 4))) && sourceCount()) {
            m_rescan = false;
            scan();
        }
    });
    connect(m_worker, &ScannerWorker::progress, this,
            [this](int checked, int total, const QString &path) {
                m_checked = checked;
                m_total = total;
                m_currentPath = path;
                emit scanChanged();
            });
    connect(m_worker, &ScannerWorker::directoriesFound, this, &LibraryController::watchDirectories);
    connect(m_worker, &ScannerWorker::finished, this, [this](bool interrupted) {
        m_busy = false;
        m_paused = false;
        m_status =
            interrupted
                ? "Scan unterbrochen – bisherige Ergebnisse bleiben erhalten"
                : QString("Scan beendet · %1 Filme · %2 Serien · %3 nicht zugeordnet · %4 Fehler")
                      .arg(movieCount())
                      .arg(seriesCount())
                      .arg(unassignedCount())
                      .arg(errorCount());
        emit scanChanged();
        emit scanCompleted(interrupted);
        if (!interrupted && m_notifications)
            notifyCompletion();
        if (m_rescan && !m_closing) {
            m_rescan = false;
            QTimer::singleShot(0, this, [this] { scan(); });
        }
    });
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(1200);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        if (m_monitoring)
            m_debounce.start();
    });
    connect(&m_debounce, &QTimer::timeout, this, [this] { scan(); });
    m_poll.setInterval(60000);
    connect(&m_poll, &QTimer::timeout, this, [this] {
        if (m_monitoring && (m_setupComplete || m_wizardStep >= 4) && sourceCount() && !m_busy)
            scan();
    });
    m_watchTimer.setInterval(0);
    connect(&m_watchTimer, &QTimer::timeout, this, [this] {
        const auto batch = m_watchQueue.mid(0, 64);
        m_watchQueue.erase(m_watchQueue.begin(), m_watchQueue.begin() + batch.size());
        m_failedWatches += m_watcher.addPaths(batch).size();
        if (m_watchQueue.isEmpty()) {
            m_watchTimer.stop();
            m_watchNotice = m_failedWatches
                                ? QString("%1 Ordner konnten nicht direkt überwacht werden. "
                                          "Zusätzlich erfolgt alle 60 Sekunden eine Prüfung.")
                                      .arg(m_failedWatches)
                                : QString{};
            emit scanChanged();
        }
    });
    reload();
    m_thread.start();
    if (autoStart && m_monitoring)
        m_poll.start();
}
LibraryController::~LibraryController() {
    m_closing = true;
    m_poll.stop();
    m_debounce.stop();
    m_watchTimer.stop();
    m_worker->cancel();
    m_thread.quit();
    m_thread.wait();
}
void LibraryController::report(const QString &message) {
    m_error = message;
    emit errorChanged();
}
void LibraryController::clearError() {
    m_error.clear();
    emit errorChanged();
}
int LibraryController::sourceCount() const {
    int count = 0;
    for (const auto &value : m_sources.rows())
        if (value.toMap().value("enabled").toBool())
            ++count;
    return count;
}
void LibraryController::reload() {
    try {
        m_monitoring = m_database.setting("monitor_sources", "1") != "0";
        m_setupComplete = m_database.setting("setup_complete", "0") == "1";
        m_wizardStep = std::clamp(m_database.setting("wizard_step", "0").toInt(), 0, 5);
        m_consent = m_database.setting("online_consent", "undecided");
        if (m_consent != "granted" && m_consent != "denied") {
            m_consent = "undecided";
            if (m_setupComplete) {
                m_setupComplete = false;
                m_wizardStep = 3;
            }
        }
        m_profileName = m_repository.profileName();
        m_tileSize = std::clamp(m_repository.preference("tile_size", "170").toInt(), 130, 230);
        for (auto *proxy : {&m_movies, &m_series}) {
            proxy->order = m_repository.preference(proxy->kind + "_sort", "title");
            proxy->availability =
                m_repository.preference(proxy->kind + "_availability", "-1").toInt();
            proxy->refresh();
        }
        applySnapshot(m_repository.snapshot());
        emit setupChanged();
        emit optionsChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::applySnapshot(const QVariantMap &snapshot) {
    m_movieCount = 0;
    m_seriesCount = 0;
    for (const auto &value : snapshot.value("titles").toList()) {
        if (value.toMap().value("kind") == "movie")
            ++m_movieCount;
        else
            ++m_seriesCount;
    }
    m_titles.replace(snapshot.value("titles").toList());
    m_unassigned.replace(snapshot.value("unassigned").toList());
    m_sources.replace(snapshot.value("sources").toList());
    m_errors.replace(snapshot.value("errors").toList());
    for (auto *proxy : {&m_movies, &m_series, &m_recent, &m_randomMovies, &m_randomSeries})
        proxy->sort(0);
    if (!m_selection.isEmpty()) {
        const auto selectedId = m_selection.value("id").toLongLong();
        QVariantMap updated;
        for (const auto &row : m_titles.rows())
            if (row.toMap().value("id").toLongLong() == selectedId) {
                updated = row.toMap();
                break;
            }
        m_selection = updated;
        try {
            m_files.replace(m_repository.files(m_selection.value("id").toLongLong()));
        } catch (const std::exception &e) {
            report(QString::fromUtf8(e.what()));
        }
        emit selectionChanged();
    }
    emit catalogChanged();
}
bool LibraryController::addSource(const QUrl &url, const QString &kind) {
    if (!url.isLocalFile() && !isSmb(url.toString())) {
        report("Bitte einen lokalen Ordner oder eine SMB-Freigabe wählen.");
        return false;
    }
    try {
        m_repository.addSource(url.isLocalFile() ? url.toLocalFile()
                                                 : normalizedSmb(url).toString(QUrl::FullyEncoded),
                               kind);
        reload();
        if (m_setupComplete || m_wizardStep >= 4)
            scan();
        return true;
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
        return false;
    }
}
void LibraryController::removeSource(qint64 id) {
    if (m_busy) {
        report("Bitte den laufenden Scan zuerst abschließen lassen.");
        return;
    }
    try {
        m_repository.disableSource(id);
        m_watchTimer.stop();
        m_watchQueue.clear();
        const auto watched = m_watcher.directories();
        if (!watched.isEmpty())
            m_watcher.removePaths(watched);
        reload();
        if (sourceCount())
            scan();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::scan(qint64 sourceId) {
    if (m_closing || !sourceCount())
        return;
    if (!m_ready) {
        m_rescan = true;
        return;
    }
    if (m_busy) {
        m_rescan = true;
        return;
    }
    m_busy = true;
    m_paused = false;
    m_checked = 0;
    m_total = 0;
    m_status = "Bibliothek wird geprüft";
    m_worker->prepare();
    emit scanChanged();
    QMetaObject::invokeMethod(
        m_worker, [worker = m_worker, sourceId] { worker->scan(sourceId); }, Qt::QueuedConnection);
}
void LibraryController::togglePause() {
    if (!m_busy)
        return;
    m_paused = !m_paused;
    m_worker->setPaused(m_paused);
    m_status = m_paused ? "Scan pausiert" : "Bibliothek wird geprüft";
    emit scanChanged();
}
void LibraryController::setMonitoring(bool enabled) {
    if (enabled == m_monitoring)
        return;
    try {
        m_database.setSetting("monitor_sources", enabled ? "1" : "0");
        m_monitoring = enabled;
        if (enabled) {
            if (m_autoStart)
                m_poll.start();
            scan();
        } else {
            m_poll.stop();
            m_debounce.stop();
            m_watchTimer.stop();
            m_watchQueue.clear();
            const auto paths = m_watcher.directories();
            if (!paths.isEmpty())
                m_watcher.removePaths(paths);
            m_watchNotice.clear();
            emit scanChanged();
        }
        emit optionsChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::watchDirectories(const QStringList &paths) {
    if (!m_monitoring)
        return;
    QSet<QString> existing;
    for (const auto &path : m_watcher.directories())
        existing.insert(path);
    m_watchQueue.clear();
    m_failedWatches = 0;
    for (const auto &path : paths)
        if (!existing.contains(path)) {
            existing.insert(path);
            m_watchQueue.append(path);
        }
    if (!m_watchQueue.isEmpty())
        m_watchTimer.start();
}
void LibraryController::notifyCompletion() {
    if (!QDBusConnection::sessionBus().isConnected())
        return;
    QDBusInterface notifications("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                                 "org.freedesktop.Notifications", QDBusConnection::sessionBus());
    notifications.asyncCall("Notify", "VELUNO", uint(0), "veluno", "Bibliotheksscan beendet",
                            m_status, QStringList{}, QVariantMap{}, 5000);
}
void LibraryController::openDetails(qint64 id) {
    for (const auto &value : m_titles.rows())
        if (value.toMap().value("id").toLongLong() == id) {
            m_files.replace({});
            try {
                m_selection = value.toMap();
                m_files.replace(m_repository.files(id));
                emit selectionChanged();
                emit detailsRequested();
            } catch (const std::exception &e) {
                report(QString::fromUtf8(e.what()));
            }
            return;
        }
    m_selection.clear();
    m_files.replace({});
    emit selectionChanged();
    report("Dieser Bibliothekseintrag ist nicht mehr vorhanden.");
}
void LibraryController::playTitle(qint64 id) {
    openDetails(id);
    if (m_selection.value("id").toLongLong() != id)
        return;
    if (m_selection.value("kind") == "series")
        return;
    if (!m_files.rows().isEmpty())
        playFile(m_files.rows().first().toMap().value("path").toString());
}
void LibraryController::cancelPendingPlayback() {
    ++m_playRequest;
    if (m_networkBusy) {
        m_networkBusy = false;
        emit networkChanged();
    }
}
void LibraryController::playFile(const QString &path) {
    cancelPendingPlayback();
    clearError();
    qint64 titleId = 0;
    for (const auto &row : m_files.rows())
        if (row.toMap().value("path").toString() == path) {
            titleId = m_selection.value("id").toLongLong();
            break;
        }
    if (isSmb(path)) {
        const auto request = m_playRequest;
        m_networkBusy = true;
        emit networkChanged();
        NetworkAccess::resolve(QUrl(path), this,
                               [this, request, titleId, path](QString local, QString error) {
                                   if (request != m_playRequest)
                                       return;
                                   m_networkBusy = false;
                                   emit networkChanged();
                                   if (!error.isEmpty()) {
                                       report(error);
                                       return;
                                   }
                                   emit playbackStarting(path);
                                   emit playRequested(QUrl::fromLocalFile(local), true, titleId);
                               });
        return;
    }
    const QFileInfo file(path);
    if (!file.isFile() || !file.isReadable()) {
        report("Diese Version ist nicht verfügbar. Bitte Laufwerk prüfen oder eine andere Version "
               "wählen.");
        return;
    }
    emit playbackStarting(path);
    emit playRequested(QUrl::fromLocalFile(path), false, titleId);
}
void LibraryController::assignFile(qint64 id, const QString &title, int year, int season,
                                   int episode) {
    try {
        m_repository.assign(id, title, year, season, episode);
        reload();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
QUrl LibraryController::assetUrl(const QString &relative) const {
    if ((!relative.startsWith("images/") && !relative.startsWith("metadata/")) ||
        relative.split('/').contains("..") || relative.contains('\\'))
        return {};
    return QUrl::fromLocalFile(QFileInfo(m_database.path()).dir().filePath(relative));
}
QVariantList LibraryController::search(const QString &query) const {
    QVariantList result;
    const auto text = query.trimmed();
    if (text.isEmpty())
        return result;
    for (const auto &value : m_titles.rows()) {
        if (value.toMap().value("title").toString().contains(text, Qt::CaseInsensitive))
            result.append(value);
    }
    return result;
}
QString LibraryController::sortOrder(const QString &kind) const {
    return kind == "series" ? m_series.order : m_movies.order;
}
int LibraryController::availability(const QString &kind) const {
    return kind == "series" ? m_series.availability : m_movies.availability;
}
void LibraryController::setOptions(const QString &kind, const QString &order, int availability) {
    if ((kind != "movie" && kind != "series") ||
        !QStringList{"title", "recent", "year"}.contains(order) || availability < -1 ||
        availability > 1)
        return;
    try {
        m_repository.setPreference(kind + "_sort", order);
        m_repository.setPreference(kind + "_availability", QString::number(availability));
        reload();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::setTileSize(int size) {
    size = std::clamp(size, 130, 230);
    if (size == m_tileSize)
        return;
    try {
        m_repository.setPreference("tile_size", QString::number(size));
        m_tileSize = size;
        emit optionsChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::setWizardStep(int step) {
    try {
        m_wizardStep = std::clamp(step, 0, 5);
        m_database.setSetting("wizard_step", QString::number(m_wizardStep));
        emit setupChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::setOnlineConsent(const QString &consent) {
    if (consent != "granted" && consent != "denied")
        return;
    try {
        m_database.setSetting("online_consent", consent);
        m_consent = consent;
        emit setupChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::setProfileName(const QString &name) {
    try {
        m_repository.setProfileName(name);
        m_profileName = m_repository.profileName();
        emit setupChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void LibraryController::finishSetup() {
    if (m_consent != "denied" && m_consent != "granted") {
        report("Bitte bewusst entscheiden, ob Online-Metadaten grundsätzlich erlaubt sind.");
        return;
    }
    try {
        m_database.setSetting("setup_complete", "1");
        m_setupComplete = true;
        emit setupChanged();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
} // namespace veluno
