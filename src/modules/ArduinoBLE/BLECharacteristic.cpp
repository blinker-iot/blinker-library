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

#include "BLEProperty.h"

#include "local/BLELocalCharacteristic.h"
#include "remote/BLERemoteCharacteristic.h"

#include "BLECharacteristic.h"

extern "C" int strcasecmp(char const *a, char const *b);

BLECharacteristic::BLECharacteristic() :
  BLECharacteristic((BLELocalCharacteristic*)NULL)
{
}

BLECharacteristic::BLECharacteristic(BLELocalCharacteristic* local) :
  _local(local),
  _remote(NULL)
{
  if (_local) {
    _local->retain();
  }
}

BLECharacteristic::BLECharacteristic(BLERemoteCharacteristic* remote) :
  _local(NULL),
  _remote(remote)
{
  if (_remote) {
    _remote->retain();
  }
}

BLECharacteristic::BLECharacteristic(const char* uuid, uint16_t permissions, int valueSize, bool fixedLength) :
  BLECharacteristic(new BLELocalCharacteristic(uuid, permissions, valueSize, fixedLength))
{
}

BLECharacteristic::BLECharacteristic(const char* uuid, uint16_t permissions, const char* value) :
  BLECharacteristic(new BLELocalCharacteristic(uuid, permissions, value))
{
}

BLECharacteristic::BLECharacteristic(const BLECharacteristic& other)
{
  _local = other._local;
  if (_local) {
    _local->retain();
  }

  _remote = other._remote;
  if (_remote) {
    _remote->retain();
  }
}

BLECharacteristic::~BLECharacteristic()
{
  if (_local && _local->release() == 0) {
    delete _local;
  }

  if (_remote && _remote->release() == 0) {
    delete _remote;
  }
}

const char* BLECharacteristic::uuid() const
{
  if (_local) {
    return _local->uuid();
  }

  if (_remote) {
    return _remote->uuid();
  }

  return "";
}

uint8_t BLECharacteristic::properties() const
{
  if (_local) {
    return _local->properties();
  }

  if (_remote) {
    return _remote->properties();
  }

  return 0;
}

int BLECharacteristic::valueSize() const
{
  if (_local) {
    return _local->valueSize();
  }

  if (_remote) {
    return _remote->valueLength();
  }

  return 0;
}

const uint8_t* BLECharacteristic::value() const
{
  if (_local) {
    return _local->value();
  }

  if (_remote) {
    return _remote->value();
  }

  return NULL;
}

int BLECharacteristic::valueLength() const
{
  if (_local) {
    return _local->valueLength();
  }

  if (_remote) {
    return _remote->valueLength();
  }

  return 0;
}

uint8_t BLECharacteristic::operator[] (int offset) const
{
  if (_local) {
    return (*_local)[offset];
  }

  if (_remote) {
    return (*_remote)[offset];
  }

  return 0;
}

int BLECharacteristic::readValue(uint8_t value[], int length)
{
  int bytesRead = 0;

  if (_local) {
    bytesRead = min(length, _local->valueLength());

    memcpy(value, _local->value(), bytesRead);
  }

  if (_remote) {
    // trigger a read if the updated value (notification/indication)
    // has already been read and the characteristic is readable
    if (_remote->updatedValueRead() && canRead()) {
      if (!read()) {
        // read failed
        return 0;
      }
    }

    bytesRead = min(length, _remote->valueLength());

    memcpy(value, _remote->value(), bytesRead);
  }

  return bytesRead;
}

int BLECharacteristic::readValue(void* value, int length)
{
  return readValue((uint8_t*)value, length);
}

int BLECharacteristic::readValue(uint8_t& value)
{
  value = 0;

  return readValue((uint8_t*)&value, sizeof(value));
}

int BLECharacteristic::readValue(int8_t& value)
{
  value = 0;

  return readValue((uint8_t*)&value, sizeof(value));
}

int BLECharacteristic::readValue(uint16_t& value)
{
  value = 0;

  return readValue((uint8_t*)&value, sizeof(value));
}

int BLECharacteristic::readValue(int16_t& value)
{
  value = 0;

  return readValue((uint8_t*)&value, sizeof(value));
}

int BLECharacteristic::readValue(uint32_t& value)
{
  value = 0;

  return readValue((uint8_t*)&value, sizeof(value));
}

int BLECharacteristic::readValue(int32_t& value)
{
  value = 0;

  return readValue((uint8_t*)&value, sizeof(value));
}

int BLECharacteristic::writeValue(const uint8_t value[], int length, bool withResponse)
{
  if (_local) {
    return _local->writeValue(value, length);
  }

  if (_remote) {
    return _remote->writeValue(value, length, withResponse);
  }

  return 0;
}

int BLECharacteristic::writeValue(const void* value, int length, bool withResponse)
{
  return writeValue((const uint8_t*)value, length, withResponse);
}

