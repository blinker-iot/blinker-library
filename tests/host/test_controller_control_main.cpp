#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/protocol/bbp2/Messages.h>
#include <BlinkerV2/provisioning/ControllerControlEndpoint.h>
#include <BlinkerV2/provisioning/ControllerControlCoordinator.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

static_assert(
    static_cast<size_t>(blinker::kControllerGrantMaxEncodedSize) ==
        static_cast<size_t>(blinker::bbp2::kControllerGrantMaximumSize),
    "Controller grant bounds drifted between contract and BBP/2");
static_assert(
    static_cast<size_t>(blinker::kControllerReceiptMaxEncodedSize) ==
        static_cast<size_t>(blinker::bbp2::kControllerMutationReceiptMaximumSize),
    "Controller receipt bounds drifted between contract and BBP/2");

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Controller control check failed at line "           \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

blinker::DeviceInstanceId makeDeviceInstanceId() {
    blinker::DeviceInstanceId value;
    for (size_t index = 0U; index < blinker::kDeviceInstanceIdSize;
         ++index) {
        value.bytes[index] = static_cast<uint8_t>(0x10U + index);
    }
    return value;
}

class AccessEpochSource final : public blinker::IAccessEpochSource {
public:
    AccessEpochSource() : epoch(5U), failure(blinker::ErrorCode::Ok) {}

    blinker::Result loadAccessEpoch(uint32_t& output) override {
        if (failure != blinker::ErrorCode::Ok) {
            return blinker::Result::failure(failure);
        }
        output = epoch;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::PlainFlash;
    }

    uint32_t epoch;
    blinker::ErrorCode failure;
};

class CredentialStore final : public blinker::IControllerCredentialStore {
public:
    CredentialStore()
        : credential(),
          present(false),
          installs(0U),
          rotates(0U),
          revokes(0U) {}

    ~CredentialStore() override {
        blinker::clearControllerCredential(credential);
    }

