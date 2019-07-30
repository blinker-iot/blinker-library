#ifndef BLINKER_AUTO_H
#define BLINKER_AUTO_H

#if ((defined(ESP8266) || defined(ESP32)) && \
    (defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_PRO_SIM7020) || \
    defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
    defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_SUBDEVICE)))

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include <EEPROM.h>

// #include "Blinker/BlinkerAuto.h"
#include "BlinkerConfig.h"
#include "BlinkerDebug.h"
#include "BlinkerUtility.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif

class BlinkerAUTO
{
    public :
        BlinkerAUTO()
            : _haveAuto(false)
            , _autoState(false)
            , _trigged(false)
        {}

        void run(const String & key, float data, int32_t nowTime);
        void run(const String & key, const String & state, int32_t nowTime);
        void manager(const String & data);
        void deserialization();
        void serialization();
        void setNum(uint8_t num) { a_num = num; }
        bool isTrigged() { return _trigged; }
        void fresh();
        uint32_t id() { return _autoId; }

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
        //         "range":[540, 1260]
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

        // | _linkNum
        // - - - - - - - -
        // | | |_logicType state/numberic/and/or 2    
        // | | _autoState true/false 1    
        // | _haveAuto     
        // | 
        // typestate
        bool        _haveAuto;
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

        void triggerCheck(const String & state, uint8_t num);
};

