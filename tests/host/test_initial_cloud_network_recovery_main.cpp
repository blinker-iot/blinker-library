// Reuse the original Method 2/BLE fixture, not its prior test matrix.
#define main initialCloudNetworkAuthorityFixtureMain
#include "test_initial_cloud_key_registration_main.cpp"
#undef main
#include <BlinkerV2/security/InitialCloudKeyRegistrationEndpoint.h>
#include <BlinkerV2/security/InitialCloudNetworkRecoveryEndpoint.h>

namespace {

class NetworkBaseline final : public IInitialCloudNetworkBaseline {
public:
    Result validateEmptyNetwork() override {
        ++calls;
        if (hook != nullptr) hook(context);
        return Result(outcome);
    }
    unsigned calls = 0U;
    ErrorCode outcome = ErrorCode::Ok;
    void (*hook)(void*) = nullptr;
    void* context = nullptr;
};

class NetworkReadOnlyRoot final : public IDeviceAccessChangeObserver {
public:
    Result beginDeviceAccessChange() override { ++writes; return Result::failure(ErrorCode::StateConflict); }
    void endDeviceAccessChange(Result) override {}
    unsigned writes = 0U;
};

DeviceKeyLocator networkLocator(const DeviceKey& key) {
    DeviceKeyLocator locator;
    check(deriveDeviceKeyLocator(key, locator).ok(), "network fixture locator failed");
    return locator;
}

struct NetworkFixture {
    Fixture f;
    NetworkBaseline baseline;
    NetworkReadOnlyRoot readOnly;
    DeviceKeyLocator locator;
    security::InitialCloudNetworkRecoveryEndpoint endpoint;
    Result outcome;
    uint8_t before[DeviceAccessStore::serializedSize];
    uint8_t notification[96U];
    size_t notificationSize = 0U;
    uint8_t nextFragment = 0U;
    uint8_t nextRecord = 2U;
    bool complete = false;
    uint16_t maximum = 512U;

    explicit NetworkFixture(bool authenticate = true, bool legacyBinding = false,
        security::InitialCloudKeySetupWindow setup = security::InitialCloudKeySetupWindow())
        : f(100U, legacyBinding, true, setup), baseline(), readOnly(), locator(networkLocator(f.key)),
          endpoint(f.registration, f.access, baseline, f.direct, ByteView(f.intent, sizeof(f.intent)), locator),
          outcome(), before(), notification() {
        // This is the pre-existing durable Key, not an install performed by
        // the network owner. Subsequent writes are rejected and counted.
        // The shared fixture starts BLE-only. Seed a distinct combined-root
        // fixture: plain replace must not upgrade an enrolled BLE-only root.
        check(f.access.eraseAccess().ok() && f.access.bootstrap(f.key, 7U, f.used).ok(),
            "network fixture existing Key failed");
        std::memcpy(before, f.blob.bytes, sizeof(before));
        f.access.setChangeObserver(&readOnly);
        if (authenticate) {
            check(f.authenticate(), "network fixture Method 2 failed");
            drain();
        }
        f.direct.setReceiver(&NetworkFixture::receive, this);
        f.link.observeSent = &NetworkFixture::packet;
        f.link.observeSentContext = this;
    }
    ~NetworkFixture() { f.access.setChangeObserver(nullptr); }

    bool authenticate(bool queued = true) {
        f.direct.setReceiver(&Fixture::receive, &f);
        const bool result = f.authenticate(queued);
        f.direct.setReceiver(&NetworkFixture::receive, this);
        return result;
    }

