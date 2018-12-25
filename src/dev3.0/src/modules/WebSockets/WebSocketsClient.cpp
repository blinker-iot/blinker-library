/**
 * @file WebSocketsClient.cpp
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

#if defined(ESP8266) || defined(ESP32)

#include "WebSockets.h"
#include "WebSocketsClient.h"

WebSocketsClient::WebSocketsClient() {
    _cbEvent = NULL;
    _client.num = 0;
    _client.cIsClient = true;
    _client.extraHeaders = WEBSOCKETS_STRING("Origin: file://");
}

WebSocketsClient::~WebSocketsClient() {
    disconnect();
}

/**
 * calles to init the Websockets server
 */
void WebSocketsClient::begin(const char *host, uint16_t port, const char * url, const char * protocol) {
    _host = host;
    _port = port;
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
    _fingerprint = "";
#endif

    _client.num = 0;
    _client.status = WSC_NOT_CONNECTED;
    _client.tcp = NULL;
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
    _client.isSSL = false;
    _client.ssl = NULL;
#endif
    _client.cUrl = url;
    _client.cCode = 0;
    _client.cIsUpgrade = false;
    _client.cIsWebsocket = true;
    _client.cKey = "";
    _client.cAccept = "";
    _client.cProtocol = protocol;
    _client.cExtensions = "";
    _client.cVersion = 0;
    _client.base64Authorization = "";
    _client.plainAuthorization = "";
    _client.isSocketIO = false;

#ifdef ESP8266
    randomSeed(RANDOM_REG32);
#else
    // todo find better seed
    randomSeed(millis());
#endif
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
    asyncConnect();
#endif

    _lastConnectionFail = 0;
    _reconnectInterval = 500;
}

void WebSocketsClient::begin(String host, uint16_t port, String url, String protocol) {
    begin(host.c_str(), port, url.c_str(), protocol.c_str());
}

void WebSocketsClient::begin(IPAddress host, uint16_t port, const char * url, const char * protocol) {
    return begin(host.toString().c_str(), port, url, protocol);
}

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
void WebSocketsClient::beginSSL(const char *host, uint16_t port, const char * url, const char * fingerprint, const char * protocol) {
    begin(host, port, url, protocol);
    _client.isSSL = true;
    _fingerprint = fingerprint;
}

void WebSocketsClient::beginSSL(String host, uint16_t port, String url, String fingerprint, String protocol) {
    beginSSL(host.c_str(), port, url.c_str(), fingerprint.c_str(), protocol.c_str());
}
#endif

void WebSocketsClient::beginSocketIO(const char *host, uint16_t port, const char * url, const char * protocol) {
    begin(host, port, url, protocol);
    _client.isSocketIO = true;
}

void WebSocketsClient::beginSocketIO(String host, uint16_t port, String url, String protocol) {
    beginSocketIO(host.c_str(), port, url.c_str(), protocol.c_str());
}

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
void WebSocketsClient::beginSocketIOSSL(const char *host, uint16_t port, const char * url, const char * protocol) {
    begin(host, port, url, protocol);
    _client.isSocketIO = true;
    _client.isSSL = true;
    _fingerprint = "";
}

void WebSocketsClient::beginSocketIOSSL(String host, uint16_t port, String url, String protocol) {
    beginSocketIOSSL(host.c_str(), port, url.c_str(), protocol.c_str());
}
#endif

#if (WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
/**
 * called in arduino loop
 */
void WebSocketsClient::loop(void) {
    if(!clientIsConnected(&_client)) {
        // do not flood the server
        if((millis() - _lastConnectionFail) < _reconnectInterval) {
            return;
        }

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
        if(_client.isSSL) {
            DEBUG_WEBSOCKETS("[WS-Client] connect wss...\n");
            if(_client.ssl) {
                delete _client.ssl;
                _client.ssl = NULL;
                _client.tcp = NULL;
            }
            _client.ssl = new WiFiClientSecure();
            _client.tcp = _client.ssl;
        } else {
            DEBUG_WEBSOCKETS("[WS-Client] connect ws...\n");
            if(_client.tcp) {
                delete _client.tcp;
                _client.tcp = NULL;
            }
            _client.tcp = new WiFiClient();
        }
#else
        _client.tcp = new WEBSOCKETS_NETWORK_CLASS();
#endif

        if(!_client.tcp) {
            DEBUG_WEBSOCKETS("[WS-Client] creating Network class failed!");
            return;
        }

        if(_client.tcp->connect(_host.c_str(), _port)) {
            connectedCb();
            _lastConnectionFail = 0;
        } else {
            connectFailedCb();
            _lastConnectionFail = millis();

        }
    } else {
        handleClientData();
    }
}
#endif

