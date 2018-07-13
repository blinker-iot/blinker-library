#ifndef BlinkerMQTT_H
#define BlinkerMQTT_H

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif
#include "Blinker/BlinkerProtocol.h"
#include "modules/WebSockets/WebSocketsServer.h"
#include "modules/mqtt/Adafruit_MQTT.h"
#include "modules/mqtt/Adafruit_MQTT_Client.h"
#include "modules/ArduinoJson/ArduinoJson.h"

static char MQTT_HOST[BLINKER_MQTT_HOST_SIZE];
static uint16_t MQTT_PORT;
// static char MQTT_DEVICEID[BLINKER_MQTT_DEVICEID_SIZE];
static char MQTT_ID[BLINKER_MQTT_ID_SIZE];
static char MQTT_NAME[BLINKER_MQTT_NAME_SIZE];
static char MQTT_KEY[BLINKER_MQTT_KEY_SIZE];
static char MQTT_PRODUCTINFO[BLINKER_MQTT_PINFO_SIZE];
static char UUID[BLINKER_MQTT_UUID_SIZE];
// static char DEVICE_NAME[BLINKER_MQTT_DEVICENAME_SIZE];
static char DEVICE_NAME[BLINKER_MQTT_DEVICEID_SIZE];
static char *BLINKER_PUB_TOPIC;
static char *BLINKER_SUB_TOPIC;

WiFiClientSecure        client_s;
WiFiClient              client;
Adafruit_MQTT_Client    *mqtt;
Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe *iotSub;

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

static char msgBuf[BLINKER_MAX_READ_SIZE];
static bool isConnect = false;
static bool isAvail = false;

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("Disconnected! ", num);
#endif

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG6("num: ", num, ", Connected from: ", ip, ", url: ", (char *)payload);
#endif

                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}");

                isConnect = true;
            }
            break;
        case WStype_TEXT:

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6("num: ", num, ", get Text: ", (char *)payload, ", length: ", length);
#endif
            if (length < BLINKER_MAX_READ_SIZE) {
                // msgBuf = (char*)malloc((length+1)*sizeof(char));
                // memcpy (msgBuf, (char*)payload, length);
                // buflen = length;
                strcpy(msgBuf, (char*)payload);
                isAvail = true;
            }

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG4("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}

class BlinkerMQTT {
    public :
        BlinkerMQTT() :
            authkey(NULL) {}
        
        bool connect();
        bool connected() { return mqtt->connected()||*isHandle; }
        void disconnect() {
            mqtt->disconnect();

            if (*isHandle)
                webSocket.disconnect();
        }
        void ping();
        
        bool available () {
            webSocket.loop();

            checkKA();

            if (!mqtt->connected() || (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT) {
                ping();
            }
            else {
                subscribe();
            }

            if (isAvail) {
                isAvail = false;

                return true;
            }
            else {
                return false;
            }
        }

        bool extraAvailable() {
            if (isBavail) {
                isBavail = false;
                
                return true;
            }
            else {
                return false;
            }
        }

        void subscribe();
        String lastRead() { return STRING_format(msgBuf); }
        bool print(String data);
        bool bPrint(String name, String data);
        
        void begin(const char* auth) {
            authkey = auth;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("authkey: ", auth);
#endif
            connectServer();
            mDNSInit();
        }

        bool autoPrint(uint32_t id) {
            String payload = "{\"data\":{\"set\":{" + \
                STRING_format("\"trigged\":true,\"autoData\":{") + \
                "\"autoId\":" + STRING_format(id)  + "}}}" + \
                ",\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"" + \
                ",\"toDevice\":\"" + "autoManager" + "\"}";
                // "\",\"deviceType\":\"" + "type" + "\"}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("autoPrint...");
#endif
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    // linkTime = millis();

                    if (! iotPub->publish(payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(payload, ("...Failed"));
#endif
                        return false;
                    }
                    else {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG2(payload, ("...OK!"));
#endif
                        linkTime = millis();
                        return true;
                    }
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG2("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
#endif
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG1("MQTT Disconnected");
                return false;
            }
        }

        bool autoPrint(char *name, char *type, char *data) {
            String payload = "{\"data\":{" + STRING_format(data) + "}," + \ 
                + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                + "\"toDevice\":\"" + name + "\"," + \
                + "\"deviceType\":\"" + type + "\"}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("autoPrint...");
#endif
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    linkTime = millis();
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif
                    return true;
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG2("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
#endif
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG1("MQTT Disconnected");
                return false;
            }
        }

        bool autoPrint(char *name1, char *type1, char *data1
            , char *name2, char *type2, char *data2)
        {
            String payload = "{\"data\":{" + STRING_format(data1) + "}," + \ 
                + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                + "\"toDevice\":\"" + name1 + "\"," + \
                + "\"deviceType\":\"" + type1 + "\"}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG1("autoPrint...");
#endif
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    linkTime = millis();
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif
                    payload = "{\"data\":{" + STRING_format(data2) + "}," + \ 
                        + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                        + "\"toDevice\":\"" + name2 + "\"," + \
                        + "\"deviceType\":\"" + type2 + "\"}";

#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif                    
                    return true;
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG2("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
#endif
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG1("MQTT Disconnected");
                return false;
            }
        }

        String deviceName() { return MQTT_ID; }

    private :    

        void connectServer();

        void mDNSInit()
        {
#if defined(ESP8266)
            if (!MDNS.begin(DEVICE_NAME, WiFi.localIP())) {
#elif defined(ESP32)
            if (!MDNS.begin(DEVICE_NAME)) {
#endif
                while(1) {
                    ::delay(100);
                }
            }

            BLINKER_LOG1(("mDNS responder started"));
            
            MDNS.addService("DiyArduinoMQTT", "tcp", WS_SERVERPORT);
            MDNS.addServiceTxt("DiyArduinoMQTT", "tcp", "deviceName", String(DEVICE_NAME));

            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG1(("webSocket server started"));
            BLINKER_LOG4("ws://", DEVICE_NAME, ".local:", WS_SERVERPORT);
        }

        void checkKA() {
            if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
                isAlive = false;
        }

        bool checkCanPrint() {
            if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("MQTT NOT ALIVE OR MSG LIMIT");
#endif
                checkKA();

                return false;
            }
        }

        bool checkCanBprint() {
            if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("MQTT NOT ALIVE OR MSG LIMIT");
#endif
                return false;
            }
        }

        bool checkPrintSpan() {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG1("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT");
#endif
                    return false;
                }
                else {
                    respTimes++;
                    return true;
                }
            }
            else {
                respTimes = 0;
                return true;
            }
        }

    protected :
        const char* authkey;
        bool*       isHandle = &isConnect;
        bool        isAlive = false;
        bool        isBavail = false;
        uint32_t    latestTime;
        uint32_t    printTime = 0;
        uint32_t    bPrintTime = 0;
        uint32_t    kaTime = 0;
        uint32_t    linkTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
};

