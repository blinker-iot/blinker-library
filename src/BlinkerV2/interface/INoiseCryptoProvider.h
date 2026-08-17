#ifndef BLINKER_INTERFACE_INOISECRYPTOPROVIDER_H
#define BLINKER_INTERFACE_INOISECRYPTOPROVIDER_H

#include "IX25519AesGcmCryptoProvider.h"

namespace blinker {

// Platform crypto boundary shared by Noise_NN and Noise_NNpsk0 with
// 25519, AESGCM and SHA256.
//
// Implementations must use real X25519 and AES-256-GCM. AES-GCM output is
// ciphertext followed by the 16-byte authentication tag. Decryption must not
// expose unauthenticated plaintext and should report AuthenticationRequired
// when tag verification fails. Keys, nonces and public keys have exact sizes;
// providers must reject all other sizes.
class INoiseCryptoProvider : public IX25519AesGcmCryptoProvider {
public:
    virtual ~INoiseCryptoProvider() {}
};

} // namespace blinker

#endif
