#ifndef BLINKER_RUNTIME_WIFICONTROLPLANEARENA_H
#define BLINKER_RUNTIME_WIFICONTROLPLANEARENA_H

#include "../control/HttpCloudSessionProvider.h"
#include "../core/SecureMemory.h"
#include "../provisioning/WifiOnboardingFlow.h"

namespace blinker {
namespace runtime_detail {

constexpr size_t wifiControlMaximum(size_t first, size_t second) {
    return first > second ? first : second;
}

} // namespace runtime_detail

enum : size_t {
    kWifiControlRequestArenaSize =
        runtime_detail::wifiControlMaximum(
            static_cast<size_t>(
                kHttpCloudSessionMinimumRequestBufferSize),
            static_cast<size_t>(
                kWifiOnboardingMinimumRequestBufferSize)),
    kWifiControlResponseArenaSize =
        runtime_detail::wifiControlMaximum(
            static_cast<size_t>(
                kHttpCloudSessionMinimumResponseBufferSize),
            static_cast<size_t>(
                kWifiOnboardingMinimumResponseBufferSize)),
    kWifiControlSharedArenaSize =
        runtime_detail::wifiControlMaximum(
            static_cast<size_t>(kCloudSessionCredentialArenaSize),
            static_cast<size_t>(kWifiOnboardingMinimumWorkspaceSize)),
    kWifiControlPlaneArenaSize =
        kWifiControlRequestArenaSize +
        kWifiControlResponseArenaSize +
        kWifiControlSharedArenaSize
};

// One physical arena is borrowed by two non-overlapping phases:
// onboarding uses request/response/workspace, then Cloud Session uses
// request/response/session credentials. Neither phase owns another copy.
class WifiControlPlaneArena {
public:
    WifiControlPlaneArena() : request_(), response_(), shared_() {}
    ~WifiControlPlaneArena() { clear(); }

    MutableByteSpan onboardingRequest() {
        return MutableByteSpan(request_, sizeof(request_));
    }
    MutableByteSpan onboardingResponse() {
        return MutableByteSpan(response_, sizeof(response_));
    }
    MutableByteSpan onboardingWorkspace() {
        return MutableByteSpan(
            shared_,
            kWifiOnboardingMinimumWorkspaceSize);
    }

    MutableByteSpan cloudRequest() {
        return MutableByteSpan(request_, sizeof(request_));
    }
    MutableByteSpan cloudResponse() {
        return MutableByteSpan(response_, sizeof(response_));
    }
    MutableCharSpan cloudCredentials() {
        return MutableCharSpan(
            reinterpret_cast<char*>(shared_),
            sizeof(shared_));
    }

    void clear() {
        secureZero(MutableByteSpan(request_, sizeof(request_)));
        secureZero(MutableByteSpan(response_, sizeof(response_)));
        secureZero(MutableByteSpan(shared_, sizeof(shared_)));
    }

private:
    uint8_t request_[kWifiControlRequestArenaSize];
    uint8_t response_[kWifiControlResponseArenaSize];
    uint8_t shared_[kWifiControlSharedArenaSize];

    WifiControlPlaneArena(const WifiControlPlaneArena&);
    WifiControlPlaneArena& operator=(const WifiControlPlaneArena&);
};

static_assert(
    sizeof(WifiControlPlaneArena) == kWifiControlPlaneArenaSize,
    "WiFi control-plane arena contains unexpected padding");

} // namespace blinker

#endif
