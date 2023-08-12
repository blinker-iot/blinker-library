#ifndef BLINKER_WIFI_ESP_AT_H
#define BLINKER_WIFI_ESP_AT_H

#if (defined(ESP8266) || defined(ESP32))

#define BLINKER_WIFI_AT
#define BLINKER_ESP_AT

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFiMulti.h>
    #include <ESP8266HTTPClient.h>

    #include <base64.h>

    ESP8266WiFiMulti wifiMulti;
#elif defined(ESP32)
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #include <WiFiMulti.h>
    #include <HTTPClient.h>

    #include <base64.h>

    WiFiMulti  wifiMulti;
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
// #include "../Blinker/BlinkerStream.h"
#include "../Blinker/BlinkerUtility.h"
#include "../Blinker/BlinkerHTTP.h"
#include "../Blinker/BlinkerMQTTATBase.h"

static const char *TAG_ESP_AT = "[BlinkerWiFiESPAT] ";

enum b_config_t {
    COMM,
    BLINKER_SMART_CONFIG,
    BLINKER_AP_CONFIG,
    BLINKER_MULTI
};

enum b_configStatus_t {
    AUTO_INIT,
    AUTO_DONE,
    SMART_BEGIN,
    SMART_DONE,
    SMART_TIMEOUT,
    APCFG_BEGIN,
    APCFG_DONE,
    APCFG_TIMEOUT
};

enum b_broker_t {
    aliyun_b,
    blinker_b
};

char*       MQTT_HOST_MQTT_AT;
char*       MQTT_ID_MQTT_AT;
char*       MQTT_NAME_MQTT_AT;
char*       MQTT_KEY_MQTT_AT;
char*       MQTT_PRODUCTINFO_MQTT_AT;
char*       UUID_MQTT_AT;
char*       DEVICE_NAME_MQTT_AT;
char*       BLINKER_PUB_TOPIC_MQTT_AT;
char*       BLINKER_SUB_TOPIC_MQTT_AT;
// char*       BLINKER_RRPC_PUB_TOPIC_MQTT;
char*       BLINKER_RRPC_SUB_TOPIC_MQTT_AT;
uint16_t    MQTT_PORT_MQTT_AT;

class BlinkerWiFiESPAT
{
    public :
        BlinkerWiFiESPAT();

        // void atBegin();

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
        
        void begin(const char* auth);
        bool init();
        void reset();
        b_device_staus_t status()       { return _status;}
        bool connect();
        void disconnect();
        void ping();
        bool available();
        char *lastRead();
        void flush();
        int  print(char * data, bool needCheck = true);
        int  mqttPrint(const String & data);
        int  autoPrint(unsigned long id);
        void subscribe();
        int  isJson(const String & data);
        bool deviceRegister();
        void freshSharers();

        char * deviceName()             { return DEVICE_NAME_MQTT_AT; }
        char * deviceId()               { return MQTT_ID_MQTT_AT; }
        char * uuid()                   { return UUID_MQTT_AT; }
        char * authKey()                { return (char *)_authKey; }
        char * token()                  { if (!isMQTTinit) return ""; else return MQTT_KEY_MQTT_AT; }

        String httpServer(uint8_t _type, const String & msg, bool state = false)
        { return httpToServer(_type, msg, state); }
        
        void setAliType(int _type)  { _aliType = _type; }
        void setDuerType(int _type) { _duerType = _type; }
        void setMiType(int _type)   { _miType = _type; }
        int getAliType()  { return _aliType; }
        int getDuerType() { return _duerType; }
        int getMiType()   { return _miType; }
        
        bool checkInit()                    { return isMQTTinit; }
        bool checkWlanInit();
        void commonBegin(const char* _ssid, const char* _pswd);
        // #if defined(BLINKER_ESP_SMARTCONFIG)
        void smartconfigBegin();
        void smartconfig();
        // #elif defined(BLINKER_WIFI_MULTI)
        void multiBegin(const char* _ssid, const char* _pswd);
        // #elif defined(BLINKER_APCONFIG)
        void apconfigBegin();
        void softAPinit();
        void checkAPCFG();
        bool parseUrl(String data);
        // #endif
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);

    // #if defined(BLINKER_ALIGENIE)
        int aliAvail();
        int aliPrint(const String & data);
    // #endif
    
    // #if defined(BLINKER_DUEROS)
        int duerAvail();
        int duerPrint(const String & data, bool report);
    // #endif

    // #if defined(BLINKER_MIOT)
        int miAvail();
        int miPrint(const String & data);
    // #endif

    
        bool autoInit();

    private :
        // void parseATdata();

        void mDNSInit();
        bool checkConfig();

        void checkKA();
        int checkCanPrint();
        int checkPrintSpan();
        int checkPrintLimit();

    // #if defined(BLINKER_ALIGENIE)
        int checkAliKA();
        int checkAliPrintSpan();
    // #endif

    // #if defined(BLINKER_DUEROS)
        int checkDuerKA();
        int checkDuerPrintSpan();
    // #endif

    // #if defined(BLINKER_MIOT)
        int checkMIOTKA();
        int checkMIOTPrintSpan();
    // #endif

        void parseData(const char* data);
        // bool        _isBegin = false;
        int         _aliType = ALI_NONE;
        int         _duerType = DUER_NONE;
        int         _miType = MIOT_NONE;

    protected :
        b_broker_t  _use_broker = blinker_b;
        char        _messageId[20];
        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
        bool        _isWiFiInit = false;
        bool        isMQTTinit = false;
        bool        _at_init = false;
        uint32_t    latestTime;
        bool*       isHandle;
        bool        isAlive = false;
        bool        _needCheckShare = false;
        uint8_t     _sharerCount = 0;
        uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        char*       _authKey = NULL;

        Stream*     stream;
        char*       streamData;//[BLINKER_MAX_READ_SIZE];
        bool        isSeriaFresh;
        bool        isSerialConnect;
        bool        isHWS = false;

        uint32_t    kaTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;
        uint8_t     respDuerTimes = 0;
        uint32_t    respDuerTime = 0;
        uint8_t     respMIOTTimes = 0;
        uint32_t    respMIOTTime = 0;

        uint32_t    _print_time = 0;
        uint8_t     _print_times = 0;
        uint32_t    printTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        uint32_t    duerKaTime = 0;
        bool        isDuerAlive = false;
        bool        isDuerAvail = false;
        uint32_t    miKaTime = 0;
        bool        isMIOTAlive = false;
        bool        isMIOTAvail = false;
        uint32_t    linkTime = 0;

        // const char* _authKey;

        b_config_t  _configType = COMM;
        b_configStatus_t _configStatus = AUTO_INIT;
        uint32_t    _connectTime = 0;
        uint8_t     _connectTimes = 0;

        uint8_t     reconnect_time = 0;
        uint32_t    _reRegister_time = 0;
        uint8_t     _reRegister_times = 0;

        char        message_id[24];
        bool        is_rrpc = false;        

        uint8_t     data_dataCount = 0;
        uint8_t     data_timeSlotDataCount = 0;
        uint32_t    time_timeSlotData = 0;
        uint8_t     _aCount = 0;
        uint8_t     _bridgeCount = 0;

        b_device_staus_t _status = BLINKER_WLAN_CONNECTING;

        // blinker_at_status_t _at_status = BL_BEGIN;
        // class BlinkerSlaverAT * _slaverAT;
        // uint8_t                 pinDataNum = 0;
        // class PinData *         _pinData[BLINKER_MAX_PIN_NUM];
};

#if defined(ESP8266)
    #include <SoftwareSerial.h>

    // BearSSL::WiFiClientSecure   client_mqtt;
    // WiFiClientSecure            client_mqtt;
#elif defined(ESP32)
    #include "HardwareSerial.h"

    // WiFiClientSecure            client_s;
#endif

// WiFiClient               client;
Adafruit_MQTT_Client*    mqtt_MQTT_AT;
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe* iotSub_MQTT_AT;

#define WS_SERVERPORT       81
WebSocketsServer webSocket_MQTT_AT = WebSocketsServer(WS_SERVERPORT);

char*    msgBuf_MQTT_AT;
bool     isFresh_MQTT_AT = false;
bool     isConnect_MQTT_AT = false;
bool     isAvail_MQTT_AT = false;
bool     isApCfg = false;
uint8_t  ws_num_MQTT_AT = 0;
uint8_t  dataFrom_MQTT_AT = BLINKER_MSG_FROM_MQTT;
// bool     isATAvaill = false;

void webSocketEvent_MQTT_AT(uint8_t num, WStype_t type, \
                    uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Disconnected! "), num);

            isConnect_MQTT_AT = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket_MQTT_AT.remoteIP(num);
                
                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("num: "), num, \
                                BLINKER_F(", Connected from: "), ip, 
                                BLINKER_F(", url: "), (char *)payload);
                
                // send message to client
                webSocket_MQTT_AT.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num_MQTT_AT = num;

                isConnect_MQTT_AT = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("num: "), num, \
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

BlinkerWiFiESPAT WiFiESPAT;

BlinkerWiFiESPAT::BlinkerWiFiESPAT()
{
    stream = NULL;
    isSerialConnect = false;
    isHandle = &isConnect_MQTT_AT;
}

// void BlinkerWiFiESPAT::atBegin()
// {
//     while (_at_status == BL_BEGIN)
//     {
//         if (serialAvailable())
//         {
//             _slaverAT = new BlinkerSlaverAT();

//             _slaverAT->update(serialLastRead());

