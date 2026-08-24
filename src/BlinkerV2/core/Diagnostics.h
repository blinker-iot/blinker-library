#ifndef BLINKER_CORE_DIAGNOSTICS_H
#define BLINKER_CORE_DIAGNOSTICS_H

#include <stdint.h>

#include "Result.h"

namespace blinker {

enum class LogLevel : uint8_t {
    Error = 0U,
    Warning,
    Info,
    Debug,
    Trace
};

enum class DiagnosticComponent : uint8_t {
    Library = 0U,
    Product,
    State,
    Event
};

enum class DiagnosticCode : uint16_t {
    DiagnosticsEnabled = 1U,
    ProductStatusChanged,
    OperationSucceeded,
    OperationFailed,
    StateChanged,
    StateUnchanged,
    EventSent,
    OutputSuppressed
};

enum class DiagnosticOperation : uint8_t {
    Begin = 0U,
    StateReport,
    EventEmit
};

// Fixed numeric event passed between the portable core and optional platform
// formatters. It cannot contain credentials, payloads or dynamic strings.
struct DiagnosticEvent {
    uint32_t timestampMillis;
    uint32_t argument0;
    uint32_t argument1;
    uint16_t code;
    uint8_t component;
    uint8_t level;

    DiagnosticEvent()
        : timestampMillis(0U),
          argument0(0U),
          argument1(0U),
          code(0U),
          component(0U),
          level(0U) {}
};

static_assert(
    sizeof(DiagnosticEvent) == 16U,
    "DiagnosticEvent must remain a compact 16-byte record");

struct DiagnosticCounters {
    uint32_t lifecycleTransitions;
    uint32_t operationFailures;
    uint32_t wouldBlock;
    uint32_t stateChanges;
    uint32_t stateUnchanged;
    uint32_t eventsSent;
    uint32_t outputSuppressed;
    uint32_t outputDropped;

    DiagnosticCounters()
        : lifecycleTransitions(0U),
          operationFailures(0U),
          wouldBlock(0U),
          stateChanges(0U),
          stateUnchanged(0U),
          eventsSent(0U),
          outputSuppressed(0U),
          outputDropped(0U) {}
};

typedef void (*DiagnosticSink)(void* context, const DiagnosticEvent& event);
typedef uint32_t (*DiagnosticClock)(void* context);

// Allocation-free diagnostic recorder. Counters remain available while
// output is disabled. One pending event is retained so formatting happens
// only when flush() is called from a safe application context.
class Diagnostics {
public:
    Diagnostics();

    void configure(
        DiagnosticSink sink,
        void* sinkContext,
        LogLevel level = LogLevel::Info,
        DiagnosticClock clock = nullptr,
        void* clockContext = nullptr);
    void disable();
    bool enabled() const { return sink_ != nullptr; }
    LogLevel level() const { return level_; }

    void recordProductStatus(uint16_t previous, uint16_t current);
    void recordOperation(DiagnosticOperation operation, Result result);
    void recordState(bool changed, Result result, uint32_t revision);
    void recordEvent(Result result);

    void flush();
    const DiagnosticCounters& counters() const { return counters_; }

private:
    uint32_t now() const;
    void queue(
        LogLevel level,
        DiagnosticComponent component,
        DiagnosticCode code,
        uint32_t argument0,
        uint32_t argument1);
    void emitSuppressed(uint32_t timestamp);
    static bool sameEvent(
        const DiagnosticEvent& first,
        const DiagnosticEvent& second);

    DiagnosticSink sink_;
    void* sinkContext_;
    DiagnosticClock clock_;
    void* clockContext_;
    DiagnosticCounters counters_;
    DiagnosticEvent pending_;
    DiagnosticEvent lastEmitted_;
    uint32_t lastEmittedAt_;
    uint16_t pendingRepeats_;
    uint16_t emittedRepeats_;
    LogLevel level_;
    bool hasPending_;
    bool hasLastEmitted_;

    Diagnostics(const Diagnostics&);
    Diagnostics& operator=(const Diagnostics&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(Diagnostics) <= 96U,
    "Diagnostics exceeds its 32-bit singleton RAM gate");
#endif

} // namespace blinker

#endif
