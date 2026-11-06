#ifndef BLINKER_INTERFACE_IINITIALCLOUDKEYINSTALLER_H
#define BLINKER_INTERFACE_IINITIALCLOUDKEYINSTALLER_H

#include "../identity/ControllerCredential.h"
#include "../identity/DeviceKey.h"

namespace blinker {

// Internal synchronous authorization check at the actual root write boundary.
// The product already owns the original task/root fence. Implementations must
// not perform storage I/O, wait for an owner, retry, or retain the root secret.
class IInitialCloudKeyCommitGuard {
public:
    virtual ~IInitialCloudKeyCommitGuard() {}
    virtual Result validateInitialCloudKeyCommit(const DeviceKey& candidate,
        uint32_t accessEpoch, const ControllerCredential& actualAdmin) = 0;
};

// A trusted product adapter, not a Sketch or remote registration API. The
// implementation must invoke the guard under its existing root mutation fence,
// after reading/CAS-checking the real root and before any Key write or no-op.
class IInitialCloudKeyInstaller {
public:
    virtual ~IInitialCloudKeyInstaller() {}
    virtual Result installInitialCloudKeyAuthorized(const DeviceKey& candidate,
        uint32_t expectedEpoch, const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard& guard) = 0;
};

} // namespace blinker

#endif
