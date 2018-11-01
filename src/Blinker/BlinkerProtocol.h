#ifndef BlinkerProtocol_H
#define BlinkerProtocol_H

#include <Blinker/BlinkerApi.h>

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

                // BLINKER_LOG2("bridgeAvailable b_name: ", b_name);

                if (b_name.length() > 0) {
                    // _bKey_forwhile = b_name;
                    String b_data = conn.lastRead();

                    // BLINKER_LOG2("bridgeAvailable b_data: ", b_data);

                    DynamicJsonBuffer jsonBuffer;
                    JsonObject& extra_data = jsonBuffer.parseObject(b_data);

                    String _from = extra_data["fromDevice"];

                    // BLINKER_LOG2("bridgeAvailable _from: ", _from);

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
                return conn.lastRead();
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
//                 BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
//             }
//         }
// #else
//         void print(T n) {
//             String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
//             if (data.length() <= BLINKER_MAX_SEND_BUFFER_SIZE) {
//                 conn.print(data);
//             }
//             else {
//                 BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
//             BLINKER_LOG2(BLINKER_F("printArray _msg: "), "\"" + STRING_format(n1) + "\":" + s2);
// #endif
            // if (isFormat) {
            //     formatData("\"" + STRING_format(n1) + "\":" + s2);
            // }
            // else {
            //     _print("{" + _msg + "}");
            // }

                if (!autoFormat) {
                    autoFormat = true;
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
//                 BLINKER_ERR_LOG1("data is not a JSON!");
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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
                    memset(_sendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
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

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)

        void beginAuto() {
            BLINKER_LOG1(BLINKER_F("======================================================="));
            BLINKER_LOG1(BLINKER_F("=========== Blinker Auto Control mode init! ==========="));
            BLINKER_LOG1(BLINKER_F("Warning!EEPROM address 0-1279 is used for Auto Control!"));
            BLINKER_LOG1(BLINKER_F("=========== DON'T USE THESE EEPROM ADDRESS! ==========="));
            BLINKER_LOG1(BLINKER_F("======================================================="));

            BLINKER_LOG2(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

            _isAuto = true;
            // deserialization();
            // autoStart();
            BApi::autoInit();
        }

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

#if (defined(BLINKER_MQTT) || defined(BLINKER_PRO)) && defined(BLINKER_ALIGENIE)
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
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("response to AliGenie: "), _msg);
    #endif        

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
            isFresh = false;
            availState = false;
        }

#if defined(BLINKER_PRO)
        bool init() { return _isInit;}

        bool registered() { return conn.authCheck(); }

        uint8_t status() { return _proStatus; }
#endif

    private :
//         void formatData(String data) {
// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG2(BLINKER_F("formatData data: "), data);
// #endif
//             if ((strlen(_sendBuf) + data.length()) >= BLINKER_MAX_SEND_SIZE) {
//                 BLINKER_ERR_LOG1("FORMAT DATA SIZE IS MAX THAN LIMIT");
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

#if defined(ESP8266) || defined(ESP32)
        void autoFormatData(String key, String jsonValue) {
            // String _value = STRING_format(value);
            // if ((strlen(_sendBuf) + key.length() + _value.length()) >= BLINKER_MAX_SEND_SIZE) {
            //     BLINKER_ERR_LOG1("FORMAT DATA SIZE IS MAX THAN LIMIT");
            //     return;
            // }

    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG4(BLINKER_F("autoFormatData key: "), key, BLINKER_F(", json: "), jsonValue);
    #endif

            String _data;

            if (STRING_contains_string(STRING_format(_sendBuf), key)) {

                DynamicJsonBuffer jsonSendBuffer;                

                if (strlen(_sendBuf)) {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("add"));
    #endif
                    JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_sendBuf));

                    if (root.containsKey(key)) {
                        root.remove(key);
                    }
                    root.printTo(_data);

                    _data = _data.substring(0, _data.length() - 1);

                    _data += "," + jsonValue + "}";
                }
                else {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("new"));
    #endif
                    _data = "{" + jsonValue + "}";
                }
            }
            else {
                _data = STRING_format(_sendBuf);

                if (_data.length()) {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("add."));
    #endif                    

                    _data = _data.substring(0, _data.length() - 1);

                    _data += "," + jsonValue + "}";
                }
                else {
    #ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG1(BLINKER_F("new."));
    #endif
                    _data = "{" + jsonValue + "}";
                }
            }

            if (strlen(_sendBuf) > BLINKER_MAX_SEND_SIZE) {
                BLINKER_ERR_LOG1(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
                return;
            }

            strcpy(_sendBuf, _data.c_str());
        }
