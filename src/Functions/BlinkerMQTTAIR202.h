#ifndef BLINKER_MQTT_AIR202_H
#define BLINKER_MQTT_AIR202_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "../Blinker/BlinkerATMaster.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"

// #if defined(ESP32)
//     #include <HardwareSerial.h>

//     HardwareSerial *HSerial_MQTT;
// #else
//     #include <SoftwareSerial.h>

//     SoftwareSerial *SSerial_MQTT;
// #endif

#define BLINKER_MQTT_AIR202_DEFAULT_TIMEOUT 5000UL
#define BLINKER_MQTT_AIR202_DATA_BUFFER_SIZE 1024

enum air202_mqtt_status_t
{
    mqtt_init,
    mqtt_init_success,
    mqtt_set,
    mqtt_set_ok,
    mqtt_set_connect_ok,
    mqtt_connect,
    mqtt_connect_ok,
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
        BlinkerMQTTAIR202(Stream& s, bool isHardware, 
                    const char * server, uint16_t port, 
                    const char * cid, const char * user, 
                    const char * pass, blinker_callback_t func)
        {
            stream = &s; isHWS = isHardware;
            servername = server; portnum = port;
            clientid = cid; username = user;
            password = pass; listenFunc = func;
        }

        ~BlinkerMQTTAIR202() { flush(); }
        
        int connect();
        int connected();
        int disconnect();
        void subscribe(const char * topic);
        int publish(const char * topic, const char * msg);
        int readSubscription(uint16_t time_out = 1000);

        char*   lastRead;
        const char* subTopic;
        // char    streamData[1024];
        char*       streamData;

        void flush()
        {
            if(isFresh) free(streamData);
            if(isRead) free(lastRead);

            isFresh = false;
            isRead = false;
        }

    protected :
        class BlinkerMasterAT * _masterAT;
        blinker_callback_t      listenFunc = NULL;
        Stream* stream;
        // char*   streamData;
        bool    isFresh = false;
        bool    isHWS = false;
        bool    isConnected = false;
        bool    isFreshSub = false;
        bool    isRead = false;
        const char *    servername;
        uint16_t        portnum;
        const char *    clientid;
        const char *    username;
        const char *    password;
        uint32_t        mqtt_time;
        uint32_t        connect_time;
        uint16_t        _mqttTimeout = 5000;
        uint32_t        _debug_time;
        air202_mqtt_status_t    mqtt_status;

        bool streamAvailable();

        void streamPrint(const String & s)
        {
            BLINKER_LOG_ALL(s);
            stream->println(s);
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

int BlinkerMQTTAIR202::connect()
{
    streamPrint(STRING_format(BLINKER_CMD_CSTT_REQ) +
                "=\"" + BLINKER_CMD_CMNET + "\"");
    // BLINKER_LOG_ALL(STRING_format(BLINKER_CMD_MCONFIG_REQ) +
    //             "=\"" + clientid + "\",\"" + username + 
    //             "\",\"" + password + "\"");

    mqtt_status = mqtt_init;
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt cstt success"));
                // mqtt_status = mqtt_init_success;
                break;
            }
        }
    }

    streamPrint(STRING_format(BLINKER_CMD_CIICR_REQ));
    // BLINKER_LOG_ALL(STRING_format(BLINKER_CMD_MCONFIG_REQ) +
    //             "=\"" + clientid + "\",\"" + username + 
    //             "\",\"" + password + "\"");

    mqtt_status = mqtt_init;
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt ciicr success"));
                // mqtt_status = mqtt_init_success;
                break;
            }
        }
    }

    streamPrint(STRING_format(BLINKER_CMD_MCONFIG_REQ) +
                "=\"" + clientid + "\",\"" + username + 
                "\",\"" + password + "\"");
    // BLINKER_LOG_ALL(STRING_format(BLINKER_CMD_MCONFIG_REQ) +
    //             "=\"" + clientid + "\",\"" + username + 
    //             "\",\"" + password + "\"");

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

    streamPrint(STRING_format(BLINKER_CMD_SSLMIPSTART) + 
                "=\"" + servername + "\"," + STRING_format(portnum));
    // BLINKER_LOG_ALL(STRING_format(BLINKER_CMD_SSLMIPSTART) + 
    //             "=\"" + servername + "\"," + STRING_format(portnum));
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

    streamPrint(STRING_format(BLINKER_CMD_MCONNECT_REQ) +
                "=1,300");
    mqtt_status = mqtt_connect;
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt set connect, get ok, wait connact"));
                mqtt_status = mqtt_connect_ok;
                break;
            }
        }
    }

    if (mqtt_status != mqtt_connect_ok) return false;
    mqtt_time = millis();

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

    isConnected = true;

    // return true;
    streamPrint(STRING_format(BLINKER_CMD_MSUB_REQ) +
                "=\"" + subTopic + "\",0");
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
                mqtt_status = mqtt_set_sub_success;
                break;
            }
        }
    }

    if (mqtt_status != mqtt_set_sub_success) return false;

    return true;
}

