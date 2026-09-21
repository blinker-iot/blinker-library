#ifndef BLINKER_ESP32_WIFI_LAN_EXTENSION_H
#define BLINKER_ESP32_WIFI_LAN_EXTENSION_H

#if !defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || !BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL
#error "Internal LAN candidate requires the shared LocalAccess cloud authority"
#endif

#include "../../ports/esp32/Esp32LocalWebSocket.h"
#if defined(BLINKER_INTERNAL_LAN_ENCRYPTED) && BLINKER_INTERNAL_LAN_ENCRYPTED
#include "../../ports/esp32/Esp32Crypto.h"
#include <BlinkerV2/security/LocalAccessNoiseSession.h>
#else
#include <BlinkerV2/security/LocalAccessPlainSession.h>
#endif
#include <BlinkerV2/transport/LocalAccessFrameTransport.h>

namespace blinker {
namespace integration {
namespace official_detail {

// One optional self transport. No second credential store, Client, fields,
// cloud login or MQTT session. Reusable by later product compositions.
template <typename Stack>
class Esp32WifiLanExtension {
public:
    explicit Esp32WifiLanExtension(Stack& stack)
        : link_(stack.random(), linkConfig()),
#if defined(BLINKER_INTERNAL_LAN_ENCRYPTED) && BLINKER_INTERNAL_LAN_ENCRYPTED
          records_(crypto_, stack.random()),
#endif
          transport_(link_, stack.clock(), stack.random(), records_,
              stack.cloudTransport().authority(), MutableByteSpan(plaintext_, sizeof(plaintext_)),
              MutableByteSpan(outgoing_, sizeof(outgoing_)), BLINKER_DEVICE_FRAME_SIZE) {
        static_assert(BLINKER_DEVICE_FRAME_SIZE <= 400U, "First LAN profile is bounded to 400 B");
        static_assert(Stack::CloudTransport::securityProfile == Records::profile,
            "Cloud mode declaration must match the physical LAN record session");
    }
    Result attach(Client& client) {
        if (client_ != nullptr) return client_ == &client ? Result::success() : Result::failure(ErrorCode::AlreadyExists);
        const Result result = client.addTransport(transport_);
        if (result) client_ = &client;
        return result;
    }
    uint16_t capabilities() const { return ProductCapabilityLanData; }
private:
#if defined(BLINKER_INTERNAL_LAN_ENCRYPTED) && BLINKER_INTERNAL_LAN_ENCRYPTED
    using Records = security::LocalAccessNoiseSession;
    Esp32MbedTlsCryptoProvider crypto_;
#else
    using Records = security::LocalAccessPlainSession;
#endif
    Esp32LocalWebSocket link_;
    Records records_;
    uint8_t plaintext_[BLINKER_DEVICE_FRAME_SIZE] = {};
    uint8_t outgoing_[BLINKER_DEVICE_FRAME_SIZE + Records::overhead] = {};
    LocalAccessFrameTransport<1U, Records> transport_;
    Client* client_ = nullptr;
    static Esp32LocalWebSocketConfig linkConfig() {
        Esp32LocalWebSocketConfig config;
        config.encryptedMessages = Records::encrypted;
        return config;
    }
};

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
