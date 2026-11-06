#ifndef BLINKER_IDENTITY_DEVICEACCESSSTORE_H
#define BLINKER_IDENTITY_DEVICEACCESSSTORE_H

#include "ControllerCredentialTable.h"
#include "../interface/IDeviceAccessStore.h"
#include "../interface/IDeviceAccessChangeObserver.h"

namespace blinker {

// Final no-I/O owner/capability check inside the existing root commit fence.
class IWifiprovRekeyCommitGuard {
public:
    virtual ~IWifiprovRekeyCommitGuard() {}
    virtual Result validateWifiprovRekeyCommit() = 0;
};

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
        : storage_(storage), observer_(nullptr) {}

    Result load(DeviceKey& output) override;
    Result replace(const DeviceKey& key) override;
    Result clear() override;

    Result loadAccessEpoch(uint32_t& output) override;
    Result loadAdminSelector(DeviceAccessAdminSelector& output) override;
    // One read of the durable root; exposes neither Key nor Admin secret.
    Result verifyInitialCloudWifiRoot(ByteView adminId, uint32_t epoch,
        uint32_t credentialVersion, const DeviceKeyLocator& locator);
    // Internal, verified WiFiProv replacement only. The caller must already
    // have authenticated the physical session, authorized the exact intent
    // and enforced its task/root policy. Public selectors are CAS, not proof.
    Result replaceWifiProvisioningKeyVerified(ByteView expectedAdminId,
        uint32_t expectedEpoch, uint32_t expectedAdminVersion,
        const DeviceKeyLocator& expectedCurrent, const DeviceKey& candidate,
        IWifiprovRekeyCommitGuard& guard);
    // Challenge/open only. This handle may not prove persistence after a failure.
    Result loadWifiProvisioningKeyVerified(ByteView adminId, uint32_t epoch,
        uint32_t adminVersion, const DeviceKeyLocator& locator, DeviceKey& output);
    // Caller must prove this separate handle is a fresh durable, read-only view
    // of the same root, not another wrapper around the attempted write handle.
    Result readDurableWifiProvisioningKeyVerified(IAtomicBlobStore& fresh,
        ByteView adminId, uint32_t epoch, uint32_t adminVersion,
        const DeviceKeyLocator& locator, DeviceKey& output);
    // The product owns this observer and serializes attachment/detachment with
    // store users. Default products install none and retain the existing path.
    void setChangeObserver(IDeviceAccessChangeObserver* observer) {
        observer_ = observer;
    }
    bool hasChangeObserver(const IDeviceAccessChangeObserver* observer) const {
        return observer_ == observer;
    }
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
    Result installInitialCloudKeyVerified(
        const DeviceKey& candidate,
        uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin) override;
    Result installInitialCloudKeyAuthorized(
        const DeviceKey& candidate, uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard& guard) override;
    Result loadPresence(
        uint32_t accessEpoch,
        DevicePresenceKey& output) override;
    Result replacePresenceVerified(
        uint32_t accessEpoch,
        uint32_t expectedVersion,
        const DevicePresenceKey& value) override;
    Result replacePresenceVerifiedCooperatively(
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
    Result installInitialCloudKeyImpl(const DeviceKey& candidate,
        uint32_t expectedEpoch, const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard* guard);
    Result replacePresenceImpl(
        uint32_t accessEpoch,
        uint32_t expectedVersion,
        const DevicePresenceKey& value,
        bool cooperative);
    IAtomicBlobStore& storage_;
    IDeviceAccessChangeObserver* observer_;

    DeviceAccessStore(const DeviceAccessStore&);
    DeviceAccessStore& operator=(const DeviceAccessStore&);
};

} // namespace blinker

#endif
