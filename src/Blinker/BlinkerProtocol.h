#ifndef BLINKER_PROTOCOL_H
#define BLINKER_PROTOCOL_H

#include "Blinker/BlinkerApi.h"

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#if defined(BLINKER_AT_MQTT)
    #include "Blinker/BlinkerMQTTATBase.h"
#endif

#if defined(BLINKER_PRO)
    enum BlinkerStatus
    {
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
class BlinkerProtocol : public BlinkerApi< BlinkerProtocol<Transp> >
{
    friend class BlinkerApi< BlinkerProtocol<Transp> >;

    typedef BlinkerApi< BlinkerProtocol<Transp> > BApi;

    public :
        enum _blinker_state_t
        {
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
        {}

        bool connect(uint32_t timeout = BLINKER_STREAM_TIMEOUT);
        bool connected()    { return state == CONNECTED; }
        void disconnect()   { conn.disconnect(); state = DISCONNECTED; }
        bool available()    { if (availState) {availState = false; return true;} else return false; }
        // String readString();
        void attachData(blinker_callback_with_string_arg_t newFunction)
        { _availableFunc = newFunction; }

        template <typename T>
        void print(T n);
        void print();
        
        template <typename T>
        void println(T n)   { print(n); }
        void println()      { print(); }

        template <typename T1, typename T2, typename T3>
        void print(T1 n1, T2 n2, T3 n3);
        
        template <typename T1, typename T2, typename T3>
        void println(T1 n1, T2 n2, T3 n3)   { print(n1, n2, n3); }

        template <typename T1>
        void printArray(T1 n1, const String &s2);

        template <typename T1>
        void printObject(T1 n1, const String &s2);

        template <typename T1>
        void print(T1 n1, const String &s2);
        template <typename T1>
        void print(T1 n1, const char str2[]);
        template <typename T1>
        void print(T1 n1, char c);
        template <typename T1>
        void print(T1 n1, unsigned char b);
        template <typename T1>
        void print(T1 n1, int n);
        template <typename T1>
        void print(T1 n1, unsigned int n);
        template <typename T1>
        void print(T1 n1, long n);
        template <typename T1>
        void print(T1 n1, unsigned long n);
        template <typename T1>
        void print(T1 n1, double n);

        template <typename T1>
        void println(T1 n1, const String &s2)   { print(n1, s2); }
        template <typename T1>
        void println(T1 n1, const char str2[])  { print(n1, str2); }
        template <typename T1>
        void println(T1 n1, char c)             { print(n1, c); }
        template <typename T1>
        void println(T1 n1, unsigned char b)    { print(n1, b); }
        template <typename T1>
        void println(T1 n1, int n)              { print(n1, n); }
        template <typename T1>
        void println(T1 n1, unsigned int n)     { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, long n)             { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, unsigned long n)    { print(n1, n); }        
        template <typename T1>
        void println(T1 n1, double n)           { print(n1, n); }

        template <typename T>
        void notify(T n);

        void checkState(bool state = true)      { isCheck = state; }

        void flush();

        void run();

        #if defined(BLINKER_PRO)
            bool init()                         { return _isInit; }
            bool registered()                   { return conn.authCheck(); }
            uint8_t status()                    { return _proStatus; }
        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
            void beginAuto();
            bool autoTrigged(uint32_t _id);
            bool checkCanOTA();
            // bool autoTrigged(char *name, char *type, char *data);
            // bool autoTrigged(char *name1, char *type1, char *data1, 
            //                 char *name2, char *type2, char *data2);
        #endif

        // template <typename T1, typename T2, typename T3>
        // void bridgePrint(const String & bKey, T1 n1, T2 n2, T3 n3);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, const String &s2);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, const char str2[]);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, char c);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, unsigned char b);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, int n);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, unsigned int n);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, long n);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, unsigned long n);
        // template <typename T1>
        // void bridgePrint(const String & bKey, T1 n1, double n);
        // void bridgeBeginFormat();
        // bool bridgeEndFormat();
        // bool bridgeAvailable(const String & bKey);
        // String bridgeRead();
        // String bridgeRead(const String & bKey);

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_MQTT_AT)
            void bridgePrint(char * bName, const String & data);
            void aligeniePrint(String & _msg);
            void duerPrint(String & _msg);
            // void freshAlive();
        #endif

        // void printNow();

    private :
        #if defined(BLINKER_ARDUINOJSON)
            void autoFormatData(const String & key, const String & jsonValue);
        #else
            void autoFormatData(String & data);
        #endif
        void checkFormat();
        bool checkAvail();
        bool checkAliAvail() { return conn.aligenieAvail(); }
        bool checkDuerAvail() { return conn.duerAvail(); }
        char* dataParse()   { if (canParse) return conn.lastRead(); else return NULL; }
        char* lastRead()    { return conn.lastRead(); }
        void isParsed()     { flush(); }
        bool parseState()   { return canParse; }
        void printNow();
        void checkAutoFormat();

        #if defined(BLINKER_AT_MQTT)
            class BlinkerSlaverAT * _slaverAT;
            class PinData *         _pinData[BLINKER_MAX_PIN_NUM];

            bool serialAvailable();
            void serialPrint(const String & s);
            void atHeartbeat();
        #endif

    protected :
        Transp&             conn;
        _blinker_state_t    state;
        bool                isFresh;
        bool                isAvail;
        bool                availState;
        bool                canParse;
        bool                autoFormat = false;
        bool                isCheck = true;
        uint32_t            autoFormatFreshTime;
        char*               _sendBuf;
        blinker_callback_with_string_arg_t  _availableFunc = NULL;
        
        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
            bool                _isInit = false;
            bool                _isAuto = false;
            bool                _isAutoInit = false;
            uint8_t             _disconnectCount = 0;
            uint32_t            _disFreshTime = 0;
            uint32_t            _disconnectTime = 0;
            uint32_t            _refreshTime = 0;
        #endif

        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
            uint32_t            _reconTime = 0;
        #endif

        #if defined(BLINKER_PRO)
            bool            _isConnBegin = false;
            bool            _getRegister = false;
            // bool            _isInit = false;
            bool            _isRegistered = false;
            uint32_t        _register_fresh = 0;
            uint32_t        _initTime;
            uint8_t         _proStatus = PRO_WLAN_CONNECTING;
        #endif

        #if defined(BLINKER_AT_MQTT)
            blinker_at_status_t     _status = BL_BEGIN;
            blinker_at_aligenie_t   _aliType = ALI_NONE;
            blinker_at_dueros_t     _duerType = DUER_NONE;
            uint8_t                 _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
            uint8_t                 pinDataNum = 0;
            // bool                    _isAtRegister = false;

            void atBegin();
            void parseATdata();
            String getMode(uint8_t mode);
        #endif

        // bool                isBridgeFresh = false;
        // bool                isExtraAvail = false;
        // bool                isBridgeAvail = false;
        // bool                isBformat = false;
        // bool                autoBFormat = false;
        // char                _bSendBuf[BLINKER_MAX_SEND_BUFFER_SIZE];
        // String              _bridgeKey;
        // String              _bKey_forwhile;
                
        void _timerPrint(const String & n);
        void _print(char * n, bool needCheckLength = true);
        void begin();

        #if defined(BLINKER_PRO)
            bool beginPro() { return BApi::wlanRun(); }
            void begin(const char* _type);
        #endif

        // #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
        //     void loadOTA();
        // #endif

        #if defined(BLINKER_MQTT_AT)
            void atInit(const char* _auth);
            void atInit(const char* _auth, const char* _ssid, const char* _pswd);
        #endif
        
};

template <class Transp>
bool BlinkerProtocol<Transp>::connect(uint32_t timeout)
{
    state = CONNECTING;

    uint32_t startTime = millis();
    while ( (state != CONNECTED) && \
        (millis() - startTime) < timeout )
    { run(); }

    return state == CONNECTED;
}

// template <class Transp>
// String BlinkerProtocol<Transp>::readString()
// {
//     if (isFresh)
//     {
//         isFresh = false;
//         // char* r_data = (char*)malloc(strlen(conn.lastRead())*sizeof(char));
//         // strcpy(r_data, conn.lastRead());

//         String r_data = conn.lastRead();

//         flush();
//         return r_data;
//     }
//     else {
//         return "";
//     }
// }

template <class Transp>
void BlinkerProtocol<Transp>::print()
{
    String _msg = BLINKER_F("");

    checkFormat();
    strcpy(_sendBuf, _msg.c_str());
    _print(_sendBuf);
    free(_sendBuf);
    autoFormat = false;
}

template <class Transp> template <typename T>
void BlinkerProtocol<Transp>::print(T n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n);
    _msg += BLINKER_F("\"");

    checkFormat();
    strcpy(_sendBuf, _msg.c_str());
    _print(_sendBuf);
    free(_sendBuf);
    autoFormat = false;
}

