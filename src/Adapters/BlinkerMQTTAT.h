#ifndef BLINKER_MQTT_AT_H
#define BLINKER_MQTT_AT_H

#if (defined(ESP8266) || defined(ESP32))

#define BLINKER_AT_MQTT
#define BLINKER_ESP_AT

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266mDNS.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #include <HTTPClient.h>
#endif

#include <EEPROM.h>

#include "../modules/WebSockets/WebSocketsServer.h"
#include "../modules/mqtt/Adafruit_MQTT.h"
#include "../modules/mqtt/Adafruit_MQTT_Client.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "Adapters/BlinkerMQTTAT.h"
#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"
#include "../Blinker/BlinkerMQTTATBase.h"

char*       MQTT_HOST_MQTT_AT;
char*       MQTT_ID_MQTT_AT;
char*       MQTT_NAME_MQTT_AT;
char*       MQTT_KEY_MQTT_AT;
char*       MQTT_PRODUCTINFO_MQTT_AT;
char*       UUID_MQTT_AT;
char*       DEVICE_NAME_MQTT_AT;
char*       BLINKER_PUB_TOPIC_MQTT_AT;
char*       BLINKER_SUB_TOPIC_MQTT_AT;
uint16_t    MQTT_PORT_MQTT_AT;

class BlinkerMQTTAT : public BlinkerStream
{
    public :
        BlinkerMQTTAT();

        int serialAvailable();
        void serialBegin(Stream& s, bool state);
        int serialTimedRead();
        char * serialLastRead();
        // void serialFlush();
        int serialPrint(const String & s1, const String & s2, bool needCheck = true);
        int serialPrint(const String & s, bool needCheck = true);
        int serialConnect();
        int serialConnected();
        void serialDisconnect();

        int connect();
        int connected();
        int mConnected();
        void disconnect();
        void ping();
        int available();
        int aligenieAvail();
        int duerAvail();
        int miAvail();
        // bool extraAvailable();
        void subscribe();
        char * lastRead();
        void flush();
        int print(char * data, bool needCheck = true);
        int mqttPrint(const String & data);
        int bPrint(char * name, const String & data);
        int aliPrint(const String & data);
        int  duerPrint(const String & data, bool report = false);
        int miPrint(const String & data);
        // void aliType(const String & type);
        void begin(const char* auth);
        bool begin();
        int autoPrint(unsigned long id);
        // bool autoPrint(char *name, char *type, char *data);
        // bool autoPrint(char *name1, char *type1, char *data1, 
        //             char *name2, char *type2, char *data2);
        int toServer(char * data) { return true; }
        char * deviceName();
        char * authKey() { return _authKey; }
        char * token() { if (!isMQTTinit) return ""; else return MQTT_KEY_MQTT_AT; }
        char * deviceId();// { return MQTT_ID; }
        char * uuid();// { return UUID; }
        void softAPinit();
        int parseUrl(String data);
        int autoInit();
        void smartconfig();
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);
        int init() { return isMQTTinit; }
        int reRegister() { return connectServer(); }
        void aligenieType(int _type) { _aliType = _type; }
        void duerType(int _type) { _duerType = _type; }
        void freshAlive() { kaTime = millis(); isAlive = true; }
        void sharers(const String & data);
        int  needFreshShare() {
            if (_needCheckShare)
            {
                BLINKER_LOG_ALL(BLINKER_F("needFreshShare"));
                _needCheckShare = false;
                return true;
            }
            else
            {
                return false;
            }
        }

    private :
        bool isMQTTinit = false;

        int connectServer();
        void mDNSInit();
        void checkKA();
        int checkAliKA();
        int checkDuerKA();
        int checkMIOTKA();
        int checkCanPrint();
        int checkCanBprint();
        int checkPrintSpan();
        int checkAliPrintSpan();
        int checkDuerPrintSpan();
        int checkMIOTPrintSpan();
        int checkPrintLimit();

    protected :
        bool        _isBegin = false;

        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
        uint8_t     _sharerCount = 0;
        uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        int _aliType = ALI_NONE;
        int _duerType = DUER_NONE;
        // uint8_t     pinDataNum = 0;

        // io 
        // - - - -  - - - - | - - - -  - - - -
        // |                  |        |pull_up
        // |                  |mode
        // |pin
        // pin         40 0-39
        // mode        2 input/output
        // pull_up     3 none/pull_up/pull_down
        
        // const char* _authKey;
        char*       _authKey;
        // char*       _aliType;
        // char        _authKey[BLINKER_AUTHKEY_SIZE];
        bool*       isHandle;// = &isConnect;
        bool        isAlive = false;
        // bool        isBavail = false;
        bool        _needCheckShare = false;
        uint32_t    latestTime;
        uint32_t    printTime = 0;
        uint32_t    bPrintTime = 0;
        uint32_t    kaTime = 0;
        uint32_t    linkTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;
        uint8_t     respDuerTimes = 0;
        uint32_t    respDuerTime = 0;
        uint8_t     respMIOTTimes = 0;
        uint32_t    respMIOTTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        uint32_t    duerKaTime = 0;
        bool        isDuerAlive = false;
        bool        isDuerAvail = false;
        uint32_t    miKaTime = 0;
        bool        isMIOTAlive = false;
        bool        isMIOTAvail = false;
        char*       mqtt_broker;

        Stream*     stream;
        char*       streamData;//[BLINKER_MAX_READ_SIZE];
        bool        isSeriaFresh;
        bool        isSerialConnect;
        bool        isHWS = false;

        int isJson(const String & data);

        uint8_t     reconnect_time = 0;

        uint32_t    _print_time = 0;
        uint8_t     _print_times = 0;
};

// #if defined(ESP8266)
//     extern BearSSL::WiFiClientSecure   client_mqtt;
//     // WiFiClientSecure            client_mqtt;
// #elif defined(ESP32)
//     extern WiFiClientSecure            client_s;
// #endif

// extern WiFiClient              client;

// #endif

#if defined(ESP8266)
    #include <SoftwareSerial.h>

    BearSSL::WiFiClientSecure   client_mqtt;
    // WiFiClientSecure            client_mqtt;
#elif defined(ESP32)
    #include "HardwareSerial.h"

    WiFiClientSecure            client_s;
#endif

WiFiClient               client;
Adafruit_MQTT_Client*    mqtt_MQTT_AT;
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe* iotSub_MQTT_AT;

#define WS_SERVERPORT       81
WebSocketsServer webSocket_MQTT_AT = WebSocketsServer(WS_SERVERPORT);

char*    msgBuf_MQTT_AT;
bool     isFresh_MQTT_AT = false;
bool     isConnect_MQTT_AT = false;
bool     isAvail_MQTT_AT = false;
uint8_t  ws_num_MQTT_AT = 0;
uint8_t  dataFrom_MQTT_AT = BLINKER_MSG_FROM_MQTT;
// bool     isATAvaill = false;

void webSocketEvent_MQTT_AT(uint8_t num, WStype_t type, \
                    uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

            isConnect_MQTT_AT = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket_MQTT_AT.remoteIP(num);
                
                BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                                BLINKER_F(", Connected from: "), ip, 
                                BLINKER_F(", url: "), (char *)payload);
                
                // send message to client
                webSocket_MQTT_AT.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num_MQTT_AT = num;

                isConnect_MQTT_AT = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                            BLINKER_F(", get Text: "), (char *)payload, \
                            BLINKER_F(", length: "), length);
            
            if (length < BLINKER_MAX_READ_SIZE) {
                if (isFresh_MQTT_AT) free(msgBuf_MQTT_AT);
                msgBuf_MQTT_AT = (char*)malloc((length+1)*sizeof(char));
                strcpy(msgBuf_MQTT_AT, (char*)payload);
                isAvail_MQTT_AT = true;
                isFresh_MQTT_AT = true;
            }

            dataFrom_MQTT_AT = BLINKER_MSG_FROM_WS;

            ws_num_MQTT_AT = num;

            // send message to client
            // webSocket_MQTT_AT.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket_MQTT_AT.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket_MQTT_AT.sendBIN(num, payload, length);
            break;
        default :
            break;
    }
}

