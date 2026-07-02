#ifndef BLINKER_HTTP_H
#define BLINKER_HTTP_H

#include "../Blinker/BlinkerWiFiPlatform.h"

#if defined(BLINKER_NATIVE_WIFI)

#define BLINKER_HTTP

// #include "../modules/WebSockets/WebSocketsServer.h"
// #include "../modules/mqtt/Adafruit_HTTP.h"
// #include "../modules/mqtt/Adafruit_HTTP_Client.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "Adapters/BlinkerHTTP.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

enum b_config_t {
    COMM,
    BLINKER_SMART_CONFIG,
};

enum b_configStatus_t {
    SMART_BEGIN,
    SMART_DONE,
    SMART_TIMEOUT,
};

BlinkerWiFiSecureClient client_s;

WiFiClient               client;

class BlinkerHTTP : public BlinkerStream
{
    public :
        // BlinkerHTTP() : {}

        int connect();
        int connected();
        void disconnect();
        int available();
        void subscribe();
        char * lastRead();
        void flush();
        int print(char * data, bool needCheck = true);
        void begin(const char* auth);
        bool begin();
        // int autoPrint(unsigned long id); // TBD
        char * deviceName();
        char * authKey() { return _authKey; }
        int init() { if (!isInit) checkInit(); return isInit; }
        int reRegister() { return connectServer(); }

        bool checkInit();
        void commonBegin(const char* _ssid, const char* _pswd);
        void smartconfigBegin();
        bool autoInit();
        void smartconfig();
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);

    private :
        bool isInit = false;

        int connectServer();
        void parseData(const char* data);

    protected :
        bool        _isWiFiInit = false;
        bool        _isBegin = false;
        b_config_t  _configType = COMM;
        b_configStatus_t _configStatus = SMART_BEGIN;
        uint32_t    _connectTime = 0;
        uint8_t     _connectTimes = 0;

        char*       _authKey;

        char*       MQTT_ID_HTTP;
        char*       MQTT_NAME_HTTP;
        char*       MQTT_KEY_HTTP;
        char*       UUID_HTTP;
        char*       DEVICE_NAME_HTTP;

        char*       msgBuf_HTTP;
        bool        isFresh_HTTP = false;
        bool        isConnect_HTTP = false;
        bool        isAvail_HTTP = false;
        uint32_t    _reRegister_time = 0;
        uint8_t     _reRegister_times = 0;
};

int BlinkerHTTP::connect()
{
    if (isInit) return true;

    if (begin())
    {
        return true;
    }
    else
    {
        return false;
    }
}

int BlinkerHTTP::connected()
{
    return isConnect_HTTP;
}

void BlinkerHTTP::disconnect()
{
    isConnect_HTTP = false;
}

int BlinkerHTTP::available()
{
    if (!isConnect_HTTP) return false;

    // TBD

    if (isAvail_HTTP)
    {
        isAvail_HTTP = false;
        return true;
    }
    else {
        return false;
    }
}

