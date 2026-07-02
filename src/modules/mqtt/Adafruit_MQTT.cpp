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
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include "Adafruit_MQTT.h"

#if defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_SAMD_MKR1000) ||             \
    defined(ARDUINO_SAMD_MKR1010) || defined(ARDUINO_ARCH_SAMD)
static char *dtostrf(double val, signed char width, unsigned char prec,
                     char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}
#endif

#if defined(ESP8266)
int strncasecmp(const char *str1, const char *str2, int len) {
  int d = 0;
  while (len--) {
    int c1 = tolower(*str1++);
    int c2 = tolower(*str2++);
    if (((d = c1 - c2) != 0) || (c2 == '\0')) {
      return d;
    }
  }
  return 0;
}
#endif

void printBuffer(uint8_t *buffer, uint16_t len) {
  DEBUG_PRINTER.print('\t');
  for (uint16_t i = 0; i < len; i++) {
    if (isprint(buffer[i]))
      DEBUG_PRINTER.write(buffer[i]);
    else
      DEBUG_PRINTER.print(" ");
    DEBUG_PRINTER.print(F(" [0x"));
    if (buffer[i] < 0x10)
      DEBUG_PRINTER.print("0");
    DEBUG_PRINTER.print(buffer[i], HEX);
    DEBUG_PRINTER.print("], ");
    if (i % 8 == 7) {
      DEBUG_PRINTER.print("\n\t");
    }
  }
  DEBUG_PRINTER.println();
}

/* Not used now, but might be useful in the future
static uint8_t *stringprint(uint8_t *p, char *s) {
  uint16_t len = strlen(s);
  p[0] = len >> 8; p++;
  p[0] = len & 0xFF; p++;
  memmove(p, s, len);
  return p+len;
}
*/

static uint8_t *stringprint(uint8_t *p, const char *s, uint16_t maxlen = 0) {
  // If maxlen is specified (has a non-zero value) then use it as the maximum
  // length of the source string to write to the buffer.  Otherwise write
  // the entire source string.
  uint16_t len = strlen(s);
  if (maxlen > 0 && len > maxlen) {
    len = maxlen;
  }
  /*
  for (uint8_t i=0; i<len; i++) {
    Serial.write(pgm_read_byte(s+i));
  }
  */
  p[0] = len >> 8;
  p++;
  p[0] = len & 0xFF;
  p++;
  strncpy((char *)p, s, len);
  return p + len;
}

// packetAdditionalLen is a helper function used to figure out
// how bigger the payload needs to be in order to account for
// its variable length field. As per
// http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Table_2.4_Size
// See also readFullPacket
static uint16_t packetAdditionalLen(uint32_t currLen) {
  /* Increase length field based on current length */
  if (currLen < 128) // 7-bits
    return 0;
  if (currLen < 16384) // 14-bits
    return 1;
  if (currLen < 2097152) // 21-bits
    return 2;
  return 3;
}

// Adafruit_MQTT Definition ////////////////////////////////////////////////////

Adafruit_MQTT::Adafruit_MQTT(const char *server, uint16_t port, const char *cid,
                             const char *user, const char *pass) {
  servername = server;
  portnum = port;
  clientid = cid;
  username = user;
  password = pass;

  // reset subscriptions
  for (uint8_t i = 0; i < MAXSUBSCRIPTIONS; i++) {
    subscriptions[i] = 0;
  }

  will_topic = 0;
  will_payload = 0;
  will_qos = 0;
  will_retain = 0;

  packet_id_counter = 1; // MQTT spec forbids packet id of 0 if QOS=1
  keepAliveInterval = MQTT_CONN_KEEPALIVE;
}

Adafruit_MQTT::Adafruit_MQTT(const char *server, uint16_t port,
                             const char *user, const char *pass) {
  servername = server;
  portnum = port;
  clientid = "";
  username = user;
  password = pass;

  // reset subscriptions
  for (uint8_t i = 0; i < MAXSUBSCRIPTIONS; i++) {
    subscriptions[i] = 0;
  }

  will_topic = 0;
  will_payload = 0;
  will_qos = 0;
  will_retain = 0;

  packet_id_counter = 1; // MQTT spec forbids packet id of 0 if QOS=1
  keepAliveInterval = MQTT_CONN_KEEPALIVE;
}

