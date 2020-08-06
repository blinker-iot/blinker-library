// #ifndef BLINKER_MQTT_LP_H
// #define BLINKER_MQTT_LP_H

// #if defined(ESP8266) || defined(ESP32)

// // #define BLINKER_MQTT

// #if defined(ESP8266)
//     #include <ESP8266mDNS.h>
//     #include <ESP8266WiFi.h>
//     #include <ESP8266HTTPClient.h>
// #elif defined(ESP32)
//     #include <ESPmDNS.h>
//     #include <WiFi.h>
//     #include <HTTPClient.h>
// #endif

// // #include <EEPROM.h>

// #include "modules/WebSockets/WebSocketsServer.h"
// #include "modules/mqtt/Adafruit_MQTT.h"
// #include "modules/mqtt/Adafruit_MQTT_Client.h"
// #include "modules/ArduinoJson/ArduinoJson.h"

// // #include "Adapters/BlinkerMQTTLP.h"
// #include "Blinker/BlinkerConfig.h"
// #include "Blinker/BlinkerDebug.h"
// #include "Blinker/BlinkerStream.h"
// #include "Blinker/BlinkerUtility.h"

// enum b_config_t {
//     COMM,
//     BLINKER_SMART_CONFIG,
//     BLINKER_AP_CONFIG
// };

// enum b_configStatus_t {
//     SMART_BEGIN,
//     SMART_DONE,
//     SMART_TIMEOUT,
//     APCFG_BEGIN,
//     APCFG_DONE,
//     APCFG_TIMEOUT
// };


// // static WiFiServer *_apServer;
// // static WiFiClient _apClient;

// // class BlinkerSharer
// // {
// //     public :
// //         BlinkerSharer(const String & _uuid)
// //         {
// //             name = (char*)malloc((_uuid.length()+1)*sizeof(char));
// //             strcpy(name, _uuid.c_str());
// //         }

// //         char * uuid() { return name; }

// //     private :
// //         char * name;
// // };

// class BlinkerMQTTLP : public BlinkerStream
// {
//     public :
//         BlinkerMQTTLP();

//         int connect();
//         int connected();
//         int mConnected();
//         void disconnect();
//         void ping();
//         int available();
//         int aligenieAvail();
//         int duerAvail();
//         // bool extraAvailable();
//         void subscribe();
//         char * lastRead();
//         void flush();
//         int print(char * data, bool needCheck = true);
//         int bPrint(char * name, const String & data);
//         int aliPrint(const String & data);
//         int  duerPrint(const String & data, bool report = false);
//         void aliType(const String & type);
//         void duerType(const String & type);
//         void begin(const char* auth);
//         bool begin();
//         int autoPrint(unsigned long id);
//         // bool autoPrint(char *name, char *type, char *data);
//         // bool autoPrint(char *name1, char *type1, char *data1,
//         //             char *name2, char *type2, char *data2);
//         char * deviceName();
//         char * authKey() { return _authKey; }
//         int init() { if (!isMQTTinit) checkInit(); return isMQTTinit; }
//         int reRegister() { return connectServer(); }
//         void freshAlive() { kaTime = millis(); isAlive = true; }
//         void sharers(const String & data);
//         int  needFreshShare() {
//             if (_needCheckShare)
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("needFreshShare"));
//                 _needCheckShare = false;
//                 return true;
//             }
//             else
//             {
//                 return false;
//             }
//         }

//         bool checkInit();
//         void commonBegin(const char* _ssid, const char* _pswd);
//         void smartconfigBegin();
//         void apconfigBegin();
//         bool autoInit();
//         void smartconfig();
//         #if defined(BLINKER_APCONFIG)
//         void softAPinit();
//         void checkAPCFG();
//         bool parseUrl(String data);
//         #endif
//         void connectWiFi(String _ssid, String _pswd);
//         void connectWiFi(const char* _ssid, const char* _pswd);

//     private :
//         bool isMQTTinit = false;

//         int connectServer();
//         void mDNSInit();
//         void checkKA();
//         int checkAliKA();
//         int checkDuerKA();
//         int checkCanPrint();
//         int checkCanBprint();
//         int checkPrintSpan();
//         int checkAliPrintSpan();
//         int checkDuerPrintSpan();

//         bool isLowPower = false;

//     protected :
//         BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
//         uint8_t     _sharerCount = 0;
//         uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
//         bool        _isWiFiInit = false;
//         bool        _isBegin = false;
//         b_config_t  _configType = COMM;
//         b_configStatus_t _configStatus = SMART_BEGIN;
//         uint32_t    _connectTime = 0;
//         // const char* _authKey;
//         char*       _authKey;
//         char*       _aliType;
//         char*       _duerType;
//         // char        _authKey[BLINKER_AUTHKEY_SIZE];
//         bool*       isHandle;// = &isConnect;
//         bool        isAlive = false;
//         // bool        isBavail = false;
//         bool        _needCheckShare = false;
//         uint32_t    latestTime;
//         uint32_t    printTime = 0;
//         uint32_t    bPrintTime = 0;
//         uint32_t    kaTime = 0;
//         uint32_t    linkTime = 0;
//         uint8_t     respTimes = 0;
//         uint32_t    respTime = 0;
//         uint8_t     respAliTimes = 0;
//         uint32_t    respAliTime = 0;
//         uint8_t     respDuerTimes = 0;
//         uint32_t    respDuerTime = 0;

//         uint32_t    aliKaTime = 0;
//         bool        isAliAlive = false;
//         bool        isAliAvail = false;
//         uint32_t    duerKaTime = 0;
//         bool        isDuerAlive = false;
//         bool        isDuerAvail = false;
//         char*       mqtt_broker;

//         int isJson(const String & data);

//         #if defined(BLINKER_APCONFIG)
//         WiFiServer *_apServer;
//         WiFiClient _apClient;
//         #endif

//         uint8_t     reconnect_time = 0;
// };

// char*       MQTT_HOST_MQTT;
// char*       MQTT_ID_MQTT;
// char*       MQTT_NAME_MQTT;
// char*       MQTT_KEY_MQTT;
// char*       MQTT_PRODUCTINFO_MQTT;
// char*       UUID_MQTT;
// char*       DEVICE_NAME_MQTT;
// char*       BLINKER_PUB_TOPIC_MQTT;
// char*       BLINKER_SUB_TOPIC_MQTT;
// uint16_t    MQTT_PORT_MQTT;

// #if defined(ESP8266)
//     BearSSL::WiFiClientSecure   client_mqtt;
//     // WiFiClientSecure         client_mqtt;
// #elif defined(ESP32)
//     WiFiClientSecure     client_s;
// #endif

// WiFiClient               client;
// Adafruit_MQTT_Client*    mqtt_MQTT;
// // Adafruit_MQTT_Publish   *iotPub;
// Adafruit_MQTT_Subscribe* iotSub_MQTT;

// #define WS_SERVERPORT       81
// WebSocketsServer webSocket_MQTT = WebSocketsServer(WS_SERVERPORT);

// char*    msgBuf_MQTT;
// bool     isFresh_MQTT = false;
// bool     isConnect_MQTT = false;
// bool     isAvail_MQTT = false;
// uint8_t  ws_num_MQTT = 0;
// uint8_t  dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

// void webSocketEvent_MQTT(uint8_t num, WStype_t type, \
//                     uint8_t * payload, size_t length)
// {

//     switch(type)
//     {
//         case WStype_DISCONNECTED:
//             BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

//             isConnect_MQTT = false;
//             break;
//         case WStype_CONNECTED:
//             {
//                 IPAddress ip = webSocket_MQTT.remoteIP(num);

//                 BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
//                                 BLINKER_F(", Connected from: "), ip,
//                                 BLINKER_F(", url: "), (char *)payload);

//                 // send message to client
//                 webSocket_MQTT.sendTXT(num, "{\"state\":\"connected\"}\n");

//                 ws_num_MQTT = num;

//                 isConnect_MQTT = true;
//             }
//             break;
//         case WStype_TEXT:
//             BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
//                             BLINKER_F(", get Text: "), (char *)payload, \
//                             BLINKER_F(", length: "), length);

//             if (length < BLINKER_MAX_READ_SIZE) {
//                 if (isFresh_MQTT) free(msgBuf_MQTT);
//                 msgBuf_MQTT = (char*)malloc((length+1)*sizeof(char));
//                 strcpy(msgBuf_MQTT, (char*)payload);
//                 isAvail_MQTT = true;
//                 isFresh_MQTT = true;
//             }

//             dataFrom_MQTT = BLINKER_MSG_FROM_WS;

//             ws_num_MQTT = num;

//             // send message to client
//             // webSocket_MQTT.sendTXT(num, "message here");

//             // send data to all connected clients
//             // webSocket_MQTT.broadcastTXT("message here");
//             break;
//         case WStype_BIN:
//             // BLINKER_LOG("num: ", num, " get binary length: ", length);
//             // hexdump(payload, length);

