#ifndef BLINKER_WIFI_H
#define BLINKER_WIFI_H

#if !defined(BLINKER_WIFI)
    #error "BLINKER_WIFI must be defined to use WiFi functionality!"
#endif

#if defined(ESP32)

#if defined(BLINKER_ESPTOUCH_V2) || !defined(BLINKER_DEFAULT_CONFIG)
    #define BLINKER_ESP_SMARTCONFIG_V2
#endif

#include "Functions/BlinkerWiFiESP.h"
#include "Blinker/BlinkerProtocol.h"
#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerWiFi: public BlinkerProtocol<BlinkerWiFiESP>
{
    typedef BlinkerProtocol<BlinkerWiFiESP> Base;

    private:
        static BlinkerWiFi* instance;
        BlinkerWiFiESP* transport;

        BlinkerWiFi(BlinkerWiFiESP &transp) : Base(transp), transport(&transp) {}

        BlinkerWiFi(const BlinkerWiFi&) = delete;
        BlinkerWiFi& operator=(const BlinkerWiFi&) = delete;

    public:
        static BlinkerWiFi& getInstance(BlinkerWiFiESP &transp) 
        {
            if (instance == nullptr) {
                instance = new BlinkerWiFi(transp);
            }
            return *instance;
        }

        static BlinkerWiFi& getInstance() 
        {
            if (instance == nullptr) {
            }
            return *instance;
        }

        void begin( const char* _auth, 
                    const char* _ssid, 
                    const char* _pswd)
        {
            Base::begin();
            this->conn.begin(_auth);

        #if defined(BLINKER_WIFI_MULTI)
            this->conn.multiBegin(_ssid, _pswd);
        #else
            this->conn.commonBegin(_ssid, _pswd);
        #endif

        #if defined(BLINKER_WIDGET)
            Base::loadTimer();
        #endif
        }

    #if defined(BLINKER_ESP_SMARTCONFIG_V2)
        void begin()
        {
            Base::begin();
            this->conn.smartconfigBegin();
        
        #if defined(BLINKER_WIDGET)
            Base::loadTimer();
        #endif
        }
    #endif

        ~BlinkerWiFi() 
        {
            instance = nullptr;
        }
};

BlinkerWiFi* BlinkerWiFi::instance = nullptr;

#elif defined(ARDUINO_UNOR4_WIFI)

#include "Functions/BlinkerWiFiUNO.h"
#include "Blinker/BlinkerProtocol.h"
#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerWiFi: public BlinkerProtocol<BlinkerWiFiUNO>
{
    typedef BlinkerProtocol<BlinkerWiFiUNO> Base;

    private:
        static BlinkerWiFi* instance;
        BlinkerWiFiUNO* transport;

        BlinkerWiFi(BlinkerWiFiUNO &transp) : Base(transp), transport(&transp) {}

        BlinkerWiFi(const BlinkerWiFi&) = delete;
        BlinkerWiFi& operator=(const BlinkerWiFi&) = delete;

    public:
        static BlinkerWiFi& getInstance(BlinkerWiFiUNO &transp) 
        {
            if (instance == nullptr) {
                instance = new BlinkerWiFi(transp);
            }
            return *instance;
        }

        static BlinkerWiFi& getInstance() 
        {
            if (instance == nullptr) {
            }
            return *instance;
        }

        void begin( const char* _auth, 
                    const char* _ssid, 
                    const char* _pswd)
        {
            Base::begin();
            this->conn.begin(_auth);
            this->conn.commonBegin(_ssid, _pswd);

        #if defined(BLINKER_WIDGET)
            // Base::loadTimer();
        #endif
        }

        ~BlinkerWiFi() 
        {
            instance = nullptr;
        }
};

BlinkerWiFi* BlinkerWiFi::instance = nullptr;

#else
    #error "WiFi is not supported on this platform! Supported platforms: ESP32, Arduino UNO R4 WiFi"
#endif

#endif
