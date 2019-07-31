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
#include "SyncClient.h"
#include <Arduino.h>
#include "ESPAsyncTCP.h"
#include <cbuf.h>


SyncClient::SyncClient(size_t txBufLen)
  : _client(NULL)
  , _tx_buffer(NULL)
  , _tx_buffer_size(txBufLen)
  , _rx_buffer(NULL)
{}

SyncClient::SyncClient(AsyncClient *client, size_t txBufLen)
  : _client(client)
  , _tx_buffer(new cbuf(txBufLen))
  , _tx_buffer_size(txBufLen)
  , _rx_buffer(NULL)
{
  _attachCallbacks();
}

SyncClient::~SyncClient(){
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  while(_rx_buffer != NULL){
    cbuf *b = _rx_buffer;
    _rx_buffer = _rx_buffer->next;
    delete b;
  }
}

#if ASYNC_TCP_SSL_ENABLED
int SyncClient::connect(IPAddress ip, uint16_t port, bool secure){
#else
int SyncClient::connect(IPAddress ip, uint16_t port){
#endif
  if(_client != NULL && connected())
    return 0;
  _client = new AsyncClient();
  _client->onConnect([](void *obj, AsyncClient *c){ ((SyncClient*)(obj))->_onConnect(c); }, this);
  _attachCallbacks_Disconnect();
#if ASYNC_TCP_SSL_ENABLED
  if(_client->connect(ip, port, secure)){
#else
  if(_client->connect(ip, port)){
#endif
    while(_client != NULL && !_client->connected() && !_client->disconnecting())
      delay(1);
    return connected();
  }
  return 0;
}

#if ASYNC_TCP_SSL_ENABLED
int SyncClient::connect(const char *host, uint16_t port, bool secure){
#else
int SyncClient::connect(const char *host, uint16_t port){
#endif
  if(_client != NULL && connected()){
    return 0;
  }
  _client = new AsyncClient();
  _client->onConnect([](void *obj, AsyncClient *c){ ((SyncClient*)(obj))->_onConnect(c); }, this);
  _attachCallbacks_Disconnect();
#if ASYNC_TCP_SSL_ENABLED
  if(_client->connect(host, port, secure)){
#else
  if(_client->connect(host, port)){
#endif
    while(_client != NULL && !_client->connected() && !_client->disconnecting())
      delay(1);
    return connected();
  }
  return 0;
}

SyncClient & SyncClient::operator=(const SyncClient &other){
  if(_client != NULL){
    _client->abort();
    _client->free();
    _client = NULL;
  }
  _tx_buffer_size = other._tx_buffer_size;
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  while(_rx_buffer != NULL){
    cbuf *b = _rx_buffer;
    _rx_buffer = b->next;
    delete b;
  }
  _tx_buffer = new cbuf(other._tx_buffer_size);
  _client = other._client;
  _attachCallbacks();
  return *this;
}

void SyncClient::setTimeout(uint32_t seconds){
  if(_client != NULL)
    _client->setRxTimeout(seconds);
}

uint8_t SyncClient::status(){
  if(_client == NULL)
    return 0;
  return _client->state();
}

uint8_t SyncClient::connected(){
  return (_client != NULL && _client->connected());
}

void SyncClient::stop(){
  if(_client != NULL)
    _client->close(true);
}

size_t SyncClient::_sendBuffer(){
  size_t available = _tx_buffer->available();
  if(!connected() || !_client->canSend() || available == 0)
    return 0;
  size_t sendable = _client->space();
  if(sendable < available)
    available= sendable;
  char *out = new char[available];
  _tx_buffer->read(out, available);
  size_t sent = _client->write(out, available);
  delete[] out;
  return sent;
}

void SyncClient::_onData(void *data, size_t len){
  _client->ackLater();
  cbuf *b = new cbuf(len+1);
  if(b != NULL){
    b->write((const char *)data, len);
    if(_rx_buffer == NULL)
      _rx_buffer = b;
    else {
      cbuf *p = _rx_buffer;
      while(p->next != NULL)
        p = p->next;
      p->next = b;
    }
  }
}

void SyncClient::_onDisconnect(){
  if(_client != NULL){
    _client = NULL;
  }
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
}

void SyncClient::_onConnect(AsyncClient *c){
  _client = c;
  if(_tx_buffer != NULL){
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  _tx_buffer = new cbuf(_tx_buffer_size);
  _attachCallbacks_AfterConnected();
}

void SyncClient::_attachCallbacks(){
  _attachCallbacks_Disconnect();
  _attachCallbacks_AfterConnected();
}

void SyncClient::_attachCallbacks_AfterConnected(){
  _client->onAck([](void *obj, AsyncClient* c, size_t len, uint32_t time){ ((SyncClient*)(obj))->_sendBuffer(); }, this);
  _client->onData([](void *obj, AsyncClient* c, void *data, size_t len){ ((SyncClient*)(obj))->_onData(data, len); }, this);
  _client->onTimeout([](void *obj, AsyncClient* c, uint32_t time){ c->close(); }, this);
}

void SyncClient::_attachCallbacks_Disconnect(){
  _client->onDisconnect([](void *obj, AsyncClient* c){ ((SyncClient*)(obj))->_onDisconnect(); delete c; }, this);
}

size_t SyncClient::write(uint8_t data){
  return write(&data, 1);
}

size_t SyncClient::write(const uint8_t *data, size_t len){
  if(_tx_buffer == NULL || !connected()){
    return 0;
  }
  size_t toWrite = 0;
  size_t toSend = len;
  while(_tx_buffer->room() < toSend){
    toWrite = _tx_buffer->room();
    _tx_buffer->write((const char*)data, toWrite);
    while(!_client->canSend() && connected())
      delay(0);
    _sendBuffer();
    toSend -= toWrite;
  }
  _tx_buffer->write((const char*)(data+(len - toSend)), toSend);
  if(_client->canSend() && connected())
    _sendBuffer();
  return len;
}

int SyncClient::available(){
  if(_rx_buffer == NULL) return 0;
  size_t a = 0;
  cbuf *b = _rx_buffer;
  while(b != NULL){
    a += b->available();
    b = b->next;
  }
  return a;
}

int SyncClient::peek(){
  if(_rx_buffer == NULL) return -1;
  return _rx_buffer->peek();
}

int SyncClient::read(uint8_t *data, size_t len){
  if(_rx_buffer == NULL) return -1;

  size_t readSoFar = 0;
  while(_rx_buffer != NULL && (len - readSoFar) >= _rx_buffer->available()){
    cbuf *b = _rx_buffer;
    _rx_buffer = _rx_buffer->next;
    size_t toRead = b->available();
    readSoFar += b->read((char*)(data+readSoFar), toRead);
    if(connected()){
        _client->ack(b->size() - 1);
    }
    delete b;
  }
  if(_rx_buffer != NULL && readSoFar < len){
    readSoFar += _rx_buffer->read((char*)(data+readSoFar), (len - readSoFar));
  }
  return readSoFar;
}

int SyncClient::read(){
  uint8_t res = 0;
  if(read(&res, 1) != 1)
    return -1;
  return res;
}

void SyncClient::flush(){
  if(_tx_buffer == NULL || !connected())
    return;
  if(_tx_buffer->available()){
    while(!_client->canSend() && connected())
      delay(0);
    _sendBuffer();
  }
}
#endif
