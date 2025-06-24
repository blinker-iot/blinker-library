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

#include <Arduino.h>
#include "BLELocalDevice.h"

#include "../utility/ATT.h"
#include "../utility/GAP.h"
#include "../utility/GATT.h"

#include "BLELocalDescriptor.h"
#include "../BLEProperty.h"

#include "BLELocalCharacteristic.h"

BLELocalCharacteristic::BLELocalCharacteristic(const char* uuid, uint16_t permissions, int valueSize, bool fixedLength) :
  BLELocalAttribute(uuid),
  _properties((uint8_t)(permissions&0x000FF)),
  _permissions((uint8_t)((permissions&0xFF00)>>8)),
  _valueSize(min(valueSize, 512)),
  _valueLength(0),
  _fixedLength(fixedLength),
  _handle(0x0000),
  _broadcast(false),
  _written(false),
  _cccdValue(0x0000)
{
  memset(_eventHandlers, 0x00, sizeof(_eventHandlers));

  if (permissions & (BLENotify | BLEIndicate)) {
    BLELocalDescriptor* cccd = new BLELocalDescriptor("2902", (uint8_t*)&_cccdValue, sizeof(_cccdValue));
  
    cccd->retain();
    _descriptors.add(cccd);
  }

  _value = (uint8_t*)malloc(valueSize);
}

BLELocalCharacteristic::BLELocalCharacteristic(const char* uuid, uint16_t permissions, const char* value) :
  BLELocalCharacteristic(uuid, permissions, strlen(value))
{
  writeValue(value);
}
BLELocalCharacteristic::~BLELocalCharacteristic()
{
  for (unsigned int i = 0; i < descriptorCount(); i++) {
    BLELocalDescriptor* d = descriptor(i);

    if (d->release() == 0) {
      delete d;
    }
  }

  _descriptors.clear();

  if (_value) {
    free(_value);
  }
}

enum BLEAttributeType BLELocalCharacteristic::type() const
{
  return BLETypeCharacteristic;
}

uint8_t BLELocalCharacteristic::properties() const
{
  return _properties;
}

uint8_t BLELocalCharacteristic::permissions() const {
  return _permissions;
}

int BLELocalCharacteristic::valueSize() const
{
  return _valueSize;
}

const uint8_t* BLELocalCharacteristic::value() const
{
  return _value;
}

int BLELocalCharacteristic::valueLength() const
{
  return _valueLength;
}

uint8_t BLELocalCharacteristic::operator[] (int offset) const
{
  return _value[offset];
}

int BLELocalCharacteristic::writeValue(const uint8_t value[], int length)
{
  _valueLength = min(length, _valueSize);
  memcpy(_value, value, _valueLength);

  if (_fixedLength) {
    _valueLength = _valueSize;
  }

  if ((_properties & BLEIndicate) && (_cccdValue & 0x0002)) {
    return ATT.handleInd(valueHandle(), _value, _valueLength);
  } else if ((_properties & BLENotify) && (_cccdValue & 0x0001)) {
    return ATT.handleNotify(valueHandle(), _value, _valueLength);
  }

  if (_broadcast) {
    uint16_t serviceUuid = GATT.serviceUuidForCharacteristic(this);
    BLE.setAdvertisedServiceData(serviceUuid, value, _valueLength);
    if (!ATT.connected() && GAP.advertising()) {
      BLE.advertise();
    }
  }

  return _valueLength;
}

int BLELocalCharacteristic::writeValue(const char* value)
{
  return writeValue((uint8_t*)value, strlen(value));
}

int BLELocalCharacteristic::broadcast()
{
  if (_properties & BLEBroadcast) {
    _broadcast = true;

    return 1;
  }

  return 0;
}

bool BLELocalCharacteristic::written()
{
  bool written = _written;

  _written = false;

  return written;
}

bool BLELocalCharacteristic::subscribed()
{
  return (_cccdValue != 0x0000);
}

void BLELocalCharacteristic::addDescriptor(BLEDescriptor& descriptor)
{
  BLELocalDescriptor* localDescriptor = descriptor.local();

  if (localDescriptor) {
    localDescriptor->retain();

    _descriptors.add(localDescriptor);
  }
}

void BLELocalCharacteristic::setEventHandler(BLECharacteristicEvent event, BLECharacteristicEventHandler eventHandler)
{
  if (event < (sizeof(_eventHandlers) / sizeof(_eventHandlers[0]))) {
    _eventHandlers[event] = eventHandler;
  }
}

void BLELocalCharacteristic::setHandle(uint16_t handle)
{
  _handle = handle;
}

uint16_t BLELocalCharacteristic::handle() const
{
  return _handle;
}

uint16_t BLELocalCharacteristic::valueHandle() const
{
  return (_handle + 1);
}

unsigned int BLELocalCharacteristic::descriptorCount() const
{
  return _descriptors.size();
}

BLELocalDescriptor* BLELocalCharacteristic::descriptor(unsigned int index) const
{
  return _descriptors.get(index);
}

void BLELocalCharacteristic::readValue(BLEDevice device, uint16_t offset, uint8_t value[], int length)
{
  if (_eventHandlers[BLERead]) {
    _eventHandlers[BLERead](device, BLECharacteristic(this));
  }

  memcpy(value, _value + offset, length);
}

void BLELocalCharacteristic::writeValue(BLEDevice device, const uint8_t value[], int length)
{
  _written = true;

  writeValue(value, length);

  if (_eventHandlers[BLEWritten]) {
    _eventHandlers[BLEWritten](device, BLECharacteristic(this));
  }
}

void BLELocalCharacteristic::writeCccdValue(BLEDevice device, uint16_t value)
{
  value &= 0x0003;

  if (_cccdValue != value) {
    _cccdValue = value;

    BLECharacteristicEvent event = (_cccdValue) ? BLESubscribed : BLEUnsubscribed;

    if (_eventHandlers[event]) {
      _eventHandlers[event](device, BLECharacteristic(this));
    }
  }
}