//             // send message to client
//             // webSocket_MQTT.sendBIN(num, payload, length);
//             break;
//         default :
//             break;
//     }
// }

// BlinkerMQTTLP::BlinkerMQTTLP() { isHandle = &isConnect_MQTT; }

// int BlinkerMQTTLP::connect()
// {
//     if (!checkInit()) return false;

//     int8_t ret;

//     webSocket_MQTT.loop();

//     if (mqtt_MQTT->connected())
//     {
//         return true;
//     }

//     disconnect();

//     if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
//     {
//         yield();
//         return false;
//     }

//     BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));

//     // BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_MQTT: "), MQTT_ID_MQTT);
//     // BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_MQTT: "), MQTT_NAME_MQTT);
//     // BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_MQTT: "), MQTT_KEY_MQTT);

//     #if defined(ESP8266)
//         client_mqtt.setInsecure();
//         ::delay(10);
//     #endif

//     if ((ret = mqtt_MQTT->connect()) != 0)
//     {
//         BLINKER_LOG(mqtt_MQTT->connectErrorString(ret));
//         BLINKER_LOG(BLINKER_F("Retrying MQTT connection in "), \
//                     BLINKER_MQTT_CONNECT_TIMESLOT/1000, \
//                     BLINKER_F(" seconds..."));

//         if (ret == 4) reRegister();

//         this->latestTime = millis();
//         reconnect_time += 1;
//         if (reconnect_time >= 12)
//         {
//             reRegister();
//             reconnect_time = 0;
//         }
//         return false;
//     }
    
//     reconnect_time = 0;

//     BLINKER_LOG(BLINKER_F("MQTT Connected!"));
//     BLINKER_LOG_FreeHeap();

//     if (!isMQTTinit)
//     {
//         isMQTTinit = true;

//         mqtt_MQTT->subscribeTopic(BLINKER_SUB_TOPIC_MQTT);
//         BLINKER_LOG_ALL(BLINKER_F("MQTT subscribe: "), BLINKER_SUB_TOPIC_MQTT);
//     }

//     this->latestTime = millis();

//     return true;
// }

// int BlinkerMQTTLP::connected()
// {
//     if (!checkInit()) return false;

//     if (!isMQTTinit)
//     {
//         return *isHandle;
//     }

//     return mqtt_MQTT->connected() || *isHandle;
// }

// int BlinkerMQTTLP::mConnected()
// {
//     if (!checkInit()) return false;

//     if (!isMQTTinit) return false;
//     else return mqtt_MQTT->connected();
// }

// void BlinkerMQTTLP::disconnect()
// {
//     if (!checkInit()) return;

//     mqtt_MQTT->disconnect();

//     if (*isHandle) webSocket_MQTT.disconnect();
// }

// void BlinkerMQTTLP::ping()
// {
//     if (!checkInit()) return;

//     BLINKER_LOG(BLINKER_F("MQTT Ping!"));

//     BLINKER_LOG_FreeHeap_ALL();

//     if (!mqtt_MQTT->ping())
//     {
//         disconnect();
//         // delay(100);

//         // connect();
//     }
//     else
//     {
//         this->latestTime = millis();
//     }
// }

// int BlinkerMQTTLP::available()
// {
//     if (!checkInit()) return false;

//     webSocket_MQTT.loop();

//     checkKA();

//     // if (!mqtt_MQTT->connected() || 
//     //     (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
//     if ((millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
//     {
//         ping();
//     }
//     else
//     {
//         subscribe();
//     }

//     if (isAvail_MQTT)
//     {
//         isAvail_MQTT = false;
//         return true;
//     }
//     else {
//         return false;
//     }
// }

// int BlinkerMQTTLP::aligenieAvail()
// {
//     if (!checkInit()) return false;

//     if (isAliAvail)
//     {
//         isAliAvail = false;
//         return true;
//     }
//     else {
//         return false;
//     }
// }

// int BlinkerMQTTLP::duerAvail()
// {
//     if (!checkInit()) return false;

//     if (isDuerAvail)
//     {
//         isDuerAvail = false;
//         return true;
//     }
//     else {
//         return false;
//     }
// }

// // bool BlinkerMQTTLP::extraAvailable()
// // {
// //     if (isBavail)
// //     {
// //         isBavail = false;
// //         return true;
// //     }
// //     else
// //     {
// //         return false;
// //     }
// // }

// void BlinkerMQTTLP::subscribe()
// {
//     if (!checkInit()) return;

//     if (!isMQTTinit) return;

//     Adafruit_MQTT_Subscribe *subscription;
//     while ((subscription = mqtt_MQTT->readSubscription(10)))
//     {
//         if (subscription == iotSub_MQTT)
//         {
//             BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub_MQTT->lastread);

//             DynamicJsonBuffer jsonBuffer;
//             JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub_MQTT->lastread));

//             String _uuid = root["fromDevice"];
//             String dataGet = root["data"];

//             BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
//             BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);

//             if (strcmp(_uuid.c_str(), UUID_MQTT) == 0)
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));

//                 kaTime = millis();
//                 isAvail_MQTT = true;
//                 isAlive = true;

//                 _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
//             }
//             else if (_uuid == BLINKER_CMD_ALIGENIE)
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("form AliGenie"));

//                 aliKaTime = millis();
//                 isAliAlive = true;
//                 isAliAvail = true;
//             }
//             else if (_uuid == BLINKER_CMD_DUEROS)
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("form DuerOS"));

//                 duerKaTime = millis();
//                 isDuerAlive = true;
//                 isDuerAvail = true;
//             }
//             else if (_uuid == BLINKER_CMD_SERVERCLIENT)
//             {
//                 BLINKER_LOG_ALL(BLINKER_F("form Sever"));

//                 isAvail_MQTT = true;
//                 isAlive = true;

//                 _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
//             }
//             else
//             {
//                 if (_sharerCount)
//                 {
//                     for (uint8_t num = 0; num < _sharerCount; num++)
//                     {
//                         if (strcmp(_uuid.c_str(), _sharers[num]->uuid()) == 0)
//                         {
//                             _sharerFrom = num;

//                             kaTime = millis();

//                             BLINKER_LOG_ALL(BLINKER_F("From sharer: "), _uuid);
//                             BLINKER_LOG_ALL(BLINKER_F("sharer num: "), num);
                            
//                             _needCheckShare = false;

//                             break;
//                         }
//                         else
//                         {
//                             BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
//                                         check is from bridge/share device, \
//                                         data: "), dataGet);

//                             _needCheckShare = true;
//                         }
//                     }
//                 }
//                 // else
//                 // {
//                     // dataGet = String((char *)iotSub_MQTT->lastread);
//                     root.printTo(dataGet);

//                 //     BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
//                 //                         check is from bridge/share device, \
//                 //                         data: "), dataGet);

//                 //     // return;

//                 //     // isBavail = true;

//                 //     _needCheckShare = true;
//                 // }

//                 isAvail_MQTT = true;
//                 isAlive = true;
//             }

//             // memset(msgBuf_MQTT, 0, BLINKER_MAX_READ_SIZE);
//             // memcpy(msgBuf_MQTT, dataGet.c_str(), dataGet.length());

//             if (isFresh_MQTT) free(msgBuf_MQTT);
//             msgBuf_MQTT = (char*)malloc((dataGet.length()+1)*sizeof(char));
//             strcpy(msgBuf_MQTT, dataGet.c_str());
//             isFresh_MQTT = true;

//             this->latestTime = millis();

//             dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;
//         }
//     }
// }

// char * BlinkerMQTTLP::lastRead()
// {
//     if (isFresh_MQTT) return msgBuf_MQTT;
//     else return "";
// }

// void BlinkerMQTTLP::flush()
// {
//     if (isFresh_MQTT)
//     {
//         free(msgBuf_MQTT); isFresh_MQTT = false; isAvail_MQTT = false;
//         isAliAvail = false; //isBavail = false;
//     }
// }

// int BlinkerMQTTLP::print(char * data, bool needCheck)
// {
//     if (!checkInit()) return false;

//     // BLINKER_LOG_FreeHeap();
//     if (*isHandle && dataFrom_MQTT == BLINKER_MSG_FROM_WS)
//     {
//         if (needCheck)
//         {
//             if (!checkPrintSpan())
//             {
//                 respTime = millis();
//                 return false;
//             }
//         }

//         respTime = millis();

//         BLINKER_LOG_ALL(BLINKER_F("WS response: "));
//         BLINKER_LOG_ALL(data);
//         BLINKER_LOG_ALL(BLINKER_F("Success..."));

//         strcat(data, BLINKER_CMD_NEWLINE);

//         webSocket_MQTT.sendTXT(ws_num_MQTT, data);

