#ifndef BLINKER_API_H
#define BLINKER_API_H

#include "Blinker/BlinkerApiBase.h"
#include "Blinker/BlinkerProtocol.h"

typedef BlinkerProtocol BProto;

enum b_joystickaxis_t {
    BLINKER_J_Xaxis,
    BLINKER_J_Yaxis
};

enum b_ahrsattitude_t {
    Yaw,
    Pitch,
    Roll
};

enum b_gps_t {
    LONG,
    LAT
};

enum b_rgb_t {
    BLINKER_R,
    BLINKER_G,
    BLINKER_B,
    BLINKER_BRIGHT
};

class BlinkerApi : public BlinkerProtocol
{
    public :
        void run();

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
        void vibrate(uint16_t ms = 200);
        void delay(unsigned long ms);
        void attachAhrs();
        void detachAhrs();
        int16_t ahrs(b_ahrsattitude_t attitude) { return ahrsValue[attitude]; }
        float gps(b_gps_t axis);
        
        void attachData(blinker_callback_with_string_arg_t newFunction)
        { _availableFunc = newFunction; }
        void attachHeartbeat(blinker_callback_t newFunction)
        { _heartbeatFunc = newFunction; }
        void attachSummary(blinker_callback_return_string_t newFunction)
        { _summaryFunc = newFunction; }

        void freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        #if defined(BLINKER_BLE)
            void freshAttachWidget(char _name[], blinker_callback_with_joy_arg_t _func);
        #endif
        void freshAttachWidget(char _name[], blinker_callback_with_rgb_arg_t _func);
        void freshAttachWidget(char _name[], blinker_callback_with_int32_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        #if defined(BLINKER_BLE)
            uint8_t attachWidget(char _name[], blinker_callback_with_joy_arg_t _func);
        #endif
        uint8_t attachWidget(char _name[], blinker_callback_with_rgb_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_int32_arg_t _func);
        void attachSwitch(blinker_callback_with_string_arg_t _func);
        char * widgetName_str(uint8_t num);
        #if defined(BLINKER_BLE)
            char * widgetName_joy(uint8_t num);
        #endif
        char * widgetName_rgb(uint8_t num);
        char * widgetName_int(uint8_t num);

    private :
        bool        _fresh = false;
        int16_t     ahrsValue[3];
        float       gpsValue[2];
        uint32_t    gps_get_time;
        
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_joy = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_int = 0;

        class BlinkerWidgets_string *       _Widgets_str[BLINKER_MAX_WIDGET_SIZE*2];
        #if defined(BLINKER_BLE)
            class BlinkerWidgets_joy *          _Widgets_joy[BLINKER_MAX_WIDGET_SIZE/2];
        #endif
        class BlinkerWidgets_rgb *          _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_int32 *        _Widgets_int[BLINKER_MAX_WIDGET_SIZE*2];
        // class BlinkerWidgets_string *       _BUILTIN_SWITCH;
        BlinkerWidgets_string _BUILTIN_SWITCH = BlinkerWidgets_string(BLINKER_CMD_BUILTIN_SWITCH);

        blinker_callback_t                  _heartbeatFunc = NULL;
        blinker_callback_return_string_t    _summaryFunc = NULL;
        
        void parse(char _data[], bool ex_data = false);

        #if defined(BLINKER_ARDUINOJSON)
            int16_t ahrs(b_ahrsattitude_t attitude, const JsonObject& data);
            float gps(b_gps_t axis, const JsonObject& data);

            void heartBeat(const JsonObject& data);
            void getVersion(const JsonObject& data);
            void setSwitch(const JsonObject& data);

            #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
                void bridgeParse(char _bName[], const JsonObject& data);
            #endif
            void strWidgetsParse(char _wName[], const JsonObject& data);
            #if defined(BLINKER_BLE)
                void joyWidgetsParse(char _wName[], const JsonObject& data);
            #endif
            void rgbWidgetsParse(char _wName[], const JsonObject& data);
            void intWidgetsParse(char _wName[], const JsonObject& data);

            void json_parse(const JsonObject& data);
        #else
            int16_t ahrs(b_ahrsattitude_t attitude, char data[]);
            float gps(b_gps_t axis, char data[]);

            void heartBeat(char data[]);
            void getVersion(char data[]);
            void setSwitch(char data[]);

            void strWidgetsParse(char _wName[], char _data[]);
            #if defined(BLINKER_BLE)
                void joyWidgetsParse(char _wName[], char _data[]);
            #endif
            void rgbWidgetsParse(char _wName[], char _data[]);
            void intWidgetsParse(char _wName[], char _data[]);

            void json_parse(char _data[]);
        #endif

    protected :
        void begin();
};

void BlinkerApi::begin()
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
}

