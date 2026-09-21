#ifndef BLINKER_RUNTIME_TIMESYNCRESPONDER_H
#define BLINKER_RUNTIME_TIMESYNCRESPONDER_H

#include "../interface/ITimeSync.h"

namespace blinker {
// Optional response policy for one existing authenticated session. No clock
// copy, security IClock, connection, pending request, timer or TX buffer. The
// owner supplies current trusted peer metadata and a negotiated-size output,
// then sends synchronously on that SAME live session. A failed send is not
// retried here; existing carrier lifecycle/queue rules remain authoritative.
class TimeSyncResponder {
public:
    explicit TimeSyncResponder(IWallTimeSource& source);
    void reset(); // Owner retires the old session before reusing this policy.
    Result reply(const TimeSyncPeer& peer, const bbp2::FrameView& request,
                 uint32_t nowMillis, MutableByteSpan output, ByteView& response);
private:
    IWallTimeSource& source_;
    uint32_t lastReplyMillis_;
    bool replied_;
    TimeSyncResponder(const TimeSyncResponder&) = delete;
    TimeSyncResponder& operator=(const TimeSyncResponder&) = delete;
};
}
#endif
