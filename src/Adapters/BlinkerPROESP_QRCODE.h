#ifndef BLINKER_PRO_ESP_QRCODE_H
#define BLINKER_PRO_ESP_QRCODE_H

#if (defined(ESP8266) || defined(ESP32))

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>

    #include <base64.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>

    #include <base64.h>
#endif

#include <EEPROM.h>

#include "../modules/WebSockets/WebSocketsServer.h"
#include "../modules/mqtt/Adafruit_MQTT.h"
#include "../modules/mqtt/Adafruit_MQTT_Client.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "../Adapters/BlinkerPROESP_QRCODE.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

char*       MQTT_HOST_PRO;
char*       MQTT_ID_PRO;
char*       MQTT_NAME_PRO;
char*       MQTT_KEY_PRO;
char*       MQTT_PRODUCTINFO_PRO;
char*       UUID_PRO;
char*       AUTHKEY_PRO;
char*       MQTT_DEVICEID_PRO;
// char*       DEVICE_NAME;
char*       BLINKER_PUB_TOPIC_PRO;
char*       BLINKER_SUB_TOPIC_PRO;
char*       BLINKER_RRPC_SUB_TOPIC_MQTT;
uint16_t    MQTT_PORT_PRO;

enum b_config_t {
    BLINKER_SMART_CONFIG,
    BLINKER_AP_CONFIG
};

b_config_t  _configType = BLINKER_SMART_CONFIG;

class BlinkerPROESP_QRCODE : public BlinkerStream
{
    public :
        BlinkerPROESP_QRCODE();

        void setSmartConfig() { _configType = BLINKER_SMART_CONFIG; BLINKER_LOG_ALL(BLINKER_F("SmartConfig")); }
        void setApConfig() { _configType = BLINKER_AP_CONFIG; BLINKER_LOG_ALL(BLINKER_F("ApConfig")); }
        bool checkSmartConfig() { return _configType == BLINKER_SMART_CONFIG; }
        int connect();
        int connected();
        int mConnected();// { if (!isMQTTinit) return false; else return mqtt->connected(); }
        void disconnect();
        void ping();
        int available();
        int aligenieAvail();
        int duerAvail();
        int miAvail();
        // bool extraAvailable();
        void subscribe();
        char * lastRead();
        void flush();
        int print(char * data, bool needCheck = true);
        int toServer(char * data);
        int bPrint(char * name, const String & data);
        int aliPrint(const String & data);
        int duerPrint(const String & data, bool report = false);
        int miPrint(const String & data);
        // void aliType(const String & type);
        void begin(const char* _key, const char* _type);
        int autoPrint(unsigned long id);
        // bool autoPrint(char *name, char *type, char *data);
        // bool autoPrint(char *name1, char *type1, char *data1, \
        //             char *name2, char *type2, char *data2);
        char * deviceName();
        char * authKey() { return AUTHKEY_PRO; }
        char * token() { if (!isMQTTinit) return ""; else return MQTT_KEY_PRO; }
        int init()
        { 
            // webSocket_PRO.loop();

            // #if defined(ESP8266)
            //     MDNS.update();
            // #endif

            return isMQTTinit;
        }
        int reRegister() { return connectServer(); }
        int deviceRegister() { return connectServer(); }
        int authCheck();
        void freshAlive() { kaTime = millis(); isAlive = true; }
        void sharers(const String & data);
        int  needFreshShare() {
            if (_needCheckShare)
            {
                BLINKER_LOG_ALL(BLINKER_F("needFreshShare"));
                _needCheckShare = false;
                return true;
            }
            else
            {
                return false;
            }
        }
            

    private :
        bool isMQTTinit = false;
        bool isWSinit = false;

        int connectServer();
        void mDNSInit(String name = macDeviceName());
        void checkKA();
        int checkAliKA();
        int checkDuerKA();
        int checkMIOTKA();
        int checkCanPrint();
        int checkCanBprint();
        int checkPrintSpan();
        int checkAliPrintSpan();
        int checkDuerPrintSpan();
        int checkMIOTPrintSpan();
        int pubHello();
        void parseData(const char* data);

    protected :
        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
        uint8_t     _sharerCount = 0;
        uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        const char* _vipKey;
        const char* _deviceType;
        // char*       _authKey;
        // char*       _aliType;
        bool*       isHandle;// = &isConnect;
        // bool*       isHandle = &isConnect_PRO;
        bool        isAlive = false;
        bool        isBavail = false;
        bool        _needCheckShare = false;
        uint32_t    latestTime;
        uint32_t    printTime = 0;
        uint32_t    bPrintTime = 0;
        uint32_t    kaTime = 0;
        uint32_t    linkTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;
        uint8_t     respDuerTimes = 0;
        uint32_t    respDuerTime = 0;
        uint8_t     respMIOTTimes = 0;
        uint32_t    respMIOTTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        uint32_t    duerKaTime = 0;
        bool        isDuerAlive = false;
        bool        isDuerAvail = false;
        uint32_t    miKaTime = 0;
        bool        isMIOTAlive = false;
        bool        isMIOTAvail = false;

        bool        isNew = false;
        bool        isAuth = false;
        bool        isFirst = false;

        int isJson(const String & data);

        uint8_t     reconnect_time = 0;
        uint32_t    _reRegister_time = 0;
        uint8_t     _reRegister_times = 0;

        bool        _isAuthKey = false;

        char        message_id[24];
        bool        is_rrpc = false;
};

// #if defined(ESP8266)
//     extern BearSSL::WiFiClientSecure   client_mqtt;
//     // WiFiClientSecure            client_mqtt;
// #elif defined(ESP32)
//     extern WiFiClientSecure            client_s;
// #endif

// extern WiFiClient              client;


#if defined(ESP8266)
    BearSSL::WiFiClientSecure   client_mqtt;
    // WiFiClientSecure            client_mqtt;
#elif defined(ESP32)
    WiFiClientSecure            client_s;
#endif

WiFiClient              client;
Adafruit_MQTT_Client*       mqtt_PRO;
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe*    iotSub_PRO;

#define WS_SERVERPORT       81
WebSocketsServer webSocket_PRO = WebSocketsServer(WS_SERVERPORT);

char*   msgBuf_PRO;
bool    isFresh_PRO = false;
bool    isConnect_PRO = false;
bool    isAvail_PRO = false;
bool    isApCfg = false;
uint8_t ws_num_PRO = 0;
uint8_t dataFrom_PRO = BLINKER_MSG_FROM_MQTT;

void webSocketEvent_PRO(uint8_t num, WStype_t type, \
                    uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

            isConnect_PRO = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket_PRO.remoteIP(num);
                
                BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                                BLINKER_F(", Connected from: "), ip, 
                                BLINKER_F(", url: "), (char *)payload);
                
                // send message to client
                webSocket_PRO.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num_PRO = num;

                if (!isApCfg) isConnect_PRO = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                            BLINKER_F(", get Text: "), (char *)payload, \
                            BLINKER_F(", length: "), length);
            
            if (length < BLINKER_MAX_READ_SIZE) {
                if (isFresh_PRO) free(msgBuf_PRO);
                msgBuf_PRO = (char*)malloc((length+1)*sizeof(char));
                strcpy(msgBuf_PRO, (char*)payload);
                isAvail_PRO = true;
                isFresh_PRO = true;
            }

            if (!isApCfg) isConnect_PRO = true;

            if (!isApCfg) dataFrom_PRO = BLINKER_MSG_FROM_WS;

            ws_num_PRO = num;

            // BLINKER_LOG_ALL(BLINKER_F("isConnect_PRO: "), isConnect_PRO);

            // send message to client
            // webSocket_PRO.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket_PRO.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket_PRO.sendBIN(num, payload, length);
            break;
    }
}

BlinkerPROESP_QRCODE::BlinkerPROESP_QRCODE() { isHandle = &isConnect_PRO; }

int BlinkerPROESP_QRCODE::connect()
{
    int8_t ret;
    
#if defined(ESP8266)
    MDNS.update();
#endif

    webSocket_PRO.loop();

    if (!isMQTTinit) {
        return *isHandle;
    }

    if (mqtt_PRO->connected())
    {
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    {
        yield();
        return false;
    }

    BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));

    #if defined(ESP8266)
        client_mqtt.setInsecure();
        ::delay(10);
    #endif

    BLINKER_LOG_FreeHeap_ALL();

    if ((ret = mqtt_PRO->connect()) != 0)
    {
        BLINKER_LOG(mqtt_PRO->connectErrorString(ret));
        BLINKER_LOG(BLINKER_F("Retrying MQTT connection in "), \
                    BLINKER_MQTT_CONNECT_TIMESLOT/1000, \
                    BLINKER_F(" seconds..."));

        if (ret == 4) 
        {
            if (_reRegister_times < BLINKER_SERVER_CONNECT_LIMIT)
            {
                if (reRegister())
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
                if (millis() - _reRegister_time >= 60000 * 5) _reRegister_times = 0;
            }

            BLINKER_LOG_ALL(BLINKER_F("_reRegister_times: "), _reRegister_times);
        }

        this->latestTime = millis();
        reconnect_time += 1;
        if (reconnect_time >= 12)
        {
            if (_reRegister_times < BLINKER_SERVER_CONNECT_LIMIT)
            {
                if (reRegister())
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
                if (millis() - _reRegister_time >= 60000 * 5) _reRegister_times = 0;
            }
            
            reconnect_time = 0;

            BLINKER_LOG_ALL(BLINKER_F("_reRegister_times: "), _reRegister_times);
        }
        return false;
    }
    
    reconnect_time = 0;
    
    BLINKER_LOG(BLINKER_F("MQTT Connected!"));
    BLINKER_LOG_FreeHeap();

    if (isNew)
    {
        if (pubHello()) {
            isNew = false;
        }
    }

    this->latestTime = millis();

    return true;
}

