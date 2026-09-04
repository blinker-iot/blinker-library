#include "GatewayManagementClient.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace gateway_management_client_detail {

const char kTopicPrefix[] = "/edge-hub/";
const char kPublishSuffix[] = "/management/up";
const char kSubscribeSuffix[] = "/management/down";

} // namespace gateway_management_client_detail

GatewayManagementClient::GatewayManagementClient(
    GatewayCloudMux& cloud,
    IGatewayManagementDelivery& delivery,
    IGatewayManagementControl& control)
    : cloud_(cloud), delivery_(delivery), control_(control), context_(), publishTopic_(),
      subscribeTopic_(), envelope_(), envelopeSize_(0U), ack_(),
      ackSize_(0U), subscribedGeneration_(0U), configured_(false),
      subscribed_(false),
      envelopePending_(false), ackPending_(false),
      lastError_(ErrorCode::Ok) {
    (void)cloud_.bind(
        GatewayCloudChannel::Management,
        &GatewayManagementClient::messageThunk, this);
}

GatewayManagementClient::~GatewayManagementClient() {
    cloud_.unbind(GatewayCloudChannel::Management, this);
    clearTransient();
}

Result GatewayManagementClient::configure(
    const SessionCredentials& credentials,
    const GatewayAccessDeliveryContext& context) {
    // A SessionCredential generation belongs to the cloud transport, not to
    // the already-proved local child topology. Rebind the MQTT side channel
    // without retiring a valid DirectSecure child session.
    clearCloudBinding();
    if (credentials.security != MqttSecurity::Tls ||
        credentials.clientId.data == nullptr ||
        credentials.clientId.empty() ||
        credentials.clientId.size > maximumClientIdSize ||
        context.deviceKeyVersion == 0U ||
        !buildTopic(
            credentials.clientId,
            gateway_management_client_detail::kPublishSuffix,
            publishTopic_, sizeof(publishTopic_)) ||
        !buildTopic(
            credentials.clientId,
            gateway_management_client_detail::kSubscribeSuffix,
            subscribeTopic_, sizeof(subscribeTopic_))) {
        lastError_ = ErrorCode::InvalidArgument;
        return Result::failure(lastError_);
    }
    context_ = context;
    configured_ = true;
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

void GatewayManagementClient::reset() {
    control_.reset();
    clearCloudBinding();
}

void GatewayManagementClient::clearCloudBinding() {
    clearTransient();
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(publishTopic_), sizeof(publishTopic_)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(subscribeTopic_), sizeof(subscribeTopic_)));
    context_ = GatewayAccessDeliveryContext();
    configured_ = false;
    subscribed_ = false;
    subscribedGeneration_ = 0U;
    lastError_ = ErrorCode::Ok;
}

