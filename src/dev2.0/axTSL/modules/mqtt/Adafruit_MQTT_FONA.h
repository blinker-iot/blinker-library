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
#ifndef _ADAFRUIT_MQTT_FONA_H_
#define _ADAFRUIT_MQTT_FONA_H_

#include <Adafruit_FONA.h>
#include "Adafruit_MQTT.h"

#define MQTT_FONA_INTERAVAILDELAY 100
#define MQTT_FONA_QUERYDELAY 500


// FONA-specific version of the Adafruit_MQTT class.
// Note that this is defined as a header-only class to prevent issues with using
// the library on non-FONA platforms (since Arduino will include all .cpp files
// in the compilation of the library).
class Adafruit_MQTT_FONA : public Adafruit_MQTT {
 public:
  Adafruit_MQTT_FONA(Adafruit_FONA *f, const char *server, uint16_t port,
                       const char *cid, const char *user, const char *pass):
    Adafruit_MQTT(server, port, cid, user, pass),
    fona(f)
  {}

  Adafruit_MQTT_FONA(Adafruit_FONA *f, const char *server, uint16_t port,
                     const char *user="", const char *pass=""):
    Adafruit_MQTT(server, port, user, pass),
    fona(f)
  {}

  bool connectServer() {
    char server[40];
    strncpy(server, servername, 40);
#ifdef ADAFRUIT_SLEEPYDOG_H
    Watchdog.reset();
#endif

    // connect to server
    DEBUG_PRINTLN(F("Connecting to TCP"));
    return fona->TCPconnect(server, portnum);
  }

  bool disconnectServer() {
    return fona->TCPclose();
  }

  bool connected() {
    // Return true if connected, false if not connected.
    return fona->TCPconnected();
  }

  uint16_t readPacket(uint8_t *buffer, uint16_t maxlen, int16_t timeout) {
    uint8_t *buffp = buffer;
    DEBUG_PRINTLN(F("Reading data.."));

    if (!fona->TCPconnected()) return 0;


    /* Read data until either the connection is closed, or the idle timeout is reached. */
    uint16_t len = 0;
    int16_t t = timeout;
    uint16_t avail;

    while (fona->TCPconnected() && (timeout >= 0)) {
      //DEBUG_PRINT('.');
      while (avail = fona->TCPavailable()) {
        //DEBUG_PRINT('!');

        if (len + avail > maxlen) {
	  avail = maxlen - len;
	  if (avail == 0) return len;
        }

        // try to read the data into the end of the pointer
        if (! fona->TCPread(buffp, avail)) return len;

        // read it! advance pointer
        buffp += avail;
        len += avail;
        timeout = t;  // reset the timeout

        //DEBUG_PRINTLN((uint8_t)c, HEX);

        if (len == maxlen) {  // we read all we want, bail
          DEBUG_PRINT(F("Read:\t"));
          DEBUG_PRINTBUFFER(buffer, len);
	  return len;
        }
      }
#ifdef ADAFRUIT_SLEEPYDOG_H
      Watchdog.reset();
#endif
      timeout -= MQTT_FONA_INTERAVAILDELAY;
      timeout -= MQTT_FONA_QUERYDELAY; // this is how long it takes to query the FONA for avail()
      delay(MQTT_FONA_INTERAVAILDELAY);
    }
    
    return len;
  }

  bool sendPacket(uint8_t *buffer, uint16_t len) {
    DEBUG_PRINTLN(F("Writing packet"));
    if (fona->TCPconnected()) {
      boolean ret = fona->TCPsend((char *)buffer, len);
      //DEBUG_PRINT(F("sendPacket returned: ")); DEBUG_PRINTLN(ret);
      if (!ret) {
        DEBUG_PRINTLN("Failed to send packet.");
        return false;
      }
    } else {
      DEBUG_PRINTLN(F("Connection failed!"));
      return false;
    }
    return true;
  }

 private:
  uint32_t serverip;
  Adafruit_FONA *fona;
};


#endif
