#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/config/EdgeHubConfig.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Security.h>

namespace {

const __FlashStringHelper* statusText(
    blinker::Esp32ProtectedNvsStatus status) {
    switch (status) {
        case blinker::Esp32ProtectedNvsStatus::Ready:
            return F("ready");
        case blinker::Esp32ProtectedNvsStatus::FlashEncryptionDisabled:
            return F("flash_encryption_disabled");
        case blinker::Esp32ProtectedNvsStatus::PartitionMissing:
            return F("partition_missing");
        case blinker::Esp32ProtectedNvsStatus::PartitionNotEncrypted:
            return F("partition_not_encrypted");
    }
    return F("unknown");
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t deadline = millis() + 3000U;
    while (!Serial && static_cast<int32_t>(millis() - deadline) < 0) {
        delay(10U);
    }

    const blinker::Esp32ProtectedNvsStatus status =
        blinker::esp32ProtectedNvsStatus(
            blinker::official::edgeHubNvsPartition);
    Serial.println(F("edge_hub_security_preflight_v1"));
    Serial.print(F("partition="));
    Serial.println(blinker::official::edgeHubNvsPartition);
    Serial.print(F("status="));
    Serial.println(statusText(status));
    Serial.print(F("ready="));
    Serial.println(
        status == blinker::Esp32ProtectedNvsStatus::Ready ? 1 : 0);
    Serial.println(F("readonly=1"));
}

void loop() {}
