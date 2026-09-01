#ifndef BLINKER_IDENTITY_DEVICEKEYSTORE_H
#define BLINKER_IDENTITY_DEVICEKEYSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "../interface/IDeviceKeyStore.h"

namespace blinker {

// Narrow crash-consistent DeviceKey root for cloud-only products. Products
// that also accept Direct controllers use DeviceAccessStore instead.
class DeviceKeyStore final : public IDeviceKeyStore {
public:
    enum : size_t { serializedSize = 44U };

    explicit DeviceKeyStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(DeviceKey& output) override;
    Result replace(const DeviceKey& key) override;
    Result clear() override;
    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    static Result encode(const DeviceKey& key, MutableByteSpan output);
    static Result decode(ByteView input, DeviceKey& output);

    IAtomicBlobStore& storage_;

    DeviceKeyStore(const DeviceKeyStore&);
    DeviceKeyStore& operator=(const DeviceKeyStore&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(DeviceKeyStore) <= 8U,
    "DeviceKey store exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
