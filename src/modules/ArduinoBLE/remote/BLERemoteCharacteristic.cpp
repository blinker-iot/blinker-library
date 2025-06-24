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

#include "../BLEProperty.h"

#include "../utility/ATT.h"

#include "BLERemoteCharacteristic.h"

BLERemoteCharacteristic::BLERemoteCharacteristic(const uint8_t uuid[], uint8_t uuidLen, uint16_t connectionHandle,
                                                  uint16_t startHandle, uint16_t permissions, uint16_t valueHandle) :
  BLERemoteAttribute(uuid, uuidLen),
  _connectionHandle(connectionHandle),
  _startHandle(startHandle),
  _properties((uint8_t)(permissions & 0x00FF)),
  _permissions((uint8_t)((permissions & 0xFF00)>>8)),
  _valueHandle(valueHandle),
  _value(NULL),
  _valueLength(0),
  _valueUpdated(false),
  _updatedValueRead(true),
  _valueUpdatedEventHandler(NULL)
{
}

BLERemoteCharacteristic::~BLERemoteCharacteristic()
{
  for (unsigned int i = 0; i < descriptorCount(); i++) {
    BLERemoteDescriptor* d = descriptor(i);

    if (d->release() == 0) {
      delete d;
    }
  }

  _descriptors.clear();

  if (_value) {
    free(_value);
    _value = NULL;
  }
}

uint16_t BLERemoteCharacteristic::startHandle() const
{
  return _startHandle;
}

uint8_t BLERemoteCharacteristic::properties() const
{
  return _properties;
}

const uint8_t* BLERemoteCharacteristic::value() const
{
  return _value;
}

int BLERemoteCharacteristic::valueLength() const
{
  return _valueLength;
}

uint8_t BLERemoteCharacteristic::operator[] (int offset) const
{
  if (_value) {
    return _value[offset];
  }

  return 0;
}

int BLERemoteCharacteristic::writeValue(const uint8_t value[], int length, bool withResponse)
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

  if ((_properties & BLEWrite) && withResponse) {
    uint8_t resp[4];
    int respLength = ATT.writeReq(_connectionHandle, _valueHandle, value, length, resp);

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
  } else if (_properties & BLEWriteWithoutResponse) {
    ATT.writeCmd(_connectionHandle, _valueHandle, value, length);

    memcpy(_value, value, length);
    _valueLength = length;

    return 1;
  }

  return 0;
}

int BLERemoteCharacteristic::writeValue(const char* value, bool withResponse)
{
  return writeValue((uint8_t*)value, strlen(value), withResponse);
}

bool BLERemoteCharacteristic::valueUpdated()
{
  ATT.connected(_connectionHandle); // to force a poll

  bool result = _valueUpdated;

  _valueUpdated = false;

  return result;
}

bool BLERemoteCharacteristic::updatedValueRead()
{
  bool result = _updatedValueRead;

  _updatedValueRead = true;

  return result;
}

bool BLERemoteCharacteristic::read()
{
  if (!ATT.connected(_connectionHandle)) {
    return false;
  }
  
  uint8_t resp[256];

  int respLength = ATT.readReq(_connectionHandle, _valueHandle, resp);

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

bool BLERemoteCharacteristic::writeCccd(uint16_t value)
{
  int numDescriptors = descriptorCount();

  for (int i = 0; i < numDescriptors; i++) {
    BLERemoteDescriptor* d = descriptor(i);

    if (strcmp(d->uuid(), "2902") == 0) {
      return d->writeValue((uint8_t*)&value, sizeof(value));
    }
  }

  if (_properties & (BLENotify | BLEIndicate)) {
    // no CCCD descriptor found, fallback to _valueHandle + 1
    BLERemoteDescriptor cccd(NULL, 0, _connectionHandle, _valueHandle + 1);

    return cccd.writeValue((uint8_t*)&value, sizeof(value));
  }

  return false;
}

uint16_t BLERemoteCharacteristic::valueHandle() const
{
  return _valueHandle;
}

unsigned int BLERemoteCharacteristic::descriptorCount() const
{
  return _descriptors.size();
}

BLERemoteDescriptor* BLERemoteCharacteristic::descriptor(unsigned int index) const
{
  return _descriptors.get(index);
}

void BLERemoteCharacteristic::setEventHandler(BLECharacteristicEvent event, BLECharacteristicEventHandler eventHandler)
{
  if (event == BLEUpdated) {
    _valueUpdatedEventHandler = eventHandler;
  }
}

void BLERemoteCharacteristic::addDescriptor(BLERemoteDescriptor* descriptor)
{
  descriptor->retain();

  _descriptors.add(descriptor);
}

void BLERemoteCharacteristic::writeValue(BLEDevice device, const uint8_t value[], int length)
{
  _valueLength = length;
  _value = (uint8_t*)realloc(_value, _valueLength);

  if (_value == NULL) {
    _valueLength = 0;
    return;
  }

  _valueUpdated = true;
  _updatedValueRead = false;
  memcpy(_value, value, _valueLength);

  if (_valueUpdatedEventHandler) {
    _valueUpdatedEventHandler(device, BLECharacteristic(this));
  }
}

#endif
