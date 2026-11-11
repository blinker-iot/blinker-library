# WIFIduino32 S3 本地网关地址更新记录

## Intent

将 `ee` 板卡与本地 Docker Gateway 的地址更新为 `192.168.1.163`，沿用原 Wi-Fi/BLE 共存配置。见[更新计划与架构图](WIFIduino32S3本地网关地址更新计划.md)。

## Data

- 复用 2026-10-16 共存固件归档、ESP32 Arduino SDK 3.3.1 和原 Ninja 构建规则。隔离构建中只改控制面主机宏，生成 1,279,344 字节的应用镜像。
- 写入前从板卡读取实际分区表，app0 范围为 `0x10000–0x150000`。连续读取在高速串口下中断，改用五块分段读取后得到完整的 1,310,720 字节原应用备份。
- 原应用、分区表、原 Gateway 环境配置、构建及写入日志位于 `.build/wifiduino-s3-lan-20261111.xLzVAmO2/`。此目录是私有备份。

## Edges

- 写入范围只有 app0。NVS 中的原 BLE 绑定、Wi-Fi 配置和设备密钥得到保留。
- Docker 仅以原镜像重建 Gateway，PostgreSQL、Redis 和 Broker 沿用原容器与数据卷。

## Answer

- 本地 Docker 的 `GATEWAY_PUBLIC_HOST` 已更新为 `192.168.1.163`，Compose 完成 Gateway 容器重建和启动。
- esptool 完成应用分区写入并复位板卡。
- 按用户最后指令取消 App 验证及写入后状态复测。
