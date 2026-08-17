#ifndef BLINKER_IDENTITY_OWNERSHIPRECORDSTORE_H
#define BLINKER_IDENTITY_OWNERSHIPRECORDSTORE_H

#include "../interface/IOwnershipRecordStore.h"

namespace blinker {

class OwnershipRecordStore final : public IOwnershipRecordStore {
public:
    enum : size_t { serializedSize = 112U };

    explicit OwnershipRecordStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(OwnershipRecord& output) override;
    // Only an ownership coordinator that has verified the exact server grant
    // may call this. Fresh storage accepts generation 1; a tombstone accepts
    // only its exact generation. Active ownership permits exact replay only.
    Result installVerifiedActive(const OwnershipRecord& record) override;
    // Commits the generation CAS for an already verified reset/transfer.
    Result retireVerified(
        uint32_t expectedGeneration,
        uint32_t nextGeneration) override;
    // Full storage repair/whole-chip erase only. Normal ownership reset must
    // use retireVerified() so the anti-rollback generation survives.
    Result eraseAll() override;
    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    static Result encode(
        const OwnershipRecord& record,
        MutableByteSpan output);
    static Result decode(ByteView input, OwnershipRecord& record);
    Result replaceAndVerify(const OwnershipRecord& record);

    IAtomicBlobStore& storage_;

    OwnershipRecordStore(const OwnershipRecordStore&);
    OwnershipRecordStore& operator=(const OwnershipRecordStore&);
};

} // namespace blinker

#endif
