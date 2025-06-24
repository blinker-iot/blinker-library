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

#include "HCI.h"
#include "ATT.h"
#include "btct.h"
#include "L2CAPSignaling.h"
#include "keyDistribution.h"
#include "bitDescriptions.h"
#define CONNECTION_PARAMETER_UPDATE_REQUEST  0x12
#define CONNECTION_PARAMETER_UPDATE_RESPONSE 0x13

//#define _BLE_TRACE_

L2CAPSignalingClass::L2CAPSignalingClass() :
  _minInterval(0),
  _maxInterval(0),
  _supervisionTimeout(0),
  _pairing_enabled(1)
{
}

L2CAPSignalingClass::~L2CAPSignalingClass()
{
}

void L2CAPSignalingClass::addConnection(uint16_t handle, uint8_t role, uint8_t /*peerBdaddrType*/,
                                        uint8_t /*peerBdaddr*/[6], uint16_t interval,
                                        uint16_t /*latency*/, uint16_t supervisionTimeout,
                                        uint8_t /*masterClockAccuracy*/)
{
  if (role != 1) {
    // ignore
    return;
  }

  bool updateParameters = false;
  uint16_t updatedMinInterval = interval;
  uint16_t updatedMaxInterval = interval;
  uint16_t updatedSupervisionTimeout = supervisionTimeout;

  if (_minInterval && _maxInterval) {
    if (interval < _minInterval || interval > _maxInterval) {
      updatedMinInterval = _minInterval;
      updatedMaxInterval = _maxInterval;
      updateParameters = true;
    }
  }

  if (_supervisionTimeout && supervisionTimeout != _supervisionTimeout) {
    updatedSupervisionTimeout = _supervisionTimeout;
    updateParameters = true;
  }

  if (updateParameters) {
    struct __attribute__ ((packed)) L2CAPConnectionParameterUpdateRequest {
      uint8_t code;
      uint8_t identifier;
      uint16_t length;
      uint16_t minInterval;
      uint16_t maxInterval;
      uint16_t latency;
      uint16_t supervisionTimeout;
    } request = { CONNECTION_PARAMETER_UPDATE_REQUEST, 0x01, 8,
                  updatedMinInterval, updatedMaxInterval, 0x0000, updatedSupervisionTimeout };

    HCI.sendAclPkt(handle, SIGNALING_CID, sizeof(request), &request);
  }
}

