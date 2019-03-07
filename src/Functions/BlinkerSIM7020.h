#ifndef BLINKER_SIM7020_H
#define BLINKER_SIM7020_H

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

enum sim7020_status_t
{
    sim7020_cpin_resq,
    sim7020_cpin_success,
    sim7020_csq_resq,
    sim7020_csq_success,
    sim7020_cgreg_resq,
    sim7020_cgreg_success,
    sim7020_cgact_resq,
    sim7020_cgact_success,
    sim7020_cops_resq,
    sim7020_cops_success,
    sim7020_cgcontrdp_resq,
    sim7020_cgcontrdp_success,
};

class BlinkerSIM7020
{
    public :
        BlinkerSIM7020() :
            isHWS(false)
        {}

        void setStream(Stream& s, bool isHardware, blinker_callback_t _func)
        { stream = &s; isHWS = isHardware; listenFunc = _func; }

        int checkPDN()
        {
            uint32_t sim_time = millis();
            sim7020_status_t pdn_status = sim7020_cpin_resq;

            streamPrint(BLINKER_CMD_CPIN_RESQ);

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CPIN &&
                        _masterAT->getParam(0) == BLINKER_CMD_READY)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_cpin_success"));
                        pdn_status = sim7020_cpin_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                }
            }

            if (pdn_status != sim7020_cpin_success) return false;

            streamPrint(BLINKER_CMD_CSQ_RESQ);
            sim_time = millis();

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CSQ &&
                        _masterAT->getParam(0).toInt() != 99)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_csq_success"));
                        pdn_status = sim7020_csq_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                }
            }

            if (pdn_status != sim7020_csq_success) return false;

            streamPrint(BLINKER_CMD_CGREG_RESQ);
            sim_time = millis();

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CGACT &&
                        _masterAT->getParam(1).toInt() == 1)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_cgact_success"));
                        pdn_status = sim7020_cgact_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                }
            }

            if (pdn_status != sim7020_cgact_success) return false;

            streamPrint(BLINKER_CMD_COPS_RESQ);
            sim_time = millis();

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_COPS &&
                        _masterAT->getParam(3).toInt() == 9)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_cops_success"));
                        pdn_status = sim7020_cops_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                }
            }

            if (pdn_status != sim7020_cops_success) return false;

            streamPrint(BLINKER_CMD_CGCONTRDP_RESQ);
            sim_time = millis();

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CGCONTRDP &&
                        _masterAT->getParam(3).toInt() == 9)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_cops_success"));
                        pdn_status = sim7020_cops_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                }
            }

            return true;
        }

        bool powerCheck()
        {
            streamPrint(BLINKER_CMD_AT);
            streamPrint("ATE0");

            if (!checkPDN()) return false;

            BLINKER_LOG_ALL(BLINKER_F("power check"));
            streamPrint(BLINKER_CMD_AT);
            uint32_t dev_time = millis();

            while(millis() - dev_time < _airTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("power on"));
                        
                        // SAPBR();

                        return true;
                    }
                }
            }

            return false;
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        blinker_callback_t listenFunc = NULL;
        Stream* stream;
        // char    streamData[128];
        char*   streamData;
        bool    isFresh = false;
        bool    isHWS = false;
        uint16_t    _simTimeout = 1000;

        void streamPrint(const String & s)
        {
            stream->println(s);
            BLINKER_LOG_ALL(s);
        }

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
            yield();

            if (!isHWS)
            {
                // #if defined(__AVR__) || defined(ESP8266)
                //     if (!SSerial_API->isListening())
                //     {
                //         SSerial_API->listen();
                //         ::delay(100);
                //     }
                // #endif

                if (listenFunc) listenFunc();
            }

            if (stream->available())
            {
                // streamData = "";
                // memset(streamData, '\0', 128);
                if (isFresh) free(streamData);
                streamData = (char*)malloc(1*sizeof(char));

                // strcpy(streamData, stream->readStringUntil('\n').c_str());

                // int16_t dNum = strlen(streamData);

                // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), streamData,
                //                 BLINKER_F(", dNum: "), dNum);
                // // stream->readString();
                
                int16_t dNum = 0;
                int c_d = timedRead();
                while (dNum < BLINKER_MAX_READ_SIZE && 
                    c_d >=0 && c_d != '\n')
                {
                    // if (c_d != '\r')
                    {
                        streamData[dNum] = (char)c_d;
                        dNum++;
                        streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
                    }

                    c_d = timedRead();
                }
                // dNum++;
                // // // streamData = (char*)realloc(streamData, dNum*sizeof(char));

                // streamData[dNum-1] = '\0';
                // stream->flush();
// 7b2264657461696c223a207b2262726f6b6572223a2022616c6979756e222c20226465766963654e616d65223a20223237383636394232304d3235423634323230354e33435850222c2022696f744964223a20225346455467613255784b784e386a69716e4e516730303130356435343030222c2022696f74546f6b656e223a20226331353530643464346334623432666338376431343639373333363166353539222c202270726f647563744b6579223a20224a67434762486c6e64677a222c202275756964223a20223733633762356134623266323231633061373264376234313238653430323337227d2c20226d657373616765223a20313030307d

                // BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
                //                 BLINKER_F(" , dNum: "), dNum);
                // BLINKER_LOG_FreeHeap_ALL();
                
                if (dNum < BLINKER_MAX_READ_SIZE && dNum > 0)
                {
                    // if (streamData[dNum - 1] == '\r')
                    streamData[dNum - 1] = '\0';
                    BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
                                    BLINKER_F(" , dNum: "), dNum);

                    isFresh = true;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
};

#endif
