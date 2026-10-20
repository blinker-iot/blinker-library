// Internal, operator-driven, one-shot diagnostic. NOT an Arduino example.
// Core setup/loop calls are wrapped; original product functions are retained
// by address references but NEVER invoked. I performs the original bounded
// modem.begin (including SOFTRESETWIFI) and a firmware query, not Cloud login,
// Prefs, task or identity operations. A whole-board power cycle is required after F.
#include <Arduino.h>
#include <Modem.h>
#include <WiFiS3.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>
#include <stdint.h>

extern "C" void __real_setup();
extern "C" void __real_loop();
extern "C" fsp_err_t __real_R_SCI_UART_Open(uart_ctrl_t*, const uart_cfg_t*);
extern "C" fsp_err_t __real_R_SCI_UART_Write(uart_ctrl_t*, const uint8_t*, uint32_t);
extern "C" fsp_err_t __real_R_SCI_UART_Close(uart_ctrl_t*);

namespace {
constexpr uint32_t kSci1Mstp = 1UL << 30U; // RA4M1 SCI1 MSTPB30.
constexpr char kQuery[] = CMD_READ(_FWVERSION_U32);
static_assert(sizeof(kQuery) - 1U == 19U, "Only the fixed read-only query is admitted");
struct Stop {
    uint32_t seen, elapsed, open, remaining, source, irqEnabled, irqPending;
    uint32_t mstp, scr, scrRead, bound, initialized;
};
struct Trace {
    uint32_t opens, writes, closes, accepted, length, remaining, source;
    uint32_t txi, tei, rxi, eri, noTransfer, firstByte, lastByte;
    uint32_t maskApplied, writeResult, faultMillis, executionMillis;
    uint32_t closedResult, preMask, preBasepri, irqAfterWrite;
    Stop stops[3];
};
Trace trace{};
bool initialized = false, attempted = false, armed = false, fired = false;
sci_uart_instance_ctrl_t* captured = nullptr;

bool target(const uart_cfg_t* cfg) { return cfg != nullptr && cfg->channel == 1U; }
uint32_t irqBit(IRQn_Type irq, bool pending) {
    return irq < 0 ? 0U : (pending ? NVIC_GetPendingIRQ(irq) : NVIC_GetEnableIRQ(irq));
}
uint32_t irqMask(const uart_cfg_t& cfg, bool pending) {
    return irqBit(cfg.txi_irq, pending) | (irqBit(cfg.tei_irq, pending) << 1U) |
        (irqBit(cfg.rxi_irq, pending) << 2U) | (irqBit(cfg.eri_irq, pending) << 3U);
}
void field(const char* key, uint32_t value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void header(const char* phase) {
    Serial.print("{\"fixture\":\"r4-uart-1147\",\"phase\":\"");
    Serial.print(phase); Serial.print('"');
}
void counts() {
    field("opens", trace.opens); field("writes", trace.writes); field("closes", trace.closes);
    field("faulted", blinker_r4_rpc_faulted());
    field("ownerQuarantined", blinker::renesasUnoModemOwner().quarantined());
    field("ownerOutcome", blinker::renesasUnoModemOwner().outcome());
    field("networkAllowed", blinker::renesasUnoModemOwner().networkAllowed());
}
uint32_t uartHash() {
    const volatile uint8_t* bytes = reinterpret_cast<const volatile uint8_t*>(&_UART3_);
    uint32_t result = 2166136261UL;
    for (size_t index = 0U; index < sizeof(_UART3_); ++index)
        result = (result ^ bytes[index]) * 16777619UL;
    return result;
}
void inspect() {
    header("inspect"); counts();
    field("initialized", initialized); field("attempted", attempted);
    field("armed", armed); field("fired", fired); field("bound", UART::g_uarts[1U] == &_UART3_);
    field("length", trace.length); field("accepted", trace.accepted);
    field("remaining", trace.remaining); field("source", trace.source);
    field("maskApplied", trace.maskApplied); field("writeResult", trace.writeResult);
    field("executionMillis", trace.executionMillis); field("closeResult", trace.closedResult);
    field("txi", trace.txi); field("tei", trace.tei); field("rxi", trace.rxi); field("eri", trace.eri);
    field("noTransfer", trace.noTransfer); field("preMask", trace.preMask); field("preBasepri", trace.preBasepri);
    field("irqAfterWrite", trace.irqAfterWrite);
    field("firstByte", trace.firstByte); field("lastByte", trace.lastByte);
    field("mstpSci1", (R_MSTP->MSTPCRB & kSci1Mstp) != 0U);
    // Do not touch SCI registers here: Close has stopped its peripheral clock.
    Serial.print(",\"syntheticOnly\":true,\"wireWaveformMeasured\":false}"); Serial.println();
    for (unsigned phase = 0U; phase < 3U; ++phase) {
        const Stop& value = trace.stops[phase];
        header("stop"); field("index", phase); field("seen", value.seen);
        field("elapsed", value.elapsed); field("open", value.open);
        field("remaining", value.remaining); field("source", value.source);
        field("irqEnabled", value.irqEnabled); field("irqPending", value.irqPending);
        field("mstp", value.mstp); field("scrBeforeCloseOnly", value.scr);
        field("scrRead", value.scrRead);
        field("bound", value.bound); field("initialized", value.initialized);
        Serial.println('}');
    }
}
void initialize() {
    if (attempted || fired || blinker_r4_rpc_faulted()) {
        header("initialize-denied"); counts(); Serial.println('}'); return;
    }
    attempted = true;
    modem.begin();
    std::string response;
    const bool valid = modem.beginned && !blinker_r4_rpc_faulted() &&
        modem.write(std::string(PROMPT(_FWVERSION_U32)), response, kQuery);
    initialized = valid && !response.empty() && !blinker_r4_rpc_faulted();
    header("initialized"); field("valid", valid); field("ready", initialized);
    field("responseBytes", response.size()); counts(); Serial.println('}');
}
void faultOnce() {
    if (!initialized || fired || armed || blinker_r4_rpc_faulted() ||
        __get_PRIMASK() != 0U || __get_BASEPRI() != 0U) {
        header("fault-denied"); counts(); Serial.println('}'); return;
    }
    // No malformed packet or mutation. The wrapper masks only modem TXI/TEI
    // before the next real write; the AGT millis timebase and other IRQs are
    // not masked here. This is not a SysTick or arbitrary-IRQ progress proof.
    armed = true; fired = true;
    std::string response;
    const uint32_t started = millis();
    const bool accepted = modem.write(std::string(PROMPT(_FWVERSION_U32)), response, kQuery);
    trace.executionMillis = millis() - started;
    header("fault-return"); field("accepted", accepted); counts(); Serial.println('}');
    inspect();
}
void deniedCalls() {
    if (!fired || !blinker_r4_rpc_faulted()) {
        header("deny-precondition"); counts(); Serial.println('}'); return;
    }
    const uint32_t opens = trace.opens, writes = trace.writes, closes = trace.closes;
    const uint32_t before = uartHash();
    uint8_t byte[] = {0x41U};
    Serial2.begin(115200U);
    const size_t single = Serial2.write(byte[0]);
    const size_t bulk = Serial2.write(byte, sizeof(byte));
    const size_t raw = Serial2.write_raw(byte, sizeof(byte));
    Serial2.flush();
    modem.begin();
    std::string response;
    const bool write = modem.write(std::string(PROMPT(_FWVERSION_U32)), response, kQuery);
    modem.write_nowait(std::string(PROMPT(_FWVERSION_U32)), response, kQuery);
    const bool passthrough = modem.passthrough(byte, sizeof(byte));
    const uint32_t version = WiFi.firmwareVersionU32();
    const uint32_t after = uartHash();
    header("postfault-deny"); field("openDelta", trace.opens - opens);
    field("writeDelta", trace.writes - writes); field("closeDelta", trace.closes - closes);
    field("single", single); field("bulk", bulk); field("raw", raw);
    field("modemWrite", write); field("passthrough", passthrough); field("firmwareVersion", version);
    field("uartUnchanged", before == after); counts();
    Serial.print(",\"directEndNotTested\":true}"); Serial.println();
}
void syntheticCallbacks() {
    if (!fired || !blinker_r4_rpc_faulted() || UART::g_uarts[1U] != nullptr) {
        header("callback-precondition"); counts(); Serial.println('}'); return;
    }
    const uint32_t before = uartHash();
    uart_callback_args_t args{};
    args.channel = 1U;
    args.event = UART_EVENT_TX_DATA_EMPTY; UART::WrapperCallback(&args);
    args.event = UART_EVENT_TX_COMPLETE; UART::WrapperCallback(&args);
    args.event = UART_EVENT_RX_CHAR; args.data = 0xA5U; UART::WrapperCallback(&args);
    const uint32_t after = uartHash();
    header("synthetic-callbacks"); field("count", 3U); field("uartUnchanged", before == after);
    field("unbound", UART::g_uarts[1U] == nullptr); counts();
    Serial.print(",\"actualLateIrqVerified\":false}"); Serial.println();
}
} // namespace

extern "C" fsp_err_t __wrap_R_SCI_UART_Open(uart_ctrl_t* control, const uart_cfg_t* config) {
    if (target(config)) ++trace.opens;
    return __real_R_SCI_UART_Open(control, config);
}
extern "C" fsp_err_t __wrap_R_SCI_UART_Write(uart_ctrl_t* control, const uint8_t* bytes, uint32_t length) {
    auto* native = reinterpret_cast<sci_uart_instance_ctrl_t*>(control);
    const bool modemWrite = native != nullptr && target(native->p_cfg);
    if (!modemWrite) return __real_R_SCI_UART_Write(control, bytes, length);
    ++trace.writes;
    if (armed && !trace.maskApplied) {
        const uart_cfg_t& cfg = *native->p_cfg;
        captured = native;
        trace.preMask = __get_PRIMASK(); trace.preBasepri = __get_BASEPRI();
        trace.noTransfer = cfg.p_transfer_tx == nullptr && cfg.p_transfer_rx == nullptr;
        trace.txi = cfg.txi_irq; trace.tei = cfg.tei_irq; trace.rxi = cfg.rxi_irq; trace.eri = cfg.eri_irq;
        // Only the reviewed modem and no-transfer graph may be fault-injected.
        if (!trace.noTransfer || cfg.txi_irq < 0 || cfg.tei_irq < 0) {
            armed = false; return __real_R_SCI_UART_Write(control, bytes, length);
        }
        NVIC_DisableIRQ(cfg.txi_irq); NVIC_DisableIRQ(cfg.tei_irq);
        __DSB(); __ISB();
        trace.maskApplied = 1U; trace.faultMillis = millis(); trace.length = length;
        trace.source = reinterpret_cast<uintptr_t>(bytes);
        trace.firstByte = length != 0U ? bytes[0] : 0U;
        trace.lastByte = length != 0U ? bytes[length - 1U] : 0U;
        const fsp_err_t result = __real_R_SCI_UART_Write(control, bytes, length);
        trace.writeResult = result; trace.accepted = result == FSP_SUCCESS ? length : 0U;
        trace.remaining = native->tx_src_bytes;
        trace.irqAfterWrite = irqMask(cfg, false);
        armed = false;
        return result;
    }
    return __real_R_SCI_UART_Write(control, bytes, length);
}
extern "C" fsp_err_t __wrap_R_SCI_UART_Close(uart_ctrl_t* control) {
    auto* native = reinterpret_cast<sci_uart_instance_ctrl_t*>(control);
    const bool modemClose = native != nullptr && target(native->p_cfg);
    if (modemClose) ++trace.closes;
    const fsp_err_t result = __real_R_SCI_UART_Close(control);
    if (modemClose) trace.closedResult = result;
    return result;
}
extern "C" void blinker_r4_uart_gate_stop(unsigned phase, UART* uart,
    sci_uart_instance_ctrl_t* control, const uart_cfg_t* config, bool ready) {
    if (phase >= 3U || uart != &_UART3_ || control != captured || !target(config)) return;
    Stop& value = trace.stops[phase];
    ++value.seen; value.elapsed = millis() - trace.faultMillis;
    value.open = control->open; value.remaining = control->tx_src_bytes;
    value.source = reinterpret_cast<uintptr_t>(control->p_tx_src);
    value.irqEnabled = irqMask(*config, false); value.irqPending = irqMask(*config, true);
    value.mstp = (R_MSTP->MSTPCRB & kSci1Mstp) != 0U;
    value.bound = UART::g_uarts[1U] == &_UART3_; value.initialized = ready;
    // SCR is readable only BEFORE Close. Phases 1/2 access RAM/NVIC/MSTP only.
    if (phase == 0U && value.mstp == 0U) {
        value.scr = control->p_reg->SCR;
        value.scrRead = 1U;
    }
}
extern "C" void __wrap_setup() {
    Serial.begin(115200U);
    header("ready");
    field("originalSetup", reinterpret_cast<uintptr_t>(&__real_setup));
    field("originalLoop", reinterpret_cast<uintptr_t>(&__real_loop));
    field("wholeRpcBudgetMillis", BLINKER_INTERNAL_R4_MODEM_RPC_MS);
    Serial.print(",\"originalProductEntryExecuted\":false,\"formalTaskAdmission\":false}"); Serial.println();
}
extern "C" void __wrap_loop() {
    if (!Serial.available()) return;
    const int command = Serial.read();
    if (command == 'I') initialize();
    else if (command == 'F') faultOnce();
    else if (command == 'D') deniedCalls();
    else if (command == 'C') syntheticCallbacks();
    else if (command == 'J') inspect();
}
