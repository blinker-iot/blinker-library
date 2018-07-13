#ifndef BlinkerProtocol_H
#define BlinkerProtocol_H

#include <Blinker/BlinkerApi.h>

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
            , isFormat(false)
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
//             if (data.length() <= BLINKER_MAX_SEND_SIZE) {
//                 conn.print(data);
//             }
//             else {
//                 BLINKER_ERR_LOG1("SEND DATA BYTES MAX THAN LIMIT!");
//             }
//             _print(n);
//         }
// #endif
        void beginFormat() {
            isFormat = true;
            memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
        }

        bool endFormat() {
            isFormat = false;
            if (strlen(_sendBuf)) {
                _print("{" + STRING_format(_sendBuf) + "}");
            }

            if (strlen(_sendBuf) > BLINKER_MAX_SEND_SIZE - 3) {
                return false;
            }
            else {
                return true;
            }
        }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        void bridgeBeginFormat() {
            isBformat = true;
            _bridgeKey = "";
            memset(_bSendBuf, '\0', BLINKER_MAX_SEND_SIZE);
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
            if (!isFormat)
                _print("\""  + STRING_format(n)+ "\"");
        }
        void print() {
            if (!isFormat)
                _print("\"\"");
        }
        
        template <typename T>
        void println(T n) {
            if (!isFormat)
                _print("\""  + STRING_format(n)+ "\"");
        }
        void println() {
            if (!isFormat)
                _print("\"\"");
        }

        template <typename T1, typename T2, typename T3>
        void print(T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }
        
        template <typename T1, typename T2, typename T3>
        void println(T1 n1, T2 n2, T3 n3) {
            String _msg = "\""  + STRING_format(n1) + "\":\"" + STRING_format(n2) + BLINKER_CMD_INTERSPACE + STRING_format(n3) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
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
        template <typename T1>
        void printArray(T1 n1, const String &s2) {
            String _msg = "\"" + STRING_format(n1) + "\":" + s2;

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }
        
        template <typename T1>
        void print(T1 n1, const String &s2) {
            String _msg = "\"" + STRING_format(n1) + "\":\"" + s2 + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, const char str2[]) {
            String _msg = "\"" + STRING_format(n1) + "\":\"" + STRING_format(str2) + "\"";

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, char c) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(c);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, unsigned char b) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(b);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, int n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, unsigned int n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, long n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, unsigned long n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
        }

        template <typename T1>
        void print(T1 n1, double n) {
            String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
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
        
        template <typename T>
        void notify(T n) {
            String _msg = "\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"";
            if (isFormat) {
                formatData(_msg);
            }
            else {
                _print("{" + _msg + "}");
            }
// #if defined(BLINKER_MQTT)
//             print("{\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"}", true);
// #else
//             _print("{\"" + STRING_format(BLINKER_CMD_NOTICE) + "\":\"" + STRING_format(n) + "\"}");
// #endif
        }
        
        void flush() {
            isFresh = false;
            availState = false;
        }

#if defined(BLINKER_PRO)
        bool inited() { return _isInit;}
#endif

    private :
        void formatData(String data) {
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

    protected :
        Transp&         conn;
        BlinkerState    state;
        bool            isFresh;
        bool            isAvail;
        bool            availState;
        bool            canParse;
        bool            isFormat;
        char            _sendBuf[BLINKER_MAX_SEND_SIZE];
#if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        bool            isBridgeFresh = false;
        bool            isExtraAvail = false;
        bool            isBridgeAvail = false;
        bool            isBformat = false;
        char            _bSendBuf[BLINKER_MAX_SEND_SIZE];
        String          _bridgeKey;
        String          _bKey_forwhile;
#endif

#if defined(BLINKER_MQTT)
        char            _authKey[BLINKER_AUTHKEY_SIZE];
        char            _deviceName[BLINKER_MQTT_DEVICEID_SIZE];
#endif

#if defined(BLINKER_PRO)
        bool            _isConnBegin = false;
        bool            _getRegister = false;
        bool            _isInit = false;

        uint32_t        _initTime;
#endif

#if defined(BLINKER_PRO)
        bool beginPro() {
            return BApi::wlanRun();
        }
#endif

        void begin()
        {
            BLINKER_LOG1((""));
        #if defined(BLINKER_NO_LOGO)
            BLINKER_LOG2(("Blinker v"), BLINKER_VERSION);
        #elif defined(BLINKER_LOGO_3D)
            BLINKER_LOG1(("\n"
                " ____    ___                __                       \n"
                "/\\  _`\\ /\\_ \\    __        /\\ \\               v"BLINKER_VERSION"\n"
                "\\ \\ \\L\\ \\//\\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"
                " \\ \\  _ <'\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"
                "  \\ \\ \\L\\ \\\\_\\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/  \n"
                "   \\ \\____//\\____\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"
                "    \\/___/ \\/____/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"));
        #else
            BLINKER_LOG1(("\n"
                "   ___  ___      __    v"BLINKER_VERSION"\n"
                "  / _ )/ (_)__  / /_____ ____\n"
                " / _  / / / _ \\/  '_/ -_) __/\n"
                "/____/_/_/_//_/_/\\_\\\\__/_/   \n"));
        #endif
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

            BLINKER_LOG1(BLINKER_F("==========================================================="));
            BLINKER_LOG1(BLINKER_F("================= Blinker PRO mode init ! ================="));
            BLINKER_LOG1(BLINKER_F("Warning! EEPROM address 1280-1535 is used for Auto Control!"));
            BLINKER_LOG1(BLINKER_F("============= DON'T USE THESE EEPROM ADDRESS! ============="));
            BLINKER_LOG1(BLINKER_F("==========================================================="));

            BLINKER_LOG2(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

    #if defined(BLINKER_BUTTON)
            BApi::buttonInit();
    #endif
            BApi::setType(_type);
        }
#endif

        template <typename T>
        void _print(T n, bool needParse = true) {
            String data = STRING_format(n) + BLINKER_CMD_NEWLINE;
            if (data.length() <= BLINKER_MAX_SEND_SIZE) {
                conn.print(data);
                if (needParse) {
                    BApi::parse(data, true);
                }
            }
            else {
                BLINKER_ERR_LOG1(("SEND DATA BYTES MAX THAN LIMIT!"));
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
                    BLINKER_ERR_LOG1(("SEND DATA BYTES MAX THAN LIMIT!"));
                }
            }
            else {
                BLINKER_ERR_LOG1(("MAKE SURE THE BRIDGE DEVICE IS REGISTERED!"));
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
        return;
    }
    else {
        if (!_isConnBegin) {
            conn.begin(BApi::type());
            _isConnBegin = true;
            _initTime = millis();
    #ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("conn begin, fresh _initTime: "), _initTime);
    #endif
            if (conn.authCheck()) {
    #ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("is auth, conn deviceRegister"));
    #endif
                conn.deviceRegister();
            }
        }
    }

    if (_getRegister) {
    #ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1(BLINKER_F("conn deviceRegister"));
    #endif
        conn.deviceRegister();
        _getRegister = false;
    }

    if (!conn.init()) {
        if ((millis() - _initTime) >= BLINKER_CHECK_AUTH_TIME) {
            BApi::reset();
        }
    }
    else {
        if (!_isInit) {
            _isInit = true;
            strcpy(_deviceName, conn.deviceName().c_str());
        }
    }
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO)
    BApi::ntpInit();
// #endif
// #if defined(ESP8266) || defined(ESP32)
    BApi::checkTimer();
#endif

    bool conState = conn.connected();

    switch (state)
    {
        case CONNECTING :
            if (conn.connect()) {
                state = CONNECTED;
            }
            break;
        case CONNECTED :
            if (conState) {
                checkAvail();
                if (isAvail) {
                    BApi::parse(dataParse());
                }
            }
            else {
                state = DISCONNECTED;
            }
            break;
        case DISCONNECTED :
            conn.disconnect();
            state = CONNECTING;
            break;
    }
}

#endif