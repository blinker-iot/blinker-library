#include "WifiOnboardingHttpContract.h"

#include "../protocol/cbor/Cbor.h"

namespace blinker {

namespace {

const char kOwnershipClaimTarget[] =
    "/api/v2/device-onboarding/ownership-claims";
const char kOwnershipReceiptTarget[] =
    "/api/v2/device-onboarding/ownership-receipts";
const char kCloudCredentialTarget[] =
    "/api/v2/device-onboarding/cloud-credentials";
const char kCloudCredentialReceiptTarget[] =
    "/api/v2/device-onboarding/cloud-receipts";

cbor::Limits limits() {
    cbor::Limits value;
    value.maxContainerItems = 4U;
    value.maxDepth = 1U;
    value.maxByteStringLength = 0U;
    value.maxTextLength = 0U;
    return value;
}

Result key(cbor::Writer& writer, uint8_t value) {
    return writer.writeUnsigned(value);
}

Result key(cbor::Reader& reader, uint8_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    return result && value != expected
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

bool validWireError(WifiOnboardingWireError error) {
    switch (error) {
        case WifiOnboardingWireError::Malformed:
        case WifiOnboardingWireError::UnsupportedVersion:
        case WifiOnboardingWireError::AuthenticationRequired:
        case WifiOnboardingWireError::EnrollmentKeyExpired:
        case WifiOnboardingWireError::EnrollmentKeyConsumed:
        case WifiOnboardingWireError::EnrollmentKeyConflict:
        case WifiOnboardingWireError::OwnershipAlreadyActive:
        case WifiOnboardingWireError::OwnershipGenerationConflict:
        case WifiOnboardingWireError::ClaimConflict:
        case WifiOnboardingWireError::RequestExpired:
        case WifiOnboardingWireError::RateLimited:
        case WifiOnboardingWireError::TemporarilyUnavailable:
            return true;
    }
    return false;
}

bool validError(const WifiOnboardingErrorBody& body) {
    if (!validWireError(body.error) ||
        body.hasRetryAfter !=
            (body.retryClass == WifiOnboardingRetryClass::Backoff) ||
        (body.hasRetryAfter && body.retryAfterSeconds == 0U)) {
        return false;
    }
    switch (body.error) {
        case WifiOnboardingWireError::Malformed:
        case WifiOnboardingWireError::UnsupportedVersion:
        case WifiOnboardingWireError::OwnershipAlreadyActive:
            return body.retryClass == WifiOnboardingRetryClass::Never;
        case WifiOnboardingWireError::AuthenticationRequired:
        case WifiOnboardingWireError::EnrollmentKeyExpired:
        case WifiOnboardingWireError::EnrollmentKeyConsumed:
        case WifiOnboardingWireError::EnrollmentKeyConflict:
        case WifiOnboardingWireError::ClaimConflict:
            return body.retryClass ==
                   WifiOnboardingRetryClass::ReplaceEnrollmentKey;
        case WifiOnboardingWireError::OwnershipGenerationConflict:
            return body.retryClass ==
                   WifiOnboardingRetryClass::ReclaimOwnership;
        case WifiOnboardingWireError::RequestExpired:
            return body.retryClass ==
                   WifiOnboardingRetryClass::RestartTransaction;
        case WifiOnboardingWireError::RateLimited:
            return body.retryClass == WifiOnboardingRetryClass::Backoff;
        case WifiOnboardingWireError::TemporarilyUnavailable:
            return body.retryClass == WifiOnboardingRetryClass::SameRequest ||
                   body.retryClass == WifiOnboardingRetryClass::Backoff;
    }
    return false;
}

Result finish(cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

} // namespace

WifiOnboardingErrorBody::WifiOnboardingErrorBody()
    : error(WifiOnboardingWireError::Malformed),
      retryClass(WifiOnboardingRetryClass::Never),
      retryAfterSeconds(0U),
      hasRetryAfter(false) {}

Result encodeWifiOnboardingErrorBody(
    const WifiOnboardingErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (!validError(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(body.hasRetryAfter ? 4U : 3U);
    if (result) result = key(writer, 0U);
    if (result) result = writer.writeUnsigned(kWifiOnboardingHttpContractVersion);
    if (result) result = key(writer, 1U);
    if (result) result = writer.writeUnsigned(static_cast<uint16_t>(body.error));
    if (result) result = key(writer, 2U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.retryClass));
    }
    if (result && body.hasRetryAfter) result = key(writer, 3U);
    if (result && body.hasRetryAfter) {
        result = writer.writeUnsigned(body.retryAfterSeconds);
    }
    if (result) encoded = writer.view();
    return result;
}

Result decodeWifiOnboardingErrorBody(
    ByteView encoded,
    WifiOnboardingErrorBody& body) {
    cbor::Reader reader(encoded, limits());
    Result result = cbor::validate(encoded, limits());
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 3U && pairs != 4U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    WifiOnboardingErrorBody decoded;
    uint64_t value = 0U;
    if (result) result = key(reader, 0U);
    if (result) result = reader.readUnsigned(value);
    if (result && value != kWifiOnboardingHttpContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = key(reader, 1U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT16_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.error = static_cast<WifiOnboardingWireError>(value);
    if (result) result = key(reader, 2U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.retryClass = static_cast<WifiOnboardingRetryClass>(value);
    decoded.hasRetryAfter = pairs == 4U;
    if (result && decoded.hasRetryAfter) result = key(reader, 3U);
    if (result && decoded.hasRetryAfter) result = reader.readUnsigned(value);
    if (result && decoded.hasRetryAfter && value > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result && decoded.hasRetryAfter) {
        decoded.retryAfterSeconds = static_cast<uint32_t>(value);
    }
    if (result) result = finish(reader);
    if (result && !validError(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

uint16_t wifiOnboardingHttpStatus(WifiOnboardingWireError error) {
    switch (error) {
        case WifiOnboardingWireError::Malformed:
        case WifiOnboardingWireError::UnsupportedVersion:
            return 400U;
        case WifiOnboardingWireError::AuthenticationRequired:
        case WifiOnboardingWireError::EnrollmentKeyExpired:
            return 401U;
        case WifiOnboardingWireError::EnrollmentKeyConsumed:
        case WifiOnboardingWireError::EnrollmentKeyConflict:
        case WifiOnboardingWireError::OwnershipAlreadyActive:
        case WifiOnboardingWireError::OwnershipGenerationConflict:
        case WifiOnboardingWireError::ClaimConflict:
        case WifiOnboardingWireError::RequestExpired:
            return 409U;
        case WifiOnboardingWireError::RateLimited:
            return 429U;
        case WifiOnboardingWireError::TemporarilyUnavailable:
            return 503U;
    }
    return 0U;
}

bool wifiOnboardingHttpStatusMatches(
    uint16_t statusCode,
    WifiOnboardingWireError error) {
    return statusCode == wifiOnboardingHttpStatus(error);
}

StringView wifiOwnershipClaimTarget() {
    return StringView(kOwnershipClaimTarget, sizeof(kOwnershipClaimTarget) - 1U);
}

StringView wifiOwnershipReceiptTarget() {
    return StringView(
        kOwnershipReceiptTarget,
        sizeof(kOwnershipReceiptTarget) - 1U);
}

StringView wifiCloudCredentialTarget() {
    return StringView(
        kCloudCredentialTarget,
        sizeof(kCloudCredentialTarget) - 1U);
}

StringView wifiCloudCredentialReceiptTarget() {
    return StringView(
        kCloudCredentialReceiptTarget,
        sizeof(kCloudCredentialReceiptTarget) - 1U);
}

} // namespace blinker
