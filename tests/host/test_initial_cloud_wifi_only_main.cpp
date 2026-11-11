#include <cstdlib>
#include <cstring>
#include <iostream>
#include <BlinkerV2/provisioning/InitialCloudWifiConfigWire.h>
#include <BlinkerV2/provisioning/InitialCloudWifiOnly.h>

using namespace blinker;

namespace {
using State = InitialCloudWifiOnlyState;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

struct Guard final : IInitialCloudWifiWriteGuard {
    bool allowed = true;
    unsigned calls = 0;
    Result validateWifiWrite() override {
        ++calls;
        return allowed ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
    }
};

struct Sink final : IWifiCredentialSink {
    explicit Sink(Guard& guard) : guard(guard) {}
    Guard& guard;
    unsigned calls = 0;
    bool fail = false;
    Result commit(const WifiNetworkConfig& value) override {
        ++calls;
        check(guard.calls == 1 && guard.allowed, "sink ran before the late guard");
        check(value.ssid.size == 4 && std::memcmp(value.ssid.data, "Test", 4) == 0 &&
            value.credential.size == 8 && std::memcmp(value.credential.data, "safe1234", 8) == 0,
            "stage did not take ownership of exact credential bytes");
        return fail ? Result::failure(ErrorCode::InternalError) : Result::success();
    }
};

void stage(InitialCloudWifiOnly& gate, uint32_t session) {
    uint8_t ssid[] = {'T','e','s','t'};
    uint8_t secret[] = {'s','a','f','e','1','2','3','4'};
    WifiNetworkConfig config;
    config.ssid = ByteView(ssid, sizeof(ssid));
    config.credential = ByteView(secret, 7U);
    config.authentication = WifiAuthentication::Wpa2Personal;
    check(!gate.stage(session, config), "invalid credential admitted");
    config.credential = ByteView(secret, sizeof(secret));
    check(gate.stage(session, config).ok() && !gate.stage(session, config), "Set not one-shot");
    std::memset(ssid, 0, sizeof(ssid));
    std::memset(secret, 0, sizeof(secret));
}

void exerciseCommit(unsigned variant) {
    InitialCloudWifiOnly gate;
    Guard guard;
    Sink sink(guard);
    guard.allowed = variant != 1U;
    sink.fail = variant == 2U;
    constexpr uint32_t owner = 0U;
    constexpr uint32_t other = 8U;
    WifiCredentialProfile profile;
    check(!gate.apply(owner) && !gate.take(profile), "Apply before Set reached write path");
    stage(gate, owner);
    check(gate.status(owner) == State::Staged && gate.status(other) == State::Empty &&
        !gate.apply(other) && !gate.take(profile), "another session took staged bytes");
    check(gate.apply(owner).ok() && !gate.apply(owner) && gate.status(owner) == State::Pending &&
        guard.calls == 0 && sink.calls == 0, "Apply wrote credentials directly or queued more than once");
    check(gate.take(profile) && gate.status(owner) == State::Processing, "cooperative owner lost queued data");
    const Result outcome = commitInitialCloudWifiOnly(profile, guard, sink);
    check(outcome.ok() == (variant == 0U) && guard.calls == 1U && sink.calls == (variant == 1U ? 0U : 1U),
        "journal succeeded without fresh authority or replayed");
    const WifiCredentialProfile zero;
    check(profile.ssidLength == 0U && profile.credentialLength == 0U &&
        std::memcmp(profile.ssid, zero.ssid, sizeof(profile.ssid)) == 0 &&
        std::memcmp(profile.credential, zero.credential, sizeof(profile.credential)) == 0, "candidate bytes retained");
    gate.finish(outcome);
    gate.finish(Result::success());
    check(gate.status(owner) == (variant == 0U ? State::Committed : State::Failed) &&
        !gate.apply(owner) && !gate.take(profile), "terminal write could reopen");
}

void exerciseCancel() {
    InitialCloudWifiOnly gate;
    stage(gate, 7U);
    check(gate.apply(7U).ok(), "cancel fixture was not pending");
    gate.cancel();
    WifiCredentialProfile output;
    check(!gate.take(output) && !gate.apply(7U) && gate.status(7U) == State::Cancelled,
        "cancel preserved a pending commit");
}

void exerciseExactWire() {
    const uint8_t open[] = {8, 2, 0x62, 3, 0x0a, 1, 'S'};
    const uint8_t locked[] = {8, 2, 0x62, 13, 0x0a, 1, 'S', 0x12, 8, 'a','b','c','d','1','2','3','4'};
    const uint8_t apply[] = {8, 4, 0x72, 0};
    const uint8_t status[] = {0x52, 0};
    for (ByteView frame : { ByteView(open, sizeof(open)), ByteView(locked, sizeof(locked)),
        ByteView(apply, sizeof(apply)), ByteView(status, sizeof(status)) })
        check(validateInitialCloudWifiConfigWire(frame).ok(), "pinned Android request was not admitted");
    for (unsigned variant = 0; variant < 5U; ++variant) {
        uint8_t value[sizeof(locked)];
        std::memcpy(value, locked, sizeof(value));
        switch (variant) {
        case 0: value[6] = 0; break;
        case 1: value[10] = 0; break;
        case 2: value[7] = 0x1a; break; // BSSID, not password.
        case 3: value[8] = 9; break;
        default: value[3] = 0x8d; break; // Noncanonical/inexact length.
        }
        check(!validateInitialCloudWifiConfigWire(ByteView(value, sizeof(value))), "ambiguous Set was admitted");
    }
    const uint8_t duplicateStatus[] = {0x52, 0, 0x52, 0};
    const uint8_t applyWithoutOneof[] = {8, 4};
    check(!validateInitialCloudWifiConfigWire(ByteView(duplicateStatus, sizeof(duplicateStatus))) &&
        !validateInitialCloudWifiConfigWire(ByteView(applyWithoutOneof, sizeof(applyWithoutOneof))) &&
        !validateInitialCloudWifiConfigWire(ByteView()), "empty or ambiguous protobuf got success");
    uint8_t longest[104U] = {8, 2, 0x62, 99, 0x0a, 32};
    std::memset(longest + 6U, 's', 32U);
    longest[38] = 0x12;
    longest[39] = 63;
    std::memset(longest + 40U, 'a', 64U);
    check(validateInitialCloudWifiConfigWire(ByteView(longest, 103U)).ok(), "SDK max lengths rejected");
    ++longest[3];
    ++longest[39];
    check(!validateInitialCloudWifiConfigWire(ByteView(longest, sizeof(longest))), "SDK-unsupported raw PSK admitted");
}
} // namespace

int main() {
    for (unsigned variant = 0U; variant < 3U; ++variant) exerciseCommit(variant);
    exerciseCancel();
    exerciseExactWire();
    return 0;
}
