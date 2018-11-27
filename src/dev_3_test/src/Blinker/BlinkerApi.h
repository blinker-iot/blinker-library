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

#include "Blinker/BlinkerApiBase.h"

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
        void vibrate(uint16_t ms = 200);
        void delay(unsigned long ms);
        void attachAhrs();
        void detachAhrs();
        int16_t ahrs(b_ahrsattitude_t attitude) { return ahrsValue[attitude]; }
        float gps(b_gps_t axis);

        void setTimezone(float tz);
        float getTimezone() { return _timezone; }
        int8_t second();
        /**< seconds after the minute - [ 0 to 59 ] */
        int8_t minute();
        /**< minutes after the hour - [ 0 to 59 ] */
        int8_t hour();
        /**< hours since midnight - [ 0 to 23 ] */
        int8_t mday();
        /**< day of the month - [ 1 to 31 ] */
        int8_t wday();
        /**< days since Sunday - [ 0 to 6 ] */
        int8_t month();
        /**< months since January - [ 1 to 12 ] */
        int16_t year();
        /**< years since 1900 */
        int16_t yday();
        /**< days since January 1 - [ 1 to 366 ] */
        time_t  time();

        int32_t dtime();

        void attachHeartbeat(blinker_callback_t newFunction)
        { _heartbeatFunc = newFunction; }
        void attachSummary(blinker_callback_return_string_t newFunction)
        { _summaryFunc = newFunction; }

        void freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        void freshAttachWidget(char _name[], blinker_callback_with_joy_arg_t _func);
        void freshAttachWidget(char _name[], blinker_callback_with_rgb_arg_t _func);
        void freshAttachWidget(char _name[], blinker_callback_with_int32_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_joy_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_rgb_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_int32_arg_t _func);
        void attachSwitch(blinker_callback_with_string_arg_t _func);
        char * widgetName_str(uint8_t num);
        char * widgetName_joy(uint8_t num);
        char * widgetName_rgb(uint8_t num);
        char * widgetName_int(uint8_t num);

        bool bridge(char _name[]);

    private :
        int16_t     ahrsValue[3];
        float       gpsValue[2];
        uint32_t    gps_get_time;
        bool        _fresh = false;
        bool        _isNTPInit = false;
        float       _timezone = 8.0;
        uint32_t    _ntpStart;

        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_joy = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_int = 0;
        
        class BlinkerWidgets_string *    _Widgets_str[BLINKER_MAX_WIDGET_SIZE*2];
        class BlinkerWidgets_joy *       _Widgets_joy[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_rgb *       _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_int32 *     _Widgets_int[BLINKER_MAX_WIDGET_SIZE*2];
        class BlinkerWidgets_string *    _BUILTIN_SWITCH;

        uint8_t     _bridgeCount = 0;
        class BlinkerBridge *            _Bridge[BLINKER_MAX_BRIDGE_SIZE];
        
        // time_t      now_ntp;
        // struct tm   timeinfo;
        
    protected :
        blinker_callback_t                  _heartbeatFunc = NULL;
        blinker_callback_return_string_t    _summaryFunc = NULL;

        void parse(char _data[], bool ex_data = false);

        #if defined(BLINKER_ARDUINOJSON)
            int16_t ahrs(b_ahrsattitude_t attitude, const JsonObject& data);
            float gps(b_gps_t axis, const JsonObject& data);

            void heartBeat(const JsonObject& data);
            void getVersion(const JsonObject& data);
            void setSwitch(const JsonObject& data);

            void strWidgetsParse(char _wName[], const JsonObject& data);
            void joyWidgetsParse(char _wName[], const JsonObject& data);
            void rgbWidgetsParse(char _wName[], const JsonObject& data);
            void intWidgetsParse(char _wName[], const JsonObject& data);

            void json_parse(const JsonObject& data);
        #else
            int16_t ahrs(b_ahrsattitude_t attitude, char data[]);
            float gps(b_gps_t axis, char data[]);

            void heartBeat(char data[]);
            void getVersion(char data[]);
            void setSwitch(char data[]);

            void strWidgetsParse(char _wName[], char _data[]);
            void joyWidgetsParse(char _wName[], char _data[]);
            void rgbWidgetsParse(char _wName[], char _data[]);
            void intWidgetsParse(char _wName[], char _data[]);

            void json_parse(char _data[]);
        #endif

        void freshNTP();
        bool ntpInit();
        void ntpConfig();

        String bridgeQuery(char key[]);
        // String blinkServer(uint8_t _type, const String & msg, bool state = false);
};

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
                BLINKER_LOG_ALL(BLINKER_F("defined BLINKER_ARDUINOJSON"));

                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(STRING_format(_data));

                if (!root.success()) return;

                heartBeat(root);
                getVersion(root);

                json_parse(root);

                ahrs(Yaw, root);
                gps(LONG, root);
            #else
                BLINKER_LOG_ALL(BLINKER_F("ndef BLINKER_ARDUINOJSON"));

                heartBeat(_data);
                getVersion(_data);

                json_parse(_data);

                ahrs(Yaw, _data);
                gps(LONG, _data);
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
        else
        {
            #if defined(BLINKER_ARDUINOJSON)
                String arrayData = BLINKER_F("{\"data\":");
                arrayData += _data;
                arrayData += BLINKER_F("}");
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(arrayData);

                if (!root.success()) return;

                arrayData = root["data"][0].as<String>();

                if (arrayData.length())
                {
                    for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
                    {
                        arrayData = root["data"][a_num].as<String>();

                        if(arrayData.length()) {
                            DynamicJsonBuffer _jsonBuffer;
                            JsonObject& _array = _jsonBuffer.parseObject(arrayData);

                            json_parse(_array);
                        }
                        else {
                            return;
                        }
                    }
                }
                else {
                    JsonObject& root = jsonBuffer.parseObject(_data);

                    if (!root.success()) return;

                    json_parse(root);
                }
            #else
                json_parse(_data);
            #endif
        }
    }
}

