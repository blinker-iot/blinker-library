#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "../internal/support/UtcSnapshotSerial.h"
#include "BlinkerV2/runtime/TimeSyncService.h"

using namespace blinker;
using blinker_test::UtcSnapshotSerial;
#define CHECK(condition) do { if (!(condition)) { std::cerr << "line " << __LINE__ << ": " << #condition << '\n'; std::exit(1); } } while (0)

namespace {
const uint64_t kUtc = 1793508000000ULL;
struct Tick final : IClock {
    uint32_t now = 1000U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& seconds) const override { seconds = 0U; return false; }
};
struct Random final : IRandom {
    Result fill(MutableByteSpan output) override {
        std::memset(output.data, 0x5a, output.size); return Result::success();
    }
};
std::string format(const UtcSnapshotSerial& serial) {
    char line[64] = {};
    size_t written = 0U;
    CHECK(serial.format(line, sizeof(line), written));
    CHECK(written > 0U && written <= 63U && written == std::strlen(line));
    CHECK(line[written - 1U] == '\n');
    return std::string(line, written);
}
bool drain(UtcSnapshotSerial& serial, size_t available, std::vector<std::string>& output) {
    const std::string line = format(serial);
    if (available < line.size()) return false; // Same exact-size gate as the Sketch.
    output.push_back(line); serial.consume(); return true;
}
void unavailableIsNotUtcZero() {
    TimeSyncService original;
    WallClockSnapshot wall;
    const bool valid = original.readTime(wall);
    CHECK(!valid && wall.state == WallClockState::Unsynchronized);
    UtcSnapshotSerial serial;
    CHECK(serial.capture(valid, wall, 12U, 13U));
    CHECK(format(serial) == "UTC1,1,0,0,0,0,12,13\n");
    serial.consume();
    CHECK(format(serial) == "UTC2,1,0,0,0\n");
    serial.consume(); CHECK(!serial.pending());
}
void originalTimePortAndStale() {
    Tick tick; Random random; TimeSyncService original;
    ITimeSync& port = original;
    CHECK(port.configure(tick, random));
    TimeSyncPeer peer;
    peer.authenticated = true; peer.permissions = kAuthorizationPermissionControl;
    peer.features = bbp2::FeatureTimeSync; peer.transportId = 1U;
    peer.sessionId = 3U; peer.generation = 1U; peer.source = WallTimeSource::Server;
    uint8_t request[bbp2::kTimeRequestSize] = {}, response[64] = {};
    size_t written = 0U;
    CHECK(port.request(peer, 17U, MutableByteSpan(request, sizeof(request)), written));
    bbp2::TimeResponseView reply;
    CHECK(bbp2::decodeTimeRequest(ByteView(request, written), reply.nonce));
    reply.utcAtReplyMillis = kUtc; reply.uncertaintyMillis = 1000U;
    CHECK(bbp2::encodeTimeResponse(reply, MutableByteSpan(response, sizeof(response)), written));
    bbp2::FrameView frame;
    frame.header.kind = static_cast<uint8_t>(bbp2::MessageKind::TimeResponse);
    frame.header.flags = bbp2::FlagIsResponse; frame.header.sequence = 17U;
    frame.header.bodyLength = static_cast<uint16_t>(written);
    frame.body = ByteView(response, written);
    tick.now = 1100U; CHECK(port.receive(peer, frame)); // Original admission/correlation/clock path.
    WallClockSnapshot wall;
    const uint32_t before = tick.monotonicMillis();
    const bool valid = port.readTime(wall);
    const uint32_t after = tick.monotonicMillis();
    CHECK(valid && wall.state == WallClockState::Synchronized && wall.source == WallTimeSource::Server);
    CHECK(wall.utcMillis == kUtc + 50U && wall.uncertaintyMillis == 1050U);
    CHECK(wall.sampleRevision == 1U && wall.ageMillis == 0U);
    UtcSnapshotSerial serial;
    CHECK(serial.capture(valid, wall, before, after));
    CHECK(format(serial) == "UTC1,1,1,1,1,1,1100,1100\n");
    serial.consume();
    CHECK(format(serial) == "UTC2,1,1793508000050,0,1050\n");
    serial.consume();
    tick.now += 3600001U; // Keep the original default age/uncertainty policy.
    const bool staleValid = port.readTime(wall);
    CHECK(!staleValid && wall.state == WallClockState::Stale);
    CHECK(serial.capture(staleValid, wall, tick.now, tick.now));
    CHECK(format(serial).find("UTC1,2,0,2,") == 0U);
    serial.consume(); CHECK(format(serial) == "UTC2,2,0,0,0\n");
    serial.consume(); CHECK(!serial.pending());
}
void boundedDrainAndCaptureLimit() {
    UtcSnapshotSerial serial;
    WallClockSnapshot wall;
    CHECK(serial.capture(false, wall, 1U, 2U));
    const std::string first = format(serial);
    CHECK(!serial.capture(true, wall, 9U, 9U)); // Cannot replace a pending report.
    char tiny[8] = {};
    size_t written = 99U;
    CHECK(!serial.format(tiny, sizeof(tiny), written) && written == 0U && tiny[0] == '\0');
    CHECK(!serial.format(nullptr, 64U, written) && written == 0U);
    CHECK(!serial.format(tiny, 0U, written) && written == 0U);
    CHECK(format(serial) == first);
    std::vector<std::string> output;
    CHECK(!drain(serial, first.size() - 1U, output) && output.empty() && format(serial) == first);
    CHECK(drain(serial, first.size(), output));
    CHECK(output.size() == 1U && output[0] == first && serial.pending());
    const std::string second = format(serial);
    CHECK(second == "UTC2,1,0,0,0\n" && !serial.capture(true, wall, 9U, 9U));
    CHECK(!drain(serial, second.size() - 1U, output) && output.size() == 1U && format(serial) == second);
    CHECK(drain(serial, second.size(), output) && !serial.pending());
    CHECK(serial.capture(false, wall, 3U, 4U));
    CHECK(format(serial) == "UTC1,2,0,0,0,0,3,4\n");
    serial.consume(); CHECK(format(serial) == "UTC2,2,0,0,0\n");
    serial.consume(); serial.consume();
    CHECK(!serial.pending() && !serial.capture(false, wall, 5U, 6U));
    CHECK(!serial.format(tiny, sizeof(tiny), written) && written == 0U);
}
void maximumLines() {
    UtcSnapshotSerial serial;
    WallClockSnapshot wall;
    wall.utcMillis = UINT64_MAX; wall.ageMillis = UINT32_MAX;
    wall.uncertaintyMillis = UINT32_MAX; wall.sampleRevision = UINT32_MAX;
    wall.state = static_cast<WallClockState>(UINT8_MAX);
    wall.source = static_cast<WallTimeSource>(UINT8_MAX);
    CHECK(serial.capture(true, wall, UINT32_MAX, UINT32_MAX));
    CHECK(format(serial) == "UTC1,1,1,255,255,4294967295,4294967295,4294967295\n");
    serial.consume();
    CHECK(format(serial) == "UTC2,1,18446744073709551615,4294967295,4294967295\n");
    serial.consume(); CHECK(!serial.pending());
}
} // namespace
int main() {
    unavailableIsNotUtcZero(); originalTimePortAndStale();
    boundedDrainAndCaptureLimit(); maximumLines();
    std::cout << "UTC snapshot serial contracts passed: unavailable, original accepted sample, stale, bounded drain, two captures, extreme lines; object="
              << sizeof(UtcSnapshotSerial) << " B\n";
}
