#include <QtTest>
#include <QTemporaryDir>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusContext>
#include <QFile>
#include "platform/MediaLocation.h"
#include "library/LibraryController.h"
#include "player/MpvEngine.h"
using namespace veluno;
class FakeKioFuse : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KIOFuse.VFS")
public:
    QString root;
    bool offline=false;
    QStringList requests;
public slots:
    QString mountUrl(const QString &address) {
        requests.append(address);
        if (offline) { sendErrorReply("org.kde.KIOFuse.Error.CannotMount","Offline"); return {}; }
        const auto path=QUrl(address).path();
        return root+path.mid(QString("/Filme").size());
    }
};
static QVariantMap first(QAbstractItemModel *model) { return model->index(0,0).data(Qt::UserRole+1).toMap(); }
class NetworkTest : public QObject {
    Q_OBJECT
private slots:
    void normalizedAddressesAndCredentials() {
        QCOMPARE(normalizedSmb(QUrl("SMB://NAS/Filme/")).toString(),QString("smb://nas/Filme"));
        QCOMPARE(childLocation("smb://nas/Filme","Unterordner/Ä & #.mp4"),QString("smb://nas/Filme/Unterordner/%C3%84%20&%20%23.mp4"));
        for (const auto &text : {"smb://nas","smb://user:secret@nas/Filme","smb://user@nas/Filme","smb://nas/Filme?password=secret","smb://nas/Filme/../Other","https://nas/Filme"})
            QVERIFY_EXCEPTION_THROWN(normalizedSmb(QUrl(text)),std::runtime_error);
        QTemporaryDir dir; Database db(dir.filePath("db")); LibraryRepository repo(db);
        QVERIFY_EXCEPTION_THROWN(repo.addSource("smb://user:secret@nas/Filme","movie"),std::runtime_error);
        QCOMPARE(repo.sources().size(),0);
        repo.addSource("smb://NAS/Filme/","movie"); repo.addSource("smb://nas/Filme","movie"); QCOMPARE(repo.sources().size(),1);
        QVERIFY_EXCEPTION_THROWN(repo.addSource("smb://nas/Filme/Unterordner","series"),std::runtime_error);
    }
    void absentServiceReturnsActionableError() {
        QString error; bool finished=false;
        NetworkAccess::resolve(QUrl("smb://nas/Filme"),this,[&](QString path,QString message){ QVERIFY(path.isEmpty()); error=message; finished=true; });
        QTRY_VERIFY_WITH_TIMEOUT(finished,5000); QVERIFY(error.contains("kio-fuse"));
    }
    void scanPlaybackRemountOfflineAndManualAssignment() {
        QTemporaryDir dir; const auto root=dir.filePath("first-mount"); QVERIFY(QDir().mkpath(root+"/Unterordner"));
        const auto relative=QString("/Unterordner/Ä.Film.2020.mp4"),unknown=QString("/clip001.mp4");
        QProcess ffmpeg;
        ffmpeg.start("ffmpeg",{"-hide_banner","-loglevel","error","-f","lavfi","-i","color=c=blue:s=96x64:r=1:d=1","-frames:v","1","-an","-c:v","mpeg4",root+relative});
        QVERIFY(ffmpeg.waitForFinished(15000)); QCOMPARE(ffmpeg.exitCode(),0); QVERIFY(QFile::copy(root+relative,root+unknown));
        FakeKioFuse service; service.root=root;
        auto bus=QDBusConnection::sessionBus(); QVERIFY(bus.isConnected());
        QVERIFY(bus.registerService("org.kde.KIOFuse"));
        QVERIFY(bus.registerObject("/org/kde/KIOFuse",&service,QDBusConnection::ExportAllSlots));
        Database db(dir.filePath("db")); LibraryRepository repo(db);
        LibraryController library(db,false,false); QSignalSpy ready(&library,&LibraryController::ready); QVERIFY(ready.wait(5000));
        QVERIFY(library.addSource(QUrl("smb://nas/Filme"),"movie"));
        QSignalSpy completed(&library,&LibraryController::scanCompleted); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,15000);
        QCOMPARE(library.movieCount(),1); QCOMPARE(library.unassignedCount(),1); QCOMPARE(library.errorCount(),0);
        const auto title=first(library.movies()); const auto id=title.value("id").toLongLong(); library.openDetails(id);
        QCOMPARE(first(library.selectedFiles()).value("path").toString(),childLocation("smb://nas/Filme",relative.mid(1)));
        const auto unresolved=repo.file(childLocation("smb://nas/Filme",unknown.mid(1))); library.assignFile(unresolved.value("id").toLongLong(),"Mein Film",0,0,0);
        QSignalSpy playing(&library,&LibraryController::playRequested);
        MpvEngine player(nullptr,true);
        connect(&library,&LibraryController::playRequested,&player,&MpvEngine::openFile);
        library.playTitle(id); QVERIFY(library.networkBusy()); QTRY_COMPARE_WITH_TIMEOUT(playing.count(),1,5000);
        QCOMPARE(playing.first().first().toUrl(),QUrl::fromLocalFile(root+relative)); QVERIFY(!library.networkBusy());
        QVERIFY(playing.first().at(1).toBool()); QTRY_VERIFY_WITH_TIMEOUT(!player.loading(),5000); QVERIFY2(player.error().isEmpty(),qPrintable(player.error())); player.stop();
        playing.clear(); library.playTitle(id); library.cancelPendingPlayback(); QTest::qWait(100); QCOMPARE(playing.count(),0);
        const auto remount=dir.filePath("second-mount"); QVERIFY(QDir().rename(root,remount)); service.root=remount;
        completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,15000);
        QCOMPARE(library.movieCount(),2); QCOMPARE(library.unassignedCount(),0); QCOMPARE(repo.file(childLocation("smb://nas/Filme",unknown.mid(1))).value("manual").toInt(),1);
        library.playTitle(id); QTRY_COMPARE_WITH_TIMEOUT(playing.count(),1,5000); QCOMPARE(playing.first().first().toUrl(),QUrl::fromLocalFile(remount+relative));
        service.offline=true; completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,15000);
        QCOMPARE(library.movieCount(),2); QVERIFY(library.errorCount()>0);
        for (const auto &value : repo.snapshot().value("titles").toList()) QCOMPARE(value.toMap().value("available").toInt(),0);
        playing.clear(); library.playTitle(id); QTRY_VERIFY_WITH_TIMEOUT(!library.networkBusy(),5000); QCOMPARE(playing.count(),0); QVERIFY(!library.error().isEmpty());
        service.offline=false; library.clearError(); completed.clear(); library.scan(); QTRY_VERIFY_WITH_TIMEOUT(completed.count()>0,15000);
        QCOMPARE(library.errorCount(),0); QCOMPARE(library.movieCount(),2);
        bus.unregisterObject("/org/kde/KIOFuse"); bus.unregisterService("org.kde.KIOFuse");
    }
};
QTEST_GUILESS_MAIN(NetworkTest)
#include "NetworkTest.moc"
