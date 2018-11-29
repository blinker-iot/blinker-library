#ifndef BLINKER_SERIAL_H
#define BLINKER_SERIAL_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include <SoftwareSerial.h>
// #include <HardwareSerial.h>

class BlinkerSerial
{
    public :
        BlinkerSerial()
            : stream(NULL), isConnect(false)
        {}

        bool available();
        void begin(Stream& s, bool state);
        int timedRead();
        char * lastRead() { return isFresh ? streamData : NULL; }
        void flush();
        bool print(const String & s, bool needCheck = true);
        bool connect()      { isConnect = true; return connected(); }
        bool connected()    { return isConnect; }
        void disconnect()   { isConnect = false; }

    protected :
        Stream* stream;
        char*   streamData;
        bool    isFresh;
        bool    isConnect;
        bool    isHWS = false;
        uint8_t respTimes = 0;
        uint32_t    respTime = 0;

        bool checkPrintSpan();
};

// #if defined(ESP32)
//     extern HardwareSerial *HSerialBLE;
// #else
    extern SoftwareSerial *SSerialBLE;
// #endif

#endif
