#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arduino/CompileCloudLocation/SerialObservation.h"

using namespace blinker;
using namespace blinker::cloud_location_candidate;
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)

namespace {
LocationFix latest;
uint32_t measured = 0U;
size_t fixes = 0U, losses = 0U;
bool fail = false;
Result accept(const LocationFix& observation, uint32_t sampledAt) {
    ++fixes; latest = observation; measured = sampledAt;
    return fail ? Result::failure(ErrorCode::WouldBlock) : validateLocationFix(observation);
}
void lost() { ++losses; }

ObservationReply line(SerialObservation& parser, const char* text) {
    ObservationReply reply;
    size_t terminal = 0U;
    // Same 32-byte budget the Arduino wrapper must enforce; chunks do not
    // invent a new observation or a new line when the input is incomplete.
    const size_t size = strlen(text);
    for (size_t start = 0U; start < size; start += SerialObservation::maximumBytesPerPoll) {
        size_t end = start + SerialObservation::maximumBytesPerPoll;
        if (end > size) end = size;
        for (size_t index = start; index < end; ++index) if (parser.push(static_cast<uint8_t>(text[index]), 1234U, reply)) ++terminal;
    }
    CHECK(terminal == 1U);
    return reply;
}

void valid() {
    SerialObservation parser(&accept, &lost);
    CHECK(fixes == 0U && losses == 0U);
    ObservationReply reply = line(parser, "FIX 1 -900000000 1800000000\r\n");
    CHECK(reply.status == ObservationStatus::Accepted && reply.kind == ObservationKind::Fix && reply.sequence == 1U);
    CHECK(strcmp(reply.source(), "InjectedNonSatellite") == 0);
    CHECK(fixes == 1U && measured == 1234U && latest.latitudeE7 == -900000000 && latest.longitudeE7 == 1800000000);
    CHECK(latest.timeQuality == LocationTimeQuality::Unknown && latest.capturedAtUtcMillis == 0U);
    CHECK(!latest.hasHorizontalAccuracy && latest.horizontalAccuracyCm == 0U);
    reply = line(parser, "FIX 2 0 0\n"); // Explicit user input, never a default fix.
    CHECK(reply.status == ObservationStatus::Accepted && latest.latitudeE7 == 0 && latest.longitudeE7 == 0);
    reply = line(parser, "LOST 3\n");
    CHECK(reply.status == ObservationStatus::Accepted && reply.kind == ObservationKind::Lost && losses == 1U);
    SerialObservation maximum(&accept, &lost);
    reply = line(maximum, "FIX 4294967295 -900000000 -1800000000\n"); // Split across bounded polls.
    CHECK(reply.status == ObservationStatus::Accepted && reply.sequence == UINT32_MAX);
}

void invalid() {
    SerialObservation parser(&accept, &lost);
    const size_t before = fixes + losses;
    const char* bad[] = {"\n", "FIX 0 1 2\n", "FIX 01 1 2\n", "FIX 4294967296 1 2\n", "FIX 1 900000001 2\n",
        "FIX 1 1 -1800000001\n", "FIX 1 1 2 \n", "FIX 1 +1 2\n", "FIX 1 -0 2\n", "FIX 1 01 2\n",
        "FIX 1 1.2 2\n", " FIX 1 1 2\n", "FIX 1  1 2\n", "FIX 1 1\n", "LOST 1 2\n", "FIX 1 1\r 2\n"};
    for (size_t i = 0U; i < sizeof(bad) / sizeof(bad[0]); ++i) CHECK(line(parser, bad[i]).status == ObservationStatus::Invalid);
    char huge[130]; memset(huge, 'x', sizeof(huge)); huge[sizeof(huge) - 2U] = '\n'; huge[sizeof(huge) - 1U] = '\0';
    CHECK(line(parser, huge).status == ObservationStatus::Invalid);
    ObservationReply reply;
    CHECK(!parser.push(0U, 1234U, reply)); CHECK(parser.push('\n', 1234U, reply));
    CHECK(reply.status == ObservationStatus::Invalid && parser.attempts() == 0U && fixes + losses == before);
    CHECK(line(parser, "LOST 1\n").status == ObservationStatus::Accepted); // One terminal rejection then clean resync.
}

void replayAndLimit() {
    SerialObservation parser(&accept, &lost);
    fail = true;
    CHECK(line(parser, "FIX 1 1 2\n").status == ObservationStatus::ApplyRejected);
    const size_t afterAttempt = fixes;
    const ObservationReply repeated = line(parser, "FIX 1 1 2\n");
    CHECK(repeated.status == ObservationStatus::OutOfOrder && repeated.error == ErrorCode::SequenceConflict && fixes == afterAttempt);
    fail = false;
    for (uint32_t i = 2U; i <= 16U; ++i) {
        char text[32]; snprintf(text, sizeof(text), "LOST %lu\n", static_cast<unsigned long>(i));
        CHECK(line(parser, text).status == ObservationStatus::Accepted);
    }
    const size_t lastLosses = losses;
    CHECK(parser.attempts() == 16U && parser.lastSequence() == 16U);
    CHECK(line(parser, "LOST 17\n").status == ObservationStatus::LimitReached && losses == lastLosses);
    SerialObservation missing(nullptr, nullptr);
    CHECK(line(missing, "LOST 1\n").status == ObservationStatus::ApplyRejected);
    CHECK(line(missing, "LOST 1\n").status == ObservationStatus::OutOfOrder);
}
}

int main() {
    valid(); invalid(); replayAndLimit();
    printf("location serial observation: 3 groups passed; parser=%zu B; max line=96 B, 16 attempts\n", sizeof(SerialObservation));
    return 0;
}
