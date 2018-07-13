#ifndef BlinkerApi_H
#define BlinkerApi_H

#include <time.h>
#if defined(ESP8266) || defined(ESP32)
    #include <Ticker.h>
    #include <EEPROM.h>
    #include "modules/ArduinoJson/ArduinoJson.h"
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

    extern "C" {
        typedef void (*callbackFunction)(void);

        typedef void (*callback_t)(void);
        typedef void (*callback_with_arg_t)(void*);
        typedef bool (*callback_with_json_arg_t)(const JsonObject & data);
    }
#else
    #include <Blinker/BlinkerConfig.h>
    #include <utility/BlinkerUtility.h>
#endif
// #include "modules/ArduinoJson/ArduinoJson.h"
// #include <Blinker/BlinkerConfig.h>
// #include <utility/BlinkerDebug.h>
// #include <utility/BlinkerUtility.h>

enum b_widgettype_t {
    W_BUTTON,
    W_SLIDER,
    W_TOGGLE,
    W_RGB
};

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
    B
};

static class BlinkerButton * _Button[BLINKER_MAX_WIDGET_SIZE];
static class BlinkerSlider * _Slider[BLINKER_MAX_WIDGET_SIZE];
static class BlinkerToggle * _Toggle[BLINKER_MAX_WIDGET_SIZE];
static class BlinkerToggle *_BUILTIN_SWITCH;
static class BlinkerRGB * _RGB[BLINKER_MAX_WIDGET_SIZE];
#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
static class BlinkerAUTO * _AUTO[2];
static class BlinkerBridge * _Bridge[BLINKER_MAX_BRIDGE_SIZE];
#endif

#if defined(BLINKER_WIFI)
#include <WiFiClientSecure.h>
static WiFiClientSecure client_s;
#endif
// #if defined(BLINKER_PRO)
// static OneButton button1(BLINKER_BUTTON_PIN, true);

// #endif

class BlinkerButton
{
    public :
        BlinkerButton()
            : buttonName(NULL), buttonState(false)
        {}
        
        void name(String name) { buttonName = name; }
        String getName() { return buttonName; }
        void freshState(bool state, bool isLong = false) { buttonState = state; isLPress = isLong; }
        bool getState() { return buttonState; }
        bool longPress() { return isLPress; }
        bool checkName(String name) { return ((buttonName == name) ? true : false); }
    
    private :
        String  buttonName;
        bool    buttonState;
        bool    isLPress;
};

class BlinkerSlider
{
    public :
        BlinkerSlider()
            : sliderName(NULL), sliderValue(0)
        {}
        
        void name(String name) { sliderName = name; }
        String getName() { return sliderName; }
        void freshValue(uint8_t value) { sliderValue = value; }
        uint8_t getValue() { return sliderValue; }
        bool checkName(String name) { return ((sliderName == name) ? true : false); }
    
    private :
        String  sliderName;
        uint8_t sliderValue;
};

class BlinkerToggle
{
    public :
        BlinkerToggle()
            : toggleName(NULL), toggleState(false)
        {}
        
        void name(String name) { toggleName = name; }
        String getName() { return toggleName; }
        void freshState(bool state) { toggleState = state; }
        bool getState() { return toggleState; }
        bool checkName(String name) { return ((toggleName == name) ? true : false); }
    
    private :
        String  toggleName;
        bool    toggleState;
};

class BlinkerRGB
{
    public :
        BlinkerRGB()
            : rgbName(NULL)
        {}
        
        void name(String name) { rgbName = name; }
        String getName() { return rgbName; }
        void freshValue(b_rgb_t color,uint8_t value) { rgbValue[color] = value; }
        uint8_t getValue(b_rgb_t color) { return rgbValue[color]; }
        bool checkName(String name) { return ((rgbName == name) ? true : false); }
    
