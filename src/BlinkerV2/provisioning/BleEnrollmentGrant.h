#ifndef BLINKER_PROVISIONING_BLEENROLLMENTGRANT_H
#define BLINKER_PROVISIONING_BLEENROLLMENTGRANT_H

#include "../core/Sha256.h"
#include "../identity/ControllerCredential.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/IServerSignatureVerifier.h"
#include "../protocol/SetupSession.h"

namespace blinker {

enum : size_t {
    kBleEnrollmentGrantIdSize = 16U,
    kBleEnrollmentGrantNonceSize = 16U,
    kBleEnrollmentGrantSignatureSize = 64U,
    kBleEnrollmentGrantMaxEncodedSize = 270U,
    kBleEnrollmentGrantTranscriptMaxEncodedSize = 217U,
    kBleEnrollmentGrantWorkspaceSize =
        kBleEnrollmentGrantTranscriptMaxEncodedSize
};

static const uint8_t kBleEnrollmentGrantVersion = 2U;
static const uint64_t kBleEnrollmentGrantMaxLifetimeSeconds = 15U * 60U;

enum class BleEnrollmentSecurityProfile : uint8_t {
    EducationNearby = 1U,
    ProductOob = 2U
};

// One server-authorized installation of the first Admin controller. The raw
// secret travels only inside the Noise session and is represented here by its
// digest. Account, owner, family and logical-device records stay server-side.
struct BleEnrollmentGrant {
    ByteView grantId;
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupTranscriptHash;
    uint32_t accessEpoch;
    ByteView controllerId;
    ByteView controllerSecretDigest;
    uint32_t controllerPermissions;
    ByteView nonce;
    uint64_t issuedAt;
    uint64_t expiresAt;
    BleEnrollmentSecurityProfile securityProfile;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    BleEnrollmentGrant();
};

Result encodeBleEnrollmentGrant(const BleEnrollmentGrant& grant,
                                MutableByteSpan output,
                                ByteView& encoded);
Result decodeBleEnrollmentGrant(ByteView encoded,
                                BleEnrollmentGrant& grant);
Result encodeBleEnrollmentGrantTranscript(const BleEnrollmentGrant& grant,
                                          MutableByteSpan output,
                                          ByteView& encoded);

struct BleEnrollmentGrantContext {
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupTranscriptHash;
    uint32_t accessEpoch;
    BleEnrollmentSecurityProfile securityProfile;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    uint64_t nowEpochSeconds;
    bool hasTrustedTime;

    BleEnrollmentGrantContext();
};

class BleEnrollmentGrantVerifier {
public:
    explicit BleEnrollmentGrantVerifier(
        IServerSignatureVerifier& signatureVerifier)
        : signatureVerifier_(signatureVerifier) {}

    Result verify(ByteView encodedGrant,
                  const BleEnrollmentGrantContext& context,
                  MutableByteSpan workspace,
                  BleEnrollmentGrant& grant);

private:
    IServerSignatureVerifier& signatureVerifier_;
};

} // namespace blinker

#endif
