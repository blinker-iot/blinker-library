#ifndef BLINKER_SERIAL_GPRS_H
#define BLINKER_SERIAL_GPRS_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"
#include "Functions/BlinkerHTTPAIR202.h"

#include "modules/ArduinoJson/ArduinoJson.h"

#if defined(ESP32)
    #include <HardwareSerial.h>

    HardwareSerial *HSerial;
#else
    #include <SoftwareSerial.h>

    SoftwareSerial *SSerial;
#endif

char*       MQTT_HOST_GPRS;
char*       MQTT_ID_GPRS;
char*       MQTT_NAME_GPRS;
char*       MQTT_KEY_GPRS;
char*       MQTT_PRODUCTINFO_GPRS;
char*       UUID_GPRS;
char*       AUTHKEY_GPRS;
char*       MQTT_DEVICEID_GPRS;
// char*       DEVICE_NAME;
char*       BLINKER_PUB_TOPIC_GPRS;
char*       BLINKER_SUB_TOPIC_GPRS;
uint16_t    MQTT_PORT_GPRS;

class BlinkerSerialGPRS : public BlinkerStream
{
    public :
        BlinkerSerialGPRS()
            : stream(NULL), isConnect(false)
        {}

        int available();
        int timedRead();
        void begin(String imei, Stream& s, bool state);
        char * lastRead() { return isFresh ? streamData : NULL; }
        void flush();
        int aliPrint(const String & s);
        int duerPrint(const String & s);
        // int print(const String & s, bool needCheck = true);
        int print(char * data, bool needCheck = true);
        int connect();//      { isConnect = true; return connected(); }
        int connected()    { return isConnect; }
        void disconnect()   { isConnect = false; }

        int deviceRegister() { return connectServer(); }

    protected :
        Stream* stream;
        char*   streamData;
        bool    isFresh = false;
        bool    isConnect;
        bool    isHWS = false;
        char*   _imei;
        uint8_t respTimes = 0;
        uint32_t    respTime = 0;
        bool    isMQTTinit = false;

        int checkPrintSpan();

        int connectServer();
};

int BlinkerSerialGPRS::available()
{
    if (!isHWS)
    {
        #if defined(__AVR__) || defined(ESP8266)
            if (!SSerial->isListening())
            {
                SSerial->listen();
                ::delay(100);
            }
        #endif
    }


    if (stream->available())
    {
        if (isFresh) free(streamData);
        streamData = (char*)malloc(1*sizeof(char));
        
        int16_t dNum = 0;
        int c_d = timedRead();
        while (dNum < BLINKER_MAX_READ_SIZE && 
            c_d >=0 && c_d != '\n')
        {
            if (c_d != '\r')
            {
                streamData[dNum] = (char)c_d;
                dNum++;
                streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
            }

            c_d = timedRead();
        }
        dNum++;
        streamData = (char*)realloc(streamData, dNum*sizeof(char));

        streamData[dNum-1] = '\0';
        stream->flush();
        
        BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData);
        BLINKER_LOG_FreeHeap_ALL();
        
        if (strlen(streamData) < BLINKER_MAX_READ_SIZE)
        {
            if (streamData[strlen(streamData) - 1] == '\r')
                streamData[strlen(streamData) - 1] = '\0';

            isFresh = true;
            return true;
        }
        else
        {
            free(streamData);
            return false;
        }
    }
    else
    {
        return false;
    }
}

void BlinkerSerialGPRS::begin(String imei, Stream& s, bool state)
{
    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;

    _imei = (char*)malloc(imei.length()*sizeof(char));
    strcpy(_imei, imei.c_str());
}

int BlinkerSerialGPRS::timedRead()
{
    int c;
    uint32_t _startMillis = millis();
    do {
        c = stream->read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < 1000);
    return -1; 
}

void BlinkerSerialGPRS::flush()
{
    if (isFresh)
    {
        free(streamData); isFresh = false;
    }
}

int BlinkerSerialGPRS::aliPrint(const String & s)
{
    if (!checkPrintSpan()) {
        respTime = millis();
        return false;
    }

    String _s = s.substring(0, s.length() - 1);
    _s += BLINKER_F(",\"toDeviceAT\":\"AliGenie\"}");

    respTime = millis();
    
    BLINKER_LOG_ALL(BLINKER_F("AliGenie Response: "), _s);
    
    if(connected()) {
        BLINKER_LOG_ALL(BLINKER_F("Succese..."));
        
        stream->println(_s);
        return true;
    }
    else {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
        
        return false;
    }
}

int BlinkerSerialGPRS::duerPrint(const String & s)
{
    if (!checkPrintSpan()) {
        respTime = millis();
        return false;
    }

    String _s = s.substring(0, s.length() - 1);
    _s += BLINKER_F(",\"toDeviceAT\":\"DuerOS\"}");

    respTime = millis();
    
    BLINKER_LOG_ALL(BLINKER_F("DuerOS Response: "), _s);
    
    if(connected()) {
        BLINKER_LOG_ALL(BLINKER_F("Succese..."));
        
        stream->println(_s);
        return true;
    }
    else {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
        
        return false;
    }
}

