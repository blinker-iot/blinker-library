// Internal read-only cold observation. The host must retain the ORIGINAL raw
// backup and unknown write intent, prove a real whole-board power cycle, and
// reconcile two current raw reads before preparing this exact bound profile.
// No executor, networking, automatic storage I/O, write, clear or reset.
#include <Arduino.h>
#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalTaskColdRecovery requires Wio Terminal"
#endif
#if !defined(BLINKER_INTERNAL_WIO_COLD_RECOVERY) || BLINKER_INTERNAL_WIO_COLD_RECOVERY != 1
#error "Wio cold recovery requires its read-only maintenance profile"
#endif
#if !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1
#error "Wio cold recovery requires the original reserved region declaration"
#endif
#ifndef BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID
#error "Wio cold recovery requires frozen source ID"
#endif
#ifndef BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX
#error "Wio cold recovery requires ORIGINAL baseline SHA"
#endif
#ifndef BLINKER_INTERNAL_WIO_IDENTITY_SHA256_HEX
#error "Wio cold recovery requires original identity SHA"
#endif
#ifndef BLINKER_INTERNAL_WIO_COLD_CURRENT_SHA256_HEX
#error "Wio cold recovery requires independently reconciled cold-current SHA"
#endif
#ifndef BLINKER_INTERNAL_WIO_INTENT_SHA256_HEX
#error "Wio cold recovery requires original write intent SHA"
#endif
#ifndef BLINKER_INTERNAL_WIO_BOARD_UID_HEX
#error "Wio cold recovery requires expected physical board UID"
#endif
#include <BlinkerV2Advanced.h>
#include "ColdRecoveryPort.h"

constexpr bool wioColdHex(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}
constexpr bool wioColdHexValue(const char* value, size_t size, size_t offset = 0U) {
    return offset == size ? value[offset] == '\0' : wioColdHex(value[offset]) && wioColdHexValue(value, size, offset + 1U);
}
#define BLINKER_COLD_DIGEST_CHECK(value) \
    static_assert(sizeof(value) == 65U, "Wio cold digest must contain exactly 64 hexadecimal characters"); \
    static_assert(wioColdHexValue(value, 64U), "Wio cold digest must be hexadecimal")
BLINKER_COLD_DIGEST_CHECK(BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID);
BLINKER_COLD_DIGEST_CHECK(BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX);
BLINKER_COLD_DIGEST_CHECK(BLINKER_INTERNAL_WIO_IDENTITY_SHA256_HEX);
BLINKER_COLD_DIGEST_CHECK(BLINKER_INTERNAL_WIO_COLD_CURRENT_SHA256_HEX);
BLINKER_COLD_DIGEST_CHECK(BLINKER_INTERNAL_WIO_INTENT_SHA256_HEX);
#undef BLINKER_COLD_DIGEST_CHECK
static_assert(sizeof(BLINKER_INTERNAL_WIO_BOARD_UID_HEX) == 33U, "Wio cold UID must contain exactly 32 hexadecimal characters");
static_assert(wioColdHexValue(BLINKER_INTERNAL_WIO_BOARD_UID_HEX, 32U), "Wio cold UID must be hexadecimal");

