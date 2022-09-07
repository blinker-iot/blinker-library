#ifndef BLINKER_PROTOCOL_H
#define BLINKER_PROTOCOL_H

#include <time.h>

#include "Blinker/BlinkerApi.h"
#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_AT)
    #include "Blinker/BlinkerOTA.h"

    #if defined(ESP8266) && !defined(BLINKER_BLE)
        #if defined(BLINKER_WIFI_MULTI)
            extern ESP8266WiFiMulti wifiMulti;
        #endif
    #elif defined(ESP32) && !defined(BLINKER_BLE)
        #if defined(BLINKER_WIFI_MULTI)
            extern WiFiMulti wifiMulti;
        #endif
    #endif
#endif

#if defined(BLINKER_WIFI_AT)
    #include "Blinker/BlinkerMQTTATBase.h"
#endif

static const char *TAG_PROTO = "[BlinkerPortocol] ";

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
    #if defined(BLINKER_WIFI_AT)
        void atBegin();
    #endif
        void begin();
        void run();
        void delay(unsigned long ms);
        template <typename T>
        void notify(T n);
        void vibrate(uint16_t ms = 200);
        b_device_staus_t status()               { return conn.status(); }
        char * lastRead()                       { return conn.lastRead(); }
        bool init()                             { return conn.init(); }

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

        void setTimezone(float tz)              { _timezone = tz; _isNTPInit = false; }
        float getTimezone()                     { return _timezone; }

        int8_t second();
        int8_t minute();
        int8_t hour();
        int8_t mday();
        int8_t wday();
        int8_t month();
        int16_t year();
        int16_t yday();
        time_t  time();
        int32_t dtime();
        time_t  startTime();
        time_t  runTime();

        // template<typename T>
        // bool sms(const T& msg)
        // { return conn.sms(msg); }

        void reset()                            { conn.reset(); }

#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_AT)
        template<typename T>
        bool sms(const T& msg, const String & cel = "");

        template<typename T>
        bool push(const T& msg, const String & users = "");

        template<typename T>
        bool wechat(const T& msg);

        template<typename T>
        bool wechat(const String & title, const String & state, const T& msg, const String & users = "");

        void weather(uint32_t _city = 0);
        void weatherForecast(uint32_t _city = 0);
        void air(uint32_t _city = 0);
        bool log(const String & msg);

        void attachAir(blinker_callback_with_string_arg_t newFunction)
        { _airFunc = newFunction; }
        void attachWeather(blinker_callback_with_string_arg_t newFunction)
        { _weatherFunc = newFunction; }
        void attachWeatherForecast(blinker_callback_with_string_arg_t newFunction)
        { _weather_forecast_Func = newFunction; }

    #if defined(BLINKER_ALIGENIE)
        bool aliAvail()                         { return conn.aliAvail(); }
        void aliPrint(const String & _msg)      { conn.aliPrint(_msg); }
    #endif

    #if defined(BLINKER_DUEROS)
        bool duerAvail()                        { return conn.duerAvail(); }
        void duerPrint(const String & _msg, bool report = false)
        { conn.duerPrint(_msg, report); }
    #endif

    #if defined(BLINKER_MIOT)
        bool miotAvail()                        { return conn.miAvail(); }
        void miotPrint(const String & _msg)     { conn.miPrint(_msg); }
    #endif

        template<typename T>
        void dataStorage(char _name[], const T& msg);
        void attachDataStorage(blinker_callback_t newFunction, uint32_t _time = 60, uint8_t d_times = BLINKER_DATA_UPDATE_COUNT);
        
        template<typename T>
        void timeSlotData(char _name[], const T& _data);
        void textData(const String & msg);
        void configUpdate(const String & msg);
        void configGet();
#endif
    private :
        void autoPrint(const String & key, const String & data);
        void autoFormatData(const String & key, const String & jsonValue);
        void checkAutoFormat();
        void printNow();
        int  _print(char * n, bool needCheckLength = true);
        void _timerPrint(const String & n);

        bool wlanCheck();
        bool ntpConfig();
        bool ntpInit();

#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_AT)
        void checkDataStorage();
        bool dataUpdate();

        bool dataStorage(const String & msg);
        bool timeSlot(const String & msg);
        void httpHeartbeat();

    #if defined(BLINKER_PRO_ESP)
        void loadOTA();
        void ota();
        String checkOTA();
        bool updateOTAStatus(int8_t status, const String & msg);
        void otaStatus(int8_t status, const String & msg);
        void otaInit();
        // void otaParse(const JsonObject& data);
        BlinkerOTA                      _OTA;
    #endif
#endif        
        char                _sendBuf[BLINKER_MAX_SEND_SIZE];
        uint32_t            autoFormatFreshTime;
        bool                autoFormat = false;
        bool                isCheck = true;
        uint32_t            _reconTime = 0;

        bool                _isNTPInit = false;
        uint32_t            _ntpStart;
        float               _timezone = 8.0;
        time_t              _deviceStartTime = 0;
        uint32_t            _dHeartTime = 0;

    protected :
        Transp&             conn;
        _blinker_state_t    state;
        bool                isAvail;
    #if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
        defined(BLINKER_WIFI_AT)
        blinker_callback_with_string_arg_t  _airFunc = NULL;
        blinker_callback_with_string_arg_t  _weatherFunc = NULL;
        blinker_callback_with_string_arg_t  _weather_forecast_Func = NULL;

        class BlinkerData *                 _Data[BLINKER_MAX_BLINKER_DATA_SIZE];
        class BlinkerTimeSlotData *         _TimeSlotData[BLINKER_MAX_BLINKER_DATA_SIZE];
        
        blinker_callback_t                  _dataStorageFunc = NULL;
        uint32_t                            _autoStorageTime = 60;
        uint32_t                            _autoDataTime = 0;
        uint8_t                             _dataTimes = BLINKER_MAX_DATA_COUNT;
        uint32_t                            _autoUpdateTime = 0;
        uint8_t                             data_dataCount = 0;
        uint8_t                             data_timeSlotDataCount = 0;
        uint32_t                            time_timeSlotData = 0;
    #endif

    
    #if defined(BLINKER_WIFI_AT)
        bool atSerialTask();
        void parseATdata();
        void serialPrint(const String & s) { conn.serialPrint(s); }

        blinker_at_status_t     _at_status = BL_BEGIN;
        class BlinkerSlaverAT * _slaverAT;
        uint8_t                 pinDataNum = 0;
        class PinData *         _pinData[BLINKER_MAX_PIN_NUM];
        uint8_t                 _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
    #endif
};

