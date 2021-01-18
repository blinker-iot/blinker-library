#ifndef BLINKER_SERIAL_SIM7020_H
#define BLINKER_SERIAL_SIM7020_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"
#include "../Functions/BlinkerSIM7020.h"
#include "../Functions/BlinkerHTTPSIM7020.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
#include "../Functions/BlinkerMQTTSIM7020.h"

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
        int aligenieAvail();
        int duerAvail();
        void subscribe();
        int timedRead();
        char * lastRead() { if (isFresh_NBIoT) return msgBuf_NBIoT; return ""; }
        void flush();
        // int print(const String & s, bool needCheck = true);
        int print(char * data, bool needCheck = true);
        int toServer(char * data);
        int bPrint(char * name, const String & data);
        int aliPrint(const String & data);
        int  duerPrint(const String & data, bool report = false);
        void aliType(const String & type);
        void duerType(const String & type);
        void begin(const char* _deviceType, String _imei);
        void initStream(Stream& s, bool state, blinker_callback_t func);
        int autoPrint(unsigned long id);
        char * deviceName();
        char * authKey() { return AUTHKEY_NBIoT; }
        char * token() { if (!isMQTTinit) return ""; else return MQTT_KEY_NBIoT; }
        int init() { return isMQTTinit; }
        int reRegister() { return connectServer(); }
        int deviceRegister() { return connectServer(); }
        // int authCheck();
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
        bool        isMQTTinit = false;

        int connectServer();
        void checkKA();
        int checkAliKA();
        int checkDuerKA();
        int checkCanPrint();
        int checkCanBprint();
        int checkPrintSpan();
        int checkAliPrintSpan();
        int checkDuerPrintSpan();

    protected :
        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
        uint8_t     _sharerCount = 0;
        uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        Stream*     stream;
        // char*       streamData;
        char*       msgBuf_NBIoT;
        // bool        isFresh = false;
        bool        isFresh_NBIoT = false;
        bool        isConnect;
        bool        isHWS = false;
        char*       imei;
        // uint8_t     respTimes = 0;
        // uint32_t    respTime = 0;
        bool        isAvail_NBIoT = false;
        uint8_t     dataFrom_NBIoT = BLINKER_MSG_FROM_MQTT;

        // uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        const char* _deviceType;//_authKey
        char*       _aliType;
        char*       _duerType;
        bool        isAlive = false;
        // bool        isBavail = false;
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

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        uint32_t    duerKaTime = 0;
        bool        isDuerAlive = false;
        bool        isDuerAvail = false;
        char*       mqtt_broker;

        int isJson(const String & data);

        uint8_t     reconnect_time = 0;
        blinker_callback_t listenFunc = NULL;
};

int BlinkerSerialSIM7020::connect()
{
    BLINKER_LOG_ALL(BLINKER_F(">>>>>> mqtt connect <<<<<<"));

    if (!isMQTTinit) return false;

    // if (mqtt_NBIoT->connected()) return true;
    if (isConnect) return true;

    BLINKER_LOG_ALL(BLINKER_F(">>>>>> mqtt connect failed <<<<<<"));

    disconnect();

    // if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    // {
    //     yield();
    //     return false;
    // }

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

    isConnect = true;

    this->latestTime = millis();

    return true;
}

int BlinkerSerialSIM7020::connected()
{
    if (!isMQTTinit) return false;

    return isConnect;//mqtt_NBIoT->connected();
}

int BlinkerSerialSIM7020::mConnected()
{
    if (!isMQTTinit) return false;
    else return mqtt_NBIoT->connected();
}

void BlinkerSerialSIM7020::disconnect()
{
    if (isMQTTinit) {
        isConnect = false;
        mqtt_NBIoT->disconnect();
    }
}

