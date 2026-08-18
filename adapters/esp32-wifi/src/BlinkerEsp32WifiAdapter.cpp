#include "BlinkerEsp32WifiAdapter.h"

#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/provisioning/WifiCredential.h>
#include <WiFi.h>

#include <string.h>

namespace blinker {

Esp32WifiStation::Esp32WifiStation()
    : state_(WifiStationState::Stopped),
      lastError_(ErrorCode::Ok) {}

void Esp32WifiStation::fail(ErrorCode error) {
    state_ = WifiStationState::Failed;
    lastError_ = error;
}

Result Esp32WifiStation::start(const WifiNetworkConfig& config) {
    if (state_ != WifiStationState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    const Result valid = validateWifiNetworkConfig(config);
    if (!valid) return valid;

    wifi_auth_mode_t minimumSecurity = WIFI_AUTH_OPEN;
    if (config.authentication == WifiAuthentication::Wpa3Personal) {
        minimumSecurity = WIFI_AUTH_WPA3_PSK;
    } else if (config.authentication != WifiAuthentication::Open) {
        // WPA2/WPA3 transition networks are also admitted by a WPA2 floor.
        minimumSecurity = WIFI_AUTH_WPA2_PSK;
    }
    // WifiCredentialStore is the single durable source. Do not let Arduino's
    // WiFi layer retain a second copy in the SDK WiFi NVS namespace.
    WiFi.persistent(false);
    WiFi.setMinSecurity(minimumSecurity);

    char ssid[kWifiSsidMaxSize + 1U] = {};
    char credential[kWifiCredentialMaxSize + 1U] = {};
    memcpy(ssid, config.ssid.data, config.ssid.size);
    if (!config.credential.empty()) {
        memcpy(credential, config.credential.data, config.credential.size);
    }

    state_ = WifiStationState::Connecting;
    lastError_ = ErrorCode::Ok;
    const wl_status_t nativeState =
        config.authentication == WifiAuthentication::Open
            ? WiFi.begin(ssid)
            : WiFi.begin(ssid, credential);

    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(credential), sizeof(credential)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(ssid), sizeof(ssid)));

    if (nativeState == WL_CONNECT_FAILED) {
        fail(ErrorCode::NotConnected);
        return Result::failure(lastError_);
    }
    return Result::success();
}

void Esp32WifiStation::poll() {
    if (state_ == WifiStationState::Stopped ||
        state_ == WifiStationState::Failed) {
        return;
    }
    const wl_status_t nativeState = WiFi.status();
    if (nativeState == WL_CONNECTED) {
        state_ = WifiStationState::Connected;
        lastError_ = ErrorCode::Ok;
    } else if (nativeState == WL_NO_SHIELD) {
        fail(ErrorCode::NotConfigured);
    } else if (nativeState == WL_NO_SSID_AVAIL ||
               nativeState == WL_CONNECT_FAILED ||
               state_ == WifiStationState::Connected) {
        fail(ErrorCode::NotConnected);
    }
}

void Esp32WifiStation::stop() {
    if (state_ != WifiStationState::Stopped) {
        (void)WiFi.disconnectAsync(false, false);
    }
    state_ = WifiStationState::Stopped;
    lastError_ = ErrorCode::Ok;
}

} // namespace blinker
