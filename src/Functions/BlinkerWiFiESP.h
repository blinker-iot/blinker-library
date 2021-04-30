#ifndef BLINKER_WIFI_ESP_H
#define BLINKER_WIFI_ESP_H

#if (defined(ESP8266) || defined(ESP32))

#define BLINKER_WIFI

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266WiFiMulti.h>
    #include <ESP8266HTTPClient.h>

    #include <base64.h>

    ESP8266WiFiMulti wifiMulti;
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <WiFiMulti.h>
    #include <HTTPClient.h>

    #include <base64.h>

    WiFiMulti  wifiMulti;
#endif

#include <EEPROM.h>

#include "../modules/WebSockets/WebSocketsServer.h"
#include "../modules/mqtt/Adafruit_MQTT.h"
#include "../modules/mqtt/Adafruit_MQTT_Client.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "Adapters/BlinkerMQTT.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
// #include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"
#include "../Blinker/BlinkerHTTP.h"

enum b_config_t {
    COMM,
    BLINKER_SMART_CONFIG,
    BLINKER_AP_CONFIG,
    BLINKER_MULTI
};

enum b_configStatus_t {
    AUTO_INIT,
    AUTO_DONE,
    SMART_BEGIN,
    SMART_DONE,
    SMART_TIMEOUT,
    APCFG_BEGIN,
    APCFG_DONE,
    APCFG_TIMEOUT
};

enum b_broker_t {
    aliyun_b,
    blinker_b
};

char*       MQTT_HOST_MQTT;
char*       MQTT_ID_MQTT;
char*       MQTT_NAME_MQTT;
char*       MQTT_KEY_MQTT;
char*       MQTT_PRODUCTINFO_MQTT;
char*       UUID_MQTT;
char*       DEVICE_NAME_MQTT;
char*       BLINKER_PUB_TOPIC_MQTT;
char*       BLINKER_SUB_TOPIC_MQTT;
// char*       BLINKER_RRPC_PUB_TOPIC_MQTT;
char*       BLINKER_RRPC_SUB_TOPIC_MQTT;
uint16_t    MQTT_PORT_MQTT;

class BlinkerWiFiESP
{
    public :
        BlinkerWiFiESP();

        void begin(const char* auth);
        bool init();
        void reset();
        b_device_staus_t status()       { return _status;}
        bool connect();
        void disconnect();
        void ping();
        bool available();
        char * lastRead();
        void flush();
        int  print(char * data, bool needCheck = true);
        int  autoPrint(unsigned long id);
        void subscribe();
        int  isJson(const String & data);
        bool deviceRegister();
        void freshSharers();

        char * deviceName()             { return DEVICE_NAME_MQTT; }
        char * authKey()                { return (char *)_authKey; }
        char * token()                  { if (!isMQTTinit) return ""; else return MQTT_KEY_MQTT; }

        String httpServer(uint8_t _type, const String & msg, bool state = false)
        { return httpToServer(_type, msg, state); }
        
        bool checkInit()            { return isMQTTinit; }
        bool checkWlanInit();
        void commonBegin(const char* _ssid, const char* _pswd);
        #if defined(BLINKER_ESP_SMARTCONFIG)
        void smartconfigBegin();
        void smartconfig();
        #elif defined(BLINKER_WIFI_MULTI)
        void multiBegin(const char* _ssid, const char* _pswd);
        #elif defined(BLINKER_APCONFIG)
        void apconfigBegin();
        void softAPinit();
        void checkAPCFG();
        bool parseUrl(String data);
        #endif
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);
    
    #if defined(BLINKER_ALIGENIE)
        int aliAvail();
        int aliPrint(const String & data);
    #endif
    
    #if defined(BLINKER_DUEROS)
        int duerAvail();
        int duerPrint(const String & data, bool report);
    #endif

    #if defined(BLINKER_MIOT)
        int miAvail();
        int miPrint(const String & data);
    #endif

    private :
        void mDNSInit();
        bool autoInit();
        bool checkConfig();

        void checkKA();
        int checkCanPrint();
        int checkPrintSpan();
        int checkPrintLimit();

    #if defined(BLINKER_ALIGENIE)
        int checkAliKA();
        int checkAliPrintSpan();
    #endif

    #if defined(BLINKER_DUEROS)
        int checkDuerKA();
        int checkDuerPrintSpan();
    #endif

    #if defined(BLINKER_MIOT)
        int checkMIOTKA();
        int checkMIOTPrintSpan();
    #endif

        void parseData(const char* data);

    protected :
        b_broker_t  _use_broker = aliyun_b;
        char        _messageId[20];
        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
        bool        _isWiFiInit = false;
        // bool        _isBegin = false;
        bool        isMQTTinit = false;
        uint32_t    latestTime;
        bool*       isHandle;
        bool        isAlive = false;
        bool        _needCheckShare = false;
        uint8_t     _sharerCount = 0;
        uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        uint32_t    kaTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;
        uint8_t     respDuerTimes = 0;
        uint32_t    respDuerTime = 0;
        uint8_t     respMIOTTimes = 0;
        uint32_t    respMIOTTime = 0;

        uint32_t    _print_time = 0;
        uint8_t     _print_times = 0;
        uint32_t    printTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        uint32_t    duerKaTime = 0;
        bool        isDuerAlive = false;
        bool        isDuerAvail = false;
        uint32_t    miKaTime = 0;
        bool        isMIOTAlive = false;
        bool        isMIOTAvail = false;
        uint32_t    linkTime = 0;

        const char* _authKey;

        b_config_t  _configType = COMM;
        b_configStatus_t _configStatus = AUTO_INIT;
        uint32_t    _connectTime = 0;
        uint8_t     _connectTimes = 0;

        uint8_t     reconnect_time = 0;
        uint32_t    _reRegister_time = 0;
        uint8_t     _reRegister_times = 0;

        char        message_id[24];
        bool        is_rrpc = false;        

        uint8_t     data_dataCount = 0;
        uint8_t     data_timeSlotDataCount = 0;
        uint32_t    time_timeSlotData = 0;
        uint8_t     _aCount = 0;
        uint8_t     _bridgeCount = 0;

        b_device_staus_t _status = WLAN_CONNECTING;
};

// #if defined(ESP8266)
//     #ifndef BLINKER_WITHOUT_SSL
//         BearSSL::WiFiClientSecure   client_mqtt;
//     #else
//         WiFiClient               client_mqtt;
//     #endif
// #elif defined(ESP32)
//     WiFiClientSecure     client_s;
// #endif

