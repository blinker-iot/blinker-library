#ifndef BLINKER_TIMER_H
#define BLINKER_TIMER_H

#if defined(ESP8266) || defined(ESP32)

#include <Ticker.h>
#include <EEPROM.h>

extern Ticker cdTicker;
extern Ticker lpTicker;
extern Ticker tmTicker;

extern bool _cdRunState;
extern bool _lpRunState;
extern bool _tmRunState;
extern bool _cdState;
extern bool _lpState;
extern bool _tmState;
extern bool _lpRun1;
extern bool _tmRun1;
extern bool _tmDay;
extern bool _cdTrigged;
extern bool _lpTrigged;
extern bool _tmTrigged;
extern bool _isTimingLoop;

extern uint8_t  _lpTimes;
extern uint8_t  _lpTrigged_times;

extern uint32_t _cdTime1;
extern uint32_t _cdTime2;
extern uint32_t _cdStart;
extern uint32_t _cdData;
// bool     _cdStop = true;

extern uint32_t _lpTime1;
extern uint32_t _lpTime1_start;
extern uint32_t _lpTime2;
extern uint32_t _lpTime2_start;
extern uint32_t _lpData;
extern bool     _lpStop;

extern uint32_t _tmTime1;
extern uint32_t _tmTime2;
extern uint32_t _tmTime;
extern uint8_t  _timingDay;
extern uint8_t  taskCount;
extern uint8_t  triggedTask;

void disableTimer();
void _cd_callback();
void _lp_callback();
void timingHandle(uint8_t cbackData);

#endif

#endif
