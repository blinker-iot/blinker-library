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

#include "ATT.h"
#include "GAP.h"
#include "HCITransport.h"
#include "L2CAPSignaling.h"
#include "btct.h"
#include "HCI.h"
#include "bitDescriptions.h"
// #define _BLE_TRACE_


#define HCI_COMMAND_PKT   0x01
#define HCI_ACLDATA_PKT   0x02
#define HCI_EVENT_PKT     0x04
#define HCI_SECURITY_PKT  0x06

#define EVT_DISCONN_COMPLETE  0x05
#define EVT_ENCRYPTION_CHANGE 0x08
#define EVT_CMD_COMPLETE      0x0e
#define EVT_CMD_STATUS        0x0f
#define EVT_NUM_COMP_PKTS     0x13
#define EVT_RETURN_LINK_KEYS  0x15
#define EVT_UNKNOWN           0x10
#define EVT_LE_META_EVENT     0x3e

#define EVT_LE_CONN_COMPLETE      0x01
#define EVT_LE_ADVERTISING_REPORT 0x02


// OGF_LINK_CTL
#define OCF_DISCONNECT         0x0006

// OGF_HOST_CTL
#define OCF_SET_EVENT_MASK     0x0001
#define OCF_RESET              0x0003

// OGF_INFO_PARAM
#define OCF_READ_LOCAL_VERSION 0x0001
#define OCF_READ_BD_ADDR       0x0009

// OGF_STATUS_PARAM
#define OCF_READ_RSSI          0x0005

// OGF_LE_CTL
#define OCF_LE_READ_BUFFER_SIZE           0x0002
#define OCF_LE_SET_RANDOM_ADDRESS         0x0005
#define OCF_LE_SET_ADVERTISING_PARAMETERS 0x0006
#define OCF_LE_SET_ADVERTISING_DATA       0x0008
#define OCF_LE_SET_SCAN_RESPONSE_DATA     0x0009
#define OCF_LE_SET_ADVERTISE_ENABLE       0x000a
#define OCF_LE_SET_SCAN_PARAMETERS        0x000b
#define OCF_LE_SET_SCAN_ENABLE            0x000c
#define OCF_LE_CREATE_CONN                0x000d
#define OCF_LE_CANCEL_CONN                0x000e
#define OCF_LE_CONN_UPDATE                0x0013

#define HCI_OE_USER_ENDED_CONNECTION 0x13

String metaEventToString(LE_META_EVENT event)
{
  switch(event){
    case CONN_COMPLETE: return F("CONN_COMPLETE");
    case ADVERTISING_REPORT: return F("ADVERTISING_REPORT");
    case LONG_TERM_KEY_REQUEST: return F("LE_LONG_TERM_KEY_REQUEST");
    case READ_LOCAL_P256_COMPLETE: return F("READ_LOCAL_P256_COMPLETE");
    case GENERATE_DH_KEY_COMPLETE: return F("GENERATE_DH_KEY_COMPLETE");
    case ENHANCED_CONN_COMPLETE: return F("ENHANCED_CONN_COMPLETE");
    default: return "event unknown";
  }
}
String commandToString(LE_COMMAND command){
  switch (command)
  {
    case ENCRYPT: return F("ENCRYPT");
    case LONG_TERM_KEY_REPLY: return F("LONG_TERM_KEY_REPLY");
    case READ_LOCAL_P256: return F("READ_LOCAL_P256");
    case GENERATE_DH_KEY_V1: return F("GENERATE_DH_KEY_V1");
    case GENERATE_DH_KEY_V2: return F("GENERATE_DH_KEY_V2");
    default: return "UNKNOWN";
  }
}

HCIClass::HCIClass() :
  _debug(NULL),
  _recvIndex(0),
  _pendingPkt(0)
{
}

HCIClass::~HCIClass()
{
}

int HCIClass::begin()
{
  _recvIndex = 0;

  return HCITransport.begin();
}

void HCIClass::end()
{
  HCITransport.end();
}

void HCIClass::poll()
{
  poll(0);
}

void HCIClass::poll(unsigned long timeout)
{
#ifdef ARDUINO_AVR_UNO_WIFI_REV2
  digitalWrite(NINA_RTS, LOW);
#endif

  if (timeout) {
    HCITransport.wait(timeout);
  }

  while (HCITransport.available()) {
    byte b = HCITransport.read();

    if (_recvIndex >= (int)sizeof(_recvBuffer)) {
        _recvIndex = 0;
        if (_debug) {
            _debug->println("_recvBuffer overflow");
        }
    }

    _recvBuffer[_recvIndex++] = b;

    if (_recvBuffer[0] == HCI_ACLDATA_PKT) {
      if (_recvIndex > 5 && _recvIndex >= (5 + (_recvBuffer[3] + (_recvBuffer[4] << 8)))) {
        if (_debug) {
          dumpPkt("HCI ACLDATA RX <- ", _recvIndex, _recvBuffer);
        }
#ifdef ARDUINO_AVR_UNO_WIFI_REV2
        digitalWrite(NINA_RTS, HIGH);
#endif
        int pktLen = _recvIndex - 1;
        _recvIndex = 0;

        handleAclDataPkt(pktLen, &_recvBuffer[1]);

#ifdef ARDUINO_AVR_UNO_WIFI_REV2
        digitalWrite(NINA_RTS, LOW);
#endif
      }
    } else if (_recvBuffer[0] == HCI_EVENT_PKT) {
      if (_recvIndex > 3 && _recvIndex >= (3 + _recvBuffer[2])) {
        if (_debug) {
          dumpPkt("HCI EVENT RX <- ", _recvIndex, _recvBuffer);
        }
#ifdef ARDUINO_AVR_UNO_WIFI_REV2
        digitalWrite(NINA_RTS, HIGH);
#endif
        // received full event
        int pktLen = _recvIndex - 1;
        _recvIndex = 0;

        handleEventPkt(pktLen, &_recvBuffer[1]);

#ifdef ARDUINO_AVR_UNO_WIFI_REV2
        digitalWrite(NINA_RTS, LOW);
#endif
      }
    } else {
      _recvIndex = 0;

      if (_debug) {
        _debug->println(b, HEX);
      }
    }
  }

#ifdef ARDUINO_AVR_UNO_WIFI_REV2
  digitalWrite(NINA_RTS, HIGH);
#endif
}

int HCIClass::reset()
{
  return sendCommand(OGF_HOST_CTL << 10 | OCF_RESET);
}

int HCIClass::readLocalVersion(uint8_t& hciVer, uint16_t& hciRev, uint8_t& lmpVer, uint16_t& manufacturer, uint16_t& lmpSubVer)
{
  int result = sendCommand(OGF_INFO_PARAM << 10 | OCF_READ_LOCAL_VERSION);

  if (result == 0) {
    struct __attribute__ ((packed)) HCILocalVersion {
      uint8_t hciVer;
      uint16_t hciRev;
      uint8_t lmpVer;
      uint16_t manufacturer;
      uint16_t lmpSubVer;
    } *localVersion = (HCILocalVersion*)_cmdResponse;

    hciVer = localVersion->hciVer;
    hciRev = localVersion->hciRev;
    lmpVer = localVersion->lmpVer;
    manufacturer = localVersion->manufacturer;
    lmpSubVer = localVersion->lmpSubVer;
  }

  return result;
}

