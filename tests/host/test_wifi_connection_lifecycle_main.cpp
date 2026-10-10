#include <BlinkerV2/interface/IAtomicBlobStore.h>
#include <BlinkerV2/runtime/WifiConnectionLifecycle.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "WiFi lifecycle check failed at line " << __LINE__  \
                      << ": " << #condition << std::endl;                    \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class MemoryAtomicStore : public blinker::IAtomicBlobStore {
public:
    MemoryAtomicStore()
        : data(),
          size(0U),
          present(false),
          replaceError(blinker::ErrorCode::Ok),
          applyOnFailure(false),
          replaceCalls(0U) {}

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < size) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, data, size);
        written = size;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        ++replaceCalls;
        const bool fail = replaceError != blinker::ErrorCode::Ok;
        if (!fail || applyOnFailure) {
            CHECK_TRUE(value.data != nullptr && value.size <= sizeof(data));
            std::memset(data, 0, sizeof(data));
            std::memcpy(data, value.data, value.size);
            size = value.size;
            present = true;
        }
        if (fail) {
            const blinker::ErrorCode error = replaceError;
            replaceError = blinker::ErrorCode::Ok;
            applyOnFailure = false;
            return blinker::Result::failure(error);
        }
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        std::memset(data, 0, sizeof(data));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

    uint8_t data[128];
    size_t size;
    bool present;
    blinker::ErrorCode replaceError;
    bool applyOnFailure;
    uint32_t replaceCalls;
};

class FakeClock : public blinker::IClock {
public:
    FakeClock() : now(0U) {}

    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }

    void advance(uint32_t milliseconds) { now += milliseconds; }

    uint32_t now;
};

class FakeWifiStation : public blinker::IWifiStation {
public:
    FakeWifiStation()
        : stateValue(blinker::WifiStationState::Stopped),
          errorValue(blinker::ErrorCode::Ok),
          nextStartError(blinker::ErrorCode::Ok),
          startCalls(0U),
          stopCalls(0U),
          pollCalls(0U),
          ssid(),
          ssidLength(0U) {}

    blinker::Result start(
        const blinker::WifiNetworkConfig& config) override {
        ++startCalls;
        CHECK_EQ(blinker::WifiStationState::Stopped, stateValue);
        CHECK_TRUE(config.ssid.size <= sizeof(ssid));
        std::memset(ssid, 0, sizeof(ssid));
        std::memcpy(ssid, config.ssid.data, config.ssid.size);
        ssidLength = config.ssid.size;
        if (nextStartError != blinker::ErrorCode::Ok) {
            const blinker::ErrorCode error = nextStartError;
            nextStartError = blinker::ErrorCode::Ok;
            stateValue = blinker::WifiStationState::Failed;
            errorValue = error;
            return blinker::Result::failure(error);
        }
        stateValue = blinker::WifiStationState::Connecting;
        errorValue = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }

    void poll() override { ++pollCalls; }
    blinker::WifiStationState state() const override { return stateValue; }
    blinker::ErrorCode lastError() const override { return errorValue; }

    void stop() override {
        ++stopCalls;
        stateValue = blinker::WifiStationState::Stopped;
        errorValue = blinker::ErrorCode::Ok;
    }

    void connect() { stateValue = blinker::WifiStationState::Connected; }
    void fail(blinker::ErrorCode error) {
        stateValue = blinker::WifiStationState::Failed;
        errorValue = error;
    }
    bool currentSsid(const char* expected) const {
        const size_t length = std::strlen(expected);
        return length == ssidLength &&
               std::memcmp(ssid, expected, length) == 0;
    }

    blinker::WifiStationState stateValue;
    blinker::ErrorCode errorValue;
    blinker::ErrorCode nextStartError;
    uint32_t startCalls;
    uint32_t stopCalls;
    uint32_t pollCalls;
    uint8_t ssid[32];
    size_t ssidLength;
};

blinker::WifiNetworkConfig network(const char* ssid, const char* password) {
    blinker::WifiNetworkConfig config;
    config.ssid = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(ssid),
        std::strlen(ssid));
    config.authentication = blinker::WifiAuthentication::Wpa2Personal;
    config.credential = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(password),
        std::strlen(password));
    return config;
}

struct Fixture {
    MemoryAtomicStore selector;
    MemoryAtomicStore slot0;
    MemoryAtomicStore slot1;
    blinker::WifiCredentialStore credentials;
    FakeWifiStation station;
    FakeClock clock;
    blinker::WifiCredentialProfile workspace;
    blinker::WifiConnectionLifecycleConfig config;
    blinker::WifiConnectionLifecycle lifecycle;

