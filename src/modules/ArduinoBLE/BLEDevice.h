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

#ifndef _BLE_DEVICE_H_
#define _BLE_DEVICE_H_

#include <Arduino.h>

#include "BLEService.h"

enum BLEDeviceEvent {
  BLEConnected = 0,
  BLEDisconnected = 1,
  BLEDiscovered = 2,

  BLEDeviceLastEvent
};

class BLEDevice;

typedef void (*BLEDeviceEventHandler)(BLEDevice device);

class BLEDevice {
public:
  BLEDevice();
  virtual ~BLEDevice();

  virtual void poll();
  virtual void poll(unsigned long timeout);

  virtual bool connected() const;
  virtual bool disconnect();

  virtual String address() const;

  bool hasLocalName() const;
    
  bool hasAdvertisedServiceUuid() const;
  bool hasAdvertisedServiceUuid(int index) const;
  int advertisedServiceUuidCount() const;

  String localName() const;
  String advertisedServiceUuid() const;
  String advertisedServiceUuid(int index) const;

  bool hasAdvertisementData() const;
  int advertisementDataLength() const;
  int advertisementData(uint8_t value[], int length) const;

  bool hasManufacturerData() const;
  int manufacturerDataLength() const;
  int manufacturerData(uint8_t value[], int length) const;

  virtual int rssi();

  bool connect();
  bool discoverAttributes();
  bool discoverService(const char* serviceUuid);

  virtual operator bool() const;
  virtual bool operator==(const BLEDevice& rhs) const;
  virtual bool operator!=(const BLEDevice& rhs) const;

  String deviceName();
  int appearance();

  int serviceCount() const; 
  bool hasService(const char* uuid) const;
  bool hasService(const char* uuid, int index) const;
  BLEService service(int index) const;
  BLEService service(const char * uuid) const;
  BLEService service(const char * uuid, int index) const;
  int characteristicCount() const;
  bool hasCharacteristic(const char* uuid) const;
  bool hasCharacteristic(const char* uuid, int index) const;
  BLECharacteristic characteristic(int index) const;
  BLECharacteristic characteristic(const char * uuid) const;
  BLECharacteristic characteristic(const char * uuid, int index) const;

protected:
  friend class ATTClass;
  friend class GAPClass;

  BLEDevice(uint8_t addressType, uint8_t address[6]);

protected:
  friend class GAPClass;

  bool hasAddress(uint8_t addressType, uint8_t address[6]);

  void setAdvertisementData(uint8_t type, uint8_t eirDataLength, uint8_t eirData[], int8_t rssi);
  void setScanResponseData(uint8_t eirDataLength, uint8_t eirData[], int8_t rssi);

  bool discovered();

private:
  uint8_t _addressType;
  uint8_t _address[6];
  uint8_t _advertisementTypeMask;
  uint8_t _eirDataLength;
  uint8_t _eirData[31 * 2];
  int8_t _rssi;
};

#endif
