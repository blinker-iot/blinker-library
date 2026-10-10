#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Controller authorization check failed at line "     \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

class SequenceRandom final : public blinker::IRandom {
public:
    explicit SequenceRandom(uint8_t first) : next(first), calls(0U) {}

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
    CredentialSource()
        : credential(),
          present(true),
          failure(blinker::ErrorCode::Ok),
          loads(0U) {}

    blinker::Result loadActive(
        blinker::ByteView controllerId,
        blinker::ControllerCredentialDomain domain,
        uint32_t generation,
        blinker::ControllerCredential& output) override {
        ++loads;
        if (failure != blinker::ErrorCode::Ok) {
            return blinker::Result::failure(failure);
        }
        if (!present || controllerId.size != blinker::kControllerIdSize ||
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
    bool present;
    blinker::ErrorCode failure;
    uint32_t loads;
};

class DirectSessions final
    : public blinker::security::IDirectSecureSessionController {
public:
    DirectSessions()
        : preparedSession(0U), committedSession(0U), cancelledSession(0U) {}

    blinker::Result prepare(
        uint32_t sessionId,
        blinker::ByteView secret,
        const blinker::security::DirectSecureContext& context) override {
        if (sessionId == 0U ||
            secret.size != blinker::kControllerCredentialSecretSize ||
            context.controllerId.size != blinker::kControllerIdSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        preparedSession = sessionId;
        return blinker::Result::success();
    }

    void commit(uint32_t sessionId) override {
        committedSession = sessionId;
    }

    void cancel(uint32_t sessionId) override {
        cancelledSession = sessionId;
    }

    uint32_t preparedSession;
    uint32_t committedSession;
    uint32_t cancelledSession;
};

blinker::ControllerCredential makeCredential() {
    blinker::ControllerCredential credential;
    credential.domain = blinker::ControllerCredentialDomain::Local;
    credential.ownershipGeneration = 0U;
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

void makeInit(
    const blinker::ControllerCredential& credential,
    uint8_t firstNonceByte,
    uint8_t output[blinker::security::kControllerAuthInitPayloadSize]) {
    output[0] = blinker::security::ControllerAuthPayloadInit;
    std::memcpy(
        output + 1U,
        credential.controllerId,
        blinker::kControllerIdSize);
    output[1U + blinker::kControllerIdSize] =
        static_cast<uint8_t>(credential.domain);
    writeU32(
        output + 1U + blinker::kControllerIdSize + 1U,
        credential.ownershipGeneration);
    writeU32(
        output + 1U + blinker::kControllerIdSize + 1U + 4U,
        credential.credentialVersion);
    for (size_t index = 0U;
         index < blinker::security::kControllerAuthNonceSize;
         ++index) {
        output[1U + blinker::kControllerIdSize + 1U + 8U + index] =
            static_cast<uint8_t>(firstNonceByte + index);
    }
}

void testExactProofAndHandshake() {
    static const uint8_t expectedDeviceProof[] = {
        0x25, 0x2E, 0x60, 0xB1, 0xDB, 0xDD, 0xC1, 0x0B,
        0xD4, 0xFF, 0x3C, 0x7F, 0x10, 0x4D, 0x92, 0x44,
        0x34, 0xA6, 0xA8, 0x2B, 0xBD, 0x6D, 0x73, 0xD3,
        0x13, 0xD9, 0x12, 0x8F, 0xB1, 0x37, 0xA1, 0x35
    };
    static const uint8_t expectedAppProof[] = {
        0xF6, 0xA4, 0xBC, 0x63, 0xB6, 0x76, 0xDD, 0xF5,
        0x65, 0x95, 0xE3, 0xF2, 0x5A, 0x23, 0xD5, 0xBF,
        0x3B, 0xFB, 0x56, 0x9B, 0x96, 0xE3, 0xBD, 0xF9,
        0x05, 0x2A, 0xB8, 0xB7, 0x45, 0x2E, 0x45, 0x06
    };

    CredentialSource source;
    source.credential = makeCredential();
    SequenceRandom random(0x40U);
    DirectSessions direct;
    blinker::security::ControllerAuthSession sessions[2];
    blinker::security::ControllerHmacSha256Authorizer authorizer(
        source,
        random,
        direct,
        sessions,
        2U);
    CHECK_EQ(static_cast<size_t>(1U), authorizer.methodCount());
    CHECK_EQ(
        blinker::security::kControllerHmacSha256Method,
        authorizer.methodAt(0U));
    CHECK_EQ(static_cast<uint16_t>(0U), authorizer.methodAt(1U));
    volatile size_t sessionSize =
        sizeof(blinker::security::ControllerAuthSession);
    CHECK_TRUE(sessionSize <= 68U);

    uint8_t init[blinker::security::kControllerAuthInitPayloadSize];
    makeInit(source.credential, 0x20U, init);
    blinker::AuthorizationSessionContext session;
    session.transportId = 1U;
    session.sessionId = 77U;
    session.encrypted = true;
    blinker::AuthorizationDecision decision;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Continue);
    CHECK_EQ(static_cast<uint32_t>(0U), decision.permissions);
    CHECK_EQ(
        blinker::security::kControllerAuthChallengePayloadSize,
        decision.responsePayload.size);
    CHECK_EQ(
        static_cast<uint8_t>(
            blinker::security::ControllerAuthPayloadChallenge),
        decision.responsePayload.data[0]);
    for (size_t index = 0U;
         index < blinker::security::kControllerAuthNonceSize;
         ++index) {
        CHECK_EQ(
            static_cast<uint8_t>(0x40U + index),
            decision.responsePayload.data[1U + index]);
    }
    CHECK_TRUE(std::memcmp(
                   decision.responsePayload.data +
                       1U + blinker::security::kControllerAuthNonceSize + 4U,
                   expectedDeviceProof,
                   sizeof(expectedDeviceProof)) == 0);
    CHECK_EQ(static_cast<uint32_t>(1U), random.calls);

    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Continue);
    CHECK_EQ(static_cast<uint32_t>(1U), random.calls);

    uint8_t differentInit[sizeof(init)];
    std::memcpy(differentInit, init, sizeof(init));
    differentInit[sizeof(differentInit) - 1U] ^= 1U;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(differentInit, sizeof(differentInit)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Rejected);
    CHECK_EQ(static_cast<uint32_t>(1U), random.calls);

    uint8_t proof[blinker::security::kControllerAuthProofPayloadSize];
    proof[0] = blinker::security::ControllerAuthPayloadProof;
    std::memcpy(proof + 1U, expectedAppProof, sizeof(expectedAppProof));
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(proof, sizeof(proof)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Authorized);
    CHECK_EQ(source.credential.permissions, decision.permissions);
    CHECK_TRUE(decision.responsePayload.empty());
    CHECK_EQ(session.sessionId, direct.preparedSession);
    authorizer.authorizationResultQueued(session);
    CHECK_EQ(session.sessionId, direct.committedSession);

    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(proof, sizeof(proof)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Rejected);

}

void testRotationRejectionAndInputGuards() {
    CredentialSource source;
    source.credential = makeCredential();
    SequenceRandom random(0x40U);
    DirectSessions direct;
    blinker::security::ControllerAuthSession sessions[1];
    blinker::security::ControllerHmacSha256Authorizer authorizer(
        source,
        random,
        direct,
        sessions,
        1U);
    blinker::AuthorizationSessionContext session;
    session.transportId = 2U;
    session.sessionId = 9U;
    session.encrypted = true;
    uint8_t init[blinker::security::kControllerAuthInitPayloadSize];
    makeInit(source.credential, 0x20U, init);
    blinker::AuthorizationDecision decision;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());

    uint8_t proof[blinker::security::kControllerAuthProofPayloadSize] = {};
    proof[0] = blinker::security::ControllerAuthPayloadProof;
    // A repeated Init must notice a rotation and release the stale slot.
    source.credential.credentialVersion = 8U;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Rejected);
    CHECK_TRUE(!sessions[0].occupied);

