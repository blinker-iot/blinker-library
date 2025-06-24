/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

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

#if defined(ARDUINO_ARCH_RENESAS)

#include "../utility/ATT.h"

#include "BLERemoteDescriptor.h"

BLERemoteDescriptor::BLERemoteDescriptor(const uint8_t uuid[], uint8_t uuidLen, uint16_t connectionHandle, uint16_t handle) :
  BLERemoteAttribute(uuid, uuidLen),
  _connectionHandle(connectionHandle),
  _handle(handle),
  _value(NULL),
  _valueLength(0)
{
}

BLERemoteDescriptor::~BLERemoteDescriptor()
{
  if (_value) {
    free(_value);
    _value = NULL;
  }
}

const uint8_t* BLERemoteDescriptor::value() const
{
  return _value;
}

int BLERemoteDescriptor::valueLength() const
{
  return _valueLength;
}

uint8_t BLERemoteDescriptor::operator[] (int offset) const
{
  if (_value) {
    return _value[offset];
  }

  return 0;
}

int BLERemoteDescriptor::writeValue(const uint8_t value[], int length)
{
  if (!ATT.connected(_connectionHandle)) {
    return false;
  }

  uint16_t maxLength = ATT.mtu(_connectionHandle) - 3;

  if (length > (int)maxLength) {
    // cap to MTU max length
    length = maxLength;
  }

  _value = (uint8_t*)realloc(_value, length);
  if (_value == NULL) {
    // realloc failed
    return 0;
  }  

  uint8_t resp[4];
  int respLength = ATT.writeReq(_connectionHandle, _handle, value, length, resp);

  if (!respLength) {
    return 0;
  }

  if (resp[0] == 0x01) {
    // error
    return 0;
  }

  memcpy(_value, value, length);
  _valueLength = length;

  return 1;
}

bool BLERemoteDescriptor::read()
{
  if (!ATT.connected(_connectionHandle)) {
    return false;
  }

  uint8_t resp[256];

  int respLength = ATT.readReq(_connectionHandle, _handle, resp);

  if (!respLength) {
    _valueLength = 0;
    return false;
  }

  if (resp[0] == 0x01) {
    // error
    _valueLength = 0;
    return false;
  }

  _valueLength = respLength - 1;
  _value = (uint8_t*)realloc(_value, _valueLength);

  if (_value == NULL) {
    _valueLength = 0;
    return false;
  }

  memcpy(_value, &resp[1], _valueLength); 

  return true;
}

uint16_t BLERemoteDescriptor::handle() const
{
  return _handle;
}

#endif
