# blinker-library

Blinker library for embedded hardware. Works with ESP32, Arduino UNO R4 WiFi, Raspberry Pi Pico W/Pico 2 W, and generic Arduino boards through serial BLE mode.

[中文说明](README_zh.md)

## Supported Chips and Features

This table is based on the current source entry points, platform adapters, and examples. `BLINKER_WIFI` currently uses the MQTT cloud connection path. BLE support on non-ESP32 boards means serial BLE peripheral mode.

| Feature | ESP32 | Arduino UNO R4 WiFi | Raspberry Pi Pico W / Pico 2 W | Generic Arduino board (serial BLE) |
| --- | --- | --- | --- | --- |
| WiFi/MQTT cloud connection | ✅ | ✅ | ✅ | ❌ |
| HTTP mode | ✅ | ✅ | ✅ | ❌ |
| BLE control | ✅ | ✅ | ✅ | ✅ |
| Basic widgets | ✅ | ✅ | ✅ | ✅ |
| Notify/Print/Heartbeat | ✅ | ✅ | ✅ | ✅ |
| Time/Timer/Automation | ✅ | ✅ | ✅ | ❌ |
| Cloud data/config/realtime data | ✅ | ✅ | ✅ | ❌ |
| SmartConfig / ESPTouch v1 provisioning | ✅ | ❌ | ❌ | ❌ |
| ESPTouch v2 provisioning | ✅ | ❌ | ❌ | ❌ |
| bleFi provisioning | ✅ | ❌ | ❌ | ❌ |
| WiFiMulti multi-AP connection | ✅ | ❌ | ❌ | ❌ |

Removed and no longer supported: `BLINKER_PRO`, AT/cellular modules, WiFi Gateway, Bridge, Weather/Air/AQI, SMS, WeChat, voice assistants, WiFi SubDevice/Mesh, ESP32-CAM, OTA, Push/Log/GPS/Coordinate, APConfig, APConfig V2.

## Why named blinker

When we learn a new embedded hardware platform, we usually start with the sketch **Blink**.
We think **Blink** is the **hello world** of embedded hardware.
We aim to build an easy-to-learn IoT solution, as simple as blinking an LED.
So we named this project **blinker** with this inspiration.

## What's blinker

[blinker](https://blinker.app/) is a cross-hardware, cross-platform solution for the IoT. It provides app, device, and server support, and uses public cloud services for data transmission and storage. It can be used in smart home, data monitoring, and other fields to help users build Internet of Things projects better and faster.

## Reference

* [English reference](https://github.com/blinker-iot/blinker-doc/wiki/Blinker-Arduino-library-reference)
* [Chinese reference](https://github.com/blinker-iot/blinker-doc/wiki/Blinker-Arduino-%E5%BA%93%E4%BD%BF%E7%94%A8%E6%89%8B%E5%86%8C)
* [Chinese official docs](https://diandeng.tech/doc)

## Thanks

[WebSockets](https://github.com/Links2004/arduinoWebSockets) - for Blinker to build up a websocket server
[Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library) - for Blinker to build up an MQTT client
[ArduinoJson](https://github.com/bblanchon/ArduinoJson) - for Blinker to format JSON data
[base64](https://github.com/adamvr/arduino-base64) - for Blinker to encode and decode Base64 data
[TaskScheduler](https://github.com/arkhipenko/TaskScheduler) - for Blinker to schedule cooperative tasks

Due to frequent version updates, individual items may not be added in time. If you find one, please let us know.