#endif

        void autoFormatData(String data) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("autoFormatData data: "), data);
#endif
            if ((strlen(_sendBuf) + data.length()) >= BLINKER_MAX_SEND_SIZE) {
                BLINKER_ERR_LOG1(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
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

        void autoBridgeFormatData(String key, String jsonValue) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG4(BLINKER_F("autoBridgeFormatData key: "), key, BLINKER_F(", jsonValue: "), jsonValue);
    #endif

            DynamicJsonBuffer jsonSendBuffer;

            String _data;

            if (strlen(_bSendBuf)) {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("autoBridgeFormatData add"));
    #endif
                JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_bSendBuf));

                if (root.containsKey(key)) {
                    root.remove(key);
                }
                root.printTo(_data);

                _data = _data.substring(0, _data.length() - 1);

                _data += "," + jsonValue + "}";
            }
            else {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("autoBridgeFormatData new"));
    #endif
                _data = "{" + jsonValue + "}";
            }

            if (strlen(_bSendBuf) > BLINKER_MAX_SEND_SIZE) {
                BLINKER_ERR_LOG1(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
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
                // BLINKER_LOG1("isExtraAvail true");
            }
            return isExtraAvail;
        }
#endif

        bool checkAvail()
        {
            isAvail = conn.available();
            if (isAvail) {
                isFresh = true;
                canParse = true;
                availState = true;
            }
            return isAvail;
        }

        bool checkAliAvail()
        {
            return conn.aligenieAvail();
        }

        String dataParse()
        {
            if (canParse) {
                return conn.lastRead();
            }
            else {
                return "";
            }
        }

        void isParsed() { isFresh = false; canParse = false; availState = false; }// BLINKER_LOG1("isParsed");

        bool parseState() { return canParse; }

        void printNow() {
            if (strlen(_sendBuf) && autoFormat) {
#if defined(ESP8266) || defined(ESP32)
                        _print(STRING_format(_sendBuf));
#else
                        _print("{" + STRING_format(_sendBuf) + "}");
#endif
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

                    autoFormat = false;
                }
            }
        }

    protected :
        Transp&         conn;
        BlinkerState    state;
        bool            isFresh;
        bool            isAvail;
        bool            availState;
        bool            canParse;
        // bool            isFormat;
        bool            autoFormat = false;
        uint32_t        autoFormatFreshTime;
        char            _sendBuf[BLINKER_MAX_SEND_BUFFER_SIZE];
#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        bool            isBridgeFresh = false;
        bool            isExtraAvail = false;
        bool            isBridgeAvail = false;
        bool            isBformat = false;
        bool            autoBFormat = false;
        char            _bSendBuf[BLINKER_MAX_SEND_BUFFER_SIZE];
        String          _bridgeKey;
        String          _bKey_forwhile;

        bool            _isAuto = false;
        bool            _isAutoInit = false;
#endif

#if defined(BLINKER_MQTT)
        char            _authKey[BLINKER_AUTHKEY_SIZE];
        char            _deviceName[BLINKER_MQTT_DEVICEID_SIZE];
        bool            _isInit = false;
        uint8_t         _disconnectCount = 0;
        uint32_t        _disFreshTime = 0;
        uint32_t        _disconnectTime = 0;
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

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
        uint32_t        _reconTime = 0;
#endif

#if defined(BLINKER_PRO)
        bool beginPro() {
            return BApi::wlanRun();
        }