void L2CAPSignalingClass::handleData(uint16_t connectionHandle, uint8_t dlen, uint8_t data[])
{
  struct __attribute__ ((packed)) L2CAPSignalingHdr {
    uint8_t code;
    uint8_t identifier;
    uint16_t length;
  } *l2capSignalingHdr = (L2CAPSignalingHdr*)data;

  if (dlen < sizeof(L2CAPSignalingHdr)) {
    // too short, ignore
    return;
  }

  if (dlen != (sizeof(L2CAPSignalingHdr) + l2capSignalingHdr->length)) {
    // invalid length, ignore
    return;
  }

  uint8_t code = l2capSignalingHdr->code;
  uint8_t identifier = l2capSignalingHdr->identifier;
  uint16_t length = l2capSignalingHdr->length;
  data = &data[sizeof(L2CAPSignalingHdr)];

  if (code == CONNECTION_PARAMETER_UPDATE_REQUEST) {
    connectionParameterUpdateRequest(connectionHandle, identifier, length, data);
  } else if (code == CONNECTION_PARAMETER_UPDATE_RESPONSE) {
    connectionParameterUpdateResponse(connectionHandle, identifier, length, data);
  }
}
void L2CAPSignalingClass::handleSecurityData(uint16_t connectionHandle, uint8_t dlen, uint8_t data[])
{
  struct __attribute__ ((packed)) L2CAPSignalingHdr {
    uint8_t code;
    uint8_t data[64];
  } *l2capSignalingHdr = (L2CAPSignalingHdr*)data;
#ifdef _BLE_TRACE_
  Serial.print("dlen: ");
  Serial.println(dlen);
#else
  (void)dlen;
#endif
  uint8_t code = l2capSignalingHdr->code;

#ifdef _BLE_TRACE_
  Serial.print("handleSecurityData: code: 0x");
  Serial.println(code, HEX);
  Serial.print("rx security:");
  btct.printBytes(data,dlen);
#endif
  if (code == CONNECTION_PAIRING_REQUEST) {
	  
    if (isPairingEnabled()){
      if (_pairing_enabled >= 2) _pairing_enabled = 0;  // 2 = pair once only
      
      // 0x1
      struct __attribute__ ((packed)) PairingRequest {
        uint8_t ioCapability;
        uint8_t oobDataFlag;
        uint8_t authReq;
        uint8_t maxEncSize;
        uint8_t initiatorKeyDistribution;
        uint8_t responderKeyDistribution;
      } *pairingRequest = (PairingRequest*)l2capSignalingHdr->data;

      KeyDistribution responseKD = KeyDistribution();
      responseKD.setIdKey(true);

      ATT.remoteKeyDistribution = responseKD;// KeyDistribution(pairingRequest->initiatorKeyDistribution);
      ATT.localKeyDistribution = responseKD; //KeyDistribution(pairingRequest->responderKeyDistribution);
      // KeyDistribution rkd(pairingRequest->responderKeyDistribution);
      AuthReq req(pairingRequest->authReq);
#ifdef _BLE_TRACE_
      Serial.print("Req has properties: ");
      Serial.print(req.Bonding()?"bonding, ":"no bonding, ");
      Serial.print(req.CT2()?"CT2, ":"no CT2, ");
      Serial.print(req.KeyPress()?"KeyPress, ":"no KeyPress, ");
      Serial.print(req.MITM()?"MITM, ":"no MITM, ");
      Serial.print(req.SC()?"SC, ":"no SC, ");
#endif
    
      uint8_t peerIOCap[3];
      peerIOCap[0] = pairingRequest->authReq;
      peerIOCap[1] = pairingRequest->oobDataFlag;
      peerIOCap[2] = pairingRequest->ioCapability;
      ATT.setPeerIOCap(connectionHandle, peerIOCap);
      ATT.setPeerEncryption(connectionHandle, ATT.getPeerEncryption(connectionHandle) | PEER_ENCRYPTION::PAIRING_REQUEST);
#ifdef _BLE_TRACE_
      Serial.print("Peer encryption : 0b");
      Serial.println(ATT.getPeerEncryption(connectionHandle), BIN);
#endif
      struct __attribute__ ((packed)) PairingResponse {
        uint8_t code;
        uint8_t ioCapability;
        uint8_t oobDataFlag;
        uint8_t authReq;
        uint8_t maxEncSize;
        uint8_t initiatorKeyDistribution;
        uint8_t responderKeyDistribution;
      } response = { CONNECTION_PAIRING_RESPONSE, HCI.localIOCap(), 0, HCI.localAuthreq().getOctet(), 0x10, responseKD.getOctet(), responseKD.getOctet()};
     
      HCI.sendAclPkt(connectionHandle, SECURITY_CID, sizeof(response), &response);
      
    } else {
      // Pairing not enabled
      uint8_t ret[2] = {CONNECTION_PAIRING_FAILED, 0x05}; // reqect pairing
      HCI.sendAclPkt(connectionHandle, SECURITY_CID, sizeof(ret), ret);
      ATT.setPeerEncryption(connectionHandle, NO_ENCRYPTION);
    }
  }
  else if (code == CONNECTION_PAIRING_RANDOM)
  {
    struct __attribute__ ((packed)) PairingRandom {
      uint8_t Na[16];
    } *pairingRandom = (PairingRandom*)l2capSignalingHdr->data;
    for(int i=0; i<16; i++){
      HCI.Na[15-i] = pairingRandom->Na[i];
    }
#ifdef _BLE_TRACE_
    Serial.println("[Info] Pairing random.");
#endif
    struct __attribute__ ((packed)) PairingResponse {
      uint8_t code;
      uint8_t Nb[16];
    } response = { CONNECTION_PAIRING_RANDOM, 0};
    for(int i=0; i< 16; i++) response.Nb[15-i] = HCI.Nb[i];

    HCI.sendAclPkt(connectionHandle, SECURITY_CID, sizeof(response), &response);

    // We now have all needed for compare value
    uint8_t g2Result[4];
    uint8_t U[32];
    uint8_t V[32];
    
    for(int i=0; i<32; i++){
      U[31-i] = HCI.remotePublicKeyBuffer[i];
      V[31-i] = HCI.localPublicKeyBuffer[i];
    }

    btct.g2(U,V,HCI.Na,HCI.Nb, g2Result);
    uint32_t result = 0;
    for(int i=0; i<4; i++) result += g2Result[3-i] << 8*i;

#ifdef _BLE_TRACE_
    Serial.print("U      : ");
    btct.printBytes(U,32);
    Serial.print("V      : ");
    btct.printBytes(V,32);
    Serial.print("X      : ");
    btct.printBytes(HCI.Na,16);
    Serial.print("Y      : ");
    btct.printBytes(HCI.Nb,16);
    Serial.print("g2res  : ");
    btct.printBytes(g2Result,4);
    Serial.print("Result : ");
    Serial.println(result);
#endif

    if(HCI._displayCode!=0){
      HCI._displayCode(result%1000000);
    }
    if(HCI._binaryConfirmPairing!=0){
      if(!HCI._binaryConfirmPairing()){
#ifdef _BLE_TRACE_
        Serial.println("User rejection");
#endif
        uint8_t rejection[2];
        rejection[0] = CONNECTION_PAIRING_FAILED;
        rejection[1] = 0x0C; // Numeric comparison failed
        HCI.sendAclPkt(connectionHandle, SECURITY_CID, 2, rejection);
        ATT.setPeerEncryption(connectionHandle, PEER_ENCRYPTION::NO_ENCRYPTION);
      }else{
#ifdef _BLE_TRACE_
        Serial.println("User did confirm");
#endif
      }
    }
  }
  else if (code == CONNECTION_PAIRING_RESPONSE)
  {
  }
  else if(code == CONNECTION_PAIRING_FAILED)
  {
#ifdef _BLE_TRACE_
    struct __attribute__ ((packed)) PairingFailed
    {
      uint8_t code;
      uint8_t reason;
    } *pairingFailed = (PairingFailed*)data;
    Serial.print("Pairing failed with code: 0x");
    Serial.println(pairingFailed->reason,HEX);
#endif
    ATT.setPeerEncryption(connectionHandle, PEER_ENCRYPTION::NO_ENCRYPTION);
  }
  else if (code == CONNECTION_IDENTITY_INFORMATION){
    struct __attribute__ ((packed)) IdentityInformation {
      uint8_t code;
      uint8_t PeerIRK[16];
    } *identityInformation = (IdentityInformation*)data;
    for(int i=0; i<16; i++) ATT.peerIRK[15-i] = identityInformation->PeerIRK[i];
#ifdef _BLE_TRACE_
    Serial.println("Saved peer IRK");
#endif
  }
  else if (code == CONNECTION_IDENTITY_ADDRESS){
    struct __attribute__ ((packed)) IdentityAddress {
      uint8_t code;
      uint8_t addressType;
      uint8_t address[6];
    } *identityAddress = (IdentityAddress*)data;
    // we can save this information now.
    uint8_t peerAddress[6];
    for(int i=0; i<6; i++) peerAddress[5-i] = identityAddress->address[i];

    HCI.saveNewAddress(identityAddress->addressType, peerAddress, ATT.peerIRK, ATT.localIRK);
    if(HCI._storeLTK!=0){
      HCI._storeLTK(peerAddress, HCI.LTK);
    }
  }
  else if (code == CONNECTION_PAIRING_PUBLIC_KEY){
    /// Received a public key
    struct __attribute__ ((packed)) ConnectionPairingPublicKey {
      uint8_t x[32];
      uint8_t y[32];
    } *connectionPairingPublicKey = (ConnectionPairingPublicKey*)l2capSignalingHdr->data;
    struct __attribute__ ((packed)) GenerateDHKeyCommand {
      uint8_t x[32];
      uint8_t y[32];
    } generateDHKeyCommand = {
      {0x00},
      {0x00},
    };
    memcpy(generateDHKeyCommand.x,connectionPairingPublicKey->x,32);
    memcpy(generateDHKeyCommand.y,connectionPairingPublicKey->y,32);

    if(ATT.setPeerEncryption(connectionHandle, ATT.getPeerEncryption(connectionHandle) | PEER_ENCRYPTION::REQUESTED_ENCRYPTION)){
#ifdef _BLE_TRACE_
      Serial.println("[Info] Pairing public key");
      Serial.println("Requested encryption stored.");
#endif
    }else{
#ifdef _BLE_TRACE_
      Serial.println("[Info] Pairing public key");
      Serial.print("Failed to store encryption request with handle: 0x");
      Serial.println(connectionHandle,HEX);
#endif
    }
    
    memcpy(HCI.remotePublicKeyBuffer,&generateDHKeyCommand,sizeof(generateDHKeyCommand));
    HCI.sendCommand( (OGF_LE_CTL << 10 )| LE_COMMAND::READ_LOCAL_P256, 0);
  }
  else if(code == CONNECTION_PAIRING_DHKEY_CHECK)
  {
    uint8_t RemoteDHKeyCheck[16];
    for(int i=0; i<16; i++) RemoteDHKeyCheck[15-i] = l2capSignalingHdr->data[i];
    

#ifdef _BLE_TRACE_
    Serial.println("[Info] DH Key check");
    Serial.print("Remote DHKey Check: ");
    btct.printBytes(RemoteDHKeyCheck, 16);
#endif

    
    
    uint8_t encryptionState = ATT.getPeerEncryption(connectionHandle) | PEER_ENCRYPTION::RECEIVED_DH_CHECK;
    ATT.setPeerEncryption(connectionHandle, encryptionState);
    if((encryptionState & PEER_ENCRYPTION::DH_KEY_CALULATED) == 0){
#ifdef _BLE_TRACE_
      Serial.println("DHKey not yet ready, will calculate f5, f6 later");
#endif
      // store RemoteDHKeyCheck for later check
      memcpy(HCI.remoteDHKeyCheckBuffer,RemoteDHKeyCheck,16);

    } else {
      // We've already calculated the DHKey so we can calculate our check and send it.
      smCalculateLTKandConfirm(connectionHandle, RemoteDHKeyCheck);
      
    }
  }
}