int BlinkerPROESP_QRCODE::connected()
{ 
    if (!isMQTTinit)
    {
        return *isHandle;
    }

    return mqtt_PRO->connected() || *isHandle; 
}

int BlinkerPROESP_QRCODE::mConnected()
{
    if (!isMQTTinit) return false;
    else return mqtt_PRO->connected();
}

void BlinkerPROESP_QRCODE::disconnect()
{
    if (isMQTTinit) mqtt_PRO->disconnect();

    if (*isHandle) webSocket_PRO.disconnect();
}

void BlinkerPROESP_QRCODE::ping()
{
    BLINKER_LOG_ALL(BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!isMQTTinit) return;

    if (!mqtt_PRO->ping())
    {
        disconnect();
        // delay(100);

        // connect();
    }
    else
    {
        this->latestTime = millis();
    }
}

int BlinkerPROESP_QRCODE::available()
{
#if defined(ESP8266)
    MDNS.update();
#endif

    // BLINKER_LOG("CHECK available");

    delay(1);

    webSocket_PRO.loop();

    if (isMQTTinit) {
        checkKA();

        // if (!mqtt_PRO->connected() || \
        //     (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
        if ((millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
        {
            ping();
        }
        else
        {
            subscribe();
        }
    }

    if (isAvail_PRO)
    {
        isAvail_PRO = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerPROESP_QRCODE::aligenieAvail()
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

int BlinkerPROESP_QRCODE::duerAvail()
{
    if (isDuerAvail)
    {
        isDuerAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerPROESP_QRCODE::miAvail()
{
    if (isMIOTAvail)
    {
        isMIOTAvail = false;
        return true;
    }
    else {
        return false;
    }
}

// bool BlinkerPROESP_QRCODE::extraAvailable()
// {
//     if (isBavail)
//     {
//         isBavail = false;        
//         return true;
//     }
//     else
//     {
//         return false;
//     }
// }

void BlinkerPROESP_QRCODE::subscribe()
{
    if (!isMQTTinit) return;

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt_PRO->readSubscription(10)))
    {
        if (subscription == iotSub_PRO)
        {
            BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub_PRO->lastread);
            
            parseData((char *)iotSub_PRO->lastread);
            // // DynamicJsonBuffer jsonBuffer;
            // // JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub_PRO->lastread));
            // DynamicJsonDocument jsonBuffer(1024);
            // DeserializationError error = deserializeJson(jsonBuffer, String((char *)iotSub_PRO->lastread));
            // JsonObject root = jsonBuffer.as<JsonObject>();

            // String _uuid = root["fromDevice"];
            // String dataGet = root["data"];
            
            // BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
            // BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);
            
            // if (strcmp(_uuid.c_str(), UUID_PRO) == 0)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));
                
            //     kaTime = millis();
            //     isAvail_PRO = true;
            //     isAlive = true;

            //     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
            // }
            // else if (_uuid == BLINKER_CMD_ALIGENIE)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("form AliGenie"));
                
            //     aliKaTime = millis();
            //     isAliAlive = true;
            //     isAliAvail = true;
            // }
            // else if (_uuid == BLINKER_CMD_DUEROS)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("form DuerOS"));
                
            //     duerKaTime = millis();
            //     isDuerAlive = true;
            //     isDuerAvail = true;
            // }
            // else if (_uuid == BLINKER_CMD_MIOT)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("form DuerOS"));
                
            //     miKaTime = millis();
            //     isMIOTAlive = true;
            //     isMIOTAvail = true;
            // }         
            // else if (_uuid == BLINKER_CMD_SERVERCLIENT)
            // {
            //     BLINKER_LOG_ALL(BLINKER_F("form Sever"));

            //     isAvail_PRO = true;
            //     isAlive = true;

            //     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
            // }
            // else
            // {
            //     if (_sharerCount)
            //     {
            //         for (uint8_t num = 0; num < _sharerCount; num++)
            //         {
            //             if (strcmp(_uuid.c_str(), _sharers[num]->uuid()) == 0)
            //             {
            //                 _sharerFrom = num;

            //                 kaTime = millis();

            //                 BLINKER_LOG_ALL(BLINKER_F("From sharer: "), _uuid);
            //                 BLINKER_LOG_ALL(BLINKER_F("sharer num: "), num);

            //                 _needCheckShare = false;

            //                 break;
            //             }
            //             else
            //             {
            //                 BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found,"
            //                             "check is from bridge/share device," 
            //                             "data: "), dataGet);

            //                 _needCheckShare = true;
            //             }                        
            //         }
            //     }
                
            //     // {
            //         // dataGet = String((char *)iotSub_PRO->lastread);
            //     // root.printTo(dataGet);
            //     serializeJson(root, dataGet);
                    
            //         // BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
            //         //                     check is from bridge/share device, \
            //         //                     data: "), dataGet);
                
            //         // // return;

            //         // // isBavail = true;

            //         // _needCheckShare = true;
            //     // }

            //     isAvail_PRO = true;
            //     isAlive = true;
            // }

            // memset(msgBuf_PRO, 0, BLINKER_MAX_READ_SIZE);
            // memcpy(msgBuf_PRO, dataGet.c_str(), dataGet.length());

            // if (isFresh_PRO) free(msgBuf_PRO);
            // msgBuf_PRO = (char*)malloc((dataGet.length()+1)*sizeof(char));
            // strcpy(msgBuf_PRO, dataGet.c_str());
            // isFresh_PRO = true;
            
            // this->latestTime = millis();

            // dataFrom_PRO = BLINKER_MSG_FROM_MQTT;
        }
    }

    if (mqtt_PRO->check_extra())
    {
        BLINKER_LOG_ALL(BLINKER_F("Got extra topic: "), (char *)mqtt_PRO->get_extra_topic());
        BLINKER_LOG_ALL(BLINKER_F("Got extra data: "), (char *)mqtt_PRO->get_extra_data());

        if (strncmp(BLINKER_RRPC_SUB_TOPIC_MQTT, 
                    (char *)mqtt_PRO->get_extra_topic(), 
                    strlen(BLINKER_RRPC_SUB_TOPIC_MQTT))
                    == 0)
        {
            // char message_id[24];
            memset(message_id, '\0', 24);

            memmove(message_id, 
                mqtt_PRO->get_extra_topic()+strlen(BLINKER_RRPC_SUB_TOPIC_MQTT),
                strlen(mqtt_PRO->get_extra_topic()) - strlen(BLINKER_RRPC_SUB_TOPIC_MQTT));
            
            BLINKER_LOG_ALL(BLINKER_F("from server RRPC, message_id: "), message_id);

            parseData((char *)mqtt_PRO->get_extra_data());

            // char BLINKER_RRPC_PUB_TOPIC_MQTT[128];
            
            // strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
            // strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_PRO);
            // strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            // strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_DEVICEID_PRO);
            // strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            // strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            is_rrpc = true;
            
            // BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);

            // mqtt_PRO->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, base64::encode("hello blinker").c_str());
        }
    }
}

void BlinkerPROESP_QRCODE::parseData(const char* data)
{
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, String(data));
    JsonObject root = jsonBuffer.as<JsonObject>();

    String _uuid = root["fromDevice"].as<String>();
    String dataGet = root["data"].as<String>();

    BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
    BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);

    if (strcmp(_uuid.c_str(), UUID_PRO) == 0)
    {
        BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));

        kaTime = millis();
        isAvail_PRO = true;
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
    else if (_uuid == BLINKER_CMD_SERVERCLIENT)
    {
        BLINKER_LOG_ALL(BLINKER_F("form Sever"));

        isAvail_PRO = true;
        isAlive = true;

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    }
    else
    {
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

                    break;
                }
                else
                {
                    BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, check is from bridge/share device, data: "), dataGet);

                    _needCheckShare = true;
                }
            }
        }
        // else
        // {
            // dataGet = String((char *)iotSub_PRO->lastread);
            // root.printTo(dataGet);
            serializeJson(root, dataGet);

        //     BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, 
        //                         check is from bridge/share device, \
        //                         data: "), dataGet);

        //     // return;

        //     // isBavail = true;

        //     _needCheckShare = true;
        // }

        isAvail_PRO = true;
        isAlive = true;
    }

    // memset(msgBuf_PRO, 0, BLINKER_MAX_READ_SIZE);
    // memcpy(msgBuf_PRO, dataGet.c_str(), dataGet.length());

    if (isFresh_PRO) free(msgBuf_PRO);
    msgBuf_PRO = (char*)malloc((dataGet.length()+1)*sizeof(char));
    strcpy(msgBuf_PRO, dataGet.c_str());
    isFresh_PRO = true;

    this->latestTime = millis();

    dataFrom_PRO = BLINKER_MSG_FROM_MQTT;
}

char * BlinkerPROESP_QRCODE::lastRead()
{
    if (isFresh_PRO) return msgBuf_PRO;
    return "";
}

void BlinkerPROESP_QRCODE::flush()
{
    if (isFresh_PRO)
    {
        free(msgBuf_PRO); isFresh_PRO = false; isAvail_PRO = false;
        isAliAvail = false; isDuerAvail = false; isMIOTAvail = false;//isBavail = false;
    }
}

