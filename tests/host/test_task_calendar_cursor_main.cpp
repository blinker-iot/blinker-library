#include <cstdlib>
#include <iostream>
#include <type_traits>
#include "BlinkerV2/core/TaskCalendarCursor.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
static_assert(sizeof(TaskCalendarCursor) <= 32U, "Bound calendar cursor RAM");
static_assert(!std::is_polymorphic<TaskCalendarCursor>::value, "No virtual dispatch");

namespace {
const uint64_t base = 1790035200000ULL;
const uint64_t day = 86400000ULL;

TaskTiming daily() {
    TaskTiming timing;
    timing.kind = TaskTimingKind::Weekly;
    timing.weekdays = 0x7FU;
    timing.minuteOfDay = 1U;
    return timing;
}

WallClockSnapshot at(uint64_t utc, uint32_t revision = 1U) {
    WallClockSnapshot time;
    time.utcMillis = utc;
    time.sampleRevision = revision;
    time.state = WallClockState::Synchronized;
    time.source = WallTimeSource::Server;
    return time;
}

TaskCalendarDecision poll(TaskCalendarCursor& cursor, const TaskTiming& timing,
                          const WallClockSnapshot& time, uint32_t tick) {
    TaskCalendarDecision output;
    CHECK(cursor.poll(timing, time, tick, output));
    return output;
}

void deadlines() {
    for (uint32_t lateness : {0U, 1U, 999U, 1000U, 1001U, 60000U}) {
        TaskCalendarCursor cursor;
        const TaskTiming timing = daily();
        CHECK(poll(cursor, timing, at(base), 0U).state == CalendarPoll::Rebased);
        CHECK(cursor.nextUtcMillis() == base + 60000U);
        CHECK(poll(cursor, timing, at(base + 59999U), 59999U).state == CalendarPoll::Armed);
        const auto due = poll(cursor, timing, at(base + 60000U + lateness), 60000U + lateness);
        CHECK(due.state == (lateness <= 1000U ? CalendarPoll::Due : CalendarPoll::Skipped));
        CHECK(due.occurrenceUtcMillis == base + 60000U);
        CHECK(cursor.nextUtcMillis() == base + day + 60000U);
        CHECK(poll(cursor, timing, at(base + 60000U + lateness), 60000U + lateness).state == CalendarPoll::Armed);
    }
    TaskCalendarCursor cursor;
    TaskTiming once;
    once.kind = TaskTimingKind::UtcOnce;
    once.utcMillis = base + 60000U;
    CHECK(poll(cursor, once, at(base), 0U).state == CalendarPoll::Rebased);
    CHECK(poll(cursor, once, at(once.utcMillis), 60000U).state == CalendarPoll::Due);
    CHECK(cursor.nextUtcMillis() == 0U);
    CHECK(poll(cursor, once, at(once.utcMillis), 60000U).state == CalendarPoll::Exhausted);
    CHECK(cursor.reset());
    CHECK(poll(cursor, once, at(once.utcMillis), 0U).state == CalendarPoll::Exhausted); // Boot at due skips.
}

void corrections() {
    for (int change : {-1001, -1000, -1, 0, 1, 1000, 1001, 60000}) {
        TaskCalendarCursor cursor;
        poll(cursor, daily(), at(base), 0U);
        const uint64_t changed = static_cast<uint64_t>(static_cast<int64_t>(base + 60000U) + change);
        auto result = poll(cursor, daily(), at(changed, 2U), 60000U);
        if (change < -1000 || change > 1000) {
            CHECK(result.state == CalendarPoll::Rebased);
            CHECK(result.occurrenceUtcMillis == 0U);
        } else if (change >= 0) {
            CHECK(result.state == CalendarPoll::Due);
        } else {
            CHECK(result.state == CalendarPoll::Armed);
            result = poll(cursor, daily(), at(base + 60000U, 2U), 60000U + static_cast<uint32_t>(-change));
            CHECK(result.state == CalendarPoll::Due);
        }
    }
    TaskCalendarCursor cursor;
    poll(cursor, daily(), at(base), 0U);
    CHECK(poll(cursor, daily(), at(base + 120000U, 2U), 1000U).state == CalendarPoll::Rebased);
    CHECK(poll(cursor, daily(), at(base, 3U), 2000U).state == CalendarPoll::Rebased);
    CHECK(cursor.nextUtcMillis() == base + day + 60000U);
    CHECK(poll(cursor, daily(), at(base + 60000U, 3U), 62000U).state == CalendarPoll::Armed);
    // UTC changing without a new revision or revision going backward rebases.
    CHECK(poll(cursor, daily(), at(base + 60001U, 3U), 62000U).state == CalendarPoll::Rebased);
    CHECK(poll(cursor, daily(), at(base + 60002U, 1U), 62001U).state == CalendarPoll::Rebased);
}

void unavailableAndRestart() {
    for (unsigned fault = 0U; fault < 8U; ++fault) {
        TaskCalendarCursor cursor;
        poll(cursor, daily(), at(base), 0U);
        WallClockSnapshot bad = at(base + 1000U);
        switch (fault) {
        case 0: bad.state = WallClockState::Stale; break;
        case 1: bad.state = WallClockState::Unsynchronized; break;
        case 2: bad.source = WallTimeSource::None; break;
        case 3: bad.sampleRevision = 0U; break;
        case 4: bad.uncertaintyMillis = 5001U; break;
        case 5: bad.ageMillis = 3600000U; break;
        case 6: bad.utcMillis = kWallTimeMinimumMillis - 1U; break;
        default: bad.utcMillis = kWallTimeEndMillis; break;
        }
        CHECK(poll(cursor, daily(), bad, 1000U).state == CalendarPoll::WaitingForTime);
        CHECK(cursor.nextUtcMillis() == 0U);
        CHECK(poll(cursor, daily(), at(base + 60000U, 2U), 60000U).state == CalendarPoll::Rebased);
        CHECK(cursor.nextUtcMillis() == base + day + 60000U);
    }
    TaskCalendarCursor cursor;
    CHECK(cursor.reset(base + 60000U)); // Verified durable consumption after reboot.
    CHECK(poll(cursor, daily(), at(base), 0U).state == CalendarPoll::Rebased);
    CHECK(cursor.nextUtcMillis() == base + day + 60000U);
    CHECK(!cursor.reset(kWallTimeEndMillis));
    CHECK(cursor.watermark() == base + 60000U);
    CHECK(cursor.reset()); // New definition, not restoring an old time anchor.
    CHECK(poll(cursor, daily(), at(base), 0U).state == CalendarPoll::Rebased);
    CHECK(cursor.nextUtcMillis() == base + 60000U);
}

void realClockAndTick() {
    WallClock clock;
    WallClockSample sample;
    sample.utcMillis = base + 59000U;
    sample.source = WallTimeSource::Controller;
    CHECK(clock.synchronize(sample, 0xFFFFFF00U));
    TaskCalendarCursor cursor;
    WallClockSnapshot time;
    CHECK(clock.read(0xFFFFFF00U, time));
    poll(cursor, daily(), time, 0xFFFFFF00U);
    CHECK(clock.read(744U, time));
    CHECK(poll(cursor, daily(), time, 744U).state == CalendarPoll::Due);
    CHECK(poll(cursor, daily(), time, 743U).state == CalendarPoll::Rebased); // Tick backward.
    CHECK(poll(cursor, daily(), time, 0x800002E7U).state == CalendarPoll::Rebased); // Half range.
    clock.clear();
    CHECK(!clock.read(1000U, time));
    CHECK(poll(cursor, daily(), time, 1000U).state == CalendarPoll::WaitingForTime);
    sample.utcMillis = base + 59000U;
    CHECK(clock.synchronize(sample, 1000U));
    CHECK(clock.read(1000U, time));
    poll(cursor, daily(), time, 1000U);
    CHECK(cursor.nextUtcMillis() == base + day + 60000U); // Cleared source cannot replay.
}

void invalidAndBoundaries() {
    TaskCalendarCursor cursor;
    TaskCalendarDecision output;
    output.occurrenceUtcMillis = 1U;
    CHECK(!cursor.poll(TaskTiming(), at(base), 0U, output));
    CHECK(output.occurrenceUtcMillis == 0U);
    TaskTiming countdown;
    countdown.durationMillis = 1U;
    CHECK(cursor.poll(countdown, at(base), 0U, output).code() == ErrorCode::UnsupportedFeature);
    CHECK(poll(cursor, daily(), at(kWallTimeEndMillis - 1U), 0U).state == CalendarPoll::Exhausted);
    CHECK(cursor.reset());
    CHECK(poll(cursor, daily(), at(kWallTimeMinimumMillis), 0U).state == CalendarPoll::Rebased);
    CHECK(cursor.nextUtcMillis() == kWallTimeMinimumMillis + 60000U);
}

void boundedMultipleTasks() {
    // Future engine visits stable taskId order and dispatches at most one per
    // loop. Independent cursors must not steal another task's occurrence.
    TaskCalendarCursor cursors[10];
    for (auto& cursor : cursors) poll(cursor, daily(), at(base), 0U);
    for (uint32_t i = 0U; i < 10U; ++i) {
        const uint32_t tick = 60000U + i * 100U;
        CHECK(poll(cursors[i], daily(), at(base + tick), tick).state == CalendarPoll::Due);
        CHECK(poll(cursors[i], daily(), at(base + tick), tick).state == CalendarPoll::Armed);
    }
}
} // namespace

int main() {
    deadlines(); corrections(); unavailableAndRestart(); realClockAndTick();
    invalidAndBoundaries(); boundedMultipleTasks();
    std::cout << "calendar cursor: six groups passed; sizeof=" << sizeof(TaskCalendarCursor) << '\n';
}
