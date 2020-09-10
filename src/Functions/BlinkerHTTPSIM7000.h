#ifndef BLINKER_HTTP_SIM7000_H
#define BLINKER_HTTP_SIM7000_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

// #include "Adapters/BlinkerSerialMQTT.h"
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

#define BLINKER_HTTP_SIM7000_DEFAULT_TIMEOUT 5000UL
#define BLINKER_HTTP_SIM7000_DATA_BUFFER_SIZE 1024

enum sim7000_http_status_t
{
    sim7000_http_url_set,
    sim7000_http_url_success,
    sim7000_http_body_set,
    sim7000_http_body_success,
    sim7000_http_header_set,
    sim7000_http_header_success,
    sim7000_http_con_set,
    sim7000_http_con_success,
    sim7000_http_state_req,
    sim7000_http_state_success,
    sim7000_http_chead_set,
    sim7000_http_chead_success,
    sim7000_http_ahead_set,
    sim7000_http_ahead_success,
    // sim7000_http_body_set,
    // sim7000_http_body_success,
    sim7000_http_send_set,
    sim7000_http_send_success,
    sim7000_http_get_set,
    sim7000_http_get_success,
    sim7000_http_read_set,
    sim7000_http_read_success,
    sim7000_http_disc_set,
    sim7000_http_disc_success,
};

class BlinkerHTTPSIM7000
{
    public :
        BlinkerHTTPSIM7000(Stream& s, bool isHardware, blinker_callback_t func)
        {
            stream = &s; isHWS = isHardware; listenFunc = func; 
            // streamData = (char*)malloc(BLINKER_HTTP_SIM7000_DATA_BUFFER_SIZE*sizeof(char));
        }

        ~BlinkerHTTPSIM7000() { flush(); }

        void streamPrint(const String & s)
        {
            // stream->flush();
            BLINKER_LOG_ALL(BLINKER_F("HTTP streamPrint: "), s);
            stream->println(s);
        }

        void _streamPrint(const String & s)
        {
            // stream->flush();
            BLINKER_LOG_ALL(BLINKER_F("HTTP streamPrint: "), s);
            stream->print(s);
        }

        void _streamPrint()
        {
            // stream->flush();
            BLINKER_LOG_ALL(BLINKER_F("HTTP streamPrint: "));
            stream->println();
        }

        bool begin(String host, String uri) { _host = host; _uri = uri; }
        void setTimeout(uint16_t timeout)   { _httpTimeout = timeout; }

        void reboot()
        {
            // streamPrint(BLINKER_CMD_CRESET_RESQ);
            // ::delay(500);
            // streamPrint(BLINKER_CMD_AT);
            // streamPrint("ATE0");
        }

