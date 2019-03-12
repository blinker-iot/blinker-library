#ifndef BLINKER_SERIAL_SIM7020_H
#define BLINKER_SERIAL_SIM7020_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"
#include "Functions/BlinkerHTTPSIM7020.h"

#include "modules/ArduinoJson/ArduinoJson.h"
#include "Functions/BlinkerMQTTSIM7020.h"

char*       MQTT_HOST_NBIoT;
char*       MQTT_ID_NBIoT;
char*       MQTT_NAME_NBIoT;
char*       MQTT_KEY_NBIoT;
char*       MQTT_PRODUCTINFO_NBIoT;
char*       UUID_NBIoT;
char*       AUTHKEY_NBIoT;
char*       MQTT_DEVICEID_NBIoT;
char*       DEVICE_NAME_NBIoT;
char*       BLINKER_PUB_TOPIC_NBIoT;
char*       BLINKER_SUB_TOPIC_NBIoT;
uint16_t    MQTT_PORT_NBIoT;

BlinkerMQTTSIM7020* mqtt_NBIoT;

class BlinkerSerialSIM7020 : public BlinkerStream
{
    public :
        BlinkerSerialSIM7020()
            : stream(NULL), isConnect(false)
        {}

        int connect();
        int connected();
        int mConnected();
        void disconnect();
        void ping();
        int available();
        void subscribe();
        int timedRead();
        char * lastRead() { if (isFresh_NBIoT) return msgBuf_NBIoT; return ""; }
        void flush();
        // int print(const String & s, bool needCheck = true);
        int print(char * data, bool needCheck = true);
        // int bPrint(char * name, const String & data);
        // int aliPrint(const String & s);
        // int duerPrint(const String & s);
        // int aliPrint(const String & data);
        // int duerPrint(const String & data);
        void begin(const char* _deviceType, String _imei);
        void initStream(Stream& s, bool state, blinker_callback_t func);
        char * deviceName();
        char * authKey() { return AUTHKEY_NBIoT; }
        int init() { return isMQTTinit; }
        int reRegister() { return connectServer(); }
        int deviceRegister() { return connectServer(); }
        // int authCheck();
        void freshAlive() { kaTime = millis(); isAlive = true; }
        // int  needFreshShare();

    private :
        bool        isMQTTinit = false;

        int connectServer();
        void checkKA();
        int checkCanPrint();
        int checkPrintSpan();
    protected :
        Stream*     stream;
        // char*       streamData;
        char*       msgBuf_NBIoT;
        // bool        isFresh = false;
        bool        isFresh_NBIoT = false;
        bool        isConnect;
        bool        isHWS = false;
        char*       imei;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        bool        isAvail_NBIoT = false;
        uint8_t     dataFrom_NBIoT = BLINKER_MSG_FROM_MQTT;

        // uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        const char* _deviceType;
        bool        isAlive = false;
        uint32_t    kaTime = 0;
        uint32_t    latestTime;
        uint32_t    printTime = 0;

        int isJson(const String & data);

        uint8_t     reconnect_time = 0;
        blinker_callback_t listenFunc = NULL;
};

int BlinkerSerialSIM7020::connect()
{
    if (!isMQTTinit) return false;

    if (mqtt_NBIoT->connected()) return true;

    disconnect();

    if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    {
        yield();
        return false;
    }

    BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));

    BLINKER_LOG_FreeHeap_ALL();

    if (!mqtt_NBIoT->connect())
    {
        BLINKER_LOG(BLINKER_F("Retrying MQTT connection in "), \
                    BLINKER_MQTT_CONNECT_TIMESLOT/1000, \
                    BLINKER_F(" seconds..."));

        this->latestTime = millis();
        reconnect_time += 1;
        if (reconnect_time >= 12)
        {
            reRegister();
            reconnect_time = 0;
        }
        return false;
    }

    reconnect_time = 0;
    
    BLINKER_LOG(BLINKER_F("MQTT Connected!"));
    BLINKER_LOG_FreeHeap();

    this->latestTime = millis();

    return true;
}

int BlinkerSerialSIM7020::connected()
{
    if (!isMQTTinit) return false;

    return mqtt_NBIoT->connected();
}

int BlinkerSerialSIM7020::mConnected()
{
    if (!isMQTTinit) return false;
    else return mqtt_NBIoT->connected();
}

