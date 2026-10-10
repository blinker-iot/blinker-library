#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "BlinkerV2/runtime/TimeSyncCoordinator.h"

using namespace blinker;
using namespace blinker::bbp2;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
static_assert(sizeof(TimeSyncCoordinator) <= 48U, "One bounded optional exchange, not per-peer slots");

namespace {
const uint64_t kUtc = 1789260000000ULL;
using Bytes = std::vector<uint8_t>;
ByteView view(const Bytes& bytes) { return ByteView(bytes.data(), bytes.size()); }
Bytes hex(const std::string& value) {
    CHECK(value.size() % 2U == 0U);
    Bytes out;
    for (size_t i = 0U; i < value.size(); i += 2U)
        out.push_back(static_cast<uint8_t>(std::stoul(value.substr(i, 2U), nullptr, 16)));
    return out;
}

void golden(const char* path) {
    std::ifstream input(path);
    CHECK(input.good());
    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t equal = line.find('=');
        CHECK(equal != std::string::npos);
        values[line.substr(0U, equal)] = line.substr(equal + 1U);
    }
    CHECK(static_cast<uint8_t>(MessageKind::TimeRequest) == std::stoul(values.at("request_kind"), nullptr, 16));
    CHECK(static_cast<uint8_t>(MessageKind::TimeResponse) == std::stoul(values.at("response_kind"), nullptr, 16));
    CHECK(FeatureTimeSync == std::stoul(values.at("feature"), nullptr, 16));
    const Bytes nonce = hex(values.at("nonce")), request = hex(values.at("request")), response = hex(values.at("response"));
    uint8_t output[40] = {};
    size_t written = 999U;
    CHECK(encodeTimeRequest(view(nonce), MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == request.size() && std::memcmp(output, request.data(), written) == 0);
    TimeResponseView sample;
    sample.nonce = view(nonce);
    sample.utcAtReplyMillis = std::stoull(values.at("utc_ms"));
    sample.uncertaintyMillis = static_cast<uint32_t>(std::stoul(values.at("uncertainty_ms")));
    CHECK(encodeTimeResponse(sample, MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == response.size() && std::memcmp(output, response.data(), written) == 0);
    for (const char* name : {"request_frame", "response_frame"}) {
        const Bytes bytes = hex(values.at(name));
        FrameView frame;
        CHECK(parseFrame(view(bytes), frame));
        CHECK(frame.header.sequence == 0x1234U);
        CHECK(encodeHeader(frame.header, MutableByteSpan(output, sizeof(output))));
        CHECK(std::memcmp(output, bytes.data(), kBaseHeaderSize) == 0);
        if (frame.header.kind == static_cast<uint8_t>(MessageKind::TimeRequest)) {
            ByteView decoded;
            CHECK(frame.header.flags == FlagNone && decodeTimeRequest(frame.body, decoded));
            CHECK(decoded.size == nonce.size() && std::memcmp(decoded.data, nonce.data(), nonce.size()) == 0);
        } else {
            TimeResponseView decoded;
            CHECK(frame.header.flags == FlagIsResponse && decodeTimeResponse(frame.body, decoded));
            CHECK(decoded.utcAtReplyMillis == sample.utcAtReplyMillis && decoded.uncertaintyMillis == sample.uncertaintyMillis);
        }
    }
    for (const auto& item : values) {
        if (item.first.find("invalid_") != 0U) continue;
        const Bytes bytes = hex(item.second);
        ByteView decodedNonce = view(nonce);
        TimeResponseView decoded = sample;
        if (item.first.find("invalid_request") == 0U) {
            CHECK(!decodeTimeRequest(view(bytes), decodedNonce));
            CHECK(decodedNonce.data == nullptr && decodedNonce.size == 0U);
        } else {
            CHECK(!decodeTimeResponse(view(bytes), decoded));
            CHECK(decoded.nonce.data == nullptr && decoded.utcAtReplyMillis == 0U && decoded.uncertaintyMillis == 0U);
        }
    }
    for (size_t i = 0U; i < response.size(); ++i) {
        TimeResponseView decoded;
        CHECK(!decodeTimeResponse(ByteView(response.data(), i), decoded));
    }
    for (size_t i = 0U; i < request.size(); ++i) {
        ByteView decoded;
        CHECK(!decodeTimeRequest(ByteView(request.data(), i), decoded));
    }
    for (uint32_t error : {0U, 23U, 24U, 255U, 256U, 60000U}) {
        for (uint64_t utc : {kWallTimeMinimumMillis, kWallTimeEndMillis - 1U}) {
            sample.utcAtReplyMillis = utc;
            sample.uncertaintyMillis = error;
            TimeResponseView decoded;
            CHECK(encodeTimeResponse(sample, MutableByteSpan(output, sizeof(output)), written));
            CHECK(written <= kTimeResponseMaxSize && decodeTimeResponse(ByteView(output, written), decoded));
            CHECK(decoded.utcAtReplyMillis == utc && decoded.uncertaintyMillis == error);
        }
    }
    CHECK(!encodeTimeResponse(sample, MutableByteSpan(output, 27U), written) && written == 0U);
    CHECK(!encodeTimeRequest(view(nonce), MutableByteSpan(output, 17U), written) && written == 0U);
    sample.utcAtReplyMillis = kWallTimeEndMillis;
    CHECK(!encodeTimeResponse(sample, MutableByteSpan(output, sizeof(output)), written) && written == 0U);
    sample.utcAtReplyMillis = kUtc; sample.uncertaintyMillis = 60001U;
    CHECK(!encodeTimeResponse(sample, MutableByteSpan(output, sizeof(output)), written) && written == 0U);
    uint8_t zero[16] = {};
    CHECK(!encodeTimeRequest(ByteView(zero, sizeof(zero)), MutableByteSpan(output, sizeof(output)), written) && written == 0U);
    ByteView decoded;
    CHECK(!decodeTimeRequest(ByteView(nullptr, 18U), decoded));
    CHECK(!decodeTimeResponse(ByteView(nullptr, 30U), sample));
}

struct Random : IRandom {
    uint8_t value = 1U;
    bool fail = false;
    Result fill(MutableByteSpan output) override {
        if (fail) return Result::failure(ErrorCode::NotConfigured);
        std::memset(output.data, value, output.size);
        return Result::success();
    }
};
TimeSyncPeer peer() {
    TimeSyncPeer result;
    result.authenticated = true; result.features = FeatureTimeSync;
    result.permissions = kAuthorizationPermissionControl;
    result.transportId = 1U; result.generation = 1U; result.source = WallTimeSource::Server;
    return result; // sessionId zero is valid when a transport exposes a single connection.
}
struct Exchange {
    WallClock clock;
    TimeSyncCoordinator sync;
    Random random;
    TimeSyncPeer target;
    uint8_t request[18] = {}, response[30] = {};
    size_t written = 0U;
    Exchange() : sync(clock), target(peer()) {}
    Result begin(uint32_t now = 100U, uint16_t seq = 17U) {
        return sync.begin(target, seq, now, random, MutableByteSpan(request, sizeof(request)), written);
    }
    FrameView reply(uint64_t utc = kUtc, uint32_t error = 1000U) {
        TimeResponseView value;
        CHECK(decodeTimeRequest(ByteView(request, sizeof(request)), value.nonce));
        value.utcAtReplyMillis = utc; value.uncertaintyMillis = error;
        size_t length = 0U;
        CHECK(encodeTimeResponse(value, MutableByteSpan(response, sizeof(response)), length));
        FrameView frame;
        frame.header.kind = static_cast<uint8_t>(MessageKind::TimeResponse);
        frame.header.flags = FlagIsResponse; frame.header.sequence = 17U;
        frame.header.bodyLength = static_cast<uint16_t>(length);
        frame.body = ByteView(response, length);
        return frame;
    }
};

void admission() {
    for (unsigned mutation = 0U; mutation < 7U; ++mutation) {
        Exchange x;
        if (mutation == 0U) x.target.authenticated = false;
        if (mutation == 1U) x.target.permissions = kAuthorizationPermissionObserve;
        if (mutation == 2U) x.target.features = 0U;
        if (mutation == 3U) x.target.generation = 0U;
        if (mutation == 4U) x.target.source = WallTimeSource::Platform;
        if (mutation == 5U) x.random.fail = true;
        if (mutation == 6U) x.random.value = 0U;
        CHECK(!x.begin() && !x.sync.pending() && x.written == 0U);
    }
    Exchange x;
    CHECK(!x.begin(100U, 0U) && !x.sync.pending());
    CHECK(!x.sync.begin(x.target, 17U, 100U, x.random, MutableByteSpan(), x.written));
    CHECK(x.begin() && x.sync.pending());
    CHECK(x.begin().code() == ErrorCode::AlreadyExists);
    x.sync.cancel();
    CHECK(x.begin().code() == ErrorCode::SequenceConflict); // RNG repeat, no new request.
    ++x.random.value; CHECK(x.begin());
}

void correlation() {
    Exchange x;
    CHECK(x.begin());
    FrameView frame = x.reply();
    for (unsigned mutation = 0U; mutation < 4U; ++mutation) {
        TimeSyncPeer wrong = x.target;
        if (mutation == 0U) ++wrong.sessionId;
        if (mutation == 1U) ++wrong.generation;
        if (mutation == 2U) ++wrong.transportId;
        if (mutation == 3U) wrong.source = WallTimeSource::Controller;
        CHECK(!x.sync.receive(wrong, frame, 150U) && x.sync.pending());
    }
    for (uint8_t flags : {uint8_t(0), uint8_t(3), uint8_t(6), uint8_t(0x82)}) {
        frame.header.flags = flags;
        CHECK(!x.sync.receive(x.target, frame, 150U) && x.sync.pending());
    }
    frame = x.reply(); ++frame.header.sequence;
    CHECK(!x.sync.receive(x.target, frame, 150U) && x.sync.pending());
    frame = x.reply(); frame.header.version = 1U;
    CHECK(!x.sync.receive(x.target, frame, 150U) && x.sync.pending());
    frame = x.reply(); frame.header.kind = static_cast<uint8_t>(MessageKind::Patch);
    CHECK(!x.sync.receive(x.target, frame, 150U) && x.sync.pending());
    frame = x.reply(); x.response[2] ^= 1U;
    CHECK(!x.sync.receive(x.target, frame, 150U) && x.sync.pending());
    frame = x.reply(); frame.body.size -= 1U;
    CHECK(!x.sync.receive(x.target, frame, 150U) && x.sync.pending());
    frame = x.reply();
    CHECK(x.sync.receive(x.target, frame, 301U) && !x.sync.pending());
    WallClockSnapshot out;
    CHECK(x.clock.read(301U, out));
    CHECK(out.utcMillis == kUtc + 100U && out.uncertaintyMillis == 1101U && out.source == WallTimeSource::Server);
    CHECK(x.sync.receive(x.target, frame, 301U).code() == ErrorCode::NotFound);
    CHECK(x.clock.read(301U, out) && out.utcMillis == kUtc + 100U);
}

void revocationAndSwitch() {
    for (unsigned mutation = 0U; mutation < 3U; ++mutation) {
        Exchange x; CHECK(x.begin());
        const FrameView frame = x.reply();
        if (mutation == 0U) x.target.authenticated = false;
        if (mutation == 1U) x.target.permissions = kAuthorizationPermissionObserve;
        if (mutation == 2U) x.target.features = 0U;
        CHECK(!x.sync.receive(x.target, frame, 150U) && !x.sync.pending());
        WallClockSnapshot out; CHECK(!x.clock.read(150U, out));
    }
    Exchange x; CHECK(x.begin());
    FrameView old = x.reply();
    const Bytes oldBody(old.body.data, old.body.data + old.body.size);
    old.body = view(oldBody);
    x.sync.cancel(); ++x.target.generation; ++x.random.value;
    x.target.source = WallTimeSource::Gateway;
    CHECK(x.begin(200U));
    CHECK(!x.sync.receive(peer(), old, 220U) && x.sync.pending());
    CHECK(!x.sync.receive(x.target, old, 220U) && x.sync.pending());
    CHECK(x.sync.receive(x.target, x.reply(kUtc - 10000U), 240U));
    x.sync.cancel(); // Disconnect does not erase UTC holdover.
    WallClockSnapshot out;
    CHECK(x.clock.read(240U, out) && out.utcMillis == kUtc - 9980U && out.source == WallTimeSource::Gateway);
}

void deadlinesAndBudget() {
    for (uint32_t elapsed : {2000U, 2001U, 0xffffffffU}) {
        Exchange x; CHECK(x.begin());
        const Result result = x.sync.receive(x.target, x.reply(), 100U + elapsed);
        CHECK(result.ok() == (elapsed == 2000U));
        CHECK(!x.sync.pending());
    }
    Exchange x; CHECK(x.begin(0xffffff9bU));
    CHECK(x.sync.receive(x.target, x.reply(), 100U)); // uint32 wrap, RTT = 201.
    WallClockSnapshot out;
    CHECK(x.clock.read(100U, out) && out.utcMillis == kUtc + 100U && out.uncertaintyMillis == 1101U);
    ++x.random.value; CHECK(x.begin(100U));
    CHECK(x.sync.receive(x.target, x.reply(kUtc, 5000U), 102U).code() == ErrorCode::ValueOutOfRange);
    CHECK(!x.sync.pending() && x.clock.read(102U, out) && out.utcMillis == kUtc + 102U);
    ++x.random.value; CHECK(x.begin(102U));
    CHECK(!x.sync.receive(x.target, x.reply(kWallTimeEndMillis - 1U), 104U));
    CHECK(!x.sync.pending() && x.clock.read(104U, out) && out.utcMillis == kUtc + 104U);
    ++x.random.value; CHECK(x.begin(104U));
    x.sync.poll(2105U); CHECK(!x.sync.pending());
}
}

int main(int argc, char** argv) {
    CHECK(argc == 2);
    golden(argv[1]); admission(); correlation(); revocationAndSwitch(); deadlinesAndBudget();
    std::cout << "TimeSync shared wire, admission, correlation, revocation, switch and deadline tests passed\n";
}
