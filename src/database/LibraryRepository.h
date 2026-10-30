#pragma once
#include "Database.h"
#include <QVariantList>
#include <QVariantMap>
namespace veluno {
struct ScannedFile {
    qint64 sourceId = 0;
    QString path, kind, title, modified;
    QString diagnostic = QStringLiteral("");
    int year = 0, season = 0, episode = 0, width = 0, height = 0;
    qint64 size = 0;
    bool available = true;
    double duration = 0;
};
class LibraryRepository final {
public:
    explicit LibraryRepository(Database &database) : m_database(database) {}
    QVariantList sources(bool enabledOnly = false) const;
    qint64 addSource(const QString &path, const QString &kind);
    void disableSource(qint64 id);
    QVariantMap snapshot() const;
    QVariantList files(qint64 titleId) const;
    QVariantMap file(const QString &path) const;
    void record(const ScannedFile &file, const QString &scanToken);
    void markMissing(qint64 sourceId, const QString &token);
    void markSourceUnavailable(qint64 sourceId);
    void error(qint64 sourceId, const QString &path, const QString &message, const QString &token);
    void clearOldErrors(qint64 sourceId, const QString &token);
    void assign(qint64 fileId, const QString &title, int year, int season, int episode);
    QString preference(const QString &key, const QString &fallback) const;
    void setPreference(const QString &key, const QString &value);
    void setProfileName(const QString &name);
    QString profileName() const;
private:
    QVariantList rows(const QString &sql, const QVariantList &values = {}) const;
    void write(const QString &sql, const QVariantList &values = {}) const;
    qint64 titleId(const QString &kind, const QString &title, int year);
    Database &m_database;
};
}
