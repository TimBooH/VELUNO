#include "Migrations.h"
#include <QSqlQuery>
#include <QSqlError>
#include <stdexcept>
namespace veluno {
void execute(QSqlDatabase &db, const QString &sql) {
    QSqlQuery query(db);
    if (!query.exec(sql)) throw std::runtime_error(query.lastError().text().toStdString());
}
QVector<Migration> schemaMigrations() {
    return {{1, {
        "CREATE TABLE profiles (id INTEGER PRIMARY KEY, name TEXT NOT NULL)",
        "INSERT INTO profiles(id, name) VALUES(1, 'Standardprofil')",
        "CREATE TABLE app_settings (key TEXT PRIMARY KEY, value TEXT NOT NULL)",
        "INSERT INTO app_settings VALUES('active_profile', '1')",
        "INSERT INTO app_settings VALUES('theme', 'midnight')"
    }}, {2, {
        "CREATE TABLE media_sources(id INTEGER PRIMARY KEY, path TEXT NOT NULL UNIQUE, kind TEXT NOT NULL CHECK(kind IN ('movie','series')), enabled INTEGER NOT NULL DEFAULT 1)",
        "CREATE TABLE media_titles(id INTEGER PRIMARY KEY, kind TEXT NOT NULL, title TEXT NOT NULL, year INTEGER NOT NULL DEFAULT 0, identity_key TEXT NOT NULL, added_at TEXT NOT NULL, UNIQUE(kind,identity_key))",
        "CREATE TABLE media_files(id INTEGER PRIMARY KEY, source_id INTEGER NOT NULL REFERENCES media_sources(id), title_id INTEGER REFERENCES media_titles(id), path TEXT NOT NULL UNIQUE, season INTEGER NOT NULL DEFAULT 0, episode INTEGER NOT NULL DEFAULT 0, width INTEGER NOT NULL DEFAULT 0, height INTEGER NOT NULL DEFAULT 0, duration REAL NOT NULL DEFAULT 0, size INTEGER NOT NULL DEFAULT 0, modified TEXT NOT NULL DEFAULT '', available INTEGER NOT NULL DEFAULT 1, manual INTEGER NOT NULL DEFAULT 0, scan_token TEXT NOT NULL DEFAULT '', diagnostic TEXT NOT NULL DEFAULT '')",
        "CREATE INDEX media_files_title ON media_files(title_id)",
        "CREATE INDEX media_files_source ON media_files(source_id)",
        "CREATE TABLE scan_errors(id INTEGER PRIMARY KEY, source_id INTEGER NOT NULL REFERENCES media_sources(id), path TEXT NOT NULL, message TEXT NOT NULL, scan_token TEXT NOT NULL DEFAULT '', UNIQUE(source_id,path))",
        "CREATE TABLE profile_preferences(profile_id INTEGER NOT NULL REFERENCES profiles(id), key TEXT NOT NULL, value TEXT NOT NULL, PRIMARY KEY(profile_id,key))"
    }}};
}
void migrate(QSqlDatabase &db, const QVector<Migration> &migrations) {
    QSqlQuery query(db);
    if (!query.exec("PRAGMA user_version") || !query.next())
        throw std::runtime_error("Schemaversion ist nicht lesbar.");
    int current = query.value(0).toInt();
    query.finish();
    int expected = 1;
    for (const auto &migration : migrations)
        if (migration.version != expected++) throw std::runtime_error("Lücke in den Migrationen.");
    if (current > migrations.size()) throw std::runtime_error("Datenbank ist neuer als diese Anwendung. Downgrade gesperrt.");
    for (const auto &migration : migrations) {
        if (migration.version <= current) continue;
        if (!db.transaction()) throw std::runtime_error(db.lastError().text().toStdString());
        try {
            for (const auto &sql : migration.statements) execute(db, sql);
            execute(db, QString("PRAGMA user_version = %1").arg(migration.version));
            if (!db.commit()) throw std::runtime_error(db.lastError().text().toStdString());
        } catch (...) { db.rollback(); throw; }
    }
}
}
