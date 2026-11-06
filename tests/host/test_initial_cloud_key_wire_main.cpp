// Reuse the original Method 2/BLE fixture without running its old matrix.
#define main initialCloudKeyAuthorityFixtureMain
#include "test_initial_cloud_key_registration_main.cpp"
#undef main
#include <BlinkerV2/security/InitialCloudKeyRegistrationEndpoint.h>
#include <BlinkerV2/security/InitialCloudWifiAccess.h>
#include <BlinkerV2/runtime/PreCloudWifiHandoffSupervisor.h>

namespace {

// A synchronous host owner substitute; the original Store still does the
// complete root CAS, late guard, write, and exact readback on the test blob.
class PersistingInstaller final : public IInitialCloudKeyInstaller, public IDeviceAccessChangeObserver {
public:
    explicit PersistingInstaller(Fixture& fixture)
        : calls(0U), failure(ErrorCode::Ok), dropBinding(false), f(fixture), calling(false), held(false) {
        f.access.setChangeObserver(this);
    }
    ~PersistingInstaller() override { f.access.setChangeObserver(nullptr); }

    Result installInitialCloudKeyAuthorized(const DeviceKey& key, uint32_t epoch,
        const ControllerCredential& admin, IInitialCloudKeyCommitGuard& guard) override {
        ++calls;
        if (failure != ErrorCode::Ok) return Result::failure(failure);
        calling = true;
        const Result result = f.access.installInitialCloudKeyAuthorized(key, epoch, admin, guard);
        calling = false;
        // Simulate losing the committed session after storage but before send.
        if (result && dropBinding) f.direct.cancel(f.session.sessionId);
        return result;
    }
    Result beginDeviceAccessChange() override { return Result::failure(ErrorCode::UnsupportedFeature); }
    void endDeviceAccessChange(Result) override {}
    Result beginCooperativeInitialCloudKeyChange() override {
        if (!calling || held) return Result::failure(ErrorCode::StateConflict);
        held = true;
        f.clock.now += preCommitElapsed;
        return Result::success();
    }
    void endCooperativeInitialCloudKeyChange(Result) override { held = false; }

    unsigned calls;
    ErrorCode failure;
    bool dropBinding;
    Fixture& f;
    bool calling;
    bool held;
    uint32_t preCommitElapsed = 0U;
};

struct WireFixture {
    Fixture f;
    PersistingInstaller installer;
    security::InitialCloudKeyRegistrationEndpoint endpoint;
    security::InitialCloudKeyInstallOutcome outcome;
    Result handoffOutcome;
    unsigned received;
    uint16_t maximum;
    uint8_t nextRecord;
    uint8_t notification[96U];
    size_t notificationSize;
    uint8_t nextFragment;
    bool notificationComplete;

    explicit WireFixture(bool authenticate = true,
        security::InitialCloudKeySetupWindow setup = security::InitialCloudKeySetupWindow())
        : f(100U, false, true, setup), installer(f), endpoint(f.registration, installer, f.direct), outcome(), handoffOutcome(), received(0U),
          maximum(512U), nextRecord(2U), notification(), notificationSize(0U), nextFragment(0U), notificationComplete(false) {
        std::memset(f.intent, 1, sizeof(f.intent));
        std::memset(f.key.bytes, 6, sizeof(f.key.bytes));
        if (authenticate) {
            check(f.authenticate(), "wire Method 2 fixture failed");
            drain(); // Do not confuse original plaintext AuthResult with the new ACK.
        }
        f.direct.setReceiver(&WireFixture::onFrame, this);
        f.link.observeSent = &WireFixture::onPacket;
        f.link.observeSentContext = this;
    }