void BlinkerHTTP::subscribe()
{
    if (!isConnect_HTTP) return;

    // TBD

    
    const int httpsPort = 443;
#if 0
    String host = BLINKER_F(BLINKER_SERVER_HOST);
    String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");



    #ifndef BLINKER_WITHOUT_SSL

        std::unique_ptr<WiFiClientSecure>client_s(new WiFiClientSecure);

        // client_s->setFingerprint(fingerprint);
        client_s->setInsecure();
    #else
        WiFiClient               client_s;
    #endif

    String url_iot = BLINKER_F("/api/v1/user/device/http/sub?token=");
    url_iot += STRING_format(MQTT_KEY_HTTP);
    // url_iot += BLINKER_VERSION;

    #ifndef BLINKER_WITHOUT_SSL
        url_iot = "https://" + host + url_iot;
    #else
        url_iot = "http://" + host + url_iot;
    #endif
    HTTPClient http;

    String payload;

    BLINKER_LOG_ALL(BLINKER_F("[HTTP] begin: "), url_iot);

    #ifndef BLINKER_WITHOUT_SSL
    if (http.begin(*client_s, url_iot)) {  // HTTPS
    #else
    if (http.begin(client_s, url_iot)) {
    #endif

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
    String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    // const char* ca =
    //     "-----BEGIN CERTIFICATE-----\n"
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n"
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n"
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n"
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n"
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n"
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n"
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n"
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n"
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n"
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n"
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n"
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n"
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n"
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n"
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n"
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n"
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n"
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n"
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n"
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n"
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n"
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n"
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n"
    //     "9UBd0Q==\n"
    //     "-----END CERTIFICATE-----\n";
// #endif

    HTTPClient http;

    String url_iot = host;
    url_iot += BLINKER_F("/api/v1/user/device/http/sub?token=");
    url_iot += STRING_format(MQTT_KEY_HTTP);

// #endif

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

// #if 0
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

    // payload = "";

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
}

char * BlinkerHTTP::lastRead()
{
    if (isFresh_HTTP) return msgBuf_HTTP;
    else return "";
}

void BlinkerHTTP::flush()
{
    if (isFresh_HTTP)
    {
        free(msgBuf_HTTP);
        isFresh_HTTP = false;
        isAvail_HTTP = false;
    }
}

int BlinkerHTTP::print(char * data, bool needCheck)
{
    if (!isConnect_HTTP) return false;

    // TBD
    String msg = BLINKER_F("{\"token\":\"");
    msg += STRING_format(MQTT_KEY_HTTP);
    msg += BLINKER_F("\",\"toDevice\":\"");
    msg += STRING_format(UUID_HTTP);
    msg += BLINKER_F("\",\"deviceType\":\"OwnApp\",\"data\":");
    msg += STRING_format(data);
    msg += BLINKER_F("}");

    String conType = BLINKER_F("Content-Type");
    String application = BLINKER_F("application/json;charset=utf-8");
    
    const int httpsPort = 443;
#if 0
    String host = BLINKER_F(BLINKER_SERVER_HOST);
    String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");



    #ifndef BLINKER_WITHOUT_SSL

        std::unique_ptr<WiFiClientSecure>client_s(new WiFiClientSecure);

        // client_s->setFingerprint(fingerprint);
        client_s->setInsecure();
    #else
        WiFiClient               client_s;
    #endif

    String url_iot = BLINKER_F("/api/v1/user/device/http/pub");
    // url_iot += STRING_format(MQTT_KEY_HTTP);
    // url_iot += BLINKER_VERSION;

    #ifndef BLINKER_WITHOUT_SSL
        url_iot = "https://" + host + url_iot;
    #else
        url_iot = "http://" + host + url_iot;
    #endif
    HTTPClient http;

    String payload;

    BLINKER_LOG_ALL(BLINKER_F("[HTTP] begin: "), url_iot);

    #ifndef BLINKER_WITHOUT_SSL
    if (http.begin(*client_s, url_iot)) {  // HTTPS
    #else
    if (http.begin(client_s, url_iot)) {
    #endif

        // Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        http.addHeader(conType, application);
        int httpCode = http.POST(msg);

        BLINKER_LOG_ALL(BLINKER_F("[HTTP] POST: "), msg);

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled

            BLINKER_LOG_ALL(BLINKER_F("[HTTP] POST... code: "), httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                payload = http.getString();
                // Serial.println(payload);
            }
        } else {
            BLINKER_LOG(BLINKER_F("[HTTP] POST... failed, error: "), http.errorToString(httpCode).c_str());
            payload = http.getString();
            BLINKER_LOG(payload);
        }

        http.end();
    } else {
        // Serial.printf("[HTTPS] Unable to connect\n");
    }

#elif defined(ESP32)
    String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    // const char* ca =
    //     "-----BEGIN CERTIFICATE-----\n"
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n"
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n"
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n"
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n"
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n"
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n"
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n"
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n"
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n"
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n"
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n"
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n"
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n"
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n"
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n"
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n"
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n"
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n"
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n"
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n"
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n"
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n"
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n"
    //     "9UBd0Q==\n"
    //     "-----END CERTIFICATE-----\n";
// #endif

    HTTPClient http;

    String url_iot = host;
    url_iot += BLINKER_F("/api/v1/user/device/http/pub");
    url_iot += STRING_format(MQTT_KEY_HTTP);

// #endif

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

// #if 0
//     http.begin(url_iot, fingerprint); //HTTP
// #elif defined(ESP32)
    // http.begin(url_iot, ca); TODO
    http.begin(url_iot);
// #endif
    http.addHeader(conType, application);
    int httpCode = http.POST(msg);

    BLINKER_LOG_ALL(BLINKER_F("[HTTP] POST: "), msg);

    String payload;

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled

        BLINKER_LOG_ALL(BLINKER_F("[HTTP] POST... code: "), httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            // BLINKER_LOG(payload);
        }
    }
    else {
        BLINKER_LOG(BLINKER_F("[HTTP] POST... failed, error: "), http.errorToString(httpCode).c_str());
        payload = http.getString();
        BLINKER_LOG(payload);
    }

    http.end();
#endif

    // payload = "";

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
}

void BlinkerHTTP::begin(const char* auth)
{
    // if (!checkInit()) return;
    // _authKey = auth;
    _authKey = (char*)malloc((strlen(auth)+1)*sizeof(char));
    strcpy(_authKey, auth);

    BLINKER_LOG_ALL(BLINKER_F("_authKey: "), auth);
}

bool BlinkerHTTP::begin()
{
    if (!checkInit()) return false;

    if (reRegister())
    {
        return true;
    }
    else
    {
        return false;
    }
}

char * BlinkerHTTP::deviceName() { return DEVICE_NAME_HTTP;/*MQTT_ID_HTTP;*/ }

int BlinkerHTTP::connectServer()
{
    const int httpsPort = 443;
#if 0
    String host = BLINKER_F(BLINKER_SERVER_HOST);
    String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");



    #ifndef BLINKER_WITHOUT_SSL

        std::unique_ptr<WiFiClientSecure>client_s(new WiFiClientSecure);

        // client_s->setFingerprint(fingerprint);
        client_s->setInsecure();
    #else
        WiFiClient               client_s;
    #endif

    String url_iot = BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    url_iot += _authKey;
    url_iot += BLINKER_F("&version=");
    url_iot += BLINKER_VERSION;
    #ifndef BLINKER_WITHOUT_SSL
    url_iot += BLINKER_F("&protocol=https");
    #else
    url_iot += BLINKER_F("&protocol=http");
    #endif
    // url_iot += BLINKER_VERSION;

    #ifndef BLINKER_WITHOUT_SSL
        url_iot = "https://" + host + url_iot;
    #else
        url_iot = "http://" + host + url_iot;
    #endif
    HTTPClient http;

    String payload;

    BLINKER_LOG_ALL(BLINKER_F("[HTTP] begin: "), url_iot);

    #ifndef BLINKER_WITHOUT_SSL
    if (http.begin(*client_s, url_iot)) {  // HTTPS
    #else
    if (http.begin(client_s, url_iot)) {
    #endif

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
    String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    // const char* ca =
    //     "-----BEGIN CERTIFICATE-----\n"
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n"
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n"
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n"
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n"
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n"
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n"
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n"
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n"
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n"
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n"
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n"
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n"
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n"
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n"
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n"
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n"
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n"
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n"
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n"
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n"
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n"
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n"
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n"
    //     "9UBd0Q==\n"
    //     "-----END CERTIFICATE-----\n";
// #endif

    HTTPClient http;

    String url_iot = host;
    url_iot += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    url_iot += _authKey;
    url_iot += BLINKER_F("&version=");
    url_iot += BLINKER_VERSION;
    url_iot += BLINKER_F("&protocol=https");

// #endif

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

// #if 0
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

    // payload = "";

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(payload);
    JsonDocument jsonBuffer;
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG(BLINKER_F("Maybe you have put in the wrong AuthKey!"));
            BLINKER_ERR_LOG(BLINKER_F("Or maybe the server rejected your request!"));
            BLINKER_ERR_LOG(BLINKER_F("Or maybe your network is disconnected!"));
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
    String _host = root[BLINKER_CMD_DETAIL]["host"];
    uint32_t _port = root[BLINKER_CMD_DETAIL]["port"];
    uint8_t _num = _host.indexOf("://");

    if (isInit)
    {
        free(MQTT_ID_HTTP);
        free(MQTT_NAME_HTTP);
        free(MQTT_KEY_HTTP);
        free(UUID_HTTP);
        free(DEVICE_NAME_HTTP);

        isInit = false;
    }

    // if (_broker == BLINKER_HTTP_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME_HTTP, _userID.c_str(), 12);
        DEVICE_NAME_HTTP = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_HTTP, _userID.c_str());
        MQTT_ID_HTTP = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_HTTP, _userID.c_str());
        MQTT_NAME_HTTP = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_HTTP, _userName.c_str());
        MQTT_KEY_HTTP = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_HTTP, _key.c_str());
    // }
    // else if (_broker == BLINKER_HTTP_BORKER_QCLOUD) {
    //     // String id2name = _userID.subString(10, _userID.length());
    //     // memcpy(DEVICE_NAME_HTTP, _userID.c_str(), 12);
    //     DEVICE_NAME_HTTP = (char*)malloc((_userID.length()+1)*sizeof(char));
    //     strcpy(DEVICE_NAME_HTTP, _userID.c_str());
    //     String IDtest = _productInfo + _userID;
    //     MQTT_ID_HTTP = (char*)malloc((IDtest.length()+1)*sizeof(char));
    //     strcpy(MQTT_ID_HTTP, IDtest.c_str());
    //     String NAMEtest = IDtest + ";" + _userName;
    //     MQTT_NAME_HTTP = (char*)malloc((NAMEtest.length()+1)*sizeof(char));
    //     strcpy(MQTT_NAME_HTTP, NAMEtest.c_str());
    //     MQTT_KEY_HTTP = (char*)malloc((_key.length()+1)*sizeof(char));
    //     strcpy(MQTT_KEY_HTTP, _key.c_str());
    // }
    // else if (_broker == BLINKER_HTTP_BORKER_BLINKER) {
    //     // memcpy(DEVICE_NAME_HTTP, _userID.c_str(), 12);
    //     DEVICE_NAME_HTTP = (char*)malloc((_userID.length()+1)*sizeof(char));
    //     strcpy(DEVICE_NAME_HTTP, _userID.c_str());
    //     MQTT_ID_HTTP = (char*)malloc((_userID.length()+1)*sizeof(char));
    //     strcpy(MQTT_ID_HTTP, _userID.c_str());
    //     MQTT_NAME_HTTP = (char*)malloc((_userName.length()+1)*sizeof(char));
    //     strcpy(MQTT_NAME_HTTP, _userName.c_str());
    //     MQTT_KEY_HTTP = (char*)malloc((_key.length()+1)*sizeof(char));
    //     strcpy(MQTT_KEY_HTTP, _key.c_str());
    // }
    UUID_HTTP = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_HTTP, _uuid.c_str());

    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME_HTTP: "), DEVICE_NAME_HTTP);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_HTTP: "), MQTT_ID_HTTP);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_HTTP: "), MQTT_NAME_HTTP);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_HTTP: "), MQTT_KEY_HTTP);
    BLINKER_LOG_ALL(BLINKER_F("UUID_HTTP: "), UUID_HTTP);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    BLINKER_LOG_FreeHeap_ALL();

    isInit = true;

    isConnect_HTTP = true;

    return true;
}



