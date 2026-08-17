#ifndef BLINKER_SECURITY_P256SERVERKEYRINGVERIFIER_H
#define BLINKER_SECURITY_P256SERVERKEYRINGVERIFIER_H

#include "../interface/IP256DigestVerifier.h"
#include "../interface/IServerSignatureVerifier.h"

namespace blinker {

struct P256ServerPublicKey {
    uint32_t keyId;
    ByteView publicKey;

    P256ServerPublicKey() : keyId(0U) {}
    P256ServerPublicKey(uint32_t id, ByteView key)
        : keyId(id), publicKey(key) {}
};

// A borrowed, Flash-friendly key ring. Descriptors and key bytes must remain
// alive for the verifier lifetime; no heap allocation or key copy is made.
class P256ServerKeyRingVerifier : public IServerSignatureVerifier {
public:
    P256ServerKeyRingVerifier(
        IP256DigestVerifier& crypto,
        const P256ServerPublicKey* keys,
        size_t keyCount)
        : crypto_(crypto), keys_(keys), keyCount_(keyCount) {}

    Result verifyDigest(
        uint32_t keyId,
        ServerSignatureAlgorithm algorithm,
        ByteView digest,
        ByteView signature) override;

private:
    Result find(uint32_t keyId, ByteView& publicKey) const;

    IP256DigestVerifier& crypto_;
    const P256ServerPublicKey* keys_;
    size_t keyCount_;
};

} // namespace blinker

#endif
