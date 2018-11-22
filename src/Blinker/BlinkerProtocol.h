#ifndef BlinkerProtocol_H
#define BlinkerProtocol_H

#include "Blinker/BlinkerApi.h"

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#if defined(BLINKER_PRO)
enum BlinkerStatus{
    PRO_WLAN_CONNECTING,
    PRO_WLAN_CONNECTED,
    // PRO_WLAN_DISCONNECTED,
    PRO_WLAN_SMARTCONFIG_BEGIN,
    PRO_WLAN_SMARTCONFIG_DONE,
    PRO_DEV_AUTHCHECK_FAIL,
    PRO_DEV_AUTHCHECK_SUCCESS,
    PRO_DEV_REGISTER_FAIL,
    PRO_DEV_REGISTER_SUCCESS,
    PRO_DEV_INIT_SUCCESS,
    PRO_DEV_CONNECTING,
    PRO_DEV_CONNECTED,
    PRO_DEV_DISCONNECTED
};
#endif

#if defined(BLINKER_AT_MQTT)
static uint32_t serialSet = BLINKER_SERIAL_DEFAULT;

static SerialConfig ss_cfg;

static SerialConfig serConfig()
{
    uint8_t _bitSet = serialSet & 0xFF;

    switch (_bitSet)
    {
        case BLINKER_SERIAL_5N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5N1"));
            return SERIAL_5N1;
        case BLINKER_SERIAL_6N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6N1"));
            return SERIAL_6N1;
        case BLINKER_SERIAL_7N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7N1"));
            return SERIAL_7N1;
        case BLINKER_SERIAL_8N1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8N1"));
            return SERIAL_8N1;
        case BLINKER_SERIAL_5N2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5N2"));
            return SERIAL_5N2;
        case BLINKER_SERIAL_6N2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6N2"));
            return SERIAL_6N2;
        case BLINKER_SERIAL_7N2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7N2"));
            return SERIAL_7N2;
        case BLINKER_SERIAL_8N2 :
            BLINKER_LOG(("SerialConfig SERIAL_8N2"));
            return SERIAL_8N2;
        case BLINKER_SERIAL_5E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5E1"));
            return SERIAL_5E1;
        case BLINKER_SERIAL_6E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6E1"));
            return SERIAL_6E1;
        case BLINKER_SERIAL_7E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7E1"));
            return SERIAL_7E1;
        case BLINKER_SERIAL_8E1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8E1"));
            return SERIAL_8E1;
        case BLINKER_SERIAL_5E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5E2"));
            return SERIAL_5E2;
        case BLINKER_SERIAL_6E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6E2"));
            return SERIAL_6E2;
        case BLINKER_SERIAL_7E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7E2"));
            return SERIAL_7E2;
        case BLINKER_SERIAL_8E2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8E2"));
            return SERIAL_8E2;
        case BLINKER_SERIAL_5O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5O1"));
            return SERIAL_5O1;
        case BLINKER_SERIAL_6O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6O1"));
            return SERIAL_6O1;
        case BLINKER_SERIAL_7O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7O1"));
            return SERIAL_7O1;
        case BLINKER_SERIAL_8O1 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8O1"));
            return SERIAL_8O1;
        case BLINKER_SERIAL_5O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_5O2"));
            return SERIAL_5O2;
        case BLINKER_SERIAL_6O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_6O2"));
            return SERIAL_6O2;
        case BLINKER_SERIAL_7O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_7O2"));
            return SERIAL_7O2;
        case BLINKER_SERIAL_8O2 :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8O2"));
            return SERIAL_8O2;
        default :
            BLINKER_LOG_ALL(("SerialConfig SERIAL_8N1"));
            return SERIAL_8N1;
    }
}

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
                    }
                }

                // BLINKER_LOG(BLINKER_F("serialize _data: "), _data);

                String serData;
                uint16_t dataLen = _data.length() - 1;

                addr_start = 0;

                for (_paramNum = 0; _paramNum < 11; _paramNum++) {
                    addr_start += addr_end;
                    addr_start += 1;
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

uint8_t parseMode(uint8_t _mode, uint8_t _pullState) {
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

static class BlinkerSlaverAT * _slaverAT;
static class PinData * _pinData[BLINKER_MAX_PIN_NUM];
#endif

template <class Transp>
class BlinkerProtocol
    : public BlinkerApi< BlinkerProtocol<Transp> >
{
    friend class BlinkerApi< BlinkerProtocol<Transp> >;

    typedef BlinkerApi< BlinkerProtocol<Transp> > BApi;

    public :
        enum BlinkerState{
            CONNECTING,
            CONNECTED,
            DISCONNECTED
        };

        BlinkerProtocol(Transp& transp)
            : BApi()
            , conn(transp)
            , state(CONNECTING)
            , isFresh(false)
            , isAvail(false)
            , availState(false)
            , canParse(false)
            // , isFormat(false)
        {}

        bool connected() { return state == CONNECTED; }

        bool connect(uint32_t timeout = BLINKER_STREAM_TIMEOUT)
        {
            state = CONNECTING;

            uint32_t startTime = millis();
            while ( (state != CONNECTED) && (millis() - startTime) < timeout ) {
                run();
            }

            return state == CONNECTED;
        }

        void disconnect() 
        {
            conn.disconnect();
            state = DISCONNECTED;
        }

        void run();

        bool available() {
            bool _avail = availState;
            availState = false;
            return _avail;
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        bool bridgeAvailable(const String & bKey) {
            if (checkExtraAvail()) {
                String b_name = BApi::bridgeFind(bKey);

                // BLINKER_LOG("bridgeAvailable b_name: ", b_name);

                if (b_name.length() > 0) {
                    // _bKey_forwhile = b_name;
                    String b_data = conn.lastRead();

                    // BLINKER_LOG("bridgeAvailable b_data: ", b_data);

                    DynamicJsonBuffer jsonBuffer;
                    JsonObject& extra_data = jsonBuffer.parseObject(b_data);

                    String _from = extra_data["fromDevice"];

                    // BLINKER_LOG("bridgeAvailable _from: ", _from);

                    if (b_name == _from) {
                        _bKey_forwhile = b_name;
                        isExtraAvail = false;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                else{
                    _bKey_forwhile = "";
                    return false;
                }
            }
            else {
                return false;
            }
        }

        String bridgeRead() {
            String b_data = conn.lastRead();

            if (_bKey_forwhile.length() > 0) {
                DynamicJsonBuffer jsonBuffer;
                JsonObject& extra_data = jsonBuffer.parseObject(b_data);

                if (!extra_data.success()) {
                    return "";
                }
                else {
                    String _from = extra_data["fromDevice"];
                    if (_from == _bKey_forwhile) {
                        String _data = extra_data["data"];

                        _bKey_forwhile = "";
                        return _data;
                    }
                    else {
                        return "";
                    }
                }
                // return 
            }
            else{
                return "";
            }
        }

        String bridgeRead(const String & bKey) {
            String b_name = BApi::bridgeFind(bKey);
            String b_data = conn.lastRead();

            if (b_name.length() > 0 && isBridgeFresh) {
                DynamicJsonBuffer jsonBuffer;
                JsonObject& extra_data = jsonBuffer.parseObject(b_data);

                if (!extra_data.success()) {
                    return "";
                }
                else {
                    String _from = extra_data["fromDevice"];
                    if (_from == b_name) {
                        String _data = extra_data["data"];
                        isBridgeFresh = false;
                        return _data;
                    }
                    else {
                        return "";
                    }
                }
                // return 
            }
            else{
                return "";
            }
        }
#endif

        String readString()
        {
            if (isFresh) {
                isFresh = false;
                String r_data = conn.lastRead();
                flush();
                return r_data;
            }
            else {
                return "";
            }
        }
        
//         template <typename T>
// #if defined(BLINKER_MQTT)
//         void print(T n, bool state = false) {
//             String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
//             if (data.length() <= BLINKER_MAX_SEND_SIZE) {
//                 conn.print(data, state);
//             }
//             else {
//                 BLINKER_ERR_LOG("SEND DATA BYTES MAX THAN LIMIT!");
//             }
//         }
// #else
//         void print(T n) {
//             String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
//             if (data.length() <= BLINKER_MAX_SEND_BUFFER_SIZE) {
//                 conn.print(data);
//             }
//             else {
//                 BLINKER_ERR_LOG("SEND DATA BYTES MAX THAN LIMIT!");
//             }
//             _print(n);
//         }
// #endif
        // void beginFormat() {
        //     isFormat = true;
        //     memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
        // }

        // bool endFormat() {
        //     isFormat = false;
        //     if (strlen(_sendBuf)) {
        //         _print("{" + STRING_format(_sendBuf) + "}");
        //     }
        //     else {
        //         return true;
        //     }

        //     if (strlen(_sendBuf) > BLINKER_MAX_SEND_BUFFER_SIZE - 3) {
        //         return false;
        //     }
        //     else {
        //         return true;
        //     }
        // }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        void bridgeBeginFormat() {
            isBformat = true;
            _bridgeKey = "";
            memset(_bSendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
        }

        bool bridgeEndFormat() {
            isBformat = false;
            if (strlen(_bSendBuf)) {
                _bPrint(_bridgeKey, "{" + STRING_format(_bSendBuf) + "}");
            }

            if (strlen(_bSendBuf) > BLINKER_MAX_SEND_SIZE - 3) {
                return false;
            }
            else {
                return true;
            }
        }
#endif

        template <typename T>
        void print(T n) {
            // if (!isFormat)
                _print("\""  + STRING_format(n)+ "\"");
        }
        void print() {
            // if (!isFormat)
                _print("\"\"");
        }
        
        template <typename T>
        void println(T n) {
            // if (!isFormat)
                _print("\""  + STRING_format(n)+ "\"");
        }
        void println() {
            // if (!isFormat)
                _print("\"\"");
        }

        template <typename T1, typename T2, typename T3>
        void print(T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
// #if defined(BLINKER_MQTT_AT)
//     #if defined(BLINKER_MQTT_AT_JSON)
//                     autoFormatData("deviceTy", "OwnApp");
//     #else
//                     autoFormatData("\"deviceTy\":\"OwnApp\"");
//     #endif
// #endif
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }
        
        template <typename T1, typename T2, typename T3>
        void println(T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        // template <typename T1, typename T2>
        // void print(T1 n1, T2 n2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"";

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        
        // template <typename T1, typename T2>
        // void println(T1 n1, T2 n2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"";

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        template <typename T1>
        void printArray(T1 n1, const String &s2) {
            // String _msg = "\"" + STRING_format(n1) + "\":" + s2;
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG(BLINKER_F("printArray _msg: "), "\"" + STRING_format(n1) + "\":" + s2);
// #endif
            // if (isFormat) {
            //     formatData("\"" + STRING_format(n1) + "\":" + s2);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }

                // autoFormatData("\"" + STRING_format(n1) + "\":" + s2);
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), "\"" + STRING_format(n1) + "\":" + s2);
#else
                autoFormatData("\"" + STRING_format(n1) + "\":" + s2);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void printObject(T1 n1, const String &s2) {
            String _msg = "\"" + STRING_format(n1) + "\":" + s2;

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

// #if defined(ESP8266) || defined(ESP32)
//         void printJson(const String &s) {
//             DynamicJsonBuffer jsonBuffer;
//             JsonObject& json_data = jsonBuffer.parseObject("{" + s + "}");

//             if (!json_data.success()) {
//                 BLINKER_ERR_LOG("data is not a JSON!");
//                 return;
//             }
            
//             if (isFormat) {
//                 formatData(s);
//             }
//             else {
//             //     _print("{" + _msg + "}");
//             // }

//                 if (!autoFormat) {
//                     autoFormat = true;
//                     memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
//                 }

//                 autoFormatData(s);
//                 autoFormatFreshTime = millis();
//             }
//         }
// #endif
        
        template <typename T1>
        void print(T1 n1, const String &s2) {
            String _msg = "\"" + STRING_format(n1) + "\":\"" + s2 + "\"";

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, const char str2[]) {
            String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(str2) + "\"";

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, char c) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(c);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, unsigned char b) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(b);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, int n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, unsigned int n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, long n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, unsigned long n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void print(T1 n1, double n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(n1), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }

        template <typename T1>
        void println(T1 n1, const String &s2)    { print(n1, s2); }
        template <typename T1>
        void println(T1 n1, const char str2[])   { print(n1, str2); }
        template <typename T1>
        void println(T1 n1, char c)              { print(n1, c); }
        template <typename T1>
        void println(T1 n1, unsigned char b)     { print(n1, b); }
        template <typename T1>
        void println(T1 n1, int n)               { print(n1, n); }
        template <typename T1>
        void println(T1 n1, unsigned int n)      { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, long n)              { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, unsigned long n)     { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, double n)            { print(n1, n); }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)

        void beginAuto() {
            BLINKER_LOG(BLINKER_F("======================================================="));
            BLINKER_LOG(BLINKER_F("=========== Blinker Auto Control mode init! ==========="));
            BLINKER_LOG(BLINKER_F("Warning!EEPROM address 0-1279 is used for Auto Control!"));
            BLINKER_LOG(BLINKER_F("=========== DON'T USE THESE EEPROM ADDRESS! ==========="));
            BLINKER_LOG(BLINKER_F("======================================================="));

            BLINKER_LOG(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

            _isAuto = true;
            // deserialization();
            // autoStart();
            BApi::autoInit();
        }
#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        // template <typename T>
        // void bridgePrint(const String & bKey, T n) {
        //     if (!isBformat) {
        //         _bPrint(bKey, "\""  + STRING_format(n)+ "\"");
        //     }
        //     else {
        //         _bridgeKey = bKey;
        //     }
        // }
        // void bridgePrint(const String & bKey) {
        //     if (!isBformat)
        //         _bPrint("\"\"");
        // }

        template <typename T1, typename T2, typename T3>
        void bridgePrint(const String & bKey, T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        // template <typename T1, typename T2>
        // void print(T1 n1, T2 n2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"";

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        
        // template <typename T1, typename T2>
        // void println(T1 n1, T2 n2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(n2) + "\"";

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        // template <typename T1>
        // void printArray(T1 n1, const String &s2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + s2;

        //     if (isFormat) {
        //         formatData(_msg);
        //     }
        //     else {
        //         _print("{" + _msg + "}");
        //     }
        // }
        
        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, const String &s2) {
            String _msg = "\"" + STRING_format(n1) + "\":\"" + s2 + "\"";

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, const char str2[]) {
            String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(str2) + "\"";

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, char c) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(c);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, unsigned char b) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(b);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, int n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, unsigned int n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, long n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, unsigned long n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }

        template <typename T1>
        void bridgePrint(const String & bKey, T1 n1, double n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isBformat) {
                _bridgeKey = bKey;
                bridgeFormatData(_msg);
            }
            else {
                _bPrint(bKey, "{" + _msg + "}");
            }
        }
#endif

#if (defined(BLINKER_MQTT) || defined(BLINKER_PRO) \
    || defined(BLINKER_MQTT_AT)) && defined(BLINKER_ALIGENIE)
        // template <typename T1>
        // void aligeniePrint(T1 n1, const String &s2) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + s2 + "\"";

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, const char str2[]) {
        //     String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(str2) + "\"";

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, char c) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(c);

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, unsigned char b) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(b);

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, int n) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, unsigned int n) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, long n) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, unsigned long n) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

        //     conn.aliPrint(_msg);
        // }

        // template <typename T1>
        // void aligeniePrint(T1 n1, double n) {
        //     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

        //     conn.aliPrint(_msg);
        // }

        void aligeniePrint(const String & _msg) {
            BLINKER_LOG_ALL(BLINKER_F("response to AliGenie: "), _msg);

            conn.aliPrint(_msg);
        }
