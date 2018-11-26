#ifndef BLINKER_ESP_MQTT_H
#define BLINKER_ESP_MQTT_H

#define BLINKER_MQTT

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerMQTT.h"
#include "Blinker/BlinkerProtocol.h"

class BlinkerESPMQTT: public BlinkerProtocol<BlinkerMQTT>
{
    typedef BlinkerProtocol<BlinkerMQTT> Base;

    public : 
        BlinkerESPMQTT(BlinkerMQTT &transp)
            : Base(transp)
        {}

        void begin();
        void begin(const char* _auth)
        {
            #if defined(BLINKER_ESP_SMARTCONFIG)
                smartconfigBegin(_auth);
            #elif defined(BLINKER_APCONFIG)
                apconfigBegin(const char* _auth);
            #endif
        }

        void begin( const char* _auth, 
                    const char* _ssid, 
                    const char* _pswd );

    private :
        void smartconfigBegin(const char* _auth);
        void apconfigBegin(const char* _auth);

        bool autoInit();
        void smartconfig();
        void softAPinit();
        void serverClient();
        bool parseUrl(String data);
        
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);
};

#endif
