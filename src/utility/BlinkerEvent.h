#ifndef BLINKER_EVENT_H
#define BLINKER_EVENT_H

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
#include "modules/ArduinoJson/ArduinoJson.h"
#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerEVENT
{
    public :
        BlinkerEVENT()
            : _haveEvent(false)
            , _eventState(false)
        {}

        void manager(String data) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(data);

            _eventState = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                            [BLINKER_CMD_RUN];

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("==============================================="));
            BLINKER_LOG2(BLINKER_F("event state: "), _eventState);
#endif

            String eventType = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                [BLINKER_CMD_COMMAND][BLINKER_CMD_LOGIC];

            String dataType_0 = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                [BLINKER_CMD_TYPE];

            if (evetType.length() == 0) {
                if (dataType_0 == BLINKER_CMD_ON ||
                    dataType_0 == BLINKER_CMD_OFF) {
                    eventType = BLINKER_CMD_STATE;
                    // _eventType = BLINKER_TYPE_STATE;
                }
                else if (dataType_0 == BLINKER_CMD_LESS ||
                    dataType_0 == BLINKER_CMD_EQUAL ||
                    dataType_0 == BLINKER_CMD_GREATER) {
                    eventType = BLINKER_CMD_NUMBERIC;
                    // _eventType = BLINKER_TYPE_NUMERIC;
                }
            }

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("eventType: "), eventType);
#endif

            if (eventType == BLINKER_CMD_STATE) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("state!"));
#endif
                _dataNum = 1;

                String targetKey = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                        [BLINKER_CMD_KEY];

                String targetValue = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                        [BLINKER_CMD_VALUE];

                String targetType = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                        [BLINKER_CMD_TYPE];

                _eventType = BLINKER_TYPE_STATE;
                _dataType[0] = BLINKER_TYPE_STATE;
                strcpy(_targetKey[0], targetKey.c_str());

                if (targetType == BLINKERCMD_ON) _targetType[0] = 1;
                else if (targetType == BLINKERCMD_OFF) _targetType[0] = 0;

                if (targetValue == BLINKERCMD_ON) _targetValue[0] = 1;
                else if (targetValue == BLINKERCMD_OFF) _targetValue[0] = 0;

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_dataType[0]: "), _dataType[0]);
                BLINKER_LOG2(BLINKER_F("_targetKey[0]: "), _targetKey[0]);
                BLINKER_LOG2(BLINKER_F("_targetValue[0]: "), _targetValue[0]);
                BLINKER_LOG2(BLINKER_F("_targetType[0]: "), _targetType[0]);
                // BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
#endif
            }
            else if (eventType == BLINKER_CMD_NUMBERIC) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("numberic!"));
#endif        
                _dataNum = 1;

                String targetKey = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                        [BLINKER_CMD_KEY];

                _targetValue[0] = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                        [BLINKER_CMD_VALUE];

                String targetType = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                                        [BLINKER_CMD_TYPE];

                _eventType = BLINKER_TYPE_NUMERIC;
                _dataType[0] = BLINKER_TYPE_NUMERIC;
                strcpy(_targetKey[0], targetKey.c_str());

                if (targetType == BLINKER_CMD_LESS) _targetType[0] = BLINKER_COMPARE_LESS;
                else if (targetType == BLINKER_CMD_EQUAL) _targetType[0] = BLINKER_COMPARE_EQUAL;
                else if (targetType == BLINKER_CMD_GREATER) _targetType[0] = BLINKER_COMPARE_GREATER;

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_dataType[0]: "), _dataType[0]);
                BLINKER_LOG2(BLINKER_F("_targetKey[0]: "), _targetKey[0]);
                BLINKER_LOG2(BLINKER_F("_targetValue[0]: "), _targetValue[0]);
                BLINKER_LOG2(BLINKER_F("_targetType[0]: "), _targetType[0]);
                // BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
#endif
            }
            else if (eventType == BLINKER_CMD_OR || eventType == BLINKER_CMD_AND) {
                if (eventType == BLINKER_CMD_OR) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("or!"));
#endif
                    _eventType = BLINKER_TYPE_OR;
                }
                else if (eventType == BLINKER_CMD_AND) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("and!"));