#endif
        
        template <typename T>
        void notify(T n) {
            String _msg = "\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"";
            
            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(STRING_format(BLINKER_CMD_NOTICE), _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }

// #if defined(BLINKER_MQTT)
//             print("{\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"}", true);
// #else
//             _print("{\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"}");
// #endif
        }

        void textPrint(const String & tName, const String & title, const String & payload) {
            String _msg = "\"" + tName + "\":[\"" + title + "\",\"" + payload + "\"]";

            // if (isFormat) {
            //     formatData(_msg);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
                }
#if defined(ESP8266) || defined(ESP32)
                autoFormatData(tName, _msg);
#else
                autoFormatData(_msg);
#endif
                autoFormatFreshTime = millis();
            // }
        }
        
        void flush() {
            conn.flush();
            isFresh = false;
            availState = false;
            canParse = false;
            isAvail = false;
        }

        void flushAll() {
            conn.flush();
            isAvail = false;
            isFresh = false;
            availState = false;
            canParse = false;
        }

#if defined(BLINKER_PRO)
        bool init() { return _isInit;}

        bool registered() { return conn.authCheck(); }

        uint8_t status() { return _proStatus; }
#endif

    private :
//         void formatData(String data) {
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG(BLINKER_F("formatData data: "), data);
// #endif
//             if ((strlen(_sendBuf) + data.length()) >= BLINKER_MAX_SEND_SIZE) {
//                 BLINKER_ERR_LOG("FORMAT DATA SIZE IS MAX THAN LIMIT");
//                 return;
//             }

