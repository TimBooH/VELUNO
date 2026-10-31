#include "UpdateService.h"
#include "database/Migrations.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUuid>
#include <QVersionNumber>
#include <cmath>
#include <cstdio>
#include <gnu/libc-version.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdexcept>
#include <unistd.h>
#ifndef VELUNO_BUILD_UTC
#define VELUNO_BUILD_UTC "2026-10-30T00:00:00Z"
#endif
#ifndef VELUNO_APP_VERSION
#define VELUNO_APP_VERSION "0.1.0-dev"
#endif
#ifndef VELUNO_BUILD_CHANNEL
#define VELUNO_BUILD_CHANNEL "unstable"
#endif
namespace veluno {
namespace {
void need(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
bool https(const QUrl &url) {
    return url.isValid() && url.scheme() == "https" && !url.host().isEmpty() &&
           url.userInfo().isEmpty() && !url.hasFragment();
}
QByteArray fileHash(const QString &path) {
    QFile in(path);
    need(in.open(QIODevice::ReadOnly), "Datei nicht lesbar.");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    need(hash.addData(&in), "Prüfsumme konnte nicht gelesen werden.");
    return hash.result().toHex();
}
} // namespace
QVariantMap UpdateService::buildInfo() {
    return {{"version", VELUNO_APP_VERSION},
            {"channel", VELUNO_BUILD_CHANNEL},
            {"build", VELUNO_BUILD_UTC},
            {"schema", schemaMigrations().size()},
            {"platform", "linux"},
            {"arch", QSysInfo::currentCpuArchitecture()},
            {"minGlibc", "2.39"}};
}
QVariantMap UpdateService::verifyManifest(const QByteArray &envelope, const QByteArray &key,
                                          const QString &channel, int schema,
                                          const QString &currentVersion,
                                          const QString &currentBuild) {
    need(envelope.size() <= 1024 * 1024 && key.size() <= 16384,
         "Updateinformationen sind zu groß.");
    const auto wrapper = QJsonDocument::fromJson(envelope).object();
    const auto payload = QByteArray::fromBase64(wrapper.value("payload").toString().toLatin1());
    const auto signature = QByteArray::fromBase64(wrapper.value("signature").toString().toLatin1());
    need(!payload.isEmpty() && signature.size() == 64, "Signierte Updateinformationen fehlen.");
    auto *bio = BIO_new_mem_buf(key.constData(), key.size());
    need(bio, "Signaturschlüssel nicht lesbar.");
    auto *pub = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    need(pub, "Öffentlicher Update-Schlüssel ist ungültig.");
    auto *context = EVP_MD_CTX_new();
    const bool valid =
        context && EVP_PKEY_id(pub) == EVP_PKEY_ED25519 &&
        EVP_DigestVerifyInit(context, nullptr, nullptr, nullptr, pub) == 1 &&
        EVP_DigestVerify(context, reinterpret_cast<const unsigned char *>(signature.constData()),
                         signature.size(),
                         reinterpret_cast<const unsigned char *>(payload.constData()),
                         payload.size()) == 1;
    EVP_MD_CTX_free(context);
    EVP_PKEY_free(pub);
    need(valid, "Update-Signatur ist ungültig. Download gesperrt.");
    const auto json = QJsonDocument::fromJson(payload).object();
    need(!json.isEmpty() && json.value("format").toInt() == 1, "Unbekanntes Updateformat.");
    need(json.value("channel").toString() == channel, "Update gehört zu einem anderen Kanal.");
    need(json.value("platform").toString() == "linux" &&
             json.value("arch").toString() == QSysInfo::currentCpuArchitecture(),
         "Update passt nicht zu diesem System.");
    const auto version = json.value("version").toString();
    need(QRegularExpression("^[0-9]+\\.[0-9]+\\.[0-9]+(?:-[A-Za-z0-9.-]+)?(?:\\+[A-Za-z0-9.-]+)?$")
             .match(version)
             .hasMatch(),
         "Ungültige Updateversion.");
    need(channel != "stable" || !version.contains('-'),
         "Ein Entwicklungsbuild darf nicht als Stable-Update installiert werden.");
    const auto proposed = QVersionNumber::fromString(version),
               current = QVersionNumber::fromString(currentVersion);
    need(proposed >= current, "Downgrade gesperrt.");
    const auto build = QDateTime::fromString(json.value("build").toString(), Qt::ISODate),
               previous = QDateTime::fromString(currentBuild, Qt::ISODate);
    need(build.isValid() && (proposed > current || !previous.isValid() || build > previous),
         "Keine neuere Version verfügbar.");
    const auto minimum = QVersionNumber::fromString(json.value("minGlibc").toString());
    need(!minimum.isNull() &&
             QVersionNumber::fromString(QString::fromLatin1(gnu_get_libc_version())) >= minimum,
         "Update benötigt eine neuere Linux-Laufzeitbibliothek.");
    need(json.value("minSchema").toInt(1) <= schema && json.value("schema").toInt() >= schema,
         "Update ist mit der aktuellen Datenbank inkompatibel.");
    need(https(QUrl(json.value("url").toString())),
         "Updates benötigen eine HTTPS-Adresse ohne Zugangsdaten.");
    need(QRegularExpression("^[a-fA-F0-9]{64}$").match(json.value("sha256").toString()).hasMatch(),
         "Update-Prüfsumme fehlt.");
    const auto size = json.value("size").toDouble();
    need(size > 0 && size <= 1024.0 * 1024 * 1024 && std::floor(size) == size,
         "Ungültige Paketgröße.");
    return json.toVariantMap();
}
UpdateService::UpdateService(Database &db, BackupService &backups, bool automatic, QObject *parent)
    : QObject(parent), m_db(db), m_backups(backups), m_worker(new QObject) {
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
    m_timer.setInterval(3600000);
    connect(&m_timer, &QTimer::timeout, this, &UpdateService::check);
    if (automatic) {
        m_timer.start();
        QTimer::singleShot(5000, this, &UpdateService::check);
    }
}
UpdateService::~UpdateService() {
    m_timer.stop();
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
    }
    m_thread.quit();
    m_thread.wait();
}
QVariantMap UpdateService::settings() const {
    return {{"stable", m_db.setting("update_stable_url")},
            {"unstable", m_db.setting("update_unstable_url")},
            {"key", m_db.setting("update_public_key")}};
}
void UpdateService::setChannel(const QString &value) {
    try {
        if (m_busy || (value != "stable" && value != "unstable"))
            return;
        m_db.setSetting("update_channel", value);
        m_candidate.clear();
        m_download.clear();
        m_status = "Updatekanal gespeichert.";
        emit changed();
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
    }
}
bool UpdateService::configure(const QString &stable, const QString &unstable, const QString &key) {
    try {
        if (m_busy)
            return false;
        for (const auto &value : {stable, unstable})
            if (!value.trimmed().isEmpty() && !https(QUrl(value.trimmed()))) {
                fail("Bitte eine gültige dauerhafte HTTPS-Manifestadresse eingeben.");
                return false;
            }
        if (!key.trimmed().isEmpty()) {
            const auto bytes = key.trimmed().toUtf8();
            if (bytes.size() > 16384 || bytes.contains("PRIVATE KEY")) {
                fail("Bitte ausschließlich einen öffentlichen Ed25519-Schlüssel verwenden.");
                return false;
            }
            auto *bio = BIO_new_mem_buf(bytes.constData(), bytes.size());
            auto *parsed = bio ? PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr) : nullptr;
            if (bio)
                BIO_free(bio);
            const bool valid = parsed && EVP_PKEY_id(parsed) == EVP_PKEY_ED25519;
            if (parsed)
                EVP_PKEY_free(parsed);
            if (!valid) {
                fail("Öffentlicher Ed25519-Schlüssel ist ungültig.");
                return false;
            }
        }
        m_db.transaction([&] {
            m_db.setSetting("update_stable_url", stable.trimmed());
            m_db.setSetting("update_unstable_url", unstable.trimmed());
            m_db.setSetting("update_public_key", key.trimmed());
        });
        m_candidate.clear();
        m_download.clear();
        m_status = "Updatequelle und öffentlicher Signaturschlüssel gespeichert.";
        emit changed();
        return true;
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        return false;
    }
}
void UpdateService::fail(const QString &message) {
    if (m_output) {
        m_output->cancelWriting();
        m_output.reset();
    }
    m_hash.reset();
    m_busy = false;
    m_status = message;
    emit changed();
}
void UpdateService::check() {
    if (m_busy || m_restart)
        return;
    const auto url = m_db.setting("update_" + channel() + "_url");
    const auto key = m_db.setting("update_public_key").toUtf8();
    if (url.isEmpty() || key.isEmpty()) {
        m_status = "Noch keine verifizierbare Updatequelle für diesen Kanal eingerichtet.";
        emit changed();
        return;
    }
    if (!https(QUrl(url))) {
        fail("Updatequelle muss HTTPS verwenden.");
        return;
    }
    m_busy = true;
    m_status = "Signierte Updateinformationen werden geprüft …";
    m_candidate.clear();
    m_download.clear();
    emit changed();
    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(20000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    auto *reply = m_network.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, key] {
        m_reply = nullptr;
        const auto bytes = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!ok) {
            fail("Updatequelle nicht erreichbar. Die bestehende Installation bleibt erhalten.");
            return;
        }
        try {
            m_candidate = verifyManifest(bytes, key, channel(), schemaMigrations().size(),
                                         QCoreApplication::applicationVersion(), VELUNO_BUILD_UTC);
            m_busy = false;
            m_status = "Neue Version verfügbar: " + m_candidate.value("version").toString();
            emit changed();
        } catch (const std::exception &e) {
            fail(QString::fromUtf8(e.what()));
        }
    });
}
void UpdateService::download() {
    if (m_busy || m_candidate.isEmpty())
        return;
    const auto url = QUrl(m_candidate.value("url").toString());
    if (!https(url))
        return;
    const auto cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/updates";
    if (!QDir().mkpath(cache)) {
        fail("Downloadverzeichnis ist nicht schreibbar.");
        return;
    }
    m_download.clear();
    const auto path =
        cache + "/VELUNO-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".AppImage";
    m_output = std::make_unique<QSaveFile>(path);
    if (!m_output->open(QIODevice::WriteOnly)) {
        fail("Update kann nicht gespeichert werden.");
        return;
    }
    m_hash = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);
    m_received = 0;
    m_busy = true;
    m_progress = 0;
    m_status = "Update wird heruntergeladen …";
    emit changed();
    QNetworkRequest request(url);
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = m_network.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        if (!m_output)
            return;
        const auto bytes = reply->readAll();
        m_received += bytes.size();
        if (m_received > m_candidate.value("size").toLongLong() ||
            m_output->write(bytes) != bytes.size()) {
            reply->abort();
            return;
        }
        m_hash->addData(bytes);
        m_progress = double(m_received) / m_candidate.value("size").toDouble();
        emit changed();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, path] {
        m_reply = nullptr;
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!ok || !m_output || m_received != m_candidate.value("size").toLongLong() ||
            m_hash->result().toHex() !=
                m_candidate.value("sha256").toString().toLower().toLatin1()) {
            fail("Download unvollständig oder Prüfsumme falsch. Paket verworfen.");
            return;
        }
        if (!m_output->commit()) {
            fail("Update konnte nicht gespeichert werden.");
            return;
        }
        m_output.reset();
        m_hash.reset();
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                        QFileDevice::ExeOwner);
        m_download = path;
        m_busy = false;
        m_status = "Update vollständig und kryptografisch geprüft. Installation erfordert deine "
                   "Bestätigung.";
        emit changed();
    });
}
void UpdateService::cancel() {
    if (m_reply) {
        m_reply->abort();
    }
}
QString UpdateService::installFile(const QString &source, const QString &target,
                                   const QByteArray &expected) {
    need(QFileInfo(target).isAbsolute() && QFileInfo(target).isFile() &&
             !QFileInfo(target).isSymLink(),
         "Die laufende AppImage-Datei ist nicht regulär oder nicht erreichbar.");
    need(fileHash(source) == expected.toLower(),
         "Updatepaket hat sich nach der Prüfung verändert.");
    QFile input(source);
    need(input.open(QIODevice::ReadOnly), "Updatepaket nicht lesbar.");
    const auto header = input.peek(12);
    need(header.startsWith(QByteArray::fromHex("7f454c46")) &&
             header.mid(8, 3) == QByteArray("AI\x02", 3),
         "Das Update ist kein AppImage Typ 2.");
    const auto staging = target + ".install-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSaveFile out(staging);
    need(out.open(QIODevice::WriteOnly), "AppImage-Verzeichnis ist nicht schreibbar.");
    while (!input.atEnd()) {
        const auto bytes = input.read(1024 * 1024);
        need(!bytes.isEmpty() && out.write(bytes) == bytes.size(),
             "Update konnte nicht vollständig kopiert werden.");
    }
    need(out.commit(), "Update konnte nicht bereitgestellt werden.");
    try {
        need(fileHash(staging) == expected.toLower(),
             "Prüfsumme der bereitgestellten Datei stimmt nicht.");
        need(
            QFile::setPermissions(staging, QFileInfo(target).permissions() | QFileDevice::ExeOwner),
            "Ausführungsrechte konnten nicht gesetzt werden.");
        const auto previous =
            target + ".previous-" + QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz");
        if (::link(QFile::encodeName(target).constData(),
                   QFile::encodeName(previous).constData()) != 0)
            need(QFile::copy(target, previous),
                 "Bisherige AppImage-Datei konnte nicht erhalten werden.");
        need(std::rename(QFile::encodeName(staging).constData(),
                         QFile::encodeName(target).constData()) == 0,
             "Atomarer AppImage-Austausch fehlgeschlagen; bisherige Version bleibt erhalten.");
        return previous;
    } catch (...) {
        QFile::remove(staging);
        throw;
    }
}
void UpdateService::install() {
    if (m_busy || m_download.isEmpty() || m_backups.busy() || m_backups.restartRequired())
        return;
    if (qEnvironmentVariable("APPIMAGE").isEmpty()) {
        fail("Die automatische Übergabe ist nur innerhalb eines AppImage möglich. Der geprüfte "
             "Download bleibt verfügbar.");
        return;
    }
    m_busy = true;
    m_status = "Separate Update-Sicherung wird erstellt …";
    emit changed();
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(&m_backups, &BackupService::completed, this,
                          [this, connection](bool success, const QString &path) {
                              disconnect(*connection);
                              if (!success) {
                                  fail("Update-Sicherung fehlgeschlagen. Installation gesperrt.");
                                  return;
                              }
                              try {
                                  m_db.setSetting("last_update_backup", path);
                                  beginInstall();
                              } catch (const std::exception &e) {
                                  fail(QString::fromUtf8(e.what()));
                              }
                          });
    m_backups.create("update");
}
void UpdateService::beginInstall() {
    const auto source = m_download, target = qEnvironmentVariable("APPIMAGE");
    const auto expected = m_candidate.value("sha256").toString().toLatin1();
    QMetaObject::invokeMethod(
        m_worker,
        [this, source, target, expected] {
            QString previous, error;
            try {
                previous = installFile(source, target, expected);
            } catch (const std::exception &e) {
                error = QString::fromUtf8(e.what());
            }
            QMetaObject::invokeMethod(
                this,
                [this, previous, error] {
                    if (!error.isEmpty()) {
                        fail(error);
                        return;
                    }
                    try {
                        m_db.setSetting("previous_appimage", previous);
                    } catch (...) {
                    }
                    m_busy = false;
                    m_restart = true;
                    m_status = "AppImage atomar ersetzt. Bisherige Datei bleibt unter " + previous +
                               " erhalten. Bitte VELUNO schließen und neu starten.";
                    emit changed();
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}
} // namespace veluno
