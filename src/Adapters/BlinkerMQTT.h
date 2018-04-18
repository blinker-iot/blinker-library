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

static char MQTT_HOST[BLINKER_MQTT_HOST_SIZE];
static uint16_t MQTT_PORT;
static char MQTT_ID[BLINKER_MQTT_ID_SIZE];
static char MQTT_NAME[BLINKER_MQTT_NAME_SIZE];
static char MQTT_KEY[BLINKER_MQTT_KEY_SIZE];
static char MQTT_PRODUCTINFO[BLINKER_MQTT_PINFO_SIZE];
static char UUID[BLINKER_MQTT_UUID_SIZE];
static char DEVICE_NAME[BLINKER_MQTT_DEVICENAME_SIZE];
static char *BLINKER_PUB_TOPIC;
static char *BLINKER_SUB_TOPIC;

WiFiClientSecure        client;
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

        void subscribe();
        String lastRead() { return STRING_format(msgBuf); }
        void print(String data);
        
        void begin(const char* auth) {
            authkey = auth;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("authkey: ", auth);
#endif
            connectServer();
            mDNSInit();
        }

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
            
            MDNS.addService("DiyArduino", "tcp", WS_SERVERPORT);

            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG1(BLINKER_F("webSocket server started"));
            BLINKER_LOG4("ws://", DEVICE_NAME, ".local:", WS_SERVERPORT);
        }

    protected :
        const char* authkey;
        bool* isHandle = &isConnect;
        uint32_t latestTime;
        uint32_t printTime;
        uint32_t kaTime;
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
            BLINKER_LOG1(payload);
        }
    }
    else {
#ifdef BLINKER_DEBUG_ALL 
        BLINKER_LOG2("[HTTP] GET... failed, error: ", http.errorToString(httpCode).c_str());
#endif
    }

    http.end();

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("reply was:");
    BLINKER_LOG1("==============================");
    BLINKER_LOG1(payload);
    BLINKER_LOG1("==============================");
#endif

    String _userID = STRING_find_string(payload, "deviceName", "\"", 4);
    String _userName = STRING_find_string(payload, "iotId", "\"", 4);
    String _key = STRING_find_string(payload, "iotToken", "\"", 4);
    String _productInfo = STRING_find_string(payload, "ProductKey", "\"", 4);
    String _broker = STRING_find_string(payload, "broker", "\"", 4);
    String _uuid = STRING_find_string(payload, "uuid", "\"", 4);

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(MQTT_ID, _userID.c_str());
        strcpy(MQTT_NAME, _userName.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT = BLINKER_MQTT_ALIYUN_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        memcpy(DEVICE_NAME, _userID.c_str(), 12);
        String IDtest = _productInfo + _userID;
        strcpy(MQTT_ID, IDtest.c_str());
        String NAMEtest = IDtest + ";12010126;12345";
        strcpy(MQTT_NAME, NAMEtest.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT = BLINKER_MQTT_QCLOUD_PORT;
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
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
#endif
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG2("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
#endif
    }


    mqtt = new Adafruit_MQTT_Client(&client, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
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
    if ((millis() - latestTime) < 5000) {
        return false;
    }

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("Connecting to MQTT... ");
#endif

    if ((ret = mqtt->connect()) != 0) {
        BLINKER_LOG1(mqtt->connectErrorString(ret));
        BLINKER_LOG1("Retrying MQTT connection in 5 seconds...");

        this->latestTime = millis();
        return false;
    }
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("MQTT Connected!");
#endif

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
            String dataGet = String((char *)iotSub->lastread);

            String _uuid = STRING_find_string(dataGet, "fromDevice", "\"", 3);
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("fromDevice: ", _uuid);
#endif
            if (strcmp(_uuid.c_str(), UUID) == 0) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1("Authority uuid");
#endif
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("No authority uuid");
#endif
                // return;
            }

            memset(msgBuf, 0, BLINKER_MAX_READ_SIZE);
            memcpy(msgBuf, (char *)iotSub->lastread, iotSub->datalen);
            
            this->latestTime = millis();
            kaTime = millis();

            isAvail = true;
        }
    }
}

void BlinkerMQTT::print(String data) {
    if (*isHandle) {
        webSocket.broadcastTXT(data);
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG3("WS response: ", data, "Succese...");
#endif
    }
    else {
        String payload = "{\"data\":" + data.substring(0, data.length() - 1) + ",\"fromDevice\":\"" + MQTT_ID + "\",\"toDevice\":\"" + UUID + "\"}";
    
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("MQTT Publish...");
#endif

        if (mqtt->connected()) {
            if (millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && millis() - kaTime < BLINKER_MQTT_KEEPALIVE) {
                if (! iotPub->publish(payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...Failed"));
#endif
                }
                else {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_LOG2(payload, ("...OK!"));
#endif
                    printTime = millis();
                }
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_ERR_LOG1("MQTT MSG LIMIT or NOT ALIVE");
#endif
            }
        }
        else {
            BLINKER_ERR_LOG1("MQTT Disconnected");
        }
    }
}

#endif