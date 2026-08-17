#ifndef BLINKER_CONTROL_SESSION_H
#define BLINKER_CONTROL_SESSION_H

#include "../core/Span.h"

namespace blinker {

enum class MqttSecurity : uint8_t {
    PlainTcp = 0,
    Tls
};

enum class BrokerKind : uint8_t {
    Unknown = 0,
    Blinker
};

// All views are owned by the control-plane implementation. They remain valid
// until the next successful credential generation or stop operation. Secret
// values must never be emitted through the normal logger.
struct SessionCredentials {
    BrokerKind broker;
    StringView host;
    uint16_t port;
    MqttSecurity security;
    StringView deviceName;
    StringView clientId;
    StringView username;
    StringView password;
    StringView ownerUuid;
    StringView publishTopic;
    StringView subscribeTopic;

    SessionCredentials()
        : broker(BrokerKind::Unknown),
          port(0),
          security(MqttSecurity::Tls) {}
};

} // namespace blinker

#endif
