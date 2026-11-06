#include <BlinkerV2/runtime/WifiConnectionLifecycle.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            std::cerr << "Initial network lifecycle failed at " << __LINE__   \
                      << ": " << #condition << std::endl;                    \
            std::exit(1);                                                      \
        }                                                                      \
    } while (false)

class Blob : public blinker::IAtomicBlobStore {
public:
    Blob() : bytes(), size(0U), present(false), fail(false), applyOnFailure(false),
             writes(0U), clears(0U) {}
    blinker::Result load(blinker::MutableByteSpan output,
                         size_t& written) override {
        written = 0U;
        if (!present) return blinker::Result::failure(blinker::ErrorCode::NotFound);
        if (output.size < size)
            return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }
    blinker::Result replace(blinker::ByteView input) override {
        ++writes;
        CHECK(input.size <= sizeof(bytes));
        if (!fail || applyOnFailure) {
            std::memcpy(bytes, input.data, input.size);
            size = input.size;
            present = true;
        }
        return fail ? blinker::Result::failure(blinker::ErrorCode::InternalError)
                    : blinker::Result::success();
    }
    blinker::Result clear() override {
        ++clears;
        present = false;
        size = 0U;
        return blinker::Result::success();
    }
    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }
    uint8_t bytes[116];
    size_t size;
    bool present, fail, applyOnFailure;
    unsigned writes, clears;
};

class Clock : public blinker::IClock {
public:
    Clock() : now(0U) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
    uint32_t now;
};

class Station : public blinker::IWifiStation {
public:
    Station() : current(blinker::WifiStationState::Stopped),
                startError(blinker::ErrorCode::Ok), starts(0U), polls(0U) {}
    blinker::Result start(const blinker::WifiNetworkConfig&) override {
        ++starts;
        current = startError == blinker::ErrorCode::Ok
            ? blinker::WifiStationState::Connecting : blinker::WifiStationState::Failed;
        return startError == blinker::ErrorCode::Ok ? blinker::Result::success()
            : blinker::Result::failure(startError);
    }
    void poll() override { ++polls; }
    blinker::WifiStationState state() const override { return current; }
    blinker::ErrorCode lastError() const override {
        return blinker::ErrorCode::NotConnected;
    }
    void stop() override { current = blinker::WifiStationState::Stopped; }
    blinker::WifiStationState current;
    blinker::ErrorCode startError;
    unsigned starts, polls;
};

blinker::WifiNetworkConfig network(const char* ssid) {
    blinker::WifiNetworkConfig config;
    config.ssid = blinker::ByteView(reinterpret_cast<const uint8_t*>(ssid), std::strlen(ssid));
    config.authentication = blinker::WifiAuthentication::Wpa2Personal;
    config.credential = blinker::ByteView(reinterpret_cast<const uint8_t*>("password"), 8U);
    return config;
}

struct Fixture {
    Blob selector, slot0, slot1;
    blinker::WifiCredentialStore credentials;
    Station station;
    Clock clock;
    blinker::WifiCredentialProfile workspace;
    blinker::WifiConnectionLifecycle lifecycle;

    Fixture() : selector(), slot0(), slot1(), credentials(selector, slot0, slot1),
                station(), clock(), workspace(),
                lifecycle(credentials, station, clock, workspace) {
        CHECK(lifecycle.setCandidateFailurePolicy(blinker::WifiCandidateFailurePolicy::Stop));
        CHECK(credentials.begin());
    }

    void candidate() { CHECK(credentials.commit(network("test"))); }
    void noLaterWork(unsigned writes, unsigned starts, unsigned polls) {
        for (unsigned index = 0U; index < 5U; ++index) {
            clock.now += 60000U;
            lifecycle.poll();
        }
        CHECK(lifecycle.state() == blinker::WifiConnectionState::Fault);
        CHECK(selector.writes + slot0.writes + slot1.writes == writes);
        CHECK(station.starts == starts && station.polls == polls);
        CHECK(selector.clears == 0U && slot0.clears == 0U && slot1.clears == 0U);
        CHECK(!lifecycle.online());
    }
};

