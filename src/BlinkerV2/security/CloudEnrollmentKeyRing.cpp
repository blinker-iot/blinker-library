#include "CloudEnrollmentKeyRing.h"

#include <string.h>

namespace blinker {

Result CloudEnrollmentKeyRing::validate() const {
    if (activeKeyId_ == 0U || keys_ == nullptr || keyCount_ == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    size_t activeMatches = 0U;
    for (size_t index = 0U; index < keyCount_; ++index) {
        const CloudEnrollmentPublicKey& key = keys_[index];
        if (key.keyId == 0U || key.publicKey.data == nullptr ||
            key.publicKey.size != kX25519KeySize) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        uint8_t combined = 0U;
        for (size_t byte = 0U; byte < key.publicKey.size; ++byte) {
            combined = static_cast<uint8_t>(combined | key.publicKey.data[byte]);
        }
        if (combined == 0U) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        for (size_t previous = 0U; previous < index; ++previous) {
            if (keys_[previous].keyId == key.keyId) {
                return Result::failure(ErrorCode::NotConfigured);
            }
        }
        if (key.keyId == activeKeyId_) ++activeMatches;
    }
    return activeMatches == 1U
               ? Result::success()
               : Result::failure(ErrorCode::NotConfigured);
}

Result CloudEnrollmentKeyRing::activeKeyId(uint32_t& keyId) {
    keyId = 0U;
    const Result result = validate();
    if (result) keyId = activeKeyId_;
    return result;
}

Result CloudEnrollmentKeyRing::loadPublicKey(
    uint32_t keyId,
    MutableByteSpan publicKey) {
    Result result = validate();
    if (!result) return result;
    if (keyId == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (publicKey.data == nullptr || publicKey.size < kX25519KeySize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    for (size_t index = 0U; index < keyCount_; ++index) {
        if (keys_[index].keyId == keyId) {
            memcpy(publicKey.data, keys_[index].publicKey.data, kX25519KeySize);
            return Result::success();
        }
    }
    return Result::failure(ErrorCode::NotFound);
}

} // namespace blinker