    static void receive(void* context, ByteView frame, const RxContext& rx) {
        NetworkFixture& self = *static_cast<NetworkFixture*>(context);
        self.outcome = self.endpoint.receive(frame, rx, self.maximum);
    }
    static void packet(void* context, ByteView encoded) {
        NetworkFixture& self = *static_cast<NetworkFixture*>(context);
        ble::FragmentView fragment;
        check(ble::parseFragment(encoded, fragment).ok(), "network acceptance fragment failed");
        check(!self.complete && fragment.header.index == self.nextFragment++, "network acceptance order failed");
        check(self.notificationSize + fragment.payload.size <= sizeof(self.notification), "network acceptance overflow");
        std::memcpy(self.notification + self.notificationSize, fragment.payload.data, fragment.payload.size);
        self.notificationSize += fragment.payload.size;
        self.complete = (fragment.header.flags & ble::FragmentFlagEnd) != 0U;
    }
    void drain() {
        for (unsigned i = 0U; i < 16U && f.direct.queuedFrameCount() != 0U; ++i) f.direct.poll(1000U);
        check(f.direct.queuedFrameCount() == 0U, "network fixture queue did not drain");
    }
    void frame(uint8_t (&bytes)[bbp2::kInitialCloudNetworkFrameSize]) const {
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudNetworkHandoff);
        header.sequence = 0x2345U;
        header.bodyLength = static_cast<uint16_t>(bbp2::kInitialCloudNetworkBodySize);
        check(bbp2::encodeHeader(header, MutableByteSpan(bytes, sizeof(bytes))).ok() &&
            bbp2::encodeInitialCloudNetworkRecoveryBody(ByteView(f.intent, sizeof(f.intent)), locator,
                MutableByteSpan(bytes + bbp2::kBaseHeaderSize, bbp2::kInitialCloudNetworkBodySize)).ok(),
            "network request encode failed");
    }
    void inject(ByteView frame) {
        uint8_t record[96U] = {};
        ByteView sealed;
        check(f.initiator.secureSession().seal(f.crypto, frame, MutableByteSpan(record, sizeof(record)), sealed).ok(),
            "network request seal failed");
        f.link.injectRecord(sealed, nextRecord++);
    }
    void request() {
        uint8_t encoded[bbp2::kInitialCloudNetworkFrameSize] = {};
        frame(encoded);
        inject(ByteView(encoded, sizeof(encoded)));
    }
    void unchanged() {
        check(f.blob.present && f.blob.size == sizeof(before) && std::memcmp(before, f.blob.bytes, sizeof(before)) == 0 &&
            readOnly.writes == 0U && f.installer.calls == 0U, "network purpose changed the access root or invoked an installer");
    }
    void noHandoff() {
        security::InitialCloudKeyHandoff value = {};
        check(!endpoint.takeAuthorizedHandoff(value), "rejected network attempt produced a handoff");
    }
};

void testNetworkWireAndOriginalBinding() {
    NetworkFixture w;
    w.request();
    check(w.outcome.ok() && w.f.direct.queuedFrameCount() == 1U && w.baseline.calls == 1U,
        "network owner did not accept exactly once");
    w.drain();
    uint8_t plain[bbp2::kInitialCloudNetworkFrameSize] = {};
    ByteView decoded;
    check(w.complete && w.f.initiator.secureSession().open(w.f.crypto,
        ByteView(w.notification, w.notificationSize), MutableByteSpan(plain, sizeof(plain)), decoded).ok(),
        "network acceptance was not protected by the original committed session");
    bbp2::FrameView frame;
    bbp2::InitialCloudNetworkRecoveryBody body;
    check(bbp2::parseFrame(decoded, frame).ok() && frame.header.sequence == 0x2345U &&
        frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::InitialCloudNetworkAccepted) &&
        frame.header.flags == bbp2::FlagIsResponse && bbp2::decodeInitialCloudNetworkRecoveryBody(frame.body, body).ok() &&
        constantTimeEqual(body.operation, ByteView(w.f.intent, sizeof(w.f.intent))) &&
        constantTimeEqual(body.locator, ByteView(w.locator.bytes, sizeof(w.locator.bytes))), "network acceptance coordinates changed");
    security::InitialCloudKeyHandoff handoff = {};
    check(w.endpoint.takeAuthorizedHandoff(handoff) &&
        handoff.purpose == security::InitialCloudWifiHandoffPurpose::InstalledKeyNetwork &&
        handoff.authority.epoch == 7U && handoff.authority.credentialVersion == 1U &&
        sameDeviceInstanceId(handoff.authority.instance, w.f.deviceInstance) &&
        constantTimeEqual(ByteView(handoff.sessionBinding, sizeof(handoff.sessionBinding)),
            w.f.initiator.secureSession().sessionBinding()), "network handoff fabricated or lost its authority");
    w.noHandoff();
    w.unchanged();
}

