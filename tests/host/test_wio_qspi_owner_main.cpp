#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspiOwner.ipp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace blinker;

// These model tests deliberately do not use assert: Release/NDEBUG must run
// the same checks. They exercise the portable owner, not native QSPI timing.
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
uint32_t clockValue = 0U;
uint32_t fakeNow() { return clockValue; }

class FakeFlash : public IWioTerminalQspiFlash {
public:
    enum Call { Begin, End, Read, Erase, Program };
    typedef void (*Hook)(Call, void*);

    FakeFlash()
        : bytes(WioTerminalQspiOwner::expectedCapacity, 0xffU),
          reportedCapacity(WioTerminalQspiOwner::expectedCapacity),
          beginConfirmed(true), readConfirmed(true), eraseConfirmed(true),
          programConfirmed(true), beginCost(0U), readCost(0U), eraseCost(0U),
          programCost(0U), beginCalls(0U), endCalls(0U), readCalls(0U),
          eraseCalls(0U), programCalls(0U), hook(nullptr), hookContext(nullptr) {}

    bool begin(uint32_t& capacity) override {
        ++beginCalls;
        invoke(Begin);
        clockValue += beginCost;
        capacity = reportedCapacity;
        return beginConfirmed;
    }
    void end() override { ++endCalls; invoke(End); }
    bool read(uint32_t address, uint8_t* output, size_t size) override {
        ++readCalls;
        CHECK(address <= bytes.size() && size <= bytes.size() - address);
        invoke(Read);
        clockValue += readCost;
        if (readConfirmed) std::copy(bytes.data() + address, bytes.data() + address + size, output);
        return readConfirmed;
    }
    bool erase(uint32_t address) override {
        ++eraseCalls;
        CHECK(address <= bytes.size() && WioTerminalQspiOwner::eraseBlockSize <= bytes.size() - address);
        invoke(Erase);
        clockValue += eraseCost;
        // A failed/late return does not imply that media was left unchanged.
        std::fill(bytes.data() + address,
                  bytes.data() + address + WioTerminalQspiOwner::eraseBlockSize,
                  static_cast<uint8_t>(0xffU));
        return eraseConfirmed;
    }
    bool program(uint32_t address, const uint8_t* input, size_t size) override {
        ++programCalls;
        CHECK(address <= bytes.size() && size <= bytes.size() - address);
        invoke(Program);
        clockValue += programCost;
        for (size_t i = 0U; i < size; ++i) bytes[address + i] &= input[i];
        return programConfirmed;
    }
    unsigned calls() const {
        return beginCalls + endCalls + readCalls + eraseCalls + programCalls;
    }

    std::vector<uint8_t> bytes; // Host media model only; no production buffer.
    uint32_t reportedCapacity;
    bool beginConfirmed, readConfirmed, eraseConfirmed, programConfirmed;
    uint32_t beginCost, readCost, eraseCost, programCost;
    unsigned beginCalls, endCalls, readCalls, eraseCalls, programCalls;
    Hook hook;
    void* hookContext;
private:
    void invoke(Call call) { if (hook != nullptr) hook(call, hookContext); }
};