template <class Transp> template <typename T1, typename T2, typename T3>
void BlinkerProtocol<Transp>::print(T1 n1, T2 n2, T3 n3)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(n2);
    _msg += BLINKER_CMD_INTERSPACE;
    _msg += STRING_format(n3);
    _msg += BLINKER_F("\"");

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::printArray(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += s2;
    
    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::printObject(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += s2;
    
    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += s2;
    _msg += BLINKER_F("\"");

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, const char str2[])
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(str2);
    _msg += BLINKER_F("\"");

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, char c)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(c);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned char b)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(b);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();        
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, double n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(n1), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp> template <typename T>
void BlinkerProtocol<Transp>::notify(T n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(BLINKER_CMD_NOTICE);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(n);
    _msg += BLINKER_F("\"");
    
    checkFormat();

    #if defined(BLINKER_ARDUINOJSON)
        autoFormatData(STRING_format(BLINKER_CMD_NOTICE), _msg);
    #else
        autoFormatData(_msg);
    #endif

    autoFormatFreshTime = millis();
}

template <class Transp>
void BlinkerProtocol<Transp>::flush()
{ 
    conn.flush(); 
    isFresh = false; availState = false; 
    canParse = false; isAvail = false;
}

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
    template <class Transp>
    void BlinkerProtocol<Transp>::beginAuto()
    {
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

    template <class Transp>
    bool BlinkerProtocol<Transp>::autoTrigged(uint32_t _id)
    {
        BLINKER_LOG_ALL(BLINKER_F("autoTrigged id: "), _id);
        
        return conn.autoPrint(_id);
    }

    template <class Transp>
    bool BlinkerProtocol<Transp>::checkCanOTA()
    {
        // #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined (BLINKER_AT_MQTT)
            #if defined(ESP8266)
                int boot_mode = (GPI >> 16) & 0xf;
                if (boot_mode == 1)
                {
                    // _setError(UPDATE_ERROR_BOOTSTRAP);
                    // return false;
                    BLINKER_ERR_LOG(BLINKER_F("==========================================================="));
                    BLINKER_ERR_LOG(BLINKER_F("Invalid bootstrapping state, reset ESP8266 before updating!"));
                    BLINKER_ERR_LOG(BLINKER_F("==========================================================="));
                
                    return false;
                }
            #endif
        // #endif        
                
        return true;
    }

    // template <class Transp>
    // bool BlinkerProtocol<Transp>::autoTrigged(char *name, char *type, char *data)
    // {
    //     BLINKER_LOG_ALL(BLINKER_F("autoTrigged"));
        
    //     if (strcpy(conn.deviceName(), name) == 0)
    //     {
    //         BApi::parse(data, true);
    //         return true;
    //     }
    //     return conn.autoPrint(name, type, data);
    // }

    // template <class Transp>
    // bool BlinkerProtocol<Transp>::autoTrigged(char *name1, char *type1, char *data1, 
    //     char *name2, char *type2, char *data2)
    // {
    //     BLINKER_LOG_ALL(BLINKER_F("autoTrigged"));
        
    //     bool _link1 = false;
    //     bool _link2 = false;

    //     if (conn.deviceName() == name1)
    //     {
    //         BApi::parse(data1, true);
    //         _link1 = true;
    //     }
    //     if (conn.deviceName() == name2)
    //     {
    //         BApi::parse(data2, true);
    //         _link2 = true;
    //     }
    //     if (_link1 && _link2)
    //     {
    //         return conn.autoPrint(name1, type1, data1, name2, type2, data2);
    //     }
    //     else if (_link1)
    //     {
    //         return conn.autoPrint(name2, type2, data2);
    //     }
    //     else if (_link2)
    //     {
    //         return conn.autoPrint(name1, type1, data1);
    //     }
    // }
#endif

// template <class Transp> template <typename T1, typename T2, typename T3>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, T2 n2, T3 n3)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":\"";
//     _msg += STRING_format(n2);
//     _msg += BLINKER_CMD_INTERSPACE;
//     _msg += STRING_format(n3);
//     _msg += "\"";

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, const String &s2)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":\"";
//     _msg += s2;
//     _msg += "\"";

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, const char str2[])
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":\"";
//     _msg += STRING_format(str2);
//     _msg += "\"";

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, char c)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":";
//     _msg += STRING_format(c);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, unsigned char b)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":";
//     _msg += STRING_format(b);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, int n)
// {
//     String _msg = "\"" + STRING_format(n1) + "\":" + STRING_format(n);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, unsigned int n)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":";
//     _msg += STRING_format(n);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, long n)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":";
//     _msg += STRING_format(n);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, double n)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":";
//     _msg += STRING_format(n);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp> template <typename T1>
// void BlinkerProtocol<Transp>::bridgePrint(const String & bKey, T1 n1, unsigned long n)
// {
//     String _msg = "\"";
//     _msg += STRING_format(n1);
//     _msg += "\":";
//     _msg += STRING_format(n);

