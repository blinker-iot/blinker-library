/**
 * @file WebSockets.h
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

#ifndef WEBSOCKETS_H_
#define WEBSOCKETS_H_

#if defined(ESP8266) || defined(ESP32)

#ifdef STM32_DEVICE
#include <application.h>
#define bit(b) (1UL << (b)) // Taken directly from Arduino.h
#else
#include <Arduino.h>
#include <IPAddress.h>
#endif

#ifdef ARDUINO_ARCH_AVR
#error Version 2.x.x currently does not support Arduino with AVR since there is no support for std namespace of c++.
#error Use Version 1.x.x. (ATmega branch)
#else
#include <functional>
#endif


#ifndef NODEBUG_WEBSOCKETS
#ifdef DEBUG_ESP_PORT
#define DEBUG_WEBSOCKETS(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
//#define DEBUG_WEBSOCKETS(...) os_printf( __VA_ARGS__ )
#endif
#endif


#ifndef DEBUG_WEBSOCKETS
#define DEBUG_WEBSOCKETS(...)
#define NODEBUG_WEBSOCKETS
#endif

#if defined(ESP8266) || defined(ESP32)

#define WEBSOCKETS_MAX_DATA_SIZE  (15*1024)
#define WEBSOCKETS_USE_BIG_MEM
#define GET_FREE_HEAP ESP.getFreeHeap()
// moves all Header strings to Flash (~300 Byte)
//#define WEBSOCKETS_SAVE_RAM

#elif defined(STM32_DEVICE)

#define WEBSOCKETS_MAX_DATA_SIZE  (15*1024)
#define WEBSOCKETS_USE_BIG_MEM
#define GET_FREE_HEAP System.freeMemory()

#else

//atmega328p has only 2KB ram!
#define WEBSOCKETS_MAX_DATA_SIZE  (1024)
// moves all Header strings to Flash
#define WEBSOCKETS_SAVE_RAM

#endif


#define WEBSOCKETS_TCP_TIMEOUT    (2000)

#define NETWORK_ESP8266_ASYNC   (0)
#define NETWORK_ESP8266         (1)
#define NETWORK_W5100           (2)
#define NETWORK_ENC28J60        (3)
#define NETWORK_ESP32           (4)

// max size of the WS Message Header
#define WEBSOCKETS_MAX_HEADER_SIZE  (14)

#if !defined(WEBSOCKETS_NETWORK_TYPE)
// select Network type based
#if defined(ESP8266) || defined(ESP31B)
#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP8266
//#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP8266_ASYNC
//#define WEBSOCKETS_NETWORK_TYPE NETWORK_W5100

#elif defined(ESP32)
#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP32

#else
#define WEBSOCKETS_NETWORK_TYPE NETWORK_W5100

#endif
#endif

// Includes and defined based on Network Type
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)

// Note:
//   No SSL/WSS support for client in Async mode
//   TLS lib need a sync interface!


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#elif defined(ESP31B)
#include <ESP31BWiFi.h>
#else
#error "network type ESP8266 ASYNC only possible on the ESP mcu!"
#endif

// #include <ESPAsyncTCP.h>
#include "modules/ESPAsyncTCP/ESPAsyncTCP.h"
// #include <ESPAsyncTCPbuffer.h>
#include "modules/ESPAsyncTCP/ESPAsyncTCPbuffer.h"
#define WEBSOCKETS_NETWORK_CLASS AsyncTCPbuffer
#define WEBSOCKETS_NETWORK_SERVER_CLASS AsyncServer

#elif (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266)

#if !defined(ESP8266) && !defined(ESP31B)
#error "network type ESP8266 only possible on the ESP mcu!"
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <ESP31BWiFi.h>
#endif
#define WEBSOCKETS_NETWORK_CLASS WiFiClient
#define WEBSOCKETS_NETWORK_SERVER_CLASS WiFiServer

#elif (WEBSOCKETS_NETWORK_TYPE == NETWORK_W5100)

#ifdef STM32_DEVICE
#define WEBSOCKETS_NETWORK_CLASS TCPClient
#define WEBSOCKETS_NETWORK_SERVER_CLASS TCPServer
#else
#include <Ethernet.h>
#include <SPI.h>
#define WEBSOCKETS_NETWORK_CLASS EthernetClient
#define WEBSOCKETS_NETWORK_SERVER_CLASS EthernetServer
#endif

#elif (WEBSOCKETS_NETWORK_TYPE == NETWORK_ENC28J60)

#include <UIPEthernet.h>
#define WEBSOCKETS_NETWORK_CLASS UIPClient
#define WEBSOCKETS_NETWORK_SERVER_CLASS UIPServer

#elif (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)

#include <WiFi.h>
#include <WiFiClientSecure.h>
#define WEBSOCKETS_NETWORK_CLASS WiFiClient
#define WEBSOCKETS_NETWORK_SERVER_CLASS WiFiServer

#else
#error "no network type selected!"
#endif

// moves all Header strings to Flash (~300 Byte)
#ifdef WEBSOCKETS_SAVE_RAM
#define WEBSOCKETS_STRING(var)  F(var)
#else
#define WEBSOCKETS_STRING(var)  var
#endif

typedef enum {
    WSC_NOT_CONNECTED,
    WSC_HEADER,
    WSC_CONNECTED
} WSclientsStatus_t;

typedef enum {
    WStype_ERROR,
    WStype_DISCONNECTED,
    WStype_CONNECTED,
    WStype_TEXT,
    WStype_BIN,
	WStype_FRAGMENT_TEXT_START,
	WStype_FRAGMENT_BIN_START,
	WStype_FRAGMENT,
	WStype_FRAGMENT_FIN,
} WStype_t;

typedef enum {
    WSop_continuation = 0x00, ///< %x0 denotes a continuation frame
    WSop_text = 0x01,         ///< %x1 denotes a text frame
    WSop_binary = 0x02,       ///< %x2 denotes a binary frame
                              ///< %x3-7 are reserved for further non-control frames
    WSop_close = 0x08,        ///< %x8 denotes a connection close
    WSop_ping = 0x09,         ///< %x9 denotes a ping
    WSop_pong = 0x0A          ///< %xA denotes a pong
                              ///< %xB-F are reserved for further control frames
} WSopcode_t;

typedef struct {

        bool fin;
        bool rsv1;
        bool rsv2;
        bool rsv3;

        WSopcode_t opCode;
        bool mask;

        size_t payloadLen;

        uint8_t * maskKey;
} WSMessageHeader_t;

typedef struct {
        uint8_t num; ///< connection number

        WSclientsStatus_t status;

        WEBSOCKETS_NETWORK_CLASS * tcp;

        bool isSocketIO;    ///< client for socket.io server

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
        bool isSSL;             ///< run in ssl mode
        WiFiClientSecure * ssl;
#endif

        String cUrl;        ///< http url
        uint16_t cCode;     ///< http code

        bool cIsClient = false;     ///< will be used for masking
        bool cIsUpgrade;    ///< Connection == Upgrade
        bool cIsWebsocket;  ///< Upgrade == websocket

        String cSessionId;  ///< client Set-Cookie (session id)
        String cKey;        ///< client Sec-WebSocket-Key
        String cAccept;     ///< client Sec-WebSocket-Accept
        String cProtocol;   ///< client Sec-WebSocket-Protocol
        String cExtensions; ///< client Sec-WebSocket-Extensions
        uint16_t cVersion;  ///< client Sec-WebSocket-Version

        uint8_t cWsRXsize;  ///< State of the RX
        uint8_t cWsHeader[WEBSOCKETS_MAX_HEADER_SIZE]; ///< RX WS Message buffer
        WSMessageHeader_t cWsHeaderDecode;

        String base64Authorization; ///< Base64 encoded Auth request
        String plainAuthorization; ///< Base64 encoded Auth request

        String extraHeaders;

        bool cHttpHeadersValid; ///< non-websocket http header validity indicator
        size_t cMandatoryHeadersCount; ///< non-websocket mandatory http headers present count

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
        String cHttpLine;   ///< HTTP header lines
#endif

} WSclient_t;



class WebSockets {
    protected:
#ifdef __AVR__
        typedef void (*WSreadWaitCb)(WSclient_t * client, bool ok);
#else
        typedef std::function<void(WSclient_t * client, bool ok)> WSreadWaitCb;
#endif

        virtual void clientDisconnect(WSclient_t * client) = 0;
        virtual bool clientIsConnected(WSclient_t * client) = 0;

        virtual void messageReceived(WSclient_t * client, WSopcode_t opcode, uint8_t * payload, size_t length, bool fin) = 0;

        void clientDisconnect(WSclient_t * client, uint16_t code, char * reason = NULL, size_t reasonLen = 0);
        bool sendFrame(WSclient_t * client, WSopcode_t opcode, uint8_t * payload = NULL, size_t length = 0, bool fin = true, bool headerToPayload = false);

        void headerDone(WSclient_t * client);

        void handleWebsocket(WSclient_t * client);

        bool handleWebsocketWaitFor(WSclient_t * client, size_t size);
        void handleWebsocketCb(WSclient_t * client);
        void handleWebsocketPayloadCb(WSclient_t * client, bool ok, uint8_t * payload);

        String acceptKey(String & clientKey);
        String base64_encode(uint8_t * data, size_t length);

        bool readCb(WSclient_t * client, uint8_t *out, size_t n, WSreadWaitCb cb);
        virtual size_t write(WSclient_t * client, uint8_t *out, size_t n);
        size_t write(WSclient_t * client, const char *out);


};

#ifndef UNUSED
#define UNUSED(var) (void)(var)
#endif

#endif
#endif /* WEBSOCKETS_H_ */