template <class Proto>
void BlinkerApi<Proto>::vibrate(uint16_t ms)
{
    if (ms > 1000) ms = 1000;

    static_cast<Proto*>(this)->print(BLINKER_CMD_VIBRATE, ms);
}

template <class Proto>
void BlinkerApi<Proto>::delay(unsigned long ms)
{
    uint32_t start = micros();
    uint32_t __start = millis();
    unsigned long _ms = ms;
    while (ms > 0)
    {
        static_cast<Proto*>(this)->run();

        if ((micros() - start) >= 1000)
        {
            ms -= 1;
            start += 1000;
        }

        if ((millis() - __start) >= _ms)
        {
            ms = 0;
        }
    }
}

template <class Proto>
void BlinkerApi<Proto>::attachAhrs()
{
    bool state = false;
    uint32_t startTime = millis();
    static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
    while (!state) {
        while (!static_cast<Proto*>(this)->connected()) {
            static_cast<Proto*>(this)->run();
            if (static_cast<Proto*>(this)->connect()) {
                static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                static_cast<Proto*>(this)->printNow();
                break;
            }
        }

        ::delay(100);

        if (static_cast<Proto*>(this)->checkAvail()) {
            // BLINKER_LOG(BLINKER_F("GET: "), static_cast<Proto*>(this)->dataParse());
            if (STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS)) {
                BLINKER_LOG(BLINKER_F("AHRS attach sucessed..."));
                parse(static_cast<Proto*>(this)->dataParse());
                state = true;
                break;
            }
            else {
                BLINKER_LOG(BLINKER_F("AHRS attach failed...Try again"));
                startTime = millis();
                parse(static_cast<Proto*>(this)->dataParse());
                static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                static_cast<Proto*>(this)->printNow();
            }
        }
        else {
            if (millis() - startTime > BLINKER_CONNECT_TIMEOUT_MS) {
                BLINKER_LOG(BLINKER_F("AHRS attach failed...Try again"));
                startTime = millis();
                static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                static_cast<Proto*>(this)->printNow();
            }
        }
    }
}

template <class Proto>
void BlinkerApi<Proto>::detachAhrs()
{
    static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_OFF);
    ahrsValue[Yaw] = 0;
    ahrsValue[Roll] = 0;
    ahrsValue[Pitch] = 0;
}

