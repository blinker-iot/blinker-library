#ifndef BlinkerApi_H
#define BlinkerApi_H

#include <time.h>
#if defined(ESP8266) || defined(ESP32)
    #include <Ticker.h>
    #include <EEPROM.h>
    #include "modules/ArduinoJson/ArduinoJson.h"

    #include <utility/BlinkerTimingTimer.h>
#endif
#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
#endif
#if defined(BLINKER_MQTT)
    #include <utility/BlinkerAuto.h>
#elif defined(BLINKER_PRO)
    #include <utility/BlinkerAuto.h>
    #include <utility/BlinkerWlan.h>
    #include "modules/OneButton/OneButton.h"
#else
    #include <Blinker/BlinkerConfig.h>
    #include <utility/BlinkerUtility.h>
#endif


// enum b_widgettype_t {
//     W_BUTTON,
//     W_SLIDER,
//     W_TOGGLE,
//     W_RGB
// };


enum b_joystickaxis_t {
    J_Xaxis,
    J_Yaxis
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
    R,
    G,
    B,
    BRGB
};


static class BlinkerWidgets_string * _Widgets_str[BLINKER_MAX_WIDGET_SIZE*2];
static class BlinkerWidgets_string * _BUILTIN_SWITCH;
static class BlinkerWidgets_int32 * _Widgets_int[BLINKER_MAX_WIDGET_SIZE*2];
static class BlinkerWidgets_rgb * _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
static class BlinkerWidgets_joy * _Widgets_joy[BLINKER_MAX_WIDGET_SIZE/2];

#if defined(ESP8266) || defined(ESP32)
static class BlinkerTimingTimer * timingTask[BLINKER_TIMING_TIMER_SIZE];
#endif


#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
static class BlinkerAUTO * _AUTO[2];
static class BlinkerBridge * _Bridge[BLINKER_MAX_BRIDGE_SIZE];
// #endif
// #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
static class BlinkerData *_Data[6];
#endif

#if defined(BLINKER_WIFI)
#include <WiFiClientSecure.h>
static WiFiClientSecure client_s;
#endif

class BlinkerWidgets_string
{
    public :
        BlinkerWidgets_string(const String & _name, callback_with_string_arg_t _func = NULL)
            : wName(_name), wfunc(_func)
        {}

        String getName() { return wName; }
        void setFunc(callback_with_string_arg_t _func) { wfunc = _func; }
        callback_with_string_arg_t getFunc() { return wfunc; }
        bool checkName(String name) { return ((wName == name) ? true : false); }

    private :
        String wName;
        callback_with_string_arg_t wfunc;
};

class BlinkerWidgets_int32
{
    public :
        BlinkerWidgets_int32(const String & _name, callback_with_int32_arg_t _func = NULL)
            : wName(_name), wfunc(_func)
        {}

        String getName() { return wName; }
        void setFunc(callback_with_int32_arg_t _func) { wfunc = _func; }
        callback_with_int32_arg_t getFunc() { return wfunc; }
        bool checkName(String name) { return ((wName == name) ? true : false); }

    private :
        String wName;
        callback_with_int32_arg_t wfunc;
};

class BlinkerWidgets_rgb
{
    public :
        BlinkerWidgets_rgb(const String & _name, callback_with_rgb_arg_t _func = NULL)
            : wName(_name), wfunc(_func)
        {}

        String getName() { return wName; }
        void setFunc(callback_with_rgb_arg_t _func) { wfunc = _func; }
        callback_with_rgb_arg_t getFunc() { return wfunc; }
        bool checkName(String name) { return ((wName == name) ? true : false); }

    private :
        String wName;
        callback_with_rgb_arg_t wfunc;
};

class BlinkerWidgets_joy
{
    public :
        BlinkerWidgets_joy(const String & _name, callback_with_joy_arg_t _func = NULL)
            : wName(_name), wfunc(_func)
        {}

        String getName() { return wName; }
        void setFunc(callback_with_joy_arg_t _func) { wfunc = _func; }
        callback_with_joy_arg_t getFunc() { return wfunc; }
        bool checkName(String name) { return ((wName == name) ? true : false); }

    private :
        String wName;
        callback_with_joy_arg_t wfunc;
};

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
class BlinkerBridge
{
    public :
        BlinkerBridge()
            : _name(NULL)
        {}

        void name(String name) { _name = name; }
        String getName() { return _name; }
        void freshBridge(String name) { bridgeName = name; }
        String getBridge() { return bridgeName; }
        bool checkName(String name) { return ((_name == name) ? true : false); }

    private :
        String _name;
        String bridgeName;
};
// #endif

// #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
class BlinkerData
{
    public :
        BlinkerData()
            : _dname(NULL)
        {}

        void name(String name) { _dname = name; }
        String getName() { return _dname; }
        void saveData(time_t _time, String _data) {
            if (data != "") {
                data += ",";
            }
            data += "[" + STRING_format(_time) + "," + _data + "]";
        }
        String getData() { return data; }
        bool checkName(String name) { return ((_dname == name) ? true : false); }

    private :
        String _dname;
        String data = "";
};
#endif

