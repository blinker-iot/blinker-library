#ifndef __BLINKER_SERIAL_H__
#define __BLINKER_SERIAL_H__

#include <stdint.h>
#include <stdarg.h>

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define EX_UART_NUM UART_NUM_0

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

static const char *TAG = "uart_events";

void uart_init_task(int baud_rate);

void uart_event_task(void *pvParameters);

#endif