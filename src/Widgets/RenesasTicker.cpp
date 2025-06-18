
#ifdef ARDUINO_ARCH_RENESAS

#include "RenesasTicker.h"

extern "C" {
    #include "r_gpt.h"
    #include "r_agt.h"
}

#ifndef R_FSP_SystemClockHzGet
    #define R_FSP_SystemClockHzGet(clock) (48000000UL)
#endif

#ifndef FSP_PRIV_CLOCK_PCLKD
    #define FSP_PRIV_CLOCK_PCLKD 0
#endif

#ifndef FSP_PRIV_CLOCK_PCLKB
    #define FSP_PRIV_CLOCK_PCLKB 1
#endif

RenesasTicker::TimerSlot RenesasTicker::timerSlots[MAX_RENESAS_TIMERS];
bool RenesasTicker::initialized = false;

RenesasTicker::RenesasTicker() {
    timerIndex = -1;
    if (!initialized) {
        initializeTimers();
        initialized = true;
    }
}

RenesasTicker::~RenesasTicker() {
    detach();
}

void RenesasTicker::initializeTimers() {
    uint32_t freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD);
    uint32_t default_period_max = (uint32_t)(65535000.0 * DIVISION_RATIO_GPT / freq_hz);
    
    for (int i = 0; i < MAX_RENESAS_TIMERS; i++) {
        timerSlots[i].isActive = false;
        timerSlots[i].isRepeating = false;
        timerSlots[i].intervalMs = 0;
        timerSlots[i].remainMs = 0;
        timerSlots[i].startMs = 0;
        timerSlots[i].periodMax = default_period_max;  // 设置默认值
        timerSlots[i].timerType = GPT_TIMER;  // 默认使用GPT
        timerSlots[i].channel = -1;
        timerSlots[i].callback = nullptr;
    }
}

int RenesasTicker::findFreeTimer() {
    for (int i = 0; i < MAX_RENESAS_TIMERS; i++) {
        if (!timerSlots[i].isActive) {
            return i;
        }
    }
    return -1;
}

void RenesasTicker::attach(float seconds, callback_function_t callback) {
    attach_ms((uint32_t)(seconds * 1000.0f), callback);
}

void RenesasTicker::attach_ms(uint32_t milliseconds, callback_function_t callback) {
    _attach_internal(milliseconds, true, callback);
}

void RenesasTicker::attach_us(uint32_t microseconds, callback_function_t callback) {
    uint32_t ms = (microseconds + 999) / 1000;
    if (ms == 0) ms = 1;
    attach_ms(ms, callback);
}

void RenesasTicker::once(float seconds, callback_function_t callback) {
    once_ms((uint32_t)(seconds * 1000.0f), callback);
}

void RenesasTicker::once_ms(uint32_t milliseconds, callback_function_t callback) {
    _attach_internal(milliseconds, false, callback);
}

void RenesasTicker::once_us(uint32_t microseconds, callback_function_t callback) {
    uint32_t ms = (microseconds + 999) / 1000;
    if (ms == 0) ms = 1;
    once_ms(ms, callback);
}

void RenesasTicker::_attach_internal(uint32_t milliseconds, bool repeat, callback_function_t callback) {
    if (!callback || milliseconds == 0) {
        return;
    }
    
    detach();
    
    timerIndex = findFreeTimer();
    if (timerIndex == -1) {
        return;
    }
    
    timerSlots[timerIndex].callback = callback;
    timerSlots[timerIndex].isRepeating = repeat;
    timerSlots[timerIndex].intervalMs = milliseconds;
    timerSlots[timerIndex].remainMs = milliseconds;
    timerSlots[timerIndex].startMs = millis();
    timerSlots[timerIndex].periodMax = LIMIT_PERIOD_GPT;
    
    if (setup_long_period_timer(timerIndex, milliseconds, repeat)) {
        timerSlots[timerIndex].isActive = true;
    } else {
        timerSlots[timerIndex].isActive = false;
        timerSlots[timerIndex].callback = nullptr;
        timerIndex = -1;
    }
}

void RenesasTicker::detach() {
    if (timerIndex >= 0 && timerIndex < MAX_RENESAS_TIMERS && timerSlots[timerIndex].isActive) {
        TimerSlot &slot = timerSlots[timerIndex];
        
        slot.fspTimer.stop();
        slot.fspTimer.end();
        
        slot.callback = nullptr;
        slot.isActive = false;
        slot.isRepeating = false;
        slot.intervalMs = 0;
        slot.remainMs = 0;
        slot.startMs = 0;
        slot.periodMax = 0;
        slot.timerType = 0;
        slot.channel = -1;
        
        timerIndex = -1;
    }
}

bool RenesasTicker::active() const {
    if (timerIndex >= 0 && timerIndex < MAX_RENESAS_TIMERS) {
        return timerSlots[timerIndex].isActive;
    }
    return false;
}