//         return true;
//     }
//     else
//     {
//         // // String payload;
//         // if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
//         // {
//         //     uint8_t num = strlen(data) - 1;
//         //     for(uint8_t c_num = num; c_num > 0; c_num--)
//         //     {
//         //         data[c_num+7] = data[c_num-1];
//         //     }
//         //     // payload = BLINKER_F("{\"data\":");
//         //     // payload += data.substring(0, data.length() - 1);
//         //     // payload += BLINKER_F(",\"fromDevice\":\"");
//         //     // payload += MQTT_ID_MQTT;
//         //     // payload += BLINKER_F("\",\"toDevice\":\"");
//         //     // payload += UUID_MQTT;
//         //     // payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
//         // }
//         // else
//         // {
//         //     uint8_t num = strlen(data);
//         //     for(uint8_t c_num = num; c_num > 0; c_num--)
//         //     {
//         //         data[c_num+7] = data[c_num-1];
//         //     }
//         //     // payload = BLINKER_F("{\"data\":");
//         //     // payload += data;
//         //     // payload += BLINKER_F(",\"fromDevice\":\"");
//         //     // payload += MQTT_ID_MQTT;
//         //     // payload += BLINKER_F("\",\"toDevice\":\"");
//         //     // payload += UUID_MQTT;
//         //     // payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
//         // }

//         uint16_t num = strlen(data);

//         for(uint16_t c_num = num; c_num > 0; c_num--)
//         {
//             data[c_num+7] = data[c_num-1];
//         }

//         data[num+8] = '\0';

//         char data_add[20] = "{\"data\":";
//         for(uint8_t c_num = 0; c_num < 8; c_num++)
//         {
//             data[c_num] = data_add[c_num];
//         }

//         // data_add = BLINKER_F(",\"fromDevice\":\"");
//         // strcat(data, data_add.c_str());
//         strcat(data, ",\"fromDevice\":\"");
//         strcat(data, MQTT_ID_MQTT);
//         // data_add = BLINKER_F("\",\"toDevice\":\"");
//         // strcat(data, data_add.c_str());
//         strcat(data, "\",\"toDevice\":\"");
//         if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
//         {
//             strcat(data, _sharers[_sharerFrom]->uuid());
//         }
//         else
//         {
//             strcat(data, UUID_MQTT);
//         }
//         // data_add = BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
//         // strcat(data, data_add.c_str());

//         strcat(data, "\",\"deviceType\":\"OwnApp\"}");

//         _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

//         if (!isJson(STRING_format(data))) return false;

//         BLINKER_LOG_ALL(BLINKER_F("MQTT Publish..."));
//         BLINKER_LOG_FreeHeap_ALL();

//         bool _alive = isAlive;


//         if (needCheck)
//         {
//             if (!checkPrintSpan())
//             {
//                 return false;
//             }
//             respTime = millis();
//         }

//         if (mqtt_MQTT->connected())
//         {
//             if (needCheck)
//             {
//                 if (!checkCanPrint())
//                 {
//                     if (!_alive)
//                     {
//                         isAlive = false;
//                     }
//                     return false;
//                 }
//             }

//             if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, data))
//             {
//                 BLINKER_LOG_ALL(data);
//                 BLINKER_LOG_ALL(BLINKER_F("...Failed"));
//                 BLINKER_LOG_FreeHeap_ALL();

//                 if (!_alive)
//                 {
//                     isAlive = false;
//                 }
//                 return false;
//             }
//             else
//             {
//                 BLINKER_LOG_ALL(data);
//                 BLINKER_LOG_ALL(BLINKER_F("...OK!"));
//                 BLINKER_LOG_FreeHeap_ALL();

//                 if (needCheck) printTime = millis();

//                 if (!_alive)
//                 {
//                     isAlive = false;
//                 }

//                 this->latestTime = millis();

//                 return true;
//             }
//         }
//         else
//         {
//             BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//             isAlive = false;
//             return false;
//         }
//     }
// }

// int BlinkerMQTTLP::bPrint(char * name, const String & data)
// {
//     if (!checkInit()) return false;

//     // String payload;
//     // if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
//     // {
//     //     payload = BLINKER_F("{\"data\":");
//     //     payload += data.substring(0, data.length() - 1);
//     //     payload += BLINKER_F(",\"fromDevice\":\"");
//     //     payload += MQTT_ID_MQTT;
//     //     payload += BLINKER_F("\",\"toDevice\":\"");
//     //     payload += name;
//     //     payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
//     // }
//     // else
//     // {
//     //     payload = BLINKER_F("{\"data\":");
//     //     payload += data;
//     //     payload += BLINKER_F(",\"fromDevice\":\"");
//     //     payload += MQTT_ID_MQTT;
//     //     payload += BLINKER_F("\",\"toDevice\":\"");
//     //     payload += name;
//     //     payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
//     // }

//     // uint8_t num = strlen(data);
//     // for(uint8_t c_num = num; c_num > 0; c_num--)
//     // {
//     //     data[c_num+7] = data[c_num-1];
//     // }

//     String data_add = BLINKER_F("{\"data\":");
//     // for(uint8_t c_num = 0; c_num < 8; c_num++)
//     // {
//     //     data[c_num] = data_add[c_num];
//     // }

//     data_add += data;
//     data_add += BLINKER_F(",\"fromDevice\":\"");
//     data_add += MQTT_ID_MQTT;
//     data_add += BLINKER_F("\",\"toDevice\":\"");
//     data_add += name;
//     data_add += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");

//     // data_add = BLINKER_F(",\"fromDevice\":\"");
//     // strcat(data, data_add.c_str());
//     // strcat(data, MQTT_ID_MQTT);
//     // data_add = BLINKER_F("\",\"toDevice\":\"");
//     // strcat(data, data_add.c_str());
//     // strcat(data, name);
//     // data_add = BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
//     // strcat(data, data_add.c_str());

//     if (!isJson(data_add)) return false;

//     BLINKER_LOG_ALL(BLINKER_F("MQTT Bridge Publish..."));

//     // bool _alive = isAlive;
//     // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

//     // if (!state) {
//     //     state = (STRING_contains_string(data, BLINKER_CMD_STATE)
//     //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
//     // }

//     if (mqtt_MQTT->connected()) {
//         // if (!state) {
//         if (!checkCanBprint()) {
//             // if (!_alive) {
//             //     isAlive = false;
//             // }
//             return false;
//         }
//         // }

//         // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);

//         // if (! iotPub.publish(payload.c_str())) {

//         // String bPubTopic = BLINKER_F("");

//         // if (strcmp(mqtt_broker, BLINKER_MQTT_BORKER_ONENET) == 0)
//         // {
//         //     bPubTopic = MQTT_PRODUCTINFO_MQTT;
//         //     bPubTopic += BLINKER_F("/");
//         //     bPubTopic += name;
//         //     bPubTopic += BLINKER_F("/r");
//         // }
//         // else
//         // {
//         //     bPubTopic = BLINKER_PUB_TOPIC_MQTT;
//         // }

//         if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, data_add.c_str()))
//         {
//             BLINKER_LOG_ALL(data_add);
//             BLINKER_LOG_ALL(BLINKER_F("...Failed"));

//             // if (!_alive) {
//             //     isAlive = false;
//             // }
//             return false;
//         }
//         else
//         {
//             BLINKER_LOG_ALL(data_add);
//             BLINKER_LOG_ALL(BLINKER_F("...OK!"));

//             bPrintTime = millis();

//             // if (!_alive) {
//             //     isAlive = false;
//             // }

//             this->latestTime = millis();

//             return true;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         // isAlive = false;
//         return false;
//     }
//     // }
// }

// int BlinkerMQTTLP::aliPrint(const String & data)
// {
//     if (!checkInit()) return false;

//     String data_add = BLINKER_F("{\"data\":");

//     data_add += data;
//     data_add += BLINKER_F(",\"fromDevice\":\"");
//     data_add += MQTT_ID_MQTT;
//     data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
//     data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

//     if (!isJson(data_add)) return false;

//     BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));
//     BLINKER_LOG_FreeHeap_ALL();

//     if (mqtt_MQTT->connected())
//     {
//         if (!checkAliKA())
//         {
//             return false;
//         }

//         if (!checkAliPrintSpan())
//         {
//             respAliTime = millis();
//             return false;
//         }
//         respAliTime = millis();

//         if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, data_add.c_str()))
//         {
//             BLINKER_LOG_ALL(data_add);
//             BLINKER_LOG_ALL(BLINKER_F("...Failed"));
//             BLINKER_LOG_FreeHeap_ALL();

//             isAliAlive = false;
//             return false;
//         }
//         else
//         {
//             BLINKER_LOG_ALL(data_add);
//             BLINKER_LOG_ALL(BLINKER_F("...OK!"));
//             BLINKER_LOG_FreeHeap_ALL();

