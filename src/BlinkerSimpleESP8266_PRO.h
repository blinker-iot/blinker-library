#ifndef BlinkerSimplerESP8266_PRO_H
#define BlinkerSimplerESP8266_PRO_H

#include "Adapters/BlinkerPRO.h"

class BlinkerSimpleESP8266_PRO 
    : public BlinkerProtocol<BlinkerPRO>
{
    typedef BlinkerProtocol<BlinkerPRO> Base;

    public : 
        BlinkerSimpleESP8266_PRO(BlinkerPRO &transp)
            : Base(transp)
        {}

        void begin(const char* _type = BLINKER_AIR_DETECTOR) {
            Base::begin(_type);
            // while(!Base::beginPro()) {
            //     ::delay(1);
            // }
            // this->conn.begin(_type);
            BLINKER_LOG1("ESP8266_PRO Initialled...");
        }

// #if defined(BLINKER_ESP_SMARTCONFIG)
//         void begin(const char* _auth) {
//             Base::begin(_auth);
//             smartconfig();
//             this->conn.begin(_auth);
//             BLINKER_LOG1("ESP8266_MQTT Initialled...");
//         }
// #endif

//         void begin( const char* _auth,
//                     const char* _ssid,
//                     const char* _pswd)
//         {
//             Base::begin(_auth);
//             connectWiFi(_ssid, _pswd);
//             this->conn.begin(_auth);
//             BLINKER_LOG1("ESP8266_MQTT Initialled...");
//         }
    
//     private :
// #if defined(BLINKER_ESP_SMARTCONFIG)
//         void smartconfig() {
//             WiFi.mode(WIFI_STA);
//             WiFi.beginSmartConfig();
            
//             BLINKER_LOG1("Waiting for SmartConfig.");
//             while (!WiFi.smartConfigDone()) {
//                 ::delay(500);
//             }

//             BLINKER_LOG1("SmartConfig received.");
            
//             BLINKER_LOG1("Waiting for WiFi");
//             while (WiFi.status() != WL_CONNECTED) {
//                 ::delay(500);
//             }

//             BLINKER_LOG1("WiFi Connected.");

//             BLINKER_LOG1("IP Address: ");
//             BLINKER_LOG1(WiFi.localIP());

//             // mDNSInit();
//         }
// #endif
//         // void mDNSInit()
//         // {
//         //     if (!MDNS.begin(macDeviceName().c_str(), WiFi.localIP())) {
//         //         while(1) {
//         //             ::delay(100);
//         //         }
//         //     }

//         //     BLINKER_LOG1(("mDNS responder started"));
            
//         //     MDNS.addService("DiyArduino", "tcp", WS_SERVERPORT);

//         //     this->conn.begin(macDeviceName());
//         // }

//         void connectWiFi(const char* _ssid, const char* _pswd)
//         {
//             uint32_t connectTime = millis();

//             BLINKER_LOG2(("Connecting to "), _ssid);
//             WiFi.mode(WIFI_STA);
//             if (_pswd && strlen(_pswd)) {
//                 WiFi.begin(_ssid, _pswd);
//             }
//             else {
//                 WiFi.begin(_ssid);
//             }

//             while (WiFi.status() != WL_CONNECTED) {
//                 ::delay(50);

//                 if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
//                     connectTime = millis();
//                     BLINKER_LOG1(("WiFi connect timeout, please check ssid and pswd!"));
//                     BLINKER_LOG1(("Retring WiFi connect again!"));
//                 }
//             }
//             BLINKER_LOG1(("Connected"));

//             IPAddress myip = WiFi.localIP();
//             BLINKER_LOG2(("Your IP is: "), myip);

//             // mDNSInit();
//         }
};

static BlinkerPRO  _blinkerTransport;
BlinkerSimpleESP8266_PRO Blinker(_blinkerTransport);

#endif