int8_t Adafruit_MQTT::connect() {
  // Connect to the server.
  if (!connectServer())
    return -1;

  // Construct and send connect packet.
  uint8_t len = connectPacket(buffer);
  if (!sendPacket(buffer, len))
    return -1;

  // Read connect response packet and verify it
  len = readFullPacket(buffer, MAXBUFFERSIZE, CONNECT_TIMEOUT_MS);
  if (len != 4) {
    return -1;
  }
  if ((buffer[0] != (MQTT_CTRL_CONNECTACK << 4)) || (buffer[1] != 2)) {
    return -1;
  }
  if (buffer[3] != 0)
    return buffer[3];

  // Setup subscriptions once connected.
  for (uint8_t i = 0; i < MAXSUBSCRIPTIONS; i++) {
    // Ignore subscriptions that aren't defined.
    if (subscriptions[i] == 0)
      continue;

    boolean success = false;
    for (uint8_t retry = 0; (retry < 3) && !success;
         retry++) { // retry until we get a suback
      // Construct and send subscription packet.
      uint8_t len = subscribePacket(buffer, subscriptions[i]->topic,
                                    subscriptions[i]->qos);
      if (!sendPacket(buffer, len))
        return -1;

      if (MQTT_PROTOCOL_LEVEL < 3) // older versions didn't suback
        break;

      // Check for SUBACK if using MQTT 3.1.1 or higher
      // TODO: The Server is permitted to start sending PUBLISH packets matching
      // the Subscription before the Server sends the SUBACK Packet. (will
      // really need to use callbacks - ada)

      if (processPacketsUntil(buffer, MQTT_CTRL_SUBACK, SUBACK_TIMEOUT_MS)) {
        success = true;
        break;
      }
    }
    if (!success)
      return -2; // failed to sub for some reason
  }

  return 0;
}

int8_t Adafruit_MQTT::connect(const char *user, const char *pass) {
  username = user;
  password = pass;
  return connect();
}

void Adafruit_MQTT::processSubscriptionPacket(Adafruit_MQTT_Subscribe *sub) {
  if (sub->callback_uint32t != NULL) {
    // execute callback in integer mode
    uint32_t data = 0;
    data = atoi((char *)sub->lastread);
    sub->callback_uint32t(data);
  } else if (sub->callback_double != NULL) {
    // execute callback in doublefloat mode
    double data = 0;
    data = atof((char *)sub->lastread);
    sub->callback_double(data);
  } else if (sub->callback_buffer != NULL) {
    // execute callback in buffer mode
    sub->callback_buffer((char *)sub->lastread, sub->datalen);
  } else if (sub->callback_io != NULL) {
    // execute callback in io mode
    ((sub->io_mqtt)->*(sub->callback_io))((char *)sub->lastread, sub->datalen);
  } else {
    DEBUG_PRINTLN(
        "ERROR: Subscription packet did not have an associated callback");
    return;
  }
  // mark subscription message as "read""
  sub->new_message = false;
}

uint16_t Adafruit_MQTT::processPacketsUntil(uint8_t *buffer,
                                            uint8_t waitforpackettype,
                                            uint16_t timeout) {
  uint16_t len;

  while (true) {
    len = readFullPacket(buffer, MAXBUFFERSIZE, timeout);

    if (len == 0) {
      break;
    }

    uint8_t packetType = (buffer[0] >> 4);
    if (packetType == waitforpackettype) {
      return len;
    } else {
      if (packetType == MQTT_CTRL_PUBLISH) {
        Adafruit_MQTT_Subscribe *sub = handleSubscriptionPacket(len);
        if (sub)
          processSubscriptionPacket(sub);
      } else {
        ERROR_PRINTLN(F("Dropped a packet"));
      }
    }
  }
  return 0;
}

