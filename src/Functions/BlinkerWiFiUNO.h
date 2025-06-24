#ifndef BLINKER_WIFI_UNO_H
#define BLINKER_WIFI_UNO_H

#if defined(ARDUINO_ARCH_RENESAS)

#define BLINKER_WIFI

#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "WiFiSSLClient.h"

#include <EEPROM.h>

#include "../modules/ArduinoMDNS/ArduinoMDNS.h"
#include "../modules/WebSockets/WebSocketsServer.h"
#include "../modules/pubsubclient/PubSubClient.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "Adapters/BlinkerMQTT.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
// #include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"
#include "../Blinker/BlinkerHTTP.h"

static const char *TAG_WIFI_UNO = "[BlinkerWiFiUNO] ";

enum b_broker_t {
    aliyun_b,
    blinker_b
};

class BlinkerWiFiUNO;
BlinkerWiFiUNO* BlinkerWiFiUNO_instance = nullptr;

WiFiUDP udp;
MDNS mdns(udp);

class BlinkerWiFiUNO
{
    public :
        BlinkerWiFiUNO();
        ~BlinkerWiFiUNO();

        void begin(const char* auth);
        bool init();
        void reset();
        b_device_staus_t status() { return _status; }
        bool connect();
        void disconnect();
        void ping();
        bool available();
        char* lastRead();
        void flush();
        int print(char* data, bool needCheck = true);
        int autoPrint(unsigned long id);
        // void subcribe();
        int isJson(const String& data);
        bool deviceRegister();
        void freshSharers();

        char * deviceName() {
            return DEVICE_NAME_MQTT;
        }
        char * authKey() {
            return (char *)_authKey;
        }
        char * token() {
            if (!isMQTTinit) return "";
            else return MQTT_KEY_MQTT;
        }
        
        String httpServer(uint8_t _type, const String& msg, bool state = false)
        {
            return httpToServer(_type, msg, state);
        }

        bool checkInit() {
            return isMQTTinit;
        }
        bool checkWlanInit();
        void commonBegin(const char* _ssid, const char* _pswd);

        void webSocketEventHandler(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

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

        void subcribeEventHandler(char* topic, byte* payload, unsigned int length);

    private :
        String createTextRecord(const String& key, const String& value);
        void mDNSInit();
        // bool autoInit();
        // bool checkConfig();

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
        b_broker_t  _use_broker = blinker_b;
        char        _messageId[20];
        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM] = { nullptr };
        bool        _isWiFiInit = false;
        // bool        _isBegin = false;
        bool        isMQTTinit = false;
        uint32_t    latestTime;
        bool*       isHandle = nullptr;
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
        bool        isSubcribe = false;

        const char* _authKey;

        // b_config_t  _configType = COMM;
        // b_configStatus_t _configStatus = AUTO_INIT;
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

        b_device_staus_t _status = BLINKER_WLAN_CONNECTING;

        char*    msgBuf_MQTT = nullptr;
        bool     isFresh_MQTT = false;
        bool     isConnect_MQTT = false;
        bool     isAvail_MQTT = false;
        bool     isApCfg = false;
        uint8_t  ws_num_MQTT = 0;
        uint8_t  dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;
    #if defined(BLINKER_WITHOUT_SSL)
        WiFiClient wifiClient;
    #else
        WiFiSSLClient wifiClient;
    #endif
        PubSubClient* mqtt_MQTT = nullptr;

        char*       MQTT_HOST_MQTT = nullptr;
        char*       MQTT_ID_MQTT = nullptr;
        char*       MQTT_NAME_MQTT = nullptr;
        char*       MQTT_KEY_MQTT = nullptr;
        char*       MQTT_PRODUCTINFO_MQTT = nullptr;
        char*       UUID_MQTT = nullptr;
        char*       DEVICE_NAME_MQTT = nullptr;
        char*       BLINKER_PUB_TOPIC_MQTT = nullptr;
        char*       BLINKER_SUB_TOPIC_MQTT = nullptr;
        // char*       BLINKER_RRPC_PUB_TOPIC_MQTT;
        char*       BLINKER_RRPC_SUB_TOPIC_MQTT = nullptr;
        char*       UUID_EXTRA = nullptr;
        uint16_t    MQTT_PORT_MQTT;
        char get_key[33] = { 0 };
};

