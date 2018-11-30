#ifndef BLINKER_DEBUG_H
#define BLINKER_DEBUG_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

#define BLINKER_DEBUG_F(s)  F(s)
#define BLINKER_F(s)        F(s)

uint32_t BLINKER_FreeHeap();

class BlinkerDebug
{
    enum blinker_debug_level_t
    {
        _debug_none,
        _debug_default,
        _debug_all
    };

    public :
        BlinkerDebug()
            : isInit(false)
            , debug_level(_debug_none)
        {}

        // BlinkerDebug(Stream& s, blinker_debug_level_t level = _debug_default)
        //     : debugger(&s)
        //     , debug_level(level)
        // {}

        void stream(Stream& s, blinker_debug_level_t level = _debug_default);
        void debugAll()     { debug_level = _debug_all; }

        void time();
        void freeheap();

        bool isDebug()      { return isInit ? debug_level != _debug_none : false; }
        bool isDebugAll()   { return isInit ? debug_level == _debug_all : false;}

        template <typename T>
        void print(T arg)   { debugger->print(arg); }

        template <typename T>
        void println(T arg) { debugger->println(arg); }
        void println()      { debugger->println(); }

    private :
        bool    isInit;
        Stream* debugger;
        blinker_debug_level_t debug_level;

        // uint32_t BLINKER_FreeHeap();
};

extern BlinkerDebug BLINKER_DEBUG;

void BLINKER_LOG_TIME();
void BLINKER_LOG_FreeHeap();
void BLINKER_LOG_FreeHeap_ALL();
extern void BLINKER_LOG_T();

/* BLINKER_LOG_T递归模板 */
template <typename T,typename... Ts>
void BLINKER_LOG_T(T arg,Ts... args)
{
    if (BLINKER_DEBUG.isDebug())
    {
        BLINKER_DEBUG.print(arg);
        BLINKER_LOG_T(args...);
    }
    return;
}
/* BLINKER_LOG可变参数模板 */
template <typename... Ts>
void BLINKER_LOG(Ts... args)
{
    BLINKER_LOG_TIME();
    BLINKER_LOG_T(args...);
    return;
}
/* BLINKER_ERR_LOG可变参数模板 */
template <typename... Ts>
void BLINKER_ERR_LOG(Ts... args)
{
    if (BLINKER_DEBUG.isDebug())
    {
        BLINKER_LOG_TIME();
        BLINKER_DEBUG.print(BLINKER_DEBUG_F("ERROR: "));
        BLINKER_LOG_T(args...);
    }
    return;
}

template <typename... Ts>
void BLINKER_LOG_ALL(Ts... args)
{
    if (BLINKER_DEBUG.isDebugAll())
    {
        BLINKER_LOG_TIME();
        BLINKER_LOG_T(args...);
    }
    return;
}

template <typename... Ts>
void BLINKER_ERR_LOG_ALL(Ts... args)
{
    if (BLINKER_DEBUG.isDebugAll()) { BLINKER_ERR_LOG(args...); }
    return;
}

#endif
