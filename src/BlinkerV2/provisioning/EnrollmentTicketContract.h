#ifndef BLINKER_PROVISIONING_ENROLLMENTTICKETCONTRACT_H
#define BLINKER_PROVISIONING_ENROLLMENTTICKETCONTRACT_H

#include "../core/Sha256.h"
#include "../identity/ControllerCredential.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/IServerSignatureVerifier.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

enum : size_t {
    kEnrollmentTicketIdSize = 16U,
    kEnrollmentTicketSetupSessionIdSize = 16U,
    kEnrollmentTicketSetupTranscriptHashSize = kSha256Size,
    kEnrollmentTicketControllerSecretDigestSize = kSha256Size,
    kEnrollmentTicketNonceSize = 16U,
    kEnrollmentTicketSignatureSize = 64U,
    kEnrollmentTicketMaxEncodedSize = 268U,
    kEnrollmentTicketTranscriptMaxEncodedSize = 216U,
    kEnrollmentTicketWorkspaceSize =
        kEnrollmentTicketTranscriptMaxEncodedSize
};

enum : uint32_t {
    kEnrollmentTicketAuthorizeCloudEnrollment = 1UL << 0U,
    kEnrollmentTicketAuthorizeNetworkProvisioning = 1UL << 1U,
    kEnrollmentTicketAuthorizeAll =
        kEnrollmentTicketAuthorizeCloudEnrollment |
        kEnrollmentTicketAuthorizeNetworkProvisioning
};

static const uint8_t kEnrollmentTicketContractVersion = 1U;
static const uint64_t kEnrollmentTicketMaxLifetimeSeconds = 15U * 60U;

// Short-lived server authorization for the current physical setup session.
// User JWT, WiFi credentials, raw controller secret and CloudCredential never
// enter this signed body. A zero authorizedOperations mask is the BLE-only
// profile; WiFi+BLE explicitly enables network and cloud operations.
struct EnrollmentTicket {
    ByteView ticketId;
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupTranscriptHash;
    uint32_t ownershipGeneration;
    ByteView controllerId;
    ByteView controllerSecretDigest;
    uint32_t controllerPermissions;
    uint32_t authorizedOperations;
    ByteView ticketNonce;
    uint64_t issuedAt;
    uint64_t expiresAt;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    EnrollmentTicket();
};

Result encodeEnrollmentTicket(
    const EnrollmentTicket& ticket,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeEnrollmentTicket(
    ByteView encoded,
    EnrollmentTicket& ticket);
Result encodeEnrollmentTicketTranscript(
    const EnrollmentTicket& ticket,
    MutableByteSpan output,
    ByteView& encoded);

} // namespace blinker

#endif
