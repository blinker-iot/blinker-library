/**
 * SocketIOclient.h
 *
 *  Created on: May 12, 2018
 *      Author: links
 */

#ifndef SOCKETIOCLIENT_H_
#define SOCKETIOCLIENT_H_

#include "WebSockets.h"
#include "WebSocketsClient.h"

#define EIO_HEARTBEAT_INTERVAL 20000

#define EIO_MAX_HEADER_SIZE (WEBSOCKETS_MAX_HEADER_SIZE + 1)
#define SIO_MAX_HEADER_SIZE (EIO_MAX_HEADER_SIZE + 1)

typedef enum {
    eIOtype_OPEN    = '0',    ///< Sent from the server when a new transport is opened (recheck)
    eIOtype_CLOSE   = '1',    ///< Request the close of this transport but does not shutdown the connection itself.
    eIOtype_PING    = '2',    ///< Sent by the client. Server should answer with a pong packet containing the same data
    eIOtype_PONG    = '3',    ///< Sent by the server to respond to ping packets.
    eIOtype_MESSAGE = '4',    ///< actual message, client and server should call their callbacks with the data
    eIOtype_UPGRADE = '5',    ///< Before engine.io switches a transport, it tests, if server and client can communicate over this transport. If this test succeed, the client sends an upgrade packets which requests the server to flush its cache on the old transport and switch to the new transport.
    eIOtype_NOOP    = '6',    ///< A noop packet. Used primarily to force a poll cycle when an incoming websocket connection is received.
} engineIOmessageType_t;

typedef enum {
    sIOtype_CONNECT      = '0',
    sIOtype_DISCONNECT   = '1',
    sIOtype_EVENT        = '2',
    sIOtype_ACK          = '3',
    sIOtype_ERROR        = '4',
    sIOtype_BINARY_EVENT = '5',
    sIOtype_BINARY_ACK   = '6',
} socketIOmessageType_t;

class SocketIOclient : protected WebSocketsClient {
  public:
#ifdef __AVR__
    typedef void (*SocketIOclientEvent)(socketIOmessageType_t type, uint8_t * payload, size_t length);
#else
    typedef std::function<void(socketIOmessageType_t type, uint8_t * payload, size_t length)> SocketIOclientEvent;
#endif

    SocketIOclient(void);
    virtual ~SocketIOclient(void);

    void begin(const char * host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * protocol = "arduino", uint32_t pingInterval = 60 * 1000, uint32_t pongTimeout = 90 * 1000, uint8_t disconnectTimeoutCount = 5);
    void begin(String host, uint16_t port, String url = "/socket.io/?EIO=3", String protocol = "arduino", uint32_t pingInterval = 60 * 1000, uint32_t pongTimeout = 90 * 1000, uint8_t disconnectTimeoutCount = 5);

#ifdef HAS_SSL
    void beginSSL(const char * host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * protocol = "arduino", uint32_t pingInterval = 60 * 1000, uint32_t pongTimeout = 90 * 1000, uint8_t disconnectTimeoutCount = 5);
    void beginSSL(String host, uint16_t port, String url = "/socket.io/?EIO=3", String protocol = "arduino", uint32_t pingInterval = 60 * 1000, uint32_t pongTimeout = 90 * 1000, uint8_t disconnectTimeoutCount = 5);
#ifndef SSL_AXTLS
    void beginSSLWithCA(const char * host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * CA_cert = NULL, const char * protocol = "arduino", uint32_t pingInterval = 60 * 1000, uint32_t pongTimeout = 90 * 1000, uint8_t disconnectTimeoutCount = 5);
    void setSSLClientCertKey(const char * clientCert = NULL, const char * clientPrivateKey = NULL);
#if defined(SSL_BARESSL)
    void beginSSLWithCA(const char * host, uint16_t port, const char * url = "/socket.io/?EIO=3", BearSSL::X509List * CA_cert = NULL, const char * protocol = "arduino", uint32_t pingInterval = 60 * 1000, uint32_t pongTimeout = 90 * 1000, uint8_t disconnectTimeoutCount = 5);
    void setSSLClientCertKey(BearSSL::X509List * clientCert = NULL, BearSSL::PrivateKey * clientPrivateKey = NULL);
#endif
#endif
#endif
    bool isConnected(void);

    void onEvent(SocketIOclientEvent cbEvent);
    void disconnect(void);

    bool sendEVENT(uint8_t * payload, size_t length = 0, bool headerToPayload = false);
    bool sendEVENT(const uint8_t * payload, size_t length = 0);
    bool sendEVENT(char * payload, size_t length = 0, bool headerToPayload = false);
    bool sendEVENT(const char * payload, size_t length = 0);
    bool sendEVENT(String & payload);

    bool send(socketIOmessageType_t type, uint8_t * payload, size_t length = 0, bool headerToPayload = false);
    bool send(socketIOmessageType_t type, const uint8_t * payload, size_t length = 0);
    bool send(socketIOmessageType_t type, char * payload, size_t length = 0, bool headerToPayload = false);
    bool send(socketIOmessageType_t type, const char * payload, size_t length = 0);
    bool send(socketIOmessageType_t type, String & payload);

    void setExtraHeaders(const char * extraHeaders = NULL);
    void setReconnectInterval(unsigned long time);

    void loop(void);

    void configureEIOping(bool disableHeartbeat = false);

  protected:
    bool _disableHeartbeat  = false;
    uint64_t _lastHeartbeat = 0;
    SocketIOclientEvent _cbEvent;
    virtual void runIOCbEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
        if(_cbEvent) {
            _cbEvent(type, payload, length);
        }
    }

    void initClient(void);

    // Handeling events from websocket layer
    virtual void runCbEvent(WStype_t type, uint8_t * payload, size_t length) {
        handleCbEvent(type, payload, length);
    }
    void handleCbEvent(WStype_t type, uint8_t * payload, size_t length);
};

#endif /* SOCKETIOCLIENT_H_ */
