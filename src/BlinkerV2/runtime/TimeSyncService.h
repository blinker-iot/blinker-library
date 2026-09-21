#ifndef BLINKER_RUNTIME_TIMESYNCSERVICE_H
#define BLINKER_RUNTIME_TIMESYNCSERVICE_H

#include "TimeSyncCoordinator.h"
#include "../interface/IClock.h"

namespace blinker {
// Explicitly owned only by an enabled product. No second TX buffer, timer,
// transport, system RTC or per-peer state. Scheduling survives reconnect churn.
class TimeSyncService final : public ITimeSync {
public:
    explicit TimeSyncService(const WallClockConfig& config = WallClockConfig());
    Result configure(IClock& tick, IRandom& random) override;
    void reset(bool clearTime) override;
    bool poll(const TimeSyncPeer& selected) override;
    void sessionClosed(uint8_t transportId, uint32_t sessionId) override;
    Result request(const TimeSyncPeer& peer, uint16_t sequence,
                   MutableByteSpan body, size_t& written) override;
    void sendRejected(ErrorCode error) override;
    Result receive(const TimeSyncPeer& peer, const bbp2::FrameView& frame) override;
    bool readTime(WallClockSnapshot& snapshot) override;
private:
    WallClock clock_;
    TimeSyncCoordinator coordinator_;
    IClock* tick_;
    IRandom* random_;
    uint32_t nextAttempt_;
    uint32_t generation_;
    bool scheduled_;
    TimeSyncService(const TimeSyncService&) = delete;
    TimeSyncService& operator=(const TimeSyncService&) = delete;
};
}
#endif
