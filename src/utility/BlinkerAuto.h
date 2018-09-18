#ifndef BLINKER_AUTO_H
#define BLINKER_AUTO_H

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
#include "modules/ArduinoJson/ArduinoJson.h"
#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerAUTO
{
    public :
        BlinkerAUTO()
            : _autoState(false)
            , _trigged(false)
        {}

        void run(String key, float data, int32_t nowTime) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6(BLINKER_F("BlinkerAUTO run key: "), key, 
                        BLINKER_F(" data: "), data, 
                        BLINKER_F(" nowTime: "), nowTime);
#endif
            for (uint8_t _num = 0; _num < _targetNum; _num++) {
                if (!_autoState) return;

                if (key != STRING_format(_targetKey[_num])) return;

                if (_time1[_num] < _time2[_num]) {
                    if (!(nowTime >= _time1[_num] && nowTime <= _time2[_num])) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
#endif
                        return;
                    }
                }
                else if (_time1[_num] > _time2[_num]) {
                    if (nowTime > _time1[_num] && nowTime < _time2[_num]) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
#endif
                        return;
                    }
                }

                if ((_logicType == BLINKER_TYPE_NUMERIC)
                    || (_logicType == BLINKER_TYPE_OR 
                    && logic_type[_num] == BLINKER_TYPE_NUMERIC)
                    || (_logicType == BLINKER_TYPE_AND 
                    && logic_type[_num] == BLINKER_TYPE_NUMERIC))
                {
                    switch (_compareType[_num]) {
                        case BLINKER_COMPARE_LESS:
                            if (data < _targetData[_num]) {
                                if (!_isTrigged[_num]) {
                                    triggerCheck("less", _num);
                                }
                            }
                            else {
                                _isTrigged[_num] = false;
                                isRecord[_num] = false;
                                _trigged = false;
                            }
                            break;
                        case BLINKER_COMPARE_EQUAL:
                            if (data = _targetData[_num]) {
                                if (!_isTrigged[_num]) {
                                    triggerCheck("equal", _num);
                                }
                            }
                            else {
                                _isTrigged[_num] = false;
                                isRecord[_num] = false;
                                _trigged = false;
                            }
                            break;
                        case BLINKER_COMPARE_GREATER:
                            if (data > _targetData[_num]) {
                                if (!_isTrigged[_num]) {
                                    triggerCheck("greater", _num);
                                }
                            }
                            else {
                                _isTrigged[_num] = false;
                                isRecord[_num] = false;
                                _trigged = false;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        void run(String key, String state, int32_t nowTime) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6(BLINKER_F("BlinkerAUTO run key: "), key, BLINKER_F(" state: "), state, BLINKER_F(" nowTime: "), nowTime);
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
                        BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
#endif
                        return;
                    }
                }
                else if (_time1[_num] > _time2[_num]) {
                    if (nowTime > _time1[_num] && nowTime < _time2[_num]) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("out of time slot: "), nowTime);
#endif
                        return;
                    }
                }

                // if (_logicType == BLINKER_TYPE_STATE) {
                if ((_logicType == BLINKER_TYPE_STATE)
                    || (_logicType == BLINKER_TYPE_OR 
                    && logic_type[_num] == BLINKER_TYPE_STATE)
                    || (_logicType == BLINKER_TYPE_AND 
                    && logic_type[_num] == BLINKER_TYPE_STATE))
                {
                    if (state == BLINKER_CMD_ON) {
                        if (_targetState[_num]) {
                            if (!_isTrigged[_num]) {
                                triggerCheck("on", _num);
                            }
                        }
                        else {
                            _isTrigged[_num] = false;
                            isRecord[_num] = false;
                            _trigged = false;
                        }
                    }
                    else if (state == BLINKER_CMD_OFF) {
                        if (!_targetState[_num]) {
                            if (!_isTrigged[_num]) {
                                triggerCheck("off", _num);
                            }
                        }
                        else {
                            _isTrigged[_num] = false;
                            isRecord[_num] = false;
                            _trigged = false;
                        }
                    }
                }
                else if (_logicType == BLINKER_TYPE_OR 
                    && logic_type[_num] == BLINKER_TYPE_STATE)
                {
                    if (state == BLINKER_CMD_ON) {
                        if (_targetState[_num]) {
                            if (!_isTrigged[_num]) {
                                triggerCheck("on", _num);
                            }
                        }
                        else {
                            _isTrigged[_num] = false;
                            isRecord[_num] = false;
                            _trigged = false;
                        }
                    }
                    else if (state == BLINKER_CMD_OFF) {
                        if (!_targetState[_num]) {
                            if (!_isTrigged[_num]) {
                                triggerCheck("off", _num);
                            }
                        }
                        else {
                            _isTrigged[_num] = false;
                            isRecord[_num] = false;
                            _trigged = false;
                        }
                    }
                }
            }
        }

        void manager(String data) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(data);

            // String auto_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "auto\"", ",", 1);
            _autoState = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_ENABLE];
            // if (auto_state == "") {
            //     auto_state = STRING_find_string(static_cast<Proto*>(this)->dataParse(), "auto\"", "}", 1);
            // }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("==============================================="));
            BLINKER_LOG2(BLINKER_F("auto state: "), _autoState);
