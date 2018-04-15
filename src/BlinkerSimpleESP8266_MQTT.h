#ifndef BlinkerSimplerESP8266_MQTT_H
#define BlinkerSimplerESP8266_MQTT_H

#include "Adapters/BlinkerMQTT.h"
// #include <ESP8266WiFi.h>

class BlinkerSimpleESP8266_MQTT 
    : public BlinkerProtocol<BlinkerMQTT>
{
    typedef BlinkerProtocol<BlinkerMQTT> Base;

    public : 
        BlinkerSimpleESP8266_MQTT(BlinkerMQTT &transp)
            : Base(transp)
        {}

#if defined(BLINKER_ESP_SMARTCONFIG)
        void begin(const char* auth) {
            Base::begin();
            smartconfig();
            this->conn.begin(auth);
            BLINKER_LOG1("ESP8266_WiFi Initialled...");
        }
#endif

        void begin( const char* auth,
                    const char* ssid,
                    const char* pswd)
        {
            Base::begin();
            connectWiFi(ssid, pswd);
            this->conn.begin(auth);
            BLINKER_LOG1("ESP8266_WiFi Initialled...");
        }
    
    private :
        #if defined(BLINKER_ESP_SMARTCONFIG)
        void smartconfig() {
            WiFi.mode(WIFI_STA);
            WiFi.beginSmartConfig();
            
            BLINKER_LOG1("Waiting for SmartConfig.");
            while (!WiFi.smartConfigDone()) {
                delay(500);
            }

            BLINKER_LOG1("SmartConfig received.");
            
            BLINKER_LOG1("Waiting for WiFi");
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
            }

            BLINKER_LOG1("WiFi Connected.");

            BLINKER_LOG1("IP Address: ");
            BLINKER_LOG1(WiFi.localIP());

            // mDNSInit();
        }
#endif
        // void mDNSInit()
        // {
        //     if (!MDNS.begin(macDeviceName().c_str(), WiFi.localIP())) {
        //         while(1) {
        //             ::delay(100);
        //         }
        //     }

        //     BLINKER_LOG1(("mDNS responder started"));
            
        //     MDNS.addService("DiyArduino", "tcp", WS_SERVERPORT);

        //     this->conn.begin(macDeviceName());
        // }

        void connectWiFi(const char* ssid, const char* pswd)
        {
            uint32_t connectTime = millis();

            BLINKER_LOG2(("Connecting to "), ssid);
            WiFi.mode(WIFI_STA);
            if (pswd && strlen(pswd)) {
                WiFi.begin(ssid, pswd);
            }
            else {
                WiFi.begin(ssid);
            }

            while (WiFi.status() != WL_CONNECTED) {
                ::delay(50);

                if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                    connectTime = millis();
                    BLINKER_LOG1(("WiFi connect timeout, please check ssid and pswd!"));
                    BLINKER_LOG1(("Retring WiFi connect again!"));
                }
            }
            BLINKER_LOG1(("Connected"));

            IPAddress myip = WiFi.localIP();
            BLINKER_LOG2(("Your IP is: "), myip);

            // mDNSInit();
        }
};

static BlinkerMQTT  _blinkerTransport;
BlinkerSimpleESP8266_MQTT Blinker(_blinkerTransport);

#endif