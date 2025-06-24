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

#ifndef _HCI_H_
#define _HCI_H_

#include <Arduino.h>
#include "bitDescriptions.h"

#include "L2CAPSignaling.h"

#define OGF_LINK_CTL           0x01
#define OGF_HOST_CTL           0x03
#define OGF_INFO_PARAM         0x04
#define OGF_STATUS_PARAM       0x05
#define OGF_LE_CTL             0x08

enum LE_COMMAND {
  ENCRYPT                      = 0x0017,
  RANDOM                       = 0x0018,
  LONG_TERM_KEY_REPLY          = 0x001A,
  LONG_TERM_KEY_NEGATIVE_REPLY = 0x001B,
  READ_LOCAL_P256              = 0x0025,
  GENERATE_DH_KEY_V1           = 0x0026,
  GENERATE_DH_KEY_V2           = 0x005E
};
enum LE_META_EVENT {
  CONN_COMPLETE             = 0x01,
  ADVERTISING_REPORT        = 0x02,
  LONG_TERM_KEY_REQUEST     = 0x05,
  REMOTE_CONN_PARAM_REQ     = 0x06,
  READ_LOCAL_P256_COMPLETE  = 0x08,
  GENERATE_DH_KEY_COMPLETE  = 0x09,
  ENHANCED_CONN_COMPLETE    = 0x0A,
};
String metaEventToString(LE_META_EVENT event);
String commandToString(LE_COMMAND command);

class HCIClass {
public:
  HCIClass();
  virtual ~HCIClass();

  virtual int begin();
  virtual void end();

  virtual void poll();
  virtual void poll(unsigned long timeout);

  virtual int reset();
  virtual int readLocalVersion(uint8_t& hciVer, uint16_t& hciRev, uint8_t& lmpVer,
                       uint16_t& manufacturer, uint16_t& lmpSubVer);

  virtual int readBdAddr(uint8_t addr[6]);
  virtual int readBdAddr();

  virtual int readRssi(uint16_t handle);

  virtual int setEventMask(uint64_t eventMask);
  virtual int setLeEventMask(uint64_t leEventMask);
  virtual int readLeBufferSize(uint16_t& pktLen, uint8_t& maxPkt);
  virtual int leSetRandomAddress(uint8_t addr[6]);
  virtual int leSetAdvertisingParameters(uint16_t minInterval, uint16_t maxInterval,
                                 uint8_t advType, uint8_t ownBdaddrType,
                                 uint8_t directBdaddrType, uint8_t directBdaddr[6],
                                 uint8_t chanMap,
                                 uint8_t filter);
  virtual int leSetAdvertisingData(uint8_t length, uint8_t data[]);
  virtual int leSetScanResponseData(uint8_t length, uint8_t data[]);
  virtual int leSetAdvertiseEnable(uint8_t enable);
  virtual int leSetScanParameters(uint8_t type, uint16_t interval, uint16_t window, 
                          uint8_t ownBdaddrType, uint8_t filter);
  virtual int leSetScanEnable(uint8_t enabled, uint8_t duplicates);
  virtual int leCreateConn(uint16_t interval, uint16_t window, uint8_t initiatorFilter,
                  uint8_t peerBdaddrType, uint8_t peerBdaddr[6], uint8_t ownBdaddrType,
                  uint16_t minInterval, uint16_t maxInterval, uint16_t latency,
                  uint16_t supervisionTimeout, uint16_t minCeLength, uint16_t maxCeLength);
  virtual int leConnUpdate(uint16_t handle, uint16_t minInterval, uint16_t maxInterval, 
                  uint16_t latency, uint16_t supervisionTimeout);
  virtual int leCancelConn();
  virtual int leEncrypt(uint8_t* Key, uint8_t* plaintext, uint8_t* status, uint8_t* ciphertext);
  // Generate a 64 bit random number
  virtual int leRand(uint8_t rand[]);
  virtual AuthReq localAuthreq();
  virtual uint8_t localIOCap();

  virtual void saveNewAddress(uint8_t addressType, uint8_t* address, uint8_t* peerIrk, uint8_t* remoteIrk);
  virtual void leAddResolvingAddress(uint8_t addressType, uint8_t* address, uint8_t* peerIrk, uint8_t* remoteIrk);
  virtual int leStopResolvingAddresses();
  virtual int leStartResolvingAddresses();
  virtual int leReadPeerResolvableAddress(uint8_t peerAddressType, uint8_t* peerIdentityAddress, uint8_t* peerResolvableAddress);

  virtual void readStoredLKs();
  virtual int readStoredLK(uint8_t BD_ADDR[], uint8_t read_all = 0);
  virtual void writeLK(uint8_t peerAddress[], uint8_t LK[]);
  virtual int tryResolveAddress(uint8_t* BDAddr, uint8_t* address);

  virtual int sendAclPkt(uint16_t handle, uint8_t cid, uint8_t plen, void* data);

  virtual int disconnect(uint16_t handle);

  virtual void debug(Stream& stream);
  virtual void noDebug();

  // TODO: Send command be private again & use ATT implementation of send command within ATT.
  virtual int sendCommand(uint16_t opcode, uint8_t plen = 0, void* parameters = NULL);
  uint8_t remotePublicKeyBuffer[64];
  uint8_t localPublicKeyBuffer[64];
  uint8_t remoteDHKeyCheckBuffer[16];
  uint8_t Na[16];
  uint8_t Nb[16];
  uint8_t DHKey[32];
  uint8_t localAddr[6];
  uint8_t LTK[16];
  virtual int getLTK(uint8_t* address, uint8_t* LTK);
  virtual int storeLTK(uint8_t* address, uint8_t* LTK);
  virtual int storeIRK(uint8_t* address, uint8_t* IRK);
  int (*_storeIRK)(uint8_t* address, uint8_t* peerIrk) = 0;
  int (*_getIRKs)(uint8_t* nIRKs,uint8_t** BADDR_type, uint8_t*** BADDRs, uint8_t*** IRKs) = 0;
  int (*_storeLTK)(uint8_t*, uint8_t*) = 0;
  int (*_getLTK)(uint8_t*, uint8_t*) = 0;
  void (*_displayCode)(uint32_t confirmationCode) = 0;
  bool (*_binaryConfirmPairing)() = 0;

private:

  virtual void handleAclDataPkt(uint8_t plen, uint8_t pdata[]);
  virtual void handleNumCompPkts(uint16_t handle, uint16_t numPkts);
  virtual void handleEventPkt(uint8_t plen, uint8_t pdata[]);

  virtual void dumpPkt(const char* prefix, uint8_t plen, uint8_t pdata[]);

  Stream* _debug;

  int _recvIndex;
  uint8_t _recvBuffer[3 + 255];

  uint16_t _cmdCompleteOpcode;
  int _cmdCompleteStatus;
  uint8_t _cmdResponseLen;
  uint8_t* _cmdResponse;

  uint8_t _maxPkt;
  uint8_t _pendingPkt;

  uint8_t _aclPktBuffer[255];
};

extern HCIClass& HCI;

#endif