void testStartFailureAndTimeoutAreTerminal() {
    Fixture start;
    start.candidate();
    start.station.startError = blinker::ErrorCode::InternalError;
    CHECK(start.lifecycle.start().code() == blinker::ErrorCode::InternalError);
    CHECK(start.lifecycle.lastError() == blinker::ErrorCode::InternalError);
    start.noLaterWork(2U, 1U, 0U);

    Fixture timeout;
    timeout.candidate();
    CHECK(timeout.lifecycle.start());
    timeout.clock.now = 30000U;
    timeout.lifecycle.poll();
    CHECK(timeout.lifecycle.lastError() == blinker::ErrorCode::NotConnected);
    timeout.noLaterWork(2U, 1U, 1U);
}

void testFailureDoesNotRollBackConfirmedPriorProfile() {
    Fixture f;
    CHECK(f.credentials.commit(network("prior")));
    CHECK(f.credentials.confirmActive());
    f.candidate();
    CHECK(f.credentials.activeGeneration() == 2U);
    const unsigned writes = f.selector.writes + f.slot0.writes + f.slot1.writes;
    CHECK(f.lifecycle.start());
    f.station.current = blinker::WifiStationState::Failed;
    f.lifecycle.poll();
    CHECK(f.credentials.activeGeneration() == 2U);
    f.noLaterWork(writes, 1U, 1U);
}

void testUncertainConfirmationNeverRetries() {
    for (unsigned applied = 0U; applied < 2U; ++applied) {
        Fixture f;
        f.candidate();
        CHECK(f.lifecycle.start());
        f.slot0.fail = true;
        f.slot0.applyOnFailure = applied != 0U;
        f.station.current = blinker::WifiStationState::Connected;
        f.lifecycle.poll();
        CHECK(f.lifecycle.lastError() == blinker::ErrorCode::InternalError);
        CHECK(!f.lifecycle.confirmedProfile());
        f.slot0.fail = false; // Even a recovered port must not cause a retry.
        f.noLaterWork(3U, 1U, 1U);
        blinker::WifiCredentialProfile profile;
        CHECK(f.credentials.load(profile));
        CHECK(profile.confirmed() == (applied != 0U));
        blinker::clearWifiCredentialProfile(profile);
    }
}

void testConfirmedProfileKeepsReconnectBehavior() {
    Fixture f;
    f.candidate();
    CHECK(f.lifecycle.start());
    f.station.current = blinker::WifiStationState::Connected;
    f.lifecycle.poll();
    CHECK(f.lifecycle.online() && f.lifecycle.confirmedProfile());
    const unsigned writes = f.selector.writes + f.slot0.writes + f.slot1.writes;
    f.station.current = blinker::WifiStationState::Failed;
    f.lifecycle.poll();
    CHECK(f.lifecycle.state() == blinker::WifiConnectionState::RetryBackoff);
    f.clock.now += 5000U;
    f.lifecycle.poll();
    CHECK(f.station.starts == 2U);
    CHECK(f.lifecycle.state() == blinker::WifiConnectionState::ConnectingConfirmed);
    f.station.current = blinker::WifiStationState::Connected;
    f.lifecycle.poll();
    CHECK(f.lifecycle.online());
    CHECK(f.selector.writes + f.slot0.writes + f.slot1.writes == writes);
    CHECK(f.lifecycle.setCandidateFailurePolicy(blinker::WifiCandidateFailurePolicy::Rollback)
        .code() == blinker::ErrorCode::StateConflict);
}

} // namespace

int main() {
    testStartFailureAndTimeoutAreTerminal();
    testFailureDoesNotRollBackConfirmedPriorProfile();
    testUncertainConfirmationNeverRetries();
    testConfirmedProfileKeepsReconnectBehavior();
    std::cout << "Initial network lifecycle tests passed" << std::endl;
    return 0;
}
