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

#ifndef _BLE_DESCRIPTOR_H_
#define _BLE_DESCRIPTOR_H_

#include <stdint.h>

class BLELocalDescriptor;
class BLERemoteDescriptor;

class BLEDescriptor {
public:
  BLEDescriptor();
  BLEDescriptor(const BLEDescriptor& other);
  BLEDescriptor(const char* uuid, const uint8_t value[], int valueSize);
  BLEDescriptor(const char* uuid, const char* value);
  virtual ~BLEDescriptor();

  const char* uuid() const;

  int valueSize() const;
  const uint8_t* value() const;
  int valueLength() const;
  uint8_t operator[] (int offset) const;

  int readValue(uint8_t value[], int length);
  int readValue(void* value, int length);
  int readValue(uint8_t& value);
  int readValue(int8_t& value);
  int readValue(uint16_t& value);
  int readValue(int16_t& value);
  int readValue(uint32_t& value);
  int readValue(int32_t& value);

  operator bool() const;

  bool read();

protected:
  friend class BLELocalCharacteristic;

  BLEDescriptor(BLELocalDescriptor* local);

  BLELocalDescriptor* local();

protected:
  friend class BLECharacteristic;

  BLEDescriptor(BLERemoteDescriptor* remote);

private:
  BLELocalDescriptor* _local;
  BLERemoteDescriptor* _remote;
};

#endif
