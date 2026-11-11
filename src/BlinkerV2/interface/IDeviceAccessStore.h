#ifndef BLINKER_INTERFACE_IDEVICEACCESSSTORE_H
#define BLINKER_INTERFACE_IDEVICEACCESSSTORE_H

#include "IAccessEpochSource.h"
#include "IControllerCredentialStore.h"
#include "IDeviceKeyStore.h"
#include "IInitialCloudKeyInstaller.h"
#include "../identity/DevicePresenceKey.h"

namespace blinker {

// Non-secret selector for the one installed PlatformAccess Admin. It is not an
// authorization proof and does not contain a key, secret or task binding.
struct DeviceAccessAdminSelector {
    uint8_t controllerId[kControllerIdSize];
    uint32_t accessEpoch;
    DeviceAccessAdminSelector() : controllerId(), accessEpoch(0U) {}
};

// Atomic optional DeviceKey + direct-controller root. Cloud/direct products
// bootstrap both domains together; BLE-only products bootstrap only access.
class IDeviceAccessStore :
    public IDeviceKeyStore,
    public IControllerCredentialStore,
    public IAccessEpochSource {
public:
    virtual Result loadAdminSelector(DeviceAccessAdminSelector& output) {
        output = DeviceAccessAdminSelector();
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    virtual Result bootstrap(
        const DeviceKey& deviceKey,
        uint32_t accessEpoch,
        const ControllerCredential& initialController) = 0;

    virtual Result bootstrapAccess(
        uint32_t accessEpoch,
        const ControllerCredential& initialController) = 0;

    // Internal, separately authorized first Key addition under the original
    // Admin/epoch CAS and product-owned task fence. Not an Install wire API,
    // rotation, bootstrap, scope migration or permission to infer an empty store.
    virtual Result installInitialCloudKeyVerified(
        const DeviceKey& candidate,
        uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin) {
        (void)candidate; (void)expectedEpoch; (void)expectedAdmin;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    // Network-facing internal consumers additionally require a final live
    // session/intent check at the actual root commit boundary. Old ports must
    // not silently downgrade this guarded call to the trusted-only method.
    virtual Result installInitialCloudKeyAuthorized(
        const DeviceKey& candidate, uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard& guard) {
        (void)candidate; (void)expectedEpoch; (void)expectedAdmin; (void)guard;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

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

    // Narrow synchronous PresenceKey control path. Products without a
    // cooperative owner retain their original CAS and mutation fence.
    virtual Result replacePresenceVerifiedCooperatively(
        uint32_t accessEpoch,
        uint32_t expectedVersion,
        const DevicePresenceKey& value) {
        return replacePresenceVerified(accessEpoch, expectedVersion, value);
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
