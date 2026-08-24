#include "TransportHub.h"

namespace blinker {

TransportHub::TransportHub()
    : count_(0),
      nextPoll_(0),
      receiver_(nullptr),
      receiverContext_(nullptr),
      sessionConnected_(nullptr),
      sessionDisconnected_(nullptr),
      sessionContext_(nullptr) {
    for (size_t index = 0; index < BLINKER_MAX_TRANSPORTS; ++index) {
        transports_[index] = nullptr;
        lifecycles_[index] = TransportLifecyclePolicy::Managed;
    }
}

Result TransportHub::addTransport(
    IFrameTransport& transport,
    TransportLifecyclePolicy lifecycle) {
    for (size_t index = 0; index < count_; ++index) {
        if (transports_[index] == &transport) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
    }

    if (count_ >= BLINKER_MAX_TRANSPORTS) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    const size_t index = count_;
    transports_[index] = &transport;
    lifecycles_[index] = lifecycle;
    receiverContexts_[index].hub = this;
    receiverContexts_[index].transportId = static_cast<uint8_t>(index);
    transport.setReceiver(
        &TransportHub::receiveThunk,
        &receiverContexts_[index]);
    transport.setSessionHandlers(
        &TransportHub::connectedThunk,
        &TransportHub::disconnectedThunk,
        &receiverContexts_[index]);
    ++count_;
    return Result::success();
}

Result TransportHub::removeTransport(IFrameTransport& transport) {
    size_t found = count_;
    for (size_t index = 0; index < count_; ++index) {
        if (transports_[index] == &transport) {
            found = index;
            break;
        }
    }

    if (found == count_) {
        return Result::failure(ErrorCode::NotFound);
    }

    if (lifecycles_[found] == TransportLifecyclePolicy::Managed) {
        transport.stop();
    }
    transport.setReceiver(nullptr, nullptr);
    transport.setSessionHandlers(nullptr, nullptr, nullptr);

    for (size_t index = found; index + 1 < count_; ++index) {
        transports_[index] = transports_[index + 1];
        lifecycles_[index] = lifecycles_[index + 1];
        receiverContexts_[index].hub = this;
        receiverContexts_[index].transportId = static_cast<uint8_t>(index);
        transports_[index]->setReceiver(
            &TransportHub::receiveThunk,
            &receiverContexts_[index]);
        transports_[index]->setSessionHandlers(
            &TransportHub::connectedThunk,
            &TransportHub::disconnectedThunk,
            &receiverContexts_[index]);
    }

    --count_;
    transports_[count_] = nullptr;
    lifecycles_[count_] = TransportLifecyclePolicy::Managed;
    receiverContexts_[count_].hub = nullptr;
    receiverContexts_[count_].transportId = 0;
    if (count_ == 0) {
        nextPoll_ = 0;
    } else if (nextPoll_ >= count_) {
        nextPoll_ %= count_;
    }
    return Result::success();
}

Result TransportHub::startAll() {
    for (size_t index = 0; index < count_; ++index) {
        if (lifecycles_[index] == TransportLifecyclePolicy::External) {
            continue;
        }
        const Result result = transports_[index]->start();
        if (!result) {
            while (index > 0) {
                --index;
                if (lifecycles_[index] ==
                    TransportLifecyclePolicy::Managed) {
                    transports_[index]->stop();
                }
            }
            return result;
        }
    }
    return Result::success();
}

void TransportHub::stopAll() {
    for (size_t index = 0; index < count_; ++index) {
        if (lifecycles_[index] == TransportLifecyclePolicy::Managed) {
            transports_[index]->stop();
        }
    }
}

void TransportHub::poll(uint32_t totalBudgetMicros) {
    if (count_ == 0) {
        return;
    }

    size_t managedCount = 0U;
    for (size_t index = 0U; index < count_; ++index) {
        if (lifecycles_[index] == TransportLifecyclePolicy::Managed) {
            ++managedCount;
        }
    }
    if (managedCount == 0U) return;

    uint32_t perTransport =
        totalBudgetMicros / static_cast<uint32_t>(managedCount);
    if (totalBudgetMicros != 0 && perTransport == 0) {
        perTransport = 1;
    }

    const size_t start = nextPoll_;

    // Poll local/BLE-capable transports before cloud-only transports. Native
    // MQTT clients can still block inside DNS/TCP/TLS/CONNACK despite receiving
    // a cooperative budget, so local links must get service immediately before
    // that call. Rotation is retained within each priority class.
    for (uint8_t priority = 0; priority < 2U; ++priority) {
        for (size_t offset = 0; offset < count_; ++offset) {
            const size_t index = (start + offset) % count_;
            if (lifecycles_[index] ==
                TransportLifecyclePolicy::External) {
                continue;
            }
            const uint16_t features =
                transports_[index]->capabilities().features;
            const bool cloudOnly =
                (features & TransportFeatureCloud) != 0U &&
                (features & TransportFeatureLocal) == 0U;
            const uint8_t transportPriority = cloudOnly ? 1U : 0U;
            if (transportPriority == priority) {
                transports_[index]->poll(perTransport);
            }
        }
    }
    nextPoll_ = (start + 1) % count_;
}

Result TransportHub::send(ByteView frame, const SendTarget& target) {
    if (frame.data == nullptr || frame.size == 0) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    if (target.kind == SendTargetKind::Broadcast) {
        return broadcast(frame);
    }

    for (size_t index = 0; index < count_; ++index) {
        if (target.transportId == index) {
            return transports_[index]->send(frame, target);
        }
    }
    return Result::failure(ErrorCode::NotFound);
}

Result TransportHub::broadcast(ByteView frame) {
    if (frame.data == nullptr || frame.size == 0) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (count_ == 0) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    Result firstError = Result::success();
    SendTarget target;
    target.kind = SendTargetKind::Broadcast;

    for (size_t index = 0; index < count_; ++index) {
        target.transportId = static_cast<uint8_t>(index);
        const Result result = transports_[index]->send(frame, target);
        if (!result && firstError.ok()) {
            firstError = result;
        }
    }
    return firstError;
}

void TransportHub::setReceiver(FrameReceiver receiver, void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

void TransportHub::setSessionHandlers(
    FrameSessionHandler connected,
    FrameSessionHandler disconnected,
    void* context) {
    sessionConnected_ = connected;
    sessionDisconnected_ = disconnected;
    sessionContext_ = context;
}

IFrameTransport* TransportHub::at(size_t index) const {
    return index < count_ ? transports_[index] : nullptr;
}

void TransportHub::receiveThunk(
    void* context,
    ByteView frame,
    const RxContext& rx) {
    ReceiverContext* receiverContext =
        static_cast<ReceiverContext*>(context);
    if (receiverContext != nullptr && receiverContext->hub != nullptr) {
        receiverContext->hub->receive(
            receiverContext->transportId,
            frame,
            rx);
    }
}

void TransportHub::receive(
    uint8_t transportId,
    ByteView frame,
    const RxContext& rx) {
    if (receiver_ != nullptr) {
        RxContext normalized = rx;
        normalized.transportId = transportId;
        receiver_(receiverContext_, frame, normalized);
    }
}

void TransportHub::connectedThunk(
    void* context,
    const RxContext& rx) {
    ReceiverContext* receiverContext =
        static_cast<ReceiverContext*>(context);
    if (receiverContext != nullptr && receiverContext->hub != nullptr) {
        receiverContext->hub->sessionConnected(
            receiverContext->transportId,
            rx);
    }
}

void TransportHub::disconnectedThunk(
    void* context,
    const RxContext& rx) {
    ReceiverContext* receiverContext =
        static_cast<ReceiverContext*>(context);
    if (receiverContext != nullptr && receiverContext->hub != nullptr) {
        receiverContext->hub->sessionDisconnected(
            receiverContext->transportId,
            rx);
    }
}

void TransportHub::sessionConnected(
    uint8_t transportId,
    const RxContext& rx) {
    if (sessionConnected_ != nullptr) {
        RxContext normalized = rx;
        normalized.transportId = transportId;
        sessionConnected_(sessionContext_, normalized);
    }
}

void TransportHub::sessionDisconnected(
    uint8_t transportId,
    const RxContext& rx) {
    if (sessionDisconnected_ != nullptr) {
        RxContext normalized = rx;
        normalized.transportId = transportId;
        sessionDisconnected_(sessionContext_, normalized);
    }
}

} // namespace blinker
