#ifndef BLINKER_CORE_TASKTIMING_H
#define BLINKER_CORE_TASKTIMING_H

#include "WallClock.h"

namespace blinker {

// Internal timing primitives, not a task store, owner grant or public facade.
// Daily is Weekly with all seven bits set; bit 0 is Monday, bit 6 Sunday.
enum class TaskTimingKind : uint8_t { Countdown, UtcOnce, Weekly };

struct TaskTiming {
    uint64_t utcMillis;
    uint32_t durationMillis;
    uint16_t minuteOfDay;
    int16_t utcOffsetMinutes;
    uint8_t weekdays;
    TaskTimingKind kind;

    TaskTiming()
        : utcMillis(0U), durationMillis(0U), minuteOfDay(0U),
          utcOffsetMinutes(0), weekdays(0U), kind(TaskTimingKind::Countdown) {}
};

// Reject inactive union fields as well as invalid ranges. No wire layout implied.
Result validateTaskTiming(const TaskTiming& timing);

// Strictly AFTER the caller's cursor, in the existing business UTC range.
// The cursor may be kWallTimeMinimumMillis - 1 to include the lower boundary.
// NotFound means no remaining occurrence; output is zero on every failure.
// Clock quality, consumed occurrences and correction/reboot policy belong to
// the caller. This function alone does NOT admit execution of an occurrence.
Result nextTaskUtc(const TaskTiming& timing, uint64_t afterUtcMillis,
                   uint64_t& nextUtcMillis);

enum class CountdownPoll : uint8_t { Inactive, Waiting, Due, Skipped, Interrupted };

// Volatile, one-shot monotonic countdown. Not a persistent task identity.
// Must poll at least every 2^31 ms; cancel on known tick reset/unmeasured sleep.
// An unobserved full tick wrap cannot be detected with a uint32_t tick alone.
class TaskCountdown {
public:
    TaskCountdown() : lastMillis_(0U), remainingMillis_(0U) {}

    // An active start is rejected, never silently restarts the deadline.
    Result start(uint32_t durationMillis, uint32_t nowMillis);
    CountdownPoll poll(uint32_t nowMillis);
    void cancel() { remainingMillis_ = 0U; }
    bool active() const { return remainingMillis_ != 0U; }
    uint32_t remainingMillis() const { return remainingMillis_; }
    // Valid only while active; modulo uint32 tick, not a persistent UTC date.
    uint32_t deadlineMillis() const { return lastMillis_ + remainingMillis_; }

    // Loop lateness only, not a promise about business UTC accuracy.
    static const uint32_t kLatenessMillis = 1000U;

private:
    uint32_t lastMillis_;
    uint32_t remainingMillis_;
};

} // namespace blinker

#endif
