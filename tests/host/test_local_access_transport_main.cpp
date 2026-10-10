#include <BlinkerV2Advanced.h>
#include <BlinkerV2/transport/LocalAccessFrameTransport.h>
#include <BlinkerV2/security/LocalAccessNoiseSession.h>
#include <BlinkerV2/security/LocalAccessPlainSession.h>
#include "support/DeterministicNoiseCrypto.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
using namespace blinker;
using Bytes = std::vector<uint8_t>;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (false)
ByteView view(const Bytes& bytes) { return ByteView(bytes.data(), bytes.size()); }
Bytes copy(ByteView bytes) { return Bytes(bytes.data, bytes.data + bytes.size); }

class Clock final : public IClock {
public:
    uint32_t now = 100U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
};
class Random final : public IRandom {
public:
    uint8_t next = 1U;
    Result fill(MutableByteSpan output) override {
        for (size_t i = 0; i < output.size; ++i) output.data[i] = next++;
        return Result::success();
    }
};
class Key final : public IDeviceKeySource {
public:
    DeviceKey key;
    Key() { for (size_t i = 0; i < sizeof(key.bytes); ++i) key.bytes[i] = static_cast<uint8_t>(i + 1U); }
    Result load(DeviceKey& output) override { output = key; return Result::success(); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};
class Link final : public ILocalMessageLink {
public:
    uint32_t id = 0U;
    bool running = false, incoming = false, blocked = false, failSend = false, failStart = false;
    Bytes input, output, attempted;
    size_t sends = 0U, reads = 0U, closes = 0U;
    Result start() override { running = true; return failStart ? Result::failure(ErrorCode::NotConfigured) : Result::success(); }
    void stop() override { close(); running = false; }
    void poll(uint32_t) override {}
    TransportState state() const override { return running ? TransportState::Online : TransportState::Stopped; }
    uint32_t sessionId() const override { return id; }
    size_t maxMessageSize() const override { return 420U; }
    Result receive(ByteView& message) override {
        message = ByteView(); ++reads;
        if (!incoming) return Result::failure(ErrorCode::WouldBlock);
        message = view(input); incoming = false; return Result::success();
    }
    Result send(ByteView message) override {
        attempted = copy(message);
        if (blocked) return Result::failure(ErrorCode::WouldBlock);
        if (failSend) return Result::failure(ErrorCode::InternalError);
        output = attempted; ++sends; return Result::success();
    }
    void close() override { id = 0U; input.clear(); incoming = false; ++closes; }
    void inject(const Bytes& bytes) { CHECK(!incoming); input = bytes; incoming = true; }
};

template <typename Records>
struct FixtureT {
    Clock clock;
    Random random, callerRandom;
    Key key;
    SelfLocalAccessTargetPolicy policy;
    LocalAccessAuthority<2> authority;
    DeterministicTestCrypto crypto;
    Link link;
    Records records;
    uint8_t plain[400] = {}, out[420] = {}, instance[16] = {1U};
    LocalAccessFrameTransport<2, Records> transport;
    security::NoiseNnSession caller;
    Bytes received;
    uint32_t connected = 0U, disconnected = 0U, frames = 0U;
    Bytes authId = Bytes(16, 0x31U);
    uint8_t psk[32] = {}, grantMac[32] = {};
    FixtureT(bool start = true)
        : authority(key, policy), records(crypto, random), transport(link, clock, random, records, authority,
            MutableByteSpan(plain, sizeof(plain)), MutableByteSpan(out, sizeof(out))),
          caller(crypto, callerRandom) {
        CHECK(authority.configure(ByteView(instance, 16), 1, 1));
        transport.setSessionHandlers([](void* c, const RxContext& rx) {
            CHECK(rx.authenticated && rx.encrypted == Records::encrypted && !rx.bonded);
            ++static_cast<FixtureT*>(c)->connected;
        }, [](void* c, const RxContext&) { ++static_cast<FixtureT*>(c)->disconnected; }, this);
        transport.setReceiver([](void* c, ByteView bytes, const RxContext&) {
            FixtureT& f = *static_cast<FixtureT*>(c); f.received = copy(bytes); ++f.frames;
        }, this);
        if (start) CHECK(transport.start());
    }
    ~FixtureT() { transport.stop(); }
    void poll() { transport.poll(1000U); }
    Bytes grant(uint32_t id = 1U, uint32_t permissions = 3U, uint32_t lifetime = 5000U) {
        link.id = id; poll();
        local_access::ChallengeView challenge;
        CHECK(local_access::decodeChallenge(view(link.output), challenge));
        local_access::GrantView grant;
        uint8_t grantId[16] = {2U}, callerId[16] = {3U}, mac[32] = {}, encoded[272] = {};
        grant.grantId = ByteView(grantId, 16); grant.authorizationId = view(authId);
        grant.recipientDeviceInstanceId = challenge.recipientDeviceInstanceId;
        grant.deviceKeyVersion = challenge.deviceKeyVersion;
        grant.receiverSessionId = challenge.receiverSessionId; grant.challenge = challenge.challenge;
        grant.callerSessionId = ByteView(callerId, 16); grant.authorityRevision = challenge.authorityRevision;
        grant.targetDeviceInstanceId = grant.recipientDeviceInstanceId;
        grant.permissions = permissions; grant.lifetimeMillis = lifetime; grant.securityProfile = challenge.securityProfile;
        CHECK(security::computeLocalAccessGrantAuthenticator(key.key, grant, MutableByteSpan(mac, 32)));
        grant.authenticator = ByteView(mac, 32); std::memcpy(grantMac, mac, sizeof(grantMac));
        CHECK(security::deriveLocalAccessSessionKey(key.key, grant, MutableByteSpan(psk, 32)));
        size_t size = 0U;
        CHECK(local_access::encodeGrant(grant, MutableByteSpan(encoded, sizeof(encoded)), size));
        return Bytes(encoded, encoded + size);
    }
    Bytes record(noise::LocalSecureRecordType type, ByteView payload) {
        uint8_t encoded[420] = {}; ByteView output;
        CHECK(noise::encodeLocalSecureRecord(type, payload, MutableByteSpan(encoded, sizeof(encoded)), output));
        return copy(output);
    }
    void handshake(bool wrongPsk = false, bool wrongDomain = false) {
        if (wrongPsk) psk[0] ^= 1U;
        if (!Records::encrypted) {
            uint8_t proof[32] = {}, encoded[37] = {}; size_t written = 0U;
            CHECK(security::computeLocalAccessPlainProof(ByteView(psk, 32), ByteView(grantMac, 32),
                wrongDomain ? local_access::kRecipientProofKind : local_access::kCallerProofKind, MutableByteSpan(proof, 32)));
            CHECK(local_access::encodePlainProof(local_access::kCallerProofKind, ByteView(proof, 32),
                MutableByteSpan(encoded, sizeof(encoded)), written));
            link.inject(Bytes(encoded, encoded + written)); poll(); return;
        }
        CHECK(caller.begin(security::NoiseRole::Initiator, noise::NoiseNnPattern::NnPsk0,
            ByteView(psk, 32), wrongDomain ? noise::localSecureNoisePrologue(noise::NoiseNnPattern::NnPsk0)
                : noise::localAccessNoisePrologue()));
        secureZero(MutableByteSpan(psk, 32));
        uint8_t encoded[400] = {}; ByteView message;
        CHECK(caller.writeHandshake(ByteView(), MutableByteSpan(encoded, sizeof(encoded)), message));
        link.inject(record(noise::LocalSecureRecordType::InitiatorHandshake, message)); poll();
    }
    void ready(uint32_t id = 1U, uint32_t permissions = 3U, uint32_t lifetime = 5000U) {
        link.inject(grant(id, permissions, lifetime)); poll(); handshake(); poll();
        CHECK(link.id == id);
        if (!Records::encrypted) {
            uint8_t expected[32] = {}; ByteView proof;
            CHECK(local_access::decodePlainProof(view(link.output), local_access::kRecipientProofKind, proof));
            CHECK(security::computeLocalAccessPlainProof(ByteView(psk, 32), ByteView(grantMac, 32),
                local_access::kRecipientProofKind, MutableByteSpan(expected, 32)));
            CHECK(constantTimeEqual(proof, ByteView(expected, 32))); secureZero(MutableByteSpan(psk, 32));
            CHECK(transport.sessionPermissions(id) == permissions); return;
        }
        noise::LocalSecureRecordView record;
        CHECK(noise::parseLocalSecureRecord(view(link.output), record));
        CHECK(record.type == noise::LocalSecureRecordType::ResponderHandshake);
        uint8_t payload[400] = {}; ByteView plaintext;
        CHECK(caller.readHandshake(record.body, MutableByteSpan(payload, sizeof(payload)), plaintext));
        CHECK(plaintext.empty()); CHECK(transport.sessionPermissions(id) == permissions);
    }
    Bytes encrypt(ByteView plaintext) {
        if (!Records::encrypted) return copy(plaintext);
        uint8_t encrypted[416] = {}; ByteView ciphertext;
        CHECK(caller.encryptTransport(plaintext, MutableByteSpan(encrypted, sizeof(encrypted)), ciphertext));
        return record(noise::LocalSecureRecordType::Transport, ciphertext);
    }
    Bytes decryptOutput() {
        if (!Records::encrypted) return link.output;
        noise::LocalSecureRecordView record;
        CHECK(noise::parseLocalSecureRecord(view(link.output), record));
        CHECK(record.type == noise::LocalSecureRecordType::Transport);
        uint8_t bytes[400] = {}; ByteView plaintext;
        CHECK(caller.decryptTransport(record.body, MutableByteSpan(bytes, sizeof(bytes)), plaintext));
        return copy(plaintext);
    }
};

using Fixture = FixtureT<security::LocalAccessNoiseSession>;
// Test-only constructor uniformity; no crypto provider enters production Plain.
struct PlainRecords : security::LocalAccessPlainSession {
    PlainRecords(INoiseCryptoProvider&, IRandom&) {}
};
using PlainFixture = FixtureT<PlainRecords>;

void roundtripAndBackpressure() {
    Fixture f; f.ready(); CHECK(f.connected == 1U);
    Bytes message{0xB2, 1, 2, 3};
    SendTarget invalid; invalid.kind = static_cast<SendTargetKind>(99);
    CHECK(f.transport.send(view(message), invalid).code() == ErrorCode::InvalidArgument);
    const Bytes wire = f.encrypt(view(message)); f.link.inject(wire); f.poll();
    CHECK(f.received == message); CHECK(f.frames == 1U);
    CHECK(f.transport.send(view(message), SendTarget()));
    CHECK(f.transport.send(view(message), SendTarget()).code() == ErrorCode::WouldBlock);
    f.link.blocked = true; f.poll(); const Bytes exact = f.link.attempted;
    f.clock.now += 500U; f.poll(); CHECK(f.link.attempted == exact);
    f.link.blocked = false; f.poll(); CHECK(f.link.output == exact); CHECK(f.decryptOutput() == message);
    CHECK(f.transport.send(view(message), SendTarget())); f.poll(); CHECK(f.decryptOutput() == message);
    const Bytes unrelated(16, 0x72U);
    CHECK(f.authority.retireAuthorization(view(unrelated)));
    CHECK(f.transport.sessionPermissions(1) == 3U); // Another scope's retirement is not global.
    // Replay fails AEAD before Runtime observes any second frame.
    f.link.inject(wire); f.poll(); CHECK(f.frames == 1U); CHECK(f.link.id == 0U);
    CHECK(f.disconnected == 0U); f.poll(); CHECK(f.disconnected == 1U);
    for (uint8_t value : f.plain) CHECK(value == 0U);
    for (uint8_t value : f.out) CHECK(value == 0U);
}

void admissionAndMalformed() {
    Fixture partial(false); partial.link.failStart = true;
    CHECK(!partial.transport.start()); CHECK(!partial.link.running);
    partial.link.failStart = false; CHECK(partial.transport.start());
    for (unsigned mode = 0; mode < 5U; ++mode) {
        Fixture f;
        Bytes grant = f.grant();
        CHECK(f.connected == 0U); CHECK(f.transport.sessionPermissions(1) == 0U);
        if (mode == 0U) grant.back() ^= 1U;
        if (mode == 1U) { grant.pop_back(); }
        f.link.inject(grant); f.poll();
        if (mode >= 2U) f.handshake(mode == 2U, mode == 3U);
        if (mode == 4U) {
            // A second grant cannot replace the established Noise handshake.
            f.link.inject(grant); f.poll();
        }
        f.poll(); CHECK(f.link.id == 0U); CHECK(f.frames == 0U);
    }
    Fixture f; const Bytes grant = f.grant();
    f.clock.now += local_access::kChallengeResponseMillis;
    f.link.inject(grant); f.poll(); CHECK(f.link.id == 0U);
    Fixture g; g.link.id = 1; g.link.blocked = true; g.poll();
    g.clock.now += 3000; g.poll(); CHECK(g.link.id == 0U);
}

void expiryRevocationAndConnections() {
    for (unsigned mode = 0; mode < 5U; ++mode) {
        Fixture f; f.ready(); Bytes payload{1, 2, 3};
        CHECK(f.transport.send(view(payload), SendTarget()));
        const size_t sends = f.link.sends;
        if (mode == 0U) f.clock.now += 5000U;
        if (mode == 1U) CHECK(f.authority.retireAuthorization(view(f.authId)));
        if (mode == 2U) f.authority.disable();
        if (mode == 3U) CHECK(f.authority.configure(ByteView(f.instance, 16), 2, 1));
        if (mode == 4U) f.link.close(); // IP/network generation invalidation.
        CHECK(f.transport.sessionPermissions(1) == 0U || mode == 4U);
        f.poll(); CHECK(f.link.sends == sends); CHECK(f.disconnected == 1U);
        CHECK(f.transport.sessionPermissions(1) == 0U);
    }
    Fixture f; f.ready(); f.clock.now += 4000U;
    CHECK(f.transport.disconnectSession(1)); CHECK(f.disconnected == 0U); f.poll();
    f.ready(2); CHECK(f.connected == 2U); // Same authorization has only 1s left.
    f.clock.now += 1000U; f.poll(); CHECK(f.link.id == 0U);
    CHECK(f.disconnected == 2U);
    f.link.id = 1U; f.poll(); CHECK(f.link.id == 0U); // Old native ID rejected.
    Fixture g; g.ready(); g.link.failSend = true;
    const Bytes payload{1}; CHECK(g.transport.send(view(payload), SendTarget()));
    g.poll(); CHECK(g.link.id == 0U); g.poll(); CHECK(g.disconnected == 1U);
    Fixture h; h.ready(); h.link.id = 2U; h.poll();
    CHECK(h.disconnected == 1U); CHECK(h.connected == 1U); CHECK(h.link.id == 2U);
    CHECK(h.transport.sessionPermissions(1) == 0U); CHECK(h.transport.sessionPermissions(2) == 0U);
    local_access::ChallengeView challenge;
    CHECK(local_access::decodeChallenge(view(h.link.output), challenge));
}

void plainAdmissionAndLifecycle() {
    for (unsigned mode = 0; mode < 5U; ++mode) {
        PlainFixture f; Bytes grant = f.grant();
        if (mode == 0U) grant.back() ^= 1U;
        if (mode == 1U) { grant[grant.size() - 35U] = 1U; } // Changes MAC-bound profile to Noise.
        f.link.inject(grant); f.poll();
        CHECK(f.connected == 0 && f.transport.sessionPermissions(1) == 0);
        if (mode == 2U || mode == 3U) f.handshake(mode == 2U, mode == 3U);
        if (mode == 4U) { f.link.inject(Bytes{0xb2, 1, 2, 3}); f.poll(); } // BBP before proof rejected.
        f.poll(); CHECK(f.link.id == 0 && f.frames == 0);
    }
    for (unsigned mode = 0; mode < 4U; ++mode) {
        PlainFixture f; f.ready();
        CHECK(f.connected == 1U);
        Bytes body{0xb2, 1, 2, 3}; f.link.inject(body); f.poll(); CHECK(f.received == body);
        CHECK(f.transport.send(view(body), SendTarget()));
        CHECK(f.transport.send(view(body), SendTarget()).code() == ErrorCode::WouldBlock);
        f.link.blocked = true; f.poll(); CHECK(f.link.attempted == body); // No security envelope.
        const size_t sends = f.link.sends;
        if (mode == 0U) f.clock.now += 5000U;
        if (mode == 1U) CHECK(f.authority.retireAuthorization(view(f.authId)));
        if (mode == 2U) CHECK(f.authority.configure(ByteView(f.instance, 16), 2, 1));
        if (mode == 3U) f.link.close();
        f.link.blocked = false; f.poll();
        CHECK(f.link.sends == sends && f.disconnected == 1U && f.transport.sessionPermissions(1) == 0U);
        for (uint8_t byte : f.out) CHECK(byte == 0);
    }
    PlainFixture f; const Bytes stale = f.grant(); f.link.inject(stale); f.poll(); f.handshake(); f.poll();
    CHECK(f.connected == 1U); CHECK(f.transport.disconnectSession(1)); f.poll();
    f.link.id = 2; f.poll(); f.link.inject(stale); f.poll();
    CHECK(f.link.id == 0U && f.connected == 1U); // Old grant/proof cannot establish a new socket.
    std::cout << "Plain records=" << sizeof(security::LocalAccessPlainSession)
        << " Noise records=" << sizeof(security::LocalAccessNoiseSession)
        << " shared transport=" << sizeof(LocalAccessFrameTransport<2, security::LocalAccessPlainSession>) << '\n';
}

Bytes frame(bbp2::MessageKind kind, uint16_t sequence, ByteView body) {
    Bytes output(bbp2::kBaseHeaderSize + body.size);
    bbp2::FrameHeader header; header.kind = static_cast<uint8_t>(kind);
    if (kind == bbp2::MessageKind::Command) header.flags = bbp2::FlagAckRequired;
    header.sequence = sequence; header.bodyLength = static_cast<uint16_t>(body.size);
    CHECK(bbp2::encodeHeader(header, MutableByteSpan(output.data(), output.size())));
    std::memcpy(output.data() + bbp2::kBaseHeaderSize, body.data, body.size);
    return output;
}

template <typename F>
void runtimeScope(uint32_t permissions) {
    F f(false);
    const EndpointDescriptor endpoint(StringView("lamp"), EndpointKind::Property, ValueType::Boolean,
        static_cast<uint8_t>(AccessRead | AccessWrite | AccessNotify));
    const EndpointDescriptor* slots[1] = {};
    EndpointRegistry endpoints(slots, 1); CHECK(endpoints.add(endpoint));
    TransportHub hub; CHECK(hub.addTransport(f.transport));
    uint8_t tx[400] = {}, bodyBytes[256] = {};
    DeviceRuntimeConfig config; config.maxFrameSize = 400; config.maxReassemblySize = 400;
    DeviceRuntime runtime(hub, endpoints, MutableByteSpan(tx, sizeof(tx)), config);
    unsigned commands = 0U;
    runtime.setCommandHandler([](void* c, const EndpointDescriptor&, ByteView, cbor::Type,
        const RxContext&) { ++*static_cast<unsigned*>(c); return Result::success(); }, &commands);
    CHECK(runtime.start()); f.ready(1, permissions);
    // Exact App encodeDirectAppHelloBody('transport', 400) fixture. Transport
    // Noise has already authenticated this caller; no BLE Method 2 is offered.
    const uint8_t appHello[] = {0xa6, 0x00, 0x01, 0x01, 0x81, 0x02,
        0x02, 0x18, 0xc3, 0x03, 0x19, 0x01, 0x90, 0x04, 0x19, 0x01, 0x90, 0x09, 0x01};
    ByteView body(appHello, sizeof(appHello)); bbp2::HelloBody hello;
    CHECK(bbp2::decodeHelloBody(body, hello));
    CHECK(hello.authorizationMethodCount == 0U && hello.reliableReceiveWindow == 1U);
    f.link.inject(f.encrypt(view(frame(bbp2::MessageKind::Hello, 1, body))));
    runtime.poll(1000); runtime.poll(1000);
    bbp2::FrameView response;
    Bytes decoded = f.decryptOutput(); CHECK(bbp2::parseFrame(view(decoded), response));
    CHECK(response.header.kind == static_cast<uint8_t>(bbp2::MessageKind::Hello));
    CHECK(bbp2::decodeHelloBody(response.body, hello)); CHECK(hello.authorizationMethodCount == 0U);
    // Observe scope may read Manifest, but never inherits control from AEAD.
    bbp2::ManifestRequestBody request;
    CHECK(bbp2::encodeManifestRequestBody(request, MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body));
    f.link.inject(f.encrypt(view(frame(bbp2::MessageKind::ManifestRequest, 2, body))));
    runtime.poll(1000); runtime.poll(1000); decoded = f.decryptOutput();
    CHECK(bbp2::parseFrame(view(decoded), response));
    CHECK(response.header.kind == static_cast<uint8_t>((permissions & 1U) ? bbp2::MessageKind::Manifest : bbp2::MessageKind::Error));
    bbp2::KeyedBodyWriter writer(MutableByteSpan(bodyBytes, sizeof(bodyBytes)));
    CHECK(writer.begin(1)); CHECK(writer.writeBool(StringView("lamp"), true)); CHECK(writer.finish(body));
    f.link.inject(f.encrypt(view(frame(bbp2::MessageKind::Command, 3, body))));
    runtime.poll(1000); runtime.poll(1000);
    CHECK(commands == ((permissions & 2U) ? 1U : 0U));
    decoded = f.decryptOutput(); CHECK(bbp2::parseFrame(view(decoded), response));
    SendTarget target; target.kind = SendTargetKind::Session; target.transportId = 0; target.sessionId = 1;
    const Result sent = runtime.sendCurrentPatch(body, target);
    CHECK(sent.ok() == ((permissions & 1U) != 0U));
    if (sent) { runtime.poll(1000); (void)f.decryptOutput(); }
    // No port poll needed for Runtime's outbound permission recheck.
    CHECK(f.authority.retireAuthorization(view(f.authId)));
    CHECK(!runtime.sendCurrentPatch(body, target)); runtime.poll(1000);
    runtime.stop();
}
} // namespace

int main() {
    roundtripAndBackpressure(); admissionAndMalformed(); expiryRevocationAndConnections();
    runtimeScope<Fixture>(1U); runtimeScope<Fixture>(2U); runtimeScope<Fixture>(3U);
    runtimeScope<PlainFixture>(1U); runtimeScope<PlainFixture>(2U); runtimeScope<PlainFixture>(3U);
    plainAdmissionAndLifecycle();
    std::cout << "LocalAccess single-self transport, bounded queue and Runtime permissions passed\n";
    std::cout << "transport=" << sizeof(LocalAccessFrameTransport<2, security::LocalAccessNoiseSession>) << " buffers=820 port-RX<=420\n";
    return 0;
}
