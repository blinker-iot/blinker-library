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

#if defined(ESP32)

#include "HCIVirtualTransport.h"

StreamBufferHandle_t rec_buffer;
StreamBufferHandle_t send_buffer;
TaskHandle_t bleHandle;


static void notify_host_send_available(void)
{
}

static int notify_host_recv(uint8_t *data, uint16_t length)
{
  xStreamBufferSend(rec_buffer,data,length,portMAX_DELAY);  // !!!potentially waiting forever
  return 0;
}

static esp_vhci_host_callback_t vhci_host_cb = {
  notify_host_send_available,
  notify_host_recv
};

void bleTask(void *pvParameters)
{
  esp_vhci_host_register_callback(&vhci_host_cb);
  size_t length;
  uint8_t mybuf[258];

  while(true){
    length = xStreamBufferReceive(send_buffer,mybuf,258,portMAX_DELAY);
    while (!esp_vhci_host_check_send_available()) {}
    esp_vhci_host_send_packet(mybuf, length);
  }
}


HCIVirtualTransportClass::HCIVirtualTransportClass()
{
}

HCIVirtualTransportClass::~HCIVirtualTransportClass()
{
}

int HCIVirtualTransportClass::begin()
{
  btStarted(); // this somehow stops the arduino ide from initializing bluedroid

  rec_buffer = xStreamBufferCreate(258, 1);
  send_buffer = xStreamBufferCreate(258, 1);

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  
#if CONFIG_IDF_TARGET_ESP32
  bt_cfg.mode = ESP_BT_MODE_BLE; //original esp32 chip
#else
#if !(CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2)
  bt_cfg.bluetooth_mode = ESP_BT_MODE_BLE; //different api for newer models
#endif
#endif

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  xTaskCreatePinnedToCore(&bleTask, "bleTask", 2048, NULL, 5, &bleHandle, 0);
  return 1;
}

void HCIVirtualTransportClass::end()
{
  vStreamBufferDelete(rec_buffer);
  vStreamBufferDelete(send_buffer);
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  vTaskDelete(bleHandle);
}

void HCIVirtualTransportClass::wait(unsigned long timeout)
{
  for (unsigned long start = (esp_timer_get_time() / 1000ULL); ((esp_timer_get_time() / 1000ULL) - start) < timeout;) {
    if (available()) {
      break;
    }
  }
}

int HCIVirtualTransportClass::available()
{
  size_t bytes	= xStreamBufferBytesAvailable(rec_buffer);
  return bytes;
}

// never called
int HCIVirtualTransportClass::peek()
{
  return -1;
}

int HCIVirtualTransportClass::read()
{
  uint8_t c;
  if(xStreamBufferReceive(rec_buffer, &c, 1, portMAX_DELAY)) {
    return c;
  }
  return -1;
}

size_t HCIVirtualTransportClass::write(const uint8_t* data, size_t length)
{
  size_t result = xStreamBufferSend(send_buffer,data,length,portMAX_DELAY);
  return result;
}

HCIVirtualTransportClass HCIVirtualTransport;

HCITransportInterface& HCITransport = HCIVirtualTransport;

#endif
