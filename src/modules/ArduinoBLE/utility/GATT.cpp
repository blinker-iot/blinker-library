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

#if defined(ARDUINO_ARCH_RENESAS)

#include <Arduino.h>

#include "../local/BLELocalCharacteristic.h"
#include "../local/BLELocalDescriptor.h"
#include "../local/BLELocalService.h"

#include "../BLEProperty.h"

#include "GATT.h"

GATTClass::GATTClass() :
  _genericAccessService(NULL),
  _deviceNameCharacteristic(NULL),
  _appearanceCharacteristic(NULL),
  _genericAttributeService(NULL),
  _servicesChangedCharacteristic(NULL)
{
}

GATTClass::~GATTClass()
{
  end();
}

void GATTClass::begin()
{
  _genericAccessService = new BLELocalService("1800");
  _deviceNameCharacteristic = new BLELocalCharacteristic("2a00", BLERead, 20);
  _appearanceCharacteristic = new BLELocalCharacteristic("2a01", BLERead, 2);
  _genericAttributeService = new BLELocalService("1801");
  _servicesChangedCharacteristic = new BLELocalCharacteristic("2a05", BLEIndicate, 4);

  _genericAccessService->retain();
  _deviceNameCharacteristic->retain();
  _appearanceCharacteristic->retain();
  _genericAttributeService->retain();
  _servicesChangedCharacteristic->retain();

  _genericAccessService->addCharacteristic(_deviceNameCharacteristic);
  _genericAccessService->addCharacteristic(_appearanceCharacteristic);
  _genericAttributeService->addCharacteristic(_servicesChangedCharacteristic);

  setDeviceName("Arduino");
  setAppearance(0x000);

  clearAttributes();

  addService(_genericAccessService);
  addService(_genericAttributeService);
}

void GATTClass::end()
{
  if (_genericAccessService && _genericAccessService->release() == 0) {
    delete(_genericAccessService);
    _genericAccessService = NULL;
  }

  if (_deviceNameCharacteristic && _deviceNameCharacteristic->release() == 0) {
    delete(_deviceNameCharacteristic);
    _deviceNameCharacteristic = NULL;
  }

  if (_appearanceCharacteristic && _appearanceCharacteristic->release() == 0) {
    delete(_appearanceCharacteristic);
    _appearanceCharacteristic = NULL;
  }

  if (_genericAttributeService && _genericAttributeService->release() == 0) {
    delete(_genericAttributeService);
    _genericAttributeService = NULL;
  }

  if (_servicesChangedCharacteristic && _servicesChangedCharacteristic->release() == 0) {
    delete(_servicesChangedCharacteristic);
    _servicesChangedCharacteristic = NULL;
  }

  clearAttributes();
}

void GATTClass::setDeviceName(const char* deviceName)
{
  _deviceNameCharacteristic->writeValue(deviceName);
}

void GATTClass::setAppearance(uint16_t appearance)
{
  _appearanceCharacteristic->writeValue((uint8_t*)&appearance, sizeof(appearance));
}

void GATTClass::addService(BLEService& service)
{
  BLELocalService* localService = service.local();

  if (localService) {
    addService(localService);
  }
}

unsigned int GATTClass::attributeCount() const
{
  return _attributes.size();
}

BLELocalAttribute* GATTClass::attribute(unsigned int index) const
{
  return _attributes.get(index);
}

uint16_t GATTClass::serviceUuidForCharacteristic(BLELocalCharacteristic* characteristic) const
{
  uint16_t serviceUuid = 0x0000;

  BLELocalService* lastService = NULL;

  for (unsigned int i = 0; i < attributeCount(); i++) {
    BLELocalAttribute* a = attribute(i);
    uint16_t attributeType = a->type();

    if (attributeType == BLETypeService) {
      lastService = (BLELocalService*)a;
    } else if (a == characteristic) {
      break;
    }
  }

  if (lastService) {
    if (lastService->uuidLength() == 2) {
      serviceUuid = *(uint16_t*)(lastService->uuidData());
    } else {
      serviceUuid = *(uint16_t*)(lastService->uuidData() + 10);
    }
  }

  return serviceUuid;
}

void GATTClass::addService(BLELocalService* service)
{
  service->retain();
  _attributes.add(service);
  _services.add(service);

  uint16_t startHandle = attributeCount();

  for (unsigned int i = 0; i < service->characteristicCount(); i++) {
    BLELocalCharacteristic* characteristic = service->characteristic(i);

    characteristic->retain();
    _attributes.add(characteristic);
    characteristic->setHandle(attributeCount());
    
    // add the characteristic again to make space of the characteristic value handle
    characteristic->retain();
    _attributes.add(characteristic);

    for (unsigned int j = 0; j < characteristic->descriptorCount(); j++) {
      BLELocalDescriptor* descriptor = characteristic->descriptor(j);

      descriptor->retain();
      _attributes.add(descriptor);
      descriptor->setHandle(attributeCount());
    }
  }

  service->setHandles(startHandle, attributeCount());
}

void GATTClass::clearAttributes()
{
  for (unsigned int i = 0; i < attributeCount(); i++) {
    BLELocalAttribute* a = attribute(i);

    if (a->release() == 0) {
      delete a;
    }
  }
  _attributes.clear();

  for (unsigned int i = 0; i < _services.size(); i++) {
    _services.get(i)->clear();
  }
  _services.clear();

}

#if !defined(FAKE_GATT)
GATTClass GATTObj;
GATTClass& GATT = GATTObj;
#endif

#endif
