#include "P256ServerKeyRingVerifier.h"

#include "P256Signature.h"

namespace blinker {

Result P256ServerKeyRingVerifier::find(
    uint32_t keyId,
    ByteView& publicKey) const {
    publicKey = ByteView();
    if (keys_ == nullptr || keyCount_ == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    size_t matches = 0U;
    for (size_t index = 0U; index < keyCount_; ++index) {
        if (keys_[index].keyId == 0U ||
            !isCanonicalP256PublicKey(keys_[index].publicKey)) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        for (size_t previous = 0U; previous < index; ++previous) {
            if (keys_[previous].keyId == keys_[index].keyId) {
                return Result::failure(ErrorCode::NotConfigured);
            }
        }
        if (keys_[index].keyId == keyId) {
            publicKey = keys_[index].publicKey;
            ++matches;
        }
    }
    return matches == 1U
               ? Result::success()
               : Result::failure(ErrorCode::AuthenticationRequired);
}

Result P256ServerKeyRingVerifier::verifyDigest(
    uint32_t keyId,
    ServerSignatureAlgorithm algorithm,
    ByteView digest,
    ByteView signature) {
    if (keyId == 0U ||
        algorithm != ServerSignatureAlgorithm::EcdsaP256Sha256Raw ||
        digest.data == nullptr || digest.size != kP256DigestSize ||
        !isCanonicalP256Signature(signature)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    ByteView publicKey;
    Result result = find(keyId, publicKey);
    if (!result) return result;
    return crypto_.verifyDigest(publicKey, digest, signature);
}

} // namespace blinker