template <class Proto>
float BlinkerApi<Proto>::gps(b_gps_t axis)
{
    if ((millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT || \
        gps_get_time == 0)
    {
        static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
        static_cast<Proto*>(this)->printNow();
        delay(100);
    }

    return gpsValue[axis]*1000000;
}

template <class Proto>
void BlinkerApi<Proto>::setTimezone(float tz)
{
    _timezone = tz;
    _isNTPInit = false;
}

template <class Proto>
int8_t BlinkerApi<Proto>::second()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_sec;
    }
    return -1;
}
/**< seconds after the minute - [ 0 to 59 ] */
template <class Proto>
int8_t BlinkerApi<Proto>::minute()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_min;
    }
    return -1;
}
/**< minutes after the hour - [ 0 to 59 ] */
template <class Proto>
int8_t BlinkerApi<Proto>::hour()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_hour;
    }
    return -1;
}
/**< hours since midnight - [ 0 to 23 ] */
template <class Proto>
int8_t BlinkerApi<Proto>::mday()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_mday;
    }
    return -1;
}
/**< day of the month - [ 1 to 31 ] */
template <class Proto>
int8_t BlinkerApi<Proto>::wday()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_wday;
    }
    return -1;
}
/**< days since Sunday - [ 0 to 6 ] */
template <class Proto>
int8_t BlinkerApi<Proto>::month()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_mon + 1;
    }
    return -1;
}
/**< months since January - [ 1 to 12 ] */
template <class Proto>
int16_t BlinkerApi<Proto>::year()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_year + 1900;
    }
    return -1;
}
/**< years since 1900 */
template <class Proto>
int16_t BlinkerApi<Proto>::yday()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_yday + 1;
    }
    return -1;
}
/**< days since January 1 - [ 1 to 366 ] */
template <class Proto>
time_t  BlinkerApi<Proto>::time()
{
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return now_ntp;
    }
    return millis();
}

template <class Proto>
int32_t BlinkerApi<Proto>::dtime() {
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    }
    return -1;
}

template <class Proto>
void BlinkerApi<Proto>::freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
    if(num >= 0 ) _Widgets_str[num]->setFunc(_func);
}

template <class Proto>
void BlinkerApi<Proto>::freshAttachWidget(char _name[], blinker_callback_with_joy_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
    if(num >= 0 ) _Widgets_joy[num]->setFunc(_func);
}

template <class Proto>
void BlinkerApi<Proto>::freshAttachWidget(char _name[], blinker_callback_with_rgb_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
    if(num >= 0 ) _Widgets_rgb[num]->setFunc(_func);
}