BlinkerMQTTAT::BlinkerMQTTAT()
{
    stream = NULL;
    isSerialConnect = false;
    isHandle = &isConnect_MQTT_AT;
}

int BlinkerMQTTAT::serialAvailable()
{
    if (stream->available())
    {
        // strcpy(streamData, (stream->readStringUntil('\n')).c_str());

        // BLINKER_LOG_ALL(BLINKER_F("handleSerial: "), streamData);

        // return true;

        if (isSeriaFresh) free(streamData);
        streamData = (char*)malloc(1*sizeof(char));
        
        int16_t dNum = 0;
        int c_d = serialTimedRead();
        while (dNum < BLINKER_MAX_READ_SIZE && 
            c_d >=0 && c_d != '\n')
        {
            if (c_d != '\r')
            {
                streamData[dNum] = (char)c_d;
                dNum++;
                streamData = (char*)realloc(streamData, (dNum+1)*sizeof(char));
            }

            c_d = serialTimedRead();
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

            isSeriaFresh = true;
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

void BlinkerMQTTAT::serialBegin(Stream& s, bool state)
{
    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;

    serialConnect();

    serialPrint("");
    serialPrint(BLINKER_CMD_BLINKER_MQTT);
}

int BlinkerMQTTAT::serialTimedRead()
{
    int c;
    uint32_t _startMillis = millis();
    do {
        c = stream->read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < 1000);
    return -1; 
}

char * BlinkerMQTTAT::serialLastRead()
{
    return streamData;
}

// void BlinkerMQTTAT::serialFlush()
// {
//     if (isSeriaFresh)
//     {
//         free(streamData); isSeriaFresh = false;
//     }
// }

int BlinkerMQTTAT::serialPrint(const String & s1, const String & s2, bool needCheck)
{
    return serialPrint(s1 + s2, needCheck);
}

int BlinkerMQTTAT::serialPrint(const String & s, bool needCheck)
{
    BLINKER_LOG_ALL(BLINKER_F("Serial Response: "), s);
    
    if(serialConnected())
    {
        BLINKER_LOG_ALL(BLINKER_F("Success..."));
        
        stream->println(s);
        return true;
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
        
        return false;
    }
}

int BlinkerMQTTAT::serialConnect()
{
    isSerialConnect = true;
    return serialConnected();
}

int BlinkerMQTTAT::serialConnected() { return isSerialConnect; }

void BlinkerMQTTAT::serialDisconnect() { isSerialConnect = false; }

int BlinkerMQTTAT::connect()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    int8_t ret;

    webSocket_MQTT_AT.loop();

    if (mqtt_MQTT_AT->connected())
    {
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < 5000)
    {
        return false;
    }

    BLINKER_LOG(BLINKER_F("Connecting to MQTT... "));

    #if defined(ESP8266)
        client_mqtt.setInsecure();
    #endif

    if ((ret = mqtt_MQTT_AT->connect()) != 0)
    {
        BLINKER_LOG(mqtt_MQTT_AT->connectErrorString(ret));
        BLINKER_LOG(BLINKER_F("Retrying MQTT connection in 5 seconds..."));

        this->latestTime = millis();
        reconnect_time += 1;
        if (reconnect_time >= 12)
        {
            reRegister();
            reconnect_time = 0;
        }
        return false;
    }
    
    reconnect_time = 0;
    
    BLINKER_LOG(BLINKER_F("MQTT Connected!"));
    BLINKER_LOG_FreeHeap();

    this->latestTime = millis();

    return true;
}

int BlinkerMQTTAT::connected()
{ 
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    if (!isMQTTinit)
    {
        return *isHandle;
    }

    return mqtt_MQTT_AT->connected() || *isHandle; 
}

int BlinkerMQTTAT::mConnected()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    if (!isMQTTinit) return false;
    else return mqtt_MQTT_AT->connected();
}

void BlinkerMQTTAT::disconnect()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return;
    }

    mqtt_MQTT_AT->disconnect();

    if (*isHandle) webSocket_MQTT_AT.disconnect();
}

void BlinkerMQTTAT::ping()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return;
    }

    BLINKER_LOG_ALL(BLINKER_F("MQTT Ping!"));
    BLINKER_LOG_FreeHeap_ALL();

    if (!mqtt_MQTT_AT->ping())
    {
        disconnect();
        delay(100);

        connect();
    }
    else
    {
        this->latestTime = millis();
    }
}