#endif
            // _autoState = (auto_state == BLINKER_CMD_TRUE) ? true : false;
            // _autoState = auto_state;

            // _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);
            _autoId = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_ID];

            // String logicType;
            String logicType = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_LOGIC];
            // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), logicType, BLINKER_CMD_LOGICTYPE)) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("_autoId: "), _autoId);
            BLINKER_LOG2(BLINKER_F("logicType: "), logicType);
#endif
            // String target_key;
            // String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
            // // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_KEY);
            // strcpy(_targetKey[0], target_key.c_str());
            
            if (logicType == BLINKER_CMD_STATE) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("state!"));
#endif
                String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
                // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_KEY);
                strcpy(_targetKey[0], target_key.c_str());
                
                _targetNum = 1;

                _logicType = BLINKER_TYPE_STATE;

                logic_type[0] = BLINKER_TYPE_STATE;
                // String target_state;
                String target_state = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][0][BLINKER_CMD_VALUE];
                // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_state, BLINKER_CMD_VALUE)) {
                if (target_state == BLINKER_CMD_ON) {
                    _targetState[0] = true;
                }
                else if (target_state == BLINKER_CMD_OFF) {
                    _targetState[0] = false;
                }

                _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                [BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
                _duration[0] = 60 * _duration[0];
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
                    BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[0]);
                    BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[0]);
                    BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
#endif
                // }
            }
            else if (logicType == BLINKER_CMD_NUMBERIC) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("numberic!"));
#endif
                String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
                // STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_key, BLINKER_CMD_KEY);
                strcpy(_targetKey[0], target_key.c_str());
                
                _targetNum = 1;
                
                _logicType = BLINKER_TYPE_NUMERIC;

                logic_type[0] = BLINKER_TYPE_NUMERIC;
                // String _type;
                String _type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                            [BLINKER_CMD_DATA][0][BLINKER_CMD_TYPE];
                // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), _type, BLINKER_CMD_TYPE)) {
                    if (_type == BLINKER_CMD_LESS) {
                        _compareType[0] = BLINKER_COMPARE_LESS;
                    }
                    else if (_type == BLINKER_CMD_EQUAL) {
                        _compareType[0] = BLINKER_COMPARE_EQUAL;
                    }
                    else if (_type == BLINKER_CMD_GREATER) {
                        _compareType[0] = BLINKER_COMPARE_GREATER;
                    }

                    // _targetData = STRING_find_float_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_VALUE);
                    _targetData[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][0][BLINKER_CMD_VALUE];
                    _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
                    _duration[0] = 60 * _duration[0];

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
                    BLINKER_LOG4(BLINKER_F("_type: "), _type, 
                                BLINKER_F(" _compareType: "), _compareType[0]);
                    BLINKER_LOG4(BLINKER_F("_targetKey: "), _targetKey[0], 
                                BLINKER_F(" _targetData: "), _targetData[0]);
                    BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
