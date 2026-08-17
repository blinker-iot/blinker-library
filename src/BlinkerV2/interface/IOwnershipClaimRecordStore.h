#ifndef BLINKER_INTERFACE_IOWNERSHIPCLAIMRECORDSTORE_H
#define BLINKER_INTERFACE_IOWNERSHIPCLAIMRECORDSTORE_H

#include "IAtomicBlobStore.h"
#include "../provisioning/OwnershipClaimRecord.h"

namespace blinker {

class IOwnershipClaimRecordStore {
public:
    virtual ~IOwnershipClaimRecordStore() {}

    virtual Result load(OwnershipClaimRecord& output) = 0;
    virtual Result stage(const OwnershipClaimRecord& record) = 0;
    virtual Result clearExact(const OwnershipClaimRecord& expected) = 0;
    virtual Result eraseAll() = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
