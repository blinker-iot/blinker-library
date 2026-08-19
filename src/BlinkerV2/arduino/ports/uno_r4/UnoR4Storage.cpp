#if defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_ARCH_RENESAS)

#include "UnoR4Storage.h"

#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/core/SecureMemory.h>

#include <limits.h>
#include <string.h>

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'R', 'J', '2'};
const uint8_t kSchemaVersion = 1U;
const uint8_t kActiveState = 1U;
const uint8_t kTombstoneState = 2U;

const char* const kKeys[11][2] = {
    {"cl0", "cl1"},
    {"ws0", "ws1"},
    {"w00", "w01"},
    {"w10", "w11"},
    {"ow0", "ow1"},
    {"ct0", "ct1"},
    {"di0", "di1"},
    {"oc0", "oc1"},
    {"ce0", "ce1"},
    {"or0", "or1"},
    {"sg0", "sg1"},
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

} // namespace

RenesasUnoPreferencesBlobBank::RenesasUnoPreferencesBlobBank(
    const RenesasUnoStorageConfig& config)
    : namespaceName_(),
      preferences_(),
      cloud_(*this, BlobCloud),
      wifiSelector_(*this, BlobWifiSelector),
      wifiSlot0_(*this, BlobWifiSlot0),
      wifiSlot1_(*this, BlobWifiSlot1),
      ownership_(*this, BlobOwnership),
      controller_(*this, BlobController),
      deviceInstance_(*this, BlobDeviceInstance),
      ownershipClaim_(*this, BlobOwnershipClaim),
      cloudEnrollment_(*this, BlobCloudEnrollment),
      ownershipRelease_(*this, BlobOwnershipRelease),
      localSetupSaga_(*this, BlobLocalSetupSaga),
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
    case BlobCloud:
        return CloudCredentialStore::serializedSize;
    case BlobWifiSelector:
        return WifiCredentialStore::serializedSelectorSize;
    case BlobWifiSlot0:
    case BlobWifiSlot1:
        return WifiCredentialStore::serializedProfileSize;
    case BlobOwnership:
        return OwnershipRecordStore::serializedSize;
    case BlobController:
        return ControllerCredentialStore::serializedSize;
    case BlobDeviceInstance:
        return DeviceInstanceIdStore::serializedSize;
    case BlobOwnershipClaim:
        return OwnershipClaimRecordStore::serializedSize;
    case BlobCloudEnrollment:
        return CloudEnrollmentRecordStore::serializedSize;
    case BlobOwnershipRelease:
        return OwnershipReleaseRecordStore::serializedSize;
    case BlobLocalSetupSaga:
        return LocalSetupSagaStore::serializedSize;
    default:
        return 0U;
    }
}

const char* RenesasUnoPreferencesBlobBank::key(
    BlobId id,
    uint8_t slot) {
    return id < BlobCount && slot < 2U ? kKeys[id][slot] : nullptr;
}

