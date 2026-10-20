#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>

#include <cstdlib>
#include <iostream>

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
uint32_t clockValue = 0U, timeoutValue = 0U;
uint32_t now() { return clockValue; }
void timeout(uint32_t value) { timeoutValue = value; }

void standaloneRpc() {
    clockValue = 100U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 100U), foreign(owner, 100U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    CHECK(rpc.begin(100U) && rpc.active() && !owner.networkAllowed());
    CHECK(!foreign.begin(100U));
    {
        RenesasUnoModemOwner::Operation storage(owner, &token, 1000U);
        CHECK(!storage.result() && storage.result().code() == ErrorCode::WouldBlock);
    }
    clockValue = 140U; // Header TX completed; passthrough must not begin again.
    CHECK(!rpc.expired());
    clockValue = 199U;
    CHECK(!rpc.expired());
    clockValue = 200U;
    CHECK(rpc.expired());
    rpc.fault(); // Model calls this only after confirming safe UART stop.
    CHECK(!rpc.active() && owner.quarantined());
    CHECK(owner.outcome() == RenesasUnoModemOwner::WriteUncertain);
    CHECK(!owner.networkAllowed() && !rpc.begin(100U) && !foreign.begin(100U));
    rpc.finish(); owner.baseline(); owner.releaseNamespace(&token);
    CHECK(!owner.networkAllowed()); // No end/baseline/unlock resurrection.
}

void storageSharesRemainingBudget(bool writing) {
    clockValue = 100U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 1000U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation storage(owner, &token, 100U);
        CHECK(storage.result());
        clockValue = 125U;
        CHECK(rpc.begin(1000U) && !rpc.expired());
        clockValue = 150U;
        CHECK(rpc.finish()); // First metadata RPC completed; operation still owns modem.
        CHECK(!owner.networkAllowed() && rpc.begin(1000U));
        if (writing) owner.writing();
        clockValue = 175U;
        owner.prepareWriteResponse();
        CHECK(timeoutValue == 24U && !rpc.expired());
        clockValue = 200U;
        CHECK(rpc.expired()); // Not 250/1150: neither new RPC nor payload renews.
        rpc.fault();
        CHECK(!owner.complete(true));
        CHECK(owner.outcome() == (writing ? RenesasUnoModemOwner::WriteUncertain
                                        : RenesasUnoModemOwner::ReadUncertain));
    }
    CHECK(owner.quarantined() && !owner.networkAllowed());
}

void completedRpcAndShorterRpcBudget() {
    clockValue = 0U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 10U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation storage(owner, &token, 100U);
        CHECK(storage.result() && rpc.begin(1000U));
        clockValue = 9U;
        CHECK(!rpc.expired() && rpc.finish());
        CHECK(owner.complete(true) && rpc.begin(1000U));
        clockValue = 19U;
        CHECK(rpc.expired());
        rpc.fault();
    }
    CHECK(owner.quarantined());
}

void exactExpiryBeforeTxDoesNotSendOrQuarantine() {
    clockValue = 0U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 100U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation storage(owner, &token, 10U);
        CHECK(storage.result());
        clockValue = 10U;
        CHECK(!rpc.begin(100U) && !rpc.active() && !owner.quarantined());
        CHECK(owner.outcome() == RenesasUnoModemOwner::DeadlineBeforeWrite);
    }
    CHECK(owner.networkAllowed() && rpc.begin(100U) && rpc.finish());
    CHECK(owner.networkAllowed() && !owner.quarantined());
}

void rolloverAndInvalidBudget() {
    clockValue = UINT32_MAX - 4U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 10U), zero(owner, 0U), excessive(owner, 0x80000000U);
    CHECK(!zero.begin(10U) && !excessive.begin(10U) && !rpc.begin(0U));
    CHECK(!rpc.begin(0x80000000U) && owner.networkAllowed());
    CHECK(rpc.begin(10U));
    clockValue = 4U;
    CHECK(!rpc.expired());
    clockValue = 5U;
    CHECK(rpc.expired()); rpc.fault();
    CHECK(owner.quarantined());
}

void releasingUnfinishedOperationCannotUnlock() {
    clockValue = 0U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 100U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation storage(owner, &token, 100U);
        CHECK(storage.result() && rpc.begin(100U));
        owner.writing();
    }
    CHECK(owner.quarantined() && rpc.expired());
    CHECK(owner.outcome() == RenesasUnoModemOwner::WriteUncertain);
    rpc.finish();
    CHECK(!owner.networkAllowed() && !rpc.begin(100U));
}

void requestedBudgetAndFinalByteExpiry() {
    clockValue = 0U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 1000U);
    CHECK(rpc.begin(10U));
    clockValue = 9U;
    CHECK(!rpc.expired());
    clockValue = 10U; // The parser might already have read the final byte.
    CHECK(!rpc.finish() && rpc.faulted() && !rpc.active());
    CHECK(!owner.networkAllowed() && !rpc.begin(1000U));
}

void interruptedHeaderIsNotFreshRpc() {
    clockValue = 0U;
    RenesasUnoModemOwner owner(now, timeout, 10000U);
    RenesasUnoModemRpcBoundary rpc(owner, 100U);
    CHECK(rpc.begin(100U));
    clockValue = 1U;
    CHECK(!rpc.begin(100U));
    CHECK(rpc.faulted() && !rpc.active() && !owner.networkAllowed());
    CHECK(owner.outcome() == RenesasUnoModemOwner::WriteUncertain);
}
} // namespace

int main() {
    standaloneRpc();
    storageSharesRemainingBudget(false);
    storageSharesRemainingBudget(true);
    completedRpcAndShorterRpcBudget();
    exactExpiryBeforeTxDoesNotSendOrQuarantine();
    rolloverAndInvalidBudget();
    releasingUnfinishedOperationCannotUnlock();
    requestedBudgetAndFinalByteExpiry();
    interruptedHeaderIsNotFreshRpc();
    std::cout << "R4 whole RPC boundary finite model passed; UART stop requires overlay proof\n";
}