int BlinkerPROESP_QRCODE::print(char * data, bool needCheck)
// int BlinkerPROESP_QRCODE::print(const String & data, bool needCheck)
{
    // BLINKER_LOG_FreeHeap();
    if (*isHandle && dataFrom_PRO == BLINKER_MSG_FROM_WS)
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
        
        webSocket_PRO.sendTXT(ws_num_PRO, data);

        return true;
    }
    else
    {
        // // String payload;
        // if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
        // {
        //     uint8_t num = strlen(data) - 1;
        //     for(uint8_t c_num = num; c_num > 0; c_num--)
        //     {
        //         data[c_num+7] = data[c_num-1];
        //     }
        //     // payload = BLINKER_F("{\"data\":");
        //     // payload += data.substring(0, data.length() - 1);
        //     // payload += BLINKER_F(",\"fromDevice\":\"");
        //     // payload += MQTT_DEVICEID_PRO;
        //     // payload += BLINKER_F("\",\"toDevice\":\"");
        //     // payload += UUID_PRO;
        //     // payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        // }
        // else
        // {
        //     uint8_t num = strlen(data);
        //     for(uint8_t c_num = num; c_num > 0; c_num--)
        //     {
        //         data[c_num+7] = data[c_num-1];
        //     }
        //     // payload = BLINKER_F("{\"data\":");
        //     // payload += data;
        //     // payload += BLINKER_F(",\"fromDevice\":\"");
        //     // payload += MQTT_DEVICEID_PRO;
        //     // payload += BLINKER_F("\",\"toDevice\":\"");
        //     // payload += UUID_PRO;
        //     // payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        // }

        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';

        String data_add = BLINKER_F("{\"data\":");
        
        for(uint16_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }

        data_add = BLINKER_F(",\"fromDevice\":\"");
        strcat(data, data_add.c_str());
        strcat(data, MQTT_DEVICEID_PRO);
        data_add = BLINKER_F("\",\"toDevice\":\"");
        strcat(data, data_add.c_str());
        if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        {
            strcat(data, _sharers[_sharerFrom]->uuid());
        }
        else
        {
            strcat(data, UUID_PRO);
        }
        data_add = BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        strcat(data, data_add.c_str());

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        if (!isJson(STRING_format(data))) return false;

        // String data_add = BLINKER_F("{\"data\":");
        // data_add += data;
        // data_add += BLINKER_F(",\"fromDevice\":\"");
        // data_add += MQTT_DEVICEID_PRO;
        // data_add += BLINKER_F("\",\"toDevice\":\"");
        // if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        // {
        //     data_add += _sharers[_sharerFrom]->uuid();
        // }
        // else
        // {
        //     data_add += UUID_PRO;
        // }

        // data_add += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");

        // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        // if (!isJson(data_add)) return false;
        
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

        if (mqtt_PRO->connected())
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
            }

            if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data))
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

int BlinkerPROESP_QRCODE::toServer(char * data)
{
    // if (!checkInit()) return false;

    if (!isJson(STRING_format(data))) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT Publish to server..."));
    BLINKER_LOG_FreeHeap_ALL();

    bool _alive = isAlive;

    if (mqtt_PRO->connected())
    {
        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data))
        {
            BLINKER_LOG_ALL(data);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();
            
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

int BlinkerPROESP_QRCODE::bPrint(char * name, const String & data)
{
    // String payload;
    // if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
    // {
    //     payload = BLINKER_F("{\"data\":");
    //     payload += data.substring(0, data.length() - 1);
    //     payload += BLINKER_F(",\"fromDevice\":\"");
    //     payload += MQTT_DEVICEID_PRO;
    //     payload += BLINKER_F("\",\"toDevice\":\"");
    //     payload += name;
    //     payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    // }
    // else
    // {
    //     payload = BLINKER_F("{\"data\":");
    //     payload += data;
    //     payload += BLINKER_F(",\"fromDevice\":\"");
    //     payload += MQTT_DEVICEID_PRO;
    //     payload += BLINKER_F("\",\"toDevice\":\"");
    //     payload += name;
    //     payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    // }

    // uint8_t num = strlen(data);
    // for(uint8_t c_num = num; c_num > 0; c_num--)
    // {
    //     data[c_num+7] = data[c_num-1];
    // }

    String data_add = BLINKER_F("{\"data\":");
    // for(uint8_t c_num = 0; c_num < 8; c_num++)
    // {
    //     data[c_num] = data_add[c_num];
    // }

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F("\",\"toDevice\":\"");
    data_add += name;
    data_add += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");

    // data_add = BLINKER_F(",\"fromDevice\":\"");
    // strcat(data, data_add.c_str());
    // strcat(data, MQTT_DEVICEID_PRO);
    // data_add = BLINKER_F("\",\"toDevice\":\"");
    // strcat(data, data_add.c_str());
    // strcat(data, name);
    // data_add = BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    // strcat(data, data_add.c_str());

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT Bridge Publish..."));

    // bool _alive = isAlive;
    // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

    // if (!state) {
    //     state = (STRING_contains_string(data, BLINKER_CMD_STATE) 
    //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
    // }

    if (mqtt_PRO->connected()) {
        // if (!state) {
        if (!checkCanBprint()) {
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        // }

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_PRO, BLINKER_PUB_TOPIC_PRO);

        // if (! iotPub.publish(payload.c_str())) {

        // String bPubTopic = BLINKER_F("");

        // if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET)
        // {
        //     bPubTopic = MQTT_PRODUCTINFO_PRO;
        //     bPubTopic += BLINKER_F("/");
        //     bPubTopic += name;
        //     bPubTopic += BLINKER_F("/r");
        // }
        // else
        // {
        //     bPubTopic = BLINKER_PUB_TOPIC_PRO;
        // }

        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            
            bPrintTime = millis();

            // if (!_alive) {
            //     isAlive = false;
            // }

            this->latestTime = millis();

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

int BlinkerPROESP_QRCODE::aliPrint(const String & data)
{
    // String payload;

    // payload = BLINKER_F("{\"data\":");
    // payload += data;
    // payload += BLINKER_F(",\"fromDevice\":\"");
    // payload += MQTT_DEVICEID_PRO;
    // payload += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    // payload += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    // uint8_t num = strlen(data);
    // for(uint8_t c_num = num; c_num > 0; c_num--)
    // {
    //     data[c_num+7] = data[c_num-1];
    // }

    String data_add = BLINKER_F("{\"data\":");
    // for(uint8_t c_num = 0; c_num < 8; c_num++)
    // {
    //     data[c_num] = data_add[c_num];
    // }

    // data_add = BLINKER_F(",\"fromDevice\":\"");
    // strcat(data, data_add.c_str());
    // strcat(data, MQTT_DEVICEID_PRO);
    // data_add = BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    // strcat(data, data_add.c_str());
    // data_add = BLINKER_F(",\"deviceType\":\"vAssistant\"}");
    // strcat(data, data_add.c_str());

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
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

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_PRO, BLINKER_PUB_TOPIC_PRO);

        // if (! iotPub.publish(payload.c_str())) {

        
        char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

        if (is_rrpc)
        {
            
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_PRO);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_DEVICEID_PRO);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
        }
        else
        {
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_PRO);
        }

        is_rrpc = false;

        if (! mqtt_PRO->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, base64::encode(data_add).c_str()))
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

int BlinkerPROESP_QRCODE::duerPrint(const String & data, bool report)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT DuerOS Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
    {
        
        if (!checkDuerKA())
        {
            return false;
        }

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
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_PRO);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_DEVICEID_PRO);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
        }
        else
        {
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_PRO);
        }

        if (! mqtt_PRO->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, is_rrpc ? base64::encode(data_add).c_str() : data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            is_rrpc = false;
            
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

            is_rrpc = false;

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerPROESP_QRCODE::miPrint(const String & data)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F("\",\"toDevice\":\"MIOT_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT MIOT Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
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
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_PRO);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_DEVICEID_PRO);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            BLINKER_LOG_ALL(BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
        }
        else
        {
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_PRO);
        }

        is_rrpc = false;

        if (! mqtt_PRO->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, base64::encode(data_add).c_str()))
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

// void BlinkerPROESP_QRCODE::aliType(const String & type)
// {
//     _aliType = (char*)malloc((type.length()+1)*sizeof(char));
//     strcpy(_aliType, type.c_str());
//     BLINKER_LOG_ALL(BLINKER_F("_aliType: "), _aliType);
// }

void BlinkerPROESP_QRCODE::begin(const char* _key, const char* _type)
{
    _vipKey = _key;
    _deviceType = _type;
    
    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    mDNSInit();
}

int BlinkerPROESP_QRCODE::autoPrint(unsigned long id)
{
    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"auto\":{\"trig\":true,");
    payload += BLINKER_F("\"id\":");
    payload += String(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(MQTT_DEVICEID_PRO);
    payload += BLINKER_F("\",\"deviceType\":\"Auto\"");
    payload += BLINKER_F(",\"toDevice\":\"serverClient\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

    if (mqtt_PRO->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_PRO, BLINKER_PUB_TOPIC_PRO);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, payload.c_str()))
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

// bool BlinkerPROESP_QRCODE::autoPrint(char *name, char *type, char *data)
// {
//     String payload = BLINKER_F("{\"data\":{");
//     payload += STRING_format(data);
//     payload += BLINKER_F("},\"fromDevice\":\"");
//     payload += STRING_format(MQTT_ID_PRO);
//     payload += BLINKER_F("\",\"toDevice\":\"");
//     payload += name;
//     payload += BLINKER_F("\",\"deviceType\":\"");
//     payload += type;
//     payload += BLINKER_F("\"}");
        
//     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));
    
//     if (mqtt_PRO->connected()) {
//         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
//             linkTime == 0)
//         {
//             linkTime = millis();
            
//             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));
            
//             return true;
//         }
//         else {
//             BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
//             return false;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

// bool BlinkerPROESP_QRCODE::autoPrint(char *name1, char *type1, char *data1
//     , char *name2, char *type2, char *data2)
// {
//     String payload = BLINKER_F("{\"data\":{");
//     payload += STRING_format(data1);
//     payload += BLINKER_F("},\"fromDevice\":\"");
//     payload += STRING_format(MQTT_ID_PRO);
//     payload += BLINKER_F("\",\"toDevice\":\"");
//     payload += name1;
//     payload += BLINKER_F("\",\"deviceType\":\"");
//     payload += type1;
//     payload += BLINKER_F("\"}");
        
//     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));
    
//     if (mqtt_PRO->connected())
//     {
//         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
//             linkTime == 0)
//         {
//             linkTime = millis();

//             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));

