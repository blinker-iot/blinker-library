#include "Adapters/BlinkerMQTT.h"
#include "utility/BlinkerDebug.h"
#include "BlinkerESP8266_MQTT.h"

#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#if defined(BLINKER_APCONFIG)
WiFiServer *_server;
WiFiClient _client;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
#endif

// extern BlinkerMQTT  Blinker_MQTT;
// BlinkerESP8266_MQTT Blinker(Blinker_MQTT);
// BlinkerESP8266_MQTT Blinker;

// Serial

// #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_APCONFIG)
// void BlinkerESP8266_MQTT::begin(const char* _auth)
// {
//     // Base::begin(_auth);
// // #if defined(BLINKER_ESP_SMARTCONFIG)
//     if (!autoInit()) smartconfig();
// // #elif defined(BLINKER_APCONFIG)
// //     if (!autoInit())
// //     {
// //         softAPinit();
// //         while(WiFi.status() != WL_CONNECTED)
// //         {
// //             serverClient();
// //             ::delay(10);
// //         }
// //     }
// // #endif

// //     // this->conn.begin(_auth);
// //     // strcpy(Base::_deviceName, this->conn.deviceName().c_str());

// //     BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
// // }
// // #else
// // void BlinkerESP8266_MQTT::begin(const char* _auth,
// //                                 const char* _ssid,
// //                                 const char* _pswd)
// // {
// //     connectWiFi(_ssid, _pswd);

// //     BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
// }
// #endif

#if defined(BLINKER_APCONFIG)
void BlinkerESP8266_MQTT::softAPinit()
{
    _server = new WiFiServer(80);

    WiFi.mode(WIFI_AP);
    String softAP_ssid = ("DiyArduino_") + macDeviceName();
    WiFi.hostname(softAP_ssid);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(softAP_ssid.c_str(), ("12345678"));
    delay(100);

    _server->begin();
    BLINKER_LOG(BLINKER_F("AP IP address: "), WiFi.softAPIP());
    BLINKER_LOG(BLINKER_F("HTTP _server started"));
    BLINKER_LOG(BLINKER_F("URL: http://"), WiFi.softAPIP());
}

void BlinkerESP8266_MQTT::serverClient()
{
    if (!_client)
    {
        _client = _server->available();
    }
    else
    {
        if (_client.status() == CLOSED)
        {
            _client.stop();
            BLINKER_LOG(BLINKER_F("Connection closed on _client"));
        }
        else
        {
            if (_client.available())
            {
                String data = _client.readStringUntil('\r');

                // data = data.substring(4, data.length() - 9);
                _client.flush();

                BLINKER_LOG(BLINKER_F("clientData: "), data);

                if (STRING_contains_string(data, "ssid") && \
                    STRING_contains_string(data, "pswd"))
                {
                    String msg = BLINKER_F("{\"hello\":\"world\"}");
                    
                    String s= BLINKER_F("HTTP/1.1 200 OK\r\n");
                    s += BLINKER_F("Content-Type: application/json;");
                    s += BLINKER_F("charset=utf-8\r\n");
                    s += BLINKER_F("Content-Length: ");
                    s += String(msg.length());
                    s += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
                    s += msg;
                    s += BLINKER_F("\r\n");

                    _client.print(s);
                    
                    _client.stop();

                    parseUrl(data);
                }
            }
        }
    }
}

bool BlinkerESP8266_MQTT::parseUrl(String data)
{
    BLINKER_LOG(BLINKER_F("APCONFIG data: "), data);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& wifi_data = jsonBuffer.parseObject(data);

    if (!wifi_data.success()) {
        return false;
    }
                    
    String _ssid = wifi_data["ssid"];
    String _pswd = wifi_data["pswd"];

    BLINKER_LOG(BLINKER_F("ssid: "), _ssid);
    BLINKER_LOG(BLINKER_F("pswd: "), _pswd);

    free(_server);
    connectWiFi(_ssid, _pswd);
    return true;
}
#endif

// #if defined(BLINKER_ESP_SMARTCONFIG)
void BlinkerESP8266_MQTT::smartconfig() {
    WiFi.mode(WIFI_STA);
    String _hostname = ("DiyArduino_") + macDeviceName();
    WiFi.hostname(_hostname.c_str());
    WiFi.beginSmartConfig();
    
    BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
    while (!WiFi.smartConfigDone()) {
        ::delay(500);
    }

    BLINKER_LOG(BLINKER_F("SmartConfig received."));
    
    BLINKER_LOG(BLINKER_F("Waiting for WiFi"));
    while (WiFi.status() != WL_CONNECTED) {
        ::delay(500);
    }

    BLINKER_LOG(BLINKER_F("WiFi Connected."));

    BLINKER_LOG(BLINKER_F("IP Address: "));
    BLINKER_LOG(WiFi.localIP());

    // mDNSInit();
}
// #endif

bool BlinkerESP8266_MQTT::autoInit() {
    WiFi.mode(WIFI_STA);
    String _hostname = ("DiyArduino_") + macDeviceName();
    WiFi.hostname(_hostname.c_str());

    WiFi.begin();
    ::delay(500);
    BLINKER_LOG(BLINKER_F("Waiting for WiFi "), 
                BLINKER_WIFI_AUTO_INIT_TIMEOUT / 1000,
                BLINKER_F("s, will enter SMARTCONFIG or "),
                BLINKER_F("APCONFIG while WiFi not connect!"));
    uint8_t _times = 0;
    while (WiFi.status() != WL_CONNECTED) {
        ::delay(500);
        if (_times > BLINKER_WIFI_AUTO_INIT_TIMEOUT / 500) break;
        _times++;
    }

    if (WiFi.status() != WL_CONNECTED) return false;
    else {
        BLINKER_LOG(BLINKER_F("WiFi Connected."));

        BLINKER_LOG(BLINKER_F("IP Address: "));
        BLINKER_LOG(WiFi.localIP());

        // mDNSInit();
        return true;
    }
}

void BlinkerESP8266_MQTT::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerESP8266_MQTT::connectWiFi(const char* _ssid, const char* _pswd)
{
    uint32_t connectTime = millis();

    BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

    WiFi.mode(WIFI_STA);
    String _hostname = ("DiyArduinoMQTT_") + macDeviceName();
    WiFi.hostname(_hostname);

    if (_pswd && strlen(_pswd)) {
        WiFi.begin(_ssid, _pswd);
    }
    else {
        WiFi.begin(_ssid);
    }

    while (WiFi.status() != WL_CONNECTED) {
        ::delay(50);

        if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
            connectTime = millis();
            BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
            BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
        }
    }
    BLINKER_LOG(BLINKER_F("Connected"));

    IPAddress myip = WiFi.localIP();
    BLINKER_LOG(BLINKER_F("Your IP is: "), myip);

    // mDNSInit();
}