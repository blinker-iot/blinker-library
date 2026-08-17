#ifndef BLINKER_INTERFACE_ICONTROLLERCREDENTIALSOURCE_H
#define BLINKER_INTERFACE_ICONTROLLERCREDENTIALSOURCE_H

#include "IAtomicBlobStore.h"
#include "../identity/ControllerCredential.h"

namespace blinker {

class IControllerCredentialSource {
public:
    virtual ~IControllerCredentialSource() {}

    // Returns only a credential in the requested authorization domain and
    // ownership generation. A stale generation is indistinguishable from a
    // missing controller to authentication callers.
    virtual Result loadActive(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t ownershipGeneration,
        ControllerCredential& output) = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