    void drain() {
        for (unsigned i = 0U; i < 16U && f.direct.queuedFrameCount() != 0U; ++i) f.direct.poll(1000U);
        check(f.direct.queuedFrameCount() == 0U, "fixture queue did not drain");
    }
    static void onFrame(void* context, ByteView frame, const RxContext& rx) {
        WireFixture& self = *static_cast<WireFixture*>(context);
        ++self.received;
        if (frame.size > 3U && frame.data[3] == static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyHandoff))
            self.handoffOutcome = self.endpoint.receive(frame, rx, self.maximum);
        else self.outcome = self.endpoint.handle(frame, rx, self.maximum);
    }
    static void onPacket(void* context, ByteView packet) {
        WireFixture& self = *static_cast<WireFixture*>(context);
        ble::FragmentView fragment;
        check(ble::parseFragment(packet, fragment).ok(), "ACK fragment invalid");
        check(!self.notificationComplete && fragment.header.index == self.nextFragment++, "unexpected ACK fragment");
        check(self.notificationSize + fragment.payload.size <= sizeof(self.notification), "ACK capture overflow");
        std::memcpy(self.notification + self.notificationSize, fragment.payload.data, fragment.payload.size);
        self.notificationSize += fragment.payload.size;
        self.notificationComplete = (fragment.header.flags & ble::FragmentFlagEnd) != 0U;
    }
    void request(uint8_t (&out)[bbp2::kInitialCloudKeyInstallFrameSize], bool validProof = true) {
        uint8_t proof[32U];
        if (validProof) f.proof(proof);
        else std::memset(proof, 7, sizeof(proof));
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyInstall);
        header.sequence = 0x1234U;
        header.bodyLength = static_cast<uint16_t>(bbp2::kInitialCloudKeyInstallBodySize);
        check(bbp2::encodeHeader(header, MutableByteSpan(out, sizeof(out))).ok(), "request header failed");
        cbor::Writer body(MutableByteSpan(out + bbp2::kBaseHeaderSize, bbp2::kInitialCloudKeyInstallBodySize));
        check(body.beginArray(4U).ok() && body.writeUnsigned(1U).ok() &&
            body.writeBytes(ByteView(f.intent, sizeof(f.intent))).ok() &&
            body.writeBytes(ByteView(f.key.bytes, sizeof(f.key.bytes))).ok() &&
            body.writeBytes(ByteView(proof, sizeof(proof))).ok() && body.size() == bbp2::kInitialCloudKeyInstallBodySize,
            "request body failed");
        secureZero(MutableByteSpan(proof, sizeof(proof)));
    }
    void inject(ByteView frame, bool encrypted = true) {
        uint8_t record[128U] = {};
        ByteView wire = frame;
        if (encrypted) check(f.initiator.secureSession().seal(f.crypto, frame,
            MutableByteSpan(record, sizeof(record)), wire).ok(), "client could not seal request");
        f.link.injectRecord(wire, nextRecord++);
    }
    void install() {
        uint8_t frame[bbp2::kInitialCloudKeyInstallFrameSize] = {};
        request(frame);
        inject(ByteView(frame, sizeof(frame)));
    }
    void handoffFrame(uint8_t (&out)[bbp2::kInitialCloudKeyHandoffFrameSize]) {
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyHandoff);
        header.sequence = 0x1235U;
        header.bodyLength = static_cast<uint16_t>(bbp2::kInitialCloudKeyInstalledBodySize);
        DeviceKeyLocator locator;
        check(deriveDeviceKeyLocator(f.key, locator).ok() &&
            bbp2::encodeHeader(header, MutableByteSpan(out, sizeof(out))).ok() &&
            bbp2::encodeInitialCloudKeyInstalledBody(ByteView(f.intent, sizeof(f.intent)), locator,
                MutableByteSpan(out + bbp2::kBaseHeaderSize, bbp2::kInitialCloudKeyInstalledBodySize)).ok(),
            "handoff fixture encode failed");
    }
    void checkNoKey() {
        DeviceKey key;
        check(f.access.load(key).code() == ErrorCode::NotFound, "rejected request wrote a Key");
    }
    void checkKey() {
        DeviceKey key;
        check(f.access.load(key).ok() && sameDeviceKey(key, f.key), "stored Key differs");
        clearDeviceKey(key);
    }
};

void testSecureAckAndOneWrite() {
    WireFixture w;
    uint8_t frame[bbp2::kInitialCloudKeyInstallFrameSize] = {};
    w.request(frame);
    w.inject(ByteView(frame, sizeof(frame)));
    check(w.received == 1U && w.outcome.installation.ok() && w.outcome.response.ok() &&
        w.installer.calls == 1U && w.f.direct.queuedFrameCount() == 1U, "secure install did not queue ACK");
    w.checkKey();
    const uint8_t cleared[sizeof(frame)] = {};
    check(std::memcmp(w.f.plaintext, cleared, sizeof(cleared)) == 0, "secret receive scratch was not cleared");
    w.drain();
    check(w.notificationComplete && w.notification[0] == security::kDirectSecureMagic, "ACK was not a secure record");
    uint8_t decoded[bbp2::kInitialCloudKeyInstalledFrameSize] = {};
    ByteView ack;
    check(w.f.initiator.secureSession().open(w.f.crypto, ByteView(w.notification, w.notificationSize),
        MutableByteSpan(decoded, sizeof(decoded)), ack).ok(), "client failed to open ACK");
    const uint8_t expected[] = {
        0x42, 0x4b, 0x02, 0x39, 0x02, 0x0a, 0x12, 0x34, 0x00, 0x24,
        0x83, 0x01, 0x50, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0x50, 0x06, 0x3f, 0x21, 0xc5, 0x32, 0xbe, 0x08, 0x68, 0x32, 0x80, 0xea, 0xbb, 0xf4, 0x7e, 0xfd, 0x90
    };
    check(ack.size == sizeof(expected) && std::memcmp(ack.data, expected, sizeof(expected)) == 0, "ACK bytes drifted");
    w.inject(ByteView(frame, sizeof(frame)));
    check(w.received == 2U && !w.outcome.installation && w.installer.calls == 1U &&
        w.f.direct.queuedFrameCount() == 0U, "second request produced another install or receipt");
}

