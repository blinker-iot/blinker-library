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

#include <stdlib.h>
#include <string.h>

#include "BLEUuid.h"

BLEUuid::BLEUuid(const char * str) :
  _str(str)
{
  char temp[] = {0, 0, 0};

  memset(_data, 0x00, sizeof(_data));

  _length = 0;

  if (str == NULL) {
    return;
  }

  for (int i = strlen(str) - 1; i >= 0 && _length < BLE_UUID_MAX_LENGTH; i -= 2) {
    if (str[i] == '-') {
      i++;
      continue;
    }

    temp[0] = str[i - 1];
    temp[1] = str[i];

    _data[_length] = strtoul(temp, NULL, 16);

    _length++;
  }

  if (_length <= 2) {
    _length = 2;
  } else {
    _length = 16;
  }
}

const char* BLEUuid::str() const
{
  return _str;
}

const uint8_t* BLEUuid::data() const
{
  return _data;
}

uint8_t BLEUuid::length() const
{
  return _length;
}

const char* BLEUuid::uuidToString(const uint8_t* data, uint8_t length)
{
  static char uuid[36 + 1];
  char* c = uuid;

  for (int i = length - 1; i >= 0; i--) {
    uint8_t b = data[i];

    utoa(b >> 4, c++, 16);
    utoa(b & 0x0f, c++, 16);

    if (i == 6 || i == 8 || i == 10 || i == 12) {
      *c++ = '-';
    }
  }

  *c = '\0';

  return uuid;
}
