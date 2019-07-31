#ifndef BlinkerTimingTimer_H
#define BlinkerTimingTimer_H

#if defined(ESP8266) || defined(ESP32)
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerTimingTimer
{
    public :
        BlinkerTimingTimer()
            : timerState(false)
            , isLoopTask(false)
        {}

        // BlinkerTimingTimer(uint32_t _timerData, String _action, String _text)
        BlinkerTimingTimer(uint32_t _timerData, String _action)
            : timerState(false)
            , isLoopTask(false)
        {
            timerData  = _timerData;
            // actionData = _action;

            actionData = (char*)malloc((_action.length()+1)*sizeof(char));
            strcpy(actionData, _action.c_str());

            // timerText = _text;

            isLoopTask = timerData >> 31;
            timerState = timerData >> 23 & 0x0001;
            timingDay  = timerData >> 11 & 0x007F;
            timingTime = timerData       & 0x07FF;
        }

        // BlinkerTimingTimer(bool _state, uint8_t _timingDay, uint16_t _timingTime, String _action, String _text, bool _isLoop)
        BlinkerTimingTimer(bool _state, uint8_t _timingDay, uint16_t _timingTime, String _action, bool _isLoop)
            : timerState(false)
            , isLoopTask(false)
        {
            timerState = _state;
            timingDay  = _timingDay;
            timingTime = _timingTime;

            actionData = (char*)malloc((_action.length()+1)*sizeof(char));
            strcpy(actionData, _action.c_str());

            // actionData = _action;
            // timerText = _text;
            isLoopTask = _isLoop;

            timerData  = isLoopTask << 31 | timerState << 23 | timingDay << 11 | timingTime;
        }

        // void freshTimer(uint32_t _timerData, String _action, String _text) {
        void freshTimer(uint32_t _timerData, String _action) {
            timerData = _timerData;

            actionData = (char*)malloc((_action.length()+1)*sizeof(char));
            strcpy(actionData, _action.c_str());
            
            // actionData = _action;
            // timerText = _text;

            isLoopTask = timerData >> 31;
            timerState = timerData >> 23 & 0x0001;
            timingDay  = timerData >> 11 & 0x007F;
            timingTime = timerData       & 0x07FF;
        }

        bool isTimingDay(uint8_t _day) {
            if (timingDay & (0x01 << _day)) return true;
            else return false;
        }

        uint8_t getTimingday() { return timingDay; }

        char * getAction() { return actionData; }

        // String getText() { return timerText; }

        uint32_t getTimerData() { return timerData; }

        uint16_t getTime() { return timingTime; }

        bool state() { return timerState; }

		bool isLoop() { return isLoopTask; }

        void disableTask() {
            timerState = false;

            timerData = isLoopTask << 31 | timerState << 23 | timingDay << 11 | timingTime;
        }

    private :
        // - - - - - - - - | - - - - - - - - | - - - - - - - - | - - - - - - - -
        // |                 |           |               | 11 0-0x7FF timingTime
        // |                 |           | 18 timingDay
        // |                 | 24 timerState
        // | 32 isLoopTask
        uint32_t timerData;
        uint8_t  timingDay;
        char*   actionData;
        // String   timerText;
        uint16_t timingTime;
        bool     timerState;
        bool     isLoopTask;
};

#endif

#endif
