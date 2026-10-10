#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <BlinkerV2/arduino/ArduinoDiagnostics.inc>
#include <BlinkerV2/arduino/ports/esp32/Esp32UsbDiagnosticOutput.h>

uint32_t diagnosticTestMillis = 0U;
bool diagnosticDriverInstalled = false;
unsigned diagnosticDriverCalls = 0U;
unsigned diagnosticDriverWait = 99U;
int diagnosticDriverResult = 0;

namespace {
int checks = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::cerr << "line " << __LINE__ << ": " << #condition << '\n'; \
    std::exit(1); } } while (false)

struct Capture {
    blinker::Diagnostics* recorder;
    std::vector<blinker::DiagnosticEvent> events;
    bool accept = true;
    uint8_t reenter = 0U;
};

uint32_t testClock(void*) { return diagnosticTestMillis; }

bool capture(void* context, const blinker::DiagnosticEvent& event) {
    Capture& output = *static_cast<Capture*>(context);
    output.events.push_back(event);
    const uint8_t mode = output.reenter;
    output.reenter = 0U;
    if (mode == 1U) {
        output.recorder->recordProductStatus(0U, 1U);
    } else if (mode == 2U) {
        output.recorder->disable();
    } else if (mode == 3U) {
        output.recorder->configure(&capture, &output, blinker::LogLevel::Trace,
                                   &testClock, nullptr);
    }
    if (mode != 0U) output.recorder->flush();
    return output.accept;
}

void coreGate() {
    blinker::Diagnostics recorder;
    Capture output;
    output.recorder = &recorder;
    recorder.configure(&capture, &output, blinker::LogLevel::Trace, &testClock);
    recorder.flush();
    CHECK(output.events.size() == 1U);
    // Repeated pending event: one output now, one summary on a later flush.
    recorder.recordProductStatus(0U, 1U);
    recorder.recordProductStatus(0U, 1U);
    recorder.flush();
    CHECK(output.events.size() == 2U);
    recorder.recordProductStatus(1U, 2U);
    recorder.flush();
    CHECK(output.events.size() == 3U);
    CHECK(output.events.back().code == static_cast<uint16_t>(
        blinker::DiagnosticCode::OutputSuppressed));
    recorder.flush();
    CHECK(output.events.size() == 4U);
    CHECK(output.events.back().argument1 == 2U);

    output.accept = false;
    recorder.recordProductStatus(2U, 3U);
    recorder.flush();
    CHECK(recorder.counters().outputDropped == 1U);
    recorder.flush();
    CHECK(output.events.size() == 5U); // no retry of a rejected output
    recorder.recordProductStatus(2U, 3U);
    recorder.flush();
    CHECK(output.events.size() == 5U); // rejected repeats are also rate limited
    diagnosticTestMillis = 1000U;
    recorder.flush();
    CHECK(output.events.size() == 6U);
    CHECK(recorder.counters().outputDropped == 2U); // rejected summary counted

    for (uint8_t mode = 1U; mode <= 3U; ++mode) {
        recorder.configure(&capture, &output, blinker::LogLevel::Trace, &testClock);
        const size_t before = output.events.size();
        output.accept = true;
        output.reenter = mode;
        recorder.flush();
        CHECK(output.events.size() == before + 1U); // no recursive sink call
        recorder.flush();
        CHECK(output.events.size() == before + (mode == 2U ? 1U : 2U));
        if (mode == 1U) CHECK(output.events.back().argument1 == 1U);
        if (mode == 2U) CHECK(!recorder.enabled());
        if (mode == 3U) CHECK(output.events.back().code == static_cast<uint16_t>(
            blinker::DiagnosticCode::DiagnosticsEnabled));
    }
    recorder.disable();
    const size_t before = output.events.size();
    recorder.recordEvent(blinker::Result::success());
    recorder.flush();
    CHECK(output.events.size() == before);
    CHECK(recorder.counters().eventsSent == 1U);

    // Unsigned elapsed arithmetic must retain its interval across millis wrap.
    diagnosticTestMillis = UINT32_MAX - 50U;
    recorder.configure(&capture, &output, blinker::LogLevel::Debug, &testClock);
    recorder.flush();
    recorder.recordState(true, blinker::Result::success(), 9U);
    const size_t wrapBefore = output.events.size();
    diagnosticTestMillis = 48U;
    recorder.flush();
    CHECK(output.events.size() == wrapBefore);
    diagnosticTestMillis = 49U;
    recorder.flush();
    CHECK(output.events.size() == wrapBefore + 1U);
}

class TryOutput : public blinker::IDiagnosticOutput {
public:
    size_t capacity = 1024U;
    size_t shortLimit = 1024U;
    size_t attempts = 0U;
    size_t writes = 0U;
    bool connected = true;
    std::string accepted;
    size_t tryWrite(const uint8_t* bytes, size_t size) override {
        ++attempts;
        if (!connected || capacity < size) return 0U;
        ++writes;
        const size_t count = std::min(shortLimit, size);
        accepted.append(reinterpret_cast<const char*>(bytes), count);
        return count;
    }
};

class SynchronousPrint : public Print {
public:
    size_t byteCalls = 0U;
    size_t bulkCalls = 0U;
    size_t limit = 1024U;
    size_t write(uint8_t) override { ++byteCalls; return 1U; }
    size_t write(const uint8_t*, size_t size) override {
        ++bulkCalls;
        return std::min(limit, size);
    }
};

