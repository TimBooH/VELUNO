#pragma once
#include <QUrl>
#include <QObject>
#include <functional>
#include <atomic>
namespace veluno {
// SMB addresses are durable identities; KIO-FUSE paths are session-local only.
QUrl normalizedSmb(const QUrl &url);
bool isSmb(const QString &path);
QString childLocation(const QString &root, const QString &relative);
class NetworkAccess final {
public:
    using Completion = std::function<void(QString path, QString error)>;
    static void resolve(const QUrl &url, QObject *context, Completion completion);
    // Scanner thread only. GUI work uses the asynchronous overload above.
    static QString resolveForScan(const QUrl &url, const std::atomic_bool &cancel, QString &error);
};
}
