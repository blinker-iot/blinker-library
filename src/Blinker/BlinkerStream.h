#ifndef BLINKER_STREAM_H
#define BLINKER_STREAM_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #if defined(BLINKER_AT_MQTT)
//     #include "Blinker/BlinkerMQTTATBase.h"
// #endif

#include "BlinkerUtility.h"

class BlinkerStream
{
    public :
    // #if defined(BLINKER_LOWPOWER_AIR202)
    //     virtual void begin(const char* _key, const char* _deviceType, String _imei) = 0;
    //     virtual char * deviceName() = 0;
    //     virtual char * authKey() = 0;
    //     virtual int init() = 0;
    //     virtual int deviceRegister() = 0;
    // #else
        virtual int available() = 0;
        virtual char * lastRead() = 0;
        virtual void flush() = 0;
        // virtual int print(const String & s, bool needCheck = true) = 0;
        virtual int print(char * data, bool needCheck = true) = 0;
        virtual int connect() = 0;
        virtual int connected() = 0;
        virtual void disconnect() = 0;

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
            defined(BLINKER_AT_MQTT) || defined(BLINKER_MQTT_AT) || \
            defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_NBIOT_SIM7020) || \
            defined(BLINKER_GPRS_AIR202) || defined(BLINKER_PRO_SIM7020) || \
            defined(BLINKER_PRO_AIR202) || defined(BLINKER_MQTT_AUTO) || \
            defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_SUBDEVICE) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7000) || defined(BLINKE_HTTP)
                virtual int aliPrint(const String & data) = 0;
                virtual int duerPrint(const String & data, bool report = false) = 0;
                #if !defined(BLINKER_GPRS_AIR202) && !defined(BLINKER_NBIOT_SIM7020) && \
                    !defined(BLINKER_PRO_SIM7020) && !defined(BLINKER_PRO_AIR202) && \
                    !defined(BLINKER_QRCODE_NBIOT_SIM7020) && !defined(BLINKER_NBIOT_SIM7000) && \
                    !defined(BLINKER_QRCODE_NBIOT_SIM7000)
                virtual int miPrint(const String & data) = 0;
                #endif
                // virtual void ping() = 0;
            #if !defined(BLINKER_MQTT_AT)
                virtual int bPrint(char * name, const String & data) = 0;
                virtual int autoPrint(unsigned long id) = 0;
                virtual void sharers(const String & data);
                virtual int aligenieAvail() = 0;
                virtual int duerAvail() = 0;
                #if !defined(BLINKER_GPRS_AIR202) && !defined(BLINKER_NBIOT_SIM7020) && \
                    !defined(BLINKER_PRO_SIM7020) && !defined(BLINKER_PRO_AIR202) && \
                    !defined(BLINKER_QRCODE_NBIOT_SIM7020) && !defined(BLINKER_NBIOT_SIM7000) && \
                    !defined(BLINKER_QRCODE_NBIOT_SIM7000)
                virtual int miAvail() = 0;
                #endif
                virtual int needFreshShare() = 0;
            #endif
        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_PRO) || \
            defined(BLINKER_AT_MQTT) || defined(BLINKER_WIFI_GATEWAY) || \
            defined(BLINKER_GPRS_AIR202) || defined(BLINKER_NBIOT_SIM7020) || \
            defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
            defined(BLINKER_WIFI_SUBDEVICE) || defined(BLINKER_QRCODE_NBIOT_SIM7020) || \
            defined(BLINKER_NBIOT_SIM7000) || defined(BLINKER_QRCODE_NBIOT_SIM7000) || \
            defined(BLINKE_HTTP)
            virtual int toServer(char * data) = 0;
            virtual char * deviceName() = 0;
            virtual char * authKey() = 0;
            virtual char * token() = 0;
            virtual int init() = 0;           
            virtual int mConnected() = 0;
            virtual void freshAlive() = 0;
        #endif

        #if defined(BLINKER_LOWPOWER_AIR202)
            virtual char * deviceName() = 0;
            virtual char * authKey() = 0;
            virtual int init() = 0;
            virtual void begin(const char* _key, const char* _type, String _imei) = 0;
            virtual int deviceRegister() = 0;   
        #endif

        // #if defined(BLINKER_MQTT) || defined(BLINKER_PRO)
        //     virtual int aligenieAvail() = 0;
        //     virtual int duerAvail() = 0;
        #if defined(BLINKER_AT_MQTT)
            virtual void begin(const char* auth) = 0;
            // virtual void begin() = 0;
            virtual int serialAvailable() = 0;
            virtual int serialPrint(const String & s1, const String & s2, bool needCheck = true) = 0;
            virtual int serialPrint(const String & s, bool needCheck = true) = 0;
            virtual int mqttPrint(const String & data);
            virtual char * serialLastRead() = 0;
            virtual void aligenieType(int _type) = 0;
            virtual void duerType(int _type) = 0;
            virtual char * deviceId() = 0;
            virtual char * uuid() = 0;
            virtual void softAPinit() = 0;
            virtual void smartconfig() = 0;
            virtual int autoInit() = 0;
            virtual void connectWiFi(String _ssid, String _pswd) = 0;
            virtual void connectWiFi(const char* _ssid, const char* _pswd) = 0;
        #endif

        #if defined(BLINKER_PRO) || defined(BLINKER_MQTT_AUTO) || \
            defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY) || \
            defined(BLINKER_WIFI_SUBDEVICE)
            virtual int deviceRegister() = 0;
            virtual int authCheck() = 0;
            #if defined(BLINKER_PRO)
                virtual void begin(const char* _deviceType) = 0;
            #elif defined(BLINKER_MQTT_AUTO) || defined(BLINKER_PRO_ESP) || \
                defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_WIFI_SUBDEVICE)
                virtual void begin(const char* auth, const char* type) = 0;
            #endif
        #elif defined(BLINKER_GPRS_AIR202) || defined(BLINKER_NBIOT_SIM7020) || \
            defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_NBIOT_SIM7000) || \
            defined(BLINKER_QRCODE_NBIOT_SIM7000)
            virtual int deviceRegister() = 0;

            #if defined(BLINKER_QRCODE_NBIOT_SIM7020) || defined(BLINKER_QRCODE_NBIOT_SIM7000)
                virtual void begin(const char* _auth, const char* _type, String _imei) = 0;
            #else
                virtual void begin(const char* _type, String _imei) = 0;
            #endif

            #if defined(BLINKER_PRO_SIM7020) || defined(BLINKER_PRO_AIR202)
                virtual int authCheck() = 0;
            #endif
        #endif

        #if defined(BLINKER_WIFI_GATEWAY) || defined(BLINKER_WIFI_SUBDEVICE)
            virtual void meshCheck() = 0;
            #if !defined(BLINKER_WIFI_SUBDEVICE)
                virtual void setTimezone(float tz) = 0;
            #endif
            #if defined(BLINKER_WIFI_SUBDEVICE)
                virtual int subPrint(const String & data) = 0;
                virtual int meshAvail() = 0;
                virtual char * meshLastRead() = 0;
                virtual void meshFlush() = 0;
            #endif
        #endif

        #if defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_GATEWAY)
            virtual void setSmartConfig() = 0;
            virtual void setApConfig() = 0;
            virtual bool checkSmartConfig() = 0;
        #endif

        #if defined(BLINKER_HTTP)
            virtual void subscribe() = 0;
        #endif

        // #if defined(BLINKER_WIFI_SUBDEVICE)
        //     virtual void attachAvailable(blinker_callback_return_int_t func) = 0;
        //     virtual void attachRead(blinker_callback_return_string_t func) = 0;
        //     virtual void attachPrint(blinker_callback_with_string_arg_t func) = 0;
        //     virtual void attachBegin(blinker_callback_t func) = 0;
        //     virtual void attachConnect(blinker_callback_return_int_t func) = 0;
        //     virtual void attachConnected(blinker_callback_return_int_t func) = 0;
        //     virtual void attachDisconnect(blinker_callback_t func) = 0;
        // #endif
    // #endif
};

#endif
