#ifndef BLINKER_API_H
#define BLINKER_API_H

#include <time.h>
#if defined(ESP8266) || defined(ESP32)
    #include <Ticker.h>
    #include <EEPROM.h>
#endif

#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
#endif

#include "Blinker/BlinkerWidgetApi.h"

#if defined(BLINKER_ARDUINOJSON)
    #include "modules/ArduinoJson/ArduinoJson.h"
#endif

enum b_joystickaxis_t {
    BLINKER_J_Xaxis,
    BLINKER_J_Yaxis
};

enum b_ahrsattitude_t {
    Yaw,
    Pitch,
    Roll
};

enum b_gps_t {
    LONG,
    LAT
};

enum b_rgb_t {
    BLINKER_R,
    BLINKER_G,
    BLINKER_B,
    BLINKER_BRIGHT
};

template <class Proto>
class BlinkerApi
{
    public :
        // void hello();

        void freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        char * widgetName_str(uint8_t num);

    private :
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_int = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_joy = 0;

        class BlinkerWidgets_string *    _Widgets_str[BLINKER_MAX_WIDGET_SIZE*2];

        bool        _fresh = false;
        bool        _isNTPInit = false;
        float       _timezone = 8.0;
        uint32_t    _ntpStart;
        // time_t      now_ntp;
        // struct tm   timeinfo;

        
    protected :
        blinker_callback_t                  _heartbeatFunc = NULL;
        blinker_callback_return_string_t    _summaryFunc = NULL;

        void parse(char _data[], bool ex_data = false);

        #if defined(BLINKER_ARDUINOJSON)
            void heartBeat(const JsonObject& data);
            void getVersion(const JsonObject& data);

            void strWidgetsParse(char _wName[], const JsonObject& data);
        #else
            void heartBeat(char data[]);
            void getVersion(char data[]);
        #endif

        void freshNTP();
        bool ntpInit();
        void ntpConfig();
};

template <class Proto>
void BlinkerApi<Proto>::freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
    if(num >= 0 ) _Widgets_str[num]->setFunc(_func);
}

template <class Proto>
uint8_t BlinkerApi<Proto>::attachWidget(char _name[], blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_str < BLINKER_MAX_WIDGET_SIZE)
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
char * BlinkerApi<Proto>::widgetName_str(uint8_t num) {
    if (num) return _Widgets_str[num - 1]->getName();
    else return "";
}

template <class Proto>
void BlinkerApi<Proto>::parse(char _data[], bool ex_data)
{
    BLINKER_LOG_ALL(BLINKER_F("parse data: "), _data);
    
    if (!ex_data)
    {
        if (static_cast<Proto*>(this)->parseState())
        {
            _fresh = false;

            #if defined(BLINKER_ARDUINOJSON)
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(STRING_format(_data));

                if (!root.success()) return;

                heartBeat(root);
                getVersion(root);

                for (uint8_t wNum = 0; wNum < _wCount_str; wNum++)
                {
                    strWidgetsParse(_Widgets_str[wNum]->getName(), root);
                }
            #else
                heartBeat(_data);
                getVersion(_data);
            #endif

            if (_fresh)
            {
                static_cast<Proto*>(this)->isParsed();
            }
            // else
            // {
            //     #if defined(BLINKER_PRO)
            //         if (_parseFunc) {
            //             if(_parseFunc(root)) {
            //                 _fresh = true;
            //                 static_cast<Proto*>(this)->isParsed();
            //             }
                        
            //             BLINKER_LOG_ALL(BLINKER_F("run parse callback function"));
            //         }
            //     #endif
            // }
        }
    }
}

