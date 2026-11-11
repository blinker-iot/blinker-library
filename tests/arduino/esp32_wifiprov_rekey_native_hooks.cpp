// Core 3.3.11 ESP32-S3 link probe, not a Sketch or registered product route.
#include <sdkconfig.h>
#include <protocomm.h>

#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProvConfigFence.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProvRekeyApply.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProvSecurity1Owner.h>

extern "C" const protocomm_security_t __real_protocomm_security1;
extern "C" decltype(protocomm_req_handle) __real_protocomm_req_handle;
extern "C" decltype(network_prov_config_data_handler) __real_network_prov_config_data_handler;

namespace {
blinker::Esp32WiFiProvSecurity1Owner* active = nullptr;
blinker::IEsp32WiFiProvConfigFence* config = nullptr;

struct Binding {
    static blinker::Esp32WiFiProvSecurity1Owner* owner() { return active; }
    static const protocomm_security_t& originalSecurity1() { return __real_protocomm_security1; }
};
using Hook = blinker::Esp32WiFiProvSecurity1OwnerHook<Binding>;
} // namespace

extern "C" const protocomm_security_t __wrap_protocomm_security1 = Hook::wrap();

extern "C" esp_err_t __wrap_protocomm_req_handle(protocomm_t* pc, const char* ep, uint32_t session,
    const uint8_t* input, ssize_t size, uint8_t** output, ssize_t* written) {
    return Hook::request(&__real_protocomm_req_handle, pc, ep, session, input, size, output, written);
}

extern "C" esp_err_t __wrap_network_prov_config_data_handler(uint32_t session, const uint8_t* input,
    ssize_t size, uint8_t** output, ssize_t* written, void* context) {
    blinker::Esp32WiFiProvRekeyNativeOwner owner{};
    if (active == nullptr || config == nullptr ||
        !active->current(blinker::Esp32WiFiProvRekeyEndpointKind::SdkConfig, session, owner)) return ESP_ERR_INVALID_STATE;
    return blinker::forwardEsp32WiFiProvConfig(&__real_network_prov_config_data_handler, *config, owner.session,
        input, size, output, written, context);
}

// No binding is installed by default; the product will own this lifetime.
void blinkerEsp32RekeyLinkProbeBind(blinker::Esp32WiFiProvSecurity1Owner& owner,
    blinker::IEsp32WiFiProvConfigFence& fence) {
    active = &owner;
    config = &fence;
}

blinker::Result blinkerEsp32RekeyLinkProbeDispatch(blinker::Esp32WiFiProvRekeyAdmin& admin,
    blinker::Esp32WiFiProvRekeyApply& apply, uint32_t session, blinker::ByteView request,
    blinker::MutableByteSpan response, size_t& written) {
    if (request.data != nullptr && request.size > 1U && request.data[1] == blinker::Esp32WiFiProvRekeyApply::permitOperation)
        return apply.handle(session, request, response, written);
    return admin.handle(session, request, response, written);
}
