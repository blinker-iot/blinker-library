#ifndef BLINKER_INTERFACE_ICLOUDCREDENTIALSOURCE_H
#define BLINKER_INTERFACE_ICLOUDCREDENTIALSOURCE_H

#include "IAtomicBlobStore.h"
#include "../identity/CloudCredential.h"

namespace blinker {

// Read-only role used by daily cloud-session code. Enrollment writers do not
// need to be exposed to the data-plane session provider.
class ICloudCredentialSource {
public:
    virtual ~ICloudCredentialSource() {}

    virtual Result load(CloudCredential& output) = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
