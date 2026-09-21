#include "WallClock.h"

namespace blinker {
namespace {

const uint32_t kWallTimeMaxTickGap = 0x7FFFFFFFU;

bool validWallTimeSource(WallTimeSource source) {
    return source == WallTimeSource::Server ||
           source == WallTimeSource::Controller ||
           source == WallTimeSource::Gateway ||
           source == WallTimeSource::Platform;
}

} // namespace

WallClock::WallClock(const WallClockConfig& config)
    : utcMillis_(0U), config_(config), lastMillis_(0U), ageMillis_(0U),
      sampleUncertaintyMillis_(0U), source_(WallTimeSource::None),
      state_(WallClockState::Unsynchronized) {}

Result WallClock::synchronize(const WallClockSample& sample, uint32_t nowMillis) {
    if (config_.maxAgeMillis == 0U || config_.maxAgeMillis > kWallTimeMaxTickGap ||
        config_.maxUncertaintyMillis == 0U || config_.maxUncertaintyMillis > kWallTimeMaxUncertaintyMillis ||
        config_.driftPpm > 1000000U || !validWallTimeSource(sample.source)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (sample.utcMillis < kWallTimeMinimumMillis || sample.utcMillis >= kWallTimeEndMillis ||
        sample.uncertaintyMillis > config_.maxUncertaintyMillis) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    utcMillis_ = sample.utcMillis;
    lastMillis_ = nowMillis;
    ageMillis_ = 0U;
    sampleUncertaintyMillis_ = sample.uncertaintyMillis;
    source_ = sample.source;
    state_ = WallClockState::Synchronized;
    return Result::success();
}

uint64_t WallClock::uncertaintyMillis() const {
    return sampleUncertaintyMillis_ +
        (static_cast<uint64_t>(ageMillis_) * config_.driftPpm + 999999U) / 1000000U;
}

void WallClock::poll(uint32_t nowMillis) {
    if (state_ != WallClockState::Synchronized) return;
    const uint32_t elapsed = nowMillis - lastMillis_;
    if (elapsed > kWallTimeMaxTickGap || elapsed >= config_.maxAgeMillis - ageMillis_ ||
        elapsed >= kWallTimeEndMillis - utcMillis_) {
        expire();
        return;
    }
    ageMillis_ += elapsed;
    lastMillis_ = nowMillis;
    utcMillis_ += elapsed;
    if (uncertaintyMillis() > config_.maxUncertaintyMillis) expire();
}

bool WallClock::read(uint32_t nowMillis, WallClockSnapshot& output) {
    poll(nowMillis);
    output = WallClockSnapshot();
    output.state = state_;
    if (state_ != WallClockState::Synchronized) return false;
    output.utcMillis = utcMillis_;
    output.uncertaintyMillis = static_cast<uint32_t>(uncertaintyMillis());
    output.ageMillis = ageMillis_;
    output.source = source_;
    return true;
}

void WallClock::clear() {
    utcMillis_ = 0U;
    lastMillis_ = 0U;
    ageMillis_ = 0U;
    sampleUncertaintyMillis_ = 0U;
    source_ = WallTimeSource::None;
    state_ = WallClockState::Unsynchronized;
}

void WallClock::expire() {
    clear();
    state_ = WallClockState::Stale;
}

} // namespace blinker
