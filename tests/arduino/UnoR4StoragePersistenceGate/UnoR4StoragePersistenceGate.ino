#include <Arduino.h>
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4Storage.h>

#include <DeviceAccessMigrationFixture.h>

#ifndef BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY
#define BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY 0
#endif

namespace {

typedef blinker_test::DeviceAccessMigrationFixture Fixture;

blinker::RenesasUnoStorageConfig storageConfig() {
    blinker::RenesasUnoStorageConfig config;
    config.namespaceName = "bl_g2c_gate";
    return config;
}

blinker::RenesasUnoPreferencesBlobBank storage(storageConfig());
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

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t waitUntil = millis() + 15000U;
    while (!Serial && static_cast<int32_t>(millis() - waitUntil) < 0) {}
    Serial.println("G2C_UNO_R4_GATE_BOOT");

    blinker::Result result = storage.begin();
    if (!result) {
        printResult("BEGIN_FAILED", result);
        return;
    }

    blinker::IAtomicBlobStore& blob = storage.deviceAccessBlob();
    uint8_t raw[blinker::DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    result = blob.load(blinker::MutableByteSpan(raw, sizeof(raw)), written);
    blinker::secureZero(blinker::MutableByteSpan(raw, sizeof(raw)));
    if (result.code() == blinker::ErrorCode::NotFound) {
#if BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY
        Serial.println("ACCESS_RECORD_EMPTY");
        statusLine = "ACCESS_RECORD_EMPTY";
        return;
#else
        result = Fixture::seedPreviousSchema(blob);
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

    blinker::DeviceAccessStore access(blob);
    if (written == blinker::DeviceAccessStore::previousSerializedSize) {
        if (!Fixture::matches(access, 1U)) {
            Serial.println("ACCESS_RECORD_CONFLICT_NOT_MODIFIED");
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
        return;
    }

    if (written != blinker::DeviceAccessStore::serializedSize ||
        !Fixture::matches(access, 3U)) {
        Serial.println("ACCESS_RECORD_CONFLICT_NOT_MODIFIED");
        return;
    }
#if BLINKER_G2C_CLEAR_FIXTURE_AFTER_VERIFY
    result = blob.clear();
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