void BlinkerApi::run()
{
    // bool conState = BProto::connected();

    switch (BProto::state)
    {
        case CONNECTING :
            if (BProto::connect())
            {
                BProto::state = CONNECTED;
            }
            break;
        case CONNECTED :
            if (BProto::connected())
            {
                BProto::checkAvail();
                if (BProto::isAvail)
                {
                    parse(BProto::dataParse());
                }

                if (BProto::availState)
                {
                    BProto::availState = false;

                    if (BProto::_availableFunc)
                    {
                        BProto::_availableFunc(BProto::lastRead());
                        flush();
                    }
                }
            }
            else
            {
                BProto::disconnect();
                BProto::state = CONNECTING;
            }
            break;
        case DISCONNECTED :
            BProto::disconnect();
            BProto::state = CONNECTING;
            break;
    }

    BProto::checkAutoFormat();
}

void BlinkerApi::parse(char _data[], bool ex_data)
{
    BLINKER_LOG_ALL(BLINKER_F("parse data: "), _data);
    
    if (!ex_data)
    {
        if (BProto::parseState())
        {
            _fresh = false;

            #if defined(BLINKER_ARDUINOJSON)
                BLINKER_LOG_ALL(BLINKER_F("defined BLINKER_ARDUINOJSON"));

                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(STRING_format(_data));

                if (!root.success()) 
                {
                    #if defined(BLINKER_MQTT_AT)
                        atResp();
                    #endif
                    return;
                }

                #if defined(BLINKER_PRO)
                    checkRegister(root);
                #endif

                // #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)

                #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || \
                    defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
                    timerManager(root);
                    BLINKER_LOG_ALL(BLINKER_F("timerManager"));
                #endif

                #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
                    autoManager(root);
                    otaPrase(root);

                    for (uint8_t bNum = 0; bNum < _bridgeCount; bNum++)
                    {
                        bridgeParse(_Bridge[bNum]->getName(), root);
                    }
                #endif

                heartBeat(root);
                getVersion(root);

                json_parse(root);

                ahrs(Yaw, root);
                gps(LONG, root);
            #else
                BLINKER_LOG_ALL(BLINKER_F("ndef BLINKER_ARDUINOJSON"));

                heartBeat(_data);
                getVersion(_data);

                json_parse(_data);

                ahrs(Yaw, _data);
                gps(LONG, _data);
            #endif

            if (_fresh)
            {
                BProto::isParsed();
            }
            else
            {
                #if defined(BLINKER_PRO)
                    if (_parseFunc) {
                        if(_parseFunc(root)) {
                            _fresh = true;
                            BProto::isParsed();
                        }
                        
                        BLINKER_LOG_ALL(BLINKER_F("run parse callback function"));
                    }
                #endif
            }
        }        
    }
    else
    {
        #if defined(BLINKER_ARDUINOJSON)
            String arrayData = BLINKER_F("{\"data\":");
            arrayData += _data;
            arrayData += BLINKER_F("}");
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(arrayData);

            if (!root.success()) return;

            arrayData = root["data"][0].as<String>();

            if (arrayData.length())
            {
                for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
                {
                    arrayData = root["data"][a_num].as<String>();

                    if(arrayData.length()) {
                        DynamicJsonBuffer _jsonBuffer;
                        JsonObject& _array = _jsonBuffer.parseObject(arrayData);

                        json_parse(_array);
                        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || \
                            defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
                            timerManager(_array, true);
                        #endif
                    }
                    else {
                        return;
                    }
                }
            }
            else {
                JsonObject& root = jsonBuffer.parseObject(_data);

                if (!root.success()) return;

                json_parse(root);
            }
        #else
            json_parse(_data);
        #endif
    }
}

