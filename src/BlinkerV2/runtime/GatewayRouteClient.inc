#include "GatewayRouteClient.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace gateway_route_client_detail {

const char kTopicPrefix[] = "/edge-hub/";
const char kPublishSuffix[] = "/route/up";
const char kSubscribeSuffix[] = "/route/down";

} // namespace gateway_route_client_detail

GatewayRouteClient::GatewayRouteClient(GatewayCloudMux& cloud, IClock& clock)
    : cloud_(cloud), clock_(clock), publishTopic_(), subscribeTopic_(),
      endpoints_(nullptr), subscribedGeneration_(0U), configured_(false),
      subscribed_(false), lastError_(ErrorCode::Ok) {
    (void)cloud_.bind(
        GatewayCloudChannel::ChildRoute,
        &GatewayRouteClient::messageThunk, this);
}

GatewayRouteClient::~GatewayRouteClient() {
    cloud_.unbind(GatewayCloudChannel::ChildRoute, this);
    reset();
    while (endpoints_ != nullptr) {
        GatewayRouteEndpoint* endpoint = endpoints_;
        endpoints_ = endpoint->next_;
        endpoint->client_ = nullptr;
        endpoint->next_ = nullptr;
    }
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
        // A reconnect can finish between two route polls. Never carry the
        // previous session's admission or command into its replacement.
        clearPending();
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
    lastError_ = ErrorCode::Ok;
    for (GatewayRouteEndpoint* endpoint = endpoints_; endpoint != nullptr; endpoint = endpoint->next_) {
        if (!subscribed()) { clearPending(); return; }
        endpoint->dispatch();
    }
}

