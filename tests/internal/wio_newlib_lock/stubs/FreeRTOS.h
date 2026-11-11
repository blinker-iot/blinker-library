#ifndef BLINKER_WIO_LOCK_TEST_FREERTOS_H
#define BLINKER_WIO_LOCK_TEST_FREERTOS_H
typedef int BaseType_t;
// Match SAMD 1.8.5: timers enable this API even when INCLUDE_* is zero.
#define INCLUDE_xTaskGetSchedulerState 0
#define configUSE_TIMERS 1
#endif
