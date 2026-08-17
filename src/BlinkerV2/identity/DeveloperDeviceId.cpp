#include "DeveloperDeviceId.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

Result deriveDeveloperPhysicalDeviceId(
    FactoryProofAlgorithm algorithm,
    ByteView publicKey,
    MutableCharSpan output,
    StringView& physicalDeviceId) {
    physicalDeviceId = StringView();
    const bool validKey =
        (algorithm == FactoryProofAlgorithm::Ed25519 &&
         publicKey.data != nullptr && publicKey.size == 32U) ||
        (algorithm == FactoryProofAlgorithm::EcdsaP256Sha256Raw &&
         publicKey.data != nullptr && publicKey.size == 65U &&
         publicKey.data[0] == 0x04U);
    if (!validKey || output.data == nullptr ||
        output.size < kDeveloperPhysicalDeviceIdSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    static const char domain[] = "blinker.developer-id.v1";
    const uint8_t algorithmByte = static_cast<uint8_t>(algorithm);
    uint8_t digest[kSha256Size] = {};
    Sha256 hash;
    Result result = hash.update(ByteView(
        reinterpret_cast<const uint8_t*>(domain),
        sizeof(domain) - 1U));
    if (result) result = hash.update(ByteView(&algorithmByte, 1U));
    if (result) result = hash.update(publicKey);
    if (result) {
        result = hash.finish(MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) {
        static const char hex[] = "0123456789abcdef";
        memcpy(output.data, "dev_", 4U);
        for (size_t index = 0U; index < 16U; ++index) {
            output.data[4U + index * 2U] = hex[digest[index] >> 4U];
            output.data[5U + index * 2U] = hex[digest[index] & 0x0FU];
        }
        physicalDeviceId =
            StringView(output.data, kDeveloperPhysicalDeviceIdSize);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

} // namespace blinker
