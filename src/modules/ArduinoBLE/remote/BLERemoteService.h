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

#ifndef _BLE_REMOTE_SERVICE_H_
#define _BLE_REMOTE_SERVICE_H_

#include "BLERemoteAttribute.h"
#include "BLERemoteCharacteristic.h"

#include "../utility/BLELinkedList.h"

class BLERemoteService : public BLERemoteAttribute {
public:
  BLERemoteService(const uint8_t uuid[], uint8_t uuidLen, uint16_t startHandle, uint16_t endHandle);
  virtual ~BLERemoteService();

  unsigned int characteristicCount() const;
  BLERemoteCharacteristic* characteristic(unsigned int index) const;

protected:
  friend class ATTClass;

  uint16_t startHandle() const;
  uint16_t endHandle() const;

  void addCharacteristic(BLERemoteCharacteristic* characteristic);

private:
  uint16_t _startHandle;
  uint16_t _endHandle;

  String _uuid;

  BLELinkedList<BLERemoteCharacteristic*> _characteristics;
};

#endif
