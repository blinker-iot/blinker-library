# Device V2 各模式上线与通信协议总览

> 状态：2026-08-18 三端沟通基线。鉴权、每设备选择性分享与 Plain MQTT 边界以
> `DEVICE_V2_AUTHORIZATION_AND_SHARING_DECISION.md` 为准；本文中的“每 App controller 独立”旧描述已撤回。
> DeviceKey 细节以 `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 为准；平台配网优先级以
> `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md` 为准；业务模型以 `DEVICE_V2_CANONICAL_API.md` 为准。

## 1. 一句话模型

设备固件用 Property/Action/Event 描述能力，通过 BBP/2 传状态和命令；App/服务器保存 PageSchema。
联网设备用长期 DeviceKey 做 challenge-HMAC，只把短期 SessionCredential 交给 MQTT/WebSocket。
WiFi、BLE、WiFi+BLE 只是安装网络/Key和承载数据的不同 transport，不产生不同业务 API。

## 2. 核心对象

| 对象 | 是否秘密 | 生命周期 | 用途 |
|---|---:|---|---|
| `deviceInstanceId` | 否 | 当前物理存储实例 | 发现/诊断，不从 MAC 推导 |
| `logicalDeviceId` | 否 | App 设备页/ACL 生命周期 | 页面、历史、路由主键 |
| `DeviceKey` | 是 | 直到刷新/吊销 | 联网设备根认证；32 B/43 字符无前缀 |
| `credentialVersion` | 否 | 单调递增 | Key/session 强吊销 epoch |
| `SessionCredential` | 是 | 一次连接/短 TTL | MQTT/WS 登录 |
| `EnrollmentTicket` | 是/签名能力 | 5～15 分钟、单事务 | App 配网时授权安装 DeviceKey/认领 |
| `Admin/Shared ControllerCredential` | 是 | 每设备访问组；最多两槽 | Direct BLE/LAN HMAC；Shared 只授予该设备被分享用户 |
| `accessEpoch` | 否 | 整机 Direct 访问代次 | 转让/恢复后拒绝旧 Direct 凭据；不是 ownership |
| `PageSchema` | 否 | 用户页面生命周期 | widget/layout/style 与 stable key binding |

Education/DIY 默认不再使用独立长期 CloudCredential。现有 CloudCredential/Noise/LocalSetup 代码属于
Enhanced/历史候选，不能决定普通设备上线流程。

## 3. 模式矩阵

| 模式 | 首次安装 | 日常通信 | 当前准入 |
|---|---|---|---|
| 手动 WiFi | App 显示 DeviceKey；Sketch 写 Key/SSID/password | MQTT/BBP2；TLS 可选，Plain MAC 尚缺 | ESP32、UNO R4 第一优先 |
| ESP32 WiFi+BLE | WiFiProv Security1；标准 WiFi endpoint + Blinker access bootstrap endpoint | MQTT/BBP2 + 可选 Direct BLE | internal 软件接线完成；App/Service、全量资源重测/真机待完成 |
| UNO R4 普通 WiFi | Sketch 写 DeviceKey/SSID/password | MQTT/BBP2；TLS 可选 | 设备 facade/package 已完成；不启动 BLE |
| UNO R4 普通 BLE | App Ticket/ControllerCredential | 附近 Direct BLE BBP/2 | 设备 facade/package 已完成；不含 WiFi/cloud，App/Service/真机待完成 |
| ESP32/UNO R4 Platform BLE-only | App Ticket/Ownership + ControllerCredential | 附近 Direct BLE BBP/2 | 统一公开设备 API 已完成；发布仍等 App/Service/真机 |
| LocalBLE | 物理确认 + ControllerCredential | 完全本地 Direct BLE | internal graph 存在，不进入服务器 |
| Wio Terminal WiFi/BLE | 手动 DeviceKey WiFi；独立 BLE-only | MQTT BBP/2 或 Direct BLE BBP/2 | WiFi 真机主链完成；BLE 首次安全认领已验，但受 `rpcBLE 2.1.3` 重广播限制，仅 experimental |
| MCU + NCP | Key/配网由 Connectivity Agent 管理 | 主 MCU 走 BBP/2/UART，NCP 走网络 | 长期跨平台方向 |

WiFi-only MQTT 不保存 ControllerCredential 或 owner；服务器用每用户每设备 ACL 转发。BLE/LAN Direct 才使用
每设备 Admin/Shared。Shared 不是家庭全局 key，分享一台设备不会获得同一家庭的其他设备。

## 4. 手动 WiFi 上线

```text
App create logical device + PageSchema + DeviceKey
  -> user copies current Key into Sketch
  -> #include <BlinkerWiFi.h>
  -> Blinker.begin(key, ssid, password, interactions...)
  -> connect AP
  -> DeviceKey challenge-HMAC
  -> short SessionCredential
  -> MQTT CONNECT
  -> BBP/2 HELLO -> Manifest -> State snapshot
  -> ProtocolReady / StateFresh
