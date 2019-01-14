#ifndef BLINKER_PROTOCOL_H
#define BLINKER_PROTOCOL_H

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

enum _blinker_state_t
{
    CONNECTING,
    CONNECTED,
    DISCONNECTED
};

class BlinkerProtocol
{
    public :
        BlinkerProtocol()
            : state(CONNECTING)
            , isInit(false)
            , isFresh(false)
            , isAvail(false)
            , availState(false)
            , canParse(false)
        {}

        void transport(BlinkerStream & bStream) { conn = &bStream; isInit = true; }

        int connect()       { return isInit ? conn->connect() : false; }
        int connected()
        {
            if (isInit)
            {
                // if (conn->connected())
                // {
                //     state == CONNECTED;
                //     return true;
                // }
                // else
                // {
                //     state = DISCONNECTED;
                //     return false;
                // }
                return conn->connected();
            }
            else
            {
                state = DISCONNECTED;
                return false;
            }
        }
        void disconnect()   { if (isInit) { conn->disconnect(); state = DISCONNECTED; } }
        bool available()    { if (availState) {availState = false; return true;} else return false; }
        void flush();
        void checkState(bool state = true)      { isCheck = state; }
        void print(const String & data);
        void print(const String & key, const String & data);

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT) || defined(BLINKER_MQTT_AT)
            int aliPrint(const String & data)   { return isInit ? conn->aliPrint(data) : false; }
            int duerPrint(const String & data)  { return isInit ? conn->duerPrint(data) : false; }
            // void ping() { if (isInit) conn->ping(); }
        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
            char * deviceName() { if (isInit) return conn->deviceName(); else return NULL; }
            char * authKey()    { if (isInit) return conn->authKey(); else return NULL;  }
            int init()          { return isInit ? conn->init() : false; }
            int mConnected()    { return isInit ? conn->mConnected() : false; }
            int bPrint(char * name, const String & data) { return isInit ? conn->bPrint(name, data) : false; }
            int autoPrint(uint32_t id)  { return isInit ? conn->autoPrint(id) : false; }
            void freshAlive() { if (isInit) conn->freshAlive(); }
            void sharers(const String & data) { if (isInit) conn->sharers(data); }
        #endif

        #if defined(BLINKER_PRO)
            int deviceRegister(){ return conn->deviceRegister(); }
            int authCheck()     { return conn->authCheck(); }
            void begin(const char* _deviceType) { conn->begin(_deviceType); }
        #endif

    private :

    protected :
        BlinkerStream       *conn;
        _blinker_state_t    state;
        bool                isInit;
        bool                isFresh;
        bool                isAvail;
        bool                availState;
        bool                canParse;
        bool                autoFormat = false;
        bool                isCheck = true;
        uint32_t            autoFormatFreshTime;
        char*               _sendBuf;
        blinker_callback_with_string_arg_t  _availableFunc = NULL;

        int checkAvail();
        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
            bool checkAliAvail()    { return conn->aligenieAvail(); }
            bool checkDuerAvail()   { return conn->duerAvail(); }
        #endif
        
        #if defined(BLINKER_AT_MQTT)
            void begin(const char* auth) { return conn->begin(auth); }
            int serialAvailable()   { return conn->serialAvailable(); }
            int serialPrint(const String & s1, const String & s2, bool needCheck = true)
            { return conn->serialPrint(s1, s2, needCheck); }
            int serialPrint(const String & s, bool needCheck = true)
            { return conn->serialPrint(s, needCheck); }
            int mqttPrint(const String & data)
            { return conn->mqttPrint(data); }
            char * serialLastRead() { return conn->serialLastRead(); }
            void aligenieType(blinker_at_aligenie_t _type)
            { conn->aligenieType(_type); }
            void duerType(blinker_at_dueros_t _type)
            { conn->duerType(_type); }
            char * deviceId()   { return conn->deviceId(); }
            char * uuid()       { return conn->uuid(); }
            void softAPinit()   { conn->softAPinit(); }
            void smartconfig()  { conn->smartconfig(); }
            int autoInit()      { return conn->autoInit(); }
            void connectWiFi(String _ssid, String _pswd) { return conn->connectWiFi(_ssid, _pswd); }
            void connectWiFi(const char* _ssid, const char* _pswd) { return conn->connectWiFi(_ssid, _pswd); }
        #endif
        void checkFormat();
        void checkAutoFormat();
        char* dataParse()       { if (canParse) return conn->lastRead(); else return NULL; }
        char* lastRead()        { return conn->lastRead(); }
        void isParsed()         { flush(); }
        int parseState()        { return canParse; }
        void printNow();
        void _timerPrint(const String & n);
        void _print(char * n, bool needCheckLength = true);

        void autoFormatData(const String & key, const String & jsonValue);
};

void BlinkerProtocol::flush()
{
    isFresh = false; availState = false; 
    canParse = false; isAvail = false;
    if (isInit) conn->flush();
}

void BlinkerProtocol::print(const String & data)
{
    checkFormat();
    strcpy(_sendBuf, data.c_str());
    _print(_sendBuf);
    free(_sendBuf);
    autoFormat = false;
    BLINKER_LOG_FreeHeap_ALL();
}

void BlinkerProtocol::print(const String & key, const String & data)
{
    checkFormat();
    autoFormatData(key, data);
    if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
    {
        autoFormatFreshTime = millis();
    }
}

int BlinkerProtocol::checkAvail()
{
    if (!isInit) return false;

    flush();

    if (connected())
    {
        isAvail = conn->available();
        if (isAvail)
        {
            isFresh = true;
            canParse = true;
            availState = true;
        }
    }

    return isAvail;
}

void BlinkerProtocol::checkFormat()
{
    if (!autoFormat)
    {
        autoFormat = true;
        _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
    }
}

void BlinkerProtocol::checkAutoFormat()
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
            BLINKER_LOG_FreeHeap_ALL();
        }
    }
}

void BlinkerProtocol::printNow()
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
        BLINKER_LOG_FreeHeap_ALL();
    }
}

void BlinkerProtocol::_timerPrint(const String & n)
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

void BlinkerProtocol::_print(char * n, bool needCheckLength)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (strlen(n) <= BLINKER_MAX_SEND_SIZE || \
        !needCheckLength)
    {
        // BLINKER_LOG_FreeHeap_ALL();
        conn->print(n, isCheck);
        if (!isCheck) isCheck = true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}

void BlinkerProtocol::autoFormatData(const String & key, const String & jsonValue)
{
    #if defined(BLINKER_ARDUINOJSON)
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
    #else
        String data;

        BLINKER_LOG_ALL(BLINKER_F("autoFormatData data: "), jsonValue);
        BLINKER_LOG_ALL(BLINKER_F("strlen(_sendBuf): "), strlen(_sendBuf));
        BLINKER_LOG_ALL(BLINKER_F("data.length(): "), jsonValue.length());

        if ((strlen(_sendBuf) + jsonValue.length()) >= BLINKER_MAX_SEND_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
            return;
        }

        if (strlen(_sendBuf) > 0) {
            data = "," + jsonValue;
            strcat(_sendBuf, data.c_str());
        }
        else {
            data = "{" + jsonValue;
            strcpy(_sendBuf, data.c_str());
        }
    #endif
}

#endif
