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
        
        void begin(const char* _auth)
        {
            #if defined(BLINKER_ALIGENIE_LIGHT)
                String _aliType = BLINKER_F("&aliType=light");
            #elif defined(BLINKER_ALIGENIE_OUTLET)
                String _aliType = BLINKER_F("&aliType=outlet");
            #elif defined(BLINKER_ALIGENIE_SENSOR)
                String _aliType = BLINKER_F("&aliType=sensor");
            #else
                String _aliType = BLINKER_F("");
            #endif

            #if defined(BLINKER_ESP_SMARTCONFIG)
                smartconfigBegin(_auth, _aliType);
            #elif defined(BLINKER_APCONFIG)
                apconfigBegin(_auth, _aliType);
            #endif
        }

        void begin( const char* _auth, 
                    const char* _ssid, 
                    const char* _pswd )
        {
            #if defined(BLINKER_ALIGENIE_LIGHT)
                String _aliType = BLINKER_F("&aliType=light");
            #elif defined(BLINKER_ALIGENIE_OUTLET)
                String _aliType = BLINKER_F("&aliType=outlet");
            #elif defined(BLINKER_ALIGENIE_SENSOR)
                String _aliType = BLINKER_F("&aliType=sensor");
            #else
                String _aliType = BLINKER_F("");
            #endif
            
            commonBegin(_auth, _ssid, _pswd, _aliType);
        }

    private :
        void commonBegin(const char* _auth, 
                        const char* _ssid, 
                        const char* _pswd,
                        String & _type);
        void smartconfigBegin(const char* _auth, String & _type);
        void apconfigBegin(const char* _auth, String & _type);

        bool autoInit();
        void smartconfig();
        void softAPinit();
        void serverClient();
        bool parseUrl(String data);
        
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);
};

#endif