int BlinkerMQTTAT::available()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    webSocket_MQTT_AT.loop();

    checkKA();

    // if (!mqtt_MQTT_AT->connected() || \
    //     (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
    if ((millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT)
    {
        ping();
    }
    else
    {
        subscribe();
    }

    if (isAvail_MQTT_AT)
    {
        isAvail_MQTT_AT = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerMQTTAT::aligenieAvail()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    if (isAliAvail)
    {
        isAliAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerMQTTAT::duerAvail()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    if (isDuerAvail)
    {
        isDuerAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerMQTTAT::miAvail()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return false;
    }

    if (isMIOTAvail)
    {
        isMIOTAvail = false;
        return true;
    }
    else {
        return false;
    }
}

// int BlinkerMQTTAT::extraAvailable()
// {
//     if (isBavail)
//     {
//         isBavail = false;        
//         return true;
//     }
//     else
//     {
//         return false;
//     }
// }

void BlinkerMQTTAT::subscribe()
{
    if (!_isBegin)
    {
        _isBegin = begin();

        if (!_isBegin) return;
    }

    if (!isMQTTinit) return;

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt_MQTT_AT->readSubscription(10)))
    {
        if (subscription == iotSub_MQTT_AT)
        {
            BLINKER_LOG_ALL(BLINKER_F("Got: "), (char *)iotSub_MQTT_AT->lastread);
            
            // DynamicJsonBuffer jsonBuffer;
            // JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub_MQTT_AT->lastread));
            DynamicJsonDocument jsonBuffer(1024);
            DeserializationError error = deserializeJson(jsonBuffer, (char *)iotSub_MQTT_AT->lastread);
            JsonObject root = jsonBuffer.as<JsonObject>();

            String _uuid = root["fromDevice"];
            String dataGet = root["data"];
            
            BLINKER_LOG_ALL(BLINKER_F("data: "), dataGet);
            BLINKER_LOG_ALL(BLINKER_F("fromDevice: "), _uuid);
            
            if (strcmp(_uuid.c_str(), UUID_MQTT_AT) == 0)
            {
                BLINKER_LOG_ALL(BLINKER_F("Authority uuid"));
                
                kaTime = millis();
                isAvail_MQTT_AT = true;
                isAlive = true;

                dataGet = dataGet.substring(0, dataGet.length() - 1) + \
                        ",\"deviceType\":\"OwnApp\"}";

                if (!isFresh_MQTT_AT && dataGet.length() < BLINKER_MAX_READ_SIZE)
                {
                    msgBuf_MQTT_AT = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                    isFresh_MQTT_AT = true;
                }
                else if (dataGet.length() < BLINKER_MAX_READ_SIZE)
                {
                    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                    isFresh_MQTT_AT = true;
                }
            }
            else if (_uuid == BLINKER_CMD_ALIGENIE)
            {
                BLINKER_LOG_ALL(BLINKER_F("form AliGenie"));
                
                aliKaTime = millis();
                isAliAlive = true;
                isAliAvail = true;

                // isAvail_MQTT_AT = true;
                // isAlive = true;

                dataGet = dataGet.substring(0, dataGet.length() - 1) + \
                        ",\"deviceType\":\"AliGenie\"}";

                _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

                if (!isFresh_MQTT_AT && dataGet.length() < BLINKER_MAX_READ_SIZE)
                {
                    msgBuf_MQTT_AT = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                    isFresh_MQTT_AT = true;
                }
                else if (dataGet.length() < BLINKER_MAX_READ_SIZE)
                {
                    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                    isFresh_MQTT_AT = true;
                }
            }
            else if (_uuid == BLINKER_CMD_DUEROS)
            {
                BLINKER_LOG_ALL(BLINKER_F("form DuerOS"));
                
                duerKaTime = millis();
                isDuerAlive = true;
                isDuerAvail = true;

                // isAvail_MQTT_AT = true;
                // isAlive = true;

                dataGet = dataGet.substring(0, dataGet.length() - 1) + \
                        ",\"deviceType\":\"DuerOS\"}";

                if (!isFresh_MQTT_AT && dataGet.length() < BLINKER_MAX_READ_SIZE)
                {
                    msgBuf_MQTT_AT = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                    isFresh_MQTT_AT = true;
                }
                else if (dataGet.length() < BLINKER_MAX_READ_SIZE)
                {
                    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                    isFresh_MQTT_AT = true;
                }
            }
            else
            {
                if (_sharerCount)
                {
                    for (uint8_t num = 0; num < _sharerCount; num++)
                    {
                        if (strcmp(_uuid.c_str(), _sharers[num]->uuid()) == 0)
                        {
                            _sharerFrom = num;

                            kaTime = millis();

                            isAvail_MQTT_AT = true;
                            isAlive = true;

                            dataGet = dataGet.substring(0, dataGet.length() - 1) + \
                                        ",\"deviceType\":\"OwnApp\"}";

                            if (!isFresh_MQTT_AT && dataGet.length() < BLINKER_MAX_READ_SIZE)
                            {
                                msgBuf_MQTT_AT = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                                strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                                isFresh_MQTT_AT = true;
                            }
                            else if (dataGet.length() < BLINKER_MAX_READ_SIZE)
                            {
                                strcpy(msgBuf_MQTT_AT, dataGet.c_str());
                                isFresh_MQTT_AT = true;
                            }

                            BLINKER_LOG_ALL(BLINKER_F("From sharer: "), _uuid);
                            BLINKER_LOG_ALL(BLINKER_F("sharer num: "), num);

                            _needCheckShare = false;

                            break;
                        }
                        else
                        {
                            BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
                                        check is from bridge/share device, \
                                        data: "), dataGet);

                            _needCheckShare = true;
                        } 
                    }
                }
                // else
                // {
                    // dataGet = String((char *)iotSub_MQTT_AT->lastread);
                //     root.printTo(dataGet);
                    
                //     BLINKER_ERR_LOG_ALL(BLINKER_F("No authority uuid found, \
                //                         check is from bridge/share device, \
                //                         data: "), dataGet);

                //     _needCheckShare = true;
                // }
                
                // return;

                // isBavail = true;
            }

            // memset(msgBuf_MQTT_AT, 0, BLINKER_MAX_READ_SIZE);
            // memcpy(msgBuf_MQTT_AT, dataGet.c_str(), dataGet.length());

            // if (isFresh_MQTT_AT) free(msgBuf_MQTT_AT);
            // msgBuf_MQTT_AT = (char*)malloc((dataGet.length()+1)*sizeof(char));
            // strcpy(msgBuf_MQTT_AT, dataGet.c_str());
            // isFresh_MQTT_AT = true;
            
            this->latestTime = millis();

            dataFrom_MQTT_AT = BLINKER_MSG_FROM_MQTT;
        }
    }
}

char * BlinkerMQTTAT::lastRead()
{
    if (isFresh_MQTT_AT) return msgBuf_MQTT_AT;
    return "";
}

void BlinkerMQTTAT::flush()
{
    if (isFresh_MQTT_AT)
    {
        free(msgBuf_MQTT_AT); isFresh_MQTT_AT = false; isAvail_MQTT_AT = false;
        isAliAvail = false; isDuerAvail = false; //isBavail = false;
    }
}

int BlinkerMQTTAT::print(char * data, bool needCheck)
{
    // BLINKER_LOG_FreeHeap();
    if (*isHandle && dataFrom_MQTT_AT == BLINKER_MSG_FROM_WS)
    {
        if (needCheck)
        {
            if (!checkPrintSpan())
            {
                respTime = millis();
                return false;
            }
        }

        respTime = millis();
        
        BLINKER_LOG_ALL(BLINKER_F("WS response: "));
        BLINKER_LOG_ALL(data);
        BLINKER_LOG_ALL(BLINKER_F("Success..."));

        strcat(data, BLINKER_CMD_NEWLINE);
        
        webSocket_MQTT_AT.sendTXT(ws_num_MQTT_AT, data);

        return true;
    }
    else
    {
        // // String payload;
        // if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
        // {
        //     uint8_t num = strlen(data) - 1;
        //     for(uint8_t c_num = num; c_num > 0; c_num--)
        //     {
        //         data[c_num+7] = data[c_num-1];
        //     }
        //     // payload = BLINKER_F("{\"data\":");
        //     // payload += data.substring(0, data.length() - 1);
        //     // payload += BLINKER_F(",\"fromDevice\":\"");
        //     // payload += MQTT_ID_MQTT_AT;
        //     // payload += BLINKER_F("\",\"toDevice\":\"");
        //     // payload += UUID_MQTT_AT;
        //     // payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        // }
        // else
        // {
        //     uint8_t num = strlen(data);
        //     for(uint8_t c_num = num; c_num > 0; c_num--)
        //     {
        //         data[c_num+7] = data[c_num-1];
        //     }
        //     // payload = BLINKER_F("{\"data\":");
        //     // payload += data;
        //     // payload += BLINKER_F(",\"fromDevice\":\"");
        //     // payload += MQTT_ID_MQTT_AT;
        //     // payload += BLINKER_F("\",\"toDevice\":\"");
        //     // payload += UUID_MQTT_AT;
        //     // payload += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        // }

        // uint16_t num = strlen(data);
        // for(uint16_t c_num = num; c_num > 0; c_num--)
        // {
        //     data[c_num+7] = data[c_num-1];
        // }

        // data[num+8] = '\0';

        // String data_add = BLINKER_F("{\"data\":");
        // for(uint8_t c_num = 0; c_num < 8; c_num++)
        // {
        //     data[c_num] = data_add[c_num];
        // }

        // data_add = BLINKER_F(",\"fromDevice\":\"");
        // strcat(data, data_add.c_str());
        // strcat(data, MQTT_ID_MQTT_AT);
        // data_add = BLINKER_F("\",\"toDevice\":\"");
        // strcat(data, data_add.c_str());
        // if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        // {
        //     strcat(data, _sharers[_sharerFrom]->uuid());
        // }
        // else
        // {
        //     strcat(data, UUID_MQTT_AT);
        // }
        // data_add = BLINKER_F("\",\"deviceType\":\"OwnApp\"}");
        // strcat(data, data_add.c_str());

        // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        // if (!isJson(STRING_format(data))) return false;

        String data_add = BLINKER_F("{\"data\":");
        data_add += data;
        data_add += BLINKER_F(",\"fromDevice\":\"");
        data_add += MQTT_ID_MQTT_AT;
        data_add += BLINKER_F("\",\"toDevice\":\"");
        if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        {
            data_add += _sharers[_sharerFrom]->uuid();
        }
        else
        {
            data_add += UUID_MQTT_AT;
        }

        data_add += BLINKER_F("\",\"deviceType\":\"OwnApp\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        if (!isJson(data_add)) return false;
        
        BLINKER_LOG_ALL(BLINKER_F("MQTT Publish..."));
        BLINKER_LOG_FreeHeap_ALL();
        
        bool _alive = isAlive;
        

        if (needCheck)
        {
            if (!checkPrintSpan())
            {
                return false;
            }
            respTime = millis();
        }

        if (mqtt_MQTT_AT->connected())
        {
            if (needCheck)
            {
                if (!checkCanPrint())
                {
                    if (!_alive)
                    {
                        isAlive = false;
                    }
                    return false;
                }

                if (!checkPrintLimit())
                {
                    return false;
                }

                _print_times++;

                BLINKER_LOG_ALL(BLINKER_F("_print_times: "), _print_times);
            }

            if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, data_add.c_str()))
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                BLINKER_LOG_FreeHeap_ALL();
                
                if (!_alive)
                {
                    isAlive = false;
                }
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(data_add);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                BLINKER_LOG_FreeHeap_ALL();
                
                if (needCheck) printTime = millis();

                if (!_alive)
                {
                    isAlive = false;
                }

                this->latestTime = millis();

                return true;
            }            
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
            isAlive = false;
            return false;
        }
    }
}

