#include <BlinkerV2/protocol/ble/Mode.h>
#include <BlinkerV2/runtime/GatewayChildSession.h>
#include <BlinkerV2/runtime/GatewayPresenceMatcher.h>
#include <BlinkerV2/runtime/GatewayChildExecutor.h>
#include <BlinkerV2/runtime/GatewayTimeResponder.h>
#include <BlinkerV2/protocol/bbp2/TimeSync.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>
#include <BlinkerV2/transport/GattDirectChildLink.h>
#include "support/DeterministicNoiseCrypto.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK_TRUE(value)                                                     \
    do {                                                                      \
        if (!(value)) {                                                       \
            std::cerr << "Gateway child session check failed at line "       \
                      << __LINE__ << ": " << #value << std::endl;            \
            std::exit(1);                                                     \
        }                                                                     \
    } while (false)

void fill(uint8_t* bytes, size_t size, uint8_t seed) {
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = static_cast<uint8_t>(seed + index);
    }
}

class MemoryBlob final : public blinker::IAtomicBlobStore {
public:
    MemoryBlob() : bytes(), size(0U), present(false) {}

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < size) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        if (value.data == nullptr || value.size > sizeof(bytes)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        std::memset(bytes, 0, sizeof(bytes));
        std::memcpy(bytes, value.data, value.size);
        size = value.size;
        present = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        blinker::secureZero(blinker::MutableByteSpan(bytes, sizeof(bytes)));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

    uint8_t bytes[blinker::GatewayCredentialRenewalStore::serializedSize];
    size_t size;
    bool present;
};

class TestClock final : public blinker::IClock {
public:
    TestClock() : nowMillis(100U), nowUnix(1700000000ULL) {}

    uint32_t monotonicMillis() const override { return nowMillis; }
    bool unixTime(uint64_t& output) const override {
        output = nowUnix;
        return true;
    }

    uint32_t nowMillis;
    uint64_t nowUnix;
};

class SequenceRandom final : public blinker::IRandom {
public:
    SequenceRandom() : next(0x20U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
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
};

class FakeLink {
public:
    FakeLink()
        : matcher(nullptr), matcherContext(nullptr), receiver(nullptr),
          receiverContext(nullptr), connectedHandler(nullptr),
          disconnectedHandler(nullptr), sessionContext(nullptr),
          faultHandler(nullptr), faultContext(nullptr), sent(), sentSize(0U),
          attempt(0U), nextAttempt(0U), running(false), isConnecting(false),
          isConnected(false), cancelRequested(false),
          synchronousDisconnect(false), rejectCancel(false) {}

