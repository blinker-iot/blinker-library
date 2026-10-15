#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "BlinkerV2/runtime/DeviceRuntime.h"
#include "BlinkerV2/runtime/TimeSyncService.h"
#include "BlinkerV2/core/Authorization.h"
#include "BlinkerV2/api/BlinkerFacade.h"

using namespace blinker;
using namespace blinker::bbp2;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
using Bytes = std::vector<uint8_t>;
struct Tick : IClock {
    uint32_t now = 100U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& seconds) const override { seconds = 0U; return false; }
};
struct Random : IRandom {
    uint8_t next = 1U;
    bool fail = false;
    ErrorCode failure = ErrorCode::NotConfigured;
    Result fill(MutableByteSpan out) override {
        if (fail) return Result::failure(failure);
        std::memset(out.data, next++, out.size); return Result::success();
    }
};
struct Link : IFrameTransport {
    uint32_t permissions = kAuthorizationPermissionObserve | kAuthorizationPermissionControl;
    uint32_t sessionPermissions(uint32_t) const override { return permissions; }
    FrameReceiver receiver = nullptr; void* context = nullptr;
    FrameSessionHandler disconnected = nullptr; void* sessionContext = nullptr;
    TransportState current = TransportState::Stopped;
    uint16_t flags = TransportFeatureBinary | TransportFeatureCloud | TransportFeatureAuthenticated;
    bool fail = false;
    ErrorCode failure = ErrorCode::WouldBlock;
    Bytes rejected;
    std::vector<Bytes> sent;
    Result start() override { current = TransportState::Online; return Result::success(); }
    void stop() override { current = TransportState::Stopped; }
    void poll(uint32_t) override {}
    TransportState state() const override { return current; }
    TransportCapabilities capabilities() const override {
        TransportCapabilities c; c.maxFrameSize = 512U; c.features = flags; return c;
    }
    Result send(ByteView b, const SendTarget&) override {
        if (fail) { rejected.assign(b.data, b.data + b.size); return Result::failure(failure); }
        sent.emplace_back(b.data, b.data + b.size); return Result::success();
    }
    void setReceiver(FrameReceiver r, void* c) override { receiver = r; context = c; }
    void setSessionHandlers(FrameSessionHandler, FrameSessionHandler d, void* c) override {
        disconnected = d; sessionContext = c;
    }
    void inject(const Bytes& bytes, bool authenticated = true, uint32_t session = 1U, bool encrypted = false) {
        RxContext rx; rx.authenticated = authenticated; rx.sessionId = session; rx.encrypted = encrypted;
        receiver(context, ByteView(bytes.data(), bytes.size()), rx);
    }
    size_t count(MessageKind kind) const {
        size_t n = 0U; for (const auto& b : sent) if (b[3] == static_cast<uint8_t>(kind)) ++n; return n;
    }
    void close() {
        RxContext rx; rx.sessionId = 1U;
        if (disconnected != nullptr) disconnected(sessionContext, rx);
        current = TransportState::Backoff;
    }
};
Bytes frame(MessageKind kind, uint16_t sequence, ByteView body, uint8_t flags = FlagIsResponse) {
    FrameHeader h; h.kind = static_cast<uint8_t>(kind); h.flags = flags; h.sequence = sequence;
    h.bodyLength = static_cast<uint16_t>(body.size);
    Bytes result(kBaseHeaderSize + body.size);
    CHECK(encodeHeader(h, MutableByteSpan(result.data(), result.size())));
    std::memcpy(result.data() + kBaseHeaderSize, body.data, body.size); return result;
}
struct Fixture {
    Tick tick; Random random; TimeSyncService time; Link link, cloud; TransportHub hub;
    EndpointDescriptor descriptor;
    EndpointRegistry endpoints;
    uint8_t tx[512] = {};
    DeviceRuntime runtime;
    Fixture(bool enabled = true, bool local = false, bool dual = false,
            IAuthorizationProvider* authorization = nullptr) : endpoints(&descriptor, 1U),
        runtime(hub, endpoints, MutableByteSpan(tx, sizeof(tx))) {
        descriptor.key = StringView("power");
        descriptor.type = ValueType::Boolean;
        descriptor.kind = EndpointKind::Property;
        descriptor.access = AccessRead | AccessWrite | AccessNotify;
        if (local) link.flags = TransportFeatureBinary | TransportFeatureLocal | TransportFeatureAuthenticated;
        if (authorization != nullptr) link.flags &= ~TransportFeatureAuthenticated;
        CHECK(hub.addTransport(link, TransportLifecyclePolicy::Managed, TransportAccess(authorization)));
        if (dual) CHECK(hub.addTransport(cloud));
        CHECK(time.configure(tick, random));
        if (enabled) CHECK(runtime.setTimeSync(&time));
        CHECK(runtime.start()); runtime.poll(1000U);
    }
    ~Fixture() { runtime.stop(); }
    void hello(bool feature = true) {
        helloOn(link, feature);
    }
    void helloOn(Link& source, bool feature = true, uint16_t limit = 512U) {
        const Bytes original = source.sent.front();
        FrameView f; CHECK(parseFrame(ByteView(original.data(), original.size()), f));
        HelloBody h; CHECK(decodeHelloBody(f.body, h));
        h.role = PeerRole::Server;
        h.maxFrameSize = limit;
        if (feature) h.features |= FeatureTimeSync; else h.features &= ~uint32_t(FeatureTimeSync);
        uint8_t body[256] = {}; ByteView encoded;
        CHECK(encodeHelloBody(h, MutableByteSpan(body, sizeof(body)), encoded));
        source.inject(frame(MessageKind::Hello, f.header.sequence, encoded),
            (source.flags & TransportFeatureAuthenticated) != 0U);
    }
    Bytes reply() const { return reply(link); }
    Bytes reply(const Link& source, uint64_t utcMillis = 1789260000000ULL) const {
        const Bytes& request = source.sent.back();
        CHECK(request[3] == static_cast<uint8_t>(MessageKind::TimeRequest));
        FrameView f; CHECK(parseFrame(ByteView(request.data(), request.size()), f));
        TimeResponseView sample; CHECK(decodeTimeRequest(f.body, sample.nonce));
        sample.utcAtReplyMillis = utcMillis; sample.uncertaintyMillis = 1000U;
        uint8_t body[30]; size_t size = 0U;
        CHECK(encodeTimeResponse(sample, MutableByteSpan(body, sizeof(body)), size));
        return frame(MessageKind::TimeResponse, f.header.sequence, ByteView(body, size));
    }
};

