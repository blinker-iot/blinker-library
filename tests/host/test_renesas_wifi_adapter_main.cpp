#include <UnoR4WiFi.h>
#include <WiFiS3.h>
#include <UnoR4Storage.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

CWifi WiFi;

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
    blinker::RenesasUnoWifiStation station;
    blinker::WifiNetworkConfig secured = network(
        "station-test",
        blinker::WifiAuthentication::Wpa2Personal,
        "password-123");

    // WiFiS3 returns WL_CONNECT_FAILED when its association wait is zero even
    // after the BEGINSTA command was accepted. start() must not misclassify it.
    WiFi.beginResult = WL_CONNECT_FAILED;
    expect(station.start(secured).ok(), "zero-wait start accepted");
    expect(WiFi.timeoutValue == 0U, "association wait disabled");
    expect(WiFi.beginCalls == 1U, "one begin call");
    expect(!WiFi.openBegin, "secured overload selected");
    expect(std::strcmp(WiFi.lastSsid, "station-test") == 0, "SSID copied");
    expect(
        std::strcmp(WiFi.lastPassword, "password-123") == 0,
        "password copied");

    WiFi.statusResult = WL_CONNECTED;
    station.poll();
    expect(
        station.state() == blinker::WifiStationState::Connected,
        "connected state");
    WiFi.statusResult = WL_DISCONNECTED;
    station.poll();
    expect(
        station.state() == blinker::WifiStationState::Failed &&
            station.lastError() == blinker::ErrorCode::NotConnected,
        "disconnect mapping");
    station.stop();
    expect(WiFi.disconnectCalls == 1U, "disconnect called");

    WiFi.reset();
    blinker::WifiNetworkConfig open = network(
        "open-network", blinker::WifiAuthentication::Open, nullptr);
    expect(station.start(open).ok(), "open start");
    expect(WiFi.openBegin, "open overload selected");
    WiFi.statusResult = WL_NO_MODULE;
    station.poll();
    expect(
        station.lastError() == blinker::ErrorCode::NotConfigured,
        "missing module mapping");
    station.stop();

    WiFi.reset();
    blinker::WifiNetworkConfig wpa3 = network(
        "wpa3-network",
        blinker::WifiAuthentication::Wpa3Personal,
        "wpa3-password");
    expect(
        station.start(wpa3).code() ==
            blinker::ErrorCode::UnsupportedFeature,
        "unsupported WPA3 rejected");
    expect(WiFi.beginCalls == 0U, "WPA3 never silently downgrades");

    blinker::WifiNetworkConfig transition = network(
        "transition-network",
        blinker::WifiAuthentication::Wpa2Wpa3Personal,
        "transition-password");
    expect(
        station.start(transition).code() ==
            blinker::ErrorCode::UnsupportedFeature,
        "unsupported transition mode rejected");
    expect(WiFi.beginCalls == 0U, "transition mode never reaches SDK");

    static const char rawPsk[] =
        "0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef";
    blinker::WifiNetworkConfig raw = network(
        "raw-psk-network",
        blinker::WifiAuthentication::Wpa2Personal,
        rawPsk);
    expect(
        station.start(raw).code() ==
            blinker::ErrorCode::UnsupportedFeature,
        "unsupported raw PSK rejected");
    expect(WiFi.beginCalls == 0U, "raw PSK never reaches SDK");

    WiFi.reset();
    blinker::WifiNetworkConfig invalid;
    expect(
        station.start(invalid).code() == blinker::ErrorCode::InvalidArgument,
        "invalid config rejected");
    expect(WiFi.beginCalls == 0U, "invalid config never reaches SDK");

    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    expect(bank.begin().ok(), "shared modem namespace");
    expect(station.start(secured).ok(), "station before task operation");
    blinker::RenesasUnoModemOwner& owner = blinker::renesasUnoModemOwner();
    {
        blinker::RenesasUnoModemOwner::Operation operation(owner, &bank, 1000U);
        expect(operation.result().ok(), "storage owns modem");
        station.poll();
        expect(WiFi.statusCalls == 0U, "station poll cannot take storage modem");
        expect(owner.prepare().ok(), "prepare storage RPC");
        expect(!owner.complete(false), "failed RPC quarantines modem");
    }
    station.poll();
    expect(station.state() == blinker::WifiStationState::Failed &&
           station.lastError() == blinker::ErrorCode::StateConflict,
           "station exposes quarantined modem");
    station.stop();
    expect(WiFi.statusCalls == 0U && WiFi.disconnectCalls == 0U,
           "poll and cleanup send no RPC after ambiguity");
    expect(station.start(secured).code() == blinker::ErrorCode::StateConflict,
           "reconnect does not bypass quarantine");
    expect(WiFi.beginCalls == 1U, "no duplicate station begin");

    std::cout << "renesas wifi adapter tests passed" << std::endl;
    return 0;
}
