#ifndef BLINKER_WIO_RPC_DEADLINE_HOST_SERCOM_H
#define BLINKER_WIO_RPC_DEADLINE_HOST_SERCOM_H
enum SercomRXPad { SERCOM_RX_PAD_2 = 2 };
enum SercomUartTXPad { UART_TX_PAD_0 = 0, UART_TX_RTS_CTS_PAD_0_2_3 = 3 };
enum SercomNumberStopBit { SERCOM_STOP_BIT_1 = 0, SERCOM_STOP_BITS_2 = 1 };
enum SercomUartCharSize { UART_CHAR_SIZE_5_BITS, UART_CHAR_SIZE_6_BITS, UART_CHAR_SIZE_7_BITS, UART_CHAR_SIZE_8_BITS };
enum SercomParityMode { SERCOM_NO_PARITY, SERCOM_EVEN_PARITY, SERCOM_ODD_PARITY };
enum { UART_INT_CLOCK = 0, SAMPLE_RATE_x16 = 0, LSB_FIRST = 0 };
class SERCOM {
public:
    void initUART(int, int, uint32_t) {}
    void initFrame(SercomUartCharSize, int, SercomParityMode, SercomNumberStopBit) {}
    void initPads(SercomUartTXPad, SercomRXPad) {}
    void enableUART() {}
    bool isFrameErrorUART() { return host_rpc::frameError; }
    bool isUARTError() { return host_rpc::uartError; }
    bool availableDataUART() { return !host_rpc::incoming.empty(); }
    uint8_t readDataUART() {
        if (host_rpc::primask != 1U) throw std::runtime_error("unguarded RX register read");
        const uint8_t value = host_rpc::incoming.front(); host_rpc::incoming.pop_front(); return value;
    }
    bool isDataRegisterEmptyUART() { return host_rpc::dre; }
    void disableDataRegisterEmptyInterruptUART() { host_rpc::hardwareEvents.push_back("dre-off"); }
    void enableDataRegisterEmptyInterruptUART() { host_rpc::hardwareEvents.push_back("dre-on"); }
    void resetUART() { ++host_rpc::unsafeDriverCalls; }
    void flushUART() { ++host_rpc::unsafeDriverCalls; }
    int writeDataUART(uint8_t) { ++host_rpc::unsafeDriverCalls; throw std::runtime_error("native DRE busy wait used"); }
};
#endif
