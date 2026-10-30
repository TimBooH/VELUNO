#include <QtTest>
#include <QTemporaryDir>
#include <QProcess>
#include <QFile>
#include <QCryptographicHash>
#include <QAbstractItemModelTester>
#include <QSqlQuery>
#include "database/LibraryRepository.h"
#include "database/Migrations.h"
#include "library/LibraryController.h"
#include "scanner/FilenameParser.h"
using namespace veluno;
static bool clip(const QString &path,int width=96,int height=64) {
    QProcess p;
    p.start("ffmpeg",{"-hide_banner","-loglevel","error","-y","-f","lavfi","-i",QString("color=c=blue:s=%1x%2:r=1:d=1").arg(width).arg(height),"-frames:v","1","-an","-c:v","mpeg4",path});
    return p.waitForFinished(30000) && p.exitCode()==0;
}
static QByteArray digest(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return QCryptographicHash::hash(f.readAll(),QCryptographicHash::Sha256); }
static QVariantMap entry(QAbstractItemModel *model,int row) { return model->index(row,0).data(Qt::UserRole+1).toMap(); }
class LibraryTest : public QObject {
    Q_OBJECT
private slots:
    void parser_data() {
        QTest::addColumn<QString>("file"); QTest::addColumn<QString>("kind"); QTest::addColumn<QString>("title"); QTest::addColumn<int>("season"); QTest::addColumn<int>("episode");
        QTest::newRow("movie") << "/media/Amélie.(2001).1080p.BluRay.mkv" << "movie" << "Amélie" << 0 << 0;
        QTest::newRow("series") << "/media/Show.Name.S02E03.1080p.mkv" << "series" << "Show Name" << 2 << 3;
        QTest::newRow("parent") << "/media/Show/Season 1/S01E02.mkv" << "series" << "Show" << 1 << 2;
        QTest::newRow("unknown") << "/media/clip001.mp4" << "movie" << "" << 0 << 0;
        QTest::newRow("wrong-category") << "/media/Show.S01E01.mkv" << "movie" << "" << 0 << 0;
        QTest::newRow("unidentified-show") << "/media/S01E01.mkv" << "series" << "" << 1 << 1;
    }
    void parser() {
        QFETCH(QString,file); QFETCH(QString,kind); QFETCH(QString,title); QFETCH(int,season); QFETCH(int,episode);
        const auto parsed=parseFilename(file,"/media",kind);
        QCOMPARE(parsed.title,title); QCOMPARE(parsed.season,season); QCOMPARE(parsed.episode,episode);
    }
    void upgradeKeepsSettings() {
        QTemporaryDir dir; const auto path=dir.filePath("old.sqlite3");
        auto sql=QSqlDatabase::addDatabase("QSQLITE","upgrade-fixture"); sql.setDatabaseName(path); QVERIFY(sql.open());
        migrate(sql,{schemaMigrations().first()}); execute(sql,"UPDATE app_settings SET value='light' WHERE key='theme'");
        sql.close(); sql={}; QSqlDatabase::removeDatabase("upgrade-fixture");
        Database db(path); LibraryRepository repo(db);
        QCOMPARE(db.setting("theme"),"light"); QCOMPARE(repo.profileName(),"Standardprofil"); QVERIFY(repo.snapshot().value("titles").toList().isEmpty());
    }
    void modelReorderingAndSessionShuffle() {
        EntryModel model; QAbstractItemModelTester tester(&model,QAbstractItemModelTester::FailureReportingMode::QtTest);
        const QVariantMap a{{"id",1},{"title","Alpha"},{"kind","movie"}},b{{"id",2},{"title","Beta"},{"kind","movie"}},c{{"id",3},{"title","Gamma"},{"kind","movie"}};
        model.replace({a,b}); model.replace({b,a}); QCOMPARE(model.rowCount(),2); QCOMPARE(entry(&model,0).value("id").toInt(),2);
        CatalogProxy random; random.kind="movie"; random.order="random"; random.seed=123; random.setSourceModel(&model); random.refresh();
        QList<int> before; for (int i=0;i<random.rowCount();++i) before.append(entry(&random,i).value("id").toInt());
        model.replace({a,b,c}); QList<int> after; for (int i=0;i<random.rowCount();++i) if (entry(&random,i).value("id").toInt()!=3) after.append(entry(&random,i).value("id").toInt());
        QCOMPARE(after,before);
    }
    void recursiveScanVersionsUnknownAndRecovery() {
        QTemporaryDir dir;
        const auto movies=dir.filePath("Filme Ä"); QVERIFY(QDir().mkpath(movies+"/Unterordner"));
        const auto low=movies+"/Blue.Film.2020.1080p.mp4",high=movies+"/Unterordner/Blue.Film.2020.2160p.mp4";
        QVERIFY(clip(low,1920,1080)); QVERIFY(clip(high,3840,2160));
        const auto unknown=movies+"/clip001.mp4"; QVERIFY(QFile::copy(low,unknown));
        const auto corrupt=movies+"/Broken.Film.2024.mkv"; QFile bad(corrupt); QVERIFY(bad.open(QIODevice::WriteOnly)); bad.write("broken media"); bad.close();
        const auto shows=dir.filePath("Serien/Show/Season 1"); QVERIFY(QDir().mkpath(shows));
        QVERIFY(QFile::copy(low,shows+"/S01E01.mp4")); QVERIFY(QFile::copy(low,shows+"/S01E02.mp4"));
        const auto beforeLow=digest(low),beforeHigh=digest(high);
        Database db(dir.filePath("library.sqlite3")); LibraryRepository repo(db);
        repo.addSource(movies,"movie"); repo.addSource(movies+"/Unterordner","movie"); repo.addSource(dir.filePath("Serien"),"series");
        QVERIFY_EXCEPTION_THROWN(repo.addSource(movies,"series"),std::runtime_error);
        LibraryController library(db,false,false); QSignalSpy ready(&library,&LibraryController::ready);
        QVERIFY(ready.wait(5000)); QSignalSpy completed(&library,&LibraryController::scanCompleted);
        library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,20000);
        QVERIFY2(library.error().isEmpty(),qPrintable(library.error()));
        QCOMPARE(library.movieCount(),1); QCOMPARE(library.seriesCount(),1); QCOMPARE(library.unassignedCount(),2); QCOMPARE(library.errorCount(),1);
        const auto movie=entry(library.movies(),0); library.openDetails(movie.value("id").toLongLong());
        QCOMPARE(library.selectedFiles()->rowCount(),2); QCOMPARE(entry(library.selectedFiles(),0).value("height").toInt(),2160);
        QSignalSpy playback(&library,&LibraryController::playRequested); library.playTitle(movie.value("id").toLongLong());
        QCOMPARE(playback.count(),1); QCOMPARE(playback.first().first().toUrl(),QUrl::fromLocalFile(high));
        QCOMPARE(library.search("Show").size(),1); QCOMPARE(library.search("S01E").size(),0);
        auto file=repo.file(unknown); library.assignFile(file.value("id").toLongLong(),"Eigener Titel",0,0,0);
        QCOMPARE(library.movieCount(),2); QCOMPARE(library.unassignedCount(),1);
        completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,20000);
        QCOMPARE(library.movieCount(),2); QCOMPARE(repo.file(unknown).value("manual").toInt(),1); QCOMPARE(library.unassignedCount(),1);
        QVERIFY(QFile::remove(corrupt)); QVERIFY(QFile::copy(low,corrupt));
        completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,20000);
        QCOMPARE(library.errorCount(),0); QCOMPARE(library.unassignedCount(),0); QCOMPARE(library.movieCount(),3);
        QCOMPARE(digest(low),beforeLow); QCOMPARE(digest(high),beforeHigh);
    }
    void pauseInterruptionAndResume() {
        QTemporaryDir dir; const auto root=dir.filePath("media"); QVERIFY(QDir().mkpath(root));
        const auto original=dir.filePath("template.mp4"); QVERIFY(clip(original));
        for (int i=0;i<16;++i) QVERIFY(QFile::copy(original,root+QString("/Movie %1.2020.mp4").arg(i)));
        Database db(dir.filePath("db.sqlite3")); LibraryRepository repo(db); repo.addSource(root,"movie");
        {
            LibraryController library(db,false,false); QSignalSpy ready(&library,&LibraryController::ready); QVERIFY(ready.wait(5000));
            bool requested=false;
            connect(&library,&LibraryController::scanChanged,&library,[&]{ if (!requested && library.busy() && library.checked()>0) { requested=true; library.togglePause(); } });
            library.scan(); QTRY_VERIFY_WITH_TIMEOUT(library.paused(),10000);
            QTest::qWait(150); const auto paused=library.checked(); QTest::qWait(200); QCOMPARE(library.checked(),paused);
            QVERIFY(paused<16); QVERIFY(!repo.snapshot().value("titles").toList().isEmpty());
        }
        QCOMPARE(db.setting("scan_state"),"interrupted");
        LibraryController resumed(db,false,false); QSignalSpy ready(&resumed,&LibraryController::ready); QVERIFY(ready.wait(5000));
        QSignalSpy completed(&resumed,&LibraryController::scanCompleted); resumed.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,20000);
        QCOMPARE(resumed.movieCount(),16); QCOMPARE(db.setting("scan_state"),"idle");
    }
    void pauseAndResumeSameSession() {
        QTemporaryDir dir; const auto root=dir.filePath("media"); QVERIFY(QDir().mkpath(root));
        const auto original=dir.filePath("template.mp4"); QVERIFY(clip(original));
        for (int i=0;i<6;++i) QVERIFY(QFile::copy(original,root+QString("/Resume Movie %1.2021.mp4").arg(i)));
        Database db(dir.filePath("db.sqlite3")); LibraryRepository repo(db); repo.addSource(root,"movie");
        LibraryController library(db,false,false); QSignalSpy ready(&library,&LibraryController::ready); QVERIFY(ready.wait(5000));
        QSignalSpy completed(&library,&LibraryController::scanCompleted); bool requested=false;
        connect(&library,&LibraryController::scanChanged,&library,[&]{ if (!requested && library.busy() && library.checked()>0) { requested=true; library.togglePause(); } });
        library.scan(); QTRY_VERIFY_WITH_TIMEOUT(library.paused(),10000);
        library.togglePause(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,15000);
        QCOMPARE(library.movieCount(),6); QVERIFY(!library.paused()); QVERIFY(!library.busy());
    }
    void offlineSourceAndFolderWatch() {
        QTemporaryDir dir; const auto root=dir.filePath("media"); QVERIFY(QDir().mkpath(root));
        const auto original=root+"/Original.2020.mp4"; QVERIFY(clip(original));
        const auto hash=digest(original);
        Database db(dir.filePath("db.sqlite3")); LibraryRepository repo(db); repo.addSource(root,"movie");
        LibraryController library(db,false,false); QSignalSpy ready(&library,&LibraryController::ready); QVERIFY(ready.wait(5000));
        QSignalSpy completed(&library,&LibraryController::scanCompleted); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,10000);
        QTest::qWait(100); completed.clear();
        QVERIFY(QFile::copy(original,root+"/New.Movie.2021.mp4"));
        QTRY_COMPARE_WITH_TIMEOUT(library.movieCount(),2,10000); // filesystem watcher, no explicit scan
        QTRY_VERIFY_WITH_TIMEOUT(!library.busy(),10000);
        QVERIFY(QDir().rename(root,root+"-offline")); completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,10000);
        QCOMPARE(library.movieCount(),2);
        for (const auto &value : repo.snapshot().value("titles").toList()) QCOMPARE(value.toMap().value("available").toInt(),0);
        QVERIFY(QDir().rename(root+"-offline",root)); completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,10000);
        QCOMPARE(library.movieCount(),2);
        for (const auto &value : repo.snapshot().value("titles").toList()) QCOMPARE(value.toMap().value("available").toInt(),1);
        QCOMPARE(digest(original),hash);
    }
    void preferencesAndConsent() {
        QTemporaryDir dir; Database db(dir.filePath("db.sqlite3")); LibraryRepository repo(db);
        LibraryController library(db,false,false); QSignalSpy ready(&library,&LibraryController::ready); QVERIFY(ready.wait(5000));
        QVERIFY(!library.setupComplete()); QCOMPARE(library.onlineConsent(),"undecided");
        library.finishSetup(); QVERIFY(!library.setupComplete()); QVERIFY(!library.error().isEmpty()); library.clearError();
        library.setOnlineConsent("denied"); library.setMonitoring(false); library.setWizardStep(4); library.setOptions("movie","year",0); library.setTileSize(210); library.setProfileName("Mein Profil"); library.finishSetup();
        QVERIFY(library.setupComplete());
        LibraryController restarted(db,false,false); QVERIFY(!restarted.monitoring()); QCOMPARE(restarted.wizardStep(),4); QCOMPARE(restarted.onlineConsent(),"denied"); QCOMPARE(restarted.tileSize(),210); QCOMPARE(restarted.sortOrder("movie"),"year"); QCOMPARE(restarted.availability("movie"),0); QCOMPARE(restarted.profileName(),"Mein Profil");
        auto sql=QSqlDatabase::addDatabase("QSQLITE","profile-fixture"); sql.setDatabaseName(db.path()); QVERIFY(sql.open()); execute(sql,"INSERT INTO profiles(id,name) VALUES(2,'Zweites Profil')"); sql.close(); sql={}; QSqlDatabase::removeDatabase("profile-fixture");
        db.setSetting("active_profile","2"); QCOMPARE(repo.preference("tile_size","170"),"170"); repo.setPreference("tile_size","140");
        db.setSetting("active_profile","1"); QCOMPARE(repo.preference("tile_size","170"),"210");
    }
};
QTEST_GUILESS_MAIN(LibraryTest)
#include "LibraryTest.moc"
