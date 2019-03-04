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

enum air202_status_t
{
    air202_cgtt_state_ver_check,
    air202_cgtt_state_ver_check_success,
    air202_cgtt_state,
    air202_cgtt_state_resp,
    air202_cgtt_state_success,
    air202_sapbar_pdp_resq,
    air202_sapbar_pdp_success,
    air202_sapbar_pdp_failed,
    air202_sapbar_apn_resq,
    air202_sapbar_apn_success,
    air202_sapbar_apn_failed,
    air202_sapbar_save_resq,
    air202_sapbar_save_success,
    air202_sapbar_save_failed,
    air202_sapbar_fresh_resq,
    air202_sapbar_fresh_success,
    air202_sapbar_fresh_failed,
};

class BlinkerAIR202
{
    public :
        BlinkerAIR202() :
            isHWS(false)
        {}

        void setStream(Stream& s, bool isHardware, blinker_callback_t _func)
        { stream = &s; isHWS = isHardware; listenFunc = _func; }

        int checkCGTT()
        {
            uint32_t http_time = millis();
            air202_status_t cgtt_status = air202_cgtt_state_ver_check;

            stream->println(BLINKER_CMD_CGMMR_RESQ);

            cgtt_status = air202_cgtt_state_ver_check;

            while(millis() - http_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_CGMMR_RESP) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_cgtt_state_ver_check_success"));
                        cgtt_status = air202_cgtt_state_ver_check_success;
                        break;
                    }
                }
            }

            if (cgtt_status != air202_cgtt_state_ver_check_success) return false;

            stream->println(BLINKER_CMD_CGQTT_RESQ);
            cgtt_status = air202_cgtt_state;
            http_time = millis();

            while(millis() - http_time < 1000)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CGATT &&
                        _masterAT->getParam(0).toInt() == 1)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_cgtt_state_resp"));
                        cgtt_status = air202_cgtt_state_resp;
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (cgtt_status != air202_cgtt_state_resp) return false;
            // http_time = millis();

            // while(millis() - http_time < 1000)
            // {
            //     if (available())
            //     {
            //         if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("air202_cgtt_state_success"));
            //             cgtt_status = air202_cgtt_state_success;
            //             break;
            //         }
            //     }
            // }

            // if (cgtt_status != air202_cgtt_state_success) return false;

            BLINKER_LOG_ALL(BLINKER_F("check CGTT success"));

            return true;

            // stream->println(BLINKER_CMD_CGQTT_RESQ);

            // cgtt_status = air202_cgtt_state;
        }

        bool powerCheck()
        {
            stream->println(BLINKER_CMD_AT);
            stream->println("ATE0");

            if (!checkCGTT()) return false;
            
            BLINKER_LOG_ALL(BLINKER_F("power check"));
            stream->println(BLINKER_CMD_AT);
            uint32_t dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("power on"));
                        
                        // SAPBR();

                        return SAPBR();
                    }
                }
            }

            return false;
        }

        bool SAPBR()
        {
            stream->println(STRING_format(BLINEKR_CMD_SAPBR_RESQ) + \
                            "=3,1,\"CONTYPE\",\"GPRS\"");

            air202_status_t sapbar_status = air202_sapbar_pdp_resq;
            uint32_t dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_pdp_success"));

                        sapbar_status = air202_sapbar_pdp_success;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_pdp_success) return false;

            stream->println(STRING_format(BLINEKR_CMD_SAPBR_RESQ) + \
                            "=3,1,\"APN\",\"CMNET\"");

            sapbar_status = air202_sapbar_apn_resq;
            dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_apn_success"));

                        sapbar_status = air202_sapbar_apn_success;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_apn_success) return false;

            stream->println(STRING_format(BLINEKR_CMD_SAPBR_RESQ) + \
                            "=5,1");

            sapbar_status = air202_sapbar_save_resq;
            dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_save_success"));

                        sapbar_status = air202_sapbar_save_success;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_save_success) return false;

            stream->println(STRING_format(BLINEKR_CMD_SAPBR_RESQ) + \
                            "=1,1");

            sapbar_status = air202_sapbar_fresh_resq;
            dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_fresh_success"));

                        sapbar_status = air202_sapbar_fresh_success;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_fresh_success) return false;

            return true;
        }

        String getIMEI()
        {
            uint32_t dev_time = millis();

            stream->println(BLINEKR_CMD_CGSN_RESQ);

            char _imei[16];

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    BLINKER_LOG_ALL(BLINKER_F("get IMEI: "), streamData, 
                                    BLINKER_F(", length: "), strlen(streamData));
                    if (strlen(streamData) == 15)
                    {
                        strcpy(_imei, streamData);
                    }
                }
            }

            dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("get IMEI: "), _imei,
                                        BLINKER_F(", length: "), strlen(streamData));
                        return _imei;
                    }       
                }
            }

            BLINKER_LOG_ALL(BLINKER_F("get IMEI: "), _imei,
                            BLINKER_F(", length: "), strlen(streamData));

            return _imei;
        }

        String getICCID()
        {
            uint32_t dev_time = millis();

            stream->println(BLINKER_CMD_ICCID_RESQ);

            char _iccid[21];

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_ICCID)
                    {
                        String get_iccid = _masterAT->getParam(0);

                        if (_masterAT->getParam(0).length() == 20)
                        {
                            strcpy(_iccid, _masterAT->getParam(0).c_str());
                            free(_masterAT);

                            BLINKER_LOG_ALL(BLINKER_F("get ICCID: "), _iccid,
                            BLINKER_F(", length: "), strlen(_iccid));
                            break;
                        }                        
                    }

                    free(_masterAT);
                }
            }

            dev_time = millis();

            while(millis() - dev_time < 1000)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("get ICCID: "), _iccid,
                                        BLINKER_F(", length: "), strlen(streamData));
                        return _iccid;
                    }       
                }
            }

            BLINKER_LOG_ALL(BLINKER_F("get ICCID: "), _iccid,
                            BLINKER_F(", length: "), strlen(streamData));

            return _iccid;
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        blinker_callback_t listenFunc = NULL;
        Stream* stream;
        char    streamData[128];
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
            }

            if (stream->available())
            {
                // streamData = "";
                memset(streamData, '\0', 128);
                // if (isFresh) free(streamData);
                // streamData = (char*)malloc(1*sizeof(char));
                
                int16_t dNum = 0;
                int c_d = timedRead();
                while (dNum < BLINKER_MAX_READ_SIZE && 
                    c_d >=0 && c_d != '\n')
                {
                    if (c_d != '\r')
                    {
                        streamData[dNum] = (char)c_d;
                        dNum++;
                        // streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
                    }

                    c_d = timedRead();
                }
                dNum++;
                // streamData = (char*)realloc(streamData, dNum*sizeof(char));

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
                    // free(streamData);
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