void GatewayManagementClient::poll() {
    if (!configured_) return;
    if (cloud_.state() != TransportState::Online) {
        subscribed_ = false;
        subscribedGeneration_ = 0U;
        return;
    }
    const uint32_t generation = cloud_.connectionGeneration();
    if (!subscribed_ || subscribedGeneration_ != generation) {
        subscribed_ = false;
        Result result = cloud_.subscribe(
            subscribeTopic(),
            gateway::kGatewayCredentialRenewalEnvelopeMaximumEncodedSize,
            1U);
        if (!result) {
            lastError_ = result.code();
            return;
        }
        subscribed_ = true;
        subscribedGeneration_ = generation;
        lastError_ = ErrorCode::Ok;
    }

    if (envelopePending_ && !ackPending_) {
        size_t written = 0U;
        const ByteView message(envelope_, envelopeSize_);
        gateway::GatewayProofCommandView proofCommand;
        const Result decodedProof = gateway::decodeGatewayProofCommand(
            message, proofCommand);
        gateway::GatewayRevocationCommandView revocationCommand;
        const Result decodedRevocation = decodedProof
            ? Result::failure(ErrorCode::InvalidEncoding)
            : gateway::decodeGatewayRevocationCommand(
                  message, revocationCommand);
        gateway::GatewayPermitJoinCommandView permitJoinCommand;
        const Result decodedPermitJoin =
            (decodedProof || decodedRevocation)
                ? Result::failure(ErrorCode::InvalidEncoding)
                : gateway::decodeGatewayPermitJoinCommand(
                      message, permitJoinCommand);
        gateway::GatewayCredentialRenewalCommandView renewalCommand;
        const Result decodedRenewal =
            (decodedProof || decodedRevocation || decodedPermitJoin)
                ? Result::failure(ErrorCode::InvalidEncoding)
                : gateway::decodeGatewayCredentialRenewalCommand(
                      message, renewalCommand);
        const bool controlCommand =
            decodedProof || decodedRevocation || decodedPermitJoin ||
            decodedRenewal;
        Result result = controlCommand
                            ? control_.handleCommand(message)
                            : delivery_.process(
                                  context_, message,
                                  MutableByteSpan(ack_, sizeof(ack_)),
                                  written);
        secureZero(MutableByteSpan(envelope_, sizeof(envelope_)));
        envelopeSize_ = 0U;
        envelopePending_ = false;
        if (!result) {
            lastError_ = result.code();
            return;
        }
        if (!controlCommand) {
            ackSize_ = written;
            ackPending_ = true;
        }
        lastError_ = ErrorCode::Ok;
    }

    control_.poll();

    const ByteView controlResult = control_.pendingResult();
    if (!ackPending_ && controlResult.empty()) return;
    MqttPublishOptions options;
    // PubSubClient cannot publish QoS 1. The Broker's durable exact-envelope
    // retry makes this application ACK reliable: a lost QoS 0 ACK causes the
    // same envelope to be redelivered and the same durable ACK to be replayed.
    options.qos = 0U;
    options.retained = false;
    const ByteView payload = ackPending_
                                 ? ByteView(ack_, ackSize_)
                                 : controlResult;
    Result result = cloud_.publish(
        publishTopic(), payload, options);
    if (!result) {
        lastError_ = result.code();
        return;
    }
    if (ackPending_) {
        secureZero(MutableByteSpan(ack_, sizeof(ack_)));
        ackSize_ = 0U;
        ackPending_ = false;
    } else {
        control_.markResultPublished();
    }
    lastError_ = ErrorCode::Ok;
}

StringView GatewayManagementClient::publishTopic() const {
    return StringView(publishTopic_, strlen(publishTopic_));
}

StringView GatewayManagementClient::subscribeTopic() const {
    return StringView(subscribeTopic_, strlen(subscribeTopic_));
}

void GatewayManagementClient::messageThunk(
    void* context,
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    if (context != nullptr) {
        static_cast<GatewayManagementClient*>(context)->onMessage(
            topic, payload, info);
    }
}

void GatewayManagementClient::onMessage(
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    if (gateway::permitJoinRelayMessageKind(payload) !=
        gateway::PermitJoinRelayMessageKind::Unknown) {
        return;
    }
    if (!configured_ || !subscribed_ || envelopePending_ || ackPending_ ||
        !control_.pendingResult().empty() ||
        !equal(topic, subscribeTopic()) || payload.data == nullptr ||
        payload.empty() ||
        payload.size >
            gateway::kGatewayCredentialRenewalEnvelopeMaximumEncodedSize ||
        (info.qosKnown && info.qos != 1U) ||
        (info.retainedKnown && info.retained)) {
        return;
    }
    memcpy(envelope_, payload.data, payload.size);
    envelopeSize_ = payload.size;
    envelopePending_ = true;
}

bool GatewayManagementClient::buildTopic(
    StringView clientId,
    const char* suffix,
    char* output,
    size_t capacity) {
    if (clientId.data == nullptr || clientId.empty() || suffix == nullptr ||
        output == nullptr ||
        memchr(clientId.data, '\0', clientId.size) != nullptr) {
        return false;
    }
    const size_t prefixSize =
        sizeof(gateway_management_client_detail::kTopicPrefix) - 1U;
    const size_t suffixSize = strlen(suffix);
    const size_t required = prefixSize + clientId.size + suffixSize;
    if (required >= capacity) return false;
    memcpy(output, gateway_management_client_detail::kTopicPrefix,
           prefixSize);
    memcpy(output + prefixSize, clientId.data, clientId.size);
    memcpy(output + prefixSize + clientId.size, suffix, suffixSize);
    output[required] = '\0';
    return true;
}

bool GatewayManagementClient::equal(
    StringView first,
    StringView second) {
    return first.data != nullptr && second.data != nullptr &&
           first.size == second.size &&
           memcmp(first.data, second.data, first.size) == 0;
}

void GatewayManagementClient::clearTransient() {
    secureZero(MutableByteSpan(envelope_, sizeof(envelope_)));
    secureZero(MutableByteSpan(ack_, sizeof(ack_)));
    envelopeSize_ = 0U;
    ackSize_ = 0U;
    envelopePending_ = false;
    ackPending_ = false;
}

} // namespace blinker
