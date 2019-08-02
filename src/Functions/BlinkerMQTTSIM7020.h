#ifndef BLINKER_MQTT_SIM7020_H
#define BLINKER_MQTT_SIM7020_H

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

#define BLINKER_MQTT_SIM7020_DEFAULT_TIMEOUT 5000UL
#define BLINKER_MQTT_SIM7020_DATA_BUFFER_SIZE 1024

enum sim7020_mqtt_status_t
{
    sim7020_mqtt_init,
    sim7020_mqtt_init_success,
    sim7020_mqtt_connect,
    sim7020_mqtt_connect_success,
    sim7020_mqtt_set_sub,
    sim7020_mqtt_set_sub_success
};

class BlinkerMQTTSIM7020
{
    public :
        BlinkerMQTTSIM7020(Stream& s, bool isHardware, 
                    const char * server, uint16_t port, 
                    const char * cid, const char * user, 
                    const char * pass, blinker_callback_t func)
        {
            stream = &s; isHWS = isHardware;
            servername = server; portnum = port;
            clientid = cid; username = user;
            password = pass; listenFunc = func;
            // streamData = (char*)malloc(BLINKER_HTTP_SIM7020_DATA_BUFFER_SIZE*sizeof(char));
        }

        ~BlinkerMQTTSIM7020() { flush(); }

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
        uint32_t        ping_time;
        uint32_t        connect_time;
        uint16_t        _mqttTimeout = 5000;
        uint32_t        _debug_time;
        sim7020_mqtt_status_t    mqtt_status;

        char c_hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

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

        bool streamAvailable();

        void streamPrint(const String & s)
        {
            BLINKER_LOG_ALL(BLINKER_F("SIM MQTT: "), s);
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

int BlinkerMQTTSIM7020::connect()
{
    streamPrint(STRING_format(BLINKER_CMD_CMQNEW_REQ) + \
                "=\"" + servername + "\",\"" + STRING_format(portnum) + \
                "\",12000,1024");

    mqtt_status = sim7020_mqtt_init;
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            _masterAT = new BlinkerMasterAT();
            _masterAT->update(STRING_format(streamData));

            if (_masterAT->getState() != AT_M_NONE &&
                _masterAT->reqName() == BLINKER_CMD_CMQNEW &&
                _masterAT->getParam(0).toInt() == 0)
            {
                free(_masterAT);

                mqtt_status = sim7020_mqtt_init_success;
                break;
            }
            free(_masterAT);
        }
    }

    if (mqtt_status != sim7020_mqtt_init_success) return false;
    
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                break;
            }
        }
    }

    streamPrint(STRING_format(BLINKER_CMD_CMQCON_REQ) + 
                "=0,3,\"" + clientid + "\",600,0,0,\"" +
                username + "\",\"" + password + "\"");
    mqtt_status = sim7020_mqtt_connect;
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                mqtt_status = sim7020_mqtt_connect_success;
                // return true;
                break;
            }
        }
    }

    if (mqtt_status != sim7020_mqtt_connect_success) return false;

    // isConnected = true;

    streamPrint(STRING_format(BLINKER_CMD_CMQSUB_REQ) + 
                "=0,\"" + subTopic + "\",0");
    mqtt_status = sim7020_mqtt_set_sub;
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("mqtt set sub ok"));
                mqtt_status = sim7020_mqtt_set_sub_success;

                isConnected = true;

                ping_time = millis();

                return true;
            }
        }
    }

    if (mqtt_status != sim7020_mqtt_set_sub_success) return false;
}

int BlinkerMQTTSIM7020::connected()
{
    // yield();

    if (isConnected && (millis() - ping_time) <= 10000) 
    {
        yield();
        return isConnected;
    }

    if ((millis() - ping_time) <= 30000) return false;

    BLINKER_LOG_ALL(BLINKER_F(">>>>>> mqtt connected check <<<<<<"));
    streamPrint(STRING_format(BLINKER_CMD_CMQCON_REQ) + "?");
    mqtt_time = millis();
    ping_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            BLINKER_LOG_ALL(BLINKER_F("connected query"));

            _masterAT = new BlinkerMasterAT();
            _masterAT->update(STRING_format(streamData));

            if (_masterAT->getState() != AT_M_NONE &&
                _masterAT->reqName() == BLINKER_CMD_CMQCON &&
                _masterAT->getParam(0).toInt() == 0)
            {
                if (_masterAT->getParam(1) == "1") 
                {
                    isConnected = true;
                }
                else
                {
                    isConnected = false;
                }
                free(_masterAT);
                BLINKER_LOG_ALL(BLINKER_F("isConnected: "), isConnected);
                break;
            }

            free(_masterAT);
        }
    }

    while(streamAvailable()) { delay(10); }
    
    return isConnected;
}

