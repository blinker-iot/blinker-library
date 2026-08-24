#ifndef BLINKER_INTERFACE_IDEVICEACCESSSTORE_H
#define BLINKER_INTERFACE_IDEVICEACCESSSTORE_H

#include "IAccessEpochSource.h"
#include "IControllerCredentialStore.h"
#include "IDeviceKeyStore.h"

namespace blinker {

// Atomic optional DeviceKey + direct-controller root. Cloud/direct products
// bootstrap both domains together; BLE-only products bootstrap only access.
class IDeviceAccessStore :
    public IDeviceKeyStore,
    public IControllerCredentialStore,
    public IAccessEpochSource {
public:
    virtual Result bootstrap(
        const DeviceKey& deviceKey,
        uint32_t accessEpoch,
        const ControllerCredential& initialController) = 0;

    virtual Result bootstrapAccess(
        uint32_t accessEpoch,
        const ControllerCredential& initialController) = 0;

    // Physical factory reset boundary. DeviceKey clear() preserves direct
    // access. Controller clearAll() preserves a present DeviceKey, but a
    // BLE-only access root is removed because no cloud recovery path exists.
    virtual Result eraseAccess() = 0;
};

} // namespace blinker

#endif
