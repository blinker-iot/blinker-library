#ifndef BLINKER_INTERFACE_ICLOUDENROLLMENTRECORDSTORE_H
#define BLINKER_INTERFACE_ICLOUDENROLLMENTRECORDSTORE_H

#include "IAtomicBlobStore.h"
#include "../provisioning/CloudEnrollmentRecord.h"

namespace blinker {

class ICloudEnrollmentRecordStore {
public:
    virtual ~ICloudEnrollmentRecordStore() {}

    virtual Result load(CloudEnrollmentRecord& output) = 0;
    virtual Result stagePrepared(const CloudEnrollmentRecord& record) = 0;
    virtual Result acceptGrant(
        const CloudEnrollmentRecord& expectedPrepared,
        ByteView grantDigest) = 0;
    virtual Result clearExact(const CloudEnrollmentRecord& expected) = 0;
    virtual Result eraseAll() = 0;
    virtual StorageProtection protection() const = 0;
};

} // namespace blinker

#endif
