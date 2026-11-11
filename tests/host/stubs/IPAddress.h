#ifndef BLINKER_TEST_STUB_IP_ADDRESS_H
#define BLINKER_TEST_STUB_IP_ADDRESS_H

#include <stdint.h>

class IPAddress {
public:
    IPAddress() : bytes_{0U, 0U, 0U, 0U} {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : bytes_{a, b, c, d} {}

    uint8_t operator[](uint8_t index) const { return bytes_[index]; }

private:
    uint8_t bytes_[4U];
};

#endif