#endif
                // }
            }
            else if (logicType == BLINKER_CMD_OR || logicType == BLINKER_CMD_AND) {
                if (logicType == BLINKER_CMD_OR) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("or!"));
#endif
                    _logicType = BLINKER_TYPE_OR;
                }
                else if (logicType == BLINKER_CMD_AND) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("and!"));
#endif
                    _logicType = BLINKER_TYPE_AND;
                }

                _targetNum = 2;

                for (uint8_t t_num = 0; t_num < _targetNum; t_num++) {
                    String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                        [BLINKER_CMD_DATA][t_num][BLINKER_CMD_KEY];

                    String compare_type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                        [BLINKER_CMD_DATA][t_num][BLINKER_CMD_TYPE];

                    if (compare_type.length()) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(BLINKER_F("or/and numberic!"));
#endif
                        logic_type[t_num] = BLINKER_TYPE_NUMERIC;

                        strcpy(_targetKey[t_num], target_key.c_str());

                        // String _type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DATA][0][BLINKER_CMD_TYPE];
                    // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), _type, BLINKER_CMD_TYPE)) {
                        if (compare_type == BLINKER_CMD_LESS) {
                            _compareType[t_num] = BLINKER_COMPARE_LESS;
                        }
                        else if (compare_type == BLINKER_CMD_EQUAL) {
                            _compareType[t_num] = BLINKER_COMPARE_EQUAL;
                        }
                        else if (compare_type == BLINKER_CMD_GREATER) {
                            _compareType[t_num] = BLINKER_COMPARE_GREATER;
                        }

                        // _targetData = STRING_find_float_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_VALUE);
                        _targetData[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                            [BLINKER_CMD_DATA][t_num][BLINKER_CMD_VALUE];
                        _duration[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                            [BLINKER_CMD_DATA][t_num][BLINKER_CMD_DURATION];
                        _duration[t_num] = 60 * _duration[t_num];
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
                        BLINKER_LOG2(BLINKER_F("_compareType: "), _compareType[t_num]);
                        BLINKER_LOG4(BLINKER_F("_targetKey: "), _targetKey[t_num], BLINKER_F(" _targetData: "), _targetData[t_num]);
                        BLINKER_LOG2(BLINKER_F("_duration: "), _duration[t_num]);
#endif
                    }
                    else {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(BLINKER_F("or/and state!"));
#endif
                        logic_type[t_num] = BLINKER_TYPE_STATE;

                        strcpy(_targetKey[t_num], target_key.c_str());
                
                        // String target_state;
                        String target_state = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][t_num][BLINKER_CMD_VALUE];
                        // if (STRING_find_string_value(static_cast<Proto*>(this)->dataParse(), target_state, BLINKER_CMD_VALUE)) {
                        if (target_state == BLINKER_CMD_ON) {
                            _targetState[t_num] = true;
                        }
                        else if (target_state == BLINKER_CMD_OFF) {
                            _targetState[t_num] = false;
                        }

                        _duration[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_DATA][t_num][BLINKER_CMD_DURATION];
                        _duration[t_num] = 60 * _duration[t_num];
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(BLINKER_F("_logicType: "), _logicType);
                        BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[t_num]);
                        BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[t_num]);
                        BLINKER_LOG2(BLINKER_F("_duration: "), _duration[t_num]);
