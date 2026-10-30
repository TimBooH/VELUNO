#pragma once
#include "LibraryModels.h"
#include "database/LibraryRepository.h"
#include "scanner/Scanner.h"
#include <QThread>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QUrl>
namespace veluno {
class LibraryController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *movies READ movies CONSTANT)
    Q_PROPERTY(QAbstractItemModel *series READ series CONSTANT)
    Q_PROPERTY(QAbstractItemModel *recent READ recent CONSTANT)
    Q_PROPERTY(QAbstractItemModel *randomMovies READ randomMovies CONSTANT)
    Q_PROPERTY(QAbstractItemModel *randomSeries READ randomSeries CONSTANT)
    Q_PROPERTY(QAbstractItemModel *unassigned READ unassigned CONSTANT)
    Q_PROPERTY(QAbstractItemModel *sources READ sources CONSTANT)
    Q_PROPERTY(QAbstractItemModel *errors READ errors CONSTANT)
    Q_PROPERTY(QAbstractItemModel *selectedFiles READ selectedFiles CONSTANT)
    Q_PROPERTY(QVariantMap selectedTitle READ selectedTitle NOTIFY selectionChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY scanChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY scanChanged)
    Q_PROPERTY(int checked READ checked NOTIFY scanChanged)
    Q_PROPERTY(int total READ total NOTIFY scanChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY scanChanged)
    Q_PROPERTY(QString status READ status NOTIFY scanChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString watchNotice READ watchNotice NOTIFY scanChanged)
    Q_PROPERTY(int movieCount READ movieCount NOTIFY catalogChanged)
    Q_PROPERTY(int seriesCount READ seriesCount NOTIFY catalogChanged)
    Q_PROPERTY(int unassignedCount READ unassignedCount NOTIFY catalogChanged)
    Q_PROPERTY(int sourceCount READ sourceCount NOTIFY catalogChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY catalogChanged)
    Q_PROPERTY(bool monitoring READ monitoring WRITE setMonitoring NOTIFY optionsChanged)
    Q_PROPERTY(int tileSize READ tileSize WRITE setTileSize NOTIFY optionsChanged)
    Q_PROPERTY(bool setupComplete READ setupComplete NOTIFY setupChanged)
    Q_PROPERTY(int wizardStep READ wizardStep NOTIFY setupChanged)
    Q_PROPERTY(QString onlineConsent READ onlineConsent NOTIFY setupChanged)
    Q_PROPERTY(QString profileName READ profileName NOTIFY setupChanged)
public:
    explicit LibraryController(Database &database, bool autoStart=true, bool notifications=true, QObject *parent=nullptr);
    ~LibraryController() override;
    QAbstractItemModel *movies() { return &m_movies; }
    QAbstractItemModel *series() { return &m_series; }
    QAbstractItemModel *recent() { return &m_recent; }
    QAbstractItemModel *randomMovies() { return &m_randomMovies; }
    QAbstractItemModel *randomSeries() { return &m_randomSeries; }
    QAbstractItemModel *unassigned() { return &m_unassigned; }
    QAbstractItemModel *sources() { return &m_sources; }
    QAbstractItemModel *errors() { return &m_errors; }
    QAbstractItemModel *selectedFiles() { return &m_files; }
    QVariantMap selectedTitle() const { return m_selection; }
    bool busy() const { return m_busy; }
    bool paused() const { return m_paused; }
    int checked() const { return m_checked; }
    int total() const { return m_total; }
    QString currentPath() const { return m_currentPath; }
    QString status() const { return m_status; }
    QString error() const { return m_error; }
    QString watchNotice() const { return m_watchNotice; }
    int movieCount() const { return m_movieCount; }
    int seriesCount() const { return m_seriesCount; }
    int unassignedCount() const { return m_unassigned.rowCount(); }
    int sourceCount() const;
    int errorCount() const { return m_errors.rowCount(); }
    bool monitoring() const { return m_monitoring; }
    void setMonitoring(bool enabled);
    int tileSize() const { return m_tileSize; }
    void setTileSize(int size);
    bool setupComplete() const { return m_setupComplete; }
    int wizardStep() const { return m_wizardStep; }
    QString onlineConsent() const { return m_consent; }
    QString profileName() const { return m_profileName; }
    Q_INVOKABLE void addSource(const QUrl &url,const QString &kind);
    Q_INVOKABLE void removeSource(qint64 id);
    Q_INVOKABLE void scan(qint64 sourceId=0);
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void openDetails(qint64 titleId);
    Q_INVOKABLE void playTitle(qint64 titleId);
    Q_INVOKABLE void playFile(const QString &path);
    Q_INVOKABLE void assignFile(qint64 id,const QString &title,int year,int season,int episode);
    Q_INVOKABLE QVariantList search(const QString &query) const;
    Q_INVOKABLE QString sortOrder(const QString &kind) const;
    Q_INVOKABLE int availability(const QString &kind) const;
    Q_INVOKABLE void setOptions(const QString &kind,const QString &order,int availability);
    Q_INVOKABLE void setWizardStep(int step);
    Q_INVOKABLE void setOnlineConsent(const QString &consent);
    Q_INVOKABLE void setProfileName(const QString &name);
    Q_INVOKABLE void finishSetup();
    Q_INVOKABLE void clearError();
    void reload();
signals:
    void ready();
    void catalogChanged();
    void selectionChanged();
    void scanChanged();
    void errorChanged();
    void optionsChanged();
    void setupChanged();
    void detailsRequested();
    void playRequested(const QUrl &url);
    void scanCompleted(bool interrupted);
private:
    void applySnapshot(const QVariantMap &snapshot);
    void report(const QString &message);
    void watchDirectories(const QStringList &paths);
    void notifyCompletion();
    Database &m_database;
    LibraryRepository m_repository;
    EntryModel m_titles,m_unassigned,m_sources,m_errors,m_files;
    CatalogProxy m_movies,m_series,m_recent,m_randomMovies,m_randomSeries;
    QThread m_thread;
    ScannerWorker *m_worker;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce,m_poll,m_watchTimer;
    QStringList m_watchQueue;
    int m_failedWatches=0;
    bool m_monitoring=true,m_autoStart=true;
    bool m_ready=false,m_busy=false,m_paused=false,m_rescan=false,m_closing=false,m_notifications;
    int m_movieCount=0,m_seriesCount=0;
    int m_checked=0,m_total=0,m_tileSize=170,m_wizardStep=0;
    bool m_setupComplete=false;
    QString m_currentPath,m_status="Bereit",m_error,m_watchNotice,m_consent="undecided",m_profileName;
    QVariantMap m_selection;
};
}
