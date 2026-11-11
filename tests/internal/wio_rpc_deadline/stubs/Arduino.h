#ifndef BLINKER_WIO_RPC_DEADLINE_HOST_ARDUINO_H
#define BLINKER_WIO_RPC_DEADLINE_HOST_ARDUINO_H
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace host_rpc {
extern uint32_t clockMs, primask, ipsr, basepri;
extern int schedulerState;
extern uintptr_t currentId;
extern unsigned pauseCalls, tryLockCalls, heldLocks, unsafeDriverCalls;
extern bool failAllTryLocks, observeHardware, dre, uartError, frameError;
extern std::function<void()> onPause;
extern std::function<void(uint32_t)> onSemaphoreWait;
extern std::function<void()> afterSignal;
extern std::vector<std::string> hardwareEvents;
extern std::vector<uint8_t> transmitted;
extern std::deque<uint8_t> incoming;
struct Register {
    uint32_t value;
    const char* name;
    Register(const char* label = "register") : value(0), name(label) {}
    operator uint32_t() const { return value; }
    volatile uint32_t* operator&() { return &value; }
    Register& operator=(uint32_t v);
    Register& operator&=(uint32_t v) { return *this = value & v; }
    Register& operator|=(uint32_t v) { return *this = value | v; }
};
struct NamedRegister { Register reg; explicit NamedRegister(const char* name = "register") : reg(name) {} };
}
struct FakeUsart {
    host_rpc::NamedRegister DATA, INTENCLR;
    FakeUsart() : DATA("data"), INTENCLR("intenclr") {}
};
struct Sercom { FakeUsart USART; };
extern Sercom hostSercom0;
#define SERCOM0 (&hostSercom0)
enum EPortType { PORTA = 0, PORTB = 1, PORTC = 2 };
struct FakePortGroup {
    host_rpc::NamedRegister OUTSET, OUTCLR, DIRSET, PINCFG[32];
    FakePortGroup() : OUTSET("outset"), OUTCLR("outclr"), DIRSET("dirset") {}
};
struct FakePort { FakePortGroup Group[3]; };
extern FakePort hostPort;
#define PORT (&hostPort)
#define PORT_PINCFG_PMUXEN 1U
#define SERCOM_USART_INTENCLR_MASK 0xBFU
enum IRQn_Type { SERCOM0_0_IRQn = 46, SERCOM0_1_IRQn, SERCOM0_2_IRQn, SERCOM0_3_IRQn };
extern bool hostIrqEnabled[4], hostIrqCleared[4];
void NVIC_DisableIRQ(IRQn_Type irq);
void NVIC_ClearPendingIRQ(IRQn_Type irq);
inline uint32_t __get_PRIMASK() { return host_rpc::primask; }
inline uint32_t __get_IPSR() { return host_rpc::ipsr; }
inline uint32_t __get_BASEPRI() { return host_rpc::basepri; }
inline void __disable_irq() { host_rpc::primask = 1U; }
inline void __set_PRIMASK(uint32_t value) { host_rpc::primask = value; }
inline uint32_t millis() { return host_rpc::clockMs; }
void delay(uint32_t value);
enum { taskSCHEDULER_NOT_STARTED = 0, taskSCHEDULER_RUNNING = 1, taskSCHEDULER_SUSPENDED = 2 };
inline int xTaskGetSchedulerState() { return host_rpc::schedulerState; }
inline void vTaskDelay(uint32_t ticks) { delay(ticks); }

enum { PIO_SERCOM = 1, OUTPUT = 1, INPUT = 0, SERIAL_8N1 = 12 };
struct PinDescription { EPortType ulPort; uint32_t ulPin; int ulPinType; };
extern PinDescription g_APinDescription[86];
inline int pinPeripheral(uint32_t, int) { return 0; }
inline void pinMode(uint32_t, uint32_t) {}
inline void digitalWrite(uint32_t, uint32_t) {}
#define HARDSER_STOP_BIT_MASK 1
#define HARDSER_STOP_BIT_1 0
#define HARDSER_STOP_BIT_2 1
#define HARDSER_DATA_MASK 12
#define HARDSER_DATA_5 0
#define HARDSER_DATA_6 4
#define HARDSER_DATA_7 8
#define HARDSER_DATA_8 12
#define HARDSER_PARITY_MASK 48
#define HARDSER_PARITY_NONE 0
#define HARDSER_PARITY_EVEN 16
#define HARDSER_PARITY_ODD 32
using std::min;

class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t) = 0;
    // Arduino's original Print bulk-write control flow, OS boundary seam.
    size_t write(const uint8_t* bytes, size_t count) {
        size_t done = 0;
        while (count--) { if (write(*bytes++)) ++done; else break; }
        return done;
    }
};
#endif
