// Use the existing real BLE/Method 2 fixture without rerunning its matrix.
#define main initialCloudKeyRuntimeFixtureMain
#include "test_initial_cloud_key_registration_main.cpp"
#undef main
#include <BlinkerV2/runtime/DeviceRuntime.h>

namespace {

class Endpoint final : public IPreCloudDirectEndpoint {
public:
    Endpoint() : calls(0U), retirements(0U), kind(0U), maximum(0U) {}
    Result receive(ByteView frame, const RxContext& rx, uint16_t limit) override {
        bbp2::FrameView view;
        check(bbp2::parseFrame(frame, view).ok() && rx.encrypted &&
            rx.transportId == 0U && rx.sessionId == 91U, "unexpected scope at endpoint");
        ++calls;
        kind = view.header.kind;
        maximum = limit;
        return Result::success();
    }
    void retire() override { ++retirements; }
    unsigned calls;
    unsigned retirements;
    uint8_t kind;
    uint16_t maximum;
};

DeviceRuntimeConfig restrictedConfig() {
    DeviceRuntimeConfig config;
    config.protocolFeatures = bbp2::FeatureAuthentication;
    return config;
}

Result unusedTransaction(void*, ByteView, bool, const RxContext&, bool&) {
    check(false, "restricted runtime dispatched an ordinary state transaction");
    return Result::failure(ErrorCode::InternalError);
}

struct RuntimeFixture {
    Fixture f;
    TransportHub hub;
    Endpoint endpoint;
    const PreCloudDirectBinding binding;
    EndpointRegistry registry;
    uint8_t tx[512U];
    DeviceRuntime runtime;
    uint8_t wire[256U];
    size_t wireSize;
    uint8_t wireIndex;
    bool wireComplete;
    uint8_t record;

    explicit RuntimeFixture(const DeviceRuntimeConfig& config = restrictedConfig())
        : f(100U, false, false), hub(), endpoint(), binding{f.direct, f.registration, endpoint},
          registry(EndpointDescriptorTable()), tx(), runtime(hub, registry, MutableByteSpan(tx, sizeof(tx)), config),
          wire(), wireSize(0U), wireIndex(0U), wireComplete(false), record(1U) {}