template <class T>
int8_t checkNum(String name, T * c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++) {
        if (c[cNum]->checkName(name))
            return cNum;
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

#if defined(ESP8266) || defined(ESP32)
class BlinkerTimer
    : public Ticker
{
    public :
        typedef void (*callback_t)(void);
        typedef void (*callback_with_arg_t)(void*);

        void countdown(float seconds, callback_t callback) {
            CDowner.once(seconds, callback);
        }

        // template<typename TArg>
        // void countdown(float seconds, void (*callback)(TArg), TArg arg) {
        //     CDowner::once(seconds, arg);
        // }

        void loop(float seconds, callback_t callback) {
            Looper.attach(seconds, callback);
        }

        // template<typename TArg>
        // void loop(float seconds, void (*callback)(TArg), TArg arg, uint8_t times) {
        //     Timer::attach(seconds, arg);
        // }

        void timing(float seconds1, callback_t callback1, float seconds2 , callback_t callback2) {
            Timinger1.once(seconds1, callback1);
            Timinger2.once(seconds1 + seconds2, callback2);
        }

        void detach() {
            CDowner.detach();
            Looper.detach();
            Timinger1.detach();
            Timinger2.detach();
        }

    private :
        Ticker CDowner;
        Ticker Looper;
        Ticker Timinger1;
        Ticker Timinger2;
};

Ticker cdTicker;
Ticker lpTicker;
Ticker tmTicker;

static bool _cdRunState = false;
static bool _lpRunState = false;
static bool _tmRunState = false;
static bool _cdState = false;
static bool _lpState = false;
static bool _tmState = false;
static bool _lpRun1 = true;
static bool _tmRun1 = true;
static bool _tmDay = false;
static bool _cdTrigged = false;
static bool _lpTrigged = false;
static bool _tmTrigged = false;
static bool _isTimingLoop = false;

static uint8_t  _lpTimes;
static uint8_t  _lpTrigged_times;

static uint32_t _cdTime1;
static uint32_t _cdTime2;
static uint32_t _cdStart;
static uint16_t _cdData;
// static bool     _cdStop = true;

static uint32_t _lpTime1;
static uint32_t _lpTime2;
static uint32_t _lpData;
static bool     _lpStop = true;

static uint32_t _tmTime1;
static uint32_t _tmTime2;
static uint32_t _tmTime;
static uint8_t  _timingDay = 0;
static uint8_t  taskCount = 0;
static uint8_t  triggedTask = 0;

// static String _cbData1;
// static String _cbData2;

static void disableTimer() {
    _cdRunState = false;
    cdTicker.detach();
    _lpRunState = false;
    lpTicker.detach();
    _tmRunState = false;
    tmTicker.detach();
}

static void _cd_callback() {
    // _cdState = false;
    _cdTrigged = true;
    #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1(("countdown trigged!"));
    #endif
}

// static void _countdown(float seconds) {
//     _cdState = true;
//     cdTicker.once(seconds, _cd_callback);
// }

static void _lp_callback() {
    // _lpState = false;
    _lpRun1 = !_lpRun1;
    if (_lpRun1) {
        _lpTrigged_times++;

        if (_lpTimes) {
            if (_lpTimes == _lpTrigged_times) {
                lpTicker.detach();
                _lpStop = true;
            }
            else {
                lpTicker.once(_lpTime1 * 60, _lp_callback);
            }
        }
        else {
            lpTicker.once(_lpTime1 * 60, _lp_callback);
        }
    }
    else {
        lpTicker.once(_lpTime2 * 60, _lp_callback);
    }
    _lpTrigged = true;
    #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1(("loop trigged!"));
    #endif
}

// static void _loop(float seconds) {
//     _lpState = false;
//     lpTicker.attach(seconds, _lp_callback);
// }

// static bool isTimingDay(uint8_t _day) {
//     #ifdef BLINKER_DEBUG_ALL
//     BLINKER_LOG2(("isTimingDay: "), _day);
//     #endif
//     if (_timingDay & (0x01 << _day))
//         return true;
//     else
//         return false;
// }

static void timingHandle(uint8_t cbackData) {
    // time_t      now_ntp;
    // struct tm   timeinfo;
    // now_ntp = time(nullptr);
    // // gmtime_r(&now_ntp, &timeinfo);
    // #if defined(ESP8266)
    // gmtime_r(&now_ntp, &timeinfo);
    // #elif defined(ESP32)
    // localtime_r(&now_ntp, &timeinfo);
    // #endif

    uint8_t task = cbackData;
    // uint8_t wDay = timeinfo.tm_wday;

    // if (task < BLINKER_TIMING_TIMER_SIZE) {
    _tmTrigged = true;

    triggedTask = task;
    // }
    // else if (task == 32){

    // }
}

// static void _tm_callback() {
//     _tmRun1 = !_tmRun1;

//     time_t      now_ntp;
//     struct tm   timeinfo;
//     now_ntp = time(nullptr);
//     // gmtime_r(&now_ntp, &timeinfo);
//     #if defined(ESP8266)
//     gmtime_r(&now_ntp, &timeinfo);
//     #elif defined(ESP32)
//     localtime_r(&now_ntp, &timeinfo);
//     // getLocalTime(&timeinfo, 5000);
//     #endif

//     int32_t nowTime = timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
//     #ifdef BLINKER_DEBUG_ALL
//     BLINKER_LOG6(("nowTime: "), nowTime, (" _tmTime1: "), _tmTime1, (" _tmTime2: "), _tmTime2);
//     #endif

//     if (isTimingDay(timeinfo.tm_wday)) {
//         if (_tmRun1) {
//             if (!_isTimingLoop) {
//                 tmTicker.detach();
//     #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG1(("timing2 trigged! now need stop!"));
//     #endif
//                 _tmState = false;
//             }
//             else {
//                 if (_tmTime1 >= nowTime) {
//                     tmTicker.once(_tmTime1 - nowTime, _tm_callback);
//     #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(("timing2 trigged! next time: "), _tmTime1 - nowTime);
//     #endif
//                 }
//                 else if (_tmTime2 <= nowTime && _tmTime1 < nowTime) {
//                     tmTicker.once(BLINKER_ONE_DAY_TIME - nowTime, _tm_callback);
//     #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(("timing2 trigged! next time: "), BLINKER_ONE_DAY_TIME - nowTime);
//     #endif
//                 }
//             }
//         }
//         else {
//             tmTicker.once(_tmTime2 - _tmTime1, _tm_callback);
//     #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(("timing1 trigged! next time: "), _tmTime2 - _tmTime1);
//     #endif
//         }
//         _tmTrigged = true;
//     }
//     else {
//         tmTicker.once(BLINKER_ONE_DAY_TIME - nowTime, _tm_callback);
//     #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG2(("timing trigged! next time: "), BLINKER_ONE_DAY_TIME - nowTime);
//     #endif
//     }
// }

// static void _timing(float seconds) {
//     tmTicker.attach(seconds, _tm_callback);
// }
#endif



template <class Proto>
class BlinkerApi
{
    public :
        BlinkerApi() {
            ahrsValue[Yaw] = 0;
            ahrsValue[Roll] = 0;
            ahrsValue[Pitch] = 0;
            gpsValue[LONG] = "0.000000";
            gpsValue[LAT] = "0.000000";
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        bool bridge(const String & _name) {
            int8_t num = checkNum(_name, _Bridge, _bridgeCount);
            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                if ( _bridgeCount < BLINKER_MAX_BRIDGE_SIZE ) {
                    String register_r = bridgeQuery(_name);
                    if (register_r != BLINKER_CMD_FALSE) {
                        _Bridge[_bridgeCount] = new BlinkerBridge();
                        _Bridge[_bridgeCount]->name(_name);
                        _Bridge[_bridgeCount]->freshBridge(register_r);
                        _bridgeCount++;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                else {
                    return false;
                }
            }
            else if(num >= 0 ) {
                return true;
            }
            else {
                return false;
            }
        }
#endif

        void attachJoystick(callback_with_joy_arg_t _func) {
            _joyFunc = _func;
        }

        void attachSwitch(callback_with_string_arg_t _func) {
            if (!_BUILTIN_SWITCH) {
                _BUILTIN_SWITCH = new BlinkerWidgets_string(BLINKER_CMD_BUILTIN_SWITCH, _func);
            }
            else {
                _BUILTIN_SWITCH->setFunc(_func);
            }
        }

        void freshAttachWidget(const String & _name, callback_with_joy_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
            if(num >= 0 ) {
                _Widgets_joy[num]->setFunc(_func);
            }
        }

        bool attachWidget(const String & _name, callback_with_joy_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                if (_wCount_joy < BLINKER_MAX_WIDGET_SIZE) {
                    _Widgets_joy[_wCount_joy] = new BlinkerWidgets_joy(_name, _func);
                    _wCount_joy++;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG4("new widgets: ", _name, " _wCount_joy: ", _wCount_joy);
#endif
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(num >= 0 ) {
                BLINKER_ERR_LOG3("widgets name > ", _name, " < has been registered, please register another name!");
                return false;
            }
            else {
                return false;
            }
        }

        void freshAttachWidget(const String & _name, callback_with_rgb_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
            if(num >= 0 ) {
                _Widgets_rgb[num]->setFunc(_func);
            }
        }

        bool attachWidget(const String & _name, callback_with_rgb_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                if (_wCount_rgb < BLINKER_MAX_WIDGET_SIZE) {
                    _Widgets_rgb[_wCount_rgb] = new BlinkerWidgets_rgb(_name, _func);
                    _wCount_rgb++;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG4("new widgets: ", _name, " _wCount_rgb: ", _wCount_rgb);
#endif
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(num >= 0 ) {
                BLINKER_ERR_LOG3("widgets name > ", _name, " < has been registered, please register another name!");
                return false;
            }
            else {
                return false;
            }
        }

        void freshAttachWidget(const String & _name, callback_with_int32_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
            if(num >= 0 ) {
                _Widgets_int[num]->setFunc(_func);
            }
        }

        bool attachWidget(const String & _name, callback_with_int32_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                if (_wCount_int < BLINKER_MAX_WIDGET_SIZE) {
                    _Widgets_int[_wCount_int] = new BlinkerWidgets_int32(_name, _func);
                    _wCount_int++;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG4("new widgets: ", _name, " _wCount_int: ", _wCount_int);
#endif
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(num >= 0 ) {
                BLINKER_ERR_LOG3("widgets name > ", _name, " < has been registered, please register another name!");
                return false;
            }
            else {
                return false;
            }
        }

        void freshAttachWidget(const String & _name, callback_with_string_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
            if(num >= 0 ) {
                _Widgets_str[num]->setFunc(_func);
            }
        }

        bool attachWidget(const String & _name, callback_with_string_arg_t _func) {
            int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                if (_wCount_str < BLINKER_MAX_WIDGET_SIZE) {
                    _Widgets_str[_wCount_str] = new BlinkerWidgets_string(_name, _func);
                    _wCount_str++;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG4("new widgets: ", _name, " _wCount_str: ", _wCount_str);
#endif
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(num >= 0 ) {
                BLINKER_ERR_LOG3("widgets name > ", _name, " < has been registered, please register another name!");
                return false;
            }
            else {
                return false;
            }
        }

        int16_t ahrs(b_ahrsattitude_t attitude)
        {
            int16_t aAttiValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, attitude);

            if (aAttiValue != FIND_KEY_VALUE_FAILED) {
                ahrsValue[Yaw] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, Yaw);
                ahrsValue[Roll] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, Roll);
                ahrsValue[Pitch] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS, Pitch);

                _fresh = true;

                return aAttiValue;
            }
            else {
                return ahrsValue[attitude];
            }
        }

        void attachAhrs()
        {
            bool state = false;
            uint32_t startTime = millis();
            static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
            while (!state) {
                while (!static_cast<Proto*>(this)->connected()) {
                    static_cast<Proto*>(this)->run();
                    if (static_cast<Proto*>(this)->connect()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                        break;
                    }
                }

                ::delay(100);

                if (static_cast<Proto*>(this)->checkAvail()) {
                    // BLINKER_LOG2(BLINKER_F("GET: "), static_cast<Proto*>(this)->dataParse());
                    if (STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS)) {
                        BLINKER_LOG1(BLINKER_F("AHRS attach sucessed..."));
                        parse(static_cast<Proto*>(this)->dataParse());
                        state = true;
                        break;
                    }
                    else {
                        BLINKER_LOG1(BLINKER_F("AHRS attach failed...Try again"));
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                    }
                }
                else {
                    if (millis() - startTime > BLINKER_CONNECT_TIMEOUT_MS) {
                        BLINKER_LOG1(BLINKER_F("AHRS attach failed...Try again"));
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                    }
                }
            }
        }

        void detachAhrs()
        {
            static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_OFF);
            ahrsValue[Yaw] = 0;
            ahrsValue[Roll] = 0;
            ahrsValue[Pitch] = 0;
        }

        String gps(b_gps_t axis, bool newData = false) {
            if (!newData && (millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT) {
                static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
                delay(100);

                gps_get_time = millis();
            }

            String axisValue = STRING_find_array_string_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS, axis);

            if (axisValue != "") {
                gpsValue[LONG] = STRING_find_array_string_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS, LONG);
                gpsValue[LAT] = STRING_find_array_string_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_GPS, LAT);

                _fresh = true;

                if (_fresh) {
                    static_cast<Proto*>(this)->isParsed();
                }

                return gpsValue[axis];
            }
            else {
                return gpsValue[axis];
            }
        }

        void vibrate(uint16_t ms = 200)
        {
            if (ms > 1000) {
                ms = 1000;
            }

            static_cast<Proto*>(this)->print(BLINKER_CMD_VIBRATE, ms);
        }

        void delay(unsigned long ms)
        {
            uint32_t start = micros();
            while (ms > 0) {
                static_cast<Proto*>(this)->run();

                yield();

                if ((micros() - start)/1000 >= ms) {
                    ms = 0;
                }
            }
        }

        void setTimezone(float tz) {
            _timezone = tz;
            _isNTPInit = false;
        }

        float getTimezone() {
            return _timezone;
        }

        int8_t second() { freshNTP(); return _isNTPInit ? timeinfo.tm_sec : -1; }
        /**< seconds after the minute - [ 0 to 59 ] */
        int8_t minute() { freshNTP(); return _isNTPInit ? timeinfo.tm_min : -1; }
        /**< minutes after the hour - [ 0 to 59 ] */
        int8_t hour()   { freshNTP(); return _isNTPInit ? timeinfo.tm_hour : -1; }
        /**< hours since midnight - [ 0 to 23 ] */
        int8_t mday()   { freshNTP(); return _isNTPInit ? timeinfo.tm_mday : -1; }
        /**< day of the month - [ 1 to 31 ] */
        int8_t wday()   { freshNTP(); return _isNTPInit ? timeinfo.tm_wday : -1; }
        /**< days since Sunday - [ 0 to 6 ] */
        int8_t month()  { freshNTP(); return _isNTPInit ? timeinfo.tm_mon + 1 : -1; }
        /**< months since January - [ 1 to 12 ] */
        int16_t year()  { freshNTP(); return _isNTPInit ? timeinfo.tm_year + 1900 : -1; }
        /**< years since 1900 */
        int16_t yday()  { freshNTP(); return _isNTPInit ? timeinfo.tm_yday + 1 : -1; }
        /**< days since January 1 - [ 1 to 366 ] */
        time_t  time()  { freshNTP(); return _isNTPInit ? now_ntp : millis(); }

        int32_t dtime() {
            freshNTP();
            return _isNTPInit ? timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec : -1;
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        void beginAuto() {
            BLINKER_LOG1(BLINKER_F("======================================================="));
            BLINKER_LOG1(BLINKER_F("=========== Blinker Auto Control mode init! ==========="));
            BLINKER_LOG1(BLINKER_F("Warning!EEPROM address 0-1279 is used for Auto Control!"));
            BLINKER_LOG1(BLINKER_F("=========== DON'T USE THESE EEPROM ADDRESS! ==========="));
            BLINKER_LOG1(BLINKER_F("======================================================="));

            BLINKER_LOG2(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

            // deserialization();
            autoStart();
        }

        // void autoRun(String key, String state) {
        void autoInput(String key, String state) {
            if (!_isNTPInit) {
                return;
            }

            int32_t nowTime = dtime();

            for (uint8_t _num = 0; _num < _aCount; _num++) {
                _AUTO[_num]->run(key, state, nowTime);
            }
        }

        // void autoRun(String key, float data) {
        void autoInput(String key, float data) {
            if (!_isNTPInit) {
                return;
            }

            int32_t nowTime = dtime();

            for (uint8_t _num = 0; _num < _aCount; _num++) {
                _AUTO[_num]->run(key, data, nowTime);
            }
        }

        void autoRun() {
            for (uint8_t _num = 0; _num < _aCount; _num++) {
                if (_AUTO[_num]->isTrigged()) {
                    uint8_t link_num = _AUTO[_num]->linkNum();

                    if (link_num == 0) {
                        if (static_cast<Proto*>(this)->autoTrigged(_AUTO[_num]->id()))
                        {
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("trigged sucessed"));
    #endif
                            _AUTO[_num]->fresh();
                        }
                        else {
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("trigged failed"));
    #endif
                        }
                    }
                    else if (link_num == 1) {
                        if (static_cast<Proto*>(this)->autoTrigged(
                            _AUTO[_num]->name(0) ,
                            _AUTO[_num]->type(0) ,
                            _AUTO[_num]->data(0)))
                        {
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("trigged sucessed"));
    #endif
                            _AUTO[_num]->fresh();
                        }
                        else {
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("trigged failed"));
    #endif
                        }
                    }
                    else if (link_num == 2) {
                        if (static_cast<Proto*>(this)->autoTrigged(
                            _AUTO[_num]->name(0) ,
                            _AUTO[_num]->type(0) ,
                            _AUTO[_num]->data(0) ,
                            _AUTO[_num]->name(1) ,
                            _AUTO[_num]->type(1) ,
                            _AUTO[_num]->data(1)))
                        {
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("trigged sucessed"));
    #endif
                            _AUTO[_num]->fresh();
                        }
                        else {
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("trigged failed"));
    #endif
                        }
                    }
                }
            }
        }
