#include "SecureMemory.h"

namespace blinker {

void secureZero(MutableByteSpan memory) {
    if (memory.data == nullptr) return;
    volatile uint8_t* current = memory.data;
    for (size_t index = 0; index < memory.size; ++index) {
        current[index] = 0U;
    }
}

bool constantTimeEqual(ByteView first, ByteView second) {
    if (first.size != second.size ||
        (first.size != 0U &&
         (first.data == nullptr || second.data == nullptr))) {
        return false;
    }
    volatile uint8_t difference = 0U;
    for (size_t index = 0; index < first.size; ++index) {
        difference = static_cast<uint8_t>(
            difference | static_cast<uint8_t>(
                first.data[index] ^ second.data[index]));
    }
    return difference == 0U;
}

} // namespace blinker