//             BLINKER_LOG_ALL(BLINKER_F("state: "), _slaverAT->state());
//             BLINKER_LOG_ALL(BLINKER_F("cmd: "), _slaverAT->cmd());
//             BLINKER_LOG_ALL(BLINKER_F("paramNum: "), _slaverAT->paramNum());

//             if (_slaverAT->state())
//             {
//                 parseATdata();

//                 free(_slaverAT);

//                 return;
//             }

//             free(_slaverAT);
//         }
//     }
// }

// void BlinkerWiFiESPAT::parseATdata()
// {
//     String reqData;

//     if (_slaverAT->cmd() == BLINKER_CMD_AT) {
//         serialPrint(BLINKER_CMD_OK);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_RST) {
//         serialPrint(BLINKER_CMD_OK);
//         ::delay(100);
//         ESP.restart();
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_GMR) {
//         // reqData = "+" + STRING_format(BLINKER_CMD_GMR) +
//         //         "=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>" +
//         //         "[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]";
//         serialPrint(BLINKER_ESP_AT_VERSION);
//         serialPrint(BLINKER_VERSION);
//         serialPrint(BLINKER_CMD_OK);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_UART_CUR) {
//         blinker_at_state_t at_state = _slaverAT->state();

//         BLINKER_LOG(at_state);

//         switch (at_state)
//         {
//             case AT_NONE:
//                 // BProto::serialPrint();
//                 break;
//             case AT_TEST:
//                 reqData = BLINKER_CMD_AT;// +
//                 reqData += BLINKER_F("+");
//                 reqData += BLINKER_CMD_UART_CUR;// +
//                 reqData += BLINKER_F("=<baudrate>,<databits>,<stopbits>,<parity>");
//                 serialPrint(reqData);
//                 serialPrint(BLINKER_CMD_OK);
//                 break;
//             case AT_QUERY:
//                 reqData = BLINKER_F("+");
//                 reqData += BLINKER_CMD_UART_CUR;
//                 reqData += BLINKER_F(":");
//                 reqData += STRING_format(serialSet >> 8 & 0x00FFFFFF);
//                 reqData += BLINKER_F(",");
//                 reqData += STRING_format(serialSet >> 4 & 0x0F);
//                 reqData += BLINKER_F(",");
//                 reqData += STRING_format(serialSet >> 2 & 0x03);
//                 reqData += BLINKER_F(",");
//                 reqData += STRING_format(serialSet      & 0x03);
//                 serialPrint(reqData);
//                 serialPrint(BLINKER_CMD_OK);
//                 break;
//             case AT_SETTING:
//                 BLINKER_LOG_ALL(BLINKER_F("SER_BAUD: "), _slaverAT->getParam(SER_BAUD));
//                 BLINKER_LOG_ALL(BLINKER_F("SER_DBIT: "), _slaverAT->getParam(SER_DBIT));
//                 BLINKER_LOG_ALL(BLINKER_F("SER_SBIT: "), _slaverAT->getParam(SER_SBIT));
//                 BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), _slaverAT->getParam(SER_PRIT));
//                 if (BLINKER_UART_PARAM_NUM != _slaverAT->paramNum()) return;

//                 serialSet = (_slaverAT->getParam(SER_BAUD)).toInt() << 8 |
//                             (_slaverAT->getParam(SER_DBIT)).toInt() << 4 |
//                             (_slaverAT->getParam(SER_SBIT)).toInt() << 2 |
//                             (_slaverAT->getParam(SER_PRIT)).toInt();

//                 ss_cfg = serConfig();

//                 BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), serialSet);

//                 serialPrint(BLINKER_CMD_OK);

//                 // if (isHWS) {
//                     Serial.begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
//                 // }
//                 // else {
//                 //     SSerialBLE->begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
//                 // }
//                 break;
//             case AT_ACTION:
//                 // BProto::serialPrint();
//                 break;
//             default :
//                 break;
//         }
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_UART_DEF) {
//         blinker_at_state_t at_state = _slaverAT->state();

//         BLINKER_LOG(at_state);

//         switch (at_state)
//         {
//             case AT_NONE:
//                 // BProto::serialPrint();
//                 break;
//             case AT_TEST:
//                 reqData = BLINKER_CMD_AT;
//                 reqData += BLINKER_F("+");
//                 reqData += BLINKER_CMD_UART_DEF;
//                 reqData += BLINKER_F("=<baudrate>,<databits>,<stopbits>,<parity>");
//                 serialPrint(reqData);
//                 serialPrint(BLINKER_CMD_OK);
//                 break;
//             case AT_QUERY:
//                 reqData = BLINKER_F("+");
//                 reqData += BLINKER_CMD_UART_DEF;
//                 reqData += BLINKER_F(":");
//                 reqData += STRING_format(serialSet >> 8 & 0x00FFFFFF);
//                 reqData += BLINKER_F(",");
//                 reqData += STRING_format(serialSet >> 4 & 0x0F);
//                 reqData += BLINKER_F(",");
//                 reqData += STRING_format(serialSet >> 2 & 0x03);
//                 reqData += BLINKER_F(",");
//                 reqData += STRING_format(serialSet      & 0x03);
//                 serialPrint(reqData);
//                 serialPrint(BLINKER_CMD_OK);
//                 break;
//             case AT_SETTING:
//                 BLINKER_LOG_ALL(BLINKER_F("SER_BAUD: "), _slaverAT->getParam(SER_BAUD));
//                 BLINKER_LOG_ALL(BLINKER_F("SER_DBIT: "), _slaverAT->getParam(SER_DBIT));
//                 BLINKER_LOG_ALL(BLINKER_F("SER_SBIT: "), _slaverAT->getParam(SER_SBIT));
//                 BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), _slaverAT->getParam(SER_PRIT));

//                 if (BLINKER_UART_PARAM_NUM != _slaverAT->paramNum()) return;

//                 serialSet = (_slaverAT->getParam(SER_BAUD)).toInt() << 8 |
//                             (_slaverAT->getParam(SER_DBIT)).toInt() << 4 |
//                             (_slaverAT->getParam(SER_SBIT)).toInt() << 2 |
//                             (_slaverAT->getParam(SER_PRIT)).toInt();

//                 ss_cfg = serConfig();

//                 BLINKER_LOG_ALL(BLINKER_F("SER_PRIT: "), serialSet);

//                 serialPrint(BLINKER_CMD_OK);

//                 // if (isHWS) {
//                     Serial.begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
//                 // }
//                 // else {
//                 //     SSerialBLE->begin(serialSet >> 8 & 0x00FFFFFF, ss_cfg);
//                 // }

//                 EEPROM.begin(BLINKER_EEP_SIZE);
//                 EEPROM.put(BLINKER_EEP_ADDR_SERIALCFG, serialSet);
//                 EEPROM.commit();
//                 EEPROM.end();
//                 break;
//             case AT_ACTION:
//                 // BProto::serialPrint();
//                 break;
//             default :
//                 break;
//         }
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_RAM && _slaverAT->state() == AT_QUERY) {
//         reqData = BLINKER_F("+");
//         reqData += BLINKER_F(BLINKER_CMD_RAM);
//         reqData += BLINKER_F(":");
//         reqData += STRING_format(BLINKER_FreeHeap());

//         serialPrint(reqData);
//         serialPrint(BLINKER_CMD_OK);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_ADC && _slaverAT->state() == AT_QUERY) {
//         reqData = BLINKER_F("+");
//         reqData += BLINKER_CMD_ADC;
//         reqData += BLINKER_F(":");
//         reqData += STRING_format(analogRead(A0));

//         serialPrint(reqData);
//         serialPrint(BLINKER_CMD_OK);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_IOSETCFG && _slaverAT->state() == AT_SETTING) {
//         BLINKER_LOG_ALL(BLINKER_F("PIN_SET: "), _slaverAT->getParam(PIN_SET));
//         BLINKER_LOG_ALL(BLINKER_F("PIN_MODE: "), _slaverAT->getParam(PIN_MODE));
//         BLINKER_LOG_ALL(BLINKER_F("PIN_PULLSTATE: "), _slaverAT->getParam(PIN_PULLSTATE));

//         if (BLINKER_IOSETCFG_PARAM_NUM != _slaverAT->paramNum()) return;

//         uint8_t set_pin = (_slaverAT->getParam(PIN_SET)).toInt();
//         uint8_t set_mode = (_slaverAT->getParam(PIN_MODE)).toInt();
//         uint8_t set_pull = (_slaverAT->getParam(PIN_PULLSTATE)).toInt();

//         if (set_pin >= BLINKER_MAX_PIN_NUM ||
//             set_mode > BLINKER_IO_OUTPUT_NUM ||
//             set_pull > 2)
//         {
//             return;
//         }

//         if (pinDataNum == 0) {
//             _pinData[pinDataNum] = new PinData(set_pin, set_mode, set_pull);
//             pinDataNum++;
//         }
//         else {
//             bool _isSet = false;
//             for (uint8_t _num = 0; _num < pinDataNum; _num++)
//             {
//                 if (_pinData[_num]->checkPin(set_pin))
//                 {
//                     _isSet = true;
//                     _pinData[_num]->fresh(set_mode, set_pull);
//                 }
//             }
//             if (!_isSet) {
//                 _pinData[pinDataNum] = new PinData(set_pin, set_mode, set_pull);
//                 pinDataNum++;
//             }
//         }

//         serialPrint(BLINKER_CMD_OK);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_IOGETCFG && _slaverAT->state() == AT_SETTING) {
//         BLINKER_LOG_ALL(BLINKER_F("PIN_SET: "), _slaverAT->getParam(PIN_SET));

//         if (BLINKER_IOGETCFG_PARAM_NUM != _slaverAT->paramNum()) return;

//         uint8_t set_pin = (_slaverAT->getParam(PIN_SET)).toInt();

//         if (set_pin >= BLINKER_MAX_PIN_NUM) return;

//         bool _isGet = false;
//         for (uint8_t _num = 0; _num < pinDataNum; _num++)
//         {
//             if (_pinData[_num]->checkPin(set_pin))
//             {
//                 _isGet = true;
//                 reqData = BLINKER_F("+");
//                 reqData += BLINKER_CMD_IOGETCFG;
//                 reqData += BLINKER_F(":");
//                 reqData += _pinData[_num]->data();
//                 serialPrint(reqData);
//             }
//         }
//         if (!_isGet) {
//             reqData = BLINKER_F("+");
//             reqData += BLINKER_CMD_IOGETCFG;
//             reqData += BLINKER_F(":");
//             reqData += _slaverAT->getParam(PIN_SET);
//             reqData += BLINKER_F(",2,0");
//             serialPrint(reqData);
//         }

//         serialPrint(BLINKER_CMD_OK);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_GPIOWRITE && _slaverAT->state() == AT_SETTING) {
//         BLINKER_LOG_ALL(BLINKER_F("IO_PIN: "), _slaverAT->getParam(IO_PIN));
//         BLINKER_LOG_ALL(BLINKER_F("IO_LVL: "),  _slaverAT->getParam(IO_LVL));

//         if (BLINKER_GPIOWRITE_PARAM_NUM != _slaverAT->paramNum()) return;

//         uint8_t set_pin = (_slaverAT->getParam(IO_PIN)).toInt();
//         uint8_t set_lvl = (_slaverAT->getParam(IO_LVL)).toInt();

//         if (set_pin >= BLINKER_MAX_PIN_NUM) return;

//         BLINKER_LOG_ALL(set_pin, ", ", set_lvl, ", ", pinDataNum);

//         // bool _isSet = false;
//         for (uint8_t _num = 0; _num < pinDataNum; _num++)
//         {
//             if (_pinData[_num]->checkPin(set_pin))
//             {
//                 if (_pinData[_num]->getMode() == BLINKER_IO_OUTPUT_NUM)
//                 {
//                     if (set_lvl <= 1) {
//                         digitalWrite(set_pin, set_lvl ? HIGH : LOW);
//                         serialPrint(BLINKER_CMD_OK);
//                         return;
//                     }
//                 }
//             }
//         }

//         serialPrint(BLINKER_CMD_ERROR);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_GPIOWREAD && _slaverAT->state() == AT_SETTING) {
//         BLINKER_LOG_ALL(BLINKER_F("IO_PIN: "), _slaverAT->getParam(IO_PIN));

//         if (BLINKER_GPIOREAD_PARAM_NUM != _slaverAT->paramNum()) return;

//         uint8_t set_pin = (_slaverAT->getParam(IO_PIN)).toInt();

//         if (set_pin >= BLINKER_MAX_PIN_NUM)
//         {
//             serialPrint(BLINKER_CMD_ERROR);
//             return;
//         }

//         // bool _isSet = false;
//         for (uint8_t _num = 0; _num < pinDataNum; _num++)
//         {
//             if (_pinData[_num]->checkPin(set_pin))
//             {
//                 // if (_pinData[_num]->getMode() == BLINKER_IO_INPUT_NUM)
//                 // {
//                 //     if (set_lvl <= 1) {
//                         reqData = BLINKER_F("+");
//                         reqData += BLINKER_CMD_GPIOWREAD;
//                         reqData += BLINKER_F(":");
//                         reqData += STRING_format(set_pin);
//                         reqData += BLINKER_F(",");
//                         reqData += STRING_format(_pinData[_num]->getMode());
//                         reqData += BLINKER_F(",");
//                         reqData += STRING_format(digitalRead(set_pin));
//                         serialPrint(reqData);
//                         serialPrint(BLINKER_CMD_OK);
//                         return;
//                 //     }
//                 // }
//             }
//         }
//         reqData = BLINKER_F("+");
//         reqData += BLINKER_CMD_GPIOWREAD;
//         reqData += BLINKER_F(":");
//         reqData += STRING_format(set_pin);
//         reqData += BLINKER_F(",3,");
//         reqData += STRING_format(digitalRead(set_pin));
//         serialPrint(reqData);
//         serialPrint(BLINKER_CMD_OK);
//         // BProto::serialPrint(BLINKER_CMD_ERROR);
//     }
//     else if (_slaverAT->cmd() == BLINKER_CMD_BLINKER_MQTT) {
//         // // BProto::serialPrint(BLINKER_CMD_OK);

//         // BLINKER_LOG(BLINKER_CMD_BLINKER_MQTT);

//         // blinker_at_state_t at_state = _slaverAT->state();

//         // BLINKER_LOG(at_state);

//         // switch (at_state)
//         // {
//         //     case AT_NONE:
//         //         // BProto::serialPrint();
//         //         break;
//         //     case AT_TEST:
//         //         reqData = BLINKER_CMD_AT;
//         //         reqData += BLINKER_F("+");
//         //         reqData += BLINKER_CMD_BLINKER_MQTT;
//         //         reqData += BLINKER_F("=<MQTT_CONFIG_MODE>,<MQTT_AUTH_KEY>");
//         //         reqData += BLINKER_F("[,<MQTT_WIFI_SSID>,<MQTT_WIFI_PSWD>]");
//         //         BProto::serialPrint(reqData);
//         //         BProto::serialPrint(BLINKER_CMD_OK);
//         //         break;
//         //     case AT_QUERY:
//         //         reqData = BLINKER_F("+");
//         //         reqData += BLINKER_CMD_BLINKER_MQTT;
//         //         reqData += BLINKER_F(":");
//         //         reqData += STRING_format(_wlanMode);
//         //         reqData += BLINKER_F(",");
//         //         reqData += STRING_format(BProto::authKey());
//         //         reqData += BLINKER_F(",");
//         //         reqData += WiFi.SSID();
//         //         reqData += BLINKER_F(",");
//         //         reqData += WiFi.psk();
//         //         BProto::serialPrint(reqData);
//         //         BProto::serialPrint(BLINKER_CMD_OK);
//         //         break;
//         //     case AT_SETTING:
//         //         BLINKER_LOG_ALL(BLINKER_F("MQTT_CONFIG_MODE: "), _slaverAT->getParam(MQTT_CONFIG_MODE));
//         //         BLINKER_LOG_ALL(BLINKER_F("MQTT_AUTH_KEY: "),  _slaverAT->getParam(MQTT_AUTH_KEY));
//         //         BLINKER_LOG_ALL(BLINKER_F("MQTT_WIFI_SSID: "), _slaverAT->getParam(MQTT_WIFI_SSID));
//         //         BLINKER_LOG_ALL(BLINKER_F("MQTT_WIFI_PSWD: "), _slaverAT->getParam(MQTT_WIFI_PSWD));

//         //         if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_COMCONFIG_NUM)
//         //         {
//         //             BLINKER_LOG_ALL(BLINKER_F("BLINKER_CMD_COMWLAN"));

//         //             if (BLINKER_COMWLAN_PARAM_NUM != _slaverAT->paramNum()) return;

//         //             if (_status == BL_INITED)
//         //             {
//         //                 reqData = BLINKER_F("+");
//         //                 reqData += BLINKER_CMD_BLINKER_MQTT;
//         //                 reqData += BLINKER_F(":");
//         //                 reqData += BProto::deviceId();
//         //                 reqData += BLINKER_F(",");
//         //                 reqData += BProto::uuid();
//         //                 BProto::serialPrint(reqData);
//         //                 BProto::serialPrint(BLINKER_CMD_OK);
//         //                 return;
//         //             }

//         //             BProto::connectWiFi((_slaverAT->getParam(MQTT_WIFI_SSID)).c_str(),
//         //                         (_slaverAT->getParam(MQTT_WIFI_PSWD)).c_str());

//         //             BProto::begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
//         //             _status = BL_INITED;
//         //             _wlanMode = BLINKER_CMD_COMCONFIG_NUM;
//         //         }
//         //         else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_SMARTCONFIG_NUM)
//         //         {
//         //             BLINKER_LOG_ALL(BLINKER_F("BLINKER_CMD_SMARTCONFIG"));

//         //             if (BLINKER_SMCFG_PARAM_NUM != _slaverAT->paramNum()) return;

//         //             if (_status == BL_INITED)
//         //             {
//         //                 reqData = BLINKER_F("+");
//         //                 reqData += BLINKER_CMD_BLINKER_MQTT;
//         //                 reqData += BLINKER_F(":");
//         //                 reqData += BProto::deviceId();
//         //                 reqData += BLINKER_F(",");
//         //                 reqData += BProto::uuid();
//         //                 BProto::serialPrint(reqData);
//         //                 BProto::serialPrint(BLINKER_CMD_OK);
//         //                 return;
//         //             }

//         //             // if (!BProto::autoInit())
//         //             BProto::smartconfig();

//         //             BProto::begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
//         //             _status = BL_INITED;
//         //             _wlanMode = BLINKER_CMD_SMARTCONFIG_NUM;
//         //         }
//         //         else if ((_slaverAT->getParam(MQTT_CONFIG_MODE)).toInt() == BLINKER_CMD_APCONFIG_NUM)
//         //         {
//         //             BLINKER_LOG(BLINKER_F("BLINKER_CMD_APCONFIG"));

//         //             if (BLINKER_APCFG_PARAM_NUM != _slaverAT->paramNum()) return;

//         //             if (_status == BL_INITED)
//         //             {
//         //                 reqData = BLINKER_F("+");
//         //                 reqData += BLINKER_CMD_BLINKER_MQTT;
//         //                 reqData += BLINKER_F(":");
//         //                 reqData += BProto::deviceId();
//         //                 reqData += BLINKER_F(",");
//         //                 reqData += BProto::uuid();
//         //                 BProto::serialPrint(reqData);
//         //                 BProto::serialPrint(BLINKER_CMD_OK);
//         //                 return;
//         //             }

//         //             if (!BProto::autoInit())
//         //             {
//         //                 BProto::softAPinit();
//         //                 // while(WiFi.status() != WL_CONNECTED) {
//         //                 //     BProto::serverClient();

//         //                 //     ::delay(10);
//         //                 // }
//         //             }

//         //             BProto::begin((_slaverAT->getParam(MQTT_AUTH_KEY)).c_str());
//         //             _status = BL_INITED;
//         //             _wlanMode = BLINKER_CMD_APCONFIG_NUM;
//         //         }
//         //         else {
//         //             return;
//         //         }

//         //         // reqData = BLINKER_F("+");
//         //         // reqData += BLINKER_CMD_BLINKER_MQTT;
//         //         // reqData += BLINKER_F(":");
//         //         // reqData += BProto::deviceId();
//         //         // reqData += BLINKER_F(",");
//         //         // reqData += BProto::uuid();
//         //         // BProto::serialPrint(reqData);
//         //         // BProto::serialPrint(BLINKER_CMD_OK);
//         //         break;
//         //     case AT_ACTION:
//         //         // BProto::serialPrint();
//         //         break;
//         //     default :
//         //         break;
//         }
//     }
// }

int BlinkerWiFiESPAT::serialAvailable()
{
    if (stream->available())
    {
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
        
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("handleSerial: "), streamData);
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

void BlinkerWiFiESPAT::serialBegin(Stream& s, bool state)
{
    stream = &s;
    stream->setTimeout(BLINKER_STREAM_TIMEOUT);
    isHWS = state;

    serialConnect();

    serialPrint("");
    serialPrint(BLINKER_CMD_BLINKER_MQTT);
}

int BlinkerWiFiESPAT::serialTimedRead()
{
    int c;
    uint32_t _startMillis = millis();
    do {
        c = stream->read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < 1000);
    return -1; 
}

char * BlinkerWiFiESPAT::serialLastRead()
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

int BlinkerWiFiESPAT::serialPrint(const String & s1, const String & s2, bool needCheck)
{
    return serialPrint(s1 + s2, needCheck);
}

int BlinkerWiFiESPAT::serialPrint(const String & s, bool needCheck)
{
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Serial Response: "), s);
    
    if(serialConnected())
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Success..."));
        
        stream->println(s);
        return true;
    }
    else
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Faile... Disconnected"));
        
        return false;
    }
}

