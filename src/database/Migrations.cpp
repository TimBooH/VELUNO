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
