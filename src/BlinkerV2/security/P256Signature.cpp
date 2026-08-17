#include "P256Signature.h"

namespace blinker {

namespace {

const uint8_t kP256Order[kP256PrivateKeySize] = {
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U,
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
    0xBCU, 0xE6U, 0xFAU, 0xADU, 0xA7U, 0x17U, 0x9EU, 0x84U,
    0xF3U, 0xB9U, 0xCAU, 0xC2U, 0xFCU, 0x63U, 0x25U, 0x51U
};

const uint8_t kP256HalfOrder[kP256PrivateKeySize] = {
    0x7FU, 0xFFU, 0xFFU, 0xFFU, 0x80U, 0x00U, 0x00U, 0x00U,
    0x7FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
    0xDEU, 0x73U, 0x7DU, 0x56U, 0xD3U, 0x8BU, 0xCFU, 0x42U,
    0x79U, 0xD9U, 0xE5U, 0x61U, 0x7EU, 0x31U, 0x92U, 0xA8U
};

int compareBigEndian(ByteView first, const uint8_t* second) {
    for (size_t index = 0U; index < first.size; ++index) {
        if (first.data[index] < second[index]) return -1;
        if (first.data[index] > second[index]) return 1;
    }
    return 0;
}

bool allZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

void subtractFromOrder(uint8_t* value) {
    uint16_t borrow = 0U;
    for (size_t offset = 0U; offset < kP256PrivateKeySize; ++offset) {
        const size_t index = kP256PrivateKeySize - 1U - offset;
        const uint16_t subtrahend =
            static_cast<uint16_t>(value[index]) + borrow;
        const uint16_t minuend = kP256Order[index];
        value[index] = static_cast<uint8_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
}

} // namespace

bool isValidP256Scalar(ByteView scalar) {
    return scalar.data != nullptr &&
           scalar.size == kP256PrivateKeySize &&
           !allZero(scalar) &&
           compareBigEndian(scalar, kP256Order) < 0;
}

bool isCanonicalP256PublicKey(ByteView publicKey) {
    return publicKey.data != nullptr &&
           publicKey.size == kP256PublicKeySize &&
           publicKey.data[0] == 0x04U;
}

bool isCanonicalP256Signature(ByteView signature) {
    if (signature.data == nullptr ||
        signature.size != kP256SignatureSize) {
        return false;
    }
    const ByteView r(signature.data, kP256PrivateKeySize);
    const ByteView s(
        signature.data + kP256PrivateKeySize,
        kP256PrivateKeySize);
    return isValidP256Scalar(r) &&
           isValidP256Scalar(s) &&
           compareBigEndian(s, kP256HalfOrder) <= 0;
}

Result normalizeP256Signature(MutableByteSpan signature) {
    if (signature.data == nullptr ||
        signature.size != kP256SignatureSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const ByteView r(signature.data, kP256PrivateKeySize);
    const ByteView s(
        signature.data + kP256PrivateKeySize,
        kP256PrivateKeySize);
    if (!isValidP256Scalar(r) || !isValidP256Scalar(s)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (compareBigEndian(s, kP256HalfOrder) > 0) {
        subtractFromOrder(signature.data + kP256PrivateKeySize);
    }
    return isCanonicalP256Signature(ByteView(signature.data, signature.size))
               ? Result::success()
               : Result::failure(ErrorCode::InvalidEncoding);
}

} // namespace blinker