int BlinkerWiFiESPAT::serialConnect()
{
    isSerialConnect = true;
    return serialConnected();
}

int BlinkerWiFiESPAT::serialConnected() { return isSerialConnect; }

void BlinkerWiFiESPAT::serialDisconnect() { isSerialConnect = false; }

void BlinkerWiFiESPAT::begin(const char* auth) {
    // _authKey = auth;
    if (_authKey != NULL) {
        free(_authKey);
        _authKey = NULL;
    }
    _authKey = (char*)malloc((strlen(auth)+1)*sizeof(char));
    strcpy(_authKey, auth);
    
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("_authKey: "), auth);

    _at_init = true;
}

bool BlinkerWiFiESPAT::init()
{
    // atBegin();

    // if (_at_status == BL_BEGIN) return false;

    if (isMQTTinit) return true;

    if (deviceRegister()) {
        mDNSInit();
        isMQTTinit = true;
        return true;
    }
    else {
        isMQTTinit = false;

        return false;
    }
}

void BlinkerWiFiESPAT::reset()
{
    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Blinker reset..."));
    char config_check[3] = {0};
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, config_check);
    EEPROM.commit();
    EEPROM.end();

    ESP.restart();
}

bool BlinkerWiFiESPAT::connect()
{
    if (!checkInit()) return false;

    int8_t ret;

    webSocket_MQTT_AT.loop();

    if (mqtt_MQTT_AT->connected())
    {
        _status = BLINKER_DEV_CONNECTED;
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < BLINKER_MQTT_CONNECT_TIMESLOT && latestTime > 0)
    {
        yield();
        return false;
    }

    _status = BLINKER_DEV_CONNECTING;

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Connecting to MQTT... "));

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("reconnect_time: "), reconnect_time);

    #if defined(ESP8266)
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.setInsecure();
            ::delay(10);
        #endif
    #endif

    if ((ret = mqtt_MQTT_AT->connect()) != 0)
    {
        BLINKER_LOG(TAG_ESP_AT, mqtt_MQTT_AT->connectErrorString(ret));
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Retrying MQTT connection in "), \
                    BLINKER_MQTT_CONNECT_TIMESLOT/1000, \
                    BLINKER_F(" seconds..."));

        if (ret == 4) 
        {
            if (_reRegister_times < BLINKER_SERVER_CONNECT_LIMIT)
            {
                if (deviceRegister())
                {
                    _reRegister_times = 0;
                }
                else
                {
                    _reRegister_times++;
                    _reRegister_time = millis();
                    latestTime = millis();
                }
            }
            else
            {
                if (millis() - _reRegister_time >= 60000 * 1) _reRegister_times = 0;
            }

            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("_reRegister_times1: "), _reRegister_times);
            return false;
        }
        else
        {
            latestTime = millis();
            reconnect_time += 1;
            if (reconnect_time >= 4)
            {
                if (_reRegister_times < BLINKER_SERVER_CONNECT_LIMIT)
                {
                    if (deviceRegister())
                    {
                        _reRegister_times = 0;
                    }
                    else
                    {
                        _reRegister_times++;
                        _reRegister_time = millis();
                    }
                }
                else
                {
                    if (millis() - _reRegister_time >= 60000 * 1) _reRegister_times = 0;
                }
                
                reconnect_time = 0;

                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("_reRegister_times2: "), _reRegister_times);
            }
            return false;
        }
    }
    
    reconnect_time = 0;

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("MQTT Connected!"));
    BLINKER_LOG_FreeHeap();

    if (!isMQTTinit)
    {
        isMQTTinit = true;

        mqtt_MQTT_AT->subscribeTopic(BLINKER_SUB_TOPIC_MQTT_AT);
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT subscribe: "), BLINKER_SUB_TOPIC_MQTT_AT);
    }

    this->latestTime = millis();

    if (_at_init)
    {
        _at_init = false;

        String reqData;

        reqData = BLINKER_F("+");
        reqData += BLINKER_CMD_BLINKER_MQTT;
        reqData += BLINKER_F(":");
        reqData += deviceId();
        reqData += BLINKER_F(",");
        reqData += uuid();
        serialPrint(reqData);
        serialPrint(BLINKER_CMD_OK);
    }

    return true;
}

