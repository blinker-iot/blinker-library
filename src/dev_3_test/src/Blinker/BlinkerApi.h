#ifndef BLINKER_API_H
#define BLINKER_API_H

#include <time.h>
#if defined(ESP8266) || defined(ESP32)
    #include <Ticker.h>
    #include <EEPROM.h>
#endif

#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
#endif

#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerConfig.h"
#include "utility/BlinkerUtility.h"

#if defined(BLINKER_ARDUINOJSON)
    #include "modules/ArduinoJson/ArduinoJson.h"
#endif

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

template <class Proto>
class BlinkerApi
{
    public :

        // void hello();

    private :
        bool        _isNTPInit = false;
        float       _timezone = 8.0;
        uint32_t    _ntpStart;
        // time_t      now_ntp;
        // struct tm   timeinfo;

        void freshNTP();

    protected :
        bool ntpInit();
        void ntpConfig();
};

template <class Proto>
void BlinkerApi<Proto>:: freshNTP() {
    if (_isNTPInit) {
        time_t now_ntp = ::time(nullptr);
        struct tm timeinfo;
        #if defined(ESP8266)
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif
    }
}

template <class Proto>
bool BlinkerApi<Proto>::ntpInit()
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

        time_t now_ntp = ::time(nullptr);

        float _com_timezone = abs(_timezone);
        if (_com_timezone < 1.0) _com_timezone = 1.0;

        if (now_ntp < _com_timezone * 3600 * 12)
        {
            ntpConfig();
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
            gmtime_r(&now_ntp, &timeinfo);
        #elif defined(ESP32)
            localtime_r(&now_ntp, &timeinfo);
        #endif

        BLINKER_LOG_ALL(BLINKER_F("Current time: "), asctime(&timeinfo));
        
        _isNTPInit = true;

        // loadTiming();
    }

    return true;
}

template <class Proto>
void BlinkerApi<Proto>::ntpConfig()
{
    String ntp1 = BLINKER_F("ntp1.aliyun.com");
    String ntp2 = BLINKER_F("210.72.145.44");
    String ntp3 = BLINKER_F("time.pool.aliyun.com");

    configTime((long)(_timezone * 3600), 0, \
            ntp1.c_str(), ntp2.c_str(), ntp3.c_str());
}

// template <class Proto>
// void BlinkerApi<Proto>::hello()
// {
//     BLINKER_LOG("BlinkerApi Hello");
// }

#endif