```

用户换电脑或全擦后，从同一设备页复制当前 Key 即可回到同一 logical device。Key 不是 deviceId，服务器通过
派生 locator 找到当前 logical device。

## 5. ESP32 WiFiProv 上线

```text
App create/open logical device
  -> user opens physical provisioning window
  -> WiFiProv Security1
  -> read device instance/setup session
  -> App creates a short server provisioning intent and pending DeviceKey
  -> standard WiFiProv writes WiFi candidate
  -> blinker-config writes only the exact DeviceKey; Ticket/intent stays App/Service-side
  -> device verifies/stores pending Key and confirms WiFi
  -> DeviceKey challenge-HMAC -> SessionCredential
  -> MQTT/BBP2 ready
  -> stop WiFiProv; optional Direct BLE starts and App rescans
```

当前 ESP32 有两个明确候选：P3 的 BLE WiFiProv WiFi-only Plain 图刚好保留 10.54% app-slot，其 TLS 图只余
6.11%，拒绝作为默认；P6 的 SoftAP Security1 + NimBLE Direct Plain/TLS 在 access/control 接线前曾保留
30.02%/25.58%，使其成为 WiFi+BLE 领先候选，但当前静态门槛需重测。P4 单 Bluedroid 的 7.33% 仅作为历史决策数据，
其未发布 product code 已删除。
P6 的 AP WPA `serviceKey` 与 Security1 PoP 是两个独立层；正式 App 可以从同一 setup
code 域分离派生，但不能把任一者当 DeviceKey。P6 已用 176 B 原子 store 和 ControllerControl endpoint
完成 internal 软件接线；App/Service、Cloud Plain MAC、完整 link 资源重测和真机仍未完成，因此它仍是产品
候选，不是已经发布的默认能力。此前 P6 Flash/RAM 数字是在新增控制面之前取得，只能作为历史基线。

没有预置 Key/QR/factory secret的全新 Education 设备只能依赖物理窗口；无唯一 PoP 的 Security1 不宣称抗
主动 MITM。重新配网或预置设备可由 DeviceKey 派生独立 PoP。Security2/Noise 只作为增强档。

## 6. UNO R4 与 Wio

UNO R4 当前冻结为两个独立产品：

- WiFiS3 station、Preferences 网络/身份存储；
- Plain `WiFiClient` 与可选 `WiFiSSLClient`；
- ArduinoBLE encrypted characteristic；
- 当前 ATT connection handle 的 encryption-complete 检查；
- 普通 BLE 的统一 facade 和 Platform BLE composition；与 WiFi 一起发布在同一个 Arduino ZIP，由入口头裁剪链接。

普通 WiFi 只使用手动 DeviceKey，不启动 BLE；普通 BLE 不包含 WiFi、HTTP、MQTT 或云 DeviceKey。
明确不实现 UNO R4 的 BLE 配 WiFi和 WiFi+BLE，因此双无线共存不再是发布门禁。BLE 仍缺真机
pairing/encryption-loss、32 KiB SRAM 峰值、掉电和长稳，App/Service 也需完成 Ticket/Controller 流程。

Wio Terminal 已有独立 rpcWiFi/rpcBLE adapter、持久存储与普通 WiFi/BLE Product。WiFi 使用手动 DeviceKey，
已完成真机登录、MQTT/BBP2 与 Gateway 重启恢复；BLE 应用层闭环已完成，但真实断电后的 Android RPA bond
恢复被闭源 RTL stack 阻断，保持 blocked/preview。不把 ESP-IDF WiFiProv wire 移植到 SAMD51，也不提供
WiFi+BLE composition。

## 7. Key 刷新与离线

```text
App rotate request
  -> Server creates version N+1
  -> revoke old DeviceKey/challenges/pending transactions
  -> revoke all sessions version <= N
  -> invalidate Gateway/Broker caches
  -> actively disconnect MQTT/WS connections version <= N
  -> App receives new Key
  -> device must be reflashed or reprovisioned
