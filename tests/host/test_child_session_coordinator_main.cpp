#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/runtime/ChildSessionCoordinator.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Child session check failed at line " << __LINE__    \
                      << ": " << #condition << std::endl;                      \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class SequenceRandom final : public blinker::IRandom {
public:
    explicit SequenceRandom(uint8_t first)
        : next(first), calls(0U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        if (output.data == nullptr) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = next++;
        }
        return blinker::Result::success();
    }

    uint8_t next;
    uint32_t calls;
};

class CredentialSource final : public blinker::IControllerCredentialSource {
public:
    CredentialSource() : credential() {}

    blinker::Result loadActive(
        blinker::ByteView controllerId,
        blinker::ControllerCredentialDomain domain,
        uint32_t generation,
        blinker::ControllerCredential& output) override {
        if (controllerId.size != blinker::kControllerIdSize ||
            std::memcmp(
                controllerId.data,
                credential.controllerId,
                blinker::kControllerIdSize) != 0 ||
            domain != credential.domain ||
            generation != credential.ownershipGeneration) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        output = credential;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

    blinker::ControllerCredential credential;
};

class ResponderDirectSessions final
    : public blinker::security::IDirectSecureSessionController {
public:
    ResponderDirectSessions()
        : session(), pendingSessionId(0U), activeSessionId(0U) {}

    blinker::Result prepare(
        uint32_t sessionId,
        blinker::ByteView secret,
        const blinker::security::DirectSecureContext& context) override {
        session.clear();
        blinker::Result result = session.begin(
            blinker::security::DirectSecureRole::Responder,
            secret,
            context);
        if (result) pendingSessionId = sessionId;
        return result;
    }

    void commit(uint32_t sessionId) override {
        if (pendingSessionId == sessionId && session.ready()) {
            activeSessionId = sessionId;
            pendingSessionId = 0U;
        }
    }

    void cancel(uint32_t sessionId) override {
        if (pendingSessionId == sessionId || activeSessionId == sessionId) {
            session.clear();
            pendingSessionId = 0U;
            activeSessionId = 0U;
        }
    }

    blinker::security::DirectSecureSession session;
    uint32_t pendingSessionId;
    uint32_t activeSessionId;
};

blinker::ControllerCredential makeCredential() {
    blinker::ControllerCredential credential;
    credential.domain =
        blinker::ControllerCredentialDomain::PlatformAccess;
    credential.ownershipGeneration = 23U;
    credential.credentialVersion = 7U;
    credential.permissions =
        blinker::kControllerPermissionObserve |
        blinker::kControllerPermissionControl;
    for (size_t index = 0U; index < blinker::kControllerIdSize; ++index) {
        credential.controllerId[index] = static_cast<uint8_t>(0x10U + index);
    }
    for (size_t index = 0U;
         index < blinker::kControllerCredentialSecretSize;
         ++index) {
        credential.secret[index] = static_cast<uint8_t>(0xA0U + index);
    }
    return credential;
}

blinker::Result encodeFrame(
    blinker::bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence,
    blinker::ByteView body,
    blinker::MutableByteSpan output,
    blinker::ByteView& encoded) {
    encoded = blinker::ByteView();
    if (output.data == nullptr ||
        output.size < blinker::bbp2::kBaseHeaderSize + body.size ||
        body.size > UINT16_MAX) {
        return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
    }
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = flags;
    header.sequence = sequence;
    header.bodyLength = static_cast<uint16_t>(body.size);
    blinker::Result result = blinker::bbp2::encodeHeader(header, output);
    if (result && !body.empty()) {
        std::memcpy(
            output.data + blinker::bbp2::kBaseHeaderSize,
            body.data,
            body.size);
    }
    if (result) {
        encoded = blinker::ByteView(
            output.data,
            blinker::bbp2::kBaseHeaderSize + body.size);
    }
    return result;
}

blinker::Result makeDeviceHello(
    uint16_t sequence,
    bool includeManifest,
    blinker::MutableByteSpan output,
    blinker::ByteView& encoded) {
    uint8_t bodyStorage[192U] = {};
    uint8_t fingerprint[blinker::bbp2::kManifestFingerprintSize];
    for (size_t index = 0U; index < sizeof(fingerprint); ++index) {
        fingerprint[index] = static_cast<uint8_t>(0x60U + index);
    }
    blinker::bbp2::HelloBody hello;
    hello.role = blinker::bbp2::PeerRole::Device;
    hello.versions[0] = blinker::bbp2::kVersion;
    hello.versionCount = 1U;
    hello.features =
        blinker::kChildSessionRequiredFeatures |
        blinker::bbp2::FeatureTelemetry;
    hello.maxFrameSize = 256U;
    hello.maxReassemblySize = 512U;
    hello.authorizationMethods[0] =
        blinker::security::kControllerHmacSha256Method;
    hello.authorizationMethodCount = 1U;
    hello.reliableReceiveWindow = 2U;
    if (includeManifest) {
        hello.manifestRevision = 9U;
        hello.manifestFingerprint = blinker::ByteView(
            fingerprint, sizeof(fingerprint));
        hello.hasManifestRevision = true;
        hello.hasManifestFingerprint = true;
    }
    blinker::ByteView body;
    blinker::Result result = blinker::bbp2::encodeHelloBody(
        hello,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body);
    return result
               ? encodeFrame(
                     blinker::bbp2::MessageKind::Hello,
                     blinker::bbp2::FlagIsResponse,
                     sequence,
                     body,
                     output,
                     encoded)
               : result;
}

blinker::Result makeAuthResult(
    uint16_t sequence,
    const blinker::AuthorizationDecision& decision,
    blinker::MutableByteSpan output,
    blinker::ByteView& encoded) {
    uint8_t bodyStorage[96U] = {};
    blinker::bbp2::AuthResultBody response;
    response.method = blinker::security::kControllerHmacSha256Method;
    response.status = decision.status;
    response.payload = decision.responsePayload;
    response.hasPayload = !decision.responsePayload.empty();
    blinker::ByteView body;
    blinker::Result result = blinker::bbp2::encodeAuthResultBody(
        response,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body);
    return result
               ? encodeFrame(
                     blinker::bbp2::MessageKind::AuthResult,
                     blinker::bbp2::FlagIsResponse,
                     sequence,
                     body,
                     output,
                     encoded)
               : result;
}

void testFullResponderInterop() {
    SequenceRandom clientRandom(0x20U);
    blinker::ChildSessionCoordinator child(clientRandom);
    volatile size_t coordinatorSize = sizeof(child);
    CHECK_TRUE(coordinatorSize <= 384U);
    CredentialSource source;
    source.credential = makeCredential();
    SequenceRandom deviceRandom(0x40U);
    ResponderDirectSessions responderDirect;
    blinker::security::ControllerAuthSession responderSessions[1];
    blinker::security::ControllerHmacSha256Authorizer responder(
        source,
        deviceRandom,
        responderDirect,
        responderSessions,
        1U,
        blinker::security::ControllerAuthTransportPolicy::
            EstablishDirectSecure);
    blinker::AuthorizationSessionContext session;
    session.transportId = 3U;
    session.sessionId = 81U;

    uint8_t outbound[512U] = {};
    uint8_t inbound[512U] = {};
    blinker::ByteView request;
    CHECK_TRUE(child.begin(
        source.credential,
        100U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        request));
    blinker::bbp2::FrameView parsed;
    CHECK_TRUE(blinker::bbp2::parseFrame(request, parsed));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        parsed.header.kind);
    CHECK_EQ(static_cast<uint16_t>(1U), parsed.header.sequence);
    blinker::bbp2::HelloBody appHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(parsed.body, appHello));
    CHECK_TRUE(appHello.role == blinker::bbp2::PeerRole::App);
    CHECK_EQ(
        blinker::kChildSessionRequiredFeatures,
        appHello.features);

    blinker::ByteView response;
    CHECK_TRUE(makeDeviceHello(
        parsed.header.sequence,
        true,
        blinker::MutableByteSpan(inbound, sizeof(inbound)),
        response));
    CHECK_TRUE(child.handleFrame(
        response,
        101U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        request));
    CHECK_TRUE(blinker::bbp2::parseFrame(request, parsed));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Authenticate),
        parsed.header.kind);
    CHECK_EQ(static_cast<uint16_t>(2U), parsed.header.sequence);
    blinker::bbp2::AuthRequestBody authRequest;
    CHECK_TRUE(blinker::bbp2::decodeAuthRequestBody(
        parsed.body, authRequest));
    blinker::AuthorizationDecision decision;
    CHECK_TRUE(responder.authorize(
        authRequest.method,
        authRequest.payload,
        session,
        decision));
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Continue);

    CHECK_TRUE(makeAuthResult(
        parsed.header.sequence,
        decision,
        blinker::MutableByteSpan(inbound, sizeof(inbound)),
        response));
    CHECK_TRUE(child.handleFrame(
        response,
        102U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        request));
    CHECK_TRUE(blinker::bbp2::parseFrame(request, parsed));
    CHECK_EQ(static_cast<uint16_t>(3U), parsed.header.sequence);
    CHECK_TRUE(blinker::bbp2::decodeAuthRequestBody(
        parsed.body, authRequest));
    CHECK_TRUE(responder.authorize(
        authRequest.method,
        authRequest.payload,
        session,
        decision));
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Authorized);
    responder.authorizationResultQueued(session);

    CHECK_TRUE(makeAuthResult(
        parsed.header.sequence,
        decision,
        blinker::MutableByteSpan(inbound, sizeof(inbound)),
        response));
    CHECK_TRUE(child.handleFrame(
        response,
        103U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        request));
    CHECK_TRUE(request.empty());
    CHECK_TRUE(child.secure());
    CHECK_EQ(
        blinker::kChildSessionRequiredFeatures,
        child.negotiatedFeatures());
    CHECK_EQ(static_cast<uint16_t>(256U), child.negotiatedMaxFrameSize());
    CHECK_EQ(static_cast<uint8_t>(2U), child.remoteReliableReceiveWindow());
    CHECK_EQ(static_cast<uint32_t>(9U), child.manifestRevision());
    CHECK_EQ(
        static_cast<size_t>(blinker::bbp2::kManifestFingerprintSize),
        child.manifestFingerprint().size);
    CHECK_TRUE(blinker::constantTimeEqual(
        child.secureSession().sessionBinding(),
        responderDirect.session.sessionBinding()));
    CHECK_TRUE(child.poll(UINT32_MAX));

    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        child.handleFrame(
            response,
            104U,
            blinker::MutableByteSpan(outbound, sizeof(outbound)),
            request).code());
    CHECK_TRUE(child.state() == blinker::ChildSessionState::Idle);
    CHECK_TRUE(!child.secureSession().ready());
    responder.sessionClosed(session);
}

