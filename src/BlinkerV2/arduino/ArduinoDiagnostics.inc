#include <Arduino.h>

#include "../api/BlinkerFacade.h"

namespace {

uint32_t diagnosticMillis(void*) {
    return millis();
}

const __FlashStringHelper* levelName(uint8_t value) {
    switch (static_cast<blinker::LogLevel>(value)) {
    case blinker::LogLevel::Error: return F("error");
    case blinker::LogLevel::Warning: return F("warning");
    case blinker::LogLevel::Info: return F("info");
    case blinker::LogLevel::Debug: return F("debug");
    case blinker::LogLevel::Trace: return F("trace");
    }
    return F("unknown");
}

const __FlashStringHelper* componentName(uint8_t value) {
    switch (static_cast<blinker::DiagnosticComponent>(value)) {
    case blinker::DiagnosticComponent::Library: return F("library");
    case blinker::DiagnosticComponent::Product: return F("product");
    case blinker::DiagnosticComponent::State: return F("state");
    case blinker::DiagnosticComponent::Event: return F("event");
    }
    return F("unknown");
}

const __FlashStringHelper* stateName(uint16_t status) {
    switch (static_cast<blinker::ProductLifecycleState>(status & 0x07U)) {
    case blinker::ProductLifecycleState::Stopped: return F("stopped");
    case blinker::ProductLifecycleState::Starting: return F("starting");
    case blinker::ProductLifecycleState::Provisioning:
        return F("provisioning");
    case blinker::ProductLifecycleState::Enrolling: return F("enrolling");
    case blinker::ProductLifecycleState::Active: return F("active");
    case blinker::ProductLifecycleState::Fault: return F("fault");
    }
    return F("unknown");
}

const __FlashStringHelper* operationName(uint32_t value) {
    switch (static_cast<blinker::DiagnosticOperation>(value)) {
    case blinker::DiagnosticOperation::Begin: return F("begin");
    case blinker::DiagnosticOperation::StateReport:
        return F("state-report");
    case blinker::DiagnosticOperation::EventEmit: return F("event-emit");
    }
    return F("unknown");
}

void printStatus(Print& output, uint16_t status) {
    output.print(stateName(status));
    output.print(F(" started="));
    output.print((status & (1U << 3U)) != 0U);
    output.print(F(" network="));
    output.print((status & (1U << 4U)) != 0U);
    output.print(F(" local="));
    output.print((status & (1U << 5U)) != 0U);
    output.print(F(" cloud="));
    output.print((status & (1U << 6U)) != 0U);
    const uint8_t error = static_cast<uint8_t>(status >> 8U);
    if (error != 0U) {
        output.print(F(" error="));
        output.print(error);
    }
}

void printEvent(void* context, const blinker::DiagnosticEvent& event) {
    Print* output = static_cast<Print*>(context);
    if (output == nullptr) return;

    output->print(F("[Blinker]["));
    output->print(event.timestampMillis);
    output->print(F("]["));
    output->print(levelName(event.level));
    output->print(F("]["));
    output->print(componentName(event.component));
    output->print(F("] "));

    switch (static_cast<blinker::DiagnosticCode>(event.code)) {
    case blinker::DiagnosticCode::DiagnosticsEnabled:
        output->print(F("diagnostics enabled level="));
        output->print(levelName(static_cast<uint8_t>(event.argument0)));
        break;
    case blinker::DiagnosticCode::ProductStatusChanged:
        output->print(F("status "));
        printStatus(*output, static_cast<uint16_t>(event.argument0));
        output->print(F(" -> "));
        printStatus(*output, static_cast<uint16_t>(event.argument1));
        break;
    case blinker::DiagnosticCode::OperationSucceeded:
        output->print(operationName(event.argument0));
        output->print(F(" succeeded"));
        break;
    case blinker::DiagnosticCode::OperationFailed:
        output->print(operationName(event.argument0));
        output->print(F(" failed error="));
        output->print(event.argument1);
        break;
    case blinker::DiagnosticCode::StateChanged:
        output->print(F("changed currentRevision="));
        output->print(event.argument0);
        break;
    case blinker::DiagnosticCode::StateUnchanged:
        output->print(F("unchanged revision="));
        output->print(event.argument0);
        break;
    case blinker::DiagnosticCode::EventSent:
        output->print(F("sent count="));
        output->print(event.argument0);
        break;
    case blinker::DiagnosticCode::OutputSuppressed:
        output->print(F("repeated event="));
        output->print(event.argument0);
        output->print(F(" suppressed="));
        output->print(event.argument1);
        break;
    }
    output->println();
}

void printCounter(
    Print& output,
    const __FlashStringHelper* name,
    uint32_t value) {
    output.print(F("  "));
    output.print(name);
    output.print(F("="));
    output.println(value);
}

} // namespace