#define WS_SERVERPORT       81
WebSocketsServer webSocket_MQTT = WebSocketsServer(WS_SERVERPORT);

void webSocketEvent_MQTT(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
    if (BlinkerWiFiUNO_instance) {
        BlinkerWiFiUNO_instance->webSocketEventHandler(num, type, payload, length);
    }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    if (BlinkerWiFiUNO_instance) {
        BlinkerWiFiUNO_instance->subcribeEventHandler(topic, payload, length);
    }
}

BlinkerWiFiUNO WiFiUNO;

BlinkerWiFiUNO::BlinkerWiFiUNO() {
    isHandle = &isConnect_MQTT;
    BlinkerWiFiUNO_instance = this;
}

BlinkerWiFiUNO::~BlinkerWiFiUNO() {
    disconnect();

    if (mqtt_MQTT) {
        delete mqtt_MQTT;
        mqtt_MQTT = nullptr;
    }
    // if (iotSub_MQTT) {
    //     delete iotSub_MQTT;
    //     iotSub_MQTT = nullptr;
    // }
    if (MQTT_HOST_MQTT) {
        free(MQTT_HOST_MQTT);
        MQTT_HOST_MQTT = nullptr;
    }
    if (MQTT_ID_MQTT) {
        free(MQTT_ID_MQTT);
        MQTT_ID_MQTT = nullptr;
    }
    if (MQTT_NAME_MQTT) {
        free(MQTT_NAME_MQTT);
        MQTT_NAME_MQTT = nullptr;
    }
    if (MQTT_KEY_MQTT) {
        free(MQTT_KEY_MQTT);
        MQTT_KEY_MQTT = nullptr;
    }
    if (MQTT_PRODUCTINFO_MQTT) {
        free(MQTT_PRODUCTINFO_MQTT);
        MQTT_PRODUCTINFO_MQTT = nullptr;
    }
    if (UUID_MQTT) {
        free(UUID_MQTT);
        UUID_MQTT = nullptr;
    }
    if (DEVICE_NAME_MQTT) {
        free(DEVICE_NAME_MQTT);
        DEVICE_NAME_MQTT = nullptr;
    }
    if (BLINKER_PUB_TOPIC_MQTT) {
        free(BLINKER_PUB_TOPIC_MQTT);
        BLINKER_PUB_TOPIC_MQTT = nullptr;
    }
    if (BLINKER_SUB_TOPIC_MQTT) {
        free(BLINKER_SUB_TOPIC_MQTT);
        BLINKER_SUB_TOPIC_MQTT = nullptr;
    }
    if (BLINKER_RRPC_SUB_TOPIC_MQTT) {
        free(BLINKER_RRPC_SUB_TOPIC_MQTT);
        BLINKER_RRPC_SUB_TOPIC_MQTT = nullptr;
    }
    if (UUID_EXTRA) {
        free(UUID_EXTRA);
        UUID_EXTRA = nullptr;
    }
    if (msgBuf_MQTT) {
        free(msgBuf_MQTT);
        msgBuf_MQTT = nullptr;
    }
    
    isHandle = nullptr;
    
    BlinkerWiFiUNO_instance = nullptr;

    for (uint8_t i = 0; i < BLINKER_MQTT_MAX_SHARERS_NUM; i++) {
        if (_sharers[i]) {
            delete _sharers[i];
            _sharers[i] = nullptr;
        }
    }
}

