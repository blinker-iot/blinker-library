#ifndef BLINKER_ESP32_CRYPTO_ADAPTER_H
#define BLINKER_ESP32_CRYPTO_ADAPTER_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerEsp32CryptoAdapter requires an ESP32 Arduino target"
#endif

#include <BlinkerV2Advanced.h>

namespace blinker {

class Esp32MbedTlsCryptoProvider :
    public INoiseCryptoProvider,
    public IP256DigestVerifier {
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

private:
    static int fillRandom(
        void* context,
        unsigned char* output,
        size_t size);
};

} // namespace blinker

#endif