//     if (isBformat)
//     {
//         _bridgeKey = bKey;
//         bridgeFormatData(_msg);
//     }
//     else
//     {
//         _bPrint(bKey, "{" + _msg + "}");
//     }
// }

// template <class Transp>
// void BlinkerProtocol<Transp>::bridgeBeginFormat()
// {
//     isBformat = true;
//     _bridgeKey = "";
//     memset(_bSendBuf, '\0', BLINKER_MAX_SEND_BUFFER_SIZE);
// }

// template <class Transp>
// bool BlinkerProtocol<Transp>::bridgeEndFormat()
// {
//     isBformat = false;
//     if (strlen(_bSendBuf))
//     {
//         _bPrint(_bridgeKey, "{" + STRING_format(_bSendBuf) + "}");
//     }

//     if (strlen(_bSendBuf) > BLINKER_MAX_SEND_SIZE - 3)
//     {
//         return false;
//     }
//     else
//     {
//         return true;
//     }
// }

// template <class Transp>
// bool BlinkerProtocol<Transp>::bridgeAvailable(const String & bKey)
// {
//     if (checkExtraAvail())
//     {
//         String b_name = BApi::bridgeFind(bKey);

//         // BLINKER_LOG("bridgeAvailable b_name: ", b_name);

//         if (b_name.length() > 0)
//         {
//             // _bKey_forwhile = b_name;
//             String b_data = conn.lastRead();

//             // BLINKER_LOG("bridgeAvailable b_data: ", b_data);

//             DynamicJsonBuffer jsonBuffer;
//             JsonObject& extra_data = jsonBuffer.parseObject(b_data);

//             String _from = extra_data["fromDevice"];

//             // BLINKER_LOG("bridgeAvailable _from: ", _from);

//             if (b_name == _from)
//             {
//                 _bKey_forwhile = b_name;
//                 isExtraAvail = false;
//                 return true;
//             }
//             else
//             {
//                 return false;
//             }
//         }
//         else
//         {
//             _bKey_forwhile = "";
//             return false;
//         }
//     }
//     else
//     {
//         return false;
//     }
// }

// template <class Transp>
// String BlinkerProtocol<Transp>::bridgeRead()
// {
//     String b_data = conn.lastRead();

//     if (_bKey_forwhile.length() > 0)
//     {
//         DynamicJsonBuffer jsonBuffer;
//         JsonObject& extra_data = jsonBuffer.parseObject(b_data);

//         if (!extra_data.success())
//         {
//             return "";
//         }
//         else
//         {
//             String _from = extra_data["fromDevice"];
//             if (_from == _bKey_forwhile) {
//                 String _data = extra_data["data"];

//                 _bKey_forwhile = "";
//                 return _data;
//             }
//             else
//             {
//                 return "";
//             }
//         }
//     }
//     else
//     {
//         return "";
//     }
// }

// template <class Transp>
// String BlinkerProtocol<Transp>::bridgeRead(const String & bKey)
// {
//     String b_name = BApi::bridgeFind(bKey);
//     String b_data = conn.lastRead();

