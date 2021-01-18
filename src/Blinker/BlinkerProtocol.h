#ifndef BLINKER_PROTOCOL_H
#define BLINKER_PROTOCOL_H

#include "BlinkerConfig.h"
#include "BlinkerDebug.h"
#include "BlinkerStream.h"
#include "BlinkerUtility.h"

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

    // #if defined(BLINKER_LOWPOWER_AIR202)
    //     void print(const String & data);
    //     void print(const String & key, const String & data);
    //     char * deviceName() { if (isInit) return conn->deviceName(); else return ""; }
    //     char * authKey()    { if (isInit) return conn->authKey(); else return "";  }
    //     int init()          { return isInit ? conn->init() : false; }
    //     void begin(const char* _key, const char* _type, String _imei)
    //     { conn->begin(_key, _type, _imei); }
    //     int deviceRegister(){ return conn->deviceRegister(); }
    // #else
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
        // bool available()    { if (availState) {availState = false; return true;} else return false; }
        void flush();
        void checkState(bool state = true)      { isCheck = state; }
        void print(const String & data);
        void print(const String & key, const String & data);

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
            defined(BLINKER_AT_MQTT) || defined(BLINKER_MQTT_AT) || \
            defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_NBIOT_SIM7020) || \
            defined(BLINKER_GPRS_AIR202) || defined(BLINKER_PRO_SIM7020) || \
            defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
            defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_SUBDEVICE) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7000) || defined(BLINKE_HTTP)
            int aliPrint(const String & data)   { return isInit ? conn->aliPrint(data) : false; }
            int duerPrint(const String & data, bool report = false)  { return isInit ? conn->duerPrint(data, report) : false; }
            #if !defined(BLINKER_GPRS_AIR202) && !defined(BLINKER_NBIOT_SIM7020) && \
                !defined(BLINKER_PRO_SIM7020) && !defined(BLINKER_PRO_AIR202) && \
                !defined(BLINKER_QRCODE_NBIOT_SIM7020) && !defined(BLINKER_NBIOT_SIM7000) && \
                !defined(BLINKER_QRCODE_NBIOT_SIM7000)
            int miPrint(const String & data)  { return isInit ? conn->miPrint(data) : false; }
            #endif
            // void ping() { if (isInit) conn->ping(); }
            #if !defined(BLINKER_MQTT_AT)
            int bPrint(char * name, const String & data) { return isInit ? conn->bPrint(name, data) : false; }
            int autoPrint(unsigned long id)  { return isInit ? conn->autoPrint(id) : false; }
            void sharers(const String & data) { if (isInit) conn->sharers(data); }
            int needFreshShare() { if (isInit) return conn->needFreshShare(); else return false; }
            #endif
        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
            defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
            defined(BLINKER_GPRS_AIR202) || defined(BLINKER_NBIOT_SIM7020) || \
            defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
            defined(BLINKER_WIFI_SUBDEVICE) || defined(BLINKER_QRCODE_NBIOT_SIM7020) || \
            defined(BLINKER_NBIOT_SIM7000) || defined(BLINKER_QRCODE_NBIOT_SIM7000) || \
            defined(BLINKE_HTTP)
            int toServer(char * data) { return isInit ? conn->toServer(data) : false; }
            char * deviceName() { if (isInit) return conn->deviceName(); else return ""; }
            char * authKey()    { if (isInit) return conn->authKey(); else return "";  }
            char * token()    { if (isInit) return conn->token(); else return "";  }
            int init()          { return isInit ? conn->init() : false; }
            int mConnected()    { return isInit ? conn->mConnected() : false; }
            void freshAlive() { if (isInit) conn->freshAlive(); }
        #endif

        #if defined(BLINKER_LOWPOWER_AIR202)
            char * deviceName() { if (isInit) return conn->deviceName(); else return ""; }
            char * authKey()    { if (isInit) return conn->authKey(); else return "";  }
            char * token()    { if (isInit) return conn->token(); else return "";  }
            int init()          { return isInit ? conn->init() : false; }
            void begin(const char* _key, const char* _type, String _imei) { conn->begin(_key, _type, _imei); }
            int deviceRegister(){ return conn->deviceRegister(); }
        #endif

        #if defined(BLINKER_PRO) || defined(BLINKER_MQTT_AUTO) || \
            defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY) || \
            defined(BLINKER_WIFI_SUBDEVICE)
            int deviceRegister(){ return conn->deviceRegister(); }
            int authCheck()     { return conn->authCheck(); }
            #if defined(BLINKER_PRO)
            void begin(const char* _deviceType) { conn->begin(_deviceType); }
            #elif defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
                defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_WIFI_SUBDEVICE)
            void begin(const char* _key, const char* _type) { conn->begin(_key, _type); }
            #endif
        #elif defined(BLINKER_GPRS_AIR202) || defined(BLINKER_NBIOT_SIM7020) || \
            defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7000)
            int deviceRegister(){ return conn->deviceRegister(); }

            #if defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_QRCODE_NBIOT_SIM7000)
                void begin(const char* _authKey, const char* _deviceType, String _imei)
                { conn->begin(_authKey, _deviceType, _imei); }
            #else
                void begin(const char* _deviceType, String _imei)
                { conn->begin(_deviceType, _imei); }
            #endif

            #if defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202)
                int authCheck()     { return conn->authCheck(); }
            #endif
        #endif

        // #if defined(BLINKER_WIFI_SUBDEVICE)
        //     void attachSubAvailable(blinker_callback_return_int_t func)
        //     { if (isInit) conn->attachAvailable(func); }
            
        //     void attachSubRead(blinker_callback_return_string_t func)
        //     { if (isInit) conn->attachRead(func); }

        //     void attachSubPrint(blinker_callback_with_string_arg_t func)
        //     { if (isInit) conn->attachPrint(func); }

        //     void attachSubBegin(blinker_callback_t func)
        //     { if (isInit) conn->attachBegin(func); }

        //     void attachSubConnect(blinker_callback_return_int_t func)
        //     { if (isInit) conn->attachConnect(func); }

        //     void attachSubConnected(blinker_callback_return_int_t func)
        //     { if (isInit) conn->attachConnected(func); }

        //     void attachSubDisconnect(blinker_callback_t func)
        //     { if (isInit) conn->attachDisconnect(func); }
        // #endif

        #if defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_WIFI_SUBDEVICE)
            void meshCheck() { conn->meshCheck(); }
            #if !defined(BLINKER_WIFI_SUBDEVICE)
                void setTimezone(float tz) { conn->setTimezone(tz); }
            #endif
            #if defined(BLINKER_WIFI_SUBDEVICE)
                int subPrint(const String & data) { return conn->subPrint(data); }
                int meshAvail() { return conn->meshAvail(); }
                char * meshLastRead() { return conn->meshLastRead(); }
                void meshFlush() { conn->meshFlush(); }
            #endif
        #endif

        #if defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY)
            void smartConfigType() { conn->setSmartConfig(); }
            void apConfigType() { conn->setApConfig(); }
            bool checkIsSmartConfig() { return conn->checkSmartConfig(); }
        #endif

        #if defined(BLINKER_HTTP)
            void subscribe() { conn->subscribe(); }
        #endif
    // #endif
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

    // #if defined(BLINKER_LOWPOWER_AIR202)
    //     void checkFormat();
    //     void autoFormatData(const String & key, const String & jsonValue);
    // #else
        int checkAvail();
        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
            defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
            defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
            defined(BLINKER_WIFI_SUBDEVICE) || defined(BLINKE_HTTP)
            bool checkAliAvail()    { return conn->aligenieAvail(); }
            bool checkDuerAvail()   { return conn->duerAvail(); }
            #if !defined(BLINKER_GPRS_AIR202) && !defined(BLINKER_NBIOT_SIM7020) && \
                !defined(BLINKER_PRO_SIM7020) && !defined(BLINKER_PRO_AIR202) && \
                !defined(BLINKER_QRCODE_NBIOT_SIM7020) && !defined(BLINKER_NBIOT_SIM7000) && \
                !defined(BLINKER_QRCODE_NBIOT_SIM7000)
            bool checkMIOTAvail()   { return conn->miAvail(); }
            #endif
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
        char* dataParse()       { if (canParse) return conn->lastRead(); else return ""; }
        char* lastRead()        { return conn->lastRead(); }
        void isParsed()         { BLINKER_LOG_ALL(BLINKER_F("isParsed")); flush(); }
        int parseState()        { return canParse; }
        int printNow();
        void _timerPrint(const String & n);
        int _print(char * n, bool needCheckLength = true);

        void autoFormatData(const String & key, const String & jsonValue);
    // #endif
};

