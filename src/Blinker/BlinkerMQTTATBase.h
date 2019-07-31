#ifndef BLINKER_MQTT_AT_BASE_H
#define BLINKER_MQTT_AT_BASE_H

#if defined(ESP8266) || defined(ESP32)

#include "BlinkerDebug.h"
#include "BlinkerConfig.h"
#include "BlinkerUtility.h"

// extern uint32_t serialSet;

// #if defined(ESP8266)
// extern SerialConfig ss_cfg;
// #elif defined(ESP32)
// extern uint32_t    ss_cfg;
// #endif

// #if defined(ESP8266)
// extern SerialConfig serConfig();
// #elif defined(ESP32)
// extern uint32_t    serConfig();
// #endif

uint8_t parseMode(uint8_t _mode, uint8_t _pullState);

enum blinker_at_state_t {
    AT_NONE,
    AT_TEST,
    AT_QUERY,
    AT_SETTING,
    AT_ACTION
};

enum blinker_at_status_t {
    BL_BEGIN,
    BL_INITED
    // ,
    // BL_SUC,
    // MQTT_WIFI_PSWD
};

enum blinker_at_serial_t {
    SER_BAUD,
    SER_DBIT,
    SER_SBIT,
    SER_PRIT
};

enum blinker_at_mqtt_t {
    MQTT_CONFIG_MODE,
    MQTT_AUTH_KEY,
    MQTT_WIFI_SSID,
    MQTT_WIFI_PSWD
};

enum blinker_at_aligenie_t {
    ALI_NONE,
    ALI_LIGHT,
    ALI_OUTLET,
    ALI_SENSOR
};

enum blinker_at_dueros_t {
    DUER_NONE,
    DUER_LIGHT,
    DUER_OUTLET,
    DUER_SENSOR
};

uint32_t serialSet = BLINKER_SERIAL_DEFAULT;

#if defined(ESP8266)
SerialConfig ss_cfg;
#elif defined(ESP32)
uint32_t    ss_cfg;
#endif

#if defined(ESP8266)
SerialConfig serConfig()
#elif defined(ESP32)
uint32_t    serConfig()
#endif
{
    uint8_t _bitSet = serialSet & 0xFF;

    switch (_bitSet)
    {
        case BLINKER_SERIAL_5N1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_5N1"));
            return SERIAL_5N1;
        case BLINKER_SERIAL_6N1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_6N1"));
            return SERIAL_6N1;
        case BLINKER_SERIAL_7N1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_7N1"));
            return SERIAL_7N1;
        case BLINKER_SERIAL_8N1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8N1"));
            return SERIAL_8N1;
        case BLINKER_SERIAL_5N2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_5N2"));
            return SERIAL_5N2;
        case BLINKER_SERIAL_6N2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_6N2"));
            return SERIAL_6N2;
        case BLINKER_SERIAL_7N2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_7N2"));
            return SERIAL_7N2;
        case BLINKER_SERIAL_8N2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8N2"));
            return SERIAL_8N2;
        case BLINKER_SERIAL_5E1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_5E1"));
            return SERIAL_5E1;
        case BLINKER_SERIAL_6E1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_6E1"));
            return SERIAL_6E1;
        case BLINKER_SERIAL_7E1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_7E1"));
            return SERIAL_7E1;
        case BLINKER_SERIAL_8E1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8E1"));
            return SERIAL_8E1;
        case BLINKER_SERIAL_5E2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_5E2"));
            return SERIAL_5E2;
        case BLINKER_SERIAL_6E2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_6E2"));
            return SERIAL_6E2;
        case BLINKER_SERIAL_7E2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_7E2"));
            return SERIAL_7E2;
        case BLINKER_SERIAL_8E2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8E2"));
            return SERIAL_8E2;
        case BLINKER_SERIAL_5O1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_5O1"));
            return SERIAL_5O1;
        case BLINKER_SERIAL_6O1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_6O1"));
            return SERIAL_6O1;
        case BLINKER_SERIAL_7O1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_7O1"));
            return SERIAL_7O1;
        case BLINKER_SERIAL_8O1 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8O1"));
            return SERIAL_8O1;
        case BLINKER_SERIAL_5O2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_5O2"));
            return SERIAL_5O2;
        case BLINKER_SERIAL_6O2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_6O2"));
            return SERIAL_6O2;
        case BLINKER_SERIAL_7O2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_7O2"));
            return SERIAL_7O2;
        case BLINKER_SERIAL_8O2 :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8O2"));
            return SERIAL_8O2;
        default :
            BLINKER_LOG_ALL(BLINKER_F("SerialConfig SERIAL_8N1"));
            return SERIAL_8N1;
    }
}

