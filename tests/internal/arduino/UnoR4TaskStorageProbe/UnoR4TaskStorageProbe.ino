// Internal physical-USB storage admission probe, NOT a task product or API.
// No network/product/executor, auto-created identity, automatic writes or erase.
// Main-MCU backup does NOT protect the connectivity module Preferences.
// Before use, preserve the exact board's twelve raw/absent keys with the existing
// maintenance gate. This probe never implements backup/restore or namespace clear.
// Commands (one line at a time): PING, OPEN, READ, RAM, REPLACE1, REPLACE2,
// CLEAR_FIXTURE, REBOOT. First write requires BOTH raw task keys absent.
// Each write step is attempted once per boot. Any failure locks writes; after an
// unconfirmed operation only reconcile/read after real reboot, never replay it.
#include <Arduino.h>
#if !defined(ARDUINO_UNOR4_WIFI)
#error "UnoR4TaskStorageProbe requires the physical UNO R4 WiFi target"
#endif
// Arduino's dependency discovery resolves libraries by their public root header.
// Deep internal includes alone do not select/compile the standard library.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4TaskStorage.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/core/SecureMemory.h>
#include "../PlatformStorageBackupGate/R4StorageGate.h"
#include "TaskAllocationObserver.h"
#include "TaskSlotShape.h"
#include <malloc.h>
#include <string.h>

extern "C" uint8_t __HeapBase, __HeapLimit, __StackLimit, __StackTop;

namespace task_probe {
using namespace blinker;
enum : size_t { capacity = 4U, payloadSize = 644U, journalSize = 664U };
static_assert(TaskSnapshotCodec::headerSize + capacity * TaskSnapshotCodec::recordSize +
              TaskSnapshotCodec::trailerSize == payloadSize, "Snapshot schema changed");
static_assert(RenesasUnoTaskSnapshotBlobStore<capacity>::journalScratchSize == journalSize,
              "Journal size changed");
RenesasUnoPreferencesBlobBank bank;
RenesasUnoTaskSnapshotBlobStore<capacity> tasks(bank);
DeviceInstanceId instance;
bool openAttempted = false, ready = false, writesLocked = false;
uint8_t attemptedWrites = 0U;
uint8_t expected[payloadSize] = {}, loaded[payloadSize] = {}, identityScratch[260U] = {};
TaskSnapshotHeader header;
TaskSnapshotRecord records[capacity];

struct Fingerprints {
    ErrorCode codes[5];
    size_t sizes[5];
    uint8_t hashes[5][kSha256Size];
    Fingerprints() : codes(), sizes(), hashes() {}
} identityBefore, identityCurrent;

void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void signedNumber(const char* key, long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void hex(const uint8_t* bytes, size_t size) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0U; i < size; ++i) {
        Serial.print(digits[bytes[i] >> 4U]); Serial.print(digits[bytes[i] & 15U]);
    }
}
void done(const char* command, Result result) {
    Serial.print("{\"event\":\"done\",\"command\":\""); Serial.print(command); Serial.print('"');
    number("code", static_cast<unsigned>(result.code())); number("ready", ready);
    number("writesLocked", writesLocked); number("attemptedMask", attemptedWrites); Serial.println('}');
}
void ram() {
    const uintptr_t msp = currentMsp();
    const uintptr_t heapBreak = reinterpret_cast<uintptr_t>(_sbrk(0));
    const bool breakValid = heapBreak != UINTPTR_MAX && heapBreak != 0U;
    const struct mallinfo info = mallinfo();
    Serial.print("{\"event\":\"ram_observation\",\"peakMeasured\":false");
    number("msp", msp); number("heapBreak", heapBreak);
    number("heapBreakValid", breakValid);
    signedNumber("currentGapBytes", breakValid ? static_cast<int32_t>(msp - heapBreak) : 0);
    number("heapBase", reinterpret_cast<uintptr_t>(&__HeapBase));
    number("heapLimit", reinterpret_cast<uintptr_t>(&__HeapLimit));
    number("stackLimit", reinterpret_cast<uintptr_t>(&__StackLimit));
    number("stackTop", reinterpret_cast<uintptr_t>(&__StackTop));
    signedNumber("mallinfoArena", info.arena); signedNumber("mallinfoUsed", info.uordblks);
    signedNumber("mallinfoFree", info.fordblks); number("taskStoreObjectBytes", sizeof(tasks));
    Serial.println('}');
}
void measured(const char* operation, Result result, uint32_t elapsed, size_t bytes) {
    Serial.print("{\"event\":\"operation\",\"operation\":\""); Serial.print(operation); Serial.print('"');
    number("code", static_cast<unsigned>(result.code())); number("us", elapsed); number("bytes", bytes);
    number("allocationObserver", BLINKER_INTERNAL_TASK_ALLOCATION_OBSERVER);
    number("boundarySamples", allocation.samples); number("mallocCalls", allocation.mallocCalls);
    number("reallocCalls", allocation.reallocCalls); number("freeCalls", allocation.freeCalls);
    number("invalidBreakSamples", allocation.invalidBreakSamples);
    number("gapValid", allocation.samples != 0U && allocation.invalidBreakSamples == 0U);
    number("minObservedMsp", allocation.minimumMsp); signedNumber("minObservedGapBytes", allocation.minimumGap);
    Serial.println(",\"fullPeakMeasured\":false}");
}

