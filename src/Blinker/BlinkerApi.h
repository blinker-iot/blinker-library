#ifndef BlinkerApi_H
#define BlinkerApi_H

#include <time.h>
#if defined(ESP8266) || defined(ESP32)
    #include <Ticker.h>
    #include <EEPROM.h>
    #include "modules/ArduinoJson/ArduinoJson.h"
#endif
#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerDebug.h>
#include <utility/BlinkerUtility.h>

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
static class BlinkerRGB * _RGB[BLINKER_MAX_WIDGET_SIZE];
#if defined(BLINKER_MQTT)
static class BlinkerAUTO * _AUTO[2];
#endif

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

#if defined(BLINKER_MQTT)
class BlinkerAUTO
{
    public :
        BlinkerAUTO()
            : _autoState(false)
        {}

        void run(String key, float data, int32_t nowTime) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6("BlinkerAUTO run key: ", key, " data: ", data, " nowTime: ", nowTime);
#endif
            for (uint8_t _num = 0; _num < _targetNum; _num++) {
                if (!_autoState) {
                    return;
                }

                if (key != STRING_format(_targetKey[_num])) {
                    return;
                }

                if (_time1[_num] < _time2[_num]) {
                    if (!(nowTime >= _time1[_num] && nowTime <= _time2[_num])) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2("out of time slot: ", nowTime);
#endif
                        return;
                    }
                }
                else if (_time1[_num] > _time2[_num]) {
                    if (nowTime > _time1[_num] && nowTime < _time2[_num]) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2("out of time slot: ", nowTime);
#endif
                        return;
                    }
                }

                switch (_compareType[_num]) {
                    case BLINKER_COMPARE_LESS:
                        if (data < _targetData[_num]) {
                            if (!isTrigged[_num]) {
                                triggerCheck("less");
                            }
                        }
                        else {
                            isTrigged[_num] = false;
                            isRecord[_num] = false;
                        }
                        break;
                    case BLINKER_COMPARE_EQUAL:
                        if (data = _targetData[_num]) {
                            if (!isTrigged[_num]) {
                                triggerCheck("equal");
                            }
                        }
                        else {
                            isTrigged[_num] = false;
                            isRecord[_num] = false;
                        }
                        break;
                    case BLINKER_COMPARE_GREATER:
                        if (data > _targetData[_num]) {
                            if (!isTrigged[_num]) {
                                triggerCheck("greater");
                            }
                        }
                        else {
                            isTrigged[_num] = false;
                            isRecord[_num] = false;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        void triggerCheck(String state) {
            if (!isRecord[_num]) {
                isRecord[_num] = true;
                _treTime[_num] = millis();
            }

            if ((millis() - _treTime[_num]) / 1000 >= _duration[_num]) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(state, " trigged");
#endif
                // if (static_cast<Proto*>(this)->autoTrigged(_linkDevice[_num], _linkType[_num], _linkData[_num]))
                // {
                    isTrigged[_num] = true;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1("trigged sucessed");
#endif
//                 }
//                 else
//                 {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG1("trigged failed");
// #endif
//                 }
            }
        }

        void manager(String data) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(data);

            // String auto_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "auto\"", ",", 1);
            bool auto_state = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO];
            // if (auto_state == "") {
            //     auto_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "auto\"", "}", 1);
            // }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("auto state: ", auto_state);
#endif
            // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;
            _autoState = auto_state;

            // _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);
            _autoId = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_AUTOID];

            // String logicType;
            String logicType = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICTYPE];
            // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), logicType, BLINKER_CMD_LOGICTYPE)) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("_autoId: ", _autoId);
            BLINKER_LOG2("logicType: ", logicType);
#endif
            // String target_key;
            String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETKEY];
            // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_TARGETKEY);
            strcpy(_targetKey[0], target_key.c_str());
            
            if (logicType == BLINKER_CMD_STATE) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("state!");
#endif
                _targetNum = 1;

                _logicType = BLINKER_TYPE_STATE;

                logic_type[0] = BLINKER_TYPE_STATE;
                // String target_state;
                String target_state = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETSTATE];
                // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_state, BLINKER_CMD_TARGETSTATE)) {
                    if (target_state == BLINKER_CMD_ON) {
                        _targetState[0] = true;
                    }
                    else if (target_state == BLINKER_CMD_OFF) {
                        _targetState[0] = false;
                    }
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_logicType: ", _logicType);
                    BLINKER_LOG2("_targetKey: ", _targetKey[0]);
                    BLINKER_LOG2("_targetState: ", _targetState[0]);
#endif
                // }
            }
            else if (logicType == BLINKER_CMD_NUMBERIC) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("numberic!");