int BlinkerMQTTAT::mqttPrint(const String & data) {
    BLINKER_LOG_ALL(("mqttPrint data: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& print_data = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject print_data = jsonBuffer.as<JsonObject>();


    // if (!print_data.success())
    if (error)
    {
        BLINKER_ERR_LOG(("Print data not a Json data"));
        return false;
    }

    if (print_data.containsKey("toDeviceAT"))
    {

        String _dType = print_data["toDeviceAT"];

        BLINKER_LOG_ALL(("mqttPrint _dType: "), _dType);

        print_data.remove("toDeviceAT");
        String _data_;
        // print_data.printTo(_data_);
        serializeJson(print_data, _data_);

        if (_dType == "AliGenie")
        {
            return aliPrint(_data_);
        }
        else if (_dType == "DuerOS")
        {
            return duerPrint(_data_);
        }
        else
        {
            char data_print[512];
            strcpy(data_print, _data_.c_str());
            return print(data_print);
        }        
    }
    else
    {
        char data_print[512];
        strcpy(data_print, data.c_str());
        return print(data_print);
    }

    return false;
}

int BlinkerMQTTAT::bPrint(char * name, const String & data)
{
    // String payload;
    // if (STRING_contains_string(data, BLINKER_CMD_NEWLINE))
    // {
    //     payload = BLINKER_F("{\"data\":");
    //     payload += data.substring(0, data.length() - 1);
    //     payload += BLINKER_F(",\"fromDevice\":\"");
    //     payload += MQTT_ID_MQTT_AT;
    //     payload += BLINKER_F("\",\"toDevice\":\"");
    //     payload += name;
    //     payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    // }
    // else
    // {
    //     payload = BLINKER_F("{\"data\":");
    //     payload += data;
    //     payload += BLINKER_F(",\"fromDevice\":\"");
    //     payload += MQTT_ID_MQTT_AT;
    //     payload += BLINKER_F("\",\"toDevice\":\"");
    //     payload += name;
    //     payload += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    // }

    // uint8_t num = strlen(data);
    // for(uint8_t c_num = num; c_num > 0; c_num--)
    // {
    //     data[c_num+7] = data[c_num-1];
    // }

    String data_add = BLINKER_F("{\"data\":");
    // for(uint8_t c_num = 0; c_num < 8; c_num++)
    // {
    //     data[c_num] = data_add[c_num];
    // }

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_ID_MQTT_AT;
    data_add += BLINKER_F("\",\"toDevice\":\"");
    data_add += name;
    data_add += BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");

    // data_add = BLINKER_F(",\"fromDevice\":\"");
    // strcat(data, data_add.c_str());
    // strcat(data, MQTT_ID_MQTT_AT);
    // data_add = BLINKER_F("\",\"toDevice\":\"");
    // strcat(data, data_add.c_str());
    // strcat(data, name);
    // data_add = BLINKER_F("\",\"deviceType\":\"DiyBridge\"}");
    // strcat(data, data_add.c_str());

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(BLINKER_F("MQTT Bridge Publish..."));

    // bool _alive = isAlive;
    // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

    // if (!state) {
    //     state = (STRING_contains_string(data, BLINKER_CMD_STATE) 
    //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
    // }

    if (mqtt_MQTT_AT->connected()) {
        // if (!state) {
        if (!checkCanBprint()) {
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        // }

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT_AT, BLINKER_PUB_TOPIC_MQTT_AT);

        // if (! iotPub.publish(payload.c_str())) {

        String bPubTopic = BLINKER_F("");

        // if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET)
        // {
        //     bPubTopic = MQTT_PRODUCTINFO_MQTT_AT;
        //     bPubTopic += BLINKER_F("/");
        //     bPubTopic += name;
        //     bPubTopic += BLINKER_F("/r");
        // }
        // else
        // {
        //     bPubTopic = BLINKER_PUB_TOPIC_MQTT_AT;
        // }

        if (! mqtt_MQTT_AT->publish(bPubTopic.c_str(), data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            
            bPrintTime = millis();

            // if (!_alive) {
            //     isAlive = false;
            // }

            this->latestTime = millis();

            return true;
        }            
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        // isAlive = false;
        return false;
    }
    // }
}

int BlinkerMQTTAT::aliPrint(const String & data)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_ID_MQTT_AT;
    data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT AliGenie Publish..."));
    BLINKER_LOG_ALL(data_add);
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_MQTT_AT->connected())
    {
        if (!checkAliKA())
        {
            return false;
        }

        if (!checkAliPrintSpan())
        {
            respAliTime = millis();
            return false;
        }
        respAliTime = millis();

        if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, data_add.c_str()))
        {
            // BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            isAliAlive = false;
            return false;
        }
        else
        {
            // BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();
            
            isAliAlive = false;

            this->latestTime = millis();

            return true;
        }      
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerMQTTAT::duerPrint(const String & data, bool report)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_ID_MQTT_AT;
    data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT DuerOS Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_MQTT_AT->connected())
    {
        if (!checkDuerKA())
        {
            return false;
        }

        if (!checkDuerPrintSpan())
        {
            respDuerTime = millis();
            return false;
        }
        respDuerTime = millis();

        if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            isDuerAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();
            
            isDuerAlive = false;

            this->latestTime = millis();

            return true;
        }      
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerMQTTAT::miPrint(const String & data)
{
    String data_add = BLINKER_F("{\"data\":");

    data_add += data;
    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += MQTT_ID_MQTT_AT;
    data_add += BLINKER_F("\",\"toDevice\":\"MIOT_r\"");
    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;
            
    BLINKER_LOG_ALL(BLINKER_F("MQTT MIOT Publish..."));
    BLINKER_LOG_FreeHeap_ALL();

    if (mqtt_MQTT_AT->connected())
    {
        if (!checkMIOTKA())
        {
            return false;
        }

        if (!checkMIOTPrintSpan())
        {
            respMIOTTime = millis();
            return false;
        }
        respMIOTTime = millis();

        if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, data_add.c_str()))
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();
            
            isMIOTAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(data_add);
            BLINKER_LOG_ALL(BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();
            
            isMIOTAlive = false;

            this->latestTime = millis();

            return true;
        }      
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

// void BlinkerMQTTAT::aliType(const String & type)
// {
//     _aliType = (char*)malloc((type.length()+1)*sizeof(char));
//     strcpy(_aliType, type.c_str());
//     BLINKER_LOG_ALL(BLINKER_F("_aliType: "), _aliType);
// }

void BlinkerMQTTAT::begin(const char* auth) {
    // _authKey = auth;
    _authKey = (char*)malloc((strlen(auth)+1)*sizeof(char));
    strcpy(_authKey, auth);
    
    BLINKER_LOG_ALL(BLINKER_F("_authKey: "), auth);

    // _isBegin = begin();
}