/**
 * set callback function
 * @param cbEvent WebSocketServerEvent
 */
void WebSocketsClient::onEvent(WebSocketClientEvent cbEvent) {
    _cbEvent = cbEvent;
}

/**
 * send text data to client
 * @param num uint8_t client id
 * @param payload uint8_t *
 * @param length size_t
 * @param headerToPayload bool  (see sendFrame for more details)
 * @return true if ok
 */
bool WebSocketsClient::sendTXT(uint8_t * payload, size_t length, bool headerToPayload) {
    if(length == 0) {
        length = strlen((const char *) payload);
    }
    if(clientIsConnected(&_client)) {
        return sendFrame(&_client, WSop_text, payload, length, true, headerToPayload);
    }
    return false;
}

bool WebSocketsClient::sendTXT(const uint8_t * payload, size_t length) {
    return sendTXT((uint8_t *) payload, length);
}

bool WebSocketsClient::sendTXT(char * payload, size_t length, bool headerToPayload) {
    return sendTXT((uint8_t *) payload, length, headerToPayload);
}

bool WebSocketsClient::sendTXT(const char * payload, size_t length) {
    return sendTXT((uint8_t *) payload, length);
}

bool WebSocketsClient::sendTXT(String & payload) {
    return sendTXT((uint8_t *) payload.c_str(), payload.length());
}

/**
 * send binary data to client
 * @param num uint8_t client id
 * @param payload uint8_t *
 * @param length size_t
 * @param headerToPayload bool  (see sendFrame for more details)
 * @return true if ok
 */
bool WebSocketsClient::sendBIN(uint8_t * payload, size_t length, bool headerToPayload) {
    if(clientIsConnected(&_client)) {
        return sendFrame(&_client, WSop_binary, payload, length, true, headerToPayload);
    }
    return false;
}

bool WebSocketsClient::sendBIN(const uint8_t * payload, size_t length) {
    return sendBIN((uint8_t *) payload, length);
}

/**
 * sends a WS ping to Server
 * @param payload uint8_t *
 * @param length size_t
 * @return true if ping is send out
 */
bool WebSocketsClient::sendPing(uint8_t * payload, size_t length) {
    if(clientIsConnected(&_client)) {
        return sendFrame(&_client, WSop_ping, payload, length);
    }
    return false;
}

bool WebSocketsClient::sendPing(String & payload) {
    return sendPing((uint8_t *) payload.c_str(), payload.length());
}

/**
 * disconnect one client
 * @param num uint8_t client id
 */
void WebSocketsClient::disconnect(void) {
    if(clientIsConnected(&_client)) {
        WebSockets::clientDisconnect(&_client, 1000);
    }
}

/**
 * set the Authorizatio for the http request
 * @param user const char *
 * @param password const char *
 */
void WebSocketsClient::setAuthorization(const char * user, const char * password) {
    if(user && password) {
        String auth = user;
        auth += ":";
        auth += password;
        _client.base64Authorization = base64_encode((uint8_t *) auth.c_str(), auth.length());
    }
}

/**
 * set the Authorizatio for the http request
 * @param auth const char * base64
 */
void WebSocketsClient::setAuthorization(const char * auth) {
    if(auth) {
        //_client.base64Authorization = auth;
        _client.plainAuthorization = auth;
    }
}

/**
 * set extra headers for the http request;
 * separate headers by "\r\n"
 * @param extraHeaders const char * extraHeaders
 */
void WebSocketsClient::setExtraHeaders(const char * extraHeaders) {
    _client.extraHeaders = extraHeaders;
}

/**
 * set the reconnect Interval
 * how long to wait after a connection initiate failed
 * @param time in ms
 */
void WebSocketsClient::setReconnectInterval(unsigned long time) {
    _reconnectInterval = time;
}

//#################################################################################
//#################################################################################
//#################################################################################

/**
 *
 * @param client WSclient_t *  ptr to the client struct
 * @param opcode WSopcode_t
 * @param payload  uint8_t *
 * @param length size_t
 */
void WebSocketsClient::messageReceived(WSclient_t * client, WSopcode_t opcode, uint8_t * payload, size_t length, bool fin) {
    WStype_t type = WStype_ERROR;

    UNUSED(client);

    switch(opcode) {
        case WSop_text:
            type = fin ? WStype_TEXT : WStype_FRAGMENT_TEXT_START;
            break;
        case WSop_binary:
            type = fin ? WStype_BIN : WStype_FRAGMENT_BIN_START;
            break;
        case WSop_continuation:
            type = fin ? WStype_FRAGMENT_FIN : WStype_FRAGMENT;
            break;
        case WSop_close:
        case WSop_ping:
        case WSop_pong:
        default:
            break;
    }

    runCbEvent(type, payload, length);

}