void testPlainAndWrongProofReject() {
    for (unsigned plain = 0U; plain < 2U; ++plain) {
        WireFixture w(plain == 0U);
        uint8_t frame[bbp2::kInitialCloudKeyInstallFrameSize] = {};
        w.request(frame, false);
        w.inject(ByteView(frame, sizeof(frame)), plain == 0U);
        check(w.received == 1U && w.outcome.installation.code() == ErrorCode::AuthenticationRequired &&
            w.installer.calls == 0U && w.f.direct.queuedFrameCount() == 0U, "unauthenticated install reached storage");
        w.checkNoKey();
    }
}

void testExactRequestShape() {
    WireFixture w;
    for (unsigned variant = 0U; variant < 10U; ++variant) {
        uint8_t frame[bbp2::kInitialCloudKeyInstallFrameSize] = {};
        w.request(frame);
        switch (variant) {
        case 0U: frame[3] = 0x12U; break; // no ordinary Command alias
        case 1U: frame[4] = bbp2::FlagAckRequired; break;
        case 2U: frame[6] = 0U; frame[7] = 0U; break;
        case 3U: frame[10] = 0x83U; break;
        case 4U: frame[11] = 2U; break;
        case 5U: std::memset(frame + 13U, 0, 16U); break;
        case 6U: std::memset(frame + 31U, 0, 32U); break;
        case 7U: std::memset(frame + 65U, 0, 32U); break;
        case 8U: frame[29] = 0x59U; frame[30] = 0U; frame[31] = 32U; break; // noncanonical width
        default: frame[5] = 11U; --frame[9]; break; // parseable extension is not this version
        }
        w.inject(ByteView(frame, sizeof(frame)));
        check(w.received == variant + 1U && !w.outcome.installation && w.installer.calls == 0U &&
            w.f.direct.queuedFrameCount() == 0U, "malformed request reached storage or ACK");
    }
    w.checkNoKey();
}

void testReplyCapacityBeforeWrite() {
    WireFixture w;
    uint8_t request[bbp2::kInitialCloudKeyInstallFrameSize] = {};
    w.request(request);
    w.maximum = static_cast<uint16_t>(sizeof(request) - 1U);
    w.inject(ByteView(request, sizeof(request)));
    check(w.outcome.installation.code() == ErrorCode::BufferTooSmall && w.installer.calls == 0U, "undersized negotiated frame wrote Key");
    w.maximum = 512U;
    uint8_t ordinary[bbp2::kBaseHeaderSize] = {};
    bbp2::FrameHeader header;
    check(bbp2::encodeHeader(header, MutableByteSpan(ordinary, sizeof(ordinary))).ok(), "ordinary test frame failed");
    SendTarget target; target.kind = SendTargetKind::Session; target.sessionId = w.f.session.sessionId;
    while (w.f.direct.queuedFrameCount() < BLINKER_BLE_MAX_TX_FRAMES)
        check(w.f.direct.send(ByteView(ordinary, sizeof(ordinary)), target).ok(), "queue fill failed");
    w.inject(ByteView(request, sizeof(request)));
    check(w.outcome.installation.code() == ErrorCode::WouldBlock && w.installer.calls == 0U &&
        w.outcome.response.code() == ErrorCode::NotConfigured, "full queue still admitted a write");
    w.checkNoKey();
}

void testFailedInstallNeverAcks() {
    WireFixture w;
    w.installer.failure = ErrorCode::WouldBlock;
    uint8_t frame[bbp2::kInitialCloudKeyInstallFrameSize] = {};
    w.request(frame); w.inject(ByteView(frame, sizeof(frame)));
    check(w.outcome.installation.code() == ErrorCode::WouldBlock && w.installer.calls == 1U &&
        w.f.direct.queuedFrameCount() == 0U && w.outcome.response.code() == ErrorCode::NotConfigured,
        "non-success installer returned a successful ACK");
    w.inject(ByteView(frame, sizeof(frame)));
    check(w.installer.calls == 1U && w.f.direct.queuedFrameCount() == 0U, "failed install regained one-shot permission");
}

