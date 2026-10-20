#ifndef BLINKER_TEST_R4_UART_IRQ_H
#define BLINKER_TEST_R4_UART_IRQ_H
#define IRQ_SCI_UART 1
class IRQManager {
public:
    static IRQManager& getInstance() { static IRQManager manager; return manager; }
    bool addPeripheral(int, uart_cfg_t*) { return true; }
};
#endif
