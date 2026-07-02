#ifndef BLINKER_WEBSOCKETS_STUB_H
#define BLINKER_WEBSOCKETS_STUB_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_NATIVE_WIFI) && !defined(BLINKER_WIFI_HAS_WEBSOCKETS)

#include <Arduino.h>

typedef enum {
    WStype_ERROR,
    WStype_DISCONNECTED,
    WStype_CONNECTED,
    WStype_TEXT,
    WStype_BIN,
    WStype_FRAGMENT_TEXT_START,
    WStype_FRAGMENT_BIN_START,
    WStype_FRAGMENT,
    WStype_FRAGMENT_FIN
} WStype_t;

class WebSocketsServer
{
    public:
        typedef void (*WebSocketServerEvent)(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

        WebSocketsServer(uint16_t port) { (void)port; }
        void begin(void) {}
        void close(void) {}
        void loop(void) {}
        void disconnect(void) {}
        void disconnect(uint8_t num) { (void)num; }
        void onEvent(WebSocketServerEvent cbEvent) { (void)cbEvent; }
        bool sendTXT(uint8_t num, const char* payload, size_t length = 0)
        {
            (void)num;
            (void)payload;
            (void)length;
            return false;
        }
        IPAddress remoteIP(uint8_t num)
        {
            (void)num;
            return IPAddress(0, 0, 0, 0);
        }
};

#endif

#endif