void BlinkerWiFiESPAT::disconnect()
{
    if (!checkInit()) return;

    if (mqtt_MQTT_AT->connected()) mqtt_MQTT_AT->disconnectServer();

    if (*isHandle) webSocket_MQTT_AT.disconnect();

    _status = BLINKER_DEV_DISCONNECTED;
}

void BlinkerWiFiESPAT::ping()
{  
    if (!checkInit()) return;

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT Ping!"));

    BLINKER_LOG_FreeHeap_ALL();

    if (!mqtt_MQTT_AT->ping())
    {
        disconnect();
    }
    else
    {
        this->latestTime = millis();
    }
}

bool BlinkerWiFiESPAT::available()
{
    if (!checkInit()) return false;

    webSocket_MQTT_AT.loop();

    checkKA();
#if defined(ESP8266)
    MDNS.update();
#endif
    
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
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("available: "), isAvail_MQTT_AT);

        isAvail_MQTT_AT = false;
        return true;
    }
    else {
        return false;
    }
}

char * BlinkerWiFiESPAT::lastRead()
{
    if (isFresh_MQTT_AT) return msgBuf_MQTT_AT;
    else return "";
}

void BlinkerWiFiESPAT::flush()
{
    if (isFresh_MQTT_AT)
    {   
        free(msgBuf_MQTT_AT);     
        isFresh_MQTT_AT = false; isAvail_MQTT_AT = false;
        isAliAvail = false; isDuerAvail = false; isMIOTAvail = false;//isBavail = false;
        
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("flush"));
        BLINKER_LOG_FreeHeap_ALL();
    }
}