void testLostBindingAndQueuedDisconnect() {
    for (unsigned loss = 0U; loss < 2U; ++loss) {
        WireFixture w;
        w.installer.dropBinding = loss == 0U;
        uint8_t frame[bbp2::kInitialCloudKeyInstallFrameSize] = {};
        w.request(frame); w.inject(ByteView(frame, sizeof(frame)));
        check(w.outcome.installation.ok() && w.installer.calls == 1U, "ACK failure rewrote successful installation");
        w.checkKey();
        if (loss == 0U) {
            check(!w.outcome.response && w.f.direct.queuedFrameCount() == 0U,
                "disappeared secure binding caused a plaintext ACK");
        } else {
            check(w.outcome.response.ok() && w.f.direct.queuedFrameCount() == 1U, "fixture did not queue ACK");
            w.f.link.disconnect();
            check(w.f.direct.queuedFrameCount() == 0U, "old ACK survived native disconnect");
        }
        w.f.direct.poll(1000U);
        check(w.notificationSize == 0U, "ACK emitted after owner retired");
    }
}

void testInvalidDecryptedScratchCleared() {
    WireFixture w;
    uint8_t invalid[bbp2::kInitialCloudKeyInstallFrameSize];
    std::memset(invalid, 0x11, sizeof(invalid));
    w.inject(ByteView(invalid, sizeof(invalid)));
    const uint8_t cleared[sizeof(invalid)] = {};
    check(w.received == 0U && std::memcmp(w.f.plaintext, cleared, sizeof(cleared)) == 0,
        "invalid decrypted frame was delivered or left in scratch");
}

void testHandoffExactAndSingleUse() {
    WireFixture w;
    uint8_t frame[bbp2::kInitialCloudKeyHandoffFrameSize] = {};
    w.handoffFrame(frame);
    w.inject(ByteView(frame, sizeof(frame)));
    security::InitialCloudKeyHandoff accepted;
    check(!w.handoffOutcome && !w.endpoint.takeAuthorizedHandoff(accepted), "handoff admitted before install");
    w.install();
    check(w.outcome.response.ok(), "handoff fixture install did not enqueue ACK");
    w.drain();
    for (unsigned variant = 0U; variant < 7U; ++variant) {
        w.handoffFrame(frame);
        switch (variant) {
        case 0U: frame[13] ^= 1U; break;  // Different intent.
        case 1U: frame[30] ^= 1U; break;  // Different locator.
        case 2U: frame[7] = 0x34U; break; // Old request sequence.
        case 3U: frame[4] = bbp2::FlagAckRequired; break;
        case 4U: frame[11] = 0x18U; frame[12] = 1U; break; // Noncanonical version.
        case 5U: frame[5] = 11U; --frame[9]; break;
        default: std::memset(frame + 13U, 0, 16U); break;
        }
        w.inject(ByteView(frame, sizeof(frame)));
        check(!w.handoffOutcome && !w.endpoint.takeAuthorizedHandoff(accepted), "malformed handoff authorized a new owner");
    }
    w.handoffFrame(frame);
    w.inject(ByteView(frame, sizeof(frame)));
    check(w.handoffOutcome.ok(), "same-session exact handoff did not authorize");
    w.inject(ByteView(frame, sizeof(frame)));
    check(!w.handoffOutcome && w.f.direct.queuedFrameCount() == 0U, "duplicate handoff succeeded or produced a wire response");
    w.f.link.disconnect();
    check(w.endpoint.takeAuthorizedHandoff(accepted) && std::memcmp(accepted.intent, frame + 13U, 16U) == 0 &&
        std::memcmp(accepted.locator.bytes, frame + 30U, 16U) == 0 && w.installer.calls == 1U,
        "accepted handoff did not retain the exact non-secret installed tuple across close");
    check(!w.endpoint.takeAuthorizedHandoff(accepted), "supervisor consumed the handoff twice");
}

void testHandoffInvalidated() {
    for (unsigned variant = 0U; variant < 6U; ++variant) {
        WireFixture w;
        w.installer.dropBinding = variant == 0U;
        w.install();
        check(w.outcome.installation.ok(), "invalidation fixture did not install");
        if (variant == 1U) w.f.link.disconnect();
        if (variant == 2U) w.f.clock.now += 120000U;
        if (variant == 3U) {
            w.f.access.setChangeObserver(nullptr);
            ControllerCredential changed = w.f.used;
            ++changed.credentialVersion;
            check(w.f.access.rotateVerified(1U, changed).ok(), "handoff root rotation fixture failed");
            clearControllerCredential(changed);
        }
        uint8_t frame[bbp2::kInitialCloudKeyHandoffFrameSize] = {};
        w.handoffFrame(frame);
        const ByteView encoded(frame, sizeof(frame));
        RxContext rx = w.f.verifiedRx;
        if (variant == 4U) rx.encrypted = false;
        if (variant == 5U) {
            check(w.endpoint.authorizeHandoff(encoded, rx).ok(), "explicit retirement fixture did not authorize");
            w.endpoint.retire();
        }
        security::InitialCloudKeyHandoff accepted;
        check(!w.endpoint.authorizeHandoff(encoded, rx) && !w.endpoint.takeAuthorizedHandoff(accepted) &&
            w.installer.calls == 1U, "lost ACK/binding, expired/rotated root, plaintext or cancelled handoff advanced");
    }
}