int HCIClass::readBdAddr(uint8_t addr[6])
{
  int result = sendCommand(OGF_INFO_PARAM << 10 | OCF_READ_BD_ADDR);

  if (result == 0) {
    memcpy(addr, _cmdResponse, 6);
  }

  return result;
}

int HCIClass::readBdAddr(){
  uint8_t response[6];
  int result = readBdAddr(response);
  if(result==0){
    for(int i=0; i<6; i++){
      localAddr[5-i] = _cmdResponse[i];
    }
  }
  return result;
}

int HCIClass::readRssi(uint16_t handle)
{
  int result = sendCommand(OGF_STATUS_PARAM << 10 | OCF_READ_RSSI, sizeof(handle), &handle);
  int rssi = 127;

  if (result == 0) {
    struct __attribute__ ((packed)) HCIReadRssi {
      uint16_t handle;
        int8_t rssi;
    } *readRssi = (HCIReadRssi*)_cmdResponse;

    if (readRssi->handle == handle) {
      rssi = readRssi->rssi;
    }
  }

  return rssi;
}

int HCIClass::setEventMask(uint64_t eventMask)
{
  return sendCommand(OGF_HOST_CTL << 10 | OCF_SET_EVENT_MASK, sizeof(eventMask), &eventMask);
}
// Set LE Event mask
int HCIClass::setLeEventMask(uint64_t leEventMask)
{
  return sendCommand(OGF_LE_CTL << 10 | 0x01, sizeof(leEventMask), &leEventMask);
}

int HCIClass::readLeBufferSize(uint16_t& pktLen, uint8_t& maxPkt)
{
  int result = sendCommand(OGF_LE_CTL << 10 | OCF_LE_READ_BUFFER_SIZE);

  if (result == 0) {
    struct __attribute__ ((packed)) HCILeBufferSize {
      uint16_t pktLen;
      uint8_t maxPkt;
    } *leBufferSize = (HCILeBufferSize*)_cmdResponse;

    pktLen = leBufferSize->pktLen;
    _maxPkt = maxPkt = leBufferSize->maxPkt;

#ifndef __AVR__
    ATT.setMaxMtu(pktLen - 9); // max pkt len - ACL header size
#endif
  }

  return result;
}

int HCIClass::leSetRandomAddress(uint8_t addr[6])
{
  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_RANDOM_ADDRESS, 6, addr);
}

int HCIClass::leSetAdvertisingParameters(uint16_t minInterval, uint16_t maxInterval,
                                         uint8_t advType, uint8_t ownBdaddrType,
                                         uint8_t directBdaddrType, uint8_t directBdaddr[6],
                                         uint8_t chanMap,
                                         uint8_t filter)
{
  struct __attribute__ ((packed)) HCILeAdvertisingParameters {
    uint16_t minInterval;
    uint16_t maxInterval;
    uint8_t advType;
    uint8_t ownBdaddrType;
    uint8_t directBdaddrType;
    uint8_t directBdaddr[6];
    uint8_t chanMap;
    uint8_t filter;
  } leAdvertisingParamters;

  leAdvertisingParamters.minInterval = minInterval;
  leAdvertisingParamters.maxInterval = maxInterval;
  leAdvertisingParamters.advType = advType;
  leAdvertisingParamters.ownBdaddrType = ownBdaddrType;
  leAdvertisingParamters.directBdaddrType = directBdaddrType;
  memcpy(leAdvertisingParamters.directBdaddr, directBdaddr, 6);
  leAdvertisingParamters.chanMap = chanMap;
  leAdvertisingParamters.filter = filter;

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_ADVERTISING_PARAMETERS, sizeof(leAdvertisingParamters), &leAdvertisingParamters);
}

int HCIClass::leSetAdvertisingData(uint8_t length, uint8_t data[])
{
  struct __attribute__ ((packed)) HCILeAdvertisingData {
    uint8_t length;
    uint8_t data[31];
  } leAdvertisingData;

  memset(&leAdvertisingData, 0, sizeof(leAdvertisingData));
  leAdvertisingData.length = length;
  memcpy(leAdvertisingData.data, data, length);

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_ADVERTISING_DATA, sizeof(leAdvertisingData), &leAdvertisingData);
}

int HCIClass::leSetScanResponseData(uint8_t length, uint8_t data[])
{
  struct __attribute__ ((packed)) HCILeScanResponseData {
    uint8_t length;
    uint8_t data[31];
  } leScanResponseData;

  memset(&leScanResponseData, 0, sizeof(leScanResponseData));
  leScanResponseData.length = length;
  memcpy(leScanResponseData.data, data, length);

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_SCAN_RESPONSE_DATA, sizeof(leScanResponseData), &leScanResponseData);
}

int HCIClass::leSetAdvertiseEnable(uint8_t enable)
{
  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_ADVERTISE_ENABLE, sizeof(enable), &enable);
}

int HCIClass::leSetScanParameters(uint8_t type, uint16_t interval, uint16_t window,
                          uint8_t ownBdaddrType, uint8_t filter)
{
  struct __attribute__ ((packed)) HCILeSetScanParameters {
    uint8_t type;
    uint16_t interval;
    uint16_t window;
    uint8_t ownBdaddrType;
    uint8_t filter;
  } leScanParameters;

  leScanParameters.type = type;
  leScanParameters.interval = interval;
  leScanParameters.window = window;
  leScanParameters.ownBdaddrType = ownBdaddrType;
  leScanParameters.filter = filter;

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_SCAN_PARAMETERS, sizeof(leScanParameters), &leScanParameters);
}

int HCIClass::leSetScanEnable(uint8_t enabled, uint8_t duplicates)
{
  struct __attribute__ ((packed)) HCILeSetScanEnableData {
    uint8_t enabled;
    uint8_t duplicates;
  } leScanEnableData;

  leScanEnableData.enabled = enabled;
  leScanEnableData.duplicates = duplicates;

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_SET_SCAN_ENABLE, sizeof(leScanEnableData), &leScanEnableData);
}

int HCIClass::leCreateConn(uint16_t interval, uint16_t window, uint8_t initiatorFilter,
                            uint8_t peerBdaddrType, uint8_t peerBdaddr[6], uint8_t ownBdaddrType,
                            uint16_t minInterval, uint16_t maxInterval, uint16_t latency,
                            uint16_t supervisionTimeout, uint16_t minCeLength, uint16_t maxCeLength)
{
  struct __attribute__ ((packed)) HCILeCreateConnData {
    uint16_t interval;
    uint16_t window;
    uint8_t initiatorFilter;
    uint8_t peerBdaddrType;
    uint8_t peerBdaddr[6];
    uint8_t ownBdaddrType;
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t latency;
    uint16_t supervisionTimeout;
    uint16_t minCeLength;
    uint16_t maxCeLength;
  } leCreateConnData;

  leCreateConnData.interval = interval;
  leCreateConnData.window = window;
  leCreateConnData.initiatorFilter = initiatorFilter;
  leCreateConnData.peerBdaddrType = peerBdaddrType;
  memcpy(leCreateConnData.peerBdaddr, peerBdaddr, sizeof(leCreateConnData.peerBdaddr));
  leCreateConnData.ownBdaddrType = ownBdaddrType;
  leCreateConnData.minInterval = minInterval;
  leCreateConnData.maxInterval = maxInterval;
  leCreateConnData.latency = latency;
  leCreateConnData.supervisionTimeout = supervisionTimeout;
  leCreateConnData.minCeLength = minCeLength;
  leCreateConnData.maxCeLength = maxCeLength;

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_CREATE_CONN, sizeof(leCreateConnData), &leCreateConnData);
}

