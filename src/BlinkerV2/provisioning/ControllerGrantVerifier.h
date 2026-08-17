#ifndef BLINKER_PROVISIONING_CONTROLLERGRANTVERIFIER_H
#define BLINKER_PROVISIONING_CONTROLLERGRANTVERIFIER_H

#include "ControllerControlContract.h"
#include "../interface/IServerSignatureVerifier.h"

namespace blinker {

struct ControllerGrantVerificationContext {
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    ByteView controlNonce;
    uint64_t nowEpochSeconds;
    bool hasTrustedTime;

    ControllerGrantVerificationContext()
        : ownershipGeneration(0U),
          nowEpochSeconds(0U),
          hasTrustedTime(false) {}
};

class ControllerGrantVerifier {
public:
    explicit ControllerGrantVerifier(
        IServerSignatureVerifier& signatureVerifier)
        : signatureVerifier_(signatureVerifier) {}

    // controllerSecret is required only for Install/Rotate. The decoded grant
    // borrows encodedGrant and remains valid while that input remains valid.
    Result verify(
        ByteView encodedGrant,
        ByteView controllerSecret,
        const ControllerGrantVerificationContext& context,
        MutableByteSpan transcriptWorkspace,
        ControllerGrant& grant);

private:
    IServerSignatureVerifier& signatureVerifier_;
};

} // namespace blinker

#endif
