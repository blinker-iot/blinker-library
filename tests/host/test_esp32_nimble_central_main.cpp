#include <BlinkerV2/arduino/ports/esp32/Esp32NimBleCentralPort.h>
#include <cstdlib>
#include <iostream>

FakeHostConfig ble_hs_cfg = {};
fake_nimble::State fake_nimble::state;

namespace {
void expect(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << std::endl; std::exit(1); }
}
void resetFake() {
    fake_nimble::state = fake_nimble::State();
    fake_nimble::resetGatt();
}
blinker::BleCentralConnectRequest request(uint32_t attempt = 1U) {
    blinker::BleCentralConnectRequest result;
    result.attemptId = attempt;
    result.matcher = [](void*, const blinker::BleCentralAdvertisement&) { return true; };
    return result;
}
void offer(blinker::Esp32NimBleCentralPort& port) {
    uint8_t bytes[16U + blinker::ble::kModeServiceDataSize] = {};
    std::memcpy(bytes, blinker::ble::kServiceUuidLittleEndian, 16U);
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_DISC;
    event.disc.data = bytes; event.disc.length_data = sizeof(bytes);
    fake_nimble::emit(event); port.poll(0);
}
void finishDiscovery(blinker::Esp32NimBleCentralPort& port, uint16_t handle, bool publish = true) {
    const ble_gatt_error ok = {0}, done = {BLE_HS_EDONE};
    auto service = fake_nimble::gatt(handle).service;
    void* context = fake_nimble::gatt(handle).context;
    expect(service != nullptr, "service discovery registered");
    const ble_gatt_svc svc = {1, 10};
    service(handle, &ok, &svc, context); service(handle, &done, nullptr, context);
    auto characteristic = fake_nimble::gatt(handle).characteristic;
    expect(characteristic != nullptr, "RX discovery registered");
    const ble_gatt_chr rx = {BLE_GATT_CHR_PROP_WRITE, 3};
    characteristic(handle, &ok, &rx, context); characteristic(handle, &done, nullptr, context);
    characteristic = fake_nimble::gatt(handle).characteristic;
    const ble_gatt_chr tx = {BLE_GATT_CHR_PROP_INDICATE, 6};
    characteristic(handle, &ok, &tx, context); characteristic(handle, &done, nullptr, context);
    auto descriptor = fake_nimble::gatt(handle).descriptor;
    expect(descriptor != nullptr, "CCCD discovery registered");
    ble_gatt_dsc cccd = {}; cccd.handle = 7;
    descriptor(handle, &ok, 6, &cccd, context); descriptor(handle, &done, 6, nullptr, context);
    expect(fake_nimble::gatt(handle).write != nullptr, "CCCD write registered");
    fake_nimble::gatt(handle).write(handle, &ok, nullptr, context);
    if (publish) {
        port.poll(0);
        expect(port.state() == blinker::BleCentralPortState::Ready, "GATT Ready");
    }
}
void ready(blinker::Esp32NimBleCentralPort& port, uint16_t handle = 7) {
    offer(port); fake_nimble::connect(handle); port.poll(0); finishDiscovery(port, handle);
}
struct Fixture {
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort port;
    unsigned disconnected = 0;
    Fixture() : port(host) {
        resetFake();
        port.setConnectionHandlers(nullptr, &onDisconnect, this);
        expect(port.start().ok(), "start");
        blinker::BleCentralConnectRequest request;
        request.attemptId = 1;
        request.matcher = [](void*, const blinker::BleCentralAdvertisement&) { return true; };
        expect(port.connect(request).ok(), "scan");
    }
    ~Fixture() {
        fake_nimble::state.deferTermination = false;
        fake_nimble::state.deferCancel = false;
        fake_nimble::state.cancelResult = 0;
        fake_nimble::state.terminateResult = 0;
        fake_nimble::state.onDelay = nullptr;
    }
    static void onDisconnect(void* context, const blinker::BleCentralConnectionInfo& info) {
        expect(info.attemptId == 1, "exact attempt");
        ++static_cast<Fixture*>(context)->disconnected;
    }
    void offer() {
        ::offer(port);
    }
    void beginConnect() {
        offer();
        expect(port.state() == blinker::BleCentralPortState::Connecting, "native connecting");
    }
    void idle() {
        expect(port.state() == blinker::BleCentralPortState::Idle, "physical idle");
        expect(disconnected == 1, "exactly one completion");
        expect(fake_nimble::connectionCount() == 0, "no orphan ACL");
    }
};
void cancelledScan() {
    Fixture f;
    fake_nimble::state.cancelResult = BLE_HS_EBUSY;
    expect(!f.port.cancel(1), "scan cancel rejected"); f.port.poll(0);
    expect(f.disconnected == 0, "no synthetic completion after rejected cancel");
    fake_nimble::state.cancelResult = 0;
    expect(f.port.cancel(1).ok(), "scan cancel accepted"); f.port.poll(0); f.idle();
}
void cancelledConnect() {
    Fixture f; f.beginConnect();
    expect(f.port.cancel(1).ok(), "cancel");
    expect(f.disconnected == 0, "completion deferred to poll");
    f.port.poll(0); f.idle();
}
void scanHandoffRejected() {
    Fixture f;
    fake_nimble::state.cancelResult = BLE_HS_EBUSY;
    f.offer();
    expect(f.port.state() == blinker::BleCentralPortState::Scanning, "live scan keeps slot");
    expect(f.disconnected == 0 && fake_nimble::state.scanning, "failed handoff is not release");
    fake_nimble::state.cancelResult = 0;
    f.beginConnect();
    expect(f.port.cancel(1).ok(), "same attempt cancellable after retry");
    f.port.poll(0); f.idle();
}
void successRacesCancellation(bool alreadyComplete) {
    Fixture f; f.beginConnect();
    fake_nimble::state.deferCancel = true;
    fake_nimble::state.deferTermination = true;
    if (alreadyComplete) fake_nimble::connect();
    expect(f.port.cancel(1).ok(), "cancel accepted or already completed");
    if (!alreadyComplete) fake_nimble::connect();
    f.port.poll(0);
    expect(f.port.state() == blinker::BleCentralPortState::Disconnecting, "raced ACL still owns slot");
    expect(fake_nimble::state.terminateCalls == 1 && f.disconnected == 0, "terminate before release");
    f.port.poll(0);
    expect(f.disconnected == 0, "delay retains slot");
    fake_nimble::disconnect(7); f.port.poll(0); f.idle();
}
void failedDiscoveryWithRejectedTerminate() {
    Fixture f; f.beginConnect();
    fake_nimble::state.discoveryResult = BLE_HS_EBUSY;
    fake_nimble::state.terminateResult = BLE_HS_EBUSY;
    fake_nimble::connect(); f.port.poll(0); f.port.poll(0);
    expect(f.port.state() == blinker::BleCentralPortState::Disconnecting, "failed terminate retains ACL");
    expect(f.disconnected == 0, "error is not release");
    fake_nimble::state.terminateResult = 0;
    fake_nimble::state.deferTermination = true;
    f.port.poll(0);
    const unsigned accepted = fake_nimble::state.terminateCalls;
    expect(accepted >= 2, "rejected termination retried");
    f.port.poll(0);
    expect(fake_nimble::state.terminateCalls == accepted, "accepted terminate not repeated");
    // Native table is authoritative even if its disconnect callback is lost.
    fake_nimble::native(7)->live = false;
    f.port.poll(0); f.idle();
}
void stopWaitsForConnect() {
    Fixture f; f.beginConnect();
    fake_nimble::state.deferCancel = true;
    fake_nimble::state.onDelay = [] {
        fake_nimble::state.onDelay = nullptr;
        fake_nimble::connect();
    };
    f.port.stop();
    expect(fake_nimble::state.terminateCalls == 1, "stop drains successful late CONNECT");
    expect(fake_nimble::connectionCount() == 0, "stop leaves no ACL");
    expect(f.port.state() == blinker::BleCentralPortState::Stopped, "stopped only after native idle");
    expect(f.port.start().ok(), "same host reusable");
    expect(fake_nimble::state.initCalls == 1, "no SDK host churn");
}
void stuckConnectDoesNotPretendStopped() {
    Fixture f; f.beginConnect(); fake_nimble::state.deferCancel = true;
    f.port.stop();
    expect(f.port.state() == blinker::BleCentralPortState::Error, "stop timeout holds capacity");
    expect(!f.port.start(), "cannot reuse stuck native connect");
}
void nativeConnectedBeforeCallback(bool lost) {
    Fixture f; f.beginConnect();
    // SDK removed the initiating procedure and allocated an ACL, but its
    // CONNECT callback has not yet reached the Port (including on another CPU).
    fake_nimble::state.connecting = false;
    fake_nimble::allocateConnection(7);
    if (!lost) fake_nimble::state.onDelay = [] {
        fake_nimble::state.onDelay = nullptr; fake_nimble::connect();
    };
    f.port.stop();
    if (lost) {
        expect(f.port.state() == blinker::BleCentralPortState::Error, "missing CONNECT callback cannot release an unadopted ACL");
        fake_nimble::connect(); f.port.stop();
    }
    expect(fake_nimble::state.terminateCalls == 1 && fake_nimble::connectionCount() == 0, "late CONNECT adopted and closed before release");
    expect(f.port.state() == blinker::BleCentralPortState::Stopped, "stop waits for native completion, not procedure absence");
}
void sharedHostSeparatesInitiation() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host), overflow(host);
    expect(a.start().ok() && b.start().ok(), "two registered borrowers");
    expect(overflow.start().code() == blinker::ErrorCode::CapacityExceeded, "registry bounded");
    overflow.stop();
    expect(fake_nimble::state.initCalls == 1, "one SDK Host");
    expect(host.end().code() == blinker::ErrorCode::WouldBlock, "live borrower blocks SDK shutdown");
    expect(a.connect(request()).ok(), "A admitted");
    expect(b.connect(request(2)).code() == blinker::ErrorCode::WouldBlock, "B cannot steal scan");
    const unsigned cancels = fake_nimble::state.scanCancelCalls;
    b.stop();
    expect(fake_nimble::state.scanning && fake_nimble::state.scanCancelCalls == cancels, "B stop leaves A scan");
    expect(b.start().ok(), "B reattaches without Host restart");
    ready(a);
    expect(b.connect(request(2)).ok(), "Ready A yields initiator, not its ACL");
    b.stop();
    expect(fake_nimble::alive(7) && fake_nimble::state.terminateCalls == 0, "B stop leaves A ACL");
    a.stop(); expect(b.start().ok() && b.connect(request(2)).ok(), "capacity reusable after A closes");
    b.stop();
    expect(fake_nimble::state.initCalls == 1 && fake_nimble::state.stopCalls == 0, "Port stop never churns SDK");
    expect(host.end().ok() && fake_nimble::state.stopCalls == 1 && fake_nimble::state.deinitCalls == 1, "owner shuts down once");
}
void staleCallbacksCannotReachReusedHandle() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    fake_nimble::GattCallbacks old;
    int (*oldGap)(ble_gap_event*, void*) = nullptr;
    void* oldContext = nullptr;
    const uint8_t bytes[] = {0x31, 0x32};
    const blinker::ByteView packet(bytes, sizeof(bytes));
    const ble_gatt_error ok = {0};
    {
        blinker::Esp32NimBleCentralPort a(host);
        expect(a.start().ok() && a.connect(request()).ok(), "old owner starts"); ready(a);
        old = fake_nimble::gatt(); oldGap = fake_nimble::state.gap;
        oldContext = fake_nimble::state.context;
        expect(a.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock, "old write pending");
        old.write = fake_nimble::gatt().write;
        a.stop();
    } // SDK still alive; old Port memory no longer exists.
    blinker::Esp32NimBleCentralPort b(host);
    expect(b.start().ok() && b.connect(request()).ok(), "new owner reuses logical attempt 1"); ready(b);
    expect(fake_nimble::state.context != oldContext, "opaque token not reused with native handle 7");
    expect(b.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock, "new write pending");
    old.write(7, &ok, nullptr, oldContext);
    old.service(7, &ok, nullptr, oldContext);
    old.characteristic(7, &ok, nullptr, oldContext);
    old.descriptor(7, &ok, 6, nullptr, oldContext);
    ble_gap_event stale = {}; stale.type = BLE_GAP_EVENT_DISCONNECT;
    stale.disconnect.conn.conn_handle = 7; oldGap(&stale, oldContext);
    stale.type = BLE_GAP_EVENT_CONNECT; stale.connect.conn_handle = 7; oldGap(&stale, oldContext);
    unsigned packets = 0;
    b.setPacketReceiver([](void* p, const blinker::BleCentralConnectionInfo&, blinker::ByteView) {
        ++*static_cast<unsigned*>(p);
    }, &packets);
    os_mbuf buffer = {}; buffer.size = 1; buffer.data[0] = 0x42;
    stale.type = BLE_GAP_EVENT_NOTIFY_RX; stale.notify_rx.conn_handle = 7;
    stale.notify_rx.attr_handle = 6; stale.notify_rx.indication = 1; stale.notify_rx.om = &buffer;
    oldGap(&stale, oldContext); b.poll(0);
    expect(b.state() == blinker::BleCentralPortState::Ready && fake_nimble::state.terminateCalls == 1, "stale GAP never touches new ACL");
    expect(packets == 0 && b.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock, "old data and ACK rejected");
    fake_nimble::gatt().write(7, &ok, nullptr, fake_nimble::state.context);
    expect(b.sendPacket(1, packet).ok(), "only current ACK completes write");
    fake_nimble::emit(stale); b.poll(0); expect(packets == 1, "current indication accepted");
    b.stop(); expect(host.end().ok(), "SDK owner closes");
    expect(b.start().ok() && b.connect(request()).ok(), "explicit SDK restart");
    blinker::Esp32NimBleCentralHost::Callback obsolete(oldContext);
    expect(obsolete.owner() == nullptr, "SDK restart also rejects old token");
    b.stop();
}
void callbackBorrowBlocksReuse() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPortConfig config; config.closeTimeoutMillis = 5;
    blinker::Esp32NimBleCentralPort a(host, config), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "callback fixture");
    {
        blinker::Esp32NimBleCentralHost::Callback callback(fake_nimble::state.context);
        expect(callback.owner() == &a, "in-flight owner borrowed");
        a.stop();
        expect(a.state() == blinker::BleCentralPortState::Error && a.lastError() == blinker::ErrorCode::WouldBlock, "timeout not false stop");
        expect(b.connect(request(2)).code() == blinker::ErrorCode::WouldBlock, "in-flight callback retains admission");
        blinker::Esp32NimBleCentralHost::Callback late(fake_nimble::state.context);
        expect(late.owner() == nullptr, "closing token rejects additional callbacks");
    }
    a.stop(); expect(a.state() == blinker::BleCentralPortState::Stopped, "stop retries after callback returns");
    expect(b.connect(request(2)).ok(), "next owner admitted after drain"); b.stop();
}
void callbackBorrowDefersCompletion(bool reset) {
    Fixture f;
    {
        blinker::Esp32NimBleCentralHost::Callback callback(fake_nimble::state.context);
        expect(f.port.cancel(1).ok(), "cancel while callback borrowed"); f.port.poll(0);
        expect(f.disconnected == 0 && f.port.state() == blinker::BleCentralPortState::Disconnecting, "logical completion waits for callback drain");
        if (reset) { ble_hs_cfg.reset_cb(42); ble_hs_cfg.sync_cb(); f.port.poll(0); }
    }
    f.port.poll(0);
    if (reset) {
        expect(f.port.state() == blinker::BleCentralPortState::Error && f.disconnected == 0, "deferred completion cannot hide SDK reset");
        f.port.stop();
    } else f.idle();
}
void resetIsLatchedAcrossSync() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "reset fixture");
    fake_nimble::state.scanning = false;
    ble_hs_cfg.reset_cb(42); ble_hs_cfg.sync_cb(); a.poll(0); b.poll(0);
    expect(a.state() == blinker::BleCentralPortState::Error && b.state() == blinker::BleCentralPortState::Error, "resync cannot hide per-Port reset");
    a.stop(); b.stop();
    expect(a.start().ok() && b.start().ok() && b.connect(request(2)).ok(), "borrowers explicitly recover after reset");
    expect(fake_nimble::state.initCalls == 1, "reset recovery does not reinitialize SDK");
}
void hostFailuresRetainExactOwnership() {
    resetFake();
    blinker::Esp32NimBleCentralHost host(5);
    blinker::Esp32NimBleCentralPort port(host);
    int foreign = 0;
    expect(blinker::esp32_nimble_detail::claim(&foreign), "foreign role claims SDK");
    expect(port.start().code() == blinker::ErrorCode::AlreadyExists && fake_nimble::state.initCalls == 0, "central cannot steal peripheral role");
    port.stop(); blinker::esp32_nimble_detail::release(&foreign);
    fake_nimble::state.initResult = ESP_ERR_NO_MEM;
    expect(port.start().code() == blinker::ErrorCode::CapacityExceeded, "SDK allocation failure"); port.stop();
    expect(blinker::esp32_nimble_detail::claim(&foreign), "failed init releases claim");
    blinker::esp32_nimble_detail::release(&foreign);
    fake_nimble::state.initResult = 0; fake_nimble::state.autoSync = false;
    expect(port.start().code() == blinker::ErrorCode::NotConnected, "bounded sync timeout"); port.stop();
    expect(!blinker::esp32_nimble_detail::claim(&foreign), "initialized SDK retains claim after sync timeout");
    ble_hs_cfg.sync_cb(); expect(port.start().ok(), "late sync permits retry"); port.stop();
    fake_nimble::state.stopResult = BLE_HS_EBUSY;
    expect(host.end().code() == blinker::ErrorCode::WouldBlock && fake_nimble::state.deinitCalls == 0, "failed stop never deinitializes");
    expect(!blinker::esp32_nimble_detail::claim(&foreign), "failed stop retains role");
    fake_nimble::state.stopResult = 0; fake_nimble::state.deinitResult = -1;
    expect(host.end().code() == blinker::ErrorCode::ProtocolError, "deinit error retained");
    expect(port.start().code() == blinker::ErrorCode::NotConfigured, "stopped SDK cannot lend new Port"); port.stop();
    const unsigned stops = fake_nimble::state.stopCalls;
    fake_nimble::state.deinitResult = 0;
    expect(host.end().ok() && fake_nimble::state.stopCalls == stops, "retry deinit without stopping dead task twice");
    expect(blinker::esp32_nimble_detail::claim(&foreign), "successful shutdown releases role");
    blinker::esp32_nimble_detail::release(&foreign);
}
void completionCanStartOtherBorrower() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "handoff fixture");
    ready(a);
    a.setConnectionHandlers(nullptr, [](void* context, const blinker::BleCentralConnectionInfo&) {
        expect(static_cast<blinker::Esp32NimBleCentralPort*>(context)->connect(request(2)).ok(), "completion starts next borrower");
    }, &b);
    a.stop();
    expect(a.state() == blinker::BleCentralPortState::Stopped && b.state() == blinker::BleCentralPortState::Scanning, "A stop does not wait for B scan");
    expect(fake_nimble::state.scanning, "next scan still active");
}
struct DualFixture {
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a, b;
    DualFixture() : a(host), b(host) {
        resetFake(); expect(a.start().ok() && b.start().ok(), "two Port slots");
        expect(a.connect(request()).ok(), "A scan"); ready(a, 7);
        expect(b.connect(request(2)).ok(), "B scan alongside A ACL"); ready(b, 8);
        expect(fake_nimble::connectionCount() == 2, "two distinct native ACLs");
    }
    ~DualFixture() { fake_nimble::state.deferTermination = false; fake_nimble::state.terminateResult = 0; }
};
void notify(uint16_t handle) {
    os_mbuf buffer = {}; buffer.size = 1; buffer.data[0] = 0x42;
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_NOTIFY_RX;
    event.notify_rx.conn_handle = handle; event.notify_rx.attr_handle = 6;
    event.notify_rx.indication = 1; event.notify_rx.om = &buffer; fake_nimble::emit(event);
}
void ack(uint16_t handle) {
    const ble_gatt_error ok = {0};
    fake_nimble::gatt(handle).write(handle, &ok, nullptr, fake_nimble::gatt(handle).context);
}
void interleavedDiscoveryAndInitiation() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "A initiates"); offer(a);
    expect(b.connect(request(2)).code() == blinker::ErrorCode::WouldBlock, "one native initiator");
    fake_nimble::connect(7);
    expect(b.connect(request(2)).code() == blinker::ErrorCode::WouldBlock, "SDK completion must reach loop first");
    a.poll(0);
    expect(a.state() == blinker::BleCentralPortState::Discovering && b.connect(request(2)).ok(), "A discovery does not monopolize initiating");
    ready(b, 8); finishDiscovery(a, 7);
    expect(fake_nimble::state.initCalls == 1 && fake_nimble::connectionCount() == 2, "one Host, concurrent independent discovery");
}
void independentDataAndBackpressure() {
    DualFixture f;
    const uint8_t byte = 0x31; const blinker::ByteView packet(&byte, 1);
    expect(f.a.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock &&
           f.b.sendPacket(2, packet).code() == blinker::ErrorCode::WouldBlock, "independent pending writes");
    const ble_gatt_error ok = {0};
    fake_nimble::gatt(7).write(8, &ok, nullptr, fake_nimble::gatt(7).context);
    expect(f.a.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock, "wrong native handle cannot ACK A");
    ack(8); expect(f.b.sendPacket(2, packet).ok(), "B ACK only completes B");
    expect(f.a.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock, "B ACK never completes A");
    ack(7); expect(f.a.sendPacket(1, packet).ok(), "A ACK completes A");
    fake_nimble::native(7)->writeResult = BLE_HS_EBUSY;
    expect(f.a.sendPacket(1, packet).code() == blinker::ErrorCode::WouldBlock, "A native write backpressure");
    expect(f.b.sendPacket(2, packet).code() == blinker::ErrorCode::WouldBlock, "B can still write"); ack(8);
    expect(f.b.sendPacket(2, packet).ok(), "B progresses despite A backpressure");
    unsigned aPackets = 0, bPackets = 0;
    auto receive = [](void* context, const blinker::BleCentralConnectionInfo&, blinker::ByteView) { ++*static_cast<unsigned*>(context); };
    f.a.setPacketReceiver(receive, &aPackets); f.b.setPacketReceiver(receive, &bPackets);
    for (unsigned i = 0; i < 40; ++i) notify(7);
    notify(8); f.b.poll(0);
    expect(aPackets == 0 && bPackets == 1, "full A RX queue does not block B");
    // A valid token paired with B's handle cannot discard A's queued data,
    // change A's security state, or terminate B via a spurious CONNECT.
    auto* aNative = fake_nimble::native(7); aNative->encrypted = 1;
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_ENC_CHANGE; event.enc_change.conn_handle = 7; fake_nimble::emit(event);
    event.enc_change.conn_handle = 8; aNative->gap(&event, aNative->context);
    event.type = BLE_GAP_EVENT_DISCONNECT; event.disconnect.conn.conn_handle = 8; aNative->gap(&event, aNative->context);
    event.type = BLE_GAP_EVENT_CONNECT; event.connect.conn_handle = 8; aNative->gap(&event, aNative->context);
    f.a.poll(0); expect(aPackets == 4, "A retains own data and bounded poll quota");
    blinker::BleCentralConnectionInfo info;
    expect(f.a.connection(info).ok() && info.encrypted && fake_nimble::alive(8), "wrong-handle GAP cannot affect either ACL");
    for (unsigned i = 0; i < 8; ++i) f.a.poll(0);
    expect(aPackets == 32 && bPackets == 1, "per-Port bounded RX capacity");
}
void stopDoesNotCancelAnotherInitiator(bool connecting) {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "stop-isolation fixture"); ready(a);
    expect(b.connect(request(2)).ok(), "B initiates");
    if (connecting) offer(b);
    const unsigned cancels = fake_nimble::state.scanCancelCalls;
    a.stop();
    expect(a.state() == blinker::BleCentralPortState::Stopped && fake_nimble::state.scanCancelCalls == cancels, "A closes without global scan cancel");
    expect(connecting ? fake_nimble::state.connecting : fake_nimble::state.scanning, "B native procedure survives A close");
    if (connecting) { fake_nimble::connect(8); b.poll(0); finishDiscovery(b, 8); } else ready(b, 8);
}
void failedCloseRetainsOnlyItsSlot() {
    DualFixture f;
    fake_nimble::state.terminateResult = BLE_HS_EBUSY;
    f.a.stop();
    expect(f.a.state() == blinker::BleCentralPortState::Error && fake_nimble::alive(7), "A rejected close retains slot");
    const uint8_t byte = 1; const blinker::ByteView packet(&byte, 1);
    expect(f.b.sendPacket(2, packet).code() == blinker::ErrorCode::WouldBlock, "B writes during A close failure"); ack(8);
    expect(f.b.sendPacket(2, packet).ok(), "B ACK unaffected");
    blinker::Esp32NimBleCentralPort overflow(f.host);
    expect(overflow.start().code() == blinker::ErrorCode::CapacityExceeded, "failed close not free capacity"); overflow.stop();
    fake_nimble::state.terminateResult = 0; f.a.stop();
    expect(f.b.state() == blinker::BleCentralPortState::Ready && fake_nimble::alive(8), "A retry closes only A");
}
void oldCallbacksAfterPeerSurvives() {
    DualFixture f;
    auto oldNative = *fake_nimble::native(7); auto oldGatt = fake_nimble::gatt(7);
    f.a.stop();
    expect(f.a.start().ok() && f.a.connect(request()).ok(), "A restarts without stopping B"); ready(f.a, 7);
    ble_gap_event oldEvent = {}; oldEvent.type = BLE_GAP_EVENT_DISCONNECT; oldEvent.disconnect.conn.conn_handle = 8;
    oldNative.gap(&oldEvent, oldNative.context);
    const ble_gatt_error failure = {BLE_HS_ENOTCONN}; oldGatt.write(7, &failure, nullptr, oldGatt.context);
    f.a.poll(0); f.b.poll(0);
    expect(f.a.state() == blinker::BleCentralPortState::Ready && f.b.state() == blinker::BleCentralPortState::Ready, "old token cannot reach either live Port");
}
void resetRetiresOldHandlesBeforePeerReusesThem() {
    DualFixture f;
    auto oldNative = *fake_nimble::native(7);
    for (auto& c : fake_nimble::state.connections) c.live = false;
    ble_hs_cfg.reset_cb(42); ble_hs_cfg.sync_cb();
    f.b.poll(0); f.b.stop();
    expect(f.b.start().ok() && f.b.connect(request(2)).ok(), "B explicitly recovers first"); ready(f.b, 7);
    // A has not even polled its reset yet. Its old numeric handle is B's now.
    f.a.stop();
    expect(fake_nimble::alive(7) && fake_nimble::native(7)->terminateCalls == 0, "A reset close never terminates B reused handle");
    ble_gap_event oldEvent = {}; oldEvent.type = BLE_GAP_EVENT_CONNECT; oldEvent.connect.conn_handle = 7;
    oldNative.gap(&oldEvent, oldNative.context); f.b.poll(0);
    expect(f.b.state() == blinker::BleCentralPortState::Ready, "pre-reset token invalidated");
}
void nativeAdmissionFailureLeavesPeerReady(bool scan) {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "SDK busy fixture"); ready(a);
    if (scan) {
        fake_nimble::state.scanResult = BLE_HS_EBUSY;
        expect(b.connect(request(2)).code() == blinker::ErrorCode::WouldBlock, "SDK rejects B scan");
        fake_nimble::state.scanResult = 0;
    } else {
        expect(b.connect(request(2)).ok(), "B scans"); fake_nimble::state.connectResult = BLE_HS_EALREADY;
        offer(b); fake_nimble::state.connectResult = 0;
    }
    expect(b.state() == blinker::BleCentralPortState::Idle && a.state() == blinker::BleCentralPortState::Ready && fake_nimble::alive(7), "failed B admission neither leaks slot nor closes A");
    expect(b.connect(request(3)).ok(), "B can retry explicit new attempt"); ready(b, 8);
}
void callbackDrainIsPerSlot() {
    DualFixture f;
    {
        blinker::Esp32NimBleCentralHost::Callback callback(fake_nimble::native(7)->context);
        f.a.stop();
        expect(f.a.state() == blinker::BleCentralPortState::Error, "A callback borrow still holds A slot");
        f.b.stop();
        expect(f.b.start().ok() && f.b.connect(request(3)).ok(), "B can reuse only its own slot while A drains"); ready(f.b, 8);
        expect(callback.owner() == &f.a, "borrowed A storage has not been replaced");
    }
    f.a.stop(); expect(fake_nimble::alive(8), "A final drain leaves B active");
}
void stoppedDiscoveryCannotPublishReady() {
    resetFake();
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    expect(a.start().ok() && b.start().ok() && a.connect(request()).ok(), "late Ready fixture");
    offer(a); fake_nimble::connect(7); a.poll(0);
    expect(b.connect(request(2)).ok(), "B may connect during A discovery"); ready(b, 8);
    unsigned connected = 0;
    a.setConnectionHandlers([](void* context, const blinker::BleCentralConnectionInfo&) {
        ++*static_cast<unsigned*>(context);
    }, nullptr, &connected);
    finishDiscovery(a, 7, false); // Successful CCCD result queued, not consumed.
    a.stop();
    expect(connected == 0, "stop must not publish late discovery Ready");
    expect(!fake_nimble::alive(7) && fake_nimble::alive(8), "stopped discovery closes only its ACL");
}
}
int main() {
    cancelledScan(); cancelledConnect(); scanHandoffRejected();
    successRacesCancellation(false); successRacesCancellation(true);
    failedDiscoveryWithRejectedTerminate(); stopWaitsForConnect();
    stuckConnectDoesNotPretendStopped();
    nativeConnectedBeforeCallback(false); nativeConnectedBeforeCallback(true);
    sharedHostSeparatesInitiation(); staleCallbacksCannotReachReusedHandle();
    callbackBorrowBlocksReuse(); resetIsLatchedAcrossSync();
    callbackBorrowDefersCompletion(false); callbackBorrowDefersCompletion(true);
    hostFailuresRetainExactOwnership(); completionCanStartOtherBorrower();
    interleavedDiscoveryAndInitiation(); independentDataAndBackpressure();
    stopDoesNotCancelAnotherInitiator(false); stopDoesNotCancelAnotherInitiator(true);
    failedCloseRetainsOnlyItsSlot(); oldCallbacksAfterPeerSurvives();
    resetRetiresOldHandlesBeforePeerReusesThem();
    nativeAdmissionFailureLeavesPeerReady(false); nativeAdmissionFailureLeavesPeerReady(true);
    callbackDrainIsPerSlot();
    stoppedDiscoveryCannotPublishReady();
    std::cout << "ESP32 NimBLE Central tests passed" << std::endl;
}
