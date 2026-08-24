#include "Crc32.h"

namespace blinker {

uint32_t computeCrc32(ByteView value) {
    uint32_t crc = 0xFFFFFFFFU;
    if (value.data == nullptr && !value.empty()) return 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        crc ^= value.data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask =
                static_cast<uint32_t>(0U - (crc & 1U));
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

} // namespace blinker