void BlinkerApi::print()
{
    String _msg = BLINKER_F("");

    // checkFormat();
    // strcpy(_sendBuf, _msg.c_str());
    // _print(_sendBuf);
    // free(_sendBuf);
    // autoFormat = false;
    BProto::print(_msg);
}

template <typename T>
void BlinkerApi::print(T n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n);
    _msg += BLINKER_F("\"");

    // checkFormat();
    // strcpy(_sendBuf, _msg.c_str());
    // _print(_sendBuf);
    // free(_sendBuf);
    // autoFormat = false;
    BProto::print(_msg);
}

template <typename T1, typename T2, typename T3>
void BlinkerApi::print(T1 n1, T2 n2, T3 n3)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(n2);
    _msg += BLINKER_CMD_INTERSPACE;
    _msg += STRING_format(n3);
    _msg += BLINKER_F("\"");

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::printArray(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += s2;
    
    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::printObject(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += s2;
    
    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, const String &s2)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += s2;
    _msg += BLINKER_F("\"");

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, const char str2[])
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":\"");
    _msg += STRING_format(str2);
    _msg += BLINKER_F("\"");

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, char c)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(c);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, unsigned char b)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(b);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, unsigned int n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, unsigned long n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T1>
void BlinkerApi::print(T1 n1, double n)
{
    String _msg = BLINKER_F("\"");
    _msg += STRING_format(n1);
    _msg += BLINKER_F("\":");
    _msg += STRING_format(n);

    // checkFormat();
    // autoFormatData(STRING_format(n1), _msg);
    // autoFormatFreshTime = millis();
    BProto::print(STRING_format(n1), _msg);
}

template <typename T>
void BlinkerApi::notify(T n)
{    
    print(BLINKER_CMD_NOTICE, STRING_format(n));
}

void BlinkerApi::vibrate(uint16_t ms)
{
    if (ms > 1000) ms = 1000;

    print(BLINKER_CMD_VIBRATE, ms);
}

void BlinkerApi::delay(unsigned long ms)
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

void BlinkerApi::attachAhrs()
{
    bool state = false;
    uint32_t startTime = millis();
    print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
    while (!state)
    {
        while (!BProto::connected())
        {
            run();
            if (BProto::connect())
            {
                print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                BProto::printNow();
                break;
            }
        }

        ::delay(100);

        if (BProto::checkAvail())
        {
            if (STRING_contains_string(BProto::dataParse(), BLINKER_CMD_AHRS))
            {
                BLINKER_LOG(BLINKER_F("AHRS attach sucessed..."));
                // parse(BProto::dataParse());
                state = true;
                break;
            }
            else
            {
                BLINKER_LOG(BLINKER_F("AHRS attach failed...Try again"));
                startTime = millis();
                // parse(BProto::dataParse());
                print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                BProto::printNow();
            }
        }
        else
        {
            if (millis() - startTime > BLINKER_CONNECT_TIMEOUT_MS)
            {
                BLINKER_LOG(BLINKER_F("AHRS attach failed...Try again"));
                startTime = millis();
                print(BLINKER_CMD_AHRS, BLINKER_CMD_ON);
                BProto::printNow();
            }
        }
    }
}

void BlinkerApi::detachAhrs()
{
    print(BLINKER_CMD_AHRS, BLINKER_CMD_OFF);
    ahrsValue[Yaw] = 0;
    ahrsValue[Roll] = 0;
    ahrsValue[Pitch] = 0;
}

float BlinkerApi::gps(b_gps_t axis)
{
    if ((millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT || \
        gps_get_time == 0)
    {
        print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
        BProto::printNow();
        delay(100);
    }

    return gpsValue[axis]*1000000;
}

void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
    if(num >= 0 ) _Widgets_str[num]->setFunc(_func);
}

