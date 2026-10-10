# Device V2 DeviceKey → WiFi → MQTT 稳定化主线

> 状态：2026-08-26 设备侧功能链软件门禁、协议/composition 漂移门禁、真实 ESP32-S3、UNO R4 与 Wio Terminal
> WiFi 本机三端联调已完成；ESP32 WiFiProv、Wio BLE、真实 Aily 双账号与生产
> WSS 尚未联调。本文只定义
> 当前设备主线和验收边界。Cloud Frame Authentication 已移入后续安全批次，不再阻塞本批功能互操作，但仍是
> Plain MQTT 进入生产安全档的前置条件。

## 1. 当前结论

设备侧已经具备一条不依赖旧 JSON/authKey/CloudCredential 的完整内部链路：

```text
Sketch 提供 DeviceKey、并持久化 WiFi profile；或 WiFiProv 原子安装 DeviceKey + WiFi profile
  -> WiFi association + candidate confirmation
  -> DeviceKey locator/challenge/HMAC proof
  -> 短期 SessionCredential
  -> MQTT CONNECT
  -> 精确 subscribeTopic SUBSCRIBE
  -> 一个 MQTT payload = 一个完整 BBP/2 frame
  -> HELLO、状态上报、命令和事件交互
```

Host 模拟 E2E 已使用真实 `HttpDeviceKeySessionProvider`、`ManagedMqttTransport` 和
`MqttFrameTransport` 串行证明：网络 gate 关闭时不发起 HTTP/MQTT；challenge/session 成功后才应用
broker host/port/security/clientId/username/password；CONNECT 与 SUBSCRIBE 成功后才报告 Online；BBP/2
frame 可以按服务器签发的上下行 topic 完整发布和接收。

本机 `aily-blinker-services` 的真实 PostgreSQL DeviceKey challenge/session route、Gateway、Broker 与中央
MQTT-over-WebSocket 已和 ESP32-S3 联调。当前可以确认真实 AP、HTTP、MQTT、BBP/2、App typed client 和短期凭据
刷新闭环；这仍不等于线上已可用，因为真实 Aily 登录/分享、WiFiProv、生产 WSS、DeviceKey rotate、其他 MCU
真机和 24 h 长稳尚未完成。

## 2. 两种 Key/网络入口，共用同一登录下半链

### 2.1 手动 WiFi

用户从 App 取得长期可重新查看的 43 字符无前缀 DeviceKey，在 Sketch 中提供 SSID/password/DeviceKey。
`ConfiguredDeviceKeySource` 提供 Key，网络配置进入 `WifiCredentialStore`，ESP32 与 UNO R4 共用同一个
`DeviceKeyWifiStack`。该模式不把 MQTT password 写进 Sketch，也不持久化短期 SessionCredential。

### 2.2 ESP32 WiFiProv

App 通过 Security1 会话下发 WiFi 参数，并通过 `blinker-config` endpoint 执行 `GetInfo` 后安装 DeviceKey；
P6 首次引导使用 `Bootstrap` 原子安装 DeviceKey、accessEpoch 和 Admin direct credential。Adapter 只有在
Key 已安装、WiFi candidate 已写入且平台报告联网成功后才完成 provisioning，随后 deinit manager 并启动
完全相同的 DeviceKey cloud stack。

EnrollmentTicket 只授权本次 App/Service 配网事务，不写入设备，也不参与以后重启。设备重启只读取长期
DeviceKey 和 WiFi profile，再申请新的短期 SessionCredential。

## 3. 设备对象与职责

| 对象 | 唯一职责 | 持久化 |
|---|---|---:|
| `DeviceKeyStore` / `DeviceAccessStore` | 保存长期 DeviceKey；P6 另保存 Direct access 域 | 是 |
| `WifiCredentialStore` | 双槽保存 WiFi candidate/confirmed profile | 是 |
| `WifiConnectionLifecycle` | AP 连接、确认、回退和重连 | 否 |
| `HttpDeviceKeySessionProvider` | challenge、HMAC proof、短期凭据刷新/失效 | 否 |
| `ArduinoClientHttpAdapter` | 有界 HTTP/1.1 request/response 和请求超时 | 否 |
| `ManagedMqttTransport` | 凭据代次变化时重配 MQTT | 否 |
| `PubSubClientAdapter` | 把注入的 PubSubClient 转成 `IMqttClient` | PubSubClient 自有 buffer |
| `MqttFrameTransport` | CONNECT/SUBSCRIBE、topic gate、BBP/2 frame 收发 | 否 |
| `Client/DeviceRuntime` | HELLO、Manifest/State/Command/Event 业务协议 | 有界 RAM |

HTTP request/response/credential arena 由 composition 固定拥有；MQTT 凭据只在 provider arena 和
PubSubClient adapter 的有界工作区中存在。日志不得输出 DeviceKey、proof、MQTT password 或完整请求体。

## 4. 本批新增稳定性门禁

1. `test_http_device_key_session_main.cpp` 新增同进程功能 E2E，覆盖 DeviceKey challenge/session、
   MQTT CONNECT/SUBSCRIBE、精确 topic、BBP/2 publish/receive 和 stop/disconnect。