void optionalAndNegotiation() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        Fixture f(mode != 0U, mode == 1U);
        FrameView frameView;
        CHECK(parseFrame(ByteView(f.link.sent[0].data(), f.link.sent[0].size()), frameView));
        HelloBody hello; CHECK(decodeHelloBody(frameView.body, hello));
        CHECK(((hello.features & FeatureTimeSync) != 0U) == (mode != 0U));
        f.hello(false); f.runtime.poll(1000U);
        CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    }
    for (const bool local : {false, true}) {
        Fixture f(true, local);
        f.helloOn(f.link, true, 39U); f.runtime.poll(1000U);
        CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    }
}

void inBandAuthorization() {
    // Runtime admission seam only: cryptographic Method 2 is covered by its own tests.
    struct Authorizer : IAuthorizationProvider {
        uint32_t permissions = kAuthorizationPermissionObserve;
        size_t calls = 0U;
        size_t methodCount() const override { return 1U; }
        uint16_t methodAt(size_t) const override { return 2U; }
        bool establishesSecureTransport() const override { return true; }
        Result authorize(uint16_t method, ByteView, const AuthorizationSessionContext& ctx,
                         AuthorizationDecision& decision) override {
            CHECK(method == 2U && !ctx.transportAuthenticated); ++calls;
            decision.status = AuthStatus::Authorized; decision.permissions = permissions;
            return Result::success();
        }
        void sessionClosed(const AuthorizationSessionContext&) override {}
        void resetSessions() override {}
    };
    for (const bool control : {false, true}) {
        Authorizer auth;
        if (control) auth.permissions |= kAuthorizationPermissionControl;
        Fixture f(true, true, false, &auth);
        f.hello(); f.runtime.poll(1000U);
        CHECK(auth.calls == 0U && f.link.count(MessageKind::TimeRequest) == 0U);
        AuthRequestBody request; request.method = 2U;
        const uint8_t proof = 1U; request.payload = ByteView(&proof, 1U);
        uint8_t body[32]; ByteView encoded;
        CHECK(encodeAuthRequestBody(request, MutableByteSpan(body, sizeof(body)), encoded));
        f.link.inject(frame(MessageKind::Authenticate, 8U, encoded, FlagNone), false, 1U, true);
        f.runtime.poll(1000U);
        CHECK(auth.calls == 1U);
        CHECK(f.link.count(MessageKind::TimeRequest) == (control ? 1U : 0U));
        uint64_t utc = 12U;
        if (control) {
            const Bytes response = f.reply();
            f.link.inject(response, false, 1U, true);
            CHECK(f.time.time(utc) && utc == 1789260000ULL);
            f.tick.now += 300000U; f.runtime.poll(1000U);
            const Bytes stale = f.reply();
            f.link.close(); f.link.current = TransportState::Online; f.hello();
            f.link.inject(stale, false, 1U, true); // Reusing the session ID cannot restore permissions.
            CHECK(f.time.time(utc) && utc == 1789260300ULL);
        } else CHECK(!f.time.time(utc) && utc == 0U);
    }
}
void syncRefreshAndRestart() {
    Fixture f; uint64_t utc = 88U;
    CHECK(!f.time.time(utc) && utc == 0U);
    f.hello(); f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    const Bytes response = f.reply();
    f.tick.now += 201U; f.link.inject(response);
    CHECK(f.time.time(utc) && utc == 1789260000ULL);
    CHECK(!f.tick.unixTime(utc)); // Business UTC never takes over IClock.
    f.link.inject(response); CHECK(f.time.time(utc) && utc == 1789260000ULL);
    f.tick.now += 299999U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    ++f.tick.now; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 2U);
    f.link.close(); f.runtime.poll(1000U);
    CHECK(f.time.time(utc));
    f.tick.now += 3600000U; f.runtime.poll(1000U);
    CHECK(!f.time.time(utc) && utc == 0U);
    f.runtime.stop(); CHECK(!f.time.time(utc));
    f.link.sent.clear(); CHECK(f.runtime.start()); f.runtime.poll(1000U);
    f.hello(); f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    f.link.inject(response); CHECK(!f.time.time(utc)); // Previous boot/request nonce.
    f.link.inject(f.reply()); CHECK(f.time.time(utc));
}
void localQueuePressureDoesNotConsumeRemoteRetryWindow() {
    Fixture f(true, true); f.hello(); f.link.fail = true;
    f.runtime.poll(1000U); // ACK/bootstrap currently owns the bounded TX slot.
    Link neverSent; neverSent.sent.push_back(f.link.rejected);
    const Bytes stale = f.reply(neverSent);
    const uint8_t afterBlocked = f.random.next;
    for (unsigned i = 0U; i < 100U; ++i) f.runtime.poll(1000U);
    CHECK(f.random.next == afterBlocked); // No per-loop RNG / hot retry.
    f.link.fail = false;
    f.tick.now += 999U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    ++f.tick.now; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    CHECK(f.random.next == static_cast<uint8_t>(afterBlocked + 1U));
    uint64_t utc = 0U; f.link.inject(stale); CHECK(!f.time.time(utc));
    f.link.inject(f.reply());
    CHECK(f.time.time(utc));
    f.time.sendRejected(ErrorCode::WouldBlock); // No prepared request: do not shorten refresh.
    f.tick.now += 1000U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
}
void onlyLocalBackpressureGetsShortRetry() {
    for (const ErrorCode code : {ErrorCode::NotConnected, ErrorCode::InvalidEncoding}) {
        Fixture f; f.hello(); f.link.fail = true; f.link.failure = code; f.runtime.poll(1000U);
        f.link.fail = false; f.tick.now += 1000U; f.runtime.poll(1000U);
        CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
        f.tick.now += 29000U; f.runtime.poll(1000U);
        CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    }
    Fixture rng; rng.hello(); rng.random.fail = true; rng.random.failure = ErrorCode::WouldBlock;
    rng.runtime.poll(1000U); rng.random.fail = false;
    rng.tick.now += 1000U; rng.runtime.poll(1000U);
    CHECK(rng.link.count(MessageKind::TimeRequest) == 0U);
    rng.tick.now += 29000U; rng.runtime.poll(1000U);
    CHECK(rng.link.count(MessageKind::TimeRequest) == 1U);
    Fixture blocked(true, true); blocked.hello(); blocked.link.fail = true;
    blocked.runtime.poll(1000U);
    for (unsigned i = 0U; i < 3U; ++i) {
        blocked.tick.now += 1000U; blocked.runtime.poll(1000U);
        const uint8_t attempts = blocked.random.next;
        for (unsigned spin = 0U; spin < 100U; ++spin) blocked.runtime.poll(1000U);
        CHECK(blocked.random.next == attempts);
    }
    blocked.link.fail = false; blocked.link.permissions = kAuthorizationPermissionObserve;
    blocked.tick.now += 1000U; blocked.runtime.poll(1000U);
    CHECK(blocked.link.count(MessageKind::TimeRequest) == 0U);
    Fixture wrap; wrap.tick.now = UINT32_MAX - 499U; wrap.hello(); wrap.link.fail = true;
    wrap.runtime.poll(1000U); wrap.link.fail = false;
    wrap.link.close(); wrap.runtime.poll(1000U); wrap.link.current = TransportState::Online; wrap.hello();
    wrap.tick.now += 999U; wrap.runtime.poll(1000U);
    CHECK(wrap.link.count(MessageKind::TimeRequest) == 0U); // Reconnect cannot refund local backoff.
    ++wrap.tick.now; wrap.runtime.poll(1000U);
    CHECK(wrap.link.count(MessageKind::TimeRequest) == 1U);
}
void retryErrorAndRevocation() {
    Fixture f; f.hello(); f.link.fail = true; f.runtime.poll(1000U);
    f.link.fail = false; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    f.tick.now += 30000U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    const Bytes first = f.reply();
    f.link.inject(first, false); // Current transport authority disappeared.
    uint64_t utc = 0U; CHECK(!f.time.time(utc));
    f.hello(); f.tick.now += 30000U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 2U);
    FrameView request; const Bytes& bytes = f.link.sent.back();
    CHECK(parseFrame(ByteView(bytes.data(), bytes.size()), request));
    ErrorBody error; error.errorCode = 8U; error.relatedSequence = request.header.sequence; error.hasRelatedSequence = true;
    uint8_t body[30]; ByteView encoded; CHECK(encodeErrorBody(error, MutableByteSpan(body, sizeof(body)), encoded));
    const Bytes valid = f.reply();
    f.link.inject(frame(MessageKind::Error, 400U, encoded));
    f.link.inject(valid); CHECK(!f.time.time(utc));
    f.tick.now += 29999U; f.runtime.poll(1000U); CHECK(f.link.count(MessageKind::TimeRequest) == 2U);
    ++f.tick.now; f.runtime.poll(1000U); CHECK(f.link.count(MessageKind::TimeRequest) == 3U);
    f.tick.now += 2001U;
    const size_t sentBeforeLateReply = f.link.sent.size();
    f.link.inject(f.reply()); CHECK(!f.time.time(utc));
    CHECK(f.link.sent.size() == sentBeforeLateReply); // No Error-on-response loop or unrelated App pending failure.
    f.link.close(); f.runtime.poll(1000U);
    f.link.current = TransportState::Online;
    f.runtime.poll(1000U); f.hello();
    f.runtime.poll(1000U); CHECK(f.link.count(MessageKind::TimeRequest) == 3U); // Reconnect cannot bypass backoff.
}
void optionalApiAndClockBinding() {
    BlinkerClass facade;
    PropertySpec<bool> optionSpec("optional", readWrite()); Property<bool> option(optionSpec);
    CHECK(!facade.enableTasks(option) && facade.lastError() == ErrorCode::UnsupportedFeature);
    uint64_t utc = 123U;
    CHECK(!facade.time(utc) && utc == 0U);
    WallClockSnapshot status;
    status.utcMillis = 123U;
    status.source = WallTimeSource::Server;
    status.state = WallClockState::Synchronized;
    CHECK(!facade.timeStatus(status));
    CHECK(status.utcMillis == 0U && status.source == WallTimeSource::None &&
          status.state == WallClockState::Unsynchronized);
    CHECK(facade.enableTime());
    utc = 456U;
    CHECK(!facade.time(utc) && utc == 0U); // Enabled is not synchronized.
    status.utcMillis = 456U;
    CHECK(!facade.timeStatus(status) && status.utcMillis == 0U &&
          status.state == WallClockState::Unsynchronized);
    TimeSyncService service;
    Tick tick, otherTick;
    Random random, otherRandom;
    CHECK(service.configure(tick, random));
    CHECK(service.configure(tick, random));
    CHECK(service.configure(otherTick, random).code() == ErrorCode::AlreadyExists);
    CHECK(service.configure(tick, otherRandom).code() == ErrorCode::AlreadyExists);
}
void localControlAndRetirement() {
    Fixture f(true, true);
    f.link.permissions = kAuthorizationPermissionObserve;
    f.hello(); f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    f.link.permissions |= kAuthorizationPermissionControl;
    f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    const Bytes old = f.reply();
    f.time.sessionClosed(1U, 1U); // Another transport must not cancel it.
    f.tick.now += 100U; f.link.inject(old);
    uint64_t utc = 0U; CHECK(f.time.time(utc));
    f.tick.now += 300000U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 2U);
    const Bytes retired = f.reply();
    f.link.permissions = kAuthorizationPermissionObserve;
    f.runtime.poll(1000U); // Revoke while pending, without waiting for a response.
    f.link.permissions |= kAuthorizationPermissionControl;
    f.link.inject(retired); CHECK(f.time.time(utc) && utc == 1789260300ULL);
    f.tick.now += 30000U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 3U);
    const Bytes disconnected = f.reply();
    f.link.close(); f.link.current = TransportState::Online; f.hello();
    f.link.inject(disconnected); CHECK(f.time.time(utc) && utc == 1789260330ULL);
}