#if defined(BLINKER_BLE)
    void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_joy_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
        if(num >= 0 ) _Widgets_joy[num]->setFunc(_func);
    }
#endif

void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_rgb_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
    if(num >= 0 ) _Widgets_rgb[num]->setFunc(_func);
}

void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_int32_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
    if(num >= 0 ) _Widgets_int[num]->setFunc(_func);
}

uint8_t BlinkerApi::attachWidget(char _name[], blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_str < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_str[_wCount_str] = new BlinkerWidgets_string(_name, _func);
            _wCount_str++;
            
            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_str: "), _wCount_str);
            return _wCount_str;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                BLINKER_F(" < has been registered, please register another name!"));
        return 0;
    }
    else
    {
        return 0;
    }
}

#if defined(BLINKER_BLE)
    uint8_t BlinkerApi::attachWidget(char _name[], blinker_callback_with_joy_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
        if (num == BLINKER_OBJECT_NOT_AVAIL)
        {
            if (_wCount_joy < BLINKER_MAX_WIDGET_SIZE/2)
            {
                _Widgets_joy[_wCount_joy] = new BlinkerWidgets_joy(_name, _func);
                _wCount_joy++;

                BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                BLINKER_F(" _wCount_joy: "), _wCount_joy);

                return _wCount_joy;
            }
            else
            {
                return 0;
            }
        }
        else if(num >= 0 )
        {
            BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                    BLINKER_F(" < has been registered, please register another name!"));
            return 0;
        }
        else
        {
            return 0;
        }
    }
#endif

uint8_t BlinkerApi::attachWidget(char _name[], blinker_callback_with_rgb_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_rgb, _wCount_rgb);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_rgb < BLINKER_MAX_WIDGET_SIZE/2)
        {
            _Widgets_rgb[_wCount_rgb] = new BlinkerWidgets_rgb(_name, _func);
            _wCount_rgb++;
            
            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_rgb: "), _wCount_rgb);

            return _wCount_rgb;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                BLINKER_F(" < has been registered, please register another name!"));
        return 0;
    }
    else
    {
        return 0;
    }
}

uint8_t BlinkerApi::attachWidget(char _name[], blinker_callback_with_int32_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_int, _wCount_int);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_int < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_int[_wCount_int] = new BlinkerWidgets_int32(_name, _func);
            _wCount_int++;

            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_int: "), _wCount_int);

            return _wCount_int;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(BLINKER_F("widgets name > "), _name, \
                BLINKER_F(" < has been registered, please register another name!"));
        return 0;
    }
    else
    {
        return 0;
    }
}

void BlinkerApi::attachSwitch(blinker_callback_with_string_arg_t _func)
{
    // if (!_BUILTIN_SWITCH)
    // {
    //     _BUILTIN_SWITCH = new BlinkerWidgets_string(BLINKER_CMD_BUILTIN_SWITCH, _func);
    // }
    // else
    // {
    //     _BUILTIN_SWITCH->setFunc(_func);
    // }

    _BUILTIN_SWITCH.setFunc(_func);
}

char * BlinkerApi::widgetName_str(uint8_t num)
{
    if (num) return _Widgets_str[num - 1]->getName();
    else return NULL;
}

#if defined(BLINKER_BLE)
    char * BlinkerApi::widgetName_joy(uint8_t num)
    {
        if (num) return _Widgets_joy[num - 1]->getName();
        else return NULL;
    }
#endif

char * BlinkerApi::widgetName_rgb(uint8_t num)
{
    if (num) return _Widgets_rgb[num - 1]->getName();
    else return NULL;
}

char * BlinkerApi::widgetName_int(uint8_t num)
{
    if (num) return _Widgets_int[num - 1]->getName();
    else return NULL;
}