void BlinkerAUTO::run(const String & key, float data, int32_t nowTime)
{
    BLINKER_LOG_ALL(BLINKER_F("BlinkerAUTO run key: "), key, 
                    BLINKER_F(" data: "), data, 
                    BLINKER_F(" nowTime: "), nowTime);
    
    // static uint8_t _num;
    for (uint8_t _num = 0; _num < _targetNum; _num++)
    {
        if (!_autoState) return;

        if (key != STRING_format(_targetKey[_num])) return;

        if (_time1[_num] < _time2[_num])
        {
            if (!(nowTime >= _time1[_num] && nowTime <= _time2[_num]))
            {
                BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
                return;
            }
        }
        else if (_time1[_num] > _time2[_num])
        {
            if (nowTime > _time1[_num] && nowTime < _time2[_num])
            {
                BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
                return;
            }
        }

        if ((_logicType == BLINKER_TYPE_NUMERIC) || \
            (_logicType == BLINKER_TYPE_OR && \
            logic_type[_num] == BLINKER_TYPE_NUMERIC) || \
            (_logicType == BLINKER_TYPE_AND && \
            logic_type[_num] == BLINKER_TYPE_NUMERIC))
        {
            switch (_compareType[_num])
            {
                case BLINKER_COMPARE_LESS:
                    if (data < _targetData[_num])
                    {
                        if (!_isTrigged[_num]) {
                            triggerCheck("less", _num);
                        }
                    }
                    else
                    {
                        _isTrigged[_num] = false;
                        isRecord[_num] = false;
                        _trigged = false;
                    }
                    break;
                case BLINKER_COMPARE_EQUAL:
                    if (data == _targetData[_num])
                    {
                        if (!_isTrigged[_num])
                        {
                            triggerCheck("equal", _num);
                        }
                    }
                    else
                    {
                        _isTrigged[_num] = false;
                        isRecord[_num] = false;
                        _trigged = false;
                    }
                    break;
                case BLINKER_COMPARE_GREATER:
                    if (data > _targetData[_num])
                    {
                        if (!_isTrigged[_num])
                        {
                            triggerCheck("greater", _num);
                        }
                    }
                    else
                    {
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

void BlinkerAUTO::run(const String & key, const String & state, int32_t nowTime)
{
    BLINKER_LOG_ALL(BLINKER_F("BlinkerAUTO run key: "), key, \
                    BLINKER_F(" state: "), state, \
                    BLINKER_F(" nowTime: "), nowTime);

    for (uint8_t _num = 0; _num < _targetNum; _num++)
    {
        if (!_autoState) return;

        if (key != STRING_format(_targetKey[_num])) return;

        if (_time1[_num] < _time2[_num])
        {
            if (!(nowTime >= _time1[_num] && nowTime <= _time2[_num]))
            {
                BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
                return;
            }
        }
        else if (_time1[_num] > _time2[_num])
        {
            if (nowTime > _time1[_num] && nowTime < _time2[_num])
            {
                BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
                return;
            }
        }

        // if (_logicType == BLINKER_TYPE_STATE) {
        if ((_logicType == BLINKER_TYPE_STATE) || \
            (_logicType == BLINKER_TYPE_OR && \
            logic_type[_num] == BLINKER_TYPE_STATE) || \
            (_logicType == BLINKER_TYPE_AND && \
            logic_type[_num] == BLINKER_TYPE_STATE))
        {
            if (state == BLINKER_CMD_ON)
            {
                if (_targetState[_num])
                {
                    if (!_isTrigged[_num])
                    {
                        triggerCheck("on", _num);
                    }
                }
                else
                {
                    _isTrigged[_num] = false;
                    isRecord[_num] = false;
                    _trigged = false;
                }
            }
            else if (state == BLINKER_CMD_OFF)
            {
                if (!_targetState[_num])
                {
                    if (!_isTrigged[_num])
                    {
                        triggerCheck("off", _num);
                    }
                }
                else
                {
                    _isTrigged[_num] = false;
                    isRecord[_num] = false;
                    _trigged = false;
                }
            }
        }
        else if (_logicType == BLINKER_TYPE_OR && \
            logic_type[_num] == BLINKER_TYPE_STATE)
        {
            if (state == BLINKER_CMD_ON)
            {
                if (_targetState[_num])
                {
                    if (!_isTrigged[_num])
                    {
                        triggerCheck("on", _num);
                    }
                }
                else
                {
                    _isTrigged[_num] = false;
                    isRecord[_num] = false;
                    _trigged = false;
                }
            }
            else if (state == BLINKER_CMD_OFF)
            {
                if (!_targetState[_num])
                {
                    if (!_isTrigged[_num])
                    {
                        triggerCheck("off", _num);
                    }
                }
                else
                {
                    _isTrigged[_num] = false;
                    isRecord[_num] = false;
                    _trigged = false;
                }
            }
        }
    }
}

void BlinkerAUTO::manager(const String & data)
{
    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();
    
    _autoState = root[BLINKER_CMD_ENABLE];
    
    _haveAuto = true;

    BLINKER_LOG_ALL(BLINKER_F("==============================================="));
    BLINKER_LOG_ALL(BLINKER_F("auto state: "), _autoState);

    _autoId = root[BLINKER_CMD_ID];

    String logicType = root[BLINKER_CMD_LOGIC];
    BLINKER_LOG_ALL(BLINKER_F("_autoId: "), _autoId);
    BLINKER_LOG_ALL(BLINKER_F("logicType: "), logicType);

    if (logicType == BLINKER_CMD_STATE)
    {
        BLINKER_LOG_ALL(BLINKER_F("state!"));
        
        String target_key = root[BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
        
        strcpy(_targetKey[0], target_key.c_str());
        
        _targetNum = 1;

        _logicType = BLINKER_TYPE_STATE;

        logic_type[0] = BLINKER_TYPE_STATE;

        String target_state = root[BLINKER_CMD_DATA][0][BLINKER_CMD_VALUE];
        
        if (target_state == BLINKER_CMD_ON)
        {
            _targetState[0] = true;
        }
        else if (target_state == BLINKER_CMD_OFF)
        {
            _targetState[0] = false;
        }

        _duration[0] = root[BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
        _duration[0] = 60 * _duration[0];

        BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
        BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0]);
        BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[0]);
        BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[0]);
    }
    else if (logicType == BLINKER_CMD_NUMBERIC)
    {
        BLINKER_LOG_ALL(BLINKER_F("numberic!"));
        
        String target_key = root[BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
        
        strcpy(_targetKey[0], target_key.c_str());
        
        _targetNum = 1;
        
        _logicType = BLINKER_TYPE_NUMERIC;

        logic_type[0] = BLINKER_TYPE_NUMERIC;
        
        String _type = root[BLINKER_CMD_DATA][0][BLINKER_CMD_TYPE];
        
        if (_type == BLINKER_CMD_LESS)
        {
            _compareType[0] = BLINKER_COMPARE_LESS;
        }
        else if (_type == BLINKER_CMD_EQUAL)
        {
            _compareType[0] = BLINKER_COMPARE_EQUAL;
        }
        else if (_type == BLINKER_CMD_GREATER)
        {
            _compareType[0] = BLINKER_COMPARE_GREATER;
        }
        
        _targetData[0] = root[BLINKER_CMD_DATA][0][BLINKER_CMD_VALUE];
        _duration[0] = root[BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
        _duration[0] = 60 * _duration[0];

        BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
        BLINKER_LOG_ALL(BLINKER_F("_type: "), _type, 
                    BLINKER_F(" _compareType: "), _compareType[0]);
        BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0], 
                    BLINKER_F(" _targetData: "), _targetData[0]);
        BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[0]);
    }
    else if (logicType == BLINKER_CMD_OR || logicType == BLINKER_CMD_AND)
    {
        if (logicType == BLINKER_CMD_OR)
        {
            BLINKER_LOG_ALL(BLINKER_F("or!"));
            _logicType = BLINKER_TYPE_OR;
        }
        else if (logicType == BLINKER_CMD_AND)
        {
            BLINKER_LOG_ALL(BLINKER_F("and!"));
            _logicType = BLINKER_TYPE_AND;
        }

        _targetNum = 2;

        for (uint8_t t_num = 0; t_num < _targetNum; t_num++)
        {
            String target_key = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_KEY];

            String compare_type = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_TYPE];

            if (compare_type.length())
            {
                BLINKER_LOG_ALL(BLINKER_F("or/and numberic!"));

                logic_type[t_num] = BLINKER_TYPE_NUMERIC;

                strcpy(_targetKey[t_num], target_key.c_str());
                
                if (compare_type == BLINKER_CMD_LESS)
                {
                    _compareType[t_num] = BLINKER_COMPARE_LESS;
                }
                else if (compare_type == BLINKER_CMD_EQUAL)
                {
                    _compareType[t_num] = BLINKER_COMPARE_EQUAL;
                }
                else if (compare_type == BLINKER_CMD_GREATER)
                {
                    _compareType[t_num] = BLINKER_COMPARE_GREATER;
                }
                
                _targetData[t_num] = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_VALUE];
                _duration[t_num] = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_DURATION];
                _duration[t_num] = 60 * _duration[t_num];
                
                BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
                BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType[t_num]);
                BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[t_num], BLINKER_F(" _targetData: "), _targetData[t_num]);
                BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[t_num]);
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("or/and state!"));
                
                logic_type[t_num] = BLINKER_TYPE_STATE;

                strcpy(_targetKey[t_num], target_key.c_str());
                
                String target_state = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_VALUE];
                
                if (target_state == BLINKER_CMD_ON)
                {
                    _targetState[t_num] = true;
                }
                else if (target_state == BLINKER_CMD_OFF)
                {
                    _targetState[t_num] = false;
                }
                
                _duration[t_num] = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_DURATION];
                _duration[t_num] = 60 * _duration[t_num];
                
                BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
                BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[t_num]);
                BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[t_num]);
                BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[t_num]);
            }
        }
    }

    int32_t timeValue = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                            [BLINKER_CMD_RANGE][0];

    if (timeValue)
    {
        _time1[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                        [BLINKER_CMD_RANGE][0];
        _time1[0] = 60 * _time1[0];
        _time2[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                        [BLINKER_CMD_RANGE][1];
        _time2[0] = 60 * _time2[0];
    }
    else
    {
        _time1[0] = 0;
        _time2[0] = 24 * 60 * 60;
    }

    BLINKER_LOG_ALL(BLINKER_F("_time1: "), _time1[0], \
                    BLINKER_F(" _time2: "), _time2[0]);

    serialization();
}

void BlinkerAUTO::deserialization()
{
    uint8_t checkData;
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

    if (checkData != BLINKER_CHECK_DATA)
    {
        _haveAuto  = false;
        _autoState = false;
        EEPROM.commit();
        EEPROM.end();
        return;
    }
    
    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_AUTOID, _autoId);
    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_TYPESTATE, _typeState);

    _haveAuto  = _typeState >> 7 & 0x01;
    _autoState = _typeState >> 6 & 0x01;
    _logicType = _typeState      & 0x3F;
    
    BLINKER_LOG_ALL(BLINKER_F("_typeState: "), _typeState);
    BLINKER_LOG_ALL(BLINKER_F("_haveAuto: "), _haveAuto);

    if (!_haveAuto)
    {
        EEPROM.commit();
        EEPROM.end();
        return;
    }

    if (_logicType == BLINKER_TYPE_STATE || _logicType == BLINKER_TYPE_NUMERIC)
    {
        _targetNum = 1;
    }
    else
    {
        _targetNum = 2;
    }

    BLINKER_LOG_ALL(BLINKER_F("_targetNum: "), _targetNum);
    
    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
                
    logic_type[0] = _autoData[0] >> 30 & 0x03;
    
    BLINKER_LOG_ALL(BLINKER_F("==============================================="));
    BLINKER_LOG_ALL(BLINKER_F("_autoId: "), _autoId);
    
    switch (_logicType)
    {
        case BLINKER_TYPE_STATE :
            BLINKER_LOG_ALL(BLINKER_F("_logicType: state"));
            break;
        case BLINKER_TYPE_NUMERIC :
            BLINKER_LOG_ALL(BLINKER_F("_logicType: numberic"));
            break;
        case BLINKER_TYPE_OR :
            BLINKER_LOG_ALL(BLINKER_F("_logicType: or"));
            break;
        case BLINKER_TYPE_AND :
            BLINKER_LOG_ALL(BLINKER_F("_logicType: and"));
            break;
        default :
            break;
    }

    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
        
    if (logic_type[0] == BLINKER_TYPE_STATE)
    {
        _targetState[0] = _autoData[0] >> 28 & 0x03;
        
        BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0]);
        BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[0] ? "on" : "off");
    }
    else
    {
        _compareType[0] = _autoData[0] >> 28 & 0x03;
        // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
        EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                    a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                    BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
                    
        BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType[0] ? (_compareType[0] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
        BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0]);
        BLINKER_LOG_ALL(BLINKER_F("_targetData: "), _targetData[0]);
    }

    _duration[0] = (_autoData[0] >> 22 & 0x3f) * 60;
    _time1[0] = (_autoData[0] >> 11 & 0x7ff) * 60;
    _time2[0] = (_autoData[0] & 0x7ff) * 60;
    
    BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[0]);
    BLINKER_LOG_ALL(BLINKER_F("_time1: "), _time1[0], \
                    BLINKER_F(" _time2: "), _time2[0]);

    if (_targetNum == 2)
    {
        EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                    a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                    BLINKER_EEP_ADDR_AUTO2, _autoData[1]);
                    
        logic_type[1] = _autoData[1] >> 30 & 0x03;
        
        EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                    a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                    BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
            
        if (logic_type[1] == BLINKER_TYPE_STATE)
        {
            _targetState[1] = _autoData[1] >> 28 & 0x03;
            
            BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[1]);
            BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[1] ? "on" : "off");
        }
        else
        {
            _compareType[1] = _autoData[1] >> 28 & 0x03;
            // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
            EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                        a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                        BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
                        
            BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType[1] ? (_compareType[1] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
            BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[1]);
            BLINKER_LOG_ALL(BLINKER_F("_targetData: "), _targetData[1]);
        }

        _duration[1] = (_autoData[1] >> 22 & 0x3f) * 60;
        _time1[1] = (_autoData[1] >> 11 & 0x7ff) * 60;
        _time2[1] = (_autoData[1] & 0x7ff) * 60;
        
        BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[1]);
        BLINKER_LOG_ALL(BLINKER_F("_time1: "), _time1[1], BLINKER_F(" _time2: "), _time2[1]);
    }
    
    BLINKER_LOG_ALL(BLINKER_F("==============================================="));

    EEPROM.commit();
    EEPROM.end();
    // }
}

