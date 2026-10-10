#include <BlinkerV2/arduino/ports/esp32/Esp32Security.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <esp_flash_encrypt.h>
#include <esp_partition.h>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "ESP32 security check failed at line " << __LINE__   \
                      << ": " #condition << std::endl;                        \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

bool gFlashEncryption = false;
bool gPartitionPresent = false;
esp_partition_t gPartition = {false};
uint32_t gFindCalls = 0U;
char gLastLabel[24] = {};

void resetState() {
    gFlashEncryption = false;
    gPartitionPresent = false;
    gPartition.encrypted = false;
    gFindCalls = 0U;
    std::memset(gLastLabel, 0, sizeof(gLastLabel));
}

void testFailClosedReasons() {
    resetState();
    CHECK_EQ(
        blinker::Esp32ProtectedNvsStatus::FlashEncryptionDisabled,
        blinker::esp32ProtectedNvsStatus("blinker_nvs"));
    CHECK_EQ(static_cast<uint32_t>(0U), gFindCalls);
    CHECK_TRUE(!blinker::esp32EncryptedNvsPartitionAvailable("blinker_nvs"));

    gFlashEncryption = true;
    CHECK_EQ(
        blinker::Esp32ProtectedNvsStatus::PartitionMissing,
        blinker::esp32ProtectedNvsStatus("blinker_nvs"));
    CHECK_TRUE(std::strcmp(gLastLabel, "blinker_nvs") == 0);

    gPartitionPresent = true;
    CHECK_EQ(
        blinker::Esp32ProtectedNvsStatus::PartitionNotEncrypted,
        blinker::esp32ProtectedNvsStatus("blinker_nvs"));
    CHECK_TRUE(!blinker::esp32EncryptedNvsPartitionAvailable("blinker_nvs"));
}

void testReadyAndDefaultLabel() {
    resetState();
    gFlashEncryption = true;
    gPartitionPresent = true;
    gPartition.encrypted = true;
    CHECK_EQ(
        blinker::Esp32ProtectedNvsStatus::Ready,
        blinker::esp32ProtectedNvsStatus("edge_nvs"));
    CHECK_TRUE(blinker::esp32EncryptedNvsPartitionAvailable("edge_nvs"));
    CHECK_TRUE(std::strcmp(gLastLabel, "edge_nvs") == 0);

    CHECK_EQ(
        blinker::Esp32ProtectedNvsStatus::Ready,
        blinker::esp32ProtectedNvsStatus(nullptr));
    CHECK_TRUE(std::strcmp(gLastLabel, "nvs") == 0);
    CHECK_EQ(
        blinker::Esp32ProtectedNvsStatus::Ready,
        blinker::esp32ProtectedNvsStatus(""));
    CHECK_TRUE(std::strcmp(gLastLabel, "nvs") == 0);
}

} // namespace

extern "C" bool esp_flash_encryption_enabled(void) {
    return gFlashEncryption;
}

extern "C" const esp_partition_t* esp_partition_find_first(
    esp_partition_type_t type,
    esp_partition_subtype_t subtype,
    const char* label) {
    ++gFindCalls;
    CHECK_EQ(ESP_PARTITION_TYPE_DATA, type);
    CHECK_EQ(ESP_PARTITION_SUBTYPE_DATA_NVS, subtype);
    const size_t length = std::strlen(label);
    CHECK_TRUE(length < sizeof(gLastLabel));
    std::memcpy(gLastLabel, label, length + 1U);
    return gPartitionPresent ? &gPartition : nullptr;
}

int main() {
    testFailClosedReasons();
    testReadyAndDefaultLabel();
    std::cout << "ESP32 protected NVS attestation tests passed" << std::endl;
    return 0;
}
