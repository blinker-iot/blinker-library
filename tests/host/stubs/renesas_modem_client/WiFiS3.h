#ifndef BLINKER_TEST_RENESAS_MODEM_CLIENT_WIFI_S3_H
#define BLINKER_TEST_RENESAS_MODEM_CLIENT_WIFI_S3_H

#include <stddef.h>
#include <stdint.h>

// The owner/client tests exercise dispatch and admission, not the SDK parser.
namespace arduino { enum IPType { IPv4, IPv6 }; }
class IPAddress {
public:
    explicit IPAddress(uint32_t value = 0U) : value_(value) {}
    uint32_t value() const { return value_; }
    arduino::IPType type() const { return arduino::IPv4; }
    uint8_t operator[](size_t index) const { return static_cast<uint8_t>(value_ >> (8U * index)); }
private:
    uint32_t value_;
};

#endif
