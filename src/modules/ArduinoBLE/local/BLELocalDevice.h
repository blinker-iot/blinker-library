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

#ifndef _BLE_LOCAL_DEVICE_H_
#define _BLE_LOCAL_DEVICE_H_

#if defined(ARDUINO_ARCH_RENESAS)

#include "../BLEDevice.h"
#include "../BLEService.h"
#include "../BLEAdvertisingData.h"

enum Pairable {
  NO = 0,
  YES = 1,
  ONCE = 2,
};

class BLELocalDevice {
public:
  BLELocalDevice();
  virtual ~BLELocalDevice();

  virtual int begin();
  virtual void end();

  virtual void poll();
  virtual void poll(unsigned long timeout);

  virtual bool connected() const;
  virtual bool disconnect();

  virtual String address() const;

  virtual int rssi();

  virtual bool setAdvertisedServiceUuid(const char* advertisedServiceUuid);
  virtual bool setAdvertisedService(const BLEService& service);
  virtual bool setAdvertisedServiceData(uint16_t uuid, const uint8_t data[], int length);
  virtual bool setManufacturerData(const uint8_t manufacturerData[], int manufacturerDataLength);
  virtual bool setManufacturerData(const uint16_t companyId, const uint8_t manufacturerData[], int manufacturerDataLength);
  virtual bool setLocalName(const char *localName);

  virtual void setAdvertisingData(BLEAdvertisingData& advertisingData);
  virtual void setScanResponseData(BLEAdvertisingData& scanResponseData);

  virtual void setDeviceName(const char* deviceName);
  virtual void setAppearance(uint16_t appearance);

  virtual void addService(BLEService& service);

  virtual int advertise();
  virtual void stopAdvertise();

  virtual int scan(bool withDuplicates = false);
  virtual int scanForName(String name, bool withDuplicates = false);
  virtual int scanForUuid(String uuid, bool withDuplicates = false);
  virtual int scanForAddress(String address, bool withDuplicates = false);
  virtual void stopScan();

  virtual BLEDevice central();
  virtual BLEDevice available();

  virtual void setAdvertisingInterval(uint16_t advertisingInterval);
  virtual void setConnectionInterval(uint16_t minimumConnectionInterval, uint16_t maximumConnectionInterval);
  virtual void setSupervisionTimeout(uint16_t supervisionTimeout);
  virtual void setConnectable(bool connectable); 

  virtual void setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler);

  virtual void setTimeout(unsigned long timeout);

  virtual void debug(Stream& stream);
  virtual void noDebug();
  
  virtual void setPairable(uint8_t pairable);
  virtual bool pairable();
  virtual bool paired();

  // address - The mac to store
  // IRK - The IRK to store with this mac
  virtual void setStoreIRK(int (*storeIRK)(uint8_t* address, uint8_t* IRK));
  // nIRKs      - the number of IRKs being provided.
  // BDAddrType - an array containing the type of each address (0 public, 1 static random)
  // BDAddrs    - an array containing the list of addresses
  virtual void setGetIRKs(int (*getIRKs)(uint8_t* nIRKs, uint8_t** BDAddrType, uint8_t*** BDAddrs, uint8_t*** IRKs));
  // address - the address to store [6 bytes]
  // LTK - the LTK to store with this mac [16 bytes]
  virtual void setStoreLTK(int (*storeLTK)(uint8_t* address, uint8_t* LTK));
  // address - The mac address needing its LTK
  // LTK - 16 octet LTK for the mac address
  virtual void setGetLTK(int (*getLTK)(uint8_t* address, uint8_t* LTK));

  virtual void setDisplayCode(void (*displayCode)(uint32_t confirmationCode));
  virtual void setBinaryConfirmPairing(bool (*binaryConfirmPairing)());
  uint8_t BDaddress[6];
  
protected:
  virtual BLEAdvertisingData& getAdvertisingData();
  virtual BLEAdvertisingData& getScanResponseData();

private:
  BLEAdvertisingData _advertisingData;
  BLEAdvertisingData _scanResponseData;
};

extern BLELocalDevice& BLE;

#endif

#endif
