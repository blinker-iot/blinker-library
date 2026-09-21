#ifndef BLINKER_RUNTIME_TIMESYNCCOORDINATOR_H
#define BLINKER_RUNTIME_TIMESYNCCOORDINATOR_H

#include "../core/Authorization.h"
#include "../interface/IRandom.h"
#include "../interface/ITimeSync.h"
#include "../protocol/bbp2/Messages.h"
#include "../protocol/bbp2/TimeSync.h"

namespace blinker {

// One optional exchange for one business clock, no transport/queue or peer slots.
class TimeSyncCoordinator {
public:
    explicit TimeSyncCoordinator(WallClock& clock);
    // Call immediately before the one send; cancel on send failure. Queue time is
    // included in the conservative RTT budget. Never register reliable replay.
    Result begin(const TimeSyncPeer& peer, uint16_t sequence, uint32_t nowMillis,
                 IRandom& random, MutableByteSpan body, size_t& written);
    Result receive(const TimeSyncPeer& peer, const bbp2::FrameView& frame, uint32_t nowMillis);
    void poll(uint32_t nowMillis);
    // Disconnect, generation/source change and revocation cancel the exchange,
    // not the existing UTC holdover. Boot/unmeasured sleep also clear the clock.
    void cancel();
    bool pending() const { return pending_; }
    bool pendingFrom(uint8_t transportId, uint32_t sessionId) const {
        return pending_ && transportId == transportId_ && sessionId == sessionId_;
    }
    bool pendingFrom(const TimeSyncPeer& peer) const {
        return pendingFrom(peer.transportId, peer.sessionId) &&
            peer.generation == generation_ && peer.source == source_;
    }
    bool pendingFor(const TimeSyncPeer& peer, uint16_t sequence) const {
        return pendingFrom(peer) && sequence == sequence_;
    }

private:
    WallClock& clock_;
    uint32_t sessionId_;
    uint32_t generation_;
    uint32_t sentAtMillis_;
    uint8_t nonce_[bbp2::kTimeSyncNonceSize];
    uint16_t sequence_;
    uint8_t transportId_;
    WallTimeSource source_;
    bool pending_;
};
} // namespace blinker
#endif
