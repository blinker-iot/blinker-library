#ifndef BLINKER_GATEWAY_H
#define BLINKER_GATEWAY_H

#if (defined(ESP8266) || defined(ESP32))

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif

#include <EEPROM.h>

#include "../modules/painlessMesh/painlessMesh.h"

#include "../modules/WebSockets/WebSocketsServer.h"
#include "../modules/mqtt/Adafruit_MQTT.h"
#include "../modules/mqtt/Adafruit_MQTT_Client.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "Adapters/BlinkerGateway.h"
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
uint16_t    MQTT_PORT_PRO;

painlessMesh  mesh;

#ifndef BLINKER_MESH_SSID
    #define BLINKER_MESH_SSID   "blinkerMesh"
#endif

#ifndef BLINKER_MESH_PSWD
    #define BLINKER_MESH_PSWD   "blinkerMesh"
#endif

#ifndef BLINKER_MESH_PORT
    #define BLINKER_MESH_PORT   5555
#endif

char*       meshBuf;
bool        isFresh_mesh = false;
bool        isAvail_gate = false;
bool        isAvail_ctrl = false;
uint32_t    msgFrom;

BlinkerMeshSub  *_subDevices[BLINKER_MAX_SUB_DEVICE_NUM];
uint8_t         _subCount = 0;
bool            _newSub = false;

int16_t _checkIdAlive(uint32_t nodeId)
{
    for (uint8_t num = 0; num < _subCount; num++)
    {
        if (_subDevices[num]->id() == nodeId)
        {
            return num;
        }
    }

    return -1;
}

int16_t _checkIdAlive(const String & name)
{
    for (uint8_t num = 0; num < _subCount; num++)
    {
        if (_subDevices[num]->isAuth())
        {
            if (_subDevices[num]->deviceName() == name)
            {
                return num;
            }
        }
    }

    return -1;
}

void _receivedCallback(uint32_t from, String &msg)
{
    BLINKER_LOG_ALL("bridge: Received from: ", from, ", msg: ",msg);
    msgFrom = from;

    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, msg);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (error) 
    {
        BLINKER_ERR_LOG_ALL("msg not Json!");
        return;
    }

    if (root.containsKey(BLINKER_CMD_GATE))
    {
        BLINKER_LOG_ALL("gate data");
        
        String _data = root[BLINKER_CMD_GATE];
        meshBuf = (char*)malloc((_data.length()+1)*sizeof(char));
        strcpy(meshBuf, _data.c_str());

        isAvail_gate = true;
    }
    else if (root.containsKey(BLINKER_CMD_CONTROL))
    {
        BLINKER_LOG_ALL("control data");
        
        String _data = root[BLINKER_CMD_CONTROL];
        meshBuf = (char*)malloc((_data.length()+1)*sizeof(char));
        strcpy(meshBuf, _data.c_str());

        isAvail_ctrl = true;
    }
    

    int checkId = _checkIdAlive(from);
    if (checkId == -1)
    {
        _subDevices[_subCount] = new BlinkerMeshSub(from);
        _subCount++;
    }
    else
    {
        _subDevices[checkId]->state(true);
        BLINKER_LOG_ALL("fresh new");
    }
    // _newSub = true;
}

void _newConnectionCallback(uint32_t nodeId)
{
    BLINKER_LOG_ALL("--> startHere: New Connection, nodeId = ", nodeId);
    BLINKER_LOG_ALL("--> startHere: New Connection, ", mesh.subConnectionJson(true));

    int checkId = _checkIdAlive(nodeId);
    if (checkId == -1)
    {
        _subDevices[_subCount] = new BlinkerMeshSub(nodeId);
        _subCount++;
    }
    else
    {
        _subDevices[checkId]->state(true);
        BLINKER_LOG_ALL("fresh new");
    }
    _newSub = true;
}

void _changedConnectionCallback()
{
    BLINKER_LOG_ALL("Changed connections");
}

enum b_config_t {
    BLINKER_SMART_CONFIG,
    BLINKER_AP_CONFIG
};

b_config_t  _configType = BLINKER_SMART_CONFIG;

class BlinkerGateway : public BlinkerStream
{
    public :
        BlinkerGateway();

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
        int subPrint(const String & data, const String & toDevice, const String & subDevice);
        int bPrint(char * name, const String & data);
        int aliPrint(const String & data);
        int subAliPrint(const String & data, const String & subDevice);
        int  duerPrint(const String & data, bool report = false);
        int subDuerPrint(const String & data, const String & subDevice);
        int miPrint(const String & data);
        int subMiPrint(const String & data, const String & subDevice);
        // void aliType(const String & type);
        void begin(const char* _key, const char* _type);
        int autoPrint(unsigned long id);
        // bool autoPrint(char *name, char *type, char *data);
        // bool autoPrint(char *name1, char *type1, char *data1, \
        //             char *name2, char *type2, char *data2);
        char * deviceName();
        char * authKey() { return AUTHKEY_PRO; }
        int init() { return isMQTTinit; }
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
        void setTimezone(float tz) { _timezone = tz; }

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
        String vasDecode(uint16_t num);
        bool meshInit();
        void meshCheck();
        void sendBroadcast(String msg);
        bool sendSingle(uint32_t toId, String msg);
        String gateFormat(const String & msg);
        bool subRegister(uint32_t num);
        time_t time();
        String blinkerServer(uint8_t _type, const String & msg);

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
        bool        _isMeshInit = false;
        uint32_t    _meshCheckTime = 0;
        float       _timezone = 8.0;
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

// WiFiClient              client;
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

