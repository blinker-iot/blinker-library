#ifndef BLINKER_SUBDEVICE_H
#define BLINKER_SUBDEVICE_H

#if defined(ESP8266) || defined(ESP32)

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif

#include <EEPROM.h>

#include "modules/painlessMesh/painlessMesh.h"

// #include "modules/WebSockets/WebSocketsServer.h"
// #include "modules/mqtt/Adafruit_MQTT.h"
// #include "modules/mqtt/Adafruit_MQTT_Client.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "modules/ArduinoJson/ArduinoJson.h"
#endif
// #include "Adapters/BlinkerGateway.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"

char*   msgBuf_PRO;
bool    isFresh_PRO = false;
bool    isAvail_PRO = false;
char*   AUTHKEY_PRO;
char*   MQTT_DEVICEID_PRO;

painlessMesh  mesh;

#ifndef BLINKER_MESH_SSID
    #define BLINKER_MESH_SSID   "blinkerMesh"
#endif

#ifndef BLINKER_MESH_PSWD
    #define BLINKER_MESH_PSWD   "blinkerMesh"
#endif

#ifndef BLINKER_MESH_PORT
    #define BLINKER_MESH_PORT   5555
#endif

char*       meshBuf;
bool        isFresh_mesh = false;
bool        isAvail_mesh = false;
uint32_t    msgFrom;
bool        isTimeSet = false;
float       mesh_timezone = 8.0;

void _receivedCallback(uint32_t from, String &msg)
{
    BLINKER_LOG_ALL("bridge: Received from: ", from, ", msg: ",msg);
    msgFrom = from;

    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, msg);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (error) 
    {
        BLINKER_ERR_LOG_ALL("msg not Json!");
        return;
    }

    if (root.containsKey(BLINKER_CMD_GATE))
    {
        BLINKER_LOG_ALL("gate data");

        if (!isTimeSet || mesh_timezone != root["tz"])
        {
            time_t rtc = root[BLINKER_CMD_TIME];
            mesh_timezone = root["tz"];
            BLINKER_LOG_ALL("rtc: ", rtc, ", tz: ", mesh_timezone);
            timeval tv = { rtc, 0 };
            timezone tz = { int16_t(mesh_timezone*60) , 0 };
            settimeofday(&tv, &tz);

            isTimeSet = true;
        }
        
        time_t now = time(nullptr);
        BLINKER_LOG_ALL("now: ", now, ", ", ctime(&now));        
        
        String _data = root[BLINKER_CMD_GATE];
        meshBuf = (char*)malloc((_data.length()+1)*sizeof(char));
        strcpy(meshBuf, _data.c_str());

        isAvail_mesh = true;
    }
}

void _newConnectionCallback(uint32_t nodeId)
{
    BLINKER_LOG_ALL("--> startHere: New Connection, nodeId = ", nodeId);
    BLINKER_LOG_ALL("--> startHere: New Connection, ", mesh.subConnectionJson(true));
}

class BlinkerSubDevice : public BlinkerStream
{
    public :
        BlinkerSubDevice()
            : isDeviceInit(false)
        {}

        int connect();
        int connected();
        int mConnected();
        void disconnect();
        int available();
        int aligenieAvail();
        int duerAvail();
        int miAvail();
        char * lastRead();
        void flush();
        int print(char * data, bool needCheck = true);
        int bPrint(char * name, const String & data) { return false; }
        int aliPrint(const String & data);
        int duerPrint(const String & data);
        int miPrint(const String & data);
        void begin(const char* _key, const char* _type);
        int autoPrint(uint32_t id);
        char * deviceName();
        char * authKey() { return AUTHKEY_PRO; }
        int init() { return false; }
        int reRegister();
        int deviceRegister() { return false; }
        int authCheck();
        void freshAlive() { kaTime = millis(); isAlive = true; }
        void sharers(const String & data);
        int  needFreshShare() { return false; }

    private :
        bool isDeviceInit;

        void checkKA();
        int checkAliKA();
        int checkDuerKA();
        int checkMIOTKA();
        int checkCanPrint();
        int checkPrintSpan();
        int checkAliPrintSpan();
        int checkDuerPrintSpan();
        int checkMIOTPrintSpan();
        int pubHello();
        uint16_t vatFormat();
        bool meshInit();
        void meshCheck();
        void sendBroadcast(String & msg);
        bool sendSingle(uint32_t toId, String msg);
        String gateFormat(const String & msg);
    