#endif
                _targetNum = 1;
                
                _logicType = BLINKER_TYPE_NUMERIC;

                logic_type[0] = BLINKER_TYPE_NUMERIC;
                // String _type;
                String _type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_COMPARETYPE];
                // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), _type, BLINKER_CMD_COMPARETYPE)) {
                    if (_type == BLINKER_CMD_LESS) {
                        _compareType[0] = BLINKER_COMPARE_LESS;
                    }
                    else if (_type == BLINKER_CMD_EQUAL) {
                        _compareType[0] = BLINKER_COMPARE_EQUAL;
                    }
                    else if (_type == BLINKER_CMD_GREATER) {
                        _compareType[0] = BLINKER_COMPARE_GREATER;
                    }

                    // _targetData = STRING_find_float_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TARGETDATA);
                    _targetData[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_TARGETDATA];
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_logicType: ", _logicType);
                    BLINKER_LOG4("_type: ", _type, " _compareType: ", _compareType[0]);
                    BLINKER_LOG4("_targetKey: ", _targetKey[0], " _targetData: ", _targetData[0]);
#endif
                // }
            }

            // int32_t duValue = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DURATION);
            _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LOGICDATA][0][BLINKER_CMD_DURATION];
            _duration[0] = 60 * _duration[0];

            // if (duValue != FIND_KEY_VALUE_FAILED) {
            //     _duration = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DURATION);
            // }
            // else {
            //     _duration = 0;
            // }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("_duration: ", _duration[0]);
#endif
            // int32_t timeValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMESLOT, 0);
            int32_t timeValue = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_TIMESLOT][0];

            if (timeValue) {
                // _time1 = 60 * STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMESLOT, 0);
                _time1[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_TIMESLOT][0];
                _time1[0] = 60 * _time1[0];
                // _time2 = 60 * STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMESLOT, 1);
                _time2[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_TIMESLOT][1];
                _time2[0] = 60 * _time2[0];
            }
            else {
                _time1[0] = 0;
                _time2[0] = 24 * 60 * 60;
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG4("_time1: ", _time1[0], " _time2: ", _time2[0]);
#endif
            // String datas;
            // datas = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_LINKDATA, "]", 3);

            // strcpy(_linkDevice[0], STRING_find_string(datas, BLINKER_CMD_LINKDEVICE, "\"", 3).c_str());
            // strcpy(_linkType[0], STRING_find_string(datas, BLINKER_CMD_LINKTYPE, "\"", 3).c_str());
            // strcpy(_linkData[0], STRING_find_string(datas, BLINKER_CMD_DATA, "}", 3).c_str());
            // _aCount++;

            String link_device = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][0][BLINKER_CMD_LINKDEVICE];
            String link_type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][0][BLINKER_CMD_LINKTYPE];
            String link_data = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][0][BLINKER_CMD_DATA];
            
            strcpy(_linkDevice[0], link_device.c_str());
            strcpy(_linkType[0], link_type.c_str());
            strcpy(_linkData[0], link_data.c_str());

            const char* link_device2 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKDEVICE];

            if (link_device2) {
                _linkNum = 2;

                String link_device1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKDEVICE];
                String link_type1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKTYPE];
                String link_data1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTODATA][BLINKER_CMD_LINKDATA][1][BLINKER_CMD_DATA];
                
                strcpy(_linkDevice[1], link_device1.c_str());
                strcpy(_linkType[1], link_type1.c_str());
                strcpy(_linkData[1], link_data1.c_str());

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2("_linkNum: ", _linkNum);
                BLINKER_LOG2("_linkDevice1: ", _linkDevice[0]);
                BLINKER_LOG2("_linkType1: ", _linkType[0]);
                BLINKER_LOG2("_linkData1: ", _linkData[0]);
                BLINKER_LOG2("_linkDevice2: ", _linkDevice[1]);
                BLINKER_LOG2("_linkType2: ", _linkType[1]);
                BLINKER_LOG2("_linkData2: ", _linkData[1]);
                BLINKER_LOG2("_targetNum: ", _targetNum);
                // BLINKER_LOG2("_aCount: ", _aCount);
