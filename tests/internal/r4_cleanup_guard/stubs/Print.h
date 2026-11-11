#ifndef BLINKER_TEST_R4_CLEANUP_PRINT_H
#define BLINKER_TEST_R4_CLEANUP_PRINT_H
#include "Arduino.h"
class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t* bytes, size_t size) {
        size_t count = 0U;
        while (count < size && write(bytes[count]) == 1U) ++count;
        return count;
    }
    size_t write(const char* text) {
        return text == nullptr ? 0U : write(reinterpret_cast<const uint8_t*>(text), std::strlen(text));
    }
};
#endif
