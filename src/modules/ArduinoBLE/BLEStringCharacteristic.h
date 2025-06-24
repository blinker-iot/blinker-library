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

#ifndef _BLE_STRING_CHARACTERISTIC_H_
#define _BLE_STRING_CHARACTERISTIC_H_

#include <Arduino.h>

#include "BLECharacteristic.h"

class BLEStringCharacteristic : public BLECharacteristic
{
public:
  BLEStringCharacteristic(const char* uuid, unsigned int properties, int valueSize);

  int writeValue(const String& value);
  int setValue(const String& value) { return writeValue(value); }
  String value(void);

private:
};

#endif
