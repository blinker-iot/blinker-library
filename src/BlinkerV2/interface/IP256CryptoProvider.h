#ifndef BLINKER_INTERFACE_IP256CRYPTOPROVIDER_H
#define BLINKER_INTERFACE_IP256CRYPTOPROVIDER_H

#include "IP256DigestVerifier.h"

namespace blinker {

// Platform crypto seam. Private keys are borrowed only for the duration of a
// call and must never be retained or exported by an implementation.
class IP256CryptoProvider : public IP256DigestVerifier {
public:
    virtual ~IP256CryptoProvider() {}

    virtual Result generatePrivateKey(MutableByteSpan privateKey) = 0;
    virtual Result derivePublicKey(
        ByteView privateKey,
        MutableByteSpan publicKey) = 0;
    virtual Result signDigest(
        ByteView privateKey,
        ByteView digest,
        MutableByteSpan signature) = 0;
};

} // namespace blinker

#endif