// #if !defined(BLINKER_LOWPOWER_AIR202)
void BlinkerProtocol::flush()
{
    if (isInit && isAvail) conn->flush();
    isAvail = false;
    isFresh = false;
    availState = false; 
    canParse = false;
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
            BLINKER_LOG_ALL(BLINKER_F("checkAvail: "), isAvail);

            isFresh = true;
            canParse = true;
            availState = true;
        }
    }

    return isAvail;
}

void BlinkerProtocol::checkAutoFormat()
{
    if (autoFormat)
    {
        if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
        {
            if (strlen(_sendBuf))
            {
                // #if !defined(BLINKER_LOWPOWER_AIR202)
                #if defined(BLINKER_ARDUINOJSON)
                    _print(_sendBuf);
                #else
                    strcat(_sendBuf, "}");
                    _print(_sendBuf);
                #endif
                // #endif
            }
            free(_sendBuf);
            autoFormat = false;
            BLINKER_LOG_FreeHeap_ALL();
        }
    }
}

int BlinkerProtocol::printNow()
{
    if (strlen(_sendBuf) && autoFormat)
    {
        int8_t print_state = BLINKER_ERROR;
        #if defined(BLINKER_ARDUINOJSON)
            if (_print(_sendBuf)) print_state = BLINKER_SUCCESS;
        #else
            strcat(_sendBuf, "}");
            if (_print(_sendBuf)) print_state = BLINKER_SUCCESS;
        #endif

        free(_sendBuf);
        autoFormat = false;
        BLINKER_LOG_FreeHeap_ALL();

        return print_state;
    }

    return BLINKER_ERROR;
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

int BlinkerProtocol::_print(char * n, bool needCheckLength)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (strlen(n) <= BLINKER_MAX_SEND_SIZE || \
        !needCheckLength)
    {
        // BLINKER_LOG_FreeHeap_ALL();
        BLINKER_LOG_ALL(BLINKER_F("Proto print..."));
        BLINKER_LOG_FreeHeap_ALL();
        conn->print(n, isCheck);
        if (!isCheck) isCheck = true;

        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));

        return false;
    }
}

