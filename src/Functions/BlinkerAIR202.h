#ifndef BLINKER_AIR202_H
#define BLINKER_AIR202_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

#include "Blinker/BlinkerATMaster.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

#if defined(ESP32)
    #include <HardwareSerial.h>

    HardwareSerial *HSerial_API;
#else
    #include <SoftwareSerial.h>

    SoftwareSerial *SSerial_API;
#endif

class BlinkerAIR202
{
    public :
        BlinkerAIR202() :
            isHWS(false)
        {}

        void setStream(Stream& s, bool isHardware = false)
        { stream = &s; isHWS = isHardware; }

        bool powerCheck()
        {
            uint32_t dev_time = millis();

            stream->println(BLINKER_CMD_AT);

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("power on"));
                        
                        return true;
                    }
                }
            }

            return false;
        }

        String getIMEI()
        {
            uint32_t dev_time = millis();

            stream->println(BLINEKR_CMD_CGSN_RESQ);

            String _imei = "";

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    BLINKER_LOG_ALL(BLINKER_F("get IMEI: "), streamData);
                    _imei = streamData;
                }
            }

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("get IMEI: "), _imei);
                        return _imei;
                    }       
                }
            }

            return "";
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        Stream* stream;
        char*   streamData;
        bool    isFresh = false;
        bool    isHWS = false;

        int timedRead()
        {
            int c;
            uint32_t _startMillis = millis();
            do {
                c = stream->read();
                if (c >= 0) return c;
            } while(millis() - _startMillis < 1000);
            return -1; 
        }

        bool available()
        {
            if (!isHWS)
            {
                #if defined(__AVR__) || defined(ESP8266)
                    if (!SSerial_API->isListening())
                    {
                        SSerial_API->listen();
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
                BLINKER_LOG_FreeHeap_ALL();
                
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
};

// BlinkerAIR202 BLINKER_AIR202;

#endif