uint16_t Adafruit_MQTT::readFullPacket(uint8_t *buffer, uint16_t maxsize,
                                       uint16_t timeout) {
  // will read a packet and Do The Right Thing with length
  uint8_t *pbuff = buffer;

  uint16_t rlen;

  // read the packet type:
  rlen = readPacket(pbuff, 1, timeout);
  if (rlen != 1)
    return 0;

  DEBUG_PRINT(F("Packet Type:\t"));
  DEBUG_PRINTBUFFER(pbuff, rlen);
  pbuff++;

  uint32_t value = 0;
  uint32_t multiplier = 1;
  uint8_t encodedByte;

  do {
    rlen = readPacket(pbuff, 1, timeout);
    if (rlen != 1)
      return 0;
    encodedByte = pbuff[0]; // save the last read val
    pbuff++;                // get ready for reading the next byte
    uint32_t intermediate = encodedByte & 0x7F;
    intermediate *= multiplier;
    value += intermediate;
    multiplier *= 128;
    if (multiplier > (128UL * 128UL * 128UL)) {
      DEBUG_PRINT(F("Malformed packet len\n"));
      return 0;
    }
  } while (encodedByte & 0x80);

  DEBUG_PRINT(F("Packet Length:\t"));
  DEBUG_PRINTLN(value);

  // maxsize is limited to 65536 by 16-bit unsigned
  if (value > uint32_t(maxsize - (pbuff - buffer) - 1)) {
    DEBUG_PRINTLN(F("Packet too big for buffer"));
    rlen = readPacket(pbuff, (maxsize - (pbuff - buffer) - 1), timeout);
  } else {
    rlen = readPacket(pbuff, value, timeout);
  }
  // DEBUG_PRINT(F("Remaining packet:\t")); DEBUG_PRINTBUFFER(pbuff, rlen);

  return ((pbuff - buffer) + rlen);
}

const __FlashStringHelper *Adafruit_MQTT::connectErrorString(int8_t code) {
  switch (code) {
  case 1:
    return F(
        "The Server does not support the level of the MQTT protocol requested");
  case 2:
    return F(
        "The Client identifier is correct UTF-8 but not allowed by the Server");
  case 3:
    return F("The MQTT service is unavailable");
  case 4:
    return F("The data in the user name or password is malformed");
  case 5:
    return F("Not authorized to connect");
  case 6:
    return F("Exceeded reconnect rate limit. Please try again later.");
  case 7:
    return F("You have been banned from connecting. Please contact the MQTT "
             "server administrator for more details.");
  case -1:
    return F("Connection failed");
  case -2:
    return F("Failed to subscribe");
  default:
    return F("Unknown error");
  }
}

bool Adafruit_MQTT::disconnect() {

  // Construct and send disconnect packet.
  uint8_t len = disconnectPacket(buffer);
  if (!sendPacket(buffer, len))
    DEBUG_PRINTLN(F("Unable to send disconnect packet"));

  return disconnectServer();
}

bool Adafruit_MQTT::publish(const char *topic, const char *data, uint8_t qos,
                            bool retain) {
  return publish(topic, (uint8_t *)(data), strlen(data), qos, retain);
}

bool Adafruit_MQTT::publish(const char *topic, uint8_t *data, uint16_t bLen,
                            uint8_t qos, bool retain) {
  // Construct and send publish packet.
  uint16_t len = publishPacket(buffer, topic, data, bLen, qos,
                               (uint16_t)sizeof(buffer), retain);

  if (!sendPacket(buffer, len))
    return false;

  // If QOS level is high enough verify the response packet.
  if (qos > 0) {
    len = processPacketsUntil(buffer, MQTT_CTRL_PUBACK, PUBLISH_TIMEOUT_MS);

    DEBUG_PRINT(F("Publish QOS1+ reply:\t"));
    DEBUG_PRINTBUFFER(buffer, len);
    if (len != 4)
      return false;

    uint16_t packnum = buffer[2];
    packnum <<= 8;
    packnum |= buffer[3];

    // we increment the packet_id_counter right after publishing so inc here too
    // to match
    packnum = packnum + 1 + (packnum + 1 == 0); // Skip zero
    if (packnum != packet_id_counter)
      return false;
  }

  return true;
}

bool Adafruit_MQTT::will(const char *topic, const char *payload, uint8_t qos,
                         uint8_t retain) {

  if (connected()) {
    DEBUG_PRINT(F("Will defined after connect"));
    return false;
  }

  will_topic = topic;
  will_payload = payload;
  will_qos = qos;
  will_retain = retain;

  return true;
}

/***************************************************************************/
/*!
    @brief  Sets the connect packet's KeepAlive Interval, in seconds. This
            function MUST be called prior to connect().
    @param    keepAlive
               Maximum amount of time without communication between the
               client and the MQTT broker, in seconds.
    @returns  True if called prior to connect(), False otherwise.
*/
/***************************************************************************/
bool Adafruit_MQTT::setKeepAliveInterval(uint16_t keepAlive) {
  if (connected()) {
    DEBUG_PRINT(F("keepAlive defined after connection established."));
    return false;
  }
  keepAliveInterval = keepAlive;
  return true;
}

