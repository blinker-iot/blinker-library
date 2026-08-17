#ifndef BLINKER_SECURITY_ED25519SERVERKEYRINGVERIFIER_H
#define BLINKER_SECURITY_ED25519SERVERKEYRINGVERIFIER_H

#include "../interface/IEd25519DigestVerifier.h"
#include "../interface/IServerSignatureVerifier.h"

namespace blinker {

struct Ed25519ServerPublicKey {
    uint32_t keyId;
    ByteView publicKey;

    Ed25519ServerPublicKey() : keyId(0U) {}
    Ed25519ServerPublicKey(uint32_t id, ByteView key)
        : keyId(id), publicKey(key) {}
};

class Ed25519ServerKeyRingVerifier : public IServerSignatureVerifier {
public:
    Ed25519ServerKeyRingVerifier(
        IEd25519DigestVerifier& crypto,
        const Ed25519ServerPublicKey* keys,
        size_t keyCount)
        : crypto_(crypto), keys_(keys), keyCount_(keyCount) {}

    Result verifyDigest(
        uint32_t keyId,
        ServerSignatureAlgorithm algorithm,
        ByteView digest,
        ByteView signature) override;

private:
    Result find(uint32_t keyId, ByteView& publicKey) const;

    IEd25519DigestVerifier& crypto_;
    const Ed25519ServerPublicKey* keys_;
    size_t keyCount_;
};

} // namespace blinker

#endif