bool BlinkerHTTP::checkInit()
{
    if (!_isWiFiInit)
    {
        switch (_configType)
        {
            case COMM :
                _connectTime = millis();
                // BLINKER_LOG(BLINKER_F("checkInit..."));
                if (WiFi.status() != WL_CONNECTED) {
                    ::delay(500);

                    if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                        // _connectTime = millis();
                        BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
                        BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
                        return false;
                    }

                    // BLINKER_LOG(BLINKER_F("checkInit..."));

                    return false;
                }
                BLINKER_LOG(BLINKER_F("WiFi Connected."));
                BLINKER_LOG(BLINKER_F("IP Address: "));
                BLINKER_LOG(WiFi.localIP());
                _isWiFiInit = true;
                _connectTime = 0;

                // begin();

                return false;
            case BLINKER_SMART_CONFIG :
                switch (_configStatus)
                {
                    case SMART_BEGIN :
#if defined(BLINKER_WIFI_HAS_SMARTCONFIG)
                        if (WiFi.smartConfigDone())
                        {
                            BLINKER_LOG(BLINKER_F("SmartConfig received."));
                            _connectTime = millis();

                            _configStatus = SMART_DONE;
                        }
                        else return false;
#else
                        _configStatus = SMART_TIMEOUT;
                        return false;
#endif
                    case SMART_DONE :
                        if (WiFi.status() != WL_CONNECTED)
                        {
                            if (millis() - _connectTime > 15000)
                            {
                                BLINKER_LOG(BLINKER_F("SmartConfig timeout."));
#if defined(BLINKER_WIFI_HAS_SMARTCONFIG)
                                WiFi.stopSmartConfig();
#endif
                                _configStatus = SMART_TIMEOUT;
                            }
                            return false;
                        }
                        else if (WiFi.status() == WL_CONNECTED)
                        {
                            // WiFi.stopSmartConfig();

                            BLINKER_LOG(BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(BLINKER_F("IP Address: "));
                            BLINKER_LOG(WiFi.localIP());
                            _isWiFiInit = true;
                            _connectTime = 0;

                            // begin();
                            
                            return false;
                        }
                        // return false;
                    case SMART_TIMEOUT :
#if defined(BLINKER_WIFI_HAS_SMARTCONFIG)
                        #if defined(BLINKER_ESP_SMARTCONFIG_V2)
                        WiFi.beginSmartConfig(SC_TYPE_ESPTOUCH_V2);
#else
                        WiFi.beginSmartConfig();
#endif
                        _configStatus = SMART_BEGIN;
                        BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
#endif
                        return false;
                    default :
                        return false;
                }
            default :
                return false;
        }
    }

    // if (!_isBegin)
    // {
    //     _isBegin = begin();
    //     return _isBegin;
    // }
    return true;
}

