#ifndef BLINKER_INTERFACE_IOWNERSHIPRECORDSTORE_H
#define BLINKER_INTERFACE_IOWNERSHIPRECORDSTORE_H

#include "IOwnershipSource.h"

namespace blinker {

// Mutation boundary used only after the ownership control-plane has verified
// an exact server grant. Wire decoding never belongs in the storage adapter.
class IOwnershipRecordStore : public IOwnershipSource {
public:
    virtual Result installVerifiedActive(
        const OwnershipRecord& record) = 0;
    virtual Result retireVerified(
        uint32_t expectedGeneration,
        uint32_t nextGeneration) = 0;
    virtual Result eraseAll() = 0;
};

} // namespace blinker

#endif