```

logicalDeviceId、PageSchema 和历史保持不变。旧 MQTT/JWT 即使 TTL 未到也不能重连；现有连接也必须断开。
刷新默认不向旧设备自动下发新 Key。

## 8. Direct BLE

Direct BLE 与云 session 独立：

- BLE link 必须报告当前连接真实 encrypted 状态；
- ControllerCredential Method 2 HMAC 提供 App controller 身份；
- BBP/2 BLE 使用 4 B 分片头、独立 sequence/replay/Field ID session；
- 一个 Property 的 cloud 与 BLE 更新进入同一 StateStore/callback；
- LocalBLE 不创建 logicalDeviceId、DeviceKey、MQTT 或服务器 online；
- 平台 BLE-only 可以有 logical device/Ownership/Controller，但没有 Cloud Session 是正常状态。

## 9. Plain/TLS

- 默认允许 Plain，但 root DeviceKey 不上网；challenge/proof/session 带随机数、TTL、序号和防重放。
- Plain 不提供业务数据保密，短期 session 仍可能被监听，因此 TTL/rotate/ACL 必须有效。
- TLS 是用户/产品可选 profile，使用平台 secure client 或 NCP；启用后禁止静默降级。
- AP 的 WPA2/WPA3、BLE link encryption、cloud TLS 和应用身份认证是不同层，文档/UI 不得混称“已加密”。

## 10. BBP/2 与页面

设备只发送 Manifest、Property 状态、Action、Event、ACK/Error，不发送 widget、布局、颜色或 JS。App 通过
PageSchema 把 stable key 绑定到一个或多个组件。MQTT、Direct BLE 和未来 UART/NCP 均承载同一 BBP/2
业务语义；WiFiProv protobuf 只服务 ESP32 配网，不进入 BBP/2。

## 11. 当前执行顺序

1. DKEY-1 冻结三端 Key/challenge/session exact wire 和 golden；
2. Device/Service/App 完成强 rotate+disconnect E2E；
3. ESP32 Security1 P1～P6 与 P3/P6 TLS 静态资源 probe 已完成；P4 失败代码已删除；
4. ESP32 device WiFiProv adapter 已完成；App 分别接 BLE/SoftAP coordinator，Service 接 pending Key/status；
5. ESP32 真机首次配网、掉电、刷新、Direct BLE、24 h；
6. UNO R4 轻量默认 BLE setup；
7. Wio/NCP 按真实 adapter/资源/真机证据准入；
8. 清理默认图中无人消费的 CloudCredential/Noise saga，再冻结公共 facade/QuickStart。

现阶段不能把 internal composition clean compile 写成用户功能已完成，也不能让 App/服务器先按旧
ManualDeviceKey/CloudCredential 文档实现生产接口。
