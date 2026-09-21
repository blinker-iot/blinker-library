#ifndef BLINKER_ESP32_LOCALDISCOVERY_H
#define BLINKER_ESP32_LOCALDISCOVERY_H

#include <BlinkerV2/interface/IRandom.h>

namespace blinker {

// Discovery owns only its service registration. The locator is random public
// metadata, deliberately unrelated to MAC, DeviceKey or the authenticated ID.
// Kept separate from socket/Noise/Runtime so another IP product can reuse it.
class Esp32LocalDiscovery {
public:
    explicit Esp32LocalDiscovery(IRandom& random) : random_(random) {}
    ~Esp32LocalDiscovery() { stop(); }
    Result start(uint16_t port, bool encryptedMessages);
    void stop();
    bool advertised() const { return advertised_; }
private:
    IRandom& random_;
    char locator_[33] = {};
    bool advertised_ = false;
    Esp32LocalDiscovery(const Esp32LocalDiscovery&) = delete;
    Esp32LocalDiscovery& operator=(const Esp32LocalDiscovery&) = delete;
};

} // namespace blinker
#endif
