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

#if !defined(ARDUINO_ARCH_MBED) && !defined(ESP32) && !defined(ARDUINO_SILABS) && !defined(ARDUINO_UNOR4_WIFI) || defined(TARGET_NANO_RP2040_CONNECT) //|| defined(CORE_CM4)

#include "HCIUartTransport.h"

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
#define SerialHCI Serial2
#elif defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
// SerialHCI is already defined in the variant
#elif defined(ARDUINO_PORTENTA_H7_M4)
// SerialHCI is already defined in the variant
#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION)
#define SerialHCI Serial2
#elif defined(ARDUINO_OPTA)
#define SerialHCI Serial3
#elif defined(ARDUINO_PORTENTA_C33)
#define SerialHCI Serial5
#elif defined(ARDUINO_GIGA)
arduino::UART SerialHCI(CYBSP_BT_UART_TX, CYBSP_BT_UART_RX, CYBSP_BT_UART_RTS, CYBSP_BT_UART_CTS);
#else
#error "Unsupported board selected!"
#endif

HCIUartTransportClass::HCIUartTransportClass(HardwareSerial& uart, unsigned long baudrate) :
  _uart(&uart),
  _baudrate(baudrate)
{
}

HCIUartTransportClass::~HCIUartTransportClass()
{
}

int HCIUartTransportClass::begin()
{
  _uart->begin(_baudrate);

  return 1;
}

void HCIUartTransportClass::end()
{
  _uart->end();
}

void HCIUartTransportClass::wait(unsigned long timeout)
{
  for (unsigned long start = millis(); (millis() - start) < timeout;) {
    if (available()) {
      break;
    }
  }
}

int HCIUartTransportClass::available()
{
  return _uart->available();
}

int HCIUartTransportClass::peek()
{
  return _uart->peek();
}

int HCIUartTransportClass::read()
{
  return _uart->read();
}

size_t HCIUartTransportClass::write(const uint8_t* data, size_t length)
{
#ifdef ARDUINO_AVR_UNO_WIFI_REV2
  // wait while the CTS pin is low
  while (digitalRead(NINA_CTS) == HIGH);
#endif

  size_t result = _uart->write(data, length);

  _uart->flush();

  return result;
}

#if defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
HCIUartTransportClass HCIUartTransport(SerialHCI, 119600);
#else
HCIUartTransportClass HCIUartTransport(SerialHCI, 912600);
#endif
HCITransportInterface& HCITransport = HCIUartTransport;

#endif