#endif
                    }
                }
            }

            // int32_t duValue = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DURATION);
            // _duration[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
            // _duration[0] = 60 * _duration[0];

            // if (duValue != FIND_KEY_VALUE_FAILED) {
            //     _duration = 60 * STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DURATION);
            // }
            // else {
            //     _duration = 0;
            // }
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2("_duration: ", _duration[0]);
// #endif
            // int32_t timeValue = STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_RANGE, 0);
            int32_t timeValue = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_RANGE][0];

            if (timeValue) {
                // _time1 = 60 * STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_RANGE, 0);
                _time1[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                [BLINKER_CMD_RANGE][0];
                _time1[0] = 60 * _time1[0];
                // _time2 = 60 * STRING_find_array_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_RANGE, 1);
                _time2[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                [BLINKER_CMD_RANGE][1];
                _time2[0] = 60 * _time2[0];
            }
            else {
                _time1[0] = 0;
                _time2[0] = 24 * 60 * 60;
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG4(BLINKER_F("_time1: "), _time1[0], BLINKER_F(" _time2: "), _time2[0]);
#endif
            // String datas;
            // datas = STRING_find_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_LINKDATA, "]", 3);

            // strcpy(_linkDevice[0], STRING_find_string(datas, BLINKER_CMD_LINKDEVICE, "\"", 3).c_str());
            // strcpy(_linkType[0], STRING_find_string(datas, BLINKER_CMD_LINKTYPE, "\"", 3).c_str());
            // strcpy(_linkData[0], STRING_find_string(datas, BLINKER_CMD_DATA, "}", 3).c_str());
            // _aCount++;
            String linkData = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_LINKDATA];

            if (linkData.length() > 0) {
                String link_device = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][0][BLINKER_CMD_LINKDEVICE];
                String link_type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][0][BLINKER_CMD_LINKTYPE];
                String link_data = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][0][BLINKER_CMD_DATA];
                
                strcpy(_linkDevice[0], link_device.c_str());
                strcpy(_linkType[0], link_type.c_str());
                strcpy(_linkData[0], link_data.c_str());

                const char* link_device2 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKDEVICE];

                if (link_device2) {
                    _linkNum = 2;

                    String link_device1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKDEVICE];
                    String link_type1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][1][BLINKER_CMD_LINKTYPE];
                    String link_data1 = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                    [BLINKER_CMD_LINKDATA][1][BLINKER_CMD_DATA];
                    
                    strcpy(_linkDevice[1], link_device1.c_str());
                    strcpy(_linkType[1], link_type1.c_str());
                    strcpy(_linkData[1], link_data1.c_str());

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
                    BLINKER_LOG2(BLINKER_F("_linkDevice1: "), _linkDevice[0]);
                    BLINKER_LOG2(BLINKER_F("_linkType1: "), _linkType[0]);
                    BLINKER_LOG2(BLINKER_F("_linkData1: "), _linkData[0]);
                    BLINKER_LOG2(BLINKER_F("_linkDevice2: "), _linkDevice[1]);
                    BLINKER_LOG2(BLINKER_F("_linkType2: "), _linkType[1]);
                    BLINKER_LOG2(BLINKER_F("_linkData2: "), _linkData[1]);
                    BLINKER_LOG2(BLINKER_F("_targetNum: "), _targetNum);
                    // BLINKER_LOG2("_aCount: ", _aCount);
                    BLINKER_LOG1(BLINKER_F("==============================================="));