    blinker::Result start() {
        running = true;
        return blinker::Result::success();
    }
    void stop() {
        running = false;
        isConnecting = false;
        isConnected = false;
        attempt = 0U;
    }
    void poll(uint32_t) {}
    blinker::Result connect(
        blinker::BleCentralAdvertisementMatcher value,
        void* context) {
        if (!running || value == nullptr) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConfigured);
        }
        matcher = value;
        matcherContext = context;
        attempt = ++nextAttempt;
        isConnecting = true;
        cancelRequested = false;
        return blinker::Result::success();
    }
    blinker::Result disconnect() {
        if (rejectCancel) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        if (attempt == 0U) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        cancelRequested = true;
        if (synchronousDisconnect) announceDisconnected();
        return blinker::Result::success();
    }
    blinker::Result sendRecord(blinker::ByteView record) {
        if (!isConnected || record.data == nullptr ||
            record.size > sizeof(sent)) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        sends.emplace_back(record.data, record.data + record.size);
        if (sendError != blinker::ErrorCode::Ok)
            return blinker::Result::failure(sendError);
        std::memcpy(sent, record.data, record.size);
        sentSize = record.size;
        return blinker::Result::success();
    }
    bool connected() const { return isConnected; }
    bool connecting() const { return isConnecting; }
    uint32_t attemptId() const { return attempt; }
    void receive(blinker::ByteView record, uint32_t fromAttempt = 0U) {
        blinker::RxContext rx;
        rx.sessionId = fromAttempt == 0U ? attempt : fromAttempt;
        CHECK_TRUE(receiver != nullptr);
        receiver(receiverContext, record, rx);
    }

    void setReceiver(blinker::BleRecordReceiver value, void* context) {
        receiver = value;
        receiverContext = context;
    }
    void setSessionHandlers(
        blinker::BleRecordSessionHandler connectedValue,
        blinker::BleRecordSessionHandler disconnectedValue,
        void* context) {
        connectedHandler = connectedValue;
        disconnectedHandler = disconnectedValue;
        sessionContext = context;
    }
    void setFaultHandler(
        blinker::GattDirectChildFaultHandler value,
        void* context) {
        faultHandler = value;
        faultContext = context;
    }

    bool offer(blinker::ByteView serviceData) {
        blinker::BleCentralAdvertisement advertisement;
        advertisement.modeServiceData = serviceData;
        return matcher != nullptr && matcher(matcherContext, advertisement);
    }
    void announceConnected() {
        isConnecting = false;
        isConnected = true;
        blinker::RxContext rx;
        rx.sessionId = attempt;
        if (connectedHandler != nullptr) {
            connectedHandler(sessionContext, rx);
        }
    }
    void announceDisconnected() {
        const uint32_t retired = attempt;
        attempt = 0U;
        isConnecting = false;
        isConnected = false;
        blinker::RxContext rx;
        rx.sessionId = retired;
        if (disconnectedHandler != nullptr) {
            disconnectedHandler(sessionContext, rx);
        }
    }

    blinker::BleCentralAdvertisementMatcher matcher;
    void* matcherContext;
    blinker::BleRecordReceiver receiver;
    void* receiverContext;
    blinker::BleRecordSessionHandler connectedHandler;
    blinker::BleRecordSessionHandler disconnectedHandler;
    void* sessionContext;
    blinker::GattDirectChildFaultHandler faultHandler;
    void* faultContext;
    uint8_t sent[512U];
    size_t sentSize;
    uint32_t attempt;
    uint32_t nextAttempt;
    bool running;
    bool isConnecting;
    bool isConnected;
    bool cancelRequested;
    bool synchronousDisconnect;
    bool rejectCancel;
    blinker::ErrorCode sendError = blinker::ErrorCode::Ok;
    std::vector<std::vector<uint8_t>> sends;
};

blinker::GatewayAccessRecord makeAccess() {
    blinker::GatewayAccessRecord access;
    fill(access.operationId, sizeof(access.operationId), 0x11U);
    fill(access.envelopeDigest, sizeof(access.envelopeDigest), 0x31U);
    fill(access.accessMaterialDigest,
         sizeof(access.accessMaterialDigest), 0x51U);
    const char child[] = "child-1";
    access.childLogicalDeviceIdSize = sizeof(child) - 1U;
    std::memcpy(access.childLogicalDeviceId, child, sizeof(child) - 1U);
    fill(access.childDeviceInstanceId,
         sizeof(access.childDeviceInstanceId), 0x61U);
    access.accessEpoch = 7U;
    fill(access.controllerId, sizeof(access.controllerId), 0x71U);
    access.credentialVersion = 3U;
    access.permissions = blinker::gateway::kGatewayPermissions;
    access.expiresAtUnixSeconds = 1800000000ULL;
    fill(access.secret, sizeof(access.secret), 0x81U);
    access.activePresence.version = 5U;
    fill(access.activePresence.key,
         sizeof(access.activePresence.key), 0x91U);
    return access;
}

blinker::GatewayCredentialRenewalRecord makeRenewal(
    const blinker::GatewayAccessRecord& active) {
    blinker::GatewayCredentialRenewalRecord pending;
    fill(pending.renewalOperationId,
         sizeof(pending.renewalOperationId), 0xA1U);
    std::memcpy(pending.topologyOperationId, active.operationId,
                sizeof(pending.topologyOperationId));
    std::memcpy(pending.childDeviceInstanceId,
                active.childDeviceInstanceId,
                sizeof(pending.childDeviceInstanceId));
    pending.accessEpoch = active.accessEpoch;
    std::memcpy(pending.controllerId, active.controllerId,
                sizeof(pending.controllerId));
    pending.expectedCredentialVersion = active.credentialVersion;
    pending.credentialVersion = active.credentialVersion + 1U;
    pending.presenceKeyVersion = active.activePresence.version;
    pending.expiresAtUnixSeconds = active.expiresAtUnixSeconds + 1000U;
    fill(pending.secret, sizeof(pending.secret), 0xB1U);
    fill(pending.accessMaterialDigest,
         sizeof(pending.accessMaterialDigest), 0xC1U);
    return pending;
}