#endif
            }
            else {
                _linkNum = 1;

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2("_linkNum: ", _linkNum);
                BLINKER_LOG2("_linkDevice1: ", _linkDevice[0]);
                BLINKER_LOG2("_linkType1: ", _linkType[0]);
                BLINKER_LOG2("_linkData1: ", _linkData[0]);
                BLINKER_LOG2("_targetNum: ", _targetNum);
                // BLINKER_LOG2("_aCount: ", _aCount);
#endif
            }

            serialization();
        }

        void deserialization() {
            uint8_t checkData;
            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

            if (checkData != BLINKER_CHECK_DATA) {
                _autoState = false;
                EEPROM.commit();
                EEPROM.end();
                return;
            }

//             EEPROM.get(BLINKER_EEP_ADDR_AUTONUM, _aCount);
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2("_aCount: ", _aCount);
// #endif
            // EEPROM.get(BLINKER_EEP_ADDR_AUTOID, _autoId);
            // EEPROM.get(BLINKER_EEP_ADDR_AUTO, _autoData);
            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTOID, _autoId);
            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TYPESTATE, _typeState);

            _logicType = _typeState >> 6;// | _autoState << 4 | _linkNum;
            _autoState = _typeState >> 4 & 0x03;
            _linkNum = _typeState & 0x0F;

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("_typeState: ", _typeState);
#endif

            if (_logicType == BLINKER_TYPE_STATE || _logicType == BLINKER_TYPE_NUMERIC) {
                _targetNum = 1;
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("_targetNum: ", _targetNum);
#endif
            if (_targetNum == 1) {
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTO1, _autoData[0]);

                // _linkNum = _autoId >> 30;
                // _autoId = _autoId;// & 0xFFFFFFFF;
                // _autoState = _autoData >> 31;
                logic_type[0] = _autoData[0] >> 30 & 0x03;
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("===============================================");
                BLINKER_LOG2("_autoId: ", _autoId);
                // BLINKER_LOG2("_autoState: ", _autoState ? "true" : "false");
                BLINKER_LOG2("_logicType: ", _logicType ? "numberic" : "state");
#endif
                if (_logicType == BLINKER_TYPE_STATE) {
                    _targetState[0] = _autoData[0] >> 28 & 0x03;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_targetState: ", _targetState[0] ? "on" : "off");
#endif
                }
                else {
                    _compareType[0] = _autoData[0] >> 28 & 0x03;
                    // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
                    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
                    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_compareType: ", _compareType[0] ? (_compareType[0] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
                    BLINKER_LOG2("_targetKey: ", _targetKey[0]);
                    BLINKER_LOG2("_targetData: ", _targetData[0]);
#endif
                }

                _duration[0] = (_autoData[0] >> 22 & 0x3f) * 60;
                _time1[0] = (_autoData[0] >> 11 & 0x7ff) * 60;
                _time2[0] = (_autoData[0] & 0x7ff) * 60;

                // EEPROM.get(BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
                // EEPROM.get(BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
                // EEPROM.get(BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2("_duration: ", _duration[0]);
                BLINKER_LOG4("_time1: ", _time1[0], " _time2: ", _time2[0]);
                BLINKER_LOG2("_linkNum: ", _linkNum);
                BLINKER_LOG2("_linkDevice: ", _linkDevice[0]);
                BLINKER_LOG2("_linkType: ", _linkType[0]);
                BLINKER_LOG2("_linkData: ", _linkData[0]);
                BLINKER_LOG2("_typeState: ", _typeState);

                BLINKER_LOG1("===============================================");
#endif
                EEPROM.commit();
                EEPROM.end();
            }
        }

        void serialization() {
            uint8_t checkData;

            // _autoData = _autoState << 31 | _logicType << 30 ;
            _typeState = _logicType << 6 | _autoState << 4 | _linkNum;

            if (_targetNum == 1) {
                _autoData[0] = logic_type[0] << 30 ;
                if (_logicType == BLINKER_TYPE_STATE) {
                    _autoData[0] |= _targetState[0] << 28;
                }
                else {
                    _autoData[0] |= _compareType[0] << 28;
                }
                _autoData[0] |= _duration[0] / 60 << 22 | _time1[0] / 60 << 11 | _time2[0] / 60;

                EEPROM.begin(BLINKER_EEP_SIZE);

                EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

                if (checkData != BLINKER_CHECK_DATA) {
                    EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
                }

                // EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);

                // _autoId = _linkNum << 30 | _autoId;
                // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE, _autoId);
                // EEPROM.put(BLINKER_EEP_ADDR_AUTO, _autoData);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTOID, _autoId);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TYPESTATE, _typeState);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
                    
                if (_logicType == BLINKER_TYPE_NUMERIC) {
                    // EEPROM.put(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
                    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
                }

                // EEPROM.put(BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
                // EEPROM.put(BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
                // EEPROM.put(BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE + BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);

                EEPROM.commit();
                EEPROM.end();
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2("serialization _typeState: ", _typeState);
                BLINKER_LOG2("serialization _autoData: ", _autoData[0]);
#endif
            }
        }

        void setNum(uint8_t num) {
            _num = num;
        }

        uint32_t id() { return _autoId; }

        uint8_t type() { return _logicType; }

    private :
        uint8_t     _num;
        // - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -
        // | | | | |            | _time1 0-1440min 11  | _time2 0-1440min 11                   
        // | | | | | _duration 0-60min 6
        // | | | | _targetState|_compareType on/off|less/equal/greater 2
        // | | | _targetState|_compareType on/off|less/equal/greater
        // |  
        // | logic_type state/numberic 2
        // autoData

        // - - - - - - - -
        // |   |   |
        // |   |   | _linkNum
        // |   | _autoState true/false 1  
        // | _logicType state/numberic/and/or 2
        // typestate
        bool        _autoState;
        uint32_t    _autoId = 0;
        uint8_t     _logicType;
        uint8_t     _typeState;
        uint32_t    _autoData[2];

        uint8_t     _targetNum = 0;

        uint8_t     logic_type[2];
        char        _targetKey[2][12];
        float       _targetData[2];
        uint8_t     _compareType[2];
        bool        _targetState[2];
        uint32_t    _time1[2];
        uint32_t    _time2[2];
        uint32_t    _duration[2];
        uint32_t    _treTime[2];
        bool        isRecord[2];
        bool        isTrigged[2];
        uint8_t     _linkNum;
        char        _linkDevice[2][BLINKER_LINKDEVICE_SIZE];
        char        _linkType[2][BLINKER_LINKTYPE_SIZE];
        char        _linkData[2][BLINKER_LINKDATA_SIZE];
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

