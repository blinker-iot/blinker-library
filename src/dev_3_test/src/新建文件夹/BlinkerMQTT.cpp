#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif

#include "modules/WebSockets/WebSocketsServer.h"
#include "modules/mqtt/Adafruit_MQTT.h"
#include "modules/mqtt/Adafruit_MQTT_Client.h"
#include "modules/ArduinoJson/ArduinoJson.h"

#include "BlinkerMQTT.h"
#include "BlinkerConfig.h"
#include "BlinkerDebug.h"
#include "BlinkerUtility.h"

char*       MQTT_HOST;
uint16_t    MQTT_PORT;
char*       MQTT_ID;
char*       MQTT_NAME;
char*       MQTT_KEY;
char*       MQTT_PRODUCTINFO;
char*       UUID;
char*       DEVICE_NAME;
char*       BLINKER_PUB_TOPIC;
char*       BLINKER_SUB_TOPIC;

#if defined(ESP8266)
BearSSL::WiFiClientSecure   client_mqtt;
// WiFiClientSecure        client_mqtt;
#elif defined(ESP32)
WiFiClientSecure            client_s;
#endif
// WiFiClient              client;
Adafruit_MQTT_Client*       mqtt;
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe*    iotSub;

#define WS_SERVERPORT       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

char*   msgBuf;
bool    isFresh = false;
bool    isConnect = false;
bool    isAvail = false;
uint8_t ws_num = 0;
uint8_t dataFrom = BLINKER_MSG_FROM_MQTT;

void webSocketEvent(uint8_t num, WStype_t type, \
                    uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                
                BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                                BLINKER_F(", Connected from: "), ip, 
                                BLINKER_F(", url: "), (char *)payload);
                
                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num = num;

                isConnect = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                            BLINKER_F(", get Text: "), (char *)payload, \
                            BLINKER_F(", length: "), length);
            
            if (length < BLINKER_MAX_READ_SIZE) {
                if (!isFresh) msgBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                // msgBuf = (char*)malloc((length+1)*sizeof(char));
                // memcpy (msgBuf, (char*)payload, length);
                // buflen = length;
                strcpy(msgBuf, (char*)payload);
                isAvail = true;
                isFresh = true;
            }

            dataFrom = BLINKER_MSG_FROM_WS;

            ws_num = num;

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}
// Serial
BlinkerMQTT::BlinkerMQTT() {}//{ isHandle = &isConnect; }
// {
//     isHandle = &isConnect;
// }

bool BlinkerMQTT::connect()
{
    int8_t ret;

    // webSocket.loop();

    if (mqtt->connected())
    {
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < 5000)
    {
        return false;
    }

// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));
#if defined(ESP8266)
    client_mqtt.setInsecure();
#endif
// #endif

    if ((ret = mqtt->connect()) != 0)
    {
        BLINKER_LOG(mqtt->connectErrorString(ret));
        BLINKER_LOG(BLINKER_F("Retrying MQTT connection in 5 seconds..."));

        this->latestTime = millis();
        return false;
    }
// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG(BLINKER_F("MQTT Connected!"));
// #endif

    BLINKER_LOG_FreeHeap();

    this->latestTime = millis();

    return true;
}

bool BlinkerMQTT::connected()
{ 
    // if (!isMQTTinit)
    // {
    //     return *isHandle;
    // }

    return mqtt->connected();// || *isHandle; 
}

void BlinkerMQTT::disconnect()
{
    mqtt->disconnect();

    // if (*isHandle) webSocket.disconnect();
}

void BlinkerMQTT::ping()
{
    BLINKER_LOG_ALL(BLINKER_F("MQTT Ping!"));

    if (!mqtt->ping())
    {
        disconnect();
        delay(100);

        connect();
    }
    else
    {
        this->latestTime = millis();
    }
}