void blinker::BlinkerClass::debug(Print& output, LogLevel level) {
    facade_detail::diagnostics().configure(
        &printEvent,
        &output,
        level,
        &diagnosticMillis,
        nullptr);
    facade_detail::diagnostics().flush();
}

void blinker::BlinkerClass::noDebug() {
    facade_detail::diagnostics().disable();
}

void blinker::BlinkerClass::printDiagnostics(Print& output) {
    facade_detail::diagnostics().flush();
    const ProductStatus current = status();
    output.println(F("[Blinker] diagnostics"));
    output.print(F("  state="));
    output.println(stateName(static_cast<uint16_t>(current.state)));
    output.print(F("  started="));
    output.print(current.started);
    output.print(F(" network="));
    output.print(current.networkReady);
    output.print(F(" local="));
    output.print(current.localReady);
    output.print(F(" cloud="));
    output.println(current.cloudReady);
    output.print(F("  lastOperationError="));
    output.print(static_cast<uint8_t>(lastError()));
    output.print(F(" ("));
    output.print(lastErrorText());
    output.println(F(")"));
    output.print(F("  productError="));
    output.println(static_cast<uint8_t>(current.lastError));

    const DiagnosticCounters& counters =
        facade_detail::diagnostics().counters();
    printCounter(
        output, F("lifecycleTransitions"), counters.lifecycleTransitions);
    printCounter(
        output, F("operationFailures"), counters.operationFailures);
    printCounter(output, F("wouldBlock"), counters.wouldBlock);
    printCounter(output, F("stateChanges"), counters.stateChanges);
    printCounter(output, F("stateUnchanged"), counters.stateUnchanged);
    printCounter(output, F("eventsSent"), counters.eventsSent);
    printCounter(
        output, F("outputSuppressed"), counters.outputSuppressed);
    printCounter(output, F("outputDropped"), counters.outputDropped);

    const OutboundSchedulerCounters& outbound = outboundCounters();
    printCounter(output, F("stateAccepted"), outbound.stateAccepted);
    printCounter(output, F("stateCoalesced"), outbound.stateCoalesced);
    printCounter(output, F("stateFlushes"), outbound.stateFlushes);
    printCounter(output, F("stateDeferred"), outbound.stateDeferred);
    printCounter(
        output,
        F("stateTransportErrors"),
        outbound.stateTransportErrors);
    printCounter(output, F("eventAccepted"), outbound.eventAccepted);
    printCounter(output, F("eventDispatched"), outbound.eventDispatched);
    printCounter(output, F("eventThrottled"), outbound.eventThrottled);
    printCounter(
        output,
        F("eventTransportErrors"),
        outbound.eventTransportErrors);

    const TelemetryCounters telemetry = telemetryCounters();
    printCounter(output, F("telemetrySamples"), telemetry.samplesSent);
    printCounter(output, F("telemetryDropped"), telemetry.samplesDropped);
    printCounter(output, F("leaseExpired"), telemetry.leasesExpired);
}