static bool _cdState = false;
static bool _lpState = false;
static bool _tmState = false;
static bool _lpRun1 = true;
static bool _tmRun1 = true;
static bool _cdTrigged = false;
static bool _lpTrigged = false;
static bool _tmTrigged = false;

static uint8_t _lpTimes;
static uint8_t _lpTrigged_times;
static uint32_t _cdTime1;
static uint32_t _cdTime2;
static uint32_t _lpTime1;
static uint32_t _lpTime2;
static uint32_t _tmTime1;
static uint32_t _tmTime2;

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
    BLINKER_LOG1("countdown trigged!");
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
                lpTicker.attach(_lpTime1, _lp_callback);
            }
        }
        else {
            lpTicker.attach(_lpTime1, _lp_callback);
        }
    }
    else {
        lpTicker.attach(_lpTime2, _lp_callback);
    }
    _lpTrigged = true;
    
    BLINKER_LOG1("loop trigged!");
}

// static void _loop(float seconds) {
//     _lpState = false;
//     lpTicker.attach(seconds, _lp_callback);
// }

static void _tm_callback() {
    _tmRun1 = !_tmRun1;

    time_t      now_ntp;
    struct tm   timeinfo;
    now_ntp = time(nullptr);
    gmtime_r(&now_ntp, &timeinfo);

    int32_t nowTime = timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;

    BLINKER_LOG6("nowTime: ", nowTime, " _tmTime1: ", _tmTime1, " _tmTime2: ", _tmTime2);

    if (_tmRun1) {
        if (_tmTime1 >= nowTime) {
            tmTicker.attach(_tmTime1 - nowTime, _tm_callback);
            BLINKER_LOG2("timing2 trigged! next time: ", _tmTime1 - nowTime);
        }
        else if (_tmTime2 <= nowTime && _tmTime1 < nowTime) {
            tmTicker.attach(BLINKER_ONE_DAY_TIME - nowTime + _tmTime1, _tm_callback);
            BLINKER_LOG2("timing2 trigged! next time: ", BLINKER_ONE_DAY_TIME - nowTime + _tmTime1);
        }
    }
    else {
        tmTicker.attach(_tmTime2 - _tmTime1, _tm_callback);
        BLINKER_LOG2("timing1 trigged! next time: ", _tmTime2 - _tmTime1);
    }
    _tmTrigged = true;
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
            // rgbValue[R] = 0;
            // rgbValue[G] = 0;
            // rgbValue[B] = 0;
        }

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
            String state;

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
                    BLINKER_LOG2("GET: ", static_cast<Proto*>(this)->dataParse());
                    if (STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AHRS)) {
                        BLINKER_LOG1("AHRS attach sucessed...");
                        parse();
                        state = true;
                        break;
                    }
                    else {
                        BLINKER_LOG1("AHRS attach failed...Try again");
                        startTime = millis();
                        static_cast<Proto*>(this)->print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                    }
                }
                else {
                    if (millis() - startTime > BLINKER_CONNECT_TIMEOUT_MS) {
                        BLINKER_LOG1("AHRS attach failed...Try again");
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

        int8_t second()    { freshNTP(); return _isNTPInit ? timeinfo.tm_sec : -1; }
        /**< seconds after the minute - [ 0 to 59 ] */
        int8_t minute()    { freshNTP(); return _isNTPInit ? timeinfo.tm_min : -1; }
        /**< minutes after the hour - [ 0 to 59 ] */
        int8_t hour()   { freshNTP(); return _isNTPInit ? timeinfo.tm_hour : -1; }
        /**< hours since midnight - [ 0 to 23 ] */
        int8_t mday()   { freshNTP(); return _isNTPInit ? timeinfo.tm_mday : -1; }
        /**< day of the month - [ 1 to 31 ] */
        int8_t wday()   { freshNTP(); return _isNTPInit ? timeinfo.tm_wday : -1; }
        /**< days since Sunday - [ 0 to 6 ] */
        int8_t month()    { freshNTP(); return _isNTPInit ? timeinfo.tm_mon : -1; }
        /**< months since January - [ 0 to 11 ] */
        int16_t year()  { freshNTP(); return _isNTPInit ? 1900 + timeinfo.tm_year : -1; }
        /**< years since 1900 */
        int16_t yday()  { freshNTP(); return _isNTPInit ? timeinfo.tm_yday : -1; }
        /**< days since January 1 - [ 0 to 365 ] */
        time_t  time()  { freshNTP(); return _isNTPInit ? now_ntp : millis(); }

        int32_t dtime() {
            freshNTP();
            return _isNTPInit ? timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec : -1;
        }

#if defined(BLINKER_MQTT)
        void beginAuto() {
            BLINKER_LOG1("=======================================================");
            BLINKER_LOG1("=========== Blinker Auto Control mode init! ===========");
            BLINKER_LOG1("Warning! EEPROM address 0-255 is used for Auto Control!");
            BLINKER_LOG1("=======================================================");

            BLINKER_LOG2("Already used: ", BLINKER_ONE_AUTO_DATA_SIZE);

            // deserialization();
            autoStart();
        }

        void autoRun(String key, String state) {
#ifdef BLINKER_DEBUG_ALL            
            BLINKER_LOG2("autoRun state: ", state);
#endif
            if (key != STRING_format(_targetKey)) {
                return;
            }
            
            if (!_isNTPInit || !_autoState) {
                return;
            }

            int32_t nowTime = dtime();
            if (_time1 < _time2) {
                if (!(nowTime >= _time1 && nowTime <= _time2)) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("out of time slot: ", nowTime);
#endif
                    return;
                }
            }
            else if (_time1 > _time2) {
                if (nowTime > _time1 && nowTime < _time2) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("out of time slot: ", nowTime);
#endif
                    return;
                }
            }

            if (state == BLINKER_CMD_ON) {
                if (_targetState) {
                    if (!isTrigged) {
                        triggerCheck("on");
                    }
                }
                else {
                    isTrigged = false;
                    isRecord = false;
                }
            }
            else if (state == BLINKER_CMD_OFF) {
                if (!_targetState) {
                    if (!isTrigged) {
                        triggerCheck("off");
                    }
                }
                else {
                    isTrigged = false;
                    isRecord = false;
                }
            }
        }

        void autoRun(String key, float data) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("autoRun data: ", data);