            if (!isApCfg) dataFrom_PRO = BLINKER_MSG_FROM_WS;

            ws_num_PRO = num;

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

BlinkerGateway::BlinkerGateway() { isHandle = &isConnect_PRO; }

int BlinkerGateway::connect()
{
    int8_t ret;

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

int BlinkerGateway::connected()
{ 
    if (!isMQTTinit)
    {
        return *isHandle;
    }

    return mqtt_PRO->connected() || *isHandle; 
}

int BlinkerGateway::mConnected()
{
    if (!isMQTTinit) return false;
    else return mqtt_PRO->connected();
}

void BlinkerGateway::disconnect()
{
    if (isMQTTinit) mqtt_PRO->disconnect();

    if (*isHandle) webSocket_PRO.disconnect();
}

void BlinkerGateway::ping()
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

int BlinkerGateway::available()
{
    // meshCheck();

#if defined(ESP8266)
    MDNS.update();
#endif

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

int BlinkerGateway::aligenieAvail()
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

int BlinkerGateway::duerAvail()
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

int BlinkerGateway::miAvail()
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

// bool BlinkerGateway::extraAvailable()
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

void BlinkerGateway::subscribe()
{
    if (!isMQTTinit) return;

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt_PRO->readSubscription(10)))
    {
        if (subscription == iotSub_PRO)
        {
            BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub_PRO->lastread);
            
            // DynamicJsonBuffer jsonBuffer;
            // JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub_PRO->lastread));
            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, String((char *)iotSub_PRO->lastread));
            JsonObject root = jsonBuffer.as<JsonObject>();

            if (root.containsKey("subDevice"))
            {
                String _sub = root["subDevice"];

                int checkId = _checkIdAlive(_sub);
                if (checkId != -1)
                {
                    sendSingle(_subDevices[checkId]->id(), (char *)iotSub_PRO->lastread);
                }

                this->latestTime = millis();
            }
            else
            {
                String _uuid = root["fromDevice"];
                String dataGet = root["data"];
                
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
                                BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found,"
                                            "check is from bridge/share device," 
                                            "data: "), dataGet);

                                _needCheckShare = true;
                            }                        
                        }
                    }
                    
                    // {
                        // dataGet = String((char *)iotSub_PRO->lastread);
                    // root.printTo(dataGet);
                    serializeJson(root, dataGet);
                        
                        // BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
                        //                     check is from bridge/share device, \
                        //                     data: "), dataGet);
                    
                        // // return;

                        // // isBavail = true;

                        // _needCheckShare = true;
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
        }
    }
}

char * BlinkerGateway::lastRead()
{
    if (isFresh_PRO) return msgBuf_PRO;
    return "";
}

void BlinkerGateway::flush()
{
    if (isFresh_PRO)
    {
        free(msgBuf_PRO); isFresh_PRO = false; isAvail_PRO = false;
        isAliAvail = false; isDuerAvail = false; isMIOTAvail = false;//isBavail = false;
    }
}

int BlinkerGateway::print(char * data, bool needCheck)
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

int BlinkerGateway::toServer(char * data)
{
    if (!isJson(STRING_format(data))) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT Publish to server..."));
    BLINKER_LOG_FreeHeap_ALL();

    bool _alive = isAlive;

    if (mqtt_PRO->connected())
    {
        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_MQTT, data))
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