void cloudPreferenceAndSourceChange() {
    Fixture f(true, true, true);
    f.hello(); f.runtime.poll(1000U); // Local is currently the only usable source.
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    const Bytes localReply = f.reply();
    f.helloOn(f.cloud);
    f.link.inject(localReply); // Cloud now eligible: old local reply cannot win a race.
    uint64_t utc = 42U; CHECK(!f.time.time(utc) && utc == 0U);
    f.tick.now += 30000U; f.runtime.poll(1000U);
    CHECK(f.cloud.count(MessageKind::TimeRequest) == 1U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    // Unrelated local retirement cannot cancel this cloud exchange.
    f.link.close();
    f.cloud.inject(f.reply(f.cloud));
    CHECK(f.time.time(utc) && utc == 1789260000ULL);
}

void cloudToLocalKeepsRefreshSchedule() {
    Fixture f(true, true, true);
    f.helloOn(f.cloud); f.hello(); f.runtime.poll(1000U);
    CHECK(f.cloud.count(MessageKind::TimeRequest) == 1U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    f.tick.now += 100U; f.cloud.inject(f.reply(f.cloud));
    uint64_t utc = 0U; CHECK(f.time.time(utc));
    f.tick.now += 290000U;
    f.cloud.close(); f.runtime.poll(1000U);
    CHECK(f.time.time(utc) && utc == 1789260290ULL);
    CHECK(f.link.count(MessageKind::TimeRequest) == 0U); // Holdover, not an eager retry.
    f.tick.now += 9999U; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 0U);
    ++f.tick.now; f.runtime.poll(1000U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
    f.link.inject(f.reply(f.link, 1789260300000ULL));
    CHECK(f.time.time(utc) && utc == 1789260300ULL);
    // Rejoining the preferred cloud source must not create a second clock or
    // bypass the interval established by the accepted local sample.
    f.cloud.sent.clear(); f.cloud.current = TransportState::Online;
    f.runtime.poll(1000U); f.helloOn(f.cloud); f.runtime.poll(1000U);
    CHECK(f.cloud.count(MessageKind::TimeRequest) == 0U);
    f.tick.now += 299999U; f.runtime.poll(1000U);
    CHECK(f.cloud.count(MessageKind::TimeRequest) == 0U);
    ++f.tick.now; f.runtime.poll(1000U);
    CHECK(f.cloud.count(MessageKind::TimeRequest) == 1U);
    CHECK(f.link.count(MessageKind::TimeRequest) == 1U);
}

void sharedReadOnlySnapshot() {
    TimeSyncService unbound;
    WallClockSnapshot snapshot; snapshot.utcMillis = 999U;
    CHECK(!unbound.readTime(snapshot) && snapshot.utcMillis == 0U);
    Fixture f;
    IWallTimeSource& shared = f.time; // Borrow the same clock; no security IClock view.
    CHECK(!shared.readTime(snapshot) && snapshot.utcMillis == 0U);
    f.hello(); f.runtime.poll(1000U);
    f.tick.now += 101U; f.link.inject(f.reply());
    CHECK(shared.readTime(snapshot));
    CHECK(snapshot.utcMillis == 1789260000050ULL);
    CHECK(snapshot.uncertaintyMillis == 1051U && snapshot.ageMillis == 0U);
    CHECK(snapshot.source == WallTimeSource::Server);
    f.tick.now += 1001U;
    CHECK(shared.readTime(snapshot));
    CHECK(snapshot.utcMillis == 1789260001051ULL);
    CHECK(snapshot.uncertaintyMillis == 1053U && snapshot.ageMillis == 1001U);
    uint64_t seconds = 0U; CHECK(f.time.time(seconds) && seconds == 1789260001ULL);
    uint64_t security = 123U; CHECK(!f.tick.unixTime(security) && security == 0U);
    f.time.reset(false);
    CHECK(shared.readTime(snapshot) && snapshot.ageMillis == 1001U);
    f.tick.now += 3600000U;
    CHECK(!shared.readTime(snapshot) && snapshot.state == WallClockState::Stale);
    CHECK(snapshot.utcMillis == 0U && snapshot.uncertaintyMillis == 0U);
    // Expiry is latched by read, even without a Runtime poll or a new request.
    f.tick.now = 201U;
    CHECK(!shared.readTime(snapshot) && snapshot.state == WallClockState::Stale);
    f.time.reset(true);
    CHECK(!shared.readTime(snapshot) && snapshot.state == WallClockState::Unsynchronized);
}

int main() {
    localQueuePressureDoesNotConsumeRemoteRetryWindow();
    onlyLocalBackpressureGetsShortRetry();
    inBandAuthorization(); localControlAndRetirement(); cloudPreferenceAndSourceChange();
    cloudToLocalKeepsRefreshSchedule();
    sharedReadOnlySnapshot();
    optionalApiAndClockBinding(); optionalAndNegotiation(); syncRefreshAndRestart(); retryErrorAndRevocation();
    std::cout << "TimeSync real Runtime negotiation/refresh/retry/authority/restart tests passed\n";
}
