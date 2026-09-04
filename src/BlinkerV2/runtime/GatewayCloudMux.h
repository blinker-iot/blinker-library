#ifndef BLINKER_RUNTIME_GATEWAYCLOUDMUX_H
#define BLINKER_RUNTIME_GATEWAYCLOUDMUX_H

#include "../transport/MqttFrameTransport.h"

namespace blinker {

enum class GatewayCloudChannel : uint8_t {
    Management = 0U,
    ChildRoute = 1U,
    PermitJoinRelay = 2U
};

// Edge-Hub-only fan-out for the single authenticated MQTT callback. Ordinary
// WiFi products keep MqttFrameTransport unchanged and pay no extra storage.
class GatewayCloudMux {
public:
    explicit GatewayCloudMux(MqttFrameTransport& mqtt);
    ~GatewayCloudMux();

    Result bind(
        GatewayCloudChannel channel,
        MqttMessageHandler handler,
        void* context);
    void unbind(GatewayCloudChannel channel, void* context);

    Result subscribe(
        StringView topic,
        size_t maximumPayloadSize,
        uint8_t qos);
    Result publish(
        StringView topic,
        ByteView payload,
        const MqttPublishOptions& options);

    TransportState state() const { return mqtt_.state(); }
    uint32_t connectionGeneration() const {
        return mqtt_.connectionGeneration();
    }

private:
    struct Slot {
        MqttMessageHandler handler;
        void* context;

        Slot() : handler(nullptr), context(nullptr) {}
    };

    static void messageThunk(
        void* context,
        StringView topic,
        ByteView payload,
        const MqttMessageInfo& info);
    void onMessage(
        StringView topic,
        ByteView payload,
        const MqttMessageInfo& info);
    static size_t slotIndex(GatewayCloudChannel channel);

    MqttFrameTransport& mqtt_;
    Slot slots_[3U];

    GatewayCloudMux(const GatewayCloudMux&);
    GatewayCloudMux& operator=(const GatewayCloudMux&);
};

} // namespace blinker

#endif
