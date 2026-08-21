#include "MqttFrameTransport.h"

#include <string.h>

namespace blinker {

MqttFrameTransport::MqttFrameTransport(
    IMqttClient& mqtt,
    IClock& clock,
    const MqttFrameTransportConfig& config)
    : mqtt_(mqtt),
      clock_(clock),
      config_(config),
      receiver_(nullptr),
      receiverContext_(nullptr),
      sessionConnected_(nullptr),
      sessionDisconnected_(nullptr),
      sessionContext_(nullptr),
      state_(TransportState::Stopped),
      lastConnectAttempt_(0),
      configured_(false),
      started_(false),
      attemptedConnect_(false),
      networkAvailable_(true),
      sessionActive_(false),
      lastError_(ErrorCode::Ok) {
    mqtt_.setMessageHandler(&MqttFrameTransport::messageThunk, this);
}

Result MqttFrameTransport::configure(
    const SessionCredentials& credentials) {
    if (started_) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (credentials.broker != BrokerKind::Blinker ||
        !valid(credentials.host) ||
        credentials.port == 0 ||
        !valid(credentials.clientId) ||
        !valid(credentials.username) ||
        !valid(credentials.password) ||
        !valid(credentials.publishTopic) ||
        !valid(credentials.subscribeTopic)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (config_.maxFrameSize < bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (config_.publishQos > 2 || config_.subscribeQos > 2) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    Result capacity = mqtt_.validatePacketCapacity(
        credentials.publishTopic,
        config_.maxFrameSize,
        config_.publishQos,
        MqttPacketDirection::Outbound);
    if (!capacity) {
        return capacity;
    }
    capacity = mqtt_.validatePacketCapacity(
        credentials.subscribeTopic,
        config_.maxFrameSize,
        config_.subscribeQos,
        MqttPacketDirection::Inbound);
    if (!capacity) {
        return capacity;
    }

    credentials_ = credentials;
    configured_ = true;
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

void MqttFrameTransport::setNetworkAvailable(bool available) {
    if (networkAvailable_ == available) {
        return;
    }

    networkAvailable_ = available;
    attemptedConnect_ = false;
    if (!started_) {
        return;
    }

    if (!available) {
        notifyDisconnected();
        mqtt_.disconnect();
        state_ = TransportState::Starting;
        lastError_ = ErrorCode::NotConnected;
        return;
    }

    state_ = TransportState::Starting;
    lastError_ = ErrorCode::Ok;
}

Result MqttFrameTransport::start() {
    if (!configured_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (started_) {
        return Result::success();
    }

    started_ = true;
    attemptedConnect_ = false;
    state_ = TransportState::Starting;
    lastError_ = ErrorCode::Ok;

    // Starting a transport must not synchronously wait for DNS, TCP, TLS or an
    // MQTT CONNACK. Client::begin() starts every registered transport in order;
    // deferring the first connection attempt to poll() lets BLE/local transports
    // finish starting even when the cloud path is unavailable.
    return Result::success();
}

void MqttFrameTransport::stop() {
    started_ = false;
    attemptedConnect_ = false;
    notifyDisconnected();
    mqtt_.disconnect();
    state_ = TransportState::Stopped;
    lastError_ = ErrorCode::Ok;
}

void MqttFrameTransport::poll(uint32_t budgetMicros) {
    if (!started_ || !networkAvailable_) {
        return;
    }

    mqtt_.poll(budgetMicros);
    const uint32_t now = clock_.monotonicMillis();

    if (state_ == TransportState::Online && !mqtt_.connected()) {
        notifyDisconnected();
        enterBackoff(now);
    }

    if (state_ != TransportState::Backoff &&
        state_ != TransportState::Starting) {
        return;
    }

    if (!attemptedConnect_ ||
        static_cast<uint32_t>(now - lastConnectAttempt_) >=
            config_.reconnectDelayMillis) {
        connectNow();
    }
}

TransportState MqttFrameTransport::state() const {
    return state_;
}

TransportCapabilities MqttFrameTransport::capabilities() const {
    TransportCapabilities value;
    value.maxFrameSize = config_.maxFrameSize;
    value.features =
        TransportFeatureBinary |
        TransportFeatureCloud |
        TransportFeatureAuthenticated;
    return value;
}

Result MqttFrameTransport::send(
    ByteView frame,
    const SendTarget&) {
    if (!started_ || state_ != TransportState::Online ||
        !mqtt_.connected()) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (frame.data == nullptr ||
        frame.size < bbp2::kBaseHeaderSize ||
        frame.size > config_.maxFrameSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    bbp2::FrameView parsed;
    const Result frameResult = bbp2::parseFrame(frame, parsed);
    if (!frameResult) {
        return frameResult;
    }

    MqttPublishOptions options;
    options.qos = config_.publishQos;
    options.retained = false;
    return mqtt_.publish(
        credentials_.publishTopic,
        frame,
        options);
}

void MqttFrameTransport::setReceiver(
    FrameReceiver receiver,
    void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

void MqttFrameTransport::setSessionHandlers(
    FrameSessionHandler connected,
    FrameSessionHandler disconnected,
    void* context) {
    sessionConnected_ = connected;
    sessionDisconnected_ = disconnected;
    sessionContext_ = context;
}

void MqttFrameTransport::messageThunk(
    void* context,
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    if (context != nullptr) {
        static_cast<MqttFrameTransport*>(context)->onMessage(
            topic,
            payload,
            info);
    }
}

void MqttFrameTransport::onMessage(
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    if (!started_ ||
        state_ != TransportState::Online ||
        (info.retainedKnown && info.retained) ||
        !equal(topic, credentials_.subscribeTopic) ||
        payload.data == nullptr ||
        payload.size > config_.maxFrameSize) {
        return;
    }

    bbp2::FrameView frame;
    if (!bbp2::parseFrame(payload, frame)) {
        return;
    }

    if (receiver_ != nullptr) {
        RxContext rx;
        rx.sessionId = 1;
        rx.authenticated = true;
        receiver_(receiverContext_, payload, rx);
    }
}

Result MqttFrameTransport::connectNow() {
    const uint32_t now = clock_.monotonicMillis();
    attemptedConnect_ = true;
    lastConnectAttempt_ = now;
    state_ = TransportState::Starting;

    MqttConnectOptions options;
    options.host = credentials_.host;
    options.port = credentials_.port;
    options.security = credentials_.security;
    options.clientId = credentials_.clientId;
    options.username = credentials_.username;
    options.password = credentials_.password;
    options.keepAliveSeconds = config_.keepAliveSeconds;
    options.cleanSession = true;

    Result result = mqtt_.connect(options);
    if (!result) {
        lastError_ = result.code();
        if (isRetryable(result.code())) {
            enterBackoff(now);
        } else {
            state_ = TransportState::Error;
        }
        return result;
    }

    result = mqtt_.subscribe(
        credentials_.subscribeTopic,
        config_.subscribeQos);
    if (!result) {
        lastError_ = result.code();
        mqtt_.disconnect();
        if (isRetryable(result.code())) {
            enterBackoff(now);
        } else {
            state_ = TransportState::Error;
        }
        return result;
    }

    state_ = TransportState::Online;
    lastError_ = ErrorCode::Ok;
    notifyConnected();
    return Result::success();
}

void MqttFrameTransport::notifyConnected() {
    if (sessionActive_) return;
    sessionActive_ = true;
    if (sessionConnected_ == nullptr) return;
    RxContext session;
    session.sessionId = 1U;
    session.authenticated = true;
    sessionConnected_(sessionContext_, session);
}

void MqttFrameTransport::notifyDisconnected() {
    if (!sessionActive_) return;
    sessionActive_ = false;
    if (sessionDisconnected_ == nullptr) return;
    RxContext session;
    session.sessionId = 1U;
    session.authenticated = true;
    sessionDisconnected_(sessionContext_, session);
}

void MqttFrameTransport::enterBackoff(uint32_t now) {
    state_ = TransportState::Backoff;
    lastConnectAttempt_ = now;
    attemptedConnect_ = true;
}

bool MqttFrameTransport::isRetryable(ErrorCode error) {
    return error == ErrorCode::NotConnected ||
           error == ErrorCode::WouldBlock ||
           error == ErrorCode::InternalError;
}

bool MqttFrameTransport::equal(
    StringView left,
    StringView right) {
    return left.size == right.size &&
           left.data != nullptr &&
           right.data != nullptr &&
           memcmp(left.data, right.data, left.size) == 0;
}

bool MqttFrameTransport::valid(StringView value) {
    return value.data != nullptr && value.size != 0;
}

} // namespace blinker
