#pragma once
#include <QProcess>
#include <QString>
#include <QFile>
#include <QCryptographicHash>
inline bool createPlayerFixture(const QString &path, bool audio = true) {
    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", {"-hide_banner", "-loglevel", "error", "-y", "-f", "lavfi", "-i", "color=c=blue:s=160x90:r=20:d=14", "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=44100:duration=14", "-vf", "drawbox=x=0:y=0:w=iw:h=ih/2:color=red:t=fill", "-c:v", "mpeg4", "-q:v", "2", audio ? "-c:a" : "-an", audio ? "aac" : "-sn", "-shortest", path});
    return ffmpeg.waitForFinished(30000) && ffmpeg.exitCode() == 0;
}
inline QByteArray fixtureHash(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}
