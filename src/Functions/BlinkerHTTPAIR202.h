#ifndef BLINKER_HTTP_AIR202_H
#define BLINKER_HTTP_AIR202_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #include "Adapters/BlinkerSerialM                                         QTT.h"
#include "../Blinker/BlinkerATMaster.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

// #if defined(ESP32)
//     #include <HardwareSerial.h>

//     HardwareSerial *HSerial_HTTP;
// #else
//     #include <SoftwareSerial.h>

//     SoftwareSerial *SSerial_HTTP;
// #endif

#define BLINKER_HTTP_AIR202_DEFAULT_TIMEOUT 5000UL
#define BLINKER_HTTP_AIR202_DATA_BUFFER_SIZE 1024

enum air202_http_status_t
{
    air202_init,
    air202_init_success,
    air202_ver_check,
    air202_ver_check_success,
    air202_cgtt,
    air202_cgtt_resp,
    air202_cgtt_success,
    air202_spbar_1,
    air202_spbar_1_success,
    air202_spbar_2,
    air202_spbar_2_success,
    air202_http_init,
    air202_http_init_success,
    air202_http_init_failed,
    air202_http_para_set,
    air202_http_para_set_success,
    air202_http_para_set_failed,
    air202_http_start,
    air202_http_start_success,
    air202_http_start_failed,
    air202_http_data_set,
    air202_http_data_set_success,
    air202_http_data_set_failed,
    air202_http_data_post,
    air202_http_data_post_success,
    air202_http_data_post_failed,
    air202_http_upload_success,
    air202_http_upload_failed,
    air202_http_read_response,
    air202_http_read_success,
    air202_http_read_failed,
    air202_http_read_paylaod,
    air202_http_end,
    air202_http_end_failed,
    air202_http_end_success
};

enum air202_status_sap_t
{
    air202_sap_cgtt_state_ver_check,
    air202_sap_cgtt_state_ver_check_success,
    air202_sap_cgtt_state,
    air202_sap_cgtt_state_resp,
    air202_sap_cgtt_state_success,
    air202_sap_sapbar_pdp_REQ,
    air202_sap_sapbar_pdp_success,
    air202_sap_sapbar_pdp_failed,
    air202_sap_sapbar_apn_REQ,
    air202_sap_sapbar_apn_success,
    air202_sap_sapbar_apn_failed,
    air202_sap_sapbar_save_REQ,
    air202_sap_sapbar_save_success,
    air202_sap_sapbar_save_failed,
    air202_sap_sapbar_fresh_REQ,
    air202_sap_sapbar_fresh_success,
    air202_sap_sapbar_fresh_failed,
};

class BlinkerHTTPAIR202
{
    public :
        BlinkerHTTPAIR202(Stream& s, bool isHardware, blinker_callback_t func)
        { stream = &s; isHWS = isHardware; listenFunc = func; }

        ~BlinkerHTTPAIR202() { flush(); }

        void streamPrint(const String & s)
        {
            // stream->flush();
            BLINKER_LOG_ALL(s);
            stream->println(s);
        }