bool Adafruit_MQTT::subscribe(Adafruit_MQTT_Subscribe *sub) {
  uint8_t i;
  // see if we are already subscribed
  for (i = 0; i < MAXSUBSCRIPTIONS; i++) {
    if (subscriptions[i] == sub) {
      DEBUG_PRINTLN(F("Already subscribed"));
      return true;
    }
  }
  if (i == MAXSUBSCRIPTIONS) { // add to subscriptionlist
    for (i = 0; i < MAXSUBSCRIPTIONS; i++) {
      if (subscriptions[i] == 0) {
        DEBUG_PRINT(F("Added sub "));
        DEBUG_PRINTLN(i);
        subscriptions[i] = sub;
        return true;
      }
    }
  }

  DEBUG_PRINTLN(F("no more subscription space :("));
  return false;
}

int8_t Adafruit_MQTT::subscribePacketWrite(const char *topic, uint8_t qos) {
  if (connected()) {
    bool success = false;
    for (uint8_t retry = 0; (retry < 3) && !success; retry++) {
      uint8_t len = subscribePacket(buffer, topic, qos);
      if (!sendPacket(buffer, len))
        return -1;

      if (MQTT_PROTOCOL_LEVEL < 3)
        break;

      if (processPacketsUntil(buffer, MQTT_CTRL_SUBACK, SUBACK_TIMEOUT_MS)) {
        success = true;
        break;
      }
    }
    if (!success)
      return -2;
  }

  return 0;
}

bool Adafruit_MQTT::subscribeTopic(const char *topic) {
  int8_t rc = subscribePacketWrite(topic);

  if (rc == 0) {
    DEBUG_PRINT(F("subscribeTopic: "));
    DEBUG_PRINT(topic);
    DEBUG_PRINTLN(F(" ,success!"));
    return true;
  }

  DEBUG_PRINT(F("subscribeTopic: "));
  DEBUG_PRINT(topic);
  DEBUG_PRINTLN(F(" ,failed!"));
  return false;
}

bool Adafruit_MQTT::unsubscribe(Adafruit_MQTT_Subscribe *sub) {
  uint8_t i;

  // see if we are already subscribed
  for (i = 0; i < MAXSUBSCRIPTIONS; i++) {

    if (subscriptions[i] == sub) {

      DEBUG_PRINTLN(
          F("Found matching subscription and attempting to unsubscribe."));

      // Construct and send unsubscribe packet.
      uint8_t len = unsubscribePacket(buffer, subscriptions[i]->topic);

      // sending unsubscribe failed
      if (!sendPacket(buffer, len))
        return false;

      // if QoS for this subscription is 1 or 2, we need
      // to wait for the unsuback to confirm unsubscription
      if (subscriptions[i]->qos > 0 && MQTT_PROTOCOL_LEVEL > 3) {

        // wait for UNSUBACK
        len = readFullPacket(buffer, MAXBUFFERSIZE, CONNECT_TIMEOUT_MS);
        DEBUG_PRINT(F("UNSUBACK:\t"));
        DEBUG_PRINTBUFFER(buffer, len);

        if ((len != 5) || (buffer[0] != (MQTT_CTRL_UNSUBACK << 4))) {
          return false; // failure to unsubscribe
        }
      }

      subscriptions[i] = 0;
      return true;
    }
  }

  // subscription not found, so we are unsubscribed
  return true;
}

void Adafruit_MQTT::processPackets(int16_t timeout) {

  uint32_t elapsed = 0, endtime, starttime = millis();

  while (elapsed < (uint32_t)timeout) {
    Adafruit_MQTT_Subscribe *sub = readSubscription(timeout - elapsed);
    if (sub)
      processSubscriptionPacket(sub);
    // keep track over elapsed time
    endtime = millis();
    if (endtime < starttime) {
      starttime = endtime; // looped around!")
    }
    elapsed += (endtime - starttime);
  }
}
Adafruit_MQTT_Subscribe *Adafruit_MQTT::readSubscription(int16_t timeout) {

  // Sync or Async subscriber with message
  Adafruit_MQTT_Subscribe *s = 0;

  // Check if are unread messages
  for (uint8_t i = 0; i < MAXSUBSCRIPTIONS; i++) {
    if (subscriptions[i] && subscriptions[i]->new_message) {
      s = subscriptions[i];
      break;
    }
  }

  // not unread message
  if (!s) {
    // Check if data is available to read.
    uint16_t len = readFullPacket(buffer, MAXBUFFERSIZE,
                                  timeout); // return one full packet
    s = handleSubscriptionPacket(len);
  }

  // it there is a message, mark it as not pending
  if (s) {
    s->new_message = false;
  }

  return s;
}

