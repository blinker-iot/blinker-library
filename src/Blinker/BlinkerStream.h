#ifndef BLINKER_STREAM_H
#define BLINKER_STREAM_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "BlinkerUtility.h"

class BlinkerStream
{
    public :
        virtual int available() = 0;
        virtual char * lastRead() = 0;
        virtual void flush() = 0;
        // virtual int print(const String & s, bool needCheck = true) = 0;
        virtual int print(char * data, bool needCheck = true) = 0;
        virtual int connect() = 0;
        virtual int connected() = 0;
        virtual void disconnect() = 0;

        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
                // virtual void ping() = 0;
            virtual int autoPrint(unsigned long id) = 0;
            virtual void sharers(const String & data);
            virtual int needFreshShare() = 0;
        #endif

        #if defined(BLINKER_MQTT) || defined(BLINKER_HTTP)
            virtual int toServer(char * data) = 0;
            virtual char * deviceName() = 0;
            virtual char * authKey() = 0;
            virtual char * token() = 0;
            virtual int init() = 0;           
            virtual int mConnected() = 0;
            virtual void freshAlive() = 0;
        #endif




        #if defined(BLINKER_HTTP)
            virtual void subscribe() = 0;
        #endif

};

#endif