#endif
            // if (!_isNTPInit || !_autoState) {
            if (!_isNTPInit) {
                return;
            }

            int32_t nowTime = dtime();

            for (uint8_t _num = 0; _num < _aCount; _num++) {
                _AUTO[_num]->run(key, data, nowTime);
            }
//             if (key != STRING_format(_targetKey)) {
//                 return;
//             }

//             if (_time1 < _time2) {
//                 if (!(nowTime >= _time1 && nowTime <= _time2)) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2("out of time slot: ", nowTime);
// #endif
//                     return;
//                 }
//             }
//             else if (_time1 > _time2) {
//                 if (nowTime > _time1 && nowTime < _time2) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG2("out of time slot: ", nowTime);
// #endif
//                     return;
//                 }
//             }

//             switch (_compareType) {
//                 case BLINKER_COMPARE_LESS:
//                     if (data < _targetData) {
//                         if (!isTrigged) {
//                             triggerCheck("less");
//                         }
//                     }
//                     else {
//                         isTrigged = false;
//                         isRecord = false;
//                     }
//                     break;
//                 case BLINKER_COMPARE_EQUAL:
//                     if (data = _targetData) {
//                         if (!isTrigged) {
//                             triggerCheck("equal");
//                         }
//                     }
//                     else {
//                         isTrigged = false;
//                         isRecord = false;
//                     }
//                     break;
//                 case BLINKER_COMPARE_GREATER:
//                     if (data > _targetData) {
//                         if (!isTrigged) {
//                             triggerCheck("greater");
//                         }
//                     }
//                     else {
//                         isTrigged = false;
//                         isRecord = false;
//                     }
//                     break;
//                 default:
//                     break;
//             }
        }
// #else
//     #pragma message("This code is intended to run with BLINKER_MQTT! Please check your connect type.")
#endif
    
    private :
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

#if defined(BLINKER_MQTT)
        // - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -
        // | | | | |            | _time1 0-1440min 11  | _time2 0-1440min 11                   
        // | | | | | _duration 0-60min 6
        // | | | | _targetState|_compareType on/off|less/equal/greater 2
        // | | | _targetState|_compareType on/off|less/equal/greater
        // | | _logicType state/numberic 1
        // | _autoState true/false 1
        bool        _autoState = false;
        uint32_t    _autoId;
        uint8_t     _logicType;
        char        _targetKey[12];
        float       _targetData;
        uint8_t     _compareType = -1;
        bool        _targetState;
        uint32_t    _time1;
        uint32_t    _time2;
        uint32_t    _duration;
        uint32_t    _treTime;
        bool        isRecord = false;
        bool        isTrigged = false;
        uint8_t     _linkNum = 0;
        char        _linkDevice[2][BLINKER_LINKDEVICE_SIZE];
        char        _linkType[2][BLINKER_LINKTYPE_SIZE];
        char        _linkData[2][BLINKER_LINKDATA_SIZE];
        uint32_t    _autoData;

        uint8_t     _aCount = 0;
#endif

#if defined(ESP8266) || defined(ESP32)
        String      _cdAction;
        String      _lpAction1;
        String      _lpAction2;
        String      _tmAction1;
        String      _tmAction2;
