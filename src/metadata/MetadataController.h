#pragma once
#include "database/Database.h"
#include "library/LibraryController.h"
#include "profiles/PersonalController.h"
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QSet>
namespace veluno {
class MetadataController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList results READ results NOTIFY changed)
    Q_PROPERTY(QVariantList images READ images NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool configured READ configured NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
  public:
    MetadataController(Database &db, LibraryController &library, PersonalController &personal,
                       QObject *parent = nullptr,
                       const QUrl &api = QUrl("https://api.themoviedb.org/3"));
    ~MetadataController() override;
    QVariantList results() const {
        return m_results;
    }
    QVariantList images() const {
        return m_images;
    }
    bool busy() const {
        return !m_requests.isEmpty() || !m_replies.isEmpty() || m_imageJobs > 0;
    }
    bool configured() const {
        return !m_db.setting("tmdb_token").isEmpty();
    }
    QString status() const {
        return m_status;
    }
    Q_INVOKABLE void setToken(const QString &token);
    Q_INVOKABLE void search(qint64 title, const QString &query);
    Q_INVOKABLE void choose(qint64 providerId);
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void edit(qint64 title, const QString &name, const QString &overview,
                          int year = -1);
    Q_INVOKABLE void showImages(qint64 title);
    Q_INVOKABLE void selectImage(qint64 imageId);
    Q_INVOKABLE void selectLocalImage(qint64 title, const QString &kind, const QUrl &url);
    Q_INVOKABLE void refreshTitle(qint64 title);
  signals:
    void changed();

  private:
    struct Request {
        QString path;
        std::function<void(QJsonObject)> done;
    };
    bool allowed() const;
    void request(const QString &path, std::function<void(QJsonObject)> done);
    void pump();
    void lookup(qint64 title, bool interactive, const QString &query = {});
    void fetch(qint64 title, qint64 provider);
    void image(qint64 title, const QString &kind, const QString &remote, bool primary);
    void installImage(qint64 title, const QString &kind, const QString &remote,
                      const QString &local);
    void report(const QString &error);
    Database &m_db;
    LibraryController &m_library;
    PersonalController &m_personal;
    QNetworkAccessManager m_network;
    QUrl m_api;
    QQueue<Request> m_requests;
    QSet<QNetworkReply *> m_replies;
    QSet<qint64> m_pendingTitles;
    QTimer m_pump, m_refresh;
    QThread m_imageThread;
    QObject *m_imageWorker;
    int m_imageJobs = 0;
    qint64 m_selected = 0;
    quint64 m_generation = 0;
    QString m_status;
    QVariantList m_results, m_images;
};
} // namespace veluno
