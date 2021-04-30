#ifndef BLINKER_HTTP_H
#define BLINKER_HTTP_H

#if (defined(ESP8266) || defined(ESP32)) && \
    (defined(BLINKER_WIFI) || defined(BLINKER_PRO_ESP))

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266WiFiMulti.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <WiFiMulti.h>
    #include <HTTPClient.h>
#endif

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerUtility.h"

#if defined(ESP8266)
    #ifndef BLINKER_WITHOUT_SSL
        BearSSL::WiFiClientSecure   client_mqtt;
    #else
        WiFiClient               client_mqtt;
    #endif
#elif defined(ESP32)
    WiFiClientSecure     client_s;
#endif

static uint32_t    _smsTime = 0;
static uint32_t    _pushTime = 0;
static uint32_t    _wechatTime = 0;
static uint32_t    _weatherTime = 0;
static uint32_t    _weather_forecast_Time = 0;
static uint32_t    _aqiTime = 0;
static uint32_t    _logTime = 0;
static uint32_t    _codTime = 0;

static uint8_t     _serverTimes = 0;
static uint32_t    _serverTime = 0;

static bool checkServerLimit()
{
    if ((millis() - _serverTime) < 60 * 60 * 1000)
    {
        if (_serverTimes > BLINKER_PRINT_MSG_LIMIT)
        {
            BLINKER_ERR_LOG(BLINKER_F("SERVER NOT ALIVE OR MSG LIMIT"));

            return false;
        }
        else
        {
            _serverTimes++;
            return true;
        }
    }
    else
    {
        _serverTimes = 0;
        _serverTime = millis();
        return true;
    }
}

static bool checkSMS()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _smsTime) >= BLINKER_SMS_MSG_LIMIT || \
        _smsTime == 0) return true;
    else return false;
}


static bool checkPUSH()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _pushTime) >= BLINKER_PUSH_MSG_LIMIT || \
        _pushTime == 0) return true;
    else return false;
}


static bool checkWECHAT()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _wechatTime) >= BLINKER_WECHAT_MSG_LIMIT || \
        _wechatTime == 0) return true;
    else return false;
}


static bool checkWEATHER()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _weatherTime) >= BLINKER_WEATHER_MSG_LIMIT || \
        _weatherTime == 0) return true;
    else return false;
}    


static bool checkWEATHERFORECAST()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _weather_forecast_Time) >= BLINKER_WEATHER_MSG_LIMIT || \
        _weather_forecast_Time == 0) return true;
    else return false;
}


static bool checkAQI()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _aqiTime) >= BLINKER_AQI_MSG_LIMIT || \
        _aqiTime == 0) return true;
    else return false;
}


static bool checkLOG()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _logTime) >= BLINKER_LOG_MSG_LIMIT || \
        _logTime == 0) return true;
    else return false;
}

static bool checkCOD()
{
    if (!checkServerLimit()) return false;

    if ((millis() - _codTime) >= BLINKER_COD_MSG_LIMIT || \
        _codTime == 0) return true;
    else return false;
}

