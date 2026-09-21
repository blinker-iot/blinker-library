#if defined(ARDUINO_ARCH_ESP32) && defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET
#include "Esp32LocalDiscovery.h"
#include <BlinkerV2/core/SecureMemory.h>
#include <mdns.h>

namespace blinker {

Result Esp32LocalDiscovery::start(uint16_t port, bool encryptedMessages) {
    if (advertised_) return Result::failure(ErrorCode::AlreadyExists);
    if (port == 0U) return Result::failure(ErrorCode::InvalidArgument);
    uint8_t random[16];
    Result result = random_.fill(MutableByteSpan(random, sizeof(random)));
    if (!result) return result;
    for (size_t i = 0U; i < sizeof(random); ++i) {
        static const char hex[] = "0123456789abcdef";
        locator_[i * 2U] = hex[random[i] >> 4U]; locator_[i * 2U + 1U] = hex[random[i] & 15U];
    }
    secureZero(MutableByteSpan(random, sizeof(random)));
    if (mdns_init() != ESP_OK) return Result::failure(ErrorCode::NotConfigured);
    char hostname[MDNS_NAME_BUF_LEN] = {};
    const esp_err_t existing = mdns_hostname_get(hostname);
    // init() is idempotent; get() returns INVALID_STATE for a newly initialized
    // engine with no hostname. Do not mistake that normal first boot for failure.
    if (existing != ESP_OK && existing != ESP_ERR_INVALID_STATE) return Result::failure(ErrorCode::InternalError);
    if (hostname[0] == '\0' && mdns_hostname_set(locator_) != ESP_OK) return Result::failure(ErrorCode::NotConfigured);
    mdns_txt_item_t txt[] = {{"v", "2"}, {"path", "/bbp2"},
        {"security", encryptedMessages ? "noise" : "plain"}, {"locator", locator_}};
    advertised_ = mdns_service_add_for_host(locator_, "_blinker-v2", "_tcp", nullptr,
        port, txt, sizeof(txt) / sizeof(txt[0])) == ESP_OK;
    return advertised_ ? Result::success() : Result::failure(ErrorCode::NotConfigured);
}
void Esp32LocalDiscovery::stop() {
    if (advertised_) mdns_service_remove_for_host(locator_, "_blinker-v2", "_tcp", nullptr);
    advertised_ = false; // Never rename an existing host, MDNS.end(), mdns_free() or remove_all().
}

} // namespace blinker
#endif