//             isAliAlive = false;

//             this->latestTime = millis();

//             return true;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

// int BlinkerMQTTLP::duerPrint(const String & data, bool report)
// {
//     if (!checkInit()) return false;

//     String data_add = BLINKER_F("{\"data\":");

//     data_add += data;
//     data_add += BLINKER_F(",\"fromDevice\":\"");
//     data_add += MQTT_ID_MQTT;
//     data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
//     data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

//     if (!isJson(data_add)) return false;

//     BLINKER_LOG_ALL(BLINKER_F("MQTT DuerOS Publish..."));
//     BLINKER_LOG_FreeHeap_ALL();

//     if (mqtt_MQTT->connected())
//     {
//         if (!checkDuerKA())
//         {
//             return false;
//         }

//         if (!checkDuerPrintSpan())
//         {
//             respDuerTime = millis();
//             return false;
//         }
//         respDuerTime = millis();

//         if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, data_add.c_str()))
//         {
//             BLINKER_LOG_ALL(data_add);
//             BLINKER_LOG_ALL(BLINKER_F("...Failed"));
//             BLINKER_LOG_FreeHeap_ALL();

//             isDuerAlive = false;
//             return false;
//         }
//         else
//         {
//             BLINKER_LOG_ALL(data_add);
//             BLINKER_LOG_ALL(BLINKER_F("...OK!"));
//             BLINKER_LOG_FreeHeap_ALL();

//             isDuerAlive = false;

//             this->latestTime = millis();

//             return true;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

// void BlinkerMQTTLP::aliType(const String & type)
// {
//     _aliType = (char*)malloc((type.length()+1)*sizeof(char));
//     strcpy(_aliType, type.c_str());
//     BLINKER_LOG_ALL(BLINKER_F("_aliType: "), _aliType);
// }

// void BlinkerMQTTLP::duerType(const String & type)
// {
//     _duerType = (char*)malloc((type.length()+1)*sizeof(char));
//     strcpy(_duerType, type.c_str());
//     BLINKER_LOG_ALL(BLINKER_F("_duerType: "), _duerType);
// }

// void BlinkerMQTTLP::begin(const char* auth) {
//     // if (!checkInit()) return;
//     // _authKey = auth;
//     _authKey = (char*)malloc((strlen(auth)+1)*sizeof(char));
//     strcpy(_authKey, auth);

//     BLINKER_LOG_ALL(BLINKER_F("_authKey: "), auth);
// }

// bool BlinkerMQTTLP::begin() {
//     // if (connectServer()) {
//     //     mDNSInit();
//     //     isMQTTinit = true;
//     // }
//     // else {
//     // uint32_t re_time = millis();

//     if (isMQTTinit) return true;
//     // bool isConnect_MQTT = true;
//     // while(1)
//     // {
//     //     re_time = millis();
//         // ::delay(10000);
//         // BLINKER_ERR_LOG("Maybe you have put in the wrong AuthKey!");
//         // BLINKER_ERR_LOG("Or maybe your request is too frequently!");
//         // BLINKER_ERR_LOG("Or maybe your network is disconnected!");
//     if (_connectTime == 0 || (millis() - _connectTime) >= 10000)
//     {
//         _connectTime = millis();
//         if (connectServer()) {
//             mDNSInit();
//             isMQTTinit = true;
//             return true;
//         }
//         else {
//             isMQTTinit = false;
//             // ::delay(10000);
//             // BLINKER_ERR_LOG("init error, ", _connectTime);
//             // BLINKER_ERR_LOG("Maybe you have put in the wrong AuthKey!");
//             // BLINKER_ERR_LOG("Or maybe your request is too frequently!");
//             // BLINKER_ERR_LOG("Or maybe your network is disconnected!");
//             return false;
//         }
//     }

//     // BLINKER_ERR_LOG("init error1, ", _connectTime);
//     // ::delay(1000);
//     return false;
//         // delay(10000);
//         // while ((millis() - re_time) < 10000)
//         // {
//         //     if (WiFi.status() != WL_CONNECTED && isConnect_MQTT)
//         //     {
//         //         isConnect_MQTT = false;
//         //         WiFi.begin();
//         //         WiFi.reconnect();
//         //     }
//         //     else if (WiFi.status() == WL_CONNECTED && !isConnect_MQTT)
//         //     {
//         //         isConnect_MQTT = true;
//         //     }
//         //     // ::delay(10);
//         //     yield();
//         //     // WiFi.status();
//         // }
//     // }
//     // }
// }

// int BlinkerMQTTLP::autoPrint(unsigned long id)
// {
//     String payload = BLINKER_F("{\"data\":{\"set\":{");
//     payload += BLINKER_F("\"trigged\":true,\"autoData\":{");
//     payload += BLINKER_F("\"autoId\":");
//     payload += STRING_format(id);
//     payload += BLINKER_F("}}}");
//     payload += BLINKER_F(",\"fromDevice\":\"");
//     payload += STRING_format(MQTT_ID_MQTT);
//     payload += BLINKER_F("\",\"toDevice\":\"autoManager\"}");
//         // "\",\"deviceType\":\"" + "type" + "\"}";

//     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

//     if (mqtt_MQTT->connected())
//     {
//         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
//             linkTime == 0)
//         {
//             // linkTime = millis();

//             // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);

//             // if (! iotPub.publish(payload.c_str())) {

//             if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, payload.c_str()))
//             {
//                 BLINKER_LOG_ALL(payload);
//                 BLINKER_LOG_ALL(BLINKER_F("...Failed"));

//                 return false;
//             }
//             else
//             {
//                 BLINKER_LOG_ALL(payload);
//                 BLINKER_LOG_ALL(BLINKER_F("...OK!"));

//                 linkTime = millis();

//                 this->latestTime = millis();

//                 return true;
//             }
//         }
//         else
//         {
//             BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);

//             return false;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

// // bool BlinkerMQTTLP::autoPrint(char *name, char *type, char *data)
// // {
// //     String payload = BLINKER_F("{\"data\":{");
// //     payload += STRING_format(data);
// //     payload += BLINKER_F("},\"fromDevice\":\"");
// //     payload += STRING_format(MQTT_ID_MQTT);
// //     payload += BLINKER_F("\",\"toDevice\":\"");
// //     payload += name;
// //     payload += BLINKER_F("\",\"deviceType\":\"");
// //     payload += type;
// //     payload += BLINKER_F("\"}");

// //     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

// //     if (mqtt_MQTT->connected()) {
// //         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT ||
// //             linkTime == 0)
// //         {
// //             linkTime = millis();

// //             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));

// //             return true;
// //         }
// //         else {
// //             BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);

// //             return false;
// //         }
// //     }
// //     else
// //     {
// //         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
// //         return false;
// //     }
// // }

// // bool BlinkerMQTTLP::autoPrint(char *name1, char *type1, char *data1
// //     , char *name2, char *type2, char *data2)
// // {
// //     String payload = BLINKER_F("{\"data\":{");
// //     payload += STRING_format(data1);
// //     payload += BLINKER_F("},\"fromDevice\":\"");
// //     payload += STRING_format(MQTT_ID_MQTT);
// //     payload += BLINKER_F("\",\"toDevice\":\"");
// //     payload += name1;
// //     payload += BLINKER_F("\",\"deviceType\":\"");
// //     payload += type1;
// //     payload += BLINKER_F("\"}");

// //     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

// //     if (mqtt_MQTT->connected())
// //     {
// //         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT ||
// //             linkTime == 0)
// //         {
// //             linkTime = millis();

// //             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));

// //             payload = BLINKER_F("{\"data\":{");
// //             payload += STRING_format(data2);
// //             payload += BLINKER_F("},\"fromDevice\":\"");
// //             payload += STRING_format(MQTT_ID_MQTT);
// //             payload += BLINKER_F("\",\"toDevice\":\"");
// //             payload += name2;
// //             payload += BLINKER_F("\",\"deviceType\":\"");
// //             payload += type2;
// //             payload += BLINKER_F("\"}");

// //             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));

// //             return true;
// //         }
// //         else
// //         {
// //             BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);

// //             return false;
// //         }
// //     }
// //     else
// //     {
// //         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
// //         return false;
// //     }
// // }

// char * BlinkerMQTTLP::deviceName() { return DEVICE_NAME_MQTT;/*MQTT_ID_MQTT;*/ }

// void BlinkerMQTTLP::sharers(const String & data)
// {
//     BLINKER_LOG_ALL(BLINKER_F("sharers data: "), data);

//     DynamicJsonBuffer jsonBuffer;
//     JsonObject& root = jsonBuffer.parseObject(data);

//     if (!root.success()) return;

//     String user_name = "";

//     if (_sharerCount)
//     {
//         for (uint8_t num = _sharerCount; num > 0; num--)
//         {
//             delete _sharers[num - 1];
//         }
//     }