// WiFiClient               client;
Adafruit_MQTT_Client*    mqtt_MQTT;
Adafruit_MQTT_Subscribe* iotSub_MQTT;

#define WS_SERVERPORT       81
WebSocketsServer webSocket_MQTT = WebSocketsServer(WS_SERVERPORT);

char*    msgBuf_MQTT;
bool     isFresh_MQTT = false;
bool     isConnect_MQTT = false;
bool     isAvail_MQTT = false;
bool     isApCfg = false;
uint8_t  ws_num_MQTT = 0;
uint8_t  dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

void webSocketEvent_MQTT(uint8_t num, WStype_t type, \
                    uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

            if (!isApCfg) isConnect_MQTT = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket_MQTT.remoteIP(num);

                BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                                BLINKER_F(", Connected from: "), ip,
                                BLINKER_F(", url: "), (char *)payload);

                // send message to client
                webSocket_MQTT.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num_MQTT = num;

                if (!isApCfg) isConnect_MQTT = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                            BLINKER_F(", get Text: "), (char *)payload, \
                            BLINKER_F(", length: "), length);

            if (length < BLINKER_MAX_READ_SIZE) {
                if (isFresh_MQTT) free(msgBuf_MQTT);
                msgBuf_MQTT = (char*)malloc((length+1)*sizeof(char));
                strcpy(msgBuf_MQTT, (char*)payload);
                isAvail_MQTT = true;
                isFresh_MQTT = true;

                BLINKER_LOG_ALL(BLINKER_F("isAvail_MQTT: "), isAvail_MQTT);
            }

            if (!isApCfg) dataFrom_MQTT = BLINKER_MSG_FROM_WS;

            ws_num_MQTT = num;

            // send message to client
            // webSocket_MQTT.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket_MQTT.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket_MQTT.sendBIN(num, payload, length);
            break;
        default :
            break;
    }
}

BlinkerWiFiESP  WiFiESP;

BlinkerWiFiESP::BlinkerWiFiESP() { isHandle = &isConnect_MQTT; }

void BlinkerWiFiESP::begin(const char* auth)
{
    // _authKey = (char*)malloc((strlen(auth)+1)*sizeof(char));
    // strcpy(_authKey, auth);
    _authKey = auth;

    BLINKER_LOG_ALL(BLINKER_F("_authKey: "), _authKey);
}

bool BlinkerWiFiESP::init()
{
    if (!checkWlanInit()) return false;

    if (isMQTTinit) return true;

    if (_connectTimes < BLINKER_SERVER_CONNECT_LIMIT)
    {
        if (_connectTime == 0 || (millis() - _connectTime) >= 10000)
        {
            _connectTime = millis();
            if (deviceRegister()) {
                _connectTimes = 0;
                mDNSInit();
                isMQTTinit = true;
                // _isBegin = true;
                return true;
            }
            else {
                _connectTimes++;
                isMQTTinit = false;
                return false;
            }
        }
    }
    else
    {
        if (millis() - _connectTime > 60000 * 1) _connectTimes = 0;
    }
    
    return false;
}

void BlinkerWiFiESP::reset()
{
    BLINKER_LOG(BLINKER_F("Blinker reset..."));
    char config_check[3] = {0};
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, config_check);
    EEPROM.commit();
    EEPROM.end();

    ESP.restart();
}

bool BlinkerWiFiESP::connect()
{
    if (!checkInit()) return false;

    int8_t ret;

    webSocket_MQTT.loop();

    if (mqtt_MQTT->connected())
    {
        _status = DEV_CONNECTED;
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    {
        yield();
        return false;
    }

    _status = DEV_CONNECTING;

    BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));

    BLINKER_LOG(BLINKER_F("reconnect_time: "), reconnect_time);

    #if defined(ESP8266)
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.setInsecure();
            ::delay(10);
        #endif
    #endif

    if ((ret = mqtt_MQTT->connect()) != 0)
    {
        BLINKER_LOG(mqtt_MQTT->connectErrorString(ret));
        BLINKER_LOG(BLINKER_F("Retrying MQTT connection in "), \
                    BLINKER_MQTT_CONNECT_TIMESLOT/1000, \
                    BLINKER_F(" seconds..."));

        if (ret == 4) 
        {
            if (_reRegister_times < BLINKER_SERVER_CONNECT_LIMIT)
            {
                if (deviceRegister())
                {
                    _reRegister_times = 0;
                }
                else
                {
                    _reRegister_times++;
                    _reRegister_time = millis();
                    latestTime = millis();
                }
            }
            else
            {
                if (millis() - _reRegister_time >= 60000 * 1) _reRegister_times = 0;
            }

            BLINKER_LOG_ALL(BLINKER_F("_reRegister_times1: "), _reRegister_times);
            return false;
        }
        else
        {
            latestTime = millis();
            reconnect_time += 1;
            if (reconnect_time >= 4)
            {
                if (_reRegister_times < BLINKER_SERVER_CONNECT_LIMIT)
                {
                    if (deviceRegister())
                    {
                        _reRegister_times = 0;
                    }
                    else
                    {
                        _reRegister_times++;
                        _reRegister_time = millis();
                    }
                }
                else
                {
                    if (millis() - _reRegister_time >= 60000 * 1) _reRegister_times = 0;
                }
                
                reconnect_time = 0;

                BLINKER_LOG_ALL(BLINKER_F("_reRegister_times2: "), _reRegister_times);
            }
            return false;
        }
    }
    
    reconnect_time = 0;

    BLINKER_LOG(BLINKER_F("MQTT Connected!"));
    BLINKER_LOG_FreeHeap();

    if (!isMQTTinit)
    {
        isMQTTinit = true;

        mqtt_MQTT->subscribeTopic(BLINKER_SUB_TOPIC_MQTT);
        BLINKER_LOG_ALL(BLINKER_F("MQTT subscribe: "), BLINKER_SUB_TOPIC_MQTT);
    }

    this->latestTime = millis();

    return true;
}

void BlinkerWiFiESP::disconnect()
{
    if (!checkInit()) return;

    if (mqtt_MQTT->connected()) mqtt_MQTT->disconnectServer();

    if (*isHandle) webSocket_MQTT.disconnect();

    _status = DEV_DISCONNECTED;
}

void BlinkerWiFiESP::ping()
{  
    if (!checkInit()) return;

    BLINKER_LOG(BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!mqtt_MQTT->ping())
    {
        disconnect();
    }
    else
    {
        this->latestTime = millis();
    }
}

