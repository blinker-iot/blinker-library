#ifndef BLINKER_MQTT_H
#define BLINKER_MQTT_H

#if defined(ESP8266) || defined(ESP32)

#define BLINKER_MQTT

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif

#include "modules/WebSockets/WebSocketsServer.h"
#include "modules/mqtt/Adafruit_MQTT.h"
#include "modules/mqtt/Adafruit_MQTT_Client.h"
#include "modules/ArduinoJson/ArduinoJson.h"

// #include "Adapters/BlinkerMQTT.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

class BlinkerMQTT : public BlinkerStream
{
    public :
        BlinkerMQTT();

        int connect();
        int connected();
        int mConnected();
        void disconnect();
        void ping();
        int available();
        int aligenieAvail();
        int duerAvail();
        // bool extraAvailable();
        void subscribe();
        char * lastRead();
        void flush();
        int print(char * data, bool needCheck = true);
        int bPrint(char * name, const String & data);
        int aliPrint(const String & data);
        int duerPrint(const String & data);
        void aliType(const String & type);
        void duerType(const String & type);
        void begin(const char* auth);
        int autoPrint(uint32_t id);
        // bool autoPrint(char *name, char *type, char *data);
        // bool autoPrint(char *name1, char *type1, char *data1, 
        //             char *name2, char *type2, char *data2);
        char * deviceName();
        char * authKey() { return _authKey; }
        int init() { return isMQTTinit; }
        int reRegister() { return connectServer(); }
        void freshAlive() { kaTime = millis(); isAlive = true; }

    private :
        bool isMQTTinit = false;

        int connectServer();
        void mDNSInit();
        void checkKA();
        int checkAliKA();
        int checkDuerKA();
        int checkCanPrint();
        int checkCanBprint();
        int checkPrintSpan();
        int checkAliPrintSpan();
        int checkDuerPrintSpan();

    protected :
        // const char* _authKey;
        char*       _authKey;
        char*       _aliType;
        char*       _duerType;
        // char        _authKey[BLINKER_AUTHKEY_SIZE];
        bool*       isHandle;// = &isConnect;
        bool        isAlive = false;
        // bool        isBavail = false;
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
};

#endif

#endif
