#include "BlinkerRenesasUnoWifiAdapter.h"

#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/provisioning/WifiCredential.h>
#include <WiFiS3.h>

#include <string.h>

namespace blinker {

RenesasUnoWifiStation::RenesasUnoWifiStation()
    : state_(WifiStationState::Stopped),
      lastError_(ErrorCode::Ok) {}

void RenesasUnoWifiStation::fail(ErrorCode error) {
    state_ = WifiStationState::Failed;
    lastError_ = error;
}

Result RenesasUnoWifiStation::start(const WifiNetworkConfig& config) {
    if (state_ != WifiStationState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    const Result valid = validateWifiNetworkConfig(config);
    if (!valid) return valid;
    if (config.authentication != WifiAuthentication::Open &&
        config.authentication != WifiAuthentication::Wpa2Personal) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (config.credential.size > 63U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    char ssid[kWifiSsidMaxSize + 1U] = {};
    char credential[kWifiCredentialMaxSize + 1U] = {};
    memcpy(ssid, config.ssid.data, config.ssid.size);
    if (!config.credential.empty()) {
        memcpy(credential, config.credential.data, config.credential.size);
    }

    state_ = WifiStationState::Connecting;
    lastError_ = ErrorCode::Ok;
    WiFi.setTimeout(0U);
    if (config.authentication == WifiAuthentication::Open) {
        (void)WiFi.begin(ssid);
    } else {
        (void)WiFi.begin(ssid, credential);
    }

    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(credential), sizeof(credential)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(ssid), sizeof(ssid)));
    return Result::success();
}

void RenesasUnoWifiStation::poll() {
    if (state_ == WifiStationState::Stopped ||
        state_ == WifiStationState::Failed) {
        return;
    }
    const wl_status_t nativeState =
        static_cast<wl_status_t>(WiFi.status());
    if (nativeState == WL_CONNECTED) {
        state_ = WifiStationState::Connected;
        lastError_ = ErrorCode::Ok;
    } else if (nativeState == WL_NO_MODULE) {
        fail(ErrorCode::NotConfigured);
    } else if (nativeState == WL_NO_SSID_AVAIL ||
               nativeState == WL_CONNECT_FAILED ||
               state_ == WifiStationState::Connected) {
        fail(ErrorCode::NotConnected);
    }
}

void RenesasUnoWifiStation::stop() {
    if (state_ != WifiStationState::Stopped) {
        (void)WiFi.disconnect();
    }
    state_ = WifiStationState::Stopped;
    lastError_ = ErrorCode::Ok;
}

} // namespace blinker
