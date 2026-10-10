#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/runtime/TimeSyncResponder.h"
#include "BlinkerV2/runtime/TimeSyncCoordinator.h"
#include "BlinkerV2/core/Authorization.h"
#include "BlinkerV2/protocol/bbp2/TimeSync.h"
#include "BlinkerV2/protocol/bbp2/Messages.h"
#include "BlinkerV2/protocol/bbp2/WireError.h"

using namespace blinker;
using namespace blinker::bbp2;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

struct Source : IWallTimeSource {
    WallClockSnapshot value;
    bool valid = true;
    unsigned reads = 0U;
    Source() {
        value.utcMillis = 1789260000123ULL;
        value.uncertaintyMillis = 1051U;
        value.state = WallClockState::Synchronized;
        value.source = WallTimeSource::Server;
    }
    bool readTime(WallClockSnapshot& out) override { ++reads; out = value; return valid; }
};
struct Fixture {
    Source source;
    TimeSyncResponder responder;
    TimeSyncPeer peer;
    uint8_t body[kTimeRequestSize] = {};
    uint8_t output[40] = {};
    uint8_t nonce[kTimeSyncNonceSize] = {};
    FrameView request;
    ByteView response;
    Fixture() : responder(source) {
        peer.authenticated = true; peer.sessionId = 7U; peer.generation = 9U;
        peer.permissions = kAuthorizationPermissionControl; peer.features = FeatureTimeSync;
        for (size_t i = 0U; i < sizeof(nonce); ++i) nonce[i] = static_cast<uint8_t>(i + 1U);
        size_t size = 0U;
        CHECK(encodeTimeRequest(ByteView(nonce, sizeof(nonce)), MutableByteSpan(body, sizeof(body)), size));
        request.header.kind = static_cast<uint8_t>(MessageKind::TimeRequest);
        request.header.sequence = 65535U; request.header.bodyLength = static_cast<uint16_t>(size);
        request.body = ByteView(body, size);
    }
    Result reply(uint32_t now = 0U, size_t capacity = 40U) {
        response = ByteView(nonce, sizeof(nonce)); // All failures must clear stale output.
        return responder.reply(peer, request, now, MutableByteSpan(output, capacity), response);
    }
    FrameView decoded() {
        FrameView frame; CHECK(parseFrame(response, frame));
        CHECK(frame.header.sequence == request.header.sequence);
        CHECK(frame.header.flags == FlagIsResponse); return frame;
    }
    void checkSample() {
        FrameView frame = decoded();
        CHECK(frame.header.kind == static_cast<uint8_t>(MessageKind::TimeResponse));
        TimeResponseView sample; CHECK(decodeTimeResponse(frame.body, sample));
        CHECK(sample.nonce.size == sizeof(nonce));
        CHECK(std::memcmp(sample.nonce.data, nonce, sizeof(nonce)) == 0);
        CHECK(sample.utcAtReplyMillis == source.value.utcMillis);
        CHECK(sample.uncertaintyMillis == source.value.uncertaintyMillis);
    }
};

void admission() {
    for (unsigned mode = 0U; mode < 12U; ++mode) {
        Fixture f;
        switch (mode) {
        case 0U: f.peer.authenticated = false; break;
        case 1U: f.peer.sessionId = 0U; break;
        case 2U: f.peer.generation = 0U; break;
        case 3U: f.peer.permissions = kAuthorizationPermissionObserve; break;
        case 4U: f.peer.features = 0U; break;
        case 5U: f.request.header.sequence = 0U; break;
        case 6U: f.request.header.flags = FlagIsResponse; break;
        case 7U: f.request.header.flags = FlagAckRequired; break;
        case 8U: f.request.header.kind = static_cast<uint8_t>(MessageKind::Error); break;
        case 9U: f.request.header.version = 1U; break;
        case 10U: f.body[0] = 0x80U; break;
        case 11U: std::memset(f.body + 2U, 0U, sizeof(f.nonce)); break;
        }
        CHECK(!f.reply()); CHECK(f.response.empty()); CHECK(f.source.reads == 0U);
    }
    Fixture f;
    CHECK(f.reply(0U, 39U).code() == ErrorCode::BufferTooSmall);
    CHECK(f.response.empty() && f.source.reads == 0U);
    CHECK(f.responder.reply(f.peer, f.request, 0U, MutableByteSpan(), f.response).code() == ErrorCode::BufferTooSmall);
    CHECK(f.reply()); f.checkSample(); // A refused frame does not consume quota.
    CHECK(f.response.size == 40U);
    f.peer.permissions = kAuthorizationPermissionObserve; // Recheck every invocation.
    CHECK(f.reply(5000U).code() == ErrorCode::AuthenticationRequired);
    CHECK(f.response.empty() && f.source.reads == 1U);
}

