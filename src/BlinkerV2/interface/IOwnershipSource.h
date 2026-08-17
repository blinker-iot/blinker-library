#ifndef BLINKER_INTERFACE_IOWNERSHIPSOURCE_H
#define BLINKER_INTERFACE_IOWNERSHIPSOURCE_H

#include "IAtomicBlobStore.h"
#include "../identity/OwnershipRecord.h"

namespace blinker {

class IOwnershipSource {
public:
    virtual ~IOwnershipSource() {}

    virtual Result load(OwnershipRecord& output) = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