template <class Proto>
void BlinkerApi<Proto>::freshAttachWidget(char _name[], blinker_callback_with_int32_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
    if(num >= 0 ) _Widgets_int[num]->setFunc(_func);
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
uint8_t BlinkerApi<Proto>::attachWidget(char _name[], blinker_callback_with_joy_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_joy < BLINKER_MAX_WIDGET_SIZE)
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
uint8_t BlinkerApi<Proto>::attachWidget(char _name[], blinker_callback_with_rgb_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_rgb < BLINKER_MAX_WIDGET_SIZE)
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
uint8_t BlinkerApi<Proto>::attachWidget(char _name[], blinker_callback_with_int32_arg_t _func)
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
void BlinkerApi<Proto>::attachSwitch(blinker_callback_with_string_arg_t _func)
{
    if (!_BUILTIN_SWITCH)
    {
        _BUILTIN_SWITCH = new BlinkerWidgets_string(BLINKER_CMD_BUILTIN_SWITCH, _func);
    }
    else
    {
        _BUILTIN_SWITCH->setFunc(_func);
    }
}

template <class Proto>
char * BlinkerApi<Proto>::widgetName_str(uint8_t num)
{
    if (num) return _Widgets_str[num - 1]->getName();
    else return "";
}

template <class Proto>
char * BlinkerApi<Proto>::widgetName_joy(uint8_t num)
{
    if (num) return _Widgets_joy[num - 1]->getName();
    else return "";
}

template <class Proto>
char * BlinkerApi<Proto>::widgetName_rgb(uint8_t num)
{
    if (num) return _Widgets_rgb[num - 1]->getName();
    else return "";
}

template <class Proto>
char * BlinkerApi<Proto>::widgetName_int(uint8_t num)
{
    if (num) return _Widgets_int[num - 1]->getName();
    else return "";
}

template <class Proto>
bool BlinkerApi<Proto>::bridge(char _name[])
{
    int8_t num = checkNum(_name, _Bridge, _bridgeCount);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if ( _bridgeCount < BLINKER_MAX_BRIDGE_SIZE )
        {
            //char register_r[26];
            //strcpy(register_r, bridgeQuery(_name).c_str());
            // if (strcmp(register_r, BLINKER_CMD_FALSE) == 0)
            String register_r = bridgeQuery(_name);
            if (register_r != BLINKER_CMD_FALSE)
            {
                _Bridge[_bridgeCount] = new BlinkerBridge();
                _Bridge[_bridgeCount]->name(_name);
                _Bridge[_bridgeCount]->freshBridge(register_r);
                _bridgeCount++;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else if(num >= 0 )
    {
        return true;
    }
    else {
        return false;
    }
}

#if defined(BLINKER_ARDUINOJSON)
    template <class Proto>
    int16_t BlinkerApi<Proto>::ahrs(b_ahrsattitude_t attitude, const JsonObject& data)
    {
        if (data.containsKey(BLINKER_CMD_AHRS)) {
            int16_t aAttiValue = data[BLINKER_CMD_AHRS][attitude];
            ahrsValue[Yaw] = data[BLINKER_CMD_AHRS][Yaw];
            ahrsValue[Roll] = data[BLINKER_CMD_AHRS][Roll];
            ahrsValue[Pitch] = data[BLINKER_CMD_AHRS][Pitch];

            _fresh = true;

            return aAttiValue;
        }
        else {
            return ahrsValue[attitude];
        }
    }

    template <class Proto>
    float BlinkerApi<Proto>::gps(b_gps_t axis, const JsonObject& data)
    {
        // if (((millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT || \
        //     gps_get_time == 0) && !newData)
        // {
        //     static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
        //     static_cast<Proto*>(this)->printNow();
        //     delay(100);
        // }

        if (data.containsKey(BLINKER_CMD_GPS)) {
            String gpsValue_LONG = data[BLINKER_CMD_GPS][LONG];
            String gpsValue_LAT = data[BLINKER_CMD_GPS][LAT];
            gpsValue[LONG] = gpsValue_LONG.toFloat();
            gpsValue[LAT] = gpsValue_LAT.toFloat();

            _fresh = true;

            if (_fresh) {
                static_cast<Proto*>(this)->isParsed();
                gps_get_time = millis();
            }

            return gpsValue[axis]*1000000;
        }
        else {
            return gpsValue[axis]*1000000;
        }
    }

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
    void BlinkerApi<Proto>::setSwitch(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_BUILTIN_SWITCH];

        if (state.length())
        {
            if (_BUILTIN_SWITCH)
            {
                blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

                if (sFunc) sFunc(state);
            }
            _fresh = true;
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
            
            if (nbFunc) nbFunc(state);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::joyWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            int16_t jxAxisValue = data[_wName][BLINKER_J_Xaxis];
            uint8_t jyAxisValue = data[_wName][BLINKER_J_Yaxis];

            _fresh = true;

            blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();
            if (wFunc) wFunc(jxAxisValue, jyAxisValue);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::rgbWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            uint8_t _rValue = data[_wName][BLINKER_R];
            uint8_t _gValue = data[_wName][BLINKER_G];
            uint8_t _bValue = data[_wName][BLINKER_B];
            uint8_t _brightValue = data[_wName][BLINKER_BRIGHT];

            _fresh = true;

            blinker_callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
            if (wFunc) wFunc(_rValue, _gValue, _bValue, _brightValue);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::intWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName)) {
            int _number = data[_wName];

            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
            if (wFunc) {
                wFunc(_number);
            }
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::json_parse(const JsonObject& data)
    {
        setSwitch(data);

        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), data);
        }
        for (uint8_t wNum_int = 0; wNum_int < _wCount_int; wNum_int++) {
            intWidgetsParse(_Widgets_int[wNum_int]->getName(), data);
        }
        for (uint8_t wNum_rgb = 0; wNum_rgb < _wCount_rgb; wNum_rgb++) {
            rgbWidgetsParse(_Widgets_rgb[wNum_rgb]->getName(), data);
        }
        for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
            joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), data);
        }
    }

