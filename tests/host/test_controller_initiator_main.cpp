#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>
#include <BlinkerV2/security/ControllerHmacSha256Initiator.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Controller initiator check failed at line "         \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class SequenceRandom final : public blinker::IRandom {
public:
    explicit SequenceRandom(uint8_t first, bool zero = false)
        : next(first), allZero(zero), calls(0U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        if (output.data == nullptr) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = allZero ? 0U : next++;
        }
        return blinker::Result::success();
    }

    uint8_t next;
    bool allZero;
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

blinker::ControllerCredential makeCredential(
    blinker::ControllerCredentialDomain domain =
        blinker::ControllerCredentialDomain::Local) {
    blinker::ControllerCredential credential;
    credential.domain = domain;
    credential.ownershipGeneration =
        domain == blinker::ControllerCredentialDomain::Local ? 0U : 23U;
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

blinker::bbp2::AuthResultBody makeResult(
    const blinker::AuthorizationDecision& decision) {
    blinker::bbp2::AuthResultBody result;
    result.method = blinker::security::kControllerHmacSha256Method;
    result.status = decision.status;
    result.payload = decision.responsePayload;
    result.hasPayload = !decision.responsePayload.empty();
    return result;
}

void testExactHandshakeAndDirectBinding() {
    static const uint8_t expectedAppProof[] = {
        0xF6, 0xA4, 0xBC, 0x63, 0xB6, 0x76, 0xDD, 0xF5,
        0x65, 0x95, 0xE3, 0xF2, 0x5A, 0x23, 0xD5, 0xBF,
        0x3B, 0xFB, 0x56, 0x9B, 0x96, 0xE3, 0xBD, 0xF9,
        0x05, 0x2A, 0xB8, 0xB7, 0x45, 0x2E, 0x45, 0x06
    };

    SequenceRandom clientRandom(0x20U);
    blinker::security::ControllerHmacSha256Initiator initiator(clientRandom);
    volatile size_t initiatorSize = sizeof(initiator);
    CHECK_TRUE(initiatorSize <= 224U);
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

    uint8_t requestStorage[
        blinker::security::kControllerAuthInitPayloadSize] = {};
    blinker::ByteView request;
    CHECK_TRUE(initiator.begin(
        source.credential,
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)),
        request));
    CHECK_EQ(
        blinker::security::kControllerAuthInitPayloadSize,
        request.size);
    CHECK_EQ(
        static_cast<uint8_t>(blinker::security::ControllerAuthPayloadInit),
        request.data[0]);
    CHECK_EQ(static_cast<uint8_t>(0x20U), request.data[26U]);
    CHECK_EQ(static_cast<uint8_t>(0x2FU), request.data[41U]);

    blinker::AuthorizationSessionContext session;
    session.transportId = 1U;
    session.sessionId = 91U;
    session.encrypted = false;
    blinker::AuthorizationDecision decision;
    CHECK_TRUE(responder.authorize(
        blinker::security::kControllerHmacSha256Method,
        request,
        session,
        decision));
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Continue);

    uint8_t proofStorage[
        blinker::security::kControllerAuthProofPayloadSize] = {};
    blinker::ByteView proof;
    CHECK_TRUE(initiator.handleChallenge(
        makeResult(decision),
        blinker::MutableByteSpan(proofStorage, sizeof(proofStorage)),
        proof));
    CHECK_EQ(
        blinker::security::kControllerAuthProofPayloadSize,
        proof.size);
    CHECK_TRUE(std::memcmp(
        proof.data + 1U,
        expectedAppProof,
        sizeof(expectedAppProof)) == 0);

    CHECK_TRUE(responder.authorize(
        blinker::security::kControllerHmacSha256Method,
        proof,
        session,
        decision));
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Authorized);
    responder.authorizationResultQueued(session);
    CHECK_EQ(session.sessionId, responderDirect.activeSessionId);
    CHECK_TRUE(initiator.handleAuthorization(makeResult(decision)));
    CHECK_TRUE(initiator.authorized());
    CHECK_TRUE(responderDirect.session.ready());
    CHECK_TRUE(blinker::constantTimeEqual(
        initiator.secureSession().sessionBinding(),
        responderDirect.session.sessionBinding()));

    initiator.reset();
    CHECK_TRUE(!initiator.authorized());
    CHECK_TRUE(!initiator.secureSession().ready());
    responder.sessionClosed(session);
}

