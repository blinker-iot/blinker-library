# Device V2 ESP32 WiFiProv 优先配网与跨平台安全实施计划

> 状态：2026-09-02 G1B 真机收口批。P6 已从 `tests/internal` 迁入标准 Arduino `src`，增加
> ESP32-only `<BlinkerWiFiBLE.h>` 和同一 ZIP 内的 QuickStart；源码/安装后单 ZIP clean link、ELF/Map 闭包与
> Host ASan 已通过。配网 scheme 由产品入口选择的 `Esp32WiFiProv.ipp` 内联实例化：WiFi+BLE 产品默认 BLE
> WiFiProv Security1，可定义 `BLINKER_ESP32_PROVISIONING_SOFTAP` 选择 SoftAP 回退，
> 公开 `begin()` 不变。不得把 scheme factory 放回公共 `.cpp`，否则 Arduino 依赖扫描会污染纯 BLE 产品。
> XIAO ESP32S3 已完成 Android BLE WiFiProv、同一设备 network-only reset、无断电 WiFiProv→NimBLE Direct、
> WiFi 自动重连、正式 App 重新配网入口、原身份复用、Method 2/Manifest/State/Power 真机门禁。重新配网入口
> 依据 owner + active + cloudEnabled，不错误依赖首次 Cloud lastSeen；它只替换 Wi-Fi，不轮换 DeviceKey、
> PresenceKey 或 Direct access。最新隔离 ZIP probe 为 1,279,291 B Flash / 53,988 B globals。开放 Gateway 宿主
> `17000/18883` 后，DeviceKey challenge/session/MQTT、Cloud+Direct 同时在线、BLE-first、Cloud fallback 与
> adapter enabled 后 Direct 自动抢回均已真机通过；Cloud/Direct 双命令后的 steady/min heap 为
> `159,560/135,076 B`、largest block `116,724 B`、loop stack watermark `4,296 B`。内部门禁已枚举 13 个
> FreeRTOS task 并连续完成两轮 Provisioning→Cloud→Direct；`ipc0` 最低 76 B 且未继续下降，但属于
> Arduino-ESP32 SDK 内部窄余量，系统任务发布门禁仍不能判定通过。SoftAP、100 次循环、RF 与长稳仍待。
> DeviceKey、会话和强制轮换以 `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 为准。
> 2026-08-18 鉴权接线后，P6 已新增 DeviceAccessStore、P-256 ControllerControl 与首次 Admin Bootstrap；
> 当前 store 为 196 B schema v2（兼容读 176 B v1）。下文 30.02%/25.58% 是接线前历史基线，当前正式单 ZIP Plain 静态准入为 28.48%，TLS 仍待。

## 1. 决策

1. ESP32 的第一条 App 配网联调实现固定为 Espressif Unified Provisioning / WiFiProv Security1，并作为
   WiFi+BLE 唯一业务 composition 进入 `src`；BLE 是当前默认 bearer，SoftAP 是编译期回退。
   未通过真机/App/Service 门禁前仍是实验产品，不替换线上 `dev_edu`。
2. 不把 `WiFiProvClass`、ESP-IDF 类型、Protocomm 或 Protobuf 暴露到 `<Blinker.h>`、Core 或公共 wire。
3. portable Noise 只保留 BLE-only/LocalBLE 和 Enhanced 回归，不再作为 ESP32 WiFi+BLE 的第二配网图。
4. Security2 与 portable Noise 下沉为增强候选，不通过独立用户安装包或 profile 发布。
5. UNO R4 不移植 ESP-IDF WiFiProv，产品只提供独立手动 WiFi 与独立普通 BLE，不做 BLE 配网或 WiFi+BLE；
   Wio Terminal 同样只提供独立 WiFi 与独立普通 BLE，不宣称动态配网或 WiFi+BLE。
6. 同一安装库使用 `<BlinkerWiFi.h>`、`<BlinkerBLE.h>` 或 ESP32-only `<BlinkerWiFiBLE.h>` 选择 Product；
   `Blinker.begin(...)` 不传模式参数，WiFi+BLE 也不接收底层 WiFiProv 参数，不新增 ZIP。

## 2. 当前代码事实

| 平台 | 已有代码 | 已验证 | 仍缺失 |
|---|---|---|---|
| ESP32 | WiFi station/NVS、DeviceKey manual WiFi、v2 provisioning endpoint、BLE/SoftAP Security1 scheme、P6 配网 + NimBLE Direct 唯一业务图 | XIAO 默认 BLE WiFiProv、DeviceInstanceId 核验、network-only reset、`WiFiProv deinit -> Direct`、WiFi 重连、DeviceKey/MQTT、Cloud+Direct 路由、13-task 水位可观测及两轮真机循环已过；单 ZIP/Host/architecture 通过 | SoftAP App、100 次循环、`ipc0=76 B` SDK 窄余量处置、RF 与长稳 |
| UNO R4 WiFi / BLE | WiFiS3 + DeviceKey 普通 WiFi；ArduinoBLE + 逐连接 encryption fact + Platform BLE 普通产品；portable Noise 仅作 Enhanced 证据 | 双产品 package clean compile；WiFi/BLE 依赖隔离；两条产品真机 App/Service 基础链与 BLE 持久重连已过 | 长稳/恶意断连；明确不做 WiFi+BLE |
| Wio Terminal | rpcWiFi/rpcBLE、storage 与独立 WiFi/BLE 产品；不提供 WiFi+BLE | WiFi 真机门禁完成；BLE Noise/Method 2/DirectSecure、Manifest/State、Power/Button、同版重刷、真实断电、认证 deadline 及 32 次重连完成 | BLE 保持 opt-in preview；只待 24 h authenticated DirectSecure 长稳，不依赖闭源 RTL bond 修复 |

因此“UNO R4 已实现”只能精确表述为：普通 WiFi 与普通 BLE 两个独立产品已完成基础三端真机链，但仍有各自
发布长稳门禁；它不包含 App 通过 BLE 给 WiFi 配网。Wio 同样只有独立 WiFi/BLE 产品，不提供组合配网图。

## 3. 各平台默认安全方式

### 3.1 ESP32

- 首发动态配网协议：WiFiProv + Security1。WiFi+BLE 产品默认 BLE transport；需要 SoftAP 回退时在
  `<BlinkerWiFiBLE.h>` 前定义 `BLINKER_ESP32_PROVISIONING_SOFTAP`。
- 这是编译期平台能力选择，不是用户 `begin()` 参数，也不产生第二个库/ZIP。替换真机通过 100 次
  `Provisioning -> deinit -> Direct BLE` 循环后，才评估对 IDF-NimBLE 目标自动默认 BLE；经典 ESP32 保持 SoftAP。
- 已预置/已保存 DeviceKey 的重新配网必须核对 DeviceInstanceId 和服务器 logical-device 记录；当前
  Education 路径仍是无 PoP Security1 物理窗口，不得把“设备有 Key”误写成 Security1 已认证设备。
- 全新 DIY 设备若没有 factory secret、QR、显示屏短码或预置 Key，任何协议都无法凭空证明硬件真实性。
  Education profile 只能依赖物理按键、短配网窗口和 App 明示低保证；无唯一 PoP 的 Security1 只能防被动监听，
  不能宣称抗主动 MITM。
- Security2 只用于明确接受额外 Flash/heap、SRP verifier 管理和 App 兼容成本的 Enhanced profile。
- 云端 Plain/TLS 独立选择；无论是否 TLS，都必须使用 DeviceKey challenge-HMAC，raw Key 不上网。

### 3.2 UNO R4 WiFi

当前只保留两个互不组合的发布候选：

1. 普通 WiFi：手动 `DeviceKey + SSID/password`，WiFiS3 负责 WPA2 链路；云端默认 Plain
   challenge-HMAC，用户可选 `WiFiSSLClient` TLS。
2. 普通 BLE：Platform BLE facade，使用 ArduinoBLE 加密 characteristic、当前连接加密事实和 Controller
   Method 2；不接收 WiFi credential，也不安装云 DeviceKey。

现有
`RenesasUnoBleSecuritySource` 会从当前 ATT connection handle 检查 encryption-complete，并在加密丢失后断开，
这部分可复用；但它不提供 per-connection bonded 事实，也未经过真机验证。Noise 保留为 Enhanced profile，
不能把历史 Noise/WiFi+BLE 图改名为普通 BLE。UNO R4 不再新增轻量 setup driver。

### 3.3 Wio Terminal

首个可信发布目标是手动 WiFi + DeviceKey challenge；该链已用 rpcWiFi 在真机完成 MQTT/BBP2、Manifest/State
和 Property/Action 门禁。rpcBLE 独立产品已完成应用层 Noise 认领、Method 2、DirectSecure、同版重刷及真实断电
重连；因闭源 RTL bond 无法可靠恢复 Android RPA，原生 bearer 保持未配对，业务安全完全落在应用层合同。
它仍是 opt-in preview，且不构成 BLE 配 WiFi。后续如需动态配网，优先评估 SoftAP/captive portal 或
Connectivity Agent，而不是照搬 ESP-IDF WiFiProv 到 SAMD51。

### 3.4 外置 BLE/WiFi 模块

长期采用 NCP/Connectivity Agent：模块负责 DeviceKey storage、配网、TLS、MQTT、BLE 和 OTA；主 MCU 只跑
Property/Action/Event 与有界 BBP/2/串口 framing。串口 CRC 只处理传输损坏，不等于密钥保护；如果 PCB/UART
可能被攻击，模块与主 MCU 还需要工厂配对密钥或受保护链路。

## 4. ESP32 WiFiProv 适配边界

### 4.1 使用底层 manager，不泄漏平台 API

内部新增不暴露到公共 API 的 ESP32-only adapter，使用 `network_prov_mgr`：

- 标准 WiFiProv endpoint：扫描、SSID/password、apply/status；
- 一个 Blinker custom endpoint：`blinker-config`，支持固定长度的 `GetInfo`、`Install` 与 `Bootstrap`；
- 生命周期：init -> create endpoint -> start -> register handler -> transact -> stop/deinit；
- Core 只看 32 位 ABI 不超过 16 B 的 `DeviceKeyProvisioningEndpoint`、`IDeviceKeyStore`，以及组合产品可选的
  `IDeviceAccessStore`；不 include WiFiProv、Protocomm 或
  protobuf header；
- Arduino 高层 `WiFiProvClass` 可用于最小 probe，但正式 adapter 使用底层 API 表达 custom endpoint 和资源清理。

### 4.2 首版流程

```text
用户让 ESP32 进入短时 provisioning window
  -> App 使用 Education 物理窗口的 Security1 会话连接设备
  -> blinker-config.GetInfo 读取 deviceInstanceId、hasDeviceKey、hasAccessState、accessEpoch
  -> 首次/全擦设备：App 创建或显示用户选择的 DeviceKey；P3 使用 Install，P6 使用 Bootstrap
     原子保存 DeviceKey + accessEpoch + Admin
  -> 原设备 network reset：App 用 deviceInstanceId 解析服务器 logical device，并精确核对
     logicalDeviceId + credentialVersion + locator；hasAccessState 时再核对本机 Direct credential
  -> 原设备已有 DeviceKey 时不调用 Install/Bootstrap、不刷新 Key，也不创建第二个设备
  -> WiFiProv 标准 endpoint 再写 WiFi candidate；Blinker journal 是唯一持久来源
  -> Device 用当前 DeviceKey challenge-HMAC
  -> Device 取得 SessionCredential，连接 MQTT/BBP2
  -> Gateway/App 状态显示 Cloud Ready
  -> Device 停止 WiFiProv；需要本地控制时启动 Direct BLE BBP/2