    private :
        String  rgbName;
        uint8_t rgbValue[3] = {0};
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
#endif
// #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
// // template <class API>
// class BlinkerAUTO
// {
//     public :
//         BlinkerAUTO()
//             : _autoState(false)
//             , _trigged(false)
//         {}

//         void run(String key, float data, int32_t nowTime) {
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG6(BLINKER_F("BlinkerAUTO run key: "), key, BLINKER_F(" data: "), data, BLINKER_F(" nowTime: "), nowTime);
// #endif
//             for (uint8_t _num = 0; _num < _targetNum; _num++) {
//                 if (!_autoState) {
//                     return;
//                 }

//                 if (key != STRING_format(_targetKey[_num])) {
//                     return;
//                 }

//                 if (_time1[_num] < _time2[_num]) {
//                     if (!(nowTime >= _time1[_num] && nowTime <= _time2[_num])) {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
// #endif
//                         return;
//                     }
//                 }
//                 else if (_time1[_num] > _time2[_num]) {
//                     if (nowTime > _time1[_num] && nowTime < _time2[_num]) {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
// #endif
//                         return;
//                     }
//                 }

//                 if ((_logicType == BLINKER_TYPE_NUMERIC)
//                     || (_logicType == BLINKER_TYPE_OR 
//                     && logic_type[_num] == BLINKER_TYPE_NUMERIC)
//                     || (_logicType == BLINKER_TYPE_AND 
//                     && logic_type[_num] == BLINKER_TYPE_NUMERIC))
//                 {
//                     switch (_compareType[_num]) {
//                         case BLINKER_COMPARE_LESS:
//                             if (data < _targetData[_num]) {
//                                 if (!_isTrigged[_num]) {
//                                     triggerCheck("less", _num);
//                                 }
//                             }
//                             else {
//                                 _isTrigged[_num] = false;
//                                 isRecord[_num] = false;
//                                 _trigged = false;
//                             }
//                             break;
//                         case BLINKER_COMPARE_EQUAL:
//                             if (data = _targetData[_num]) {
//                                 if (!_isTrigged[_num]) {
//                                     triggerCheck("equal", _num);
//                                 }
//                             }
//                             else {
//                                 _isTrigged[_num] = false;
//                                 isRecord[_num] = false;
//                                 _trigged = false;
//                             }
//                             break;
//                         case BLINKER_COMPARE_GREATER:
//                             if (data > _targetData[_num]) {
//                                 if (!_isTrigged[_num]) {
//                                     triggerCheck("greater", _num);
//                                 }
//                             }
//                             else {
//                                 _isTrigged[_num] = false;
//                                 isRecord[_num] = false;
//                                 _trigged = false;
//                             }
//                             break;
//                         default:
//                             break;
//                     }
//                 }
//             }
//         }

//         void run(String key, String state, int32_t nowTime) {
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG6(BLINKER_F("BlinkerAUTO run key: "), key, BLINKER_F(" state: "), state, BLINKER_F(" nowTime: "), nowTime);
// #endif
//             for (uint8_t _num = 0; _num < _targetNum; _num++) {
//                 if (!_autoState) {
//                     return;
//                 }

//                 if (key != STRING_format(_targetKey[_num])) {
//                     return;
//                 }

//                 if (_time1[_num] < _time2[_num]) {
//                     if (!(nowTime >= _time1[_num] && nowTime <= _time2[_num])) {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
// #endif
//                         return;
//                     }
//                 }
//                 else if (_time1[_num] > _time2[_num]) {
//                     if (nowTime > _time1[_num] && nowTime < _time2[_num]) {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
// #endif
//                         return;
//                     }
//                 }

//                 // if (_logicType == BLINKER_TYPE_STATE) {
//                 if ((_logicType == BLINKER_TYPE_STATE)
//                     || (_logicType == BLINKER_TYPE_OR 
//                     && logic_type[_num] == BLINKER_TYPE_STATE)
//                     || (_logicType == BLINKER_TYPE_AND 
//                     && logic_type[_num] == BLINKER_TYPE_STATE))
//                 {
//                     if (state == BLINKER_CMD_ON) {
//                         if (_targetState[_num]) {
//                             if (!_isTrigged[_num]) {
//                                 triggerCheck("on", _num);
//                             }
//                         }
//                         else {
//                             _isTrigged[_num] = false;
//                             isRecord[_num] = false;
//                             _trigged = false;
//                         }
//                     }
//                     else if (state == BLINKER_CMD_OFF) {
//                         if (!_targetState[_num]) {
//                             if (!_isTrigged[_num]) {
//                                 triggerCheck("off", _num);
//                             }
//                         }
//                         else {
//                             _isTrigged[_num] = false;
//                             isRecord[_num] = false;
//                             _trigged = false;
//                         }
//                     }
//                 }
//                 else if (_logicType == BLINKER_TYPE_OR 
//                     && logic_type[_num] == BLINKER_TYPE_STATE)
//                 {
//                     if (state == BLINKER_CMD_ON) {
//                         if (_targetState[_num]) {
//                             if (!_isTrigged[_num]) {
//                                 triggerCheck("on", _num);
//                             }
//                         }
//                         else {
//                             _isTrigged[_num] = false;
//                             isRecord[_num] = false;
//                             _trigged = false;
//                         }
//                     }
//                     else if (state == BLINKER_CMD_OFF) {
//                         if (!_targetState[_num]) {
//                             if (!_isTrigged[_num]) {
//                                 triggerCheck("off", _num);
//                             }
//                         }
//                         else {
//                             _isTrigged[_num] = false;
//                             isRecord[_num] = false;
//                             _trigged = false;
//                         }
//                     }
//                 }
//             }
//         }

//         void manager(String data) {
//             DynamicJsonBuffer jsonBuffer;
//             JsonObject& root = jsonBuffer.parseObject(data);

//             // String auto_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "auto\"", ",", 1);
//             _autoState = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO];
//             // if (auto_state == "") {
//             //     auto_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "auto\"", "}", 1);
//             // }
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG1(BLINKER_F("==============================================="));
//             BLINKER_LOG2(BLINKER_F("auto state: "), _autoState);
// #endif
//             // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;
//             // _autoState = auto_state;

//             // _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);
//             _autoId = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_AUTOID];

//             // String logicType;
//             String logicType = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICTYPE];
//             // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), logicType, BLINKER_CMD_LOGICTYPE)) {
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(BLINKER_F("_autoId: "), _autoId);
//             BLINKER_LOG2(BLINKER_F("logicType: "), logicType);
// #endif
//             // String target_key;
//             // String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETKEY];
//             // // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_TARGETKEY);
//             // strcpy(_targetKey[0], target_key.c_str());
            
//             if (logicType == BLINKER_CMD_STATE) {
// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG1(BLINKER_F("state!"));
// #endif
//                 String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETKEY];
//                 // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_TARGETKEY);
//                 strcpy(_targetKey[0], target_key.c_str());
                
//                 _targetNum = 1;

//                 _logicType = BLINKER_TYPE_STATE;

//                 logic_type[0] = BLINKER_TYPE_STATE;
//                 // String target_state;
//                 String target_state = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETSTATE];
//                 // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_state, BLINKER_CMD_TARGETSTATE)) {
//                 if (target_state == BLINKER_CMD_ON) {
//                     _targetState[0] = true;
//                 }
//                 else if (target_state == BLINKER_CMD_OFF) {
//                     _targetState[0] = false;
//                 }

//                 _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_DURATION];
//                 _duration[0] = 60 * _duration[0];
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
//                     BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[0]);
//                     BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[0]);
//                     BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
// #endif
//                 // }
//             }
//             else if (logicType == BLINKER_CMD_NUMBERIC) {
// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG1(BLINKER_F("numberic!"));
// #endif
//                 String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETKEY];
//                 // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_TARGETKEY);
//                 strcpy(_targetKey[0], target_key.c_str());
                
//                 _targetNum = 1;
                
//                 _logicType = BLINKER_TYPE_NUMERIC;

//                 logic_type[0] = BLINKER_TYPE_NUMERIC;
//                 // String _type;
//                 String _type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_COMPARETYPE];
//                 // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), _type, BLINKER_CMD_COMPARETYPE)) {
//                     if (_type == BLINKER_CMD_LESS) {
//                         _compareType[0] = BLINKER_COMPARE_LESS;
//                     }
//                     else if (_type == BLINKER_CMD_EQUAL) {
//                         _compareType[0] = BLINKER_COMPARE_EQUAL;
//                     }
//                     else if (_type == BLINKER_CMD_GREATER) {
//                         _compareType[0] = BLINKER_COMPARE_GREATER;
//                     }

//                     // _targetData = STRING_find_float_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TARGETDATA);
//                     _targetData[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETDATA];
//                     _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_DURATION];
//                     _duration[0] = 60 * _duration[0];

// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
//                     BLINKER_LOG4(BLINKER_F("_type: "), _type, BLINKER_F(" _compareType: "), _compareType[0]);
//                     BLINKER_LOG4(BLINKER_F("_targetKey: "), _targetKey[0], BLINKER_F(" _targetData: "), _targetData[0]);
//                     BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
// #endif
//                 // }
//             }
//             else if (logicType == BLINKER_CMD_OR || logicType == BLINKER_CMD_AND) {
//                 if (logicType == BLINKER_CMD_OR) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1(BLINKER_F("or!"));
// #endif
//                     _logicType = BLINKER_TYPE_OR;
//                 }
//                 else if (logicType == BLINKER_CMD_AND) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1(BLINKER_F("and!"));
// #endif
//                     _logicType = BLINKER_TYPE_AND;
//                 }

//                 _targetNum = 2;

//                 for (uint8_t t_num = 0; t_num < _targetNum; t_num++) {
//                     String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][t_num][BLINKER_CMD_TARGETKEY];

//                     String compare_type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][t_num][BLINKER_CMD_COMPARETYPE];

//                     if (compare_type.length()) {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG1(BLINKER_F("or/and numberic!"));
// #endif
//                         logic_type[t_num] = BLINKER_TYPE_NUMERIC;

//                         strcpy(_targetKey[t_num], target_key.c_str());

//                         // String _type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_COMPARETYPE];
//                     // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), _type, BLINKER_CMD_COMPARETYPE)) {
//                         if (compare_type == BLINKER_CMD_LESS) {
//                             _compareType[t_num] = BLINKER_COMPARE_LESS;
//                         }
//                         else if (compare_type == BLINKER_CMD_EQUAL) {
//                             _compareType[t_num] = BLINKER_COMPARE_EQUAL;
//                         }
//                         else if (compare_type == BLINKER_CMD_GREATER) {
//                             _compareType[t_num] = BLINKER_COMPARE_GREATER;
//                         }

//                         // _targetData = STRING_find_float_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TARGETDATA);
//                         _targetData[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][t_num][BLINKER_CMD_TARGETDATA];
//                         _duration[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][t_num][BLINKER_CMD_DURATION];
//                         _duration[t_num] = 60 * _duration[t_num];
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
//                         BLINKER_LOG2(BLINKER_F("_compareType: "), _compareType[t_num]);
//                         BLINKER_LOG4(BLINKER_F("_targetKey: "), _targetKey[t_num], BLINKER_F(" _targetData: "), _targetData[t_num]);
//                         BLINKER_LOG2(BLINKER_F("_duration: "), _duration[t_num]);
// #endif
//                     }
//                     else {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG1(BLINKER_F("or/and state!"));
// #endif
//                         logic_type[t_num] = BLINKER_TYPE_STATE;

//                         strcpy(_targetKey[t_num], target_key.c_str());
                
//                         // String target_state;
//                         String target_state = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][t_num][BLINKER_CMD_TARGETSTATE];
//                         // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_state, BLINKER_CMD_TARGETSTATE)) {
//                         if (target_state == BLINKER_CMD_ON) {
//                             _targetState[t_num] = true;
//                         }
//                         else if (target_state == BLINKER_CMD_OFF) {
//                             _targetState[t_num] = false;
//                         }

//                         _duration[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][t_num][BLINKER_CMD_DURATION];
//                         _duration[t_num] = 60 * _duration[t_num];
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
//                         BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[t_num]);
//                         BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[t_num]);
//                         BLINKER_LOG2(BLINKER_F("_duration: "), _duration[t_num]);
// #endif
//                     }
//                 }
//             }

//             // int32_t duValue = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DURATION);
//             // _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_DURATION];
//             // _duration[0] = 60 * _duration[0];

//             // if (duValue != FIND_KEY_VALUE_FAILED) {
//             //     _duration = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DURATION);
//             // }
//             // else {
//             //     _duration = 0;
//             // }
// // #ifdef BLINKER_DEBUG_ALL
// //             BLINKER_LOG2("_duration: ", _duration[0]);
// // #endif
//             // int32_t timeValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMESLOT, 0);
//             int32_t timeValue = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_TIMESLOT][0];

//             if (timeValue) {
//                 // _time1 = 60 * STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMESLOT, 0);
//                 _time1[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_TIMESLOT][0];
//                 _time1[0] = 60 * _time1[0];
//                 // _time2 = 60 * STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMESLOT, 1);
//                 _time2[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_TIMESLOT][1];
//                 _time2[0] = 60 * _time2[0];
//             }
//             else {
//                 _time1[0] = 0;
//                 _time2[0] = 24 * 60 * 60;
//             }
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG4(BLINKER_F("_time1: "), _time1[0], BLINKER_F(" _time2: "), _time2[0]);
// #endif
//             // String datas;
//             // datas = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_LINKDATA, "]", 3);

//             // strcpy(_linkDevice[0], STRING_find_string(datas, BLINKER_CMD_LINKDEVICE, "\"", 3).c_str());
//             // strcpy(_linkType[0], STRING_find_string(datas, BLINKER_CMD_LINKTYPE, "\"", 3).c_str());
//             // strcpy(_linkData[0], STRING_find_string(datas, BLINKER_CMD_DATA, "}", 3).c_str());
//             // _aCount++;
//             String linkData = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA];

//             if (linkData.length() > 0) {
//                 String link_device = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][0][BLINKER_CMD_LINKDEVICE];
//                 String link_type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][0][BLINKER_CMD_LINKTYPE];
//                 String link_data = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][0][BLINKER_CMD_DATA];
                
//                 strcpy(_linkDevice[0], link_device.c_str());
//                 strcpy(_linkType[0], link_type.c_str());
//                 strcpy(_linkData[0], link_data.c_str());

//                 const char* link_device2 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKDEVICE];

//                 if (link_device2) {
//                     _linkNum = 2;

//                     String link_device1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKDEVICE];
//                     String link_type1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKTYPE];
//                     String link_data1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_DATA];
                    
//                     strcpy(_linkDevice[1], link_device1.c_str());
//                     strcpy(_linkType[1], link_type1.c_str());
//                     strcpy(_linkData[1], link_data1.c_str());

// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
//                     BLINKER_LOG2(BLINKER_F("_linkDevice1: "), _linkDevice[0]);
//                     BLINKER_LOG2(BLINKER_F("_linkType1: "), _linkType[0]);
//                     BLINKER_LOG2(BLINKER_F("_linkData1: "), _linkData[0]);
//                     BLINKER_LOG2(BLINKER_F("_linkDevice2: "), _linkDevice[1]);
//                     BLINKER_LOG2(BLINKER_F("_linkType2: "), _linkType[1]);
//                     BLINKER_LOG2(BLINKER_F("_linkData2: "), _linkData[1]);
//                     BLINKER_LOG2(BLINKER_F("_targetNum: "), _targetNum);
//                     // BLINKER_LOG2("_aCount: ", _aCount);
//                     BLINKER_LOG1(BLINKER_F("==============================================="));
// #endif
//                 }
//                 else {
//                     _linkNum = 1;

// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
//                     BLINKER_LOG2(BLINKER_F("_linkDevice1: "), _linkDevice[0]);
//                     BLINKER_LOG2(BLINKER_F("_linkType1: "), _linkType[0]);
//                     BLINKER_LOG2(BLINKER_F("_linkData1: "), _linkData[0]);
//                     BLINKER_LOG2(BLINKER_F("_targetNum: "), _targetNum);
//                     // BLINKER_LOG2("_aCount: ", _aCount);
//                     BLINKER_LOG1(BLINKER_F("==============================================="));
// #endif
//                 }
//             }
//             else {
//                 _linkNum = 0;
//             }

//             serialization();
//         }

//         void deserialization() {
//             uint8_t checkData;
//             EEPROM.begin(BLINKER_EEP_SIZE);
//             EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

//             if (checkData != BLINKER_CHECK_DATA) {
//                 _autoState = false;
//                 EEPROM.commit();
//                 EEPROM.end();
//                 return;
//             }

// //             EEPROM.get(BLINKER_EEP_ADDR_AUTONUM, _aCount);
// // #ifdef BLINKER_DEBUG_ALL
// //             BLINKER_LOG2("_aCount: ", _aCount);
// // #endif
//             // EEPROM.get(BLINKER_EEP_ADDR_AUTOID, _autoId);
//             // EEPROM.get(BLINKER_EEP_ADDR_AUTO, _autoData);
//             EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTOID, _autoId);
//             EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TYPESTATE, _typeState);

//             _logicType = _typeState >> 6;// | _autoState << 4 | _linkNum;
//             _autoState = _typeState >> 4 & 0x03;
//             _linkNum = _typeState & 0x0F;

// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(BLINKER_F("_typeState: "), _typeState);
// #endif

//             if (_logicType == BLINKER_TYPE_STATE || _logicType == BLINKER_TYPE_NUMERIC) {
//                 _targetNum = 1;
//             }
//             else {
//                 _targetNum = 2;
//             }
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(BLINKER_F("_targetNum: "), _targetNum);
// #endif
//             // if (_targetNum == 1) {
//             // for (uint8_t t_num = 0; t_num < _targetNum; t_num++) {
//             EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTO1, _autoData[0]);

//             // _linkNum = _autoId >> 30;
//             // _autoId = _autoId;// & 0xFFFFFFFF;
//             // _autoState = _autoData >> 31;
//             logic_type[0] = _autoData[0] >> 30 & 0x03;
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG1(BLINKER_F("==============================================="));
//             BLINKER_LOG2(BLINKER_F("_autoId: "), _autoId);
//             // BLINKER_LOG2("_autoState: ", _autoState ? "true" : "false");
//             // BLINKER_LOG2("_logicType: ", _logicType ? "numberic" : "state");
//             switch (_logicType) {
//                 case BLINKER_TYPE_STATE :
//                     BLINKER_LOG1(BLINKER_F("_logicType: state"));
//                     break;
//                 case BLINKER_TYPE_NUMERIC :
//                     BLINKER_LOG1(BLINKER_F("_logicType: numberic"));
//                     break;
//                 case BLINKER_TYPE_OR :
//                     BLINKER_LOG1(BLINKER_F("_logicType: or"));
//                     break;
//                 case BLINKER_TYPE_AND :
//                     BLINKER_LOG1(BLINKER_F("_logicType: and"));
//                     break;
//                 default :
//                     break;
//             }
// #endif

//             EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
                
//             if (logic_type[0] == BLINKER_TYPE_STATE) {
//                 _targetState[0] = _autoData[0] >> 28 & 0x03;
// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[0]);
//                 BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[0] ? "on" : "off");
// #endif
//             }
//             else {
//                 _compareType[0] = _autoData[0] >> 28 & 0x03;
//                 // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
//                 EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG2(BLINKER_F("_compareType: "), _compareType[0] ? (_compareType[0] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
//                 BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[0]);
//                 BLINKER_LOG2(BLINKER_F("_targetData: "), _targetData[0]);
// #endif
//             }

//             _duration[0] = (_autoData[0] >> 22 & 0x3f) * 60;
//             _time1[0] = (_autoData[0] >> 11 & 0x7ff) * 60;
//             _time2[0] = (_autoData[0] & 0x7ff) * 60;
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
//             BLINKER_LOG4(BLINKER_F("_time1: "), _time1[0], BLINKER_F(" _time2: "), _time2[0]);
// #endif

//             if (_targetNum == 2) {
//                 EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTO2, _autoData[1]);

//                 // _linkNum = _autoId >> 30;
//                 // _autoId = _autoId;// & 0xFFFFFFFF;
//                 // _autoState = _autoData >> 31;
//                 logic_type[1] = _autoData[1] >> 30 & 0x03;
// // #ifdef BLINKER_DEBUG_ALL
// //                     // BLINKER_LOG1("===============================================");
// //                     BLINKER_LOG2("_autoId: ", _autoId);
// //                     // BLINKER_LOG2("_autoState: ", _autoState ? "true" : "false");
// //                     BLINKER_LOG2("_logicType: ", _logicType ? "numberic" : "state");
// // #endif

//                 EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
                    
//                 if (logic_type[1] == BLINKER_TYPE_STATE) {
//                     _targetState[1] = _autoData[1] >> 28 & 0x03;
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[1]);
//                     BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[1] ? "on" : "off");
// #endif
//                 }
//                 else {
//                     _compareType[1] = _autoData[1] >> 28 & 0x03;
//                     // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
//                     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_compareType: "), _compareType[1] ? (_compareType[1] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
//                     BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[1]);
//                     BLINKER_LOG2(BLINKER_F("_targetData: "), _targetData[1]);
// #endif
//                 }

