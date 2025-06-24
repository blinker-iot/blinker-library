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

#include "../utility/ATT.h"
#include "../utility/HCI.h"
#include "../utility/GAP.h"
#include "../utility/GATT.h"
#include "../utility/L2CAPSignaling.h"

#include "BLELocalDevice.h"

#if defined(PORTENTA_H7_PINS) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_OPTA)
#ifndef BT_REG_ON
#define BT_REG_ON PJ_12
#endif
#elif defined(ARDUINO_NICLA_VISION)
#ifndef BT_REG_ON
#define BT_REG_ON PF_14
#endif
#elif defined(ARDUINO_GIGA)
#ifndef BT_REG_ON
#define BT_REG_ON PA_10
#endif
#endif

BLELocalDevice::BLELocalDevice()
{
  _advertisingData.setFlags(BLEFlagsGeneralDiscoverable | BLEFlagsBREDRNotSupported);
}

BLELocalDevice::~BLELocalDevice()
{
}

int BLELocalDevice::begin()
{
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
  // reset the NINA in BLE mode
  pinMode(SPIWIFI_SS, OUTPUT);
  pinMode(NINA_RESETN, OUTPUT);
  
  digitalWrite(SPIWIFI_SS, LOW);
#endif

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
  digitalWrite(NINA_RESETN, HIGH);
  delay(100);
  digitalWrite(NINA_RESETN, LOW);
  delay(750);
#elif defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
  // inverted reset
  digitalWrite(NINA_RESETN, LOW);
  delay(100);
  digitalWrite(NINA_RESETN, HIGH);
  delay(750);
#elif defined(PORTENTA_H7_PINS) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
  // BT_REG_ON -> HIGH
  pinMode(BT_REG_ON, OUTPUT);
  digitalWrite(BT_REG_ON, LOW);
  delay(500);
  digitalWrite(BT_REG_ON, HIGH);
  delay(500);
#elif defined(ARDUINO_PORTENTA_C33)
#define NINA_GPIO0      (100)
#define NINA_RESETN     (101)
  pinMode(NINA_GPIO0, OUTPUT);
  pinMode(NINA_RESETN, OUTPUT);
  Serial5.begin(921600);

  digitalWrite(NINA_GPIO0, HIGH);
  delay(100);
  digitalWrite(NINA_RESETN, HIGH);
  digitalWrite(NINA_RESETN, LOW);
  digitalWrite(NINA_RESETN, HIGH);
  auto _start = millis();
  while (millis() - _start < 500) {
    if (Serial5.available()) {
      Serial5.read();
    }
  }
  //pinMode(94, OUTPUT);
  //digitalWrite(94, LOW);
#endif


#ifdef ARDUINO_AVR_UNO_WIFI_REV2
  // set SS HIGH
  digitalWrite(SPIWIFI_SS, HIGH);

  // set RTS HIGH
  pinMode(NINA_RTS, OUTPUT);
  digitalWrite(NINA_RTS, HIGH);

  // set CTS as input
  pinMode(NINA_CTS, INPUT);
#endif

  if (!HCI.begin()) {
    end();
    return 0;
  }

//   HCI.debug(Serial);

  delay(100);

  if (HCI.reset() != 0) {
    end();

    return 0;
  }

  uint8_t hciVer;
  uint16_t hciRev;
  uint8_t lmpVer;
  uint16_t manufacturer;
  uint16_t lmpSubVer;

  if (HCI.readLocalVersion(hciVer, hciRev, lmpVer, manufacturer, lmpSubVer) != 0) {
    end();
    return 0;
  }

  if (HCI.setEventMask(0x3FFFFFFFFFFFFFFF) != 0) {
    end();
    return 0;
  }
  if (HCI.setLeEventMask(0x00000000000003FF) != 0) {
    end();
    return 0;
  }

  uint16_t pktLen;
  uint8_t maxPkt;

  if (HCI.readLeBufferSize(pktLen, maxPkt) != 0) {
    end();
    return 0;
  }

  /// The HCI should allow automatic address resolution.

  // // If we have callbacks to remember bonded devices:
  // if(HCI._getIRKs!=0){
  //   uint8_t nIRKs = 0;
  //   uint8_t** BADDR_Type = new uint8_t*;
  //   uint8_t*** BADDRs = new uint8_t**;
  //   uint8_t*** IRKs = new uint8_t**;
  //   uint8_t* memcheck;


  //   if(!HCI._getIRKs(&nIRKs, BADDR_Type, BADDRs, IRKs)){
  //     Serial.println("error");
  //   }
  //   for(int i=0; i<nIRKs; i++){
  //     Serial.print("Baddr type: ");
  //     Serial.println((*BADDR_Type)[i]);
  //     Serial.print("BADDR:");
  //     for(int k=0; k<6; k++){
  //       Serial.print(", 0x");
  //       Serial.print((*BADDRs)[i][k],HEX);
  //     }
  //     Serial.println();
  //     Serial.print("IRK:");
  //     for(int k=0; k<16; k++){
  //       Serial.print(", 0x");
  //       Serial.print((*IRKs)[i][k],HEX);
  //     }
  //     Serial.println();

  //     // save this 
  //     uint8_t zeros[16];
  //     for(int k=0; k<16; k++) zeros[15-k] = 0;
      
  //     // HCI.leAddResolvingAddress((*BADDR_Type)[i],(*BADDRs)[i],(*IRKs)[i], zeros);

  //     delete[] (*BADDRs)[i];
  //     delete[] (*IRKs)[i];
  //   }
  //   delete[] (*BADDR_Type);
  //   delete BADDR_Type;
  //   delete[] (*BADDRs);
  //   delete BADDRs;
  //   delete[] (*IRKs);
  //   delete IRKs;
    
  //   memcheck = new uint8_t[1];
  //   Serial.print("nIRK location: 0x");
  //   Serial.println((int)memcheck,HEX);
  //   delete[] memcheck;

  // }

  GATT.begin();

  return 1;
}

