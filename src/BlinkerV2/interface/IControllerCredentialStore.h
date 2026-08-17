#ifndef BLINKER_INTERFACE_ICONTROLLERCREDENTIALSTORE_H
#define BLINKER_INTERFACE_ICONTROLLERCREDENTIALSTORE_H

#include "IControllerCredentialSource.h"

namespace blinker {

// Mutation boundary used only after a controller control-plane has verified
// the exact authorization grant. Persistence does not parse wire messages.
class IControllerCredentialStore : public IControllerCredentialSource {
public:
    virtual Result installVerified(
        const ControllerCredential& credential) = 0;
    virtual Result rotateVerified(
        uint32_t expectedCredentialVersion,
        const ControllerCredential& credential) = 0;
    virtual Result revokeVerified(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t ownershipGeneration,
        uint32_t expectedCredentialVersion) = 0;
    virtual Result clearAll() = 0;
};

} // namespace blinker

#endif
