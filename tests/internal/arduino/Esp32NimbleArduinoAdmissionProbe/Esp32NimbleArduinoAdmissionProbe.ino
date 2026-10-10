#include "../Esp32BleApiProbeCommon.h"

#include <NimBLEDevice.h>

namespace {

constexpr char kServiceUuid[] = "647d0001-9b35-4d2b-a0b2-c9b0e28f1368";
constexpr char kValueUuid[] = "647d0002-9b35-4d2b-a0b2-c9b0e28f1368";

} // namespace

void setup() {
    ble_api_probe::begin("NimBLE-Arduino-2.5.0");

    const bool initialized = NimBLEDevice::init("NIMBLE_AB");
    ble_api_probe::initialized(initialized);
    if (!initialized) return;

    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(kServiceUuid);
    NimBLECharacteristic* value = service->createCharacteristic(
        kValueUuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    value->setValue("ok");
    const bool serviceStarted = service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->setName("NIMBLE_AB");
    advertising->addServiceUUID(kServiceUuid);
    const bool advertisingStarted = serviceStarted && advertising->start();
    ble_api_probe::advertising(advertisingStarted);
}

void loop() {
    ble_api_probe::poll();
    delay(10U);
}