Adafruit_MQTT_Subscribe *Adafruit_MQTT::handleSubscriptionPacket(uint16_t len) {
  uint16_t i, topiclen, datalen;

  if (!len) {
    return NULL; // No data available, just quit.
  }
  DEBUG_PRINT("Packet len: ");
  DEBUG_PRINTLN(len);
  DEBUG_PRINTBUFFER(buffer, len);

  if (len < 3) {
    return NULL;
  }
  if ((buffer[0] & 0xF0) != (MQTT_CTRL_PUBLISH) << 4) {
    return NULL;
  }

  // Parse MQTT Remaining Length field (variable length encoding).
  // This field can be 1-4 bytes, so we must parse it byte-by-byte.
  // Each byte uses the lower 7 bits for data, and the MSB as a continuation
  // flag. This loop advances 'offset' to the first byte after the remaining
  // length field.
  uint32_t multiplier = 1;
  uint32_t value = 0;
  uint8_t encodedByte;
  uint8_t offset = 1; // Start after the fixed header byte
  do {
    encodedByte = buffer[offset++];
    value += (encodedByte & 127) * multiplier;
    multiplier *= 128;
    // Infinite loop protection: MQTT spec allows max 4 bytes for this field
    if (offset > 5) { // 1 (header) + 4 (max length bytes)
      DEBUG_PRINTLN(
          F("Bad MQTT packet: Remaining Length field too long / malformed"));
      return NULL;
    }
  } while ((encodedByte & 128) != 0);

  // Now 'offset' points to the first byte of the topic length field.
  // The topic length is always 2 bytes, big-endian.
  uint16_t new_msb = buffer[offset] << 8;
  uint16_t new_lsb = buffer[offset + 1];
  topiclen = new_msb | new_lsb;
  uint16_t topicstart = offset + 2;

  // Find subscription associated with this packet.
  for (i = 0; i < MAXSUBSCRIPTIONS; i++) {
    if (subscriptions[i]) {
      // Skip this subscription if its name length isn't the same as the
      // received topic name.
      if (strlen(subscriptions[i]->topic) != topiclen)
        continue;
      // Stop if the subscription topic matches the received topic. Be careful
      // to make comparison case insensitive.
      if (strncasecmp((char *)buffer + topicstart, subscriptions[i]->topic,
                      topiclen) == 0) {
        DEBUG_PRINT(F("Found sub #"));
        DEBUG_PRINTLN(i);
        if (subscriptions[i]->new_message) {
          DEBUG_PRINTLN(F("Lost previous message"));
        } else {
          subscriptions[i]->new_message = true;
        }

        break;
      }
    }
  }
  uint8_t packet_id_len = 0;
  uint16_t packetid = 0;
  // Check if it is QoS 1, TODO: we dont support QoS 2
  if ((buffer[0] & 0x6) == 0x2) {
    packet_id_len = 2;
    packetid = buffer[topiclen + topicstart];
    packetid <<= 8;
    packetid |= buffer[topiclen + topicstart + 1];
  }

  if (topicstart + topiclen + packet_id_len > len) {
    DEBUG_PRINTLN(F("Bad MQTT packet: topic/payload offset out of range"));
    return NULL;
  }

  if (i == MAXSUBSCRIPTIONS) {
    if (topiclen >= sizeof(extra_topic)) {
      DEBUG_PRINTLN(F("Unknown MQTT topic too long"));
      return NULL;
    }

    memset(extra_topic, 0, sizeof(extra_topic));
    memset(extra_data, 0, SUBSCRIPTIONDATALEN);

    datalen = len - topiclen - packet_id_len - topicstart;
    if (datalen > SUBSCRIPTIONDATALEN) {
      datalen = SUBSCRIPTIONDATALEN - 1;
    }

    memmove(extra_topic, buffer + topicstart, topiclen);
    memmove(extra_data, buffer + topicstart + topiclen + packet_id_len,
            datalen);
    get_extra = true;

    DEBUG_PRINT(F("Extra data len: "));
    DEBUG_PRINTLN(datalen);
    DEBUG_PRINT(F("Extra data: "));
    DEBUG_PRINTLN((char *)extra_data);
    DEBUG_PRINT(F("Extra topic: "));
    DEBUG_PRINTLN(extra_topic);

    if ((MQTT_PROTOCOL_LEVEL > 3) && (buffer[0] & 0x6) == 0x2) {
      uint8_t ackpacket[4];
      uint8_t acklen = pubackPacket(ackpacket, packetid);
      if (!sendPacket(ackpacket, acklen))
        DEBUG_PRINT(F("Failed"));
    }

    return NULL;
  }

  // zero out the old data
  memset(subscriptions[i]->lastread, 0, SUBSCRIPTIONDATALEN);

  datalen = len - topiclen - packet_id_len - topicstart;
  if (datalen > SUBSCRIPTIONDATALEN) {
    datalen = SUBSCRIPTIONDATALEN - 1; // cut it off
  }
  // extract out just the data, into the subscription object itself
  memmove(subscriptions[i]->lastread,
          buffer + topicstart + topiclen + packet_id_len, datalen);
  subscriptions[i]->datalen = datalen;
  DEBUG_PRINT(F("Data len: "));
  DEBUG_PRINTLN(datalen);
  DEBUG_PRINT(F("Data: "));
  DEBUG_PRINTLN((char *)subscriptions[i]->lastread);

  if ((MQTT_PROTOCOL_LEVEL > 3) && (buffer[0] & 0x6) == 0x2) {
    uint8_t ackpacket[4];

    // Construct and send puback packet.
    uint8_t len = pubackPacket(ackpacket, packetid);
    if (!sendPacket(ackpacket, len))
      DEBUG_PRINT(F("Failed"));
  }

  // return the valid matching subscription
  return subscriptions[i];
}

