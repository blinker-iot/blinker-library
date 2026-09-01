#include "GatewayAccessEnvelopeSecurity.h"

#include <string.h>

#include "../core/HkdfSha256.h"
#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

namespace gateway_access_security_detail {

const uint8_t kWrapSaltDomain[] = "BLINKER-EDGE-GATEWAY-WRAP/2";
const uint8_t kWrapInfo[] = "BLINKER-GATEWAY-ACCESS-ENVELOPE/2";

bool validIdentifier(StringView value, size_t maximum) {
    if (value.data == nullptr || value.empty() || value.size > maximum) {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        const uint8_t character = static_cast<uint8_t>(value.data[index]);
        if (character < 0x21U || character > 0x7EU ||
            character == '/' || character == '\\' ||
            character == '#' || character == '+') {
            return false;
        }
    }
    return true;
}

Result updateFramed(Sha256& hash, StringView value) {
    uint8_t length[2] = {
        static_cast<uint8_t>(value.size >> 8U),
        static_cast<uint8_t>(value.size)};
    Result result = hash.update(ByteView(length, sizeof(length)));
    if (result) {
        result = hash.update(ByteView(
            reinterpret_cast<const uint8_t*>(value.data), value.size));
    }
    secureZero(MutableByteSpan(length, sizeof(length)));
    return result;
}

} // namespace gateway_access_security_detail

Result deriveGatewayAccessWrapKey(
    const DeviceKey& deviceKey,
    const GatewayAccessWrapContext& context,
    MutableByteSpan output) {
    if (!validateDeviceKey(deviceKey) || output.data == nullptr ||
        output.size < kAes256KeySize ||
        !gateway_access_security_detail::validIdentifier(
            context.environmentId, gateway::kMaximumEnvironmentIdSize) ||
        !gateway_access_security_detail::validIdentifier(
            context.tenantId, gateway::kMaximumTenantIdSize) ||
        !gateway_access_security_detail::validIdentifier(
            context.edgeHubLogicalDeviceId,
            gateway::kMaximumLogicalDeviceIdSize) ||
        context.deviceKeyVersion == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    Sha256 transcript;
    Result result = transcript.update(ByteView(
        gateway_access_security_detail::kWrapSaltDomain,
        sizeof(gateway_access_security_detail::kWrapSaltDomain)));
    if (result) {
        result = gateway_access_security_detail::updateFramed(
            transcript, context.environmentId);
    }
    if (result) {
        result = gateway_access_security_detail::updateFramed(
            transcript, context.tenantId);
    }
    if (result) {
        result = gateway_access_security_detail::updateFramed(
            transcript, context.edgeHubLogicalDeviceId);
    }
    uint8_t version[4] = {
        static_cast<uint8_t>(context.deviceKeyVersion >> 24U),
        static_cast<uint8_t>(context.deviceKeyVersion >> 16U),
        static_cast<uint8_t>(context.deviceKeyVersion >> 8U),
        static_cast<uint8_t>(context.deviceKeyVersion)};
    if (result) result = transcript.update(ByteView(version, sizeof(version)));

    uint8_t salt[kSha256Size] = {};
    if (result) result = transcript.finish(MutableByteSpan(salt, sizeof(salt)));
    if (result) {
        result = hkdfSha256(
            ByteView(salt, sizeof(salt)),
            ByteView(deviceKey.bytes, sizeof(deviceKey.bytes)),
            ByteView(gateway_access_security_detail::kWrapInfo,
                     sizeof(gateway_access_security_detail::kWrapInfo) - 1U),
            MutableByteSpan(output.data, kAes256KeySize));
    }
    if (!result) {
        secureZero(MutableByteSpan(output.data, kAes256KeySize));
    }
    secureZero(MutableByteSpan(version, sizeof(version)));
    secureZero(MutableByteSpan(salt, sizeof(salt)));
    return result;
}

Result openGatewayAccessEnvelope(
    const DeviceKey& deviceKey,
    const gateway::GatewayAccessEnvelopeView& envelope,
    IX25519AesGcmCryptoProvider& crypto,
    MutableByteSpan aadScratch,
    MutableByteSpan plaintext) {
    if (aadScratch.data == nullptr ||
        aadScratch.size < gateway::kGatewayAccessAadMaximumEncodedSize ||
        plaintext.data == nullptr ||
        plaintext.size < gateway::kGatewayAccessMaterialSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = gateway::validateGatewayAccessEnvelope(envelope);

    GatewayAccessWrapContext context;
    context.environmentId = envelope.environmentId;
    context.tenantId = envelope.tenantId;
    context.edgeHubLogicalDeviceId = envelope.edgeHubLogicalDeviceId;
    context.deviceKeyVersion = envelope.deviceKeyVersion;

    uint8_t wrapKey[kAes256KeySize] = {};
    if (result) {
        result = deriveGatewayAccessWrapKey(
            deviceKey, context, MutableByteSpan(wrapKey, sizeof(wrapKey)));
    }
    size_t aadSize = 0U;
    if (result) {
        result = gateway::encodeGatewayAccessAad(
            envelope, aadScratch, aadSize);
    }

    uint8_t ciphertextAndTag[
        gateway::kGatewayAccessMaterialSize +
        gateway::kEnvelopeTagSize] = {};
    if (result) {
        memcpy(ciphertextAndTag, envelope.ciphertext.data,
               gateway::kGatewayAccessMaterialSize);
        memcpy(ciphertextAndTag + gateway::kGatewayAccessMaterialSize,
               envelope.tag.data, gateway::kEnvelopeTagSize);
        size_t written = 0U;
        result = crypto.aes256GcmDecrypt(
            ByteView(wrapKey, sizeof(wrapKey)), envelope.nonce,
            ByteView(aadScratch.data, aadSize),
            ByteView(ciphertextAndTag, sizeof(ciphertextAndTag)),
            MutableByteSpan(
                plaintext.data, gateway::kGatewayAccessMaterialSize),
            written);
        if (result && written != gateway::kGatewayAccessMaterialSize) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        }
    }
    if (!result) {
        secureZero(MutableByteSpan(
            plaintext.data, gateway::kGatewayAccessMaterialSize));
    }
    secureZero(MutableByteSpan(wrapKey, sizeof(wrapKey)));
    secureZero(MutableByteSpan(ciphertextAndTag, sizeof(ciphertextAndTag)));
    secureZero(MutableByteSpan(aadScratch.data, aadScratch.size));
    return result;
}

} // namespace blinker
