#ifndef BLINKER_API_H
#define BLINKER_API_H

#include "Blinker/BlinkerApiBase.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerUtility.h"
#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
    #include "Blinker/BlinkerAuto.h"
    #include "../Widgets/BlinkerTimer.h"
    #include "../Widgets/BlinkerTimingTimer.h"
#endif

enum b_rgb_t {
    BLINKER_R,
    BLINKER_G,
    BLINKER_B,
    BLINKER_BRIGHT
};

enum b_joystickaxis_t {
    BLINKER_J_Xaxis,
    BLINKER_J_Yaxis
};

template <class Proto>
class BlinkerApi
{
    public :
        void attachData(blinker_callback_with_string_arg_t newFunction)
        { _availableFunc = newFunction; }

        void attachHeartbeat(blinker_callback_t newFunction)
        { _heartbeatFunc = newFunction; }
    
    #if defined(BLINKER_WIDGET)
        void freshAttachWidget(const char* _name, blinker_callback_with_string_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
            if(num >= 0 ) _Widgets_str[num]->setFunc(_func);
            else attachWidget(_name, _func);            
        }
        
        void freshAttachWidget(const char* _name, blinker_callback_with_int32_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
            if(num >= 0 ) _Widgets_int[num]->setFunc(_func);
            else attachWidget(_name, _func);            
        }
        
        void freshAttachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
            if(num >= 0 ) _Widgets_rgb[num]->setFunc(_func);
            else attachWidget(_name, _func);
        }
        
        void freshAttachWidget(const char* _name, blinker_callback_with_joy_arg_t _func)
        {
            int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
            if(num >= 0 ) _Widgets_joy[num]->setFunc(_func);
            else attachWidget(_name, _func);
        }

        void freshAttachWidget(char _name[], blinker_callback_with_table_arg_t _func, blinker_callback_t _func2)
        {
            int8_t num = checkNum(_name, _Widgets_tab, _wCount_tab);
            if(num >= 0 ) _Widgets_tab[num]->setFunc(_func, _func2);
            else attachWidget(_name, _func, _func2);
        }

        void strWidgetsParse(const char* _wName, const JsonObject& data);
        void intWidgetsParse(const char* _wName, const JsonObject& data);
        void rgbWidgetsParse(const char* _wName, const JsonObject& data);
        void joyWidgetsParse(const char* _wName, const JsonObject& data);
        void tabWidgetsParse(const char* _wName, const JsonObject& data);

        void widgetsParse(const JsonObject& data);

        uint8_t attachWidget(const char* _name, blinker_callback_with_string_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_int32_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_joy_arg_t _func);
        uint8_t attachWidget(const char* _name, blinker_callback_with_table_arg_t _func, blinker_callback_t _func2);
    #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
        void loadTimer();
        void loadTiming();
    #endif
    #endif

    #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) 
        void autoRun();
        void autoInput(const String & key, float data);
    #endif

    private :
        bool        _fresh = false;

        void heartBeat(const JsonObject& data);

        blinker_callback_with_string_arg_t  _availableFunc = NULL;
        blinker_callback_t                  _heartbeatFunc = NULL;

    #if defined(BLINKER_WIDGET)
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_int = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_joy = 0;
        uint8_t     _wCount_tab = 0;

        class BlinkerWidgets_string *       _Widgets_str[BLINKER_MAX_WIDGET_SIZE];
        class BlinkerWidgets_int32 *        _Widgets_int[BLINKER_MAX_WIDGET_SIZE];
        class BlinkerWidgets_rgb *          _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_joy *          _Widgets_joy[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_table *        _Widgets_tab[BLINKER_MAX_WIDGET_SIZE];
    #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
        char                                _cdAction[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
        char                                _lpAction1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
        char                                _lpAction2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
        class BlinkerTimingTimer *          timingTask[BLINKER_TIMING_TIMER_SIZE];

        void saveCountDown(uint32_t _data, char _action[]);
        void saveLoop(uint32_t _data, char _action1[], char _action2[]);
        void loadCountdown();
        void loadLoop();
        void checkOverlapping(uint8_t checkDays, uint16_t checkMins, uint8_t taskNum);
        void freshTiming(uint8_t wDay, uint16_t nowMins);
        void deleteTiming(uint8_t taskDel);
        void addTimingTask(uint8_t taskSet, uint32_t timerData, const String & action);
        void checkTimerErase();
        String timerSetting();
        String countdownConfig();
        String loopConfig();
        String timingConfig();
        String getTimingCfg(uint8_t task);
        bool timerManager(const JsonObject& data, bool _noSet = false);
    #endif
    #endif

#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
        uint8_t                             _aCount = 0;
        class BlinkerAUTO *                 _AUTO[2];

        void autoStart();
        bool autoManager(const JsonObject& data);
    #if defined(BLINKER_PRO_ESP)
        void otaParse(const JsonObject& data);
    #endif
#endif

#if defined(BLINKER_PRO_ESP) 
    #if defined(BLINKER_BUTTON)
        void attachClick(blinker_callback_t newFunction)
        { _clickFunc = newFunction; }
        void attachDoubleClick(blinker_callback_t newFunction)
        { _doubleClickFunc = newFunction; }
        void attachLongPressStart(blinker_callback_t newFunction)
        { _longPressStartFunc = newFunction; }
        void attachLongPressStop(blinker_callback_t newFunction)
        { _longPressStopFunc = newFunction; }
        void attachDuringLongPress(blinker_callback_t newFunction)
        { _duringLongPressFunc = newFunction; }
        #if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
            void attachLongPressPowerdown(blinker_callback_t newFunction)
            { _powerdownFunc = newFunction; }
            void attachLongPressReset(blinker_callback_t newFunction)
            { _resetFunc = newFunction; }

            uint16_t pressedTime();
        #endif
    #endif

    #if defined(BLINKER_NO_BUTTON)
        void attachNoButtonReset(blinker_callback_t newFunction)
        { _noButtonResetFunc = newFunction; }
    #endif
#endif
    protected :
        void parse(char _data[], bool ex_data = false);
#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
        void beginAuto();

    #if defined(BLINKER_WIDGET)
        bool checkTimer();
    #endif
#endif

#if defined(BLINKER_PRO_ESP) 
    #if defined(BLINKER_NO_BUTTON)
        bool            _isCheckPower = false;
        uint8_t         _power_count = 0;
        uint32_t        _reset_countdown = 0;
        blinker_callback_t _noButtonResetFunc;

        void noButtonInit()
        {
            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_POWER_ON_COUNT, _power_count);
            _power_count += 1;
            EEPROM.put(BLINKER_EEP_ADDR_POWER_ON_COUNT, _power_count);
            EEPROM.commit();
            EEPROM.end();

            BLINKER_LOG(BLINKER_F("_power_count: "), _power_count);

            _reset_countdown = millis();
            if (_power_count > 3)
            {
                if (_noButtonResetFunc) _noButtonResetFunc();
            }
        }

        void noButtonCheck()
        {
            if (millis() > 5000 && !_isCheckPower)
            {
                _isCheckPower = true;
                BLINKER_LOG_ALL(BLINKER_F("erase power count"));

                EEPROM.begin(BLINKER_EEP_SIZE);
                EEPROM.put(BLINKER_EEP_ADDR_POWER_ON_COUNT, 0);
                EEPROM.commit();
                EEPROM.end();
            }

            if (_power_count > 3)
            {
                if (millis() - _reset_countdown > 5000)
                {
                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_POWER_ON_COUNT, 0);
                    EEPROM.commit();
                    EEPROM.end();

                    reset();
                }
            }
        }
    #endif
    #if defined(BLINKER_BUTTON)
        #if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
            blinker_callback_t _powerdownFunc = NULL;
            blinker_callback_t _resetFunc = NULL;
        #endif
        // OneButton   button1;

        int _pin;        // hardware pin number.
        int _debounceTicks; // number of ticks for debounce times.
        int _clickTicks; // number of ticks that have to pass by before a click is detected
        int _pressTicks; // number of ticks that have to pass by before a long button press is detected

        int _buttonReleased;
        int _buttonPressed;

        bool _isLongPressed;

        // These variables will hold functions acting as event source.
        blinker_callback_t _clickFunc;
        blinker_callback_t _doubleClickFunc;
        blinker_callback_t _pressFunc;
        blinker_callback_t _longPressStartFunc;
        blinker_callback_t _longPressStopFunc;
        blinker_callback_t _duringLongPressFunc;

        // These variables that hold information across the upcoming tick calls.
        // They are initialized once on program start and are updated every time the tick function is called.
        int _state;
        unsigned long _startTime; // will be set in state 1
        unsigned long _stopTime; // will be set in state 2

        bool isPressed = false;

        void _click() { BLINKER_LOG_ALL(BLINKER_F("Button click.")); } // click

        void _doubleClick() { BLINKER_LOG_ALL(BLINKER_F("Button doubleclick.")); } // doubleclick1

        void _longPressStart()
        {
            BLINKER_LOG_ALL(BLINKER_F("Button longPress start"));
            // _longPressStartFunc();
            isPressed = true;
        } // longPressStart

        void _longPress()
        {
            if (isPressed)
            {
                BLINKER_LOG_ALL(BLINKER_F("Button longPress..."));
                isPressed = false;
            }// _duringLongPressFunc();
        } // longPress

        void _longPressStop()
        {
            BLINKER_LOG_ALL(BLINKER_F("Button longPress stop"));
            // _longPressStopFunc();
            // Bwlan.deleteConfig();
            // Bwlan.reset();
            // ESP.restart();
            // reset();

            uint32_t _pressedTime = millis() - _startTime;

            BLINKER_LOG_ALL(BLINKER_F("_stopTime: "), millis(), BLINKER_F(" ,_startTime: "), _startTime);
            BLINKER_LOG_ALL(BLINKER_F("long pressed time: "), _pressedTime);

            #if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
                if (_pressedTime >= BLINKER_PRESSTIME_RESET) {
                    if (_resetFunc) {
                        _resetFunc();
                    }

                    static_cast<Proto*>(this)->reset();
                }
                else {
                    BLINKER_LOG(BLINKER_F("BLINKER_PRESSTIME_POWERDOWN"));

                    if (_powerdownFunc) {
                        _powerdownFunc();
                    }
                }
            #else
                // if (_resetFunc) {
                //     _resetFunc();
                // }
                
                static_cast<Proto*>(this)->reset();
            #endif
        } // longPressStop

        void buttonInit(int activeLow = true)
        {
            _pin = BLINKER_BUTTON_PIN;

            _debounceTicks = 50;      // number of millisec that have to pass by before a click is assumed as safe.
            _clickTicks = 600;        // number of millisec that have to pass by before a click is detected.
            _pressTicks = 1000;       // number of millisec that have to pass by before a long button press is detected.

            _state = 0; // starting with state 0: waiting for button to be pressed
            _isLongPressed = false;  // Keep track of long press state

            if (activeLow) {
                // the button connects the input pin to GND when pressed.
                _buttonReleased = HIGH; // notPressed
                _buttonPressed = LOW;

                // use the given pin as input and activate internal PULLUP resistor.
                pinMode( _pin, INPUT_PULLUP );

            } else {
                // the button connects the input pin to VCC when pressed.
                _buttonReleased = LOW;
                _buttonPressed = HIGH;

                // use the given pin as input
                pinMode(_pin, INPUT);
            } // if

            _clickFunc = NULL;
            _doubleClickFunc = NULL;
            _pressFunc = NULL;
            _longPressStartFunc = NULL;
            _longPressStopFunc = NULL;
            _duringLongPressFunc = NULL;

            // attachInterrupt(BLINKER_BUTTON_PIN, checkButton, CHANGE);

            BLINKER_LOG_ALL(BLINKER_F("Button initialled"));
        }

        void tick()
        {
            // Detect the input information
            int buttonLevel = digitalRead(_pin); // current button signal.
            unsigned long now = millis(); // current (relative) time in msecs.

            // Implementation of the state machine
            if (_state == 0) { // waiting for menu pin being pressed.
                if (buttonLevel == _buttonPressed) {
                    _state = 1; // step to state 1
                    _startTime = now; // remember starting time
                } // if

            } else if (_state == 1) { // waiting for menu pin being released.

                if ((buttonLevel == _buttonReleased) && ((unsigned long)(now - _startTime) < _debounceTicks)) {
                    // button was released to quickly so I assume some debouncing.
                    // go back to state 0 without calling a function.
                    _state = 0;

                } else if (buttonLevel == _buttonReleased) {
                    _state = 2; // step to state 2
                    _stopTime = now; // remember stopping time

                } else if ((buttonLevel == _buttonPressed) && ((unsigned long)(now - _startTime) > _pressTicks)) {
                    _isLongPressed = true;  // Keep track of long press state
                    if (_pressFunc) _pressFunc();
                    _longPressStart();
                    if (_longPressStartFunc) _longPressStartFunc();
                    _longPress();
                    if (_duringLongPressFunc) _duringLongPressFunc();
                    _state = 6; // step to state 6

                } else {
                // wait. Stay in this state.
                } // if

            } else if (_state == 2) { // waiting for menu pin being pressed the second time or timeout.
                // if (_doubleClickFunc == NULL || (unsigned long)(now - _startTime) > _clickTicks) {
                if ((unsigned long)(now - _startTime) > _clickTicks) {
                    // this was only a single short click
                    _click();
                    if (_clickFunc) _clickFunc();
                    _state = 0; // restart.

                } else if ((buttonLevel == _buttonPressed) && ((unsigned long)(now - _stopTime) > _debounceTicks)) {
                    _state = 3; // step to state 3
                    _startTime = now; // remember starting time
                } // if

            } else if (_state == 3) { // waiting for menu pin being released finally.
                // Stay here for at least _debounceTicks because else we might end up in state 1 if the
                // button bounces for too long.
                if (buttonLevel == _buttonReleased && ((unsigned long)(now - _startTime) > _debounceTicks)) {
                    // this was a 2 click sequence.
                    _doubleClick();
                    if (_doubleClickFunc) _doubleClickFunc();
                    _state = 0; // restart.
                } // if

            } else if (_state == 6) { // waiting for menu pin being release after long press.
                if (buttonLevel == _buttonReleased) {
                    _isLongPressed = false;  // Keep track of long press state
                    _longPressStop();
                    if(_longPressStopFunc) _longPressStopFunc();
                    _state = 0; // restart.
                } else {
                    // button is being long pressed
                    _isLongPressed = true; // Keep track of long press state
                    _longPress();
                    if (_duringLongPressFunc) _duringLongPressFunc();
                } // if

            } // if

            // BLINKER_LOG("_state: ", _state);
        }
    #endif