//     if (b_name.length() > 0 && isBridgeFresh)
//     {
//         DynamicJsonBuffer jsonBuffer;
//         JsonObject& extra_data = jsonBuffer.parseObject(b_data);

//         if (!extra_data.success())
//         {
//             return "";
//         }
//         else
//         {
//             String _from = extra_data["fromDevice"];
//             if (_from == b_name)
//             {
//                 String _data = extra_data["data"];
//                 isBridgeFresh = false;
//                 return _data;
//             }
//             else
//             {
//                 return "";
//             }
//         }
//     }
//     else
//     {
//         return "";
//     }
// }

#if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_MQTT_AT)
    template <class Transp>
    void BlinkerProtocol<Transp>::bridgePrint(char * bName, const String & data)
    {
        conn.bPrint(bName, data);
    }
    
    template <class Transp>
    void BlinkerProtocol<Transp>::aligeniePrint(String & _msg)
    {
        BLINKER_LOG_ALL(BLINKER_F("response to AliGenie: "), _msg);

        // conn.aliPrint(_msg);

        if (_msg.length() <= BLINKER_MAX_SEND_SIZE)
        {
            // char* aliData = (char*)malloc((_msg.length()+1+128)*sizeof(char));
            // memcpy(aliData, '\0', _msg.length()+128);
            // strcpy(aliData, _msg.c_str());
            conn.aliPrint(_msg);
            // free(aliData);
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
        }
    }

    template <class Transp>
    void BlinkerProtocol<Transp>::duerPrint(String & _msg)
    {
        BLINKER_LOG_ALL(BLINKER_F("response to DuerOS: "), _msg);

        // conn.aliPrint(_msg);

        if (_msg.length() <= BLINKER_MAX_SEND_SIZE)
        {
            // char* aliData = (char*)malloc((_msg.length()+1+128)*sizeof(char));
            // memcpy(aliData, '\0', _msg.length()+128);
            // strcpy(aliData, _msg.c_str());
            conn.duerPrint(_msg);
            // free(aliData);
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
        }
    }

    // template <class Transp>
    // void BlinkerProtocol<Transp>::freshAlive()
    // {
    //     conn.freshAlive();
    // }
#endif

#if defined(BLINKER_ARDUINOJSON)
    template <class Transp>
    void BlinkerProtocol<Transp>::autoFormatData(const String & key, const String & jsonValue)
    {
        BLINKER_LOG_ALL(BLINKER_F("autoFormatData key: "), key, \
                        BLINKER_F(", json: "), jsonValue);

        String _data;

        if (STRING_contains_string(STRING_format(_sendBuf), key))
        {

            DynamicJsonBuffer jsonSendBuffer;                

            if (strlen(_sendBuf)) {
                BLINKER_LOG_ALL(BLINKER_F("add"));

                JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_sendBuf));

                if (root.containsKey(key)) {
                    root.remove(key);
                }
                root.printTo(_data);

                _data = _data.substring(0, _data.length() - 1);

                if (_data.length() > 4 ) _data += BLINKER_F(",");
                _data += jsonValue;
                _data += BLINKER_F("}");
            }
            else {
                BLINKER_LOG_ALL(BLINKER_F("new"));
                
                _data = BLINKER_F("{");
                _data += jsonValue;
                _data += BLINKER_F("}");
            }
        }
        else {
            _data = STRING_format(_sendBuf);

            if (_data.length())
            {
                BLINKER_LOG_ALL(BLINKER_F("add."));

                _data = _data.substring(0, _data.length() - 1);

                _data += BLINKER_F(",");
                _data += jsonValue;
                _data += BLINKER_F("}");
            }
            else {
                BLINKER_LOG_ALL(BLINKER_F("new."));
                
                _data = BLINKER_F("{");
                _data += jsonValue;
                _data += BLINKER_F("}");
            }
        }

        if (strlen(_sendBuf) > BLINKER_MAX_SEND_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
            return;
        }

        strcpy(_sendBuf, _data.c_str());
    }
#else
    template <class Transp>
    void BlinkerProtocol<Transp>::autoFormatData(String & data)
    {
        BLINKER_LOG_ALL(BLINKER_F("autoFormatData data: "), data);
        BLINKER_LOG_ALL(BLINKER_F("strlen(_sendBuf): "), strlen(_sendBuf));
        BLINKER_LOG_ALL(BLINKER_F("data.length(): "), data.length());

        if ((strlen(_sendBuf) + data.length()) >= BLINKER_MAX_SEND_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
            return;
        }

        if (strlen(_sendBuf) > 0) {
            data = "," + data;
            strcat(_sendBuf, data.c_str());
        }
        else {
            data = "{" + data;
            strcpy(_sendBuf, data.c_str());
        }
    }
#endif

template <class Transp>
void BlinkerProtocol<Transp>::checkFormat()
{
    if (!autoFormat)
    {
        autoFormat = true;
        _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
    }
}

template <class Transp>
bool BlinkerProtocol<Transp>::checkAvail()
{
    flush();

    isAvail = conn.available();
    if (isAvail)
    {
        isFresh = true;
        canParse = true;
        availState = true;
    }
    return isAvail;
}

