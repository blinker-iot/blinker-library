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

#if defined(ARDUINO_ARCH_RENESAS)

#include "../utility/BLEUuid.h"

#include "BLERemoteAttribute.h"

BLERemoteAttribute::BLERemoteAttribute(const uint8_t uuid[], uint8_t uuidLen) :
  _uuid(BLEUuid::uuidToString(uuid, uuidLen)),
  _refCount(0)
{
}

BLERemoteAttribute::~BLERemoteAttribute()
{
}

const char* BLERemoteAttribute::uuid() const
{
  return _uuid.c_str();
}

int BLERemoteAttribute::retain()
{
  _refCount++;

  return _refCount;
}

int BLERemoteAttribute::release()
{
  _refCount--;

  return _refCount;
}

#endif
