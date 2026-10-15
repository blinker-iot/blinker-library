#include <BlinkerV2/arduino/ports/esp32/Esp32NimBle.h>
#include <cstdlib>
#include <iostream>

FakeHostConfig ble_hs_cfg = {};
fake_nimble::State fake_nimble::state;

namespace {
void expect(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << std::endl; std::exit(1); }
}
struct Fixture {
    blinker::Esp32NimBleLink link;
    unsigned connected = 0, disconnected = 0;
    explicit Fixture(const blinker::Esp32NimBleLinkConfig& config =
                         blinker::Esp32NimBleLinkConfig())
        : link(config) {
        fake_nimble::state = fake_nimble::State();
        link.setSessionHandlers(&onConnect, &onDisconnect, this);
        expect(link.start().ok(), "start"); link.poll(0);
    }
    ~Fixture() { link.stop(); }
    static void onConnect(void* context, const blinker::BleSessionInfo&) {
        ++static_cast<Fixture*>(context)->connected;
    }
    static void onDisconnect(void* context, const blinker::BleSessionInfo&) {
        auto& self = *static_cast<Fixture*>(context);
        ++self.disconnected;
        expect(!fake_nimble::state.advertising, "retire authorization before advertise");
        expect(self.link.sessionCount() == 0, "callback sees retired session");
    }
    uint32_t connect() {
        fake_nimble::connect(); fake_nimble::subscribe(); link.poll(0);
        blinker::BleSessionInfo session;
        expect(link.sessionAt(0, session).ok(), "session announced"); return session.sessionId;
    }
    void advertised() {
        expect(link.state() == blinker::BleLinkState::Ready, "peer error must not kill host");
        expect(link.sessionCount() == 0 && fake_nimble::state.advertising, "session retired and advertising");
        expect(fake_nimble::state.initCalls == 1, "no host reboot");
    }
};

void indicationFailure() {
    Fixture f; f.connect();
    fake_nimble::tx(BLE_HS_ETIMEOUT); f.link.poll(0); f.link.poll(0);
    f.advertised(); expect(f.disconnected == 1, "failure closes peer exactly once");
    f.connect(); fake_nimble::disconnect(7); f.link.poll(0); f.advertised();
}
void lateFailure() {
    Fixture f; f.connect();
    fake_nimble::disconnect(7); fake_nimble::tx(BLE_HS_ENOTCONN);
    f.link.poll(0); f.link.poll(0); f.advertised();
    expect(f.disconnected == 1, "late TX failure does not double close");
}
unsigned exitCount = 0;
unsigned disconnectOnExit = 2;
void disconnectDuringTransfer() {
    // poll first consumes host status; its next critical section transfers GAP
    // events. Inject on that unlock, before any Arduino session callback.
    if (++exitCount != disconnectOnExit) return;
    fake_nimble::state.onExit = nullptr;
    fake_nimble::disconnect(7);
}
void connectHandoff() {
    for (disconnectOnExit = 1; disconnectOnExit <= 3; ++disconnectOnExit) {
        Fixture f; fake_nimble::connect();
        exitCount = 0; fake_nimble::state.onExit = &disconnectDuringTransfer;
        f.link.poll(0); f.link.poll(0); f.advertised();
        expect(f.connected == 0 && f.disconnected == 0, "unsubscribed ACL stays private");
        f.connect(); fake_nimble::disconnect(7); f.link.poll(0); f.advertised();
    }
}
void completedAndUnrelated() {
    Fixture f; const uint32_t session = f.connect();
    const uint8_t byte = 1; const blinker::ByteView packet(&byte, 1);
    expect(f.link.sendPacket(session, packet).ok(), "first indication");
    fake_nimble::tx(0);
    expect(f.link.sendPacket(session, packet).code() == blinker::ErrorCode::WouldBlock,
           "sent is not confirmation");
    fake_nimble::tx(BLE_HS_ETIMEOUT, 99);
    fake_nimble::tx(BLE_HS_ETIMEOUT, 7, 99);
    fake_nimble::tx(BLE_HS_ETIMEOUT, 7, 2, false);
    f.link.poll(0);
    expect(fake_nimble::state.terminateCalls == 0, "wrong peer/attribute/kind ignored");
    fake_nimble::tx(BLE_HS_EDONE);
    expect(f.link.sendPacket(session, packet).ok(), "confirmation frees indication slot");
    fake_nimble::disconnect(7); f.link.poll(0); f.advertised();
}
void missingAcl() {
    Fixture f; const uint32_t session = f.connect();
    fake_nimble::state.connected = BLE_HS_CONN_HANDLE_NONE;
    expect(f.link.disconnectSession(session).ok(), "native already gone is reconciled");
    f.link.poll(0); f.advertised();
}
void busyTermination() {
    Fixture f; f.connect(); fake_nimble::tx(BLE_HS_ETIMEOUT);
    fake_nimble::state.terminateResult = BLE_HS_EBUSY;
    f.link.poll(0);
    expect(fake_nimble::state.terminateCalls == 1 && !fake_nimble::state.advertising,
           "one bounded termination attempt, no advertise over live ACL");
    fake_nimble::state.terminateResult = 0;
    f.link.poll(0); f.link.poll(0); f.advertised();
}
void connectThenDisconnect() {
    Fixture f; fake_nimble::connect(); fake_nimble::subscribe(); fake_nimble::disconnect(7);
    f.link.poll(0); f.advertised();
    expect(f.connected == 0 && f.disconnected == 0, "coalesced dead ACL never announced");
}
void asynchronousClose() {
    Fixture f; const uint32_t session = f.connect();
    fake_nimble::state.deferTermination = true;
    fake_nimble::tx(BLE_HS_ETIMEOUT); f.link.poll(0);
    fake_nimble::tx(BLE_HS_EDONE); // Late success cannot reopen a failed stream.
    const uint8_t byte = 1;
    expect(!f.link.sendPacket(session, blinker::ByteView(&byte, 1)), "closing peer rejects send");
    expect(!fake_nimble::state.advertising && f.disconnected == 0, "await physical close");
    fake_nimble::disconnect(7); f.link.poll(0); f.advertised();
    expect(f.disconnected == 1, "async close delivered once");
}
void hostFailureStaysFailure() {
    Fixture f; f.connect(); fake_nimble::state.advertiseResult = BLE_HS_EBUSY;
    fake_nimble::disconnect(7); f.link.poll(0); f.link.poll(0);
    expect(f.link.state() == blinker::BleLinkState::Error, "real advertising failure not swallowed");
}

