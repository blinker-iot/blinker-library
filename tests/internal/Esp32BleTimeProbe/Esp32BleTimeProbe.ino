// Internal observation of the ordinary BLE product, also usable as a Hub child.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32BLEProduct.h>
#include "../support/ObservedTime.h"

namespace {
BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);
bool currentPower = false, bootUnsynchronized = false;
uint32_t presses = 0U;
blinker_test::TimeExchangeTrace trace;
blinker_test::ObservedTime businessTime(&trace);
auto schema = blinker::makeDeviceSchema(power, buttonPresses, button);
blinker::Device device(schema);
blinker::integration::official_detail::Esp32PlatformBleProduct lifecycle;
blinker::Product product(device, lifecycle);
blinker::ErrorCode startError = blinker::ErrorCode::NotConfigured;
void onPower(bool value) { currentPower = value; power.report(value); }
void onButton() { buttonPresses.report(++presses); }
void snapshot() {
    blinker::WallClockSnapshot wall;
    const bool valid = businessTime.readTime(wall);
    Serial.printf("B3_CHILD {\"ms\":%lu,\"valid\":%u,\"utc\":%lu,\"source\":%u,\"age\":%lu,\"uncertainty\":%lu,\"bootUnsynced\":%u,\"requests\":%lu,\"samples\":%lu,\"server\":%lu,\"local\":%lu,\"power\":%u,\"presses\":%lu,\"startError\":%u}\n",
        static_cast<unsigned long>(millis()), valid, static_cast<unsigned long>(wall.utcMillis / 1000U),
        static_cast<unsigned>(wall.source), static_cast<unsigned long>(wall.ageMillis),
        static_cast<unsigned long>(wall.uncertaintyMillis), bootUnsynchronized,
        static_cast<unsigned long>(businessTime.requests), static_cast<unsigned long>(businessTime.samples),
        static_cast<unsigned long>(businessTime.serverSamples), static_cast<unsigned long>(businessTime.localSamples),
        currentPower, static_cast<unsigned long>(presses), static_cast<unsigned>(startError));
}
void wireSnapshot() {
    Serial.printf("B3_WIRE {\"sampleAt\":%lu,\"requestSequence\":%u,\"responseSequence\":%u,\"requestBodyBytes\":%u,\"responseFrameBytes\":%u,\"sourceUncertaintyMs\":%lu,\"correlated\":%u,\"errors\":%lu,\"rejected\":%lu,\"sendError\":%u}\n",
        static_cast<unsigned long>(businessTime.sampleAt), trace.requestSequence, trace.responseSequence,
        static_cast<unsigned>(trace.requestBodyBytes), static_cast<unsigned>(trace.responseFrameBytes),
        static_cast<unsigned long>(trace.sourceUncertaintyMs), trace.correlated,
        static_cast<unsigned long>(trace.errorResponses), static_cast<unsigned long>(trace.sendRejections), trace.sendError);
}
}
void setup() {
    Serial.begin(115200); Serial.setTxTimeoutMs(0U);
    power.onWrite(onPower); button.onInvoke(onButton);
    const blinker::Result result = product.begin(&businessTime);
    startError = result.code();
    uint64_t utc = 1U; bootUnsynchronized = !businessTime.time(utc) && utc == 0U;
    if (result) { power.report(false); buttonPresses.report(0U); }
}
void loop() {
    product.run();
    if (Serial.available() > 0) {
        const int command = Serial.read();
        if (command == 's') snapshot();
        if (command == 't') wireSnapshot();
    }
}