//             payload = BLINKER_F("{\"data\":{");
//             payload += STRING_format(data2);
//             payload += BLINKER_F("},\"fromDevice\":\"");
//             payload += STRING_format(MQTT_ID_PRO);
//             payload += BLINKER_F("\",\"toDevice\":\"");
//             payload += name2;
//             payload += BLINKER_F("\",\"deviceType\":\"");
//             payload += type2;
//             payload += BLINKER_F("\"}");
                
//             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));
            
//             return true;
//         }
//         else
//         {
//             BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
//             return false;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

char * BlinkerPROESP_QRCODE::deviceName() { return MQTT_DEVICEID_PRO;/*MQTT_ID_PRO;*/ }

void BlinkerPROESP_QRCODE::sharers(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("sharers data: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success()) return;
    if (error) return;

    String user_name = "";

    if (_sharerCount)
    {
        for (_sharerCount; _sharerCount > 0; _sharerCount--)
        {
            delete _sharers[_sharerCount - 1];
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

int BlinkerPROESP_QRCODE::authCheck()
{
    uint8_t _authCheck;
    
    BLINKER_LOG_ALL(BLINKER_F("authCheck start"));
    
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
    if (_authCheck == BLINKER_AUTH_CHECK_DATA)
    {
        EEPROM.commit();
        EEPROM.end();
        isAuth = true;
        
        BLINKER_LOG_ALL(BLINKER_F("authCheck end"));
        
        return true;
    }
    EEPROM.commit();
    EEPROM.end();
    
    BLINKER_LOG_ALL(BLINKER_F("authCheck end"));
    
    return false;
}

int BlinkerPROESP_QRCODE::connectServer() {
    const int httpsPort = 443;
    #if defined(ESP8266)
        String host = BLINKER_F(BLINKER_SERVER_HOST);
        client_mqtt.stop();
    #elif defined(ESP32)
        String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    #endif
    if (!_isAuthKey)
    {
    #if defined(ESP8266)
        // String host = BLINKER_F(BLINKER_SERVER_HOST);
        String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");

        // WiFiClientSecure client_s;

    //     BearSSL::WiFiClientSecure *client_s;

    //     client_s = new BearSSL::WiFiClientSecure();

    //     client_mqtt.stop();
        
    //     BLINKER_LOG_ALL(BLINKER_F("connecting to "), host);

    //     // BLINKER_LOG_FreeHeap();
        
    //     uint8_t connet_times = 0;
    //     // client_s.stop();
    //     ::delay(100);

    //     bool mfln = client_s->probeMaxFragmentLength(host, httpsPort, 1024);
    //     if (mfln) {
    //         client_s->setBufferSizes(1024, 1024);
    //     }
    //     // client_s.setFingerprint(fingerprint.c_str());
        
    //     client_s->setInsecure();

    //     // while (1) {
    //         bool cl_connected = false;
    //         if (!client_s->connect(host, httpsPort)) {
    //             BLINKER_ERR_LOG(BLINKER_F("server connection failed"));
    //             // connet_times++;

    //             ::delay(1000);
    //         }
    //         else {
    //             BLINKER_LOG_ALL(BLINKER_F("connection succeed"));
    //             cl_connected = true;

    //             // break;
    //         }

    //         // if (connet_times >= 4 && !cl_connected)  return BLINKER_CMD_FALSE;
    //     // }

    //     String client_msg;

    //     String url_iot = BLINKER_F("/api/v1/user/device/register?deviceType=");
    //     url_iot += _deviceType;
    //     url_iot += BLINKER_F("&deviceName=");
    //     url_iot += macDeviceName();

    //     if (_deviceType == BLINKER_SMART_LAMP) {
    //         url_iot += BLINKER_F("&aliType=light");
    //         url_iot += BLINKER_F("&duerType=LIGHT");
    //     }
    //     else if (_deviceType == BLINKER_SMART_PLUGIN) {
    //         url_iot += BLINKER_F("&aliType=outlet");
    //         url_iot += BLINKER_F("&duerType=SOCKET");
    //     }
    //     else if (_deviceType == BLINKER_AIR_DETECTOR) {
    //         url_iot += BLINKER_F("&aliType=sensor");
    //         url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    //     }

    // // #if defined(BLINKER_ALIGENIE_LIGHT)
    // //     url_iot += BLINKER_F("&aliType=light");
    // // #elif defined(BLINKER_ALIGENIE_OUTLET)
    // //     url_iot += BLINKER_F("&aliType=outlet");
    // // #elif defined(BLINKER_ALIGENIE_SWITCH)
    // // #elif defined(BLINKER_ALIGENIE_SENSOR)
    // //     url_iot += BLINKER_F("&aliType=sensor");
    // // #endif

    //     BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), host, url_iot);
        
    //     client_msg = BLINKER_F("GET ");
    //     client_msg += url_iot;
    //     client_msg += BLINKER_F(" HTTP/1.1\r\nHost: ");
    //     client_msg += host;
    //     client_msg += BLINKER_F(":");
    //     client_msg += STRING_format(httpsPort);
    //     client_msg += BLINKER_F("\r\nConnection: close\r\n\r\n");

    //     client_s->print(client_msg);
        
    //     BLINKER_LOG_ALL(BLINKER_F("client_msg: "), client_msg);

    //     unsigned long timeout = millis();
    //     while (client_s->available() == 0) {
    //         if (millis() - timeout > 5000) {
    //             BLINKER_LOG_ALL(BLINKER_F(">>> Client Timeout !"));
    //             client_s->stop();
    //             return false;
    //         }
    //     }

    //     String _dataGet;
    //     String lastGet;
    //     String lengthOfJson;
    //     while (client_s->available()) {
    //         // String line = client_s.readStringUntil('\r');
    //         _dataGet = client_s->readStringUntil('\n');

    //         if (_dataGet.startsWith("Content-Length: ")){
    //             int addr_start = _dataGet.indexOf(' ');
    //             int addr_end = _dataGet.indexOf('\0', addr_start + 1);
    //             lengthOfJson = _dataGet.substring(addr_start + 1, addr_end);
    //         }

    //         if (_dataGet == "\r") {
    //             BLINKER_LOG_ALL(BLINKER_F("headers received"));
                
    //             break;
    //         }
    //     }

    //     for(int i=0;i<lengthOfJson.toInt();i++){
    //         lastGet += (char)client_s->read();
    //     }

    //     // BLINKER_LOG_FreeHeap();

    //     client_s->stop();
    //     client_s->flush();

    //     free(client_s);

    //     // BLINKER_LOG_FreeHeap();

    //     _dataGet = lastGet;
        
    //     BLINKER_LOG_ALL(BLINKER_F("_dataGet: "), _dataGet);

    //     String payload = _dataGet;



        // client_mqtt.stop();

        std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

        // client_s->setFingerprint(fingerprint);
        client_s->setInsecure();

        String url_iot = BLINKER_F("/api/v1/user/device/auth/get?deviceType=");
            url_iot += _deviceType;
            url_iot += BLINKER_F("&typeKey=");
            url_iot += _vipKey;
            url_iot += BLINKER_F("&deviceName=");
            url_iot += macDeviceName();
            

        #if defined(BLINKER_ALIGENIE_LIGHT)
            url_iot += BLINKER_F("&aliType=light");
        #elif defined(BLINKER_ALIGENIE_OUTLET)
            url_iot += BLINKER_F("&aliType=outlet");
        #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
            url_iot += BLINKER_F("&aliType=multi_outlet");
        #elif defined(BLINKER_ALIGENIE_SENSOR)
            url_iot += BLINKER_F("&aliType=sensor");
        #elif defined(BLINKER_ALIGENIE_TYPE)
            url_iot += BLINKER_ALIGENIE_TYPE;
        #endif

        #if defined(BLINKER_DUEROS_LIGHT)
            url_iot += BLINKER_F("&duerType=LIGHT");
        #elif defined(BLINKER_DUEROS_OUTLET)
            url_iot += BLINKER_F("&duerType=SOCKET");
        #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
            url_iot += BLINKER_F("&duerType=MULTI_SOCKET");
        #elif defined(BLINKER_DUEROS_SENSOR)
            url_iot += BLINKER_F("&duerType=AIR_MONITOR");
        #elif defined(BLINKER_DUEROS_TYPE)
            url_iot += BLINKER_DUEROS_TYPE;
        #endif

        #if defined(BLINKER_MIOT_LIGHT)
            url_iot += BLINKER_F("&miType=light");
        #elif defined(BLINKER_MIOT_OUTLET)
            url_iot += BLINKER_F("&miType=outlet");
        #elif defined(BLINKER_MIOT_MULTI_OUTLET)
            url_iot += BLINKER_F("&miType=multi_outlet");
        #elif defined(BLINKER_MIOT_SENSOR)
            url_iot += BLINKER_F("&miType=sensor");
        #elif defined(BLINKER_MIOT_TYPE)
            url_iot += BLINKER_MIOT_TYPE;
        #endif

        url_iot = "https://" + host + url_iot;

        BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

        HTTPClient http;

        String payload;

        if (http.begin(*client_s, url_iot)) {  // HTTPS

            // Serial.print("[HTTPS] GET...\n");
            // start connection and send HTTP header
            int httpCode = http.GET();

            // httpCode will be negative on error
            if (httpCode > 0) {
                // HTTP header has been send and Server response header has been handled
                
                BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                    payload = http.getString();
                    // Serial.println(payload);
                }
            } else {
                BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
                payload = http.getString();
                BLINKER_LOG(payload);
            }

            http.end();
        } else {
            // Serial.printf("[HTTPS] Unable to connect\n");
        }

    #elif defined(ESP32)
        // String host = BLINKER_F(BLINKER_SERVER_HTTPS);
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
    // #endif

        HTTPClient http;

        String url_iot = host;
        url_iot += BLINKER_F("/api/v1/user/device/auth/get?deviceType=");
        url_iot += _deviceType;
        url_iot += BLINKER_F("&typeKey=");
        url_iot += _vipKey;
        url_iot += BLINKER_F("&deviceName=");
        url_iot += macDeviceName();

        #if defined(BLINKER_ALIGENIE_LIGHT)
            url_iot += BLINKER_F("&aliType=light");
        #elif defined(BLINKER_ALIGENIE_OUTLET)
            url_iot += BLINKER_F("&aliType=outlet");
        #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
            url_iot += BLINKER_F("&aliType=multi_outlet");
        #elif defined(BLINKER_ALIGENIE_SENSOR)
            url_iot += BLINKER_F("&aliType=sensor");
        #elif defined(BLINKER_ALIGENIE_TYPE)
            url_iot += BLINKER_ALIGENIE_TYPE;
        #endif

        #if defined(BLINKER_DUEROS_LIGHT)
            url_iot += BLINKER_F("&duerType=LIGHT");
        #elif defined(BLINKER_DUEROS_OUTLET)
            url_iot += BLINKER_F("&duerType=SOCKET");
        #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
            url_iot += BLINKER_F("&duerType=MULTI_SOCKET");
        #elif defined(BLINKER_DUEROS_SENSOR)
            url_iot += BLINKER_F("&duerType=AIR_MONITOR");
        #elif defined(BLINKER_DUEROS_TYPE)
            url_iot += BLINKER_DUEROS_TYPE;
        #endif

        #if defined(BLINKER_MIOT_LIGHT)
            url_iot += BLINKER_F("&miType=light");
        #elif defined(BLINKER_MIOT_OUTLET)
            url_iot += BLINKER_F("&miType=outlet");
        #elif defined(BLINKER_MIOT_MULTI_OUTLET)
            url_iot += BLINKER_F("&miType=multi_outlet");
        #elif defined(BLINKER_MIOT_SENSOR)
            url_iot += BLINKER_F("&miType=sensor");
        #elif defined(BLINKER_MIOT_TYPE)
            url_iot += BLINKER_MIOT_TYPE;
        #endif

    // #if defined(BLINKER_ALIGENIE_LIGHT)
    //     url_iot += BLINKER_F("&aliType=light");
    // #elif defined(BLINKER_ALIGENIE_OUTLET)
    //     url_iot += BLINKER_F("&aliType=outlet");
    // #elif defined(BLINKER_ALIGENIE_SWITCH)
    // #elif defined(BLINKER_ALIGENIE_SENSOR)
    //     url_iot += BLINKER_F("&aliType=sensor");
    // #endif

        BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

    // #if defined(ESP8266)
    //     http.begin(url_iot, fingerprint); //HTTP
    // #elif defined(ESP32)
        // http.begin(url_iot, ca); TODO
        http.begin(url_iot);
    // #endif
        int httpCode = http.GET();

        String payload;

        if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled

            BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK) {
                payload = http.getString();
                // BLINKER_LOG(payload);
            }
        }
        else {
            BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
            payload = http.getString();
            BLINKER_LOG(payload);
        }

        http.end();
    #endif

        BLINKER_LOG_ALL(BLINKER_F("reply was:"));
        BLINKER_LOG_ALL(BLINKER_F("=============================="));
        BLINKER_LOG_ALL(payload);
        BLINKER_LOG_ALL(BLINKER_F("=============================="));

        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& root = jsonBuffer.parseObject(payload);
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, payload);
        JsonObject root = jsonBuffer.as<JsonObject>();

        if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
            !STRING_contains_string(payload, BLINKER_CMD_AUTHKEY)) {
            // while(1) {
                BLINKER_ERR_LOG(BLINKER_F("Maybe you have put in the wrong AuthKey!"));
                BLINKER_ERR_LOG(BLINKER_F("Or maybe your request is too frequently!"));
                BLINKER_ERR_LOG(BLINKER_F("Or maybe your network is disconnected!"));
                // ::delay(60000);

                return false;
            // }
        }

        String _getAuthKey = root[BLINKER_CMD_DETAIL][BLINKER_CMD_AUTHKEY];

        AUTHKEY_PRO = (char*)malloc((_getAuthKey.length()+1)*sizeof(char));
        strcpy(AUTHKEY_PRO, _getAuthKey.c_str());

        BLINKER_LOG_ALL(BLINKER_F("===================="));
        BLINKER_LOG_ALL(BLINKER_F("AUTHKEY_PRO: "), AUTHKEY_PRO);
        BLINKER_LOG_ALL(BLINKER_F("===================="));

        _isAuthKey = true;
    }

