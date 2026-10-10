#include "../Esp32BleApiProbeCommon.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace {

constexpr char kServiceUuid[] = "647d0011-9b35-4d2b-a0b2-c9b0e28f1368";
constexpr char kValueUuid[] = "647d0012-9b35-4d2b-a0b2-c9b0e28f1368";

} // namespace

void setup() {
    ble_api_probe::begin("Arduino-ESP32-SDK-BLE");

    const bool initialized = BLEDevice::init("SDK_BLE_AB");
    ble_api_probe::initialized(initialized);
    if (!initialized) return;

    BLEServer* server = BLEDevice::createServer();
    BLEService* service = server->createService(kServiceUuid);
    BLECharacteristic* value = service->createCharacteristic(
        kValueUuid,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    value->setValue("ok");
    const bool serviceStarted = service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(kServiceUuid);
    advertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    ble_api_probe::advertising(serviceStarted);
}

void loop() {
    ble_api_probe::poll();
    delay(10U);
}
