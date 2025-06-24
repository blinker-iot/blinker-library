/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2024 Arduino SA. All rights reserved.

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

#if defined(ARDUINO_SILABS)

#include "HCISilabsTransport.h"
#include "sl_string.h"

extern "C" {
#include "sl_btctrl_linklayer.h"
#include "sl_hci_common_transport.h"
}

extern "C" int strcasecmp(char const *a, char const *b) {
  return sl_strcasecmp(a, b);
}

static RingBufferN<512> buf;

HCISilabsTransportClass::HCISilabsTransportClass()
{
}

HCISilabsTransportClass::~HCISilabsTransportClass()
{
}

int HCISilabsTransportClass::begin()
{
  if(!sl_btctrl_is_initialized()) {
    sl_bt_controller_init(); 
  }

  /* Initialize adv & scan components */
  sl_btctrl_init_adv();
  sl_btctrl_init_scan();
  sl_btctrl_init_conn(); 
  sl_btctrl_init_adv_ext();
  sl_btctrl_init_scan_ext();

  /* Initialize HCI controller */
  sl_bthci_init_upper(); 
  sl_btctrl_hci_parser_init_default();
  sl_btctrl_hci_parser_init_conn();
  sl_btctrl_hci_parser_init_adv();
  sl_btctrl_hci_parser_init_phy();

  return 1;
}

void HCISilabsTransportClass::end()
{
  sl_bt_controller_deinit();
}

void HCISilabsTransportClass::wait(unsigned long timeout)
{
  for (unsigned long start = millis(); (millis() - start) < timeout;) {
    if (available()) {
      break;
    }
  }
}

int HCISilabsTransportClass::available()
{
  return buf.available();
}

int HCISilabsTransportClass::peek()
{
  return buf.peek();
}

int HCISilabsTransportClass::read()
{
  return buf.read_char();
}

size_t HCISilabsTransportClass::write(const uint8_t* data, size_t len)
{
  int ret = 0;
  ret = hci_common_transport_receive((uint8_t *)data, len, true);

  if (ret == 0) return len;

  return 0;
}

extern "C" {
  /**
   * @brief Transmit HCI message using the currently used transport layer.
   * The HCI calls this function to transmit a full HCI message.
   * @param[in] data Packet type followed by HCI packet data.
   * @param[in] len Length of the `data` parameter
   * @return 0 - on success, or non-zero on failure.
   */
  uint32_t hci_common_transport_transmit(uint8_t *data, int16_t len)
  {
    for (int i = 0; i < len; i++) {
      buf.store_char(data[i]);
      if (buf.isFull()) return SL_STATUS_FAIL;
    }
    
    sl_btctrl_hci_transmit_complete(0);
    return 0;
  }
}

HCISilabsTransportClass HCISilabsTransport;

HCITransportInterface& HCITransport = HCISilabsTransport;

#endif
