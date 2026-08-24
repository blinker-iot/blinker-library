#ifndef BLINKER_RUNTIME_OUTBOUND_SCHEDULER_H
#define BLINKER_RUNTIME_OUTBOUND_SCHEDULER_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

struct OutboundSchedulerConfig {
    uint32_t stateCoalesceWindowMs;
    uint32_t stateMinimumIntervalMs;
    uint16_t eventTokensPerSecond;
    uint16_t eventBurst;

    OutboundSchedulerConfig(
        uint32_t coalesceWindowMs = 250U,
        uint32_t minimumIntervalMs = 250U,
        uint16_t eventRate = 5U,
        uint16_t eventBurstCapacity = 8U)
        : stateCoalesceWindowMs(coalesceWindowMs),
          stateMinimumIntervalMs(minimumIntervalMs),
          eventTokensPerSecond(eventRate),
          eventBurst(eventBurstCapacity) {}
};

struct OutboundSchedulerCounters {
    uint32_t stateAccepted;
    uint32_t stateCoalesced;
    uint32_t stateFlushes;
    uint32_t stateDeferred;
    uint32_t stateTransportErrors;
    uint32_t eventAccepted;
    uint32_t eventDispatched;
    uint32_t eventThrottled;
    uint32_t eventTransportErrors;

    OutboundSchedulerCounters()
        : stateAccepted(0U),
          stateCoalesced(0U),
          stateFlushes(0U),
          stateDeferred(0U),
          stateTransportErrors(0U),
          eventAccepted(0U),
          eventDispatched(0U),
          eventThrottled(0U),
          eventTransportErrors(0U) {}
};

// Fixed-resource scheduler state shared by every transport. Endpoint values
// remain in EndpointStateStore; this class retains only dirty/selection bits
// and timing metadata. The caller owns both bitmaps.
class OutboundScheduler {
public:
    OutboundScheduler(
        MutableByteSpan dirtyFields,
        MutableByteSpan selectedFields,
        const OutboundSchedulerConfig& config = OutboundSchedulerConfig());

    static size_t bitmapBytes(size_t fieldCount) {
        return (fieldCount + 7U) / 8U;
    }

    Result prepare(size_t fieldCount);
    void reset();

    Result markStateDirty(size_t fieldIndex, uint32_t now);
    bool stateFlushDue(uint32_t now, bool force = false);

    // Selects the first maxFields dirty fields in manifest order. The
    // returned count and selection() stay valid until the next selection,
    // commit or reset.
    size_t selectStateFields(size_t maxFields);
    ByteView selection() const;
    Result commitStateSelection(uint32_t now, Result transportResult);
    void deferStateFlush();

    Result admitEvent(uint32_t now);
    void recordEventDispatch(Result transportResult);
    void applyRemoteBackoff(uint32_t now, uint32_t durationMs = 1000U);

    bool hasPendingState() const { return dirtyCount_ != 0U; }
    bool stateDirty(size_t fieldIndex) const;
    size_t pendingStateCount() const { return dirtyCount_; }
    size_t selectedStateCount() const { return selectedCount_; }
    const OutboundSchedulerCounters& counters() const { return counters_; }

private:
    static bool isBitSet(ByteView bitmap, size_t index);
    static void setBit(MutableByteSpan bitmap, size_t index);
    static void clearBit(MutableByteSpan bitmap, size_t index);
    void clearBitmap(MutableByteSpan bitmap);
    bool remoteBackoffPending(uint32_t now);

    MutableByteSpan dirtyFields_;
    MutableByteSpan selectedFields_;
    OutboundSchedulerConfig config_;
    OutboundSchedulerCounters counters_;
    size_t fieldCount_;
    size_t dirtyCount_;
    size_t selectedCount_;
    uint32_t firstDirtyAt_;
    uint32_t lastFlushAt_;
    uint32_t eventCredits_;
    uint32_t eventRefillAt_;
    bool prepared_;
    bool hasFlushed_;
    bool eventBucketStarted_;
    uint32_t remoteBackoffUntil_;
    bool remoteBackoffActive_;
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(OutboundScheduler) <= 104U,
    "OutboundScheduler exceeds its 32-bit RAM gate");
#endif

} // namespace blinker

#endif
