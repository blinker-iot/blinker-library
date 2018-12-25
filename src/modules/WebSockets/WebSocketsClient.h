/**
 * @file WebSocketsClient.h
 * @date 20.05.2015
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the WebSockets for Arduino.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef WEBSOCKETSCLIENT_H_
#define WEBSOCKETSCLIENT_H_

#if defined(ESP8266) || defined(ESP32)

#include "WebSockets.h"

class WebSocketsClient: private WebSockets {
    public:
#ifdef __AVR__
        typedef void (*WebSocketClientEvent)(WStype_t type, uint8_t * payload, size_t length);
#else
        typedef std::function<void (WStype_t type, uint8_t * payload, size_t length)> WebSocketClientEvent;
#endif


        WebSocketsClient(void);
        virtual ~WebSocketsClient(void);

        void begin(const char *host, uint16_t port, const char * url = "/", const char * protocol = "arduino");
        void begin(String host, uint16_t port, String url = "/", String protocol = "arduino");
        void begin(IPAddress host, uint16_t port, const char * url = "/", const char * protocol = "arduino");

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
        void beginSSL(const char *host, uint16_t port, const char * url = "/", const char * = "", const char * protocol = "arduino");
        void beginSSL(String host, uint16_t port, String url = "/", String fingerprint = "", String protocol = "arduino");
#endif

        void beginSocketIO(const char *host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * protocol = "arduino");
        void beginSocketIO(String host, uint16_t port, String url = "/socket.io/?EIO=3", String protocol = "arduino");

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
        void beginSocketIOSSL(const char *host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * protocol = "arduino");
        void beginSocketIOSSL(String host, uint16_t port, String url = "/socket.io/?EIO=3", String protocol = "arduino");
#endif

#if (WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
        void loop(void);
#else
        // Async interface not need a loop call
        void loop(void) __attribute__ ((deprecated)) {}
#endif

        void onEvent(WebSocketClientEvent cbEvent);

        bool sendTXT(uint8_t * payload, size_t length = 0, bool headerToPayload = false);
        bool sendTXT(const uint8_t * payload, size_t length = 0);
        bool sendTXT(char * payload, size_t length = 0, bool headerToPayload = false);
        bool sendTXT(const char * payload, size_t length = 0);
        bool sendTXT(String & payload);

        bool sendBIN(uint8_t * payload, size_t length, bool headerToPayload = false);
        bool sendBIN(const uint8_t * payload, size_t length);

        bool sendPing(uint8_t * payload = NULL, size_t length = 0);
        bool sendPing(String & payload);

        void disconnect(void);

        void setAuthorization(const char * user, const char * password);
        void setAuthorization(const char * auth);
	
        void setExtraHeaders(const char * extraHeaders = NULL);

        void setReconnectInterval(unsigned long time);

    protected:
        String _host;
        uint16_t _port;

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
        String _fingerprint;
#endif
        WSclient_t _client;

        WebSocketClientEvent _cbEvent;

        unsigned long _lastConnectionFail;
        unsigned long _reconnectInterval;

        void messageReceived(WSclient_t * client, WSopcode_t opcode, uint8_t * payload, size_t length, bool fin);

        void clientDisconnect(WSclient_t * client);
        bool clientIsConnected(WSclient_t * client);

#if (WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
        void handleClientData(void);
#endif

        void sendHeader(WSclient_t * client);
        void handleHeader(WSclient_t * client, String * headerLine);

        void connectedCb();
        void connectFailedCb();

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
        void asyncConnect();
#endif

        /**
         * called for sending a Event to the app
         * @param type WStype_t
         * @param payload uint8_t *
         * @param length size_t
         */
        virtual void runCbEvent(WStype_t type, uint8_t * payload, size_t length) {
            if(_cbEvent) {
                _cbEvent(type, payload, length);
            }
        }

};

#endif

#endif /* WEBSOCKETSCLIENT_H_ */
