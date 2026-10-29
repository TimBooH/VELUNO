#pragma once
#include <QSqlDatabase>
#include <QString>
namespace veluno {
// Owns a single connection. Access only from its creating thread.
class Database final {
public:
    explicit Database(const QString &path);
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    QString setting(const QString &key, const QString &fallback = {}) const;
    void setSetting(const QString &key, const QString &value);
    QString path() const { return m_db.databaseName(); }
private:
    QString m_connection;
    QSqlDatabase m_db;
};
}