#endif
                }
                else {
                    _linkNum = 1;

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
                    BLINKER_LOG2(BLINKER_F("_linkDevice1: "), _linkDevice[0]);
                    BLINKER_LOG2(BLINKER_F("_linkType1: "), _linkType[0]);
                    BLINKER_LOG2(BLINKER_F("_linkData1: "), _linkData[0]);
                    BLINKER_LOG2(BLINKER_F("_targetNum: "), _targetNum);
                    // BLINKER_LOG2("_aCount: ", _aCount);
                    BLINKER_LOG1(BLINKER_F("==============================================="));
#endif
                }
            }
            else {
                _linkNum = 0;
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
            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_AUTOID, _autoId);
            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_TYPESTATE, _typeState);

            _logicType = _typeState >> 6;// | _autoState << 4 | _linkNum;
            _autoState = _typeState >> 4 & 0x03;
            _linkNum = _typeState & 0x0F;

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("_typeState: "), _typeState);
#endif

            if (_logicType == BLINKER_TYPE_STATE || _logicType == BLINKER_TYPE_NUMERIC) {
                _targetNum = 1;
            }
            else {
                _targetNum = 2;
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("_targetNum: "), _targetNum);
#endif
            // if (_targetNum == 1) {
            // for (uint8_t t_num = 0; t_num < _targetNum; t_num++) {
            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_AUTO1, _autoData[0]);

            // _linkNum = _autoId >> 30;
            // _autoId = _autoId;// & 0xFFFFFFFF;
            // _autoState = _autoData >> 31;
            logic_type[0] = _autoData[0] >> 30 & 0x03;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("==============================================="));
            BLINKER_LOG2(BLINKER_F("_autoId: "), _autoId);
            // BLINKER_LOG2("_autoState: ", _autoState ? "true" : "false");
            // BLINKER_LOG2("_logicType: ", _logicType ? "numberic" : "state");
            switch (_logicType) {
                case BLINKER_TYPE_STATE :
                    BLINKER_LOG1(BLINKER_F("_logicType: state"));
                    break;
                case BLINKER_TYPE_NUMERIC :
                    BLINKER_LOG1(BLINKER_F("_logicType: numberic"));
                    break;
                case BLINKER_TYPE_OR :
                    BLINKER_LOG1(BLINKER_F("_logicType: or"));
                    break;
                case BLINKER_TYPE_AND :
                    BLINKER_LOG1(BLINKER_F("_logicType: and"));
                    break;
                default :
                    break;
            }
#endif

            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
                
            if (logic_type[0] == BLINKER_TYPE_STATE) {
                _targetState[0] = _autoData[0] >> 28 & 0x03;
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[0]);
                BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[0] ? "on" : "off");
#endif
            }
            else {
                _compareType[0] = _autoData[0] >> 28 & 0x03;
                // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_compareType: "), _compareType[0] ? (_compareType[0] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
                BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[0]);
                BLINKER_LOG2(BLINKER_F("_targetData: "), _targetData[0]);
#endif
            }

            _duration[0] = (_autoData[0] >> 22 & 0x3f) * 60;
            _time1[0] = (_autoData[0] >> 11 & 0x7ff) * 60;
            _time2[0] = (_autoData[0] & 0x7ff) * 60;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
            BLINKER_LOG4(BLINKER_F("_time1: "), _time1[0], BLINKER_F(" _time2: "), _time2[0]);
#endif

            if (_targetNum == 2) {
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_AUTO2, _autoData[1]);

                // _linkNum = _autoId >> 30;
                // _autoId = _autoId;// & 0xFFFFFFFF;
                // _autoState = _autoData >> 31;
                logic_type[1] = _autoData[1] >> 30 & 0x03;
// #ifdef BLINKER_DEBUG_ALL
//                     // BLINKER_LOG1("===============================================");
//                     BLINKER_LOG2("_autoId: ", _autoId);
//                     // BLINKER_LOG2("_autoState: ", _autoState ? "true" : "false");
//                     BLINKER_LOG2("_logicType: ", _logicType ? "numberic" : "state");
// #endif

                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
                    
                if (logic_type[1] == BLINKER_TYPE_STATE) {
                    _targetState[1] = _autoData[1] >> 28 & 0x03;
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[1]);
                    BLINKER_LOG2(BLINKER_F("_targetState: "), _targetState[1] ? "on" : "off");
