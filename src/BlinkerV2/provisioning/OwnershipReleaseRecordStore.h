#ifndef BLINKER_PROVISIONING_OWNERSHIPRELEASERECORDSTORE_H
#define BLINKER_PROVISIONING_OWNERSHIPRELEASERECORDSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "../interface/IOwnershipReleaseRecordStore.h"

namespace blinker {

class OwnershipReleaseRecordStore final
    : public IOwnershipReleaseRecordStore {
public:
    enum : size_t { serializedSize = 168U };

    explicit OwnershipReleaseRecordStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(OwnershipReleaseRecord& output) override;
    Result stagePrepared(const OwnershipReleaseRecord& record) override;
    Result acceptGrant(
        const OwnershipReleaseRecord& expectedPrepared,
        ByteView grantId,
        uint32_t nextGeneration,
        ByteView grantDigest) override;
    Result acknowledgeCommit(
        const OwnershipReleaseRecord& expectedAccepted) override;
    Result clearExact(const OwnershipReleaseRecord& expected) override;
    Result eraseAll() override;

private:
    static Result encode(
        const OwnershipReleaseRecord& record,
        MutableByteSpan output);
    static Result decode(ByteView input, OwnershipReleaseRecord& record);
    Result replaceAndVerify(const OwnershipReleaseRecord& record);

    IAtomicBlobStore& storage_;
};

} // namespace blinker

#endif