void testDeadlineAndInputGates() {
    blinker::ControllerCredential credential = makeCredential();
    blinker::ChildSessionConfig config;
    config.authenticationTimeoutMillis = 150U;
    SequenceRandom random(0x20U);
    blinker::ChildSessionCoordinator child(random, config);
    uint8_t outbound[512U] = {};
    uint8_t inbound[512U] = {};
    blinker::ByteView frame;
    const uint32_t start = UINT32_MAX - 100U;
    CHECK_TRUE(child.begin(
        credential,
        start,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        frame));
    CHECK_TRUE(child.poll(48U));
    CHECK_EQ(
        blinker::ErrorCode::NotConnected,
        child.poll(49U).code());
    CHECK_TRUE(child.state() == blinker::ChildSessionState::Idle);

    CHECK_TRUE(child.begin(
        credential,
        0U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        frame));
    uint8_t oversized[513U] = {};
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        child.handleFrame(
            blinker::ByteView(oversized, sizeof(oversized)),
            1U,
            blinker::MutableByteSpan(outbound, sizeof(outbound)),
            frame).code());
    CHECK_TRUE(child.state() == blinker::ChildSessionState::Idle);

    CHECK_TRUE(child.begin(
        credential,
        0U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        frame));
    blinker::bbp2::FrameView request;
    CHECK_TRUE(blinker::bbp2::parseFrame(frame, request));
    blinker::ByteView response;
    CHECK_TRUE(makeDeviceHello(
        request.header.sequence,
        true,
        blinker::MutableByteSpan(inbound, sizeof(inbound)),
        response));
    CHECK_TRUE(child.handleFrame(
        response,
        149U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        frame));
    CHECK_TRUE(
        child.state() ==
        blinker::ChildSessionState::AwaitingChallenge);
    CHECK_EQ(
        blinker::ErrorCode::NotConnected,
        child.poll(150U).code());

    CHECK_TRUE(child.begin(
        credential,
        0U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        frame));
    CHECK_TRUE(blinker::bbp2::parseFrame(frame, request));
    CHECK_TRUE(makeDeviceHello(
        static_cast<uint16_t>(request.header.sequence + 1U),
        true,
        blinker::MutableByteSpan(inbound, sizeof(inbound)),
        response));
    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        child.handleFrame(
            response,
            1U,
            blinker::MutableByteSpan(outbound, sizeof(outbound)),
            frame).code());
    CHECK_TRUE(child.state() == blinker::ChildSessionState::Idle);

    CHECK_TRUE(child.begin(
        credential,
        0U,
        blinker::MutableByteSpan(outbound, sizeof(outbound)),
        frame));
    CHECK_TRUE(blinker::bbp2::parseFrame(frame, request));
    CHECK_TRUE(makeDeviceHello(
        request.header.sequence,
        false,
        blinker::MutableByteSpan(inbound, sizeof(inbound)),
        response));
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        child.handleFrame(
            response,
            1U,
            blinker::MutableByteSpan(outbound, sizeof(outbound)),
            frame).code());
    CHECK_TRUE(child.manifestFingerprint().empty());

    blinker::ChildSessionConfig invalid;
    invalid.features &= ~static_cast<uint32_t>(
        blinker::bbp2::FeatureReliableDelivery);
    SequenceRandom invalidRandom(0x20U);
    blinker::ChildSessionCoordinator invalidChild(
        invalidRandom, invalid);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        invalidChild.begin(
            credential,
            0U,
            blinker::MutableByteSpan(outbound, sizeof(outbound)),
            frame).code());
    CHECK_EQ(static_cast<uint32_t>(0U), invalidRandom.calls);
    blinker::clearControllerCredential(credential);
}

