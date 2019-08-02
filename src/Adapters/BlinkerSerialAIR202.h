#ifndef BLINKER_SERIAL_AIR202_H
#define BLINKER_SERIAL_AIR202_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"
#include "../Functions/BlinkerAIR202.h"
#include "../Functions/BlinkerHTTPAIR202.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
#include "../Functions/BlinkerMQTTAIR202.h"

char*       MQTT_HOST_GPRS;
char*       MQTT_ID_GPRS;
char*       MQTT_NAME_GPRS;
char*       MQTT_KEY_GPRS;
char*       MQTT_PRODUCTINFO_GPRS;
char*       UUID_GPRS;
char*       AUTHKEY_GPRS;
char*       MQTT_DEVICEID_GPRS;
char*       DEVICE_NAME_GPRS;
char*       BLINKER_PUB_TOPIC_GPRS;
char*       BLINKER_SUB_TOPIC_GPRS;
uint16_t    MQTT_PORT_GPRS;

BlinkerMQTTAIR202* mqtt_GPRS;

class BlinkerSerialAIR202 : public BlinkerStream
{
    public :
        BlinkerSerialAIR202()
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
        // int timedRead();
        char * lastRead() { if (isFresh_GPRS) return msgBuf_GPRS; return ""; }
        void flush();
        // int print(const String & s, bool needCheck = true);
        int print(char * data, bool needCheck = true);
        int bPrint(char * name, const String & data);
        int aliPrint(const String & data);
        int duerPrint(const String & data);
        void aliType(const String & type);
        void duerType(const String & type);
        void begin(const char* _deviceType, String _imei);
        void initStream(Stream& s, bool state, blinker_callback_t func);
        int autoPrint(uint32_t id);
        char * deviceName();
        char * authKey() { return AUTHKEY_GPRS; }
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
        char*       msgBuf_GPRS;
        // bool        isFresh = false;
        bool        isFresh_GPRS = false;
        bool        isConnect;
        bool        isHWS = false;
        char*       imei;
        // uint8_t     respTimes = 0;
        // uint32_t    respTime = 0;
        bool        isAvail_GPRS = false;
        uint8_t     dataFrom_GPRS = BLINKER_MSG_FROM_MQTT;

        // uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        const char* _deviceType;
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

int BlinkerSerialAIR202::connect()
{
    if (!isMQTTinit) return false;

    if (mqtt_GPRS->connected()) return true;

    disconnect();
    
    if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    {
        yield();
        return false;
    }

    BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));

    BLINKER_LOG_FreeHeap_ALL();

    if (!mqtt_GPRS->connect())
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

int BlinkerSerialAIR202::connected()
{
    if (!isMQTTinit) return false;

    return mqtt_GPRS->connected();
}

int BlinkerSerialAIR202::mConnected()
{
    if (!isMQTTinit) return false;
    else return mqtt_GPRS->connected();
}

void BlinkerSerialAIR202::disconnect()
{
    if (isMQTTinit) mqtt_GPRS->disconnect();
}

void BlinkerSerialAIR202::ping()
{
    BLINKER_LOG_ALL(BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!isMQTTinit) return;

    if (!mqtt_GPRS->connected())
    {
        disconnect();
        // delay(100);

        // connect();
    }
    else
    {
        this->latestTime = millis();
    }

    BlinkerAIR202 BLINKER_AIR202;
    BLINKER_AIR202.setStream(*stream, isHWS, listenFunc);  

    if (BLINKER_AIR202.isReboot())
    {
        BLINKER_AIR202.powerCheck();

        disconnect();
    }
}

