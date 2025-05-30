#ifndef BLINKER_WIFI_H
#define BLINKER_WIFI_H

#if defined(ESP32)

#if defined(BLINKER_ESPTOUCH_V2)
    #define BLINKER_ESP_SMARTCONFIG_V2
#endif

#include "Functions/BlinkerWiFiESP.h"
#include "Blinker/BlinkerProtocol.h"
#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerWiFi: public BlinkerProtocol<BlinkerWiFiESP>
{
    typedef BlinkerProtocol<BlinkerWiFiESP> Base;

    public :
        BlinkerWiFi(BlinkerWiFiESP &transp) : Base(transp) {}

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

    #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_APCONFIG)
        void begin( const char* _auth)
        {
            Base::begin();
            this->conn.begin(_auth);

        #if defined(BLINKER_ESP_SMARTCONFIG)
            this->conn.smartconfigBegin();
        #elif defined(BLINKER_APCONFIG)
            this->conn.apconfigBegin();
        #endif
        
        #if defined(BLINKER_WIDGET)
            Base::loadTimer();
        #endif
        }
    #endif

    #if defined(BLINKER_ESP_SMARTCONFIG_V2)
        void begin()
        {
            Base::begin();
            // this->conn.begin();

            this->conn.smartconfigBegin();
        
        #if defined(BLINKER_WIDGET)
            Base::loadTimer();
        #endif
        }
    #endif

    #if defined(BLINKER_WIFI_MULTI)
        void addAP( const char* _ssid, 
                    const char* _pswd)
        {
            BLINKER_LOG(BLINKER_F("wifiMulti add "), _ssid);
            wifiMulti.addAP(_ssid, _pswd);
        }
    
        void existAP(   const char* _ssid, 
                        const char* _pswd)
        {
            BLINKER_LOG(BLINKER_F("wifiMulti existAP "), _ssid);
            wifiMulti.existsAP(_ssid, _pswd);
        }
    
        void cleanAPlist()
        {
            wifiMulti.cleanAPlist();
        }
    #endif

    private :

};

#elif defined(ARDUINO_ARCH_RENESAS_UNO)

#include "Functions/BlinkerWiFiUNO.h"
#include "Blinker/BlinkerProtocol.h"
#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerWiFi: public BlinkerProtocol<BlinkerWiFiUNO>
{
    typedef BlinkerProtocol<BlinkerWiFiUNO> Base;

    public :
        BlinkerWiFi(BlinkerWiFiUNO &transp) : Base(transp) {}

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
};

#else
    #error This code is intended to run on the ESP32 or Renesas UNO R4 platform! Please check your Tools->Board setting.
#endif

#endif
