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

#if defined(ARDUINO_ARCH_MBED) && !defined(TARGET_NANO_RP2040_CONNECT) // && !defined(CORE_CM4)
#include <Arduino.h>
#include <mbed.h>

#include <driver/CordioHCITransportDriver.h>
#include <driver/CordioHCIDriver.h>

#if defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
#include "ble/BLE.h"
#include <events/mbed_events.h>
#endif

// Parts of this file are based on: https://github.com/ARMmbed/mbed-os-cordio-hci-passthrough/pull/2
// With permission from the Arm Mbed team to re-license

#if CORDIO_ZERO_COPY_HCI
#include <wsf_types.h>
#include <wsf_buf.h>
#include <wsf_msg.h>
#include <wsf_os.h>
#include <wsf_buf.h>
#include <wsf_timer.h>

/* avoid many small allocs (and WSF doesn't have smaller buffers) */
#define MIN_WSF_ALLOC (16)
#endif //CORDIO_ZERO_COPY_HCI

#include "HCICordioTransport.h"

#if (MBED_VERSION > MBED_ENCODE_VERSION(6, 2, 0))
#define BLE_NAMESPACE ble 
#else
#define BLE_NAMESPACE ble::vendor::cordio
#endif

#include "CordioHCICustomDriver.h"

extern BLE_NAMESPACE::CordioHCIDriver& ble_cordio_get_hci_driver();
extern "C" void hciTrSerialRxIncoming(uint8_t *pBuf, uint8_t len);

namespace BLE_NAMESPACE {
  struct CordioHCIHook {
    static CordioHCIDriver& getDriver() {
      return ble_cordio_get_hci_driver();
    }

    static CordioHCITransportDriver& getTransportDriver() {
      return getDriver()._transport_driver;
    }

    static void setDataReceivedHandler(void (*handler)(uint8_t*, uint8_t)) {
      getTransportDriver().set_data_received_handler(handler);
    }
  };
}

using BLE_NAMESPACE::CordioHCIHook;

#if CORDIO_ZERO_COPY_HCI
extern uint8_t *SystemHeapStart;
extern uint32_t SystemHeapSize;

void init_wsf(BLE_NAMESPACE::buf_pool_desc_t& buf_pool_desc) {
    static bool init = false;

    if (init) {
      return;
    }
    init = true;

    // use the buffer for the WSF heap
    SystemHeapStart = buf_pool_desc.buffer_memory;
    SystemHeapSize = buf_pool_desc.buffer_size;

    // Initialize buffers with the ones provided by the HCI driver
    uint16_t bytes_used = WsfBufInit(
        buf_pool_desc.pool_count,
        (wsfBufPoolDesc_t*)buf_pool_desc.pool_description
    );

    // Raise assert if not enough memory was allocated
    MBED_ASSERT(bytes_used != 0);

    SystemHeapStart += bytes_used;
    SystemHeapSize -= bytes_used;

    WsfTimerInit();
}


extern "C" void wsf_mbed_ble_signal_event(void)
{
    // do nothing
}
#endif //CORDIO_ZERO_COPY_HCI

static void bleLoop()
{
#if CORDIO_ZERO_COPY_HCI
    uint64_t last_update_us = 0;
    mbed::LowPowerTimer timer;

    timer.start();

    while (true) {
        last_update_us += (uint64_t) timer.read_high_resolution_us();
        timer.reset();

        uint64_t last_update_ms = (last_update_us / 1000);
        wsfTimerTicks_t wsf_ticks = (last_update_ms / WSF_MS_PER_TICK);

        if (wsf_ticks > 0) {
            WsfTimerUpdate(wsf_ticks);
            last_update_us -= (last_update_ms * 1000);
        }

        wsfOsDispatcher();

        bool sleep = false;
        {
            /* call needs interrupts disabled */
            mbed::CriticalSectionLock critical_section;
            if (wsfOsReadyToSleep()) {
                sleep = true;
            }
        }

        uint64_t time_spent = (uint64_t) timer.read_high_resolution_us();

        /* don't bother sleeping if we're already past tick */
        if (sleep && (WSF_MS_PER_TICK * 1000 > time_spent)) {
            /* sleep to maintain constant tick rate */
            uint64_t wait_time_us = WSF_MS_PER_TICK * 1000 - time_spent;
            uint64_t wait_time_ms = wait_time_us / 1000;

            wait_time_us = wait_time_us % 1000;

            if (wait_time_ms) {
              rtos::ThisThread::sleep_for(wait_time_ms);
            }

            if (wait_time_us) {
              wait_us(wait_time_us);
            }
        }
    }
#else
    while(true) {
        rtos::ThisThread::sleep_for(osWaitForever);
    }
#endif // CORDIO_ZERO_COPY_HCI
}