//     _sharerCount = 0;

//     for (uint8_t num = 0; num < BLINKER_MQTT_MAX_SHARERS_NUM; num++)
//     {
//         user_name = root["users"][num].as<String>();

//         if (user_name.length() >= BLINKER_MQTT_USER_UUID_SIZE)
//         {
//             BLINKER_LOG_ALL(BLINKER_F("sharer uuid: "), user_name, BLINKER_F(", length: "), user_name.length());

//             _sharerCount++;

//             _sharers[num] = new BlinkerSharer(user_name);
//         }
//         else
//         {
//             break;
//         }
//     }
// }

// int BlinkerMQTTLP::connectServer() {
//     const int httpsPort = 443;
// #if defined(ESP8266)
//     String host = BLINKER_F(BLINKER_SERVER_HOST);
//     String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");

//  // WiFiClientSecure client_s;

// //     BearSSL::WiFiClientSecure *client_s;

// //     client_s = new BearSSL::WiFiClientSecure();

// //     BLINKER_LOG_ALL(BLINKER_F("connecting to "), host);

// //     // BLINKER_LOG_FreeHeap();

// //     uint8_t connet_times = 0;
// //     // client_s.stop();
// //     ::delay(100);

// //     // bool mfln = client_s.probeMaxFragmentLength(host, httpsPort, 1024);
// //     // if (mfln) {
// //     //     client_s.setBufferSizes(1024, 1024);
// //     // }
// //     client_s->setFingerprint(fingerprint.c_str());

// //     client_s->setInsecure();

// //     // while (1) {
// //         bool cl_connected = false;
// //         if (!client_s->connect(host, httpsPort)) {
// //             BLINKER_ERR_LOG(BLINKER_F("server connection failed"));
// //             // connet_times++;

// //             ::delay(1000);
// //         }
// //         else {
// //             BLINKER_LOG_ALL(BLINKER_F("connection succeed"));
// //             cl_connected = true;

// //             // break;
// //         }

// //         // if (connet_times >= 4 && !cl_connected)  return BLINKER_CMD_FALSE;
// //     // }

// //     String client_msg;

// //     String url_iot = BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
// //     url_iot += _authKey;
// //     url_iot += _aliType;
// //     url_iot += _duerType;

// // // #if defined(BLINKER_ALIGENIE_LIGHT)
// // //     url_iot += BLINKER_F("&aliType=light");
// // // #elif defined(BLINKER_ALIGENIE_OUTLET)
// // //     url_iot += BLINKER_F("&aliType=outlet");
// // // #elif defined(BLINKER_ALIGENIE_SWITCH)
// // // #elif defined(BLINKER_ALIGENIE_SENSOR)
// // //     url_iot += BLINKER_F("&aliType=sensor");
// // // #endif

// //     BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), host, url_iot);

// //     client_msg = BLINKER_F("GET ");
// //     client_msg += url_iot;
// //     client_msg += BLINKER_F(" HTTP/1.1\r\nHost: ");
// //     client_msg += host;
// //     client_msg += BLINKER_F(":");
// //     client_msg += STRING_format(httpsPort);
// //     client_msg += BLINKER_F("\r\nConnection: close\r\n\r\n");

// //     client_s->print(client_msg);

// //     BLINKER_LOG_ALL(BLINKER_F("client_msg: "), client_msg);

// //     unsigned long timeout = millis();
// //     while (client_s->available() == 0) {
// //         if (millis() - timeout > 5000) {
// //             BLINKER_LOG_ALL(BLINKER_F(">>> Client Timeout !"));
// //             client_s->stop();
// //             return false;
// //         }
// //     }

// //     String _dataGet;
// //     String lastGet;
// //     String lengthOfJson;
// //     while (client_s->available()) {
// //         // String line = client_s.readStringUntil('\r');
// //         _dataGet = client_s->readStringUntil('\n');

// //         if (_dataGet.startsWith("Content-Length: ")){
// //             int addr_start = _dataGet.indexOf(' ');
// //             int addr_end = _dataGet.indexOf('\0', addr_start + 1);
// //             lengthOfJson = _dataGet.substring(addr_start + 1, addr_end);
// //         }

// //         if (_dataGet == "\r") {
// //             BLINKER_LOG_ALL(BLINKER_F("headers received"));

// //             break;
// //         }
// //     }

// //     for(int i=0;i<lengthOfJson.toInt();i++){
// //         lastGet += (char)client_s->read();
// //     }

// //     // BLINKER_LOG_FreeHeap();

// //     client_s->stop();
// //     client_s->flush();

// //     free(client_s);

// //     // BLINKER_LOG_FreeHeap();

// //     _dataGet = lastGet;

// //     BLINKER_LOG_ALL(BLINKER_F("_dataGet: "), _dataGet);

// //     String payload = _dataGet;



//     client_mqtt.stop();

//     std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

//     // client_s->setFingerprint(fingerprint);
//     client_s->setInsecure();

//     String url_iot = BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
//     url_iot += _authKey;
//     url_iot += _aliType;
//     url_iot += _duerType;

//     url_iot = "https://" + host + url_iot;

//     HTTPClient http;

//     String payload;

//     BLINKER_LOG_ALL(BLINKER_F("[HTTP] begin: "), url_iot);

//     if (http.begin(*client_s, url_iot)) {  // HTTPS

//         // Serial.print("[HTTPS] GET...\n");
//         // start connection and send HTTP header
//         int httpCode = http.GET();

//         // httpCode will be negative on error
//         if (httpCode > 0) {
//             // HTTP header has been send and Server response header has been handled

//             BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

//             // file found at server
//             if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
//                 payload = http.getString();
//                 // Serial.println(payload);
//             }
//         } else {
//             BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
//             payload = http.getString();
//             BLINKER_LOG(payload);
//         }

//         http.end();
//     } else {
//         // Serial.printf("[HTTPS] Unable to connect\n");
//     }

// #elif defined(ESP32)
//     String host = BLINKER_F(BLINKER_SERVER_HTTPS);
//     // const char* ca =
//     //     "-----BEGIN CERTIFICATE-----\n"
//     //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n"
//     //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n"
//     //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n"
//     //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n"
//     //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
//     //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n"
//     //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n"
//     //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n"
//     //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n"
//     //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n"
//     //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n"
//     //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n"
//     //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n"
//     //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n"
//     //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n"
//     //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n"
//     //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n"
//     //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n"
//     //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n"
//     //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n"
//     //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n"
//     //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n"
//     //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n"
//     //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n"
//     //     "9UBd0Q==\n"
//     //     "-----END CERTIFICATE-----\n";
// // #endif

//     HTTPClient http;

//     String url_iot = host;
//     url_iot += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
//     url_iot += _authKey;
//     url_iot += _aliType;
//     url_iot += _duerType;

// // #if defined(BLINKER_ALIGENIE_LIGHT)
// //     url_iot += BLINKER_F("&aliType=light");
// // #elif defined(BLINKER_ALIGENIE_OUTLET)
// //     url_iot += BLINKER_F("&aliType=outlet");
// // #elif defined(BLINKER_ALIGENIE_SWITCH)
// // #elif defined(BLINKER_ALIGENIE_SENSOR)
// //     url_iot += BLINKER_F("&aliType=sensor");
// // #endif

//     BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

// // #if defined(ESP8266)
// //     http.begin(url_iot, fingerprint); //HTTP
// // #elif defined(ESP32)
//     // http.begin(url_iot, ca); TODO
//     http.begin(url_iot);
// // #endif
//     int httpCode = http.GET();

//     String payload;

//     if (httpCode > 0) {
//       // HTTP header has been send and Server response header has been handled

//         BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

//         // file found at server
//         if (httpCode == HTTP_CODE_OK) {
//             payload = http.getString();
//             // BLINKER_LOG(payload);
//         }
//     }
//     else {
//         BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
//         payload = http.getString();
//         BLINKER_LOG(payload);
//     }

//     http.end();
// #endif

//     // payload = "";

//     BLINKER_LOG_ALL(BLINKER_F("reply was:"));
//     BLINKER_LOG_ALL(BLINKER_F("=============================="));
//     BLINKER_LOG_ALL(payload);
//     BLINKER_LOG_ALL(BLINKER_F("=============================="));

//     DynamicJsonBuffer jsonBuffer;
//     JsonObject& root = jsonBuffer.parseObject(payload);

//     if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || !root.success() ||
//         !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
//         // while(1) {
//             BLINKER_ERR_LOG(BLINKER_F("Maybe you have put in the wrong AuthKey!"));
//             BLINKER_ERR_LOG(BLINKER_F("Or maybe your request is too frequently!"));
//             BLINKER_ERR_LOG(BLINKER_F("Or maybe your network is disconnected!"));
//             // ::delay(60000);

//             return false;
//         // }
//     }