String httpToServer(uint8_t _type, const String & msg, bool state = false)
{
    switch (_type)
    {
        case BLINKER_CMD_SMS_NUMBER :
            if (!checkSMS()) {
                return BLINKER_CMD_FALSE;
            }

            if ((!state && msg.length() > BLINKER_SMS_MAX_SEND_SIZE) ||
                (state && msg.length() > BLINKER_SMS_MAX_SEND_SIZE + 15)) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_PUSH_NUMBER :
            if (!checkPUSH()) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_WECHAT_NUMBER :
            if (!checkWECHAT()) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_WEATHER_NUMBER :
            if (!checkWEATHER()) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_WEATHER_FORECAST_NUMBER :
            if (!checkWEATHERFORECAST()) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_AQI_NUMBER :
            if (!checkAQI()) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_LOG_NUMBER :
            if (!checkLOG()) {
                return BLINKER_CMD_FALSE;
            }
            break;
        case BLINKER_CMD_DATA_STORAGE_NUMBER :
            break;
        case BLINKER_CMD_TIME_SLOT_DATA_NUMBER :
            break;
        case BLINKER_CMD_TEXT_DATA_NUMBER :
            break;
        case BLINKER_CMD_JSON_DATA_NUMBER :
            break;
        case BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER :
            break;
        case BLINKER_CMD_WIFI_AUTH_NUMBER :
            break;
        case BLINKER_CMD_PRO_AUTH_GET_NUMBER :
            break;
        case BLINKER_CMD_PRO_AUTH_NUMBER :
            break;
        case BLINKER_CMD_FRESH_SHARERS_NUMBER :
            break;
        case BLINKER_CMD_OTA_NUMBER :
            break;
        case BLINKER_CMD_OTA_STATUS_NUMBER :
            break;
        default :
            return BLINKER_CMD_FALSE;
    }

    #if defined(ESP8266)
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.stop();
            
            std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

            client_s->setInsecure();
        #else
            WiFiClient               client_s;
        #endif
    #endif

    HTTPClient http;

    String url_iot;

    int httpCode;

    String host = BLINKER_F(BLINKER_SERVER_HTTPS);

    String conType = BLINKER_F("Content-Type");
    String application = BLINKER_F("application/json;charset=utf-8");

    BLINKER_LOG_ALL(BLINKER_F("blinker server begin"));
    BLINKER_LOG_FreeHeap_ALL();

    switch (_type)
    {
        case BLINKER_CMD_SMS_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/sms");

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
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
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_WECHAT_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/wxMsg/");

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_WEATHER_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v3");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_WEATHER_FORECAST_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v3");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_AQI_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v3");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_LOG_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/cloud_storage/logs");

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_DATA_STORAGE_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/cloudStorage/");

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_TIME_SLOT_DATA_NUMBER :
            // url_iot = host;
            #ifndef BLINKER_WITHOUT_SSL
                url_iot = BLINKER_F("https://storage.diandeng.tech/api/v1/storage/ts");
            #else
                url_iot = BLINKER_F("http://storage.diandeng.tech/api/v1/storage/ts");
            #endif

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_TEXT_DATA_NUMBER :
            // url_iot = host;
            #ifndef BLINKER_WITHOUT_SSL
                url_iot = BLINKER_F("https://storage.diandeng.tech/api/v1/storage/tt");
            #else
                url_iot = BLINKER_F("http://storage.diandeng.tech/api/v1/storage/tt");
            #endif

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            http.addHeader(conType, application);
            httpCode = http.POST(msg);
            break;
        case BLINKER_CMD_JSON_DATA_NUMBER :
            // url_iot = host;
            #ifndef BLINKER_WITHOUT_SSL
                url_iot = BLINKER_F("https://storage.diandeng.tech/api/v1/storage/ot");
            #else
                url_iot = BLINKER_F("http://storage.diandeng.tech/api/v1/storage/ot");
            #endif

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
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
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_WIFI_AUTH_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_PRO_AUTH_GET_NUMBER :
            url_iot = host;
            // url_iot += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_PRO_AUTH_NUMBER :
            url_iot = host;
            // url_iot += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_FRESH_SHARERS_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device");
            url_iot += msg;

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
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
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
            #else
                http.begin(url_iot);
            #endif

            httpCode = http.GET();
            break;
        case BLINKER_CMD_OTA_STATUS_NUMBER :
            url_iot = host;
            url_iot += BLINKER_F("/api/v1/user/device/ota/upgrade_status");

            #if defined(ESP8266)
                #ifndef BLINKER_WITHOUT_SSL
                http.begin(*client_s, url_iot);
                #else
                http.begin(client_s, url_iot);
                #endif
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
        if (httpCode == HTTP_CODE_OK || httpCode == 201)
        {
            payload = http.getString();

            BLINKER_LOG_ALL(payload);

            DynamicJsonDocument jsonBuffer(2048);
            DeserializationError error = deserializeJson(jsonBuffer, payload);
            JsonObject data_rp = jsonBuffer.as<JsonObject>();

            // if (data_rp.success())
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
                    switch (_type)
                    {
                        default:
                            payload = data_rp[BLINKER_CMD_DETAIL].as<String>();
                            break;
                    }

                    BLINKER_LOG_ALL(BLINKER_F("_type: "), _type);
                }
            }

            BLINKER_LOG_ALL(BLINKER_F("payload: "), payload);

            switch (_type)
            {
                case BLINKER_CMD_SMS_NUMBER :
                    _smsTime = millis();
                    break;
                case BLINKER_CMD_PUSH_NUMBER :
                    _pushTime = millis();
                    break;
                case BLINKER_CMD_WECHAT_NUMBER :
                    _wechatTime = millis();
                    break;
                case BLINKER_CMD_WEATHER_NUMBER :
                    _weatherTime = millis();
                    break;
                case BLINKER_CMD_WEATHER_FORECAST_NUMBER :
                    _weather_forecast_Time = millis();
                    break;
                case BLINKER_CMD_AQI_NUMBER :
                    _aqiTime = millis();
                    break;
                case BLINKER_CMD_LOG_NUMBER :
                    _logTime = millis();
                    break;
                case BLINKER_CMD_DATA_STORAGE_NUMBER :
                    break;
                case BLINKER_CMD_TIME_SLOT_DATA_NUMBER :
                    break;
                case BLINKER_CMD_TEXT_DATA_NUMBER :
                    break;
                case BLINKER_CMD_JSON_DATA_NUMBER :
                    break;
                case BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER :
                    break;
                case BLINKER_CMD_WIFI_AUTH_NUMBER :
                    break;
                case BLINKER_CMD_PRO_AUTH_GET_NUMBER :
                    break;
                case BLINKER_CMD_PRO_AUTH_NUMBER :
                    break;
                case BLINKER_CMD_FRESH_SHARERS_NUMBER :
                    break;
                case BLINKER_CMD_OTA_NUMBER :
                    break;
                case BLINKER_CMD_OTA_STATUS_NUMBER :
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

        if (_type == BLINKER_CMD_SMS_NUMBER) {
            _smsTime = millis();
        }
        
        http.end();

        return BLINKER_CMD_FALSE;
    }
}

#endif

#endif