int BLECharacteristic::writeValue(const char* value, bool withResponse)
{
  if (_local) {
    return _local->writeValue(value);
  }

  if (_remote) {
    return _remote->writeValue(value, withResponse);
  }

  return 0;
}

int BLECharacteristic::writeValue(uint8_t value, bool withResponse)
{
  return writeValue((uint8_t*)&value, sizeof(value), withResponse);
}

int BLECharacteristic::writeValue(int8_t value, bool withResponse)
{
  return writeValue((uint8_t*)&value, sizeof(value), withResponse);
}

int BLECharacteristic::writeValue(uint16_t value, bool withResponse)
{
  return writeValue((uint8_t*)&value, sizeof(value), withResponse);
}

int BLECharacteristic::writeValue(int16_t value, bool withResponse)
{
  return writeValue((uint8_t*)&value, sizeof(value), withResponse);
}

int BLECharacteristic::writeValue(uint32_t value, bool withResponse)
{
  return writeValue((uint8_t*)&value, sizeof(value), withResponse);
}

int BLECharacteristic::writeValue(int32_t value, bool withResponse)
{
  return writeValue((uint8_t*)&value, sizeof(value), withResponse);
}

int BLECharacteristic::broadcast()
{
  if (_local) {
    return _local->broadcast();
  }

  return 0;
}

bool BLECharacteristic::written()
{
  if (_local) {
    return _local->written();
  }

  return false;
}

bool BLECharacteristic::subscribed()
{
  if (_local) {
    return _local->subscribed();
  }

  return false;
}

bool BLECharacteristic::valueUpdated()
{
  if (_remote) {
    return _remote->valueUpdated();
  }

  return false; 
}

void BLECharacteristic::addDescriptor(BLEDescriptor& descriptor)
{
  if (_local) {
    return _local->addDescriptor(descriptor);
  }
}

BLECharacteristic::operator bool() const
{
  return (_local != NULL) || (_remote != NULL);
}

BLELocalCharacteristic* BLECharacteristic::local()
{
  return _local;
}

void BLECharacteristic::setEventHandler(int event, BLECharacteristicEventHandler eventHandler)
{
  if (_local) {
    _local->setEventHandler((BLECharacteristicEvent)event, eventHandler);
  }

  if (_remote) {
    _remote->setEventHandler((BLECharacteristicEvent)event, eventHandler);
  }
}

int BLECharacteristic::descriptorCount() const
{
  if (_remote) {
    return _remote->descriptorCount();
  }

  return 0;
}

bool BLECharacteristic::hasDescriptor(const char* uuid) const
{
  return hasDescriptor(uuid, 0);
}

bool BLECharacteristic::hasDescriptor(const char* uuid, int index) const
{
  if (_remote) {
    int count = 0;
    int numDescriptors = _remote->descriptorCount();

    for (int i = 0; i < numDescriptors; i++) {
      BLERemoteDescriptor* d = _remote->descriptor(i);

      if (strcasecmp(uuid, d->uuid()) == 0) {
        if (count == index) {
          return true;
        }

        count++;
      }
    }
  }

  return false;
}

BLEDescriptor BLECharacteristic::descriptor(int index) const
{
  if (_remote) {
    return BLEDescriptor(_remote->descriptor(index));
  }

  return BLEDescriptor();
}

BLEDescriptor BLECharacteristic::descriptor(const char * uuid) const
{
  return descriptor(uuid, 0);
}

BLEDescriptor BLECharacteristic::descriptor(const char * uuid, int index) const
{
  if (_remote) {
    int count = 0;
    int numDescriptors = _remote->descriptorCount();

    for (int i = 0; i < numDescriptors; i++) {
      BLERemoteDescriptor* d = _remote->descriptor(i);

      if (strcasecmp(uuid, d->uuid()) == 0) {
        if (count == index) {
          return BLEDescriptor(d);
        }

        count++;
      }
    }
  }

  return BLEDescriptor();
}

bool BLECharacteristic::canRead()
{
  if (_remote) {
    return (properties() & BLERead) != 0;
  }

  return false;
}

bool BLECharacteristic::read()
{
  if (_remote) {
    return _remote->read();
  }

  return false;
}

bool BLECharacteristic::canWrite()
{
  if (_remote) {
    return (properties() & (BLEWrite | BLEWriteWithoutResponse)) != 0;
  }

  return false;
}

bool BLECharacteristic::canSubscribe()
{
  if (_remote) {
    return (properties() & (BLENotify | BLEIndicate)) != 0;
  }

  return false;
}

bool BLECharacteristic::subscribe()
{
  if (_remote) {
    return _remote->writeCccd((properties() & BLEIndicate) ? 0x0002 : 0x0001);
  }

  return false;
}

bool BLECharacteristic::canUnsubscribe()
{
  return canSubscribe();
}

bool BLECharacteristic::unsubscribe()
{
  if (_remote) {
    return _remote->writeCccd(0x0000);
  }

  return false;
}
