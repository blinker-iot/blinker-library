#if defined(ARDUINO_ARCH_ESP32)

#include "Esp32Security.h"

#include <esp_flash_encrypt.h>
#include <esp_partition.h>

namespace blinker {

Esp32ProtectedNvsStatus esp32ProtectedNvsStatus(
    const char* partitionLabel) {
    const char* label = partitionLabel == nullptr || partitionLabel[0] == '\0'
                            ? "nvs"
                            : partitionLabel;
    if (!esp_flash_encryption_enabled()) {
        return Esp32ProtectedNvsStatus::FlashEncryptionDisabled;
    }
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_NVS,
        label);
    if (partition == nullptr) {
        return Esp32ProtectedNvsStatus::PartitionMissing;
    }
    return partition->encrypted
               ? Esp32ProtectedNvsStatus::Ready
               : Esp32ProtectedNvsStatus::PartitionNotEncrypted;
}

bool esp32EncryptedNvsPartitionAvailable(const char* partitionLabel) {
    return esp32ProtectedNvsStatus(partitionLabel) ==
           Esp32ProtectedNvsStatus::Ready;
}

} // namespace blinker

#endif // ARDUINO_ARCH_ESP32