int HCIClass::leCancelConn()
{
  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_CANCEL_CONN, 0, NULL);
}

int HCIClass::leConnUpdate(uint16_t handle, uint16_t minInterval, uint16_t maxInterval,
                          uint16_t latency, uint16_t supervisionTimeout)
{
  struct __attribute__ ((packed)) HCILeConnUpdateData {
    uint16_t handle;
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t latency;
    uint16_t supervisionTimeout;
    uint16_t minCeLength;
    uint16_t maxCeLength;
  } leConnUpdateData;

  leConnUpdateData.handle = handle;
  leConnUpdateData.minInterval = minInterval;
  leConnUpdateData.maxInterval = maxInterval;
  leConnUpdateData.latency = latency;
  leConnUpdateData.supervisionTimeout = supervisionTimeout;
  leConnUpdateData.minCeLength = 0x0004;
  leConnUpdateData.maxCeLength = 0x0006;

  return sendCommand(OGF_LE_CTL << 10 | OCF_LE_CONN_UPDATE, sizeof(leConnUpdateData), &leConnUpdateData);
}
void HCIClass::saveNewAddress(uint8_t addressType, uint8_t* address, uint8_t* peerIrk, uint8_t* localIrk){
  (void)addressType;
  (void)localIrk;
  if(_storeIRK!=0){
    _storeIRK(address, peerIrk);
  }
  // Again... this should work
  // leAddResolvingAddress(addressType, address, peerIrk, localIrk);
}
void HCIClass::leAddResolvingAddress(uint8_t addressType, uint8_t* peerAddress, uint8_t* peerIrk, uint8_t* localIrk){
  leStopResolvingAddresses();

  struct __attribute__ ((packed)) AddDevice {
    uint8_t peerAddressType;
    uint8_t peerAddress[6];
    uint8_t peerIRK[16];
    uint8_t localIRK[16];
  } addDevice;
  addDevice.peerAddressType = addressType;
  for(int i=0; i<6; i++) addDevice.peerAddress[5-i] = peerAddress[i];
  for(int i=0; i<16; i++) {
    addDevice.peerIRK[15-i]  = peerIrk[i];
    addDevice.localIRK[15-i] = localIrk[i];
  }
#ifdef _BLE_TRACE_
  Serial.print("ADDTYPE    :");
  btct.printBytes(&addDevice.peerAddressType,1);
  Serial.print("adddddd    :");
  btct.printBytes(addDevice.peerAddress,6);
  Serial.print("Peer IRK   :");
  btct.printBytes(addDevice.peerIRK,16);
  Serial.print("localIRK   :");
  btct.printBytes(addDevice.localIRK,16);
#endif
  sendCommand(OGF_LE_CTL << 10 | 0x27, sizeof(addDevice), &addDevice);

  leStartResolvingAddresses();
}
int HCIClass::leStopResolvingAddresses(){
    uint8_t enable = 0;
    return HCI.sendCommand(OGF_LE_CTL << 10 | 0x2D, 1,&enable); // Disable address resolution
}
int HCIClass::leStartResolvingAddresses(){
  uint8_t enable = 1;
  return HCI.sendCommand(OGF_LE_CTL << 10 | 0x2D, 1,&enable); // Disable address resolution
}
int HCIClass::leReadPeerResolvableAddress(uint8_t peerAddressType, uint8_t* peerIdentityAddress, uint8_t* peerResolvableAddress){
  (void)peerResolvableAddress;
  struct __attribute__ ((packed)) Request {
    uint8_t addressType;
    uint8_t identityAddress[6];
  } request;
  request.addressType = peerAddressType;
  for(int i=0; i<6; i++) request.identityAddress[5-i] = peerIdentityAddress[i];


  int res = sendCommand(OGF_LE_CTL << 10 | 0x2B, sizeof(request), &request);
#ifdef _BLE_TRACE_
  Serial.print("res: 0x");
  Serial.println(res, HEX);
#endif
  if(res==0){
    struct __attribute__ ((packed)) Response {
      uint8_t status;
      uint8_t peerResolvableAddress[6];
    } *response = (Response*)_cmdResponse;
#ifdef _BLE_TRACE_
    Serial.print("Address resolution status: 0x");
    Serial.println(response->status, HEX);
    Serial.print("peer resolvable address: ");
    btct.printBytes(response->peerResolvableAddress,6);
#endif
  }
  return res;
}

void HCIClass::writeLK(uint8_t peerAddress[], uint8_t LK[]){
  struct __attribute__ ((packed)) StoreLK {
    uint8_t nKeys;
    uint8_t BD_ADDR[6];
    uint8_t LTK[16];
  } storeLK;
  storeLK.nKeys = 1;
  memcpy(storeLK.BD_ADDR, peerAddress, 6);
  for(int i=0; i<16; i++) storeLK.LTK[15-i] = LK[i];
  HCI.sendCommand(OGF_HOST_CTL << 10 | 0x11, sizeof(storeLK), &storeLK);
}
void HCIClass::readStoredLKs(){
  uint8_t BD_ADDR[6];
  readStoredLK(BD_ADDR, 1);
}
int HCIClass::readStoredLK(uint8_t BD_ADDR[], uint8_t read_all ){
  struct __attribute__ ((packed)) Request {
    uint8_t BD_ADDR[6];
    uint8_t read_a;
  } request = {{0},0};
  for(int i=0; i<6; i++) request.BD_ADDR[5-i] = BD_ADDR[i];
  request.read_a = read_all;
  return sendCommand(OGF_HOST_CTL << 10 | 0xD, sizeof(request), &request);
}

int HCIClass::tryResolveAddress(uint8_t* BDAddr, uint8_t* address){
  bool foundMatch = false;
  if(HCI._getIRKs!=0){
    uint8_t nIRKs = 0;
    uint8_t** BDAddrType = new uint8_t*;
    uint8_t*** BADDRs = new uint8_t**;
    uint8_t*** IRKs = new uint8_t**;


    if(!HCI._getIRKs(&nIRKs, BDAddrType, BADDRs, IRKs)){
#ifdef _BLE_TRACE_
      Serial.println("error getting IRKs.");
#endif
    }
    for(int i=0; i<nIRKs; i++){
      if(!foundMatch){
#ifdef _BLE_TRACE_
        Serial.print("BDAddr type:      : 0x");
        Serial.println((*BDAddrType)[i],HEX);
        Serial.print("BDAddr            : ");
        btct.printBytes((*BADDRs)[i],6);
        Serial.print("IRK               : ");
        btct.printBytes((*IRKs)[i],16);
#endif
        uint8_t hashresult[3];
        btct.ah((*IRKs)[i], BDAddr, hashresult);
#ifdef _BLE_TRACE_
        Serial.print("hash match        : ");
        btct.printBytes(hashresult,3);
        Serial.print("                  : ");
        btct.printBytes(&BDAddr[3],3);
#endif

        for(int k=0; k<3; k++){
          if(hashresult[k] == BDAddr[3 + k]){
            foundMatch = true;
          }else{
            foundMatch = false;
            break;
          }
        }
        if(foundMatch){
          memcpy(address, (*BADDRs)[i],6);
        }
      }
      delete[] (*BADDRs)[i];
      delete[] (*IRKs)[i];
    }
    delete[] (*BDAddrType);
    delete BDAddrType;
    delete[] (*BADDRs);
    delete BADDRs;
    delete[] (*IRKs);
    delete IRKs;

    if(foundMatch){
      return 1;
    }
  }
  return 0;
}

