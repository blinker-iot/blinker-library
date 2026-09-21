#ifndef BLINKER_INTERFACE_ITIMESYNC_H
#define BLINKER_INTERFACE_ITIMESYNC_H

#include "IWallTimeSource.h"
#include "../protocol/bbp2/Frame.h"

namespace blinker {
class IClock;
class IRandom;

// Trusted runtime metadata, never a claimed role decoded from the wire.
struct TimeSyncPeer {
    uint32_t sessionId;
    uint32_t generation;
    uint32_t features;
    uint32_t permissions;
    uint8_t transportId;
    WallTimeSource source;
    bool authenticated;
    TimeSyncPeer() : sessionId(0U), generation(0U), features(0U), permissions(0U),
        transportId(0U), source(WallTimeSource::None), authenticated(false) {}
};

// Optional protocol capability. Runtime borrows it, shares its existing TX
// buffer, and never constructs a clock or owns platform/RNG objects.
class ITimeSync : public IWallTimeSource {
public:
    virtual ~ITimeSync() {}
    virtual Result configure(IClock& tick, IRandom& random) = 0;
    virtual void reset(bool clearTime) = 0;
    // Recheck the runtime-selected peer, cancel an obsolete exchange, poll
    // expiry, then report whether a request is due. An empty peer selects none.
    virtual bool poll(const TimeSyncPeer& selected) = 0;
    virtual void sessionClosed(uint8_t transportId, uint32_t sessionId) = 0;
    virtual Result request(const TimeSyncPeer& peer, uint16_t sequence,
                           MutableByteSpan body, size_t& written) = 0;
    // Only after an encoded request is rejected by the local transport.
    // This is not a remote Error, timeout, RNG failure or session close.
    virtual void sendRejected(ErrorCode error) = 0;
    virtual Result receive(const TimeSyncPeer& peer, const bbp2::FrameView& frame) = 0;
    bool time(uint64_t& utcSeconds) {
        utcSeconds = 0U;
        WallClockSnapshot snapshot;
        if (!readTime(snapshot)) return false;
        utcSeconds = snapshot.utcMillis / 1000U;
        return true;
    }
};
}
#endif
