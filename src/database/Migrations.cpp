#include "Migrations.h"
#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>
namespace veluno {
void execute(QSqlDatabase &db, const QString &sql) {
    QSqlQuery query(db);
    if (!query.exec(sql))
        throw std::runtime_error(query.lastError().text().toStdString());
}
QVector<Migration> schemaMigrations() {
    return {
        {1,
         {"CREATE TABLE profiles (id INTEGER PRIMARY KEY, name TEXT NOT NULL)",
          "INSERT INTO profiles(id, name) VALUES(1, 'Standardprofil')",
          "CREATE TABLE app_settings (key TEXT PRIMARY KEY, value TEXT NOT NULL)",
          "INSERT INTO app_settings VALUES('active_profile', '1')",
          "INSERT INTO app_settings VALUES('theme', 'midnight')"}},
        {2,
         {"CREATE TABLE media_sources(id INTEGER PRIMARY KEY, path TEXT NOT NULL UNIQUE, kind TEXT "
          "NOT NULL CHECK(kind IN ('movie','series')), enabled INTEGER NOT NULL DEFAULT 1)",
          "CREATE TABLE media_titles(id INTEGER PRIMARY KEY, kind TEXT NOT NULL, title TEXT NOT "
          "NULL, year INTEGER NOT NULL DEFAULT 0, identity_key TEXT NOT NULL, added_at TEXT NOT "
          "NULL, UNIQUE(kind,identity_key))",
          "CREATE TABLE media_files(id INTEGER PRIMARY KEY, source_id INTEGER NOT NULL REFERENCES "
          "media_sources(id), title_id INTEGER REFERENCES media_titles(id), path TEXT NOT NULL "
          "UNIQUE, season INTEGER NOT NULL DEFAULT 0, episode INTEGER NOT NULL DEFAULT 0, width "
          "INTEGER NOT NULL DEFAULT 0, height INTEGER NOT NULL DEFAULT 0, duration REAL NOT NULL "
          "DEFAULT 0, size INTEGER NOT NULL DEFAULT 0, modified TEXT NOT NULL DEFAULT '', "
          "available INTEGER NOT NULL DEFAULT 1, manual INTEGER NOT NULL DEFAULT 0, scan_token "
          "TEXT NOT NULL DEFAULT '', diagnostic TEXT NOT NULL DEFAULT '')",
          "CREATE INDEX media_files_title ON media_files(title_id)",
          "CREATE INDEX media_files_source ON media_files(source_id)",
          "CREATE TABLE scan_errors(id INTEGER PRIMARY KEY, source_id INTEGER NOT NULL REFERENCES "
          "media_sources(id), path TEXT NOT NULL, message TEXT NOT NULL, scan_token TEXT NOT NULL "
          "DEFAULT '', UNIQUE(source_id,path))",
          "CREATE TABLE profile_preferences(profile_id INTEGER NOT NULL REFERENCES profiles(id), "
          "key TEXT NOT NULL, value TEXT NOT NULL, PRIMARY KEY(profile_id,key))"}},
        {3,
         {"ALTER TABLE profiles ADD COLUMN avatar TEXT NOT NULL DEFAULT 'initials'",
          "ALTER TABLE profiles ADD COLUMN color TEXT NOT NULL DEFAULT '#36BBD1'",
          "CREATE TABLE favorites(profile_id INTEGER NOT NULL REFERENCES profiles(id) ON DELETE "
          "CASCADE,title_id INTEGER NOT NULL REFERENCES media_titles(id) ON DELETE CASCADE,PRIMARY "
          "KEY(profile_id,title_id))",
          "CREATE TABLE collections(id INTEGER PRIMARY KEY,profile_id INTEGER NOT NULL REFERENCES "
          "profiles(id) ON DELETE CASCADE,name TEXT NOT NULL,cover TEXT NOT NULL DEFAULT '',sort "
          "TEXT NOT NULL DEFAULT 'title',UNIQUE(profile_id,name))",
          "CREATE TABLE collection_items(collection_id INTEGER NOT NULL REFERENCES collections(id) "
          "ON DELETE CASCADE,title_id INTEGER NOT NULL REFERENCES media_titles(id) ON DELETE "
          "CASCADE,added_at TEXT NOT NULL,PRIMARY KEY(collection_id,title_id))",
          "CREATE TABLE playback_progress(profile_id INTEGER NOT NULL REFERENCES profiles(id) ON "
          "DELETE CASCADE,title_id INTEGER NOT NULL REFERENCES media_titles(id) ON DELETE "
          "CASCADE,season INTEGER NOT NULL DEFAULT 0,episode INTEGER NOT NULL DEFAULT 0,file_id "
          "INTEGER NOT NULL REFERENCES media_files(id),position REAL NOT NULL DEFAULT 0,duration "
          "REAL NOT NULL DEFAULT 0,seen INTEGER NOT NULL DEFAULT 0,updated_at TEXT NOT "
          "NULL,PRIMARY KEY(profile_id,title_id,season,episode))",
          "CREATE TABLE credits_markers(file_id INTEGER PRIMARY KEY REFERENCES media_files(id) ON "
          "DELETE CASCADE,position REAL NOT NULL)",
          "CREATE INDEX progress_profile ON playback_progress(profile_id,updated_at)"}},
        {4,
         {"CREATE TABLE media_details(title_id INTEGER PRIMARY KEY REFERENCES media_titles(id) ON "
          "DELETE CASCADE,provider_id INTEGER NOT NULL DEFAULT 0,overview TEXT NOT NULL DEFAULT "
          "'',genres TEXT NOT NULL DEFAULT '',rating REAL NOT NULL DEFAULT 0,poster TEXT NOT NULL "
          "DEFAULT '',backdrop TEXT NOT NULL DEFAULT '',manual_title INTEGER NOT NULL DEFAULT "
          "0,manual_overview INTEGER NOT NULL DEFAULT 0,manual_poster INTEGER NOT NULL DEFAULT "
          "0,manual_backdrop INTEGER NOT NULL DEFAULT 0,fetched_at TEXT NOT NULL DEFAULT '')",
          "CREATE TABLE media_images(id INTEGER PRIMARY KEY,title_id INTEGER NOT NULL REFERENCES "
          "media_titles(id) ON DELETE CASCADE,kind TEXT NOT NULL CHECK(kind IN "
          "('poster','backdrop')),remote_path TEXT NOT NULL,local_path TEXT NOT NULL DEFAULT "
          "'',is_primary INTEGER NOT NULL DEFAULT 0,UNIQUE(title_id,kind,remote_path))"}}};
}
void migrate(QSqlDatabase &db, const QVector<Migration> &migrations) {
    QSqlQuery query(db);
    if (!query.exec("PRAGMA user_version") || !query.next())
        throw std::runtime_error("Schemaversion ist nicht lesbar.");
    int current = query.value(0).toInt();
    query.finish();
    int expected = 1;
    for (const auto &migration : migrations)
        if (migration.version != expected++)
            throw std::runtime_error("Lücke in den Migrationen.");
    if (current > migrations.size())
        throw std::runtime_error("Datenbank ist neuer als diese Anwendung. Downgrade gesperrt.");
    for (const auto &migration : migrations) {
        if (migration.version <= current)
            continue;
        if (!db.transaction())
            throw std::runtime_error(db.lastError().text().toStdString());
        try {
            for (const auto &sql : migration.statements)
                execute(db, sql);
            execute(db, QString("PRAGMA user_version = %1").arg(migration.version));
            if (!db.commit())
                throw std::runtime_error(db.lastError().text().toStdString());
        } catch (...) {
            db.rollback();
            throw;
        }
    }
}
} // namespace veluno