bool BlinkerWiFiESP::available()
{
    if (!checkInit()) return false;

    webSocket_MQTT.loop();

    checkKA();
#if defined(ESP8266)
    MDNS.update();
#endif
    
    if ((millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
    {
        ping();
    }
    else
    {
        subscribe();
    }

    if (isAvail_MQTT)
    {
        BLINKER_LOG_ALL(BLINKER_F("available: "), isAvail_MQTT);

        isAvail_MQTT = false;
        return true;
    }
    else {
        return false;
    }
}

char * BlinkerWiFiESP::lastRead()
{
    if (isFresh_MQTT) return msgBuf_MQTT;
    else return "";
}

void BlinkerWiFiESP::flush()
{
    if (isFresh_MQTT)
    {        
        isFresh_MQTT = false; isAvail_MQTT = false;
        isAliAvail = false; isDuerAvail = false; isMIOTAvail = false;//isBavail = false;
        
        BLINKER_LOG_ALL(BLINKER_F("flush"));
        BLINKER_LOG_FreeHeap_ALL();
    }
}

void BlinkerWiFiESP::subscribe()
{
    if (!checkInit()) return;

    if (!isMQTTinit) return;

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt_MQTT->readSubscription(10)))
    {
        if (subscription == iotSub_MQTT)
        {
            BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub_MQTT->lastread);

            parseData((char *)iotSub_MQTT->lastread);
        }
    }

    if (mqtt_MQTT->check_extra())
    {
        BLINKER_LOG_ALL(BLINKER_F("Got extra topic: "), (char *)mqtt_MQTT->get_extra_topic());
        BLINKER_LOG_ALL(BLINKER_F("Got extra data: "), (char *)mqtt_MQTT->get_extra_data());

        if (strncmp(BLINKER_RRPC_SUB_TOPIC_MQTT, 
                    (char *)mqtt_MQTT->get_extra_topic(), 
                    strlen(BLINKER_RRPC_SUB_TOPIC_MQTT))
                    == 0)
        {
            // char message_id[24];
            memset(message_id, '\0', 24);

            memmove(message_id, 
                mqtt_MQTT->get_extra_topic()+strlen(BLINKER_RRPC_SUB_TOPIC_MQTT),
                strlen(mqtt_MQTT->get_extra_topic()) - strlen(BLINKER_RRPC_SUB_TOPIC_MQTT));
            
            BLINKER_LOG_ALL(BLINKER_F("from server RRPC, message_id: "), message_id);

            parseData((char *)mqtt_MQTT->get_extra_data());

            is_rrpc = true;
        }
    }
}