void testCredentialRecoverySelectionAlternatesAfterAuthenticationFailure() {
    MemoryBlob activeBlob;
    blinker::GatewayAccessStore activeStore(activeBlob);
    blinker::GatewayAccessRecord active = makeAccess();
    blinker::GatewayAccessCommitOutcome activeOutcome;
    CHECK_TRUE(activeStore.commit(active, activeOutcome));

    MemoryBlob renewalBlob;
    blinker::GatewayCredentialRenewalStore renewalStore(renewalBlob);
    blinker::GatewayCredentialRenewalRecord pending = makeRenewal(active);
    blinker::GatewayCredentialRenewalCommitOutcome pendingOutcome;
    CHECK_TRUE(renewalStore.commit(pending, pendingOutcome));

    blinker::GatewaySessionAccessSource source(activeStore, renewalStore);
    blinker::GatewayAccessRecord selected;
    CHECK_TRUE(source.load(selected));
    CHECK_TRUE(selected.credentialVersion == active.credentialVersion);
    CHECK_TRUE(std::memcmp(selected.secret, active.secret,
                           sizeof(selected.secret)) == 0);
    blinker::clearGatewayAccessRecord(selected);

    uint8_t grantDigest[blinker::gateway::kAccessMaterialDigestSize] = {};
    fill(grantDigest, sizeof(grantDigest), 0xD1U);
    uint32_t mutationRevision = 0U;
    CHECK_TRUE(renewalStore.markMutationAttempted(
        blinker::ByteView(pending.renewalOperationId,
                          sizeof(pending.renewalOperationId)),
        blinker::ByteView(pending.topologyOperationId,
                          sizeof(pending.topologyOperationId)),
        blinker::ByteView(pending.accessMaterialDigest,
                          sizeof(pending.accessMaterialDigest)),
        blinker::ByteView(grantDigest, sizeof(grantDigest)),
        mutationRevision));

    CHECK_TRUE(source.load(selected));
    CHECK_TRUE(selected.credentialVersion == pending.credentialVersion);
    CHECK_TRUE(std::memcmp(selected.secret, pending.secret,
                           sizeof(selected.secret)) == 0);
    CHECK_TRUE(selected.activePresence.version ==
               active.activePresence.version);
    blinker::clearGatewayAccessRecord(selected);

    source.authenticationFailed();
    CHECK_TRUE(source.load(selected));
    CHECK_TRUE(selected.credentialVersion == active.credentialVersion);
    CHECK_TRUE(std::memcmp(selected.secret, active.secret,
                           sizeof(selected.secret)) == 0);
    blinker::clearGatewayAccessRecord(selected);

    source.authenticationFailed();
    CHECK_TRUE(source.load(selected));
    CHECK_TRUE(selected.credentialVersion == pending.credentialVersion);
    blinker::clearGatewayAccessRecord(selected);

    blinker::secureZero(blinker::MutableByteSpan(
        grantDigest, sizeof(grantDigest)));
    blinker::clearGatewayCredentialRenewalRecord(pending);
    blinker::clearGatewayAccessRecord(active);
}

blinker::ByteView advertisementFor(
    const blinker::GatewayAccessRecord& access,
    uint8_t (&storage)[blinker::ble::kModeServiceDataSize]) {
    blinker::DeviceInstanceId child;
    std::memcpy(child.bytes, access.childDeviceInstanceId,
                sizeof(child.bytes));
    const uint8_t nonce[blinker::ble::kPresenceNonceSize] = {
        0x21U, 0x22U, 0x23U, 0x24U
    };
    uint8_t locator[blinker::ble::kPresenceLocatorSize] = {};
    CHECK_TRUE(blinker::ble::derivePresenceLocator(
        blinker::ByteView(access.activePresence.key,
                          sizeof(access.activePresence.key)),
        child, access.accessEpoch, access.activePresence.version,
        blinker::ByteView(nonce, sizeof(nonce)),
        blinker::MutableByteSpan(locator, sizeof(locator))));
    const blinker::ble::ModeProfile profile =
        blinker::ble::makeAuthorizedDirectModeProfile(
            blinker::ByteView(locator, sizeof(locator)));
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        profile, blinker::MutableByteSpan(storage, sizeof(storage)),
        encoded));
    return encoded;
}

