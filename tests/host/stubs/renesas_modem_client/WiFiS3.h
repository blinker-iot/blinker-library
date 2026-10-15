#ifndef BLINKER_TEST_RENESAS_MODEM_CLIENT_WIFI_S3_H
#define BLINKER_TEST_RENESAS_MODEM_CLIENT_WIFI_S3_H

#include <stddef.h>
#include <stdint.h>

// The owner/client tests exercise dispatch and admission, not the SDK parser.
class IPAddress {
public:
    explicit IPAddress(uint32_t value = 0U) : value_(value) {}
    uint32_t value() const { return value_; }
private:
    uint32_t value_;
};

#endif
