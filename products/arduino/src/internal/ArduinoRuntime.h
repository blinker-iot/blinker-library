#ifndef BLINKER_V2_ARDUINO_RUNTIME_H
#define BLINKER_V2_ARDUINO_RUNTIME_H

#include <Arduino.h>
#include <BlinkerV2/interface/IClock.h>

namespace blinker {
namespace integration {
namespace official_detail {

class ArduinoClock final : public IClock {
public:
    uint32_t monotonicMillis() const override { return millis(); }
    bool unixTime(uint64_t&) const override { return false; }
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