void BlinkerWiFiESPAT::subscribe()
{
    if (!checkInit()) return;

    if (!isMQTTinit) return;

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt_MQTT_AT->readSubscription(10)))
    {
        if (subscription == iotSub_MQTT_AT)
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Got: "), (char *)iotSub_MQTT_AT->lastread);

            parseData((char *)iotSub_MQTT_AT->lastread);
        }
    }

    if (mqtt_MQTT_AT->check_extra())
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Got extra topic: "), (char *)mqtt_MQTT_AT->get_extra_topic());
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Got extra data: "), (char *)mqtt_MQTT_AT->get_extra_data());

        if (strncmp(BLINKER_RRPC_SUB_TOPIC_MQTT_AT, 
                    (char *)mqtt_MQTT_AT->get_extra_topic(), 
                    strlen(BLINKER_RRPC_SUB_TOPIC_MQTT_AT))
                    == 0)
        {
            // char message_id[24];
            memset(message_id, '\0', 24);

            memmove(message_id, 
                mqtt_MQTT_AT->get_extra_topic()+strlen(BLINKER_RRPC_SUB_TOPIC_MQTT_AT),
                strlen(mqtt_MQTT_AT->get_extra_topic()) - strlen(BLINKER_RRPC_SUB_TOPIC_MQTT_AT));
            
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("from server RRPC, message_id: "), message_id);

            parseData((char *)mqtt_MQTT_AT->get_extra_data());

            is_rrpc = true;
        }
    }
}

int BlinkerWiFiESPAT::print(char * data, bool needCheck)
{
    if (!checkInit()) return false;
    
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

        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("WS response: "));
        BLINKER_LOG_ALL(TAG_ESP_AT, data);
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Success..."));

        strcat(data, BLINKER_CMD_NEWLINE);

        webSocket_MQTT_AT.sendTXT(ws_num_MQTT_AT, data);

        return true;
    }
    else
    {
        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';        

        char data_add[20] = "{\"data\":";
        for(uint8_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }
        
        strcat(data, ",\"fromDevice\":\"");
        strcat(data, DEVICE_NAME_MQTT_AT);
        strcat(data, "\",\"toDevice\":\"");
        
        if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        {
            strcat(data, _sharers[_sharerFrom]->uuid());
        }
        else
        {
            strcat(data, UUID_MQTT_AT);
        }

        strcat(data, "\",\"deviceType\":\"OwnApp\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        if (!isJson(STRING_format(data))) return false;

        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT Publish..."));
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

                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("_print_times: "), _print_times);
            }

            if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, data))
            {
                BLINKER_LOG_ALL(TAG_ESP_AT, data);
                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...Failed"));
                BLINKER_LOG_FreeHeap_ALL();

                if (!_alive)
                {
                    isAlive = false;
                }
                return false;
            }
            else
            {
                BLINKER_LOG_ALL(TAG_ESP_AT, data);
                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...OK!"));
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

int BlinkerWiFiESPAT::mqttPrint(const String & data)
{
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("mqttPrint data: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& print_data = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject print_data = jsonBuffer.as<JsonObject>();


    // if (!print_data.success())
    if (error)
    {
        BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("Print data not a Json data"));
        return false;
    }

    if (print_data.containsKey("toDeviceAT"))
    {

        String _dType = print_data["toDeviceAT"];

        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("mqttPrint _dType: "), _dType);

        print_data.remove("toDeviceAT");
        String _data_;
        // print_data.printTo(_data_);
        serializeJson(print_data, _data_);

        // if (_dType == "AliGenie")
        // {
        //     return aliPrint(_data_);
        // }
        // else if (_dType == "DuerOS")
        // {
        //     return duerPrint(_data_);
        // }
        // else
        // {
        //     char data_print[512];
        //     strcpy(data_print, _data_.c_str());
        //     return print(data_print);
        // }        
    }
    else
    {
        char data_print[512];
        strcpy(data_print, data.c_str());
        return print(data_print);
    }

    return false;
}

int BlinkerWiFiESPAT::autoPrint(unsigned long id)
{
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("autoTrigged id: "), id);

    String payload = BLINKER_F("{\"data\":{\"set\":{");
    payload += BLINKER_F("\"auto\":{\"trig\":true,");
    payload += BLINKER_F("\"id\":");
    payload += String(id);
    payload += BLINKER_F("}}}");
    payload += BLINKER_F(",\"fromDevice\":\"");
    payload += STRING_format(DEVICE_NAME_MQTT_AT);
    payload += BLINKER_F("\",\"deviceType\":\"Auto\"");
    payload += BLINKER_F(",\"toDevice\":\"serverClient\"}");
        // "\",\"deviceType\":\"" + "type" + "\"}";

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("autoPrint..."));

    if (mqtt_MQTT_AT->connected())
    {
        if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || \
            linkTime == 0)
        {
            // linkTime = millis();

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt_MQTT_AT, BLINKER_PUB_TOPIC_MQTT);

            // if (! iotPub.publish(payload.c_str())) {

            if (! mqtt_MQTT_AT->publish(BLINKER_PUB_TOPIC_MQTT_AT, payload.c_str()))
            {
                BLINKER_LOG_ALL(TAG_ESP_AT, payload);
                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...Failed"));

                return false;
            }
            else
            {
                BLINKER_LOG_ALL(TAG_ESP_AT, payload);
                BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...OK!"));

                linkTime = millis();

                this->latestTime = millis();

                return true;
            }
        }
        else
        {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT "), linkTime);

            return false;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiESPAT::isJson(const String & data)
{
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("isJson: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(STRING_format(data));
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, STRING_format(data));
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG("Print data is not Json! ", data);
        return false;
    }

    return true;
}

bool BlinkerWiFiESPAT::deviceRegister()
{
    if (WiFi.status() != WL_CONNECTED) return false;

    _status = BLINKER_DEV_REGISTER;

    String urls = _authKey;
    if (_aliType == ALI_LIGHT) {
        urls += BLINKER_F("&aliType=light"); }
    else if (_aliType == ALI_OUTLET) {
        urls += BLINKER_F("&aliType=outlet"); }
    else if (_aliType == ALI_MULTI_OUTLET) {
        urls += BLINKER_F("&aliType=multi_outlet"); }
    else if (_aliType == ALI_SENSOR) {
        urls += BLINKER_F("&aliType=sensor"); }
    else if (_aliType == ALI_FAN) {
        urls += BLINKER_F("&aliType=fan"); }
    else if (_aliType == ALI_AIRCONDITION) {
        urls += BLINKER_F("&aliType=aircondition"); }

    if (_duerType == DUER_LIGHT) {
        urls += BLINKER_F("&duerType=LIGHT"); }
    else if (_duerType == DUER_OUTLET) {
        urls += BLINKER_F("&duerType=SOCKET"); }
    else if (_duerType == DUER_MULTI_OUTLET) {
        urls += BLINKER_F("&duerType=MULTI_SOCKET"); }
    else if (_duerType == DUER_SENSOR) {
        urls += BLINKER_F("&duerType=AIR_MONITOR"); }
    else if (_duerType == DUER_FAN) {
        urls += BLINKER_F("&duerType=FAN"); }
    else if (_duerType == DUER_AIRCONDITION) {
        urls += BLINKER_F("&duerType=AIR_CONDITION"); }

    if (_miType == MIOT_LIGHT) {
        urls += BLINKER_F("&miType=light"); }
    else if (_miType == MIOT_OUTLET) {
        urls += BLINKER_F("&miType=outlet"); }
    else if (_miType == MIOT_MULTI_OUTLET) {
        urls += BLINKER_F("&miType=multi_outlet"); }
    else if (_miType == MIOT_SENSOR) {
        urls += BLINKER_F("&miType=sensor"); }
    else if (_miType == MIOT_FAN) {
        urls += BLINKER_F("&miType=fan"); }
    else if (_miType == MIOT_AIRCONDITION) {
        urls += BLINKER_F("&miType=aircondition"); }

    urls += BLINKER_F("&version=");
    urls += BLINKER_OTA_VERSION_CODE;
    #ifndef BLINKER_WITHOUT_SSL
    urls += BLINKER_F("&protocol=mqtts");
    #else
    urls += BLINKER_F("&protocol=mqtt");
    #endif

    String payload = httpToServer(BLINKER_CMD_WIFI_AUTH_NUMBER, urls);

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("reply was:"));
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("=============================="));
    BLINKER_LOG_ALL(TAG_ESP_AT, payload);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("=============================="));

    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || error ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("Maybe you have put in the wrong AuthKey!"));
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("Or maybe your request is too frequently!"));
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("Or maybe your network is disconnected!"));

            return false;
    }

    String _userID = root[BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_UUID];
    String _host = root["host"];
    uint32_t _port = root["port"];
    uint8_t _num = _host.indexOf("://");
    BLINKER_LOG_ALL(TAG_ESP_AT, "_num: ", _num);
    if (_num > 0) _num += 3;
    _host = _host.substring(_num, _host.length());
    
    if (mqtt_MQTT_AT)
    {
        free(mqtt_MQTT_AT);
        free(iotSub_MQTT_AT);
    }

    if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        // memcpy(DEVICE_NAME_MQTT, _userID.c_str(), 12);
        if(!isMQTTinit) DEVICE_NAME_MQTT_AT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(DEVICE_NAME_MQTT_AT, _userID.c_str());
        if(!isMQTTinit) MQTT_ID_MQTT_AT = (char*)malloc((_userID.length()+1)*sizeof(char));
        strcpy(MQTT_ID_MQTT_AT, _userID.c_str());
        if(!isMQTTinit) MQTT_NAME_MQTT_AT = (char*)malloc((_userName.length()+1)*sizeof(char));
        strcpy(MQTT_NAME_MQTT_AT, _userName.c_str());
        if(!isMQTTinit) MQTT_KEY_MQTT_AT = (char*)malloc((_key.length()+1)*sizeof(char));
        strcpy(MQTT_KEY_MQTT_AT, _key.c_str());
        if(!isMQTTinit) MQTT_PRODUCTINFO_MQTT_AT = (char*)malloc((_productInfo.length()+1)*sizeof(char));
        strcpy(MQTT_PRODUCTINFO_MQTT_AT, _productInfo.c_str());
        if(!isMQTTinit) MQTT_HOST_MQTT_AT = (char*)malloc((_host.length()+1)*sizeof(char));
        strcpy(MQTT_HOST_MQTT_AT, _host.c_str());
        MQTT_PORT_MQTT_AT = _port;

        _use_broker = blinker_b;
    }
    if(!isMQTTinit) UUID_MQTT_AT = (char*)malloc((_uuid.length()+1)*sizeof(char));
    strcpy(UUID_MQTT_AT, _uuid.c_str());

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("===================="));
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("DEVICE_NAME_MQTT_AT: "), DEVICE_NAME_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT_PRODUCTINFO_MQTT_AT: "), MQTT_PRODUCTINFO_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT_ID_MQTT_AT: "), MQTT_ID_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT_NAME_MQTT_AT: "), MQTT_NAME_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT_KEY_MQTT_AT: "), MQTT_KEY_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT_BROKER: "), _broker);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("HOST: "), MQTT_HOST_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("PORT: "), MQTT_PORT_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("HOST: "), _host);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("PORT: "), _port);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("UUID_MQTT_AT: "), UUID_MQTT_AT);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("===================="));

    if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        String PUB_TOPIC_STR = BLINKER_F("/device");
        // PUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        PUB_TOPIC_STR += BLINKER_F("/");
        PUB_TOPIC_STR += MQTT_ID_MQTT_AT;
        PUB_TOPIC_STR += BLINKER_F("/s");

        if(!isMQTTinit) BLINKER_PUB_TOPIC_MQTT_AT = (char*)malloc((PUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_PUB_TOPIC_MQTT, PUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_PUB_TOPIC_MQTT_AT, PUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("BLINKER_PUB_TOPIC_MQTT: "), BLINKER_PUB_TOPIC_MQTT_AT);

        String SUB_TOPIC_STR = BLINKER_F("/device");
        // SUB_TOPIC_STR += MQTT_PRODUCTINFO_MQTT;
        SUB_TOPIC_STR += BLINKER_F("/");
        SUB_TOPIC_STR += MQTT_ID_MQTT_AT;
        SUB_TOPIC_STR += BLINKER_F("/r");

        if(!isMQTTinit) BLINKER_SUB_TOPIC_MQTT_AT = (char*)malloc((SUB_TOPIC_STR.length() + 1)*sizeof(char));
        // memcpy(BLINKER_SUB_TOPIC_MQTT, SUB_TOPIC_STR.c_str(), str_len);
        strcpy(BLINKER_SUB_TOPIC_MQTT_AT, SUB_TOPIC_STR.c_str());

        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("BLINKER_SUB_TOPIC_MQTT: "), BLINKER_SUB_TOPIC_MQTT_AT);
    }

    if (_broker == BLINKER_MQTT_BORKER_BLINKER) {
        #if defined(ESP8266)
            mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client_mqtt, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
        #elif defined(ESP32)
            mqtt_MQTT_AT = new Adafruit_MQTT_Client(&client_s, MQTT_HOST_MQTT_AT, MQTT_PORT_MQTT_AT, MQTT_ID_MQTT_AT, MQTT_NAME_MQTT_AT, MQTT_KEY_MQTT_AT);
        #endif
    }

    iotSub_MQTT_AT = new Adafruit_MQTT_Subscribe(mqtt_MQTT_AT, BLINKER_SUB_TOPIC_MQTT_AT);
    
    mqtt_MQTT_AT->subscribe(iotSub_MQTT_AT);

    #if defined(ESP8266)
        #ifndef BLINKER_WITHOUT_SSL
            client_mqtt.setInsecure();
        #endif
    #elif defined(ESP32)
        client_s.setInsecure();
    #endif

    freshSharers();

    BLINKER_LOG_FreeHeap();

    return true;
}

