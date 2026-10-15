#ifndef BLINKER_TEST_STORAGE_GATE_DRIVER_H
#define BLINKER_TEST_STORAGE_GATE_DRIVER_H

// Include exactly one target port before this driver. No product, network,
// timer, automatic migration or persistent maintenance state is constructed.
namespace storage_gate {

static uint8_t current[port::maximumSnapshot] = {};
static uint8_t desired[port::maximumSnapshot] = {};
static bool restoreLocked = false;
static_assert(port::maximumSnapshot <= 65535U, "Snapshot must fit the binary length");
static const uint32_t serialTimeoutMs = 15000U;

enum Failure : uint8_t {
    UnknownCommand = 1U,
    RequestTimeout = 2U,
    InvalidRequest = 3U,
    DesiredCrcMismatch = 4U,
    SnapshotFailure = 5U,
    UnstableSnapshot = 6U,
    CurrentCrcMismatch = 7U,
    TargetMismatch = 8U,
    WriteOrReadbackFailure = 9U,
    FinalSnapshotMismatch = 10U,
    RestoreAlreadyLocked = 11U
};
inline bool timedOut(uint32_t started) {
    return static_cast<uint32_t>(millis() - started) >= serialTimeoutMs;
}
inline void clearBuffers() {
    wipe(current, sizeof(current)); wipe(desired, sizeof(desired));
}
inline bool receive(uint8_t* bytes, size_t size, uint32_t started) {
    while (size != 0U) {
        if (timedOut(started)) return false;
        const int next = Serial.read();
        if (next >= 0) { *bytes++ = static_cast<uint8_t>(next); --size; }
        else delay(1U);
    }
    return true;
}
inline bool send(const uint8_t* bytes, size_t size, uint32_t started) {
    while (size != 0U) {
        if (timedOut(started)) return false;
        size_t chunk = size < 64U ? size : 64U;
#if defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)
        // R4 NO_USB Serial is UART: its inherited availableForWrite() is 0.
        // Only the Wio CDC implementation exposes usable free-bank capacity.
        const int space = Serial.availableForWrite();
        if (space <= 0) { delay(1U); continue; }
        if (chunk > static_cast<size_t>(space)) chunk = static_cast<size_t>(space);
#endif
        const size_t written = Serial.write(bytes, chunk);
        if (written == 0U || written > chunk) return false;
        bytes += written; size -= written;
    }
    return true;
}
inline void fail(Failure code) {
    const uint8_t response[5U] = {'B', 'V', 'E', '1', static_cast<uint8_t>(code)};
    clearBuffers();
    if (!send(response, sizeof(response), millis())) restoreLocked = true;
}
inline bool stableSnapshot(size_t& size, Failure& failure) {
    size = 0U;
    if (!port::snapshot(current, size)) { failure = SnapshotFailure; return false; }
    if (!port::matchesSnapshot(current, size)) { failure = UnstableSnapshot; return false; }
    return true;
}
inline void dump() {
    size_t size = 0U;
    Failure failure = SnapshotFailure;
    if (!stableSnapshot(size, failure)) { fail(failure); return; }
    uint8_t header[10U] = {'B', 'V', 'S', '1'};
    putLittle16(header + 4U, static_cast<uint16_t>(size));
    putLittle32(header + 6U, crc32(current, size));
    const uint32_t started = millis();
    // A partial response is not followed by another frame: the host must treat
    // it as failed, close the stream and reconcile instead of scanning secrets.
    if (!send(header, sizeof(header), started) || !send(current, size, started))
        restoreLocked = true;
    clearBuffers();
}
inline void restore() {
    if (restoreLocked) { fail(RestoreAlreadyLocked); return; }
    // Every failed restore stays locked, including incomplete/invalid requests.
    // Payload bytes arriving after a timeout cannot become a new write request.
    restoreLocked = true;
    uint8_t header[14U] = {};
    const uint32_t started = millis();
    if (!receive(header, sizeof(header), started)) { fail(RequestTimeout); return; }
    const size_t desiredSize = little16(header + 4U);
    if (memcmp(header, "BVR1", 4U) != 0 || desiredSize == 0U ||
        desiredSize > port::maximumSnapshot) { fail(InvalidRequest); return; }
    if (!receive(desired, desiredSize, started)) { fail(RequestTimeout); return; }
    // One command at a time. No host pipelining or trailing request bytes.
    if (Serial.available() != 0) { fail(InvalidRequest); return; }
    const uint32_t expectedCurrent = little32(header + 6U);
    const uint32_t expectedDesired = little32(header + 10U);
    if (crc32(desired, desiredSize) != expectedDesired) { fail(DesiredCrcMismatch); return; }
    size_t currentSize = 0U;
    Failure failure = SnapshotFailure;
    if (!stableSnapshot(currentSize, failure)) { fail(failure); return; }
    if (crc32(current, currentSize) != expectedCurrent) { fail(CurrentCrcMismatch); return; }
    if (!port::validateTarget(desired, desiredSize, current, currentSize)) {
        fail(TargetMismatch); return;
    }
    if (Serial.available() != 0) { fail(InvalidRequest); return; }
    // The entire desired frame, both CRCs, layout, immutable instance anchor,
    // and two independent current reads are validated BEFORE any media write.
    if (!port::restore(desired, desiredSize, current, currentSize)) {
        fail(WriteOrReadbackFailure); return;
    }
    if (!stableSnapshot(currentSize, failure)) { fail(failure); return; }
    const uint32_t finalCrc = crc32(current, currentSize);
    if (currentSize != desiredSize || finalCrc != expectedDesired ||
        memcmp(current, desired, currentSize) != 0) { fail(FinalSnapshotMismatch); return; }
    uint8_t response[8U] = {'B', 'V', 'A', '1'};
    putLittle32(response + 4U, finalCrc);
    clearBuffers();
    if (send(response, sizeof(response), millis())) restoreLocked = false;
}
inline void service() {
    const int command = Serial.read();
    if (command < 0) { delay(1U); return; }
    if (command == 'd') dump();
    else if (command == 'r') restore();
#if (defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)) && \
    defined(BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE) && BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE >= 1
    else if (command == 'p') {
        // Public fixture/source/layout/UID only; never opens or touches QSPI.
        uint8_t info[63U] = {}; size_t size = 0U;
        if (!port::publicInfo(info, size, sizeof(info)) || size != sizeof(info)) {
            fail(InvalidRequest); return;
        }
        uint8_t header[10U] = {'B', 'V', 'P', '1'};
        putLittle16(header + 4U, static_cast<uint16_t>(size));
        putLittle32(header + 6U, crc32(info, size));
        const uint32_t started = millis();
        if (!send(header, sizeof(header), started) || !send(info, size, started)) restoreLocked = true;
        wipe(info, sizeof(info));
    }
#endif
    else fail(UnknownCommand);
}

} // namespace storage_gate
#endif