void noSampleAndRateLimit() {
    for (unsigned mode = 0U; mode < 5U; ++mode) {
        Fixture f;
        switch (mode) {
        case 0U: f.source.valid = false; break;
        case 1U: f.source.value.state = WallClockState::Stale; break;
        case 2U: f.source.value.state = WallClockState::Unsynchronized; break;
        case 3U: f.source.value.utcMillis = 0U; break;
        case 4U: f.source.value.uncertaintyMillis = 60001U; break;
        }
        CHECK(f.reply()); FrameView frame = f.decoded();
        CHECK(frame.header.kind == static_cast<uint8_t>(MessageKind::Error));
        ErrorBody error; CHECK(decodeErrorBody(frame.body, error));
        CHECK(error.errorCode == static_cast<uint16_t>(WireError::InternalError));
        CHECK(error.hasRelatedSequence && error.relatedSequence == 65535U);
        CHECK(f.reply(4999U).code() == ErrorCode::WouldBlock && f.response.empty());
        CHECK(f.source.reads == 1U);
        CHECK(f.reply(5000U)); CHECK(f.source.reads == 2U);
    }
    Fixture f;
    CHECK(f.reply(0xFFFFF800U)); f.checkSample();
    CHECK(f.reply(0xFFFFF800U + 4999U).code() == ErrorCode::WouldBlock);
    CHECK(f.reply(0xFFFFF800U + 5000U)); f.checkSample(); // uint32 monotonic wrap.
    // No deferred retry or queue. Owner may retire the old session and reset.
    f.responder.reset(); ++f.peer.generation; ++f.peer.sessionId;
    CHECK(f.reply(0U)); f.checkSample();
}

void aliasedOutput() {
    Fixture f;
    // The owner's existing TX scratch may also contain the parsed input body.
    std::memcpy(f.output + kBaseHeaderSize, f.body, sizeof(f.body));
    f.request.body = ByteView(f.output + kBaseHeaderSize, sizeof(f.body));
    CHECK(f.reply()); f.checkSample();
}

void sharedClockToChildRoundTrip() {
    struct ClockSource : IWallTimeSource {
        WallClock clock;
        uint32_t now = 100U;
        bool readTime(WallClockSnapshot& out) override { return clock.read(now, out); }
    } hub;
    struct Random : IRandom {
        Result fill(MutableByteSpan out) override {
            std::memset(out.data, 0x5AU, out.size); return Result::success();
        }
    } random;
    WallClockSample sample; sample.utcMillis = 1789260000000ULL;
    sample.uncertaintyMillis = 1000U; sample.source = WallTimeSource::Server;
    CHECK(hub.clock.synchronize(sample, hub.now));
    TimeSyncResponder first(hub), second(hub); // Separate quotas, ONE shared business clock.
    WallClock childClock;
    TimeSyncCoordinator child(childClock);
    Fixture f;
    f.peer.source = WallTimeSource::Gateway;
    size_t size = 0U;
    CHECK(child.begin(f.peer, f.request.header.sequence, 2000U, random,
                      MutableByteSpan(f.body, sizeof(f.body)), size));
    hub.now += 1001U;
    CHECK(first.reply(f.peer, f.request, hub.now, MutableByteSpan(f.output, sizeof(f.output)), f.response));
    CHECK(child.receive(f.peer, f.decoded(), 2101U));
    WallClockSnapshot received; CHECK(childClock.read(2101U, received));
    CHECK(received.utcMillis == sample.utcMillis + 1001U + 50U);
    CHECK(received.uncertaintyMillis == 1000U + 2U + 51U); // Hub drift + child RTT, not fresh/zero error.
    CHECK(received.source == WallTimeSource::Gateway);
    CHECK(first.reply(f.peer, f.request, hub.now, MutableByteSpan(f.output, sizeof(f.output)), f.response).code() == ErrorCode::WouldBlock);
    CHECK(second.reply(f.peer, f.request, hub.now, MutableByteSpan(f.output, sizeof(f.output)), f.response));
    hub.now += 3600000U;
    CHECK(second.reply(f.peer, f.request, hub.now, MutableByteSpan(f.output, sizeof(f.output)), f.response));
    CHECK(f.decoded().header.kind == static_cast<uint8_t>(MessageKind::Error));
    CHECK(!hub.readTime(received) && received.state == WallClockState::Stale);
}

int main() {
    admission(); noSampleAndRateLimit(); aliasedOutput(); sharedClockToChildRoundTrip();
    std::cout << "Time responder permission/feature/nonce/sequence/sample/quota/alias tests passed\n";
}