void BlinkerWiFiUNO::webSocketEventHandler(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
    switch(type)
    {
        case WStype_DISCONNECTED:
            // BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

            if (!isApCfg) isConnect_MQTT = false;
            break;
        case WStype_CONNECTED:
            {
                // IPAddress ip = webSocket_MQTT.remoteIP(num);

                BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("num: "), num,
                                BLINKER_F(", url: "), (char *)payload);

                // send message to client
                webSocket_MQTT.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num_MQTT = num;

                if (!isApCfg) isConnect_MQTT = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("num: "), num,
                            BLINKER_F(", get Text: "), (char *)payload,
                            BLINKER_F(", length: "), length);

            if (length < BLINKER_MAX_READ_SIZE) {
                if (isFresh_MQTT) free(msgBuf_MQTT);
                msgBuf_MQTT = (char*)malloc((length+1)*sizeof(char));
                strcpy(msgBuf_MQTT, (char*)payload);
                isAvail_MQTT = true;
                isFresh_MQTT = true;

                BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("isAvail_MQTT: "), isAvail_MQTT);
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

void BlinkerWiFiUNO::subcribeEventHandler(char* topic, byte* payload, unsigned int length) {
    // if (mqtt_MQTT) {
    //     mqtt_MQTT->setCallback(onMqttMessage);
    //     mqtt_MQTT->loop();
    // }
    
    String message = String((char*)payload).substring(0, length);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("Received message on topic: "), topic);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("Message: "), message);
    
    // 对比topic是否和BLINKER_SUB_TOPIC_MQTT相同
    if (strcmp(topic, BLINKER_SUB_TOPIC_MQTT) == 0) {
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("Message is for this device"));
    } else {
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("Message is not for this device"));
        return; // 如果不是本设备的消息，则不处理
    }
    parseData(message.c_str());
}

bool BlinkerWiFiUNO::init() {
    if (!checkWlanInit()) return false;
    
    if (isMQTTinit) return true;

    if (_connectTimes < BLINKER_SERVER_CONNECT_LIMIT) {
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

void BlinkerWiFiUNO::reset() {
    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("Blinker reset..."));
    // todo
    NVIC_SystemReset();
}

bool BlinkerWiFiUNO::connect() {
    if (!checkInit()) return false;

    int8_t ret;

    webSocket_MQTT.loop();

    if (mqtt_MQTT->connected())
    {
        _status = BLINKER_DEV_CONNECTED;
        return true;
    }
    
    disconnect();

    // if (_status == BLINKER_DEV_CONNECTED)
    // {
    //     return true;
    // }

    if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    {
        yield();
        return false;
    }

    _status = BLINKER_DEV_CONNECTING;

    ret = mqtt_MQTT->connect(MQTT_ID_MQTT, MQTT_NAME_MQTT, MQTT_KEY_MQTT);

    if (!ret)
    {
        BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("ret: "), ret);    

        BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("Connecting to MQTT... "));

        BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("reconnect_time: "), reconnect_time);
        
        ret = mqtt_MQTT->state();
        BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT connect failed, state: "), ret);
        BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("Retrying MQTT connection in "), \
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

            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("_reRegister_times1: "), _reRegister_times);
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
    else
    {
        // if (!isSubcribe)
        // {
        //     isSubcribe = true;
            mqtt_MQTT->subscribe(BLINKER_SUB_TOPIC_MQTT);
        //     BLINKER_LOG_ALL(BLINKER_F("MQTT subscribe: "), BLINKER_SUB_TOPIC_MQTT);
        // }
        _status = BLINKER_DEV_CONNECTED;
        return true;
    }

    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Connected!"));
    BLINKER_LOG_FreeHeap();

    if (!isMQTTinit)
    {
        isMQTTinit = true;

        mqtt_MQTT->subscribe(BLINKER_SUB_TOPIC_MQTT);
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT subscribe: "), BLINKER_SUB_TOPIC_MQTT);
    }

    this->latestTime = millis();

    return true;
}