void testFailedAttemptCannotReopenAfterRekey() {
    for (unsigned variant = 0U; variant < 2U; ++variant) {
        WireFixture w;
        w.installer.failure = variant == 0U ? ErrorCode::InternalError : ErrorCode::Ok;
        w.installer.dropBinding = variant == 1U;
        w.install();
        check(w.installer.calls == 1U && !w.outcome.response, "fixture did not fail after entering storage");
        w.f.link.disconnect();
        w.f.registration.resetSessions();
        w.f.initiator.reset();
        w.f.direct.setReceiver(&Fixture::receive, &w.f);
        w.f.link.observeSent = nullptr;
        w.f.session.sessionId = 92U;
        w.f.link.connect(w.f.session.sessionId);
        check(w.f.authenticate(), "replacement peer could not authenticate");
        w.drain();
        w.f.direct.setReceiver(&WireFixture::onFrame, &w);
        w.installer.failure = ErrorCode::Ok;
        w.installer.dropBinding = false;
        w.install();
        check(!w.outcome.installation && w.installer.calls == 1U && w.f.direct.queuedFrameCount() == 0U,
            "new Method 2 session revived a prior attempted install");
    }
}

class DirectRetirement final : public IPreCloudDirectRetirement {
public:
    Result retireDirect() override { ++calls; return Result(outcome); }
    bool shutdownComplete() const override { return released; }
    unsigned calls = 0U;
    ErrorCode outcome = ErrorCode::Ok;
    bool released = false;
};

class WifiReceiver final : public IPreCloudWifiOnlyReceiver, private IWifiCredentialSink {
public:
    Result beginWifiOnly(const security::InitialCloudKeyHandoff& handoff,
        IInitialCloudWifiWriteGuard& suppliedGuard) override {
        ++starts;
        received = handoff;
        guard = &suppliedGuard;
        check(!guard->validateWifiWrite(), "receiver begin could write before active ownership");
        return Result(startOutcome);
    }
    Result commitProfile() {
        check(guard != nullptr, "receiver guard not installed");
        WifiCredentialProfile profile;
        profile.authentication = WifiAuthentication::Open;
        profile.ssidLength = 3U;
        std::memcpy(profile.ssid, "lab", 3U);
        return commitInitialCloudWifiOnly(profile, *guard, *this);
    }
    unsigned starts = 0U;
    unsigned commits = 0U;
    ErrorCode startOutcome = ErrorCode::Ok;
    IInitialCloudWifiWriteGuard* guard = nullptr;
    security::InitialCloudKeyHandoff received = {};
private:
    Result commit(const WifiNetworkConfig&) override { ++commits; return Result::success(); }
};

struct SupervisorFixture {
    WireFixture wire;
    Blob instanceBlob;
    DeviceInstanceIdStore instances;
    DirectRetirement direct;
    WifiReceiver wifi;
    PreCloudWifiHandoffSupervisor supervisor;

    explicit SupervisorFixture(uint32_t setupWindow = 600000U)
        : wire(), instanceBlob(), instances(instanceBlob), direct(), wifi(),
          supervisor(wire.endpoint, direct, wifi, instances, wire.f.access, wire.f.clock,
              wire.f.clock.now, setupWindow) {
        Random initial(1U);
        DeviceInstanceId actual;
        check(instances.loadOrCreate(initial, actual).ok() && sameDeviceInstanceId(actual, wire.f.deviceInstance),
            "fixture durable instance did not match Direct instance");
    }
    void install(bool authorize = true) {
        wire.install();
        check(wire.outcome.installation.ok() && wire.outcome.response.ok(), "handoff supervisor fixture failed to install");
        wire.drain();
        if (authorize) {
            uint8_t frame[bbp2::kInitialCloudKeyHandoffFrameSize] = {};
            wire.handoffFrame(frame);
            wire.inject(ByteView(frame, sizeof(frame)));
            check(wire.handoffOutcome.ok(), "handoff supervisor fixture failed to authorize");
        }
        wire.f.link.disconnect();
    }
    void changeRoot(unsigned variant) {
        wire.f.access.setChangeObserver(nullptr);
        if (variant == 0U) {
            Random replacement(51U);
            DeviceInstanceId other;
            check(instanceBlob.clear().ok() && instances.loadOrCreate(replacement, other).ok(), "fixture instance replace failed");
        } else if (variant == 1U) {
            ControllerCredential changed = wire.f.used;
            ++changed.credentialVersion;
            check(wire.f.access.rotateVerified(wire.f.used.credentialVersion, changed).ok(), "fixture Admin version change failed");
            clearControllerCredential(changed);
        } else if (variant == 2U) {
            DeviceKey key = wire.f.key;
            key.bytes[0] ^= 1U;
            check(wire.f.access.replace(key).ok(), "fixture Key replace failed");
            clearDeviceKey(key);
        } else if (variant == 3U) {
            wire.f.clock.now += security::kInitialCloudKeyRegistrationWindowMillis;
        } else {
            check(wire.f.blob.clear().ok(), "fixture missing root failed");
        }
    }
};

