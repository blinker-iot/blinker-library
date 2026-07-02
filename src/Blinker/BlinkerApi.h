#if !defined(BLINKER_API_H)
#define BLINKER_API_H

#include <time.h>
#include "BlinkerWiFiPlatform.h"

#if defined(BLINKER_NATIVE_WIFI)
    #if defined(ESP32)
        #include <Ticker.h>
    #endif
    #include <EEPROM.h>

    #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
        #include "BlinkerAuto.h"
    #endif

#else
    #include "../Functions/BlinkerTicker.h"
#endif

#if defined(BLINKER_NATIVE_WIFI) && !defined(BLINKER_BLE)
    #if defined(BLINKER_WIFI_MULTI) && defined(BLINKER_WIFI_HAS_MULTI)
    extern WiFiMulti wifiMulti;
    #endif
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
    #include "BlinkerTimer.h"
    #include "../Functions/BlinkerTimingTimer.h"
#endif







#include "BlinkerApiBase.h"
#include "BlinkerProtocol.h"

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

        void printJson(const String &s);

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

        // template <typename T1>
        void printNumArray(char * _name, const String & data);

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


        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)

            void setTimezone(float tz);

            float getTimezone() {
                return _timezone;
            }
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


            // void deviceHeartbeat(uint32_t heart_time = 600);


                void loadTimer();
                void deleteTimer();
                void deleteCountdown();
                void deleteLoop();
                void deleteTiming();
                bool countdownState()   { return _cdState; }
                bool loopState()        { return _lpState; }
                bool timingState()      { return taskCount ? true : false; }

            template<typename T>
            bool configUpdate(const T& msg);
            void configGet();
            bool configDelete();
            template<typename T>
            void dataStorage(char _name[], const T& msg);
            template<typename T>
            void sendRtData(char _name[], const T& msg);
            void printRtData();
            void timeSlotData(char _name[], int32_t msg);
            void timeSlotData(char _name[], uint32_t msg);
            void timeSlotData(char _name[], float msg);
            void textData(const String & msg);
            void jsonData(const String & msg);
            void jsonDataGet();
            bool dataUpdate();
            void dataGet();
            void dataGet(const String & _type);
            void dataGet(const String & _type, const String & _date);
            bool dataDelete();
            bool dataDelete(const String & _type);
            bool event(const String & _key, String _value = "");

                bool autoPull();
                void autoInit()         { autoStart(); }
                // void autoInput(const String & key, const String & state);
                void autoInput(const String & key, float data);
                void autoRun();

            String freshSharers();


            #if defined(BLINKER_LOWPOWER)
                int32_t comFreqGet();
                bool comFreqUpdate();
                String comDataGet();
                bool comDateUpdate();
            #endif



        #endif


        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

            void attachRTData(blinker_callback_t newFunction, uint8_t rt_sec = 1)
            {
                // strcpy(_RTDataKey, _name);
                _RTDataFunc = newFunction;

                if (rt_sec > 0 && rt_sec < 9)
                {
                    _RTTime = rt_sec;
                }
            }

        #endif

        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
            void reset()
            {
                BLINKER_LOG(BLINKER_F("Blinker reset..."));
                char config_check[3] = {0};
                blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, config_check);
                blinkerEEPROMCommit();
                blinkerEEPROMEnd();

                blinkerWiFiRestart();
            }
        #endif


        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)

            void attachDataStorage(blinker_callback_t newFunction, uint32_t _time = 60, uint8_t d_times = BLINKER_DATA_UPDATE_COUNT)
            {
                _dataStorageFunc = newFunction;
                if (_time < 5) _time = 5;
                _autoStorageTime = _time;
                _autoDataTime = millis();
                if (d_times > BLINKER_MAX_DATA_COUNT || d_times == 0) d_times = BLINKER_DATA_UPDATE_COUNT;
                _dataTimes = d_times;
            }
            // #endif

            #if defined(BLINKER_LOWPOWER)
                void attachLowPower(blinker_callback_t newFunction, uint32_t _time)
                {
                    _LowPowerFunc = newFunction;
                    _LowPowerFreq = _time;
                }

                void attachSleep(blinker_callback_t newFunction)
                {
                    _sleepFunc = newFunction;
                }
            #endif

            bool init()                         { return _isInit; }

        #endif

        void attachData(blinker_callback_with_string_arg_t newFunction)
        { BProto::_availableFunc = newFunction; }
        void attachHeartbeat(blinker_callback_t newFunction)
        { _heartbeatFunc = newFunction; }
        void attachSummary(blinker_callback_return_string_t newFunction)
        { _summaryFunc = newFunction; }
        void attachConfigGet(blinker_callback_with_string_arg_t newFunction)
        { _configGetFunc = newFunction; }
        void attachDataGet(blinker_callback_with_string_arg_t newFunction)
        { _dataGetFunc = newFunction; }

        void freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        // #if defined(BLINKER_BLE)
            void freshAttachWidget(char _name[], blinker_callback_with_joy_arg_t _func);
        // #endif
        void freshAttachWidget(char _name[], blinker_callback_with_rgb_arg_t _func);
        void freshAttachWidget(char _name[], blinker_callback_with_int32_arg_t _func);
        void freshAttachWidget(char _name[], blinker_callback_with_table_arg_t _func, blinker_callback_t _func2);
        uint8_t attachWidget(char _name[], blinker_callback_with_string_arg_t _func);
        // #if defined(BLINKER_BLE)
            uint8_t attachWidget(char _name[], blinker_callback_with_joy_arg_t _func);
        // #endif
        uint8_t attachWidget(char _name[], blinker_callback_with_rgb_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_int32_arg_t _func);
        uint8_t attachWidget(char _name[], blinker_callback_with_table_arg_t _func, blinker_callback_t _func2);
        void attachSwitch(blinker_callback_with_string_arg_t _func);
        char * widgetName_str(uint8_t num);
        // #if defined(BLINKER_BLE)
            char * widgetName_joy(uint8_t num);
        // #endif
        char * widgetName_rgb(uint8_t num);
        char * widgetName_int(uint8_t num);
        char * widgetName_tab(uint8_t num);






        #if defined(BLINKER_HTTP)
            void subscribe() { BProto::subscribe(); }
        #endif

    private :
        bool        _isNew = false;
        bool        _fresh = false;
        int16_t     ahrsValue[3];

        uint8_t     _wCount_num = 0;
        uint8_t     _wCount_str = 0;
        uint8_t     _wCount_joy = 0;
        uint8_t     _wCount_rgb = 0;
        uint8_t     _wCount_int = 0;
        uint8_t     _wCount_tab = 0;

        class BlinkerWidgets_num *          _Widgets_num[BLINKER_MAX_WIDGET_SIZE*2];
        class BlinkerWidgets_string *       _Widgets_str[BLINKER_MAX_WIDGET_SIZE*2];
        // #if defined(BLINKER_BLE)
            class BlinkerWidgets_joy *          _Widgets_joy[BLINKER_MAX_WIDGET_SIZE/2];
        // #endif
        class BlinkerWidgets_rgb *          _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
        class BlinkerWidgets_int32 *        _Widgets_int[BLINKER_MAX_WIDGET_SIZE*2];
        class BlinkerWidgets_table *        _Widgets_tab[BLINKER_MAX_WIDGET_SIZE*2];
        // class BlinkerWidgets_string *       _BUILTIN_SWITCH;
        BlinkerWidgets_string _BUILTIN_SWITCH = BlinkerWidgets_string(BLINKER_CMD_BUILTIN_SWITCH);

        bool _needInit = false;

        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
            bool        _isInit = false;
            bool        _isAuto = false;
            bool        _isAutoInit = false;
            uint8_t     _disconnectCount = 0;
            uint32_t    _disFreshTime = 0;
            uint32_t    _disconnectTime = 0;
            uint32_t    _refreshTime = 0;
            uint32_t    _reconTime = 0;

            bool        _isNTPInit = false;
            time_t      _deviceStartTime = 0;
            float       _timezone = 8.0;
            uint32_t    _ntpStart;

            uint8_t     data_dataCount = 0;
            uint8_t     data_rtDataCount = 0;
            uint8_t     data_rtKeyCount = 0;
            uint8_t     data_rtTimes = 0;
            time_t      data_rtTime = 0;
            bool        data_rtRun = false;
            uint8_t     data_timeSlotDataCount = 0;
            uint32_t    time_timeSlotData = 0;
            uint8_t     _aCount = 0;
            uint32_t    _cUpdateTime = 0;
            uint32_t    _dUpdateTime = 0;
            uint32_t    _cGetTime = 0;
            uint32_t    _cDelTime = 0;
            uint32_t    _dGetTime = 0;
            uint32_t    _dDelTime = 0;
            uint32_t    _autoPullTime = 0;

            uint32_t    _autoUpdateTime = 0;

            uint32_t    _dHeartTime = 0;

                char                            _cdAction[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
                char                            _lpAction1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
                char                            _lpAction2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
                class BlinkerTimingTimer *      timingTask[BLINKER_TIMING_TIMER_SIZE];
            class BlinkerData *             _Data[BLINKER_MAX_BLINKER_DATA_SIZE];
            // class BlinkerRTData *           _RTData[BLINKER_MAX_RTDATA_SIZE];

            class BlinkerTimeSlotData *     _TimeSlotData[BLINKER_MAX_BLINKER_DATA_SIZE];

                class BlinkerAUTO *             _AUTO[2];
        #endif





        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
            char                                _RTDataKey[BLINKER_MAX_RTDATA_SIZE][16];
            uint8_t                             _RTKeyCount = 0;
            blinker_callback_t                  _RTDataFunc = NULL;
            uint8_t                             _RTTimesCount = 0;
            Ticker                              _RTTicker;
            uint8_t                            _RTTime = 1;
        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)





            blinker_callback_t                  _dataStorageFunc = NULL;
            uint32_t                            _autoStorageTime = 60;
            uint32_t                            _autoDataTime = 0;
            uint8_t                             _dataTimes = BLINKER_MAX_DATA_COUNT;
            // #endif

            #if defined(BLINKER_LOWPOWER)
            blinker_callback_t                  _LowPowerFunc = NULL;
            uint32_t                            _LowPowerFreq = 10;
            // char*                               _LowPowerData;
            blinker_callback_t                  _sleepFunc = NULL;
            #endif
        #endif

        blinker_callback_t                  _heartbeatFunc = NULL;
        blinker_callback_return_string_t    _summaryFunc = NULL;
        blinker_callback_with_string_arg_t  _configGetFunc = NULL;
        blinker_callback_with_string_arg_t  _dataGetFunc = NULL;

        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
            void rtParse(const JsonObject& data);
        #endif

        void parse(char _data[], bool ex_data = false);

        bool deviceHeartbeat(uint32_t heart_time = BLINKER_DEVICE_HEARTBEAT_TIME);

        #if defined(BLINKER_ARDUINOJSON)
            int16_t ahrs(b_ahrsattitude_t attitude, const JsonObject& data);

            void heartBeat(const JsonObject& data);
            void getVersion(const JsonObject& data);
            void setSwitch(const JsonObject& data);

            void strWidgetsParse(char _wName[], const JsonObject& data);
            // #if defined(BLINKER_BLE)
                void joyWidgetsParse(char _wName[], const JsonObject& data);
            // #endif
            void rgbWidgetsParse(char _wName[], const JsonObject& data);
            void intWidgetsParse(char _wName[], const JsonObject& data);
            void tabWidgetsParse(char _wName[], const JsonObject& data);

            void json_parse(const JsonObject& data);
        #else
            int16_t ahrs(b_ahrsattitude_t attitude, char data[]);

            void heartBeat(char data[]);
            void getVersion(char data[]);
            void setSwitch(char data[]);

            void strWidgetsParse(char _wName[], char _data[]);
            // #if defined(BLINKER_BLE)
                void joyWidgetsParse(char _wName[], char _data[]);
            // #endif
            void rgbWidgetsParse(char _wName[], char _data[]);
            void intWidgetsParse(char _wName[], char _data[]);
            void tabWidgetsParse(char _wName[], char _data[]);

            void json_parse(char _data[]);
        #endif



        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)


                void beginAuto();
                bool autoTrigged(uint32_t _id);

                bool ntpInit();
                void freshNTP();
                void ntpConfig();

                void saveCountDown(uint32_t _data, char _action[]);
                void saveLoop(uint32_t _data, char _action1[], char _action2[]);
                void loadCountdown();
                void loadLoop();
                void loadTiming();
                void checkOverlapping(uint8_t checkDays, uint16_t checkMins, uint8_t taskNum);
                void freshTiming(uint8_t wDay, uint16_t nowMins);
                void deleteTiming(uint8_t taskDel);
                void addTimingTask(uint8_t taskSet, uint32_t timerData, const String & action);

                // #if defined(ESP32)
                //     uint8_t isErase;
                // #endif

                void checkTimerErase();

                String timerSetting();
                String countdownConfig();
                String loopConfig();
                String timingConfig();
                String getTimingCfg(uint8_t task);
                bool timerManager(const JsonObject& data, bool _noSet = false);
                bool checkTimer();


            bool checkCUPDATE();
            bool checkCGET();
            bool checkCDEL();
            bool checkDataUpdata();
            bool checkDataGet();
            bool checkDataDel();
            bool checkAutoPull();


            void autoStart();
            bool autoManager(const JsonObject& data);

                void shareParse(const JsonObject& data);

                    void numParse(const JsonObject& data);


                // String postServer(const String & url, const String & host, int port, const String & msg);
                // String getServer(const String & url, const String & host, int port);
                String blinkerServer(uint8_t _type, const String & msg, bool state = false);


            uint32_t ntpFreshTime = 0;
            time_t ntpGetTime = 0;

        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
        #endif



    protected :
        uint32_t log_time = 0;
        void begin();

        void needInit();




        uint32_t debug_time = 0;

};

void BlinkerApi::begin()
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

void BlinkerApi::needInit()
{
    #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
        BLINKER_LOG_ALL(BLINKER_F("==== needInit ===="));
            String _shareData = freshSharers();
            if (STRING_contains_string(_shareData, "users") == false)
            {
                _shareData = freshSharers();
            }
            if (STRING_contains_string(_shareData, "users") == true)
            {
                BProto::sharers(_shareData);
            }

            loadTiming();

        ::delay(100);

        // BProto::ping();
        // BProto::disconnect();
        // BProto::connect();

        #if defined(BLINKER_LOWPOWER)//
            int32_t _freq_get = comFreqGet();

            if (_freq_get < 1) _freq_get = comFreqGet();

            if (_freq_get >= 1) _LowPowerFreq = _freq_get;
        #endif
    #endif

}