void BlinkerSerialSIM7020::ping()
{
    BLINKER_LOG_ALL(BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!isMQTTinit) return;

    if (!isConnect)
    {
        // disconnect();

        delay(100);

        connect();
        // delay(100);

        // connect();
    }
    else
    {
        this->latestTime = millis();
    }

    BlinkerSIM7020 BLINKER_SIM7020;
    BLINKER_SIM7020.setStream(*stream, isHWS, listenFunc);  

    BLINKER_LOG_ALL(BLINKER_F("check isReboot"));

    if (BLINKER_SIM7020.isReboot())
    {
        BLINKER_SIM7020.powerCheck();

        // disconnect();
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

int BlinkerSerialSIM7020::aligenieAvail()
{
    if (!isMQTTinit) return false;

    if (isAliAvail)
    {
        isAliAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerSerialSIM7020::duerAvail()
{
    if (!isMQTTinit) return false;

    if (isDuerAvail)
    {
        isDuerAvail = false;
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

        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& root = jsonBuffer.parseObject(String(mqtt_NBIoT->lastRead));
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, String(mqtt_NBIoT->lastRead));
        JsonObject root = jsonBuffer.as<JsonObject>();

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

            isAvail_NBIoT = true;
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
                        BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
                                    check is from bridge/share device, \
                                    data: "), dataGet);

                        _needCheckShare = true;
                    }
                }
            }
            // root.printTo(dataGet);
            serializeJson(root, dataGet);

            isAvail_NBIoT = true;
            isAlive = true;
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
        isAliAvail = false; //isBavail = false;
    }
}

int BlinkerSerialSIM7020::print(char * data, bool needCheck)
{
    if (!isMQTTinit) return false;

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
    if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
    {
        strcat(data, _sharers[_sharerFrom]->uuid());
    }
    else
    {
        strcat(data, UUID_NBIoT);
    }
    // data_add = BLINKER_F("\",\"deviceType\":\"OwnApp\"}");

    _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
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
    
    if (connected())
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

            disconnect();
            
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


int BlinkerSerialSIM7020::toServer(char * data)
{
    if (!isMQTTinit) return false;    

    if (!isJson(STRING_format(data))) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT Publish to server..."));
    BLINKER_LOG_FreeHeap_ALL();

    bool _alive = isAlive;

    if (mqtt_NBIoT->connected())
    {
        if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, data))
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

int BlinkerSerialSIM7020::bPrint(char * name, const String & data)
{
    if (!isMQTTinit) return false;

    char data_add[1024] = { '\0' };

    strcpy(data_add, "{\"data\":");
    strcat(data_add, data.c_str());
    strcat(data_add, ",\"fromDevice\":\"");
    strcat(data_add, MQTT_ID_NBIoT);
    strcat(data_add, ",\"toDevice\":\"");
    strcat(data_add, name);
    strcat(data_add, "\",\"deviceType\":\"DiyBridge\"}");
    // String data_add = BLINKER_F("{\"data\":");

    // data_add += data;
    // data_add += BLINKER_F(",\"fromDevice\":\"");
    // data_add += MQTT_ID_MQTT;
    // data_add += BLINKER_F("\",\"toDevice\":\"");
    // data_add += name;
    // data_add += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");

    if (!isJson(STRING_format(data_add))) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT Bridge Publish..."));

    // bool _alive = isAlive;
    // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

    // if (!state) {
    //     state = (STRING_contains_string(data, BLINKER_CMD_STATE)
    //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
    // }

    if (mqtt_NBIoT->connected()) {
        // if (!state) {
        if (!checkCanBprint()) {
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        // }

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);

        // if (! iotPub.publish(payload.c_str())) {

        // String bPubTopic = BLINKER_F("");
        // char bPubTopic[60] = { '\0' };

        // if (strcmp(mqtt_broker, BLINKER_MQTT_BORKER_ONENET) == 0)
        // {
        //     // bPubTopic = MQTT_PRODUCTINFO_MQTT;
        //     // bPubTopic += BLINKER_F("/");
        //     // bPubTopic += name;
        //     // bPubTopic += BLINKER_F("/r");
        // }
        // else
        // {
            // strcpy(bPubTopic, BLINKER_PUB_TOPIC_MQTT);
            // bPubTopic = BLINKER_PUB_TOPIC_MQTT;
        // }

        if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, data_add))
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

int BlinkerSerialSIM7020::aliPrint(const String & data)
{
    if (!isMQTTinit) return false;

    char data_add[1024] = { '\0' };

    strcpy(data_add, "{\"data\":");
    strcat(data_add, data.c_str());
    strcat(data_add, ",\"fromDevice\":\"");
    strcat(data_add, MQTT_ID_NBIoT);
    strcat(data_add, "\",\"toDevice\":\"AliGenie_r\"");
    strcat(data_add, "\",\"deviceType\":\"vAssistant\"}");

    // String data_add = BLINKER_F("{\"data\":");

    // data_add += data;
    // data_add += BLINKER_F(",\"fromDevice\":\"");
    // data_add += MQTT_ID_MQTT;
    // data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    // data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(STRING_format(data_add))) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_NBIoT->connected())
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

        if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, data_add))
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