    source.credential.credentialVersion = 7U;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Continue);
    source.credential.credentialVersion = 8U;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(proof, sizeof(proof)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Rejected);

    source.credential.credentialVersion = 7U;
    session.encrypted = false;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        authorizer.authorize(
            blinker::security::kControllerHmacSha256Method,
            blinker::ByteView(init, sizeof(init)),
            session,
            decision)
            .code());
    session.encrypted = true;
    source.present = false;
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Rejected);
    source.present = true;
    source.failure = blinker::ErrorCode::InvalidEncoding;
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        authorizer.authorize(
            blinker::security::kControllerHmacSha256Method,
            blinker::ByteView(init, sizeof(init)),
            session,
            decision)
            .code());

    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        authorizer.authorize(
            blinker::security::kControllerHmacSha256Method,
            blinker::ByteView(init, sizeof(init) - 1U),
            session,
            decision)
            .code());
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        authorizer.authorize(
            99U,
            blinker::ByteView(init, sizeof(init)),
            session,
            decision)
            .code());
}

void testDirectSecureBootstrapPolicy() {
    CredentialSource source;
    source.credential = makeCredential();
    SequenceRandom random(0x40U);
    DirectSessions direct;
    blinker::security::ControllerAuthSession sessions[1];
    blinker::security::ControllerHmacSha256Authorizer authorizer(
        source,
        random,
        direct,
        sessions,
        1U,
        blinker::security::ControllerAuthTransportPolicy::EstablishDirectSecure);
    blinker::AuthorizationSessionContext session;
    session.transportId = 2U;
    session.sessionId = 10U;
    session.encrypted = false;
    uint8_t init[blinker::security::kControllerAuthInitPayloadSize];
    makeInit(source.credential, 0x20U, init);
    blinker::AuthorizationDecision decision;

    CHECK_TRUE(authorizer.establishesSecureTransport());
    CHECK_TRUE(authorizer.authorize(
                   blinker::security::kControllerHmacSha256Method,
                   blinker::ByteView(init, sizeof(init)),
                   session,
                   decision)
                   .ok());
    CHECK_TRUE(decision.status == blinker::bbp2::AuthStatus::Continue);
}

} // namespace

int main() {
    testExactProofAndHandshake();
    testRotationRejectionAndInputGuards();
    testDirectSecureBootstrapPolicy();
    std::cout << "controller authorization tests passed" << std::endl;
    return 0;
}
