# WIFIduino32 S3 自有云端切换记录

## Intent

让板卡以当前 Development 账号自有设备上报实时数据，保留原 `ee` 的 Direct BLE 绑定。见[切换计划与架构图](WIFIduino32S3自有云端切换计划.md)。

## Data

- 从旧会话记录确认两条设备身份来自同一块物理板卡。
- 沿用已部署的 10 月 16 日共存固件和 `192.168.1.163` 网关地址。隔离构建复用 `ConfiguredDeviceKeySource` 供 Wi-Fi 使用，Direct BLE 继续使用原 DeviceAccessStore。
- 板卡的原 app0、分区表、NVS、构建和烧录日志保存在私有目录 `.build/wifiduino-s3-own-cloud-20261111.h9TyKcXb/`。服务端 PostgreSQL 和 Broker 备份保存在 `aily-blinker-services/data/backups/2026-11-11-wifiduino-canonical-name.U49MQSBW/`。

## Edges

- 写入仅覆盖 app0。烧录前后整段 NVS 逐字节一致。网关仍保有原 `ee` 的 BLE 访问根。
- 只调整原自有设备的登记名以及板子的云端凭据来源。沿用原逻辑设备 ID、设备密钥版本 3 和页面布局。两条设备的历史数据保留。
- 固件和私有构建文件含本地设备凭据，存于 Git 忽略目录。现场验证覆盖串口诊断和 Broker 新状态。

## Answer

1. Gateway 与 Broker 的登记名都从 `test` 改成 `WIFIduino32 S3`。
2. 新共存固件仅写入原 app0，串口诊断为 `state=active network=1 local=1 cloud=1`。
3. 分片 1 确认原物理板卡以自有设备凭据建立新会话。17:51:35 到 17:51:40，状态版本从 9 到 10，运行时长从 40 秒到 45 秒，Wi-Fi 信号为 -52 dBm。
4. 原页面布局版本 14，匹配当前九字段 Manifest，可继续使用原自有页面。