#endif
                    _eventType = BLINKER_TYPE_AND;
                }
                _dataNum = 2;

                for (uint8_t t_num = 0; t_num < _dataNum; t_num++) {
                    String targetType = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                            [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][t_num]
                                            [BLINKER_CMD_TYPE];

                    // String targetValue = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                    //                     [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][0]
                    //                     [BLINKER_CMD_VALUE];

                    String targetKey = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                        [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][t_num]
                                        [BLINKER_CMD_KEY];
                    
                    strcpy(_targetKey[t_num], targetKey.c_str());

                    if (targetType == BLINKERCMD_ON) {
                        _dataType[t_num]   = BLINKER_TYPE_STATE;
                        _targetType[t_num] = 1;
                        _targetValue[t_num] = 1;
                    }
                    else if (targetType == BLINKERCMD_OFF) {
                        _dataType[t_num]   = BLINKER_TYPE_STATE;
                        _targetType[t_num] = 0;
                        _targetValue[t_num] = 0;
                    }
                    else if (targetType == BLINKER_CMD_LESS) {
                        _dataType[t_num]   = BLINKER_TYPE_NUMERIC;
                        _targetType[t_num] = BLINKER_COMPARE_LESS;
                        _targetValue[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                                [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][t_num]
                                                [BLINKER_CMD_VALUE];
                    }
                    else if (targetType == BLINKER_CMD_EQUAL) {
                        _dataType[t_num]   = BLINKER_TYPE_NUMERIC;
                        _targetType[t_num] = BLINKER_COMPARE_EQUAL;
                        _targetValue[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                                [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][t_num]
                                                [BLINKER_CMD_VALUE];
                    }
                    else if (targetType == BLINKER_CMD_GREATER) {
                        _dataType[t_num]   = BLINKER_TYPE_NUMERIC;
                        _targetType[t_num] = BLINKER_COMPARE_GREATER;
                        _targetValue[t_num] = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                                [BLINKER_CMD_COMMAND][BLINKER_CMD_DATA][t_num]
                                                [BLINKER_CMD_VALUE];
                    }
                }
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("_dataType[0]: "),    _dataType[0]);
                BLINKER_LOG2(BLINKER_F("_targetKey[0]: "),   _targetKey[0]);
                BLINKER_LOG2(BLINKER_F("_targetValue[0]: "), _targetValue[0]);
                BLINKER_LOG2(BLINKER_F("_targetType[0]: "),  _targetType[0]);

                BLINKER_LOG2(BLINKER_F("_dataType[1]: "),    _dataType[1]);
                BLINKER_LOG2(BLINKER_F("_targetKey[1]: "),   _targetKey[1]);
                BLINKER_LOG2(BLINKER_F("_targetValue[1]: "), _targetValue[1]);
                BLINKER_LOG2(BLINKER_F("_targetType[1]: "),  _targetType[1]);
                // BLINKER_LOG2(BLINKER_F("_duration: "), _duration[0]);
#endif
            }

            String smsMsg = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                [BLINKER_CMD_COMMAND][BLINKER_CMD_SMS];

            String pushMsg = root[BLINKER_CMD_SET][BLINKER_CMD_EVENT]
                                [BLINKER_CMD_COMMAND][BLINKER_CMD_PUSH];

            if (smsMsg.length() == 0) {
                _msgType = 1;

                strcpy(_msg, pushMsg.c_str());
            }
            else if (pushMsg.length() == 0) {
                _msgType = 0;

                strcpy(_msg, smsMsg.c_str());
            }
            else {
                memcpy(_msg, "\0", BLINKER_EVENT_MSG_SIZE);
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("_msg: "), _msg);
#endif
        }

        void serialization() {}

        void deserialization() {
            uint8_t checkData;

            EEPROM.begin(BLINKER_EEP_SIZE);

            EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);

            if (checkData != BLINKER_CHECK_DATA) {
                EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
            }
        }

        void setNum(uint8_t num) {
            e_num = num;
        }

    private :
        // - - - - - - - -  - - - - - - - -
        // |   |   |            
        // |   |   |_targetValue
        // |   |_targetType on/off|less/equal/greater
        // |_dataType state/numberic
        // dataData

        // - - - - - - - -  - - - - - - - -
        // | | |   |   |_msgType sms/push
        // | | |   |_dataNum
        // | | |_eventType state/numberic/and/or
        // | |_eventState run
        // |_haveEvent
        // eventData

        uint8_t e_num;

        uint8_t _eventData;
        uint16_t _dataData[2];
        
        bool    _haveEvent;
        bool    _eventState;
        uint8_t _eventType;
        uint8_t _dataNum;
        uint8_t _msgType;

        uint8_t _dataType[2];
        char    _targetKey[2][12];
        int8_t  _targetValue[2];
        uint8_t _targetType[2];
        uint32_t _duration[2];

        uint8_t _msgType;
        char    _msg[BLINKER_EVENT_MSG_SIZE];
        
};
#endif

#endif