# WIFIduino32 S3 本地网关地址更新计划

## IDEA

| 项 | 内容 |
| --- | --- |
| Intent | 将 `ee` 板卡固件的网关地址和本地 Docker Gateway 公布的地址改为 `192.168.1.163`。 |
| Data | 当前 Mac 局域网地址为 `192.168.1.163`。已安装的 Wi-Fi/BLE 共存固件编译参数以及 Docker Gateway 原配置均使用 `192.168.1.108`。串口为 `/dev/cu.usbserial-310`。 |
| Edges | 复用原共存构建，仅调整控制面主机。写入前保存应用分区，只写原 app0，保留 NVS、OTA 数据和分区表。沿用原 Gateway 镜像，只重建网关容器。 |
| Answer | 交付更新后的固件写入和网关配置。按用户最后指令取消 App 验证和写入后的状态复测。 |

## 架构图

```mermaid
flowchart LR
  B["ee：原 Wi-Fi / BLE 共存固件"] -->|设备认证| G["192.168.1.163：本地 Gateway"]
  B -->|MQTT| G
  G --> R["原 Broker 与数据卷"]
  G --> P["原 PostgreSQL 与 Redis 数据卷"]
  B --- N["原 NVS：BLE 绑定、Wi-Fi 和设备密钥"]
```

## 数据流图

```mermaid
flowchart TD
  A["归档共存构建"] --> C["替换控制面主机并重编译"]
  B["备份板卡 app0"] --> D["仅写入 app0"]
  C --> D
  E["备份 Docker 环境配置"] --> F["替换 Gateway 公布主机"]
  F --> G["用同一镜像重建 Gateway"]
  D --> H["板卡按新地址启动"]
  G --> H
```