void BlinkerHTTP::commonBegin(const char* _ssid, const char* _pswd)
{
    _configType = COMM;

    connectWiFi(_ssid, _pswd);

    BLINKER_LOG(blinkerWiFiPlatformName(), BLINKER_F("_HTTP initialized..."));
}

void BlinkerHTTP::smartconfigBegin()
{
    _configType = BLINKER_SMART_CONFIG;

    if (!autoInit()) smartconfig();
    else _configStatus = SMART_DONE;

    BLINKER_LOG(blinkerWiFiPlatformName(), BLINKER_F("_HTTP initialized..."));
}

bool BlinkerHTTP::autoInit()
{
    blinkerWiFiModeSTA();
    String _hostname = BLINKER_F("DiyArduino_");
    _hostname += macDeviceName();

    blinkerWiFiSetHostname(_hostname.c_str());

    WiFi.begin();
    ::delay(500);

    BLINKER_LOG(BLINKER_F("Waiting for WiFi "),
                BLINKER_WIFI_INIT_TIMEOUT / 1000,
                BLINKER_F("s, will enter SMARTCONFIG while WiFi not connect!"));

    uint8_t _times = 0;
    while (WiFi.status() != WL_CONNECTED) {
        ::delay(500);
        if (_times > BLINKER_WIFI_INIT_TIMEOUT / 500) break;
        _times++;
    }

    if (WiFi.status() != WL_CONNECTED) return false;
    else {
        // BLINKER_LOG(BLINKER_F("WiFi Connected."));
        // BLINKER_LOG(BLINKER_F("IP Address: "));
        // BLINKER_LOG(WiFi.localIP());
        // _isWiFiInit = true;

        // begin();

        return true;
    }
}