// int BlinkerSerialGPRS::print(const String & s, bool needCheck)
int BlinkerSerialGPRS::print(char * data, bool needCheck)
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
    
    BLINKER_LOG_ALL(BLINKER_F("Response: "), data);

    if(connected())
    {
        BLINKER_LOG_ALL(BLINKER_F("Succese..."));
        
        stream->println(data);
        return true;
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
        
        return false;
    }
}

int BlinkerSerialGPRS::checkPrintSpan()
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

int BlinkerSerialGPRS::connectServer()
{
    String host = BLINKER_F("https://iotdev.clz.me");
    String uri = BLINKER_F("/api/v1/user/device/register?deviceType=");
    uri += _deviceType;
    uri += BLINKER_F("&deviceName=");
    uri += _imei;

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), host + uri);

    BlinkerHTTPAIR202 http(stream, isHWS);

    http.begin(host, uri);

    String payload;

    if (http.GET())
    {
        payload = http.getString();

        return true;
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
    String _authKey = root[BLINKER_CMD_DETAIL][BLINKER_CMD_KEY];

    if (isMQTTinit)
    {
        free(MQTT_HOST_GPRS);
        free(MQTT_ID_GPRS);
        free(MQTT_NAME_GPRS);
        free(MQTT_KEY_GPRS);
        free(MQTT_PRODUCTINFO_GPRS);
        free(UUID_GPRS);
        free(AUTHKEY_GPRS);
        free(MQTT_DEVICEID_GPRS);
        free(BLINKER_PUB_TOPIC_GPRS);
        free(BLINKER_SUB_TOPIC_GPRS);

        isMQTTinit = false;
    }

    BLINKER_LOG_ALL(("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        String _deviceName = _userID.substring(12, 36);
        MQTT_DEVICEID_PRO = (char*)malloc((_deviceName.length()+1)*sizeof(char));
        strcpy(MQTT_DEVICEID_PRO, _deviceName.c_str());
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
        AUTHKEY_PRO = (char*)malloc((_authKey.length()+1)*sizeof(char));
        strcpy(AUTHKEY_PRO, _authKey.c_str());
        MQTT_PORT_PRO = BLINKER_MQTT_ALIYUN_PORT;

        BLINKER_LOG_ALL(("===================="));
    }

    UUID_PRO = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_PRO, _uuid.c_str());

    char uuid_eeprom[BLINKER_AUUID_SIZE];

    BLINKER_LOG_ALL(("==========AUTH CHECK=========="));

    // if (!isFirst)
    // {
    //     char _authCheck;
    //     EEPROM.begin(BLINKER_EEP_SIZE);
    //     EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
    //     if (strcmp(uuid_eeprom, _uuid.c_str()) != 0) {
    //         // strcpy(UUID_PRO, _uuid.c_str());

    //         strcpy(uuid_eeprom, _uuid.c_str());
    //         EEPROM.put(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);
    //         EEPROM.get(BLINKER_EEP_ADDR_AUUID, uuid_eeprom);

    //         BLINKER_LOG_ALL(BLINKER_F("===================="));
    //         BLINKER_LOG_ALL(BLINKER_F("uuid_eeprom: "), uuid_eeprom);
    //         BLINKER_LOG_ALL(BLINKER_F("_uuid: "), _uuid);
    //         isNew = true;
    //     }
    //     EEPROM.get(BLINKER_EEP_ADDR_AUTH_CHECK, _authCheck);
    //     if (_authCheck != BLINKER_AUTH_CHECK_DATA) {
    //         EEPROM.put(BLINKER_EEP_ADDR_AUTH_CHECK, BLINKER_AUTH_CHECK_DATA);
    //         isAuth = true;
    //     }
    //     EEPROM.commit();
    //     EEPROM.end();

    //     isFirst = true;
    // }
    
    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME: "), _imei);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_GPRS: "), MQTT_PRODUCTINFO_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_DEVICEID_GPRS: "), MQTT_DEVICEID_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_GPRS: "), MQTT_ID_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_GPRS: "), MQTT_NAME_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_GPRS: "), MQTT_KEY_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("UUID_GPRS: "), UUID_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("AUTHKEY_GPRS: "), AUTHKEY_GPRS);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        String PUB_TOPIC_STR = BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_PRODUCTINFO_GPRS;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_DEVICEID_GPRS;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_GPRS = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        strcpy(BLINKER_PUB_TOPIC_GPRS, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_GPRS: "), BLINKER_PUB_TOPIC_GPRS);
        
        String SUB_TOPIC_STR = BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_GPRS;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_DEVICEID_GPRS;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_GPRS = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        strcpy(BLINKER_SUB_TOPIC_GPRS, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_GPRS: "), BLINKER_SUB_TOPIC_GPRS);
    }
}

// int BlinkerSerialGPRS::connect()
// {
//     stream->println(STRING_format(BLINKER_CMD_MCONFIG_RESQ) +
//                     "=\"" + MQTT_ID_GPRS + 
//                     "\",\"" + MQTT_NAME_GPRS + 
//                     "\",\"" + MQTT_KEY_GPRS + "\"");
// }

#endif
