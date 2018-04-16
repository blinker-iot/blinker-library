#ifndef BlinkerMQTT_H
#define BlinkerMQTT_H

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
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
    const char* host = "iotdev.clz.me";
    const char* fingerprint = "b0 79 d7 e6 b2 be a7 1f 28 65 40 db a7 6a 7d 78 83 19 85 68";
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG2("connecting to ", host);
#endif
    while (!client.connect(host, httpsPort)) {
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("connection failed");
#endif
        // return;
        ::delay(100);
    }
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("connection success");
#endif

#if defined(ESP8266)
    if (client.verify(fingerprint, host)) {
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("certificate matches");
#endif
    }
    else {
#ifdef BLINKER_DEBUG_ALL
        BLINKER_LOG1("certificate doesn't match");
#endif
    }
#endif

    String url_iot = "/user/device/auth?yK6RNFrGRJl&authkey=" + String(authkey);
#ifdef BLINKER_DEBUG_ALL    
    BLINKER_LOG2("requesting URL: ", url_iot);
#endif    
    client.print(String("GET ") + "/" + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "User-Agent: Blinker_MQTT_DEVICE\r\n" +
                    "Connection: close\r\n\r\n");
#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("request sent");
#endif
    String dataGet;
	String lastGet;
	String lengthOfJson;

    while (client.connected()) {
        dataGet = client.readStringUntil('\n');

        if (dataGet.startsWith("Content-Length: ")){
            lengthOfJson = STRING_find_string(dataGet, " ", "\0", 0);
        }

        if (dataGet == "\r") {
#ifdef BLINKER_DEBUG_ALL            
            BLINKER_LOG1("headers received");
#endif
            break;
        }
    }

    for(int i=0;i<lengthOfJson.toInt();i++){
        lastGet += (char)client.read();
    }

    dataGet = lastGet;

#ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG1("reply was:");
    BLINKER_LOG1("==============================");
    BLINKER_LOG1(dataGet);
    BLINKER_LOG1("==============================");
#endif

    String _userID = STRING_find_string(dataGet, "deviceName", "\"", 3);
    String _userName = STRING_find_string(dataGet, "iotId", "\"", 3);
    String _key = STRING_find_string(dataGet, "iotToken", "\"", 3);
    String _productInfo = STRING_find_string(dataGet, "ProductKey", "\"", 3);
    String _broker = STRING_find_string(dataGet, "broker", "\"", 3);
    String _uuid = STRING_find_string(dataGet, "uuid", "\"", 3);

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