/**
 * Disconnect an client
 * @param client WSclient_t *  ptr to the client struct
 */
void WebSocketsClient::clientDisconnect(WSclient_t * client) {

    bool event = false;

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
    if(client->isSSL && client->ssl) {
        if(client->ssl->connected()) {
            client->ssl->flush();
            client->ssl->stop();
        }
        event = true;
        delete client->ssl;
        client->ssl = NULL;
        client->tcp = NULL;
    }
#endif

    if(client->tcp) {
        if(client->tcp->connected()) {
#if (WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
            client->tcp->flush();
#endif
            client->tcp->stop();
        }
        event = true;
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
        client->status = WSC_NOT_CONNECTED;
#else
        delete client->tcp;
#endif
        client->tcp = NULL;
    }

    client->cCode = 0;
    client->cKey = "";
    client->cAccept = "";
    client->cVersion = 0;
    client->cIsUpgrade = false;
    client->cIsWebsocket = false;
    client->cSessionId = "";

    client->status = WSC_NOT_CONNECTED;

    DEBUG_WEBSOCKETS("[WS-Client] client disconnected.\n");
    if(event) {
        runCbEvent(WStype_DISCONNECTED, NULL, 0);
    }
}

/**
 * get client state
 * @param client WSclient_t *  ptr to the client struct
 * @return true = conneted
 */
bool WebSocketsClient::clientIsConnected(WSclient_t * client) {

    if(!client->tcp) {
        return false;
    }

    if(client->tcp->connected()) {
        if(client->status != WSC_NOT_CONNECTED) {
            return true;
        }
    } else {
        // client lost
        if(client->status != WSC_NOT_CONNECTED) {
            DEBUG_WEBSOCKETS("[WS-Client] connection lost.\n");
            // do cleanup
            clientDisconnect(client);
        }
    }

    if(client->tcp) {
        // do cleanup
        clientDisconnect(client);
    }

    return false;
}
#if (WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
/**
 * Handel incomming data from Client
 */
void WebSocketsClient::handleClientData(void) {
    int len = _client.tcp->available();
    if(len > 0) {
        switch(_client.status) {
            case WSC_HEADER: {
                String headerLine = _client.tcp->readStringUntil('\n');
                handleHeader(&_client, &headerLine);
            }
                break;
            case WSC_CONNECTED:
                WebSockets::handleWebsocket(&_client);
                break;
            default:
                WebSockets::clientDisconnect(&_client, 1002);
                break;
        }
    }
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
    delay(0);
#endif
}
#endif

/**
 * send the WebSocket header to Server
 * @param client WSclient_t *  ptr to the client struct
 */
void WebSocketsClient::sendHeader(WSclient_t * client) {

    static const char * NEW_LINE = "\r\n";

    DEBUG_WEBSOCKETS("[WS-Client][sendHeader] sending header...\n");

    uint8_t randomKey[16] = { 0 };

    for(uint8_t i = 0; i < sizeof(randomKey); i++) {
        randomKey[i] = random(0xFF);
    }

    client->cKey = base64_encode(&randomKey[0], 16);

#ifndef NODEBUG_WEBSOCKETS
    unsigned long start = micros();
#endif

    String handshake;
    bool ws_header = true;
    String url = client->cUrl;

    if(client->isSocketIO) {
        if(client->cSessionId.length() == 0) {
            url += WEBSOCKETS_STRING("&transport=polling");
            ws_header = false;
        } else {
            url += WEBSOCKETS_STRING("&transport=websocket&sid=");
            url += client->cSessionId;
        }
    }

    handshake = WEBSOCKETS_STRING("GET ");
    handshake += url + WEBSOCKETS_STRING(" HTTP/1.1\r\n"
            "Host: ");
    handshake += _host + ":" + _port + NEW_LINE;

    if(ws_header) {
        handshake += WEBSOCKETS_STRING("Connection: Upgrade\r\n"
                "Upgrade: websocket\r\n"
                "Sec-WebSocket-Version: 13\r\n"
                "Sec-WebSocket-Key: ");
        handshake += client->cKey + NEW_LINE;

        if(client->cProtocol.length() > 0) {
            handshake += WEBSOCKETS_STRING("Sec-WebSocket-Protocol: ");
            handshake += client->cProtocol + NEW_LINE;
        }

        if(client->cExtensions.length() > 0) {
            handshake += WEBSOCKETS_STRING("Sec-WebSocket-Extensions: ");
            handshake += client->cExtensions + NEW_LINE;
        }
    } else {
        handshake += WEBSOCKETS_STRING("Connection: keep-alive\r\n");
    }

    // add extra headers; by default this includes "Origin: file://"
    if(client->extraHeaders) {
        handshake += client->extraHeaders + NEW_LINE;
    }

    handshake += WEBSOCKETS_STRING("User-Agent: arduino-WebSocket-Client\r\n");

    if(client->base64Authorization.length() > 0) {
        handshake += WEBSOCKETS_STRING("Authorization: Basic ");
        handshake += client->base64Authorization + NEW_LINE;
    }

    if(client->plainAuthorization.length() > 0) {
        handshake += WEBSOCKETS_STRING("Authorization: ");
        handshake += client->plainAuthorization + NEW_LINE;
    }

    handshake += NEW_LINE;

    DEBUG_WEBSOCKETS("[WS-Client][sendHeader] handshake %s", (uint8_t* )handshake.c_str());
    write(client, (uint8_t*) handshake.c_str(), handshake.length());

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
    client->tcp->readStringUntil('\n', &(client->cHttpLine), std::bind(&WebSocketsClient::handleHeader, this, client, &(client->cHttpLine)));
#endif

    DEBUG_WEBSOCKETS("[WS-Client][sendHeader] sending header... Done (%luus).\n", (micros() - start));

}