int BlinkerWiFiESP::print(char * data, bool needCheck)
{
    if (!checkInit()) return false;
    
    if (*isHandle && dataFrom_MQTT == BLINKER_MSG_FROM_WS)
    {
        if (needCheck)
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
        BLINKER_LOG_ALL(BLINKER_F("Success..."));

        strcat(data, BLINKER_CMD_NEWLINE);

        webSocket_MQTT.sendTXT(ws_num_MQTT, data);

        return true;
    }
    else
    {
        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';        

        char data_add[20] = "{\"data\":";
        for(uint8_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }
        
        strcat(data, ",\"fromDevice\":\"");
        strcat(data, DEVICE_NAME_MQTT);
        strcat(data, "\",\"toDevice\":\"");
        
        if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        {
            strcat(data, _sharers[_sharerFrom]->uuid());
        }
        else
        {
            strcat(data, UUID_MQTT);
        }

        strcat(data, "\",\"deviceType\":\"OwnApp\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        if (!isJson(STRING_format(data))) return false;

        BLINKER_LOG_ALL(BLINKER_F("MQTT Publish..."));
        BLINKER_LOG_FreeHeap_ALL();

        bool _alive = isAlive;


        if (needCheck)
        {
            if (!checkPrintSpan())
            {
                return false;
            }
            respTime = millis();
        }

        if (mqtt_MQTT->connected())
        {
            if (needCheck)
            {
                if (!checkCanPrint())
                {
                    if (!_alive)
                    {
                        isAlive = false;
                    }
                    return false;
                }

                if (!checkPrintLimit())
                {
                    return false;
                }

                _print_times++;

                BLINKER_LOG_ALL(BLINKER_F("_print_times: "), _print_times);
            }

            if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, data))
            {
                BLINKER_LOG_ALL(data);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                BLINKER_LOG_FreeHeap_ALL();

                if (!_alive)
                {
                    isAlive = false;
                }
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(data);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                BLINKER_LOG_FreeHeap_ALL();

                if (needCheck) printTime = millis();

                if (!_alive)
                {
                    isAlive = false;
                }

                this->latestTime = millis();

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

int BlinkerWiFiESP::autoPrint(unsigned long id)
{
    BLINKER_LOG_ALL(BLINKER_F("autoTrigged id: "), id);

    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"auto\":{\"trig\":true,");
    payload += BLINKER_F("\"id\":");
    payload += String(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(DEVICE_NAME_MQTT);
    payload += BLINKER_F("\",\"deviceType\":\"Auto\"");
    payload += BLINKER_F(",\"toDevice\":\"serverClient\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

    if (mqtt_MQTT->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, payload.c_str()))
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

                this->latestTime = millis();

                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);

            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiESP::isJson(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("isJson: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(STRING_format(data));
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, STRING_format(data));
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG("Print data is not Json! ", data);
        return false;
    }

    return true;
}

bool BlinkerWiFiESP::deviceRegister()
{
    _status = DEV_REGISTER;

    String urls = _authKey;
    #if defined(BLINKER_ALIGENIE_LIGHT)
        urls += BLINKER_F("&aliType=light");
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        urls += BLINKER_F("&aliType=outlet");
    #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
        urls += BLINKER_F("&aliType=multi_outlet");
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        urls += BLINKER_F("&aliType=sensor");
    #elif defined(BLINKER_ALIGENIE_FAN)
        urls += BLINKER_F("&aliType=fan");
    #elif defined(BLINKER_ALIGENIE_AIRCONDITION)
        urls += BLINKER_F("&aliType=aircondition");
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        urls += BLINKER_F("&duerType=LIGHT");
    #elif defined(BLINKER_DUEROS_OUTLET)
        urls += BLINKER_F("&duerType=SOCKET");
    #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
        urls += BLINKER_F("&duerType=MULTI_SOCKET");
    #elif defined(BLINKER_DUEROS_SENSOR)
        urls += BLINKER_F("&duerType=AIR_MONITOR");
    #elif defined(BLINKER_DUEROS_FAN)
        urls += BLINKER_F("&duerType=FAN");
    #elif defined(BLINKER_DUEROS_AIRCONDITION)
        urls += BLINKER_F("&duerType=AIR_CONDITION");
    #endif

    #if defined(BLINKER_MIOT_LIGHT)
        urls += BLINKER_F("&miType=light");
    #elif defined(BLINKER_MIOT_OUTLET)
        urls += BLINKER_F("&miType=outlet");
    #elif defined(BLINKER_MIOT_MULTI_OUTLET)
        urls += BLINKER_F("&miType=multi_outlet");
    #elif defined(BLINKER_MIOT_SENSOR)
        urls += BLINKER_F("&miType=sensor");
    #elif defined(BLINKER_MIOT_FAN)
        urls += BLINKER_F("&miType=fan");
    #elif defined(BLINKER_MIOT_AIRCONDITION)
        urls += BLINKER_F("&miType=aircondition");
    #endif

    urls += BLINKER_F("&version=");
    urls += BLINKER_OTA_VERSION_CODE;
    #ifndef BLINKER_WITHOUT_SSL
    urls += BLINKER_F("&protocol=mqtts");
    #else
    urls += BLINKER_F("&protocol=mqtt");
    #endif

    String payload = httpToServer(BLINKER_CMD_WIFI_AUTH_NUMBER, urls);

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));

    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
            BLINKER_ERR_LOG(BLINKER_F("Maybe you have put in the wrong AuthKey!"));
            BLINKER_ERR_LOG(BLINKER_F("Or maybe your request is too frequently!"));
            BLINKER_ERR_LOG(BLINKER_F("Or maybe your network is disconnected!"));

            return false;
    }

    String _userID = root[BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_UUID];
    String _host = root["host"];
    uint32_t _port = root["port"];
    uint8_t _num = _host.indexOf("://");
    BLINKER_LOG_ALL("_num: ", _num);
    if (_num > 0) _num += 3;
    _host = _host.substring(_num, _host.length());

    if (mqtt_MQTT)
    {
        free(mqtt_MQTT);
        free(iotSub_MQTT);
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
        #if defined(BLINKER_WITHOUT_SSL)
            if(!isMQTTinit) DEVICE_NAME_MQTT = (char*)malloc((24+1)*sizeof(char));
            strcpy(DEVICE_NAME_MQTT, _userName.substring(0, 24).c_str());
        #else
            if(!isMQTTinit) DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
            strcpy(DEVICE_NAME_MQTT, _userID.c_str());
        #endif
        if(!isMQTTinit) MQTT_ID_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT, _userID.c_str());
        if(!isMQTTinit) MQTT_NAME_MQTT = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT, _userName.c_str());
        if(!isMQTTinit) MQTT_KEY_MQTT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT, _key.c_str());
        if(!isMQTTinit) MQTT_PRODUCTINFO_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo.c_str());
        if(!isMQTTinit) MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT_MQTT = BLINKER_MQTT_ALIYUN_PORT;

        _use_broker = aliyun_b;
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        // String id2name = _userID.subString(10, _userID.length());
        // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
        if(!isMQTTinit) DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT, _userID.c_str());
        String IDtest = _productInfo + _userID;
        if(!isMQTTinit) MQTT_ID_MQTT = (char*)malloc((IDtest.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT, IDtest.c_str());
        String NAMEtest = IDtest + ";" + _userName;
        if(!isMQTTinit) MQTT_NAME_MQTT = (char*)malloc((NAMEtest.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT, NAMEtest.c_str());
        if(!isMQTTinit) MQTT_KEY_MQTT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT, _key.c_str());
        if(!isMQTTinit) MQTT_PRODUCTINFO_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo.c_str());
        if(!isMQTTinit) MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_QCLOUD_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT_MQTT = BLINKER_MQTT_QCLOUD_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
        if(!isMQTTinit) DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT, _userID.c_str());
        if(!isMQTTinit) MQTT_ID_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT, _userID.c_str());
        if(!isMQTTinit) MQTT_NAME_MQTT = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT, _userName.c_str());
        if(!isMQTTinit) MQTT_KEY_MQTT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT, _key.c_str());
        if(!isMQTTinit) MQTT_PRODUCTINFO_MQTT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo.c_str());
        if(!isMQTTinit) MQTT_HOST_MQTT = (char*)malloc((_host.length()+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT, _host.c_str());
        MQTT_PORT_MQTT = _port;

        _use_broker = blinker_b;
    }
    if(!isMQTTinit) UUID_MQTT = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_MQTT, _uuid.c_str());

    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME_MQTT: "), DEVICE_NAME_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_MQTT: "), MQTT_PRODUCTINFO_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_MQTT: "), MQTT_ID_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_MQTT: "), MQTT_NAME_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_MQTT: "), MQTT_KEY_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), _host);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), _port);
    BLINKER_LOG_ALL(BLINKER_F("UUID_MQTT: "), UUID_MQTT);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        String PUB_TOPIC_STR = BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += DEVICE_NAME_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/s");

        if(!isMQTTinit) BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

        String SUB_TOPIC_STR = BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += DEVICE_NAME_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/r");

        if(!isMQTTinit) BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);

        
        SUB_TOPIC_STR = BLINKER_F("/sys/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += DEVICE_NAME_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/rrpc/request/");

        if(!isMQTTinit) BLINKER_RRPC_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_RRPC_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        String PUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += _userID;
        PUB_TOPIC_STR += BLINKER_F("/s");

        if(!isMQTTinit) BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

        String SUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += _userID;
        SUB_TOPIC_STR += BLINKER_F("/r");

        if(!isMQTTinit) BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);
    }
    else if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        String PUB_TOPIC_STR = BLINKER_F("/device");
        // PUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_ID_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/s");

        if(!isMQTTinit) BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

        String SUB_TOPIC_STR = BLINKER_F("/device");
        // SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_ID_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/r");

        if(!isMQTTinit) BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        #if defined(ESP8266)
            mqtt_MQTT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
        #elif defined(ESP32)
            mqtt_MQTT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
        #endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        #if defined(ESP8266)
            mqtt_MQTT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
        #elif defined(ESP32)
            mqtt_MQTT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
        #endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        #if defined(ESP8266)
            mqtt_MQTT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
        #elif defined(ESP32)
            mqtt_MQTT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT, MQTT_PORT_MQTT, MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);
        #endif
    }

    iotSub_MQTT = new Adafruit_MQTT_Subscribe(mqtt_MQTT, BLINKER_SUB_TOPIC_MQTT);

    this->latestTime = millis() - BLINKER_MQTT_CONNECT_TIMESLOT;
    
    mqtt_MQTT->subscribe(iotSub_MQTT);

    #if defined(ESP8266)
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.setInsecure();
        #endif
    #elif defined(ESP32)
        client_s.setInsecure();
    #endif

    freshSharers();

    BLINKER_LOG_FreeHeap();

    return true;
}