void RenesasTicker::timer_callback_0(timer_callback_args_t *p_args) {
    (void)p_args;
    handleCallback(0);
}

void RenesasTicker::timer_callback_1(timer_callback_args_t *p_args) {
    (void)p_args;
    handleCallback(1);
}

void RenesasTicker::timer_callback_2(timer_callback_args_t *p_args) {
    (void)p_args;
    handleCallback(2);
}

void RenesasTicker::timer_callback_3(timer_callback_args_t *p_args) {
    (void)p_args;
    handleCallback(3);
}

void RenesasTicker::handleCallback(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < MAX_RENESAS_TIMERS && 
        timerSlots[slotIndex].isActive && timerSlots[slotIndex].callback) {
        
        TimerSlot &slot = timerSlots[slotIndex];
        
        if (slot.intervalMs <= slot.periodMax) {
            slot.callback();
            
            if (!slot.isRepeating) {
                slot.fspTimer.stop();
                slot.fspTimer.end();
                slot.isActive = false;
                slot.callback = nullptr;
            }
        } else {
            uint32_t currentTime = millis();
            uint32_t elapsed = currentTime - slot.startMs;
            slot.startMs = currentTime;  // 更新开始时间
            
            if (slot.remainMs > slot.periodMax) {
                slot.remainMs -= elapsed;
                timer_mode_t mode = TIMER_MODE_PERIODIC;  // 中间分段使用PERIODIC
                timer_config(slotIndex, mode, slot.remainMs > slot.periodMax ? slot.periodMax : slot.remainMs, true);
            } else {
                if (slot.isRepeating) {
                    timer_config(slotIndex, TIMER_MODE_PERIODIC, slot.remainMs = slot.intervalMs, false);
                }
                
                slot.callback();
                
                if (!slot.isRepeating) {
                    slot.fspTimer.stop();
                    slot.fspTimer.end();
                    slot.isActive = false;
                    slot.callback = nullptr;
                }
            }
        }
    }
}

uint32_t RenesasTicker::calculate_period_max(uint8_t timerType) {
    if (timerType == GPT_TIMER) {
        return LIMIT_PERIOD_GPT;  // 1000ms
    } else {
        return LIMIT_PERIOD_AGT;  // 20ms
    }
}

bool RenesasTicker::setup_long_period_timer(int slotIndex, uint32_t period_ms, bool repeat) {
    if (slotIndex < 0 || slotIndex >= MAX_RENESAS_TIMERS) {
        return false;
    }
    
    TimerSlot &slot = timerSlots[slotIndex];
    
    if (slot.periodMax == 0) {
        slot.periodMax = LIMIT_PERIOD_GPT;
    }
    
    uint32_t first_segment;
    timer_mode_t mode;
    
    if (period_ms > slot.periodMax) {
        first_segment = slot.periodMax;
        mode = TIMER_MODE_PERIODIC;
    } else {
        first_segment = period_ms;
        mode = repeat ? TIMER_MODE_PERIODIC : TIMER_MODE_ONE_SHOT;
    }
    
    return timer_config(slotIndex, mode, first_segment, true);
}