int BlinkerMQTTAIR202::connected()
{
    // if (isConnected && millis() - connect_time <= 30000)
    // {
    //     while(millis() - mqtt_time < _mqttTimeout)
    //     {
    //         if (millis() - _debug_time >= 2000)
    //         {
    //             _debug_time = millis();
    //             BLINKER_LOG_ALL(BLINKER_F("== connected, readSubscription =="));
    //         }

    //         if (streamAvailable())
    //         {
    //             BLINKER_LOG_ALL(BLINKER_F("== connected available readSubscription =="));

    //             _masterAT = new BlinkerMasterAT();
    //             _masterAT->update(STRING_format(streamData));

    //             if (_masterAT->getState() != AT_M_NONE &&
    //                 _masterAT->reqName() == BLINKER_CMD_MQTTSTATUS)
    //             {
    //                 if(_masterAT->getParam(0).toInt() == 1)
    //                 {
    //                     free(_masterAT);
    //                     connect_time = millis();
    //                     isConnected = true;
    //                     return true;
    //                 }
    //                 else
    //                 {
    //                     BLINKER_LOG_ALL("mqtt not connected!");
    //                     isConnected = false;
    //                     free(_masterAT);
    //                     return false;
    //                 }
                    
    //             }
    //             else if (_masterAT->getState() != AT_M_NONE &&
    //                 _masterAT->reqName() == BLINKER_CMD_MSUB)
    //             {
    //                 String subData = String(streamData).substring(
    //                                 _masterAT->getParam(0).length() +
    //                                 _masterAT->getParam(1).length() + 
    //                                 10);

    //                 BLINKER_LOG_ALL("sub data: ", subData);

    //                 if (isFreshSub) free(lastRead);
    //                 lastRead = (char*)malloc((subData.length()+1)*sizeof(char));
    //                 strcpy(lastRead, subData.c_str());

    //                 isFreshSub = true;

    //                 connect_time = millis();

    //                 free(_masterAT);
    //                 return true;
    //             }

    //             free(_masterAT);

    //             BLINKER_LOG_ALL("== free connected at master ==");
    //         }
    //     }
        
    //     return true;
    // }
    // else
    {
        if (isFreshSub)
        {
            connect_time = millis();
            return true;
        }

        if (!isConnected || millis() - connect_time >= 15000)
        {
            streamPrint(STRING_format(BLINKER_CMD_MQTTSTATU_REQ));
            // connect_time = millis();
        // }
            mqtt_time = millis();
            uint8_t status_get = 0;        

            while(millis() - mqtt_time < _mqttTimeout)
            {
                if (streamAvailable())
                {
                    BLINKER_LOG_ALL(BLINKER_F("== connected available =="));

                    _masterAT = new BlinkerMasterAT();
                    _masterAT->update(STRING_format(streamData));

                    if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_MQTTSTATUS)
                    {
                        if(_masterAT->getParam(0).toInt() == 1)
                        {
                            free(_masterAT);
                            connect_time = millis();
                            isConnected = true;
                            return true;
                        }
                        else
                        {
                            BLINKER_LOG_ALL("mqtt not connected!");
                            isConnected = false;
                            free(_masterAT);
                            return false;
                        }
                        
                    }
                    else if (_masterAT->getState() != AT_M_NONE &&
                        _masterAT->reqName() == BLINKER_CMD_MSUB)
                    {
                        String subData = String(streamData).substring(
                                        _masterAT->getParam(0).length() +
                                        _masterAT->getParam(1).length() + 
                                        10);

                        BLINKER_LOG_ALL("sub data: ", subData);

                        if (isRead) free(lastRead);
                        lastRead = (char*)malloc((subData.length()+1)*sizeof(char));
                        strcpy(lastRead, subData.c_str());

                        isFreshSub = true;
                        isConnected = true;
                        isRead = true;
                        connect_time = millis();

                        free(_masterAT);

                        BLINKER_LOG_ALL("== free at master, return ==");
                        return true;
                    }

                    free(_masterAT);

                    BLINKER_LOG_ALL("== free connected at master ==");
                }
            }
        }
    }

    return isConnected;

    // return status_get;
}