    blinker::Result loadActive(
        blinker::ByteView controllerId,
        blinker::ControllerCredentialDomain domain,
        uint32_t generation,
        blinker::ControllerCredential& output) override {
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

    blinker::Result installVerified(
        const blinker::ControllerCredential& value) override {
        ++installs;
        if (present) {
            return blinker::sameControllerCredential(credential, value)
                       ? blinker::Result::success()
                       : blinker::Result::failure(
                             blinker::ErrorCode::StateConflict);
        }
        credential = value;
        present = true;
        return blinker::Result::success();
    }

    blinker::Result rotateVerified(
        uint32_t expectedVersion,
        const blinker::ControllerCredential& value) override {
        ++rotates;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (blinker::sameControllerCredential(credential, value)) {
            return blinker::Result::success();
        }
        if (credential.credentialVersion != expectedVersion) {
            return blinker::Result::failure(
                blinker::ErrorCode::SequenceConflict);
        }
        if (std::memcmp(
                credential.controllerId,
                value.controllerId,
                blinker::kControllerIdSize) != 0 ||
            credential.domain != value.domain ||
            credential.ownershipGeneration != value.ownershipGeneration) {
            return blinker::Result::failure(
                blinker::ErrorCode::StateConflict);
        }
        credential = value;
        return blinker::Result::success();
    }

    blinker::Result revokeVerified(
        blinker::ByteView controllerId,
        blinker::ControllerCredentialDomain domain,
        uint32_t generation,
        uint32_t expectedVersion) override {
        ++revokes;
        if (!present) return blinker::Result::success();
        if (controllerId.size != blinker::kControllerIdSize ||
            std::memcmp(
                controllerId.data,
                credential.controllerId,
                blinker::kControllerIdSize) != 0 ||
            domain != credential.domain ||
            generation != credential.ownershipGeneration) {
            return blinker::Result::failure(
                blinker::ErrorCode::StateConflict);
        }
        if (expectedVersion != credential.credentialVersion) {
            return blinker::Result::failure(
                blinker::ErrorCode::SequenceConflict);
        }
        blinker::clearControllerCredential(credential);
        present = false;
        return blinker::Result::success();
    }

    blinker::Result clearAll() override {
        blinker::clearControllerCredential(credential);
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

    blinker::ControllerCredential credential;
    bool present;
    uint32_t installs;
    uint32_t rotates;
    uint32_t revokes;
};

class SignatureVerifier final : public blinker::IServerSignatureVerifier {
public:
    SignatureVerifier()
        : expectedDigest(),
          expected(false),
          reject(false),
          calls(0U) {}

    void expect(const blinker::ControllerGrant& grant) {
        uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
        blinker::ByteView transcript;
        CHECK_TRUE(blinker::encodeControllerGrantTranscript(
                       grant,
                       blinker::MutableByteSpan(
                           workspace,
                           sizeof(workspace)),
                       transcript)
                       .ok());
        CHECK_TRUE(blinker::sha256(
                       transcript,
                       blinker::MutableByteSpan(
                           expectedDigest,
                           sizeof(expectedDigest)))
                       .ok());
        expected = true;
        blinker::secureZero(blinker::MutableByteSpan(
            workspace,
            sizeof(workspace)));
    }

    blinker::Result verifyDigest(
        uint32_t keyId,
        blinker::ServerSignatureAlgorithm algorithm,
        blinker::ByteView digest,
        blinker::ByteView signature) override {
        ++calls;
        if (reject || !expected || keyId != 7U ||
            algorithm !=
                blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw ||
            signature.size != blinker::kControllerGrantSignatureSize ||
            !blinker::constantTimeEqual(
                digest,
                blinker::ByteView(
                    expectedDigest,
                    sizeof(expectedDigest)))) {
            return blinker::Result::failure(
                blinker::ErrorCode::AuthenticationRequired);
        }
        return blinker::Result::success();
    }

    uint8_t expectedDigest[blinker::kSha256Size];
    bool expected;
    bool reject;
    uint32_t calls;
};

class SequenceRandom final : public blinker::IRandom {
public:
    SequenceRandom() : next(1U), calls(0U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        if (output.data == nullptr && output.size != 0U) {
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

struct Fixture {
    blinker::DeviceInstanceId deviceInstanceId;
    AccessEpochSource accessEpoch;
    CredentialStore credentials;
    SignatureVerifier signatures;
    blinker::ControllerGrantVerifier grantVerifier;
    blinker::ControllerControlCoordinator coordinator;
    uint8_t grantId[blinker::kControllerGrantIdSize];
    uint8_t controllerId[blinker::kControllerIdSize];
    uint8_t controlNonce[blinker::kControllerControlNonceSize];
    uint8_t secret[blinker::kControllerCredentialSecretSize];
    uint8_t secretDigest[blinker::kControllerSecretDigestSize];
    uint8_t signature[blinker::kControllerGrantSignatureSize];

    Fixture()
        : deviceInstanceId(makeDeviceInstanceId()),
          accessEpoch(),
          credentials(),
          signatures(),
          grantVerifier(signatures),
          coordinator(
              deviceInstanceId,
              accessEpoch,
              credentials,
              grantVerifier),
          grantId(),
          controllerId(),
          controlNonce(),
          secret(),
          secretDigest(),
          signature() {
        for (size_t index = 0U; index < blinker::kDeviceInstanceIdSize;
             ++index) {
            deviceInstanceId.bytes[index] =
                static_cast<uint8_t>(0x10U + index);
            grantId[index] = static_cast<uint8_t>(0x20U + index);
            controllerId[index] = static_cast<uint8_t>(0x30U + index);
            controlNonce[index] = static_cast<uint8_t>(0x40U + index);
        }
        for (size_t index = 0U; index < sizeof(secret); ++index) {
            secret[index] = static_cast<uint8_t>(0x80U + index);
        }
        for (size_t index = 0U; index < sizeof(signature); ++index) {
            signature[index] = static_cast<uint8_t>(0xC0U + index);
        }
        refreshSecretDigest();
    }

    ~Fixture() {
        blinker::secureZero(
            blinker::MutableByteSpan(secret, sizeof(secret)));
        blinker::secureZero(
            blinker::MutableByteSpan(secretDigest, sizeof(secretDigest)));
    }

    void refreshSecretDigest() {
        CHECK_TRUE(blinker::sha256(
                       blinker::ByteView(secret, sizeof(secret)),
                       blinker::MutableByteSpan(
                           secretDigest,
                           sizeof(secretDigest)))
                       .ok());
    }

    blinker::ControllerGrant grant(
        blinker::ControllerMutationOperation operation,
        uint32_t expectedVersion,
        uint32_t version) {
        blinker::ControllerGrant body;
        body.operation = operation;
        body.grantId = blinker::ByteView(grantId, sizeof(grantId));
        body.deviceInstanceId = blinker::ByteView(
            deviceInstanceId.bytes,
            sizeof(deviceInstanceId.bytes));
        body.ownershipGeneration = accessEpoch.epoch;
        body.controllerId =
            blinker::ByteView(controllerId, sizeof(controllerId));
        body.expectedCredentialVersion = expectedVersion;
        body.credentialVersion = version;
        body.permissions =
            operation == blinker::ControllerMutationOperation::Revoke
                ? 0U
                : blinker::kControllerPermissionObserve |
                      blinker::kControllerPermissionControl;
        body.secretDigest = blinker::ByteView(
            secretDigest,
            sizeof(secretDigest));
        body.controlNonce =
            blinker::ByteView(controlNonce, sizeof(controlNonce));
        body.serverKeyId = 7U;
        body.signatureAlgorithm =
            blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
        body.signature = blinker::ByteView(signature, sizeof(signature));
        return body;
    }

    blinker::ByteView encodeGrant(
        blinker::ControllerGrant& body,
        uint8_t* output,
        size_t outputSize) {
        signatures.expect(body);
        blinker::ByteView encoded;
        CHECK_TRUE(blinker::encodeControllerGrant(
                       body,
                       blinker::MutableByteSpan(output, outputSize),
                       encoded)
                       .ok());
        return encoded;
    }
};

void testInstallRotateRevoke() {
    Fixture fixture;
    CHECK_TRUE(fixture.coordinator.beginControlWindow(blinker::ByteView(
                   fixture.controlNonce,
                   sizeof(fixture.controlNonce)))
                   .ok());
    uint8_t grantBuffer[blinker::kControllerGrantMaxEncodedSize] = {};
    uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
    uint8_t receiptBuffer[blinker::kControllerReceiptMaxEncodedSize] = {};
    blinker::ControllerGrant install = fixture.grant(
        blinker::ControllerMutationOperation::Install,
        0U,
        1U);
    blinker::ByteView encodedGrant = fixture.encodeGrant(
        install,
        grantBuffer,
        sizeof(grantBuffer));
    blinker::ByteView receipt;
    CHECK_TRUE(fixture.coordinator.apply(
                   encodedGrant,
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt)
                   .ok());
    CHECK_TRUE(fixture.credentials.present);
    CHECK_EQ(static_cast<uint32_t>(1U), fixture.credentials.installs);
    blinker::ControllerMutationReceipt decodedReceipt;
    CHECK_TRUE(blinker::decodeControllerMutationReceipt(
                   receipt,
                   decodedReceipt)
                   .ok());
    CHECK_TRUE(
        decodedReceipt.proofKind ==
        blinker::ControllerReceiptProofKind::HmacSha256);
    CHECK_TRUE(blinker::verifyControllerMutationReceiptProof(
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   decodedReceipt,
                   blinker::MutableByteSpan(workspace, sizeof(workspace)))
                   .ok());

    // Exact replay is durable-store idempotent and returns the same receipt.
    uint8_t firstReceipt[blinker::kControllerReceiptMaxEncodedSize] = {};
    const size_t firstReceiptSize = receipt.size;
    std::memcpy(firstReceipt, receipt.data, receipt.size);
    CHECK_TRUE(fixture.coordinator.apply(
                   encodedGrant,
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt)
                   .ok());
    CHECK_EQ(firstReceiptSize, receipt.size);
    CHECK_TRUE(std::memcmp(firstReceipt, receipt.data, receipt.size) == 0);

    for (size_t index = 0U; index < sizeof(fixture.secret); ++index) {
        fixture.secret[index] = static_cast<uint8_t>(0xA0U + index);
    }
    fixture.refreshSecretDigest();
    fixture.grantId[0] = 0x21U;
    fixture.controlNonce[0] = 0x41U;
    fixture.coordinator.endControlWindow();
    CHECK_TRUE(fixture.coordinator.beginControlWindow(blinker::ByteView(
                   fixture.controlNonce,
                   sizeof(fixture.controlNonce)))
                   .ok());
    blinker::ControllerGrant rotate = fixture.grant(
        blinker::ControllerMutationOperation::Rotate,
        1U,
        3U);
    encodedGrant = fixture.encodeGrant(
        rotate,
        grantBuffer,
        sizeof(grantBuffer));
    CHECK_TRUE(fixture.coordinator.apply(
                   encodedGrant,
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt)
                   .ok());
    CHECK_EQ(
        static_cast<uint32_t>(3U),
        fixture.credentials.credential.credentialVersion);
    uint8_t rotateReceipt[blinker::kControllerReceiptMaxEncodedSize] = {};
    const size_t rotateReceiptSize = receipt.size;
    std::memcpy(rotateReceipt, receipt.data, receipt.size);

    // Model a child reboot after the credential slot was atomically rotated
    // but before the Hub durably stored the receipt. The exact signed Grant
    // must be accepted as a replay and return the same HMAC receipt without
    // incrementing the credential version again.
    fixture.coordinator.endControlWindow();
    blinker::ControllerControlCoordinator resumed(
        fixture.deviceInstanceId,
        fixture.accessEpoch,
        fixture.credentials,
        fixture.grantVerifier);
    CHECK_TRUE(resumed.beginControlWindow(blinker::ByteView(
                   fixture.controlNonce,
                   sizeof(fixture.controlNonce)))
                   .ok());
    CHECK_TRUE(resumed.apply(
                   encodedGrant,
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt)
                   .ok());
    CHECK_EQ(rotateReceiptSize, receipt.size);
    CHECK_TRUE(std::memcmp(
                   rotateReceipt, receipt.data, receipt.size) == 0);
    CHECK_EQ(
        static_cast<uint32_t>(3U),
        fixture.credentials.credential.credentialVersion);
    resumed.endControlWindow();

    std::memset(fixture.secretDigest, 0, sizeof(fixture.secretDigest));
    fixture.grantId[0] = 0x22U;
    fixture.controlNonce[0] = 0x42U;
    fixture.coordinator.endControlWindow();
    CHECK_TRUE(fixture.coordinator.beginControlWindow(blinker::ByteView(
                   fixture.controlNonce,
                   sizeof(fixture.controlNonce)))
                   .ok());
    blinker::ControllerGrant revoke = fixture.grant(
        blinker::ControllerMutationOperation::Revoke,
        3U,
        3U);
    encodedGrant = fixture.encodeGrant(
        revoke,
        grantBuffer,
        sizeof(grantBuffer));
    CHECK_TRUE(fixture.coordinator.apply(
                   encodedGrant,
                   blinker::ByteView(),
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt)
                   .ok());
    CHECK_TRUE(!fixture.credentials.present);
    CHECK_TRUE(blinker::decodeControllerMutationReceipt(
                   receipt,
                   decodedReceipt)
                   .ok());
    CHECK_TRUE(
        decodedReceipt.proofKind ==
        blinker::ControllerReceiptProofKind::None);
    CHECK_TRUE(decodedReceipt.proof.empty());
    blinker::secureZero(blinker::MutableByteSpan(
        rotateReceipt, sizeof(rotateReceipt)));
}

void testVerificationAndCommitGuards() {
    Fixture fixture;
    uint8_t grantBuffer[blinker::kControllerGrantMaxEncodedSize] = {};
    uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
    uint8_t receiptBuffer[blinker::kControllerReceiptMaxEncodedSize] = {};
    blinker::ControllerGrant grant = fixture.grant(
        blinker::ControllerMutationOperation::Install,
        0U,
        1U);
    blinker::ByteView encodedGrant = fixture.encodeGrant(
        grant,
        grantBuffer,
        sizeof(grantBuffer));
    blinker::ByteView receipt;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt)
            .code());
    CHECK_TRUE(fixture.coordinator.beginControlWindow(blinker::ByteView(
                   fixture.controlNonce,
                   sizeof(fixture.controlNonce)))
                   .ok());
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(grantBuffer, sizeof(grantBuffer)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt)
            .code());
    uint8_t wrongSecret[blinker::kControllerCredentialSecretSize] = {};
    std::memset(wrongSecret, 0x5A, sizeof(wrongSecret));
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(wrongSecret, sizeof(wrongSecret)),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt)
            .code());
    CHECK_TRUE(!fixture.credentials.present);

