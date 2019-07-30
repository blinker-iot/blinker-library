#if defined(ESP8266) || defined(ESP32)

#include "BlinkerConfig.h"
#include "BlinkerDebug.h"
#include "BlinkerTimer.h"

Ticker cdTicker;
Ticker lpTicker;
Ticker tmTicker;

bool _cdRunState = false;
bool _lpRunState = false;
bool _tmRunState = false;
bool _cdState = false;
bool _lpState = false;
bool _tmState = false;
bool _lpRun1 = true;
bool _tmRun1 = true;
bool _tmDay = false;
bool _cdTrigged = false;
bool _lpTrigged = false;
bool _tmTrigged = false;
bool _isTimingLoop = false;

uint8_t  _lpTimes;
uint8_t  _lpTrigged_times;

uint32_t _cdTime1;
uint32_t _cdTime2;
uint32_t _cdStart;
uint32_t _cdData;
// bool     _cdStop = true;

uint32_t _lpTime1;
uint32_t _lpTime1_start;
uint32_t _lpTime2;
uint32_t _lpTime2_start;
uint32_t _lpData;
bool     _lpStop = true;

uint32_t _tmTime1;
uint32_t _tmTime2;
uint32_t _tmTime;
uint8_t  _timingDay = 0;
uint8_t  taskCount = 0;
uint8_t  triggedTask = 0;

void disableTimer() {
    _cdRunState = false;
    cdTicker.detach();
    _lpRunState = false;
    lpTicker.detach();
    _tmRunState = false;
    tmTicker.detach();
}

void _cd_callback()
{
    uint32_t cd_during = millis() -_cdStart;
    uint32_t cd_remain = _cdTime1 * 60 - cd_during / 1000;

    if (_cdTime1 * 60 > BLINKER_ONE_HOUR_TIME) {
        if (cd_remain > 0) {
            if (cd_remain > BLINKER_ONE_HOUR_TIME) cd_remain = BLINKER_ONE_HOUR_TIME;

            cdTicker.once(cd_remain, _cd_callback);

            return;
        }
    }
    // _cdState = false;
    _cdTrigged = true;
    
    BLINKER_LOG_ALL(BLINKER_F("countdown trigged!"));
}

void _lp_callback()
{
    if (_lpRun1) {
        uint32_t lp_1_during = millis() -_lpTime1_start;
        uint32_t lp_1_remain = _lpTime1 * 60 - lp_1_during / 1000;

        if (_lpTime1 * 60 > BLINKER_ONE_HOUR_TIME) {
            if (lp_1_remain > 0) {
                if (lp_1_remain > BLINKER_ONE_HOUR_TIME) lp_1_remain = BLINKER_ONE_HOUR_TIME;

                lpTicker.once(lp_1_remain, _lp_callback);

                return;
            }
        }
    }
    else {
        uint32_t lp_2_during = millis() -_lpTime2_start;
        uint32_t lp_2_remain = _lpTime2 * 60 - lp_2_during / 1000;

        if (_lpTime2 * 60 > BLINKER_ONE_HOUR_TIME) {
            if (lp_2_remain > 0) {
                if (lp_2_remain > BLINKER_ONE_HOUR_TIME) lp_2_remain = BLINKER_ONE_HOUR_TIME;

                lpTicker.once(lp_2_remain, _lp_callback);

                return;
            }
        }
    }


    _lpRun1 = !_lpRun1;
    if (_lpRun1) {
        _lpTrigged_times++;

        if (_lpTimes) {
            if (_lpTimes == _lpTrigged_times && _lpTimes != 0) {
                lpTicker.detach();
                _lpStop = true;
            }
            else {
                lpTicker.once(_lpTime1 * 60, _lp_callback);
            }
        }
        else {
            lpTicker.once(_lpTime1 * 60, _lp_callback);
        }
    }
    else {
        lpTicker.once(_lpTime2 * 60, _lp_callback);
    }
    _lpTrigged = true;

    BLINKER_LOG_ALL(BLINKER_F("loop trigged!"));
}

void timingHandle(uint8_t cbackData)
{
    uint8_t task = cbackData;
    
    _tmTrigged = true;

    triggedTask = task;
}

#endif