    explicit Fixture(blinker::WifiCandidateFailurePolicy policy =
                         blinker::WifiCandidateFailurePolicy::Rollback)
        : selector(),
          slot0(),
          slot1(),
          credentials(selector, slot0, slot1),
          station(),
          clock(),
          workspace(),
          config(makeConfig()),
          lifecycle(credentials, station, clock, workspace, config) {
        CHECK_TRUE(lifecycle.setCandidateFailurePolicy(policy));
    }

    static blinker::WifiConnectionLifecycleConfig makeConfig() {
        blinker::WifiConnectionLifecycleConfig value;
        value.connectionTimeoutMs = 100U;
        value.reconnectBackoffMs = 20U;
        value.persistenceRetryMs = 10U;
        return value;
    }
};

void makeFirstProfileOnline(Fixture& fixture) {
    CHECK_TRUE(fixture.credentials.commit(network("home", "home-pass")));
    fixture.lifecycle.poll();
    CHECK_EQ(
        blinker::WifiConnectionState::ConnectingCandidate,
        fixture.lifecycle.state());
    CHECK_TRUE(fixture.station.currentSsid("home"));
    fixture.station.connect();
    fixture.lifecycle.poll();
    CHECK_EQ(blinker::WifiConnectionState::Online, fixture.lifecycle.state());
    CHECK_TRUE(fixture.lifecycle.online());
    blinker::WifiCredentialProfile loaded;
    CHECK_TRUE(fixture.credentials.load(loaded));
    CHECK_TRUE(loaded.confirmed());
    blinker::clearWifiCredentialProfile(loaded);
}

void testWaitCandidateConfirmAndReconnect() {
    Fixture fixture;
    CHECK_TRUE(fixture.lifecycle.start());
    CHECK_EQ(
        blinker::WifiConnectionState::WaitingForCredentials,
        fixture.lifecycle.state());
    makeFirstProfileOnline(fixture);

    const uint32_t generation = fixture.credentials.activeGeneration();
    fixture.station.fail(blinker::ErrorCode::NotConnected);
    fixture.lifecycle.poll();
    CHECK_EQ(
        blinker::WifiConnectionState::RetryBackoff,
        fixture.lifecycle.state());
    CHECK_EQ(generation, fixture.credentials.activeGeneration());
    fixture.clock.advance(20U);
    fixture.lifecycle.poll();
    CHECK_EQ(
        blinker::WifiConnectionState::ConnectingConfirmed,
        fixture.lifecycle.state());
    fixture.station.connect();
    fixture.lifecycle.poll();
    CHECK_TRUE(fixture.lifecycle.online());
}

void testCandidateFailureRollsBackOnlyToConfirmed() {
    Fixture fixture;
    CHECK_TRUE(fixture.lifecycle.start());
    makeFirstProfileOnline(fixture);

    CHECK_TRUE(fixture.credentials.commit(network("bad", "bad-pass")));
    fixture.lifecycle.poll();
    CHECK_EQ(2U, fixture.lifecycle.profileGeneration());
    CHECK_TRUE(fixture.station.currentSsid("bad"));

    fixture.station.fail(blinker::ErrorCode::AuthenticationRequired);
    fixture.lifecycle.poll();
    CHECK_EQ(1U, fixture.credentials.activeGeneration());
    CHECK_EQ(
        blinker::WifiConnectionState::ConnectingConfirmed,
        fixture.lifecycle.state());
    CHECK_TRUE(fixture.station.currentSsid("home"));
    fixture.station.connect();
    fixture.lifecycle.poll();
    CHECK_TRUE(fixture.lifecycle.online());

    fixture.station.fail(blinker::ErrorCode::NotConnected);
    fixture.lifecycle.poll();
    CHECK_EQ(
        blinker::WifiConnectionState::RetryBackoff,
        fixture.lifecycle.state());
    CHECK_EQ(1U, fixture.credentials.activeGeneration());
}

void testFirstCandidateFailureNeedsProvisioning() {
    Fixture fixture;
    CHECK_TRUE(fixture.lifecycle.start());
    CHECK_TRUE(fixture.credentials.commit(network("bad", "bad-pass")));
    fixture.lifecycle.poll();
    fixture.clock.advance(100U);
    fixture.lifecycle.poll();
    CHECK_EQ(
        blinker::WifiConnectionState::CandidateRejected,
        fixture.lifecycle.state());
    CHECK_TRUE(!fixture.lifecycle.online());
    CHECK_EQ(1U, fixture.credentials.activeGeneration());
    const uint32_t starts = fixture.station.startCalls;
    fixture.lifecycle.poll();
    CHECK_EQ(starts, fixture.station.startCalls);
    CHECK_EQ(
        blinker::WifiConnectionState::CandidateRejected,
        fixture.lifecycle.state());
}

