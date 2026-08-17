#ifndef BLINKER_ARDUINO_CRYPTO_ADAPTER_H
#define BLINKER_ARDUINO_CRYPTO_ADAPTER_H

#include <BlinkerV2Advanced.h>

namespace blinker {

// Portable fallback for MCUs without a suitable native crypto backend.
// Crypto@0.4.0 supplies the primitives; this adapter owns no keys or heap.
class ArduinoCryptoProvider :
    public INoiseCryptoProvider,
    public IEd25519DigestVerifier {
public:
    Result x25519PublicKey(
        ByteView privateKey,
        MutableByteSpan publicKey) override;
    Result x25519(
        ByteView privateKey,
        ByteView remotePublicKey,
        MutableByteSpan sharedSecret) override;
    Result aes256GcmEncrypt(
        ByteView key,
        ByteView nonce,
        ByteView associatedData,
        ByteView plaintext,
        MutableByteSpan output,
        size_t& written) override;
    Result aes256GcmDecrypt(
        ByteView key,
        ByteView nonce,
        ByteView associatedData,
        ByteView ciphertextAndTag,
        MutableByteSpan output,
        size_t& written) override;

    Result verifyDigest(
        ByteView publicKey,
        ByteView digest,
        ByteView signature) override;
};

} // namespace blinker

#endif
