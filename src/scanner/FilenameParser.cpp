#include "FilenameParser.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
namespace veluno {
ScannedFile parseFilename(const QString &path, const QString &root, const QString &kind) {
    ScannedFile result; result.path=path; result.kind=kind;
    auto name=QFileInfo(path).completeBaseName();
    name.replace(QRegularExpression("[._]+")," ");
    const QRegularExpression episode("\\bS(\\d{1,2})E(\\d{1,3})(?:E\\d+)?\\b",QRegularExpression::CaseInsensitiveOption);
    const auto match=episode.match(name);
    if (kind=="series") {
        if (!match.hasMatch()) return result;
        result.season=match.captured(1).toInt(); result.episode=match.captured(2).toInt();
        if (result.episode<1) return result;
        name=name.left(match.capturedStart()).trimmed();
        if (name.isEmpty()) {
            auto parent=QFileInfo(path).dir();
            if (QRegularExpression("^(season|staffel|s)\\s*\\d+$",QRegularExpression::CaseInsensitiveOption).match(parent.dirName()).hasMatch()) parent=QFileInfo(parent.absolutePath()).dir();
            if (parent.absolutePath()!=QDir(root).absolutePath()) name=parent.dirName();
        }
    } else if (match.hasMatch()) return result;
    const auto year=QRegularExpression("\\b((?:19|20)\\d{2})\\b").match(name);
    if (year.hasMatch()) { result.year=year.captured(1).toInt(); name=name.left(year.capturedStart()); }
    name.replace(QRegularExpression("\\b(?:2160p|1080p|720p|480p|4k|uhd|bluray|bdrip|webrip|web[- ]?dl|hdtv|x26[45]|h26[45]|hevc|avc)\\b.*$",QRegularExpression::CaseInsensitiveOption),"");
    name.remove(QRegularExpression("[()\\[\\]]")); name=name.trimmed();
    while (name.endsWith('-') || name.endsWith(' ')) name.chop(1);
    name=name.simplified();
    if (name.isEmpty() || !name.contains(QRegularExpression("\\p{L}")) || QRegularExpression("^(?:video|sample|clip|unknown|unbekannt|film)\\s*\\d*$",QRegularExpression::CaseInsensitiveOption).match(name).hasMatch()) return result;
    result.title=name;
    return result;
}
}
