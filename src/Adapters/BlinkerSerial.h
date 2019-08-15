#ifndef BLINKER_SERIAL_H
#define BLINKER_SERIAL_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #include "../Adapters/BlinkerSerial.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

#if defined(ESP32)
    #include <HardwareSerial.h>

    HardwareSerial *HSerialBLE;
#elif defined (__AVR__) || defined(ESP8266)
    #include <SoftwareSerial.h>

    SoftwareSerial *SSerialBLE;
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
        char * lastRead()   { if (isFresh) return streamData; else return ""; }
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

int BlinkerSerial::available()
{
    if (!isHWS)
    {
        #if defined(__AVR__) || defined(ESP8266)
            if (!SSerialBLE->isListening())
            {
                SSerialBLE->listen();
                ::delay(100);
            }
        #endif
    }

    if (stream->available())
    {
        if (isFresh) free(streamData);
        streamData = (char*)malloc(1*sizeof(char));
        
        int16_t dNum = 0;
        int c_d = timedRead();
        while (dNum < BLINKER_MAX_READ_SIZE && 
            c_d >=0 && c_d != '\n')
        {
            if (c_d != '\r')
            {
                streamData[dNum] = (char)c_d;
                dNum++;
                streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
            }

            c_d = timedRead();
        }
        dNum++;
        streamData = (char*)realloc(streamData, dNum*sizeof(char));

        streamData[dNum-1] = '\0';
        stream->flush();
        
        BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData);
        // BLINKER_LOG_FreeHeap_ALL();
        
        if (strlen(streamData) < BLINKER_MAX_READ_SIZE)
        {
            if (streamData[strlen(streamData) - 1] == '\r')
                streamData[strlen(streamData) - 1] = '\0';

            isFresh = true;
            return true;
        }
        else
        {
            free(streamData);
            return false;
        }
    }
    else
    {
        return false;
    }
}

void BlinkerSerial::begin(Stream& s, bool state)
{
    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;
}

int BlinkerSerial::timedRead()
{
    int c;
    uint32_t _startMillis = millis();
    do {
        c = stream->read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < 1000);
    return -1; 
}

void BlinkerSerial::flush()
{
    if (isFresh)
    {
        free(streamData); isFresh = false;
    }
}

// int BlinkerSerial::print(const String & s, bool needCheck)
int BlinkerSerial::print(char * data, bool needCheck)
{
    if (needCheck)
    {
        if (!checkPrintSpan())
        {
            respTime = millis();
            return false;
        }
    }

    respTime = millis();
    
    BLINKER_LOG_ALL(BLINKER_F("Response: "), data);

    if(connected())
    {
        BLINKER_LOG_ALL(BLINKER_F("Success..."));
        
        stream->println(data);
        return true;
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
        
        return false;
    }
}

int BlinkerSerial::checkPrintSpan()
{
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT)
    {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT)
        {
            BLINKER_ERR_LOG(BLINKER_F("DEVICE NOT CONNECT OR MSG LIMIT"));
            
            return false;
        }
        else
        {
            respTimes++;
            return true;
        }
    }
    else
    {
        respTimes = 0;
        return true;
    }
}

#endif
