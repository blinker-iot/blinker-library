#ifndef BLINKER_INTERFACE_IDEVICEACCESSSTORE_H
#define BLINKER_INTERFACE_IDEVICEACCESSSTORE_H

#include "IAccessEpochSource.h"
#include "IControllerCredentialStore.h"
#include "IDeviceKeyStore.h"

namespace blinker {

// Atomic DeviceKey + direct-controller root used by combined cloud/direct
// products. WiFi-only and local-only products do not need this interface.
class IDeviceAccessStore :
    public IDeviceKeyStore,
    public IControllerCredentialStore,
    public IAccessEpochSource {
public:
    virtual Result bootstrap(
        const DeviceKey& deviceKey,
        uint32_t accessEpoch,
        const ControllerCredential& initialController) = 0;

    // Physical factory reset boundary. DeviceKey clear() and controller
    // clearAll() intentionally preserve the other access domain and epoch.
    virtual Result eraseAccess() = 0;
};

} // namespace blinker

#endif