void testExactDiscoveryStartsOneAuthenticationAttempt() {
    MemoryBlob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord access = makeAccess();
    blinker::GatewayAccessCommitOutcome outcome;
    CHECK_TRUE(store.commit(access, outcome));

    FakeLink link;
    TestClock clock;
    SequenceRandom random;
    DeterministicTestCrypto crypto;
    uint8_t handshake[512U] = {};
    uint8_t plaintext[512U] = {};
    uint8_t secureRecord[536U] = {};
    blinker::BasicGatewayChildSession<
        FakeLink,
        blinker::GatewayPresenceMatcher> session(
        link, store, clock, random, crypto,
        blinker::MutableByteSpan(handshake, sizeof(handshake)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        blinker::MutableByteSpan(secureRecord, sizeof(secureRecord)));

    CHECK_TRUE(session.start());
    CHECK_TRUE(session.connect());
    CHECK_TRUE(session.state() ==
               blinker::GatewayChildSessionState::Connecting);

    uint8_t serviceData[blinker::ble::kModeServiceDataSize] = {};
    blinker::ByteView exact = advertisementFor(access, serviceData);
    serviceData[sizeof(serviceData) - 1U] ^= 0x01U;
    CHECK_TRUE(!link.offer(exact));
    serviceData[sizeof(serviceData) - 1U] ^= 0x01U;
    CHECK_TRUE(link.offer(exact));
    CHECK_TRUE(session.matchedPresenceVersion() ==
               access.activePresence.version);

    link.announceConnected();
    CHECK_TRUE(session.state() ==
               blinker::GatewayChildSessionState::Authenticating);
    blinker::bbp2::FrameView hello;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(link.sent, link.sentSize), hello));
    CHECK_TRUE(hello.header.kind == static_cast<uint8_t>(
        blinker::bbp2::MessageKind::Hello));

    clock.nowMillis += 15000U;
    session.poll(1000U);
    CHECK_TRUE(link.cancelRequested);
    CHECK_TRUE(session.state() ==
               blinker::GatewayChildSessionState::Disconnecting);
    link.announceDisconnected();
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Idle);
    CHECK_TRUE(!session.secure());

    CHECK_TRUE(session.connect());
    CHECK_TRUE(link.offer(exact));
    link.announceConnected();
    link.synchronousDisconnect = true;
    CHECK_TRUE(session.disconnect());
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Idle);
    link.synchronousDisconnect = false;

    clock.nowUnix = access.expiresAtUnixSeconds;
    CHECK_TRUE(session.connect().code() ==
               blinker::ErrorCode::AuthenticationRequired);

    clock.nowUnix = 1700000000ULL;
    CHECK_TRUE(session.connect());
    clock.nowUnix = access.expiresAtUnixSeconds;
    session.poll(1000U);
    CHECK_TRUE(session.state() ==
               blinker::GatewayChildSessionState::Disconnecting);
    session.poll(1000U);
    CHECK_TRUE(session.state() ==
               blinker::GatewayChildSessionState::Disconnecting);
    link.announceDisconnected();
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Idle);

    // A bearer failure may revoke record I/O before native cancellation is
    // accepted. It must not expose Idle or let management lose exact ownership.
    clock.nowUnix = 1700000000ULL;
    CHECK_TRUE(session.connect());
    const uint32_t retiring = session.attemptId();
    link.isConnecting = false;
    link.rejectCancel = true;
    link.faultHandler(link.faultContext, blinker::ErrorCode::NotConnected,
                      retiring);
    session.poll(1000U);
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Disconnecting);
    CHECK_TRUE(session.attemptId() == retiring);
    session.poll(1000U);
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Disconnecting);
    CHECK_TRUE(!session.connect());
    link.rejectCancel = false;
    session.poll(1000U);
    CHECK_TRUE(link.cancelRequested);
    link.announceDisconnected();
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Idle);
    CHECK_TRUE(session.connect());
    const uint32_t current = session.attemptId();
    blinker::RxContext stale;
    stale.sessionId = retiring;
    link.disconnectedHandler(link.sessionContext, stale);
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Connecting);
    CHECK_TRUE(session.attemptId() == current);
    CHECK_TRUE(session.disconnect());
    link.announceDisconnected();
    CHECK_TRUE(session.state() == blinker::GatewayChildSessionState::Idle);
}

// The real Method 2 authorizer/initiator and DirectSecure record machinery
// drive this fixture. Only the radio/storage and primitive provider are fakes.
using namespace blinker;
using namespace blinker::bbp2;