void BlinkerAUTO::serialization()
{
    uint8_t checkData;
    
    _typeState = _haveAuto << 7 | _autoState << 6 | _logicType;// | _linkNum;

    EEPROM.begin(BLINKER_EEP_SIZE);

    EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

    if (checkData != BLINKER_CHECK_DATA)
    {
        EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
    }

    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_AUTOID, _autoId);
    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_TYPESTATE, _typeState);
                
    _autoData[0] = logic_type[0] << 30 ;
    if (logic_type[0] == BLINKER_TYPE_STATE)
    {
        _autoData[0] |= _targetState[0] << 28;
    }
    else
    {
        _autoData[0] |= _compareType[0] << 28;
    }
    _autoData[0] |= _duration[0] / 60 << 22 | _time1[0] / 60 << 11 | _time2[0] / 60;

    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
        
    if (logic_type[0] == BLINKER_TYPE_NUMERIC)
    {
        EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
    }
    
    BLINKER_LOG_ALL(BLINKER_F("serialization _typeState: "), _typeState);
    BLINKER_LOG_ALL(BLINKER_F("serialization _autoData: "), _autoData[0]);
    
    if (_targetNum == 2)
    {
        _autoData[1] = logic_type[1] << 30 ;
        if (logic_type[1] == BLINKER_TYPE_STATE)
        {
            _autoData[1] |= _targetState[1] << 28;
        }
        else
        {
            _autoData[1] |= _compareType[1] << 28;
        }
        _autoData[1] |= _duration[1] / 60 << 22 | _time1[1] / 60 << 11 | _time2[1] / 60;
    
        EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                    a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                    BLINKER_EEP_ADDR_AUTO2, _autoData[1]);
        EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                    a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                    BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
            
        if (logic_type[1] == BLINKER_TYPE_NUMERIC)
        {
            EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                    a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                    BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
        }
        BLINKER_LOG_ALL(BLINKER_F("serialization _autoData: "), _autoData[1]);
    }

    EEPROM.commit();
    EEPROM.end();
}