int HCIClass::sendAclPkt(uint16_t handle, uint8_t cid, uint8_t plen, void* data)
{
  while (_pendingPkt >= _maxPkt) {
    poll();
  }

  struct __attribute__ ((packed)) HCIACLHdr {
    uint8_t pktType;
    uint16_t handle;
    uint16_t dlen;
    uint16_t plen;
    uint16_t cid;
  } aclHdr = { HCI_ACLDATA_PKT, handle, uint8_t(plen + 4), plen, cid };

  uint8_t txBuffer[sizeof(aclHdr) + plen];
  memcpy(txBuffer, &aclHdr, sizeof(aclHdr));
  memcpy(&txBuffer[sizeof(aclHdr)], data, plen);

  if (_debug) {
    dumpPkt("HCI ACLDATA TX -> ", sizeof(aclHdr) + plen, txBuffer);
  }
#ifdef _BLE_TRACE_
  Serial.print("Data tx -> ");
  for(int i=0; i< sizeof(aclHdr) + plen;i++){
    Serial.print(" 0x");
    Serial.print(txBuffer[i],HEX);
  }
  Serial.println(".");
#endif

  _pendingPkt++;
  HCITransport.write(txBuffer, sizeof(aclHdr) + plen);

  return 0;
}

int HCIClass::disconnect(uint16_t handle)
{
    struct __attribute__ ((packed)) HCIDisconnectData {
    uint16_t handle;
    uint8_t reason;
  } disconnectData = { handle, HCI_OE_USER_ENDED_CONNECTION };

  return sendCommand(OGF_LINK_CTL << 10 | OCF_DISCONNECT, sizeof(disconnectData), &disconnectData);
}

void HCIClass::debug(Stream& stream)
{
  _debug = &stream;
}

void HCIClass::noDebug()
{
  _debug = NULL;
}

int HCIClass::sendCommand(uint16_t opcode, uint8_t plen, void* parameters)
{
  struct __attribute__ ((packed)) {
    uint8_t pktType;
    uint16_t opcode;
    uint8_t plen;
  } pktHdr = {HCI_COMMAND_PKT, opcode, plen};

  uint8_t txBuffer[sizeof(pktHdr) + plen];
  memcpy(txBuffer, &pktHdr, sizeof(pktHdr));
  memcpy(&txBuffer[sizeof(pktHdr)], parameters, plen);

  if (_debug) {
    dumpPkt("HCI COMMAND TX -> ", sizeof(pktHdr) + plen, txBuffer);
  }
#ifdef _BLE_TRACE_
  Serial.print("Command tx -> ");
  for(int i=0; i< sizeof(pktHdr) + plen;i++){
    Serial.print(" 0x");
    Serial.print(txBuffer[i],HEX);
  }
  Serial.println("");
#endif
  HCITransport.write(txBuffer, sizeof(pktHdr) + plen);

  _cmdCompleteOpcode = 0xffff;
  _cmdCompleteStatus = -1;

  for (unsigned long start = millis(); _cmdCompleteOpcode != opcode && millis() < (start + 1000);) {
    poll();
  }

  return _cmdCompleteStatus;
}

void HCIClass::handleAclDataPkt(uint8_t /*plen*/, uint8_t pdata[])
{
  struct __attribute__ ((packed)) HCIACLHdr {
    uint16_t handle;
    uint16_t dlen;
    uint16_t len;
    uint16_t cid;
  } *aclHdr = (HCIACLHdr*)pdata;


  uint16_t aclFlags = (aclHdr->handle & 0xf000) >> 12;

  if ((aclHdr->dlen - 4) != aclHdr->len) {
    // packet is fragmented
    if (aclFlags != 0x01) {
      // copy into ACL buffer
      memcpy(_aclPktBuffer, &_recvBuffer[1], sizeof(HCIACLHdr) + aclHdr->dlen - 4);
    } else {
      // copy next chunk into the buffer
      HCIACLHdr* aclBufferHeader = (HCIACLHdr*)_aclPktBuffer;

      memcpy(&_aclPktBuffer[sizeof(HCIACLHdr) + aclBufferHeader->dlen - 4], &_recvBuffer[1 + sizeof(aclHdr->handle) + sizeof(aclHdr->dlen)], aclHdr->dlen);

      aclBufferHeader->dlen += aclHdr->dlen;
      aclHdr = aclBufferHeader;
    }
  }

  if ((aclHdr->dlen - 4) != aclHdr->len) {
#ifdef _BLE_TRACE_
    Serial.println("Don't have full packet yet");
    Serial.print("Handle: ");
    btct.printBytes((uint8_t*)&aclHdr->handle,2);
    Serial.print("dlen: ");
    btct.printBytes((uint8_t*)&aclHdr->dlen,2);
    Serial.print("len: ");
    btct.printBytes((uint8_t*)&aclHdr->len,2);
    Serial.print("cid: ");
    btct.printBytes((uint8_t*)&aclHdr->cid,2);
#endif
    // don't have the full packet yet
    return;
  }

  if (aclHdr->cid == ATT_CID) {
    if (aclFlags == 0x01) {
      // use buffered packet
      ATT.handleData(aclHdr->handle & 0x0fff, aclHdr->len, &_aclPktBuffer[sizeof(HCIACLHdr)]);
    } else {
      // use the recv buffer
      ATT.handleData(aclHdr->handle & 0x0fff, aclHdr->len, &_recvBuffer[1 + sizeof(HCIACLHdr)]);
    }
  } else if (aclHdr->cid == SIGNALING_CID) {
#ifdef _BLE_TRACE_
    Serial.println("Signaling");
#endif
    L2CAPSignaling.handleData(aclHdr->handle & 0x0fff, aclHdr->len, &_recvBuffer[1 + sizeof(HCIACLHdr)]);
  } else if (aclHdr->cid == SECURITY_CID){
    // Security manager
#ifdef _BLE_TRACE_
    Serial.println("Security data");
#endif
    if (aclFlags == 0x1){
      L2CAPSignaling.handleSecurityData(aclHdr->handle & 0x0fff, aclHdr->len, &_aclPktBuffer[sizeof(HCIACLHdr)]);
    }else{
      L2CAPSignaling.handleSecurityData(aclHdr->handle & 0x0fff, aclHdr->len, &_recvBuffer[1 + sizeof(HCIACLHdr)]);
    }

  }else {
    struct __attribute__ ((packed)) {
      uint8_t op;
      uint8_t id;
      uint16_t length;
      uint16_t reason;
      uint16_t localCid;
      uint16_t remoteCid;
    } l2capRejectCid= { 0x01, 0x00, 0x006, 0x0002, aclHdr->cid, 0x0000 };
#ifdef _BLE_TRACE_
    Serial.print("rejecting packet cid: 0x");
    Serial.println(aclHdr->cid,HEX);
#endif

    sendAclPkt(aclHdr->handle & 0x0fff, 0x0005, sizeof(l2capRejectCid), &l2capRejectCid);
  }
}