#endif

        void freshNTP() {
            if (_isNTPInit) {
                now_ntp = ::time(nullptr);
                gmtime_r(&now_ntp, &timeinfo);
            }
        }

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

        bool buttonParse(const String & _bName, String data)
        {
            int8_t num = checkNum(_bName, _Button, _bCount);
            String state;

            if (STRING_find_string_value(data, state, _bName)) {
                // _fresh = true;
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

                // _fresh = true;
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

                // _fresh = true;
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

                // _fresh = true;
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

        bool toggle(const String & _tName, String data)
        {
            int8_t num = checkNum(_tName, _Toggle, _tCount);
            String state;

            if (STRING_find_string_value(data, state, _tName)) {
                // _fresh = true;
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

                // _fresh = true;
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

                // _fresh = true;
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

        uint8_t slider(const String & _sName, String data)
        {
            int8_t num = checkNum(_sName, _Slider, _sCount);
            int16_t value = STRING_find_numberic_value(data, _sName);

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

                // _fresh = true;
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

        uint8_t rgb(const String & _rgbName, b_rgb_t color, String data) {
            int8_t num = checkNum(_rgbName, _RGB, _rgbCount);
            int16_t value = STRING_find_array_numberic_value(data, _rgbName, color);

            if (value != FIND_KEY_VALUE_FAILED) {
                uint8_t _rValue = STRING_find_array_numberic_value(data, _rgbName, R);
                uint8_t _gValue = STRING_find_array_numberic_value(data, _rgbName, G);
                uint8_t _bValue = STRING_find_array_numberic_value(data, _rgbName, B);

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

                // _fresh = true;
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

        void heartBeat() {
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
                // _fresh = true;
                if (state == BLINKER_CMD_STATE) {
#if defined(BLINKER_MQTT)
                    static_cast<Proto*>(this)->beginFormat();
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                    stateData();
                    if (!static_cast<Proto*>(this)->endFormat()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                    }
#else
                    static_cast<Proto*>(this)->beginFormat();
                    static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                    stateData();
                    if (!static_cast<Proto*>(this)->endFormat()) {
                        static_cast<Proto*>(this)->print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                    }
#endif
                    _fresh = true;
                }
            }
        }

        void getVersion() {
            String state;

            if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), state, BLINKER_CMD_GET)) {
                // _fresh = true;
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
            for (uint8_t _rgbNum = 0; _rgbNum < _rgbCount; _rgbNum++) {
                static_cast<Proto*>(this)->print(_RGB[_rgbNum]->getName(), "[" + STRING_format(_RGB[_rgbNum]->getValue(R)) + "," + STRING_format(_RGB[_rgbNum]->getValue(G)) + "," + STRING_format(_RGB[_rgbNum]->getValue(B)) + "]");
            }
        }

#if defined(BLINKER_MQTT)
        bool autoManager() {
            // String set;
            bool isSet = false;
            bool isAuto = false;

            isSet = STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET);
            isAuto = STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTO);

            if (isSet && isAuto) {
                _fresh = true;
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("get auto setting");
#endif
                _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);

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
                        BLINKER_LOG2("_aCount: ", _aCount);
#endif
                        // static_cast<Proto*>(this)->_print(autoData(), false);
                        return true;
                    }
                    else {
                        _AUTO[_aCount - 1]->manager(static_cast<Proto*>(this)->dataParse());
                        return true;
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
                    BLINKER_LOG2("_aCount: ", _aCount);
#endif
                    // static_cast<Proto*>(this)->_print(autoData(), false);
                    return true;
                }
            }
            else {
                return false;
            }
        }

        void triggerCheck(String state) {
            if (!isRecord) {
                isRecord = true;
                _treTime = millis();
            }

            if ((millis() - _treTime) / 1000 >= _duration) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(state, " trigged");
#endif
                if (static_cast<Proto*>(this)->autoTrigged(_linkDevice[0], _linkType[0], _linkData[0]))
                {
                    isTrigged = true;
// #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1("trigged sucessed");
// #endif
                }
                else
                {
// #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1("trigged failed");
// #endif
                }
            }
        }

        void autoStart() {
            EEPROM.begin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_AUTONUM, _aCount);
            EEPROM.commit();
            EEPROM.end();
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("_aCount: ", _aCount);
#endif
            if (_aCount > 2) {
                _aCount = 2;
            }
            if (_aCount) {
                for (uint8_t _num = 0; _num < _aCount; _num++) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("new BlinkerAUTO() _num: ", _num);
#endif
                    _AUTO[_num] = new BlinkerAUTO();
                    _AUTO[_num]->setNum(_num);
                    _AUTO[_num]->deserialization();
                }
            }
        }

        String logicData() {
            String _logicData;

            if (_logicType == BLINKER_TYPE_STATE) {
                _logicData = "{\"targetState\":\"" + STRING_format(_targetState ? BLINKER_CMD_ON : BLINKER_CMD_OFF) + "\"" + \
                    ",\"targetKey\":\"" + STRING_format(_targetKey) + "\"" + \
                    ",\"duration\":" + STRING_format(_duration) + "}";
            }
            else if (_logicType == BLINKER_TYPE_NUMERIC) {
                _logicData = "{\"copmpareType\":\"" + STRING_format(_compareType ? ((_compareType == BLINKER_COMPARE_GREATER) ? BLINKER_CMD_GREATER : BLINKER_CMD_EQUAL) : BLINKER_CMD_LESS) + "\"" + \
                    ",\"targetKey\":\"" + STRING_format(_targetKey) + "\"" + \
                    ",\"targetData\":" + STRING_format(_targetData) + \
                    ",\"duration\":" + STRING_format(_duration) + "}";
            }

            return _logicData;
        }

        String autoData() {
            String _auto_data;
            _auto_data = "{\"auto\":" + STRING_format(_autoState ? "true" : "false") + \
                ",\"autoData\":{\"autoId\":\"123\"" + \
                ",\"logicType\":\"" + STRING_format(_logicType ? BLINKER_CMD_NUMBERIC : BLINKER_CMD_STATE) + "\"" + \
                ",\"logicData\":[" + logicData() + "]" + \
                ",\"timeSlot\":[" + STRING_format(_time1/60) + "," + STRING_format(_time2/60) + "]" + \
                ",\"linkData\":[{\"linkDevice\":\"" + STRING_format(_linkDevice[0]) + "\"" + \
                ",\"linkType\":\"" + STRING_format(_linkType[0]) + "\"" + \
                ",\"data\":{" + STRING_format(_linkData[0]) + "}" + \
                "}]}}";

            return _auto_data;
        }