void BlinkerSerialSIM7020::disconnect()
{
    if (isMQTTinit) mqtt_NBIoT->disconnect();
}

void BlinkerSerialSIM7020::ping()
{
    BLINKER_LOG_ALL(BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!isMQTTinit) return;

    if (!mqtt_NBIoT->connected())
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

int BlinkerSerialSIM7020::available()
{
    if (isMQTTinit) {
        checkKA();

        // if (!mqtt_PRO->connected() || \
        //     (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
        if ((millis() - this->latestTime) > 30000)
        {
            ping();
        }
        else
        {
            subscribe();
        }
    }

    if (isAvail_NBIoT)
    {
        isAvail_NBIoT = false;
        return true;
    }
    else {
        return false;
    }
}

void BlinkerSerialSIM7020::subscribe()
{
    if (!isMQTTinit) return;

    if (mqtt_NBIoT->readSubscription())
    {
        BLINKER_LOG_ALL(BLINKER_F("Got: "), mqtt_NBIoT->lastRead);

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(String(mqtt_NBIoT->lastRead));

        String _uuid = root["fromDevice"];
        String dataGet = root["data"];
        
        BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
        BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);
        
        if (strcmp(_uuid.c_str(), UUID_NBIoT) == 0)
        {
            BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));
            
            kaTime = millis();
            isAvail_NBIoT = true;
            isAlive = true;

            // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        }

        if (isFresh_NBIoT) free(msgBuf_NBIoT);
        msgBuf_NBIoT = (char*)malloc((dataGet.length()+1)*sizeof(char));
        strcpy(msgBuf_NBIoT, dataGet.c_str());
        isFresh_NBIoT = true;
        
        this->latestTime = millis();

        dataFrom_NBIoT = BLINKER_MSG_FROM_MQTT;
    }
}

void BlinkerSerialSIM7020::flush()
{
    if (isFresh_NBIoT)
    {
        free(msgBuf_NBIoT); isFresh_NBIoT = false; isAvail_NBIoT = false;
        // isAliAvail = false; //isBavail = false;
    }
}

int BlinkerSerialSIM7020::print(char * data, bool needCheck)
{
    BLINKER_LOG_ALL(BLINKER_F("data: "), data);

    uint16_t num = strlen(data);

    data[num+8] = '\0';

    for(uint16_t c_num = num; c_num > 0; c_num--)
    {
        data[c_num+7] = data[c_num-1];
    }

    // String data_add = BLINKER_F("{\"data\":");
    char data_add[20] = "{\"data\":";
    for(uint16_t c_num = 0; c_num < 8; c_num++)
    {
        data[c_num] = data_add[c_num];
    }

    // data_add = BLINKER_F(",\"fromDevice\":\"");
    // strcat(data, data_add.c_str());
    strcat(data, ",\"fromDevice\":\"");
    strcat(data, MQTT_ID_NBIoT);
    // strcat(data, MQTT_DEVICEID_NBIoT); //PRO
    // data_add = BLINKER_F("\",\"toDevice\":\"");
    // strcat(data, data_add.c_str());
    strcat(data, "\",\"toDevice\":\"");
    // if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
    // {
    //     strcat(data, _sharers[_sharerFrom]->uuid());
    // }
    // else
    // {
        strcat(data, UUID_NBIoT);
    // }
    // data_add = BLINKER_F("\",\"deviceType\":\"OwnApp\"}");

    // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    // strcat(data, data_add.c_str());
    strcat(data, "\",\"deviceType\":\"OwnApp\"}");

    // data_add = STRING_format(data);

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
    
    if (mqtt_NBIoT->connected())
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
        // if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, msg_data.c_str()))
        if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, data))
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

void BlinkerSerialSIM7020::begin(const char* _type, String _imei)
{
    _deviceType = _type;
    
    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    // stream = &s;
    // stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    // isHWS = state;

    imei = (char*)malloc((_imei.length() + 1)*sizeof(char));
    strcpy(imei, _imei.c_str());
}

void BlinkerSerialSIM7020::initStream(Stream& s, bool state, blinker_callback_t func)
{
    // _deviceType = _type;
    
    // BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;

    listenFunc = func;

    // _imei = (char*)malloc(imei.length()*sizeof(char));
    // strcpy(_imei, imei.c_str());
}

char * BlinkerSerialSIM7020::deviceName() { return MQTT_DEVICEID_NBIoT;/*MQTT_ID_PRO;*/ }