void HCIClass::handleNumCompPkts(uint16_t /*handle*/, uint16_t numPkts)
{
  if (numPkts && _pendingPkt > numPkts) {
    _pendingPkt -= numPkts;
  } else {
    _pendingPkt = 0;
  }
}

void HCIClass::handleEventPkt(uint8_t /*plen*/, uint8_t pdata[])
{
  struct __attribute__ ((packed)) HCIEventHdr {
    uint8_t evt;
    uint8_t plen;
  } *eventHdr = (HCIEventHdr*)pdata;
#ifdef _BLE_TRACE_
  Serial.print("HCI event: ");
  Serial.println(eventHdr->evt, HEX);
#endif

  if (eventHdr->evt == EVT_DISCONN_COMPLETE)
  {
    struct __attribute__ ((packed)) DisconnComplete {
      uint8_t status;
      uint16_t handle;
      uint8_t reason;
    } *disconnComplete = (DisconnComplete*)&pdata[sizeof(HCIEventHdr)];

    ATT.removeConnection(disconnComplete->handle, disconnComplete->reason);
    L2CAPSignaling.removeConnection(disconnComplete->handle, disconnComplete->reason);

    if (GAP.advertising())
    {
      HCI.leSetAdvertiseEnable(0x01);
    }
  }
  else if (eventHdr->evt == EVT_ENCRYPTION_CHANGE)
  {
    struct __attribute__ ((packed)) EncryptionChange {
      uint8_t status;
      uint16_t connectionHandle;
      uint8_t enabled;
    } *encryptionChange = (EncryptionChange*)&pdata[sizeof(HCIEventHdr)];
#ifdef _BLE_TRACE_
    Serial.println("[Info] Encryption changed");
    Serial.print("status : ");
    btct.printBytes(&encryptionChange->status,1);
    Serial.print("handle : ");
    btct.printBytes((uint8_t*)&encryptionChange->connectionHandle,2);
    Serial.print("enabled: ");
    btct.printBytes(&encryptionChange->enabled,1);
#endif
    if(encryptionChange->enabled>0){
      // 0001 1110
      if((ATT.getPeerEncryption(encryptionChange->connectionHandle)&PEER_ENCRYPTION::PAIRING_REQUEST)>0){
        if(ATT.localKeyDistribution.EncKey()){
#ifdef _BLE_TRACE_
          Serial.println("Enc key set but should be ignored");
#endif
        }else{
#ifdef _BLE_TRACE_
          Serial.println("No enc key distribution");
#endif
        }
        // From page 1681 bluetooth standard - order matters
        if(ATT.localKeyDistribution.IdKey()){
          /// We shall distribute IRK and address using identity information
          {
            uint8_t response[17];
            response[0] = CONNECTION_IDENTITY_INFORMATION; // Identity information.
            for(int i=0; i<16; i++) response[16-i] = ATT.localIRK[i];
            HCI.sendAclPkt(encryptionChange->connectionHandle, SECURITY_CID, sizeof(response), response);
#ifdef _BLE_TRACE_
            Serial.println("Distribute ID Key");
#endif
          }
          {
            uint8_t response[8];
            response[0] = CONNECTION_IDENTITY_ADDRESS; // Identity address information
            response[1] = 0x00; // Static local address
            for(int i=0; i<6; i++) response[7-i] = HCI.localAddr[i];
            HCI.sendAclPkt(encryptionChange->connectionHandle, SECURITY_CID, sizeof(response), response);
          }
        }
        if(ATT.localKeyDistribution.SignKey()){
          /// We shall distribut CSRK
#ifdef _BLE_TRACE_
          Serial.println("We shall distribute CSRK // not implemented");
#endif

        }else{
          // Serial.println("We don't want to distribute CSRK");
        }
        if(ATT.localKeyDistribution.LinkKey()){
#ifdef _BLE_TRACE_
          Serial.println("We would like to use LTK to generate BR/EDR // not implemented");
#endif
        }
      }else{
#ifdef _BLE_TRACE_
        Serial.println("Reconnection, not pairing so no keys");
        Serial.println(ATT.getPeerEncryption(encryptionChange->connectionHandle),HEX);
#endif
      }

      ATT.setPeerEncryption(encryptionChange->connectionHandle, PEER_ENCRYPTION::ENCRYPTED_AES);
      if(ATT.writeBufferSize > 0){
        ATT.processWriteBuffer();
      }
      if(ATT.holdBufferSize>0){
#ifdef _BLE_TRACE_
        Serial.print("Sending queued response size: ");
        Serial.println(ATT.holdBufferSize);
#endif
        HCI.sendAclPkt(encryptionChange->connectionHandle, ATT_CID, ATT.holdBufferSize, ATT.holdBuffer);
        ATT.holdBufferSize = 0;
      }
    }else{
      ATT.setPeerEncryption(encryptionChange->connectionHandle, PEER_ENCRYPTION::NO_ENCRYPTION);
    }
  }
  else if (eventHdr->evt == EVT_CMD_COMPLETE)
  {
    struct __attribute__ ((packed)) CmdComplete {
      uint8_t ncmd;
      uint16_t opcode;
      uint8_t status;
    } *cmdCompleteHeader = (CmdComplete*)&pdata[sizeof(HCIEventHdr)];
#ifdef _BLE_TRACE_
    Serial.print("E ncmd:   0x");
    Serial.println(cmdCompleteHeader->ncmd,HEX);
    Serial.print("E opcode: 0x");
    Serial.println(cmdCompleteHeader->opcode, HEX);
    Serial.print("E status: 0x");
    Serial.println(cmdCompleteHeader->status, HEX);
#endif
    _cmdCompleteOpcode = cmdCompleteHeader->opcode;
    _cmdCompleteStatus = cmdCompleteHeader->status;
    _cmdResponseLen = pdata[1] - sizeof(CmdComplete);
    _cmdResponse = &pdata[sizeof(HCIEventHdr) + sizeof(CmdComplete)];

  }
  else if (eventHdr->evt == EVT_CMD_STATUS)
  {
    struct __attribute__ ((packed)) CmdStatus {
      uint8_t status;
      uint8_t ncmd;
      uint16_t opcode;
    } *cmdStatusHeader = (CmdStatus*)&pdata[sizeof(HCIEventHdr)];

#ifdef _BLE_TRACE_
    Serial.print("F n cmd:  0x");
    Serial.println(cmdStatusHeader->ncmd, HEX);
    Serial.print("F status: 0x");
    Serial.println(cmdStatusHeader->status, HEX);
    Serial.print("F opcode: 0x");
    Serial.println(cmdStatusHeader->opcode, HEX);
#endif
    _cmdCompleteOpcode = cmdStatusHeader->opcode;
    _cmdCompleteStatus = cmdStatusHeader->status;
    _cmdResponseLen = 0;
  }
  else if (eventHdr->evt == EVT_NUM_COMP_PKTS)
  {
    uint8_t numHandles = pdata[sizeof(HCIEventHdr)];
    uint16_t* data = (uint16_t*)&pdata[sizeof(HCIEventHdr) + sizeof(numHandles)];

    for (uint8_t i = 0; i < numHandles; i++) {
      handleNumCompPkts(data[0], data[1]);
#ifdef _BLE_TRACE_
      Serial.print("Outstanding packets: ");
      Serial.println(_pendingPkt);
      Serial.print("Data[0]: 0x");
      Serial.println(data[0]);
      Serial.print("Data[1]: 0x");
      Serial.println(data[1]);
#endif
      data += 2;
    }
  }
  else if(eventHdr->evt == 0x10)
  {
#ifdef _BLE_TRACE_
    struct __attribute__ ((packed)) CmdHardwareError {
      uint8_t hardwareCode;
    } *cmdHardwareError = (CmdHardwareError*)&pdata[sizeof(HCIEventHdr)];
    Serial.print("Bluetooth hardware error.");
    Serial.print(" Code: 0x");
    Serial.println(cmdHardwareError->hardwareCode, HEX);
#endif
  }
  else if (eventHdr->evt == EVT_LE_META_EVENT)
  {
    struct __attribute__ ((packed)) LeMetaEventHeader {
      uint8_t subevent;
    } *leMetaHeader = (LeMetaEventHeader*)&pdata[sizeof(HCIEventHdr)];
#ifdef _BLE_TRACE_
    Serial.print("\tSubEvent: 0x");
    Serial.println(leMetaHeader->subevent,HEX);
#endif
    switch((LE_META_EVENT)leMetaHeader->subevent){
      case ENHANCED_CONN_COMPLETE:{
        struct __attribute__ ((packed)) EvtLeConnectionComplete {
          uint8_t status;
          uint16_t handle;
          uint8_t role;
          uint8_t peerBdaddrType;
          uint8_t peerBdaddr[6];
          uint8_t localResolvablePrivateAddress[6];
          uint8_t peerResolvablePrivateAddress[6];
          uint16_t interval;
          uint16_t latency;
          uint16_t supervisionTimeout;
          uint8_t masterClockAccuracy;
        } *leConnectionComplete = (EvtLeConnectionComplete*)&pdata[sizeof(HCIEventHdr) + sizeof(LeMetaEventHeader)];

        if (leConnectionComplete->status == 0x00) {
          ATT.addConnection(leConnectionComplete->handle,
                            leConnectionComplete->role,
                            leConnectionComplete->peerBdaddrType,
                            leConnectionComplete->peerBdaddr,
                            leConnectionComplete->interval,
                            leConnectionComplete->latency,
                            leConnectionComplete->supervisionTimeout,
                            leConnectionComplete->masterClockAccuracy);

          L2CAPSignaling.addConnection(leConnectionComplete->handle,
                                leConnectionComplete->role,
                                leConnectionComplete->peerBdaddrType,
                                leConnectionComplete->peerBdaddr,
                                leConnectionComplete->interval,
                                leConnectionComplete->latency,
                                leConnectionComplete->supervisionTimeout,
                                leConnectionComplete->masterClockAccuracy);
        }
        // uint8_t address[6];
        // uint8_t BDAddr[6];
        // for(int i=0; i<6; i++) BDAddr[5-i] = leConnectionComplete->peerBdaddr[i];
        // leReadPeerResolvableAddress(leConnectionComplete->peerBdaddrType,BDAddr,address);
        // Serial.print("Resolving address: ");
        // btct.printBytes(BDAddr, 6);
        // Serial.print("BT answer         : ");
        // btct.printBytes(address, 6);

#ifdef _BLE_TRACE_
        Serial.print("Resolved peer     : ");
        btct.printBytes(leConnectionComplete->peerResolvablePrivateAddress,6);
        Serial.print("Resolved local    : ");
        btct.printBytes(leConnectionComplete->localResolvablePrivateAddress,6);
#endif
        break;
      }
      case CONN_COMPLETE:{
        struct __attribute__ ((packed)) EvtLeConnectionComplete {
          uint8_t status;
          uint16_t handle;
          uint8_t role;
          uint8_t peerBdaddrType;
          uint8_t peerBdaddr[6];
          uint16_t interval;
          uint16_t latency;
          uint16_t supervisionTimeout;
          uint8_t masterClockAccuracy;
        } *leConnectionComplete = (EvtLeConnectionComplete*)&pdata[sizeof(HCIEventHdr) + sizeof(LeMetaEventHeader)];

        if (leConnectionComplete->status == 0x00) {
          ATT.addConnection(leConnectionComplete->handle,
                            leConnectionComplete->role,
                            leConnectionComplete->peerBdaddrType,
                            leConnectionComplete->peerBdaddr,
                            leConnectionComplete->interval,
                            leConnectionComplete->latency,
                            leConnectionComplete->supervisionTimeout,
                            leConnectionComplete->masterClockAccuracy);

          L2CAPSignaling.addConnection(leConnectionComplete->handle,
                                leConnectionComplete->role,
                                leConnectionComplete->peerBdaddrType,
                                leConnectionComplete->peerBdaddr,
                                leConnectionComplete->interval,
                                leConnectionComplete->latency,
                                leConnectionComplete->supervisionTimeout,
                                leConnectionComplete->masterClockAccuracy);
        }
        // leReadPeerResolvableAddress(leConnectionComplete->peerBdaddrType,BDAddr,address);
        // Serial.print("Resolving address: ");
        // btct.printBytes(BDAddr, 6);
        // Serial.print("BT answer         : ");
        // btct.printBytes(address, 6);
        break;
      }
      case ADVERTISING_REPORT:{
        struct __attribute__ ((packed)) EvtLeAdvertisingReport {
          uint8_t status;
          uint8_t type;
          uint8_t peerBdaddrType;
          uint8_t peerBdaddr[6];
          uint8_t eirLength;
          uint8_t eirData[31];
        } *leAdvertisingReport = (EvtLeAdvertisingReport*)&pdata[sizeof(HCIEventHdr) + sizeof(LeMetaEventHeader)];

        if(leAdvertisingReport->eirLength > sizeof(leAdvertisingReport->eirData)){
          return ;
        }

        if (leAdvertisingReport->status == 0x01) {
          // last byte is RSSI
          int8_t rssi = leAdvertisingReport->eirData[leAdvertisingReport->eirLength];

          GAP.handleLeAdvertisingReport(leAdvertisingReport->type,
                                        leAdvertisingReport->peerBdaddrType,
                                        leAdvertisingReport->peerBdaddr,
                                        leAdvertisingReport->eirLength,
                                        leAdvertisingReport->eirData,
                                        rssi);
        }
        break;
      }
      case LONG_TERM_KEY_REQUEST:{
        struct __attribute__ ((packed)) LTKRequest
        {
          uint8_t subEventCode;
          uint16_t connectionHandle;
          uint8_t randomNumber[8];
          uint8_t encryptedDiversifier[2];
        } *ltkRequest = (LTKRequest*)&pdata[sizeof(HCIEventHdr)];
#ifdef _BLE_TRACE_
        Serial.println("LTK request received");
        Serial.print("Connection Handle: ");
        btct.printBytes((uint8_t*)&ltkRequest->connectionHandle,2);
        Serial.print("Random Number    : ");
        btct.printBytes(ltkRequest->randomNumber,8);
        Serial.print("EDIV           : ");
        btct.printBytes(ltkRequest->encryptedDiversifier,2);
#endif
        // Load our LTK for this connection.
        uint8_t peerAddr[7];
        uint8_t resolvableAddr[6];
        uint8_t foundLTK;
        ATT.getPeerAddrWithType(ltkRequest->connectionHandle, peerAddr);

        if((ATT.getPeerEncryption(ltkRequest->connectionHandle) & PEER_ENCRYPTION::PAIRING_REQUEST)>0){
          // Pairing request - LTK is one in buffer already
          foundLTK = 1;
        }else{
          if(ATT.getPeerResolvedAddress(ltkRequest->connectionHandle, resolvableAddr)){
            foundLTK = getLTK(resolvableAddr, HCI.LTK);
          }else{
            foundLTK = getLTK(&peerAddr[1], HCI.LTK);
          }
        }
        // } //2d
        // Send our LTK back
        if(foundLTK){
          struct __attribute__ ((packed)) LTKReply
          {
            uint16_t connectionHandle;
            uint8_t LTK[16];
          } ltkReply = {0,0};
          ltkReply.connectionHandle = ltkRequest->connectionHandle;
          for(int i=0; i<16; i++) ltkReply.LTK[15-i] = HCI.LTK[i];
          int result = sendCommand(OGF_LE_CTL << 10 | LE_COMMAND::LONG_TERM_KEY_REPLY,sizeof(ltkReply), &ltkReply);

  #ifdef _BLE_TRACE_
          Serial.println("Sending LTK as: ");
          btct.printBytes(ltkReply.LTK,16);
  #endif

          if(result == 0){
            struct __attribute__ ((packed)) LTKReplyResult
            {
              uint8_t status;
              uint16_t connectionHandle;
            } ltkReplyResult = {0,0};
            memcpy(&ltkReplyResult, _cmdResponse, 3);

  #ifdef _BLE_TRACE_
            Serial.println("LTK send success");
            Serial.print("status     : ");
            btct.printBytes(&ltkReplyResult.status,1);
            Serial.print("Conn Handle: ");
            btct.printBytes((uint8_t*)&ltkReplyResult.connectionHandle,2);
  #endif
          }else{
  #ifdef _BLE_TRACE_
            Serial.print("Failed to send LTK...: ");
            btct.printBytes((uint8_t*)&result,2);
  #endif
          }
        }else{
          /// do LTK rejection
#ifdef _BLE_TRACE_
          Serial.println("LTK not found, rejecting");
#endif
          sendCommand(OGF_LE_CTL << 10 | LE_COMMAND::LONG_TERM_KEY_NEGATIVE_REPLY,2, &ltkRequest->connectionHandle);
        }
        break;
      }
      case REMOTE_CONN_PARAM_REQ:{
        struct __attribute__ ((packed)) RemoteConnParamReq {
          uint8_t subEventCode;
          uint16_t connectionHandle;
          uint16_t intervalMin;
          uint16_t intervalMax;
          uint16_t latency;
          uint16_t timeOut;
        } *remoteConnParamReq = (RemoteConnParamReq*)&pdata[sizeof(HCIEventHdr)];
#ifdef _BLE_TRACE_
        Serial.println("--- Remtoe conn param req");
        Serial.print("Handle      : ");
        btct.printBytes((uint8_t*)&remoteConnParamReq->connectionHandle,2);
        Serial.print("Interval min: ");
        btct.printBytes((uint8_t*)&remoteConnParamReq->intervalMin,2);
        Serial.print("Interval max: ");
        btct.printBytes((uint8_t*)&remoteConnParamReq->intervalMax,2);
        Serial.print("Latency     : ");
        btct.printBytes((uint8_t*)&remoteConnParamReq->latency,2);
        Serial.print("Timeout     : ");
        btct.printBytes((uint8_t*)&remoteConnParamReq->timeOut,2);
#endif

        struct __attribute__ ((packed)) RemoteConnParamReqReply {
          uint16_t connectionHandle;
          uint16_t intervalMin;
          uint16_t intervalMax;
          uint16_t latency;
          uint16_t timeOut;
          uint16_t minLength;
          uint16_t maxLength;
        } remoteConnParamReqReply;
        memcpy(&remoteConnParamReqReply, &remoteConnParamReq->connectionHandle, sizeof(RemoteConnParamReq)-1);

        remoteConnParamReqReply.minLength = 0x000F;
        remoteConnParamReqReply.maxLength = 0x0FFF;
        sendCommand(OGF_LE_CTL << 10 | 0x20, sizeof(RemoteConnParamReqReply), &remoteConnParamReqReply);
        break;
      }
      case READ_LOCAL_P256_COMPLETE:{
        struct __attribute__ ((packed)) EvtReadLocalP256Complete{
          uint8_t subEventCode;
          uint8_t status;
          uint8_t localPublicKey[64];
        } *evtReadLocalP256Complete = (EvtReadLocalP256Complete*)&pdata[sizeof(HCIEventHdr)];
        if(evtReadLocalP256Complete->status == 0x0){
#ifdef _BLE_TRACE_
          Serial.println("Key read success");
#endif
          struct __attribute__ ((packed)) PairingPublicKey
          {
            uint8_t code;
            uint8_t publicKey[64];
          } pairingPublicKey = {CONNECTION_PAIRING_PUBLIC_KEY,0};
          memcpy(pairingPublicKey.publicKey,evtReadLocalP256Complete->localPublicKey,64);
          memcpy(localPublicKeyBuffer,      evtReadLocalP256Complete->localPublicKey,64);

          // Send the local public key to the remote
          uint16_t connectionHandle = ATT.getPeerEncrptingConnectionHandle();
          if(connectionHandle>ATT_MAX_PEERS){
#ifdef _BLE_TRACE_
            Serial.println("failed to find connection handle");
#endif
            break;
          }
          HCI.sendAclPkt(connectionHandle,SECURITY_CID,sizeof(PairingPublicKey),&pairingPublicKey);
          uint8_t encryption = ATT.getPeerEncryption(connectionHandle) | PEER_ENCRYPTION::SENT_PUBKEY;
          ATT.setPeerEncryption(connectionHandle, encryption);


          uint8_t Z = 0;

          HCI.leRand(Nb);
          HCI.leRand(&Nb[8]);

#ifdef _BLE_TRACE_
          Serial.print("nb: ");
          btct.printBytes(Nb, 16);
#endif
          struct __attribute__ ((packed)) F4Params
          {
            uint8_t U[32];
            uint8_t V[32];
            uint8_t Z;
          } f4Params = {{0},{0},Z};
          for(int i=0; i<32; i++){
            f4Params.U[31-i] = pairingPublicKey.publicKey[i];
            f4Params.V[31-i] = HCI.remotePublicKeyBuffer[i];
          }

          struct __attribute__ ((packed)) PairingConfirm
          {
            uint8_t code;
            uint8_t cb[16];
          } pairingConfirm = {CONNECTION_PAIRING_CONFIRM,0};

          btct.AES_CMAC(Nb,(unsigned char *)&f4Params,sizeof(f4Params),pairingConfirm.cb);

#ifdef _BLE_TRACE_
          Serial.print("cb: ");
          btct.printBytes(pairingConfirm.cb, 16);
#endif

          uint8_t cb_temp[sizeof(pairingConfirm.cb)];
          for(unsigned int i=0; i<sizeof(pairingConfirm.cb);i++){
            cb_temp[sizeof(pairingConfirm.cb)-1-i] = pairingConfirm.cb[i];
          }
          /// cb wa back to front.
          memcpy(pairingConfirm.cb,cb_temp,sizeof(pairingConfirm.cb));

          // Send Pairing confirm response
          HCI.sendAclPkt(connectionHandle, SECURITY_CID, sizeof(pairingConfirm), &pairingConfirm);

          HCI.sendCommand( (OGF_LE_CTL << 10) | LE_COMMAND::GENERATE_DH_KEY_V1, sizeof(HCI.remotePublicKeyBuffer), HCI.remotePublicKeyBuffer);
        }else{
#ifdef _BLE_TRACE_
          Serial.print("Key read error: 0x");
          Serial.println(evtReadLocalP256Complete->status,HEX);
          for(int i=0; i<64; i++){
            Serial.print(" 0x");
            Serial.print(evtReadLocalP256Complete->localPublicKey[i],HEX);
          }
          Serial.println(".");
#endif
        }
        break;
      }
      case GENERATE_DH_KEY_COMPLETE:{
        struct __attribute__ ((packed)) EvtLeDHKeyComplete{
          uint8_t subEventCode;
          uint8_t status;
          uint8_t DHKey[32];
        } *evtLeDHKeyComplete = (EvtLeDHKeyComplete*)&pdata[sizeof(HCIEventHdr)];
        if(evtLeDHKeyComplete->status == 0x0){
#ifdef _BLE_TRACE_
          Serial.println("DH key generated");
#endif
          uint16_t connectionHandle = ATT.getPeerEncrptingConnectionHandle();
          if(connectionHandle>ATT_MAX_PEERS){
#ifdef _BLE_TRACE_
            Serial.println("Failed to find connection handle DH key check");
#endif
            break;
          }


          for(int i=0; i<32; i++) DHKey[31-i] = evtLeDHKeyComplete->DHKey[i];

#ifdef _BLE_TRACE_
          Serial.println("Stored our DHKey:");
          btct.printBytes(DHKey,32);
#endif
          uint8_t encryption = ATT.getPeerEncryption(connectionHandle) | PEER_ENCRYPTION::DH_KEY_CALULATED;
          ATT.setPeerEncryption(connectionHandle, encryption);

          if((encryption & PEER_ENCRYPTION::RECEIVED_DH_CHECK) > 0){
#ifdef _BLE_TRACE_
            Serial.println("Received DHKey check already so calculate f5, f6 now.");
#endif
            L2CAPSignaling.smCalculateLTKandConfirm(connectionHandle, HCI.remoteDHKeyCheckBuffer);

          }else{
#ifdef _BLE_TRACE_
            Serial.println("Waiting on other DHKey check before calculating.");
#endif
          }
        }else{
#ifdef _BLE_TRACE_
          Serial.print("Key generation error: 0x");
          Serial.println(evtLeDHKeyComplete->status, HEX);
#endif
        }
        break;
      }
      default:
      {
#ifdef _BLE_TRACE_
        Serial.println("[Info] Unhandled meta event");
#endif
      }
    }
  }else{
#ifdef _BLE_TRACE_
    Serial.println("[Info] Unhandled event");
#endif
  }
}
int HCIClass::leEncrypt(uint8_t* key, uint8_t* plaintext, uint8_t* status, uint8_t* ciphertext){
  (void)status;
  struct __attribute__ ((packed)) LeEncryptCommand
  {
    uint8_t key[16];
    uint8_t plaintext[16];
  } leEncryptCommand = {{0},{0}};
  for(int i=0; i<16; i++){
    leEncryptCommand.key[15-i] = key[i];
    leEncryptCommand.plaintext[15-i] = plaintext[i];
  }

  int res = sendCommand(OGF_LE_CTL << 10 | LE_COMMAND::ENCRYPT, 32, &leEncryptCommand);
  if(res == 0){
#ifdef _BLE_TRACE_
    Serial.print("Copying from command Response length: ");
    Serial.println(_cmdResponseLen);
    Serial.println(".");
    for(int i=0; i<20; i++){
        Serial.print(" 0x");
        Serial.print(_cmdResponse[i],HEX);
    }
    Serial.println(".");
#endif
    for(int i=0; i<16; i++){
      ciphertext[15-i] = _cmdResponse[i];
    }
    return 1;
  }
#ifdef _BLE_TRACE_
  Serial.print("Error with AES: 0x");
  Serial.println(res, HEX);
#endif
  return res;
}
int HCIClass::leRand(uint8_t rand[]){
  int res = sendCommand(OGF_LE_CTL << 10 | LE_COMMAND::RANDOM);
  if(res == 0){
    memcpy(rand,_cmdResponse, 8); /// backwards but it's a random number
  }
  return res;
}
int HCIClass::getLTK(uint8_t* address, uint8_t* LTK){
  if(_getLTK!=0){
    return _getLTK(address, LTK);
  }else{
    return 0;
  }
}
int HCIClass::storeIRK(uint8_t* address, uint8_t* IRK){
  if(_storeIRK!=0){
    return _storeIRK(address, IRK);
  }else{
    return 0;
  }
}
int HCIClass::storeLTK(uint8_t* address, uint8_t* LTK){
  if(_storeLTK!=0){
    return _storeLTK(address, LTK);
  }else{
    return 0;
  }
}
uint8_t HCIClass::localIOCap(){
  if(_displayCode!=0){
    /// We have a display
    if(_binaryConfirmPairing!=0){
      return IOCAP_DISPLAY_YES_NO;
    }else{
      return IOCAP_DISPLAY_ONLY;
    }
  }else{
    // We have no display
    return IOCAP_NO_INPUT_NO_OUTPUT;
  }
}

/// Stub function to generate parameters for local authreq
AuthReq HCIClass::localAuthreq(){
  // If get, set, IRK, LTK all set then we can bond.
  AuthReq local = AuthReq();
  if(_storeIRK!=0 && _storeLTK!=0 && _getLTK!=0 && _getIRKs!=0){
    local.setBonding(true);
  }
  local.setSC(true);
  local.setMITM(true);
  local.setCT2(true);
  return LOCAL_AUTHREQ;
}

void HCIClass::dumpPkt(const char* prefix, uint8_t plen, uint8_t pdata[])
{
  if (_debug) {
    _debug->print(prefix);

    for (uint8_t i = 0; i < plen; i++) {
      byte b = pdata[i];

      if (b < 16) {
        _debug->print("0");
      }

      _debug->print(b, HEX);
    }

    _debug->println();
    _debug->flush();
  }
}

#if !defined(FAKE_HCI)
HCIClass HCIObj;
HCIClass& HCI = HCIObj;
#endif