template <class Transp>
void BlinkerProtocol<Transp>::begin()
{
    #if defined(BLINKER_WIFI_AT)
    Serial.println(BLINKER_F(""));
    Serial.print(BLINKER_F("blinker lib version: "));
    Serial.println(BLINKER_VERSION);
    #if defined(ESP8266)
    Serial.print(BLINKER_F("package version: "));
    Serial.println(ESP.getCoreVersion());
    #elif defined(ESP32)
    Serial.print(BLINKER_F("package version: "));
    Serial.println(ESP.getSdkVersion());
    #endif
    Serial.print(BLINKER_F("board type: "));
    Serial.println(ARDUINO_BOARD);
    #else
    BLINKER_LOG(TAG_PROTO, BLINKER_F(""));

    BLINKER_LOG(TAG_PROTO, BLINKER_F("blinker lib version: "), BLINKER_VERSION);
    #if defined(ESP8266)
    BLINKER_LOG(TAG_PROTO, BLINKER_F("package version: "), ESP.getCoreVersion());
    #elif defined(ESP32)
    BLINKER_LOG(TAG_PROTO, BLINKER_F("package version: "), ESP.getSdkVersion());
    #endif
    BLINKER_LOG(TAG_PROTO, BLINKER_F("board type: "), ARDUINO_BOARD);
    #endif

    #if defined(BLINKER_NO_LOGO)
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("blinker v"), BLINKER_VERSION, BLINKER_F("\n"),
                    BLINKER_F("    To better use blinker with your IoT project!\n"),
                    BLINKER_F("    Download latest blinker library here!\n"),
                    BLINKER_F("    => https://github.com/blinker-iot/blinker-library\n"));
    #elif defined(BLINKER_LOGO_3D)
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("\n"),
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
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("\n"),
                    BLINKER_F(".   .        .   v"), BLINKER_VERSION, BLINKER_F("\n"),
                    BLINKER_F("|-. |  . ,-. | , ,-. ,-.\n"),
                    BLINKER_F("| | |  | | | |<  |-' |\n"),
                    BLINKER_F("`-' `' ' ' ' ' ` `-' '\n"),
                    BLINKER_F( "To better use blinker with your IoT project!\n"),
                    BLINKER_F("Download latest blinker library here!\n"),
                    BLINKER_F("=> https://github.com/blinker-iot/blinker-library\n"));
    #endif

    #if defined(BLINKER_PRO_ESP) 
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F(
                    "\n==========================================================="
                    "\n================= Blinker PRO mode init ! ================="
                    "\n     EEPROM address 1280-1535 is used for PRO ESP Mode!"
                    "\n========= PLEASE AVOID USING THESE EEPROM ADDRESS! ========"
                    "\n===========================================================\n"));
                    
        #if defined(BLINKER_BUTTON)
            #if defined(BLINKER_BUTTON_PULLDOWN)
                BApi::buttonInit(false);
            #else
                BApi::buttonInit();
            #endif
        #endif

        #if defined(BLINKER_NO_BUTTON)
            BApi::noButtonInit();
        #endif
    #endif
}

template <class Transp>
void BlinkerProtocol<Transp>::run()
{
#if defined(BLINKER_PRO_ESP) 
    #if defined(BLINKER_BUTTON)
        BApi::tick();
    #endif

    #if defined(BLINKER_NO_BUTTON)
        BApi::noButtonCheck();
    #endif
#endif

#if defined(BLINKER_WIFI_AT)
    atBegin();

    if (_at_status == BL_BEGIN) return;
#endif

    if (!conn.init()) return;

#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_AT)
    if (!wlanCheck()) return;

    ntpInit();
    
    #if defined(BLINKER_WIDGET)
        BApi::checkTimer();
    #endif

    if (((millis() - _dHeartTime)/1000UL >= BLINKER_DEVICE_HEARTBEAT_TIME) && conn.checkInit())
    {
        _dHeartTime += BLINKER_DEVICE_HEARTBEAT_TIME * 1000;
        httpHeartbeat();
    }
#endif

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

#if defined(BLINKER_WIFI_AT)
                serialPrint(conn.lastRead());
#endif

                BApi::parse(conn.lastRead());
            }

#if defined(BLINKER_WIFI_AT)
            if (atSerialTask())
            {
                conn.mqttPrint(conn.serialLastRead());
            }
            // atSerialTask();

            

            if (conn.aliAvail() || conn.duerAvail() || conn.miAvail())
            {
                serialPrint(conn.lastRead());
            }
#endif

            // else if (conn.miAvail())
            // {
            //     BApi::miotParse(conn.lastRead());
            // }
            break;
        
        case DISCONNECTED :
            conn.disconnect();
            state = CONNECTING;
            break;

        default:
            break;
    }
#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_AT)
    checkDataStorage();
#endif
    checkAutoFormat();
}

template <class Transp>
void BlinkerProtocol<Transp>::delay(unsigned long ms)
{
    uint32_t start = micros();
    uint32_t __start = millis();
    unsigned long _ms = ms;
    while (ms > 0)
    {
        run();

        if ((micros() - start) >= 1000)
        {
            ms -= 1;
            start += 1000;
        }

        if ((millis() - __start) >= _ms)
        {
            ms = 0;
        }

        yield();
    }
}

template <class Transp> template <typename T>
void BlinkerProtocol<Transp>::notify(T n)
{
    print(BLINKER_CMD_NOTICE, STRING_format(n));
}

template <class Transp> 
void BlinkerProtocol<Transp>::vibrate(uint16_t ms)
{
    if (ms > 1000) ms = 1000;

    print(BLINKER_CMD_VIBRATE, ms);
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
    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("autoFormatData key: "), key, \
                    BLINKER_F(", json: "), jsonValue);
    
    String _data;

    if (STRING_contains_string(STRING_format(_sendBuf), key))
    {

        // DynamicJsonBuffer jsonSendBuffer;
        DynamicJsonDocument jsonBuffer(1024);

        if (strlen(_sendBuf)) {
            BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("add"));

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
            BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("new"));
            
            _data = BLINKER_F("{");
            _data += jsonValue;
            _data += BLINKER_F("}");
        }
    }
    else {
        _data = STRING_format(_sendBuf);

        if (_data.length())
        {
            BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("add."));

            _data = _data.substring(0, _data.length() - 1);

            _data += BLINKER_F(",");
            _data += jsonValue;
            _data += BLINKER_F("}");
        }
        else {
            BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("new."));
            
            _data = BLINKER_F("{");
            _data += jsonValue;
            _data += BLINKER_F("}");
        }
    }

    if (_data.length() > BLINKER_MAX_SEND_BUFFER_SIZE)
    {
        BLINKER_ERR_LOG(TAG_PROTO, BLINKER_F("FORMAT DATA SIZE IS MAX THAN LIMIT: "), BLINKER_MAX_SEND_BUFFER_SIZE);
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
    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("print: "), n);
    
    if (strlen(n) <= BLINKER_MAX_SEND_SIZE || \
        !needCheckLength)
    {
        // BLINKER_LOG_FreeHeap_ALL();
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("Proto print..."));
        BLINKER_LOG_FreeHeap_ALL();
        conn.print(n, isCheck);
        if (!isCheck) isCheck = true;

        return true;
    }
    else {
        BLINKER_ERR_LOG(TAG_PROTO, BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));

        return false;
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::_timerPrint(const String & n)
{
    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("print: "), n);
    
    if (n.length() <= BLINKER_MAX_SEND_SIZE)
    {
        if (!autoFormat) autoFormat = true;
        checkState(false);
        strcpy(_sendBuf, n.c_str());
    }
    else
    {
        BLINKER_ERR_LOG(TAG_PROTO, BLINKER_F("SEND DATA BYTES MAX THAN LIMIT!"));
    }
}

template <class Transp>
void BlinkerProtocol<Transp>::flush()
{ 
    conn.flush();
    isAvail = false;
}

template <class Transp>
int8_t BlinkerProtocol<Transp>::second()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_sec;
    }
    return -1;
}

template <class Transp>
int8_t BlinkerProtocol<Transp>::minute()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_min;
    }
    return -1;
}

template <class Transp>
int8_t BlinkerProtocol<Transp>::hour()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_hour;
    }
    return -1;
}

template <class Transp>
int8_t BlinkerProtocol<Transp>::mday()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_mday;
    }
    return -1;
}

template <class Transp>
int8_t BlinkerProtocol<Transp>::wday()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_wday;
    }
    return -1;
}

template <class Transp>
int8_t BlinkerProtocol<Transp>::month()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_mon + 1;
    }
    return -1;
}

template <class Transp>
int16_t BlinkerProtocol<Transp>::year()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_year + 1900;
    }
    return -1;
}