bool RenesasTicker::timer_config(int slotIndex, timer_mode_t mode, uint32_t period_ms, bool start) {
    if (slotIndex < 0 || slotIndex >= MAX_RENESAS_TIMERS) {
        return false;
    }
    
    TimerSlot &slot = timerSlots[slotIndex];
    
    period_ms = (period_ms < (slot.periodMax > 0 ? slot.periodMax : LIMIT_PERIOD_GPT)) ? 
                 period_ms : (slot.periodMax > 0 ? slot.periodMax : LIMIT_PERIOD_GPT);
    
    if (slot.fspTimer.is_opened()) {
        slot.fspTimer.stop();
        slot.fspTimer.set_period_ms((double)period_ms);
        if (start) {
            slot.fspTimer.start();
            slot.startMs = millis();
        }
        return true;
    } else {        // 首次配置 - 基于Arduino UNO R4硬件限制的智能分配策略
        uint8_t timerType;
        int channel = -1;
        
        if (slotIndex < 2) {
            timerType = GPT_TIMER;
            
            channel = FspTimer::get_available_timer(timerType);
            
            if (channel == -1) {
                int preferred_channels[] = {3, 4, 5, 6, 7};
                for (int i = 0; i < 5; i++) {
                    bool channel_free = true;
                    for (int j = 0; j < MAX_RENESAS_TIMERS; j++) {
                        if (j != slotIndex && timerSlots[j].isActive && 
                            timerSlots[j].timerType == GPT_TIMER && 
                            timerSlots[j].channel == preferred_channels[i]) {
                            channel_free = false;
                            break;
                        }
                    }
                    if (channel_free) {
                        channel = preferred_channels[i];
                        break;
                    }
                }
            }
        #ifdef TICKER_DEBUG
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.print("分配GPT通道: ");
            Serial.println(channel);
        #endif
        } else if (slotIndex == 2) {
            timerType = AGT_TIMER;
            channel = FspTimer::get_available_timer(timerType);
            
            if (channel == -1 || channel == 0) {
            #ifdef TICKER_DEBUG
                Serial.print("定时器");
                Serial.print(slotIndex);
                Serial.println("AGT不可用，回退到GPT");
            #endif
                timerType = GPT_TIMER;
                channel = FspTimer::get_available_timer(timerType);
                
                if (channel == -1) {
                    int backup_channels[] = {2, 6, 7}; // 后备通道
                    for (int i = 0; i < 3; i++) {
                        bool channel_free = true;
                        for (int j = 0; j < MAX_RENESAS_TIMERS; j++) {
                            if (j != slotIndex && timerSlots[j].isActive && 
                                timerSlots[j].timerType == GPT_TIMER && 
                                timerSlots[j].channel == backup_channels[i]) {
                                channel_free = false;
                                break;
                            }
                        }
                        if (channel_free) {
                            channel = backup_channels[i];
                            break;
                        }
                    }
                }
            }
        #ifdef TICKER_DEBUG
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.print("分配");
            Serial.print(timerType == GPT_TIMER ? "GPT" : "AGT");
            Serial.print("通道: ");
            Serial.println(channel);
        #endif
        } else {
        #ifdef TICKER_DEBUG
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.println("错误：超出硬件限制，最多支持3个并发定时器");
        #endif
            return false;
        }
        
        if (channel == -1) {
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.println("分配失败：无可用定时器资源");
            return false;
        }
        uint32_t freq_hz;
        if (timerType == GPT_TIMER) {
            freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD);
            slot.periodMax = (uint32_t)(65535000.0 * DIVISION_RATIO_GPT / freq_hz);
        } else {
            freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB);
            slot.periodMax = (uint32_t)(65535000.0 * DIVISION_RATIO_AGT / freq_hz);
        }
        
        period_ms = min(period_ms, (uint32_t)slot.periodMax);
        
        slot.timerType = timerType;
        slot.channel = channel;
    #ifdef TIMER_DEBUG
        Serial.print("定时器");
        Serial.print(slotIndex);
        Serial.print("配置: ");
        Serial.print(timerType == GPT_TIMER ? "GPT" : "AGT");
        Serial.print("通道");
        Serial.print(channel);
        Serial.print(", period_max=");
        Serial.print(slot.periodMax);
        Serial.print("ms, 实际period=");
        Serial.print(period_ms);
        Serial.println("ms");
    #endif
        // 选择回调函数
        void (*timer_callback)(timer_callback_args_t*) = nullptr;
        switch (slotIndex) {
            case 0: timer_callback = timer_callback_0; break;
            case 1: timer_callback = timer_callback_1; break;
            case 2: timer_callback = timer_callback_2; break;
            case 3: timer_callback = timer_callback_3; break;
            default: return false;
        }        // 计算频率
        double frequency = 1000.0 / (double)period_ms;
    #ifdef TICKER_DEBUG
        Serial.print("定时器");
        Serial.print(slotIndex);
        Serial.print("初始化: 频率=");
        Serial.print(frequency, 4);
        Serial.print("Hz, 模式=");
        Serial.println(mode);
    #endif
        // 简化的FSP定时器初始化序列
        bool success = slot.fspTimer.begin(mode, timerType, channel, frequency, 
                                         (timerType == AGT_TIMER) ? 0.0 : 100.0, 
                                         timer_callback, nullptr);
        if (!success) {
        #ifdef TICKER_DEBUG
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.print("begin()失败，通道");
            Serial.print(channel);
            Serial.println("可能被占用");
        #endif
            return false;
        }
        
        success = slot.fspTimer.setup_overflow_irq();
        if (!success) {
        #ifdef TICKER_DEBUG
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.println("setup_overflow_irq()失败");
        #endif
            slot.fspTimer.end();
            return false;
        }
        
        success = slot.fspTimer.open();
        if (!success) {
        #ifdef TICKER_DEBUG
            Serial.print("定时器");
            Serial.print(slotIndex);
            Serial.println("open()失败");
        #endif
            slot.fspTimer.end();
            return false;
        }
        
        if (start) {
            success = slot.fspTimer.start();
            if (!success) {
                #ifdef TICKER_DEBUG
                Serial.print("定时器");
                Serial.print(slotIndex);
                Serial.println("start()失败");
                #endif
                slot.fspTimer.close();
                slot.fspTimer.end();
                return false;
            }
            slot.startMs = millis();
        }
        #ifdef TICKER_DEBUG
        Serial.print("定时器");
        Serial.print(slotIndex);
        Serial.println("配置成功");
        #endif
        return true;
    }
}

#endif // ARDUINO_ARCH_RENESAS