Result captureIdentity(Fingerprints& output) {
    // Avoid a 188-byte temporary on the nominal 1 KiB R4 main stack.
    for (size_t i = 0U; i < 5U; ++i) { output.codes[i] = ErrorCode::Ok; output.sizes[i] = 0U; }
    memset(output.hashes, 0, sizeof(output.hashes));
    IAtomicBlobStore* blobs[5] = {&bank.wifiSelectorBlob(), &bank.wifiSlot0Blob(),
        &bank.wifiSlot1Blob(), &bank.deviceInstanceBlob(), &bank.deviceAccessBlob()};
    for (size_t i = 0U; i < 5U; ++i) {
        size_t written = 0U;
        Result result = blobs[i]->load(MutableByteSpan(identityScratch, sizeof(identityScratch)), written);
        output.codes[i] = result.code(); output.sizes[i] = written;
        if (result) result = sha256(ByteView(identityScratch, written), MutableByteSpan(output.hashes[i], kSha256Size));
        secureZero(MutableByteSpan(identityScratch, sizeof(identityScratch)));
        if (!result && result.code() != ErrorCode::NotFound) return result;
    }
    return Result::success();
}
bool sameIdentity() {
    for (size_t i = 0U; i < 5U; ++i)
        if (identityBefore.codes[i] != identityCurrent.codes[i] || identityBefore.sizes[i] != identityCurrent.sizes[i] ||
            memcmp(identityBefore.hashes[i], identityCurrent.hashes[i], kSha256Size) != 0) return false;
    return true;
}
void identityReport(const char* phase) {
    Serial.print("{\"event\":\"identity\",\"phase\":\""); Serial.print(phase);
    Serial.print("\",\"instance\":\""); hex(instance.bytes, sizeof(instance.bytes)); Serial.print('"');
    number("matchesOpenBaseline", sameIdentity()); Serial.println('}');
    for (size_t i = 0U; i < 5U; ++i) {
        Serial.print("{\"event\":\"identity_blob\""); number("index", i);
        number("code", static_cast<unsigned>(identityCurrent.codes[i])); number("bytes", identityCurrent.sizes[i]);
        Serial.print(",\"sha256\":\""); hex(identityCurrent.hashes[i], kSha256Size); Serial.println("\"}");
    }
}
Result checkIdentity(const char* phase) {
    Result result = captureIdentity(identityCurrent);
    if (result) identityReport(phase);
    return result && !sameIdentity() ? Result::failure(ErrorCode::StateConflict) : result;
}