static rtos::EventFlags bleEventFlags; 
static rtos::Thread* bleLoopThread = NULL;


HCICordioTransportClass::HCICordioTransportClass() :
  _begun(false)
{
}

HCICordioTransportClass::~HCICordioTransportClass()
{
}

#if (defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)) && !defined(CUSTOM_HCI_DRIVER)
events::EventQueue eventQueue(10 * EVENTS_EVENT_SIZE);
void scheduleMbedBleEvents(BLE::OnEventsToProcessCallbackContext *context) {
  eventQueue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

void completeCallback(BLE::InitializationCompleteCallbackContext *context) {
  eventQueue.break_dispatch();
}
#endif

int HCICordioTransportClass::begin()
{
  _rxBuf.clear();

#if CORDIO_ZERO_COPY_HCI
  BLE_NAMESPACE::buf_pool_desc_t bufPoolDesc = CordioHCIHook::getDriver().get_buffer_pool_description();
  init_wsf(bufPoolDesc);
#endif

#if (defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)) && !defined(CUSTOM_HCI_DRIVER)

  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(scheduleMbedBleEvents);

  ble.init(completeCallback);
  eventQueue.dispatch(10000);

  if (!ble.hasInitialized()){
    return 0;
  } 
#else 
  CordioHCIHook::getDriver().initialize();
#endif

  if (bleLoopThread == NULL) {
    bleLoopThread = new rtos::Thread();
    bleLoopThread->start(bleLoop);
  }

  CordioHCIHook::setDataReceivedHandler(HCICordioTransportClass::onDataReceived);

  _begun = true;

  return 1;
}

void HCICordioTransportClass::end()
{
  if (bleLoopThread != NULL) {
    bleLoopThread->terminate();
    delete bleLoopThread;
    bleLoopThread = NULL;
  }
  // Reset the callback with the mbed-os default handler to properly handle the following CYW43xxx chip initializations and begins
  CordioHCIHook::setDataReceivedHandler(hciTrSerialRxIncoming);

#if (defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)) && !defined(CUSTOM_HCI_DRIVER)
  BLE &ble = BLE::Instance();
  ble.shutdown();
#endif

#if !defined(TARGET_STM32H7)
  CordioHCIHook::getDriver().terminate();
#endif

  _begun = false;
}

void HCICordioTransportClass::wait(unsigned long timeout)
{
  if (available()) {
    return;
  }

  // wait for handleRxData to signal
  bleEventFlags.wait_all(0x01, timeout, true);
}

int HCICordioTransportClass::available()
{
  return _rxBuf.available();
}

int HCICordioTransportClass::peek()
{
  return _rxBuf.peek();
}

int HCICordioTransportClass::read()
{
  return _rxBuf.read_char();
}

size_t HCICordioTransportClass::write(const uint8_t* data, size_t length)
{
  if (!_begun) {
    return 0;
  }

  uint8_t packetLength = length - 1;
  uint8_t packetType   = data[0];

#if CORDIO_ZERO_COPY_HCI
  uint8_t* packet = (uint8_t*)WsfMsgAlloc(max(packetLength, MIN_WSF_ALLOC));

  memcpy(packet, &data[1], packetLength);

  return CordioHCIHook::getTransportDriver().write(packetType, packetLength, packet);
#else
  return CordioHCIHook::getTransportDriver().write(packetType, packetLength, (uint8_t*)&data[1]);
#endif
}

void HCICordioTransportClass::handleRxData(uint8_t* data, uint8_t len)
{
  if (_rxBuf.availableForStore() < len) {
    // drop!
    return;
  }

  for (int i = 0; i < len; i++) {
    _rxBuf.store_char(data[i]);
  }

  bleEventFlags.set(0x01);
}

HCICordioTransportClass HCICordioTransport;
HCITransportInterface& HCITransport = HCICordioTransport;

void HCICordioTransportClass::onDataReceived(uint8_t* data, uint8_t len)
{
  HCICordioTransport.handleRxData(data, len);
}

#endif
