#if defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)

#include "WioTerminalStorage.h"

namespace blinker {

__attribute__((aligned(8192)))
const uint8_t WioTerminalFlashBlobBank::bank0_[kEraseBlockSize] = {};

__attribute__((aligned(8192)))
const uint8_t WioTerminalFlashBlobBank::bank1_[kEraseBlockSize] = {};

} // namespace blinker

#endif
