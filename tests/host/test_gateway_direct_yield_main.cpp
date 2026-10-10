#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "BlinkerV2/runtime/GatewayDirectYieldLease.h"
#include "BlinkerV2/runtime/GatewayChildConnection.h"
using namespace blinker;
using namespace blinker::gateway;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
namespace {
class Random final : public IRandom {
public:
    uint8_t first = 0x20U;
    Result fill(MutableByteSpan out) override {
        for (size_t i = 0; i < out.size; ++i) out.data[i] = static_cast<uint8_t>(first + i);
        return Result::success();
    }
};
std::vector<uint8_t> hex(const char* text) {
    std::vector<uint8_t> out;
    const size_t length = std::strlen(text);
    CHECK(length % 2U == 0U && length <= 400U);
    for (size_t i = 0; i < length; i += 2) {
        CHECK(std::strchr("0123456789abcdefABCDEF", text[i]) && std::strchr("0123456789abcdefABCDEF", text[i + 1]));
        char pair[] = {text[i], text[i + 1], 0}; out.push_back(static_cast<uint8_t>(std::strtoul(pair, nullptr, 16)));
    }
    return out;
}
ByteView view(const std::vector<uint8_t>& bytes) { return ByteView(bytes.data(), bytes.size()); }
const char* golden = "8c0105020769626c655f6368696c64500102030405060708090a0b0c0d0e0f10030450202122232425262728292a2b2c2d2e2f0119753001";
void identity(GatewayDirectYieldView& out, const GatewayDirectYieldView& in) {
    out.demand.topologyVersion = in.demand.topologyVersion;
    out.demand.childLogicalDeviceId = in.demand.childLogicalDeviceId;
    out.demand.childDeviceInstanceId = in.demand.childDeviceInstanceId;
    out.demand.accessEpoch = in.demand.accessEpoch;
    out.demand.gatewayCredentialVersion = in.demand.gatewayCredentialVersion;
}
GatewayDirectYieldView decision(GatewayDirectYieldLease& lease, uint32_t control, uint32_t time, uint32_t duration = 30000U) {
    CHECK(lease.probe(control, lease.incarnation(), time)); GatewayDirectYieldView out;
    CHECK(lease.challenge(time, out)); out.demand.operation = GatewayExecutionOperation::Decision;
    out.demand.leaseMillis = duration; return out;
}
void wire() {
    auto bytes = hex(golden); GatewayDirectYieldView d;
    CHECK(decodeGatewayDirectYield(view(bytes), d));
    CHECK(d.controlId == 1U && d.demand.leaseMillis == 30000U);
    uint8_t out[200]; size_t size = 0;
    CHECK(encodeGatewayDirectYield(d, MutableByteSpan(out, sizeof(out)), size));
    CHECK(size == bytes.size() && std::memcmp(out, bytes.data(), size) == 0);
    GatewayExecutionDemandView old;
    CHECK(!decodeGatewayExecutionDemand(view(bytes), old));
    for (size_t i = 0; i < bytes.size(); ++i) CHECK(!decodeGatewayDirectYield(ByteView(bytes.data(), i), d));
    bytes.push_back(0); CHECK(!decodeGatewayDirectYield(view(bytes), d)); bytes.pop_back();
    auto bad = bytes; bad.insert(bad.begin(), 0x98U); bad[1] = 12;
    CHECK(!decodeGatewayDirectYield(view(bad), d));
    CHECK(decodeGatewayDirectYield(view(bytes), d));
    d.controlId = 0U; CHECK(!encodeGatewayDirectYield(d, MutableByteSpan(out, sizeof(out)), size));
    d.controlId = 1U; d.demand.operation = GatewayExecutionOperation::Yielded;
    CHECK(encodeGatewayDirectYield(d, MutableByteSpan(out, sizeof(out)), size));
    CHECK(!encodeGatewayExecutionDemand(d.demand, MutableByteSpan(out, sizeof(out)), size));
    d.demand.leaseMillis = 0; CHECK(!encodeGatewayDirectYield(d, MutableByteSpan(out, sizeof(out)), size));
    char longest[128]; std::memset(longest, 'a', sizeof(longest));
    d.demand.childLogicalDeviceId = StringView(longest, sizeof(longest));
    d.demand.topologyVersion = d.demand.accessEpoch = d.demand.gatewayCredentialVersion = d.demand.challengeId = d.controlId = UINT32_MAX;
    d.demand.leaseMillis = 30000;
    CHECK(encodeGatewayDirectYield(d, MutableByteSpan(out, sizeof(out)), size)); CHECK(size <= sizeof(out));
    CHECK(decodeGatewayDirectYield(ByteView(out, size), d));
}
void clocksAndCancellation() {
    Random random; GatewayDirectYieldLease lease; GatewayDirectYieldView out;
    CHECK(!lease.probe(1, lease.incarnation(), 0)); CHECK(lease.begin(random));
    auto d = decision(lease, 1, 1000);
    // Persist wire bytes, not a borrowed view into the receiver's incarnation.
    uint8_t originalIncarnation[16]; std::memcpy(originalIncarnation, d.demand.incarnation.data, 16);
    d.demand.incarnation = ByteView(originalIncarnation, sizeof(originalIncarnation));
    CHECK(!lease.holding(1000)); CHECK(!lease.receipt(1000, true, out));
    CHECK(lease.probe(1, lease.incarnation(), 2000).code() == ErrorCode::AlreadyExists);
    CHECK(lease.probe(2, lease.incarnation(), 2000).code() == ErrorCode::WouldBlock);
    CHECK(lease.apply(d, 3000)); CHECK(!lease.receipt(3000, false, out));
    CHECK(lease.receipt(3000, true, out)); CHECK(out.controlId == 1 && out.demand.leaseMillis == 30000);
    CHECK(lease.apply(d, 4000).code() == ErrorCode::AlreadyExists);
    CHECK(lease.holding(30999)); CHECK(!lease.holding(31000)); // Not receive + duration.
    CHECK(!lease.receipt(31000, true, out)); CHECK(!lease.probe(1, lease.incarnation(), 31000));
    auto second = decision(lease, 2, 32000);
    auto zero = second; zero.demand.leaseMillis = 0;
    CHECK(lease.apply(zero, 33000)); CHECK(!lease.apply(second, 34000)); // Release overtakes positive.
    auto third = decision(lease, 3, 35000);
    CHECK(lease.apply(third, 35100));
    CHECK(!lease.apply(zero, 35200)); CHECK(lease.holding(35200));
    auto wrong = third; wrong.controlId = 4; CHECK(!lease.apply(wrong, 35300));
    uint8_t changed[16]; std::memset(changed, 9, sizeof(changed));
    wrong = third; wrong.demand.incarnation = ByteView(changed, sizeof(changed)); CHECK(!lease.apply(wrong, 35300));
    lease.clear(); CHECK(!lease.holding(36000)); CHECK(!lease.apply(third, 36000));
    random.first = 0x40; CHECK(lease.begin(random));
    CHECK(!lease.probe(UINT32_MAX, d.demand.incarnation, 0)); // Old Probe cannot poison the new floor.
    CHECK(lease.probe(1, lease.incarnation(), 0)); CHECK(!lease.apply(d, 100));
    CHECK(lease.begin(random)); const uint32_t nearWrap = UINT32_MAX - 500U;
    auto wrapped = decision(lease, 1, nearWrap, 2000);
    CHECK(lease.apply(wrapped, nearWrap + 100U));
    CHECK(lease.holding(nearWrap + 1999U)); CHECK(!lease.holding(nearWrap + 2000U));
    CHECK(lease.begin(random)); auto late = decision(lease, UINT32_MAX, 1000);
    CHECK(!lease.apply(late, 6000)); CHECK(!lease.probe(UINT32_MAX, lease.incarnation(), 7000)); CHECK(!lease.probe(1, lease.incarnation(), 7000));
}
void retiredPromise() {
  for (const uint32_t duration : {2000U, 30000U}) {
    Random random; GatewayDirectYieldLease lease; GatewayDirectYieldView out;
    CHECK(lease.begin(random));
    const uint32_t start = UINT32_MAX - 500U;
    auto d = decision(lease, 1, start, duration);
    CHECK(lease.apply(d, start + 100)); CHECK(lease.receipt(start + 100, true, out));
    lease.retire(start + 200); CHECK(lease.holding(start + 300));
    CHECK(!lease.receipt(start + 300, true, out)); // Old authority can no longer emit a receipt.
    lease.retire(start + 400); // Repeated offline polling cannot extend it.
    uint8_t incarnation[16] = {2}; CHECK(lease.begin(ByteView(incarnation, 16)));
    auto zero = decision(lease, 1, start + 500, 0); CHECK(lease.apply(zero, start + 500));
    CHECK(lease.holding(start + duration - 11)); // New authority's zero cannot cancel the old physical promise.
    lease.retire(start + duration - 10); // Even a second reconnect near expiry cannot refund time.
    CHECK(lease.holding(start + duration - 1));
    CHECK(!lease.holding(start + duration));
  }
  // No positive Decision means no physical promise to retain across reconnect.
  Random random; GatewayDirectYieldLease lease; GatewayDirectYieldView out;
  CHECK(lease.begin(random));
  auto pending = decision(lease, 1, 1000U);
  uint8_t oldIncarnation[16]; std::memcpy(oldIncarnation, pending.demand.incarnation.data, sizeof(oldIncarnation));
  pending.demand.incarnation = ByteView(oldIncarnation, sizeof(oldIncarnation));
  lease.retire(1100U); CHECK(!lease.holding(1100U));
  uint8_t nextIncarnation[16] = {3}; CHECK(lease.begin(ByteView(nextIncarnation, sizeof(nextIncarnation))));
  CHECK(!lease.apply(pending, 1200U)); CHECK(!lease.receipt(1200U, true, out));
  CHECK(!lease.holding(1200U));
}
class Child {
public:
    GatewayChildSessionState s = GatewayChildSessionState::Secure;
    unsigned disconnects = 0;
    GatewayChildSessionState state() const { return s; }
    bool secure() const { return s == GatewayChildSessionState::Secure; }
    uint32_t attemptId() const { return 3; }
    Result start() { s = GatewayChildSessionState::Idle; return Result::success(); }
    Result connect() { s = GatewayChildSessionState::Connecting; return Result::success(); }
    Result disconnect() { ++disconnects; s = GatewayChildSessionState::Disconnecting; return Result::success(); }
};
void physicalBarrier() {
    Random random; GatewayDirectYieldLease lease; Child child; BasicGatewayChildConnection<Child> connection(child);
    CHECK(lease.begin(random)); auto d = decision(lease, 1, 1000); CHECK(lease.apply(d, 1100));
    connection.allowAcquisition(false);
    GatewayDirectYieldView receipt;
    CHECK(!lease.receipt(1200, connection.retire(child.attemptId()), receipt));
    CHECK(child.disconnects == 1 && connection.closing());
    CHECK(!lease.receipt(1300, connection.retire(child.attemptId()), receipt)); CHECK(child.disconnects == 1);
    child.s = GatewayChildSessionState::Fault; CHECK(!connection.retire(child.attemptId()));
    child.s = GatewayChildSessionState::Idle; CHECK(connection.retire(child.attemptId()));
    for (unsigned otherOwner = 0; otherOwner < 3; ++otherOwner) {
        // Management, permit or native closing is independently sufficient to
        // withhold a receipt even after the ordinary connection is released.
        CHECK(!lease.receipt(1400, false, receipt));
    }
    CHECK(lease.receipt(1500, true, receipt)); CHECK(!connection.ensureConnected());
    CHECK(child.state() == GatewayChildSessionState::Idle);
}
}
int main(int argc, char** argv) {
    if (argc > 1) {
        CHECK(argc == 6 && std::strcmp(argv[1], "--wire") == 0);
        auto bytes = hex(argv[2]); GatewayDirectYieldView d;
        CHECK(decodeGatewayDirectYield(view(bytes), d));
        CHECK(d.demand.operation == GatewayExecutionOperation::Decision);
        uint32_t times[2];
        for (size_t i = 0; i < 2; ++i) {
            char* end = nullptr; const char* input = argv[i + 3];
            CHECK(*input >= '0' && *input <= '9');
            const unsigned long long value = std::strtoull(input, &end, 10);
            CHECK(end != input && *end == '\0' && value <= UINT32_MAX); times[i] = static_cast<uint32_t>(value);
        }
        CHECK(std::strcmp(argv[5], "0") == 0 || std::strcmp(argv[5], "1") == 0);
        const uint32_t issued = times[0], received = times[1];
        Random random; GatewayDirectYieldLease lease; CHECK(lease.begin(random)); CHECK(lease.probe(d.controlId, lease.incarnation(), issued));
        const bool accepted = lease.apply(d, received).ok(); GatewayDirectYieldView receipt;
        const bool released = lease.receipt(received, std::strcmp(argv[5], "1") == 0, receipt).ok();
        std::cout << "{\"accepted\":" << (accepted ? "true" : "false") << ",\"receipt\":\"";
        if (released) {
            identity(receipt, d); uint8_t out[200]; size_t size = 0;
            CHECK(encodeGatewayDirectYield(receipt, MutableByteSpan(out, sizeof(out)), size));
            const char* digits = "0123456789abcdef";
            for (size_t i = 0; i < size; ++i) std::cout << digits[out[i] >> 4U] << digits[out[i] & 15U];
        }
        std::cout << "\"}\n"; return 0;
    }
    wire(); clocksAndCancellation(); retiredPromise(); physicalBarrier();
    std::cout << "Direct yield wire/receiver clock/replay/cancel/native barrier passed; state=" << sizeof(GatewayDirectYieldLease) << " B\n";
}
