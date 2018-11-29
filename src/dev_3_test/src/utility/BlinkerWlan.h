#ifndef BLINKER_WLAN_H
#define BLINKER_WLAN_H

#if defined(ESP8266) || defined(ESP32)

// #if defined(BLINKER_PRO)

// #include "Blinker/BlinkerConfig.h"
// #include "modules/ArduinoJson/ArduinoJson.h"
// #if defined(ESP8266)
//     #include <ESP8266WiFi.h>
// #elif defined(ESP32)
//     #include <WiFi.h>
// #endif
// #include <EEPROM.h>

// static WiFiServer *_server;
// static WiFiClient _client;
// static IPAddress apIP(192, 168, 4, 1);
// #if defined(ESP8266)
// static IPAddress netMsk(255, 255, 255, 0);
// #endif

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

enum bwl_status_t
{
    BWL_CONFIG_CKECK,
    BWL_CONFIG_FAIL,
    BWL_CONFIG_SUCCESS,
    BWL_CONNECTING,
    BWL_CONNECTED,
    BWL_DISCONNECTED,
    BWL_SMARTCONFIG_BEGIN,
    BWL_SMARTCONFIG_DONE,
    BWL_SMARTCONFIG_TIMEOUT,
    BWL_STACONFIG_BEGIN,
    BWL_APCONFIG_BEGIN,
    BWL_APCONFIG_DONE,
    BWL_APCONFIG_TIMEOUT,
    BWL_CONNECTED_CHECK,
    BWL_RESET
};

class BlinkerWlan
{
    public :
        BlinkerWlan()
            : _status(BWL_CONFIG_CKECK)
        {}

        bool checkConfig();
        void loadConfig(char *_ssid, char *_pswd);
        void saveConfig(char *_ssid, char *_pswd);
        void deleteConfig();
        void smartconfigBegin(uint16_t _time = 15000);
        bool smartconfigDone();
        void connect();
        bool connected();
        void disconnect();
        void reset();
        bool run();
        bwl_status_t status() { return _status; }

        void setType(const char* _type) {
            _deviceType = _type;

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG(("API deviceType: "), _type);
#endif
        }

        void softAPinit();
        void serverClient();
        void parseUrl(String data);
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);

        // uint8_t status() { return _status; }

    private :

    protected :
        char *SSID;
        char *PSWD;
        const char* _deviceType;
        uint32_t connectTime;
        uint16_t timeout;
        bwl_status_t _status;
        uint32_t debugStatusTime;
};

// #endif
#endif

#endif