void BlinkerProtocol::print(const String & data)
{
    #if !defined(BLINKER_LOWPOWER_AIR202)
    checkFormat();
    strcpy(_sendBuf, data.c_str());
    _print(_sendBuf);
    free(_sendBuf);
    autoFormat = false;
    BLINKER_LOG_FreeHeap_ALL();
    #endif
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

void BlinkerProtocol::checkFormat()
{
    if (!autoFormat)
    {
        autoFormat = true;
        _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
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

            // DynamicJsonBuffer jsonSendBuffer;
            DynamicJsonDocument jsonBuffer(1024);

            if (strlen(_sendBuf)) {
                BLINKER_LOG_ALL(BLINKER_F("add"));

                // JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_sendBuf));
                deserializeJson(jsonBuffer, STRING_format(_sendBuf));
                JsonObject root = jsonBuffer.as<JsonObject>();

                if (root.containsKey(key)) {
                    root.remove(key);
                }
                // root.printTo(_data);
                serializeJson(root, _data);

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

        if (_data.length() > BLINKER_MAX_SEND_BUFFER_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT: "), BLINKER_MAX_SEND_BUFFER_SIZE);
            return;
        }

        strcpy(_sendBuf, _data.c_str());
    #else
        String data;

        BLINKER_LOG_ALL(BLINKER_F("autoFormatData data: "), jsonValue);
        BLINKER_LOG_ALL(BLINKER_F("strlen(_sendBuf): "), strlen(_sendBuf));
        BLINKER_LOG_ALL(BLINKER_F("data.length(): "), jsonValue.length());

        if ((strlen(_sendBuf) + jsonValue.length()) >= BLINKER_MAX_SEND_BUFFER_SIZE)
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

// #elif defined(BLINKER_LOWPOWER_AIR202)

// void BlinkerProtocol::print(const String & data)
// {
//     #if !defined(BLINKER_LOWPOWER_AIR202)
//     checkFormat();
//     strcpy(_sendBuf, data.c_str());
//     _print(_sendBuf);
//     free(_sendBuf);
//     autoFormat = false;
//     BLINKER_LOG_FreeHeap_ALL();
//     #endif
// }

// void BlinkerProtocol::print(const String & key, const String & data)
// {
//     checkFormat();
//     autoFormatData(key, data);
//     if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
//     {
//         autoFormatFreshTime = millis();
//     }
// }

// void BlinkerProtocol::checkFormat()
// {
//     if (!autoFormat)
//     {
//         autoFormat = true;
//         _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
//         memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
//     }
// }

// void BlinkerProtocol::autoFormatData(const String & key, const String & jsonValue)
// {
//     #if defined(BLINKER_ARDUINOJSON)
//         BLINKER_LOG_ALL(BLINKER_F("autoFormatData key: "), key, 
//                         BLINKER_F(", json: "), jsonValue);
        
//         String _data;

//         if (STRING_contains_string(STRING_format(_sendBuf), key))
//         {

//             DynamicJsonBuffer jsonSendBuffer;                

//             if (strlen(_sendBuf)) {
//                 BLINKER_LOG_ALL(BLINKER_F("add"));

//                 JsonObject& root = jsonSendBuffer.parseObject(STRING_format(_sendBuf));

//                 if (root.containsKey(key)) {
//                     root.remove(key);
//                 }
//                 root.printTo(_data);

//                 _data = _data.substring(0, _data.length() - 1);

//                 if (_data.length() > 4 ) _data += BLINKER_F(",");
//                 _data += jsonValue;
//                 _data += BLINKER_F("}");
//             }
//             else {
//                 BLINKER_LOG_ALL(BLINKER_F("new"));
                
//                 _data = BLINKER_F("{");
//                 _data += jsonValue;
//                 _data += BLINKER_F("}");
//             }
//         }
//         else {
//             _data = STRING_format(_sendBuf);

//             if (_data.length())
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("add."));

//                 _data = _data.substring(0, _data.length() - 1);

//                 _data += BLINKER_F(",");
//                 _data += jsonValue;
//                 _data += BLINKER_F("}");
//             }
//             else {
//                 BLINKER_LOG_ALL(BLINKER_F("new."));
                
//                 _data = BLINKER_F("{");
//                 _data += jsonValue;
//                 _data += BLINKER_F("}");
//             }
//         }

//         if (_data.length() > BLINKER_MAX_SEND_BUFFER_SIZE)
//         {
//             BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT: "), BLINKER_MAX_SEND_BUFFER_SIZE);
//             return;
//         }

//         strcpy(_sendBuf, _data.c_str());
//     #else
//         String data;

//         BLINKER_LOG_ALL(BLINKER_F("autoFormatData data: "), jsonValue);
//         BLINKER_LOG_ALL(BLINKER_F("strlen(_sendBuf): "), strlen(_sendBuf));
//         BLINKER_LOG_ALL(BLINKER_F("data.length(): "), jsonValue.length());

//         if ((strlen(_sendBuf) + jsonValue.length()) >= BLINKER_MAX_SEND_BUFFER_SIZE)
//         {
//             BLINKER_ERR_LOG(BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT"));
//             return;
//         }

//         if (strlen(_sendBuf) > 0) {
//             data = "," + jsonValue;
//             strcat(_sendBuf, data.c_str());
//         }
//         else {
//             data = "{" + jsonValue;
//             strcpy(_sendBuf, data.c_str());
//         }
//     #endif
// }
// #endif

#endif
