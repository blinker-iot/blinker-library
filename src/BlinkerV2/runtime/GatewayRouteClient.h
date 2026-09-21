#ifndef BLINKER_RUNTIME_GATEWAYROUTECLIENT_H
#define BLINKER_RUNTIME_GATEWAYROUTECLIENT_H

#include "GatewayCloudMux.h"
#include "../interface/IClock.h"
#include "../protocol/gateway/Contracts.h"

namespace blinker {

// Callbacks borrow the view only until return. A matcher is read-only and
// bounded; a receiver may reset the client or replace its own handler. Neither
// may destroy endpoints, recursively poll, or pump MQTT inside the callback.
// WouldBlock means no acceptance/side effects and permits the same exact retry.
typedef Result (*GatewayRouteMessageReceiver)(void* context, ByteView exactMessage);
typedef bool (*GatewayRouteMessageMatcher)(void* context, ByteView exactMessage);
class GatewayRouteEndpoint;

// One MQTT subscription for all child endpoints. No child buffer, BLE owner,
// credential registry or second socket is hidden in this northbound adapter.
class GatewayRouteClient {
public:
    enum : size_t {
        maximumIngressWaitMillis = 1000U,
        maximumClientIdSize = 95U,
        maximumTopicSize = 128U,
        maximumEnvelopeSize = gateway::kEdgeHubChildRouteEnvelopeMaximumEncodedSize
    };

    GatewayRouteClient(GatewayCloudMux& cloud, IClock& clock);
    ~GatewayRouteClient();
    Result configure(const SessionCredentials& credentials);
    void reset();
    void poll();
    Result publish(ByteView exactEnvelope);

    bool configured() const { return configured_; }
    bool subscribed() const {
        return subscribed_ && cloud_.state() == TransportState::Online &&
               subscribedGeneration_ == cloud_.connectionGeneration();
    }
    uint32_t subscriptionGeneration() const {
        return subscribed() ? subscribedGeneration_ : 0U;
    }
    ErrorCode lastError() const { return lastError_; }
    StringView publishTopic() const;
    StringView subscribeTopic() const;

private:
    friend class GatewayRouteEndpoint;
    static void messageThunk(void*, StringView, ByteView, const MqttMessageInfo&);
    void onMessage(StringView topic, ByteView payload, const MqttMessageInfo& info);
    static bool buildTopic(StringView clientId, const char* suffix, char* output, size_t capacity);
    static bool equal(StringView first, StringView second);
    static Result validateProfile(ByteView encoded, gateway::ChildRouteEnvelopeView& envelope);
    GatewayRouteEndpoint* select(ByteView payload) const;
    void clearPending();

    GatewayCloudMux& cloud_;
    IClock& clock_;
    char publishTopic_[maximumTopicSize + 1U];
    char subscribeTopic_[maximumTopicSize + 1U];
    GatewayRouteEndpoint* endpoints_;
    uint32_t subscribedGeneration_;
    bool configured_;
    bool subscribed_;
    ErrorCode lastError_;

    GatewayRouteClient(const GatewayRouteClient&) = delete;
    GatewayRouteClient& operator=(const GatewayRouteClient&) = delete;
};

// Composition owns a fixed number of endpoints: one bounded ingress slot per
// active child, not per App page or MQTT subscription. No heap/maximum-sized
// global child array. Destroying A unregisters only A; client reset retires all
// MQTT ingress, not other transports or the child native sessions.
class GatewayRouteEndpoint {
public:
    enum : size_t { maximumEnvelopeSize = GatewayRouteClient::maximumEnvelopeSize };
    GatewayRouteEndpoint(GatewayRouteClient& client, GatewayRouteMessageMatcher matcher, void* context);
    ~GatewayRouteEndpoint();
    void setReceiver(GatewayRouteMessageReceiver receiver, void* context);
    bool subscribed() const { return client_ != nullptr && client_->subscribed(); }
    uint32_t subscriptionGeneration() const {
        return client_ != nullptr ? client_->subscriptionGeneration() : 0U;
    }
    Result publish(ByteView exactEnvelope);
    bool hasPendingEnvelope() const { return pendingSize_ != 0U; }
    ErrorCode lastError() const { return lastError_; }

protected:
    uint32_t monotonicMillis() const {
        return client_ != nullptr ? client_->clock_.monotonicMillis() : 0U;
    }

private:
    friend class GatewayRouteClient;
    void clearPending();
    void dispatch();

    GatewayRouteClient* client_;
    GatewayRouteEndpoint* next_;
    GatewayRouteMessageMatcher matcher_;
    void* matcherContext_;
    GatewayRouteMessageReceiver receiver_;
    void* receiverContext_;
    uint8_t pendingEnvelope_[maximumEnvelopeSize];
    size_t pendingSize_;
    uint32_t receivedAtMillis_;
    uint32_t revision_;
    ErrorCode lastError_;

    GatewayRouteEndpoint(const GatewayRouteEndpoint&) = delete;
    GatewayRouteEndpoint& operator=(const GatewayRouteEndpoint&) = delete;
};

} // namespace blinker
#endif
