#ifndef BLINKER_MQTT_AT_H
#define BLINKER_MQTT_AT_H

#if defined(ESP8266) || defined(ESP32)

#define BLINKER_AT_MQTT
#define BLINKER_ESP_AT

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif

#include "Blinker/BlinkerMQTTATBase.h"
#include "utility/BlinkerUtility.h"

class BlinkerMQTTAT
{
    public :
        BlinkerMQTTAT();

        bool serialAvailable();
        void serialBegin(Stream& s, bool state);
        int serialTimedRead();
        char * serialLastRead();
        // void serialFlush();
        bool serialPrint(const String & s1, const String & s2, bool needCheck = true);
        bool serialPrint(const String & s, bool needCheck = true);
        bool serialConnect();
        bool serialConnected();
        void serialDisconnect();

        bool connect();
        bool connected();
        bool mConnected();
        void disconnect();
        void ping();
        bool available();
        bool aligenieAvail();
        bool extraAvailable();
        void subscribe();
        char * lastRead();
        void flush();
        bool print(char * data, bool needCheck = true);
        bool bPrint(char * name, const String & data);
        bool aliPrint(const String & data);
        // void aliType(const String & type);
        void begin(const char* auth);
        bool autoPrint(uint32_t id);
        // bool autoPrint(char *name, char *type, char *data);
        // bool autoPrint(char *name1, char *type1, char *data1, \
        //             char *name2, char *type2, char *data2);
        char * deviceName();
        char * authKey() { return _authKey; }
        char * deviceId();// { return MQTT_ID; }
        char * uuid();// { return UUID; }
        void softAPinit();
        bool parseUrl(String data);
        bool autoInit();
        void smartconfig();
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);
        bool init() { return isMQTTinit; }
        bool reRegister() { return connectServer(); }
        void aligenieType(blinker_at_aligenie_t _type) { _aliType = _type; }

    private :
        bool isMQTTinit = false;

        bool connectServer();
        void mDNSInit();
        void checkKA();
        bool checkAliKA();
        bool checkCanPrint();
        bool checkCanBprint();
        bool checkPrintSpan();
        bool checkAliPrintSpan();

    protected :
        blinker_at_aligenie_t _aliType = ALI_NONE;
        // uint8_t     pinDataNum = 0;

        // io 
        // - - - -  - - - - | - - - -  - - - -
        // |                  |        |pull_up
        // |                  |mode
        // |pin
        // pin         40 0-39
        // mode        2 input/output
        // pull_up     3 none/pull_up/pull_down
        
        // const char* _authKey;
        char*       _authKey;
        // char*       _aliType;
        // char        _authKey[BLINKER_AUTHKEY_SIZE];
        bool*       isHandle;// = &isConnect;
        bool        isAlive = false;
        bool        isBavail = false;
        uint32_t    latestTime;
        uint32_t    printTime = 0;
        uint32_t    bPrintTime = 0;
        uint32_t    kaTime = 0;
        uint32_t    linkTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        char*       mqtt_broker;

        Stream*     stream;
        char*       streamData;//[BLINKER_MAX_READ_SIZE];
        bool        isSeriaFresh;
        bool        isSerialConnect;
        bool        isHWS = false;

        bool isJson(const String & data);
};

#if defined(ESP8266)
    extern BearSSL::WiFiClientSecure   client_mqtt_at;
    // WiFiClientSecure            client_mqtt;
#elif defined(ESP32)
    extern WiFiClientSecure            client_s_MQTT_AT;
#endif

extern WiFiClient              client_MQTT_AT;

#endif

#endif