//                 _duration[1] = (_autoData[1] >> 22 & 0x3f) * 60;
//                 _time1[1] = (_autoData[1] >> 11 & 0x7ff) * 60;
//                 _time2[1] = (_autoData[1] & 0x7ff) * 60;

// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG2(BLINKER_F("_duration: "), _duration[1]);
//                 BLINKER_LOG4(BLINKER_F("_time1: "), _time1[1], BLINKER_F(" _time2: "), _time2[1]);
// #endif
//             }

//             // EEPROM.get(BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
//             // EEPROM.get(BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
//             // EEPROM.get(BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
//             if (_linkNum > 0) {
//                 EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
//                 EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
//                 EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);

// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
//                 BLINKER_LOG2(BLINKER_F("_linkDevice: "), _linkDevice[0]);
//                 BLINKER_LOG2(BLINKER_F("_linkType: "), _linkType[0]);
//                 BLINKER_LOG2(BLINKER_F("_linkData: "), _linkData[0]);
//                 BLINKER_LOG2(BLINKER_F("_typeState: "), _typeState);

//                 // BLINKER_LOG1("===============================================");
// #endif          

//                 if (_linkNum == 2) {
//                     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDEVICE2, _linkDevice[1]);
//                     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKTYPE2, _linkType[1]);
//                     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDATA2, _linkData[1]);
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
//                     BLINKER_LOG2(BLINKER_F("_linkDevice: "), _linkDevice[1]);
//                     BLINKER_LOG2(BLINKER_F("_linkType: "), _linkType[1]);
//                     BLINKER_LOG2(BLINKER_F("_linkData: "), _linkData[1]);
//                     BLINKER_LOG2(BLINKER_F("_typeState: "), _typeState);

//                     // BLINKER_LOG1("===============================================");
// #endif              
//                 }
//             }
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG1(BLINKER_F("==============================================="));
// #endif
//             EEPROM.commit();
//             EEPROM.end();
//             // }
//         }

//         void serialization() {
//             uint8_t checkData;

//             // _autoData = _autoState << 31 | _logicType << 30 ;
//             _typeState = _logicType << 6 | _autoState << 4 | _linkNum;

//             EEPROM.begin(BLINKER_EEP_SIZE);

//             EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

//             if (checkData != BLINKER_CHECK_DATA) {
//                 EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
//             }

//             EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTOID, _autoId);
//             EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TYPESTATE, _typeState);

//             // if (_targetNum == 1) {
//             // for (uint8_t t_num = 0; t_num < _targetNum; t_num++) {
//             _autoData[0] = logic_type[0] << 30 ;
//             if (logic_type[0] == BLINKER_TYPE_STATE) {
//                 _autoData[0] |= _targetState[0] << 28;
//             }
//             else {
//                 _autoData[0] |= _compareType[0] << 28;
//             }
//             _autoData[0] |= _duration[0] / 60 << 22 | _time1[0] / 60 << 11 | _time2[0] / 60;
        
//             // EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);

//             // _autoId = _linkNum << 30 | _autoId;
//             // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE, _autoId);
//             // EEPROM.put(BLINKER_EEP_ADDR_AUTO, _autoData);
//             EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
//             EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
                
//             if (logic_type[0] == BLINKER_TYPE_NUMERIC) {
//                 // EEPROM.put(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
//                 EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
//             }
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(BLINKER_F("serialization _typeState: "), _typeState);
//             BLINKER_LOG2(BLINKER_F("serialization _autoData: "), _autoData[0]);
// #endif                 
//             if (_targetNum == 2) {
//                 _autoData[1] = logic_type[1] << 30 ;
//                 if (logic_type[1] == BLINKER_TYPE_STATE) {
//                     _autoData[1] |= _targetState[1] << 28;
//                 }
//                 else {
//                     _autoData[1] |= _compareType[1] << 28;
//                 }
//                 _autoData[1] |= _duration[1] / 60 << 22 | _time1[1] / 60 << 11 | _time2[1] / 60;
            