Result RenesasUnoPreferencesBlobBank::begin() {
    if (opened_) return Result::success();
    if (!configValid_) return Result::failure(ErrorCode::InvalidArgument);
    if (!preferences_.begin(namespaceName_, false)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    opened_ = true;
    return Result::success();
}

void RenesasUnoPreferencesBlobBank::end() {
    if (!opened_) return;
    preferences_.end();
    opened_ = false;
}

RenesasUnoPreferencesBlobBank::SlotInfo
RenesasUnoPreferencesBlobBank::inspect(
    BlobId id,
    uint8_t slot,
    uint8_t* scratch,
    size_t scratchSize) {
    SlotInfo info;
    const char* slotKey = key(id, slot);
    if (!opened_ || slotKey == nullptr || scratch == nullptr ||
        scratchSize < kMaximumRecordSize) {
        return info;
    }

    const size_t storedSize = preferences_.getBytesLength(slotKey);
    if (storedSize == 0U) {
        info.present = preferences_.isKey(slotKey);
        return info;
    }
    info.present = true;
    const size_t limit = kHeaderSize + maximumSize(id) + kCrcSize;
    if (storedSize < kHeaderSize + kCrcSize || storedSize > limit ||
        preferences_.getBytes(slotKey, scratch, scratchSize) != storedSize) {
        return info;
    }

    const uint16_t payloadSize = readU16(scratch + 12U);
    const uint8_t state = scratch[6];
    if (memcmp(scratch, kMagic, sizeof(kMagic)) != 0 ||
        scratch[4] != kSchemaVersion ||
        scratch[5] != static_cast<uint8_t>(id) || scratch[7] != 0U ||
        scratch[14] != 0U || scratch[15] != 0U ||
        readU32(scratch + 8U) == 0U ||
        payloadSize > maximumSize(id) ||
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
    if (output.data == nullptr && output.size != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    bool anyPresent = false;
    bool anyValid = false;
    bool selectedCopied = false;
    uint8_t scratch[kMaximumRecordSize] = {};
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        const SlotInfo info = inspect(
            id,
            slot,
            scratch,
            sizeof(scratch));
        anyPresent = anyPresent || info.present;
        if (!info.valid) continue;
        if (anyValid && info.generation == generation) {
            secureZero(MutableByteSpan(scratch, sizeof(scratch)));
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
            memcpy(output.data, scratch + kHeaderSize, written);
            selectedCopied = true;
        }
    }
    secureZero(MutableByteSpan(scratch, sizeof(scratch)));

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
    ByteView payload) {
    if (generation == 0U ||
        (state != kActiveState && state != kTombstoneState) ||
        (state == kActiveState && payload.empty()) ||
        (state == kTombstoneState && !payload.empty()) ||
        payload.size > maximumSize(id)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint8_t encoded[kMaximumRecordSize] = {};
    memcpy(encoded, kMagic, sizeof(kMagic));
    encoded[4] = kSchemaVersion;
    encoded[5] = static_cast<uint8_t>(id);
    encoded[6] = state;
    writeU32(encoded + 8U, generation);
    writeU16(encoded + 12U, static_cast<uint16_t>(payload.size));
    if (!payload.empty()) {
        memcpy(encoded + kHeaderSize, payload.data, payload.size);
    }
    writeU32(
        encoded + kHeaderSize + payload.size,
        computeCrc32(ByteView(
            encoded,
            kHeaderSize + payload.size)));
    const size_t encodedSize = kHeaderSize + payload.size + kCrcSize;
    const char* slotKey = key(id, slot);
    const size_t reported = preferences_.putBytes(
        slotKey,
        encoded,
        encodedSize);

    uint8_t verified[kMaximumRecordSize] = {};
    const size_t storedSize = preferences_.getBytesLength(slotKey);
    const size_t readSize = storedSize == encodedSize
                                ? preferences_.getBytes(
                                      slotKey,
                                      verified,
                                      sizeof(verified))
                                : 0U;
    const bool matches = readSize == encodedSize &&
                         memcmp(verified, encoded, encodedSize) == 0;
    secureZero(MutableByteSpan(verified, sizeof(verified)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));

    if (matches) return Result::success();
    return Result::failure(
        reported == encodedSize
            ? ErrorCode::InternalError
            : ErrorCode::WouldBlock);
}

Result RenesasUnoPreferencesBlobBank::load(
    BlobId id,
    MutableByteSpan output,
    size_t& written) {
    uint32_t generation = 0U;
    uint8_t slot = 0U;
    uint8_t state = 0U;
    return newest(
        id,
        output,
        written,
        generation,
        slot,
        state);
}

Result RenesasUnoPreferencesBlobBank::replace(
    BlobId id,
    ByteView value) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (value.data == nullptr || value.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (value.size > maximumSize(id)) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    size_t ignored = 0U;
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    uint8_t state = 0U;
    const Result current = newest(
        id,
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
        value);
}

Result RenesasUnoPreferencesBlobBank::clear(BlobId id) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    size_t ignored = 0U;
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    uint8_t state = 0U;
    const Result current = newest(
        id,
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
        ByteView());
}

} // namespace blinker

#endif // ARDUINO_ARCH_RENESAS_UNO || ARDUINO_ARCH_RENESAS
