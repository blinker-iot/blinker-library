#ifndef BLINKER_PROTOCOL_BBP2_WIREERROR_H
#define BLINKER_PROTOCOL_BBP2_WIREERROR_H

#include <stdint.h>

namespace blinker {
// Shared wire contract, independent of any device or gateway runtime.
enum class WireError : uint16_t {
    MalformedMessage = 1,
    AuthenticationRequired = 2,
    NegotiationRequired = 3,
    UnsupportedMessage = 4,
    UnknownEndpoint = 5,
    CommandRejected = 6,
    ResourceExhausted = 7,
    InternalError = 8,
    SequenceConflict = 9,
    StateConflict = 10,
    ManifestConflict = 11,
    RateLimited = 12
};
}
#endif
