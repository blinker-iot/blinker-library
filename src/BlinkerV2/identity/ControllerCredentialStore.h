#ifndef BLINKER_IDENTITY_CONTROLLERCREDENTIALSTORE_H
#define BLINKER_IDENTITY_CONTROLLERCREDENTIALSTORE_H

#include "ControllerCredentialTable.h"
#include "../interface/IControllerCredentialStore.h"

namespace blinker {

class ControllerCredentialStore final
    : public IControllerCredentialStore {
public:
    enum : size_t {
        capacity = ControllerCredentialTable::capacity,
        serializedSize = 140U
    };

    explicit ControllerCredentialStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result loadActive(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        ControllerCredential& output) override;
    Result loadAt(size_t index, ControllerCredential& output);
    Result count(size_t& output);

    Result installVerified(
        const ControllerCredential& credential) override;
    Result rotateVerified(
        uint32_t expectedCredentialVersion,
        const ControllerCredential& credential) override;
    Result revokeVerified(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        uint32_t expectedCredentialVersion) override;
    Result clearAll() override;

    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    Result loadTable(ControllerCredentialTable& table);
    Result replaceAndVerify(const ControllerCredentialTable& table);
    Result clearAndVerify();

    IAtomicBlobStore& storage_;

    ControllerCredentialStore(const ControllerCredentialStore&);
    ControllerCredentialStore& operator=(
        const ControllerCredentialStore&);
};

} // namespace blinker

#endif
