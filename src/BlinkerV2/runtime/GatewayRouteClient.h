#ifndef BLINKER_RUNTIME_GATEWAYROUTECLIENT_H
#define BLINKER_RUNTIME_GATEWAYROUTECLIENT_H

#include "GatewayCloudMux.h"
#include "../protocol/gateway/Contracts.h"

namespace blinker {

typedef void (*GatewayRouteMessageReceiver)(
    void* context,
    ByteView exactMessage);

// One bounded ChildRouteEnvelope channel for the G2A single-child profile.
// It owns only MQTT subscription/deferred decode; topology policy and BLE
// session state stay in GatewayChildRouteBridge.
class GatewayRouteClient {
public:
    enum : size_t {
        maximumClientIdSize = 95U,
        maximumTopicSize = 128U,
        maximumEnvelopeSize =
            gateway::kEdgeHubChildRouteEnvelopeMaximumEncodedSize
    };

    explicit GatewayRouteClient(GatewayCloudMux& cloud);
    ~GatewayRouteClient();

    Result configure(const SessionCredentials& credentials);
    void reset();
    void poll();
    Result publish(ByteView exactEnvelope);
    void setReceiver(
        GatewayRouteMessageReceiver receiver,
        void* context);

    bool configured() const { return configured_; }
    bool subscribed() const {
        return subscribed_ &&
               subscribedGeneration_ == cloud_.connectionGeneration();
    }
    uint32_t subscriptionGeneration() const {
        return subscribed() ? subscribedGeneration_ : 0U;
    }
    bool hasPendingEnvelope() const { return pending_; }
    ErrorCode lastError() const { return lastError_; }
    StringView publishTopic() const;
    StringView subscribeTopic() const;

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
    static bool buildTopic(
        StringView clientId,
        const char* suffix,
        char* output,
        size_t capacity);
    static bool equal(StringView first, StringView second);
    static Result validateProfile(
        ByteView encoded,
        gateway::ChildRouteEnvelopeView& envelope);
    void clearPending();

    GatewayCloudMux& cloud_;
    char publishTopic_[maximumTopicSize + 1U];
    char subscribeTopic_[maximumTopicSize + 1U];
    uint8_t pendingEnvelope_[maximumEnvelopeSize];
    size_t pendingSize_;
    GatewayRouteMessageReceiver receiver_;
    void* receiverContext_;
    uint32_t subscribedGeneration_;
    bool configured_;
    bool subscribed_;
    bool pending_;
    ErrorCode lastError_;

    GatewayRouteClient(const GatewayRouteClient&);
    GatewayRouteClient& operator=(const GatewayRouteClient&);
};

} // namespace blinker

#endif
