#ifndef BLINKER_INTERFACE_IDEVICEACCESSSTORE_H
#define BLINKER_INTERFACE_IDEVICEACCESSSTORE_H

#include "IAccessEpochSource.h"
#include "IControllerCredentialStore.h"
#include "IDeviceKeyStore.h"
#include "../identity/DevicePresenceKey.h"

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

    virtual Result loadPresence(
        uint32_t accessEpoch,
        DevicePresenceKey& output) {
        (void)accessEpoch;
        clearDevicePresenceKey(output);
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    // expectedVersion=0 installs version 1. Later updates are strict CAS and
    // must advance exactly one version. Exact retries are idempotent.
    virtual Result replacePresenceVerified(
        uint32_t accessEpoch,
        uint32_t expectedVersion,
        const DevicePresenceKey& value) {
        (void)accessEpoch;
        (void)expectedVersion;
        (void)value;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    virtual Result bootstrapWithPresence(
        const DeviceKey& deviceKey,
        uint32_t accessEpoch,
        const ControllerCredential& initialController,
        const DevicePresenceKey& presence) {
        (void)deviceKey;
        (void)accessEpoch;
        (void)initialController;
        (void)presence;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    virtual Result bootstrapAccessWithPresence(
        uint32_t accessEpoch,
        const ControllerCredential& initialController,
        const DevicePresenceKey& presence) {
        (void)accessEpoch;
        (void)initialController;
        (void)presence;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    // Physical factory reset boundary. DeviceKey clear() preserves direct
    // access. Controller clearAll() preserves a present DeviceKey, but a
    // BLE-only access root is removed because no cloud recovery path exists.
    virtual Result eraseAccess() = 0;
};

} // namespace blinker

#endif
