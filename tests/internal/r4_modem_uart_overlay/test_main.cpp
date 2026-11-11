// Compile the complete generated original Serial.cpp, not a second UART stack.
#include "Serial.h"
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>
#include <cstring>
#include <csetjmp>
#include <iostream>
#include <stdexcept>

UART _UART1_(0,1), _UART2_(2,3), _UART3_(24,25), _UART4_(4,5), _UART5_(6,7);
FakePinCfg g_pin_cfg[32] = {};
int g_ioport_ctrl = 0;
namespace {
bool active=true, faulted=false, emitEmpty=true, emitComplete=true;
bool injectFault=false;
unsigned checks=0, deadline=8, opens=0, writes=0, closes=0;
int writeResult=FSP_SUCCESS, closeResult=FSP_SUCCESS;
const uint8_t* retained=nullptr;
uart_cfg_t* lastCfg=nullptr;
FakeSciRegisters registers = {};
std::jmp_buf stopEnvironment;
#define CHECK(test) do { if (!(test)) throw std::runtime_error(#test); } while (0)
void callback(int event, int data=0) {
    uart_callback_args_t args = {1U,event,data};
    UART::WrapperCallback(&args);
}
void begin(UART& uart=_UART3_) { uart.begin(115200); CHECK(opens==1U); }
void stopped() {
    CHECK(faulted && closes==1U && retained==nullptr && UART::g_uarts[1]==nullptr);
    const unsigned oldOpens=opens, oldWrites=writes;
    _UART3_.begin(115200);
    CHECK(_UART3_.write(static_cast<uint8_t>(5))==0U);
    _UART3_.flush();
    CHECK(opens==oldOpens && writes==oldWrites && closes==1U);
}
bool haltedWrite(uint8_t* data, size_t size) {
    if (setjmp(stopEnvironment)==0) { _UART3_.write(data,size); return false; }
    return true;
}
}
extern "C" bool blinker_r4_rpc_active() { return active; }
extern "C" bool blinker_r4_rpc_expired() {
    ++checks;
    if (injectFault && checks>=deadline) { faulted=true; active=false; return false; }
    return checks>=deadline;
}
extern "C" bool blinker_r4_rpc_faulted() { return faulted; }
extern "C" void blinker_r4_rpc_fault() { faulted=true; active=false; }
// Test-only escape from a production noreturn halt; no UART frame owns C++
// objects requiring unwinding, and no assertion runs until the halt is caught.
extern "C" [[noreturn]] void blinker_r4_rpc_fail_stop() { std::longjmp(stopEnvironment,1); }
fsp_err_t R_SCI_UART_BaudCalculate(unsigned long, bool, uint32_t, baud_setting_t* baud) { baud->mddr=1; return 0; }
fsp_err_t R_SCI_UART_Open(sci_uart_instance_ctrl_t* control, const uart_cfg_t* config) {
    ++opens; control->p_cfg=config; control->p_reg=&registers;
    registers.SSR_b.TEND=1; lastCfg=const_cast<uart_cfg_t*>(config); return 0;
}
fsp_err_t R_SCI_UART_BaudSet(sci_uart_instance_ctrl_t*, void*) { return 0; }
fsp_err_t R_SCI_UART_Write(sci_uart_instance_ctrl_t*, const uint8_t* bytes, size_t) {
    ++writes; retained=bytes;
    if (writeResult!=FSP_SUCCESS) return writeResult;
    if (emitEmpty) { retained=nullptr; callback(UART_EVENT_TX_DATA_EMPTY); }
    if (emitComplete) callback(UART_EVENT_TX_COMPLETE);
    return FSP_SUCCESS;
}
fsp_err_t R_SCI_UART_Close(sci_uart_instance_ctrl_t*) {
    ++closes;
    if (closeResult==FSP_SUCCESS) retained=nullptr;
    return closeResult;
}
int main(int argc, char** argv) {
    if (argc!=2) return 2;
    const char* mode=argv[1];
    try {
        if (!std::strcmp(mode,"other-uart")) {
            begin(_UART1_); deadline=0;
            CHECK(_UART1_.write(static_cast<uint8_t>(1))==1U);
            _UART1_.flush(); CHECK(checks==0U && !faulted && closes==0U);
        } else {
            begin(); uint8_t data[]={1,2,3};
            if (!std::strcmp(mode,"normal")) {
                CHECK(_UART3_.write(static_cast<uint8_t>(1))==1U);
                CHECK(_UART3_.write(data,3)==3U); _UART3_.flush();
                CHECK(!faulted && closes==0U && retained==nullptr);
            } else if (!std::strcmp(mode,"inactive")) {
                active=false; deadline=0;
                CHECK(_UART3_.write(data,3)==3U); _UART3_.flush();
                CHECK(checks==0U && closes==0U && !faulted);
            } else if (!std::strcmp(mode,"const-whole-budget")) {
                deadline=5;
                CHECK(static_cast<arduino::Print&>(_UART3_).write(data,3)==2U);
                CHECK(writes==2U); stopped();
            } else if (!std::strcmp(mode,"expired-before-tx")) {
                deadline=0; CHECK(_UART3_.write(data,3)==0U);
                CHECK(writes==0U); stopped();
            } else if (!std::strcmp(mode,"write-error")) {
                writeResult=1; CHECK(_UART3_.write(data,3)==0U); stopped();
            } else if (!std::strcmp(mode,"flush-timeout") || !std::strcmp(mode,"fault-during-flush")) {
                emitComplete=false; CHECK(_UART3_.write(data,3)==3U);
                injectFault=!std::strcmp(mode,"fault-during-flush");
                _UART3_.flush(); stopped();
            } else if (!std::strcmp(mode,"raw-timeout")) {
                registers.SSR_b.TEND=0; CHECK(_UART3_.write_raw(data,3)==0U); stopped();
            } else if (!std::strcmp(mode,"close-failure") || !std::strcmp(mode,"transfer-refused")) {
                emitEmpty=false; emitComplete=false;
                const bool transfer=!std::strcmp(mode,"transfer-refused");
                if (transfer) lastCfg->p_transfer_tx=data; else closeResult=1;
                CHECK(haltedWrite(data,3) && faulted);
                CHECK(retained==(transfer ? nullptr : data));
                CHECK(writes==(transfer ? 0U : 1U));
                CHECK(closes==(transfer ? 0U : 1U));
            } else {
                emitEmpty=false; emitComplete=false;
                injectFault=!std::strcmp(mode,"fault-during-tx");
                if (!std::strcmp(mode,"byte-timeout")) CHECK(_UART3_.write(static_cast<uint8_t>(4))==0U);
                else if (!std::strcmp(mode,"const-print")) {
                    const uint8_t* constant=data;
                    CHECK(static_cast<arduino::Print&>(_UART3_).write(constant,3)==0U);
                } else CHECK(_UART3_.write(data,3)==0U);
                stopped();
                if (!std::strcmp(mode,"late-callback")) {
                    callback(UART_EVENT_RX_CHAR,42);
                    callback(UART_EVENT_TX_DATA_EMPTY); callback(UART_EVENT_TX_COMPLETE);
                    CHECK(_UART3_.available()==0 && UART::g_uarts[1]==nullptr);
                }
            }
        }
        std::cout<<"original Serial.cpp finite case passed: "<<mode<<"\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<"\n"; return 1; }
}
