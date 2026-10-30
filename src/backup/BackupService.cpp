#include "BackupService.h"
#include "database/Migrations.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStorageInfo>
#include <QUuid>
#include <algorithm>
#include <stdexcept>
namespace veluno {
namespace {
void require(bool condition, const char *error) {
    if (!condition)
        throw std::runtime_error(error);
}
QString nonce() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
QString parentOf(const QString &db) {
    return QFileInfo(db).absolutePath();
}
void jsonFile(const QString &file, const QJsonObject &object) {
    QSaveFile out(file);
    require(out.open(QIODevice::WriteOnly), "Manifest nicht schreibbar.");
    const auto bytes = QJsonDocument(object).toJson();
    require(out.write(bytes) == bytes.size() && out.commit(),
            "Manifest konnte nicht gespeichert werden.");
}
QJsonObject readJson(const QString &path) {
    QFile in(path);
    require(in.open(QIODevice::ReadOnly) && in.size() < 16000000,
            "Manifest fehlt oder ist zu groß.");
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(in.readAll(), &error);
    require(error.error == QJsonParseError::NoError && doc.isObject(), "Manifest ist beschädigt.");
    return doc.object();
}
QByteArray hash(const QString &path) {
    QFile in(path);
    require(in.open(QIODevice::ReadOnly), "Sicherungsdatei ist nicht lesbar.");
    QCryptographicHash result(QCryptographicHash::Sha256);
    require(result.addData(&in), "Sicherungsdatei konnte nicht geprüft werden.");
    return result.result().toHex();
}
bool safeRelative(const QString &path) {
    return !QDir::isAbsolutePath(path) && !path.contains('\\') && !path.split('/').contains("..") &&
           !path.split('/').contains(".") && !path.contains(QChar::Null) &&
           (path == "database.sqlite3" || path.startsWith("images/") ||
            path.startsWith("metadata/"));
}
QStringList inspectDatabase(const QString &path, int *version = nullptr) {
    QStringList assets;
    const auto name = nonce();
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(path);
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        try {
            require(db.open(), "Sicherungsdatenbank konnte nicht geöffnet werden.");
            QSqlQuery q(db);
            require(q.exec("PRAGMA integrity_check") && q.next() && q.value(0).toString() == "ok",
                    "Sicherungsdatenbank ist inkonsistent.");
            q.finish();
            require(q.exec("PRAGMA user_version") && q.next() && q.value(0).toInt() > 0 &&
                        q.value(0).toInt() <= schemaMigrations().size(),
                    "Sicherungsdatenbank ist inkompatibel.");
            if (version)
                *version = q.value(0).toInt();
            q.finish();
            require(q.exec("PRAGMA foreign_key_check") && !q.next(),
                    "Sicherungsdatenbank enthält ungültige Verknüpfungen.");
            q.finish();
            const QList<QPair<QString, QString>> fields = {{"profiles", "avatar"},
                                                           {"collections", "cover"},
                                                           {"media_details", "poster"},
                                                           {"media_details", "backdrop"},
                                                           {"media_images", "local_path"}};
            for (const auto &field : fields) {
                bool exists = false;
                require(q.exec("PRAGMA table_info(" + field.first + ")"),
                        "Sicherungsschema nicht lesbar.");
                while (q.next())
                    if (q.value(1).toString() == field.second)
                        exists = true;
                q.finish();
                if (!exists)
                    continue;
                require(q.exec("SELECT " + field.second + " FROM " + field.first + " WHERE " +
                               field.second + "<>''"),
                        "Bildverweise nicht lesbar.");
                while (q.next()) {
                    const auto asset = q.value(0).toString();
                    if (field.first == "profiles" &&
                        QStringList{"initials", "cinema", "rocket", "planet"}.contains(asset))
                        continue;
                    require(safeRelative(asset) && asset != "database.sqlite3",
                            "Ungültiger Bildverweis in Sicherung.");
                    if (!assets.contains(asset))
                        assets.append(asset);
                }
                q.finish();
            }
        } catch (...) {
            db.close();
            db = {};
            QSqlDatabase::removeDatabase(name);
            throw;
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
    return assets;
}
void copyTree(const QString &source, const QString &target) {
    require(QDir().mkpath(target), "Zielverzeichnis konnte nicht erstellt werden.");
    QFile::setPermissions(target,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QDirIterator it(source, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        require(!info.isSymLink(), "Verknüpfungen sind in Sicherungsdaten nicht erlaubt.");
        const auto relative = QDir(source).relativeFilePath(info.filePath());
        const auto dest = QDir(target).filePath(relative);
        if (info.isDir())
            require(QDir().mkpath(dest), "Zielordner konnte nicht angelegt werden.");
        else
            require(QDir().mkpath(QFileInfo(dest).absolutePath()) &&
                        QFile::copy(info.filePath(), dest),
                    "Datei konnte nicht kopiert werden.");
    }
}
} // namespace
QString BackupService::createSnapshot(const QString &database, const QString &destination,
                                      const QString &kind) {
    require(QStringList{"daily", "manual", "update", "pre-restore"}.contains(kind),
            "Ungültige Sicherungsart.");
    const auto source = parentOf(database);
    const auto targetRoot = QDir(destination).absolutePath();
    require(targetRoot != source && !targetRoot.startsWith(source + "/"),
            "Sicherungsziel muss außerhalb des Anwendungsdatenordners liegen.");
    require(QDir().mkpath(targetRoot),
            "Sicherungsziel ist nicht erreichbar oder nicht schreibbar.");
    const auto storage = QStorageInfo(targetRoot);
    require(!storage.isValid() || storage.bytesAvailable() < 0 ||
                storage.bytesAvailable() > QFileInfo(database).size() + 16 * 1024 * 1024,
            "Nicht genügend freier Speicher für die Sicherung.");
    const auto final =
        QDir(targetRoot)
            .filePath(kind + "-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmsszzz") +
                      "-" + nonce() + ".veluno-backup");
    const auto temporary = final + ".partial";
    require(QDir().mkpath(temporary), "Temporäre Sicherung konnte nicht erstellt werden.");
    QFile::setPermissions(temporary,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    try {
        {
            const auto name = nonce();
            {
                auto db = QSqlDatabase::addDatabase("QSQLITE", name);
                db.setDatabaseName(database);
                db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
                try {
                    require(QFileInfo::exists(database) && db.open(),
                            "Quelldatenbank ist nicht lesbar.");
                    QSqlQuery q(db);
                    q.prepare("VACUUM INTO ?");
                    q.addBindValue(QDir(temporary).filePath("database.sqlite3"));
                    require(q.exec(), "Konsistenter Datenbanksnapshot fehlgeschlagen.");
                } catch (...) {
                    db.close();
                    db = {};
                    QSqlDatabase::removeDatabase(name);
                    throw;
                }
                db.close();
            }
            QSqlDatabase::removeDatabase(name);
        }
        int snapshotSchema = 0;
        for (const auto &asset :
             inspectDatabase(QDir(temporary).filePath("database.sqlite3"), &snapshotSchema)) {
            QString path = source;
            for (const auto &part : asset.split('/')) {
                path = QDir(path).filePath(part);
                require(!QFileInfo(path).isSymLink(), "Bildverweis enthält eine Verknüpfung.");
            }
            const auto target = QDir(temporary).filePath(asset);
            require(QDir().mkpath(QFileInfo(target).absolutePath()) && QFile::copy(path, target),
                    "Ein dauerhaft relevantes Bild fehlt oder konnte nicht gesichert werden.");
        }
        QJsonObject files;
        QDirIterator it(temporary, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const auto relative = QDir(temporary).relativeFilePath(it.filePath());
            require(safeRelative(relative), "Unerwartete Datei im Sicherungsumfang.");
            files.insert(relative, QString::fromLatin1(hash(it.filePath())));
        }
        const QJsonObject manifest{
            {"format", 1},
            {"kind", kind},
            {"created", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
            {"schema", snapshotSchema},
            {"files", files}};
        jsonFile(QDir(temporary).filePath("manifest.json"), manifest);
        verifySnapshot(temporary);
        require(QDir().rename(temporary, final), "Sicherung konnte nicht abgeschlossen werden.");
        if (kind == "daily") {
            QList<QPair<QString, QString>> daily;
            for (const auto &entry :
                 QDir(targetRoot)
                     .entryInfoList({"daily-*.veluno-backup"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
                try {
                    const auto info = verifySnapshot(entry.filePath());
                    if (info.value("kind") == "daily")
                        daily.append({info.value("created").toString(), entry.filePath()});
                } catch (...) {
                }
            }
            std::sort(daily.begin(), daily.end());
            while (daily.size() > 7) {
                const auto old = daily.takeFirst().second;
                require(QDir(old).removeRecursively(),
                        "Alte tägliche Sicherung konnte nicht entfernt werden.");
            }
        }
        return final;
    } catch (...) {
        QDir(temporary).removeRecursively();
        throw;
    }
}
QVariantMap BackupService::verifySnapshot(const QString &directory) {
    require(!QFileInfo(directory).isSymLink(), "Sicherung darf keine Verknüpfung sein.");
    const auto manifest = readJson(QDir(directory).filePath("manifest.json"));
    require(manifest.value("format").toInt() == 1 && manifest.value("files").isObject(),
            "Unbekanntes Sicherungsformat.");
    const auto files = manifest.value("files").toObject();
    require(files.contains("database.sqlite3"), "Datenbank fehlt in Sicherung.");
    for (auto it = files.begin(); it != files.end(); ++it) {
        const auto relative = it.key();
        require(safeRelative(relative), "Unsicherer Sicherungspfad.");
        auto components = relative.split('/');
        QString path = directory;
        for (const auto &part : components) {
            path = QDir(path).filePath(part);
            require(!QFileInfo(path).isSymLink(), "Sicherungsdatei enthält eine Verknüpfung.");
        }
        require(QFileInfo(path).isFile() &&
                    QString::fromLatin1(hash(path)) == it.value().toString(),
                "Sicherung unvollständig oder Prüfsumme falsch.");
    }
    // Reject extra files so restore cannot import unverified content.
    QDirIterator all(directory, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (all.hasNext()) {
        all.next();
        const auto relative = QDir(directory).relativeFilePath(all.filePath());
        require(relative == "manifest.json" || files.contains(relative),
                "Nicht geprüfte Zusatzdatei in Sicherung.");
    }
    int snapshotSchema = 0;
    for (const auto &asset :
         inspectDatabase(QDir(directory).filePath("database.sqlite3"), &snapshotSchema))
        require(files.contains(asset),
                "Ein dauerhaft relevantes Bild fehlt im Sicherungsmanifest.");
    require(manifest.value("schema").toInt() == snapshotSchema,
            "Sicherungsschema stimmt nicht mit Manifest überein.");
    return manifest.toVariantMap();
}
BackupService::BackupService(Database &db, bool automatic, QObject *parent)
    : QObject(parent), m_db(db), m_worker(new QObject) {
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
    connect(this, &BackupService::completed, this, [this](bool ok, const QString &path) {
        if (ok && QFileInfo(path).fileName().startsWith("daily-")) {
            try {
                m_db.setSetting("last_daily_backup", QDate::currentDate().toString(Qt::ISODate));
            } catch (const std::exception &e) {
                m_status = QString::fromUtf8(e.what());
                emit changed();
            }
        }
    });
    m_daily.setInterval(60000);
    connect(&m_daily, &QTimer::timeout, this, &BackupService::daily);
    if (automatic) {
        m_daily.start();
        QTimer::singleShot(1000, this, &BackupService::daily);
    }
    refresh();
}
BackupService::~BackupService() {
    m_daily.stop();
    m_thread.quit();
    m_thread.wait();
}
void BackupService::resetDirectory() {
    setDirectory(
        QUrl::fromLocalFile(QFileInfo(parentOf(m_db.path())).dir().filePath("VELUNO-backups")));
}
QString BackupService::directory() const {
    return m_db.setting("backup_directory",
                        QFileInfo(parentOf(m_db.path())).dir().filePath("VELUNO-backups"));
}
void BackupService::setDirectory(const QUrl &url) {
    try {
        if (m_busy)
            return;
        if (!url.isLocalFile()) {
            m_status = "Sicherungsziel muss als lokaler oder eingebundener Ordner erreichbar sein.";
            emit changed();
            return;
        }
        m_db.setSetting("backup_directory", url.toLocalFile());
        refresh();
    } catch (const std::exception &e) {
        m_status = QString::fromUtf8(e.what());
        emit changed();
    }
}
void BackupService::run(const std::function<QString()> &operation, bool restoring) {
    if (m_busy || m_restartRequired)
        return;
    m_busy = true;
    m_status = "Datensicherung läuft …";
    emit changed();
    QMetaObject::invokeMethod(
        m_worker,
        [this, operation, restoring] {
            QString path, error;
            try {
                path = operation();
            } catch (const std::exception &e) {
                error = QString::fromUtf8(e.what());
            }
            QMetaObject::invokeMethod(
                this,
                [this, path, error, restoring] {
                    m_busy = false;
                    m_status = error.isEmpty() ? "Datensicherung erfolgreich" : error;
                    m_restartRequired = restoring && error.isEmpty();
                    refresh();
                    emit completed(error.isEmpty(), path);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}
void BackupService::create(const QString &kind) {
    const auto db = m_db.path(), dest = directory();
    run([db, dest, kind] { return createSnapshot(db, dest, kind); });
}
void BackupService::daily() {
    if (!m_busy && !m_restartRequired &&
        m_db.setting("last_daily_backup") != QDate::currentDate().toString(Qt::ISODate))
        create("daily");
}
void BackupService::refresh() {
    const auto root = directory();
    QMetaObject::invokeMethod(
        m_worker,
        [this, root] {
            QVariantList list;
            for (const auto &entry :
                 QDir(root).entryInfoList({"*.veluno-backup"}, QDir::Dirs | QDir::NoDotAndDotDot,
                                          QDir::Name | QDir::Reversed)) {
                QVariantMap m{{"path", QUrl::fromLocalFile(entry.filePath()).toString()},
                              {"name", entry.fileName()},
                              {"valid", false}};
                try {
                    const auto metadata = verifySnapshot(entry.filePath());
                    for (auto it = metadata.begin(); it != metadata.end(); ++it)
                        if (it.key() != "files")
                            m.insert(it.key(), it.value());
                    m["valid"] = true;
                } catch (const std::exception &e) {
                    m["error"] = QString::fromUtf8(e.what());
                }
                list.append(m);
            }
            QMetaObject::invokeMethod(
                this,
                [this, list] {
                    m_backups = list;
                    emit changed();
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}
void BackupService::restore(const QUrl &backup) {
    if (!backup.isLocalFile())
        return;
    const auto selected = backup.toLocalFile(), db = m_db.path(), dest = directory();
    run(
        [selected, db, dest] {
            verifySnapshot(selected);
            createSnapshot(db, dest, "pre-restore");
            const auto appData = parentOf(db);
            const auto stage = appData + ".restore-" + nonce();
            copyTree(selected, stage);
            verifySnapshot(stage);
            require(QFile::rename(stage + "/database.sqlite3", stage + "/veluno.sqlite3"),
                    "Wiederherstellung konnte nicht vorbereitet werden.");
            QFile::remove(stage + "/manifest.json");
            jsonFile(appData + ".restore-request.json",
                     {{"stage", stage}, {"previous", appData + ".previous-" + nonce()}});
            return stage;
        },
        true);
}
void BackupService::protectBeforeMigration(const QString &database) {
    if (!QFileInfo::exists(database))
        return;
    const auto name = nonce();
    int version = 0;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(database);
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        require(db.open(), "Datenbank vor Migration nicht lesbar.");
        QSqlQuery q(db);
        require(q.exec("PRAGMA user_version") && q.next(), "Datenbankversion nicht lesbar.");
        version = q.value(0).toInt();
        q.finish();
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
    if (version > 0 && version < schemaMigrations().size())
        createSnapshot(database, QFileInfo(parentOf(database)).dir().filePath("VELUNO-backups"),
                       "update");
}
void BackupService::applyPendingRestore(const QString &database) {
    const auto live = parentOf(database), request = live + ".restore-request.json";
    if (!QFileInfo::exists(request))
        return;
    const auto state = readJson(request);
    const auto stage = state.value("stage").toString(),
               previous = state.value("previous").toString();
    require(stage.startsWith(live + ".restore-") && previous.startsWith(live + ".previous-") &&
                QFileInfo(stage).absolutePath() == QFileInfo(live).absolutePath() &&
                QFileInfo(previous).absolutePath() == QFileInfo(live).absolutePath(),
            "Wiederherstellungsauftrag ist ungültig.");
    require(!QFileInfo(stage).isSymLink() && !QFileInfo(previous).isSymLink(),
            "Wiederherstellungsverzeichnisse sind ungültig.");
    if (QDir(stage).exists()) {
        inspectDatabase(stage + "/veluno.sqlite3");
        if (QDir(live).exists())
            require(!QDir(previous).exists() && QDir().rename(live, previous),
                    "Aktueller Datenstand konnte nicht zurückgelegt werden.");
        if (!QDir().rename(stage, live)) {
            QDir().rename(previous, live);
            throw std::runtime_error(
                "Wiederherstellung fehlgeschlagen; alter Datenstand wurde zurückgelegt.");
        }
    } else if (!QDir(live).exists() && QDir(previous).exists())
        require(QDir().rename(previous, live),
                "Alter Datenstand konnte nicht zurückgeholt werden.");
    require(QDir(live).exists(), "Wiederherstellung enthält keinen nutzbaren Datenstand.");
    require(QFile::remove(request), "Wiederherstellungsauftrag konnte nicht abgeschlossen werden.");
}
void BackupService::remove(const QUrl &backup) {
    if (m_busy || !backup.isLocalFile())
        return;
    const auto path = backup.toLocalFile();
    if (QFileInfo(path).absolutePath() != QDir(directory()).absolutePath() ||
        !QFileInfo(path).fileName().endsWith(".veluno-backup") || QFileInfo(path).isSymLink())
        return;
    const auto selected = path;
    run([selected] {
        const auto manifest = readJson(selected + "/manifest.json");
        require(manifest.value("format").toInt() == 1 && manifest.value("files").isObject(),
                "Unbekanntes Verzeichnis wird nicht gelöscht.");
        const auto files = manifest.value("files").toObject();
        require(files.contains("database.sqlite3"), "Sicherungsdatenbank fehlt im Manifest.");
        QDirIterator it(selected, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            require(!it.fileInfo().isSymLink(),
                    "Sicherung enthält eine Verknüpfung und wird nicht automatisch gelöscht.");
            if (it.fileInfo().isFile()) {
                const auto relative = QDir(selected).relativeFilePath(it.filePath());
                require(
                    relative == "manifest.json" ||
                        (safeRelative(relative) && files.contains(relative)),
                    "Sicherung enthält zusätzliche Dateien. Bitte vor dem Löschen manuell prüfen.");
            }
        }
        require(QDir(selected).removeRecursively(), "Sicherung konnte nicht gelöscht werden.");
        return QString{};
    });
}
} // namespace veluno
