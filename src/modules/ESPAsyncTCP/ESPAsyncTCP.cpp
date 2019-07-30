/*
  Asynchronous TCP library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#if defined(ESP8266)
#include <Arduino.h>

#include "ESPAsyncTCP.h"
extern "C"{
  #include <lwip/opt.h>
  #include <lwip/tcp.h>
  #include <lwip/inet.h>
  #include <lwip/dns.h>
  #include <lwip/init.h>
}
#include "tcp_axtls.h"

/*
  Async TCP Client
*/

#if ASYNC_TCP_SSL_ENABLED
AsyncClient::AsyncClient(tcp_pcb* pcb, SSL_CTX * ssl_ctx):
#else
AsyncClient::AsyncClient(tcp_pcb* pcb):
#endif
  _connect_cb(0)
  , _connect_cb_arg(0)
  , _discard_cb(0)
  , _discard_cb_arg(0)
  , _sent_cb(0)
  , _sent_cb_arg(0)
  , _error_cb(0)
  , _error_cb_arg(0)
  , _recv_cb(0)
  , _recv_cb_arg(0)
  , _pb_cb(0)
  , _pb_cb_arg(0)
  , _timeout_cb(0)
  , _timeout_cb_arg(0)
  , _pcb_busy(false)
#if ASYNC_TCP_SSL_ENABLED
  , _pcb_secure(false)
  , _handshake_done(true)
#endif
  , _pcb_sent_at(0)
  , _close_pcb(false)
  , _ack_pcb(true)
  , _tx_unacked_len(0)
  , _tx_acked_len(0)
  , _tx_unsent_len(0)
  , _rx_last_packet(0)
  , _rx_since_timeout(0)
  , _ack_timeout(ASYNC_MAX_ACK_TIME)
  , _connect_port(0)
  , prev(NULL)
  , next(NULL)
{
  _pcb = pcb;
  if(_pcb){
    _rx_last_packet = millis();
    tcp_setprio(_pcb, TCP_PRIO_MIN);
    tcp_arg(_pcb, this);
    tcp_recv(_pcb, &_s_recv);
    tcp_sent(_pcb, &_s_sent);
    tcp_err(_pcb, &_s_error);
    tcp_poll(_pcb, &_s_poll, 1);
#if ASYNC_TCP_SSL_ENABLED
    if(ssl_ctx){
      if(tcp_ssl_new_server(_pcb, ssl_ctx) < 0){
        _close();
        return;
      }
      tcp_ssl_arg(_pcb, this);
      tcp_ssl_data(_pcb, &_s_data);
      tcp_ssl_handshake(_pcb, &_s_handshake);
      tcp_ssl_err(_pcb, &_s_ssl_error);

      _pcb_secure = true;
      _handshake_done = false;
    }
#endif
  }
}

AsyncClient::~AsyncClient(){
  if(_pcb)
    _close();
}