    protected :
        BlinkerSharer * _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
        uint8_t     _sharerCount = 0;
        uint8_t     _sharerFrom = BLINKER_MQTT_FROM_AUTHER;
        const char* _vipKey;
        const char* _deviceType;

        bool        isAlive = false;
        bool        isBavail = false;
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

        bool        isNew = false;
        bool        isAuth = false;
        bool        isFirst = false;

        int isJson(const String & data);

        bool        _isAuthKey = false;
        bool        _isMeshInit = false;
        // bool        _isTimeSet = false;
};

int BlinkerSubDevice::connect()
{
    return true;
}

int BlinkerSubDevice::connected()
{
    return true;
}

int BlinkerSubDevice::mConnected()
{
    return true;
}

void BlinkerSubDevice::disconnect()
{}

int BlinkerSubDevice::available()
{
    return false;
}

int BlinkerSubDevice::aligenieAvail()
{
    if (isAliAvail)
    {
        isAliAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerSubDevice::duerAvail()
{
    if (isDuerAvail)
    {
        isDuerAvail = false;
        return true;
    }
    else {
        return false;
    }
}

int BlinkerSubDevice::miAvail()
{
    if (isMIOTAvail)
    {
        isMIOTAvail = false;
        return true;
    }
    else {
        return false;
    }
}

char * BlinkerSubDevice::lastRead()
{
    if (isFresh_PRO) return msgBuf_PRO;
    return "";
}

void BlinkerSubDevice::flush()
{
    if (isFresh_PRO)
    {
        free(msgBuf_PRO); isFresh_PRO = false; isAvail_PRO = false;
        isAliAvail = false; isDuerAvail = false; isMIOTAvail = false;//isBavail = false;
    }
}

int BlinkerSubDevice::print(char * data, bool needCheck)
{
    return false;
}

int BlinkerSubDevice::aliPrint(const String & data)
{
    return false;
}

int BlinkerSubDevice::duerPrint(const String & data)
{
    return false;
}

int BlinkerSubDevice::miPrint(const String & data)
{
    return false;
}

void BlinkerSubDevice::begin(const char* _key, const char* _type)
{
    _vipKey = _key;
    _deviceType = _type;
    
    BLINKER_LOG_ALL(BLINKER_F("PRO deviceType: "), _type);
}

int BlinkerSubDevice::autoPrint(uint32_t id)
{
    return false;
}

char * BlinkerSubDevice::deviceName() { return MQTT_DEVICEID_PRO;/*MQTT_ID_PRO;*/ }

void BlinkerSubDevice::sharers(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("sharers data: "), data);
}

int BlinkerSubDevice::authCheck()
{
    return false;
}

void BlinkerSubDevice::checkKA() {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = false;
}

int BlinkerSubDevice::checkAliKA() {
    if (millis() - aliKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerSubDevice::checkDuerKA() {
    if (millis() - duerKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerSubDevice::checkMIOTKA() {
    if (millis() - miKaTime >= 10000)
        return false;
    else
        return true;
}

int BlinkerSubDevice::checkCanPrint() {
    if ((millis() - printTime >= BLINKER_PRO_MSG_LIMIT && isAlive) || printTime == 0) {
        return true;
    }
    else {
        BLINKER_ERR_LOG(BLINKER_F("MQTT NOT ALIVE OR MSG LIMIT"));
        
        checkKA();

        return false;
    }
}

int BlinkerSubDevice::checkMIOTPrintSpan()
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

int BlinkerSubDevice::checkPrintSpan() {
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

int BlinkerSubDevice::checkAliPrintSpan()
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

int BlinkerSubDevice::checkDuerPrintSpan()
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

int BlinkerSubDevice::pubHello()
{
    char stateJsonStr[256] = ("{\"message\":\"Registration successful\"}");
    
    BLINKER_LOG_ALL(BLINKER_F("PUB hello: "), stateJsonStr);
    
    return print(stateJsonStr, false);
}

int BlinkerSubDevice::isJson(const String & data)
{
    BLINKER_LOG_ALL(BLINKER_F("isJson: "), data);

    // DynamicJsonBuffer jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject(STRING_format(data));
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    JsonObject root = jsonBuffer.as<JsonObject>();

    // if (!root.success())
    if (error)
    {
        BLINKER_ERR_LOG(BLINKER_F("Print data is not Json! "), data);
        return false;
    }

    return true;
}

uint16_t BlinkerSubDevice::vatFormat()
{
    uint16_t vstNum = 0;
    #if defined(BLINKER_ALIGENIE_LIGHT)
        vstNum |= 0x01 << 0;
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        vstNum |= 0x01 << 1;
    #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
        vstNum |= 0x01 << 2;
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        vstNum |= 0x01 << 3;
    #elif defined(BLINKER_ALIGENIE_TYPE)
        vstNum |= 0x01 << 0;
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        vstNum |= 0x01 << 4;
    #elif defined(BLINKER_DUEROS_OUTLET)
        vstNum |= 0x01 << 5;
    #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
        vstNum |= 0x01 << 6;
    #elif defined(BLINKER_DUEROS_SENSOR)
        vstNum |= 0x01 << 7;
    #elif defined(BLINKER_DUEROS_TYPE)
        vstNum |= 0x01 << 4;
    #endif

    #if defined(BLINKER_MIOT_LIGHT)
        vstNum |= 0x01 << 8;
    #elif defined(BLINKER_MIOT_OUTLET)
        vstNum |= 0x01 << 9;
    #elif defined(BLINKER_MIOT_MULTI_OUTLET)
        vstNum |= 0x01 << 10;
    #elif defined(BLINKER_MIOT_SENSOR)
        vstNum |= 0x01 << 11;
    #elif defined(BLINKER_MIOT_TYPE)
        vstNum |= 0x01 << 8;
    #endif

    BLINKER_LOG_ALL("vstNum: ", vstNum);

    return vstNum;
}

bool BlinkerSubDevice::meshInit()
{
    // if (WiFi.status() != WL_CONNECTED) return false;

    mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
    // Channel set to 6. Make sure to use the same channel for your mesh and for you other
    // network (STATION_SSID)
    mesh.init(BLINKER_MESH_SSID, BLINKER_MESH_PSWD, BLINKER_MESH_PORT);
    // mesh.stationManual(STATION_SSID, STATION_PASSWORD, STATION_PORT, station_ip);
    // BLINKER_LOG_ALL("SSID: ", WiFi.SSID(), ", PWSD: ", WiFi.psk());
    // mesh.stationManual(WiFi.SSID(), WiFi.psk());
    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
    // mesh.setRoot(true);
    // This and all other mesh should ideally now the mesh contains a root
    // mesh.setContainsRoot(true);

    mesh.onReceive(&_receivedCallback);
    mesh.onNewConnection(&_newConnectionCallback);

    // WiFi.reconnect();

    return true;
}

void BlinkerSubDevice::meshCheck()
{
    if (!_isMeshInit)
    {
        if (meshInit()) _isMeshInit = true;
    }
    else
    {
        // if (WiFi.status() != WL_CONNECTED) return;
        mesh.update();

        if (isAvail_mesh)
        {
            BLINKER_LOG_ALL("new mesh data: ", meshBuf);
            if (strcmp(meshBuf, BLINKER_CMD_WHOIS) == 0)
            {
                sendSingle(msgFrom, gateFormat(
                    "{\"" + STRING_format(BLINKER_CMD_DEVICEINFO) + \
                    "\":{" + \
                    "\"name\":\"" + macDeviceName() + "\"," + \
                    "\"key\":\"" + _vipKey + "\"," + \
                    "\"type\":\"" + _deviceType + "\"," + \
                    "\"vas\":" + STRING_format(vatFormat()) + \
                    "}}"));
            }

            isAvail_mesh = false;
            free(meshBuf);
        }
    }
}

void BlinkerSubDevice::sendBroadcast(String & msg)
{
    mesh.sendBroadcast(msg);
}

bool BlinkerSubDevice::sendSingle(uint32_t toId, String msg)
{
    BLINKER_LOG_ALL("to id: ", toId, ", msg: ", msg);
    if (mesh.isConnected(toId))
    {
        return mesh.sendSingle(toId, msg);
    }
    else
    {
        BLINKER_ERR_LOG("device disconnected");
        return false;
    }
}

String BlinkerSubDevice::gateFormat(const String & msg)
{
    return "{\"" + STRING_format(BLINKER_CMD_GATE) + "\":" + msg + "}";
}

#endif

#endif
