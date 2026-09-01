#ifndef BLINKER_RUNTIME_GATEWAYMANAGEMENTCLIENT_H
#define BLINKER_RUNTIME_GATEWAYMANAGEMENTCLIENT_H

#include "GatewayAccessDeliveryProcessor.h"
#include "GatewayCloudMux.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// MQTTS side-channel client for one ESP32-class Edge Hub. It defers flash and
// crypto work out of the MQTT callback, owns fixed buffers, and publishes the
// durable application ACK only after GatewayAccessDeliveryProcessor succeeds.
class GatewayManagementClient {
public:
    enum : size_t {
        maximumClientIdSize = 95U,
        maximumTopicSize = 128U
    };

    GatewayManagementClient(
        GatewayCloudMux& cloud,
        GatewayAccessDeliveryProcessor& delivery,
        IGatewayManagementControl& control);
    ~GatewayManagementClient();

    Result configure(
        const SessionCredentials& credentials,
        const GatewayAccessDeliveryContext& context);
    void reset();
    void poll();

    bool configured() const { return configured_; }
    bool subscribed() const {
        return subscribed_ &&
               subscribedGeneration_ == cloud_.connectionGeneration();
    }
    bool hasPendingEnvelope() const { return envelopePending_; }
    bool hasPendingAck() const { return ackPending_; }
    bool hasPendingControlResult() const {
        return !control_.pendingResult().empty();
    }
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
    void clearTransient();
    void clearCloudBinding();

    GatewayCloudMux& cloud_;
    GatewayAccessDeliveryProcessor& delivery_;
    IGatewayManagementControl& control_;
    GatewayAccessDeliveryContext context_;
    char publishTopic_[maximumTopicSize + 1U];
    char subscribeTopic_[maximumTopicSize + 1U];
    uint8_t envelope_[gateway::kGatewayAccessEnvelopeMaximumEncodedSize];
    size_t envelopeSize_;
    uint8_t ack_[gateway::kGatewayAccessDeliveryAckMaximumEncodedSize];
    size_t ackSize_;
    uint32_t subscribedGeneration_;
    bool configured_;
    bool subscribed_;
    bool envelopePending_;
    bool ackPending_;
    ErrorCode lastError_;

    GatewayManagementClient(const GatewayManagementClient&);
    GatewayManagementClient& operator=(const GatewayManagementClient&);
};

} // namespace blinker

#endif
