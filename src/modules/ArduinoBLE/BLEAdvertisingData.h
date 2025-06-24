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

#ifndef _BLE_ADVERTISING_DATA_H_
#define _BLE_ADVERTISING_DATA_H_

#include <Arduino.h>
#include "utility/BLEUuid.h"
#include "BLEService.h"

#define MAX_AD_DATA_LENGTH (31)

enum BLEFlags {
  BLEFlagsLimitedDiscoverable = 0x01,
  BLEFlagsGeneralDiscoverable = 0x02,
  BLEFlagsBREDRNotSupported   = 0x04
};

enum BLEAdField {
  BLEFieldFlags = 0x01,
  BLEFieldIncompleteAdvertisedService16 = 0x02,
  BLEFieldCompleteAdvertisedService16 = 0x03,
  BLEFieldIncompleteAdvertisedService128 = 0x06,
  BLEFieldCompleteAdvertisedService128 = 0x07,
  BLEFieldShortLocalName = 0x08,
  BLEFieldCompleteLocalName = 0x09,
  BLEFieldServiceData = 0x16,
  BLEFieldManufacturerData = 0xFF,

  BLEAdFieldLast
};

struct BLEAdvertisingRawData {
  uint8_t data[MAX_AD_DATA_LENGTH];
  int length;
};

class BLEAdvertisingData {
public:
  BLEAdvertisingData(); 
  virtual ~BLEAdvertisingData();

  int availableForWrite(); 
  void clear();
  void copy(const BLEAdvertisingData& adv);
  BLEAdvertisingData& operator=(const BLEAdvertisingData &other);

  bool setAdvertisedService(const BLEService& service);
  bool setAdvertisedServiceUuid(const char* advertisedServiceUuid);
  bool setManufacturerData(const uint8_t manufacturerData[], int manufacturerDataLength);
  bool setManufacturerData(const uint16_t companyId, const uint8_t manufacturerData[], int manufacturerDataLength);
  bool setLocalName(const char *localName);
  bool setAdvertisedServiceData(uint16_t uuid, const uint8_t data[], int length);
  bool setRawData(const uint8_t* data, int length);
  bool setRawData(const BLEAdvertisingRawData& data);
  bool setFlags(uint8_t flags);

protected:
  friend class BLELocalDevice;
  bool updateData();
  uint8_t* data();
  int dataLength() const;
  int remainingLength() const;
  bool hasFlags() const;

private:
  bool updateRemainingLength(int oldFieldLength, int newFieldLength);

  bool addAdvertisedServiceUuid(const char* advertisedServiceUuid);
  bool addManufacturerData(const uint8_t manufacturerData[], int manufacturerDataLength);
  bool addManufacturerData(const uint16_t companyId, const uint8_t manufacturerData[], int manufacturerDataLength);
  bool addLocalName(const char *localName);
  bool addAdvertisedServiceData(uint16_t uuid, const uint8_t data[], int length);
  bool addRawData(const uint8_t* data, int length);
  bool addFlags(uint8_t flags);

  bool addField(BLEAdField field, const char* data);
  bool addField(BLEAdField field, const uint8_t* data, int length);

  uint8_t _data[MAX_AD_DATA_LENGTH];
  int _dataLength;

  int _remainingLength;

  const uint8_t* _rawData;
  int _rawDataLength;

  uint8_t _flags;
  bool _hasFlags;
  const char* _localName;

  const uint8_t* _manufacturerData;
  int _manufacturerDataLength;
  uint16_t _manufacturerCompanyId;
  bool _hasManufacturerCompanyId;

  const char* _advertisedServiceUuid; 
  int _advertisedServiceUuidLength;
  uint16_t _serviceDataUuid;
  const uint8_t* _serviceData;
  int _serviceDataLength;
};

#endif
