#ifndef BlinkerSimplerESP32_H
#define BlinkerSimplerESP32_H

#include <Adapters/BlinkerArduinoWS.h>
#include <ESPmDNS.h>
#include <WiFi.h>

class BlinkerSimpleESP32_WS 
    : public BlinkerProtocol<BlinkerArduinoWS>
{
    typedef BlinkerProtocol<BlinkerArduinoWS> Base;

    public :
        BlinkerSimpleESP32_WS(BlinkerArduinoWS &transp)
            : Base(transp)
        {}

        void begin() {
            Base::begin();
            smartconfig();
            BLINKER_LOG1("ESP8266_WiFi Initialled...");
        }
    
        void begin(const char* ssid,
                    const char* pswd)
        {
            Base::begin();
            connectWiFi(ssid, pswd);
            BLINKER_LOG1("ESP32_WiFi Initialled...");
        }
    
    private :
        void smartconfig() {
            WiFi.mode(WIFI_AP_STA);
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
        }

        void mDNSInit()
        {
            if (!MDNS.begin(macDeviceName().c_str())) {
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
};

static BlinkerArduinoWS  _blinkerTransport;
BlinkerSimpleESP32_WS Blinker(_blinkerTransport);

#endif