//             if (strlen(_sendBuf) > 0) {
//                 data = "," + data;
//                 strcat(_sendBuf, data.c_str());
//             }
//             else {
//                 strcpy(_sendBuf, data.c_str());
//             }
//         }

#if defined(ESP8266) || defined(ESP32) || defined(BLINKER_MQTT_AT_JSON)
        void autoFormatData(const String & key, const String & jsonValue) {
            // String _value = STRING_format(value);
            // if ((strlen(_sendBuf) + key.length() + _value.length()) >= BLINKER_MAX_SEND_SIZE) {
            //     BLINKER_ERR_LOG("FORMAT DATA SIZE IS MAX THAN LIMIT");
            //     return;
            // }

            BLINKER_LOG_ALL(BLINKER_F("autoFormatData key: "), key, BLINKER_F(", json: "), jsonValue);

            String _data;

            if (STRING_contains_string(STRING_format(_sendBuf), key)) {

                DynamicJsonBuffer jsonSendBuffer;                

                if (strlen(_sendBuf)) {
                    BLINKER_LOG_ALL(BLINKER_F("add"));

                    JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_sendBuf));

                    if (root.containsKey(key)) {
                        root.remove(key);
                    }
                    root.printTo(_data);

                    _data = _data.substring(0, _data.length() - 1);

                    _data += "," + jsonValue + "}";
                }
                else {
                    BLINKER_LOG_ALL(BLINKER_F("new"));
                    
                    _data = "{" + jsonValue + "}";
                }
            }
            else {
                _data = STRING_format(_sendBuf);

                if (_data.length()) {
                    BLINKER_LOG_ALL(BLINKER_F("add."));

                    _data = _data.substring(0, _data.length() - 1);

                    _data += "," + jsonValue + "}";
                }
                else {
                    BLINKER_LOG_ALL(BLINKER_F("new."));
                    
                    _data = "{" + jsonValue + "}";
                }
            }

            if (strlen(_sendBuf) > BLINKER_MAX_SEND_SIZE) {
                BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
                return;
            }

            strcpy(_sendBuf, _data.c_str());
        }
#endif

        void autoFormatData(String & data) {
            BLINKER_LOG_ALL(BLINKER_F("autoFormatData data: "), data);
            BLINKER_LOG_ALL(BLINKER_F("strlen(_sendBuf): "), strlen(_sendBuf));
            BLINKER_LOG_ALL(BLINKER_F("data.length(): "), data.length());

            if ((strlen(_sendBuf) + data.length()) >= BLINKER_MAX_SEND_SIZE) {
                BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
                return;
            }

            if (strlen(_sendBuf) > 0) {
                data = "," + data;
                strcat(_sendBuf, data.c_str());
            }
            else {
                strcpy(_sendBuf, data.c_str());
            }
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        void bridgeFormatData(String data) {
            if (strlen(_bSendBuf) > 0) {
                data = "," + data;
                strcat(_bSendBuf, data.c_str());
            }
            else {
                strcpy(_bSendBuf, data.c_str());
            }
        }

        void autoBridgeFormatData(const String & key, const String & jsonValue) {
            BLINKER_LOG_ALL(BLINKER_F("autoBridgeFormatData key: "), key, BLINKER_F(", jsonValue: "), jsonValue);

            DynamicJsonBuffer jsonSendBuffer;

            String _data;

            if (strlen(_bSendBuf)) {
                BLINKER_LOG_ALL(BLINKER_F("autoBridgeFormatData add"));
                
                JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_bSendBuf));

                if (root.containsKey(key)) {
                    root.remove(key);
                }
                root.printTo(_data);

                _data = _data.substring(0, _data.length() - 1);

                _data += "," + jsonValue + "}";
            }
            else {
                BLINKER_LOG_ALL(BLINKER_F("autoBridgeFormatData new"));
                
                _data = "{" + jsonValue + "}";
            }

            if (strlen(_bSendBuf) > BLINKER_MAX_SEND_SIZE) {
                BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
                return;
            }

            strcpy(_bSendBuf, _data.c_str());
        }

        bool checkExtraAvail()
        {
            if (conn.extraAvailable()) {
                isExtraAvail = true;
            }

            if (isExtraAvail) {
                isBridgeFresh = true;
                // BLINKER_LOG("isExtraAvail true");
            }
            return isExtraAvail;
        }
#endif

        bool checkAvail()
        {
            flush();

            isAvail = conn.available();
            if (isAvail) {
                isFresh = true;
                canParse = true;
                availState = true;
            }

// #if defined(BLINKER_AT_MQTT)
//             if (isAvail) conn.serialPrint(conn.lastRead());

//             if (serialAvailable()) conn.mqttPrint(conn.serialLastRead());
// #endif
            return isAvail;
        }

        bool checkAliAvail()
        {
            return conn.aligenieAvail();
        }

        char * dataParse()
        {
            if (canParse) {
                // return conn.lastRead();
// #if defined(BLINKER_AT_MQTT) 
                // DynamicJsonBuffer jsonBuffer;
                // JsonObject& root = jsonBuffer.parseObject(conn.lastRead());
                // // JsonObject& root = jsonBuffer.parseObject((char *)iotSub->lastread);

                // if (!root.success()) {
                //     // BLINKER_LOG("json test error");
                // return conn.dataParse();
                // }

                // String _uuid = root["fromDevice"];

                // if (_uuid == conn.uuid())
                // {
                //     String dataGet = root["data"];
                //     char dataP[512-128];
                //     strcpy(dataP, dataGet.c_str());

                //     return dataP;
                // }
                // else {
                //     return "";
                // }
// #else
                return conn.lastRead();
// #endif
            }
            else {
                return "";
            }
        }

        char * lastRead() { return conn.lastRead(); }

        void isParsed() {
            // BLINKER_LOG("isParsed");
            isFresh = false; 
            canParse = false; 
            availState = false;
            isAvail = false;
            conn.flush();
        }// BLINKER_LOG("isParsed");

        bool parseState() { return canParse; }

        void printNow() {
            if (strlen(_sendBuf) && autoFormat) {
#if defined(ESP8266) || defined(ESP32)
                _print(STRING_format(_sendBuf));
#else
                _print("{" + STRING_format(_sendBuf) + "}");
#endif

                free(_sendBuf);
                autoFormat = false;
            }
        }

        void checkAutoFormat()
        {
            if (autoFormat) {
                if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT) {
                    if (strlen(_sendBuf)) {
#if defined(ESP8266) || defined(ESP32)
                        _print(STRING_format(_sendBuf));
#else
                        _print("{" + STRING_format(_sendBuf) + "}");
#endif
                    }
                    free(_sendBuf);
                    autoFormat = false;
                }
            }
        }