int BlinkerSerialAIR202::available()
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

    if (isAvail_GPRS)
    {
        isAvail_GPRS = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerSerialAIR202::aligenieAvail()
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

int BlinkerSerialAIR202::duerAvail()
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

void BlinkerSerialAIR202::subscribe()
{
    if (!isMQTTinit) return;

    if (mqtt_GPRS->readSubscription())
    {
        BLINKER_LOG_ALL(BLINKER_F("Got: "), mqtt_GPRS->lastRead);

        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& root = jsonBuffer.parseObject(String(mqtt_GPRS->lastRead));
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, String(mqtt_GPRS->lastRead));
        JsonObject root = jsonBuffer.as<JsonObject>();

        String _uuid = root["fromDevice"];
        String dataGet = root["data"];
        
        BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
        BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);
        
        if (strcmp(_uuid.c_str(), UUID_GPRS) == 0)
        {
            BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));
            
            kaTime = millis();
            isAvail_GPRS = true;
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

            isAvail_GPRS = true;
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
                        BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid, \
                                    check is from bridge/share device, \
                                    data: "), dataGet);

                        _needCheckShare = true;
                    }
                }
            }
            // root.printTo(dataGet);
            serializeJson(root, dataGet);

            isAvail_GPRS = true;
            isAlive = true;
        }

        if (isFresh_GPRS) free(msgBuf_GPRS);
        msgBuf_GPRS = (char*)malloc((dataGet.length()+1)*sizeof(char));
        strcpy(msgBuf_GPRS, dataGet.c_str());
        isFresh_GPRS = true;
        
        this->latestTime = millis();

        dataFrom_GPRS = BLINKER_MSG_FROM_MQTT;
    }
}

void BlinkerSerialAIR202::flush()
{
    if (isFresh_GPRS)
    {
        free(msgBuf_GPRS); isFresh_GPRS = false; isAvail_GPRS = false;
        // isAliAvail = false; //isBavail = false;
    }
}

int BlinkerSerialAIR202::print(char * data, bool needCheck)
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
    strcat(data, MQTT_ID_GPRS);
    // strcat(data, MQTT_DEVICEID_GPRS); //PRO
    // data_add = BLINKER_F("\",\"toDevice\":\"");
    // strcat(data, data_add.c_str());
    strcat(data, "\",\"toDevice\":\"");
    if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
    {
        strcat(data, _sharers[_sharerFrom]->uuid());
    }
    else
    {
        strcat(data, UUID_GPRS);
    }
    // data_add = BLINKER_F("\",\"deviceType\":\"OwnApp\"}");

    _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    // strcat(data, data_add.c_str());
    strcat(data, "\",\"deviceType\":\"OwnApp\"}");

    // data_add = STRING_format(data);

    if (!isJson(STRING_format(data))) return false;

    // data_add.replace("\"", "\\22");
    // strcpy(data, data_add.c_str());
    uint16_t d_data_len;

    for (uint16_t d_num = 0; d_num < 1024; d_num++)
    {
        if (data[d_num] == '\"')
        {
            data[d_num] = '\\';
            d_data_len = strlen(data);

            // BLINKER_LOG_ALL(BLINKER_F("d_num: "), d_num,
            //                 BLINKER_F(", d_data_len: "), d_data_len);

            for(uint16_t c_num = d_data_len; c_num > d_num; c_num--)
            {
                data[c_num + 2] = data[c_num];
            }
            data[d_num + 1] = '2';
            data[d_num + 2] = '2';
        }
    }

    // #if defined(ESP8266)
        // data_add = "";
    // #endif

    // if (!isJson(STRING_format(data)) return false;

    // strcpy(data, STRING_format(data).replace("\"", "\\22").c_str());

    // String msg_data = STRING_format(data);

    // msg_data.replace("\"", "\\22");
    
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
    
    if (mqtt_GPRS->connected())
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
        // if (! mqtt_GPRS->publish(BLINKER_PUB_TOPIC_GPRS, msg_data.c_str()))
        if (! mqtt_GPRS->publish(BLINKER_PUB_TOPIC_GPRS, data))
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

