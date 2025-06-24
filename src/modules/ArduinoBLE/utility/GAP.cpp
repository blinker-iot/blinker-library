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

#include "BLEUuid.h"
#include "HCI.h"

#include "GAP.h"

#define GAP_MAX_DISCOVERED_QUEUE_SIZE 32

#define GAP_ADV_IND (0x00)
#define GAP_ADV_SCAN_IND (0x02)
#define GAP_ADV_NONCONN_IND (0x03)

GAPClass::GAPClass() :
  _advertising(false),
  _scanning(false),
  _advertisingInterval(160),
  _connectable(true),
  _discoverEventHandler(NULL)
{
}

GAPClass::~GAPClass()
{
}

bool GAPClass::advertising()
{
  return _advertising;
}

int GAPClass::advertise(uint8_t* advData, uint8_t advDataLen, uint8_t* scanData, uint8_t scanDataLen)
{
  uint8_t directBdaddr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  uint8_t type = (_connectable) ? GAP_ADV_IND : (scanDataLen ? GAP_ADV_SCAN_IND : GAP_ADV_NONCONN_IND);

  stopAdvertise();

  if (HCI.leSetAdvertisingParameters(_advertisingInterval, _advertisingInterval, type, 0x00, 0x00, directBdaddr, 0x07, 0) != 0) {
    return 0;
  }

  if (HCI.leSetAdvertisingData(advDataLen, advData) != 0) {
    return 0;
  }

  if (HCI.leSetScanResponseData(scanDataLen, scanData) != 0) {
    return 0;
  }

  if (HCI.leSetAdvertiseEnable(0x01) != 0) {
    return 0;
  }

  _advertising = true;

  return 1;
}

void GAPClass::stopAdvertise()
{
  _advertising = false;

  HCI.leSetAdvertiseEnable(0x00);
}

int GAPClass::scan(bool withDuplicates)
{
  HCI.leSetScanEnable(false, true);

  // active scan, 20 ms scan interval (N * 0.625), 20 ms scan window (N * 0.625), public own address type, no filter
  /*
    Warning (from BLUETOOTH SPECIFICATION 5.x):
    - scan interval: mandatory range from 0x0012 to 0x1000; only even values are valid
    - scan window: mandatory range from 0x0011 to 0x1000
    - The scan window can only be less than or equal to the scan interval
  */
  if (HCI.leSetScanParameters(0x01, 0x0020, 0x0020, 0x00, 0x00) != 0) {
    return false;
  }

  _scanning = true;

  if (HCI.leSetScanEnable(true, !withDuplicates) != 0) {
    return 0;
  }

  return 1;
}

int GAPClass::scanForName(String name, bool withDuplicates)
{
  _scanNameFilter    = name;
  _scanUuidFilter    = "";
  _scanAddressFilter = "";

  return scan(withDuplicates);
}

int GAPClass::scanForUuid(String uuid, bool withDuplicates)
{
  _scanNameFilter    = "";
  _scanUuidFilter    = uuid;
  _scanAddressFilter = "";

  return scan(withDuplicates);
}

int GAPClass::scanForAddress(String address, bool withDuplicates)
{
  _scanNameFilter    = "";
  _scanUuidFilter    = "";
  _scanAddressFilter = address;

  return scan(withDuplicates);
}

void GAPClass::stopScan()
{
  HCI.leSetScanEnable(false, false);

  _scanning = false;

  for (unsigned int i = 0; i < _discoveredDevices.size(); i++) {
    BLEDevice* device = _discoveredDevices.get(i);

    delete device;
  }

  _discoveredDevices.clear();
}

BLEDevice GAPClass::available()
{
  for (unsigned int i = 0; i < _discoveredDevices.size(); i++) {
    BLEDevice* device = _discoveredDevices.get(i);

    if (device->discovered()) {
      BLEDevice result = *device;

      _discoveredDevices.remove(i);
      delete device;

      if (matchesScanFilter(result)) {
        return result;
      } else {
        continue;
      } 
    }
  }

  return BLEDevice();
}

void GAPClass::setAdvertisingInterval(uint16_t advertisingInterval)
{
  _advertisingInterval = advertisingInterval;
}

void GAPClass::setConnectable(bool connectable)
{
  _connectable = connectable;
}

void GAPClass::setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler)
{
  if (event == BLEDiscovered) {
    _discoverEventHandler = eventHandler;
  }
}

void GAPClass::handleLeAdvertisingReport(uint8_t type, uint8_t addressType, uint8_t address[6],
                                          uint8_t eirLength, uint8_t eirData[], int8_t rssi)
{
  if (!_scanning) {
    return;
  }

  if (_discoverEventHandler && type == 0x03) {
    // call event handler and skip adding to discover list
    BLEDevice device(addressType, address);

    device.setAdvertisementData(type, eirLength, eirData, rssi);

    if (matchesScanFilter(device)) {
      _discoverEventHandler(device);
    }
    return;
  }

  BLEDevice* discoveredDevice = NULL;
  int discoveredIndex = -1;

  for (unsigned int i = 0; i < _discoveredDevices.size(); i++) {
    BLEDevice* device = _discoveredDevices.get(i);

    if (device->hasAddress(addressType, address)) {
      discoveredDevice = device;
      discoveredIndex = i;

      break;
    }
  }

  if (discoveredDevice == NULL) {
    if (_discoveredDevices.size() >= GAP_MAX_DISCOVERED_QUEUE_SIZE) {
      BLEDevice* device_first = _discoveredDevices.remove(0);
      if (device_first != NULL) {
        delete device_first;
      }
    }

    discoveredDevice = new BLEDevice(addressType, address);

    _discoveredDevices.add(discoveredDevice);
    discoveredIndex = _discoveredDevices.size() - 1;
  }

  if (type != 0x04) {
    discoveredDevice->setAdvertisementData(type, eirLength, eirData, rssi);
  } else {
    discoveredDevice->setScanResponseData(eirLength, eirData, rssi);
  }

  if (discoveredDevice->discovered() && _discoverEventHandler) {
    // remove from list and report as discovered
    BLEDevice device = *discoveredDevice;

    _discoveredDevices.remove(discoveredIndex);
    delete discoveredDevice;

    if (matchesScanFilter(device)) {
      _discoverEventHandler(device);
    }
  }
}

bool GAPClass::matchesScanFilter(const BLEDevice& device)
{
  if (_scanAddressFilter.length() > 0 && !(_scanAddressFilter.equalsIgnoreCase(device.address()))) {
    return false; // drop doesn't match
  } else if (_scanNameFilter.length() > 0 && _scanNameFilter != device.localName()) {
    return false; // drop doesn't match
  } else if (_scanUuidFilter.length() > 0 && !(_scanUuidFilter.equalsIgnoreCase(device.advertisedServiceUuid()))) {
    return false; // drop doesn't match
  }

  return true;
}

#if !defined(FAKE_GAP)
GAPClass GAPObj;
GAPClass& GAP = GAPObj;
#endif
