#ifndef BLINKER_SERIAL_H
#define BLINKER_SERIAL_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #include "Adapters/BlinkerSerial.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

#if defined(ESP32)
    #include <HardwareSerial.h>

    extern HardwareSerial *HSerialBLE;
#else
    #include <SoftwareSerial.h>

    extern SoftwareSerial *SSerialBLE;
#endif

class BlinkerSerial : public BlinkerStream
{
    public :
        BlinkerSerial()
            : stream(NULL), isConnect(false)
        {}

        int available();
        void begin(Stream& s, bool state);
        int timedRead();
        char * lastRead()   { return isFresh ? streamData : NULL; }
        void flush();
        int print(char * data, bool needCheck = true);
        int connect()       { isConnect = true; return connected(); }
        int connected()     { return isConnect; }
        void disconnect()   { isConnect = false; }

    protected :
        Stream* stream;
        char*   streamData;
        bool    isFresh;
        bool    isConnect;
        bool    isHWS = false;
        uint8_t respTimes = 0;
        uint32_t    respTime = 0;

        int checkPrintSpan();
};

// #if defined(ESP32)
//     extern HardwareSerial *HSerialBLE;
// #else
//     extern SoftwareSerial *SSerialBLE;
// #endif

#endif
