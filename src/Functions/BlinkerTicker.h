#ifndef BLINEKR_TICKER_H
#define BLINEKR_TICKER_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

#include "../Blinker/BlinkerDebug.h"

extern "C" {
    typedef void (*blinker_callback_t)(void);
}

class BlinkerTicker
{
    public :
        BlinkerTicker() {}
            // : day_time(0)
            // , os_time(0)
        // {}

        // void freshTime(uint32_t time_s) { day_time = time_s; os_time = millis(); }

        void attach(uint32_t seconds, blinker_callback_t func)
        {
            aim_time = seconds;
            tickerFunc = func;

            isRun = true;
        }

        void detach() { isRun = false; }

        void run()
        {
            if (millis() - os_time >= 1000 && millis() - os_time < 2000)
            {
                os_time += 1000;
                tick_time ++;
            }
            else if (millis() - os_time >= 2000)
            {
                tick_time += (millis() - os_time) / 1000;
                os_time = millis();
            }

            if (isRun)
            {
                if (tick_time - aim_time >= 0 && tick_time - aim_time < 60)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ticker trigged"));
                    isRun = false;

                    if (tickerFunc) tickerFunc();
                }
            }
        }

    protected :
        uint32_t tick_time;
        uint32_t os_time;
        uint32_t aim_time;
        bool     isRun = false;

        blinker_callback_t tickerFunc = NULL;
};

#endif
