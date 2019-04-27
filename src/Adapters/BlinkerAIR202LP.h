#ifndef BLINKER_AIR202_LP_H
#define BLINKER_AIR202_LP_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"
#include "Functions/BlinkerAIR202.h"
#include "Functions/BlinkerHTTPAIR202.h"

#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerAIR202LP : public BlinkerStream
{
    public :
        BlinkerAIR202LP()
            : stream(NULL), isConnect(false)
        {}
        
        int connect() { return true; }
        int connected() { return true; }
        void disconnect() { delay(1); }
        void ping() { delay(1); }
        int available() { return true; }
        void dataGet();
        void begin(const char* _key, const char* _deviceType, String _imei);
        void initStream(Stream& s, bool state, blinker_callback_t func);
        char * deviceName() { return _deviceName; }
        char * authKey() { return _authKey; }
        int init() { return isGPRSinit; }
        int deviceRegister() { return connectServer(); }

    private :
        bool isGPRSinit = false;
        int connectServer();

    protected :
        Stream*     stream;
        char*       msgBuf;
        bool        isConnect;
        bool        isHWS = false;
        char*       imei;
        bool        isAvail = false;
        bool        _isAuthKey = false;
        const char* _vipKey;
        const char* _deviceType;
        char*       _authKey;
        char*       _deviceName;

        blinker_callback_t listenFunc = NULL;
};

void BlinkerAIR202LP::dataGet()
{
    String host = BLINKER_F("https://iotdev.clz.me");
    String uri = "";
    uri += BLINKER_F("/user/device/lowpower/data/get?deviceName=");
    uri += _deviceName;
    uri += BLINKER_F("&key=");
    uri += _authKey;

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

    DynamicJsonBuffer jsonBuffer;
    JsonObject& data_rp = jsonBuffer.parseObject(payload);

    if (data_rp.success())
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
}

void BlinkerAIR202LP::begin(const char* _key, const char* _type, String _imei)
{
    _vipKey = _key;
    _deviceType = _type;
    
    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);

    imei = (char*)malloc((_imei.length() + 1)*sizeof(char));
    strcpy(imei, _imei.c_str());

    // authKey = (char*)malloc((strlen(_deviceType) + 1)*sizeof(char));
    // strcpy(authKey, _deviceType);
}

void BlinkerAIR202LP::initStream(Stream& s, bool state, blinker_callback_t func)
{
    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;

    listenFunc = func;
}

int BlinkerAIR202LP::connectServer()
{
    String host = BLINKER_F("https://iotdev.clz.me");
    String uri = "";
    if (!_isAuthKey)
    {
        // uri += BLINKER_F("/api/v1/user/device/register?deviceType=");
        // uri += _deviceType;
        // uri += BLINKER_F("&deviceName=");
        // uri += imei;
        uri += BLINKER_F("/user/device/pro/lowpower/auth?deviceType=");
        uri += _deviceType;
        uri += BLINKER_F("&vipKey=");
        uri += _vipKey;
        uri += BLINKER_F("&deviceName=");
        uri += imei;

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

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(payload);

        if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || !root.success() ||
            !STRING_contains_string(payload, BLINKER_CMD_AUTHKEY)) {
            // while(1) {
                BLINKER_ERR_LOG(("Please make sure you have register this device!"));
                // ::delay(60000);

                return false;
            // }
        }

        String _getAuthKey = root[BLINKER_CMD_DETAIL][BLINKER_CMD_AUTHKEY];

        _authKey = (char*)malloc((_getAuthKey.length()+1)*sizeof(char));
        strcpy(_authKey, _getAuthKey.c_str());

        BLINKER_LOG_ALL(BLINKER_F("===================="));
        BLINKER_LOG_ALL(BLINKER_F("_authKey: "), _authKey);
        BLINKER_LOG_ALL(BLINKER_F("===================="));

        _isAuthKey = true;
    }

    String url_iot = BLINKER_F("/api/v1/user/device/pro/lowpower/auth?authKey=");
    url_iot += _authKey;

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), host + url_iot);

    BlinkerHTTPAIR202 http(*stream, isHWS, listenFunc);

    http.begin(host, url_iot);

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

    String _device_name = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];

    if (isGPRSinit)
    {
        free(_deviceName);

        isGPRSinit = false;
    }

    _deviceName = (char*)malloc((_device_name.length()+1)*sizeof(char));
    strcpy(_deviceName, _device_name.c_str());
    isGPRSinit = true;

    return true;
}

#endif
