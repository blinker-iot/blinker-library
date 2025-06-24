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

#ifndef _BLE_REMOTE_CHARACTERISTIC_H_
#define _BLE_REMOTE_CHARACTERISTIC_H_

#include "../BLECharacteristic.h"

#include "BLERemoteAttribute.h"
#include "BLERemoteDescriptor.h"

#include "../utility/BLELinkedList.h"

class BLERemoteCharacteristic : public BLERemoteAttribute {
public:
  BLERemoteCharacteristic(const uint8_t uuid[], uint8_t uuidLen, uint16_t connectionHandle, uint16_t startHandle, uint16_t permissions, uint16_t valueHandle);
  virtual ~BLERemoteCharacteristic();

  uint8_t properties() const;
  uint8_t permissions() const;

  const uint8_t* value() const;
  int valueLength() const;
  uint8_t operator[] (int offset) const;

  int writeValue(const uint8_t value[], int length, bool withResponse = true);
  int writeValue(const char* value, bool withResponse = true);

  bool valueUpdated();
  bool updatedValueRead();

  bool read();
  bool writeCccd(uint16_t value);

  unsigned int descriptorCount() const;
  BLERemoteDescriptor* descriptor(unsigned int index) const;

  void setEventHandler(BLECharacteristicEvent event, BLECharacteristicEventHandler eventHandler);

protected:
  friend class ATTClass;

  uint16_t startHandle() const;
  uint16_t valueHandle() const;

  void addDescriptor(BLERemoteDescriptor* descriptor);

  void writeValue(BLEDevice device, const uint8_t value[], int length);

private:
  uint16_t _connectionHandle;
  uint16_t _startHandle;
  uint8_t _properties;
  uint8_t _permissions;
  uint16_t _valueHandle;

  uint8_t* _value;
  int _valueLength;

  bool _valueUpdated;
  bool _updatedValueRead;

  BLELinkedList<BLERemoteDescriptor*> _descriptors;

  BLECharacteristicEventHandler _valueUpdatedEventHandler;
};

#endif
