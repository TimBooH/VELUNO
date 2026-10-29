#pragma once
#include <QStringList>
#include <functional>
namespace veluno {
struct ScanProgress { qsizetype discovered = 0; QString currentPath; };
// Implementations run on a worker and deliver progress through queued signals.
// Scan sources are read-only; no rename/move/delete operations are exposed.
class Scanner {
public:
    virtual ~Scanner() = default;
    virtual void start(const QStringList &roots) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void cancel() = 0;
};
}
