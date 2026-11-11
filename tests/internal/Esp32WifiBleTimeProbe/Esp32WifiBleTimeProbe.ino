// Internal HIL: the actual persisted WiFiProv/WiFi+BLE graph and one business clock.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h>
#include "../support/ObservedTime.h"

namespace {
using namespace blinker;
namespace official = blinker::integration::official_detail;
BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);
bool currentPower = false;
uint32_t presses = 0U;
auto schema = makeDeviceSchema(power, buttonPresses, button);
Device device(schema);
official::Esp32OfficialWifiBleProduct lifecycle;
blinker_test::ObservedTime clockService;
Product product(device, lifecycle);
bool bootUnsynchronized = false;
ErrorCode startError = ErrorCode::NotConfigured;
void onPower(bool value) { currentPower = value; power.report(value); }
void onButton() { buttonPresses.report(++presses); }
void snapshot() {
    WallClockSnapshot wall;
    const bool valid = clockService.readTime(wall);
    const uint64_t utc = wall.utcMillis / 1000U;
    const auto status = product.status();
    char line[440];
    const int size = snprintf(line, sizeof(line),
        "B3_MIXED {\"ms\":%lu,\"valid\":%u,\"utc\":%lu,\"cloud\":%u,\"local\":%u,\"radio\":%u,\"bootUnsynced\":%u,\"requests\":%lu,\"samples\":%lu,\"serverSamples\":%lu,\"localSamples\":%lu,\"sampleAt\":%lu,\"power\":%u,\"presses\":%lu,\"state\":%u,\"error\":%u,\"startError\":%u,\"source\":%u,\"ageMs\":%lu,\"uncertaintyMs\":%lu}\n",
        static_cast<unsigned long>(millis()), valid, static_cast<unsigned long>(utc),
        status.cloudReady, status.localReady, ::WiFi.status() == WL_CONNECTED, bootUnsynchronized,
        static_cast<unsigned long>(clockService.requests), static_cast<unsigned long>(clockService.samples),
        static_cast<unsigned long>(clockService.serverSamples), static_cast<unsigned long>(clockService.localSamples),
        static_cast<unsigned long>(clockService.sampleAt), currentPower, static_cast<unsigned long>(presses),
        static_cast<unsigned>(status.state), static_cast<unsigned>(status.lastError), static_cast<unsigned>(startError),
        static_cast<unsigned>(wall.source), static_cast<unsigned long>(wall.ageMillis),
        static_cast<unsigned long>(wall.uncertaintyMillis));
    if (size > 0 && static_cast<size_t>(size) < sizeof(line)) Serial.write(reinterpret_cast<const uint8_t*>(line), size);
}
}
void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0U);
    power.onWrite(onPower); button.onInvoke(onButton);
    const Result result = product.begin(&clockService);
    startError = result.code();
    uint64_t utc = 123U; bootUnsynchronized = !clockService.time(utc) && utc == 0U;
    if (result) { power.report(false); buttonPresses.report(0U); }
}
void loop() {
    product.run();
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 's') snapshot();
#if defined(BLINKER_TEST_NETWORK_RESET)
    // Explicit network-only HIL setup; once per boot, never erase access/Key.
    static bool resetIssued = false;
    if (command == 'n' && !resetIssued) {
        resetIssued = true;
        const Result result = product.resetNetwork();
        Serial.printf("B3_NETWORK_RESET error=%u\n", static_cast<unsigned>(result.code()));
    }
#endif
}