2. `ArduinoClientHttpAdapter` 现在必须注入 `IClock` 并执行 `HttpRequest.timeoutMillis`；此前服务器保持连接但
   不返回数据时可能永久停在 `InProgress`。超时会关闭 Client 并返回可重试的 `NotConnected`。
3. HTTP adapter 新增独立 Host 回归，锁定 `start()` 不联网、正常 Content-Length response 和超时关闭。
4. `DeviceKeyProvisioningEndpoint` 的 32 位资源门槛统一为 `<=16 B`，不再保留旧的 `==12 B` 断言。
5. Host 严格编译/测试、architecture gate 和 official aggregate package gate 必须同时通过；Arduino clean
   compile 只能证明真实工具链链接，不替代真机。
6. `tests/freeze/device_key_cloud_login.sha256` 冻结 DeviceKey contract/provider/session arena、MQTT transport、
   official DeviceKey WiFi composition 与 golden vector；`check_device_v2.py` 在 architecture gate 开始时校验
   文件存在性、重复项和 SHA-256 漂移。任何有意修改必须同时更新三端 codec/golden、三平台编译资源证据和清单。
7. 数字 IPv4 gateway 直接使用平台 `IPAddress` connect；没有该重载的 Arduino Client 在编译期回退到 host
   connect，避免把平台差异暴露到用户 API。
8. `408/425/429/5xx` 即使返回非规范错误 body 也只进入退避；短期 MQTT 凭据到期不等于 DeviceKey 被撤销，
   只有服务器明确的 Key/credential revoke 才进入终止态。
9. 受控 DeviceKey HTTP 端点必须返回 `Content-Length`；设备端有意不实现 chunked decoder，以控制 Flash/RAM。

本批当前源码的可复现结果：

- MSVC 严格 Host 全量构建与 CTest：`41/41` 通过，另有 `200,000` 次 deterministic fuzz；
- ESP32 DeviceKey `CompileDeviceKeySession` clean link：Flash `599,063 B`，global RAM `28,768 B`；
- UNO R4 WiFi DeviceKey `CompileDeviceKeySession` clean link：Flash `88,908 B`，global RAM `11,232 B`；
- UNO R4 WiFi `SizeProbe` clean link：Flash `52,452 B`，global RAM `16,632 B`；
- architecture gate 与 official aggregate package gate 通过。
- ESP32-S3 `LiveDeviceSmoke`（`power` + `uptimeSeconds`、公开 WiFi API）真机链接：Flash `985,331 B`，
  global RAM `50,020 B`；该数字包含完整公开组件/Runtime/DeviceKey HTTP/PubSubClient/MQTT 链，不应与只探测
  session composition 的 `CompileDeviceKeySession` 数字混用；
- 2026-08-26 XIAO ESP32-S3 公开 WiFi Product 复测为 `974,927 B` Flash、`49,768 B` global RAM；
  DeviceKey challenge/session、MQTT CONNECT/SUBSCRIBE、Manifest/State、Property Command/ACK 通过。
  主动 `WiFi.disconnect()` 后约 6 秒恢复 Active，重连后的命令再次 ACK；测试地址和凭据只存在临时探针，
  未写入正式库；
- 真机完成 Manifest/State、App Command/ACK/Patch、Broker/Gateway 重连，并在本机 `60/30 s` 加速配置下连续
  完成两次 session/MQTT 凭据刷新；测试后服务恢复默认 `600/480 s`。
- Wio Terminal 公开 WiFi Product clean link 为 `138,616 B` Flash；真机完成 Gateway 未启动时保持 loop、
  503 退避、DeviceKey session、MQTT 登录、会话刷新、Gateway 停止后降级以及跨 expiry 无复位恢复。
- UNO R4 WiFi 公开 Product 真机探针为 `139,448 B` Flash、`12,988 B` global RAM；完成真实 AP 关联、
  DeviceKey challenge/session、Plain MQTT CONNECT/SUBSCRIBE、Manifest/State 和 Property Command/ACK。
  主动调用 `WiFi.disconnect()` 后，设备自动恢复网络并重新申请 session；Broker 接受新连接后再次下发
  `power` 写入并收到 ACK，State revision 前进。该门禁使用临时本机配置注入，没有把 SSID、密码、
  DeviceKey 或 LAN 地址写入正式库。

Arduino 日志仍有本机无效版本目录以及 ArduinoCore-renesas、WiFiS3、PubSubClient 上游告警；这些不是本库
本批源码产生的告警，但发布 CI 应使用干净的固定版本环境消除环境噪声。

`CompileDeviceKeySession` 已从旧 `CloudCredential + HttpCloudSessionProvider` 探针改为真实的
`ConfiguredDeviceKeySource + HttpDeviceKeySessionProvider + DeviceKeySessionArena`。旧探针产生的资源数字不得再
作为 STAB-1 证据。

