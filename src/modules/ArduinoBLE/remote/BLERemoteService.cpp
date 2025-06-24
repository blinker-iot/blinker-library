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

#include "BLERemoteService.h"

BLERemoteService::BLERemoteService(const uint8_t uuid[], uint8_t uuidLen, uint16_t startHandle, uint16_t endHandle) :
  BLERemoteAttribute(uuid, uuidLen),
  _startHandle(startHandle),
  _endHandle(endHandle)
{
}

BLERemoteService::~BLERemoteService()
{
  for (unsigned int i = 0; i < characteristicCount(); i++) {
    BLERemoteCharacteristic* c = characteristic(i);

    if (c->release() == 0) {
      delete c;
    }
  }

  _characteristics.clear();
}

uint16_t BLERemoteService::startHandle() const
{
  return _startHandle;
}

uint16_t BLERemoteService::endHandle() const
{
  return _endHandle;
}

unsigned int BLERemoteService::characteristicCount() const
{
  return _characteristics.size();
}

BLERemoteCharacteristic* BLERemoteService::characteristic(unsigned int index) const
{
  return _characteristics.get(index);
}

void BLERemoteService::addCharacteristic(BLERemoteCharacteristic* characteristic)
{
  characteristic-> retain();

  _characteristics.add(characteristic);
}