void BlinkerWiFiESPAT::freshSharers()
{
    String  data = BLINKER_F("/share/device?");
            data += BLINKER_F("deviceName=");
            data += STRING_format(DEVICE_NAME_MQTT_AT);
            data += BLINKER_F("&key=");
            data += STRING_format(_authKey);

    String payload = httpToServer(BLINKER_CMD_FRESH_SHARERS_NUMBER, data);

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("sharers data: "), payload);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(data);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success()) return;
    if (error) return;

    String user_name = "";

    if (_sharerCount)
    {
        for (uint8_t num = _sharerCount; num > 0; num--)
        {
            delete _sharers[num - 1];
        }
    }

    _sharerCount = 0;

    for (uint8_t num = 0; num < BLINKER_MQTT_MAX_SHARERS_NUM; num++)
    {
        user_name = root["users"][num].as<String>();

        if (user_name.length() >= BLINKER_MQTT_USER_UUID_SIZE)
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("sharer uuid: "), user_name, BLINKER_F(", length: "), user_name.length());

            _sharerCount++;

            _sharers[num] = new BlinkerSharer(user_name);
        }
        else
        {
            break;
        }
    }
}

void BlinkerWiFiESPAT::mDNSInit()
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

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("mDNS responder started"));

    MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME_MQTT_AT));
    MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "mac", macDeviceName());

    webSocket_MQTT_AT.begin();
    webSocket_MQTT_AT.onEvent(webSocketEvent_MQTT_AT);
    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("webSocket_MQTT_AT server started"));
    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ws://"), DEVICE_NAME_MQTT_AT, BLINKER_F(".local:"), WS_SERVERPORT);

    isApCfg = false;
}

bool BlinkerWiFiESPAT::checkWlanInit()
{
    char ok[2 + 1] = "OK";

    if (!_isWiFiInit)
    {
        switch (_configType)
        {
            case COMM :
                _connectTime = millis();
                
                if (WiFi.status() != WL_CONNECTED) {
                    ::delay(500);

                    if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                        // _connectTime = millis();
                        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
                        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Retring WiFi connect again!"));
                        return false;
                    }

                    return false;
                }
                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi Connected."));
                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("IP Address: "));
                BLINKER_LOG(TAG_ESP_AT, WiFi.localIP());
                _isWiFiInit = true;
                _connectTime = 0;

                // begin();

                _status = BLINKER_DEV_CONNECTED;

                return false;
            // #if defined(BLINKER_ESP_SMARTCONFIG)
            case BLINKER_SMART_CONFIG :
                switch (_configStatus)
                {
                    case AUTO_INIT :
                        if (WiFi.status() != WL_CONNECTED) {
                            ::delay(500);
                            return false;
                        }
                        else {
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("IP Address: "));
                            BLINKER_LOG(TAG_ESP_AT, WiFi.localIP());

                            _isWiFiInit = true;
                            _connectTime = 0;
                            // _isWiFiInit = true;

                            // begin();
                            _configStatus = AUTO_DONE;

                            _status = BLINKER_DEV_CONNECTED;

                            return false;
                        }
                    case SMART_BEGIN :
                        if (WiFi.smartConfigDone())
                        {
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("SmartConfig received."));
                            _connectTime = millis();

                            #if defined(ESP8266)
                                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("SSID: "), WiFi.SSID(), BLINKER_F(" PSWD: "), WiFi.psk());
                            #endif

                            _configStatus = SMART_DONE;

                            _status = BLINKER_DEV_CONNECTING;
                        }
                        else return false;
                    case SMART_DONE :
                        if (WiFi.status() != WL_CONNECTED)
                        {
                            if (millis() - _connectTime > 15000)
                            {
                                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("SmartConfig timeout."));
                                WiFi.stopSmartConfig();
                                _configStatus = SMART_TIMEOUT;
                            }
                            return false;
                        }
                        else if (WiFi.status() == WL_CONNECTED)
                        {
                            // WiFi.stopSmartConfig();

                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("IP Address: "));
                            BLINKER_LOG(TAG_ESP_AT, WiFi.localIP());
                            _isWiFiInit = true;
                            _connectTime = 0;                            

                            EEPROM.begin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
                            EEPROM.commit();
                            EEPROM.end();

                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Save wlan config"));

                            // begin();

                            _status = BLINKER_DEV_CONNECTED;
                            
                            return false;
                        }
                        // return false;
                    case SMART_TIMEOUT :
                        WiFi.beginSmartConfig();
                        _configStatus = SMART_BEGIN;

                        _status = BLINKER_WLAN_SMARTCONFIG_BEGIN;

                        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Waiting for SmartConfig."));
                        return false;
                    default :
                        yield();
                        return false;
                }
            // #elif defined(BLINKER_APCONFIG)
            case BLINKER_AP_CONFIG :
                switch (_configStatus)
                {
                    case AUTO_INIT :
                        if (WiFi.status() != WL_CONNECTED) {
                            ::delay(500);
                            return false;
                        }
                        else {
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("IP Address: "));
                            BLINKER_LOG(TAG_ESP_AT, WiFi.localIP());

                            _isWiFiInit = true;
                            _connectTime = 0;
                            // _isWiFiInit = true;

                            // begin();
                            _configStatus = AUTO_DONE;

                            _status = BLINKER_WLAN_CONNECTED;

                            return false;
                        }
                    case APCFG_BEGIN :
                        checkAPCFG();
                        return false;
                    case APCFG_DONE :
                        if (WiFi.status() != WL_CONNECTED)
                        {
                            if (millis() - _connectTime > 15000)
                            {
                                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("APConfig timeout."));
                                _configStatus = APCFG_TIMEOUT;
                            }
                            return false;
                        }
                        else if (WiFi.status() == WL_CONNECTED)
                        {
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi Connected."));
                            BLINKER_LOG(TAG_ESP_AT, BLINKER_F("IP Address: "));
                            BLINKER_LOG(TAG_ESP_AT, WiFi.localIP());
                            _isWiFiInit = true;
                            _connectTime = 0;

                            // begin();

                            EEPROM.begin(BLINKER_EEP_SIZE);
                            EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
                            EEPROM.commit();
                            EEPROM.end();

                            _status = BLINKER_WLAN_CONNECTED;
                            
                            return false;
                        }
                        // return false;
                    case APCFG_TIMEOUT :
                        softAPinit();
                        return false;
                    default :
                        yield();
                        return false;
                }
            // #elif defined(BLINKER_WIFI_MULTI)
            case BLINKER_MULTI:
                _connectTime = millis();
                if (wifiMulti.run() != WL_CONNECTED) {
                    ::delay(500);

                    if (millis() - _connectTime > BLINKER_CONNECT_TIMEOUT_MS && WiFi.status() != WL_CONNECTED) {
                        // _connectTime = millis();
                        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi connect timeout, please check ssid and pswd!"));
                        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Retring WiFi connect again!"));
                        return false;
                    }

                    return false;
                }
                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("WiFi Connected."));
                BLINKER_LOG(TAG_ESP_AT, BLINKER_F("IP Address: "));
                BLINKER_LOG(TAG_ESP_AT, WiFi.localIP());
                _isWiFiInit = true;
                _connectTime = 0;

                // begin();

                return false;
            // #endif
            default :
                return false;
        }
    }

    // if (!_isBegin)
    // {
    //     _isBegin = init();
    //     return _isBegin;
    // }
    return true;
}