size_t formatterGate() {
    size_t maximum = 0U;
    // Max timestamp/arguments and every status name, error, code, level,
    // component. Future text growth must not silently truncate a known event.
    for (uint16_t code = 1U; code <= 8U; ++code) {
        for (uint8_t state = 0U; state < 8U; ++state) {
            for (uint8_t level = 0U; level <= 5U; ++level) {
                blinker::DiagnosticEvent event;
                event.timestampMillis = UINT32_MAX;
                event.argument0 = code == 2U ? 0xff78U | state : UINT32_MAX;
                event.argument1 = event.argument0;
                event.code = code;
                event.level = level;
                event.component = 0xffU;
                DiagnosticLine line;
                formatEvent(&line, event);
                CHECK(line.complete());
                CHECK(line.size() <= 192U);
                const std::string text(reinterpret_cast<const char*>(line.data()), line.size());
                CHECK(text.find("[Blinker][4294967295]") == 0U);
                CHECK(text.substr(text.size() - 2U) == "\r\n");
                maximum = std::max(maximum, line.size());
            }
        }
    }
    DiagnosticLine overflow;
    for (size_t i = 0U; i < 192U; ++i) CHECK(overflow.write('x') == 1U);
    CHECK(overflow.complete());
    CHECK(overflow.write('x') == 0U);
    CHECK(!overflow.complete());
    CHECK(overflow.size() == 192U);
    return maximum;
}

void outputGate() {
    blinker::BlinkerClass logger;
    TryOutput output;
    output.capacity = 0U;
    const uint32_t baseline = logger.diagnosticCounters().outputDropped;
    logger.debug(output, blinker::LogLevel::Trace);
    CHECK(output.attempts == 1U && output.writes == 0U);
    CHECK(logger.diagnosticCounters().outputDropped == baseline + 1U);
    auto& recorder = blinker::facade_detail::diagnostics();
    for (uint16_t i = 1U; i <= 100U; ++i) {
        recorder.recordProductStatus(0U, i);
        const size_t before = output.attempts;
        recorder.flush();
        CHECK(output.attempts == before + 1U);
    }
    CHECK(output.writes == 0U); // full sink never creates an unbounded queue
    output.capacity = 1024U;
    output.connected = false;
    recorder.recordProductStatus(0U, 101U);
    recorder.flush();
    CHECK(output.writes == 0U);
    output.connected = true;
    output.shortLimit = 1U;
    const uint32_t beforeShort = logger.diagnosticCounters().outputDropped;
    recorder.recordProductStatus(0U, 102U);
    recorder.flush();
    CHECK(output.writes == 1U && output.accepted.size() == 1U);
    CHECK(logger.diagnosticCounters().outputDropped == beforeShort + 1U);
    recorder.flush();
    CHECK(output.writes == 1U); // no write of the remaining suffix
    output.shortLimit = 1024U;
    recorder.recordProductStatus(0U, 103U);
    recorder.flush();
    CHECK(output.writes == 2U);
    CHECK(output.accepted.find("[Blinker]", 1U) == 1U);
    CHECK(logger.diagnosticCounters().outputDropped == beforeShort + 1U);
    logger.noDebug();

    SynchronousPrint sync;
    logger.debug(sync);
    CHECK(sync.bulkCalls == 1U && sync.byteCalls == 0U);
    sync.limit = 0U;
    const uint32_t beforeZero = logger.diagnosticCounters().outputDropped;
    recorder.recordProductStatus(0U, 1U);
    recorder.flush();
    CHECK(sync.bulkCalls == 2U && sync.byteCalls == 0U);
    CHECK(logger.diagnosticCounters().outputDropped == beforeZero + 1U);
    logger.noDebug();
}

void nativeAdapterGate() {
    blinker::esp32::Esp32UsbDiagnosticOutput output;
    const uint8_t bytes[3] = {1U, 2U, 3U};
    CHECK(output.tryWrite(bytes, sizeof(bytes)) == 0U);
    CHECK(diagnosticDriverCalls == 0U); // no driver installation or invalid call
    diagnosticDriverInstalled = true;
    CHECK(output.tryWrite(nullptr, 3U) == 0U);
    CHECK(output.tryWrite(bytes, 0U) == 0U);
    CHECK(diagnosticDriverCalls == 0U);
    for (int result = -1; result <= 3; ++result) {
        diagnosticDriverResult = result;
        const unsigned before = diagnosticDriverCalls;
        CHECK(output.tryWrite(bytes, 3U) == (result > 0 ? static_cast<size_t>(result) : 0U));
        CHECK(diagnosticDriverCalls == before + 1U);
        CHECK(diagnosticDriverWait == 0U);
    }
}
} // namespace

int main() {
    coreGate();
    const size_t maximum = formatterGate();
    outputGate();
    nativeAdapterGate();
    std::cout << "diagnostics checks=" << checks << " event="
              << sizeof(blinker::DiagnosticEvent) << " recorder="
              << sizeof(blinker::Diagnostics) << " line=" << sizeof(DiagnosticLine)
              << " maxText=" << maximum << '\n';
}
