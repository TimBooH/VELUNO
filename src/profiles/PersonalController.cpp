#include "PersonalController.h"
#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QRegularExpression>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace veluno {
static QString now() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
PersonalController::PersonalController(Database &db, LibraryController &library, MpvEngine &player,
                                       QObject *parent)
    : QObject(parent), m_db(db), m_library(library), m_player(player) {
    connect(&library, &LibraryController::catalogChanged, this, &PersonalController::refresh);
    connect(&library, &LibraryController::playbackStarting, this,
            &PersonalController::preparePlayback);
    connect(&player, &MpvEngine::stateChanged, this, &PersonalController::playerState);
    connect(&player, &MpvEngine::mediaInfoChanged, this, &PersonalController::checkNext);
    connect(&player, &MpvEngine::durationChanged, this, &PersonalController::playerState);
    connect(&player, &MpvEngine::positionChanged, this, &PersonalController::checkNext);
    connect(&player, &MpvEngine::aboutToStop, this, &PersonalController::saveProgress);
    m_saveTimer.setInterval(5000);
    connect(&m_saveTimer, &QTimer::timeout, this, &PersonalController::saveProgress);
    m_saveTimer.start();
    m_nextTimer.setInterval(1000);
    connect(&m_nextTimer, &QTimer::timeout, this, [this] {
        if (m_countdown > 0)
            --m_countdown;
        emit countdownChanged();
        if (m_countdown == 0)
            playNext();
    });
    refresh();
}
PersonalController::~PersonalController() {
    saveProgress();
}
qint64 PersonalController::profileId() const {
    return m_db.setting("active_profile", "1").toLongLong();
}
void PersonalController::report(const std::exception &e) {
    m_error = QString::fromUtf8(e.what());
    emit changed();
}
QVariantList PersonalController::titleRows(const QString &join, const QString &where,
                                           const QVariantList &values, const QString &order) const {
    return m_db.query(
        "SELECT t.*,d.poster,d.backdrop,d.overview,d.genres,d.rating,max(f.available) AS "
        "available,count(DISTINCT f.id) AS file_count FROM media_titles t JOIN media_files f ON "
        "f.title_id=t.id LEFT JOIN media_details d ON d.title_id=t.id " +
            join + " WHERE " + where + " GROUP BY t.id ORDER BY " + order,
        values);
}
void PersonalController::refresh() {
    try {
        const auto id = profileId();
        auto profiles = m_db.query("SELECT * FROM profiles ORDER BY id");
        for (auto &v : profiles) {
            auto p = v.toMap();
            if (p.value("avatar").toString().startsWith("images/"))
                p["avatar_url"] =
                    QUrl::fromLocalFile(
                        QFileInfo(m_db.path()).dir().filePath(p.value("avatar").toString()))
                        .toString();
            v = p;
        }
        m_profiles.replace(profiles);
        const auto active = m_db.query("SELECT * FROM profiles WHERE id=?", {id});
        if (active.isEmpty())
            throw std::runtime_error("Aktives Profil fehlt.");
        m_profile = active.first().toMap();
        const auto imageUrl = [this](const QString &path) {
            return QUrl::fromLocalFile(QFileInfo(m_db.path()).dir().filePath(path)).toString();
        };
        if (m_profile.value("avatar").toString().startsWith("images/"))
            m_profile["avatar_url"] = imageUrl(m_profile.value("avatar").toString());
        m_favorites.replace(
            titleRows("JOIN favorites v ON v.title_id=t.id", "v.profile_id=?", {id}));
        auto groups = m_db.query(
            "SELECT * FROM collections WHERE profile_id=? ORDER BY name COLLATE NOCASE", {id});
        for (auto &group : groups) {
            auto g = group.toMap();
            g["posters"] = titleRows("JOIN collection_items i ON i.title_id=t.id",
                                     "i.collection_id=?", {g.value("id")})
                               .mid(0, 4);
            if (!g.value("cover").toString().isEmpty())
                g["cover_url"] = imageUrl(g.value("cover").toString());
            group = g;
        }
        m_collections.replace(groups);
        if (!m_collection.isEmpty()) {
            const auto c = m_db.query("SELECT * FROM collections WHERE id=? AND profile_id=?",
                                      {m_collection.value("id"), id});
            m_collection = c.isEmpty() ? QVariantMap{} : c.first().toMap();
            m_collectionTitles.replace(
                m_collection.isEmpty()
                    ? QVariantList{}
                    : titleRows("JOIN collection_items i ON i.title_id=t.id", "i.collection_id=?",
                                {m_collection.value("id")},
                                m_collection.value("sort") == "recent" ? "max(i.added_at) DESC"
                                                                       : "t.title COLLATE NOCASE"));
        } else
            m_collectionTitles.replace({});
        m_continue.replace(m_db.query(
            "SELECT p.rowid AS id,p.*,t.title,t.kind,t.year,d.poster,d.backdrop,f.path,f.available "
            "FROM playback_progress p JOIN media_titles t ON t.id=p.title_id JOIN media_files f ON "
            "f.id=p.file_id LEFT JOIN media_details d ON d.title_id=t.id WHERE p.profile_id=? AND "
            "p.position>0 AND p.seen=0 AND p.rowid=(SELECT p2.rowid FROM playback_progress p2 "
            "WHERE p2.profile_id=p.profile_id AND p2.title_id=p.title_id ORDER BY p2.updated_at "
            "DESC,p2.rowid DESC LIMIT 1) ORDER BY p.updated_at DESC",
            {id}));
        ++m_revision;
        emit changed();
    } catch (const std::exception &e) {
        report(e);
    }
}
QString PersonalController::importImage(const QUrl &url) {
    if (!url.isLocalFile())
        throw std::runtime_error("Bitte ein lokales Bild auswählen.");
    QImageReader reader(url.toLocalFile());
    const auto size = reader.size();
    if (!size.isValid() || qint64(size.width()) * size.height() > 64000000)
        throw std::runtime_error("Bildformat oder Bildgröße wird nicht unterstützt.");
    reader.setAutoTransform(true);
    reader.setScaledSize(size.scaled(800, 800, Qt::KeepAspectRatio));
    const auto image = reader.read();
    if (image.isNull())
        throw std::runtime_error("Bild konnte nicht gelesen werden.");
    const auto relative = "images/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".png";
    const auto dir = QFileInfo(m_db.path()).dir();
    if (!QDir().mkpath(dir.filePath("images")) || !image.save(dir.filePath(relative), "PNG"))
        throw std::runtime_error("Bild konnte nicht gespeichert werden.");
    return relative;
}
void PersonalController::saveProfile(qint64 id, const QString &name, const QString &avatar,
                                     const QString &color) {
    try {
        if (name.trimmed().isEmpty() || !QColor(color).isValid())
            throw std::runtime_error("Bitte Profilname und gültige Farbe wählen.");
        QString stored = avatar;
        if (QUrl(avatar).isLocalFile())
            stored = importImage(QUrl(avatar));
        if (!QStringList{"initials", "cinema", "rocket", "planet"}.contains(stored) &&
            !QRegularExpression("^images/[a-zA-Z0-9-]+\\.png$").match(stored).hasMatch())
            throw std::runtime_error("Ungültiger Avatar.");
        if (!id)
            m_db.query("INSERT INTO profiles(name,avatar,color) VALUES(?,?,?)",
                       {name.trimmed(), stored, color});
        else
            m_db.query("UPDATE profiles SET name=?,avatar=?,color=? WHERE id=?",
                       {name.trimmed(), stored, color, id});
        m_library.reload();
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::switchProfile(qint64 id) {
    try {
        if (m_db.query("SELECT id FROM profiles WHERE id=?", {id}).isEmpty())
            throw std::runtime_error("Profil nicht gefunden.");
        stopTracking();
        m_library.cancelPendingPlayback();
        m_db.setSetting("active_profile", QString::number(id));
        m_collection.clear();
        m_library.reload();
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::deleteProfile(qint64 id) {
    try {
        const auto remaining = m_db.query("SELECT id FROM profiles WHERE id<>? ORDER BY id", {id});
        if (remaining.isEmpty())
            throw std::runtime_error("Mindestens ein Profil muss erhalten bleiben.");
        if (id == profileId())
            switchProfile(remaining.first().toMap().value("id").toLongLong());
        m_db.query("DELETE FROM profiles WHERE id=?", {id});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
bool PersonalController::isFavorite(qint64 title) const {
    return !m_db.query("SELECT 1 FROM favorites WHERE profile_id=? AND title_id=?",
                       {profileId(), title})
                .isEmpty();
}
void PersonalController::toggleFavorite(qint64 title) {
    try {
        if (isFavorite(title))
            m_db.query("DELETE FROM favorites WHERE profile_id=? AND title_id=?",
                       {profileId(), title});
        else
            m_db.query("INSERT INTO favorites VALUES(?,?)", {profileId(), title});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::saveCollection(qint64 id, const QString &name) {
    try {
        if (name.trimmed().isEmpty())
            throw std::runtime_error("Bitte einen Sammlungsnamen eingeben.");
        if (id)
            m_db.query("UPDATE collections SET name=? WHERE id=? AND profile_id=?",
                       {name.trimmed(), id, profileId()});
        else
            m_db.query("INSERT INTO collections(profile_id,name) VALUES(?,?)",
                       {profileId(), name.trimmed()});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::deleteCollection(qint64 id) {
    try {
        m_db.query("DELETE FROM collections WHERE id=? AND profile_id=?", {id, profileId()});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::openCollection(qint64 id) {
    m_collection = {{"id", id}};
    refresh();
}
void PersonalController::setCollectionSort(const QString &sort) {
    if (sort != "title" && sort != "recent")
        return;
    try {
        m_db.query("UPDATE collections SET sort=? WHERE id=? AND profile_id=?",
                   {sort, m_collection.value("id"), profileId()});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::setCollectionCover(const QUrl &url) {
    try {
        const auto image = importImage(url);
        m_db.query("UPDATE collections SET cover=? WHERE id=? AND profile_id=?",
                   {image, m_collection.value("id"), profileId()});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
bool PersonalController::collectionContains(qint64 id, qint64 title) const {
    return !m_db.query("SELECT 1 FROM collection_items i JOIN collections c ON "
                       "c.id=i.collection_id WHERE c.profile_id=? AND c.id=? AND i.title_id=?",
                       {profileId(), id, title})
                .isEmpty();
}
void PersonalController::toggleCollectionTitle(qint64 id, qint64 title) {
    try {
        if (m_db.query("SELECT id FROM collections WHERE id=? AND profile_id=?", {id, profileId()})
                .isEmpty())
            throw std::runtime_error("Sammlung gehört nicht zu diesem Profil.");
        if (collectionContains(id, title))
            m_db.query("DELETE FROM collection_items WHERE collection_id=? AND title_id=?",
                       {id, title});
        else
            m_db.query("INSERT INTO collection_items VALUES(?,?,?)", {id, title, now()});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
bool PersonalController::isSeen(qint64 title) const {
    const auto rows =
        m_db.query("SELECT count(*) AS total,sum(coalesce(p.seen,0)) AS seen FROM (SELECT DISTINCT "
                   "title_id,season,episode FROM media_files WHERE title_id=?) f LEFT JOIN "
                   "playback_progress p ON p.title_id=f.title_id AND p.season=f.season AND "
                   "p.episode=f.episode AND p.profile_id=?",
                   {title, profileId()});
    return !rows.isEmpty() && rows.first().toMap().value("total").toInt() > 0 &&
           rows.first().toMap().value("total").toInt() ==
               rows.first().toMap().value("seen").toInt();
}
void PersonalController::markSeen(qint64 title, bool seen) {
    try {
        m_db.transaction([&] {
            for (const auto &row :
                 m_db.query("SELECT min(id) AS id,season,episode,max(duration) AS duration FROM "
                            "media_files WHERE title_id=? GROUP BY season,episode",
                            {title})) {
                const auto f = row.toMap();
                m_db.query("INSERT INTO "
                           "playback_progress(profile_id,title_id,season,episode,file_id,position,"
                           "duration,seen,updated_at) VALUES(?,?,?,?,?,0,?,?,?) ON "
                           "CONFLICT(profile_id,title_id,season,episode) DO UPDATE SET "
                           "seen=excluded.seen,position=0,updated_at=excluded.updated_at",
                           {profileId(), title, f.value("season"), f.value("episode"),
                            f.value("id"), f.value("duration"), seen ? 1 : 0, now()});
            }
        });
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::preparePlayback(const QString &path) {
    stopTracking();
    const auto file = m_db.query("SELECT f.*,t.kind FROM media_files f LEFT JOIN media_titles t ON "
                                 "t.id=f.title_id WHERE f.path=?",
                                 {path});
    if (file.isEmpty() || file.first().toMap().value("title_id").isNull())
        return;
    m_file = file.first().toMap();
    m_waiting = true;
    m_started = false;
    m_suppressNext = false;
    const auto saved = m_db.query(
        "SELECT position,seen FROM playback_progress WHERE profile_id=? AND title_id=? AND "
        "season=? AND episode=?",
        {profileId(), m_file.value("title_id"), m_file.value("season"), m_file.value("episode")});
    m_resume = saved.isEmpty() || saved.first().toMap().value("seen").toBool()
                   ? 0
                   : saved.first().toMap().value("position").toDouble();
    const auto marker =
        m_db.query("SELECT position FROM credits_markers WHERE file_id=?", {m_file.value("id")});
    m_credit = marker.isEmpty() ? -1 : marker.first().toMap().value("position").toDouble();
    if (m_file.value("kind") == "series") {
        const auto next = m_db.query(
            "SELECT path FROM media_files WHERE title_id=? AND available=1 AND (season>? OR "
            "(season=? AND episode>?)) ORDER BY season,episode,height DESC,width DESC LIMIT 1",
            {m_file.value("title_id"), m_file.value("season"), m_file.value("season"),
             m_file.value("episode")});
        if (!next.isEmpty())
            m_next = next.first().toMap();
    }
    emit countdownChanged();
}
void PersonalController::playerState() {
    if (m_waiting && m_player.active() && !m_player.loading() && m_player.duration() > 0) {
        m_waiting = false;
        m_started = true;
        if (m_resume > 0 && m_resume < m_player.duration() * .9)
            m_player.seek(m_resume);
    }
    if (m_player.paused() || m_player.ended())
        saveProgress();
    checkNext();
}
void PersonalController::saveProgress() {
    if (m_file.isEmpty() || !m_started || m_player.duration() <= 0 ||
        !std::isfinite(m_player.position()))
        return;
    try {
        const auto duration = m_player.duration();
        const auto position = m_player.ended() ? duration : m_player.position();
        m_db.query("INSERT INTO "
                   "playback_progress(profile_id,title_id,season,episode,file_id,position,duration,"
                   "seen,updated_at) VALUES(?,?,?,?,?,?,?,?,?) ON "
                   "CONFLICT(profile_id,title_id,season,episode) DO UPDATE SET "
                   "file_id=excluded.file_id,position=excluded.position,duration=excluded.duration,"
                   "seen=excluded.seen,updated_at=excluded.updated_at",
                   {profileId(), m_file.value("title_id"), m_file.value("season"),
                    m_file.value("episode"), m_file.value("id"), position, duration,
                    position >= duration * .9 ? 1 : 0, now()});
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::stopTracking() {
    saveProgress();
    m_player.stop();
    m_file.clear();
    m_next.clear();
    m_waiting = false;
    m_started = false;
    cancelNext();
}
QVariantList PersonalController::progressForTitle(qint64 title) const {
    return m_db.query("SELECT p.*,(SELECT f.id FROM media_files f WHERE f.title_id=p.title_id "
                      "AND f.season=p.season AND f.episode=p.episode AND f.available=1 "
                      "ORDER BY f.height DESC,f.width DESC,f.id LIMIT 1) AS preferred_file_id "
                      "FROM playback_progress p WHERE p.profile_id=? AND p.title_id=? "
                      "ORDER BY p.updated_at DESC,p.rowid DESC",
                      {profileId(), title});
}
void PersonalController::resumeTitle(qint64 title) {
    m_library.openDetails(title);
    if (m_library.selectedTitle().value("kind") == "movie") {
        m_library.playTitle(title);
        return;
    }
    for (const auto &value : progressForTitle(title)) {
        const auto progress = value.toMap();
        if (!progress.value("seen").toBool() && progress.value("position").toDouble() > 0 &&
            !progress.value("preferred_file_id").isNull()) {
            resume(progress.value("preferred_file_id").toLongLong());
            return;
        }
    }
}
void PersonalController::resume(qint64 fileId) {
    const auto files = m_db.query("SELECT path,title_id FROM media_files WHERE id=?", {fileId});
    if (!files.isEmpty()) {
        const auto f = files.first().toMap();
        m_library.openDetails(f.value("title_id").toLongLong());
        m_library.playFile(f.value("path").toString());
    }
}
void PersonalController::markCredits() {
    if (m_file.isEmpty() || m_file.value("kind") != "series" || m_player.position() <= 0)
        return;
    try {
        m_credit = m_player.position();
        m_db.query("INSERT INTO credits_markers VALUES(?,?) ON CONFLICT(file_id) DO UPDATE SET "
                   "position=excluded.position",
                   {m_file.value("id"), m_credit});
        checkNext();
    } catch (const std::exception &e) {
        report(e);
    }
}
void PersonalController::checkNext() {
    if (m_next.isEmpty() || !m_started || m_suppressNext || m_countdown >= 0)
        return;
    if (m_player.ended() ||
        ((m_credit >= 0 ? m_credit : m_player.creditsStart()) >= 0 &&
         m_player.position() >= (m_credit >= 0 ? m_credit : m_player.creditsStart()))) {
        m_countdown = 10;
        m_nextTimer.start();
        emit countdownChanged();
    }
}
void PersonalController::cancelNext() {
    m_nextTimer.stop();
    m_countdown = -1;
    m_suppressNext = true;
    emit countdownChanged();
}
void PersonalController::playNext() {
    if (m_next.isEmpty())
        return;
    const auto path = m_next.value("path").toString();
    m_nextTimer.stop();
    m_library.playFile(path);
}
} // namespace veluno