void BlinkerHTTP::smartconfig()
{
    blinkerWiFiModeSTA();

    String _hostname = BLINKER_F("DiyArduino_");
    _hostname += macDeviceName();

    blinkerWiFiSetHostname(_hostname.c_str());

#if defined(BLINKER_WIFI_HAS_SMARTCONFIG)
    #if defined(BLINKER_ESP_SMARTCONFIG_V2)
    WiFi.beginSmartConfig(SC_TYPE_ESPTOUCH_V2);
#else
    WiFi.beginSmartConfig();
#endif

    _configStatus = SMART_BEGIN;

    BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
#else
    _configStatus = SMART_TIMEOUT;
    BLINKER_ERR_LOG(BLINKER_F("SmartConfig is not supported on this platform."));
#endif
    // while (!WiFi.smartConfigDone()) {
    //     ::delay(500);
    // }

    // BLINKER_LOG(BLINKER_F("SmartConfig received."));

    // BLINKER_LOG(BLINKER_F("Waiting for WiFi"));
    // while (WiFi.status() != WL_CONNECTED) {
    //     ::delay(500);
    // }

    // BLINKER_LOG(BLINKER_F("WiFi Connected."));

    // BLINKER_LOG(BLINKER_F("IP Address: "));
    // BLINKER_LOG(WiFi.localIP());
}
void BlinkerHTTP::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerHTTP::connectWiFi(const char* _ssid, const char* _pswd)
{
    uint32_t _connectTime = millis();

    BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

    blinkerWiFiModeSTA();
    String _hostname = BLINKER_F("DiyArduinoMQTT_");
    _hostname += macDeviceName();

    blinkerWiFiSetHostname(_hostname.c_str());

    if (_pswd && strlen(_pswd)) {
        WiFi.begin(_ssid, _pswd);
    }
    else {
        WiFi.begin(_ssid);
    }

    // while (WiFi.status() != WL_CONNECTED) {
    //     ::delay(50);

    //     if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
    //         _connectTime = millis();
    //         BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
    //         BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
    //     }
    // }
    // BLINKER_LOG(BLINKER_F("Connected"));

    // IPAddress myip = WiFi.localIP();
    // BLINKER_LOG(BLINKER_F("Your IP is: "), myip);

    // _isWiFiInit = true;
}

#endif

#endif
