#ifndef BLINKER_WIFI_H
#define BLINKER_WIFI_H

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

class BlinkerWiFi
{
    public :
        BlinkerWiFi();// {}

        void begin(const String & deviceName);

        bool available();
        char * lastRead();        
        void flush();
        void print(const String & s_data, bool needCheck = true);
        bool connect();
        bool connected();
        void disconnect();

    private :
        bool checkPrintSpan();

    protected :
        bool*       isHandle;// = &isConnect;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
};

#endif

#endif