template <class Transp>
int16_t BlinkerProtocol<Transp>::yday()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_yday + 1;
    }
    return -1;
}

template <class Transp>
time_t BlinkerProtocol<Transp>::time()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        return now_ntp;
    }
    return millis();
}

template <class Transp>
int32_t BlinkerProtocol<Transp>::dtime()
{
    if (_isNTPInit)
    {
        time_t _ntpGetTime;

        time_t now_ntp = ::time(nullptr);

        struct tm timeinfo;

        #if defined(ESP8266)
            time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
            gmtime_r(&_now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        return timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    }
    return -1;
}

template <class Transp>
time_t BlinkerProtocol<Transp>::startTime()
{
    if (_isNTPInit) return _deviceStartTime;
    else return 0;
}

template <class Transp>
time_t BlinkerProtocol<Transp>::runTime()
{
    if (_isNTPInit)
    {
        return time() - _deviceStartTime;
    }
    else
    {
        return millis()/1000;
    }
}


#if defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP) || \
    defined(BLINKER_WIFI_AT)

template <class Transp>
bool BlinkerProtocol<Transp>::wlanCheck()
{
    #if defined(BLINKER_WIFI_MULTI)
        if (wifiMulti.run() != WL_CONNECTED)
        {
            if ((millis() - _reconTime) >= 10000 || \
                _reconTime == 0 )
            {
                _reconTime = millis();
                BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));
            }

            return false;
        }
    #else
        if (WiFi.status() != WL_CONNECTED)
        {
            if ((millis() - _reconTime) >= 10000 || \
                _reconTime == 0 )
            {
                _reconTime = millis();
                BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));
                WiFi.reconnect();
            }

            return false;
        }
    #endif

    return true;
}

template <class Transp>
bool BlinkerProtocol<Transp>::ntpConfig()
{
    configTime((long)(_timezone * 3600), 0, 
                "ntp1.aliyun.com", 
                "120.25.108.11", 
                "time.pool.aliyun.com");

    time_t now_ntp = ::time(nullptr);

    float _com_timezone = abs(_timezone);
    if (_com_timezone < 1.0) _com_timezone = 1.0;

    if (now_ntp < _com_timezone * 3600 * 12)
    {
        configTime((long)(_timezone * 3600), 0, 
                "ntp1.aliyun.com", 
                "120.25.108.11", 
                "time.pool.aliyun.com");
        now_ntp = ::time(nullptr);
        if (now_ntp < _com_timezone * 3600 * 12)
        {
            ::delay(50);
            now_ntp = ::time(nullptr);
            return false;
        }
    }

    struct tm timeinfo;

    #if defined(ESP8266)
        time_t _now_ntp = now_ntp + (long)(_timezone * 3600);
        gmtime_r(&_now_ntp, &timeinfo);
    #elif defined(ESP32)
        localtime_r(&now_ntp, &timeinfo);
    #endif

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("Current time: "), asctime(&timeinfo));
    #if defined(ESP8266)
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("NTP time: "), now_ntp);
    #elif defined(ESP32)
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("NTP time: "), now_ntp);
    #endif

    return true;
}

template <class Transp>
bool BlinkerProtocol<Transp>::ntpInit()
{
    if (!_isNTPInit)
    {
        // if (_isNTPInit)
        // {
        //     time_t now_ntp = ::time(nullptr);
        //     struct tm timeinfo;
        //     #if defined(ESP8266)
        //         gmtime_r(&now_ntp, &timeinfo);
        //     #elif defined(ESP32)
        //         localtime_r(&now_ntp, &timeinfo);
        //     #endif
        // }

        // if ((millis() - _ntpStart) > BLINKER_NTP_TIMEOUT)
        // {
        //     _ntpStart = millis();
        // }
        // else {
        //     return false;
        // }

        if (!ntpConfig()) return false;

        _ntpStart = millis();

        _isNTPInit = true;

        _deviceStartTime = time() - millis()/1000;

    #if defined(BLINKER_WIDGET)
        BApi::loadTiming();
    #endif
        BApi::beginAuto();

    #if defined(BLINKER_PRO_ESP)
        otaInit();
    #endif

        return true;
    }
    else if (_isNTPInit && (millis() - _ntpStart) >= 60 * 60 * 1000)
    {
        ntpConfig();
        _ntpStart += 60 * 60 * 1000;
    }
    return true;
}

template <class Transp> template<typename T>
bool BlinkerProtocol<Transp>::sms(const T& msg, const String & cel)
{
    String _msg = STRING_format(msg);
    
    String  data = BLINKER_F("{\"deviceName\":\"");
            data += conn.deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += conn.authKey();
            data += BLINKER_F("\",\"cel\":\"");
            data += cel;
            data += BLINKER_F("\",\"msg\":\"");
            data += _msg;
            data += BLINKER_F("\"}");

    if (_msg.length() > 20) return false;

    return conn.httpServer(BLINKER_CMD_SMS_NUMBER, data) != "false";
}

template <class Transp> template<typename T>
bool BlinkerProtocol<Transp>::push(const T& msg, const String & users)
{
    String _msg = STRING_format(msg);
    
    String  data = BLINKER_F("{\"deviceName\":\"");
            data += conn.deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += conn.authKey();
            data += BLINKER_F("\",\"msg\":\"");
            data += _msg;
            data += BLINKER_F("\",\"receivers\":\"");
            data += users;
            data += BLINKER_F("\"}");

    return conn.httpServer(BLINKER_CMD_PUSH_NUMBER, data) != "false";
}

template <class Transp> template<typename T>
bool BlinkerProtocol<Transp>::wechat(const T& msg)
{
    String _msg = STRING_format(msg);
    
    String  data = BLINKER_F("{\"deviceName\":\"");
            data += conn.deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += conn.authKey();
            data += BLINKER_F("\",\"msg\":\"");
            data += _msg;
            data += BLINKER_F("\"}");

    return conn.httpServer(BLINKER_CMD_WECHAT_NUMBER, data) != "false";
}

template <class Transp> template<typename T>
bool BlinkerProtocol<Transp>::wechat(const String & title, const String & state, const T& msg, const String & users)
{
    String _msg = STRING_format(msg);

    String  data = BLINKER_F("{\"deviceName\":\"");
            data += conn.deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += conn.authKey();
            data += BLINKER_F("\",\"title\":\"");
            data += title;
            data += BLINKER_F("\",\"state\":\"");
            data += state;
            data += BLINKER_F("\",\"msg\":\"");
            data += _msg;
            data += BLINKER_F("\",\"receivers\":\"");
            data += users;
            data += BLINKER_F("\"}");

    return conn.httpServer(BLINKER_CMD_WECHAT_NUMBER, data) != "false";
}

template <class Transp>
void BlinkerProtocol<Transp>::weather(uint32_t _city)
{
    String data = BLINKER_F("/weather?");

    if (_city != 0)
    {
        data += BLINKER_F("code=");
        data += STRING_format(_city);
        data += BLINKER_F("&");
    }

    data += BLINKER_F("deviceName=");
    data += conn.deviceName();
    data += BLINKER_F("&key=");
    data += conn.token();

    #if defined(BLINKER_WIFI_AT)
        String reqData;

        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_WEATHER_AT;
        reqData += BLINKER_F(":");
        reqData += conn.httpServer(BLINKER_CMD_WEATHER_NUMBER, data);

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    #else
        if (_weatherFunc) _weatherFunc(conn.httpServer(BLINKER_CMD_WEATHER_NUMBER, data));
    #endif
}