bool BlinkerMQTTAT::begin()
{
    // if (connectServer()) {
    //     mDNSInit();
    //     isMQTTinit = true;
    // }
    // else {
    uint32_t re_time = millis();
    bool isConnect_MQTT_AT = true;

    if (isMQTTinit) return true;

    // while(1)
    {
        re_time = millis();
        // ::delay(10000);
        // BLINKER_ERR_LOG("Maybe you have put in the wrong AuthKey!");
        // BLINKER_ERR_LOG("Or maybe your request is too frequently!");
        // BLINKER_ERR_LOG("Or maybe your network is disconnected!");
        BLINKER_LOG_ALL("connectServer");
        if (connectServer()) {
            mDNSInit();
            isMQTTinit = true;
            return true;
        }
        else {
            isMQTTinit = false;

            return false;
        }
        // delay(10000);
        // while ((millis() - re_time) < 10000)
        // {
        //     if (WiFi.status() != WL_CONNECTED && isConnect_MQTT_AT)
        //     {
        //         isConnect_MQTT_AT = false;
        //         WiFi.begin();
        //         WiFi.reconnect();
        //     }
        //     else if (WiFi.status() == WL_CONNECTED && !isConnect_MQTT_AT)
        //     {
        //         isConnect_MQTT_AT = true;
        //     }
        //     ::delay(10);
        //     // WiFi.status();
        // }
    }
    // }
}

