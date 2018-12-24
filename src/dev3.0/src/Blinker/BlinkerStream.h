#ifndef BLINKER_STREAM_H
#define BLINKER_STREAM_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

class BlinkerStream
{
    public :
        virtual int available() = 0;
        virtual char * lastRead() = 0;
        virtual void flush() = 0;
        virtual int print(const String & s, bool needCheck = true) = 0;
        virtual int connect() = 0;
        virtual int connected() = 0;
        virtual void disconnect() = 0;
};

#endif