template <class Transp>
void BlinkerProtocol<Transp>::printNow()
{
    if (strlen(_sendBuf) && autoFormat)
    {
        #if defined(BLINKER_ARDUINOJSON)
            _print(_sendBuf);
        #else
            strcat(_sendBuf, "}");
            _print(_sendBuf);
        #endif

        free(_sendBuf);
        autoFormat = false;
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::checkAutoFormat()
{
    if (autoFormat)
    {
        if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
        {
            if (strlen(_sendBuf))
            {
                #if defined(BLINKER_ARDUINOJSON)
                    _print(_sendBuf);
                #else
                    strcat(_sendBuf, "}");
                    _print(_sendBuf);
                #endif
            }
            free(_sendBuf);
            autoFormat = false;
        }
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::_timerPrint(const String & n)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (n.length() <= BLINKER_MAX_SEND_SIZE)
    {
        checkFormat();
        checkState(false);
        strcpy(_sendBuf, n.c_str());
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::_print(char * n, bool needCheckLength)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (strlen(n) <= BLINKER_MAX_SEND_SIZE || \
        !needCheckLength)
    {
        BLINKER_LOG_FreeHeap_ALL();
        conn.print(n, isCheck);
        if (!isCheck) isCheck = true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}

#if defined(BLINKER_AT_MQTT)
    template <class Transp>
    void BlinkerProtocol<Transp>::atBegin()
    {
        while (_status == BL_BEGIN)
        {
            serialAvailable();
        }
    }

    template <class Transp>
    void BlinkerProtocol<Transp>::parseATdata()
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
            // reqData = "+" + STRING_format(BLINKER_CMD_GMR) + 
            //         "=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>" + 
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
                    reqData = BLINKER_CMD_AT;// + 
                    reqData += BLINKER_F("+");
                    reqData += BLINKER_CMD_UART_CUR;// + 
                    reqData += BLINKER_F("=<baudrate>,<databits>,<stopbits>,<parity>");
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_QUERY:
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_UART_CUR;
                    reqData += BLINKER_F(":");
                    reqData += STRING_format(serialSet >> 8 & 0x00FFFFFF);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(serialSet >> 4 & 0x0F);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(serialSet >> 2 & 0x03);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(serialSet      & 0x03);
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
                    reqData = BLINKER_CMD_AT;
                    reqData += BLINKER_F("+");
                    reqData += BLINKER_CMD_UART_DEF;
                    reqData += BLINKER_F("=<baudrate>,<databits>,<stopbits>,<parity>");
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_QUERY:
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_UART_DEF;
                    reqData += BLINKER_F(":");
                    reqData += STRING_format(serialSet >> 8 & 0x00FFFFFF);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(serialSet >> 4 & 0x0F);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(serialSet >> 2 & 0x03);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(serialSet      & 0x03);
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
            reqData = BLINKER_F("+");
            reqData += BLINKER_F(BLINKER_CMD_RAM);
            reqData += BLINKER_F(":");
            reqData += STRING_format(BLINKER_FreeHeap());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_ADC && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_ADC;
            reqData += BLINKER_F(":");
            reqData += STRING_format(analogRead(A0));
            
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
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_IOGETCFG;
                    reqData += BLINKER_F(":");
                    reqData += _pinData[_num]->data();
                    conn.serialPrint(reqData);
                }
            }
            if (!_isGet) {
                reqData = BLINKER_F("+");
                reqData += BLINKER_CMD_IOGETCFG;
                reqData += BLINKER_F(":");
                reqData += _slaverAT->getParam(PIN_SET);
                reqData += BLINKER_F(",2,0");
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
                            reqData = BLINKER_F("+");
                            reqData += BLINKER_CMD_GPIOWREAD;
                            reqData += BLINKER_F(":");
                            reqData += STRING_format(set_pin);
                            reqData += BLINKER_F(",");
                            reqData += STRING_format(_pinData[_num]->getMode());
                            reqData += BLINKER_F(",");
                            reqData += STRING_format(digitalRead(set_pin));
                            conn.serialPrint(reqData);
                            conn.serialPrint(BLINKER_CMD_OK);
                            return;
                    //     }
                    // }
                }
            }
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_GPIOWREAD;
            reqData += BLINKER_F(":");
            reqData += STRING_format(set_pin);
            reqData += BLINKER_F(",3,");
            reqData += STRING_format(digitalRead(set_pin));
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
                    reqData = BLINKER_CMD_AT;
                    reqData += BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_MQTT;
                    reqData += BLINKER_F("=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>");
                    reqData += BLINKER_F("[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]");
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_QUERY:
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_MQTT;
                    reqData += BLINKER_F(":");
                    reqData += STRING_format(_wlanMode);
                    reqData += BLINKER_F(",");
                    reqData += STRING_format(conn.authKey());
                    reqData += BLINKER_F(",");
                    reqData += WiFi.SSID();
                    reqData += BLINKER_F(",");
                    reqData += WiFi.psk();
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
                            reqData = BLINKER_F("+");
                            reqData += BLINKER_CMD_BLINKER_MQTT;
                            reqData += BLINKER_F(":");
                            reqData += conn.deviceId();
                            reqData += BLINKER_F(",");
                            reqData += conn.uuid();
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
                            reqData = BLINKER_F("+");
                            reqData += BLINKER_CMD_BLINKER_MQTT;
                            reqData += BLINKER_F(":");
                            reqData += conn.deviceId();
                            reqData += BLINKER_F(",");
                            reqData += conn.uuid();
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
                            reqData = BLINKER_F("+");
                            reqData += BLINKER_CMD_BLINKER_MQTT;
                            reqData += BLINKER_F(":");
                            reqData += conn.deviceId();
                            reqData += BLINKER_F(",");
                            reqData += conn.uuid();
                            conn.serialPrint(reqData);
                            conn.serialPrint(BLINKER_CMD_OK);
                            return;
                        }

                        if (!conn.autoInit())
                        {
                            conn.softAPinit();
                            // while(WiFi.status() != WL_CONNECTED) {
                            //     conn.serverClient();

                            //     ::delay(10);
                            // }
                        }

                        conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                        _status = BL_INITED;
                        _wlanMode = BLINKER_CMD_APCONFIG_NUM;
                    }
                    else {
                        return;
                    }

                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_MQTT;
                    reqData += BLINKER_F(":");
                    reqData += conn.deviceId();
                    reqData += BLINKER_F(",");
                    reqData += conn.uuid();
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
                    reqData = BLINKER_CMD_AT;
                    reqData += BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_ALIGENIE;
                    reqData += BLINKER_F("=<type>");
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_QUERY:
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_ALIGENIE;
                    reqData += BLINKER_F(":");
                    reqData += STRING_format(_aliType);
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
                    else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_OUTLET)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("ALI_OUTLET"));
                        _aliType = ALI_OUTLET;
                    }
                    else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_SENSOR)
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
        else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_DUEROS) {
            // conn.serialPrint(BLINKER_CMD_OK);

            BLINKER_LOG(BLINKER_CMD_BLINKER_DUEROS);

            blinker_at_state_t at_state = _slaverAT->state();

            BLINKER_LOG_ALL(at_state);

            switch (at_state)
            {
                case AT_NONE:
                    // conn.serialPrint();
                    break;
                case AT_TEST:
                    reqData = BLINKER_CMD_AT;
                    reqData += BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_DUEROS;
                    reqData += BLINKER_F("=<type>");
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_QUERY:
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_DUEROS;
                    reqData += BLINKER_F(":");
                    reqData += STRING_format(_duerType);
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_SETTING:
                    BLINKER_LOG_ALL(BLINKER_F("BLINKER_DUEROS_CFG_NUM: "), _slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM));

                    if (BLINKER_DUEROS_PARAM_NUM != _slaverAT->paramNum()) return;

                    if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == ALI_LIGHT)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("DUER_LIGHT"));
                        _duerType = DUER_LIGHT;
                    }
                    else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_OUTLET)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("DUER_OUTLET"));
                        _duerType = DUER_OUTLET;
                    }
                    else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_SENSOR)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("DUER_SENSOR"));
                        _duerType = DUER_SENSOR;
                    }
                    else {
                        BLINKER_LOG_ALL(BLINKER_F("DUER_NONE"));
                        _duerType = DUER_NONE;
                    }
                    conn.duerType(_duerType);
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
                    reqData = BLINKER_CMD_AT;
                    reqData += BLINKER_F("+");
                    reqData += BLINKER_CMD_TIMEZONE;
                    reqData += BLINKER_F("=<TIMEZONE>");
                    conn.serialPrint(reqData);
                    conn.serialPrint(BLINKER_CMD_OK);
                    break;
                case AT_QUERY:
                    reqData = BLINKER_F("+");
                    reqData += BLINKER_CMD_BLINKER_MQTT;
                    reqData += BLINKER_F(":");
                    reqData += STRING_format(BApi::getTimezone());
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
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_TIME_AT;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::time());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_SECOND && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_SECOND;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::second());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_SECOND && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_SECOND;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::second());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_MINUTE && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_MINUTE;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::minute());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_HOUR && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_HOUR;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::hour());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_WDAY && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_WDAY;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::wday());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_MDAY && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_MDAY;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::mday());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_YDAY && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_YDAY;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::yday());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_MONTH && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_MONTH;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::month());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_YEAR && _slaverAT->state() == AT_QUERY) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_YEAR;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::year());
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_WEATHER_AT && _slaverAT->state() == AT_SETTING) {
            if (1 != _slaverAT->paramNum()) return;
            
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_WEATHER_AT;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::weather(_slaverAT->getParam(0)));
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_AQI_AT && _slaverAT->state() == AT_SETTING) {
            if (1 != _slaverAT->paramNum()) return;
            
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_AQI_AT;
            reqData += BLINKER_F(":");
            reqData += STRING_format(BApi::aqi(_slaverAT->getParam(0)));
            
            conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_NOTICE_AT && _slaverAT->state() == AT_SETTING) {
            if (1 != _slaverAT->paramNum()) return;
            
            // reqData = "+" + STRING_format(BLINKER_CMD_NOTICE_AT) + 
            //         ":" + STRING_format(BApi::aqi(_slaverAT->getParam(0)));
            notify(_slaverAT->getParam(0));
            // conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
        else if (_slaverAT->cmd() == BLINKER_CMD_SMS_AT && _slaverAT->state() == AT_SETTING) {
            if (1 != _slaverAT->paramNum()) return;
            
            // reqData = "+" + STRING_format(BLINKER_CMD_NOTICE_AT) + 
            //         ":" + STRING_format(BApi::aqi(_slaverAT->getParam(0)));
            BApi::sms(_slaverAT->getParam(0));
            // conn.serialPrint(reqData);
            conn.serialPrint(BLINKER_CMD_OK);
        }
    }

    template <class Transp>
    String BlinkerProtocol<Transp>::getMode(uint8_t mode)
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
    
    template <class Transp>
    bool BlinkerProtocol<Transp>::serialAvailable()
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

    template <class Transp>
    void BlinkerProtocol<Transp>::serialPrint(const String & s)
    {
        conn.serialPrint(s);
    }

    template <class Transp>
    void BlinkerProtocol<Transp>::atHeartbeat()
    {
        conn.serialPrint(conn.lastRead());
    }