void actualMtuAndSendErrors() {
    Fixture f;
    expect(fake_nimble::state.preferredMtu == 247U, "best-effort ATT247 preference");
    const uint32_t id = f.connect();
    blinker::BleSessionInfo info;
    expect(f.link.sessionAt(0U, info).ok() && info.maxPacketSize == 20U,
           "preference does not imply negotiated capacity");
    fake_nimble::state.mtu = 83U;
    expect(f.link.sessionAt(0U, info).ok() && info.maxPacketSize == 80U,
           "live actual MTU minus ATT overhead");
    const uint8_t bytes[244U] = {};
    const unsigned sends = fake_nimble::state.sends;
    expect(f.link.sendPacket(id, blinker::ByteView(bytes, 81U)).code() ==
           blinker::ErrorCode::CapacityExceeded && fake_nimble::state.sends == sends,
           "definite oversized failure is checked before native transmit");
    expect(f.link.sendPacket(id, blinker::ByteView(bytes, 80U)).ok(), "actual MTU supports 80-byte send");
    fake_nimble::tx(BLE_HS_EDONE);
    fake_nimble::state.mtu = 517U;
    expect(f.link.sessionAt(0U, info).ok() && info.maxPacketSize == 244U,
           "large native MTU remains bounded by port memory");
    fake_nimble::state.sendResult = BLE_HS_ENOMEM;
    expect(f.link.sendPacket(id, blinker::ByteView(bytes, 244U)).code() ==
           blinker::ErrorCode::WouldBlock, "temporary memory pressure retains the packet");
    fake_nimble::state.sendResult = BLE_HS_EMSGSIZE;
    expect(f.link.sendPacket(id, blinker::ByteView(bytes, 244U)).code() ==
           blinker::ErrorCode::ProtocolError, "uncertain native failure cannot be silently replayed");
    fake_nimble::state.sendResult = BLE_HS_ENOTCONN;
    expect(f.link.sendPacket(id, blinker::ByteView(bytes, 244U)).code() ==
           blinker::ErrorCode::NotConnected, "dead ACL is not perpetual backpressure");
}

void unsupportedMtuPreference() {
    fake_nimble::state = fake_nimble::State();
    fake_nimble::state.preferredMtuResult = BLE_HS_EINVAL;
    blinker::Esp32NimBleLink link;
    expect(link.start().ok(), "smaller SDK ATT preference support does not block start");
    fake_nimble::connect(); fake_nimble::subscribe(); link.poll(0U);
    blinker::BleSessionInfo info;
    expect(link.sessionAt(0U, info).ok() && info.maxPacketSize == 20U,
           "unsupported preference remains physical ATT20");
    link.stop();
}

