#ifndef BlinkerMQTT_AT_H
#define BlinkerMQTT_AT_H

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
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe *iotSub;

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

static char msgBuf[BLINKER_MAX_READ_SIZE];
static bool isConnect = false;
static bool isAvail = false;
static uint8_t ws_num = 0;
static uint8_t dataFrom = BLINKER_MSG_FROM_MQTT;



#include <SoftwareSerial.h>
#include "HardwareSerial.h"
// #include <Blinker/BlinkerProtocol.h>
#include <EEPROM.h>

SoftwareSerial *SSerialBLE;
// HardwareSerial *HSerialBLE;



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
                webSocket.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num = num;

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

            dataFrom = BLINKER_MSG_FROM_WS;

            ws_num = num;

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

class BlinkerTransportStream
{
    public :
        BlinkerTransportStream()
            : stream(NULL), s_isConnect(false), authkey(NULL)
        {}

        bool serialAvailable()
        {
            if (!isHWS) {
                if (!SSerialBLE->isListening()) {
                    SSerialBLE->listen();
                    ::delay(100);
                }
            }
            
            if (stream->available()) {
                strcpy(streamData, (stream->readStringUntil('\n')).c_str());
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG2(BLINKER_F("handleSerial: "), streamData);
#endif
                return true;
            }
            else {
                return false;
            }
        }

        void begin(Stream& s, bool state)
        {
            stream = &s;
            stream->setTimeout(BLINKER_STREAM_TIMEOUT);
            isHWS = state;
        }

        String serialLastRead() { return STRING_format(streamData); }

        bool serialPrint(String s)
        {
            // bool state = STRING_contains_string(s, BLINKER_CMD_NOTICE);

            // if (!state) {
            //     state = (STRING_contains_string(s, BLINKER_CMD_STATE) 
            //         && STRING_contains_string(s, BLINKER_CMD_CONNECTED));
            // }

            // if (!state) {
            //     if (!checkPrintSpan()) {
            //         respTime = millis();
            //         return false;
            //     }
            // }

            // respTime = millis();

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
            if(serialConnected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
                stream->println(s);
                return true;
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif
                return false;
            }
        }

        bool serialConnect()
        {
            s_isConnect = true;
            return serialConnected();
        }

        bool serialConnected() { return s_isConnect; }

        void serialDisconnect() { s_isConnect = false; }



        bool connect();

        bool connected() { 
            if (!isMQTTinit) {
                return *isHandle;
            }

            return mqtt->connected()||*isHandle; 
        }

        bool mConnected() {
            if (!isMQTTinit) return false;
            else return mqtt->connected();
        }

        void disconnect() {
            mqtt->disconnect();

            if (*isHandle)
                webSocket.disconnect();
        }
        void ping();
        
        bool available() {
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

        bool aligenieAvail() {
            if (isAliAvail) {
                isAliAvail = false;

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

#if defined(BLINKER_ALIGENIE)
        bool aliPrint(String data);
#endif

        void begin(const char* auth) {
            authkey = auth;
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("authkey: ", auth);
#endif
            // if (connectServer()) {
            //     mDNSInit();
            //     isMQTTinit = true;
            // }
            // else {
                uint32_t re_time = millis();
                bool isConnect = true;
                while(1) {
                    re_time = millis();
                    // ::delay(10000);
                    // BLINKER_ERR_LOG1("Maybe you have put in the wrong AuthKey!");
                    // BLINKER_ERR_LOG1("Or maybe your request is too frequently!");
                    // BLINKER_ERR_LOG1("Or maybe your network is disconnected!");
                    if (connectServer()) {
                        mDNSInit();
                        isMQTTinit = true;
                        return;
                    }
                    // delay(10000);
                    while ((millis() - re_time) < 10000) {
                        if (WiFi.status() != WL_CONNECTED && isConnect) {
                            isConnect = false;
                            WiFi.begin();
                            WiFi.reconnect();
                        }
                        else if (WiFi.status() == WL_CONNECTED && !isConnect) {
                            isConnect = true;
                        }
                        ::delay(10);
                        // WiFi.status();
                    }
                }
            // }
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

                    // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

                    // if (! iotPub.publish(payload.c_str())) {

                    if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(payload);
                        BLINKER_LOG1("...Failed");
#endif
                        return false;
                    }
                    else {
#ifdef BLINKER_DEBUG_ALL
                        BLINKER_LOG1(payload);
                        BLINKER_LOG1("...OK!");
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

        String deviceName() { return DEVICE_NAME;/*MQTT_ID;*/ }

        bool init() { return isMQTTinit; }

        bool reRegister() { return connectServer(); }

    private :    

        bool isMQTTinit = false;

        bool connectServer();

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
            
            MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
            MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME));

            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG1(("webSocket server started"));
            BLINKER_LOG4("ws://", DEVICE_NAME, ".local:", WS_SERVERPORT);
        }

        void checkKA() {
            if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
                isAlive = false;
        }

        bool checkAliKA() {
            if (millis() - aliKaTime >= 10000)
                return false;
            else
                return true;
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

        bool checkAliPrintSpan() {
            if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2) {
                if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2) {
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG1("ALIGENIE NOT ALIVE OR MSG LIMIT");
#endif
                    return false;
                }
                else {
                    respAliTimes++;
                    return true;
                }
            }
            else {
                respAliTimes = 0;
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
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        String      mqtt_broker;

    // protected :
        Stream* stream;
        char    streamData[BLINKER_MAX_READ_SIZE];
        bool    s_isConnect;
        bool    isHWS = false;
//         uint8_t respTimes = 0;
//         uint32_t    respTime = 0;

//         bool checkPrintSpan() {
//             if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
//                 if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_ERR_LOG1("DEVICE NOT CONNECT OR MSG LIMIT");
// #endif
//                     return false;
//                 }
//                 else {
//                     respTimes++;
//                     return true;
//                 }
//             }
//             else {
//                 respTimes = 0;
//                 return true;
//             }
//         }
};

class BlinkerMQTT_AT
    : public BlinkerProtocol<BlinkerTransportStream>
{
    typedef BlinkerProtocol<BlinkerTransportStream> Base;

    public :
        BlinkerMQTT_AT(BlinkerTransportStream& transp)
            : Base(transp)
        {}

        void begin(uint8_t ss_rx_pin = RX,
                    uint8_t ss_tx_pin = TX,
                    uint32_t ss_baud = 9600)
        {
            if (ss_rx_pin == RX && ss_tx_pin == TX){
                Base::begin();
                
                Serial.begin(ss_baud);
                this->conn.begin(Serial, true);
                
                BLINKER_LOG1(BLINKER_F("BLINKER_MQTT_AT initialized..."));
                return;
            }

            Base::begin();
            SSerialBLE = new SoftwareSerial(ss_rx_pin, ss_tx_pin);
            SSerialBLE->begin(ss_baud);
            this->conn.begin(*SSerialBLE, false);
            BLINKER_LOG1(BLINKER_F("BLINKER_MQTT_AT initialized..."));
        }

    protected :
        uint8_t at_rx_pin = RX;
        uint8_t at_tx_pin = TX;
        uint32_t at_baud = 9600;
};

#endif