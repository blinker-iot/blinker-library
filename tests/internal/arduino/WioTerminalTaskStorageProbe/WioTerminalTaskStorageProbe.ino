// Internal physical-USB storage admission only. No automatic I/O, task engine,
// networking, authority creation, software reboot, arbitrary payload or erase.
// Preserve the exact board's full 16 KiB raw region before compiling this
// matching maintenance profile. REGION_ATTEST is an explicit operator claim,
// not proof from blank bytes; all external QSPI file-system use must be absent.
#include <Arduino.h>
#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalTaskStorageProbe requires Wio Terminal"
#endif
#if !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1
#error "Wio storage probe requires an explicitly reserved maintenance profile"
#endif
#if !defined(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX)
#error "Wio storage probe requires the protected original 16 KiB baseline SHA"
#endif
#if !defined(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID)
#error "Wio storage probe requires its frozen source ID"
#endif
#include <BlinkerV2Advanced.h>
#include "TaskStorageProbePort.h"

constexpr bool wioProbeHex(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}
constexpr bool wioProbeDigest(const char* value, size_t offset = 0U) {
    return offset == 64U ? value[offset] == '\0' : wioProbeHex(value[offset]) && wioProbeDigest(value, offset + 1U);
}
static_assert(sizeof(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX) == 65U,
              "Baseline SHA must be exactly 64 hexadecimal characters");
static_assert(sizeof(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID) == 65U,
              "Frozen source ID must be exactly 64 hexadecimal characters");
static_assert(wioProbeDigest(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX), "Baseline SHA must be hexadecimal");
static_assert(wioProbeDigest(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID), "Frozen source ID must be hexadecimal");

namespace wio_task_probe {
TaskStorageProbePort port(wioTerminalQspiOwner(), millis, BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX);
ProbeController controller(port);

void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void hex(const uint8_t* bytes, size_t size) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0U; i < size; ++i) {
        Serial.print(digits[bytes[i] >> 4U]); Serial.print(digits[bytes[i] & 15U]);
    }
}
void boardUid() {
    // Exact word order/format used by Seeed SAMD 1.8.5 USB/USBCore.cpp
    // SERIAL_NUMBER_WORD_0..3 and utox8() for __SAMD51__; compare case-folded
    // with the running CDC USB iSerial, not another board's bootloader ID.
    static const uintptr_t addresses[4U] = {0x008061fcUL, 0x00806010UL, 0x00806014UL, 0x00806018UL};
    static const char digits[] = "0123456789abcdef";
    for (size_t word = 0U; word < 4U; ++word) {
        const uint32_t value = *reinterpret_cast<volatile const uint32_t*>(addresses[word]);
        for (int shift = 28; shift >= 0; shift -= 4) Serial.print(digits[(value >> shift) & 15U]);
    }
}
void profile() {
    Serial.print("{\"event\":\"profile\",\"fixture\":\"wio-task-storage-v1\",\"sourceId\":\"");
    Serial.print(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID); Serial.print("\",\"boardUid\":\"");
    boardUid(); Serial.print("\",\"baselineSha256\":\"");
    Serial.print(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX); Serial.print('"');
    number("capacity", TaskStorageProbePort::capacity); number("payloadBytes", TaskStorageProbePort::payloadSize);
    number("journalBytes", TaskStorageProbePort::journalSize); number("taskBase", TaskStorageProbePort::taskBase);
    number("identityBase", TaskStorageProbePort::identityBase); number("regionBytes", TaskStorageProbePort::regionBytes);
    number("regionReserved", 1U); Serial.println('}');
}
void done(const char* name, Result result) {
    Serial.print("{\"event\":\"done\",\"command\":\""); Serial.print(name); Serial.print('"');
    number("code", static_cast<unsigned>(result.code())); Serial.print(",\"state\":\"");
    Serial.print(stateName(controller.state())); Serial.print('"');
    number("attemptedMask", controller.attemptedMask()); Serial.println('}');
}
void evidence(bool snapshot) {
    if (snapshot && port.snapshotAvailable()) {
        Serial.print("{\"event\":\"snapshot\",\"bytes\":644,\"sha256\":\"");
        hex(port.snapshotDigest(), kSha256Size); Serial.print("\",\"fixturePhase\":1,\"identitySame\":");
        Serial.print(port.identitySame() ? "true" : "false"); Serial.println('}');
    }
}
bool commandId(const char* name, Command& output) {
    static const char* const names[] = {"PING", "OPEN_CAPTURE", "REGION_ATTEST", "READ", "REPLACE1",
                                      "IDENTITY_END", "CLEAR_FIXTURE", "STATUS"};
    for (uint8_t i = 0U; i < sizeof(names) / sizeof(names[0]); ++i)
        if (strcmp(name, names[i]) == 0) { output = static_cast<Command>(i); return true; }
    return false;
}
void command(const char* name) {
    Command id = Command::Status;
    if (!commandId(name, id) || Serial.available() != 0) {
        controller.malformedInput(); done("INVALID", Result::failure(ErrorCode::InvalidArgument)); return;
    }
    const uint32_t started = micros();
    const Result result = controller.execute(id);
    const uint32_t elapsed = micros() - started;
    if (id == Command::Ping) profile();
    if (id == Command::Replace1 || id == Command::ClearFixture) {
        Serial.print("{\"event\":\"operation\",\"command\":\""); Serial.print(name); Serial.print('"');
        number("code", static_cast<unsigned>(result.code())); number("us", elapsed); Serial.println('}');
    }
    if (result || (id == Command::Read && result.code() == ErrorCode::NotFound)) {
        if (id != Command::Ping && id != Command::Status) evidence(id == Command::Read ||
            id == Command::Replace1 || id == Command::IdentityEnd);
    }
    done(name, result);
}
} // namespace wio_task_probe

void setup() { Serial.begin(115200U); } // NO begin/read/write, and no boot-time command
void loop() {
    static char input[24U] = {}; static uint8_t length = 0U; static bool overflow = false;
    while (Serial.available()) {
        const int incoming = Serial.read(); if (incoming < 0) break;
        const char value = static_cast<char>(incoming); if (value == '\r') continue;
        if (value == '\n') {
            input[length] = '\0';
            if (!overflow && length != 0U) wio_task_probe::command(input);
            else if (overflow) {
                wio_task_probe::controller.malformedInput();
                wio_task_probe::done("INVALID", blinker::Result::failure(blinker::ErrorCode::InvalidArgument));
            }
            length = 0U; overflow = false;
        } else if (length + 1U < sizeof(input) &&
                   ((value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9') || value == '_'))
            input[length++] = value;
        else overflow = true;
    }
    delay(1U);
}
