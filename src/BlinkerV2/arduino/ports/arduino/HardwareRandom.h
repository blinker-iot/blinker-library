#ifndef BLINKER_V2_ARDUINO_PORTS_HARDWARE_RANDOM_H
#define BLINKER_V2_ARDUINO_PORTS_HARDWARE_RANDOM_H

#if !defined(ARDUINO_ARCH_ESP32) && \
    !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "HardwareRandom requires ESP32, UNO R4 or Wio Terminal"
#endif

#include <BlinkerV2/interface/IRandom.h>

namespace blinker {

class PlatformHardwareRandom : public IRandom {
public:
    Result fill(MutableByteSpan output) override;
};

} // namespace blinker

#endif