void testSupervisorNeedsExplicitAuthority() {
    SupervisorFixture f;
    f.install(false);
    f.direct.released = true;
    check(f.supervisor.advance().code() == ErrorCode::WouldBlock && f.supervisor.waitingForDirectHandoff() &&
        f.direct.calls == 0U && f.wifi.starts == 0U,
        "installed Key, plain close and native release authorized a handoff");
    f.supervisor.cancel();
    check(!f.supervisor.advance() && !f.supervisor.waitingForDirectHandoff() && f.direct.calls == 0U,
        "cancelled supervisor could progress");
}

void testSupervisorNativeAndSingleWrite() {
    SupervisorFixture f;
    f.install();
    f.direct.outcome = ErrorCode::WouldBlock;
    check(f.supervisor.advance().code() == ErrorCode::WouldBlock && !f.supervisor.waitingForDirectHandoff() &&
        f.direct.calls == 1U && f.wifi.starts == 0U,
        "known pending native retirement started receiver");
    f.direct.outcome = ErrorCode::Ok;
    f.direct.released = true;
    check(f.supervisor.advance().ok() && f.supervisor.advance().ok() && f.direct.calls == 2U && f.wifi.starts == 1U &&
        f.wifi.received.authority.epoch == 7U &&
        f.wifi.received.authority.credentialVersion == f.wire.f.used.credentialVersion &&
        sameDeviceInstanceId(f.wifi.received.authority.instance, f.wire.f.deviceInstance),
        "receiver did not begin exactly once with original non-secret authority");
    check(f.wifi.commitProfile().ok() && !f.wifi.commitProfile() && f.wifi.commits == 1U,
        "cooperative journal did not receive exactly one guarded commit");
}

void testSupervisorUncertainAndCancelled() {
    for (unsigned variant = 0U; variant < 4U; ++variant) {
        SupervisorFixture f;
        f.install();
        if (variant == 0U) f.direct.outcome = ErrorCode::InternalError;
        if (variant == 2U) { f.direct.released = true; f.wifi.startOutcome = ErrorCode::WouldBlock; }
        if (variant == 3U) f.supervisor.cancel();
        check(!f.supervisor.advance() && !f.supervisor.advance() &&
            f.direct.calls == (variant == 3U ? 0U : 1U) && f.wifi.starts == (variant == 2U ? 1U : 0U),
            "unknown, unproved or cancelled handoff resumed or restarted receiver");
        if (f.wifi.guard != nullptr) check(!f.wifi.commitProfile() && f.wifi.commits == 0U,
            "begin failure left WiFi write guard enabled");
    }
}

void testSupervisorLateRootAndDeadline() {
    for (unsigned stage = 0U; stage < 2U; ++stage) {
        for (unsigned variant = 0U; variant < 5U; ++variant) {
            SupervisorFixture f;
            f.install();
            f.direct.released = true;
            if (stage == 1U) check(f.supervisor.advance().ok(), "fixture start before root change failed");
            f.changeRoot(variant);
            if (stage == 0U) check(!f.supervisor.advance() && f.wifi.starts == 0U,
                "wrong durable root or expired authority started receiver");
            if (stage == 1U) check(!f.wifi.commitProfile() && !f.wifi.commitProfile() && f.wifi.commits == 0U,
                "wrong durable root or expired authority passed late write gate");
        }
    }
    SupervisorFixture shortWindow(10U);
    shortWindow.install();
    shortWindow.direct.released = true;
    check(shortWindow.supervisor.advance().ok(), "fixture short explicit window failed to open");
    shortWindow.wire.f.clock.now += 10U;
    check(!shortWindow.supervisor.advance() && !shortWindow.wifi.commitProfile() && shortWindow.wifi.commits == 0U,
        "explicit setup window was extended by handoff");
}

void testDedicatedSetupDeadlineAtOriginalStoreCommit() {
    for (unsigned variant = 0U; variant < 2U; ++variant) {
        WireFixture f(true, security::InitialCloudKeySetupWindow(100U, 10U));
        f.installer.preCommitElapsed = variant == 0U ? 9U : 10U;
        f.install();
        check(f.installer.calls == 1U, "fixture did not enter original guarded store");
        if (variant == 0U) {
            check(f.outcome.installation.ok() && f.outcome.response.ok(), "live setup deadline denied Key install");
            f.checkKey();
        } else {
            check(f.outcome.installation.code() == ErrorCode::StateConflict &&
                f.f.direct.queuedFrameCount() == 0U, "expired setup wrote or queued ACK at late commit gate");
            f.checkNoKey();
        }
    }
}

