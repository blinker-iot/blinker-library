#ifndef BLINKER_PROVISIONING_CLOUDENROLLMENTRECORDSTORE_H
#define BLINKER_PROVISIONING_CLOUDENROLLMENTRECORDSTORE_H

#include "../interface/ICloudEnrollmentRecordStore.h"

namespace blinker {

class CloudEnrollmentRecordStore final
    : public ICloudEnrollmentRecordStore {
public:
    enum : size_t { serializedSize = 136U };

    explicit CloudEnrollmentRecordStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(CloudEnrollmentRecord& output) override;
    Result stagePrepared(const CloudEnrollmentRecord& record) override;
    Result acceptGrant(
        const CloudEnrollmentRecord& expectedPrepared,
        ByteView grantDigest) override;
    Result clearExact(const CloudEnrollmentRecord& expected) override;
    Result eraseAll() override;
    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    static Result encode(
        const CloudEnrollmentRecord& record,
        MutableByteSpan output);
    static Result decode(
        ByteView input,
        CloudEnrollmentRecord& record);
    Result replaceAndVerify(const CloudEnrollmentRecord& record);

    IAtomicBlobStore& storage_;

    CloudEnrollmentRecordStore(const CloudEnrollmentRecordStore&);
    CloudEnrollmentRecordStore& operator=(
        const CloudEnrollmentRecordStore&);
};

} // namespace blinker

#endif
