#ifndef BLINKER_TEST_WIO_IDENTITY_TRANSITION_SERIAL_ADAPTER_H
#define BLINKER_TEST_WIO_IDENTITY_TRANSITION_SERIAL_ADAPTER_H

#include "TransitionController.h"
#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <string.h>

namespace wio_identity_transition {

// These two seams keep the physical USB shim separate from storage policy.
// Neither constructing this adapter nor serving public metadata opens QSPI.
class SerialTransport {
public:
    virtual ~SerialTransport() {}
    virtual uint32_t now() = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int writable() = 0;
    virtual size_t write(const uint8_t* bytes, size_t size) = 0;
    virtual void idle() = 0;
};
class TransitionReadback {
public:
    virtual ~TransitionReadback() {}
    virtual bool readUid(uint8_t* output16) = 0;
    virtual ByteView capturedRaw() const = 0;
    virtual ByteView temporaryInstance() const = 0;
};
struct SerialBinding {
    uint32_t capacity, taskAddress, identityAddress;
    uint16_t regionBytes;
    const char* sourceId;
    const char* originalSha256;
    bool executable;
};

class SerialAdapter {
public:
    enum : size_t { rawSize = 16384U, infoSize = 96U, transferChunk = 64U };
    enum : uint32_t { timeoutMs = 15000U };
    SerialAdapter(TransitionController& controller, TransitionReadback& readback,
                  SerialTransport& stream, const SerialBinding& binding)
        : controller_(controller), readback_(readback), stream_(stream), binding_(binding),
          desired_(), active_(false), invalidated_(false) {}