void testTwoRegionLeasesAndLastEnd() {
    static_assert(WioTerminalQspiOwner::taskAddress == 0x3fc000UL &&
                  WioTerminalQspiOwner::identityAddress == 0x3fe000UL &&
                  WioTerminalQspiOwner::reservedAddress == 0x3fc000UL &&
                  WioTerminalQspiOwner::reservedSize == 16384UL,
                  "Product and maintenance must share the fixed tail layout");
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int identity = 0, task = 0, duplicate = 0;
    CHECK(owner.capacity() == 0U && !owner.quarantined());
    CHECK(owner.regionAddress(WioTerminalQspiOwner::Region::Identity) == 0U);
    CHECK(!owner.acquire(nullptr, WioTerminalQspiOwner::Region::Identity));
    CHECK(!owner.acquire(&identity, static_cast<WioTerminalQspiOwner::Region>(2U)));
    CHECK(flash.calls() == 0U);
    CHECK(owner.acquire(&identity, WioTerminalQspiOwner::Region::Identity));
    CHECK(owner.acquire(&identity, WioTerminalQspiOwner::Region::Identity));
    CHECK(flash.beginCalls == 1U && owner.capacity() == WioTerminalQspiOwner::expectedCapacity);
    CHECK(owner.regionAddress(WioTerminalQspiOwner::Region::Identity) == owner.capacity() - 8192U);
    CHECK(owner.regionAddress(WioTerminalQspiOwner::Region::Tasks) == owner.capacity() - 16384U);
    CHECK(owner.regionAddress(WioTerminalQspiOwner::Region::Identity) == WioTerminalQspiOwner::identityAddress);
    CHECK(owner.regionAddress(WioTerminalQspiOwner::Region::Tasks) == WioTerminalQspiOwner::taskAddress);
    CHECK(owner.regionAddress(static_cast<WioTerminalQspiOwner::Region>(2U)) == 0U);
    CHECK(!owner.acquire(&duplicate, WioTerminalQspiOwner::Region::Identity));
    CHECK(!owner.acquire(&identity, WioTerminalQspiOwner::Region::Tasks));
    CHECK(owner.acquire(&task, WioTerminalQspiOwner::Region::Tasks));
    CHECK(flash.beginCalls == 1U);
    CHECK(!owner.release(&duplicate) && flash.endCalls == 0U);
    CHECK(owner.release(&identity) && flash.endCalls == 0U);
    CHECK(owner.capacity() == WioTerminalQspiOwner::expectedCapacity);
    CHECK(owner.release(&task) && flash.endCalls == 1U && owner.capacity() == 0U);
    CHECK(owner.acquire(&duplicate, WioTerminalQspiOwner::Region::Identity));
    CHECK(flash.beginCalls == 2U);
    CHECK(owner.release(&duplicate) && flash.endCalls == 2U);
}

void testInvalidCallsNeverReachNative() {
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0, foreign = 0;
    uint8_t bytes[4] = { 1U, 2U, 3U, 4U };
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
    const unsigned calls = flash.calls();
    CHECK(!owner.read(&token, base, bytes, sizeof(bytes))); // No active operation.
    CHECK(!owner.verificationFailed(&token) && !owner.quarantined());
    {
        WioTerminalQspiOwner::Operation nullToken(owner, nullptr, 10U);
        WioTerminalQspiOwner::Operation zero(owner, &token, 0U);
        WioTerminalQspiOwner::Operation excessive(owner, &token, 0x80000000U);
        WioTerminalQspiOwner::Operation unknown(owner, &foreign, 10U);
        CHECK(!nullToken.result() && nullToken.result().code() == ErrorCode::InvalidArgument);
        CHECK(!zero.result() && zero.result().code() == ErrorCode::InvalidArgument);
        CHECK(!excessive.result() && excessive.result().code() == ErrorCode::InvalidArgument);
        CHECK(!unknown.result() && unknown.result().code() == ErrorCode::NotConfigured);
    }
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 10U);
        CHECK(operation.result());
        CHECK(!owner.read(nullptr, base, bytes, 1U));
        CHECK(!owner.read(&foreign, base, bytes, 1U));
        CHECK(!owner.read(&token, base, nullptr, 1U));
        CHECK(!owner.read(&token, base, bytes, 0U));
        CHECK(!owner.read(&token, base - 1U, bytes, 1U));
        CHECK(!owner.read(&token, base + WioTerminalQspiOwner::regionSize, bytes, 1U));
        CHECK(!owner.read(&token, UINT32_MAX, bytes, sizeof(bytes)));
        CHECK(!owner.read(&token, base, bytes, static_cast<size_t>(-1)));
        CHECK(!owner.program(&token, base, nullptr, 1U));
        CHECK(!owner.program(&token, base, bytes, 0U));
        CHECK(!owner.program(&token, base + WioTerminalQspiOwner::regionSize - 2U, bytes, sizeof(bytes)));
        CHECK(!owner.program(&token, UINT32_MAX, bytes, sizeof(bytes)));
        CHECK(!owner.erase(&token, base + 1U));
        CHECK(!owner.erase(&token, base - WioTerminalQspiOwner::eraseBlockSize));
        CHECK(!owner.erase(&token, base + WioTerminalQspiOwner::regionSize));
        CHECK(!owner.verificationFailed(&foreign));
        CHECK(flash.calls() == calls && !owner.quarantined());
        CHECK(owner.read(&token, base + WioTerminalQspiOwner::regionSize - sizeof(bytes), bytes, sizeof(bytes)));
        CHECK(flash.readCalls == 1U && owner.outcome() == WioTerminalQspiOwner::None);
    }
    CHECK(owner.release(&token));
}