template <class Transp>
void BlinkerProtocol<Transp>::weatherForecast(uint32_t _city)
{
    String data = BLINKER_F("/forecast?");

    if (_city != 0)
    {
        data += BLINKER_F("code=");
        data += STRING_format(_city);
        data += BLINKER_F("&");
    }

    data += BLINKER_F("deviceName=");
    data += conn.deviceName();
    data += BLINKER_F("&key=");
    data += conn.token();

    #if defined(BLINKER_WIFI_AT)
        String reqData;

        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_WEATHER_FORECAST_AT;
        reqData += BLINKER_F(":");
        reqData += conn.httpServer(BLINKER_CMD_WEATHER_FORECAST_NUMBER, data);

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    #else
        if (_weather_forecast_Func) _weather_forecast_Func(conn.httpServer(BLINKER_CMD_WEATHER_FORECAST_NUMBER, data));
    #endif
}

template <class Transp>
void BlinkerProtocol<Transp>::air(uint32_t _city)
{
    String data = BLINKER_F("/air?");

    if (_city != 0)
    {
        data += BLINKER_F("code=");
        data += STRING_format(_city);
        data += BLINKER_F("&");
    }

    data += BLINKER_F("deviceName=");
    data += conn.deviceName();
    data += BLINKER_F("&key=");
    data += conn.token();

    #if defined(BLINKER_WIFI_AT)
        String reqData;

        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_AIR_AT;
        reqData += BLINKER_F(":");
        reqData += conn.httpServer(BLINKER_CMD_AQI_NUMBER, data);

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    #else
        if (_airFunc) _airFunc(conn.httpServer(BLINKER_CMD_AQI_NUMBER, data));
    #endif
}

template <class Transp>
bool BlinkerProtocol<Transp>::log(const String & msg)
{
    String data = BLINKER_F("{\"token\":\"");
    data += conn.token();
    data += BLINKER_F("\",\"data\":[[");
    data += STRING_format(time());
    data += BLINKER_F(",\"");
    data += msg;
    data += BLINKER_F("\"]]}");

    return conn.httpServer(BLINKER_CMD_LOG_NUMBER, data) != "false";
}

template <class Transp>
bool BlinkerProtocol<Transp>::dataStorage(const String & msg)
{
    String data = BLINKER_F("{\"deviceName\":\"");
    data += conn.deviceName();
    data += BLINKER_F("\",\"key\":\"");
    data += conn.authKey();
    data += BLINKER_F("\",\"data\":{");
    data += msg;
    data += BLINKER_F("}}");
    
    return conn.httpServer(BLINKER_CMD_DATA_STORAGE_NUMBER, data) != "false";
}

template <class Transp>
bool BlinkerProtocol<Transp>::timeSlot(const String & msg)
{
    String data = BLINKER_F("{\"device\":\"");
    data += conn.deviceName();
    data += BLINKER_F("\",\"key\":\"");
    data += conn.authKey();
    data += BLINKER_F("\",\"data\":[");
    data += msg;
    data += BLINKER_F("]}");
    
    return conn.httpServer(BLINKER_CMD_TIME_SLOT_DATA_NUMBER, data) != "false";
}

template <class Transp>
void BlinkerProtocol<Transp>::textData(const String & msg)
{
    String data = BLINKER_F("{");
    data += BLINKER_F("\"device\":\"");
    data += conn.deviceName();
    data += BLINKER_F("\",\"key\":\"");
    data += conn.authKey();
    data += BLINKER_F("\",\"data\":\"");
    data += msg;
    data += BLINKER_F("\"}");

    conn.httpServer(BLINKER_CMD_TEXT_DATA_NUMBER, data);
}

template <class Transp>
void BlinkerProtocol<Transp>::configUpdate(const String & msg)
{
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, msg);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG(TAG_PROTO, "update data is not Json! ", msg);
        return;
    }

    String data = BLINKER_F("{");
    data += BLINKER_F("\"token\":\"");
    data += conn.token();
    data += BLINKER_F("\",\"data\":");
    data += msg;
    data += BLINKER_F("}");

    conn.httpServer(BLINKER_CMD_CONFIG_UPDATE_NUMBER, data);
}

template <class Transp>
void BlinkerProtocol<Transp>::configGet()
{
    String data = BLINKER_F("/cloud_storage/object?token=");
    data += conn.token();

    conn.httpServer(BLINKER_CMD_CONFIG_GET_NUMBER, data);
}

template <class Transp>
void BlinkerProtocol<Transp>::httpHeartbeat()
{
    String data = BLINKER_F("/heartbeat?");
    data += BLINKER_F("deviceName=");
    data += conn.deviceName();
    data += BLINKER_F("&key=");
    data += conn.authKey();
    data += BLINKER_F("&heartbeat=");
    data += STRING_format(BLINKER_DEVICE_HEARTBEAT_TIME);

    conn.httpServer(BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER, data);
}

#if defined(BLINKER_PRO_ESP)
template <class Transp>
void BlinkerProtocol<Transp>::loadOTA()
{
    // if (_OTA.loadOTACheck())
    // {
    //     if (!_OTA.loadVersion())
    //     {
    //         _OTA.saveVersion();
    //     }
    // }

    ::delay(1000);

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("OTA load: "), BLINKER_OTA_VERSION_CODE);

    uint8_t ota_check = _OTA.loadOTACheck();

    if (ota_check == BLINKER_OTA_RUN)
    {
        _OTA.saveOTACheck();
        updateOTAStatus(10, "download firmware");

        String otaData = checkOTA();

        if (otaData != BLINKER_CMD_FALSE)
        {
            // DynamicJsonBuffer jsonBuffer;
            // JsonObject& otaJson = jsonBuffer.parseObject(otaData);
            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, otaData);
            JsonObject otaJson = jsonBuffer.as<JsonObject>();

            // if (!otaJson.success())
            if (error)
            {
                BLINKER_ERR_LOG_ALL(BLINKER_F("check ota data error"));
                return;
            }

            // String otaHost = otaJson["host"];
            // String otaUrl = otaJson["url"];
            // String otaFp = otaJson["fingerprint"];
            // String otaMD5 = otaJson["hash"];

            // _OTA.config(otaHost, otaUrl, otaFp, otaMD5);
            _OTA.config(otaJson["host"].as<String>(), otaJson["url"].as<String>(), otaJson["fingerprint"].as<String>(), otaJson["hash"].as<String>());

            if (_OTA.update())
            {
                // _OTA.saveVersion();
                // _OTA.clearOTACheck();

                // updateOTAStatus(100);

                conn.freshAlive();
                otaStatus(99, BLINKER_F("Firmware download sucessed"));
                updateOTAStatus(99, BLINKER_F("Firmware download sucessed"));
                ESP.restart();
            }
            else
            {
                _OTA.clearOTACheck();

                conn.freshAlive();
                otaStatus(-2, BLINKER_F("Firmware download failed"));
                updateOTAStatus(-2, BLINKER_F("Firmware download failed"));
            }
        }
    }
    else if (ota_check == BLINKER_OTA_START)
    {
        if (!_OTA.loadVersion())
        {
            _OTA.saveVersion();
            _OTA.clearOTACheck();

            conn.freshAlive();
            otaStatus(100, BLINKER_F("Firmware update success"));
            updateOTAStatus(100, BLINKER_F("Firmware update success"));
            // ota failed
        }
        else
        {
            // _OTA.saveVersion();
            _OTA.clearOTACheck();

            conn.freshAlive();
            otaStatus(-2, BLINKER_F("Firmware download failed"));
            updateOTAStatus(-2, BLINKER_F("Firmware download failed"));
            // ota success
        }
    }
    else if (ota_check == BLINKER_OTA_CLEAR)
    {
        // _OTA.saveVersion();
        // _OTA.clearOTACheck();

        if (!_OTA.loadVersion()) _OTA.saveVersion();
    }
    else
    {
        if (!_OTA.loadVersion())
        {
            if (ota_check > 0)
            {
                // updateOTAStatus(100);
                _OTA.clearOTACheck();
                _OTA.saveVersion();
            }
            else
            {
                _OTA.saveVersion();
            }
        }
        else if (_OTA.loadVersion() && ota_check == 0)
        {
            // updateOTAStatus(-2);
            _OTA.clearOTACheck();
        }
        else
        {
            _OTA.clearOTACheck();
        }
    }

}