    void service() {
        if (active_) { invalidated_ = true; controller_.malformedInput(); return; }
        const int command = stream_.read();
        if (command < 0) return;
        invalidated_ = false; // A new frame may read public failed-state metadata, never unlock storage.
        ActiveCall active(active_);
        if (command == 'r') restore();
        else if (command == 'p' || command == 'd' || command == 't') {
            if (trailing()) return;
            if (command == 'p') profile();
            else if (!binding_.executable) fail(ErrorCode::NotConfigured);
            else if (command == 'd') capture();
            else prepare();
        } else fail(ErrorCode::InvalidArgument);
    }
private:
    class ActiveCall {
    public:
        explicit ActiveCall(bool& active) : active_(active) { active_ = true; }
        ~ActiveCall() { active_ = false; }
    private:
        bool& active_;
    };
    static int digit(char value) {
        return value >= '0' && value <= '9' ? value - '0' :
               value >= 'a' && value <= 'f' ? value - 'a' + 10 :
               value >= 'A' && value <= 'F' ? value - 'A' + 10 : -1;
    }
    static bool digestBytes(const char* text, uint8_t* output) {
        if (text == nullptr || strlen(text) != 64U) return false;
        for (size_t i = 0U; i < 32U; ++i) {
            const int high = digit(text[i * 2U]), low = digit(text[i * 2U + 1U]);
            if (high < 0 || low < 0) return false;
            output[i] = static_cast<uint8_t>((high << 4U) | low);
        }
        return true;
    }
    static void little16(uint8_t* bytes, uint16_t value) {
        bytes[0U] = static_cast<uint8_t>(value); bytes[1U] = static_cast<uint8_t>(value >> 8U);
    }
    static void little32(uint8_t* bytes, uint32_t value) {
        for (size_t i = 0U; i < 4U; ++i) bytes[i] = static_cast<uint8_t>(value >> (i * 8U));
    }
    static uint16_t read16(const uint8_t* bytes) {
        return static_cast<uint16_t>(bytes[0U] | (static_cast<uint16_t>(bytes[1U]) << 8U));
    }
    static uint32_t read32(const uint8_t* bytes) {
        uint32_t value = 0U;
        for (size_t i = 0U; i < 4U; ++i) value |= static_cast<uint32_t>(bytes[i]) << (i * 8U);
        return value;
    }
    bool timedOut(uint32_t started) { return static_cast<uint32_t>(stream_.now() - started) >= timeoutMs; }
    bool receive(uint8_t* bytes, size_t size, uint32_t started) {
        while (size != 0U) {
            if (invalidated_ || timedOut(started)) return false;
            const int value = stream_.read();
            if (value < 0) stream_.idle();
            else { *bytes++ = static_cast<uint8_t>(value); --size; }
        }
        // A blocking final read can return at/after the deadline; do not let
        // leaving the loop turn it into permission for a storage mutation.
        return !invalidated_ && !timedOut(started);
    }
    bool send(const uint8_t* bytes, size_t size, uint32_t started) {
        while (size != 0U) {
            if (timedOut(started)) return false;
            const int space = stream_.writable();
            if (space <= 0) { stream_.idle(); continue; }
            size_t chunk = size < transferChunk ? size : transferChunk;
            if (chunk > static_cast<size_t>(space)) chunk = static_cast<size_t>(space);
            const size_t written = stream_.write(bytes, chunk);
            if (written == 0U || written > chunk) return false;
            bytes += written; size -= written;
            if (invalidated_ || timedOut(started)) return false;
        }
        return true;
    }
    void wipeDesired() { blinker::secureZero(blinker::MutableByteSpan(desired_, sizeof(desired_))); }
    void fail(ErrorCode code) {
        controller_.malformedInput(); wipeDesired();
        const uint8_t error[5U] = {'B', 'V', 'E', '1', static_cast<uint8_t>(code)};
        // A partial response is never followed by a second frame or resync.
        (void)send(error, sizeof(error), stream_.now());
    }
    bool trailing() {
        if (stream_.available() == 0) return false;
        fail(ErrorCode::TrailingData); return true;
    }
    void response(const char* magic, ByteView payload) {
        if (trailing()) return;
        uint8_t header[10U] = {};
        memcpy(header, magic, 4U); little16(header + 4U, static_cast<uint16_t>(payload.size));
        little32(header + 6U, blinker::computeCrc32(payload));
        const uint32_t started = stream_.now();
        if (!send(header, sizeof(header), started) || !send(payload.data, payload.size, started))
            controller_.malformedInput();
    }
    bool currentDigest(uint8_t* digest) {
        const ByteView current = readback_.capturedRaw();
        return current.data != nullptr && current.size == rawSize &&
            blinker::sha256(current, blinker::MutableByteSpan(digest, blinker::kSha256Size));
    }
    void profile() {
        uint8_t info[infoSize] = {};
        info[0U] = 1U; little32(info + 1U, binding_.capacity);
        little32(info + 5U, binding_.taskAddress); little32(info + 9U, binding_.identityAddress);
        little16(info + 13U, binding_.regionBytes);
        if (!readback_.readUid(info + 15U) || !digestBytes(binding_.sourceId, info + 31U) ||
            !digestBytes(binding_.originalSha256, info + 63U)) { fail(ErrorCode::NotConfigured); return; }
        info[95U] = static_cast<uint8_t>(controller_.state());
        response("BVI1", ByteView(info, sizeof(info)));
    }
    void capture() {
        // Read UID on every admitted operation; the storage port compares it
        // to the compiled board binding before opening the owner.
        uint8_t uid[16U] = {};
        if (!readback_.readUid(uid)) { fail(ErrorCode::NotConfigured); return; }
        const Result result = controller_.execute(Command::CaptureCurrent);
        if (!result) { fail(result.code()); return; }
        const ByteView current = readback_.capturedRaw();
        if (current.data == nullptr || current.size != rawSize) { fail(ErrorCode::StateConflict); return; }
        response("BVS1", current);
    }
    void prepare() {
        uint8_t uid[16U] = {}, value[48U] = {};
        if (!readback_.readUid(uid)) { fail(ErrorCode::NotConfigured); return; }
        const Result result = controller_.execute(Command::PrepareTemp);
        if (!result) { fail(result.code()); return; }
        const ByteView instance = readback_.temporaryInstance();
        if (instance.data == nullptr || instance.size != 16U || !currentDigest(value + 16U)) {
            fail(ErrorCode::StateConflict); return;
        }
        memcpy(value, instance.data, 16U); response("BVT1", ByteView(value, sizeof(value)));
        blinker::secureZero(blinker::MutableByteSpan(value, sizeof(value)));
    }
    void restore() {
        // The command cannot be retried after incomplete input or uncertain
        // delivery. Host durable intents provide restart/recovery admission.
        if (!binding_.executable) { fail(ErrorCode::NotConfigured); return; }
        uint8_t header[10U] = {}, uid[16U] = {}, digest[blinker::kSha256Size] = {};
        const uint32_t started = stream_.now();
        if (!receive(header, sizeof(header), started)) { fail(ErrorCode::TruncatedInput); return; }
        if (memcmp(header, "BVR1", 4U) != 0 || read16(header + 4U) != rawSize) {
            fail(ErrorCode::InvalidHeader); return;
        }
        if (!receive(desired_, sizeof(desired_), started)) { fail(ErrorCode::TruncatedInput); return; }
        if (blinker::computeCrc32(ByteView(desired_, sizeof(desired_))) != read32(header + 6U)) {
            fail(ErrorCode::InvalidEncoding); return;
        }
        if (trailing()) return;
        if (!readback_.readUid(uid)) { fail(ErrorCode::NotConfigured); return; }
        const Result result = controller_.execute(Command::RestoreOriginal, ByteView(desired_, sizeof(desired_)));
        wipeDesired();
        if (!result) { fail(result.code()); return; }
        if (!currentDigest(digest)) { fail(ErrorCode::StateConflict); return; }
        response("BVA1", ByteView(digest, sizeof(digest)));
        blinker::secureZero(blinker::MutableByteSpan(digest, sizeof(digest)));
    }
    TransitionController& controller_;
    TransitionReadback& readback_;
    SerialTransport& stream_;
    SerialBinding binding_;
    uint8_t desired_[rawSize]; // Standalone maintenance only; never product RAM.
    bool active_, invalidated_;
    SerialAdapter(const SerialAdapter&) = delete;
    SerialAdapter& operator=(const SerialAdapter&) = delete;
};
} // namespace wio_identity_transition
#endif
