#include "Arduino.h"
#include "erpc_arduino_uart_transport.h"
#include "erpc_crc16.h"
#include "blinker_wio_rpc_boundary.h"
#include <iostream>

using namespace erpc;
using namespace blinker::wio_rpc;
SERCOM nativeSercom;
EUart ble_uart(&nativeSercom, 84, 85, SERCOM_RX_PAD_2, UART_TX_PAD_0);
Thread clientThread(2);
#include "blinker_wio_rpc_port.inc"

namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Parked {};
struct ServerCounter {
    unsigned polls = 0;
    erpc_status_t poll() { ++polls; return kErpcStatus_Success; }
} g_server;
}
#include "run_server_excerpt.inc"

namespace {
void initialize() {
    hostPort.Group[PORTB].PINCFG[24].reg.name = "tx-mux";
    hostPort.Group[PORTC].PINCFG[24].reg.name = "rx-mux";
    hostPort.Group[PORTB].PINCFG[24].reg = PORT_PINCFG_PMUXEN;
    hostPort.Group[PORTC].PINCFG[24].reg = PORT_PINCFG_PMUXEN;
    host_rpc::observeHardware = true;
}
void feed(const std::vector<uint8_t>& bytes) {
    for (uint8_t byte : bytes) {
        host_rpc::incoming.push_back(byte);
        host_rpc::ipsr = 62U;
        ble_uart.IrqHandler();
        host_rpc::ipsr = 0U;
    }
}
std::vector<uint8_t> wire(const std::vector<uint8_t>& body, bool badCrc = false) {
    Crc16 crc;
    FramedTransport::Header header;
    header.m_messageSize = static_cast<uint16_t>(body.size());
    header.m_crc = crc.computeCRC16(body.data(), static_cast<uint32_t>(body.size()));
    if (badCrc) header.m_crc ^= 1U;
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&header);
    std::vector<uint8_t> result(raw, raw + sizeof(header));
    result.insert(result.end(), body.begin(), body.end());
    return result;
}
struct Transport {
    Crc16 crc;
    UartTransport uart;
    Transport() : uart(&ble_uart, 614400) { uart.setCrc16(&crc); }
};
struct RefusingDriver : HardwareSerialEx {
    unsigned attempts = 0, accepted = 0;
    explicit RefusingDriver(unsigned prefix) : accepted(prefix) {}
    int available() { return 0; }
    int peek() { return -1; }
    int read() { return -1; }
    void flush() {}
    operator bool() { return true; }
    size_t write(uint8_t) {
        ++attempts;
        if (attempts > accepted) return 0;
        boundary.markSend(); return 1;
    }
    void waitForRead() {}
    void waitForWrite(size_t) {}
};
void checkStopped() {
    for (unsigned i = 0; i != 4; ++i)
        require(!hostIrqEnabled[i] && hostIrqCleared[i], "all four IRQs disabled and pending cleared");
    require((uint32_t(hostPort.Group[PORTB].OUTSET.reg) & (1UL << 24)) != 0U, "TX idle-high before isolation");
    require((uint32_t(hostPort.Group[PORTB].PINCFG[24].reg) & PORT_PINCFG_PMUXEN) == 0U, "TX mux detached");
    require((uint32_t(hostPort.Group[PORTC].PINCFG[24].reg) & PORT_PINCFG_PMUXEN) == 0U, "RX mux detached");
    require(host_rpc::unsafeDriverCalls == 0U, "no native reset/flush/DRE busy wait");
    require(host_rpc::primask == 0U, "normal thread PRIMASK restored by native Guard");
}
void run(const std::string& name) {
    initialize();
    if (name == "wire-normal") {
        Transport transport;
        std::vector<uint8_t> body{1, 2, 3};
        MessageBuffer message(body.data(), static_cast<uint16_t>(body.size()));
        message.setUsed(static_cast<uint16_t>(body.size()));
        require(transport.uart.send(&message) == kErpcStatus_Success, "real frame sends");
        require(host_rpc::transmitted == wire(body), "wire header/CRC/payload unchanged");
        feed(wire(body));
        uint8_t output[8] = {};
        MessageBuffer received(output, sizeof(output));
        require(transport.uart.receive(&received) == kErpcStatus_Success, "real frame receives");
        require(received.getUsed() == 3U && std::equal(body.begin(), body.end(), output), "real frame body and length");
        require(!boundary.faulted() && host_rpc::unsafeDriverCalls == 0U, "healthy direct register path");
    } else if (name == "idle-first-byte") {
        Transport transport;
        bool delivered = false;
        host_rpc::onPause = [&] { if (!delivered && host_rpc::clockMs >= 1400U) { delivered = true; feed(wire({7})); } };
        uint8_t output[8] = {}; MessageBuffer received(output, sizeof(output));
        require(transport.uart.receive(&received) == kErpcStatus_Success, "idle parser may wait without an active request");
        require(host_rpc::clockMs == 1400U && output[0] == 7U && !boundary.faulted(), "first parser byte starts local RX budget");
    } else if (name == "partial-frame-deadline" || name == "whole-call-deadline") {
        Transport transport;
        const bool whole = name == "whole-call-deadline";
        if (whole) require(boundary.begin(1, blinker_wio_rpc_caller_allowed()), "whole-call owner");
        const std::vector<uint8_t> bytes = wire({9});
        bool first = false, header = false;
        host_rpc::onPause = [&] {
            const uint32_t at = whole ? 800U : 10U;
            if (!first && host_rpc::clockMs >= at) { first = true; feed({bytes[0]}); }
            if (!header && host_rpc::clockMs >= 900U) { header = true; feed({bytes[1], bytes[2], bytes[3]}); }
        };
        uint8_t output[8] = {}; MessageBuffer received(output, sizeof(output));
        require(transport.uart.receive(&received) != kErpcStatus_Success, "missing body must not succeed");
        require(boundary.reason() == Fault::Deadline, "deadline is the first sticky fault");
        require(host_rpc::clockMs == (whole ? 1000U : 1010U), "header/body and whole call do not reset budget");
        require(received.getUsed() == 0U, "no successful message length on failure");
        checkStopped();
    } else if (name == "root-lock-budget" || name == "local-lock-budget") {
        if (name == "root-lock-budget") {
            require(boundary.begin(1, blinker_wio_rpc_caller_allowed()), "owner admitted");
            host_rpc::clockMs = 900U;
        }
        Transport transport; host_rpc::failAllTryLocks = true;
        uint8_t body = 1; MessageBuffer message(&body, 1); message.setUsed(1);
        require(transport.uart.send(&message) != kErpcStatus_Success, "bounded lock exits");
        require(host_rpc::clockMs == 1000U && boundary.reason() == Fault::Deadline, "same remaining root/local lock budget");
        require(host_rpc::heldLocks == 0U && host_rpc::transmitted.empty(), "failed lock not unlocked or sent");
        checkStopped();
    } else if (name == "tx-full") {
        require(boundary.begin(1, blinker_wio_rpc_caller_allowed()), "owner admitted");
        Transport transport; host_rpc::dre = false;
        std::vector<uint8_t> body(252, 7); MessageBuffer message(body.data(), static_cast<uint16_t>(body.size()));
        message.setUsed(static_cast<uint16_t>(body.size()));
        require(transport.uart.send(&message) != kErpcStatus_Success, "TX-capacity waiting terminates");
        require(host_rpc::clockMs == 1000U && boundary.outcome() == Outcome::Uncertain, "queued prefix remains uncertain");
        checkStopped();
    } else if (name == "raw-tx-full") {
        require(boundary.begin(1, blinker_wio_rpc_caller_allowed()), "owner admitted");
        FrameScope frame(Direction::Tx); host_rpc::dre = false;
        for (unsigned i = 0; i != 255; ++i) require(ble_uart.write(uint8_t(i)) == 1U, "ring accepts finite prefix");
        require(ble_uart.write(1) == 0U && host_rpc::clockMs == 1000U, "raw full ring observes same deadline");
        checkStopped();
    } else if (name == "zero-write" || name == "short-write") {
        RefusingDriver driver(name == "short-write" ? 1U : 0U);
        UartTransport transport(&driver, 614400); Crc16 crc; transport.setCrc16(&crc);
        uint8_t body = 3; MessageBuffer message(&body, 1); message.setUsed(1);
        require(transport.send(&message) == kErpcStatus_SendFailed, "0/short native write fails, never loops");
        require(driver.attempts == (name == "short-write" ? 2U : 1U), "short write not retried");
        require(host_rpc::pauseCalls == 0U && boundary.reason() == Fault::Transport, "immediate transport fault");
        checkStopped();
    } else if (name == "crc-fault" || name == "oversized-frame") {
        Transport transport; feed(wire({1, 2, 3}, name == "crc-fault"));
        uint8_t output[8] = {}; MessageBuffer received(output, name == "oversized-frame" ? 2 : 8);
        require(transport.uart.receive(&received) != kErpcStatus_Success, "malformed frame fails");
        require(boundary.reason() == Fault::Transport && received.getUsed() == 0, "corrupt framing cannot resume as a new header");
        checkStopped();
    } else if (name == "quarantine-order" || name == "late-irq") {
        boundary.trip(Fault::Transport); checkStopped();
        const std::vector<std::string> expected{"intenclr", "disable-46", "disable-47", "disable-48", "disable-49",
            "outset", "dirset", "tx-mux", "rx-mux", "clear-46", "clear-47", "clear-48", "clear-49"};
        require(host_rpc::hardwareEvents == expected, "native stop ordering is finite and explicit");
        const size_t stoppedEvents = host_rpc::hardwareEvents.size();
        boundary.trip(Fault::Deadline);
        require(host_rpc::hardwareEvents.size() == stoppedEvents && boundary.reason() == Fault::Transport, "first fault and stop are sticky");
        if (name == "late-irq") {
            feed({8});
            require(ble_uart.available() == 0 && ble_uart.read() == -1 && ble_uart.write(1) == 0U, "late IRQ/API cannot touch rings/DATA");
            require(host_rpc::incoming.size() == 1U && host_rpc::transmitted.empty(), "late hardware byte not consumed or sent");
            ble_uart.begin(614400);
            Transport denied;
            require(denied.uart.init() == kErpcStatus_InitFailed, "faulted transport init cannot reopen UART");
            require(host_rpc::hardwareEvents.size() == stoppedEvents, "late init/IRQ leaves physical quarantine unchanged");
        }
    } else if (name == "park") {
        boundary.trip(Fault::Transport);
        host_rpc::onPause = [] { throw Parked(); };
        try { runServer(nullptr); require(false, "park should reach delay seam"); } catch (const Parked&) {}
        require(g_server.polls == 0U && host_rpc::clockMs == 20U, "generated server parks before poll/allocation");
    } else if (name == "caller-context") {
        require(blinker_wio_rpc_caller_allowed(), "exact client thread accepted");
        host_rpc::ipsr = 16; require(!blinker_wio_rpc_caller_allowed(), "ISR denied"); host_rpc::ipsr = 0;
        host_rpc::primask = 1; require(!blinker_wio_rpc_caller_allowed(), "PRIMASK denied"); host_rpc::primask = 0;
        host_rpc::basepri = 16; require(!blinker_wio_rpc_caller_allowed(), "BASEPRI denied"); host_rpc::basepri = 0;
        host_rpc::schedulerState = taskSCHEDULER_NOT_STARTED; require(!blinker_wio_rpc_caller_allowed(), "cold denied");
        host_rpc::schedulerState = taskSCHEDULER_SUSPENDED; require(!blinker_wio_rpc_caller_allowed(), "suspended denied");
        host_rpc::schedulerState = taskSCHEDULER_RUNNING; host_rpc::currentId = 3;
        require(!blinker_wio_rpc_caller_allowed(), "server/other thread denied");
        host_rpc::currentId = 2; require(blinker_wio_rpc_caller_allowed(), "client remains admitted");
    } else if (name == "clock-wrap") {
        host_rpc::clockMs = 0xfffffff0U;
        require(boundary.begin(1, blinker_wio_rpc_caller_allowed()), "wrap owner admitted");
        host_rpc::clockMs += 999U; require(boundary.ready(), "wrap before deadline");
        ++host_rpc::clockMs; require(!boundary.ready() && boundary.reason() == Fault::Deadline, "unsigned elapsed crosses wrap");
        checkStopped();
    } else throw std::runtime_error("unknown transport case");
    require(host_rpc::heldLocks == 0U, "all real scoped locks released");
}
}
int main(int argc, char** argv) {
    try { if (argc != 2) throw std::runtime_error("one case required"); run(argv[1]); std::cout << "PASS " << argv[1] << '\n'; return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
