#include "HmacSha256.h"

#include <string.h>

#include "SecureMemory.h"

namespace blinker {

HmacSha256::HmacSha256()
    : inner_(), outerPad_(), initialized_(false), finished_(false) {}

HmacSha256::~HmacSha256() {
    clear();
}

Result HmacSha256::begin(ByteView key) {
    clear();
    if (key.size != 0U && key.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t keyBlock[64] = {};
    Result result = Result::success();
    if (key.size > sizeof(keyBlock)) {
        result = sha256(
            key,
            MutableByteSpan(keyBlock, kSha256Size));
    } else if (key.size != 0U) {
        memcpy(keyBlock, key.data, key.size);
    }
    if (!result) {
        secureZero(MutableByteSpan(keyBlock, sizeof(keyBlock)));
        return result;
    }
    uint8_t innerPad[64];
    for (size_t index = 0; index < sizeof(keyBlock); ++index) {
        innerPad[index] = static_cast<uint8_t>(keyBlock[index] ^ 0x36U);
        outerPad_[index] = static_cast<uint8_t>(keyBlock[index] ^ 0x5CU);
    }
    secureZero(MutableByteSpan(keyBlock, sizeof(keyBlock)));
    inner_.reset();
    result = inner_.update(ByteView(innerPad, sizeof(innerPad)));
    secureZero(MutableByteSpan(innerPad, sizeof(innerPad)));
    if (!result) {
        clear();
        return result;
    }
    initialized_ = true;
    finished_ = false;
    return Result::success();
}

Result HmacSha256::update(ByteView input) {
    if (!initialized_ || finished_) {
        return Result::failure(ErrorCode::ProtocolError);
    }
    return inner_.update(input);
}

Result HmacSha256::finish(MutableByteSpan output) {
    if (!initialized_ || finished_) {
        return Result::failure(ErrorCode::ProtocolError);
    }
    if (output.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.size < kSha256Size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    uint8_t innerDigest[kSha256Size];
    Result result = inner_.finish(
        MutableByteSpan(innerDigest, sizeof(innerDigest)));
    if (result) {
        Sha256 outer;
        result = outer.update(ByteView(outerPad_, sizeof(outerPad_)));
        if (result) {
            result = outer.update(ByteView(innerDigest, sizeof(innerDigest)));
        }
        if (result) result = outer.finish(output);
        outer.reset();
    }
    secureZero(MutableByteSpan(innerDigest, sizeof(innerDigest)));
    inner_.reset();
    secureZero(MutableByteSpan(outerPad_, sizeof(outerPad_)));
    finished_ = true;
    initialized_ = false;
    return result;
}

void HmacSha256::clear() {
    inner_.reset();
    secureZero(MutableByteSpan(outerPad_, sizeof(outerPad_)));
    initialized_ = false;
    finished_ = false;
}

Result hmacSha256(
    ByteView key,
    ByteView input,
    MutableByteSpan output) {
    HmacSha256 hmac;
    Result result = hmac.begin(key);
    if (result) result = hmac.update(input);
    if (result) result = hmac.finish(output);
    return result;
}

} // namespace blinker
