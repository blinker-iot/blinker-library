#ifndef RENESAS_TICKER_H
#define RENESAS_TICKER_H

#ifdef ARDUINO_ARCH_RENESAS

#include <Arduino.h>
#include <functional>
#include "FspTimer.h"

#define MAX_RENESAS_TIMERS 3

#define DIVISION_RATIO_GPT  1024
#define DIVISION_RATIO_AGT  8
#define LIMIT_PERIOD_GPT    1000  // 使用1000ms作为安全的分段大小
#define LIMIT_PERIOD_AGT    20    // 使用20ms而不是21ms，留出安全余量

// #define TICKER_DEBUG

typedef std::function<void()> callback_function_t;

class RenesasTicker {
public:
    RenesasTicker();
    ~RenesasTicker();

    void attach(float seconds, callback_function_t callback);
    void attach_ms(uint32_t milliseconds, callback_function_t callback);
    void attach_us(uint32_t microseconds, callback_function_t callback);
    
    template<typename TArg>
    void attach(float seconds, void (*callback)(TArg), TArg arg) {
        attach(seconds, [callback, arg]() { callback(arg); });
    }
    
    template<typename TArg>
    void attach_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
        attach_ms(milliseconds, [callback, arg]() { callback(arg); });
    }

    void once(float seconds, callback_function_t callback);
    void once_ms(uint32_t milliseconds, callback_function_t callback);
    void once_us(uint32_t microseconds, callback_function_t callback);
    
    template<typename TArg>
    void once(float seconds, void (*callback)(TArg), TArg arg) {
        once(seconds, [callback, arg]() { callback(arg); });
    }
    
    template<typename TArg>
    void once_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
        once_ms(milliseconds, [callback, arg]() { callback(arg); });
    }

    void detach();
    bool active() const;

private:
    int timerIndex;
    void _attach_internal(uint32_t milliseconds, bool repeat, callback_function_t callback);
    
    static bool timer_config(int slotIndex, timer_mode_t mode, uint32_t period_ms, bool start = true);
    static uint32_t calculate_period_max(uint8_t timerType);
    static bool setup_long_period_timer(int slotIndex, uint32_t period_ms, bool repeat);    // 静态定时器管理
    static void initializeTimers();
    static int findFreeTimer();
    static bool initialized;
    struct TimerSlot {
        FspTimer fspTimer;
        std::function<void()> callback;
        bool isActive;
        bool isRepeating;
        uint32_t intervalMs;
        
        // CBTimer长周期分段支持
        uint32_t remainMs;      // 剩余时间（用于长周期分段）
        uint32_t startMs;       // 启动时间戳
        uint32_t periodMax;     // 最大周期限制（根据定时器类型动态计算）
        uint8_t timerType;      // 定时器类型（GPT_TIMER或AGT_TIMER）
        int channel;            // 分配的通道号
        
        TimerSlot() : isActive(false), isRepeating(false), intervalMs(0), 
                     remainMs(0), startMs(0), periodMax(0), timerType(0), channel(-1) {}
    };
    
    static TimerSlot timerSlots[MAX_RENESAS_TIMERS];
    
    // 静态回调函数（供FSP定时器使用）
    static void timer_callback_0(timer_callback_args_t *p_args);
    static void timer_callback_1(timer_callback_args_t *p_args);
    static void timer_callback_2(timer_callback_args_t *p_args);
    static void timer_callback_3(timer_callback_args_t *p_args);
    
    static void handleCallback(int slotIndex);
    
    friend void processRenesasTimers();
};


#endif // ARDUINO_ARCH_RENESAS
#endif // RENESAS_TICKER_H