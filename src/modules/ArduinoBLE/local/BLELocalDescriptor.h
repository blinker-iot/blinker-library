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

#ifndef _BLE_LOCAL_DESCRIPTOR_H_
#define _BLE_LOCAL_DESCRIPTOR_H_

#include <stdint.h>

#include "BLELocalAttribute.h"

class BLELocalDescriptor : public BLELocalAttribute {
public:
  BLELocalDescriptor(const char* uuid, const uint8_t value[], int valueSize);
  BLELocalDescriptor(const char* uuid, const char* value);
  virtual ~BLELocalDescriptor();

  virtual enum BLEAttributeType type() const;

  int valueSize() const;
  const uint8_t* value() const;
  uint8_t operator[] (int offset) const;

protected:
  friend class GATTClass;

  void setHandle(uint16_t handle);
  uint16_t handle() const;

private:
  const uint8_t* _value;
  int            _valueSize;

  uint16_t       _handle;
};

#endif