void testConfirmationFailureGatesNetworkUntilDurable() {
    Fixture fixture;
    CHECK_TRUE(fixture.lifecycle.start());
    CHECK_TRUE(fixture.credentials.commit(network("home", "home-pass")));
    fixture.lifecycle.poll();
    fixture.slot0.replaceError = blinker::ErrorCode::InternalError;
    fixture.station.connect();
    fixture.lifecycle.poll();
    CHECK_EQ(
        blinker::WifiConnectionState::ConfirmingCandidate,
        fixture.lifecycle.state());
    CHECK_TRUE(!fixture.lifecycle.online());

    fixture.clock.advance(10U);
    fixture.lifecycle.poll();
    CHECK_TRUE(fixture.lifecycle.online());
    CHECK_EQ(blinker::ErrorCode::Ok, fixture.lifecycle.lastError());
}

uint32_t writes(const Fixture& fixture) {
    return fixture.selector.replaceCalls + fixture.slot0.replaceCalls +
           fixture.slot1.replaceCalls;
}

void testManualCandidateRetriesWithoutRollbackOrWrites(
    bool hasPrevious, bool startFailure) {
    Fixture fixture(blinker::WifiCandidateFailurePolicy::Retry);
    CHECK_TRUE(fixture.lifecycle.start());
    if (hasPrevious) makeFirstProfileOnline(fixture);
    // Retry deadlines also have to work across the uint32_t millis wrap.
    fixture.clock.now = 0xfffffff0U;
    CHECK_TRUE(fixture.credentials.commit(network("configured", "configured-pass")));
    const uint32_t generation = fixture.credentials.activeGeneration();
    const uint32_t beforeWrites = writes(fixture);
    if (startFailure) fixture.station.nextStartError = blinker::ErrorCode::NotConnected;
    fixture.lifecycle.poll();
    for (unsigned attempt = 0U; attempt < 3U; ++attempt) {
        if (fixture.lifecycle.state() == blinker::WifiConnectionState::ConnectingCandidate) {
            fixture.clock.advance(100U);
            fixture.lifecycle.poll();
        }
        CHECK_EQ(blinker::WifiConnectionState::RetryBackoff, fixture.lifecycle.state());
        CHECK_EQ(blinker::ErrorCode::NotConnected, fixture.lifecycle.lastError());
        CHECK_EQ(generation, fixture.credentials.activeGeneration());
        CHECK_TRUE(!fixture.lifecycle.online());
        CHECK_TRUE(!fixture.lifecycle.confirmedProfile());
        CHECK_EQ(beforeWrites, writes(fixture));
        CHECK_EQ(blinker::ErrorCode::StateConflict,
            fixture.lifecycle.setCandidateFailurePolicy(blinker::WifiCandidateFailurePolicy::Rollback).code());
        const uint32_t starts = fixture.station.startCalls;
        fixture.clock.advance(19U); fixture.lifecycle.poll();
        CHECK_EQ(starts, fixture.station.startCalls);
        fixture.clock.advance(1U); fixture.lifecycle.poll();
        CHECK_EQ(starts + 1U, fixture.station.startCalls);
        CHECK_EQ(blinker::WifiConnectionState::ConnectingCandidate, fixture.lifecycle.state());
        CHECK_TRUE(fixture.station.currentSsid("configured"));
        CHECK_EQ(beforeWrites, writes(fixture));
    }
    // Retry is not a shortcut around durable confirmation or a fake confirmed flag.
    MemoryAtomicStore& activeSlot = hasPrevious ? fixture.slot1 : fixture.slot0;
    activeSlot.replaceError = blinker::ErrorCode::InternalError;
    fixture.station.connect(); fixture.lifecycle.poll();
    CHECK_EQ(blinker::WifiConnectionState::ConfirmingCandidate, fixture.lifecycle.state());
    CHECK_TRUE(!fixture.lifecycle.online());
    fixture.clock.advance(10U); fixture.lifecycle.poll();
    CHECK_TRUE(fixture.lifecycle.online());
    CHECK_TRUE(fixture.lifecycle.confirmedProfile());
    CHECK_EQ(generation, fixture.credentials.activeGeneration());
}