void BlinkerMQTT::connectServer() {
    const int httpsPort = 443;
    const char* host = "https://iotdev.clz.me";
#if defined(ESP8266)
    const char* fingerprint = "84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a";
#elif defined(ESP32)
    // const char* ca = \ 
    //     "-----BEGIN CERTIFICATE-----\n" \
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n" \
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n" \
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n" \
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n" \
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n" \
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n" \
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n" \
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n" \
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n" \
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n" \
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n" \
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n" \
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n" \
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n" \
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n" \
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n" \
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n" \
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n" \
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n" \
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n" \
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n" \
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n" \
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n" \
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n" \
    //     "9UBd0Q==\n" \
    //     "-----END CERTIFICATE-----\n";
#endif

    HTTPClient http;

    String url_iot = String(host) + "/api/v1/user/device/diy/auth?authKey=" + String(authkey);
#ifdef BLINKER_DEBUG_ALL 
    BLINKER_LOG2("HTTPS begin: ", url_iot);
#endif

#if defined(ESP8266)
    http.begin(url_iot, fingerprint); //HTTP
#elif defined(ESP32)
    // http.begin(url_iot, ca); TODO
    http.begin(url_iot);
#endif
    int httpCode = http.GET();

    String payload;

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
#ifdef BLINKER_DEBUG_ALL 
        BLINKER_LOG2("[HTTP] GET... code: ", httpCode);
#endif

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            // BLINKER_LOG1(payload);
        }
    }
    else {
#ifdef BLINKER_DEBUG_ALL 
        BLINKER_LOG2("[HTTP] GET... failed, error: ", http.errorToString(httpCode).c_str());
        payload = http.getString();
        BLINKER_LOG1(payload);
#endif
    }

    http.end();

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("reply was:");
    BLINKER_LOG1("==============================");
    BLINKER_LOG1(payload);
    BLINKER_LOG1("==============================");