#endif

template <class Transp>
void BlinkerProtocol<Transp>::begin()
{
    BLINKER_LOG(BLINKER_F(""));
    #if defined(BLINKER_NO_LOGO)
        BLINKER_LOG(BLINKER_F("Blinker v"), BLINKER_VERSION, BLINKER_F("\n"),
                    BLINKER_F("    Give Blinker a Github star, thanks!\n"),
                    BLINKER_F("    => https://github.com/blinker-iot/blinker-library\n"));
    #elif defined(BLINKER_LOGO_3D)
        BLINKER_LOG(BLINKER_F("\n"),
            BLINKER_F(" ____    ___                __                       \n"),
            BLINKER_F("/\\  _`\\ /\\_ \\    __        /\\ \\               v"), BLINKER_VERSION, BLINKER_F("\n"),
            BLINKER_F("\\ \\ \\L\\ \\//\\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"),
            BLINKER_F(" \\ \\  _ <'\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"),
            BLINKER_F("  \\ \\ \\L\\ \\\\_\\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/  \n"),
            BLINKER_F("   \\ \\____//\\____\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"),
            BLINKER_F("    \\/___/ \\/____/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"),
            BLINKER_F("   Give Blinker a Github star, thanks!\n"),
            BLINKER_F("   => https://github.com/blinker-iot/blinker-library\n"));
    #else
        BLINKER_LOG(BLINKER_F("\n"),
            BLINKER_F("   ___  ___      __    v"), BLINKER_VERSION, BLINKER_F("\n"),
            BLINKER_F("  / _ )/ (_)__  / /_____ ____\n"),
            BLINKER_F(" / _  / / / _ \\/  '_/ -_) __/\n")
            BLINKER_F("/____/_/_/_//_/_/\\_\\\\__/_/   \n")
            BLINKER_F("Give Blinker a github star, thanks!\n")
            BLINKER_F("=> https://github.com/blinker-iot/blinker-library\n"));
    #endif

    #if defined(BLINKER_OTA_VERSION_CODE) && (defined(BLINKER_MQTT) || defined(BLINKER_MQTT_AT))
        if (strlen(BLINKER_OTA_VERSION_CODE) >= BLINKER_OTA_INFO_SIZE)
        {
            while(1)
            {
                BLINKER_ERR_LOG(BLINKER_F("The OTA version code you set is too long!"));
                ::delay(10000);
            }
        }
    #endif
}