template <class Transp>
void BlinkerProtocol<Transp>::ota()
{
    // if (checkCanOTA())
    // {
        _OTA.saveOTARun();

        ::delay(100);
        ESP.restart();

        loadOTA();
    // }
}

template <class Transp>
String BlinkerProtocol<Transp>::checkOTA()
{
    String data = BLINKER_F("/ota/upgrade?deviceName=");// +
    data += conn.deviceName();

    return conn.httpServer(BLINKER_CMD_OTA_NUMBER, data);
}

template <class Transp>
bool BlinkerProtocol<Transp>::updateOTAStatus(int8_t status, const String & msg)
{
    String data;

    data = BLINKER_F("{\"deviceName\":\"");
    data += conn.deviceName();
    data += BLINKER_F("\",\"key\":\"");
    data += conn.authKey();
    data += BLINKER_F("\",\"upgrade\":true,\"upgradeData\":{\"step\":\"");
    data += STRING_format(status);
    data += BLINKER_F("\",\"desc\":\"");
    data += msg;
    data += BLINKER_F("\"}}");

    return conn.httpServer(BLINKER_CMD_OTA_STATUS_NUMBER, data) != "false";
}

template <class Transp>
void BlinkerProtocol<Transp>::otaStatus(int8_t status, const String & msg)
{
    String data;

    data = BLINKER_F("{\"step\":\"");
    data += STRING_format(status);
    data += BLINKER_F("\",\"desc\":\"");
    data += msg;
    data += BLINKER_F("\"}");

    print(BLINKER_CMD_VERSION, BLINKER_OTA_VERSION_CODE);
    print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
    printObject(BLINKER_F("upgradeData"), data);
    printNow();
}

template <class Transp>
void BlinkerProtocol<Transp>::otaInit()
{
    uint8_t ota_check = _OTA.loadOTACheck();

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("otaInit: "), ota_check);

    if (ota_check == BLINKER_OTA_RUN)
    {
        _OTA.saveOTACheck();
        updateOTAStatus(10, "download firmware");

        String otaData = checkOTA();

        if (otaData != BLINKER_CMD_FALSE)
        {
            // DynamicJsonBuffer jsonBuffer;
            // JsonObject& otaJson = jsonBuffer.parseObject(otaData);
            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, otaData);
            JsonObject otaJson = jsonBuffer.as<JsonObject>();

            // if (!otaJson.success())
            if (error)
            {
                BLINKER_ERR_LOG_ALL(TAG_PROTO, BLINKER_F("check ota data error"));
                return;
            }

            // String otaHost = otaJson["host"];
            // String otaUrl = otaJson["url"];
            // String otaFp = otaJson["fingerprint"];
            // String otaMD5 = otaJson["hash"];

            // _OTA.config(otaHost, otaUrl, otaFp, otaMD5);
            _OTA.config(otaJson["host"].as<String>(), otaJson["url"].as<String>(), otaJson["fingerprint"].as<String>(), otaJson["hash"].as<String>());

            if (_OTA.update())
            {
                // _OTA.saveVersion();
                // _OTA.clearOTACheck();

                // updateOTAStatus(100);

                conn.freshAlive();
                otaStatus(99, BLINKER_F("Firmware download sucessed"));
                updateOTAStatus(99, BLINKER_F("Firmware download sucessed"));
                ESP.restart();
            }
            else
            {
                _OTA.clearOTACheck();

                conn.freshAlive();
                otaStatus(-2, BLINKER_F("Firmware download failed"));
                updateOTAStatus(-2, BLINKER_F("Firmware download failed"));
            }
        }
    }
    else if (ota_check == BLINKER_OTA_START)
    {
        if (!_OTA.loadVersion())
        {
            _OTA.saveVersion();
            _OTA.clearOTACheck();

            conn.freshAlive();
            otaStatus(100, BLINKER_F("Firmware update success"));
            updateOTAStatus(100, BLINKER_F("Firmware update success"));
            // ota failed
        }
        else
        {
            // _OTA.saveVersion();
            _OTA.clearOTACheck();

            conn.freshAlive();
            otaStatus(-2, BLINKER_F("Firmware download failed"));
            updateOTAStatus(-2, BLINKER_F("Firmware download failed"));
            // ota success
        }
    }
    else if (ota_check == BLINKER_OTA_CLEAR)
    {
        // _OTA.saveVersion();
        // _OTA.clearOTACheck();

        if (!_OTA.loadVersion()) _OTA.saveVersion();
    }
    else
    {
        if (!_OTA.loadVersion())
        {
            if (ota_check > 0)
            {
                // updateOTAStatus(100);
                _OTA.clearOTACheck();
                _OTA.saveVersion();
            }
            else
            {
                _OTA.saveVersion();
            }
        }
        else if (_OTA.loadVersion() && ota_check == 0)
        {
            // updateOTAStatus(-2);
            _OTA.clearOTACheck();
        }
        else
        {
            _OTA.clearOTACheck();
        }
    }
}
#endif

template <class Transp> template <typename T>
void BlinkerProtocol<Transp>::dataStorage(char _name[], const T& msg)
{
    String _msg = STRING_format(msg);

    int8_t num = checkNum(_name, _Data, data_dataCount);

    time_t _time = time();
    uint8_t _second = second();
    time_t now_time = _time - _second;

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("time: "), _time, BLINKER_F(",second: "), _second);

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("now_time: "), now_time);

    now_time = now_time - now_time % 10;

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("dataStorage num: "), num, BLINKER_F(" ,"), now_time);
    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("dataStorage count: "), data_dataCount);

    String data_msg = String(msg);

    if (data_msg.length() > 10) return;

    if( num == BLINKER_OBJECT_NOT_AVAIL )
    {
        if (data_dataCount == BLINKER_MAX_BLINKER_DATA_SIZE)
        {
            return;
        }
        _Data[data_dataCount] = new BlinkerData();
        _Data[data_dataCount]->name(_name);
        // _Data[data_dataCount]->saveData(time(), _msg);
        // if
        _Data[data_dataCount]->saveData(data_msg, now_time, BLINKER_DATA_FREQ_TIME);
        data_dataCount++;
        // {
        //     dataUpdate();
        // }

        BLINKER_LOG_ALL(TAG_PROTO, _name, BLINKER_F(" save: "), _msg, BLINKER_F(" time: "), now_time);
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("data_dataCount: "), data_dataCount);
    }
    else {
        // _Data[num]->saveData(time(), _msg);
        // if
        _Data[num]->saveData(data_msg, now_time, BLINKER_DATA_FREQ_TIME);
        // {
        //     dataUpdate();
        // }

        BLINKER_LOG_ALL(TAG_PROTO, _name, BLINKER_F(" save: "), _msg, BLINKER_F(" time: "), now_time);
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("data_dataCount: "), data_dataCount);
    }


}