    void start() {
        check(runtime.bindPreCloudDirect(binding).ok() && runtime.start().ok(), "standalone owner did not start");
        check(runtime.bindPreCloudDirect(binding).code() == ErrorCode::StateConflict, "live owner replaced");
        f.link.connect(f.session.sessionId);
        f.link.observeSent = &RuntimeFixture::onPacket;
        f.link.observeSentContext = this;
        runtime.poll(1000U);
        check(wireSize == 0U, "standalone owner emitted unsolicited Hello");
    }
    static void onPacket(void* context, ByteView packet) {
        RuntimeFixture& self = *static_cast<RuntimeFixture*>(context);
        ble::FragmentView fragment;
        check(ble::parseFragment(packet, fragment).ok(), "runtime reply fragment invalid");
        check(!self.wireComplete && fragment.header.index == self.wireIndex++, "unexpected extra reply or fragment");
        check(self.wireSize + fragment.payload.size <= sizeof(self.wire), "reply capture overflow");
        std::memcpy(self.wire + self.wireSize, fragment.payload.data, fragment.payload.size);
        self.wireSize += fragment.payload.size;
        self.wireComplete = (fragment.header.flags & ble::FragmentFlagEnd) != 0U;
    }
    void inject(bbp2::MessageKind kind, ByteView body = ByteView(), bool secure = false,
        uint8_t flags = bbp2::FlagNone) {
        uint8_t frame[170U] = {}, encrypted[196U] = {};
        check(body.size + bbp2::kBaseHeaderSize <= sizeof(frame), "fixture frame exceeds storage");
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(kind);
        header.sequence = record;
        header.flags = flags;
        header.bodyLength = static_cast<uint16_t>(body.size);
        check(bbp2::encodeHeader(header, MutableByteSpan(frame, sizeof(frame))).ok(), "runtime request header failed");
        if (!body.empty()) std::memcpy(frame + bbp2::kBaseHeaderSize, body.data, body.size);
        ByteView input(frame, bbp2::kBaseHeaderSize + body.size);
        if (secure) check(f.initiator.secureSession().seal(f.crypto, input,
            MutableByteSpan(encrypted, sizeof(encrypted)), input).ok(), "client runtime request seal failed");
        wireSize = 0U;
        wireIndex = 0U;
        wireComplete = false;
        f.link.injectRecord(input, record++);
        for (unsigned i = 0U; i < 16U && f.direct.queuedFrameCount() != 0U; ++i) runtime.poll(1000U);
        check(f.direct.queuedFrameCount() == 0U, "runtime reply failed to drain");
    }
    bbp2::FrameView reply(bbp2::MessageKind kind) {
        bbp2::FrameView parsed;
        check(wireComplete && bbp2::parseFrame(ByteView(wire, wireSize), parsed).ok() &&
            parsed.header.kind == static_cast<uint8_t>(kind), "missing original Hello or Auth reply");
        return parsed;
    }
    void hello(uint16_t peerMaximum = 150U, bool secure = false, uint8_t flags = bbp2::FlagNone) {
        uint8_t bytes[128U] = {};
        bbp2::HelloBody hello;
        hello.versions[0] = bbp2::kVersion;
        hello.versionCount = 1U;
        hello.features = bbp2::FeatureAuthentication;
        hello.maxFrameSize = peerMaximum;
        hello.maxReassemblySize = peerMaximum;
        hello.authorizationMethodCount = 1U;
        hello.authorizationMethods[0] = security::kControllerHmacSha256Method;
        ByteView body;
        check(bbp2::encodeHelloBody(hello, MutableByteSpan(bytes, sizeof(bytes)), body).ok(), "fixture Hello invalid");
        inject(bbp2::MessageKind::Hello, body, secure, flags);
        if (flags != bbp2::FlagNone || secure) return;
        check(bbp2::decodeHelloBody(reply(bbp2::MessageKind::Hello).body, hello).ok() &&
            hello.features == bbp2::FeatureAuthentication && hello.authorizationMethodCount == 1U,
            "restricted Hello advertises ordinary capabilities");
    }
    bbp2::AuthResultBody auth(ByteView payload) {
        uint8_t bytes[128U] = {};
        bbp2::AuthRequestBody request;
        request.method = security::kControllerHmacSha256Method;
        request.payload = payload;
        ByteView body;
        check(bbp2::encodeAuthRequestBody(request, MutableByteSpan(bytes, sizeof(bytes)), body).ok(), "fixture auth invalid");
        inject(bbp2::MessageKind::Authenticate, body);
        bbp2::AuthResultBody response;
        check(bbp2::decodeAuthResultBody(reply(bbp2::MessageKind::AuthResult).body, response).ok(), "runtime auth failed");
        return response;
    }
    void authenticate() {
        uint8_t init[security::kControllerAuthInitPayloadSize] = {};
        uint8_t proof[security::kControllerAuthProofPayloadSize] = {};
        ByteView payload;
        check(f.initiator.begin(f.used, MutableByteSpan(init, sizeof(init)), payload).ok(), "fixture initiator failed");
        check(f.initiator.handleChallenge(auth(payload), MutableByteSpan(proof, sizeof(proof)), payload).ok(),
            "original runtime challenge was not valid");
        check(f.initiator.handleAuthorization(auth(payload)).ok() &&
            constantTimeEqual(f.direct.committedSessionBinding(f.session.sessionId), f.initiator.secureSession().sessionBinding()),
            "original queued AuthResult did not commit DirectSecure");
    }
};

void testExactGraph() {
    RuntimeFixture mixedContext;
    mixedContext.runtime.setStateApplyTransactionHandler(&unusedTransaction, &mixedContext);
    check(mixedContext.runtime.bindPreCloudDirect(mixedContext.binding).code() == ErrorCode::StateConflict &&
        mixedContext.hub.size() == 0U, "binding displaced an existing ordinary transaction context");
    mixedContext.runtime.setStateApplyTransactionHandler(nullptr, nullptr);
    mixedContext.start();
    mixedContext.runtime.setStateApplyTransactionHandler(&unusedTransaction, &mixedContext);
    mixedContext.hello();
    mixedContext.runtime.stop();
    check(mixedContext.endpoint.retirements == 1U, "ordinary setter displaced the active dedicated binding");
    RuntimeFixture regularConfig(DeviceRuntimeConfig{});
    check(regularConfig.runtime.bindPreCloudDirect(regularConfig.binding).ok() &&
        !regularConfig.runtime.start(), "normal feature configuration was admitted");
    RuntimeFixture replacedBinding;
    check(replacedBinding.runtime.bindPreCloudDirect(replacedBinding.binding).ok() &&
        replacedBinding.hub.removeTransport(replacedBinding.f.direct).ok() &&
        replacedBinding.hub.addTransport(replacedBinding.f.direct, TransportLifecyclePolicy::External,
            TransportAccess(&replacedBinding.f.registration)).ok() && !replacedBinding.runtime.start(),
        "external graph substitution was admitted");
}

void testOriginalAuthAndExclusiveDispatch() {
    RuntimeFixture live;
    live.start();
    live.inject(bbp2::MessageKind::InitialCloudKeyInstall);
    check(live.endpoint.calls == 0U && live.wireSize == 0U, "pre-Hello install escaped guard");
    const uint32_t rejected = live.runtime.rejectedFrameCount();
    live.hello(150U, false, bbp2::FlagIsResponse);
    check(live.wireSize == 0U && live.runtime.rejectedFrameCount() == rejected + 1U,
        "unsolicited Hello response established negotiation");
    live.hello();
    live.inject(bbp2::MessageKind::InitialCloudKeyHandoff);
    check(live.endpoint.calls == 0U && live.wireSize == 0U, "pre-Auth handoff escaped guard");
    live.authenticate();
    live.hello(150U, true);
    check(live.wireComplete, "authenticated pre-purpose packet negotiation was disabled");
    uint8_t padded[141U] = {};
    live.inject(bbp2::MessageKind::InitialCloudKeyInstall, ByteView(padded, sizeof(padded)), true);
    check(live.endpoint.calls == 0U && live.wireSize == 0U, "over-peer-budget frame escaped guard");
    const bbp2::MessageKind forbidden[] = {bbp2::MessageKind::Command, bbp2::MessageKind::StateRequest,
        bbp2::MessageKind::ManifestRequest, bbp2::MessageKind::InitialCloudKeyInstalled,
        bbp2::MessageKind::ControllerMutation};
    for (bbp2::MessageKind kind : forbidden) {
        live.inject(kind, ByteView(), true);
        check(live.endpoint.calls == 0U && live.wireSize == 0U, "ordinary request escaped dedicated dispatch");
    }
    live.inject(bbp2::MessageKind::InitialCloudKeyInstall, ByteView(), true, bbp2::FlagAckRequired);
    check(live.endpoint.calls == 0U && live.wireSize == 0U, "generic reliable flag escaped dedicated dispatch");
    live.inject(bbp2::MessageKind::InitialCloudKeyInstall, ByteView(), true);
    check(live.endpoint.calls == 1U && live.endpoint.maximum == 150U && live.wireSize == 0U &&
        live.endpoint.kind == static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyInstall),
        "authenticated install was not routed with exact negotiated budget");
    live.hello(200U, true);
    check(live.wireSize == 0U, "negotiated frame agreement reopened after purpose request");
    live.inject(bbp2::MessageKind::InitialCloudKeyHandoff, ByteView(), true);
    check(live.endpoint.calls == 2U && live.wireSize == 0U &&
        live.endpoint.kind == static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyHandoff),
        "authenticated handoff was not routed without generic ACK");
    live.f.link.disconnect();
    check(live.endpoint.retirements == 0U, "BLE peer close was mistaken for explicit owner retirement");
    live.f.link.connect(92U);
    live.inject(bbp2::MessageKind::InitialCloudKeyInstall);
    check(live.endpoint.calls == 2U && live.wireSize == 0U, "new peer inherited retired Runtime authorization");
    live.runtime.stop();
    live.runtime.stop();
    check(live.endpoint.retirements == 1U && live.runtime.start().code() == ErrorCode::StateConflict,
        "owner did not retire exactly once or was restartable after cancellation");
}