#endif

        void begin()
        {
            BLINKER_LOG1(BLINKER_F(""));
        #if defined(BLINKER_NO_LOGO)
            BLINKER_LOG1(BLINKER_F("Blinker v"BLINKER_VERSION"\n"
                        "    Give Blinker a Github star, thanks!\n"
                        "    => https://github.com/blinker-iot/blinker-library\n"));
        #elif defined(BLINKER_LOGO_3D)
            BLINKER_LOG1(BLINKER_F("\n"
                " ____    ___                __                       \n"
                "/\\  _`\\ /\\_ \\    __        /\\ \\               v"BLINKER_VERSION"\n"
                "\\ \\ \\L\\ \\//\\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"
                " \\ \\  _ <'\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"
                "  \\ \\ \\L\\ \\\\_\\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/  \n"
                "   \\ \\____//\\____\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"
                "    \\/___/ \\/____/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"
                "   Give Blinker a Github star, thanks!\n"
                "   => https://github.com/blinker-iot/blinker-library\n"));

            // BLINKER_LOG1(("Give Blinker a github star, thanks!"));
            // BLINKER_LOG1(("=> https://github.com/blinker-iot/blinker-library"));
        #else
            BLINKER_LOG1(BLINKER_F("\n"
                "   ___  ___      __    v"BLINKER_VERSION"\n"
                "  / _ )/ (_)__  / /_____ ____\n"
                " / _  / / / _ \\/  '_/ -_) __/\n"
                "/____/_/_/_//_/_/\\_\\\\__/_/   \n"
                "Give Blinker a github star, thanks!\n"
                "=> https://github.com/blinker-iot/blinker-library\n"));

            // BLINKER_LOG1(("Give Blinker a github star, thanks!"));
            // BLINKER_LOG1(("=> https://github.com/blinker-iot/blinker-library"));
        #endif

// #if defined(ESP8266) || defined(ESP32)
//             BApi::loadTimer();
// #endif
        }

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

            BLINKER_LOG1(BLINKER_F(
                        "\n==========================================================="
                        "\n================= Blinker PRO mode init ! ================="
                        "\nWarning! EEPROM address 1280-1535 is used for Auto Control!"
                        "\n============= DON'T USE THESE EEPROM ADDRESS! ============="
                        "\n===========================================================\n"));

            BLINKER_LOG2(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

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

        // template <typename T>
        // void _print(T n, bool needParse = true, bool needCheckLength = true) {
        //     String data = STRING_format(n) + BLINKER_CMD_NEWLINE;

        //     BLINKER_LOG2(BLINKER_F("data: "), data);
        //     BLINKER_LOG2(BLINKER_F("n: "), n);

        //     if (data.length() <= BLINKER_MAX_SEND_SIZE || !needCheckLength) {
        //         conn.print(data);
        //         // if (needParse) {
        //         //     BApi::parse(data, true);
        //         // }
        //     }
        //     else {
        //         BLINKER_ERR_LOG1(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
        //     }
        // }

        void _print(const String & n, bool needParse = true, bool needCheckLength = true) {
            // String data = n + BLINKER_CMD_NEWLINE;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("print: "), n);
#endif
            if (n.length() <= BLINKER_MAX_SEND_SIZE || !needCheckLength) {
                conn.print(n);
                // if (needParse) {
                //     BApi::parse(data, true);
                // }
            }
            else {
                BLINKER_ERR_LOG1(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
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
                    BLINKER_ERR_LOG1(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
                }
            }
            else {
                BLINKER_ERR_LOG1(BLINKER_F("MAKE SURE THE BRIDGE DEVICE IS REGISTERED!"));
            }
        }

        bool autoTrigged(uint32_t _id) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("autoTrigged"));
    #endif
            return conn.autoPrint(_id);
        }

        bool autoTrigged(char *name, char *type, char *data) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("autoTrigged"));
    #endif
            if (conn.deviceName() == name) {
                BApi::parse(data, true);
                return true;
            }
            return conn.autoPrint(name, type, data);
        }

        bool autoTrigged(char *name1, char *type1, char *data1
            , char *name2, char *type2, char *data2) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("autoTrigged"));
    #endif
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
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("conn begin, fresh _initTime: "), _initTime);
    #endif
            if (conn.authCheck()) {
                _proStatus = PRO_DEV_AUTHCHECK_SUCCESS;
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("is auth, conn deviceRegister"));
    #endif
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
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("not auth, conn deviceRegister"));
    #endif
            }
        }
    }

    // if (_getRegister) {
    // #ifdef BLINKER_DEBUG_ALL
    //     BLINKER_LOG1(BLINKER_F("conn deviceRegister"));
    // #endif
    //     _isRegistered = conn.deviceRegister();

    //     if (!_isRegistered) _register_fresh = millis();
    //     _getRegister = false;
    // }

    if (_getRegister) {
        if (!_isRegistered && ((millis() - _register_fresh) > 60000 || _register_fresh == 0)) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("conn deviceRegister"));
    #endif
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
//     BLINKER_LOG1(BLINKER_F("check ntp init"));
// #endif
    BApi::ntpInit();
