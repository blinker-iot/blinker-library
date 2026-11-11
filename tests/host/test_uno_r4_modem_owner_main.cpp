#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemClient.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemBleLink.h>

#include <cstdlib>
#include <iostream>

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
uint32_t clockValue = 0U;
uint32_t timeoutValue = 0U;
unsigned timeoutCalls = 0U;
uint32_t fakeNow() { return clockValue; }
void fakeTimeout(uint32_t value) { timeoutValue = value; ++timeoutCalls; }
void resetClock(uint32_t value = 0U) {
    clockValue = value;
    timeoutValue = 0U;
    timeoutCalls = 0U;
}

void testExclusiveNamespaceAndOperation() {
    resetClock(50U);
    RenesasUnoModemOwner owner(fakeNow, fakeTimeout, 10000U);
    int first = 0, second = 0;
    CHECK(owner.networkAllowed() && !owner.quarantined());
    CHECK(!owner.claimNamespace(nullptr));
    CHECK(owner.claimNamespace(&first) && owner.claimNamespace(&first));
    CHECK(!owner.claimNamespace(&second));
    owner.releaseNamespace(&second);
    {
        RenesasUnoModemOwner::Operation outer(owner, &first, 1000U);
        CHECK(outer.result() && !owner.networkAllowed());
        CHECK(!owner.claimNamespace(&second));
        CHECK(owner.prepare() && timeoutValue == 999U);
        clockValue = 250U;
        CHECK(owner.prepare() && timeoutValue == 799U);
        {
            RenesasUnoModemOwner::Operation nested(owner, &first, 1000U);
            CHECK(!nested.result() && nested.result().code() == ErrorCode::WouldBlock);
            RenesasUnoModemOwner::Operation foreign(owner, &second, 1000U);
            CHECK(!foreign.result() && foreign.result().code() == ErrorCode::WouldBlock);
        }
        CHECK(timeoutValue == 799U && timeoutCalls == 2U);
        CHECK(owner.complete(true));
    }
    CHECK(owner.networkAllowed() && timeoutValue == 10000U && timeoutCalls == 3U);
    CHECK(!owner.complete(true) && !owner.quarantined());
    CHECK(!owner.complete(false) && !owner.quarantined());
    CHECK(!owner.prepare());
    CHECK(!owner.claimNamespace(&second));
    owner.releaseNamespace(&first);
    CHECK(owner.claimNamespace(&second));
    owner.releaseNamespace(&second);
}