template <class Transp>
void BlinkerProtocol<Transp>::attachDataStorage(blinker_callback_t newFunction, uint32_t _time, uint8_t d_times)
{
    _dataStorageFunc = newFunction;
    if (_time < 5) _time = 5;
    _autoStorageTime = _time;
    _autoDataTime = millis();
    if (d_times > BLINKER_MAX_DATA_COUNT || d_times == 0) d_times = BLINKER_DATA_UPDATE_COUNT;
    _dataTimes = d_times;
}

template <class Transp>
void BlinkerProtocol<Transp>::checkDataStorage()
{
    if (_dataStorageFunc)
    {
        if (millis() - _autoDataTime >= _autoStorageTime * 1000)
        {
            _autoDataTime += _autoStorageTime * 1000;
            _dataStorageFunc();
        }
    }

    if (millis() - _autoUpdateTime >= _autoStorageTime * _dataTimes * 1000)
    {
        if ((data_dataCount || data_timeSlotDataCount) && conn.checkInit() )// && ESP.getFreeHeap() > 4000)
        {
            if (dataUpdate()) _autoUpdateTime += _autoStorageTime * _dataTimes * 1000;
            else _autoUpdateTime = millis() - 100000;
        }
    }
}

template <class Transp>
bool BlinkerProtocol<Transp>::dataUpdate()
{
    String data = "";

    if (data_timeSlotDataCount > 0)
    {
        for (uint8_t _num = 0; _num < data_timeSlotDataCount; _num++) {
            data += _TimeSlotData[_num]->getData();
            if (_num < data_timeSlotDataCount - 1) {
                data += BLINKER_F(",");
            }
        }

        if (timeSlot(data))
        {
            for (uint8_t _num = 0; _num < data_timeSlotDataCount; _num++)
            {
                // _TimeSlotData[_num]->flush();
                free(_TimeSlotData[_num]);
            }

            data_timeSlotDataCount = 0;
            return true;
        }
        return false;
    }

    if (!data_dataCount) return false;

    for (uint8_t _num = 0; _num < data_dataCount; _num++) {
        data += BLINKER_F("\"");
        data += _Data[_num]->getName();
        data += BLINKER_F("\":");
        data += _Data[_num]->getData();
        if (_num < data_dataCount - 1) {
            data += BLINKER_F(",");
        }

        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("num: "), _num, \
                BLINKER_F(" name: "), _Data[_num]->getName());

        BLINKER_LOG_FreeHeap_ALL();
    }

    if (dataStorage(data))
    {
        for (uint8_t _num = 0; _num < data_dataCount; _num++)
        {
            _Data[_num]->flush();
        }

        return true;
    }

    return false;
}