//                 // EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);

//                 // _autoId = _linkNum << 30 | _autoId;
//                 // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE, _autoId);
//                 // EEPROM.put(BLINKER_EEP_ADDR_AUTO, _autoData);
//                 EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTO2, _autoData[1]);
//                 EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
                    
//                 if (logic_type[1] == BLINKER_TYPE_NUMERIC) {
//                     // EEPROM.put(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
//                     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
//                 }
// #ifdef BLINKER_DEBUG_ALL
//             // BLINKER_LOG2("serialization _typeState: ", _typeState);
//                 BLINKER_LOG2(BLINKER_F("serialization _autoData: "), _autoData[1]);
// #endif  
//             }

//             // EEPROM.put(BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
//             // EEPROM.put(BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
//             // EEPROM.put(BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
//             if (_linkNum > 0) {
//                 EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
//                 EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
//                 EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
            
//                 if (_linkNum == 2) {
//                     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDEVICE2, _linkDevice[1]);
//                     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKTYPE2, _linkType[1]);
//                     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + a_num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDATA2, _linkData[1]);
//                 }
//             }
            
//             EEPROM.commit();
//             EEPROM.end();
//         }

//         void setNum(uint8_t num) {
//             a_num = num;
//         }

//         bool isTrigged() { return _trigged; }

//         void fresh() {
//             for (uint8_t _num = 0; _num < _targetNum; _num++) {
//                 if (isRecord[_num]) {
//                     _isTrigged[_num] = true;
//                 }
//             }
//             _trigged = false;
//         }

//         uint32_t id() { return _autoId; }

//         // uint8_t type() { return _logicType; }

//         uint8_t linkNum() { return _linkNum; }

//         char * name(uint8_t num) { return _linkDevice[num]; }

//         char * type(uint8_t num) { return _linkType[num]; }

//         char * data(uint8_t num) { return _linkData[num]; }

//     private :
//         uint8_t     a_num;
//         // - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -
//         // | | | | |            | _time1 0-1440min 11  | _time2 0-1440min 11                   
//         // | | | | | _duration 0-60min 6
//         // | | | | _targetState|_compareType on/off|less/equal/greater 2
//         // | | | _targetState|_compareType on/off|less/equal/greater
//         // |  
//         // | logic_type state/numberic 2
//         // autoData

//         // - - - - - - - -
//         // |   |   |
//         // |   |   | _linkNum
//         // |   | _autoState true/false 1  
//         // | _logicType state/numberic/and/or 2
//         // typestate
//         bool        _autoState;
//         uint32_t    _autoId = 0;
//         uint8_t     _logicType;
//         uint8_t     _typeState;
//         uint32_t    _autoData[2];

//         uint8_t     _targetNum = 0;

//         uint8_t     logic_type[2];
//         char        _targetKey[2][12];
//         float       _targetData[2];
//         uint8_t     _compareType[2];
//         bool        _targetState[2];
//         uint32_t    _time1[2];
//         uint32_t    _time2[2];
//         uint32_t    _duration[2];
//         uint32_t    _treTime[2];
//         bool        isRecord[2];
//         bool        _isTrigged[2];
//         bool        _trigged;
//         uint8_t     _linkNum;
//         char        _linkDevice[2][BLINKER_LINKDEVICE_SIZE];
//         char        _linkType[2][BLINKER_LINKTYPE_SIZE];
//         char        _linkData[2][BLINKER_LINKDATA_SIZE];

//         void triggerCheck(String state, uint8_t num) {
//             if (!isRecord[num]) {
//                 isRecord[num] = true;
//                 _treTime[num] = millis();
//             }
// #ifdef BLINKER_DEBUG_ALL
//             switch (_logicType) {
//                 case BLINKER_TYPE_STATE :
//                     // BLINKER_LOG1("_logicType: state");
//                     state = "state " + state;
//                     break;
//                 case BLINKER_TYPE_NUMERIC :
//                     // BLINKER_LOG1("_logicType: numberic");
//                     state = "numberic " + state;
//                     break;
//                 case BLINKER_TYPE_OR :
//                     // BLINKER_LOG1("_logicType: or");
//                     state = "or " + state;
//                     break;
//                 case BLINKER_TYPE_AND :
//                     // BLINKER_LOG1("_logicType: and");
//                     state = "and " + state;
//                     break;
//                 default :
//                     break;
//             }
// #endif

//             if ((millis() - _treTime[num]) / 1000 >= _duration[num]) {
//                 if (_logicType != BLINKER_TYPE_AND) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2(state, BLINKER_F(" trigged"));
// #endif
//                     _trigged = true;
//                 }
//                 else {
//                     _isTrigged[num] = true;
//                     if (_isTrigged[0] && _isTrigged[1]) {
// #ifdef BLINKER_DEBUG_ALL
//                         BLINKER_LOG2(state, BLINKER_F(" trigged"));
// #endif
//                         _trigged = true;
//                     }
//                 }
//             }
//         }
// };
// #endif

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
static uint32_t _lpTime1;
static uint32_t _lpTime2;
static uint32_t _tmTime1;
static uint32_t _tmTime2;
static uint8_t  _timingDay = 0;

// static String _cbData1;
// static String _cbData2;

static void disableTimer() {
    _cdState = false;
    cdTicker.detach();
    _lpState = false;
    lpTicker.detach();
    _tmState = false;
    tmTicker.detach();
}