#if ASYNC_TCP_SSL_ENABLED
bool AsyncClient::connect(IPAddress ip, uint16_t port, bool secure){
#else
bool AsyncClient::connect(IPAddress ip, uint16_t port){
#endif
  if (_pcb) //already connected
    return false;
  ip_addr_t addr;
  addr.addr = ip;
#if LWIP_VERSION_MAJOR == 1
  netif* interface = ip_route(&addr);
  if (!interface){ //no route to host
    return false;
  }
#endif
  tcp_pcb* pcb = tcp_new();
  if (!pcb){ //could not allocate pcb
    return false;
  }

#if ASYNC_TCP_SSL_ENABLED
  _pcb_secure = secure;
  _handshake_done = !secure;
#endif
  tcp_arg(pcb, this);
  tcp_err(pcb, &_s_error);
  tcp_connect(pcb, &addr, port,(tcp_connected_fn)&_s_connected);
  return true;
}

#if ASYNC_TCP_SSL_ENABLED
bool AsyncClient::connect(const char* host, uint16_t port, bool secure){
#else
bool AsyncClient::connect(const char* host, uint16_t port){
#endif
  ip_addr_t addr;
  err_t err = dns_gethostbyname(host, &addr, (dns_found_callback)&_s_dns_found, this);
  if(err == ERR_OK) {
#if ASYNC_TCP_SSL_ENABLED
    return connect(IPAddress(addr.addr), port, secure);
#else
    return connect(IPAddress(addr.addr), port);
#endif
  } else if(err == ERR_INPROGRESS) {
#if ASYNC_TCP_SSL_ENABLED
    _pcb_secure = secure;
    _handshake_done = !secure;
#endif
    _connect_port = port;
    return true;
  }
  return false;
}

AsyncClient& AsyncClient::operator=(const AsyncClient& other){
  if (_pcb)
    _close();

  _pcb = other._pcb;
  if (_pcb) {
    _rx_last_packet = millis();
    tcp_setprio(_pcb, TCP_PRIO_MIN);
    tcp_arg(_pcb, this);
    tcp_recv(_pcb, &_s_recv);
    tcp_sent(_pcb, &_s_sent);
    tcp_err(_pcb, &_s_error);
    tcp_poll(_pcb, &_s_poll, 1);
#if ASYNC_TCP_SSL_ENABLED
    if(tcp_ssl_has(_pcb)){
      _pcb_secure = true;
      _handshake_done = false;
      tcp_ssl_arg(_pcb, this);
      tcp_ssl_data(_pcb, &_s_data);
      tcp_ssl_handshake(_pcb, &_s_handshake);
      tcp_ssl_err(_pcb, &_s_ssl_error);
    } else {
      _pcb_secure = false;
      _handshake_done = true;
    }
#endif
  }
  return *this;
}

bool AsyncClient::operator==(const AsyncClient &other) {
  return (_pcb != NULL && other._pcb != NULL && (_pcb->remote_ip.addr == other._pcb->remote_ip.addr) && (_pcb->remote_port == other._pcb->remote_port));
}

int8_t AsyncClient::abort(){
  if(_pcb) {
    tcp_abort(_pcb);
    _pcb = NULL;
  }
  return ERR_ABRT;
}

void AsyncClient::close(bool now){
  if(_pcb)
    tcp_recved(_pcb, _rx_ack_len);
  if(now)
    _close();
  else
    _close_pcb = true;
}

void AsyncClient::stop() {
  close(false);
}

bool AsyncClient::free(){
  if(!_pcb)
    return true;
  if(_pcb->state == 0 || _pcb->state > 4)
    return true;
  return false;
}

size_t AsyncClient::write(const char* data) {
  if(data == NULL)
    return 0;
  return write(data, strlen(data));
}

size_t AsyncClient::write(const char* data, size_t size, uint8_t apiflags) {
  size_t will_send = add(data, size, apiflags);
  if(!will_send || !send())
    return 0;
  return will_send;
}

size_t AsyncClient::add(const char* data, size_t size, uint8_t apiflags) {
  if(!_pcb || size == 0 || data == NULL)
    return 0;
  size_t room = space();
  if(!room)
    return 0;
#if ASYNC_TCP_SSL_ENABLED
  if(_pcb_secure){
    int sent = tcp_ssl_write(_pcb, (uint8_t*)data, size);
    if(sent >= 0){
      _tx_unacked_len += sent;
      return sent;
    }
    _close();
    return 0;
  }
#endif
  size_t will_send = (room < size) ? room : size;
  int8_t err = tcp_write(_pcb, data, will_send, apiflags);
  if(err != ERR_OK)
    return 0;
  _tx_unsent_len += will_send;
  return will_send;
}

bool AsyncClient::send(){
#if ASYNC_TCP_SSL_ENABLED
  if(_pcb_secure)
    return true;
#endif
  if(tcp_output(_pcb) == ERR_OK){
    _pcb_busy = true;
    _pcb_sent_at = millis();
    _tx_unacked_len += _tx_unsent_len;
    _tx_unsent_len = 0;
    return true;
  }
  _tx_unsent_len = 0;
  return false;
}

size_t AsyncClient::ack(size_t len){
  if(len > _rx_ack_len)
    len = _rx_ack_len;
  if(len)
    tcp_recved(_pcb, len);
  _rx_ack_len -= len;
  return len;
}

// Private Callbacks

err_t AsyncClient::_connected(void* pcb, err_t err){
  _pcb = reinterpret_cast<tcp_pcb*>(pcb);
  if(_pcb){
    _pcb_busy = false;
    _rx_last_packet = millis();
    tcp_setprio(_pcb, TCP_PRIO_MIN);
    tcp_recv(_pcb, &_s_recv);
    tcp_sent(_pcb, &_s_sent);
    tcp_poll(_pcb, &_s_poll, 1);
#if ASYNC_TCP_SSL_ENABLED
    if(_pcb_secure){
      if(tcp_ssl_new_client(_pcb) < 0){
        return _close();
      }
      tcp_ssl_arg(_pcb, this);
      tcp_ssl_data(_pcb, &_s_data);
      tcp_ssl_handshake(_pcb, &_s_handshake);
      tcp_ssl_err(_pcb, &_s_ssl_error);
    }
  }
  if(!_pcb_secure && _connect_cb)
#else
  }
  if(_connect_cb)
#endif
    _connect_cb(_connect_cb_arg, this);
  return ERR_OK;
}

int8_t AsyncClient::_close(){
  int8_t err = ERR_OK;
  if(_pcb) {
#if ASYNC_TCP_SSL_ENABLED
    if(_pcb_secure){
      tcp_ssl_free(_pcb);
    }
#endif
    tcp_arg(_pcb, NULL);
    tcp_sent(_pcb, NULL);
    tcp_recv(_pcb, NULL);
    tcp_err(_pcb, NULL);
    tcp_poll(_pcb, NULL, 0);
    err = tcp_close(_pcb);
    if(err != ERR_OK) {
      err = abort();
    }
    _pcb = NULL;
    if(_discard_cb)
      _discard_cb(_discard_cb_arg, this);
  }
  return err;
}

void AsyncClient::_error(err_t err) {
  if(_pcb){
#if ASYNC_TCP_SSL_ENABLED
    if(_pcb_secure){
      tcp_ssl_free(_pcb);
    }
#endif
    tcp_arg(_pcb, NULL);
    tcp_sent(_pcb, NULL);
    tcp_recv(_pcb, NULL);
    tcp_err(_pcb, NULL);
    tcp_poll(_pcb, NULL, 0);
    _pcb = NULL;
  }
  if(_error_cb)
    _error_cb(_error_cb_arg, this, err);
  if(_discard_cb)
    _discard_cb(_discard_cb_arg, this);
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncClient::_ssl_error(int8_t err){
  if(_error_cb)
    _error_cb(_error_cb_arg, this, err+64);
}
#endif

err_t AsyncClient::_sent(tcp_pcb* pcb, uint16_t len) {
#if ASYNC_TCP_SSL_ENABLED
  if (_pcb_secure && !_handshake_done)
    return ERR_OK;
#endif
  _rx_last_packet = millis();
  _tx_unacked_len -= len;
  _tx_acked_len += len;
  ASYNC_TCP_DEBUG("_sent: %u (%d %d)\n", len, _tx_unacked_len, _tx_acked_len);
  if(_tx_unacked_len == 0){
    _pcb_busy = false;
    if(_sent_cb)
      _sent_cb(_sent_cb_arg, this, _tx_acked_len, (millis() - _pcb_sent_at));
      _tx_acked_len = 0;
  }
  return ERR_OK;
}

err_t AsyncClient::_recv(tcp_pcb* pcb, pbuf* pb, err_t err) {
  if(pb == NULL){
    ASYNC_TCP_DEBUG("_recv: pb == NULL! Closing... %d\n", err);
    return _close();
  }

  _rx_last_packet = millis();
#if ASYNC_TCP_SSL_ENABLED
  if(_pcb_secure){
    ASYNC_TCP_DEBUG("_recv: %d\n", pb->tot_len);
    int read_bytes = tcp_ssl_read(pcb, pb);
    if(read_bytes < 0){
      if (read_bytes != SSL_CLOSE_NOTIFY) {
        ASYNC_TCP_DEBUG("_recv err: %d\n", read_bytes);
        _close();
      }
      //return read_bytes;
    }
    return ERR_OK;
  }
#endif
  while(pb != NULL){
    //we should not ack before we assimilate the data
    _ack_pcb = true;
    pbuf *b = pb;
    pb = b->next;
    b->next = NULL;
    ASYNC_TCP_DEBUG("_recv: %d\n", b->len);
    if(_pb_cb){
      _pb_cb(_pb_cb_arg, this, b);
    } else {
      if(_recv_cb)
        _recv_cb(_recv_cb_arg, this, b->payload, b->len);
      if(!_ack_pcb)
        _rx_ack_len += b->len;
      else
        tcp_recved(pcb, b->len);
      pbuf_free(b);
    }
  }
  return ERR_OK;
}

err_t AsyncClient::_poll(tcp_pcb* pcb){
  // Close requested
  if(_close_pcb){
    _close_pcb = false;
    _close();
    return ERR_OK;
  }
  uint32_t now = millis();

  // ACK Timeout
  if(_pcb_busy && _ack_timeout && (now - _pcb_sent_at) >= _ack_timeout){
    _pcb_busy = false;
    if(_timeout_cb)
      _timeout_cb(_timeout_cb_arg, this, (now - _pcb_sent_at));
    return ERR_OK;
  }
  // RX Timeout
  if(_rx_since_timeout && (now - _rx_last_packet) >= (_rx_since_timeout * 1000)){
    _close();
    return ERR_OK;
  }
#if ASYNC_TCP_SSL_ENABLED
  // SSL Handshake Timeout
  if(_pcb_secure && !_handshake_done && (now - _rx_last_packet) >= 2000){
    _close();
    return ERR_OK;
  }
#endif
  // Everything is fine
  if(_poll_cb)
    _poll_cb(_poll_cb_arg, this);
  return ERR_OK;
}

#if LWIP_VERSION_MAJOR == 1
void AsyncClient::_dns_found(struct ip_addr *ipaddr){
#else
void AsyncClient::_dns_found(const ip_addr *ipaddr){
#endif
  if(ipaddr){
#if ASYNC_TCP_SSL_ENABLED
    connect(IPAddress(ipaddr->addr), _connect_port, _pcb_secure);
#else
    connect(IPAddress(ipaddr->addr), _connect_port);
#endif
  } else {
    if(_error_cb)
      _error_cb(_error_cb_arg, this, -55);
    if(_discard_cb)
      _discard_cb(_discard_cb_arg, this);
  }
}

// lWIP Callbacks
#if LWIP_VERSION_MAJOR == 1
void AsyncClient::_s_dns_found(const char *name, ip_addr_t *ipaddr, void *arg){
#else
void AsyncClient::_s_dns_found(const char *name, const ip_addr *ipaddr, void *arg){
#endif
  reinterpret_cast<AsyncClient*>(arg)->_dns_found(ipaddr);
}

err_t AsyncClient::_s_poll(void *arg, struct tcp_pcb *tpcb) {
  return reinterpret_cast<AsyncClient*>(arg)->_poll(tpcb);
}

err_t AsyncClient::_s_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *pb, err_t err) {
  return reinterpret_cast<AsyncClient*>(arg)->_recv(tpcb, pb, err);
}

void AsyncClient::_s_error(void *arg, err_t err) {
  reinterpret_cast<AsyncClient*>(arg)->_error(err);
}

err_t AsyncClient::_s_sent(void *arg, struct tcp_pcb *tpcb, uint16_t len) {
  return reinterpret_cast<AsyncClient*>(arg)->_sent(tpcb, len);
}

err_t AsyncClient::_s_connected(void* arg, void* tpcb, err_t err){
    return reinterpret_cast<AsyncClient*>(arg)->_connected(tpcb, err);
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncClient::_s_data(void *arg, struct tcp_pcb *tcp, uint8_t * data, size_t len){
  AsyncClient *c = reinterpret_cast<AsyncClient*>(arg);
  if(c->_recv_cb)
    c->_recv_cb(c->_recv_cb_arg, c, data, len);
}

void AsyncClient::_s_handshake(void *arg, struct tcp_pcb *tcp, SSL *ssl){
  AsyncClient *c = reinterpret_cast<AsyncClient*>(arg);
  c->_handshake_done = true;
  if(c->_connect_cb)
    c->_connect_cb(c->_connect_cb_arg, c);
}

void AsyncClient::_s_ssl_error(void *arg, struct tcp_pcb *tcp, int8_t err){
  reinterpret_cast<AsyncClient*>(arg)->_ssl_error(err);
}
#endif

// Operators

AsyncClient & AsyncClient::operator+=(const AsyncClient &other) {
  if(next == NULL){
    next = (AsyncClient*)(&other);
    next->prev = this;
  } else {
    AsyncClient *c = next;
    while(c->next != NULL) c = c->next;
    c->next =(AsyncClient*)(&other);
    c->next->prev = c;
  }
  return *this;
}

void AsyncClient::setRxTimeout(uint32_t timeout){
  _rx_since_timeout = timeout;
}

uint32_t AsyncClient::getRxTimeout(){
  return _rx_since_timeout;
}

uint32_t AsyncClient::getAckTimeout(){
  return _ack_timeout;
}

void AsyncClient::setAckTimeout(uint32_t timeout){
  _ack_timeout = timeout;
}

void AsyncClient::setNoDelay(bool nodelay){
  if(!_pcb)
    return;
  if(nodelay)
    tcp_nagle_disable(_pcb);
  else
    tcp_nagle_enable(_pcb);
}

bool AsyncClient::getNoDelay(){
  if(!_pcb)
    return false;
  return tcp_nagle_disabled(_pcb);
}

uint16_t AsyncClient::getMss(){
  if(_pcb)
    return tcp_mss(_pcb);
  return 0;
}

uint32_t AsyncClient::getRemoteAddress() {
  if(!_pcb)
    return 0;
  return _pcb->remote_ip.addr;
}

uint16_t AsyncClient::getRemotePort() {
  if(!_pcb)
    return 0;
  return _pcb->remote_port;
}

uint32_t AsyncClient::getLocalAddress() {
  if(!_pcb)
    return 0;
  return _pcb->local_ip.addr;
}

uint16_t AsyncClient::getLocalPort() {
  if(!_pcb)
    return 0;
  return _pcb->local_port;
}

IPAddress AsyncClient::remoteIP() {
  return IPAddress(getRemoteAddress());
}

uint16_t AsyncClient::remotePort() {
  return getRemotePort();
}

IPAddress AsyncClient::localIP() {
  return IPAddress(getLocalAddress());
}

uint16_t AsyncClient::localPort() {
  return getLocalPort();
}

#if ASYNC_TCP_SSL_ENABLED
SSL * AsyncClient::getSSL(){
  if(_pcb && _pcb_secure){
    return tcp_ssl_get_ssl(_pcb);
  }
  return NULL;
}
#endif

uint8_t AsyncClient::state() {
  if(!_pcb)
    return 0;
  return _pcb->state;
}

bool AsyncClient::connected(){
  if (!_pcb)
    return false;
#if ASYNC_TCP_SSL_ENABLED
  return _pcb->state == 4 && _handshake_done;
#else
  return _pcb->state == 4;
#endif
}

bool AsyncClient::connecting(){
  if (!_pcb)
    return false;
  return _pcb->state > 0 && _pcb->state < 4;
}

bool AsyncClient::disconnecting(){
  if (!_pcb)
    return false;
  return _pcb->state > 4 && _pcb->state < 10;
}

bool AsyncClient::disconnected(){
  if (!_pcb)
    return true;
  return _pcb->state == 0 || _pcb->state == 10;
}

bool AsyncClient::freeable(){
  if (!_pcb)
    return true;
  return _pcb->state == 0 || _pcb->state > 4;
}

bool AsyncClient::canSend(){
  return !_pcb_busy && (space() > 0);
}


// Callback Setters

void AsyncClient::onConnect(AcConnectHandler cb, void* arg){
  _connect_cb = cb;
  _connect_cb_arg = arg;
}

void AsyncClient::onDisconnect(AcConnectHandler cb, void* arg){
  _discard_cb = cb;
  _discard_cb_arg = arg;
}

void AsyncClient::onAck(AcAckHandler cb, void* arg){
  _sent_cb = cb;
  _sent_cb_arg = arg;
}

void AsyncClient::onError(AcErrorHandler cb, void* arg){
  _error_cb = cb;
  _error_cb_arg = arg;
}

void AsyncClient::onData(AcDataHandler cb, void* arg){
  _recv_cb = cb;
  _recv_cb_arg = arg;
}

void AsyncClient::onPacket(AcPacketHandler cb, void* arg){
  _pb_cb = cb;
  _pb_cb_arg = arg;
}

void AsyncClient::onTimeout(AcTimeoutHandler cb, void* arg){
  _timeout_cb = cb;
  _timeout_cb_arg = arg;
}

void AsyncClient::onPoll(AcConnectHandler cb, void* arg){
  _poll_cb = cb;
  _poll_cb_arg = arg;
}


size_t AsyncClient::space(){
#if ASYNC_TCP_SSL_ENABLED
  if((_pcb != NULL) && (_pcb->state == 4) && _handshake_done){
    uint16_t s = tcp_sndbuf(_pcb);
    if(_pcb_secure){
#ifdef AXTLS_2_0_0_SNDBUF
      return tcp_ssl_sndbuf(_pcb);
#else
      if(s >= 128) //safe approach
        return s - 128;
      return 0;
#endif
    }
    return s;
  }
#else
  if((_pcb != NULL) && (_pcb->state == 4)){
    return tcp_sndbuf(_pcb);
  }
#endif
  return 0;
}

void AsyncClient::ackPacket(struct pbuf * pb){
  if(!pb){
    return;
  }
  tcp_recved(_pcb, pb->len);
  pbuf_free(pb);
}

const char * AsyncClient::errorToString(int8_t error) {
    switch (error) {
    case ERR_OK: 
		return "OK";
    case ERR_MEM: 
		return "Out of memory error";
    case ERR_BUF: 
		return "Buffer error";
    case ERR_TIMEOUT: 
		return "Timeout";
    case ERR_RTE: 
		return "Routing problem";
    case ERR_INPROGRESS: 
		return "Operation in progress";
    case ERR_VAL: 
		return "Illegal value";
    case ERR_WOULDBLOCK: 
		return "Operation would block";
    case ERR_ABRT:
		return "Connection aborted";
    case ERR_RST: 
		return "Connection reset";
    case ERR_CLSD: 
		return "Connection closed";
    case ERR_CONN: 
		return "Not connected";
    case ERR_ARG: 
		return "Illegal argument";
    case ERR_USE: 
		return "Address in use";
    case ERR_IF: 
		return "Low-level netif error";
    case ERR_ISCONN: 
		return "Connection already established";
    default: 
		return "Unknown error";
    }
}

const char * AsyncClient::stateToString(){
  switch(state()){
    case 0: return "Closed";
    case 1: return "Listen";
    case 2: return "SYN Sent";
    case 3: return "SYN Received";
    case 4: return "Established";
    case 5: return "FIN Wait 1";
    case 6: return "FIN Wait 2";
    case 7: return "Close Wait";
    case 8: return "Closing";
    case 9: return "Last ACK";
    case 10: return "Time Wait";
    default: return "UNKNOWN";
  }
}

/*
  Async TCP Server
*/
struct pending_pcb {
    tcp_pcb* pcb;
    pbuf *pb;
    struct pending_pcb * next;
};

AsyncServer::AsyncServer(IPAddress addr, uint16_t port)
  : _port(port)
  , _addr(addr)
  , _noDelay(false)
  , _pcb(0)
  , _connect_cb(0)
  , _connect_cb_arg(0)
#if ASYNC_TCP_SSL_ENABLED
  , _pending(NULL)
  , _ssl_ctx(NULL)
  , _file_cb(0)
  , _file_cb_arg(0)
#endif
{}

AsyncServer::AsyncServer(uint16_t port)
  : _port(port)
  , _addr((uint32_t) IPADDR_ANY)
  , _noDelay(false)
  , _pcb(0)
  , _connect_cb(0)
  , _connect_cb_arg(0)
#if ASYNC_TCP_SSL_ENABLED
  , _pending(NULL)
  , _ssl_ctx(NULL)
  , _file_cb(0)
  , _file_cb_arg(0)
#endif
{}

AsyncServer::~AsyncServer(){
  end();
}

void AsyncServer::onClient(AcConnectHandler cb, void* arg){
  _connect_cb = cb;
  _connect_cb_arg = arg;
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncServer::onSslFileRequest(AcSSlFileHandler cb, void* arg){
  _file_cb = cb;
  _file_cb_arg = arg;
}
#endif

void AsyncServer::begin(){
  if(_pcb)
    return;

  int8_t err;
  tcp_pcb* pcb = tcp_new();
  if (!pcb){
    return;
  }

  ip_addr_t local_addr;
  local_addr.addr = (uint32_t) _addr;
  err = tcp_bind(pcb, &local_addr, _port);

  if (err != ERR_OK) {
    tcp_close(pcb);
    return;
  }

  tcp_pcb* listen_pcb = tcp_listen(pcb);
  if (!listen_pcb) {
    tcp_close(pcb);
    return;
  }
  _pcb = listen_pcb;
  tcp_arg(_pcb, (void*) this);
  tcp_accept(_pcb, &_s_accept);
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncServer::beginSecure(const char *cert, const char *key, const char *password){
  if(_ssl_ctx){
    return;
  }
  tcp_ssl_file(_s_cert, this);
  _ssl_ctx = tcp_ssl_new_server_ctx(cert, key, password);
  if(_ssl_ctx){
    begin();
  }
}
#endif

void AsyncServer::end(){
  if(_pcb){
    //cleanup all connections?
    tcp_arg(_pcb, NULL);
    tcp_accept(_pcb, NULL);
    if(tcp_close(_pcb) != ERR_OK){
      tcp_abort(_pcb);
    }
    _pcb = NULL;
  }
#if ASYNC_TCP_SSL_ENABLED
  if(_ssl_ctx){
    ssl_ctx_free(_ssl_ctx);
    _ssl_ctx = NULL;
    if(_pending){
      struct pending_pcb * p;
      while(_pending){
        p = _pending;
        _pending = _pending->next;
        if(p->pb){
          pbuf_free(p->pb);
        }
        free(p);
      }
    }
  }
#endif
}

void AsyncServer::setNoDelay(bool nodelay){
  _noDelay = nodelay;
}

bool AsyncServer::getNoDelay(){
  return _noDelay;
}

uint8_t AsyncServer::status(){
  if (!_pcb)
    return 0;
  return _pcb->state;
}

err_t AsyncServer::_accept(tcp_pcb* pcb, err_t err){
  if(_connect_cb){
#if ASYNC_TCP_SSL_ENABLED
    if (_noDelay || _ssl_ctx)
#else
    if (_noDelay)
#endif
      tcp_nagle_disable(pcb);
    else
      tcp_nagle_enable(pcb);


#if ASYNC_TCP_SSL_ENABLED
    if(_ssl_ctx){
      if(tcp_ssl_has_client() || _pending){
        struct pending_pcb * new_item = (struct pending_pcb*)malloc(sizeof(struct pending_pcb));
        if(!new_item){
          ASYNC_TCP_DEBUG("### malloc new pending failed!\n");
          if(tcp_close(pcb) != ERR_OK){
            tcp_abort(pcb);
          }
          return ERR_OK;
        }
        ASYNC_TCP_DEBUG("### put to wait: %d\n", _clients_waiting);
        new_item->pcb = pcb;
        new_item->pb = NULL;
        new_item->next = NULL;
        tcp_setprio(_pcb, TCP_PRIO_MIN);
        tcp_arg(pcb, this);
        tcp_poll(pcb, &_s_poll, 1);
        tcp_recv(pcb, &_s_recv);

        if(_pending == NULL){
          _pending = new_item;
        } else {
          struct pending_pcb * p = _pending;
          while(p->next != NULL)
            p = p->next;
          p->next = new_item;
        }
      } else {
        AsyncClient *c = new AsyncClient(pcb, _ssl_ctx);
        if(c){
            c->onConnect([this](void * arg, AsyncClient *c){
              _connect_cb(_connect_cb_arg, c);
            }, this);
        }
      }
      return ERR_OK;
    } else {
      AsyncClient *c = new AsyncClient(pcb, NULL);
#else
      AsyncClient *c = new AsyncClient(pcb);
#endif
      if(c){
        _connect_cb(_connect_cb_arg, c);
        return ERR_OK;
      }
#if ASYNC_TCP_SSL_ENABLED
    }
#endif
  }
  if(tcp_close(pcb) != ERR_OK){
    tcp_abort(pcb);
  }
  return ERR_OK;
}

  err_t AsyncServer::_s_accept(void *arg, tcp_pcb* pcb, err_t err){
    return reinterpret_cast<AsyncServer*>(arg)->_accept(pcb, err);
  }

#if ASYNC_TCP_SSL_ENABLED
err_t AsyncServer::_poll(tcp_pcb* pcb){
  if(!tcp_ssl_has_client() && _pending){
    struct pending_pcb * p = _pending;
    if(p->pcb == pcb){
      _pending = _pending->next;
    } else {
      while(p->next && p->next->pcb != pcb) p = p->next;
      if(!p->next) return 0;
      struct pending_pcb * b = p->next;
      p->next = b->next;
      p = b;
    }
    ASYNC_TCP_DEBUG("### remove from wait: %d\n", _clients_waiting);
    AsyncClient *c = new AsyncClient(pcb, _ssl_ctx);
    if(c){
      c->onConnect([this](void * arg, AsyncClient *c){
        _connect_cb(_connect_cb_arg, c);
      }, this);
      if(p->pb)
        c->_recv(pcb, p->pb, 0);
    }
    free(p);
  }
  return ERR_OK;
}

err_t AsyncServer::_recv(struct tcp_pcb *pcb, struct pbuf *pb, err_t err){
  if(!_pending)
    return ERR_OK;

  struct pending_pcb * p;

  if(!pb){
    ASYNC_TCP_DEBUG("### close from wait: %d\n", _clients_waiting);
    p = _pending;
    if(p->pcb == pcb){
      _pending = _pending->next;
    } else {
      while(p->next && p->next->pcb != pcb) p = p->next;
      if(!p->next) return 0;
      struct pending_pcb * b = p->next;
      p->next = b->next;
      p = b;
    }
    if(p->pb){
      pbuf_free(p->pb);
    }
    free(p);
    tcp_close(pcb);
    tcp_abort(pcb);
  } else {
    ASYNC_TCP_DEBUG("### wait _recv: %u %d\n", pb->tot_len, _clients_waiting);
    p = _pending;
    while(p && p->pcb != pcb)
      p = p->next;
    if(p){
      if(p->pb){
        pbuf_chain(p->pb, pb);
      } else {
        p->pb = pb;
      }
    }
  }
  return ERR_OK;
}

int AsyncServer::_cert(const char *filename, uint8_t **buf){
  if(_file_cb){
    return _file_cb(_file_cb_arg, filename, buf);
  }
  *buf = 0;
  return 0;
}

int AsyncServer::_s_cert(void *arg, const char *filename, uint8_t **buf){
  return reinterpret_cast<AsyncServer*>(arg)->_cert(filename, buf);
}

err_t AsyncServer::_s_poll(void *arg, struct tcp_pcb *pcb){
  return reinterpret_cast<AsyncServer*>(arg)->_poll(pcb);
}

err_t AsyncServer::_s_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, err_t err){
  return reinterpret_cast<AsyncServer*>(arg)->_recv(pcb, pb, err);
}
#endif
#endif