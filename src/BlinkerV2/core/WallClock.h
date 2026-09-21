#ifndef BLINKER_CORE_WALLCLOCK_H
#define BLINKER_CORE_WALLCLOCK_H

#include "Result.h"

namespace blinker {

static const uint64_t kWallTimeMinimumMillis = 946684800000ULL;
static const uint64_t kWallTimeEndMillis = 4102444800000ULL; // Exclusive.
static const uint32_t kWallTimeMaxUncertaintyMillis = 60000U;

// Business time only. A source tag is provenance, not authorization or trust.
enum class WallTimeSource : uint8_t { None, Server, Controller, Gateway, Platform };
enum class WallClockState : uint8_t { Unsynchronized, Synchronized, Stale };

struct WallClockConfig {
    uint32_t maxAgeMillis;
    uint32_t maxUncertaintyMillis;
    uint32_t driftPpm;

    WallClockConfig()
        : maxAgeMillis(3600000U), maxUncertaintyMillis(5000U), driftPpm(1000U) {}
};

struct WallClockSample {
    uint64_t utcMillis;
    uint32_t uncertaintyMillis;
    WallTimeSource source;

    WallClockSample()
        : utcMillis(0U), uncertaintyMillis(0U), source(WallTimeSource::None) {}
};

struct WallClockSnapshot {
    uint64_t utcMillis;
    uint32_t uncertaintyMillis;
    uint32_t ageMillis;
    WallTimeSource source;
    WallClockState state;

    WallClockSnapshot()
        : utcMillis(0U), uncertaintyMillis(0U), ageMillis(0U),
          source(WallTimeSource::None), state(WallClockState::Unsynchronized) {}
};

// Deliberately not IClock: business samples must not change credential expiry.
// Call poll at intervals < 2^31 ms. Clear on boot/tick reset or unmeasured sleep;
// no 32-bit tick can distinguish an unobserved full wrap from a short interval.
class WallClock {
public:
    explicit WallClock(const WallClockConfig& config = WallClockConfig());

    // The caller admits the source and correlates a fresh response before here.
    Result synchronize(const WallClockSample& sample, uint32_t nowMillis);
    void poll(uint32_t nowMillis);
    bool read(uint32_t nowMillis, WallClockSnapshot& output);
    void clear();

private:
    void expire();
    uint64_t uncertaintyMillis() const;

    uint64_t utcMillis_;
    WallClockConfig config_;
    uint32_t lastMillis_;
    uint32_t ageMillis_;
    uint32_t sampleUncertaintyMillis_;
    WallTimeSource source_;
    WallClockState state_;
};

} // namespace blinker

#endif