static void _cd_callback() {
    _cdState = false;
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
            }
            else {
                lpTicker.once(_lpTime1, _lp_callback);
            }
        }
        else {
            lpTicker.once(_lpTime1, _lp_callback);
        }
    }
    else {
        lpTicker.once(_lpTime2, _lp_callback);
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

static bool isTimingDay(uint8_t _day) {
    #ifdef BLINKER_DEBUG_ALL     
    BLINKER_LOG2(("isTimingDay: "), _day);
    #endif   
    if (_timingDay & (0x01 << _day))
        return true;
    else
        return false;
}

static void _tm_callback() {
    _tmRun1 = !_tmRun1;

    time_t      now_ntp;
    struct tm   timeinfo;
    now_ntp = time(nullptr);
    // gmtime_r(&now_ntp, &timeinfo);
    #if defined(ESP8266)
    gmtime_r(&now_ntp, &timeinfo);
    #elif defined(ESP32)
    localtime_r(&now_ntp, &timeinfo);
    #endif

    int32_t nowTime = timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    #ifdef BLINKER_DEBUG_ALL 
    BLINKER_LOG6(("nowTime: "), nowTime, (" _tmTime1: "), _tmTime1, (" _tmTime2: "), _tmTime2);
    #endif

    if (isTimingDay(timeinfo.tm_wday)) {
        if (_tmRun1) {
            if (!_isTimingLoop) {
                tmTicker.detach();
    #ifdef BLINKER_DEBUG_ALL 
                BLINKER_LOG1(("timing2 trigged! now need stop!"));
    #endif
                _tmState = false;
            }
            else {
                if (_tmTime1 >= nowTime) {
                    tmTicker.once(_tmTime1 - nowTime, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
                    BLINKER_LOG2(("timing2 trigged! next time: "), _tmTime1 - nowTime);
    #endif
                }
                else if (_tmTime2 <= nowTime && _tmTime1 < nowTime) {
                    tmTicker.once(BLINKER_ONE_DAY_TIME - nowTime, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
                    BLINKER_LOG2(("timing2 trigged! next time: "), BLINKER_ONE_DAY_TIME - nowTime);
    #endif
                }
            }
        }
        else {
            tmTicker.once(_tmTime2 - _tmTime1, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
            BLINKER_LOG2(("timing1 trigged! next time: "), _tmTime2 - _tmTime1);
    #endif
        }
        _tmTrigged = true;
    }
    else {
        tmTicker.once(BLINKER_ONE_DAY_TIME - nowTime, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
        BLINKER_LOG2(("timing trigged! next time: "), BLINKER_ONE_DAY_TIME - nowTime);
    #endif
    }
}

// static void _timing(float seconds) {
//     tmTicker.attach(seconds, _tm_callback);
// }
#endif



template <class Proto>
class BlinkerApi
{
    public :
        BlinkerApi() {
            joyValue[J_Xaxis] = BLINKER_JOYSTICK_VALUE_DEFAULT;
            joyValue[J_Yaxis] = BLINKER_JOYSTICK_VALUE_DEFAULT;
            ahrsValue[Yaw] = 0;
            ahrsValue[Roll] = 0;
            ahrsValue[Pitch] = 0;
            gpsValue[LONG] = "0.000000";
            gpsValue[LAT] = "0.000000";

            _BUILTIN_SWITCH = new BlinkerToggle();
            _BUILTIN_SWITCH->name(BLINKER_CMD_SWITCH);
            _BUILTIN_SWITCH->freshState(true);
            // rgbValue[R] = 0;
            // rgbValue[G] = 0;
            // rgbValue[B] = 0;
        }

        void switchOn() {
            _BUILTIN_SWITCH->freshState(true);
            String data = "{\"" + STRING_format(BLINKER_CMD_SET) + "\":{\"" + \
                            BLINKER_CMD_SWITCH + "\":\"on\"}}";
            parse(data, true);
        }

        void switchOff() {
            _BUILTIN_SWITCH->freshState(false);
            String data = "{\"" + STRING_format(BLINKER_CMD_SET) + "\":{\"" + \
                            BLINKER_CMD_SWITCH + "\":\"off\"}}";
            parse(data, true);
        }

        bool switchAvailable() {
            // if ((builtInSwitch() && !_switchFresh)
            //     || (!builtInSwitch() && _switchFresh)) {
            if (_switchFresh) {
                _switchFresh = false;
                return true;
            }
            else {
                return false;
            }
        }

        bool switchGet() {
            // if (builtInSwitch() && !_switchFresh) {
            //     _switchFresh = true;
            //     return true;
            // }
            // else if (!builtInSwitch() && _switchFresh) {
            //     _switchFresh = false;
            //     return false;
            // }
            // else {
            //     return false;
            // }
            return _BUILTIN_SWITCH->getState();
        }

        bool builtInSwitch() {
            return _BUILTIN_SWITCH->getState();
        }

        void switchUpdate() {
            static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
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

        void wInit(const String & _name, b_widgettype_t _type) {
            switch (_type) {
                case W_BUTTON :
                    if (checkNum(_name, _Button, _bCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _Button[_bCount] = new BlinkerButton();
                            _Button[_bCount]->name(_name);
                            _bCount++;
                        }
                    }
                    break;
                case W_SLIDER :
                    if (checkNum(_name, _Slider, _sCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _Slider[_sCount] = new BlinkerSlider();
                            _Slider[_sCount]->name(_name);
                            _sCount++;
                        }
                    }
                    break;
                case W_TOGGLE :
                    if (checkNum(_name, _Toggle, _tCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _Toggle[_tCount] = new BlinkerToggle();
                            _Toggle[_tCount]->name(_name);
                            _tCount++;
                        }
                    }
                    break;
                case W_RGB :
                    if (checkNum(_name, _RGB, _rgbCount) == BLINKER_OBJECT_NOT_AVAIL) {
                        if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                            _RGB[_rgbCount] = new BlinkerRGB();
                            _RGB[_rgbCount]->name(_name);
                            _rgbCount++;
                        }
                    }
                    break;
                default :
                    break;
            }
        }

        bool button(const String & _bName)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _bName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_BUTTON_TAP) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_PRESSED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true, true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true, true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_RELEASED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(false);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _bCount++;
                    }
                    return false;
                }

                bool _state = _Button[num]->getState();
                if ( !_Button[num]->longPress() )
                    _Button[num]->freshState(false);

                return _state;
            }
        }

        bool toggle(const String & _tName)
        {
            int8_t num = checkNum(_tName, _Toggle, _tCount);
            String state = "";

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _tName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_ON) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(true);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_OFF) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(false);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _tCount++;
                    }
                    return false;
                }

                return _Toggle[num]->getState();
            }
        }

        uint8_t slider(const String & _sName)
        {
            int8_t num = checkNum(_sName, _Slider, _sCount);
            int16_t value = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), _sName);

            if (value != FIND_KEY_VALUE_FAILED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _Slider[_sCount]->freshValue(value);
                        _sCount++;
                    }
                }
                else {
                    _Slider[num]->freshValue(value);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _sCount++;
                    }
                    return 0;
                }
                
                return _Slider[num]->getValue();
            }
        }

        uint8_t joystick(b_joystickaxis_t axis)
        {
            int16_t jAxisValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_JOYSTICK, axis);

            if (jAxisValue != FIND_KEY_VALUE_FAILED) {
                joyValue[J_Xaxis] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_JOYSTICK, J_Xaxis);
                joyValue[J_Yaxis] = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_JOYSTICK, J_Yaxis);

                _fresh = true;
                return jAxisValue;
            }
            else {
                return joyValue[axis];
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

        uint8_t rgb(const String & _rgbName, b_rgb_t color) {
            int8_t num = checkNum(_rgbName, _RGB, _rgbCount);
            int16_t value = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _rgbName, color);

            if (value != FIND_KEY_VALUE_FAILED) {
                uint8_t _rValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _rgbName, R);
                uint8_t _gValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _rgbName, G);
                uint8_t _bValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), _rgbName, B);

                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _RGB[_rgbCount] = new BlinkerRGB();
                        _RGB[_rgbCount]->name(_rgbName);
                        _RGB[_rgbCount]->freshValue(R, _rValue);
                        _RGB[_rgbCount]->freshValue(G, _gValue);
                        _RGB[_rgbCount]->freshValue(B, _bValue);
                        _rgbCount++;
                    }
                }
                else {
                    _RGB[num]->freshValue(R, _rValue);
                    _RGB[num]->freshValue(G, _gValue);
                    _RGB[num]->freshValue(B, _bValue);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _RGB[_rgbCount] = new BlinkerRGB();
                        _RGB[_rgbCount]->name(_rgbName);
                        _rgbCount++;
                    }
                    return 0;
                }
                
                return _RGB[num]->getValue(color);
            }
        }

        // void freshGPS()
        // {
        //     static_cast<Proto*>(this)->print(BLINKER_CMD_GPS, "");
        //     delay(100);
        // }

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
            return (blinkServer(BLINKER_CMD_PUSH_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
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
            return (blinkServer(BLINKER_CMD_PUSH_NUMBER, data) == BLINKER_CMD_FALSE) ? false:true;
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
#endif

#if defined(BLINKER_PRO)
        void attachParse(callback_with_json_arg_t newFunction) {
            _parseFunc = newFunction;
        }

        void attachHeartbeat(callbackFunction newFunction) {
            _heartbeatFunc = newFunction;
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
        bool        _switchFresh = false;
        uint8_t     _bCount = 0;
        uint8_t     _sCount = 0;
        uint8_t     _tCount = 0;
        uint8_t     _rgbCount = 0;
        uint8_t     joyValue[2];
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
        uint8_t     _aCount = 0;
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        uint32_t    _smsTime = 0;

        uint32_t    _pushTime = 0;
        uint32_t    _wechatTime = 0;
        uint32_t    _weatherTime = 0;
        uint32_t    _aqiTime = 0;

        uint32_t    _cUpdateTime = 0;
        uint32_t    _cGetTime = 0;
#endif

#if defined(ESP8266) || defined(ESP32)
        String      _cdAction;
        String      _lpAction1;
        String      _lpAction2;
        String      _tmAction1;
        String      _tmAction2;

        void freshNTP() {
            if (_isNTPInit) {
                now_ntp = ::time(nullptr);
                // gmtime_r(&now_ntp, &timeinfo);
    #if defined(ESP8266)
                gmtime_r(&now_ntp, &timeinfo);
    #elif defined(ESP32)
                localtime_r(&now_ntp, &timeinfo);
    #endif
            }
        }
#else
        void freshNTP() {}
#endif

        bool buttonParse(const String & _bName)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, _bName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_BUTTON_TAP) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else  if (state == BLINKER_CMD_BUTTON_PRESSED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true, true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true, true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_RELEASED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(false);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _bCount++;
                    }
                    return false;
                }

                return _Button[num]->getState();
            }
        }