#if defined(ESP8266)
    // client_mqtt.stop();

    std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

    // client_s->setFingerprint(fingerprint);
    client_s->setInsecure();

    String url_iot = BLINKER_F("/api/v1/user/device/auth?authKey=");
    url_iot += AUTHKEY_PRO;
    // url_iot += _aliType;
    // url_iot += _duerType;

    #if defined(BLINKER_ALIGENIE_LIGHT)
        url_iot += BLINKER_F("&aliType=light");
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        url_iot += BLINKER_F("&aliType=outlet");
    #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
        url_iot += BLINKER_F("&aliType=multi_outlet");
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        url_iot += BLINKER_F("&aliType=sensor");
    #elif defined(BLINKER_ALIGENIE_TYPE)
        url_iot += BLINKER_ALIGENIE_TYPE;
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        url_iot += BLINKER_F("&duerType=LIGHT");
    #elif defined(BLINKER_DUEROS_OUTLET)
        url_iot += BLINKER_F("&duerType=SOCKET");
    #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
        url_iot += BLINKER_F("&duerType=MULTI_SOCKET");
    #elif defined(BLINKER_DUEROS_SENSOR)
        url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    #elif defined(BLINKER_DUEROS_TYPE)
        url_iot += BLINKER_DUEROS_TYPE;
    #endif

    #if defined(BLINKER_MIOT_LIGHT)
        url_iot += BLINKER_F("&miType=light");
    #elif defined(BLINKER_MIOT_OUTLET)
        url_iot += BLINKER_F("&miType=outlet");
    #elif defined(BLINKER_MIOT_MULTI_OUTLET)
        url_iot += BLINKER_F("&miType=multi_outlet");
    #elif defined(BLINKER_MIOT_SENSOR)
        url_iot += BLINKER_F("&miType=sensor");
    #elif defined(BLINKER_MIOT_TYPE)
        url_iot += BLINKER_MIOT_TYPE;
    #endif
    
    url_iot += BLINKER_F("&version=");
    url_iot += BLINKER_OTA_VERSION_CODE;

    url_iot = "https://" + host + url_iot;

    HTTPClient http;

    String payload = "";

    BLINKER_LOG_ALL(BLINKER_F("[HTTP] begin: "), url_iot);

    if (http.begin(*client_s, url_iot)) {  // HTTPS

        // Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled

            BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                payload = http.getString();
                // Serial.println(payload);
            }
        } else {
            BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
            payload = http.getString();
            BLINKER_LOG(payload);
        }

        http.end();
    } else {
        // Serial.printf("[HTTPS] Unable to connect\n");
    }

#elif defined(ESP32)
    HTTPClient http;

    String url_iot = host;
    url_iot += BLINKER_F("/api/v1/user/device/auth?authKey=");
    url_iot += AUTHKEY_PRO;
    // url_iot += _aliType;
    // url_iot += _duerType;

    #if defined(BLINKER_ALIGENIE_LIGHT)
        url_iot += BLINKER_F("&aliType=light");
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        url_iot += BLINKER_F("&aliType=outlet");
    #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
        url_iot += BLINKER_F("&aliType=multi_outlet");
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        url_iot += BLINKER_F("&aliType=sensor");
    #elif defined(BLINKER_ALIGENIE_TYPE)
        url_iot += BLINKER_ALIGENIE_TYPE;
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        url_iot += BLINKER_F("&duerType=LIGHT");
    #elif defined(BLINKER_DUEROS_OUTLET)
        url_iot += BLINKER_F("&duerType=SOCKET");
    #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
        url_iot += BLINKER_F("&duerType=MULTI_SOCKET");
    #elif defined(BLINKER_DUEROS_SENSOR)
        url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    #elif defined(BLINKER_DUEROS_TYPE)
        url_iot += BLINKER_DUEROS_TYPE;
    #endif

    #if defined(BLINKER_MIOT_LIGHT)
        url_iot += BLINKER_F("&miType=light");
    #elif defined(BLINKER_MIOT_OUTLET)
        url_iot += BLINKER_F("&miType=outlet");
    #elif defined(BLINKER_MIOT_MULTI_OUTLET)
        url_iot += BLINKER_F("&miType=multi_outlet");
    #elif defined(BLINKER_MIOT_SENSOR)
        url_iot += BLINKER_F("&miType=sensor");
    #elif defined(BLINKER_MIOT_TYPE)
        url_iot += BLINKER_MIOT_TYPE;
    #endif
    
    url_iot += BLINKER_F("&version=");
    url_iot += BLINKER_OTA_VERSION_CODE;

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

// #if defined(ESP8266)
//     http.begin(url_iot, fingerprint); //HTTP
// #elif defined(ESP32)
    // http.begin(url_iot, ca); TODO
    http.begin(url_iot);
// #endif
    int httpCode = http.GET();

    String payload = "";

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled

        BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            // BLINKER_LOG(payload);
        }
    }
    else {
        BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
        payload = http.getString();
        BLINKER_LOG(payload);
    }

    http.end();
