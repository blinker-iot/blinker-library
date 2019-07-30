#ifndef BLINKER_AIR202_H
#define BLINKER_AIR202_H

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

// #if defined(ESP32)
//     #include <HardwareSerial.h>

//     HardwareSerial *HSerial_API;
// #else
//     #include <SoftwareSerial.h>

//     SoftwareSerial *SSerial_API;
// #endif

#include <time.h>

#define BLINKER_AIR202_DATA_BUFFER_SIZE 1024

enum air202_status_t
{
    air202_cgtt_state_ver_check,
    air202_cgtt_state_ver_check_success,
    air202_cgtt_state,
    air202_cgtt_state_req,
    air202_cgtt_state_success,
    air202_sapbar_pdp_req,
    air202_sapbar_pdp_success,
    air202_sapbar_pdp_failed,
    air202_sapbar_apn_req,
    air202_sapbar_apn_success,
    air202_sapbar_apn_failed,
    air202_sapbar_save_req,
    air202_sapbar_save_success,
    air202_sapbar_save_failed,
    air202_sapbar_fresh_req,
    air202_sapbar_fresh_success,
    air202_sapbar_fresh_failed,
};

class BlinkerAIR202
{
    public :
        BlinkerAIR202() :
            isHWS(false)
        {}

        ~BlinkerAIR202() { flush(); }

        time_t  _ntpTime = 0;

        void setStream(Stream& s, bool isHardware, blinker_callback_t _func)
        { stream = &s; isHWS = isHardware; listenFunc = _func; }

        void setTimezone(float tz)  { _timezone = tz; }
        String lang()                { return _LANG; }
        String lat()                 { return _LAT; }
        // int16_t year()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_year + 1900;
        //     }
        //     return -1;
        // }

        // int8_t  month()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_mon + 1;
        //     }
        //     return -1;
        // }

        // int8_t  mday()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_mday;
        //     }
        //     return -1;
        // }

        // int8_t  wday()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_wday;
        //     }
        //     return -1;
        // }

        // int8_t  hour()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_hour;
        //     }
        //     return -1;
        // }

        // int8_t  minute()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_min;
        //     }
        //     return -1;
        // }

        // int8_t  second()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_sec;
        //     }
        //     return -1;
        // }

        // time_t  time()
        // {
        //     if (_ntpTime)
        //     {
        //         return _ntpTime - _timezone * 3600;
        //     }
        //     return millis();
        // }

        // int32_t dtime()
        // {
        //     if (_ntpTime)
        //     {
        //         struct tm timeinfo;
        //         #if defined(ESP8266) || defined(__AVR__)
        //             gmtime_r(&_ntpTime, &timeinfo);
        //         #elif defined(ESP32)
        //             localtime_r(&_ntpTime, &timeinfo);
        //         #endif
                
        //         return timeinfo.tm_hour * 60 * 60 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
        //     }
        //     return -1;
        // }

