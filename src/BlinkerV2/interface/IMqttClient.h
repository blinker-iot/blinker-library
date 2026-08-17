#ifndef BLINKER_INTERFACE_IMQTTCLIENT_H
#define BLINKER_INTERFACE_IMQTTCLIENT_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../control/Session.h"

namespace blinker {

struct MqttConnectOptions {
    StringView host;
    uint16_t port;
    MqttSecurity security;
    StringView clientId;
    StringView username;
    StringView password;
    uint16_t keepAliveSeconds;
    bool cleanSession;

    MqttConnectOptions()
        : port(0),
          security(MqttSecurity::Tls),
          keepAliveSeconds(30),
          cleanSession(true) {}
};

struct MqttPublishOptions {
    uint8_t qos;
    bool retained;

    MqttPublishOptions() : qos(0), retained(false) {}
};

struct MqttMessageInfo {
    uint8_t qos;
    bool qosKnown;
    bool retained;
    bool retainedKnown;

    MqttMessageInfo()
        : qos(0),
          qosKnown(false),
          retained(false),
          retainedKnown(false) {}
};

enum class MqttPacketDirection : uint8_t {
    Inbound = 0,
    Outbound = 1
};

typedef void (*MqttMessageHandler)(
    void* context,
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info);

class IMqttClient {
public:
    virtual ~IMqttClient() {}

    // Fixed-buffer MQTT engines may reject a transport configuration before
    // connecting. Streaming/unbounded adapters can keep the default.
    virtual Result validatePacketCapacity(
        StringView,
        size_t,
        uint8_t,
        MqttPacketDirection) const {
        return Result::success();
    }

    virtual Result connect(const MqttConnectOptions& options) = 0;
    virtual void disconnect() = 0;
    virtual bool connected() const = 0;
    virtual Result subscribe(StringView topic, uint8_t qos) = 0;
    virtual Result publish(
        StringView topic,
        ByteView payload,
        const MqttPublishOptions& options) = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual void setMessageHandler(MqttMessageHandler handler, void* context) = 0;
};

} // namespace blinker

#endif
