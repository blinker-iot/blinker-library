#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/runtime/GatewayGattRadio.h"
#include "BlinkerV2/runtime/GatewayChildManagementRouter.h"
#include "BlinkerV2/runtime/GatewayDirectYieldLease.h"
#include "BlinkerV2/runtime/DeviceRuntime.h"
using namespace blinker;
using namespace blinker::gateway;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
namespace {
struct Port {
    BleCentralPortState s = BleCentralPortState::Stopped;
    BleCentralPortState state() const { return s; }
};
struct Child {
    Port& port;
    GatewayChildSessionState s = GatewayChildSessionState::Stopped;
    unsigned starts = 0, connects = 0, disconnects = 0, stops = 0, polls = 0;
    uint32_t attempt = 0;
    explicit Child(Port& value) : port(value) {}
    GatewayChildSessionState state() const { return s; }
    bool secure() const { return s == GatewayChildSessionState::Secure; }
    uint32_t attemptId() const { return attempt; }
    Result start() { ++starts; s = GatewayChildSessionState::Idle; port.s = BleCentralPortState::Idle; return Result::success(); }
    Result connect() { ++connects; ++attempt; s = GatewayChildSessionState::Connecting; port.s = BleCentralPortState::Connecting; return Result::success(); }
    Result disconnect() { ++disconnects; s = GatewayChildSessionState::Disconnecting; return Result::success(); }
    void stop() { CHECK(s == GatewayChildSessionState::Idle && port.s == BleCentralPortState::Idle); ++stops; s = GatewayChildSessionState::Stopped; port.s = BleCentralPortState::Stopped; }
    void poll(uint32_t) { ++polls; }
};
struct Control final : IGatewayManagementControl {
    bool owner = false, continuation = false;
    unsigned handled = 0, polls = 0;
    Result handleCommand(ByteView) override { ++handled; owner = true; return Result::success(); }
    void poll() override { ++polls; }
    bool busy() const override { return owner; }
    bool ownsChildSession() const override { return owner; }
    bool continuesCommand(ByteView) const override { return owner && continuation; }
    ByteView pendingResult() const override { return ByteView(); }
    void markResultPublished() override {}
    void reset() override { owner = false; }
};
struct Bridge : IClock {
    GatewayDirectYieldLease yield;
    uint32_t now = 1000;
    uint8_t incarnation[16] = {1};
    bool wanted = false;
    unsigned receipts = 0;
    Bridge() { CHECK(yield.begin(ByteView(incarnation, sizeof(incarnation)))); }
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& out) const override { out = 1000; return true; }
    bool directYieldHolding() { return yield.holding(now); }
    bool executionWanted() { return wanted; }
    bool poll() { return true; }
    bool pollControlPlane() { return true; }
    void reset() { yield.retire(now); wanted = false; }
    void pollDirectYield(bool quiescent) {
        GatewayDirectYieldView out;
        if (yield.receipt(now, quiescent, out)) ++receipts;
    }
    GatewayDirectYieldView grant(uint32_t id, uint32_t millis = 10000) {
        CHECK(yield.probe(id, yield.incarnation(), now));
        GatewayDirectYieldView out;
        CHECK(yield.challenge(now, out)); out.demand.operation = GatewayExecutionOperation::Decision;
        out.demand.leaseMillis = millis; CHECK(yield.apply(out, now)); return out;
    }
};
struct Fixture {
    Port port;
    Child child;
    BasicGatewayChildConnection<Child> connection;
    Bridge bridge;
    GatewayGattBootGuard boot;
    Control proof, revoke, renewal, permit;
    GatewayManagementControlMux mux;
    GatewayChildDirectory directory;
    GatewayChildManagementRouter router;
    BasicGatewayGattRadio<Child, Port, Bridge> radio;
    bool leased = false;
    explicit Fixture(bool warm = true) : child(port), connection(child), boot(bridge), mux(proof, revoke, renewal),
        directory(nullptr, 0), router(directory, permit),
        radio(child, port, connection, bridge, mux, boot, bridge) {
        router.setPermitAdmission(radio);
        if (warm) { radio.prepare(false); bridge.now += kGatewayExecutionMaximumLeaseMillis; }
    }
    void poll(bool online = true) {
        if (leased && !permit.owner) { radio.returnPermit(); leased = false; }
        radio.prepare(online); router.poll(); radio.poll(online, 1000);
    }
    void scanning() { child.s = GatewayChildSessionState::Connecting; child.attempt = 1; port.s = BleCentralPortState::Scanning; }
    void releaseNative() { child.s = GatewayChildSessionState::Idle; port.s = BleCentralPortState::Idle; }
    void openPermit() {
        uint8_t id[16] = {1}, bytes[64]; size_t size = 0;
        GatewayPermitJoinCommandView value;
        value.operationId = ByteView(id, sizeof(id)); value.adapterId = 1;
        value.action = GatewayPermitJoinAction::Open; value.expiresAtUnixSeconds = 1200;
        CHECK(encodeGatewayPermitJoinCommand(value, MutableByteSpan(bytes, sizeof(bytes)), size));
        const Result result = router.handleCommand(ByteView(bytes, size));
        CHECK(result.code() == (radio.allowNewManagementWork() ? ErrorCode::Ok : ErrorCode::WouldBlock));
        if (result) { CHECK(radio.holdForPermit()); leased = true; }
    }
    void openManagement() {
        uint8_t id[16] = {1}, bytes[200]; size_t size = 0;
        GatewayRevocationCommandView value;
        value.operationId = value.childDeviceInstanceId = value.controllerId = ByteView(id, sizeof(id));
        value.topologyVersion = value.accessEpoch = value.gatewayCredentialVersion = value.accessStorageRevision = 1;
        value.phase = GatewayRevocationPhase::Prepare; value.expiresAtUnixSeconds = 1200;
        CHECK(encodeGatewayRevocationCommand(value, MutableByteSpan(bytes, sizeof(bytes)), size));
        CHECK(mux.handleCommand(ByteView(bytes, size)));
    }
};
void ordinaryAndBarriers() {
    for (unsigned i = 0; i < 100; ++i) {
        Fixture f; f.poll(); CHECK(f.child.starts == 0); // Admission alone is not work.
        f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 1);
        f.child.s = GatewayChildSessionState::Secure;
        f.bridge.grant(1); f.poll();
        CHECK(f.child.disconnects == 1 && f.bridge.receipts == 0);
        f.openPermit(); CHECK(f.permit.handled == 0); // No Opening waiter created.
        f.child.s = GatewayChildSessionState::Fault; f.poll();
        CHECK(f.bridge.receipts == 0 && f.child.connects == 1);
        f.child.s = GatewayChildSessionState::Idle; f.port.s = BleCentralPortState::Connecting;
        f.poll(); CHECK(f.bridge.receipts == 0); // Logical Idle is not native Idle.
        f.releaseNative(); f.poll(); CHECK(f.bridge.receipts == 1 && f.child.stops == 1);
        f.openPermit(); CHECK(f.permit.handled == 0);
        CHECK(!f.radio.holdForPermit());
        // Even an internal caller bypassing the mux cannot take the port once
        // the promise has been made. It is not another authorized Open path.
        f.permit.owner = true; CHECK(!f.radio.holdForPermit()); f.permit.owner = false;
        f.poll(); CHECK(f.child.connects == 1);
        f.bridge.now += 10000; f.poll(); CHECK(f.child.connects == 2);
    }
}
void coldBootPromiseBarrier() {
    Fixture f(false); f.bridge.now = UINT32_MAX - 500U; f.radio.prepare(false);
    f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 0);
    f.openPermit(); CHECK(f.permit.handled == 0);
    f.permit.owner = true; CHECK(!f.radio.holdForPermit()); f.permit.owner = false;
    f.bridge.now += kGatewayExecutionMaximumLeaseMillis - 1U; f.poll(); CHECK(f.child.connects == 0);
    ++f.bridge.now; f.poll(); CHECK(f.child.connects == 1);
    f.bridge.wanted = false; f.poll(); f.releaseNative(); f.poll();
    f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 2); // No per-connection 30 s delay.
    f.bridge.wanted = false; f.poll(); f.releaseNative(); f.poll();
    f.bridge.now += 6000; f.poll(); // Full natural opportunity refunds occupancy.
    f.bridge.now = UINT32_MAX - 500U; f.radio.prepare(true);
    CHECK(f.radio.allowNewManagementWork()); // A later full clock wrap cannot re-arm cold start.
}
void cancelledWakeAndOffline() {
    Fixture f; f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 1);
    f.bridge.wanted = false; f.poll(); CHECK(f.child.disconnects == 1);
    f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 1); // New demand cannot bypass closing.
    f.child.s = GatewayChildSessionState::Idle; f.port.s = BleCentralPortState::Disconnecting;
    f.poll(); CHECK(f.child.connects == 1 && !f.radio.allowNewManagementWork());
    f.releaseNative(); f.poll(); f.poll(); CHECK(f.child.connects == 2);
    f.poll(false); CHECK(f.child.disconnects == 2 && !f.radio.allowNewManagementWork());
    f.openPermit(); CHECK(f.permit.handled == 0);
    f.releaseNative(); f.poll(false); CHECK(f.child.stops == 1);
    f.poll(); CHECK(f.child.connects == 2); // MQTT recovery still needs fresh execution.
    f.bridge.wanted = true; f.poll(); f.radio.stop(); CHECK(f.child.s == GatewayChildSessionState::Disconnecting);
    f.releaseNative(); f.poll(false); CHECK(f.child.stops == 2);
}
void managementDrainsAndLiveGate() {
    Fixture f; f.radio.prepare(true);
    // Simulate a Decision arriving inside northbound poll, after prepare.
    f.bridge.grant(1); f.openPermit(); CHECK(f.permit.handled == 0);

    Fixture old; old.radio.prepare(true); old.openPermit(); CHECK(old.permit.owner);
    old.bridge.grant(1); CHECK(!old.radio.stopped()); // Already borrowed, not a new window.
    old.poll(); CHECK(old.bridge.receipts == 0); // Ready reply is still ownership.
    old.poll(false); CHECK(old.permit.polls != 0); // Budgets progress without management subscription.
    old.permit.owner = false; old.poll(); CHECK(!old.radio.allowNewManagementWork()); // Retired promise remains.

    Fixture management; management.bridge.wanted = true; management.poll();
    management.child.s = GatewayChildSessionState::Secure;
    management.openManagement(); CHECK(management.mux.ownsChildSession());
    management.bridge.grant(1); management.poll();
    CHECK(management.bridge.receipts == 0 && management.child.disconnects == 0);
    CHECK(management.connection.ensureConnected()); // Existing transaction can finish.
    management.revoke.owner = false; management.poll();
    CHECK(management.child.disconnects == 1 && management.bridge.receipts == 0);
    management.releaseNative(); management.poll(); CHECK(management.bridge.receipts == 1);
}
void promiseSurvivesReconnect() {
    Fixture f; auto grant = f.bridge.grant(1); f.poll(); CHECK(f.bridge.receipts == 1);
    f.bridge.now += 1000; f.poll(false);
    CHECK(f.bridge.yield.begin(ByteView(f.bridge.incarnation, 16))); f.bridge.wanted = true;
    for (unsigned i = 0; i < 8; ++i) { f.bridge.now += 1000; f.poll(); CHECK(f.child.connects == 0); }
    f.bridge.now += 1000; f.poll(); CHECK(f.child.connects == 1);

    Fixture zero; grant = zero.bridge.grant(1); zero.poll();
    grant.demand.leaseMillis = 0; CHECK(zero.bridge.yield.apply(grant, zero.bridge.now));
    zero.bridge.wanted = true; zero.poll(); CHECK(zero.child.connects == 1); // Current exact zero cancels early.
}
void continuousDemandAndNativeOpportunity() {
    Fixture f; f.bridge.wanted = true;
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
        f.poll(); const unsigned connected = f.child.connects;
        CHECK(connected == cycle + 1);
        // Connecting counts. Keep "renewing" wanted; no Secure prerequisite.
        for (unsigned n = 0; n < 9; ++n) { f.bridge.now += 1000; f.poll(); CHECK(f.child.connects == connected); }
        f.bridge.now += 999; f.poll(); CHECK(f.radio.allowNewManagementWork());
        ++f.bridge.now;
        CHECK(!f.radio.allowNewManagementWork()); // Live, before next prepare.
        f.openPermit(); CHECK(f.permit.handled == 0);
        f.poll(); CHECK(f.child.s == GatewayChildSessionState::Disconnecting);
        f.child.s = GatewayChildSessionState::Idle; f.port.s = BleCentralPortState::Disconnecting;
        f.bridge.now += 20000; f.poll(); CHECK(f.child.connects == connected);
        f.releaseNative(); f.poll(); CHECK(f.child.stops == cycle + 1);
        f.bridge.now += 5999; f.poll(); CHECK(f.child.connects == connected);
        ++f.bridge.now; // The next iteration may acquire only now.
    }
}
void shortGapsOfflineAndWrap() {
    Fixture f; f.bridge.now = UINT32_MAX - 4000U; f.bridge.wanted = true; f.poll();
    f.bridge.now += 3000; f.bridge.wanted = false; f.poll(); f.releaseNative(); f.poll();
    f.bridge.now += 2000; f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 2);
    f.bridge.now += 5000; f.poll(); CHECK(f.child.s == GatewayChildSessionState::Disconnecting);
    f.radio.stop(); f.poll(false); f.bridge.now += 50000; f.poll(false);
    CHECK(!f.radio.allowNewManagementWork());
    f.child.s = GatewayChildSessionState::Fault; f.poll(false);
    CHECK(f.child.connects == 2); // Neither wall time nor Fault proves release.
    f.releaseNative(); f.poll(false);
    f.bridge.now += 5999; f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 2);
    ++f.bridge.now; f.poll(); CHECK(f.child.connects == 3);
}
void originalManagementDrainsButNewWorkCannotExtend() {
    Fixture f; f.bridge.wanted = true; f.poll(); f.child.s = GatewayChildSessionState::Secure;
    f.bridge.now += 9999; f.openManagement(); f.revoke.continuation = true;
    ++f.bridge.now; CHECK(!f.radio.allowNewManagementWork());
    f.openManagement(); CHECK(f.revoke.handled == 2); // Original coordinator authorizes exact continuation.
    f.openPermit(); CHECK(f.permit.handled == 0);
    f.poll(); CHECK(f.child.disconnects == 0 && f.connection.ensureConnected());
    f.bridge.now += 20000; f.poll(); CHECK(f.child.disconnects == 0); // Original bounded coordinator owns the budget.
    f.revoke.owner = false; f.poll(); CHECK(f.child.disconnects == 1);
    f.releaseNative(); f.poll();
    f.bridge.now += 5999; f.poll(); CHECK(f.child.connects == 1);
    ++f.bridge.now; f.poll(); CHECK(f.child.connects == 2);

    Fixture permit; permit.radio.prepare(true); permit.openPermit(); permit.poll();
    permit.bridge.now += 10000; CHECK(!permit.radio.allowNewManagementWork());
    CHECK(!permit.radio.stopped()); // Existing Open keeps its original borrow.
    permit.bridge.now += 20000; permit.poll(); permit.permit.owner = false; permit.poll();
    permit.bridge.now += 5999; CHECK(!permit.radio.allowNewManagementWork());
    ++permit.bridge.now; CHECK(permit.radio.allowNewManagementWork());
}
void naturalIdleAndCloudPromiseAreNotAddedTwice() {
    Fixture f; f.bridge.wanted = true; f.poll(); f.bridge.now += 1000;
    f.bridge.wanted = false; f.poll(); f.releaseNative(); f.poll();
    f.bridge.now += 6000; f.poll(); f.bridge.wanted = true; f.poll(); CHECK(f.child.connects == 2);
    f.bridge.now += 9999; f.poll(); CHECK(f.child.disconnects == 1); // New full budget after natural idle.
    ++f.bridge.now; f.poll(); f.releaseNative(); f.poll();
    auto grant = f.bridge.grant(1, 1000); f.poll();
    grant.demand.leaseMillis = 0; CHECK(f.bridge.yield.apply(grant, f.bridge.now));
    f.bridge.now += 5999; f.poll(); CHECK(f.child.connects == 2); // Zero cannot erase local opportunity.
    ++f.bridge.now; f.poll(); CHECK(f.child.connects == 3);
    f.bridge.now += 10000; f.poll(); f.releaseNative(); f.poll();
    f.bridge.grant(2, 10000); f.poll();
    f.bridge.now += 6000; f.poll(); CHECK(f.child.connects == 3); // Local window cannot shorten cloud promise.
    f.bridge.now += 4000; f.poll(); CHECK(f.child.connects == 4); // No extra 6 s after promised idle.
}
void boundedSearchAndLateAcquisition() {
    Fixture f; f.bridge.now = UINT32_MAX - 8000U;
    f.bridge.wanted = true; f.scanning(); f.poll();
    f.bridge.now += 15999; f.poll(); CHECK(f.child.disconnects == 0);
    // The boundary is checked before a late Ready, even across millis wrap.
    ++f.bridge.now; f.port.s = BleCentralPortState::Ready;
    f.child.s = GatewayChildSessionState::Secure; f.poll();
    CHECK(f.child.disconnects == 1 && !f.radio.allowNewManagementWork());
    f.releaseNative(); f.poll(); f.bridge.now += 5999; f.poll(); CHECK(f.child.connects == 0);
    ++f.bridge.now; f.poll(); CHECK(f.child.connects == 1);

    Fixture scan; scan.bridge.wanted = true;
    scan.scanning();
    scan.poll(); scan.bridge.now += 16000; scan.poll();
    CHECK(scan.child.disconnects == 1); // No advertising peer is bounded too.
}
void searchDoesNotConsumeConnectedWindow() {
    Fixture f; f.bridge.wanted = true;
    f.scanning(); f.poll();
    f.bridge.now += 15000; f.port.s = BleCentralPortState::Connecting; f.poll();
    f.bridge.now += 1000; f.port.s = BleCentralPortState::Discovering; f.poll();
    f.bridge.now += 1000; f.port.s = BleCentralPortState::Ready; f.child.s = GatewayChildSessionState::Secure; f.poll();
    f.bridge.now += 7999; f.poll(); CHECK(f.child.disconnects == 0);
    ++f.bridge.now; f.poll(); CHECK(f.child.disconnects == 1); // 10 s since native initiation, not Secure.
}
void attemptsAndShortGapsCannotRefundStages() {
    Fixture f; f.bridge.wanted = true;
    f.scanning(); f.poll();
    f.bridge.now += 7000; f.port.s = BleCentralPortState::Connecting; f.radio.prepare(true);
    // A one-poll native failure is observed before its completion; retrying
    // the same logical work may scan again, but cannot reset occupancy.
    f.port.s = BleCentralPortState::Scanning; ++f.child.attempt; f.poll();
    f.bridge.now += 2000; f.bridge.wanted = false; f.poll(); f.releaseNative(); f.poll();
    f.bridge.now += 1000; f.poll(false); f.bridge.wanted = true; f.poll();
    f.bridge.now += 6999; f.poll(); CHECK(f.child.disconnects == 1);
    ++f.bridge.now; f.poll(); CHECK(f.child.disconnects == 2);

    Fixture rejected; rejected.bridge.wanted = true;
    rejected.scanning(); rejected.poll();
    // A synchronous native rejection never acquires an ACL. Even if only
    // Scanning is observable, repeated failures cannot extend the search cap.
    for (unsigned n = 0; n < 15; ++n) { ++rejected.child.attempt; rejected.bridge.now += 1000; rejected.poll(); }
    rejected.bridge.now += 1000; rejected.poll(); CHECK(rejected.child.disconnects == 1);
}
}
int main() {
    CHECK((DeviceRuntimeConfig().protocolFeatures & bbp2::FeatureGatewayDirectYield) == 0U);
    coldBootPromiseBarrier(); ordinaryAndBarriers(); cancelledWakeAndOffline(); managementDrainsAndLiveGate(); promiseSurvivesReconnect();
    continuousDemandAndNativeOpportunity(); shortGapsOfflineAndWrap();
    originalManagementDrainsButNewWorkCannotExtend(); naturalIdleAndCloudPromiseAreNotAddedTwice();
    boundedSearchAndLateAcquisition(); searchDoesNotConsumeConnectedWindow(); attemptsAndShortGapsCannotRefundStages();
    std::cout << "Gateway GATT policy: 100 yield + 100 local fairness simulated cycles, native/management/clock barriers passed\n";
}