#endif

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(payload);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG(BLINKER_F("Please make sure you have register this device!"));
            // ::delay(60000);

            return false;
        // }
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
    String _authKey = root[BLINKER_CMD_DETAIL][BLINKER_CMD_KEY];

    if (isMQTTinit)
    {
        free(MQTT_HOST_PRO);
        free(MQTT_ID_PRO);
        free(MQTT_NAME_PRO);
        free(MQTT_KEY_PRO);
        free(MQTT_PRODUCTINFO_PRO);
        free(UUID_PRO);
        // free(AUTHKEY_PRO);
        free(MQTT_DEVICEID_PRO);
        free(BLINKER_PUB_TOPIC_PRO);
        free(BLINKER_SUB_TOPIC_PRO);
        free(BLINKER_RRPC_SUB_TOPIC_MQTT);
        free(mqtt_PRO);
        free(iotSub_PRO);

        isMQTTinit = false;
    }

    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        // String _deviceName = _userID.substring(12, 36);
        // MQTT_DEVICEID_PRO = (char*)malloc((_deviceName.length()+1)*sizeof(char));
        // String _deviceName = _userID.substring(12, 36);
        MQTT_DEVICEID_PRO = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_DEVICEID_PRO, _userID.c_str());
        MQTT_ID_PRO = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_PRO, _userID.c_str());
        MQTT_NAME_PRO = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_PRO, _userName.c_str());
        MQTT_KEY_PRO = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_PRO, _key.c_str());
        MQTT_PRODUCTINFO_PRO = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_PRO, _productInfo.c_str());
        MQTT_HOST_PRO = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_PRO, BLINKER_MQTT_ALIYUN_HOST);
        // AUTHKEY_PRO = (char*)malloc((_authKey.length()+1)*sizeof(char));
        // strcpy(AUTHKEY_PRO, _authKey.c_str());
        MQTT_PORT_PRO = BLINKER_MQTT_ALIYUN_PORT;

        BLINKER_LOG_ALL(BLINKER_F("===================="));
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        // String id2name = _userID.subString(10, _userID.length());
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        MQTT_DEVICEID_PRO = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_DEVICEID_PRO, _userID.c_str());
        String IDtest = _productInfo + _userID;
        MQTT_ID_PRO = (char*)malloc((IDtest.length()+1)*sizeof(char));
        strcpy(MQTT_ID_PRO, IDtest.c_str());
        String NAMEtest = IDtest + ";" + _userName;
        MQTT_NAME_PRO = (char*)malloc((NAMEtest.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_PRO, NAMEtest.c_str());
        MQTT_KEY_PRO = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_PRO, _key.c_str());
        MQTT_PRODUCTINFO_PRO = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_PRO, _productInfo.c_str());
        MQTT_HOST_PRO = (char*)malloc((strlen(BLINKER_MQTT_QCLOUD_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_PRO, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT_PRO = BLINKER_MQTT_QCLOUD_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        MQTT_DEVICEID_PRO = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_DEVICEID_PRO, _userID.c_str());
        MQTT_ID_PRO = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_ID_PRO, _userName.c_str());
        MQTT_NAME_PRO = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_PRO, _productInfo.c_str());
        MQTT_KEY_PRO = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_PRO, _key.c_str());
        MQTT_PRODUCTINFO_PRO = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_PRO, _productInfo.c_str());
        MQTT_HOST_PRO = (char*)malloc((strlen(BLINKER_MQTT_ONENET_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_PRO, BLINKER_MQTT_ONENET_HOST);
        MQTT_PORT_PRO = BLINKER_MQTT_ONENET_PORT;
    }
    UUID_PRO = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_PRO, _uuid.c_str());

    char uuid_eeprom[BLINKER_AUUID_SIZE];

    BLINKER_LOG_ALL(BLINKER_F("==========AUTH CHECK=========="));

    if (!isFirst)
    {
        char _authCheck;
        EEPROM.begin(BLINKER_EEP_SIZE);
        EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
        if (strcmp(uuid_eeprom, _uuid.c_str()) != 0) {
            // strcpy(UUID_PRO, _uuid.c_str());

            strcpy(uuid_eeprom, _uuid.c_str());
            EEPROM.put(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
            EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);

            BLINKER_LOG_ALL(BLINKER_F("===================="));
            BLINKER_LOG_ALL(BLINKER_F("uuid_eeprom: "), uuid_eeprom);
            BLINKER_LOG_ALL(BLINKER_F("_uuid: "), _uuid);
            isNew = true;
        }
        EEPROM.get(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
        if (_authCheck != BLINKER_AUTH_CHECK_DATA) {
            EEPROM.put(BLINKER_EEP_ADDR_AUTH_CHECK, BLINKER_AUTH_CHECK_DATA);
            isAuth = true;
        }
        EEPROM.commit();
        EEPROM.end();

        isFirst = true;
    }
    
    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME: "), macDeviceName());
    BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_PRO: "), MQTT_PRODUCTINFO_PRO);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_DEVICEID_PRO: "), MQTT_DEVICEID_PRO);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_PRO: "), MQTT_ID_PRO);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_PRO: "), MQTT_NAME_PRO);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_PRO: "), MQTT_KEY_PRO);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_PRO);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_PRO);
    BLINKER_LOG_ALL(BLINKER_F("UUID_PRO: "), UUID_PRO);
    BLINKER_LOG_ALL(BLINKER_F("AUTHKEY_PRO: "), AUTHKEY_PRO);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        String PUB_TOPIC_STR = BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_PRODUCTINFO_PRO;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_DEVICEID_PRO;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_PRO = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_PRO: "), BLINKER_PUB_TOPIC_PRO);
        
        String SUB_TOPIC_STR = BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_PRO;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_DEVICEID_PRO;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_PRO = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_PRO, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_PRO, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_PRO: "), BLINKER_SUB_TOPIC_PRO);

        SUB_TOPIC_STR = BLINKER_F("/sys/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_PRO;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_DEVICEID_PRO;
        SUB_TOPIC_STR += BLINKER_F("/rrpc/request/");

        BLINKER_RRPC_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_RRPC_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        String PUB_TOPIC_STR = MQTT_PRODUCTINFO_PRO;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += _userID;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_PRO = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_PRO: "), BLINKER_PUB_TOPIC_PRO);
        
        String SUB_TOPIC_STR = MQTT_PRODUCTINFO_PRO;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += _userID;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_PRO = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_PRO, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_PRO, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_PRO: "), BLINKER_SUB_TOPIC_PRO);
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        uint8_t str_len;
        String PUB_TOPIC_STR = MQTT_PRODUCTINFO_PRO;
        PUB_TOPIC_STR += BLINKER_F("/onenet_rule/r");
        // str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC_PRO = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_PRO, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_PRO: "), BLINKER_PUB_TOPIC_PRO);
        
        String SUB_TOPIC_STR = MQTT_PRODUCTINFO_PRO;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += _userID;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_PRO = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_PRO, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_PRO, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_PRO: "), BLINKER_SUB_TOPIC_PRO);
    }

    // BLINKER_LOG_FreeHeap();

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        #if defined(ESP8266)
            // bool mfln = client_mqtt.probeMaxFragmentLength(MQTT_HOST_PRO, MQTT_PORT_PRO, 4096);
            // if (mfln) {
            //     client_mqtt.setBufferSizes(1024, 1024);
            // }
            // client_mqtt.setInsecure();
            mqtt_PRO = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_PRO, MQTT_PORT_PRO, MQTT_ID_PRO, MQTT_NAME_PRO, MQTT_KEY_PRO);
        #elif defined(ESP32)
            mqtt_PRO = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_PRO, MQTT_PORT_PRO, MQTT_ID_PRO, MQTT_NAME_PRO, MQTT_KEY_PRO);
        #endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        #if defined(ESP8266)
            // bool mfln = client_mqtt.probeMaxFragmentLength(MQTT_HOST_PRO, MQTT_PORT_PRO, 4096);
            // if (mfln) {
            //     client_mqtt.setBufferSizes(1024, 1024);
            // }
            // client_mqtt.setInsecure();
            mqtt_PRO = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_PRO, MQTT_PORT_PRO, MQTT_ID_PRO, MQTT_NAME_PRO, MQTT_KEY_PRO);
        #elif defined(ESP32)
            mqtt_PRO = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_PRO, MQTT_PORT_PRO, MQTT_ID_PRO, MQTT_NAME_PRO, MQTT_KEY_PRO);
        #endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        mqtt_PRO = new Adafruit_MQTT_Client(&client, MQTT_HOST_PRO, MQTT_PORT_PRO, MQTT_ID_PRO, MQTT_NAME_PRO, MQTT_KEY_PRO);
    }

    // iotPub = new Adafruit_MQTT_Publish(mqtt_PRO, BLINKER_PUB_TOPIC_PRO);
    // if (!isMQTTinit) 
    iotSub_PRO = new Adafruit_MQTT_Subscribe(mqtt_PRO, BLINKER_SUB_TOPIC_PRO);

    // mqtt_broker = (char*)malloc((_broker.length()+1)*sizeof(char));
    // strcpy(mqtt_broker, _broker.c_str());
    // mqtt_broker = _broker;

    MDNS.end();
    webSocket_PRO.close();
    mDNSInit(MQTT_DEVICEID_PRO);
    this->latestTime = millis();
    // if (!isMQTTinit) 
    mqtt_PRO->subscribe(iotSub_PRO);
    isMQTTinit = true;
    
    #if defined(ESP8266)
        // client_s->stop();
        client_mqtt.setInsecure();
    #endif
    // connect();

    

    return true;
}

