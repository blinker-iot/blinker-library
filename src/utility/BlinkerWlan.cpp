// #if defined(ESP8266) || defined(ESP32)

// #define BLINKER_PRO

// #include "Blinker/BlinkerConfig.h"
// #include "Blinker/BlinkerDebug.h"
// #include "utility/BlinkerUtility.h"
// #include "utility/BlinkerWlan.h"
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
//     static IPAddress netMsk(255, 255, 255, 0);
// #endif


// bool BlinkerWlan::checkConfig() {
//     char ok[2 + 1];
//     EEPROM.begin(BLINKER_EEP_SIZE);
//     EEPROM.get(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
//     EEPROM.commit();
//     EEPROM.end();

//     if (String(ok) != String("OK")) {
        
//         BLINKER_LOG(("wlan config check,fail"));

//         _status = BWL_CONFIG_FAIL;
//         return false;
//     }
//     else {

//         BLINKER_LOG(("wlan config check,success"));

//         _status = BWL_CONFIG_SUCCESS;
//         return true;
//     }
// }

// void BlinkerWlan::loadConfig(char *_ssid, char *_pswd) {
//     char loadssid[BLINKER_SSID_SIZE];
//     char loadpswd[BLINKER_PSWD_SIZE];

//     EEPROM.begin(BLINKER_EEP_SIZE);
//     EEPROM.get(BLINKER_EEP_ADDR_SSID, loadssid);
//     EEPROM.get(BLINKER_EEP_ADDR_PSWD, loadpswd);
//     // char ok[2 + 1];
//     // EEPROM.get(EEP_ADDR_WIFI_CFG + BLINKER_SSID_SIZE + BLINKER_PSWD_SIZE, ok);
//     EEPROM.commit();
//     EEPROM.end();

//     strcpy(_ssid, loadssid);
//     strcpy(_pswd, loadpswd);

//     BLINKER_LOG(("SSID: "), _ssid, (" PASWD: "), _pswd);
// }

// void BlinkerWlan::saveConfig(char *_ssid, char *_pswd) {
//     char loadssid[BLINKER_SSID_SIZE];
//     char loadpswd[BLINKER_PSWD_SIZE];

//     memcpy(loadssid, _ssid, BLINKER_SSID_SIZE);
//     memcpy(loadpswd, _pswd, BLINKER_PSWD_SIZE);

//     EEPROM.begin(BLINKER_EEP_SIZE);
//     EEPROM.put(BLINKER_EEP_ADDR_SSID, loadssid);
//     EEPROM.put(BLINKER_EEP_ADDR_PSWD, loadpswd);
//     char ok[2 + 1] = "OK";
//     EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
//     EEPROM.commit();
//     EEPROM.end();

//     BLINKER_LOG(("Save wlan config"));
// }

// void BlinkerWlan::deleteConfig() {
//     char ok[3] = {0};
//     EEPROM.begin(BLINKER_EEP_SIZE);
//     // for (int i = BLINKER_EEP_ADDR_WLAN_CHECK; i < BLINKER_WLAN_CHECK_SIZE; i++)
//     //     EEPROM.write(i, 0);
//     EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
//     EEPROM.commit();
//     EEPROM.end();

//     BLINKER_LOG(("Erase wlan config"));
// }

// void BlinkerWlan::smartconfigBegin(uint16_t _time) {
//     WiFi.mode(WIFI_STA);
//     delay(100);
//     String softAP_ssid = STRING_format(_deviceType) + "_" + macDeviceName();

// #if defined(ESP8266)
//     WiFi.hostname(softAP_ssid);
// #elif defined(ESP32)
//     WiFi.setHostname(softAP_ssid.c_str());
// #endif

//     WiFi.beginSmartConfig();
//     connectTime = millis();
//     timeout = _time;
//     _status = BWL_SMARTCONFIG_BEGIN;

//     BLINKER_LOG(("Wait for Smartconfig"));
// }

// bool BlinkerWlan::smartconfigDone() {
//     if (WiFi.smartConfigDone())
//     {
//         // WiFi.setAutoConnect(true);
//         // WiFi.setAutoReconnect(true);
//         connectTime = millis();

//         _status = BWL_SMARTCONFIG_DONE;

