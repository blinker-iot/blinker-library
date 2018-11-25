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

        BlinkerDebug(Stream& s, blinker_debug_level_t level = _debug_default)
            : debugger(&s)
            , debug_level(level)
        {}

        void stream(Stream& s, blinker_debug_level_t level = _debug_default);

        void time();
        void freeheap();

        bool isDebug()      { return debug_level != _debug_none; }
        bool isDebugAll()   { return debug_level == _debug_all;}

        template <typename T>
        void print(T arg)   { debugger->print(arg); }

        template <typename T>
        void println(T arg) { debugger->println(arg); }
        void println()      { debugger->println(); }

    private :
        Stream* debugger;
        blinker_debug_level_t debug_level;

        uint32_t BLINKER_FreeHeap();
};

extern BlinkerDebug Debug;

extern void BLINKER_LOG_TIME();
extern void BLINKER_LOG_FreeHeap();
extern void BLINKER_LOG_T();

/* BLINKER_LOG_T递归模板 */
template <typename T,typename... Ts>
void BLINKER_LOG_T(T arg,Ts... args)
{
    if (Debug.isDebug())
    {
        Debug.print(arg);
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
    if (Debug.isDebug())
    {
        BLINKER_LOG_TIME();
        Debug.print(BLINKER_DEBUG_F("ERROR: "));
        BLINKER_LOG_T(args...);
    }
    return;
}

template <typename... Ts>
void BLINKER_LOG_ALL(Ts... args)
{
    if (Debug.isDebugAll())
    {
        BLINKER_LOG_TIME();
        BLINKER_LOG_T(args...);
    }
    return;
}

template <typename... Ts>
void BLINKER_ERR_LOG_ALL(Ts... args)
{
    if (Debug.isDebugAll()) { BLINKER_ERR_LOG(args...); }
    return;
}

#endif
