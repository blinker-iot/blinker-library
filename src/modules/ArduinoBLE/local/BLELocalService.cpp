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

#include "BLELocalCharacteristic.h"

#include "BLELocalService.h"

BLELocalService::BLELocalService(const char* uuid) :
  BLELocalAttribute(uuid),
  _startHandle(0x0000),
  _endHandle(0x0000)
{
}

void BLELocalService::clear() {
  _characteristics.clear();
  _startHandle = 0;
  _endHandle = 0;
}

BLELocalService::~BLELocalService()
{
  for (unsigned int i = 0; i < characteristicCount(); i++) {
    BLELocalCharacteristic* c = characteristic(i);

    if (c->release() == 0) {
      delete c;
    }
  }
  clear();
}

enum BLEAttributeType BLELocalService::type() const
{
  return BLETypeService;
}

void BLELocalService::addCharacteristic(BLECharacteristic& characteristic)
{
  BLELocalCharacteristic* localCharacteristic = characteristic.local();

  if (localCharacteristic) {
    addCharacteristic(localCharacteristic);
  }
}

void BLELocalService::setHandles(uint16_t start, uint16_t end)
{
  _startHandle = start;
  _endHandle = end;
}

uint16_t BLELocalService::startHandle() const
{
  return _startHandle;
}

uint16_t BLELocalService::endHandle() const
{
  return _endHandle;
}

unsigned int BLELocalService::characteristicCount() const
{
  return _characteristics.size();
}

BLELocalCharacteristic* BLELocalService::characteristic(unsigned int index) const
{
  return _characteristics.get(index);
}

void BLELocalService::addCharacteristic(BLELocalCharacteristic* characteristic)
{
  characteristic->retain();

  _characteristics.add(characteristic);
}
