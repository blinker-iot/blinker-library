#include <Arduino.h>
#include <BLEDevice.h>

namespace {

constexpr char kServiceUuid[] =
    "5f6d0001-3f5b-4e4f-9f4d-626c696e6b32";
constexpr char kTransmitUuid[] =
    "5f6d0003-3f5b-4e4f-9f4d-626c696e6b32";
constexpr uint8_t kCycles = 3U;

bool findTarget(BLEAdvertisedDevice& target) {
    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    scan->setInterval(45U);
    scan->setWindow(30U);
    BLEScanResults* results = scan->start(5U, false);
    if (results == nullptr) return false;

    bool found = false;
    for (int index = 0; index < results->getCount(); ++index) {
        BLEAdvertisedDevice candidate = results->getDevice(index);
        if (!candidate.isAdvertisingService(BLEUUID(kServiceUuid))) continue;
        target = candidate;
        found = true;
        break;
    }
    return found;
}

bool runCycle(uint8_t cycle) {
    static BLEClient* client = BLEDevice::createClient();
    BLEAdvertisedDevice target;
    const uint32_t scanStartedMs = millis();
    if (!findTarget(target)) {
        Serial.printf("[client] cycle=%u scan=not-found\n", cycle);
        BLEDevice::getScan()->clearResults();
        return false;
    }
    Serial.printf(
        "[client] cycle=%u scanMs=%u name=%s rssi=%d\n",
        cycle,
        millis() - scanStartedMs,
        target.haveName() ? target.getName().c_str() : "",
        target.getRSSI());

    const uint32_t connectStartedMs = millis();
    if (client == nullptr || !client->connectTimeout(&target, 5000U)) {
        Serial.printf("[client] cycle=%u connect=failed\n", cycle);
        BLEDevice::getScan()->clearResults();
        return false;
    }

    BLERemoteService* service = client->getService(kServiceUuid);
    BLERemoteCharacteristic* transmit = service == nullptr
        ? nullptr
        : service->getCharacteristic(kTransmitUuid);
    const bool subscribed = transmit != nullptr && transmit->canNotify() &&
                            transmit->subscribe(true, nullptr, true);
    Serial.printf(
        "[client] cycle=%u connectMs=%u mtu=%u service=%u notify=%u\n",
        cycle,
        millis() - connectStartedMs,
        client->getMTU(),
        service != nullptr ? 1U : 0U,
        subscribed ? 1U : 0U);

    delay(1000U);
    client->disconnect();
    BLEDevice::getScan()->clearResults();
    delay(1000U);
    return service != nullptr && subscribed;
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 3000U;
    while (!Serial && static_cast<int32_t>(millis() - serialDeadline) < 0) {
        delay(10U);
    }

    Serial.println("[client] ESP32 BLE GATT regression");
    if (!BLEDevice::init("BLINKER_GATT_TEST")) {
        Serial.println("[client] init=failed");
        return;
    }

    uint8_t passed = 0U;
    for (uint8_t cycle = 1U; cycle <= kCycles; ++cycle) {
        if (runCycle(cycle)) ++passed;
    }
    Serial.printf(
        "[client] result=%s passed=%u/%u\n",
        passed == kCycles ? "PASS" : "FAIL",
        passed,
        kCycles);
}

void loop() {
    delay(1000U);
}
