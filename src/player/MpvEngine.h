#pragma once
#include <memory>
struct mpv_handle;
namespace veluno {
// Deferred lifecycle: constructing the app never opens audio/video devices.
// Render API integration and playback commands belong to the next milestone.
class MpvEngine final {
public:
    MpvEngine();
    ~MpvEngine();
    MpvEngine(const MpvEngine &) = delete;
    MpvEngine &operator=(const MpvEngine &) = delete;
    static unsigned long apiVersion();
private:
    struct Deleter { void operator()(mpv_handle *handle) const; };
    std::unique_ptr<mpv_handle, Deleter> m_handle;
};
}
