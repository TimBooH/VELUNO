#pragma once
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QVariantMap>
#include <memory>
#include <atomic>
#include "database/LibraryRepository.h"
namespace veluno {
class ScannerWorker final : public QObject {
    Q_OBJECT
public:
    explicit ScannerWorker(QString databasePath);
    void initialize();
    void prepare();
    void setPaused(bool paused);
    void cancel();
    void scan(qint64 sourceId = 0);
signals:
    void ready();
    void snapshotReady(const QVariantMap &snapshot);
    void progress(int checked, int total, const QString &path);
    void directoriesFound(const QStringList &paths);
    void finished(bool interrupted);
    void failed(const QString &message);
private:
    bool checkpoint();
    bool probe(ScannedFile &file);
    QString m_path;
    std::unique_ptr<Database> m_database;
    std::unique_ptr<LibraryRepository> m_repository;
    QMutex m_mutex;
    QWaitCondition m_condition;
    std::atomic_bool m_cancel{false}, m_paused{false};
};
}