int BlinkerMQTTAT::autoPrint(unsigned long id)
{
    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"trigged\":true,\"autoData\":{");
    payload += BLINKER_F("\"autoId\":");
    payload += STRING_format(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(MQTT_ID_MQTT_AT);
    payload += BLINKER_F("\",\"toDevice\":\"autoManager\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));

    if (mqtt_MQTT_AT->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT_AT, BLINKER_PUB_TOPIC_MQTT_AT);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, payload.c_str()))
            {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL(BLINKER_F("...Failed"));
                
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL(BLINKER_F("...OK!"));
                
                linkTime = millis();

                this->latestTime = millis();
                
                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

// int BlinkerMQTTAT::autoPrint(char *name, char *type, char *data)
// {
//     String payload = BLINKER_F("{\"data\":{");
//     payload += STRING_format(data);
//     payload += BLINKER_F("},\"fromDevice\":\"");
//     payload += STRING_format(MQTT_ID_MQTT_AT);
//     payload += BLINKER_F("\",\"toDevice\":\"");
//     payload += name;
//     payload += BLINKER_F("\",\"deviceType\":\"");
//     payload += type;
//     payload += BLINKER_F("\"}");
        
//     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));
    
//     if (mqtt_MQTT_AT->connected()) {
//         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || 
//             linkTime == 0)
//         {
//             linkTime = millis();
            
//             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));
            
//             return true;
//         }
//         else {
//             BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
//             return false;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

// int BlinkerMQTTAT::autoPrint(char *name1, char *type1, char *data1
//     , char *name2, char *type2, char *data2)
// {
//     String payload = BLINKER_F("{\"data\":{");
//     payload += STRING_format(data1);
//     payload += BLINKER_F("},\"fromDevice\":\"");
//     payload += STRING_format(MQTT_ID_MQTT_AT);
//     payload += BLINKER_F("\",\"toDevice\":\"");
//     payload += name1;
//     payload += BLINKER_F("\",\"deviceType\":\"");
//     payload += type1;
//     payload += BLINKER_F("\"}");
        
//     BLINKER_LOG_ALL(BLINKER_F("autoPrint..."));
    
//     if (mqtt_MQTT_AT->connected())
//     {
//         if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || 
//             linkTime == 0)
//         {
//             linkTime = millis();

//             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));

//             payload = BLINKER_F("{\"data\":{");
//             payload += STRING_format(data2);
//             payload += BLINKER_F("},\"fromDevice\":\"");
//             payload += STRING_format(MQTT_ID_MQTT_AT);
//             payload += BLINKER_F("\",\"toDevice\":\"");
//             payload += name2;
//             payload += BLINKER_F("\",\"deviceType\":\"");
//             payload += type2;
//             payload += BLINKER_F("\"}");
                
//             BLINKER_LOG_ALL(payload, BLINKER_F("...OK!"));
            
//             return true;
//         }
//         else
//         {
//             BLINKER_ERR_LOG_ALL(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);
            
//             return false;
//         }
//     }
//     else
//     {
//         BLINKER_ERR_LOG(BLINKER_F("MQTT Disconnected"));
//         return false;
//     }
// }

char * BlinkerMQTTAT::deviceName() { return DEVICE_NAME_MQTT_AT;/*MQTT_ID_MQTT_AT;*/ }

char * BlinkerMQTTAT::deviceId() { return MQTT_ID_MQTT_AT; }

char * BlinkerMQTTAT::uuid() { return UUID_MQTT_AT; }

void BlinkerMQTTAT::sharers(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("sharers data: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success()) return;
    if (error) return;

    String user_name = "";

    if (_sharerCount)
    {
        for (_sharerCount; _sharerCount > 0; _sharerCount--)
        {
            delete _sharers[_sharerCount - 1];
        }
    }

    _sharerCount = 0;

    for (uint8_t num = 0; num < BLINKER_MQTT_MAX_SHARERS_NUM; num++)
    {
        user_name = root["users"][num].as<String>();

        if (user_name.length() >= BLINKER_MQTT_USER_UUID_SIZE)
        {
            BLINKER_LOG_ALL(BLINKER_F("sharer uuid: "), user_name, BLINKER_F(", length: "), user_name.length());

            _sharerCount++;

            _sharers[num] = new BlinkerSharer(user_name);
        }
        else
        {
            break;
        }
    }
}

void BlinkerMQTTAT::softAPinit()
{
    WiFiServer _server(80);
    WiFiClient _client;
    IPAddress apIP(192, 168, 4, 1);
    #if defined(ESP8266)
        IPAddress netMsk(255, 255, 255, 0);
    #endif

    // _server = new WiFiServer(80);

    WiFi.mode(WIFI_AP);
    String softAP_ssid = BLINKER_F("DiyArduino_");
    softAP_ssid += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(softAP_ssid.c_str());
        WiFi.softAPConfig(apIP, apIP, netMsk);
    #elif defined(ESP32)
        WiFi.setHostname(softAP_ssid.c_str());
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    #endif
    
    WiFi.softAP(softAP_ssid.c_str(), ("12345678"));
    delay(100);

    _server.begin();
    BLINKER_LOG(BLINKER_F("AP IP address: "), WiFi.softAPIP());
    BLINKER_LOG(BLINKER_F("HTTP _server started"));
    BLINKER_LOG(BLINKER_F("URL: http://"), WiFi.softAPIP());

    while(WiFi.status() != WL_CONNECTED)
    {
        // serverClient();
        _client = _server.available();
        // if (_client.status() == CLOSED)
        if (!_client.connected())
        {
            _client.stop();
            BLINKER_LOG(BLINKER_F("Connection closed on _client"));
        }
        else
        {
            if (_client.available())
            {
                String data = _client.readStringUntil('\r');

                // data = data.substring(4, data.length() - 9);
                _client.flush();

                BLINKER_LOG(BLINKER_F("clientData: "), data);

                if (STRING_contains_string(data, "ssid") && \
                    STRING_contains_string(data, "pswd"))
                {
                    String msg = BLINKER_F("{\"hello\":\"world\"}");
                    
                    String s= BLINKER_F("HTTP/1.1 200 OK\r\n");
                    s += BLINKER_F("Content-Type: application/json;");
                    s += BLINKER_F("charset=utf-8\r\n");
                    s += BLINKER_F("Content-Length: ");
                    s += String(msg.length());
                    s += BLINKER_F("\r\nConnection: Keep Alive\r\n\r\n");
                    s += msg;
                    s += BLINKER_F("\r\n");

                    _client.print(s);
                    
                    _client.stop();

                    parseUrl(data);
                }
            }
        }
        ::delay(10);
    }
}

int BlinkerMQTTAT::parseUrl(String data)
{
    BLINKER_LOG(BLINKER_F("APCONFIG data: "), data);
    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& wifi_data = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject wifi_data = jsonBuffer.as<JsonObject>();

    // if (!wifi_data.success()) 
    if (error)
    {
        return false;
    }
                    
    String _ssid = wifi_data["ssid"];
    String _pswd = wifi_data["pswd"];

    BLINKER_LOG(BLINKER_F("ssid: "), _ssid);
    BLINKER_LOG(BLINKER_F("pswd: "), _pswd);

    // free(_server);
    connectWiFi(_ssid, _pswd);
    return true;
}

int BlinkerMQTTAT::autoInit()
{
    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduino_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    WiFi.begin();
    ::delay(500);

    BLINKER_LOG(BLINKER_F("Waiting for WiFi "), 
                BLINKER_WIFI_AUTO_INIT_TIMEOUT / 1000,
                BLINKER_F("s, will enter SMARTCONFIG or "),
                BLINKER_F("APCONFIG while WiFi not connect!"));

    uint8_t _times = 0;
    while (WiFi.status() != WL_CONNECTED) {
        ::delay(500);
        if (_times > BLINKER_WIFI_AUTO_INIT_TIMEOUT / 500) break;
        _times++;
    }

    if (WiFi.status() != WL_CONNECTED) return false;
    else {
        BLINKER_LOG(BLINKER_F("WiFi Connected."));
        BLINKER_LOG(BLINKER_F("IP Address: "));
        BLINKER_LOG(WiFi.localIP());
        // mDNSInit();
        return true;
    }
}

void BlinkerMQTTAT::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerMQTTAT::connectWiFi(const char* _ssid, const char* _pswd)
{
    uint32_t connectTime = millis();

    BLINKER_LOG(BLINKER_F("Connecting to "), _ssid);

    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduinoWIFI_");
    _hostname += macDeviceName();
    
    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    if (_pswd && strlen(_pswd)) {
        WiFi.begin(_ssid, _pswd);
    }
    else {
        WiFi.begin(_ssid);
    }

    while (WiFi.status() != WL_CONNECTED) {
        ::delay(50);

        if (millis() - connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
            connectTime = millis();
            BLINKER_LOG(BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
            BLINKER_LOG(BLINKER_F("Retring WiFi connect again!"));
        }
    }
    BLINKER_LOG(BLINKER_F("Connected"));

    IPAddress myip = WiFi.localIP();
    BLINKER_LOG(BLINKER_F("Your IP is: "), myip);

    // mDNSInit();
}

void BlinkerMQTTAT::smartconfig()
{
    // Base::begin();
    if (!autoInit()) //smartconfig();
    {
        WiFi.mode(WIFI_STA);
        String _hostname = BLINKER_F("DiyArduino_");
        _hostname += macDeviceName();
        
        #if defined(ESP8266)
            WiFi.hostname(_hostname.c_str());
        #elif defined(ESP32)
            WiFi.setHostname(_hostname.c_str());
        #endif

        WiFi.beginSmartConfig();
        
        BLINKER_LOG(BLINKER_F("Waiting for SmartConfig."));
        while (!WiFi.smartConfigDone()) {
            ::delay(500);
        }

        BLINKER_LOG(BLINKER_F("SmartConfig received."));
        
        BLINKER_LOG(BLINKER_F("Waiting for WiFi"));
        while (WiFi.status() != WL_CONNECTED) {
            ::delay(500);
        }

        BLINKER_LOG(BLINKER_F("WiFi Connected."));

        BLINKER_LOG(BLINKER_F("IP Address: "));
        BLINKER_LOG(WiFi.localIP());
    }
    // Base::loadTimer();

    #if defined(ESP8266)
        BLINKER_LOG(BLINKER_F("ESP8266_WIFI initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(BLINKER_F("ESP32_WIFI initialized..."));
    #endif
}

int BlinkerMQTTAT::connectServer() {
    const int httpsPort = 443;
#if defined(ESP8266)
    String host = BLINKER_F(BLINKER_SERVER_HOST);
    String fingerprint = BLINKER_F("84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a");

    // BearSSL::WiFiClientSecure *client_s;

    // client_s = new BearSSL::WiFiClientSecure();

    // client_mqtt.stop();
    
    // BLINKER_LOG_ALL(BLINKER_F("connecting to "), host);

    // // BLINKER_LOG_FreeHeap();
    
    // uint8_t connet_times = 0;
    // // client_s.stop();
    // ::delay(100);

    // bool mfln = client_s->probeMaxFragmentLength(host, httpsPort, 1024);
    // if (mfln) {
    //     client_s->setBufferSizes(1024, 1024);
    // }
    // // client_s.setFingerprint(fingerprint.c_str());

    // client_s->setInsecure();

    // // while (1) {
    //     bool cl_connected = false;
    //     if (!client_s->connect(host, httpsPort)) {
    //         BLINKER_ERR_LOG(BLINKER_F("server connection failed"));
    //         // connet_times++;

    //         ::delay(1000);
    //     }
    //     else {
    //         BLINKER_LOG_ALL(BLINKER_F("connection succeed"));
    //         cl_connected = true;

    //         // break;
    //     }

    //     // if (connet_times >= 4 && !cl_connected)  return BLINKER_CMD_FALSE;
    // // }

    // String client_msg;

    // String url_iot = BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    // url_iot += _authKey;

    // if (_aliType == ALI_LIGHT) {
    //     url_iot += BLINKER_F("&aliType=light");
    // }
    // else if (_aliType == ALI_OUTLET) {
    //     url_iot += BLINKER_F("&aliType=outlet");
    // }
    // else if (_aliType == ALI_SENSOR) {
    //     url_iot += BLINKER_F("&aliType=sensor");
    // }

    // if (_duerType == DUER_LIGHT) {
    //     url_iot += BLINKER_F("&duerType=LIGHT");
    // }
    // else if (_duerType == DUER_OUTLET) {
    //     url_iot += BLINKER_F("&duerType=SOCKET");
    // }
    // else if (_duerType == DUER_SENSOR) {
    //     url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    // }

    // BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), host, url_iot);
    
    // client_msg = BLINKER_F("GET ");
    // client_msg += url_iot;
    // client_msg += BLINKER_F(" HTTP/1.1\r\nHost: ");
    // client_msg += host;
    // client_msg += BLINKER_F(":");
    // client_msg += STRING_format(httpsPort);
    // client_msg += BLINKER_F("\r\nConnection: close\r\n\r\n");

    // client_s->print(client_msg);
    
    // BLINKER_LOG_ALL(BLINKER_F("client_msg: "), client_msg);

    // unsigned long timeout = millis();
    // while (client_s->available() == 0) {
    //     if (millis() - timeout > 5000) {
    //         BLINKER_LOG_ALL(BLINKER_F(">>> Client Timeout !"));
    //         client_s->stop();
    //         return false;
    //     }
    // }

    // String _dataGet;
    // String lastGet;
    // String lengthOfJson;
    // while (client_s->available()) {
    //     // String line = client_s.readStringUntil('\r');
    //     _dataGet = client_s->readStringUntil('\n');

    //     if (_dataGet.startsWith("Content-Length: ")){
    //         int addr_start = _dataGet.indexOf(' ');
    //         int addr_end = _dataGet.indexOf('\0', addr_start + 1);
    //         lengthOfJson = _dataGet.substring(addr_start + 1, addr_end);
    //     }

    //     if (_dataGet == "\r") {
    //         BLINKER_LOG_ALL(BLINKER_F("headers received"));
            
    //         break;
    //     }
    // }

    // for(int i=0;i<lengthOfJson.toInt();i++){
    //     lastGet += (char)client_s->read();
    // }

    // // BLINKER_LOG_FreeHeap();

    // client_s->stop();
    // client_s->flush();

    // free(client_s);

    // // BLINKER_LOG_FreeHeap();

    // _dataGet = lastGet;
    
    // BLINKER_LOG_ALL(BLINKER_F("_dataGet: "), _dataGet);

    // String payload = _dataGet;



    client_mqtt.stop();

    std::unique_ptr<BearSSL::WiFiClientSecure>client_s(new BearSSL::WiFiClientSecure);

    // client_s->setFingerprint(fingerprint);
    client_s->setInsecure();

    String url_iot = BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    url_iot += _authKey;
    // url_iot += _aliType;

    if (_aliType == ALI_LIGHT) {
        url_iot += BLINKER_F("&aliType=light");
    }
    else if (_aliType == ALI_OUTLET) {
        url_iot += BLINKER_F("&aliType=outlet");
    }
    else if (_aliType == ALI_SENSOR) {
        url_iot += BLINKER_F("&aliType=sensor");
    }

    if (_duerType == DUER_LIGHT) {
        url_iot += BLINKER_F("&duerType=LIGHT");
    }
    else if (_duerType == DUER_OUTLET) {
        url_iot += BLINKER_F("&duerType=SOCKET");
    }
    else if (_duerType == DUER_SENSOR) {
        url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    }

    url_iot = "https://" + host + url_iot;

    BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... url_iot: "), url_iot);

    BLINKER_LOG_FreeHeap_ALL();

    HTTPClient http;

    String payload;

    if (http.begin(*client_s, url_iot)) {  // HTTPS

        // Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            
            BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                payload = http.getString();
                // Serial.println(payload);
            }
        } else {
            BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
            payload = http.getString();
            BLINKER_LOG(payload);
        }

        http.end();
    } else {
        // Serial.printf("[HTTPS] Unable to connect\n");
        BLINKER_ERR_LOG_ALL("[HTTPS] Unable to connect\n");
    }

#elif defined(ESP32)
    String host = BLINKER_F(BLINKER_SERVER_HTTPS);
    // const char* ca = 
    //     "-----BEGIN CERTIFICATE-----\n" 
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n" 
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n" 
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n" 
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n" 
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n" 
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n" 
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n" 
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n" 
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n" 
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n" 
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n" 
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n" 
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n" 
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n" 
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n" 
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n" 
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n" 
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n" 
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n" 
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n" 
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n" 
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n" 
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n" 
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n" 
    //     "9UBd0Q==\n" 
    //     "-----END CERTIFICATE-----\n";
// #endif

    HTTPClient http;

    String url_iot = host;
    url_iot += BLINKER_F("/api/v1/user/device/diy/auth?authKey=");
    url_iot += _authKey;
    // url_iot += _aliType;

    if (_aliType == ALI_LIGHT) {
        url_iot += BLINKER_F("&aliType=light");
    }
    else if (_aliType == ALI_OUTLET) {
        url_iot += BLINKER_F("&aliType=outlet");
    }
    else if (_aliType == ALI_SENSOR) {
        url_iot += BLINKER_F("&aliType=sensor");
    }

    if (_duerType == DUER_LIGHT) {
        url_iot += BLINKER_F("&duerType=LIGHT");
    }
    else if (_duerType == DUER_OUTLET) {
        url_iot += BLINKER_F("&duerType=SOCKET");
    }
    else if (_duerType == DUER_SENSOR) {
        url_iot += BLINKER_F("&duerType=AIR_MONITOR");
    }

// #if defined(BLINKER_ALIGENIE_LIGHT)
//     url_iot += BLINKER_F("&aliType=light");
// #elif defined(BLINKER_ALIGENIE_OUTLET)
//     url_iot += BLINKER_F("&aliType=outlet");
// #elif defined(BLINKER_ALIGENIE_SWITCH)
// #elif defined(BLINKER_ALIGENIE_SENSOR)
//     url_iot += BLINKER_F("&aliType=sensor");
// #endif

    BLINKER_LOG_ALL(BLINKER_F("HTTPS begin: "), url_iot);

// #if defined(ESP8266)
//     http.begin(url_iot, fingerprint); //HTTP
// #elif defined(ESP32)
    // http.begin(url_iot, ca); TODO
    http.begin(url_iot);
// #endif
    int httpCode = http.GET();

    String payload;

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled

        BLINKER_LOG_ALL(BLINKER_F("[HTTP] GET... code: "), httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            // BLINKER_LOG(payload);
        }
    }
    else {
        BLINKER_LOG(BLINKER_F("[HTTP] GET... failed, error: "), http.errorToString(httpCode).c_str());
        payload = http.getString();
        BLINKER_LOG(payload);
    }

    http.end();
#endif

    BLINKER_LOG_ALL(BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(BLINKER_F("=============================="));
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL(BLINKER_F("=============================="));

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(payload);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG(BLINKER_F("Maybe you have put in the wrong AuthKey!"));
            BLINKER_ERR_LOG(BLINKER_F("Or maybe your request is too frequently!"));
            BLINKER_ERR_LOG(BLINKER_F("Or maybe your network is disconnected!"));
            // ::delay(60000);

            return false;
        // }
    }

    // String _userID = STRING_find_string(payload, "deviceName", "\"", 4);
    // String _userName = STRING_find_string(payload, "iotId", "\"", 4);
    // String _key = STRING_find_string(payload, "iotToken", "\"", 4);
    // String _productInfo = STRING_find_string(payload, "productKey", "\"", 4);
    // String _broker = STRING_find_string(payload, "broker", "\"", 4);
    // String _uuid = STRING_find_string(payload, "uuid", "\"", 4);
    String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];

    if (isMQTTinit)
    {
        free(MQTT_HOST_MQTT_AT);
        free(MQTT_ID_MQTT_AT);
        free(MQTT_NAME_MQTT_AT);
        free(MQTT_KEY_MQTT_AT);
        free(MQTT_PRODUCTINFO_MQTT_AT);
        free(UUID_MQTT_AT);
        free(DEVICE_NAME_MQTT_AT);
        free(BLINKER_PUB_TOPIC_MQTT_AT);
        free(BLINKER_SUB_TOPIC_MQTT_AT);
        free(mqtt_MQTT_AT);
        free(iotSub_MQTT_AT);

        isMQTTinit = false;
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME_MQTT_AT, _userID.c_str(), 12);
        DEVICE_NAME_MQTT_AT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT_AT, _userID.c_str());
        MQTT_ID_MQTT_AT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT_AT, _userID.c_str());
        MQTT_NAME_MQTT_AT = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT_AT, _userName.c_str());
        MQTT_KEY_MQTT_AT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT_AT, _key.c_str());
        MQTT_PRODUCTINFO_MQTT_AT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT_AT, _productInfo.c_str());
        MQTT_HOST_MQTT_AT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT_AT, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT_MQTT_AT = BLINKER_MQTT_ALIYUN_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        // String id2name = _userID.subString(10, _userID.length());
        // memcpy(DEVICE_NAME_MQTT_AT, _userID.c_str(), 12);
        DEVICE_NAME_MQTT_AT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT_AT, _userID.c_str());
        String IDtest = _productInfo + _userID;
        MQTT_ID_MQTT_AT = (char*)malloc((IDtest.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT_AT, IDtest.c_str());
        String NAMEtest = IDtest + ";" + _userName;
        MQTT_NAME_MQTT_AT = (char*)malloc((NAMEtest.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT_AT, NAMEtest.c_str());
        MQTT_KEY_MQTT_AT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT_AT, _key.c_str());
        MQTT_PRODUCTINFO_MQTT_AT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT_AT, _productInfo.c_str());
        MQTT_HOST_MQTT_AT = (char*)malloc((strlen(BLINKER_MQTT_QCLOUD_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT_AT, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT_MQTT_AT = BLINKER_MQTT_QCLOUD_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        // memcpy(DEVICE_NAME_MQTT_AT, _userID.c_str(), 12);
        DEVICE_NAME_MQTT_AT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT_AT, _userID.c_str());
        MQTT_ID_MQTT_AT = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT_AT, _userName.c_str());
        MQTT_NAME_MQTT_AT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT_AT, _productInfo.c_str());
        MQTT_KEY_MQTT_AT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT_AT, _key.c_str());
        MQTT_PRODUCTINFO_MQTT_AT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT_AT, _productInfo.c_str());
        MQTT_HOST_MQTT_AT = (char*)malloc((strlen(BLINKER_MQTT_ONENET_HOST)+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT_AT, BLINKER_MQTT_ONENET_HOST);
        MQTT_PORT_MQTT_AT = BLINKER_MQTT_ONENET_PORT;
    }
    UUID_MQTT_AT = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_MQTT_AT, _uuid.c_str());
    
    BLINKER_LOG_ALL(BLINKER_F("===================="));
    BLINKER_LOG_ALL(BLINKER_F("DEVICE_NAME_MQTT_AT: "), DEVICE_NAME_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_PRODUCTINFO_MQTT_AT: "), MQTT_PRODUCTINFO_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_ID_MQTT_AT: "), MQTT_ID_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_NAME_MQTT_AT: "), MQTT_NAME_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_KEY_MQTT_AT: "), MQTT_KEY_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(BLINKER_F("HOST: "), MQTT_HOST_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("PORT: "), MQTT_PORT_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("UUID_MQTT_AT: "), UUID_MQTT_AT);
    BLINKER_LOG_ALL(BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        String PUB_TOPIC_STR = BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT_AT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_ID_MQTT_AT;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_MQTT_AT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT_AT: "), BLINKER_PUB_TOPIC_MQTT_AT);
        
        String SUB_TOPIC_STR = BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT_AT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_ID_MQTT_AT;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_MQTT_AT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT_AT: "), BLINKER_SUB_TOPIC_MQTT_AT);
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        String PUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT_AT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += _userID;
        PUB_TOPIC_STR += BLINKER_F("/s");

        BLINKER_PUB_TOPIC_MQTT_AT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT_AT: "), BLINKER_PUB_TOPIC_MQTT_AT);
        
        String SUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT_AT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += _userID;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_MQTT_AT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT_AT: "), BLINKER_SUB_TOPIC_MQTT_AT);
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        uint8_t str_len;
        String PUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT_AT;
        PUB_TOPIC_STR += BLINKER_F("/onenet_rule/r");
        // str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC_MQTT_AT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_PUB_TOPIC_MQTT_AT: "), BLINKER_PUB_TOPIC_MQTT_AT);
        
        String SUB_TOPIC_STR = MQTT_PRODUCTINFO_MQTT_AT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += _userID;
        SUB_TOPIC_STR += BLINKER_F("/r");
        
        BLINKER_SUB_TOPIC_MQTT_AT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str());
        
        BLINKER_LOG_ALL(BLINKER_F("BLINKER_SUB_TOPIC_MQTT_AT: "), BLINKER_SUB_TOPIC_MQTT_AT);
    }

    // BLINKER_LOG_FreeHeap();

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        #if defined(ESP8266)
            // bool mfln = client_mqtt.probeMaxFragmentLength(MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, 4096);
            // if (mfln) {
            //     client_mqtt.setBufferSizes(1024, 1024);
            // }
            // client_mqtt.setInsecure();
            mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
        #elif defined(ESP32)
            mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
        #endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        #if defined(ESP8266)
            // bool mfln = client_mqtt.probeMaxFragmentLength(MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, 4096);
            // if (mfln) {
            //     client_mqtt.setBufferSizes(1024, 1024);
            // }
            // client_mqtt.setInsecure();
            mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
        #elif defined(ESP32)
            mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
        #endif
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
    }

    // iotPub = new Adafruit_MQTT_Publish(mqtt_MQTT_AT, BLINKER_PUB_TOPIC_MQTT_AT);
    // if (!isMQTTinit) 
    iotSub_MQTT_AT = new Adafruit_MQTT_Subscribe(mqtt_MQTT_AT, BLINKER_SUB_TOPIC_MQTT_AT);

    mqtt_broker = (char*)malloc((_broker.length()+1)*sizeof(char));
    strcpy(mqtt_broker, _broker.c_str());
    // mqtt_broker = _broker;

    // mDNSInit(MQTT_ID_MQTT_AT);
    this->latestTime = millis();
    // if (!isMQTTinit) 
    mqtt_MQTT_AT->subscribe(iotSub_MQTT_AT);
    
    #if defined(ESP8266)
        // client_s->stop();
        client_mqtt.setInsecure();
    #endif
    // connect();

    return true;
}

