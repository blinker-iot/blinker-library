#ifndef BLINKER_INTERFACE_IDEVICEKEYSOURCE_H
#define BLINKER_INTERFACE_IDEVICEKEYSOURCE_H

#include "IAtomicBlobStore.h"
#include "../identity/DeviceKey.h"

namespace blinker {

class IDeviceKeySource {
public:
    virtual ~IDeviceKeySource() {}

    virtual Result load(DeviceKey& output) = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