#if defined(BLINKER_ARDUINOJSON)
    int16_t BlinkerApi::ahrs(b_ahrsattitude_t attitude, const JsonObject& data)
    {
        if (data.containsKey(BLINKER_CMD_AHRS)) {
            int16_t aAttiValue = data[BLINKER_CMD_AHRS][attitude];
            ahrsValue[Yaw] = data[BLINKER_CMD_AHRS][Yaw];
            ahrsValue[Roll] = data[BLINKER_CMD_AHRS][Roll];
            ahrsValue[Pitch] = data[BLINKER_CMD_AHRS][Pitch];

            _fresh = true;

            return aAttiValue;
        }
        else {
            return ahrsValue[attitude];
        }
    }

    float BlinkerApi::gps(b_gps_t axis, const JsonObject& data)
    {
        // if (((millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT || 
        //     gps_get_time == 0) && !newData)
        // {
        //     static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
        //     static_cast<Proto*>(this)->printNow();
        //     delay(100);
        // }

        if (data.containsKey(BLINKER_CMD_GPS)) {
            String gpsValue_LONG = data[BLINKER_CMD_GPS][LONG];
            String gpsValue_LAT = data[BLINKER_CMD_GPS][LAT];
            gpsValue[LONG] = gpsValue_LONG.toFloat();
            gpsValue[LAT] = gpsValue_LAT.toFloat();

            _fresh = true;

            if (_fresh) {
                BProto::isParsed();
                gps_get_time = millis();
            }

            return gpsValue[axis]*1000000;
        }
        else {
            return gpsValue[axis]*1000000;
        }
    }

    void BlinkerApi::heartBeat(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_GET];

        if (state.length())
        {
            if (state == BLINKER_CMD_STATE)
            {
                #if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                    print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                #else
                    print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                #endif

                #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || \
                    defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
                    String _timer = taskCount ? "1":"0";
                    _timer += _cdState ? "1":"0";
                    _timer += _lpState ? "1":"0";

                    BLINKER_LOG_ALL(BLINKER_F("timer codes: "), _timer);

                    print(BLINKER_CMD_TIMER, _timer);
                    // static_cast<Proto*>(this)->printJson(timerSetting());

                    #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
                        defined(BLINKER_AT_MQTT)
                        print(BLINKER_CMD_VERSION, BLINKER_OTA_VERSION_CODE);
                    #endif
                #endif

                if (_heartbeatFunc) {
                    _heartbeatFunc();
                }

                if (_summaryFunc) {
                    String summary_data = _summaryFunc();
                    if (summary_data.length()) {
                        summary_data = summary_data.substring(0, BLINKER_MAX_SUMMARY_DATA_SIZE);

                        BLINKER_LOG_ALL(BLINKER_F("summary_data: "), summary_data);

                        print(BLINKER_CMD_SUMMARY, summary_data);
                    }
                }

                BProto::checkState(false);
                BProto::printNow();
                _fresh = true;

                // #if defined(BLINKER_AT_MQTT)
                //     static_cast<Proto*>(this)->atHeartbeat();
                // #endif
            }
        }
    }

    void BlinkerApi::getVersion(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_GET];
        
        if (state.length())
        {
            if (state == BLINKER_CMD_VERSION)
            {
                print(BLINKER_CMD_VERSION, "0.1.0");
                _fresh = true;
            }
        }
    }

    void BlinkerApi::setSwitch(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_BUILTIN_SWITCH];

        if (state.length())
        {
            // if (_BUILTIN_SWITCH)
            // {
            //     blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

            //     if (sFunc) sFunc(state);
            // }
            blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH.getFunc();

            if (sFunc) sFunc(state);
            _fresh = true;
        }
    }

    #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || defined(BLINKER_AT_MQTT)
        void BlinkerApi::bridgeParse(char * _bName, const JsonObject& data)
        {
            int8_t num = checkNum(_bName, _Bridge, _bridgeCount);

            if (num == BLINKER_OBJECT_NOT_AVAIL ||
                !data.containsKey(BLINKER_CMD_FROMDEVICE))
            {
                return;
            }
            
            String _name = data[BLINKER_CMD_FROMDEVICE];

            // if (data.containsKey(_bName))
            if (_name == _bName)
            {
                String state = data[BLINKER_CMD_DATA];//[_bName];

                _fresh = true;
                
                BLINKER_LOG_ALL(BLINKER_F("bridgeParse: "), _bName);

                blinker_callback_with_string_arg_t nbFunc = _Bridge[num]->getFunc();
                
                if (nbFunc) nbFunc(state);
            }
        }
    #endif

    void BlinkerApi::strWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            String state = data[_wName];

            _fresh = true;
            
            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse: "), _wName);

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
            
            if (nbFunc) nbFunc(state);
        }
    }

    #if defined(BLINKER_BLE)
        void BlinkerApi::joyWidgetsParse(char _wName[], const JsonObject& data)
        {
            int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

            if (num == BLINKER_OBJECT_NOT_AVAIL) return;

            if (data.containsKey(_wName))
            {
                int16_t jxAxisValue = data[_wName][BLINKER_J_Xaxis];
                uint8_t jyAxisValue = data[_wName][BLINKER_J_Yaxis];

                _fresh = true;

                blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();
                if (wFunc) wFunc(jxAxisValue, jyAxisValue);
            }
        }
    #endif

    void BlinkerApi::rgbWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            uint8_t _rValue = data[_wName][BLINKER_R];
            uint8_t _gValue = data[_wName][BLINKER_G];
            uint8_t _bValue = data[_wName][BLINKER_B];
            uint8_t _brightValue = data[_wName][BLINKER_BRIGHT];

            _fresh = true;

            blinker_callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();
            if (wFunc) wFunc(_rValue, _gValue, _bValue, _brightValue);
        }
    }

    void BlinkerApi::intWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName)) {
            int _number = data[_wName];

            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
            if (wFunc) {
                wFunc(_number);
            }
        }
    }

    void BlinkerApi::json_parse(const JsonObject& data)
    {
        setSwitch(data);

        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), data);
        }
        for (uint8_t wNum_int = 0; wNum_int < _wCount_int; wNum_int++) {
            intWidgetsParse(_Widgets_int[wNum_int]->getName(), data);
        }
        for (uint8_t wNum_rgb = 0; wNum_rgb < _wCount_rgb; wNum_rgb++) {
            rgbWidgetsParse(_Widgets_rgb[wNum_rgb]->getName(), data);
        }
        #if defined(BLINKER_BLE)
            for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
                joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), data);
            }
        #endif
    }

