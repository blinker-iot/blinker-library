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

#ifndef _L2CAP_SIGNALING_H_
#define _L2CAP_SIGNALING_H_

#include <Arduino.h>

#define SIGNALING_CID 0x0005
#define SECURITY_CID 0x0006


#define CONNECTION_PAIRING_REQUEST        0x01
#define CONNECTION_PAIRING_RESPONSE       0x02
#define CONNECTION_PAIRING_CONFIRM        0x03
#define CONNECTION_PAIRING_RANDOM         0x04
#define CONNECTION_PAIRING_FAILED         0x05
#define CONNECTION_ENCRYPTION_INFORMATION 0x06
#define CONNECTION_MASTER_IDENTIFICATION  0x07
#define CONNECTION_IDENTITY_INFORMATION   0x08
#define CONNECTION_IDENTITY_ADDRESS       0x09
#define CONNECTION_SIGNING_INFORMATION    0x0A
#define CONNECTION_SECURITY_REQUEST       0x0B
#define CONNECTION_PAIRING_PUBLIC_KEY     0x0C
#define CONNECTION_PAIRING_DHKEY_CHECK    0x0D
#define CONNECTION_PAIRING_KEYPRESS       0x0E

#define IOCAP_DISPLAY_ONLY         0x00
#define IOCAP_DISPLAY_YES_NO       0x01
#define IOCAP_KEYBOARD_ONLY        0x02
#define IOCAP_NO_INPUT_NO_OUTPUT   0x03
#define IOCAP_KEYBOARD_DISPLAY     0x04


#define LOCAL_AUTHREQ 0b00101101
// #define LOCAL_IOCAP   IOCAP_DISPLAY_ONLY // will use JustWorks pairing

class L2CAPSignalingClass {
public:
  L2CAPSignalingClass();
  virtual ~L2CAPSignalingClass();

  virtual void addConnection(uint16_t handle, uint8_t role, uint8_t peerBdaddrType,
                    uint8_t peerBdaddr[6], uint16_t interval,
                    uint16_t latency, uint16_t supervisionTimeout,
                    uint8_t masterClockAccuracy);

  virtual void handleData(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);

  virtual void handleSecurityData(uint16_t connectionHandle, uint8_t dlen, uint8_t data[]);

  virtual void removeConnection(uint8_t handle, uint16_t reason);

  virtual void setConnectionInterval(uint16_t minInterval, uint16_t maxInterval);

  virtual void setSupervisionTimeout(uint16_t supervisionTimeout);
  
  virtual void setPairingEnabled(uint8_t enabled);
  virtual bool isPairingEnabled();



  virtual void smCalculateLTKandConfirm(uint16_t handle, uint8_t expectedEa[]);


private:
  virtual void connectionParameterUpdateRequest(uint16_t handle, uint8_t identifier, uint8_t dlen, uint8_t data[]);
  virtual void connectionParameterUpdateResponse(uint16_t handle, uint8_t identifier, uint8_t dlen, uint8_t data[]);


private:
  uint16_t _minInterval;
  uint16_t _maxInterval;
  uint16_t _supervisionTimeout;
  uint8_t _pairing_enabled;
};

extern L2CAPSignalingClass& L2CAPSignaling;

#endif