void BlinkerWiFiESPAT::commonBegin(const char* _ssid, const char* _pswd)
{
    _configType = COMM;

    #if defined(ESP8266)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP32_MQTT initialized..."));
    #endif

    connectWiFi(_ssid, _pswd);
}

// #if defined(BLINKER_ESP_SMARTCONFIG)

void BlinkerWiFiESPAT::smartconfigBegin()
{
    _configType = BLINKER_SMART_CONFIG;

    #if defined(ESP8266)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP32_MQTT initialized..."));
    #endif

    if (!autoInit()) smartconfig();
    // else _configStatus = SMART_DONE;
}

void BlinkerWiFiESPAT::smartconfig()
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

    _configStatus = SMART_BEGIN;

    _status = BLINKER_WLAN_SMARTCONFIG_BEGIN;

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Waiting for SmartConfig."));
}

// #elif defined(BLINKER_WIFI_MULTI)

void BlinkerWiFiESPAT::multiBegin(const char* _ssid, const char* _pswd)
{
    _configType = BLINKER_MULTI;

    #if defined(ESP8266)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP32_MQTT initialized..."));
    #endif

    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduinoMQTT_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    wifiMulti.addAP(_ssid, _pswd);

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("wifiMulti add "), _ssid);
}

// #elif defined(BLINKER_APCONFIG)

void BlinkerWiFiESPAT::apconfigBegin()
{
    // #if defined(BLINKER_APCONFIG)
    _configType = BLINKER_AP_CONFIG;

    #if defined(ESP8266)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP8266_MQTT initialized..."));
    #elif defined(ESP32)
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ESP32_MQTT initialized..."));
    #endif
    
    if (!autoInit()) softAPinit();
    // else _configStatus = APCFG_DONE;
    // #endif
}

void BlinkerWiFiESPAT::softAPinit()
{
    IPAddress apIP(192, 168, 4, 1);
    #if defined(ESP8266)
        IPAddress netMsk(255, 255, 255, 0);
    #endif

    WiFi.mode(WIFI_AP);    

    delay(1000);

    String softAP_ssid = BLINKER_F("DiyArduino_");
    softAP_ssid += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(softAP_ssid.c_str());
        WiFi.softAPConfig(apIP, apIP, netMsk);
    #elif defined(ESP32)
        WiFi.setHostname(softAP_ssid.c_str());
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    #endif

    WiFi.softAP(softAP_ssid.c_str(), NULL);
    delay(100);

    webSocket_MQTT_AT.begin();
    webSocket_MQTT_AT.onEvent(webSocketEvent_MQTT_AT);

    _configStatus = APCFG_BEGIN;
    isApCfg = true;

    _status = BLINKER_WLAN_APCONFIG_BEGIN;

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Wait for APConfig"));
}

void BlinkerWiFiESPAT::checkAPCFG()
{
    webSocket_MQTT_AT.loop();

    #if defined(ESP8266)
        MDNS.update();
    #endif

    if (isAvail_MQTT_AT)
    {
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("checkAPCFG: "), msgBuf_MQTT_AT);

        if (STRING_contains_string(msgBuf_MQTT_AT, "ssid") && \
            STRING_contains_string(msgBuf_MQTT_AT, "pswd"))
        {
            parseUrl(msgBuf_MQTT_AT);
        }
        isAvail_MQTT_AT = false;
    }
}

bool BlinkerWiFiESPAT::parseUrl(String data)
{
    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("APCONFIG data: "), data);
    
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject wifi_data = jsonBuffer.as<JsonObject>();
    
    if (error)
    {
        return false;
    }

    String _ssid = wifi_data["ssid"];
    String _pswd = wifi_data["pswd"];

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("ssid: "), _ssid);
    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("pswd: "), _pswd);

    // free(_apServer);
    // MDNS.end();
    webSocket_MQTT_AT.close();

    connectWiFi(_ssid, _pswd);

    _configStatus = APCFG_DONE;
    _connectTime = millis();
    return true;
}

// #endif

void BlinkerWiFiESPAT::connectWiFi(String _ssid, String _pswd)
{
    connectWiFi(_ssid.c_str(), _pswd.c_str());
}

void BlinkerWiFiESPAT::connectWiFi(const char* _ssid, const char* _pswd)
{
    uint32_t _connectTime = millis();

    BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Connecting to ["), _ssid, "]...");

    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduinoMQTT_");
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

    _status = BLINKER_WLAN_CONNECTING;
}

bool BlinkerWiFiESPAT::autoInit()
{
    WiFi.mode(WIFI_STA);
    String _hostname = BLINKER_F("DiyArduino_");
    _hostname += macDeviceName();

    #if defined(ESP8266)
        WiFi.hostname(_hostname.c_str());
    #elif defined(ESP32)
        WiFi.setHostname(_hostname.c_str());
    #endif

    if (checkConfig())
    {

        WiFi.begin();
        ::delay(500);

        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("Connecting to WiFi"));

        _status = BLINKER_WLAN_CONNECTING;
        return true;
    }

    return false;
}

bool BlinkerWiFiESPAT::checkConfig() {
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("check wlan config"));
    
    char ok[2 + 1];
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    if (String(ok) != String("OK")) {
        
        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("wlan config check,fail"));
        return false;
    }
    else {

        BLINKER_LOG(TAG_ESP_AT, BLINKER_F("wlan config check,success"));
        return true;
    }
}

void BlinkerWiFiESPAT::parseData(const char* data)
{
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, String(data));
    JsonObject root = jsonBuffer.as<JsonObject>();

    String _uuid = root["fromDevice"].as<String>();
    String dataGet = root["data"].as<String>();

    if (_use_broker == blinker_b)
    {
        if (_uuid == "ServerSender")
        {
            _uuid = root["data"]["from"].as<String>();
            String _mId = root["data"]["messageId"].as<String>();
            strcpy(_messageId, _mId.c_str());
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("_messageId: "), _mId);
        }
    }

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("data: "), dataGet);
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("fromDevice: "), _uuid);

    if (strcmp(_uuid.c_str(), UUID_MQTT_AT) == 0)
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("Authority uuid"));

        kaTime = millis();
        isAvail_MQTT_AT = true;
        isAlive = true;

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    }
    else if (_uuid == BLINKER_CMD_ALIGENIE)
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("form AliGenie"));

        aliKaTime = millis();
        isAliAlive = true;
        isAliAvail = true;
    }
    else if (_uuid == BLINKER_CMD_DUEROS)
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("form DuerOS"));

        duerKaTime = millis();
        isDuerAlive = true;
        isDuerAvail = true;
    }
    else if (_uuid == BLINKER_CMD_MIOT)
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("form MIOT"));

        miKaTime = millis();
        isMIOTAlive = true;
        isMIOTAvail = true;
    }
    else if (_uuid == BLINKER_CMD_SERVERCLIENT || _uuid == "senderClient1")
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("form Sever"));

        isAvail_MQTT_AT = true;
        isAlive = true;

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
    }
    else
    {
        BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("_sharerCount: "), _sharerCount);
        if (_sharerCount)
        {
            for (uint8_t num = 0; num < _sharerCount; num++)
            {
                if (strcmp(_uuid.c_str(), _sharers[num]->uuid()) == 0)
                {
                    _sharerFrom = num;

                    kaTime = millis();

                    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("From sharer: "), _uuid);
                    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("sharer num: "), num);
                    
                    _needCheckShare = false;

                    dataGet = root["data"].as<String>();

                    break;
                }
                else
                {
                    BLINKER_ERR_LOG_ALL(TAG_ESP_AT, BLINKER_F("No authority uuid found, check is from bridge/share device, data: "), dataGet);

                    _needCheckShare = true;

                    dataGet = data;
                }
            }
        }
        else
        {
            BLINKER_ERR_LOG_ALL(TAG_ESP_AT, BLINKER_F("No authority&share uuid found, check is from bridge/share device, data: "), dataGet);

            _needCheckShare = true;

            dataGet = data;
        }

        isAvail_MQTT_AT = true;
        isAlive = true;
    }

    if (isFresh_MQTT_AT) free(msgBuf_MQTT_AT);
    msgBuf_MQTT_AT = (char*)malloc((dataGet.length()+1)*sizeof(char));
    strcpy(msgBuf_MQTT_AT, dataGet.c_str());
    isFresh_MQTT_AT = true;

    this->latestTime = millis();

    dataFrom_MQTT_AT = BLINKER_MSG_FROM_MQTT;

    if (_needCheckShare && isMQTTinit) freshSharers();
}

