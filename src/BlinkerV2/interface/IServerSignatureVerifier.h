#ifndef BLINKER_INTERFACE_ISERVERSIGNATUREVERIFIER_H
#define BLINKER_INTERFACE_ISERVERSIGNATUREVERIFIER_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class ServerSignatureAlgorithm : uint8_t {
    Ed25519 = 1,
    EcdsaP256Sha256Raw = 2
};

// Verifies a SHA-256 digest against a pinned server key selected by keyId.
// Signatures use the fixed wire format of their algorithm; P-256 is raw r||s.
class IServerSignatureVerifier {
public:
    virtual ~IServerSignatureVerifier() {}

    virtual Result verifyDigest(
        uint32_t keyId,
        ServerSignatureAlgorithm algorithm,
        ByteView digest,
        ByteView signature) = 0;
};

} // namespace blinker

#endif
