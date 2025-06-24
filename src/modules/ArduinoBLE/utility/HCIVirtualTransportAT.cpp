/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2018 Arduino SA. All rights reserved.

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(ARDUINO_UNOR4_WIFI)

#include "HCIVirtualTransportAT.h"

extern ModemClass modem;

HCIVirtualTransportATClass::HCIVirtualTransportATClass()
{
}

HCIVirtualTransportATClass::~HCIVirtualTransportATClass()
{
}

static RingBufferN<258> buf;

int HCIVirtualTransportATClass::begin()
{
  // TODO: add this helper
  //modem.debug(Serial);
  buf.clear();
  //modem.debug(true);
  std::string res = "";
  modem.begin();
  if (modem.write(std::string(PROMPT(_HCI_BEGIN)), res, CMD(_HCI_BEGIN))) {
      return 1;
  }
  return 0;
}

void HCIVirtualTransportATClass::end()
{
}

void HCIVirtualTransportATClass::wait(unsigned long timeout)
{
  std::string res = "";
  modem.write(std::string(PROMPT(_HCI_WAIT)), res, "%d\n\r", CMD_WRITE(_HCI_WAIT), timeout);
}

int HCIVirtualTransportATClass::available()
{
  std::string res = "";
  if (buf.available()) {
    return buf.available();
  }
  if (modem.write(std::string(PROMPT(_HCI_AVAILABLE)), res, CMD_READ(_HCI_AVAILABLE))) {
    return atoi(res.c_str());
  }

  return 0;
}

// never called
int HCIVirtualTransportATClass::peek()
{
  return -1;
}

int HCIVirtualTransportATClass::read()
{
  uint8_t c;
  std::string res = "";
  if (buf.available()) {
    return buf.read_char();
  }
  modem.avoid_trim_results();
  modem.read_using_size();
  if (modem.write(std::string(PROMPT(_HCI_READ)), res, CMD(_HCI_READ))) {
    for(int i = 0; i < res.size(); i++) {
      buf.store_char((uint8_t)res[i]);
    }
    return buf.read_char();
  }

  return -1;
}

size_t HCIVirtualTransportATClass::write(const uint8_t* data, size_t length)
{
  std::string res = "";
  modem.write_nowait(std::string(PROMPT(_HCI_WRITE)), res, "%s%d\r\n" , CMD_WRITE(_HCI_WRITE), length);
  if(modem.passthrough(data, length)) {
    return length;
  }
  return 0;
}

HCIVirtualTransportATClass HCIVirtualTransportAT;

HCITransportInterface& HCITransport = HCIVirtualTransportAT;

#endif
