#ifndef BLINKER_MQTT_H
#define BLINKER_MQTT_H

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

class BlinkerMQTT
{
    public :
        BlinkerMQTT();

        bool connect();
        bool connected();
        void disconnect();
        void ping();
        bool available();
        bool aligenieAvail();
        bool extraAvailable();
        void subscribe();
        char * lastRead();
        void flush();
        bool print(char * data, bool needCheck = true);
        bool bPrint(char * name, char * data);
        bool aliPrint(String & data);
        void aliType(String & type);
        void begin(const char* auth);
        bool autoPrint(uint32_t id);
        bool autoPrint(char *name, char *type, char *data);
        bool autoPrint(char *name1, char *type1, char *data1
                    , char *name2, char *type2, char *data2);
        char * deviceName();
        char * authKey() { return _authKey; }
        bool init() { return isMQTTinit; }
        bool reRegister() { return connectServer(); }

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
        // const char* _authKey;
        char*       _authKey;
        char*       _aliType;
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
};

#if defined(ESP8266)
    extern BearSSL::WiFiClientSecure   client_s;
    // WiFiClientSecure            client_mqtt;
#elif defined(ESP32)
    extern WiFiClientSecure            client_s;
#endif

extern WiFiClient              client;

#endif
