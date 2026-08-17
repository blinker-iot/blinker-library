#ifndef BLINKER_INTERFACE_IFACTORYIDENTITY_H
#define BLINKER_INTERFACE_IFACTORYIDENTITY_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class FactoryIdentityKind : uint8_t {
    DeveloperSoftware = 0,
    SymmetricKey,
    AsymmetricKey,
    SecureElement
};

enum class FactoryProofAlgorithm : uint8_t {
    HmacSha256 = 1,
    Ed25519 = 2,
    EcdsaP256Sha256Raw = 3
};

// Bootstrap key material never leaves this interface. physicalDeviceId() must
// return a stable view for at least the lifetime of the identity adapter.
class IFactoryIdentity {
public:
    virtual ~IFactoryIdentity() {}

    virtual Result physicalDeviceId(StringView& output) const = 0;
    virtual Result prove(
        ByteView challenge,
        MutableByteSpan proof,
        size_t& written) = 0;
    virtual FactoryIdentityKind kind() const = 0;
    virtual FactoryProofAlgorithm proofAlgorithm() const = 0;

    // DeveloperSoftware identities must expose their public key for first
    // enrollment. Symmetric identities return NotFound with written == 0.
    // Private/bootstrap key material is never returned by this interface.
    virtual Result publicKey(
        MutableByteSpan output,
        size_t& written) const = 0;
};

} // namespace blinker

#endif
