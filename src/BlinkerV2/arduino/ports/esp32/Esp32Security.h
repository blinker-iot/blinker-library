#ifndef BLINKER_V2_ARDUINO_PORTS_ESP32_SECURITY_H
#define BLINKER_V2_ARDUINO_PORTS_ESP32_SECURITY_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32Security requires an ESP32 Arduino target"
#endif

#include <stdint.h>

namespace blinker {

enum class Esp32ProtectedNvsStatus : uint8_t {
    Ready = 0U,
    FlashEncryptionDisabled,
    PartitionMissing,
    PartitionNotEncrypted
};

// Read-only attestation. This function never initializes encryption or writes
// eFuse state. The exact NVS partition must both be marked encrypted and run
// under active ESP32 Flash Encryption.
Esp32ProtectedNvsStatus esp32ProtectedNvsStatus(
    const char* partitionLabel);
bool esp32EncryptedNvsPartitionAvailable(const char* partitionLabel);

} // namespace blinker

#endif
