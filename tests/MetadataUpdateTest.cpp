#include "metadata/MetadataController.h"
#include "updater/UpdateService.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSysInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>
#include <openssl/evp.h>
#include <openssl/pem.h>
using namespace veluno;
class ApiServer : public QObject {
  public:
    QTcpServer server;
    QList<QByteArray> requests;
    bool holdReplies = false;
    bool artwork = false;
    int httpStatus = 200;
    QList<QPointer<QTcpSocket>> heldReplies;
    ApiServer() {
        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (server.hasPendingConnections()) {
                auto *socket = server.nextPendingConnection();
                socket->setParent(this);
                auto bytes = std::make_shared<QByteArray>();
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, bytes] {
                    *bytes += socket->readAll();
                    if (!bytes->contains("\r\n\r\n"))
                        return;
                    const auto line = bytes->split('\n').first();
                    requests.append(*bytes);
                    if (holdReplies) {
                        heldReplies.append(socket);
                        return;
                    }
                    QJsonObject data;
                    if (line.contains("/configuration"))
                        data = {{"images",
                                 QJsonObject{{"secure_base_url", "https://image.tmdb.org/t/p/"}}}};
                    else if (line.contains("/search/"))
                        data = {
                            {"results",
                             QJsonArray{QJsonObject{
                                 {"id", 12}, {"title", "Movie"}, {"release_date", "2020-01-01"}}}}};
                    else
                        data = {{"id", 12},
                                {"title", "Provider title"},
                                {"overview", "Provider description"},
                                {"genres", QJsonArray{QJsonObject{{"name", "Drama"}}}},
                                {"images", QJsonObject{}}};
                    if (artwork && !line.contains("/configuration") && !line.contains("/search/")) {
                        data["poster_path"] = "/test.png";
                        data["backdrop_path"] = "/test.png";
                    }
                    auto body = QJsonDocument(data).toJson(QJsonDocument::Compact);
                    if (line.contains("/w500/") || line.contains("/w1280/")) {
                        QImage image(24, 16, QImage::Format_RGB32);
                        image.fill(Qt::blue);
                        QBuffer buffer(&body);
                        buffer.open(QIODevice::WriteOnly | QIODevice::Truncate);
                        image.save(&buffer, "PNG");
                    }
                    socket->write("HTTP/1.1 " + QByteArray::number(httpStatus) +
                                  " Result\r\nContent-Type: "
                                  "application/json\r\nConnection: close\r\nContent-Length: " +
                                  QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }
};
class MetadataUpdateTest : public QObject {
    Q_OBJECT
  private slots:
    void metadataConsentMatchingAndManualProtection() {
        QTemporaryDir dir;
        Database db(dir.filePath("app/veluno.sqlite3"));
        LibraryRepository repo(db);
        QVERIFY(QDir().mkpath(dir.filePath("media")));
        ScannedFile file;
        file.sourceId = repo.addSource(dir.filePath("media"), "movie");
        file.path = dir.filePath("media/Movie.mp4");
        file.kind = "movie";
        file.title = "Movie";
        file.year = 2020;
        repo.record(file, "test");
        const auto title = repo.file(file.path).value("title_id").toLongLong();
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        ApiServer api;
        QVERIFY(api.server.listen(QHostAddress::LocalHost));
        const QUrl base(QString("http://127.0.0.1:%1").arg(api.server.serverPort()));
        MetadataController metadata(db, library, personal, nullptr, base);
        metadata.edit(title, "Movie", "Locally edited without TMDB");
        QCOMPARE(metadata.statistics().value("matched").toInt(), 0);
        metadata.setToken("test-token");
        metadata.search(title, "Movie");
        QTest::qWait(650);
        QCOMPARE(api.requests.size(), 0);
        library.setOnlineConsent("granted");
        metadata.search(title, "Movie");
        QTRY_COMPARE_WITH_TIMEOUT(metadata.results().size(), 1, 5000);
        QVERIFY(api.requests.first().toLower().contains("authorization: bearer test-token"));
        QVERIFY(!api.requests.first().split('\n').first().contains("test-token"));
        metadata.choose(12);
        QTRY_VERIFY_WITH_TIMEOUT(
            !db.query("SELECT * FROM media_details WHERE provider_id=12").isEmpty(), 5000);
        metadata.edit(title, "My title", "My description");
        metadata.refreshTitle(title);
        QTRY_VERIFY_WITH_TIMEOUT(!metadata.busy(), 8000);
        QCOMPARE(db.query("SELECT title FROM media_titles WHERE id=?", {title})
                     .first()
                     .toMap()
                     .value("title")
                     .toString(),
                 QString("My title"));
        QCOMPARE(db.query("SELECT overview FROM media_details WHERE title_id=?", {title})
                     .first()
                     .toMap()
                     .value("overview")
                     .toString(),
                 QString("My description"));
        const auto requests = api.requests.size();
        metadata.search(title, "Something");
        library.setOnlineConsent("denied");
        QTest::qWait(700);
        QCOMPARE(api.requests.size(), requests);
        db.query("PRAGMA query_only=ON");
        metadata.edit(title, "Must not be saved", "Read-only test");
        QCOMPARE(db.query("SELECT title FROM media_titles WHERE id=?", {title})
                     .first()
                     .toMap()
                     .value("title")
                     .toString(),
                 QString("My title"));
        QVERIFY(!metadata.status().isEmpty());
    }
    void metadataSwitchPersistsCancelsAndRetainsOfflineData() {
        QTemporaryDir dir;
        Database db(dir.filePath("app/veluno.sqlite3"));
        LibraryRepository repo(db);
        QVERIFY(QDir().mkpath(dir.filePath("media")));
        ScannedFile file;
        file.sourceId = repo.addSource(dir.filePath("media"), "movie");
        file.path = dir.filePath("media/Movie.mp4");
        file.kind = "movie";
        file.title = "Movie";
        repo.record(file, "test");
        const auto title = repo.file(file.path).value("title_id").toLongLong();
        db.query("INSERT INTO media_details(title_id,provider_id,overview) VALUES(?,12,?)",
                 {title, "Cached description"});
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        ApiServer api;
        QVERIFY(api.server.listen(QHostAddress::LocalHost));
        const QUrl base(QString("http://127.0.0.1:%1").arg(api.server.serverPort()));
        {
            MetadataController metadata(db, library, personal, nullptr, base);
            QVERIFY(!metadata.enabled());
            metadata.setToken("test-token");
            QVERIFY(metadata.configured());
            QVERIFY(!metadata.enabled()); // Saving a token never opts in.
            api.holdReplies = true;
            metadata.setEnabled(true);
            QCOMPARE(library.onlineConsent(), QString("granted"));
            QVERIFY(metadata.enabled());
            metadata.refreshTitle(title);
            QTRY_COMPARE_WITH_TIMEOUT(api.requests.size(), 1, 5000);
            QVERIFY(metadata.busy());
            metadata.setEnabled(false);
            QVERIFY(!metadata.enabled());
            QTRY_VERIFY_WITH_TIMEOUT(!metadata.busy(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(api.heldReplies.first().isNull() ||
                                         api.heldReplies.first()->state() ==
                                             QAbstractSocket::UnconnectedState,
                                     3000);
            // Disabling cancels both the active request and already queued lookups.
            metadata.refreshAll();
            metadata.refreshTitle(title);
            metadata.search(title, "Movie");
            QTest::qWait(700);
            QCOMPARE(api.requests.size(), 1);
            QCOMPARE(db.query("SELECT overview FROM media_details WHERE title_id=?", {title})
                         .first()
                         .toMap()
                         .value("overview")
                         .toString(),
                     QString("Cached description"));
            QVERIFY(metadata.configured());
        }
        // A reconstructed controller observes the persisted off state and token.
        MetadataController restored(db, library, personal, nullptr, base);
        QVERIFY(!restored.enabled());
        QVERIFY(restored.configured());
        api.holdReplies = false;
        restored.setEnabled(true);
        // Turning on triggers the existing automatic background fetch without another button.
        QTRY_COMPARE_WITH_TIMEOUT(
            db.query("SELECT overview FROM media_details WHERE title_id=?", {title})
                .first()
                .toMap()
                .value("overview")
                .toString(),
            QString("Provider description"), 6000);
        restored.setEnabled(false);
        QCOMPARE(db.setting("online_consent"), QString("denied"));
        library.setOnlineConsent("granted"); // Wizard and settings share one choice.
        QVERIFY(restored.enabled());
        restored.setEnabled(false);
        db.query("PRAGMA query_only=ON");
        restored.setEnabled(true);
        QVERIFY(!restored.enabled()); // A failed save cannot silently enable requests.
    }
    void metadataAccessAndOfflineArtworkRoundTrip() {
        QTemporaryDir dir;
        Database db(dir.filePath("app/veluno.sqlite3"));
        LibraryRepository repo(db);
        QDir().mkpath(dir.filePath("media"));
        ScannedFile f;
        f.sourceId = repo.addSource(dir.filePath("media"), "movie");
        f.path = dir.filePath("media/Movie.mp4");
        f.title = "Movie";
        f.kind = "movie";
        repo.record(f, "fixture");
        const auto title = repo.file(f.path).value("title_id").toLongLong();
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        ApiServer api;
        api.artwork = true;
        QVERIFY(api.server.listen(QHostAddress::LocalHost));
        const QUrl base(QString("http://127.0.0.1:%1/").arg(api.server.serverPort()));
        MetadataController metadata(db, library, personal, nullptr,
                                    QUrl(base.toString().chopped(1)), base);
        metadata.testAccess();
        QTest::qWait(600);
        QCOMPARE(api.requests.size(), 0);
        metadata.setToken("test-token");
        metadata.setEnabled(true);
        metadata.testAccess();
        QTRY_VERIFY_WITH_TIMEOUT(metadata.status().contains("erfolgreich geprüft"), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(metadata.statistics().value("posters").toInt(), 1, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(!metadata.busy(), 10000);
        QCOMPARE(metadata.statistics().value("matched").toInt(), 1);
        QCOMPARE(metadata.pendingCount(), 0);
        const auto details =
            db.query("SELECT poster,backdrop FROM media_details WHERE title_id=?", {title})
                .first()
                .toMap();
        QVERIFY(QFileInfo::exists(dir.filePath("app/" + details.value("poster").toString())));
        QVERIFY(!details.value("backdrop").toString().isEmpty());
        metadata.setEnabled(false);
        const auto count = api.requests.size();
        metadata.retryMissingArtwork();
        QTest::qWait(650);
        QCOMPARE(api.requests.size(), count);
        QCOMPARE(db.query("SELECT poster FROM media_details WHERE title_id=?", {title})
                     .first()
                     .toMap()
                     .value("poster"),
                 details.value("poster"));
    }
    void invalidMetadataAccessCanBeCorrected() {
        QTemporaryDir dir;
        Database db(dir.filePath("db.sqlite3"));
        LibraryController library(db, false, false);
        MpvEngine player(nullptr, true);
        PersonalController personal(db, library, player);
        ApiServer api;
        api.httpStatus = 401;
        QVERIFY(api.server.listen(QHostAddress::LocalHost));
        MetadataController metadata(
            db, library, personal, nullptr,
            QUrl(QString("http://127.0.0.1:%1").arg(api.server.serverPort())));
        metadata.setToken("invalid-test-token");
        metadata.setEnabled(true);
        metadata.testAccess();
        QTRY_VERIFY_WITH_TIMEOUT(metadata.status().contains("abgewiesen"), 5000);
        QTRY_VERIFY(!metadata.busy());
        api.httpStatus = 200;
        metadata.setToken("corrected-test-token");
        metadata.testAccess();
        QTRY_VERIFY_WITH_TIMEOUT(metadata.status().contains("erfolgreich geprüft"), 5000);
        QVERIFY(api.requests.last().contains("corrected-test-token"));
    }
    void signedUpdatesRejectTamperingAndIncompatibleData() {
        auto *context = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
        QVERIFY(context);
        EVP_PKEY *key = nullptr;
        QVERIFY(EVP_PKEY_keygen_init(context) == 1);
        QVERIFY(EVP_PKEY_keygen(context, &key) == 1);
        EVP_PKEY_CTX_free(context);
        auto *bio = BIO_new(BIO_s_mem());
        QVERIFY(PEM_write_bio_PUBKEY(bio, key) == 1);
        char *data = nullptr;
        const auto length = BIO_get_mem_data(bio, &data);
        const QByteArray publicKey(data, length);
        BIO_free(bio);
        const auto envelope = [key](QJsonObject document) {
            const auto bytes = QJsonDocument(document).toJson(QJsonDocument::Compact);
            auto *ctx = EVP_MD_CTX_new();
            EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, key);
            size_t count = 64;
            QByteArray signature(64, '\0');
            EVP_DigestSign(ctx, reinterpret_cast<unsigned char *>(signature.data()), &count,
                           reinterpret_cast<const unsigned char *>(bytes.constData()),
                           bytes.size());
            EVP_MD_CTX_free(ctx);
            signature.resize(count);
            return QJsonDocument(
                       QJsonObject{{"payload", QString::fromLatin1(bytes.toBase64())},
                                   {"signature", QString::fromLatin1(signature.toBase64())}})
                .toJson();
        };
        QJsonObject manifest{
            {"format", 1},         {"channel", "stable"},
            {"version", "0.2.0"},  {"build", "2030-01-01T00:00:00Z"},
            {"platform", "linux"}, {"arch", QSysInfo::currentCpuArchitecture()},
            {"minSchema", 1},      {"schema", 100},
            {"minGlibc", "2.31"},  {"url", "https://updates.example.invalid/VELUNO.AppImage"},
            {"size", 100},         {"sha256", QString(64, 'a')}};
        const auto valid = envelope(manifest);
        QCOMPARE(UpdateService::verifyManifest(valid, publicKey, "stable", 4, "0.1.0",
                                               "2026-01-01T00:00:00Z")
                     .value("version")
                     .toString(),
                 QString("0.2.0"));
        auto wrapper = QJsonDocument::fromJson(valid).object();
        wrapper["payload"] = QString::fromLatin1(QByteArray("tampered").toBase64());
        QVERIFY_EXCEPTION_THROWN(UpdateService::verifyManifest(QJsonDocument(wrapper).toJson(),
                                                               publicKey, "stable", 4, "0.1.0", ""),
                                 std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(
            UpdateService::verifyManifest(valid, publicKey, "unstable", 4, "0.1.0", ""),
            std::runtime_error);
        manifest["schema"] = 2;
        QVERIFY_EXCEPTION_THROWN(
            UpdateService::verifyManifest(envelope(manifest), publicKey, "stable", 4, "0.1.0", ""),
            std::runtime_error);
        manifest["schema"] = 100;
        manifest["url"] = "http://insecure.invalid/app";
        QVERIFY_EXCEPTION_THROWN(
            UpdateService::verifyManifest(envelope(manifest), publicKey, "stable", 4, "0.1.0", ""),
            std::runtime_error);
        manifest["url"] = "https://updates.example.invalid/app";
        manifest["version"] = "0.0.1";
        QVERIFY_EXCEPTION_THROWN(
            UpdateService::verifyManifest(envelope(manifest), publicKey, "stable", 4, "0.1.0", ""),
            std::runtime_error);
        manifest["version"] = "0.2.0-beta.1";
        QVERIFY_EXCEPTION_THROWN(
            UpdateService::verifyManifest(envelope(manifest), publicKey, "stable", 4, "0.1.0", ""),
            std::runtime_error);
        EVP_PKEY_free(key);
    }
    void atomicInstallRetainsPreviousAndRejectsBadPackage() {
        QTemporaryDir dir;
        const auto source = dir.filePath("new.AppImage"), target = dir.filePath("VELUNO.AppImage");
        QByteArray bytes(64, 'x');
        bytes.replace(0, 4, QByteArray::fromHex("7f454c46"));
        bytes.replace(8, 3, QByteArray("AI\x02", 3));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(bytes);
        file.close();
        QFile old(target);
        QVERIFY(old.open(QIODevice::WriteOnly));
        old.write("old appimage");
        old.close();
        const auto hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
        const auto previous = UpdateService::installFile(source, target, hash);
        QVERIFY(old.open(QIODevice::ReadOnly));
        QCOMPARE(old.readAll(), bytes);
        old.close();
        QFile prior(previous);
        QVERIFY(prior.open(QIODevice::ReadOnly));
        QCOMPARE(prior.readAll(), QByteArray("old appimage"));
        QVERIFY_EXCEPTION_THROWN(UpdateService::installFile(source, target, QByteArray(64, '0')),
                                 std::runtime_error);
        QVERIFY(old.open(QIODevice::ReadOnly));
        QCOMPARE(old.readAll(), bytes);
    }
};
QTEST_GUILESS_MAIN(MetadataUpdateTest)
#include "MetadataUpdateTest.moc"