// #endif
// #if defined(ESP8266) || defined(ESP32)
    BApi::checkTimer();
#endif

#if defined(BLINKER_MQTT)
    BApi::checkTimer();

    if (!_isInit) {
        if (conn.init() && BApi::ntpInit()) {
            _isInit =true;
            _disconnectTime = millis();
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("MQTT conn init success"));
    #endif
        }
    }
    else {
        if (((millis() - _disconnectTime) > 60000 && _disconnectCount) 
            || _disconnectCount >= 12) {
        // if (_disconnectCount >= 12) {
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1(BLINKER_F("device reRegister"));
    #endif
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
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
    if (WiFi.status() != WL_CONNECTED) {
        if (_reconTime == 0) {
            _reconTime = millis();

            BLINKER_LOG1(BLINKER_F("WiFi disconnected! reconnecting!"));

            WiFi.reconnect();
        }

        if ((millis() > _reconTime) && (millis() - _reconTime) >= 10000) {
            _reconTime += 10000;

            BLINKER_LOG1(BLINKER_F("WiFi disconnected! reconnecting!"));

            WiFi.reconnect();
        }

        return;
    }
#endif

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
#if defined(BLINKER_MQTT)
                _disconnectCount = 0;
#endif
            }
            else {
#if defined(BLINKER_MQTT)
                if (_isInit) {
                    if (_disconnectCount == 0) {
                        _disconnectCount++;
                        _disconnectTime = millis();
                        _disFreshTime = millis();
                    }
                    else {
                        if ((millis() > _disFreshTime) && (millis() - _disFreshTime) >= 5000) {
                            _disFreshTime += 5000;
                            _disconnectCount++;

                            if (_disconnectCount > 12) _disconnectCount = 12;
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("_disFreshTime: "), _disFreshTime);
                            BLINKER_LOG2(BLINKER_F("_disconnectCount: "), _disconnectCount);
    #endif
                        }
                    }
                }
#endif   
            }
            break;
        case CONNECTED :
            if (conState) {
                checkAvail();
                if (isAvail) {
                    BApi::parse(dataParse());
                }
#if (defined(BLINKER_MQTT) || defined(BLINKER_PRO)) && defined(BLINKER_ALIGENIE)
                if (checkAliAvail()) {
                    BApi::aliParse(conn.lastRead());
                }
#endif
            }
            else {
                // state = DISCONNECTED;
                conn.disconnect();
                state = CONNECTING;
#if defined(BLINKER_MQTT)
                if (_isInit) {
                    if (_disconnectCount == 0) {
                        _disconnectCount++;
                        _disconnectTime = millis();
                        _disFreshTime = millis();
                    }
                    else {
                        if ((millis() > _disFreshTime) && (millis() - _disFreshTime) >= 5000) {
                            _disFreshTime += 5000;
                            _disconnectCount++;

                            if (_disconnectCount > 12) _disconnectCount = 12;
    #ifdef BLINKER_DEBUG_ALL
                            BLINKER_LOG2(BLINKER_F("_disFreshTime: "), _disFreshTime);
                            BLINKER_LOG2(BLINKER_F("_disconnectCount: "), _disconnectCount);
    #endif
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
    
#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
    if (_isAuto && _isInit && state == CONNECTED && !_isAutoInit) {
        if (BApi::autoPull()) _isAutoInit = true;
    }
#endif
    // if (autoFormat) 
    checkAutoFormat();
}

#endif