namespace wio_task_cold {
static char actualUid[33U] = {};
static const ColdRecoveryBinding binding = {
    BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID,
    BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX,
    BLINKER_INTERNAL_WIO_IDENTITY_SHA256_HEX,
    BLINKER_INTERNAL_WIO_COLD_CURRENT_SHA256_HEX,
    BLINKER_INTERNAL_WIO_INTENT_SHA256_HEX,
    BLINKER_INTERNAL_WIO_BOARD_UID_HEX
};
ColdRecoveryPort port(wioTerminalQspiOwner(), millis, binding, actualUid);
ColdRecoveryController controller(port);

void uidMetadata() {
    // Read only on an explicit PING/OBSERVE. Exact SAMD51 word order from the
    // installed Seeed 1.8.5 USB/USBCore.cpp: this equals running CDC iSerial
    // case-insensitively, not an assumed MAC or another board's bootloader ID.
    static const uintptr_t addresses[4U] = {0x008061fcUL, 0x00806010UL, 0x00806014UL, 0x00806018UL};
    static const char digits[] = "0123456789abcdef";
    for (size_t word = 0U; word < 4U; ++word) {
        const uint32_t value = *reinterpret_cast<volatile const uint32_t*>(addresses[word]);
        for (size_t digit = 0U; digit < 8U; ++digit)
            actualUid[word * 8U + digit] = digits[(value >> ((7U - digit) * 4U)) & 15U];
    }
    actualUid[32U] = '\0';
}
void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void field(const char* key, const char* value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":\""); Serial.print(value); Serial.print('"');
}
void profile() {
    Serial.print("{\"event\":\"cold-profile\",\"fixture\":\"wio-task-cold-v1\"");
    field("sourceId", binding.sourceId); field("boardUid", actualUid);
    field("baselineSha256", binding.baselineSha256); field("identitySha256", binding.identitySha256);
    field("currentSha256", binding.currentSha256); field("intentSha256", binding.intentSha256);
    number("capacity", ColdRecoveryPort::capacity); number("payloadBytes", ColdRecoveryPort::payloadSize);
    number("taskBase", ColdRecoveryPort::taskBase); number("identityBase", ColdRecoveryPort::identityBase);
    number("regionBytes", ColdRecoveryPort::regionBytes); Serial.println('}');
}
void done(const char* name, Result result) {
    Serial.print("{\"event\":\"cold-done\",\"command\":\""); Serial.print(name); Serial.print('"');
    number("code", static_cast<unsigned>(result.code())); field("state", stateName(controller.state())); Serial.println('}');
}
void observation() {
    static const char digits[] = "0123456789abcdef";
    Serial.print("{\"event\":\"cold-observation\""); number("bytes", port.hasSnapshot() ? ColdRecoveryPort::payloadSize : 0U);
    Serial.print(",\"sha256\":\"");
    if (port.hasSnapshot()) for (size_t i = 0U; i < kSha256Size; ++i) {
        Serial.print(digits[port.payloadDigest()[i] >> 4U]); Serial.print(digits[port.payloadDigest()[i] & 15U]);
    }
    Serial.print('"'); number("records", port.hasSnapshot() ? ColdRecoveryPort::capacity : 0U);
    Serial.print(",\"identitySame\":"); Serial.print(port.identitySame() ? "true" : "false");
    Serial.println(",\"writeConfirmed\":false}");
}
void command(const char* name) {
    Command id = Command::Status;
    if (strcmp(name, "PING") == 0) id = Command::Ping;
    else if (strcmp(name, "OBSERVE") == 0) id = Command::Observe;
    else if (strcmp(name, "STATUS") != 0) {
        controller.malformedInput(); done("INVALID", Result::failure(ErrorCode::InvalidArgument)); return;
    }
    if (Serial.available() != 0) {
        controller.malformedInput(); done("INVALID", Result::failure(ErrorCode::InvalidArgument)); return;
    }
    if (id != Command::Status) uidMetadata(); // public MCU metadata only, no QSPI
    const Result result = controller.execute(id);
    if (id == Command::Ping) profile();
    if (id == Command::Observe && port.observed() &&
        (result || result.code() == ErrorCode::NotFound)) observation();
    done(name, result);
}
} // namespace wio_task_cold

void setup() { Serial.begin(115200U); } // No QSPI, UID, bank/store begin or boot-time observation.
void loop() {
    static char input[16U] = {}; static uint8_t length = 0U; static bool overflow = false;
    while (Serial.available()) {
        const int incoming = Serial.read(); if (incoming < 0) break;
        const char value = static_cast<char>(incoming); if (value == '\r') continue;
        if (value == '\n') {
            input[length] = '\0';
            if (!overflow && length != 0U) wio_task_cold::command(input);
            else if (overflow) {
                wio_task_cold::controller.malformedInput();
                wio_task_cold::done("INVALID", blinker::Result::failure(blinker::ErrorCode::InvalidArgument));
            }
            length = 0U; overflow = false;
        } else if (length + 1U < sizeof(input) && value >= 'A' && value <= 'Z') input[length++] = value;
        else overflow = true;
    }
    delay(1U);
}
