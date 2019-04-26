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

        void begin(const char* _key, const char* _deviceType, String _imei);
        void initStream(Stream& s, bool state, blinker_callback_t func);
        int deviceRegister() { return connectServer(); }

    private :
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
        char*       authKey;

        blinker_callback_t listenFunc = NULL;
};

void BlinkerSerialAIR202::begin(const char* _key, const char* _type, String _imei)
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

        authKey = (char*)malloc((_getAuthKey.length()+1)*sizeof(char));
        strcpy(authKey, _getAuthKey.c_str());

        BLINKER_LOG_ALL(BLINKER_F("===================="));
        BLINKER_LOG_ALL(BLINKER_F("authKey: "), authKey);
        BLINKER_LOG_ALL(BLINKER_F("===================="));

        _isAuthKey = true;
    }

    String url_iot = BLINKER_F("/api/v1/user/device/pro/lowpower/auth?authKey=");
    url_iot += authKey;

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
}

#endif
