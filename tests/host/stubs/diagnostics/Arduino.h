#ifndef BLINKER_TEST_DIAGNOSTICS_ARDUINO_H
#define BLINKER_TEST_DIAGNOSTICS_ARDUINO_H

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

class __FlashStringHelper;
#define F(value) reinterpret_cast<const __FlashStringHelper*>(value)

extern uint32_t diagnosticTestMillis;
inline uint32_t millis() { return diagnosticTestMillis; }

// Only the Print surface used by the production formatter. Device builds use
// the SDK Print implementation; this fake makes byte and bulk I/O observable.
class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t byte) = 0;
    virtual size_t write(const uint8_t* bytes, size_t size) {
        size_t count = 0U;
        for (size_t i = 0U; i < size; ++i) count += write(bytes[i]);
        return count;
    }
    size_t print(const char* value) {
        size_t count = 0U;
        while (*value != '\0') count += write(static_cast<uint8_t>(*value++));
        return count;
    }
    size_t print(const __FlashStringHelper* value) {
        return print(reinterpret_cast<const char*>(value));
    }
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, size_t>::type print(T value) {
        uint64_t number = static_cast<uint64_t>(value);
        char digits[21];
        size_t end = sizeof(digits) - 1U;
        digits[end] = '\0';
        do {
            digits[--end] = static_cast<char>('0' + number % 10U);
            number /= 10U;
        } while (number != 0U);
        return print(digits + end);
    }
    size_t println() { return print("\r\n"); }
    template <typename T> size_t println(T value) {
        return print(value) + println();
    }
};

#endif
