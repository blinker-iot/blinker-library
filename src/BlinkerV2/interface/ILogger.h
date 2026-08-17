#ifndef BLINKER_INTERFACE_ILOGGER_H
#define BLINKER_INTERFACE_ILOGGER_H

#include "../core/Span.h"

namespace blinker {

enum class LogLevel : uint8_t {
    Error = 0,
    Warning,
    Info,
    Debug,
    Trace
};

class ILogger {
public:
    virtual ~ILogger() {}

    virtual void log(LogLevel level, StringView component, StringView message) = 0;
};

} // namespace blinker

#endif
