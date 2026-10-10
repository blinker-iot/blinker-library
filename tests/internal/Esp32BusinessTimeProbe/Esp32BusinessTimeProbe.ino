// Internal hardware gate only. No faults, counters or SDK policy in public API.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32WiFiPlatform.h>
#include <BlinkerV2/arduino/internal/WiFiProduct.h>
#include <BlinkerV2/runtime/TimeSyncService.h>

#ifndef ARDUINO_ARCH_ESP32
#error "This network fault probe requires ESP32"
#endif
#ifndef BLINKER_DEVICE_KEY
#define BLINKER_DEVICE_KEY "REPLACE_WITH_DEVICE_KEY"
#endif
#ifndef BLINKER_WIFI_SSID
#define BLINKER_WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef BLINKER_WIFI_PASSWORD
#define BLINKER_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

namespace {
using namespace blinker;
namespace official = blinker::integration::official_detail;
uint32_t offlineUntil = 0U;
bool outageIssued = false;
bool offline() {
    return outageIssued && static_cast<int32_t>(offlineUntil - millis()) > 0;
}

// Only the injected station rejects new starts. Real WiFi/MQTT/store/runtime
// remain unchanged, and the real radio is disconnected once, not every loop.
class FaultStation final : public IWifiStation {
public:
    explicit FaultStation(IWifiStation& station) : real_(station), stopped_(false) {}
    Result start(const WifiNetworkConfig& config) override {
        return offline() ? Result::failure(ErrorCode::NotConnected) : real_.start(config);
    }
    void poll() override {
        if (offline()) {
            if (!stopped_) { real_.stop(); stopped_ = true; }
        } else { stopped_ = false; real_.poll(); }
    }
    WifiStationState state() const override {
        return offline() ? WifiStationState::Failed : real_.state();
    }
    ErrorCode lastError() const override {
        return offline() ? ErrorCode::NotConnected : real_.lastError();
    }
    void stop() override { real_.stop(); }
private:
    IWifiStation& real_;
    bool stopped_;
};
class FaultPlatform : public official::Esp32WifiNetworkPlatform {
public:
    FaultPlatform() : station_(official::Esp32WifiNetworkPlatform::wifiStation()) {}
    IWifiStation& wifiStation() { return station_; }
private:
    FaultStation station_;
};

// Observe accepted real protocol responses, not a fake local UTC source.
class ObservedTime final : public ITimeSync {
public:
    uint32_t requests = 0U, samples = 0U, sampleAt = 0U;
    Result configure(IClock& tick, IRandom& random) override { return service_.configure(tick, random); }
    void reset(bool clearTime) override { service_.reset(clearTime); }
    bool poll(const TimeSyncPeer& peer) override { return service_.poll(peer); }
    void sessionClosed(uint8_t transport, uint32_t session) override { service_.sessionClosed(transport, session); }
    Result request(const TimeSyncPeer& peer, uint16_t sequence, MutableByteSpan body, size_t& size) override {
        const Result result = service_.request(peer, sequence, body, size);
        if (result) ++requests;
        return result;
    }
    Result receive(const TimeSyncPeer& peer, const bbp2::FrameView& frame) override {
        const Result result = service_.receive(peer, frame);
        if (result && frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TimeResponse)) {
            ++samples; sampleAt = millis();
        }
        return result;
    }
    bool readTime(WallClockSnapshot& snapshot) override { return service_.readTime(snapshot); }
private:
    TimeSyncService service_;
};
BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));
auto schema = makeDeviceSchema(power, uptimeSeconds);
Device device(schema);
official::DeviceKeyWifiProduct<FaultPlatform> lifecycle;
Product product(device, lifecycle);
ObservedTime clockService;
bool bootUnsynchronized = false;
uint32_t runMax = 0U;
ErrorCode startError = ErrorCode::NotConfigured;

void onPower(bool value) { power.report(value); }
uint32_t sampleUptime() { return millis() / 1000U; }
void snapshot() {
    uint64_t utc = 0U;
    const bool valid = clockService.time(utc);
    const ProductStatus status = product.status();
    char line[256];
    const int size = snprintf(line, sizeof(line),
        "{\"ms\":%lu,\"utc\":%lu,\"valid\":%u,\"bootUnsynced\":%u,\"offline\":%u,\"radio\":%u,\"cloud\":%u,\"requests\":%lu,\"samples\":%lu,\"sampleAt\":%lu,\"runMaxUs\":%lu,\"startError\":%u}\n",
        static_cast<unsigned long>(millis()), static_cast<unsigned long>(utc), valid, bootUnsynchronized,
        offline(), ::WiFi.status() == WL_CONNECTED, status.cloudReady,
        static_cast<unsigned long>(clockService.requests), static_cast<unsigned long>(clockService.samples),
        static_cast<unsigned long>(clockService.sampleAt), static_cast<unsigned long>(runMax),
        static_cast<unsigned>(startError));
    if (size > 0 && static_cast<size_t>(size) < sizeof(line)) Serial.write(reinterpret_cast<const uint8_t*>(line), size);
}
} // namespace

void setup() {
    Serial.begin(115200);
    power.onWrite(onPower);
    uptimeSeconds.onSample(sampleUptime);
    Result result = lifecycle.configure(BLINKER_WIFI_SSID, BLINKER_WIFI_PASSWORD, BLINKER_DEVICE_KEY);
    if (result) result = product.begin(&clockService);
    startError = result.code();
    uint64_t utc = 123U;
    bootUnsynchronized = !clockService.time(utc) && utc == 0U;
    if (result) power.report(false);
}
void loop() {
    const uint32_t before = micros();
    product.run();
    const uint32_t elapsed = micros() - before;
    if (elapsed > runMax) runMax = elapsed;
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 'o' && !outageIssued) { offlineUntil = millis() + 30000U; outageIssued = true; }
    if (command == 's') snapshot(); // Query only; no unsolicited or periodic output.
}
