#pragma once
#include "database/Database.h"
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
namespace veluno {
class BackupService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString directory READ directory NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList backups READ backups NOTIFY changed)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY changed)
  public:
    explicit BackupService(Database &db, bool automatic = true, QObject *parent = nullptr);
    ~BackupService() override;
    bool busy() const {
        return m_busy;
    }
    bool restartRequired() const {
        return m_restartRequired;
    }
    QString directory() const;
    QString status() const {
        return m_status;
    }
    QVariantList backups() const {
        return m_backups;
    }
    Q_INVOKABLE void setDirectory(const QUrl &url);
    Q_INVOKABLE void resetDirectory();
    Q_INVOKABLE void create(const QString &kind = "manual");
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void restore(const QUrl &backup);
    Q_INVOKABLE void remove(const QUrl &backup);
    static QString createSnapshot(const QString &database, const QString &destination,
                                  const QString &kind);
    static QVariantMap verifySnapshot(const QString &directory);
    static void protectBeforeMigration(const QString &database);
    static void applyPendingRestore(const QString &database);
  signals:
    void changed();
    void completed(bool success, const QString &path);

  private:
    void daily();
    void run(const std::function<QString()> &operation, bool restoring = false);
    Database &m_db;
    QThread m_thread;
    QObject *m_worker;
    QTimer m_daily;
    bool m_busy = false, m_restartRequired = false;
    QString m_status;
    QVariantList m_backups;
};
} // namespace veluno