// #else
//     #pragma message("This code is intended to run with BLINKER_MQTT! Please check your connect type.")
#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        template<typename T>
        bool cloudUpdate(const T& msg) {
            String _msg = STRING_format(msg);

            String data = "{\"deviceName\":\"" + STRING_format(static_cast<Proto*>(this)->_deviceName) + \
                            "\",\"config\":\"" + _msg + "\"}";

            if (_msg.length() > 256) {
                return false;
            }
            return (blinkServer(BLINKER_CMD_CONFIG_UPDATE_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
        }

        String cloudGet() {
            String data = "/pull_userconfig?deviceName=" + STRING_format(static_cast<Proto*>(this)->_deviceName);

            return blinkServer(BLINKER_CMD_CONFIG_GET_NUMBER, data);
        }

        template<typename T>
        void dataStorage(const String & _name, const T& msg) {
            String _msg = STRING_format(msg);

            int8_t num = checkNum(_name, _Data, _dataCount);

            // BLINKER_LOG2("num: ", num);

            if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                if (_dataCount == 5) {
                    return;
                }
                _Data[_dataCount] = new BlinkerData();
                _Data[_dataCount]->name(_name);
                _Data[_dataCount]->saveData(time(), _msg);
                _dataCount++;
            }
            else {
                _Data[num]->saveData(time(), _msg);
            }
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG3(_name, " save: ", _msg);
            BLINKER_LOG2("_dataCount: ", _dataCount);
    #endif
        }

        bool dataUpdate() {
            String data = "{\"deviceName\":\"" + STRING_format(static_cast<Proto*>(this)->_deviceName) + \
                            "\",\"data\":{";
            // String _sdata;

            if (!_dataCount) {
                BLINKER_ERR_LOG1("none data storaged!");
                return false;
            }

            for (uint8_t _num = 0; _num < _dataCount; _num++) {
                data += "\"" + _Data[_num]->getName() + "\":[" + _Data[_num]->getData() + "]";
                if (_num < _dataCount - 1) {
                    data += ",";
                }
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG4("num: ", _num, " name: ", _Data[_num]->getName());
    #endif
            }

            data += "}}";
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("dataUpdate: ", data);
    #endif
            // return true;
                            //  + \ _msg + \
                            // "\"}}";
            if (blinkServer(BLINKER_CMD_DATA_STORAGE_NUMBER, data) == BLINKER_CMD_FALSE) {
                return false;

            }
            else {
                for (uint8_t _num = 0; _num < _dataCount; _num++) {
                    delete _Data[_num];
                }
                _dataCount = 0;
                return true;
            }
        }
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        template<typename T>
        bool sms(const T& msg) {
            String _msg = STRING_format(msg);
    #if defined(BLINKER_MQTT)
            String data = "{\"authKey\":\"" + STRING_format(static_cast<Proto*>(this)->_authKey) + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #elif defined(BLINKER_WIFI) || defined(BLINKER_PRO)
            String data = "{\"deviceName\":\"" + macDeviceName() + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #endif

            if (_msg.length() > 20) {
                return false;
            }
            return (blinkServer(BLINKER_CMD_SMS_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
        }

        template<typename T>
        bool sms(const T& msg, const char* cel) {
            String _msg = STRING_format(msg);
    #if defined(BLINKER_MQTT)
            String data = "{\"authKey\":\"" + STRING_format(static_cast<Proto*>(this)->_authKey) + \
                            "\",\"cel\":\"" + cel + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #elif defined(BLINKER_WIFI) ||  defined(BLINKER_PRO)
            String data = "{\"deviceName\":\"" + macDeviceName() + \
                            "\",\"cel\":\"" + cel + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #endif

            if (_msg.length() > 20) {
                return false;
            }
            return (blinkServer(BLINKER_CMD_SMS_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
        }

        template<typename T>
        bool push(const T& msg) {
            String _msg = STRING_format(msg);
    #if defined(BLINKER_MQTT)
            String data = "{\"authKey\":\"" + STRING_format(static_cast<Proto*>(this)->_deviceName) + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #elif defined(BLINKER_WIFI) ||  defined(BLINKER_PRO)
            String data = "{\"deviceName\":\"" + macDeviceName() + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #endif

            // if (_msg.length() > 20) {
            //     return false;
            // }
            return (blinkServer(BLINKER_CMD_PUSH_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
        }

        template<typename T>
        bool wechat(const T& msg) {
            String _msg = STRING_format(msg);
    #if defined(BLINKER_MQTT)
            String data = "{\"authKey\":\"" + STRING_format(static_cast<Proto*>(this)->_deviceName) + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #elif defined(BLINKER_WIFI) ||  defined(BLINKER_PRO)
            String data = "{\"deviceName\":\"" + macDeviceName() + \
                            "\",\"msg\":\"" + _msg + "\"}";
    #endif

            // if (_msg.length() > 20) {
            //     return false;
            // }
            return (blinkServer(BLINKER_CMD_PUSH_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
        }

        String weather(String _city = BLINKER_CMD_DEFAULT) {
            // String _msg = STRING_format(msg);
            String data = "/weather/now?";
    #if defined(BLINKER_MQTT)
            // String data = "{\"authKey\":\"" + STRING_format(static_cast<Proto*>(this)->_deviceName) + \
            //                 "\",\"city\":\"" + _city + "\"}";

            data += "deviceName=" + STRING_format(static_cast<Proto*>(this)->_deviceName);
    #elif defined(BLINKER_WIFI) ||  defined(BLINKER_PRO)
            // String data = "{\"deviceName\":\"" + macDeviceName() + \
            //                 "\",\"city\":\"" + _city + "\"}";

            data += "deviceName=" + macDeviceName();
    #endif

            if (_city != BLINKER_CMD_DEFAULT) {
                data += "&location=" + STRING_format(_city);
            }

            // if (_msg.length() > 20) {
            //     return false;
            // }
            return blinkServer(BLINKER_CMD_WEATHER_NUMBER, data);
        }

        String aqi(String _city = BLINKER_CMD_DEFAULT) {
            // String _msg = STRING_format(msg);
            String data = "/weather/aqi?";
    #if defined(BLINKER_MQTT)
            // String data = "{\"authKey\":\"" + STRING_format(static_cast<Proto*>(this)->_deviceName) + \
            //                 "\",\"city\":\"" + _city + "\"}";

            data += "deviceName=" + STRING_format(static_cast<Proto*>(this)->_deviceName);
    #elif defined(BLINKER_WIFI) ||  defined(BLINKER_PRO)
            // String data = "{\"deviceName\":\"" + macDeviceName() + \
            //                 "\",\"city\":\"" + _city + "\"}";

            data += "deviceName=" + macDeviceName();
    #endif

            if (_city != BLINKER_CMD_DEFAULT) {
                data += "&location=" + STRING_format(_city);
            }

            // if (_msg.length() > 20) {
            //     return false;
            // }
            return blinkServer(BLINKER_CMD_WEATHER_NUMBER, data);
        }

        void loadTimer() {
            BLINKER_LOG1((
                "\n==========================================================="
                "\n================== Blinker Timer loaded! =================="
                "\nWarning!EEPROM address 1536-2432 is used for Blinker Timer!"
                "\n============= DON'T USE THESE EEPROM ADDRESS! ============="
                "\n===========================================================\n"));

            checkTimerErase();
            loadCountdown();
            loadLoop();
        }

        void deletTimer() {
            EEPROM.begin(BLINKER_EEP_SIZE);

            for (uint16_t _addr = BLINKER_EEP_ADDR_TIMER;
                _addr < BLINKER_EEP_ADD_TIMER_END; _addr++) {
                EEPROM.put(_addr, "\0");
            }

            EEPROM.commit();
            EEPROM.end();
        }
#endif

        void attachHeartbeat(callbackFunction newFunction) {
            _heartbeatFunc = newFunction;
        }

#if defined(BLINKER_PRO)
        void attachParse(callback_with_json_arg_t newFunction) {
            _parseFunc = newFunction;
        }

        void attachClick(callbackFunction newFunction) {
            _clickFunc = newFunction;
        }

        void attachDoubleClick(callbackFunction newFunction) {
            _doubleClickFunc = newFunction;
        }

        void attachLongPressStart(callbackFunction newFunction) {
            _longPressStartFunc = newFunction;
        }

        void attachLongPressStop(callbackFunction newFunction) {
            _longPressStopFunc = newFunction;
        }

        void attachDuringLongPress(callbackFunction newFunction) {
            _duringLongPressFunc = newFunction;
        }

    #if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
        void attachLongPressPowerdown(callbackFunction newFunction) {
            _powerdownFunc = newFunction;
        }

        void attachLongPressReset(callbackFunction newFunction) {
            _resetFunc = newFunction;
        }

        uint16_t pressedTime()
        {
            uint32_t _pressedTime = millis() - _startTime;

            if (_isLongPressed) {
                if (_pressedTime >= BLINKER_PRESSTIME_RESET) return BLINKER_PRESSTIME_RESET;
                else return _pressedTime;
            }
            else {
                return 0;
            }
        }
    #endif

        void setType(const char* _type) {
            _deviceType = _type;

            Bwlan.setType(_type);

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("API deviceType: "), _type);
    #endif
        }

        const char* type() {
            return _deviceType;
        }

        void reset() {
            BLINKER_LOG1(BLINKER_F("Blinker reset..."));
            char _authCheck = 0x00;
            char _uuid[BLINKER_AUUID_SIZE] = {0};
            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.put(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
            EEPROM.put(BLINKER_EEP_ADDR_AUUID, _uuid);
            EEPROM.commit();
            EEPROM.end();
            Bwlan.deleteConfig();
            Bwlan.reset();
            ESP.restart();
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

            // BLINKER_LOG2("_state: ", _state);
        }
#endif

    private :
        // bool        _switchFresh = false;
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_int = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_joy = 0;
        // uint8_t     _bCount = 0;
        // uint8_t     _nbCount = 0;
        // uint8_t     _nbCount_test = 0;
        // uint8_t     _sCount = 0;
        // uint8_t     _tCount = 0;
        // uint8_t     _rgbCount = 0;
        // uint8_t     joyValue[2];
        int16_t     ahrsValue[3];
        uint32_t    gps_get_time;
        String      gpsValue[2];
        // uint8_t rgbValue[3];
        bool        _fresh = false;
        bool        _isNTPInit = false;
        float       _timezone = 8.0;
        uint32_t    _ntpStart;
        time_t      now_ntp;
        struct tm   timeinfo;

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        uint8_t     _bridgeCount = 0;
        uint8_t     _dataCount = 0;
        uint8_t     _aCount = 0;
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        uint32_t    _smsTime = 0;

        uint32_t    _pushTime = 0;
        uint32_t    _wechatTime = 0;
        uint32_t    _weatherTime = 0;
        uint32_t    _aqiTime = 0;

        uint32_t    _cUpdateTime = 0;
        uint32_t    _dUpdateTime = 0;
        uint32_t    _cGetTime = 0;
#endif

#if defined(ESP8266) || defined(ESP32)
        String      _cdAction;
        String      _lpAction1;
        String      _lpAction2;
        // String      _tmAction1;
        // String      _tmAction2;
        // String      _tmAction;

#endif
// #else
        // void freshNTP() {}
        void freshNTP() {
#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
            if (_isNTPInit) {
                // configTime((long)(_timezone * 3600), 0, "ntp1.aliyun.com", "210.72.145.44", "time.pool.aliyun.com");
                // BLINKER_LOG2("mday1: ", timeinfo.tm_mday);
                now_ntp = ::time(nullptr);
    #if defined(ESP8266)
                // gmtime_r(&now_ntp, &timeinfo);
                gmtime_r(&now_ntp, &timeinfo);
    #elif defined(ESP32)
                localtime_r(&now_ntp, &timeinfo);
                // getLocalTime(&timeinfo, 5000);
    #endif
                // BLINKER_LOG2("mday2: ", timeinfo.tm_mday);
            }
#endif
        }
// #endif

        void strWidgetsParse(const String & _wName)
        {
            int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _wName)) {
                _fresh = true;

                callback_with_string_arg_t wFunc = _Widgets_str[num]->getFunc();
                if (wFunc) {
                    wFunc(state);
                }
            }
        }

        void intWidgetsParse(const String & _wName)
        {
            int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            int _number = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName);

            if (_number != FIND_KEY_VALUE_FAILED) {
                _fresh = true;

                callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
                if (wFunc) {
                    wFunc(_number);
                }
            }
        }

        void rgbWidgetsParse(const String & _wName)
        {
            int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            int16_t _rValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, R);

            if (_rValue != FIND_KEY_VALUE_FAILED) {
                // uint8_t _rValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, R);
                uint8_t _gValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, G);
                uint8_t _bValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, B);
                uint8_t _brightValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, BRGB);

                callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
                if (wFunc) {
                    wFunc(_rValue, _gValue, _bValue, _brightValue);
                }
            }
        }

        void joyWidgetsParse(const String & _wName)
        {
            int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            int16_t jxAxisValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, J_Xaxis);

            if (jxAxisValue != FIND_KEY_VALUE_FAILED) {
                // joyValue[J_Xaxis] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, J_Xaxis);
                uint8_t jyAxisValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _wName, J_Yaxis);

                _fresh = true;
                // return jAxisValue;
                callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();

                if (wFunc) {
                    wFunc(jxAxisValue, jyAxisValue);
                }
            }
        }

#if defined(ESP8266) || defined(ESP32)

        void strWidgetsParse(const String & _wName, const JsonObject& data)
        {
            int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            if (data.containsKey(_wName)) {
                String state = data[_wName];

                _fresh = true;
    #if defined(BLINKER_DEBUG_ALL)
                BLINKER_LOG2("strWidgetsParse: ", _wName);
    #endif
                callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
                if (nbFunc) {
                    nbFunc(state);
                }
            }
        }

        void intWidgetsParse(const String & _wName, const JsonObject& data)
        {
            int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            if (data.containsKey(_wName)) {
                int _number = data[_wName];

                _fresh = true;

                callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
                if (wFunc) {
                    wFunc(_number);
                }
            }
        }

        void rgbWidgetsParse(const String & _wName, const JsonObject& data)
        {
            int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            if (data.containsKey(_wName)) {
                uint8_t _rValue = data[_wName][R];
                uint8_t _gValue = data[_wName][G];
                uint8_t _bValue = data[_wName][B];
                uint8_t _brightValue = data[_wName][BRGB];

                _fresh = true;

                callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
                if (wFunc) {
                    wFunc(_rValue, _gValue, _bValue, _brightValue);
                }
            }
        }

        void joyWidgetsParse(const String & _wName, const JsonObject& data)
        {
            int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            if (data.containsKey(_wName)) {
                int16_t jxAxisValue = data[_wName][J_Xaxis];
                // joyValue[J_Xaxis] = data[_wName][J_Xaxis];
                uint8_t jyAxisValue = data[_wName][J_Yaxis];

                _fresh = true;

                callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();
                if (wFunc) {
                    wFunc(jxAxisValue, jyAxisValue);
                }
            }
        }

        int16_t ahrs(b_ahrsattitude_t attitude, const JsonObject& data)
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

        String gps(b_gps_t axis, bool newData, const JsonObject& data) {
            if (!newData && (millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT) {
                static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
                delay(100);

                gps_get_time = millis();
            }

            if (data.containsKey(BLINKER_CMD_GPS)) {
                String gpsValue_LONG = data[BLINKER_CMD_GPS][LONG];
                String gpsValue_LAT = data[BLINKER_CMD_GPS][LAT];
                gpsValue[LONG] = gpsValue_LONG;
                gpsValue[LAT] = gpsValue_LAT;

                _fresh = true;

                if (_fresh) {
                    static_cast<Proto*>(this)->isParsed();
                }

                return gpsValue[axis];
            }
            else {
                return gpsValue[axis];
            }
        }

        void heartBeat(const JsonObject& data) {
            String state = data[BLINKER_CMD_GET];

            if (state.length()) {
                if (state == BLINKER_CMD_STATE) {
                    static_cast<Proto*>(this)->beginFormat();

#if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
#else
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
#endif  

                    if (_heartbeatFunc) {
                        _heartbeatFunc();
                    }
                    if (!static_cast<Proto*>(this)->endFormat()) {
                        
#if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
#else
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
#endif  

                        static_cast<Proto*>(this)->endFormat();
                    }
                    _fresh = true;
                }
            }
        }

        void setSwitch(const JsonObject& data) {
            String state = data[BLINKER_CMD_SET][BLINKER_CMD_BUILTIN_SWITCH];

            if (state.length()) {
                // if (state == BLINKER_CMD_ON) {
                //     _BUILTIN_SWITCH->freshState(true);
                // }
                // else {
                //     _BUILTIN_SWITCH->freshState(false);
                // }
                callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

                if (sFunc) {
                    sFunc(state);
                }
                _fresh = true;
                // _switchFresh = true;
                // static_cast<Proto*>(this)->print(BLINKER_CMD_BUILTIN_SWITCH, builtInSwitch()?"on":"off");
            }
        }

        void getVersion(const JsonObject& data) {
            String state = data[BLINKER_CMD_GET];

            // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
            if (state.length()) {
                // _fresh = true;
                if (state == BLINKER_CMD_VERSION) {
                    static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_VERSION);
                    _fresh = true;
                }
            }
        }