#endif
                }
                else {
                    _compareType[1] = _autoData[1] >> 28 & 0x03;
                    // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
                    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_compareType: "), _compareType[1] ? (_compareType[1] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
                    BLINKER_LOG2(BLINKER_F("_targetKey: "), _targetKey[1]);
                    BLINKER_LOG2(BLINKER_F("_targetData: "), _targetData[1]);
#endif
                }

                _duration[1] = (_autoData[1] >> 22 & 0x3f) * 60;
                _time1[1] = (_autoData[1] >> 11 & 0x7ff) * 60;
                _time2[1] = (_autoData[1] & 0x7ff) * 60;

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_duration: "), _duration[1]);
                BLINKER_LOG4(BLINKER_F("_time1: "), _time1[1], BLINKER_F(" _time2: "), _time2[1]);
#endif
            }

            // EEPROM.get(BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
            // EEPROM.get(BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
            // EEPROM.get(BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
            if (_linkNum > 0) {
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
                EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
                BLINKER_LOG2(BLINKER_F("_linkDevice: "), _linkDevice[0]);
                BLINKER_LOG2(BLINKER_F("_linkType: "), _linkType[0]);
                BLINKER_LOG2(BLINKER_F("_linkData: "), _linkData[0]);
                BLINKER_LOG2(BLINKER_F("_typeState: "), _typeState);

                // BLINKER_LOG1("===============================================");
#endif          

                if (_linkNum == 2) {
                    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_LINKDEVICE2, _linkDevice[1]);
                    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_LINKTYPE2, _linkType[1]);
                    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_LINKDATA2, _linkData[1]);
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(BLINKER_F("_linkNum: "), _linkNum);
                    BLINKER_LOG2(BLINKER_F("_linkDevice: "), _linkDevice[1]);
                    BLINKER_LOG2(BLINKER_F("_linkType: "), _linkType[1]);
                    BLINKER_LOG2(BLINKER_F("_linkData: "), _linkData[1]);
                    BLINKER_LOG2(BLINKER_F("_typeState: "), _typeState);

                    // BLINKER_LOG1("===============================================");
