// The MIT License (MIT)
//
// Copyright (c) 2015 Adafruit Industries
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef _ADAFRUIT_MQTT_CLIENT_H_
#define _ADAFRUIT_MQTT_CLIENT_H_

#include <Client.h>
#include "Adafruit_MQTT.h"


// How long to delay waiting for new data to be available in readPacket.
#define MQTT_CLIENT_READINTERVAL_MS 10


// MQTT client implementation for a generic Arduino Client interface.  Can work
// with almost all Arduino network hardware like ethernet shield, wifi shield,
// and even other platforms like ESP8266.
class Adafruit_MQTT_Client : public Adafruit_MQTT {
 public:
  Adafruit_MQTT_Client(Client *client, const char *server, uint16_t port,
                       const char *cid, const char *user, const char *pass):
    Adafruit_MQTT(server, port, cid, user, pass),
    client(client)
  {}

  Adafruit_MQTT_Client(Client *client, const char *server, uint16_t port,
                       const char *user="", const char *pass=""):
    Adafruit_MQTT(server, port, user, pass),
    client(client)
  {}

  bool connectServer();
  bool disconnectServer();
  bool connected();
  uint16_t readPacket(uint8_t *buffer, uint16_t maxlen, int16_t timeout);
  bool sendPacket(uint8_t *buffer, uint16_t len);

 private:
  Client* client;
};


#endif
