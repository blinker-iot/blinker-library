#include "WioTerminalQspi.h"

#if defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)
#include <Arduino.h>
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


class NativeWioTerminalQspiFlash final : public blinker::IWioTerminalQspiFlash {
public:
    bool begin(uint32_t& capacity) override { return beginQspi(capacity); }
    void end() override { endQspi(); }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        return readQspi(address, output, size);
    }
    bool erase(uint32_t address) override { return eraseQspiSector(address); }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        return programQspi(address, input, size);
    }
};
uint32_t wioQspiNow() { return static_cast<uint32_t>(millis()); }
} // namespace

namespace blinker {
WioTerminalQspiOwner& wioTerminalQspiOwner() {
    static NativeWioTerminalQspiFlash flash;
    static WioTerminalQspiOwner owner(flash, &wioQspiNow);
    return owner;
}
} // namespace blinker
#endif

#include "WioTerminalQspiOwner.ipp"
