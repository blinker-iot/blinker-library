#ifndef BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_STORAGE_H
#define BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_STORAGE_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalStorage requires a Wio Terminal target"
#endif

#include <Arduino.h>
#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/identity/ControllerCredentialStore.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/identity/OwnershipRecordStore.h>
#include <BlinkerV2/interface/IAtomicBlobStore.h>
#include <BlinkerV2/provisioning/LocalSetupSagaStore.h>
#include <BlinkerV2/provisioning/OwnershipClaimRecordStore.h>
#include <BlinkerV2/provisioning/WifiCredentialStore.h>

#include <limits.h>
#include <string.h>

namespace blinker {

// SAMD51 erases main flash in 8 KiB blocks. All small Device V2 records share
// one fixed-layout snapshot, alternating between two independent blocks.
// Updating a field writes and verifies the inactive block, so reset or power
// loss leaves either the prior or the complete new snapshot readable.
class WioTerminalFlashBlobBank {
public:
    WioTerminalFlashBlobBank();

    Result begin() {
        const uintptr_t first = reinterpret_cast<uintptr_t>(bank0_);
        const uintptr_t second = reinterpret_cast<uintptr_t>(bank1_);
        const uintptr_t flashSize =
            (static_cast<uintptr_t>(8U) << NVMCTRL->PARAM.bit.PSZ) *
            NVMCTRL->PARAM.bit.NVMP;
        if ((first % kEraseBlockSize) != 0U ||
            (second % kEraseBlockSize) != 0U || first == second ||
            first + kEraseBlockSize > flashSize ||
            second + kEraseBlockSize > flashSize) {
            return Result::failure(ErrorCode::InternalError);
        }
        opened_ = true;
        return Result::success();
    }
    void end() { opened_ = false; }

    IAtomicBlobStore& deviceInstanceBlob() { return deviceInstance_; }
    IAtomicBlobStore& wifiSelectorBlob() { return wifiSelector_; }
    IAtomicBlobStore& wifiSlot0Blob() { return wifiSlot0_; }
    IAtomicBlobStore& wifiSlot1Blob() { return wifiSlot1_; }
    IAtomicBlobStore& ownershipBlob() { return ownership_; }
    IAtomicBlobStore& ownershipClaimBlob() { return ownershipClaim_; }
    IAtomicBlobStore& controllerBlob() { return controller_; }
    IAtomicBlobStore& localSetupSagaBlob() { return localSetupSaga_; }

private:
    enum BlobId : uint8_t {
        BlobDeviceInstance = 0U,
        BlobWifiSelector,
        BlobWifiSlot0,
        BlobWifiSlot1,
        BlobOwnership,
        BlobOwnershipClaim,
        BlobController,
        BlobLocalSetupSaga,
        BlobCount
    };

    class BlobView final : public IAtomicBlobStore {
    public:
        BlobView(WioTerminalFlashBlobBank& bank, BlobId id)
            : bank_(bank), id_(id) {}

        Result load(MutableByteSpan output, size_t& written) override {
            return bank_.load(id_, output, written);
        }
        Result replace(ByteView value) override {
            return bank_.replace(id_, value);
        }
        Result clear() override { return bank_.clear(id_); }
        StorageProtection protection() const override {
            return StorageProtection::PlainFlash;
        }

    private:
        WioTerminalFlashBlobBank& bank_;
        BlobId id_;
    };

    enum : size_t {
        kHeaderSize = 16U,
        kEntryHeaderSize = 4U,
        kEraseBlockSize = 8192U,
        kPageSize = 512U,
        kImageSize =
            kHeaderSize + kEntryHeaderSize * BlobCount +
            DeviceInstanceIdStore::serializedSize +
            WifiCredentialStore::serializedSelectorSize +
            WifiCredentialStore::serializedProfileSize * 2U +
            OwnershipRecordStore::serializedSize +
            OwnershipClaimRecordStore::serializedSize +
            ControllerCredentialStore::serializedSize +
            LocalSetupSagaStore::serializedSize
    };

    static_assert(
        kImageSize <= kEraseBlockSize,
        "Wio Terminal Device V2 snapshot exceeds one erase block");

    static size_t maximumSize(BlobId id);
    static size_t entryOffset(BlobId id);
    static uint8_t* entry(uint8_t* image, BlobId id) {
        return image + entryOffset(id);
    }
    static const uint8_t* entry(const uint8_t* image, BlobId id) {
        return image + entryOffset(id);
    }
    static const uint8_t* address(uint8_t slot) {
        return slot == 0U ? bank0_ : bank1_;
    }
    static bool validImage(uint8_t* image, uint32_t& generation);
    Result newest(
        uint8_t* image,
        uint32_t& generation,
        uint8_t& slot) const;
    static bool eraseBlock(uint8_t slot);
    static bool programImage(uint8_t slot, const uint8_t* image);
    static Result writeImage(
        uint8_t slot,
        uint32_t generation,
        uint8_t* image);
    Result load(BlobId id, MutableByteSpan output, size_t& written);
    Result replace(BlobId id, ByteView value);
    Result clear(BlobId id);