int BlinkerMQTTAIR202::disconnect()
{
    streamPrint(STRING_format(BLINKER_CMD_MDISCONNECT_REQ));
    mqtt_time = millis();
    uint8_t status_get = 0;

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            BLINKER_LOG_ALL(BLINKER_F("== disconnect available =="));
            if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt disconnect"));
                // return true;
                break;
            }
        }
    }

    streamPrint(STRING_format(BLINKER_CMD_MIPCLOSE_REQ));
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            BLINKER_LOG_ALL(BLINKER_F("== disconnect available =="));
            if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt disconnect"));
                // return true;
                break;
            }
        }
    }

    streamPrint(STRING_format(BLINKER_CMD_CIPSHUT_REQ));
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            BLINKER_LOG_ALL(BLINKER_F("== disconnect available =="));
            if (strcmp(streamData, BLINKER_CMD_CONNACK_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt disconnect"));
                return true;
                // break;
            }
        }
    }

    return false;
}

void BlinkerMQTTAIR202::subscribe(const char * topic)
{
    subTopic = topic;
    // streamPrint(STRING_format(BLINKER_CMD_MPUB_REQ) +
    //             "=\"" + topic + "\",0");
    // mqtt_status = mqtt_set_sub_topic;
    // mqtt_time = millis();

    // while(millis() - mqtt_time < _mqttTimeout)
    // {
    //     if (streamAvailable())
    //     {
    //         if (strcmp(streamData, BLINKER_CMD_OK) == 0)
    //         {
    //             BLINKER_LOG_ALL(BLINKER_F("mqtt set sub ok"));
    //             mqtt_status = mqtt_set_sub_ok;
    //             break;
    //         }
    //     }
    // }

    // if (mqtt_status != mqtt_set_sub_ok) return false;
    // mqtt_time = millis();

    // while(millis() - mqtt_time < _mqttTimeout)
    // {
    //     if (streamAvailable())
    //     {
    //         if (strcmp(streamData, BLINKER_CMD_SUBACK) == 0)
    //         {
    //             BLINKER_LOG_ALL(BLINKER_F("mqtt set sub success"));
    //             mqtt_status = mqtt_set_sub_success;
    //             break;
    //         }
    //     }
    // }

    // if (mqtt_status != mqtt_set_sub_success) return false;

    // return true;
}

int BlinkerMQTTAIR202::publish(const char * topic, const char * msg)
{
    streamPrint(STRING_format(BLINKER_CMD_MPUB_REQ) +
                "=\"" + topic + "\",0,0,\"" + msg + "\"");
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

    // mqtt_time = millis();

    // while(millis() - mqtt_time < _mqttTimeout)
    // {
    //     if (streamAvailable())
    //     {
    //         if (strcmp(streamData, BLINKER_CMD_PUBACK) == 0)
    //         {
    //             BLINKER_LOG_ALL(BLINKER_F("mqtt set pub success"));
    //             mqtt_status = mqtt_set_pub_success;
    //             break;
    //         }
    //     }
    // }

    // if (mqtt_status != mqtt_set_pub_success) return false;

    return true;
}