#if defined(BLINKER_AT_MQTT)
        bool serialAvailable()
        {
            if (conn.serialAvailable())
            {
                _slaverAT = new BlinkerSlaverAT();

                _slaverAT->update(conn.serialLastRead());
                
                BLINKER_LOG_ALL(BLINKER_F("state: "), _slaverAT->state());
                BLINKER_LOG_ALL(BLINKER_F("cmd: "), _slaverAT->cmd());
                BLINKER_LOG_ALL(BLINKER_F("paramNum: "), _slaverAT->paramNum());

                if (_slaverAT->state())
                {
                    parseATdata();

                    free(_slaverAT);

                    return false;
                }

                free(_slaverAT);

                return true;
            }
            else
            {
                return false;
            }
        }

        void serialPrint(const String & s)
        {
            conn.serialPrint(s);
        }

        void atHeartbeat()
        {
            conn.serialPrint(conn.lastRead());
        }
#endif

    protected :
// #if defined(BLINKER_MQTT_AT)
//         char            _authKey[BLINKER_AUTHKEY_SIZE];
//         char            _deviceName[BLINKER_MQTT_DEVICEID_SIZE];
// #endif

#if defined(BLINKER_AT_MQTT)
        blinker_at_status_t     _status = BL_BEGIN;
        blinker_at_aligenie_t   _aliType = ALI_NONE;
        uint8_t                 _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
        uint8_t                 pinDataNum = 0;
        // bool                    _isAtRegister = false;
#endif

        Transp&         conn;
        BlinkerState    state;
        bool            isFresh;
        bool            isAvail;
        bool            availState;
        bool            canParse;
        // bool            isFormat;
        bool            autoFormat = false;
        uint32_t        autoFormatFreshTime;
// #if defined(BLINKER_MQTT_AT)
//         bool            _isFreshBuf = false;
        char*           _sendBuf;
// #else
//         char            _sendBuf[BLINKER_MAX_SEND_BUFFER_SIZE];
// #endif

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        bool            isBridgeFresh = false;
        bool            isExtraAvail = false;
        bool            isBridgeAvail = false;
        bool            isBformat = false;
        bool            autoBFormat = false;
        char            _bSendBuf[BLINKER_MAX_SEND_BUFFER_SIZE];
        String          _bridgeKey;
        String          _bKey_forwhile;
#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
        bool            _isAuto = false;
        bool            _isAutoInit = false;
#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
        char            _authKey[BLINKER_AUTHKEY_SIZE];
        char            _deviceName[BLINKER_MQTT_DEVICEID_SIZE];
        bool            _isInit = false;
        uint8_t         _disconnectCount = 0;
        uint32_t        _disFreshTime = 0;
        uint32_t        _disconnectTime = 0;
        uint32_t        _refreshTime = 0;
#endif

#if defined(BLINKER_PRO)
        char            _authKey[BLINKER_MQTT_KEY_SIZE];
        char            _deviceName[BLINKER_MQTT_DEVICEID_SIZE];
        
        bool            _isConnBegin = false;
        bool            _getRegister = false;
        bool            _isInit = false;

        bool            _isRegistered = false;
        uint32_t        _register_fresh = 0;

        uint32_t        _initTime;

        uint8_t         _proStatus = PRO_WLAN_CONNECTING;
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
        uint32_t        _reconTime = 0;
#endif

#if defined(BLINKER_PRO)
        bool beginPro() {
            return BApi::wlanRun();
        }
#endif

        void begin()
        {
            BLINKER_LOG(BLINKER_F(""));
        #if defined(BLINKER_NO_LOGO)
            BLINKER_LOG(BLINKER_F("Blinker v"BLINKER_VERSION"\n"
                        "    Give Blinker a Github star, thanks!\n"
                        "    => https://github.com/blinker-iot/blinker-library\n"));
        #elif defined(BLINKER_LOGO_3D)
            BLINKER_LOG(BLINKER_F("\n"
                " ____    ___                __                       \n"
                "/\\  _`\\ /\\_ \\    __        /\\ \\               v"BLINKER_VERSION"\n"
                "\\ \\ \\L\\ \\//\\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"
                " \\ \\  _ <'\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"
                "  \\ \\ \\L\\ \\\\_\\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/  \n"
                "   \\ \\____//\\____\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"
                "    \\/___/ \\/____/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"
                "   Give Blinker a Github star, thanks!\n"
                "   => https://github.com/blinker-iot/blinker-library\n"));

            // BLINKER_LOG(("Give Blinker a github star, thanks!"));
            // BLINKER_LOG(("=> https://github.com/blinker-iot/blinker-library"));
        #else
            BLINKER_LOG(BLINKER_F("\n"
                "   ___  ___      __    v"BLINKER_VERSION"\n"
                "  / _ )/ (_)__  / /_____ ____\n"
                " / _  / / / _ \\/  '_/ -_) __/\n"
                "/____/_/_/_//_/_/\\_\\\\__/_/   \n"
                "Give Blinker a github star, thanks!\n"
                "=> https://github.com/blinker-iot/blinker-library\n"));

            // BLINKER_LOG(("Give Blinker a github star, thanks!"));
            // BLINKER_LOG(("=> https://github.com/blinker-iot/blinker-library"));
        #endif

// #if defined(ESP8266) || defined(ESP32)
//             BApi::loadTimer();
// #endif
        }

// #if defined(BLINKER_AT_MQTT)
//         void atBegin(const char* _auth)
//         {
//             strcpy(_authKey, _auth);
//         }
// #endif

#if defined(BLINKER_MQTT)
        void begin(const char* _auth)
        {
            begin();
            strcpy(_authKey, _auth);
        }
#endif