int BlinkerSerialAIR202::bPrint(char * name, const String & data)
{
    if (!isMQTTinit) return false;

    char data_add[1024] = { '\0' };

    strcpy(data_add, "{\"data\":");
    strcat(data_add, data.c_str());
    strcat(data_add, ",\"fromDevice\":\"");
    strcat(data_add, MQTT_ID_GPRS);
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

    if (mqtt_GPRS->connected()) {
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

        if (! mqtt_GPRS->publish(BLINKER_PUB_TOPIC_GPRS, data_add))
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

int BlinkerSerialAIR202::aliPrint(const String & data)
{
    if (!isMQTTinit) return false;

    char data_add[1024] = { '\0' };

    strcpy(data_add, "{\"data\":");
    strcat(data_add, data.c_str());
    strcat(data_add, ",\"fromDevice\":\"");
    strcat(data_add, MQTT_ID_GPRS);
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

    if (mqtt_GPRS->connected())
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

        if (! mqtt_GPRS->publish(BLINKER_PUB_TOPIC_GPRS, data_add))
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

int BlinkerSerialAIR202::duerPrint(const String & data)
{
    if (!isMQTTinit) return false;

    char data_add[1024] = { '\0' };

    strcpy(data_add, "{\"data\":");
    strcat(data_add, data.c_str());
    strcat(data_add, ",\"fromDevice\":\"");
    strcat(data_add, MQTT_ID_GPRS);
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

    if (mqtt_GPRS->connected())
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

        if (! mqtt_GPRS->publish(BLINKER_PUB_TOPIC_GPRS, data_add))
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

void BlinkerSerialAIR202::aliType(const String & type)
{
    _aliType = (char*)malloc((type.length()+1)*sizeof(char));
    strcpy(_aliType, type.c_str());
    BLINKER_LOG_ALL(BLINKER_F("_aliType: "), _aliType);
}

void BlinkerSerialAIR202::duerType(const String & type)
{
    _duerType = (char*)malloc((type.length()+1)*sizeof(char));
    strcpy(_duerType, type.c_str());
    BLINKER_LOG_ALL(BLINKER_F("_duerType: "), _duerType);
}

void BlinkerSerialAIR202::begin(const char* _type, String _imei)
{
    _deviceType = _type;
    
    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    // stream = &s;
    // stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    // isHWS = state;

    imei = (char*)malloc((_imei.length() + 1)*sizeof(char));
    strcpy(imei, _imei.c_str());

    AUTHKEY_GPRS = (char*)malloc((strlen(_deviceType) + 1)*sizeof(char));
    strcpy(AUTHKEY_GPRS, _deviceType);
}

void BlinkerSerialAIR202::initStream(Stream& s, bool state, blinker_callback_t func)
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

int BlinkerSerialAIR202::autoPrint(uint32_t id)
{
    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"trigged\":true,\"autoData\":{");
    payload += BLINKER_F("\"autoId\":");
    payload += STRING_format(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(MQTT_ID_GPRS);
    payload += BLINKER_F("\",\"toDevice\":\"autoManager\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

    if (mqtt_GPRS->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT, BLINKER_PUB_TOPIC_MQTT);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt_GPRS->publish(BLINKER_PUB_TOPIC_GPRS, payload.c_str()))
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

char * BlinkerSerialAIR202::deviceName() { return MQTT_ID_GPRS;/*MQTT_ID_PRO;*/ }

void BlinkerSerialAIR202::sharers(const String & data)
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

        if (user_name.length() == BLINKER_MQTT_USER_UUID_SIZE)
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

int BlinkerSerialAIR202::connectServer()
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

    BlinkerHTTPAIR202 http(*stream, isHWS, listenFunc);

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
        free(MQTT_HOST_GPRS);
        free(MQTT_ID_GPRS);
        free(MQTT_NAME_GPRS);
        free(MQTT_KEY_GPRS);
        free(MQTT_PRODUCTINFO_GPRS);
        free(UUID_GPRS);
        free(DEVICE_NAME_GPRS);
        free(BLINKER_PUB_TOPIC_GPRS);
        free(BLINKER_SUB_TOPIC_GPRS);
        free(mqtt_GPRS);
        // free(iotSub_GPRS);

        isMQTTinit = false;
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
        DEVICE_NAME_GPRS = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_GPRS, _userID.c_str());
        MQTT_ID_GPRS = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_GPRS, _userID.c_str());
        MQTT_NAME_GPRS = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_GPRS, _userName.c_str());
        MQTT_KEY_GPRS = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_GPRS, _key.c_str());
        MQTT_PRODUCTINFO_GPRS = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_GPRS, _productInfo.c_str());
        MQTT_HOST_GPRS = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_GPRS, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT_GPRS = BLINKER_MQTT_ALIYUN_PORT;
    }

    UUID_GPRS = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_GPRS, _uuid.c_str());

    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME_GPRS: "), DEVICE_NAME_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_GPRS: "), MQTT_PRODUCTINFO_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_GPRS: "), MQTT_ID_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_GPRS: "), MQTT_NAME_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_GPRS: "), MQTT_KEY_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("UUID_GPRS: "), UUID_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        String PUB_TOPIC_STR = BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_PRODUCTINFO_GPRS;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_ID_GPRS;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_GPRS = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_GPRS, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_GPRS, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_GPRS: "), BLINKER_PUB_TOPIC_GPRS);

        String SUB_TOPIC_STR = BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_GPRS;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_ID_GPRS;
        SUB_TOPIC_STR += BLINKER_F("/r");

        BLINKER_SUB_TOPIC_GPRS = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_GPRS, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_GPRS, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_GPRS: "), BLINKER_SUB_TOPIC_GPRS);
    }

    // String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
    // String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
    // String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
    // String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
    // String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
    // String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];
    // String _authKey = root[BLINKER_CMD_DETAIL][BLINKER_CMD_KEY];

    // if (isMQTTinit)
    // {
    //     free(MQTT_HOST_GPRS);
    //     free(MQTT_ID_GPRS);
    //     free(MQTT_NAME_GPRS);
    //     free(MQTT_KEY_GPRS);
    //     free(MQTT_PRODUCTINFO_GPRS);
    //     free(UUID_GPRS);
    //     free(AUTHKEY_GPRS);
    //     free(MQTT_DEVICEID_GPRS);
    //     free(BLINKER_PUB_TOPIC_GPRS);
    //     free(BLINKER_SUB_TOPIC_GPRS);
    //     free(mqtt_GPRS);

    //     isMQTTinit = false;
    // }

    // BLINKER_LOG_ALL(("===================="));

    // // if (_broker == "BLINKER_MQTT_BORKER_ALIYUN") {
    //     // memcpy(DEVICE_NAME, _userID.c_str(), 12);
    //     String _deviceName = _userID.substring(12, 36);
    //     MQTT_DEVICEID_GPRS = (char*)malloc((_deviceName.length()+1)*sizeof(char));
    //     strcpy(MQTT_DEVICEID_GPRS, _deviceName.c_str());
    //     MQTT_ID_GPRS = (char*)malloc((_userID.length()+1)*sizeof(char));
    //     strcpy(MQTT_ID_GPRS, _userID.c_str());
    //     MQTT_NAME_GPRS = (char*)malloc((_userName.length()+1)*sizeof(char));
    //     strcpy(MQTT_NAME_GPRS, _userName.c_str());
    //     MQTT_KEY_GPRS = (char*)malloc((_key.length()+1)*sizeof(char));
    //     strcpy(MQTT_KEY_GPRS, _key.c_str());
    //     MQTT_PRODUCTINFO_GPRS = (char*)malloc((_productInfo.length()+1)*sizeof(char));
    //     strcpy(MQTT_PRODUCTINFO_GPRS, _productInfo.c_str());
    //     MQTT_HOST_GPRS = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
    //     strcpy(MQTT_HOST_GPRS, BLINKER_MQTT_ALIYUN_HOST);
    //     AUTHKEY_GPRS = (char*)malloc((_authKey.length()+1)*sizeof(char));
    //     strcpy(AUTHKEY_GPRS, _authKey.c_str());
    //     MQTT_PORT_GPRS = BLINKER_MQTT_ALIYUN_PORT;

    //     BLINKER_LOG_ALL(("===================="));
    // // }

    // UUID_GPRS = (char*)malloc((_uuid.length()+1)*sizeof(char));
    // strcpy(UUID_GPRS, _uuid.c_str());

    // char uuid_eeprom[BLINKER_AUUID_SIZE];

    // BLINKER_LOG_ALL(("==========AUTH CHECK=========="));

    // // if (!isFirst)
    // // {
    // //     char _authCheck;
    // //     EEPROM.begin(BLINKER_EEP_SIZE);
    // //     EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
    // //     if (strcmp(uuid_eeprom, _uuid.c_str()) != 0) {
    // //         // strcpy(UUID_PRO, _uuid.c_str());

    // //         strcpy(uuid_eeprom, _uuid.c_str());
    // //         EEPROM.put(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
    // //         EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);

    // //         BLINKER_LOG_ALL(BLINKER_F("===================="));
    // //         BLINKER_LOG_ALL(BLINKER_F("uuid_eeprom: "), uuid_eeprom);
    // //         BLINKER_LOG_ALL(BLINKER_F("_uuid: "), _uuid);
    // //         isNew = true;
    // //     }
    // //     EEPROM.get(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
    // //     if (_authCheck != BLINKER_AUTH_CHECK_DATA) {
    // //         EEPROM.put(BLINKER_EEP_ADDR_AUTH_CHECK, BLINKER_AUTH_CHECK_DATA);
    // //         isAuth = true;
    // //     }
    // //     EEPROM.commit();
    // //     EEPROM.end();

    // //     isFirst = true;
    // // }
    
    // BLINKER_LOG_ALL(BLINKER_F("===================="));
    // BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME: "), imei);
    // BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_GPRS: "), MQTT_PRODUCTINFO_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("MQTT_DEVICEID_GPRS: "), MQTT_DEVICEID_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_GPRS: "), MQTT_ID_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_GPRS: "), MQTT_NAME_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_GPRS: "), MQTT_KEY_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    // BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("UUID_GPRS: "), UUID_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("AUTHKEY_GPRS: "), AUTHKEY_GPRS);
    // BLINKER_LOG_ALL(BLINKER_F("===================="));

    // // if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
    //     String PUB_TOPIC_STR = BLINKER_F("/");
    //     PUB_TOPIC_STR += MQTT_PRODUCTINFO_GPRS;
    //     PUB_TOPIC_STR += BLINKER_F("/");
    //     PUB_TOPIC_STR += MQTT_DEVICEID_GPRS;
    //     PUB_TOPIC_STR += BLINKER_F("/s");

    //     BLINKER_PUB_TOPIC_GPRS = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
    //     strcpy(BLINKER_PUB_TOPIC_GPRS, PUB_TOPIC_STR.c_str());
        
    //     BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_GPRS: "), BLINKER_PUB_TOPIC_GPRS);
        
    //     String SUB_TOPIC_STR = BLINKER_F("/");
    //     SUB_TOPIC_STR += MQTT_PRODUCTINFO_GPRS;
    //     SUB_TOPIC_STR += BLINKER_F("/");
    //     SUB_TOPIC_STR += MQTT_DEVICEID_GPRS;
    //     SUB_TOPIC_STR += BLINKER_F("/r");
        
    //     BLINKER_SUB_TOPIC_GPRS = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
    //     strcpy(BLINKER_SUB_TOPIC_GPRS, SUB_TOPIC_STR.c_str());
        
    //     BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_GPRS: "), BLINKER_SUB_TOPIC_GPRS);
    // // }

    // if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        mqtt_GPRS = new BlinkerMQTTAIR202(*stream, isHWS, MQTT_HOST_GPRS, MQTT_PORT_GPRS, 
                                        MQTT_ID_GPRS, MQTT_NAME_GPRS, MQTT_KEY_GPRS, listenFunc);
    // }

    this->latestTime = millis();
    isMQTTinit = true;
    mqtt_GPRS->subscribe(BLINKER_SUB_TOPIC_GPRS);

    return true;
}

void BlinkerSerialAIR202::checkKA()
{
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerSerialAIR202::checkAliKA() {
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerSerialAIR202::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerSerialAIR202::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_PRO_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

int BlinkerSerialAIR202::checkCanBprint() {
    if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

        return false;
    }
}

int BlinkerSerialAIR202::checkPrintSpan()
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

int BlinkerSerialAIR202::checkAliPrintSpan()
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

int BlinkerSerialAIR202::checkDuerPrintSpan()
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

int BlinkerSerialAIR202::isJson(const String & data)
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
