#include "LocalAccessSecurity.h"

#include "../core/HkdfSha256.h"
#include "../core/HmacSha256.h"
#include "../core/SecureMemory.h"

namespace blinker {
namespace security {

namespace {

const uint8_t kGrantKeyInfo[] = "BLINKER-LAN-ACCESS-GRANT-MAC/1";
const uint8_t kNoisePskInfo[] = "BLINKER-LAN-ACCESS-NOISE-PSK/1";
const uint8_t kPlainProofInfo[] = "BLINKER-LAN-ACCESS-PLAIN-PROOF/1";
const uint8_t kControlKeyInfo[] = "BLINKER-LAN-ACCESS-CONTROL-MAC/1";
const uint8_t kProgressKeyInfo[] = "BLINKER-LAN-ACCESS-PROGRESS-MAC/1";

struct HmacSinkContext {
    HmacSha256* hmac;
};

Result updateHmacSink(void* context, ByteView bytes) {
    if (context == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    HmacSinkContext* state = static_cast<HmacSinkContext*>(context);
    return state->hmac != nullptr
               ? state->hmac->update(bytes)
               : Result::failure(ErrorCode::InvalidArgument);
}

Result deriveLocalAccessKey(
    const DeviceKey& deviceKey,
    ByteView instance, uint32_t version, ByteView info,
    MutableByteSpan output) {
    Result result = validateDeviceKey(deviceKey);
    if (!result || instance.data == nullptr ||
        instance.size != local_access::kIdSize ||
        version == 0U || output.data == nullptr ||
        output.size < kDeviceAuthKeySize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t salt[local_access::kIdSize + 4U] = {};
    for (size_t index = 0U; index < local_access::kIdSize; ++index) {
        salt[index] = instance.data[index];
    }
    salt[local_access::kIdSize] =
        static_cast<uint8_t>(version >> 24U);
    salt[local_access::kIdSize + 1U] =
        static_cast<uint8_t>(version >> 16U);
    salt[local_access::kIdSize + 2U] =
        static_cast<uint8_t>(version >> 8U);
    salt[local_access::kIdSize + 3U] =
        static_cast<uint8_t>(version);
    result = hkdfSha256(
        ByteView(salt, sizeof(salt)),
        ByteView(deviceKey.bytes, sizeof(deviceKey.bytes)),
        info,
        MutableByteSpan(output.data, kDeviceAuthKeySize));
    secureZero(MutableByteSpan(salt, sizeof(salt)));
    return result;
}

} // namespace

Result computeLocalAccessGrantAuthenticator(
    const DeviceKey& deviceKey,
    const local_access::GrantView& grant,
    MutableByteSpan output) {
    if (output.data == nullptr ||
        output.size < local_access::kAuthenticatorSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = local_access::validateGrantUnsigned(grant);
    uint8_t key[kDeviceAuthKeySize] = {};
    if (result) {
        result = deriveLocalAccessKey(
            deviceKey, grant.recipientDeviceInstanceId, grant.deviceKeyVersion,
            ByteView(kGrantKeyInfo, sizeof(kGrantKeyInfo) - 1U),
            MutableByteSpan(key, sizeof(key)));
    }
    HmacSha256 hmac;
    if (result) result = hmac.begin(ByteView(key, sizeof(key)));
    HmacSinkContext sink = {&hmac};
    size_t written = 0U;
    if (result) {
        result = local_access::streamGrantTranscript(
            grant, updateHmacSink, &sink, written);
    }
    if (result && written == 0U) {
        result = Result::failure(ErrorCode::InternalError);
    }
    if (result) {
        result = hmac.finish(MutableByteSpan(
            output.data, local_access::kAuthenticatorSize));
    }
    if (!result) {
        secureZero(MutableByteSpan(
            output.data, local_access::kAuthenticatorSize));
    }
    secureZero(MutableByteSpan(key, sizeof(key)));
    return result;
}

Result verifyLocalAccessGrantAuthenticator(
    const DeviceKey& deviceKey,
    const local_access::GrantView& grant) {
    Result result = local_access::validateGrant(grant);
    uint8_t expected[local_access::kAuthenticatorSize] = {};
    if (result) {
        result = computeLocalAccessGrantAuthenticator(
            deviceKey, grant, MutableByteSpan(expected, sizeof(expected)));
    }
    if (result && !constantTimeEqual(
            ByteView(expected, sizeof(expected)), grant.authenticator)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

Result deriveLocalAccessSessionKey(
    const DeviceKey& deviceKey,
    const local_access::GrantView& grant,
    MutableByteSpan output) {
    if (output.data == nullptr || output.size < kDeviceAuthKeySize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = verifyLocalAccessGrantAuthenticator(deviceKey, grant);
    if (result) {
        const ByteView info = grant.securityProfile == local_access::SecurityProfile::PlainHmacSha256
            ? ByteView(kPlainProofInfo, sizeof(kPlainProofInfo) - 1U)
            : ByteView(kNoisePskInfo, sizeof(kNoisePskInfo) - 1U);
        result = hkdfSha256(
            grant.authenticator,
            ByteView(deviceKey.bytes, sizeof(deviceKey.bytes)),
            info,
            MutableByteSpan(output.data, kDeviceAuthKeySize));
    }
    if (!result) {
        secureZero(MutableByteSpan(output.data, kDeviceAuthKeySize));
    }
    return result;
}

Result computeLocalAccessPlainProof(ByteView sessionKey, ByteView grantAuthenticator,
    uint8_t kind, MutableByteSpan output) {
    if (output.data == nullptr || output.size < local_access::kAuthenticatorSize)
        return Result::failure(ErrorCode::BufferTooSmall);
    secureZero(MutableByteSpan(output.data, local_access::kAuthenticatorSize));
    if (sessionKey.data == nullptr || sessionKey.size != kDeviceAuthKeySize ||
        grantAuthenticator.data == nullptr || grantAuthenticator.size != local_access::kAuthenticatorSize ||
        (kind != local_access::kCallerProofKind && kind != local_access::kRecipientProofKind))
        return Result::failure(ErrorCode::InvalidArgument);
    uint8_t keyBits = 0U, macBits = 0U;
    for (size_t index = 0U; index < kDeviceAuthKeySize; ++index) {
        keyBits |= sessionKey.data[index];
        macBits |= grantAuthenticator.data[index];
    }
    if (keyBits == 0U || macBits == 0U) return Result::failure(ErrorCode::InvalidArgument);
    const uint8_t caller[] = "BLINKER-LAN-PLAIN-CALLER/1";
    const uint8_t recipient[] = "BLINKER-LAN-PLAIN-RECIPIENT/1";
    HmacSha256 hmac;
    Result result = hmac.begin(sessionKey);
    if (result) result = hmac.update(kind == local_access::kCallerProofKind
        ? ByteView(caller, sizeof(caller) - 1U) : ByteView(recipient, sizeof(recipient) - 1U));
    if (result) result = hmac.update(grantAuthenticator);
    if (result) result = hmac.finish(MutableByteSpan(output.data, local_access::kAuthenticatorSize));
    return result;
}

Result computeLocalAccessControlAuthenticator(
    const DeviceKey& deviceKey, const local_access::ControlView& control,
    MutableByteSpan output, ByteView proofSessionId) {
    if (output.data == nullptr || output.size < local_access::kAuthenticatorSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = local_access::validateControlUnsigned(control);
    if (result && control.kind == local_access::ControlKind::Hello) {
        uint8_t combined = 0U;
        if (proofSessionId.data != nullptr && proofSessionId.size == local_access::kIdSize) {
            for (size_t i = 0U; i < proofSessionId.size; ++i) combined |= proofSessionId.data[i];
        }
        if (combined == 0U) result = Result::failure(ErrorCode::InvalidArgument);
    } else if (result && !proofSessionId.empty()) result = Result::failure(ErrorCode::InvalidArgument);
    uint8_t key[kDeviceAuthKeySize] = {};
    if (result) result = deriveLocalAccessKey(
        deviceKey, control.recipientDeviceInstanceId, control.deviceKeyVersion,
        ByteView(kControlKeyInfo, sizeof(kControlKeyInfo) - 1U), MutableByteSpan(key, sizeof(key)));
    HmacSha256 hmac;
    if (result) result = hmac.begin(ByteView(key, sizeof(key)));
    HmacSinkContext sink = {&hmac};
    size_t written = 0U;
    if (result) result = local_access::streamControlTranscript(control, updateHmacSink, &sink, written);
    if (result && written == 0U) result = Result::failure(ErrorCode::InternalError);
    // Fixed 16-byte suffix only for Hello; no new wire field or stored nonce.
    if (result && control.kind == local_access::ControlKind::Hello) result = hmac.update(proofSessionId);
    if (result) result = hmac.finish(MutableByteSpan(output.data, local_access::kAuthenticatorSize));
    if (!result) secureZero(MutableByteSpan(output.data, local_access::kAuthenticatorSize));
    secureZero(MutableByteSpan(key, sizeof(key)));
    return result;
}

Result verifyLocalAccessControlAuthenticator(
    const DeviceKey& deviceKey, const local_access::ControlView& control, ByteView proofSessionId) {
    Result result = local_access::validateControl(control);
    uint8_t expected[local_access::kAuthenticatorSize] = {};
    if (result) result = computeLocalAccessControlAuthenticator(deviceKey, control,
        MutableByteSpan(expected, sizeof(expected)), proofSessionId);
    if (result && !constantTimeEqual(ByteView(expected, sizeof(expected)), control.authenticator)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

Result computeLocalAccessProgressAuthenticator(const DeviceKey& deviceKey, const local_access::ProgressView& value,
    ByteView proofSessionId, MutableByteSpan output) {
    if (!output.data || output.size < local_access::kAuthenticatorSize) return Result::failure(ErrorCode::BufferTooSmall);
    Result result = local_access::validateProgressUnsigned(value);
    uint8_t bits = 0U;
    if (proofSessionId.data && proofSessionId.size == local_access::kIdSize)
        for (size_t i = 0U; i < proofSessionId.size; ++i) bits |= proofSessionId.data[i];
    if (!bits) result = Result::failure(ErrorCode::InvalidArgument);
    uint8_t key[kDeviceAuthKeySize] = {};
    if (result) result = deriveLocalAccessKey(deviceKey, value.recipientDeviceInstanceId, value.deviceKeyVersion,
        ByteView(kProgressKeyInfo, sizeof(kProgressKeyInfo) - 1U), MutableByteSpan(key, sizeof(key)));
    HmacSha256 hmac;
    if (result) result = hmac.begin(ByteView(key, sizeof(key)));
    HmacSinkContext sink = {&hmac}; size_t written = 0U;
    if (result) result = local_access::streamProgressTranscript(value, updateHmacSink, &sink, written);
    if (result) result = hmac.update(proofSessionId);
    if (result) result = hmac.finish(MutableByteSpan(output.data, local_access::kAuthenticatorSize));
    if (!result) secureZero(MutableByteSpan(output.data, local_access::kAuthenticatorSize));
    secureZero(MutableByteSpan(key, sizeof(key)));
    return result;
}

Result verifyLocalAccessProgressAuthenticator(const DeviceKey& key, const local_access::ProgressView& value,
    ByteView proofSessionId) {
    uint8_t mac[local_access::kAuthenticatorSize] = {};
    Result result = computeLocalAccessProgressAuthenticator(key, value, proofSessionId, MutableByteSpan(mac, sizeof(mac)));
    if (result && !constantTimeEqual(ByteView(mac, sizeof(mac)), value.authenticator)) result = Result::failure(ErrorCode::AuthenticationRequired);
    secureZero(MutableByteSpan(mac, sizeof(mac)));
    return result;
}

} // namespace security
} // namespace blinker
