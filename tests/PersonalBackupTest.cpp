#include "PlayerFixture.h"
#include "backup/BackupService.h"
#include "database/Migrations.h"
#include "player/PlayerSettings.h"
#include "profiles/PersonalController.h"
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
using namespace veluno;
class PersonalBackupTest : public QObject {
    Q_OBJECT
  private slots:
    void profilesCollectionsAndOriginals() {
        QTemporaryDir dir;
        const auto media = dir.filePath("media");
        QVERIFY(QDir().mkpath(media));
        const auto file = media + "/Movie.2020.mp4";
        QVERIFY(createPlayerFixture(file));
        const auto hash = fixtureHash(file);
        Database db(dir.filePath("app/veluno.sqlite3"));
        LibraryRepository repo(db);
        const auto source = repo.addSource(media, "movie");
        ScannedFile f;
        f.sourceId = source;
        f.path = file;
        f.kind = "movie";
        f.title = "Movie";
        f.year = 2020;
        f.width = 96;
        f.height = 64;
        f.duration = 14;
        repo.record(f, "test");
        const auto title = repo.file(file).value("title_id").toLongLong();
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        personal.toggleFavorite(title);
        QVERIFY(personal.isFavorite(title));
        personal.saveCollection(0, "Meine Sammlung");
        const auto collection =
            db.query("SELECT id FROM collections").first().toMap().value("id").toLongLong();
        personal.toggleCollectionTitle(collection, title);
        QVERIFY(personal.collectionContains(collection, title));
        personal.saveProfile(0, "Zweites Profil", "planet", "#36BBD1");
        const auto second =
            db.query("SELECT max(id) AS id FROM profiles").first().toMap().value("id").toLongLong();
        personal.switchProfile(second);
        QVERIFY(!personal.isFavorite(title));
        QVERIFY(!personal.collectionContains(collection, title));
        QCOMPARE(personal.collections()->rowCount(), 0);
        personal.toggleCollectionTitle(collection, title);
        QVERIFY(!personal.error().isEmpty());
        personal.clearError();
        personal.markSeen(title, true);
        QVERIFY(personal.isSeen(title));
        personal.switchProfile(1);
        QVERIFY(!personal.isSeen(title));
        QVERIFY(personal.isFavorite(title));
        personal.deleteCollection(collection);
        QVERIFY(!personal.collectionContains(collection, title));
        personal.deleteProfile(second);
        QCOMPARE(personal.profiles()->rowCount(), 1);
        QCOMPARE(fixtureHash(file), hash);
    }
    void persistedProgressAndNinetyPercent() {
        QTemporaryDir dir;
        const auto media = dir.filePath("media");
        QVERIFY(QDir().mkpath(media));
        const auto path = media + "/Movie.mp4";
        QVERIFY(createPlayerFixture(path));
        Database db(dir.filePath("app/veluno.sqlite3"));
        LibraryRepository repo(db);
        ScannedFile file;
        file.sourceId = repo.addSource(media, "movie");
        file.kind = "movie";
        file.title = "Movie";
        file.path = path;
        repo.record(file, "scan");
        const auto id = repo.file(path).value("title_id").toLongLong();
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        connect(&library, &LibraryController::playRequested, &player, &MpvEngine::openFile);
        library.openDetails(id);
        library.playTitle(id);
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_VERIFY(player.duration() > 13);
        player.seek(6);
        QTRY_VERIFY(player.position() > 5.8);
        player.togglePause();
        personal.saveProgress();
        QVERIFY(!personal.isSeen(id));
        QCOMPARE(personal.continueWatching()->rowCount(), 1);
        player.stop();
        library.playTitle(id);
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(player.position() > 5.8, 5000);
        player.seek(13);
        QTRY_VERIFY(player.position() > 12.8);
        personal.saveProgress();
        QVERIFY(personal.isSeen(id));
        QCOMPARE(personal.continueWatching()->rowCount(), 0);
        player.stop();
        library.playTitle(id);
        QTRY_VERIFY_WITH_TIMEOUT(!player.loading(), 10000);
        QTRY_VERIFY(player.position() < 2);
        personal.stopTracking();
    }
    void episodeCountdownAndNextEpisode() {
        QTemporaryDir dir;
        const auto media = dir.filePath("media");
        QVERIFY(QDir().mkpath(media));
        const auto first = media + "/Show.S01E01.mp4", second = media + "/Show.S01E02.mp4";
        QVERIFY(createPlayerFixture(first));
        QVERIFY(QFile::copy(first, second));
        Database db(dir.filePath("app/veluno.sqlite3"));
        LibraryRepository repo(db);
        const auto source = repo.addSource(media, "series");
        for (int episode = 1; episode <= 2; ++episode) {
            ScannedFile file;
            file.sourceId = source;
            file.path = episode == 1 ? first : second;
            file.kind = "series";
            file.title = "Show";
            file.season = 1;
            file.episode = episode;
            repo.record(file, "test");
        }
        const auto title = repo.file(first).value("title_id").toLongLong();
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        connect(&library, &LibraryController::playRequested, &player, &MpvEngine::openFile);
        library.openDetails(title);
        library.playFile(first);
        QTRY_VERIFY(!player.loading());
        QTRY_VERIFY(player.duration() > 13);
        QVERIFY(personal.hasNext());
        player.seek(2);
        QTRY_VERIFY(player.position() > 1.8);
        personal.markCredits();
        QCOMPARE(personal.nextCountdown(), 10);
        personal.cancelNext();
        QCOMPARE(personal.nextCountdown(), -1);
        personal.playNext();
        QTRY_VERIFY(!player.loading());
        QVERIFY(!personal.hasNext());
        QCOMPARE(personal.nextCountdown(), -1);
        library.playFile(first);
        QTRY_VERIFY(!player.loading());
        QTRY_VERIFY(player.duration() > 13);
        personal.cancelNext();
        player.seek(13.8);
        QTRY_VERIFY_WITH_TIMEOUT(player.ended(), 5000); // canceled countdown stays canceled
        QCOMPARE(personal.nextCountdown(), -1);
        db.query("DELETE FROM credits_markers");
        library.playFile(first);
        QTRY_VERIFY(!player.loading());
        QTRY_VERIFY(player.duration() > 13);
        player.seek(13.8);
        QTRY_VERIFY_WITH_TIMEOUT(player.ended(), 5000);
        QTRY_VERIFY(personal.nextCountdown() >= 9 && personal.nextCountdown() <= 10);
        personal.cancelNext();
        personal.stopTracking();
    }
    void shortcutConflictsAndSettings() {
        QTemporaryDir dir;
        Database db(dir.filePath("db"));
        MpvEngine player(nullptr, true);
        PlayerSettings settings(db, player);
        QVERIFY(!settings.setShortcut("pause", "F"));
        QVERIFY(!settings.setShortcut("pause", "Ctrl+O"));
        QVERIFY(settings.setShortcut("pause", "Ctrl+P"));
        settings.setOption("sub_size", 48);
        settings.setOption("decoder", "software");
        PlayerSettings again(db, player);
        QCOMPARE(again.shortcuts().value("pause").toString(), QString("Ctrl+P"));
        QCOMPARE(again.options().value("sub_size").toInt(), 48);
        settings.setOption("sub_background", true);
        QTest::qWait(100);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
        settings.setOption("sub_background", false);
        settings.setOption("sub_size", -1);
        QVERIFY(!settings.error().isEmpty());
        QTest::qWait(100);
        QVERIFY2(player.error().isEmpty(), qPrintable(player.error()));
    }
    void snapshotBeforeMigrationKeepsOriginalSchema() {
        QTemporaryDir dir;
        const auto path = dir.filePath("app/veluno.sqlite3");
        QVERIFY(QDir().mkpath(dir.filePath("app")));
        const QString name = "preupgrade-fixture";
        {
            auto sql = QSqlDatabase::addDatabase("QSQLITE", name);
            sql.setDatabaseName(path);
            QVERIFY(sql.open());
            migrate(sql, {schemaMigrations()[0], schemaMigrations()[1]});
            QSqlQuery q(sql);
            QVERIFY(q.exec("UPDATE app_settings SET value='light' WHERE key='theme'"));
            q.finish();
            sql.close();
        }
        QSqlDatabase::removeDatabase(name);
        BackupService::protectBeforeMigration(path);
        const auto list =
            QDir(dir.filePath("VELUNO-backups"))
                .entryList({"update-*.veluno-backup"}, QDir::Dirs | QDir::NoDotAndDotDot);
        QCOMPARE(list.size(), 1);
        const auto info =
            BackupService::verifySnapshot(dir.filePath("VELUNO-backups/") + list.first());
        QCOMPARE(info.value("schema").toInt(), 2);
        Database upgraded(path);
        QCOMPARE(upgraded.setting("theme"), QString("light"));
        QCOMPARE(upgraded.query("PRAGMA user_version").first().toMap().first().toInt(),
                 schemaMigrations().size());
    }
    void backupsRotationCorruptionAndRestore() {
        QTemporaryDir dir;
        const auto dbpath = dir.filePath("app/veluno.sqlite3"), dest = dir.filePath("backups");
        QString first, manual, update;
        {
            Database db(dbpath);
            db.setSetting("test_value", "original");
            QVERIFY(QDir().mkpath(dir.filePath("app/images")));
            QImage image(20, 20, QImage::Format_RGB32);
            image.fill(Qt::red);
            QVERIFY(image.save(dir.filePath("app/images/test.png")));
            db.query("UPDATE profiles SET avatar='images/test.png' WHERE id=1");
            manual = BackupService::createSnapshot(dbpath, dest, "manual");
            update = BackupService::createSnapshot(dbpath, dest, "update");
            for (int i = 0; i < 8; ++i) {
                const auto path = BackupService::createSnapshot(dbpath, dest, "daily");
                if (!i)
                    first = path;
            }
            QVERIFY(!QDir(first).exists());
            QVERIFY(QDir(manual).exists());
            QVERIFY(QDir(update).exists());
            QCOMPARE(QDir(dest)
                         .entryList({"daily-*.veluno-backup"}, QDir::Dirs | QDir::NoDotAndDotDot)
                         .size(),
                     7);
            const auto info = BackupService::verifySnapshot(manual);
            QCOMPARE(info.value("kind").toString(), QString("manual"));
            QFile bad(update + "/images/test.png");
            QVERIFY(bad.open(QIODevice::WriteOnly | QIODevice::Append));
            bad.write("corrupt");
            bad.close();
            QVERIFY_EXCEPTION_THROWN(BackupService::verifySnapshot(update), std::runtime_error);
            db.setSetting("test_value", "changed");
            BackupService service(db, false);
            service.setDirectory(QUrl::fromLocalFile(dest));
            QSignalSpy done(&service, &BackupService::completed);
            service.restore(QUrl::fromLocalFile(manual));
            QTRY_VERIFY_WITH_TIMEOUT(done.count() > 0, 10000);
            QVERIFY(service.restartRequired());
        }
        BackupService::applyPendingRestore(dbpath);
        Database restored(dbpath);
        QCOMPARE(restored.setting("test_value"), QString("original"));
        QVERIFY(QFileInfo::exists(dir.filePath("app/images/test.png")));
        QVERIFY(!QDir(dest)
                     .entryList({"pre-restore-*.veluno-backup"}, QDir::Dirs | QDir::NoDotAndDotDot)
                     .isEmpty());
        QFile extra(manual + "/original-video.mp4");
        QVERIFY(extra.open(QIODevice::WriteOnly));
        extra.write("keep me");
        extra.close();
        BackupService cleanup(restored, false);
        cleanup.setDirectory(QUrl::fromLocalFile(dest));
        QSignalSpy removed(&cleanup, &BackupService::completed);
        cleanup.remove(QUrl::fromLocalFile(manual));
        QTRY_VERIFY_WITH_TIMEOUT(removed.count() > 0, 5000);
        QVERIFY(!removed.first().first().toBool());
        QVERIFY(QFileInfo::exists(extra.fileName()));
        removed.clear();
        cleanup.remove(QUrl::fromLocalFile(update));
        QTRY_VERIFY_WITH_TIMEOUT(removed.count() > 0, 5000);
        QVERIFY(removed.first().first().toBool());
        QVERIFY(!QDir(update).exists());
    }
};
QTEST_GUILESS_MAIN(PersonalBackupTest)
#include "PersonalBackupTest.moc"
