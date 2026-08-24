#include "ReliableOutbox.h"

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

ReliableOutbox::ReliableOutbox(
    TransportHub& transports,
    IClock& clock,
    ReliableFrameSlot* slots,
    size_t slotCount,
    const ReliableRetryPolicy& policy)
    : transports_(transports),
      clock_(clock),
      slots_(slots),
      slotCount_(slotCount),
      policy_(policy),
      handler_(nullptr),
      handlerContext_(nullptr),
      acknowledgedCount_(0),
      rejectedCount_(0),
      retryCount_(0),
      failedCount_(0) {}

Result ReliableOutbox::validate(size_t requiredFrameSize) const {
    if (slots_ == nullptr || slotCount_ == 0U ||
        requiredFrameSize < bbp2::kBaseHeaderSize ||
        policy_.initialDelayMillis == 0U ||
        policy_.maximumDelayMillis < policy_.initialDelayMillis ||
        policy_.maximumDelayMillis > 0x7FFFFFFFUL ||
        policy_.overallTimeoutMillis < policy_.initialDelayMillis ||
        policy_.overallTimeoutMillis > 0x7FFFFFFFUL ||
        policy_.maxAttempts == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    for (size_t index = 0; index < slotCount_; ++index) {
        const MutableByteSpan storage = slots_[index].storage;
        if (storage.data == nullptr || storage.size < requiredFrameSize) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }
        const uintptr_t begin = reinterpret_cast<uintptr_t>(storage.data);
        const uintptr_t end = begin + storage.size;
        if (end < begin) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        for (size_t prior = 0; prior < index; ++prior) {
            const uintptr_t priorBegin =
                reinterpret_cast<uintptr_t>(slots_[prior].storage.data);
            const uintptr_t priorEnd =
                priorBegin + slots_[prior].storage.size;
            if (begin < priorEnd && priorBegin < end) {
                return Result::failure(ErrorCode::InvalidArgument);
            }
        }
    }
    return Result::success();
}