/**
 * handle the WebSocket header reading
 * @param client WSclient_t *  ptr to the client struct
 */
void WebSocketsClient::handleHeader(WSclient_t * client, String * headerLine) {

    headerLine->trim(); // remove \r

    if(headerLine->length() > 0) {
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader] RX: %s\n", headerLine->c_str());

        if(headerLine->startsWith(WEBSOCKETS_STRING("HTTP/1."))) {
            // "HTTP/1.1 101 Switching Protocols"
            client->cCode = headerLine->substring(9, headerLine->indexOf(' ', 9)).toInt();
        } else if(headerLine->indexOf(':')) {
            String headerName = headerLine->substring(0, headerLine->indexOf(':'));
            String headerValue = headerLine->substring(headerLine->indexOf(':') + 1);

            // remove space in the beginning  (RFC2616)
            if(headerValue[0] == ' ') {
                headerValue.remove(0, 1);
            }

            if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Connection"))) {
                if(headerValue.equalsIgnoreCase(WEBSOCKETS_STRING("upgrade"))) {
                    client->cIsUpgrade = true;
                }
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Upgrade"))) {
                if(headerValue.equalsIgnoreCase(WEBSOCKETS_STRING("websocket"))) {
                    client->cIsWebsocket = true;
                }
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Accept"))) {
                client->cAccept = headerValue;
                client->cAccept.trim(); // see rfc6455
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Protocol"))) {
                client->cProtocol = headerValue;
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Extensions"))) {
                client->cExtensions = headerValue;
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Version"))) {
                client->cVersion = headerValue.toInt();
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Set-Cookie"))) {
                if(headerValue.indexOf(WEBSOCKETS_STRING("HttpOnly")) > -1) {
                    client->cSessionId = headerValue.substring(headerValue.indexOf('=') + 1, headerValue.indexOf(";"));
                } else {
                    client->cSessionId = headerValue.substring(headerValue.indexOf('=') + 1);
                }
            }
        } else {
            DEBUG_WEBSOCKETS("[WS-Client][handleHeader] Header error (%s)\n", headerLine->c_str());
        }

        (*headerLine) = "";
#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
        client->tcp->readStringUntil('\n', &(client->cHttpLine), std::bind(&WebSocketsClient::handleHeader, this, client, &(client->cHttpLine)));