#else

        void strWidgetsParse(const String & _wName, String _data)
        {
            int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            String state;

            if (STRING_find_string_value(_data, state, _wName)) {
                _fresh = true;

                callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
                if (nbFunc) {
                    nbFunc(state);
                }
            }
        }

        void intWidgetsParse(const String & _wName, String _data)
        {
            int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            int _number = STRING_find_numberic_value(_data, _wName);

            if (_number != FIND_KEY_VALUE_FAILED) {
                _fresh = true;

                callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
                if (wFunc) {
                    wFunc(_number);
                }
            }
        }

        void rgbWidgetsParse(const String & _wName, String _data)
        {
            int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            int16_t _rValue = STRING_find_array_numberic_value(_data, _wName, R);

            if (_rValue != FIND_KEY_VALUE_FAILED) {
                // uint8_t _rValue = STRING_find_array_numberic_value(_data, _wName, R);
                uint8_t _gValue = STRING_find_array_numberic_value(_data, _wName, G);
                uint8_t _bValue = STRING_find_array_numberic_value(_data, _wName, B);
                uint8_t _brightValue = STRING_find_array_numberic_value(_data, _wName, BRGB);

                callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
                if (wFunc) {
                    wFunc(_rValue, _gValue, _bValue, _brightValue);
                }
            }
        }

        void joyWidgetsParse(const String & _wName, String _data)
        {
            int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

            if (num == BLINKER_OBJECT_NOT_AVAIL) {
                return;
            }

            int16_t jxAxisValue = STRING_find_array_numberic_value(_data, _wName, J_Xaxis);

            if (jxAxisValue != FIND_KEY_VALUE_FAILED) {
                // joyValue[J_Xaxis] = STRING_find_array_numberic_value(_data, _wName, J_Xaxis);
                uint8_t jyAxisValue = STRING_find_array_numberic_value(_data, _wName, J_Yaxis);

                // return jAxisValue;
                callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();

                if (wFunc) {
                    wFunc(jxAxisValue, jyAxisValue);
                }
            }
        }
#endif

        void heartBeat() {
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
            // if (state.length()) {
                // _fresh = true;
                if (state == BLINKER_CMD_STATE) {
                    static_cast<Proto*>(this)->beginFormat();

#if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
#else
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
#endif

                    if (_heartbeatFunc) {
                        _heartbeatFunc();
                    }

                    if (!static_cast<Proto*>(this)->endFormat()) {
                        static_cast<Proto*>(this)->beginFormat();

#if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
#else
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
#endif                        
                        static_cast<Proto*>(this)->endFormat();
                    }
                    _fresh = true;
                }
            }
        }

        void setSwitch() {
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_BUILTIN_SWITCH)) {
                if (STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET)) {
                    // if (state == BLINKER_CMD_ON) {
                    //     _BUILTIN_SWITCH->freshState(true);
                    // }
                    // else {
                    //     _BUILTIN_SWITCH->freshState(false);
                    // }
                    callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

                    if (sFunc) {
                        sFunc(state);
                    }
                    _fresh = true;
                    // _switchFresh = true;
                    // static_cast<Proto*>(this)->print(BLINKER_CMD_BUILTIN_SWITCH, builtInSwitch()?"on":"off");
                }
            }
            // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
            // // if (state.length()) {
            //     _fresh = true;
            //     if (state == BLINKER_CMD_VERSION) {
            //         static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_VERSION);
            //         _fresh = true;
            //     }
            // }
        }

        void setSwitch(const String & data) {
            String state;

            if (STRING_find_string_value(data, state, BLINKER_CMD_BUILTIN_SWITCH)) {
                if (STRING_contains_string(data, BLINKER_CMD_SET)) {
                    // if (state == BLINKER_CMD_ON) {
                    //     _BUILTIN_SWITCH->freshState(true);
                    // }
                    // else {
                    //     _BUILTIN_SWITCH->freshState(false);
                    // }
                    callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

                    if (sFunc) {
                        sFunc(state);
                    }
                    _fresh = true;
                    // _switchFresh = true;
                    // static_cast<Proto*>(this)->print(BLINKER_CMD_BUILTIN_SWITCH, builtInSwitch()?"on":"off");
                }
            }
        }

        void getVersion() {
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
            // if (state.length()) {
                _fresh = true;
                if (state == BLINKER_CMD_VERSION) {
                    static_cast<Proto*>(this)->print(BLINKER_CMD_VERSION, BLINKER_VERSION);
                    _fresh = true;
                }
            }
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        bool autoManager(const JsonObject& data) {
            // String set;
            bool isSet = false;
            bool isAuto = false;

            // isSet = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET);
            // isAuto = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTODATA);
            isSet = data.containsKey(BLINKER_CMD_SET);
            String aData = data[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA];
            if (aData.length()) {
                isAuto = true;
            }

            if (isSet && isAuto) {
                _fresh = true;
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("get auto setting"));
    #endif
                bool isDelet = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DELETID);
                String isTriggedArray = data[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][0];

                if (isDelet) {
                    // uint32_t _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DELETID);
                    uint32_t _autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_DELETID];

                    if (_aCount) {
                        for (uint8_t _num = 0; _num < _aCount; _num++) {
                            if (_AUTO[_num]->id() == _autoId) {
                                // _AUTO[_num]->manager(static_cast<Proto*>(this)->dataParse());
                                for (_num; _num < _aCount; _num++) {
                                    if (_num < _aCount - 1) {
                                        _AUTO[_num]->setNum(_num + 1);
                                        _AUTO[_num]->deserialization();
                                        _AUTO[_num]->setNum(_num);
                                        _AUTO[_num]->serialization();
                                    }
                                }
                                _aCount--;

                                EEPROM.begin(BLINKER_EEP_SIZE);
                                EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                                EEPROM.commit();
                                EEPROM.end();

    #ifdef BLINKER_DEBUG_ALL
                                BLINKER_LOG2(BLINKER_F("_aCount: "), _aCount);
    #endif
                                return true;
                            }
                        }
                    }
                }
                else if(isTriggedArray.length()) {
                    for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++) {
                        String _autoData_array = data[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][a_num];

                        if(_autoData_array.length()) {
                            DynamicJsonBuffer _jsonBuffer;
                            JsonObject& _array = _jsonBuffer.parseObject(_autoData_array);

                            json_parse(_array);
                            timerManager(_array, true);
                        }
                        else {
                            // a_num = BLINKER_MAX_WIDGET_SIZE;
                            return true;
                        }
                    }
                }
                else {
                    // uint32_t _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);
                    uint32_t _autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_AUTOID];

                    // _aCount = 0;

                    if (_aCount) {
                        for (uint8_t _num = 0; _num < _aCount; _num++) {
                            if (_AUTO[_num]->id() == _autoId) {
                                _AUTO[_num]->manager(static_cast<Proto*>(this)->dataParse());
                                return true;
                            }
                        }
                        if (_aCount == 1) {
                            _AUTO[_aCount] = new BlinkerAUTO();
                            _AUTO[_aCount]->setNum(_aCount);
                            _AUTO[_aCount]->manager(static_cast<Proto*>(this)->dataParse());

                            // _aCount = 1;
                            _aCount++;
                            EEPROM.begin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                            EEPROM.commit();
                            EEPROM.end();

    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("_aCount: "), _aCount);
    #endif
                            // static_cast<Proto*>(this)->_print(autoData(), false);
                            // return true;
                        }
                        else {
                            _AUTO[_aCount - 1]->manager(static_cast<Proto*>(this)->dataParse());
                            // return true;
                        }
                    }
                    else {
                        _AUTO[_aCount] = new BlinkerAUTO();
                        _AUTO[_aCount]->setNum(_aCount);
                        _AUTO[_aCount]->manager(static_cast<Proto*>(this)->dataParse());

                        _aCount = 1;
                        // _aCount++;
                        EEPROM.begin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                        EEPROM.commit();
                        EEPROM.end();

    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_aCount: "), _aCount);
    #endif
                        // static_cast<Proto*>(this)->_print(autoData(), false);
                        // return true;
                    }
                }
                return true;
            }
            else {
                return false;
            }
        }

        void autoStart() {
            uint8_t checkData;

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);
            if (checkData != BLINKER_CHECK_DATA) {
                EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
                EEPROM.commit();
                EEPROM.end();
                return;
            }
            EEPROM.get(BLINKER_EEP_ADDR_AUTONUM, _aCount);
            if (_aCount > 2) {
                _aCount = 0;
                EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
            }
            EEPROM.commit();
            EEPROM.end();

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("_aCount: "), _aCount);
    #endif

            if (_aCount) {
                for (uint8_t _num = 0; _num < _aCount; _num++) {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("new BlinkerAUTO() _num: "), _num);
    #endif
                    _AUTO[_num] = new BlinkerAUTO();
                    _AUTO[_num]->setNum(_num);
                    _AUTO[_num]->deserialization();
                }
            }
        }
#endif