Result ReliableOutbox::enqueue(ByteView frame, const SendTarget& target) {
    if (target.kind == SendTargetKind::Broadcast || frame.data == nullptr ||
        frame.size < bbp2::kBaseHeaderSize || frame.size > UINT16_MAX) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    bbp2::FrameView parsed;
    Result result = bbp2::parseFrame(frame, parsed);
    if (!result) return result;
    if (parsed.header.sequence == 0U ||
        (parsed.header.flags & bbp2::FlagAckRequired) == 0U ||
        (parsed.header.flags & bbp2::FlagIsResponse) != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (sequencePending(parsed.header.sequence)) {
        return Result::failure(ErrorCode::AlreadyExists);
    }

    ReliableFrameSlot* empty = nullptr;
    for (size_t index = 0; index < slotCount_; ++index) {
        if (!slots_[index].occupied) {
            empty = &slots_[index];
            break;
        }
    }
    if (empty == nullptr) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    if (empty->storage.data == nullptr || empty->storage.size < frame.size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    memcpy(empty->storage.data, frame.data, frame.size);
    empty->target = target;
    empty->frameSize = static_cast<uint16_t>(frame.size);
    empty->sequence = parsed.header.sequence;
    empty->messageKind = parsed.header.kind;
    empty->attempts = 0;
    empty->occupied = true;
    const uint32_t now = clock_.monotonicMillis();
    empty->deadlineMillis = now + policy_.overallTimeoutMillis;

    result = sendSlot(*empty, now);
    if (result || result.code() == ErrorCode::WouldBlock) {
        return Result::success();
    }
    clear(*empty);
    return result;
}

void ReliableOutbox::poll() {
    const uint32_t now = clock_.monotonicMillis();
    for (size_t index = 0; index < slotCount_; ++index) {
        ReliableFrameSlot& slot = slots_[index];
        if (!slot.occupied) continue;
        if (timeReached(now, slot.deadlineMillis)) {
            finish(
                slot,
                ReliableDeliveryOutcome::RetryExhausted,
                0,
                ErrorCode::WouldBlock);
            continue;
        }
        if (!timeReached(now, slot.retryAtMillis)) continue;
        if (slot.attempts >= policy_.maxAttempts) {
            finish(
                slot,
                ReliableDeliveryOutcome::RetryExhausted,
                0,
                ErrorCode::WouldBlock);
            continue;
        }
        const Result result = sendSlot(slot, now);
        if (!result && result.code() != ErrorCode::WouldBlock) {
            finish(
                slot,
                ReliableDeliveryOutcome::TransportError,
                0,
                result.code());
        }
    }
}

void ReliableOutbox::reset() {
    for (size_t index = 0; index < slotCount_; ++index) {
        clear(slots_[index]);
    }
}

bool ReliableOutbox::acknowledge(uint16_t sequence, const RxContext& rx) {
    for (size_t index = 0; index < slotCount_; ++index) {
        ReliableFrameSlot& slot = slots_[index];
        if (slot.occupied && slot.sequence == sequence &&
            matchesReceiver(slot.target, rx)) {
            ++acknowledgedCount_;
            finish(
                slot,
                ReliableDeliveryOutcome::Acknowledged,
                0,
                ErrorCode::Ok);
            return true;
        }
    }
    return false;
}

bool ReliableOutbox::reject(
    uint16_t sequence,
    uint16_t peerError,
    const RxContext& rx) {
    for (size_t index = 0; index < slotCount_; ++index) {
        ReliableFrameSlot& slot = slots_[index];
        if (slot.occupied && slot.sequence == sequence &&
            matchesReceiver(slot.target, rx)) {
            ++rejectedCount_;
            finish(
                slot,
                ReliableDeliveryOutcome::PeerRejected,
                peerError,
                ErrorCode::Ok);
            return true;
        }
    }
    return false;
}

void ReliableOutbox::sessionClosed(const RxContext& rx) {
    for (size_t index = 0; index < slotCount_; ++index) {
        ReliableFrameSlot& slot = slots_[index];
        if (slot.occupied && matchesReceiver(slot.target, rx)) {
            finish(
                slot,
                ReliableDeliveryOutcome::SessionClosed,
                0,
                ErrorCode::NotConnected);
        }
    }
}

void ReliableOutbox::transportClosed(uint8_t transportId) {
    for (size_t index = 0; index < slotCount_; ++index) {
        ReliableFrameSlot& slot = slots_[index];
        if (slot.occupied && slot.target.transportId == transportId) {
            finish(
                slot,
                ReliableDeliveryOutcome::SessionClosed,
                0,
                ErrorCode::NotConnected);
        }
    }
}

bool ReliableOutbox::sequencePending(uint16_t sequence) const {
    if (sequence == 0U) return false;
    for (size_t index = 0; index < slotCount_; ++index) {
        if (slots_[index].occupied && slots_[index].sequence == sequence) {
            return true;
        }
    }
    return false;
}

size_t ReliableOutbox::pendingCount() const {
    size_t count = 0;
    for (size_t index = 0; index < slotCount_; ++index) {
        count += slots_[index].occupied ? 1U : 0U;
    }
    return count;
}

size_t ReliableOutbox::pendingCount(const SendTarget& target) const {
    size_t count = 0;
    for (size_t index = 0; index < slotCount_; ++index) {
        if (slots_[index].occupied && sameTarget(slots_[index].target, target)) {
            ++count;
        }
    }
    return count;
}

void ReliableOutbox::setHandler(
    ReliableDeliveryHandler handler,
    void* context) {
    handler_ = handler;
    handlerContext_ = context;
}

bool ReliableOutbox::timeReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}

bool ReliableOutbox::sameTarget(
    const SendTarget& left,
    const SendTarget& right) {
    if (left.kind != right.kind || left.transportId != right.transportId) {
        return false;
    }
    return left.kind != SendTargetKind::Session ||
           left.sessionId == right.sessionId;
}

bool ReliableOutbox::matchesReceiver(
    const SendTarget& target,
    const RxContext& rx) {
    if (target.transportId != rx.transportId) return false;
    if (target.kind == SendTargetKind::Session) {
        return target.sessionId == rx.sessionId;
    }
    return target.kind == SendTargetKind::Transport && rx.sessionId == 0U;
}

uint32_t ReliableOutbox::retryDelay(uint8_t attempts) const {
    uint32_t delay = policy_.initialDelayMillis;
    for (uint8_t index = 1U; index < attempts; ++index) {
        if (delay >= policy_.maximumDelayMillis / 2U) {
            return policy_.maximumDelayMillis;
        }
        delay *= 2U;
    }
    return delay > policy_.maximumDelayMillis
               ? policy_.maximumDelayMillis
               : delay;
}

Result ReliableOutbox::sendSlot(ReliableFrameSlot& slot, uint32_t now) {
    const Result result = transports_.send(
        ByteView(slot.storage.data, slot.frameSize),
        slot.target);
    if (result) {
        if (slot.attempts != 0U) ++retryCount_;
        ++slot.attempts;
        slot.retryAtMillis = now + retryDelay(slot.attempts);
    } else if (result.code() == ErrorCode::WouldBlock) {
        slot.retryAtMillis = now + policy_.initialDelayMillis;
    }
    return result;
}

void ReliableOutbox::finish(
    ReliableFrameSlot& slot,
    ReliableDeliveryOutcome outcome,
    uint16_t peerError,
    ErrorCode transportError) {
    ReliableDeliveryEvent event;
    event.sequence = slot.sequence;
    event.messageKind = slot.messageKind;
    event.target = slot.target;
    event.outcome = outcome;
    event.peerError = peerError;
    event.transportError = transportError;
    if (outcome == ReliableDeliveryOutcome::RetryExhausted ||
        outcome == ReliableDeliveryOutcome::TransportError ||
        outcome == ReliableDeliveryOutcome::SessionClosed) {
        ++failedCount_;
    }
    clear(slot);
    if (handler_ != nullptr) handler_(handlerContext_, event);
}

void ReliableOutbox::clear(ReliableFrameSlot& slot) {
    if (slot.storage.data != nullptr && slot.frameSize != 0U) {
        secureZero(MutableByteSpan(slot.storage.data, slot.frameSize));
    }
    const MutableByteSpan storage = slot.storage;
    slot = ReliableFrameSlot();
    slot.storage = storage;
}

} // namespace blinker