void actualMtuReceive() {
    Fixture f; f.connect();
    fake_nimble::state.mtu = 247U;
    unsigned received = 0U;
    f.link.setPacketReceiver([](void* context, const blinker::BleSessionInfo& session,
                               blinker::ByteView packet) {
        expect(session.maxPacketSize == 244U && packet.size == 244U,
               "queue delivery carries actual packet limit");
        expect(packet.data[243] == 0x5aU, "bounded queue retains all packet bytes");
        ++*static_cast<unsigned*>(context);
    }, &received);
    os_mbuf input = {};
    input.size = 244U; input.data[243] = 0x5aU;
    ble_gatt_access_ctxt access = {BLE_GATT_ACCESS_OP_WRITE_CHR, &input};
    const ble_gatt_chr_def* characteristic = fake_nimble::state.receive;
    expect(characteristic->access_cb(7U, fake_nimble::state.receiveHandle,
               &access, characteristic->arg) == 0, "negotiated native write enters bounded queue");
    f.link.poll(0U);
    expect(received == 1U, "large RX delivered once");
    fake_nimble::state.mtu = 23U;
    expect(characteristic->access_cb(7U, fake_nimble::state.receiveHandle,
               &access, characteristic->arg) == BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN,
           "unnegotiated native large write is rejected before copy");
}

void receiveQueueBackpressure() {
    expect(BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH == 4U,
           "this regression exercises the unchanged four-packet default");
    blinker::Esp32NimBleLinkConfig config;
    config.maxRxPacketsPerPoll = 1U;
    Fixture f(config);
    struct Capture {
        uint32_t sessionId;
        unsigned received;
    } capture = {f.connect(), 0U};
    f.link.setPacketReceiver([](void* context, const blinker::BleSessionInfo& session,
                               blinker::ByteView packet) {
        auto& observed = *static_cast<Capture*>(context);
        blinker::ble::FragmentView fragment;
        expect(session.sessionId == observed.sessionId &&
                   blinker::ble::parseFragment(packet, fragment).ok(),
               "accepted packets retain their live session and valid fragment");
        expect(fragment.header.frameId == observed.received + 1U &&
                   fragment.header.index == 0U && fragment.payload.size == 1U &&
                   fragment.payload.data[0] == fragment.header.frameId,
               "queue retains accepted packets once and in FIFO order");
        ++observed.received;
    }, &capture);
    os_mbuf input = {};
    input.size = blinker::ble::kFragmentHeaderSize + 1U;
    ble_gatt_access_ctxt access = {BLE_GATT_ACCESS_OP_WRITE_CHR, &input};
    const ble_gatt_chr_def* characteristic = fake_nimble::state.receive;
    const uint16_t handle = fake_nimble::state.connected;
    const auto write = [&](uint8_t id) {
        blinker::ble::FragmentHeader header;
        header.flags = blinker::ble::FragmentFlagStart |
                       blinker::ble::FragmentFlagEnd;
        header.frameId = id;
        expect(blinker::ble::encodeFragmentHeader(header,
                   blinker::MutableByteSpan(input.data, sizeof(input.data))).ok(),
               "test writes a bounded valid fragment");
        input.data[blinker::ble::kFragmentHeaderSize] = id;
        return characteristic->access_cb(handle, fake_nimble::state.receiveHandle,
                                         &access, characteristic->arg);
    };
    for (uint8_t id = 1U; id <= 4U; ++id)
        expect(write(id) == 0, "four queue slots acknowledge actual acceptance");
    expect(capture.received == 0U, "ATT success does not claim poll consumption");
    expect(write(5U) == BLE_ATT_ERR_INSUFFICIENT_RES,
           "full queue rejects the fifth packet explicitly at ATT access");
    f.link.poll(0U);
    expect(capture.received == 1U, "one-packet poll budget releases exactly one slot");
    expect(write(5U) == 0, "a released slot accepts the previously rejected packet");
    for (unsigned index = 0U; index < 4U; ++index) f.link.poll(0U);
    f.link.poll(0U);
    expect(capture.received == 5U && f.connected == 1U && f.disconnected == 0U &&
               fake_nimble::state.terminateCalls == 0U &&
               f.link.state() == blinker::BleLinkState::Ready,
           "bounded backpressure loses no accepted packet and does not reset the peer");
}
}
int main(int argc, char** argv) {
    const char* selected = argc == 2 ? argv[1] : "all";
    expect(argc <= 2 && (!std::strcmp(selected, "all") || !std::strcmp(selected, "indication") ||
                        !std::strcmp(selected, "handoff")), "unknown test selector");
    if (!std::strcmp(selected, "all") || !std::strcmp(selected, "indication")) indicationFailure();
    if (!std::strcmp(selected, "all") || !std::strcmp(selected, "handoff")) connectHandoff();
    if (!std::strcmp(selected, "all")) {
        lateFailure(); completedAndUnrelated(); missingAcl(); busyTermination(); connectThenDisconnect();
        asynchronousClose(); hostFailureStaysFailure();
        actualMtuAndSendErrors(); unsupportedMtuPreference();
        actualMtuReceive(); receiveQueueBackpressure();
    }
    std::cout << "ESP32 native peripheral lifecycle tests passed" << std::endl;
}