// Reuse the maintenance gate's confirmed RPC parser, using the ONE bank owner.
// No Preferences object/begin/end or copied journal implementation here.
Result taskMetadata(bool (&present)[2], uint32_t (&sizes)[2]) {
    static const char* const keys[2] = {"ts0", "ts1"};
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    RenesasUnoModemOwner::Operation operation(owner, &bank, 1000U);
    if (!operation.result()) return operation.result();
    for (size_t i = 0U; i < 2U; ++i) {
        uint32_t type = 0U; present[i] = false; sizes[i] = 0U;
        Result ready = owner.prepare();
        if (!ready) return ready;
        ready = owner.complete(storage_gate::port::confirmedType(keys[i], type));
        if (!ready) return ready;
        if (type == PT_INVALID) continue;
        present[i] = true;
        if (type != PT_BLOB) return Result::failure(ErrorCode::InvalidEncoding);
        ready = owner.prepare();
        if (!ready) return ready;
        ready = owner.complete(storage_gate::port::confirmedLength(keys[i], journalSize, sizes[i]));
        if (!ready) return ready;
    }
    Serial.print("{\"event\":\"task_keys\""); number("ts0Present", present[0]); number("ts0Bytes", sizes[0]);
    number("ts1Present", present[1]); number("ts1Bytes", sizes[1]); Serial.println('}');
    return Result::success();
}
Result fixture(uint8_t phase) {
    header = TaskSnapshotHeader(); header.authority = TaskAuthorityBinding();
    memcpy(header.authority.deviceInstanceId, instance.bytes, sizeof(instance.bytes));
    // Synthetic probe binding only, NOT verified owner roots or task admission.
    memcpy(header.authority.taskScope, "R4TaskStoreProbe!", 16U);
    for (size_t i = 0U; i < 32U; ++i) {
        header.authority.rootDigest[i] = static_cast<uint8_t>(0xA0U + i);
        header.requestDigest[i] = static_cast<uint8_t>(0x40U + phase + i);
    }
    header.tableRevision = phase; header.nextTaskId = 5U;
    memcpy(header.requestId, "R4TS", 4U); header.requestId[15] = phase;
    for (size_t i = 0U; i < capacity; ++i) {
        records[i] = TaskSnapshotRecord(); records[i].taskId = static_cast<uint32_t>(i + 1U);
        records[i].definitionRevision = phase; records[i].timing.durationMillis = 1000U + static_cast<uint32_t>(i);
        records[i].key[0] = 'p'; records[i].key[1] = static_cast<uint8_t>('1' + i); records[i].keyLength = 2U;
        records[i].value[0] = phase == 1U ? 0xF4U : 0xF5U; records[i].valueLength = 1U;
        // Disabled/never started. There is no executor in this firmware.
    }
    size_t written = 0U;
    const Result result = TaskSnapshotCodec::encode(header, records, capacity,
        MutableByteSpan(expected, sizeof(expected)), written);
    return result && written != payloadSize ? Result::failure(ErrorCode::StateConflict) : result;
}
Result readSnapshot(uint8_t& fixturePhase, bool reportRecords) {
    fixturePhase = 0U;
    size_t written = 0U;
    startObservation(); const uint32_t started = micros();
    Result result = tasks.load(MutableByteSpan(loaded, sizeof(loaded)), written);
    const uint32_t elapsed = micros() - started; endObservation(); measured("load", result, elapsed, written);
    if (!result) return result;
    size_t count = 0U;
    result = TaskSnapshotCodec::decode(ByteView(loaded, written), header, records, capacity, count);
    if (!result) return result;
    const uint8_t phase = header.tableRevision == 1U || header.tableRevision == 2U
        ? static_cast<uint8_t>(header.tableRevision) : 0U;
    if (phase != 0U && written == payloadSize && count == capacity && fixture(phase) &&
        memcmp(loaded, expected, payloadSize) == 0) fixturePhase = phase;
    uint8_t digest[kSha256Size] = {};
    result = sha256(ByteView(loaded, written), MutableByteSpan(digest, sizeof(digest)));
    Serial.print("{\"event\":\"snapshot\""); number("bytes", written); number("count", count);
    number("fixturePhase", fixturePhase); number("crc32", computeCrc32(ByteView(loaded, written)));
    Serial.print(",\"sha256\":\""); hex(digest, sizeof(digest)); Serial.println("\"}");
    if (reportRecords && fixturePhase != 0U) {
        for (size_t i = 0U; i < capacity; ++i) {
            Serial.print("{\"event\":\"record\""); number("id", records[i].taskId);
            number("definitionRevision", records[i].definitionRevision); number("enabled", records[i].enabled);
            number("progress", static_cast<unsigned>(records[i].progress)); number("startSequence", records[i].startSequence);
            number("durationMillis", records[i].timing.durationMillis); number("scalarCbor", records[i].value[0]);
            Serial.println('}');
        }
    }
    return result;
}

