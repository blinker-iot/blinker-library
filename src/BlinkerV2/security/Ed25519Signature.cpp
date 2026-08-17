#include "Ed25519Signature.h"

namespace blinker {

namespace {

const uint8_t kEd25519Order[kEd25519PublicKeySize] = {
    0xEDU, 0xD3U, 0xF5U, 0x5CU, 0x1AU, 0x63U, 0x12U, 0x58U,
    0xD6U, 0x9CU, 0xF7U, 0xA2U, 0xDEU, 0xF9U, 0xDEU, 0x14U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x10U};

int compareLittleEndian(
    const uint8_t* first,
    const uint8_t* second,
    size_t size,
    bool maskPointSign) {
    for (size_t offset = 0U; offset < size; ++offset) {
        const size_t index = size - 1U - offset;
        uint8_t firstByte = first[index];
        if (maskPointSign && index == size - 1U) firstByte &= 0x7FU;
        if (firstByte < second[index]) return -1;
        if (firstByte > second[index]) return 1;
    }
    return 0;
}

bool canonicalPoint(const uint8_t* point) {
    static const uint8_t fieldPrime[kEd25519PublicKeySize] = {
        0xEDU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
        0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
        0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
        0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x7FU};
    if (compareLittleEndian(
            point,
            fieldPrime,
            kEd25519PublicKeySize,
            true) >= 0) {
        return false;
    }
    uint8_t combined = 0U;
    for (size_t index = 0U; index < kEd25519PublicKeySize; ++index) {
        combined = static_cast<uint8_t>(combined | point[index]);
    }
    if (combined == 0U) return false;
    if (point[0] != 0x01U) return true;
    for (size_t index = 1U; index < kEd25519PublicKeySize; ++index) {
        if (point[index] != 0U) return true;
    }
    return false;
}

} // namespace

bool isCanonicalEd25519PublicKey(ByteView publicKey) {
    return publicKey.data != nullptr &&
           publicKey.size == kEd25519PublicKeySize &&
           canonicalPoint(publicKey.data);
}

bool isCanonicalEd25519Signature(ByteView signature) {
    return signature.data != nullptr &&
           signature.size == kEd25519SignatureSize &&
           canonicalPoint(signature.data) &&
           compareLittleEndian(
               signature.data + kEd25519PublicKeySize,
               kEd25519Order,
               kEd25519PublicKeySize,
               false) < 0;
}

} // namespace blinker
