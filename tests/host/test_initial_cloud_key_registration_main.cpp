#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/identity/ControllerCredentialTable.h>
#include <BlinkerV2/interface/IInitialCloudKeyInstaller.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <BlinkerV2/security/ControllerHmacSha256Initiator.h>
#include <BlinkerV2/security/InitialCloudKeyRegistration.h>
#include <BlinkerV2/transport/BleFrameTransport.h>
#include "support/DeterministicNoiseCrypto.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
using namespace blinker;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

// Only radio, clock, RNG and persistence are boundary substitutes. The original
// Method 2 HMAC, initiator, responder and DirectSecure transport are real code.
// DeterministicTestCrypto is explicitly NOT AES-GCM/X25519; this target proves
// authorization/lifecycle contracts, not primitive, radio or durable NVS results.
class Clock final : public IClock {
public:
    explicit Clock(uint32_t start = 100U) : now(start) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
    uint32_t now;
};

class Random final : public IRandom {
public:
    explicit Random(uint8_t seed) : next(seed) {}
    Result fill(MutableByteSpan bytes) override {
        if (bytes.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
        for (size_t i = 0U; i < bytes.size; ++i) bytes.data[i] = next++;
        return Result::success();
    }
    uint8_t next;
};

class Blob final : public IAtomicBlobStore {
public:
    Blob() : bytes(), size(0U), present(false) {}
    Result load(MutableByteSpan output, size_t& written) override {
        written = 0U;
        if (!present) return Result::failure(ErrorCode::NotFound);
        if (output.data == nullptr || output.size < size) return Result::failure(ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, size);
        written = size;
        return Result::success();
    }
    Result replace(ByteView input) override {
        if (input.data == nullptr || input.size > sizeof(bytes)) return Result::failure(ErrorCode::InvalidArgument);
        std::memcpy(bytes, input.data, input.size);
        size = input.size;
        present = true;
        return Result::success();
    }
    Result clear() override { present = false; size = 0U; std::memset(bytes, 0, sizeof(bytes)); return Result::success(); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
    uint8_t bytes[DeviceAccessStore::serializedSize];
    size_t size;
    bool present;
};

class Link final : public IBleLink {
public:
    Link() : current(BleLinkState::Stopped), session(), receiver(nullptr), receiverContext(nullptr),
             connected(nullptr), disconnected(nullptr), sessionContext(nullptr), packets(0U), retirementRequests(0U),
             observeSent(nullptr), observeSentContext(nullptr) {}
    Result start() override { current = BleLinkState::Ready; return Result::success(); }
    void stop() override { current = BleLinkState::Stopped; session.connected = false; }
    void poll(uint32_t) override {}
    BleLinkState state() const override { return current; }
    ErrorCode lastError() const override { return ErrorCode::Ok; }
    size_t sessionCount() const override { return session.connected ? 1U : 0U; }
    Result sessionAt(size_t index, BleSessionInfo& output) const override {
        if (index != 0U || !session.connected) return Result::failure(ErrorCode::NotFound);
        output = session;
        return Result::success();
    }
    Result sendPacket(uint32_t id, ByteView bytes) override {
        if (!session.connected || id != session.sessionId) return Result::failure(ErrorCode::NotConnected);
        if (bytes.data == nullptr || bytes.size > session.maxPacketSize) return Result::failure(ErrorCode::CapacityExceeded);
        ++packets;
        if (observeSent != nullptr) observeSent(observeSentContext, bytes);
        return Result::success();
    }
    Result disconnectSession(uint32_t id) override {
        if (!session.connected || id != session.sessionId) return Result::failure(ErrorCode::NotFound);
        ++retirementRequests;
        return Result::success();
    }
    void setPacketReceiver(BlePacketReceiver callback, void* context) override { receiver = callback; receiverContext = context; }
    void setSessionHandlers(BleSessionHandler onConnect, BleSessionHandler onDisconnect, void* context) override {
        connected = onConnect; disconnected = onDisconnect; sessionContext = context;
    }
    void connect(uint32_t id) {
        session = BleSessionInfo();
        session.sessionId = id; session.maxPacketSize = 20U;
        session.connected = true; session.notifyEnabled = true;
        if (connected != nullptr) connected(sessionContext, session);
    }
    void disconnect() {
        session.connected = false;
        if (disconnected != nullptr) disconnected(sessionContext, session);
    }
    void injectRecord(ByteView record, uint8_t recordId) {
        check(receiver != nullptr, "missing real BLE record receiver");
        size_t offset = 0U;
        uint8_t index = 0U;
        while (offset < record.size) {
            uint8_t packet[20U] = {};
            const size_t capacity = sizeof(packet) - ble::kFragmentHeaderSize;
            const size_t remaining = record.size - offset;
            const size_t count = remaining < capacity ? remaining : capacity;
            ble::FragmentHeader header;
            header.flags = offset == 0U ? ble::FragmentFlagStart : ble::FragmentFlagNone;
            if (count == remaining) header.flags = static_cast<uint8_t>(header.flags | ble::FragmentFlagEnd);
            header.frameId = recordId; header.index = index++;
            check(ble::encodeFragmentHeader(header, MutableByteSpan(packet, sizeof(packet))).ok(), "fragment encode failed");
            std::memcpy(packet + ble::kFragmentHeaderSize, record.data + offset, count);
            receiver(receiverContext, session, ByteView(packet, ble::kFragmentHeaderSize + count));
            offset += count;
        }
    }
    BleLinkState current;
    BleSessionInfo session;
    BlePacketReceiver receiver;
    void* receiverContext;
    BleSessionHandler connected;
    BleSessionHandler disconnected;
    void* sessionContext;
    uint32_t packets;
    uint32_t retirementRequests;
    void (*observeSent)(void*, ByteView);
    void* observeSentContext;
};

ControllerCredential credential(uint8_t seed = 0x20U,
    uint32_t permissions = kControllerPermissionAll) {
    ControllerCredential value;
    value.domain = ControllerCredentialDomain::PlatformAccess;
    value.ownershipGeneration = 7U; value.credentialVersion = 1U;
    value.permissions = permissions;
    for (size_t i = 0U; i < sizeof(value.controllerId); ++i) value.controllerId[i] = static_cast<uint8_t>(seed + i);
    for (size_t i = 0U; i < sizeof(value.secret); ++i) value.secret[i] = static_cast<uint8_t>(0x60U + i);
    return value;
}

DeviceKey candidateKey(uint8_t seed = 1U) {
    DeviceKey value;
    for (size_t i = 0U; i < sizeof(value.bytes); ++i) value.bytes[i] = static_cast<uint8_t>(seed + i);
    return value;
}

// This is only an installer-boundary spy. It always calls the production guard
// with a freshly loaded real root; success means guard acceptance, not NVS ACK.
class Installer final : public IInitialCloudKeyInstaller {
public:
    explicit Installer(DeviceAccessStore& source)
        : access(source), calls(0U), accepted(0U), outcome(ErrorCode::Ok), beforeGuard(nullptr), afterGuard(nullptr), hookContext(nullptr) {}
    Result installInitialCloudKeyAuthorized(const DeviceKey& key, uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin, IInitialCloudKeyCommitGuard& guard) override {
        ++calls;
        if (beforeGuard != nullptr) beforeGuard(hookContext);
        uint32_t actualEpoch = 0U;
        ControllerCredential actual;
        Result result = access.loadAccessEpoch(actualEpoch);
        if (result) result = access.loadActive(expectedAdmin.id(), expectedAdmin.domain, actualEpoch, actual);
        if (result && (actualEpoch != expectedEpoch || !sameControllerCredential(actual, expectedAdmin)))
            result = Result::failure(ErrorCode::StateConflict);
        if (result) result = guard.validateInitialCloudKeyCommit(key, actualEpoch, actual);
        clearControllerCredential(actual);
        if (!result) return result;
        ++accepted;
        if (afterGuard != nullptr) afterGuard(hookContext);
        return outcome == ErrorCode::Ok ? Result::success() : Result::failure(outcome);
    }
    DeviceAccessStore& access;
    uint32_t calls;
    uint32_t accepted;
    ErrorCode outcome;
    void (*beforeGuard)(void*);
    void (*afterGuard)(void*);
    void* hookContext;
};

class CredentialSource final : public IControllerCredentialSource {
public:
    explicit CredentialSource(DeviceAccessStore& source) : root(source), additional() {}
    Result loadActive(ByteView id, ControllerCredentialDomain domain, uint32_t epoch,
        ControllerCredential& output) override {
        Result result = root.loadActive(id, domain, epoch, output);
        return result.code() == ErrorCode::NotFound
            ? additional.loadActive(id, domain, epoch, output) : result;
    }
    StorageProtection protection() const override { return root.protection(); }
    DeviceAccessStore& root;
    // Only credential storage data for roles the concrete access-root format
    // does not install. The real authorizer still verifies each role's HMAC.
    ControllerCredentialTable additional;
};

class LegacyDirectObservation final : public security::IDirectSecureSessionController {
public:
    explicit LegacyDirectObservation(BleFrameTransport& actual) : direct(actual) {}
    Result prepare(uint32_t id, ByteView secret, const security::DirectSecureContext& context) override {
        return direct.prepare(id, secret, context);
    }
    void commit(uint32_t id) override { direct.commit(id); }
    void cancel(uint32_t id) override { direct.cancel(id); }
    // Deliberately inherits the old default-empty committed binding query.
    BleFrameTransport& direct;
};

DeviceInstanceId instance() {
    DeviceInstanceId value;
    for (size_t i = 0U; i < sizeof(value.bytes); ++i) value.bytes[i] = static_cast<uint8_t>(i + 1U);
    return value;
}

bbp2::AuthResultBody authResult(const AuthorizationDecision& decision) {
    bbp2::AuthResultBody result;
    result.method = security::kControllerHmacSha256Method;
    result.status = decision.status; result.payload = decision.responsePayload;
    result.hasPayload = !result.payload.empty();
    return result;
}

struct Fixture {
    Blob blob;
    DeviceAccessStore access;
    CredentialSource source;
    Clock clock;
    Random deviceRandom, clientRandom;
    DeterministicTestCrypto crypto;
    Link link;
    uint8_t rxStorage[536U * BLINKER_BLE_MAX_SESSIONS];
    uint8_t txStorage[536U * BLINKER_BLE_MAX_TX_FRAMES];
    uint8_t packet[20U], plaintext[512U];
    BleFrameTransport direct;
    LegacyDirectObservation legacy;
    security::ControllerAuthSession sessions[1U];
    security::InitialCloudKeyRegistration registration;
    security::ControllerHmacSha256Initiator initiator;
    Installer installer;
    const DeviceInstanceId deviceInstance;
    ControllerCredential used;
    DeviceKey key;
    uint8_t intent[16U], clientNonce[16U], deviceNonce[16U];
    AuthorizationSessionContext session;
    AuthorizationDecision finalDecision;
    RxContext verifiedRx;
    bool receivedVerifiedRecord;