int BlinkerGateway::subPrint(const String & data, const String & toDevice, const String & subDevice)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F("\",\"toDevice\":\"");
    data_add += toDevice;
    data_add += BLINKER_F("\",\"subDevice\":\"");
    data_add += subDevice;
    data_add += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");

    _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

    if (!isJson(data)) return false;
    
    BLINKER_LOG_ALL(BLINKER_F("MQTT Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
    {
        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
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
        return false;
    }
}

int BlinkerGateway::bPrint(char * name, const String & data)
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

int BlinkerGateway::aliPrint(const String & data)
{
    String data_add = BLINKER_F("{\"data\":");
    
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

        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
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

int BlinkerGateway::subAliPrint(const String & data, const String & subDevice)
{
    String data_add = BLINKER_F("{\"data\":");
    
    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F(",\"subDevice\":\"");
    data_add += subDevice;
    data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
    {
        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();
            
            return true;
        }      
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerGateway::duerPrint(const String & data, bool report)
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

        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
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

int BlinkerGateway::subDuerPrint(const String & data, const String & subDevice)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F(",\"subDevice\":\"");
    data_add += subDevice;
    data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT DuerOS Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
    {
        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();
            
            return true;
        }      
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerGateway::miPrint(const String & data)
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

        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
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

int BlinkerGateway::subMiPrint(const String & data, const String & subDevice)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_DEVICEID_PRO;
    data_add += BLINKER_F(",\"subDevice\":\"");
    data_add += subDevice;
    data_add += BLINKER_F("\",\"toDevice\":\"MIOT_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT MIOT Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_PRO->connected())
    {
        if (! mqtt_PRO->publish(BLINKER_PUB_TOPIC_PRO, data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

// void BlinkerGateway::aliType(const String & type)
// {
//     _aliType = (char*)malloc((type.length()+1)*sizeof(char));
//     strcpy(_aliType, type.c_str());
//     BLINKER_LOG_ALL(BLINKER_F("_aliType: "), _aliType);
// }

void BlinkerGateway::begin(const char* _key, const char* _type)
{
    _vipKey = _key;
    _deviceType = _type;
    
    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    mDNSInit();
}

int BlinkerGateway::autoPrint(unsigned long id)
{
    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"trigged\":true,\"autoData\":{");
    payload += BLINKER_F("\"autoId\":");
    payload += STRING_format(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(MQTT_DEVICEID_PRO);
    payload += BLINKER_F("\",\"toDevice\":\"autoManager\"}");
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

// bool BlinkerGateway::autoPrint(char *name, char *type, char *data)
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

// bool BlinkerGateway::autoPrint(char *name1, char *type1, char *data1
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

char * BlinkerGateway::deviceName() { return MQTT_DEVICEID_PRO;/*MQTT_ID_PRO;*/ }

void BlinkerGateway::sharers(const String & data)
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

int BlinkerGateway::authCheck()
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

int BlinkerGateway::connectServer() {
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
            BLINKER_ERR_LOG(("Please make sure you have register this device!"));
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
        free(mqtt_PRO);
        free(iotSub_PRO);

        isMQTTinit = false;
    }

    BLINKER_LOG_ALL(("===================="));

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

        BLINKER_LOG_ALL(("===================="));
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

    BLINKER_LOG_ALL(("==========AUTH CHECK=========="));

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
    // else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
    //     mqtt_PRO = new Adafruit_MQTT_Client(&client, MQTT_HOST_PRO, MQTT_PORT_PRO, MQTT_ID_PRO, MQTT_NAME_PRO, MQTT_KEY_PRO);
    // }

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

void BlinkerGateway::mDNSInit(String name)
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

void BlinkerGateway::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerGateway::checkAliKA() {
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerGateway::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerGateway::checkMIOTKA() {
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerGateway::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_PRO_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

int BlinkerGateway::checkCanBprint() {
    if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        return false;
    }
}

int BlinkerGateway::checkMIOTPrintSpan()
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

int BlinkerGateway::checkPrintSpan() {
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

int BlinkerGateway::checkAliPrintSpan()
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

int BlinkerGateway::checkDuerPrintSpan()
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

int BlinkerGateway::pubHello()
{
    char stateJsonStr[256] = ("{\"message\":\"Registration successful\"}");
    
    BLINKER_LOG_ALL(BLINKER_F("PUB hello: "), stateJsonStr);
    
    return print(stateJsonStr, false);
}

int BlinkerGateway::isJson(const String & data)
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

String BlinkerGateway::vasDecode(uint16_t num)
{
    String vas = "";

    if (num & 0xF)
    {
        if (num & 0x01) vas += "&aliType=light";
        else if (num >> 1 & 0x01) vas += "&aliType=outlet";
        else if (num >> 2 & 0x01) vas += "&aliType=multi_outlet";
        else if (num >> 3 & 0x01) vas += "&aliType=sensor";        
    }

    if (num >> 4 & 0xF)
    {
        if (num >> 4 & 0x01) vas += "&duerType=light";
        else if (num >> 5 & 0x01) vas += "&duerType=outlet";
        else if (num >> 6 & 0x01) vas += "&duerType=multi_outlet";
        else if (num >> 7 & 0x01) vas += "&duerType=sensor";  
    }

    if (num >> 8 & 0xF)
    {
        if (num >> 8 & 0x01) vas += "&miType=light";
        else if (num >> 9 & 0x01) vas += "&miType=outlet";
        else if (num >> 10 & 0x01) vas += "&miType=multi_outlet";
        else if (num >> 11 & 0x01) vas += "&miType=sensor"; 
    }

    BLINKER_LOG_ALL("vas: ", vas);

    return vas;
}

bool BlinkerGateway::meshInit()
{
    if (WiFi.status() != WL_CONNECTED) return false;

    mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
    // Channel set to 6. Make sure to use the same channel for your mesh and for you other
    // network (STATION_SSID)
    mesh.init(BLINKER_MESH_SSID, BLINKER_MESH_PSWD, BLINKER_MESH_PORT, WIFI_AP_STA, 6);
    // mesh.stationManual(STATION_SSID, STATION_PASSWORD, STATION_PORT, station_ip);
    // BLINKER_LOG_ALL("SSID: ", WiFi.SSID(), ", PWSD: ", WiFi.psk());
    mesh.stationManual(WiFi.SSID(), WiFi.psk());
    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
    mesh.setRoot(true);
    // This and all other mesh should ideally now the mesh contains a root
    mesh.setContainsRoot(true);

    mesh.onReceive(&_receivedCallback);
    mesh.onNewConnection(&_newConnectionCallback);
    mesh.onChangedConnections(&_changedConnectionCallback);

    WiFi.reconnect();

    return true;
}

void BlinkerGateway::meshCheck()
{
    if (!_isMeshInit)
    {
        if (meshInit()) _isMeshInit = true;
    }
    else
    {
        if (WiFi.status() != WL_CONNECTED) return;
        mesh.update();

        if (isAvail_gate)
        {
            isAvail_gate = false;
            
            BLINKER_LOG_ALL("new gate data: ", meshBuf);

            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, meshBuf);
            JsonObject root = jsonBuffer.as<JsonObject>();

            if (error) 
            {
                BLINKER_ERR_LOG_ALL("msg not Json!");
                free(meshBuf);
                return;
            }

            if (root.containsKey(BLINKER_CMD_DEVICEINFO))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    bool authState = root[BLINKER_CMD_DEVICEINFO]["auth"];
                    BLINKER_LOG_ALL("auth state: ", authState);
                    
                    _subDevices[checkId]->auth(root[BLINKER_CMD_DEVICEINFO]["name"],
                    root[BLINKER_CMD_DEVICEINFO]["key"],
                    root[BLINKER_CMD_DEVICEINFO]["type"],
                    root[BLINKER_CMD_DEVICEINFO]["vas"].as<uint16_t>());

                    vasDecode(root[BLINKER_CMD_DEVICEINFO]["vas"].as<uint16_t>());


                    if (!authState || !_subDevices[checkId]->isAuth())
                    {
                        // TODO
                        if (subRegister(checkId))
                        {
                            _subDevices[checkId]->freshAuth(true);
                        }
                    }
                }
            }

            free(meshBuf);
        }
        else if (isAvail_ctrl)
        {
            isAvail_ctrl = false;
                
            BLINKER_LOG_ALL("new ctrl data: ", meshBuf);

            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, meshBuf);
            JsonObject root = jsonBuffer.as<JsonObject>();

            if (error) 
            {
                BLINKER_ERR_LOG_ALL("msg not Json!");
                free(meshBuf);
                return;
            }

            if (root.containsKey("user"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    if (_subDevices[checkId]->isAuth())
                    {
                        subPrint(root["user"], root["toDevice"], _subDevices[checkId]->deviceName());
                    }
                }
            }
            else if (root.containsKey("ali"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    if (_subDevices[checkId]->isAuth())
                    {
                        subAliPrint(root["ali"], _subDevices[checkId]->deviceName());
                    }
                }
            }
            else if (root.containsKey("duer"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    if (_subDevices[checkId]->isAuth())
                    {
                        subDuerPrint(root["duer"], _subDevices[checkId]->deviceName());
                    }
                }
            }
            else if (root.containsKey("miot"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    if (_subDevices[checkId]->isAuth())
                    {
                        subMiPrint(root["miot"], _subDevices[checkId]->deviceName());
                    }
                }
            }
            else if (root.containsKey("sms"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"msg\":\"");
                    data += root["sms"].as<String>();

                    if (root.containsKey("cel"))
                    {
                        data += BLINKER_F("\",\"cel\":\"");
                        data += root["cel"].as<String>();
                    }

                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_SMS_NUMBER, data);
                }
            }
            else if (root.containsKey("push"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"msg\":\"");
                    data += root["push"].as<String>();
                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_PUSH_NUMBER, data);
                }
            }
            else if (root.containsKey("wechat"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"msg\":\"");
                    data += root["wechat"].as<String>();

                    if (root.containsKey("title"))
                    {
                        data += BLINKER_F("\",\"title\":\"");
                        data += root["title"].as<String>();
                    }
                    
                    if (root.containsKey("state"))
                    {
                        data += BLINKER_F("\",\"state\":\"");
                        data += root["state"].as<String>();
                    }

                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_WECHAT_NUMBER, data);
                }
            }
            else if (root.containsKey("weather"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/weather/now?");
                    data += BLINKER_F("deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("&location=");
                    data += root["weather"].as<String>();

                    // String dataBack = "{\"ctrl\":{\"weather\":" + \
                    //     blinkerServer(BLINKER_CMD_WEATHER_NUMBER, data) + \
                    //     "}}";
                    String dataBack = blinkerServer(BLINKER_CMD_WEATHER_NUMBER, data);

                    if (dataBack == "null") dataBack = "\"null\"";

                    dataBack = "{\"ctrl\":{\"weather\":" + dataBack + "}}";

                    sendSingle(_subDevices[checkId]->id(), dataBack);
                }
            }
            else if (root.containsKey("aqi"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/weather/aqi?");
                    data += BLINKER_F("deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("&location=");
                    data += root["aqi"].as<String>();

                    // String dataBack = "{\"ctrl\":{\"aqi\":" + \
                    //     blinkerServer(BLINKER_CMD_AQI_NUMBER, data) + \
                    //     "}}";
                    String dataBack = blinkerServer(BLINKER_CMD_AQI_NUMBER, data);

                    if (dataBack == "null") dataBack = "\"null\"";

                    dataBack = "{\"meshData\":{\"aqi\":" + dataBack + "}}";
                    
                    sendSingle(_subDevices[checkId]->id(), dataBack);
                }
            }
            else if (root.containsKey("freshSharers"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/share/device?");
                    data += BLINKER_F("deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();
                    
                    String dataBack = blinkerServer(BLINKER_CMD_FRESH_SHARERS_NUMBER, data);

                    if (dataBack == "null") dataBack = "\"null\"";

                    dataBack = "{\"meshData\":{\"freshSharers\":" + dataBack + "}}";
                    
                    sendSingle(_subDevices[checkId]->id(), dataBack);
                }
            }
            else if (root.containsKey("configUpdate"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String _msg = root["configUpdate"].as<String>();

                    if (_msg.length() <= 256) 
                    {
                        String data = BLINKER_F("{\"deviceName\":\"");
                        data += _subDevices[checkId]->deviceName();
                        data += BLINKER_F("\",\"key\":\"");
                        data += _subDevices[checkId]->authKey();
                        data += BLINKER_F("\",\"config\":\"");
                        data += _msg;
                        data += BLINKER_F("\"}");

                        blinkerServer(BLINKER_CMD_CONFIG_UPDATE_NUMBER, data);
                    }
                }
            }
            else if (root.containsKey("configGet"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/pull_userconfig?deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();

                    String dataBack = blinkerServer(BLINKER_CMD_CONFIG_GET_NUMBER, data);

                    if (dataBack == "null") dataBack = "\"null\"";

                    dataBack = "{\"meshData\":{\"configGet\":" + dataBack + "}}";
                    
                    sendSingle(_subDevices[checkId]->id(), dataBack);
                }
            }
            else if (root.containsKey("configDel"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/delete_userconfig?deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();

                    blinkerServer(BLINKER_CMD_CONFIG_DELETE_NUMBER, data);
                }
            }
            else if (root.containsKey("dataUpdate"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"data\":");
                    data += root["dataUpdate"].as<String>();
                    data += BLINKER_F("}");

                    blinkerServer(BLINKER_CMD_DATA_STORAGE_NUMBER, data);
                }
            }
            else if (root.containsKey("dataGet"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/pull_cloudStorage?deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();

                    String _type = root["dataGet"].as<String>();
                    if (_type != "")
                    {
                        data += BLINKER_F("&dataType=");
                        data += _type;
                    }
                    if (root.containsKey("date"))
                    {
                        data += BLINKER_F("&date=");
                        data += root["date"].as<String>();
                    }

                    String dataBack = blinkerServer(BLINKER_CMD_DATA_GET_NUMBER, data);

                    if (dataBack == "null") dataBack = "\"null\"";

                    dataBack = "{\"meshData\":{\"dataGet\":" + dataBack + "}}";
                    
                    sendSingle(_subDevices[checkId]->id(), dataBack);
                }
            }
            else if (root.containsKey("dataDel"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/delete_cloudStorage?deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();

                    String _type = root["dataDel"].as<String>();
                    if (_type != "")
                    {
                        data += BLINKER_F("&dataType=");
                        data += _type;
                    }

                    blinkerServer(BLINKER_CMD_DATA_DELETE_NUMBER, data);
                }
            }
            else if (root.containsKey("eKey"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"eKey\":\"");
                    data += root["eKey"].as<String>();
                    data += BLINKER_F("\",\"date\":\"");
                    data += root["date"].as<String>();
                    data += BLINKER_F("\",\"value\":\"");
                    data += root["value"].as<String>();
                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_EVENT_DATA_NUMBER, data);
                }
            }
            else if (root.containsKey("gpsUpdate"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"data\":[");
                    data += root["gpsUpdate"][0].as<String>();
                    data += BLINKER_F(",");
                    data += root["gpsUpdate"][1].as<String>();
                    data += BLINKER_F(",");
                    data += root["gpsUpdate"][2].as<String>();
                    data += BLINKER_F("]}");

                    blinkerServer(BLINKER_CMD_GPS_DATA_NUMBER, data);
                }
            }
            else if (root.containsKey("autoPull"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/auto/pull?deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();

                    String dataBack = blinkerServer(BLINKER_CMD_AUTO_PULL_NUMBER, data);

                    if (dataBack == "null") dataBack = "\"null\"";

                    dataBack = "{\"meshData\":{\"autoPull\":" + dataBack + "}}";
                    
                    sendSingle(_subDevices[checkId]->id(), dataBack);
                }
            }
            else if (root.containsKey("deviceHeartbeat"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("/heartbeat?");
                    data += BLINKER_F("deviceName=");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("&key=");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("&heartbeat=");
                    data += root["deviceHeartbeat"].as<String>();

                    blinkerServer(BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER, data);
                }
            }
            else if (root.containsKey("eventWarn"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"msgType\":\"warning");
                    data += BLINKER_F("\",\"msg\":\"");
                    data += root["eventWarn"].as<String>();
                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_EVENT_WARNING_NUMBER, data);
                }
            }
            else if (root.containsKey("eventError"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"msgType\":\"error");
                    data += BLINKER_F("\",\"msg\":\"");
                    data += root["eventError"].as<String>();
                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_EVENT_ERROR_NUMBER, data);
                }
            }
            else if (root.containsKey("eventMsg"))
            {
                int checkId = _checkIdAlive(msgFrom);
                if (checkId != -1)
                {
                    String data = BLINKER_F("{\"deviceName\":\"");
                    data += _subDevices[checkId]->deviceName();
                    data += BLINKER_F("\",\"key\":\"");
                    data += _subDevices[checkId]->authKey();
                    data += BLINKER_F("\",\"msgType\":\"error");
                    data += BLINKER_F("\",\"msg\":\"");
                    data += root["eventMsg"].as<String>();
                    data += BLINKER_F("\"}");

                    blinkerServer(BLINKER_CMD_EVENT_MSG_NUMBER, data);
                }
            }

            free(meshBuf);
        }

        if (_newSub)
        {
            _newSub = false;

            sendBroadcast(gateFormat(STRING_format(BLINKER_CMD_WHOIS)));

            _meshCheckTime = millis();
        }
        else if (millis() - _meshCheckTime >= BLINKER_MESH_CHECK_FREQ)
        {
            sendBroadcast(gateFormat(STRING_format(BLINKER_CMD_WHOIS)));

            _meshCheckTime = millis();
        }
    }
}

