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

#ifndef _BLE_TYPED_CHARACTERISTIC_H_
#define _BLE_TYPED_CHARACTERISTIC_H_

#include "BLECharacteristic.h"

template<typename T> class BLETypedCharacteristic : public BLECharacteristic
{
public:
  BLETypedCharacteristic(const char* uuid, unsigned int permissions);

  int writeValue(T value);
  int setValue(T value) { return writeValue(value); }
  T value(void);

  int writeValueLE(T value);
  int setValueLE(T value) { return writeValueLE(value); }
  T valueLE(void);

  int writeValueBE(T value);
  int setValueBE(T value) { return writeValueBE(value); }
  T valueBE(void);

private:
  T byteSwap(T value);
};

template<typename T> BLETypedCharacteristic<T>::BLETypedCharacteristic(const char* uuid, unsigned int permissions) :
  BLECharacteristic(uuid, permissions, sizeof(T), true)
{
  T value;
  memset(&value, 0x00, sizeof(value));

  writeValue(value);
}

template<typename T> int BLETypedCharacteristic<T>::writeValue(T value)
{
  return BLECharacteristic::writeValue((uint8_t*)&value, sizeof(T));
}

template<typename T> T BLETypedCharacteristic<T>::value()
{
  T value;

  memcpy(&value, (unsigned char*)BLECharacteristic::value(), BLECharacteristic::valueSize());

  return value;
}

template<typename T> int BLETypedCharacteristic<T>::writeValueLE(T value)
{
  return writeValue(value);
}

template<typename T> T BLETypedCharacteristic<T>::valueLE()
{
  return value();
}

template<typename T> int BLETypedCharacteristic<T>::writeValueBE(T value)
{
  return writeValue(byteSwap(value));
}

template<typename T> T BLETypedCharacteristic<T>::valueBE()
{
  return byteSwap(value());
}

template<typename T> T BLETypedCharacteristic<T>::byteSwap(T value)
{
  T result;
  unsigned char* src = (unsigned char*)&value;
  unsigned char* dst = (unsigned char*)&result;

  for (int i = 0; i < sizeof(T); i++) {
    dst[i] = src[sizeof(T) - i - 1];
  }

  return result;
}

#endif