#if defined(ESP8266) || defined(ESP32)
        void checkOverlapping(uint8_t checkDays, uint16_t checkMins) {
            for (uint8_t task = 0; task < taskCount; task++) {
                if((timingTask[task]->getTime() == checkMins) && !timingTask[task]->isLoop() \
                    && timingTask[task]->state() && timingTask[task]->isTimingDay(checkDays)) {

                    timingTask[task]->disableTask();

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
                    
                    // char _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];
                    // strcpy(_tmAction_, timingTask[task]->getAction().c_str());

                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE
                                , timingTask[task]->getTimerData());
                    // EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE + 
                    //             BLINKER_TIMER_TIMING_SIZE, _tmAction_);
    
                    EEPROM.commit();
                    EEPROM.end();
                    
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("disable timerData: "), timingTask[task]->getTimerData());
                    // BLINKER_LOG2(BLINKER_F("disable _tmAction_: "), _tmAction_);
                    BLINKER_LOG2(F("disableTask: "), task);
    #endif
                }
            }
        }

        void freshTiming(uint8_t wDay, uint16_t nowMins) {
            tmTicker.detach();

            uint8_t  cbackData;
            uint8_t  nextTask = BLINKER_TIMING_TIMER_SIZE;
            uint16_t timingMinsNext;
            uint32_t apartSeconds = BLINKER_ONE_DAY_TIME;
            uint32_t checkSeconds = BLINKER_ONE_DAY_TIME;
            uint32_t nowSeconds = dtime();

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6("freshTiming wDay: ", wDay, ", nowMins: ", nowMins, ", nowSeconds: ", nowSeconds);
    #endif

            for (uint8_t task = 0; task < taskCount; task++) {
                if (timingTask[task]->isTimingDay(wDay) && timingTask[task]->state()) {
                    if (timingTask[task]->getTime() > nowMins) {
                        checkSeconds = timingTask[task]->getTime() * 60 - nowSeconds;

                        if (checkSeconds <= apartSeconds) {
                            apartSeconds = checkSeconds;
                            nextTask = task;
                        }
                    }
                }
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2("isTimingDay: ", timingTask[task]->isTimingDay(wDay));
                BLINKER_LOG2("state: ", timingTask[task]->state());
                BLINKER_LOG2("getTime: ", timingTask[task]->getTime());

                BLINKER_LOG6("for nextTask: ", nextTask, "  apartSeconds: ", apartSeconds, " wDay: ", wDay);
    #endif
            }

            if (apartSeconds == BLINKER_ONE_DAY_TIME) {
                apartSeconds -= nowSeconds;
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG6("nextTask: ", nextTask, "  apartSeconds: ", apartSeconds, " wDay: ", wDay);
    #endif
                cbackData = nextTask;
            }
            else {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG6("nextTask: ", nextTask, "  apartSeconds: ", apartSeconds, " wDay: ", wDay);
    #endif
                cbackData = nextTask;
            }
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("cbackData: ", cbackData);
    #endif
            tmTicker.once(apartSeconds, timingHandle, cbackData);
        }

        void deleteTiming(uint8_t taskDel) {
            if (taskDel < taskCount) {
                tmTicker.detach();

                for (uint8_t task = taskDel; task < (taskCount - 2); task++) {
                    // timingTask[task]->freshTimer(timingTask[task + 1]->getTimerData(), 
                    //     timingTask[task + 1]->getAction(), timingTask[task + 1]->getText());
                    timingTask[task]->freshTimer(timingTask[task + 1]->getTimerData(), 
                                                timingTask[task + 1]->getAction());
                }

                delete timingTask[taskCount - 1];

                taskCount--;
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG3(("delete task: "), taskDel, " success!");
    #endif
                uint8_t  wDay = wday();
                uint16_t nowMins = hour() * 60 + minute();
                freshTiming(wDay, nowMins);
            }
            else {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("none task to delete!");
    #endif
            }
        }

        // void addTimingTask(uint8_t taskSet, uint32_t timerData, String action, String text) {
        void addTimingTask(uint8_t taskSet, uint32_t timerData, String action) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("addTimingTask taskSet: ", taskSet);
            BLINKER_LOG2("addTimingTask timerData: ", timerData);
    #endif
            if (taskSet <= taskCount && taskCount <= BLINKER_TIMING_TIMER_SIZE) {
                // tmTicker.detach();

                if (taskSet == taskCount) {
                    if (taskCount == BLINKER_TIMING_TIMER_SIZE) {
                        BLINKER_ERR_LOG1("timing timer task is full");
                        return;
                    }
                    // timingTask[taskSet] = new BlinkerTimingTimer(timerData, action, text);
                    timingTask[taskSet] = new BlinkerTimingTimer(timerData, action);
                    taskCount++;
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("new BlinkerTimingTimer"));
    #endif
                }
                else {
                    // timingTask[taskSet]->freshTimer(timerData, action, text);
                    timingTask[taskSet]->freshTimer(timerData, action);
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("freshTimer"));
    #endif
                }

                // if (taskSet <= taskCount) taskCount++;
                
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("taskCount: "), taskCount);
    #endif
                uint8_t  wDay = wday();
                uint16_t nowMins = hour() * 60 + minute();

                freshTiming(wDay, nowMins);
            }
            else {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("timing timer task is full");
    #endif
            }
        }

        void checkTimerErase() {
            uint8_t isErase;

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_ERASE, isErase);

            if (isErase) {
                for (uint16_t _addr = BLINKER_EEP_ADDR_TIMER;
                    _addr < BLINKER_EEP_ADD_TIMER_END; _addr++) {
                    EEPROM.put(_addr, "\0");
                }
            }

            EEPROM.commit();
            EEPROM.end();
        }

        void saveCountDown(uint16_t _data, const String & _action) {
            char _action_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];

            strcpy(_action_, _action.c_str());

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _data);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _action_);
            EEPROM.commit();
            EEPROM.end();
        }

        void loadCountdown() {
            // _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);

            char _cdAction_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction_);
            EEPROM.commit();
            EEPROM.end();

            _cdState    = _cdData >> 15;
            _cdRunState = _cdData >> 14 & 0x0001;
            _cdTime1    = _cdData       & 0x0FFF;
            _cdTime2    = 0;

            _cdAction = STRING_format(_cdAction_);

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
            BLINKER_LOG2(BLINKER_F("_cdRunState: "), _cdRunState);
            BLINKER_LOG2(BLINKER_F("_totalTime: "), _cdTime1);
            BLINKER_LOG2(BLINKER_F("_runTime: "), _cdTime2);
            BLINKER_LOG2(BLINKER_F("_action: "), _cdAction);
    #endif

            if (_cdState && _cdRunState) {
                cdTicker.once(_cdTime1 * 60, _cd_callback);

                _cdStart = millis();
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("countdown start!"));
    #endif
            }
        }

        void saveLoop(uint32_t _data, const String & _action1, const String & _action2) {
            char _action_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
            char _action_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];

            strcpy(_action_1, _action1.c_str());
            strcpy(_action_2, _action2.c_str());

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _data);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _action_1);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _action_2);
            EEPROM.commit();
            EEPROM.end();
        }

        void loadLoop() {
            // _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;

            char _lpAction_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
            char _lpAction_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];

            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction_1);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction_2);
            EEPROM.commit();
            EEPROM.end();

            _lpState    = _lpData >> 31;
            _lpRunState = _lpData >> 30 & 0x0001;
            _lpTimes    = _lpData >> 22 & 0x007F;
            _lpTime1    = _lpData >> 11 & 0x07FF;
            _lpTime2    = _lpData       & 0x07FF;

            _lpAction1 = STRING_format(_lpAction_1);
            _lpAction2 = STRING_format(_lpAction_2);

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("loop state: "), _lpState ? "true" : "false");
            BLINKER_LOG2(BLINKER_F("_lpRunState: "), _lpRunState);
            BLINKER_LOG2(BLINKER_F("_times: "), _lpTimes);
            BLINKER_LOG2(BLINKER_F("_time1: "), _lpTime1);
            BLINKER_LOG2(BLINKER_F("_action1: "), _lpAction1);
            BLINKER_LOG2(BLINKER_F("_time2: "), _lpTime2);
            BLINKER_LOG2(BLINKER_F("_action2: "), _lpAction2);
            BLINKER_LOG2(BLINKER_F("_lpData: "), _lpData);
    #endif

            if (_lpState && _lpRunState && (_lpTimes == 0)) {
                _lpRun1 = true;
                _lpTrigged_times = 0;
                _lpStop = false;
                lpTicker.once(_lpTime1 * 60, _lp_callback);
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("loop start!"));
    #endif
            }
        }

        void loadTiming() {
            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
            uint32_t _tmData;
            char     _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("load timing taskCount: "), taskCount);
    #endif
            for(uint8_t task = 0; task < taskCount; task++) {
                EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE 
                            , _tmData);
                EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE + 
                            BLINKER_TIMER_TIMING_SIZE, _tmAction_);

                timingTask[task] = new BlinkerTimingTimer(_tmData, STRING_format(_tmAction_));
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_tmData: "), _tmData);
                BLINKER_LOG2(BLINKER_F("_tmAction: "), STRING_format(_tmAction_));
    #endif
            }
            EEPROM.commit();
            EEPROM.end();

            uint8_t  wDay = wday();
            uint16_t nowMins = hour() * 60 + minute();

            freshTiming(wDay, nowMins);
        }

        bool timerManager(const JsonObject& data, bool _noSet = false) {
            bool isSet = false;
            bool isCount = false;
            bool isLoop = false;
            bool isTiming = false;

            if (!_noSet) {
                isSet = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET);
                isCount = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_COUNTDOWN);
                isLoop = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_LOOP);
                isTiming = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMING);
            }
            else {
                isCount = data.containsKey(BLINKER_CMD_COUNTDOWN);
                isLoop = data.containsKey(BLINKER_CMD_LOOP);
                isTiming = data.containsKey(BLINKER_CMD_TIMING);
            }

            if ((isSet || _noSet) && (isCount || isLoop || isTiming)) {
                _fresh = true;

    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("get timer setting"));
    #endif

                if (isCount) {

                    String _delete = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN];

                    if (_delete == "dlt") _cdState = false;
                    else _cdState = true;

                    if (isSet) {
                        _cdRunState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUN];
                    }
                    else if(_noSet) {
                        _cdRunState = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUN];
                    }

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
    #endif

                    if (isSet) {
                        // _cdRunState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_STATE];
                        // _cdRunState = _cdState;
                        int32_t _totalTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_TOTALTIME];
                        // _totalTime = 60 * _totalTime;
                        int32_t _runTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUNTIME];
                        // _runTime = 60 * _runTime;
                        String _action = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_ACTION];

                        if (_action.length() > BLINKER_TIMER_COUNTDOWN_ACTION_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        _cdAction = _action;
                        _cdTime1 = _totalTime;
                        _cdTime2 = _runTime;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_cdRunState: "), _cdRunState);
    #endif
                    }
                    else if (_noSet) {
                        // _cdRunState = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_STATE];
                        // _cdRunState = _cdState;
                        int32_t _totalTime = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_TOTALTIME];
                        // _totalTime = 60 * _totalTime;
                        int32_t _runTime = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUNTIME];
                        // _runTime = 60 * _runTime;
                        String _action = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_ACTION];

                        if (_action.length() > BLINKER_TIMER_COUNTDOWN_ACTION_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        _cdAction = _action;
                        _cdTime1 = _totalTime;
                        _cdTime2 = _runTime;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_cdRunState: "), _cdRunState);
    #endif
                    }

                    _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_totalTime: "), _cdTime1);
                    BLINKER_LOG2(BLINKER_F("_runTime: "), _cdTime2);
                    BLINKER_LOG2(BLINKER_F("_action: "), _cdAction);
                    BLINKER_LOG2(BLINKER_F("_cdData: "), _cdData);
    #endif

                    char _cdAction_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
                    strcpy(_cdAction_, _cdAction.c_str());

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction_);
                    EEPROM.commit();
                    EEPROM.end();
    
                    if (_cdState && _cdRunState) {
                        cdTicker.once((_cdTime1 - _cdTime2) * 60, _cd_callback);

                        _cdStart = millis();
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(BLINKER_F("countdown start!"));
    #endif
                    }
                    else {
                        cdTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(countdownConfig(), false, false);
                    return true;
                }
                else if (isLoop) {
                    _lpState = true;

                    String _delete = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP];

                    if (_delete == "del") _lpState = false;
                    else _lpState = true;

                    if (isSet) {
                        _lpRunState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_RUN];
                    }
                    else if (_noSet) {
                        _lpRunState = data[BLINKER_CMD_LOOP][BLINKER_CMD_RUN];
                    }
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("loop state: "), _lpState ? "true" : "false");
    #endif
                    if (isSet) {
                        int8_t _times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIMES];
                        // _lpRunState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_STATE];
                        // _lpRunState = _lpState;
                        int32_t _time1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIME1];
                        // _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIME2];
                        // _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_ACTION2];

                        if (_action1.length() > BLINKER_TIMER_LOOP_ACTION1_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        if (_action2.length() > BLINKER_TIMER_LOOP_ACTION2_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        _lpAction1 = _action1;
                        _lpAction2 = _action2;

                        _lpTimes = _times;
                        _lpTime1 = _time1;
                        _lpTime2 = _time2;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_lpRunState: "), _lpRunState);
    #endif
                    }
                    else if (_noSet) {
                        int8_t _times = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIMES];
                        // _lpRunState = data[BLINKER_CMD_LOOP][BLINKER_CMD_STATE];
                        // _lpRunState = _lpState;
                        int32_t _time1 = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIME1];
                        // _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_LOOP][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIME2];
                        // _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_LOOP][BLINKER_CMD_ACTION2];

                        if (_action1.length() > BLINKER_TIMER_LOOP_ACTION1_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        if (_action2.length() > BLINKER_TIMER_LOOP_ACTION2_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        _lpAction1 = _action1;
                        _lpAction2 = _action2;

                        _lpTimes = _times;
                        _lpTime1 = _time1;
                        _lpTime2 = _time2;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_lpRunState: "), _lpRunState);
    #endif
                    }

                    if (_lpTimes > 100) _lpTimes = 0;

                    _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_times: "), _lpTimes);
                    BLINKER_LOG2(BLINKER_F("_time1: "), _lpTime1);
                    BLINKER_LOG2(BLINKER_F("_action1: "), _lpAction1);
                    BLINKER_LOG2(BLINKER_F("_time2: "), _lpTime2);
                    BLINKER_LOG2(BLINKER_F("_action2: "), _lpAction2);
                    BLINKER_LOG2(BLINKER_F("_lpData: "), _lpData);
    #endif

                    char _lpAction_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
                    char _lpAction_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
                    strcpy(_lpAction_1, _lpAction1.c_str());
                    strcpy(_lpAction_2, _lpAction2.c_str());

                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction_1);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction_2);
                    EEPROM.commit();
                    EEPROM.end();

                    if (_lpState && _lpRunState) {
                        _lpRun1 = true;
                        _lpTrigged_times = 0;
                        _lpStop = false;
                        lpTicker.once(_lpTime1 * 60, _lp_callback);
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(BLINKER_F("loop start!"));
    #endif
                    }
                    else {
                        lpTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(loopConfig(), false, false);
                    return true;
                }
                else if (isTiming) {
                    bool isDelet = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DELETETASK);

                    _tmState = true;
                    
                    if (isSet) {
                        _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_ENABLE];
                    }
                    else if (_noSet) {
                        _tmRunState = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_ENABLE];
                    }
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("timing state: "), _tmState ? "true" : "false");
    #endif

                    int32_t _time;
                    String _action;
                    String _text;
                    uint8_t _task;

                    if (isSet && !isDelet) {
                        // _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_STATE];
                        // _tmRunState = _tmState;
                        _time = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TIME];
                        // _time = 60 * _time;
                        String tm_action = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_ACTION];
                        // String tm_text = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TEXT];
                        _task = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TASK];

                        // _tmAction = _action;

                        if (tm_action.length() > BLINKER_TIMER_TIMING_ACTION_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        _action = tm_action;
                        // _text = tm_text;

                        // _tmTime = _time;

                        String tm_day = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY];

                        if (tm_day.toInt() == 0) {
                            if (60 * _time > dtime()) {
                                _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
                            }
                            else {
                                _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
                            }

                            _isTimingLoop = false;
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
    #endif
                        }
                        else {

                            _isTimingLoop = true;

                            // uint8_t taskDay;

                            for (uint8_t day = 0; day < 7; day++) {
                                if (tm_day.substring(day, day+1) == "1") {
                                    _timingDay |= (0x01 << day);
    #ifdef BLINKER_DEBUG_ALL
                                    BLINKER_LOG4(BLINKER_F("day: "), day, BLINKER_F(" timingDay: "), _timingDay);
    #endif
                                }
                            }
                        }

    //                     if (data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0] == 7) {
    //                         if (_tmTime2 > dtime()) {
    //                             _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
    //                         }
    //                         else {
    //                             _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
    //                         }

    //                         _isTimingLoop = false;
    // #ifdef BLINKER_DEBUG_ALL
    //                         BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
    // #endif
    //                     }
    //                     else {
    //                         uint8_t taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0];
    //                         _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    // #ifdef BLINKER_DEBUG_ALL
    //                         BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    // #endif

    //                         for (uint8_t day = 1;day < 7;day++) {
    //                             taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][day];
    //                             if (taskDay > 0) {
    //                                 _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    // #ifdef BLINKER_DEBUG_ALL
    //                                 BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    // #endif
    //                             }
    //                         }

    //                         _isTimingLoop = true;
    //                     }

    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
                        // BLINKER_LOG2(BLINKER_F("_text: "), _text);
                        BLINKER_LOG2(BLINKER_F("_tmRunState: "), _tmRunState);
    #endif

    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                        BLINKER_LOG2(BLINKER_F("_time: "), _time);
                        BLINKER_LOG2(BLINKER_F("_action: "), _action);
    #endif
                        uint32_t _timerData = _isTimingLoop << 31 | _tmRunState << 23 | _timingDay << 11 | _time;

                        // addTimingTask(_task, _timerData, _action, _text);
                        addTimingTask(_task, _timerData, _action);
                    }
                    else if (_noSet) {
                        // _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_STATE];
                        // _tmRunState = _tmState;
                        _time = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_TIME];
                        // _time = 60 * _time;
                        String tm_action = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_ACTION];
                        // String tm_text = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TEXT];
                        _task = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_TASK];

                        // _tmAction = _action;

                        if (tm_action.length() > BLINKER_TIMER_TIMING_ACTION_SIZE) {
                            BLINKER_ERR_LOG1("TIMER ACTION TOO LONG");
                            return true;
                        }

                        _action = tm_action;
                        // _text = tm_text;

                        // _tmTime = _time;

                        String tm_day = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY];

                        if (tm_day.toInt() == 0) {
                            if (60 * _time > dtime()) {
                                _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
                            }
                            else {
                                _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
                            }

                            _isTimingLoop = false;
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
    #endif
                        }
                        else {
                            // uint8_t taskDay;

                            _isTimingLoop = true;

                            for (uint8_t day = 0; day < 7; day++) {
                                if (tm_day.substring(day, day+1) == "1") {
                                    _timingDay |= (0x01 << day);
    #ifdef BLINKER_DEBUG_ALL
                                    BLINKER_LOG4(BLINKER_F("day: "), day, BLINKER_F(" timingDay: "), _timingDay);
    #endif
                                }
                            }
                        }

    //                     if (data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0] == 7) {
    //                         if (_tmTime2 > dtime()) {
    //                             _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
    //                         }
    //                         else {
    //                             _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
    //                         }

    //                         _isTimingLoop = false;
    // #ifdef BLINKER_DEBUG_ALL
    //                         BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
    // #endif
    //                     }
    //                     else {
    //                         uint8_t taskDay = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0];
    //                         _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    // #ifdef BLINKER_DEBUG_ALL
    //                         BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    // #endif

    //                         for (uint8_t day = 1;day < 7;day++) {
    //                             taskDay = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][day];
    //                             if (taskDay > 0) {
    //                                 _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    // #ifdef BLINKER_DEBUG_ALL
    //                                 BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    // #endif
    //                             }
    //                         }

    //                         _isTimingLoop = true;
    //                     }

    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
                        // BLINKER_LOG2(BLINKER_F("_text: "), _text);
                        BLINKER_LOG2(BLINKER_F("_tmRunState: "), _tmRunState);
    #endif

    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                        BLINKER_LOG2(BLINKER_F("_time: "), _time);
                        BLINKER_LOG2(BLINKER_F("_action: "), _action);
    #endif
                        uint32_t _timerData = _isTimingLoop << 31 | _tmRunState << 23 | _timingDay << 11 | _time;

                        // addTimingTask(_task, _timerData, _action, _text);
                        addTimingTask(_task, _timerData, _action);
                    }
                    else if (isDelet) {
                        uint8_t _delTask = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DELETETASK];

                        deleteTiming(_delTask);
                    }

                    char _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];
                    
                    EEPROM.begin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
                    for(uint8_t task = 0; task < taskCount; task++) {
                        strcpy(_tmAction_, timingTask[task]->getAction().c_str());

                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE
                                    , timingTask[task]->getTimerData());
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE + 
                                    BLINKER_TIMER_TIMING_SIZE, _tmAction_);
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("getTimerData: "), timingTask[task]->getTimerData());
                        BLINKER_LOG2(BLINKER_F("_tmAction_: "), _tmAction_);
    #endif
                    }
                    EEPROM.commit();
                    EEPROM.end();

                    static_cast<Proto*>(this)->_print(timingConfig(), false, false);
                    return true;
                }
            }
            else if (data.containsKey(BLINKER_CMD_GET)) {
                String get_timer = data[BLINKER_CMD_GET];

                if (get_timer == BLINKER_CMD_TIMER) {
                    static_cast<Proto*>(this)->_print(timerSetting(), false, false);
                    _fresh = true;
                    return true;
                }
                else if (get_timer == BLINKER_CMD_COUNTDOWN) {
                    static_cast<Proto*>(this)->_print(countdownConfig(), false, false);
                    _fresh = true;
                    return true;
                }
                else if (get_timer == BLINKER_CMD_LOOP) {
                    static_cast<Proto*>(this)->_print(loopConfig(), false, false);
                    _fresh = true;
                    return true;
                }
                else if (get_timer == BLINKER_CMD_TIMING) {
                    static_cast<Proto*>(this)->_print(timingConfig(), false, false);
                    _fresh = true;
                    return true;
                }
            }
            else {
                return false;
            }
        }

        String timerSetting() {
            String _data = "{\""BLINKER_CMD_COUNTDOWN"\":" + STRING_format(_cdState ? "true" : "false") + \
                            "\""BLINKER_CMD_LOOP"\":" + STRING_format(_lpState ? "true" : "false") + \
                            "\""BLINKER_CMD_TIMING"\":" + STRING_format(taskCount ? "true" : "false") + "}";

            return _data;
        }

        String countdownConfig() {
            String cdData;

            if (!_cdState) {
                cdData = "{\""BLINKER_CMD_COUNTDOWN"\":false}";
            }
            else {
                cdData = "{\""BLINKER_CMD_COUNTDOWN"\":{\""BLINKER_CMD_RUN"\":" + STRING_format(_cdRunState ? 1 : 0) + \
                    ",\""BLINKER_CMD_TOTALTIME"\":" + STRING_format(_cdTime1) + \
                    ",\""BLINKER_CMD_RUNTIME"\":" + STRING_format((millis() - _cdStart) / 1000 / 60) + \
                    ",\""BLINKER_CMD_ACTION"\":" + _cdAction + \
                    "}}";
            }

            return cdData;
        }

        String loopConfig() {
            String lpData;
            if (!_lpState) {
                lpData = "{\""BLINKER_CMD_LOOP"\":false}";
            }
            else {
                lpData = "{\""BLINKER_CMD_LOOP"\":{\""BLINKER_CMD_TIMES"\":" + STRING_format(_lpTimes) + \
                    ",\""BLINKER_CMD_RUN"\":" + STRING_format(_lpRunState ? 1 : 0) + \
                    ",\""BLINKER_CMD_TIME1"\":" + STRING_format(_lpTime1) + \
                    ",\""BLINKER_CMD_ACTION1"\":" + _lpAction1 + \
                    ",\""BLINKER_CMD_TIME2"\":" + STRING_format(_lpTime2) + \
                    ",\""BLINKER_CMD_ACTION2"\":" + _lpAction2 + \
                    "}}";
            }

            return lpData;
        }

        String getTimingCfg(uint8_t task) {
            String timingDayStr = "";
            uint8_t timingDay = timingTask[task]->getTimingday();
            if (timingTask[task]->isLoop()) {
                for (uint8_t day = 0; day < 7; day++) {
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

                BLINKER_LOG2(F("timingDayStr: "), timingDayStr);

            }
            else {
                timingDayStr = "0000000";

                BLINKER_LOG2(F("timingDayStr: "), timingDay);
            }

            String timingConfig = "{\""BLINKER_CMD_TASK"\":" + STRING_format(task) + \
                ",\""BLINKER_CMD_ENABLE"\":" + STRING_format((timingTask[task]->state()) ? 1 : 0) + \
                ",\""BLINKER_CMD_DAY"\":\"" + timingDayStr + "\"" + \
                ",\""BLINKER_CMD_TIME"\":" + STRING_format(timingTask[task]->getTime()) + \
                ",\""BLINKER_CMD_ACTION"\":" + timingTask[task]->getAction() + "}";

            return timingConfig;
        }

        String timingConfig() {
            String timingTaskStr = "{\""BLINKER_CMD_TIMING"\":[";
            for (uint8_t task = 0; task < taskCount; task++) {
                //Serial.print(timingTask[task].getTimingCfg());
                timingTaskStr += getTimingCfg(task);
                if (task + 1 < taskCount) {
                    //Serial.println(",");
                    timingTaskStr += ",";
                }
                // else {
                //     Serial.println("");
                // }
            }
            timingTaskStr += "]}";
            
            BLINKER_LOG2(F("timingTaskStr: "), timingTaskStr);
            
            return timingTaskStr;
        }
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        // bool _smsSend(String msg, bool state = false) {
        String blinkServer(uint8_t _type, String msg, bool state = false) {
            switch (_type) {
                case BLINKER_CMD_SMS_NUMBER :
                    if (!checkSMS()) {
                        return BLINKER_CMD_FALSE;
                    }

                    if ((!state && msg.length() > BLINKER_SMS_MAX_SEND_SIZE) ||
                        (state && msg.length() > BLINKER_SMS_MAX_SEND_SIZE + 15)) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_PUSH_NUMBER :
                    if (!checkPUSH()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_WECHAT_NUMBER :
                    if (!checkWECHAT()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_WEATHER_NUMBER :
                    if (!checkWEATHER()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_AQI_NUMBER :
                    if (!checkAQI()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_BRIDGE_NUMBER :
                    break;
                case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                    if (!checkCUPDATE()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    if (!checkCGET()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    if (!checkDataUpdata()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                default :
                    return BLINKER_CMD_FALSE;
            }

        #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("message: "), msg);
        #endif

        #ifndef BLINKER_LAN_DEBUG
            const int httpsPort = 443;
        #elif defined(BLINKER_LAN_DEBUG)
            const int httpsPort = 5000;
        #endif
    #if defined(ESP8266)
        #ifndef BLINKER_LAN_DEBUG
            const char* host = "iotdev.clz.me";
        #elif defined(BLINKER_LAN_DEBUG)
            const char* host = "192.168.0.104";
        #endif
            const char* fingerprint = "84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a";
        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
            #ifndef BLINKER_LAN_DEBUG
            extern WiFiClientSecure client_s;
            #elif defined(BLINKER_LAN_DEBUG)
            WiFiClient client_s;
            #endif
        #endif

        #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("connecting to "), host);
        #endif

            if (!client_s.connect(host, httpsPort)) {
        #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("connection failed"));
        #endif
                return BLINKER_CMD_FALSE;
            }
            else {
        #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("connection succeed"));
        #endif
                // return true;
            }

        #ifndef BLINKER_LAN_DEBUG
            if (client_s.verify(fingerprint, host)) {
            #ifdef BLINKER_DEBUG_ALL
                // _status = DH_VERIFIED;
                BLINKER_LOG1(BLINKER_F("Fingerprint verified"));
                // return true;
            #endif
            }
            else {
            #ifdef BLINKER_DEBUG_ALL
                // _status = DH_VERIFY_FAILED;
                // _status = DH_VERIFIED;
                BLINKER_LOG1(BLINKER_F("Fingerprint verification failed!"));
                // return false;
            #endif
            }
        #endif

            String url;
            // if (_type == BLINKER_CMD_SMS_NUMBER) {
            //     url = "/api/v1/user/device/sms";
            // }

            // client_s.print(String("POST ") + url + " HTTP/1.1\r\n" +
            //     "Host: " + host + ":" + httpsPort + "\r\n" +
            //     "Content-Type: application/json;charset=utf-8\r\n" +
            //     "Content-Length: " + msg.length() + "\r\n" +
            //     "Connection: Keep Alive\r\n\r\n" +
            //     msg + "\r\n");
            String client_msg;

            switch (_type) {
                case BLINKER_CMD_SMS_NUMBER :
                    url = "/api/v1/user/device/sms";

                    client_msg = STRING_format("POST " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Content-Type: application/json;charset=utf-8\r\n" +
                        "Content-Length: " + STRING_format(msg.length()) + "\r\n" +
                        "Connection: Keep Alive\r\n\r\n" +
                        msg + "\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                case BLINKER_CMD_PUSH_NUMBER :
                    return BLINKER_CMD_FALSE;
                case BLINKER_CMD_WECHAT_NUMBER :
                    return BLINKER_CMD_FALSE;
                case BLINKER_CMD_WEATHER_NUMBER :
                    url = "/api/v1" + msg;

                    client_msg = STRING_format("GET " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Connection: close\r\n\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                case BLINKER_CMD_AQI_NUMBER :
                    url = "/api/v1" + msg;

                    client_msg = STRING_format("GET " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Connection: close\r\n\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                case BLINKER_CMD_BRIDGE_NUMBER :
                    url = "/api/v1/user/device" + msg;

                    client_msg = STRING_format("GET " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Connection: close\r\n\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                    url = "/api/v1/user/device/userconfig";

                    client_msg = STRING_format("POST " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Content-Type: application/json;charset=utf-8\r\n" +
                        "Content-Length: " + STRING_format(msg.length()) + "\r\n" +
                        "Connection: Keep Alive\r\n\r\n" +
                        msg + "\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    url = "/api/v1/user/device" + msg;

                    client_msg = STRING_format("GET " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Connection: close\r\n\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    url = "/api/v1/user/device/cloudStorage";

                    client_msg = STRING_format("POST " + url + " HTTP/1.1\r\n" +
                        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
                        "Content-Type: application/json;charset=utf-8\r\n" +
                        "Content-Length: " + STRING_format(msg.length()) + "\r\n" +
                        "Connection: Keep Alive\r\n\r\n" +
                        msg + "\r\n");

                    client_s.print(client_msg);
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("client_msg: ", client_msg);
        #endif
                    break;
                default :
                    return BLINKER_CMD_FALSE;
            }

            unsigned long timeout = millis();
            while (client_s.available() == 0) {
                if (millis() - timeout > 5000) {
                    BLINKER_LOG1(BLINKER_F(">>> Client Timeout !"));
                    client_s.stop();
                    return BLINKER_CMD_FALSE;
                }
            }

            // Read all the lines of the reply from server and print them to Serial
            String dataGet;
            String lastGet;
            String lengthOfJson;
            while (client_s.available()) {
                // String line = client_s.readStringUntil('\r');
                dataGet = client_s.readStringUntil('\n');

                if (dataGet.startsWith("Content-Length: ")){
                    int addr_start = dataGet.indexOf(' ');
                    int addr_end = dataGet.indexOf('\0', addr_start + 1);
                    lengthOfJson = dataGet.substring(addr_start + 1, addr_end);
                }

                if (dataGet == "\r") {
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("headers received"));
        #endif
                    break;
                }
            }

            for(int i=0;i<lengthOfJson.toInt();i++){
                lastGet += (char)client_s.read();
            }

            dataGet = lastGet;

        #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("dataGet: "), dataGet);
        #endif
            // if (_type == BLINKER_CMD_SMS_NUMBER) {
            //     DynamicJsonBuffer jsonBuffer;
            //     JsonObject& sms_rp = jsonBuffer.parseObject(dataGet);

            //     if (sms_rp.success()) {
            //         uint16_t msg_code = sms_rp[BLINKER_CMD_MESSAGE];
            //         if (msg_code != 1000) {
            //             String _detail = sms_rp[BLINKER_CMD_DETAIL];
            //             BLINKER_ERR_LOG1(_detail);
            //         }
            //     }
            //     _smsTime = millis();
            // }
            DynamicJsonBuffer jsonBuffer;
            JsonObject& data_rp = jsonBuffer.parseObject(dataGet);

            switch (_type) {
                case BLINKER_CMD_SMS_NUMBER :
                    // DynamicJsonBuffer jsonBuffer;
                    // JsonObject& sms_rp = jsonBuffer.parseObject(dataGet);

                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                            dataGet = _dataGet;
                        }
                    }
                    _smsTime = millis();
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
        #endif
                    break;
                case BLINKER_CMD_PUSH_NUMBER :
                    return BLINKER_CMD_FALSE;
                case BLINKER_CMD_WECHAT_NUMBER :
                    return BLINKER_CMD_FALSE;
                case BLINKER_CMD_WEATHER_NUMBER :
                    // DynamicJsonBuffer jsonBuffer;
                    // JsonObject& wth_rp = jsonBuffer.parseObject(dataGet);

                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                            dataGet = _dataGet;
                        }
                    }
                    _weatherTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
            #endif
                    break;
                case BLINKER_CMD_AQI_NUMBER :
                    // DynamicJsonBuffer jsonBuffer;
                    // JsonObject& wth_rp = jsonBuffer.parseObject(dataGet);

                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                            dataGet = _dataGet;
                        }
                    }
                    _aqiTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
            #endif
                    break;
                case BLINKER_CMD_BRIDGE_NUMBER :
                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
                            dataGet = _dataGet;
                        }
                    }
            #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
            #endif
                    break;
                case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL];
                            dataGet = _dataGet;
                        }
                    }
                    _cUpdateTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
            #endif
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_CONFIG];
                            dataGet = _dataGet;
                        }
                    }
                    _cGetTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
            #endif
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    if (data_rp.success()) {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000) {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG1(_detail);
                        }
                        else {
                            String _dataGet = data_rp[BLINKER_CMD_DETAIL];
                            dataGet = _dataGet;
                        }
                    }
                    _dUpdateTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("dataGet: ", dataGet);
            #endif
                    break;
                default :
                    return BLINKER_CMD_FALSE;
            }

            client_s.stop();

            return dataGet;
    #elif defined(ESP32)
        #ifndef BLINKER_LAN_DEBUG
            const char* host = "https://iotdev.clz.me";
        #elif defined(BLINKER_LAN_DEBUG)
            const char* host = "http://192.168.0.104:5000";
        #endif

            // const char* ca = \
            //     "-----BEGIN CERTIFICATE-----\n" \
            //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n" \
            //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n" \
            //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n" \
            //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n" \
            //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n" \
            //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n" \
            //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n" \
            //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n" \
            //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n" \
            //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n" \
            //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n" \
            //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n" \
            //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n" \
            //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n" \
            //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n" \
            //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n" \
            //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n" \
            //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n" \
            //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n" \
            //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n" \
            //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n" \
            //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n" \
            //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n" \
            //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n" \
            //     "9UBd0Q==\n" \
            //     "-----END CERTIFICATE-----\n";