#endif

#if defined(ESP8266) || defined(ESP32)
        bool timerManager() {
            bool isSet = false;
            bool isCount = false;
            bool isLoop = false;
            bool isTiming = false;

            isSet = STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET);
            isCount = STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_COUNTDOWN);
            isLoop = STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_LOOP);
            isTiming = STRING_contais_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMING);

            if (isSet && (isCount || isLoop || isTiming)) {
                _fresh = true;
                BLINKER_LOG1("get timer setting");

                if (isCount) {
                    String cd_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "countdown\"", ",", 1);
                    if (cd_state == "") {
                        cd_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "countdown\"", "}", 1);
                    }
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("countdown state: ", cd_state);
#endif
                    // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;

                    String _state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_STATE, "\"", 3);
                    int32_t _totalTime = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TOTALTIME);
                    int32_t _runTime = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_RUNTIME);
                    String _action = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION, "]", 3);

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_state: ", _state);
                    BLINKER_LOG2("_totalTime: ", _totalTime);
                    BLINKER_LOG2("_runTime: ", _runTime);
                    BLINKER_LOG2("_action: ", _action);
#endif
                    _cdAction = _action;

                    _cdTime1 = _totalTime;
                    _cdTime2 = _runTime;
                    
                    _cdState = (cd_state == BLINKER_CMD_TRUE) ? true : false;
                    if (_cdState) {
                        cdTicker.once(_totalTime - _runTime, _cd_callback);
                        BLINKER_LOG1("countdown start!");
                    }
                    else {
                        cdTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(countdownData(), false);
                }
                else if (isLoop) {
                    String lp_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "loop\"", ",", 1);
                    if (lp_state == "") {
                        lp_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "loop\"", "}", 1);
                    }
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("countdown state: ", lp_state);
#endif
                    // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;

                    int8_t _times = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMES);
                    String _state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_STATE, "\"", 3);
                    int32_t _time1 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME1);
                    String _action1 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION1, "]", 3);
                    int32_t _time2 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME2);
                    String _action2 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION2, "]", 3);

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_times: ", _times);
                    BLINKER_LOG2("_state: ", _state);
                    BLINKER_LOG2("_time1: ", _time1);
                    BLINKER_LOG2("_action1: ", _action1);
                    BLINKER_LOG2("_time2: ", _time2);
                    BLINKER_LOG2("_action2: ", _action2);
#endif
                    _lpAction1 = _action1;
                    _lpAction2 = _action2;

                    _lpTimes = _times;
                    _lpTime1 = _time1;
                    _lpTime2 = _time2;

                    _lpState = (lp_state == BLINKER_CMD_TRUE) ? true : false;
                    if (_lpState) {
                        _lpRun1 = true;
                        _lpTrigged_times = 0;
                        lpTicker.attach(_lpTime1, _lp_callback);
                        BLINKER_LOG1("loop start!");
                    }
                    else {
                        lpTicker.detach();
                    }

                    static_cast<Proto*>(this)->_print(loopData(), false);
                }
                else if (isTiming) {
                    String tm_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "timing\"", ",", 1);
                    if (tm_state == "") {
                        tm_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "timing\"", "}", 1);
                    }
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("countdown state: ", tm_state);
#endif
                    // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;

                    // int8_t _times = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIMES);
                    String _state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_STATE, "\"", 3);
                    int32_t _time1 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME1);
                    String _action1 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION1, "]", 3);
                    int32_t _time2 = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_TIME2);
                    String _action2 = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_ACTION2, "]", 3);

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2("_state: ", _state);
                    BLINKER_LOG2("_time1: ", _time1);
                    BLINKER_LOG2("_action1: ", _action1);
                    BLINKER_LOG2("_time2: ", _time2);
                    BLINKER_LOG2("_action2: ", _action2);