```

“重新配网”“刷新 Key”“resetAccess”是三个不同操作。重新配网只替换 WiFi；刷新 Key 才轮换 DeviceKey 并使旧
cloud session/credential 失效；resetAccess 擦除本地 access root。App 不 relay CloudCredential enrollment；
服务器只参与 DeviceKey 管理、challenge/session、实例解析和状态查询。设备 endpoint wire 固定为 v2：2 B GetInfo request、
24 B GetInfo response、45 B Install request、101 B Bootstrap request 与 3 B status response。Bootstrap 只用于
P6 首次建立 access state；显式 Key refresh 后的安装是独立事务，不得借重新配网隐式触发；
不复用旧 8-operation Noise saga、CBOR、JSON 或 protobuf body。

## 5. 保留现有 BLE 实现的方式

现有 `BleNoiseProvisioningChannel`、`LocalSetupApplication/Saga`、ESP32 NimBLE 与 UNO R4 ArduinoBLE graph 暂不删除：

- LocalBLE/Direct BLE 的 controller、mode/fragment、状态同步仍可复用；
- portable Noise 作为 Enhanced 与非 ESP32 平台实验参考；
- WiFiProv 首版不得把旧 8-operation saga 整体套入 custom endpoint；只实现最小 WiFi + DeviceKey，Ticket/intent
  留在 App/Service；
- 等 ESP32 E2E 完成后，逐项判断旧 Cloud/Network saga 哪些仍被 Factory/Enhanced 消费；设备保存 Ownership 的
  路径已经否决；其余无消费者的
  未发布代码再从 release manifest、测试和源码中删除。

## 6. 资源与共存 spike

冻结为默认前必须分别 clean build：

| Probe | 必测组合 |
|---|---|
| P1 | WiFiProv Security1 BLE only |
| P2 | P1 + 一个 custom endpoint |
| P3 | P2 + DeviceKey challenge + PubSubClient/BBP2，Plain |
| P4（历史） | P3 + Direct BLE；已确认单 Bluedroid 可接线但资源不准入，可执行图已删除 |
| P5 | P3/P6 + TLS；分别决定 WiFi-only 与 WiFi+BLE TLS 准入 |
| P6-S | WiFiProv SoftAP Security1 + NimBLE Direct BLE；编译期回退图 |
| P6-B | WiFiProv BLE Security1 + NimBLE Direct BLE；当前默认，只允许 Arduino Core 已使用 IDF-NimBLE 的目标 |

每个 probe 记录 Flash、静态 RAM、最小 free heap、largest block、任务 stack watermark、配网结束后的 heap 回收、
双 OTA app slot 余量和 100 次 Provisioning -> Cloud -> Direct BLE 循环。当前 P2+DeviceKeyStore+
WifiCredentialStore 的 Security1 clean compile 为 `1,689,811 B Flash / 58,268 B global RAM`：默认
`1,310,720 B` app slot 超限；ESP32 `min_spiffs` 双 OTA 的 `1,966,080 B` slot 可装入，余量 `276,269 B`
（约 14.1%）。清理后的 P3 Plain 为 `1,758,907 B Flash / 62,964 B global RAM`，同一 slot
只剩 `207,173 B`（约 10.54%）。历史 P4 使用与 WiFiProv 相同的 Arduino-ESP32 Bluedroid Host，配网阶段不调用
FREE_BLE，manager deinit 后才启动 Direct BLE；clean build 为 `1,822,043 B Flash / 65,776 B global RAM`，
只余 `144,037 B`（约 7.33%）。它证明单 Host 接线成立但未通过门槛；其未发布 wrapper/profile/probe 已删除。

P6 把公共 manager/DeviceKey transaction 与 transport scheme 解耦：编译期选择宏只包含
`Esp32WiFiProv.ipp` 中对应的 SoftAP 或 BLE SDK 分支，并在产品 Sketch translation unit 内联生成实现。
这不只是链接裁剪：它同时阻止 Arduino 的全库 `.cpp` 依赖扫描把另一种 scheme 或 WiFi 依赖带入纯 BLE。
P6-S SoftAP 回退图在配网完成并
deinit manager 后启动 NimBLE Direct；2026-08-23 正式安装 ZIP 为
`1,406,115 B Flash / 62,848 B global RAM`，双 OTA slot 余 `559,965 B`（约 28.48%）。此前
`1,395,187/62,848 B` 仅为历史快照。
ELF 中 `network_prov_scheme_softap`、NimBLE、DeviceKey session、PubSubClient、
BBP/2 和 Controller Method 2 存在；`network_prov_scheme_ble`、protocomm BLE、Bluedroid、旧
CloudCredential/Ownership/Noise product graph 均为 0。

P6-B 在 XIAO ESP32S3 上 clean build 为 `1,303,487 B Flash / 59,684 B global RAM`，ELF 只含 BLE scheme、
IDF-NimBLE 和 Direct BLE，不含 SoftAP 或 Bluedroid。经典 ESP32 Core 3.3.1 的同一选择会同时带入 Bluedroid
WiFiProv 与 NimBLE-Arduino，实编为 `1,899,723/73,052 B`，双 OTA slot 只余约 3.37%，因此由编译门禁拒绝。
2026-09-02 P6-B 已在 XIAO ESP32-S3 + Android 上完成 BLE Security1、custom GetInfo、WiFi 写入、manager
deinit、NimBLE Direct、active Direct→network reset→WiFiProv→Direct、Method 2、Manifest/State 与 Power
Command。最新隔离 ZIP probe 为 `1,279,291/53,988 B`；随后同一设备又完成 DeviceKey challenge/session/MQTT、
Cloud+Direct 同时在线、BLE-first、Cloud fallback 与 adapter enabled 后自动抢回。这替代“仅静态候选/开发板
损坏”的旧结论。冷启动并依次执行 Method 2、Cloud fallback Command、adapter 恢复与 Direct Command 后，
steady/min free heap 为 `159,560/135,076 B`、largest 8-bit block `116,724 B`、loop-task stack watermark
`4,296 B`，诊断无 operation/transport failure 或 drop。随后内部门禁枚举 13 个系统任务；两轮完整模式
切换及 Cloud/Direct 下发后 `ipc0` 稳定在 76 B，`ipc1` 最低 232 B、NimBLE 最低 1,652 B。该结果关闭
“不可观测”缺口，但 76 B 仍是发布阻断，不替代 100 次循环/RF 和长稳门禁。

随后把完整流程固化为 `tests/internal/esp32_wifi_ble_cycle_gate.cjs`。门禁按真实交互顺序先离开控制页、进入
重置引导，再执行 network-only reset，避免测试代码人为触发控制页 Direct 重连。真实 Android 暴露出路由页
离开后嵌套 V2 组件仍可能持有 5 s scan 的问题：修正为 Router exact-path 驱动的显式 `viewActive`，所有
`synchronize()` 统一受 foreground gate 约束，`disconnect()` 无条件委托 BLE service 取消该 logical device
的 active/opening session（service 无所有权时为 no-op）。WiFiProv handoff 也从 45 s 收敛为 15 s 上限。
全新 App 进程后的连续两轮实物结果均在设备页出现后约 4.27 s 完成 Direct，Property Command 和完整 13-task
探针通过；门禁默认以 15 s 为 Direct-ready 性能上限，禁止“最终能连上”掩盖数十秒退化。最小 free
heap/largest block 为 135,124/104,436 B，`ipc0=76 B` 结论不变。

P6 是 ESP32 WiFi+BLE 的唯一正式业务图，当前默认 bearer 为 P6-B，P6-S 是回退。P5 历史 TLS 结果为：P3
`1,845,907/63,556 B`，只余 `120,173 B`（6.11%），拒绝默认；
P6 `1,463,067/61,456 B`，余 `503,013 B`（25.58%）。当前 P6 必须重测后才能恢复静态准入。

SoftAP 的 `serviceKey` 是 AP WPA 密码，BLE bearer 忽略该字段；Security1 PoP 是 protocomm 会话证明，三者
都不是 DeviceKey。首发 EducationPhysicalWindow 明确使用无 PoP 的 Security1；默认 BLE 和未设置 serviceKey 的
SoftAP 回退都只通过临时 ECDH 会话降低被动窃听风险，
但不宣称设备真实性或抗主动 MITM。服务名由持久随机 deviceInstanceId 的前 6 B 生成，只用于发现，不作为主键。
Enhanced/Manufactured 才增加唯一 PoP/QR。P6 的 BLE App 路径和当前 composition 动态快照已实连，仍缺
SoftAP App、100 次循环、`ipc0` 窄余量、RF 与 24 h 真机证据，因此“源码可选”不等于“生产准入”。

任一条件失败即不准入：

- 同时链接两套 BLE host（经典 ESP32 的 BLE WiFiProv 已因此被拒绝）；
- 配网 stop/deinit 后资源不回收；
- `min_spiffs` 的 1,966,080 B dual-OTA app slot 少于 10% headroom；
- P6 的循环、系统任务水位或 WiFi/BLE RF 共存不稳定；
- custom endpoint 需要无界 protobuf/JSON/heap buffer；
- Security1 无 PoP却在 UI 标记为“已验证设备”。

## 7. 设备/App/服务器执行顺序

1. **WP-0 合同**：完成 DeviceKey DKEY-1；冻结最小 provisioning bundle 和状态机。
2. **WP-1 ESP32 probe**：P3 Plain 勉强通过、P3 TLS 拒绝；P6-S Plain 回退图已通过历史 clean link 和 ELF
   依赖审计；XIAO S3 默认 P6-B 的静态门禁、当前 composition heap/loop-stack 快照、13-task 观测及两轮真机
   自动化连续循环基础门禁均已通过，经典 ESP32 P6-B 已拒绝。TLS、100 次循环、`ipc0` 窄余量、RF 共存仍待。失败 P4 只保留
   文档证据，可执行图已删除。
3. **WP-2 device adapter（正式源码接线完成）**：ESP32-only manager wrapper、custom endpoint、candidate journal、
   stop/deinit 和错误门禁已实现；network-only reset、Direct 活跃切换、END/DEINIT 单一所有权及 composition
   heap/loop-stack 快照已过。
4. **WP-3 App coordinator（BLE 核心完成）**：Android 已接官方 Espressif provisioning library，完成无 PoP
   Security1 BLE、WiFi scan/config、custom bytes、DeviceInstanceId exact 核验和 network-only reset；iOS、
   SoftAP 与完整 UI 仍待。
5. **WP-4 Service**：实现 DeviceKey/Ticket/challenge/session/status；WiFiProv 本身不进入服务器。
6. **WP-5 E2E（核心链完成）**：同一已激活设备的重配网、Direct BLE 切换、WiFi 失败后自动重试与重烧恢复已过；
   宿主开放 `17000/18883` 后，DeviceKey challenge/session/MQTT 与 Cloud+Direct 同时在线也已通过。关闭蓝牙
   自动回退 Cloud，重新开启后由 adapter enabled 边沿触发一次有界 Direct 重连，禁止轮询。剩余首次全擦、
   错误密码、Key refresh 强踢、SoftAP、100 次循环、`ipc0` 窄余量、RF 和 24 h。
7. **WP-6 UNO R4（决策完成）**：只发布独立 WiFi 与独立 BLE；不补 ArduinoBLE setup driver，不做
   WiFi+BLE。继续完成 BLE 的 App/Service 与 32 KiB 真机门禁。
8. **WP-7 Wio/NCP**：有真实 adapter、资源与真机证据后再准入，不阻塞 ESP32 首发联调。

App/服务器可以在 WP-1 同期实现 DKEY 与 Ticket/status，但不得假设 probe 必然通过，也不得把 Espressif
protobuf 当成 Blinker 跨平台协议。

## 8. 当前设备库文件级改造清单

状态以代码消费者和编译门禁为准；不能因为类存在就把公开切换写成完成：

| 当前位置 | 改造 |
|---|---|
| `src/BlinkerV2/api/WifiOnboardingProfile.h` | `enrollmentKey` 改为 `deviceKey`；只借用 view，不复制 secret |
| `src/BlinkerV2/api/BlinkerFacade.h` | 普通 WiFi 只保留 `begin(WiFi, DeviceKey, SSID, password, interactions...)`；已删除 `WiFi.onboard(...)`，不增加 ESP32 专属 public facade |
| `src/BlinkerV2/provisioning/EnrollmentKey.*` | 删除带前缀/可变长 parser；由新的 32 B `DeviceKey` strict decoder/derive 取代 |
| `src/BlinkerV2/provisioning/EnrollmentKeyHttpExchange.*` | 不做字符串 bearer header；替换为 challenge/proof/session client |
| `src/BlinkerV2/identity/CloudCredential*`、`provisioning/CloudCredential*` | 从 Education composition/release manifest 移除；代码暂留 Enhanced/历史直到消费者审查 |
| `src/BlinkerV2/control/DeviceKeySessionContract.*`、`HttpDeviceKeySessionProvider.*` | **已完成**独立 DeviceKey locator/challenge/proof 和 versioned SessionCredential；不破坏历史 CloudSession 类型 |
| `ConfiguredDeviceKeySource`、`DeviceKeySessionArena`、`WifiCloudLifecycle` | **已完成** manual path 与平台无关生命周期；32 位为 40/1260/20 B |
| `src/BlinkerV2/arduino/internal/WiFiProduct.h` | 普通 WiFi 与 WiFi+BLE 复用唯一 DeviceKey cloud stack；不再保留 P3/P6 平行登录实现 |
| ESP32/UNO R4 最小 network platform | **已完成**；DKEY 包不携带 crypto Port，旧 platform 继承最小层继续支撑兼容图 |
| ESP32/UNO R4 storage Port | `DeviceKeyStore` Core 已完成；ESP32 使用独立 NVS blob 和 WiFi 双槽 journal；UNO R4 普通 WiFi 使用 Preferences，普通 BLE 不安装 DeviceKey |
| `src/BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.*` | **已完成**；32 位 `<=16 B`、2/45/101 B GetInfo/Install/Bootstrap 请求、24/3 B 响应；GetInfo 暴露 identity/access flags，Install/Bootstrap 保持单 session 单 Key与同 Key 幂等 |
| `src/BlinkerV2/arduino/ports/esp32/Esp32WiFiProv.h/.ipp` | 公共 manager/transaction + 编译期互斥的 BLE/SoftAP scheme factory；成功 GetInfo 的 `hasDeviceKey` 可关闭 network-only 配网门禁，SDK 类型不越过 Port/Product 内部边界 |
| `src/BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h` | 唯一 P6 业务 graph 与 facade binding；默认 BLE、SoftAP 编译期回退，DeviceAccessStore、Cloud 与 Direct 只组装一次；实现独立 `resetNetwork()` |
| `tools/build_arduino_release.py` | 单库/单 ZIP 的唯一发布边界；selector 隔离依赖发现，最终由 section GC + ELF defined-symbol 门禁验证裁剪，不生成产品 ZIP |
| `tests/golden/`、host、Arduino probes | endpoint fault host、P2/P3/P5/P6 ESP32 clean probe、P6 Plain/TLS scheme/Host ELF 隔离、manual ESP32/UNO R4 回归和 package boundary 已完成；掉电、session revoke 仍待补 |
| `examples/WiFiBleQuickStart/` | 只展示 `<BlinkerWiFiBLE.h>`、组件、`begin()`、`run()`；不出现 DeviceKey、SSID、PoP、serviceKey 或 `official_detail` |

建议实施时先新增窄接口和并行 composition，再删除旧默认路径；每一批都要保证没有同时存在两个可被普通
facade 选中的身份根。由于这些实验功能从未发布，不建立 EnrollmentKey/ManualDeviceKey 兼容 parser。

## 9. 官方参考边界

- ESP-IDF Unified Provisioning：<https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/provisioning/provisioning.rst>
- ESP-IDF Wi-Fi Provisioning Manager：<https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/provisioning/wifi_provisioning.html>
- Arduino-ESP32 WiFiProv：<https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiProv>
- Espressif Android provisioning：<https://github.com/espressif/esp-idf-provisioning-android>
- Espressif iOS provisioning：<https://github.com/espressif/esp-idf-provisioning-ios>

这些库解决 ESP32 本地 WiFi provisioning transport/session，不替代 Blinker 的 DeviceKey、logical device、
Broker session、PageSchema、BBP/2 或 Direct BLE 合同。
