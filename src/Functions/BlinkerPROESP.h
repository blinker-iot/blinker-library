#ifndef BLINKER_WIFI_ESP_H
#define BLINKER_WIFI_ESP_H

#if (defined(ESP8266) || defined(ESP32))



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
    BLINKER_SMART_CONFIG,
    BLINKER_AP_CONFIG
};

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

b_config_t  _configType = BLINKER_SMART_CONFIG;

class BlinkerPROESP
{
    public :
        BlinkerPROESP();

        void setSmartConfig()   { _configType = BLINKER_SMART_CONFIG; BLINKER_LOG_ALL(BLINKER_F("SmartConfig")); }
        void setApConfig()      { _configType = BLINKER_AP_CONFIG; BLINKER_LOG_ALL(BLINKER_F("ApConfig")); }
        bool checkSmartConfig() { return _configType == BLINKER_SMART_CONFIG; }

        bool init();
        void reset();
        void begin(const char* _key, const char* _type);
        bool connect();
        bool connected();
        bool mConnected();
        void disconnect();
        void ping();
        bool available();
        void subscribe();
        char * lastRead();
        void flush();
        bool print(char * data, bool needCheck = true);
        bool autoPrint(unsigned long id);
        void freshSharers();
        bool authCheck();
        bool deviceRegister();
        void mDNSInit(const String & name = macDeviceName());
        bool checkInit()        { return isMQTTinit; }

        void setRegister()      { _getRegister = true; _register_fresh = millis(); }

        char * deviceName() { return MQTT_DEVICEID_PRO; }
        char * authKey() { return AUTHKEY_PRO; }
        char * token() { if (!isMQTTinit) return ""; else return MQTT_KEY_PRO; }

        String httpServer(uint8_t _type, const String & msg, bool state = false)
        { return httpToServer(_type, msg, state); }

        void freshAlive() { kaTime = millis(); isAlive = true; }

    private :
        bool isMQTTinit = false;
        bool isWSinit = false;

        void checkKA();
        bool checkAliKA();
        bool checkDuerKA();
        bool checkMIOTKA();
        bool checkCanPrint();
        bool checkCanBprint();
        bool checkPrintSpan();
        bool checkAliPrintSpan();
        bool checkDuerPrintSpan();
        bool checkMIOTPrintSpan();
        bool pubHello();
        bool isJson(const String & data);
        void parseData(const char* data);

        bool checkWlanConfig();
        void loadWlanConfig(char *_ssid, char *_pswd);
        void saveWlanConfig(char *_ssid, char *_pswd);
        void deleteWlanConfig();
        void smartconfigBegin(uint16_t _time = 15000);
        bool smartconfigDone();
        void wlanConnect();
        bool wlanConnected();
        void wlanDisconnect();
        void wlanReset();
        bool wlanRun();
        bwl_status_t status() { return _status; }

        void softAPinit();
        void serverClient();
        void parseUrl(String data);
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);

    protected :
        char        _messageId[20];
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

        uint8_t     reconnect_time = 0;
        uint32_t    _reRegister_time = 0;
        uint8_t     _reRegister_times = 0;

        bool        _isAuthKey = false;

        char        message_id[24];
        bool        is_rrpc = false;

        char *      SSID;
        char *      PSWD;
        // const char* _deviceType;
        uint32_t    connectTime;
        uint16_t    timeout;
        uint32_t    debugStatusTime;
        bwl_status_t _status;

        bool        _isConnBegin = false;
        time_t      _initTime;
        bool        _isRegistered;
        bool        _getRegister;
        time_t      _register_fresh;
        uint8_t     _register_times;
        bool        _isNew = false;

        void checkRegister();
};

// #if defined(ESP8266)
//     #ifndef BLINKER_WITHOUT_SSL
//         BearSSL::WiFiClientSecure   client_mqtt;
//     #else
//         WiFiClient               client_mqtt;
//     #endif
//     // WiFiClientSecure            client_mqtt;
// #elif defined(ESP32)
//     WiFiClientSecure            client_s;
// #endif

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

BlinkerPROESP   PROESP;

BlinkerPROESP::BlinkerPROESP() { isHandle = &isConnect_PRO; }

