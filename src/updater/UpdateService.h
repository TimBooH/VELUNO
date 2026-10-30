#pragma once
#include "backup/BackupService.h"
#include "database/Database.h"
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QThread>
namespace veluno {
class UpdateService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap buildInfo READ buildInfo CONSTANT)
    Q_PROPERTY(QString channel READ channel NOTIFY changed)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY changed)
    Q_PROPERTY(QVariantMap candidate READ candidate NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool downloaded READ downloaded NOTIFY changed)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
  public:
    UpdateService(Database &db, BackupService &backups, bool automatic = true,
                  QObject *parent = nullptr);
    ~UpdateService() override;
    static QVariantMap buildInfo();
    QString channel() const {
        return m_db.setting("update_channel", "stable");
    }
    QVariantMap settings() const;
    QVariantMap candidate() const {
        return m_candidate;
    }
    QString status() const {
        return m_status;
    }
    bool busy() const {
        return m_busy;
    }
    bool downloaded() const {
        return !m_download.isEmpty();
    }
    bool restartRequired() const {
        return m_restart;
    }
    double progress() const {
        return m_progress;
    }
    Q_INVOKABLE void setChannel(const QString &channel);
    Q_INVOKABLE bool configure(const QString &stable, const QString &unstable,
                               const QString &publicKey);
    Q_INVOKABLE void check();
    Q_INVOKABLE void download();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void install();
    static QVariantMap verifyManifest(const QByteArray &envelope, const QByteArray &key,
                                      const QString &channel, int schema, const QString &version,
                                      const QString &build);
    static QString installFile(const QString &source, const QString &target,
                               const QByteArray &sha256);
  signals:
    void changed();

  private:
    void fail(const QString &message);
    void beginInstall();
    Database &m_db;
    BackupService &m_backups;
    QNetworkAccessManager m_network;
    QNetworkReply *m_reply = nullptr;
    std::unique_ptr<QSaveFile> m_output;
    std::unique_ptr<QCryptographicHash> m_hash;
    QThread m_thread;
    QObject *m_worker;
    QTimer m_timer;
    bool m_busy = false, m_restart = false;
    double m_progress = 0;
    qint64 m_received = 0;
    QString m_status, m_download;
    QVariantMap m_candidate;
};
} // namespace veluno
