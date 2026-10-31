#include "FilenameParser.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
namespace veluno {
namespace {
QString clean(QString value) {
    value.replace(QRegularExpression("[._]+"), " ");
    return value.simplified();
}
bool seasonFolder(const QString &name) {
    return QRegularExpression("^(?:(?:season|staffel|s)\\s*0*\\d+|specials|extras)$",
                              QRegularExpression::CaseInsensitiveOption)
        .match(name)
        .hasMatch();
}
} // namespace
ScannedFile parseFilename(const QString &path, const QString &root, const QString &kind) {
    ScannedFile result;
    result.path = path;
    result.kind = kind;
    auto name = clean(QFileInfo(path).completeBaseName());
    const auto flags = QRegularExpression::CaseInsensitiveOption;
    const QRegularExpression episode(
        "\\b(?:S(\\d{1,2})[ -]*E(\\d{1,3})(?:E\\d+)?|(\\d{1,2})x(\\d{1,3})|"
        "Staffel[ -]*(\\d{1,2})[ -]*(?:Episode|Folge)[ -]*(\\d{1,3}))\\b",
        flags);
    auto match = episode.match(name);
    if (kind == "series") {
        if (match.hasMatch()) {
            for (int group : {1, 3, 5})
                if (!match.captured(group).isEmpty()) {
                    result.season = match.captured(group).toInt();
                    result.episode = match.captured(group + 1).toInt();
                    break;
                }
            name = name.left(match.capturedStart()).trimmed();
        } else {
            // An episode-only filename is accepted only with an explicit season folder.
            const auto parentName = clean(QFileInfo(path).dir().dirName());
            const auto folder =
                QRegularExpression("^(?:season|staffel|s)\\s*(\\d{1,2})$", flags).match(parentName);
            const auto number =
                QRegularExpression("^(?:E|Episode|Folge)[ -]*(\\d{1,3})(?:\\b|$)", flags)
                    .match(name);
            if (!folder.hasMatch() || !number.hasMatch())
                return result;
            result.season = folder.captured(1).toInt();
            result.episode = number.captured(1).toInt();
            name.clear();
        }
        if (result.episode < 1)
            return result;
        if (name.isEmpty() || seasonFolder(name)) {
            auto parent = QFileInfo(path).dir();
            const auto boundary = QDir(root).absolutePath();
            while (parent.absolutePath() != boundary && seasonFolder(clean(parent.dirName()))) {
                const auto next = QFileInfo(parent.absolutePath()).dir();
                if (next.absolutePath() == parent.absolutePath())
                    break;
                parent = next;
            }
            name = parent.absolutePath() == boundary ? QString{} : clean(parent.dirName());
        }
    } else if (match.hasMatch())
        return result;
    auto years = QRegularExpression("\\b((?:19|20)\\d{2})\\b").globalMatch(name);
    int yearStart = -1;
    while (years.hasNext()) {
        const auto year = years.next();
        auto prefix = name.left(year.capturedStart());
        auto numeric = prefix;
        numeric.remove(QRegularExpression("[()\\[\\] ]"));
        if (prefix.contains(QRegularExpression("\\p{L}")) ||
            QRegularExpression("^\\d{4}$").match(numeric).hasMatch()) {
            result.year = year.captured(1).toInt();
            yearStart = year.capturedStart();
        }
    }
    if (yearStart >= 0)
        name = name.left(yearStart);
    name.replace(QRegularExpression("\\b(?:2160p|1080p|720p|480p|4k|uhd|bluray|bdrip|webrip|web[- "
                                    "]?dl|hdtv|x26[45]|h26[45]|hevc|avc)\\b.*$",
                                    flags),
                 "");
    name.remove(QRegularExpression("[()\\[\\]]"));
    name = name.trimmed();
    while (name.endsWith('-') || name.endsWith(' '))
        name.chop(1);
    name = name.simplified();
    if (name.isEmpty() || seasonFolder(name) ||
        QRegularExpression("^(?:video|sample|clip|unknown|unbekannt|film)\\s*\\d*$", flags)
            .match(name)
            .hasMatch() ||
        (!name.contains(QRegularExpression("\\p{L}")) &&
         !QRegularExpression("^\\d{4}$").match(name).hasMatch()))
        return result;
    result.title = name;
    return result;
}
} // namespace veluno
