#ifndef BLINKER_PROVISIONING_CONTROLLERGRANTVERIFIER_H
#define BLINKER_PROVISIONING_CONTROLLERGRANTVERIFIER_H

#include "ControllerControlContract.h"
#include "../interface/IServerSignatureVerifier.h"

namespace blinker {

struct ControllerGrantVerificationContext {
    ByteView deviceInstanceId;
    uint32_t ownershipGeneration;
    ByteView controlNonce;

    ControllerGrantVerificationContext()
        : ownershipGeneration(0U) {}
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