void BlinkerWiFiUNO::disconnect() {
    if (!checkInit()) return;

    if (mqtt_MQTT->connected()) mqtt_MQTT->disconnect();

    if (*isHandle) webSocket_MQTT.disconnect();

    _status = BLINKER_DEV_DISCONNECTED;
}

void BlinkerWiFiUNO::ping() {
    if (!checkInit()) return;

    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!mqtt_MQTT->connected())
    {
        disconnect();
    }
    else
    {
        this->latestTime = millis();
    }
}

bool BlinkerWiFiUNO::available() {
    if (!checkInit()) return false;

    webSocket_MQTT.loop();

    mdns.run();

    if ((millis() - latestTime) > BLINKER_MQTT_PING_TIMEOUT)
    {
        ping();
    }
    else
    {
        mqtt_MQTT->loop();
    }

    if (isAvail_MQTT)
    {
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("available: "), isAvail_MQTT);

        isAvail_MQTT = false;
        return true;
    }
    else {
        return false;
    }
}

char * BlinkerWiFiUNO::lastRead() {
    if (isFresh_MQTT) {
        return msgBuf_MQTT;
    } else {
        return nullptr;
    }
}

void BlinkerWiFiUNO::flush() {
    if (isFresh_MQTT) {
        free(msgBuf_MQTT);
        msgBuf_MQTT = nullptr;
        isFresh_MQTT = false;
        isAvail_MQTT = false;
        isAliAvail = false;
        isDuerAvail = false;
        isMIOTAvail = false; // isBavail = false;

        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("flush"));
        BLINKER_LOG_FreeHeap_ALL();
    }
}

int BlinkerWiFiUNO::print(char* data, bool needCheck) {
    if (!checkInit()) return false;

    if (!mqtt_MQTT->connected()) {
        return false;
    }

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

        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("WS response: "));
        BLINKER_LOG_ALL(TAG_WIFI_UNO, data);
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("Success..."));

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
            // strcat(data, _sharers[_sharerFrom]->uuid());
            strcat(data, UUID_EXTRA);

            free(UUID_EXTRA);
        }
        else
        {
            strcat(data, UUID_MQTT);
        }

        strcat(data, "\",\"deviceType\":\"OwnApp\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        if (!isJson(STRING_format(data))) return false;

        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT Publish..."));
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
                    if (!_alive) isAlive = false;
                    return false;
                }

                if (!checkPrintLimit())
                {
                    return false;
                }

                _print_times++;

                BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("_print_times: "), _print_times);
            }

            if (mqtt_MQTT->publish(BLINKER_PUB_TOPIC_MQTT, data))
            {
                BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT Publish Success!"));
                BLINKER_LOG_FreeHeap_ALL();
                if (needCheck) printTime = millis();

                if (!_alive)
                {
                    isAlive = false;
                }

                this->latestTime = millis();
                return num + 8;
            }
            else
            {
                if (!_alive)
                {
                    isAlive = false;
                }
                BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Publish Failed!"));
                return false;
            }
        }
        else
        {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Disconnected"));
            isAlive = false;
            return false;
        }
    }
}

int BlinkerWiFiUNO::autoPrint(unsigned long id) {
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("autoTrigged id: "), id);

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

    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("autoPrint..."));

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
                BLINKER_LOG_ALL(TAG_WIFI_UNO, payload);
                BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...Failed"));

                return false;
            }
            else
            {
                BLINKER_LOG_ALL(TAG_WIFI_UNO, payload);
                BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...OK!"));

                linkTime = millis();

                this->latestTime = millis();

                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);

            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiUNO::isJson(const String& data) {
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("isJson: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(STRING_format(data));
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, STRING_format(data));
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, "Print data is not Json! ", data);
        return false;
    }

    return true;
}