void testManualNewGenerationAndPolicyAdmission() {
    Fixture fixture(blinker::WifiCandidateFailurePolicy::Retry);
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
        fixture.lifecycle.setCandidateFailurePolicy(static_cast<blinker::WifiCandidateFailurePolicy>(9U)).code());
    CHECK_TRUE(fixture.lifecycle.start());
    CHECK_TRUE(fixture.credentials.commit(network("first", "first-pass")));
    fixture.lifecycle.poll(); fixture.clock.advance(100U); fixture.lifecycle.poll();
    CHECK_EQ(blinker::WifiConnectionState::RetryBackoff, fixture.lifecycle.state());
    CHECK_TRUE(fixture.credentials.commit(network("replacement", "replacement-pass")));
    fixture.lifecycle.poll();
    CHECK_TRUE(fixture.station.currentSsid("replacement"));
    CHECK_EQ(blinker::WifiConnectionState::ConnectingCandidate, fixture.lifecycle.state());
    fixture.lifecycle.stop();
    CHECK_TRUE(!fixture.lifecycle.confirmedProfile());
    CHECK_TRUE(fixture.lifecycle.setCandidateFailurePolicy(blinker::WifiCandidateFailurePolicy::Rollback));
    CHECK_TRUE(fixture.lifecycle.start()); fixture.clock.advance(100U); fixture.lifecycle.poll();
    // Neither unconfirmed generation is a valid rollback destination.
    CHECK_EQ(blinker::WifiConnectionState::CandidateRejected, fixture.lifecycle.state());
}

void testInitialStationFailureStillAdmitsScheduledRetry(bool confirmed) {
    Fixture fixture(confirmed ? blinker::WifiCandidateFailurePolicy::Rollback
                              : blinker::WifiCandidateFailurePolicy::Retry);
    CHECK_TRUE(fixture.credentials.begin());
    CHECK_TRUE(fixture.credentials.commit(network("configured", "configured-pass")));
    if (confirmed) CHECK_TRUE(fixture.credentials.confirmActive());
    const uint32_t beforeWrites = writes(fixture);
    fixture.station.nextStartError = blinker::ErrorCode::NotConnected;
    CHECK_TRUE(fixture.lifecycle.start());
    CHECK_EQ(blinker::WifiConnectionState::RetryBackoff, fixture.lifecycle.state());
    CHECK_EQ(blinker::ErrorCode::NotConnected, fixture.lifecycle.lastError());
    CHECK_TRUE(!fixture.lifecycle.online());
    CHECK_EQ(beforeWrites, writes(fixture));
    fixture.clock.advance(20U); fixture.lifecycle.poll();
    CHECK_TRUE(fixture.station.currentSsid("configured"));
    fixture.station.connect(); fixture.lifecycle.poll();
    CHECK_TRUE(fixture.lifecycle.online());
}

void testInitialTransactionalFailureIsNotAdmittedAsRetry() {
    Fixture fixture;
    CHECK_TRUE(fixture.credentials.begin());
    CHECK_TRUE(fixture.credentials.commit(network("candidate", "candidate-pass")));
    fixture.station.nextStartError = blinker::ErrorCode::NotConnected;
    CHECK_TRUE(!fixture.lifecycle.start());
    CHECK_EQ(blinker::WifiConnectionState::CandidateRejected, fixture.lifecycle.state());
}

} // namespace

int main() {
    testWaitCandidateConfirmAndReconnect();
    testCandidateFailureRollsBackOnlyToConfirmed();
    testFirstCandidateFailureNeedsProvisioning();
    testConfirmationFailureGatesNetworkUntilDurable();
    testManualCandidateRetriesWithoutRollbackOrWrites(false, false);
    testManualCandidateRetriesWithoutRollbackOrWrites(true, false);
    testManualCandidateRetriesWithoutRollbackOrWrites(false, true);
    testManualCandidateRetriesWithoutRollbackOrWrites(true, true);
    testManualNewGenerationAndPolicyAdmission();
    testInitialStationFailureStillAdmitsScheduledRetry(false);
    testInitialStationFailureStillAdmitsScheduledRetry(true);
    testInitialTransactionalFailureIsNotAdmittedAsRetry();
    std::cout << "WiFi lifecycle tests passed; lifecycle="
              << sizeof(blinker::WifiConnectionLifecycle)
              << " B; caller profile="
              << sizeof(blinker::WifiCredentialProfile) << " B"
              << std::endl;
    return 0;
}
