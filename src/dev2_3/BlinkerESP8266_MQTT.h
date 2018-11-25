#ifndef BlinkerESP8266_MQTT_H
#define BlinkerESP8266_MQTT_H

// #if ARDUINO >= 100
//     #include <Arduino.h>
// #else
//     #include <WProgram.h>
// #endif
// #define BLINKER_ESP_SMARTCONFIG

#include "Adapters/BlinkerMQTT.h"
#include "Blinker/BlinkerProtocol.h"
// #include "utility/BlinkerDebug.h"

class BlinkerESP8266_MQTT
    : public BlinkerProtocol<BlinkerMQTT>
{
    typedef BlinkerProtocol<BlinkerMQTT> Base;

    public : 
        BlinkerESP8266_MQTT(BlinkerMQTT &transp)
            : Base(transp)
        {}
        // BlinkerESP8266_MQTT() {}

    // #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_APCONFIG)
        void begin(const char* _auth)
        {
            Base::begin(_auth);
        // #if defined(BLINKER_ESP_SMARTCONFIG)
            if (!autoInit()) smartconfig();
        // #elif defined(BLINKER_APCONFIG)
        //     if (!autoInit())
        //     {
        //         softAPinit();
        //         while(WiFi.status() != WL_CONNECTED)
        //         {
        //             serverClient();
        //             ::delay(10);
        //         }
        //     }
        // #endif

            // this->conn.begin(_auth);
            // strcpy(Base::_deviceName, this->conn.deviceName().c_str());

            // BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
        }
    // #else
    //     void begin( const char* _auth, const char* _ssid, const char* _pswd);
    // #endif

    private :
    // #if defined(BLINKER_APCONFIG)
    //     void softAPinit();
    //     void serverClient();
    //     bool parseUrl(String data);
    // #endif

    // #if defined(BLINKER_ESP_SMARTCONFIG)
        void smartconfig();
    // #endif

        bool autoInit();
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);

};

// extern BlinkerESP8266_MQTT Blinker;
// extern BlinkerMQTT  Blinker_MQTT;
// extern BlinkerESP8266_MQTT Blinker(Blinker_MQTT);

static BlinkerMQTT  _blinkerTransport;
BlinkerESP8266_MQTT Blinker(_blinkerTransport);

#endif
