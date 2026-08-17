#include "BlinkerRenesasUnoCryptoAdapter.h"

#include <BlinkerV2/security/P256Signature.h>

namespace blinker {

RenesasUnoP256Verifier::~RenesasUnoP256Verifier() {
    end();
}

Result RenesasUnoP256Verifier::begin() {
    if (active_) return Result::success();
    if (device_.begin() != 1) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    active_ = true;
    return Result::success();
}

void RenesasUnoP256Verifier::end() {
    if (!active_) return;
    device_.end();
    active_ = false;
}

Result RenesasUnoP256Verifier::verifyDigest(
    ByteView publicKey,
    ByteView digest,
    ByteView signature) {
    if (!active_) return Result::failure(ErrorCode::NotConfigured);
    if (!isCanonicalP256PublicKey(publicKey) ||
        digest.data == nullptr || digest.size != kP256DigestSize ||
        !isCanonicalP256Signature(signature)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return device_.ecdsaVerify(
               digest.data,
               signature.data,
               publicKey.data + 1U) == 1
               ? Result::success()
               : Result::failure(ErrorCode::AuthenticationRequired);
}

} // namespace blinker
