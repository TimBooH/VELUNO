#include "Scanner.h"
#include "platform/MediaLocation.h"
#include "FilenameParser.h"
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDateTime>
#include <QProcess>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QSet>
namespace veluno {
ScannerWorker::ScannerWorker(QString path) : m_path(std::move(path)) {}
void ScannerWorker::initialize() {
    try {
        m_database=std::make_unique<Database>(m_path);
        m_repository=std::make_unique<LibraryRepository>(*m_database);
        emit snapshotReady(m_repository->snapshot()); emit ready();
    } catch (const std::exception &e) { emit failed(QString::fromUtf8(e.what())); }
}
void ScannerWorker::prepare() { QMutexLocker lock(&m_mutex); m_cancel=false; m_paused=false; }
void ScannerWorker::setPaused(bool paused) { QMutexLocker lock(&m_mutex); m_paused=paused; if (!paused) m_condition.wakeAll(); }
void ScannerWorker::cancel() { QMutexLocker lock(&m_mutex); m_cancel=true; m_condition.wakeAll(); }
bool ScannerWorker::checkpoint() {
    QMutexLocker lock(&m_mutex);
    while (m_paused && !m_cancel) m_condition.wait(&m_mutex);
    return !m_cancel;
}
bool ScannerWorker::probe(ScannedFile &file,const QString &localPath) {
    auto executable=QCoreApplication::applicationDirPath()+"/ffprobe";
    if (!QFileInfo::exists(executable)) executable=QStandardPaths::findExecutable("ffprobe");
    if (executable.isEmpty()) { file.diagnostic="ffprobe wurde nicht gefunden. Bitte die FFmpeg-Installation prüfen."; return false; }
    QProcess process;
    process.start(executable,{"-v","error","-select_streams","v:0","-show_entries","stream=width,height:format=duration","-of","json",localPath});
    int remaining=10000;
    while (process.state()!=QProcess::NotRunning) {
        if (!checkpoint()) { process.kill(); process.waitForFinished(1000); return false; }
        if (process.waitForFinished(50)) break;
        if ((remaining-=50)<=0) { process.kill(); process.waitForFinished(1000); file.diagnostic="Medienanalyse dauerte zu lange. Datei oder Laufwerk prüfen und erneut versuchen."; return false; }
    }
    if (process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0 || process.error()==QProcess::FailedToStart) {
        file.diagnostic="Medienanalyse fehlgeschlagen: "+QString::fromUtf8(process.readAllStandardError()).trimmed().left(600);
        if (process.error()==QProcess::FailedToStart) file.diagnostic="ffprobe konnte nicht gestartet werden: "+process.errorString();
        return false;
    }
    const auto data=QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    const auto streams=data.value("streams").toArray();
    if (streams.isEmpty()) { file.diagnostic="Keine unterstützte Videospur gefunden."; return false; }
    file.width=streams.first().toObject().value("width").toInt(); file.height=streams.first().toObject().value("height").toInt();
    file.duration=data.value("format").toObject().value("duration").toString().toDouble();
    if (file.width<=0 || file.height<=0) { file.diagnostic="Auflösung konnte nicht ermittelt werden."; return false; }
    return true;
}
void ScannerWorker::scan(qint64 onlySource) {
    if (!m_repository) { emit failed("Scanner ist noch nicht bereit."); emit finished(true); return; }
    try {
        m_database->setSetting("scan_state","running");
        const QString token=QUuid::createUuid().toString();
        const QSet<QString> extensions={"mkv","mp4","m4v","avi","mov","webm","mpeg","mpg","ts","m2ts","wmv","ogv"};
        struct Candidate { QString path, root, kind, identity; qint64 source; };
        QList<Candidate> candidates;
        QList<qint64> completeRoots;
        QStringList watched;
        QSet<QString> discovered;
        QElapsedTimer progressClock; progressClock.start();
        for (const auto &value : m_repository->sources(true)) {
            if (!checkpoint()) break;
            const auto source=value.toMap(); const auto id=source.value("id").toLongLong();
            if (onlySource && id!=onlySource) continue;
            const auto identity=source.value("path").toString(), kind=source.value("kind").toString();
            QString root=identity;
            if (isSmb(identity)) {
                emit progress(0,candidates.size(),identity);
                QString error;
                root=NetworkAccess::resolveForScan(QUrl(identity),m_cancel,error);
                if (m_cancel) break;
                if (root.isEmpty()) {
                    m_repository->markSourceUnavailable(id);
                    m_repository->error(id,identity,error,token); continue;
                }
            }
            if (!QFileInfo(root).isDir() || !QFileInfo(root).isReadable()) {
                m_repository->markSourceUnavailable(id);
                m_repository->error(id,identity,"Ordner oder Laufwerk ist nicht verfügbar. Einträge bleiben erhalten.",token); continue;
            }
            QStringList pending{root}; bool complete=true;
            while (!pending.isEmpty() && checkpoint()) {
                const auto directory=pending.takeLast();
                if (!QFileInfo(directory).isReadable()) { complete=false; m_repository->error(id,isSmb(identity) ? childLocation(identity,QDir(root).relativeFilePath(directory)) : directory,"Unterordner ist nicht lesbar. Zugriffsrechte prüfen.",token); continue; }
                if (!isSmb(identity)) watched.append(directory);
                if (progressClock.elapsed()>50) { emit progress(0,candidates.size(),directory); progressClock.restart(); }
                const auto entries=QDir(directory).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden,QDir::Name);
                for (const auto &info : entries) {
                    if (!checkpoint()) break;
                    if (info.isDir()) { if (!info.isSymLink()) pending.append(info.absoluteFilePath()); continue; }
                    if (!info.isFile() || !extensions.contains(info.suffix().toLower())) continue;
                    const auto path=isSmb(identity) ? info.absoluteFilePath() : info.canonicalFilePath();
                    const auto key=isSmb(identity) ? childLocation(identity,QDir(root).relativeFilePath(path)) : path;
                    if (path.isEmpty() || discovered.contains(key)) continue;
                    discovered.insert(key); candidates.append({path,root,kind,key,id});
                }
            }
            if (complete && !m_cancel) completeRoots.append(id);
        }
        emit directoriesFound(watched);
        emit snapshotReady(m_repository->snapshot());
        QElapsedTimer updates; updates.start(); int checked=0;
        for (const auto &candidate : candidates) {
            if (m_paused) emit snapshotReady(m_repository->snapshot());
            if (!checkpoint()) break;
            auto file=parseFilename(candidate.path,candidate.root,candidate.kind); file.sourceId=candidate.source; file.path=candidate.identity;
            const QFileInfo info(candidate.path);
            file.size=info.size(); file.modified=info.lastModified().toUTC().toString(Qt::ISODateWithMs);
            const auto previous=m_repository->file(file.path);
            if (!info.exists() || !info.isReadable()) {
                file.available=false; file.diagnostic="Datei ist nicht lesbar oder nicht mehr verfügbar.";
                m_repository->record(file,token);
                m_repository->error(file.sourceId,file.path,file.diagnostic,token);
            } else {
                if (previous.value("size").toLongLong()==file.size && previous.value("modified").toString()==file.modified && previous.value("diagnostic").toString().isEmpty() && previous.value("width").toInt()>0) {
                    file.width=previous.value("width").toInt(); file.height=previous.value("height").toInt(); file.duration=previous.value("duration").toDouble();
                } else probe(file,candidate.path);
                if (m_cancel) break;
                m_repository->record(file,token);
                if (!file.diagnostic.isEmpty()) m_repository->error(file.sourceId,file.path,file.diagnostic,token);
            }
            ++checked;
            if (checked==1 || checked==candidates.size() || progressClock.elapsed()>50) { emit progress(checked,candidates.size(),file.path); progressClock.restart(); }
            if (updates.elapsed()>400) { emit snapshotReady(m_repository->snapshot()); updates.restart(); }
        }
        if (!m_cancel) for (auto id : completeRoots) { m_repository->markMissing(id,token); m_repository->clearOldErrors(id,token); }
        m_database->setSetting("scan_state",m_cancel ? "interrupted" : "idle");
        emit snapshotReady(m_repository->snapshot()); emit finished(m_cancel);
    } catch (const std::exception &e) {
        try { m_database->setSetting("scan_state","interrupted"); } catch (...) {}
        emit failed(QString::fromUtf8(e.what())); emit finished(true);
    }
}
}
