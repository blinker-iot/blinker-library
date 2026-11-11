#ifndef BLINKER_TEST_R4_UART_ARDUINO_H
#define BLINKER_TEST_R4_UART_ARDUINO_H
#include <stddef.h>
#include <stdint.h>
#include <vector>
#ifdef _MSC_VER
#define __attribute(x)
#endif
#include "r_sci_uart.h"
#define PINS_COUNT 32
#define PIN_CFG_REQ_UART_TX 1
#define PIN_CFG_REQ_UART_RX 2
#define GET_CHANNEL(value) (value)
#define USE_SCI_EVEN_CFG(value) false
#define IOPORT_PERIPHERAL_SCI0_2_4_6_8 0
#define IOPORT_PERIPHERAL_SCI1_3_5_7_9 1
#define IOPORT_CFG_PERIPHERAL_PIN 1
typedef int ioport_peripheral_t;
typedef int bsp_io_port_pin_t;
struct FakePinCfg { int pin; };
extern FakePinCfg g_pin_cfg[32];
extern int g_ioport_ctrl;
inline std::vector<uint16_t> getPinCfgs(int, int) { return std::vector<uint16_t>(1, 1); }
inline int R_IOPORT_PinCfg(int*, int, uint32_t) { return 0; }
namespace arduino {
class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t) = 0;
    size_t write(const uint8_t* data, size_t size) {
        size_t done = 0;
        while (done < size && write(data[done]) == 1) ++done;
        return done;
    }
};
}
using arduino::Print;
#endif