#if defined(ESP8266) || defined(ESP32)
        bool buttonParse(const String & _bName, const JsonObject& data)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state = data[_bName];

            if (data.containsKey(_bName)) {
                _fresh = true;
            }
            else {
                state = "";
            }

            if (state == BLINKER_CMD_BUTTON_TAP) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else  if (state == BLINKER_CMD_BUTTON_PRESSED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true, true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true, true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_RELEASED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(false);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _bCount++;
                    }
                    return false;
                }

                return _Button[num]->getState();
            }
        }

        bool toggle(const String & _tName, const JsonObject& data)
        {
            int8_t num = checkNum(_tName, _Toggle, _tCount);
            String state = data[_tName];

            if (data.containsKey(_tName)) {
                // BLINKER_LOG2("STATE: ", state);
                _fresh = true;
            }
            // else {
            //     state = "";
            // }

            if (state == BLINKER_CMD_ON) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(true);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_OFF) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(false);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _tCount++;
                    }
                    return false;
                }

                return _Toggle[num]->getState();
            }
        }

        uint8_t slider(const String & _sName, const JsonObject& data)
        {
            int8_t num = checkNum(_sName, _Slider, _sCount);
            
            if (data.containsKey(_sName)) {
                int16_t value = data[_sName];
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _Slider[_sCount]->freshValue(value);
                        _sCount++;
                    }
                }
                else {
                    _Slider[num]->freshValue(value);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _sCount++;
                    }
                    return 0;
                }
                
                return _Slider[num]->getValue();
            }
        }

        uint8_t joystick(b_joystickaxis_t axis, const JsonObject& data)
        {
            if (data.containsKey(BLINKER_CMD_JOYSTICK)) {
                int16_t jAxisValue = data[BLINKER_CMD_JOYSTICK][axis];
                joyValue[J_Xaxis] = data[BLINKER_CMD_JOYSTICK][J_Xaxis];
                joyValue[J_Yaxis] = data[BLINKER_CMD_JOYSTICK][J_Yaxis];

                _fresh = true;
                return jAxisValue;
            }
            else {
                return joyValue[axis];
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

        uint8_t rgb(const String & _rgbName, b_rgb_t color, const JsonObject& data) {
            int8_t num = checkNum(_rgbName, _RGB, _rgbCount);
            
            if (data.containsKey(_rgbName)) {
                int16_t value = data[_rgbName][color];
                uint8_t _rValue = data[_rgbName][R];
                uint8_t _gValue = data[_rgbName][G];
                uint8_t _bValue = data[_rgbName][B];

                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _RGB[_rgbCount] = new BlinkerRGB();
                        _RGB[_rgbCount]->name(_rgbName);
                        _RGB[_rgbCount]->freshValue(R, _rValue);
                        _RGB[_rgbCount]->freshValue(G, _gValue);
                        _RGB[_rgbCount]->freshValue(B, _bValue);
                        _rgbCount++;
                    }
                }
                else {
                    _RGB[num]->freshValue(R, _rValue);
                    _RGB[num]->freshValue(G, _gValue);
                    _RGB[num]->freshValue(B, _bValue);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _RGB[_rgbCount] = new BlinkerRGB();
                        _RGB[_rgbCount]->name(_rgbName);
                        _rgbCount++;
                    }
                    return 0;
                }
                
                return _RGB[num]->getValue(color);
            }
        }

        void heartBeat(const JsonObject& data) {
            String state = data[BLINKER_CMD_GET];

            // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
            if (state.length()) {
                // _fresh = true;
                if (state == BLINKER_CMD_STATE) {
    #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
                    static_cast<Proto*>(this)->beginFormat();
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                    static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
                    stateData();
        #if defined(BLINKER_PRO)
                    if (_heartbeatFunc) {
                        _heartbeatFunc();
                    }
        #endif
                    if (!static_cast<Proto*>(this)->endFormat()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                        static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
                        static_cast<Proto*>(this)->endFormat();
                    }
    #else
                    static_cast<Proto*>(this)->beginFormat();
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                    static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
                    stateData();
                    if (!static_cast<Proto*>(this)->endFormat()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                        static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
                        static_cast<Proto*>(this)->endFormat();
                    }
    #endif
                    _fresh = true;
                }
            }
        }

        void setSwitch(const JsonObject& data) {
            String state = data[BLINKER_CMD_SET][BLINKER_CMD_SWITCH];

            if (state.length()) {
                if (state == BLINKER_CMD_ON) {
                    _BUILTIN_SWITCH->freshState(true);
                }
                else {
                    _BUILTIN_SWITCH->freshState(false);
                }
                _fresh = true;
                _switchFresh = true;
                // static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
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
        bool buttonParse(const String & _bName, String _data)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state;

            if (STRING_find_string_value(_data, state, _bName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_BUTTON_TAP) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else  if (state == BLINKER_CMD_BUTTON_PRESSED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(true, true);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(true, true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_BUTTON_RELEASED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _Button[_bCount]->freshState(false);
                        _bCount++;
                    }
                }
                else {
                    _Button[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _bCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Button[_bCount] = new BlinkerButton();
                        _Button[_bCount]->name(_bName);
                        _bCount++;
                    }
                    return false;
                }

                return _Button[num]->getState();
            }
        }

        uint8_t slider(const String & _sName, String _data)
        {
            int8_t num = checkNum(_sName, _Slider, _sCount);
            int16_t value = STRING_find_numberic_value(_data, _sName);

            if (value != FIND_KEY_VALUE_FAILED) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _Slider[_sCount]->freshValue(value);
                        _sCount++;
                    }
                }
                else {
                    _Slider[num]->freshValue(value);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _sCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Slider[_sCount] = new BlinkerSlider();
                        _Slider[_sCount]->name(_sName);
                        _sCount++;
                    }
                    return 0;
                }
                
                return _Slider[num]->getValue();
            }
        }

        bool toggle(const String & _tName, String _data)
        {
            int8_t num = checkNum(_tName, _Toggle, _tCount);
            String state;

            if (STRING_find_string_value(_data, state, _tName)) {
                _fresh = true;
            }

            if (state == BLINKER_CMD_ON) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(true);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(true);
                }

                _fresh = true;
                return true;
            }
            else if (state == BLINKER_CMD_OFF) {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _Toggle[_tCount]->freshState(false);
                        _tCount++;
                    }
                }
                else {
                    _Toggle[num]->freshState(false);
                }

                _fresh = true;
                return false;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _tCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _Toggle[_tCount] = new BlinkerToggle();
                        _Toggle[_tCount]->name(_tName);
                        _tCount++;
                    }
                    return false;
                }

                return _Toggle[num]->getState();
            }
        }

        uint8_t rgb(const String & _rgbName, b_rgb_t color, String _data) {
            int8_t num = checkNum(_rgbName, _RGB, _rgbCount);
            int16_t value = STRING_find_array_numberic_value(_data, _rgbName, color);

            if (value != FIND_KEY_VALUE_FAILED) {
                uint8_t _rValue = STRING_find_array_numberic_value(_data, _rgbName, R);
                uint8_t _gValue = STRING_find_array_numberic_value(_data, _rgbName, G);
                uint8_t _bValue = STRING_find_array_numberic_value(_data, _rgbName, B);

                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _RGB[_rgbCount] = new BlinkerRGB();
                        _RGB[_rgbCount]->name(_rgbName);
                        _RGB[_rgbCount]->freshValue(R, _rValue);
                        _RGB[_rgbCount]->freshValue(G, _gValue);
                        _RGB[_rgbCount]->freshValue(B, _bValue);
                        _rgbCount++;
                    }
                }
                else {
                    _RGB[num]->freshValue(R, _rValue);
                    _RGB[num]->freshValue(G, _gValue);
                    _RGB[num]->freshValue(B, _bValue);
                }

                _fresh = true;
                return value;
            }
            else {
                if( num == BLINKER_OBJECT_NOT_AVAIL ) {
                    if ( _rgbCount < BLINKER_MAX_WIDGET_SIZE ) {
                        _RGB[_rgbCount] = new BlinkerRGB();
                        _RGB[_rgbCount]->name(_rgbName);
                        _rgbCount++;
                    }
                    return 0;
                }
                
                return _RGB[num]->getValue(color);
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
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                    static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
                    stateData();
                    if (!static_cast<Proto*>(this)->endFormat()) {
                        static_cast<Proto*>(this)->beginFormat();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                        static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
                        static_cast<Proto*>(this)->endFormat();
                    }
                    _fresh = true;
                }
            }
        }

        void setSwitch() {
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
                if (STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET)) {
                    if (state == BLINKER_CMD_ON) {
                        _BUILTIN_SWITCH->freshState(true);
                    }
                    else {
                        _BUILTIN_SWITCH->freshState(false);
                    }
                    _fresh = true;
                    _switchFresh = true;
                    // static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
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

            if (STRING_find_string_value(data, state, BLINKER_CMD_GET)) {
                if (STRING_contains_string(data, BLINKER_CMD_SET)) {
                    if (state == BLINKER_CMD_ON) {
                        _BUILTIN_SWITCH->freshState(true);
                    }
                    else {
                        _BUILTIN_SWITCH->freshState(false);
                    }
                    _fresh = true;
                    _switchFresh = true;
                    // static_cast<Proto*>(this)->print(BLINKER_CMD_SWITCH, builtInSwitch()?"on":"off");
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

        void stateData() {
            for (uint8_t _tNum = 0; _tNum < _tCount; _tNum++) {
                static_cast<Proto*>(this)->print(_Toggle[_tNum]->getName(), _Toggle[_tNum]->getState() ? "on" : "off");
            }
            for (uint8_t _sNum = 0; _sNum < _sCount; _sNum++) {
                static_cast<Proto*>(this)->print(_Slider[_sNum]->getName(), _Slider[_sNum]->getValue());
            }
            // for (uint8_t _rgbNum = 0; _rgbNum < _rgbCount; _rgbNum++) {
            //     static_cast<Proto*>(this)->printArray(_RGB[_rgbNum]->getName(), "[" + STRING_format(_RGB[_rgbNum]->getValue(R)) + "," + STRING_format(_RGB[_rgbNum]->getValue(G)) + "," + STRING_format(_RGB[_rgbNum]->getValue(B)) + "]");
            // }
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
        bool timerManager(const JsonObject& data, bool _noSet = false) {
        // bool timerManager() {
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

            // if (isSet && (isCount || isLoop || isTiming)) {
            if (isCount || isLoop || isTiming) {
                _fresh = true;
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("get timer setting"));
    #endif
                if(!isSet && !_noSet) {
                    return false;
                }

                if (isCount) {
                    // String cd_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "countdown\"", ",", 1);
                    // if (cd_state == "") {
                    //     cd_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "countdown\"", "}", 1);
                    // }
                    if (isSet) {
                        _cdState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN];
                    }
                    else if(_noSet) {
                        _cdState = data[BLINKER_CMD_COUNTDOWN];
                    }
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
    #endif
                    // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;

                    // String _state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_STATE, "\"", 3);
                    // int32_t _totalTime = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TOTALTIME);
                    // int32_t _runTime = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_RUNTIME);
                    // String _action = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION, "]", 3);
                    if (isSet) {
                        String _state = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_STATE];
                        int32_t _totalTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_TOTALTIME];
                        _totalTime = 60 * _totalTime;
                        int32_t _runTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_RUNTIME];
                        _runTime = 60 * _runTime;
                        String _action = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_ACTION];
                    
                        _cdAction = _action;
                        _cdTime1 = _totalTime;
                        _cdTime2 = _runTime;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_state: "), _state);
    #endif
                    }
                    else if (_noSet) {
                        String _state = data[BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_STATE];
                        int32_t _totalTime = data[BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_TOTALTIME];
                        _totalTime = 60 * _totalTime;
                        int32_t _runTime = data[BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_RUNTIME];
                        _runTime = 60 * _runTime;
                        String _action = data[BLINKER_CMD_COUNTDOWNDATA][BLINKER_CMD_ACTION];
                    
                        _cdAction = _action;
                        _cdTime1 = _totalTime;
                        _cdTime2 = _runTime;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_state: "), _state);
    #endif
                    }

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_totalTime: "), _cdTime1);
                    BLINKER_LOG2(BLINKER_F("_runTime: "), _cdTime2);
                    BLINKER_LOG2(BLINKER_F("_action: "), _cdAction);
    #endif
                    // _cdAction = _action;
                    // _cdTime1 = _totalTime;
                    // _cdTime2 = _runTime;
                    
                    // _cdState = (cd_state == BLINKER_CMD_TRUE) ? true : false;
                    if (_cdState) {
                        cdTicker.once(_cdTime1 - _cdTime2, _cd_callback);
    #ifdef BLINKER_DEBUG_ALL 
                        BLINKER_LOG1(BLINKER_F("countdown start!"));
    #endif
                    }
                    else {
                        cdTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(countdownData(), false);
                }
                else if (isLoop) {
                    // String lp_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "loop\"", ",", 1);
                    // if (lp_state == "") {
                    //     lp_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "loop\"", "}", 1);
                    // }
                    if (isSet) {
                        _lpState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP];
                    }
                    else if (_noSet) {
                        _lpState = data[BLINKER_CMD_LOOP];
                    }
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("loop state: "), _lpState ? "true" : "false");
    #endif
                    // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;

                    // int8_t _times = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMES);
                    // String _state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_STATE, "\"", 3);
                    // int32_t _time1 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME1);
                    // String _action1 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION1, "]", 3);
                    // int32_t _time2 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME2);
                    // String _action2 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION2, "]", 3);
                    if (isSet) {
                        int8_t _times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOPDATA][BLINKER_CMD_TIMES];
                        String _state = data[BLINKER_CMD_SET][BLINKER_CMD_LOOPDATA][BLINKER_CMD_STATE];
                        int32_t _time1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOPDATA][BLINKER_CMD_TIME1];
                        _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOPDATA][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOPDATA][BLINKER_CMD_TIME2];
                        _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOPDATA][BLINKER_CMD_ACTION2];
                        
                        _lpAction1 = _action1;
                        _lpAction2 = _action2;

                        _lpTimes = _times;
                        _lpTime1 = _time1;
                        _lpTime2 = _time2;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_state: "), _state);
    #endif
                    }
                    else if (_noSet) {
                        int8_t _times = data[BLINKER_CMD_LOOPDATA][BLINKER_CMD_TIMES];
                        String _state = data[BLINKER_CMD_LOOPDATA][BLINKER_CMD_STATE];
                        int32_t _time1 = data[BLINKER_CMD_LOOPDATA][BLINKER_CMD_TIME1];
                        _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_LOOPDATA][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_LOOPDATA][BLINKER_CMD_TIME2];
                        _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_LOOPDATA][BLINKER_CMD_ACTION2];
                        
                        _lpAction1 = _action1;
                        _lpAction2 = _action2;

                        _lpTimes = _times;
                        _lpTime1 = _time1;
                        _lpTime2 = _time2;
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_state: "), _state);
    #endif
                    }

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_times: "), _lpTimes);
                    BLINKER_LOG2(BLINKER_F("_time1: "), _lpTime1);
                    BLINKER_LOG2(BLINKER_F("_action1: "), _lpAction1);
                    BLINKER_LOG2(BLINKER_F("_time2: "), _lpTime2);
                    BLINKER_LOG2(BLINKER_F("_action2: "), _lpAction2);
    #endif
                    // _lpAction1 = _action1;
                    // _lpAction2 = _action2;

                    // _lpTimes = _times;
                    // _lpTime1 = _time1;
                    // _lpTime2 = _time2;

                    // _lpState = (lp_state == BLINKER_CMD_TRUE) ? true : false;
                    if (_lpState) {
                        _lpRun1 = true;
                        _lpTrigged_times = 0;
                        lpTicker.once(_lpTime1, _lp_callback);
    #ifdef BLINKER_DEBUG_ALL 
                        BLINKER_LOG1(BLINKER_F("loop start!"));
    #endif
                    }
                    else {
                        lpTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(loopData(), false);
                }
                else if (isTiming) {
                    // String tm_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "timing\"", ",", 1);
                    // if (tm_state == "") {
                    //     tm_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "timing\"", "}", 1);
                    // }
                    if (isSet) {
                        _tmState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING];
                    }
                    else if (_noSet) {
                        _tmState = data[BLINKER_CMD_TIMING];
                    }
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("timing state: "), _tmState ? "true" : "false");
    #endif
                    // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;

                    // int8_t _times = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMES);
                    // String _state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_STATE, "\"", 3);
                    // int32_t _time1 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME1);
                    // String _action1 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION1, "]", 3);
                    // int32_t _time2 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME2);
                    // String _action2 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION2, "]", 3);
                    if (isSet) {
                        String _state = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_STATE];
                        int32_t _time1 = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_TIME1];
                        _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_TIME2];
                        _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_ACTION2];
                    
                        _tmAction1 = _action1;
                        _tmAction2 = _action2;

                        _tmTime1 = _time1;
                        _tmTime2 = _time2;

                        if (data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_DAY][0] == 7) {
                            // now_ntp = time(nullptr);
                            // gmtime_r(&now_ntp, &timeinfo);

                            if (_tmTime2 > dtime()) {
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
                            uint8_t taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_DAY][0];
                            // timingDay = 0x80;
                            _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    #ifdef BLINKER_DEBUG_ALL                            
                            BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    #endif                            

                            for (uint8_t day = 1;day < 7;day++) {
                                taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_DAY][day];
                                if (taskDay > 0) {
                                    _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    #ifdef BLINKER_DEBUG_ALL                                    
                                    BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    #endif
                                }
                            }

                            _isTimingLoop = true;
                        }
                        
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
                        BLINKER_LOG2(BLINKER_F("_state: "), _state);
    #endif
                    }
                    else if (_noSet) {
                        String _state = data[BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_STATE];
                        int32_t _time1 = data[BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_TIME1];
                        _time1 = 60 * _time1;
                        String _action1 = data[BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_ACTION1];
                        int32_t _time2 = data[BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_TIME2];
                        _time2 = 60 * _time2;
                        String _action2 = data[BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_ACTION2];
                    
                        _tmAction1 = _action1;
                        _tmAction2 = _action2;

                        _tmTime1 = _time1;
                        _tmTime2 = _time2;

                        if (data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_DAY][0] == 7) {
                            // now_ntp = time(nullptr);
                            // gmtime_r(&now_ntp, &timeinfo);

                            if (_tmTime2 > dtime()) {
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
                            uint8_t taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_DAY][0];
                            // timingDay = 0x80;
                            _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    #ifdef BLINKER_DEBUG_ALL                            
                            BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    #endif                            

                            for (uint8_t day = 1;day < 7;day++) {
                                taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMINGDATA][0][BLINKER_CMD_DAY][day];
                                if (taskDay > 0) {
                                    _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
    #ifdef BLINKER_DEBUG_ALL                                    
                                    BLINKER_LOG4(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
    #endif
                                }
                            }

                            _isTimingLoop = true;
                        }
                        
    #ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("timingDay: "), _timingDay);
                        BLINKER_LOG2(BLINKER_F("_state: "), _state);
    #endif
                    }

    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                    BLINKER_LOG2(BLINKER_F("_time1: "), _tmTime1);
                    BLINKER_LOG2(BLINKER_F("_action1: "), _tmAction1);
                    BLINKER_LOG2(BLINKER_F("_time2: "), _tmTime2);
                    BLINKER_LOG2(BLINKER_F("_action2: "), _tmAction2);
    #endif
                    // _tmAction1 = _action1;
                    // _tmAction2 = _action2;

                    // _tmTime1 = _time1;
                    // _tmTime2 = _time2;

                    // _tmState = (tm_state == BLINKER_CMD_TRUE) ? true : false;
                    if (_tmState) {
                        if (isTimingDay(wday())) {
                            int32_t nowTime = dtime();
    #ifdef BLINKER_DEBUG_ALL 
                                BLINKER_LOG2(BLINKER_F("nowTime: "), nowTime);
    #endif

                            if (_tmTime1 >= nowTime) {
                                _tmRun1 = true;
                                tmTicker.once(_tmTime1 - nowTime, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
                                BLINKER_LOG2(BLINKER_F("timing1 start! next time: "), _tmTime1 - nowTime);
    #endif
                            }
                            else if (_tmTime1 < nowTime && _tmTime2 > nowTime) {
                                _tmRun1 = false;
                                tmTicker.once(_tmTime2 - nowTime, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
                                BLINKER_LOG2(BLINKER_F("timing2 start! next time: "), _tmTime2 - nowTime);
    #endif                               
                            }
                            else if (_tmTime2 <= nowTime && _tmTime1 < nowTime) {
                                _tmRun1 = false;
                                tmTicker.once(BLINKER_ONE_DAY_TIME - nowTime, _tm_callback);
    #ifdef BLINKER_DEBUG_ALL 
                                BLINKER_LOG2(F("next day start! next time: "), BLINKER_ONE_DAY_TIME - nowTime);
    #endif
                            }
                        }
                        else {
                            int32_t nowTime = dtime();

                            _tmRun1 = false;
                            tmTicker.once(BLINKER_ONE_DAY_TIME - nowTime, _tm_callback);
                        }
    #ifdef BLINKER_DEBUG_ALL 
                        BLINKER_LOG1(BLINKER_F("timing start!"));
    #endif
                    }
                    else {
                        tmTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(timingData(), false);
                }

                return true;
            }
            else {
                return false;
            }
        }

        String countdownData() {
            String cdData;
            cdData = "{\"countdown\":" + STRING_format(_cdState ? "true" : "false") + \
                ",\"countdownData\":{\"state\":" + STRING_format(_cdState ? "\"run\"" : "\"parse\"") + \
                ",\"totalTime\":" + STRING_format(_cdTime1/60) + ",\"runTime\":" + STRING_format(_cdTime2) + \
                ",\"action\":" + _cdAction + \
                "}}";

            return cdData;
        }

        String loopData() {
            String lpData;
            lpData = "{\"loop\":" + STRING_format(_lpState ? "true" : "false") + \
                ",\"loopData\":{\"times\":" + _lpTimes + \
                ",\"state\":" + STRING_format(_lpState ? "\"run\"" : "\"parse\"") + \
                ",\"time1\":" + STRING_format(_lpTime1/60) + \
                ",\"action1\":" + _lpAction1 + \
                ",\"time2\":" + STRING_format(_lpTime2/60) + \
                ",\"action2\":" + _lpAction2 + \
                "}}";

            return lpData;
        }

        String timingDay() {    
            String timingDayStr = "";
            // uint8_t _timingDay;// = timingTask[task]->getTimingday();
            if (_isTimingLoop) {
                for (uint8_t day = 0; day < 7; day++) {
                    // timingDayStr += (timingDay & (uint8_t)pow(2,day)) ? String(day):String("");
                    if ((_timingDay >> day) & 0x01) {
                        timingDayStr += STRING_format(day);
                        if (day < 6 && (_timingDay >> (day + 1)))
                            timingDayStr += STRING_format(",");
                    }
                    // timingDayStr += String((day < 6) ? ((timingDay >> (day + 1)) ? ",":""):"");
                }
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG4(("timingDayStr: "), timingDayStr, (" timingDay: "), _timingDay);
    #endif
            }
            else {
                timingDayStr = STRING_format("7");
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(("timingDayStr: "), timingDayStr);
    #endif
            }

            return timingDayStr;
        }

        String timingData() {
            String tmData;
            tmData = "{\"timing\":" + STRING_format(_tmState ? "true" : "false") + \
                ",\"timingData\":{\"task\":0" + \
                ",\"state\":" + STRING_format(_tmState ? "\"run\"" : "\"parse\"") + \
                ",\"day\":[" + timingDay() + "]" + \
                ",\"time1\":" + STRING_format(_tmTime1/60) + \
                ",\"action1\":" + _tmAction1 + \
                ",\"time2\":" + STRING_format(_tmTime2/60) + \
                ",\"action2\":" + _tmAction2 + \
                "}}";

            return tmData;
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
                default :
                    return BLINKER_CMD_FALSE;
            }

        #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("message: "), msg);
        #endif

        #ifndef BLINKER_LAN_DEBUG
            const int httpsPort = 443;
        #elif defined(BLINKER_LAN_DEBUG)
            const int httpsPort = 9090;
        #endif
    #if defined(ESP8266)
        #ifndef BLINKER_LAN_DEBUG
            const char* host = "iotdev.clz.me";
        #elif defined(BLINKER_LAN_DEBUG)
            const char* host = "192.168.1.152";
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
                default :
                    return BLINKER_CMD_FALSE;
            }

            client_s.stop();

            return dataGet;
    #elif defined(ESP32)
            const char* host = "https://iotdev.clz.me";

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
                    http.addHeader("Content-Type", "application/json");
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
                    http.addHeader("Content-Type", "application/json");
                    httpCode = http.POST(msg);
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    url_iot = String(host) + "/api/v1" + msg;

                    http.begin(url_iot);
                    httpCode = http.GET();
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

                            BLINKER_LOG2("payload: ", payload);
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

                            BLINKER_LOG2("payload: ", payload);
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

                            BLINKER_LOG2("payload: ", payload);
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

                            BLINKER_LOG2("payload: ", payload);
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

                            BLINKER_LOG2("payload: ", payload);
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

                            BLINKER_LOG2("payload: ", payload);
                            break;
                        default :
                            return BLINKER_CMD_FALSE;
                    }
                }
                if (_type == BLINKER_CMD_SMS_NUMBER) {
                    _smsTime = millis();
                }
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
            if ((millis() - _smsTime) > BLINKER_SMS_MSG_LIMIT || _smsTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkPUSH() {
            if ((millis() - _pushTime) > BLINKER_PUSH_MSG_LIMIT || _pushTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkWECHAT() {
            if ((millis() - _wechatTime) > BLINKER_WECHAT_MSG_LIMIT || _wechatTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkWEATHER() {
            if ((millis() - _weatherTime) > BLINKER_WEATHER_MSG_LIMIT || _weatherTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkAQI() {
            if ((millis() - _aqiTime) > BLINKER_AQI_MSG_LIMIT || _aqiTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkCUPDATE() {
            if ((millis() - _cUpdateTime) > BLINKER_CONFIG_UPDATE_LIMIT || _cUpdateTime == 0) {
                return true;
            }
            else {
                return false;
            }
        }

        bool checkCGET() {
            if ((millis() - _cGetTime) > BLINKER_CONFIG_GET_LIMIT || _cGetTime == 0) {
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
#if defined(BLINKER_PRO)
        const char* _deviceType;
        BlinkerWlan Bwlan;

        callback_with_json_arg_t    _parseFunc = NULL;
        callbackFunction            _heartbeatFunc = NULL;
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
            reset();
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

                    for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                        buttonParse(_Button[bNum]->getName(), root);
                    }
                    for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                        slider(_Slider[sNum]->getName(), root);
                    }
                    for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                        toggle(_Toggle[kNum]->getName(), root);
                    }
                    for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
                        rgb(_RGB[rgbNum]->getName(), R, root);
                    }

                    joystick(J_Xaxis, root);
                    ahrs(Yaw, root);
                    gps(LONG, true, root);
#else
                    heartBeat();
                    setSwitch();
                    getVersion();

                    for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                        buttonParse(_Button[bNum]->getName(), _data);
                    }
                    for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                        slider(_Slider[sNum]->getName(), _data);
                    }
                    for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                        toggle(_Toggle[kNum]->getName(), _data);
                    }
                    for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
                        rgb(_RGB[rgbNum]->getName(), R, _data);
                    }

                    joystick(J_Xaxis);
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

                setSwitch(root);

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

                for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                    buttonParse(_Button[bNum]->getName(), _data);
                }
                for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                    slider(_Slider[sNum]->getName(), _data);
                }
                for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                    toggle(_Toggle[kNum]->getName(), _data);
                }
                for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
                    rgb(_RGB[rgbNum]->getName(), R, _data);
                }
