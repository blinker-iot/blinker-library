#ifndef BLINKER_IDENTITY_CONTROLLERCREDENTIALSTORE_H
#define BLINKER_IDENTITY_CONTROLLERCREDENTIALSTORE_H

#include "../interface/IControllerCredentialStore.h"

namespace blinker {

class ControllerCredentialStore final
    : public IControllerCredentialStore {
public:
    enum : size_t {
        capacity = 4U,
        serializedSize = 268U
    };

    explicit ControllerCredentialStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result loadActive(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t ownershipGeneration,
        ControllerCredential& output) override;
    Result loadAt(size_t index, ControllerCredential& output);
    Result count(size_t& output);

    // These methods accept only mutations already authorized by the
    // controller control-plane. Persistence intentionally does not verify
    // grants or transport transcripts.
    Result installVerified(
        const ControllerCredential& credential) override;
    Result rotateVerified(
        uint32_t expectedCredentialVersion,
        const ControllerCredential& credential) override;
    Result revokeVerified(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t ownershipGeneration,
        uint32_t expectedCredentialVersion) override;
    Result clearAll() override;

    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    static Result encode(
        const ControllerCredential* credentials,
        size_t count,
        MutableByteSpan output);
    static Result decode(
        ByteView input,
        ControllerCredential* credentials,
        size_t& count);
    Result loadTable(
        ControllerCredential* credentials,
        size_t& count);
    Result replaceAndVerify(
        const ControllerCredential* credentials,
        size_t count);
    Result clearAndVerify();

    IAtomicBlobStore& storage_;

    ControllerCredentialStore(const ControllerCredentialStore&);
    ControllerCredentialStore& operator=(
        const ControllerCredentialStore&);
};

} // namespace blinker

#endif