//         BLINKER_LOG(("SmartConfig Success"));
// #if defined(ESP8266)
//         BLINKER_LOG(("SSID: "), WiFi.SSID(), (" PSWD: "), WiFi.psk());
//         // WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
//         connectWiFi(WiFi.SSID().c_str(), WiFi.psk().c_str());
// #endif
//         return true;
//     }
//     else {
//         return false;
//     }
// }

// void BlinkerWlan::connect() {
//     switch (_status) {
//         case BWL_CONFIG_SUCCESS :
//             // WiFi.setAutoConnect(false);
//             // WiFi.setAutoReconnect(true);

//             SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
//             PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

//             loadConfig(SSID, PSWD);
//             // WiFi.begin(SSID, PSWD);
//             connectWiFi(SSID, PSWD);

//             free(SSID);
//             free(PSWD);

//             _status = BWL_CONNECTING;
//             break;
//         case BWL_DISCONNECTED :
//             if (millis() - connectTime > 30000 && WiFi.status() != WL_CONNECTED) {
//                 BLINKER_LOG("status: ", WiFi.status());

//                 disconnect();
//                 // SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
//                 // PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
//                 // WiFi.reconnect();

//                 char _ssid_[BLINKER_SSID_SIZE];
//                 char _pswd_[BLINKER_PSWD_SIZE];

//                 loadConfig(_ssid_, _pswd_);
//                 connectWiFi(_ssid_, _pswd_);

//                 // WiFi.setAutoConnect(false);
//                 // WiFi.setAutoReconnect(true);

//                 // free(SSID);
//                 // free(PSWD);
//                 connectTime = millis();
//                 BLINKER_LOG(("connecting BWL_DISCONNECTED"));
                
//                 BLINKER_LOG(("_ssid_: "), _ssid_, (" _pswd_: "), _pswd_);
//             }
//             else if(WiFi.status() == WL_CONNECTED) {
//                 _status = BWL_CONNECTED;
//             }
//             break;
//     }
// }

// bool BlinkerWlan::connected() {
//     switch (_status) {
//         case BWL_SMARTCONFIG_DONE :
//             if (WiFi.status() != WL_CONNECTED) {
//                 if (millis() - connectTime > 15000)
//                 {
//                     BLINKER_LOG(("smartConfig time out"));
                    
//                     WiFi.stopSmartConfig();
//                     _status = BWL_SMARTCONFIG_TIMEOUT;
//                 }
//                 return false;
//             }
//             else if (WiFi.status() == WL_CONNECTED) {
//                 IPAddress deviceIP = WiFi.localIP();
//                 BLINKER_LOG(("WiFi connected"));
//                 BLINKER_LOG(("IP address: "));
//                 BLINKER_LOG(deviceIP);
//                 BLINKER_LOG(("SSID: "), WiFi.SSID(), (" PSWD: "), WiFi.psk());
                
//                 SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
//                 PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
//                 memcpy(SSID,"\0",BLINKER_SSID_SIZE);
//                 memcpy(SSID,WiFi.SSID().c_str(),BLINKER_SSID_SIZE);
//                 memcpy(PSWD,"\0",BLINKER_PSWD_SIZE);
//                 memcpy(PSWD,WiFi.psk().c_str(),BLINKER_PSWD_SIZE);
//                 saveConfig(SSID, PSWD);
//                 free(SSID);
//                 free(PSWD);

//                 // WiFi.setAutoConnect(true);
//                 // WiFi.setAutoReconnect(true);

//                 _status = BWL_CONNECTED_CHECK;
//                 return true;
//             }
//             break;
//         case BWL_APCONFIG_DONE :
//             if (WiFi.status() != WL_CONNECTED) {
//                 if (millis() - connectTime > 15000)
//                 {
//                     BLINKER_LOG(("APConfig time out"));
                    
//                     // WiFi.stopSmartConfig();
//                     _status = BWL_APCONFIG_TIMEOUT;
//                 }
//                 return false;
//             }
//             else if (WiFi.status() == WL_CONNECTED) {
//                 IPAddress deviceIP = WiFi.localIP();
//                 BLINKER_LOG(("WiFi connected"));
//                 BLINKER_LOG(("IP address: "));
//                 BLINKER_LOG(deviceIP);
//                 BLINKER_LOG(("SSID: "), WiFi.SSID(), (" PSWD: "), WiFi.psk());
                
