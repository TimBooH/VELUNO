#include "MediaLocation.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <stdexcept>
namespace veluno {
QUrl normalizedSmb(const QUrl &input) {
    if (!input.isValid() || input.scheme().compare("smb",Qt::CaseInsensitive)!=0 || input.host().isEmpty() || input.path().split('/',Qt::SkipEmptyParts).isEmpty())
        throw std::runtime_error("Bitte eine SMB-Adresse mit Server und Freigabe angeben, zum Beispiel smb://NAS/Filme.");
    if (!input.userInfo().isEmpty()) throw std::runtime_error("Bitte keine Zugangsdaten in der Adresse angeben. Melde dich über Dolphin/KDE an.");
    if (input.hasQuery() || input.hasFragment() || input.path().split('/').contains("..") || input.path().split('/').contains("."))
        throw std::runtime_error("Die SMB-Adresse enthält ungültige Pfadbestandteile. Bitte Leerzeichen und Sonderzeichen als URL eingeben.");
    auto result=input; result.setScheme("smb");
    auto path=result.path(); while (path.endsWith('/')) path.chop(1);
    result.setPath(path); return result;
}
bool isSmb(const QString &path) { return QUrl(path).scheme().compare("smb",Qt::CaseInsensitive)==0; }
QString childLocation(const QString &root,const QString &relative) {
    if (!isSmb(root)) return QDir(root).filePath(relative);
    auto url=normalizedSmb(QUrl(root)); url.setPath(url.path()+"/"+relative);
    return url.toString(QUrl::FullyEncoded);
}
void NetworkAccess::resolve(const QUrl &input,QObject *context,Completion completion) {
    QUrl url;
    try { url=normalizedSmb(input); }
    catch (const std::exception &e) { completion({},QString::fromUtf8(e.what())); return; }
    const auto bus=QDBusConnection::sessionBus();
    if (!bus.isConnected()) { completion({},"Keine KDE-Sitzungsverbindung verfügbar. SMB benötigt KIO-FUSE in deiner Desktop-Sitzung."); return; }
    auto message=QDBusMessage::createMethodCall("org.kde.KIOFuse","/org/kde/KIOFuse","org.kde.KIOFuse.VFS","mountUrl");
    message << url.toString(QUrl::FullyEncoded);
    auto *watcher=new QDBusPendingCallWatcher(bus.asyncCall(message,30000),context);
    QObject::connect(watcher,&QDBusPendingCallWatcher::finished,context,[watcher,completion=std::move(completion)] {
        const QDBusPendingReply<QString> reply=*watcher;
        watcher->deleteLater();
        if (reply.isError()) {
            const auto name=reply.error().name();
            if (name=="org.freedesktop.DBus.Error.ServiceUnknown" || name=="org.freedesktop.DBus.Error.Spawn.ServiceNotFound")
                completion({},"KIO-FUSE ist nicht verfügbar. Auf CachyOS bitte kio-fuse und kio-extras installieren und die SMB-Freigabe in Dolphin öffnen.");
            else completion({},"SMB-Freigabe nicht erreichbar oder Anmeldung erforderlich. Bitte die Adresse und Netzwerkverbindung prüfen, die Freigabe in Dolphin öffnen und erneut scannen.");
            return;
        }
        const auto path=reply.value();
        if (!QDir::isAbsolutePath(path) || path.contains(QChar::Null)) { completion({},"KIO-FUSE lieferte keinen gültigen lokalen Zugriffspfad."); return; }
        completion(path,{});
    });
}
QString NetworkAccess::resolveForScan(const QUrl &url,const std::atomic_bool &cancel,QString &error) {
    QEventLoop loop; QObject context; QString path; bool finished=false;
    resolve(url,&context,[&](QString resolved,QString failure){ path=resolved; error=failure; finished=true; loop.quit(); });
    QTimer cancellation; cancellation.setInterval(50);
    QObject::connect(&cancellation,&QTimer::timeout,&loop,[&]{ if (cancel) loop.quit(); });
    cancellation.start(); if (!finished && !cancel) loop.exec();
    return cancel ? QString{} : path;
}
}