    BlobView deviceInstance_;
    BlobView wifiSelector_;
    BlobView wifiSlot0_;
    BlobView wifiSlot1_;
    BlobView ownership_;
    BlobView ownershipClaim_;
    BlobView controller_;
    BlobView localSetupSaga_;
    bool opened_;

    __attribute__((aligned(kEraseBlockSize)))
    static const uint8_t bank0_[kEraseBlockSize];
    __attribute__((aligned(kEraseBlockSize)))
    static const uint8_t bank1_[kEraseBlockSize];

    WioTerminalFlashBlobBank(const WioTerminalFlashBlobBank&);
    WioTerminalFlashBlobBank& operator=(
        const WioTerminalFlashBlobBank&);
};

inline void wioWriteU16(uint8_t* output, uint16_t value) {
    output[0] = static_cast<uint8_t>(value >> 8U);
    output[1] = static_cast<uint8_t>(value);
}

inline uint16_t wioReadU16(const uint8_t* input) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(input[0]) << 8U) |
        static_cast<uint16_t>(input[1]));
}

inline void wioWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

inline uint32_t wioReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

inline WioTerminalFlashBlobBank::WioTerminalFlashBlobBank()
    : deviceInstance_(*this, BlobDeviceInstance),
      wifiSelector_(*this, BlobWifiSelector),
      wifiSlot0_(*this, BlobWifiSlot0),
      wifiSlot1_(*this, BlobWifiSlot1),
      ownership_(*this, BlobOwnership),
      ownershipClaim_(*this, BlobOwnershipClaim),
      controller_(*this, BlobController),
      localSetupSaga_(*this, BlobLocalSetupSaga),
      opened_(false) {}

inline size_t WioTerminalFlashBlobBank::maximumSize(BlobId id) {
    switch (id) {
    case BlobDeviceInstance:
        return DeviceInstanceIdStore::serializedSize;
    case BlobWifiSelector:
        return WifiCredentialStore::serializedSelectorSize;
    case BlobWifiSlot0:
    case BlobWifiSlot1:
        return WifiCredentialStore::serializedProfileSize;
    case BlobOwnership:
        return OwnershipRecordStore::serializedSize;
    case BlobOwnershipClaim:
        return OwnershipClaimRecordStore::serializedSize;
    case BlobController:
        return ControllerCredentialStore::serializedSize;
    case BlobLocalSetupSaga:
        return LocalSetupSagaStore::serializedSize;
    default:
        return 0U;
    }
}

inline size_t WioTerminalFlashBlobBank::entryOffset(BlobId id) {
    size_t result = kHeaderSize;
    for (uint8_t current = 0U; current < static_cast<uint8_t>(id);
         ++current) {
        result += kEntryHeaderSize +
                  maximumSize(static_cast<BlobId>(current));
    }
    return result;
}

inline bool WioTerminalFlashBlobBank::validImage(
    uint8_t* image,
    uint32_t& generation) {
    static const uint8_t magic[4] = {'B', 'W', 'B', '2'};
    generation = wioReadU32(image + 8U);
    const uint32_t expected = wioReadU32(image + 12U);
    wioWriteU32(image + 12U, 0U);
    const uint32_t actual = computeCrc32(ByteView(image, kImageSize));
    wioWriteU32(image + 12U, expected);
    if (memcmp(image, magic, sizeof(magic)) != 0 || image[4] != 1U ||
        image[5] != 0U || image[6] != 0U || image[7] != 0U ||
        generation == 0U || expected != actual) {
        return false;
    }
    for (uint8_t current = 0U; current < BlobCount; ++current) {
        const uint8_t* item = entry(image, static_cast<BlobId>(current));
        const uint8_t state = item[0];
        const uint16_t size = wioReadU16(item + 2U);
        if (item[1] != 0U ||
            size > maximumSize(static_cast<BlobId>(current)) ||
            (state == 0U && size != 0U) ||
            (state == 1U && size == 0U) ||
            (state == 2U && size != 0U) || state > 2U) {
            return false;
        }
    }
    return true;
}