#endif

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || !root.success()) {
        while(1) {
            BLINKER_ERR_LOG1("May you have put in the wrong AuthKey!Or maybe your request is too frequent!");
            ::delay(10000);
        }
    }

    // String _userID = STRING_find_string(payload, "deviceName", "\"", 4);
    // String _userName = STRING_find_string(payload, "iotId", "\"", 4);
    // String _key = STRING_find_string(payload, "iotToken", "\"", 4);
    // String _productInfo = STRING_find_string(payload, "productKey", "\"", 4);
    // String _broker = STRING_find_string(payload, "broker", "\"", 4);
    // String _uuid = STRING_find_string(payload, "uuid", "\"", 4);
    String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        strcpy(MQTT_ID, _userID.c_str());
        strcpy(MQTT_NAME, _userName.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT = BLINKER_MQTT_ALIYUN_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        // String id2name = _userID.subString(10, _userID.length());
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        String IDtest = _productInfo + _userID;
        strcpy(MQTT_ID, IDtest.c_str());
        String NAMEtest = IDtest + ";" + _userName;
        strcpy(MQTT_NAME, NAMEtest.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT = BLINKER_MQTT_QCLOUD_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        strcpy(MQTT_ID, _userName.c_str());
        strcpy(MQTT_NAME, _productInfo.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ONENET_HOST);
        MQTT_PORT = BLINKER_MQTT_ONENET_PORT;
    }
    strcpy(UUID, _uuid.c_str());

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("====================");
    BLINKER_LOG2("DEVICE_NAME: ", DEVICE_NAME);
    BLINKER_LOG2("MQTT_PRODUCTINFO: ", MQTT_PRODUCTINFO);
    BLINKER_LOG2("MQTT_ID: ", MQTT_ID);
    BLINKER_LOG2("MQTT_NAME: ", MQTT_NAME);
    BLINKER_LOG2("MQTT_KEY: ", MQTT_KEY);
    BLINKER_LOG2("MQTT_BROKER: ", _broker);
    BLINKER_LOG2("HOST: ", MQTT_HOST);
    BLINKER_LOG2("PORT: ", MQTT_PORT);
    BLINKER_LOG2("UUID: ", UUID);
    BLINKER_LOG1("====================");
#endif

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        uint8_t str_len;
        String PUB_TOPIC_STR = "/" + String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = "/" + String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        uint8_t str_len;
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        uint8_t str_len;
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        mqtt = new Adafruit_MQTT_Client(&client_s, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        mqtt = new Adafruit_MQTT_Client(&client_s, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        mqtt = new Adafruit_MQTT_Client(&client, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }

    iotPub = new Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);
    iotSub = new Adafruit_MQTT_Subscribe(mqtt, BLINKER_SUB_TOPIC);

    // mDNSInit(MQTT_ID);
    this->latestTime = millis();
    mqtt->subscribe(iotSub);
    connect();
}

bool BlinkerMQTT::connect() {
    int8_t ret;

    webSocket.loop();

    if (mqtt->connected()) {
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < 5000) {
        return false;
    }

// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("Connecting to MQTT... ");
// #endif

    if ((ret = mqtt->connect()) != 0) {
        BLINKER_LOG1(mqtt->connectErrorString(ret));
        BLINKER_LOG1("Retrying MQTT connection in 5 seconds...");

        this->latestTime = millis();
        return false;
    }
// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Connected!");
// #endif

    this->latestTime = millis();

    return true;
}

void BlinkerMQTT::ping() {
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Ping!");
#endif
    if (!mqtt->ping()) {
        disconnect();
        delay(100);

        connect();
    }
    else {
        this->latestTime = millis();
    }
}

void BlinkerMQTT::subscribe() {
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt->readSubscription(10))) {
        if (subscription == iotSub) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(("Got: "), (char *)iotSub->lastread);
#endif
            // String dataGet = String((char *)iotSub->lastread);

            // DynamicJsonDocument doc;
            // deserializeJson(doc, String((char *)iotSub->lastread));
            // JsonObject& root = doc.as<JsonObject>();
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub->lastread));
	        // JsonObject& root = jsonBuffer.parseObject((char *)iotSub->lastread);

            // if (!root.success()) {
            //     BLINKER_LOG1("json test error");
            //     return;
            // }

            String _uuid = root["fromDevice"];
            String dataGet = root["data"];

            // String _uuid = STRING_find_string(dataGet, "fromDevice", "\"", 3);

            // dataGet = STRING_find_string(dataGet, BLINKER_CMD_DATA, ",\"fromDevice", 2);

            // if (dataGet.indexOf("\"") != -1 && dataGet.indexOf("\"") == 0) {
            //     dataGet = STRING_find_string(dataGet, "\"", "\"", 0);
            // }

            // BLINKER_LOG2("data: ", dataGet);
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("data: ", dataGet);
            BLINKER_LOG2("fromDevice: ", _uuid);
