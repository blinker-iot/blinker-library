#ifndef BLINKER_AUTO_H
#define BLINKER_AUTO_H

#if ((defined(ESP8266) || defined(ESP32)) && \
    (defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
    defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
    defined(BLINKER_NBIOT_SIM7020) || defined(BLINKER_PRO_SIM7020) || \
    defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
    defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_SUBDEVICE) || \
    defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
    defined(BLINKER_QRCODE_NBIOT_SIM7000) || defined(BLINKE_HTTP)))

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
        void manager(const JsonObject& data);
        void deserialization();
        void serialization();
        void setNum(uint8_t num) { a_num = num; }
        bool isTrigged() 
        { 
            return _trigged;
            // if (_trigged)
            // {
            //     if (millis() - _last_tri_time >= 6000 || _last_tri_time == 0)
            //     {
            //         _last_tri_time = millis();
            //         return true;
            //     }
            // }
            // return false;
        }
        void fresh();
        unsigned long id() { return _autoId; }

    private :
        uint8_t     a_num;
        time_t      _last_tri_time;
        
        // {
        //     "auto":{
        //         "enable":true,//_autoState
        //         "id":123456,//_autoId
        //         "mode":"or",//_logicType
        //         "time": {
        //             "day": "1111111",
        //             "range": [540, 1260],
        //         },
        //         "triggers":
        //         [
        //             {
        //                 "source":"humi",
        //                 "operator":"<",//_targetState|_compareType
        //                 "value":40,
        //                 "duration":10
        //             }
        //         ]
        //     }
        // }

        // - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -
        // | | | |     |                         | _time1 0-1440min 11  | _time2 0-1440min 11                   
        // | | | |     | _duration 0-60min 6
        // | | | | _targetState|_compareType on/off|less/equal/greater 2
        // | | | _targetState|_compareType on/off|less/equal/greater
        // |  
        // | logic_type state/numberic 2
        // |
        // autoData

        // - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -  - - - - - - - -
        // | | | |   |              |                         | _time1 0-1440min 11  | _time2 0-1440min 11
        // | | | |   |              | _duration 0-3600s 12
        // | | | |   |day 7
        // | | | | _targetState|_compareType less/equal/greater 2
        // | | |_logicType and/or 1
        // | | _autoState true/false 1    
        // | _haveAuto 1
        // | 
        // autoData
        // typestatexxxxxx
        bool        _haveAuto;
        bool        _autoState;
        unsigned long    _autoId = 0;
        uint8_t     _logicType;
        uint8_t     _compareType;
        uint8_t     _day = 0;
        // uint8_t     _autoData[6];

        uint16_t    _start_time;
        uint16_t    _end_time;
        uint16_t    _duration;

        // uint8_t     logic_type[2];
        char        _targetKey[12];
        float       _targetState;
        // float       _targetData[2];
        // uint8_t     _compareType[2];
        // bool        _targetState[2];
        // uint32_t    _time1[2];
        // uint32_t    _time2[2];
        // uint32_t    _duration[2];
        uint32_t    _treTime;
        bool        isRecord;
        bool        _isTrigged;
        bool        _trigged;

        void triggerCheck(const String & state);
};

void BlinkerAUTO::run(const String & key, float data, int32_t nowTime)
{
    BLINKER_LOG_ALL(BLINKER_F("BlinkerAUTO run key: "), key,
                    BLINKER_F(" ,data: "), data, 
                    BLINKER_F(" ,nowTime: "), nowTime,
                    BLINKER_F(" ,autoId: "), _autoId);
    
    // static uint8_t _num;
    // for (uint8_t _num = 0; _num < _targetNum; _num++)
    // {
        if (!_autoState) return;

        if (key != STRING_format(_targetKey)) return;

        if (_start_time < _end_time)
        {
            if (!(nowTime >= _start_time && nowTime <= _end_time))
            {
                BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
                return;
            }
        }
        else if (_start_time > _end_time)
        {
            if (nowTime > _start_time && nowTime < _end_time)
            {
                BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
                return;
            }
        }

        if (_logicType == BLINKER_TYPE_OR || _logicType == BLINKER_TYPE_AND)
        {
            switch (_compareType)
            {
                case BLINKER_COMPARE_LESS:
                    if (data < _targetState)
                    {
                        if (!_isTrigged) {
                            triggerCheck("less");
                        }
                    }
                    else
                    {
                        _isTrigged = false;
                        isRecord = false;
                        _trigged = false;
                    }
                    break;
                case BLINKER_COMPARE_EQUAL:
                    if (data == _targetState)
                    {
                        if (!_isTrigged)
                        {
                            triggerCheck("equal");
                        }
                    }
                    else
                    {
                        _isTrigged = false;
                        isRecord = false;
                        _trigged = false;
                    }
                    break;
                case BLINKER_COMPARE_GREATER:
                    if (data > _targetState)
                    {
                        if (!_isTrigged)
                        {
                            triggerCheck("greater");
                        }
                    }
                    else
                    {
                        _isTrigged = false;
                        isRecord = false;
                        _trigged = false;
                    }
                    break;
                default:
                    break;
            }
        }
    // }
}