void BlinkerWiFiESPAT::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerWiFiESPAT::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT / 2 && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));

        checkKA();

        return false;
    }
}

int BlinkerWiFiESPAT::checkPrintSpan() {
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));

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

int BlinkerWiFiESPAT::checkPrintLimit()
{
    if ((millis() - _print_time) < 60000)
    {
        if (_print_times < 30) return true;
        else 
        {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT MSG LIMIT"));
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

// #if defined(BLINKER_ALIGENIE)
int BlinkerWiFiESPAT::aliAvail()
{
    if (!checkInit()) return false;

    if (isAliAvail)
    {
        isAliAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerWiFiESPAT::aliPrint(const String & data)
{
    if (!checkInit()) return false;

    String data_add = BLINKER_F("{\"data\":");

    // if (_use_broker == aliyun_b)
    // {
    //     data_add += data;
    // }
    // else if (_use_broker == blinker_b)
    // {
        data_add += data.substring(0, data.length() - 1);
        data_add += BLINKER_F(",\"messageId\":\"");
        data_add += STRING_format(_messageId);
        data_add += BLINKER_F("\"}");
    // }

    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += DEVICE_NAME_MQTT_AT;

    // if (_use_broker == aliyun_b)
    // {
    //     data_add += BLINKER_F("\",\"toDevice\":\"AliGenie_r\"");
    // }
    // else if (_use_broker == blinker_b)
    // {
        data_add += BLINKER_F("\",\"toDevice\":\"ServerReceiver\"");
    // }

    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT AliGenie Publish..."));
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

        
        char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

        if (is_rrpc)
        {
            
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT_AT);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, DEVICE_NAME_MQTT_AT);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
        }
        else
        {
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_MQTT_AT);
        }

        is_rrpc = false;

        char send_data[BLINKER_MAX_SEND_SIZE];

        // if (_use_broker == aliyun_b)
        // {
        //     strcpy(send_data, base64::encode(data_add).c_str());
        // }
        // else if (_use_broker == blinker_b)
        // {
            strcpy(send_data, data_add.c_str());
        // }

        if (! mqtt_MQTT_AT->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, send_data))
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, data_add);
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            isAliAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, data_add);
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            isAliAlive = false;

            this->latestTime = millis();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}    

int BlinkerWiFiESPAT::checkAliKA()
{
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerWiFiESPAT::checkAliPrintSpan()
{
    if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("ALIGENIE NOT ALIVE OR MSG LIMIT"));

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
// #endif

// #if defined(BLINKER_DUEROS)
int BlinkerWiFiESPAT::duerAvail()
{
    if (!checkInit()) return false;

    if (isDuerAvail)
    {
        isDuerAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerWiFiESPAT::duerPrint(const String & data, bool report)
{
    if (!checkInit()) return false;

    String data_add = BLINKER_F("{\"data\":");

    if (report)
    {
        data_add += BLINKER_F("{\"report\":");

        // if (_use_broker == aliyun_b)
        // {
        //     data_add += data;
        // }
        // else if (_use_broker == blinker_b)
        // {
            data_add += data.substring(0, data.length() - 1);
            data_add += BLINKER_F(",\"messageId\":\"");
            data_add += STRING_format(_messageId);
            data_add += BLINKER_F("\"}");
        // }

        data_add += BLINKER_F("}");
    }
    else
    {
        // if (_use_broker == aliyun_b)
        // {
        //     data_add += data;
        // }
        // else if (_use_broker == blinker_b)
        // {
            data_add += data.substring(0, data.length() - 1);
            data_add += BLINKER_F(",\"messageId\":\"");
            data_add += STRING_format(_messageId);
            data_add += BLINKER_F("\"}");
        // }
    }

    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += DEVICE_NAME_MQTT_AT;

    // if (_use_broker == aliyun_b)
    // {
    //     data_add += BLINKER_F("\",\"toDevice\":\"DuerOS_r\"");
    // }
    // else if (_use_broker == blinker_b)
    // {
        data_add += BLINKER_F("\",\"toDevice\":\"ServerReceiver\"");
    // }

    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT DuerOS Publish..."));
    BLINKER_LOG_FreeHeap_ALL();
    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("is_rrpc: "), is_rrpc);

    if (mqtt_MQTT_AT->connected())
    {
        // if (!checkDuerKA())
        // {
        //     return false;
        // }

        if (!checkDuerPrintSpan())
        {
            respDuerTime = millis();
            return false;
        }
        respDuerTime = millis();

        
        char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

        if (is_rrpc)
        {
            
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT_AT);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, DEVICE_NAME_MQTT_AT);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
        }
        else
        {
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_MQTT_AT);
        }

        is_rrpc = false;

        char send_data[BLINKER_MAX_SEND_SIZE];

        // if (_use_broker == aliyun_b)
        // {
        //     strcpy(send_data, base64::encode(data_add).c_str());
        // }
        // else if (_use_broker == blinker_b)
        // {
            strcpy(send_data, data_add.c_str());
        // }

        if (! mqtt_MQTT_AT->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, send_data))
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, data_add);
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            isDuerAlive = false;
            
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, data_add);
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            isDuerAlive = false;

            this->latestTime = millis();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiESPAT::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerWiFiESPAT::checkDuerPrintSpan()
{
    if (millis() - respDuerTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respDuerTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

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
// #endif

// #if defined(BLINKER_MIOT)
int BlinkerWiFiESPAT::miAvail()
{
    if (!checkInit()) return false;

    if (isMIOTAvail)
    {
        isMIOTAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerWiFiESPAT::miPrint(const String & data)
{
    if (!checkInit()) return false;

    String data_add = BLINKER_F("{\"data\":");

    // if (_use_broker == aliyun_b)
    // {
    //     data_add += data;
    // }
    // else if (_use_broker == blinker_b)
    // {
        data_add += data.substring(0, data.length() - 1);
        data_add += BLINKER_F(",\"messageId\":\"");
        data_add += STRING_format(_messageId);
        data_add += BLINKER_F("\"}");
    // }

    data_add += BLINKER_F(",\"fromDevice\":\"");
    data_add += DEVICE_NAME_MQTT_AT;

    // if (_use_broker == aliyun_b)
    // {
    //     data_add += BLINKER_F("\",\"toDevice\":\"MIOT_r\"");
    // }
    // else if (_use_broker == blinker_b)
    // {
        data_add += BLINKER_F("\",\"toDevice\":\"ServerReceiver\"");
    // }

    data_add += BLINKER_F(",\"deviceType\":\"vAssistant\"}");

    if (!isJson(data_add)) return false;

    BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("MQTT MIOT Publish..."));
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

        
        char BLINKER_RRPC_PUB_TOPIC_MQTT[128];

        if (is_rrpc)
        {
            
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, "/sys/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT_AT);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, DEVICE_NAME_MQTT_AT);
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, "/rrpc/response/");
            strcat(BLINKER_RRPC_PUB_TOPIC_MQTT, message_id);

            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("BLINKER_RRPC_PUB_TOPIC_MQTT: "), BLINKER_RRPC_PUB_TOPIC_MQTT);
        }
        else
        {
            strcpy(BLINKER_RRPC_PUB_TOPIC_MQTT, BLINKER_PUB_TOPIC_MQTT_AT);
        }

        is_rrpc = false;

        char send_data[BLINKER_MAX_SEND_SIZE];

        if (_use_broker == aliyun_b)
        {
            strcpy(send_data, base64::encode(data_add).c_str());
        }
        else if (_use_broker == blinker_b)
        {
            strcpy(send_data, data_add.c_str());
        }

        if (! mqtt_MQTT_AT->publish(BLINKER_RRPC_PUB_TOPIC_MQTT, send_data))
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, data_add);
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...Failed"));
            BLINKER_LOG_FreeHeap_ALL();

            isMIOTAlive = false;
            return false;
        }
        else
        {
            BLINKER_LOG_ALL(TAG_ESP_AT, data_add);
            BLINKER_LOG_ALL(TAG_ESP_AT, BLINKER_F("...OK!"));
            BLINKER_LOG_FreeHeap_ALL();

            isMIOTAlive = false;

            this->latestTime = millis();

            return true;
        }
    }
    else
    {
        BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("MQTT Disconnected"));
        return false;
    }
}

int BlinkerWiFiESPAT::checkMIOTKA() {
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerWiFiESPAT::checkMIOTPrintSpan()
{
    if (millis() - respMIOTTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respMIOTTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG_ESP_AT, BLINKER_F("DUEROS NOT ALIVE OR MSG LIMIT"));

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
// #endif

#else

    #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.

#endif

#endif
