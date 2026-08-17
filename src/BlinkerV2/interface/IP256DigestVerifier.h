#ifndef BLINKER_INTERFACE_IP256DIGESTVERIFIER_H
#define BLINKER_INTERFACE_IP256DIGESTVERIFIER_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kP256PrivateKeySize = 32U,
    kP256PublicKeySize = 65U,
    kP256SignatureSize = 64U,
    kP256DigestSize = 32U
};

// Narrow platform seam for server signature verification. Implementations
// receive an uncompressed SEC1 public key and a raw low-S r||s signature.
class IP256DigestVerifier {
public:
    virtual ~IP256DigestVerifier() {}

    virtual Result verifyDigest(
        ByteView publicKey,
        ByteView digest,
        ByteView signature) = 0;
};

} // namespace blinker

#endif
