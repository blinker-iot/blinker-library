#ifndef BLINKER_TEST_R4_CLEANUP_CLIENT_H
#define BLINKER_TEST_R4_CLEANUP_CLIENT_H
#include "Stream.h"
#include "IPAddress.h"
class Client : public Stream {
public:
    virtual int connect(IPAddress, uint16_t) = 0;
    virtual int connect(const char*, uint16_t) = 0;
    virtual int read(uint8_t*, size_t) = 0;
    virtual void stop() = 0;
    virtual uint8_t connected() = 0;
    virtual operator bool() = 0;
    using Stream::read;
    using Print::write;
};
#endif