int BlinkerSerialSIM7020::duerPrint(const String & data, bool report)
{
    if (!isMQTTinit) return false;

    char data_add[1024] = { '\0' };

    strcpy(data_add, "{\"data\":");
    strcat(data_add, data.c_str());
    strcat(data_add, ",\"fromDevice\":\"");
    strcat(data_add, MQTT_ID_NBIoT);
    strcat(data_add, "\",\"toDevice\":\"DuerOS_r\"");
    strcat(data_add, "\",\"deviceType\":\"vAssistant\"}");

    // String data_add = BLINKER_F("{\"data\":");

    // data_add += data;
    // data_add += BLINKER_F(",\"fromDevice\":\"");
    // data_add += MQTT_ID_MQTT;
    // data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
    // data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(STRING_format(data_add))) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT DuerOS Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_NBIoT->connected())
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

        if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, data_add))
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

void BlinkerSerialSIM7020::aliType(const String & type)
{
    _aliType = (char*)malloc((type.length()+1)*sizeof(char));
    strcpy(_aliType, type.c_str());
    BLINKER_LOG_ALL(BLINKER_F("_aliType: "), _aliType);
}

void BlinkerSerialSIM7020::duerType(const String & type)
{
    _duerType = (char*)malloc((type.length()+1)*sizeof(char));
    strcpy(_duerType, type.c_str());
    BLINKER_LOG_ALL(BLINKER_F("_duerType: "), _duerType);
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

    AUTHKEY_NBIoT = (char*)malloc((strlen(_deviceType) + 1)*sizeof(char));
    strcpy(AUTHKEY_NBIoT, _deviceType);
}

void BlinkerSerialSIM7020::initStream(Stream& s, bool state, blinker_callback_t func)
{
    // _deviceType = _type;
    
    // BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;

    listenFunc = func;

    // streamPrint(BLINKER_CMD_AT);
    // ::delay(200);
    // streamPrint(BLINKER_CMD_CRESET_RESQ);

    // _imei = (char*)malloc(imei.length()*sizeof(char));
    // strcpy(_imei, imei.c_str());
}

int BlinkerSerialSIM7020::autoPrint(unsigned long id)
{
    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"trigged\":true,\"autoData\":{");
    payload += BLINKER_F("\"autoId\":");
    payload += STRING_format(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(MQTT_ID_NBIoT);
    payload += BLINKER_F("\",\"toDevice\":\"autoManager\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

    if (mqtt_NBIoT->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt_NBIoT->publish(BLINKER_PUB_TOPIC_NBIoT, payload.c_str()))
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

char * BlinkerSerialSIM7020::deviceName()
{
    BLINKER_LOG_ALL(BLINKER_F("BlinkerSerialSIM7020::deviceName: "), MQTT_ID_NBIoT);
    return MQTT_ID_NBIoT;/*MQTT_ID_PRO;*/
}

void BlinkerSerialSIM7020::sharers(const String & data)
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

int BlinkerSerialSIM7020::connectServer()
{
    String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    String uri = "";
    // uri += BLINKER_F("/api/v1/user/device/register?deviceType=");
    // uri += _deviceType;
    // uri += BLINKER_F("&deviceName=");
    // uri += imei;
    uri += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    uri += _deviceType;
    uri += _aliType;
    uri += _duerType;

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

void BlinkerSerialSIM7020::checkKA()
{
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerSerialSIM7020::checkAliKA() {
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerSerialSIM7020::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
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

int BlinkerSerialSIM7020::checkCanBprint() {
    if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

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

int BlinkerSerialSIM7020::checkAliPrintSpan()
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

int BlinkerSerialSIM7020::checkDuerPrintSpan()
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

int BlinkerSerialSIM7020::isJson(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("isJson: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG("Print data is not Json! ", data);
        return false;
    }
    return true;
}


#endif
