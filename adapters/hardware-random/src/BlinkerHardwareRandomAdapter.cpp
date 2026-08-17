#include "BlinkerHardwareRandomAdapter.h"

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
