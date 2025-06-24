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

#include <Arduino.h>

#include "BLELocalDescriptor.h"

BLELocalDescriptor::BLELocalDescriptor(const char* uuid, const uint8_t value[], int valueSize) :
  BLELocalAttribute(uuid),
  _value(value),
  _valueSize(min(valueSize, 512)),
  _handle(0x0000)
{
}

BLELocalDescriptor::BLELocalDescriptor(const char* uuid, const char* value) :
  BLELocalDescriptor(uuid, (const uint8_t*)value, strlen(value))
{
}

BLELocalDescriptor::~BLELocalDescriptor()
{
}

enum BLEAttributeType BLELocalDescriptor::type() const
{
  return BLETypeDescriptor;
}

int BLELocalDescriptor::valueSize() const
{
  return _valueSize;
}

const uint8_t* BLELocalDescriptor::value() const
{
  return _value;
}

uint8_t BLELocalDescriptor::operator[] (int offset) const
{
  return _value[offset];
}

void BLELocalDescriptor::setHandle(uint16_t handle)
{
  _handle = handle;
}

uint16_t BLELocalDescriptor::handle() const
{
  return _handle;
}
