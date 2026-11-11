#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/provisioning/BleEnrollmentApplication.h>

#include <stdio.h>
#include <string.h>

namespace {

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at line %d: %s\n",              \
                    __LINE__, #condition);                                   \
            return 1;                                                        \
        }                                                                    \
    } while (false)

class MemoryBlob final : public blinker::IAtomicBlobStore {
public:
    MemoryBlob() : replaceCalls(0U), data_(), size_(0U), present_(false) {}

    blinker::Result load(blinker::MutableByteSpan output,
                         size_t& written) override {
        written = 0U;
        if (!present_) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.size < size_) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        memcpy(output.data, data_, size_);
        written = size_;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        if (value.data == nullptr || value.size > sizeof(data_)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        memcpy(data_, value.data, value.size);
        ++replaceCalls;
        size_ = value.size;
        present_ = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        memset(data_, 0, sizeof(data_));
        size_ = 0U;
        present_ = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::PlainFlash;
    }

    size_t replaceCalls;

private:
    uint8_t data_[blinker::DeviceAccessStore::serializedSize];
    size_t size_;
    bool present_;
};

class FixedRandom final : public blinker::IRandom {
public:
    FixedRandom() : next_(1U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (output.data == nullptr || output.empty()) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = next_++;
            if (next_ == 0U) next_ = 1U;
        }
        return blinker::Result::success();
    }

private:
    uint8_t next_;
};

class FixedClock final : public blinker::IClock {
public:
    uint64_t now = 1000U;
    uint32_t monotonicMillis() const override { return 1U; }
    bool unixTime(uint64_t& seconds) const override {
        seconds = now;
        return true;
    }
};

// An exact fixture, not an Ed25519 implementation. This tests the application
// verification/storage boundary; real signature algorithms have separate gates.
class FixtureSignatureVerifier final : public blinker::IServerSignatureVerifier {
public:
    FixtureSignatureVerifier() : calls(0U), expectedDigest() {}

    blinker::Result verifyDigest(
        uint32_t keyId,
        blinker::ServerSignatureAlgorithm algorithm,
        blinker::ByteView digest,
        blinker::ByteView signature) override {
        ++calls;
        bool signatureMatches = signature.data != nullptr &&
            signature.size == blinker::kBleEnrollmentGrantSignatureSize;
        for (size_t index = 0U; signatureMatches && index < signature.size; ++index) {
            signatureMatches = signature.data[index] == 0xa5U;
        }
        return keyId == 7U &&
                       algorithm ==
                           blinker::ServerSignatureAlgorithm::Ed25519 &&
                       digest.size == blinker::kSha256Size &&
                       memcmp(digest.data, expectedDigest, sizeof(expectedDigest)) == 0 &&
                       signatureMatches
                   ? blinker::Result::success()
                   : blinker::Result::failure(
                         blinker::ErrorCode::AuthenticationRequired);
    }

    size_t calls;
    uint8_t expectedDigest[blinker::kSha256Size];
};

void fill(uint8_t* output, size_t size, uint8_t first) {
    for (size_t index = 0U; index < size; ++index) {
        output[index] = static_cast<uint8_t>(first + index);
    }
}

} // namespace

