#include <BlinkerBLE.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <mbedtls/sha256.h>
#include <nvs.h>
#include "AccessWriteWitness.h"
#include "BleGapWitness.h"

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);

bool currentPower = false;
uint32_t pressCount = 0;

// Test-only, read-only durable-state witness. Never print credentials or dump
// NVS. Equal digests prove unchanged content, not a physical flash-write count.
void printAccessWitness() {
#if defined(BLINKER_TEST_NVS_WITNESS)
    printAccessWriteWitness();
#endif
    nvs_handle_t handle;
    esp_err_t result = nvs_open("bl_v2_access", NVS_READONLY, &handle);
    uint8_t bytes[blinker::DeviceAccessStore::serializedSize] = {};
    size_t size = sizeof(bytes);
    if (result == ESP_OK) {
        result = nvs_get_blob(handle, "root", bytes, &size);
        nvs_close(handle);
    }
    Serial.printf("accessWitness result=%d", static_cast<int>(result));
    if (result == ESP_OK) {
        uint8_t digest[32] = {};
        const int hashed = mbedtls_sha256(bytes, size, digest, 0);
        Serial.printf(" bytes=%u hashResult=%d sha256=", static_cast<unsigned int>(size), hashed);
        if (hashed == 0) {
            for (const uint8_t value : digest) Serial.printf("%02x", value);
        }
        blinker::secureZero(blinker::MutableByteSpan(digest, sizeof(digest)));
    }
    blinker::secureZero(blinker::MutableByteSpan(bytes, sizeof(bytes)));
    Serial.println();
}

void onPower(bool value) {
    currentPower = value;
    power.report(currentPower);
}

void onButton() {
    buttonPresses.report(++pressCount);
}

void setup() {
    Serial.begin(115200);
    delay(200U);
    power.onWrite(onPower);
    button.onInvoke(onButton);
    Blinker.debug(Serial, blinker::LogLevel::Info);
    const bool started = Blinker.begin(power, buttonPresses, button);
    Serial.print("begin=");
    Serial.println(started ? 1 : 0);
    Serial.println("send 'a' to clear Direct BLE access");
    Serial.println("send 's' for read-only access witness");
    Serial.println("send 'b' for read-only native BLE witness");
    if (started) {
        Blinker.report(power, currentPower, buttonPresses, pressCount);
    }
}

void loop() {
    Blinker.run();
    while (Serial.available() > 0) {
        const int command = Serial.read();
        if (command == 'b' || command == 'B') {
            printBleWitness();
            continue;
        }
        if (command == 's' || command == 'S') {
            printAccessWitness();
            continue;
        }
        if (command != 'a' && command != 'A') continue;
        const bool cleared = Blinker.resetAccess();
        Serial.print("resetAccess=");
        Serial.print(cleared ? 1 : 0);
        Serial.print(" error=");
        Serial.println(static_cast<unsigned int>(Blinker.lastError()));
    }
}
