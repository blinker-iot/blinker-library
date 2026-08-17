#include "ManagedMqttTransport.h"

namespace blinker {

ManagedMqttTransport::ManagedMqttTransport(
    ISessionProvider& sessionProvider,
    MqttFrameTransport& mqttTransport)
    : sessionProvider_(sessionProvider),
      mqttTransport_(mqttTransport),
      appliedGeneration_(0),
      started_(false),
      networkAvailable_(true),
      refreshRequested_(false),
      bindingError_(ErrorCode::Ok) {}

Result ManagedMqttTransport::start() {
    if (started_) return Result::success();
    started_ = true;
    appliedGeneration_ = 0;
    refreshRequested_ = false;
    bindingError_ = ErrorCode::Ok;
    sessionProvider_.setNetworkAvailable(networkAvailable_);
    mqttTransport_.setNetworkAvailable(networkAvailable_);
    const Result result = sessionProvider_.start();
    if (!result) {
        started_ = false;
        return result;
    }
    return Result::success();
}

void ManagedMqttTransport::setNetworkAvailable(bool available) {
    if (networkAvailable_ == available) return;
    networkAvailable_ = available;
    sessionProvider_.setNetworkAvailable(available);
    mqttTransport_.setNetworkAvailable(available);
}

void ManagedMqttTransport::stop() {
    mqttTransport_.stop();
    sessionProvider_.stop();
    appliedGeneration_ = 0;
    refreshRequested_ = false;
    bindingError_ = ErrorCode::Ok;
    started_ = false;
}

void ManagedMqttTransport::poll(uint32_t budgetMicros) {
    if (!started_ || !networkAvailable_) return;
    const uint32_t controlBudget = budgetMicros / 2U;
    sessionProvider_.poll(controlBudget);

    if (sessionProvider_.state() == SessionProviderState::Failed) {
        mqttTransport_.stop();
        bindingError_ = sessionProvider_.lastError();
        return;
    }

    if (sessionProvider_.state() == SessionProviderState::Ready &&
        sessionProvider_.credentialGeneration() != appliedGeneration_) {
        const Result result = applyCredentials();
        if (!result) {
            if (result.code() == ErrorCode::AuthenticationRequired &&
                !refreshRequested_ && sessionProvider_.requestRefresh()) {
                refreshRequested_ = true;
            } else if (result.code() != ErrorCode::AuthenticationRequired) {
                bindingError_ = result.code();
            }
            return;
        }
    }

    mqttTransport_.poll(budgetMicros - controlBudget);
    if (mqttTransport_.state() == TransportState::Error &&
        mqttTransport_.lastError() == ErrorCode::AuthenticationRequired &&
        !refreshRequested_) {
        if (sessionProvider_.requestRefresh()) {
            refreshRequested_ = true;
        }
    }
}

Result ManagedMqttTransport::applyCredentials() {
    SessionCredentials credentials;
    Result result = sessionProvider_.credentials(credentials);
    if (!result) return result;
    mqttTransport_.stop();
    result = mqttTransport_.configure(credentials);
    if (!result) return result;
    result = mqttTransport_.start();
    if (!result) return result;
    appliedGeneration_ = sessionProvider_.credentialGeneration();
    refreshRequested_ = false;
    return Result::success();
}

TransportState ManagedMqttTransport::state() const {
    if (!started_) return TransportState::Stopped;
    if (bindingError_ != ErrorCode::Ok) return TransportState::Error;
    if (!networkAvailable_) return TransportState::Starting;
    if (appliedGeneration_ != 0U &&
        mqttTransport_.state() == TransportState::Online) {
        return TransportState::Online;
    }
    if (sessionProvider_.state() == SessionProviderState::Failed) {
        return TransportState::Error;
    }
    if (sessionProvider_.state() == SessionProviderState::Backoff) {
        return TransportState::Backoff;
    }
    if (appliedGeneration_ == 0U ||
        sessionProvider_.state() == SessionProviderState::Requesting ||
        sessionProvider_.state() == SessionProviderState::Idle) {
        return TransportState::Starting;
    }
    return mqttTransport_.state();
}

ErrorCode ManagedMqttTransport::lastError() const {
    if (bindingError_ != ErrorCode::Ok) return bindingError_;
    if (sessionProvider_.state() == SessionProviderState::Failed) {
        return sessionProvider_.lastError();
    }
    if (mqttTransport_.state() == TransportState::Error) {
        return mqttTransport_.lastError();
    }
    return ErrorCode::Ok;
}

TransportCapabilities ManagedMqttTransport::capabilities() const {
    return mqttTransport_.capabilities();
}

Result ManagedMqttTransport::send(
    ByteView frame,
    const SendTarget& target) {
    if (state() != TransportState::Online) {
        return Result::failure(ErrorCode::NotConnected);
    }
    return mqttTransport_.send(frame, target);
}

void ManagedMqttTransport::setReceiver(
    FrameReceiver receiver,
    void* context) {
    mqttTransport_.setReceiver(receiver, context);
}

} // namespace blinker