bool BlinkerMQTT::available()
{
    // webSocket.loop();

    checkKA();

    if (!mqtt->connected() || \
        (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
    {
        ping();
    }
    else
    {
        subscribe();
    }

    if (isAvail)
    {
        isAvail = false;
        return true;
    }
    else {
        return false;
    }
}

bool BlinkerMQTT::aligenieAvail()
{
    if (isAliAvail)
    {
        isAliAvail = false;
        return true;
    }
    else {
        return false;
    }
}

bool BlinkerMQTT::extraAvailable()
{
    if (isBavail)
    {
        isBavail = false;        
        return true;
    }
    else
    {
        return false;
    }
}

void BlinkerMQTT::subscribe()
{
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt->readSubscription(10)))
    {
        if (subscription == iotSub)
        {
            BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub->lastread);

            // String dataGet = String((char *)iotSub->lastread);

            // DynamicJsonDocument doc;
            // deserializeJson(doc, String((char *)iotSub->lastread));
            // JsonObject& root = doc.as<JsonObject>();
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub->lastread));
	        // JsonObject& root = jsonBuffer.parseObject((char *)iotSub->lastread);

            // if (!root.success()) {
            //     BLINKER_LOG("json test error");
            //     return;
            // }

            String _uuid = root["fromDevice"];
            String dataGet = root["data"];

            // String _uuid = STRING_find_string(dataGet, "fromDevice", "\"", 3);

            // dataGet = STRING_find_string(dataGet, BLINKER_CMD_DATA, ",\"fromDevice", 2);

            // if (dataGet.indexOf("\"") != -1 && dataGet.indexOf("\"") == 0) {
            //     dataGet = STRING_find_string(dataGet, "\"", "\"", 0);
            // }

            // BLINKER_LOG("data: ", dataGet);
            
            BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
            BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);
            
            if (strcmp(_uuid.c_str(), UUID) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));
                
                kaTime = millis();
                isAvail = true;
                isAlive = true;
            }
            else if (_uuid == BLINKER_CMD_ALIGENIE)
            {
                BLINKER_LOG_ALL(BLINKER_F("form AliGenie"));
                
                aliKaTime = millis();
                isAliAlive = true;
                isAliAvail = true;
            }
            else
            {
                dataGet = String((char *)iotSub->lastread);
                
                BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid, \
                                    check is from bridge/share device, \
                                    data: "), dataGet);
                
                // return;

                isBavail = true;
            }

            // memset(msgBuf, 0, BLINKER_MAX_READ_SIZE);
            // memcpy(msgBuf, dataGet.c_str(), dataGet.length());

            if (!isFresh) msgBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
            strcpy(msgBuf, dataGet.c_str());
            isFresh = true;
            
            this->latestTime = millis();

            dataFrom = BLINKER_MSG_FROM_MQTT;
        }
    }
}

char * BlinkerMQTT::lastRead() { return isFresh ? msgBuf : NULL; }

void BlinkerMQTT::flush()
{
    if (isFresh)
    {
        free(msgBuf); isFresh = false; isAvail = false;
        isAliAvail = false; isBavail = false;
    }
}

bool BlinkerMQTT::print(String data)
{
    if (*isHandle && dataFrom == BLINKER_MSG_FROM_WS)
    {
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state)
        {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) && \
                    STRING_contains_string(data, BLINKER_CMD_ONLINE)) || \
                    (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan())
            {
                respTime = millis();
                return false;
            }
            respTime = millis();
        }

        if (!state)
        {
            if (!checkPrintSpan())
            {
                respTime = millis();
                return false;
            }
        }

        respTime = millis();
        
        BLINKER_LOG_ALL(BLINKER_F("WS response: "));
        BLINKER_LOG_ALL(data);
        BLINKER_LOG_ALL(BLINKER_F("Succese..."));
        
        // webSocket.sendTXT(ws_num, data + BLINKER_CMD_NEWLINE);

        return true;
// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG("WS response: ", data, "Succese...");
// #endif
    }
    else
    {
        String payload;
        if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
        {
            payload = BLINKER_F("{\"data\":");
            payload += data.substring(0, data.length() - 1);
            payload += BLINKER_F(",\"fromDevice\":\"");
            payload += MQTT_ID;
            payload += BLINKER_F("\",\"toDevice\":\"");
            payload += UUID;
            payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        }
        else
        {
            payload = BLINKER_F("{\"data\":");
            payload += data;
            payload += BLINKER_F(",\"fromDevice\":\"");
            payload += MQTT_ID;
            payload += BLINKER_F("\",\"toDevice\":\"");
            payload += UUID;
            payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        }
        
        BLINKER_LOG_ALL(BLINKER_F("MQTT Publish..."));
        
        bool _alive = isAlive;
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state)
        {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) && \
                    STRING_contains_string(data, BLINKER_CMD_ONLINE)) || \
                    (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan())
            {
                return false;
            }
            respTime = millis();
        }

// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG("state: ", state);

//         BLINKER_LOG("state: ", STRING_contains_string(data, BLINKER_CMD_TIMING));

//         BLINKER_LOG("state: ", data.indexOf(BLINKER_CMD_TIMING));

//         BLINKER_LOG("data: ", data);
// #endif

        if (mqtt->connected())
        {
            if (!state)
            {
                if (!checkCanPrint())
                {
                    if (!_alive)
                    {
                        isAlive = false;
                    }
                    return false;
                }
            }

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

            // if (!iotPub.publish(payload.c_str())) {

            if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str()))
            {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                
                if (!_alive)
                {
                    isAlive = false;
                }
                return false;
            }
//             else if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET) {
//                 char buf[BLINKER_MAX_SEND_BUFFER_SIZE];
//                 buf[0] = 0x01;
//                 buf[1] = 0x00;
//                 buf[2] = (uint8_t)payload.length();

//                 memcpy(buf+3, payload.c_str(), payload.length());

//                 if (!iotPub.publish((uint8_t *)buf, payload.length()+3)) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG(payload);
//                     BLINKER_LOG("...Failed");
// #endif
//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return false;
//                 } else {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG(payload);
//                     BLINKER_LOG("...OK!");
// #endif
//                     if (!state) printTime = millis();

//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return true;
//                 }
//             }
            else
            {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                
                if (!state) printTime = millis();

                if (!_alive)
                {
                    isAlive = false;
                }
                return true;
            }            
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
            isAlive = false;
            return false;
        }
    }
}

bool BlinkerMQTT::bPrint(String name, String data)
{
    String payload;
    if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
    {
        payload = BLINKER_F("{\"data\":");
        payload += data.substring(0, data.length() - 1);
        payload += BLINKER_F(",\"fromDevice\":\"");
        payload += MQTT_ID;
        payload += BLINKER_F("\",\"toDevice\":\"");
        payload += name;
        payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    }
    else
    {
        payload = BLINKER_F("{\"data\":");
        payload += data;
        payload += BLINKER_F(",\"fromDevice\":\"");
        payload += MQTT_ID;
        payload += BLINKER_F("\",\"toDevice\":\"");
        payload += name;
        payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    }

    BLINKER_LOG_ALL(BLINKER_F("MQTT Publish..."));

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

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

        // if (! iotPub.publish(payload.c_str())) {

        String bPubTopic = BLINKER_F("");

        if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET)
        {
            bPubTopic = STRING_format(MQTT_PRODUCTINFO);
            bPubTopic += BLINKER_F("/");
            bPubTopic += name;
            bPubTopic += BLINKER_F("/r");
        }
        else
        {
            bPubTopic = STRING_format(BLINKER_PUB_TOPIC);
        }

        if (! mqtt->publish(bPubTopic.c_str(), payload.c_str()))
        {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            
            bPrintTime = millis();

            // if (!_alive) {
            //     isAlive = false;
            // }
            return true;
        }            
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        // isAlive = false;
        return false;
    }
    // }
}

