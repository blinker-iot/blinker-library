#ifndef BLINKER_INTERFACE_ICONTROLLERCREDENTIALSOURCE_H
#define BLINKER_INTERFACE_ICONTROLLERCREDENTIALSOURCE_H

#include "IAtomicBlobStore.h"
#include "../identity/ControllerCredential.h"

namespace blinker {

class IControllerCredentialSource {
public:
    virtual ~IControllerCredentialSource() {}

    // Returns only a credential in the requested authorization domain and
    // access epoch. A stale epoch is indistinguishable from a missing group
    // to authentication callers.
    virtual Result loadActive(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        ControllerCredential& output) = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
