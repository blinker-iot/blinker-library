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

#ifndef _BLE_CHARACTERISTIC_H_
#define _BLE_CHARACTERISTIC_H_

#include <stdint.h>

#include "BLEDescriptor.h"

enum BLECharacteristicEvent {
  BLESubscribed = 0,
  BLEUnsubscribed = 1,
//BLERead = 2, // defined in BLEProperties.h
  BLEWritten = 3,
  BLEUpdated = BLEWritten, // alias

  BLECharacteristicEventLast
};

class BLECharacteristic;
class BLEDevice;

typedef void (*BLECharacteristicEventHandler)(BLEDevice device, BLECharacteristic characteristic);

class BLELocalCharacteristic;
class BLERemoteCharacteristic;

class BLECharacteristic  {
public:
  BLECharacteristic();
  BLECharacteristic(const char* uuid, uint16_t permissions, int valueSize, bool fixedLength = false);
  BLECharacteristic(const char* uuid, uint16_t permissions, const char* value);
  BLECharacteristic(const BLECharacteristic& other);
  virtual ~BLECharacteristic();

  const char* uuid() const;

  uint8_t properties() const;

  int valueSize() const;
  const uint8_t* value() const;
  int valueLength() const;
  uint8_t operator[] (int offset) const;

  int readValue(uint8_t value[], int length);
  int readValue(void* value, int length);
  int readValue(uint8_t& value);
  int readValue(int8_t& value);
  int readValue(uint16_t& value);
  int readValue(int16_t& value);
  int readValue(uint32_t& value);
  int readValue(int32_t& value);

  int writeValue(const uint8_t value[], int length, bool withResponse = true);
  int writeValue(const void* value, int length, bool withResponse = true);
  int writeValue(const char* value, bool withResponse = true);
  int writeValue(uint8_t value, bool withResponse = true);
  int writeValue(int8_t value, bool withResponse = true);
  int writeValue(uint16_t value, bool withResponse = true);
  int writeValue(int16_t value, bool withResponse = true);
  int writeValue(uint32_t value, bool withResponse = true);
  int writeValue(int32_t value, bool withResponse = true);

  // deprecated, use writeValue(...)
  int setValue(const uint8_t value[], int length) { return writeValue(value, length); }
  int setValue(const char* value) { return writeValue(value); }

  int broadcast();

  bool written();
  bool subscribed();
  bool valueUpdated();

  void addDescriptor(BLEDescriptor& descriptor);

  operator bool() const;

  void setEventHandler(int event, BLECharacteristicEventHandler eventHandler);

  int descriptorCount() const;
  bool hasDescriptor(const char* uuid) const;
  bool hasDescriptor(const char* uuid, int index) const;
  BLEDescriptor descriptor(int index) const;
  BLEDescriptor descriptor(const char * uuid) const;
  BLEDescriptor descriptor(const char * uuid, int index) const;

  bool canRead();
  bool read();
  bool canWrite();
  bool canSubscribe();
  bool subscribe();
  bool canUnsubscribe();
  bool unsubscribe();

protected:
  friend class BLELocalCharacteristic;
  friend class BLELocalService;

  BLECharacteristic(BLELocalCharacteristic* local);

  BLELocalCharacteristic* local();

protected:
  friend class BLEDevice;
  friend class BLEService;
  friend class BLERemoteCharacteristic;

  BLECharacteristic(BLERemoteCharacteristic* remote);

private:
  BLELocalCharacteristic* _local;
  BLERemoteCharacteristic* _remote;
};

#endif