void BlinkerPROESP_QRCODE::mDNSInit(String name)
{
    delay(1000);

    BLINKER_LOG(BLINKER_F("WiFi.localIP: "), WiFi.localIP());

#if defined(ESP8266)
    if (!MDNS.begin(name.c_str(), WiFi.localIP())) {
#elif defined(ESP32)
    if (!MDNS.begin(name.c_str())) {
#endif
        while(1) {
            ::delay(100);
        }
    }

    BLINKER_LOG(BLINKER_F("mDNS responder started"));
    
    String _service = STRING_format(BLINKER_MDNS_SERVICE_BLINKER) + STRING_format(_deviceType);
            
    MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", name);

    // if (!isWSinit)
    // {
        webSocket_PRO.begin();
        webSocket_PRO.onEvent(webSocketEvent_PRO);
    // }
    BLINKER_LOG(BLINKER_F("webSocket_PRO server started"));
    BLINKER_LOG(BLINKER_F("ws://"), name, BLINKER_F(".local:"), WS_SERVERPORT);

    // isWSinit = true;

    isApCfg = false;
}

void BlinkerPROESP_QRCODE::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerPROESP_QRCODE::checkAliKA() {
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerPROESP_QRCODE::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerPROESP_QRCODE::checkMIOTKA() {
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerPROESP_QRCODE::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_PRO_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

int BlinkerPROESP_QRCODE::checkCanBprint() {
    if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        return false;
    }
}

int BlinkerPROESP_QRCODE::checkMIOTPrintSpan()
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

int BlinkerPROESP_QRCODE::checkPrintSpan() {
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

int BlinkerPROESP_QRCODE::checkAliPrintSpan()
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

int BlinkerPROESP_QRCODE::checkDuerPrintSpan()
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

int BlinkerPROESP_QRCODE::pubHello()
{
    char stateJsonStr[256] = ("{\"message\":\"Registration successful\"}");
    
    BLINKER_LOG_ALL(BLINKER_F("PUB hello: "), stateJsonStr);
    
    return print(stateJsonStr, false);
}

int BlinkerPROESP_QRCODE::isJson(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("isJson: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(STRING_format(data));
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG(BLINKER_F("Print data is not Json! "), data);
        return false;
    }

    return true;
}


// static IPAddress apIP(192, 168, 4, 1);
// #if defined(ESP8266)
//     static IPAddress netMsk(255, 255, 255, 0);
// #endif


enum bwl_status_t
{
    BWL_CONFIG_CKECK,
    BWL_CONFIG_FAIL,
    BWL_CONFIG_SUCCESS,
    BWL_CONNECTING,
    BWL_CONNECTED,
    BWL_DISCONNECTED,
    BWL_SMARTCONFIG_BEGIN,
    BWL_SMARTCONFIG_DONE,
    BWL_SMARTCONFIG_TIMEOUT,
    BWL_STACONFIG_BEGIN,
    BWL_APCONFIG_BEGIN,
    BWL_APCONFIG_DONE,
    BWL_APCONFIG_TIMEOUT,
    BWL_CONNECTED_CHECK,
    BWL_RESET
};

class BlinkerWlan
{
    public :
        BlinkerWlan()
            : _status(BWL_CONFIG_CKECK)
        {}

        bool checkConfig();
        void loadConfig(char *_ssid, char *_pswd);
        void saveConfig(char *_ssid, char *_pswd);
        void deleteConfig();
        void smartconfigBegin(uint16_t _time = 15000);
        bool smartconfigDone();
        void connect();
        bool connected();
        void disconnect();
        void reset();
        bool run();
        bwl_status_t status() { return _status; }

        void setType(const char* _type) {
            _deviceType = _type;

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG(BLINKER_F("API deviceType: "), _type);
#endif
        }

        void softAPinit();
        void serverClient();
        void parseUrl(String data);
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);

        // uint8_t status() { return _status; }

    private :

    protected :
        char *SSID;
        char *PSWD;
        const char* _deviceType;
        uint32_t connectTime;
        uint16_t timeout;
        bwl_status_t _status;
        uint32_t debugStatusTime;
};

bool BlinkerWlan::checkConfig() {
    BLINKER_LOG_ALL(BLINKER_F("check wlan config"));
    
    char ok[2 + 1];
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    if (String(ok) != String("OK")) {
        
        BLINKER_LOG(BLINKER_F("wlan config check,fail"));

        _status = BWL_CONFIG_FAIL;
        return false;
    }
    else {

        BLINKER_LOG(BLINKER_F("wlan config check,success"));

        _status = BWL_CONFIG_SUCCESS;
        return true;
    }
}

void BlinkerWlan::loadConfig(char *_ssid, char *_pswd) {
    char loadssid[BLINKER_SSID_SIZE];
    char loadpswd[BLINKER_PSWD_SIZE];

    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_SSID, loadssid);
    EEPROM.get(BLINKER_EEP_ADDR_PSWD, loadpswd);
    // char ok[2 + 1];
    // EEPROM.get(EEP_ADDR_WIFI_CFG + BLINKER_SSID_SIZE + BLINKER_PSWD_SIZE, ok);
    EEPROM.commit();
    EEPROM.end();

    strcpy(_ssid, loadssid);
    strcpy(_pswd, loadpswd);

    BLINKER_LOG(BLINKER_F("SSID: "), _ssid, BLINKER_F(" PASWD: "), _pswd);
}

void BlinkerWlan::saveConfig(char *_ssid, char *_pswd) {
    char loadssid[BLINKER_SSID_SIZE];
    char loadpswd[BLINKER_PSWD_SIZE];

    memcpy(loadssid, _ssid, BLINKER_SSID_SIZE);
    memcpy(loadpswd, _pswd, BLINKER_PSWD_SIZE);

    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_SSID, loadssid);
    EEPROM.put(BLINKER_EEP_ADDR_PSWD, loadpswd);
    char ok[2 + 1] = "OK";
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG(BLINKER_F("Save wlan config"));
}

void BlinkerWlan::deleteConfig() {
    char ok[3] = {0};
    EEPROM.begin(BLINKER_EEP_SIZE);
    // for (int i = BLINKER_EEP_ADDR_WLAN_CHECK; i < BLINKER_WLAN_CHECK_SIZE; i++)
    //     EEPROM.write(i, 0);
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG(BLINKER_F("Erase wlan config"));
}

void BlinkerWlan::smartconfigBegin(uint16_t _time) {
    WiFi.mode(WIFI_STA);
    delay(100);
    String softAP_ssid = STRING_format(_deviceType) + "_" + macDeviceName();

#if defined(ESP8266)
    WiFi.hostname(softAP_ssid);
#elif defined(ESP32)
    WiFi.setHostname(softAP_ssid.c_str());
#endif

    WiFi.beginSmartConfig();
    connectTime = millis();
    timeout = _time;
    _status = BWL_SMARTCONFIG_BEGIN;

    BLINKER_LOG(BLINKER_F("Wait for Smartconfig"));
}

bool BlinkerWlan::smartconfigDone() {
    if (WiFi.smartConfigDone())
    {
        // WiFi.setAutoConnect(true);
        // WiFi.setAutoReconnect(true);
        connectTime = millis();

        _status = BWL_SMARTCONFIG_DONE;

        BLINKER_LOG(BLINKER_F("SmartConfig Success"));
#if defined(ESP8266)
        BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
        // WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
        connectWiFi(WiFi.SSID().c_str(), WiFi.psk().c_str());
        // connectWiFi("有没有wifi", "i8888888");
#endif
        return true;
    }
    else {
        return false;
    }
}

void BlinkerWlan::connect() {
    switch (_status) {
        case BWL_CONFIG_SUCCESS :
            // WiFi.setAutoConnect(false);
            // WiFi.setAutoReconnect(true);

            SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
            PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

            loadConfig(SSID, PSWD);
            // WiFi.begin(SSID, PSWD);
            connectWiFi(SSID, PSWD);

            free(SSID);
            free(PSWD);

            _status = BWL_CONNECTING;
            break;
        case BWL_DISCONNECTED :
            if (millis() - connectTime > 30000 && WiFi.status() != WL_CONNECTED) {
                BLINKER_LOG(BLINKER_F("status: "), WiFi.status());

                disconnect();
                // SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
                // PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
                // WiFi.reconnect();

                char _ssid_[BLINKER_SSID_SIZE];
                char _pswd_[BLINKER_PSWD_SIZE];

                loadConfig(_ssid_, _pswd_);
                connectWiFi(_ssid_, _pswd_);

                // WiFi.setAutoConnect(false);
                // WiFi.setAutoReconnect(true);

                // free(SSID);
                // free(PSWD);
                connectTime = millis();
                BLINKER_LOG(BLINKER_F("connecting BWL_DISCONNECTED"));
                
                BLINKER_LOG(BLINKER_F("_ssid_: "), _ssid_, BLINKER_F(" _pswd_: "), _pswd_);
            }
            else if(WiFi.status() == WL_CONNECTED) {
                _status = BWL_CONNECTED;
            }
            break;
    }
}