#else

    template <class Proto>
    int16_t BlinkerApi<Proto>::ahrs(b_ahrsattitude_t attitude, char data[])
    {
        int16_t aAttiValue = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, attitude);

        if (aAttiValue != FIND_KEY_VALUE_FAILED)
        {
            ahrsValue[Yaw] = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, Yaw);
            ahrsValue[Roll] = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, Roll);
            ahrsValue[Pitch] = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, Pitch);

            _fresh = true;

            return aAttiValue;
        }
        else {
            return ahrsValue[attitude];
        }
    }

    template <class Proto>
    float BlinkerApi<Proto>::gps(b_gps_t axis, char data[])
    {
        // if (((millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT || \
        //     gps_get_time == 0) && !newData)
        // {
        //     static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
        //     static_cast<Proto*>(this)->printNow();
        //     delay(100);
        // }

        String axisValue = STRING_find_array_string_value(data, BLINKER_CMD_GPS, axis);

        if (axisValue != "") {
            gpsValue[LONG] = STRING_find_array_string_value(data, BLINKER_CMD_GPS, LONG).toFloat();
            gpsValue[LAT] = STRING_find_array_string_value(data, BLINKER_CMD_GPS, LAT).toFloat();

            _fresh = true;

            if (_fresh) {
                static_cast<Proto*>(this)->isParsed();
                gps_get_time = millis();
            }

            return gpsValue[axis]*1000000;
        }
        else {
            return gpsValue[axis]*1000000;
        }
    }

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

    template <class Proto>
    void BlinkerApi<Proto>::setSwitch(char data[])
    {
        String state;

        if (STRING_find_string_value(data, state, BLINKER_CMD_BUILTIN_SWITCH))
        {
            if (_BUILTIN_SWITCH)
            {
                blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

                if (sFunc) sFunc(state);
            }
            _fresh = true;
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::strWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        String state;

        if (STRING_find_string_value(_data, state, _wName))
        {
            _fresh = true;

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
            if (nbFunc) nbFunc(state);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::joyWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        int16_t jxAxisValue = STRING_find_array_numberic_value(_data, \
                                            _wName, BLINKER_J_Xaxis);

        if (jxAxisValue != FIND_KEY_VALUE_FAILED)
        {
            uint8_t jyAxisValue = STRING_find_array_numberic_value(_data, \
                                                _wName, BLINKER_J_Yaxis);

            _fresh = true;

            blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();

            if (wFunc) wFunc(jxAxisValue, jyAxisValue);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::rgbWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        int16_t _rValue = STRING_find_array_numberic_value(_data, \
                                                _wName, BLINKER_R);

        if (_rValue != FIND_KEY_VALUE_FAILED)
        {
            uint8_t _gValue = STRING_find_array_numberic_value(_data, _wName, BLINKER_G);
            uint8_t _bValue = STRING_find_array_numberic_value(_data, _wName, BLINKER_B);
            uint8_t _brightValue = STRING_find_array_numberic_value(_data, _wName, BLINKER_BRIGHT);

            _fresh = true;

            blinker_callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();

            if (wFunc) wFunc(_rValue, _gValue, _bValue, _brightValue);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::intWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        int _number = STRING_find_numberic_value(_data, _wName);

        if (_number != FIND_KEY_VALUE_FAILED)
        {
            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();

            if (wFunc) wFunc(_number);
        }
    }

    template <class Proto>
    void BlinkerApi<Proto>::json_parse(char _data[])
    {
        setSwitch(_data);

        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), _data);
        }
        for (uint8_t wNum_int = 0; wNum_int < _wCount_int; wNum_int++) {
            intWidgetsParse(_Widgets_int[wNum_int]->getName(), _data);
        }
        for (uint8_t wNum_rgb = 0; wNum_rgb < _wCount_rgb; wNum_rgb++) {
            rgbWidgetsParse(_Widgets_rgb[wNum_rgb]->getName(), _data);
        }
        for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
            joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), _data);
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

template <class Proto>
String BlinkerApi<Proto>::bridgeQuery(char key[])
{
    String data = BLINKER_F("/query?");
    data += BLINKER_F("deviceName=");
    data += static_cast<Proto*>(this)->conn.deviceName();
    data += BLINKER_F("&bridgeKey=");
    data += STRING_format(key);

    // return blinkServer(BLINKER_CMD_BRIDGE_NUMBER, data);
    return "";
}

// template <class Proto>
// String BlinkerApi<Proto>::blinkServer(uint8_t _type, \
//                 const String & msg, bool state = false)
// {
// }
#endif
