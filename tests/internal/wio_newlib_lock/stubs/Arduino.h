#ifndef BLINKER_WIO_LOCK_TEST_ARDUINO_H
#define BLINKER_WIO_LOCK_TEST_ARDUINO_H
#include <stdint.h>
uint32_t __get_IPSR();
void __disable_irq();
void __NOP();
#endif