int main() {
    blinker::DeviceInstanceId device;
    fill(device.bytes, sizeof(device.bytes), 0x10U);
    uint8_t locator[blinker::kSetupSessionLocatorSize];
    uint8_t transcript[blinker::kSha256Size];
    uint8_t grantId[blinker::kBleEnrollmentGrantIdSize];
    uint8_t controllerId[blinker::kControllerIdSize];
    uint8_t controllerSecret[blinker::kControllerCredentialSecretSize];
    uint8_t secretDigest[blinker::kSha256Size];
    uint8_t presenceKey[blinker::kDevicePresenceKeySize];
    uint8_t presenceKeyDigest[blinker::kSha256Size];
    uint8_t nonce[blinker::kBleEnrollmentGrantNonceSize];
    uint8_t signature[blinker::kBleEnrollmentGrantSignatureSize];
    fill(locator, sizeof(locator), 0x20U);
    fill(transcript, sizeof(transcript), 0x30U);
    fill(grantId, sizeof(grantId), 0x40U);
    fill(controllerId, sizeof(controllerId), 0x50U);
    fill(controllerSecret, sizeof(controllerSecret), 0x60U);
    fill(presenceKey, sizeof(presenceKey), 0x80U);
    fill(nonce, sizeof(nonce), 0x70U);
    memset(signature, 0xa5, sizeof(signature));
    CHECK(blinker::sha256(
        blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
        blinker::MutableByteSpan(secretDigest, sizeof(secretDigest))));
    CHECK(blinker::sha256(
        blinker::ByteView(presenceKey, sizeof(presenceKey)),
        blinker::MutableByteSpan(
            presenceKeyDigest, sizeof(presenceKeyDigest))));

    MemoryBlob memory;
    blinker::DeviceAccessStore access(memory);
    FixedRandom random;
    FixedClock clock;
    FixtureSignatureVerifier signatureVerifier;
    blinker::BleEnrollmentGrantVerifier grantVerifier(signatureVerifier);
    blinker::BleEnrollmentApplicationConfig enrollmentConfig;
    enrollmentConfig.serverKeyId = 7U;
    blinker::BleEnrollmentApplication application(
        device, random, clock, grantVerifier, access, enrollmentConfig);

    CHECK(application.beginSession(
        blinker::ByteView(locator, sizeof(locator))));

    uint8_t requestStorage[blinker::kBleEnrollmentRequestMaxEncodedSize] = {};
    uint8_t workspace[blinker::kBleEnrollmentWorkspaceSize] = {};
    uint8_t responseStorage[blinker::kBleEnrollmentResponseMaxEncodedSize] = {};
    blinker::ByteView request;
    blinker::ByteView response;
    CHECK(blinker::encodeBleEnrollmentHelloRequest(
        1U,
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)),
        request));
    CHECK(application.handle(
        request,
        blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
        response));
    CHECK(application.state() ==
          blinker::BleEnrollmentApplicationState::AwaitingSecureSession);
    CHECK(application.secureSessionReady(
        blinker::ByteView(transcript, sizeof(transcript))));
    CHECK(application.state() ==
          blinker::BleEnrollmentApplicationState::AwaitingEnrollment);

    blinker::BleEnrollmentResponse hello;
    CHECK(blinker::decodeBleEnrollmentResponse(response, hello));
    CHECK(hello.type == blinker::BleEnrollmentMessageType::HelloResponse);
    CHECK(hello.accessEpoch != 0U);
    CHECK(hello.setupSessionId.size == blinker::kSetupSessionIdSize);
    CHECK(hello.securityProfile ==
          blinker::BleEnrollmentSecurityProfile::EducationNearby);
    CHECK(hello.serverKeyId == 7U);
    CHECK(hello.signatureAlgorithm ==
          blinker::ServerSignatureAlgorithm::Ed25519);
    const uint32_t firstAccessEpoch = hello.accessEpoch;
    uint8_t firstSetupSessionId[blinker::kSetupSessionIdSize] = {};
    memcpy(firstSetupSessionId,
           hello.setupSessionId.data,
           sizeof(firstSetupSessionId));

    application.endSession();
    locator[0] ^= 0x55U;
    CHECK(application.beginSession(
        blinker::ByteView(locator, sizeof(locator))));
    CHECK(application.secureSessionReady(
        blinker::ByteView(transcript, sizeof(transcript))));
    CHECK(blinker::encodeBleEnrollmentHelloRequest(
        1U,
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)),
        request));
    CHECK(application.handle(
        request,
        blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
        response));
    CHECK(blinker::decodeBleEnrollmentResponse(response, hello));
    CHECK(hello.accessEpoch != firstAccessEpoch);
    CHECK(memcmp(firstSetupSessionId,
                 hello.setupSessionId.data,
                 sizeof(firstSetupSessionId)) != 0);

    blinker::BleEnrollmentGrant grant;
    grant.grantId = blinker::ByteView(grantId, sizeof(grantId));
    grant.deviceInstanceId =
        blinker::ByteView(device.bytes, sizeof(device.bytes));
    grant.setupSessionId = hello.setupSessionId;
    grant.setupTranscriptHash =
        blinker::ByteView(transcript, sizeof(transcript));
    grant.accessEpoch = hello.accessEpoch;
    grant.controllerId = blinker::ByteView(controllerId, sizeof(controllerId));
    grant.controllerSecretDigest =
        blinker::ByteView(secretDigest, sizeof(secretDigest));
    grant.controllerPermissions = blinker::kControllerPermissionAll;
    grant.presenceKeyVersion = 1U;
    grant.presenceKeyDigest =
        blinker::ByteView(presenceKeyDigest, sizeof(presenceKeyDigest));
    grant.nonce = blinker::ByteView(nonce, sizeof(nonce));
    grant.issuedAt = 900U;
    grant.expiresAt = 1100U;
    grant.securityProfile =
        blinker::BleEnrollmentSecurityProfile::EducationNearby;
    grant.serverKeyId = 7U;
    grant.signatureAlgorithm = blinker::ServerSignatureAlgorithm::Ed25519;
    grant.signature = blinker::ByteView(signature, sizeof(signature));
    blinker::ByteView grantTranscript;
    CHECK(blinker::encodeBleEnrollmentGrantTranscript(
        grant, blinker::MutableByteSpan(workspace, sizeof(workspace)), grantTranscript));
    CHECK(blinker::sha256(grantTranscript,
        blinker::MutableByteSpan(signatureVerifier.expectedDigest,
                                 sizeof(signatureVerifier.expectedDigest))));

    uint8_t encodedGrantStorage[blinker::kBleEnrollmentGrantMaxEncodedSize] = {};
    blinker::ByteView encodedGrant;
    grant.securityProfile = blinker::BleEnrollmentSecurityProfile::ProductOob;
    CHECK(blinker::encodeBleEnrollmentGrant(
        grant,
        blinker::MutableByteSpan(
            encodedGrantStorage, sizeof(encodedGrantStorage)),
        encodedGrant));
    CHECK(blinker::encodeBleEnrollmentRequest(
        2U,
        encodedGrant,
        blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
        blinker::ByteView(presenceKey, sizeof(presenceKey)),
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)),
        request));
    CHECK(application.handle(
              request,
              blinker::MutableByteSpan(workspace, sizeof(workspace)),
              blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
              response)
              .code() == blinker::ErrorCode::AuthenticationRequired);
    CHECK(signatureVerifier.calls == 0U);

    grant.securityProfile =
        blinker::BleEnrollmentSecurityProfile::EducationNearby;
    CHECK(blinker::encodeBleEnrollmentGrant(
        grant,
        blinker::MutableByteSpan(
            encodedGrantStorage, sizeof(encodedGrantStorage)),
        encodedGrant));
    CHECK(blinker::encodeBleEnrollmentRequest(
        2U,
        encodedGrant,
        blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
        blinker::ByteView(presenceKey, sizeof(presenceKey)),
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)),
        request));

    uint8_t exactRequest[blinker::kBleEnrollmentRequestMaxEncodedSize] = {};
    memcpy(exactRequest, request.data, request.size);
    const size_t exactRequestSize = request.size;

    // The exact expiry boundary (and not-yet-valid grant) rejects a first
    // install without consuming the request or touching durable access.
    const uint64_t invalidTimes[] = {899U, 1100U};
    for (size_t index = 0U; index < 2U; ++index) {
        clock.now = invalidTimes[index];
        CHECK(application.handle(request,
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
            response).code() == blinker::ErrorCode::AuthenticationRequired);
        CHECK(response.empty());
        CHECK(memory.replaceCalls == 0U);
        CHECK(signatureVerifier.calls == 0U);
    }
    clock.now = 1000U;

    // Signature, controller secret and PresenceKey tampering are independent
    // failure boundaries. Re-encode the original request after each mutation.
    uint8_t* mutations[] = {signature + sizeof(signature) - 1U,
                            controllerSecret, presenceKey};
    for (size_t index = 0U; index < 3U; ++index) {
        *mutations[index] ^= 1U;
        CHECK(blinker::encodeBleEnrollmentGrant(grant,
            blinker::MutableByteSpan(encodedGrantStorage, sizeof(encodedGrantStorage)),
            encodedGrant));
        CHECK(blinker::encodeBleEnrollmentRequest(2U, encodedGrant,
            blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
            blinker::ByteView(presenceKey, sizeof(presenceKey)),
            blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)), request));
        CHECK(application.handle(request,
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
            response).code() == blinker::ErrorCode::AuthenticationRequired);
        CHECK(response.empty());
        CHECK(memory.replaceCalls == 0U);
        *mutations[index] ^= 1U;
    }
    signatureVerifier.calls = 0U;
    request = blinker::ByteView(exactRequest, exactRequestSize);
    CHECK(application.handle(
        request,
        blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
        response));
    CHECK(memory.replaceCalls == 1U);
    uint8_t exactResponse[blinker::kBleEnrollmentResponseMaxEncodedSize] = {};
    memcpy(exactResponse, response.data, response.size);
    const size_t exactResponseSize = response.size;

    blinker::BleEnrollmentResponse enrolled;
    CHECK(blinker::decodeBleEnrollmentResponse(response, enrolled));
    CHECK(enrolled.type == blinker::BleEnrollmentMessageType::EnrollResponse);
    blinker::ControllerMutationReceipt receipt;
    CHECK(blinker::decodeControllerMutationReceipt(enrolled.receipt, receipt));
    CHECK(receipt.ownershipGeneration == hello.accessEpoch);
    CHECK(blinker::verifyControllerMutationReceiptProof(
        blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
        receipt,
        blinker::MutableByteSpan(workspace, sizeof(workspace))));

    uint32_t accessEpoch = 0U;
    CHECK(access.loadAccessEpoch(accessEpoch));
    CHECK(accessEpoch == hello.accessEpoch);
    blinker::ControllerCredential installed;
    CHECK(access.loadActive(
        blinker::ByteView(controllerId, sizeof(controllerId)),
        blinker::ControllerCredentialDomain::PlatformAccess,
        hello.accessEpoch,
        installed));
    CHECK(memcmp(installed.secret,
                  controllerSecret,
                  sizeof(controllerSecret)) == 0);
    blinker::DevicePresenceKey installedPresence;
    CHECK(access.loadPresence(hello.accessEpoch, installedPresence));
    CHECK(installedPresence.version == 1U);
    CHECK(memcmp(installedPresence.bytes,
                 presenceKey,
                 sizeof(presenceKey)) == 0);

    // Already-installed exact bytes remain idempotent after expiry. New bytes
    // with that request ID cannot acquire this completed-operation exemption.
    clock.now = 1100U;
    CHECK(application.handle(
        blinker::ByteView(exactRequest, exactRequestSize),
        blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
        response));
    CHECK(response.size == exactResponseSize);
    CHECK(memcmp(response.data, exactResponse, exactResponseSize) == 0);
    CHECK(signatureVerifier.calls == 2U);
    CHECK(memory.replaceCalls == 1U);
    uint8_t storedBefore[blinker::DeviceAccessStore::serializedSize] = {};
    uint8_t storedAfter[blinker::DeviceAccessStore::serializedSize] = {};
    size_t beforeSize = 0U, afterSize = 0U;
    CHECK(memory.load(blinker::MutableByteSpan(storedBefore, sizeof(storedBefore)), beforeSize));
    // Grant, controller-secret and PresenceKey content conflicts keep the same
    // requestId. Rejection cannot poison the completed request's replay slot.
    blinker::BleEnrollmentRequest decodedExact;
    CHECK(blinker::decodeBleEnrollmentRequest(
        blinker::ByteView(exactRequest, exactRequestSize), decodedExact));
    const size_t conflictOffsets[] = {
        static_cast<size_t>(decodedExact.grant.data - exactRequest) + decodedExact.grant.size - 1U,
        static_cast<size_t>(decodedExact.controllerSecret.data - exactRequest),
        static_cast<size_t>(decodedExact.presenceKey.data - exactRequest)
    };
    for (size_t index = 0U; index < 3U; ++index) {
        exactRequest[conflictOffsets[index]] ^= 1U;
        const size_t verifications = signatureVerifier.calls;
        CHECK(application.handle(
            blinker::ByteView(exactRequest, exactRequestSize),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)),
            response).code() == blinker::ErrorCode::AuthenticationRequired);
        CHECK(response.empty());
        CHECK(signatureVerifier.calls == verifications);
        CHECK(memory.replaceCalls == 1U);
        exactRequest[conflictOffsets[index]] ^= 1U;
        CHECK(application.handle(
            blinker::ByteView(exactRequest, exactRequestSize),
            blinker::MutableByteSpan(workspace, sizeof(workspace)),
            blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)), response));
        CHECK(response.size == exactResponseSize);
        CHECK(memcmp(response.data, exactResponse, exactResponseSize) == 0);
        CHECK(memory.replaceCalls == 1U);
        CHECK(memory.load(blinker::MutableByteSpan(storedAfter, sizeof(storedAfter)), afterSize));
        CHECK(beforeSize == afterSize);
        CHECK(memcmp(storedBefore, storedAfter, beforeSize) == 0);
    }

    application.endSession();
    CHECK(application.beginSession(
              blinker::ByteView(locator, sizeof(locator)))
              .code() == blinker::ErrorCode::StateConflict);
    // A fresh application instance (as after reboot) also cannot open an
    // enrollment session over installed access. Recovery uses original Admin
    // Method 2, not a persisted or cross-session requestId replay exception.
    blinker::BleEnrollmentApplication restarted(
        device, random, clock, grantVerifier, access, enrollmentConfig);
    CHECK(restarted.beginSession(blinker::ByteView(locator, sizeof(locator)))
        .code() == blinker::ErrorCode::StateConflict);
    CHECK(memory.replaceCalls == 1U);
    blinker::secureZero(blinker::MutableByteSpan(storedBefore, sizeof(storedBefore)));
    blinker::secureZero(blinker::MutableByteSpan(storedAfter, sizeof(storedAfter)));

    // Physical access reset reuses the same application object. A new setup
    // window must discard its completed-request cache, even while the old grant
    // is still within its installation TTL. No server deletion is simulated here.
    CHECK(access.eraseAccess());
    clock.now = 1000U;
    locator[0] ^= 0x33U;
    transcript[0] ^= 0x44U;
    CHECK(application.beginSession(blinker::ByteView(locator, sizeof(locator))));
    CHECK(application.secureSessionReady(blinker::ByteView(transcript, sizeof(transcript))));
    CHECK(blinker::encodeBleEnrollmentHelloRequest(1U,
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)), request));
    CHECK(application.handle(request, blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)), response));
    CHECK(blinker::decodeBleEnrollmentResponse(response, hello));
    CHECK(hello.accessEpoch != installed.ownershipGeneration);
    CHECK(blinker::constantTimeEqual(hello.deviceInstanceId,
        blinker::ByteView(device.bytes, sizeof(device.bytes))));
    uint8_t newSetup[blinker::kSetupSessionIdSize] = {};
    memcpy(newSetup, hello.setupSessionId.data, sizeof(newSetup));
    const uint32_t newEpoch = hello.accessEpoch;
    CHECK(application.handle(blinker::ByteView(exactRequest, exactRequestSize),
        blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)), response)
        .code() == blinker::ErrorCode::AuthenticationRequired);
    CHECK(response.empty());
    CHECK(memory.replaceCalls == 1U);
    uint32_t absentEpoch = 0U;
    CHECK(access.loadAccessEpoch(absentEpoch).code() == blinker::ErrorCode::NotFound);

    grant.setupSessionId = blinker::ByteView(newSetup, sizeof(newSetup));
    grant.accessEpoch = newEpoch;
    grantId[0] ^= 0x55U;
    nonce[0] ^= 0x66U;
    controllerId[0] ^= 0x77U;
    controllerSecret[0] ^= 0x88U;
    presenceKey[0] ^= 0x99U;
    CHECK(blinker::sha256(blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
        blinker::MutableByteSpan(secretDigest, sizeof(secretDigest))));
    CHECK(blinker::sha256(blinker::ByteView(presenceKey, sizeof(presenceKey)),
        blinker::MutableByteSpan(presenceKeyDigest, sizeof(presenceKeyDigest))));
    CHECK(blinker::encodeBleEnrollmentGrantTranscript(grant,
        blinker::MutableByteSpan(workspace, sizeof(workspace)), grantTranscript));
    CHECK(blinker::sha256(grantTranscript, blinker::MutableByteSpan(
        signatureVerifier.expectedDigest, sizeof(signatureVerifier.expectedDigest))));
    CHECK(blinker::encodeBleEnrollmentGrant(grant,
        blinker::MutableByteSpan(encodedGrantStorage, sizeof(encodedGrantStorage)), encodedGrant));
    CHECK(blinker::encodeBleEnrollmentRequest(2U, encodedGrant,
        blinker::ByteView(controllerSecret, sizeof(controllerSecret)),
        blinker::ByteView(presenceKey, sizeof(presenceKey)),
        blinker::MutableByteSpan(requestStorage, sizeof(requestStorage)), request));
    CHECK(application.handle(request, blinker::MutableByteSpan(workspace, sizeof(workspace)),
        blinker::MutableByteSpan(responseStorage, sizeof(responseStorage)), response));
    CHECK(application.state() == blinker::BleEnrollmentApplicationState::Enrolled);
    CHECK(memory.replaceCalls == 2U);
    blinker::ControllerCredential reclaimed;
    CHECK(!access.loadActive(installed.id(), installed.domain, installed.ownershipGeneration, reclaimed));
    CHECK(access.loadActive(blinker::ByteView(controllerId, sizeof(controllerId)),
        blinker::ControllerCredentialDomain::PlatformAccess, newEpoch, reclaimed));
    CHECK(memcmp(reclaimed.secret, controllerSecret, sizeof(controllerSecret)) == 0);
    blinker::clearControllerCredential(reclaimed);

    blinker::clearControllerCredential(installed);
    blinker::clearDevicePresenceKey(installedPresence);
    printf("BLE enrollment grant/application tests passed\n");
    return 0;
}