#endif
            if (strcmp(_uuid.c_str(), UUID) == 0) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("Authority uuid");
#endif
                kaTime = millis();
                isAvail = true;
                isAlive = true;
            }
            else {
                dataGet = String((char *)iotSub->lastread);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG2("No authority uuid, data: ", dataGet);
#endif
                // return;

                isBavail = true;
            }

            memset(msgBuf, 0, BLINKER_MAX_READ_SIZE);
            memcpy(msgBuf, dataGet.c_str(), dataGet.length());
            
            this->latestTime = millis();
        }
    }
}

bool BlinkerMQTT::print(String data) {
    if (*isHandle) {
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_SWITCH)));

            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
            respTime = millis();
        }

        if (!state) {
            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
        }

        respTime = millis();

#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1(("Succese..."));
#endif
        webSocket.broadcastTXT(data);

        return true;
// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG3("WS response: ", data, "Succese...");
// #endif
    }
    else {
        String payload;
        if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
            payload = "{\"data\":" + data.substring(0, data.length() - 1) + ",\"fromDevice\":\"" + MQTT_ID + "\",\"toDevice\":\"" + UUID + "\"}";
        }
        else {
            payload = "{\"data\":" + data + ",\"fromDevice\":\"" + MQTT_ID + "\",\"toDevice\":\"" + UUID + "\"}";
        }
    
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("MQTT Publish...");
#endif
        bool _alive = isAlive;
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_SWITCH)));

            if (!checkPrintSpan()) {
                return false;
            }
            respTime = millis();
        }

        if (mqtt->connected()) {
            if (!state) {
                if (!checkCanPrint()) {
                    if (!_alive) {
                        isAlive = false;
                    }
                    return false;
                }
            }

            if (! iotPub->publish(payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(payload, ("...Failed"));
#endif
                if (!_alive) {
                    isAlive = false;
                }
                return false;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(payload, ("...OK!"));
#endif
                printTime = millis();

                if (!_alive) {
                    isAlive = false;
                }
                return true;
            }            
        }
        else {
            BLINKER_ERR_LOG1("MQTT Disconnected");
            isAlive = false;
            return false;
        }
    }
}

bool BlinkerMQTT::bPrint(String name, String data) {
    String payload;
    if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
        payload = "{\"data\":" + data.substring(0, data.length() - 1) + ",\"fromDevice\":\"" + MQTT_ID + "\",\"toDevice\":\"" + name + "\",\"deviceType\":\"DiyBridge\"}";
    }
    else {
        payload = "{\"data\":" + data + ",\"fromDevice\":\"" + MQTT_ID + "\",\"toDevice\":\"" + name + "\",\"deviceType\":\"DiyBridge\"}";
    }

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Publish...");
#endif
    // bool _alive = isAlive;
    // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

    // if (!state) {
    //     state = (STRING_contains_string(data, BLINKER_CMD_STATE) 
    //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
    // }

    if (mqtt->connected()) {
        // if (!state) {
        if (!checkCanBprint()) {
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        // }

        if (! iotPub->publish(payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(payload, ("...Failed"));
#endif
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        else {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(payload, ("...OK!"));
#endif
            bPrintTime = millis();

            // if (!_alive) {
            //     isAlive = false;
            // }
            return true;
        }            
    }
    else {
        BLINKER_ERR_LOG1("MQTT Disconnected");
        // isAlive = false;
        return false;
    }
    // }
}

#endif