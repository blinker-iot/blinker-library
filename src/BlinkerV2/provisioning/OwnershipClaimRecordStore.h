#ifndef BLINKER_PROVISIONING_OWNERSHIPCLAIMRECORDSTORE_H
#define BLINKER_PROVISIONING_OWNERSHIPCLAIMRECORDSTORE_H

#include "../interface/IOwnershipClaimRecordStore.h"

namespace blinker {

class OwnershipClaimRecordStore final
    : public IOwnershipClaimRecordStore {
public:
    enum : size_t { serializedSize = 48U };

    explicit OwnershipClaimRecordStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(OwnershipClaimRecord& output) override;
    Result stage(const OwnershipClaimRecord& record) override;
    Result clearExact(const OwnershipClaimRecord& expected) override;
    Result eraseAll() override;
    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    static Result encode(
        const OwnershipClaimRecord& record,
        MutableByteSpan output);
    static Result decode(
        ByteView input,
        OwnershipClaimRecord& record);
    Result replaceAndVerify(const OwnershipClaimRecord& record);

    IAtomicBlobStore& storage_;

    OwnershipClaimRecordStore(const OwnershipClaimRecordStore&);
    OwnershipClaimRecordStore& operator=(
        const OwnershipClaimRecordStore&);
};

} // namespace blinker

#endif
