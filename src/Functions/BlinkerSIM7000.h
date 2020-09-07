#ifndef BLINKER_SIM7000_H
#define BLINKER_SIM7000_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

#include "../Blinker/BlinkerATMaster.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

#include <time.h>

#define BLINKER_SIM7000_DATA_BUFFER_SIZE 1024

enum sim7000_status_t
{
    sim7000_cpin_REQ,
    sim7000_cpin_success,
    sim7000_csq_REQ,
    sim7000_csq_success,
    sim7000_cgreg_REQ,
    sim7000_cgreg_success,
    sim7000_cgact_REQ,
    sim7000_cgact_success,
    sim7000_cops_REQ,
    sim7000_cops_success,
    sim7000_cnact_REQ,
    sim7000_cnact_success,
    sim7000_cgcontrdp_REQ,
    sim7000_cgcontrdp_success,
};

class BlinkerSIM7000
{
    public :
        BlinkerSIM7000() :
            isHWS(false)
        {}

        ~BlinkerSIM7000() { flush(); }

        time_t  _ntpTime = 0;

        void setStream(Stream& s, bool isHardware, blinker_callback_t _func)
        { stream = &s; isHWS = isHardware; listenFunc = _func; }

        bool getSNTP(float _tz = 8.0, char _url[] = "120.25.108.11")
        {
            // uint32_t os_time = millis();
            // _timezone = _tz;
            // streamPrint(STRING_format(BLINKER_CMD_CNTP_REQ) + \
            //             "=" + STRING_format(_url) + \
            //             "," + STRING_format((int8_t)(_tz * 4)));

            // while(millis() - os_time < _simTimeout * 10)
            // {
            //     if (available())
            //     {
            //         _masterAT = new BlinkerMasterAT();
            //         _masterAT->update(STRING_format(streamData));

            //         if (_masterAT->getState() != AT_M_NONE &&
            //             _masterAT->reqName() == BLINKER_CMD_CNTP)
            //         {
            //             free(_masterAT);
            //             return true;
            //         }
            //         free(_masterAT);
            //     }
            // }
            // return false;
            return true;
        }

        int checkPDN()
        {
            uint32_t sim_time = millis();
            sim7000_status_t pdn_status = sim7000_cpin_REQ;

            streamPrint(BLINKER_CMD_CPIN_REQ);

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
                        BLINKER_LOG_ALL(BLINKER_F("sim7000_cpin_success"));
                        pdn_status = sim7000_cpin_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                }
            }

            if (pdn_status != sim7000_cpin_success) return false;

            // streamPrint(BLINKER_CMD_CSQ_REQ);
            // sim_time = millis();

            // while(millis() - sim_time < _simTimeout)
            // {
            //     if (available())
            //     {
            //         _masterAT = new BlinkerMasterAT();
            //         _masterAT->update(STRING_format(streamData));

            //         if (_masterAT->getState() != AT_M_NONE &&
            //             _masterAT->reqName() == BLINKER_CMD_CSQ &&
            //             _masterAT->getParam(0).toInt() != 99)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("sim7000_csq_success"));
            //             pdn_status = sim7000_csq_success;
            //             free(_masterAT);
            //             break;
            //         }
            //         free(_masterAT);
            //     }
            // }

            // if (pdn_status != sim7000_csq_success) return false;

            // streamPrint(BLINKER_CMD_COPS_REQ);
            // sim_time = millis();

            // while(millis() - sim_time < _simTimeout)
            // {
            //     if (available())
            //     {
            //         _masterAT = new BlinkerMasterAT();
            //         _masterAT->update(STRING_format(streamData));

            //         if (_masterAT->getState() != AT_M_NONE &&
            //             _masterAT->reqName() == BLINKER_CMD_COPS)
            //             //  &&
            //             // _masterAT->getParam(3).toInt() == 9)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("sim7000_cops_success"));
            //             pdn_status = sim7000_cops_success;
            //             free(_masterAT);
            //             break;
            //         }
            //         free(_masterAT);
            //     }
            // }

            // if (pdn_status != sim7000_cops_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CNACT_REQ) + "=?");
            sim_time = millis();

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == "CNACT" &&
                        _masterAT->getParam(0).toInt() == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7000_cops_success"));
                        pdn_status = sim7000_cops_success;
                        free(_masterAT);
                        break;
                    }
                    free(_masterAT);
                    // return true;
                }
            }

            streamPrint(STRING_format(BLINKER_CMD_CNACT_REQ) + "=1,\"cmnet\"");
            sim_time = millis();
            
            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        //return true;
                        pdn_status = sim7000_cnact_success;
                    }
                }
            }

            delay(2000);

            while(millis() - sim_time < _simTimeout)
            {
                if (available())
                {
                    if (strstr(streamData, "ACTIVE") && strstr(streamData, "DEACTIVED") == 0)
                    {
                        return true;
                        // pdn_status = sim7000_cnact_success;
                    }
                }
            }

            return true;
        }

        String getIMEI()
        {
            uint32_t dev_time = millis();

            streamPrint(BLINEKR_CMD_GSN_REQ);

            char _imei[16];

            while(millis() - dev_time < _simTimeout)
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

            while(millis() - dev_time < _simTimeout)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
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

            streamPrint(BLINKER_CMD_CCID_REQ);

            char _iccid[21];

            while(millis() - dev_time < _simTimeout)
            {
                if (available())
                {
                    BLINKER_LOG_ALL(BLINKER_F("get ICCID: "), streamData, 
                                    BLINKER_F(", length: "), strlen(streamData));
                    if (strlen(streamData) == 20)
                    {
                        strcpy(_iccid, streamData);
                    }
                }
            }

            dev_time = millis();

            while(millis() - dev_time < _simTimeout)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
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

        bool powerCheck()
        {
            // streamPrint(BLINKER_CMD_AT);
            streamPrint("ATE0");
            uint32_t dev_time = millis();

            while(millis() - dev_time < _simTimeout)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("ATE0"));
                    }
                }
            }

            if (!checkPDN()) return false;

            // BLINKER_LOG_ALL(BLINKER_F("power check"));
            // streamPrint(BLINKER_CMD_AT);
            // uint32_t dev_time = millis();

            // while(millis() - dev_time < _simTimeout)
            // {
            //     if (available())
            //     {
            //         if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("power on"));
                        
            //             // SAPBR();

                        return true;
            //         }
            //     }
            // }

            // return false;
        }

        bool isReboot()
        {
            streamPrint(BLINKER_CMD_AT);
            uint32_t dev_time = millis();

            while(millis() - dev_time < _simTimeout)
            {
                if (available())
                {
                    BLINKER_LOG_ALL(BLINKER_F("data len: "), strlen(streamData));
                    if (strncmp(streamData, BLINKER_CMD_AT, 2) == 0 || \
                        (strlen(streamData) != 0 && strncmp(streamData, BLINKER_CMD_OK, 2) != 0 && \
                        strncmp(streamData, BLINKER_CMD_ERROR, 5) != 0))
                    {
                        BLINKER_LOG_ALL(BLINKER_F("device reboot"));
                        
                        // SAPBR();
                        // streamPrint("ATE0");

                        return true;
                    }

                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0) return false;
                    // else if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    // {
                    //     BLINKER_LOG_ALL(BLINKER_F("device not reboot"));
                    //     return false;
                    // }                    
                }
            }

            return false;
        }

        bool isAlive()
        {
            streamPrint(BLINKER_CMD_AT, "isAlive");
            uint32_t dev_time = millis();

            while(millis() - dev_time < _simTimeout * 2)
            {
                if (available()) 
                {
                    BLINKER_LOG_ALL(BLINKER_F("alive"));
                    return true;
                }
            }

            BLINKER_LOG_ALL(BLINKER_F("not alive"));

            return false;
        }

        void flush()
        {
            if (isFresh) free(streamData);

            BLINKER_LOG_ALL(BLINKER_F("flush sim7000"));
        }

        bool checkStream(char * data)
        {
            bool _check_ = strcmp(data, streamData);

            BLINKER_LOG_ALL(BLINKER_F("checkStream: "), _check_);

            return _check_ == 0;
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

            // char _data[BLINKER_SIM7000_DATA_BUFFER_SIZE];// = { '\0' };
            // memset(_data, '\0', BLINKER_SIM7000_DATA_BUFFER_SIZE);

            if (stream->available())
            {
                // strcpy(_data, stream->readStringUntil('\n').c_str());
                String _data = stream->readStringUntil('\n');
                BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), _data);
                // _data[strlen(_data) - 1] = '\0';
                if (isFresh) 
                {
                    free(streamData);
                    isFresh = false;
                }

                if (_data.length() <= 1) return false;
                
                streamData = (char*)malloc((_data.length() + 1)*sizeof(char));
                strcpy(streamData, _data.c_str());
                if (_data.length() > 0) streamData[_data.length() - 1] = '\0';
                isFresh = true;
                return true;