uint8_t parseMode(uint8_t _mode, uint8_t _pullState)
{
    uint8_t _modes = _mode << 4 | _pullState;

    switch(_modes)
    {
        case 0 << 4 | 0 :
            return INPUT;
        case 1 << 4 | 0 :
            return OUTPUT;
        case 0 << 4 | 1 :
            return INPUT_PULLUP;
        case 0 << 4 | 2 :
#if defined(ESP8266)
            return INPUT_PULLDOWN_16;
#elif defined(ESP32)
            return INPUT_PULLDOWN;
#endif
        default :
            return INPUT;
    }
}

class BlinkerSlaverAT
{
    public :
        BlinkerSlaverAT() {}

        void update(const String & data) {
            // _data = data;
            // BLINKER_LOG(BLINKER_F("update data: "), data);
            // _isAT = serialize(data);
            serialize(data);

            BLINKER_LOG(BLINKER_F("serialize _set: "), _set);
            // return _isAT;
        }

        blinker_at_state_t state() { return _set; }

        String cmd() { return _atCmd; }

        uint8_t paramNum() { return _paramNum; }

        String getParam(uint8_t num) {
            if (num >= _paramNum) return "";
            else return _param[num];
        }

    private :
        // bool _isAT;
        blinker_at_state_t _set;
        uint8_t _paramNum;
        // String _data;
        String _atCmd;
        String _param[6];

        void serialize(const String & _data) {
            BLINKER_LOG_ALL(BLINKER_F("serialize _data: "), _data);

            _paramNum = 0;
            // _isAT = false;
            _set = AT_NONE;
            int addr_start = 0;//_data.indexOf("+");
            int addr_end = 0;

            String startCmd = _data.substring(0, 2);
            BLINKER_LOG_ALL(BLINKER_F("startCmd: "), startCmd);

            // startCmd = _data.substring(0, 3);
            // BLINKER_LOG(BLINKER_F("startCmd: "), startCmd);

            BLINKER_LOG_ALL(BLINKER_F("startCmd len: "), _data.length());

            // BLINKER_LOG(BLINKER_F("serialize addr_start: "), addr_start);
            // BLINKER_LOG(BLINKER_F("serialize addr_end: "), addr_end);

            // if ((addr_start != -1) && STRING_contains_string(_data, ":"))
            if (startCmd == BLINKER_CMD_AT)
            {
                // _isAT = true;

                // if (_data.length() == 3) {
                //     _set = AT_ACTION;
                //     return true;
                // }

                addr_start = 3;
                // int addr_end1 = _data.indexOf("=");
                // int addr_end2 = _data.indexOf("?");
                // int addr_end3 = _data.indexOf("\n");

                // check "="
                addr_end = _data.indexOf("=");
                BLINKER_LOG_ALL(BLINKER_F("serialize addr_end: "), addr_end);
                if (addr_end != -1)
                {
                    _set = AT_SETTING;

                    _atCmd = _data.substring(addr_start, addr_end);
                    BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                }
                else
                {
                    addr_end = _data.indexOf("?");
                    BLINKER_LOG_ALL(BLINKER_F("serialize addr_end: "), addr_end);
                    if (addr_end != -1)
                    {
                        _set = AT_QUERY;

                        _atCmd = _data.substring(addr_start, addr_end);
                        BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                        return;
                    }
                    else
                    {
                        addr_end = _data.indexOf("\r");
                        BLINKER_LOG_ALL(BLINKER_F("serialize addr_end: "), addr_end);
                        if (addr_end != -1)
                        {
                            _set = AT_ACTION;

                            if (addr_end == (addr_start-1)) {
                                _atCmd = startCmd;
                                BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                                return;
                            }

                            _atCmd = _data.substring(addr_start, addr_end);
                            BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                            return;
                        }
                        else if (_data.length() == startCmd.length())
                        {
                            _set = AT_ACTION;

                            // if (addr_end == -1) {
                            _atCmd = startCmd;
                            BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                            return;
                            // }

                            // _atCmd = _data.substring(addr_start, addr_end);
                            // BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                            // return;
                        }
                        else
                        {
                            _set = AT_ACTION;

                            _atCmd = _data.substring(3, _data.length());
                            BLINKER_LOG_ALL(BLINKER_F("serialize _atCmd: "), _atCmd);
                            return;
                        }
                    }
                }

                // BLINKER_LOG(BLINKER_F("serialize _data: "), _data);

                String serData;
                uint16_t dataLen;
                if (_data.indexOf("\r") != -1) dataLen = _data.length() - 1;
                else dataLen = _data.length();

                addr_start = 0;

                for (_paramNum = 0; _paramNum < 11; _paramNum++) {
                    // if (addr_end != -1)
                    // {
                        addr_start += addr_end;
                        addr_start += 1;
                    // }
                    // else
                    // {
                    //     addr_start = 3;
                    // }
                    
                    serData = _data.substring(addr_start, dataLen);

                    addr_end = serData.indexOf(",");

                    BLINKER_LOG_ALL(BLINKER_F("serialize serData: "), serData);
                    BLINKER_LOG_ALL(BLINKER_F("serialize addr_start: "), addr_start);
                    BLINKER_LOG_ALL(BLINKER_F("serialize addr_end: "), addr_end);

                    if (addr_end == -1) {
                        if (addr_start >= dataLen) return;

                        addr_end = serData.indexOf(" ");

                        if (addr_end != -1) {
                            _param[_paramNum] = serData.substring(0, addr_end);

                            if (_param[_paramNum] == "?" && _paramNum == 0) {
                                _set = AT_TEST;
                            }
                            BLINKER_LOG_ALL(BLINKER_F("_param0["), _paramNum, \
                                            BLINKER_F("]: "), _param[_paramNum]);
                            // BLINKER_LOG_ALL(BLINKER_F("_set: "), _set);
                            _paramNum++;
                            return;
                        }

                        _param[_paramNum] = serData;

                        if (_param[_paramNum] == "?" && _paramNum == 0) {
                            _set = AT_TEST;
                        }
                        BLINKER_LOG_ALL(BLINKER_F("_param1["), _paramNum, \
                                        BLINKER_F("]: "), _param[_paramNum]);
                        // BLINKER_LOG_ALL(BLINKER_F("_set: "), _set);
                        _paramNum++;
                        return;
                    }
                    else {
                        _param[_paramNum] = serData.substring(0, addr_end);
                    }
                    BLINKER_LOG_ALL(BLINKER_F("_param["), _paramNum, \
                                    BLINKER_F("]: "), _param[_paramNum]);
                }
                // return;
            }
            // else {
            //     // _isAT = false;
            //     return;
            // }
        }
};