        bool GET()
        {
            streamPrint(STRING_format(BLINKER_CMD_SHCONF_REQ) + \
                        "=\"URL\",\"" + _host + "\"");
            uint8_t  h_id = 0;
            uint32_t http_time = millis();
            sim7000_http_status_t http_status = sim7000_http_url_set;

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7000_http_url_set check"));
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7000_http_url_success"));
                        http_status = sim7000_http_url_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_url_success)
            {
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHCONF_REQ) +
                        "=\"BODYLEN\",350");
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_body_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_body_success)
            {
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHCONF_REQ) + \
                        "=\"HEADERLEN\",350");
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_header_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_header_success)
            {
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHCONN_REQ));
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_con_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_con_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHSTATE_REQ) + 
                        "?");
            http_time = millis();

            http_status = sim7000_http_state_req;

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_SHSTATE)
                    {
                        uint8_t get_state = _masterAT->getParam(0).toInt();
                        BLINKER_LOG_ALL(BLINKER_F("sim7000_http_state_req, get_state: "), get_state);
                        http_status = sim7000_http_state_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            // if (http_status != sim7000_http_state_success)
            // {
            //     streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
            //     return false;
            // }

            streamPrint(STRING_format(BLINKER_CMD_SHCHEAD_REQ));
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_chead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_chead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"User-Agent\",\"curl/7.47.0\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Cache-control\",\"no-cache\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Connection\",\"keep-alive\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Accept\",\"*/*\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHREQ_REQ) + \
                        "=\"" + _uri + "\",1");
            http_time = millis();

            http_status = sim7000_http_send_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_send_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_send_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            String get_string;
            uint16_t get_code;
            uint16_t get_len;

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_SHREQ)
                    {
                        get_string = _masterAT->getParam(0);
                        get_code = _masterAT->getParam(1).toInt();
                        get_len = _masterAT->getParam(2).toInt();
                        BLINKER_LOG_ALL(BLINKER_F("get_string: "), get_string, \
                                        BLINKER_F(", get_code: "), get_code, \
                                        BLINKER_F(", get_len: "), get_len);
                        http_status = sim7000_http_get_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7000_http_get_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHREAD_REQ) + \
                        "=0," + get_len + "");
            http_time = millis();

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_SHREAD)
                    {
                        String read_string = _masterAT->getParam(0);
                        BLINKER_LOG_ALL(BLINKER_F("read_string: "), read_string);
                        http_status = sim7000_http_read_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7000_http_read_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            http_time = millis();

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    String http_data = STRING_format(streamData);
                    BLINKER_LOG_ALL(BLINKER_F("http_data: "), http_data);
                    
                    if (isFreshPayload) free(payload);
                    payload = (char*)malloc((http_data.length() + 1)*sizeof(char));
                    memset(payload, '\0', (http_data.length() + 1));
                    strcpy(payload, http_data.c_str());
                    isFreshPayload = true;
                    break;
                }
            }

            streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_disc_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_disc_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }
            return true;
        }

        bool POST(String _msg, String _type, String _application)
        {
            streamPrint(STRING_format(BLINKER_CMD_SHCONF_REQ) + \
                        "=\"URL\",\"" + _host + "\"");
            uint8_t  h_id = 0;
            uint32_t http_time = millis();
            sim7000_http_status_t http_status = sim7000_http_url_set;

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    BLINKER_LOG_ALL(BLINKER_F("sim7000_http_url_set check"));
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("sim7000_http_url_success"));
                        http_status = sim7000_http_url_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_url_success)
            {
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHCONF_REQ) +
                        "=\"BODYLEN\",350");
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_body_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_body_success)
            {
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHCONF_REQ) + \
                        "=\"HEADERLEN\",350");
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_header_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_header_success)
            {
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHCONN_REQ));
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_con_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_con_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHSTATE_REQ) + 
                        "?");
            http_time = millis();

            http_status = sim7000_http_state_req;

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_SHSTATE)
                    {
                        uint8_t get_state = _masterAT->getParam(0).toInt();
                        BLINKER_LOG_ALL(BLINKER_F("sim7000_http_state_req, get_state: "), get_state);
                        http_status = sim7000_http_state_success;
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            // if (http_status != sim7000_http_state_success)
            // {
            //     streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
            //     return false;
            // }

            streamPrint(STRING_format(BLINKER_CMD_SHCHEAD_REQ));
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_chead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_chead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Content-Type\",\"application/json\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Cache-control\",\"no-cache\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Connection\",\"keep-alive\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHAHEAD_REQ) + \
                        "=\"Accept\",\"*/*\"");
            http_time = millis();

            http_status = sim7000_http_ahead_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_ahead_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_ahead_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            uint16_t _msg_len = _msg.length();
            _msg.replace("\"", "\\\"");
            streamPrint(STRING_format(BLINKER_CMD_SHBOD_REQ) + \
                        "=\"" + _msg + "\"," + STRING_format(_msg_len));
            http_time = millis();

            http_status = sim7000_http_body_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_body_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_body_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }
            
            streamPrint(STRING_format(BLINKER_CMD_SHREQ_REQ) + \
                        "=\"" + _uri + "\",3");
            http_time = millis();

            http_status = sim7000_http_send_set;
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_send_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_send_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            String get_string;
            uint16_t get_code;
            uint16_t get_len;

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_SHREQ)
                    {
                        get_string = _masterAT->getParam(0);
                        get_code = _masterAT->getParam(1).toInt();
                        get_len = _masterAT->getParam(2).toInt();
                        BLINKER_LOG_ALL(BLINKER_F("get_string: "), get_string, \
                                        BLINKER_F(", get_code: "), get_code, \
                                        BLINKER_F(", get_len: "), get_len);
                        if (get_code == 200)
                        {
                            http_status = sim7000_http_get_success;
                        }
                        free(_masterAT);
                        break;
                    }

                    free(_masterAT);
                }
            }

            if (http_status != sim7000_http_get_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHREAD_REQ) + \
                        "=0," + get_len + "");
            http_time = millis();

            while(millis() - http_time < _httpTimeout * 4)
            {
                if (available())
                {
                    // _masterAT = new BlinkerMasterAT();
                    // _masterAT->update(STRING_format(streamData));

                    // if (_masterAT->getState() != AT_M_NONE &&
                    //     _masterAT->reqName() == BLINKER_CMD_SHREAD)
                    // {
                    //     String read_string = _masterAT->getParam(0);
                    //     BLINKER_LOG_ALL(BLINKER_F("read_string: "), read_string);
                    //     http_status = sim7000_http_read_success;
                    //     free(_masterAT);
                    //     break;
                    // }

                    // free(_masterAT);

                    BLINKER_LOG_ALL(BLINKER_F("read_string: "), streamData);
                }
            }

            if (http_status != sim7000_http_get_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }

            streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
            http_time = millis();
            
            while(millis() - http_time < _httpTimeout * 2)
            {
                if (available())
                {
                    if (strncmp(streamData, BLINKER_CMD_OK, 2) == 0)
                    {
                        http_status = sim7000_http_disc_success;
                        break;
                    }
                }
            }

            if (http_status != sim7000_http_disc_success)
            {
                streamPrint(STRING_format(BLINKER_CMD_SHDISC_REQ));
                return false;
            }
            return true;
        }

        String getString()
        {
            if (isFreshPayload) return payload;//TBD
            return "";
        }

        void flush()
        {
            if (isFreshPayload) free(payload); 
            if (isFresh) free(streamData);

            BLINKER_LOG_ALL(BLINKER_F("flush sim7000 http"));
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        Stream* stream;
        // String  streamData;
        // char    streamData[1024];
        char*   streamData;
        // char    streamBuffer[2048] = { '\0' };
        char*   payload;
        bool    isFreshPayload = false;
        bool    isFresh = false;
        bool    isHWS = false;
        String  _host;
        String  _uri;

        blinker_callback_t listenFunc = NULL;

        uint16_t _httpTimeout = BLINKER_HTTP_SIM7000_DEFAULT_TIMEOUT;

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
            // char _d[2048] = {'\0'};

            String _d = "";

            BLINKER_LOG_FreeHeap_ALL();
            BLINKER_LOG_ALL("encode data: ", data);
            BLINKER_LOG_ALL("encode len: ", data.length());

            for(uint16_t num = 0; num < data.length(); num++)
            {
                _d += (ecode_data((uint8_t)data.c_str()[num] >> 4));
                _d += (ecode_data((uint8_t)data.c_str()[num] & 0x0F));
            }

            BLINKER_LOG_ALL("encode data: ", _d);

            return _d;
        }

        // String encode(char data[])
        // {
        //     // char _d[2048] = {'\0'};
        //     String _d = "";

        //     BLINKER_LOG_FreeHeap_ALL();
        //     BLINKER_LOG_ALL("encode data: ", data);
        //     BLINKER_LOG_ALL("encode len: ", strlen(data));

        //     for(uint16_t num = 0; num < strlen(data); num++)
        //     {
        //         _d += (ecode_data(data[num] >> 4));
        //         _d += (ecode_data(data[num] & 0x0F));
        //     }

        //     BLINKER_LOG_ALL("encode data: ", _d);

        //     return _d;
        // }

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

        bool available(const char *str)
        {
            uint32_t http_time = millis();
            char _data[1024];// = { '\0' };
            memset(_data, '\0', 1024);

            BLINKER_LOG_ALL(BLINKER_F("available str: "), str);
            while(millis() - http_time < _httpTimeout * 4)
            {
                if (stream->available())
                {
                    // if (isFresh) free(streamData);
                    // streamData = (char*)malloc(1*sizeof(char));
                    // isFresh = true;
                    // if (strspn(streamData, str)) return true;
                    // // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), stream->readStringUntil('\n'));
                    
                    strcpy(_data, stream->readStringUntil('\n').c_str());
                    BLINKER_LOG_ALL(BLINKER_F("handleSerial rs available(const char *str): "), _data);
                    if (strstr(_data, str))
                    {
                        _data[strlen(_data) - 1] = '\0';
                        if (isFresh) free(streamData);
                        streamData = (char*)malloc((strlen(_data) + 1)*sizeof(char));
                        strcpy(streamData, _data);
                        isFresh = true;
                        return true;
                    }
                    BLINKER_LOG_ALL(BLINKER_F("strstr(_data, str): "), strstr(_data, str));
                    memset(_data, '\0', 1024);
                }
                yield();
                // delay(100);
            }

            return false;
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

            // char _data[BLINKER_HTTP_SIM7000_DATA_BUFFER_SIZE];// = { '\0' };
            // memset(_data, '\0', BLINKER_HTTP_SIM7000_DATA_BUFFER_SIZE);

            // if (!isFresh) streamData = (char*)malloc(BLINKER_HTTP_SIM7000_DATA_BUFFER_SIZE*sizeof(char));

            if (stream->available())
            {
                // strcpy(_data, stream->readStringUntil('\n').c_str());
                String _data = stream->readStringUntil('\n');
                // if ( _data[_data.length() - 1] == '\n')
                //     _data[_data.length() - 1] = '\0';
                BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), _data);
                if (isFresh) 
                {
                    free(streamData);
                    isFresh = false;
                }

                if (_data.length() <= 1) return false;
                
                streamData = (char*)malloc((_data.length() + 1)*sizeof(char));
                strcpy(streamData, _data.c_str());
                if (_data.length() > 0) streamData[_data.length()] = '\0';
                isFresh = true;
                return true;
            }
            else
            {
                return false;
            }
        }
};

#endif