struct ResponderPeer : IControllerCredentialSource,
                       security::IDirectSecureSessionController {
    ControllerCredential credential;
    security::DirectSecureSession direct;
    bool committed = false;
    Result loadActive(ByteView id, ControllerCredentialDomain domain,
                      uint32_t generation, ControllerCredential& output) override {
        if (id.size != sizeof(credential.controllerId) ||
            std::memcmp(id.data, credential.controllerId, id.size) != 0 ||
            domain != credential.domain || generation != credential.ownershipGeneration)
            return Result::failure(ErrorCode::NotFound);
        output = credential;
        return Result::success();
    }
    StorageProtection protection() const override { return StorageProtection::EncryptedFlash; }
    Result prepare(uint32_t, ByteView secret, const security::DirectSecureContext& context) override {
        committed = false;
        return direct.begin(security::DirectSecureRole::Responder, secret, context);
    }
    void commit(uint32_t) override { committed = true; }
    void cancel(uint32_t) override { committed = false; direct.clear(); }
};

struct WallSource : IWallTimeSource {
    unsigned reads = 0U;
    bool valid = true;
    void (*onRead)(void*) = nullptr;
    void* context = nullptr;
    bool readTime(WallClockSnapshot& output) override {
        ++reads;
        if (onRead != nullptr) onRead(context);
        output.utcMillis = 1789260000123ULL;
        output.uncertaintyMillis = 1051U;
        output.state = WallClockState::Synchronized;
        return valid;
    }
};

std::vector<uint8_t> encodeTestFrame(MessageKind kind, uint16_t sequence,
                                   ByteView body, uint8_t flags = FlagIsResponse) {
    std::vector<uint8_t> output(kBaseHeaderSize + body.size);
    FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.sequence = sequence; header.flags = flags;
    header.bodyLength = static_cast<uint16_t>(body.size);
    CHECK_TRUE(encodeHeader(header, MutableByteSpan(output.data(), output.size())));
    if (!body.empty()) std::memcpy(output.data() + kBaseHeaderSize, body.data, body.size);
    return output;
}
ByteView view(const std::vector<uint8_t>& bytes) { return ByteView(bytes.data(), bytes.size()); }

