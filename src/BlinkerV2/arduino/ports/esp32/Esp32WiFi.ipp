#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/provisioning/WifiCredential.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <string.h>

namespace blinker {

inline Esp32WifiStation::Esp32WifiStation()
    : state_(WifiStationState::Stopped),
      lastError_(ErrorCode::Ok) {}

inline void Esp32WifiStation::fail(ErrorCode error) {
    state_ = WifiStationState::Failed;
    lastError_ = error;
}

inline Result Esp32WifiStation::start(const WifiNetworkConfig& config) {
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
    // WiFi layer retain a second copy in SDK NVS or run a competing retry
    // loop; WifiConnectionLifecycle owns reconnect policy.
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
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

inline void Esp32WifiStation::poll() {
    if (state_ == WifiStationState::Stopped ||
        state_ == WifiStationState::Failed) {
        return;
    }
    const bool wasConnected = state_ == WifiStationState::Connected;
    const wl_status_t nativeState = WiFi.status();
    if (nativeState == WL_CONNECTED) {
        state_ = WifiStationState::Connected;
        lastError_ = ErrorCode::Ok;
    } else if (nativeState == WL_NO_SHIELD) {
        fail(ErrorCode::NotConfigured);
    } else if (wasConnected) {
        fail(ErrorCode::NotConnected);
    }
    // WL_NO_SSID_AVAIL and WL_CONNECT_FAILED can be transient while the
    // asynchronous ESP32 association is still starting. The portable WiFi
    // lifecycle owns the bounded connection timeout, so keep polling until it
    // either observes WL_CONNECTED or expires that deadline.
}

inline void Esp32WifiStation::stop() {
    if (state_ != WifiStationState::Stopped) {
        // Arduino's disconnect() is a no-op until its connected bit is set,
        // but an association attempt may still be active. Cancel it at the
        // ESP-IDF boundary before the lifecycle starts another attempt.
        (void)esp_wifi_disconnect();
    }
    state_ = WifiStationState::Stopped;
    lastError_ = ErrorCode::Ok;
}

} // namespace blinker
