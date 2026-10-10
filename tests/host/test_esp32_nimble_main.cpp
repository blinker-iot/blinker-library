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
    Fixture() {
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
    }
    std::cout << "ESP32 native peripheral lifecycle tests passed" << std::endl;
}