#if defined(BLINKER_PRO)
        void begin(const char* _type)
        {
            begin();

            BLINKER_LOG(BLINKER_F(
                        "\n==========================================================="
                        "\n================= Blinker PRO mode init ! ================="
                        "\nWarning! EEPROM address 1280-1535 is used for Auto Control!"
                        "\n============= DON'T USE THESE EEPROM ADDRESS! ============="
                        "\n===========================================================\n"));

            BLINKER_LOG(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

    #if defined(BLINKER_BUTTON)
        #if defined(BLINKER_BUTTON_PULLDOWN)
            BApi::buttonInit(false);
        #else
            BApi::buttonInit();
        #endif
    #endif
            BApi::setType(_type);
        }
#endif

#if defined(BLINKER_AT_MQTT)
        void atBegin()
        {
            while (_status == BL_BEGIN)
            {
                serialAvailable();
            }
        }
#endif

#if defined(BLINKER_MQTT_AT)
        void atInit(const char* _auth)
        {
            // strcpy(_authKey, _auth);

            BApi::initCheck(STRING_format(_auth));
        }

        void atInit(const char* _auth,
                    const char* _ssid,
                    const char* _pswd)
        {
            // strcpy(_authKey, _auth);

            String init_data =  STRING_format(_auth) + "," + \
                                STRING_format(_ssid) + "," + \
                                STRING_format(_pswd);

            BApi::initCheck(init_data);
        }
#endif

        // template <typename T>
        // void _print(T n, bool needParse = true, bool needCheckLength = true) {
        //     String data = STRING_format(n) + BLINKER_CMD_NEWLINE;

        //     BLINKER_LOG(BLINKER_F("data: "), data);
        //     BLINKER_LOG(BLINKER_F("n: "), n);

        //     if (data.length() <= BLINKER_MAX_SEND_SIZE || !needCheckLength) {
        //         conn.print(data);
        //         // if (needParse) {
        //         //     BApi::parse(data, true);
        //         // }
        //     }
        //     else {
        //         BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
        //     }
        // }

        void _print(const String & n, bool needParse = true, bool needCheckLength = true) {
            // String data = n + BLINKER_CMD_NEWLINE;
            BLINKER_LOG_ALL(BLINKER_F("print: "), n);
            
            if (n.length() <= BLINKER_MAX_SEND_SIZE || !needCheckLength) {
                conn.print(n);
                // if (needParse) {
                //     BApi::parse(data, true);
                // }
            }
            else {
                BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
            }
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        template <typename T>
        void _bPrint(const String & bKey, T n) {
            String _bName = BApi::bridgeFind(bKey);
            String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
            if (_bName.length() > 0) {
                if (data.length() <= BLINKER_MAX_SEND_SIZE) {
                    conn.bPrint(_bName, data);
                    // if (needParse) {
                    //     BApi::parse(data, true);
                    // }
                }
                else {
                    BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
                }
            }
            else {
                BLINKER_ERR_LOG(BLINKER_F("MAKE SURE THE BRIDGE DEVICE IS REGISTERED!"));
            }
        }

        bool autoTrigged(uint32_t _id) {
            BLINKER_LOG_ALL(BLINKER_F("autoTrigged"));
            
            return conn.autoPrint(_id);
        }

        bool autoTrigged(char *name, char *type, char *data) {
            BLINKER_LOG(BLINKER_F("autoTrigged"));
            
            if (conn.deviceName() == name) {
                BApi::parse(data, true);
                return true;
            }
            return conn.autoPrint(name, type, data);
        }

        bool autoTrigged(char *name1, char *type1, char *data1
            , char *name2, char *type2, char *data2) {
            BLINKER_LOG(BLINKER_F("autoTrigged"));
            
            bool _link1 = false;
            bool _link2 = false;

            if (conn.deviceName() == name1) {
                BApi::parse(data1, true);
                _link1 = true;
            }
            if (conn.deviceName() == name2) {
                BApi::parse(data2, true);
                _link2 = true;
            }
            if (_link1 && _link2) {
                return conn.autoPrint(name1, type1, data1, name2, type2, data2);
            }
            else if (_link1) {
                return conn.autoPrint(name2, type2, data2);
            }
            else if (_link2) {
                return conn.autoPrint(name1, type1, data1);
            }
        }
#endif

#if defined(BLINKER_AT_MQTT)
        void parseATdata()
        {
            String reqData;
            
            if (_slaverAT->cmd() == BLINKER_CMD_AT) {
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_RST) {
                conn.serialPrint(BLINKER_CMD_OK);
                ::delay(100);
                ESP.restart();
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_GMR) {
                // reqData = "+" + STRING_format(BLINKER_CMD_GMR) + \
                //         "=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>" + \
                //         "[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]";
                conn.serialPrint(BLINKER_ESP_AT_VERSION);
                conn.serialPrint(BLINKER_VERSION);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_UART_CUR) {
                blinker_at_state_t at_state = _slaverAT->state();

                BLINKER_LOG(at_state);

                switch (at_state)
                {
                    case AT_NONE:
                        // conn.serialPrint();
                        break;
                    case AT_TEST:
                        reqData = STRING_format(BLINKER_CMD_AT) + \
                                "+" + STRING_format(BLINKER_CMD_UART_CUR) + \
                                "=<baudrate>,<databits>,<stopbits>,<parity>";
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_QUERY:
                        reqData = "+" + STRING_format(BLINKER_CMD_UART_CUR) + \
                                ":" + STRING_format(serialSet >> 8 & 0x00FFFFFF) + \
                                "," + STRING_format(serialSet >> 4 & 0x0F) + \
                                "," + STRING_format(serialSet >> 2 & 0x03) + \
                                "," + STRING_format(serialSet      & 0x03);
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_SETTING:
                        BLINKER_LOG_ALL(BLINKER_F("SER_BAUD: "), _slaverAT->getParam(SER_BAUD));
                        BLINKER_LOG_ALL(BLINKER_F("SER_DBIT: "), _slaverAT->getParam(SER_DBIT));
                        BLINKER_LOG_ALL(BLINKER_F("SER_SBIT: "), _slaverAT->getParam(SER_SBIT));
                        BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), _slaverAT->getParam(SER_PRIT));
                        if (BLINKER_UART_PARAM_NUM != _slaverAT->paramNum()) return;

                        serialSet = (_slaverAT->getParam(SER_BAUD)).toInt() << 8 |
                                    (_slaverAT->getParam(SER_DBIT)).toInt() << 4 |
                                    (_slaverAT->getParam(SER_SBIT)).toInt() << 2 |
                                    (_slaverAT->getParam(SER_PRIT)).toInt();

                        ss_cfg = serConfig();

                        BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), serialSet);

                        conn.serialPrint(BLINKER_CMD_OK);

                        // if (isHWS) {
                            Serial.begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
                        // }
                        // else {
                        //     SSerialBLE->begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
                        // }
                        break;
                    case AT_ACTION:
                        // conn.serialPrint();
                        break;
                    default :
                        break;
                }
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_UART_DEF) {
                blinker_at_state_t at_state = _slaverAT->state();

                BLINKER_LOG(at_state);

                switch (at_state)
                {
                    case AT_NONE:
                        // conn.serialPrint();
                        break;
                    case AT_TEST:
                        reqData = STRING_format(BLINKER_CMD_AT) + \
                                "+" + STRING_format(BLINKER_CMD_UART_DEF) + \
                                "=<baudrate>,<databits>,<stopbits>,<parity>";
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_QUERY:
                        reqData = "+" + STRING_format(BLINKER_CMD_UART_DEF) + \
                                ":" + STRING_format(serialSet >> 8 & 0x00FFFFFF) + \
                                "," + STRING_format(serialSet >> 4 & 0x0F) + \
                                "," + STRING_format(serialSet >> 2 & 0x03) + \
                                "," + STRING_format(serialSet      & 0x03);
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_SETTING:
                        BLINKER_LOG_ALL(BLINKER_F("SER_BAUD: "), _slaverAT->getParam(SER_BAUD));
                        BLINKER_LOG_ALL(BLINKER_F("SER_DBIT: "), _slaverAT->getParam(SER_DBIT));
                        BLINKER_LOG_ALL(BLINKER_F("SER_SBIT: "), _slaverAT->getParam(SER_SBIT));
                        BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), _slaverAT->getParam(SER_PRIT));
                        
                        if (BLINKER_UART_PARAM_NUM != _slaverAT->paramNum()) return;

                        serialSet = (_slaverAT->getParam(SER_BAUD)).toInt() << 8 |
                                    (_slaverAT->getParam(SER_DBIT)).toInt() << 4 |
                                    (_slaverAT->getParam(SER_SBIT)).toInt() << 2 |
                                    (_slaverAT->getParam(SER_PRIT)).toInt();

                        ss_cfg = serConfig();

                        BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), serialSet);

                        conn.serialPrint(BLINKER_CMD_OK);

                        // if (isHWS) {
                            Serial.begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
                        // }
                        // else {
                        //     SSerialBLE->begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
                        // }

                        EEPROM.begin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_SERIALCFG, serialSet);
                        EEPROM.commit();
                        EEPROM.end();
                        break;
                    case AT_ACTION:
                        // conn.serialPrint();
                        break;
                    default :
                        break;
                }
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_RAM && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_RAM) + \
                        ":" + STRING_format(BLINKER_FreeHeap());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_ADC && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_ADC) + \
                        ":" + STRING_format(analogRead(A0));
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_IOSETCFG && _slaverAT->state() == AT_SETTING) {
                BLINKER_LOG_ALL(BLINKER_F("PIN_SET: "), _slaverAT->getParam(PIN_SET));
                BLINKER_LOG_ALL(BLINKER_F("PIN_MODE: "), _slaverAT->getParam(PIN_MODE));
                BLINKER_LOG_ALL(BLINKER_F("PIN_PULLSTATE: "), _slaverAT->getParam(PIN_PULLSTATE));

                if (BLINKER_IOSETCFG_PARAM_NUM != _slaverAT->paramNum()) return;

                uint8_t set_pin = (_slaverAT->getParam(PIN_SET)).toInt();
                uint8_t set_mode = (_slaverAT->getParam(PIN_MODE)).toInt();
                uint8_t set_pull = (_slaverAT->getParam(PIN_PULLSTATE)).toInt();

                if (set_pin >= BLINKER_MAX_PIN_NUM || 
                    set_mode > BLINKER_IO_OUTPUT_NUM ||
                    set_pull > 2)
                {
                    return;
                }

                if (pinDataNum == 0) {
                    _pinData[pinDataNum] = new PinData(set_pin, set_mode, set_pull);
                    pinDataNum++;
                }
                else {
                    bool _isSet = false;
                    for (uint8_t _num = 0; _num < pinDataNum; _num++)
                    {
                        if (_pinData[_num]->checkPin(set_pin))
                        {
                            _isSet = true;
                            _pinData[_num]->fresh(set_mode, set_pull);
                        }
                    }
                    if (!_isSet) {
                        _pinData[pinDataNum] = new PinData(set_pin, set_mode, set_pull);
                        pinDataNum++;
                    }
                }
                
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_IOGETCFG && _slaverAT->state() == AT_SETTING) {
                BLINKER_LOG_ALL(BLINKER_F("PIN_SET: "), _slaverAT->getParam(PIN_SET));

                if (BLINKER_IOGETCFG_PARAM_NUM != _slaverAT->paramNum()) return;

                uint8_t set_pin = (_slaverAT->getParam(PIN_SET)).toInt();

                if (set_pin >= BLINKER_MAX_PIN_NUM) return;

                bool _isGet = false;
                for (uint8_t _num = 0; _num < pinDataNum; _num++)
                {
                    if (_pinData[_num]->checkPin(set_pin))
                    {
                        _isGet = true;
                        reqData = "+" + STRING_format(BLINKER_CMD_IOGETCFG) + \
                                ":" + _pinData[_num]->data();
                        conn.serialPrint(reqData);
                    }
                }
                if (!_isGet) {
                    reqData = "+" + STRING_format(BLINKER_CMD_IOGETCFG) + \
                            ":" + _slaverAT->getParam(PIN_SET) + \
                            ",2,0";
                    conn.serialPrint(reqData);
                }

                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_GPIOWRITE && _slaverAT->state() == AT_SETTING) {
                BLINKER_LOG_ALL(BLINKER_F("IO_PIN: "), _slaverAT->getParam(IO_PIN));
                BLINKER_LOG_ALL(BLINKER_F("IO_LVL: "),  _slaverAT->getParam(IO_LVL));

                if (BLINKER_GPIOWRITE_PARAM_NUM != _slaverAT->paramNum()) return;

                uint8_t set_pin = (_slaverAT->getParam(IO_PIN)).toInt();
                uint8_t set_lvl = (_slaverAT->getParam(IO_LVL)).toInt();

                if (set_pin >= BLINKER_MAX_PIN_NUM) return;

                // bool _isSet = false;
                for (uint8_t _num = 0; _num < pinDataNum; _num++)
                {
                    if (_pinData[_num]->checkPin(set_pin))
                    {
                        if (_pinData[_num]->getMode() == BLINKER_IO_OUTPUT_NUM)
                        {
                            if (set_lvl <= 1) {
                                digitalWrite(set_pin, set_lvl ? HIGH : LOW);
                                conn.serialPrint(BLINKER_CMD_OK);
                                return;
                            }
                        }
                    }
                }

                conn.serialPrint(BLINKER_CMD_ERROR);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_GPIOWREAD && _slaverAT->state() == AT_SETTING) {
                BLINKER_LOG_ALL(BLINKER_F("IO_PIN: "), _slaverAT->getParam(IO_PIN));

                if (BLINKER_GPIOREAD_PARAM_NUM != _slaverAT->paramNum()) return;

                uint8_t set_pin = (_slaverAT->getParam(IO_PIN)).toInt();

                if (set_pin >= BLINKER_MAX_PIN_NUM)
                {
                    conn.serialPrint(BLINKER_CMD_ERROR);
                    return;
                }

                // bool _isSet = false;
                for (uint8_t _num = 0; _num < pinDataNum; _num++)
                {
                    if (_pinData[_num]->checkPin(set_pin))
                    {
                        // if (_pinData[_num]->getMode() == BLINKER_IO_INPUT_NUM)
                        // {
                        //     if (set_lvl <= 1) {
                                reqData = "+" + STRING_format(BLINKER_CMD_GPIOWREAD) + \
                                        ":" + STRING_format(set_pin) + \
                                        "," + STRING_format(_pinData[_num]->getMode()) + \
                                        "," + STRING_format(digitalRead(set_pin));
                                conn.serialPrint(reqData);
                                conn.serialPrint(BLINKER_CMD_OK);
                                return;
                        //     }
                        // }
                    }
                }
                reqData = "+" + STRING_format(BLINKER_CMD_GPIOWREAD) + \
                        ":" + STRING_format(set_pin) + \
                        ",3," + STRING_format(digitalRead(set_pin));
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
                // conn.serialPrint(BLINKER_CMD_ERROR);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_MQTT) {
                // conn.serialPrint(BLINKER_CMD_OK);

                BLINKER_LOG(BLINKER_CMD_BLINKER_MQTT);

                blinker_at_state_t at_state = _slaverAT->state();

                BLINKER_LOG(at_state);

                switch (at_state)
                {
                    case AT_NONE:
                        // conn.serialPrint();
                        break;
                    case AT_TEST:
                        reqData = STRING_format(BLINKER_CMD_AT) + \
                                "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                "=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>" + \
                                "[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]";
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_QUERY:
                        reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                ":" + STRING_format(_wlanMode) + \
                                "," + STRING_format(_authKey) + \
                                "," + WiFi.SSID() + \
                                "," + WiFi.psk();
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_SETTING:
                        BLINKER_LOG_ALL(BLINKER_F("MQTT_CONFIG_MODE: "), _slaverAT->getParam(MQTT_CONFIG_MODE));
                        BLINKER_LOG_ALL(BLINKER_F("MQTT_AUTH_KEY: "),  _slaverAT->getParam(MQTT_AUTH_KEY));
                        BLINKER_LOG_ALL(BLINKER_F("MQTT_WIFI_SSID: "), _slaverAT->getParam(MQTT_WIFI_SSID));
                        BLINKER_LOG_ALL(BLINKER_F("MQTT_WIFI_PSWD: "), _slaverAT->getParam(MQTT_WIFI_PSWD));

                        if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_COMCONFIG_NUM)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("BLINKER_CMD_COMWLAN"));

                            if (BLINKER_COMWLAN_PARAM_NUM != _slaverAT->paramNum()) return;

                            if (_status == BL_INITED)
                            {
                                reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                        ":" + conn.deviceId() + \
                                        "," + conn.uuid();
                                conn.serialPrint(reqData);
                                conn.serialPrint(BLINKER_CMD_OK);
                                return;
                            }

                            conn.connectWiFi((_slaverAT->getParam(MQTT_WIFI_SSID)).c_str(), 
                                        (_slaverAT->getParam(MQTT_WIFI_PSWD)).c_str());
                            
                            conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                            _status = BL_INITED;
                            _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
                        }
                        else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_SMARTCONFIG_NUM)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("BLINKER_CMD_SMARTCONFIG"));

                            if (BLINKER_SMCFG_PARAM_NUM != _slaverAT->paramNum()) return;

                            if (_status == BL_INITED)
                            {
                                reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                        ":" + conn.deviceId() + \
                                        "," + conn.uuid();
                                conn.serialPrint(reqData);
                                conn.serialPrint(BLINKER_CMD_OK);
                                return;
                            }

                            if (!conn.autoInit()) conn.smartconfig();

                            conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                            _status = BL_INITED;
                            _wlanMode = BLINKER_CMD_SMARTCONFIG_NUM;
                        }
                        else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_APCONFIG_NUM)
                        {
                            BLINKER_LOG(BLINKER_F("BLINKER_CMD_APCONFIG"));

                            if (BLINKER_APCFG_PARAM_NUM != _slaverAT->paramNum()) return;

                            if (_status == BL_INITED)
                            {
                                reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                        ":" + conn.deviceId() + \
                                        "," + conn.uuid();
                                conn.serialPrint(reqData);
                                conn.serialPrint(BLINKER_CMD_OK);
                                return;
                            }

                            if (!conn.autoInit())
                            {
                                conn.softAPinit();
                                while(WiFi.status() != WL_CONNECTED) {
                                    conn.serverClient();

                                    ::delay(10);
                                }
                            }

                            conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                            _status = BL_INITED;
                            _wlanMode = BLINKER_CMD_APCONFIG_NUM;
                        }
                        else {
                            return;
                        }

                        reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                ":" + conn.deviceId() + \
                                "," + conn.uuid();
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_ACTION:
                        // conn.serialPrint();
                        break;
                    default :
                        break;
                }
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_ALIGENIE) {
                // conn.serialPrint(BLINKER_CMD_OK);

                BLINKER_LOG(BLINKER_CMD_BLINKER_ALIGENIE);

                blinker_at_state_t at_state = _slaverAT->state();

                BLINKER_LOG_ALL(at_state);

                switch (at_state)
                {
                    case AT_NONE:
                        // conn.serialPrint();
                        break;
                    case AT_TEST:
                        reqData = STRING_format(BLINKER_CMD_AT) + \
                                "+" + STRING_format(BLINKER_CMD_BLINKER_ALIGENIE) + \
                                "=<type>";
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_QUERY:
                        reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_ALIGENIE) + \
                                ":" + STRING_format(_aliType);
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_SETTING:
                        BLINKER_LOG_ALL(BLINKER_F("BLINKER_ALIGENIE_CFG_NUM: "), _slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM));

                        if (BLINKER_ALIGENIE_PARAM_NUM != _slaverAT->paramNum()) return;

                        if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_LIGHT)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("ALI_LIGHT"));
                            _aliType = ALI_LIGHT;
                        }
                        else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == ALI_OUTLET)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("ALI_OUTLET"));
                            _aliType = ALI_OUTLET;
                        }
                        else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == ALI_SENSOR)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("ALI_SENSOR"));
                            _aliType = ALI_SENSOR;
                        }
                        else {
                            BLINKER_LOG_ALL(BLINKER_F("ALI_NONE"));
                            _aliType = ALI_NONE;
                        }
                        conn.aligenieType(_aliType);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_ACTION:
                        // conn.serialPrint();
                        break;
                    default :
                        break;
                }
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_TIMEZONE) {

                BLINKER_LOG(BLINKER_CMD_TIMEZONE);

                blinker_at_state_t at_state = _slaverAT->state();

                BLINKER_LOG(at_state);

                switch (at_state)
                {
                    case AT_NONE:
                        // conn.serialPrint();
                        break;
                    case AT_TEST:
                        reqData = STRING_format(BLINKER_CMD_AT) + \
                                "+" + STRING_format(BLINKER_CMD_TIMEZONE) + \
                                "=<TIMEZONE>";
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_QUERY:
                        reqData = "+" + STRING_format(BLINKER_CMD_BLINKER_MQTT) + \
                                ":" + STRING_format(BApi::getTimezone());
                        conn.serialPrint(reqData);
                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_SETTING:
                        BLINKER_LOG_ALL(BLINKER_F("BLINKER_TIMEZONE_CFG_NUM: "), _slaverAT->getParam(BLINKER_TIMEZONE_CFG_NUM));

                        if (BLINKER_TIMEZONE_PARAM_NUM != _slaverAT->paramNum()) return;

                        BApi::setTimezone((_slaverAT->getParam(BLINKER_TIMEZONE_CFG_NUM)).toFloat());

                        conn.serialPrint(BLINKER_CMD_OK);
                        break;
                    case AT_ACTION:
                        // conn.serialPrint();
                        break;
                    default :
                        break;
                }
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_TIME_AT && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_TIME_AT) + \
                        ":" + STRING_format(BApi::time());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_SECOND && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_SECOND) + \
                        ":" + STRING_format(BApi::second());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_SECOND && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_SECOND) + \
                        ":" + STRING_format(BApi::second());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_MINUTE && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_MINUTE) + \
                        ":" + STRING_format(BApi::minute());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_HOUR && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_HOUR) + \
                        ":" + STRING_format(BApi::hour());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_WDAY && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_WDAY) + \
                        ":" + STRING_format(BApi::wday());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_MDAY && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_MDAY) + \
                        ":" + STRING_format(BApi::mday());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_YDAY && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_YDAY) + \
                        ":" + STRING_format(BApi::yday());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_MONTH && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_MONTH) + \
                        ":" + STRING_format(BApi::month());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_YEAR && _slaverAT->state() == AT_QUERY) {
                reqData = "+" + STRING_format(BLINKER_CMD_YEAR) + \
                        ":" + STRING_format(BApi::year());
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_WEATHER_AT && _slaverAT->state() == AT_SETTING) {
                if (1 != _slaverAT->paramNum()) return;
                
                reqData = "+" + STRING_format(BLINKER_CMD_WEATHER_AT) + \
                        ":" + STRING_format(BApi::weather(_slaverAT->getParam(0)));
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_AQI_AT && _slaverAT->state() == AT_SETTING) {
                if (1 != _slaverAT->paramNum()) return;
                
                reqData = "+" + STRING_format(BLINKER_CMD_AQI_AT) + \
                        ":" + STRING_format(BApi::aqi(_slaverAT->getParam(0)));
                
                conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_NOTICE_AT && _slaverAT->state() == AT_SETTING) {
                if (1 != _slaverAT->paramNum()) return;
                
                // reqData = "+" + STRING_format(BLINKER_CMD_NOTICE_AT) + \
                //         ":" + STRING_format(BApi::aqi(_slaverAT->getParam(0)));
                notify(_slaverAT->getParam(0));
                // conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
            else if (_slaverAT->cmd() == BLINKER_CMD_SMS_AT && _slaverAT->state() == AT_SETTING) {
                if (1 != _slaverAT->paramNum()) return;
                
                // reqData = "+" + STRING_format(BLINKER_CMD_NOTICE_AT) + \
                //         ":" + STRING_format(BApi::aqi(_slaverAT->getParam(0)));
                BApi::sms(_slaverAT->getParam(0));
                // conn.serialPrint(reqData);
                conn.serialPrint(BLINKER_CMD_OK);
            }
        }

        String getMode(uint8_t mode)
        {
            switch (mode)
            {
                case BLINKER_CMD_COMCONFIG_NUM :
                    return BLINKER_CMD_COMCONFIG;
                case BLINKER_CMD_SMARTCONFIG_NUM :
                    return BLINKER_CMD_SMARTCONFIG;
                case BLINKER_CMD_APCONFIG_NUM :
                    return BLINKER_CMD_APCONFIG;
                default :
                    return BLINKER_CMD_COMCONFIG;
            }
        }
