#ifndef BLINKER_TEST_STORAGE_GATE_COMMON_H
#define BLINKER_TEST_STORAGE_GATE_COMMON_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

namespace storage_gate {

inline uint16_t little16(const uint8_t* value) {
    return static_cast<uint16_t>(value[0] | (static_cast<uint16_t>(value[1]) << 8U));
}
inline uint32_t little32(const uint8_t* value) {
    return static_cast<uint32_t>(value[0]) | (static_cast<uint32_t>(value[1]) << 8U) |
           (static_cast<uint32_t>(value[2]) << 16U) | (static_cast<uint32_t>(value[3]) << 24U);
}
inline uint16_t big16(const uint8_t* value) {
    return static_cast<uint16_t>((static_cast<uint16_t>(value[0]) << 8U) | value[1]);
}
inline uint32_t big32(const uint8_t* value) {
    return (static_cast<uint32_t>(value[0]) << 24U) | (static_cast<uint32_t>(value[1]) << 16U) |
           (static_cast<uint32_t>(value[2]) << 8U) | value[3];
}
inline void putLittle16(uint8_t* value, uint16_t input) {
    value[0] = static_cast<uint8_t>(input); value[1] = static_cast<uint8_t>(input >> 8U);
}
inline void putLittle32(uint8_t* value, uint32_t input) {
    for (uint8_t i = 0U; i < 4U; ++i) value[i] = static_cast<uint8_t>(input >> (8U * i));
}
inline void wipe(uint8_t* bytes, size_t count) {
    volatile uint8_t* destination = bytes;
    while (count-- != 0U) *destination++ = 0U;
}
inline uint32_t crc32(const uint8_t* bytes, size_t count) {
    uint32_t result = 0xffffffffU;
    while (count-- != 0U) {
        result ^= *bytes++;
        for (uint8_t bit = 0U; bit < 8U; ++bit)
            result = (result >> 1U) ^ ((0U - (result & 1U)) & 0xedb88320U);
    }
    return ~result;
}
inline bool instanceAnchor(const uint8_t* bytes, size_t size, uint8_t* identity) {
    if (size != 28U || memcmp(bytes, "BDN2", 4U) != 0 || bytes[4] != 1U ||
        bytes[5] != 0U || bytes[6] != 0U || bytes[7] != 0U ||
        big32(bytes + 24U) != crc32(bytes, 24U)) return false;
    uint8_t bits = 0U;
    for (uint8_t i = 0U; i < 16U; ++i) bits |= bytes[8U + i];
    if (bits == 0U) return false;
    memcpy(identity, bytes + 8U, 16U);
    return true;
}

} // namespace storage_gate
#endif