// void BlinkerAUTO::run(const String & key, const String & state, int32_t nowTime)
// {
//     BLINKER_LOG_ALL(BLINKER_F("BlinkerAUTO run key: "), key, \
//                     BLINKER_F(" state: "), state, \
//                     BLINKER_F(" nowTime: "), nowTime);

//     // for (uint8_t _num = 0; _num < _targetNum; _num++)
//     // {
//         if (!_autoState) return;

//         if (key != STRING_format(_targetKey)) return;

//         if (_time1 < _time2)
//         {
//             if (!(nowTime >= _time1 && nowTime <= _time2))
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
//                 return;
//             }
//         }
//         else if (_time1 > _time2)
//         {
//             if (nowTime > _time1 && nowTime < _time2)
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("out of time slot: "), nowTime);
//                 return;
//             }
//         }

//         if (_logicType == BLINKER_TYPE_OR || _logicType == BLINKER_TYPE_AND)
//         {
//             if (state == BLINKER_CMD_ON)
//             {
//                 if (_targetState)
//                 {
//                     if (!_isTrigged)
//                     {
//                         triggerCheck("on");
//                     }
//                 }
//                 else
//                 {
//                     _isTrigged = false;
//                     isRecord = false;
//                     _trigged = false;
//                 }
//             }
//             else if (state == BLINKER_CMD_OFF)
//             {
//                 if (!_targetState)
//                 {
//                     if (!_isTrigged)
//                     {
//                         triggerCheck("off");
//                     }
//                 }
//                 else
//                 {
//                     _isTrigged = false;
//                     isRecord = false;
//                     _trigged = false;
//                 }
//             }
//         }
//     //     else if (_logicType == BLINKER_TYPE_OR && \
//     //         logic_type[_num] == BLINKER_TYPE_STATE)
//     //     {
//     //         if (state == BLINKER_CMD_ON)
//     //         {
//     //             if (_targetState[_num])
//     //             {
//     //                 if (!_isTrigged[_num])
//     //                 {
//     //                     triggerCheck("on", _num);
//     //                 }
//     //             }
//     //             else
//     //             {
//     //                 _isTrigged[_num] = false;
//     //                 isRecord[_num] = false;
//     //                 _trigged = false;
//     //             }
//     //         }
//     //         else if (state == BLINKER_CMD_OFF)
//     //         {
//     //             if (!_targetState[_num])
//     //             {
//     //                 if (!_isTrigged[_num])
//     //                 {
//     //                     triggerCheck("off", _num);
//     //                 }
//     //             }
//     //             else
//     //             {
//     //                 _isTrigged[_num] = false;
//     //                 isRecord[_num] = false;
//     //                 _trigged = false;
//     //             }
//     //         }
//     //     }
//     // }
// }

