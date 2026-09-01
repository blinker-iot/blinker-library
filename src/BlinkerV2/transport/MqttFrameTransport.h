#ifndef BLINKER_TRANSPORT_MQTTFRAMETRANSPORT_H
#define BLINKER_TRANSPORT_MQTTFRAMETRANSPORT_H

#include "../interface/IClock.h"
#include "../interface/IMqttClient.h"
#include "../protocol/bbp2/Frame.h"
#include "IFrameTransport.h"

namespace blinker {

struct MqttFrameTransportConfig {
    uint16_t maxFrameSize;
    uint16_t keepAliveSeconds;
    uint32_t reconnectDelayMillis;
    uint8_t subscribeQos;
    uint8_t publishQos;

    MqttFrameTransportConfig()
        : maxFrameSize(1024),
          keepAliveSeconds(30),
          reconnectDelayMillis(5000),
          subscribeQos(0),
          publishQos(0) {}
};

class MqttFrameTransport : public IFrameTransport {
public:
    MqttFrameTransport(
        IMqttClient& mqtt,
        IClock& clock,
        const MqttFrameTransportConfig& config =
            MqttFrameTransportConfig());

    Result configure(const SessionCredentials& credentials);

    // One product-owned side channel may share the authenticated MQTT
    // connection and callback. Edge Hub management uses this seam so the
    // product does not construct a second MQTT client or steal the adapter's
    // single message callback.
    Result subscribeSideChannel(
        StringView topic,
        size_t maximumPayloadSize,
        uint8_t qos);
    Result publishSideChannel(
        StringView topic,
        ByteView payload,
        const MqttPublishOptions& options);
    void setSideChannelHandler(
        MqttMessageHandler handler,
        void* context);

    // Product connectivity code owns WiFi/Ethernet/cellular state and updates
    // this cached gate. Keeping platform status calls outside the protocol
    // transport prevents an unavailable link from triggering blocking
    // DNS/TCP/TLS work while local transports still need service.
    void setNetworkAvailable(bool available);
    bool networkAvailable() const { return networkAvailable_; }

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    TransportState state() const override;
    ErrorCode lastError() const { return lastError_; }
    TransportCapabilities capabilities() const override;
    uint32_t connectionGeneration() const;
    Result send(ByteView frame, const SendTarget& target) override;
    void setReceiver(FrameReceiver receiver, void* context) override;
    void setSessionHandlers(
        FrameSessionHandler connected,
        FrameSessionHandler disconnected,
        void* context) override;

private:
    static void messageThunk(
        void* context,
        StringView topic,
        ByteView payload,
        const MqttMessageInfo& info);

    void onMessage(
        StringView topic,
        ByteView payload,
        const MqttMessageInfo& info);

    Result connectNow();
    void notifyConnected();
    void notifyDisconnected();
    void enterBackoff(uint32_t now);
    static bool isRetryable(ErrorCode error);
    static bool equal(StringView left, StringView right);
    static bool valid(StringView value);

    IMqttClient& mqtt_;
    IClock& clock_;
    MqttFrameTransportConfig config_;
    SessionCredentials credentials_;
    FrameReceiver receiver_;
    void* receiverContext_;
    FrameSessionHandler sessionConnected_;
    FrameSessionHandler sessionDisconnected_;
    void* sessionContext_;
    MqttMessageHandler sideChannelHandler_;
    void* sideChannelContext_;
    TransportState state_;
    uint32_t connectionGeneration_;
    uint32_t lastConnectAttempt_;
    bool configured_;
    bool started_;
    bool attemptedConnect_;
    bool networkAvailable_;
    bool sessionActive_;
    ErrorCode lastError_;
};

} // namespace blinker

#endif
