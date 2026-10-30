#pragma once
#include "database/Database.h"
#include "library/LibraryController.h"
#include "player/MpvEngine.h"
namespace veluno {
class PersonalController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *profiles READ profiles CONSTANT)
    Q_PROPERTY(QAbstractItemModel *favorites READ favorites CONSTANT)
    Q_PROPERTY(QAbstractItemModel *collections READ collections CONSTANT)
    Q_PROPERTY(QAbstractItemModel *collectionTitles READ collectionTitles CONSTANT)
    Q_PROPERTY(QAbstractItemModel *continueWatching READ continueWatching CONSTANT)
    Q_PROPERTY(QVariantMap activeProfile READ activeProfile NOTIFY changed)
    Q_PROPERTY(QVariantMap activeCollection READ activeCollection NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(int nextCountdown READ nextCountdown NOTIFY countdownChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY countdownChanged)
    Q_PROPERTY(QString error READ error NOTIFY changed)
  public:
    PersonalController(Database &db, LibraryController &library, MpvEngine &player,
                       QObject *parent = nullptr);
    ~PersonalController() override;
    QAbstractItemModel *profiles() {
        return &m_profiles;
    }
    QAbstractItemModel *favorites() {
        return &m_favorites;
    }
    QAbstractItemModel *collections() {
        return &m_collections;
    }
    QAbstractItemModel *collectionTitles() {
        return &m_collectionTitles;
    }
    QAbstractItemModel *continueWatching() {
        return &m_continue;
    }
    QVariantMap activeProfile() const {
        return m_profile;
    }
    QVariantMap activeCollection() const {
        return m_collection;
    }
    int revision() const {
        return m_revision;
    }
    QString error() const {
        return m_error;
    }
    int nextCountdown() const {
        return m_countdown;
    }
    bool hasNext() const {
        return !m_next.isEmpty();
    }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearError() {
        m_error.clear();
        emit changed();
    }
    Q_INVOKABLE void saveProfile(qint64 id, const QString &name, const QString &avatar,
                                 const QString &color);
    Q_INVOKABLE void switchProfile(qint64 id);
    Q_INVOKABLE void deleteProfile(qint64 id);
    Q_INVOKABLE bool isFavorite(qint64 title) const;
    Q_INVOKABLE void toggleFavorite(qint64 title);
    Q_INVOKABLE void saveCollection(qint64 id, const QString &name);
    Q_INVOKABLE void deleteCollection(qint64 id);
    Q_INVOKABLE void openCollection(qint64 id);
    Q_INVOKABLE void setCollectionSort(const QString &sort);
    Q_INVOKABLE void setCollectionCover(const QUrl &url);
    Q_INVOKABLE bool collectionContains(qint64 id, qint64 title) const;
    Q_INVOKABLE void toggleCollectionTitle(qint64 id, qint64 title);
    Q_INVOKABLE void markSeen(qint64 title, bool seen);
    Q_INVOKABLE bool isSeen(qint64 title) const;
    Q_INVOKABLE void resume(qint64 fileId);
    Q_INVOKABLE void saveProgress();
    Q_INVOKABLE void stopTracking();
    Q_INVOKABLE void markCredits();
    Q_INVOKABLE void cancelNext();
    Q_INVOKABLE void playNext();
    // Shared by image selectors; copies a decoded image, never changes its source.
    QString importImage(const QUrl &url);
  signals:
    void changed();
    void countdownChanged();

  private:
    qint64 profileId() const;
    void preparePlayback(const QString &path);
    void playerState();
    void checkNext();
    void report(const std::exception &e);
    QVariantList titleRows(const QString &join, const QString &where, const QVariantList &values,
                           const QString &order = "t.title COLLATE NOCASE") const;
    Database &m_db;
    LibraryController &m_library;
    MpvEngine &m_player;
    EntryModel m_profiles, m_favorites, m_collections, m_collectionTitles, m_continue;
    QVariantMap m_profile, m_collection, m_file, m_next;
    QString m_error;
    int m_revision = 0, m_countdown = -1;
    double m_resume = 0, m_credit = -1;
    bool m_waiting = false, m_started = false, m_suppressNext = false;
    QTimer m_saveTimer, m_nextTimer;
};
} // namespace veluno