//                 // SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
//                 // PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
//                 // memcpy(SSID,"\0",BLINKER_SSID_SIZE);
//                 // memcpy(SSID,WiFi.SSID().c_str(),BLINKER_SSID_SIZE);
//                 // memcpy(PSWD,"\0",BLINKER_PSWD_SIZE);
//                 // memcpy(PSWD,WiFi.psk().c_str(),BLINKER_PSWD_SIZE);
//                 saveConfig(SSID, PSWD);
//                 free(SSID);
//                 free(PSWD);

//                 // WiFi.setAutoConnect(true);
//                 // WiFi.setAutoReconnect(true);

//                 _status = BWL_CONNECTED_CHECK;
//                 return true;
//             }
//             break;
//         case BWL_CONNECTING :
//             if (WiFi.status() != WL_CONNECTED) {
//                 return false;
//             }
//             else if (WiFi.status() == WL_CONNECTED) {
//                 IPAddress deviceIP = WiFi.localIP();
//                 BLINKER_LOG(("WiFi connected"));
//                 BLINKER_LOG(("IP address: "));
//                 BLINKER_LOG(deviceIP);
//                 BLINKER_LOG(("SSID: "), WiFi.SSID(), (" PSWD: "), WiFi.psk());
                
//                 _status = BWL_CONNECTED_CHECK;
//                 return true;
//             }
//         case BWL_CONNECTED_CHECK :
//             // if (WiFi.status() != WL_CONNECTED)
//             //     _status = BWL_DISCONNECTED;
//             return (WiFi.status() == WL_CONNECTED);
//         case BWL_RESET :
//             return false;
//         default :
//             return false;
//     }
//     return false;
// }

// void BlinkerWlan::disconnect() {
//     WiFi.disconnect();
//     delay(100);
//     _status = BWL_DISCONNECTED;
//     BLINKER_LOG(("WiFi disconnected"));
// }

// void BlinkerWlan::reset() {
//     disconnect();
//     _status = BWL_RESET;
// }

// void BlinkerWlan::softAPinit() {
//     _server = new WiFiServer(80);

//     WiFi.mode(WIFI_AP);
//     String softAP_ssid = STRING_format(_deviceType) + "_" + macDeviceName();
    
// #if defined(ESP8266)
//     WiFi.hostname(softAP_ssid);
// #elif defined(ESP32)
//     WiFi.setHostname(softAP_ssid.c_str());
// #endif

// #if defined(ESP8266)
//     WiFi.softAPConfig(apIP, apIP, netMsk);
// #elif defined(ESP32)
//     WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
// #endif

//     WiFi.softAP(softAP_ssid.c_str(), "12345678");
//     delay(100);

//     _server->begin();
//     BLINKER_LOG(("AP IP address: "), WiFi.softAPIP());
//     BLINKER_LOG("HTTP _server started");
//     BLINKER_LOG(String("URL: http://" + WiFi.softAPIP()));

//     _status = BWL_APCONFIG_BEGIN;

//     BLINKER_LOG(("Wait for APConfig"));
// }

// void BlinkerWlan::serverClient()
// {
//     if (!_client)
//     {
//         _client = _server->available();
//     }
//     else
//     {
//         // if (_client.status() == CLOSED)
//         if (!_client.connected())
//         {
//             _client.stop();
//             BLINKER_LOG(("Connection closed on _client"));
//         }
//         else
//         {
//             if (_client.available())
//             {
//                 String data = _client.readStringUntil('\r');

//                 // data = data.substring(4, data.length() - 9);
//                 _client.flush();

//                 // BLINKER_LOG("clientData: ", data);

//                 if (STRING_contains_string(data, "ssid") && STRING_contains_string(data, "pswd")) {

//                     String msg = "{\"hello\":\"world\"}";
                    
//                     String s= "HTTP/1.1 200 OK\r\nContent-Type: application/json;charset=utf-8\r\n";
//                     s += String("Content-Length: " + String(msg.length()) + "\r\n" +  
//                         "Connection: Keep Alive\r\n\r\n" +  
//                         msg + "\r\n");

