#ifndef BLINKER_PROVISIONING_INITIALCLOUDWIFIONLY_H
#define BLINKER_PROVISIONING_INITIALCLOUDWIFIONLY_H

#include <string.h>
#include "WifiCredentialStore.h"

namespace blinker {

enum class InitialCloudWifiOnlyState : uint8_t {
    Empty, Staged, Pending, Processing, Committed, Failed, Cancelled
};

class IInitialCloudWifiWriteGuard {
public:
    virtual ~IInitialCloudWifiWriteGuard() {}
    virtual Result validateWifiWrite() = 0;
};

// Candidate-only one-shot mailbox. All calls to it are serialized by the
// carrier. Set only copies bytes; Apply only queues. Neither calls the SDK
// station configuration nor opens the journal. Zero is a valid native ID.
class InitialCloudWifiOnly {
public:
    InitialCloudWifiOnly() : profile_(), session_(0U), state_(InitialCloudWifiOnlyState::Empty) {}
    ~InitialCloudWifiOnly() { clearWifiCredentialProfile(profile_); }
    InitialCloudWifiOnly(const InitialCloudWifiOnly&) = delete;
    InitialCloudWifiOnly& operator=(const InitialCloudWifiOnly&) = delete;

    Result stage(uint32_t session, const WifiNetworkConfig& config) {
        if (state_ != InitialCloudWifiOnlyState::Empty) return Result::failure(ErrorCode::StateConflict);
        const Result valid = validateWifiNetworkConfig(config);
        if (!valid) return valid;
        profile_.authentication = config.authentication;
        profile_.setHidden(config.hidden);
        profile_.ssidLength = static_cast<uint8_t>(config.ssid.size);
        profile_.credentialLength = static_cast<uint8_t>(config.credential.size);
        memcpy(profile_.ssid, config.ssid.data, config.ssid.size);
        if (!config.credential.empty()) memcpy(profile_.credential, config.credential.data, config.credential.size);
        session_ = session;
        state_ = InitialCloudWifiOnlyState::Staged;
        return Result::success();
    }

    Result apply(uint32_t session) {
        if (state_ != InitialCloudWifiOnlyState::Staged) return Result::failure(ErrorCode::StateConflict);
        if (session != session_) return Result::failure(ErrorCode::AuthenticationRequired);
        state_ = InitialCloudWifiOnlyState::Pending;
        return Result::success();
    }

    InitialCloudWifiOnlyState status(uint32_t session) const {
        return session == session_ ? state_ : InitialCloudWifiOnlyState::Empty;
    }

    // Only the cooperative owner may take/finish/cancel; the carrier lock
    // covers the transfer, not slow persistence or root validation.
    bool take(WifiCredentialProfile& output) {
        clearWifiCredentialProfile(output);
        if (state_ != InitialCloudWifiOnlyState::Pending) return false;
        output = profile_;
        clearWifiCredentialProfile(profile_);
        state_ = InitialCloudWifiOnlyState::Processing;
        return true;
    }

    void finish(Result result) {
        if (state_ == InitialCloudWifiOnlyState::Processing)
            state_ = result ? InitialCloudWifiOnlyState::Committed : InitialCloudWifiOnlyState::Failed;
    }

    void cancel() {
        clearWifiCredentialProfile(profile_);
        state_ = InitialCloudWifiOnlyState::Cancelled;
    }

private:
    WifiCredentialProfile profile_;
    uint32_t session_;
    InitialCloudWifiOnlyState state_;
};

// Consumes the cooperative owner's temporary profile even on failure. The
// owner must serialize cancellation/root changes with this synchronous call.
// Success is journal persistence, not WiFi association or Cloud availability.
inline Result commitInitialCloudWifiOnly(WifiCredentialProfile& profile,
    IInitialCloudWifiWriteGuard& guard, IWifiCredentialSink& sink) {
    const WifiNetworkConfig config = profile.networkConfig();
    Result result = validateWifiNetworkConfig(config);
    if (result) result = guard.validateWifiWrite();
    if (result) result = sink.commit(config);
    clearWifiCredentialProfile(profile);
    return result;
}

} // namespace blinker
#endif
