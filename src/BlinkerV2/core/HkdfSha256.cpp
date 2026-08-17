#include "HkdfSha256.h"

#include "SecureMemory.h"

namespace blinker {

namespace {

bool validView(ByteView value) {
    return value.data != nullptr || value.empty();
}

} // namespace

Result hkdfSha256(
    ByteView salt,
    ByteView inputKeyMaterial,
    ByteView info,
    MutableByteSpan output) {
    if (!validView(salt) || !validView(inputKeyMaterial) ||
        !validView(info) || output.data == nullptr || output.empty() ||
        output.size > 255U * kSha256Size) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint8_t zeroSalt[kSha256Size] = {};
    uint8_t pseudoRandomKey[kSha256Size] = {};
    uint8_t previous[kSha256Size] = {};
    const ByteView effectiveSalt = salt.empty()
                                       ? ByteView(zeroSalt, sizeof(zeroSalt))
                                       : salt;
    Result result = hmacSha256(
        effectiveSalt,
        inputKeyMaterial,
        MutableByteSpan(pseudoRandomKey, sizeof(pseudoRandomKey)));

    size_t offset = 0U;
    uint8_t counter = 1U;
    size_t previousSize = 0U;
    while (result && offset < output.size) {
        HmacSha256 hmac;
        result = hmac.begin(ByteView(
            pseudoRandomKey,
            sizeof(pseudoRandomKey)));
        if (result && previousSize != 0U) {
            result = hmac.update(ByteView(previous, previousSize));
        }
        if (result) result = hmac.update(info);
        if (result) result = hmac.update(ByteView(&counter, 1U));
        if (result) {
            result = hmac.finish(MutableByteSpan(
                previous,
                sizeof(previous)));
        }
        if (result) {
            const size_t remaining = output.size - offset;
            const size_t copySize = remaining < sizeof(previous)
                                        ? remaining
                                        : sizeof(previous);
            for (size_t index = 0U; index < copySize; ++index) {
                output.data[offset + index] = previous[index];
            }
            offset += copySize;
            previousSize = sizeof(previous);
            ++counter;
        }
    }

    secureZero(MutableByteSpan(zeroSalt, sizeof(zeroSalt)));
    secureZero(MutableByteSpan(
        pseudoRandomKey,
        sizeof(pseudoRandomKey)));
    secureZero(MutableByteSpan(previous, sizeof(previous)));
    return result;
}

} // namespace blinker