#if defined(BLINKER_ARDUINOJSON)
    template <class Proto>
    void BlinkerApi<Proto>::heartBeat(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_GET];

        if (state.length())
        {
            if (state == BLINKER_CMD_STATE)
            {
                #if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                #else
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                #endif

                // #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
                //     String _timer = taskCount ? "1":"0";
                //     _timer += _cdState ? "1":"0";
                //     _timer += _lpState ? "1":"0";

                //     BLINKER_LOG_ALL(BLINKER_F("timer codes: "), _timer);

                //     static_cast<Proto*>(this)->print(BLINKER_CMD_TIMER, _timer);
                //     // static_cast<Proto*>(this)->printJson(timerSetting());
                // #endif

                if (_heartbeatFunc) {
                    _heartbeatFunc();
                }

                if (_summaryFunc) {
                    String summary_data = _summaryFunc();
                    if (summary_data.length()) {
                        summary_data = summary_data.substring(0, BLINKER_MAX_SUMMARY_DATA_SIZE);

                        BLINKER_LOG_ALL(BLINKER_F("summary_data: "), summary_data);

                        static_cast<Proto*>(this)->print(BLINKER_CMD_SUMMARY, summary_data);
                    }
                }

                static_cast<Proto*>(this)->checkState(false);
                static_cast<Proto*>(this)->printNow();
                _fresh = true;

                // #if defined(BLINKER_AT_MQTT)
                //     static_cast<Proto*>(this)->atHeartbeat();
                // #endif
            }
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::getVersion(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_GET];
        
        if (state.length())
        {
            if (state == BLINKER_CMD_VERSION)
            {
                static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_OTA_VERSION_CODE);
                _fresh = true;
            }
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::strWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            String state = data[_wName];

            _fresh = true;
            
            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse: "), _wName);

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
            
            if (nbFunc) {
                nbFunc(state);
            }
        }
    }
#else
    template <class Proto>
    void BlinkerApi<Proto>::heartBeat(char data[])
    {
        if (strstr(data, BLINKER_CMD_GET) && \
            strstr(data, BLINKER_CMD_STATE))
        {
            #if defined(BLINKER_BLE)
                static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
            #else
                static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
            #endif

            if (_heartbeatFunc) {
                _heartbeatFunc();
            }

            if (_summaryFunc)
            {
                String summary_data = _summaryFunc();
                if (summary_data.length())
                {
                    summary_data = summary_data.substring(0, BLINKER_MAX_SUMMARY_DATA_SIZE);

                    BLINKER_LOG_ALL(BLINKER_F("summary_data: "), summary_data);

                    static_cast<Proto*>(this)->print(BLINKER_CMD_SUMMARY, summary_data);
                }
            }
            
            static_cast<Proto*>(this)->checkState(false);
            static_cast<Proto*>(this)->printNow();

            _fresh = true;

            // #if defined(BLINKER_AT_MQTT)
            //     static_cast<Proto*>(this)->atHeartbeat();
            // #endif
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::getVersion(char data[])
    {
        if (strstr(data, BLINKER_CMD_GET) && \
            strstr(data, BLINKER_CMD_VERSION))
        {
            static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_VERSION);
            _fresh = true;
        }
    }
#endif

template <class Proto>
void BlinkerApi<Proto>:: freshNTP() {
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif
    }
}

template <class Proto>
bool BlinkerApi<Proto>::ntpInit()
{
    if (!_isNTPInit)
    {
        freshNTP();

        if ((millis() - _ntpStart) > BLINKER_NTP_TIMEOUT)
        {
            _ntpStart = millis();
        }
        else {
            return false;
        }

        ntpConfig();

        time_t now_ntp = ::time(nullptr);

        float _com_timezone = abs(_timezone);
        if (_com_timezone < 1.0) _com_timezone = 1.0;

        if (now_ntp < _com_timezone * 3600 * 12)
        {
            ntpConfig();
            now_ntp = ::time(nullptr);
            if (now_ntp < _com_timezone * 3600 * 12)
            {
                ::delay(50);
                now_ntp = ::time(nullptr);
                return false;
            }
        }

        struct tm timeinfo;
        
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        BLINKER_LOG_ALL(BLINKER_F("Current time: "), asctime(&timeinfo));
        
        _isNTPInit = true;

        // loadTiming();
    }

    return true;
}

template <class Proto>
void BlinkerApi<Proto>::ntpConfig()
{
    String ntp1 = BLINKER_F("ntp1.aliyun.com");
    String ntp2 = BLINKER_F("210.72.145.44");
    String ntp3 = BLINKER_F("time.pool.aliyun.com");

    configTime((long)(_timezone * 3600), 0, \
            ntp1.c_str(), ntp2.c_str(), ntp3.c_str());
}

// template <class Proto>
// void BlinkerApi<Proto>::hello()
// {
//     BLINKER_LOG("BlinkerApi Hello");
// }

#endif