void Adafruit_MQTT::flushIncoming(uint16_t timeout) {
  // flush input!
  DEBUG_PRINTLN(F("Flushing input buffer"));
  while (readPacket(buffer, MAXBUFFERSIZE, timeout))
    ;
}

bool Adafruit_MQTT::ping(uint8_t num) {
  // flushIncoming(100);

  while (num--) {
    // Construct and send ping packet.
    uint8_t len = pingPacket(buffer);
    if (!sendPacket(buffer, len))
      continue;

    // Process ping reply.
    len = processPacketsUntil(buffer, MQTT_CTRL_PINGRESP, PING_TIMEOUT_MS);
    if (buffer[0] == (MQTT_CTRL_PINGRESP << 4))
      return true;
  }

  return false;
}

// Packet Generation Functions /////////////////////////////////////////////////

// The current MQTT spec is 3.1.1 and available here:
//   http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718028
// However this connect packet and code follows the MQTT 3.1 spec here (some
// small differences in the protocol):
//   http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html#connect
uint8_t Adafruit_MQTT::connectPacket(uint8_t *packet) {
  uint8_t *p = packet;
  uint16_t len;

  // fixed header, connection messsage no flags
  p[0] = (MQTT_CTRL_CONNECT << 4) | 0x0;
  p += 2;
  // fill in packet[1] last

#if MQTT_PROTOCOL_LEVEL == 3
  p = stringprint(p, "MQIsdp");
#elif MQTT_PROTOCOL_LEVEL == 4
  p = stringprint(p, "MQTT");
#else
#error "MQTT level not supported"
#endif

  p[0] = MQTT_PROTOCOL_LEVEL;
  p++;

  // always clean the session
  p[0] = MQTT_CONN_CLEANSESSION;

  // set the will flags if needed
  if (will_topic && pgm_read_byte(will_topic) != 0) {

    p[0] |= MQTT_CONN_WILLFLAG;

    if (will_qos == 1)
      p[0] |= MQTT_CONN_WILLQOS_1;
    else if (will_qos == 2)
      p[0] |= MQTT_CONN_WILLQOS_2;

    if (will_retain == 1)
      p[0] |= MQTT_CONN_WILLRETAIN;
  }

  if (pgm_read_byte(username) != 0)
    p[0] |= MQTT_CONN_USERNAMEFLAG;
  if (pgm_read_byte(password) != 0)
    p[0] |= MQTT_CONN_PASSWORDFLAG;
  p++;

  p[0] = keepAliveInterval >> 8;
  p++;
  p[0] = keepAliveInterval & 0xFF;
  p++;

  if (MQTT_PROTOCOL_LEVEL == 3) {
    p = stringprint(p, clientid, 23); // Limit client ID to first 23 characters.
  } else {
    if (pgm_read_byte(clientid) != 0) {
      p = stringprint(p, clientid);
    } else {
      p[0] = 0x0;
      p++;
      p[0] = 0x0;
      p++;
      DEBUG_PRINTLN(F("SERVER GENERATING CLIENT ID"));
    }
  }

  if (will_topic && pgm_read_byte(will_topic) != 0) {
    p = stringprint(p, will_topic);
    p = stringprint(p, will_payload);
  }

  if (pgm_read_byte(username) != 0) {
    p = stringprint(p, username);
  }
  if (pgm_read_byte(password) != 0) {
    p = stringprint(p, password);
  }

  len = p - packet;

  packet[1] = len - 2; // don't include the 2 bytes of fixed header data
  DEBUG_PRINTLN(F("MQTT connect packet:"));
  DEBUG_PRINTBUFFER(buffer, len);
  return len;
}