//     // String _userID = STRING_find_string(payload, "deviceName", "\"", 4);
//     // String _userName = STRING_find_string(payload, "iotId", "\"", 4);
//     // String _key = STRING_find_string(payload, "iotToken", "\"", 4);
//     // String _productInfo = STRING_find_string(payload, "productKey", "\"", 4);
//     // String _broker = STRING_find_string(payload, "broker", "\"", 4);
//     // String _uuid = STRING_find_string(payload, "uuid", "\"", 4);
//     String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
//     String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
//     String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
//     String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
//     String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
//     String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];

//     if (isMQTTinit)
//     {
//         free(MQTT_HOST_MQTT);
//         free(MQTT_ID_MQTT);
//         free(MQTT_NAME_MQTT);
//         free(MQTT_KEY_MQTT);
//         free(MQTT_PRODUCTINFO_MQTT);
//         free(UUID_MQTT);
//         free(DEVICE_NAME_MQTT);
//         free(BLINKER_PUB_TOPIC_MQTT);
//         free(BLINKER_SUB_TOPIC_MQTT);
//         free(mqtt_MQTT);
//         free(iotSub_MQTT);

//         isMQTTinit = false;
//     }

//     if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
//         // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
//         DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
//         strcpy(DEVICE_NAME_MQTT, _userID.c_str());
//         MQTT_ID_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
//         strcpy(MQTT_ID_MQTT, _userID.c_str());
//         MQTT_NAME_MQTT = (char*)malloc((_userName.length()+1)*sizeof(char));
//         strcpy(MQTT_NAME_MQTT, _userName.c_str());
//         MQTT_KEY_MQTT = (char*)malloc((_key.length()+1)*sizeof(char));
//         strcpy(MQTT_KEY_MQTT, _key.c_str());
//         MQTT_PRODUCTINFO_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
//         strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo.c_str());
//         MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
//         strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ALIYUN_HOST);
//         MQTT_PORT_MQTT = BLINKER_MQTT_ALIYUN_PORT;
//     }
//     else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
//         // String id2name = _userID.subString(10, _userID.length());
//         // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
//         DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
//         strcpy(DEVICE_NAME_MQTT, _userID.c_str());
//         String IDtest = _productInfo + _userID;
//         MQTT_ID_MQTT = (char*)malloc((IDtest.length()+1)*sizeof(char));
//         strcpy(MQTT_ID_MQTT, IDtest.c_str());
//         String NAMEtest = IDtest + ";" + _userName;
//         MQTT_NAME_MQTT = (char*)malloc((NAMEtest.length()+1)*sizeof(char));
//         strcpy(MQTT_NAME_MQTT, NAMEtest.c_str());
//         MQTT_KEY_MQTT = (char*)malloc((_key.length()+1)*sizeof(char));
//         strcpy(MQTT_KEY_MQTT, _key.c_str());
//         MQTT_PRODUCTINFO_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
//         strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo.c_str());
//         MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_QCLOUD_HOST)+1)*sizeof(char));
//         strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_QCLOUD_HOST);
//         MQTT_PORT_MQTT = BLINKER_MQTT_QCLOUD_PORT;
//     }
//     else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
//         // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
//         DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
//         strcpy(DEVICE_NAME_MQTT, _userID.c_str());
//         MQTT_ID_MQTT = (char*)malloc((_userName.length()+1)*sizeof(char));
//         strcpy(MQTT_ID_MQTT, _userName.c_str());
//         MQTT_NAME_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
//         strcpy(MQTT_NAME_MQTT, _productInfo.c_str());
//         MQTT_KEY_MQTT = (char*)malloc((_key.length()+1)*sizeof(char));
//         strcpy(MQTT_KEY_MQTT, _key.c_str());
//         MQTT_PRODUCTINFO_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
//         strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo.c_str());
//         MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ONENET_HOST)+1)*sizeof(char));
//         strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ONENET_HOST);
//         MQTT_PORT_MQTT = BLINKER_MQTT_ONENET_PORT;
//     }
//     UUID_MQTT = (char*)malloc((_uuid.length()+1)*sizeof(char));
//     strcpy(UUID_MQTT, _uuid.c_str());

//     // char uuid_eeprom[BLINKER_AUUID_SIZE];

//     // // BLINKER_LOG_ALL(("===================="));

//     // char _authCheck;
//     // EEPROM.begin(BLINKER_EEP_SIZE);
//     // EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
//     // if (strcmp(uuid_eeprom, _uuid.c_str()) != 0) {
//     //     // strcpy(UUID_MQTT, _uuid.c_str());

//     //     strcpy(uuid_eeprom, _uuid.c_str());
//     //     EEPROM.put(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
//     //     EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);

//     //     BLINKER_LOG_ALL(BLINKER_F("===================="));
//     //     BLINKER_LOG_ALL(BLINKER_F("uuid_eeprom: "), uuid_eeprom);
//     //     BLINKER_LOG_ALL(BLINKER_F("_uuid: "), _uuid);
//     //     // isNew = true;
//     // }
//     // EEPROM.get(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
//     // // if (_authCheck != BLINKER_AUTH_CHECK_DATA) {
//     // //     EEPROM.put(BLINKER_EEP_ADDR_AUTH_CHECK, BLINKER_AUTH_CHECK_DATA);
//     // //     isAuth = true;
//     // // }
//     // EEPROM.commit();
//     // EEPROM.end();

//     BLINKER_LOG_ALL(BLINKER_F("===================="));
//     BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME_MQTT: "), DEVICE_NAME_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_MQTT: "), MQTT_PRODUCTINFO_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_MQTT: "), MQTT_ID_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_MQTT: "), MQTT_NAME_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_MQTT: "), MQTT_KEY_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
//     BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("UUID_MQTT: "), UUID_MQTT);
//     BLINKER_LOG_ALL(BLINKER_F("===================="));

//     if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
//         String PUB_TOPIC_STR = BLINKER_F("/");
//         PUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
//         PUB_TOPIC_STR += BLINKER_F("/");
//         PUB_TOPIC_STR += MQTT_ID_MQTT;
//         PUB_TOPIC_STR += BLINKER_F("/s");

//         BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
//         // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
//         strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

//         BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

//         String SUB_TOPIC_STR = BLINKER_F("/");
//         SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
//         SUB_TOPIC_STR += BLINKER_F("/");
//         SUB_TOPIC_STR += MQTT_ID_MQTT;
//         SUB_TOPIC_STR += BLINKER_F("/r");

//         BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
//         // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
//         strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

//         BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);
//     }
//     else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
//         String PUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT;
//         PUB_TOPIC_STR += BLINKER_F("/");
//         PUB_TOPIC_STR += _userID;
//         PUB_TOPIC_STR += BLINKER_F("/s");

//         BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
//         // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
//         strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

//         BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

//         String SUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT;
//         SUB_TOPIC_STR += BLINKER_F("/");
//         SUB_TOPIC_STR += _userID;
//         SUB_TOPIC_STR += BLINKER_F("/r");

//         BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
//         // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
//         strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

//         BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);
//     }
//     else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
//         // uint8_t str_len;
//         String PUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT;
//         PUB_TOPIC_STR += BLINKER_F("/onenet_rule/r");
//         // str_len = PUB_TOPIC_STR.length() + 1;
//         BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
//         // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
//         strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

//         BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

//         String SUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT;
//         SUB_TOPIC_STR += BLINKER_F("/");
//         SUB_TOPIC_STR += _userID;
//         SUB_TOPIC_STR += BLINKER_F("/r");

//         BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
//         // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
//         strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

//         BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);
//     }

//     // BLINKER_LOG_FreeHeap();

//     if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
//         #if defined(ESP8266)
//             // bool mfln = client_mqtt.probeMaxFragmentLength(MQTT_HOST_MQTT, MQTT_PORT_MQTT, 4096);
//             // if (mfln) {
//             //     client_mqtt.setBufferSizes(1024, 1024);
//             // }
//             // client_mqtt.setInsecure();
//             mqtt_MQTT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
//         #elif defined(ESP32)
//             mqtt_MQTT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
//         #endif
//     }
//     else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
//         #if defined(ESP8266)
//             // bool mfln = client_mqtt.probeMaxFragmentLength(MQTT_HOST_MQTT, MQTT_PORT_MQTT, 4096);
//             // if (mfln) {
//             //     client_mqtt.setBufferSizes(1024, 1024);
//             // }
//             // client_mqtt.setInsecure();
//             mqtt_MQTT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
//         #elif defined(ESP32)
//             mqtt_MQTT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
//         #endif
//     }
//     else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
//         mqtt_MQTT = new Adafruit_MQTT_Client(&client, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
//     }

//     // iotPub = new Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);
//     // if (!isMQTTinit)
//     iotSub_MQTT = new Adafruit_MQTT_Subscribe(mqtt_MQTT, BLINKER_SUB_TOPIC_MQTT);

