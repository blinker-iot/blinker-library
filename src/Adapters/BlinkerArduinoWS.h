#ifndef BlinkerWebSocket_H
#define BlinkerWebSocket_H

#include <Blinker/BlinkerProtocol.h>
#include <modules/WebSockets/WebSocketsServer.h>
// #include <WebSocketsServer.h>
// #include <Hash.h>

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

static char msgBuf[BLINKER_MAX_READ_SIZE];
static bool isConnect = false;
static bool isAvail = false;

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Disconnected! "), num);
#endif

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG6(BLINKER_F("num: "), num, BLINKER_F(", Connected from: "), ip, BLINKER_F(", url: "), (char *)payload);
#endif

                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}");

                isConnect = true;
            }
            break;
        case WStype_TEXT:

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6(BLINKER_F("num: "), num, BLINKER_F(", get Text: "), (char *)payload, BLINKER_F(", length: "), length);
#endif
            if (length < BLINKER_MAX_READ_SIZE) {
                // msgBuf = (char*)malloc((length+1)*sizeof(char));
                // memcpy (msgBuf, (char*)payload, length);
                // buflen = length;
                strcpy(msgBuf, (char*)payload);
                isAvail = true;
            }

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG4("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}

class BlinkerArduinoWS
{
    public :
        BlinkerArduinoWS() {}

        void begin(String deviceName)
        {
            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG1(BLINKER_F("webSocket server started"));
            BLINKER_LOG4(BLINKER_F("ws://"), deviceName, BLINKER_F(".local:"), WS_SERVERPORT);
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

        String lastRead() { return STRING_format(msgBuf); }

        void print(String s_data)
        {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Response: "), s_data);
#endif
            if(connected()) {
                bool state = STRING_contains_string(s_data, BLINKER_CMD_NOTICE);

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

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif

                webSocket.broadcastTXT(s_data);
            }
            else {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Faile... Disconnected"));
#endif
            }
#ifdef BLINKER_DEBUG_ALL
            BLINKER_FreeHeap();
#endif
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
#ifdef BLINKER_DEBUG_ALL
                    BLINKER_ERR_LOG1(BLINKER_F("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT"));
#endif
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
//                 BLINKER_ERR_LOG1("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT");
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