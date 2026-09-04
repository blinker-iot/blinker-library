#ifndef BLINKER_SECURITY_GATEWAYCREDENTIALRENEWALENVELOPESECURITY_H
#define BLINKER_SECURITY_GATEWAYCREDENTIALRENEWALENVELOPESECURITY_H

#include "GatewayAccessEnvelopeSecurity.h"

namespace blinker {

// A second HKDF domain rooted in the identity-bound Gateway access wrap key.
// Renewal ciphertext can therefore never be opened as an initial-access
// envelope even when DeviceKey, context and nonce happen to be identical.
Result deriveGatewayCredentialRenewalWrapKey(
    const DeviceKey& deviceKey,
    const GatewayAccessWrapContext& context,
    MutableByteSpan output);

// Opens the 32-byte candidate Gateway secret and verifies the envelope's
// SHA-256(candidateSecret || currentPresenceKey) binding. All caller-owned
// scratch and failed plaintext are cleared before return.
Result openGatewayCredentialRenewalEnvelope(
    const DeviceKey& deviceKey,
    const gateway::GatewayCredentialRenewalEnvelopeView& envelope,
    ByteView currentPresenceKey,
    IX25519AesGcmCryptoProvider& crypto,
    MutableByteSpan aadScratch,
    MutableByteSpan candidateSecret);

} // namespace blinker

#endif