#else

    int16_t BlinkerApi::ahrs(b_ahrsattitude_t attitude, char data[])
    {
        int16_t aAttiValue = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, attitude);

        if (aAttiValue != FIND_KEY_VALUE_FAILED)
        {
            ahrsValue[Yaw] = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, Yaw);
            ahrsValue[Roll] = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, Roll);
            ahrsValue[Pitch] = STRING_find_array_numberic_value(data, BLINKER_CMD_AHRS, Pitch);

            _fresh = true;

            return aAttiValue;
        }
        else {
            return ahrsValue[attitude];
        }
    }

    float BlinkerApi::gps(b_gps_t axis, char data[])
    {
        // if (((millis() - gps_get_time) >= BLINKER_GPS_MSG_LIMIT || 
        //     gps_get_time == 0) && !newData)
        // {
        //     static_cast<Proto*>(this)->print(BLINKER_CMD_GET, BLINKER_CMD_GPS);
        //     static_cast<Proto*>(this)->printNow();
        //     delay(100);
        // }

        String axisValue = STRING_find_array_string_value(data, BLINKER_CMD_GPS, axis);

        if (axisValue != "") {
            gpsValue[LONG] = STRING_find_array_string_value(data, BLINKER_CMD_GPS, LONG).toFloat();
            gpsValue[LAT] = STRING_find_array_string_value(data, BLINKER_CMD_GPS, LAT).toFloat();

            _fresh = true;

            if (_fresh) {
                BProto::isParsed();
                gps_get_time = millis();
            }

            return gpsValue[axis]*1000000;
        }
        else {
            return gpsValue[axis]*1000000;
        }
    }

    void BlinkerApi::heartBeat(char data[])
    {
        if (strstr(data, BLINKER_CMD_GET) && \
            strstr(data, BLINKER_CMD_STATE))
        {
            #if defined(BLINKER_BLE)
                print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
            #else
                print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
            #endif

            if (_heartbeatFunc) {
                _heartbeatFunc();
            }

            if (_summaryFunc)
            {
                String summary_data = _summaryFunc();
                if (summary_data.length())
                {
                    summary_data = summary_data.substring(0, BLINKER_MAX_SUMMARY_DATA_SIZE);

                    BLINKER_LOG_ALL(BLINKER_F("summary_data: "), summary_data);

                    print(BLINKER_CMD_SUMMARY, summary_data);
                }
            }
            
            BProto::checkState(false);
            BProto::printNow();

            _fresh = true;

            // #if defined(BLINKER_AT_MQTT)
            //     static_cast<Proto*>(this)->atHeartbeat();
            // #endif
        }
    }

    void BlinkerApi::getVersion(char data[])
    {
        if (strstr(data, BLINKER_CMD_GET) && \
            strstr(data, BLINKER_CMD_VERSION))
        {
            print(BLINKER_CMD_VERSION, BLINKER_VERSION);
            _fresh = true;
        }
    }

    void BlinkerApi::setSwitch(char data[])
    {
        String state;

        if (STRING_find_string_value(data, state, BLINKER_CMD_BUILTIN_SWITCH))
        {
            // if (_BUILTIN_SWITCH)
            // {
            //     blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

            //     if (sFunc) sFunc(state);
            // }
            blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH.getFunc();

            if (sFunc) sFunc(state);
            _fresh = true;
        }
    }

    void BlinkerApi::strWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        String state;

        if (STRING_find_string_value(_data, state, _wName))
        {
            _fresh = true;

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
            if (nbFunc) nbFunc(state);
        }
    }

    #if defined(BLINKER_BLE)
        void BlinkerApi::joyWidgetsParse(char _wName[], char _data[])
        {
            int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

            if (num == BLINKER_OBJECT_NOT_AVAIL) return;

            int16_t jxAxisValue = STRING_find_array_numberic_value(_data, \
                                                _wName, BLINKER_J_Xaxis);

            if (jxAxisValue != FIND_KEY_VALUE_FAILED)
            {
                uint8_t jyAxisValue = STRING_find_array_numberic_value(_data, \
                                                    _wName, BLINKER_J_Yaxis);

                _fresh = true;

                blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();

                if (wFunc) wFunc(jxAxisValue, jyAxisValue);
            }
        }
    #endif

    void BlinkerApi::rgbWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_rgb, _wCount_rgb);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        int16_t _rValue = STRING_find_array_numberic_value(_data, \
                                                _wName, BLINKER_R);

        if (_rValue != FIND_KEY_VALUE_FAILED)
        {
            uint8_t _gValue = STRING_find_array_numberic_value(_data, _wName, BLINKER_G);
            uint8_t _bValue = STRING_find_array_numberic_value(_data, _wName, BLINKER_B);
            uint8_t _brightValue = STRING_find_array_numberic_value(_data, _wName, BLINKER_BRIGHT);

            _fresh = true;

            blinker_callback_with_rgb_arg_t wFunc = _Widgets_rgb[num]->getFunc();

            if (wFunc) wFunc(_rValue, _gValue, _bValue, _brightValue);
        }
    }

    void BlinkerApi::intWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_int, _wCount_int);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        int _number = STRING_find_numberic_value(_data, _wName);

        if (_number != FIND_KEY_VALUE_FAILED)
        {
            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();

            if (wFunc) wFunc(_number);
        }
    }

    void BlinkerApi::json_parse(char _data[])
    {
        setSwitch(_data);        

        BLINKER_LOG_ALL("====_wCount_str: ", _wCount_str, " ====");

        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), _data);
        }
        for (uint8_t wNum_int = 0; wNum_int < _wCount_int; wNum_int++) {
            intWidgetsParse(_Widgets_int[wNum_int]->getName(), _data);
        }
        for (uint8_t wNum_rgb = 0; wNum_rgb < _wCount_rgb; wNum_rgb++) {
            rgbWidgetsParse(_Widgets_rgb[wNum_rgb]->getName(), _data);
        }
        #if defined(BLINKER_BLE)
            for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
                joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), _data);
            }
        #endif
    }
#endif

#endif
