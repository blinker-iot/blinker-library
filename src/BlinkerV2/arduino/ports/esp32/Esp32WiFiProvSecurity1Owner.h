#ifndef BLINKER_ESP32_WIFIPROV_SECURITY1_OWNER_H
#define BLINKER_ESP32_WIFIPROV_SECURITY1_OWNER_H

#include <protocomm_security.h>
#include <stdint.h>
#include <string.h>

namespace blinker {

enum class Esp32WiFiProvRekeyEndpointKind : uint8_t { None, BlinkerConfig, SdkConfig };

struct Esp32WiFiProvRekeyNativeOwner {
    uint32_t session;
    uint32_t generation;
};

// Candidate-only synchronous Security1 vtable hook. All callbacks, including
// the protected original protocomm request, run serialized on the BLE host.
// Arm before starting its exclusive manager; disarm only after real stop.
class Esp32WiFiProvSecurity1Owner {
public:
    void arm() { armed_ = true; }
    void disarm() { armed_ = false; invalidate(); }

    void opened(protocomm_security_handle_t handle, uint32_t session) {
        if (!armed_ || handle == nullptr || !advance()) return;
        invalidate();
        handle_ = handle;
        session_ = session;
    }
    void closed(protocomm_security_handle_t handle, uint32_t session) {
        if (matches(handle, session)) invalidate();
    }
    void handshake(protocomm_security_handle_t handle, uint32_t session,
                   const protocomm_security1_params_t* pop) {
        if (!matches(handle, session)) return;
        // A new handshake under a recycled handle never inherits a nonce or
        // the previous scope. A later SDK decrypt must still actually succeed.
        decrypted_ = false;
        popBound_ = advance() && pop != nullptr && pop->data != nullptr && pop->len != 0U;
    }

    bool beginRequest(Esp32WiFiProvRekeyEndpointKind endpoint, uint32_t session) {
        if (!armed_ || handle_ == nullptr || session != session_ || !popBound_ ||
            endpoint == Esp32WiFiProvRekeyEndpointKind::None || endpoint_ != Esp32WiFiProvRekeyEndpointKind::None)
            return false;
        endpoint_ = endpoint;
        decrypted_ = false;
        return true;
    }
    void endRequest() { endpoint_ = Esp32WiFiProvRekeyEndpointKind::None; decrypted_ = false; }
    void decrypted(protocomm_security_handle_t handle, uint32_t session) {
        if (matches(handle, session) && endpoint_ != Esp32WiFiProvRekeyEndpointKind::None && popBound_)
            decrypted_ = true;
    }
    bool current(Esp32WiFiProvRekeyEndpointKind endpoint, uint32_t session, Esp32WiFiProvRekeyNativeOwner& output) const {
        if (!armed_ || !decrypted_ || endpoint == Esp32WiFiProvRekeyEndpointKind::None ||
            endpoint != endpoint_ || handle_ == nullptr || session != session_ || !popBound_) return false;
        output = {session_, generation_};
        return true;
    }

private:
    bool matches(protocomm_security_handle_t handle, uint32_t session) const {
        return armed_ && handle_ != nullptr && handle_ == handle && session_ == session;
    }
    void invalidate() {
        handle_ = nullptr;
        popBound_ = false;
        endRequest();
    }
    bool advance() {
        if (generation_ == UINT32_MAX) { invalidate(); return false; }
        ++generation_;
        return true;
    }

    protocomm_security_handle_t handle_ = nullptr;
    uint32_t session_ = 0U;
    uint32_t generation_ = 0U;
    Esp32WiFiProvRekeyEndpointKind endpoint_ = Esp32WiFiProvRekeyEndpointKind::None;
    bool armed_ = false;
    bool popBound_ = false;
    bool decrypted_ = false;
};

// Candidate link seam: manager.c takes &protocomm_security1, and the pinned
// NimBLE archive calls new/close through that object, not protocomm_open/close.
// Binding::originalSecurity1() supplies __real_protocomm_security1;
// Binding::owner() supplies the one active candidate, or null to only forward.
template <typename Binding>
class Esp32WiFiProvSecurity1OwnerHook {
public:
    static protocomm_security_t wrap() {
        protocomm_security_t result = Binding::originalSecurity1();
        result.new_transport_session = &open;
        result.close_transport_session = &close;
        result.security_req_handler = &handshake;
        result.decrypt = &decrypt;
        return result;
    }

    static Esp32WiFiProvRekeyEndpointKind endpoint(const char* name) {
        if (name == nullptr) return Esp32WiFiProvRekeyEndpointKind::None;
        if (strcmp(name, "blinker-config") == 0) return Esp32WiFiProvRekeyEndpointKind::BlinkerConfig;
        if (strcmp(name, "prov-config") == 0) return Esp32WiFiProvRekeyEndpointKind::SdkConfig;
        return Esp32WiFiProvRekeyEndpointKind::None;
    }

    // Outer linker wrapper for actual protocomm_req_handle; scopes the precise
    // name to the original decrypt+dispatch call, never to a previous request.
    template <typename Original, typename Context>
    static esp_err_t request(Original original, Context* context, const char* name,
        uint32_t session, const uint8_t* input, ssize_t size, uint8_t** output, ssize_t* written) {
        auto* const owner = Binding::owner();
        const auto kind = endpoint(name);
        const bool scoped = owner != nullptr && kind != Esp32WiFiProvRekeyEndpointKind::None;
        if (scoped && !owner->beginRequest(kind, session)) {
            if (output != nullptr) *output = nullptr;
            if (written != nullptr) *written = 0;
            return ESP_ERR_INVALID_STATE;
        }
        const esp_err_t result = original(context, name, session, input, size, output, written);
        if (scoped) owner->endRequest();
        return result;
    }

private:
    static esp_err_t open(protocomm_security_handle_t handle, uint32_t session) {
        const auto original = Binding::originalSecurity1().new_transport_session;
        if (original == nullptr) return ESP_ERR_INVALID_STATE;
        const esp_err_t result = original(handle, session);
        auto* const owner = Binding::owner();
        if (result == ESP_OK && owner != nullptr) owner->opened(handle, session);
        return result;
    }
    static esp_err_t close(protocomm_security_handle_t handle, uint32_t session) {
        auto* const owner = Binding::owner();
        if (owner != nullptr) owner->closed(handle, session);
        const auto original = Binding::originalSecurity1().close_transport_session;
        return original == nullptr ? ESP_ERR_INVALID_STATE : original(handle, session);
    }
    static esp_err_t handshake(protocomm_security_handle_t handle, const void* params, uint32_t session,
        const uint8_t* input, ssize_t size, uint8_t** output, ssize_t* written, void* data) {
        auto* const owner = Binding::owner();
        if (owner != nullptr) owner->handshake(handle, session, static_cast<const protocomm_security1_params_t*>(params));
        const auto original = Binding::originalSecurity1().security_req_handler;
        return original == nullptr ? ESP_ERR_INVALID_STATE : original(handle, params, session, input, size, output, written, data);
    }
    static esp_err_t decrypt(protocomm_security_handle_t handle, uint32_t session,
        const uint8_t* input, ssize_t size, uint8_t** output, ssize_t* written) {
        const auto original = Binding::originalSecurity1().decrypt;
        if (original == nullptr) return ESP_ERR_INVALID_STATE;
        const esp_err_t result = original(handle, session, input, size, output, written);
        auto* const owner = Binding::owner();
        if (result == ESP_OK && owner != nullptr) owner->decrypted(handle, session);
        return result;
    }
};

} // namespace blinker

#endif
