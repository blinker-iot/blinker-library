#include <Esp32WiFi.h>
#include <WiFi.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

WiFiClass WiFi;

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
}

blinker::WifiNetworkConfig network(
    const char* ssid,
    blinker::WifiAuthentication authentication,
    const char* credential) {
    blinker::WifiNetworkConfig config;
    config.ssid = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(ssid), std::strlen(ssid));
    config.authentication = authentication;
    if (credential != nullptr) {
        config.credential = blinker::ByteView(
            reinterpret_cast<const uint8_t*>(credential),
            std::strlen(credential));
    }
    return config;
}

} // namespace

int main() {
    WiFi.reset();
    blinker::Esp32WifiStation station;
    blinker::WifiNetworkConfig secured = network(
        "station-test",
        blinker::WifiAuthentication::Wpa2Personal,
        "password-123");
    expect(station.start(secured).ok(), "secured start");
    expect(WiFi.beginCalls == 1U, "one begin call");
    expect(
        WiFi.persistentCalls == 1U && !WiFi.lastPersistent,
        "SDK WiFi persistence was not disabled");
    expect(
        WiFi.lastMinSecurity == WIFI_AUTH_WPA2_PSK,
        "WPA2 security floor");
    expect(!WiFi.passwordWasNull, "secured password passed");
    expect(std::strcmp(WiFi.lastSsid, "station-test") == 0, "SSID copied");
    expect(
        std::strcmp(WiFi.lastPassword, "password-123") == 0,
        "password copied");
    expect(
        station.start(secured).code() == blinker::ErrorCode::StateConflict,
        "duplicate start rejected");

    WiFi.statusResult = WL_CONNECTED;
    station.poll();
    expect(
        station.state() == blinker::WifiStationState::Connected,
        "connected state");
    WiFi.statusResult = WL_CONNECTION_LOST;
    station.poll();
    expect(
        station.state() == blinker::WifiStationState::Failed &&
            station.lastError() == blinker::ErrorCode::NotConnected,
        "lost connection is terminal");
    station.stop();
    expect(WiFi.disconnectCalls == 1U, "async disconnect called");
    expect(
        station.state() == blinker::WifiStationState::Stopped,
        "stop resets state");

    WiFi.reset();
    blinker::WifiNetworkConfig open = network(
        "open-network", blinker::WifiAuthentication::Open, nullptr);
    expect(station.start(open).ok(), "open start");
    expect(WiFi.passwordWasNull, "open network uses null password");
    expect(
        WiFi.lastMinSecurity == WIFI_AUTH_OPEN,
        "open security floor");
    WiFi.statusResult = WL_NO_SHIELD;
    station.poll();
    expect(
        station.lastError() == blinker::ErrorCode::NotConfigured,
        "missing radio mapping");
    station.stop();

    WiFi.reset();
    blinker::WifiNetworkConfig wpa3 = network(
        "wpa3-network",
        blinker::WifiAuthentication::Wpa3Personal,
        "wpa3-password");
    expect(station.start(wpa3).ok(), "WPA3 start");
    expect(
        WiFi.lastMinSecurity == WIFI_AUTH_WPA3_PSK,
        "WPA3 security floor");
    station.stop();

    WiFi.reset();
    blinker::WifiNetworkConfig transition = network(
        "transition-network",
        blinker::WifiAuthentication::Wpa2Wpa3Personal,
        "transition-password");
    expect(station.start(transition).ok(), "transition start");
    expect(
        WiFi.lastMinSecurity == WIFI_AUTH_WPA2_PSK,
        "transition network uses WPA2 floor");
    station.stop();

    WiFi.reset();
    WiFi.beginResult = WL_CONNECT_FAILED;
    expect(
        station.start(secured).code() == blinker::ErrorCode::NotConnected,
        "native begin failure mapping");
    station.stop();

    WiFi.reset();
    blinker::WifiNetworkConfig invalid;
    expect(
        station.start(invalid).code() == blinker::ErrorCode::InvalidArgument,
        "invalid config rejected");
    expect(WiFi.beginCalls == 0U, "invalid config never reaches SDK");

    std::cout << "esp32 wifi adapter tests passed" << std::endl;
    return 0;
}
