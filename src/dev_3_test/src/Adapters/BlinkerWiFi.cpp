#if defined(ESP8266) || defined(ESP32)

#define BLINKER_WIFI

#include "modules/WebSockets/WebSocketsServer.h"
#include "modules/ArduinoJson/ArduinoJson.h"

#include "Adapters/BlinkerWiFi.h"
#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "utility/BlinkerUtility.h"

#define WS_SERVERPORT       81
static WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

static char* msgBuf;
static bool isFresh = false;
static bool isConnect = false;
static bool isAvail = false;
static uint8_t ws_num = 0;

static void webSocketEvent(uint8_t num, WStype_t type, \
                    uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(BLINKER_F("Disconnected! "), num);

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                
                BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                                BLINKER_F(", Connected from: "), ip, 
                                BLINKER_F(", url: "), (char *)payload);
                
                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num = num;

                isConnect = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(BLINKER_F("num: "), num, \
                            BLINKER_F(", get Text: "), (char *)payload, \
                            BLINKER_F(", length: "), length);
            
            if (length < BLINKER_MAX_READ_SIZE) {
                if (isFresh) free(msgBuf);
                msgBuf = (char*)malloc((length+1)*sizeof(char));
                strcpy(msgBuf, (char*)payload);
                isAvail = true;
                isFresh = true;
            }

            ws_num = num;

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}

BlinkerWiFi::BlinkerWiFi() { isHandle = &isConnect; }

void BlinkerWiFi::begin(const String & deviceName)
{
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    BLINKER_LOG(BLINKER_F("webSocket server started"));
    BLINKER_LOG(BLINKER_F("ws://"), deviceName, BLINKER_F(".local:"), WS_SERVERPORT);
}

bool BlinkerWiFi::available()
{
    webSocket.loop();
    if (isAvail)
    {
        isAvail = false;

        return true;
    }
    else {
        return false;
    }
}

char * BlinkerWiFi::lastRead()
{
    if (isFresh) return msgBuf;
    else "";
}

void BlinkerWiFi::flush()
{ 
    if (isFresh)
    {
        free(msgBuf); isFresh = false; isAvail = false;
    }
}

void BlinkerWiFi::print(const String & s_data, bool needCheck)
{
    BLINKER_LOG_ALL(BLINKER_F("Response: "), s_data);

    if(connected())
    {
        if (needCheck)
        {
            if (!checkPrintSpan())
            {
                respTime = millis();
                return;
            }
        }

        respTime = millis();
        
        BLINKER_LOG_ALL(BLINKER_F("Succese..."));
        
        webSocket.sendTXT(ws_num, s_data + BLINKER_CMD_NEWLINE);
    }
    else
    {
        BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
    }
    // BLINKER_FreeHeap();
}

bool BlinkerWiFi::connect()
{
    webSocket.loop();
    return connected();
}

bool BlinkerWiFi::connected() { return *isHandle; }

void BlinkerWiFi::disconnect() { webSocket.disconnect(); }

bool BlinkerWiFi::checkPrintSpan()
{
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
            BLINKER_ERR_LOG_ALL(BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));
            
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

#endif
