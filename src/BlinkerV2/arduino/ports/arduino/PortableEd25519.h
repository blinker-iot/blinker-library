#ifndef BLINKER_ARDUINO_PORTABLE_ED25519_H
#define BLINKER_ARDUINO_PORTABLE_ED25519_H

#include <BlinkerV2/interface/IEd25519DigestVerifier.h>
#include <BlinkerV2/security/Ed25519Signature.h>
#include <Ed25519.h>

namespace blinker {

// Signature-only port. WiFi tasks need no Noise/X25519/AES-GCM provider.
// The full Direct crypto provider delegates here so canonical checks stay unique.
class ArduinoEd25519DigestVerifier : public IEd25519DigestVerifier {
public:
    Result verifyDigest(ByteView publicKey, ByteView digest, ByteView signature) override {
        if (publicKey.data == nullptr || publicKey.size != kEd25519PublicKeySize ||
            digest.data == nullptr || digest.size != kEd25519DigestSize ||
            signature.data == nullptr || signature.size != kEd25519SignatureSize) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (!isCanonicalEd25519PublicKey(publicKey) || !isCanonicalEd25519Signature(signature)) {
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        return Ed25519::verify(signature.data, publicKey.data, digest.data, digest.size)
                   ? Result::success()
                   : Result::failure(ErrorCode::AuthenticationRequired);
    }
};

} // namespace blinker
#endif