int BlinkerMQTTAIR202::readSubscription(uint16_t time_out)
{
    // if (millis() - _debug_time >= 2000)
    // {
    //     _debug_time = millis();
    //     BLINKER_LOG_ALL(BLINKER_F("== readSubscription =="));
    // }

    if (isFreshSub)
    {
        isFreshSub = false;
        return true;
    }
    else
    {
        // return false;
    // }
        mqtt_time = millis();
    
        while(millis() - mqtt_time < time_out)
        {
            if (streamAvailable())
            {
                BLINKER_LOG_ALL(BLINKER_F("readSubscription"));

                _masterAT = new BlinkerMasterAT();
                _masterAT->update(STRING_format(streamData));

                if (_masterAT->getState() != AT_M_NONE &&
                    _masterAT->reqName() == BLINKER_CMD_MSUB)
                {
                    String subData = String(streamData).substring(
                                        _masterAT->getParam(0).length() +
                                        _masterAT->getParam(1).length() + 
                                        8);
                    BLINKER_LOG_ALL(BLINKER_F("mqtt sub data: "), subData);

                    if (isRead) free(lastRead);
                    lastRead = (char*)malloc((subData.length()+1)*sizeof(char));
                    strcpy(lastRead, subData.c_str());

                    isFreshSub = false;
                    isConnected = true;
                    isRead = true;
                    connect_time = millis();

                    free(_masterAT);

                    return true;
                }

                free(_masterAT);
            }
        }

        return false;
    }
}

bool BlinkerMQTTAIR202::streamAvailable()
{
    yield();

    if (!isHWS)
    {
        // #if defined(__AVR__) || defined(ESP8266)
        //     if (!SSerial_MQTT->isListening())
        //     {
        //         SSerial_MQTT->listen();
        //         ::delay(100);
        //     }
        // #endif

        if (listenFunc) listenFunc();
    }

    // char _data[BLINKER_MQTT_AIR202_DATA_BUFFER_SIZE];// = { '\0' };
    // memset(_data, '\0', BLINKER_MQTT_AIR202_DATA_BUFFER_SIZE);

    // if (!isFresh) streamData = (char*)malloc(BLINKER_MQTT_AIR202_DATA_BUFFER_SIZE*sizeof(char));

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
        // if (isFresh) free(streamData);
        // streamData = (char*)malloc(1*sizeof(char));
        // // streamData = "";
        // // memset(streamData, '\0', 1024);

        // // strcpy(streamData, stream->readStringUntil('\n').c_str());

        // // BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), streamData);

        // // int16_t dNum = strlen(streamData);
        
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
        // // streamData = (char*)realloc(streamData, dNum*sizeof(char));

        // // streamData[dNum-1] = '\0';

        // // streamData = stream->readStringUntil('\n');
        // // streamData[streamData.length()-1] = '\0';

        // // BLINKER_LOG_ALL(BLINKER_F("handleSerial TEST: "), stream->readStringUntil('\n'));

        // // stream->flush();
        
        // // BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
        // //                 BLINKER_F(" , dNum: "), dNum);
        // // BLINKER_LOG_FreeHeap_ALL();
        
        // if (dNum < BLINKER_MAX_READ_SIZE && dNum > 0)
        // {
        //     // if (streamData[strlen(streamData) - 1] == '\r')
        //     streamData[dNum - 1] = '\0';
        //     BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData,
        //                     BLINKER_F(" , dNum: "), dNum);

        //     isFresh = true;
        //     return true;
        // }
        // else
        // {
        //     // free(streamData);
        //     return false;
        // }
    }
    else
    {
        return false;
    }
}

#endif