#endif
};

template <class Proto>
void BlinkerApi<Proto>::parse(char _data[], bool ex_data)
{
    BLINKER_LOG_ALL(BLINKER_F("parse data: "), _data);
    
    if (!ex_data)
    {
        _fresh = false;

        
        DynamicJsonDocument jsonBuffer(BLINKER_JSON_BUFFER_SIZE);
        DeserializationError error = deserializeJson(jsonBuffer, STRING_format(_data));
        JsonObject root = jsonBuffer.as<JsonObject>();

        if (error)
        {
            return;
        }
    #if defined(BLINKER_PRO_ESP)
        otaParse(root);
    #endif
    #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
        autoManager(root);
    #endif

        heartBeat(root);

    #if defined(BLINKER_WIDGET)
        #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
            timerManager(root);
        #endif

        widgetsParse(root);
    #endif

        if (_fresh)
        {
            static_cast<Proto*>(this)->flush();
        }
        else
        {
            if (_availableFunc)
            {
                _availableFunc(_data);
            }
            static_cast<Proto*>(this)->flush();
        }
    }
    else
    {
        String arrayData = BLINKER_F("{\"data\":");
        arrayData += _data;
        arrayData += BLINKER_F("}");

        BLINKER_LOG_ALL(BLINKER_F("ex_data parse data: "), arrayData);
        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& root = jsonBuffer.parseObject(arrayData);
        DynamicJsonDocument jsonBuffer(BLINKER_JSON_BUFFER_SIZE);
        DeserializationError error = deserializeJson(jsonBuffer, arrayData);
        JsonObject root = jsonBuffer.as<JsonObject>();

        // if (!root.success()) return;
        if (error) return;

        arrayData = root["data"][0].as<String>();

        if (arrayData != "null")
        {
            for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
            {
                arrayData = root["data"][a_num].as<String>();

                if(arrayData != "null")
                {
                    // DynamicJsonBuffer _jsonBuffer;
                    // JsonObject& _array = _jsonBuffer.parseObject(arrayData);
                    DynamicJsonDocument jsonBuffer(BLINKER_JSON_BUFFER_SIZE);
                    deserializeJson(jsonBuffer, arrayData);
                    JsonObject _array = jsonBuffer.as<JsonObject>();

                #if defined(BLINKER_WIDGET)
                #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
                    timerManager(_array, true);
                #endif

                    widgetsParse(_array);
                #endif
                }
                else
                {
                    return;
                }
            }
        }
        else
        {
            // JsonObject& root = jsonBuffer.parseObject(_data);
            DeserializationError error = deserializeJson(jsonBuffer, _data);
            JsonObject root = jsonBuffer.as<JsonObject>();

            // if (!root.success()) return;
            if (error) return;

        #if defined(BLINKER_WIDGET)
            widgetsParse(root);
        #endif
        }
    }
}

template <class Proto>
void BlinkerApi<Proto>::heartBeat(const JsonObject& data)
{
    String state = data[BLINKER_CMD_GET];

    // if (state.length())
    if (data.containsKey(BLINKER_CMD_GET))
    {
        if (state == BLINKER_CMD_STATE)
        {
        #if defined(BLINKER_WIDGET)
        #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
            String  _timer = taskCount ? "1":"0";
                    _timer += _cdState ? "1":"0";
                    _timer += _lpState ? "1":"0";

            BLINKER_LOG_ALL(BLINKER_F("timer codes: "), _timer);
            static_cast<Proto*>(this)->print(BLINKER_CMD_TIMER, _timer);
        #endif
        #endif

            if (_heartbeatFunc) { _heartbeatFunc(); }
            static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
            static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_OTA_VERSION_CODE);
            static_cast<Proto*>(this)->checkState(false);
            static_cast<Proto*>(this)->printNow();

            _fresh = true;
        }
    }
}

