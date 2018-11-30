#ifndef BLINKER_PRO_H
#define BLINKER_PRO_H

#if defined(ESP8266) || defined(ESP32)

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif

class BlinkerPRO {
    public :
        BlinkerPRO();

        bool connect();
        bool connected();
        bool mConnected();// { if (!isMQTTinit) return false; else return mqtt->connected(); }
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
        void begin(const char* _deviceType);
        bool autoPrint(uint32_t id);
        // bool autoPrint(char *name, char *type, char *data);
        // bool autoPrint(char *name1, char *type1, char *data1, \
        //             char *name2, char *type2, char *data2);
        char * deviceName();
        char * authKey() { return _authKey; }
        bool init() { return isMQTTinit; }
        bool reRegister() { return connectServer(); }
        bool deviceRegister() { return connectServer(); }
        bool authCheck();

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
        bool pubHello();

    protected :
        const char* _deviceType;
        char*       _authKey;
        // char*       _aliType;
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

        bool        isNew = false;
        bool        isAuth = false;

        bool isJson(const String & data);
};

#if defined(ESP8266)
    extern BearSSL::WiFiClientSecure   client_pro;
    // WiFiClientSecure            client_mqtt;
#elif defined(ESP32)
    extern WiFiClientSecure            client_s_PRO;
#endif

extern WiFiClient              client_PRO;

#endif

#endif
