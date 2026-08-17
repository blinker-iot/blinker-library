#ifndef BLINKER_RENESAS_UNO_BLE_SECURITY_H
#define BLINKER_RENESAS_UNO_BLE_SECURITY_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "BlinkerRenesasUnoBleSecurity requires an Arduino Renesas target"
#endif

#include <BlinkerArduinoBleAdapter.h>

namespace blinker {

// ArduinoBLE 2.0.2 keeps the controller encryption-complete fact per ATT
// connection handle, but exposes only an any-peer BLE.paired() shortcut.
// This platform probe resolves the exact central and fails closed. ArduinoBLE
// does not expose a matching per-connection bond fact, so bonded() is false.
class RenesasUnoBleSecuritySource final
    : public IArduinoBleSecuritySource {
public:
    RenesasUnoBleSecuritySource() : connectionHandle_(0xffffU) {}

    void connected(const BLEDevice& central) override;
    void disconnected() override { connectionHandle_ = 0xffffU; }
    bool encrypted() const override;
    bool bonded() const override { return false; }

private:
    uint16_t connectionHandle_;
};

} // namespace blinker

#endif