void BLELocalDevice::end()
{
  GATT.end();

  HCI.end();

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
  // disable the NINA
  digitalWrite(NINA_RESETN, HIGH);
#elif defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
  // disable the NINA
  digitalWrite(NINA_RESETN, LOW);
#elif defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
  digitalWrite(BT_REG_ON, LOW);
#endif 
  _advertisingData.clear();
  _scanResponseData.clear();
}

void BLELocalDevice::poll()
{
  HCI.poll();
}

void BLELocalDevice::poll(unsigned long timeout)
{
  HCI.poll(timeout);
}

bool BLELocalDevice::connected() const
{
  HCI.poll();

  return ATT.connected();
}

/*
 * Whether there is at least one paired device
 */
bool BLELocalDevice::paired()
{
  HCI.poll();

  return ATT.paired();
}

bool BLELocalDevice::disconnect()
{
  return ATT.disconnect();
}

String BLELocalDevice::address() const
{
  uint8_t addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  HCI.readBdAddr(addr);

  char result[18];
  sprintf(result, "%02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

  return result;
}

int BLELocalDevice::rssi()
{
  BLEDevice central = ATT.central();

  if (central) {
    return central.rssi();
  }

  return 127;
}

bool BLELocalDevice::setAdvertisedServiceUuid(const char* advertisedServiceUuid)
{
  return _advertisingData.setAdvertisedServiceUuid(advertisedServiceUuid);
}

bool BLELocalDevice::setAdvertisedService(const BLEService& service)
{
  return setAdvertisedServiceUuid(service.uuid());
}

bool BLELocalDevice::setAdvertisedServiceData(uint16_t uuid, const uint8_t data[], int length)
{
  return _advertisingData.setAdvertisedServiceData(uuid, data, length);
}

bool BLELocalDevice::setManufacturerData(const uint8_t manufacturerData[], int manufacturerDataLength)
{
  return _advertisingData.setManufacturerData(manufacturerData, manufacturerDataLength);
}

bool BLELocalDevice::setManufacturerData(const uint16_t companyId, const uint8_t manufacturerData[], int manufacturerDataLength)
{
  return _advertisingData.setManufacturerData(companyId, manufacturerData, manufacturerDataLength);
}

bool BLELocalDevice::setLocalName(const char *localName)
{
  return _scanResponseData.setLocalName(localName);  
}

void BLELocalDevice::setAdvertisingData(BLEAdvertisingData& advertisingData)
{
  _advertisingData = advertisingData;
  if (!_advertisingData.hasFlags()) {
    _advertisingData.setFlags(BLEFlagsGeneralDiscoverable | BLEFlagsBREDRNotSupported);
  }
}

void BLELocalDevice::setScanResponseData(BLEAdvertisingData& scanResponseData)
{
  _scanResponseData = scanResponseData;
}

BLEAdvertisingData& BLELocalDevice::getAdvertisingData()
{
  return _advertisingData;
}

BLEAdvertisingData& BLELocalDevice::getScanResponseData()
{
  return _scanResponseData;
}

void BLELocalDevice::setDeviceName(const char* deviceName)
{
  GATT.setDeviceName(deviceName);
}

void BLELocalDevice::setAppearance(uint16_t appearance)
{
  GATT.setAppearance(appearance);
}

void BLELocalDevice::addService(BLEService& service)
{
  GATT.addService(service);
}

int BLELocalDevice::advertise()
{
  _advertisingData.updateData();
  _scanResponseData.updateData();
  return GAP.advertise( _advertisingData.data(), _advertisingData.dataLength(), 
                        _scanResponseData.data(), _scanResponseData.dataLength());
}

void BLELocalDevice::stopAdvertise()
{
  GAP.stopAdvertise();
}

int BLELocalDevice::scan(bool withDuplicates)
{
  return GAP.scan(withDuplicates);
}

int BLELocalDevice::scanForName(String name, bool withDuplicates)
{
  return GAP.scanForName(name, withDuplicates);
}

int BLELocalDevice::scanForUuid(String uuid, bool withDuplicates)
{
  return GAP.scanForUuid(uuid, withDuplicates);
}

int BLELocalDevice::scanForAddress(String address, bool withDuplicates)
{
  return GAP.scanForAddress(address, withDuplicates);
}

void BLELocalDevice::stopScan()
{
  GAP.stopScan();
}

BLEDevice BLELocalDevice::central()
{
  HCI.poll();

  return ATT.central();
}

BLEDevice BLELocalDevice::available()
{
  HCI.poll();

  return GAP.available();
}

void BLELocalDevice::setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler)
{
  if (event == BLEDiscovered) {
    GAP.setEventHandler(event, eventHandler);
  } else {
    ATT.setEventHandler(event, eventHandler);
  }
}