    uint8_t wrongNonce[blinker::kControllerControlNonceSize];
    std::memcpy(wrongNonce, fixture.controlNonce, sizeof(wrongNonce));
    wrongNonce[0] ^= 1U;
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        fixture.coordinator.beginControlWindow(
            blinker::ByteView(wrongNonce, sizeof(wrongNonce)))
            .code());
    grant.controlNonce = blinker::ByteView(wrongNonce, sizeof(wrongNonce));
    encodedGrant = fixture.encodeGrant(
        grant,
        grantBuffer,
        sizeof(grantBuffer));
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt)
            .code());
    grant.controlNonce = blinker::ByteView(
        fixture.controlNonce,
        sizeof(fixture.controlNonce));
    encodedGrant = fixture.encodeGrant(
        grant,
        grantBuffer,
        sizeof(grantBuffer));
    fixture.signatures.reject = true;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt)
            .code());
    fixture.signatures.reject = false;

    uint8_t undersizedReceipt[1] = {};
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(
                undersizedReceipt,
                sizeof(undersizedReceipt)),
            receipt)
            .code());
    CHECK_TRUE(!fixture.credentials.present);
    CHECK_EQ(static_cast<uint32_t>(0U), fixture.credentials.installs);
    CHECK_TRUE(receipt.empty());

    CHECK_TRUE(fixture.coordinator.apply(
                   encodedGrant,
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt)
                   .ok());

    uint8_t receiptDigest[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::sha256(
                   receipt,
                   blinker::MutableByteSpan(
                       receiptDigest,
                       sizeof(receiptDigest)))
                   .ok());
    uint8_t idempotency[blinker::kControllerCommitIdempotencyKeySize];
    std::memset(idempotency, 0x66, sizeof(idempotency));
    blinker::ControllerCommitRequest commit;
    commit.grantId = grant.grantId;
    commit.operation = grant.operation;
    commit.deviceInstanceId = grant.deviceInstanceId;
    commit.ownershipGeneration = grant.ownershipGeneration;
    commit.controllerId = grant.controllerId;
    commit.credentialVersion = grant.credentialVersion;
    commit.receiptDigest = blinker::ByteView(
        receiptDigest,
        sizeof(receiptDigest));
    commit.verification =
        blinker::ControllerCommitVerification::Method2Confirmed;
    commit.idempotencyKey =
        blinker::ByteView(idempotency, sizeof(idempotency));
    uint8_t commitBuffer[blinker::kControllerCommitMaxEncodedSize] = {};
    blinker::ByteView encodedCommit;
    CHECK_TRUE(blinker::encodeControllerCommitRequest(
                   commit,
                   blinker::MutableByteSpan(
                       commitBuffer,
                       sizeof(commitBuffer)),
                   encodedCommit)
                   .ok());
    blinker::ControllerCommitRequest decodedCommit;
    CHECK_TRUE(blinker::decodeControllerCommitRequest(
                   encodedCommit,
                   decodedCommit)
                   .ok());
    commit.verification =
        blinker::ControllerCommitVerification::AuthorizedRevoke;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::encodeControllerCommitRequest(
            commit,
            blinker::MutableByteSpan(
                commitBuffer,
                sizeof(commitBuffer)),
            encodedCommit)
            .code());
    fixture.coordinator.endControlWindow();
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        fixture.coordinator.apply(
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt)
            .code());
}

