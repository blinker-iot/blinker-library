#ifndef BLINKER_MQTT_AIR202_H
#define BLINKER_MQTT_AIR202_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Blinker/BlinkerATMaster.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

#define BLINKER_MQTT_AIR202_DEFAULT_TIMEOUT 5000UL

enum air202_mqtt_status_t
{
    mqtt_init,
    mqtt_init_success,
    mqtt_set,
    mqtt_set_ok,
    mqtt_set_connect_ok,
    mqtt_coneect,
    mqtt_coneect_ok,
    mqtt_connect_success,
    mqtt_set_sub_topic,
    mqtt_set_sub_ok,
    mqtt_set_sub_success,
    mqtt_set_pub,
    mqtt_set_pub_ok,
    mqtt_set_pub_success
};

class BlinkerMQTTAIR202
{
    public :
        BlinkerHTTPAIR202(Stream & s, bool isHardware = false,
                        const char * server, uint16_t port,
                        const char * cid, const char * user, const char * pass)
        {
            stream = s; isHWS = isHardware;
            servername = server; portnum = port;
            clientid = cid; username = user;
            password = pass;
        }

        int connect()
        {
            stream->print(STRING_format(BLINKER_CMD_MCONFIG_RESQ) +
                        "=\"" + clientid + "\",\"" + username + 
                        "\",\"" + password + "\"")
            mqtt_status = mqtt_init;
            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt init success"));
                        mqtt_status = mqtt_init_success;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_init_success) return false;

            stream->print(STRING_format(BLINKER_CMD_SSLMIPSTART) + 
                        "=\"" + servername + "\"," + STRING_format(portnum));
            mqtt_status = mqtt_set;
            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set ok"));
                        mqtt_status = mqtt_set_ok;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_set_ok) return false;
            
            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_CONNECT_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set connect ok, can connect now"));
                        mqtt_status = mqtt_set_connect_ok;
                        break;
                    }
                }
            }

            stream->print(STRING_format(BLINKER_CMD_MCONNECT_RESQ) +
                        "=1,300");
            mqtt_status = mqtt_connect;
            mqtt_time = milis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set connect, get ok, wait connact"));
                        mqtt_status = mqtt_coneect_ok;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_coneect_ok) return false;
            mqtt_time = milis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt connacted"));
                        mqtt_status = mqtt_connect_success;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_connect_success) return false;

            return true;
        }

        int connected()
        {
            stream->println(STRING_format(BLINKER_CMD_MQTTSTATU_RESQ))
            mqtt_time = milis();
            uint8_t status_get = 0;

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (streamData == "1")
                    {
                        status_get = 1;
                    }
                    else
                    {
                        status_get = 0;
                    }
                }
            }

            return status_get;
        }

        int subscribe(const char * topic)
        {
            stream->println(STRING_format(BLINKER_CMD_MPUB_RESQ) +
                            "=\"" + topic + "\",0");
            mqtt_status = mqtt_set_sub_topic;
            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set sub ok"));
                        mqtt_status = mqtt_set_sub_ok;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_set_sub_ok) return false;
            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_SUBACK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set sub success"));
                        mqtt_status = BLINKER_CMD_SUBACK;
                        break;
                    }
                }
            }

            if (mqtt_status != BLINKER_CMD_SUBACK) return false;

            return true;
        }

        int publish(const char * topic, const char * msg)
        {
            stream->println(STRING_format(BLINKER_CMD_MPUB_RESQ) +
                            "=\"" + topic "\",0,0,\"" + msg + "\"");
            mqtt_status = mqtt_set_pub;
            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_OK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set pub ok"));
                        mqtt_status = mqtt_set_pub_ok;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_set_pub_ok) return false;

            mqtt_time = millis();

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    if (strcmp(streamData, BLINKER_CMD_PUBACK) == 0)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt set pub success"));
                        mqtt_status = mqtt_set_pub_success;
                        break;
                    }
                }
            }

            if (mqtt_status != mqtt_set_pub_success) return false;

            return true;
        }

        int available(uint16_t time_out == 100)
        {
            while(millis() - mqtt_time < time_out)
            {
                if (streamAvailable())
                {
                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_MSUB)
                    {
                        BLINKER_LOG_ALL(BLINKER_F("mqtt sub data, data: "), _masterAT->getParam(2));

                        mqttData = (char*)realloc(mqttData, (_masterAT->getParam(2).toInt()+1)*sizeof(char));
                        strcpy(mqttData, _masterAT->getParam(2).c_str());

                        free(_masterAT);

                        return true;
                    }

                    free(_masterAT);

                    return false;
                }
            }

            return false;
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        Stream* stream;
        char*   streamData;
        char*   mqttData;
        bool    isFresh = false;
        bool    isHWS = false;
        const char *    servername;
        uint16_t        portnum;
        const char *    clientid;
        const char *    username;
        const char *    password;
        uint32_t        mqtt_time;
        uint16_t        _mqttTimeout;
        air202_mqtt_status_t    mqtt_status;

        bool streamAvailable()
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

#endif