// as per
// http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718040
uint16_t Adafruit_MQTT::publishPacket(uint8_t *packet, const char *topic,
                                      uint8_t *data, uint16_t bLen, uint8_t qos,
                                      uint16_t maxPacketLen, bool retain) {
  uint8_t *p = packet;
  uint16_t len = 0;

  // calc length of non-header data
  len += 2;             // two bytes to set the topic size
  len += strlen(topic); // topic length
  if (qos > 0) {
    len += 2; // qos packet id
  }
  // Calculate additional bytes for length field (if any)
  uint16_t additionalLen = packetAdditionalLen(len + bLen);

  // Payload remaining length. When maxPacketLen provided is 0, let's
  // assume buffer is big enough. Fingers crossed.
  // 2 + additionalLen: header byte + remaining length field (from 1 to 4 bytes)
  // len = topic size field + value (string)
  // bLen = buffer size
  if (!(maxPacketLen == 0 ||
        (len + bLen + 2 + additionalLen <= maxPacketLen))) {
    // If we make it here, we got a pickle: the payload is not going
    // to fit in the packet buffer. Instead of corrupting memory, let's
    // do something less damaging by reducing the bLen to what we are
    // able to accomodate. Alternatively, consider using a bigger
    // maxPacketLen.
    bLen = maxPacketLen - (len + 2 + packetAdditionalLen(maxPacketLen));
  }
  len += bLen; // remaining len excludes header byte & length field

  // Now you can start generating the packet!
  p[0] = MQTT_CTRL_PUBLISH << 4 | qos << 1 | (retain ? 1 : 0);
  p++;

  // fill in packet[1] last
  do {
    uint8_t encodedByte = len % 128;
    len /= 128;
    // if there are more data to encode, set the top bit of this byte
    if (len > 0) {
      encodedByte |= 0x80;
    }
    p[0] = encodedByte;
    p++;
  } while (len > 0);

  // topic comes before packet identifier
  p = stringprint(p, topic);

  // add packet identifier. used for checking PUBACK in QOS > 0
  if (qos > 0) {
    p[0] = (packet_id_counter >> 8) & 0xFF;
    p[1] = packet_id_counter & 0xFF;
    p += 2;

    // increment the packet id, skipping 0
    packet_id_counter = packet_id_counter + 1 + (packet_id_counter + 1 == 0);
  }

  memmove(p, data, bLen);
  p += bLen;
  len = p - packet;
  DEBUG_PRINTLN(F("MQTT publish packet:"));
  DEBUG_PRINTBUFFER(buffer, len);
  return len;
}

uint8_t Adafruit_MQTT::subscribePacket(uint8_t *packet, const char *topic,
                                       uint8_t qos) {
  uint8_t *p = packet;
  uint16_t len;

  p[0] = MQTT_CTRL_SUBSCRIBE << 4 | MQTT_QOS_1 << 1;
  // fill in packet[1] last
  p += 2;

  // packet identifier. used for checking SUBACK
  p[0] = (packet_id_counter >> 8) & 0xFF;
  p[1] = packet_id_counter & 0xFF;
  p += 2;

  // increment the packet id, skipping 0
  packet_id_counter = packet_id_counter + 1 + (packet_id_counter + 1 == 0);

  p = stringprint(p, topic);

  p[0] = qos;
  p++;

  len = p - packet;
  packet[1] = len - 2; // don't include the 2 bytes of fixed header data
  DEBUG_PRINTLN(F("MQTT subscription packet:"));
  DEBUG_PRINTBUFFER(buffer, len);
  return len;
}