template <class Transp> template<typename T>
void BlinkerProtocol<Transp>::timeSlotData(char _name[], const T& _data)
{
    uint32_t now_time = time();

    if (data_timeSlotDataCount == BLINKER_MAX_BLINKER_DATA_SIZE)
    {
        BLINKER_ERR_LOG(TAG_PROTO, BLINKER_F("BLINKER MAX DATA STORAGE LIMIT!"));
        return;
    }

    BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("timeSlotData save"));

    if (millis() - time_timeSlotData < 1000)
    {
        if (data_timeSlotDataCount != 0)
        {
            BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("data_timeSlotDataCount != 0"));
            _TimeSlotData[data_timeSlotDataCount - 1]->saveData(_name, _data, now_time);
        }
        else
        {
            BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("data_timeSlotDataCount == 0"));
            _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
            _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
            data_timeSlotDataCount++;
        }

        BLINKER_LOG_ALL(TAG_PROTO, _name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
    }
    else
    {
        time_timeSlotData = millis();

        _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
        _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
        data_timeSlotDataCount++;

        BLINKER_LOG_ALL(TAG_PROTO, _name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
        BLINKER_LOG_ALL(TAG_PROTO, BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
    }
}

#endif

#if defined(BLINKER_WIFI_AT)
template <class Transp>
void BlinkerProtocol<Transp>::atBegin()
{
    while (_at_status == BL_BEGIN)
    {
        atSerialTask();
    }
}

template <class Transp>
bool BlinkerProtocol<Transp>::atSerialTask()
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
void BlinkerProtocol<Transp>::parseATdata()
{
    String reqData;

    if (_slaverAT->cmd() == BLINKER_CMD_AT) {
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_RST) {
        serialPrint(BLINKER_CMD_OK);
        ::delay(100);
        ESP.restart();
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_GMR) {
        // reqData = "+" + STRING_format(BLINKER_CMD_GMR) +
        //         "=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>" +
        //         "[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]";
        serialPrint(BLINKER_ESP_AT_VERSION);
        serialPrint(BLINKER_VERSION);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_UART_CUR) {
        blinker_at_state_t at_state = _slaverAT->state();

        BLINKER_LOG(at_state);

        switch (at_state)
        {
            case AT_NONE:
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;// +
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_UART_CUR;// +
                reqData += BLINKER_F("=<baudrate>,<databits>,<stopbits>,<parity>");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
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
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
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

                serialPrint(BLINKER_CMD_OK);

                // if (isHWS) {
                    Serial.begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
                // }
                // else {
                //     SSerialBLE->begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
                // }
                break;
            case AT_ACTION:
                // BProto::serialPrint();
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
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_UART_DEF;
                reqData += BLINKER_F("=<baudrate>,<databits>,<stopbits>,<parity>");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
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
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
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

                serialPrint(BLINKER_CMD_OK);

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
                // BProto::serialPrint();
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

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_ADC && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_ADC;
        reqData += BLINKER_F(":");
        reqData += STRING_format(analogRead(A0));

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
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

        serialPrint(BLINKER_CMD_OK);
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
                serialPrint(reqData);
            }
        }
        if (!_isGet) {
            reqData = BLINKER_F("+");
            reqData += BLINKER_CMD_IOGETCFG;
            reqData += BLINKER_F(":");
            reqData += _slaverAT->getParam(PIN_SET);
            reqData += BLINKER_F(",2,0");
            serialPrint(reqData);
        }

        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_GPIOWRITE && _slaverAT->state() == AT_SETTING) {
        BLINKER_LOG_ALL(BLINKER_F("IO_PIN: "), _slaverAT->getParam(IO_PIN));
        BLINKER_LOG_ALL(BLINKER_F("IO_LVL: "),  _slaverAT->getParam(IO_LVL));

        if (BLINKER_GPIOWRITE_PARAM_NUM != _slaverAT->paramNum()) return;

        uint8_t set_pin = (_slaverAT->getParam(IO_PIN)).toInt();
        uint8_t set_lvl = (_slaverAT->getParam(IO_LVL)).toInt();

        if (set_pin >= BLINKER_MAX_PIN_NUM) return;

        BLINKER_LOG_ALL(set_pin, ", ", set_lvl, ", ", pinDataNum);

        // bool _isSet = false;
        for (uint8_t _num = 0; _num < pinDataNum; _num++)
        {
            if (_pinData[_num]->checkPin(set_pin))
            {
                if (_pinData[_num]->getMode() == BLINKER_IO_OUTPUT_NUM)
                {
                    if (set_lvl <= 1) {
                        digitalWrite(set_pin, set_lvl ? HIGH : LOW);
                        serialPrint(BLINKER_CMD_OK);
                        return;
                    }
                }
            }
        }

        serialPrint(BLINKER_CMD_ERROR);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_GPIOWREAD && _slaverAT->state() == AT_SETTING) {
        BLINKER_LOG_ALL(BLINKER_F("IO_PIN: "), _slaverAT->getParam(IO_PIN));

        if (BLINKER_GPIOREAD_PARAM_NUM != _slaverAT->paramNum()) return;

        uint8_t set_pin = (_slaverAT->getParam(IO_PIN)).toInt();

        if (set_pin >= BLINKER_MAX_PIN_NUM)
        {
            serialPrint(BLINKER_CMD_ERROR);
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
                        serialPrint(reqData);
                        serialPrint(BLINKER_CMD_OK);
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
        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
        // BProto::serialPrint(BLINKER_CMD_ERROR);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_MQTT) {
        // // BProto::serialPrint(BLINKER_CMD_OK);

        BLINKER_LOG(BLINKER_CMD_BLINKER_MQTT);

        blinker_at_state_t at_state = _slaverAT->state();

        BLINKER_LOG(at_state);

        switch (at_state)
        {
            case AT_NONE:
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_MQTT;
                reqData += BLINKER_F("=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>");
                reqData += BLINKER_F("[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
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
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
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

                    if (_at_status == BL_INITED)
                    {
                        reqData = BLINKER_F("+");
                        reqData += BLINKER_CMD_BLINKER_MQTT;
                        reqData += BLINKER_F(":");
                        reqData += conn.deviceId();
                        reqData += BLINKER_F(",");
                        reqData += conn.uuid();
                        serialPrint(reqData);
                        serialPrint(BLINKER_CMD_OK);
                        return;
                    }

                    conn.connectWiFi((_slaverAT->getParam(MQTT_WIFI_SSID)).c_str(),
                                (_slaverAT->getParam(MQTT_WIFI_PSWD)).c_str());

                    conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                    _at_status = BL_INITED;
                    _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
                }
                else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_SMARTCONFIG_NUM)
                {
                    BLINKER_LOG_ALL(BLINKER_F("BLINKER_CMD_SMARTCONFIG"));

                    if (BLINKER_SMCFG_PARAM_NUM != _slaverAT->paramNum()) return;

                    if (_at_status == BL_INITED)
                    {
                        reqData = BLINKER_F("+");
                        reqData += BLINKER_CMD_BLINKER_MQTT;
                        reqData += BLINKER_F(":");
                        reqData += conn.deviceId();
                        reqData += BLINKER_F(",");
                        reqData += conn.uuid();
                        serialPrint(reqData);
                        serialPrint(BLINKER_CMD_OK);
                        return;
                    }

                    // if (!BProto::autoInit())
                    conn.smartconfig();

                    conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                    _at_status = BL_INITED;
                    _wlanMode = BLINKER_CMD_SMARTCONFIG_NUM;
                }
                else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_APCONFIG_NUM)
                {
                    BLINKER_LOG(BLINKER_F("BLINKER_CMD_APCONFIG"));

                    if (BLINKER_APCFG_PARAM_NUM != _slaverAT->paramNum()) return;

                    if (_at_status == BL_INITED)
                    {
                        reqData = BLINKER_F("+");
                        reqData += BLINKER_CMD_BLINKER_MQTT;
                        reqData += BLINKER_F(":");
                        reqData += conn.deviceId();
                        reqData += BLINKER_F(",");
                        reqData += conn.uuid();
                        serialPrint(reqData);
                        serialPrint(BLINKER_CMD_OK);
                        return;
                    }

                    if (!conn.autoInit())
                    {
                        conn.softAPinit();
                        // while(WiFi.status() != WL_CONNECTED) {
                        //     BProto::serverClient();

                        //     ::delay(10);
                        // }
                    }

                    conn.begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
                    _at_status = BL_INITED;
                    _wlanMode = BLINKER_CMD_APCONFIG_NUM;
                }
                else {
                    return;
                }

                // reqData = BLINKER_F("+");
                // reqData += BLINKER_CMD_BLINKER_MQTT;
                // reqData += BLINKER_F(":");
                // reqData += BProto::deviceId();
                // reqData += BLINKER_F(",");
                // reqData += BProto::uuid();
                // BProto::serialPrint(reqData);
                // BProto::serialPrint(BLINKER_CMD_OK);
                break;
            case AT_ACTION:
                // BProto::serialPrint();
                break;
            default :
                break;
        }
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_ALIGENIE) {
        // BProto::serialPrint(BLINKER_CMD_OK);

        BLINKER_LOG(BLINKER_CMD_BLINKER_ALIGENIE);

        blinker_at_state_t at_state = _slaverAT->state();

        BLINKER_LOG_ALL(at_state);

        switch (at_state)
        {
            case AT_NONE:
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_ALIGENIE;
                reqData += BLINKER_F("=<type>");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_QUERY:
                reqData = BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_ALIGENIE;
                reqData += BLINKER_F(":");
                reqData += STRING_format(conn.getAliType());
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_SETTING:
                BLINKER_LOG_ALL(BLINKER_F("BLINKER_ALIGENIE_CFG_NUM: "), _slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM));

                if (BLINKER_ALIGENIE_PARAM_NUM != _slaverAT->paramNum()) return;

                if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_LIGHT)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_LIGHT"));
                    conn.setAliType(ALI_LIGHT);
                }
                else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_OUTLET)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_OUTLET"));
                    conn.setAliType(ALI_OUTLET);
                }
                else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_MULTI_OUTLET)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_MULTI_OUTLET"));
                    conn.setAliType(ALI_MULTI_OUTLET);
                }
                else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_SENSOR)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_SENSOR"));
                    conn.setAliType(ALI_SENSOR);
                }
                else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_FAN)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_FAN"));
                    conn.setAliType(ALI_FAN);
                }
                else if ((_slaverAT->getParam(BLINKER_ALIGENIE_CFG_NUM)).toInt() == ALI_AIRCONDITION)
                {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_AIRCONDITION"));
                    conn.setAliType(ALI_AIRCONDITION);
                }
                else {
                    BLINKER_LOG_ALL(BLINKER_F("ALI_NONE"));
                    conn.setAliType(ALI_NONE);
                }
                // conn.setAliType(_aliType);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_ACTION:
                // BProto::serialPrint();
                break;
            default :
                break;
        }
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_DUEROS) {
        // BProto::serialPrint(BLINKER_CMD_OK);

        BLINKER_LOG(BLINKER_CMD_BLINKER_DUEROS);

        blinker_at_state_t at_state = _slaverAT->state();

        BLINKER_LOG_ALL(at_state);

        switch (at_state)
        {
            case AT_NONE:
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_DUEROS;
                reqData += BLINKER_F("=<type>");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_QUERY:
                reqData = BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_DUEROS;
                reqData += BLINKER_F(":");
                reqData += STRING_format(conn.getDuerType());
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_SETTING:
                BLINKER_LOG_ALL(BLINKER_F("BLINKER_DUEROS_CFG_NUM: "), _slaverAT->getParam(BLINKER_DUEROS_CFG_NUM));

                if (BLINKER_DUEROS_PARAM_NUM != _slaverAT->paramNum()) return;

                if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == DUER_LIGHT)
                {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_LIGHT"));
                    conn.setDuerType(DUER_LIGHT);
                }
                else if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == DUER_OUTLET)
                {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_OUTLET"));
                    conn.setDuerType(DUER_OUTLET);
                }
                else if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == DUER_MULTI_OUTLET)
                {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_MULTI_OUTLET"));
                    conn.setDuerType(DUER_MULTI_OUTLET);
                }
                else if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == DUER_SENSOR)
                {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_SENSOR"));
                    conn.setDuerType(DUER_SENSOR);
                }
                else if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == DUER_FAN)
                {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_FAN"));
                    conn.setDuerType(DUER_FAN);
                }
                else if ((_slaverAT->getParam(BLINKER_DUEROS_CFG_NUM)).toInt() == DUER_AIRCONDITION)
                {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_AIRCONDITION"));
                    conn.setDuerType(DUER_AIRCONDITION);
                }
                else {
                    BLINKER_LOG_ALL(BLINKER_F("DUER_NONE"));
                    conn.setDuerType(DUER_NONE);
                }
                // conn.duerType(conn._duerType);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_ACTION:
                // BProto::serialPrint();
                break;
            default :
                break;
        }
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_MIOT) {
        // BProto::serialPrint(BLINKER_CMD_OK);

        BLINKER_LOG(BLINKER_CMD_BLINKER_MIOT);

        blinker_at_state_t at_state = _slaverAT->state();

        BLINKER_LOG_ALL(at_state);

        switch (at_state)
        {
            case AT_NONE:
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_MIOT;
                reqData += BLINKER_F("=<type>");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_QUERY:
                reqData = BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_MIOT;
                reqData += BLINKER_F(":");
                reqData += STRING_format(conn.getMiType());
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_SETTING:
                BLINKER_LOG_ALL(BLINKER_F("BLINKER_MIOT_CFG_NUM: "), _slaverAT->getParam(BLINKER_MIOT_CFG_NUM));

                if (BLINKER_DUEROS_PARAM_NUM != _slaverAT->paramNum()) return;

                if ((_slaverAT->getParam(BLINKER_MIOT_CFG_NUM)).toInt() == MIOT_LIGHT)
                {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_LIGHT"));
                    conn.setMiType(MIOT_LIGHT);
                }
                else if ((_slaverAT->getParam(BLINKER_MIOT_CFG_NUM)).toInt() == MIOT_OUTLET)
                {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_OUTLET"));
                    conn.setMiType(MIOT_OUTLET);
                }
                else if ((_slaverAT->getParam(BLINKER_MIOT_CFG_NUM)).toInt() == MIOT_MULTI_OUTLET)
                {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_MULTI_OUTLET"));
                    conn.setMiType(MIOT_MULTI_OUTLET);
                }
                else if ((_slaverAT->getParam(BLINKER_MIOT_CFG_NUM)).toInt() == MIOT_SENSOR)
                {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_SENSOR"));
                    conn.setMiType(MIOT_SENSOR);
                }
                else if ((_slaverAT->getParam(BLINKER_MIOT_CFG_NUM)).toInt() == MIOT_FAN)
                {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_FAN"));
                    conn.setMiType(MIOT_FAN);
                }
                else if ((_slaverAT->getParam(BLINKER_MIOT_CFG_NUM)).toInt() == MIOT_AIRCONDITION)
                {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_AIRCONDITION"));
                    conn.setMiType(MIOT_AIRCONDITION);
                }
                else {
                    BLINKER_LOG_ALL(BLINKER_F("MIOT_NONE"));
                    conn.setMiType(MIOT_NONE);
                }
                // conn.duerType(_miType);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_ACTION:
                // BProto::serialPrint();
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
                // BProto::serialPrint();
                break;
            case AT_TEST:
                reqData = BLINKER_CMD_AT;
                reqData += BLINKER_F("+");
                reqData += BLINKER_CMD_TIMEZONE;
                reqData += BLINKER_F("=<TIMEZONE>");
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_QUERY:
                reqData = BLINKER_F("+");
                reqData += BLINKER_CMD_BLINKER_MQTT;
                reqData += BLINKER_F(":");
                reqData += STRING_format(getTimezone());
                serialPrint(reqData);
                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_SETTING:
                BLINKER_LOG_ALL(BLINKER_F("BLINKER_TIMEZONE_CFG_NUM: "), _slaverAT->getParam(BLINKER_TIMEZONE_CFG_NUM));

                if (BLINKER_TIMEZONE_PARAM_NUM != _slaverAT->paramNum()) return;

                setTimezone((_slaverAT->getParam(BLINKER_TIMEZONE_CFG_NUM)).toFloat());

                serialPrint(BLINKER_CMD_OK);
                break;
            case AT_ACTION:
                // BProto::serialPrint();
                break;
            default :
                break;
        }
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_TIME_AT && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_TIME_AT;
        reqData += BLINKER_F(":");
        reqData += STRING_format(time());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_SECOND && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_SECOND;
        reqData += BLINKER_F(":");
        reqData += STRING_format(second());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_SECOND && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_SECOND;
        reqData += BLINKER_F(":");
        reqData += STRING_format(second());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_MINUTE && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_MINUTE;
        reqData += BLINKER_F(":");
        reqData += STRING_format(minute());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_HOUR && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_HOUR;
        reqData += BLINKER_F(":");
        reqData += STRING_format(hour());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_WDAY && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_WDAY;
        reqData += BLINKER_F(":");
        reqData += STRING_format(wday());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_MDAY && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_MDAY;
        reqData += BLINKER_F(":");
        reqData += STRING_format(mday());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_YDAY && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_YDAY;
        reqData += BLINKER_F(":");
        reqData += STRING_format(yday());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_MONTH && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_MONTH;
        reqData += BLINKER_F(":");
        reqData += STRING_format(month());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_YEAR && _slaverAT->state() == AT_QUERY) {
        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_YEAR;
        reqData += BLINKER_F(":");
        reqData += STRING_format(year());

        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_WEATHER_AT && _slaverAT->state() == AT_SETTING) {
        if (1 != _slaverAT->paramNum()) return;

        // reqData = BLINKER_F("+");
        // reqData += BLINKER_CMD_WEATHER_AT;
        // reqData += BLINKER_F(":");
        // reqData += STRING_format(weather(_slaverAT->getParam(0).toInt()));

        // serialPrint(reqData);
        // serialPrint(BLINKER_CMD_OK);

        weather(_slaverAT->getParam(0).toInt());
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_WEATHER_FORECAST_AT && _slaverAT->state() == AT_SETTING) {
        if (1 != _slaverAT->paramNum()) return;

        // reqData = BLINKER_F("+");
        // reqData += BLINKER_CMD_WEATHER_FORECAST_AT;
        // reqData += BLINKER_F(":");
        // reqData += STRING_format(weatherForecast(_slaverAT->getParam(0).toInt()));

        // serialPrint(reqData);
        // serialPrint(BLINKER_CMD_OK);

        weatherForecast(_slaverAT->getParam(0).toInt());
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_AIR_AT && _slaverAT->state() == AT_SETTING) {
        if (1 != _slaverAT->paramNum()) return;

        // reqData = BLINKER_F("+");
        // reqData += BLINKER_CMD_AIR_AT;
        // reqData += BLINKER_F(":");
        // reqData += STRING_format(air(_slaverAT->getParam(0).toInt()));

        // serialPrint(reqData);
        // serialPrint(BLINKER_CMD_OK);

        air(_slaverAT->getParam(0).toInt());
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_NOTICE_AT && _slaverAT->state() == AT_SETTING) {
        if (1 != _slaverAT->paramNum()) return;

        // reqData = "+" + STRING_format(BLINKER_CMD_NOTICE_AT) +
        //         ":" + STRING_format(aqi(_slaverAT->getParam(0)));
        notify(_slaverAT->getParam(0));
        // BProto::serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
    else if (_slaverAT->cmd() == BLINKER_CMD_SMS_AT && _slaverAT->state() == AT_SETTING) {
        if (1 != _slaverAT->paramNum()) return;

        // reqData = "+" + STRING_format(BLINKER_CMD_NOTICE_AT) +
        //         ":" + STRING_format(aqi(_slaverAT->getParam(0)));
        sms(_slaverAT->getParam(0));
        // BProto::serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }
}
#endif

#endif