void BLELocalDevice::setAdvertisingInterval(uint16_t advertisingInterval)
{
  GAP.setAdvertisingInterval(advertisingInterval);
}

void BLELocalDevice::setConnectionInterval(uint16_t minimumConnectionInterval, uint16_t maximumConnectionInterval)
{
  L2CAPSignaling.setConnectionInterval(minimumConnectionInterval, maximumConnectionInterval);
}

void BLELocalDevice::setSupervisionTimeout(uint16_t supervisionTimeout)
{
  L2CAPSignaling.setSupervisionTimeout(supervisionTimeout);
}

void BLELocalDevice::setConnectable(bool connectable)
{
  GAP.setConnectable(connectable);
}

void BLELocalDevice::setTimeout(unsigned long timeout)
{
  ATT.setTimeout(timeout);
}

/*
 * Control whether pairing is allowed or rejected
 * Use true/false or the Pairable enum
 */
void BLELocalDevice::setPairable(uint8_t pairable)
{
  L2CAPSignaling.setPairingEnabled(pairable);
}

/*
 * Whether pairing is currently allowed
 */
bool BLELocalDevice::pairable()
{
  return L2CAPSignaling.isPairingEnabled();
}

void BLELocalDevice::setGetIRKs(int (*getIRKs)(uint8_t* nIRKs, uint8_t** BADDR_type, uint8_t*** BADDRs, uint8_t*** IRKs)){
  HCI._getIRKs = getIRKs;
}
void BLELocalDevice::setGetLTK(int (*getLTK)(uint8_t* BADDR, uint8_t* LTK)){
  HCI._getLTK = getLTK;
}
void BLELocalDevice::setStoreLTK(int (*storeLTK)(uint8_t*, uint8_t*)){
  HCI._storeLTK = storeLTK;
}
void BLELocalDevice::setStoreIRK(int (*storeIRK)(uint8_t*, uint8_t*)){
  HCI._storeIRK = storeIRK;
}
void BLELocalDevice::setDisplayCode(void (*displayCode)(uint32_t confirmationCode)){
  HCI._displayCode = displayCode;
}
void BLELocalDevice::setBinaryConfirmPairing(bool (*binaryConfirmPairing)()){
  HCI._binaryConfirmPairing = binaryConfirmPairing;
}

void BLELocalDevice::debug(Stream& stream)
{
  HCI.debug(stream);
}

void BLELocalDevice::noDebug()
{
  HCI.noDebug();
}

#if !defined(FAKE_BLELOCALDEVICE)
BLELocalDevice BLEObj;
BLELocalDevice& BLE = BLEObj;
#endif