void L2CAPSignalingClass::smCalculateLTKandConfirm(uint16_t handle, uint8_t expectedEa[])
{ // Authentication stage 2: LTK Calculation
  
  uint8_t localAddress[7];
  uint8_t remoteAddress[7];
  ATT.getPeerAddrWithType(handle, remoteAddress);
  
  HCI.readBdAddr();
  memcpy(&localAddress[1],HCI.localAddr,6);
  localAddress[0] = 0; // IOT 33 uses a static address // TODO: confirm for Nano BLE

  // Compute the LTK and MacKey
  uint8_t MacKey[16];
  btct.f5(HCI.DHKey, HCI.Na, HCI.Nb, remoteAddress, localAddress, MacKey, HCI.LTK);

  // Compute Ea and Eb
  uint8_t Ea[16];
  uint8_t Eb[16];
  uint8_t R[16];
  uint8_t MasterIOCap[3];
  uint8_t SlaveIOCap[3] = {HCI.localAuthreq().getOctet(), 0x0, HCI.localIOCap()};
  
  ATT.getPeerIOCap(handle, MasterIOCap);
  for(int i=0; i<16; i++) R[i] = 0;
  
  btct.f6(MacKey, HCI.Na,HCI.Nb,R, MasterIOCap, remoteAddress, localAddress, Ea);
  btct.f6(MacKey, HCI.Nb,HCI.Na,R, SlaveIOCap, localAddress, remoteAddress, Eb);

#ifdef _BLE_TRACE_
  Serial.println("Calculate and confirm LTK via f5, f6:");
  Serial.print("DHKey      : ");  btct.printBytes(HCI.DHKey,32);
  Serial.print("Na         : ");  btct.printBytes(HCI.Na,16);
  Serial.print("Nb         : ");  btct.printBytes(HCI.Nb,16);
  Serial.print("MacKey     : ");  btct.printBytes(MacKey,16);
  Serial.print("LTK        : ");  btct.printBytes(HCI.LTK,16);
  Serial.print("Expected Ea: ");  btct.printBytes(expectedEa, 16);
  Serial.print("Ea         : ");  btct.printBytes(Ea, 16);
  Serial.print("Eb         : ");  btct.printBytes(Eb,16);
  Serial.print("Local Addr : ");  btct.printBytes(localAddress, 7);
  Serial.print("LocalIOCap : ");  btct.printBytes(SlaveIOCap, 3);
  Serial.print("MasterAddr : ");  btct.printBytes(remoteAddress, 7);
  Serial.print("MasterIOCAP: ");  btct.printBytes(MasterIOCap, 3);
#endif
      
  // Check if Ea = expectedEa
  if (memcmp(Ea, expectedEa, 16) == 0){
    // Check ok
    // Send our confirmation value to complete authentication stage 2
    uint8_t ret[17];
    ret[0] = CONNECTION_PAIRING_DHKEY_CHECK;
    for(uint32_t i=0; i<sizeof(Eb); i++){
      ret[sizeof(Eb)-i] = Eb[i];
    }
    HCI.sendAclPkt(handle, SECURITY_CID, sizeof(ret), ret );
    uint8_t encryption = ATT.getPeerEncryption(handle) | PEER_ENCRYPTION::SENT_DH_CHECK;
    ATT.setPeerEncryption(handle, encryption);
#ifdef _BLE_TRACE_
    Serial.println("DHKey check ok - send Eb back");
#endif

  } else {
    // Check failed, abort pairing
    uint8_t ret[2] = {CONNECTION_PAIRING_FAILED, 0x0B}; // 0x0B = DHKey Check Failed
    HCI.sendAclPkt(handle, SECURITY_CID, sizeof(ret), ret);
    ATT.setPeerEncryption(handle, NO_ENCRYPTION);
#ifdef _BLE_TRACE_
    Serial.println("Error: DHKey check failed - Aborting");
#endif
  }
}