void BlinkerAUTO::fresh()
{
    for (uint8_t _num = 0; _num < _targetNum; _num++)
    {
        if (isRecord[_num]) {
            _isTrigged[_num] = true;
        }
    }
    _trigged = false;
}

void BlinkerAUTO::triggerCheck(const String & state, uint8_t num)
{
    if (!isRecord[num])
    {
        isRecord[num] = true;
        _treTime[num] = millis();
    }
    
    switch (_logicType)
    {
        case BLINKER_TYPE_STATE :
            BLINKER_LOG_ALL(("_logicType: state"));
            // state = BLINKER_F("state ") + state;
            break;
        case BLINKER_TYPE_NUMERIC :
            BLINKER_LOG_ALL(("_logicType: numberic"));
            // state = BLINKER_F("numberic ") + state;
            break;
        case BLINKER_TYPE_OR :
            BLINKER_LOG_ALL(("_logicType: or"));
            // state = BLINKER_F("or ") + state;
            break;
        case BLINKER_TYPE_AND :
            BLINKER_LOG_ALL(("_logicType: and"));
            // state = BLINKER_F("and ") + state;
            break;
        default :
            break;
    }

    if ((millis() - _treTime[num]) / 1000 >= _duration[num])
    {
        if (_logicType != BLINKER_TYPE_AND)
        {
            BLINKER_LOG_ALL(state, BLINKER_F(" trigged"));
            _trigged = true;
        }
        else
        {
            _isTrigged[num] = true;
            if (_isTrigged[0] && _isTrigged[1])
            {
                BLINKER_LOG_ALL(state, BLINKER_F(" trigged"));
                _trigged = true;
            }
        }
    }
}

#endif

#endif
