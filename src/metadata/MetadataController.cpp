#include "MetadataController.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrlQuery>
#include <algorithm>
namespace veluno {
static QString matchKey(QString value) {
    value = value.normalized(QString::NormalizationForm_KC).toCaseFolded();
    value.remove(QRegularExpression("[^\\p{L}\\p{N}]+"));
    return value;
}
MetadataController::MetadataController(Database &db, LibraryController &library,
                                       PersonalController &personal, QObject *parent,
                                       const QUrl &api, const QUrl &imageBase)
    : QObject(parent), m_db(db), m_library(library), m_personal(personal), m_api(api),
      m_imageBase(imageBase), m_imageWorker(new QObject) {
    m_imageWorker->moveToThread(&m_imageThread);
    connect(&m_imageThread, &QThread::finished, m_imageWorker, &QObject::deleteLater);
    m_imageThread.start();
    m_pump.setInterval(550);
    connect(&m_pump, &QTimer::timeout, this, &MetadataController::pump);
    m_pump.start();
    m_refresh.setInterval(3600000);
    connect(&m_refresh, &QTimer::timeout, this, &MetadataController::refreshAll);
    m_refresh.start();
    connect(&library, &LibraryController::setupChanged, this, [this] {
        if (!allowed()) {
            ++m_generation;
            m_requests.clear();
            m_imageRequests.clear();
            m_pendingTitles.clear();
            const auto pending = m_replies;
            for (auto *r : pending)
                r->abort();
        } else
            QTimer::singleShot(2000, this, &MetadataController::refreshAll);
        emit changed();
    });
    connect(&library, &LibraryController::catalogChanged, this, &MetadataController::changed);
    connect(&library, &LibraryController::scanCompleted, this,
            [this](bool) { QTimer::singleShot(1000, this, &MetadataController::refreshAll); });
    QTimer::singleShot(3000, this, &MetadataController::refreshAll);
}
MetadataController::~MetadataController() {
    ++m_generation;
    m_pump.stop();
    m_refresh.stop();
    m_requests.clear();
    m_imageRequests.clear();
    const auto replies = m_replies;
    for (auto *reply : replies) {
        reply->disconnect(this);
        reply->abort();
    }
    m_imageThread.quit();
    m_imageThread.wait();
}
bool MetadataController::allowed() const {
    return configured() && enabled();
}
void MetadataController::setEnabled(bool value) {
    const QString consent = value ? "granted" : "denied";
    if (m_db.setting("online_consent") == consent)
        return;
    // One persisted choice is shared with the wizard; never grant consent merely
    // because a token was entered. setupChanged cancels pending requests on opt-out.
    m_library.setOnlineConsent(consent);
    if (m_db.setting("online_consent") != consent) {
        report("Die TMDB-Einstellung konnte nicht gespeichert werden.");
        return;
    }
    report(!value ? "TMDB ausgeschaltet. Gespeicherte Informationen bleiben verfügbar."
           : configured()
               ? "TMDB eingeschaltet. Metadaten werden im Hintergrund ergänzt."
               : "TMDB eingeschaltet. Bitte noch den API-Lesezugriffstoken hinterlegen.");
}
void MetadataController::report(const QString &error) {
    m_status = error;
    emit changed();
}
void MetadataController::setToken(const QString &token) {
    try {
        if (token.size() > 16384 || token.contains('\n') || token.contains('\r')) {
            report("Ungültiger API-Zugang.");
            return;
        }
        m_db.setSetting("tmdb_token", token.trimmed());
        ++m_generation;
        m_requests.clear();
        m_imageRequests.clear();
        m_pendingTitles.clear();
        m_results.clear();
        const auto replies = m_replies;
        for (auto *reply : replies)
            reply->abort();
        report(token.trimmed().isEmpty() ? "TMDB-Zugang entfernt." : "TMDB-Zugang gespeichert.");
        if (allowed())
            refreshAll();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void MetadataController::request(const QString &path, std::function<void(QJsonObject)> done) {
    if (!allowed()) {
        report("Für TMDB sind eine ausdrückliche Metadaten-Zustimmung und ein "
               "API-Lesezugriffstoken erforderlich.");
        return;
    }
    m_requests.enqueue({path, std::move(done)});
    emit changed();
}
void MetadataController::pump() {
    if (!allowed() || m_replies.size() >= 2)
        return;
    if (!m_imageRequests.isEmpty()) {
        const auto work = m_imageRequests.dequeue();
        downloadImage(work.title, work.kind, work.remote);
        return;
    }
    if (m_requests.isEmpty())
        return;
    const auto work = m_requests.dequeue();
    const auto generation = m_generation;
    QUrl url(m_api.toString() + work.path);
    QNetworkRequest request(url);
    request.setTransferTimeout(20000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    request.setRawHeader("Authorization", "Bearer " + m_db.setting("tmdb_token").toUtf8());
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network.get(request);
    m_replies.insert(reply);
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 4 * 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, work, generation] {
        m_replies.remove(reply);
        if (generation != m_generation || !allowed()) {
            reply->deleteLater();
            emit changed();
            return;
        }
        const auto bytes = reply->readAll();
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError || bytes.size() > 4 * 1024 * 1024) {
            m_pendingTitles.clear();
            if (status == 401 || status == 403) {
                ++m_generation;
                m_requests.clear();
                m_imageRequests.clear();
                const auto remaining = m_replies;
                for (auto *pending : remaining)
                    pending->abort();
            }
            if (status == 429) {
                m_requests.clear();
                m_imageRequests.clear();
                m_pump.stop();
                const auto delay = std::clamp(reply->rawHeader("Retry-After").toInt(), 60, 3600);
                QTimer::singleShot(delay * 1000, this, [this] {
                    m_pump.start();
                    refreshAll();
                });
            }
            report(
                (status == 401 || status == 403)
                    ? "TMDB-Zugang abgewiesen. Bitte API-Lesezugriffstoken prüfen."
                : status == 429
                    ? "TMDB begrenzt die Abfragen. Die Aktualisierung wird später erneut versucht."
                    : "TMDB ist nicht erreichbar. Lokal gespeicherte Informationen bleiben "
                      "erhalten.");
            return;
        }
        QJsonParseError error;
        const auto json = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError || !json.isObject()) {
            report("TMDB lieferte keine gültigen Daten.");
            return;
        }
        try {
            work.done(json.object());
        } catch (const std::exception &e) {
            report(QString::fromUtf8(e.what()));
        }
        emit changed();
    });
    emit changed();
}
void MetadataController::search(qint64 title, const QString &query) {
    m_selected = title;
    m_results.clear();
    lookup(title, true, query);
}
void MetadataController::lookup(qint64 title, bool interactive, const QString &query) {
    const auto titles = m_db.query("SELECT * FROM media_titles WHERE id=?", {title});
    if (titles.isEmpty())
        return;
    const auto t = titles.first().toMap();
    const auto search = query.trimmed().isEmpty() ? t.value("title").toString() : query.trimmed();
    const auto kind = t.value("kind") == "series" ? "tv" : "movie";
    QUrlQuery params;
    params.addQueryItem("query", search);
    params.addQueryItem("language", m_db.setting("language", "de") == "en" ? "en-US" : "de-DE");
    params.addQueryItem("include_adult", "false");
    request(QString("/search/%1?").arg(kind) + params.toString(QUrl::FullyEncoded),
            [this, title, t, interactive, search](QJsonObject json) {
                QVariantList hits;
                QVariantList exact;
                for (const auto &value : json.value("results").toArray()) {
                    const auto item = value.toObject();
                    const auto name = item.value("title").toString(item.value("name").toString());
                    const auto year = item.value("release_date")
                                          .toString(item.value("first_air_date").toString())
                                          .left(4)
                                          .toInt();
                    const QVariantMap hit{{"id", item.value("id").toInteger()},
                                          {"title", name},
                                          {"year", year},
                                          {"overview", item.value("overview").toString()}};
                    hits.append(hit);
                    if (matchKey(name) == matchKey(search) &&
                        (!t.value("year").toInt() || t.value("year").toInt() == year))
                        exact.append(hit);
                }
                if (interactive) {
                    if (m_selected == title) {
                        m_results = hits;
                        report(hits.isEmpty() ? "Keine Treffer gefunden."
                                              : "Bitte den passenden Treffer auswählen.");
                    }
                } else if (exact.size() == 1) {
                    fetch(title, exact.first().toMap().value("id").toLongLong());
                    return;
                }
                m_db.query("INSERT INTO media_details(title_id,fetched_at) VALUES(?,?) ON "
                           "CONFLICT(title_id) DO UPDATE SET fetched_at=excluded.fetched_at",
                           {title, QDateTime::currentDateTimeUtc().toString(Qt::ISODate)});
                m_pendingTitles.remove(title);
                if (m_pendingTitles.isEmpty() && !interactive)
                    QTimer::singleShot(1000, this, &MetadataController::refreshAll);
            });
}
void MetadataController::choose(qint64 providerId) {
    if (m_selected && providerId > 0)
        fetch(m_selected, providerId);
}
void MetadataController::fetch(qint64 title, qint64 provider) {
    const auto titles = m_db.query("SELECT * FROM media_titles WHERE id=?", {title});
    if (titles.isEmpty())
        return;
    const auto kind = titles.first().toMap().value("kind") == "series" ? "tv" : "movie";
    request(
        QString("/%1/%2?language=%3&append_to_response=images&include_image_language=de,en,null")
            .arg(kind)
            .arg(provider)
            .arg(m_db.setting("language", "de") == "en" ? "en-US" : "de-DE"),
        [this, title, provider](QJsonObject json) {
            if (json.value("id").toInteger() != provider ||
                json.value("title").toString(json.value("name").toString()).isEmpty()) {
                report("TMDB lieferte keinen gültigen Titel.");
                m_pendingTitles.remove(title);
                return;
            }
            QStringList genres;
            for (const auto &v : json.value("genres").toArray())
                genres.append(v.toObject().value("name").toString());
            m_db.transaction([&] {
                m_db.query("INSERT INTO "
                           "media_details(title_id,provider_id,overview,genres,rating,fetched_at) "
                           "VALUES(?,?,?,?,?,?) ON CONFLICT(title_id) DO UPDATE SET "
                           "provider_id=excluded.provider_id,overview=CASE WHEN "
                           "media_details.manual_overview=1 THEN media_details.overview ELSE "
                           "excluded.overview "
                           "END,genres=excluded.genres,rating=excluded.rating,fetched_at=excluded."
                           "fetched_at",
                           {title, provider, json.value("overview").toString(), genres.join(", "),
                            json.value("vote_average").toDouble(),
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODate)});
                const auto details =
                    m_db.query("SELECT manual_title FROM media_details WHERE title_id=?", {title})
                        .first()
                        .toMap();
                const auto name = json.value("title").toString(json.value("name").toString());
                if (!details.value("manual_title").toBool() && !name.isEmpty())
                    m_db.query("UPDATE media_titles SET title=? WHERE id=?", {name, title});
            });
            m_db.query("UPDATE media_images SET is_primary=0 WHERE title_id=?", {title});
            const auto variants = json.value("images").toObject();
            for (const QString &kind : {QString("poster"), QString("backdrop")}) {
                QStringList paths;
                const auto primary = json.value(kind + "_path").toString();
                if (!primary.isEmpty())
                    paths.append(primary);
                for (const auto &v : variants.value(kind + "s").toArray()) {
                    const auto path = v.toObject().value("file_path").toString();
                    if (!paths.contains(path))
                        paths.append(path);
                    if (paths.size() >= 6)
                        break;
                }
                for (int index = 0; index < paths.size(); ++index)
                    image(title, kind, paths[index], index == 0);
            }
            m_pendingTitles.remove(title);
            if (m_pendingTitles.isEmpty())
                QTimer::singleShot(1000, this, &MetadataController::refreshAll);
            m_library.reload();
            report("Metadaten lokal gespeichert.");
            if (m_selected == title)
                showImages(title);
        });
}
QVariantMap MetadataController::statistics() const {
    const auto rows =
        m_db.query("SELECT COUNT(*) AS total,COALESCE(SUM(d.provider_id>0),0) AS matched,"
                   "COALESCE(SUM(d.poster<>''),0) AS posters FROM media_titles t LEFT JOIN "
                   "media_details d ON d.title_id=t.id WHERE EXISTS(SELECT 1 FROM media_files f "
                   "WHERE f.title_id=t.id)");
    return rows.isEmpty() ? QVariantMap{} : rows.first().toMap();
}
void MetadataController::testAccess() {
    if (!allowed()) {
        report(
            "TMDB einschalten und einen API-Lesezugriffstoken speichern, um den Zugang zu prüfen.");
        return;
    }
    report("TMDB-Zugang wird geprüft …");
    request("/configuration", [this](QJsonObject json) {
        const auto url = QUrl(json.value("images").toObject().value("secure_base_url").toString());
        report(url.scheme() == "https" && !url.host().isEmpty()
                   ? "TMDB-Zugang erfolgreich geprüft. Titel und Bilder können abgerufen werden."
                   : "TMDB lieferte keine gültige Konfiguration. Bitte später erneut prüfen.");
    });
}
void MetadataController::retryMissingArtwork() {
    if (!allowed())
        return;
    for (const auto &value : m_db.query(
             "SELECT title_id,provider_id FROM media_details WHERE provider_id>0 AND "
             "EXISTS(SELECT 1 FROM media_files f WHERE f.title_id=media_details.title_id) AND "
             "(poster='' OR backdrop='' OR EXISTS(SELECT 1 FROM media_images i WHERE "
             "i.title_id=media_details.title_id AND i.local_path='')) LIMIT 50")) {
        const auto row = value.toMap();
        const auto title = row.value("title_id").toLongLong();
        if (m_pendingTitles.contains(title))
            continue;
        m_pendingTitles.insert(title);
        fetch(title, row.value("provider_id").toLongLong());
    }
}
void MetadataController::refreshAll() {
    if (!allowed())
        return;
    const auto before = QDateTime::currentDateTimeUtc().addDays(-7).toString(Qt::ISODate);
    for (const auto &v :
         m_db.query("SELECT t.id,d.provider_id FROM media_titles t LEFT JOIN media_details d ON "
                    "d.title_id=t.id WHERE EXISTS(SELECT 1 FROM media_files f WHERE "
                    "f.title_id=t.id) AND (d.fetched_at IS NULL OR d.fetched_at<?) LIMIT 50",
                    {before})) {
        const auto title = v.toMap().value("id").toLongLong();
        if (m_pendingTitles.contains(title))
            continue;
        m_pendingTitles.insert(title);
        const auto provider = v.toMap().value("provider_id").toLongLong();
        if (provider > 0)
            fetch(title, provider);
        else
            lookup(title, false);
    }
}
void MetadataController::refreshTitle(qint64 title) {
    const auto row = m_db.query("SELECT provider_id FROM media_details WHERE title_id=?", {title});
    if (!row.isEmpty() && row.first().toMap().value("provider_id").toLongLong() > 0)
        fetch(title, row.first().toMap().value("provider_id").toLongLong());
    else
        search(title, {});
}
void MetadataController::edit(qint64 title, const QString &name, const QString &overview,
                              int year) {
    try {
        if (name.trimmed().isEmpty() || year < -1 || year > 9999)
            return;
        m_db.transaction([&] {
            m_db.query("UPDATE media_titles SET title=?,year=CASE WHEN ?<0 THEN year ELSE ? END "
                       "WHERE id=?",
                       {name.trimmed(), year, year, title});
            m_db.query("INSERT INTO media_details(title_id,overview,manual_title,manual_overview) "
                       "VALUES(?,?,1,1) ON CONFLICT(title_id) DO UPDATE SET "
                       "overview=excluded.overview,manual_title=1,manual_overview=1",
                       {title, overview});
        });
        m_library.reload();
        report("Manuelle Änderungen geschützt gespeichert.");
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void MetadataController::showImages(qint64 title) {
    m_selected = title;
    m_images = m_db.query(
        "SELECT * FROM media_images WHERE title_id=? AND local_path<>'' ORDER BY kind,id", {title});
    for (auto &v : m_images) {
        auto m = v.toMap();
        m["url"] = QUrl::fromLocalFile(
                       QFileInfo(m_db.path()).dir().filePath(m.value("local_path").toString()))
                       .toString();
        v = m;
    }
    emit changed();
}
void MetadataController::selectImage(qint64 id) {
    try {
        const auto rows =
            m_db.query("SELECT * FROM media_images WHERE id=? AND local_path<>''", {id});
        if (rows.isEmpty())
            return;
        const auto image = rows.first().toMap();
        const auto kind = image.value("kind").toString();
        if (kind != "poster" && kind != "backdrop")
            return;
        m_db.query("UPDATE media_details SET " + kind + "=?,manual_" + kind + "=1 WHERE title_id=?",
                   {image.value("local_path"), image.value("title_id")});
        m_library.reload();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void MetadataController::selectLocalImage(qint64 title, const QString &kind, const QUrl &url) {
    if (kind != "poster" && kind != "backdrop")
        return;
    try {
        const auto image = m_personal.importImage(url);
        m_db.query("INSERT INTO media_details(title_id) VALUES(?) ON CONFLICT(title_id) DO NOTHING",
                   {title});
        m_db.query("UPDATE media_details SET " + kind + "=?,manual_" + kind + "=1 WHERE title_id=?",
                   {image, title});
        m_library.reload();
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
void MetadataController::image(qint64 title, const QString &kind, const QString &remote,
                               bool primary) {
    if (!allowed() ||
        !QRegularExpression("^/[a-zA-Z0-9_/-]+\\.(jpg|png|webp)$").match(remote).hasMatch())
        return;
    m_db.query("INSERT INTO media_images(title_id,kind,remote_path,is_primary) VALUES(?,?,?,?) ON "
               "CONFLICT(title_id,kind,remote_path) DO UPDATE SET is_primary=excluded.is_primary",
               {title, kind, remote, primary ? 1 : 0});
    const auto existing =
        m_db.query(
                "SELECT local_path FROM media_images WHERE title_id=? AND kind=? AND remote_path=?",
                {title, kind, remote})
            .first()
            .toMap()
            .value("local_path")
            .toString();
    if (!existing.isEmpty() && QFileInfo::exists(QFileInfo(m_db.path()).dir().filePath(existing))) {
        installImage(title, kind, remote, existing);
        return;
    }
    m_imageRequests.enqueue({title, kind, remote});
    emit changed();
}
void MetadataController::downloadImage(qint64 title, const QString &kind, const QString &remote) {
    if (!allowed())
        return;
    const auto generation = m_generation;
    const QUrl url =
        m_imageBase.resolved(QUrl(QString(kind == "poster" ? "w500" : "w1280") + remote));
    QNetworkRequest request(url);
    request.setTransferTimeout(20000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    auto *reply = m_network.get(request);
    m_replies.insert(reply);
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 12 * 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, title, kind, remote] {
        m_replies.remove(reply);
        if (generation != m_generation || !allowed()) {
            reply->deleteLater();
            emit changed();
            return;
        }
        const auto bytes = reply->readAll();
        const auto ok =
            reply->error() == QNetworkReply::NoError && bytes.size() <= 12 * 1024 * 1024;
        reply->deleteLater();
        if (!ok || generation != m_generation || !allowed()) {
            if (!ok && generation == m_generation && allowed())
                report("Bildabruf fehlgeschlagen. Gespeicherte Daten bleiben erhalten; fehlende "
                       "Bilder können erneut geladen werden.");
            emit changed();
            return;
        }
        ++m_imageJobs;
        const auto directory = QFileInfo(m_db.path()).absolutePath();
        QMetaObject::invokeMethod(
            m_imageWorker,
            [this, bytes, directory, generation, title, kind, remote] {
                QBuffer buffer;
                buffer.setData(bytes);
                buffer.open(QIODevice::ReadOnly);
                QImageReader reader(&buffer);
                const auto size = reader.size();
                QString local;
                if (size.isValid() && qint64(size.width()) * size.height() <= 32000000) {
                    reader.setScaledSize(size.scaled(1280, 1280, Qt::KeepAspectRatio));
                    const auto image = reader.read();
                    if (!image.isNull()) {
                        local = "metadata/" +
                                QString::fromLatin1(
                                    QCryptographicHash::hash(bytes, QCryptographicHash::Sha256)
                                        .toHex()) +
                                ".png";
                        QDir().mkpath(directory + "/metadata");
                        if (!QFileInfo::exists(directory + "/" + local)) {
                            QSaveFile out(directory + "/" + local);
                            if (!out.open(QIODevice::WriteOnly) || !image.save(&out, "PNG") ||
                                !out.commit())
                                local.clear();
                        }
                    }
                }
                QMetaObject::invokeMethod(
                    this,
                    [this, local, generation, title, kind, remote] {
                        --m_imageJobs;
                        if (!local.isEmpty() && generation == m_generation && allowed())
                            installImage(title, kind, remote, local);
                        emit changed();
                    },
                    Qt::QueuedConnection);
            },
            Qt::QueuedConnection);
    });
}
void MetadataController::installImage(qint64 title, const QString &kind, const QString &remote,
                                      const QString &local) {
    try {
        m_db.query(
            "UPDATE media_images SET local_path=? WHERE title_id=? AND kind=? AND remote_path=?",
            {local, title, kind, remote});
        m_db.query("UPDATE media_details SET " + kind + "=? WHERE title_id=? AND manual_" + kind +
                       "=0 AND (" + kind +
                       "='' OR EXISTS(SELECT 1 FROM media_images WHERE title_id=? AND kind=? AND "
                       "remote_path=? AND is_primary=1))",
                   {local, title, title, kind, remote});
        m_library.reload();
        if (m_selected == title)
            showImages(title);
    } catch (const std::exception &e) {
        report(QString::fromUtf8(e.what()));
    }
}
} // namespace veluno
