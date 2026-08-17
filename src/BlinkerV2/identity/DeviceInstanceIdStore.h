#ifndef BLINKER_IDENTITY_DEVICEINSTANCEIDSTORE_H
#define BLINKER_IDENTITY_DEVICEINSTANCEIDSTORE_H

#include "DeviceInstanceId.h"
#include "../interface/IAtomicBlobStore.h"
#include "../interface/IRandom.h"

namespace blinker {

// Stores the public instance ID as one crash-consistent record. Corrupt data
// is never silently replaced because that would change the physical identity.
class DeviceInstanceIdStore {
public:
    enum : size_t { serializedSize = 28U };

    explicit DeviceInstanceIdStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(DeviceInstanceId& output);
    Result loadOrCreate(IRandom& random, DeviceInstanceId& output);
    StorageProtection protection() const { return storage_.protection(); }

private:
    static Result encode(
        const DeviceInstanceId& value,
        MutableByteSpan output);
    static Result decode(ByteView input, DeviceInstanceId& output);

    IAtomicBlobStore& storage_;

    DeviceInstanceIdStore(const DeviceInstanceIdStore&);
    DeviceInstanceIdStore& operator=(const DeviceInstanceIdStore&);
};

} // namespace blinker

#endif
