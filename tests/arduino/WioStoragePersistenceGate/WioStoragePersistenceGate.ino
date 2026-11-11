#include <Arduino.h>
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.h>

#include <DeviceAccessMigrationFixture.h>

#ifndef BLINKER_G2C_ERASE_EXISTING_ACCESS
#define BLINKER_G2C_ERASE_EXISTING_ACCESS 0
#endif

#ifndef BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY
#define BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY 0
#endif

namespace {

typedef blinker_test::DeviceAccessMigrationFixture Fixture;

blinker::WioTerminalFlashBlobBank storage;
const char* statusLine = nullptr;

void printResult(const char* label, blinker::Result result) {
    Serial.print(label);
    Serial.print(" code=");
    Serial.println(static_cast<unsigned int>(result.code()));
}

void restart() {
    Serial.flush();
    delay(250U);
    NVIC_SystemReset();
}

void handleAccessConflict() {
#if BLINKER_G2C_ERASE_EXISTING_ACCESS
    const blinker::Result result = storage.deviceAccessBlob().clear();
    if (!result) {
        printResult("ACCESS_CLEAR_FAILED", result);
        return;
    }
    Serial.println("EXISTING_ACCESS_CLEARED");
    restart();
#else
    Serial.println("ACCESS_RECORD_CONFLICT_NOT_MODIFIED");
#endif
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t waitUntil = millis() + 15000U;
    while (!Serial && static_cast<int32_t>(millis() - waitUntil) < 0) {}
    Serial.println("G2C_GATE_BOOT");
    Serial.flush();

    blinker::Result result = storage.begin();
    if (!result) {
        printResult("BEGIN_FAILED", result);
        return;
    }

    uint8_t raw[blinker::DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    result = storage.deviceAccessBlob().load(
        blinker::MutableByteSpan(raw, sizeof(raw)), written);
    blinker::secureZero(blinker::MutableByteSpan(raw, sizeof(raw)));
    if (result.code() == blinker::ErrorCode::NotFound) {
#if BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY
        Serial.println("ACCESS_RECORD_EMPTY");
        statusLine = "ACCESS_RECORD_EMPTY";
        return;
#else
        result = Fixture::seedPreviousSchema(storage.deviceAccessBlob());
        if (!result) {
            printResult("SEED_V2_FAILED", result);
            return;
        }
        Serial.println("SEEDED_V2_SOFTWARE_RESET");
        restart();
        return;
#endif
    }
    if (!result) {
        printResult("LOAD_FAILED", result);
        return;
    }

    blinker::DeviceAccessStore access(storage.deviceAccessBlob());
    if (written == blinker::DeviceAccessStore::previousSerializedSize) {
        if (!Fixture::matches(access, 1U)) {
            handleAccessConflict();
            return;
        }
        result = access.installVerified(Fixture::sharedCredential());
        if (result) {
            result = access.installVerified(Fixture::gatewayCredential());
        }
        if (!result || !Fixture::matches(access, 3U)) {
            printResult("MIGRATE_V3_FAILED", result);
            return;
        }
        Serial.println("MIGRATED_V3_POWER_OFF_NOW");
        Serial.flush();
        return;
    }

    if (written != blinker::DeviceAccessStore::serializedSize ||
        !Fixture::matches(access, 3U)) {
        handleAccessConflict();
        return;
    }
#if BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY
    result = storage.deviceAccessBlob().clear();
    if (!result) {
        printResult("FIXTURE_CLEAR_FAILED", result);
        return;
    }
    Serial.println("TEST_FIXTURE_CLEARED");
    restart();
    return;
#else
    Serial.println("V3_POWER_CYCLE_OK");
    statusLine = "V3_POWER_CYCLE_OK";
#endif
}

void loop() {
    if (statusLine == nullptr) return;
    delay(1000U);
    Serial.println(statusLine);
}