void BlinkerSerialSIM7020::checkKA()
{
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerSerialSIM7020::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_PRO_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

int BlinkerSerialSIM7020::checkPrintSpan()
{
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
    {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT)
        {
            BLINKER_ERR_LOG(BLINKER_F("DEVICE NOT CONNECT OR MSG LIMIT"));
            
            return false;
        }
        else
        {
            respTimes++;
            return true;
        }
    }
    else
    {
        respTimes = 0;
        return true;
    }
}

int BlinkerSerialSIM7020::connectServer()
{
    String host = BLINKER_F("https://iotdev.clz.me");
    String uri = "";
    // uri += BLINKER_F("/api/v1/user/device/register?deviceType=");
    // uri += _deviceType;
    // uri += BLINKER_F("&deviceName=");
    // uri += imei;
    uri += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    uri += _deviceType;

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), host + uri);

    BlinkerHTTPSIM7020 http(*stream, isHWS, listenFunc);

    http.begin(host, uri);

    String payload;

    if (http.GET())
    {
        BLINKER_LOG(BLINKER_F("[HTTP] GET... success"));

        payload = http.getString();

        // return true;
    }
    else
    {
        BLINKER_LOG(BLINKER_F("[HTTP] GET... failed"));

        return false;
    }

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || !root.success() ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG(("Please make sure you have register this device!"));
            // ::delay(60000);

            return false;
        // }
    }

    String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];

    if (isMQTTinit)
    {
        free(MQTT_HOST_NBIoT);
        free(MQTT_ID_NBIoT);
        free(MQTT_NAME_NBIoT);
        free(MQTT_KEY_NBIoT);
        free(MQTT_PRODUCTINFO_NBIoT);
        free(UUID_NBIoT);
        free(DEVICE_NAME_NBIoT);
        free(BLINKER_PUB_TOPIC_NBIoT);
        free(BLINKER_SUB_TOPIC_NBIoT);
        free(mqtt_NBIoT);
        // free(iotSub_NBIoT);

        isMQTTinit = false;
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
        DEVICE_NAME_NBIoT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_NBIoT, _userID.c_str());
        MQTT_ID_NBIoT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_NBIoT, _userID.c_str());
        MQTT_NAME_NBIoT = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_NBIoT, _userName.c_str());
        MQTT_KEY_NBIoT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_NBIoT, _key.c_str());
        MQTT_PRODUCTINFO_NBIoT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_NBIoT, _productInfo.c_str());
        MQTT_HOST_NBIoT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_NBIoT, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT_NBIoT = BLINKER_MQTT_ALIYUN_PORT;
    }

    UUID_NBIoT = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_NBIoT, _uuid.c_str());

    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME_NBIoT: "), DEVICE_NAME_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_NBIoT: "), MQTT_PRODUCTINFO_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_NBIoT: "), MQTT_ID_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_NBIoT: "), MQTT_NAME_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_NBIoT: "), MQTT_KEY_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("UUID_NBIoT: "), UUID_NBIoT);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        String PUB_TOPIC_STR = BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_PRODUCTINFO_NBIoT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_ID_NBIoT;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_NBIoT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_NBIoT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_NBIoT, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_NBIoT: "), BLINKER_PUB_TOPIC_NBIoT);

        String SUB_TOPIC_STR = BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_NBIoT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_ID_NBIoT;
        SUB_TOPIC_STR += BLINKER_F("/r");

        BLINKER_SUB_TOPIC_NBIoT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_NBIoT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_NBIoT, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_NBIoT: "), BLINKER_SUB_TOPIC_NBIoT);
    }

    // if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        mqtt_NBIoT = new BlinkerMQTTSIM7020(*stream, isHWS, MQTT_HOST_NBIoT, MQTT_PORT_NBIoT, 
                                        MQTT_ID_NBIoT, MQTT_NAME_NBIoT, MQTT_KEY_NBIoT, listenFunc);
    // }

    this->latestTime = millis();
    isMQTTinit = true;
    mqtt_NBIoT->subscribe(BLINKER_SUB_TOPIC_NBIoT);

    return true;
}

int BlinkerSerialSIM7020::isJson(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("isJson: "), data);

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(data);

    if (!root.success())
    {
        BLINKER_ERR_LOG("Print data is not Json! ", data);
        return false;
    }
    return true;
}


#endif
