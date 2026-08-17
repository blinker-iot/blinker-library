#ifndef BLINKER_PROTOCOL_SETUPSESSION_H
#define BLINKER_PROTOCOL_SETUPSESSION_H

#include <stddef.h>

namespace blinker {

enum : size_t {
    kSetupSessionLocatorSize = 8U,
    kSetupSessionIdSize = 16U,
    kSetupTranscriptHashSize = 32U
};

} // namespace blinker

#endif