void BlinkerWiFiESP::freshSharers()
{
    String  data = BLINKER_F("/share/device?");
            data += BLINKER_F("deviceName=");
            data += STRING_format(DEVICE_NAME_MQTT);
            data += BLINKER_F("&key=");
            data += STRING_format(_authKey);

    String payload = httpToServer(BLINKER_CMD_FRESH_SHARERS_NUMBER, data);

    BLINKER_LOG_ALL(BLINKER_F("sharers data: "), payload);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success()) return;
    if (error) return;

    String user_name = "";

    if (_sharerCount)
    {
        for (uint8_t num = _sharerCount; num > 0; num--)
        {
            delete _sharers[num - 1];
        }
    }

    _sharerCount = 0;

    for (uint8_t num = 0; num < BLINKER_MQTT_MAX_SHARERS_NUM; num++)
    {
        user_name = root["users"][num].as<String>();

        if (user_name.length() >= BLINKER_MQTT_USER_UUID_SIZE)
        {
            BLINKER_LOG_ALL(BLINKER_F("sharer uuid: "), user_name, BLINKER_F(", length: "), user_name.length());

            _sharerCount++;

            _sharers[num] = new BlinkerSharer(user_name);
        }
        else
        {
            break;
        }
    }
}

void BlinkerWiFiESP::mDNSInit()
{
#if defined(ESP8266)
    if (!MDNS.begin(DEVICE_NAME_MQTT, WiFi.localIP())) {
#elif defined(ESP32)
    if (!MDNS.begin(DEVICE_NAME_MQTT)) {
#endif
        while(1) {
            ::delay(100);
        }
    }

    BLINKER_LOG(BLINKER_F("mDNS responder started"));

    MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME_MQTT));
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "mac", macDeviceName());

    webSocket_MQTT.begin();
    webSocket_MQTT.onEvent(webSocketEvent_MQTT);
    BLINKER_LOG(BLINKER_F("webSocket_MQTT server started"));
    BLINKER_LOG(BLINKER_F("ws://"), DEVICE_NAME_MQTT, BLINKER_F(".local:"), WS_SERVERPORT);

    isApCfg = false;
}

