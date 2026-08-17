#ifndef BLINKER_PROVISIONING_WIFIONBOARDINGHTTPCONTRACT_H
#define BLINKER_PROVISIONING_WIFIONBOARDINGHTTPCONTRACT_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kWifiOnboardingErrorMaxEncodedSize = 15U
};

static const uint8_t kWifiOnboardingHttpContractVersion = 1U;

enum class WifiOnboardingRetryClass : uint8_t {
    Never = 0U,
    SameRequest = 1U,
    Backoff = 2U,
    RestartTransaction = 3U,
    ReplaceEnrollmentKey = 4U,
    ReclaimOwnership = 5U
};

// HTTP/profile errors used by the EnrollmentKey-authorized ownership phase.
// CloudCredential semantic errors keep their existing 3xxx contract.
enum class WifiOnboardingWireError : uint16_t {
    Malformed = 4000U,
    UnsupportedVersion = 4001U,
    AuthenticationRequired = 4002U,
    EnrollmentKeyExpired = 4100U,
    EnrollmentKeyConsumed = 4101U,
    EnrollmentKeyConflict = 4102U,
    OwnershipAlreadyActive = 4200U,
    OwnershipGenerationConflict = 4201U,
    ClaimConflict = 4202U,
    RequestExpired = 4203U,
    RateLimited = 4500U,
    TemporarilyUnavailable = 4501U
};

struct WifiOnboardingErrorBody {
    WifiOnboardingWireError error;
    WifiOnboardingRetryClass retryClass;
    uint32_t retryAfterSeconds;
    bool hasRetryAfter;

    WifiOnboardingErrorBody();
};

Result encodeWifiOnboardingErrorBody(
    const WifiOnboardingErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeWifiOnboardingErrorBody(
    ByteView encoded,
    WifiOnboardingErrorBody& body);
uint16_t wifiOnboardingHttpStatus(WifiOnboardingWireError error);
bool wifiOnboardingHttpStatusMatches(
    uint16_t statusCode,
    WifiOnboardingWireError error);

StringView wifiOwnershipClaimTarget();
StringView wifiOwnershipReceiptTarget();
StringView wifiCloudCredentialTarget();
StringView wifiCloudCredentialReceiptTarget();

} // namespace blinker

#endif
