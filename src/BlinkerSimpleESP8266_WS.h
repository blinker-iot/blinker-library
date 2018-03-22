#ifndef BlinkerSimplerESP8266_H
#define BlinkerSimplerESP8266_H

#include <Adapters/BlinkerArduinoWS.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>

class BlinkerSimpleESP8266_WS 
    : public BlinkerProtocol<BlinkerArduinoWS>
{
    typedef BlinkerProtocol<BlinkerArduinoWS> Base;

    public :
        
        BlinkerSimpleESP8266_WS(BlinkerArduinoWS &transp)
            : Base(transp)
        {}

        void mDNSInit()
        {
            if (!MDNS.begin(macDeviceName().c_str(), WiFi.localIP())) {
                while(1) {
                    ::delay(100);
                }
            }

            BLINKER_LOG1(BLINKER_F("mDNS responder started"));
            
            MDNS.addService("DiyArduino", "tcp", WS_SERVERPORT);

            this->conn.begin(macDeviceName());
        }

        void connectWiFi(const char* ssid, const char* pswd)
        {
            uint32_t connectTime = millis();

            BLINKER_LOG2(BLINKER_F("Connecting to "), ssid);
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
                    BLINKER_LOG1(BLINKER_F("Connect timeout"));
                }
            }
            BLINKER_LOG1(BLINKER_F("Connected"));

            IPAddress myip = WiFi.localIP();
            BLINKER_LOG2(BLINKER_F("Your IP is: "), myip);

            mDNSInit();
        }
    
        void  begin(const char* ssid,
                    const char* pswd)
        {
            Base::begin();
            connectWiFi(ssid, pswd);
            BLINKER_LOG1("ESP8266_WiFi Initialled...");
        }
    
    // private :
};

static BlinkerArduinoWS  _blinkerTransport;
BlinkerSimpleESP8266_WS Blinker(_blinkerTransport);

#endif