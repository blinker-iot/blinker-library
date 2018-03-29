#ifndef BlinkerWebSocket_H
#define BlinkerWebSocket_H

#include <Blinker/BlinkerProtocol.h>
#include <modules/WebSockets/WebSocketsServer.h>
// #include <WebSocketsServer.h>
// #include <Hash.h>

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

static char msgBuf[BLINKER_BUFFER_SIZE];
static bool isConnect = false;
static bool isAvail = false;

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2("Disconnected! ", num);
#endif

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);

#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG6("num: ", num, ", Connected from: ", ip, ", url: ", (char *)payload);
#endif

                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}");

                isConnect = true;
            }
            break;
        case WStype_TEXT:

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG6("num: ", num, ", get Text: ", (char *)payload, ", length: ", length);
#endif
            if (length < BLINKER_BUFFER_SIZE) {
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
            BLINKER_LOG4("ws://", deviceName, ".local:", WS_SERVERPORT);
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

        void print(String s)
        {
#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("Response: "), s);
#endif
            if(connected()) {
#ifdef BLINKER_DEBUG_ALL
                BLINKER_LOG1(BLINKER_F("Succese..."));
#endif
                webSocket.broadcastTXT(s);
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

    protected :
        bool*       isHandle = &isConnect;
};

#endif