void testMaximumEncodingBounds() {
    Fixture fixture;
    blinker::ControllerGrant grant = fixture.grant(
        blinker::ControllerMutationOperation::Rotate,
        UINT32_MAX - 1U,
        UINT32_MAX);
    grant.ownershipGeneration = UINT32_MAX;
    grant.permissions = blinker::kAuthorizationPermissionAll;
    grant.serverKeyId = UINT32_MAX;

    uint8_t grantOutput[blinker::kControllerGrantMaxEncodedSize] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeControllerGrant(
                   grant,
                   blinker::MutableByteSpan(
                       grantOutput,
                       sizeof(grantOutput)),
                   encoded)
                   .ok());
    CHECK_EQ(blinker::kControllerGrantMaxEncodedSize, encoded.size);

    uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
    CHECK_TRUE(blinker::encodeControllerGrantTranscript(
                   grant,
                   blinker::MutableByteSpan(workspace, sizeof(workspace)),
                   encoded)
                   .ok());
    CHECK_EQ(static_cast<size_t>(156U), encoded.size);

    blinker::ControllerMutationReceipt receipt;
    receipt.operation = grant.operation;
    receipt.grantId = grant.grantId;
    receipt.deviceInstanceId = grant.deviceInstanceId;
    receipt.ownershipGeneration = grant.ownershipGeneration;
    receipt.controllerId = grant.controllerId;
    receipt.credentialVersion = grant.credentialVersion;
    receipt.permissions = grant.permissions;
    receipt.secretDigest = grant.secretDigest;
    receipt.proofKind = blinker::ControllerReceiptProofKind::HmacSha256;
    receipt.proof = blinker::ByteView(
        fixture.secretDigest,
        sizeof(fixture.secretDigest));
    uint8_t receiptOutput[blinker::kControllerReceiptMaxEncodedSize] = {};
    CHECK_TRUE(blinker::encodeControllerMutationReceipt(
                   receipt,
                   blinker::MutableByteSpan(
                       receiptOutput,
                       sizeof(receiptOutput)),
                   encoded)
                   .ok());
    CHECK_EQ(blinker::kControllerReceiptMaxEncodedSize, encoded.size);

    blinker::ControllerCommitRequest commit;
    commit.grantId = grant.grantId;
    commit.operation = grant.operation;
    commit.deviceInstanceId = grant.deviceInstanceId;
    commit.ownershipGeneration = grant.ownershipGeneration;
    commit.controllerId = grant.controllerId;
    commit.credentialVersion = grant.credentialVersion;
    commit.receiptDigest = grant.secretDigest;
    commit.verification =
        blinker::ControllerCommitVerification::Method2Confirmed;
    commit.idempotencyKey = grant.grantId;
    uint8_t commitOutput[blinker::kControllerCommitMaxEncodedSize] = {};
    CHECK_TRUE(blinker::encodeControllerCommitRequest(
                   commit,
                   blinker::MutableByteSpan(
                       commitOutput,
                       sizeof(commitOutput)),
                   encoded)
                   .ok());
    CHECK_EQ(blinker::kControllerCommitMaxEncodedSize, encoded.size);
}