struct Reentry {
    WioTerminalQspiOwner& owner;
    int& identity;
    int& task;
    unsigned visits;
};

void onNativeYield(FakeFlash::Call call, void* context) {
    Reentry& state = *static_cast<Reentry*>(context);
    ++state.visits;
    if (call == FakeFlash::Begin || call == FakeFlash::End) {
        const Result acquire = state.owner.acquire(&state.task, WioTerminalQspiOwner::Region::Tasks);
        CHECK(!acquire && acquire.code() == ErrorCode::WouldBlock);
        return;
    }
    CHECK(call == FakeFlash::Read);
    const Result release = state.owner.release(&state.identity);
    CHECK(!release && release.code() == ErrorCode::WouldBlock);
    {
        WioTerminalQspiOwner::Operation same(state.owner, &state.identity, 10U);
        WioTerminalQspiOwner::Operation other(state.owner, &state.task, 10U);
        CHECK(!same.result() && same.result().code() == ErrorCode::WouldBlock);
        CHECK(!other.result() && other.result().code() == ErrorCode::WouldBlock);
    } // Failed nested destructors must not release the outer operation.
}

void testNativeYieldCannotEndOrUnlockActiveToken() {
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int identity = 0, task = 0;
    Reentry reentry = { owner, identity, task, 0U };
    flash.hook = onNativeYield;
    flash.hookContext = &reentry;
    CHECK(owner.acquire(&identity, WioTerminalQspiOwner::Region::Identity));
    flash.hook = nullptr;
    CHECK(owner.acquire(&task, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Identity);
    uint8_t output = 0U;
    {
        WioTerminalQspiOwner::Operation outer(owner, &identity, 10U);
        CHECK(outer.result());
        flash.hook = onNativeYield;
        CHECK(owner.read(&identity, base, &output, 1U));
        CHECK(owner.read(&identity, base, &output, 1U));
        CHECK(output == 0xffU && flash.endCalls == 0U);
        flash.hook = nullptr;
        CHECK(owner.release(&task) && flash.endCalls == 0U);
        CHECK(!owner.acquire(&task, WioTerminalQspiOwner::Region::Tasks));
    }
    flash.hook = onNativeYield;
    CHECK(owner.release(&identity));
    CHECK(flash.endCalls == 1U && owner.capacity() == 0U && reentry.visits == 4U);
    flash.hook = nullptr;
    CHECK(owner.acquire(&task, WioTerminalQspiOwner::Region::Tasks));
    CHECK(owner.release(&task) && flash.endCalls == 2U);
}

void testWholeOperationBudgetDoesNotRenew() {
    clockValue = 10U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0;
    const uint8_t value = 0x5aU;
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 20U);
        CHECK(operation.result());
        flash.eraseCost = 5U;
        flash.programCost = 10U;
        CHECK(owner.erase(&token, base));
        CHECK(owner.program(&token, base, &value, 1U));
        clockValue += 5U;
        CHECK(!owner.program(&token, base + 1U, &value, 1U));
        CHECK(flash.eraseCalls == 1U && flash.programCalls == 1U);
        CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
    }
    CHECK(owner.release(&token) && flash.endCalls == 0U);
    CHECK(!owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
}