//     // mqtt_broker = (char*)malloc((_broker.length()+1)*sizeof(char));
//     // strcpy(mqtt_broker, _broker.c_str());
//     // mqtt_broker = _broker;

//     // mDNSInit(MQTT_ID_MQTT);
//     this->latestTime = millis() - BLINKER_MQTT_CONNECT_TIMESLOT;
//     // if (!isMQTTinit)
//     mqtt_MQTT->subscribe(iotSub_MQTT);

//     #if defined(ESP8266)
//         // client_s->stop();
//         // if (!isMQTTinit)
//         client_mqtt.setInsecure();
//     #endif
//     // connect();

//     BLINKER_LOG_FreeHeap();

//     return true;
// }

// void BlinkerMQTTLP::mDNSInit()
// {
// #if defined(ESP8266)
//     if (!MDNS.begin(DEVICE_NAME_MQTT, WiFi.localIP())) {
// #elif defined(ESP32)
//     if (!MDNS.begin(DEVICE_NAME_MQTT)) {
// #endif
//         while(1) {
//             ::delay(100);
//         }
//     }

//     BLINKER_LOG(BLINKER_F("mDNS responder started"));

//     MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
//     MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME_MQTT));

//     webSocket_MQTT.begin();
//     webSocket_MQTT.onEvent(webSocketEvent_MQTT);
//     BLINKER_LOG(BLINKER_F("webSocket_MQTT server started"));
//     BLINKER_LOG(BLINKER_F("ws://"), DEVICE_NAME_MQTT, BLINKER_F(".local:"), WS_SERVERPORT);
// }

// void BlinkerMQTTLP::checkKA() {
//     if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
//         isAlive = false;
// }

// int BlinkerMQTTLP::checkAliKA() {
//     if (millis() - aliKaTime >= 10000)
//         return false;
//     else
//         return true;
// }

// int BlinkerMQTTLP::checkDuerKA() {
//     if (millis() - duerKaTime >= 10000)
//         return false;
//     else
//         return true;
// }

// int BlinkerMQTTLP::checkCanPrint() {
//     if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
//         return true;
//     }
//     else {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

//         checkKA();

//         return false;
//     }
// }

// int BlinkerMQTTLP::checkCanBprint() {
//     if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
//         return true;
//     }
//     else {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

//         return false;
//     }
// }

// int BlinkerMQTTLP::checkPrintSpan() {
//     if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
//         if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
//             BLINKER_ERR_LOG(BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));

//             return false;
//         }
//         else {
//             respTimes++;
//             return true;
//         }
//     }
//     else {
//         respTimes = 0;
//         return true;
//     }
// }

// int BlinkerMQTTLP::checkAliPrintSpan()
// {
//     if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2)
//     {
//         if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2)
//         {
//             BLINKER_ERR_LOG(BLINKER_F("ALIGENIE NOT ALIVE OR MSG LIMIT"));

//             return false;
//         }
//         else
//         {
//             respAliTimes++;
//             return true;
//         }
//     }
//     else
//     {
//         respAliTimes = 0;
//         return true;
//     }
// }

// int BlinkerMQTTLP::checkDuerPrintSpan()
// {
//     if (millis() - respDuerTime < BLINKER_PRINT_MSG_LIMIT/2)
//     {
//         if (respDuerTimes > BLINKER_PRINT_MSG_LIMIT/2)
//         {
//             BLINKER_ERR_LOG(BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

//             return false;
//         }
//         else
//         {
//             respDuerTimes++;
//             return true;
//         }
//     }
//     else
//     {
//         respDuerTimes = 0;
//         return true;
//     }
// }

// int BlinkerMQTTLP::isJson(const String & data)
// {
//     BLINKER_LOG_ALL(BLINKER_F("isJson: "), data);

//     DynamicJsonBuffer jsonBuffer;
//     JsonObject& root = jsonBuffer.parseObject(STRING_format(data));

//     if (!root.success())
//     {
//         BLINKER_ERR_LOG("Print data is not Json! ", data);
//         return false;
//     }

//     return true;
// }

// bool BlinkerMQTTLP::checkInit()
// {
//     if (!_isWiFiInit)
//     {
//         switch (_configType)
//         {
//             case COMM :
//                 _connectTime = millis();
//                 // BLINKER_LOG(BLINKER_F("checkInit..."));
//                 if (WiFi.status() != WL_CONNECTED) {
//                     ::delay(500);

//                     if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
//                         // _connectTime = millis();
//                         BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
//                         BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
//                         return false;
//                     }

//                     // BLINKER_LOG(BLINKER_F("checkInit..."));

//                     return false;
//                 }
//                 BLINKER_LOG(BLINKER_F("WiFi Connected."));
//                 BLINKER_LOG(BLINKER_F("IP Address: "));
//                 BLINKER_LOG(WiFi.localIP());
//                 _isWiFiInit = true;
//                 _connectTime = 0;

//                 // begin();

//                 return false;
//             case BLINKER_SMART_CONFIG :
//                 switch (_configStatus)
//                 {
//                     case SMART_BEGIN :
//                         if (WiFi.smartConfigDone())
//                         {
//                             BLINKER_LOG(BLINKER_F("SmartConfig received."));
//                             _connectTime = millis();

//                             #if defined(ESP8266)
//                                 BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
//                             #endif

//                             _configStatus = SMART_DONE;
//                         }
//                         else return false;
//                     case SMART_DONE :
//                         if (WiFi.status() != WL_CONNECTED)
//                         {
//                             if (millis() - _connectTime > 15000)
//                             {
//                                 BLINKER_LOG(BLINKER_F("SmartConfig timeout."));
//                                 WiFi.stopSmartConfig();
//                                 _configStatus = SMART_TIMEOUT;
//                             }
//                             return false;
//                         }
//                         else if (WiFi.status() == WL_CONNECTED)
//                         {
//                             // WiFi.stopSmartConfig();

//                             BLINKER_LOG(BLINKER_F("WiFi Connected."));
//                             BLINKER_LOG(BLINKER_F("IP Address: "));
//                             BLINKER_LOG(WiFi.localIP());
//                             _isWiFiInit = true;
//                             _connectTime = 0;

//                             // begin();
                            
//                             return false;
//                         }
//                         // return false;
//                     case SMART_TIMEOUT :
//                         WiFi.beginSmartConfig();
//                         _configStatus = SMART_BEGIN;
//                         BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
//                         return false;
//                     default :
//                         return false;
//                 }
//             case BLINKER_AP_CONFIG :
//                 #if defined(BLINKER_APCONFIG)
//                 switch (_configStatus)
//                 {
//                     case APCFG_BEGIN :
//                         checkAPCFG();
//                         return false;
//                     case APCFG_DONE :
//                         if (WiFi.status() != WL_CONNECTED)
//                         {
//                             if (millis() - _connectTime > 15000)
//                             {
//                                 BLINKER_LOG(BLINKER_F("APConfig timeout."));
//                                 _configStatus = APCFG_TIMEOUT;
//                             }
//                             return false;
//                         }
//                         else if (WiFi.status() == WL_CONNECTED)
//                         {
//                             BLINKER_LOG(BLINKER_F("WiFi Connected."));
//                             BLINKER_LOG(BLINKER_F("IP Address: "));
//                             BLINKER_LOG(WiFi.localIP());
//                             _isWiFiInit = true;
//                             _connectTime = 0;

//                             // begin();
                            
//                             return false;
//                         }
//                         // return false;
//                     case APCFG_TIMEOUT :
//                         softAPinit();
//                         return false;
//                     default :
//                         return false;
//                 }                
//                 #endif
//             default :
//                 return false;
//         }
//     }

//     if (!_isBegin)
//     {
//         _isBegin = begin();
//         return _isBegin;
//     }
//     return true;
// }

// void BlinkerMQTTLP::commonBegin(const char* _ssid, const char* _pswd)
// {
//     _configType = COMM;

//     connectWiFi(_ssid, _pswd);

//     #if defined(ESP8266)
//         BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
//     #elif defined(ESP32)
//         BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
//     #endif
// }

// void BlinkerMQTTLP::smartconfigBegin()
// {
//     _configType = BLINKER_SMART_CONFIG;

//     if (!autoInit()) smartconfig();
//     else _configStatus = SMART_DONE;

//     #if defined(ESP8266)
//         BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
//     #elif defined(ESP32)
//         BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
//     #endif
// }

// void BlinkerMQTTLP::apconfigBegin()
// {
//     #if defined(BLINKER_APCONFIG)
//     _configType = BLINKER_AP_CONFIG;

//     if (!autoInit()) softAPinit();
//     else _configStatus = APCFG_DONE;

//     #if defined(ESP8266)
//         BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
//     #elif defined(ESP32)
//         BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
//     #endif
//     #endif
// }

