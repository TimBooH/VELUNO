#include "Database.h"
#include "Migrations.h"
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <stdexcept>
namespace veluno {
Database::Database(const QString &path) : m_connection(QUuid::createUuid().toString()) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) throw std::runtime_error("Datenverzeichnis kann nicht erstellt werden.");
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connection);
    m_db.setDatabaseName(path);
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    try {
        if (!m_db.open()) throw std::runtime_error(m_db.lastError().text().toStdString());
        execute(m_db, "PRAGMA foreign_keys=ON");
        QSqlQuery check(m_db);
        if (!check.exec("PRAGMA quick_check") || !check.next() || check.value(0).toString() != "ok")
            throw std::runtime_error("SQLite-Konsistenzprüfung fehlgeschlagen.");
        check.finish();
        migrate(m_db, schemaMigrations());
    } catch (...) {
        m_db.close(); m_db = {}; QSqlDatabase::removeDatabase(m_connection); throw;
    }
}
Database::~Database() {
    m_db.close(); m_db = {}; QSqlDatabase::removeDatabase(m_connection);
}
QString Database::setting(const QString &key, const QString &fallback) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT value FROM app_settings WHERE key=?"); query.addBindValue(key);
    if (!query.exec()) throw std::runtime_error(query.lastError().text().toStdString());
    return query.next() ? query.value(0).toString() : fallback;
}
void Database::setSetting(const QString &key, const QString &value) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO app_settings(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value");
    query.addBindValue(key); query.addBindValue(value);
    if (!query.exec()) throw std::runtime_error(query.lastError().text().toStdString());
}
}
