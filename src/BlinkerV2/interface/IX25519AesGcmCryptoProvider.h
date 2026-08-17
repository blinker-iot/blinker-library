#ifndef BLINKER_INTERFACE_IX25519AESGCMCRYPTOPROVIDER_H
#define BLINKER_INTERFACE_IX25519AESGCMCRYPTOPROVIDER_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kX25519KeySize = 32U,
    kAes256KeySize = 32U,
    kAesGcmNonceSize = 12U,
    kAesGcmTagSize = 16U
};

// Reusable platform seam for protocols that need X25519 and AES-256-GCM.
// AES-GCM output is ciphertext followed by its 16-byte authentication tag.
class IX25519AesGcmCryptoProvider {
public:
    virtual ~IX25519AesGcmCryptoProvider() {}

    virtual Result x25519PublicKey(
        ByteView privateKey,
        MutableByteSpan publicKey) = 0;
    virtual Result x25519(
        ByteView privateKey,
        ByteView remotePublicKey,
        MutableByteSpan sharedSecret) = 0;
    virtual Result aes256GcmEncrypt(
        ByteView key,
        ByteView nonce,
        ByteView associatedData,
        ByteView plaintext,
        MutableByteSpan output,
        size_t& written) = 0;
    virtual Result aes256GcmDecrypt(
        ByteView key,
        ByteView nonce,
        ByteView associatedData,
        ByteView ciphertextAndTag,
        MutableByteSpan output,
        size_t& written) = 0;
};

} // namespace blinker

#endif