    explicit Fixture(uint32_t now = 100U, bool legacyObservation = false, bool startDirect = true,
        security::InitialCloudKeySetupWindow setup = security::InitialCloudKeySetupWindow())
        : blob(), access(blob), source(access), clock(now), deviceRandom(0x30U), clientRandom(0x10U),
          crypto(), link(), rxStorage(), txStorage(), packet(), plaintext(),
          direct(link, clock, crypto, MutableByteSpan(rxStorage, sizeof(rxStorage)),
            MutableByteSpan(txStorage, sizeof(txStorage)), MutableByteSpan(packet, sizeof(packet)),
            MutableByteSpan(plaintext, sizeof(plaintext))), legacy(direct), sessions(),
          registration(instance(), source, deviceRandom, clock,
            legacyObservation ? static_cast<security::IDirectSecureSessionController&>(legacy)
                              : static_cast<security::IDirectSecureSessionController&>(direct), sessions, 1U, setup),
          initiator(clientRandom), installer(access), deviceInstance(instance()), used(credential()), key(candidateKey()),
          intent(), clientNonce(), deviceNonce(), session(), finalDecision(), verifiedRx(), receivedVerifiedRecord(false) {
        check(access.bootstrapAccess(7U, credential()).ok(), "fixture Admin root failed");
        for (size_t i = 0U; i < sizeof(intent); ++i) intent[i] = static_cast<uint8_t>(0xb0U + i);
        session.sessionId = 91U; // transport zero is the real standalone transport index.
        if (startDirect) {
            direct.setReceiver(&Fixture::receive, this);
            check(direct.start().ok(), "real transport start failed");
            link.connect(session.sessionId);
        }
    }
    static void receive(void* context, ByteView frame, const RxContext& rx) {
        Fixture& self = *static_cast<Fixture*>(context);
        check(frame.size == bbp2::kBaseHeaderSize, "unexpected secure probe frame");
        self.verifiedRx = rx;
        self.receivedVerifiedRecord = true;
    }
    security::DirectSecureContext authContext() const {
        security::DirectSecureContext context;
        context.controllerId = used.id(); context.domain = used.domain;
        context.ownershipGeneration = used.ownershipGeneration;
        context.credentialVersion = used.credentialVersion; context.permissions = used.permissions;
        context.clientNonce = ByteView(clientNonce, sizeof(clientNonce));
        context.deviceNonce = ByteView(deviceNonce, sizeof(deviceNonce));
        return context;
    }
    bool authenticate(bool queued = true, bool tamper = false) {
        uint8_t init[security::kControllerAuthInitPayloadSize] = {};
        ByteView request;
        check(initiator.begin(used, MutableByteSpan(init, sizeof(init)), request).ok(), "real initiator begin failed");
        std::memcpy(clientNonce, request.data + 26U, sizeof(clientNonce));
        AuthorizationDecision decision;
        Result result = registration.authorize(security::kControllerHmacSha256Method, request, session, decision);
        if (!result || decision.status != bbp2::AuthStatus::Continue) return false;
        check(decision.responsePayload.size == security::kControllerAuthChallengePayloadSize, "challenge shape changed");
        std::memcpy(deviceNonce, decision.responsePayload.data + 1U, sizeof(deviceNonce));
        uint8_t proof[security::kControllerAuthProofPayloadSize] = {};
        check(initiator.handleChallenge(authResult(decision), MutableByteSpan(proof, sizeof(proof)), request).ok(),
              "real initiator rejected device HMAC");
        if (tamper) proof[1U] ^= 1U;
        result = registration.authorize(security::kControllerHmacSha256Method, request, session, finalDecision);
        if (!result || finalDecision.status != bbp2::AuthStatus::Authorized) return false;
        if (queued) queueAuthorization();
        return true;
    }
    void queueAuthorization(bool expectTransportCommitted = true) {
        uint8_t bodyStorage[64U] = {}, frame[74U] = {};
        ByteView body;
        check(bbp2::encodeAuthResultBody(authResult(finalDecision), MutableByteSpan(bodyStorage, sizeof(bodyStorage)), body).ok(),
              "final AuthResult encode failed");
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::AuthResult);
        header.flags = bbp2::FlagIsResponse; header.sequence = 3U;
        header.bodyLength = static_cast<uint16_t>(body.size);
        check(bbp2::encodeHeader(header, MutableByteSpan(frame, sizeof(frame))).ok(), "AuthResult frame encode failed");
        std::memcpy(frame + bbp2::kBaseHeaderSize, body.data, body.size);
        SendTarget target; target.kind = SendTargetKind::Session; target.sessionId = session.sessionId;
        check(direct.send(ByteView(frame, bbp2::kBaseHeaderSize + body.size), target).ok(), "raw AuthResult was not queued");
        // Controlled Runtime queue callback, not a claim of Runtime/radio wiring.
        registration.authorizationResultQueued(session);
        check(initiator.handleAuthorization(authResult(finalDecision)).ok(), "real initiator final authorization failed");
        if (!expectTransportCommitted) return;
        check(direct.committedSessionBinding(session.sessionId).size == security::kDirectSecureBindingSize,
              "actual transport was not committed");
        check(constantTimeEqual(direct.committedSessionBinding(session.sessionId), initiator.secureSession().sessionBinding()),
              "real initiator/responder binding mismatch");
        uint8_t probe[bbp2::kBaseHeaderSize] = {}, sealed[64U] = {};
        bbp2::FrameHeader probeHeader;
        probeHeader.kind = static_cast<uint8_t>(bbp2::MessageKind::StateRequest);
        probeHeader.sequence = 1U;
        check(bbp2::encodeHeader(probeHeader, MutableByteSpan(probe, sizeof(probe))).ok(), "probe frame failed");
        ByteView record;
        check(initiator.secureSession().seal(crypto, ByteView(probe, sizeof(probe)), MutableByteSpan(sealed, sizeof(sealed)), record).ok(),
              "real DirectSecure sealing state failed");
        link.injectRecord(record, 1U);
        check(receivedVerifiedRecord && verifiedRx.encrypted && verifiedRx.sessionId == session.sessionId,
              "real transport did not deliver a verified secure context");
    }
    void proof(uint8_t (&output)[32U]) const {
        security::DirectSecureKeyMaterial material;
        check(security::deriveDirectSecureKeyMaterial(used.authenticationSecret(), authContext(), material).ok(),
              "client proof binding derivation failed");
        check(security::computeInitialCloudKeyRegistrationProof(used.authenticationSecret(), deviceInstance, authContext(),
            ByteView(material.sessionBinding, sizeof(material.sessionBinding)), ByteView(intent, sizeof(intent)), key,
            MutableByteSpan(output, sizeof(output))).ok(), "client registration HMAC failed");
        material.clear();
    }
    Result install(ByteView proofBytes) {
        return registration.install(verifiedRx, ByteView(intent, sizeof(intent)), key, proofBytes, installer);
    }
};

void fill(uint8_t* bytes, size_t count, uint8_t seed) {
    for (size_t i = 0U; i < count; ++i) bytes[i] = static_cast<uint8_t>(seed + i);
}

void testIndependentNodeGoldenAndBoundFields() {
    static const uint8_t expected[] = {
        0x2e, 0x5e, 0x81, 0x6a, 0x37, 0x5f, 0x91, 0xe6,
        0x7c, 0x7e, 0xf9, 0x8a, 0x14, 0x76, 0xe1, 0x83,
        0x24, 0xb7, 0xa0, 0xc1, 0x75, 0xe5, 0x1a, 0x11,
        0xee, 0xa3, 0xd8, 0x8c, 0xd7, 0xfa, 0xba, 0x1c,
    };
    // Independently computed with Node crypto.createHmac over the fixed
    // 186-byte public synthetic transcript, not a call to the helper itself.
    for (unsigned variant = 0U; variant <= 13U; ++variant) {
        ControllerCredential owner = credential();
        DeviceInstanceId id = instance();
        DeviceKey key = candidateKey();
        uint8_t client[16U], device[16U], binding[16U], intent[16U], actual[32U] = {};
        fill(client, sizeof(client), 0x10U); fill(device, sizeof(device), 0x30U);
        fill(binding, sizeof(binding), 0x90U); fill(intent, sizeof(intent), 0xb0U);
        security::DirectSecureContext context;
        context.controllerId = owner.id(); context.domain = owner.domain;
        context.ownershipGeneration = owner.ownershipGeneration;
        context.credentialVersion = owner.credentialVersion; context.permissions = owner.permissions;
        context.clientNonce = ByteView(client, sizeof(client)); context.deviceNonce = ByteView(device, sizeof(device));
        switch (variant) {
        case 1U: id.bytes[0] ^= 1U; break;
        case 2U: owner.controllerId[0] ^= 1U; break;
        case 3U: context.domain = ControllerCredentialDomain::Local; break;
        case 4U: ++context.ownershipGeneration; break;
        case 5U: ++context.credentialVersion; break;
        case 6U: context.permissions = kControllerPermissionControl; break;
        case 7U: client[0] ^= 1U; break;
        case 8U: device[0] ^= 1U; break;
        case 9U: binding[0] ^= 1U; break;
        case 10U: intent[0] ^= 1U; break;
        case 11U: key.bytes[0] ^= 1U; break;
        case 12U: owner.secret[0] ^= 1U; break;
        case 13U: context.clientNonce = ByteView(client, sizeof(client) - 1U); break;
        default: break;
        }
        Result result = security::computeInitialCloudKeyRegistrationProof(owner.authenticationSecret(), id, context,
            ByteView(binding, sizeof(binding)), ByteView(intent, sizeof(intent)), key, MutableByteSpan(actual, sizeof(actual)));
        if (variant == 0U) check(result.ok() && std::memcmp(actual, expected, sizeof(expected)) == 0, "Node HMAC golden drifted");
        else check(!result || std::memcmp(actual, expected, sizeof(expected)) != 0, "registration HMAC omitted a bound field");
    }
}

void testOnlyRealCommittedMethod2CanInstallOnce() {
    {
        Fixture f;
        check(f.authenticate(false), "prepared real Method2 fixture failed");
        check(f.direct.committedSessionBinding(f.session.sessionId).empty(), "prepared transport looked committed");
        uint8_t proof[32U]; f.proof(proof);
        RxContext forged;
        forged.sessionId = f.session.sessionId; forged.encrypted = true;
        check(!f.registration.install(forged, ByteView(f.intent, sizeof(f.intent)), f.key, ByteView(proof, sizeof(proof)), f.installer)
            && f.installer.calls == 0U, "public encrypted snapshot minted prepared-session authority");
    }
    {
        Fixture f;
        check(!f.authenticate(true, true), "altered real Method2 HMAC was accepted");
        uint8_t proof[32U] = {}; std::memset(proof, 0x55, sizeof(proof));
        RxContext forged;
        forged.sessionId = f.session.sessionId; forged.encrypted = true;
        check(!f.registration.install(forged, ByteView(f.intent, sizeof(f.intent)), f.key, ByteView(proof, sizeof(proof)), f.installer)
            && f.installer.accepted == 0U, "failed HMAC minted registration authority");
    }
    {
        Fixture f;
        check(f.authenticate(), "committed real Method2 fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        check(f.install(ByteView(proof, sizeof(proof))).ok() && f.installer.accepted == 1U, "committed Admin command was not accepted");
        f.registration.authorizationResultQueued(f.session);
        check(f.direct.committedSessionBinding(f.session.sessionId).size == security::kDirectSecureBindingSize,
              "duplicate queued callback recommitted/retired an active transport");
        check(!f.install(ByteView(proof, sizeof(proof))) && f.installer.calls == 1U,
              "replay or duplicate queued callback restored consumed command authority");
    }
}

void testSharedControlOnlyAndLocalAllNeverBecomeAdmin() {
    for (unsigned role = 0U; role < 3U; ++role) {
        Fixture f;
        const uint32_t permissions = role == 0U ? kControllerPermissionObserve | kControllerPermissionControl
            : role == 1U ? kControllerPermissionControl : kControllerPermissionAll;
        f.used = credential(0x40U, permissions);
        if (role == 0U) check(f.access.installVerified(f.used).ok(), "Shared credential fixture was invalid");
        else {
            if (role == 2U) { f.used.domain = ControllerCredentialDomain::Local; f.used.ownershipGeneration = 0U; }
            bool changed = false;
            check(f.source.additional.install(f.used, changed).ok() && changed, "generic role fixture was invalid");
        }
        // Even when ordinary Method2 can authenticate this role, it never
        // grants the separate first-Cloud-Key permission.
        (void)f.authenticate();
        uint8_t proof[32U] = {}; std::memset(proof, 0x66, sizeof(proof));
        RxContext rx = f.verifiedRx; rx.encrypted = true; rx.sessionId = f.session.sessionId;
        check(!f.registration.install(rx, ByteView(f.intent, sizeof(f.intent)), f.key, ByteView(proof, sizeof(proof)), f.installer)
            && f.installer.accepted == 0U, "non-Admin role obtained first-Key authorization");
    }
}

void testWrongPeerIntentCandidateAndProof() {
    for (unsigned variant = 0U; variant < 7U; ++variant) {
        Fixture f;
        check(f.authenticate(), "mismatch fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        RxContext rx = f.verifiedRx;
        ByteView intent(f.intent, sizeof(f.intent));
        switch (variant) {
        case 0U: ++rx.transportId; break;
        case 1U: ++rx.sessionId; break;
        case 2U: rx.encrypted = false; break;
        case 3U: f.intent[0] ^= 1U; break;
        case 4U: f.key.bytes[0] ^= 1U; break;
        case 5U: proof[0] ^= 1U; break;
        case 6U: intent = ByteView(f.intent, sizeof(f.intent) - 1U); break;
        default: break;
        }
        check(!f.registration.install(rx, intent, f.key, ByteView(proof, sizeof(proof)), f.installer)
            && f.installer.accepted == 0U, "mismatched registration command reached an accepted guard");
    }
}

void testCloseCancelResetAndActualTransportLoss() {
    for (unsigned event = 0U; event < 7U; ++event) {
        Fixture f;
        check(f.authenticate(), "loss fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        switch (event) {
        case 0U: f.registration.sessionClosed(f.session); break;
        case 1U: f.direct.cancel(f.session.sessionId); break;
        case 2U: f.direct.stop(); break;
        case 3U: f.link.disconnect(); break;
        case 4U: f.registration.resetSessions(); break;
        case 5U: f.link.session.connected = false; break; // No callback/poll yet.
        case 6U: f.link.current = BleLinkState::Stopped; break; // No callback/poll yet.
        default: break;
        }
        check(!f.install(ByteView(proof, sizeof(proof))) && f.installer.accepted == 0U,
              "closed/cancelled or actually disconnected transport retained command authority");
    }
    Fixture unsupported(100U, true);
    check(unsupported.authenticate(false), "legacy observation Method2 fixture failed");
    unsupported.queueAuthorization(false);
    uint8_t proof[32U]; unsupported.proof(proof);
    RxContext forged;
    forged.encrypted = true; forged.sessionId = unsupported.session.sessionId;
    check(!unsupported.registration.install(forged, ByteView(unsupported.intent, sizeof(unsupported.intent)), unsupported.key,
        ByteView(proof, sizeof(proof)), unsupported.installer) && unsupported.installer.accepted == 0U,
        "default-empty committed binding query was treated as permission");
}

void testNewHandshakeRetiresOldProofAndRootVersion() {
    Fixture f;
    check(f.authenticate(), "rekey fixture failed");
    uint8_t oldProof[32U]; f.proof(oldProof);
    f.registration.resetSessions();
    f.initiator.reset();
    check(f.authenticate(false), "new actual Method2 prepare failed");
    RxContext oldRx = f.verifiedRx;
    check(!f.registration.install(oldRx, ByteView(f.intent, sizeof(f.intent)), f.key, ByteView(oldProof, sizeof(oldProof)), f.installer)
        && f.installer.accepted == 0U, "new prepared handshake accepted an old committed command proof");

    Fixture root;
    check(root.authenticate(), "root-version fixture failed");
    uint8_t proof[32U]; root.proof(proof);
    ControllerCredential changed = credential();
    changed.credentialVersion = 2U; changed.secret[0] ^= 1U;
    check(root.access.rotateVerified(1U, changed).ok(), "actual Admin rotation fixture failed");
    check(!root.install(ByteView(proof, sizeof(proof))) && root.installer.accepted == 0U,
          "old authenticated Admin version authorized a changed actual root");
}

void testDeadlineDuplicateQueueAndMonotonicWrap() {
    for (unsigned timing = 0U; timing < 4U; ++timing) {
        Fixture f(timing < 2U ? 100U : 0xfffffff0U);
        check(f.authenticate(), "deadline fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        if (timing == 0U) {
            f.clock.now += 119999U;
            f.registration.authorizationResultQueued(f.session);
            check(f.direct.committedSessionBinding(f.session.sessionId).size == security::kDirectSecureBindingSize,
                  "duplicate queued callback modified live transport");
            ++f.clock.now;
        } else f.clock.now += timing == 2U ? 119999U : 120000U;
        const Result result = f.install(ByteView(proof, sizeof(proof)));
        if (timing == 2U) check(result.ok() && f.installer.accepted == 1U, "valid wrapped monotonic deadline was rejected");
        else check(!result && f.installer.accepted == 0U, "deadline or repeated queued callback extended authorization");
    }
}

struct Hook {
    Fixture* fixture;
    const uint8_t* proof;
    unsigned mode;
    bool reentryRejected;
};

void interfere(void* context) {
    Hook& hook = *static_cast<Hook*>(context);
    Fixture& f = *hook.fixture;
    if (hook.mode == 0U) f.clock.now += 120000U;
    else if (hook.mode == 1U) f.registration.resetSessions();
    else if (hook.mode == 3U) f.registration.sessionClosed(f.session);
    else {
        const uint32_t calls = f.installer.calls;
        hook.reentryRejected = !f.install(ByteView(hook.proof, 32U)) && f.installer.calls == calls;
    }
}

void testLateGuardDeadlineCancellationAndReentry() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        Fixture f;
        check(f.authenticate(), "late guard fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        Hook hook = { &f, proof, mode, false };
        f.installer.beforeGuard = &interfere; f.installer.hookContext = &hook;
        const Result result = f.install(ByteView(proof, sizeof(proof)));
        if (mode == 2U) check(result.ok() && hook.reentryRejected && f.installer.accepted == 1U && f.installer.calls == 1U,
                             "reentrant installer call bypassed the command's single owner");
        else check(!result && f.installer.accepted == 0U, "late guard did not recheck expiry/cancellation at commit boundary");
    }
}

void testKnownAndUnknownInstallerFailureNeverRestoreConsumption() {
    for (unsigned unknown = 0U; unknown < 2U; ++unknown) {
        Fixture f;
        check(f.authenticate(), "failure consumption fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        f.installer.outcome = unknown ? ErrorCode::InternalError : ErrorCode::StateConflict;
        const Result result = f.install(ByteView(proof, sizeof(proof)));
        check(result.code() == f.installer.outcome && f.installer.accepted == 1U && f.installer.calls == 1U,
              "installer failure fixture did not reach the actual authorization guard");
        f.installer.outcome = ErrorCode::Ok;
        f.registration.authorizationResultQueued(f.session);
        check(!f.install(ByteView(proof, sizeof(proof))) && f.installer.calls == 1U,
              "known/UNKNOWN installer failure restored a consumed command");
    }
}

void testAfterGuardCancellationPreservesStartedCommitOutcome() {
    for (unsigned failure = 0U; failure < 2U; ++failure) {
        Fixture f;
        check(f.authenticate(), "after-guard fixture failed");
        uint8_t proof[32U]; f.proof(proof);
        Hook hook = { &f, proof, failure == 0U ? 1U : 3U, false };
        f.installer.outcome = failure == 0U ? ErrorCode::Ok : ErrorCode::WouldBlock;
        f.installer.afterGuard = &interfere; f.installer.hookContext = &hook;
        const Result result = f.install(ByteView(proof, sizeof(proof)));
        check(result.code() == f.installer.outcome && f.installer.accepted == 1U && f.installer.calls == 1U,
              "after-guard cancellation rewrote an already started commit outcome");
        f.installer.afterGuard = nullptr;
        check(!f.install(ByteView(proof, sizeof(proof))) && f.installer.calls == 1U,
              "after-guard cancellation restored command authority");
    }
}

} // namespace

int main() {
    testIndependentNodeGoldenAndBoundFields();
    testOnlyRealCommittedMethod2CanInstallOnce();
    testSharedControlOnlyAndLocalAllNeverBecomeAdmin();
    testWrongPeerIntentCandidateAndProof();
    testCloseCancelResetAndActualTransportLoss();
    testNewHandshakeRetiresOldProofAndRootVersion();
    testDeadlineDuplicateQueueAndMonotonicWrap();
    testLateGuardDeadlineCancellationAndReentry();
    testKnownAndUnknownInstallerFailureNeverRestoreConsumption();
    testAfterGuardCancellationPreservesStartedCommitOutcome();
    std::cout << "Initial Cloud Key registration authority: 10 finite groups passed; no durable/radio claim" << std::endl;
    return 0;
}
