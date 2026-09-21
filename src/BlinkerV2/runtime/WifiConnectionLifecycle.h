#ifndef BLINKER_RUNTIME_WIFICONNECTIONLIFECYCLE_H
#define BLINKER_RUNTIME_WIFICONNECTIONLIFECYCLE_H

#include "../interface/IClock.h"
#include "../interface/IWifiStation.h"
#include "../provisioning/WifiCredentialStore.h"

namespace blinker {

enum class WifiConnectionState : uint8_t {
    Stopped = 0,
    WaitingForCredentials,
    ConnectingCandidate,
    ConnectingConfirmed,
    ConfirmingCandidate,
    Online,
    RetryBackoff,
    CandidateRejected,
    Fault
};

struct WifiConnectionLifecycleConfig {
    uint32_t connectionTimeoutMs;
    uint32_t reconnectBackoffMs;
    uint32_t persistenceRetryMs;

    WifiConnectionLifecycleConfig()
        : connectionTimeoutMs(30000U),
          reconnectBackoffMs(5000U),
          persistenceRetryMs(1000U) {}
};

// The product owns network intent; the station and durable store do not.
enum class WifiCandidateFailurePolicy : uint8_t {
    Rollback = 0U, // Transactional provisioning may restore a confirmed profile.
    Retry         // Explicit Sketch configuration keeps its selected network.
};

// Portable, allocation-free WiFi connection policy. The caller owns the
// profile workspace for the full lifetime of this object. A newly committed
// profile is a candidate: it becomes usable by cloud/enrollment only after AP
// connection and confirmActive() have both succeeded durably. Transactional
// candidates may roll back only to a confirmed previous slot; explicit manual
// candidates instead reuse the same bounded retry state without Flash writes.
class WifiConnectionLifecycle {
public:
    WifiConnectionLifecycle(
        WifiCredentialStore& credentials,
        IWifiStation& station,
        IClock& clock,
        WifiCredentialProfile& profileWorkspace,
        const WifiConnectionLifecycleConfig& config =
            WifiConnectionLifecycleConfig());
    ~WifiConnectionLifecycle();

    Result setCandidateFailurePolicy(WifiCandidateFailurePolicy policy);
    Result start();
    Result reload();
    void poll();
    void stop();

    WifiConnectionState state() const { return state_; }
    bool online() const { return state_ == WifiConnectionState::Online; }
    // True only after the active profile has been durably confirmed. This is
    // deliberately independent of current radio connectivity: product
    // composition may expose direct BLE while WiFi is reconnecting, but must
    // not leave provisioning for an untested candidate.
    bool confirmedProfile() const {
        return loadedGeneration_ != 0U && profile_.confirmed();
    }
    ErrorCode lastError() const { return lastError_; }
    uint32_t profileGeneration() const { return loadedGeneration_; }

private:
    Result loadAndConnect();
    Result startStation();
    Result handleConnectionFailure(ErrorCode error);
    Result scheduleRetry(ErrorCode error);
    Result confirmCandidate();
    bool connectionTimedOut(uint32_t now) const;
    bool retryDue(uint32_t now) const;
    void enterFault(ErrorCode error);
    void clearProfile();

    WifiCredentialStore& credentials_;
    IWifiStation& station_;
    IClock& clock_;
    WifiCredentialProfile& profile_;
    WifiConnectionLifecycleConfig config_;
    uint32_t loadedGeneration_;
    uint32_t connectionStartedAt_;
    uint32_t retryAt_;
    WifiConnectionState state_;
    ErrorCode lastError_;
    WifiCandidateFailurePolicy candidateFailurePolicy_;

    WifiConnectionLifecycle(const WifiConnectionLifecycle&);
    WifiConnectionLifecycle& operator=(const WifiConnectionLifecycle&);
};

} // namespace blinker

#endif
