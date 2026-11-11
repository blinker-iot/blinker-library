#ifndef BLINKER_CORE_TASKCALENDARCURSOR_H
#define BLINKER_CORE_TASKCALENDARCURSOR_H

#include "TaskTiming.h"

namespace blinker {

// Shared by the cursor and the executor's post-commit recheck. The previous
// anchor must already have passed the calendar quality gate.
bool taskCalendarContinues(uint64_t previousUtc, uint32_t previousRevision,
                           uint32_t elapsed, const WallClockSnapshot& time);

enum class CalendarPoll : uint8_t {
    WaitingForTime, Armed, Rebased, Exhausted, Due, Skipped
};

struct TaskCalendarDecision {
    CalendarPoll state;
    uint64_t occurrenceUtcMillis;
    TaskCalendarDecision()
        : state(CalendarPoll::WaitingForTime), occurrenceUtcMillis(0U) {}
};

// One immutable calendar definition per cursor; reset on definition changes or
// disable/re-enable. RAM consumption is NOT a durable Claim or authorization.
// Snapshots must come from the same WallClock, read at nowMillis. Call at gaps
// < 2^31 ms; reset after a known tick reset/unmeasured sleep. No heap or I/O.
class TaskCalendarCursor {
public:
    TaskCalendarCursor()
        : lastUtcMillis_(0U), watermark_(0U), nextUtcMillis_(0U),
          lastMillis_(0U), sampleRevision_(0U) {}

    // Restore only a verified consumed occurrence, never an old clock anchor.
    Result reset(uint64_t consumedUtcMillis = 0U);
    Result poll(const TaskTiming& timing, const WallClockSnapshot& time,
                uint32_t nowMillis, TaskCalendarDecision& output);
    uint64_t nextUtcMillis() const { return nextUtcMillis_; }
    uint64_t watermark() const { return watermark_; }

    static const uint32_t kSmallCorrectionMillis = 1000U;

private:
    uint64_t lastUtcMillis_;
    uint64_t watermark_;
    uint64_t nextUtcMillis_;
    uint32_t lastMillis_;
    uint32_t sampleRevision_;
};

} // namespace blinker

#endif
