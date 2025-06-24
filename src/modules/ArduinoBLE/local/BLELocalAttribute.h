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

#ifndef _BLE_LOCAL_ATTRIBUTE_H_
#define _BLE_LOCAL_ATTRIBUTE_H_

#include "../utility/BLEUuid.h"

#define BLE_ATTRIBUTE_TYPE_SIZE 2

enum BLEAttributeType {
  BLETypeUnknown        = 0x0000,

  BLETypeService        = 0x2800,
  BLETypeCharacteristic = 0x2803,
  BLETypeDescriptor     = 0x2900
};

class BLELocalAttribute
{
public:
  BLELocalAttribute(const char* uuid);
  virtual ~BLELocalAttribute();

  const char* uuid() const;

  virtual enum BLEAttributeType type() const;

  int retain();
  int release();
  bool active();

protected:
  friend class ATTClass;
  friend class GATTClass;

  const uint8_t* uuidData() const;
  uint8_t uuidLength() const;

private:
  BLEUuid _uuid;
  int _refCount;
};

#endif
