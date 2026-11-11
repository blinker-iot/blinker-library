#ifndef BLINKER_TEST_R4_UART_FSP_H
#define BLINKER_TEST_R4_UART_FSP_H
#include <stddef.h>
#include <stdint.h>
typedef int fsp_err_t;
typedef void uart_ctrl_t;
enum { FSP_SUCCESS=0, FSP_INVALID_VECTOR=-1, SCI_UART_CLOCK_INT=0,
    SCI_UART_START_BIT_FALLING_EDGE=0, SCI_UART_NOISE_CANCELLATION_DISABLE=0,
    SCI_UART_RX_FIFO_TRIGGER_MAX=0, SCI_UART_FLOW_CONTROL_RTS=0,
    SCI_UART_FLOW_CONTROL_HARDWARE_CTSRTS=1, SCI_UART_RS485_DISABLE=0,
    SCI_UART_RS485_DE_POLARITY_HIGH=0, UART_DATA_BITS_8=8, UART_PARITY_OFF=0,
    UART_PARITY_EVEN=1, UART_PARITY_ODD=2, UART_STOP_BITS_1=1, UART_STOP_BITS_2=2 };
enum { UART_EVENT_ERR_PARITY, UART_EVENT_ERR_FRAMING, UART_EVENT_ERR_OVERFLOW,
    UART_EVENT_RX_COMPLETE, UART_EVENT_TX_COMPLETE, UART_EVENT_TX_DATA_EMPTY,
    UART_EVENT_RX_CHAR, UART_EVENT_BREAK_DETECT };
struct uart_callback_args_t { uint32_t channel; int event; int data; };
struct baud_setting_t {
    struct { int abcse, abcs, bgdm, brme; } semr_baudrate_bits_b;
    int cks, brr;
    uint8_t mddr;
};
struct sci_uart_extended_cfg_t {
    int clock, rx_edge_start, noise_cancel, rx_fifo_trigger;
    baud_setting_t* p_baud_setting;
    int flow_control, flow_control_pin;
    struct { int enable, polarity, de_control_pin; } rs485_setting;
};
struct uart_cfg_t {
    int txi_irq, tei_irq, rxi_irq, eri_irq;
    uint32_t channel;
    void* p_context;
    void* p_extend;
    void* p_transfer_tx;
    void* p_transfer_rx;
    int data_bits, parity, stop_bits;
    void (*p_callback)(uart_callback_args_t*);
};
struct FakeSciRegisters { volatile uint8_t TDR; struct { volatile int TEND; } SSR_b; };
struct sci_uart_instance_ctrl_t { FakeSciRegisters* p_reg; const uart_cfg_t* p_cfg; };
fsp_err_t R_SCI_UART_BaudCalculate(unsigned long, bool, uint32_t, baud_setting_t*);
fsp_err_t R_SCI_UART_Open(sci_uart_instance_ctrl_t*, const uart_cfg_t*);
fsp_err_t R_SCI_UART_BaudSet(sci_uart_instance_ctrl_t*, void*);
fsp_err_t R_SCI_UART_Close(sci_uart_instance_ctrl_t*);
fsp_err_t R_SCI_UART_Write(sci_uart_instance_ctrl_t*, const uint8_t*, size_t);
#endif
