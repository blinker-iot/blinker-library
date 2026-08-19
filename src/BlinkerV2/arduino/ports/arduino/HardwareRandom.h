#ifndef BLINKER_V2_ARDUINO_PORTS_HARDWARE_RANDOM_H
#define BLINKER_V2_ARDUINO_PORTS_HARDWARE_RANDOM_H

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_RENESAS_UNO)
#error "HardwareRandom requires ESP32 or Renesas UNO"
#endif

#include <BlinkerV2/interface/IRandom.h>

namespace blinker {

class PlatformHardwareRandom : public IRandom {
public:
    Result fill(MutableByteSpan output) override;
};

} // namespace blinker

#endif