uint8_t Adafruit_MQTT::unsubscribePacket(uint8_t *packet, const char *topic) {

  uint8_t *p = packet;
  uint16_t len;

  p[0] = MQTT_CTRL_UNSUBSCRIBE << 4 | 0x1;
  // fill in packet[1] last
  p += 2;

  // packet identifier. used for checking UNSUBACK
  p[0] = (packet_id_counter >> 8) & 0xFF;
  p[1] = packet_id_counter & 0xFF;
  p += 2;

  // increment the packet id, skipping 0
  packet_id_counter = packet_id_counter + 1 + (packet_id_counter + 1 == 0);

  p = stringprint(p, topic);

  len = p - packet;
  packet[1] = len - 2; // don't include the 2 bytes of fixed header data
  DEBUG_PRINTLN(F("MQTT unsubscription packet:"));
  DEBUG_PRINTBUFFER(buffer, len);
  return len;
}

uint8_t Adafruit_MQTT::pingPacket(uint8_t *packet) {
  packet[0] = MQTT_CTRL_PINGREQ << 4;
  packet[1] = 0;
  DEBUG_PRINTLN(F("MQTT ping packet:"));
  DEBUG_PRINTBUFFER(buffer, 2);
  return 2;
}

uint8_t Adafruit_MQTT::pubackPacket(uint8_t *packet, uint16_t packetid) {
  packet[0] = MQTT_CTRL_PUBACK << 4;
  packet[1] = 2;
  packet[2] = packetid >> 8;
  packet[3] = packetid;
  DEBUG_PRINTLN(F("MQTT puback packet:"));
  DEBUG_PRINTBUFFER(buffer, 4);
  return 4;
}

uint8_t Adafruit_MQTT::disconnectPacket(uint8_t *packet) {
  packet[0] = MQTT_CTRL_DISCONNECT << 4;
  packet[1] = 0;
  DEBUG_PRINTLN(F("MQTT disconnect packet:"));
  DEBUG_PRINTBUFFER(buffer, 2);
  return 2;
}

// Adafruit_MQTT_Publish Definition ////////////////////////////////////////////

Adafruit_MQTT_Publish::Adafruit_MQTT_Publish(Adafruit_MQTT *mqttserver,
                                             const char *feed, uint8_t q) {
  mqtt = mqttserver;
  topic = feed;
  qos = q;
}

bool Adafruit_MQTT_Publish::publish(int32_t i, bool retain) {
  char payload[12];
  ltoa(i, payload, 10);
  return mqtt->publish(topic, payload, qos, retain);
}

bool Adafruit_MQTT_Publish::publish(uint32_t i, bool retain) {
  char payload[11];
  ultoa(i, payload, 10);
  return mqtt->publish(topic, payload, qos, retain);
}

bool Adafruit_MQTT_Publish::publish(double f, uint8_t precision, bool retain) {
  char payload[41]; // Need to technically hold float max, 39 digits and minus
                    // sign.
  dtostrf(f, 0, precision, payload);
  return mqtt->publish(topic, payload, qos, retain);
}

bool Adafruit_MQTT_Publish::publish(const char *payload, bool retain) {
  return mqtt->publish(topic, payload, qos, retain);
}

// publish buffer of arbitrary length
bool Adafruit_MQTT_Publish::publish(uint8_t *payload, uint16_t bLen,
                                    bool retain) {
  return mqtt->publish(topic, payload, bLen, qos, retain);
}

// Adafruit_MQTT_Subscribe Definition //////////////////////////////////////////

Adafruit_MQTT_Subscribe::Adafruit_MQTT_Subscribe(Adafruit_MQTT *mqttserver,
                                                 const char *feed, uint8_t q) {
  mqtt = mqttserver;
  topic = feed;
  qos = q;
  datalen = 0;
  callback_uint32t = 0;
  callback_buffer = 0;
  callback_double = 0;
  callback_io = 0;
  io_mqtt = 0;
  new_message = false;
}

void Adafruit_MQTT_Subscribe::setCallback(SubscribeCallbackUInt32Type cb) {
  callback_uint32t = cb;
}

void Adafruit_MQTT_Subscribe::setCallback(SubscribeCallbackDoubleType cb) {
  callback_double = cb;
}

void Adafruit_MQTT_Subscribe::setCallback(SubscribeCallbackBufferType cb) {
  callback_buffer = cb;
}

void Adafruit_MQTT_Subscribe::setCallback(AdafruitIO_MQTT *io,
                                          SubscribeCallbackIOType cb) {
  callback_io = cb;
  io_mqtt = io;
}

void Adafruit_MQTT_Subscribe::removeCallback(void) {
  callback_uint32t = 0;
  callback_buffer = 0;
  callback_double = 0;
  callback_io = 0;
  io_mqtt = 0;
}
