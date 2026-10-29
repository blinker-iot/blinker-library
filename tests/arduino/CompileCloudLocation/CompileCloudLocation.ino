// Internal LOC-2 full-link candidate, not an additional public support target.
#include <Blinker.h>
#include <BlinkerV2/location/CloudLocationBinding.h>
#include "Schema.h"

#if !defined(ARDUINO_ARCH_ESP32)
#error "This internal candidate admits ESP32 manual WiFi only"
#endif
#if !BLINKER_OFFICIAL_CLOUD_SESSION_TLS || !BLINKER_OFFICIAL_MQTT_TLS
#error "Install the existing cloud-beta public profile before compiling"
#endif
#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#error "This internal candidate does not admit persistent tasks"
#endif
#if defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET
#error "This internal candidate does not admit a LAN transport"
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

// Destruction is reverse declaration order: Device releases the borrowed
// binding before either the binding or its Snapshot can be destroyed.
blinker::LocationSnapshot locationSnapshot;
blinker::CloudLocationBinding locationBinding(locationSnapshot);
blinker::TimeSyncService businessTime;
blinker::Device device(blinker::cloud_location_candidate::fields,
    blinker::DeviceConfig(blinker::cloud_location_candidate::manifestRevision));
blinker::Product* runningProduct = nullptr;
bool currentPower = false;

blinker::Result applyPower(void*, const blinker::EndpointHandle& endpoint,
    const blinker::EndpointValueView& value, const blinker::RxContext&) {
    bool requested = false;
    blinker::Result result = value.asBool(requested);
    if (result) result = endpoint.set(requested);
    if (result) currentPower = requested;
    return result;
}

blinker::Result sampleUptime(void*, uint16_t id, blinker::bbp2::IdBodyWriter& writer) {
    return writer.writeUnsigned(id, millis() / 1000U);
}

} // namespace

// Optional synchronous ports for a real user-supplied GNSS driver. These are
// never called by a timer or populated with a synthetic/zero/default fix.
// sampledAtMillis must use the same boot-local millis() clock as this product.
blinker::Result acceptDeviceGnssObservation(const blinker::LocationFix& observation,
    uint32_t sampledAtMillis) {
    return locationSnapshot.update(observation, sampledAtMillis, millis());
}

void deviceGnssFixLost() { locationSnapshot.invalidate(); }

void setup() {
    Serial.begin(115200);
    blinker::Result result = device.endpoints().find("power").onCommand(&applyPower, nullptr);
    if (result) result = device.endpoints().find("uptimeSeconds").onSample(&sampleUptime, nullptr);
    if (result) result = locationBinding.attach(device, blinker::cloud_location_candidate::locationKey);
    if (!result) { Serial.println("LOC-2 binding unavailable"); return; }

    // Reuse the existing manual WiFi composition and public TLS configuration.
    // A local static Product outlives loop() and is destroyed before Device.
    static blinker::Product product(device, blinker::integration::lifecycle(
        blinker::WifiDeviceKeyProfile(BLINKER_DEVICE_KEY, BLINKER_WIFI_SSID, BLINKER_WIFI_PASSWORD)));
    result = product.begin(&businessTime);
    if (result) result = device.endpoints().find("power").set(currentPower);
    if (!result) { product.end(); Serial.println("LOC-2 startup unavailable"); return; }
    runningProduct = &product;
    // position deliberately has no set()/report()/setEncoded() call. Without
    // a real GNSS observation it remains absent; ordinary uptime still samples.
}

void loop() {
    if (runningProduct != nullptr) runningProduct->run();
}
