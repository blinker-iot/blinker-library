#include "GatewayRouteClient.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace gateway_route_client_detail {

const char kTopicPrefix[] = "/edge-hub/";
const char kPublishSuffix[] = "/route/up";
const char kSubscribeSuffix[] = "/route/down";

} // namespace gateway_route_client_detail

GatewayRouteClient::GatewayRouteClient(GatewayCloudMux& cloud)
    : cloud_(cloud), publishTopic_(), subscribeTopic_(), pendingEnvelope_(),
      pendingSize_(0U), receiver_(nullptr), receiverContext_(nullptr),
      subscribedGeneration_(0U), configured_(false), subscribed_(false),
      pending_(false), lastError_(ErrorCode::Ok) {
    (void)cloud_.bind(
        GatewayCloudChannel::ChildRoute,
        &GatewayRouteClient::messageThunk, this);
}

GatewayRouteClient::~GatewayRouteClient() {
    cloud_.unbind(GatewayCloudChannel::ChildRoute, this);
    reset();
}

Result GatewayRouteClient::configure(
    const SessionCredentials& credentials) {
    reset();
    if (credentials.security != MqttSecurity::Tls ||
        credentials.clientId.data == nullptr ||
        credentials.clientId.empty() ||
        credentials.clientId.size > maximumClientIdSize ||
        !buildTopic(
            credentials.clientId,
            gateway_route_client_detail::kPublishSuffix,
            publishTopic_, sizeof(publishTopic_)) ||
        !buildTopic(
            credentials.clientId,
            gateway_route_client_detail::kSubscribeSuffix,
            subscribeTopic_, sizeof(subscribeTopic_))) {
        lastError_ = ErrorCode::InvalidArgument;
        return Result::failure(lastError_);
    }
    configured_ = true;
    return Result::success();
}

void GatewayRouteClient::reset() {
    clearPending();
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(publishTopic_), sizeof(publishTopic_)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(subscribeTopic_), sizeof(subscribeTopic_)));
    subscribedGeneration_ = 0U;
    configured_ = false;
    subscribed_ = false;
    lastError_ = ErrorCode::Ok;
}

void GatewayRouteClient::poll() {
    if (!configured_) return;
    if (cloud_.state() != TransportState::Online) {
        subscribed_ = false;
        subscribedGeneration_ = 0U;
        clearPending();
        return;
    }
    const uint32_t generation = cloud_.connectionGeneration();
    if (!subscribed_ || subscribedGeneration_ != generation) {
        subscribed_ = false;
        Result result = cloud_.subscribe(
            subscribeTopic(), maximumEnvelopeSize, 0U);
        if (!result) {
            lastError_ = result.code();
            return;
        }
        subscribed_ = true;
        subscribedGeneration_ = generation;
    }
    if (!pending_) {
        lastError_ = ErrorCode::Ok;
        return;
    }
    const ByteView message(pendingEnvelope_, pendingSize_);
    const Result result = receiver_ == nullptr
                              ? Result::failure(ErrorCode::NotConfigured)
                              : Result::success();
    if (result) receiver_(receiverContext_, message);
    clearPending();
    lastError_ = result.code();
}

Result GatewayRouteClient::publish(ByteView exactEnvelope) {
    if (!configured_ || !subscribed_) {
        return Result::failure(ErrorCode::NotConnected);
    }
    gateway::ChildRouteEnvelopeView envelope;
    Result result = validateProfile(exactEnvelope, envelope);
    if (!result) return result;
    MqttPublishOptions options;
    options.qos = 0U;
    options.retained = false;
    result = cloud_.publish(publishTopic(), exactEnvelope, options);
    lastError_ = result.code();
    return result;
}

void GatewayRouteClient::setReceiver(
    GatewayRouteMessageReceiver receiver,
    void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

StringView GatewayRouteClient::publishTopic() const {
    return StringView(publishTopic_, strlen(publishTopic_));
}

StringView GatewayRouteClient::subscribeTopic() const {
    return StringView(subscribeTopic_, strlen(subscribeTopic_));
}

void GatewayRouteClient::messageThunk(
    void* context,
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    GatewayRouteClient* self = static_cast<GatewayRouteClient*>(context);
    if (self != nullptr) self->onMessage(topic, payload, info);
}

void GatewayRouteClient::onMessage(
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    if (!configured_ || !subscribed_ || pending_ ||
        !equal(topic, subscribeTopic()) || payload.data == nullptr ||
        payload.empty() || payload.size > maximumEnvelopeSize ||
        (info.qosKnown && info.qos != 0U) ||
        (info.retainedKnown && info.retained)) {
        return;
    }
    memcpy(pendingEnvelope_, payload.data, payload.size);
    pendingSize_ = payload.size;
    pending_ = true;
}

bool GatewayRouteClient::buildTopic(
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
        sizeof(gateway_route_client_detail::kTopicPrefix) - 1U;
    const size_t suffixSize = strlen(suffix);
    const size_t required = prefixSize + clientId.size + suffixSize;
    if (required >= capacity) return false;
    memcpy(output, gateway_route_client_detail::kTopicPrefix, prefixSize);
    memcpy(output + prefixSize, clientId.data, clientId.size);
    memcpy(output + prefixSize + clientId.size, suffix, suffixSize);
    output[required] = '\0';
    return true;
}

bool GatewayRouteClient::equal(
    StringView first,
    StringView second) {
    return first.data != nullptr && second.data != nullptr &&
           first.size == second.size &&
           memcmp(first.data, second.data, first.size) == 0;
}

Result GatewayRouteClient::validateProfile(
    ByteView encoded,
    gateway::ChildRouteEnvelopeView& envelope) {
    if (encoded.data == nullptr || encoded.empty() ||
        encoded.size > maximumEnvelopeSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    Result result = gateway::decodeChildRouteEnvelope(encoded, envelope);
    if (result && envelope.innerBbp2Frame.size >
                      gateway::kEdgeHubChildMaximumInnerFrameSize) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    return result;
}

void GatewayRouteClient::clearPending() {
    secureZero(MutableByteSpan(
        pendingEnvelope_, sizeof(pendingEnvelope_)));
    pendingSize_ = 0U;
    pending_ = false;
}

} // namespace blinker