struct TimeSessionFixture {
    using Session = BasicGatewayChildSession<FakeLink, GatewayPresenceMatcher>;
    MemoryBlob blob;
    GatewayAccessStore store;
    GatewayAccessRecord access;
    FakeLink link;
    TestClock clock;
    SequenceRandom random;
    DeterministicTestCrypto crypto;
    uint8_t handshake[512U] = {}, plaintext[512U] = {}, record[536U] = {};
    Session session;
    BasicGatewayChildConnection<Session> connection;
    WallSource source;
    GatewayTimeResponder time;
    BasicGatewayChildExecutor<Session> executor;
    ResponderPeer peer;
    SequenceRandom remoteRandom;
    security::ControllerAuthSession authSessions[1U];
    security::ControllerHmacSha256Authorizer authorizer;
    unsigned events = 0U, replies = 0U;
    TimeSessionFixture()
        : store(blob), access(makeAccess()),
          session(link, store, clock, random, crypto,
                  MutableByteSpan(handshake, sizeof(handshake)),
                  MutableByteSpan(plaintext, sizeof(plaintext)),
                  MutableByteSpan(record, sizeof(record))),
          connection(session), time(source), executor(session, connection),
          authorizer(peer, remoteRandom, peer, authSessions, 1U,
                     security::ControllerAuthTransportPolicy::EstablishDirectSecure) {
        GatewayAccessCommitOutcome outcome;
        CHECK_TRUE(store.commit(access, outcome));
        CHECK_TRUE(store.load(access)); // Use the committed storage revision.
        CHECK_TRUE(makeGatewayControllerCredential(access, clock.nowUnix, peer.credential));
        executor.setObserver(&onEvent, nullptr, nullptr, nullptr, this);
    }
    static void onEvent(void* context, ByteView, GatewayChildFrameSource) {
        ++static_cast<TimeSessionFixture*>(context)->events;
    }
    static void onReply(void* context, ByteView, GatewayChildFrameSource source) {
        CHECK_TRUE(source == GatewayChildFrameSource::PendingResponse);
        ++static_cast<TimeSessionFixture*>(context)->replies;
    }
    FrameView sentFrame() {
        FrameView frame;
        CHECK_TRUE(parseFrame(ByteView(link.sent, link.sentSize), frame));
        return frame;
    }
    void connect(bool localTime = true, bool remoteTime = true) {
        if (link.nextAttempt == 0U) {
            CHECK_TRUE(executor.setRequestHandler(localTime ? &time : nullptr));
        } else {
            AuthorizationSessionContext old;
            old.transportId = 3U; old.sessionId = link.nextAttempt;
            authorizer.sessionClosed(old);
        }
        CHECK_TRUE(session.authenticatedPermissions() == 0U);
        if (session.state() == GatewayChildSessionState::Stopped) CHECK_TRUE(session.start());
        CHECK_TRUE(session.connect());
        CHECK_TRUE(!executor.setRequestHandler(nullptr)); // No live mutation.
        CHECK_TRUE(session.authenticatedPermissions() == 0U);
        link.announceConnected();
        HelloBody hello;
        const FrameView request = sentFrame();
        CHECK_TRUE(decodeHelloBody(request.body, hello));
        CHECK_TRUE(((hello.features & FeatureTimeSync) != 0U) == localTime);
        CHECK_TRUE((hello.features & FeatureControllerControl) != 0U);
        hello.role = PeerRole::Device;
        hello.features = kChildSessionRequiredFeatures | FeatureControllerControl |
            (remoteTime ? static_cast<uint32_t>(FeatureTimeSync) : 0U);
        uint8_t fingerprint[kManifestFingerprintSize] = {1U};
        hello.hasManifestRevision = hello.hasManifestFingerprint = true;
        hello.manifestRevision = 1U;
        hello.manifestFingerprint = ByteView(fingerprint, sizeof(fingerprint));
        uint8_t encoded[256U]; ByteView body;
        CHECK_TRUE(encodeHelloBody(hello, MutableByteSpan(encoded, sizeof(encoded)), body));
        link.receive(view(encodeTestFrame(MessageKind::Hello, request.header.sequence, body)));
        AuthorizationSessionContext context;
        context.transportId = 3U; context.sessionId = link.attempt;
        for (unsigned step = 0U; step < 2U; ++step) {
            CHECK_TRUE(session.authenticatedPermissions() == 0U);
            FrameView auth = sentFrame(); AuthRequestBody init;
            CHECK_TRUE(decodeAuthRequestBody(auth.body, init));
            AuthorizationDecision decision;
            CHECK_TRUE(authorizer.authorize(init.method, init.payload, context, decision));
            CHECK_TRUE(decision.status == (step == 0U ? AuthStatus::Continue : AuthStatus::Authorized));
            AuthResultBody result;
            result.method = init.method; result.status = decision.status;
            result.payload = decision.responsePayload; result.hasPayload = !result.payload.empty();
            CHECK_TRUE(encodeAuthResultBody(result, MutableByteSpan(encoded, sizeof(encoded)), body));
            if (step != 0U) authorizer.authorizationResultQueued(context);
            link.receive(view(encodeTestFrame(MessageKind::AuthResult, auth.header.sequence, body)));
            if (step == 0U && peer.credential.permissions != access.permissions) {
                CHECK_TRUE(!session.secure()); CHECK_TRUE(session.authenticatedPermissions() == 0U);
                session.poll(1000U);
                CHECK_TRUE(session.state() == GatewayChildSessionState::Disconnecting);
                return;
            }
        }
        CHECK_TRUE(peer.committed && session.secure());
        CHECK_TRUE(session.authenticatedPermissions() == access.permissions);
        CHECK_TRUE(((session.negotiatedFeatures() & FeatureTimeSync) != 0U) == (localTime && remoteTime));
    }
    std::vector<uint8_t> sealed(ByteView frame) {
        uint8_t output[536U]; ByteView value;
        CHECK_TRUE(peer.direct.seal(crypto, frame, MutableByteSpan(output, sizeof(output)), value));
        return std::vector<uint8_t>(value.data, value.data + value.size);
    }
    std::vector<uint8_t> requestTime(uint32_t attempt = 0U, uint16_t sequence = 65535U) {
        uint8_t nonce[kTimeSyncNonceSize] = {1U}, body[kTimeRequestSize]; size_t size = 0U;
        CHECK_TRUE(encodeTimeRequest(ByteView(nonce, sizeof(nonce)), MutableByteSpan(body, sizeof(body)), size));
        auto encrypted = sealed(view(encodeTestFrame(
            MessageKind::TimeRequest, sequence, ByteView(body, size), FlagNone)));
        link.receive(view(encrypted), attempt);
        return encrypted;
    }
    std::vector<uint8_t> decryptLast() {
        uint8_t decoded[512U]; ByteView value;
        CHECK_TRUE(peer.direct.open(crypto, ByteView(link.sent, link.sentSize),
                                   MutableByteSpan(decoded, sizeof(decoded)), value));
        return std::vector<uint8_t>(value.data, value.data + value.size);
    }
    void sample(uint16_t sequence = 65535U) {
        const auto decoded = decryptLast(); FrameView frame;
        CHECK_TRUE(parseFrame(view(decoded), frame));
        CHECK_TRUE(frame.header.kind == static_cast<uint8_t>(MessageKind::TimeResponse));
        CHECK_TRUE(frame.header.sequence == sequence && decoded.size() == 40U);
        TimeResponseView timeValue; CHECK_TRUE(decodeTimeResponse(frame.body, timeValue));
        CHECK_TRUE(timeValue.nonce.data[0] == 1U && timeValue.utcAtReplyMillis == 1789260000123ULL);
        CHECK_TRUE(timeValue.uncertaintyMillis == 1051U);
    }
};

