#ifndef BLINKER_UNO_R4_MODEM_CLIENT_H
#define BLINKER_UNO_R4_MODEM_CLIENT_H

#include "UnoR4ModemOwner.h"
#include <WiFiS3.h>

namespace blinker {

// Gate only: SDK connect/write may virtually call other methods on this same
// object. Taking a new non-reentrant lease in each method would self-deadlock.
// The cooperative modem owner excludes storage/foreign callbacks; these native
// network operations retain their existing SDK timeouts (not task deadlines).
template <typename Native>
class RenesasUnoModemClient : public Native {
public:
    int connect(IPAddress ip, uint16_t port) override {
        return allowed() ? Native::connect(ip, port) : 0;
    }
    int connect(const char* host, uint16_t port) override {
        return allowed() ? Native::connect(host, port) : 0;
    }
    size_t write(uint8_t byte) override { return allowed() ? Native::write(byte) : 0U; }
    size_t write(const uint8_t* data, size_t size) override {
        return allowed() ? Native::write(data, size) : 0U;
    }
    int available() override { return allowed() ? Native::available() : 0; }
    int read() override { return allowed() ? Native::read() : -1; }
    int read(uint8_t* data, size_t size) override { return allowed() ? Native::read(data, size) : -1; }
    int peek() override { return allowed() ? Native::peek() : -1; }
    void flush() override { if (allowed()) Native::flush(); }
    void stop() override { if (allowed()) Native::stop(); }
    uint8_t connected() override { return allowed() ? Native::connected() : 0U; }
    operator bool() override { return allowed() && Native::operator bool(); }
    IPAddress remoteIP() override { return allowed() ? Native::remoteIP() : IPAddress(); }
    uint16_t remotePort() override { return allowed() ? Native::remotePort() : 0U; }
    using Native::write;
private:
    static bool allowed() { return renesasUnoModemOwner().networkAllowed(); }
};

} // namespace blinker
#endif