enum atPin_t {
    PIN_SET,
    PIN_MODE,
    PIN_PULLSTATE
};

enum atIO_t {
    IO_PIN,
    IO_LVL
};

class PinData
{
    public :
        PinData(uint8_t _pin, uint8_t _mode, uint8_t _pullState)
        {
            // pin_num = _pin;
            // pin_mode = _mode;
            // pin_pull = _pullState;
            _pinDatas = _pin << 8 | _mode << 4 | _pullState;

            pinMode(_pin, parseMode(_mode, _pullState));
        }

        uint8_t getPin()  { return (_pinDatas >> 8 & 0xFF); }
        uint8_t getMode() { return (_pinDatas >> 4 & 0x0F); }
        uint8_t getPull() { return (_pinDatas      & 0x0F); }
        
        String data() {
            String _data =  STRING_format(_pinDatas >> 8 & 0xFF) + "," + \
                            STRING_format(_pinDatas >> 4 & 0x0F) + "," + \
                            STRING_format(_pinDatas      & 0x0F);

            return _data;
        }

        bool checkPin(uint8_t _pin) { return (_pinDatas >> 8 & 0xFF) == _pin; }

        void fresh(uint8_t _mode, uint8_t _pullState)
        {
            _pinDatas = (_pinDatas >> 8 & 0xFF) << 8 | _mode << 4 | _pullState;

            pinMode((_pinDatas >> 8 & 0xFF) << 8, parseMode(_mode, _pullState));
        }

    private :
        uint16_t _pinDatas;
        // uint8_t     pin_num;
        // atPinMode_t pin_mode;
        // atPull_t    pin_pull;
};
// static class BlinkerSlaverAT * _slaverAT;
// static class PinData * _pinData[BLINKER_MAX_PIN_NUM];

#endif

#endif
