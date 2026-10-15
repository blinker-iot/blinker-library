// Internal, physical USB-only temporary-identity gate. Never a user example.
// No automatic read, migration, network, executor, reset or retry at boot.
#include "GateConfig.h"
#include <Arduino.h>
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/arduino/HardwareRandom.h>
#include "TransitionStorage.h"
#include "SerialAdapter.h"

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "Wio identity transition gate requires Wio Terminal"
#endif
#if !defined(GATE_COMPILE_ONLY) || (GATE_COMPILE_ONLY != 0 && GATE_COMPILE_ONLY != 1)
#error "An explicit compile-only or actual profile is required"
#endif
#if !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1
#error "Wio identity transition requires explicit fixed-tail reservation"
#endif
#if !defined(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID) || \
    !defined(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX) || !defined(BLINKER_INTERNAL_WIO_BOARD_UID_HEX)
#error "Wio identity transition requires frozen source, original raw digest and physical UID"
#endif

constexpr bool wioTransitionHex(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}
constexpr bool wioTransitionHexValue(const char* value, size_t size, size_t offset = 0U) {
    return offset == size ? value[offset] == '\0' :
        wioTransitionHex(value[offset]) && wioTransitionHexValue(value, size, offset + 1U);
}
static_assert(sizeof(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID) == 65U &&
    wioTransitionHexValue(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID, 64U), "Source must be exact SHA256 hex");
static_assert(sizeof(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX) == 65U &&
    wioTransitionHexValue(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX, 64U), "Original raw must be exact SHA256 hex");
static_assert(sizeof(BLINKER_INTERNAL_WIO_BOARD_UID_HEX) == 33U &&
    wioTransitionHexValue(BLINKER_INTERNAL_WIO_BOARD_UID_HEX, 32U), "Physical UID must be exact 32-character hex");
static_assert(wio_identity_transition::SerialAdapter::rawSize == blinker::WioTerminalQspiOwner::reservedSize,
    "Serial maintenance buffer must match the fixed reserved tail");

namespace {
using namespace wio_identity_transition;
char actualUid[33U] = {};
const TransitionBinding storageBinding = {BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID,
    BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX, BLINKER_INTERNAL_WIO_BOARD_UID_HEX};
blinker::PlatformHardwareRandom hardwareRandom;
TransitionStorage storage(blinker::wioTerminalQspiOwner(), millis, hardwareRandom, storageBinding, actualUid);
TransitionController controller(storage);
class UsbTransport final : public SerialTransport {
public:
    uint32_t now() override { return millis(); }
    int available() override { return Serial.available(); }
    int read() override { return Serial.read(); }
    int writable() override { return Serial.availableForWrite(); }
    size_t write(const uint8_t* bytes, size_t size) override { return Serial.write(bytes, size); }
    void idle() override { delay(1U); }
} usb;
class StorageReadback final : public TransitionReadback {
public:
    bool readUid(uint8_t* output) override {
        // SAMD51 word order matches installed Seeed 1.8.5 CDC iSerial.
        static const uintptr_t addresses[4U] = {0x008061fcUL, 0x00806010UL, 0x00806014UL, 0x00806018UL};
        static const char digits[] = "0123456789abcdef";
        for (size_t word = 0U; word < 4U; ++word) {
            const uint32_t value = *reinterpret_cast<volatile const uint32_t*>(addresses[word]);
            for (size_t byte = 0U; byte < 4U; ++byte)
                output[word * 4U + byte] = static_cast<uint8_t>(value >> ((3U - byte) * 8U));
            for (size_t digit = 0U; digit < 8U; ++digit)
                actualUid[word * 8U + digit] = digits[(value >> ((7U - digit) * 4U)) & 15U];
        }
        actualUid[32U] = '\0'; return true;
    }
    blinker::ByteView capturedRaw() const override { return storage.capturedRaw(); }
    blinker::ByteView temporaryInstance() const override {
        return blinker::ByteView(storage.temporaryInstance().bytes, sizeof(storage.temporaryInstance().bytes));
    }
} readback;
const SerialBinding serialBinding = {blinker::WioTerminalQspiOwner::expectedCapacity,
    blinker::WioTerminalQspiOwner::taskAddress, blinker::WioTerminalQspiOwner::identityAddress,
    blinker::WioTerminalQspiOwner::regionSize, BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID,
    BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX, GATE_COMPILE_ONLY == 0};
SerialAdapter adapter(controller, readback, usb, serialBinding);
}
void setup() { Serial.begin(115200U); } // No QSPI or UID reads until an explicit command.
void loop() { adapter.service(); delay(1U); }