bool BlinkerWlan::connected() {
    switch (_status) {
        case BWL_SMARTCONFIG_DONE :
            if (WiFi.status() != WL_CONNECTED) {
                if (millis() - connectTime > 15000)
                {
                    BLINKER_LOG(BLINKER_F("smartConfig time out"));
                    
                    WiFi.stopSmartConfig();
                    _status = BWL_SMARTCONFIG_TIMEOUT;
                }
                return false;
            }
            else if (WiFi.status() == WL_CONNECTED) {
                // WiFi.stopSmartConfig();

                IPAddress deviceIP = WiFi.localIP();
                BLINKER_LOG(BLINKER_F("WiFi connected"));
                BLINKER_LOG(BLINKER_F("IP address: "));
                BLINKER_LOG(deviceIP);
                BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
                
                SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
                PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
                memcpy(SSID,"\0",BLINKER_SSID_SIZE);
                memcpy(SSID,WiFi.SSID().c_str(),BLINKER_SSID_SIZE);
                memcpy(PSWD,"\0",BLINKER_PSWD_SIZE);
                memcpy(PSWD,WiFi.psk().c_str(),BLINKER_PSWD_SIZE);
                saveConfig(SSID, PSWD);
                free(SSID);
                free(PSWD);

                // WiFi.setAutoConnect(true);
                // WiFi.setAutoReconnect(true);

                _status = BWL_CONNECTED_CHECK;

                BLINKER_LOG_FreeHeap_ALL();
                
                return true;
            }
            // break;
        case BWL_APCONFIG_DONE :
            if (WiFi.status() != WL_CONNECTED) {
                if (millis() - connectTime > 15000)
                {
                    BLINKER_LOG(BLINKER_F("APConfig time out"));
                    
                    // WiFi.stopSmartConfig();
                    _status = BWL_APCONFIG_TIMEOUT;
                }
                return false;
            }
            else if (WiFi.status() == WL_CONNECTED) {
                IPAddress deviceIP = WiFi.localIP();
                BLINKER_LOG(BLINKER_F("WiFi connected"));
                BLINKER_LOG(BLINKER_F("IP address: "));
                BLINKER_LOG(deviceIP);
                BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
                
                // SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
                // PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
                // memcpy(SSID,"\0",BLINKER_SSID_SIZE);
                // memcpy(SSID,WiFi.SSID().c_str(),BLINKER_SSID_SIZE);
                // memcpy(PSWD,"\0",BLINKER_PSWD_SIZE);
                // memcpy(PSWD,WiFi.psk().c_str(),BLINKER_PSWD_SIZE);
                saveConfig(SSID, PSWD);
                free(SSID);
                free(PSWD);

                // WiFi.setAutoConnect(true);
                // WiFi.setAutoReconnect(true);

                _status = BWL_CONNECTED_CHECK;
                return true;
            }
            break;
        case BWL_CONNECTING :
            if (WiFi.status() == WL_CONNECTED) {
                IPAddress deviceIP = WiFi.localIP();
                BLINKER_LOG(BLINKER_F("WiFi connected"));
                BLINKER_LOG(BLINKER_F("IP address: "));
                BLINKER_LOG(deviceIP);
                BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
                
                _status = BWL_CONNECTED_CHECK;
                return true;
            }
            else if (WiFi.status() != WL_CONNECTED) {
                return false;
            }
        case BWL_CONNECTED_CHECK :
            // if (WiFi.status() != WL_CONNECTED)
            //     _status = BWL_DISCONNECTED;
            if (WiFi.status() == WL_CONNECTED)
            {
                return true;
            }
            else
            {
                _status = BWL_DISCONNECTED;
                return false;
            }
        case BWL_RESET :
            return false;
        default :
            if (WiFi.status() == WL_CONNECTED) {
                IPAddress deviceIP = WiFi.localIP();
                BLINKER_LOG(BLINKER_F("WiFi connected"));
                BLINKER_LOG(BLINKER_F("IP address: "));
                BLINKER_LOG(deviceIP);
                BLINKER_LOG(BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
                
                _status = BWL_CONNECTED_CHECK;
                return true;
            }
            return false;
    }
    return false;
}

void BlinkerWlan::disconnect() {
    WiFi.disconnect();
    delay(100);
    _status = BWL_DISCONNECTED;
    BLINKER_LOG(BLINKER_F("WiFi disconnected"));
}

void BlinkerWlan::reset() {
    disconnect();
    _status = BWL_RESET;
}

void BlinkerWlan::softAPinit() {
    // _server = new WiFiServer(80);

    IPAddress apIP(192, 168, 4, 1);
    #if defined(ESP8266)
        IPAddress netMsk(255, 255, 255, 0);
    #endif

    WiFi.mode(WIFI_AP);

    delay(1000);
    
    String softAP_ssid = STRING_format(_deviceType) + "_" + macDeviceName();
    
#if defined(ESP8266)
    WiFi.hostname(softAP_ssid.c_str());
    WiFi.softAPConfig(apIP, apIP, netMsk);
#elif defined(ESP32)
    WiFi.setHostname(softAP_ssid.c_str());
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
#endif

    WiFi.softAP(softAP_ssid.c_str(), NULL);
    delay(100);

    // _server->begin();
    // BLINKER_LOG(BLINKER_F("AP IP address: "), WiFi.softAPIP());
    // BLINKER_LOG(BLINKER_F("HTTP _server started"));
    // BLINKER_LOG(String("URL: http://" + WiFi.softAPIP()));

    #if defined(ESP8266)
    if (!MDNS.begin(softAP_ssid.c_str(), WiFi.localIP())) {
    #elif defined(ESP32)
    if (!MDNS.begin(softAP_ssid.c_str())) {
    #endif
        while(1) {
            ::delay(100);
        }
    }

    BLINKER_LOG(BLINKER_F("mDNS responder started"));

    MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", macDeviceName());

    webSocket_PRO.begin();
    webSocket_PRO.onEvent(webSocketEvent_PRO);

    _status = BWL_APCONFIG_BEGIN;

    isApCfg = true;

    BLINKER_LOG(BLINKER_F("Wait for APConfig"));
}

void BlinkerWlan::serverClient()
{
    webSocket_PRO.loop();

    #if defined(ESP8266)
        MDNS.update();
    #endif

    if (isAvail_PRO)
    {
        BLINKER_LOG(BLINKER_F("checkAPCFG: "), msgBuf_PRO);

        if (STRING_contains_string(msgBuf_PRO, "ssid") && \
            STRING_contains_string(msgBuf_PRO, "pswd"))
        {
            parseUrl(msgBuf_PRO);
        }
        isAvail_PRO = false;
    }
    // if (!_client)
    // {
    //     _client = _server->available();
    // }
    // else
    // {
    //     // if (_client.status() == CLOSED)
    //     if (!_client.connected())
    //     {
    //         _client.stop();
    //         BLINKER_LOG(BLINKER_F("Connection closed on _client"));
    //     }
    //     else
    //     {
    //         if (_client.available())
    //         {
    //             String data = _client.readStringUntil('\r');

    //             // data = data.substring(4, data.length() - 9);
    //             _client.flush();

    //             // BLINKER_LOG("clientData: ", data);

    //             if (STRING_contains_string(data, "ssid") && STRING_contains_string(data, "pswd")) {

    //                 String msg = BLINKER_F("{\"hello\":\"world\"}");
                    
    //                 String s= BLINKER_F("HTTP/1.1 200 OK\r\nContent-Type: application/json;charset=utf-8\r\n");
    //                 s += BLINKER_F("Content-Length: ");
    //                 s += String(msg.length());
    //                 s += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
    //                 s += msg;
    //                 s += BLINKER_F("\r\n");

    //                 _client.print(s);
                    
    //                 _client.stop();

    //                 parseUrl(data);
    //             }
    //         }
    //     }
    // }
}

void BlinkerWlan::parseUrl(String data)
{
    BLINKER_LOG(BLINKER_F("APCONFIG data: "), data);
    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& wifi_data = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject wifi_data = jsonBuffer.as<JsonObject>();

    // if (!wifi_data.success())
    if (error)
    {
        return;
    }
                    
    String _ssid = wifi_data["ssid"];
    String _pswd = wifi_data["pswd"];

    BLINKER_LOG(BLINKER_F("ssid: "), _ssid);
    BLINKER_LOG(BLINKER_F("pswd: "), _pswd);

    // free(_server);
    MDNS.end();
    webSocket_PRO.close();

    SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
    PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

    strcpy(SSID, _ssid.c_str());
    strcpy(PSWD, _pswd.c_str());
    connectWiFi(_ssid, _pswd);
    connectTime = millis();
    _status = BWL_APCONFIG_DONE;

    BLINKER_LOG(BLINKER_F("APConfig Success"));
}

void BlinkerWlan::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerWlan::connectWiFi(const char* _ssid, const char* _pswd)
{
    uint32_t connectTime = millis();

    BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

    WiFi.mode(WIFI_STA);
    String _hostname = STRING_format(_deviceType) + "_" + macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname);
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    if (_pswd && strlen(_pswd)) {
        WiFi.begin(_ssid, _pswd);
    }
    else {
        WiFi.begin(_ssid);
    }

    // while (WiFi.status() != WL_CONNECTED) {
    //     ::delay(50);

    //     if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
    //         connectTime = millis();
    //         BLINKER_LOG(("WiFi connect timeout, please check ssid and pswd!"));
    //         BLINKER_LOG(("Retring WiFi connect again!"));
    //     }
    // }
    // BLINKER_LOG(("Connected"));

    // IPAddress myip = WiFi.localIP();
    // BLINKER_LOG(("Your IP is: "), myip);

    // mDNSInit();
}

bool BlinkerWlan::run()
{
    // if (millis() - debugStatusTime > 10000) {
    //     debugStatusTime = millis();

    //     BLINKER_LOG_ALL("WLAN status: ", _status);
    // }

    switch (_status) {
        case BWL_CONFIG_CKECK :
            checkConfig();
            break;
        case BWL_CONFIG_FAIL :
            // #if defined(BLINKER_ESP_SMARTCONFIG)
            //     smartconfigBegin();
            // #elif defined(BLINKER_APCONFIG)
            //     softAPinit();
            // #endif
            if (_configType == BLINKER_SMART_CONFIG)
            {
                smartconfigBegin();
            }
            else
            {
                softAPinit();
            }            
            break;
        case BWL_CONFIG_SUCCESS :
            connect();
            break;
        case BWL_CONNECTING :
            return connected();
            break;
        case BWL_CONNECTED :
            return connected();
            break;
        case BWL_DISCONNECTED :
            connect();
            break;
        case BWL_SMARTCONFIG_BEGIN :
            if (_configType == BLINKER_SMART_CONFIG)
            {
                smartconfigDone();
            }
            else
            {
                WiFi.stopSmartConfig();
                _status = BWL_CONFIG_FAIL;
            }
            break;
        case BWL_SMARTCONFIG_DONE :
            return connected();
            break;
        case BWL_SMARTCONFIG_TIMEOUT :
            _status = BWL_CONFIG_FAIL;
            break;
        case BWL_STACONFIG_BEGIN :
            connect();
            break;
        case BWL_APCONFIG_BEGIN :
            serverClient();
            break;
        case BWL_APCONFIG_DONE :
            return connected();
            break;
        case BWL_APCONFIG_TIMEOUT :
            _status = BWL_CONFIG_FAIL;
            break;
        case BWL_CONNECTED_CHECK :
            return connected();
            break;
        case BWL_RESET:
            break;
        default :
            break;
    }
    return false;
}

#endif

#endif