bool BlinkerWiFiESP::checkWlanInit()
{
    char ok[2 + 1] = "OK";

    if (!_isWiFiInit)
    {
        switch (_configType)
        {
            case COMM :
                _connectTime = millis();
                
                if (WiFi.status() != WL_CONNECTED) {
                    ::delay(500);

                    if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                        // _connectTime = millis();
                        BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
                        BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
                        return false;
                    }

                    return false;
                }
                BLINKER_LOG(BLINKER_F("WiFi Connected."));
                BLINKER_LOG(BLINKER_F("IP Address: "));
                BLINKER_LOG(WiFi.localIP());
                _isWiFiInit = true;
                _connectTime = 0;

                // begin();

                _status = WLAN_CONNECTED;

                return false;
            #if defined(BLINKER_ESP_SMARTCONFIG)
            case BLINKER_SMART_CONFIG :
                switch (_configStatus)
                {
                    case AUTO_INIT :
                        if (WiFi.status() != WL_CONNECTED) {
                            ::delay(500);
                            return false;
                        }
                        else {
                            BLINKER_LOG(BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(BLINKER_F("IP Address: "));
                            BLINKER_LOG(WiFi.localIP());

                            _isWiFiInit = true;
                            _connectTime = 0;
                            // _isWiFiInit = true;

                            // begin();
                            _configStatus = AUTO_DONE;

                            _status = WLAN_CONNECTED;

                            return false;
                        }
                    case SMART_BEGIN :
                        if (WiFi.smartConfigDone())
                        {
                            BLINKER_LOG(BLINKER_F("SmartConfig received."));
                            _connectTime = millis();

                            #if defined(ESP8266)
                                BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
                            #endif

                            _configStatus = SMART_DONE;

                            _status = WLAN_CONNECTING;
                        }
                        else return false;
                    case SMART_DONE :
                        if (WiFi.status() != WL_CONNECTED)
                        {
                            if (millis() - _connectTime > 15000)
                            {
                                BLINKER_LOG(BLINKER_F("SmartConfig timeout."));
                                WiFi.stopSmartConfig();
                                _configStatus = SMART_TIMEOUT;
                            }
                            return false;
                        }
                        else if (WiFi.status() == WL_CONNECTED)
                        {
                            // WiFi.stopSmartConfig();

                            BLINKER_LOG(BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(BLINKER_F("IP Address: "));
                            BLINKER_LOG(WiFi.localIP());
                            _isWiFiInit = true;
                            _connectTime = 0;                            

                            EEPROM.begin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
                            EEPROM.commit();
                            EEPROM.end();

                            BLINKER_LOG(BLINKER_F("Save wlan config"));

                            // begin();

                            _status = WLAN_CONNECTED;
                            
                            return false;
                        }
                        // return false;
                    case SMART_TIMEOUT :
                        WiFi.beginSmartConfig();
                        _configStatus = SMART_BEGIN;

                        _status = WLAN_SMARTCONFIG_BEGIN;

                        BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
                        return false;
                    default :
                        yield();
                        return false;
                }
            #elif defined(BLINKER_APCONFIG)
            case BLINKER_AP_CONFIG :
                switch (_configStatus)
                {
                    case AUTO_INIT :
                        if (WiFi.status() != WL_CONNECTED) {
                            ::delay(500);
                            return false;
                        }
                        else {
                            BLINKER_LOG(BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(BLINKER_F("IP Address: "));
                            BLINKER_LOG(WiFi.localIP());

                            _isWiFiInit = true;
                            _connectTime = 0;
                            // _isWiFiInit = true;

                            // begin();
                            _configStatus = AUTO_DONE;

                            _status = WLAN_CONNECTED;

                            return false;
                        }
                    case APCFG_BEGIN :
                        checkAPCFG();
                        return false;
                    case APCFG_DONE :
                        if (WiFi.status() != WL_CONNECTED)
                        {
                            if (millis() - _connectTime > 15000)
                            {
                                BLINKER_LOG(BLINKER_F("APConfig timeout."));
                                _configStatus = APCFG_TIMEOUT;
                            }
                            return false;
                        }
                        else if (WiFi.status() == WL_CONNECTED)
                        {
                            BLINKER_LOG(BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(BLINKER_F("IP Address: "));
                            BLINKER_LOG(WiFi.localIP());
                            _isWiFiInit = true;
                            _connectTime = 0;

                            // begin();

                            EEPROM.begin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
                            EEPROM.commit();
                            EEPROM.end();

                            _status = WLAN_CONNECTED;
                            
                            return false;
                        }
                        // return false;
                    case APCFG_TIMEOUT :
                        softAPinit();
                        return false;
                    default :
                        yield();
                        return false;
                }
            #elif defined(BLINKER_WIFI_MULTI)
            case BLINKER_MULTI:
                _connectTime = millis();
                if (wifiMulti.run() != WL_CONNECTED) {
                    ::delay(500);

                    if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                        // _connectTime = millis();
                        BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
                        BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
                        return false;
                    }

                    return false;
                }
                BLINKER_LOG(BLINKER_F("WiFi Connected."));
                BLINKER_LOG(BLINKER_F("IP Address: "));
                BLINKER_LOG(WiFi.localIP());
                _isWiFiInit = true;
                _connectTime = 0;

                // begin();

                return false;
            #endif
            default :
                return false;
        }
    }

    // if (!_isBegin)
    // {
    //     _isBegin = init();
    //     return _isBegin;
    // }
    return true;
}

void BlinkerWiFiESP::commonBegin(const char* _ssid, const char* _pswd)
{
    _configType = COMM;

    connectWiFi(_ssid, _pswd);

    #if defined(ESP8266)
        BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
    #endif
}

#if defined(BLINKER_ESP_SMARTCONFIG)

void BlinkerWiFiESP::smartconfigBegin()
{
    _configType = BLINKER_SMART_CONFIG;

    if (!autoInit()) smartconfig();
    // else _configStatus = SMART_DONE;

    #if defined(ESP8266)
        BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
    #endif
}

void BlinkerWiFiESP::smartconfig()
{
    WiFi.mode(WIFI_STA);

    String _hostname = BLINKER_F("DiyArduino_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    WiFi.beginSmartConfig();

    _configStatus = SMART_BEGIN;

    _status = WLAN_SMARTCONFIG_BEGIN;

    BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
}

#elif defined(BLINKER_WIFI_MULTI)

void BlinkerWiFiESP::multiBegin(const char* _ssid, const char* _pswd)
{
    _configType = BLINKER_MULTI;

    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduinoMQTT_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    wifiMulti.addAP(_ssid, _pswd);

    BLINKER_LOG(BLINKER_F("wifiMulti add "), _ssid);

    #if defined(ESP8266)
        BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
    #endif
}

#elif defined(BLINKER_APCONFIG)

void BlinkerWiFiESP::apconfigBegin()
{
    #if defined(BLINKER_APCONFIG)
    _configType = BLINKER_AP_CONFIG;
    
    if (!autoInit()) softAPinit();
    // else _configStatus = APCFG_DONE;

    #if defined(ESP8266)
        BLINKER_LOG(BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(BLINKER_F("ESP32_MQTT initialized..."));
    #endif
    #endif
}

void BlinkerWiFiESP::softAPinit()
{
    IPAddress apIP(192, 168, 4, 1);
    #if defined(ESP8266)
        IPAddress netMsk(255, 255, 255, 0);
    #endif

    WiFi.mode(WIFI_AP);    

    delay(1000);

    String softAP_ssid = BLINKER_F("DiyArduino_");
    softAP_ssid += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(softAP_ssid.c_str());
        WiFi.softAPConfig(apIP, apIP, netMsk);
    #elif defined(ESP32)
        WiFi.setHostname(softAP_ssid.c_str());
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    #endif

    WiFi.softAP(softAP_ssid.c_str(), NULL);
    delay(100);

    webSocket_MQTT.begin();
    webSocket_MQTT.onEvent(webSocketEvent_MQTT);

    _configStatus = APCFG_BEGIN;
    isApCfg = true;

    _status = WLAN_APCONFIG_BEGIN;

    BLINKER_LOG(BLINKER_F("Wait for APConfig"));
}

void BlinkerWiFiESP::checkAPCFG()
{
    webSocket_MQTT.loop();

    #if defined(ESP8266)
        MDNS.update();
    #endif

    if (isAvail_MQTT)
    {
        BLINKER_LOG(BLINKER_F("checkAPCFG: "), msgBuf_MQTT);

        if (STRING_contains_string(msgBuf_MQTT, "ssid") && \
            STRING_contains_string(msgBuf_MQTT, "pswd"))
        {
            parseUrl(msgBuf_MQTT);
        }
        isAvail_MQTT = false;
    }
}

bool BlinkerWiFiESP::parseUrl(String data)
{
    BLINKER_LOG(BLINKER_F("APCONFIG data: "), data);
    
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject wifi_data = jsonBuffer.as<JsonObject>();
    
    if (error)
    {
        return false;
    }

    String _ssid = wifi_data["ssid"];
    String _pswd = wifi_data["pswd"];

    BLINKER_LOG(BLINKER_F("ssid: "), _ssid);
    BLINKER_LOG(BLINKER_F("pswd: "), _pswd);

    // free(_apServer);
    // MDNS.end();
    webSocket_MQTT.close();

    connectWiFi(_ssid, _pswd);

    _configStatus = APCFG_DONE;
    _connectTime = millis();
    return true;
}

#endif

void BlinkerWiFiESP::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerWiFiESP::connectWiFi(const char* _ssid, const char* _pswd)
{
    uint32_t _connectTime = millis();

    BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduinoMQTT_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    if (_pswd && strlen(_pswd)) {
        WiFi.begin(_ssid, _pswd);
    }
    else {
        WiFi.begin(_ssid);
    }

    _status = WLAN_CONNECTING;
}

bool BlinkerWiFiESP::autoInit()
{
    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduino_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    if (checkConfig())
    {

        WiFi.begin();
        ::delay(500);

        BLINKER_LOG(BLINKER_F("Connecting to WiFi"));

        _status = WLAN_CONNECTING;
        return true;
    }

    return false;
}

bool BlinkerWiFiESP::checkConfig() {
    BLINKER_LOG_ALL(BLINKER_F("check wlan config"));
    
    char ok[2 + 1];
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    if (String(ok) != String("OK")) {
        
        BLINKER_LOG(BLINKER_F("wlan config check,fail"));
        return false;
    }
    else {

        BLINKER_LOG(BLINKER_F("wlan config check,success"));
        return true;
    }
}

void BlinkerWiFiESP::parseData(const char* data)
{
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, String(data));
    JsonObject root = jsonBuffer.as<JsonObject>();

    String _uuid = root["fromDevice"].as<String>();
    String dataGet = root["data"].as<String>();

    if (_use_broker == blinker_b)
    {
        if (_uuid == "ServerSender")
        {
            _uuid = root["data"]["from"].as<String>();
            String _mId = root["data"]["messageId"].as<String>();
            strcpy(_messageId, _mId.c_str());
            BLINKER_LOG_ALL(BLINKER_F("_messageId: "), _mId);
        }
    }

    BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
    BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);

    if (strcmp(_uuid.c_str(), UUID_MQTT) == 0)
    {
        BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));

        kaTime = millis();
        isAvail_MQTT = true;
        isAlive = true;

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    }
    else if (_uuid == BLINKER_CMD_ALIGENIE)
    {
        BLINKER_LOG_ALL(BLINKER_F("form AliGenie"));

        aliKaTime = millis();
        isAliAlive = true;
        isAliAvail = true;
    }
    else if (_uuid == BLINKER_CMD_DUEROS)
    {
        BLINKER_LOG_ALL(BLINKER_F("form DuerOS"));

        duerKaTime = millis();
        isDuerAlive = true;
        isDuerAvail = true;
    }
    else if (_uuid == BLINKER_CMD_MIOT)
    {
        BLINKER_LOG_ALL(BLINKER_F("form MIOT"));

        miKaTime = millis();
        isMIOTAlive = true;
        isMIOTAvail = true;
    }
    else if (_uuid == BLINKER_CMD_SERVERCLIENT || _uuid == "senderClient1")
    {
        BLINKER_LOG_ALL(BLINKER_F("form Sever"));

        isAvail_MQTT = true;
        isAlive = true;

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("_sharerCount: "), _sharerCount);
        if (_sharerCount)
        {
            for (uint8_t num = 0; num < _sharerCount; num++)
            {
                if (strcmp(_uuid.c_str(), _sharers[num]->uuid()) == 0)
                {
                    _sharerFrom = num;

                    kaTime = millis();

                    BLINKER_LOG_ALL(BLINKER_F("From sharer: "), _uuid);
                    BLINKER_LOG_ALL(BLINKER_F("sharer num: "), num);
                    
                    _needCheckShare = false;

                    dataGet = root["data"].as<String>();

                    break;
                }
                else
                {
                    BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, check is from bridge/share device, data: "), dataGet);

                    _needCheckShare = true;

                    dataGet = data;
                }
            }
        }
        else
        {
            BLINKER_ERR_LOG_ALL(BLINKER_F("No authority&share uuid found, check is from bridge/share device, data: "), dataGet);

            _needCheckShare = true;

            dataGet = data;
        }

        isAvail_MQTT = true;
        isAlive = true;
    }

    if (isFresh_MQTT) free(msgBuf_MQTT);
    msgBuf_MQTT = (char*)malloc((dataGet.length()+1)*sizeof(char));
    strcpy(msgBuf_MQTT, dataGet.c_str());
    isFresh_MQTT = true;

    this->latestTime = millis();

    dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

    if (_needCheckShare && isMQTTinit) freshSharers();
}

