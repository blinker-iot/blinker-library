#include <stdlib.h>
#include <stdio.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProvConfigFence.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "failure at %d\n", __LINE__); abort(); } } while (0)

namespace {
esp_err_t nativeStatus(network_prov_config_get_wifi_data_t*, network_prov_ctx_t** ctx) {
    ++(*ctx)->statuses;
    return ESP_OK;
}
esp_err_t nativeSet(const network_prov_config_set_wifi_data_t*, network_prov_ctx_t** ctx) {
    ++(*ctx)->sets;
    return ESP_OK;
}
esp_err_t nativeApply(network_prov_ctx_t** ctx) {
    ++(*ctx)->applies;
    return ESP_OK;
}

struct Fence : blinker::IEsp32WiFiProvConfigFence {
    bool allowStatus(uint32_t session) override { return session == 3; }
    bool beginSet(uint32_t session) override { return session == 3 && committed && !set; }
    void finishSet(uint32_t session, bool success) override { CHECK(session == 3); set = success; }
    bool beginApply(uint32_t session) override {
        if (session != 3 || !committed || !set || consumed) return false;
        consumed = true;
        return true;
    }
    bool committed = false;
    bool set = false;
    bool consumed = false;
};

esp_err_t sdkDecode(uint32_t, const uint8_t* bytes, ssize_t size, uint8_t**, ssize_t*, void* data) {
    if (size != 1) return ESP_ERR_INVALID_ARG;
    auto& h = *static_cast<network_prov_config_handlers_t*>(data);
    network_prov_config_get_wifi_data_t status;
    network_prov_config_set_wifi_data_t request;
    switch (*bytes) {
        case 0: return h.wifi_get_status_handler(&status, &h.ctx);
        case 1: return h.wifi_set_config_handler(&request, &h.ctx);
        case 2: return h.wifi_apply_config_handler(&h.ctx);
    }
    return ESP_ERR_INVALID_ARG;
}
} // namespace

int main() {
    network_prov_ctx_t ctx{};
    network_prov_config_handlers_t sdk{&nativeStatus, &nativeSet, &nativeApply, &ctx};
    Fence fence;
    auto call = [&](uint32_t session, uint8_t cmd) {
        uint8_t* out = &cmd;
        ssize_t size = 9;
        const auto result = blinker::forwardEsp32WiFiProvConfig(&sdkDecode, fence, session, &cmd, 1, &out, &size, &sdk);
        CHECK(out == nullptr && size == 0);
        CHECK(sdk.ctx == &ctx && sdk.wifi_set_config_handler == &nativeSet);
        return result;
    };
    CHECK(call(3, 0) == ESP_OK && ctx.statuses == 1);
    CHECK(call(4, 0) != ESP_OK && ctx.statuses == 1);
    CHECK(call(3, 1) != ESP_OK && call(3, 2) != ESP_OK);
    CHECK(ctx.sets == 0 && ctx.applies == 0);
    fence.committed = true;
    CHECK(call(3, 2) != ESP_OK && call(4, 1) != ESP_OK);
    CHECK(call(3, 1) == ESP_OK && call(3, 1) != ESP_OK && ctx.sets == 1);
    CHECK(call(4, 2) != ESP_OK && ctx.applies == 0);
    CHECK(call(3, 2) == ESP_OK && call(3, 2) != ESP_OK && ctx.applies == 1);
    CHECK(call(3, 0) == ESP_OK && ctx.statuses == 2);
    return 0;
}
