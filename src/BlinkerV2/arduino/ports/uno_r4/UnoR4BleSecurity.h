#ifndef BLINKER_V2_ARDUINO_PORTS_UNO_R4_BLE_SECURITY_H
#define BLINKER_V2_ARDUINO_PORTS_UNO_R4_BLE_SECURITY_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "UnoR4BleSecurity requires an Arduino Renesas target"
#endif

#include <BlinkerV2/interface/IBleConnectionSecuritySource.h>

namespace blinker {

// ArduinoBLE 2.0.2 keeps the controller encryption-complete fact per ATT
// connection handle, but exposes only an any-peer BLE.paired() shortcut.
// This platform probe resolves the exact central and fails closed. ArduinoBLE
// does not expose a matching per-connection bond fact, so bonded() is false.
class RenesasUnoBleSecuritySource final
    : public IBleConnectionSecuritySource {
public:
    RenesasUnoBleSecuritySource() : connectionHandle_(0xffffU) {}

    void connected(StringView peerAddress) override;
    void disconnected() override { connectionHandle_ = 0xffffU; }
    bool encrypted() const override;
    bool bonded() const override { return false; }

private:
    uint16_t connectionHandle_;
};

} // namespace blinker

#include "UnoR4BleSecurity.ipp"

#endif
