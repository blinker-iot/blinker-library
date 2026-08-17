#include "Utf8.h"

namespace blinker {

bool isValidUtf8(StringView text) {
    if (text.size != 0U && text.data == nullptr) {
        return false;
    }

    size_t index = 0;
    while (index < text.size) {
        const uint8_t first = static_cast<uint8_t>(text.data[index]);
        if (first <= 0x7FU) {
            ++index;
            continue;
        }

        uint32_t codePoint = 0;
        size_t continuationCount = 0;
        uint32_t minimum = 0;
        if ((first & 0xE0U) == 0xC0U) {
            codePoint = first & 0x1FU;
            continuationCount = 1;
            minimum = 0x80U;
        } else if ((first & 0xF0U) == 0xE0U) {
            codePoint = first & 0x0FU;
            continuationCount = 2;
            minimum = 0x800U;
        } else if ((first & 0xF8U) == 0xF0U) {
            codePoint = first & 0x07U;
            continuationCount = 3;
            minimum = 0x10000U;
        } else {
            return false;
        }

        if (continuationCount > text.size - index - 1U) {
            return false;
        }
        for (size_t offset = 1; offset <= continuationCount; ++offset) {
            const uint8_t next =
                static_cast<uint8_t>(text.data[index + offset]);
            if ((next & 0xC0U) != 0x80U) {
                return false;
            }
            codePoint = (codePoint << 6U) | (next & 0x3FU);
        }

        if (codePoint < minimum || codePoint > 0x10FFFFU ||
            (codePoint >= 0xD800U && codePoint <= 0xDFFFU)) {
            return false;
        }
        index += continuationCount + 1U;
    }
    return true;
}

} // namespace blinker
