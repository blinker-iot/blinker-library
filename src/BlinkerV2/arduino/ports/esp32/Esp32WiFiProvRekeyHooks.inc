#if !defined(BLINKER_INTERNAL_ESP32_WIFIPROV_REKEY) || !BLINKER_INTERNAL_ESP32_WIFIPROV_REKEY || \
    defined(BLINKER_ESP32_PROVISIONING_SOFTAP)
#error "Include the rekey linker hooks once, in the restricted ESP32 BLE candidate sketch"
#endif

#include <protocomm.h>
#include "Esp32WiFiProvRekeyCandidate.h"

extern "C" const protocomm_security_t __real_protocomm_security1;
extern "C" decltype(protocomm_req_handle) __real_protocomm_req_handle;
extern "C" decltype(network_prov_config_data_handler) __real_network_prov_config_data_handler;

namespace {
using RekeyCandidate = blinker::Esp32WiFiProvRekeyCandidate;
struct RekeyNativeBinding {
    static blinker::Esp32WiFiProvSecurity1Owner* owner() {
        RekeyCandidate* const active = RekeyCandidate::active().load(std::memory_order_acquire);
        return active == nullptr ? nullptr : &active->native();
    }
    static const protocomm_security_t& originalSecurity1() { return __real_protocomm_security1; }
};
using RekeyNativeHook = blinker::Esp32WiFiProvSecurity1OwnerHook<RekeyNativeBinding>;
} // namespace

extern "C" const protocomm_security_t __wrap_protocomm_security1 = RekeyNativeHook::wrap();

extern "C" esp_err_t __wrap_protocomm_req_handle(protocomm_t* pc, const char* endpoint, uint32_t session,
    const uint8_t* input, ssize_t size, uint8_t** output, ssize_t* written) {
    return RekeyNativeHook::request(&__real_protocomm_req_handle, pc, endpoint, session, input, size, output, written);
}

extern "C" esp_err_t __wrap_network_prov_config_data_handler(uint32_t session, const uint8_t* input,
    ssize_t size, uint8_t** output, ssize_t* written, void* context) {
    if (output == nullptr || written == nullptr) return ESP_ERR_INVALID_ARG;
    *output = nullptr;
    *written = 0;
    RekeyCandidate* const active = RekeyCandidate::active().load(std::memory_order_acquire);
    blinker::Esp32WiFiProvRekeyNativeOwner owner{};
    if (active == nullptr ||
        !active->native().current(blinker::Esp32WiFiProvRekeyEndpointKind::SdkConfig, session, owner)) return ESP_ERR_INVALID_STATE;
    return blinker::forwardEsp32WiFiProvConfig(&__real_network_prov_config_data_handler, active->configFence(), owner.session,
        input, size, output, written, context);
}