Result GatewayRouteClient::publish(ByteView exactEnvelope) {
    if (!configured_ || !subscribed()) {
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
    if (!configured_ || !subscribed() ||
        !equal(topic, subscribeTopic()) || payload.data == nullptr ||
        payload.empty() || payload.size > maximumEnvelopeSize ||
        (info.qosKnown && info.qos != 0U) ||
        (info.retainedKnown && info.retained)) return;

    GatewayRouteEndpoint* endpoint = select(payload);
    if (endpoint == nullptr) return; // Unknown or ambiguous target: never broadcast.
    if (endpoint->hasPendingEnvelope()) {
        endpoint->lastError_ = ErrorCode::WouldBlock;
        return;
    }
    memcpy(endpoint->pendingEnvelope_, payload.data, payload.size);
    endpoint->pendingSize_ = payload.size;
    endpoint->receivedAtMillis_ = clock_.monotonicMillis();
    ++endpoint->revision_;
}

GatewayRouteEndpoint* GatewayRouteClient::select(ByteView payload) const {
    GatewayRouteEndpoint* selected = nullptr;
    for (GatewayRouteEndpoint* endpoint = endpoints_; endpoint != nullptr; endpoint = endpoint->next_) {
        if (endpoint->receiver_ == nullptr || endpoint->matcher_ == nullptr ||
            !endpoint->matcher_(endpoint->matcherContext_, payload)) continue;
        if (selected != nullptr) return nullptr;
        selected = endpoint;
    }
    return selected;
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
    if (encoded.data[0] == 0x89U) {
        gateway::GatewayRouteClosureView closure;
        Result result = gateway::decodeGatewayRouteClosure(encoded, closure);
        if (result && closure.acknowledgment) {
            result = Result::failure(ErrorCode::InvalidArgument);
        }
        return result;
    }
    if (encoded.data[0] == 0x8aU) {
        gateway::GatewayRouteLeaseView lease;
        Result result = gateway::decodeGatewayRouteLease(encoded, lease);
        if (result && !lease.route.acknowledgment) {
            result = Result::failure(ErrorCode::InvalidArgument);
        }
        return result;
    }
    if (encoded.size >= 3U && encoded.data[0] == 0x8bU &&
        encoded.data[1] == 1U && encoded.data[2] == 4U) {
        gateway::GatewayExecutionDemandView demand;
        Result result = gateway::decodeGatewayExecutionDemand(encoded, demand);
        if (result && demand.operation != gateway::GatewayExecutionOperation::Challenge) {
            result = Result::failure(ErrorCode::InvalidArgument);
        }
        return result;
    }
    if (encoded.data[0] == 0x8cU) {
        gateway::GatewayDirectYieldView control;
        Result result = gateway::decodeGatewayDirectYield(encoded, control);
        if (result && control.demand.operation != gateway::GatewayExecutionOperation::Challenge &&
            control.demand.operation != gateway::GatewayExecutionOperation::Yielded) {
            result = Result::failure(ErrorCode::InvalidArgument);
        }
        return result;
    }
    Result result = gateway::decodeChildRouteEnvelope(encoded, envelope);
    if (result && envelope.innerBbp2Frame.size >
                      gateway::kEdgeHubChildMaximumInnerFrameSize) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    return result;
}

void GatewayRouteClient::clearPending() {
    for (GatewayRouteEndpoint* endpoint = endpoints_; endpoint != nullptr; endpoint = endpoint->next_) {
        endpoint->clearPending();
    }
}

GatewayRouteEndpoint::GatewayRouteEndpoint(
    GatewayRouteClient& client, GatewayRouteMessageMatcher matcher, void* context)
    : client_(&client), next_(client.endpoints_), matcher_(matcher), matcherContext_(context),
      receiver_(nullptr), receiverContext_(nullptr), pendingEnvelope_(), pendingSize_(0U),
      receivedAtMillis_(0U), revision_(0U), lastError_(ErrorCode::Ok) {
    client.endpoints_ = this;
}

GatewayRouteEndpoint::~GatewayRouteEndpoint() {
    if (client_ != nullptr) {
        GatewayRouteEndpoint** cursor = &client_->endpoints_;
        while (*cursor != nullptr && *cursor != this) cursor = &(*cursor)->next_;
        if (*cursor == this) *cursor = next_;
    }
    clearPending();
}

void GatewayRouteEndpoint::setReceiver(GatewayRouteMessageReceiver receiver, void* context) {
    if (receiver_ != receiver || receiverContext_ != context) clearPending();
    receiver_ = receiver;
    receiverContext_ = context;
}

Result GatewayRouteEndpoint::publish(ByteView exactEnvelope) {
    if (!subscribed()) return Result::failure(ErrorCode::NotConnected);
    if (client_->select(exactEnvelope) != this) {
        return Result::failure(ErrorCode::StateConflict);
    }
    return client_->publish(exactEnvelope);
}

void GatewayRouteEndpoint::dispatch() {
    if (!hasPendingEnvelope()) return;
    if (static_cast<uint32_t>(monotonicMillis() - receivedAtMillis_) >=
        GatewayRouteClient::maximumIngressWaitMillis) {
        clearPending();
        lastError_ = ErrorCode::WouldBlock;
        return;
    }
    const ByteView message(pendingEnvelope_, pendingSize_);
    // Revalidate selection after record/receiver changes. Identity selection
    // never substitutes for the bridge's full authority and sequence checks.
    if (client_->select(message) != this) {
        clearPending();
        lastError_ = ErrorCode::StateConflict;
        return;
    }
    const uint32_t revision = revision_;
    const Result result = receiver_(receiverContext_, message);
    if (revision_ != revision) return; // reset/rebind retired this exact delivery.
    if (!subscribed()) { clearPending(); return; }
    if (result.code() != ErrorCode::WouldBlock) clearPending();
    lastError_ = result.code();
}

void GatewayRouteEndpoint::clearPending() {
    secureZero(MutableByteSpan(pendingEnvelope_, pendingSize_));
    pendingSize_ = 0U;
    receivedAtMillis_ = 0U;
    ++revision_;
    lastError_ = ErrorCode::Ok;
}

} // namespace blinker
