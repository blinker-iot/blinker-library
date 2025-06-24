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

#ifndef _HCI_CORDIO_TRANSPORT_H_
#define _HCI_CORDIO_TRANSPORT_H_

#include <string.h>

#include "api/RingBuffer.h"

#include "HCITransport.h"

class HCICordioTransportClass : public HCITransportInterface {
public:
  HCICordioTransportClass();
  virtual ~HCICordioTransportClass();

  virtual int begin();
  virtual void end();

  virtual void wait(unsigned long timeout);

  virtual int available();
  virtual int peek();
  virtual int read();

  virtual size_t write(const uint8_t* data, size_t length);

private:
  static void onDataReceived(uint8_t* data, uint8_t len);
  void handleRxData(uint8_t* data, uint8_t len);

private:
  bool _begun;
  RingBufferN<256> _rxBuf;
};

#endif