该超时能约束 `connect()` 返回后的整个 HTTP 事务，但无法抢占 Arduino `Client.connect()` 自身的同步 DNS/TCP/TLS
阻塞。Wio 的 rpcWiFi 真机已证明：上游 RTL8720 RPC connect 可在无监听/黑洞端口中阻塞，SAMD 侧 timeout 与
non-blocking flag 不能中断。因此部署合同必须让 HTTP/MQTT 稳定入口始终 accept，并在 Gateway 重启时快速返回
定长 503 或关闭 MQTT 连接；库不复制 Seeed 网络栈，也不引入平台专属后台任务。PubSubClient CONNECT 同样是
合作式状态机外的一次同步调用，不把 ESP32 专用 API 泄漏进协议层。

## 5. 当前明确不实施的 Cloud Frame Authentication

本批不修改 MQTT payload，不增加 tag/counter/key，不改变 DeviceKey session v1。Plain MQTT 当前只提供
功能互操作和 broker credential 鉴权，不能抵抗同网段窃听、篡改或 retained/replay。TLS 仍是显式可选 profile，
不得运行时静默降级。

后续安全批按 `DEVICE_V2_AUTHORIZATION_AND_SHARING_DECISION.md` 冻结三端 exact contract 后再实施，至少包含：

- session response proof 与域分离 `cloudFrameKey`；
- 方向绑定的 32 位 counter、截断 tag 和 replay window；
- reconnect/counter rollover、retained message 与 Key rotate 行为；
- PubSubClient packet capacity 增量和三端 golden/fault E2E。

该批仍是 **Plain production profile** 的安全发布门禁，但不是当前“设备能否拿凭据并登录 broker、交换
BBP/2”的功能门禁。

## 6. Gateway/Broker 联调阶段

第一阶段最小环境已落地到
`D:\codes\aily-blinker-services\tools\device-v2-interop`。它只使用 Node.js 内置模块，严格复用设备 golden，
并实际完成 exact CBOR challenge/session、MQTT CONNECT/SUBSCRIBE、精确 topic ACL 与 BBP/2 上下行。它用于
在生产 KMS/数据库/route 完成前先验证 ESP32/UNO R4 的真实 WiFi、HTTP 和 MQTT 行为，不承担账户、多租户、
持久化、Key rotate、主动断线或生产安全职责。

服务器生产实施拆分和交接清单见
`D:\codes\aily-blinker-services\docs\DEVICE_V2_GATEWAY_BROKER_INTEROP.md`。最小环境通过后，设备主线不再继续
修改登录合同或 composition；只允许修复真机发现的缺陷，并按冻结清单完成显式合同复审。

设备仓统一使用 `tools/build_arduino_release.py --profile <development-profile.json> --allow-development-profile --stage-dir <empty-dir>`；profile填写实际LAN host/port及公开验签材料，生成指向该环境的
单一标准 Arduino 库。ESP32、UNO R4 和 Wio Terminal 使用同一份 stage/ZIP，不再生成 platform-specific
CompositionProbe 包；用户编辑公开 `CloudSmokeTest` 的 DeviceKey、SSID/password 后编译目标 board。
公开入口已经收敛为 `<BlinkerWiFi.h>` / `<BlinkerBLE.h>`、`Blinker.begin(...)` 与 `Blinker.run()`；
platform product、lifecycle 和 composition 均为库内部实现。

## 7. 下一阶段真实联调验收

先使用最小环境完成第 2～6、9～10 项的设备功能验证；生产 route 准备好后必须按以下顺序做一次完整可观测 E2E：

1. App/Service 创建 logical device、当前 DeviceKey 和 credentialVersion；
2. 手动 Sketch 或 ESP32 WiFiProv 安装 Key/WiFi，掉电重启后仍可读取；
3. Gateway 返回 challenge，服务验证 exact HMAC proof，并签发短期 MQTT credential/topic；
4. Broker 验证 CONNECT 身份与 topic ACL，设备成功 SUBSCRIBE；
5. Gateway 收到设备 BBP/2 HELLO，完成 Manifest/State staging；
6. App 下发一个 Action/Property 写入，设备回 ACK/状态；设备主动 report/event 到 App；
7. Session 到 refresh 时间平滑换证；到 expiry 时旧连接不能继续工作；
8. App 刷新 DeviceKey 后 credentialVersion 前进，旧 Key、旧 session 和旧 broker 连接全部失效；
9. AP 断开、Gateway 5xx、MQTT CONNACK 拒绝、response loss 和掉电均按合同重试且不泄漏 secret；
10. ESP32 P3/P6 与 UNO R4 手动模式分别记录 Flash/global RAM、heap/stack watermark 和 24 h 长稳。

截至 2026-08-26，本机受控 owner 身份已完成第 1、3～6、9 项，以及第 7 项的正常 refresh、Gateway 停机跨
expiry 后恢复；第 2 项已覆盖 ESP32/UNO R4/Wio Sketch 手动模式但非 WiFiProv，第 8 项 Key rotate 与第 10 项长稳仍未完成。
UNO R4 最近一次 Command/ACK 使用 Broker 内部受控接口验证 exact BBP/2 设备链；当前 Android 测试账号因达到
设备数量上限，未重复创建 owner device，不把该限制误记为设备或协议失败。上述剩余项通过前
不切换公开 `<Blinker.h>` Education facade，也不称为生产发布完成。
