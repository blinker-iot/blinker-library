#ifndef BLINKER_PROVISIONING_LOCALSETUPSAGASTORE_H
#define BLINKER_PROVISIONING_LOCALSETUPSAGASTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "LocalSetupSagaRecord.h"

namespace blinker {

class LocalSetupSagaStore {
public:
    enum : size_t { serializedSize = 248U };

    explicit LocalSetupSagaStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(LocalSetupSagaRecord& output);
    Result save(const LocalSetupSagaRecord& record);
    Result clear();
    StorageProtection protection() const { return storage_.protection(); }

    static Result encode(const LocalSetupSagaRecord& record,
                         MutableByteSpan output);
    static Result decode(ByteView input, LocalSetupSagaRecord& record);

private:
    IAtomicBlobStore& storage_;

    LocalSetupSagaStore(const LocalSetupSagaStore&);
    LocalSetupSagaStore& operator=(const LocalSetupSagaStore&);
};

} // namespace blinker

#endif
