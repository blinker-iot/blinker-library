#ifndef BLINKER_IDENTITY_P256DEVELOPERIDENTITY_H
#define BLINKER_IDENTITY_P256DEVELOPERIDENTITY_H

#include "../interface/IAtomicBlobStore.h"
#include "../interface/IFactoryIdentity.h"
#include "../interface/IP256CryptoProvider.h"

namespace blinker {

struct P256DeveloperIdentityConfig {
    bool createIfMissing;
    bool allowPlainFlash;

    P256DeveloperIdentityConfig()
        : createIfMissing(true), allowPlainFlash(false) {}
};

// Reusable DeveloperSoftware identity state machine. Platforms provide only
// crash-atomic storage and P-256 primitives; record and wire semantics stay in
// the core library.
class P256DeveloperIdentity : public IFactoryIdentity {
public:
    P256DeveloperIdentity(
        IAtomicBlobStore& storage,
        IP256CryptoProvider& crypto,
        const P256DeveloperIdentityConfig& config =
            P256DeveloperIdentityConfig());
    ~P256DeveloperIdentity() override;

    Result begin();
    void end();
    bool ready() const { return ready_; }
    StorageProtection storageProtection() const {
        return storage_.protection();
    }

    Result physicalDeviceId(StringView& output) const override;
    Result prove(
        ByteView challenge,
        MutableByteSpan proof,
        size_t& written) override;
    FactoryIdentityKind kind() const override {
        return FactoryIdentityKind::DeveloperSoftware;
    }
    FactoryProofAlgorithm proofAlgorithm() const override {
        return FactoryProofAlgorithm::EcdsaP256Sha256Raw;
    }
    Result publicKey(
        MutableByteSpan output,
        size_t& written) const override;

private:
    Result loadExisting(ByteView record);
    Result create();
    Result installPrivateKey(ByteView privateKey);
    void clearCached();

    IAtomicBlobStore& storage_;
    IP256CryptoProvider& crypto_;
    P256DeveloperIdentityConfig config_;
    uint8_t privateKey_[kP256PrivateKeySize];
    uint8_t publicKey_[kP256PublicKeySize];
    char physicalDeviceId_[36U];
    bool ready_;

    P256DeveloperIdentity(const P256DeveloperIdentity&);
    P256DeveloperIdentity& operator=(const P256DeveloperIdentity&);
};

} // namespace blinker

#endif