void testGatewayCredentialProjection() {
    blinker::GatewayAccessRecord record;
    record.storageRevision = 3U;
    record.deliveryRevision = 3U;
    record.operationId[0] = 1U;
    const char childId[] = "ble_child_1";
    record.childLogicalDeviceIdSize = sizeof(childId) - 1U;
    std::memcpy(
        record.childLogicalDeviceId,
        childId,
        record.childLogicalDeviceIdSize);
    record.childDeviceInstanceId[0] = 2U;
    record.accessEpoch = 23U;
    record.credentialVersion = 7U;
    record.permissions = blinker::gateway::kGatewayPermissions;
    record.expiresAtUnixSeconds = 1000U;
    record.activePresence.version = 1U;
    record.activePresence.key[0] = 1U;
    for (size_t index = 0U; index < sizeof(record.controllerId); ++index) {
        record.controllerId[index] = static_cast<uint8_t>(0x10U + index);
    }
    for (size_t index = 0U; index < sizeof(record.secret); ++index) {
        record.secret[index] = static_cast<uint8_t>(0xA0U + index);
    }

    blinker::ControllerCredential credential;
    CHECK_TRUE(blinker::makeGatewayControllerCredential(
        record, 999U, credential));
    CHECK_TRUE(
        credential.domain ==
        blinker::ControllerCredentialDomain::PlatformAccess);
    CHECK_EQ(record.accessEpoch, credential.ownershipGeneration);
    CHECK_EQ(record.credentialVersion, credential.credentialVersion);
    CHECK_EQ(record.permissions, credential.permissions);
    CHECK_TRUE(blinker::constantTimeEqual(
        credential.authenticationSecret(),
        blinker::ByteView(record.secret, sizeof(record.secret))));

    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        blinker::makeGatewayControllerCredential(
            record, 1000U, credential).code());
    CHECK_EQ(static_cast<uint32_t>(0U), credential.credentialVersion);
    blinker::clearGatewayAccessRecord(record);
}

void testFailClosedTransitions() {
    CredentialSource source;
    source.credential = makeCredential(
        blinker::ControllerCredentialDomain::PlatformAccess);
    SequenceRandom clientRandom(0x20U);
    blinker::security::ControllerHmacSha256Initiator initiator(clientRandom);
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
    session.transportId = 2U;
    session.sessionId = 17U;

    uint8_t init[blinker::security::kControllerAuthInitPayloadSize] = {};
    uint8_t proof[blinker::security::kControllerAuthProofPayloadSize] = {};
    blinker::ByteView request;
    blinker::AuthorizationDecision decision;
    CHECK_TRUE(initiator.begin(
        source.credential,
        blinker::MutableByteSpan(init, sizeof(init)),
        request));
    CHECK_TRUE(responder.authorize(
        blinker::security::kControllerHmacSha256Method,
        request,
        session,
        decision));
    uint8_t badChallenge[
        blinker::security::kControllerAuthChallengePayloadSize];
    std::memcpy(
        badChallenge,
        decision.responsePayload.data,
        sizeof(badChallenge));
    badChallenge[sizeof(badChallenge) - 1U] ^= 1U;
    blinker::bbp2::AuthResultBody bad = makeResult(decision);
    bad.payload = blinker::ByteView(badChallenge, sizeof(badChallenge));
    blinker::ByteView encoded;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        initiator.handleChallenge(
            bad,
            blinker::MutableByteSpan(proof, sizeof(proof)),
            encoded).code());
    CHECK_TRUE(
        initiator.state() ==
        blinker::security::ControllerAuthInitiatorState::Idle);
    CHECK_TRUE(!initiator.secureSession().ready());

    responder.resetSessions();
    CHECK_TRUE(initiator.begin(
        source.credential,
        blinker::MutableByteSpan(init, sizeof(init)),
        request));
    CHECK_TRUE(responder.authorize(
        blinker::security::kControllerHmacSha256Method,
        request,
        session,
        decision));
    CHECK_TRUE(initiator.handleChallenge(
        makeResult(decision),
        blinker::MutableByteSpan(proof, sizeof(proof)),
        encoded));
    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        initiator.handleChallenge(
            makeResult(decision),
            blinker::MutableByteSpan(proof, sizeof(proof)),
            encoded).code());
    CHECK_TRUE(!initiator.secureSession().ready());

    responder.resetSessions();
    CHECK_TRUE(initiator.begin(
        source.credential,
        blinker::MutableByteSpan(init, sizeof(init)),
        request));
    CHECK_TRUE(responder.authorize(
        blinker::security::kControllerHmacSha256Method,
        request,
        session,
        decision));
    CHECK_TRUE(initiator.handleChallenge(
        makeResult(decision),
        blinker::MutableByteSpan(proof, sizeof(proof)),
        encoded));
    blinker::bbp2::AuthResultBody rejected;
    rejected.method = blinker::security::kControllerHmacSha256Method;
    rejected.status = blinker::bbp2::AuthStatus::Rejected;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        initiator.handleAuthorization(rejected).code());
    CHECK_TRUE(
        initiator.state() ==
        blinker::security::ControllerAuthInitiatorState::Idle);

    SequenceRandom zeroRandom(0U, true);
    blinker::security::ControllerHmacSha256Initiator zeroInitiator(zeroRandom);
    CHECK_EQ(
        blinker::ErrorCode::ProtocolError,
        zeroInitiator.begin(
            source.credential,
            blinker::MutableByteSpan(init, sizeof(init)),
            request).code());
    CHECK_TRUE(!zeroInitiator.secureSession().ready());
}

} // namespace

int main() {
    testExactHandshakeAndDirectBinding();
    testGatewayCredentialProjection();
    testFailClosedTransitions();
    std::cout << "Controller initiator tests passed" << std::endl;
    return 0;
}
