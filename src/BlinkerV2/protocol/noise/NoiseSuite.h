#ifndef BLINKER_PROTOCOL_NOISE_NOISESUITE_H
#define BLINKER_PROTOCOL_NOISE_NOISESUITE_H

#include <stdint.h>

namespace blinker {
namespace noise {

// Frozen local-setup suite ids. The product composition selects one suite
// before advertising; peers never negotiate it inside an unauthenticated
// setup record.
enum class NoiseNnPattern : uint8_t {
    Nn = 1U,
    NnPsk0 = 2U
};

inline bool validNoiseNnPattern(NoiseNnPattern pattern) {
    return pattern == NoiseNnPattern::Nn ||
           pattern == NoiseNnPattern::NnPsk0;
}

inline bool noiseNnPatternRequiresPsk(NoiseNnPattern pattern) {
    return pattern == NoiseNnPattern::NnPsk0;
}

} // namespace noise
} // namespace blinker

#endif