void BlinkerAUTO::manager(const JsonObject& root)
{
    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    // BLINKER_LOG_ALL(BLINKER_F("auto state: "), data);

    // DynamicJsonDocument jsonBuffer(1024);
    // deserializeJson(jsonBuffer, data);
    // JsonObject root = jsonBuffer.as<JsonObject>();
    
    _autoState = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]["enable"].as<bool>();
    
    _haveAuto = true;

    BLINKER_LOG_ALL(BLINKER_F("==============================================="));
    BLINKER_LOG_ALL(BLINKER_F("auto state: "), _autoState);

    String get_autoId = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_ID].as<String>();
    _autoId = strtoul(get_autoId.c_str(),NULL,10);

    String logicType = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_MODE].as<String>();
    BLINKER_LOG_ALL(BLINKER_F("_autoId: "), get_autoId, " ", String(_autoId));
    BLINKER_LOG_ALL(BLINKER_F("logicType: "), logicType);

    // if (logicType == BLINKER_CMD_STATE)
    // {
    //     BLINKER_LOG_ALL(BLINKER_F("state!"));
        
    //     String target_key = root[BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
        
    //     strcpy(_targetKey[0], target_key.c_str());
        
    //     _targetNum = 1;

    //     _logicType = BLINKER_TYPE_STATE;

    //     logic_type[0] = BLINKER_TYPE_STATE;

    //     String target_state = root[BLINKER_CMD_DATA][0][BLINKER_CMD_VALUE];
        
    //     if (target_state == BLINKER_CMD_ON)
    //     {
    //         _targetState[0] = true;
    //     }
    //     else if (target_state == BLINKER_CMD_OFF)
    //     {
    //         _targetState[0] = false;
    //     }

    //     _duration[0] = root[BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
    //     _duration[0] = 60 * _duration[0];

    //     BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
    //     BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0]);
    //     BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[0]);
    //     BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[0]);
    // }
    // else if (logicType == BLINKER_CMD_NUMBERIC)
    // {
    //     BLINKER_LOG_ALL(BLINKER_F("numberic!"));
        
    //     String target_key = root[BLINKER_CMD_DATA][0][BLINKER_CMD_KEY];
        
    //     strcpy(_targetKey[0], target_key.c_str());
        
    //     _targetNum = 1;
        
    //     _logicType = BLINKER_TYPE_NUMERIC;

    //     logic_type[0] = BLINKER_TYPE_NUMERIC;
        
    //     String _type = root[BLINKER_CMD_DATA][0][BLINKER_CMD_TYPE];
        
    //     if (_type == BLINKER_CMD_LESS)
    //     {
    //         _compareType[0] = BLINKER_COMPARE_LESS;
    //     }
    //     else if (_type == BLINKER_CMD_EQUAL)
    //     {
    //         _compareType[0] = BLINKER_COMPARE_EQUAL;
    //     }
    //     else if (_type == BLINKER_CMD_GREATER)
    //     {
    //         _compareType[0] = BLINKER_COMPARE_GREATER;
    //     }
        
    //     _targetData[0] = root[BLINKER_CMD_DATA][0][BLINKER_CMD_VALUE];
    //     _duration[0] = root[BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION];
    //     _duration[0] = 60 * _duration[0];

    //     BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
    //     BLINKER_LOG_ALL(BLINKER_F("_type: "), _type, 
    //                 BLINKER_F(" _compareType: "), _compareType[0]);
    //     BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0], 
    //                 BLINKER_F(" _targetData: "), _targetData[0]);
    //     BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[0]);
    // }
    // else 
    // if (logicType == BLINKER_CMD_OR || logicType == BLINKER_CMD_AND)
    // {
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

        String target_key = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_TRIGGER][0][BLINKER_CMD_SOURCE];
        String target_State = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_TRIGGER][0]["value"];
        String compare_type = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_TRIGGER][0][BLINKER_CMD_OPERATOR];
        String day_set = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]["time"][BLINKER_CMD_DAY];
        _start_time = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]["time"][BLINKER_CMD_RANGE][0].as<uint16_t>();
        _end_time = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]["time"][BLINKER_CMD_RANGE][1].as<uint16_t>();
        _duration = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DATA][0][BLINKER_CMD_DURATION].as<uint16_t>();

        for (uint8_t day = 0; day < 7; day++)
        {
            if (day_set.substring(day, day+1) == "1")
            {
                _day |= (0x01 << day);

                BLINKER_LOG_ALL(BLINKER_F("day: "), day, BLINKER_F(" _day: "), _day);
            }
        }

        BLINKER_LOG_ALL(BLINKER_F("target_key: "), target_key);
        BLINKER_LOG_ALL(BLINKER_F("target_State: "), target_State);
        BLINKER_LOG_ALL(BLINKER_F("compare_type: "), compare_type);
        BLINKER_LOG_ALL(BLINKER_F("day_set: "), day_set);
        BLINKER_LOG_ALL(BLINKER_F("_start_time: "), _start_time);
        BLINKER_LOG_ALL(BLINKER_F("_end_time: "), _end_time);

        strcpy(_targetKey, target_key.c_str());
        if (compare_type == BLINKER_CMD_LESS)
        {
            _compareType = BLINKER_COMPARE_LESS;
        }
        else if (compare_type == BLINKER_CMD_EQUAL)
        {
            _compareType = BLINKER_COMPARE_EQUAL;
        }
        else if (compare_type == BLINKER_CMD_GREATER)
        {
            _compareType = BLINKER_COMPARE_GREATER;
        }

        if (target_State == BLINKER_CMD_ON)
        {
            _targetState = 1;
        }
        else if (target_State == BLINKER_CMD_OFF)
        {
            _targetState = 0;
        }
        else {
            _targetState = target_State.toFloat();
        }

        // _targetNum = 2;

        // for (uint8_t t_num = 0; t_num < _targetNum; t_num++)
        // {
        //     String target_key = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_KEY];

        //     String compare_type = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_TYPE];

        //     if (compare_type.length())
        //     {
        //         BLINKER_LOG_ALL(BLINKER_F("or/and numberic!"));

        //         logic_type[t_num] = BLINKER_TYPE_NUMERIC;

        //         strcpy(_targetKey[t_num], target_key.c_str());
                
        //         if (compare_type == BLINKER_CMD_LESS)
        //         {
        //             _compareType[t_num] = BLINKER_COMPARE_LESS;
        //         }
        //         else if (compare_type == BLINKER_CMD_EQUAL)
        //         {
        //             _compareType[t_num] = BLINKER_COMPARE_EQUAL;
        //         }
        //         else if (compare_type == BLINKER_CMD_GREATER)
        //         {
        //             _compareType[t_num] = BLINKER_COMPARE_GREATER;
        //         }
                
        //         _targetData[t_num] = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_VALUE];
        //         _duration[t_num] = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_DURATION];
        //         _duration[t_num] = 60 * _duration[t_num];
                
        //         BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
        //         BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType[t_num]);
        //         BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[t_num], BLINKER_F(" _targetData: "), _targetData[t_num]);
        //         BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[t_num]);
        //     }
        //     else
        //     {
        //         BLINKER_LOG_ALL(BLINKER_F("or/and state!"));
                
        //         logic_type[t_num] = BLINKER_TYPE_STATE;

        //         strcpy(_targetKey[t_num], target_key.c_str());
                
        //         String target_state = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_VALUE];
                
        //         if (target_state == BLINKER_CMD_ON)
        //         {
        //             _targetState[t_num] = true;
        //         }
        //         else if (target_state == BLINKER_CMD_OFF)
        //         {
        //             _targetState[t_num] = false;
        //         }
                
        //         _duration[t_num] = root[BLINKER_CMD_DATA][t_num][BLINKER_CMD_DURATION];
        //         _duration[t_num] = 60 * _duration[t_num];
                
        //         BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
        //         BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[t_num]);
        //         BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[t_num]);
        //         BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[t_num]);
        //     }
        // }
    // }

    // int32_t timeValue = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
    //                         [BLINKER_CMD_RANGE][0];

    // if (timeValue)
    // {
    //     _time1[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
    //                     [BLINKER_CMD_RANGE][0];
    //     _time1[0] = 60 * _time1[0];
    //     _time2[0] = root[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
    //                     [BLINKER_CMD_RANGE][1];
    //     _time2[0] = 60 * _time2[0];
    // }
    // else
    // {
    //     _time1[0] = 0;
    //     _time2[0] = 24 * 60 * 60;
    // }

    // BLINKER_LOG_ALL(BLINKER_F("_time1: "), _time1[0], \
    //                 BLINKER_F(" _time2: "), _time2[0]);

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

    uint64_t auto_data;
    
    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE +
                BLINKER_EEP_ADDR_AUTODATA, auto_data);

    // EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_SOURCE, _targetKey);
    
    // EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE +
    //             BLINKER_EEP_ADDR_VALUE, _targetState);

    // EEPROM.commit();
    // EEPROM.end();

    _haveAuto = auto_data >> (11 + 11 + 12 + 7 + 2 + 1 + 1) & 0x01;
    _autoState = auto_data >> (11 + 11 + 12 + 7 + 2 + 1) & 0x01;
    _logicType = auto_data >> (11 + 11 + 12 + 7 + 2) & 0x01;
    _compareType = auto_data >> (11 + 11 + 12 + 7) & 0x03;

    BLINKER_LOG_ALL(BLINKER_F("_haveAuto: "), _haveAuto);
    BLINKER_LOG_ALL(BLINKER_F("_autoState: "), _autoState);
    BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
    BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType);

    if (!_haveAuto)
    {
        EEPROM.commit();
        EEPROM.end();
        return;
    }

    _day = auto_data >> (11 + 11 + 12) & 0x7F;
    _duration = auto_data >> (11 + 11) & 0xFFF;
    _start_time = auto_data >> (11) & 0x7FF;
    _end_time = auto_data & 0x7FF;    

    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_AUTOID, _autoId);

    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_SOURCE, _targetKey);
    
    EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE +
                BLINKER_EEP_ADDR_VALUE, _targetState);

    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("==============================================="));
    BLINKER_LOG_ALL(BLINKER_F("_autoId: "), String(_autoId));
    BLINKER_LOG_ALL(BLINKER_F("_day: "), _day);
    BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration);
    BLINKER_LOG_ALL(BLINKER_F("_start_time: "), _start_time);
    BLINKER_LOG_ALL(BLINKER_F("_end_time: "), _end_time);
    BLINKER_LOG_ALL(BLINKER_F("_logicType: "), _logicType);
    BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType);
    BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey);
    BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState);
    BLINKER_LOG_ALL(BLINKER_F("==============================================="));
        
    // EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_AUTOID, _autoId);
    // EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_TYPESTATE, _typeState);

    // _haveAuto  = _typeState >> 7 & 0x01;
    // _autoState = _typeState >> 6 & 0x01;
    // _logicType = _typeState      & 0x3F;
    
    // BLINKER_LOG_ALL(BLINKER_F("_typeState: "), _typeState);
    // BLINKER_LOG_ALL(BLINKER_F("_haveAuto: "), _haveAuto);

    // if (!_haveAuto)
    // {
    //     EEPROM.commit();
    //     EEPROM.end();
    //     return;
    // }

    // if (_logicType == BLINKER_TYPE_STATE || _logicType == BLINKER_TYPE_NUMERIC)
    // {
    //     _targetNum = 1;
    // }
    // else
    // {
    //     _targetNum = 2;
    // }

    // BLINKER_LOG_ALL(BLINKER_F("_targetNum: "), _targetNum);
    
    // EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
                
    // logic_type[0] = _autoData[0] >> 30 & 0x03;
    
    // BLINKER_LOG_ALL(BLINKER_F("==============================================="));
    // BLINKER_LOG_ALL(BLINKER_F("_autoId: "), _autoId);
    
    // switch (_logicType)
    // {
    //     case BLINKER_TYPE_STATE :
    //         BLINKER_LOG_ALL(BLINKER_F("_logicType: state"));
    //         break;
    //     case BLINKER_TYPE_NUMERIC :
    //         BLINKER_LOG_ALL(BLINKER_F("_logicType: numberic"));
    //         break;
    //     case BLINKER_TYPE_OR :
    //         BLINKER_LOG_ALL(BLINKER_F("_logicType: or"));
    //         break;
    //     case BLINKER_TYPE_AND :
    //         BLINKER_LOG_ALL(BLINKER_F("_logicType: and"));
    //         break;
    //     default :
    //         break;
    // }

    // EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
        
    // if (logic_type[0] == BLINKER_TYPE_STATE)
    // {
    //     _targetState[0] = _autoData[0] >> 28 & 0x03;
        
    //     BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0]);
    //     BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[0] ? "on" : "off");
    // }
    // else
    // {
    //     _compareType[0] = _autoData[0] >> 28 & 0x03;
    //     // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
    //     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //                 a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                 BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
                    
    //     BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType[0] ? (_compareType[0] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
    //     BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[0]);
    //     BLINKER_LOG_ALL(BLINKER_F("_targetData: "), _targetData[0]);
    // }

    // _duration[0] = (_autoData[0] >> 22 & 0x3f) * 60;
    // _time1[0] = (_autoData[0] >> 11 & 0x7ff) * 60;
    // _time2[0] = (_autoData[0] & 0x7ff) * 60;
    
    // BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[0]);
    // BLINKER_LOG_ALL(BLINKER_F("_time1: "), _time1[0], \
    //                 BLINKER_F(" _time2: "), _time2[0]);

    // if (_targetNum == 2)
    // {
    //     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //                 a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                 BLINKER_EEP_ADDR_AUTO2, _autoData[1]);
                    
    //     logic_type[1] = _autoData[1] >> 30 & 0x03;
        
    //     EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //                 a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                 BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
            
    //     if (logic_type[1] == BLINKER_TYPE_STATE)
    //     {
    //         _targetState[1] = _autoData[1] >> 28 & 0x03;
            
    //         BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[1]);
    //         BLINKER_LOG_ALL(BLINKER_F("_targetState: "), _targetState[1] ? "on" : "off");
    //     }
    //     else
    //     {
    //         _compareType[1] = _autoData[1] >> 28 & 0x03;
    //         // EEPROM.get(BLINKER_EEP_ADDR_TARGETDATA, _targetData);
    //         EEPROM.get(BLINKER_EEP_ADDR_AUTO_START + 
    //                     a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                     BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
                        
    //         BLINKER_LOG_ALL(BLINKER_F("_compareType: "), _compareType[1] ? (_compareType[1] == BLINKER_COMPARE_GREATER ? "greater" : "equal") : "less");
    //         BLINKER_LOG_ALL(BLINKER_F("_targetKey: "), _targetKey[1]);
    //         BLINKER_LOG_ALL(BLINKER_F("_targetData: "), _targetData[1]);
    //     }

    //     _duration[1] = (_autoData[1] >> 22 & 0x3f) * 60;
    //     _time1[1] = (_autoData[1] >> 11 & 0x7ff) * 60;
    //     _time2[1] = (_autoData[1] & 0x7ff) * 60;
        
    //     BLINKER_LOG_ALL(BLINKER_F("_duration: "), _duration[1]);
    //     BLINKER_LOG_ALL(BLINKER_F("_time1: "), _time1[1], BLINKER_F(" _time2: "), _time2[1]);
    // }
    
    // BLINKER_LOG_ALL(BLINKER_F("==============================================="));

    // EEPROM.commit();
    // EEPROM.end();
    // }
}

