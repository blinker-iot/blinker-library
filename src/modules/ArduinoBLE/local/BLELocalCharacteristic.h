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

#ifndef _BLE_LOCAL_CHARACTERISTIC_H_
#define _BLE_LOCAL_CHARACTERISTIC_H_

#include <stdint.h>

#include "../BLECharacteristic.h"
#include "../BLEDescriptor.h"

#include "BLELocalAttribute.h"

#include "../utility/BLELinkedList.h"

class BLELocalDescriptor;

class BLELocalCharacteristic : public BLELocalAttribute {
public:
  BLELocalCharacteristic(const char* uuid, uint16_t permissions, int valueSize, bool fixedLength = false);
  BLELocalCharacteristic(const char* uuid, uint16_t permissions, const char* value);
  virtual ~BLELocalCharacteristic();

  virtual enum BLEAttributeType type() const;

  uint8_t properties() const;
  uint8_t permissions() const;

  int valueSize() const;
  const uint8_t* value() const;
  int valueLength() const;
  uint8_t operator[] (int offset) const;

  int writeValue(const uint8_t value[], int length);
  int writeValue(const char* value);

  int broadcast();

  bool written();
  bool subscribed();

  void addDescriptor(BLEDescriptor& descriptor);

  void setEventHandler(BLECharacteristicEvent event, BLECharacteristicEventHandler eventHandler);

protected:
  friend class ATTClass;
  friend class GATTClass;

  void setHandle(uint16_t handle);
  uint16_t handle() const;
  uint16_t valueHandle() const;

  unsigned int descriptorCount() const;
  BLELocalDescriptor* descriptor(unsigned int index) const;

  void readValue(BLEDevice device, uint16_t offset, uint8_t value[], int length);
  void writeValue(BLEDevice device, const uint8_t value[], int length);
  void writeCccdValue(BLEDevice device, uint16_t value);

private:
  uint8_t  _properties;
  uint8_t  _permissions;
  int      _valueSize;
  uint8_t* _value;
  uint16_t  _valueLength;
  bool _fixedLength;

  uint16_t _handle;

  bool _broadcast;
  bool _written;

  uint16_t _cccdValue;
  BLELinkedList<BLELocalDescriptor*> _descriptors;

  BLECharacteristicEventHandler _eventHandlers[BLECharacteristicEventLast];
};

#endif