void testEndpointSessionOwnership() {
    Fixture fixture;
    SequenceRandom random;
    uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
    blinker::ControllerControlEndpoint endpoint(
        fixture.coordinator,
        random,
        blinker::MutableByteSpan(workspace, sizeof(workspace)));
    blinker::RxContext owner;
    owner.transportId = 2U;
    owner.sessionId = 41U;
    owner.encrypted = true;
    blinker::ByteView nonce;
    CHECK_TRUE(endpoint.beginControlWindow(owner, nonce).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), random.calls);
    CHECK_EQ(
        static_cast<size_t>(blinker::kControllerControlNonceSize),
        nonce.size);
    std::memcpy(fixture.controlNonce, nonce.data, nonce.size);

    blinker::ByteView repeatedNonce;
    CHECK_TRUE(endpoint.beginControlWindow(owner, repeatedNonce).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), random.calls);
    CHECK_TRUE(std::memcmp(
                   nonce.data,
                   repeatedNonce.data,
                   nonce.size) == 0);

    blinker::RxContext other = owner;
    other.sessionId = 42U;
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        endpoint.beginControlWindow(other, repeatedNonce).code());

    uint8_t grantBuffer[blinker::kControllerGrantMaxEncodedSize] = {};
    uint8_t receiptBuffer[blinker::kControllerReceiptMaxEncodedSize] = {};
    blinker::ControllerGrant grant = fixture.grant(
        blinker::ControllerMutationOperation::Install,
        0U,
        1U);
    blinker::ByteView encodedGrant = fixture.encodeGrant(
        grant,
        grantBuffer,
        sizeof(grantBuffer));
    blinker::ByteView receipt;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        endpoint.applyControllerMutation(
            other,
            encodedGrant,
            blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
            blinker::MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            receipt).code());
    CHECK_TRUE(endpoint.applyControllerMutation(
                   owner,
                   encodedGrant,
                   blinker::ByteView(
                       fixture.secret,
                       sizeof(fixture.secret)),
                   blinker::MutableByteSpan(
                       receiptBuffer,
                       sizeof(receiptBuffer)),
                   receipt).ok());
    CHECK_TRUE(!receipt.empty());

    endpoint.controllerSessionClosed(other);
    CHECK_TRUE(endpoint.beginControlWindow(owner, repeatedNonce).ok());
    endpoint.controllerSessionClosed(owner);
    CHECK_TRUE(endpoint.beginControlWindow(other, repeatedNonce).ok());
    CHECK_EQ(static_cast<uint32_t>(2U), random.calls);
}

} // namespace

int main() {
    testInstallRotateRevoke();
    testVerificationAndCommitGuards();
    testMaximumEncodingBounds();
    testEndpointSessionOwnership();
    std::cout << "controller control tests passed" << std::endl;
    return 0;
}
