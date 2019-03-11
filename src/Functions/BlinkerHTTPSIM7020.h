#ifndef BLINKER_HTTP_SIM7020_H
#define BLINKER_HTTP_SIM7020_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #include "Adapters/BlinkerSerialMQTT.h"
#include "Blinker/BlinkerATMaster.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

// #if defined(ESP32)
//     #include <HardwareSerial.h>

//     HardwareSerial *HSerial_HTTP;
// #else
//     #include <SoftwareSerial.h>

//     SoftwareSerial *SSerial_HTTP;
// #endif

#define BLINKER_HTTP_SIM7020_DEFAULT_TIMEOUT 5000UL

enum sim7020_http_status_t
{
    sim7020_http_creat_req,
    sim7020_http_creat_resp,
    sim7020_http_creat_success,
    sim7020_http_con_req,
    sim7020_http_con_success,
    sim7020_http_send_req,
    sim7020_http_send_success,
    sim7020_http_nmih_wait,
    sim7020_http_nmih_success,
    sim7020_http_nmic_wait,
    sim7020_http_nmic_success,
    sim7020_http_discon_req,
    sim7020_http_discon_success,
    sim7020_http_destroy_req,
    sim7020_http_destroy_success,
};

class BlinkerHTTPSIM7020
{
    public :
        BlinkerHTTPSIM7020(Stream& s, bool isHardware, blinker_callback_t func)
        { stream = &s; isHWS = isHardware; listenFunc = func; }

        ~BlinkerHTTPSIM7020() { flush(); }

        void streamPrint(const String & s)
        {
            // stream->flush();
            BLINKER_LOG_ALL(s);
            stream->println(s);
        }

