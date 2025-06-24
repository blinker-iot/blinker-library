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

#include "BLEStringCharacteristic.h"

BLEStringCharacteristic::BLEStringCharacteristic(const char* uuid, unsigned int properties, int valueSize) :
  BLECharacteristic(uuid, properties, valueSize)
{
}

int BLEStringCharacteristic::writeValue(const String& value)
{
  return BLECharacteristic::writeValue(value.c_str());
}

String BLEStringCharacteristic::value(void)
{
  String str;
  int length = BLECharacteristic::valueLength();
  const uint8_t* val = BLECharacteristic::value();

  str.reserve(length);

  for (int i = 0; i < length; i++) {
    str += (char)val[i];
  }

  return str;
}
