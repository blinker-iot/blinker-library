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

#ifndef _BLE_LOCAL_SERVICE_H_
#define _BLE_LOCAL_SERVICE_H_

#include "../BLECharacteristic.h"

#include "BLELocalAttribute.h"

#include "../utility/BLELinkedList.h"

class BLELocalCharacteristic;

class BLELocalService : public BLELocalAttribute {
public:
  BLELocalService(const char* uuid);
  virtual ~BLELocalService();

  virtual enum BLEAttributeType type() const;

  void addCharacteristic(BLECharacteristic& characteristic);
  void clear();

protected:
  friend class ATTClass;
  friend class GATTClass;

  void setHandles(uint16_t start, uint16_t end);
  uint16_t startHandle() const;
  uint16_t endHandle() const;

  unsigned int characteristicCount() const;
  BLELocalCharacteristic* characteristic(unsigned int index) const;

  void addCharacteristic(BLELocalCharacteristic* characteristic);

private:
  uint16_t _startHandle;
  uint16_t _endHandle;

  BLELinkedList<BLELocalCharacteristic*> _characteristics;
};

#endif