bool BlinkerPROESP::init()
{
    if (!isMQTTinit)
    {
        if (!wlanRun())
        {
            yield();
            return false;
        }
        else
        {
            if (!_isConnBegin)
            {
                _isConnBegin = true;
                _initTime = millis();

                mDNSInit();

                BLINKER_LOG_ALL(BLINKER_F("conn begin, fresh _initTime: "), _initTime);

                if (authCheck())
                {
                    BLINKER_LOG_ALL(BLINKER_F("is auth, conn deviceRegister"));

                    _isRegistered = deviceRegister();
                    _getRegister = true;

                    if (!_isRegistered)
                    {
                        _register_fresh = millis();
                    }
                    else
                    {
                    }
                }
                else
                {
                    BLINKER_LOG_ALL(BLINKER_F("not auth, conn deviceRegister"));
                }
            }
        }

        checkRegister();

        if (_getRegister)
        {
            if (_register_times < BLINKER_SERVER_CONNECT_LIMIT)
            {
                if (!_isRegistered && \
                    ((millis() - _register_fresh) > 5000 || \
                    _register_fresh == 0))
                {
                    BLINKER_LOG_ALL(BLINKER_F("conn deviceRegister, _register_times: "), _register_times);

                    _isRegistered = deviceRegister();

                    if (!_isRegistered)
                    {
                        _register_fresh = millis();
                        _register_times++;
                    }
                    else
                    {
                        _isRegistered = true;
                        _register_times = 0;
                    }
                }
            }
            else
            {
                if ((millis() - _register_fresh) >= (60000 * 5)) _register_times = 0;
            }
        }

        if ((millis() - _initTime) >= BLINKER_CHECK_AUTH_TIME && \
            (!_getRegister || _isNew))
        {
            reset();
        }

        return _isRegistered;
    }
    else
    {
        return true;
    }    
}

void BlinkerPROESP::reset()
{
    BLINKER_LOG(BLINKER_F("Blinker reset..."));
    char _authCheck = 0x00;
    char _uuid[BLINKER_AUUID_SIZE] = {0};
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
    EEPROM.put(BLINKER_EEP_ADDR_AUUID, _uuid);
    EEPROM.commit();
    EEPROM.end();
    wlanReset();
    ESP.restart();
}

void BlinkerPROESP::begin(const char* _key, const char* _type)
{
    _vipKey = _key;
    _deviceType = _type;

    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);
}

bool BlinkerPROESP::connect()
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
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.setInsecure();
            ::delay(10);
        #endif
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

bool BlinkerPROESP::connected()
{ 
    if (!isMQTTinit)
    {
        return *isHandle;
    }

    return mqtt_PRO->connected() || *isHandle; 
}

bool BlinkerPROESP::mConnected()
{
    if (!isMQTTinit) return false;
    else return mqtt_PRO->connected();
}

void BlinkerPROESP::disconnect()
{
    if (isMQTTinit) mqtt_PRO->disconnect();

    if (*isHandle) webSocket_PRO.disconnect();
}

void BlinkerPROESP::ping()
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

bool BlinkerPROESP::available()
{
#if defined(ESP8266)
    MDNS.update();
#endif

    delay(1);

    webSocket_PRO.loop();

    if (isMQTTinit) {
        checkKA();

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

void BlinkerPROESP::subscribe()
{
    if (!isMQTTinit) return;

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt_PRO->readSubscription(10)))
    {
        if (subscription == iotSub_PRO)
        {
            BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub_PRO->lastread);
            
            parseData((char *)iotSub_PRO->lastread);
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

            is_rrpc = true;
        }
    }
}

char * BlinkerPROESP::lastRead()
{
    if (isFresh_PRO) return msgBuf_PRO;
    return "";
}

void BlinkerPROESP::flush()
{
    if (isFresh_PRO)
    {
        free(msgBuf_PRO); isFresh_PRO = false; isAvail_PRO = false;
        isAliAvail = false; isDuerAvail = false; isMIOTAvail = false;//isBavail = false;
    }
}

