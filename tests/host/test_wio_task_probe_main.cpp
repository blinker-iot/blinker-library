#include "../internal/arduino/WioTerminalTaskStorageProbe/ProbeController.h"
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspiOwner.ipp>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.inc>
#include "../internal/arduino/WioTerminalTaskStorageProbe/TaskStorageProbePort.h"
#include <BlinkerV2/core/Crc32.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::abort(); } } while (false)
using namespace wio_task_probe;
using namespace blinker;
namespace {
class Port final : public ProbePort {
public:
    bool okay = true, present = false, wrongRead = false;
    unsigned calls = 0U, writes = 0U, ends = 0U;
    unsigned failCall = 0U;
    std::function<void()> callback;
    bool healthy() const override { return okay; }
    Result next() {
        ++calls; if (callback) callback();
        return calls == failCall ? Result::failure(ErrorCode::WouldBlock) : Result::success();
    }
    Result captureBaseline() override { return next(); }
    Result attestRegion() override { return next(); }
    Result readFixture(bool expected) override {
        const Result result = next(); if (!result) return result;
        if (wrongRead || present != expected) return Result::failure(ErrorCode::StateConflict);
        return present ? Result::success() : Result::failure(ErrorCode::NotFound);
    }
    Result replaceFixture() override { ++writes; const Result result = next(); if (result) present = true; return result; }
    Result endIdentity() override { ++ends; return next(); }
    Result clearFixture() override { ++writes; const Result result = next(); if (result) present = false; return result; }
};
void admit(ProbeController& c) {
    CHECK(c.execute(Command::OpenCapture)); CHECK(c.state() == State::Captured);
    CHECK(c.execute(Command::RegionAttest)); CHECK(c.state() == State::Admitted);
}
void noAutomaticIo() {
    Port p; ProbeController c(p); CHECK(p.calls == 0U);
    CHECK(c.execute(Command::Ping)); CHECK(c.execute(Command::Status)); CHECK(p.calls == 0U);
    CHECK(c.execute(Command::Read).code() == ErrorCode::StateConflict);
    CHECK(c.execute(Command::Replace1).code() == ErrorCode::StateConflict);
    CHECK(c.execute(Command::ClearFixture).code() == ErrorCode::StateConflict);
    CHECK(c.execute(Command::RegionAttest).code() == ErrorCode::StateConflict);
    CHECK(p.calls == 0U && c.attemptedMask() == 0U && c.state() == State::Fresh);
}
void healthySequence() {
    Port p; ProbeController c(p); admit(c);
    CHECK(c.execute(Command::Read).code() == ErrorCode::NotFound); CHECK(c.state() == State::Admitted);
    CHECK(c.execute(Command::Replace1)); CHECK(c.state() == State::Written && c.attemptedMask() == 1U);
    unsigned calls = p.calls;
    CHECK(c.execute(Command::Replace1).code() == ErrorCode::StateConflict); CHECK(p.calls == calls);
    CHECK(c.execute(Command::ClearFixture).code() == ErrorCode::StateConflict); CHECK(p.calls == calls);
    CHECK(c.execute(Command::Read)); CHECK(c.execute(Command::IdentityEnd)); CHECK(p.ends == 1U);
    CHECK(c.state() == State::IdentityEnded); CHECK(c.execute(Command::Read));
    CHECK(c.execute(Command::ClearFixture)); CHECK(c.state() == State::Cleared && c.attemptedMask() == 3U);
    CHECK(c.execute(Command::Read).code() == ErrorCode::NotFound);
    calls = p.calls; CHECK(c.execute(Command::ClearFixture).code() == ErrorCode::StateConflict);
    CHECK(c.execute(Command::IdentityEnd).code() == ErrorCode::StateConflict);
    CHECK(c.execute(Command::OpenCapture).code() == ErrorCode::StateConflict);
    CHECK(p.calls == calls && p.writes == 2U);
}
void faultNeverReplays(unsigned failure) {
    Port p; ProbeController c(p); p.failCall = failure;
    Result result = c.execute(Command::OpenCapture);
    if (result) result = c.execute(Command::RegionAttest);
    if (result) result = c.execute(Command::Replace1);
    if (result) result = c.execute(Command::IdentityEnd);
    if (result) result = c.execute(Command::ClearFixture);
    CHECK(!result && c.state() == State::Failed);
    const unsigned calls = p.calls, writes = p.writes, ends = p.ends;
    CHECK(c.execute(Command::Ping)); CHECK(c.execute(Command::Status));
    CHECK(!c.execute(Command::OpenCapture)); CHECK(!c.execute(Command::RegionAttest));
    CHECK(!c.execute(Command::Read)); CHECK(!c.execute(Command::Replace1));
    CHECK(!c.execute(Command::IdentityEnd)); CHECK(!c.execute(Command::ClearFixture));
    CHECK(p.calls == calls && p.writes == writes && p.ends == ends);
    CHECK(c.attemptedMask() == (failure <= 2U ? 0U : failure <= 4U ? 1U : 3U));
}
void absenceCannotBecomeHealthyWrite() {
    Port p; ProbeController c(p); admit(c); CHECK(c.execute(Command::Replace1));
    p.present = false; CHECK(!c.execute(Command::Read)); CHECK(c.state() == State::Failed);
    const unsigned calls = p.calls; CHECK(!c.execute(Command::ClearFixture)); CHECK(p.calls == calls);
}
void failedOwnerAndMalformedInput() {
    Port p; ProbeController c(p); admit(c); p.okay = false;
    const unsigned calls = p.calls; CHECK(!c.execute(Command::Replace1));
    CHECK(c.state() == State::Failed && c.attemptedMask() == 0U && p.calls == calls);
    Port other; ProbeController malformed(other); malformed.malformedInput();
    CHECK(!malformed.execute(Command::OpenCapture)); CHECK(other.calls == 0U);
    CHECK(malformed.execute(Command::Status));
    CHECK(std::string(stateName(State::Failed)) == "Failed");
}
void reentrantCommandsDoNotTouchPort() {
    Port p; ProbeController c(p);
    p.callback = [&]() {
        const unsigned calls = p.calls, writes = p.writes;
        CHECK(!c.execute(Command::OpenCapture)); CHECK(!c.execute(Command::Read));
        CHECK(!c.execute(Command::Replace1)); CHECK(!c.execute(Command::ClearFixture));
        CHECK(c.execute(Command::Ping)); CHECK(c.execute(Command::Status));
        CHECK(p.calls == calls && p.writes == writes);
    };
    admit(c); CHECK(c.execute(Command::Replace1)); CHECK(c.execute(Command::IdentityEnd));
    CHECK(c.execute(Command::ClearFixture)); CHECK(p.calls == 5U && p.writes == 2U);
    Port malformed; ProbeController invalid(malformed);
    malformed.callback = [&]() { invalid.malformedInput(); };
    CHECK(!invalid.execute(Command::OpenCapture)); CHECK(invalid.state() == State::Failed);
    CHECK(!invalid.execute(Command::OpenCapture)); CHECK(malformed.calls == 1U);
    Port invalidWrite; ProbeController invalidMutation(invalidWrite); admit(invalidMutation);
    invalidWrite.callback = [&]() { invalidMutation.malformedInput(); };
    CHECK(!invalidMutation.execute(Command::Replace1)); CHECK(invalidMutation.state() == State::Failed);
    CHECK(invalidMutation.attemptedMask() == 1U);
    const unsigned calls = invalidWrite.calls;
    CHECK(!invalidMutation.execute(Command::Read)); CHECK(invalidWrite.calls == calls);
}

uint32_t tick = 0U;
uint32_t expireAfterNextClock = 0U;
uint32_t now() {
    const uint32_t value = tick;
    tick += expireAfterNextClock; expireAfterNextClock = 0U;
    return value;
}
class Flash final : public IWioTerminalQspiFlash {
public:
    std::vector<uint8_t> bytes;
    unsigned begins = 0U, ends = 0U, reads = 0U, erases = 0U, programs = 0U;
    unsigned failProgram = 0U, expireCloseRead = 0U;
    uint32_t pageDelay = 0U, endCost = 0U;
    Flash() : bytes(WioTerminalQspiOwner::expectedCapacity, uint8_t(0xffU)) {}
    bool begin(uint32_t& capacityBytes) override {
        ++begins; capacityBytes = static_cast<uint32_t>(bytes.size()); return true;
    }
    void end() override { ++ends; tick += endCost; }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        ++reads; std::memcpy(output, bytes.data() + address, size);
        if (reads == expireCloseRead) expireAfterNextClock = 10000U;
        return true;
    }
    bool erase(uint32_t address) override {
        ++erases; std::fill(bytes.begin() + address, bytes.begin() + address + 4096U, uint8_t(0xffU)); return true;
    }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        ++programs; tick += pageDelay;
        for (size_t i = 0U; i < size; ++i) bytes[address + i] &= input[i];
        return failProgram == 0U || programs != failProgram;
    }
};
void putBig32(uint8_t* output, uint32_t value) {
    for (size_t i = 0U; i < 4U; ++i) output[i] = static_cast<uint8_t>(value >> ((3U - i) * 8U));
}
void initializeIdentity(Flash& flash) {
    tick = expireAfterNextClock = 0U;
    WioTerminalQspiOwner owner(flash, now); WioTerminalFlashBlobBank bank(owner);
    CHECK(bank.begin()); uint8_t anchor[28U] = {};
    std::memcpy(anchor, "BDN2", 4U); anchor[4] = 1U;
    std::memset(anchor + 8U, 0x51U, 16U);
    putBig32(anchor + 24U, computeCrc32(ByteView(anchor, 24U)));
    CHECK(bank.deviceInstanceBlob().replace(ByteView(anchor, sizeof(anchor)))); bank.end();
    flash.begins = flash.ends = flash.reads = flash.erases = flash.programs = 0U;
}
std::string baseline(const Flash& flash) {
    uint8_t digest[32U] = {};
    CHECK(sha256(ByteView(flash.bytes.data() + TaskStorageProbePort::taskBase,
          TaskStorageProbePort::regionBytes * 2U), MutableByteSpan(digest, sizeof(digest))));
    static const char digits[] = "0123456789abcdef"; std::string result;
    for (size_t i = 0U; i < sizeof(digest); ++i) {
        result.push_back(digits[digest[i] >> 4U]); result.push_back(digits[digest[i] & 15U]);
    }
    return result;
}
std::vector<uint8_t> identityBytes(const Flash& flash) {
    return std::vector<uint8_t>(flash.bytes.begin() + TaskStorageProbePort::identityBase, flash.bytes.end());
}
void realPortAdmissionFailures() {
    {
        Flash flash; WioTerminalQspiOwner owner(flash, now);
        TaskStorageProbePort p(owner, now, "not-a-digest"); ProbeController c(p);
        CHECK(!c.execute(Command::OpenCapture)); CHECK(flash.begins == 0U && flash.reads == 0U);
    }
    {
        Flash flash; initializeIdentity(flash); WioTerminalQspiOwner owner(flash, now);
        const std::string wrong(64U, '0'); TaskStorageProbePort p(owner, now, wrong.c_str()); ProbeController c(p);
        CHECK(!c.execute(Command::OpenCapture)); CHECK(flash.erases == 0U && flash.programs == 0U);
        const unsigned reads = flash.reads; CHECK(!c.execute(Command::RegionAttest)); CHECK(flash.reads == reads);
    }
    {
        Flash flash; initializeIdentity(flash); flash.bytes[TaskStorageProbePort::taskBase + 8191U] = 0U;
        const std::string expected = baseline(flash); WioTerminalQspiOwner owner(flash, now);
        TaskStorageProbePort p(owner, now, expected.c_str()); ProbeController c(p);
        CHECK(c.execute(Command::OpenCapture)); CHECK(!c.execute(Command::RegionAttest));
        CHECK(flash.erases == 0U && flash.programs == 0U);
        const unsigned reads = flash.reads; CHECK(!c.execute(Command::Replace1)); CHECK(flash.reads == reads);
    }
    {
        Flash flash; const std::string expected = baseline(flash); WioTerminalQspiOwner owner(flash, now);
        TaskStorageProbePort p(owner, now, expected.c_str()); ProbeController c(p);
        CHECK(!c.execute(Command::OpenCapture)); // blank identity is never created
        CHECK(flash.erases == 0U && flash.programs == 0U);
    }
}
void realPortHealthySequence() {
    static_assert(TaskStorageProbePort::taskBase == 0x3fc000UL &&
                  TaskStorageProbePort::identityBase == 0x3fe000UL && TaskStorageProbePort::regionBytes == 8192U,
                  "Probe must preserve the shared fixed layout");
    Flash flash; initializeIdentity(flash); const std::string expected = baseline(flash);
    const std::vector<uint8_t> original = identityBytes(flash); WioTerminalQspiOwner owner(flash, now);
    TaskStorageProbePort p(owner, now, expected.c_str()); ProbeController c(p); admit(c);
    CHECK(c.execute(Command::Read).code() == ErrorCode::NotFound);
    CHECK(c.execute(Command::Replace1)); CHECK(p.snapshotAvailable() && p.identitySame());
    CHECK(flash.erases == 1U && flash.programs == 3U);
    CHECK(c.execute(Command::IdentityEnd)); CHECK(c.state() == State::IdentityEnded);
    CHECK(c.execute(Command::Read)); CHECK(p.snapshotAvailable() && p.identitySame());
    CHECK(c.execute(Command::ClearFixture)); CHECK(c.state() == State::Cleared);
    CHECK(c.execute(Command::Read).code() == ErrorCode::NotFound);
    CHECK(flash.erases == 2U && flash.programs == 4U && identityBytes(flash) == original);
    // A tombstone is not raw restoration: the task region remains nonblank.
    CHECK(!std::all_of(flash.bytes.begin() + TaskStorageProbePort::taskBase,
          flash.bytes.begin() + TaskStorageProbePort::identityBase, [](uint8_t value) { return value == 0xffU; }));
}
void realPortUnknownNeverContinues(bool late) {
    Flash flash; initializeIdentity(flash); const std::string expected = baseline(flash);
    const std::vector<uint8_t> original = identityBytes(flash); WioTerminalQspiOwner owner(flash, now);
    TaskStorageProbePort p(owner, now, expected.c_str()); ProbeController c(p); admit(c);
    if (late) flash.pageDelay = 1000U; else flash.failProgram = 1U;
    CHECK(!c.execute(Command::Replace1)); CHECK(c.state() == State::Failed && c.attemptedMask() == 1U);
    CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
    CHECK(flash.erases == 1U && flash.programs == 1U && identityBytes(flash) == original);
    const unsigned reads = flash.reads, programs = flash.programs, erases = flash.erases;
    CHECK(!c.execute(Command::Read)); CHECK(!c.execute(Command::Replace1));
    CHECK(!c.execute(Command::IdentityEnd)); CHECK(!c.execute(Command::ClearFixture));
    CHECK(!c.execute(Command::OpenCapture)); CHECK(c.execute(Command::Ping)); CHECK(c.execute(Command::Status));
    CHECK(flash.reads == reads && flash.programs == programs && flash.erases == erases);
}
void realPortCaptureCloseDeadline() {
    Flash flash; initializeIdentity(flash); const std::string expected = baseline(flash);
    flash.expireCloseRead = 128U; // Final native read of the two 16 KiB streaming passes.
    WioTerminalQspiOwner owner(flash, now); TaskStorageProbePort p(owner, now, expected.c_str()); ProbeController c(p);
    CHECK(!c.execute(Command::OpenCapture));
    CHECK(c.state() == State::Failed && owner.quarantined() && !p.identitySame());
    CHECK(flash.reads == 128U && flash.begins == 1U && flash.ends == 0U);
    CHECK(flash.erases == 0U && flash.programs == 0U);
    CHECK(!c.execute(Command::OpenCapture)); CHECK(c.execute(Command::Status)); CHECK(flash.reads == 128U);
}
void realPortNativeCloseDeadline() {
    Flash flash; initializeIdentity(flash); const std::string expected = baseline(flash); flash.endCost = 10000U;
    WioTerminalQspiOwner owner(flash, now); TaskStorageProbePort p(owner, now, expected.c_str()); ProbeController c(p);
    CHECK(!c.execute(Command::OpenCapture));
    CHECK(c.state() == State::Failed && !owner.quarantined() && !p.healthy() && !p.identitySame());
    CHECK(flash.reads == 128U && flash.begins == 1U && flash.ends == 1U);
    CHECK(!p.captureBaseline()); CHECK(!c.execute(Command::OpenCapture)); CHECK(c.execute(Command::Status));
    CHECK(flash.reads == 128U && flash.begins == 1U && flash.ends == 1U);
    CHECK(flash.erases == 0U && flash.programs == 0U);
}
}
int main() {
    noAutomaticIo(); healthySequence();
    for (unsigned failure = 1U; failure <= 5U; ++failure) faultNeverReplays(failure);
    absenceCannotBecomeHealthyWrite(); failedOwnerAndMalformedInput();
    reentrantCommandsDoNotTouchPort();
    realPortAdmissionFailures(); realPortHealthySequence();
    realPortUnknownNeverContinues(false); realPortUnknownNeverContinues(true);
    realPortCaptureCloseDeadline();
    realPortNativeCloseDeadline();
    std::cout << "Wio task probe controller: finite admission and no-replay checks passed\n";
}
