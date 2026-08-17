#ifndef BLINKER_INTERFACE_IOWNERSHIPRELEASERECORDSTORE_H
#define BLINKER_INTERFACE_IOWNERSHIPRELEASERECORDSTORE_H

#include "../provisioning/OwnershipReleaseRecord.h"

namespace blinker {

class IOwnershipReleaseRecordStore {
public:
    virtual ~IOwnershipReleaseRecordStore() {}

    virtual Result load(OwnershipReleaseRecord& output) = 0;
    virtual Result stagePrepared(const OwnershipReleaseRecord& record) = 0;
    virtual Result acceptGrant(
        const OwnershipReleaseRecord& expectedPrepared,
        ByteView grantId,
        uint32_t nextGeneration,
        ByteView grantDigest) = 0;
    virtual Result acknowledgeCommit(
        const OwnershipReleaseRecord& expectedAccepted) = 0;
    virtual Result clearExact(const OwnershipReleaseRecord& expected) = 0;
    virtual Result eraseAll() = 0;
};

} // namespace blinker

#endif
