#include "TaskTiming.h"

namespace blinker {

Result validateTaskTiming(const TaskTiming& timing) {
    switch (timing.kind) {
    case TaskTimingKind::Countdown:
        if (timing.durationMillis == 0U || timing.durationMillis > 0x7FFFFFFFU ||
            timing.utcMillis != 0U || timing.minuteOfDay != 0U ||
            timing.utcOffsetMinutes != 0 || timing.weekdays != 0U)
            return Result::failure(ErrorCode::InvalidArgument);
        break;
    case TaskTimingKind::UtcOnce:
        if (timing.utcMillis < kWallTimeMinimumMillis ||
            timing.utcMillis >= kWallTimeEndMillis || timing.durationMillis != 0U ||
            timing.minuteOfDay != 0U || timing.utcOffsetMinutes != 0 ||
            timing.weekdays != 0U)
            return Result::failure(ErrorCode::InvalidArgument);
        break;
    case TaskTimingKind::Weekly:
        if (timing.utcMillis != 0U || timing.durationMillis != 0U ||
            timing.minuteOfDay >= 1440U || timing.utcOffsetMinutes < -720 ||
            timing.utcOffsetMinutes > 840 || timing.weekdays == 0U ||
            (timing.weekdays & 0x80U) != 0U)
            return Result::failure(ErrorCode::InvalidArgument);
        break;
    default:
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result nextTaskUtc(const TaskTiming& timing, uint64_t afterUtcMillis,
                   uint64_t& nextUtcMillis) {
    nextUtcMillis = 0U;
    Result result = validateTaskTiming(timing);
    if (!result) return result;
    if (timing.kind == TaskTimingKind::Countdown)
        return Result::failure(ErrorCode::UnsupportedFeature);
    if (afterUtcMillis < kWallTimeMinimumMillis - 1U ||
        afterUtcMillis >= kWallTimeEndMillis)
        return Result::failure(ErrorCode::ValueOutOfRange);
    if (timing.kind == TaskTimingKind::UtcOnce) {
        if (timing.utcMillis <= afterUtcMillis)
            return Result::failure(ErrorCode::NotFound);
        nextUtcMillis = timing.utcMillis;
        return Result::success();
    }

    // Range validation above keeps this signed arithmetic positive and far
    // below INT64_MAX, including offsets crossing UTC dates at both ends.
    const int64_t dayMillis = 86400000LL;
    const int64_t offset = static_cast<int64_t>(timing.utcOffsetMinutes) * 60000LL;
    const int64_t firstDay = (static_cast<int64_t>(afterUtcMillis) + offset) / dayMillis;
    for (int delta = 0; delta <= 7; ++delta) {
        const int64_t day = firstDay + delta;
        // 1970-01-01 was Thursday (index 3 with Monday as zero).
        const unsigned weekday = static_cast<unsigned>((day + 3) % 7);
        if ((timing.weekdays & (1U << weekday)) == 0U) continue;
        const int64_t utc = day * dayMillis +
            static_cast<int64_t>(timing.minuteOfDay) * 60000LL - offset;
        if (utc <= static_cast<int64_t>(afterUtcMillis)) continue;
        if (utc >= static_cast<int64_t>(kWallTimeEndMillis)) break;
        nextUtcMillis = static_cast<uint64_t>(utc);
        return Result::success();
    }
    return Result::failure(ErrorCode::NotFound);
}

Result TaskCountdown::start(uint32_t durationMillis, uint32_t nowMillis) {
    if (durationMillis == 0U || durationMillis > 0x7FFFFFFFU)
        return Result::failure(ErrorCode::InvalidArgument);
    if (active()) return Result::failure(ErrorCode::AlreadyExists);
    lastMillis_ = nowMillis;
    remainingMillis_ = durationMillis;
    return Result::success();
}

CountdownPoll TaskCountdown::poll(uint32_t nowMillis) {
    if (!active()) return CountdownPoll::Inactive;
    const uint32_t elapsed = nowMillis - lastMillis_;
    lastMillis_ = nowMillis;
    if (elapsed > 0x7FFFFFFFU) {
        cancel();
        return CountdownPoll::Interrupted;
    }
    if (elapsed < remainingMillis_) {
        remainingMillis_ -= elapsed;
        return CountdownPoll::Waiting;
    }
    const uint32_t late = elapsed - remainingMillis_;
    cancel(); // Consume once; the caller owns any durable execution intent.
    return late <= kLatenessMillis ? CountdownPoll::Due : CountdownPoll::Skipped;
}

} // namespace blinker