#endif
                    _tmAction1 = _action1;
                    _tmAction2 = _action2;

                    _tmTime1 = _time1;
                    _tmTime2 = _time2;

                    _tmState = (tm_state == BLINKER_CMD_TRUE) ? true : false;
                    if (_tmState) {
                        int32_t nowTime = dtime();

                        if (_tmTime1 >= nowTime) {
                            _tmRun1 = true;
                            tmTicker.attach(_tmTime1 - nowTime, _tm_callback);
                        }
                        else if (_tmTime1 < nowTime && _tmTime2 > nowTime) {
                            _tmRun1 = false;
                            tmTicker.attach(_tmTime2 - nowTime, _tm_callback);
                        }
                        else if (_tmTime2 <= nowTime && _tmTime1 < nowTime) {
                            _tmRun1 = true;
                            tmTicker.attach(BLINKER_ONE_DAY_TIME - nowTime + _tmTime1, _tm_callback);
                        }
                        BLINKER_LOG1("timing start!");
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
                ",\"action\":[" + _cdAction + "]" + \
                "}}";

            return cdData;
        }

        String loopData() {
            String lpData;
            lpData = "{\"loop\":" + STRING_format(_lpState ? "true" : "false") + \
                ",\"loopData\":{\"times\":" + _lpTimes + \
                ",\"state\":" + STRING_format(_lpState ? "\"run\"" : "\"parse\"") + \
                ",\"time1\":" + STRING_format(_lpTime1/60) + \
                ",\"action1\":[" + _lpAction1 + "]" + \
                ",\"time2\":" + STRING_format(_lpTime2/60) + \
                ",\"action2\":[" + _lpAction2 + "]" + \
                "}}";

            return lpData;
        }

        String timingData() {
            String tmData;
            tmData = "{\"timing\":" + STRING_format(_tmState ? "true" : "false") + \
                ",\"timingData\":{\"task\":0" + \
                ",\"state\":" + STRING_format(_tmState ? "\"run\"" : "\"parse\"") + \
                ",\"day\":[0,1,2,3,4,5,6]" + \
                ",\"time1\":" + STRING_format(_tmTime1/60) + \
                ",\"action1\":[" + _tmAction1 + "]" + \
                ",\"time2\":" + STRING_format(_tmTime2/60) + \
                ",\"action2\":[" + _tmAction2 + "]" + \
                "}}";

            return tmData;
        }
#endif

    protected :
        void parse()
        {
            if (static_cast<Proto*>(this)->parseState() ) {
                _fresh = false;

#if defined(BLINKER_MQTT)
                if (autoManager()) {
                    static_cast<Proto*>(this)->isParsed();
                    return;
                }
#endif
#if defined(ESP8266) || defined(ESP32)
                if (timerManager()) {
                    static_cast<Proto*>(this)->isParsed();
                    return;
                }
#endif
                heartBeat();
                getVersion();

                for (uint8_t bNum = 0; bNum < _bCount; bNum++) {
                    buttonParse(_Button[bNum]->getName());
                }
                for (uint8_t sNum = 0; sNum < _sCount; sNum++) {
                    slider(_Slider[sNum]->getName());
                }
                for (uint8_t kNum = 0; kNum < _tCount; kNum++) {
                    toggle(_Toggle[kNum]->getName());
                }
                for (uint8_t rgbNum = 0; rgbNum < _rgbCount; rgbNum++) {
                    rgb(_RGB[rgbNum]->getName(), R);
                }

                joystick(J_Xaxis);
                ahrs(Yaw);
                gps(LONG, true);

                if (_fresh) {
                    static_cast<Proto*>(this)->isParsed();
                }
            }
        }

        void _parse(String data)
        {
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

#if defined(ESP8266) || defined(ESP32)
        bool ntpInit() {
            freshNTP();

            if (!_isNTPInit) {
                if ((millis() - _ntpStart) > BLINKER_NTP_TIMEOUT) {
                    _ntpStart = millis();
                }
                else {
                    return false;
                }

                now_ntp = ::time(nullptr);
            
                // BLINKER_LOG4("Setting time using SNTP: ", now_ntp, " ", _timezone * 3600 * 2);
                
                if (now_ntp < _timezone * 3600 * 2) {
                    configTime(_timezone * 3600, 0, "ntp1.aliyun.com", "210.72.145.44", "time.pool.aliyun.com");// cn.pool.ntp.org
                    now_ntp = ::time(nullptr);

                    if (now_ntp < _timezone * 3600 * 2) {
                        ::delay(50);

                        now_ntp = ::time(nullptr);

                        // BLINKER_LOG4("Setting time using SNTP2: ", now_ntp, " ", _timezone * 3600 * 2);

                        return false;
                    }
                }
                // struct tm timeinfo;
                gmtime_r(&now_ntp, &timeinfo);
#ifdef BLINKER_DEBUG_ALL                
                BLINKER_LOG2("Current time: ", asctime(&timeinfo));
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
                BLINKER_LOG2("countdown trigged, action is: ", _cdAction);
                _parse(_cdAction);
            }
            if (_lpTrigged) {
                _lpTrigged = false;

                if (_lpRun1) {
                    BLINKER_LOG2("loop trigged, action is: ", _lpAction2);
                    _parse(_lpAction2);
                }
                else {
                    BLINKER_LOG2("loop trigged, action is: ", _lpAction1);
                    _parse(_lpAction1);
                }
            }
            if (_tmTrigged) {
                _tmTrigged = false;

                if (_tmRun1) {
                    BLINKER_LOG2("timing trigged, action is: ", _tmAction2);
                    _parse(_tmAction2);
                }
                else {
                    BLINKER_LOG2("timing trigged, action is: ", _tmAction1);
                    _parse(_tmAction1);
                }
            }
        }
#endif
};

#endif