// bool BlinkerMQTTLP::autoInit()
// {
//     WiFi.mode(WIFI_STA);
//     String _hostname = BLINKER_F("DiyArduino_");
//     _hostname += macDeviceName();

//     #if defined(ESP8266)
//         WiFi.hostname(_hostname.c_str());
//     #elif defined(ESP32)
//         WiFi.setHostname(_hostname.c_str());
//     #endif

//     WiFi.begin();
//     ::delay(500);

//     BLINKER_LOG(BLINKER_F("Waiting for WiFi "),
//                 BLINKER_WIFI_AUTO_INIT_TIMEOUT / 1000,
//                 BLINKER_F("s, will enter SMARTCONFIG or "),
//                 BLINKER_F("APCONFIG while WiFi not connect!"));

//     uint8_t _times = 0;
//     while (WiFi.status() != WL_CONNECTED) {
//         ::delay(500);
//         if (_times > BLINKER_WIFI_AUTO_INIT_TIMEOUT / 500) break;
//         _times++;
//     }

//     if (WiFi.status() != WL_CONNECTED) return false;
//     else {
//         // BLINKER_LOG(BLINKER_F("WiFi Connected."));
//         // BLINKER_LOG(BLINKER_F("IP Address: "));
//         // BLINKER_LOG(WiFi.localIP());
//         // _isWiFiInit = true;

//         // begin();

//         return true;
//     }
// }

// void BlinkerMQTTLP::smartconfig()
// {
//     WiFi.mode(WIFI_STA);
//     String _hostname = BLINKER_F("DiyArduino_");
//     _hostname += macDeviceName();

//     #if defined(ESP8266)
//         WiFi.hostname(_hostname.c_str());
//     #elif defined(ESP32)
//         WiFi.setHostname(_hostname.c_str());
//     #endif

//     WiFi.beginSmartConfig();

//     _configStatus = SMART_BEGIN;

//     BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
//     // while (!WiFi.smartConfigDone()) {
//     //     ::delay(500);
//     // }

//     // BLINKER_LOG(BLINKER_F("SmartConfig received."));

//     // BLINKER_LOG(BLINKER_F("Waiting for WiFi"));
//     // while (WiFi.status() != WL_CONNECTED) {
//     //     ::delay(500);
//     // }

//     // BLINKER_LOG(BLINKER_F("WiFi Connected."));

//     // BLINKER_LOG(BLINKER_F("IP Address: "));
//     // BLINKER_LOG(WiFi.localIP());
// }
// #if defined(BLINKER_APCONFIG)
// void BlinkerMQTTLP::softAPinit()
// {
//     // WiFiServer _apServer(80);
//     // WiFiClient _apClient;
//     IPAddress apIP(192, 168, 4, 1);
//     #if defined(ESP8266)
//         IPAddress netMsk(255, 255, 255, 0);
//     #endif

//     _apServer = new WiFiServer(80);

//     WiFi.mode(WIFI_AP);
//     String softAP_ssid = BLINKER_F("DiyArduino_");
//     softAP_ssid += macDeviceName();

//     #if defined(ESP8266)
//         WiFi.hostname(softAP_ssid.c_str());
//         WiFi.softAPConfig(apIP, apIP, netMsk);
//     #elif defined(ESP32)
//         WiFi.setHostname(softAP_ssid.c_str());
//         WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
//     #endif

//     WiFi.softAP(softAP_ssid.c_str(), ("12345678"));
//     delay(100);

//     _apServer->begin();
//     BLINKER_LOG(BLINKER_F("AP IP address: "), WiFi.softAPIP());
//     BLINKER_LOG(BLINKER_F("HTTP _apServer started"));
//     BLINKER_LOG(BLINKER_F("URL: http://"), WiFi.softAPIP());

//     _configStatus = APCFG_BEGIN;

//     BLINKER_LOG(BLINKER_F("Wait for APConfig"));

//     // while(WiFi.status() != WL_CONNECTED)
//     // {
//     //     // serverClient();
//     //     _apClient = _apServer->available();
//     //     // if (_apClient.status() == CLOSED)
//     //     if (!_apClient.connected())
//     //     {
//     //         _apClient.stop();
//     //         BLINKER_LOG(BLINKER_F("Connection closed on _apClient"));
//     //     }
//     //     else
//     //     {
//     //         if (_apClient.available())
//     //         {
//     //             String data = _apClient.readStringUntil('\r');

//     //             // data = data.substring(4, data.length() - 9);
//     //             _apClient.flush();

//     //             BLINKER_LOG(BLINKER_F("clientData: "), data);

//     //             if (STRING_contains_string(data, "ssid") && 
//     //                 STRING_contains_string(data, "pswd"))
//     //             {
//     //                 String msg = BLINKER_F("{\"hello\":\"world\"}");

//     //                 String s= BLINKER_F("HTTP/1.1 200 OK\r\n");
//     //                 s += BLINKER_F("Content-Type: application/json;");
//     //                 s += BLINKER_F("charset=utf-8\r\n");
//     //                 s += BLINKER_F("Content-Length: ");
//     //                 s += String(msg.length());
//     //                 s += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
//     //                 s += msg;
//     //                 s += BLINKER_F("\r\n");

//     //                 _apClient.print(s);

//     //                 _apClient.stop();

//     //                 parseUrl(data);
//     //             }
//     //         }
//     //     }
//     //     ::delay(10);
//     // }
// }

// void BlinkerMQTTLP::checkAPCFG()
// {
//     if(WiFi.status() != WL_CONNECTED)
//     {
//         // serverClient();
//         _apClient = _apServer->available();
//         // if (_apClient.status() == CLOSED)
//         if (!_apClient.connected())
//         {
//             _apClient.stop();
//             BLINKER_LOG(BLINKER_F("Connection closed on _apClient"));
//         }
//         else
//         {
//             if (_apClient.available())
//             {
//                 String data = _apClient.readStringUntil('\r');

//                 // data = data.substring(4, data.length() - 9);
//                 _apClient.flush();

//                 BLINKER_LOG(BLINKER_F("clientData: "), data);

//                 if (STRING_contains_string(data, "ssid") && \
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

//                     _apClient.print(s);

//                     _apClient.stop();

//                     parseUrl(data);
//                 }
//             }
//         }
//         ::delay(10);
//     }
// }

// // void BlinkerMQTTLP::serverClient()
// // {
// //     if (!_apClient)
// //     {
// //         _apClient = _apServer->available();
// //     }
// //     else
// //     {
// //         // if (_apClient.status() == CLOSED)
// //         if (!_apClient.connected())
// //         {
// //             _apClient.stop();
// //             BLINKER_LOG(BLINKER_F("Connection closed on _apClient"));
// //         }
// //         else
// //         {
// //             if (_apClient.available())
// //             {
// //                 String data = _apClient.readStringUntil('\r');

// //                 // data = data.substring(4, data.length() - 9);
// //                 _apClient.flush();

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

// //                     _apClient.print(s);

// //                     _apClient.stop();

// //                     parseUrl(data);
// //                 }
// //             }
// //         }
// //     }
// // }

// bool BlinkerMQTTLP::parseUrl(String data)
// {
//     BLINKER_LOG(BLINKER_F("APCONFIG data: "), data);
//     DynamicJsonBuffer jsonBuffer;
//     JsonObject& wifi_data = jsonBuffer.parseObject(data);

//     if (!wifi_data.success()) {
//         return false;
//     }

//     String _ssid = wifi_data["ssid"];
//     String _pswd = wifi_data["pswd"];

//     BLINKER_LOG(BLINKER_F("ssid: "), _ssid);
//     BLINKER_LOG(BLINKER_F("pswd: "), _pswd);

//     free(_apServer);

//     connectWiFi(_ssid, _pswd);

//     _configStatus = APCFG_DONE;
//     _connectTime = millis();
//     return true;
// }
// #endif
// void BlinkerMQTTLP::connectWiFi(String _ssid, String _pswd)
// {
//     connectWiFi(_ssid.c_str(), _pswd.c_str());
// }

// void BlinkerMQTTLP::connectWiFi(const char* _ssid, const char* _pswd)
// {
//     uint32_t _connectTime = millis();

//     BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

//     WiFi.mode(WIFI_STA);
//     String _hostname = BLINKER_F("DiyArduinoMQTT_");
//     _hostname += macDeviceName();

//     #if defined(ESP8266)
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

//     // while (WiFi.status() != WL_CONNECTED) {
//     //     ::delay(50);

//     //     if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
//     //         _connectTime = millis();
//     //         BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
//     //         BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
//     //     }
//     // }
//     // BLINKER_LOG(BLINKER_F("Connected"));

//     // IPAddress myip = WiFi.localIP();
//     // BLINKER_LOG(BLINKER_F("Your IP is: "), myip);

//     // _isWiFiInit = true;
// }

// #endif

// #endif
