# Device V2 发布就绪与三端交接

> 状态：2026-08-19。公开 WiFi/BLE facade、四个公开 Product 包和 R-ARCH-6 发布结构收口已完成；
> internal WiFiProv/WiFi+BLE/LocalBLE 仍不是普通用户产品。当前正式进入 App/服务器生产接入与真机门禁，
> 不把 reference golden 或 Arduino compile pass 写成产品已上线。

> 2026-08-18 鉴权修订：P6 不再等待 OwnershipBinding。设备已完成 176 B atomic DeviceAccessStore、
> accessEpoch、Admin/Shared 两槽、Security1 首次 Bootstrap 与 ControllerControl endpoint 接线。仍未完成的是
> App/Service production、Plain MQTT frame MAC/replay、当前 P6 完整 link/resource 重测和真机。

> 2026-08-19 手动模式修订：短期 EnrollmentKey/RecoveryKey 双码 UX 已撤回，统一为长期可查看/刷新的
> 无前缀 DeviceKey；RecoveryGrant 只处理实例恢复事务。设备 challenge/session codec 已冻结，App/Gateway
> 生产实现与刷新强吊销 E2E 尚未完成。

> 2026-08-19 数据面修订：Broker 已完成 HELLO/Manifest 协商、State snapshot staging/latest-state 和
> Report Patch 合并；Gateway 已提供 owner 范围的 Manifest/State 读取。只有完整 snapshot 才设置
> `stateFresh/protocolReady`。App PageSchema、控制下行与真机 E2E 仍是发布阻断项。

> 2026-08-28 ESP32 BLE 交接修订：BLE v1 bearer packet 固定为 20 B，App/设备不能根据平台 MTU
> 单方面改变分片容量。XIAO 当前产品已统一到 ESP-IDF NimBLE 普通 GATT bearer，不主动触发 SMP、配对或
> bonding；Noise、Method 2 与 DirectSecureRecord 是身份和应用数据安全根。Android 真机完整认领与控制闭环
> 已通过且没有系统 PIN。平台原生链路加密只可作为后续显式纵深配置。

> 2026-08-23 Wio 压力修订：设备已修复 StateRequest 前置 Patch 与非 canonical 注册顺序的 keyed
> Snapshot/Patch/Page，真实三字段 Broker 链路保持 Cloud active；RateLimited 后 Cloud telemetry 已观测到
> 对齐的 1108 ms 暂停并恢复。服务端仍有两个发布阻断项：TelemetryData/Status 不能继续默认归入
> `control` lane；State revision gap 触发 snapshot resync 时，不能因 `protocolReady=false` 把已建立租约的
> TelemetryData 当作 severe malformed。详见 `DEVICE_V2_DIAGNOSTICS_OUTBOUND_TELEMETRY_PLAN.md` 的 S1 修正项。

设备统一采用 Property/Action/Event Manifest，页面由 App/服务器的 PageSchema 与 WidgetRegistry 管理。
旧 Enrollment/1、永久 authKey/setup key、设备 WidgetCatalog 均不是 V2 兼容入口。

## 已冻结并可作为 R5 输入的范围

- BBP/2 Frame、restricted CBOR、HELLO、Manifest/State 分页、可靠交付和 BLE 4 B 分片；
- WiFi-only DeviceKey session、arena 与掉电框架；旧 EnrollmentKey outer 已删除；
- EnrollmentTicket、Noise/LocalSetup application 与 1..8 durable saga；
- Ownership Claim、Cloud Credential Enrollment、Controller Method 2 日常鉴权、Ownership Release；
- Controller 变更 v2 Core/BBP Runtime、P6 accessEpoch 与首次 Admin bootstrap 已冻结并接线；生产 App/Service
  codec/事务与资源/真机尚未完成，不能标记 production-ready；
- `device-key-session/1` 完成 challenge-HMAC/短期 SessionCredential；一个 MQTT PUBLISH 对应一个 BBP/2 frame；
- Platform BLE 与 LocalBLE 的身份、持久域和服务器边界。

DeviceKey 与 Recovery 的权威输入是 `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 和
`DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md`：无前缀 43 字符 DeviceKey 用于首次接入/全擦后回到同一逻辑设备；
N -> N+1 保留 logical ID/PageSchema/history，并立即吊销旧 Key/session/连接。RecoveryGrant 不替代日常 DeviceKey。

权威交接入口：

- 线规范：`BBP2_NORMATIVE_WIRE_SPEC.md`；
- App/服务器共同主线：`APP_SERVER_BBP2_REFACTOR_INPUT.md`；
- 身份/配网：`DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md`；
- 各模式上线/协议总览：`DEVICE_V2_MODES_ONLINE_AND_PROTOCOL_OVERVIEW.md`；
- 设备 Key：`DEVICE_V2_DEVICE_KEY_CONTRACT.md`；
- 当前设备进度：`DEVICE_V2_REFACTOR_PROGRESS.md`；
- App 执行：`D:\codes\blinker-app\docs\DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md`；
- 服务器执行：`D:\codes\aily-blinker-services\docs\DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md`；
- exact/invalid 输入：`tests/golden/`。

Cloud 首版不是 raw-secret Activation：设备向 pinned Gateway X25519 key 发送 AES-GCM sealed secret，
Gateway 返回 signed Grant，设备返回 HMAC Receipt，Gateway 再返回 HMAC CommitAck。App 仅 relay exact
bytes；Gateway 用 KMS/HSM 或等价受控域保护 X25519 private key 与 Active HMAC key。

## 当前仍未完成

- App production restricted-CBOR/BBP2/Noise/BLE coordinator、安全存储和 PageSchema reducer；
- Gateway PageSchema schema/CRUD、KMS key ring、Cloud onboarding 与生产 step-up；
- Broker/Gateway 的控制下行、历史、App stable-key 实时 stream 与跨 Shard PageSchema 协调；Manifest/State
  读取基线已经实现，不应再列为“尚未接线”；
- Broker `PublishLane` 缺少独立 telemetry lane；State snapshot 重同步期间必须保持既有 telemetry readiness，
  或有界丢弃样本，不能以 malformed 安全事件断开设备；修复后重跑真实 Wio RateLimited 持续在线门禁；
- fake-device 三端故障 E2E、ESP32 WiFi+BLE/WiFiProv 真机门禁及 ESP32/UNO R4 长稳；普通 WiFi/BLE
  基础真机链已经通过，不再重复列为完全缺失；
- ManualDeviceKey parser/version/HMAC outer、App/Gateway KMS reveal/rotate、Recovery codec/coordinator/journal、
  shared golden、在线旧实例确认、N -> N+1 cutover 与旧 session 撤销；
- R4b-7 面向普通用户的统一 `<Blinker.h>` facade/package。

发布必须同时满足平台构建、目标真机资源/长稳、真实身份与 broker E2E、生产三端全部 golden/invalid、
PageSchema 首屏和 Legacy/V2 隔离。任何单独的 host pass、平台 compile pass 或 reference codec pass 都不能
替代整体验收。
