#ifndef BLINKER_IDENTITY_CLOUDCREDENTIALSTORE_H
#define BLINKER_IDENTITY_CLOUDCREDENTIALSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "../interface/ICloudCredentialStore.h"

namespace blinker {

class CloudCredentialStore final : public ICloudCredentialStore {
public:
    enum : size_t { serializedSize = 116U };

    explicit CloudCredentialStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(CloudCredential& output) override;
    // Fresh-device/factory path. Exact replay succeeds; a different existing
    // credential is never overwritten.
    Result installFresh(const CloudCredential& credential) override;
    Result clear() override;
    StorageProtection protection() const override {
        return storage_.protection();
    }

private:
    static Result encode(
        const CloudCredential& credential,
        MutableByteSpan output);
    static Result decode(ByteView input, CloudCredential& credential);

    IAtomicBlobStore& storage_;

    CloudCredentialStore(const CloudCredentialStore&);
    CloudCredentialStore& operator=(const CloudCredentialStore&);
};

} // namespace blinker

#endif
