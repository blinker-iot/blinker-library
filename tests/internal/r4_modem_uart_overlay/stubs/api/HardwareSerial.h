#ifndef BLINKER_TEST_R4_UART_HARDWARE_SERIAL_H
#define BLINKER_TEST_R4_UART_HARDWARE_SERIAL_H
#include "Arduino.h"
#define SERIAL_8N1 1
#define SERIAL_8N2 2
#define SERIAL_8E1 3
#define SERIAL_8E2 4
#define SERIAL_8O1 5
#define SERIAL_8O2 6
namespace arduino {
class HardwareSerial : public Print {
public:
    virtual void begin(unsigned long) = 0;
    virtual void begin(unsigned long, uint16_t) = 0;
    virtual void end() = 0;
    virtual int available() = 0;
    virtual int peek() = 0;
    virtual int read() = 0;
    virtual void flush() = 0;
    virtual operator bool() = 0;
};
}
#endif
