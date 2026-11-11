#include "TaskCalendarCursor.h"

namespace blinker {
namespace {

bool taskCalendarTimeUsable(const WallClockSnapshot& time) {
    const WallClockConfig limits;
    return time.state == WallClockState::Synchronized &&
        time.source >= WallTimeSource::Server && time.source <= WallTimeSource::Platform &&
        time.sampleRevision != 0U && time.ageMillis < limits.maxAgeMillis &&
        time.uncertaintyMillis <= limits.maxUncertaintyMillis &&
        time.utcMillis >= kWallTimeMinimumMillis && time.utcMillis < kWallTimeEndMillis;
}

} // namespace

bool taskCalendarContinues(uint64_t previousUtc, uint32_t previousRevision,
                           uint32_t elapsed, const WallClockSnapshot& time) {
    if (!taskCalendarTimeUsable(time) || previousUtc == 0U || elapsed > 0x7FFFFFFFU ||
        time.sampleRevision < previousRevision) return false;
    const uint64_t projected = previousUtc + elapsed;
    const uint64_t correction = time.utcMillis >= projected
        ? time.utcMillis - projected : projected - time.utcMillis;
    return time.sampleRevision == previousRevision ? correction == 0U
        : correction <= TaskCalendarCursor::kSmallCorrectionMillis;
}

Result TaskCalendarCursor::reset(uint64_t consumedUtcMillis) {
    if (consumedUtcMillis != 0U && (consumedUtcMillis < kWallTimeMinimumMillis ||
                                   consumedUtcMillis >= kWallTimeEndMillis))
        return Result::failure(ErrorCode::ValueOutOfRange);
    lastUtcMillis_ = 0U;
    watermark_ = consumedUtcMillis;
    nextUtcMillis_ = 0U;
    lastMillis_ = 0U;
    sampleRevision_ = 0U;
    return Result::success();
}

Result TaskCalendarCursor::poll(const TaskTiming& timing, const WallClockSnapshot& time,
                                uint32_t nowMillis, TaskCalendarDecision& output) {
    output = TaskCalendarDecision();
    Result result = validateTaskTiming(timing);
    if (!result) return result;
    if (timing.kind == TaskTimingKind::Countdown)
        return Result::failure(ErrorCode::UnsupportedFeature);
    if (!taskCalendarTimeUsable(time)) {
        lastUtcMillis_ = 0U;
        nextUtcMillis_ = 0U;
        return Result::success(); // Keep the consumed/skipped watermark.
    }

    const uint32_t elapsed = nowMillis - lastMillis_;
    const bool continuous = taskCalendarContinues(lastUtcMillis_, sampleRevision_, elapsed, time);
    lastUtcMillis_ = time.utcMillis;
    lastMillis_ = nowMillis;
    sampleRevision_ = time.sampleRevision;
    if (time.utcMillis > watermark_) watermark_ = time.utcMillis;

    if (continuous && nextUtcMillis_ != 0U && time.utcMillis < nextUtcMillis_) {
        output.state = CalendarPoll::Armed;
        return Result::success();
    }
    if (continuous && nextUtcMillis_ != 0U) {
        output.occurrenceUtcMillis = nextUtcMillis_;
        output.state = time.utcMillis - nextUtcMillis_ <= TaskCountdown::kLatenessMillis
            ? CalendarPoll::Due : CalendarPoll::Skipped;
    } else {
        output.state = continuous ? CalendarPoll::Exhausted : CalendarPoll::Rebased;
    }

    // Strictly future from the largest visited/consumed UTC. A backward step
    // cannot resurrect a skipped occurrence. Due is consumed in RAM once; the
    // caller still must persist Claim and revalidate before any side effect.
    result = nextTaskUtc(timing, watermark_, nextUtcMillis_);
    if (result.code() == ErrorCode::NotFound) {
        if (output.occurrenceUtcMillis == 0U) output.state = CalendarPoll::Exhausted;
        return Result::success();
    }
    return result;
}

} // namespace blinker