void BlinkerGateway::sendBroadcast(String msg)
{
    mesh.sendBroadcast(msg);
}

bool BlinkerGateway::sendSingle(uint32_t toId, String msg)
{
    BLINKER_LOG_ALL("to id: ", toId, ", msg: ", msg);
    if (mesh.isConnected(toId))
    {
        return mesh.sendSingle(toId, msg);
    }
    else
    {
        BLINKER_ERR_LOG("device disconnected");
        return false;
    }
}

String BlinkerGateway::gateFormat(const String & msg)
{
    return "{\"" + STRING_format(BLINKER_CMD_GATE) + "\":" + msg + \
            ",\"tim\":" + STRING_format(time()) + \
            ",\"tz\":" + STRING_format(_timezone) + "}";
}

bool BlinkerGateway::subRegister(uint32_t num)
{
    const int httpsPort = 443;
    #if defined(ESP8266)
        String host = BLINKER_F(BLINKER_SERVER_HOST);
        client_mqtt.stop();
    #elif defined(ESP32)
        String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    #endif

    #if defined(ESP8266)
        String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");
        std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

        // client_s->setFingerprint(fingerprint);
        client_s->setInsecure();

        String url_iot = BLINKER_F("/api/v1/user/device/auth/get?deviceType=");
        url_iot += _subDevices[num]->type();
        url_iot += BLINKER_F("&typeKey=");
        url_iot += _subDevices[num]->key();
        url_iot += BLINKER_F("&deviceName=");
        url_iot += _subDevices[num]->name();

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
        HTTPClient http;

        String url_iot = host;
        url_iot += BLINKER_F("/api/v1/user/device/auth/get?deviceType=");
        url_iot += _subDevices[num]->type();
        url_iot += BLINKER_F("&typeKey=");
        url_iot += _subDevices[num]->key();
        url_iot += BLINKER_F("&deviceName=");
        url_iot += _subDevices[num]->name();

        BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

        http.begin(url_iot);

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

    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
        !STRING_contains_string(payload, BLINKER_CMD_AUTHKEY)) {
        BLINKER_ERR_LOG(BLINKER_F("Maybe you have put in the wrong AuthKey!"));
        BLINKER_ERR_LOG(BLINKER_F("Or maybe your request is too frequently!"));
        BLINKER_ERR_LOG(BLINKER_F("Or maybe your network is disconnected!"));

        return false;
    }

    String _getAuthKey = root[BLINKER_CMD_DETAIL][BLINKER_CMD_AUTHKEY];

    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("_getAuthKey: "), _getAuthKey);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    #if defined(ESP8266)
        // std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

        // client_s->setInsecure();

        url_iot = BLINKER_F("/api/v1/user/device/auth?authKey=");
        url_iot += _getAuthKey;

        url_iot = "https://" + host + url_iot;

        // HTTPClient http;

        payload = "";

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
        // HTTPClient http;

        url_iot = host;
        url_iot += BLINKER_F("/api/v1/user/device/auth?authKey=");
        url_iot += _getAuthKey;

        BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);
        
        http.begin(url_iot);
        
        httpCode = http.GET();

        payload = "";

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

    DynamicJsonDocument _jsonBuffer(1024);
    DeserializationError _error = deserializeJson(_jsonBuffer, payload);
    JsonObject _root = _jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || _error ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG(("Please make sure you have register this device!"));
            // ::delay(60000);

            return false;
        // }
    }

    _subDevices[num]->authData(_getAuthKey, _root["detail"]["deviceName"].as<String>());
    // _root["detail"]["authKey"] = _getAuthKey;
    JsonObject _detail = _root["detail"].as<JsonObject>();
    _detail["authKey"] = _getAuthKey;
    String _data;
    serializeJson(_detail, _data);
    sendSingle(_subDevices[num]->id(), "{\"gate\":{\"auth\":" + _data + "}}");

    return true;
}