void testRepeatedTimeoutRecovery() {
    blinker::ControllerCredential credential = makeCredential();
    blinker::ChildSessionConfig config;
    config.authenticationTimeoutMillis = 10U;
    SequenceRandom random(0x20U);
    blinker::ChildSessionCoordinator child(random, config);
    uint8_t frameStorage[512U] = {};
    blinker::ByteView frame;
    for (uint32_t cycle = 0U; cycle < 32U; ++cycle) {
        const uint32_t started = cycle * 100U;
        CHECK_TRUE(child.begin(
            credential,
            started,
            blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)),
            frame));
        CHECK_TRUE(!frame.empty());
        CHECK_EQ(
            blinker::ErrorCode::NotConnected,
            child.poll(started + 10U).code());
        CHECK_TRUE(child.state() == blinker::ChildSessionState::Idle);
        CHECK_TRUE(!child.secureSession().ready());
        CHECK_TRUE(child.manifestFingerprint().empty());
    }
    CHECK_EQ(static_cast<uint32_t>(32U), random.calls);
    blinker::clearControllerCredential(credential);
}

void testOptionalFeatureInstallationBoundary() {
    SequenceRandom random(0x20U);
    blinker::ChildSessionCoordinator child(random);
    const auto credential = makeCredential();
    uint8_t output[512U]; blinker::ByteView request;
    CHECK_TRUE(child.setOptionalFeatures(blinker::bbp2::FeatureTimeSync));
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature,
             child.setOptionalFeatures(0x80000000U).code());
    for (unsigned round = 0U; round < 3U; ++round) {
        if (round == 2U) CHECK_TRUE(child.setOptionalFeatures(0U));
        CHECK_TRUE(child.begin(credential, 0U,
            blinker::MutableByteSpan(output, sizeof(output)), request));
        blinker::bbp2::FrameView frame; blinker::bbp2::HelloBody hello;
        CHECK_TRUE(blinker::bbp2::parseFrame(request, frame));
        CHECK_TRUE(blinker::bbp2::decodeHelloBody(frame.body, hello));
        CHECK_EQ(blinker::kChildSessionRequiredFeatures |
                     (round != 2U ? static_cast<uint32_t>(blinker::bbp2::FeatureTimeSync) : 0U),
                 hello.features);
        CHECK_EQ(blinker::ErrorCode::StateConflict, child.setOptionalFeatures(0U).code());
        child.reset(); // A failed/closed handshake preserves the installed policy.
    }
}

} // namespace

int main() {
    testFullResponderInterop();
    testDeadlineAndInputGates();
    testRepeatedTimeoutRecovery();
    testOptionalFeatureInstallationBoundary();
    std::cout << "Child session coordinator tests passed" << std::endl;
    return 0;
}
