#ifndef BLINKER_ARDUINO_SKETCHTIMER_H
#define BLINKER_ARDUINO_SKETCHTIMER_H

#include <Arduino.h>
#include "../core/SketchTimer.h"

namespace blinker {
namespace timer_detail {
inline uint32_t arduinoMillis() {
    return static_cast<uint32_t>(::millis());
}
} // namespace timer_detail

// Optional Sketch-local scheduler. It does not own a Blinker session, UTC,
// storage or a network connection; drive it separately from loop().
template <size_t Capacity = 4U>
class Timer : public SketchTimer<Capacity> {
public:
    Timer() : SketchTimer<Capacity>(&timer_detail::arduinoMillis) {}
};
} // namespace blinker

#endif