bool BlinkerWiFiUNO::deviceRegister() {
    _status = BLINKER_DEV_REGISTER;

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
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("Maybe you have put in the wrong AuthKey!"));
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("Or maybe your request is too frequently!"));
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("Or maybe your network is disconnected!"));

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
    BLINKER_LOG_ALL(TAG_WIFI_UNO, "_num: ", _num);
    if (_num > 0) _num += 3;
    _host = _host.substring(_num, _host.length());

    if (mqtt_MQTT)
    {
        free(mqtt_MQTT);
        // free(iotSub_MQTT);
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("===================="));
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F(" Error! Not Blinker Broker device!"));
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("Only support Blinker Broker device!"));
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("===================="));
        return false;
    }

    if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        if(!isMQTTinit) DEVICE_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT, _userID.c_str());
        if(!isMQTTinit) MQTT_ID_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT, _userID.c_str());
        if(!isMQTTinit) MQTT_NAME_MQTT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT, _userID.c_str());
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

    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("===================="));
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("DEVICE_NAME_MQTT: "), DEVICE_NAME_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT_PRODUCTINFO_MQTT: "), MQTT_PRODUCTINFO_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT_ID_MQTT: "), MQTT_ID_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT_NAME_MQTT: "), MQTT_NAME_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT_KEY_MQTT: "), MQTT_KEY_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("HOST: "), MQTT_HOST_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("PORT: "), MQTT_PORT_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("HOST: "), _host);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("PORT: "), _port);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("UUID_MQTT: "), UUID_MQTT);
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        String PUB_TOPIC_STR = BLINKER_F("/device");
        // PUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_ID_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/s");

        if(!isMQTTinit) BLINKER_PUB_TOPIC_MQTT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT);

        String SUB_TOPIC_STR = BLINKER_F("/device");
        // SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_ID_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/r");

        if(!isMQTTinit) BLINKER_SUB_TOPIC_MQTT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT);
    }

    if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        mqtt_MQTT = new PubSubClient(wifiClient);
        mqtt_MQTT->setServer(MQTT_HOST_MQTT, MQTT_PORT_MQTT);
    }

    this->latestTime = millis() - BLINKER_MQTT_CONNECT_TIMESLOT;

    mqtt_MQTT->setCallback(onMqttMessage);
    // mqtt_MQTT->subscribe(BLINKER_SUB_TOPIC_MQTT);

    // freshSharers();

    return true;
}

void BlinkerWiFiUNO::freshSharers() {
    String  data = BLINKER_F("/share/device?");
            data += BLINKER_F("deviceName=");
            data += STRING_format(DEVICE_NAME_MQTT);
            data += BLINKER_F("&key=");
            data += STRING_format(_authKey);

    String payload = httpToServer(BLINKER_CMD_FRESH_SHARERS_NUMBER, data);

    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("sharers data: "), payload);

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
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("sharer uuid: "), user_name, BLINKER_F(", length: "), user_name.length());

            _sharerCount++;

            _sharers[num] = new BlinkerSharer(user_name);
        }
        else
        {
            break;
        }
    }
}

String BlinkerWiFiUNO::createTextRecord(const String& key, const String& value) {
    String record = key + "=" + value;
    char lengthByte = (char)record.length();
    return String(lengthByte) + record;
}

void BlinkerWiFiUNO::mDNSInit() {
    mdns.begin(WiFi.localIP(), DEVICE_NAME_MQTT);
    String serviceName = String(BLINKER_MDNS_SERVICE_BLINKER) + "._blinker";

    String textRecord = createTextRecord("deviceName", DEVICE_NAME_MQTT) + 
                        createTextRecord("mac", macDeviceName());
    
    mdns.addServiceRecord(serviceName.c_str(), WS_SERVERPORT, MDNSServiceTCP, textRecord.c_str());

    webSocket_MQTT.begin();
    webSocket_MQTT.onEvent(webSocketEvent_MQTT);
    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("WebSocket server started"));
    // BLINKER_LOG(BLINKER_F("Connect to: ws://"), DEVICE_NAME_MQTT, BLINKER_F(":"), WS_SERVERPORT);
}

