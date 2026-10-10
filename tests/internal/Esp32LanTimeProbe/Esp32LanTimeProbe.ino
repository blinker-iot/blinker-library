// Internal HIL only: borrow the real MQTT Client, inject a bounded TCP outage.
// No WiFi shutdown, fake time/grant, extra socket or production fault switch.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32WiFiPlatform.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32WiFiLanExtension.h>
#include <BlinkerV2/arduino/internal/WiFiProduct.h>
#include "../support/ObservedTime.h"

#ifndef BLINKER_DEVICE_KEY
#define BLINKER_DEVICE_KEY "REPLACE_WITH_DEVICE_KEY"
#define BLINKER_WIFI_SSID "YOUR_WIFI_SSID"
#define BLINKER_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

namespace {
using namespace blinker;
namespace official = blinker::integration::official_detail;
class FaultMqttClient;
FaultMqttClient* mqtt = nullptr;

class FaultMqttClient final : public ::Client {
public:
    explicit FaultMqttClient(::Client& client) : real_(client) { mqtt = this; }
    bool paused() const { return until_ != 0U && static_cast<int32_t>(until_ - millis()) > 0; }
    void pause() { until_ = millis() + 60000U; real_.stop(); }
    void resume() { until_ = 0U; }
    int connect(IPAddress ip, uint16_t port) override { return paused() ? 0 : real_.connect(ip, port); }
    int connect(const char* host, uint16_t port) override { return paused() ? 0 : real_.connect(host, port); }
    size_t write(uint8_t byte) override { return real_.write(byte); }
    size_t write(const uint8_t* data, size_t size) override { return real_.write(data, size); }
    int available() override { return real_.available(); }
    int read() override { return real_.read(); }
    int read(uint8_t* data, size_t size) override { return real_.read(data, size); }
    int peek() override { return real_.peek(); }
    void flush() override { real_.flush(); }
    void stop() override { real_.stop(); }
    uint8_t connected() override { return real_.connected(); }
    operator bool() override { return static_cast<bool>(real_); }
private:
    ::Client& real_;
    uint32_t until_ = 0U;
};
class FaultPlatform : public official::Esp32WifiNetworkPlatform {
public:
    using MqttClient = FaultMqttClient;
    FaultPlatform() : mqtt_(official::Esp32WifiNetworkPlatform::mqttNetwork()) {}
    MqttClient& mqttNetwork() { return mqtt_; }
private:
    MqttClient mqtt_;
};

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);
bool currentPower = false;
uint32_t presses = 0U;
auto schema = makeDeviceSchema(power, buttonPresses, button);
Device device(schema);
using Stack = official::DeviceKeyWifiStack<FaultPlatform>;
official::DeviceKeyWifiProduct<FaultPlatform, official::Esp32WifiLanExtension<Stack>> lifecycle;
blinker_test::ObservedTime clockService;
Product product(device, lifecycle);
bool bootUnsynchronized = false, clockCleared = false;
ErrorCode startError = ErrorCode::NotConfigured;
void onPower(bool value) { currentPower = value; power.report(value); }
void onButton() { buttonPresses.report(++presses); }
void snapshot() {
    uint64_t utc = 0U;
    const bool valid = clockService.time(utc);
    const auto status = product.status();
    char line[320];
    const int size = snprintf(line, sizeof(line),
        "B3_LAN {\"ms\":%lu,\"valid\":%u,\"utc\":%lu,\"cloud\":%u,\"radio\":%u,\"paused\":%u,\"bootUnsynced\":%u,\"cleared\":%u,\"requests\":%lu,\"samples\":%lu,\"serverSamples\":%lu,\"localSamples\":%lu,\"sampleAt\":%lu,\"power\":%u,\"presses\":%lu,\"error\":%u}\n",
        static_cast<unsigned long>(millis()), valid, static_cast<unsigned long>(utc), status.cloudReady,
        ::WiFi.status() == WL_CONNECTED, mqtt && mqtt->paused(), bootUnsynchronized, clockCleared,
        static_cast<unsigned long>(clockService.requests), static_cast<unsigned long>(clockService.samples),
        static_cast<unsigned long>(clockService.serverSamples), static_cast<unsigned long>(clockService.localSamples),
        static_cast<unsigned long>(clockService.sampleAt), currentPower, static_cast<unsigned long>(presses),
        static_cast<unsigned>(startError));
    if (size > 0 && static_cast<size_t>(size) < sizeof(line)) Serial.write(reinterpret_cast<const uint8_t*>(line), size);
}
}
void setup() {
    Serial.begin(115200); power.onWrite(onPower); button.onInvoke(onButton);
    Result result = lifecycle.configure(BLINKER_WIFI_SSID, BLINKER_WIFI_PASSWORD, BLINKER_DEVICE_KEY);
    if (result) result = product.begin(&clockService);
    startError = result.code();
    uint64_t utc = 123U; bootUnsynchronized = !clockService.time(utc) && utc == 0U;
    if (result) { power.report(false); buttonPresses.report(0U); }
}
void loop() {
    product.run();
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 'p' && mqtt && !mqtt->paused()) mqtt->pause(); // 60 s, auto-resumes even without operator.
    if (command == 'r' && mqtt) mqtt->resume();
    if (command == 'c' && !clockCleared && mqtt && mqtt->paused() && !product.status().cloudReady) {
        clockService.reset(true); clockCleared = true; // One optional business-anchor loss; NOT security time.
    }
    if (command == 's') snapshot();
}
