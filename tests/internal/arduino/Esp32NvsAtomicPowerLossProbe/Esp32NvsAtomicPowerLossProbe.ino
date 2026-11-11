#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Storage.h>

#include <Arduino.h>

namespace {

const uint8_t kMagic[] = {'B', 'P', '1', 'E'};
const uint32_t kCommitIntervalMs = 250U;
const uint16_t kMaximumCommits = 120U;
const size_t kSequenceOffset = sizeof(kMagic);
const size_t kPayloadOffset = kSequenceOffset + sizeof(uint32_t);
const size_t kChecksumOffset = blinker::DeviceAccessStore::serializedSize - 4U;

blinker::Esp32NvsBlobStoreConfig makeConfig() {
    blinker::Esp32NvsBlobStoreConfig config;
    config.namespaceName = "bl_p1e_fault";
    config.key = "access";
    config.maximumSize = blinker::DeviceAccessStore::serializedSize;
    return config;
}

blinker::Esp32NvsAtomicBlobStore store(makeConfig());
uint8_t record[blinker::DeviceAccessStore::serializedSize] = {};
uint32_t sequence = 0U;
uint16_t commits = 0U;
uint32_t nextCommitMs = 0U;
bool armed = false;

void writeUint32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readUint32(const uint8_t* input) {
    return static_cast<uint32_t>(input[0]) << 24U |
           static_cast<uint32_t>(input[1]) << 16U |
           static_cast<uint32_t>(input[2]) << 8U |
           static_cast<uint32_t>(input[3]);
}

void makeRecord(uint32_t next) {
    memcpy(record, kMagic, sizeof(kMagic));
    writeUint32(record + kSequenceOffset, next);
    const uint8_t seed = (next & 1U) == 0U ? 0xA5U : 0x5AU;
    for (size_t index = kPayloadOffset; index < kChecksumOffset; ++index) {
        record[index] = static_cast<uint8_t>(seed ^ index ^ next);
    }
    writeUint32(
        record + kChecksumOffset,
        blinker::computeCrc32(blinker::ByteView(record, kChecksumOffset)));
}

bool validRecord(size_t size) {
    return size == sizeof(record) &&
           memcmp(record, kMagic, sizeof(kMagic)) == 0 &&
           readUint32(record + kChecksumOffset) ==
               blinker::computeCrc32(
                   blinker::ByteView(record, kChecksumOffset));
}

void fail(const __FlashStringHelper* message, blinker::ErrorCode error) {
    Serial.print(F("[fault-probe] FAIL "));
    Serial.print(message);
    Serial.print(F(" error="));
    Serial.println(static_cast<uint8_t>(error));
    while (true) delay(1000U);
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 3000U;
    while (!Serial && static_cast<int32_t>(millis() - serialDeadline) < 0) {
        delay(10U);
    }

    blinker::Result result = store.begin();
    if (!result) fail(F("open"), result.code());

    size_t size = 0U;
    result = store.load(blinker::MutableByteSpan(record, sizeof(record)), size);
    if (result) {
        if (!validRecord(size)) fail(F("torn-record"), blinker::ErrorCode::ProtocolError);
        sequence = readUint32(record + kSequenceOffset);
        Serial.print(F("[fault-probe] BOOT_VALID sequence="));
        Serial.println(sequence);
    } else if (result.code() == blinker::ErrorCode::NotFound) {
        makeRecord(sequence);
        result = store.replace(blinker::ByteView(record, sizeof(record)));
        if (!result) fail(F("seed"), result.code());
        Serial.println(F("[fault-probe] SEEDED sequence=0"));
    } else {
        fail(F("load"), result.code());
    }

    Serial.println(F("[fault-probe] Send 'a' to arm a 30-second commit window"));
}

void loop() {
    if (!armed) {
        if (Serial.available() == 0 || Serial.read() != 'a') {
            delay(10U);
            return;
        }
        armed = true;
        commits = 0U;
        nextCommitMs = millis();
        Serial.println(F("[fault-probe] ARMED: remove USB power now"));
    }

    if (commits >= kMaximumCommits) {
        Serial.println(F("[fault-probe] WINDOW_COMPLETE; send 'a' to repeat"));
        armed = false;
        return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextCommitMs) < 0) return;

    const uint32_t next = sequence + 1U;
    makeRecord(next);
    Serial.print(F("[fault-probe] COMMIT sequence="));
    Serial.println(next);
    const blinker::Result result =
        store.replace(blinker::ByteView(record, sizeof(record)));
    if (!result) fail(F("replace"), result.code());

    size_t size = 0U;
    const blinker::Result loaded =
        store.load(blinker::MutableByteSpan(record, sizeof(record)), size);
    if (!loaded) fail(F("verify-load"), loaded.code());
    if (!validRecord(size) || readUint32(record + kSequenceOffset) != next) {
        fail(F("verify-record"), blinker::ErrorCode::ProtocolError);
    }

    sequence = next;
    ++commits;
    nextCommitMs = now + kCommitIntervalMs;
}
