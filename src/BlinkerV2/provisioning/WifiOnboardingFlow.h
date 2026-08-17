#ifndef BLINKER_PROVISIONING_WIFIONBOARDINGFLOW_H
#define BLINKER_PROVISIONING_WIFIONBOARDINGFLOW_H

#include "CloudCredentialEnrollmentCoordinator.h"
#include "EnrollmentKeyHttpExchange.h"
#include "OwnershipClaimCoordinator.h"
#include "WifiOnboardingHttpContract.h"
#include "../interface/IClock.h"

namespace blinker {

enum : size_t {
    kWifiOnboardingMinimumRequestBufferSize =
        kCloudEnrollmentRequestMaxEncodedSize,
    kWifiOnboardingMinimumResponseBufferSize =
        kOwnershipClaimCommitAckMaxEncodedSize,
    kWifiOnboardingMinimumWorkspaceSize = kCloudEnrollmentWorkspaceSize
};

enum class WifiOnboardingState : uint8_t {
    Stopped = 0U,
    ClaimGrantPending,
    ClaimReceiptAckPending,
    CloudGrantPending,
    CloudCommitAckPending,
    Active,
    Fault
};

// One resumable WiFi-only onboarding transaction. Durable state remains in
// the existing Claim and CloudCredential journals; this object owns no third
// journal and shares three caller-owned buffers across all four HTTP POSTs.
class WifiOnboardingFlow {
public:
    WifiOnboardingFlow(
        OwnershipClaimCoordinator& claim,
        CloudCredentialEnrollmentCoordinator& cloud,
        EnrollmentKeyHttpExchange& http,
        IClock& clock);
    ~WifiOnboardingFlow();

    Result start(
        MutableByteSpan requestBuffer,
        MutableByteSpan responseBuffer,
        MutableByteSpan workspace);
    Result retry();
    void poll(uint32_t budgetMicros);
    void stop();

    WifiOnboardingState state() const { return state_; }
    ErrorCode lastError() const { return lastError_; }
    bool active() const { return state_ == WifiOnboardingState::Active; }
    bool hasWireError() const { return hasWireError_; }
    bool wireErrorIsCloudEnrollment() const { return cloudWireError_; }
    uint16_t lastWireError() const { return lastWireError_; }
    WifiOnboardingRetryClass retryClass() const { return retryClass_; }
    bool hasRetryAfter() const { return hasRetryAfter_; }
    uint32_t retryAfterSeconds() const { return retryAfterSeconds_; }

private:
    Result validateBuffers() const;
    Result resume();
    Result beginCloud();
    Result send(
        WifiOnboardingState phase,
        StringView target,
        ByteView body);
    void processResponse();
    void processClaimGrant(ByteView body);
    void processClaimCommitAck(ByteView body);
    void processCloudGrant(ByteView body);
    void processCloudCommitAck(ByteView body);
    void processClaimError(uint16_t statusCode, ByteView body);
    void processCloudError(uint16_t statusCode, ByteView body);
    void complete();
    void fail(
        ErrorCode error,
        WifiOnboardingRetryClass retryClass,
        WifiOnboardingState phase);
    void clearBuffers();
    void clearError();
    bool retryDue() const;
    static bool overlaps(ByteView first, ByteView second);
    static ErrorCode mapClaimError(WifiOnboardingWireError error);
    static ErrorCode mapCloudError(CloudEnrollmentWireError error);
    static WifiOnboardingRetryClass mapCloudRetry(
        CloudEnrollmentRetryClass retryClass);

    OwnershipClaimCoordinator& claim_;
    CloudCredentialEnrollmentCoordinator& cloud_;
    EnrollmentKeyHttpExchange& http_;
    IClock& clock_;
    MutableByteSpan requestBuffer_;
    MutableByteSpan responseBuffer_;
    MutableByteSpan workspace_;
    uint32_t retryAtMillis_;
    uint32_t retryAfterSeconds_;
    uint16_t lastWireError_;
    WifiOnboardingState state_;
    WifiOnboardingState failedPhase_;
    ErrorCode lastError_;
    WifiOnboardingRetryClass retryClass_;
    bool hasWireError_;
    bool cloudWireError_;
    bool hasRetryAfter_;

    WifiOnboardingFlow(const WifiOnboardingFlow&);
    WifiOnboardingFlow& operator=(const WifiOnboardingFlow&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(WifiOnboardingFlow) <= 60U,
    "WiFi onboarding flow exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
