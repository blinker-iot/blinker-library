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

#ifndef _ATT_H_
#define _ATT_H_

#include <Arduino.h>

#include "../BLEDevice.h"
#include "keyDistribution.h"

#define ATT_CID       0x0004
#define BLE_CTL       0x0008

#if DM_CONN_MAX
#define ATT_MAX_PEERS DM_CONN_MAX // Mbed + Cordio
#elif __AVR__
#define ATT_MAX_PEERS 3
#else
#define ATT_MAX_PEERS 8
#endif

enum PEER_ENCRYPTION {
  NO_ENCRYPTION         = 0,
  PAIRING_REQUEST       = 1 << 0,
  REQUESTED_ENCRYPTION  = 1 << 1,
  SENT_PUBKEY           = 1 << 2,
  DH_KEY_CALULATED      = 1 << 3,
  RECEIVED_DH_CHECK     = 1 << 4,
  SENT_DH_CHECK         = 1 << 5,
  ENCRYPTED_AES         = 1 << 7
};

class BLERemoteDevice;

class ATTClass {
public:
  ATTClass();
  virtual ~ATTClass();

  virtual void setMaxMtu(uint16_t maxMtu);
  virtual void setTimeout(unsigned long timeout);

  virtual bool connect(uint8_t peerBdaddrType, uint8_t peerBdaddr[6]);
  virtual bool disconnect(uint8_t peerBdaddrType, uint8_t peerBdaddr[6]);
  virtual bool discoverAttributes(uint8_t peerBdaddrType, uint8_t peerBdaddr[6], const char* serviceUuidFilter);

  virtual void addConnection(uint16_t handle, uint8_t role, uint8_t peerBdaddrType,
                    uint8_t peerBdaddr[6], uint16_t interval,
                    uint16_t latency, uint16_t supervisionTimeout,
                    uint8_t masterClockAccuracy);

  virtual void handleData(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);

  virtual void removeConnection(uint16_t handle, uint8_t reason);

  virtual uint16_t connectionHandle(uint8_t addressType, const uint8_t address[6]) const;
  virtual BLERemoteDevice* device(uint8_t addressType, const uint8_t address[6]) const;
  virtual bool connected() const;
  virtual bool connected(uint8_t addressType, const uint8_t address[6]) const;
  virtual bool connected(uint16_t handle) const;
  virtual bool paired() const;
  virtual bool paired(uint16_t handle) const;
  virtual uint16_t mtu(uint16_t handle) const;

  virtual bool disconnect();

  virtual BLEDevice central();

  virtual int handleNotify(uint16_t handle, const uint8_t* value, int length);
  virtual int handleInd(uint16_t handle, const uint8_t* value, int length);

  virtual void setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler);

  virtual int readReq(uint16_t connectionHandle, uint16_t handle, uint8_t responseBuffer[]);
  virtual int writeReq(uint16_t connectionHandle, uint16_t handle, const uint8_t* data, uint8_t dataLen, uint8_t responseBuffer[]);
  virtual void writeCmd(uint16_t connectionHandle, uint16_t handle, const uint8_t* data, uint8_t dataLen);
  virtual int setPeerEncryption(uint16_t connectionHandle, uint8_t encryption);
  uint8_t getPeerEncryption(uint16_t connectionHandle);
  uint16_t getPeerEncrptingConnectionHandle();
  virtual int getPeerAddr(uint16_t connectionHandle, uint8_t peerAddr[]);
  virtual int getPeerAddrWithType(uint16_t connectionHandle, uint8_t peerAddr[]);
  virtual int setPeerIOCap(uint16_t connectionHandle, uint8_t IOCap[]);
  virtual int getPeerIOCap(uint16_t connectionHandle, uint8_t IOCap[]);
  virtual int getPeerResolvedAddress(uint16_t connectionHandle, uint8_t* resolvedAddress);
  uint8_t holdBuffer[64];
  uint8_t writeBuffer[64];
  uint8_t holdBufferSize;
  uint8_t writeBufferSize;
  virtual int processWriteBuffer();
  KeyDistribution remoteKeyDistribution;
  KeyDistribution localKeyDistribution;
  uint8_t peerIRK[16];
  /// This is just a random number... Not sure it has use unless privacy mode is active.
  uint8_t localIRK[16] = {0x54,0x83,0x63,0x7c,0xc5,0x1e,0xf7,0xec,0x32,0xdd,0xad,0x51,0x89,0x4b,0x9e,0x07};
