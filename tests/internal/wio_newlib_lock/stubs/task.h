#ifndef BLINKER_WIO_LOCK_TEST_TASK_H
#define BLINKER_WIO_LOCK_TEST_TASK_H
#include <FreeRTOS.h>
#define taskSCHEDULER_SUSPENDED 0
#define taskSCHEDULER_NOT_STARTED 1
#define taskSCHEDULER_RUNNING 2
BaseType_t xTaskGetSchedulerState();
void vTaskSuspendAll();
BaseType_t xTaskResumeAll();
#endif