#endif
            }
        }

//         void _parse(String data)
//         {
// #if defined(ESP8266) || defined(ESP32)
//             String data1 = "{\"data\":" + data + "}";
//             DynamicJsonBuffer jsonBuffer;
//             JsonObject& root = jsonBuffer.parseObject(data1);

//             if (!root.success()) {
//                 return;
//             }

//             String arrayData = root["data"][0];
// #ifdef BLINKER_DEBUG_ALL
//             // BLINKER_LOG4("data1: ", data1, " arrayData: ", arrayData);
//             BLINKER_LOG2("_parse data: ", data);
// #endif            
//             if (arrayData.length()) {
//                 for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++) {
//                     String array_data = root["data"][a_num];

//                     // BLINKER_LOG2("array_data: ", array_data);

//                     if(array_data.length()) {
//                         DynamicJsonBuffer _jsonBuffer;
//                         JsonObject& _array = _jsonBuffer.parseObject(array_data);
                        
//                         json_parse(_array);
//                         timerManager(_array, true);
//                     }
//                     else {
//                         return;
//                     }
//                 }
//             }
//             else {
//                 JsonObject& root = jsonBuffer.parseObject(data);

//                 if (!root.success()) {
//                     return;
//                 }

//                 json_parse(root);
//             }
// #else
//             for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
//                 buttonParse(_Button[bNum]->getName(), data);
//             }
//             for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
//                 slider(_Slider[sNum]->getName(), data);
//             }
//             for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
//                 toggle(_Toggle[kNum]->getName(), data);
//             }
//             for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
//                 rgb(_RGB[rgbNum]->getName(), R, data);
//             }
// #endif
//         }

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

            for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                buttonParse(_Button[bNum]->getName(), data);
            }
            for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                slider(_Slider[sNum]->getName(), data);
            }
            for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                toggle(_Toggle[kNum]->getName(), data);
            }
            for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
                rgb(_RGB[rgbNum]->getName(), R, data);
            }

            joystick(J_Xaxis, data);
            ahrs(Yaw, data);
            gps(LONG, true, data);
        }

        void json_parse(const JsonObject& data) {
            for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                buttonParse(_Button[bNum]->getName(), data);
            }
            for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                slider(_Slider[sNum]->getName(), data);
            }
            for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                toggle(_Toggle[kNum]->getName(), data);
            }
            for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
                rgb(_RGB[rgbNum]->getName(), R, data);
            }
        }

        bool ntpInit() {
            freshNTP();

            if (!_isNTPInit) {
                if ((millis() - _ntpStart) > BLINKER_NTP_TIMEOUT) {
                    _ntpStart = millis();
                }
                else {
                    return false;
                }

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
    #endif
    #ifdef BLINKER_DEBUG_ALL                
                BLINKER_LOG2(BLINKER_F("Current time: "), asctime(&timeinfo));
    #endif
                _isNTPInit = true;
            }

            return true;
        }
// #endif
// #if defined(BLINKER_MQTT)
        bool checkTimer() {
            if (_cdTrigged) {
                _cdTrigged = false;
    #ifdef BLINKER_DEBUG_ALL 
                BLINKER_LOG2(BLINKER_F("countdown trigged, action is: "), _cdAction);
    #endif
                // _parse(_cdAction);
                parse(_cdAction, true);
            }
            if (_lpTrigged) {
                _lpTrigged = false;

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

                if (_tmRun1) {
    #ifdef BLINKER_DEBUG_ALL 
                    BLINKER_LOG2(BLINKER_F("timing trigged, action is: "), _tmAction2);
    #endif
                    // _parse(_tmAction2);
                    parse(_tmAction2, true);
                }
                else {
    #ifdef BLINKER_DEBUG_ALL 
                    BLINKER_LOG2(BLINKER_F("timing trigged, action is: "), _tmAction1);
    #endif
                    // _parse(_tmAction1);
                    parse(_tmAction1, true);
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