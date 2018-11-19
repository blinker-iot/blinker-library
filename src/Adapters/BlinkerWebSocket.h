#ifndef BlinkerWebSocket_H
#define BlinkerWebSocket_H

#include "Blinker/BlinkerProtocol.h"
#include "modules/WebSockets/WebSocketsServer.h"
// #include <WebSocketsServer.h>
// #include <Hash.h>

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

// static char msgBuf[BLINKER_MAX_READ_SIZE];
static char* msgBuf;//[BLINKER_MAX_READ_SIZE];
static bool isFresh = false;
static bool isConnect = false;
static bool isAvail = false;
static uint8_t ws_num = 0;

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL(("Disconnected! "), num);
            
            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                
                BLINKER_LOG_ALL(("num: "), num, (", Connected from: "), ip, (", url: "), (char *)payload);
                
                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num = num;

                isConnect = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL(("num: "), num, (", get Text: "), (char *)payload, (", length: "), length);
            
            if (length < BLINKER_MAX_READ_SIZE) {
                if (!isFresh) msgBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                // msgBuf = (char*)malloc((length+1)*sizeof(char));
                // memcpy (msgBuf, (char*)payload, length);
                // buflen = length;
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

class BlinkerWebSocket
{
    public :
        BlinkerWebSocket() {}

        void begin(String deviceName)
        {
            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG(BLINKER_F("webSocket server started"));
            BLINKER_LOG(BLINKER_F("ws://"), deviceName, BLINKER_F(".local:"), WS_SERVERPORT);
        }

        bool available()
        {
            webSocket.loop();
            if (isAvail) {
                isAvail = false;

                return true;
            }
            else {
                return false;
            }
        }

        char * lastRead() { return isFresh ? msgBuf : NULL; }
        
        void flush() { 
            if (isFresh) {
                free(msgBuf); isFresh = false; isAvail = false;
            }
        }

        void print(String s_data)
        {
            BLINKER_LOG_ALL(BLINKER_F("Response: "), s_data);

            if(connected()) {
                bool state = STRING_contains_string(s_data, BLINKER_CMD_NOTICE) ||
                            (STRING_contains_string(s_data, BLINKER_CMD_TIMING) && 
                             STRING_contains_string(s_data, BLINKER_CMD_ENABLE)) ||
                            (STRING_contains_string(s_data, BLINKER_CMD_LOOP) && 
                             STRING_contains_string(s_data, BLINKER_CMD_TIMES)) ||
                            (STRING_contains_string(s_data, BLINKER_CMD_COUNTDOWN) &&
                             STRING_contains_string(s_data, BLINKER_CMD_TOTALTIME));

                if (!state) {
                    state = (STRING_contains_string(s_data, BLINKER_CMD_STATE) 
                        && STRING_contains_string(s_data, BLINKER_CMD_CONNECTED));
                }

                if (!state) {
                    if (!checkPrintSpan()) {
                        respTime = millis();
                        return;
                    }
                }

                respTime = millis();
                
                BLINKER_LOG_ALL(BLINKER_F("Succese..."));
                
                webSocket.sendTXT(ws_num, s_data + BLINKER_CMD_NEWLINE);
            }
            else {
                BLINKER_LOG_ALL(BLINKER_F("Faile... Disconnected"));
            }
            // BLINKER_FreeHeap();
        }

        bool connect()
        {
            webSocket.loop();
            return connected();
        }

        bool connected() { return *isHandle; }

        void disconnect() { webSocket.disconnect(); }

    private :
        bool checkPrintSpan() {
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

//         bool checkWSCanPrint() {
//             if (millis() - respTime >= BLINKER_PRINT_MSG_LIMIT || respTime == 0) {
//                 return true;
//             }
//             else {
// #ifdef BLINKER_DEBUG_ALL
//                 BLINKER_ERR_LOG("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT");
// #endif
//                 return false;
//             }
//         }

    protected :
        bool*       isHandle = &isConnect;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
};

#endif