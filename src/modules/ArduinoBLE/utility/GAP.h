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

#ifndef _GAP_H_
#define _GAP_H_

#if defined(ARDUINO_ARCH_RENESAS)

#include "../utility/BLELinkedList.h"

#include "../BLEDevice.h"

class GAPClass {
public:
  GAPClass();
  virtual ~GAPClass();

  virtual bool advertising();
  virtual int advertise(uint8_t* advData, uint8_t advDataLength, uint8_t* scanData, uint8_t scanDataLength);
  virtual void stopAdvertise();

  virtual int scan(bool withDuplicates);
  virtual int scanForName(String name, bool withDuplicates);
  virtual int scanForUuid(String uuid, bool withDuplicates);
  virtual int scanForAddress(String address, bool withDuplicates);
  virtual void stopScan();
  virtual BLEDevice available();

  virtual void setAdvertisingInterval(uint16_t advertisingInterval);
  virtual void setConnectable(bool connectable);

  virtual void setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler);

protected:
  friend class HCIClass;

  virtual void handleLeAdvertisingReport(uint8_t type, uint8_t addressType, uint8_t address[6],
                                  uint8_t eirLength, uint8_t eirData[], int8_t rssi);

private:
  virtual bool matchesScanFilter(const BLEDevice& device);

private:
  bool _advertising;
  bool _scanning;

  uint16_t _advertisingInterval;
  bool _connectable;

  BLEDeviceEventHandler _discoverEventHandler;
  BLELinkedList<BLEDevice*> _discoveredDevices;

  String _scanNameFilter;
  String _scanUuidFilter;
  String _scanAddressFilter;
};

extern GAPClass& GAP;

#endif

#endif
