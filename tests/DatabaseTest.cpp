#include <QtTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QFile>
#include "database/Database.h"
#include "database/Migrations.h"
#include "core/AppSettings.h"
#include <stdexcept>
class DatabaseTest : public QObject {
    Q_OBJECT
private slots:
    void initializationAndPersistence() {
        QTemporaryDir dir;
        const auto path = dir.filePath("nested/veluno.sqlite3");
        {
            veluno::Database db(path);
            QCOMPARE(db.setting("active_profile"), "1");
            veluno::AppSettings settings(db);
            QCOMPARE(settings.theme(), "midnight");
            settings.setTheme("light");
            QCOMPARE(db.setting("theme"), "light");
            settings.setTheme("invalid");
            QCOMPARE(settings.theme(), "light");
        }
        {
            veluno::Database db(path);
            veluno::AppSettings settings(db);
            QCOMPARE(settings.theme(), "light");
        }
        auto sql = QSqlDatabase::addDatabase("QSQLITE", "inspect");
        sql.setDatabaseName(path);
        QVERIFY(sql.open());
        {
            QSqlQuery q(sql);
            QVERIFY(q.exec("SELECT count(*) FROM profiles")); QVERIFY(q.next()); QCOMPARE(q.value(0).toInt(), 1);
            QVERIFY(q.exec("PRAGMA user_version")); QVERIFY(q.next()); QCOMPARE(q.value(0).toInt(), 1);
        }
        sql.close(); sql = {}; QSqlDatabase::removeDatabase("inspect");
    }
    void migrationFailureRollsBack() {
        auto sql = QSqlDatabase::addDatabase("QSQLITE", "rollback");
        sql.setDatabaseName(":memory:"); QVERIFY(sql.open());
        QVERIFY_EXCEPTION_THROWN(veluno::migrate(sql, {{1, {"CREATE TABLE sentinel(id INTEGER)", "INVALID SQL"}}}), std::runtime_error);
        {
            QSqlQuery q(sql);
            QVERIFY(q.exec("PRAGMA user_version")); QVERIFY(q.next()); QCOMPARE(q.value(0).toInt(), 0);
            QVERIFY(!sql.tables().contains("sentinel"));
        }
        veluno::migrate(sql, veluno::schemaMigrations());
        QVERIFY(sql.tables().contains("profiles"));
        sql.close(); sql = {}; QSqlDatabase::removeDatabase("rollback");
    }
    void newerSchemaIsNotModified() {
        QTemporaryDir dir;
        const auto path = dir.filePath("future.sqlite3");
        auto sql = QSqlDatabase::addDatabase("QSQLITE", "future");
        sql.setDatabaseName(path); QVERIFY(sql.open());
        veluno::execute(sql, "PRAGMA user_version=99");
        sql.close(); sql = {}; QSqlDatabase::removeDatabase("future");
        QFile before(path); QVERIFY(before.open(QIODevice::ReadOnly)); const auto bytes = before.readAll(); before.close();
        QVERIFY_EXCEPTION_THROWN(veluno::Database{path}, std::runtime_error);
        QFile after(path); QVERIFY(after.open(QIODevice::ReadOnly)); QCOMPARE(after.readAll(), bytes);
    }
    void corruptDatabaseIsPreserved() {
        QTemporaryDir dir;
        const auto path = dir.filePath("corrupt.sqlite3");
        QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("not a database"); f.close();
        QVERIFY_EXCEPTION_THROWN(veluno::Database{path}, std::runtime_error);
        QVERIFY(f.open(QIODevice::ReadOnly)); QCOMPARE(f.readAll(), QByteArray("not a database"));
    }
    void failedSettingDoesNotChangeTheme() {
        QTemporaryDir dir;
        const auto path = dir.filePath("locked.sqlite3");
        veluno::Database db(path);
        veluno::AppSettings settings(db);
        auto blocker = QSqlDatabase::addDatabase("QSQLITE", "blocker");
        blocker.setDatabaseName(path); QVERIFY(blocker.open());
        veluno::execute(blocker, "BEGIN EXCLUSIVE");
        settings.setTheme("light");
        QCOMPARE(settings.theme(), "midnight"); QVERIFY(!settings.error().isEmpty());
        veluno::execute(blocker, "ROLLBACK"); blocker.close(); blocker = {}; QSqlDatabase::removeDatabase("blocker");
        settings.setTheme("light"); QCOMPARE(settings.theme(), "light"); QVERIFY(settings.error().isEmpty());
    }
};
QTEST_GUILESS_MAIN(DatabaseTest)
#include "DatabaseTest.moc"
