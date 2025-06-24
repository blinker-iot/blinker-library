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

#ifndef _BLE_REMOTE_DESCRIPTOR_H_
#define _BLE_REMOTE_DESCRIPTOR_H_

#include "BLERemoteAttribute.h"

class BLERemoteDescriptor : public BLERemoteAttribute {
public:
  BLERemoteDescriptor(const uint8_t uuid[], uint8_t uuidLen, uint16_t connectionHandle, uint16_t handle);
  virtual ~BLERemoteDescriptor();

  const uint8_t* value() const;
  int valueLength() const;
  uint8_t operator[] (int offset) const;

  int writeValue(const uint8_t value[], int length);

  bool read();

protected:
  friend class ATTClass;
  uint16_t handle() const;

private:
  uint16_t _connectionHandle;
  uint16_t _handle;

  uint8_t* _value;
  int _valueLength;
};

#endif