//                 // streamData = "";
//                 // memset(streamData, '\0', 128);
//                 if (isFresh) free(streamData);
//                 streamData = (char*)malloc(1*sizeof(char));

//                 // strcpy(streamData, stream->readStringUntil('\n').c_str());

//                 // int16_t dNum = strlen(streamData);

//                 // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), streamData,
//                 //                 BLINKER_F(", dNum: "), dNum);
//                 // // stream->readString();
                
//                 int16_t dNum = 0;
//                 int c_d = timedRead();
//                 while (dNum < BLINKER_MAX_READ_SIZE && 
//                     c_d >=0 && c_d != '\n')
//                 {
//                     // if (c_d != '\r')
//                     {
//                         streamData[dNum] = (char)c_d;
//                         dNum++;
//                         streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
//                     }

//                     c_d = timedRead();
//                 }
//                 // dNum++;
//                 // // // streamData = (char*)realloc(streamData, dNum*sizeof(char));

//                 // streamData[dNum-1] = '\0';
//                 // stream->flush();
// // 7b2264657461696c223a207b2262726f6b6572223a2022616c6979756e222c20226465766963654e616d65223a20223237383636394232304d3235423634323230354e33435850222c2022696f744964223a20225346455467613255784b784e386a69716e4e516730303130356435343030222c2022696f74546f6b656e223a20226331353530643464346334623432666338376431343639373333363166353539222c202270726f647563744b6579223a20224a67434762486c6e64677a222c202275756964223a20223733633762356134623266323231633061373264376234313238653430323337227d2c20226d657373616765223a20313030307d

//                 // BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
//                 //                 BLINKER_F(" , dNum: "), dNum);
//                 BLINKER_LOG_FreeHeap_ALL();
                
//                 if (dNum < BLINKER_MAX_READ_SIZE && dNum > 0)
//                 {
//                     // if (streamData[dNum - 1] == '\r')
//                     streamData[dNum - 1] = '\0';
//                     BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
//                                     BLINKER_F(" , dNum: "), dNum);

//                     isFresh = true;
//                     return true;
//                 }
//                 else
//                 {
//                     return false;
//                 }
            }
            else
            {
                return false;
            }
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

        // time_t  _ntpTime = 0;

        float   _timezone = 8.0;

        void streamPrint(const String & s, String from = "")
        {
            stream->println(s);
            BLINKER_LOG_ALL(from, BLINKER_F(" SIM: "), s);
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
};

#endif
