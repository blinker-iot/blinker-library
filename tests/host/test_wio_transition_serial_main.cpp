#include "../internal/arduino/WioTerminalIdentityTransitionGate/SerialAdapter.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::abort(); } } while (false)
using namespace blinker;
using namespace wio_identity_transition;
namespace {
const char source[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
const char baseline[] = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
struct Transport final : SerialTransport {
    std::vector<uint8_t> input, output;
    size_t position = 0U, maximumWrite = 7U, failAfter = static_cast<size_t>(-1);
    uint32_t tick = 0U;
    unsigned readCalls = 0U, writeCalls = 0U, lateReadCall = 0U, lateWriteCall = 0U;
    int freeSpace = 64;
    std::function<void()> onIdle, onWrite;
    uint32_t now() override { return tick; }
    int available() override { return static_cast<int>(input.size() - position); }
    int read() override {
        ++readCalls;
        if (readCalls == lateReadCall) tick += SerialAdapter::timeoutMs;
        return position == input.size() ? -1 : input[position++];
    }
    int writable() override { return freeSpace; }
    size_t write(const uint8_t* bytes, size_t size) override {
        ++writeCalls;
        if (output.size() >= failAfter) return 0U;
        const size_t copied = std::min(size, std::min(maximumWrite, failAfter - output.size()));
        output.insert(output.end(), bytes, bytes + copied);
        if (writeCalls == lateWriteCall) tick += SerialAdapter::timeoutMs;
        if (onWrite) onWrite();
        return copied;
    }
    void idle() override { ++tick; if (onIdle) onIdle(); }
    void command(char value) {
        CHECK(position == input.size()); input.push_back(static_cast<uint8_t>(value)); output.clear();
    }
};
struct Port final : TransitionPort, TransitionReadback {
    uint8_t raw[SerialAdapter::rawSize] = {}, instance[16U] = {};
    unsigned calls = 0U, uidReads = 0U, captures = 0U, prepares = 0U, restores = 0U;
    bool ready = true, captured = false, uidValid = true, shortRaw = false, shortInstance = false;
    Result result;
    std::function<void()> callback;
    Port() { std::memset(raw, 0xff, sizeof(raw)); std::memset(instance, 0x75, sizeof(instance)); }
    bool healthy() const override { return ready; }
    Result act() { ++calls; if (callback) callback(); captured = result.ok(); return result; }
    Result prepareTemporary() override { ++prepares; raw[9000U] = 0x75U; return act(); }
    Result captureCurrent() override { ++captures; return act(); }
    Result restoreOriginal(ByteView desired) override {
        ++restores;
        if (desired.size == sizeof(raw) && desired.data != nullptr) std::memcpy(raw, desired.data, sizeof(raw));
        return act();
    }
    bool readUid(uint8_t* output) override {
        ++uidReads; for (size_t i = 0U; i < 16U; ++i) output[i] = static_cast<uint8_t>(i);
        return uidValid;
    }
    ByteView capturedRaw() const override {
        return captured ? ByteView(raw, shortRaw ? sizeof(raw) - 1U : sizeof(raw)) : ByteView();
    }
    ByteView temporaryInstance() const override { return ByteView(instance, shortInstance ? 15U : 16U); }
};
struct Model {
    Port port;
    Transport stream;
    TransitionController controller;
    SerialBinding binding;
    SerialAdapter adapter;
    explicit Model(bool executable = true)
        : controller(port), binding{4194304U, 0x3fc000U, 0x3fe000U, 8192U, source, baseline, executable},
          adapter(controller, port, stream, binding) {}
    void run(char value) { stream.command(value); adapter.service(); }
};
uint32_t read32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0U]) | (static_cast<uint32_t>(bytes[1U]) << 8U) |
        (static_cast<uint32_t>(bytes[2U]) << 16U) | (static_cast<uint32_t>(bytes[3U]) << 24U);
}
void put32(uint8_t* bytes, uint32_t value) {
    for (size_t i = 0U; i < 4U; ++i) bytes[i] = static_cast<uint8_t>(value >> (i * 8U));
}
ByteView frame(const Model& model, const char* magic, size_t size) {
    const auto& bytes = model.stream.output;
    CHECK(bytes.size() == size + 10U && std::memcmp(bytes.data(), magic, 4U) == 0);
    CHECK((static_cast<size_t>(bytes[4U]) | (static_cast<size_t>(bytes[5U]) << 8U)) == size);
    const ByteView value(bytes.data() + 10U, size); CHECK(read32(bytes.data() + 6U) == computeCrc32(value)); return value;
}
void error(const Model& model, ErrorCode expected) {
    CHECK(model.stream.output.size() == 5U);
    CHECK(std::memcmp(model.stream.output.data(), "BVE1", 4U) == 0);
    CHECK(model.stream.output[4U] == static_cast<uint8_t>(expected));
    CHECK(model.controller.state() == State::Failed);
}
std::vector<uint8_t> request() {
    std::vector<uint8_t> value(10U + SerialAdapter::rawSize, uint8_t(0x42U));
    std::memcpy(value.data(), "BVR1", 4U); value[4U] = 0U; value[5U] = 0x40U;
    put32(value.data() + 6U, computeCrc32(ByteView(value.data() + 10U, SerialAdapter::rawSize)));
    return value;
}
void restore(Model& model, const std::vector<uint8_t>& value) {
    model.stream.command('r'); model.stream.input.insert(model.stream.input.end(), value.begin(), value.end());
    model.adapter.service();
}
void metadataAndHappyPath() {
    Model model;
    CHECK(model.port.calls == 0U && model.port.uidReads == 0U);
    model.run('p'); const ByteView info = frame(model, "BVI1", 96U);
    CHECK(info.data[0U] == 1U && read32(info.data + 1U) == 4194304U);
    CHECK(read32(info.data + 5U) == 0x3fc000U && read32(info.data + 9U) == 0x3fe000U);
    CHECK(info.data[13U] == 0U && info.data[14U] == 0x20U && info.data[95U] == 0U);
    for (size_t i = 0U; i < 16U; ++i) CHECK(info.data[15U + i] == i);
    CHECK(info.data[31U] == 0x01U && info.data[62U] == 0xefU);
    CHECK(info.data[63U] == 0xabU && info.data[94U] == 0x89U);
    CHECK(model.port.calls == 0U);
    model.run('d'); CHECK(frame(model, "BVS1", SerialAdapter::rawSize).data[0U] == 0xffU);
    CHECK(model.port.captures == 1U && model.controller.state() == State::Fresh);
    model.run('t'); const ByteView prepared = frame(model, "BVT1", 48U);
    uint8_t digest[kSha256Size] = {}; CHECK(sha256(ByteView(model.port.raw, sizeof(model.port.raw)), MutableByteSpan(digest, sizeof(digest))));
    CHECK(std::memcmp(prepared.data, model.port.instance, 16U) == 0 && std::memcmp(prepared.data + 16U, digest, 32U) == 0);
    CHECK(model.controller.state() == State::Prepared);
    restore(model, request()); CHECK(sha256(ByteView(model.port.raw, sizeof(model.port.raw)), MutableByteSpan(digest, sizeof(digest))));
    CHECK(std::memcmp(frame(model, "BVA1", 32U).data, digest, 32U) == 0);
    CHECK(model.controller.state() == State::Restored && model.port.restores == 1U);
    model.run('p'); CHECK(frame(model, "BVI1", 96U).data[95U] == 2U && model.port.calls == 3U);
    model.run('t'); error(model, ErrorCode::StateConflict); CHECK(model.port.calls == 3U);
}
void malformedAndLocked() {
    for (unsigned kind = 0U; kind < 7U; ++kind) {
        Model model; std::vector<uint8_t> value = request(); ErrorCode expected = ErrorCode::InvalidHeader;
        if (kind == 0U) value[0U] = 'X';
        if (kind == 1U) value[5U] = 0x20U;
        if (kind == 2U) { value[6U] ^= 1U; expected = ErrorCode::InvalidEncoding; }
        if (kind == 3U) { value.pop_back(); expected = ErrorCode::TruncatedInput; }
        if (kind == 4U) { value.resize(8U); expected = ErrorCode::TruncatedInput; }
        if (kind == 5U) { value.push_back('p'); expected = ErrorCode::TrailingData; }
        if (kind == 6U) { value.clear(); expected = ErrorCode::TruncatedInput; model.stream.tick = 0xfffffff0U; }
        restore(model, value); error(model, expected); CHECK(model.port.calls == 0U);
        // No resync: buffered malformed request bytes remain, and are not
        // scanned for a future command. Explicitly closing/opening the host
        // stream is insufficient to clear the controller's failure state.
        model.stream.position = model.stream.input.size(); model.run('t'); error(model, ErrorCode::StateConflict);
        CHECK(model.port.calls == 0U); model.run('p'); CHECK(frame(model, "BVI1", 96U).data[95U] == 3U);
    }
    Model unknown; unknown.run('x'); error(unknown, ErrorCode::InvalidArgument); CHECK(unknown.port.calls == 0U);
    for (char command : {'p', 'd', 't'}) {
        Model pipelined; pipelined.stream.input = {static_cast<uint8_t>(command), 'p'};
        pipelined.adapter.service(); error(pipelined, ErrorCode::TrailingData); CHECK(pipelined.port.calls == 0U);
    }
}
void failedCaptureAndProfile() {
    for (char command : {'d', 't', 'r'}) {
        Model model; model.port.result = Result::failure(ErrorCode::InternalError);
        if (command == 'r') restore(model, request()); else model.run(command);
        error(model, ErrorCode::InternalError); CHECK(model.port.calls == 1U);
        model.run('d'); error(model, ErrorCode::StateConflict); CHECK(model.port.calls == 1U);
    }
    for (unsigned kind = 0U; kind < 3U; ++kind) {
        Model model;
        if (kind == 0U) model.port.uidValid = false;
        if (kind == 1U) model.port.shortRaw = true;
        if (kind == 2U) model.port.shortInstance = true;
        model.run(kind == 0U ? 'p' : 't'); error(model, kind == 0U ? ErrorCode::NotConfigured : ErrorCode::StateConflict);
        CHECK(model.port.calls == (kind == 0U ? 0U : 1U));
    }
    Model compileOnly(false); compileOnly.run('p'); frame(compileOnly, "BVI1", 96U);
    compileOnly.run('t'); error(compileOnly, ErrorCode::NotConfigured); CHECK(compileOnly.port.calls == 0U);
    Model compileRead(false); compileRead.run('d'); error(compileRead, ErrorCode::NotConfigured); CHECK(compileRead.port.calls == 0U);
    Model compileRestore(false); restore(compileRestore, request()); error(compileRestore, ErrorCode::NotConfigured); CHECK(compileRestore.port.calls == 0U);
}
void transportAndReentry() {
    Model partial; partial.stream.failAfter = 13U; partial.run('d');
    CHECK(partial.stream.output.size() == 13U && partial.controller.state() == State::Failed);
    CHECK(partial.port.calls == 1U && std::memcmp(partial.stream.output.data(), "BVS1", 4U) == 0);
    partial.stream.failAfter = static_cast<size_t>(-1); partial.run('d'); error(partial, ErrorCode::StateConflict); CHECK(partial.port.calls == 1U);
    Model blocked; blocked.stream.freeSpace = 0; blocked.run('p');
    CHECK(blocked.stream.tick == SerialAdapter::timeoutMs && blocked.stream.output.empty() && blocked.controller.state() == State::Failed);
    Model callback; callback.port.callback = [&]() { callback.adapter.service(); };
    callback.run('t'); error(callback, ErrorCode::StateConflict); CHECK(callback.port.calls == 1U);
    Model duringSend; duringSend.stream.onWrite = [&]() { duringSend.adapter.service(); };
    duringSend.run('t'); CHECK(duringSend.controller.state() == State::Failed);
    CHECK(duringSend.stream.output.size() == duringSend.stream.maximumWrite); // No completed success ACK.
    duringSend.stream.onWrite = nullptr; duringSend.run('p');
    CHECK(frame(duringSend, "BVI1", 96U).data[95U] == 3U && duringSend.port.calls == 1U);
    Model lateTrailing; lateTrailing.port.callback = [&]() { lateTrailing.stream.input.push_back('p'); };
    lateTrailing.run('t'); error(lateTrailing, ErrorCode::TrailingData); CHECK(lateTrailing.port.calls == 1U);
}
void finalByteDeadlines() {
    for (unsigned finalRead : {11U, 16395U}) { // command + header, or command + header + raw.
        Model model; model.stream.lateReadCall = finalRead;
        restore(model, request()); error(model, ErrorCode::TruncatedInput); CHECK(model.port.calls == 0U);
        CHECK(model.stream.tick == SerialAdapter::timeoutMs);
    }
    for (unsigned finalWrite : {2U, 9U}) { // Header-last, or 48-byte payload-last (7-byte writes).
        Model model; model.stream.lateWriteCall = finalWrite; model.run('t');
        CHECK(model.controller.state() == State::Failed && model.port.calls == 1U);
        CHECK(model.stream.tick == SerialAdapter::timeoutMs);
        CHECK(model.stream.output.size() == (finalWrite == 2U ? 10U : 58U));
        CHECK(std::memcmp(model.stream.output.data(), "BVT1", 4U) == 0);
        // Already transmitted bytes cannot be recalled. No second error or
        // retry frame is appended, even if the late final write delivered all
        // bytes. The host must confirm the controller state before sealing.
        model.run('p'); CHECK(frame(model, "BVI1", 96U).data[95U] == 3U);
    }
}
} // namespace
int main() {
    metadataAndHappyPath(); malformedAndLocked(); failedCaptureAndProfile(); transportAndReentry(); finalByteDeadlines();
    std::cout << "Wio identity serial adapter finite protocol model passed (no hardware)\n";
    return 0;
}