        int checkCGTT()
        {
            uint32_t http_time = millis();
            air202_http_status_t http_status = air202_init;

            streamPrint(BLINKER_CMD_AT);

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_init_success"));
                        http_status = air202_init_success;
                        break;
                    }
                }
            }

            if (http_status != air202_init_success) return false;

            streamPrint(BLINKER_CMD_CGMMR_REQ);

            http_status = air202_ver_check;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_CGMMR_RESP) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_ver_check_success"));
                        http_status = air202_ver_check_success;
                        break;
                    }
                }
            }

            if (http_status != air202_ver_check_success) return false;

            streamPrint(BLINKER_CMD_CGQTT_REQ);

            http_status = air202_cgtt;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CGATT &&
                        _masterAT->getParam(0).toInt() == 1)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_cgtt_resp"));
                        http_status = air202_cgtt_resp;
                    }

                    free(_masterAT);

                    break;
                }
            }

            if (http_status != air202_cgtt_resp) return false;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_cgtt_success"));
                        http_status = air202_cgtt_success;
                        break;
                    }
                }
            }

            if (http_status != air202_cgtt_success) return false;

            // stream->println(BLINKER_CMD_CGQTT_REQ);

            // http_status = air202_cgtt;
        }

        bool begin(String host, String uri) { _host = host; _uri = uri; }
        void setTimeout(uint16_t timeout)   { _httpTimeout = timeout; }
        bool GET()
        {
            // streamPrint(STRING_format(BLINKER_CMD_MDISCONNECT_REQ));
            // uint32_t mqtt_time = millis();
            // uint8_t status_get = 0;

            // while(millis() - mqtt_time < _httpTimeout)
            // {
            //     if (available())
            //     {
            //         BLINKER_LOG_ALL(BLINKER_F("== disconnect available =="));
            //         if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("mqtt disconnect"));
            //             // return true;
            //             break;
            //         }
            //     }
            // }

            // streamPrint(STRING_format(BLINKER_CMD_MIPCLOSE_REQ));
            // mqtt_time = millis();

            // while(millis() - mqtt_time < _httpTimeout)
            // {
            //     if (available())
            //     {
            //         BLINKER_LOG_ALL(BLINKER_F("== disconnect available =="));
            //         if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("mqtt disconnect"));
            //             // return true;
            //             break;
            //         }
            //     }
            // }

            // streamPrint(STRING_format(BLINKER_CMD_CIPSHUT_REQ));
            // mqtt_time = millis();

            // while(millis() - mqtt_time < _httpTimeout)
            // {
            //     if (available())
            //     {
            //         BLINKER_LOG_ALL(BLINKER_F("== disconnect available =="));
            //         if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("mqtt disconnect"));
            //             return true;
            //             // break;
            //         }
            //     }
            // }
            
            streamPrint(BLINKER_CMD_HTTPINIT_REQ);
            uint32_t http_time = millis();
            air202_http_status_t http_status = air202_http_init;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_init_success"));
                        http_status = air202_http_init_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_init_success) 
            {
                BLINKER_LOG_ALL(BLINKER_F("air202_http_init_failed"));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_HTTPPARA_REQ) + "=\"CID\",1");
            
            http_status = air202_http_para_set;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_para_set_success 1"));
                        http_status = air202_http_para_set_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_para_set_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_HTTPPARA_REQ) + \
                        "=\"URL\",\"" + _host + _uri + "\"");
            
            http_status = air202_http_para_set;
            http_time = millis();

            while(millis() - http_time < _httpTimeout*2)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_para_set_success 2"));
                        http_status = air202_http_para_set_success;
                        break;
                    }
                    else if (strcmp(streamData, BLINKER_CMD_ERROR) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_para_set_failed 2"));
                        // streamPrint("AT+CPOWD");
                        break;
                    }
                }
            }

            // http_status = air202_http_para_set_success;// TBD

            if (http_status != air202_http_para_set_success) 
            {
                // streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
                // streamPrint("AT+CPOWD");
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_HTTPACTION_REQ) + "=0");
            
            http_status = air202_http_start;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_start_success"));
                        http_status = air202_http_start_success;
                        break;
                    }
                    else if (strcmp(streamData, BLINKER_CMD_ERROR) == 0)
                    {
                        streamPrint(STRING_format(BLINKER_CMD_HTTPREAD_REQ));
                        ::delay(500);

                        streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
                        ::delay(500);

                        break;
                    }
                }
            }

            if (http_status != air202_http_start_success) 
            {
                streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
                return false;
            }

            http_time = millis();

            while(millis() - http_time < _httpTimeout*10)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_HTTPACTION)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_upload_success"));
                        http_status = air202_http_upload_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);

                    // break;
                }
            }

            // if (http_status != air202_http_upload_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_HTTPREAD_REQ));
            http_status = air202_http_read_response;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_HTTPREAD)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_read_success"));
                        http_status = air202_http_read_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);

                    // break;
                }
            }

            if (http_status != air202_http_read_success) return false;

            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) != 0)
                    {
                        // payload = streamData;

                        if (isFreshPayload) free(payload);

                        isFreshPayload = true;
                        // char data_buff[1024] = { '\0' };
                        payload = (char*)malloc((strlen(streamData) + 1)*sizeof(char));
                        strcpy(payload, streamData);

                        BLINKER_LOG_ALL(BLINKER_F("payload: "), payload);
                    }
                }
            }

            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        break;
                    }
                }
            }

            streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
            http_status = air202_http_end;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_end_success"));
                        http_status = air202_http_end_success;
                        return true;
                    }
                }
            }

            if (http_status != air202_http_end_success) return false;            
        }

        bool POST(String _msg, String _type, String _application)
        {
            
            
            uint32_t http_time = millis();
            air202_http_status_t http_status = air202_http_init;

            streamPrint(BLINKER_CMD_HTTPINIT_REQ);

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_init_success"));
                        http_status = air202_http_init_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_init_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_HTTPPARA_REQ) + "=\"CID\",1");
            http_status = air202_http_para_set;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_para_set_success 1"));
                        http_status = air202_http_para_set_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_para_set_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_HTTPPARA_REQ) + \
                        "=\"URL\",\"" + _host + _uri + "\"");
            
            http_status = air202_http_para_set;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_para_set_success 2"));
                        http_status = air202_http_para_set_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_para_set_success) return false;

            // streamPrint(STRING_format(BLINKER_CMD_HTTPPARA_REQ) + \
            //             "=\"" + _type + "\",\"" + _application + "\"");
            
            // http_status = air202_http_para_set;
            // http_time = millis();

            // while(millis() - http_time < _httpTimeout)
            // {
            //     if (available())
            //     {
            //         if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            //         {
            //             BLINKER_LOG_ALL(BLINKER_F("air202_http_para_set_success 3"));
            //             http_status = air202_http_para_set_success;
            //             break;
            //         }
            //     }
            // }

            // if (http_status != air202_http_para_set_success) return false;
            // _msg.replace("\"", "\\22");

            // _msg = "\"" + _msg + "\"";
            streamPrint(STRING_format(BLINKER_CMD_HTTPDATA_REQ) + \
                        "=" + _msg.length() + ",10000");
            
            http_status = air202_http_data_set;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_DOWNLOAD) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_data_set_success"));
                        http_status = air202_http_data_set_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_data_set_success) return false;

            streamPrint(_msg);
            http_status = air202_http_data_post;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_data_post_success"));
                        http_status = air202_http_data_post_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_data_post_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_HTTPACTION_REQ) + "=1");

            http_status = air202_http_start;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_start_success"));
                        http_status = air202_http_start_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_start_success) 
            {
                streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
                return false;
            }
            http_time = millis();

            while(millis() - http_time < _httpTimeout*2)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_HTTPACTION)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_upload_success"));
                        http_status = air202_http_upload_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);

                    // break;
                }
            }

            if (http_status != air202_http_upload_success) 
            {
                streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_HTTPREAD_REQ));
            
            http_status = air202_http_read_response;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_HTTPREAD)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_read_success, data len: "), _masterAT->getParam(0));
                        http_status = air202_http_read_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);

                    // break;
                }
            }

            if (http_status != air202_http_read_success) return false;
            
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) != 0)
                    {
                        // payload = streamData;
                        if (isFreshPayload) free(payload);

                        isFreshPayload = true;
                        char data_buff[1024] = { '\0' };
                        payload = (char*)malloc((strlen(streamData) + 1)*sizeof(char));
                        strcpy(payload, streamData);

                        BLINKER_LOG_ALL(BLINKER_F("payload: "), payload);
                    }
                }
            }

            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        break;
                    }
                }
            }

            streamPrint(STRING_format(BLINKER_CMD_HTTPERM_REQ));
            
            http_status = air202_http_end;
            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("air202_http_end_success"));
                        http_status = air202_http_end_success;
                        break;
                    }
                }
            }

            if (http_status != air202_http_end_success) return false;

            return true;
        }

        String getString()
        {
            return payload;//TBD
        }

        void flush()
        {
            if (isFreshPayload) free(payload); 
            if (isFresh) free(streamData);
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        Stream* stream;
        // String  streamData;
        // char    streamData[1024];
        char*   streamData;
        // String  payload = "";
        char*   payload;
        bool    isFreshPayload = false;
        bool    isFresh = false;
        bool    isHWS = false;
        String  _host;
        String  _uri;

        blinker_callback_t listenFunc = NULL;

        uint16_t _httpTimeout = BLINKER_HTTP_AIR202_DEFAULT_TIMEOUT;

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
                //     if (!SSerial_HTTP->isListening())
                //     {
                //         SSerial_HTTP->listen();
                //         ::delay(100);
                //     }
                // #endif
                if (listenFunc) listenFunc();
            }

            // char _data[BLINKER_HTTP_AIR202_DATA_BUFFER_SIZE];// = { '\0' };
            // memset(_data, '\0', BLINKER_HTTP_AIR202_DATA_BUFFER_SIZE);

            // if (!isFresh) streamData = (char*)malloc(BLINKER_HTTP_AIR202_DATA_BUFFER_SIZE*sizeof(char));
            // else memset(streamData, '\0', BLINKER_HTTP_AIR202_DATA_BUFFER_SIZE);

            if (stream->available())
            {
                // strcpy(_data, stream->readStringUntil('\n').c_str());
                String _data = stream->readStringUntil('\n');
                BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), _data, BLINKER_F(", len: "), _data.length());
                // _data[strlen(_data) - 1] = '\0';
                if (isFresh) 
                {
                    free(streamData);
                    isFresh = false;
                }

                if (_data.length() <= 1) return false;
                
                streamData = (char*)malloc((_data.length() + 1)*sizeof(char));
                strcpy(streamData, _data.c_str());
                if (_data.length() > 0 && streamData[_data.length() - 1] == '\r') streamData[_data.length() - 1] = '\0';
                isFresh = true;
                return true;
                // if (isFresh) free(streamData);
                // streamData = (char*)malloc(1*sizeof(char));
                // // streamData = "";

                // // memset(streamData, '\0', 1024);

                // // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), stream->readString());

                // // strcpy(streamData, stream->readStringUntil('\n').c_str());

                // // int16_t dNum = strlen(streamData);

                // // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), streamData,
                // //                 BLINKER_F(", dNum: "), dNum);
                
                // int16_t dNum = 0;
                // int c_d = timedRead();
                // while (dNum < BLINKER_MAX_READ_SIZE && 
                //     c_d >=0 && c_d != '\n')
                // {
                //     // if (c_d != '\r')
                //     {
                //         streamData[dNum] = (char)c_d;
                //         dNum++;
                //         streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
                //     }

                //     c_d = timedRead();
                // }
                // // dNum++;
                // // // streamData = (char*)realloc(streamData, dNum*sizeof(char));

                // // streamData[dNum-1] = '\0';

                // // strcpy(streamData, stream->readStringUntil('\n').c_str());
                // // int16_t dNum = strlen(streamData);
                // // streamData[dNum-1] = '\0';

                // // stream->flush();
                
                // // BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
                // //                 BLINKER_F(" , dNum: "), dNum);
                // // BLINKER_LOG_FreeHeap_ALL();
                
                // if (dNum < BLINKER_MAX_READ_SIZE && dNum > 0)
                // {
                //     // if (streamData[dNum - 1] == '\r')
                //     streamData[dNum - 1] = '\0';
                //     BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
                //                     BLINKER_F(" , dNum: "), dNum);

                //     isFresh = true;
                //     return true;
                // }
                // else
                // {
                //     return false;
                // }
            }
            else
            {
                return false;
            }
        }
};

#endif