bool BlinkerWiFiUNO::checkWlanInit() {
    if (_isWiFiInit) return true;

    if (WiFi.status() == WL_CONNECTED) {
        _isWiFiInit = true;
        BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("WiFi connected!"));
        return true;
    } else {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("WiFi not connected!"));
        ::delay(1000);
        return false;
    }
}

void BlinkerWiFiUNO::commonBegin(const char* _ssid, const char* _pswd) {
    
    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("UNOR4_MQTT initialied..."));

    connectWiFi(_ssid, _pswd);
}

void BlinkerWiFiUNO::connectWiFi(String _ssid, String _pswd) {
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerWiFiUNO::connectWiFi(const char* _ssid, const char* _pswd) {
    uint32_t _connectTime = millis();
    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("Connecting to ["), _ssid, "]...");

    if (WiFi.status() == WL_NO_MODULE) {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("Communication with WiFi module failed!"));
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(_ssid, _pswd);
    }
}

void BlinkerWiFiUNO::parseData(const char* data) {
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
        _sharerFrom = 0;

        BLINKER_LOG_ALL(BLINKER_F("form extra uuid"), _uuid);

        UUID_EXTRA = (char*)malloc((_uuid.length()+1)*sizeof(char));
        strcpy(UUID_EXTRA, _uuid.c_str());

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

void BlinkerWiFiUNO::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerWiFiUNO::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT / 2 && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

        checkKA();

        return false;
    }
}

int BlinkerWiFiUNO::checkPrintSpan() {
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));

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

int BlinkerWiFiUNO::checkPrintLimit() {
    if ((millis() - _print_time) < 60000)
    {
        if (_print_times < 30) return true;
        else 
        {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT MSG LIMIT"));
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
int BlinkerWiFiUNO::aliAvail()
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

int BlinkerWiFiUNO::aliPrint(const String & data)
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

    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT AliGenie Publish..."));
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

            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
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
            BLINKER_LOG_ALL(TAG_WIFI_UNO, data_add);
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            isAliAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(TAG_WIFI_UNO, data_add);
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            isAliAlive = false;

            this->latestTime = millis();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}    

int BlinkerWiFiUNO::checkAliKA()
{
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerWiFiUNO::checkAliPrintSpan()
{
    if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("ALIGENIE NOT ALIVE OR MSG LIMIT"));

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
int BlinkerWiFiUNO::duerAvail()
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

int BlinkerWiFiUNO::duerPrint(const String & data, bool report)
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

    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("MQTT DuerOS Publish..."));
    BLINKER_LOG_FreeHeap_ALL();
    BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("is_rrpc: "), is_rrpc);

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

            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
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
            BLINKER_LOG_ALL(TAG_WIFI_UNO, data_add);
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            isDuerAlive = false;
            
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(TAG_WIFI_UNO, data_add);
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            isDuerAlive = false;

            this->latestTime = millis();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiUNO::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerWiFiUNO::checkDuerPrintSpan()
{
    if (millis() - respDuerTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respDuerTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

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
int BlinkerWiFiUNO::miAvail()
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

int BlinkerWiFiUNO::miPrint(const String & data)
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

            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
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
            BLINKER_LOG_ALL(TAG_WIFI_UNO, data_add);
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            isMIOTAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(TAG_WIFI_UNO, data_add);
            BLINKER_LOG_ALL(TAG_WIFI_UNO, BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            isMIOTAlive = false;

            this->latestTime = millis();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiUNO::checkMIOTKA() {
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerWiFiUNO::checkMIOTPrintSpan()
{
    if (millis() - respMIOTTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respMIOTTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG_WIFI_UNO, BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

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

void BlinkerWiFiUNO::begin(const char* auth)
{
    _authKey = auth;

    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("Blinker WiFi UNO R4 begin..."));
    BLINKER_LOG(TAG_WIFI_UNO, BLINKER_F("AuthKey: "), auth);
}

#endif

#endif