void BlinkerApi::run()
{
    #if defined(BLINKER_NATIVE_WIFI) && !defined(ESP32) && (defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP))
        cdTicker.run();
        lpTicker.run();
        tmTicker.run();
    #endif

        // delayMicroseconds(1);
    // #else
        // BLINKER_LOG_ALL(BLINKER_F("BProto::run00001"));

        // BLINKER_LOG_ALL(BLINKER_F("BProto::run0001"));


        // BLINKER_LOG_ALL(BLINKER_F("BProto::run001"));


        // BLINKER_LOG_ALL(BLINKER_F("BProto::run01"));


        // BLINKER_LOG_ALL(BLINKER_F("BProto::run0"));
        // #if defined(BLINKER_HTTP)
        // #endif
        // ::delay(1000);
        // if (WiFi.status() != WL_CONNECTED)
        // {
        //     BLINKER_LOG_ALL(BLINKER_F("!WL_CONNECTED============"));
        //     return;
        // }
        // BLINKER_LOG_ALL(BLINKER_F("============WL_CONNECTED"));


        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
            checkTimer();

            if (!BProto::init()) {
                ::delay(10);
                // BLINKER_LOG_ALL(BLINKER_F("RETURN"));

                return;
            }

            if (!_isInit)
            {
                if (ntpInit())
                {
                    _isInit =true;
                    _disconnectTime = millis();

                    uint32_t connect_time = millis();
                    uint32_t time_slot = 0;

                    if (_needInit == false)
                    {
                        _needInit = true;
                        needInit();

                        #if defined(BLINKER_LOWPOWER)
                            return;
                        #endif
                    }

                    // while (time_slot < 30000)
                    // {
                    //     time_slot = millis() - connect_time;
                    //     BProto::connect();
                    //     yield();

                    //     if (BProto::mConnected())
                    //     {
                    //         state = CONNECTED;
                    //         break;
                    //     }
                    // }

                    BLINKER_LOG_ALL(BLINKER_F("millis: "), millis(),
                                    BLINKER_F(", connect_time: "), connect_time);
                    // BProto::sharers(freshSharers());

                    BLINKER_LOG_ALL(BLINKER_F("MQTT conn init success"));

                    beginAuto();
                }
                else
                {
                    return;
                }

            }
            else {
                ntpInit();
            }

            #if defined(BLINKER_WIFI_MULTI)
                if (wifiMulti.run() != WL_CONNECTED)
                {
                    if ((millis() - _reconTime) >= 10000 || \
                        _reconTime == 0 )
                    {
                        _reconTime = millis();
                        BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));
                    }

                    return;
                }
            #else
                if (WiFi.status() != WL_CONNECTED)
                {
                    if ((millis() - _reconTime) >= 10000 || \
                        _reconTime == 0 )
                    {
                        _reconTime = millis();
                        BLINKER_LOG(BLINKER_F("WiFi disconnected! reconnecting!"));
                        blinkerWiFiReconnect();
                    }

                    return;
                }
            #endif
        #endif





        // BLINKER_LOG_ALL(BLINKER_F("BProto::run1"));

        #if defined(BLINKER_LOWPOWER)
            BLINKER_LOG_ALL(BLINKER_F("LOW POWER"));

            char _lp_data_get[1024];
            strcpy(_lp_data_get, comDataGet().c_str());
            if (_lp_data_get == BLINKER_CMD_FALSE) strcpy(_lp_data_get, "");
            else if (strcmp(_lp_data_get, "{}") == 0) strcpy(_lp_data_get, "");
            else
            {
                flush();
                parse(_lp_data_get);

                if (!_fresh)
                {
                    if (BProto::_availableFunc)
                    {
                        BProto::_availableFunc(_lp_data_get);
                        flush();
                    }
                }
            }

            if (_LowPowerFunc) _LowPowerFunc();

            if (strlen(BProto::_sendBuf))
            {
                if (!comDateUpdate()) comDateUpdate();
            }
            // ::delay(60000); // sleep func TBD
            if (_sleepFunc) _sleepFunc();
            return;
        #endif

        // BLINKER_LOG_ALL(BLINKER_F("BProto::run"));

        //     BProto::checkAutoFormat();
        //     return; // TBD
        // #endif

        bool conState = BProto::connect();

        // delay(10);


        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
            if (((millis() - _dHeartTime)/1000 >= BLINKER_DEVICE_HEARTBEAT_TIME) && _isInit)
            {
                deviceHeartbeat();
                _dHeartTime = millis();
            }
        #endif

        // if (millis() - log_time >= 1000)
        // {
        //     BLINKER_LOG_ALL(BLINKER_F("BProto::run1"));
        //     log_time += 1000;
        //     BLINKER_LOG_ALL(BLINKER_F("BProto::state: "), BProto::state);
        // }

        switch (BProto::state)
        {
            // BLINKER_LOG_ALL(BLINKER_F("BProto::state: "), BProto::state);
            case CONNECTING :
                if (BProto::connect())
                {
                    BProto::state = CONNECTED;

                    #if defined(BLINKER_MQTT)
                        _disconnectCount = 0;
                    #endif
                }
                else
                {
                    #if defined(BLINKER_MQTT)
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

                    #if defined(BLINKER_MQTT)
                        if (BProto::needFreshShare())
                        {
                            String _shareData = freshSharers();
                            if (STRING_contains_string(_shareData, "users") == false)
                            {
                                _shareData = freshSharers();
                            }
                            if (STRING_contains_string(_shareData, "users") == true)
                            {
                                BProto::sharers(_shareData);
                            }
                        }
                    #endif
                }
                else
                {
                    BProto::disconnect();
                    BProto::state = CONNECTING;

                    #if defined(BLINKER_MQTT)
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

                                    if (_disconnectCount > 6) _disconnectCount = 6;

                                    BLINKER_LOG_ALL(BLINKER_F("_disFreshTime: "), _disFreshTime);
                                    BLINKER_LOG_ALL(BLINKER_F("_disconnectCount: "), _disconnectCount);

                                    // blinkerWiFiRestart();
                                }
                            }
                        }
                    #endif
                }
                break;
            case DISCONNECTED :
                BProto::disconnect();
                BProto::state = CONNECTING;
                break;
        }

        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)

                if (_isAuto && _isInit && state == CONNECTED && !_isAutoInit)
                {
                    // if (autoPull()) _isAutoInit = true;
                    // else
                    // {
                    //     if (autoPull()) _isAutoInit = true;
                    // } // TODO
                }

            if (_dataStorageFunc)
            {
                if (millis() - _autoDataTime >= _autoStorageTime * 1000)
                {
                    _dataStorageFunc();
                    _autoDataTime += _autoStorageTime * 1000;
                }
            }

            if (millis() - _autoUpdateTime >= _autoStorageTime * _dataTimes * 1000)
            {
                // BLINKER_LOG_ALL("dataUpdate data_dataCount: ", data_dataCount);
                // BLINKER_LOG_ALL("_isInit: ", _isInit);

                if ((data_dataCount || data_timeSlotDataCount) && _isInit)// && ESP.getFreeHeap() > 4000)
                {
                    // if (dataUpdate()) _autoUpdateTime = millis();
                    if (dataUpdate()) _autoUpdateTime = millis();
                    else
                    {
                        _autoUpdateTime = millis() - 100000;
                    }
                }
            }
            // #endif
        #endif

        BProto::checkAutoFormat();
    // #endif
}

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)
void BlinkerApi::rtParse(const JsonObject& data)
{
    // data_rtKeyCount = 0;

    // for (size_t i = 0; i < BLINKER_MAX_RTDATA_SIZE; i++)
    // {
    //     String get_key = data["rt"][i];

    //     if (get_key != "null")
    //     {
    //         BLINKER_LOG_ALL(BLINKER_F("===>rt get_key: "), get_key);

    //         for (size_t num = 0; num < data_rtDataCount; num++)
    //         {
    //             if (_RTData[num]->checkName(get_key.c_str()))
    //             {
    //                 strcpy(_RTDataKey[data_rtKeyCount], get_key.c_str());
    //                 data_rtKeyCount++;
    //             }
    //         }
    //     }
    // }


    data_rtKeyCount = 0;

    if (data.containsKey("rt")) {

        BLINKER_LOG_ALL(BLINKER_F("containsKey rt"));

        for (size_t i = 0; i < BLINKER_MAX_RTDATA_SIZE; i++)
        {
            String get_key = data["rt"][i];

            if (get_key != "null")
            {
                BLINKER_LOG_ALL(BLINKER_F("===>rt get_key: "), get_key);

                // for (size_t num = 0; num < data_rtDataCount; num++)
                // {
                //     if (_RTData[num]->checkName(get_key.c_str()))
                //     {
                //         strcpy(_RTDataKey[data_rtKeyCount], get_key.c_str());
                        data_rtKeyCount++;
                //     }
                // }
            }
            else
            {
                break;
            }
        }
    }

    BLINKER_LOG_ALL("data_rtKeyCount: ",data_rtKeyCount);

    // String rt_data_str = "";

    // bool is_print = false;

    // for (size_t data_num = 0; data_num < BLINKER_MAX_RTDATA_DATA_SIZE; data_num++)
    // {
    //     rt_data_str = "{";

    //     for (size_t key_num = 0; key_num < data_rtDataCount; key_num++)
    //     {
    //         for (size_t num = 0; num < data_rtKeyCount; num++)
    //         {
    //             BLINKER_LOG_ALL("_RTDataKey: ",_RTDataKey[num]);

    //             if (_RTData[key_num]->checkName(_RTDataKey[num]))
    //             {
    //                 is_print = _RTData[key_num]->available();
    //                 rt_data_str += _RTData[key_num]->getData();
    //             }
    //         }

    //         if (key_num + 1 < data_rtDataCount)
    //         {
    //             rt_data_str += ",";
    //         }
    //     }

    //     rt_data_str += "}";\

    //     if (is_print)
    //     {
    //         BLINKER_LOG_ALL(BLINKER_F("===>RTData: "), rt_data_str);
    //         BProto::checkState(false);
    //         printJson(rt_data_str);
    //         // BProto::printNow();
    //     }
    // }

    // for (size_t key_num = 0; key_num < data_rtDataCount; key_num++)
    // {
    //     _RTData[key_num]->flush();
    // }


    // String get_key = data["rt"][0];
    // BLINKER_LOG_ALL(BLINKER_F("_RTDataKey: "), _RTDataKey);
    // BLINKER_LOG_ALL(BLINKER_F("rt get_key: "), get_key);
    // BLINKER_LOG_ALL(BLINKER_F("strncmp: "), strncmp(_RTDataKey, get_key.c_str(), strlen(_RTDataKey)) == 0);
    if (data_rtKeyCount)
    {
        // data_rtTime = millis();

        // if (_RTDataFunc && data_rtRun == false)
        // {
        //     data_rtRun = true;
            _RTTicker.once(_RTTime, _RTDataFunc);

            data_rtTimes = 0;

            _fresh = true;

        BLINKER_LOG_ALL("========data_rtRun");
        // }
    }
    // else
    // {
    //     if (data_rtRun)
    //     {
    //         if (millis() - data_rtTime > 11000UL)
    //         {
    //             _RTTicker.detach();
    //             data_rtRun = false;
    //         }
    //     }
    // }
}
#endif

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

                // DynamicJsonBuffer jsonBuffer;
                // JsonObject& root = jsonBuffer.parseObject(STRING_format(_data));
                JsonDocument jsonBuffer;
                DeserializationError error = deserializeJson(jsonBuffer, STRING_format(_data));
                JsonObject root = jsonBuffer.as<JsonObject>();

                // if (!root.success())
                if (error)
                {
                    // #endif
                    return;
                }


                // #if defined(BLINKER_MQTT) || 0

                #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                    timerManager(root);
                    // BLINKER_LOG_ALL(BLINKER_F("timerManager"));
                #endif


                #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

                #endif

                #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                    shareParse(root);
                    autoManager(root);

                    rtParse(root);
                    numParse(root);

                #endif

                heartBeat(root);
                getVersion(root);

                json_parse(root);

                ahrs(Yaw, root);
            #else
                BLINKER_LOG_ALL(BLINKER_F("ndef BLINKER_ARDUINOJSON"));


                heartBeat(_data);
                getVersion(_data);

                json_parse(_data);

                ahrs(Yaw, _data);
            #endif

            if (_fresh)
            {
                BLINKER_LOG_ALL(BLINKER_F("_fresh need flush"));
                BProto::isParsed();
            }
            else
            {
            }
        }
    }
    else
    {
        #if defined(BLINKER_ARDUINOJSON)
            String arrayData = BLINKER_F("{\"data\":");
            arrayData += _data;
            arrayData += BLINKER_F("}");
            // DynamicJsonBuffer jsonBuffer;
            // JsonObject& root = jsonBuffer.parseObject(arrayData);
            JsonDocument jsonBuffer;
            DeserializationError error = deserializeJson(jsonBuffer, arrayData);
            JsonObject root = jsonBuffer.as<JsonObject>();

            // if (!root.success()) return;
            if (error) return;

            arrayData = root["data"][0].as<String>();

            if (arrayData != "null")
            {
                for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
                {
                    arrayData = root["data"][a_num].as<String>();

                    if(arrayData != "null") {
                        // DynamicJsonBuffer _jsonBuffer;
                        // JsonObject& _array = _jsonBuffer.parseObject(arrayData);
                        JsonDocument jsonBuffer;
                        deserializeJson(jsonBuffer, arrayData);
                        JsonObject _array = jsonBuffer.as<JsonObject>();

                        json_parse(_array);
                        #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                            timerManager(_array, true);
                        #endif

                    }
                    else {
                        return;
                    }
                }
            }
            else {
                // JsonObject& root = jsonBuffer.parseObject(_data);
                DeserializationError error = deserializeJson(jsonBuffer, _data);
                JsonObject root = jsonBuffer.as<JsonObject>();

                // if (!root.success()) return;
                if (error) return;

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

void BlinkerApi::printJson(const String &s)
{
    BProto::print(s);
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

// template <typename T1>
void BlinkerApi::printNumArray(char * _name, const String & data)
{
    // String _msg = BLINKER_F("\"");
    // _msg += STRING_format(n1);
    // _msg += BLINKER_F("\":");
    // _msg += s2;

    // // checkFormat();
    // // autoFormatData(STRING_format(n1), _msg);
    // // autoFormatFreshTime = millis();
    // BProto::print(STRING_format(n1), _msg);

    int8_t num = checkNum(_name, _Widgets_num, _wCount_num);

    if( num != BLINKER_OBJECT_NOT_AVAIL )
    {
        if (_Widgets_num[num]->state())
        {
            #if defined(ESP32) && (defined(BLINKER_MQTT) || defined(BLINKER_HTTP))
            dataStorage(_name, data);
            #endif
        }
    }
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

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
    void BlinkerApi::setTimezone(float tz)
    {
        _timezone = tz;
        _isNTPInit = false;

    }

    int8_t BlinkerApi::second()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("second (millis() - ntpFreshTime): "), (millis() - ntpFreshTime));
            //     BLINKER_LOG_ALL(BLINKER_F("ntpGetTime: "), ntpGetTime);
            //     BLINKER_LOG_ALL((ntpGetTime == 0 || (millis() - ntpFreshTime) >= 600000));

                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_sec;
        }
        return -1;
    }
    /**< seconds after the minute - [ 0 to 59 ] */

    int8_t BlinkerApi::minute()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;
            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_min;
        }
        return -1;
    }
    /**< minutes after the hour - [ 0 to 59 ] */

    int8_t BlinkerApi::hour()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_hour;
        }
        return -1;
    }
    /**< hours since midnight - [ 0 to 23 ] */

    int8_t BlinkerApi::mday()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_mday;
        }
        return -1;
    }
    /**< day of the month - [ 1 to 31 ] */

    int8_t BlinkerApi::wday()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;
            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_wday;
        }
        return -1;
    }
    /**< days since Sunday - [ 0 to 6 ] */

    int8_t BlinkerApi::month()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_mon + 1;
        }
        return -1;
    }
    /**< months since January - [ 1 to 12 ] */

    int16_t BlinkerApi::year()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_year + 1900;
        }
        return -1;
    }
    /**< years since 1900 */

    int16_t BlinkerApi::yday()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_yday + 1;
        }
        return -1;
    }
    /**< days since January 1 - [ 1 to 366 ] */

    time_t  BlinkerApi::time()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("time (millis() - ntpFreshTime): "), (millis() - ntpFreshTime));
            //     BLINKER_LOG_ALL(BLINKER_F("ntpGetTime: "), ntpGetTime);
            //     BLINKER_LOG_ALL((ntpGetTime == 0 || (millis() - ntpFreshTime) >= 600000));

                    time_t now_ntp = ::time(nullptr);

                struct tm timeinfo;

                #if defined(__AVR__)
                    gmtime_r(&now_ntp, &timeinfo);
                #elif defined(ESP32)
                    localtime_r(&now_ntp, &timeinfo);
                #endif

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp;// - (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);

                //     BLINKER_LOG_ALL(BLINKER_F("millis() - ntpFreshTime1:"), (millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();

                // #if defined(ESP32)
                    return _ntpGetTime;
                // #else
                //     return _ntpGetTime - (int)(getTimezone()*3600);
                // #endif
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr);
            //     #else
            //         BLINKER_LOG_ALL("ntpGetTime: ", ntpGetTime);
            //         BLINKER_LOG_ALL("millis() - ntpFreshTime2:", (millis() - ntpFreshTime) / 1000);
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif

            //     // #if defined(ESP32)
            //         return _ntpGetTime;
            //     // #else
            //     //     return _ntpGetTime - (int)(getTimezone()*3600);
            //     // #endif
            // }

        }
        return millis();
    }


    int32_t BlinkerApi::dtime()
    {
        if (_isNTPInit)
        {
            time_t _ntpGetTime;

            // if (ntpGetTime == 0 || (millis() - ntpFreshTime) >= 120000)
            // {
                    time_t now_ntp = ::time(nullptr);

                // if (now_ntp > ntpGetTime)
                // {
                    _ntpGetTime = now_ntp + (int)_timezone*3600;
                // }
                // else
                // {
                //     _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
                // }

                ntpGetTime = _ntpGetTime;
                ntpFreshTime = millis();
            // }
            // else
            // {
            //         1 && 1 && \
            //         _ntpGetTime = ::time(nullptr) + (int)_timezone*3600;
            //     #else
            //         _ntpGetTime = ntpGetTime + ((millis() - ntpFreshTime) / 1000);
            //     #endif
            // }

            struct tm timeinfo;

            #if defined(__AVR__)
                gmtime_r(&_ntpGetTime, &timeinfo);
            #elif defined(ESP32)
                _ntpGetTime = _ntpGetTime - (int)_timezone*3600;

                localtime_r(&_ntpGetTime, &timeinfo);
            #endif

            return timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
        }
        return -1;
    }

    time_t BlinkerApi::startTime()
    {
        if (_isNTPInit) return _deviceStartTime;
        else return 0;
    }

    time_t BlinkerApi::runTime()
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


    bool BlinkerApi::deviceHeartbeat(uint32_t heart_time)
    {
            String data = BLINKER_F("/heartbeat?");
            data += BLINKER_F("deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();
            data += BLINKER_F("&heartbeat=");
            data += STRING_format(heart_time);

            return blinkerServer(BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER, data) != BLINKER_CMD_FALSE;
    }


    void BlinkerApi::loadTimer()
    {
        BLINKER_LOG(BLINKER_F(
            "\n==========================================================="
            "\n================== Blinker Timer loaded! =================="
            "\n     EEPROM address 1536-2431 is used for Blinker Timer!"
            "\n========= PLEASE AVOID USING THESE EEPROM ADDRESS! ========"
            "\n===========================================================\n"));

        checkTimerErase();
        loadCountdown();
        loadLoop();
    }

    void BlinkerApi::deleteTimer()
    {
        blinkerEEPROMBegin(BLINKER_EEP_SIZE);

        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, 0);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, 0);
        EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, 0);

        blinkerEEPROMCommit();
        blinkerEEPROMEnd();
    }

    void BlinkerApi::deleteCountdown()
    {
        blinkerEEPROMBegin(BLINKER_EEP_SIZE);

        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, 0);

        blinkerEEPROMCommit();
        blinkerEEPROMEnd();
    }

    void BlinkerApi::deleteLoop()
    {
        blinkerEEPROMBegin(BLINKER_EEP_SIZE);

        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, 0);

        blinkerEEPROMCommit();
        blinkerEEPROMEnd();
    }

    void BlinkerApi::deleteTiming()
    {
        blinkerEEPROMBegin(BLINKER_EEP_SIZE);

        EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, 0);

        blinkerEEPROMCommit();
        blinkerEEPROMEnd();
    }

    template<typename T>
    bool BlinkerApi::configUpdate(const T& msg)
    {
        String _msg = STRING_format(msg);

            String data = BLINKER_F("{\"token\":\"");
            data += BProto::token();
            data += BLINKER_F("\",\"data\":");
            data += _msg;
            data += BLINKER_F("}");

            if (_msg.length() > 256) return false;

            return blinkerServer(BLINKER_CMD_CONFIG_UPDATE_NUMBER, data) != "false";
    }


    void BlinkerApi::configGet()
    {
            String data = BLINKER_F("/cloud_storage/object?token=");
            data += BProto::token();

            blinkerServer(BLINKER_CMD_CONFIG_GET_NUMBER, data);
    }


    bool BlinkerApi::configDelete()
    {
            String data = BLINKER_F("/delete_userconfig?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            return blinkerServer(BLINKER_CMD_CONFIG_DELETE_NUMBER, data) != "false";
    }

    template<typename T>
    void BlinkerApi::dataStorage(char _name[], const T& msg)
    {
        String _msg = STRING_format(msg);

        int8_t num = checkNum(_name, _Data, data_dataCount);

        time_t _time = time();
        uint8_t _second = second();
        time_t now_time = _time - _second;

        BLINKER_LOG_ALL(BLINKER_F("time: "), _time, BLINKER_F(",second: "), _second);

        BLINKER_LOG_ALL(BLINKER_F("now_time: "), now_time);

        now_time = now_time - now_time % 10;

        BLINKER_LOG_ALL(BLINKER_F("dataStorage num: "), num, BLINKER_F(" ,"), now_time);
        BLINKER_LOG_ALL(BLINKER_F("dataStorage count: "), data_dataCount);

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
            _Data[data_dataCount]->saveData(data_msg, now_time);
            data_dataCount++;
            // {
            //     dataUpdate();
            // }

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _msg, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_dataCount: "), data_dataCount);
        }
        else {
            // _Data[num]->saveData(time(), _msg);
            // if
            _Data[num]->saveData(data_msg, now_time);
            // {
            //     dataUpdate();
            // }

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _msg, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_dataCount: "), data_dataCount);
        }
    }

    template<typename T>
    void BlinkerApi::sendRtData(char _name[], const T& msg)
    {
        // String _msg = STRING_format(msg);



        // ============================================



        String _data_ = "{\"date\":";
        _data_ += String(time());
        _data_ += ",\"val\":";
        _data_ += String(msg);
        _data_ += "}";

        BProto::checkState(false);
        printObject(_name, _data_);
        // BProto::printNow();


        // ============================================




        // int8_t num = checkNum(_name, _RTData, data_rtDataCount);

        // time_t _time = time();

        // if (1600000000 > time()) return;
        // // uint8_t _second = second();
        // time_t now_time = _time;// - _second;

        // // BLINKER_LOG_ALL(BLINKER_F("time: "), _time, BLINKER_F(",second: "), _second);

        // BLINKER_LOG_ALL(BLINKER_F("now_time: "), now_time);

        // // now_time = now_time - now_time % 10;

        // BLINKER_LOG_ALL(BLINKER_F("dataStorage num: "), num, BLINKER_F(" ,"), now_time);
        // BLINKER_LOG_ALL(BLINKER_F("dataStorage count: "), data_rtDataCount);

        // // String data_msg = String(msg);

        // // if (data_msg.length() > 10) return;

        // if( num == BLINKER_OBJECT_NOT_AVAIL )
        // {
        //     if (data_rtDataCount >= BLINKER_MAX_RTDATA_SIZE)
        //     {
        //         return;
        //     }
        //     _RTData[data_rtDataCount] = new BlinkerRTData();
        //     _RTData[data_rtDataCount]->name(_name);
        //     // _RTData[data_rtDataCount]->saveData(time(), _msg);
        //     // if
        //     _RTData[data_rtDataCount]->saveData(msg, now_time);
        //     data_rtDataCount++;
        //     // {
        //     //     dataUpdate();
        //     // }

        //     BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), msg, BLINKER_F(" time: "), now_time);
        //     BLINKER_LOG_ALL(BLINKER_F("data_rtDataCount: "), data_rtDataCount);
        // }
        // else {
        //     // _RTData[num]->saveData(time(), _msg);
        //     // if
        //     _RTData[num]->saveData(msg, now_time);
        //     // {
        //     //     dataUpdate();
        //     // }

        //     BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), msg, BLINKER_F(" time: "), now_time);
        //     BLINKER_LOG_ALL(BLINKER_F("data_rtDataCount: "), data_rtDataCount);
        // }


    }

    void BlinkerApi::printRtData()
    {
        // String data = "{\"rt\":{";

        // for (uint8_t _num = 0; _num < data_rtDataCount; _num++) {
        //     data += BLINKER_F("\"");
        //     data += _RTData[_num]->getName();
        //     data += BLINKER_F("\":");
        //     data += _RTData[_num]->getData();
        //     if (_num < data_rtDataCount - 1) {
        //         data += BLINKER_F(",");
        //     }

        //     BLINKER_LOG_ALL(BLINKER_F("num: "), _num, \
        //             BLINKER_F(" name: "), _RTData[_num]->getName());

        //     BLINKER_LOG_FreeHeap_ALL();
        // }

        // data += BLINKER_F("}}");

        // printObject(_RTDataKey, data);

        _RTTicker.once(_RTTime, _RTDataFunc);

        data_rtTimes++;

        if (data_rtTimes <= 9) BProto::printNow();
    }


    void BlinkerApi::timeSlotData(char _name[], int32_t _data)
    {
        uint32_t now_time = time();

        if (data_timeSlotDataCount == BLINKER_MAX_BLINKER_DATA_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("BLINKER MAX DATA STORAGE LIMIT!"));
            return;
        }

        BLINKER_LOG_ALL(BLINKER_F("timeSlotData save"));

        if (millis() - time_timeSlotData < 1000)
        {
            if (data_timeSlotDataCount != 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount != 0"));
                _TimeSlotData[data_timeSlotDataCount - 1]->saveData(_name, _data, now_time);
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount == 0"));
                _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
                _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
                data_timeSlotDataCount++;
            }

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
        }
        else
        {
            time_timeSlotData = millis();

            _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
            _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
            data_timeSlotDataCount++;

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
        }
    }


    void BlinkerApi::timeSlotData(char _name[], uint32_t _data)
    {
        uint32_t now_time = time();

        if (data_timeSlotDataCount == BLINKER_MAX_BLINKER_DATA_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("BLINKER MAX DATA STORAGE LIMIT!"));
            return;
        }

        BLINKER_LOG_ALL(BLINKER_F("timeSlotData save"));

        if (millis() - time_timeSlotData < 1000)
        {
            if (data_timeSlotDataCount != 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount != 0"));
                _TimeSlotData[data_timeSlotDataCount - 1]->saveData(_name, _data, now_time);
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount == 0"));
                _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
                _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
                data_timeSlotDataCount++;
            }

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
        }
        else
        {
            time_timeSlotData = millis();

            _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
            _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
            data_timeSlotDataCount++;

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
        }
    }


    void BlinkerApi::timeSlotData(char _name[], float _data)
    {
        uint32_t now_time = time();

        if (data_timeSlotDataCount == BLINKER_MAX_BLINKER_DATA_SIZE)
        {
            BLINKER_ERR_LOG(BLINKER_F("BLINKER MAX DATA STORAGE LIMIT!"));
            return;
        }

        BLINKER_LOG_ALL(BLINKER_F("timeSlotData save"));

        if (millis() - time_timeSlotData < 1000)
        {
            if (data_timeSlotDataCount != 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount != 0"));
                _TimeSlotData[data_timeSlotDataCount - 1]->saveData(_name, _data, now_time);
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount == 0"));
                _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
                _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
                data_timeSlotDataCount++;
            }

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
        }
        else
        {
            time_timeSlotData = millis();

            _TimeSlotData[data_timeSlotDataCount] = new BlinkerTimeSlotData();
            _TimeSlotData[data_timeSlotDataCount]->saveData(_name, _data, now_time);
            data_timeSlotDataCount++;

            BLINKER_LOG_ALL(_name, BLINKER_F(" save: "), _data, BLINKER_F(" time: "), now_time);
            BLINKER_LOG_ALL(BLINKER_F("data_timeSlotDataCount: "), data_timeSlotDataCount);
        }
    }


    void BlinkerApi::textData(const String & msg)
    {
        String data = BLINKER_F("{");

        #if defined(BLINKER_PROTOCOL_HTTP_SERVER)
            data += BLINKER_F("\"device\":\"");
            data += BProto::deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += BProto::authKey();
            data += BLINKER_F("\"");
        #else
            data += BLINKER_F("\"toStorage\":\"tt\"");
        #endif

        data += BLINKER_F(",\"data\":\"");
        data += msg;
        data += BLINKER_F("\"}");

        BLINKER_LOG_ALL(BLINKER_F("textData: "), data);

        #if !defined(BLINKER_PROTOCOL_HTTP_SERVER)
            BProto::toServer((char *)data.c_str());
        #endif

        #if defined(BLINKER_PROTOCOL_HTTP_SERVER)
            if (blinkerServer(BLINKER_CMD_TEXT_DATA_NUMBER, data) == "false")
            {
                // return false;
            }
            else
            {
                // return true;
            }
        #else
            // return true;
        #endif
    }


    void BlinkerApi::jsonData(const String & msg)
    {
        JsonDocument jsonBuffer;
        DeserializationError error = deserializeJson(jsonBuffer, msg);
        JsonObject root = jsonBuffer.as<JsonObject>();

        // if (!root.success())
        if (error)
        {
            BLINKER_ERR_LOG("Print data is not Json! ", msg);
            return;
        }

        String data = BLINKER_F("{");

        #if defined(BLINKER_PROTOCOL_HTTP_SERVER)
            data += BLINKER_F("\"device\":\"");
            data += BProto::deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += BProto::authKey();
            data += BLINKER_F("\"");
        #else
            data += BLINKER_F("\"toStorage\":\"ot\"");
        #endif

        data += BLINKER_F(",\"data\":");
        data += msg;
        data += BLINKER_F("}");

        BLINKER_LOG_ALL(BLINKER_F("jsonData: "), data);

        #if !defined(BLINKER_PROTOCOL_HTTP_SERVER)
            BProto::toServer((char *)data.c_str());
        #endif

        #if defined(BLINKER_PROTOCOL_HTTP_SERVER)
            if (blinkerServer(BLINKER_CMD_JSON_DATA_NUMBER, data) == "false")
            {
                // return false;
            }
            else
            {
                // return true;
            }
        #else
            // return true;
        #endif
    }


    void BlinkerApi::jsonDataGet()
    {
        String data = BLINKER_F("/cloud_storage/object?token=");
            data += BProto::token();

        blinkerServer(BLINKER_CMD_JSON_DATA_GET_NUMBER, data);
    }


    bool BlinkerApi::dataUpdate()
    {
        BLINKER_LOG_ALL(BLINKER_F("dataUpdate: "), data_timeSlotDataCount);

        if (data_timeSlotDataCount > 0)
        {
            String data = BLINKER_F("{");

            #if defined(BLINKER_PROTOCOL_HTTP_SERVER)
                data += BLINKER_F("\"device\":\"");
                data += BProto::deviceName();
                data += BLINKER_F("\",\"key\":\"");
                data += BProto::authKey();
                data += BLINKER_F("\"");
            #else
                data += BLINKER_F("\"device\":\"");
                data += BProto::deviceName();
                data += BLINKER_F("\",\"toStorage\":\"ts\"");
            #endif

            data += BLINKER_F(",\"data\":[");

            for (uint8_t _num = 0; _num < data_timeSlotDataCount; _num++) {
                data += _TimeSlotData[_num]->getData();
                if (_num < data_timeSlotDataCount - 1) {
                    data += BLINKER_F(",");
                }
            }

            data += BLINKER_F("]}");

            BLINKER_LOG_ALL(BLINKER_F("dataUpdate: "), data);

            #if !defined(BLINKER_PROTOCOL_HTTP_SERVER)
                BProto::toServer((char *)data.c_str());
            #endif

            for (uint8_t _num = 0; _num < data_timeSlotDataCount; _num++)
            {
                // _TimeSlotData[_num]->flush();
                free(_TimeSlotData[_num]);
            }

            data_timeSlotDataCount = 0;

            #if defined(BLINKER_PROTOCOL_HTTP_SERVER)
                if (blinkerServer(BLINKER_CMD_TIME_SLOT_DATA_NUMBER, data) == "false")
                {
                    return false;
                }
                else
                {
                    return true;
                }
            #else
                return true;
            #endif
        }

        if (!data_dataCount) {
            // BLINKER_ERR_LOG(BLINKER_F("none data storaged!"));
            return false;
        }

        //     String data = BLINKER_F("deviceName=");
        //     data += BProto::deviceName();
        //     data += BLINKER_F("&key=");
        //     data += BProto::authKey();
        //     data += BLINKER_F("&data=");

        //     String _data_ = "{";

        //     for (uint8_t _num = 0; _num < data_dataCount; _num++) {
        //         _data_ += BLINKER_F("\"");
        //         _data_ += _Data[_num]->getName();
        //         _data_ += BLINKER_F("\":");
        //         _data_ += _Data[_num]->getData();
        //         if (_num < data_dataCount - 1) {
        //             _data_ += BLINKER_F(",");
        //         }

        //         BLINKER_LOG_ALL(BLINKER_F("num: "), _num, \
        //                 BLINKER_F(" name: "), _Data[_num]->getName());

        //         BLINKER_LOG_FreeHeap_ALL();
        //     }

        //     _data_ += "}";

        //     int encodedLen = base64_enc_len(_data_.length());
        //     char encoded[encodedLen];
        //     base64_encode(encoded, _data_.c_str(), _data_.length());
        //     BLINKER_LOG_ALL(BLINKER_F("encoded: "), encoded);

        //     data += encoded;

        //     // data += BLINKER_F("");
        // #else

            String data = BLINKER_F("{\"deviceName\":\"");
            data += BProto::deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += BProto::authKey();
            data += BLINKER_F("\",\"data\":{");
            // String _sdata;

            BLINKER_LOG_FreeHeap_ALL();

            // uint32_t now_time = time() - second();

            for (uint8_t _num = 0; _num < data_dataCount; _num++) {
                data += BLINKER_F("\"");
                data += _Data[_num]->getName();
                data += BLINKER_F("\":");
                data += _Data[_num]->getData();
                if (_num < data_dataCount - 1) {
                    data += BLINKER_F(",");
                }

                BLINKER_LOG_ALL(BLINKER_F("num: "), _num, \
                        BLINKER_F(" name: "), _Data[_num]->getName());

                BLINKER_LOG_FreeHeap_ALL();
            }

            data += BLINKER_F("}}");
        // #endif

        BLINKER_LOG_ALL(BLINKER_F("dataUpdate: "), data);

        BLINKER_LOG_FreeHeap_ALL();

        // return true;
                        //  + \ _msg +
                        // "\"}}";
        if (blinkerServer(BLINKER_CMD_DATA_STORAGE_NUMBER, data) == "false")
        {
            return false;
        }
        else
        {
            for (uint8_t _num = 0; _num < data_dataCount; _num++)
            {
                _Data[_num]->flush();
            }

            return true;
        }
    }


    void BlinkerApi::dataGet()
    {
            String data = BLINKER_F("/pull_cloudStorage?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            blinkerServer(BLINKER_CMD_DATA_GET_NUMBER, data);
    }


    void BlinkerApi::dataGet(const String & _type)
    {
            String data = BLINKER_F("/pull_cloudStorage?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();
            data += BLINKER_F("&dataType=");
            data += _type;

            blinkerServer(BLINKER_CMD_DATA_GET_NUMBER, data);
    }


    void BlinkerApi::dataGet(const String & _type, const String & _date)
    {
            String data = BLINKER_F("/pull_cloudStorage?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();
            data += BLINKER_F("&dataType=");
            data += _type;
            data += BLINKER_F("&date=");
            data += _date;

            blinkerServer(BLINKER_CMD_DATA_GET_NUMBER, data);
    }


    bool BlinkerApi::dataDelete()
    {
            String data = BLINKER_F("/delete_cloudStorage?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            return blinkerServer(BLINKER_CMD_DATA_DELETE_NUMBER, data) != "false";
    }


    bool BlinkerApi::dataDelete(const String & _type)
    {
            String data = BLINKER_F("/delete_cloudStorage?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();
            data += BLINKER_F("&dataType=");
            data += _type;

            return blinkerServer(BLINKER_CMD_DATA_DELETE_NUMBER, data) != "false";
    }


    bool BlinkerApi::event(const String & _key, String _value)
    {
            String data = BLINKER_F("{\"deviceName\":\"");
            data += BProto::deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += BProto::authKey();
            data += BLINKER_F("\",\"eKey\":\"");
            data += _key;
            data += BLINKER_F("\",\"date\":\"");
            data += STRING_format(time());
            data += BLINKER_F("\",\"value\":\"");
            data += _value;
            data += BLINKER_F("\"}");

            return blinkerServer(BLINKER_CMD_EVENT_DATA_NUMBER, data) != "false";
    }


    bool BlinkerApi::autoPull()
    {
            String data = BLINKER_F("/auto/pull?deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            String payload = blinkerServer(BLINKER_CMD_AUTO_PULL_NUMBER, data);

            if (payload == BLINKER_CMD_FALSE)
            {
                return false;
            }
            else
            {
                // DynamicJsonBuffer jsonBuffer;
                // JsonObject& autoJson = jsonBuffer.parseObject(payload);
                JsonDocument jsonBuffer;
                deserializeJson(jsonBuffer, payload);
                JsonObject autoJson = jsonBuffer.as<JsonObject>();

                return autoManager(autoJson);
            }
    }


    // void BlinkerApi::autoInput(const String & key, const String & state)
    // {
    //     if (!_isNTPInit) return;

    //     int32_t nowTime = dtime();

    //     for (uint8_t _num = 0; _num < _aCount; _num++)
    //     {
    //         _AUTO[_num]->run(key, state, nowTime);
    //     }
    // }


    void BlinkerApi::autoInput(const String & key, float data)
    {
        if (!_isNTPInit) return;

        int32_t nowTime = dtime()/60;

        for (uint8_t _num = 0; _num < _aCount; _num++)
        {
            _AUTO[_num]->run(key, data, nowTime);
        }
    }


    void BlinkerApi::autoRun()
    {
        for (uint8_t _num = 0; _num < _aCount; _num++)
        {
            if (_AUTO[_num]->isTrigged())
            {
                if (autoTrigged(_AUTO[_num]->id()))
                {
                    run();

                    BLINKER_LOG_ALL(BLINKER_F("trigged sucessed"));

                    _AUTO[_num]->fresh();
                }
                else
                {
                    BLINKER_LOG_ALL(BLINKER_F("trigged failed"));
                }

                run();
            }
        }
    }

    String BlinkerApi::freshSharers()
    {
            String data = BLINKER_F("/share/device?");
            data += BLINKER_F("deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            return blinkerServer(BLINKER_CMD_FRESH_SHARERS_NUMBER, data);
    }



    #if defined(BLINKER_LOWPOWER)
        int32_t BlinkerApi::comFreqGet()
        {
            String data = BLINKER_F("/lowpower?");
            data += BLINKER_F("deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            int32_t _freq = blinkerServer(BLINKER_CMD_LOWPOWER_FREQ_GET_NUM, data).toInt();

            if (_freq) return _freq;
            else return -1;
        }

        bool BlinkerApi::comFreqUpdate()
        {
            String data = BLINKER_F("/lowpower/modify?");
            data += BLINKER_F("deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();
            data += BLINKER_F("&freq=");
            data += STRING_format(_LowPowerFreq);

            return blinkerServer(BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER, data) != "false";
        }

        String BlinkerApi::comDataGet()
        {
            String data = BLINKER_F("/lowpower/data?");
            data += BLINKER_F("deviceName=");
            data += BProto::deviceName();
            data += BLINKER_F("&key=");
            data += BProto::authKey();

            return blinkerServer(BLINKER_CMD_LOWPOWER_DATA_GET_NUM, data);
        }

        bool BlinkerApi::comDateUpdate()
        {
            String data = BLINKER_F("{\"deviceName\":\"");
            data += BProto::deviceName();
            data += BLINKER_F("\",\"key\":\"");
            data += BProto::authKey();
            data += BLINKER_F("\",\"data\":");
            data += BProto::_sendBuf;
            data += BLINKER_F("}");

            return blinkerServer(BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER, data) != "false";
        }
    #endif


#endif

void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_string_arg_t _func)
{
    int8_t num = checkNum(_name, _Widgets_str, _wCount_str);
    if(num >= 0 ) _Widgets_str[num]->setFunc(_func);
}

// #if defined(BLINKER_BLE)
    void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_joy_arg_t _func)
    {
        int8_t num = checkNum(_name, _Widgets_joy, _wCount_joy);
        if(num >= 0 ) _Widgets_joy[num]->setFunc(_func);
    }
// #endif

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

void BlinkerApi::freshAttachWidget(char _name[], blinker_callback_with_table_arg_t _func, blinker_callback_t _func2)
{
    int8_t num = checkNum(_name, _Widgets_tab, _wCount_tab);
    if(num >= 0 ) _Widgets_tab[num]->setFunc(_func, _func2);
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

// #if defined(BLINKER_BLE)
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
// #endif

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

uint8_t BlinkerApi::attachWidget(char _name[], blinker_callback_with_table_arg_t _func,
        blinker_callback_t _func2)
{
    int8_t num = checkNum(_name, _Widgets_tab, _wCount_tab);
    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_tab < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_tab[_wCount_tab] = new BlinkerWidgets_table(_name, _func, _func2);
            _wCount_tab++;

            BLINKER_LOG_ALL(BLINKER_F("new widgets: "), _name, \
                        BLINKER_F(" _wCount_tab: "), _wCount_tab);

            return _wCount_tab;
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
    else return "";
}

// #if defined(BLINKER_BLE)
    char * BlinkerApi::widgetName_joy(uint8_t num)
    {
        if (num) return _Widgets_joy[num - 1]->getName();
        else return "";
    }
// #endif

char * BlinkerApi::widgetName_rgb(uint8_t num)
{
    if (num) return _Widgets_rgb[num - 1]->getName();
    else return "";
}

char * BlinkerApi::widgetName_int(uint8_t num)
{
    if (num) return _Widgets_int[num - 1]->getName();
    else return "";
}

char * BlinkerApi::widgetName_tab(uint8_t num)
{
    if (num) return _Widgets_tab[num - 1]->getName();
    else return "";
}

#if defined(BLINKER_ARDUINOJSON)
    int16_t BlinkerApi::ahrs(b_ahrsattitude_t attitude, const JsonObject& data)
    {
        if (data.containsKey(BLINKER_CMD_AHRS)) {
            int16_t aAttiValue = data[BLINKER_CMD_AHRS][attitude];
            ahrsValue[Yaw] = data[BLINKER_CMD_AHRS][Yaw];
            ahrsValue[Roll] = data[BLINKER_CMD_AHRS][Roll];
            ahrsValue[Pitch] = data[BLINKER_CMD_AHRS][Pitch];
            BLINKER_LOG_ALL(BLINKER_F("ahrs isParsed"));
            _fresh = true;

            return aAttiValue;
        }
        else {
            return ahrsValue[attitude];
        }
    }

    void BlinkerApi::heartBeat(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_GET];

        // if (state.length())
        if (data.containsKey(BLINKER_CMD_GET))
        {
            if (state == BLINKER_CMD_STATE)
            {
                #if defined(BLINKER_BLE) || defined(BLINKER_WIFI)
                    print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                #else
                    print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                #endif

                #if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                    String _timer = taskCount ? "1":"0";
                    _timer += _cdState ? "1":"0";
                    _timer += _lpState ? "1":"0";

                    BLINKER_LOG_ALL(BLINKER_F("timer codes: "), _timer);

                    print(BLINKER_CMD_TIMER, _timer);
                    // static_cast<Proto*>(this)->printJson(timerSetting());

                    #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                        print(BLINKER_CMD_VERSION, BLINKER_VERSION);
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
                if (!BProto::printNow())
                {
                    #if defined(BLINKER_BLE)
                        print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                    #else
                        print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                    #endif

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
                }
                BLINKER_LOG_ALL(BLINKER_F("heartBeat isParsed 1"));
                _fresh = true;

            }
        }
    }

    void BlinkerApi::getVersion(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_GET];

        // if (state.length())
        if (data.containsKey(BLINKER_CMD_GET))
        {
            if (state == BLINKER_CMD_VERSION)
            {
                print(BLINKER_CMD_VERSION, BLINKER_VERSION);
                BLINKER_LOG_ALL(BLINKER_F("getVersion isParsed"));
                _fresh = true;
            }
        }
    }

    void BlinkerApi::setSwitch(const JsonObject& data)
    {
        String state = data[BLINKER_CMD_BUILTIN_SWITCH];

        // if (state.length())
        if (data.containsKey(BLINKER_CMD_BUILTIN_SWITCH))
        {
            // if (_BUILTIN_SWITCH)
            // {
            //     blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH->getFunc();

            //     if (sFunc) sFunc(state);
            // }
            blinker_callback_with_string_arg_t sFunc = _BUILTIN_SWITCH.getFunc();

            if (sFunc) sFunc(state);
            BLINKER_LOG_ALL(BLINKER_F("setSwitch isParsed"));
            _fresh = true;
        }
    }

    void BlinkerApi::strWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName))
        {
            String state = data[_wName];
            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse isParsed"));
            _fresh = true;

            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse: "), _wName);

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();

            if (nbFunc) nbFunc(state);
        }
    }

    // #if defined(BLINKER_BLE)
        void BlinkerApi::joyWidgetsParse(char _wName[], const JsonObject& data)
        {
            int8_t num = checkNum(_wName, _Widgets_joy, _wCount_joy);

            if (num == BLINKER_OBJECT_NOT_AVAIL) return;

            if (data.containsKey(_wName))
            {
                int16_t jxAxisValue = data[_wName][BLINKER_J_Xaxis];
                uint8_t jyAxisValue = data[_wName][BLINKER_J_Yaxis];
                BLINKER_LOG_ALL(BLINKER_F("joyWidgetsParse isParsed"));
                _fresh = true;

                blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();
                if (wFunc) wFunc(jxAxisValue, jyAxisValue);
            }
        }
    // #endif

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
            BLINKER_LOG_ALL(BLINKER_F("rgbWidgetsParse isParsed"));
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
            BLINKER_LOG_ALL(BLINKER_F("intWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();
            if (wFunc) {
                wFunc(_number);
            }
        }
    }

    void BlinkerApi::tabWidgetsParse(char _wName[], const JsonObject& data)
    {
        int8_t num = checkNum(_wName, _Widgets_tab, _wCount_tab);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        if (data.containsKey(_wName)) {
            String _setData = data[_wName];

            uint8_t _number = 0;

            blinker_callback_with_table_arg_t wFunc = _Widgets_tab[num]->getFunc();

            for (uint8_t num = 0; num < 5; num++)
            {
                // BLINKER_LOG_ALL(BLINKER_F("num: "), _setData.substring(num, num + 1));

                if (strcmp(_setData.substring(num, num + 1).c_str(), "1") == 0)
                {
                    if (wFunc) {
                        switch (num)
                        {
                            case 0:
                                wFunc(BLINKER_CMD_TAB_0);
                                break;
                            case 1:
                                wFunc(BLINKER_CMD_TAB_1);
                                break;
                            case 2:
                                wFunc(BLINKER_CMD_TAB_2);
                                break;
                            case 3:
                                wFunc(BLINKER_CMD_TAB_3);
                                break;
                            case 4:
                                wFunc(BLINKER_CMD_TAB_4);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }


            // if (_setData == "10000") _number = BLINKER_CMD_TAB_0;
            // else if (_setData == "01000") _number = BLINKER_CMD_TAB_1;
            // else if (_setData == "00100") _number = BLINKER_CMD_TAB_2;
            // else if (_setData == "00010") _number = BLINKER_CMD_TAB_3;
            // else if (_setData == "00001") _number = BLINKER_CMD_TAB_4;
            BLINKER_LOG_ALL(BLINKER_F("tabWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_t wFunc2 = _Widgets_tab[num]->getFunc2();
            if (wFunc2) {
                wFunc2();
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
        // #if defined(BLINKER_BLE)
            for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
                joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), data);
            }
        // #endif
        for (uint8_t wNum_tab = 0; wNum_tab < _wCount_tab; wNum_tab++) {
            tabWidgetsParse(_Widgets_tab[wNum_tab]->getName(), data);
        }
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
            BLINKER_LOG_ALL(BLINKER_F("ahrs isParsed"));
            _fresh = true;

            return aAttiValue;
        }
        else {
            return ahrsValue[attitude];
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
            if (!BProto::printNow())
            {
                #if defined(BLINKER_BLE)
                    print(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED);
                #else
                    print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE);
                #endif

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
            }
            BLINKER_LOG_ALL(BLINKER_F("heartBeat isParsed"));
            _fresh = true;

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
            BLINKER_LOG_ALL(BLINKER_F("getVersion isParsed"));
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
            BLINKER_LOG_ALL(BLINKER_F("setSwitch isParsed"));
            _fresh = true;
        }
    }

    void BlinkerApi::strWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_str, _wCount_str);

        // BLINKER_LOG_ALL("====checkNum: ", num, " ====");
        // BLINKER_LOG_ALL("====_data: ", _data, " ====");

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        String state;

        if (STRING_find_string_value(_data, state, _wName))
        {
            BLINKER_LOG_ALL(BLINKER_F("state: "), state);
            BLINKER_LOG_ALL(BLINKER_F("strWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num]->getFunc();
            if (nbFunc) nbFunc(state);
        }
    }

    // #if defined(BLINKER_BLE)
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
                BLINKER_LOG_ALL(BLINKER_F("joyWidgetsParse isParsed"));
                _fresh = true;

                blinker_callback_with_joy_arg_t wFunc = _Widgets_joy[num]->getFunc();

                if (wFunc) wFunc(jxAxisValue, jyAxisValue);
            }
        }
    // #endif

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
            BLINKER_LOG_ALL(BLINKER_F("rgbWidgetsParse isParsed"));
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
            BLINKER_LOG_ALL(BLINKER_F("intWidgetsParse isParsed"));
            _fresh = true;

            blinker_callback_with_int32_arg_t wFunc = _Widgets_int[num]->getFunc();

            if (wFunc) wFunc(_number);
        }
    }

    void BlinkerApi::tabWidgetsParse(char _wName[], char _data[])
    {
        int8_t num = checkNum(_wName, _Widgets_tab, _wCount_tab);

        if (num == BLINKER_OBJECT_NOT_AVAIL) return;

        String _setData;

        if (STRING_find_string_value(_data, _setData, _wName))
        {
            BLINKER_LOG_ALL(BLINKER_F("_setData: "), _setData);
            BLINKER_LOG_ALL(BLINKER_F("tabWidgetsParse isParsed"));
            _fresh = true;

            // uint8_t _number = 0;

            // if (_setData == "10000") _number = BLINKER_CMD_TAB_0;
            // else if (_setData == "01000") _number = BLINKER_CMD_TAB_1;
            // else if (_setData == "00100") _number = BLINKER_CMD_TAB_2;
            // else if (_setData == "00010") _number = BLINKER_CMD_TAB_3;
            // else if (_setData == "00001") _number = BLINKER_CMD_TAB_4;

            // blinker_callback_with_table_arg_t wFunc = _Widgets_tab[num]->getFunc();
            // if (wFunc) {
            //     wFunc(_number);
            // }

            blinker_callback_with_table_arg_t wFunc = _Widgets_tab[num]->getFunc();

            for (uint8_t num = 0; num < 5; num++)
            {
                // BLINKER_LOG_ALL(BLINKER_F("num: "), _setData.substring(num, num + 1));

                if (strcmp(_setData.substring(num, num + 1).c_str(), "1") == 0)
                {
                    if (wFunc) {
                        switch (num)
                        {
                            case 0:
                                wFunc(BLINKER_CMD_TAB_0);
                                break;
                            case 1:
                                wFunc(BLINKER_CMD_TAB_1);
                                break;
                            case 2:
                                wFunc(BLINKER_CMD_TAB_2);
                                break;
                            case 3:
                                wFunc(BLINKER_CMD_TAB_3);
                                break;
                            case 4:
                                wFunc(BLINKER_CMD_TAB_4);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }

            blinker_callback_t wFunc2 = _Widgets_tab[num]->getFunc2();
            if (wFunc2) {
                wFunc2();
            }
        }

        // int _number = STRING_find_numberic_value(_data, _wName);

        // if (_number != FIND_KEY_VALUE_FAILED)
        // {
        //     _fresh = true;

        //     blinker_callback_with_table_arg_t wFunc = _Widgets_tab[num]->getFunc();

        //     if (wFunc) wFunc(_number);
        // }
    }

    void BlinkerApi::json_parse(char _data[])
    {
        setSwitch(_data);

        BLINKER_LOG_ALL(BLINKER_F("====_wCount_str: "), _wCount_str, BLINKER_F(" ===="));

        for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
            strWidgetsParse(_Widgets_str[wNum]->getName(), _data);
        }
        for (uint8_t wNum_int = 0; wNum_int < _wCount_int; wNum_int++) {
            intWidgetsParse(_Widgets_int[wNum_int]->getName(), _data);
        }
        for (uint8_t wNum_rgb = 0; wNum_rgb < _wCount_rgb; wNum_rgb++) {
            rgbWidgetsParse(_Widgets_rgb[wNum_rgb]->getName(), _data);
        }
        // #if defined(BLINKER_BLE)
            for (uint8_t wNum_joy = 0; wNum_joy < _wCount_joy; wNum_joy++) {
                joyWidgetsParse(_Widgets_joy[wNum_joy]->getName(), _data);
            }
        // #endif
        for (uint8_t wNum_tab = 0; wNum_tab < _wCount_tab; wNum_tab++) {
            tabWidgetsParse(_Widgets_tab[wNum_tab]->getName(), _data);
        }
    }
#endif

#if defined(BLINKER_WIFI) || defined(BLINKER_MQTT) || defined(BLINKER_HTTP)


        void BlinkerApi::beginAuto()
        {
            BLINKER_LOG(BLINKER_F("======================================================="));
            BLINKER_LOG(BLINKER_F("=========== Blinker Auto Control mode init! ==========="));
            BLINKER_LOG(BLINKER_F("     EEPROM address 0-1279 is used for Auto Control!"));
            BLINKER_LOG(BLINKER_F("======= PLEASE AVOID USING THESE EEPROM ADDRESS! ======"));
            BLINKER_LOG(BLINKER_F("======================================================="));

            // BLINKER_LOG(BLINKER_F("Already used: "), BLINKER_ONE_AUTO_DATA_SIZE);

            _isAuto = true;
            // deserialization();
            // autoStart();

            autoInit(); //TODO
        }

        bool BlinkerApi::autoTrigged(uint32_t _id)
        {
            BLINKER_LOG_ALL(BLINKER_F("autoTrigged id: "), _id);

            return BProto::autoPrint(_id);
        }


        bool BlinkerApi::ntpInit()
        {
            if (!_isNTPInit)
            {
                freshNTP();

                if ((millis() - _ntpStart) > BLINKER_NTP_TIMEOUT)
                {
                    _ntpStart = millis();
                }
                else {
                    return false;
                }

                ntpConfig();
                // String ntp1 = BLINKER_F("ntp1.aliyun.com");
                // String ntp2 = BLINKER_F("210.72.145.44");
                // String ntp3 = BLINKER_F("time.pool.aliyun.com");

                // configTime((long)(_timezone * 3600), 0,
                //     ntp1.c_str(), ntp2.c_str(), ntp3.c_str());
                // configTime((long)(_timezone * 3600), 0, "ntp1.aliyun.com", "210.72.145.44", "time.pool.aliyun.com");

                time_t now_ntp = ::time(nullptr);

                float _com_timezone = abs(getTimezone());
                if (_com_timezone < 1.0) _com_timezone = 1.0;

                if (now_ntp < _com_timezone * 3600 * 12)
                {
                    ntpConfig();
                    // configTime((long)(_timezone * 3600), 0,
                    //     ntp1.c_str(), ntp2.c_str(), ntp3.c_str());
                    // configTime((long)(_timezone * 3600), 0, "ntp1.aliyun.com", "210.72.145.44", "time.pool.aliyun.com");
                    now_ntp = ::time(nullptr);
                    if (now_ntp < _com_timezone * 3600 * 12)
                    {
                        ::delay(50);
                        now_ntp = ::time(nullptr);
                        return false;
                    }
                }

                struct tm timeinfo;

                #if defined(ESP32)
                    localtime_r(&now_ntp, &timeinfo);
                #endif

                BLINKER_LOG_ALL(BLINKER_F("Current time: "), asctime(&timeinfo));
                #if defined(ESP32)
                    BLINKER_LOG_ALL(BLINKER_F("NTP time: "), now_ntp);
                #endif

                _isNTPInit = true;

                _deviceStartTime = time() - millis()/1000;

                return true;
            }
            return true;
        }


        void BlinkerApi:: freshNTP()
        {
            if (_isNTPInit)
            {
                time_t now_ntp = ::time(nullptr);
                struct tm timeinfo;
                #if defined(ESP32)
                    localtime_r(&now_ntp, &timeinfo);
                #endif
            }
        }


        void BlinkerApi::ntpConfig()
        {
            // String ntp1 = BLINKER_F("ntp1.aliyun.com");
            // String ntp2 = BLINKER_F("210.72.145.44");
            // String ntp3 = BLINKER_F("time.pool.aliyun.com");

            // configTime((long)(_timezone * 3600), 0,
            //         ntp1.c_str(), ntp2.c_str(), ntp3.c_str());

            blinkerWiFiConfigTime((long)(getTimezone() * 3600));
        }


        void BlinkerApi::saveCountDown(uint32_t _data, char _action[])
        {
            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _data);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _action);
            blinkerEEPROMCommit();
            blinkerEEPROMEnd();
        }


        void BlinkerApi::saveLoop(uint32_t _data, char _action1[], char _action2[])
        {
            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _data);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _action1);
            EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _action2);
            blinkerEEPROMCommit();
            blinkerEEPROMEnd();
        }


        void BlinkerApi::loadCountdown()
        {
            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction);
            blinkerEEPROMCommit();
            blinkerEEPROMEnd();

            _cdState    = _cdData >> 31;
            _cdRunState = _cdData >> 30 & 0x0001;
            _cdTime1    = _cdData >> 12 & 0x0FFF;
            _cdTime2    = _cdData       & 0x0FFF;

            if (_cdTime1 == 0)
            {
                _cdState = 0;
                _cdRunState = 0;
            }

            BLINKER_LOG_ALL(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
            BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
            BLINKER_LOG_ALL(BLINKER_F("_totalTime: "), _cdTime1);
            BLINKER_LOG_ALL(BLINKER_F("_runTime: "), _cdTime2);
            BLINKER_LOG_ALL(BLINKER_F("_action: "), _cdAction);

            if (_cdState && _cdRunState)
            {
                uint32_t _cdTime1_;

                if (_cdTime1 * 60 > BLINKER_ONE_HOUR_TIME) _cdTime1_ = BLINKER_ONE_HOUR_TIME;
                else _cdTime1_ = _cdTime1 * 60;

                cdTicker.once(_cdTime1_, _cd_callback);

                _cdStart = millis();

                BLINKER_LOG_ALL(BLINKER_F("countdown start!"));
            }
        }


        void BlinkerApi::loadLoop()
        {
            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_TRI, _lpTrigged_times);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction1);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction2);
            blinkerEEPROMCommit();
            blinkerEEPROMEnd();

            _lpState    = _lpData >> 31;
            _lpRunState = _lpData >> 30 & 0x0001;
            _lpTimes    = _lpData >> 22 & 0x007F;
            _lpTime1    = _lpData >> 11 & 0x07FF;
            _lpTime2    = _lpData       & 0x07FF;

            BLINKER_LOG_ALL(BLINKER_F("loop state: "), _lpState ? "true" : "false");
            BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);
            BLINKER_LOG_ALL(BLINKER_F("_times: "), _lpTimes);
            BLINKER_LOG_ALL(BLINKER_F("_tri_times: "), _lpTrigged_times);
            BLINKER_LOG_ALL(BLINKER_F("_time1: "), _lpTime1);
            BLINKER_LOG_ALL(BLINKER_F("_action1: "), _lpAction1);
            BLINKER_LOG_ALL(BLINKER_F("_time2: "), _lpTime2);
            BLINKER_LOG_ALL(BLINKER_F("_action2: "), _lpAction2);
            BLINKER_LOG_ALL(BLINKER_F("_lpData: "), _lpData);

            if (_lpState && _lpRunState && (_lpTimes == 0))
            {
                _lpRun1 = true;
                _lpStop = false;

                uint32_t _lpTime1_;

                if (_lpTime1 * 60 > BLINKER_ONE_HOUR_TIME) _lpTime1_ = BLINKER_ONE_HOUR_TIME;
                else _lpTime1_ = _lpTime1 * 60;

                _lpTime1_start = millis();
                lpTicker.once(_lpTime1_, _lp_callback);

                BLINKER_LOG_ALL(BLINKER_F("loop start!"));
            }
        }


        void BlinkerApi::loadTiming()
        {
            BLINKER_LOG_ALL(BLINKER_F("load timing"));

            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
            uint32_t _tmData;
            char     _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];

            if (taskCount > BLINKER_TIMING_TIMER_SIZE)
            {
                taskCount = 0;
            }
            BLINKER_LOG_ALL(BLINKER_F("load timing taskCount: "), taskCount);

            for(uint8_t task = 0; task < taskCount; task++)
            {
                EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE
                            , _tmData);
                EEPROM.get(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE +
                            BLINKER_TIMER_TIMING_SIZE, _tmAction_);

                timingTask[task] = new BlinkerTimingTimer(_tmData, STRING_format(_tmAction_));

                BLINKER_LOG_ALL(BLINKER_F("_tmData: "), _tmData);
                BLINKER_LOG_ALL(BLINKER_F("_tmAction: "), STRING_format(_tmAction_));
            }
            blinkerEEPROMCommit();
            blinkerEEPROMEnd();

            uint8_t  wDay = wday();
            uint16_t nowMins = hour() * 60 + minute();

            freshTiming(wDay, nowMins);
        }


        void BlinkerApi::checkOverlapping(uint8_t checkDays, uint16_t checkMins, uint8_t taskNum)
        {
            BLINKER_LOG_ALL(BLINKER_F("checkMins: "), checkMins);
            BLINKER_LOG_ALL(BLINKER_F("checkDays: "), checkDays);

            char _tmAction[BLINKER_TIMER_TIMING_ACTION_SIZE];

            for (uint8_t task = 0; task < taskCount; task++)
            {

                BLINKER_LOG_ALL(BLINKER_F("getTime: "), timingTask[task]->getTime());
                BLINKER_LOG_ALL(BLINKER_F("isLoop: "), timingTask[task]->isLoop());
                BLINKER_LOG_ALL(BLINKER_F("state: "), timingTask[task]->state());
                BLINKER_LOG_ALL(BLINKER_F("isTimingDay: "), timingTask[task]->isTimingDay(checkDays));

                if((timingTask[task]->getTime() == checkMins) && \
                    !timingTask[task]->isLoop() && \
                    timingTask[task]->state() && \
                    timingTask[task]->isTimingDay(checkDays))
                {

                    timingTask[task]->disableTask();

                    blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);

                    EEPROM.put( BLINKER_EEP_ADDR_TIMER_TIMING + \
                                task * BLINKER_ONE_TIMER_TIMING_SIZE, \
                                timingTask[task]->getTimerData());

                    blinkerEEPROMCommit();
                    blinkerEEPROMEnd();

                    BLINKER_LOG_ALL(BLINKER_F("disable timerData: "), timingTask[task]->getTimerData());
                    BLINKER_LOG_ALL(BLINKER_F("disableTask: "), task);

                    strcpy(_tmAction, timingTask[task]->getAction());

                    if (task != taskNum)
                    {
                            parse(_tmAction, true);
                    }
                }
                else if((timingTask[task]->getTime() == checkMins) && \
                    timingTask[task]->state() && \
                    timingTask[task]->isTimingDay(checkDays))
                {
                    BLINKER_LOG(BLINKER_F("checkOverlapping, timing trigged, action is: "), _tmAction);

                    strcpy(_tmAction, timingTask[task]->getAction());

                    if (task != taskNum)
                    {
                            parse(_tmAction, true);
                    }
                }
            }
        }


        void BlinkerApi::freshTiming(uint8_t wDay, uint16_t nowMins)
        {
            tmTicker.detach();

            uint8_t  cbackData;
            uint8_t  nextTask = BLINKER_TIMING_TIMER_SIZE;
            // uint16_t timingMinsNext;
            uint32_t apartSeconds = BLINKER_ONE_DAY_TIME;
            uint32_t checkSeconds = BLINKER_ONE_DAY_TIME;
            uint32_t nowSeconds = dtime();

            BLINKER_LOG_ALL(BLINKER_F("freshTiming wDay: "), wDay,
                            BLINKER_F(", nowMins: "), nowMins,
                            BLINKER_F(", nowSeconds: "), nowSeconds);

            for (uint8_t task = 0; task < taskCount; task++)
            {
                if (timingTask[task]->isTimingDay(wDay) && timingTask[task]->state())
                {
                    if (timingTask[task]->getTime() > nowMins)
                    {
                        checkSeconds = timingTask[task]->getTime() * 60 - nowSeconds;

                        // checkSeconds =  checkSeconds / 60 / 30;

                        if (checkSeconds <= apartSeconds) {
                            apartSeconds = checkSeconds;
                            nextTask = task;
                        }
                    }
                }

                BLINKER_LOG_ALL(BLINKER_F("isTimingDay: "), timingTask[task]->isTimingDay(wDay));
                BLINKER_LOG_ALL(BLINKER_F("state: "), timingTask[task]->state());
                BLINKER_LOG_ALL(BLINKER_F("getTime: "), timingTask[task]->getTime());

                BLINKER_LOG_ALL(BLINKER_F("for nextTask: "), nextTask,
                                BLINKER_F("  apartSeconds: "), apartSeconds,
                                BLINKER_F(" wDay: "), wDay);
            }

            if (apartSeconds == BLINKER_ONE_DAY_TIME)
            {
                apartSeconds -= nowSeconds;

                // apartSeconds = apartSeconds / 60 / 30;
                BLINKER_LOG_ALL(BLINKER_F("nextTask: "), nextTask,
                                BLINKER_F("  apartSeconds: "), apartSeconds,
                                BLINKER_F(" wDay: "), wDay);

                cbackData = nextTask;
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("nextTask: "), nextTask,
                                BLINKER_F("  apartSeconds: "), apartSeconds,
                                BLINKER_F(" wDay: "), wDay);

                cbackData = nextTask;
            }
            BLINKER_LOG_ALL(BLINKER_F("cbackData: "), cbackData);

            if (apartSeconds > BLINKER_ONE_HOUR_TIME)
            {
                apartSeconds = BLINKER_ONE_HOUR_TIME;

                BLINKER_LOG_ALL(BLINKER_F("change apartSeconds: "), apartSeconds);
            }

            tmTicker.once(apartSeconds, timingHandle, cbackData);
        }


        void BlinkerApi::deleteTiming(uint8_t taskDel)
        {
            if (taskDel < taskCount)
            {
                // tmTicker.detach();

                for (uint8_t task = taskDel; task < (taskCount - 1); task++)
                {
                    // timingTask[task]->freshTimer(timingTask[task + 1]->getTimerData(),
                    //     timingTask[task + 1]->getAction(), timingTask[task + 1]->getText());
                    timingTask[task]->freshTimer(timingTask[task + 1]->getTimerData(),
                                                timingTask[task + 1]->getAction());
                }

                delete timingTask[taskCount - 1];

                taskCount--;

                BLINKER_LOG_ALL(BLINKER_F("delete task: "), taskDel, BLINKER_F(" success!"));

                uint8_t  wDay = wday();
                uint16_t nowMins = hour() * 60 + minute();
                freshTiming(wDay, nowMins);
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("none task to delete!"));
            }
        }


        void BlinkerApi::addTimingTask(uint8_t taskSet, uint32_t timerData, const String & action)
        {
            BLINKER_LOG_ALL(BLINKER_F("addTimingTask taskSet: "), taskSet);
            BLINKER_LOG_ALL(BLINKER_F("addTimingTask timerData: "), timerData);

            if (taskSet <= taskCount && taskCount <= BLINKER_TIMING_TIMER_SIZE)
            {
                // tmTicker.detach();

                if (taskSet == taskCount)
                {
                    if (taskCount == BLINKER_TIMING_TIMER_SIZE)
                    {
                        BLINKER_ERR_LOG(BLINKER_F("timing timer task is full"));
                        return;
                    }
                    // timingTask[taskSet] = new BlinkerTimingTimer(timerData, action, text);
                    timingTask[taskSet] = new BlinkerTimingTimer(timerData, action);
                    taskCount++;

                    BLINKER_LOG_ALL(BLINKER_F("new BlinkerTimingTimer"));
                }
                else
                {
                    // timingTask[taskSet]->freshTimer(timerData, action, text);
                    timingTask[taskSet]->freshTimer(timerData, action);

                    BLINKER_LOG_ALL(BLINKER_F("freshTimer"));
                }

                // if (taskSet <= taskCount) taskCount++;

                BLINKER_LOG_ALL(BLINKER_F("taskCount: "), taskCount);

                uint8_t  wDay = wday();
                uint16_t nowMins = hour() * 60 + minute();

                freshTiming(wDay, nowMins);
            }
            else {
                BLINKER_ERR_LOG(BLINKER_F("timing timer task is full"));
            }
        }


        void BlinkerApi::checkTimerErase()
        {
            // #if 0
            static uint8_t isErase;
            // #endif

            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_TIMER_ERASE, isErase);

            if (isErase)
            {
                for (uint16_t _addr = BLINKER_EEP_ADDR_TIMER;
                    _addr < BLINKER_EEP_ADDR_TIMER_END; _addr++)
                {
                    EEPROM.put(_addr, "\0");
                }
            }

            blinkerEEPROMCommit();
            blinkerEEPROMEnd();
        }


        String BlinkerApi::timerSetting()
        {

            String _data = BLINKER_F("\"");
            _data += BLINKER_F(BLINKER_CMD_COUNTDOWN);
            _data += BLINKER_F("\":");
            _data += STRING_format(_cdState ? "true" : "false");
            _data += BLINKER_F(",\"");
            _data += BLINKER_F(BLINKER_CMD_LOOP);
            _data += BLINKER_F("\":");
            _data += STRING_format(_lpState ? "true" : "false");
            _data += BLINKER_F(",\"");
            _data += BLINKER_F(BLINKER_CMD_TIMING);
            _data += BLINKER_F("\":");
            _data += STRING_format(taskCount ? "true" : "false");


            BLINKER_LOG_ALL(BLINKER_F("timerSetting: "), _data);

            return _data;
        }


        String BlinkerApi::countdownConfig()
        {
            String cdData;

            if (!_cdState)
            {
                cdData = BLINKER_F("{\"");
                cdData += BLINKER_F(BLINKER_CMD_COUNTDOWN);
                cdData += BLINKER_F("\":false}");
            }
            else
            {
                if (_cdRunState)
                {
                    cdData = BLINKER_F("{\"");
                    cdData += BLINKER_F(BLINKER_CMD_COUNTDOWN);
                    cdData += BLINKER_F("\":{\"");
                    cdData += BLINKER_F(BLINKER_CMD_RUN);
                    cdData += BLINKER_F("\":");
                    cdData += STRING_format(_cdRunState ? 1 : 0);
                    cdData += BLINKER_F(",\"");
                    cdData += BLINKER_F(BLINKER_CMD_TOTALTIME);
                    cdData += BLINKER_F("\":");
                    cdData += STRING_format(_cdTime1);
                    cdData += BLINKER_F(",\"");
                    cdData += BLINKER_F(BLINKER_CMD_RUNTIME);
                    cdData += BLINKER_F("\":");
                    cdData += STRING_format((millis() - _cdStart) / 1000 / 60);
                    cdData += BLINKER_F(",\"");
                    cdData += BLINKER_F(BLINKER_CMD_ACTION);
                    cdData += BLINKER_F("\":");
                    cdData += _cdAction;
                    cdData += BLINKER_F("}}");
                }
                else
                {
                    cdData = BLINKER_F("{\"");
                    cdData += BLINKER_F(BLINKER_CMD_COUNTDOWN);
                    cdData += BLINKER_F("\":{\"");
                    cdData += BLINKER_F(BLINKER_CMD_RUN);
                    cdData += BLINKER_F("\":");
                    cdData += STRING_format(_cdRunState ? 1 : 0);
                    cdData += BLINKER_F(",\"");
                    cdData += BLINKER_F(BLINKER_CMD_TOTALTIME);
                    cdData += BLINKER_F("\":");
                    cdData += STRING_format(_cdTime1);
                    cdData += BLINKER_F(",\"");
                    cdData += BLINKER_F(BLINKER_CMD_RUNTIME);
                    cdData += BLINKER_F("\":");
                    cdData += STRING_format(_cdTime2);
                    cdData += BLINKER_F(",\"");
                    cdData += BLINKER_F(BLINKER_CMD_ACTION);
                    cdData += BLINKER_F("\":");
                    cdData += _cdAction;
                    cdData += BLINKER_F("}}");
                }
            }

            return cdData;
        }


        String BlinkerApi::timingConfig()
        {
            String timingTaskStr = BLINKER_F("{\"");
            timingTaskStr += BLINKER_F(BLINKER_CMD_TIMING);
            timingTaskStr += BLINKER_F("\":[");

            for (uint8_t task = 0; task < taskCount; task++)
            {
                //Serial.print(timingTask[task].getTimingCfg());
                timingTaskStr += getTimingCfg(task);
                if (task + 1 < taskCount)
                {
                    //Serial.println(",");
                    timingTaskStr += BLINKER_F(",");
                }
                // else {
                //     Serial.println("");
                // }
            }
            timingTaskStr += BLINKER_F("]}");

            BLINKER_LOG_ALL(BLINKER_F("timingTaskStr: "), timingTaskStr);

            return timingTaskStr;
        }


        String BlinkerApi::getTimingCfg(uint8_t task)
        {
            String timingDayStr = BLINKER_F("");
            uint8_t timingDay = timingTask[task]->getTimingday();
            if (timingTask[task]->isLoop())
            {
                for (uint8_t day = 0; day < 7; day++)
                {
                    // timingDayStr += (timingDay & (uint8_t)pow(2,day)) ? String(day):String("");
                    if ((timingDay >> day) & 0x01) {
                        timingDayStr += STRING_format(1);
                        // if (day < 6 && (timingDay >> (day + 1)))
                        //     timingDayStr += STRING_format(",");
                    }
                    else {
                        timingDayStr += STRING_format(0);
                    }
                    // timingDayStr += String((day < 6) ? ((timingDay >> (day + 1)) ? ",":""):"");
                }

                BLINKER_LOG_ALL(BLINKER_F("timingDayStr: "), timingDayStr);

            }
            else {
                timingDayStr = BLINKER_F("0000000");

                BLINKER_LOG_ALL(BLINKER_F("timingDayStr: "), timingDay);
            }

            String timingConfig = BLINKER_F("{\"");
            timingConfig += BLINKER_F(BLINKER_CMD_TASK);
            timingConfig += BLINKER_F("\":");
            timingConfig += STRING_format(task);
            timingConfig += BLINKER_F(",\"");
            timingConfig += BLINKER_F(BLINKER_CMD_ENABLE);
            timingConfig += BLINKER_F("\":");
            timingConfig += STRING_format((timingTask[task]->state()) ? 1 : 0);
            timingConfig += BLINKER_F(",\"");
            timingConfig += BLINKER_F(BLINKER_CMD_DAY);
            timingConfig += BLINKER_F("\":\"");
            timingConfig += timingDayStr;
            timingConfig += BLINKER_F("\",\"");
            timingConfig += BLINKER_F(BLINKER_CMD_TIME);
            timingConfig += BLINKER_F("\":");
            timingConfig += STRING_format(timingTask[task]->getTime());
            timingConfig += BLINKER_F(",\"");
            timingConfig += BLINKER_F(BLINKER_CMD_ACTION);
            timingConfig += BLINKER_F("\":");
            timingConfig += timingTask[task]->getAction();
            timingConfig += BLINKER_F("}");

            return timingConfig;
        }


        String BlinkerApi::loopConfig()
        {
            String lpData;
            if (!_lpState) {
                lpData = BLINKER_F("{\"");
                lpData += BLINKER_F(BLINKER_CMD_LOOP);
                lpData += BLINKER_F("\":false}");
            }
            else {
                lpData = BLINKER_F("{\"");
                lpData += BLINKER_F(BLINKER_CMD_LOOP);
                lpData += BLINKER_F("\":{\"");
                lpData += BLINKER_F(BLINKER_CMD_TIMES);
                lpData += BLINKER_F("\":");
                lpData += STRING_format(_lpTimes);
                lpData += BLINKER_F(",\"");
                lpData += BLINKER_F(BLINKER_CMD_RUN);
                lpData += BLINKER_F("\":");
                lpData += STRING_format(_lpRunState ? 1 : 0);
                lpData += BLINKER_F(",\"");
                lpData += BLINKER_F(BLINKER_CMD_TRIGGED);
                lpData += BLINKER_F("\":");
                lpData += STRING_format(_lpTimes ? _lpTrigged_times : 0);
                lpData += BLINKER_F(",\"");
                lpData += BLINKER_F(BLINKER_CMD_TIME1);
                lpData += BLINKER_F("\":");
                lpData += STRING_format(_lpTime1);
                lpData += BLINKER_F(",\"");
                lpData += BLINKER_F(BLINKER_CMD_ACTION1);
                lpData += BLINKER_F("\":");
                lpData += _lpAction1;
                lpData += BLINKER_F(",\"");
                lpData += BLINKER_F(BLINKER_CMD_TIME2);
                lpData += BLINKER_F("\":");
                lpData += STRING_format(_lpTime2);
                lpData += BLINKER_F(",\"");
                lpData += BLINKER_F(BLINKER_CMD_ACTION2);
                lpData += BLINKER_F("\":");
                lpData += _lpAction2;
                lpData += BLINKER_F("}}");
            }

            return lpData;
        }


        bool BlinkerApi::timerManager(const JsonObject& data, bool _noSet)
        {
            bool isSet = false;
            bool isCount = false;
            bool isLoop = false;
            bool isTiming = false;

            if (!_noSet)
            {
                isSet = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_SET);
                isCount = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_COUNTDOWN);
                isLoop = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_LOOP);
                isTiming = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_TIMING);
            }
            else {
                isCount = data.containsKey(BLINKER_CMD_COUNTDOWN);
                isLoop = data.containsKey(BLINKER_CMD_LOOP);
                isTiming = data.containsKey(BLINKER_CMD_TIMING);
            }

            if ((isSet || _noSet) && (isCount || isLoop || isTiming))
            {
                BLINKER_LOG_ALL(BLINKER_F("timerManager isParsed"));
                _fresh = true;

                BLINKER_LOG_ALL(BLINKER_F("get timer setting"));

                if (isCount)
                {

                    String _delete = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN];

                    if (_delete == "dlt") _cdState = false;
                    else _cdState = true;

                    if (_cdState)
                    {
                        if (isSet)
                        {
                            _cdRunState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUN];
                        }
                        else if(_noSet)
                        {
                            _cdRunState = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUN];
                        }

                        BLINKER_LOG_ALL(BLINKER_F("countdown state: "), _cdState ? "true" : "false");

                        if (isSet)
                        {
                            // _cdRunState = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_STATE];
                            // _cdRunState = _cdState;
                            int32_t _totalTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_TOTALTIME];
                            // _totalTime = 60 * _totalTime;
                            int32_t _runTime = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUNTIME];
                            // _runTime = 60 * _runTime;
                            String _action = data[BLINKER_CMD_SET][BLINKER_CMD_COUNTDOWN][BLINKER_CMD_ACTION];

                            if (_action.length() > BLINKER_TIMER_COUNTDOWN_ACTION_SIZE)
                            {
                                BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                                return true;
                            }

                            // BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION: "), _action , BLINKER_F(", LEN: "), _action.length());

                            if (_cdRunState && _action != "null")
                            {
                                // _cdAction = _action;
                                strcpy(_cdAction, _action.c_str());
                                _cdTime1 = _totalTime;
                                _cdTime2 = _runTime;
                            }

                            if (!_cdRunState && _action == "null")
                            {
                                _cdTime2 += (millis() - _cdStart) / 1000 / 60;
                            }
                            // else if (_cdRunState && _action.length() == 0) {
                            //     _cdTime2 = 0;
                            // }

                            BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
                        }
                        else if (_noSet)
                        {
                            // _cdRunState = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_STATE];
                            // _cdRunState = _cdState;
                            int32_t _totalTime = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_TOTALTIME];
                            // _totalTime = 60 * _totalTime;
                            int32_t _runTime = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_RUNTIME];
                            // _runTime = 60 * _runTime;
                            String _action = data[BLINKER_CMD_COUNTDOWN][BLINKER_CMD_ACTION];

                            if (_action.length() > BLINKER_TIMER_COUNTDOWN_ACTION_SIZE)
                            {
                                BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                                return true;
                            }

                            if (_cdRunState && _action != "null")
                            {
                                // _cdAction = _action;
                                strcpy(_cdAction, _action.c_str());
                                _cdTime1 = _totalTime;
                                _cdTime2 = _runTime;
                            }

                            if (!_cdRunState && _action == "null")
                            {
                                _cdTime2 += (millis() - _cdStart) / 1000 / 60;
                            }
                            // else if (_cdRunState && _action.length() == 0) {
                            //     _cdTime2 = 0;
                            // }

                            BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
                        }
                        _cdData = _cdState << 31 | _cdRunState << 30 | _cdTime1 << 12 | _cdTime2;

                        BLINKER_LOG_ALL(BLINKER_F("_totalTime: "), _cdTime1);
                        BLINKER_LOG_ALL(BLINKER_F("_runTime: "), _cdTime2);
                        BLINKER_LOG_ALL(BLINKER_F("_action: "), _cdAction);
                        BLINKER_LOG_ALL(BLINKER_F("_cdData: "), _cdData);

                        // char _cdAction_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
                        // strcpy(_cdAction_, _cdAction.c_str());

                        blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
                        // EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction_);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction);
                        blinkerEEPROMCommit();
                        blinkerEEPROMEnd();

                        if (_cdState && _cdRunState)
                        {
                            // _cdTime1 = _cdTime1 - _cdTime2;
                            // _cdTime2 = 0;

                            uint32_t _cdTime1_ = _cdTime1 - _cdTime2;

                            if (_cdTime1_ * 60 > BLINKER_ONE_HOUR_TIME) _cdTime1_ = BLINKER_ONE_HOUR_TIME;
                            else _cdTime1_ = _cdTime1_ * 60;

                            cdTicker.once(_cdTime1_, _cd_callback);

                            _cdStart = millis();

                            BLINKER_LOG_ALL(BLINKER_F("countdown start! time: "), _cdTime1);
                        }
                        else
                        {
                            cdTicker.detach();
                        }
                    }
                    else {
                        _cdRunState = 0;
                        _cdTime1 = 0;
                        _cdTime2 = 0;
                        // _cdAction = "";
                        // memcpy(_cdAction, '\0', BLINKER_ACTION_SIZE);

                        _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);

                        BLINKER_LOG_ALL(BLINKER_F("countdown state: "), _cdState ? "true" : "false");
                        BLINKER_LOG_ALL(BLINKER_F("_cdRunState: "), _cdRunState);
                        BLINKER_LOG_ALL(BLINKER_F("_totalTime: "), _cdTime1);
                        BLINKER_LOG_ALL(BLINKER_F("_runTime: "), _cdTime2);
                        BLINKER_LOG_ALL(BLINKER_F("_action: "), _cdAction);
                        BLINKER_LOG_ALL(BLINKER_F("_cdData: "), _cdData);

                        // char _cdAction_[BLINKER_TIMER_COUNTDOWN_ACTION_SIZE];
                        // strcpy(_cdAction_, _cdAction.c_str());

                        blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN, _cdData);
                        // EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction_);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_COUNTDOWN_ACTION, _cdAction);
                        blinkerEEPROMCommit();
                        blinkerEEPROMEnd();

                        cdTicker.detach();
                    }

                    // static_cast<Proto*>(this)->checkState(false);
                    BProto::_timerPrint(countdownConfig());
                    BProto::printNow();
                    return true;
                }
                else if (isLoop)
                {
                    _lpState = true;

                    String _delete = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP];

                    if (_delete == "dlt") _lpState = false;
                    else _lpState = true;

                    if (_lpState)
                    {
                        if (isSet)
                        {
                            _lpRunState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_RUN];
                        }
                        else if (_noSet)
                        {
                            _lpRunState = data[BLINKER_CMD_LOOP][BLINKER_CMD_RUN];
                        }

                        BLINKER_LOG_ALL(BLINKER_F("loop state: "), _lpState ? "true" : "false");

                        if (isSet)
                        {
                            int8_t _times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIMES];
                            int8_t _tri_times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TRIGGED];
                            // _lpRunState = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_STATE];
                            // _lpRunState = _lpState;
                            int32_t _time1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIME1];
                            // _time1 = 60 * _time1;
                            String _action1 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_ACTION1];
                            int32_t _time2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TIME2];
                            // _time2 = 60 * _time2;
                            String _action2 = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_ACTION2];

                            if (_action1.length() > BLINKER_TIMER_LOOP_ACTION1_SIZE)
                            {
                                BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                                return true;
                            }

                            if (_action2.length() > BLINKER_TIMER_LOOP_ACTION2_SIZE)
                            {
                                BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                                return true;
                            }

                            if (_lpRunState && _action2 != "null")
                            {
                                // _lpAction1 = _action1;
                                // _lpAction2 = _action2;
                                strcpy(_lpAction1, _action1.c_str());
                                strcpy(_lpAction2, _action2.c_str());

                                _lpTimes = _times;
                                _lpTrigged_times = _tri_times;
                                _lpTime1 = _time1;
                                _lpTime2 = _time2;
                            }

                            BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);

                        }
                        else if (_noSet)
                        {
                            int8_t _times = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIMES];
                            int8_t _tri_times = data[BLINKER_CMD_SET][BLINKER_CMD_LOOP][BLINKER_CMD_TRIGGED];
                            // _lpRunState = data[BLINKER_CMD_LOOP][BLINKER_CMD_STATE];
                            // _lpRunState = _lpState;
                            int32_t _time1 = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIME1];
                            // _time1 = 60 * _time1;
                            String _action1 = data[BLINKER_CMD_LOOP][BLINKER_CMD_ACTION1];
                            int32_t _time2 = data[BLINKER_CMD_LOOP][BLINKER_CMD_TIME2];
                            // _time2 = 60 * _time2;
                            String _action2 = data[BLINKER_CMD_LOOP][BLINKER_CMD_ACTION2];

                            if (_action1.length() > BLINKER_TIMER_LOOP_ACTION1_SIZE)
                            {
                                BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                                return true;
                            }

                            if (_action2.length() > BLINKER_TIMER_LOOP_ACTION2_SIZE)
                            {
                                BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                                return true;
                            }

                            if (_lpRunState && _action2 != "null") {
                                // _lpAction1 = _action1;
                                // _lpAction2 = _action2;
                                strcpy(_lpAction1, _action1.c_str());
                                strcpy(_lpAction2, _action2.c_str());

                                _lpTimes = _times;
                                _lpTrigged_times = _tri_times;
                                _lpTime1 = _time1;
                                _lpTime2 = _time2;
                            }

                            BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);
                        }

                        if (_lpTimes > 100) _lpTimes = 0;

                        _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;

                        BLINKER_LOG_ALL(BLINKER_F("_times: "), _lpTimes);
                        BLINKER_LOG_ALL(BLINKER_F("_tri_times: "), _lpTrigged_times);
                        BLINKER_LOG_ALL(BLINKER_F("_time1: "), _lpTime1);
                        BLINKER_LOG_ALL(BLINKER_F("_action1: "), _lpAction1);
                        BLINKER_LOG_ALL(BLINKER_F("_time2: "), _lpTime2);
                        BLINKER_LOG_ALL(BLINKER_F("_action2: "), _lpAction2);
                        BLINKER_LOG_ALL(BLINKER_F("_lpData: "), _lpData);

                        // char _lpAction_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
                        // char _lpAction_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
                        // strcpy(_lpAction_1, _lpAction1.c_str());
                        // strcpy(_lpAction_2, _lpAction2.c_str());

                        blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_TRI, _lpTrigged_times);
                        // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction_1);
                        // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction_2);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction1);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction2);
                        blinkerEEPROMCommit();
                        blinkerEEPROMEnd();

                        if (_lpState && _lpRunState)
                        {
                            _lpRun1 = true;
                            // _lpTrigged_times = 0;
                            _lpStop = false;

                            uint32_t _lpTime1_;

                            if (_lpTime1 * 60 > BLINKER_ONE_HOUR_TIME) _lpTime1_ = BLINKER_ONE_HOUR_TIME;
                            else _lpTime1_ = _lpTime1 * 60;

                            _lpTime1_start = millis();
                            lpTicker.once(_lpTime1_, _lp_callback);

                            BLINKER_LOG_ALL(BLINKER_F("loop start!"));
                        }
                        else
                        {
                            lpTicker.detach();
                        }
                    }
                    else
                    {
                        _lpRunState = 0;
                        _lpTimes = 0;
                        _lpTrigged_times = 0;
                        _lpTime1 = 0;
                        _lpTime2 = 0;
                        // _lpAction1 = "";
                        // _lpAction2 = "";
                        // memcpy(_lpAction1, '\0', BLINKER_ACTION_SIZE);
                        // memcpy(_lpAction2, '\0', BLINKER_ACTION_SIZE);

                        _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;

                        BLINKER_LOG_ALL(BLINKER_F("loop state: "), _lpState ? "true" : "false");
                        BLINKER_LOG_ALL(BLINKER_F("_lpRunState: "), _lpRunState);
                        BLINKER_LOG_ALL(BLINKER_F("_times: "), _lpTimes);
                        BLINKER_LOG_ALL(BLINKER_F("_tri_times: "), _lpTrigged_times);
                        BLINKER_LOG_ALL(BLINKER_F("_time1: "), _lpTime1);
                        BLINKER_LOG_ALL(BLINKER_F("_action1: "), _lpAction1);
                        BLINKER_LOG_ALL(BLINKER_F("_time2: "), _lpTime2);
                        BLINKER_LOG_ALL(BLINKER_F("_action2: "), _lpAction2);
                        BLINKER_LOG_ALL(BLINKER_F("_lpData: "), _lpData);

                        // char _lpAction_1[BLINKER_TIMER_LOOP_ACTION1_SIZE];
                        // char _lpAction_2[BLINKER_TIMER_LOOP_ACTION2_SIZE];
                        // strcpy(_lpAction_1, _lpAction1.c_str());
                        // strcpy(_lpAction_2, _lpAction2.c_str());

                        blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP, _lpData);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_TRI, _lpTrigged_times);
                        // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction_1);
                        // EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction_2);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION1, _lpAction1);
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_LOOP_ACTION2, _lpAction2);
                        blinkerEEPROMCommit();
                        blinkerEEPROMEnd();

                        lpTicker.detach();
                    }

                    BProto::_timerPrint(loopConfig());
                    BProto::printNow();
                    return true;
                }
                else if (isTiming)
                {
                    bool isDelet = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_DELETETASK);

                    _tmState = true;

                    if (isSet)
                    {
                        _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_ENABLE];
                    }
                    else if (_noSet)
                    {
                        _tmRunState = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_ENABLE];
                    }

                    BLINKER_LOG_ALL(BLINKER_F("timing state: "), _tmState ? "true" : "false");

                    int32_t _time;
                    String _action;
                    String _text;
                    uint8_t _task;

                    if (isSet && !isDelet)
                    {
                        // _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_STATE];
                        // _tmRunState = _tmState;
                        _time = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TIME];
                        // _time = 60 * _time;
                        String tm_action = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_ACTION];
                        // String tm_text = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TEXT];
                        _task = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TASK];

                        // _tmAction = _action;

                        if (tm_action.length() > BLINKER_TIMER_TIMING_ACTION_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        _action = tm_action;
                        // _text = tm_text;

                        // _tmTime = _time;

                        String tm_day = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY];

                        _timingDay = 0;

                        if (tm_day.toInt() == 0)
                        {
                            if (60 * _time > dtime())
                            {
                                _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
                            }
                            else {
                                _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
                            }

                            _isTimingLoop = false;

                            BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                        }
                        else
                        {

                            _isTimingLoop = true;

                            // uint8_t taskDay;

                            for (uint8_t day = 0; day < 7; day++)
                            {
                                if (tm_day.substring(day, day+1) == "1")
                                {
                                    _timingDay |= (0x01 << day);

                                    BLINKER_LOG_ALL(BLINKER_F("day: "), day, BLINKER_F(" timingDay: "), _timingDay);
                                }
                            }
                        }

        //                     if (data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0] == 7) {
        //                         if (_tmTime2 > dtime()) {
        //                             _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
        //                         }
        //                         else {
        //                             _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
        //                         }

        //                         _isTimingLoop = false;
        // #ifdef BLINKER_DEBUG_ALL
        //                         BLINKER_LOG(BLINKER_F("timingDay: "), _timingDay);
        // #endif
        //                     }
        //                     else {
        //                         uint8_t taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0];
        //                         _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
        // #ifdef BLINKER_DEBUG_ALL
        //                         BLINKER_LOG(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
        // #endif

        //                         for (uint8_t day = 1;day < 7;day++) {
        //                             taskDay = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][day];
        //                             if (taskDay > 0) {
        //                                 _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
        // #ifdef BLINKER_DEBUG_ALL
        //                                 BLINKER_LOG(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
        // #endif
        //                             }
        //                         }

        //                         _isTimingLoop = true;
        //                     }

                        BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                        // BLINKER_LOG_ALL(BLINKER_F("_text: "), _text);
                        BLINKER_LOG_ALL(BLINKER_F("_tmRunState: "), _tmRunState);
                        BLINKER_LOG_ALL(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                        BLINKER_LOG_ALL(BLINKER_F("_time: "), _time);
                        BLINKER_LOG_ALL(BLINKER_F("_action: "), _action);

                        uint32_t _timerData = _isTimingLoop << 31 | _tmRunState << 23 | _timingDay << 11 | _time;

                        // addTimingTask(_task, _timerData, _action, _text);
                        addTimingTask(_task, _timerData, _action);
                    }
                    else if (_noSet)
                    {
                        // _tmRunState = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_STATE];
                        // _tmRunState = _tmState;
                        _time = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_TIME];
                        // _time = 60 * _time;
                        String tm_action = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_ACTION];
                        // String tm_text = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_TEXT];
                        _task = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_TASK];

                        // _tmAction = _action;

                        if (tm_action.length() > BLINKER_TIMER_TIMING_ACTION_SIZE)
                        {
                            BLINKER_ERR_LOG(BLINKER_F("TIMER ACTION TOO LONG"));
                            return true;
                        }

                        _action = tm_action;
                        // _text = tm_text;

                        // _tmTime = _time;

                        String tm_day = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY];

                        if (tm_day.toInt() == 0)
                        {
                            if (60 * _time > dtime())
                            {
                                _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
                            }
                            else {
                                _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
                            }

                            _isTimingLoop = false;

                            BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                        }
                        else
                        {
                            // uint8_t taskDay;

                            _isTimingLoop = true;

                            for (uint8_t day = 0; day < 7; day++)
                            {
                                if (tm_day.substring(day, day+1) == "1")
                                {
                                    _timingDay |= (0x01 << day);

                                    BLINKER_LOG_ALL(BLINKER_F("day: "), day, BLINKER_F(" timingDay: "), _timingDay);
                                }
                            }
                        }

        //                     if (data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0] == 7) {
        //                         if (_tmTime2 > dtime()) {
        //                             _timingDay |= (0x01 << wday());//timeinfo.tm_wday(uint8_t)pow(2,timeinfo.tm_wday);
        //                         }
        //                         else {
        //                             _timingDay |= (0x01 << ((wday() + 1) % 7));//timeinfo.tm_wday(uint8_t)pow(2,(timeinfo.tm_wday + 1) % 7);
        //                         }

        //                         _isTimingLoop = false;
        // #ifdef BLINKER_DEBUG_ALL
        //                         BLINKER_LOG(BLINKER_F("timingDay: "), _timingDay);
        // #endif
        //                     }
        //                     else {
        //                         uint8_t taskDay = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][0];
        //                         _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
        // #ifdef BLINKER_DEBUG_ALL
        //                         BLINKER_LOG(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
        // #endif

        //                         for (uint8_t day = 1;day < 7;day++) {
        //                             taskDay = data[BLINKER_CMD_TIMING][0][BLINKER_CMD_DAY][day];
        //                             if (taskDay > 0) {
        //                                 _timingDay |= (0x01 << taskDay);//(uint8_t)pow(2,taskDay);
        // #ifdef BLINKER_DEBUG_ALL
        //                                 BLINKER_LOG(BLINKER_F("day: "), taskDay, BLINKER_F(" timingDay: "), _timingDay);
        // #endif
        //                             }
        //                         }

        //                         _isTimingLoop = true;
        //                     }

                        BLINKER_LOG_ALL(BLINKER_F("timingDay: "), _timingDay);
                        // BLINKER_LOG_ALL(BLINKER_F("_text: "), _text);
                        BLINKER_LOG_ALL(BLINKER_F("_tmRunState: "), _tmRunState);
                        BLINKER_LOG_ALL(BLINKER_F("_isTimingLoop: "), _isTimingLoop ? "true":"false");
                        BLINKER_LOG_ALL(BLINKER_F("_time: "), _time);
                        BLINKER_LOG_ALL(BLINKER_F("_action: "), _action);

                        uint32_t _timerData = _isTimingLoop << 31 | _tmRunState << 23 | _timingDay << 11 | _time;

                        // addTimingTask(_task, _timerData, _action, _text);
                        addTimingTask(_task, _timerData, _action);
                    }
                    else if (isDelet)
                    {
                        uint8_t _delTask = data[BLINKER_CMD_SET][BLINKER_CMD_TIMING][0][BLINKER_CMD_DELETETASK];

                        deleteTiming(_delTask);
                    }

                    char _tmAction_[BLINKER_TIMER_TIMING_ACTION_SIZE];

                    blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                    EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING_COUNT, taskCount);
                    for(uint8_t task = 0; task < taskCount; task++)
                    {
                        strcpy(_tmAction_, timingTask[task]->getAction());

                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE
                                    , timingTask[task]->getTimerData());
                        EEPROM.put(BLINKER_EEP_ADDR_TIMER_TIMING + task * BLINKER_ONE_TIMER_TIMING_SIZE +
                                    BLINKER_TIMER_TIMING_SIZE, _tmAction_);

                        BLINKER_LOG_ALL(BLINKER_F("getTimerData: "), timingTask[task]->getTimerData());
                        BLINKER_LOG_ALL(BLINKER_F("_tmAction_: "), _tmAction_);
                    }
                    blinkerEEPROMCommit();
                    blinkerEEPROMEnd();

                    BProto::_timerPrint(timingConfig());
                    BProto::printNow();

                    BLINKER_LOG_FreeHeap_ALL();

                    return true;
                }
            }
            else if (data.containsKey(BLINKER_CMD_GET))
            {
                String get_timer = data[BLINKER_CMD_GET];

                if (get_timer == BLINKER_CMD_TIMER)
                {
                    BProto::_timerPrint(timerSetting());
                    BProto::printNow();
                    BLINKER_LOG_ALL(BLINKER_F("timerManager1 isParsed"));
                    _fresh = true;
                    return true;
                }
                else if (get_timer == BLINKER_CMD_COUNTDOWN)
                {
                    BProto::_timerPrint(countdownConfig());
                    BProto::printNow();
                    BLINKER_LOG_ALL(BLINKER_F("timerManager2 isParsed"));
                    _fresh = true;
                    return true;
                }
                else if (get_timer == BLINKER_CMD_LOOP)
                {
                    BProto::_timerPrint(loopConfig());
                    BProto::printNow();
                    BLINKER_LOG_ALL(BLINKER_F("timerManager3 isParsed"));
                    _fresh = true;
                    return true;
                }
                else if (get_timer == BLINKER_CMD_TIMING)
                {
                    BProto::_timerPrint(timingConfig());
                    BProto::printNow();
                    BLINKER_LOG_ALL(BLINKER_F("timerManager4 isParsed"));
                    _fresh = true;
                    return true;
                }
            }
            else
            {
                return false;
            }

            return false;
        }


        bool BlinkerApi::checkTimer()
        {
            if (_cdTrigged)
            {
                _cdTrigged = false;

                // _cdRunState = false;
                _cdState = false;
                // _cdData |= _cdRunState << 14;
                // _cdData = _cdState << 15 | _cdRunState << 14 | (_cdTime1 - _cdTime2);
                _cdData = _cdState << 31 | _cdRunState << 30 | _cdTime1 << 12 | _cdTime2;
                saveCountDown(_cdData, _cdAction);

                BLINKER_LOG_ALL(BLINKER_F("countdown trigged, action is: "), _cdAction);

                // _parse(_cdAction);

                    parse(_cdAction, true);
            }
            if (_lpTrigged)
            {
                _lpTrigged = false;

                if (_lpStop)
                {
                    // _lpRunState = false;
                    _lpState = false;
                    // _lpData |= _lpRunState << 30;
                    _lpData = _lpState << 31 | _lpRunState << 30 | _lpTimes << 22 | _lpTime1 << 11 | _lpTime2;
                    saveLoop(_lpData, _lpAction1, _lpAction2);
                }

                if (_lpRun1)
                {
                    BLINKER_LOG_ALL(BLINKER_F("loop trigged, action is: "), _lpAction2);
                    // _parse(_lpAction2);

                        parse(_lpAction2, true);
                }
                else
                {

                    BLINKER_LOG_ALL(BLINKER_F("loop trigged, action is: "), _lpAction1);
                    // _parse(_lpAction1);

                        parse(_lpAction1, true);
                }
            }
            if (_tmTrigged)
            {
                _tmTrigged = false;

        //             if (_tmRun1) {
        // #ifdef BLINKER_DEBUG_ALL
        //                 BLINKER_LOG(BLINKER_F("timing trigged, action is: "), _tmAction2);
        // #endif
        //                 // _parse(_tmAction2);
        //                 parse(_tmAction2, true);
        //             }
        //             else {
        // #ifdef BLINKER_DEBUG_ALL
        //                 BLINKER_LOG(BLINKER_F("timing trigged, action is: "), _tmAction1);
        // #endif
        //                 // _parse(_tmAction1);
        //                 parse(_tmAction1, true);
        //             }

                uint8_t wDay =  wday();

                BLINKER_LOG_ALL(hour(), ":", minute(), ":", second());

                uint16_t nowMins = hour() * 60 + minute();

                if (triggedTask < BLINKER_TIMING_TIMER_SIZE && \
                    nowMins != timingTask[triggedTask]->getTime())
                {
                    BLINKER_LOG_ALL(BLINKER_F("timing trigged, now minutes check error!"));

                    freshTiming(wDay, nowMins);

                    return false;
                }

                if (triggedTask < BLINKER_TIMING_TIMER_SIZE)
                {
                    // String _tmAction = timingTask[triggedTask]->getAction();
                    char _tmAction[BLINKER_TIMER_TIMING_ACTION_SIZE];

                    strcpy(_tmAction, timingTask[triggedTask]->getAction());

                    BLINKER_LOG(BLINKER_F("timing trigged, action is: "), _tmAction);

                        parse(_tmAction, true);

                    checkOverlapping(wDay, timingTask[triggedTask]->getTime(), triggedTask);

                    freshTiming(wDay, timingTask[triggedTask]->getTime());

                    return true;
                }
                else
                {
                    BLINKER_LOG_ALL(BLINKER_F("timing trigged, none action"));

                    freshTiming(wDay, 0);

                    return false;
                }
            }

            return false;
        }


    bool BlinkerApi::checkCUPDATE()
    {
        if ((millis() - _cUpdateTime) >= BLINKER_CONFIG_UPDATE_LIMIT || \
            _cUpdateTime == 0) return true;
        else return false;
    }


    bool BlinkerApi::checkCGET()
    {
        if ((millis() - _cGetTime) >= BLINKER_CONFIG_GET_LIMIT || \
            _cGetTime == 0) return true;
        else return false;
    }


    bool BlinkerApi::checkCDEL()
    {
        if ((millis() - _cDelTime) >= BLINKER_CONFIG_GET_LIMIT || \
            _cDelTime == 0) return true;
        else return false;
    }


    bool BlinkerApi::checkDataUpdata()
    {
        if ((millis() - _dUpdateTime) >= BLINKER_CONFIG_UPDATE_LIMIT || \
            _dUpdateTime == 0) return true;
        else return false;
    }


    bool BlinkerApi::checkDataGet()
    {
        if ((millis() - _dGetTime) >= BLINKER_CONFIG_UPDATE_LIMIT || \
            _dGetTime == 0) return true;
        else return false;
    }


    bool BlinkerApi::checkDataDel()
    {
        if ((millis() - _dDelTime) >= BLINKER_CONFIG_UPDATE_LIMIT || \
            _dDelTime == 0) return true;
        else return false;
    }

    bool BlinkerApi::checkAutoPull()
    {
        if ((millis() - _autoPullTime) >= 60000 || \
            _autoPullTime == 0) return true;
        else return false;
    }



        void BlinkerApi::autoStart()
        {
            // autoPull();

            BLINKER_LOG_ALL(BLINKER_F("_______autoStart_______"));
            uint8_t checkData;

            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
            EEPROM.get(BLINKER_EEP_ADDR_CHECK, checkData);
            if (checkData != BLINKER_CHECK_DATA)
            {
                for (uint16_t _addr = BLINKER_EEP_ADDR_AUTO_START;
                    _addr < BLINKER_EEP_ADDR_AUTO_START +
                    BLINKER_ONE_AUTO_DATA_SIZE * 2; _addr++)
                {
                    EEPROM.put(_addr, "\0");
                }
                EEPROM.put(BLINKER_EEP_ADDR_CHECK, BLINKER_CHECK_DATA);
                blinkerEEPROMCommit();
                blinkerEEPROMEnd();
                return;
            }
            EEPROM.get(BLINKER_EEP_ADDR_AUTONUM, _aCount);
            if (_aCount > 2)
            {
                _aCount = 0;
                EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
            }
            blinkerEEPROMCommit();
            blinkerEEPROMEnd();

            BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

            if (_aCount)
            {
                for (uint8_t _num = 0; _num < _aCount; _num++)
                {
                    BLINKER_LOG_ALL(BLINKER_F("new BlinkerAUTO() _num: "), _num);

                    _AUTO[_num] = new BlinkerAUTO();
                    _AUTO[_num]->setNum(_num);
                    _AUTO[_num]->deserialization();
                }
            }
        }


        bool BlinkerApi::autoManager(const JsonObject& data)
        {
            // String set;
            bool isSet = false;
            bool isAuto = false;

            // isSet = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_SET);
            // isAuto = STRING_contains_string(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTO);
            isSet = data.containsKey(BLINKER_CMD_SET);
            String aData = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO].as<String>();
            // const char* aDataArray = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][0];

            // if (aData.length()) isAuto = true;
            if (aData != "null") isAuto = true;

            BLINKER_LOG_ALL(BLINKER_F("autoManager begin: "), isAuto, " ", isSet);

            // if (aDataArray && !isAuto)
            if (!isAuto)
            {
                for (uint8_t num = 0; num < 2; num++)
                {
                    unsigned long _autoId = data[BLINKER_CMD_AUTO][num][BLINKER_CMD_AUTOID];
                    String arrayData = data[BLINKER_CMD_AUTO][num];

                    if (_aCount)
                    {
                        for (uint8_t _num = 0; _num < _aCount; _num++)
                        {
                            if (_AUTO[_num]->id() == _autoId) {
                                // _AUTO[_num]->manager(arrayData);
                                return true;
                            }
                        }
                        if (_aCount == 1)
                        {
                            _AUTO[_aCount] = new BlinkerAUTO();
                            _AUTO[_aCount]->setNum(_aCount);
                            // _AUTO[_aCount]->manager(arrayData);

                            // _aCount = 1;
                            _aCount++;
                            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                            blinkerEEPROMCommit();
                            blinkerEEPROMEnd();

                            BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);
                            // static_cast<Proto*>(this)->_print(autoData(), false);
                            // return true;
                        }
                        else
                        {
                            // _AUTO[_aCount - 1]->manager(arrayData);
                            // return true;
                        }
                    }
                    else
                    {
                        _AUTO[_aCount] = new BlinkerAUTO();
                        _AUTO[_aCount]->setNum(_aCount);
                        // _AUTO[_aCount]->manager(arrayData);

                        _aCount = 1;
                        // _aCount++;
                        blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                        blinkerEEPROMCommit();
                        blinkerEEPROMEnd();

                        BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);
                        // static_cast<Proto*>(this)->_print(autoData(), false);
                        // return true;
                    }
                }
                return true;
            }
            else if (isSet && isAuto)
            {
                // BLINKER_LOG_ALL(BLINKER_F("timerManager5 isParsed"));
                _fresh = true;

                BLINKER_LOG_ALL(BLINKER_F("get auto setting"));

                // bool isDelet = STRING_contains_string(BProto::dataParse(), BLINKER_CMD_DELETID);
                String isTriggedArray = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                            [BLINKER_CMD_ACTION][0];

                // if (isDelet)
                // {
                //     // uint32_t _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_DELETID);
                //     uint32_t _autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DELETE];

                //     if (_aCount)
                //     {
                //         for (uint8_t _num = 0; _num < _aCount; _num++)
                //         {
                //             if (_AUTO[_num]->id() == _autoId)
                //             {
                //                 // _AUTO[_num]->manager(static_cast<Proto*>(this)->dataParse());
                //                 for (uint8_t a_num = _num; a_num < _aCount; a_num++)
                //                 {
                //                     if (a_num < _aCount - 1)
                //                     {
                //                         _AUTO[a_num]->setNum(a_num + 1);
                //                         _AUTO[a_num]->deserialization();
                //                         _AUTO[a_num]->setNum(a_num);
                //                         _AUTO[a_num]->serialization();
                //                     }
                //                     else{
                //                         _num = _aCount;
                //                     }
                //                 }
                //                 _aCount--;

                //                 blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                //                 EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                //                 blinkerEEPROMCommit();
                //                 blinkerEEPROMEnd();

                //                 BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

                //                 return true;
                //             }
                //         }
                //     }
                // }
                // else if(isTriggedArray != "null")
                // {
                //     for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
                //     {
                //         String _autoData_array = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                //                                     [BLINKER_CMD_ACTION][a_num];

                //         if(_autoData_array != "null")
                //         {
                //             // DynamicJsonBuffer _jsonBuffer;
                //             // JsonObject& _array = _jsonBuffer.parseObject(_autoData_array);
                //             JsonDocument jsonBuffer;
                //             deserializeJson(jsonBuffer, _autoData_array);
                //             JsonObject _array = jsonBuffer.as<JsonObject>();

                //             json_parse(_array);
                //                 1 && 1 && \
                //             timerManager(_array, true);
                //             #endif
                //         }
                //         else
                //         {
                //             // a_num = BLINKER_MAX_WIDGET_SIZE;
                //             return true;
                //         }
                //     }
                // }
                if (isTriggedArray != "null")
                {
                    BLINKER_LOG_ALL(BLINKER_F("_auto trigged action: "), isTriggedArray);

                    for (uint8_t a_num = 0; a_num < BLINKER_MAX_WIDGET_SIZE; a_num++)
                    {
                        String _autoData_array = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO]
                                                    [BLINKER_CMD_ACTION][a_num];

                        if(_autoData_array != "null")
                        {
                            _fresh = false;
                            // DynamicJsonBuffer _jsonBuffer;
                            // JsonObject& _array = _jsonBuffer.parseObject(_autoData_array);
                            JsonDocument jsonBuffer;
                            deserializeJson(jsonBuffer, _autoData_array);
                            JsonObject _array = jsonBuffer.as<JsonObject>();

                            json_parse(_array);
                            timerManager(_array, true);

                            if (_fresh)
                            {
                                BProto::isParsed();
                            }
                            else
                            {
                            }
                        }
                        else
                        {
                            // a_num = BLINKER_MAX_WIDGET_SIZE;
                            return true;
                        }
                    }
                }
                else
                {
                    // uint32_t _autoId = STRING_find_numberic_value(static_cast<Proto*>(this)->dataParse(), BLINKER_CMD_AUTOID);
                    String get_autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_ID].as<String>();
                    String del_autoId = data[BLINKER_CMD_SET][BLINKER_CMD_AUTO][BLINKER_CMD_DELETE].as<String>();
                    // _autoId = get_autoId.toInt();

                    BLINKER_LOG_ALL(BLINKER_F("get_autoId: "), strtoul(get_autoId.c_str(),NULL,10));
                    BLINKER_LOG_ALL(BLINKER_F("del_autoId: "), strtoul(del_autoId.c_str(),NULL,10));
                    BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);
                    // _aCount = 0;

                    if (_aCount && strtoul(del_autoId.c_str(),NULL,10) == 0)
                    {
                        for (uint8_t _num = 0; _num < _aCount; _num++)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("check _autoId: "), _AUTO[_num]->id(), " ", _AUTO[_num]->id() == strtoul(get_autoId.c_str(),NULL,10));
                            if (_AUTO[_num]->id() == strtoul(get_autoId.c_str(),NULL,10))
                            {
                                _AUTO[_num]->manager(data);
                                return true;
                            }
                        }
                        if (_aCount == 1)
                        {
                            _AUTO[_aCount] = new BlinkerAUTO();
                            _AUTO[_aCount]->setNum(_aCount);
                            _AUTO[_aCount]->manager(data);

                            // _aCount = 1;
                            _aCount++;
                            blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                            blinkerEEPROMCommit();
                            blinkerEEPROMEnd();

                            BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

                            // static_cast<Proto*>(this)->_print(autoData(), false);
                            // return true;
                        }
                        else
                        {
                            _AUTO[0]->setNum(1);
                            _AUTO[0]->deserialization();
                            _AUTO[0]->setNum(0);
                            _AUTO[0]->serialization();

                            _AUTO[1] = new BlinkerAUTO();
                            _AUTO[1]->setNum(1);
                            _AUTO[1]->manager(data);
                            // return true;
                        }
                    }
                    else if (_aCount == 0 && strtoul(del_autoId.c_str(),NULL,10) == 0)
                    {
                        _AUTO[_aCount] = new BlinkerAUTO();
                        _AUTO[_aCount]->setNum(_aCount);
                        _AUTO[_aCount]->manager(data);

                        _aCount = 1;
                        // _aCount++;
                        blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                        EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                        blinkerEEPROMCommit();
                        blinkerEEPROMEnd();

                        BLINKER_LOG_ALL(BLINKER_F("_aCount: "), _aCount);

                        // static_cast<Proto*>(this)->_print(autoData(), false);
                        // return true;
                    }
                    else if (strtoul(del_autoId.c_str(),NULL,10) != 0)
                    {
                        for (uint8_t _num = 0; _num < _aCount; _num++)
                        {
                            BLINKER_LOG_ALL(BLINKER_F("check _autoId: "), _AUTO[_num]->id(), " ", _AUTO[_num]->id() == strtoul(del_autoId.c_str(),NULL,10));
                            if (_AUTO[_num]->id() == strtoul(del_autoId.c_str(),NULL,10))
                            {
                                if (_num == 0)
                                {
                                    _AUTO[1]->setNum(0);
                                    _AUTO[1]->serialization();

                                    _AUTO[0]->deserialization();

                                    free(_AUTO[1]);

                                    _aCount--;
                                    blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                                    EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                                    blinkerEEPROMCommit();
                                    blinkerEEPROMEnd();
                                }
                                else
                                {
                                    free(_AUTO[1]);
                                    _aCount--;
                                    blinkerEEPROMBegin(BLINKER_EEP_SIZE);
                                    EEPROM.put(BLINKER_EEP_ADDR_AUTONUM, _aCount);
                                    blinkerEEPROMCommit();
                                    blinkerEEPROMEnd();
                                }

                            }
                        }
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        void BlinkerApi::shareParse(const JsonObject& data)
        {
            if (data.containsKey(BLINKER_CMD_SET))
            {
                String value = data[BLINKER_CMD_SET];

                // DynamicJsonBuffer jsonBufferSet;
                // JsonObject& rootSet = jsonBufferSet.parseObject(value);
                JsonDocument jsonBuffer;
                DeserializationError error = deserializeJson(jsonBuffer, value);
                JsonObject rootSet = jsonBuffer.as<JsonObject>();

                // if (!rootSet.success())
                if (error)
                {
                    // BLINKER_ERR_LOG_ALL("Json error");
                    return;
                }

                if (rootSet.containsKey(BLINKER_CMD_SHARE))
                {
                    BLINKER_LOG_ALL(BLINKER_F("shareParse isParsed"));
                    _fresh = true;

                    // BProto::sharers(freshSharers());
                    String _shareData = freshSharers();
                    if (STRING_contains_string(_shareData, "users") == false)
                    {
                        _shareData = freshSharers();
                    }
                    if (STRING_contains_string(_shareData, "users") == true)
                    {
                        BProto::sharers(_shareData);
                    }
                    BProto::connect();
                }
            }
        }


        void BlinkerApi::numParse(const JsonObject& data)
        {
            if (data.containsKey(BLINKER_CMD_SET))
            {
                String value = data[BLINKER_CMD_SET];

                // DynamicJsonBuffer jsonBufferSet;
                // JsonObject& rootSet = jsonBufferSet.parseObject(value);
                JsonDocument jsonBuffer;
                DeserializationError error = deserializeJson(jsonBuffer, value);
                JsonObject rootSet = jsonBuffer.as<JsonObject>();

                // if (!rootSet.success())
                if (error)
                {
                    // BLINKER_ERR_LOG_ALL("Json error");
                    return;
                }

                if (rootSet.containsKey(BLINKER_CMD_AUTO_UPDATE_KEY))
                {
                    BLINKER_LOG_ALL(BLINKER_F("numParse isParsed"));
                    _fresh = true;

                    String _name_ = rootSet[BLINKER_CMD_AUTO_UPDATE_KEY];

                    char _name[16];

                    strcpy(_name, _name_.c_str());

                    int8_t num = checkNum(_name, _Widgets_num, _wCount_num);

                    if( num == BLINKER_OBJECT_NOT_AVAIL )
                    {
                        _Widgets_num[_wCount_num] = new BlinkerWidgets_num(_name);
                        _wCount_num++;
                    }
                    else
                    {
                        _Widgets_num[num]->setState(true);
                    }
                }
                else if (rootSet.containsKey(BLINKER_CMD_CANCEL_UPDATE_KEY))
                {
                    BLINKER_LOG_ALL(BLINKER_F("numParse2 isParsed"));
                    _fresh = true;

                    String _name_ = rootSet[BLINKER_CMD_AUTO_UPDATE_KEY];

                    char _name[16];

                    strcpy(_name, _name_.c_str());

                    int8_t num = checkNum(_name, _Widgets_num, _wCount_num);

                    if( num == BLINKER_OBJECT_NOT_AVAIL )
                    {
                        _Widgets_num[num]->setState(false);
                    }
                }
            }
        }





    // String BlinkerApi::postServer(const String & url, const String & host, int port, const String & msg)
    // {

    //     String client_msg;

    //     client_msg = BLINKER_F("POST ");
    //     client_msg += url;
    //     client_msg += BLINKER_F(" HTTP/1.1\r\nHost: ");
    //     client_msg += host;
    //     client_msg += BLINKER_F(":");
    //     client_msg += STRING_format(port);
    //     client_msg += BLINKER_F("\r\nContent-Type: application/json;charset=utf-8\r\nContent-Length: ");
    //     client_msg += STRING_format(msg.length());
    //     client_msg += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
    //     client_msg += msg;
    //     client_msg += BLINKER_F("\r\n");

    //     // client_s.print(client_msg);

    //     BLINKER_LOG_ALL(BLINKER_F("client_msg: "), client_msg);

    //     return client_msg;
    // }


    // String BlinkerApi::getServer(const String & url, const String & host, int port)
    // {
    //     String client_msg;

    //     client_msg = BLINKER_F("GET ");
    //     client_msg += url;
    //     client_msg += BLINKER_F(" HTTP/1.1\r\nHost: ");
    //     client_msg += host;
    //     client_msg += BLINKER_F(":");
    //     client_msg += STRING_format(port);
    //     client_msg += BLINKER_F("\r\nConnection: close\r\n\r\n");

    //     // client_s.print(client_msg);

    //     BLINKER_LOG_ALL(BLINKER_F("client_msg: "), client_msg);

    //     return client_msg;
    // }


    String BlinkerApi::blinkerServer(uint8_t _type, const String & msg, bool state)
    {
        // if (ESP.getFreeHeap() < 4000) return BLINKER_CMD_FALSE;

        switch (_type)
        {
            #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                    if (!checkCUPDATE()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    if (!checkCGET()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_CONFIG_DELETE_NUMBER :
                    if (!checkCDEL()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    // if (!checkDataUpdata()) {
                    //     return BLINKER_CMD_FALSE;
                    // }
                    break;
                case BLINKER_CMD_TIME_SLOT_DATA_NUMBER :
                    // if (!checkDataUpdata()) {
                    //     return BLINKER_CMD_FALSE;
                    // }
                    break;
                case BLINKER_CMD_TEXT_DATA_NUMBER :
                    // if (!checkDataUpdata()) {
                    //     return BLINKER_CMD_FALSE;
                    // }
                    break;
                case BLINKER_CMD_JSON_DATA_NUMBER :
                    // if (!checkDataUpdata()) {
                    //     return BLINKER_CMD_FALSE;
                    // }
                    break;
                case BLINKER_CMD_JSON_DATA_GET_NUMBER :
                    // if (!checkDataUpdata()) {
                    //     return BLINKER_CMD_FALSE;
                    // }
                    break;
                case BLINKER_CMD_DATA_GET_NUMBER :
                    if (!checkDataGet()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_DATA_DELETE_NUMBER :
                    if (!checkDataDel()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_AUTO_PULL_NUMBER :
                    if (!checkAutoPull()) {
                        return BLINKER_CMD_FALSE;
                    }
                    break;
                case BLINKER_CMD_FRESH_SHARERS_NUMBER :
                    break;
                case BLINKER_CMD_LOWPOWER_FREQ_GET_NUM :
                    break;
                case BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER :
                    break;
                case BLINKER_CMD_LOWPOWER_DATA_GET_NUM :
                    break;
                case BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER :
                    break;
                case BLINKER_CMD_EVENT_DATA_NUMBER :
                    break;
            #endif
            default :
                return BLINKER_CMD_FALSE;
        }

        BLINKER_LOG_ALL(BLINKER_F("message: "), msg);

        #if !defined(BLINKER_LAN_DEBUG)
            const int httpsPort = 443;
        #elif defined(BLINKER_LAN_DEBUG)
            const int httpsPort = 9090;
        #endif

        // #if 0
        //     #ifndef BLINKER_LAN_DEBUG
        //         String host = BLINKER_F(BLINKER_SERVER_HOST);
        //     #elif defined(BLINKER_LAN_DEBUG)
        //         String host = BLINKER_F("192.168.1.121");
        //     #endif
        //         String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");
        //         #ifndef BLINKER_LAN_DEBUG
        //             // #if defined(BLINKER_MQTT)
        //                 extern WiFiClientSecure client_mqtt;
        //             // #elif 0
        //             //     extern WiFiClientSecure client_pro;
        //             //     extern WiFiClientSecure client_mqtt_at;
        //             // #endif
        //             WiFiClientSecure *client_s;

        //             client_s = new WiFiClientSecure();
        //             // extern WiFiClientSecure client_mqtt;
        //         #elif defined(BLINKER_LAN_DEBUG)
        //             WiFiClient *client_s;
        //             client_s = new WiFiClient();
        //         #endif
        //     #endif

        //     // #if defined(BLINKER_WIFI)
        //         // WiFiClientSecure client_s;
        //     // #endif

        //     BLINKER_LOG_ALL(BLINKER_F("connecting to "), host);

        //     uint8_t connet_times = 0;

        //         client_mqtt.stop();
        //     // #elif 0
        //     //     client_pro.stop();
        //     //     client_mqtt_at.stop();
        //     #endif

        //     ::delay(100);

        //     bool mfln = client_s->probeMaxFragmentLength(host, httpsPort, 1024);
        //     if (mfln) {
        //         client_s->setBufferSizes(1024, 1024);
        //     }

        //     // client_s.setFingerprint(fingerprint.c_str());

        //     client_s->setInsecure();

        //     // while (1) {
        //         bool cl_connected = false;
        //         if (!client_s->connect(host, httpsPort)) {
        //     // #ifdef BLINKER_DEBUG_ALL
        //             BLINKER_ERR_LOG(BLINKER_F("server connection failed"));
        //     // #endif
        //             // return BLINKER_CMD_FALSE;

        //             // connet_times++;
        //             ::delay(1000);
        //         }
        //         else {
        //             BLINKER_LOG_ALL(BLINKER_F("connection succeed"));
        //             // return true;
        //             cl_connected = true;

        //             // break;
        //         }

        //         // if (connet_times >= 4 && !cl_connected)  return BLINKER_CMD_FALSE;
        //     // }

        //     String url;
        //     // String client_msg;

        //     switch (_type) {
        //             url = BLINKER_F("/api/v1/user/device/push");
        //             client_s->print(postServer(url, host, httpsPort, msg));
        //             break;
        //             url = BLINKER_F("/api/v1");
        //             url += msg;
        //             client_s->print(getServer(url, host, httpsPort));
        //             break;
        //             url = BLINKER_F("/api/v1");
        //             url += msg;
        //             client_s->print(getServer(url, host, httpsPort));
        //             break;
        //             case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device/userconfig");
        //                 client_s->print(postServer(url, host, httpsPort, msg));
        //                 break;
        //             case BLINKER_CMD_CONFIG_GET_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //             case BLINKER_CMD_CONFIG_DELETE_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //             case BLINKER_CMD_DATA_STORAGE_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device/cloudStorage/");
        //                 client_s->print(postServer(url, host, httpsPort, msg));
        //                 break;
        //             case BLINKER_CMD_DATA_GET_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //             case BLINKER_CMD_DATA_DELETE_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //             case BLINKER_CMD_AUTO_PULL_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //                 url = BLINKER_F("/api/v1/user/device/ota/upgrade_status");
        //                 // url += msg;
        //                 client_s->print(postServer(url, host, httpsPort, msg));
        //                 break;
        //             case BLINKER_CMD_FRESH_SHARERS_NUMBER :
        //                 url = BLINKER_F("/api/v1/user/device");
        //                 url += msg;
        //                 client_s->print(getServer(url, host, httpsPort));
        //                 break;
        //         #endif
        //         default :
        //             return BLINKER_CMD_FALSE;
        //     }

        //     unsigned long timeout = millis();
        //     while (client_s->available() == 0)
        //     {
        //         if (millis() - timeout > 5000)
        //         {
        //             BLINKER_LOG_ALL(BLINKER_F(">>> Client Timeout !"));
        //             client_s->stop();
        //             return BLINKER_CMD_FALSE;
        //         }
        //     }

        //     // Read all the lines of the reply from server and print them to Serial
        //     String _dataGet;
        //     String lastGet;
        //     String lengthOfJson;
        //     while (client_s->available())
        //     {
        //         // String line = client_s.readStringUntil('\r');
        //         _dataGet = client_s->readStringUntil('\n');

        //         if (_dataGet.startsWith("Content-Length: "))
        //         {
        //             int addr_start = _dataGet.indexOf(' ');
        //             int addr_end = _dataGet.indexOf('\0', addr_start + 1);
        //             lengthOfJson = _dataGet.substring(addr_start + 1, addr_end);
        //         }

        //         if (_dataGet == "\r")
        //         {
        //             BLINKER_LOG_ALL(BLINKER_F("headers received"));
        //             break;
        //         }
        //     }

        //     for(int i=0;i<lengthOfJson.toInt();i++)
        //     {
        //         lastGet += (char)client_s->read();
        //     }

        //     _dataGet = lastGet;

        //     BLINKER_LOG_ALL(BLINKER_F("_dataGet: "), _dataGet);

        //     client_s->stop();
        //     client_s->flush();

        //     free(client_s);

        //     // client_mqtt.setInsecure();

        //     DynamicJsonBuffer jsonBuffer;
        //     JsonObject& data_rp = jsonBuffer.parseObject(_dataGet);

        //     if (data_rp.success())
        //     {
        //         uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
        //         if (msg_code != 1000)
        //         {
        //             String _detail = data_rp[BLINKER_CMD_DETAIL];
        //             BLINKER_ERR_LOG(_detail);
        //         }
        //         else
        //         {
        //             // String _dataGet_ = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
        //             if (_type == BLINKER_CMD_FRESH_SHARERS_NUMBER)
        //                 _dataGet = data_rp[BLINKER_CMD_DETAIL].as<String>();
        //             else
        //                 _dataGet = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA].as<String>();
        //         }
        //     }

        //     BLINKER_LOG_ALL(BLINKER_F("_dataGet: "), _dataGet);

        //     switch (_type)
        //     {
        //             _pushTime = millis();
        //             break;
        //             case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
        //                 _cUpdateTime = millis();
        //                 break;
        //             case BLINKER_CMD_CONFIG_GET_NUMBER :
        //                 _cGetTime = millis();
        //                 break;
        //             case BLINKER_CMD_CONFIG_DELETE_NUMBER :
        //                 _cDelTime = millis();
        //                 break;
        //             case BLINKER_CMD_DATA_STORAGE_NUMBER :
        //                 _dUpdateTime = millis();
        //                 break;
        //             case BLINKER_CMD_DATA_GET_NUMBER :
        //                 _dGetTime = millis();
        //                 break;
        //             case BLINKER_CMD_DATA_DELETE_NUMBER :
        //                 _dDelTime = millis();
        //                 break;
        //             case BLINKER_CMD_AUTO_PULL_NUMBER :
        //                 _autoPullTime = millis();
        //                 break;
        //                 break;
        //                 break;
        //             case BLINKER_CMD_FRESH_SHARERS_NUMBER :
        //                 break;
        //         #endif
        //         default :
        //             return BLINKER_CMD_FALSE;
        //     }

        //     return _dataGet;
        // #elif defined(ESP32)
            #if !defined(BLINKER_LAN_DEBUG)
                String host = BLINKER_F(BLINKER_SERVER_HTTPS);
            #elif defined(BLINKER_LAN_DEBUG)
                String host = BLINKER_F("http://192.168.0.105:8887");
            #endif

            // const char* ca =
            //     "-----BEGIN CERTIFICATE-----\n"
            //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n"
            //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n"
            //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n"
            //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n"
            //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
            //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n"
            //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n"
            //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n"
            //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n"
            //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n"
            //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n"
            //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n"
            //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n"
            //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n"
            //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n"
            //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n"
            //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n"
            //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n"
            //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n"
            //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n"
            //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n"
            //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n"
            //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n"
            //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n"
            //     "9UBd0Q==\n"
            //     "-----END CERTIFICATE-----\n";
        // #endif


            HTTPClient http;

            String url_iot;

            int httpCode;

            String conType = BLINKER_F("Content-Type");
            String application = BLINKER_F("application/json;charset=utf-8");

            BLINKER_LOG_ALL(BLINKER_F("blinker server begin"));
            BLINKER_LOG_FreeHeap_ALL();

            switch (_type) {
                    // return BLINKER_CMD_FALSE;
                #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                    case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device/cloud_storage/object");

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_CONFIG_GET_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_CONFIG_DELETE_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_DATA_STORAGE_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device/cloudStorage/");

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_TIME_SLOT_DATA_NUMBER :
                        // url_iot = host;
                        #if !defined(BLINKER_WITHOUT_SSL)
                            url_iot = BLINKER_STORAGE_HTTPS;
                            url_iot += BLINKER_F("/api/v1/storage/ts");
                        #else
                            url_iot = BLINKER_STORAGE_HTTPS;
                            url_iot += BLINKER_F("/api/v1/storage/ts");
                        #endif


                        // url_iot = host;
                        // url_iot += BLINKER_F("/api/v1/user/device/cloud_storage/object");

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_TEXT_DATA_NUMBER :
                        // url_iot = host;
                        #if !defined(BLINKER_WITHOUT_SSL)
                            url_iot = BLINKER_STORAGE_HTTPS;
                            url_iot += BLINKER_F("/api/v1/storage/tt");
                        #else
                            url_iot = BLINKER_STORAGE_HTTPS;
                            url_iot += BLINKER_F("/api/v1/storage/tt");
                        #endif

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_JSON_DATA_NUMBER :
                        // url_iot = host;
                        #if !defined(BLINKER_WITHOUT_SSL)
                            url_iot = BLINKER_STORAGE_HTTPS;
                            url_iot += BLINKER_F("/api/v1/storage/ot");
                        #else
                            url_iot = BLINKER_STORAGE_HTTPS;
                            url_iot += BLINKER_F("/api/v1/storage/ot");
                        #endif

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_DATA_GET_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_DATA_DELETE_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_AUTO_PULL_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_FRESH_SHARERS_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_LOWPOWER_FREQ_GET_NUM :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_LOWPOWER_DATA_GET_NUM :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                    case BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device/lowpower/data");

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_EVENT_DATA_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device/event");

                            http.begin(url_iot);

                        http.addHeader(conType, application);
                        httpCode = http.POST(msg);
                        break;
                    case BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER :
                        url_iot = host;
                        url_iot += BLINKER_F("/api/v1/user/device");
                        url_iot += msg;

                            http.begin(url_iot);

                        httpCode = http.GET();
                        break;
                #endif
                default :
                    return BLINKER_CMD_FALSE;
            }

            BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

            // http.begin(url_iot, ca); TODO
            // http.begin(url_iot);

            // http.addHeader("Content-Type", "application/json");

            // httpCode = http.POST(msg);

            BLINKER_LOG_ALL(BLINKER_F("HTTPS payload: "), msg);

            if (httpCode > 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("[HTTP] status... code: "), httpCode);

                String payload;
                if (httpCode == HTTP_CODE_OK) {
                    payload = http.getString();

                    BLINKER_LOG_ALL(payload);

                    // DynamicJsonBuffer jsonBuffer;
                    // JsonObject& data_rp = jsonBuffer.parseObject(payload);
                    JsonDocument jsonBuffer;
                    DeserializationError error = deserializeJson(jsonBuffer, payload);
                    JsonObject data_rp = jsonBuffer.as<JsonObject>();

                    // if (data_rp.success())
                    if (!error)
                    {
                        uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                        if (msg_code != 1000)
                        {
                            String _detail = data_rp[BLINKER_CMD_DETAIL];
                            BLINKER_ERR_LOG(_detail);
                        }
                        else
                        {
                            BLINKER_LOG_ALL(BLINKER_F("_type: "), _type);
                            // String _payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA];
                            // payload = _payload;

                            if (_type == BLINKER_CMD_FRESH_SHARERS_NUMBER)
                                payload = data_rp[BLINKER_CMD_DETAIL].as<String>();
                            else if (_type == BLINKER_CMD_LOWPOWER_FREQ_GET_NUM)
                                payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_FREQ].as<String>();
                            else if (_type == BLINKER_CMD_CONFIG_GET_NUMBER)
                                payload = data_rp[BLINKER_CMD_DETAIL].as<String>();
                            else
                                payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA].as<String>();
                        }
                    }

                    BLINKER_LOG_ALL(BLINKER_F("payload: "), payload);

                    switch (_type) {
                        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                            case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                                _cUpdateTime = millis();
                                break;
                            case BLINKER_CMD_CONFIG_GET_NUMBER :
                                _cGetTime = millis();
                                if (_configGetFunc) _configGetFunc(payload);
                                break;
                            case BLINKER_CMD_CONFIG_DELETE_NUMBER :
                                _cDelTime = millis();
                                break;
                            case BLINKER_CMD_DATA_STORAGE_NUMBER :
                                _dUpdateTime = millis();
                                break;
                            case BLINKER_CMD_TIME_SLOT_DATA_NUMBER :
                                _dUpdateTime = millis();
                                break;
                            case BLINKER_CMD_TEXT_DATA_NUMBER :
                                _dUpdateTime = millis();
                                break;
                            case BLINKER_CMD_JSON_DATA_NUMBER :
                                _dUpdateTime = millis();
                                break;
                            case BLINKER_CMD_JSON_DATA_GET_NUMBER :
                                // _dUpdateTime = millis();
                                break;
                            case BLINKER_CMD_DATA_GET_NUMBER :
                                _dGetTime = millis();
                                if (_dataGetFunc) _dataGetFunc(payload);
                                break;
                            case BLINKER_CMD_DATA_DELETE_NUMBER :
                                _dDelTime = millis();
                                break;
                            case BLINKER_CMD_AUTO_PULL_NUMBER :
                                _autoPullTime = millis();
                                break;
                            case BLINKER_CMD_FRESH_SHARERS_NUMBER :
                                break;
                            case BLINKER_CMD_LOWPOWER_FREQ_GET_NUM :
                                break;
                            case BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER :
                                break;
                            case BLINKER_CMD_LOWPOWER_DATA_GET_NUM :
                                break;
                            case BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER :
                                break;
                            case BLINKER_CMD_EVENT_DATA_NUMBER :
                                break;
                        #endif
                        default :
                            return BLINKER_CMD_FALSE;
                    }
                }

                http.end();

                return payload;
            }
            else {
                BLINKER_LOG_ALL(BLINKER_F("[HTTP] ... failed, error: "), http.errorToString(httpCode).c_str());
                String payload = http.getString();
                BLINKER_LOG_ALL(payload);

                http.end();
                return BLINKER_CMD_FALSE;
            }
        // #endif
    }



#endif







#endif
