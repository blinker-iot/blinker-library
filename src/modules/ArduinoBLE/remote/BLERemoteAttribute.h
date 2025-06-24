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

#ifndef _BLE_REMOTE_ATTRIBUTE_H_
#define _BLE_REMOTE_ATTRIBUTE_H_

#include <Arduino.h>

class BLERemoteAttribute
{
public:
  BLERemoteAttribute(const uint8_t uuid[], uint8_t uuidLen);
  virtual ~BLERemoteAttribute();

  const char* uuid() const;

  int retain();
  int release();

private:
  String _uuid;
  int _refCount;
};

#endif
