# blinker-library

Blinker 嵌入式硬件库。当前支持 ESP32、Arduino UNO R4 WiFi、Raspberry Pi Pico W/Pico 2 W，以及通过串口 BLE 模式接入的 Arduino 通用板。

[English README](README.md)

## 当前芯片与功能支持矩阵

本表按当前源码入口、平台适配层和示例目录整理。`BLINKER_WIFI` 当前会进入 MQTT 云连接；非 ESP32 的 BLE 支持指串口 BLE 外设模式。

| 功能 | ESP32 | Arduino UNO R4 WiFi | Raspberry Pi Pico W / Pico 2 W | Arduino 通用板（串口 BLE） |
| --- | --- | --- | --- | --- |
| WiFi/MQTT 云连接 | ✅ | ✅ | ✅ | ❌ |
| HTTP 模式 | ✅ | ✅ | ✅ | ❌ |
| BLE 控制 | ✅ | ✅ | ✅ | ✅ |
| 基础 Widget | ✅ | ✅ | ✅ | ✅ |
| Notify/Print/Heartbeat | ✅ | ✅ | ✅ | ✅ |
| 时间/定时/自动化 | ✅ | ✅ | ✅ | ❌ |
| 云端数据/配置/实时数据 | ✅ | ✅ | ✅ | ❌ |
| SmartConfig / ESPTouch v1 配网 | ✅ | ❌ | ❌ | ❌ |
| ESPTouch v2 配网 | ✅ | ❌ | ❌ | ❌ |
| bleFi 配网 | ✅ | ❌ | ❌ | ❌ |
| WiFiMulti 多热点连接 | ✅ | ❌ | ❌ | ❌ |

已移除且当前不再支持的功能：`BLINKER_PRO`、AT/蜂窝模组、WiFi Gateway、Bridge、Weather/Air/AQI、SMS、微信、语音助手、WiFi SubDevice/Mesh、ESP32-CAM、OTA、Push/Log/GPS/坐标、APConfig、APConfig V2。

## 为什么叫 blinker

当我们接触学习一个新的嵌入式硬件平台时，通常是从 **Blink** 点亮一颗 LED 开始的。
我们把 **Blink** 当作硬件开发第一课的 **hello world**。
我们期望做像 **Blink** 点亮一颗 LED 那样简单的物联网解决方案。
根据这个灵感，我们将这个项目命名为 **blinker**。

## 什么是 blinker

[blinker](https://blinker.app/) 是一套跨硬件、跨平台的物联网解决方案，提供 App 端、设备端、服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、数据监测等领域，帮助用户更好更快地搭建物联网项目。

## 参考

* [英文参考](https://github.com/blinker-iot/blinker-doc/wiki/Blinker-Arduino-library-reference)
* [中文参考](https://github.com/blinker-iot/blinker-doc/wiki/Blinker-Arduino-%E5%BA%93%E4%BD%BF%E7%94%A8%E6%89%8B%E5%86%8C)
* [中文官网文档](https://diandeng.tech/doc)

## 本项目内部集成了如下库

[WebSockets](https://github.com/Links2004/arduinoWebSockets) - Blinker 用这个库建立了 websocket 服务器
[Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library) - Blinker 用这个库建立了 MQTT 客户端
[ArduinoJson](https://github.com/bblanchon/ArduinoJson) - Blinker 用这个库解析 JSON
[base64](https://github.com/adamvr/arduino-base64) - Blinker 用这个库进行 Base64 编解码
[TaskScheduler](https://github.com/arkhipenko/TaskScheduler) - Blinker 用这个库进行任务调度

由于版本更新频繁，个别项目可能没有及时添加，望理解。如有发现，欢迎告知我们。