void testDeadlineBeforeMutationMakesNoNativeWrite() {
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0;
    uint8_t output = 0U;
    const uint8_t value = 0x5aU;
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 10U);
        CHECK(operation.result());
        flash.readCost = 1U;
        CHECK(owner.read(&token, base, &output, 1U));
        clockValue += 9U;
        CHECK(!owner.erase(&token, base));
        CHECK(owner.outcome() == WioTerminalQspiOwner::DeadlineBeforeWrite && owner.quarantined());
        CHECK(!owner.program(&token, base, &value, 1U));
        CHECK(flash.eraseCalls == 0U && flash.programCalls == 0U && flash.readCalls == 1U);
    }
}

void testUncertainMutationIsSticky(bool erase, bool confirmed, uint32_t elapsed) {
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0, other = 0;
    uint8_t output = 0U;
    const uint8_t value = 0x5aU;
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 20U);
        CHECK(operation.result());
        flash.eraseConfirmed = confirmed;
        flash.programConfirmed = confirmed;
        flash.eraseCost = elapsed;
        flash.programCost = elapsed;
        const Result write = erase ? owner.erase(&token, base)
                                   : owner.program(&token, base, &value, 1U);
        CHECK(!write && write.code() == ErrorCode::WouldBlock);
        CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
        const unsigned calls = flash.calls();
        CHECK(!owner.read(&token, base, &output, 1U));
        CHECK(!owner.program(&token, base + 1U, &value, 1U));
        CHECK(!owner.erase(&token, base));
        CHECK(!owner.verificationFailed(&token));
        CHECK(flash.calls() == calls); // No follow-up readback or retry.
    }
    CHECK(owner.release(&token) && flash.endCalls == 0U);
    CHECK(!owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    CHECK(!owner.acquire(&other, WioTerminalQspiOwner::Region::Identity));
    WioTerminalQspiOwner::Operation retry(owner, &token, 20U);
    CHECK(!retry.result() && retry.result().code() == ErrorCode::StateConflict);
    CHECK(owner.quarantined() && flash.beginCalls == 1U);
}

void testUncertainRead(bool confirmed, uint32_t elapsed) {
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0;
    uint8_t output = 0x7bU;
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Identity));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Identity);
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 20U);
        CHECK(operation.result());
        flash.readConfirmed = confirmed;
        flash.readCost = elapsed;
        CHECK(!owner.read(&token, base, &output, 1U));
        CHECK(owner.outcome() == WioTerminalQspiOwner::ReadUncertain && owner.quarantined());
        if (!confirmed) CHECK(output == 0x7bU);
        CHECK(!owner.erase(&token, base) && flash.eraseCalls == 0U);
    }
    CHECK(owner.release(&token) && flash.endCalls == 0U);
}

void testVerificationFailureAfterConfirmedProgram() {
    clockValue = 0U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0;
    const uint8_t value = 0x5aU;
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 20U);
        CHECK(operation.result() && owner.program(&token, base, &value, 1U));
        CHECK(!owner.verificationFailed(&token));
        CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::WriteUncertain);
        CHECK(flash.programCalls == 1U && flash.readCalls == 0U);
    }
}

void testClockWrapUsesElapsedBudget() {
    clockValue = UINT32_MAX - 5U;
    FakeFlash flash;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0;
    uint8_t output = 0U;
    const uint8_t value = 0x5aU;
    CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    const uint32_t base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
    {
        WioTerminalQspiOwner::Operation operation(owner, &token, 10U);
        CHECK(operation.result());
        flash.readCost = 3U;
        flash.programCost = 3U;
        CHECK(owner.read(&token, base, &output, 1U));
        CHECK(owner.program(&token, base, &value, 1U));
        CHECK(clockValue == 0U);
        clockValue = 4U;
        CHECK(!owner.program(&token, base + 1U, &value, 1U));
        CHECK(owner.outcome() == WioTerminalQspiOwner::WriteUncertain && flash.programCalls == 1U);
    }
}

