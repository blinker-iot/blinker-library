#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspiOwner.ipp>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.inc>
#include "../internal/arduino/WioTerminalIdentityTransitionGate/TransitionStorage.h"
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
using namespace wio_identity_transition;
namespace {
uint32_t tick = 0U;
unsigned clockReadsBeforeJump = 0U;
uint32_t now() {
    if (clockReadsBeforeJump != 0U && --clockReadsBeforeJump == 0U) tick += 10000U;
    return tick;
}
const char uid[] = "0123456789abcdef0123456789abcdef";
const uint32_t base = WioTerminalQspiOwner::taskAddress;
const size_t rawSize = TransitionStorage::rawSize;

// No hardware is involved. Setup uses production bank encoders; mutation
// counters are reset before each gate operation, never before a failed retry.
class Flash final : public IWioTerminalQspiFlash {
public:
    std::vector<uint8_t> bytes;
    unsigned begins = 0U, ends = 0U, reads = 0U, erases = 0U, programs = 0U;
    unsigned failRead = 0U, changeRead = 0U, lateRead = 0U;
    unsigned failErase = 0U, failProgram = 0U, lateProgram = 0U;
    unsigned changeBegin = 0U;
    unsigned expireEndBegin = 0U;
    unsigned expireEndPrograms = 0U;
    uint32_t expireEndAddress = WioTerminalQspiOwner::expectedCapacity - 64U;
    std::function<void()> callback;
    Flash() : bytes(WioTerminalQspiOwner::expectedCapacity, uint8_t(0xffU)) {}
    bool begin(uint32_t& capacity) override {
        ++begins;
        if (begins == changeBegin) bytes[base + 11U] ^= 1U;
        capacity = static_cast<uint32_t>(bytes.size()); return true;
    }
    void end() override { ++ends; }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        CHECK(address >= base && size <= bytes.size() - address);
        ++reads;
        if (reads == changeRead) bytes[base + 10U] ^= 1U;
        std::memcpy(output, bytes.data() + address, size);
        if (callback) callback();
        if (reads == lateRead) tick += 10000U;
        if (begins == expireEndBegin && programs == expireEndPrograms && address == expireEndAddress && size == 64U) {
            // The first Now call is the successful last owner.complete;
            // only the next call, end's remainingBudget, crosses the deadline.
            clockReadsBeforeJump = 2U; expireEndBegin = 0U;
        }
        return reads != failRead;
    }
    bool erase(uint32_t address) override {
        CHECK(address >= base && address % 4096U == 0U && address + 4096U <= bytes.size());
        ++erases;
        std::fill(bytes.begin() + address, bytes.begin() + address + 4096U, uint8_t(0xffU));
        return erases != failErase; // false may still have reached the medium
    }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        CHECK(address >= base && size <= bytes.size() - address);
        ++programs;
        for (size_t i = 0U; i < size; ++i) bytes[address + i] &= input[i];
        if (programs == lateProgram) tick += 10000U;
        return programs != failProgram; // deliberately applied-but-unconfirmed
    }
    void resetCounts() { begins = ends = reads = erases = programs = 0U; tick = 0U; clockReadsBeforeJump = 0U; }
    unsigned calls() const { return begins + ends + reads + erases + programs; }
    void noWrites() const { CHECK(erases == 0U && programs == 0U); }
    std::vector<uint8_t> raw() const {
        return std::vector<uint8_t>(bytes.begin() + base, bytes.end());
    }
};
class Random final : public IRandom {
public:
    unsigned calls = 0U;
    uint8_t value = 0x73U;
    bool fail = false;
    Result fill(MutableByteSpan output) override {
        ++calls; std::memset(output.data, value, output.size);
        return fail ? Result::failure(ErrorCode::InternalError) : Result::success();
    }
};
void big32(uint8_t* output, uint32_t value) {
    for (size_t i = 0U; i < 4U; ++i) output[i] = static_cast<uint8_t>(value >> ((3U - i) * 8U));
}
std::string sha(ByteView value) {
    uint8_t digest[kSha256Size] = {};
    CHECK(sha256(value, MutableByteSpan(digest, sizeof(digest))));
    static const char hex[] = "0123456789abcdef";
    std::string output;
    for (size_t i = 0U; i < sizeof(digest); ++i) {
        output.push_back(hex[digest[i] >> 4U]); output.push_back(hex[digest[i] & 15U]);
    }
    return output;
}
void originalIdentity(Flash& flash, bool optional = true, bool valid = true) {
    WioTerminalQspiOwner owner(flash, now); WioTerminalFlashBlobBank bank(owner); CHECK(bank.begin());
    uint8_t anchor[28U] = {}; std::memcpy(anchor, "BDN2", 4U); anchor[4] = 1U;
    std::memset(anchor + 8U, 0x51U, 16U); big32(anchor + 24U, computeCrc32(ByteView(anchor, 24U)));
    if (!valid) anchor[24U] ^= 1U;
    CHECK(bank.deviceInstanceBlob().replace(ByteView(anchor, sizeof(anchor))));
    if (optional) {
        const uint8_t value = 0x42U;
        IAtomicBlobStore* blobs[] = {&bank.wifiSelectorBlob(), &bank.wifiSlot0Blob(),
                                    &bank.wifiSlot1Blob(), &bank.deviceAccessBlob()};
        for (auto* blob : blobs) CHECK(blob->replace(ByteView(&value, 1U)));
    }
    bank.end();
}
struct Config {
    std::vector<uint8_t> original;
    std::string source, originalHash;
    TransitionBinding binding;
    explicit Config(const Flash& flash)
        : original(flash.raw()), source(64U, 'a'),
          originalHash(sha(ByteView(original.data(), original.size()))),
          binding{source.c_str(), originalHash.c_str(), uid} {}
    ByteView bytes() const { return ByteView(original.data(), original.size()); }
};
void inspectTemporary(Flash& flash, const DeviceInstanceId& expected) {
    WioTerminalQspiOwner owner(flash, now); WioTerminalFlashBlobBank bank(owner); CHECK(bank.begin());
    DeviceInstanceId actual; DeviceInstanceIdStore identity(bank.deviceInstanceBlob());
    CHECK(identity.load(actual)); CHECK(sameDeviceInstanceId(actual, expected));
    uint8_t output[260U] = {}; size_t written = 99U;
    IAtomicBlobStore* blobs[] = {&bank.wifiSelectorBlob(), &bank.wifiSlot0Blob(),
                                &bank.wifiSlot1Blob(), &bank.deviceAccessBlob()};
    for (auto* blob : blobs) {
        CHECK(blob->load(MutableByteSpan(output, sizeof(output)), written).code() == ErrorCode::NotFound);
        CHECK(written == 0U);
    }
    bank.end();
}
struct ModelPort final : TransitionPort {
    unsigned calls = 0U;
    bool available = true;
    Result result;
    std::function<void()> callback;
    bool healthy() const override { return available; }
    Result act() { ++calls; if (callback) callback(); return result; }
    Result prepareTemporary() override { return act(); }
    Result captureCurrent() override { return act(); }
    Result restoreOriginal(ByteView) override { return act(); }
};
void controllerContract() {
    ModelPort port; TransitionController controller(port);
    CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status)); CHECK(port.calls == 0U);
    CHECK(controller.execute(Command::CaptureCurrent)); CHECK(controller.state() == State::Fresh);
    CHECK(!controller.execute(Command::CaptureCurrent)); CHECK(port.calls == 1U);
    CHECK(controller.execute(Command::PrepareTemp)); CHECK(controller.state() == State::Prepared);
    CHECK(!controller.execute(Command::PrepareTemp)); CHECK(port.calls == 2U);
    CHECK(controller.execute(Command::RestoreOriginal)); CHECK(controller.state() == State::Restored);
    CHECK(!controller.execute(Command::RestoreOriginal)); CHECK(port.calls == 3U);
    ModelPort native; native.available = false; TransitionController unavailable(native);
    CHECK(!unavailable.execute(Command::PrepareTemp)); CHECK(unavailable.state() == State::Failed && native.calls == 0U);
    ModelPort failed; failed.result = Result::failure(ErrorCode::InternalError); TransitionController lock(failed);
    CHECK(!lock.execute(Command::PrepareTemp)); CHECK(!lock.execute(Command::RestoreOriginal)); CHECK(failed.calls == 1U);
    ModelPort nested; TransitionController guarded(nested);
    nested.callback = [&]() {
        CHECK(!guarded.execute(Command::PrepareTemp)); CHECK(!guarded.execute(Command::RestoreOriginal));
        CHECK(guarded.execute(Command::Status));
    };
    CHECK(guarded.execute(Command::PrepareTemp)); CHECK(nested.calls == 1U);
    ModelPort malformed; TransitionController invalidated(malformed);
    malformed.callback = [&]() { invalidated.malformedInput(); };
    CHECK(!invalidated.execute(Command::PrepareTemp)); CHECK(invalidated.state() == State::Failed);
    CHECK(!invalidated.execute(Command::RestoreOriginal)); CHECK(malformed.calls == 1U);
    CHECK(invalidated.execute(Command::Ping)); CHECK(invalidated.execute(Command::Status));
}
void successfulRoundTrip(bool optional) {
    Flash flash; originalIdentity(flash, optional); Config config(flash); Random random;
    const auto fullOriginal = flash.bytes; flash.resetCounts();
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    TransitionController controller(port);
    CHECK(flash.calls() == 0U); CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status));
    CHECK(flash.calls() == 0U);
    CHECK(controller.execute(Command::PrepareTemp)); CHECK(controller.state() == State::Prepared);
    CHECK(random.calls == 1U && port.temporaryInstance().bytes[0U] == random.value);
    CHECK(flash.programs == (optional ? 6U : 2U));
    CHECK(port.capturedRaw().size == rawSize);
    CHECK(std::all_of(flash.bytes.begin() + base, flash.bytes.begin() + base + 8192U,
                     [](uint8_t value) { return value == 0xffU; }));
    inspectTemporary(flash, port.temporaryInstance());
    CHECK(controller.execute(Command::RestoreOriginal, config.bytes()));
    CHECK(controller.state() == State::Restored && port.healthy() && flash.bytes == fullOriginal);
    const unsigned before = flash.calls();
    CHECK(!controller.execute(Command::PrepareTemp)); CHECK(!controller.execute(Command::RestoreOriginal, config.bytes()));
    CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status)); CHECK(flash.calls() == before);
}
void bindingAndOriginalGuards() {
    for (unsigned kind = 0U; kind < 7U; ++kind) {
        Flash flash; originalIdentity(flash); Config config(flash); Random random;
        const std::string wrong(64U, '0');
        if (kind == 0U) config.binding.boardUid = "1123456789abcdef0123456789abcdef";
        if (kind == 1U) config.binding.originalSha256 = wrong.c_str();
        if (kind == 2U) config.binding.sourceId = "invalid";
        if (kind == 3U) config.binding.originalSha256 = "invalid";
        if (kind == 4U) config.binding.boardUid = "invalid";
        if (kind == 5U) config.binding.sourceId = nullptr;
        const char* actual = kind == 6U ? "invalid" : uid;
        flash.resetCounts(); WioTerminalQspiOwner owner(flash, now);
        TransitionStorage port(owner, now, random, config.binding, actual); TransitionController controller(port);
        CHECK(!controller.execute(Command::PrepareTemp)); CHECK(controller.state() == State::Failed && !port.healthy());
        flash.noWrites(); CHECK(random.calls == 0U);
        if (kind != 1U) CHECK(flash.calls() == 0U);
        const unsigned before = flash.calls(); CHECK(!controller.execute(Command::RestoreOriginal, config.bytes()));
        CHECK(!port.captureCurrent()); CHECK(flash.calls() == before);
    }
    for (unsigned kind = 0U; kind < 3U; ++kind) {
        Flash flash;
        if (kind != 2U) originalIdentity(flash, false, kind != 1U);
        if (kind == 0U) flash.bytes[base] = 0U; // original nonblank tasks must not be retired
        Config config(flash); Random random; flash.resetCounts(); WioTerminalQspiOwner owner(flash, now);
        TransitionStorage port(owner, now, random, config.binding, uid);
        CHECK(!port.prepareTemporary()); CHECK(!port.healthy()); flash.noWrites(); CHECK(random.calls == 0U);
    }
}
void captureFailure(unsigned kind) {
    Flash flash; originalIdentity(flash); Config config(flash); Random random; flash.resetCounts();
    if (kind == 0U) flash.changeRead = 3U; // first independent second-read chunk sees a changed task byte
    if (kind == 1U) flash.failRead = 1U;
    if (kind == 2U) flash.lateRead = 1U;
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    CHECK(!port.prepareTemporary()); CHECK(!port.healthy() && port.capturedRaw().size == 0U); flash.noWrites();
    const unsigned before = flash.calls();
    CHECK(!port.captureCurrent()); CHECK(!port.restoreOriginal(config.bytes())); CHECK(!port.prepareTemporary());
    CHECK(flash.calls() == before);
    if (kind != 0U) CHECK(owner.quarantined() && flash.ends == 0U && flash.reads == 1U);
}
void prepareFailureWindows(unsigned kind, unsigned mutation) {
    Flash flash; originalIdentity(flash); Config config(flash); Random random; flash.resetCounts();
    if (kind == 0U) flash.failProgram = mutation;
    if (kind == 1U) flash.failErase = mutation;
    if (kind == 2U) flash.lateProgram = mutation;
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    TransitionController controller(port);
    CHECK(!controller.execute(Command::PrepareTemp)); CHECK(controller.state() == State::Failed && owner.quarantined());
    CHECK(flash.erases == mutation && flash.programs == (kind == 1U ? mutation - 1U : mutation));
    CHECK(flash.ends == 1U); // only the healthy pre-mutation capture was closed
    const unsigned before = flash.calls();
    CHECK(!controller.execute(Command::RestoreOriginal, config.bytes())); CHECK(!port.captureCurrent());
    CHECK(controller.execute(Command::Status)); CHECK(flash.calls() == before);
}
void noAnchorRecovery(unsigned randomKind) {
    Flash flash; originalIdentity(flash); Config config(flash); Random random;
    random.fail = randomKind == 0U;
    if (randomKind == 1U) random.value = 0U;
    if (randomKind == 2U) random.value = 0x51U;
    flash.resetCounts();
    {
        WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
        CHECK(!port.prepareTemporary()); CHECK(!port.healthy() && random.calls == 1U);
        CHECK(flash.programs == 5U && flash.erases == 5U);
        const unsigned before = flash.calls(); CHECK(!port.restoreOriginal(config.bytes())); CHECK(flash.calls() == before);
    }
    // A NEW owner is a model of a separately admitted post-power recovery,
    // not evidence of an actual power cycle and not an on-device retry API.
    {
        WioTerminalQspiOwner owner(flash, now); WioTerminalFlashBlobBank bank(owner); CHECK(bank.begin());
        DeviceInstanceId instance; DeviceInstanceIdStore identity(bank.deviceInstanceBlob());
        CHECK(identity.load(instance).code() == ErrorCode::NotFound); bank.end();
    }
    WioTerminalQspiOwner recoveryOwner(flash, now); Random unused;
    TransitionStorage recovery(recoveryOwner, now, unused, config.binding, uid); TransitionController controller(recovery);
    CHECK(controller.execute(Command::CaptureCurrent)); CHECK(recovery.capturedRaw().size == rawSize);
    CHECK(controller.execute(Command::RestoreOriginal, config.bytes())); CHECK(flash.raw() == config.original);
    CHECK(unused.calls == 0U);
}
void restoreGuards(unsigned kind) {
    Flash flash; originalIdentity(flash); Config config(flash); Random random; flash.resetCounts();
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    ByteView desired = config.bytes();
    if (kind == 0U) desired.size -= 1U;
    if (kind == 1U) desired.data = nullptr;
    if (kind == 2U) config.original[0U] ^= 1U;
    if (kind == 3U) { CHECK(port.captureCurrent()); desired = port.capturedRaw(); }
    const unsigned before = flash.calls();
    CHECK(!port.restoreOriginal(desired)); CHECK(!port.healthy()); flash.noWrites(); CHECK(flash.calls() == before);
    CHECK(!port.restoreOriginal(config.bytes())); CHECK(flash.calls() == before);
}
void differentCurrent(Flash& flash) {
    // No anchor/valid BWB2 is required on the recovery side, including when
    // every sector was left torn. The desired ORIGINAL remains SHA-pinned.
    for (size_t offset = 0U; offset < rawSize; offset += 4096U) flash.bytes[base + offset] ^= 1U;
}
void tornRecoveryAndFinalVerification(unsigned kind, unsigned mutation = 1U) {
    Flash flash; originalIdentity(flash); Config config(flash); Random random; differentCurrent(flash); flash.resetCounts();
    if (kind == 1U) flash.failProgram = mutation;
    if (kind == 2U) flash.failErase = mutation;
    if (kind == 3U) flash.lateProgram = mutation;
    if (kind == 4U) flash.changeBegin = 4U; // after all sectors, before final complete double read
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    TransitionController controller(port); const Result result = controller.execute(Command::RestoreOriginal, config.bytes());
    CHECK(random.calls == 0U);
    if (kind == 0U) {
        CHECK(result && port.healthy() && controller.state() == State::Restored);
        CHECK(flash.raw() == config.original && flash.erases == 4U && flash.programs == 64U);
    } else {
        CHECK(!result && !port.healthy() && controller.state() == State::Failed);
        if (kind != 4U) CHECK(owner.quarantined());
        const unsigned before = flash.calls(); CHECK(!port.restoreOriginal(config.bytes())); CHECK(!port.captureCurrent());
        CHECK(controller.execute(Command::Status)); CHECK(flash.calls() == before);
        // A separately admitted, fresh-owner recovery can restore exact old
        // bytes after an applied-but-unconfirmed program or absent BDN2.
        flash.failProgram = flash.failErase = flash.lateProgram = flash.changeBegin = 0U; tick = 0U;
        WioTerminalQspiOwner recoveredOwner(flash, now); TransitionStorage recovered(recoveredOwner, now, random, config.binding, uid);
        CHECK(recovered.restoreOriginal(config.bytes())); CHECK(flash.raw() == config.original);
    }
}
void nativeReentrancy() {
    Flash flash; originalIdentity(flash); Config config(flash); Random random; flash.resetCounts();
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    TransitionController controller(port);
    flash.callback = [&]() {
        const unsigned before = flash.calls();
        CHECK(!controller.execute(Command::PrepareTemp)); CHECK(!controller.execute(Command::RestoreOriginal, config.bytes()));
        CHECK(!port.captureCurrent()); CHECK(!port.prepareTemporary()); CHECK(!port.restoreOriginal(config.bytes()));
        CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status)); CHECK(flash.calls() == before);
    };
    CHECK(controller.execute(Command::PrepareTemp)); CHECK(controller.state() == State::Prepared);
    CHECK(controller.execute(Command::RestoreOriginal, config.bytes())); CHECK(flash.raw() == config.original);
}
void deadlineAtSessionEnd(unsigned phase) {
    Flash flash; originalIdentity(flash); Config config(flash); Random random;
    const bool restore = phase >= 3U;
    if (restore) differentCurrent(flash);
    flash.resetCounts();
    // 0: standalone capture; 1/2: PREPARE pre/post capture;
    // 3..6: RESTORE preflight, task, identity and final capture sessions.
    const unsigned session = phase == 2U ? 3U : (restore ? phase - 2U : 1U);
    flash.expireEndBegin = session;
    flash.expireEndPrograms = phase == 2U ? 6U : (phase >= 4U ? (phase == 4U ? 32U : 64U) : 0U);
    if (phase == 4U) flash.expireEndAddress = WioTerminalQspiOwner::identityAddress - 64U;
    WioTerminalQspiOwner owner(flash, now); TransitionStorage port(owner, now, random, config.binding, uid);
    TransitionController controller(port);
    const Command command = phase == 0U ? Command::CaptureCurrent :
                            (restore ? Command::RestoreOriginal : Command::PrepareTemp);
    CHECK(!controller.execute(command, restore ? config.bytes() : ByteView()));
    CHECK(clockReadsBeforeJump == 0U && tick == 10000U && flash.expireEndBegin == 0U);
    CHECK(controller.state() == State::Failed && !port.healthy() && owner.quarantined());
    CHECK(port.capturedRaw().size == 0U && flash.begins == session && flash.ends == session - 1U);
    if (phase == 0U || phase == 1U || phase == 3U) flash.noWrites();
    if (phase == 2U) CHECK(flash.programs == 6U && random.calls == 1U);
    if (phase == 4U) CHECK(flash.erases == 2U && flash.programs == 32U);
    if (phase == 5U || phase == 6U) {
        CHECK(flash.erases == 4U && flash.programs == 64U);
        CHECK(flash.raw() == config.original); // committed bytes are NOT a confirmed success
    }
    const unsigned before = flash.calls();
    CHECK(!controller.execute(Command::RestoreOriginal, config.bytes())); CHECK(!controller.execute(Command::CaptureCurrent));
    CHECK(!port.captureCurrent()); CHECK(!port.prepareTemporary()); CHECK(!port.restoreOriginal(config.bytes()));
    CHECK(controller.execute(Command::Ping)); CHECK(controller.execute(Command::Status)); CHECK(flash.calls() == before);
}
} // namespace
int main() {
    controllerContract(); successfulRoundTrip(true); successfulRoundTrip(false); bindingAndOriginalGuards();
    for (unsigned kind = 0U; kind < 3U; ++kind) captureFailure(kind);
    for (unsigned kind = 0U; kind < 3U; ++kind)
        for (unsigned mutation = 1U; mutation <= 6U; ++mutation) prepareFailureWindows(kind, mutation);
    for (unsigned kind = 0U; kind < 3U; ++kind) noAnchorRecovery(kind);
    for (unsigned kind = 0U; kind < 4U; ++kind) restoreGuards(kind);
    tornRecoveryAndFinalVerification(0U); tornRecoveryAndFinalVerification(4U);
    for (unsigned slot = 0U; slot < 4U; ++slot) {
        tornRecoveryAndFinalVerification(1U, slot * 16U + 1U);
        tornRecoveryAndFinalVerification(2U, slot + 1U);
        tornRecoveryAndFinalVerification(3U, slot * 16U + 1U);
    }
    nativeReentrancy();
    for (unsigned phase = 0U; phase < 7U; ++phase) deadlineAtSessionEnd(phase);
    std::cout << "Wio identity transition controller/storage model passed (no hardware)\n";
    return 0;
}