void testInvalidOperationArguments() {
    resetClock();
    RenesasUnoModemOwner owner(fakeNow, fakeTimeout, 10000U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    RenesasUnoModemOwner::Operation nullToken(owner, nullptr, 1U);
    RenesasUnoModemOwner::Operation zero(owner, &token, 0U);
    RenesasUnoModemOwner::Operation excessive(owner, &token, 0x80000000U);
    CHECK(!nullToken.result() && nullToken.result().code() == ErrorCode::InvalidArgument);
    CHECK(!zero.result() && zero.result().code() == ErrorCode::InvalidArgument);
    CHECK(!excessive.result() && excessive.result().code() == ErrorCode::InvalidArgument);
    CHECK(owner.networkAllowed() && timeoutCalls == 0U);
}

void testRemainingDeadlineAndWrap() {
    resetClock(UINT32_MAX - 3U);
    RenesasUnoModemOwner owner(fakeNow, fakeTimeout, 10000U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation operation(owner, &token, 10U);
        CHECK(operation.result() && owner.prepare() && timeoutValue == 9U);
        clockValue = 2U; // Six milliseconds elapsed across rollover.
        CHECK(owner.complete(true) && owner.prepare() && timeoutValue == 3U);
        clockValue = 5U;
        CHECK(owner.prepare() && timeoutValue == 0U); // One millisecond remains.
        clockValue = 6U;
        CHECK(!owner.prepare());
        CHECK(owner.outcome() == RenesasUnoModemOwner::DeadlineBeforeWrite);
        CHECK(!owner.quarantined()); // No incomplete response is in flight.
    }
    CHECK(owner.networkAllowed() && timeoutValue == 10000U);
    {
        RenesasUnoModemOwner::Operation next(owner, &token, 1U);
        CHECK(next.result() && owner.prepare() && timeoutValue == 0U);
        CHECK(owner.complete(true));
    }
    CHECK(owner.networkAllowed() && timeoutValue == 10000U);
}

void testQuarantine(bool writing, bool validResponse, uint32_t elapsed) {
    resetClock(10U);
    RenesasUnoModemOwner owner(fakeNow, fakeTimeout, 10000U);
    int token = 0, other = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation operation(owner, &token, 100U);
        CHECK(operation.result() && owner.prepare());
        if (writing) owner.writing();
        clockValue = 10U + elapsed;
        CHECK(!owner.complete(validResponse));
        CHECK(owner.quarantined() && !owner.networkAllowed());
        CHECK(owner.outcome() == (writing ? RenesasUnoModemOwner::WriteUncertain
                                        : RenesasUnoModemOwner::ReadUncertain));
        CHECK(!owner.prepare());
    }
    CHECK(timeoutValue == 10000U && !owner.networkAllowed());
    owner.releaseNamespace(&token);
    CHECK(!owner.claimNamespace(&token) && !owner.claimNamespace(&other));
    RenesasUnoModemOwner::Operation retry(owner, &token, 100U);
    CHECK(!retry.result() && retry.result().code() == ErrorCode::StateConflict);
    CHECK(owner.quarantined()); // Destruction, release, and baseline cannot unlock.
    owner.baseline();
    CHECK(owner.quarantined() && !owner.networkAllowed());
}

void testPutHeaderDoesNotRenewBudget() {
    resetClock();
    RenesasUnoModemOwner owner(fakeNow, fakeTimeout, 10000U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation operation(owner, &token, 100U);
        CHECK(operation.result() && owner.prepare() && timeoutValue == 99U);
        owner.writing(); // Header has promised a complete binary payload.
        clockValue = 25U;
        owner.prepareWriteResponse();
        CHECK(timeoutValue == 74U);
        clockValue = 100U; // Finishing the promised payload is not cancellation.
        owner.prepareWriteResponse();
        CHECK(timeoutValue == 0U);
        CHECK(!owner.complete(true)); // Even a late OK cannot confirm the commit.
        CHECK(owner.outcome() == RenesasUnoModemOwner::WriteUncertain);
        CHECK(!owner.prepare() && owner.quarantined());
    }
    CHECK(timeoutValue == 10000U && !owner.networkAllowed());
}

// Models the native virtual dispatch paths that would self-lock if every
// override acquired an independent non-reentrant operation lease.
class NativeClient {
public:
    NativeClient() : calls(0U) {}
    virtual ~NativeClient() {}
    virtual int connect(IPAddress, uint16_t port) {
        ++calls;
        return connect("test-host", port);
    }
    virtual int connect(const char*, uint16_t) { ++calls; return connected(); }
    virtual size_t write(uint8_t byte) { ++calls; return write(&byte, 1U); }
    virtual size_t write(const uint8_t*, size_t size) { ++calls; return size; }
    virtual int available() { ++calls; return 1; }
    virtual int read() {
        ++calls;
        uint8_t byte = 0U;
        return read(&byte, 1U) == 1 ? byte : -1;
    }
    virtual int read(uint8_t* data, size_t size) {
        ++calls;
        if (size == 0U) return 0;
        data[0] = 42U;
        return 1;
    }
    virtual int peek() { ++calls; return 42; }
    virtual void flush() { ++calls; }
    virtual void stop() { ++calls; }
    virtual uint8_t connected() { ++calls; return available() > 0 ? 1U : 0U; }
    virtual operator bool() { ++calls; return true; }
    virtual IPAddress remoteIP() { ++calls; return IPAddress(1U); }
    virtual uint16_t remotePort() { ++calls; return 1883U; }
    unsigned calls;
};