void testLifecycleFailureDoesNotReleaseQuarantine(bool confirmed, uint32_t capacity, uint32_t elapsed) {
    clockValue = 0U;
    FakeFlash flash;
    flash.beginConfirmed = confirmed;
    flash.reportedCapacity = capacity;
    flash.beginCost = elapsed;
    WioTerminalQspiOwner owner(flash, fakeNow);
    int token = 0;
    CHECK(!owner.acquire(&token, WioTerminalQspiOwner::Region::Identity));
    CHECK(owner.quarantined() && owner.outcome() == WioTerminalQspiOwner::ReadUncertain);
    CHECK(owner.capacity() == 0U && !owner.release(&token));
    CHECK(!owner.acquire(&token, WioTerminalQspiOwner::Region::Identity));
    CHECK(flash.beginCalls == 1U && flash.endCalls == 0U);
}

void testFreshOwnerReadsUncertainMediaWithoutRewriting() {
    clockValue = 0U;
    FakeFlash flash;
    int token = 0;
    const uint8_t value[4] = { 0x5aU, 0x42U, 0x52U, 0x4aU };
    uint32_t base = 0U;
    {
        WioTerminalQspiOwner owner(flash, fakeNow);
        CHECK(owner.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
        base = owner.regionAddress(WioTerminalQspiOwner::Region::Tasks);
        WioTerminalQspiOwner::Operation operation(owner, &token, 20U);
        CHECK(operation.result());
        flash.programConfirmed = false;
        CHECK(!owner.program(&token, base, value, sizeof(value)));
        CHECK(owner.quarantined() && flash.programCalls == 1U && flash.readCalls == 0U);
    }
    // A new object represents a new owner lifecycle, not end/begin unlocking
    // the old latch. This is only a media model, never a power-cycle proof.
    clockValue = 0U;
    WioTerminalQspiOwner fresh(flash, fakeNow);
    CHECK(fresh.acquire(&token, WioTerminalQspiOwner::Region::Tasks));
    uint8_t actual[sizeof(value)] = {};
    {
        WioTerminalQspiOwner::Operation operation(fresh, &token, 20U);
        CHECK(operation.result() && fresh.read(&token, base, actual, sizeof(actual)));
        CHECK(std::equal(actual, actual + sizeof(actual), value));
        CHECK(!fresh.quarantined() && fresh.outcome() == WioTerminalQspiOwner::None);
    }
    CHECK(flash.beginCalls == 2U && flash.programCalls == 1U && flash.readCalls == 1U);
    CHECK(fresh.release(&token) && flash.endCalls == 1U);
}
} // namespace

int main() {
    testTwoRegionLeasesAndLastEnd();
    testInvalidCallsNeverReachNative();
    testNativeYieldCannotEndOrUnlockActiveToken();
    testWholeOperationBudgetDoesNotRenew();
    testDeadlineBeforeMutationMakesNoNativeWrite();
    testUncertainMutationIsSticky(false, false, 0U);
    testUncertainMutationIsSticky(true, false, 0U);
    testUncertainMutationIsSticky(false, true, 20U);
    testUncertainMutationIsSticky(true, true, 21U);
    testUncertainRead(false, 0U);
    testUncertainRead(true, 20U);
    testVerificationFailureAfterConfirmedProgram();
    testClockWrapUsesElapsedBudget();
    testLifecycleFailureDoesNotReleaseQuarantine(false, WioTerminalQspiOwner::expectedCapacity, 0U);
    testLifecycleFailureDoesNotReleaseQuarantine(true, WioTerminalQspiOwner::expectedCapacity - 1U, 0U);
    testLifecycleFailureDoesNotReleaseQuarantine(true, WioTerminalQspiOwner::expectedCapacity,
                                               WioTerminalQspiOwner::identityBudgetMillis);
    testFreshOwnerReadsUncertainMediaWithoutRewriting();
    std::cout << "Wio QSPI owner lease, range, budget and quarantine model tests passed\n";
    return 0;
}