bool BlinkerMQTT::aliPrint(String data)
{
    String payload;

    payload = BLINKER_F("{\"data\":");
    payload += data;
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += MQTT_ID;
    payload += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    payload += BLINKER_F(",\"deviceType\":\"vAssistant\"}");
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));

    if (mqtt->connected())
    {
        if (!checkAliKA())
        {
            return false;
        }

        if (!checkAliPrintSpan())
        {
            respAliTime = millis();
            return false;
        }
        respAliTime = millis();

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

        // if (! iotPub.publish(payload.c_str())) {

        if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str()))
        {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            
            isAliAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            
            isAliAlive = false;
            return true;
        }      
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

void BlinkerMQTT::begin(const char* auth) {
    _authKey = auth;
    // strcpy(_authKey, auth);
    
    BLINKER_LOG_ALL(BLINKER_F("_authKey: "), auth);

    // if (connectServer()) {
    //     mDNSInit();
    //     isMQTTinit = true;
    // }
    // else {
    uint32_t re_time = millis();
    bool isConnect = true;
    while(1)
    {
        re_time = millis();
        // ::delay(10000);
        // BLINKER_ERR_LOG("Maybe you have put in the wrong AuthKey!");
        // BLINKER_ERR_LOG("Or maybe your request is too frequently!");
        // BLINKER_ERR_LOG("Or maybe your network is disconnected!");
        if (connectServer()) {
            mDNSInit();
            isMQTTinit = true;
            return;
        }
        // delay(10000);
        while ((millis() - re_time) < 10000)
        {
            if (WiFi.status() != WL_CONNECTED && isConnect)
            {
                isConnect = false;
                WiFi.begin();
                WiFi.reconnect();
            }
            else if (WiFi.status() == WL_CONNECTED && !isConnect)
            {
                isConnect = true;
            }
            ::delay(10);
            // WiFi.status();
        }
    }
    // }
}

bool BlinkerMQTT::autoPrint(uint32_t id)
{
    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"trigged\":true,\"autoData\":{");
    payload += BLINKER_F("\"autoId\":");
    payload += STRING_format(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(MQTT_ID);
    payload += BLINKER_F("\",\"toDevice\":\"autoManager\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

    if (mqtt->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str()))
            {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                
                linkTime = millis();
                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

bool BlinkerMQTT::autoPrint(char *name, char *type, char *data)
{
    String payload = BLINKER_F("{\"data\":{");
    payload += STRING_format(data);
    payload += BLINKER_F("},\"fromDevice\":\"");
    payload += STRING_format(MQTT_ID);
    payload += BLINKER_F("\",\"toDevice\":\"");
    payload += name;
    payload += BLINKER_F("\",\"deviceType\":\"");
    payload += type;
    payload += BLINKER_F("\"}");
        
    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));
    
    if (mqtt->connected()) {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            linkTime = millis();
            
            BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));
            
            return true;
        }
        else {
            BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

bool BlinkerMQTT::autoPrint(char *name1, char *type1, char *data1
    , char *name2, char *type2, char *data2)
{
    String payload = BLINKER_F("{\"data\":{");
    payload += STRING_format(data1);
    payload += BLINKER_F("},\"fromDevice\":\"");
    payload += STRING_format(MQTT_ID);
    payload += BLINKER_F("\",\"toDevice\":\"");
    payload += name1;
    payload += BLINKER_F("\",\"deviceType\":\"");
    payload += type1;
    payload += BLINKER_F("\"}");
        
    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));
    
    if (mqtt->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            linkTime = millis();

            BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));

            payload = BLINKER_F("{\"data\":{");
            payload += STRING_format(data2);
            payload += BLINKER_F("},\"fromDevice\":\"");
            payload += STRING_format(MQTT_ID);
            payload += BLINKER_F("\",\"toDevice\":\"");
            payload += name2;
            payload += BLINKER_F("\",\"deviceType\":\"");
            payload += type2;
            payload += BLINKER_F("\"}");
                
            BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));
            
            return true;
        }
        else
        {
            BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

String BlinkerMQTT::deviceName() { return DEVICE_NAME;/*MQTT_ID;*/ }