#if defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_ARCH_RENESAS)

#include "UnoR4Storage.h"

#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/core/SecureMemory.h>

#include <limits.h>
#include <string.h>
#include <string>

namespace blinker {

namespace {

uint32_t modemNow() { return static_cast<uint32_t>(millis()); }
void modemTimeout(uint32_t value) { modem.timeout(value); }

const uint8_t kMagic[4] = {'B', 'R', 'J', '2'};
const uint8_t kSchemaVersion = 1U;
const uint8_t kActiveState = 1U;
const uint8_t kTombstoneState = 2U;

const char* const kKeys[6][2] = {
    {"ws0", "ws1"},
    {"w00", "w01"},
    {"w10", "w11"},
    {"di0", "di1"},
    {"ac0", "ac1"},
    {"ts0", "ts1"},
};

bool validIdentifierCharacter(char value) {
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '_' || value == '-';
}

void writeU16(uint8_t* output, uint16_t value) {
    output[0] = static_cast<uint8_t>(value >> 8U);
    output[1] = static_cast<uint8_t>(value);
}

uint16_t readU16(const uint8_t* input) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(input[0]) << 8U) |
        static_cast<uint16_t>(input[1]));
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool parsePreferenceDecimal(const std::string& text, uint32_t maximum, uint32_t& value) {
    value = 0U;
    if (text.empty()) return false;
    for (size_t index = 0U; index < text.size(); ++index) {
        if (text[index] < '0' || text[index] > '9') return false;
        const uint32_t digit = static_cast<uint32_t>(text[index] - '0');
        if (digit > maximum || value > (maximum - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    return true;
}

// Preferences folds AT failure into length=0/type=PT_INVALID. Confirm the two
// metadata RPCs rather than treating an unreadable slot as durable absence.
// This uses the existing SDK modem and the bank's one active namespace only.
Result checkedPreferenceMetadata(const char* slotKey, bool& present, size_t& size) {
    present = false;
    size = 0U;
    std::string response;
    uint32_t type = 0U;
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    Result ready = owner.prepare();
    if (!ready) return ready;
    const bool typeReply = modem.write(std::string(PROMPT(_PREF_TYPE)), response,
                                       "%s%s\r\n", CMD_WRITE(_PREF_TYPE), slotKey);
    ready = owner.complete(typeReply);
    if (!ready) return ready;
    if (!parsePreferenceDecimal(response, PT_INVALID, type)) {
        (void)owner.complete(false);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (type == PT_INVALID) return Result::success();
    present = true;
    if (type != PT_BLOB) return Result::failure(ErrorCode::InvalidEncoding);
    ready = owner.prepare();
    if (!ready) return ready;
    ready = owner.complete(modem.write(std::string(PROMPT(_PREF_LEN)), response,
                                      "%s%s\r\n", CMD_WRITE(_PREF_LEN), slotKey));
    if (!ready) return ready;
    uint32_t length = 0U;
    if (!parsePreferenceDecimal(response, UINT32_MAX, length)) {
        (void)owner.complete(false);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    size = static_cast<size_t>(length);
    return Result::success();
}

Result checkedPreferenceBytes(const char* slotKey, MutableByteSpan output, size_t size) {
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    Result ready = owner.prepare();
    if (!ready) return ready;
    std::string response;
    modem.avoid_trim_results();
    ready = owner.complete(modem.write(std::string(PROMPT(_PREF_GET)), response,
                                      "%s%s,%d\r\n", CMD_WRITE(_PREF_GET), slotKey, PT_BLOB));
    if (!ready) return ready;
    if (response.size() != size || size > output.size) {
        (void)owner.complete(false);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    memcpy(output.data, response.data(), size);
    return Result::success();
}

} // namespace

RenesasUnoModemOwner& renesasUnoModemOwner() {
    static RenesasUnoModemOwner owner(&modemNow, &modemTimeout, MODEM_TIMEOUT);
    return owner;
}

RenesasUnoPreferencesBlobBank::RenesasUnoPreferencesBlobBank(
    const RenesasUnoStorageConfig& config)
    : namespaceName_(),
      wifiSelector_(*this, BlobWifiSelector),
      wifiSlot0_(*this, BlobWifiSlot0),
      wifiSlot1_(*this, BlobWifiSlot1),
      deviceInstance_(*this, BlobDeviceInstance),
      deviceAccess_(*this, BlobDeviceAccess),
      configValid_(copyNamespace(config.namespaceName, namespaceName_)),
      opened_(false) {}

RenesasUnoPreferencesBlobBank::~RenesasUnoPreferencesBlobBank() {
    end();
}

bool RenesasUnoPreferencesBlobBank::copyNamespace(
    const char* source,
    char* destination) {
    if (source == nullptr || source[0] == '\0') return false;
    size_t length = 0U;
    while (source[length] != '\0') {
        if (length >= kNamespaceCapacity ||
            !validIdentifierCharacter(source[length])) {
            destination[0] = '\0';
            return false;
        }
        destination[length] = source[length];
        ++length;
    }
    destination[length] = '\0';
    return true;
}

size_t RenesasUnoPreferencesBlobBank::maximumSize(BlobId id) {
    switch (id) {
    case BlobWifiSelector:
        return WifiCredentialStore::serializedSelectorSize;
    case BlobWifiSlot0:
    case BlobWifiSlot1:
        return WifiCredentialStore::serializedProfileSize;
    case BlobDeviceInstance:
        return DeviceInstanceIdStore::serializedSize;
    case BlobDeviceAccess:
        return DeviceAccessStore::serializedSize;
    default:
        return 0U;
    }
}

uint8_t RenesasUnoPreferencesBlobBank::legacyId(BlobId id) {
    // Before the storage graph was compacted, these same keys used IDs
    // 1, 2, 3 and 6. Accept their intact journal envelopes so a library
    // upgrade preserves WiFi credentials and the physical device identity.
    switch (id) {
    case BlobWifiSelector:
        return 1U;
    case BlobWifiSlot0:
        return 2U;
    case BlobWifiSlot1:
        return 3U;
    case BlobDeviceInstance:
        return 6U;
    default:
        return 0xFFU;
    }
}

const char* RenesasUnoPreferencesBlobBank::key(
    BlobId id,
    uint8_t slot) {
    return id < BlobCount && slot < 2U ? kKeys[id][slot] : nullptr;
}

Result RenesasUnoPreferencesBlobBank::begin() {
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    if (owner.quarantined()) return Result::failure(ErrorCode::StateConflict);
    if (opened_) return Result::success();
    if (!configValid_) return Result::failure(ErrorCode::InvalidArgument);
    if (!owner.claimNamespace(this)) return Result::failure(ErrorCode::WouldBlock);
    // SDK initialization has its own 500 ms retries; no task operation runs
    // until initialization/namespace setup is complete.
    modem.begin();
    owner.baseline();
    RenesasUnoModemOwner::Operation operation(owner, this, MODEM_TIMEOUT);
    Result ready = operation.result();
    if (ready) ready = owner.complete(modem.beginned);
    if (ready) ready = owner.prepare();
    std::string response;
    if (ready) ready = owner.complete(modem.write(std::string(PROMPT(_PREF_BEGIN)), response,
        "%s%s,%d,%s\r\n", CMD_WRITE(_PREF_BEGIN), namespaceName_, 0, ""));
    uint32_t accepted = 0U;
    if (ready && !parsePreferenceDecimal(response, 1U, accepted)) {
        (void)owner.complete(false); ready = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!ready || accepted != 1U) {
        owner.releaseNamespace(this);
        return ready ? Result::failure(ErrorCode::NotConfigured) : ready;
    }
    opened_ = true;
    return Result::success();
}

void RenesasUnoPreferencesBlobBank::end() {
    if (!opened_) return;
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    if (!owner.quarantined()) {
        RenesasUnoModemOwner::Operation operation(owner, this, MODEM_TIMEOUT);
        if (!operation.result()) return; // No close/release during another operation.
        if (owner.prepare()) {
            std::string response;
            (void)owner.complete(modem.write(std::string(PROMPT(_PREF_END)), response,
                                             "%s", CMD(_PREF_END)));
        }
    }
    opened_ = false;
    owner.releaseNamespace(this);
}

RenesasUnoPreferencesBlobBank::SlotInfo
RenesasUnoPreferencesBlobBank::inspect(
    BlobId id,
    uint8_t slot,
    size_t maximumPayload,
    uint8_t* scratch,
    size_t scratchSize) {
    SlotInfo info;
    const char* slotKey = key(id, slot);
    if (!opened_ || slotKey == nullptr || scratch == nullptr ||
        maximumPayload == 0U || maximumPayload > UINT16_MAX ||
        scratchSize < kHeaderSize + maximumPayload + kCrcSize) {
        return info;
    }

    size_t storedSize = 0U;
    const Result metadata = checkedPreferenceMetadata(slotKey, info.present, storedSize);
    if (!metadata) {
        info.ioError = metadata.code();
        return info;
    }
    if (!info.present) return info;
    const size_t limit = kHeaderSize + maximumPayload + kCrcSize;
    if (storedSize < kHeaderSize + kCrcSize || storedSize > limit) {
        return info;
    }
    const Result bytes = checkedPreferenceBytes(slotKey, MutableByteSpan(scratch, scratchSize), storedSize);
    if (!bytes) {
        info.ioError = bytes.code();
        return info;
    }

    const uint16_t payloadSize = readU16(scratch + 12U);
    const uint8_t state = scratch[6];
    const uint8_t storedId = scratch[5];
    if (memcmp(scratch, kMagic, sizeof(kMagic)) != 0 ||
        scratch[4] != kSchemaVersion ||
        (storedId != static_cast<uint8_t>(id) &&
         (id == BlobTaskSnapshot || storedId != legacyId(id))) ||
        scratch[7] != 0U ||
        scratch[14] != 0U || scratch[15] != 0U ||
        readU32(scratch + 8U) == 0U ||
        payloadSize > maximumPayload ||
        storedSize != kHeaderSize + payloadSize + kCrcSize ||
        (state == kActiveState && payloadSize == 0U) ||
        (state == kTombstoneState && payloadSize != 0U) ||
        (state != kActiveState && state != kTombstoneState) ||
        readU32(scratch + kHeaderSize + payloadSize) !=
            computeCrc32(ByteView(
                scratch,
                kHeaderSize + payloadSize))) {
        return info;
    }

    info.generation = readU32(scratch + 8U);
    info.payloadSize = payloadSize;
    info.state = state;
    info.valid = true;
    return info;
}

Result RenesasUnoPreferencesBlobBank::newest(
    BlobId id,
    size_t maximumPayload,
    MutableByteSpan scratch,
    MutableByteSpan output,
    size_t& written,
    uint32_t& generation,
    uint8_t& selectedSlot,
    uint8_t& state) {
    written = 0U;
    generation = 0U;
    selectedSlot = 0U;
    state = 0U;
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if ((output.data == nullptr && output.size != 0U) ||
        key(id, 0U) == nullptr || maximumPayload == 0U ||
        maximumPayload > UINT16_MAX || scratch.data == nullptr ||
        scratch.size < kHeaderSize + maximumPayload + kCrcSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    bool anyPresent = false;
    bool anyValid = false;
    bool selectedCopied = false;
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        const SlotInfo info = inspect(
            id,
            slot,
            maximumPayload,
            scratch.data,
            scratch.size);
        if (info.ioError != ErrorCode::Ok) {
            written = 0U;
            secureZero(scratch);
            return Result::failure(info.ioError);
        }
        anyPresent = anyPresent || info.present;
        if (!info.valid) continue;
        if (anyValid && info.generation == generation) {
            secureZero(scratch);
            return Result::failure(ErrorCode::SequenceConflict);
        }
        if (anyValid && info.generation < generation) continue;

        anyValid = true;
        generation = info.generation;
        selectedSlot = slot;
        state = info.state;
        written = info.payloadSize;
        selectedCopied = false;
        if (state == kActiveState && output.data != nullptr &&
            output.size >= written) {
            memcpy(output.data, scratch.data + kHeaderSize, written);
            selectedCopied = true;
        }
    }
    secureZero(scratch);

    if (!anyValid) {
        return Result::failure(
            anyPresent ? ErrorCode::InvalidEncoding : ErrorCode::NotFound);
    }
    if (state == kTombstoneState) {
        written = 0U;
        return Result::failure(ErrorCode::NotFound);
    }
    if (output.data == nullptr || output.size < written || !selectedCopied) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    return Result::success();
}

Result RenesasUnoPreferencesBlobBank::writeRecord(
    BlobId id,
    uint8_t slot,
    uint32_t generation,
    uint8_t state,
    ByteView payload,
    size_t maximumPayload,
    MutableByteSpan encoded,
    MutableByteSpan verified) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (key(id, slot) == nullptr || maximumPayload == 0U ||
        maximumPayload > UINT16_MAX || encoded.data == nullptr ||
        verified.data == nullptr ||
        encoded.size < kHeaderSize + maximumPayload + kCrcSize ||
        verified.size < kHeaderSize + maximumPayload + kCrcSize ||
        generation == 0U ||
        (state != kActiveState && state != kTombstoneState) ||
        (state == kActiveState && payload.empty()) ||
        (state == kTombstoneState && !payload.empty()) ||
        payload.size > maximumPayload ||
        (payload.data == nullptr && !payload.empty())) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    memset(encoded.data, 0, encoded.size);
    memcpy(encoded.data, kMagic, sizeof(kMagic));
    encoded.data[4] = kSchemaVersion;
    encoded.data[5] = static_cast<uint8_t>(id);
    encoded.data[6] = state;
    writeU32(encoded.data + 8U, generation);
    writeU16(encoded.data + 12U, static_cast<uint16_t>(payload.size));
    if (!payload.empty()) {
        memcpy(encoded.data + kHeaderSize, payload.data, payload.size);
    }
    writeU32(
        encoded.data + kHeaderSize + payload.size,
        computeCrc32(ByteView(
            encoded.data,
            kHeaderSize + payload.size)));
    const size_t encodedSize = kHeaderSize + payload.size + kCrcSize;
    const char* slotKey = key(id, slot);
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    Result ready = owner.prepare();
    if (!ready) { secureZero(encoded); secureZero(verified); return ready; }
    owner.writing();
    std::string response;
    modem.write_nowait(std::string(PROMPT(_PREF_PUT)), response,
        "%s%s,%d,%d\r\n", CMD_WRITE(_PREF_PUT), slotKey, PT_BLOB, static_cast<int>(encodedSize));
    owner.prepareWriteResponse();
    ready = owner.complete(modem.passthrough(encoded.data, encodedSize));
    if (!ready) { secureZero(encoded); secureZero(verified); return ready; }

    bool present = false;
    size_t storedSize = 0U;
    const Result metadata = checkedPreferenceMetadata(slotKey, present, storedSize);
    const Result bytes = metadata && present && storedSize == encodedSize
        ? checkedPreferenceBytes(slotKey, verified, encodedSize)
        : Result::failure(ErrorCode::InternalError);
    const bool matches = bytes &&
                         memcmp(verified.data, encoded.data, encodedSize) == 0;
    secureZero(verified);
    secureZero(encoded);

    if (!metadata) return metadata;
    if (!bytes) return bytes;
    if (matches) return Result::success();
    return Result::failure(ErrorCode::InternalError);
}

Result RenesasUnoPreferencesBlobBank::load(
    BlobId id,
    MutableByteSpan output,
    size_t& written) {
    uint8_t scratch[kMaximumRecordSize] = {};
    return loadBounded(id, maximumSize(id),
                       MutableByteSpan(scratch, sizeof(scratch)), output, written);
}

Result RenesasUnoPreferencesBlobBank::loadBounded(
    BlobId id, size_t maximumPayload, MutableByteSpan scratch,
    MutableByteSpan output, size_t& written, uint32_t budgetMillis) {
    written = 0U;
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    RenesasUnoModemOwner::Operation operation(renesasUnoModemOwner(), this, budgetMillis);
    if (!operation.result()) return operation.result();
    uint32_t generation = 0U;
    uint8_t slot = 0U;
    uint8_t state = 0U;
    return newest(
        id,
        maximumPayload,
        scratch,
        output,
        written,
        generation,
        slot,
        state);
}

Result RenesasUnoPreferencesBlobBank::replace(
    BlobId id,
    ByteView value) {
    uint8_t encoded[kMaximumRecordSize] = {};
    uint8_t verified[kMaximumRecordSize] = {};
    return replaceBounded(id, maximumSize(id),
                          MutableByteSpan(encoded, sizeof(encoded)),
                          MutableByteSpan(verified, sizeof(verified)), value);
}

Result RenesasUnoPreferencesBlobBank::replaceBounded(
    BlobId id, size_t maximumPayload, MutableByteSpan encoded,
    MutableByteSpan verified, ByteView value, uint32_t budgetMillis) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (value.data == nullptr || value.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (value.size > maximumPayload) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    RenesasUnoModemOwner::Operation operation(renesasUnoModemOwner(), this, budgetMillis);
    if (!operation.result()) return operation.result();

    size_t ignored = 0U;
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    uint8_t state = 0U;
    const Result current = newest(
        id,
        maximumPayload,
        encoded,
        MutableByteSpan(),
        ignored,
        generation,
        slot,
        state);
    if (!current && current.code() != ErrorCode::NotFound &&
        current.code() != ErrorCode::BufferTooSmall) {
        return current;
    }
    if (generation == UINT32_MAX) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    return writeRecord(
        id,
        static_cast<uint8_t>(slot ^ 1U),
        generation + 1U,
        kActiveState,
        value,
        maximumPayload,
        encoded,
        verified);
}

Result RenesasUnoPreferencesBlobBank::clear(BlobId id) {
    uint8_t encoded[kMaximumRecordSize] = {};
    uint8_t verified[kMaximumRecordSize] = {};
    return clearBounded(id, maximumSize(id),
                        MutableByteSpan(encoded, sizeof(encoded)),
                        MutableByteSpan(verified, sizeof(verified)));
}

Result RenesasUnoPreferencesBlobBank::clearBounded(
    BlobId id, size_t maximumPayload, MutableByteSpan encoded,
    MutableByteSpan verified, uint32_t budgetMillis) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    RenesasUnoModemOwner::Operation operation(renesasUnoModemOwner(), this, budgetMillis);
    if (!operation.result()) return operation.result();
    size_t ignored = 0U;
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    uint8_t state = 0U;
    const Result current = newest(
        id,
        maximumPayload,
        encoded,
        MutableByteSpan(),
        ignored,
        generation,
        slot,
        state);
    if (!current && current.code() == ErrorCode::NotFound &&
        generation != 0U && state == kTombstoneState) {
        return Result::failure(ErrorCode::NotFound);
    }
    if (!current && current.code() != ErrorCode::BufferTooSmall &&
        current.code() != ErrorCode::NotFound) {
        return current;
    }
    if (generation == 0U) return Result::failure(ErrorCode::NotFound);
    if (generation == UINT32_MAX) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    return writeRecord(
        id,
        static_cast<uint8_t>(slot ^ 1U),
        generation + 1U,
        kTombstoneState,
        ByteView(),
        maximumPayload,
        encoded,
        verified);
}

} // namespace blinker

#endif // ARDUINO_ARCH_RENESAS_UNO || ARDUINO_ARCH_RENESAS