void testNetworkCodecAndPurposeSeparation() {
    for (unsigned variant = 0U; variant < 11U; ++variant) {
        NetworkFixture w;
        uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {};
        w.frame(frame);
        switch (variant) {
        case 0U: frame[3] = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyInstall); break;
        case 1U: frame[3] = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyHandoff); break;
        case 2U: frame[3] = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudNetworkAccepted); break;
        case 3U: frame[4] = bbp2::FlagAckRequired; break;
        case 4U: frame[6] = 0U; frame[7] = 0U; break;
        case 5U: frame[10] = 0x82U; break;
        case 6U: frame[11] = 2U; break;
        case 7U: std::memset(frame + 13U, 0, 16U); break;
        case 8U: std::memset(frame + 30U, 0, 16U); break;
        case 9U: frame[13] ^= 1U; break;
        default: frame[30] ^= 1U; break;
        }
        w.inject(ByteView(frame, sizeof(frame)));
        check(!w.outcome && w.f.direct.queuedFrameCount() == 0U && w.baseline.calls == 0U,
            "malformed or foreign purpose obtained network authority");
        w.noHandoff();
        w.unchanged();
    }
    NetworkFixture w;
    security::InitialCloudKeyRegistrationEndpoint old(w.f.registration, w.f.installer, w.f.direct);
    uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {};
    w.frame(frame);
    check(!old.receive(ByteView(frame, sizeof(frame)), w.f.verifiedRx, 512U) && w.f.installer.calls == 0U,
        "old install endpoint accepted the network-only purpose");
    bbp2::InitialCloudNetworkRecoveryBody body;
    check(!bbp2::decodeInitialCloudNetworkRecoveryBody(ByteView(frame + 10U, 35U), body), "short network body was accepted");
    w.maximum = bbp2::kInitialCloudNetworkFrameSize - 1U;
    w.request();
    check(w.outcome.code() == ErrorCode::BufferTooSmall && w.baseline.calls == 0U, "network purpose ignored the negotiated budget");
    w.unchanged();
    NetworkFixture copied;
    copied.frame(frame);
    copied.f.intent[0] ^= 1U;
    copied.locator.bytes[0] ^= 1U;
    copied.inject(ByteView(frame, sizeof(frame)));
    check(copied.outcome.ok(), "network owner retained borrowed operation or locator input");
    copied.unchanged();
    NetworkFixture invalid;
    uint8_t zero[bbp2::kInitialCloudNetworkOperationSize] = {};
    security::InitialCloudNetworkRecoveryEndpoint unconfigured(invalid.f.registration, invalid.f.access,
        invalid.baseline, invalid.f.direct, ByteView(zero, sizeof(zero)), invalid.locator);
    invalid.frame(frame);
    check(unconfigured.receive(ByteView(frame, sizeof(frame)), invalid.f.verifiedRx, 512U).code() == ErrorCode::NotConfigured &&
        invalid.baseline.calls == 0U, "zero expected operation opened a network owner");
    invalid.unchanged();
}

void testNetworkRequiresCommittedMethod2() {
    for (unsigned variant = 0U; variant < 4U; ++variant) {
        NetworkFixture w(false, variant == 3U);
        if (variant == 1U) check(w.authenticate(false), "prepared-only fixture failed");
        if (variant >= 2U) check(w.authenticate(true), "uncommitted observation fixture failed");
        if (variant == 2U) w.f.direct.cancel(w.f.session.sessionId);
        uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {};
        w.frame(frame);
        RxContext forged;
        forged.encrypted = true; forged.authenticated = true; forged.bonded = true;
        forged.sessionId = w.f.session.sessionId;
        check(!w.endpoint.receive(ByteView(frame, sizeof(frame)), forged, 512U) && w.baseline.calls == 0U,
            "flags, prepare or unsupported committed binding minted a network handoff");
        w.noHandoff();
        w.unchanged();
    }
}

