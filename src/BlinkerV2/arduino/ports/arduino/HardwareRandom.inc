#include "HardwareRandom.h" // Shared by supported Arduino platform ports.

#if defined(ARDUINO_ARCH_ESP32)

#include <esp_random.h>

namespace blinker {

Result PlatformHardwareRandom::fill(MutableByteSpan output) {
    if (output.data == nullptr && output.size != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.size != 0U) esp_fill_random(output.data, output.size);
    return Result::success();
}

} // namespace blinker

#elif defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)

#include <Arduino.h>
#include <BlinkerV2/core/SecureMemory.h>

#include <string.h>

namespace blinker {

Result PlatformHardwareRandom::fill(MutableByteSpan output) {
    if (output.data == nullptr && output.size != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const bool clockWasEnabled =
        (MCLK->APBCMASK.reg & MCLK_APBCMASK_TRNG) != 0U;
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TRNG;
    const uint8_t control = TRNG->CTRLA.reg;
    TRNG->CTRLA.reg = TRNG_CTRLA_ENABLE;

    size_t offset = 0U;
    while (offset < output.size) {
        uint32_t remainingPolls = 1000000U;
        while ((TRNG->INTFLAG.reg & TRNG_INTFLAG_DATARDY) == 0U &&
               remainingPolls != 0U) {
            --remainingPolls;
        }
        if (remainingPolls == 0U) {
            TRNG->CTRLA.reg = control;
            if (!clockWasEnabled) {
                MCLK->APBCMASK.reg &= ~MCLK_APBCMASK_TRNG;
            }
            return Result::failure(ErrorCode::InternalError);
        }
        uint32_t word = TRNG->DATA.reg;
        const size_t remaining = output.size - offset;
        const size_t copied = remaining < sizeof(word)
                                  ? remaining
                                  : sizeof(word);
        memcpy(output.data + offset, &word, copied);
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(&word), sizeof(word)));
        offset += copied;
    }
    TRNG->CTRLA.reg = control;
    if (!clockWasEnabled) {
        MCLK->APBCMASK.reg &= ~MCLK_APBCMASK_TRNG;
    }
    return Result::success();
}

} // namespace blinker

#elif defined(ARDUINO_ARCH_RENESAS_UNO)

#include <BlinkerV2/core/SecureMemory.h>

#include <string.h>

extern "C" {
#include "common_data.h"
fsp_err_t HW_SCE_McuSpecificInit(void);
fsp_err_t HW_SCE_RNG_Read(uint32_t* output);
}

namespace blinker {

namespace {

bool sceInitialized = false;

} // namespace

Result PlatformHardwareRandom::fill(MutableByteSpan output) {
    if (output.data == nullptr && output.size != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (!sceInitialized) {
        if (HW_SCE_McuSpecificInit() != FSP_SUCCESS) {
            return Result::failure(ErrorCode::InternalError);
        }
        sceInitialized = true;
    }
    size_t offset = 0U;
    while (offset < output.size) {
        uint32_t words[4] = {};
        if (HW_SCE_RNG_Read(words) != FSP_SUCCESS) {
            secureZero(MutableByteSpan(
                reinterpret_cast<uint8_t*>(words),
                sizeof(words)));
            return Result::failure(ErrorCode::InternalError);
        }
        const size_t remaining = output.size - offset;
        const size_t copied = remaining < sizeof(words)
                                  ? remaining
                                  : sizeof(words);
        memcpy(output.data + offset, words, copied);
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(words),
            sizeof(words)));
        offset += copied;
    }
    return Result::success();
}

} // namespace blinker

#endif
