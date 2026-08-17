#ifndef BLINKER_INTERFACE_IED25519DIGESTVERIFIER_H
#define BLINKER_INTERFACE_IED25519DIGESTVERIFIER_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kEd25519PublicKeySize = 32U,
    kEd25519SignatureSize = 64U,
    kEd25519DigestSize = 32U
};

// Verifies an Ed25519 signature over the 32-byte canonical transcript digest.
// This intentionally exposes no signing or private-key API.
class IEd25519DigestVerifier {
public:
    virtual ~IEd25519DigestVerifier() {}

    virtual Result verifyDigest(
        ByteView publicKey,
        ByteView digest,
        ByteView signature) = 0;
};

} // namespace blinker

#endif
