#include <cstdlib>
#include <iostream>
#include <type_traits>
#include "BlinkerV2/core/WallClock.h"
#include "BlinkerV2/interface/IClock.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

static_assert(!std::is_base_of<IClock, WallClock>::value, "Business time is not a security clock");
static_assert(!std::is_polymorphic<WallClock>::value, "No virtual clock adapter");
static_assert(sizeof(WallClock) <= 48U, "Optional wall clock RAM budget");

namespace {
const uint64_t kUtc = 1789260000000ULL;
WallClockSample sample(uint64_t utc = kUtc, uint32_t uncertainty = 20U,
                       WallTimeSource source = WallTimeSource::Server) {
    WallClockSample result;
    result.utcMillis = utc;
    result.uncertaintyMillis = uncertainty;
    result.source = source;
    return result;
}

void noTimeAndClear() {
    WallClock clock;
    WallClockSnapshot out;
    out.utcMillis = kUtc;
    CHECK(!clock.read(1U, out));
    CHECK(out.state == WallClockState::Unsynchronized && out.utcMillis == 0U);
    CHECK(clock.synchronize(sample(), 100U));
    CHECK(clock.read(1100U, out));
    CHECK(out.utcMillis == kUtc + 1000U && out.ageMillis == 1000U);
    CHECK(out.uncertaintyMillis == 21U && out.source == WallTimeSource::Server);
    clock.clear();
    CHECK(!clock.read(1100U, out) && out.state == WallClockState::Unsynchronized);
    CHECK(out.utcMillis == 0U && out.ageMillis == 0U && out.uncertaintyMillis == 0U);
    CHECK(out.source == WallTimeSource::None);
}

void inputValidation() {
    WallClock clock;
    WallClockSnapshot out;
    CHECK(clock.synchronize(sample(), 100U));
    const uint64_t invalidTimes[] = {0U, 1789260000ULL, 946684799999ULL,
                                    4102444800000ULL, UINT64_MAX};
    for (uint64_t utc : invalidTimes) {
        CHECK(clock.synchronize(sample(utc), 100U).code() == ErrorCode::ValueOutOfRange);
        CHECK(clock.read(100U, out) && out.utcMillis == kUtc);
    }
    CHECK(clock.synchronize(sample(kUtc, 5001U), 100U).code() == ErrorCode::ValueOutOfRange);
    CHECK(clock.synchronize(sample(kUtc, 1U, WallTimeSource::None), 100U).code() == ErrorCode::InvalidArgument);
    CHECK(clock.synchronize(sample(kUtc, 1U, static_cast<WallTimeSource>(255)), 100U).code() == ErrorCode::InvalidArgument);
    CHECK(clock.read(100U, out) && out.utcMillis == kUtc);
    CHECK(clock.synchronize(sample(946684800000ULL), 100U));
    CHECK(clock.synchronize(sample(4102444799999ULL), 100U));
    CHECK(!clock.read(101U, out) && out.state == WallClockState::Stale);
    CHECK(out.utcMillis == 0U && out.source == WallTimeSource::None);
}

void configuration() {
    WallClockConfig config;
    for (unsigned i = 0; i < 5; ++i) {
        config = WallClockConfig();
        if (i == 0) config.maxAgeMillis = 0U;
        if (i == 1) config.maxAgeMillis = 0x80000000U;
        if (i == 2) config.maxUncertaintyMillis = 0U;
        if (i == 3) config.maxUncertaintyMillis = 60001U;
        if (i == 4) config.driftPpm = 1000001U;
        WallClock clock(config);
        CHECK(clock.synchronize(sample(), 0U).code() == ErrorCode::InvalidArgument);
        WallClockSnapshot out;
        CHECK(!clock.read(1U, out) && out.state == WallClockState::Unsynchronized);
    }
    config = WallClockConfig();
    config.maxAgeMillis = 1U;
    config.driftPpm = 0U;
    WallClock clock(config);
    CHECK(clock.synchronize(sample(kUtc, 0U), 0U));
    WallClockSnapshot out;
    CHECK(clock.read(0U, out) && out.uncertaintyMillis == 0U);
    CHECK(!clock.read(1U, out));
}

void ageAndRecovery() {
    WallClock clock;
    WallClockSnapshot out;
    CHECK(clock.synchronize(sample(), 100U));
    CHECK(clock.read(3600099U, out));
    CHECK(out.ageMillis == 3599999U && out.uncertaintyMillis == 3620U);
    CHECK(!clock.read(3600100U, out) && out.state == WallClockState::Stale);
    CHECK(!clock.read(101U, out)); // A later tick wrap does not revive stale time.
    CHECK(clock.synchronize(sample(), 101U));
    CHECK(clock.read(102U, out) && out.ageMillis == 1U);
}

void driftBudget() {
    WallClockConfig config;
    config.maxUncertaintyMillis = 21U;
    WallClock clock(config);
    WallClockSnapshot out;
    CHECK(clock.synchronize(sample(), 0U));
    CHECK(clock.read(1000U, out) && out.uncertaintyMillis == 21U);
    CHECK(!clock.read(1001U, out) && out.state == WallClockState::Stale);

    // Poll frequency must not compound ceil() error or truncate fractional age.
    WallClock frequent, once;
    CHECK(frequent.synchronize(sample(), 0U) && once.synchronize(sample(), 0U));
    for (uint32_t tick = 1; tick <= 10001U; ++tick) frequent.poll(tick);
    WallClockSnapshot a, b;
    CHECK(frequent.read(10001U, a) && once.read(10001U, b));
    CHECK(a.utcMillis == b.utcMillis && a.uncertaintyMillis == b.uncertaintyMillis);
    CHECK(a.uncertaintyMillis == 31U && a.ageMillis == 10001U);

    config.maxUncertaintyMillis = 60000U;
    config.driftPpm = 1000000U;
    WallClock extreme(config);
    CHECK(extreme.synchronize(sample(kUtc, 0U), 0U));
    CHECK(extreme.read(60000U, out) && out.uncertaintyMillis == 60000U);
    CHECK(!extreme.read(60001U, out));
}

void tickWrapAndRollback() {
    WallClock clock;
    WallClockSnapshot out;
    CHECK(clock.synchronize(sample(), UINT32_MAX - 9U));
    clock.poll(UINT32_MAX);
    CHECK(clock.read(15U, out));
    CHECK(out.ageMillis == 25U && out.utcMillis == kUtc + 25U);
    CHECK(!clock.read(14U, out) && out.state == WallClockState::Stale);
    CHECK(clock.synchronize(sample(), 0U));
    CHECK(!clock.read(0x80000000U, out));

    WallClockConfig config;
    config.maxAgeMillis = 0x7FFFFFFFU;
    config.driftPpm = 0U;
    WallClock longClock(config);
    CHECK(longClock.synchronize(sample(), 20U));
    CHECK(longClock.read(20U + 0x7FFFFFFEU, out));
    CHECK(!longClock.read(20U + 0x7FFFFFFFU, out));
}

void businessCorrectionAndIsolation() {
    class SecurityClock final : public IClock {
    public:
        uint32_t monotonicMillis() const override { return 4000U; }
        bool unixTime(uint64_t& seconds) const override { seconds = 2000000000U; return true; }
    } security;
    WallClock clock;
    WallClockSnapshot out;
    CHECK(clock.synchronize(sample(), security.monotonicMillis()));
    CHECK(clock.synchronize(sample(kUtc - 10000U, 5U, WallTimeSource::Controller), 5000U));
    CHECK(clock.read(5010U, out));
    CHECK(out.utcMillis == kUtc - 9990U && out.source == WallTimeSource::Controller);
    CHECK(clock.synchronize(sample(kUtc + 10000U, 5U, WallTimeSource::Gateway), 5010U));
    CHECK(clock.read(5010U, out) && out.utcMillis == kUtc + 10000U);
    CHECK(clock.synchronize(sample(kUtc, 5U, WallTimeSource::Platform), 5010U));
    uint64_t securityTime = 0U;
    CHECK(security.unixTime(securityTime) && securityTime == 2000000000U);
    CHECK(security.monotonicMillis() == 4000U);
    WallClock rebooted;
    CHECK(!rebooted.read(5010U, out) && out.state == WallClockState::Unsynchronized);
}

void sampleRevision() {
    WallClock clock;
    WallClockSnapshot out;
    CHECK(!clock.read(0U, out) && out.sampleRevision == 0U);
    CHECK(clock.synchronize(sample(), 100U));
    CHECK(clock.read(100U, out) && out.sampleRevision == 1U);
    CHECK(clock.read(200U, out) && out.sampleRevision == 1U);
    CHECK(!clock.synchronize(sample(0U), 200U));
    CHECK(clock.read(200U, out) && out.sampleRevision == 1U);
    // Even an identical accepted anchor is distinguishable; its timing effect
    // is for the task policy to classify, not a clock-owned task callback.
    CHECK(clock.synchronize(sample(kUtc + 100U), 200U));
    CHECK(clock.read(200U, out) && out.sampleRevision == 2U);
    clock.clear();
    CHECK(!clock.read(200U, out) && out.sampleRevision == 2U);
    clock.clear();
    CHECK(clock.synchronize(sample(kUtc - 10000U), 300U));
    CHECK(clock.read(300U, out) && out.sampleRevision == 3U);
    CHECK(!clock.read(3600300U, out) && out.sampleRevision == 3U);
    CHECK(out.state == WallClockState::Stale);
    CHECK(out.utcMillis == 0U && out.source == WallTimeSource::None);
    CHECK(clock.synchronize(sample(kUtc + 10000U), 3600300U));
    CHECK(clock.read(3600300U, out) && out.sampleRevision == 4U);
    WallClock rebooted;
    CHECK(!rebooted.read(3600300U, out) && out.sampleRevision == 0U);
}
} // namespace

int main() {
    noTimeAndClear();
    inputValidation();
    configuration();
    ageAndRecovery();
    driftBudget();
    tickWrapAndRollback();
    businessCorrectionAndIsolation();
    sampleRevision();
    std::cout << "WallClock: 8 groups passed; sizeof=" << sizeof(WallClock)
              << ", snapshot=" << sizeof(WallClockSnapshot) << '\n';
}
