#ifndef BLINKER_INTERFACE_IDIAGNOSTICOUTPUT_H
#define BLINKER_INTERFACE_IDIAGNOSTICOUTPUT_H

#include <stddef.h>
#include <stdint.h>

namespace blinker {

// Optional platform-owned, non-waiting diagnostic byte sink. The caller owns
// its lifetime until noDebug() (or replacement). Implementations must return
// immediately on contention, disconnect or insufficient capacity. They must
// not retain the buffer, wait for space, retry, flush, or change shared I/O
// configuration. Return accepted bytes; short writes lose the rest of the
// line and count as one dropped event. This is not a delivery acknowledgement.
//
// Checking Print::availableForWrite() alone does not satisfy this contract:
// both the capacity query and write must have verified no-wait semantics.
class IDiagnosticOutput {
public:
    virtual ~IDiagnosticOutput() {}
    virtual size_t tryWrite(const uint8_t* bytes, size_t size) = 0;
};

} // namespace blinker

#endif