#endif              
                }
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("==============================================="));
#endif
            EEPROM.commit();
            EEPROM.end();
            // }
        }

        void serialization() {
            uint8_t checkData;

            // _autoData = _autoState << 31 | _logicType << 30 ;
            _typeState = _logicType << 6 | _autoState << 4 | _linkNum;

            EEPROM.begin(BLINKER_EEP_SIZE);

            EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

            if (checkData != BLINKER_CHECK_DATA) {
                EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
            }

            EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_AUTOID, _autoId);
            EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_TYPESTATE, _typeState);

            // if (_targetNum == 1) {
            // for (uint8_t t_num = 0; t_num < _targetNum; t_num++) {
            _autoData[0] = logic_type[0] << 30 ;
            if (logic_type[0] == BLINKER_TYPE_STATE) {
                _autoData[0] |= _targetState[0] << 28;
            }
            else {
                _autoData[0] |= _compareType[0] << 28;
            }
            _autoData[0] |= _duration[0] / 60 << 22 | _time1[0] / 60 << 11 | _time2[0] / 60;
        
            // EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);

            // _autoId = _linkNum << 30 | _autoId;
            // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE, _autoId);
            // EEPROM.put(BLINKER_EEP_ADDR_AUTO, _autoData);
            EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
            EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
                
            if (logic_type[0] == BLINKER_TYPE_NUMERIC) {
                // EEPROM.put(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("serialization _typeState: "), _typeState);
            BLINKER_LOG2(BLINKER_F("serialization _autoData: "), _autoData[0]);
#endif                 
            if (_targetNum == 2) {
                _autoData[1] = logic_type[1] << 30 ;
                if (logic_type[1] == BLINKER_TYPE_STATE) {
                    _autoData[1] |= _targetState[1] << 28;
                }
                else {
                    _autoData[1] |= _compareType[1] << 28;
                }
                _autoData[1] |= _duration[1] / 60 << 22 | _time1[1] / 60 << 11 | _time2[1] / 60;
            
                // EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);

                // _autoId = _linkNum << 30 | _autoId;
                // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + _num * BLINKER_ONE_AUTO_DATA_SIZE, _autoId);
                // EEPROM.put(BLINKER_EEP_ADDR_AUTO, _autoData);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_AUTO2, _autoData[1]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
                    
                if (logic_type[1] == BLINKER_TYPE_NUMERIC) {
                    // EEPROM.put(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
                    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
                }
#ifdef BLINKER_DEBUG_ALL
            // BLINKER_LOG2("serialization _typeState: ", _typeState);
                BLINKER_LOG2(BLINKER_F("serialization _autoData: "), _autoData[1]);
#endif  
            }

            // EEPROM.put(BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
            // EEPROM.put(BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
            // EEPROM.put(BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
            if (_linkNum > 0) {
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_LINKDEVICE1, _linkDevice[0]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_LINKTYPE1, _linkType[0]);
                EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                            a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                            BLINKER_EEP_ADDR_LINKDATA1, _linkData[0]);
            
                if (_linkNum == 2) {
                    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_LINKDEVICE2, _linkDevice[1]);
                    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_LINKTYPE2, _linkType[1]);
                    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                                BLINKER_EEP_ADDR_LINKDATA2, _linkData[1]);
                }
            }
            
            EEPROM.commit();
            EEPROM.end();
        }

        void setNum(uint8_t num) {
            a_num = num;
        }

        bool isTrigged() { return _trigged; }

        void fresh() {
            for (uint8_t _num = 0; _num < _targetNum; _num++) {
                if (isRecord[_num]) {
                    _isTrigged[_num] = true;
                }
            }
            _trigged = false;
        }

        uint32_t id() { return _autoId; }

        // uint8_t type() { return _logicType; }

        uint8_t linkNum() { return _linkNum; }

        char * name(uint8_t num) { return _linkDevice[num]; }

        char * type(uint8_t num) { return _linkType[num]; }

        char * data(uint8_t num) { return _linkData[num]; }

    private :
        uint8_t     a_num;
        
        // {
        //     "auto":{
        //         "ena":1,//_autoState
        //         "id":123456,//_autoId
        //         "logic":"numberic",//_logicType
        //         "data":
        //         [
        //             {
        //                 "key":"humi",
        //                 "value":40,
        //                 "type":"<",//_targetState|_compareType
        //                 "dur":10
        //             }
        //         ],
        //         "range":[540, 1260],
        //     }
        // }

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
        bool        _isTrigged[2];
        bool        _trigged;
        uint8_t     _linkNum;
        char        _linkDevice[2][BLINKER_LINKDEVICE_SIZE];
        char        _linkType[2][BLINKER_LINKTYPE_SIZE];
        char        _linkData[2][BLINKER_LINKDATA_SIZE];

        void triggerCheck(String state, uint8_t num) {
            if (!isRecord[num]) {
                isRecord[num] = true;
                _treTime[num] = millis();
            }
#ifdef BLINKER_DEBUG_ALL
            switch (_logicType) {
                case BLINKER_TYPE_STATE :
                    // BLINKER_LOG1("_logicType: state");
                    state = "state " + state;
                    break;
                case BLINKER_TYPE_NUMERIC :
                    // BLINKER_LOG1("_logicType: numberic");
                    state = "numberic " + state;
                    break;
                case BLINKER_TYPE_OR :
                    // BLINKER_LOG1("_logicType: or");
                    state = "or " + state;
                    break;
                case BLINKER_TYPE_AND :
                    // BLINKER_LOG1("_logicType: and");
                    state = "and " + state;
                    break;
                default :
                    break;
            }
#endif

            if ((millis() - _treTime[num]) / 1000 >= _duration[num]) {
                if (_logicType != BLINKER_TYPE_AND) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(state, BLINKER_F(" trigged"));
#endif
                    _trigged = true;
                }
                else {
                    _isTrigged[num] = true;
                    if (_isTrigged[0] && _isTrigged[1]) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(state, BLINKER_F(" trigged"));
#endif
                        _trigged = true;
                    }
                }
            }
        }
};
#endif

#endif