#if defined(BLINKER_PRO)
    template <class Transp>
    void BlinkerProtocol<Transp>::begin(const char* _type)
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

// #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
//     template <class Transp>
//     void BlinkerProtocol<Transp>::loadOTA()
//     {
//         if (checkCanOTA()) BApi::loadOTA();
//     }
// #endif

#if defined(BLINKER_MQTT_AT)
    template <class Transp>
    void BlinkerProtocol<Transp>::atInit(const char* _auth)
    {
        // strcpy(_authKey, _auth);

        BApi::initCheck(STRING_format(_auth));
    }

    template <class Transp>
    void BlinkerProtocol<Transp>::atInit(const char* _auth, const char* _ssid, const char* _pswd)
    {
        // strcpy(_authKey, _auth);

        String init_data =  STRING_format(_auth) + "," + \
                            STRING_format(_ssid) + "," + \
                            STRING_format(_pswd);

        BApi::initCheck(init_data);
    }
#endif

template <class Transp>
void BlinkerProtocol<Transp>::run()
{
    #if defined(BLINKER_PRO)

        if (!BApi::wlanRun())
        {
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
        else
        {
            if (!_isConnBegin)
            {
                _proStatus = PRO_WLAN_CONNECTED;

                if (checkCanOTA()) BApi::loadOTA();

                conn.begin(BApi::type());
                _isConnBegin = true;
                _initTime = millis();
                
                BLINKER_LOG_ALL(BLINKER_F("conn begin, fresh _initTime: "), _initTime);

                // loadOTA();

                if (conn.authCheck())
                {
                    _proStatus = PRO_DEV_AUTHCHECK_SUCCESS;

                    BLINKER_LOG_ALL(BLINKER_F("is auth, conn deviceRegister"));

                    _isRegistered = conn.deviceRegister();
                    _getRegister = true;

                    if (!_isRegistered)
                    {
                        _register_fresh = millis();

                        _proStatus = PRO_DEV_REGISTER_FAIL;
                    }
                    else
                    {
                        _proStatus = PRO_DEV_REGISTER_SUCCESS;
                    }
                }
                else
                {
                    _proStatus = PRO_DEV_AUTHCHECK_FAIL;

                    BLINKER_LOG_ALL(BLINKER_F("not auth, conn deviceRegister"));
                }

                BLINKER_LOG_FreeHeap_ALL();
            }
        }

        if (_getRegister)
        {
            if (!_isRegistered && ((millis() - _register_fresh) > 5000 || \
                _register_fresh == 0))
            {
                BLINKER_LOG_ALL(BLINKER_F("conn deviceRegister"));
                
                _isRegistered = conn.deviceRegister();

                if (!_isRegistered)
                {
                    _register_fresh = millis();
                    _proStatus = PRO_DEV_REGISTER_FAIL;
                }
                else
                {
                    _isRegistered = true;
                    _proStatus = PRO_DEV_REGISTER_SUCCESS;
                }
            }
        }
        
        if (!conn.init())
        {
            yield();

            if ((millis() - _initTime) >= BLINKER_CHECK_AUTH_TIME && \
                !_getRegister)
            {
                BApi::reset();
            }
        }
        else
        {
            if (!_isInit)
            {
                if (BApi::ntpInit())
                {
                    _isInit = true;
                    // strcpy(_authKey, conn.authKey());
                    // strcpy(_deviceName, conn.deviceName());
                    _proStatus = PRO_DEV_INIT_SUCCESS;

                    uint32_t connect_time = millis();
                    uint32_t time_slot = 0;

                    while (time_slot < 30000)
                    {
                        time_slot = millis() - connect_time;
                        conn.connect();
                        yield();

                        if (conn.mConnected())
                        {
                            state = CONNECTED;
                            break;
                        }
                    }

                    // if (checkCanOTA()) BApi::loadOTA();
                }
            }
            else
            {
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
        BApi::ntpInit();
        BApi::checkTimer();
    #endif

    #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
        BApi::checkTimer();

        if (!_isInit)
        {
            if (conn.init() && BApi::ntpInit())
            {
                _isInit =true;
                _disconnectTime = millis();

                // conn.connect();
                // conn.connect();
                // conn.connect();
                // conn.connect();
                // if (!conn.connected()) conn.connect();

                // conn.disconnect();

                uint32_t connect_time = millis();
                uint32_t time_slot = 0;

                while (time_slot < 30000)
                {
                    time_slot = millis() - connect_time;
                    conn.connect();
                    yield();

                    if (conn.mConnected())
                    {
                        state = CONNECTED;
                        break;
                    }
                }

                BLINKER_LOG_ALL(BLINKER_F("millis: "), millis(),
                                BLINKER_F(", connect_time: "), connect_time);

                if (checkCanOTA()) BApi::loadOTA();

                BApi::bridgeInit();
                
                BLINKER_LOG_ALL(BLINKER_F("MQTT conn init success"));
            }
        }
        else {
            // if (((millis() - _disconnectTime) > BLINKER_MQTT_CONNECT_TIMESLOT * 10 && 
            //     _disconnectCount) || _disconnectCount >= 12)

            // if (_disconnectCount >= 12)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("device reRegister"));
            //     BLINKER_LOG_FreeHeap();
                
            //     // if (BLINKER_FreeHeap() < 15000) {
            //     //     conn.disconnect();
            //     //     return;
            //     // }

            //     // BLINKER_LOG_FreeHeap();

            //     if (conn.reRegister()) {
            //         _disconnectCount = 0;
            //         _disconnectTime = millis();
            //     }
            //     else {
            //         _disconnectCount = 0;
            //         _disconnectTime = millis() - 10000;
            //     }
            // }

            BApi::ntpInit();
        }

        // if ((millis() - _refreshTime) >= BLINKER_ONE_DAY_TIME * 2 * 1000)
        // {
        //     conn.disconnect();

        //     BLINKER_LOG_ALL(BLINKER_F("device reRegister"));
        //     BLINKER_LOG_FreeHeap();

        //     if (BLINKER_FreeHeap() < 15000) {
        //         conn.disconnect();
        //         return;
        //     }

        //     BLINKER_LOG_FreeHeap();

        //     if (conn.reRegister()) {
        //         _refreshTime = millis();
        //     }
        // }
    #endif

    #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
        if (WiFi.status() != WL_CONNECTED)
        {        
            if ((millis() - _reconTime) >= 10000 || \
                _reconTime == 0 )
            {
                _reconTime = millis();
                BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));
                WiFi.reconnect();
            }

            return;
        }
    #endif

    bool conState = conn.connected();

    #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
        if (_disconnectCount >= 3)
        {
            if ((millis() - _disconnectTime) > \
                BLINKER_MQTT_CONNECT_TIMESLOT * 12)
            {
                _disconnectCount = 0;
                _disconnectTime = millis();
            }
            yield();
            return;
        }
    #endif

    switch (state)
    {
        case CONNECTING :
            if (conn.connect())
            {
                state = CONNECTED;

                #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
                    _disconnectCount = 0;
                #endif
            }
            else
            {
                #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
                    if (_isInit)
                    {
                        if (_disconnectCount == 0)
                        {
                            _disconnectCount++;
                            _disconnectTime = millis();
                            _disFreshTime = millis();
                        }
                        else
                        {
                            // if ((millis() > _disFreshTime) && (millis() - _disFreshTime) >= 5000) {
                            if ((millis() - _disFreshTime) >= BLINKER_MQTT_CONNECT_TIMESLOT)
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
        case CONNECTED :
            if (conState)
            {
                checkAvail();
                if (isAvail)
                {
                    BApi::parse(dataParse());
                }

                #if (defined(BLINKER_MQTT) || defined(BLINKER_PRO)) 
                    #if defined(BLINKER_ALIGENIE)
                        if (checkAliAvail())
                        {
                            BApi::aliParse(conn.lastRead());
                        }
                    #endif

                    #if defined(BLINKER_DUEROS)
                        if (checkDuerAvail())
                        {
                            BApi::duerParse(conn.lastRead());
                        }
                    #endif
                #endif

                #if defined(BLINKER_MQTT_AT) && defined(BLINKER_ALIGENIE)
                    if (isAvail)
                    {
                        BApi::aliParse(conn.lastRead());

                        if (STRING_contains_string(conn.lastRead(), "AliGenie"))
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
    
                if (availState)
                {
                    availState = false;

                    if (_availableFunc)
                    {
                        _availableFunc(conn.lastRead());
                        flush();
                    }
                }
            }
            else
            {
                conn.disconnect();
                state = CONNECTING;
                #if defined(BLINKER_MQTT) || defined(BLINKER_AT_MQTT)
                    if (_isInit)
                    {
                        if (_disconnectCount == 0)
                        {
                            _disconnectCount++;
                            _disconnectTime = millis();
                            _disFreshTime = millis();
                        }
                        else
                        {
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
        if (_isAuto && _isInit && state == CONNECTED && !_isAutoInit)
        {
            if (BApi::autoPull()) _isAutoInit = true;
        }
    #endif

    checkAutoFormat();
}

#endif