void BlinkerAUTO::serialization()
{
    uint64_t auto_data;

    auto_data = _haveAuto & 0x01;
    auto_data = auto_data << 1 | _autoState & 0x01;    
    auto_data = auto_data << 1 | _logicType & 0x01;
    auto_data = auto_data << 2 | _compareType & 0x03;
    auto_data = auto_data << 7 | _day & 0x7F;
    auto_data = auto_data << 12 | _duration & 0xFFF;
    auto_data = auto_data << 11 | _start_time & 0x7FF;
    auto_data = auto_data << 11 | _end_time & 0x7FF;

    uint8_t checkData;

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
                BLINKER_EEP_ADDR_AUTODATA, auto_data);

    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
                BLINKER_EEP_ADDR_SOURCE, _targetKey);
    
    EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
                a_num * BLINKER_ONE_AUTO_DATA_SIZE +
                BLINKER_EEP_ADDR_VALUE, _targetState);

    EEPROM.commit();
    EEPROM.end();

    // uint8_t checkData;
    
    // _typeState = _haveAuto << 7 | _autoState << 6 | _logicType;// | _linkNum;

    // EEPROM.begin(BLINKER_EEP_SIZE);

    // EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

    // if (checkData != BLINKER_CHECK_DATA)
    // {
    //     EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
    // }

    // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_AUTOID, _autoId);
    // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_TYPESTATE, _typeState);
                
    // _autoData[0] = logic_type[0] << 30 ;
    // if (logic_type[0] == BLINKER_TYPE_STATE)
    // {
    //     _autoData[0] |= _targetState[0] << 28;
    // }
    // else
    // {
    //     _autoData[0] |= _compareType[0] << 28;
    // }
    // _autoData[0] |= _duration[0] / 60 << 22 | _time1[0] / 60 << 11 | _time2[0] / 60;

    // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_AUTO1, _autoData[0]);
    // EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_TARGETKEY1, _targetKey[0]);
        
    // if (logic_type[0] == BLINKER_TYPE_NUMERIC)
    // {
    //     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //             a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //             BLINKER_EEP_ADDR_TARGETDATA1, _targetData[0]);
    // }
    
    // BLINKER_LOG_ALL(BLINKER_F("serialization _typeState: "), _typeState);
    // BLINKER_LOG_ALL(BLINKER_F("serialization _autoData: "), _autoData[0]);
    
    // if (_targetNum == 2)
    // {
    //     _autoData[1] = logic_type[1] << 30 ;
    //     if (logic_type[1] == BLINKER_TYPE_STATE)
    //     {
    //         _autoData[1] |= _targetState[1] << 28;
    //     }
    //     else
    //     {
    //         _autoData[1] |= _compareType[1] << 28;
    //     }
    //     _autoData[1] |= _duration[1] / 60 << 22 | _time1[1] / 60 << 11 | _time2[1] / 60;
    
    //     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //                 a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                 BLINKER_EEP_ADDR_AUTO2, _autoData[1]);
    //     EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //                 a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                 BLINKER_EEP_ADDR_TARGETKEY2, _targetKey[1]);
            
    //     if (logic_type[1] == BLINKER_TYPE_NUMERIC)
    //     {
    //         EEPROM.put(BLINKER_EEP_ADDR_AUTO_START + 
    //                 a_num * BLINKER_ONE_AUTO_DATA_SIZE + 
    //                 BLINKER_EEP_ADDR_TARGETDATA2, _targetData[1]);
    //     }
    //     BLINKER_LOG_ALL(BLINKER_F("serialization _autoData: "), _autoData[1]);
    // }

    // EEPROM.commit();
    // EEPROM.end();
}

