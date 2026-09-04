#include "GatewayCredentialRenewalEnvelopeSecurity.h"

#include <string.h>

#include "../core/HkdfSha256.h"
#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

namespace gateway_credential_renewal_security_detail {

const uint8_t kRenewalSaltDomain[] =
    "BLINKER-GATEWAY-RENEWAL-WRAP/1";
const uint8_t kRenewalInfo[] =
    "BLINKER-GATEWAY-CREDENTIAL-RENEWAL/1";

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

} // namespace gateway_credential_renewal_security_detail

Result deriveGatewayCredentialRenewalWrapKey(
    const DeviceKey& deviceKey,
    const GatewayAccessWrapContext& context,
    MutableByteSpan output) {
    if (output.data == nullptr || output.size < kAes256KeySize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    uint8_t accessWrapKey[kAes256KeySize] = {};
    Result result = deriveGatewayAccessWrapKey(
        deviceKey, context,
        MutableByteSpan(accessWrapKey, sizeof(accessWrapKey)));
    if (result) {
        result = hkdfSha256(
            ByteView(
                gateway_credential_renewal_security_detail::
                    kRenewalSaltDomain,
                sizeof(gateway_credential_renewal_security_detail::
                           kRenewalSaltDomain)),
            ByteView(accessWrapKey, sizeof(accessWrapKey)),
            ByteView(
                gateway_credential_renewal_security_detail::kRenewalInfo,
                sizeof(gateway_credential_renewal_security_detail::
                           kRenewalInfo) - 1U),
            MutableByteSpan(output.data, kAes256KeySize));
    }
    if (!result) {
        secureZero(MutableByteSpan(output.data, kAes256KeySize));
    }
    secureZero(MutableByteSpan(accessWrapKey, sizeof(accessWrapKey)));
    return result;
}

Result openGatewayCredentialRenewalEnvelope(
    const DeviceKey& deviceKey,
    const gateway::GatewayCredentialRenewalEnvelopeView& envelope,
    ByteView currentPresenceKey,
    IX25519AesGcmCryptoProvider& crypto,
    MutableByteSpan aadScratch,
    MutableByteSpan candidateSecret) {
    if (aadScratch.data == nullptr ||
        aadScratch.size <
            gateway::kGatewayCredentialRenewalAadMaximumEncodedSize ||
        candidateSecret.data == nullptr ||
        candidateSecret.size < gateway::kGatewaySecretSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (currentPresenceKey.size != gateway::kGatewayPresenceKeySize ||
        !gateway_credential_renewal_security_detail::nonZero(
            currentPresenceKey)) {
        secureZero(MutableByteSpan(
            candidateSecret.data, gateway::kGatewaySecretSize));
        secureZero(MutableByteSpan(aadScratch.data, aadScratch.size));
        return Result::failure(ErrorCode::InvalidArgument);
    }

    Result result = gateway::validateGatewayCredentialRenewalEnvelope(
        envelope);
    GatewayAccessWrapContext context;
    context.environmentId = envelope.environmentId;
    context.tenantId = envelope.tenantId;
    context.edgeHubLogicalDeviceId = envelope.edgeHubLogicalDeviceId;
    context.deviceKeyVersion = envelope.deviceKeyVersion;

    uint8_t wrapKey[kAes256KeySize] = {};
    if (result) {
        result = deriveGatewayCredentialRenewalWrapKey(
            deviceKey, context,
            MutableByteSpan(wrapKey, sizeof(wrapKey)));
    }
    size_t aadSize = 0U;
    if (result) {
        result = gateway::encodeGatewayCredentialRenewalAad(
            envelope, aadScratch, aadSize);
    }

    uint8_t ciphertextAndTag[
        gateway::kGatewaySecretSize + gateway::kEnvelopeTagSize] = {};
    if (result) {
        memcpy(ciphertextAndTag, envelope.ciphertext.data,
               gateway::kGatewaySecretSize);
        memcpy(ciphertextAndTag + gateway::kGatewaySecretSize,
               envelope.tag.data, gateway::kEnvelopeTagSize);
        size_t written = 0U;
        result = crypto.aes256GcmDecrypt(
            ByteView(wrapKey, sizeof(wrapKey)), envelope.nonce,
            ByteView(aadScratch.data, aadSize),
            ByteView(ciphertextAndTag, sizeof(ciphertextAndTag)),
            MutableByteSpan(candidateSecret.data,
                            gateway::kGatewaySecretSize),
            written);
        if (result && written != gateway::kGatewaySecretSize) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        }
    }

    uint8_t digest[kSha256Size] = {};
    if (result) {
        Sha256 hash;
        result = hash.update(ByteView(
            candidateSecret.data, gateway::kGatewaySecretSize));
        if (result) result = hash.update(currentPresenceKey);
        if (result) {
            result = hash.finish(MutableByteSpan(digest, sizeof(digest)));
        }
        if (result && !constantTimeEqual(
                          ByteView(digest, sizeof(digest)),
                          envelope.accessMaterialDigest)) {
            result = Result::failure(ErrorCode::AuthenticationRequired);
        }
    }
    if (result &&
        !gateway_credential_renewal_security_detail::nonZero(ByteView(
            candidateSecret.data, gateway::kGatewaySecretSize))) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!result) {
        secureZero(MutableByteSpan(
            candidateSecret.data, gateway::kGatewaySecretSize));
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(MutableByteSpan(wrapKey, sizeof(wrapKey)));
    secureZero(MutableByteSpan(
        ciphertextAndTag, sizeof(ciphertextAndTag)));
    secureZero(MutableByteSpan(aadScratch.data, aadScratch.size));
    return result;
}

} // namespace blinker
