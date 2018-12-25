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
        int connected()     { return isInit ? state == CONNECTED : false; }
        void disconnect()   { if (isInit) { conn->disconnect(); state = DISCONNECTED; } }
        void flush();
        void checkState(bool state = true)      { isCheck = state; }
        void print(const String & data);
        void print(const String & key, const String & data);

        #if defined(BLINKER_MQTT)
            char * deviceName() { if (isInit) return conn->deviceName(); else return NULL; }
            char * authKey()    { if (isInit) return conn->authKey(); else return NULL;  }
            int init()          { return isInit ? conn->init() : false; }
            int mConnected()    { return isInit ? conn->mConnected() : false; }
            int autoPrint(uint32_t id)  { return isInit ? conn->autoPrint(id) : false; }
            int bPrint(char * name, const String & data) { conn->bPrint(name, data); }
            int aliPrint(const String & data) { conn->aliPrint(data); }
            int duerPrint(const String & data) { conn->duerPrint(data); }
            void freshAlive() { conn->freshAlive(); }
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
        void checkFormat();
        void checkAutoFormat();
        char* dataParse()   { if (canParse) return conn->lastRead(); else return NULL; }
        char* lastRead()    { return conn->lastRead(); }
        void isParsed()     { flush(); }
        int parseState()   { return canParse; }
        void printNow();
        void _timerPrint(const String & n);
        void _print(char * n, bool needCheckLength = true);

        void autoFormatData(const String & key, const String & jsonValue)
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
};

#endif