void testNetworkRejectsRolesAndChangedRoot() {
    for (unsigned role = 0U; role < 2U; ++role) {
        NetworkFixture w(false);
        w.f.used = credential(0x40U, role == 0U ? kControllerPermissionObserve | kControllerPermissionControl : kControllerPermissionAll);
        if (role == 1U) { w.f.used.domain = ControllerCredentialDomain::Local; w.f.used.ownershipGeneration = 0U; }
        bool changed = false;
        check(w.f.source.additional.install(w.f.used, changed).ok() && changed, "network role fixture failed");
        check(w.authenticate(), "network role authentication failed");
        uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {};
        w.frame(frame);
        check(!w.endpoint.receive(ByteView(frame, sizeof(frame)), w.f.verifiedRx, 512U) && w.baseline.calls == 0U,
            "non-Admin Method 2 role obtained network authority");
        w.noHandoff();
        w.unchanged();
    }
    for (unsigned change = 0U; change < 4U; ++change) {
        NetworkFixture w;
        w.f.access.setChangeObserver(nullptr);
        if (change == 0U) check(w.f.access.clear().ok(), "network missing-Key fixture failed");
        else if (change == 1U) {
            const DeviceKey other = candidateKey(0x70U);
            check(w.f.access.replace(other).ok(), "network locator-change fixture failed");
        } else if (change == 2U) {
            ControllerCredential other = w.f.used; ++other.credentialVersion;
            check(w.f.access.rotateVerified(w.f.used.credentialVersion, other).ok(), "network Admin-change fixture failed");
            clearControllerCredential(other);
        } else w.f.blob.bytes[0] ^= 1U;
        w.request();
        check(!w.outcome && w.f.direct.queuedFrameCount() == 0U, "network purpose accepted a missing, changed or corrupt root");
        w.noHandoff();
        check(w.f.installer.calls == 0U, "root rejection invoked a Key installer");
    }
}

void testNetworkSessionAndWindowFences() {
    for (unsigned variant = 0U; variant < 7U; ++variant) {
        NetworkFixture w(true, false, security::InitialCloudKeySetupWindow(100U, 1000U));
        RxContext rx = w.f.verifiedRx;
        if (variant == 0U) ++rx.transportId;
        else if (variant == 1U) ++rx.sessionId;
        else if (variant == 2U) rx.encrypted = false;
        else if (variant == 3U) w.f.clock.now += security::kInitialCloudKeyRegistrationWindowMillis;
        else if (variant == 4U) w.f.clock.now += 1000U;
        else if (variant == 5U) w.f.link.session.connected = false; // No forwarded native callback yet.
        else w.f.registration.resetSessions();
        uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {};
        w.frame(frame);
        check(!w.endpoint.receive(ByteView(frame, sizeof(frame)), rx, 512U) && w.baseline.calls == 0U,
            "wrong or expired network session reached the baseline guard");
        w.noHandoff();
        w.unchanged();
    }
}

struct NetworkHook {
    NetworkFixture& wire;
    unsigned action;
    Result nested;
};
void interfereNetwork(void* context) {
    NetworkHook& hook = *static_cast<NetworkHook*>(context);
    NetworkFixture& w = hook.wire;
    if (hook.action == 0U) {
        uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {};
        w.frame(frame);
        hook.nested = w.endpoint.receive(ByteView(frame, sizeof(frame)), w.f.verifiedRx, 512U);
        w.noHandoff();
    } else if (hook.action == 1U) w.endpoint.retire();
    else if (hook.action == 2U) w.f.registration.resetSessions();
    else {
        w.f.access.setChangeObserver(nullptr);
        check(w.f.access.replace(candidateKey(0x70U)).ok(), "network callback root mutation failed");
    }
}

