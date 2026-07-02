#ifndef BLINKER_ESP_MQTT_H
#define BLINKER_ESP_MQTT_H

#include "Blinker/BlinkerWiFiPlatform.h"

#if defined(BLINKER_NATIVE_WIFI)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#if !defined(ESP32) && (defined(BLINKER_ESP_SMARTCONFIG) || \
    defined(BLINKER_ESP_SMARTCONFIG_V2) || \
    defined(BLINKER_BLEFI) || \
    defined(BLINKER_WIFI_MULTI))
    #error SMARTCONFIG/BLEFI/WIFI_MULTI currently require ESP32. Use Blinker.begin(auth, ssid, pswd) on Arduino UNO R4 WiFi and Raspberry Pi Pico W.
#endif

#include "Adapters/BlinkerMQTT.h"
#include "Blinker/BlinkerApi.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "modules/ArduinoJson/ArduinoJson.h"
#endif

typedef BlinkerApi BApi;

class BlinkerESPMQTT : public BlinkerApi
{
    public : 
    #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_ESP_SMARTCONFIG_V2) || defined(BLINKER_BLEFI)
        void begin(const char* _auth)
        {            BApi::begin();

            Transp.begin(_auth);

            transport(Transp);

            #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_ESP_SMARTCONFIG_V2)
                Transp.smartconfigBegin();
            #elif defined(BLINKER_BLEFI)
                Transp.bleFiBegin();
            #endif

            BApi::loadTimer();

            // #if defined(BLINKER_ESP_SMARTCONFIG)
            // #endif

            // __auth = _auth;

            // #ifndef BLINKER_ESP_TASK
            //     beginMQTT();
            // #endif
        }
    #else
        void begin( const char* _auth, 
                    const char* _ssid, 
                    const char* _pswd )
        {            BApi::begin();

            Transp.begin(_auth);

            transport(Transp);

            #if defined(BLINKER_WIFI_MULTI)
                Transp.multiBegin(_ssid, _pswd);
            #else
                Transp.commonBegin(_ssid, _pswd);
            #endif
            BApi::loadTimer();
            // __auth = _auth;
            // __ssid = _ssid;
            // __pswd = _pswd;

            // #ifndef BLINKER_ESP_TASK
            //     beginMQTT();
            // #endif
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
        // void beginMQTT()
        // {
        //     #else
        //     #endif

        //     #else
        //     #endif

        //     #if defined(BLINKER_ESP_SMARTCONFIG)
        //     #else
        //     #endif
        // }

    private :
        // void commonBegin(const char* _auth, 
        //                 const char* _ssid, 
        //                 const char* _pswd,

        // bool autoInit();
        // void smartconfig();
        // // void serverClient();
        
        // void connectWiFi(String _ssid, String _pswd);
        // void connectWiFi(const char* _ssid, const char* _pswd);

        // const char* __auth;
        // const char* __ssid;
        // const char* __pswd;

        BlinkerMQTT Transp;
};

// void BlinkerESPMQTT::commonBegin(const char* _auth,
//                                 const char* _ssid,
//                                 const char* _pswd,
// {
//     BApi::begin();
//     connectWiFi(_ssid, _pswd);
//     Transp.begin(_auth);
//     transport(Transp);
//     BApi::loadTimer();

//     #if 0
//     #elif defined(ESP32)
//         BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
//     #endif
// }

// {
//     BApi::begin();
//     if (!autoInit()) smartconfig();
//     Transp.begin(_auth);
//     transport(Transp);
//     BApi::loadTimer();

//     #if 0
//     #elif defined(ESP32)
//         BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
//     #endif
// }

// {
//     BApi::begin();
//     if (!autoInit())
//     {
//         // while(WiFi.status() != WL_CONNECTED)
//         // {
//         //     serverClient();
//         //     ::delay(10);
//         // }
//     }

//     Transp.begin(_auth);
//     transport(Transp);
//     BApi::loadTimer();

//     #if 0
//     #elif defined(ESP32)
//         BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
//     #endif
// }

// bool BlinkerESPMQTT::autoInit()
// {
//     WiFi.mode(WIFI_STA);
//     String _hostname = BLINKER_F("DiyArduino_");
//     _hostname += macDeviceName();

//     #if 0
//         WiFi.hostname(_hostname.c_str());
//     #elif defined(ESP32)
//         WiFi.setHostname(_hostname.c_str());
//     #endif

//     WiFi.begin();
//     ::delay(500);

//     BLINKER_LOG(BLINKER_F("Waiting for WiFi "), 
//                 BLINKER_WIFI_INIT_TIMEOUT / 1000,
//                 BLINKER_F("s, will enter SMARTCONFIG or "),

//     uint8_t _times = 0;
//     while (WiFi.status() != WL_CONNECTED) {
//         ::delay(500);
//         if (_times > BLINKER_WIFI_INIT_TIMEOUT / 500) break;
//         _times++;
//     }

//     if (WiFi.status() != WL_CONNECTED) return false;
//     else {
//         BLINKER_LOG(BLINKER_F("WiFi Connected."));
//         BLINKER_LOG(BLINKER_F("IP Address: "));
//         BLINKER_LOG(WiFi.localIP());

//         return true;
//     }
// }

// void BlinkerESPMQTT::smartconfig()
// {
//     WiFi.mode(WIFI_STA);
//     String _hostname = BLINKER_F("DiyArduino_");
//     _hostname += macDeviceName();
    
//     #if 0
//         WiFi.hostname(_hostname.c_str());
//     #elif defined(ESP32)
//         WiFi.setHostname(_hostname.c_str());
//     #endif

//     WiFi.beginSmartConfig();
    
//     BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
//     while (!WiFi.smartConfigDone()) {
//         ::delay(500);
//     }

//     BLINKER_LOG(BLINKER_F("SmartConfig received."));
    
//     BLINKER_LOG(BLINKER_F("Waiting for WiFi"));
//     while (WiFi.status() != WL_CONNECTED) {
//         ::delay(500);
//     }

//     BLINKER_LOG(BLINKER_F("WiFi Connected."));

//     BLINKER_LOG(BLINKER_F("IP Address: "));
//     BLINKER_LOG(WiFi.localIP());
// }

// {
//     WiFiServer _server(80);
//     WiFiClient _client;
//     IPAddress apIP(192, 168, 4, 1);
//     #if 0
//         IPAddress netMsk(255, 255, 255, 0);
//     #endif

//     // _server = new WiFiServer(80);

//     WiFi.mode(WIFI_AP);
//     String softAP_ssid = BLINKER_F("DiyArduino_");
//     softAP_ssid += macDeviceName();

//     #if 0
//         WiFi.hostname(softAP_ssid.c_str());
//     #elif defined(ESP32)
//         WiFi.setHostname(softAP_ssid.c_str());
//     #endif
    
//     WiFi.softAP(softAP_ssid.c_str(), ("12345678"));
//     delay(100);

//     _server.begin();
//     BLINKER_LOG(BLINKER_F("AP IP address: "), WiFi.softAPIP());
//     BLINKER_LOG(BLINKER_F("HTTP _server started"));
//     BLINKER_LOG(BLINKER_F("URL: http://"), WiFi.softAPIP());

//     while(WiFi.status() != WL_CONNECTED)
//     {
//         // serverClient();
//         _client = _server.available();
//         // if (_client.status() == CLOSED)
//         if (!_client.connected())
//         {
//             _client.stop();
//             BLINKER_LOG(BLINKER_F("Connection closed on _client"));
//         }
//         else
//         {
//             if (_client.available())
//             {
//                 String data = _client.readStringUntil('\r');

//                 // data = data.substring(4, data.length() - 9);
//                 _client.flush();

//                 BLINKER_LOG(BLINKER_F("clientData: "), data);

//                 if (STRING_contains_string(data, "ssid") && 
//                     STRING_contains_string(data, "pswd"))
//                 {
//                     String msg = BLINKER_F("{\"hello\":\"world\"}");
                    
//                     String s= BLINKER_F("HTTP/1.1 200 OK\r\n");
//                     s += BLINKER_F("Content-Type: application/json;");
//                     s += BLINKER_F("charset=utf-8\r\n");
//                     s += BLINKER_F("Content-Length: ");
//                     s += String(msg.length());
//                     s += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
//                     s += msg;
//                     s += BLINKER_F("\r\n");

//                     _client.print(s);
                    
//                     _client.stop();

//                 }
//             }
//         }
//         ::delay(10);
//     }
// }

// // void BlinkerESPMQTT::serverClient()
// // {
// //     if (!_client)
// //     {
// //         _client = _server->available();
// //     }
// //     else
// //     {
// //         // if (_client.status() == CLOSED)
// //         if (!_client.connected())
// //         {
// //             _client.stop();
// //             BLINKER_LOG(BLINKER_F("Connection closed on _client"));
// //         }
// //         else
// //         {
// //             if (_client.available())
// //             {
// //                 String data = _client.readStringUntil('\r');

// //                 // data = data.substring(4, data.length() - 9);
// //                 _client.flush();

// //                 BLINKER_LOG(BLINKER_F("clientData: "), data);

// //                 if (STRING_contains_string(data, "ssid") &&  
// //                     STRING_contains_string(data, "pswd"))
// //                 {
// //                     String msg = BLINKER_F("{\"hello\":\"world\"}");
                    
// //                     String s= BLINKER_F("HTTP/1.1 200 OK\r\n");
// //                     s += BLINKER_F("Content-Type: application/json;");
// //                     s += BLINKER_F("charset=utf-8\r\n");
// //                     s += BLINKER_F("Content-Length: ");
// //                     s += String(msg.length());
// //                     s += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
// //                     s += msg;
// //                     s += BLINKER_F("\r\n");

// //                     _client.print(s);
                    
// //                     _client.stop();

// //                 }
// //             }
// //         }
// //     }
// // }

// {
//     DynamicJsonBuffer jsonBuffer;
//     JsonObject& wifi_data = jsonBuffer.parseObject(data);

//     if (!wifi_data.success()) {
//         return false;
//     }
                    
//     String _ssid = wifi_data["ssid"];
//     String _pswd = wifi_data["pswd"];

//     BLINKER_LOG(BLINKER_F("ssid: "), _ssid);
//     BLINKER_LOG(BLINKER_F("pswd: "), _pswd);

//     // free(_server);
//     connectWiFi(_ssid, _pswd);
//     return true;
// }

// void BlinkerESPMQTT::connectWiFi(String _ssid, String _pswd)
// {
//     connectWiFi(_ssid.c_str(), _pswd.c_str());
// }

// void BlinkerESPMQTT::connectWiFi(const char* _ssid, const char* _pswd)
// {
//     uint32_t connectTime = millis();

//     BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

//     WiFi.mode(WIFI_STA);
//     String _hostname = BLINKER_F("DiyArduinoMQTT_");
//     _hostname += macDeviceName();
    
//     #if 0
//         WiFi.hostname(_hostname.c_str());
//     #elif defined(ESP32)
//         WiFi.setHostname(_hostname.c_str());
//     #endif

//     if (_pswd && strlen(_pswd)) {
//         WiFi.begin(_ssid, _pswd);
//     }
//     else {
//         WiFi.begin(_ssid);
//     }

//     while (WiFi.status() != WL_CONNECTED) {
//         ::delay(50);

//         if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
//             connectTime = millis();
//             BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
//             BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
//         }
//     }
//     BLINKER_LOG(BLINKER_F("Connected"));

//     IPAddress myip = WiFi.localIP();
//     BLINKER_LOG(BLINKER_F("Your IP is: "), myip);
// }

#endif

#endif
