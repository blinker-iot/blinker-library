#ifndef BLINKER_IDENTITY_DEVICEACCESSSTORE_H
#define BLINKER_IDENTITY_DEVICEACCESSSTORE_H

#include "ControllerCredentialTable.h"
#include "../interface/IDeviceAccessStore.h"

namespace blinker {

// Crash-consistent access root for a device that supports both cloud and
// direct control. The device stores no account, owner or logical-device ID.
class DeviceAccessStore final : public IDeviceAccessStore {
public:
    enum : size_t {
        capacity = ControllerCredentialTable::capacity,
        legacySerializedSize = 176U,
        previousSerializedSize = 196U,
        serializedSize = 260U
    };

    explicit DeviceAccessStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(DeviceKey& output) override;
    Result replace(const DeviceKey& key) override;
    Result clear() override;

    Result loadAccessEpoch(uint32_t& output) override;
    Result loadActive(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        ControllerCredential& output) override;
    Result loadAt(size_t index, ControllerCredential& output);
    Result count(size_t& output);

    Result installVerified(
        const ControllerCredential& credential) override;
    Result rotateVerified(
        uint32_t expectedCredentialVersion,
        const ControllerCredential& credential) override;
    Result revokeVerified(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        uint32_t expectedCredentialVersion) override;
    Result clearAll() override;

    Result bootstrap(
        const DeviceKey& deviceKey,
        uint32_t accessEpoch,
        const ControllerCredential& initialController) override;
    Result bootstrapAccess(
        uint32_t accessEpoch,
        const ControllerCredential& initialController) override;
    Result loadPresence(
        uint32_t accessEpoch,
        DevicePresenceKey& output) override;
    Result replacePresenceVerified(
        uint32_t accessEpoch,
        uint32_t expectedVersion,
        const DevicePresenceKey& value) override;
    Result bootstrapWithPresence(
        const DeviceKey& deviceKey,
        uint32_t accessEpoch,
        const ControllerCredential& initialController,
        const DevicePresenceKey& presence) override;
    Result bootstrapAccessWithPresence(
        uint32_t accessEpoch,
        const ControllerCredential& initialController,
        const DevicePresenceKey& presence) override;
    Result eraseAccess() override;

    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    IAtomicBlobStore& storage_;

    DeviceAccessStore(const DeviceAccessStore&);
    DeviceAccessStore& operator=(const DeviceAccessStore&);
};

} // namespace blinker

#endif
