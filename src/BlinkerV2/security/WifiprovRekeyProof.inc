#include "WifiprovRekeyProof.h"

#include <string.h>

#include "../core/HmacSha256.h"
#include "../core/SecureMemory.h"

namespace blinker {
namespace security {
namespace {

// The terminator is included in all three domain prefixes.
const uint8_t kRekeyChallengeDomain[] = "blinker.wifiprov-rekey.challenge.v1";
const uint8_t kRekeyWrapDomain[] = "blinker.wifiprov-rekey.wrap.v1";
const uint8_t kRekeyInstalledDomain[] = "blinker.wifiprov-rekey.installed.v1";
const uint8_t kRekeyWifiPermitDomain[] = "blinker.wifiprov-rekey.wifi-permit.v1";
const uint8_t kRekeySetupAdminDomain[] = "blinker.wifiprov-rekey.setup-admin.v1";

enum : size_t {
    kRekeyChallengeOffset = kWifiprovRekeyOperationSize,
    kRekeyInstanceOffset = kRekeyChallengeOffset + kWifiprovRekeyChallengeSize,
    kRekeyEpochOffset = kRekeyInstanceOffset + kDeviceInstanceIdSize,
    kRekeyVersionOffset = kRekeyEpochOffset + 4U,
    kRekeyOldLocatorOffset = kRekeyVersionOffset + 4U,
    kRekeyOldBindingSize = kRekeyOldLocatorOffset + kDeviceKeyLocatorSize,
    kRekeyNewBindingSize = kRekeyOldBindingSize + kDeviceKeyLocatorSize
};

static_assert(static_cast<size_t>(kRekeyOldBindingSize) == kWifiprovRekeyOldBindingSize &&
    static_cast<size_t>(kRekeyNewBindingSize) == kWifiprovRekeyBindingSize, "WiFiProv rekey binding sizes");
static_assert(static_cast<size_t>(kWifiprovRekeyMacSize) == kSha256Size, "WiFiProv rekey MAC size");

bool nonzeroRekeyBytes(ByteView bytes, size_t size) {
    if (bytes.data == nullptr || bytes.size != size) return false;
    uint8_t combined = 0U;
    for (size_t i = 0U; i < size; ++i) combined = static_cast<uint8_t>(combined | bytes.data[i]);
    return combined != 0U;
}

ByteView rekeyLocator(const DeviceKeyLocator& locator) {
    return ByteView(locator.bytes, sizeof(locator.bytes));
}

void writeRekeyU32(uint8_t* output, uint32_t value) {
    for (size_t i = 0U; i < 4U; ++i) output[i] = static_cast<uint8_t>(value >> (24U - 8U * i));
}

uint32_t readRekeyU32(const uint8_t* input) {
    uint32_t value = 0U;
    for (size_t i = 0U; i < 4U; ++i) value = (value << 8U) | input[i];
    return value;
}

bool validRekeyOldBinding(const WifiprovRekeyOldBinding& binding) {
    return nonzeroRekeyBytes(ByteView(binding.operationId, sizeof(binding.operationId)), kWifiprovRekeyOperationSize) &&
        nonzeroRekeyBytes(ByteView(binding.challenge, sizeof(binding.challenge)), kWifiprovRekeyChallengeSize) &&
        isValidDeviceInstanceId(binding.instance) && binding.accessEpoch != 0U &&
        binding.fromVersion != 0U && binding.fromVersion != UINT32_MAX &&
        nonzeroRekeyBytes(rekeyLocator(binding.oldLocator), kDeviceKeyLocatorSize);
}

bool validRekeyNewLocator(const WifiprovRekeyBinding& binding) {
    return nonzeroRekeyBytes(rekeyLocator(binding.newLocator), kDeviceKeyLocatorSize) &&
        !constantTimeEqual(rekeyLocator(binding.previous.oldLocator), rekeyLocator(binding.newLocator));
}

Result encodeRekeyOldBinding(const WifiprovRekeyOldBinding& binding, uint8_t* output) {
    if (!validRekeyOldBinding(binding)) return Result::failure(ErrorCode::InvalidArgument);
    memcpy(output, binding.operationId, sizeof(binding.operationId));
    memcpy(output + kRekeyChallengeOffset, binding.challenge, sizeof(binding.challenge));
    memcpy(output + kRekeyInstanceOffset, binding.instance.bytes, sizeof(binding.instance.bytes));
    writeRekeyU32(output + kRekeyEpochOffset, binding.accessEpoch);
    writeRekeyU32(output + kRekeyVersionOffset, binding.fromVersion);
    memcpy(output + kRekeyOldLocatorOffset, binding.oldLocator.bytes, sizeof(binding.oldLocator.bytes));
    return Result::success();
}

Result encodeRekeyNewBinding(const WifiprovRekeyBinding& binding, uint8_t* output) {
    if (!validRekeyNewLocator(binding)) return Result::failure(ErrorCode::InvalidArgument);
    const Result result = encodeRekeyOldBinding(binding.previous, output);
    if (result) memcpy(output + kRekeyOldBindingSize, binding.newLocator.bytes, sizeof(binding.newLocator.bytes));
    return result;
}

Result checkRekeyLocator(const DeviceKey& key, const DeviceKeyLocator& expected) {
    DeviceKeyLocator actual;
    const Result result = deriveDeviceKeyLocator(key, actual);
    if (!result) return result;
    return constantTimeEqual(rekeyLocator(actual), rekeyLocator(expected))
        ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
}

Result rekeyMac(ByteView key, ByteView domain, ByteView binding, MutableByteSpan output) {
    HmacSha256 hmac;
    Result result = hmac.begin(key);
    if (result) result = hmac.update(domain);
    if (result) result = hmac.update(binding);
    if (result) result = hmac.finish(output);
    return result;
}

Result rekeyMac(const DeviceKey& key, ByteView domain, ByteView binding, MutableByteSpan output) {
    return rekeyMac(ByteView(key.bytes, sizeof(key.bytes)), domain, binding, output);
}

Result verifyRekeyMac(ByteView key, ByteView domain, ByteView encoded, ByteView given) {
    if (!nonzeroRekeyBytes(given, kWifiprovRekeyMacSize)) return Result::failure(ErrorCode::AuthenticationRequired);
    WifiprovRekeyMac expected;
    const MutableByteSpan scratch(expected.bytes, sizeof(expected.bytes));
    const Result result = rekeyMac(key, domain, encoded, scratch);
    const bool matches = result && constantTimeEqual(ByteView(expected.bytes, sizeof(expected.bytes)), given);
    secureZero(scratch);
    if (!result) return result;
    return matches ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
}

} // namespace

Result decodeWifiprovRekeyOldBinding(ByteView bytes, WifiprovRekeyOldBinding& binding) {
    if (bytes.data == nullptr || bytes.size != kWifiprovRekeyOldBindingSize)
        return Result::failure(ErrorCode::InvalidArgument);
    WifiprovRekeyOldBinding decoded;
    memcpy(decoded.operationId, bytes.data, sizeof(decoded.operationId));
    memcpy(decoded.challenge, bytes.data + kRekeyChallengeOffset, sizeof(decoded.challenge));
    memcpy(decoded.instance.bytes, bytes.data + kRekeyInstanceOffset, sizeof(decoded.instance.bytes));
    decoded.accessEpoch = readRekeyU32(bytes.data + kRekeyEpochOffset);
    decoded.fromVersion = readRekeyU32(bytes.data + kRekeyVersionOffset);
    memcpy(decoded.oldLocator.bytes, bytes.data + kRekeyOldLocatorOffset, sizeof(decoded.oldLocator.bytes));
    if (!validRekeyOldBinding(decoded)) return Result::failure(ErrorCode::InvalidArgument);
    binding = decoded;
    return Result::success();
}

Result decodeWifiprovRekeyBinding(ByteView bytes, WifiprovRekeyBinding& binding) {
    if (bytes.data == nullptr || bytes.size != kWifiprovRekeyBindingSize)
        return Result::failure(ErrorCode::InvalidArgument);
    WifiprovRekeyBinding decoded;
    const Result result = decodeWifiprovRekeyOldBinding(bytes.subspan(0U, kWifiprovRekeyOldBindingSize), decoded.previous);
    if (!result) return result;
    memcpy(decoded.newLocator.bytes, bytes.data + kRekeyOldBindingSize, sizeof(decoded.newLocator.bytes));
    if (!validRekeyNewLocator(decoded)) return Result::failure(ErrorCode::InvalidArgument);
    binding = decoded;
    return Result::success();
}

Result encodeWifiprovRekeyBinding(const WifiprovRekeyBinding& binding, MutableByteSpan bytes) {
    if (bytes.data == nullptr || bytes.size != kWifiprovRekeyBindingSize)
        return Result::failure(ErrorCode::InvalidArgument);
    return encodeRekeyNewBinding(binding, bytes.data);
}

Result createWifiprovRekeyChallengeProof(const DeviceKey& oldKey,
    const WifiprovRekeyOldBinding& binding, WifiprovRekeyMac& proof) {
    const MutableByteSpan output(proof.bytes, sizeof(proof.bytes));
    uint8_t encoded[kRekeyOldBindingSize];
    Result result = encodeRekeyOldBinding(binding, encoded);
    if (result) result = checkRekeyLocator(oldKey, binding.oldLocator);
    if (result) result = rekeyMac(oldKey, ByteView(kRekeyChallengeDomain, sizeof(kRekeyChallengeDomain)),
        ByteView(encoded, sizeof(encoded)), output);
    if (!result) secureZero(output);
    return result;
}

Result openWifiprovRekeyCandidate(const DeviceKey& oldKey,
    const WifiprovRekeyBinding& binding, ByteView iv, ByteView box,
    IX25519AesGcmCryptoProvider& crypto, DeviceKey& candidate) {
    if (&oldKey == &candidate) return Result::failure(ErrorCode::InvalidArgument);
    clearDeviceKey(candidate);
    if (!nonzeroRekeyBytes(iv, kAesGcmNonceSize) || box.data == nullptr || box.size != kWifiprovRekeyBoxSize)
        return Result::failure(ErrorCode::InvalidArgument);

    uint8_t aad[sizeof(kRekeyWrapDomain) + kRekeyNewBindingSize];
    uint8_t* const encoded = aad + sizeof(kRekeyWrapDomain);
    Result result = encodeRekeyNewBinding(binding, encoded);
    if (result) result = checkRekeyLocator(oldKey, binding.previous.oldLocator);
    if (!result) return result;
    memcpy(aad, kRekeyWrapDomain, sizeof(kRekeyWrapDomain));

    uint8_t wrappingKey[kAes256KeySize] = {};
    const MutableByteSpan wrappingOutput(wrappingKey, sizeof(wrappingKey));
    result = rekeyMac(oldKey, ByteView(kRekeyWrapDomain, sizeof(kRekeyWrapDomain)),
        ByteView(encoded, kRekeyNewBindingSize), wrappingOutput);
    size_t written = 0U;
    if (result) result = crypto.aes256GcmDecrypt(ByteView(wrappingKey, sizeof(wrappingKey)), iv,
        ByteView(aad, sizeof(aad)), box, MutableByteSpan(candidate.bytes, sizeof(candidate.bytes)), written);
    secureZero(wrappingOutput);
    if (result && written != sizeof(candidate.bytes)) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = checkRekeyLocator(candidate, binding.newLocator);
    if (!result) clearDeviceKey(candidate);
    return result;
}

Result createWifiprovRekeyInstalledReceipt(const DeviceKey& readbackKey,
    const WifiprovRekeyBinding& binding, WifiprovRekeyMac& receipt) {
    const MutableByteSpan output(receipt.bytes, sizeof(receipt.bytes));
    uint8_t encoded[kRekeyNewBindingSize];
    Result result = encodeRekeyNewBinding(binding, encoded);
    if (result) result = checkRekeyLocator(readbackKey, binding.newLocator);
    if (result) result = rekeyMac(readbackKey, ByteView(kRekeyInstalledDomain, sizeof(kRekeyInstalledDomain)),
        ByteView(encoded, sizeof(encoded)), output);
    if (!result) secureZero(output);
    return result;
}

Result verifyWifiprovRekeyWifiPermit(const DeviceKey& readbackKey,
    const WifiprovRekeyBinding& binding, ByteView permit) {
    uint8_t encoded[kRekeyNewBindingSize];
    Result result = encodeRekeyNewBinding(binding, encoded);
    if (result) result = checkRekeyLocator(readbackKey, binding.newLocator);
    if (result) result = verifyRekeyMac(ByteView(readbackKey.bytes, sizeof(readbackKey.bytes)),
        ByteView(kRekeyWifiPermitDomain, sizeof(kRekeyWifiPermitDomain)), ByteView(encoded, sizeof(encoded)), permit);
    return result;
}

Result verifyWifiprovRekeySetupAdminProof(const ControllerCredential& admin,
    const WifiprovRekeyOldBinding& binding, ByteView ownerNonce, ByteView proof) {
    if (!validateControllerCredential(admin) || admin.suite != ControllerCredentialSuite::HmacSha256_32 ||
        admin.domain != ControllerCredentialDomain::PlatformAccess || admin.permissions != kControllerPermissionAll ||
        admin.ownershipGeneration != binding.accessEpoch ||
        !nonzeroRekeyBytes(ownerNonce, kWifiprovRekeyOwnerNonceSize)) return Result::failure(ErrorCode::AuthenticationRequired);
    constexpr size_t adminVersionOffset = static_cast<size_t>(kRekeyOldBindingSize) + kControllerIdSize;
    constexpr size_t nonceOffset = adminVersionOffset + 4U;
    uint8_t encoded[nonceOffset + kWifiprovRekeyOwnerNonceSize];
    Result result = encodeRekeyOldBinding(binding, encoded);
    if (!result) return result;
    memcpy(encoded + kRekeyOldBindingSize, admin.controllerId, sizeof(admin.controllerId));
    writeRekeyU32(encoded + adminVersionOffset, admin.credentialVersion);
    memcpy(encoded + nonceOffset, ownerNonce.data, ownerNonce.size);
    return verifyRekeyMac(admin.authenticationSecret(), ByteView(kRekeySetupAdminDomain, sizeof(kRekeySetupAdminDomain)),
        ByteView(encoded, sizeof(encoded)), proof);
}

} // namespace security
} // namespace blinker