void L2CAPSignalingClass::removeConnection(uint8_t /*handle*/, uint16_t /*reason*/)
{
}

void L2CAPSignalingClass::setConnectionInterval(uint16_t minInterval, uint16_t maxInterval)
{
  _minInterval = minInterval;
  _maxInterval = maxInterval;
}

void L2CAPSignalingClass::setSupervisionTimeout(uint16_t supervisionTimeout)
{
  _supervisionTimeout = supervisionTimeout;
}

void L2CAPSignalingClass::setPairingEnabled(uint8_t enabled)
{
  _pairing_enabled = enabled;
}
bool L2CAPSignalingClass::isPairingEnabled()
{
  return _pairing_enabled > 0;
}

void L2CAPSignalingClass::connectionParameterUpdateRequest(uint16_t handle, uint8_t identifier, uint8_t dlen, uint8_t data[])
{
  struct __attribute__ ((packed)) L2CAPConnectionParameterUpdateRequest {
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t latency;
    uint16_t supervisionTimeout;
  } *request = (L2CAPConnectionParameterUpdateRequest*)data;

  if (dlen < sizeof(L2CAPConnectionParameterUpdateRequest)) {
    // too short, ignore
    return;
  }

  struct __attribute__ ((packed)) L2CAPConnectionParameterUpdateResponse {
    uint8_t code;
    uint8_t identifier;
    uint16_t length;
    uint16_t value;
  } response = { CONNECTION_PARAMETER_UPDATE_RESPONSE, identifier, 2, 0x0000 };

  if (_minInterval && _maxInterval) {
    if (request->minInterval < _minInterval || request->maxInterval > _maxInterval) {
      response.value = 0x0001; // reject
    }
  }

  if  (_supervisionTimeout) {
    if (request->supervisionTimeout != _supervisionTimeout) {
      response.value = 0x0001; // reject
    }
  }

  HCI.sendAclPkt(handle, SIGNALING_CID, sizeof(response), &response);

  if (response.value == 0x0000) {
    HCI.leConnUpdate(handle, request->minInterval, request->maxInterval, request->latency, request->supervisionTimeout);
  }
}

void L2CAPSignalingClass::connectionParameterUpdateResponse(uint16_t /*handle*/, uint8_t /*identifier*/, uint8_t /*dlen*/, uint8_t /*data*/[])
{
}

#if !defined(FAKE_L2CAP)
L2CAPSignalingClass L2CAPSignalingObj;
L2CAPSignalingClass& L2CAPSignaling = L2CAPSignalingObj;
#endif
