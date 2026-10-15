#include "WioTerminalStorage.h"

#include <BlinkerV2/core/Crc32.h>
#include <limits.h>
#include <string.h>

namespace {

bool qspiMatches(blinker::WioTerminalQspiOwner& owner, const void* token,
                 uint32_t address, const uint8_t* expected, size_t size) {
    uint8_t actual[64];
    while (size != 0U) {
        const size_t checked = size < sizeof(actual) ? size : sizeof(actual);
        if (!owner.read(token, address, actual, checked)) return false;
        if (memcmp(actual, expected, checked) != 0) {
            (void)owner.verificationFailed(token);
            return false;
        }
        address += static_cast<uint32_t>(checked);
        expected += checked;
        size -= checked;
    }
    return true;
}

bool qspiBlank(blinker::WioTerminalQspiOwner& owner, const void* token,
               uint32_t address, size_t size) {
    uint8_t bytes[64];
    while (size != 0U) {
        const size_t checked = size < sizeof(bytes) ? size : sizeof(bytes);
        if (!owner.read(token, address, bytes, checked)) return false;
        for (size_t index = 0U; index < checked; ++index)
            if (bytes[index] != 0xffU) return false;
        address += static_cast<uint32_t>(checked);
        size -= checked;
    }
    return true;
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

namespace blinker {

WioTerminalFlashBlobBank::BlobView::BlobView(
    WioTerminalFlashBlobBank& bank,
    BlobId id)
    : bank_(bank), id_(id) {}

Result WioTerminalFlashBlobBank::BlobView::load(
    MutableByteSpan output,
    size_t& written) {
    return bank_.load(id_, output, written);
}

Result WioTerminalFlashBlobBank::BlobView::replace(ByteView value) {
    return bank_.replace(id_, value);
}

Result WioTerminalFlashBlobBank::BlobView::clear() {
    return bank_.clear(id_);
}

StorageProtection WioTerminalFlashBlobBank::BlobView::protection() const {
    return StorageProtection::PlainFlash;
}

WioTerminalFlashBlobBank::WioTerminalFlashBlobBank(WioTerminalQspiOwner& owner)
    : deviceInstance_(*this, BlobDeviceInstance),
      wifiSelector_(*this, BlobWifiSelector),
      wifiSlot0_(*this, BlobWifiSlot0),
      wifiSlot1_(*this, BlobWifiSlot1),
      deviceAccess_(*this, BlobDeviceAccess),
      owner_(owner),
      opened_(false) {}

Result WioTerminalFlashBlobBank::begin() {
    if (opened_) return owner_.quarantined()
        ? Result::failure(ErrorCode::StateConflict) : Result::success();
    Result result = owner_.acquire(this, WioTerminalQspiOwner::Region::Identity);
    if (!result) return result;
    {
        WioTerminalQspiOwner::Operation operation(
            owner_, this, WioTerminalQspiOwner::identityBudgetMillis);
        result = operation.result();
        if (result && !storageRegionAvailable())
            result = Result::failure(owner_.quarantined()
                ? ErrorCode::WouldBlock : ErrorCode::StateConflict);
    }
    if (!result) {
        (void)owner_.release(this);
        return result;
    }
    opened_ = true;
    return Result::success();
}

void WioTerminalFlashBlobBank::end() {
    if (opened_ && owner_.release(this)) opened_ = false;
}

IAtomicBlobStore& WioTerminalFlashBlobBank::deviceInstanceBlob() {
    return deviceInstance_;
}

IAtomicBlobStore& WioTerminalFlashBlobBank::wifiSelectorBlob() {
    return wifiSelector_;
}

IAtomicBlobStore& WioTerminalFlashBlobBank::wifiSlot0Blob() {
    return wifiSlot0_;
}

IAtomicBlobStore& WioTerminalFlashBlobBank::wifiSlot1Blob() {
    return wifiSlot1_;
}

IAtomicBlobStore& WioTerminalFlashBlobBank::deviceAccessBlob() {
    return deviceAccess_;
}

size_t WioTerminalFlashBlobBank::maximumSize(BlobId id) {
    switch (id) {
    case BlobDeviceInstance:
        return DeviceInstanceIdStore::serializedSize;
    case BlobWifiSelector:
        return WifiCredentialStore::serializedSelectorSize;
    case BlobWifiSlot0:
    case BlobWifiSlot1:
        return WifiCredentialStore::serializedProfileSize;
    case BlobDeviceAccess:
        return DeviceAccessStore::serializedSize;
    default:
        return 0U;
    }
}

size_t WioTerminalFlashBlobBank::entryOffset(BlobId id) {
    size_t result = kHeaderSize;
    for (uint8_t current = 0U; current < static_cast<uint8_t>(id);
         ++current) {
        result += kEntryHeaderSize +
                  maximumSize(static_cast<BlobId>(current));
    }
    return result;
}

uint8_t* WioTerminalFlashBlobBank::entry(uint8_t* image, BlobId id) {
    return image + entryOffset(id);
}

const uint8_t* WioTerminalFlashBlobBank::entry(
    const uint8_t* image,
    BlobId id) {
    return image + entryOffset(id);
}

bool WioTerminalFlashBlobBank::validImage(
    uint8_t* image,
    uint32_t& generation) {
    static const uint8_t magic[4] = {'B', 'W', 'B', '2'};
    generation = readU32(image + 8U);
    const uint32_t expected = readU32(image + 12U);
    writeU32(image + 12U, 0U);
    const uint32_t currentCrc = computeCrc32(ByteView(image, kImageSize));
    const uint32_t previousCrc = computeCrc32(
        ByteView(image, kPreviousImageSize));
    const uint32_t legacyCrc = computeCrc32(
        ByteView(image, kLegacyImageSize));
    writeU32(image + 12U, expected);
    const bool previous =
        expected != currentCrc && expected == previousCrc;
    const bool legacy =
        expected != currentCrc && !previous && expected == legacyCrc;
    if (memcmp(image, magic, sizeof(magic)) != 0 || image[4] != 2U ||
        image[5] != 0U || image[6] != 0U || image[7] != 0U ||
        generation == 0U ||
        (expected != currentCrc && !previous && !legacy)) {
        return false;
    }
    for (uint8_t current = 0U; current < BlobCount; ++current) {
        const uint8_t* item = entry(image, static_cast<BlobId>(current));
        const uint8_t state = item[0];
        const uint16_t size = readU16(item + 2U);
        const size_t limit =
            legacy && current == BlobDeviceAccess
                ? DeviceAccessStore::legacySerializedSize
                : (previous && current == BlobDeviceAccess
                       ? DeviceAccessStore::previousSerializedSize
                       : maximumSize(static_cast<BlobId>(current)));
        if (item[1] != 0U ||
            size > limit ||
            (state == 0U && size != 0U) ||
            (state == 1U && size == 0U) ||
            (state == 2U && size != 0U) || state > 2U) {
            return false;
        }
    }
    if (legacy || previous) {
        // Normalize erased bytes beyond the old image before comparing slots
        // or writing the next current-format snapshot.
        const size_t oldImageSize = legacy
            ? kLegacyImageSize
            : kPreviousImageSize;
        memset(
            image + oldImageSize,
            0,
            kImageSize - oldImageSize);
    }
    return true;
}

uint32_t WioTerminalFlashBlobBank::slotAddress(uint8_t slot) const {
    return owner_.regionAddress(WioTerminalQspiOwner::Region::Identity) +
           static_cast<uint32_t>(slot * kEraseBlockSize);
}

bool WioTerminalFlashBlobBank::storageRegionAvailable() const {
    static const uint8_t magic[4] = {'B', 'W', 'B', '2'};
    bool claimable[2] = {};
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        uint8_t image[kImageSize] = {};
        uint32_t generation = 0U;
        if (!owner_.read(this, slotAddress(slot), image, sizeof(image))) return false;
        claimable[slot] =
            validImage(image, generation) ||
            memcmp(image, magic, sizeof(magic)) == 0 ||
            qspiBlank(owner_, this, slotAddress(slot), kEraseBlockSize);
    }
    // An interrupted inactive-slot update is either erased or already starts
    // with our magic. Refuse unrelated data even when the peer is valid.
    return claimable[0] && claimable[1];
}

Result WioTerminalFlashBlobBank::newest(
    uint8_t* image,
    uint32_t& generation,
    uint8_t& selectedSlot) const {
    generation = 0U;
    selectedSlot = 1U;
    bool valid = false;
    bool present = false;
    uint8_t scratch[kImageSize] = {};
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        if (!owner_.read(this, slotAddress(slot), scratch, sizeof(scratch))) {
            return Result::failure(ErrorCode::InternalError);
        }
        for (size_t i = 0U; i < sizeof(scratch); ++i) {
            if (scratch[i] != 0xffU) { present = true; break; }
        }
        uint32_t candidate = 0U;
        if (!validImage(scratch, candidate)) continue;
        if (valid && candidate == generation) {
            if (memcmp(image, scratch, kImageSize) == 0) continue;
            return Result::failure(ErrorCode::SequenceConflict);
        }
        if (!valid || candidate > generation) {
            memcpy(image, scratch, kImageSize);
            generation = candidate;
            selectedSlot = slot;
            valid = true;
        }
    }
    return valid
               ? Result::success()
               : Result::failure(present ? ErrorCode::InvalidEncoding : ErrorCode::NotFound);
}

Result WioTerminalFlashBlobBank::writeImage(
    uint8_t slot,
    uint32_t generation,
    uint8_t* image) {
    image[0] = 'B';
    image[1] = 'W';
    image[2] = 'B';
    image[3] = '2';
    image[4] = 2U;
    image[5] = 0U;
    image[6] = 0U;
    image[7] = 0U;
    writeU32(image + 8U, generation);
    writeU32(image + 12U, 0U);
    writeU32(image + 12U, computeCrc32(ByteView(image, kImageSize)));

    const uint32_t address = slotAddress(slot);
    Result result = owner_.erase(this, address);
    if (result) result = owner_.program(this, address, image, kImageSize);
    if (!result) return result;
    if (!qspiMatches(owner_, this, address, image, kImageSize)) {
        return Result::failure(ErrorCode::InternalError);
    }
    return Result::success();
}

Result WioTerminalFlashBlobBank::load(
    BlobId id,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    WioTerminalQspiOwner::Operation operation(
        owner_, this, WioTerminalQspiOwner::identityBudgetMillis);
    if (!operation.result()) return operation.result();
    uint8_t image[kImageSize] = {};
    uint32_t generation = 0U;
    uint8_t slot = 0U;
    const Result result = newest(image, generation, slot);
    if (!result) return result;
    const uint8_t* item = entry(image, id);
    if (item[0] != 1U) return Result::failure(ErrorCode::NotFound);
    written = readU16(item + 2U);
    if (output.data == nullptr || output.size < written) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memcpy(output.data, item + kEntryHeaderSize, written);
    return Result::success();
}

Result WioTerminalFlashBlobBank::replace(BlobId id, ByteView value) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (value.data == nullptr || value.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (value.size > maximumSize(id)) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    WioTerminalQspiOwner::Operation operation(
        owner_, this, WioTerminalQspiOwner::identityBudgetMillis);
    if (!operation.result()) return operation.result();
    uint8_t image[kImageSize] = {};
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    const Result found = newest(image, generation, slot);
    if (!found && found.code() != ErrorCode::NotFound) return found;
    if (generation == UINT32_MAX) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    uint8_t* item = entry(image, id);
    memset(item, 0U, kEntryHeaderSize + maximumSize(id));
    item[0] = 1U;
    writeU16(item + 2U, static_cast<uint16_t>(value.size));
    memcpy(item + kEntryHeaderSize, value.data, value.size);
    return writeImage(
        static_cast<uint8_t>(slot ^ 1U),
        generation + 1U,
        image);
}

Result WioTerminalFlashBlobBank::clear(BlobId id) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    WioTerminalQspiOwner::Operation operation(
        owner_, this, WioTerminalQspiOwner::identityBudgetMillis);
    if (!operation.result()) return operation.result();
    uint8_t image[kImageSize] = {};
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    const Result found = newest(image, generation, slot);
    if (!found) return found;
    uint8_t* item = entry(image, id);
    if (item[0] != 1U) return Result::failure(ErrorCode::NotFound);
    if (generation == UINT32_MAX) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    memset(item, 0U, kEntryHeaderSize + maximumSize(id));
    item[0] = 2U;
    return writeImage(
        static_cast<uint8_t>(slot ^ 1U),
        generation + 1U,
        image);
}

} // namespace blinker