typedef RenesasUnoModemClient<NativeClient> GuardedClient;

struct FakeBleConfig {
    uint32_t marker;
    explicit FakeBleConfig(uint32_t value) : marker(value) {}
};

class NativeBleLink : public IBleModeLink {
public:
    explicit NativeBleLink(const FakeBleConfig& config)
        : calls(0U), marker(config.marker), pollBudget(0U), sentSession(0U),
          sentSize(0U), disconnectedSession(0U), profile_() {}
    Result start() override { ++calls; return Result::success(); }
    void stop() override { ++calls; }
    void poll(uint32_t budget) override { ++calls; pollBudget = budget; }
    BleLinkState state() const override { ++calls; return BleLinkState::Ready; }
    ErrorCode lastError() const override { ++calls; return ErrorCode::Ok; }
    size_t sessionCount() const override { ++calls; return 1U; }
    Result sessionAt(size_t index, BleSessionInfo& session) const override {
        ++calls;
        if (index != 0U) return Result::failure(ErrorCode::NotFound);
        session = BleSessionInfo();
        session.sessionId = 41U;
        session.maxPacketSize = 20U;
        session.connected = true;
        session.notifyEnabled = true;
        return Result::success();
    }
    Result sendPacket(uint32_t session, ByteView packet) override {
        ++calls; sentSession = session; sentSize = packet.size;
        return Result::success();
    }
    Result disconnectSession(uint32_t session) override {
        ++calls; disconnectedSession = session;
        return Result::success();
    }
    Result configureBleProfile(const ble::ModeProfile& profile) override {
        ++calls; profile_ = profile; return Result::success();
    }
    // Native callback registration and profile getters only touch RAM; these
    // do not issue HCI/modem RPCs and do not need the shared modem lease.
    void setPacketReceiver(BlePacketReceiver, void*) override {}
    void setSessionHandlers(BleSessionHandler, BleSessionHandler, void*) override {}
    ble::ModeProfile bleProfile() const override { return profile_; }
    mutable unsigned calls;
    uint32_t marker, pollBudget, sentSession;
    size_t sentSize;
    uint32_t disconnectedSession;
private:
    ble::ModeProfile profile_;
};

typedef RenesasUnoModemBleLink<NativeBleLink> GuardedBleLink;
static_assert(sizeof(GuardedBleLink) == sizeof(NativeBleLink),
              "The R4 BLE modem gate must not allocate fields or a workspace");

void assertBleApisForwarded(GuardedBleLink& link) {
    link.calls = 0U;
    IBleModeLink& api = link; // Exercise the production interface dispatch.
    ble::ModeProfile profile;
    profile.mode = ble::ApplicationMode::Direct;
    profile.wireVersion = ble::kDirectBbp2WireVersion;
    CHECK(api.start() && api.configureBleProfile(profile));
    api.poll(123U);
    CHECK(link.pollBudget == 123U);
    CHECK(api.state() == BleLinkState::Ready && api.lastError() == ErrorCode::Ok);
    CHECK(api.sessionCount() == 1U);
    BleSessionInfo session;
    CHECK(api.sessionAt(0U, session) && session.sessionId == 41U && session.connected);
    const uint8_t packet = 7U;
    CHECK(api.sendPacket(41U, ByteView(&packet, 1U)));
    CHECK(link.sentSession == 41U && link.sentSize == 1U);
    CHECK(api.disconnectSession(41U) && link.disconnectedSession == 41U);
    api.stop();
    CHECK(api.bleProfile().mode == profile.mode && link.calls == 10U);
}

