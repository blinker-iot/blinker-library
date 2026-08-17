#ifndef BLINKER_RUNTIME_RELIABLEOUTBOX_H
#define BLINKER_RUNTIME_RELIABLEOUTBOX_H

#include "../interface/IClock.h"
#include "../protocol/bbp2/Frame.h"
#include "../transport/TransportHub.h"

namespace blinker {

enum class ReliableDeliveryOutcome : uint8_t {
    Acknowledged = 0,
    PeerRejected,
    RetryExhausted,
    TransportError,
    SessionClosed
};

struct ReliableDeliveryEvent {
    uint16_t sequence;
    uint8_t messageKind;
    SendTarget target;
    ReliableDeliveryOutcome outcome;
    uint16_t peerError;
    ErrorCode transportError;

    ReliableDeliveryEvent()
        : sequence(0),
          messageKind(0),
          target(),
          outcome(ReliableDeliveryOutcome::Acknowledged),
          peerError(0),
          transportError(ErrorCode::Ok) {}
};

typedef void (*ReliableDeliveryHandler)(
    void* context,
    const ReliableDeliveryEvent& event);

struct ReliableRetryPolicy {
    uint32_t initialDelayMillis;
    uint32_t maximumDelayMillis;
    uint32_t overallTimeoutMillis;
    uint8_t maxAttempts;

    ReliableRetryPolicy()
        : initialDelayMillis(250),
          maximumDelayMillis(4000),
          overallTimeoutMillis(15000),
          maxAttempts(4) {}
};

// Frame storage is caller-owned. Give every slot a distinct buffer at least as
// large as DeviceRuntimeConfig::maxFrameSize before Runtime::start().
struct ReliableFrameSlot {
    MutableByteSpan storage;
    SendTarget target;
    uint16_t frameSize;
    uint16_t sequence;
    uint32_t retryAtMillis;
    uint32_t deadlineMillis;
    uint8_t messageKind;
    uint8_t attempts;
    bool occupied;

    ReliableFrameSlot()
        : storage(),
          target(),
          frameSize(0),
          sequence(0),
          retryAtMillis(0),
          deadlineMillis(0),
          messageKind(0),
          attempts(0),
          occupied(false) {}
};

class ReliableOutbox {
public:
    ReliableOutbox(
        TransportHub& transports,
        IClock& clock,
        ReliableFrameSlot* slots,
        size_t slotCount,
        const ReliableRetryPolicy& policy = ReliableRetryPolicy());

    Result validate(size_t requiredFrameSize) const;
    Result enqueue(ByteView frame, const SendTarget& target);
    void poll();
    void reset();

    bool acknowledge(uint16_t sequence, const RxContext& rx);
    bool reject(uint16_t sequence, uint16_t peerError, const RxContext& rx);
    void sessionClosed(const RxContext& rx);
    void transportClosed(uint8_t transportId);

    bool sequencePending(uint16_t sequence) const;
    size_t pendingCount() const;
    size_t pendingCount(const SendTarget& target) const;

    void setHandler(ReliableDeliveryHandler handler, void* context);

    uint32_t acknowledgedCount() const { return acknowledgedCount_; }
    uint32_t rejectedCount() const { return rejectedCount_; }
    uint32_t retryCount() const { return retryCount_; }
    uint32_t failedCount() const { return failedCount_; }

private:
    static bool timeReached(uint32_t now, uint32_t deadline);
    static bool sameTarget(
        const SendTarget& left,
        const SendTarget& right);
    static bool matchesReceiver(
        const SendTarget& target,
        const RxContext& rx);

    uint32_t retryDelay(uint8_t attempts) const;
    Result sendSlot(ReliableFrameSlot& slot, uint32_t now);
    void finish(
        ReliableFrameSlot& slot,
        ReliableDeliveryOutcome outcome,
        uint16_t peerError,
        ErrorCode transportError);
    void clear(ReliableFrameSlot& slot);

    TransportHub& transports_;
    IClock& clock_;
    ReliableFrameSlot* slots_;
    size_t slotCount_;
    ReliableRetryPolicy policy_;
    ReliableDeliveryHandler handler_;
    void* handlerContext_;
    uint32_t acknowledgedCount_;
    uint32_t rejectedCount_;
    uint32_t retryCount_;
    uint32_t failedCount_;
};

} // namespace blinker

#endif