void BlinkerMQTTAT::mDNSInit()
{
#if defined(ESP8266)
    if (!MDNS.begin(DEVICE_NAME_MQTT_AT, WiFi.localIP())) {
#elif defined(ESP32)
    if (!MDNS.begin(DEVICE_NAME_MQTT_AT)) {
#endif
        while(1) {
            ::delay(100);
        }
    }

    BLINKER_LOG(BLINKER_F("mDNS responder started"));
    
    MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME_MQTT_AT));

    webSocket_MQTT_AT.begin();
    webSocket_MQTT_AT.onEvent(webSocketEvent_MQTT_AT);
    BLINKER_LOG(BLINKER_F("webSocket_MQTT_AT server started"));
    BLINKER_LOG(BLINKER_F("ws://"), DEVICE_NAME_MQTT_AT, BLINKER_F(".local:"), WS_SERVERPORT);
}

void BlinkerMQTTAT::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerMQTTAT::checkAliKA() {
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerMQTTAT::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerMQTTAT::checkMIOTKA() {
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerMQTTAT::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

int BlinkerMQTTAT::checkCanBprint() {
    if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        return false;
    }
}

int BlinkerMQTTAT::checkPrintSpan() {
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
            BLINKER_ERR_LOG(BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));
            
            return false;
        }
        else {
            respTimes++;
            return true;
        }
    }
    else {
        respTimes = 0;
        return true;
    }
}