private:
  virtual void error(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void mtuReq(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual int mtuReq(uint16_t connectionHandle, uint16_t mtu, uint8_t responseBuffer[]);
  virtual void mtuResp(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void findInfoReq(uint16_t connectionHandle, uint16_t mtu, uint8_t dlen, uint8_t data[]);
  virtual int findInfoReq(uint16_t connectionHandle, uint16_t startHandle, uint16_t endHandle, uint8_t responseBuffer[]);
  virtual void findInfoResp(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void findByTypeReq(uint16_t connectionHandle, uint16_t mtu, uint8_t dlen, uint8_t data[]);
  virtual void readByTypeReq(uint16_t connectionHandle, uint16_t mtu, uint8_t dlen, uint8_t data[]);
  virtual int readByTypeReq(uint16_t connectionHandle, uint16_t startHandle, uint16_t endHandle, uint16_t type, uint8_t responseBuffer[]);
  virtual void readByTypeResp(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void readOrReadBlobReq(uint16_t connectionHandle, uint16_t mtu, uint8_t opcode, uint8_t dlen, uint8_t data[]);
  virtual void readResp(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void readByGroupReq(uint16_t connectionHandle, uint16_t mtu, uint8_t dlen, uint8_t data[]);
  virtual int readByGroupReq(uint16_t connectionHandle, uint16_t startHandle, uint16_t endHandle, uint16_t uuid, uint8_t responseBuffer[]);
  virtual void readByGroupResp(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void writeReqOrCmd(uint16_t connectionHandle, uint16_t mtu, uint8_t op, uint8_t dlen, uint8_t data[]);
  virtual void writeResp(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void prepWriteReq(uint16_t connectionHandle, uint16_t mtu, uint8_t dlen, uint8_t data[]);
  virtual void execWriteReq(uint16_t connectionHandle, uint16_t mtu, uint8_t dlen, uint8_t data[]);
  virtual void handleNotifyOrInd(uint16_t connectionHandle, uint8_t opcode, uint8_t dlen, uint8_t data[]);
  virtual void handleCnf(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);
  virtual void sendError(uint16_t connectionHandle, uint8_t opcode, uint16_t handle, uint8_t code);

  virtual bool exchangeMtu(uint16_t connectionHandle);
  virtual bool discoverServices(uint16_t connectionHandle, BLERemoteDevice* device, const char* serviceUuidFilter);
  virtual bool discoverCharacteristics(uint16_t connectionHandle, BLERemoteDevice* device);
  virtual bool discoverDescriptors(uint16_t connectionHandle, BLERemoteDevice* device);

  virtual int sendReq(uint16_t connectionHandle, void* requestBuffer, int requestLength, uint8_t responseBuffer[]);

private:
  uint16_t _maxMtu;
  unsigned long _timeout;
  struct {
    uint16_t connectionHandle;
    uint8_t role;
    uint8_t addressType;
    uint8_t address[6];
    uint8_t resolvedAddress[6];
    uint16_t mtu;
    BLERemoteDevice* device;
    uint8_t encryption;
    uint8_t IOCap[3];
  } _peers[ATT_MAX_PEERS];

  volatile bool _cnf;

  uint16_t _longWriteHandle;
  uint8_t* _longWriteValue;
  uint16_t _longWriteValueLength;

  struct {
    uint16_t connectionHandle;
    uint8_t op;
    uint8_t* buffer;
    uint8_t length;
  } _pendingResp;

  BLEDeviceEventHandler _eventHandlers[2];
};

extern ATTClass& ATT;

#endif
