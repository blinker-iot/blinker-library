#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspiOwner.ipp>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.inc>
#include "../internal/arduino/WioTerminalTaskColdRecovery/ColdRecoveryPort.h"
#include <BlinkerV2/core/Crc32.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::abort(); } } while (false)
using namespace blinker;
using namespace wio_task_cold;
namespace {
uint32_t tick = 0U;
uint32_t expireAfterNextClock = 0U;
uint32_t now() {
    const uint32_t value = tick;
    tick += expireAfterNextClock; expireAfterNextClock = 0U;
    return value;
}
static const char uid[] = "0123456789abcdef0123456789abcdef";
class Flash final : public IWioTerminalQspiFlash {
public:
    std::vector<uint8_t> bytes;
    unsigned begins = 0U, ends = 0U, reads = 0U, erases = 0U, programs = 0U;
    unsigned failRead = 0U, lateRead = 0U, changeRead = 0U;
    unsigned expireCloseBegin = 0U, captureEnds = 0U;
    bool changeFinalCapture = false, failFinalCapture = false;
    std::function<void()> callback;
    Flash() : bytes(WioTerminalQspiOwner::expectedCapacity, uint8_t(0xffU)) {}
    bool begin(uint32_t& size) override {
        ++begins;
        if (begins == 3U && changeFinalCapture) bytes[ColdRecoveryPort::taskBase + 4095U] ^= 1U;
        if (begins == 3U && failFinalCapture) failRead = reads + 1U;
        size = static_cast<uint32_t>(bytes.size()); return true;
    }
    void end() override { ++ends; }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        ++reads;
        if (reads == changeRead) bytes[ColdRecoveryPort::taskBase + 10U] ^= 1U;
        std::memcpy(output, bytes.data() + address, size);
        if (callback) callback();
        if (reads == lateRead) tick += 10000U;
        if (begins == expireCloseBegin && size == 256U && address == bytes.size() - size &&
            ++captureEnds == (begins == 1U ? 2U : 1U)) expireAfterNextClock = 10000U;
        return reads != failRead;
    }
    // Only the test setup uses these. Every observation checks both stay zero.
    bool erase(uint32_t address) override {
        ++erases; std::fill(bytes.begin() + address, bytes.begin() + address + 4096U, uint8_t(0xffU)); return true;
    }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        ++programs; for (size_t i = 0U; i < size; ++i) bytes[address + i] &= input[i]; return true;
    }
    void resetCounts() {
        begins = ends = reads = erases = programs = captureEnds = 0U;
        tick = expireAfterNextClock = 0U;
    }
    void noWrites() const { CHECK(erases == 0U && programs == 0U); }
};
void big32(uint8_t* p, uint32_t value) {
    for (size_t i = 0U; i < 4U; ++i) p[i] = static_cast<uint8_t>(value >> ((3U - i) * 8U));
}
std::string sha(const uint8_t* bytes, size_t size) {
    uint8_t output[kSha256Size] = {}; CHECK(sha256(ByteView(bytes, size), MutableByteSpan(output, sizeof(output))));
    static const char digits[] = "0123456789abcdef"; std::string result;
    for (size_t i = 0U; i < sizeof(output); ++i) {
        result.push_back(digits[output[i] >> 4U]); result.push_back(digits[output[i] & 15U]);
    }
    return result;
}
std::string current(const Flash& flash) {
    return sha(flash.bytes.data() + ColdRecoveryPort::taskBase, ColdRecoveryPort::regionBytes * 2U);
}
void identity(Flash& flash) {
    WioTerminalQspiOwner owner(flash, now); WioTerminalFlashBlobBank bank(owner); CHECK(bank.begin());
    uint8_t anchor[28U] = {}; std::memcpy(anchor, "BDN2", 4U); anchor[4] = 1U;
    std::memset(anchor + 8U, 0x51U, 16U); big32(anchor + 24U, computeCrc32(ByteView(anchor, 24U)));
    CHECK(bank.deviceInstanceBlob().replace(ByteView(anchor, sizeof(anchor)))); bank.end();
}
std::vector<uint8_t> fixture(unsigned variant = 0U) {
    TaskSnapshotHeader header;
    std::memset(header.authority.deviceInstanceId, 0x51U, 16U);
    std::memcpy(header.authority.taskScope, "WioStorageProbe!", 16U);
    std::memset(header.authority.rootDigest, 0xa5U, 32U);
    header.tableRevision = 1U; header.nextTaskId = 5U;
    std::memcpy(header.requestId, "WIO1", 4U); header.requestId[15] = 1U;
    std::memset(header.requestDigest, 0x31U, 32U);
    TaskSnapshotRecord records[4U];
    for (size_t i = 0U; i < 4U; ++i) {
        records[i].taskId = static_cast<uint32_t>(i + 1U); records[i].definitionRevision = 1U;
        records[i].timing.durationMillis = 1000U;
        records[i].key[0] = 'p'; records[i].key[1] = static_cast<uint8_t>('1' + i); records[i].keyLength = 2U;
        records[i].value[0] = 0xf4U; records[i].valueLength = 1U;
    }
    if (variant == 1U) header.authority.taskScope[0] ^= 1U;
    if (variant == 2U) records[0].enabled = true;
    if (variant == 3U) { records[0].progress = TaskProgress::Interrupted; records[0].startSequence = 1U; }
    if (variant == 4U) header.authority.deviceInstanceId[0] ^= 1U;
    if (variant == 5U) records[0].value[0] = 0xf5U;
    if (variant == 6U) header.requestDigest[0] ^= 1U;
    std::vector<uint8_t> output(644U); size_t written = 0U;
    CHECK(TaskSnapshotCodec::encode(header, records, 4U, MutableByteSpan(output.data(), output.size()), written));
    CHECK(written == output.size()); return output;
}
void task(Flash& flash, unsigned variant = 0U, bool tombstone = false) {
    WioTerminalQspiOwner owner(flash, now); WioTerminalFlashBlobBank bank(owner);
    WioTerminalTaskSnapshotBlobStore<4U> tasks(bank, true); CHECK(tasks.begin());
    const auto value = fixture(variant); CHECK(tasks.replace(ByteView(value.data(), value.size())));
    if (tombstone) CHECK(tasks.clear()); tasks.end();
}
struct Config {
    std::string source, original, identityHash, currentHash, intent;
    ColdRecoveryBinding binding;
    Config(const Flash& flash, const std::string& originalHash)
        : source(64U, 'a'), original(originalHash),
          identityHash(sha(flash.bytes.data() + ColdRecoveryPort::identityBase, ColdRecoveryPort::regionBytes)),
          currentHash(current(flash)), intent(64U, 'b'),
          binding{source.c_str(), original.c_str(), identityHash.c_str(), currentHash.c_str(), intent.c_str(), uid} {}
};
struct Reader final : ColdRecoveryReader {
    unsigned calls = 0U;
    Result result;
    bool complete = true, present = true;
    std::function<void()> callback;
    Result observe() override { ++calls; if (callback) callback(); return result; }
    bool observed() const override { return complete; }
    bool hasSnapshot() const override { return present; }
};
void controllerAdmission() {
    Reader reader; ColdRecoveryController controller(reader);
    CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status)); CHECK(reader.calls == 0U);
    CHECK(controller.execute(Command::Observe)); CHECK(controller.state() == State::ObservedSnapshot);
    CHECK(!controller.execute(Command::Observe)); CHECK(reader.calls == 1U);
    Reader missing; missing.result = Result::failure(ErrorCode::NotFound); missing.present = false;
    ColdRecoveryController absent(missing); CHECK(absent.execute(Command::Observe).code() == ErrorCode::NotFound);
    CHECK(absent.state() == State::ObservedNotFound); CHECK(!absent.execute(Command::Observe)); CHECK(missing.calls == 1U);
    Reader malformed; malformed.result = Result::failure(ErrorCode::NotFound); malformed.complete = false;
    ColdRecoveryController bad(malformed); CHECK(!bad.execute(Command::Observe)); CHECK(bad.state() == State::Failed);
    Reader nested; ColdRecoveryController nesting(nested);
    nested.callback = [&]() { CHECK(!nesting.execute(Command::Observe)); CHECK(nesting.execute(Command::Ping)); };
    CHECK(nesting.execute(Command::Observe)); CHECK(nested.calls == 1U);
    Reader invalid; ColdRecoveryController invalidated(invalid);
    invalid.callback = [&]() { invalidated.malformedInput(); };
    CHECK(!invalidated.execute(Command::Observe)); CHECK(invalidated.state() == State::Failed);
    CHECK(!invalidated.execute(Command::Observe)); CHECK(invalid.calls == 1U);
}
void successfulObservation(bool tombstone, bool empty = false) {
    static_assert(ColdRecoveryPort::taskBase == 0x3fc000UL &&
                  ColdRecoveryPort::identityBase == 0x3fe000UL && ColdRecoveryPort::regionBytes == 8192U,
                  "Cold observation must preserve the shared fixed layout");
    Flash flash; identity(flash); const std::string original = current(flash);
    if (!empty) task(flash, 0U, tombstone);
    Config config(flash, original); const auto preserved = flash.bytes; flash.resetCounts();
    WioTerminalQspiOwner owner(flash, now); ColdRecoveryPort port(owner, now, config.binding, uid);
    ColdRecoveryController controller(port); CHECK(flash.begins == 0U && flash.reads == 0U);
    CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status)); CHECK(flash.reads == 0U);
    const Result result = controller.execute(Command::Observe);
    if (tombstone || empty) {
        CHECK(result.code() == ErrorCode::NotFound && controller.state() == State::ObservedNotFound);
        CHECK(port.observed() && !port.hasSnapshot());
    } else {
        CHECK(result && controller.state() == State::ObservedSnapshot && port.hasSnapshot());
        const auto expected = fixture();
        uint8_t digest[kSha256Size] = {};
        CHECK(sha256(ByteView(expected.data(), expected.size()), MutableByteSpan(digest, sizeof(digest))));
        CHECK(std::memcmp(digest, port.payloadDigest(), sizeof(digest)) == 0);
    }
    CHECK(port.identitySame() && flash.bytes == preserved); flash.noWrites();
    const unsigned reads = flash.reads;
    CHECK(!controller.execute(Command::Observe)); CHECK(controller.execute(Command::Status)); CHECK(flash.reads == reads);
}
void bindingFailures() {
    for (unsigned kind = 0U; kind < 6U; ++kind) {
        Flash flash; identity(flash); const std::string original = current(flash); task(flash);
        Config config(flash, original); const std::string wrongDigest(64U, '0');
        if (kind == 0U) config.binding.boardUid = "1123456789abcdef0123456789abcdef";
        if (kind == 1U) config.binding.currentSha256 = wrongDigest.c_str();
        if (kind == 2U) config.binding.identitySha256 = wrongDigest.c_str();
        if (kind == 3U) config.binding.intentSha256 = "invalid";
        if (kind == 4U) config.binding.baselineSha256 = "invalid";
        if (kind == 5U) config.binding.sourceId = "invalid";
        flash.resetCounts(); WioTerminalQspiOwner owner(flash, now);
        ColdRecoveryPort port(owner, now, config.binding, uid); ColdRecoveryController controller(port);
        CHECK(!controller.execute(Command::Observe)); CHECK(controller.state() == State::Failed && !port.observed());
        if (kind == 0U || kind >= 3U) CHECK(flash.begins == 0U && flash.reads == 0U);
        const unsigned reads = flash.reads; CHECK(!controller.execute(Command::Observe)); CHECK(flash.reads == reads);
        flash.noWrites();
    }
}
void unstableAndNativeFailure(unsigned kind) {
    Flash flash; identity(flash); const std::string original = current(flash); task(flash);
    Config config(flash, original); flash.resetCounts();
    if (kind == 0U) flash.changeRead = 65U;
    if (kind == 1U) flash.failRead = 1U;
    if (kind == 2U) flash.lateRead = 1U;
    WioTerminalQspiOwner owner(flash, now); ColdRecoveryPort port(owner, now, config.binding, uid);
    ColdRecoveryController controller(port); CHECK(!controller.execute(Command::Observe));
    CHECK(controller.state() == State::Failed && !port.observed());
    if (kind != 0U) CHECK(owner.quarantined() && flash.reads == 1U && flash.ends == 0U);
    const unsigned reads = flash.reads;
    CHECK(!controller.execute(Command::Observe)); CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status));
    CHECK(flash.reads == reads); flash.noWrites();
}
void semanticFailures() {
    for (unsigned variant = 1U; variant <= 6U; ++variant) {
        Flash flash; identity(flash); const std::string original = current(flash); task(flash, variant);
        Config config(flash, original); flash.resetCounts(); WioTerminalQspiOwner owner(flash, now);
        ColdRecoveryPort port(owner, now, config.binding, uid); ColdRecoveryController controller(port);
        CHECK(controller.execute(Command::Observe).code() == ErrorCode::InvalidEncoding);
        CHECK(controller.state() == State::Failed && !port.observed()); flash.noWrites();
    }
}
void peerPolicies(unsigned kind, ErrorCode expected) {
    Flash flash; identity(flash); const std::string original = current(flash); task(flash);
    uint8_t* first = flash.bytes.data() + ColdRecoveryPort::taskBase;
    uint8_t* second = first + 4096U;
    if (kind == 0U) first[660U] ^= 1U; // sole CRC-invalid record is NOT empty
    else if (kind == 1U) second[0] = 0U; // foreign peer region
    else {
        std::memcpy(second, first, 664U);
        if (kind == 2U) second[4] = 2U;
        if (kind == 3U) { second[12] = 2U; second[13] = 0x85U; }
        if (kind == 5U) big32(second + 8U, 2U); // recognizable torn newer record, original policy picks intact peer
        // kind4 is exact duplicate generation: original policy rejects it.
    }
    Config config(flash, original); flash.resetCounts(); WioTerminalQspiOwner owner(flash, now);
    ColdRecoveryPort port(owner, now, config.binding, uid); ColdRecoveryController controller(port);
    const Result result = controller.execute(Command::Observe); CHECK(result.code() == expected);
    CHECK(controller.state() == (expected == ErrorCode::Ok ? State::ObservedSnapshot : State::Failed));
    flash.noWrites();
}
void realNativeReentrancy() {
    Flash flash; identity(flash); const std::string original = current(flash); task(flash);
    Config config(flash, original); flash.resetCounts(); WioTerminalQspiOwner owner(flash, now);
    ColdRecoveryPort port(owner, now, config.binding, uid); ColdRecoveryController controller(port);
    flash.callback = [&]() {
        const unsigned reads = flash.reads;
        CHECK(!controller.execute(Command::Observe)); CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status));
        CHECK(!port.observe());
        CHECK(flash.reads == reads);
    };
    CHECK(controller.execute(Command::Observe)); CHECK(controller.state() == State::ObservedSnapshot); flash.noWrites();
}
void finalRawMustStillMatch(bool nativeFailure) {
    Flash flash; identity(flash); const std::string original = current(flash); task(flash);
    Config config(flash, original); flash.resetCounts();
    flash.changeFinalCapture = !nativeFailure; flash.failFinalCapture = nativeFailure;
    WioTerminalQspiOwner owner(flash, now); ColdRecoveryPort port(owner, now, config.binding, uid);
    ColdRecoveryController controller(port);
    CHECK(!controller.execute(Command::Observe)); CHECK(controller.state() == State::Failed && !port.observed());
    CHECK(flash.begins == 3U); if (nativeFailure) CHECK(owner.quarantined());
    const unsigned reads = flash.reads; CHECK(!controller.execute(Command::Observe)); CHECK(flash.reads == reads);
    flash.noWrites();
}
void closeAfterSuccessfulCapture(bool finalCapture) {
    Flash flash; identity(flash); const std::string original = current(flash); task(flash);
    Config config(flash, original); flash.resetCounts(); flash.expireCloseBegin = finalCapture ? 3U : 1U;
    WioTerminalQspiOwner owner(flash, now); ColdRecoveryPort port(owner, now, config.binding, uid);
    ColdRecoveryController controller(port);
    CHECK(!controller.execute(Command::Observe));
    CHECK(controller.state() == State::Failed && !port.observed() && !port.identitySame());
    CHECK(owner.quarantined() && flash.begins == flash.expireCloseBegin);
    // The close that discovered the deadline issues no native cleanup.
    CHECK(flash.ends == (finalCapture ? 2U : 0U));
    const unsigned reads = flash.reads, ends = flash.ends;
    CHECK(!controller.execute(Command::Observe)); CHECK(controller.execute(Command::Status));
    CHECK(flash.reads == reads && flash.ends == ends); flash.noWrites();
}
}
int main() {
    controllerAdmission(); successfulObservation(false); successfulObservation(true); successfulObservation(false, true);
    bindingFailures(); unstableAndNativeFailure(0U); unstableAndNativeFailure(1U); unstableAndNativeFailure(2U);
    semanticFailures(); peerPolicies(0U, ErrorCode::InvalidEncoding); peerPolicies(1U, ErrorCode::StateConflict);
    peerPolicies(2U, ErrorCode::UnsupportedVersion); peerPolicies(3U, ErrorCode::CapacityExceeded);
    peerPolicies(4U, ErrorCode::SequenceConflict); peerPolicies(5U, ErrorCode::Ok); realNativeReentrancy();
    finalRawMustStillMatch(false); finalRawMustStillMatch(true);
    closeAfterSuccessfulCapture(false); closeAfterSuccessfulCapture(true);
    std::cout << "Wio cold recovery read-only models passed; no physical power-loss evidence\n";
}