int BlinkerMQTTSIM7020::disconnect()
{
    streamPrint(STRING_format(BLINKER_CMD_CMQDISCON_RESQ) + "=0");
    mqtt_time = millis();

    isConnected = false;

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                // isConnected = false;

                return true;
            }
        }
    }

    return false;
}

void BlinkerMQTTSIM7020::subscribe(const char * topic)
{
    subTopic = topic;
}

int BlinkerMQTTSIM7020::publish(const char * topic, const char * msg)
{
    streamPrint(STRING_format(BLINKER_CMD_CMQPUB_REQ) +
                "=0,\"" + topic + "\",0,0,0," + 
                STRING_format(strlen(msg)*2) + ",\"" +
                encode(msg) + "\"");
    mqtt_time = millis();

    while(millis() - mqtt_time < _mqttTimeout)
    {
        if (streamAvailable())
        {
            if (strcmp(streamData, BLINKER_CMD_OK) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

int BlinkerMQTTSIM7020::readSubscription(uint16_t time_out)
{
    if (isFreshSub)
    {
        isFreshSub = false;
        return true;
    }
    else
    {
        mqtt_time = millis();

        while(millis() - mqtt_time < time_out)
        {
            if (streamAvailable())
            {
                BLINKER_LOG_ALL(BLINKER_F("readSubscription"));
                BLINKER_LOG_FreeHeap_ALL();

                _masterAT = new BlinkerMasterAT();
                _masterAT->update(STRING_format(streamData));

                if (_masterAT->getState() != AT_M_NONE &&
                    _masterAT->reqName() == BLINKER_CMD_CMQPUB)
                {
                    String subData = String(streamData).substring(
                                        _masterAT->getParam(0).length() +
                                        _masterAT->getParam(1).length() + 
                                        _masterAT->getParam(2).length() + 
                                        _masterAT->getParam(3).length() + 
                                        _masterAT->getParam(4).length() + 
                                        _masterAT->getParam(5).length() + 
                                        15, strlen(streamData) - 1);
                    // BLINKER_LOG_ALL(BLINKER_F("leng 0: "), _masterAT->getParam(0).length());
                    // BLINKER_LOG_ALL(BLINKER_F("leng 1: "), _masterAT->getParam(1).length());
                    // BLINKER_LOG_ALL(BLINKER_F("leng 2: "), _masterAT->getParam(2).length());
                    // BLINKER_LOG_ALL(BLINKER_F("leng 3: "), _masterAT->getParam(3).length());
                    // BLINKER_LOG_ALL(BLINKER_F("leng 4: "), _masterAT->getParam(4).length());
                    // BLINKER_LOG_ALL(BLINKER_F("leng 5: "), _masterAT->getParam(5).length());
                    BLINKER_LOG_ALL(BLINKER_F("mqtt sub data: "), subData);

                    if (isRead) free(lastRead);
                    lastRead = (char*)malloc((subData.length()/2+1)*sizeof(char));
                    // strcpy(lastRead, subData.c_str());

                    memset(lastRead, '\0', subData.length()/2+1);

                    for(uint16_t num = 0; num < subData.length()/2; num++)
                    {
                        lastRead[num] = (char)(dcode_data(subData[num*2])<< 4 | dcode_data(subData[num*2+1]));
                    }

                    isFreshSub = false;
                    isConnected = true;
                    isRead = true;
                    connect_time = millis();

                    free(_masterAT);

                    return true;
                }
                else if (_masterAT->getState() != AT_M_NONE &&
                    _masterAT->reqName() == BLINKER_CMD_CMQDISCON)
                {
                    free(_masterAT);

                    isConnected = false;

                    return false;
                }
                

                free(_masterAT);
            }
        }

        return false;
    }
}

bool BlinkerMQTTSIM7020::streamAvailable()
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

    // char _data[BLINKER_MQTT_SIM7020_DATA_BUFFER_SIZE];// = { '\0' };
    // memset(_data, '\0', BLINKER_MQTT_SIM7020_DATA_BUFFER_SIZE);
    // if (!isFresh) streamData = (char*)malloc(BLINKER_MQTT_SIM7020_DATA_BUFFER_SIZE*sizeof(char));

    if (stream->available())
    {
        // strcpy(_data, stream->readStringUntil('\n').c_str());
        String _data = stream->readStringUntil('\n');
        BLINKER_LOG_ALL(BLINKER_F("handleSerial rs: "), _data);
        BLINKER_LOG_ALL("len: ", _data.length());
        // _data[strlen(_data) - 1] = '\0';

        // stream->flush();
        
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