void testNetworkConsumesBeforeCallbacks() {
    for (unsigned action = 0U; action < 4U; ++action) {
        NetworkFixture w;
        NetworkHook hook = {w, action, Result::success()};
        w.baseline.hook = &interfereNetwork; w.baseline.context = &hook;
        w.request();
        if (action == 0U) {
            check(w.outcome.ok() && hook.nested.code() == ErrorCode::StateConflict && w.baseline.calls == 1U,
                "network callback reentry duplicated authority");
        } else {
            check(!w.outcome && w.f.direct.queuedFrameCount() == 0U, "network callback cancellation/root change still queued acceptance");
            w.noHandoff();
        }
        if (action != 3U) w.unchanged();
    }
    NetworkFixture w;
    w.baseline.outcome = ErrorCode::WouldBlock;
    w.request();
    check(!w.outcome && w.baseline.calls == 1U && w.f.direct.queuedFrameCount() == 0U, "unknown baseline accepted network work");
    uint8_t proof[32U] = {}; w.f.proof(proof);
    check(!w.f.install(ByteView(proof, sizeof(proof))) && w.f.installer.calls == 0U,
        "network failure restored unconsumed installation authority");
    w.baseline.outcome = ErrorCode::Ok;
    w.f.registration.resetSessions();
    w.f.direct.setReceiver(&Fixture::receive, &w.f);
    check(w.f.authenticate(), "replacement network Method 2 fixture failed");
    uint8_t frame[bbp2::kInitialCloudNetworkFrameSize] = {}; w.frame(frame);
    check(w.endpoint.receive(ByteView(frame, sizeof(frame)), w.f.verifiedRx, 512U).code() == ErrorCode::StateConflict &&
        w.baseline.calls == 1U, "new handshake revived a terminal network owner");
    w.noHandoff();
    w.unchanged();
}

void testNetworkQueueUnknownAndExplicitRetirement() {
    NetworkFixture full;
    uint8_t ordinary[bbp2::kBaseHeaderSize] = {};
    bbp2::FrameHeader header; header.kind = static_cast<uint8_t>(bbp2::MessageKind::StateRequest);
    check(bbp2::encodeHeader(header, MutableByteSpan(ordinary, sizeof(ordinary))).ok(), "network queue fixture frame failed");
    SendTarget target; target.kind = SendTargetKind::Session; target.sessionId = full.f.session.sessionId;
    while (full.f.direct.queuedFrameCount() < BLINKER_BLE_MAX_TX_FRAMES)
        check(full.f.direct.send(ByteView(ordinary, sizeof(ordinary)), target).ok(), "network queue fixture fill failed");
    full.request();
    check(!full.outcome && full.baseline.calls == 1U, "full response queue accepted a network handoff");
    full.noHandoff();
    full.unchanged();
    for (unsigned cancel = 0U; cancel < 2U; ++cancel) {
        NetworkFixture w;
        w.request();
        check(w.outcome.ok() && w.f.direct.queuedFrameCount() == 1U, "network UNKNOWN fixture did not enqueue");
        w.f.link.disconnect(); // The acceptance can be lost before physical transmission.
        check(w.f.direct.queuedFrameCount() == 0U, "lost network acceptance remained queued for a new peer");
        if (cancel != 0U) w.endpoint.retire();
        security::InitialCloudKeyHandoff handoff = {};
        check(w.endpoint.takeAuthorizedHandoff(handoff) == (cancel == 0U),
            "peer closure was an owner cancel, or explicit cancel preserved authority");
        w.noHandoff();
        w.unchanged();
    }
}

} // namespace

int main() {
    testNetworkWireAndOriginalBinding();
    testNetworkCodecAndPurposeSeparation();
    testNetworkRequiresCommittedMethod2();
    testNetworkRejectsRolesAndChangedRoot();
    testNetworkSessionAndWindowFences();
    testNetworkConsumesBeforeCallbacks();
    testNetworkQueueUnknownAndExplicitRetirement();
    std::cout << "Installed-Key network recovery: 7 finite host groups passed; no radio or durability claim" << std::endl;
    return 0;
}