Result open() {
    if (openAttempted) return Result::failure(ErrorCode::AlreadyExists);
    openAttempted = true;
    modem.noDebug();
    const uint32_t started = micros();
    Result result = bank.begin();
    const uint32_t elapsed = micros() - started;
    Serial.print("{\"event\":\"open\""); number("us", elapsed); number("code", static_cast<unsigned>(result.code()));
    Serial.println('}');
    if (!result) return result;
    DeviceInstanceIdStore identity(bank.deviceInstanceBlob());
    result = identity.load(instance); // Strictly read existing identity, never loadOrCreate.
    if (result) result = captureIdentity(identityBefore);
    if (result) { identityCurrent = identityBefore; identityReport("open"); ready = true; }
    return result;
}
Result writeStep(uint8_t step) {
    if (!ready || writesLocked) return Result::failure(ErrorCode::StateConflict);
    const uint8_t mask = static_cast<uint8_t>(1U << step);
    if (attemptedWrites & mask) return Result::failure(ErrorCode::AlreadyExists);
    attemptedWrites |= mask; writesLocked = true; // Consume BEFORE any admission/read/write failure.
    if (Serial.available() != 0) return Result::failure(ErrorCode::InvalidArgument); // No pipelining.
    Result result = checkIdentity("before_write");
    bool present[2] = {}; uint32_t sizes[2] = {};
    if (result) result = taskMetadata(present, sizes);
    if (!result) return result;
    if (step == 1U) {
        if (!matchesTaskSlotShape(0U, present, sizes, journalSize)) return Result::failure(ErrorCode::AlreadyExists);
    } else {
        uint8_t currentPhase = 0U; result = readSnapshot(currentPhase, false);
        if (!result) return result;
        const uint8_t required = step == 2U ? 1U : 2U;
        if (currentPhase != required || !matchesTaskSlotShape(required, present, sizes, journalSize))
            return Result::failure(ErrorCode::StateConflict);
    }
    if (step != 3U) { result = fixture(step); if (!result) return result; }
    startObservation(); const uint32_t started = micros();
    result = step == 3U ? tasks.clear() : tasks.replace(ByteView(expected, sizeof(expected)));
    const uint32_t elapsed = micros() - started; endObservation();
    measured(step == 3U ? "clear" : "replace", result, elapsed, step == 3U ? 0U : payloadSize);
    if (!result) return result; // Unknown write outcome stays locked; never retries/rolls back.
    uint8_t phase = 0U;
    result = readSnapshot(phase, true);
    if (step == 3U) {
        if (result.code() != ErrorCode::NotFound) return Result::failure(ErrorCode::StateConflict);
    } else if (!result || phase != step) return Result::failure(ErrorCode::StateConflict);
    result = taskMetadata(present, sizes);
    if (!result || !matchesTaskSlotShape(step, present, sizes, journalSize))
        return Result::failure(ErrorCode::StateConflict);
    result = checkIdentity("after_write");
    if (result) writesLocked = false;
    ram();
    return result;
}
void command(const char* input) {
    Result result = Result::failure(ErrorCode::InvalidArgument);
    if (strcmp(input, "PING") == 0) {
        Serial.print("{\"event\":\"ping\",\"probe\":\"uno-r4-task-storage-v1\"");
        number("capacity", capacity); number("payloadBytes", payloadSize); number("journalBytes", journalSize);
        number("observerCompiled", BLINKER_INTERNAL_TASK_ALLOCATION_OBSERVER); Serial.println('}');
        result = Result::success();
    } else if (strcmp(input, "OPEN") == 0) result = open();
    else if (strcmp(input, "RAM") == 0) { ram(); result = Result::success(); }
    else if (strcmp(input, "READ") == 0 && ready) {
        result = checkIdentity("read");
        if (result) { uint8_t phase = 0U; result = readSnapshot(phase, true); }
        if (result || result.code() == ErrorCode::NotFound) {
            bool present[2] = {}; uint32_t sizes[2] = {};
            const Result metadata = taskMetadata(present, sizes);
            if (!metadata) result = metadata;
        }
        if (!result && result.code() != ErrorCode::NotFound) writesLocked = true;
    } else if (strcmp(input, "REPLACE1") == 0) result = writeStep(1U);
    else if (strcmp(input, "REPLACE2") == 0) result = writeStep(2U);
    else if (strcmp(input, "CLEAR_FIXTURE") == 0) result = writeStep(3U);
    else if (strcmp(input, "REBOOT") == 0) {
        Serial.println("{\"event\":\"software_reset\",\"notPowerLoss\":true}");
        Serial.flush(); delay(200U); NVIC_SystemReset(); return;
    }
    done(input, result);
}
} // namespace task_probe

void setup() {
    Serial.begin(115200U);
    // No storage/modem begin, reads, writes, identity creation or reboot at boot.
    Serial.println("{\"event\":\"boot\",\"probe\":\"uno-r4-task-storage-v1\",\"automaticWrites\":false}");
}
void loop() {
    static char input[24] = {}; static uint8_t length = 0U; static bool overflow = false;
    while (Serial.available()) {
        const int incoming = Serial.read();
        if (incoming < 0) break;
        const char value = static_cast<char>(incoming);
        if (value == '\r') continue;
        if (value == '\n') {
            input[length] = '\0';
            if (!overflow && length != 0U) task_probe::command(input);
            else if (overflow) {
                task_probe::writesLocked = true;
                task_probe::done("invalid_line", blinker::Result::failure(blinker::ErrorCode::InvalidArgument));
            }
            length = 0U; overflow = false;
        } else if (length + 1U < sizeof(input) &&
                   ((value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9') || value == '_'))
            input[length++] = value;
        else overflow = true;
    }
    delay(1U);
}