bool BlinkerPROESP::print(char * data, bool needCheck)
{
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

bool BlinkerPROESP::autoPrint(unsigned long id)
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

void BlinkerPROESP::freshSharers()
{
    String  data = BLINKER_F("/share/device?");
            data += BLINKER_F("deviceName=");
            data += MQTT_DEVICEID_PRO;
            data += BLINKER_F("&key=");
            data += AUTHKEY_PRO;

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

bool BlinkerPROESP::authCheck()
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

bool BlinkerPROESP::deviceRegister()
{
    if (!_isAuthKey)
    {
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
        #elif defined(BLINKER_ALIGENIE_FAN)
            url_iot += BLINKER_F("&aliType=fan");
        #elif defined(BLINKER_ALIGENIE_AIRCONDITION)
            url_iot += BLINKER_F("&aliType=aircondition");
        #endif

        #if defined(BLINKER_DUEROS_LIGHT)
            url_iot += BLINKER_F("&duerType=LIGHT");
        #elif defined(BLINKER_DUEROS_OUTLET)
            url_iot += BLINKER_F("&duerType=SOCKET");
        #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
            url_iot += BLINKER_F("&duerType=MULTI_SOCKET");
        #elif defined(BLINKER_DUEROS_SENSOR)
            url_iot += BLINKER_F("&duerType=AIR_MONITOR");
        #elif defined(BLINKER_DUEROS_FAN)
            url_iot += BLINKER_F("&duerType=FAN");
        #elif defined(BLINKER_DUEROS_AIRCONDITION)
            url_iot += BLINKER_F("&duerType=AIR_CONDITION");
        #endif

        #if defined(BLINKER_MIOT_LIGHT)
            url_iot += BLINKER_F("&miType=light");
        #elif defined(BLINKER_MIOT_OUTLET)
            url_iot += BLINKER_F("&miType=outlet");
        #elif defined(BLINKER_MIOT_MULTI_OUTLET)
            url_iot += BLINKER_F("&miType=multi_outlet");
        #elif defined(BLINKER_MIOT_SENSOR)
            url_iot += BLINKER_F("&miType=sensor");
        #elif defined(BLINKER_MIOT_FAN)
            url_iot += BLINKER_F("&miType=fan");
        #elif defined(BLINKER_MIOT_AIRCONDITION)
            url_iot += BLINKER_F("&miType=aircondition");
        #endif

        url_iot += BLINKER_F("&version=");
        url_iot += BLINKER_OTA_VERSION_CODE;
        #ifndef BLINKER_WITHOUT_SSL
        url_iot += BLINKER_F("&protocol=mqtts");
        #else
        url_iot += BLINKER_F("&protocol=mqtt");
        #endif

        String payload = httpToServer(BLINKER_CMD_PRO_AUTH_GET_NUMBER, url_iot);

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

        String _getAuthKey = root[BLINKER_CMD_AUTHKEY];

        AUTHKEY_PRO = (char*)malloc((_getAuthKey.length()+1)*sizeof(char));
        strcpy(AUTHKEY_PRO, _getAuthKey.c_str());

        BLINKER_LOG_ALL(BLINKER_F("===================="));
        BLINKER_LOG_ALL(BLINKER_F("AUTHKEY_PRO: "), AUTHKEY_PRO);
        BLINKER_LOG_ALL(BLINKER_F("===================="));

        _isAuthKey = true;
    }

    String url_iot = BLINKER_F("/api/v1/user/device/auth?authKey=");
    url_iot += AUTHKEY_PRO;

    #if defined(BLINKER_ALIGENIE_LIGHT)
        url_iot += BLINKER_F("&aliType=light");
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        url_iot += BLINKER_F("&aliType=outlet");
    #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
        url_iot += BLINKER_F("&aliType=multi_outlet");
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        url_iot += BLINKER_F("&aliType=sensor");
    #elif defined(BLINKER_ALIGENIE_FAN)
        url_iot += BLINKER_F("&aliType=fan");
    #elif defined(BLINKER_ALIGENIE_AIRCONDITION)
        url_iot += BLINKER_F("&aliType=aircondition");
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        url_iot += BLINKER_F("&duerType=LIGHT");
    #elif defined(BLINKER_DUEROS_OUTLET)
        url_iot += BLINKER_F("&duerType=SOCKET");
    #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
        url_iot += BLINKER_F("&duerType=MULTI_SOCKET");
    #elif defined(BLINKER_DUEROS_SENSOR)
        url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    #elif defined(BLINKER_DUEROS_FAN)
        url_iot += BLINKER_F("&duerType=FAN");
    #elif defined(BLINKER_DUEROS_AIRCONDITION)
        url_iot += BLINKER_F("&duerType=AIR_CONDITION");
    #endif

    #if defined(BLINKER_MIOT_LIGHT)
        url_iot += BLINKER_F("&miType=light");
    #elif defined(BLINKER_MIOT_OUTLET)
        url_iot += BLINKER_F("&miType=outlet");
    #elif defined(BLINKER_MIOT_MULTI_OUTLET)
        url_iot += BLINKER_F("&miType=multi_outlet");
    #elif defined(BLINKER_MIOT_SENSOR)
        url_iot += BLINKER_F("&miType=sensor");
    #elif defined(BLINKER_MIOT_FAN)
        url_iot += BLINKER_F("&miType=fan");
    #elif defined(BLINKER_MIOT_AIRCONDITION)
        url_iot += BLINKER_F("&miType=aircondition");
    #endif

    url_iot += BLINKER_F("&version=");
    url_iot += BLINKER_OTA_VERSION_CODE;
    #ifndef BLINKER_WITHOUT_SSL
    url_iot += BLINKER_F("&protocol=mqtts");
    #else
    url_iot += BLINKER_F("&protocol=mqtt");
    #endif

    String payload = httpToServer(BLINKER_CMD_PRO_AUTH_NUMBER, url_iot);

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));

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
    String _userID = root[BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_UUID];
    String _authKey = root[BLINKER_CMD_KEY];

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
        // MQTT_DEVICEID_PRO = (char*)malloc((_userID.length()+1)*sizeof(char));
        // strcpy(MQTT_DEVICEID_PRO, _userID.c_str());
        #if defined(BLINKER_WITHOUT_SSL)
            MQTT_DEVICEID_PRO = (char*)malloc((24+1)*sizeof(char));
            strcpy(MQTT_DEVICEID_PRO, _userName.substring(0, 24).c_str());
        #else
            MQTT_DEVICEID_PRO = (char*)malloc((_userID.length()+1)*sizeof(char));
            strcpy(MQTT_DEVICEID_PRO, _userID.c_str());
        #endif
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
    
    iotSub_PRO = new Adafruit_MQTT_Subscribe(mqtt_PRO, BLINKER_SUB_TOPIC_PRO);

    MDNS.end();
    webSocket_PRO.close();
    mDNSInit(MQTT_DEVICEID_PRO);
    this->latestTime = millis();
    mqtt_PRO->subscribe(iotSub_PRO);
    isMQTTinit = true;
    
    #if defined(ESP8266)
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.setInsecure();
        #endif
    #elif defined(ESP32)
        client_s.setInsecure();
    #endif
    // connect();    

    return true;
}