int BlinkerMQTTAT::checkMIOTPrintSpan()
{
    if (millis() - respMIOTTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respMIOTTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

            return false;
        }
        else
        {
            respMIOTTimes++;
            return true;
        }
    }
    else
    {
        respMIOTTimes = 0;
        return true;
    }
}

int BlinkerMQTTAT::checkAliPrintSpan()
{
    if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(BLINKER_F("ALIGENIE NOT ALIVE OR MSG LIMIT"));
            
            return false;
        }
        else
        {
            respAliTimes++;
            return true;
        }
    }
    else
    {
        respAliTimes = 0;
        return true;
    }
}

int BlinkerMQTTAT::checkDuerPrintSpan()
{
    if (millis() - respDuerTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respDuerTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));
            
            return false;
        }
        else
        {
            respDuerTimes++;
            return true;
        }
    }
    else
    {
        respDuerTimes = 0;
        return true;
    }
}

int BlinkerMQTTAT::checkPrintLimit()
{
    if ((millis() - _print_time) < 60000)
    {
        if (_print_times < 10) return true;
        else 
        {
            BLINKER_ERR_LOG(BLINKER_F("MQTT MSG LIMIT"));
            return false;
        }
    }
    else
    {
        _print_time = millis();
        _print_times = 0;
        return true;
    }
}

int BlinkerMQTTAT::isJson(const String & data)
{
    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(STRING_format(data));
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG("Print data is not Json! ", data);
        return false;
    }

    return true;
}

#endif

#endif
