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

#ifndef _HCI_TRANSPORT_INTERFACE_H_
#define _HCI_TRANSPORT_INTERFACE_H_

#include <Arduino.h>

class HCITransportInterface {
public:
  virtual int begin() = 0;
  virtual void end() = 0;

  virtual void wait(unsigned long timeout) = 0;

  virtual int available() = 0;
  virtual int peek() = 0;
  virtual int read() = 0;

  virtual size_t write(const uint8_t* data, size_t length) = 0;
};

extern HCITransportInterface& HCITransport;

#endif