//                     _client.print(s);
                    
//                     _client.stop();

//                     parseUrl(data);
//                 }
//             }
//         }
//     }
// }

// void BlinkerWlan::parseUrl(String data)
// {
//     BLINKER_LOG("APCONFIG data: ", data);
//     DynamicJsonBuffer jsonBuffer;
//     JsonObject& wifi_data = jsonBuffer.parseObject(data);

//     if (!wifi_data.success()) {
//         return;
//     }
                    
//     String _ssid = wifi_data["ssid"];
//     String _pswd = wifi_data["pswd"];

//     BLINKER_LOG("ssid: ", _ssid);
//     BLINKER_LOG("pswd: ", _pswd);

//     free(_server);

//     SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
//     PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

//     strcpy(SSID, _ssid.c_str());
//     strcpy(PSWD, _pswd.c_str());
//     connectWiFi(_ssid, _pswd);
//     connectTime = millis();
//     _status = BWL_APCONFIG_DONE;

//     BLINKER_LOG(("APConfig Success"));
// }

// void BlinkerWlan::connectWiFi(String _ssid, String _pswd)
// {
//     connectWiFi(_ssid.c_str(), _pswd.c_str());
// }

// void BlinkerWlan::connectWiFi(const char* _ssid, const char* _pswd)
// {
//     uint32_t connectTime = millis();

//     BLINKER_LOG(("Connecting to "), _ssid);

//     WiFi.mode(WIFI_STA);
//     String _hostname = STRING_format(_deviceType) + "_" + macDeviceName();

//     #if defined(ESP8266)
//         WiFi.hostname(_hostname);
//     #elif defined(ESP32)
//         WiFi.setHostname(_hostname.c_str());
//     #endif

//     if (_pswd && strlen(_pswd)) {
//         WiFi.begin(_ssid, _pswd);
//     }
//     else {
//         WiFi.begin(_ssid);
//     }

//     // while (WiFi.status() != WL_CONNECTED) {
//     //     ::delay(50);

//     //     if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
//     //         connectTime = millis();
//     //         BLINKER_LOG(("WiFi connect timeout, please check ssid and pswd!"));
//     //         BLINKER_LOG(("Retring WiFi connect again!"));
//     //     }
//     // }
//     // BLINKER_LOG(("Connected"));

//     // IPAddress myip = WiFi.localIP();
//     // BLINKER_LOG(("Your IP is: "), myip);

//     // mDNSInit();
// }

// bool BlinkerWlan::run()
// {
//     if (millis() - debugStatusTime > 10000) {
//         debugStatusTime = millis();

//         BLINKER_LOG_ALL("WLAN status: ", _status);
//     }

//     switch (_status) {
//         case BWL_CONFIG_CKECK :
//             checkConfig();
//             break;
//         case BWL_CONFIG_FAIL :
//             #if defined(BLINKER_ESP_SMARTCONFIG)
//                 smartconfigBegin();
//             #elif defined(BLINKER_APCONFIG)
//                 softAPinit();
//             #endif
//             break;
//         case BWL_CONFIG_SUCCESS :
//             connect();
//             break;
//         case BWL_CONNECTING :
//             return connected();
//             break;
//         case BWL_CONNECTED :
//             return connected();
//             break;
//         case BWL_DISCONNECTED :
//             connect();
//             break;
//         case BWL_SMARTCONFIG_BEGIN :
//             smartconfigDone();
//             break;
//         case BWL_SMARTCONFIG_DONE :
//             return connected();
//             break;
//         case BWL_SMARTCONFIG_TIMEOUT :
//             _status = BWL_CONFIG_FAIL;
//             break;
//         case BWL_STACONFIG_BEGIN :
//             connect();
//             break;
//         case BWL_APCONFIG_BEGIN :
//             serverClient();
//             break;
//         case BWL_APCONFIG_DONE :
//             return connected();
//             break;
//         case BWL_APCONFIG_TIMEOUT :
//             _status = BWL_CONFIG_FAIL;
//             break;
//         case BWL_CONNECTED_CHECK :
//             return connected();
//             break;
//         case BWL_RESET:
//             break;
//         default :
//             break;
//     }
//     return false;
// }

// #endif
