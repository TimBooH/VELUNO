#include "MpvEngine.h"
#include <mpv/client.h>
namespace veluno {
MpvEngine::MpvEngine() = default;
MpvEngine::~MpvEngine() = default;
void MpvEngine::Deleter::operator()(mpv_handle *handle) const { if (handle) mpv_terminate_destroy(handle); }
unsigned long MpvEngine::apiVersion() { return mpv_client_api_version(); }
}
