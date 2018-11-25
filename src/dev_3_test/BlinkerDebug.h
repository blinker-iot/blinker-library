#ifndef BLINKER_DEBUG_H
#define BLINKER_DEBUG_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

enum blinker_debug_level_t
{
    _debug_none,
    _debug_default,
    _debug_all
};

class BlinkerDebug
{
    public :
        BlinkerDebug()
            : debug_level(_debug_none)
        {}

        void stream(Stream& s, blinker_debug_level_t level = _debug_default);

        void time();
        void freeheap();
        void BLINKER_LOG_T();

        template <typename T,typename... Ts>
        void BLINKER_LOG_T(T arg,Ts... args);

        template <typename... Ts>
        void log(Ts... args);

        template <typename... Ts>
        void error(Ts... args);

        template <typename... Ts>
        void logAll(Ts... args);

        template <typename... Ts>
        void errorAll(Ts... args);

    private :
        Stream* debugger;
        blinker_debug_level_t debug_level;

        uint32_t BLINKER_FreeHeap();
};

extern BlinkerDebug Debug;

#endif