time_t BlinkerGateway::time()
{
    time_t now_ntp = ::time(nullptr);
    BLINKER_LOG_ALL("now_ntp: ", now_ntp);
    return now_ntp;
}

String BlinkerGateway::blinkerServer(uint8_t _type, const String & msg)
{
    switch (_type)
    {
        case BLINKER_CMD_SMS_NUMBER :
            break;
        case BLINKER_CMD_PUSH_NUMBER :
            break;
        case BLINKER_CMD_WECHAT_NUMBER :
            break;
        case BLINKER_CMD_WEATHER_NUMBER :
            break;
        case BLINKER_CMD_AQI_NUMBER :
            break;
        case BLINKER_CMD_BRIDGE_NUMBER :
            break;
        case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
            break;
        case BLINKER_CMD_CONFIG_GET_NUMBER :
            break;
        case BLINKER_CMD_CONFIG_DELETE_NUMBER :
            break;
        case BLINKER_CMD_DATA_STORAGE_NUMBER :
            break;
        case BLINKER_CMD_DATA_GET_NUMBER :
            break;
        case BLINKER_CMD_DATA_DELETE_NUMBER :
            break;
        case BLINKER_CMD_AUTO_PULL_NUMBER :
            break;
        case BLINKER_CMD_OTA_NUMBER :
            break;
        case BLINKER_CMD_OTA_STATUS_NUMBER :
            break;
        case BLINKER_CMD_FRESH_SHARERS_NUMBER :
            break;
        case BLINKER_CMD_LOWPOWER_FREQ_GET_NUM :
            break;
        case BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER :
            break;
        case BLINKER_CMD_LOWPOWER_DATA_GET_NUM :
            break;
        case BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER :
            break;
        case BLINKER_CMD_EVENT_DATA_NUMBER :
            break;
        case BLINKER_CMD_GPS_DATA_NUMBER :
            break;
        case BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER :
            break;
        case BLINKER_CMD_EVENT_WARNING_NUMBER :
            break;
        case BLINKER_CMD_EVENT_ERROR_NUMBER :
            break;
        case BLINKER_CMD_EVENT_MSG_NUMBER :
            break;
        default :
            return BLINKER_CMD_FALSE;
    }

    BLINKER_LOG_ALL(BLINKER_F("message: "), msg);

    #ifndef BLINKER_LAN_DEBUG
        const int httpsPort = 443;
    #elif defined(BLINKER_LAN_DEBUG)
        const int httpsPort = 9090;
    #endif

    #ifndef BLINKER_LAN_DEBUG
        String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    #elif defined(BLINKER_LAN_DEBUG)
        String host = BLINKER_F("http://192.168.1.121:9090");
    #endif

    #if defined(ESP8266)
        extern BearSSL::WiFiClientSecure client_mqtt;
        client_mqtt.stop();

        std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

        // client_s->setFingerprint(fingerprint);
        client_s->setInsecure();
    #endif

    HTTPClient http;

    String url_iot;

    int httpCode;
    
    String conType = BLINKER_F("Content-Type");
    String application = BLINKER_F("application/json;charset=utf-8");

    BLINKER_LOG_ALL(BLINKER_F("blinker server begin"));
    BLINKER_LOG_FreeHeap_ALL();

    switch (_type) {
        case BLINKER_CMD_SMS_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/sms");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_PUSH_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/push");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
            // return BLINKER_CMD_FALSE;
        case BLINKER_CMD_WECHAT_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/wxMsg/");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
            // return BLINKER_CMD_FALSE;
        case BLINKER_CMD_WEATHER_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_AQI_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_BRIDGE_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/userconfig");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_CONFIG_GET_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_CONFIG_DELETE_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_DATA_STORAGE_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/cloudStorage/");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_DATA_GET_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_DATA_DELETE_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_AUTO_PULL_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_OTA_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_OTA_STATUS_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/ota/upgrade_status");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_FRESH_SHARERS_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_LOWPOWER_FREQ_GET_NUM :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_LOWPOWER_DATA_GET_NUM :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/lowpower/data");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_EVENT_DATA_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/event");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_GPS_DATA_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/gps");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_EVENT_WARNING_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/event");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_EVENT_ERROR_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/event");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_EVENT_MSG_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/event");

            #if defined(ESP8266)
                http.begin(*client_s, url_iot);
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        default :
            return BLINKER_CMD_FALSE;
    }

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);
    BLINKER_LOG_ALL(BLINKER_F("HTTPS payload: "), msg);

    if (httpCode > 0)
    {
        BLINKER_LOG_ALL(BLINKER_F("[HTTP] status... code: "), httpCode);

        String payload;
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();

            BLINKER_LOG_ALL(payload);
            
            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, payload);
            JsonObject data_rp = jsonBuffer.as<JsonObject>();
            
            if (!error)
            {
                uint16_t msg_code = data_rp[BLINKER_CMD_MESSAGE];
                if (msg_code != 1000)
                {
                    String _detail = data_rp[BLINKER_CMD_DETAIL];
                    BLINKER_ERR_LOG(_detail);
                }
                else
                {
                    if (_type == BLINKER_CMD_BRIDGE_NUMBER)
                        payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME].as<String>();
                    else if (_type == BLINKER_CMD_OTA_NUMBER || _type == BLINKER_CMD_FRESH_SHARERS_NUMBER)
                        payload = data_rp[BLINKER_CMD_DETAIL].as<String>();
                    else if (_type == BLINKER_CMD_LOWPOWER_FREQ_GET_NUM)
                        payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_FREQ].as<String>();
                    else
                        payload = data_rp[BLINKER_CMD_DETAIL][BLINKER_CMD_DATA].as<String>();
                }
            }

            BLINKER_LOG_ALL(BLINKER_F("payload: "), payload);

            switch (_type) {
                case BLINKER_CMD_SMS_NUMBER :
                    break;
                case BLINKER_CMD_PUSH_NUMBER :
                    break;
                case BLINKER_CMD_WECHAT_NUMBER :
                    break;
                case BLINKER_CMD_WEATHER_NUMBER :
                    break;
                case BLINKER_CMD_AQI_NUMBER :
                    break;
                case BLINKER_CMD_BRIDGE_NUMBER :
                    break;
                case BLINKER_CMD_CONFIG_UPDATE_NUMBER :
                    break;
                case BLINKER_CMD_CONFIG_GET_NUMBER :
                    break;
                case BLINKER_CMD_CONFIG_DELETE_NUMBER :
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    break;
                case BLINKER_CMD_DATA_GET_NUMBER :
                    break;
                case BLINKER_CMD_DATA_DELETE_NUMBER :
                    break;
                case BLINKER_CMD_AUTO_PULL_NUMBER :
                    break;
                case BLINKER_CMD_OTA_NUMBER :
                    break;
                case BLINKER_CMD_OTA_STATUS_NUMBER :
                    break;
                case BLINKER_CMD_FRESH_SHARERS_NUMBER :
                    break;
                case BLINKER_CMD_LOWPOWER_FREQ_GET_NUM :
                    break;
                case BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER :
                    break;
                case BLINKER_CMD_LOWPOWER_DATA_GET_NUM :
                    break;
                case BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER :
                    break;
                case BLINKER_CMD_EVENT_DATA_NUMBER :
                    break;
                case BLINKER_CMD_GPS_DATA_NUMBER :
                    break;
                case BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER :
                    break;
                case BLINKER_CMD_EVENT_WARNING_NUMBER :
                    break;
                case BLINKER_CMD_EVENT_ERROR_NUMBER :
                    break;
                case BLINKER_CMD_EVENT_MSG_NUMBER :
                    break;
                default :
                    return BLINKER_CMD_FALSE;
            }
        }

        http.end();

        return payload;
    }
    else {
        BLINKER_LOG_ALL(BLINKER_F("[HTTP] ... failed, error: "), http.errorToString(httpCode).c_str());
        String payload = http.getString();
        BLINKER_LOG_ALL(payload);

        http.end();
        return BLINKER_CMD_FALSE;
    }
}

static IPAddress apIP(192, 168, 4, 1);
#if defined(ESP8266)
    static IPAddress netMsk(255, 255, 255, 0);
#endif


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

    WiFi.mode(WIFI_AP);
    String softAP_ssid = STRING_format(_deviceType) + "_" + macDeviceName();
    
#if defined(ESP8266)
    WiFi.hostname(softAP_ssid);
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
            smartconfigDone();
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