void BlinkerPROESP::mDNSInit(const String & name)
{
    delay(1000);

    BLINKER_LOG(BLINKER_F("WiFi.localIP: "), WiFi.localIP());

    // MDNS.end();

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
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "mac", macDeviceName());

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

void BlinkerPROESP::checkKA()
{
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

bool BlinkerPROESP::checkAliKA()
{
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

bool BlinkerPROESP::checkDuerKA()
{
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

bool BlinkerPROESP::checkMIOTKA()
{
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

bool BlinkerPROESP::checkCanPrint()
{
    if ((millis() - printTime >= BLINKER_PRO_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

bool BlinkerPROESP::checkCanBprint()
{
    if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        return false;
    }
}

bool BlinkerPROESP::checkMIOTPrintSpan()
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

bool BlinkerPROESP::checkPrintSpan()
{
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

bool BlinkerPROESP::checkAliPrintSpan()
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

bool BlinkerPROESP::checkDuerPrintSpan()
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

bool BlinkerPROESP::pubHello()
{
    char stateJsonStr[256] = ("{\"message\":\"Registration successful\"}");
    
    BLINKER_LOG_ALL(BLINKER_F("PUB hello: "), stateJsonStr);
    
    return print(stateJsonStr, false);
}

bool BlinkerPROESP::isJson(const String & data)
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

void BlinkerPROESP::parseData(const char* data)
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
    else if (_uuid == BLINKER_CMD_SERVERCLIENT || _uuid == "senderClient1")
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
            _needCheckShare = true;

            dataGet = data;
        }

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

    if (_needCheckShare && isMQTTinit) freshSharers();
}

void BlinkerPROESP::checkRegister()
{
    if (available())
    {
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, String(msgBuf_PRO));
        JsonObject root = jsonBuffer.as<JsonObject>();

        if (error)
        {
            return;
        }

        String _type = root[BLINKER_CMD_REGISTER];

        // if (_type.length() > 0)
        if (root.containsKey(BLINKER_CMD_REGISTER))
        {
            if (_type == STRING_format(_deviceType))
            {
                _getRegister = true;

                _isNew = true;

                _register_fresh = millis();

                BLINKER_LOG_ALL(BLINKER_F("getRegister!"));

                char msg[256] = "{\"message\":\"success\"}";
                print(msg);
            }
            else
            {
                BLINKER_LOG_ALL(BLINKER_F("not getRegister!"));

                char msg[256] = "{\"message\":\"deviceType check fail\"}";
                print(msg);
            }
            BLINKER_LOG_ALL(BLINKER_F("checkRegister isParsed"));

            flush();
        }
    }
}

// class BlinkerWlan
// {
//     public :
//         BlinkerWlan()
//             : _status(BWL_CONFIG_CKECK)
//         {}

//         bool checkWlanConfig();
//         void loadWlanConfig(char *_ssid, char *_pswd);
//         void saveWlanConfig(char *_ssid, char *_pswd);
//         void deleteWlanConfig();
//         void smartconfigBegin(uint16_t _time = 15000);
//         bool smartconfigDone();
//         void wlanConnect();
//         bool wlanConnected();
//         void wlanDisconnect();
//         void wlanReset();
//         bool wlanRun();
//         bwl_status_t status() { return _status; }

//         void setType(const char* _type) {
//             _deviceType = _type;

// #ifdef BLINKER_DEBUG_ALL
//             BLINKER_LOG(BLINKER_F("API deviceType: "), _type);
// #endif
//         }

//         void softAPinit();
//         void serverClient();
//         void parseUrl(String data);
//         void connectWiFi(String _ssid, String _pswd);
//         void connectWiFi(const char* _ssid, const char* _pswd);

//         // uint8_t status() { return _status; }

//     private :

//     protected :
//         char *SSID;
//         char *PSWD;
//         const char* _deviceType;
//         uint32_t connectTime;
//         uint16_t timeout;
//         bwl_status_t _status;
//         uint32_t debugStatusTime;
// };

bool BlinkerPROESP::checkWlanConfig() {
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

void BlinkerPROESP::loadWlanConfig(char *_ssid, char *_pswd) {
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

void BlinkerPROESP::saveWlanConfig(char *_ssid, char *_pswd) {
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

void BlinkerPROESP::deleteWlanConfig() {
    char ok[3] = {0};
    EEPROM.begin(BLINKER_EEP_SIZE);
    // for (int i = BLINKER_EEP_ADDR_WLAN_CHECK; i < BLINKER_WLAN_CHECK_SIZE; i++)
    //     EEPROM.write(i, 0);
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG(BLINKER_F("Erase wlan config"));
}

void BlinkerPROESP::smartconfigBegin(uint16_t _time) {
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

bool BlinkerPROESP::smartconfigDone() {
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

void BlinkerPROESP::wlanConnect() {
    switch (_status) {
        case BWL_CONFIG_SUCCESS :
            // WiFi.setAutoConnect(false);
            // WiFi.setAutoReconnect(true);

            SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
            PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

            loadWlanConfig(SSID, PSWD);
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

                loadWlanConfig(_ssid_, _pswd_);
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

bool BlinkerPROESP::wlanConnected() {
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
                saveWlanConfig(SSID, PSWD);
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
                saveWlanConfig(SSID, PSWD);
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

void BlinkerPROESP::wlanDisconnect() {
    WiFi.disconnect();
    delay(100);
    _status = BWL_DISCONNECTED;
    BLINKER_LOG(BLINKER_F("WiFi disconnected"));
}

void BlinkerPROESP::wlanReset() {
    wlanDisconnect();
    deleteWlanConfig();
    _status = BWL_RESET;
}

void BlinkerPROESP::softAPinit() {
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

    webSocket_PRO.begin();
    webSocket_PRO.onEvent(webSocketEvent_PRO);

    _status = BWL_APCONFIG_BEGIN;

    isApCfg = true;

    BLINKER_LOG(BLINKER_F("Wait for APConfig"));
}

void BlinkerPROESP::serverClient()
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
}

void BlinkerPROESP::parseUrl(String data)
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
    // MDNS.end();
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

void BlinkerPROESP::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerPROESP::connectWiFi(const char* _ssid, const char* _pswd)
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

bool BlinkerPROESP::wlanRun()
{
    // if (millis() - debugStatusTime > 10000) {
    //     debugStatusTime = millis();

    //     BLINKER_LOG_ALL("WLAN status: ", _status);
    // }

    switch (_status) {
        case BWL_CONFIG_CKECK :
            checkWlanConfig();
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
            wlanConnect();
            break;
        case BWL_CONNECTING :
            return wlanConnected();
            break;
        case BWL_CONNECTED :
            return wlanConnected();
            break;
        case BWL_DISCONNECTED :
            wlanConnect();
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
            return wlanConnected();
            break;
        case BWL_SMARTCONFIG_TIMEOUT :
            _status = BWL_CONFIG_FAIL;
            break;
        case BWL_STACONFIG_BEGIN :
            wlanConnect();
            break;
        case BWL_APCONFIG_BEGIN :
            serverClient();
            break;
        case BWL_APCONFIG_DONE :
            return wlanConnected();
            break;
        case BWL_APCONFIG_TIMEOUT :
            _status = BWL_CONFIG_FAIL;
            break;
        case BWL_CONNECTED_CHECK :
            return wlanConnected();
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
