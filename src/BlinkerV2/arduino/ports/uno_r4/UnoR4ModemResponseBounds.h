#ifndef BLINKER_UNO_R4_MODEM_RESPONSE_BOUNDS_H
#define BLINKER_UNO_R4_MODEM_RESPONSE_BOUNDS_H

#include <stddef.h>
#include <limits.h>

namespace blinker {

// Pure pre-append checks for an opt-in SDK source overlay, not an allocator,
// parser, SDK hook or production policy. A bounded string can still fail new().
inline bool unoR4ModemAppendAllowed(size_t length, size_t maximum) {
    return maximum != 0U && length < maximum;
}

inline bool unoR4ModemSizedLength(const char* input, size_t length,
    size_t maximum, unsigned int& output) {
    output = 0U;
    if (input == nullptr || length == 0U || maximum > UINT_MAX) return false;
    // The SDK preserves the response's leading ASCII spaces when PREFGET
    // disables trimming. Skip only that framing prefix, not binary payload,
    // signs, trailing whitespace or other atoi-compatible garbage.
    size_t first = 0U;
    while (first < length && input[first] == ' ') ++first;
    if (first == length) return false;
    unsigned int value = 0U;
    for (size_t i = first; i < length; ++i) {
        if (input[i] < '0' || input[i] > '9') return false;
        const unsigned int digit = static_cast<unsigned int>(input[i] - '0');
        if (digit > maximum || value > (maximum - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    output = value;
    return true;
}

} // namespace blinker
#endif
