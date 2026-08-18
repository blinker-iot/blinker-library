#ifndef BLINKER_INTERFACE_IACCESSEPOCHSOURCE_H
#define BLINKER_INTERFACE_IACCESSEPOCHSOURCE_H

#include "IAtomicBlobStore.h"

namespace blinker {

// Minimal anti-replay boundary for server-managed direct access. Account,
// owner and logical-device data remain server-side.
class IAccessEpochSource {
public:
    virtual ~IAccessEpochSource() {}

    virtual Result loadAccessEpoch(uint32_t& output) = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