// #endif
            HTTPClient http;

            String url_iot;

            int httpCode;

            // if (_type == BLINKER_CMD_SMS_NUMBER) {
            //     url_iot = String(host) + "/api/v1/user/device/sms";
            // }
            switch (_type) {
                case BLINKER_CMD_SMS_NUMBER :
                    url_iot = String(host) + "/api/v1/user/device/sms";

                    http.begin(url_iot);
                    http.addHeader("Content-Type", "application/json;charset=utf-8");
                    httpCode = http.POST(msg);
                    break;
                case BLINKER_CMD_PUSH_NUMBER :
                    return BLINKER_CMD_FALSE;
                case BLINKER_CMD_WECHAT_NUMBER :
                    return BLINKER_CMD_FALSE;
                case BLINKER_CMD_WEATHER_NUMBER :
                    url_iot = String(host) + "/api/v1" + msg;

                    http.begin(url_iot);
                    httpCode = http.GET();
                    break;
                case BLINKER_CMD_AQI_NUMBER :
                    url_iot = String(host) + "/api/v1" + msg;

                    http.begin(url_iot);
                    httpCode = http.GET();
                    break;
                case BLINKER_CMD_BRIDGE_NUMBER :
                    url_iot = String(host) + "/api/v1/user/device" + msg;

                    http.begin(url_iot);
                    httpCode = http.GET();
                    break;
                case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                    url_iot = String(host) + "/api/v1/user/device/userconfig";

                    http.begin(url_iot);
                    http.addHeader("Content-Type", "application/json;charset=utf-8");
                    httpCode = http.POST(msg);
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    url_iot = String(host) + "/api/v1/user/device" + msg;

                    http.begin(url_iot);
                    httpCode = http.GET();
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    url_iot = String(host) + "/api/v1/user/device/cloudStorage";

                    http.begin(url_iot);
                    http.addHeader("Content-Type", "application/json;charset=utf-8");
                    httpCode = http.POST(msg);
                    break;
                default :
                    return BLINKER_CMD_FALSE;
            }

        #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("HTTPS begin: "), url_iot);
        #endif

            // http.begin(url_iot, ca); TODO
            // http.begin(url_iot);

            // http.addHeader("Content-Type", "application/json");

            // httpCode = http.POST(msg);

        #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("HTTPS POST: "), msg);
        #endif

            if (httpCode > 0) {
        #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("[HTTP] POST... code: "), httpCode);
        #endif
                String payload;
                if (httpCode == HTTP_CODE_OK) {
                    payload = http.getString();
        #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(payload);
        #endif

                    // if (_type == BLINKER_CMD_SMS_NUMBER) {
                    //     DynamicJsonBuffer jsonBuffer;
                    //     JsonObject& sms_rp = jsonBuffer.parseObject(payload);

                    //     uint16_t msg_code = sms_rp[BLINKER_CMD_MESSAGE];
                    //     if (msg_code != 1000) {
                    //         String _detail = sms_rp[BLINKER_CMD_DETAIL];
                    //         BLINKER_ERR_LOG1(_detail);
                    //     }
                    // }
                    DynamicJsonBuffer jsonBuffer;
                    JsonObject& data_rp = jsonBuffer.parseObject(payload);

                    switch (_type) {
                        case BLINKER_CMD_SMS_NUMBER :
                            // DynamicJsonBuffer jsonBuffer;
                            // JsonObject& sms_rp = jsonBuffer.parseObject(payload);

                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                                    payload = _payload;
                                }
                            }
                            _smsTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        case BLINKER_CMD_PUSH_NUMBER :
                            return BLINKER_CMD_FALSE;
                        case BLINKER_CMD_WECHAT_NUMBER :
                            return BLINKER_CMD_FALSE;
                        case BLINKER_CMD_WEATHER_NUMBER :
                            // DynamicJsonBuffer jsonBuffer;
                            // JsonObject& wth_rp = jsonBuffer.parseObject(payload);

                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                                    payload = _payload;
                                }
                            }
                            _weatherTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        case BLINKER_CMD_AQI_NUMBER :
                            // DynamicJsonBuffer jsonBuffer;
                            // JsonObject& wth_rp = jsonBuffer.parseObject(payload);

                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                                    payload = _payload;
                                }
                            }
                            _aqiTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        case BLINKER_CMD_BRIDGE_NUMBER :
                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
                                    payload = _payload;
                                }
                            }
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL];
                                    payload = _payload;
                                }
                            }
                            _cUpdateTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        case BLINKER_CMD_CONFIG_GET_NUMBER :
                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_CONFIG];
                                    payload = _payload;
                                }
                            }
                            _cGetTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        case BLINKER_CMD_DATA_STORAGE_NUMBER :
                            if (data_rp.success()) {
                                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                                if (msg_code != 1000) {
                                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                                    BLINKER_ERR_LOG1(_detail);
                                }
                                else {
                                    String _payload = data_rp[BLINKER_CMD_DETAIL];
                                    payload = _payload;
                                }
                            }
                            _dUpdateTime = millis();
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2("payload: ", payload);
            #endif
                            break;
                        default :
                            return BLINKER_CMD_FALSE;
                    }
                }
                // if (_type == BLINKER_CMD_SMS_NUMBER) {
                //     _smsTime = millis();
                // }
                http.end();

                return payload;
            }
            else {
        #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("[HTTP] POST... failed, error: "), http.errorToString(httpCode).c_str());
                String payload = http.getString();
                BLINKER_LOG1(payload);
        #endif
                if (_type == BLINKER_CMD_SMS_NUMBER) {
                    _smsTime = millis();
                }
                http.end();
                return BLINKER_CMD_FALSE;
            }
    #endif
        }

        bool checkSMS() {
            if ((millis() - _smsTime) >= BLINKER_SMS_MSG_LIMIT || _smsTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkPUSH() {
            if ((millis() - _pushTime) >= BLINKER_PUSH_MSG_LIMIT || _pushTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkWECHAT() {
            if ((millis() - _wechatTime) >= BLINKER_WECHAT_MSG_LIMIT || _wechatTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkWEATHER() {
            if ((millis() - _weatherTime) >= BLINKER_WEATHER_MSG_LIMIT || _weatherTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkAQI() {
            if ((millis() - _aqiTime) >= BLINKER_AQI_MSG_LIMIT || _aqiTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkCUPDATE() {
            if ((millis() - _cUpdateTime) >= BLINKER_CONFIG_UPDATE_LIMIT || _cUpdateTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkCGET() {
            if ((millis() - _cGetTime) >= BLINKER_CONFIG_GET_LIMIT || _cGetTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkDataUpdata() {
            if ((millis() - _dUpdateTime) >= BLINKER_CONFIG_UPDATE_LIMIT * 60 || _cUpdateTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }
#endif

#if defined(BLINKER_PRO)
        void checkRegister(const JsonObject& data) {
            String _type = data[BLINKER_CMD_REGISTER];

            if (_type.length() > 0) {
                if (_type == STRING_format(_deviceType)) {
                    static_cast<Proto*>(this)->_getRegister = true;
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("getRegister!"));
    #endif
                    static_cast<Proto*>(this)->print(BLINKER_CMD_MESSAGE, "success");
                }
                else {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("not getRegister!"));
    #endif
                    static_cast<Proto*>(this)->print(BLINKER_CMD_MESSAGE, "deviceType check fail");
                }
            }
        }
#endif

    protected :
        callbackFunction            _heartbeatFunc = NULL;

        callback_with_joy_arg_t     _joyFunc = NULL;

#if defined(BLINKER_PRO)
        const char* _deviceType;
        BlinkerWlan Bwlan;

        callback_with_json_arg_t    _parseFunc = NULL;

    #if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
        callbackFunction _powerdownFunc = NULL;
        callbackFunction _resetFunc = NULL;
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
        callbackFunction _clickFunc;
        callbackFunction _doubleClickFunc;
        callbackFunction _pressFunc;
        callbackFunction _longPressStartFunc;
        callbackFunction _longPressStopFunc;
        callbackFunction _duringLongPressFunc;

        // These variables that hold information across the upcoming tick calls.
        // They are initialized once on program start and are updated every time the tick function is called.
        int _state;
        unsigned long _startTime; // will be set in state 1
        unsigned long _stopTime; // will be set in state 2

        bool wlanRun() {
    #if defined(BLINKER_BUTTON)
            tick();
    #endif
            return Bwlan.run();
        }

        bool isPressed = false;

        // void checkButton()
        // {
        //     button1.tick();
        // }

        void _click()
        {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("Button click."));
            // _clickFunc();
    #endif
        } // click

        void _doubleClick() {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("Button doubleclick."));
            // _doubleClickFunc();
    #endif
        } // doubleclick1

        void _longPressStart()
        {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("Button longPress start"));
            // _longPressStartFunc();
    #endif
            isPressed = true;
        } // longPressStart

        void _longPress()
        {
            if (isPressed)
            {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Button longPress..."));
    #endif
                isPressed = false;
            }
            // _duringLongPressFunc();
        } // longPress

        void _longPressStop()
        {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("Button longPress stop"));
    #endif
            // _longPressStopFunc();
            // Bwlan.deleteConfig();
            // Bwlan.reset();
            // ESP.restart();
            // reset();

            uint32_t _pressedTime = millis() - _startTime;

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG4("_stopTime: ", millis(), " ,_startTime: ", _startTime);
            BLINKER_LOG2("long pressed time: ", _pressedTime);
    #endif

    #if defined(BLINKER_BUTTON_LONGPRESS_POWERDOWN)
            if (_pressedTime >= BLINKER_PRESSTIME_RESET) {
                if (_resetFunc) {
                    _resetFunc();
                }

                reset();
            }
            else {
                BLINKER_LOG1(BLINKER_F("BLINKER_PRESSTIME_POWERDOWN"));

                if (_powerdownFunc) {
                    _powerdownFunc();
                }
            }
    #else
            // if (_resetFunc) {
            //     _resetFunc();
            // }

            reset();
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
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("Button initialled"));
    #endif
        }
#endif
        void parse(String _data, bool ex_data = false)
        {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("parse data: "), _data);
    #endif
            if (!ex_data) {
                if (static_cast<Proto*>(this)->parseState() ) {
                    _fresh = false;

#if defined(ESP8266) || defined(ESP32)
                    DynamicJsonBuffer jsonBuffer;
                    JsonObject& root = jsonBuffer.parseObject(_data);

                    if (!root.success()) {
                        return;
                    }
// (const JsonObject& data)
    #if defined(BLINKER_PRO)
                    checkRegister(root);
    #endif

    #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
                    // if (autoManager(root)) {
                    //     static_cast<Proto*>(this)->isParsed();
                    //     return;
                    // }
                    autoManager(root);
    #endif
                    // if (timerManager(root)) {
                    //     static_cast<Proto*>(this)->isParsed();
                    //     return;
                    // }
                    timerManager(root);
                    heartBeat(root);
                    setSwitch(root);
                    getVersion(root);

                    for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
                        strWidgetsParse(_Widgets_str[wNum]->getName(), root);
                    }
                    for (uint8_t wNum_int = 0; wNum_int < _wCount_int; wNum_int++) {
                        intWidgetsParse(_Widgets_int[wNum_int]->getName(), root);
                    }
                    for (uint8_t wNum_rgb = 0; wNum_rgb < _wCount_rgb; wNum_rgb++) {
                        rgbWidgetsParse(_Widgets_rgb[wNum_rgb]->getName(), root);
                    }
                    for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
                        joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), root);
                    }

                    // joystick(root);
                    ahrs(Yaw, root);
                    gps(LONG, true, root);
#else
                    heartBeat();
                    setSwitch();
                    getVersion();

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

                    // joystick();
                    ahrs(Yaw);
                    gps(LONG, true);
#endif
                    if (_fresh) {
                        static_cast<Proto*>(this)->isParsed();
                    }
                    else {
        #if defined(BLINKER_PRO)
                        if (_parseFunc) {
                            if(_parseFunc(root)) {
                                _fresh = true;
                                static_cast<Proto*>(this)->isParsed();
                            }
            #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG1(BLINKER_F("run parse callback function"));
            #endif
                        }
        #endif
                    }
                }
            }
            else {
#if defined(ESP8266) || defined(ESP32)
                String data1 = "{\"data\":" + _data + "}";
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(data1);

                if (!root.success()) {
                    return;
                }

                String arrayData = root["data"][0];
    #ifdef BLINKER_DEBUG_ALL
                // BLINKER_LOG4("data1: ", data1, " arrayData: ", arrayData);
                BLINKER_LOG2(BLINKER_F("_parse data: "), _data);
    #endif
                if (arrayData.length()) {
                    for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++) {
                        String array_data = root["data"][a_num];

                        // BLINKER_LOG2("array_data: ", array_data);

                        if(array_data.length()) {
                            DynamicJsonBuffer _jsonBuffer;
                            JsonObject& _array = _jsonBuffer.parseObject(array_data);

                            json_parse(_array);
                            timerManager(_array, true);
                        }
                        else {
                            return;
                        }
                    }
                }
                else {
                    JsonObject& root = jsonBuffer.parseObject(_data);

                    if (!root.success()) {
                        return;
                    }

                    json_parse(root);
                }
#else
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

                // joystick(_data);
#endif
            }
        }

#if defined(ESP8266) || defined(ESP32)
        void json_parse_all(const JsonObject& data) {
    #if defined(BLINKER_PRO)
            checkRegister(data);
    #endif

    #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
            // if (autoManager(root)) {
            //     static_cast<Proto*>(this)->isParsed();
            //     return;
            // }
            autoManager(data);
    #endif
            timerManager(data);
            heartBeat(data);
            setSwitch(data);
            getVersion(data);

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

            // joystick(data);
            ahrs(Yaw, data);
            gps(LONG, true, data);
        }

        void json_parse(const JsonObject& data) {
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

            // joystick(data);
        }

        bool ntpInit() {
            if (!_isNTPInit) {
                freshNTP();

                if ((millis() - _ntpStart) > BLINKER_NTP_TIMEOUT) {
                    _ntpStart = millis();
                }
                else {
                    return false;
                }

                configTime((long)(_timezone * 3600), 0, "ntp1.aliyun.com", "210.72.145.44", "time.pool.aliyun.com");// cn.pool.ntp.org

                now_ntp = ::time(nullptr);//getLocalTime(&timeinfo)

                // BLINKER_LOG4("Setting time using SNTP: ", now_ntp, " ", _timezone * 3600 * 2);

                if (now_ntp < _timezone * 3600 * 2) {
                    configTime((long)(_timezone * 3600), 0, "ntp1.aliyun.com", "210.72.145.44", "time.pool.aliyun.com");// cn.pool.ntp.org
                    now_ntp = ::time(nullptr);

                    if (now_ntp < _timezone * 3600 * 2) {
                        ::delay(50);

                        now_ntp = ::time(nullptr);

                        // BLINKER_LOG4("Setting time using SNTP2: ", now_ntp, " ", _timezone * 3600 * 2);

                        return false;
                    }
                }
                // struct tm timeinfo;
    #if defined(ESP8266)
                gmtime_r(&now_ntp, &timeinfo);
    #elif defined(ESP32)
                localtime_r(&now_ntp, &timeinfo);
                // getLocalTime(&timeinfo, 5000);
    #endif
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("Current time: "), asctime(&timeinfo));
    #endif

                // struct tm tmstruct;

                // getLocalTime(&timeinfo, 5000);
                
                _isNTPInit = true;
                
                // BLINKER_LOG2("mday: ", timeinfo.tm_mday);

                // BLINKER_LOG2("mday: ", mday());

                loadTiming();
            }

            return true;
        }
// #endif
// #if defined(BLINKER_MQTT)
        bool checkTimer() {
            if (_cdTrigged) {
                _cdTrigged = false;

                _cdRunState = false;
                // _cdData |= _cdRunState << 14;
                _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);
                saveCountDown(_cdData, _cdAction);

    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("countdown trigged, action is: "), _cdAction);
    #endif
                // _parse(_cdAction);
                parse(_cdAction, true);
            }
            if (_lpTrigged) {
                _lpTrigged = false;

                if (_lpStop) {
                    _lpRunState = false;
                    // _lpData |= _lpRunState << 30;
                    _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;
                    saveLoop(_lpData, _lpAction1, _lpAction2);
                }

                if (_lpRun1) {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("loop trigged, action is: "), _lpAction2);
    #endif
                    // _parse(_lpAction2);
                    parse(_lpAction2, true);
                }
                else {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("loop trigged, action is: "), _lpAction1);
    #endif
                    // _parse(_lpAction1);
                    parse(_lpAction1, true);
                }
            }
            if (_tmTrigged) {
                _tmTrigged = false;

    //             if (_tmRun1) {
    // #ifdef BLINKER_DEBUG_ALL
    //                 BLINKER_LOG2(BLINKER_F("timing trigged, action is: "), _tmAction2);
    // #endif
    //                 // _parse(_tmAction2);
    //                 parse(_tmAction2, true);
    //             }
    //             else {
    // #ifdef BLINKER_DEBUG_ALL
    //                 BLINKER_LOG2(BLINKER_F("timing trigged, action is: "), _tmAction1);
    // #endif
    //                 // _parse(_tmAction1);
    //                 parse(_tmAction1, true);
    //             }

                uint8_t wDay =  wday();

                if (triggedTask < BLINKER_TIMING_TIMER_SIZE) {
                    String _tmAction = timingTask[triggedTask]->getAction();

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("timing trigged, action is: "), _tmAction);
    #endif
                    parse(_tmAction, true);

                    checkOverlapping(wDay, timingTask[triggedTask]->getTime());

                    freshTiming(wDay, timingTask[triggedTask]->getTime());
                }
                else {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("timing trigged, none action"));
    #endif

                    freshTiming(wDay, 0);
                }
            }
        }
#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        String bridgeFind(const String & _Name)
        {
            int8_t num = checkNum(_Name, _Bridge, _bridgeCount);

            if( num != BLINKER_OBJECT_NOT_AVAIL ) {
                return _Bridge[num]->getBridge();
            }
            else {
                if (bridge(_Name)) {
                    num = checkNum(_Name, _Bridge, _bridgeCount);

                    if( num != BLINKER_OBJECT_NOT_AVAIL ) {
                        return _Bridge[num]->getBridge();
                    }
                }
                return "";
            }
        }

        String bridgeQuery(const String & key) {
            String data = "/query?";
    // #if defined(BLINKER_MQTT)
            data += "deviceName=" + STRING_format(static_cast<Proto*>(this)->_deviceName);
    // #elif defined(BLINKER_PRO)
    //         data += "deviceName=" + macDeviceName();
    // #endif
            data += "&bridgeKey=" + key;

            return blinkServer(BLINKER_CMD_BRIDGE_NUMBER, data);
        }
#endif
};

#endif