#if defined(BLINKER_WIDGET)
    template <class Proto>
    uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_string_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL)
        {
            if (_wCount_str <= BLINKER_MAX_WIDGET_SIZE)
            {
                _Widgets_str[_wCount_str] = new BlinkerWidgets_string(_name, _func);
                _wCount_str++;

                BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                            BLINKER_F(" _wCount_str: "), _wCount_str);
                return _wCount_str;
            }
            else
            {
                return 0;
            }
        }
        else if(num >= 0 )
        {
            BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                    BLINKER_F(" < has been registered, please register another name!"));
            return 0;
        }
        else
        {
            return 0;
        }
    }

    template <class Proto>
    uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_int32_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
        if (num == BLINKER_OBJECT_NOT_AVAIL)
        {
            if (_wCount_int < BLINKER_MAX_WIDGET_SIZE)
            {
                _Widgets_int[_wCount_int] = new BlinkerWidgets_int32(_name, _func);
                _wCount_int++;

                BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                            BLINKER_F(" _wCount_int: "), _wCount_int);

                return _wCount_int;
            }
            else
            {
                return 0;
            }
        }
        else if(num >= 0 )
        {
            BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                    BLINKER_F(" < has been registered, please register another name!"));
            return 0;
        }
        else
        {
            return 0;
        }
    }

    template <class Proto>
    uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_rgb_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
        if (num == BLINKER_OBJECT_NOT_AVAIL)
        {
            if (_wCount_rgb < BLINKER_MAX_WIDGET_SIZE/2)
            {
                _Widgets_rgb[_wCount_rgb] = new BlinkerWidgets_rgb(_name, _func);
                _wCount_rgb++;

                BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                            BLINKER_F(" _wCount_rgb: "), _wCount_rgb);

                return _wCount_rgb;
            }
            else
            {
                return 0;
            }
        }
        else if(num >= 0 )
        {
            BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                    BLINKER_F(" < has been registered, please register another name!"));
            return 0;
        }
        else
        {
            return 0;
        }
    }

    template <class Proto>
    uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_joy_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
        if (num == BLINKER_OBJECT_NOT_AVAIL)
        {
            if (_wCount_joy < BLINKER_MAX_WIDGET_SIZE/2)
            {
                _Widgets_joy[_wCount_joy] = new BlinkerWidgets_joy(_name, _func);
                _wCount_joy++;

                BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                            BLINKER_F(" _wCount_joy: "), _wCount_joy);

                return _wCount_joy;
            }
            else
            {
                return 0;
            }
        }
        else if(num >= 0 )
        {
            BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                    BLINKER_F(" < has been registered, please register another name!"));
            return 0;
        }
        else
        {
            return 0;
        }
    }

    template <class Proto>
    uint8_t BlinkerApi<Proto>::attachWidget(const char* _name, blinker_callback_with_table_arg_t _func,
            blinker_callback_t _func2)
    {
        int8_t num = checkNum(_name, _Widgets_tab, _wCount_tab);
        if (num == BLINKER_OBJECT_NOT_AVAIL)
        {
            if (_wCount_tab < BLINKER_MAX_WIDGET_SIZE*2)
            {
                _Widgets_tab[_wCount_tab] = new BlinkerWidgets_table(_name, _func, _func2);
                _wCount_tab++;

                BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                            BLINKER_F(" _wCount_tab: "), _wCount_tab);

                return _wCount_tab;
            }
            else
            {
                return 0;
            }
        }
        else if(num >= 0 )
        {
            BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                    BLINKER_F(" < has been registered, please register another name!"));
            return 0;
        }
        else
        {
            return 0;
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::widgetsParse(const JsonObject& data)
    {
        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), data);
        }
        for (uint8_t wNum = 0; wNum < _wCount_int; wNum++) {
            intWidgetsParse(_Widgets_int[wNum]->getName(), data);
        }
        for (uint8_t wNum = 0; wNum < _wCount_rgb; wNum++) {
            rgbWidgetsParse(_Widgets_rgb[wNum]->getName(), data);
        }
        for (uint8_t wNum = 0; wNum < _wCount_joy; wNum++) {
            joyWidgetsParse(_Widgets_joy[wNum]->getName(), data);
        }
        for (uint8_t wNum = 0; wNum < _wCount_tab; wNum++) {
            tabWidgetsParse(_Widgets_tab[wNum]->getName(), data);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::strWidgetsParse(const char* _wName, const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            String state = data[_wName];
            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse isParsed"));
            _fresh = true;

            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse: "), _wName);

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();

            if (nbFunc) nbFunc(state);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::intWidgetsParse(const char* _wName, const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName)) {
            int _number = data[_wName];
            BLINKER_LOG_ALL(BLINKER_F("intWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
            if (wFunc) {
                wFunc(_number);
            }
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::rgbWidgetsParse(const char* _wName, const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            uint8_t _rValue = data[_wName][BLINKER_R];
            uint8_t _gValue = data[_wName][BLINKER_G];
            uint8_t _bValue = data[_wName][BLINKER_B];
            uint8_t _brightValue = data[_wName][BLINKER_BRIGHT];
            BLINKER_LOG_ALL(BLINKER_F("rgbWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
            if (wFunc) wFunc(_rValue, _gValue, _bValue, _brightValue);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::joyWidgetsParse(const char* _wName, const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            int16_t jxAxisValue = data[_wName][BLINKER_J_Xaxis];
            uint8_t jyAxisValue = data[_wName][BLINKER_J_Yaxis];
            BLINKER_LOG_ALL(BLINKER_F("joyWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();
            if (wFunc) wFunc(jxAxisValue, jyAxisValue);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::tabWidgetsParse(const char* _wName, const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_tab, _wCount_tab);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName)) {
            String _setData = data[_wName];

            uint8_t _number = 0;

            blinker_callback_with_table_arg_t wFunc = _Widgets_tab[num]->getFunc();
                    
            for (uint8_t num = 0; num < 5; num++)
            {
                if (strcmp(_setData.substring(num, num + 1).c_str(), "1") == 0)
                {
                    if (wFunc) {
                        switch (num)
                        {
                            case 0:
                                wFunc(BLINKER_CMD_TAB_0);
                                break;
                            case 1:
                                wFunc(BLINKER_CMD_TAB_1);
                                break;
                            case 2:
                                wFunc(BLINKER_CMD_TAB_2);
                                break;
                            case 3:
                                wFunc(BLINKER_CMD_TAB_3);
                                break;
                            case 4:
                                wFunc(BLINKER_CMD_TAB_4);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            
            BLINKER_LOG_ALL(BLINKER_F("tabWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_t wFunc2 = _Widgets_tab[num]->getFunc2();
            if (wFunc2) {
                wFunc2();
            }
        }
    }
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) 
    template <class Proto>
    void BlinkerApi<Proto>::saveCountDown(uint32_t _data, char _action[])
    {
        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _data);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _action);
        EEPROM.commit();
        EEPROM.end();
    }

    template <class Proto>
    void BlinkerApi<Proto>::saveLoop(uint32_t _data, char _action1[], char _action2[])
    {
        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _data);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _action1);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _action2);
        EEPROM.commit();
        EEPROM.end();
    }

    template <class Proto>
    void BlinkerApi<Proto>::loadCountdown()
    {
        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction);
        EEPROM.commit();
        EEPROM.end();

        _cdState    = _cdData >> 31;
        _cdRunState = _cdData >> 30 & 0x0001;
        _cdTime1    = _cdData >> 12 & 0x0FFF;
        _cdTime2    = _cdData       & 0x0FFF;

        if (_cdTime1 == 0)
        {
            _cdState = 0;
            _cdRunState = 0;
        }

        BLINKER_LOG_ALL(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
        BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
        BLINKER_LOG_ALL(BLINKER_F("_totalTime: "), _cdTime1);
        BLINKER_LOG_ALL(BLINKER_F("_runTime: "), _cdTime2);
        BLINKER_LOG_ALL(BLINKER_F("_action: "), _cdAction);

        if (_cdState && _cdRunState)
        {
            uint32_t _cdTime1_;

            if (_cdTime1 * 60 > BLINKER_ONE_HOUR_TIME) _cdTime1_ = BLINKER_ONE_HOUR_TIME;
            else _cdTime1_ = _cdTime1 * 60;

            cdTicker.once(_cdTime1_, _cd_callback);

            _cdStart = millis();

            BLINKER_LOG_ALL(BLINKER_F("countdown start!"));
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::loadLoop()
    {
        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_TRI, _lpTrigged_times);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction1);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction2);
        EEPROM.commit();
        EEPROM.end();

        _lpState    = _lpData >> 31;
        _lpRunState = _lpData >> 30 & 0x0001;
        _lpTimes    = _lpData >> 22 & 0x007F;
        _lpTime1    = _lpData >> 11 & 0x07FF;
        _lpTime2    = _lpData       & 0x07FF;

        BLINKER_LOG_ALL(BLINKER_F("loop state: "), _lpState ? "true" : "false");
        BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);
        BLINKER_LOG_ALL(BLINKER_F("_times: "), _lpTimes);
        BLINKER_LOG_ALL(BLINKER_F("_tri_times: "), _lpTrigged_times);
        BLINKER_LOG_ALL(BLINKER_F("_time1: "), _lpTime1);
        BLINKER_LOG_ALL(BLINKER_F("_action1: "), _lpAction1);
        BLINKER_LOG_ALL(BLINKER_F("_time2: "), _lpTime2);
        BLINKER_LOG_ALL(BLINKER_F("_action2: "), _lpAction2);
        BLINKER_LOG_ALL(BLINKER_F("_lpData: "), _lpData);

        if (_lpState && _lpRunState && (_lpTimes == 0))
        {
            _lpRun1 = true;
            _lpStop = false;

            uint32_t _lpTime1_;

            if (_lpTime1 * 60 > BLINKER_ONE_HOUR_TIME) _lpTime1_ = BLINKER_ONE_HOUR_TIME;
            else _lpTime1_ = _lpTime1 * 60;

            _lpTime1_start = millis();
            lpTicker.once(_lpTime1_, _lp_callback);

            BLINKER_LOG_ALL(BLINKER_F("loop start!"));
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::loadTiming()
    {
        BLINKER_LOG_ALL(BLINKER_F("load timing"));

        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
        uint32_t _tmData;
        char     _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];

        if (taskCount > BLINKER_TIMING_TIMER_SIZE)
        {
            taskCount = 0;
        }
        BLINKER_LOG_ALL(BLINKER_F("load timing taskCount: "), taskCount);

        for(uint8_t task = 0; task < taskCount; task++)
        {
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE
                        , _tmData);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE +
                        BLINKER_TIMER_TIMING_SIZE, _tmAction_);

            timingTask[task] = new BlinkerTimingTimer(_tmData, STRING_format(_tmAction_));

            BLINKER_LOG_ALL(BLINKER_F("_tmData: "), _tmData);
            BLINKER_LOG_ALL(BLINKER_F("_tmAction: "), STRING_format(_tmAction_));
        }
        EEPROM.commit();
        EEPROM.end();

        uint8_t  wDay = static_cast<Proto*>(this)->wday();
        uint16_t nowMins = static_cast<Proto*>(this)->hour() * 60 + static_cast<Proto*>(this)->minute();

        freshTiming(wDay, nowMins);
    }

    template <class Proto>
    void BlinkerApi<Proto>::checkOverlapping(uint8_t checkDays, uint16_t checkMins, uint8_t taskNum)
    {
        BLINKER_LOG_ALL(BLINKER_F("checkMins: "), checkMins);
        BLINKER_LOG_ALL(BLINKER_F("checkDays: "), checkDays);

        char _tmAction[BLINKER_TIMER_TIMING_ACTION_SIZE];

        for (uint8_t task = 0; task < taskCount; task++)
        {
            
            BLINKER_LOG_ALL(BLINKER_F("getTime: "), timingTask[task]->getTime());
            BLINKER_LOG_ALL(BLINKER_F("isLoop: "), timingTask[task]->isLoop());
            BLINKER_LOG_ALL(BLINKER_F("state: "), timingTask[task]->state());
            BLINKER_LOG_ALL(BLINKER_F("isTimingDay: "), timingTask[task]->isTimingDay(checkDays));

            if((timingTask[task]->getTime() == checkMins) && \
                !timingTask[task]->isLoop() && \
                timingTask[task]->state() && \
                timingTask[task]->isTimingDay(checkDays))
            {

                timingTask[task]->disableTask();

                EEPROM.begin(BLINKER_EEP_SIZE);
                EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);

                EEPROM.put( BLINKER_EEP_ADDR_TIMER_TIMING + \
                            task * BLINKER_ONE_TIMER_TIMING_SIZE, \
                            timingTask[task]->getTimerData());

                EEPROM.commit();
                EEPROM.end();

                BLINKER_LOG_ALL(BLINKER_F("disable timerData: "), timingTask[task]->getTimerData());
                BLINKER_LOG_ALL(BLINKER_F("disableTask: "), task);

                strcpy(_tmAction, timingTask[task]->getAction());

                if (task != taskNum)
                {
                    // #if defined(BLINKER_AT_MQTT)
                    //     static_cast<Proto*>(this)->serialPrint(_tmAction);
                    // #else
                        parse(_tmAction, true);
                    // #endif
                }
            }
            else if((timingTask[task]->getTime() == checkMins) && \
                timingTask[task]->state() && \
                timingTask[task]->isTimingDay(checkDays))
            {
                BLINKER_LOG(BLINKER_F("checkOverlapping, timing trigged, action is: "), _tmAction);

                strcpy(_tmAction, timingTask[task]->getAction());
                
                if (task != taskNum)
                {
                    // #if defined(BLINKER_AT_MQTT)
                    //     static_cast<Proto*>(this)->serialPrint(_tmAction);
                    // #else
                        parse(_tmAction, true);
                    // #endif
                }
            }
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::freshTiming(uint8_t wDay, uint16_t nowMins)
    {
        tmTicker.detach();

        uint8_t  cbackData;
        uint8_t  nextTask = BLINKER_TIMING_TIMER_SIZE;
        // uint16_t timingMinsNext;
        uint32_t apartSeconds = BLINKER_ONE_DAY_TIME;
        uint32_t checkSeconds = BLINKER_ONE_DAY_TIME;
        uint32_t nowSeconds = static_cast<Proto*>(this)->dtime();

        BLINKER_LOG_ALL(BLINKER_F("freshTiming wDay: "), wDay,
                        BLINKER_F(", nowMins: "), nowMins,
                        BLINKER_F(", nowSeconds: "), nowSeconds);

        for (uint8_t task = 0; task < taskCount; task++)
        {
            if (timingTask[task]->isTimingDay(wDay) && timingTask[task]->state())
            {
                if (timingTask[task]->getTime() > nowMins)
                {
                    checkSeconds = timingTask[task]->getTime() * 60 - nowSeconds;

                    // checkSeconds =  checkSeconds / 60 / 30;

                    if (checkSeconds <= apartSeconds) {
                        apartSeconds = checkSeconds;
                        nextTask = task;
                    }
                }
            }

            BLINKER_LOG_ALL(BLINKER_F("isTimingDay: "), timingTask[task]->isTimingDay(wDay));
            BLINKER_LOG_ALL(BLINKER_F("state: "), timingTask[task]->state());
            BLINKER_LOG_ALL(BLINKER_F("getTime: "), timingTask[task]->getTime());

            BLINKER_LOG_ALL(BLINKER_F("for nextTask: "), nextTask,
                            BLINKER_F("  apartSeconds: "), apartSeconds,
                            BLINKER_F(" wDay: "), wDay);
        }

        if (apartSeconds == BLINKER_ONE_DAY_TIME)
        {
            apartSeconds -= nowSeconds;

            // apartSeconds = apartSeconds / 60 / 30;
            BLINKER_LOG_ALL(BLINKER_F("nextTask: "), nextTask,
                            BLINKER_F("  apartSeconds: "), apartSeconds,
                            BLINKER_F(" wDay: "), wDay);

            cbackData = nextTask;
        }
        else
        {
            BLINKER_LOG_ALL(BLINKER_F("nextTask: "), nextTask,
                            BLINKER_F("  apartSeconds: "), apartSeconds,
                            BLINKER_F(" wDay: "), wDay);

            cbackData = nextTask;
        }
        BLINKER_LOG_ALL(BLINKER_F("cbackData: "), cbackData);

        if (apartSeconds > BLINKER_ONE_HOUR_TIME)
        {
            apartSeconds = BLINKER_ONE_HOUR_TIME;

            BLINKER_LOG_ALL(BLINKER_F("change apartSeconds: "), apartSeconds);
        }

        tmTicker.once(apartSeconds, timingHandle, cbackData);
    }

    template <class Proto>
    void BlinkerApi<Proto>::deleteTiming(uint8_t taskDel)
    {
        if (taskDel < taskCount)
        {
            // tmTicker.detach();

            for (uint8_t task = taskDel; task < (taskCount - 1); task++)
            {
                // timingTask[task]->freshTimer(timingTask[task + 1]->getTimerData(),
                //     timingTask[task + 1]->getAction(), timingTask[task + 1]->getText());
                timingTask[task]->freshTimer(timingTask[task + 1]->getTimerData(),
                                            timingTask[task + 1]->getAction());
            }

            delete timingTask[taskCount - 1];

            taskCount--;

            BLINKER_LOG_ALL(BLINKER_F("delete task: "), taskDel, BLINKER_F(" success!"));

            uint8_t  wDay = static_cast<Proto*>(this)->wday();
            uint16_t nowMins = static_cast<Proto*>(this)->hour() * 60 + static_cast<Proto*>(this)->minute();
            freshTiming(wDay, nowMins);
        }
        else
        {
            BLINKER_LOG_ALL(BLINKER_F("none task to delete!"));
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::addTimingTask(uint8_t taskSet, uint32_t timerData, const String & action)
    {
        BLINKER_LOG_ALL(BLINKER_F("addTimingTask taskSet: "), taskSet);
        BLINKER_LOG_ALL(BLINKER_F("addTimingTask timerData: "), timerData);

        if (taskSet <= taskCount && taskCount <= BLINKER_TIMING_TIMER_SIZE)
        {
            // tmTicker.detach();

            if (taskSet == taskCount)
            {
                if (taskCount == BLINKER_TIMING_TIMER_SIZE)
                {
                    BLINKER_ERR_LOG(BLINKER_F("timing timer task is full"));
                    return;
                }
                // timingTask[taskSet] = new BlinkerTimingTimer(timerData, action, text);
                timingTask[taskSet] = new BlinkerTimingTimer(timerData, action);
                taskCount++;

                BLINKER_LOG_ALL(BLINKER_F("new BlinkerTimingTimer"));
            }
            else
            {
                // timingTask[taskSet]->freshTimer(timerData, action, text);
                timingTask[taskSet]->freshTimer(timerData, action);

                BLINKER_LOG_ALL(BLINKER_F("freshTimer"));
            }

            // if (taskSet <= taskCount) taskCount++;

            BLINKER_LOG_ALL(BLINKER_F("taskCount: "), taskCount);

            uint8_t  wDay = static_cast<Proto*>(this)->wday();
            uint16_t nowMins = static_cast<Proto*>(this)->hour() * 60 + static_cast<Proto*>(this)->minute();

            freshTiming(wDay, nowMins);
        }
        else {
            BLINKER_ERR_LOG(BLINKER_F("timing timer task is full"));
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::checkTimerErase()
    {
        // #if defined(ESP8266)
        static uint8_t isErase;
        // #endif

        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.get(BLINKER_EEP_ADDR_TIMER_ERASE, isErase);

        if (isErase)
        {
            for (uint16_t _addr = BLINKER_EEP_ADDR_TIMER;
                _addr < BLINKER_EEP_ADDR_TIMER_END; _addr++)
            {
                EEPROM.put(_addr, "\0");
            }
        }

        EEPROM.commit();
        EEPROM.end();
    }

    template <class Proto>
    String BlinkerApi<Proto>::timerSetting()
    {

        String _data = BLINKER_F("\"");
        _data += BLINKER_F(BLINKER_CMD_COUNTDOWN);
        _data += BLINKER_F("\":");
        _data += STRING_format(_cdState ? "true" : "false");
        _data += BLINKER_F(",\"");
        _data += BLINKER_F(BLINKER_CMD_LOOP);
        _data += BLINKER_F("\":");
        _data += STRING_format(_lpState ? "true" : "false");
        _data += BLINKER_F(",\"");
        _data += BLINKER_F(BLINKER_CMD_TIMING);
        _data += BLINKER_F("\":");
        _data += STRING_format(taskCount ? "true" : "false");


        BLINKER_LOG_ALL(BLINKER_F("timerSetting: "), _data);

        return _data;
    }

    template <class Proto>
    String BlinkerApi<Proto>::countdownConfig()
    {
        String cdData;

        if (!_cdState)
        {
            cdData = BLINKER_F("{\"");
            cdData += BLINKER_F(BLINKER_CMD_COUNTDOWN);
            cdData += BLINKER_F("\":false}");
        }
        else
        {
            if (_cdRunState)
            {
                cdData = BLINKER_F("{\"");
                cdData += BLINKER_F(BLINKER_CMD_COUNTDOWN);
                cdData += BLINKER_F("\":{\"");
                cdData += BLINKER_F(BLINKER_CMD_RUN);
                cdData += BLINKER_F("\":");
                cdData += STRING_format(_cdRunState ? 1 : 0);
                cdData += BLINKER_F(",\"");
                cdData += BLINKER_F(BLINKER_CMD_TOTALTIME);
                cdData += BLINKER_F("\":");
                cdData += STRING_format(_cdTime1);
                cdData += BLINKER_F(",\"");
                cdData += BLINKER_F(BLINKER_CMD_RUNTIME);
                cdData += BLINKER_F("\":");
                cdData += STRING_format((millis() - _cdStart) / 1000 / 60);
                cdData += BLINKER_F(",\"");
                cdData += BLINKER_F(BLINKER_CMD_ACTION);
                cdData += BLINKER_F("\":");
                cdData += _cdAction;
                cdData += BLINKER_F("}}");
            }
            else
            {
                cdData = BLINKER_F("{\"");
                cdData += BLINKER_F(BLINKER_CMD_COUNTDOWN);
                cdData += BLINKER_F("\":{\"");
                cdData += BLINKER_F(BLINKER_CMD_RUN);
                cdData += BLINKER_F("\":");
                cdData += STRING_format(_cdRunState ? 1 : 0);
                cdData += BLINKER_F(",\"");
                cdData += BLINKER_F(BLINKER_CMD_TOTALTIME);
                cdData += BLINKER_F("\":");
                cdData += STRING_format(_cdTime1);
                cdData += BLINKER_F(",\"");
                cdData += BLINKER_F(BLINKER_CMD_RUNTIME);
                cdData += BLINKER_F("\":");
                cdData += STRING_format(_cdTime2);
                cdData += BLINKER_F(",\"");
                cdData += BLINKER_F(BLINKER_CMD_ACTION);
                cdData += BLINKER_F("\":");
                cdData += _cdAction;
                cdData += BLINKER_F("}}");
            }
        }

        return cdData;
    }

    template <class Proto>
    String BlinkerApi<Proto>::timingConfig()
    {
        String timingTaskStr = BLINKER_F("{\"");
        timingTaskStr += BLINKER_F(BLINKER_CMD_TIMING);
        timingTaskStr += BLINKER_F("\":[");

        for (uint8_t task = 0; task < taskCount; task++)
        {
            //Serial.print(timingTask[task].getTimingCfg());
            timingTaskStr += getTimingCfg(task);
            if (task + 1 < taskCount)
            {
                //Serial.println(",");
                timingTaskStr += BLINKER_F(",");
            }
            // else {
            //     Serial.println("");
            // }
        }
        timingTaskStr += BLINKER_F("]}");

        BLINKER_LOG_ALL(BLINKER_F("timingTaskStr: "), timingTaskStr);

        return timingTaskStr;
    }

    template <class Proto>
    String BlinkerApi<Proto>::getTimingCfg(uint8_t task)
    {
        String timingDayStr = BLINKER_F("");
        uint8_t timingDay = timingTask[task]->getTimingday();
        if (timingTask[task]->isLoop())
        {
            for (uint8_t day = 0; day < 7; day++)
            {
                // timingDayStr += (timingDay & (uint8_t)pow(2,day)) ? String(day):String("");
                if ((timingDay >> day) & 0x01) {
                    timingDayStr += STRING_format(1);
                    // if (day < 6 && (timingDay >> (day + 1)))
                    //     timingDayStr += STRING_format(",");
                }
                else {
                    timingDayStr += STRING_format(0);
                }
                // timingDayStr += String((day < 6) ? ((timingDay >> (day + 1)) ? ",":""):"");
            }

            BLINKER_LOG_ALL(BLINKER_F("timingDayStr: "), timingDayStr);

        }
        else {
            timingDayStr = BLINKER_F("0000000");

            BLINKER_LOG_ALL(BLINKER_F("timingDayStr: "), timingDay);
        }

        String timingConfig = BLINKER_F("{\"");
        timingConfig += BLINKER_F(BLINKER_CMD_TASK);
        timingConfig += BLINKER_F("\":");
        timingConfig += STRING_format(task);
        timingConfig += BLINKER_F(",\"");
        timingConfig += BLINKER_F(BLINKER_CMD_ENABLE);
        timingConfig += BLINKER_F("\":");
        timingConfig += STRING_format((timingTask[task]->state()) ? 1 : 0);
        timingConfig += BLINKER_F(",\"");
        timingConfig += BLINKER_F(BLINKER_CMD_DAY);
        timingConfig += BLINKER_F("\":\"");
        timingConfig += timingDayStr;
        timingConfig += BLINKER_F("\",\"");
        timingConfig += BLINKER_F(BLINKER_CMD_TIME);
        timingConfig += BLINKER_F("\":");
        timingConfig += STRING_format(timingTask[task]->getTime());
        timingConfig += BLINKER_F(",\"");
        timingConfig += BLINKER_F(BLINKER_CMD_ACTION);
        timingConfig += BLINKER_F("\":");
        timingConfig += timingTask[task]->getAction();
        timingConfig += BLINKER_F("}");

        return timingConfig;
    }

    template <class Proto>
    String BlinkerApi<Proto>::loopConfig()
    {
        String lpData;
        if (!_lpState) {
            lpData = BLINKER_F("{\"");
            lpData += BLINKER_F(BLINKER_CMD_LOOP);
            lpData += BLINKER_F("\":false}");
        }
        else {
            lpData = BLINKER_F("{\"");
            lpData += BLINKER_F(BLINKER_CMD_LOOP);
            lpData += BLINKER_F("\":{\"");
            lpData += BLINKER_F(BLINKER_CMD_TIMES);
            lpData += BLINKER_F("\":");
            lpData += STRING_format(_lpTimes);
            lpData += BLINKER_F(",\"");
            lpData += BLINKER_F(BLINKER_CMD_RUN);
            lpData += BLINKER_F("\":");
            lpData += STRING_format(_lpRunState ? 1 : 0);
            lpData += BLINKER_F(",\"");
            lpData += BLINKER_F(BLINKER_CMD_TRIGGED);
            lpData += BLINKER_F("\":");
            lpData += STRING_format(_lpTimes ? _lpTrigged_times : 0);
            lpData += BLINKER_F(",\"");
            lpData += BLINKER_F(BLINKER_CMD_TIME1);
            lpData += BLINKER_F("\":");
            lpData += STRING_format(_lpTime1);
            lpData += BLINKER_F(",\"");
            lpData += BLINKER_F(BLINKER_CMD_ACTION1);
            lpData += BLINKER_F("\":");
            lpData += _lpAction1;
            lpData += BLINKER_F(",\"");
            lpData += BLINKER_F(BLINKER_CMD_TIME2);
            lpData += BLINKER_F("\":");
            lpData += STRING_format(_lpTime2);
            lpData += BLINKER_F(",\"");
            lpData += BLINKER_F(BLINKER_CMD_ACTION2);
            lpData += BLINKER_F("\":");
            lpData += _lpAction2;
            lpData += BLINKER_F("}}");
        }

        return lpData;
    }

    template <class Proto>
    bool BlinkerApi<Proto>::timerManager(const JsonObject& data, bool _noSet)
    {
        bool isSet = false;
        bool isCount = false;
        bool isLoop = false;
        bool isTiming = false;

        if (!_noSet)
        {
            isSet = STRING_contains_string(static_cast<Proto*>(this)->lastRead(), BLINKER_CMD_SET);
            isCount = STRING_contains_string(static_cast<Proto*>(this)->lastRead(), BLINKER_CMD_COUNTDOWN);
            isLoop = STRING_contains_string(static_cast<Proto*>(this)->lastRead(), BLINKER_CMD_LOOP);
            isTiming = STRING_contains_string(static_cast<Proto*>(this)->lastRead(), BLINKER_CMD_TIMING);
        }
        else {
            isCount = data.containsKey(BLINKER_CMD_COUNTDOWN);
            isLoop = data.containsKey(BLINKER_CMD_LOOP);
            isTiming = data.containsKey(BLINKER_CMD_TIMING);
        }

        if ((isSet || _noSet) && (isCount || isLoop || isTiming))
        {
            BLINKER_LOG_ALL(BLINKER_F("timerManager isParsed"));
            _fresh = true;

            BLINKER_LOG_ALL(BLINKER_F("get timer setting"));

            if (isCount)
            {

                String _delete = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN];

                if (_delete == "dlt") _cdState = false;
                else _cdState = true;

                if (_cdState)
                {
                    if (isSet)
                    {
                        _cdRunState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUN];
                    }
                    else if(_noSet)
                    {
                        _cdRunState = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUN];
                    }

                    BLINKER_LOG_ALL(BLINKER_F("countdown state: "), _cdState ? "true" : "false");

                    if (isSet)
                    {
                        // _cdRunState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_STATE];
                        // _cdRunState = _cdState;
                        int32_t _totalTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_TOTALTIME];
                        // _totalTime = 60 * _totalTime;
                        int32_t _runTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUNTIME];
                        // _runTime = 60 * _runTime;
                        String _action = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_ACTION];

                        if (_action.length() > BLINKER_TIMER_COUNTDOWN_ACTION_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        // BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION: "), _action , BLINKER_F(", LEN: "), _action.length());

                        if (_cdRunState && _action != "null")
                        {
                            // _cdAction = _action;
                            strcpy(_cdAction, _action.c_str());
                            _cdTime1 = _totalTime;
                            _cdTime2 = _runTime;
                        }

                        if (!_cdRunState && _action == "null")
                        {
                            _cdTime2 += (millis() - _cdStart) / 1000 / 60;
                        }
                        // else if (_cdRunState && _action.length() == 0) {
                        //     _cdTime2 = 0;
                        // }

                        BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
                    }
                    else if (_noSet)
                    {
                        // _cdRunState = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_STATE];
                        // _cdRunState = _cdState;
                        int32_t _totalTime = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_TOTALTIME];
                        // _totalTime = 60 * _totalTime;
                        int32_t _runTime = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUNTIME];
                        // _runTime = 60 * _runTime;
                        String _action = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_ACTION];

                        if (_action.length() > BLINKER_TIMER_COUNTDOWN_ACTION_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        if (_cdRunState && _action != "null")
                        {
                            // _cdAction = _action;
                            strcpy(_cdAction, _action.c_str());
                            _cdTime1 = _totalTime;
                            _cdTime2 = _runTime;
                        }

                        if (!_cdRunState && _action == "null")
                        {
                            _cdTime2 += (millis() - _cdStart) / 1000 / 60;
                        }
                        // else if (_cdRunState && _action.length() == 0) {
                        //     _cdTime2 = 0;
                        // }

                        BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
                    }
                    _cdData = _cdState << 31 | _cdRunState << 30 | _cdTime1 << 12 | _cdTime2;

                    BLINKER_LOG_ALL(BLINKER_F("_totalTime: "), _cdTime1);
                    BLINKER_LOG_ALL(BLINKER_F("_runTime: "), _cdTime2);
                    BLINKER_LOG_ALL(BLINKER_F("_action: "), _cdAction);
                    BLINKER_LOG_ALL(BLINKER_F("_cdData: "), _cdData);

                    // char _cdAction_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
                    // strcpy(_cdAction_, _cdAction.c_str());

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction_);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction);
                    EEPROM.commit();
                    EEPROM.end();

                    if (_cdState && _cdRunState)
                    {
                        // _cdTime1 = _cdTime1 - _cdTime2;
                        // _cdTime2 = 0;

                        uint32_t _cdTime1_ = _cdTime1 - _cdTime2;

                        if (_cdTime1_ * 60 > BLINKER_ONE_HOUR_TIME) _cdTime1_ = BLINKER_ONE_HOUR_TIME;
                        else _cdTime1_ = _cdTime1_ * 60;

                        cdTicker.once(_cdTime1_, _cd_callback);

                        _cdStart = millis();

                        BLINKER_LOG_ALL(BLINKER_F("countdown start! time: "), _cdTime1);
                    }
                    else
                    {
                        cdTicker.detach();
                    }
                }
                else {
                    _cdRunState = 0;
                    _cdTime1 = 0;
                    _cdTime2 = 0;
                    // _cdAction = "";
                    // memcpy(_cdAction, '\0', BLINKER_ACTION_SIZE);

                    _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);

                    BLINKER_LOG_ALL(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
                    BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
                    BLINKER_LOG_ALL(BLINKER_F("_totalTime: "), _cdTime1);
                    BLINKER_LOG_ALL(BLINKER_F("_runTime: "), _cdTime2);
                    BLINKER_LOG_ALL(BLINKER_F("_action: "), _cdAction);
                    BLINKER_LOG_ALL(BLINKER_F("_cdData: "), _cdData);

                    // char _cdAction_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
                    // strcpy(_cdAction_, _cdAction.c_str());

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction_);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction);
                    EEPROM.commit();
                    EEPROM.end();

                    cdTicker.detach();
                }

                // static_cast<Proto*>(this)->checkState(false);
                static_cast<Proto*>(this)->_timerPrint(countdownConfig());
                static_cast<Proto*>(this)->printNow();
                return true;
            }
            else if (isLoop)
            {
                _lpState = true;

                String _delete = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP];

                if (_delete == "dlt") _lpState = false;
                else _lpState = true;

                if (_lpState)
                {
                    if (isSet)
                    {
                        _lpRunState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_RUN];
                    }
                    else if (_noSet)
                    {
                        _lpRunState = data[BLINKER_CMD_LOOP][BLINKER_CMD_RUN];
                    }

                    BLINKER_LOG_ALL(BLINKER_F("loop state: "), _lpState ? "true" : "false");

                    if (isSet)
                    {
                        int8_t _times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIMES];
                        int8_t _tri_times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TRIGGED];
                        // _lpRunState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_STATE];
                        // _lpRunState = _lpState;
                        int32_t _time1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIME1];
                        // _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIME2];
                        // _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_ACTION2];

                        if (_action1.length() > BLINKER_TIMER_LOOP_ACTION1_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        if (_action2.length() > BLINKER_TIMER_LOOP_ACTION2_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        if (_lpRunState && _action2 != "null")
                        {
                            // _lpAction1 = _action1;
                            // _lpAction2 = _action2;
                            strcpy(_lpAction1, _action1.c_str());
                            strcpy(_lpAction2, _action2.c_str());

                            _lpTimes = _times;
                            _lpTrigged_times = _tri_times;
                            _lpTime1 = _time1;
                            _lpTime2 = _time2;
                        }

                        BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);

                    }
                    else if (_noSet)
                    {
                        int8_t _times = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIMES];
                        int8_t _tri_times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TRIGGED];
                        // _lpRunState = data[BLINKER_CMD_LOOP][BLINKER_CMD_STATE];
                        // _lpRunState = _lpState;
                        int32_t _time1 = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIME1];
                        // _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_LOOP][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIME2];
                        // _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_LOOP][BLINKER_CMD_ACTION2];

                        if (_action1.length() > BLINKER_TIMER_LOOP_ACTION1_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        if (_action2.length() > BLINKER_TIMER_LOOP_ACTION2_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        if (_lpRunState && _action2 != "null") {
                            // _lpAction1 = _action1;
                            // _lpAction2 = _action2;
                            strcpy(_lpAction1, _action1.c_str());
                            strcpy(_lpAction2, _action2.c_str());

                            _lpTimes = _times;
                            _lpTrigged_times = _tri_times;
                            _lpTime1 = _time1;
                            _lpTime2 = _time2;
                        }

                        BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);
                    }

                    if (_lpTimes > 100) _lpTimes = 0;

                    _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;

                    BLINKER_LOG_ALL(BLINKER_F("_times: "), _lpTimes);
                    BLINKER_LOG_ALL(BLINKER_F("_tri_times: "), _lpTrigged_times);
                    BLINKER_LOG_ALL(BLINKER_F("_time1: "), _lpTime1);
                    BLINKER_LOG_ALL(BLINKER_F("_action1: "), _lpAction1);
                    BLINKER_LOG_ALL(BLINKER_F("_time2: "), _lpTime2);
                    BLINKER_LOG_ALL(BLINKER_F("_action2: "), _lpAction2);
                    BLINKER_LOG_ALL(BLINKER_F("_lpData: "), _lpData);

                    // char _lpAction_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
                    // char _lpAction_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
                    // strcpy(_lpAction_1, _lpAction1.c_str());
                    // strcpy(_lpAction_2, _lpAction2.c_str());

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_TRI, _lpTrigged_times);
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction_1);
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction_2);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction1);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction2);
                    EEPROM.commit();
                    EEPROM.end();

                    if (_lpState && _lpRunState)
                    {
                        _lpRun1 = true;
                        // _lpTrigged_times = 0;
                        _lpStop = false;

                        uint32_t _lpTime1_;

                        if (_lpTime1 * 60 > BLINKER_ONE_HOUR_TIME) _lpTime1_ = BLINKER_ONE_HOUR_TIME;
                        else _lpTime1_ = _lpTime1 * 60;

                        _lpTime1_start = millis();
                        lpTicker.once(_lpTime1_, _lp_callback);

                        BLINKER_LOG_ALL(BLINKER_F("loop start!"));
                    }
                    else
                    {
                        lpTicker.detach();
                    }
                }
                else
                {
                    _lpRunState = 0;
                    _lpTimes = 0;
                    _lpTrigged_times = 0;
                    _lpTime1 = 0;
                    _lpTime2 = 0;
                    // _lpAction1 = "";
                    // _lpAction2 = "";
                    // memcpy(_lpAction1, '\0', BLINKER_ACTION_SIZE);
                    // memcpy(_lpAction2, '\0', BLINKER_ACTION_SIZE);

                    _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;

                    BLINKER_LOG_ALL(BLINKER_F("loop state: "), _lpState ? "true" : "false");
                    BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);
                    BLINKER_LOG_ALL(BLINKER_F("_times: "), _lpTimes);
                    BLINKER_LOG_ALL(BLINKER_F("_tri_times: "), _lpTrigged_times);
                    BLINKER_LOG_ALL(BLINKER_F("_time1: "), _lpTime1);
                    BLINKER_LOG_ALL(BLINKER_F("_action1: "), _lpAction1);
                    BLINKER_LOG_ALL(BLINKER_F("_time2: "), _lpTime2);
                    BLINKER_LOG_ALL(BLINKER_F("_action2: "), _lpAction2);
                    BLINKER_LOG_ALL(BLINKER_F("_lpData: "), _lpData);

                    // char _lpAction_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
                    // char _lpAction_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
                    // strcpy(_lpAction_1, _lpAction1.c_str());
                    // strcpy(_lpAction_2, _lpAction2.c_str());

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_TRI, _lpTrigged_times);
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction_1);
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction_2);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction1);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction2);
                    EEPROM.commit();
                    EEPROM.end();

                    lpTicker.detach();
                }

                static_cast<Proto*>(this)->_timerPrint(loopConfig());
                static_cast<Proto*>(this)->printNow();
                return true;
            }
            else if (isTiming)
            {
                bool isDelet = STRING_contains_string(static_cast<Proto*>(this)->lastRead(), BLINKER_CMD_DELETETASK);

                _tmState = true;

                if (isSet)
                {
                    _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_ENABLE];
                }
                else if (_noSet)
                {
                    _tmRunState = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_ENABLE];
                }

                BLINKER_LOG_ALL(BLINKER_F("timing state: "), _tmState ? "true" : "false");

                int32_t _time;
                String _action;
                String _text;
                uint8_t _task;

                if (isSet && !isDelet)
                {
                    // _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_STATE];
                    // _tmRunState = _tmState;
                    _time = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TIME];
                    // _time = 60 * _time;
                    String tm_action = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_ACTION];
                    // String tm_text = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TEXT];
                    _task = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TASK];

                    // _tmAction = _action;

                    if (tm_action.length() > BLINKER_TIMER_TIMING_ACTION_SIZE)
                    {
                        BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                        return true;
                    }

                    _action = tm_action;
                    // _text = tm_text;

                    // _tmTime = _time;

                    String tm_day = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY];

                    _timingDay = 0;

                    if (tm_day.toInt() == 0)
                    {
                        if (60 * _time > static_cast<Proto*>(this)->dtime())
                        {
                            _timingDay |= (0x01 << static_cast<Proto*>(this)->wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
                        }
                        else {
                            _timingDay |= (0x01 << ((static_cast<Proto*>(this)->wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
                        }

                        _isTimingLoop = false;

                        BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                    }
                    else
                    {

                        _isTimingLoop = true;

                        // uint8_t taskDay;

                        for (uint8_t day = 0; day < 7; day++)
                        {
                            if (tm_day.substring(day, day+1) == "1")
                            {
                                _timingDay |= (0x01 << day);

                                BLINKER_LOG_ALL(BLINKER_F("day: "), day, BLINKER_F(" timingDay: "), _timingDay);
                            }
                        }
                    }

                    BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                    // BLINKER_LOG_ALL(BLINKER_F("_text: "), _text);
                    BLINKER_LOG_ALL(BLINKER_F("_tmRunState: "), _tmRunState);
                    BLINKER_LOG_ALL(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                    BLINKER_LOG_ALL(BLINKER_F("_time: "), _time);
                    BLINKER_LOG_ALL(BLINKER_F("_action: "), _action);

                    uint32_t _timerData = _isTimingLoop << 31 | _tmRunState << 23 | _timingDay << 11 | _time;

                    // addTimingTask(_task, _timerData, _action, _text);
                    addTimingTask(_task, _timerData, _action);
                }
                else if (_noSet)
                {
                    // _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_STATE];
                    // _tmRunState = _tmState;
                    _time = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_TIME];
                    // _time = 60 * _time;
                    String tm_action = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_ACTION];
                    // String tm_text = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TEXT];
                    _task = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_TASK];

                    // _tmAction = _action;

                    if (tm_action.length() > BLINKER_TIMER_TIMING_ACTION_SIZE)
                    {
                        BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                        return true;
                    }

                    _action = tm_action;
                    // _text = tm_text;

                    // _tmTime = _time;

                    String tm_day = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY];

                    if (tm_day.toInt() == 0)
                    {
                        if (60 * _time > static_cast<Proto*>(this)->dtime())
                        {
                            _timingDay |= (0x01 << static_cast<Proto*>(this)->wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
                        }
                        else {
                            _timingDay |= (0x01 << ((static_cast<Proto*>(this)->wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
                        }

                        _isTimingLoop = false;

                        BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                    }
                    else
                    {
                        // uint8_t taskDay;

                        _isTimingLoop = true;

                        for (uint8_t day = 0; day < 7; day++)
                        {
                            if (tm_day.substring(day, day+1) == "1")
                            {
                                _timingDay |= (0x01 << day);

                                BLINKER_LOG_ALL(BLINKER_F("day: "), day, BLINKER_F(" timingDay: "), _timingDay);
                            }
                        }
                    }

                    BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                    // BLINKER_LOG_ALL(BLINKER_F("_text: "), _text);
                    BLINKER_LOG_ALL(BLINKER_F("_tmRunState: "), _tmRunState);
                    BLINKER_LOG_ALL(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                    BLINKER_LOG_ALL(BLINKER_F("_time: "), _time);
                    BLINKER_LOG_ALL(BLINKER_F("_action: "), _action);

                    uint32_t _timerData = _isTimingLoop << 31 | _tmRunState << 23 | _timingDay << 11 | _time;

                    // addTimingTask(_task, _timerData, _action, _text);
                    addTimingTask(_task, _timerData, _action);
                }
                else if (isDelet)
                {
                    uint8_t _delTask = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DELETETASK];

                    deleteTiming(_delTask);
                }

                char _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];

                EEPROM.begin(BLINKER_EEP_SIZE);
                EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
                for(uint8_t task = 0; task < taskCount; task++)
                {
                    strcpy(_tmAction_, timingTask[task]->getAction());

                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE
                                , timingTask[task]->getTimerData());
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE +
                                BLINKER_TIMER_TIMING_SIZE, _tmAction_);

                    BLINKER_LOG_ALL(BLINKER_F("getTimerData: "), timingTask[task]->getTimerData());
                    BLINKER_LOG_ALL(BLINKER_F("_tmAction_: "), _tmAction_);
                }
                EEPROM.commit();
                EEPROM.end();

                static_cast<Proto*>(this)->_timerPrint(timingConfig());
                static_cast<Proto*>(this)->printNow();

                BLINKER_LOG_FreeHeap_ALL();

                return true;
            }
        }
        else if (data.containsKey(BLINKER_CMD_GET))
        {
            String get_timer = data[BLINKER_CMD_GET];

            if (get_timer == BLINKER_CMD_TIMER)
            {
                static_cast<Proto*>(this)->_timerPrint(timerSetting());
                static_cast<Proto*>(this)->printNow();
                BLINKER_LOG_ALL(BLINKER_F("timerManager1 isParsed"));
                _fresh = true;
                return true;
            }
            else if (get_timer == BLINKER_CMD_COUNTDOWN)
            {
                static_cast<Proto*>(this)->_timerPrint(countdownConfig());
                static_cast<Proto*>(this)->printNow();
                BLINKER_LOG_ALL(BLINKER_F("timerManager2 isParsed"));
                _fresh = true;
                return true;
            }
            else if (get_timer == BLINKER_CMD_LOOP)
            {
                static_cast<Proto*>(this)->_timerPrint(loopConfig());
                static_cast<Proto*>(this)->printNow();
                BLINKER_LOG_ALL(BLINKER_F("timerManager3 isParsed"));
                _fresh = true;
                return true;
            }
            else if (get_timer == BLINKER_CMD_TIMING)
            {
                static_cast<Proto*>(this)->_timerPrint(timingConfig());
                static_cast<Proto*>(this)->printNow();
                BLINKER_LOG_ALL(BLINKER_F("timerManager4 isParsed"));
                _fresh = true;
                return true;
            }
        }
        else
        {
            return false;
        }

        return false;
    }

    template <class Proto>
    bool BlinkerApi<Proto>::checkTimer()
    {
        if (_cdTrigged)
        {
            _cdTrigged = false;

            // _cdRunState = false;
            _cdState = false;
            // _cdData |= _cdRunState << 14;
            // _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);
            _cdData = _cdState << 31 | _cdRunState << 30 | _cdTime1 << 12 | _cdTime2;
            saveCountDown(_cdData, _cdAction);

            BLINKER_LOG_ALL(BLINKER_F("countdown trigged, action is: "), _cdAction);

            // _parse(_cdAction);

            // #if defined(BLINKER_AT_MQTT)
            //     static_cast<Proto*>(this)->serialPrint(_cdAction);
            // #else
                parse(_cdAction, true);
            // #endif
        }
        if (_lpTrigged)
        {
            _lpTrigged = false;

            if (_lpStop)
            {
                // _lpRunState = false;
                _lpState = false;
                // _lpData |= _lpRunState << 30;
                _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;
                saveLoop(_lpData, _lpAction1, _lpAction2);
            }

            if (_lpRun1)
            {
                BLINKER_LOG_ALL(BLINKER_F("loop trigged, action is: "), _lpAction2);
                // _parse(_lpAction2);

                // #if defined(BLINKER_AT_MQTT)
                //     static_cast<Proto*>(this)->serialPrint(_lpAction2);
                // #else
                    parse(_lpAction2, true);
                // #endif
            }
            else
            {

                BLINKER_LOG_ALL(BLINKER_F("loop trigged, action is: "), _lpAction1);
                // _parse(_lpAction1);

                // #if defined(BLINKER_AT_MQTT)
                //     static_cast<Proto*>(this)->serialPrint(_lpAction1);
                // #else
                    parse(_lpAction1, true);
                // #endif
            }
        }
        if (_tmTrigged)
        {
            _tmTrigged = false;

    //             if (_tmRun1) {
    // #ifdef BLINKER_DEBUG_ALL
    //                 BLINKER_LOG(BLINKER_F("timing trigged, action is: "), _tmAction2);
    // #endif
    //                 // _parse(_tmAction2);
    //                 parse(_tmAction2, true);
    //             }
    //             else {
    // #ifdef BLINKER_DEBUG_ALL
    //                 BLINKER_LOG(BLINKER_F("timing trigged, action is: "), _tmAction1);
    // #endif
    //                 // _parse(_tmAction1);
    //                 parse(_tmAction1, true);
    //             }

            uint8_t wDay =  static_cast<Proto*>(this)->wday();

            BLINKER_LOG_ALL(static_cast<Proto*>(this)->hour(), ":", 
                            static_cast<Proto*>(this)->minute(), ":", 
                            static_cast<Proto*>(this)->second());

            uint16_t nowMins = static_cast<Proto*>(this)->hour() * 60 + static_cast<Proto*>(this)->minute();

            if (triggedTask < BLINKER_TIMING_TIMER_SIZE && \
                nowMins != timingTask[triggedTask]->getTime())
            {
                BLINKER_LOG_ALL(BLINKER_F("timing trigged, now minutes check error!"));

                freshTiming(wDay, nowMins);

                return false;
            }

            if (triggedTask < BLINKER_TIMING_TIMER_SIZE)
            {
                // String _tmAction = timingTask[triggedTask]->getAction();
                char _tmAction[BLINKER_TIMER_TIMING_ACTION_SIZE];

                strcpy(_tmAction, timingTask[triggedTask]->getAction());

                BLINKER_LOG(BLINKER_F("timing trigged, action is: "), _tmAction);

                // #if defined(BLINKER_AT_MQTT)
                //     static_cast<Proto*>(this)->serialPrint(_tmAction);
                // #else
                    parse(_tmAction, true);
                // #endif

                checkOverlapping(wDay, timingTask[triggedTask]->getTime(), triggedTask);

                freshTiming(wDay, timingTask[triggedTask]->getTime());

                return true;
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("timing trigged, none action"));

                freshTiming(wDay, 0);

                return false;
            }
        }

        return false;
    }

    template <class Proto>
    void BlinkerApi<Proto>::loadTimer()
    {
        BLINKER_LOG(BLINKER_F(
            "\n==========================================================="
            "\n================== Blinker Timer loaded! =================="
            "\n    EEPROM address 1536-2431 is used for Blinker Timer!"
            "\n========= PLEASE AVOID USING THESE EEPROM ADDRESS! ========"
            "\n===========================================================\n"));

        checkTimerErase();
        loadCountdown();
        loadLoop();
    }

    template <class Proto>
    void BlinkerApi<Proto>::autoRun()
    {
        for (uint8_t _num = 0; _num < _aCount; _num++)
        {
            if (_AUTO[_num]->isTrigged())
            {
                if (static_cast<Proto*>(this)->conn.autoPrint(_AUTO[_num]->id()))
                {
                    static_cast<Proto*>(this)->run();

                    BLINKER_LOG_ALL(BLINKER_F("trigged sucessed"));

                    _AUTO[_num]->fresh();
                }
                else
                {
                    BLINKER_LOG_ALL(BLINKER_F("trigged failed"));
                }

                static_cast<Proto*>(this)->run();
            }
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::autoInput(const String & key, float data)
    {
        // if (!_isNTPInit) return;

        int32_t nowTime = static_cast<Proto*>(this)->dtime()/60;

        if (nowTime == -1) return;

        for (uint8_t _num = 0; _num < _aCount; _num++)
        {
            _AUTO[_num]->run(key, data, nowTime);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::beginAuto()
    {
        BLINKER_LOG(BLINKER_F("======================================================="));
        BLINKER_LOG(BLINKER_F("=========== Blinker Auto Control mode init! ==========="));
        BLINKER_LOG(BLINKER_F("    EEPROM address 0-1279 is used for Auto Control!"));
        BLINKER_LOG(BLINKER_F("======= PLEASE AVOID USING THESE EEPROM ADDRESS! ======"));
        BLINKER_LOG(BLINKER_F("======================================================="));

        // BLINKER_LOG(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

        // _isAuto = true;
        // deserialization();
        autoStart();

        // autoInit(); //TODO
    }

    template <class Proto>
    void BlinkerApi<Proto>::autoStart()
    {
        // autoPull();

        BLINKER_LOG_ALL(BLINKER_F("_______autoStart_______"));
        uint8_t checkData;

        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);
        if (checkData != BLINKER_CHECK_DATA)
        {
            for (uint16_t _addr = BLINKER_EEP_ADDR_AUTO_START;
                _addr < BLINKER_EEP_ADDR_AUTO_START +
                BLINKER_ONE_AUTO_DATA_SIZE * 2; _addr++)
            {
                EEPROM.put(_addr, "\0");
            }
            EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
            EEPROM.commit();
            EEPROM.end();
            return;
        }
        EEPROM.get(BLINKER_EEP_ADDR_AUTONUM, _aCount);
        if (_aCount > 2)
        {
            _aCount = 0;
            EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
        }
        EEPROM.commit();
        EEPROM.end();

        BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

        if (_aCount)
        {
            for (uint8_t _num = 0; _num < _aCount; _num++)
            {
                BLINKER_LOG_ALL(BLINKER_F("new BlinkerAUTO() _num: "), _num);

                _AUTO[_num] = new BlinkerAUTO();
                _AUTO[_num]->setNum(_num);
                _AUTO[_num]->deserialization();
            }
        }
    }

    template <class Proto>
    bool BlinkerApi<Proto>::autoManager(const JsonObject& data)
    {
        // String set;
        bool isSet = false;
        bool isAuto = false;

        // isSet = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET);
        // isAuto = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTO);
        isSet = data.containsKey(BLINKER_CMD_SET);
        String aData = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO].as<String>();
        // const char* aDataArray = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][0];

        // if (aData.length()) isAuto = true;
        if (aData != "null") isAuto = true;

        BLINKER_LOG_ALL(BLINKER_F("autoManager begin: "), isAuto, " ", isSet);

        // if (aDataArray && !isAuto)
        if (!isAuto)
        {
            for (uint8_t num = 0; num < 2; num++)
            {
                unsigned long _autoId = data[BLINKER_CMD_AUTO][num][BLINKER_CMD_AUTOID];
                String arrayData = data[BLINKER_CMD_AUTO][num];

                if (_aCount)
                {
                    for (uint8_t _num = 0; _num < _aCount; _num++)
                    {
                        if (_AUTO[_num]->id() == _autoId) {
                            // _AUTO[_num]->manager(arrayData);
                            return true;
                        }
                    }
                    if (_aCount == 1)
                    {
                        _AUTO[_aCount] = new BlinkerAUTO();
                        _AUTO[_aCount]->setNum(_aCount);
                        // _AUTO[_aCount]->manager(arrayData);

                        // _aCount = 1;
                        _aCount++;
                        EEPROM.begin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                        EEPROM.commit();
                        EEPROM.end();

                        BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);
                        // static_cast<Proto*>(this)->_print(autoData(), false);
                        // return true;
                    }
                    else
                    {
                        // _AUTO[_aCount - 1]->manager(arrayData);
                        // return true;
                    }
                }
                else
                {
                    _AUTO[_aCount] = new BlinkerAUTO();
                    _AUTO[_aCount]->setNum(_aCount);
                    // _AUTO[_aCount]->manager(arrayData);

                    _aCount = 1;
                    // _aCount++;
                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                    EEPROM.commit();
                    EEPROM.end();

                    BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);
                    // static_cast<Proto*>(this)->_print(autoData(), false);
                    // return true;
                }
            }
            return true;
        }
        else if (isSet && isAuto)
        {
            // BLINKER_LOG_ALL(BLINKER_F("timerManager5 isParsed"));
            _fresh = true;

            BLINKER_LOG_ALL(BLINKER_F("get auto setting"));

            // bool isDelet = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_DELETID);
            String isTriggedArray = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                        [BLINKER_CMD_ACTION][0];

            if (isTriggedArray != "null")
            {
                BLINKER_LOG_ALL(BLINKER_F("_auto trigged action: "), isTriggedArray);

                for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
                {
                    String _autoData_array = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                                [BLINKER_CMD_ACTION][a_num];

                    if(_autoData_array != "null")
                    {
                        _fresh = false;
                        // DynamicJsonBuffer _jsonBuffer;
                        // JsonObject& _array = _jsonBuffer.parseObject(_autoData_array);
                        DynamicJsonDocument jsonBuffer(BLINKER_JSON_BUFFER_SIZE);
                        deserializeJson(jsonBuffer, _autoData_array);
                        JsonObject _array = jsonBuffer.as<JsonObject>();

                        #if defined(BLINKER_WIDGET)
                        #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP)
                            timerManager(_array);
                        #endif

                            widgetsParse(_array);
                        #endif

                        if (_fresh)
                        {
                            static_cast<Proto*>(this)->flush();
                        }
                        else
                        {
                            #if defined(BLINKER_PRO) || defined(BLINKER_MQTT_AUTO) || \
                                defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY)
                                // if (_parseFunc) {
                                //     if(_parseFunc(_array)) {
                                //         BLINKER_LOG_ALL(BLINKER_F("_parseFunc(_array) isParsed"));
                                //         _fresh = true;
                                //         static_cast<Proto*>(this)->flush();
                                //     }

                                //     BLINKER_LOG_ALL(BLINKER_F("run parse callback function"));
                                // }
                            #endif
                        }
                    }
                    else
                    {
                        // a_num = BLINKER_MAX_WIDGET_SIZE;
                        return true;
                    }
                }
            }
            else
            {
                // uint32_t _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);
                String get_autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_ID].as<String>();
                String del_autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DELETE].as<String>();
                // _autoId = get_autoId.toInt();

                BLINKER_LOG_ALL(BLINKER_F("get_autoId: "), strtoul(get_autoId.c_str(),NULL,10));
                BLINKER_LOG_ALL(BLINKER_F("del_autoId: "), strtoul(del_autoId.c_str(),NULL,10));
                BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);
                // _aCount = 0;

                if (_aCount && strtoul(del_autoId.c_str(),NULL,10) == 0)
                {
                    for (uint8_t _num = 0; _num < _aCount; _num++)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("check _autoId: "), _AUTO[_num]->id(), " ", _AUTO[_num]->id() == strtoul(get_autoId.c_str(),NULL,10));
                        if (_AUTO[_num]->id() == strtoul(get_autoId.c_str(),NULL,10))
                        {
                            _AUTO[_num]->manager(data);
                            return true;
                        }
                    }
                    if (_aCount == 1)
                    {
                        _AUTO[_aCount] = new BlinkerAUTO();
                        _AUTO[_aCount]->setNum(_aCount);
                        _AUTO[_aCount]->manager(data);

                        // _aCount = 1;
                        _aCount++;
                        EEPROM.begin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                        EEPROM.commit();
                        EEPROM.end();

                        BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

                        // static_cast<Proto*>(this)->_print(autoData(), false);
                        // return true;
                    }
                    else
                    {
                        _AUTO[0]->setNum(1);
                        _AUTO[0]->deserialization();
                        _AUTO[0]->setNum(0);
                        _AUTO[0]->serialization();

                        _AUTO[1] = new BlinkerAUTO();
                        _AUTO[1]->setNum(1);
                        _AUTO[1]->manager(data);
                        // return true;
                    }
                }
                else if (_aCount == 0 && strtoul(del_autoId.c_str(),NULL,10) == 0)
                {
                    _AUTO[_aCount] = new BlinkerAUTO();
                    _AUTO[_aCount]->setNum(_aCount);
                    _AUTO[_aCount]->manager(data);

                    _aCount = 1;
                    // _aCount++;
                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                    EEPROM.commit();
                    EEPROM.end();

                    BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

                    // static_cast<Proto*>(this)->_print(autoData(), false);
                    // return true;
                }
                else if (strtoul(del_autoId.c_str(),NULL,10) != 0)
                {
                    for (uint8_t _num = 0; _num < _aCount; _num++)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("check _autoId: "), _AUTO[_num]->id(), " ", _AUTO[_num]->id() == strtoul(del_autoId.c_str(),NULL,10));
                        if (_AUTO[_num]->id() == strtoul(del_autoId.c_str(),NULL,10))
                        {
                            if (_num == 0) 
                            {
                                _AUTO[1]->setNum(0);
                                _AUTO[1]->serialization();

                                _AUTO[0]->deserialization();

                                free(_AUTO[1]);
                                
                                _aCount--;
                                EEPROM.begin(BLINKER_EEP_SIZE);
                                EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                                EEPROM.commit();
                                EEPROM.end();
                            }
                            else
                            {
                                free(_AUTO[1]);
                                _aCount--;
                                EEPROM.begin(BLINKER_EEP_SIZE);
                                EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                                EEPROM.commit();
                                EEPROM.end();
                            }
                            
                        }
                    }
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    #if defined(BLINKER_PRO_ESP)
        template <class Proto>
        void BlinkerApi<Proto>::otaParse(const JsonObject& data)
        {
            if (data.containsKey(BLINKER_CMD_SET))
            {
                String value = data[BLINKER_CMD_SET];

                // DynamicJsonBuffer jsonBufferSet;
                // JsonObject& rootSet = jsonBufferSet.parseObject(value);
                DynamicJsonDocument jsonBuffer(BLINKER_JSON_BUFFER_SIZE);
                DeserializationError error = deserializeJson(jsonBuffer, value);
                JsonObject rootSet = jsonBuffer.as<JsonObject>();

                // if (!rootSet.success())
                if (error)
                {
                    // BLINKER_ERR_LOG_ALL("Json error");
                    return;
                }

                if (rootSet.containsKey(BLINKER_CMD_UPGRADE))
                {
                    BLINKER_LOG_ALL(BLINKER_F("otaParse isParsed"));
                    _fresh = true;

                    static_cast<Proto*>(this)->ota();
                }
            }
        }
    #endif

#endif

#endif