void testWifiAccessKnownAnswer() {
    security::InitialCloudKeyHandoff handoff{};
    uint8_t secret[32U];
    std::memset(secret, 0x0b, sizeof(secret));
    std::memset(handoff.authority.instance.bytes, 2, sizeof(handoff.authority.instance.bytes));
    std::memset(handoff.sessionBinding, 8, sizeof(handoff.sessionBinding));
    std::memset(handoff.intent, 1, sizeof(handoff.intent));
    const uint8_t locator[] = {0x06,0x3f,0x21,0xc5,0x32,0xbe,0x08,0x68,0x32,0x80,0xea,0xbb,0xf4,0x7e,0xfd,0x90};
    std::memcpy(handoff.locator.bytes, locator, sizeof(locator));
    security::InitialCloudWifiAccess access;
    check(security::deriveInitialCloudWifiAccess(ByteView(secret, sizeof(secret)), handoff, access).ok() &&
        std::strcmp(access.ssid, "BLK-74bc382b85aaf840") == 0 &&
        std::strcmp(access.password, "23ee394de6bad1f536e3d13dafdeba77") == 0 &&
        std::strcmp(access.pop, "59723ae943c16b76dafa58bf0316799f") == 0,
        "cross-language WiFi access vector drifted");
    ++handoff.sessionBinding[0];
    check(security::deriveInitialCloudWifiAccess(ByteView(secret, sizeof(secret)), handoff, access).ok() &&
        std::strcmp(access.ssid, "BLK-74bc382b85aaf840") != 0 &&
        std::strcmp(access.password, "23ee394de6bad1f536e3d13dafdeba77") != 0 &&
        std::strcmp(access.pop, "59723ae943c16b76dafa58bf0316799f") != 0,
        "replacement session inherited access material");
    std::memset(handoff.sessionBinding, 0, sizeof(handoff.sessionBinding));
    check(!security::deriveInitialCloudWifiAccess(ByteView(secret, sizeof(secret)), handoff, access) &&
        access.ssid[0] == 0 && access.password[0] == 0 && access.pop[0] == 0, "invalid scope left old secrets");
    secureZero(MutableByteSpan(secret, sizeof(secret)));
}

void testWifiAccessUsesCurrentAdminAndActualBinding() {
    SupervisorFixture fixture;
    fixture.install();
    fixture.direct.released = true;
    check(fixture.supervisor.advance().ok(), "fixture did not hand off");
    const security::InitialCloudKeyHandoff& handoff = fixture.wifi.received;
    check(constantTimeEqual(ByteView(handoff.sessionBinding, sizeof(handoff.sessionBinding)),
        fixture.wire.f.initiator.secureSession().sessionBinding()), "same-session binding lost during release");
    security::InitialCloudWifiAccessSource source(fixture.wire.f.access);
    security::InitialCloudWifiAccess access;
    check(source.derive(handoff, access).ok() && std::strlen(access.password) == 32U &&
        std::strlen(access.pop) == 32U && std::strcmp(access.password, access.pop) != 0,
        "actual original Admin did not yield separate WPA2 and PoP");
    fixture.changeRoot(1U);
    check(!source.derive(handoff, access) && access.ssid[0] == 0 && access.password[0] == 0 && access.pop[0] == 0,
        "changed Admin produced a usable carrier");
}

// Supervisor-only port substitute. Real Method2/new-purpose admission is
// independently covered by test_initial_cloud_network_recovery_main.cpp.
class NetworkHandoffSource final : public security::IInitialCloudWifiHandoffSource {
public:
    explicit NetworkHandoffSource(security::IInitialCloudWifiHandoffSource& source) : source_(source) {}
    bool takeAuthorizedHandoff(security::InitialCloudKeyHandoff& output) override {
        if (!source_.takeAuthorizedHandoff(output)) return false;
        output.purpose = security::InitialCloudWifiHandoffPurpose::InstalledKeyNetwork;
        std::memset(output.intent, 0x55, sizeof(output.intent));
        return true;
    }
    void retire() override { source_.retire(); }
private:
    security::IInitialCloudWifiHandoffSource& source_;
};

class NetworkBaseline final : public IInitialCloudNetworkBaseline {
public:
    Result validateEmptyNetwork() override {
        ++calls;
        if (cancel != nullptr) cancel->cancel();
        return allowed ? Result::success() : Result::failure(ErrorCode::StateConflict);
    }
    bool allowed = true;
    unsigned calls = 0U;
    PreCloudWifiHandoffSupervisor* cancel = nullptr;
};