#endif

    } else {
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader] Header read fin.\n");
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader] Client settings:\n");

        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cURL: %s\n", client->cUrl.c_str());
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cKey: %s\n", client->cKey.c_str());

        DEBUG_WEBSOCKETS("[WS-Client][handleHeader] Server header:\n");
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cCode: %d\n", client->cCode);
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cIsUpgrade: %d\n", client->cIsUpgrade);
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cIsWebsocket: %d\n", client->cIsWebsocket);
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cAccept: %s\n", client->cAccept.c_str());
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cProtocol: %s\n", client->cProtocol.c_str());
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cExtensions: %s\n", client->cExtensions.c_str());
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cVersion: %d\n", client->cVersion);
        DEBUG_WEBSOCKETS("[WS-Client][handleHeader]  - cSessionId: %s\n", client->cSessionId.c_str());

        bool ok = (client->cIsUpgrade && client->cIsWebsocket);

        if(ok) {
            switch(client->cCode) {
                case 101:  ///< Switching Protocols

                    break;
                case 200:
                    if(client->isSocketIO) {
                        break;
                    }
                case 403: ///< Forbidden
                    // todo handle login
                default:   ///< Server dont unterstand requrst
                    ok = false;
                    DEBUG_WEBSOCKETS("[WS-Client][handleHeader] serverCode is not 101 (%d)\n", client->cCode);
                    clientDisconnect(client);
                    _lastConnectionFail = millis();
                    break;
            }
        }

        if(ok) {

            if(client->cAccept.length() == 0) {
                ok = false;
            } else {
                // generate Sec-WebSocket-Accept key for check
                String sKey = acceptKey(client->cKey);
                if(sKey != client->cAccept) {
                    DEBUG_WEBSOCKETS("[WS-Client][handleHeader] Sec-WebSocket-Accept is wrong\n");
                    ok = false;
                }
            }
        }

        if(ok) {

            DEBUG_WEBSOCKETS("[WS-Client][handleHeader] Websocket connection init done.\n");
            headerDone(client);

            runCbEvent(WStype_CONNECTED, (uint8_t *) client->cUrl.c_str(), client->cUrl.length());

        } else if(clientIsConnected(client) && client->isSocketIO && client->cSessionId.length() > 0) {
            sendHeader(client);
        } else {
            DEBUG_WEBSOCKETS("[WS-Client][handleHeader] no Websocket connection close.\n");
            _lastConnectionFail = millis();
            if(clientIsConnected(client)) {
                write(client, "This is a webSocket client!");
            }
            clientDisconnect(client);
        }
    }
}

void WebSocketsClient::connectedCb() {

    DEBUG_WEBSOCKETS("[WS-Client] connected to %s:%u.\n", _host.c_str(), _port);

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
    _client.tcp->onDisconnect(std::bind([](WebSocketsClient * c, AsyncTCPbuffer * obj, WSclient_t * client) -> bool {
                        DEBUG_WEBSOCKETS("[WS-Server][%d] Disconnect client\n", client->num);
                        client->status = WSC_NOT_CONNECTED;
                        client->tcp = NULL;

                        // reconnect
                        c->asyncConnect();

                        return true;
                    }, this, std::placeholders::_1, &_client));
#endif

    _client.status = WSC_HEADER;

#if (WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
    // set Timeout for readBytesUntil and readStringUntil
    _client.tcp->setTimeout(WEBSOCKETS_TCP_TIMEOUT);
#endif

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266)
    _client.tcp->setNoDelay(true);

    if(_client.isSSL && _fingerprint.length()) {
        if(!_client.ssl->verify(_fingerprint.c_str(), _host.c_str())) {
            DEBUG_WEBSOCKETS("[WS-Client] certificate mismatch\n");
            WebSockets::clientDisconnect(&_client, 1000);
            return;
        }
    }
#endif

    // send Header to Server
    sendHeader(&_client);

}

void WebSocketsClient::connectFailedCb() {
    DEBUG_WEBSOCKETS("[WS-Client] connection to %s:%u Faild\n", _host.c_str(), _port);
}

#if (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)

void WebSocketsClient::asyncConnect() {

    DEBUG_WEBSOCKETS("[WS-Client] asyncConnect...\n");

    AsyncClient * tcpclient = new AsyncClient();

    if(!tcpclient) {
        DEBUG_WEBSOCKETS("[WS-Client] creating AsyncClient class failed!\n");
        return;
    }

    tcpclient->onDisconnect([](void *obj, AsyncClient* c) {
                c->free();
                delete c;
            });

    tcpclient->onConnect(std::bind([](WebSocketsClient * ws , AsyncClient * tcp) {
                        ws->_client.tcp = new AsyncTCPbuffer(tcp);
                        if(!ws->_client.tcp) {
                            DEBUG_WEBSOCKETS("[WS-Client] creating Network class failed!\n");
                            ws->connectFailedCb();
                            return;
                        }
                        ws->connectedCb();
                    }, this, std::placeholders::_2));

    tcpclient->onError(std::bind([](WebSocketsClient * ws , AsyncClient * tcp) {
                        ws->connectFailedCb();

                        // reconnect
                        ws->asyncConnect();
                    }, this, std::placeholders::_2));

    if(!tcpclient->connect(_host.c_str(), _port)) {
        connectFailedCb();
        delete tcpclient;
    }

}

#endif
#endif
