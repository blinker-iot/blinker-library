#include "Diagnostics.h"

namespace blinker {

namespace {

const uint32_t kRepeatWindowMillis = 1000U;
const uint32_t kVerboseOutputIntervalMillis = 100U;

DiagnosticComponent componentFor(DiagnosticOperation operation) {
    switch (operation) {
    case DiagnosticOperation::StateReport:
        return DiagnosticComponent::State;
    case DiagnosticOperation::EventEmit:
        return DiagnosticComponent::Event;
    case DiagnosticOperation::Begin:
        return DiagnosticComponent::Product;
    }
    return DiagnosticComponent::Library;
}

LogLevel failureLevel(ErrorCode error) {
    return error == ErrorCode::WouldBlock ||
                   error == ErrorCode::NotConnected ||
                   error == ErrorCode::AuthenticationRequired
               ? LogLevel::Warning
               : LogLevel::Error;
}

} // namespace

Diagnostics::Diagnostics()
    : sink_(nullptr),
      sinkContext_(nullptr),
      clock_(nullptr),
      clockContext_(nullptr),
      counters_(),
      pending_(),
      lastEmitted_(),
      lastEmittedAt_(0U),
      pendingRepeats_(0U),
      emittedRepeats_(0U),
      level_(LogLevel::Info),
      hasPending_(false),
      hasLastEmitted_(false) {}

void Diagnostics::configure(
    DiagnosticSink sink,
    void* sinkContext,
    LogLevel level,
    DiagnosticClock clock,
    void* clockContext) {
    sink_ = sink;
    sinkContext_ = sinkContext;
    clock_ = clock;
    clockContext_ = clockContext;
    level_ = level;
    hasPending_ = false;
    hasLastEmitted_ = false;
    pendingRepeats_ = 0U;
    emittedRepeats_ = 0U;
    if (sink_ != nullptr) {
        queue(
            LogLevel::Info,
            DiagnosticComponent::Library,
            DiagnosticCode::DiagnosticsEnabled,
            static_cast<uint32_t>(level_),
            0U);
    }
}

void Diagnostics::disable() {
    sink_ = nullptr;
    sinkContext_ = nullptr;
    clock_ = nullptr;
    clockContext_ = nullptr;
    hasPending_ = false;
    hasLastEmitted_ = false;
    pendingRepeats_ = 0U;
    emittedRepeats_ = 0U;
}

uint32_t Diagnostics::now() const {
    return clock_ != nullptr ? clock_(clockContext_) : 0U;
}

bool Diagnostics::sameEvent(
    const DiagnosticEvent& first,
    const DiagnosticEvent& second) {
    return first.level == second.level &&
           first.component == second.component &&
           first.code == second.code &&
           first.argument0 == second.argument0 &&
           first.argument1 == second.argument1;
}

void Diagnostics::queue(
    LogLevel level,
    DiagnosticComponent component,
    DiagnosticCode code,
    uint32_t argument0,
    uint32_t argument1) {
    if (sink_ == nullptr ||
        static_cast<uint8_t>(level) > static_cast<uint8_t>(level_)) {
        return;
    }

    DiagnosticEvent event;
    event.timestampMillis = now();
    event.argument0 = argument0;
    event.argument1 = argument1;
    event.code = static_cast<uint16_t>(code);
    event.component = static_cast<uint8_t>(component);
    event.level = static_cast<uint8_t>(level);

    if (hasLastEmitted_ && sameEvent(event, lastEmitted_) &&
        static_cast<uint32_t>(event.timestampMillis - lastEmittedAt_) <
            kRepeatWindowMillis) {
        if (emittedRepeats_ != UINT16_MAX) ++emittedRepeats_;
        ++counters_.outputSuppressed;
        return;
    }

    if (!hasPending_) {
        pending_ = event;
        hasPending_ = true;
        pendingRepeats_ = 0U;
        return;
    }

    if (sameEvent(event, pending_)) {
        if (pendingRepeats_ != UINT16_MAX) ++pendingRepeats_;
        ++counters_.outputSuppressed;
        return;
    }

    ++counters_.outputDropped;
    if (event.level < pending_.level ||
        (event.level == pending_.level &&
         event.level >= static_cast<uint8_t>(LogLevel::Debug))) {
        pending_ = event;
        pendingRepeats_ = 0U;
    }
}

void Diagnostics::emitSuppressed(uint32_t timestamp) {
    if (sink_ == nullptr || emittedRepeats_ == 0U) return;
    DiagnosticEvent summary;
    summary.timestampMillis = timestamp;
    summary.argument0 = lastEmitted_.code;
    summary.argument1 = emittedRepeats_;
    summary.code = static_cast<uint16_t>(
        DiagnosticCode::OutputSuppressed);
    summary.component = lastEmitted_.component;
    summary.level = lastEmitted_.level;
    sink_(sinkContext_, summary);
    emittedRepeats_ = 0U;
}

void Diagnostics::flush() {
    if (sink_ == nullptr) return;
    const uint32_t timestamp = now();
    if (emittedRepeats_ != 0U &&
        (hasPending_ ||
         static_cast<uint32_t>(timestamp - lastEmittedAt_) >=
             kRepeatWindowMillis)) {
        emitSuppressed(timestamp);
    }
    if (!hasPending_) return;
    if (clock_ != nullptr &&
        pending_.level >= static_cast<uint8_t>(LogLevel::Debug) &&
        hasLastEmitted_ &&
        static_cast<uint32_t>(timestamp - lastEmittedAt_) <
            kVerboseOutputIntervalMillis) {
        return;
    }

    sink_(sinkContext_, pending_);
    lastEmitted_ = pending_;
    lastEmittedAt_ = timestamp;
    hasLastEmitted_ = true;
    hasPending_ = false;
    if (pendingRepeats_ != 0U) {
        emittedRepeats_ = pendingRepeats_;
        pendingRepeats_ = 0U;
        emitSuppressed(timestamp);
    }
}

void Diagnostics::recordProductStatus(uint16_t previous, uint16_t current) {
    if (previous == current) return;
    ++counters_.lifecycleTransitions;
    queue(
        LogLevel::Info,
        DiagnosticComponent::Product,
        DiagnosticCode::ProductStatusChanged,
        previous,
        current);
}

void Diagnostics::recordOperation(
    DiagnosticOperation operation,
    Result result) {
    const DiagnosticComponent component = componentFor(operation);
    if (result) return;

    ++counters_.operationFailures;
    if (result.code() == ErrorCode::WouldBlock) ++counters_.wouldBlock;
    queue(
        failureLevel(result.code()),
        component,
        DiagnosticCode::OperationFailed,
        static_cast<uint32_t>(operation),
        static_cast<uint32_t>(result.code()));
}

void Diagnostics::recordState(
    bool changed,
    Result result,
    uint32_t revision) {
    if (changed) ++counters_.stateChanges;
    if (!result) {
        recordOperation(DiagnosticOperation::StateReport, result);
        return;
    }
    if (!changed) {
        ++counters_.stateUnchanged;
        queue(
            LogLevel::Trace,
            DiagnosticComponent::State,
            DiagnosticCode::StateUnchanged,
            revision,
            0U);
        return;
    }
    queue(
        LogLevel::Debug,
        DiagnosticComponent::State,
        DiagnosticCode::StateChanged,
        revision,
        0U);
}

void Diagnostics::recordEvent(Result result) {
    if (!result) {
        recordOperation(DiagnosticOperation::EventEmit, result);
        return;
    }
    ++counters_.eventsSent;
    queue(
        LogLevel::Debug,
        DiagnosticComponent::Event,
        DiagnosticCode::EventSent,
        counters_.eventsSent,
        0U);
}

} // namespace blinker
