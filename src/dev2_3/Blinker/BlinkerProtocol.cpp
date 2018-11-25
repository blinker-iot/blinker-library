#include "Blinker/BlinkerProtocol.h"
#include "utility/BlinkerDebug.h"
#include "modules/ArduinoJson/ArduinoJson.h"

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

template <class Transp>
bool BlinkerProtocol<Transp>::connect(uint32_t timeout)
{
    state = CONNECTING;

    uint32_t startTime = millis();
    while ( (state != CONNECTED) && \
        (millis() - startTime) < timeout )
    {
        run();
    }

    return state == CONNECTED;
}

template <class Transp>
void BlinkerProtocol<Transp>::disconnect() 
{
    conn.disconnect();
    state = DISCONNECTED;
}

template <class Transp>
bool BlinkerProtocol<Transp>::available()
{
    bool _avail = availState;
    availState = false;
    return _avail;
}

template <class Transp>
String BlinkerProtocol<Transp>::readString()
{
    if (isFresh)
    {
        isFresh = false;
        String r_data = conn.lastRead();
        flush();
        return r_data;
    }
    else
    {
        return BLINKER_F("");
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::flush()
{
    conn.flush();
    isFresh = false;
    availState = false;
    canParse = false;
    isAvail = false;
}

//template <typename T1, typename T2, typename T3>
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

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1, typename T2, typename T3>
void BlinkerProtocol<Transp>::println(T1 n1, T2 n2, T3 n3)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(n2);
    _msg += BLINKER_CMD_INTERSPACE;
    _msg += STRING_format(n3);
    _msg += BLINKER_F("\"");

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::printArray(T1 n1, const String &s2)
{
    if (!autoFormat)
    {
        autoFormat = true;
        _sendBuf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(_sendBuf, '\0', BLINKER_MAX_SEND_SIZE);
    }
#if defined(ESP8266) || defined(ESP32)
    autoFormatData(STRING_format(n1), "\"" + STRING_format(n1) + "\":" + s2);
#else
    autoFormatData("\"" + STRING_format(n1) + "\":" + s2);
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
    
    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += s2;
    _msg += BLINKER_F("\"");

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, const char str2[])
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(str2);
    _msg += BLINKER_F("\"");

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, char c)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(c);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned char b)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(b);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, unsigned long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T1>
void BlinkerProtocol<Transp>::print(T1 n1, double n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    if (!autoFormat)
    {
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
}

template <class Transp> template <typename T>
void BlinkerProtocol<Transp>::notify(T n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(BLINKER_CMD_NOTICE);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(n);
    _msg += BLINKER_F("\"");
    
    if (!autoFormat)
    {
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
}

// template <class Transp>
// void BlinkerProtocol<Transp>::begin()
// {
//     BLINKER_LOG(BLINKER_F(""));
// #if defined(BLINKER_NO_LOGO)
//     BLINKER_LOG(BLINKER_F("Blinker v"BLINKER_VERSION"\n"
//                 "    Give Blinker a Github star, thanks!\n"
//                 "    => https://github.com/blinker-iot/blinker-library\n"));
// #elif defined(BLINKER_LOGO_3D)
//     BLINKER_LOG(BLINKER_F("\n"
//         " ____    ___                __                       \n"
//         "/\\  _`\\ /\\_ \\    __        /\\ \\               v"BLINKER_VERSION"\n"
//         "\\ \\ \\L\\ \\//\\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n"
//         " \\ \\  _ <'\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n"
//         "  \\ \\ \\L\\ \\\\_\\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/  \n"
//         "   \\ \\____//\\____\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n"
//         "    \\/___/ \\/____/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n"
//         "   Give Blinker a Github star, thanks!\n"
//         "   => https://github.com/blinker-iot/blinker-library\n"));
// #else
//     BLINKER_LOG(BLINKER_F("\n"
//         "   ___  ___      __    v"BLINKER_VERSION"\n"
//         "  / _ )/ (_)__  / /_____ ____\n"
//         " / _  / / / _ \\/  '_/ -_) __/\n"
//         "/____/_/_/_//_/_/\\_\\\\__/_/   \n"
//         "Give Blinker a github star, thanks!\n"
//         "=> https://github.com/blinker-iot/blinker-library\n"));
// #endif
// }

template <class Transp>
void BlinkerProtocol<Transp>::begin(const char* _auth)
{
    // begin();
    _authKey = _auth;
    // strcpy(_authKey, _auth);
}

template <class Transp>
void BlinkerProtocol<Transp>::_print(const String & n, bool needParse, bool needCheckLength)
{
    // String data = n + BLINKER_CMD_NEWLINE;
    BLINKER_LOG_ALL(BLINKER_F("print: "), n);
    
    if (n.length() <= BLINKER_MAX_SEND_SIZE || !needCheckLength)
    {
        conn.print(n);
        // if (needParse) {
        //     BApi::parse(data, true);
        // }
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::run()
{
    if (!_isInit)
    {
        if (conn.init() && BApi::ntpInit())
        {
            _isInit =true;
            _disconnectTime = millis();

            BApi::loadOTA();
            
            BLINKER_LOG_ALL(BLINKER_F("MQTT conn init success"));
        }
    }
    else {
        if (((millis() - _disconnectTime) > 60000 && _disconnectCount) 
            || _disconnectCount >= 12)
        {
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

        // BApi::ntpInit();
    }

    if ((millis() - _refreshTime) >= BLINKER_ONE_DAY_TIME * 2 * 1000)
    {
    // if ((millis() - _refreshTime) >= 120000) {
        conn.disconnect();

        BLINKER_LOG_ALL(BLINKER_F("device reRegister"));
        BLINKER_LOG_FreeHeap();

        if (BLINKER_FreeHeap() < 15000)
        {
            conn.disconnect();
            return;
        }

        BLINKER_LOG_FreeHeap();

        if (conn.reRegister())
        {
            _refreshTime = millis();
        }
    }

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

    bool conState = conn.connected();

    switch (state)
    {
        case CONNECTING :
            if (conn.connect())
            {
                state = CONNECTED;
                _disconnectCount = 0;
            }
            else
            {
                if (_isInit)
                {
                    if (_disconnectCount == 0)
                    {
                        _disconnectCount++;
                        _disconnectTime = millis();
                        _disFreshTime = millis();
                    }
                    else {
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
            }
            break;
        case CONNECTED :
            if (conState)
            {
                checkAvail();
                if (isAvail)
                {
                    //BApi::parse(dataParse());
                }
                // if (checkAliAvail())
                // {
                //     BApi::aliParse(conn.lastRead());
                // }
            }
            else
            {
                // state = DISCONNECTED;
                conn.disconnect();
                state = CONNECTING;
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
            }
            break;
        case DISCONNECTED :
            conn.disconnect();
            state = CONNECTING;
            break;
    }

    checkAutoFormat();
}