void BlinkerWiFiESP::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerWiFiESP::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

        checkKA();

        return false;
    }
}

int BlinkerWiFiESP::checkPrintSpan() {
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
            BLINKER_ERR_LOG(BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));

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

int BlinkerWiFiESP::checkPrintLimit()
{
    if ((millis() - _print_time) < 60000)
    {
        if (_print_times < 10) return true;
        else 
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT MSG LIMIT"));
            return false;
        }
    }
    else
    {
        _print_time = millis();
        _print_times = 0;
        return true;
    }
}

#if defined(BLINKER_ALIGENIE)
    int BlinkerWiFiESP::aliAvail()
    {
        if (!checkInit()) return false;

        if (isAliAvail)
        {
            isAliAvail = false;
            return true;
        }
        else {
            return false;
        }
    }

    int BlinkerWiFiESP::aliPrint(const String & data)
    {
        if (!checkInit()) return false;

        String data_add = BLINKER_F("{\"data\":");
    
        if (_use_broker == aliyun_b)
        {
            data_add += data;
        }
        else if (_use_broker == blinker_b)
        {
            data_add += data.substring(0, data.length() - 1);
            data_add += BLINKER_F(",\"messageId\":\"");
            data_add += STRING_format(_messageId);
            data_add += BLINKER_F("\"}");
        }

        data_add += BLINKER_F(",\"fromDevice\":\"");
        data_add += DEVICE_NAME_MQTT;

        if (_use_broker == aliyun_b)
        {
            data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
        }
        else if (_use_broker == blinker_b)
        {
            data_add += BLINKER_F("\",\"toDevice\":\"ServerReceiver\"");
        }

        data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

        if (!isJson(data_add)) return false;

        BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));
        BLINKER_LOG_FreeHeap_ALL();

        if (mqtt_MQTT->connected())
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

            
            char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

            if (is_rrpc)
            {
                
                strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, DEVICE_NAME_MQTT);
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

                BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
            }
            else
            {
                strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_MQTT);
            }

            is_rrpc = false;

            char send_data[BLINKER_MAX_SEND_SIZE];

            if (_use_broker == aliyun_b)
            {
                strcpy(send_data, base64::encode(data_add).c_str());
            }
            else if (_use_broker == blinker_b)
            {
                strcpy(send_data, data_add.c_str());
            }

            if (! mqtt_MQTT->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, send_data))
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                BLINKER_LOG_FreeHeap_ALL();

                isAliAlive = false;
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                BLINKER_LOG_FreeHeap_ALL();

                isAliAlive = false;

                this->latestTime = millis();

                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
            return false;
        }
    }    

    int BlinkerWiFiESP::checkAliKA()
    {
        if (millis() - aliKaTime >= 10000)
            return false;
        else
            return true;
    }

    int BlinkerWiFiESP::checkAliPrintSpan()
    {
        if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2)
        {
            if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2)
            {
                BLINKER_ERR_LOG(BLINKER_F("ALIGENIE NOT ALIVE OR MSG LIMIT"));

                return false;
            }
            else
            {
                respAliTimes++;
                return true;
            }
        }
        else
        {
            respAliTimes = 0;
            return true;
        }
    }
#endif

