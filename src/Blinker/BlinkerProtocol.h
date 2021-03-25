#ifndef BLINKER_PROTOCOL_H
#define BLINKER_PROTOCOL_H

#include "Blinker/BlinkerApi.h"

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
            , conn(transp) {}

        void begin();
        void run();

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
        void printArray(T1 n1, const String &s2);

        // template <typename T1>
        // void printNumArray(char * _name, const String & data);

        template <typename T1>
        void printObject(T1 n1, const String &s2);

        void flush();

        void checkState(bool state = true)      { isCheck = state; }

    #if defined(BLINKER_MIOT)
        void miotPrint(const String & _msg)       { conn.miPrint(_msg); }
    #endif
    private :
        void autoPrint(const String & key, const String & data);
        void autoFormatData(const String & key, const String & jsonValue);
        void checkAutoFormat();
        void printNow();
        int  _print(char * n, bool needCheckLength = true);
        
        char                _sendBuf[BLINKER_MAX_SEND_SIZE];
        uint32_t            autoFormatFreshTime;
        bool                autoFormat = false;
        bool                isCheck = true;

    protected :
        Transp&             conn;
        _blinker_state_t    state;
        bool                isAvail;
};

template <class Transp>
void BlinkerProtocol<Transp>::begin()
{
    BLINKER_LOG(BLINKER_F(""));
    #if defined(BLINKER_NO_LOGO)
        BLINKER_LOG(BLINKER_F("blinker v"), BLINKER_VERSION, BLINKER_F("\n"),
                    BLINKER_F("    To better use blinker with your IoT project!\n"),
                    BLINKER_F("    Download latest blinker library here!\n"),
                    BLINKER_F("    => https://github.com/blinker-iot/blinker-library\n"));
    #elif defined(BLINKER_LOGO_3D)
        BLINKER_LOG(BLINKER_F("\n"),
                    BLINKER_F(" __       __                __\n"),
                    BLINKER_F("/\\ \\     /\\ \\    __        /\\ \\              v"), BLINKER_VERSION, BLINKER_F("\n"),
                    BLINKER_F("\\ \\ \\___ \\ \\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"),
                    BLINKER_F(" \\ \\ '__`\\\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"),
                    BLINKER_F("  \\ \\ \\L\\ \\\\ \\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\./ \n"),
                    BLINKER_F("   \\ \\_,__/ \\ \\__\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"),
                    BLINKER_F("    \\/___/   \\/__/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"),
                    BLINKER_F("    To better use blinker with your IoT project!\n"),
                    BLINKER_F("    Download latest blinker library here!\n"),
                    BLINKER_F("    => https://github.com/blinker-iot/blinker-library\n"));
    #else
        BLINKER_LOG(BLINKER_F("\n"),
                    BLINKER_F(".   .        .   v"), BLINKER_VERSION, BLINKER_F("\n"),
                    BLINKER_F("|-. |  . ,-. | , ,-. ,-.\n"),
                    BLINKER_F("| | |  | | | |<  |-' |\n"),
                    BLINKER_F("`-' `' ' ' ' ' ` `-' '\n"),
                    BLINKER_F("To better use blinker with your IoT project!\n"),
                    BLINKER_F("Download latest blinker library here!\n"),
                    BLINKER_F("=> https://github.com/blinker-iot/blinker-library\n"));
    #endif
}

template <class Transp>
void BlinkerProtocol<Transp>::run()
{
    switch (state)
    {
        case CONNECTING :
            if (conn.connect())
            {
                state = CONNECTED;
            }
            break;

        case CONNECTED :
            if (!conn.connect())
            {
                state = DISCONNECTED;
            }
            else if (conn.available())
            {
                isAvail = true;
                BApi::parse(conn.lastRead());
            }
            else if (conn.miAvail())
            {
                BApi::miotParse(conn.lastRead());
            }
            break;
        
        case DISCONNECTED :
            conn.disconnect();
            state = CONNECTING;
            break;

        default:
            break;
    }

    checkAutoFormat();
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += s2;
    _msg += BLINKER_F("\"");

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, const char str2[])
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(str2);
    _msg += BLINKER_F("\"");

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, char c)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(c);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned char b)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(b);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, double n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::printArray(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += s2;

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::printObject(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += s2;

    autoPrint(STRING_format(n1), _msg);
}

template <class Transp>
void BlinkerProtocol<Transp>::autoPrint(const String & key, const String & data)
{
    if (!autoFormat) autoFormat = true;

    autoFormatData(key, data);
    
    if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
    {
        autoFormatFreshTime = millis();
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::autoFormatData(const String & key, const String & jsonValue)
{
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
                _print(_sendBuf);
            }
            memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
            autoFormat = false;
            BLINKER_LOG_FreeHeap_ALL();
        }
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::printNow()
{
    if (strlen(_sendBuf) && autoFormat)
    {
        _print(_sendBuf);
        
        memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
        autoFormat = false;
    }
}

template <class Transp>
int BlinkerProtocol<Transp>::_print(char * n, bool needCheckLength)
{
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (strlen(n) <= BLINKER_MAX_SEND_SIZE || \
        !needCheckLength)
    {
        // BLINKER_LOG_FreeHeap_ALL();
        BLINKER_LOG_ALL(BLINKER_F("Proto print..."));
        BLINKER_LOG_FreeHeap_ALL();
        conn.print(n, isCheck);
        if (!isCheck) isCheck = true;

        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));

        return false;
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::flush()
{ 
    conn.flush();
    isAvail = false;
}

#endif