#endif
};

template <class Transp>
void BlinkerProtocol<Transp>::run()
{
#if defined(BLINKER_PRO)

    if (!BApi::wlanRun()) {
        uint8_t wl_status = BApi::wlanStatus();
        
        if (wl_status == BWL_SMARTCONFIG_BEGIN) {
            _proStatus = PRO_WLAN_SMARTCONFIG_BEGIN;
        }
        else if (wl_status == BWL_SMARTCONFIG_DONE) {
            _proStatus = PRO_WLAN_SMARTCONFIG_DONE;
        }
        else {
            _proStatus = PRO_WLAN_CONNECTING;
        }
        return;
    }
    else {
        if (!_isConnBegin) {
            _proStatus = PRO_WLAN_CONNECTED;

            conn.begin(BApi::type());
            _isConnBegin = true;
            _initTime = millis();
            
            BLINKER_LOG_ALL(BLINKER_F("conn begin, fresh _initTime: "), _initTime);

            if (conn.authCheck()) {
                _proStatus = PRO_DEV_AUTHCHECK_SUCCESS;

                BLINKER_LOG_ALL(BLINKER_F("is auth, conn deviceRegister"));

                _isRegistered = conn.deviceRegister();
                _getRegister = true;

                if (!_isRegistered) {
                    _register_fresh = millis();

                    _proStatus = PRO_DEV_REGISTER_FAIL;
                }
                else {
                    _proStatus = PRO_DEV_REGISTER_SUCCESS;
                }
            }
            else {
                _proStatus = PRO_DEV_AUTHCHECK_FAIL;

                BLINKER_LOG_ALL(BLINKER_F("not auth, conn deviceRegister"));
            }
        }
    }

    // if (_getRegister) {
    // #ifdef BLINKER_DEBUG_ALL
    //     BLINKER_LOG(BLINKER_F("conn deviceRegister"));
    // #endif
    //     _isRegistered = conn.deviceRegister();

    //     if (!_isRegistered) _register_fresh = millis();
    //     _getRegister = false;
    // }

    if (_getRegister) {
        if (!_isRegistered && ((millis() - _register_fresh) > 5000 || _register_fresh == 0)) {
            BLINKER_LOG_ALL(BLINKER_F("conn deviceRegister"));
            
            _isRegistered = conn.deviceRegister();

            if (!_isRegistered) {
                _register_fresh = millis();
                _proStatus = PRO_DEV_REGISTER_FAIL;
            }
            else {
                _isRegistered = true;
                _proStatus = PRO_DEV_REGISTER_SUCCESS;
            }
        }
    }
    
    if (!conn.init()) {
        if ((millis() - _initTime) >= BLINKER_CHECK_AUTH_TIME && !_getRegister) {
            BApi::reset();
        }
    }
    else {
        if (!_isInit) {
            if (BApi::ntpInit()) {
                _isInit = true;
                strcpy(_authKey, conn.key().c_str());
                strcpy(_deviceName, conn.deviceName().c_str());
                _proStatus = PRO_DEV_INIT_SUCCESS;
            }
        }
        else {
            if (state == CONNECTING && _proStatus != PRO_DEV_CONNECTING) {
                _proStatus = PRO_DEV_CONNECTING;
            }
            else if (state == CONNECTED && _proStatus != PRO_DEV_CONNECTED) {
                if (conn.mConnected()) _proStatus = PRO_DEV_CONNECTED;
            }
            else if (state == DISCONNECTED && _proStatus != PRO_DEV_DISCONNECTED) {
                _proStatus = PRO_DEV_DISCONNECTED;
            }
        }
    }

#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_PRO)
// #ifdef BLINKER_DEBUG_ALL
//     BLINKER_LOG(BLINKER_F("check ntp init"));
// #endif
    BApi::ntpInit();
// #endif
// #if defined(ESP8266) || defined(ESP32)
    BApi::checkTimer();
#endif

#if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
    BApi::checkTimer();

    if (!_isInit) {
        if (conn.init() && BApi::ntpInit()) {
            _isInit =true;
            _disconnectTime = millis();
            
            BLINKER_LOG_ALL(BLINKER_F("MQTT conn init success"));
        }
    }
    else {
        if (((millis() - _disconnectTime) > 60000 && _disconnectCount) 
            || _disconnectCount >= 12) {
        // if (_disconnectCount >= 12) {
            BLINKER_LOG_ALL(BLINKER_F("device reRegister"));
            BLINKER_LOG_FreeHeap();
            
            if (BLINKER_FreeHeap() < 15000) {
                conn.disconnect();
                return;
            }

            BLINKER_LOG_FreeHeap();

            if (conn.reRegister()) {
                _disconnectCount = 0;
                _disconnectTime = millis();
            }
            else {
                _disconnectCount = 0;
                _disconnectTime = millis() - 10000;
            }
        }

        BApi::ntpInit();
    }

    if ((millis() - _refreshTime) >= BLINKER_ONE_DAY_TIME * 2 * 1000) {
    // if ((millis() - _refreshTime) >= 120000) {
        conn.disconnect();

        BLINKER_LOG_ALL(BLINKER_F("device reRegister"));
        BLINKER_LOG_FreeHeap();

        if (BLINKER_FreeHeap() < 15000) {
            conn.disconnect();
            return;
        }

        BLINKER_LOG_FreeHeap();

        if (conn.reRegister()) {
            _refreshTime = millis();
        }
    }
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
    if (WiFi.status() != WL_CONNECTED) {
        if (_reconTime == 0) {
            _reconTime = millis();

            BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));

            WiFi.reconnect();
        }

        // if ((millis() > _reconTime) && (millis() - _reconTime) >= 10000) {
        if ((millis() - _reconTime) >= 10000) {
            _reconTime = millis();

            BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));

            WiFi.reconnect();
        }

        return;
    }
