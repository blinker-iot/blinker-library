#if defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)

#include "WioTerminalStorage.h"

#include <BlinkerV2/core/Crc32.h>
#include <limits.h>
#include <string.h>
#include <wiring_private.h>

/*
 * The SAMD51 QSPI transaction flow is adapted from Adafruit_SPIFlash.
 * Copyright (c) 2019 Ha Thach and Dean Miller for Adafruit Industries LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

namespace {

enum : uint8_t {
    kCommandRead = 0x03U,
    kCommandPageProgram = 0x02U,
    kCommandReadStatus = 0x05U,
    kCommandWriteEnable = 0x06U,
    kCommandWriteDisable = 0x04U,
    kCommandSectorErase = 0x20U,
    kCommandReadJedec = 0x9fU,
    kCommandEnableReset = 0x66U,
    kCommandReset = 0x99U
};

enum : uint32_t {
    kExpectedCapacity = 4UL * 1024UL * 1024UL,
    kPageSize = 256U,
    kSectorSize = 4096U,
    kInstructionTimeoutMs = 100U,
    kReadyTimeoutMs = 1000U,
    kEraseTimeoutMs = 10000U
};

uint32_t qspiCapacity = 0U;
bool qspiOpened = false;

bool elapsed(uint32_t started, uint32_t timeout) {
    return static_cast<uint32_t>(millis() - started) >= timeout;
}

bool disableCache() {
    const bool enabled = CMCC->SR.bit.CSTS != 0U;
    if (enabled) {
        CMCC->CTRL.bit.CEN = 0U;
        while (CMCC->SR.bit.CSTS != 0U) {}
    }
    CMCC->MAINT0.bit.INVALL = 1U;
    return enabled;
}

void restoreCache(bool enabled) {
    if (enabled) CMCC->CTRL.bit.CEN = 1U;
}

bool runInstruction(
    uint8_t command,
    uint32_t frame,
    uint32_t address,
    uint8_t* buffer,
    size_t size) {
    const bool restore = buffer != nullptr && size != 0U
                             ? disableCache()
                             : false;

    if ((frame & QSPI_INSTRFRAME_ADDREN) != 0U &&
        (buffer == nullptr || size == 0U)) {
        QSPI->INSTRADDR.reg = address;
    }
    QSPI->INTFLAG.reg = QSPI_INTFLAG_INSTREND;
    QSPI->INSTRCTRL.bit.INSTR = command;
    QSPI->INSTRFRAME.reg = frame;
    volatile uint32_t synchronizedFrame = QSPI->INSTRFRAME.reg;
    (void)synchronizedFrame;

    if (buffer != nullptr && size != 0U) {
        uint8_t* qspiMemory =
            reinterpret_cast<uint8_t*>(QSPI_AHB + address);
        const uint32_t transfer = frame & QSPI_INSTRFRAME_TFRTYPE_Msk;
        if (transfer == QSPI_INSTRFRAME_TFRTYPE_READ ||
            transfer == QSPI_INSTRFRAME_TFRTYPE_READMEMORY) {
            memcpy(buffer, qspiMemory, size);
        } else {
            memcpy(qspiMemory, buffer, size);
        }
    }

    QSPI->CTRLA.reg = QSPI_CTRLA_ENABLE | QSPI_CTRLA_LASTXFER;
    const uint32_t started = millis();
    while (QSPI->INTFLAG.bit.INSTREND == 0U) {
        if (elapsed(started, kInstructionTimeoutMs)) {
            restoreCache(restore);
            return false;
        }
        yield();
    }
    QSPI->INTFLAG.reg = QSPI_INTFLAG_INSTREND;
    restoreCache(restore);
    return true;
}

bool runCommand(uint8_t command) {
    const uint32_t frame = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
                           QSPI_INSTRFRAME_ADDRLEN_24BITS |
                           QSPI_INSTRFRAME_TFRTYPE_READ |
                           QSPI_INSTRFRAME_INSTREN;
    return runInstruction(command, frame, 0U, nullptr, 0U);
}

bool readCommand(uint8_t command, uint8_t* output, size_t size) {
    const uint32_t frame = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
                           QSPI_INSTRFRAME_ADDRLEN_24BITS |
                           QSPI_INSTRFRAME_TFRTYPE_READ |
                           QSPI_INSTRFRAME_INSTREN |
                           QSPI_INSTRFRAME_DATAEN;
    return runInstruction(command, frame, 0U, output, size);
}

bool readMemory(uint32_t address, uint8_t* output, size_t size) {
    const uint32_t frame = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
                           QSPI_INSTRFRAME_ADDRLEN_24BITS |
                           QSPI_INSTRFRAME_TFRTYPE_READMEMORY |
                           QSPI_INSTRFRAME_INSTREN |
                           QSPI_INSTRFRAME_ADDREN |
                           QSPI_INSTRFRAME_DATAEN;
    return runInstruction(kCommandRead, frame, address, output, size);
}

bool writeMemory(uint32_t address, const uint8_t* input, size_t size) {
    const uint32_t frame = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
                           QSPI_INSTRFRAME_ADDRLEN_24BITS |
                           QSPI_INSTRFRAME_TFRTYPE_WRITEMEMORY |
                           QSPI_INSTRFRAME_INSTREN |
                           QSPI_INSTRFRAME_ADDREN |
                           QSPI_INSTRFRAME_DATAEN;
    return runInstruction(
        kCommandPageProgram,
        frame,
        address,
        const_cast<uint8_t*>(input),
        size);
}

bool eraseSectorCommand(uint32_t address) {
    const uint32_t frame = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
                           QSPI_INSTRFRAME_ADDRLEN_24BITS |
                           QSPI_INSTRFRAME_TFRTYPE_WRITE |
                           QSPI_INSTRFRAME_INSTREN |
                           QSPI_INSTRFRAME_ADDREN;
    return runInstruction(
        kCommandSectorErase,
        frame,
        address,
        nullptr,
        0U);
}

bool waitReady(uint32_t timeout) {
    const uint32_t started = millis();
    uint8_t status = 0xffU;
    do {
        if (!readCommand(kCommandReadStatus, &status, sizeof(status))) {
            return false;
        }
        if ((status & 0x01U) == 0U) return true;
        yield();
    } while (!elapsed(started, timeout));
    return false;
}

bool enableWrite() {
    uint8_t status = 0U;
    return runCommand(kCommandWriteEnable) &&
           readCommand(kCommandReadStatus, &status, sizeof(status)) &&
           (status & 0x02U) != 0U;
}

bool beginQspi(uint32_t& capacity) {
    if (qspiOpened) {
        capacity = qspiCapacity;
        return true;
    }

    MCLK->APBCMASK.bit.QSPI_ = true;
    MCLK->AHBMASK.bit.QSPI_ = true;
    MCLK->AHBMASK.bit.QSPI_2X_ = false;

    QSPI->CTRLA.bit.SWRST = 1U;
    const uint32_t resetStarted = millis();
    while (QSPI->CTRLA.bit.SWRST != 0U) {
        if (elapsed(resetStarted, kInstructionTimeoutMs)) return false;
    }

    pinPeripheral(PIN_QSPI_SCK, PIO_COM);
    pinPeripheral(PIN_QSPI_CS, PIO_COM);
    pinPeripheral(PIN_QSPI_IO0, PIO_COM);
    pinPeripheral(PIN_QSPI_IO1, PIO_COM);
    pinPeripheral(PIN_QSPI_IO2, PIO_COM);
    pinPeripheral(PIN_QSPI_IO3, PIO_COM);

    QSPI->BAUD.reg = QSPI_BAUD_BAUD(VARIANT_MCK / 4000000UL);
    QSPI->CTRLB.reg = QSPI_CTRLB_MODE_MEMORY |
                      QSPI_CTRLB_DATALEN_8BITS |
                      QSPI_CTRLB_CSMODE_LASTXFER;
    QSPI->CTRLA.bit.ENABLE = 1U;

    if (!waitReady(kReadyTimeoutMs) ||
        !runCommand(kCommandEnableReset) ||
        !runCommand(kCommandReset)) {
        return false;
    }
    delayMicroseconds(30U);

    uint8_t jedec[3] = {};
    if (!readCommand(kCommandReadJedec, jedec, sizeof(jedec)) ||
        jedec[0] == 0U || jedec[0] == 0xffU || jedec[2] != 0x16U ||
        !runCommand(kCommandWriteDisable) ||
        !waitReady(kReadyTimeoutMs)) {
        return false;
    }

    qspiCapacity = kExpectedCapacity;
    qspiOpened = true;
    capacity = qspiCapacity;
    return true;
}

void endQspi() {
    if (qspiOpened) runCommand(kCommandWriteDisable);
    QSPI->CTRLA.bit.ENABLE = 0U;
    MCLK->APBCMASK.bit.QSPI_ = false;
    MCLK->AHBMASK.bit.QSPI_ = false;
    MCLK->AHBMASK.bit.QSPI_2X_ = false;
    qspiOpened = false;
    qspiCapacity = 0U;
}

bool readQspi(uint32_t address, uint8_t* output, size_t size) {
    if (!qspiOpened || output == nullptr || size == 0U ||
        address > qspiCapacity || size > qspiCapacity - address) {
        return false;
    }
    return waitReady(kReadyTimeoutMs) &&
           readMemory(address, output, size);
}

bool eraseQspiSector(uint32_t address) {
    if (!qspiOpened || (address % kSectorSize) != 0U ||
        address > qspiCapacity - kSectorSize) {
        return false;
    }
    return waitReady(kReadyTimeoutMs) && enableWrite() &&
           eraseSectorCommand(address) && waitReady(kEraseTimeoutMs);
}

bool programQspi(uint32_t address, const uint8_t* input, size_t size) {
    if (!qspiOpened || input == nullptr || size == 0U ||
        address > qspiCapacity || size > qspiCapacity - address) {
        return false;
    }

    size_t remaining = size;
    while (remaining != 0U) {
        const size_t pageRemaining =
            kPageSize - static_cast<size_t>(address % kPageSize);
        const size_t written = remaining < pageRemaining
                                   ? remaining
                                   : pageRemaining;
        if (!waitReady(kReadyTimeoutMs) || !enableWrite() ||
            !writeMemory(address, input, written) ||
            !waitReady(kReadyTimeoutMs)) {
            return false;
        }
        address += static_cast<uint32_t>(written);
        input += written;
        remaining -= written;
    }
    return true;
}

bool qspiMatches(uint32_t address, const uint8_t* expected, size_t size) {
    uint8_t actual[64];
    while (size != 0U) {
        const size_t checked = size < sizeof(actual) ? size : sizeof(actual);
        if (!readQspi(address, actual, checked) ||
            memcmp(actual, expected, checked) != 0) {
            return false;
        }
        address += static_cast<uint32_t>(checked);
        expected += checked;
        size -= checked;
    }
    return true;
}

bool qspiBlank(uint32_t address, size_t size) {
    uint8_t bytes[64];
    while (size != 0U) {
        const size_t checked = size < sizeof(bytes) ? size : sizeof(bytes);
        if (!readQspi(address, bytes, checked)) return false;
        for (size_t index = 0U; index < checked; ++index) {
            if (bytes[index] != 0xffU) return false;
        }
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

WioTerminalFlashBlobBank::WioTerminalFlashBlobBank()
    : deviceInstance_(*this, BlobDeviceInstance),
      wifiSelector_(*this, BlobWifiSelector),
      wifiSlot0_(*this, BlobWifiSlot0),
      wifiSlot1_(*this, BlobWifiSlot1),
      deviceAccess_(*this, BlobDeviceAccess),
      flashSize_(0U),
      opened_(false) {}

Result WioTerminalFlashBlobBank::begin() {
    if (opened_) return Result::success();
    if (!beginQspi(flashSize_) ||
        flashSize_ < kEraseBlockSize * 2U) {
        endQspi();
        flashSize_ = 0U;
        return Result::failure(ErrorCode::InternalError);
    }
    if (!storageRegionAvailable()) {
        endQspi();
        flashSize_ = 0U;
        return Result::failure(ErrorCode::StateConflict);
    }
    opened_ = true;
    return Result::success();
}

void WioTerminalFlashBlobBank::end() {
    opened_ = false;
    flashSize_ = 0U;
    endQspi();
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
    return flashSize_ -
           static_cast<uint32_t>((2U - slot) * kEraseBlockSize);
}

bool WioTerminalFlashBlobBank::storageRegionAvailable() const {
    static const uint8_t magic[4] = {'B', 'W', 'B', '2'};
    bool claimable[2] = {};
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        uint8_t image[kImageSize] = {};
        uint32_t generation = 0U;
        if (!readQspi(slotAddress(slot), image, sizeof(image))) return false;
        claimable[slot] =
            validImage(image, generation) ||
            memcmp(image, magic, sizeof(magic)) == 0 ||
            qspiBlank(slotAddress(slot), kEraseBlockSize);
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
    uint8_t scratch[kImageSize] = {};
    for (uint8_t slot = 0U; slot < 2U; ++slot) {
        if (!readQspi(slotAddress(slot), scratch, sizeof(scratch))) {
            return Result::failure(ErrorCode::InternalError);
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
               : Result::failure(ErrorCode::NotFound);
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
    if (!eraseQspiSector(address) ||
        !programQspi(address, image, kImageSize) ||
        !qspiMatches(address, image, kImageSize)) {
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

#endif
