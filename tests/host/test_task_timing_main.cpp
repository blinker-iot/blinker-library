#include <cstdlib>
#include <iostream>
#include <type_traits>
#include "BlinkerV2/core/TaskTiming.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

static_assert(sizeof(TaskCountdown) == 8U, "Countdown holds only monotonic state");
static_assert(sizeof(TaskTiming) <= 24U, "Bound timing definition size");
static_assert(!std::is_polymorphic<TaskCountdown>::value, "No virtual dispatch");

namespace {
const uint64_t day = 86400000ULL;
const uint64_t minute = 60000ULL;
const uint64_t tuesday = 1790035200000ULL; // 2026-09-22 00:00 UTC

TaskTiming weekly(uint16_t time = 480U, int16_t offset = 0, uint8_t mask = 0x7FU) {
    TaskTiming result;
    result.kind = TaskTimingKind::Weekly;
    result.minuteOfDay = time;
    result.utcOffsetMinutes = offset;
    result.weekdays = mask;
    return result;
}

uint64_t next(const TaskTiming& timing, uint64_t cursor) {
    uint64_t output = UINT64_MAX;
    CHECK(nextTaskUtc(timing, cursor, output));
    CHECK(output > cursor && output >= kWallTimeMinimumMillis && output < kWallTimeEndMillis);
    return output;
}

void invalidInput() {
    CHECK(!validateTaskTiming(TaskTiming()));
    for (unsigned i = 0; i < 9U; ++i) {
        TaskTiming value = weekly();
        switch (i) {
        case 0: value.kind = static_cast<TaskTimingKind>(255U); break;
        case 1: value.weekdays = 0U; break;
        case 2: value.weekdays = 128U; break;
        case 3: value.minuteOfDay = 1440U; break;
        case 4: value.utcOffsetMinutes = -721; break;
        case 5: value.utcOffsetMinutes = 841; break;
        case 6: value.utcMillis = tuesday; break;
        case 7: value.durationMillis = 1U; break;
        default: value.utcOffsetMinutes = INT16_MIN; break;
        }
        uint64_t output = 77U;
        CHECK(nextTaskUtc(value, tuesday, output).code() == ErrorCode::InvalidArgument);
        CHECK(output == 0U);
    }
    const uint64_t invalid[] = {0U, kWallTimeMinimumMillis - 2U, kWallTimeEndMillis, UINT64_MAX};
    for (uint64_t cursor : invalid) {
        uint64_t output = 77U;
        CHECK(nextTaskUtc(weekly(), cursor, output).code() == ErrorCode::ValueOutOfRange);
        CHECK(output == 0U);
    }
    TaskTiming timer;
    timer.durationMillis = 100U;
    CHECK(validateTaskTiming(timer));
    uint64_t output = 77U;
    CHECK(nextTaskUtc(timer, tuesday, output).code() == ErrorCode::UnsupportedFeature && output == 0U);
    timer.durationMillis = 0x80000000U;
    CHECK(!validateTaskTiming(timer));
}

void once() {
    TaskTiming value;
    value.kind = TaskTimingKind::UtcOnce;
    value.utcMillis = tuesday;
    CHECK(next(value, tuesday - 1U) == tuesday);
    uint64_t output = 77U;
    CHECK(nextTaskUtc(value, tuesday, output).code() == ErrorCode::NotFound && output == 0U);
    value.utcMillis = kWallTimeMinimumMillis;
    CHECK(next(value, kWallTimeMinimumMillis - 1U) == kWallTimeMinimumMillis);
    value.utcMillis = kWallTimeEndMillis - 1U;
    CHECK(next(value, kWallTimeEndMillis - 2U) == kWallTimeEndMillis - 1U);
    value.utcMillis = kWallTimeEndMillis;
    CHECK(!validateTaskTiming(value));
    value.utcMillis = tuesday;
    value.weekdays = 1U;
    CHECK(!validateTaskTiming(value));
}

void calendar() {
    CHECK(next(weekly(), tuesday) == tuesday + 480U * minute);
    CHECK(next(weekly(), tuesday + 480U * minute) == tuesday + day + 480U * minute);
    CHECK(next(weekly(480U, 480), tuesday - 1U) == tuesday); // UTC+8 08:00
    CHECK(next(weekly(0U, -720), tuesday) == tuesday + 720U * minute);
    CHECK(next(weekly(0U, 840), tuesday) == tuesday + 600U * minute);
    CHECK(next(weekly(0U, 345), tuesday) == tuesday + 1095U * minute); // Nepal
    CHECK(next(weekly(0U, 0, 1U), tuesday) == tuesday + 6U * day); // Monday
    CHECK(next(weekly(0U, 0, 2U), tuesday) == tuesday + 7U * day); // Next Tuesday
    CHECK(next(weekly(0U, 480, 2U), tuesday - 481U * minute) == tuesday - 480U * minute);
    CHECK(next(weekly(0U), 1709078400000ULL) == 1709164800000ULL); // Feb 28 -> 29
    CHECK(next(weekly(0U), 1709164800000ULL) == 1709251200000ULL); // Feb 29 -> Mar 1
    CHECK(next(weekly(0U), 1798761540000ULL) == 1798761600000ULL); // Dec 31 -> Jan 1
    CHECK(next(weekly(0U, 0, 32U), kWallTimeMinimumMillis - 1U) == kWallTimeMinimumMillis); // Saturday
    CHECK(next(weekly(0U, 840), kWallTimeMinimumMillis - 1U) == kWallTimeMinimumMillis + 600U * minute);
    CHECK(next(weekly(0U, -720), kWallTimeMinimumMillis - 1U) == kWallTimeMinimumMillis + 720U * minute);
    uint64_t output = 77U;
    CHECK(nextTaskUtc(weekly(), kWallTimeEndMillis - 1U, output).code() == ErrorCode::NotFound && output == 0U);
}

void exhaustiveWeek() {
    // Oracle advances weekdays from an independently known Monday, rather
    // than using the implementation's Unix-epoch weekday formula.
    const uint64_t monday = tuesday - day;
    const int16_t offsets[] = {-720, -330, 0, 345, 480, 840};
    const uint16_t minutes[] = {0U, 479U, 480U, 481U, 1439U};
    for (int16_t offset : offsets) {
        for (uint8_t mask = 1U; mask < 128U; ++mask) {
            for (unsigned weekday = 0U; weekday < 7U; ++weekday) {
                for (uint16_t clockMinute : minutes) {
                    const uint64_t local = monday + weekday * day + clockMinute * minute;
                    const uint64_t cursor = static_cast<uint64_t>(static_cast<int64_t>(local) - offset * 60000LL);
                    unsigned distance = 0U;
                    for (; distance <= 7U; ++distance) {
                        if ((mask & (1U << ((weekday + distance) % 7U))) &&
                            (distance != 0U || clockMinute < 480U)) break;
                    }
                    CHECK(distance <= 7U);
                    const uint64_t expected = cursor - clockMinute * minute + distance * day + 480U * minute;
                    CHECK(next(weekly(480U, offset, mask), cursor) == expected);
                }
            }
        }
    }
}

void countdown() {
    TaskCountdown timer;
    CHECK(timer.poll(1U) == CountdownPoll::Inactive);
    CHECK(!timer.start(0U, 0U) && !timer.start(0x80000000U, 0U));
    CHECK(timer.start(1000U, 100U));
    CHECK(timer.start(5000U, 100U).code() == ErrorCode::AlreadyExists);
    CHECK(timer.poll(600U) == CountdownPoll::Waiting && timer.remainingMillis() == 500U);
    CHECK(timer.poll(600U) == CountdownPoll::Waiting && timer.remainingMillis() == 500U);
    CHECK(timer.poll(1100U) == CountdownPoll::Due && !timer.active());
    CHECK(timer.poll(1100U) == CountdownPoll::Inactive);
    CHECK(timer.start(32U, 0xFFFFFFF0U));
    CHECK(timer.poll(0U) == CountdownPoll::Waiting && timer.remainingMillis() == 16U);
    CHECK(timer.poll(16U) == CountdownPoll::Due);
    CHECK(timer.start(100U, 0U));
    CHECK(timer.poll(1100U) == CountdownPoll::Due); // Boundary grace.
    CHECK(timer.start(100U, 0U));
    CHECK(timer.poll(1101U) == CountdownPoll::Skipped);
    CHECK(timer.poll(1101U) == CountdownPoll::Inactive);
    CHECK(timer.start(100U, 1000U));
    CHECK(timer.poll(999U) == CountdownPoll::Interrupted && !timer.active());
    CHECK(timer.start(0x7FFFFFFFU, 10U));
    CHECK(timer.poll(0x80000009U) == CountdownPoll::Due);
    CHECK(timer.start(100U, 0U));
    CHECK(timer.poll(0x80000000U) == CountdownPoll::Interrupted);
    CHECK(timer.start(100U, 0U));
    timer.cancel();
    CHECK(timer.poll(100U) == CountdownPoll::Inactive);
    CHECK(TaskCountdown().poll(100U) == CountdownPoll::Inactive); // Boot never resumes.
}
} // namespace

int main() {
    invalidInput(); once(); calendar(); exhaustiveWeek(); countdown();
    std::cout << "task timing: boundaries + 26670 calendar combinations passed; sizeof timing="
              << sizeof(TaskTiming) << ", countdown=" << sizeof(TaskCountdown) << '\n';
}