        bool getNTP(float tz = 8.0)
        {
            streamPrint(BLINKER_CMD_CNTP_REQ);
            uint32_t os_time = millis();

            while(millis() - os_time < _airTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        break;
                    }
                }
            }

            streamPrint(BLINKER_CMD_CCLK_REQ);
            os_time = millis();

            while(millis() - os_time < _airTimeout * 10)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CCLK)
                    {
                        struct tm timeinfo;

                        timeinfo.tm_year = _masterAT->getParam(0).substring(2, 4).toInt() + 130;
                        timeinfo.tm_mon  = _masterAT->getParam(0).substring(5, 7).toInt() - 1;
                        timeinfo.tm_mday = _masterAT->getParam(0).substring(8, 10).toInt() - 1;                        
                        
                        timeinfo.tm_hour = _masterAT->getParam(1).substring(0, 2).toInt();
                        timeinfo.tm_min  = _masterAT->getParam(1).substring(3, 5).toInt();
                        timeinfo.tm_sec  = _masterAT->getParam(1).substring(6, 8).toInt();

                        // BLINKER_LOG_ALL(BLINKER_F("year: "), timeinfo.tm_year);
                        // BLINKER_LOG_ALL(BLINKER_F("mon: "), timeinfo.tm_mon);
                        // BLINKER_LOG_ALL(BLINKER_F("mday: "), timeinfo.tm_mday);
                        // BLINKER_LOG_ALL(BLINKER_F("hour: "), timeinfo.tm_hour);
                        // BLINKER_LOG_ALL(BLINKER_F("mins: "), timeinfo.tm_min);
                        // BLINKER_LOG_ALL(BLINKER_F("secs: "), timeinfo.tm_sec);
                        
                        #if defined(ESP8266) || defined(ESP32)
                        _ntpTime = mktime(&timeinfo);
                        #else
                        _ntpTime = mk_gmtime(&timeinfo);
                        #endif

                        BLINKER_LOG_ALL(BLINKER_F("year: "), timeinfo.tm_year);
                        BLINKER_LOG_ALL(BLINKER_F("mon: "), timeinfo.tm_mon);
                        BLINKER_LOG_ALL(BLINKER_F("mday: "), timeinfo.tm_mday);
                        BLINKER_LOG_ALL(BLINKER_F("hour: "), timeinfo.tm_hour);
                        BLINKER_LOG_ALL(BLINKER_F("mins: "), timeinfo.tm_min);
                        BLINKER_LOG_ALL(BLINKER_F("secs: "), timeinfo.tm_sec);

                        // ::delay(200);

                        // _ntpTime = mktime(&timeinfo);
                        // _ntpTime -= _timezone * 3600;

                        BLINKER_LOG_ALL(BLINKER_F("==_ntpTime: "), _ntpTime);
                        BLINKER_LOG_ALL(BLINKER_F("==_ntpTime: "), timeinfo.tm_hour);
                        BLINKER_LOG_ALL(BLINKER_F("==Current time: "), asctime(&timeinfo));

                        free(_masterAT);
                        return true;
                    }

                    free(_masterAT);
                }
            }
            return false;
        }

        bool getAMGSMLOC(float tz = 8.0)
        {
            uint32_t os_time = millis();
            streamPrint(BLINKER_CMD_AMGSMLOC_REQ);

            while(millis() - os_time < _airTimeout * 10)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_AMGSMLOC &&
                        _masterAT->getParam(0) == " 0")
                    {
                        strcpy(_LANG, _masterAT->getParam(1).c_str());
                        strcpy(_LAT, _masterAT->getParam(2).c_str());

                        BLINKER_LOG_ALL(BLINKER_F("LANG.: "), _LANG);
                        BLINKER_LOG_ALL(BLINKER_F("LAT.: "), _LAT);

                        struct tm timeinfo;

                        timeinfo.tm_year = _masterAT->getParam(3).substring(0, 4).toInt() - 1870;
                        timeinfo.tm_mon  = _masterAT->getParam(3).substring(5, 7).toInt() - 1;
                        timeinfo.tm_mday = _masterAT->getParam(3).substring(8, 10).toInt() - 1;

                        BLINKER_LOG_ALL(BLINKER_F("year: "), timeinfo.tm_year);
                        BLINKER_LOG_ALL(BLINKER_F("mon: "), timeinfo.tm_mon);
                        BLINKER_LOG_ALL(BLINKER_F("mday: "), timeinfo.tm_mday);
                        
                        timeinfo.tm_hour = _masterAT->getParam(4).substring(0, 2).toInt();
                        timeinfo.tm_min  = _masterAT->getParam(4).substring(3, 5).toInt();
                        timeinfo.tm_sec  = _masterAT->getParam(4).substring(6, 8).toInt();

                        BLINKER_LOG_ALL(BLINKER_F("hour: "), timeinfo.tm_hour);
                        BLINKER_LOG_ALL(BLINKER_F("mins: "), timeinfo.tm_min);
                        BLINKER_LOG_ALL(BLINKER_F("secs: "), timeinfo.tm_sec);

                        #if defined(ESP8266) || defined(ESP32)
                        _ntpTime = mktime(&timeinfo);
                        #else
                        _ntpTime = mk_gmtime(&timeinfo);
                        #endif
                        // _ntpTime -= _timezone * 3600;

                        BLINKER_LOG_ALL(BLINKER_F("_ntpTime: "), _ntpTime);

                        free(_masterAT);
                        return true;
                    }

                    free(_masterAT);
                }
            }
            return false;
        }

        int checkCGTT()
        {
            uint32_t http_time = millis();
            air202_status_t cgtt_status = air202_cgtt_state_ver_check;

            streamPrint(BLINKER_CMD_CGMMR_REQ);

            cgtt_status = air202_cgtt_state_ver_check;

            while(millis() - http_time < _airTimeout)
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

            streamPrint(BLINKER_CMD_CGQTT_REQ);
            cgtt_status = air202_cgtt_state;
            http_time = millis();

            while(millis() - http_time < _airTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CGATT &&
                        _masterAT->getParam(0).toInt() == 1)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_cgtt_state_req"));
                        cgtt_status = air202_cgtt_state_req;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (cgtt_status != air202_cgtt_state_req) return false;
            // http_time = millis();

            // while(millis() - http_time < _airTimeout)
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

            // stream->println(BLINKER_CMD_CGQTT_REQ);

            // cgtt_status = air202_cgtt_state;
        }

        bool powerCheck()
        {
            streamPrint(BLINKER_CMD_AT);
            streamPrint("ATE0");

            if (!checkCGTT()) return false;
            
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

                        return SAPBR();
                    }
                }
            }

            return false;
        }

        bool SAPBR()
        {
            streamPrint(STRING_format(BLINEKR_CMD_SAPBR_REQ) + \
                        "=3,1,\"CONTYPE\",\"GPRS\"");

            air202_status_t sapbar_status = air202_sapbar_pdp_req;
            uint32_t dev_time = millis();

            while(millis() - dev_time < _airTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_pdp_success"));

                        sapbar_status = air202_sapbar_pdp_success;
                        break;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_pdp_success) return false;

            streamPrint(STRING_format(BLINEKR_CMD_SAPBR_REQ) + \
                        "=3,1,\"APN\",\"CMNET\"");

            sapbar_status = air202_sapbar_apn_req;
            dev_time = millis();

            while(millis() - dev_time < _airTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_apn_success"));

                        sapbar_status = air202_sapbar_apn_success;
                        break;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_apn_success) return false;

            streamPrint(STRING_format(BLINEKR_CMD_SAPBR_REQ) + "=5,1");

            sapbar_status = air202_sapbar_save_req;
            dev_time = millis();

            while(millis() - dev_time < _airTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_save_success"));

                        sapbar_status = air202_sapbar_save_success;
                        break;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_save_success) return false;

            streamPrint(STRING_format(BLINEKR_CMD_SAPBR_REQ) + "=1,1");

            sapbar_status = air202_sapbar_fresh_req;
            dev_time = millis();

            while(millis() - dev_time < _airTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_sapbar_fresh_success"));

                        sapbar_status = air202_sapbar_fresh_success;
                        break;
                    }
                }
            }

            if (sapbar_status != air202_sapbar_fresh_success) return false;

            return true;
        }

        String getIMEI()
        {
            uint32_t dev_time = millis();

            streamPrint(BLINEKR_CMD_CGSN_REQ);

            char _imei[16];

            while(millis() - dev_time < _airTimeout)
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

            while(millis() - dev_time < _airTimeout)
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

            streamPrint(BLINKER_CMD_ICCID_REQ);

            char _iccid[21];

            while(millis() - dev_time < _airTimeout)
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

            while(millis() - dev_time < _airTimeout)
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

        bool isReboot()
        {
            streamPrint(BLINKER_CMD_AT);
            uint32_t dev_time = millis();

            while(millis() - dev_time < _airTimeout)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_AT, 2) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("device reboot"));
                        
                        // SAPBR();
                        streamPrint("ATE0");

                        return true;
                    }
                    // else if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    // {
                    //     BLINKER_LOG_ALL(BLINKER_F("device not reboot"));
                    //     return false;
                    // }                    
                }
            }
            streamPrint("ATE0");

            return false;
        }

        void flush()
        {
            if (isFresh) free(streamData);
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        blinker_callback_t listenFunc = NULL;
        Stream* stream;
        // char    streamData[128];
        char*   streamData;
        bool    isFresh = false;
        bool    isHWS = false;
        uint16_t    _airTimeout = 1000;

        // time_t  _ntpTime = 0;

        float   _timezone = 8.0;

        char    _LANG[14];
        char    _LAT[14];
        // int16_t _year = -1;
        // int8_t  _mon  = -1;
        // int8_t  _mday = -1;
        // int16_t _hour = -1;
        // int8_t  _mins = -1;
        // int8_t  _secs = -1;

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

            // char _data[BLINKER_AIR202_DATA_BUFFER_SIZE];// = { '\0' };
            // memset(_data, '\0', BLINKER_AIR202_DATA_BUFFER_SIZE);

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
//                 // BLINKER_LOG_FreeHeap_ALL();
                
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
};

// BlinkerAIR202 BLINKER_AIR202;

#endif
