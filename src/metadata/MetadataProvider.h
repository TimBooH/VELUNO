#pragma once
#include <QString>
#include <QVector>
namespace veluno {
struct MetadataCandidate { QString providerId; QString title; int year = 0; };
enum class OnlineConsent { Undecided, Denied, Granted };
// Provider-independent contract. No provider/network client is instantiated yet.
// Implementations must require Granted consent and preserve manually edited data.
class MetadataProvider {
public:
    virtual ~MetadataProvider() = default;
    virtual QString providerName() const = 0;
    virtual void search(const QString &title, OnlineConsent consent) = 0;
    virtual void cancel() = 0;
};
}