void testInstalledNetworkDispatch() {
    RuntimeFixture plaintext;
    plaintext.start();
    plaintext.hello();
    plaintext.authenticate();
    plaintext.inject(bbp2::MessageKind::InitialCloudNetworkHandoff);
    check(plaintext.endpoint.calls == 0U &&
        plaintext.f.direct.committedSessionBinding(plaintext.f.session.sessionId).empty(),
        "plaintext network purpose did not retire the secure session");

    RuntimeFixture live;
    live.start();
    live.inject(bbp2::MessageKind::InitialCloudNetworkHandoff);
    check(live.endpoint.calls == 0U, "network purpose bypassed Hello");
    live.hello();
    live.inject(bbp2::MessageKind::InitialCloudNetworkHandoff);
    check(live.endpoint.calls == 0U, "network purpose bypassed Method 2");
    live.authenticate();
    live.inject(bbp2::MessageKind::InitialCloudNetworkHandoff, ByteView(), true, bbp2::FlagAckRequired);
    live.inject(bbp2::MessageKind::InitialCloudNetworkAccepted, ByteView(), true);
    check(live.endpoint.calls == 0U, "network purpose bypassed encrypted request gate");
    live.inject(bbp2::MessageKind::InitialCloudNetworkHandoff, ByteView(), true);
    check(live.endpoint.calls == 1U && live.endpoint.maximum == 150U &&
        live.endpoint.kind == static_cast<uint8_t>(bbp2::MessageKind::InitialCloudNetworkHandoff),
        "network purpose was not routed to the explicit owner");
}

} // namespace

int main() {
    testExactGraph();
    testOriginalAuthAndExclusiveDispatch();
    testInstalledNetworkDispatch();
    return 0;
}