void assertBleApisBlocked(GuardedBleLink& link) {
    link.calls = 0U;
    IBleModeLink& api = link;
    CHECK(!api.start() && api.start().code() == ErrorCode::StateConflict);
    api.stop(); api.poll(123U);
    const uint8_t packet = 7U;
    CHECK(!api.sendPacket(41U, ByteView(&packet, 1U)));
    CHECK(!api.disconnectSession(41U));
    CHECK(!api.configureBleProfile(ble::ModeProfile()));
    CHECK(api.state() == BleLinkState::Error && api.lastError() == ErrorCode::StateConflict);
    CHECK(api.sessionCount() == 0U);
    BleSessionInfo session;
    session.sessionId = 41U;
    session.maxPacketSize = 20U;
    session.connected = true;
    session.notifyEnabled = true;
    session.encrypted = true;
    session.bonded = true;
    session.authenticated = true;
    CHECK(!api.sessionAt(0U, session));
    CHECK(session.sessionId == 0U && session.maxPacketSize == 0U &&
          !session.connected && !session.notifyEnabled && !session.encrypted &&
          !session.bonded && !session.authenticated);
    CHECK(link.calls == 0U);
}

void assertEveryApiBlocked(GuardedClient& client) {
    client.calls = 0U;
    uint8_t data = 7U;
    CHECK(client.connect(IPAddress(1U), 1883U) == 0);
    CHECK(client.connect("host", 1883U) == 0);
    CHECK(client.write(data) == 0U && client.write(&data, 1U) == 0U);
    CHECK(client.available() == 0);
    CHECK(client.read() == -1 && client.read(&data, 1U) == -1 && data == 7U);
    CHECK(client.peek() == -1);
    client.flush(); client.stop();
    CHECK(client.connected() == 0U && !static_cast<bool>(client));
    CHECK(client.remoteIP().value() == 0U && client.remotePort() == 0U);
    CHECK(client.calls == 0U);
}

void testClientAdmissionAndNativeReentry() {
    resetClock();
    RenesasUnoModemOwner& owner = renesasUnoModemOwner();
    GuardedClient first, second;
    GuardedBleLink bleLink(FakeBleConfig(73U));
    CHECK(owner.networkAllowed());
    CHECK(bleLink.marker == 73U);
    assertBleApisForwarded(bleLink);
    CHECK(first.connect(IPAddress(1U), 1883U) == 1 && first.calls == 3U);
    CHECK(first.write(static_cast<uint8_t>(1U)) == 1U && first.calls == 5U);
    CHECK(first.read() == 42 && first.calls == 7U);
    CHECK(first.connected() == 1U && first.calls == 9U);
    int token = 0;
    CHECK(owner.claimNamespace(&token));
    {
        RenesasUnoModemOwner::Operation operation(owner, &token, 1000U);
        CHECK(operation.result());
        assertEveryApiBlocked(first);
        assertEveryApiBlocked(second);
        assertBleApisBlocked(bleLink);
        CHECK(owner.prepare() && owner.complete(true));
    }
    CHECK(owner.networkAllowed());
    assertBleApisForwarded(bleLink);
    CHECK(first.connect("host", 1883U) == 1 && first.calls == 3U);
    {
        RenesasUnoModemOwner::Operation operation(owner, &token, 1000U);
        CHECK(operation.result() && owner.prepare());
        owner.writing();
        CHECK(!owner.complete(false));
    }
    assertEveryApiBlocked(first);
    assertEveryApiBlocked(second);
    assertBleApisBlocked(bleLink);
    owner.releaseNamespace(&token);
    CHECK(!owner.claimNamespace(&token) && owner.quarantined());
}
} // namespace

namespace blinker {
RenesasUnoModemOwner& renesasUnoModemOwner() {
    static RenesasUnoModemOwner owner(fakeNow, fakeTimeout, 10000U);
    return owner;
}
} // namespace blinker

int main() {
    testExclusiveNamespaceAndOperation();
    testInvalidOperationArguments();
    testRemainingDeadlineAndWrap();
    testQuarantine(false, false, 0U);
    testQuarantine(true, false, 0U);
    testQuarantine(false, true, 100U);
    testQuarantine(true, true, 101U);
    testPutHeaderDoesNotRenewBudget();
    testClientAdmissionAndNativeReentry();
    std::cout << "R4 modem owner, Client and BLE admission tests passed\n";
    return 0;
}