        bool begin(String host, String uri) { _host = host; _uri = uri; }
        void setTimeout(uint16_t timeout)   { _httpTimeout = timeout; }
        bool GET()
        {
            streamPrint(STRING_format(BLINKER_CMD_CHTTPCREATE_REQ) + \
                        "=" + _host);
            uint32_t http_time = millis();
            sim7020_http_status_t http_status = sim7020_http_creat_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CHTTPCREATE)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_http_creat_resp"));
                        http_status = sim7020_http_creat_resp;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7020_http_creat_resp) return false;

            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_creat_success"));
                    http_status = sim7020_http_creat_success;
                    break;
                }
            }

            if (http_status != sim7020_http_creat_success) return false;

            streamPrint(STRING_format(BLINEKR_CMD_CHTTPCON_REQ) + "=0");
            http_time = millis();
            http_status = sim7020_http_con_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_con_success"));
                    http_status = sim7020_http_con_success;
                    break;
                }
            }

            if (http_status != sim7020_http_con_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CHTTPSEND_REQ) + \
                        "=0,0,\"" + _uri + "\"");
            http_time = millis();
            http_status = sim7020_http_send_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_send_success"));
                    http_status = sim7020_http_send_success;
                    break;
                }
            }

            if (http_status != sim7020_http_send_success) return false;

            http_time = millis();
            http_status = sim7020_http_nmih_wait;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CHTTPNMIH)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_http_nmih_success"));
                        http_status = sim7020_http_nmih_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7020_http_nmih_success) return false;

            http_time = millis();
            http_status = sim7020_http_nmih_wait;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CHTTPNMIH)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_http_nmic_success"));
                        http_status = sim7020_http_nmic_success;

                        if (isFreshPayload) free(payload);

                        isFreshPayload = true;
                        char data_buff[1024] = { '\0' };

                        memcpy(data_buff,strrchr(streamData, ',')+1, strlen(strrchr(streamData, ',')));

                        payload = (char*)malloc((strlen(data_buff)/2 + 1)*sizeof(char));

                        memset(payload, '\0', (strlen(data_buff)/2 + 1));

                        // memcpy(payload,strrchr(streamData, ',')+1, strlen(strrchr(streamData, ',')));

                        for(uint16_t num = 0; num < strlen(data_buff)/2; num++)
                        {
                            payload[num] = (char)(dcode_data(data_buff[num*2])<< 4 | dcode_data(data_buff[num*2+1]));
                        }

                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7020_http_nmih_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CHTTPDISCON_REQ) + "=0");
            http_time = millis();
            http_status = sim7020_http_discon_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_discon_success"));
                    http_status = sim7020_http_discon_success;
                    break;
                }
            }

            if (http_status != sim7020_http_discon_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CHTTPDISTROY_REQ) + "=0");
            http_time = millis();
            http_status = sim7020_http_destroy_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_destroy_success"));
                    http_status = sim7020_http_destroy_success;
                    return true;
                }
            }

            if (http_status != sim7020_http_discon_success) return false;
        }

        bool POST(String _msg, String _type, String _application)
        {
            streamPrint(STRING_format(BLINKER_CMD_CHTTPCREATE_REQ) + \
                        "=" + _host);
            uint32_t http_time = millis();
            sim7020_http_status_t http_status = sim7020_http_creat_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CHTTPCREATE)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_http_creat_resp"));
                        http_status = sim7020_http_creat_resp;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7020_http_creat_resp) return false;

            http_time = millis();

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_creat_success"));
                    http_status = sim7020_http_creat_success;
                    break;
                }
            }

            if (http_status != sim7020_http_creat_success) return false;

            streamPrint(STRING_format(BLINEKR_CMD_CHTTPCON_REQ) + "=0");
            http_time = millis();
            http_status = sim7020_http_con_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_con_success"));
                    http_status = sim7020_http_con_success;
                    break;
                }
            }

            if (http_status != sim7020_http_con_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CHTTPSEND_REQ) + \
                        "=0,1,\"" + _uri + ",4163636570743a202a2f2a0d0a436f6e6" + \
                        "e656374696f6e3a204b6565702d416c6976650d0a557365722d41" + \
                        "67656e743a2053494d434f4d5f4d4f44554c450d0a,\"" + \
                        _type + "\"," + encode(_msg));
            http_time = millis();
            http_status = sim7020_http_send_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_send_success"));
                    http_status = sim7020_http_send_success;
                    break;
                }
            }

            if (http_status != sim7020_http_send_success) return false;

            http_time = millis();
            http_status = sim7020_http_nmih_wait;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CHTTPNMIH)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_http_nmih_success"));
                        http_status = sim7020_http_nmih_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7020_http_nmih_success) return false;

            http_time = millis();
            http_status = sim7020_http_nmih_wait;

            while(millis() - http_time < _httpTimeout)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_CHTTPNMIH)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7020_http_nmic_success"));
                        http_status = sim7020_http_nmic_success;

                        if (isFreshPayload) free(payload);

                        isFreshPayload = true;
                        char data_buff[1024] = { '\0' };

                        memcpy(data_buff,strrchr(streamData, ',')+1, strlen(strrchr(streamData, ',')));

                        payload = (char*)malloc((strlen(data_buff)/2 + 1)*sizeof(char));

                        memset(payload, '\0', (strlen(data_buff)/2 + 1));

                        // memcpy(payload,strrchr(streamData, ',')+1, strlen(strrchr(streamData, ',')));

                        for(uint16_t num = 0; num < strlen(data_buff)/2; num++)
                        {
                            payload[num] = (char)(dcode_data(data_buff[num*2])<< 4 | dcode_data(data_buff[num*2+1]));
                        }

                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7020_http_nmih_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CHTTPDISCON_REQ) + "=0");
            http_time = millis();
            http_status = sim7020_http_discon_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_discon_success"));
                    http_status = sim7020_http_discon_success;
                    break;
                }
            }

            if (http_status != sim7020_http_discon_success) return false;

            streamPrint(STRING_format(BLINKER_CMD_CHTTPDISTROY_REQ) + "=0");
            http_time = millis();
            http_status = sim7020_http_destroy_req;

            while(millis() - http_time < _httpTimeout)
            {
                if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7020_http_destroy_success"));
                    http_status = sim7020_http_destroy_success;
                    return true;
                }
            }

            if (http_status != sim7020_http_discon_success) return false;
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
        char*   payload;
        bool    isFreshPayload = false;
        bool    isFresh = false;
        bool    isHWS = false;
        String  _host;
        String  _uri;

        blinker_callback_t listenFunc = NULL;

        uint16_t _httpTimeout = BLINKER_HTTP_SIM7020_DEFAULT_TIMEOUT;

        char c_hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        int8_t dcode_data(char d_test)
        {
            for (uint8_t num = 0; num < 16; num++)
            {
                if (d_test == c_hex[num]) return num;
            }

            return -1;
        }

        char ecode_data(uint8_t d_test)
        {
            return c_hex[d_test];
        }

        String encode(String data)
        {
            char _d[1024] = {'\0'};

            for(uint16_t num = 0; num < data.length(); num++)
            {
                _d[num*2] = (ecode_data((uint8_t)data[num] >> 4));
                _d[num*2+1] = (ecode_data((uint8_t)data[num] & 0x0F));
            }

            return _d;
        }

        String encode(char data[])
        {
            char _d[1024] = {'\0'};

            for(uint16_t num = 0; num < strlen(data); num++)
            {
                _d[num*2] = (ecode_data(data[num] >> 4));
                _d[num*2+1] = (ecode_data(data[num] & 0x0F));
            }

            return _d;
        }

        // for(uint16_t num = 0; num < strlen(test); num++)
        // {
        //     Serial.print(ecode_data(test[num] >> 4));
        //     Serial.print(ecode_data(test[num] & 0x0F));
        // }
        // Serial.println();

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

            if (stream->available())
            {
                if (isFresh) free(streamData);
                streamData = (char*)malloc(1*sizeof(char));
                // streamData = "";

                // memset(streamData, '\0', 1024);

                // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), stream->readString());

                // strcpy(streamData, stream->readStringUntil('\n').c_str());

                // int16_t dNum = strlen(streamData);

                // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), streamData,
                //                 BLINKER_F(", dNum: "), dNum);
                
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
                // // streamData = (char*)realloc(streamData, dNum*sizeof(char));

                // streamData[dNum-1] = '\0';

                // strcpy(streamData, stream->readStringUntil('\n').c_str());
                // int16_t dNum = strlen(streamData);
                // streamData[dNum-1] = '\0';

                // stream->flush();
                
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