inline Result WioTerminalFlashBlobBank::newest(
    uint8_t* image,
    uint32_t& generation,
    uint8_t& selectedSlot) const {
    generation = 0U;
    selectedSlot = 1U;
    bool valid = false;
    uint8_t scratch[kImageSize] = {};
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        memcpy(scratch, address(slot), kImageSize);
        uint32_t candidate = 0U;
        if (!validImage(scratch, candidate)) continue;
        if (valid && candidate == generation) {
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
               : Result::failure(ErrorCode::NotFound);
}

inline bool WioTerminalFlashBlobBank::eraseBlock(uint8_t slot) {
    while ((NVMCTRL->STATUS.reg & NVMCTRL_STATUS_READY) == 0U) {}
    NVMCTRL->INTFLAG.reg = NVMCTRL_INTFLAG_DONE |
                           NVMCTRL_INTFLAG_ADDRE |
                           NVMCTRL_INTFLAG_PROGE |
                           NVMCTRL_INTFLAG_LOCKE |
                           NVMCTRL_INTFLAG_NVME;
    NVMCTRL->ADDR.reg = reinterpret_cast<uint32_t>(address(slot));
    NVMCTRL->CTRLB.reg =
        NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB;
    while ((NVMCTRL->STATUS.reg & NVMCTRL_STATUS_READY) == 0U) {}
    const uint16_t errors = NVMCTRL->INTFLAG.reg &
        (NVMCTRL_INTFLAG_ADDRE | NVMCTRL_INTFLAG_PROGE |
         NVMCTRL_INTFLAG_LOCKE | NVMCTRL_INTFLAG_NVME);
    return errors == 0U;
}

inline bool WioTerminalFlashBlobBank::programImage(
    uint8_t slot,
    const uint8_t* image) {
    NVMCTRL->CTRLA.bit.WMODE = 0U;
    uint8_t page[kPageSize];
    size_t offset = 0U;
    while (offset < kImageSize) {
        memset(page, 0xff, sizeof(page));
        const size_t remaining = kImageSize - offset;
        const size_t copied = remaining < sizeof(page)
                                  ? remaining
                                  : sizeof(page);
        memcpy(page, image + offset, copied);
        while ((NVMCTRL->STATUS.reg & NVMCTRL_STATUS_READY) == 0U) {}
        NVMCTRL->INTFLAG.reg = NVMCTRL_INTFLAG_DONE |
                               NVMCTRL_INTFLAG_ADDRE |
                               NVMCTRL_INTFLAG_PROGE |
                               NVMCTRL_INTFLAG_LOCKE |
                               NVMCTRL_INTFLAG_NVME;
        NVMCTRL->CTRLB.reg =
            NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;
        while ((NVMCTRL->STATUS.reg & NVMCTRL_STATUS_READY) == 0U) {}

        volatile uint32_t* destination =
            reinterpret_cast<volatile uint32_t*>(
                const_cast<uint8_t*>(address(slot)) + offset);
        for (size_t word = 0U; word < kPageSize / sizeof(uint32_t);
             ++word) {
            uint32_t value = 0U;
            memcpy(&value, page + word * sizeof(uint32_t), sizeof(value));
            destination[word] = value;
        }
        NVMCTRL->CTRLB.reg =
            NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WP;
        while ((NVMCTRL->STATUS.reg & NVMCTRL_STATUS_READY) == 0U) {}
        const uint16_t errors = NVMCTRL->INTFLAG.reg &
            (NVMCTRL_INTFLAG_ADDRE | NVMCTRL_INTFLAG_PROGE |
             NVMCTRL_INTFLAG_LOCKE | NVMCTRL_INTFLAG_NVME);
        if (errors != 0U) return false;
        offset += copied;
    }
    return true;
}

inline Result WioTerminalFlashBlobBank::writeImage(
    uint8_t slot,
    uint32_t generation,
    uint8_t* image) {
    image[0] = 'B';
    image[1] = 'W';
    image[2] = 'B';
    image[3] = '2';
    image[4] = 1U;
    image[5] = 0U;
    image[6] = 0U;
    image[7] = 0U;
    wioWriteU32(image + 8U, generation);
    wioWriteU32(image + 12U, 0U);
    wioWriteU32(image + 12U, computeCrc32(ByteView(image, kImageSize)));

    const uint32_t interruptState = __get_PRIMASK();
    __disable_irq();
    const bool written = eraseBlock(slot) && programImage(slot, image);
    if (interruptState == 0U) __enable_irq();
    if (!written || memcmp(address(slot), image, kImageSize) != 0) {
        return Result::failure(ErrorCode::InternalError);
    }
    return Result::success();
}

inline Result WioTerminalFlashBlobBank::load(
    BlobId id,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    uint8_t image[kImageSize] = {};
    uint32_t generation = 0U;
    uint8_t slot = 0U;
    Result result = newest(image, generation, slot);
    if (!result) return result;
    const uint8_t* item = entry(image, id);
    if (item[0] != 1U) return Result::failure(ErrorCode::NotFound);
    written = wioReadU16(item + 2U);
    if (output.data == nullptr || output.size < written) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memcpy(output.data, item + kEntryHeaderSize, written);
    return Result::success();
}

inline Result WioTerminalFlashBlobBank::replace(
    BlobId id,
    ByteView value) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (value.data == nullptr || value.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (value.size > maximumSize(id)) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
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
    wioWriteU16(item + 2U, static_cast<uint16_t>(value.size));
    memcpy(item + kEntryHeaderSize, value.data, value.size);
    return writeImage(
        static_cast<uint8_t>(slot ^ 1U),
        generation + 1U,
        image);
}

inline Result WioTerminalFlashBlobBank::clear(BlobId id) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    uint8_t image[kImageSize] = {};
    uint32_t generation = 0U;
    uint8_t slot = 1U;
    Result result = newest(image, generation, slot);
    if (!result) return result;
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

#endif