#if defined(BLINKER_DUEROS)
    int BlinkerWiFiESP::duerAvail()
    {
        if (!checkInit()) return false;

        if (isDuerAvail)
        {
            isDuerAvail = false;
            return true;
        }
        else {
            return false;
        }
    }

    int BlinkerWiFiESP::duerPrint(const String & data, bool report)
    {
        if (!checkInit()) return false;

        String data_add = BLINKER_F("{\"data\":");

        if (report)
        {
            data_add += BLINKER_F("{\"report\":");
    
            if (_use_broker == aliyun_b)
            {
                data_add += data;
            }
            else if (_use_broker == blinker_b)
            {
                data_add += data.substring(0, data.length() - 1);
                data_add += BLINKER_F(",\"messageId\":\"");
                data_add += STRING_format(_messageId);
                data_add += BLINKER_F("\"}");
            }

            data_add += BLINKER_F("}");
        }
        else
        {
            if (_use_broker == aliyun_b)
            {
                data_add += data;
            }
            else if (_use_broker == blinker_b)
            {
                data_add += data.substring(0, data.length() - 1);
                data_add += BLINKER_F(",\"messageId\":\"");
                data_add += STRING_format(_messageId);
                data_add += BLINKER_F("\"}");
            }
        }

        data_add += BLINKER_F(",\"fromDevice\":\"");
        data_add += DEVICE_NAME_MQTT;

        if (_use_broker == aliyun_b)
        {
            data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
        }
        else if (_use_broker == blinker_b)
        {
            data_add += BLINKER_F("\",\"toDevice\":\"ServerReceiver\"");
        }

        data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

        if (!isJson(data_add)) return false;

        BLINKER_LOG_ALL(BLINKER_F("MQTT DuerOS Publish..."));
        BLINKER_LOG_FreeHeap_ALL();
        BLINKER_LOG_ALL(BLINKER_F("is_rrpc: "), is_rrpc);

        if (mqtt_MQTT->connected())
        {
            // if (!checkDuerKA())
            // {
            //     return false;
            // }

            if (!checkDuerPrintSpan())
            {
                respDuerTime = millis();
                return false;
            }
            respDuerTime = millis();

            
            char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

            if (is_rrpc)
            {
                
                strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, DEVICE_NAME_MQTT);
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

                BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
            }
            else
            {
                strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_MQTT);
            }

            is_rrpc = false;

            char send_data[BLINKER_MAX_SEND_SIZE];

            if (_use_broker == aliyun_b)
            {
                strcpy(send_data, base64::encode(data_add).c_str());
            }
            else if (_use_broker == blinker_b)
            {
                strcpy(send_data, data_add.c_str());
            }

            if (! mqtt_MQTT->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, send_data))
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                BLINKER_LOG_FreeHeap_ALL();

                isDuerAlive = false;
                
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                BLINKER_LOG_FreeHeap_ALL();

                isDuerAlive = false;

                this->latestTime = millis();

                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
            return false;
        }
    }

    int BlinkerWiFiESP::checkDuerKA() {
        if (millis() - duerKaTime >= 10000)
            return false;
        else
            return true;
    }

    int BlinkerWiFiESP::checkDuerPrintSpan()
    {
        if (millis() - respDuerTime < BLINKER_PRINT_MSG_LIMIT/2)
        {
            if (respDuerTimes > BLINKER_PRINT_MSG_LIMIT/2)
            {
                BLINKER_ERR_LOG(BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

                return false;
            }
            else
            {
                respDuerTimes++;
                return true;
            }
        }
        else
        {
            respDuerTimes = 0;
            return true;
        }
    }
#endif

#if defined(BLINKER_MIOT)
    int BlinkerWiFiESP::miAvail()
    {
        if (!checkInit()) return false;

        if (isMIOTAvail)
        {
            isMIOTAvail = false;
            return true;
        }
        else {
            return false;
        }
    }

    int BlinkerWiFiESP::miPrint(const String & data)
    {
        if (!checkInit()) return false;

        String data_add = BLINKER_F("{\"data\":");
    
        if (_use_broker == aliyun_b)
        {
            data_add += data;
        }
        else if (_use_broker == blinker_b)
        {
            data_add += data.substring(0, data.length() - 1);
            data_add += BLINKER_F(",\"messageId\":\"");
            data_add += STRING_format(_messageId);
            data_add += BLINKER_F("\"}");
        }

        data_add += BLINKER_F(",\"fromDevice\":\"");
        data_add += DEVICE_NAME_MQTT;

        if (_use_broker == aliyun_b)
        {
            data_add += BLINKER_F("\",\"toDevice\":\"MIOT_r\"");
        }
        else if (_use_broker == blinker_b)
        {
            data_add += BLINKER_F("\",\"toDevice\":\"ServerReceiver\"");
        }

        data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

        if (!isJson(data_add)) return false;

        BLINKER_LOG_ALL(BLINKER_F("MQTT MIOT Publish..."));
        BLINKER_LOG_FreeHeap_ALL();

        if (mqtt_MQTT->connected())
        {
            if (!checkMIOTKA())
            {
                return false;
            }

            if (!checkMIOTPrintSpan())
            {
                respMIOTTime = millis();
                return false;
            }
            respMIOTTime = millis();

            
            char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

            if (is_rrpc)
            {
                
                strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, DEVICE_NAME_MQTT);
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
                strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

                BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
            }
            else
            {
                strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_MQTT);
            }

            is_rrpc = false;

            char send_data[BLINKER_MAX_SEND_SIZE];

            if (_use_broker == aliyun_b)
            {
                strcpy(send_data, base64::encode(data_add).c_str());
            }
            else if (_use_broker == blinker_b)
            {
                strcpy(send_data, data_add.c_str());
            }

            if (! mqtt_MQTT->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, send_data))
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                BLINKER_LOG_FreeHeap_ALL();

                isMIOTAlive = false;
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                BLINKER_LOG_FreeHeap_ALL();

                isMIOTAlive = false;

                this->latestTime = millis();

                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
            return false;
        }
    }

    int BlinkerWiFiESP::checkMIOTKA() {
        if (millis() - miKaTime >= 10000)
            return false;
        else
            return true;
    }

    int BlinkerWiFiESP::checkMIOTPrintSpan()
    {
        if (millis() - respMIOTTime < BLINKER_PRINT_MSG_LIMIT/2)
        {
            if (respMIOTTimes > BLINKER_PRINT_MSG_LIMIT/2)
            {
                BLINKER_ERR_LOG(BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

                return false;
            }
            else
            {
                respMIOTTimes++;
                return true;
            }
        }
        else
        {
            respMIOTTimes = 0;
            return true;
        }
    }
#endif

#endif

#endif