#endif

// #if defined(BLINKER_MQTT)
//     // if ((millis() - _refreshTime) >= BLINKER_ONE_DAY_TIME * 2 * 1000) {
//     if ((millis() - _refreshTime) >= 120000) {
//         conn.begin(_authKey);

//         _refreshTime += BLINKER_ONE_DAY_TIME * 2 * 1000;
//     }
// #endif

    bool conState = conn.connected();

    switch (state)
    {
        case CONNECTING :
            if (conn.connect()) {
// #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
//                 if (conn.mConnected()) state = CONNECTED;
// #else
                state = CONNECTED;
// #endif
#if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
                _disconnectCount = 0;
#endif
            }
            else {
#if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
                if (_isInit) {
                    if (_disconnectCount == 0) {
                        _disconnectCount++;
                        _disconnectTime = millis();
                        _disFreshTime = millis();
                    }
                    else {
                        // if ((millis() > _disFreshTime) && (millis() - _disFreshTime) >= 5000) {
                        if ((millis() - _disFreshTime) >= 5000) {
                            _disFreshTime = millis();
                            _disconnectCount++;

                            if (_disconnectCount > 12) _disconnectCount = 12;

                            BLINKER_LOG_ALL(BLINKER_F("_disFreshTime: "), _disFreshTime);
                            BLINKER_LOG_ALL(BLINKER_F("_disconnectCount: "), _disconnectCount);
                        }
                    }
                }
#endif   
            }
            break;
        case CONNECTED :
            if (conState)
            {
                checkAvail();
                if (isAvail)
                {
                    BApi::parse(dataParse());
                }
    #if defined(BLINKER_MQTT_AT) && defined(BLINKER_ALIGENIE)
                if (isAvail)
                {
                    BApi::aliParse(conn.lastRead());

                    if (STRING_contains_string(conn.lastRead(), "vAssistant"))
                    {
                        flush();
                    }
                }
    #endif
#if defined(BLINKER_AT_MQTT)
                if (isAvail)
                {
                    // BLINKER_LOG_ALL("isAvail");
                    conn.serialPrint(conn.lastRead());
                }

                if (serialAvailable())
                {
                    conn.mqttPrint(conn.serialLastRead());
                }
#endif

#if (defined(BLINKER_MQTT) || defined(BLINKER_PRO)) && defined(BLINKER_ALIGENIE)
                if (checkAliAvail())
                {
                    BApi::aliParse(conn.lastRead());
                }
#endif
            }
            else
            {
                // state = DISCONNECTED;
                conn.disconnect();
                state = CONNECTING;
#if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
                if (_isInit) {
                    if (_disconnectCount == 0)
                    {
                        _disconnectCount++;
                        _disconnectTime = millis();
                        _disFreshTime = millis();
                    }
                    else
                    {
                        // if ((millis() > _disFreshTime) && (millis() - _disFreshTime) >= 5000) {
                        if ((millis() - _disFreshTime) >= 5000)
                        {
                            _disFreshTime = millis();
                            _disconnectCount++;

                            if (_disconnectCount > 12) _disconnectCount = 12;
                            
                            BLINKER_LOG_ALL(BLINKER_F("_disFreshTime: "), _disFreshTime);
                            BLINKER_LOG_ALL(BLINKER_F("_disconnectCount: "), _disconnectCount);
                        }
                    }
                }
#endif
            }
            break;
        case DISCONNECTED :
            conn.disconnect();
            state = CONNECTING;
            break;
    }
    
#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
    if (_isAuto && _isInit && state == CONNECTED && !_isAutoInit) {
        if (BApi::autoPull()) _isAutoInit = true;
    }
#endif
    // if (autoFormat) 
    checkAutoFormat();
}

#endif