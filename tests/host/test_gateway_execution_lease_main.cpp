#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "BlinkerV2/runtime/GatewayExecutionLease.h"
using namespace blinker;
using namespace blinker::gateway;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
namespace {
class Random final : public IRandom {
public:
    uint8_t first = 0x20U;
    bool fail = false, zero = false;
    Result fill(MutableByteSpan out) override {
        if (fail) return Result::failure(ErrorCode::NotConfigured);
        for (size_t i = 0; i < out.size; ++i) out.data[i] = zero ? 0U : static_cast<uint8_t>(first + i);
        return Result::success();
    }
};
std::vector<uint8_t> hex(const char* text) {
    std::vector<uint8_t> out;
    for (size_t i = 0; text[i]; i += 2) {
        const char pair[] = {text[i], text[i + 1], 0};
        out.push_back(static_cast<uint8_t>(std::strtoul(pair, nullptr, 16)));
    }
    return out;
}
ByteView view(const std::vector<uint8_t>& value) { return ByteView(value.data(), value.size()); }
void wire() {
    // Same fixed bytes as Service; no Unix timestamp.
    auto bytes = hex("8b0104020769626c655f6368696c64500102030405060708090a0b0c0d0e0f10030450202122232425262728292a2b2c2d2e2f01197530");
    GatewayExecutionDemandView d;
    CHECK(decodeGatewayExecutionDemand(view(bytes), d));
    CHECK(d.operation == GatewayExecutionOperation::Decision && d.challengeId == 1U && d.leaseMillis == 30000U);
    uint8_t out[kGatewayExecutionMaximumEncodedSize]; size_t size = 0U;
    CHECK(encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    CHECK(size == bytes.size() && std::memcmp(out, bytes.data(), size) == 0);
    Random random; GatewayExecutionLease lease;
    CHECK(lease.begin(random)); CHECK(lease.request(1000U));
    CHECK(lease.apply(d.incarnation, d.challengeId, d.leaseMillis, 4000U));
    CHECK(lease.wanted(30999U)); CHECK(!lease.wanted(31000U)); // NOT receipt + 30 s.
    for (size_t n = 0; n < bytes.size(); ++n) CHECK(!decodeGatewayExecutionDemand(ByteView(bytes.data(), n), d));
    bytes.push_back(0U); CHECK(!decodeGatewayExecutionDemand(view(bytes), d)); bytes.pop_back();
    auto bad = bytes; bad.insert(bad.begin(), 0x98U); bad[1] = 11U;
    CHECK(!decodeGatewayExecutionDemand(view(bad), d));
    for (size_t i = 0; i < 4; ++i) { bad = bytes; bad[i] = 5U; CHECK(!decodeGatewayExecutionDemand(view(bad), d)); }
    CHECK(decodeGatewayExecutionDemand(view(bytes), d));
    d.operation = GatewayExecutionOperation::Challenge;
    CHECK(!encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    d.leaseMillis = 0; CHECK(encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    GatewayExecutionDemandView decoded;
    CHECK(decodeGatewayExecutionDemand(ByteView(out, size), decoded));
    CHECK(decoded.operation == GatewayExecutionOperation::Challenge && decoded.leaseMillis == 0U);
    d.operation = GatewayExecutionOperation::Decision;
    CHECK(encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    CHECK(decodeGatewayExecutionDemand(ByteView(out, size), decoded));
    CHECK(decoded.operation == GatewayExecutionOperation::Decision && decoded.leaseMillis == 0U);
    d.operation = GatewayExecutionOperation::Probe;
    CHECK(!encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    d.incarnation = ByteView(); d.challengeId = 0;
    CHECK(encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    CHECK(decodeGatewayExecutionDemand(ByteView(out, size), d));
    CHECK(decodeGatewayExecutionDemand(view(bytes), d));
    char longestId[128]; std::memset(longestId, 'a', sizeof(longestId));
    d.childLogicalDeviceId = StringView(longestId, sizeof(longestId));
    d.topologyVersion = d.accessEpoch = d.gatewayCredentialVersion = d.challengeId = UINT32_MAX;
    CHECK(encodeGatewayExecutionDemand(d, MutableByteSpan(out, sizeof(out)), size));
    CHECK(size <= kGatewayExecutionMaximumEncodedSize);
    CHECK(decodeGatewayExecutionDemand(ByteView(out, size), decoded));
    CHECK(decoded.challengeId == UINT32_MAX && decoded.childLogicalDeviceId.size == sizeof(longestId));
}
void deadlineAndLoss() {
    Random random; GatewayExecutionLease x;
    CHECK(!x.request(0)); CHECK(!x.wanted(0)); CHECK(x.begin(random)); CHECK(x.request(1000));
    CHECK(x.request(2000).code() == ErrorCode::AlreadyExists); CHECK(x.challengeId() == 1 && !x.wanted(2000));
    CHECK(!x.apply(x.incarnation(), 1, 30000, 6000)); CHECK(!x.wanted(6000)); // Delayed timer is irrelevant.
    CHECK(x.request(7000)); CHECK(!x.apply(x.incarnation(), 2, 1000, 8000)); CHECK(!x.wanted(8000));
    CHECK(x.request(9000)); CHECK(x.apply(x.incarnation(), 3, 30000, 10000));
    CHECK(!x.apply(x.incarnation(), 3, 30000, 11000)); // Duplicate cannot extend.
    CHECK(x.request(19000)); CHECK(!x.apply(x.incarnation(), 3, 30000, 20000));
    CHECK(x.wanted(38999)); CHECK(!x.wanted(39000)); CHECK(!x.wanted(50000)); CHECK(x.challengeId() == 4);
}
void releaseAndIdentity() {
    Random random; GatewayExecutionLease x; CHECK(x.begin(random)); CHECK(x.request(1000));
    CHECK(x.apply(x.incarnation(), 1, 0, 1100)); CHECK(!x.apply(x.incarnation(), 1, 30000, 1200));
    CHECK(x.request(1500).code() == ErrorCode::WouldBlock);
    CHECK(x.request(2000)); CHECK(x.apply(x.incarnation(), 2, 30000, 2100));
    CHECK(x.apply(x.incarnation(), 1, 0, 2200)); CHECK(x.wanted(2200)); // Old release.
    CHECK(x.request(12000)); CHECK(x.apply(x.incarnation(), 2, 0, 12100));
    CHECK(!x.wanted(12100) && x.pending(12100));
    CHECK(x.apply(x.incarnation(), 3, 30000, 12200)); CHECK(x.wanted(12200));
    CHECK(!x.apply(x.incarnation(), 4, 0, 12300)); // No unissued future fence.
    uint8_t old[16]; std::memcpy(old, x.incarnation().data, sizeof(old));
    random.first = 0x40; CHECK(x.begin(random)); CHECK(x.request(13000));
    CHECK(!x.apply(ByteView(old, 16), 1, 30000, 13100)); CHECK(!x.apply(ByteView(old, 16), 1, 0, 13100));
    CHECK(x.apply(x.incarnation(), 1, 30000, 13100)); x.clear(); CHECK(!x.wanted(13101) && !x.request(13101));
}
void clockAndEntropy() {
    Random random; GatewayExecutionLease x;
    random.fail = true; CHECK(!x.begin(random)); random.fail = false; random.zero = true; CHECK(!x.begin(random));
    random.zero = false; CHECK(x.begin(random)); CHECK(x.request(UINT32_MAX - 999U));
    CHECK(x.apply(x.incarnation(), 1, 30000, 10)); CHECK(x.wanted(28999)); CHECK(!x.wanted(29000));
    CHECK(x.request(30000)); CHECK(!x.apply(x.incarnation(), 2, 30000, 29999)); // Unexpected backwards monotonic time.
    CHECK(!x.wanted(30000)); CHECK(x.request(31000));
    CHECK(!x.apply(x.incarnation(), 3, 30001, 31001)); CHECK(x.pending(31001));
    CHECK(x.apply(x.incarnation(), 3, 5, 31002)); CHECK(x.wanted(31004)); CHECK(!x.wanted(31005));
}
}
int main(int argc, char** argv) {
    if (argc != 1) {
        // Test-only binary boundary for Service-produced CBOR, not firmware API.
        CHECK(argc == 5 && std::strcmp(argv[1], "--wire") == 0);
        const size_t length = std::strlen(argv[2]);
        CHECK(length != 0U && length % 2U == 0U && length <= 2U * kGatewayExecutionMaximumEncodedSize);
        for (size_t i = 0U; i < length; ++i) CHECK(std::strchr("0123456789abcdefABCDEF", argv[2][i]) != nullptr);
        uint32_t times[2];
        for (size_t i = 0U; i < 2U; ++i) {
            char* end = nullptr;
            const unsigned long long parsed = std::strtoull(argv[i + 3U], &end, 10);
            CHECK(end != argv[i + 3U] && *end == '\0' && parsed <= UINT32_MAX);
            times[i] = static_cast<uint32_t>(parsed);
        }
        const auto bytes = hex(argv[2]); GatewayExecutionDemandView decision;
        CHECK(decodeGatewayExecutionDemand(view(bytes), decision));
        CHECK(decision.operation == GatewayExecutionOperation::Decision);
        Random random; GatewayExecutionLease lease;
        CHECK(lease.begin(random)); CHECK(lease.request(times[0]));
        const bool accepted = lease.apply(decision.incarnation, decision.challengeId, decision.leaseMillis, times[1]).ok();
        const bool wanted = lease.wanted(times[1]);
        std::cout << "{\"accepted\":" << (accepted ? "true" : "false")
            << ",\"wanted\":" << (wanted ? "true" : "false") << "}\n";
        if (wanted) {
            CHECK(lease.wanted(times[0] + decision.leaseMillis - 1U));
            CHECK(!lease.wanted(times[0] + decision.leaseMillis));
        }
        return 0;
    }
    wire(); deadlineAndLoss(); releaseAndIdentity(); clockAndEntropy();
    std::cout << "Execution lease golden/deadline/loss/replay/release/clock/random passed; state=" << sizeof(GatewayExecutionLease) << " B\n";
}