void testTimeExecutorEndToEnd() {
    TimeSessionFixture f; f.connect();
    // An unrelated ordinary request remains pending while child-originated
    // time uses its own request sequence and the SAME DirectSecure sequence.
    const auto request = encodeTestFrame(MessageKind::StateRequest, 7U, ByteView(), FlagNone);
    CHECK_TRUE(f.executor.sendDataFrame(&f, &TimeSessionFixture::onReply, view(request)));
    auto sent = f.decryptLast(); FrameView pending; CHECK_TRUE(parseFrame(view(sent), pending));
    const uint16_t ordinarySequence = pending.header.sequence;
    f.requestTime(0U, ordinarySequence); f.sample(ordinarySequence);
    CHECK_TRUE(f.source.reads == 1U && f.events == 0U && f.replies == 0U);
    const size_t before = f.link.sends.size();
    f.requestTime(); CHECK_TRUE(f.source.reads == 1U && f.link.sends.size() == before);
    AckBody ack; ack.acknowledgedSequence = ordinarySequence;
    uint8_t ackStorage[32U]; ByteView ackBody;
    CHECK_TRUE(encodeAckBody(ack, MutableByteSpan(ackStorage, sizeof(ackStorage)), ackBody));
    const auto answer = encodeTestFrame(MessageKind::Ack, ordinarySequence, ackBody);
    f.link.receive(view(f.sealed(view(answer))));
    CHECK_TRUE(f.replies == 1U && f.events == 0U);
    f.clock.nowMillis += 5000U;
    f.source.valid = false; f.requestTime();
    auto failure = f.decryptLast(); FrameView error; CHECK_TRUE(parseFrame(view(failure), error));
    CHECK_TRUE(error.header.kind == static_cast<uint8_t>(MessageKind::Error));
    ErrorBody body; CHECK_TRUE(decodeErrorBody(error.body, body));
    CHECK_TRUE(body.errorCode == 8U && body.relatedSequence == 65535U);
}

void testTimeOptionalAndSessionFences() {
    for (unsigned mode = 0U; mode < 2U; ++mode) {
        TimeSessionFixture f; f.connect(mode != 0U, mode == 0U);
        const size_t before = f.link.sends.size(); f.requestTime();
        CHECK_TRUE(f.link.sends.size() == before && f.source.reads == 0U && f.events == 0U);
    }
    TimeSessionFixture mismatch;
    mismatch.peer.credential.permissions = kControllerPermissionObserve;
    mismatch.connect(); // Real proof rejects the differing permission transcript.
    for (unsigned mode = 0U; mode < 6U; ++mode) {
        TimeSessionFixture f; f.connect();
        const size_t before = f.link.sends.size();
        switch (mode) {
        case 0U: f.clock.nowUnix = f.access.expiresAtUnixSeconds; break;
        case 1U: f.link.rejectCancel = true; CHECK_TRUE(!f.session.disconnect()); break;
        case 2U: f.link.faultHandler(f.link.faultContext, ErrorCode::NotConnected, f.session.attemptId()); break;
        case 3U: ++f.link.attempt; break;
        case 4U: f.session.stop(); break;
        case 5U: f.clock.nowUnix = 0U; break;
        }
        CHECK_TRUE(f.session.authenticatedPermissions() == 0U);
        CHECK_TRUE(!f.session.sendFrame(view(encodeTestFrame(MessageKind::Event, 0U, ByteView()))));
        f.requestTime(); CHECK_TRUE(f.link.sends.size() == before && f.source.reads == 0U);
    }
    // Stale callbacks neither act nor poison the current decrypt sequence.
    TimeSessionFixture stale; stale.connect();
    const auto record = stale.requestTime(stale.session.attemptId() + 1U);
    CHECK_TRUE(stale.source.reads == 0U && stale.session.secure());
    stale.link.receive(view(record)); stale.sample();
    // Reset the per-session quota only after native disconnect completion.
    CHECK_TRUE(stale.session.disconnect()); stale.link.announceDisconnected();
    stale.connect(); stale.requestTime(); stale.sample();
    CHECK_TRUE(stale.source.reads == 2U);
    // Full stop detaches native callbacks; a fresh successful handshake must
    // reset module state even without the ordinary disconnect notification.
    stale.session.stop(); stale.connect(); stale.requestTime(); stale.sample();
    CHECK_TRUE(stale.source.reads == 3U);
}