void BlinkerAUTO::fresh()
{
    // for (uint8_t _num = 0; _num < _targetNum; _num++)
    // {
    //     if (isRecord[_num]) {
    //         _isTrigged[_num] = true;
    //     }
    // }
    // _trigged = false;

    if (isRecord) _isTrigged = true;

    _trigged = false;
}

void BlinkerAUTO::triggerCheck(const String & state)
{
    if (!isRecord)
    {
        isRecord = true;
        _treTime = millis();
    }

    switch (_logicType)
    {
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

    if ((millis() - _treTime) / 1000 >= _duration)
    {
        if (_logicType != BLINKER_TYPE_AND)
        {
            BLINKER_LOG_ALL(state, BLINKER_F(" trigged"));
            _trigged = true;
        }
        else
        {
            _isTrigged = true;
            if (_isTrigged)
            {
                BLINKER_LOG_ALL(state, BLINKER_F(" trigged"));
                _trigged = true;
            }
        }
    }

    // if (!isRecord[num])
    // {
    //     isRecord[num] = true;
    //     _treTime[num] = millis();
    // }
    
    // switch (_logicType)
    // {
    //     case BLINKER_TYPE_STATE :
    //         BLINKER_LOG_ALL(("_logicType: state"));
    //         // state = BLINKER_F("state ") + state;
    //         break;
    //     case BLINKER_TYPE_NUMERIC :
    //         BLINKER_LOG_ALL(("_logicType: numberic"));
    //         // state = BLINKER_F("numberic ") + state;
    //         break;
    //     case BLINKER_TYPE_OR :
    //         BLINKER_LOG_ALL(("_logicType: or"));
    //         // state = BLINKER_F("or ") + state;
    //         break;
    //     case BLINKER_TYPE_AND :
    //         BLINKER_LOG_ALL(("_logicType: and"));
    //         // state = BLINKER_F("and ") + state;
    //         break;
    //     default :
    //         break;
    // }

    // if ((millis() - _treTime[num]) / 1000 >= _duration[num])
    // {
    //     if (_logicType != BLINKER_TYPE_AND)
    //     {
    //         BLINKER_LOG_ALL(state, BLINKER_F(" trigged"));
    //         _trigged = true;
    //     }
    //     else
    //     {
    //         _isTrigged[num] = true;
    //         if (_isTrigged[0] && _isTrigged[1])
    //         {
    //             BLINKER_LOG_ALL(state, BLINKER_F(" trigged"));
    //             _trigged = true;
    //         }
    //     }
    // }
}

#endif

#endif
