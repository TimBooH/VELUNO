#include "LibraryRepository.h"
#include "platform/MediaLocation.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <stdexcept>
namespace veluno {
QVariantList LibraryRepository::rows(const QString &sql, const QVariantList &values) const {
    QSqlQuery query(m_database.m_db);
    query.prepare(sql);
    for (const auto &value : values)
        query.addBindValue(value);
    if (!query.exec())
        throw std::runtime_error(query.lastError().text().toStdString());
    QVariantList result;
    while (query.next()) {
        QVariantMap row;
        for (int i = 0; i < query.record().count(); ++i)
            row.insert(query.record().fieldName(i), query.value(i));
        result.append(row);
    }
    return result;
}
void LibraryRepository::write(const QString &sql, const QVariantList &values) const {
    rows(sql, values);
}
QVariantList LibraryRepository::sources(bool enabledOnly) const {
    return rows("SELECT id,path,kind,enabled FROM media_sources" +
                QString(enabledOnly ? " WHERE enabled=1" : "") + " ORDER BY id");
}
qint64 LibraryRepository::addSource(const QString &path, const QString &kind) {
    if (kind != "movie" && kind != "series")
        throw std::runtime_error("Ungültige Ordnerkategorie.");
    QString canonical;
    if (isSmb(path))
        canonical = normalizedSmb(QUrl(path)).toString(QUrl::FullyEncoded);
    else {
        QFileInfo info(path);
        if (!info.isDir() || !info.isReadable())
            throw std::runtime_error("Ordner ist nicht lesbar oder nicht erreichbar.");
        canonical = info.canonicalFilePath();
    }
    for (const auto &value : sources()) {
        const auto source = value.toMap();
        const auto other = source.value("path").toString();
        if (source.value("kind") != kind &&
            (canonical == other || canonical.startsWith(other + "/") ||
             other.startsWith(canonical + "/")))
            throw std::runtime_error("Überlappende Film- und Serienordner sind mehrdeutig. Bitte "
                                     "getrennte Ordner wählen.");
    }
    write("INSERT INTO media_sources(path,kind) VALUES(?,?) ON CONFLICT(path) DO UPDATE SET "
          "enabled=1",
          {canonical, kind});
    return rows("SELECT id FROM media_sources WHERE path=?", {canonical})
        .first()
        .toMap()
        .value("id")
        .toLongLong();
}
void LibraryRepository::disableSource(qint64 id) {
    write("UPDATE media_sources SET enabled=0 WHERE id=?", {id});
    markSourceUnavailable(id);
}
QVariantMap LibraryRepository::snapshot() const {
    return {
        {"sources", sources()},
        {"titles", rows("SELECT "
                        "t.id,t.kind,t.title,t.year,t.added_at,d.overview,d.genres,d.rating,d."
                        "poster,d.backdrop,count(f.id) AS file_count,max(f.available) AS "
                        "available,max(f.height) AS height "
                        "FROM media_titles t JOIN media_files f ON f.title_id=t.id LEFT JOIN "
                        "media_details d ON d.title_id=t.id GROUP BY t.id ORDER BY t.id")},
        {"unassigned", rows("SELECT f.id,f.path,f.diagnostic,f.available,f.season,f.episode,s.kind "
                            "FROM media_files f JOIN media_sources s ON s.id=f.source_id WHERE "
                            "f.title_id IS NULL ORDER BY f.id")},
        {"errors", rows("SELECT id,source_id,path,message FROM scan_errors ORDER BY id")}};
}
QVariantList LibraryRepository::files(qint64 titleId) const {
    return rows("SELECT id,path,season,episode,width,height,duration,available,diagnostic,manual "
                "FROM media_files WHERE title_id=? ORDER BY season,episode,height DESC,width "
                "DESC,available DESC,path",
                {titleId});
}
QVariantMap LibraryRepository::file(const QString &path) const {
    const auto result = rows("SELECT * FROM media_files WHERE path=?", {path});
    return result.isEmpty() ? QVariantMap{} : result.first().toMap();
}
qint64 LibraryRepository::titleId(const QString &kind, const QString &title, int year) {
    const auto key = title.simplified().normalized(QString::NormalizationForm_KC).toCaseFolded() +
                     "|" + QString::number(year);
    write("INSERT INTO media_titles(kind,title,year,identity_key,added_at) VALUES(?,?,?,?,?) ON "
          "CONFLICT(kind,identity_key) DO NOTHING",
          {kind, title.simplified(), year, key,
           QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)});
    return rows("SELECT id FROM media_titles WHERE kind=? AND identity_key=?", {kind, key})
        .first()
        .toMap()
        .value("id")
        .toLongLong();
}
void LibraryRepository::record(const ScannedFile &f, const QString &token) {
    auto &db = m_database.m_db;
    if (!db.transaction())
        throw std::runtime_error(db.lastError().text().toStdString());
    try {
        QVariant title;
        if (!f.title.isEmpty() && f.diagnostic.isEmpty())
            title = titleId(f.kind, f.title, f.year);
        write("INSERT INTO "
              "media_files(source_id,title_id,path,season,episode,width,height,duration,size,"
              "modified,available,scan_token,diagnostic) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?) "
              "ON CONFLICT(path) DO UPDATE SET source_id=excluded.source_id,title_id=CASE WHEN "
              "media_files.manual=1 OR excluded.diagnostic<>'' THEN media_files.title_id ELSE "
              "excluded.title_id END,"
              "season=CASE WHEN media_files.manual=1 THEN media_files.season ELSE excluded.season "
              "END,episode=CASE WHEN media_files.manual=1 THEN media_files.episode ELSE "
              "excluded.episode END,"
              "width=CASE WHEN excluded.diagnostic<>'' THEN media_files.width ELSE excluded.width "
              "END,height=CASE WHEN excluded.diagnostic<>'' THEN media_files.height ELSE "
              "excluded.height END,duration=CASE WHEN excluded.diagnostic<>'' THEN "
              "media_files.duration ELSE excluded.duration "
              "END,size=excluded.size,modified=excluded.modified,available=excluded.available,scan_"
              "token=excluded.scan_token,diagnostic=excluded.diagnostic",
              {f.sourceId, title, f.path, f.season, f.episode, f.width, f.height, f.duration,
               f.size, f.modified.isNull() ? QStringLiteral("") : f.modified, f.available ? 1 : 0,
               token, f.diagnostic});
        if (!db.commit())
            throw std::runtime_error(db.lastError().text().toStdString());
    } catch (...) {
        db.rollback();
        throw;
    }
}
void LibraryRepository::markMissing(qint64 id, const QString &token) {
    write("UPDATE media_files SET available=0 WHERE source_id=? AND scan_token<>?", {id, token});
}
void LibraryRepository::markSourceUnavailable(qint64 id) {
    write("UPDATE media_files SET available=0 WHERE source_id=?", {id});
}
void LibraryRepository::error(qint64 id, const QString &path, const QString &message,
                              const QString &token) {
    write("INSERT INTO scan_errors(source_id,path,message,scan_token) VALUES(?,?,?,?) ON "
          "CONFLICT(source_id,path) DO UPDATE SET "
          "message=excluded.message,scan_token=excluded.scan_token",
          {id, path, message, token});
}
void LibraryRepository::clearOldErrors(qint64 id, const QString &token) {
    write("DELETE FROM scan_errors WHERE source_id=? AND scan_token<>?", {id, token});
}
void LibraryRepository::assign(qint64 id, const QString &title, int year, int season, int episode) {
    if (title.simplified().isEmpty() || year < 0 || year > 9999)
        throw std::runtime_error("Bitte einen Titel und ein gültiges Jahr angeben.");
    auto &db = m_database.m_db;
    if (!db.transaction())
        throw std::runtime_error(db.lastError().text().toStdString());
    try {
        const auto data = rows("SELECT s.kind FROM media_files f JOIN media_sources s ON "
                               "s.id=f.source_id WHERE f.id=?",
                               {id});
        if (data.isEmpty())
            throw std::runtime_error("Datei wurde nicht gefunden.");
        const auto kind = data.first().toMap().value("kind").toString();
        if (kind == "series" && (season < 0 || episode < 1))
            throw std::runtime_error("Bitte Staffel und Episode angeben.");
        const auto target = titleId(kind, title, year);
        write("UPDATE media_files SET title_id=?,season=?,episode=?,manual=1 WHERE id=?",
              {target, kind == "series" ? season : 0, kind == "series" ? episode : 0, id});
        if (!db.commit())
            throw std::runtime_error(db.lastError().text().toStdString());
    } catch (...) {
        db.rollback();
        throw;
    }
}
QString LibraryRepository::preference(const QString &key, const QString &fallback) const {
    const auto values = rows("SELECT value FROM profile_preferences WHERE profile_id=? AND key=?",
                             {m_database.setting("active_profile", "1").toLongLong(), key});
    return values.isEmpty() ? fallback : values.first().toMap().value("value").toString();
}
void LibraryRepository::setPreference(const QString &key, const QString &value) {
    write("INSERT INTO profile_preferences(profile_id,key,value) VALUES(?,?,?) ON "
          "CONFLICT(profile_id,key) DO UPDATE SET value=excluded.value",
          {m_database.setting("active_profile", "1").toLongLong(), key, value});
}
QString LibraryRepository::profileName() const {
    const auto profiles = rows("SELECT name FROM profiles WHERE id=?",
                               {m_database.setting("active_profile", "1").toLongLong()});
    if (profiles.isEmpty())
        throw std::runtime_error("Aktives Profil wurde in der Datenbank nicht gefunden.");
    return profiles.first().toMap().value("name").toString();
}
void LibraryRepository::setProfileName(const QString &name) {
    if (name.simplified().isEmpty())
        throw std::runtime_error("Bitte einen Profilnamen angeben.");
    write("UPDATE profiles SET name=? WHERE id=?",
          {name.simplified(), m_database.setting("active_profile", "1").toLongLong()});
}
} // namespace veluno