void testTimeBackpressureAndReentrancy() {
    TimeSessionFixture f; f.connect();
    f.link.sendError = ErrorCode::WouldBlock;
    f.requestTime(); const auto exact = f.link.sends.back();
    f.session.poll(1000U); CHECK_TRUE(f.link.sends.back() == exact);
    f.link.sendError = ErrorCode::Ok; f.session.poll(1000U);
    CHECK_TRUE(f.link.sends.back() == exact); f.sample(); // Never re-sealed.
    // Occupy the one ciphertext slot first: responder has no second queue.
    f.clock.nowMillis += 5000U; f.link.sendError = ErrorCode::WouldBlock;
    const auto ordinary = encodeTestFrame(MessageKind::Event, 0U, ByteView());
    CHECK_TRUE(f.session.sendFrame(view(ordinary))); const auto pending = f.link.sends.back();
    const size_t before = f.link.sends.size(); f.requestTime();
    CHECK_TRUE(f.link.sends.size() == before && f.source.reads == 2U);
    f.link.sendError = ErrorCode::Ok; f.session.poll(1000U);
    CHECK_TRUE(f.link.sends.back() == pending); CHECK_TRUE(f.decryptLast() == ordinary);
    f.clock.nowMillis += 5000U; f.requestTime(); f.sample(); // No record sequence gap.
    CHECK_TRUE(f.source.reads == 3U && f.events == 0U);
    // Fatal enqueue immediately revokes authority, before the next poll.
    TimeSessionFixture fatal; fatal.connect(); fatal.link.sendError = ErrorCode::NotConnected;
    fatal.requestTime(); CHECK_TRUE(fatal.session.authenticatedPermissions() == 0U);
    fatal.session.poll(1000U); CHECK_TRUE(fatal.link.cancelRequested);
    // A sealed pending reply is discarded on expiry, not flushed on poll.
    TimeSessionFixture expired; expired.connect(); expired.link.sendError = ErrorCode::WouldBlock;
    expired.requestTime(); const size_t expireSends = expired.link.sends.size();
    expired.clock.nowUnix = expired.access.expiresAtUnixSeconds;
    expired.link.sendError = ErrorCode::Ok; expired.session.poll(1000U);
    CHECK_TRUE(expired.link.sends.size() == expireSends && expired.link.cancelRequested);
    // Expiry during a borrowed source callback fences the eventual send.
    TimeSessionFixture reentrant; reentrant.connect();
    reentrant.source.context = &reentrant;
    reentrant.source.onRead = [](void* context) {
        auto& fixture = *static_cast<TimeSessionFixture*>(context);
        fixture.clock.nowUnix = fixture.access.expiresAtUnixSeconds;
    };
    const size_t queued = reentrant.link.sends.size(); reentrant.requestTime();
    CHECK_TRUE(reentrant.source.reads == 1U && reentrant.link.sends.size() == queued);
    CHECK_TRUE(reentrant.session.authenticatedPermissions() == 0U);
}

} // namespace

int main() {
    testCredentialRecoverySelectionAlternatesAfterAuthenticationFailure();
    testExactDiscoveryStartsOneAuthenticationAttempt();
    testTimeExecutorEndToEnd();
    testTimeOptionalAndSessionFences();
    testTimeBackpressureAndReentrancy();
    std::cout << "Gateway child session tests passed" << std::endl;
    return 0;
}
