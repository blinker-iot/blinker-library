#ifndef BLINKER_IDENTITY_DEVICEKEYSTORE_H
#define BLINKER_IDENTITY_DEVICEKEYSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "../interface/IDeviceKeyStore.h"

namespace blinker {

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
    static Result decode(ByteView input, DeviceKey& key);

    IAtomicBlobStore& storage_;

    DeviceKeyStore(const DeviceKeyStore&);
    DeviceKeyStore& operator=(const DeviceKeyStore&);
};

} // namespace blinker

#endif
