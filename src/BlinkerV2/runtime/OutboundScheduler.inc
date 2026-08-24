#include "OutboundScheduler.h"

#include <string.h>

namespace blinker {

OutboundScheduler::OutboundScheduler(
    MutableByteSpan dirtyFields,
    MutableByteSpan selectedFields,
    const OutboundSchedulerConfig& config)
    : dirtyFields_(dirtyFields),
      selectedFields_(selectedFields),
      config_(config),
      counters_(),
      fieldCount_(0U),
      dirtyCount_(0U),
      selectedCount_(0U),
      firstDirtyAt_(0U),
      lastFlushAt_(0U),
      eventCredits_(0U),
      eventRefillAt_(0U),
      prepared_(false),
      hasFlushed_(false),
      eventBucketStarted_(false),
      remoteBackoffUntil_(0U),
      remoteBackoffActive_(false) {}

Result OutboundScheduler::prepare(size_t fieldCount) {
    const size_t required = bitmapBytes(fieldCount);
    if (fieldCount == 0U || dirtyFields_.data == nullptr ||
        selectedFields_.data == nullptr || dirtyFields_.size < required ||
        selectedFields_.size < required) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    fieldCount_ = fieldCount;
    prepared_ = true;
    reset();
    return Result::success();
}

void OutboundScheduler::reset() {
    clearBitmap(dirtyFields_);
    clearBitmap(selectedFields_);
    counters_ = OutboundSchedulerCounters();
    dirtyCount_ = 0U;
    selectedCount_ = 0U;
    firstDirtyAt_ = 0U;
    lastFlushAt_ = 0U;
    eventCredits_ = 0U;
    eventRefillAt_ = 0U;
    hasFlushed_ = false;
    eventBucketStarted_ = false;
    remoteBackoffUntil_ = 0U;
    remoteBackoffActive_ = false;
}

Result OutboundScheduler::markStateDirty(size_t fieldIndex, uint32_t now) {
    if (!prepared_ || fieldIndex >= fieldCount_) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (stateDirty(fieldIndex)) {
        ++counters_.stateCoalesced;
        return Result::success();
    }
    if (dirtyCount_ == 0U) firstDirtyAt_ = now;
    setBit(dirtyFields_, fieldIndex);
    ++dirtyCount_;
    ++counters_.stateAccepted;
    return Result::success();
}

bool OutboundScheduler::stateFlushDue(uint32_t now, bool force) {
    if (!prepared_ || dirtyCount_ == 0U) return false;
    if (remoteBackoffPending(now)) return false;
    if (force) return true;
    const bool windowElapsed =
        static_cast<uint32_t>(now - firstDirtyAt_) >=
        config_.stateCoalesceWindowMs;
    const bool intervalElapsed =
        !hasFlushed_ ||
        static_cast<uint32_t>(now - lastFlushAt_) >=
            config_.stateMinimumIntervalMs;
    return windowElapsed && intervalElapsed;
}

size_t OutboundScheduler::selectStateFields(size_t maxFields) {
    clearBitmap(selectedFields_);
    selectedCount_ = 0U;
    if (!prepared_ || maxFields == 0U) return 0U;
    for (size_t index = 0U;
         index < fieldCount_ && selectedCount_ < maxFields;
         ++index) {
        if (!stateDirty(index)) continue;
        setBit(selectedFields_, index);
        ++selectedCount_;
    }
    return selectedCount_;
}

ByteView OutboundScheduler::selection() const {
    return ByteView(selectedFields_.data, bitmapBytes(fieldCount_));
}

Result OutboundScheduler::commitStateSelection(
    uint32_t now,
    Result transportResult) {
    if (!prepared_ || selectedCount_ == 0U ||
        selectedCount_ > dirtyCount_) {
        return Result::failure(ErrorCode::StateConflict);
    }
    size_t cleared = 0U;
    for (size_t index = 0U; index < fieldCount_; ++index) {
        if (!isBitSet(selection(), index)) continue;
        if (!stateDirty(index)) {
            return Result::failure(ErrorCode::StateConflict);
        }
        clearBit(dirtyFields_, index);
        ++cleared;
    }
    if (cleared != selectedCount_) {
        return Result::failure(ErrorCode::InternalError);
    }
    dirtyCount_ -= cleared;
    clearBitmap(selectedFields_);
    selectedCount_ = 0U;
    lastFlushAt_ = now;
    hasFlushed_ = true;
    ++counters_.stateFlushes;
    if (!transportResult) ++counters_.stateTransportErrors;
    return Result::success();
}

void OutboundScheduler::deferStateFlush() {
    if (dirtyCount_ != 0U) ++counters_.stateDeferred;
    clearBitmap(selectedFields_);
    selectedCount_ = 0U;
}

Result OutboundScheduler::admitEvent(uint32_t now) {
    if (!prepared_ || config_.eventTokensPerSecond == 0U ||
        config_.eventBurst == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (remoteBackoffPending(now)) {
        ++counters_.eventThrottled;
        return Result::failure(ErrorCode::WouldBlock);
    }

    const uint32_t capacity =
        static_cast<uint32_t>(config_.eventBurst) * 1000U;
    if (!eventBucketStarted_) {
        eventCredits_ = capacity;
        eventRefillAt_ = now;
        eventBucketStarted_ = true;
    } else {
        const uint32_t elapsed = now - eventRefillAt_;
        const uint64_t replenished =
            static_cast<uint64_t>(elapsed) *
            config_.eventTokensPerSecond;
        const uint64_t available =
            static_cast<uint64_t>(eventCredits_) + replenished;
        eventCredits_ = available < capacity
                            ? static_cast<uint32_t>(available)
                            : capacity;
        eventRefillAt_ = now;
    }

    if (eventCredits_ < 1000U) {
        ++counters_.eventThrottled;
        return Result::failure(ErrorCode::WouldBlock);
    }
    eventCredits_ -= 1000U;
    ++counters_.eventAccepted;
    return Result::success();
}

void OutboundScheduler::applyRemoteBackoff(
    uint32_t now,
    uint32_t durationMs) {
    if (!prepared_ || durationMs == 0U) return;
    remoteBackoffUntil_ = now + durationMs;
    remoteBackoffActive_ = true;
}

bool OutboundScheduler::remoteBackoffPending(uint32_t now) {
    if (!remoteBackoffActive_) return false;
    if (static_cast<int32_t>(now - remoteBackoffUntil_) < 0) return true;
    remoteBackoffActive_ = false;
    return false;
}

void OutboundScheduler::recordEventDispatch(Result transportResult) {
    if (transportResult) {
        ++counters_.eventDispatched;
    } else {
        ++counters_.eventTransportErrors;
    }
}

bool OutboundScheduler::stateDirty(size_t fieldIndex) const {
    return prepared_ && fieldIndex < fieldCount_ &&
           isBitSet(
               ByteView(dirtyFields_.data, dirtyFields_.size), fieldIndex);
}

bool OutboundScheduler::isBitSet(ByteView bitmap, size_t index) {
    const size_t byteIndex = index / 8U;
    return bitmap.data != nullptr && byteIndex < bitmap.size &&
           (bitmap.data[byteIndex] &
            static_cast<uint8_t>(1U << (index % 8U))) != 0U;
}

void OutboundScheduler::setBit(MutableByteSpan bitmap, size_t index) {
    bitmap.data[index / 8U] |=
        static_cast<uint8_t>(1U << (index % 8U));
}

void OutboundScheduler::clearBit(MutableByteSpan bitmap, size_t index) {
    bitmap.data[index / 8U] &=
        static_cast<uint8_t>(~static_cast<uint8_t>(1U << (index % 8U)));
}

void OutboundScheduler::clearBitmap(MutableByteSpan bitmap) {
    if (bitmap.data != nullptr && bitmap.size != 0U) {
        memset(bitmap.data, 0, bitmap.size);
    }
}

} // namespace blinker
