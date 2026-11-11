#ifndef BLINKER_ESP32_WIFIPROV_CONFIG_FENCE_H
#define BLINKER_ESP32_WIFIPROV_CONFIG_FENCE_H

#include <network_provisioning/network_config.h>

#if !CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI || CONFIG_NETWORK_PROV_NETWORK_TYPE_THREAD
#error "The scoped WiFiProv Config fence requires the Wi-Fi-only SDK handler ABI"
#endif

namespace blinker {

// Implemented by the candidate's serialized setup owner. The permit must
// already be bound to the committed intent and this exact native session.
class IEsp32WiFiProvConfigFence {
public:
    virtual ~IEsp32WiFiProvConfigFence() {}
    virtual bool allowStatus(uint32_t session) = 0;
    virtual bool beginSet(uint32_t session) = 0;
    virtual void finishSet(uint32_t session, bool success) = 0;
    // Durably consume the exact attempt before permitting the SDK write.
    virtual bool beginApply(uint32_t session) = 0;
};

// Candidate-only seam for the original prov-config endpoint. Keep SDK parsing,
// status and native context unchanged; intercept its typed calls before the
// original Set or Apply can touch configuration. The SDK borrows priv_data
// synchronously, so the scoped delegate cannot outlive this request.
inline esp_err_t forwardEsp32WiFiProvConfig(
    decltype(&network_prov_config_data_handler) sdk,
    IEsp32WiFiProvConfigFence& fence, uint32_t session,
    const uint8_t* input, ssize_t inputSize, uint8_t** output, ssize_t* outputSize,
    void* privateData) {
    if (output == nullptr || outputSize == nullptr) return ESP_ERR_INVALID_ARG;
    *output = nullptr;
    *outputSize = 0;
    if (sdk == nullptr || privateData == nullptr || input == nullptr ||
        inputSize <= 0 || inputSize > 256) return ESP_ERR_INVALID_ARG;

    struct Delegate {
        network_prov_config_handlers_t& original;
        IEsp32WiFiProvConfigFence& fence;
        uint32_t session;

        static Delegate& self(network_prov_ctx_t** ctx) {
            return *reinterpret_cast<Delegate*>(*ctx);
        }

        static esp_err_t status(network_prov_config_get_wifi_data_t* output,
                               network_prov_ctx_t** ctx) {
            Delegate& call = self(ctx);
            if (!call.fence.allowStatus(call.session)) return ESP_ERR_INVALID_STATE;
            return call.original.wifi_get_status_handler(output, &call.original.ctx);
        }

        static esp_err_t set(const network_prov_config_set_wifi_data_t* input,
                            network_prov_ctx_t** ctx) {
            Delegate& call = self(ctx);
            if (!call.fence.beginSet(call.session)) return ESP_ERR_INVALID_STATE;
            const esp_err_t result = call.original.wifi_set_config_handler(input, &call.original.ctx);
            call.fence.finishSet(call.session, result == ESP_OK);
            return result;
        }

        static esp_err_t apply(network_prov_ctx_t** ctx) {
            Delegate& call = self(ctx);
            if (!call.fence.beginApply(call.session)) return ESP_ERR_INVALID_STATE;
            return call.original.wifi_apply_config_handler(&call.original.ctx);
        }
    };

    network_prov_config_handlers_t& original =
        *static_cast<network_prov_config_handlers_t*>(privateData);
    if (original.wifi_get_status_handler == nullptr || original.wifi_set_config_handler == nullptr ||
        original.wifi_apply_config_handler == nullptr) return ESP_ERR_INVALID_STATE;
    Delegate delegate{original, fence, session};
    network_prov_config_handlers_t guarded{&Delegate::status, &Delegate::set, &Delegate::apply,
        reinterpret_cast<network_prov_ctx_t*>(&delegate)};
    return sdk(session, input, inputSize, output, outputSize, &guarded);
}

} // namespace blinker

#endif
