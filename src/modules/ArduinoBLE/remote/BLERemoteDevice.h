/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

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

#ifndef _BLE_REMOTE_DEVICE_H_
#define _BLE_REMOTE_DEVICE_H_

#if defined(ARDUINO_ARCH_RENESAS)

#include "../utility/BLELinkedList.h"

#include "BLERemoteService.h"

class BLERemoteDevice /*: public BLEDevice*/ {
public:
  BLERemoteDevice();
  virtual ~BLERemoteDevice();

  void addService(BLERemoteService* service);

  unsigned int serviceCount() const;
  BLERemoteService* service(unsigned int index) const;

  void clearServices();

private:
  BLELinkedList<BLERemoteService*> _services;
};

#endif

#endif