void testInstalledNetworkSupervisor() {
    for (unsigned variant = 0U; variant < 5U; ++variant) {
        SupervisorFixture f;
        f.install();
        f.direct.released = true;
        NetworkHandoffSource source(f.wire.endpoint);
        NetworkBaseline baseline;
        PreCloudWifiHandoffSupervisor owner(source, f.direct, f.wifi, f.instances, f.wire.f.access,
            f.wire.f.clock, f.wire.f.clock.now, 600000U, variant == 0U ? nullptr : &baseline);
        if (variant == 1U) baseline.allowed = false;
        if (variant == 2U) baseline.cancel = &owner;
        const Result started = owner.advance();
        if (variant < 3U) {
            check(!started && f.wifi.starts == 0U && f.wifi.commits == 0U,
                "missing/occupied/cancelled baseline started a network receiver");
            continue;
        }
        check(started.ok() && baseline.calls == 1U && f.wifi.starts == 1U,
            "valid installed-Key handoff did not reach the existing receiver");
        if (variant == 3U) baseline.allowed = false; // Slot appears before journal commit.
        const Result written = f.wifi.commitProfile();
        check(written.ok() == (variant == 4U) && baseline.calls == 2U &&
            f.wifi.commits == (variant == 4U ? 1U : 0U), "late baseline did not fence the original sink");
        check(!f.wifi.commitProfile() && baseline.calls == 2U,
            "failed/committed network attempt was retried");
    }
}

void testWifiAccessPurposeSeparation() {
    security::InitialCloudKeyHandoff handoff = {};
    std::memset(handoff.authority.instance.bytes, 1U, sizeof(handoff.authority.instance.bytes));
    std::memset(handoff.sessionBinding, 2U, sizeof(handoff.sessionBinding));
    std::memset(handoff.intent, 3U, sizeof(handoff.intent));
    std::memset(handoff.locator.bytes, 4U, sizeof(handoff.locator.bytes));
    uint8_t secret[kControllerCredentialSecretSize]; std::memset(secret, 5U, sizeof(secret));
    security::InitialCloudWifiAccess install, network;
    check(security::deriveInitialCloudWifiAccess(ByteView(secret, sizeof(secret)), handoff, install).ok(),
        "original purpose access derivation failed");
    handoff.purpose = security::InitialCloudWifiHandoffPurpose::InstalledKeyNetwork;
    check(security::deriveInitialCloudWifiAccess(ByteView(secret, sizeof(secret)), handoff, network).ok() &&
        std::strcmp(install.ssid, network.ssid) != 0 && std::strcmp(install.password, network.password) != 0 &&
        std::strcmp(install.pop, network.pop) != 0, "different purposes reused temporary AP secrets");
    handoff.purpose = static_cast<security::InitialCloudWifiHandoffPurpose>(255U);
    check(!security::deriveInitialCloudWifiAccess(ByteView(secret, sizeof(secret)), handoff, network) &&
        network.pop[0] == 0 && network.password[0] == 0, "unknown purpose retained usable secrets");
    secureZero(MutableByteSpan(secret, sizeof(secret)));
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--network-supervisor") == 0) {
        testInstalledNetworkSupervisor();
        testWifiAccessPurposeSeparation();
        std::cout << "Initial network supervisor: baseline, cancellation, one write and purpose isolation passed" << std::endl;
        return 0;
    }
    if (argc == 2 && std::strcmp(argv[1], "--access") == 0) {
        testWifiAccessKnownAnswer();
        testWifiAccessUsesCurrentAdminAndActualBinding();
        std::cout << "PreCloud WiFi access: 2 finite groups passed" << std::endl;
        return 0;
    }
    if (argc == 2 && std::strcmp(argv[1], "--supervisor") == 0) {
        testSupervisorNeedsExplicitAuthority();
        testSupervisorNativeAndSingleWrite();
        testSupervisorUncertainAndCancelled();
        testSupervisorLateRootAndDeadline();
        testDedicatedSetupDeadlineAtOriginalStoreCommit();
        std::cout << "PreCloud WiFi handoff supervisor: 5 finite groups passed" << std::endl;
        return 0;
    }
    testSecureAckAndOneWrite();
    testPlainAndWrongProofReject();
    testExactRequestShape();
    testReplyCapacityBeforeWrite();
    testFailedInstallNeverAcks();
    testLostBindingAndQueuedDisconnect();
    testInvalidDecryptedScratchCleared();
    testHandoffExactAndSingleUse();
    testHandoffInvalidated();
    testFailedAttemptCannotReopenAfterRekey();
    std::cout << "Initial Cloud Key wire: 10 finite groups passed; host port storage and deterministic AEAD" << std::endl;
    return 0;
}
