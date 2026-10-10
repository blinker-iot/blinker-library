# Device V2 WiFi / BLE / WiFi+BLE 接入、认领与凭据决策

> **2026-08-17 凭据与配网优先级修订：**本文保留 ID、Ownership、Controller、重置和历史软件证据；
> 其中 `ManualDeviceKey/bdk_`、默认 CloudCredential、Key 刷新不掉线、默认 Noise 配网和 R4b 后续顺序已被
> `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 与 `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md` 取代。
> 新主线为无前缀长期 `DeviceKey -> challenge-HMAC -> 短期 SessionCredential`；刷新 Key 必须吊销旧 Key、
> 全部旧 session 并主动断开当前连接。ESP32 优先 WiFiProv Security1；Security2/Noise 为 Enhanced；
> UNO R4/Wio 的真实能力以新平台计划为准。下文冲突段落仅作历史记录，不得继续实现。
> 2026-08-18 再修订：设备不保存 OwnershipBinding/owner/logicalDeviceId；平台 Direct 使用 accessEpoch 和
> 每设备 Admin/Shared 两槽，P6 首次原子提交 Key+epoch+Admin。鉴权权威改为
> `DEVICE_V2_AUTHORIZATION_AND_SHARING_DECISION.md`，下文四槽/完整 Ownership 设备图只保留历史证据。
> 2026-08-18 平台矩阵覆盖：UNO R4 只提供独立普通 WiFi 与独立普通 BLE，不实现 BLE 配 WiFi或
> WiFi+BLE。下文 UNO R4 WiFi+BLE/双无线准入均为已退出的历史实验；ESP32 P6 是唯一正式设备 composition，
> 但生产准入仍等待 App/Service 和真机。
> 2026-08-18 公开 API 再收口：WiFi/BLE 分别由 `<BlinkerWiFi.h>`/`<BlinkerBLE.h>` 在编译期选择，
> `Blinker.begin()` 不再携带 mode tag；ESP32 P6 使用 `<BlinkerWiFiBLE.h>` 编译期 selector，仍在同一个 ZIP 中。
> 2026-08-22 WiFiProv bearer 再冻结：SoftAP 是跨 ESP32 默认；Arduino Core 已使用 IDF-NimBLE 的目标可在
> include 前定义 `BLINKER_ESP32_PROVISIONING_BLE` 选择 BLE WiFiProv。公开 `begin()`、DeviceKey/bootstrap
> wire、运行期 Direct BLE 与发布 ZIP 均不变化；经典 ESP32 因 Bluedroid + NimBLE 双 Host 被编译期拒绝。

> 状态：2026-08-17 正式修订基线。Ownership/Cloud/Controller/Release common wire 已冻结；WiFi-only
> 旧 EnrollmentKey outer 曾完成软件实验，但已被长期可查看/刷新的 ManualDeviceKey 决策取代；R4b-4a EnrollmentTicket signed body、R4b-4b 普通 DIY Noise NN secure wire与
> R4b-4c LocalSetup application envelope、R4b-4d durable setup saga 与 R4b-5A 平台无关 WiFi+BLE
> lifecycle 已冻结；2026-08-17 保留独立 RecoveryGrant，并撤回短期 RecoveryKey 双码 UX；其设备 codec/golden 与
> App/Gateway 生产实现尚未完成；official platform object graph/真机 composition 尚未完成。
> 本文是设备、App、Gateway 在接入、身份、
> 凭据、重启和重置方面的最高优先级决策。原 `developerEnrollmentToken/setupKey/localAccessKey`、
> `DeveloperSetupCode` 单入口、P-256 Developer 默认身份和 Enrollment/1 均为未发布实验，不是兼容目标。

## 1. 冻结结论

1. 普通 Sketch 不填写 `productId/productKey/productModelId`。服务器内部 `productModelId` 只来自
   ManualDeviceKey/EnrollmentTicket 所属项目或 factory registry；实际能力由 Manifest 描述，页面由
   PageSchema 描述。
2. WiFi、BLE、WiFi+BLE 共用同一套物理身份、逻辑设备、ownership generation、Manifest、状态仓和
   Property/Action/Event 语义；它们只在首次引导方式和日常 transport credential 上不同。
3. 不再要求所有 Developer 设备都有 `SetupCode`。首发明确区分：
   - WiFi-only 手动模式：使用长期可查看、可复制、可刷新的 `ManualDeviceKey`；
   - ESP32 WiFi+BLE App 配网：App/Service 使用短期、单次 bootstrap intent；设备只接收 exact
     DeviceKey + initial controller Bootstrap，不把 Ticket 当登录 secret；
   - BLE-only 平台设备：使用 App 登录态、物理在场和服务器签发的 Ticket/OwnershipGrant；
   - LocalBLE：只做本地 controller 配对，不要求服务器或 Ticket；
   - 量产设备：使用每台设备唯一 `DeviceBootstrapCredential`，可另加 QR claim/setup code。
4. `ManualDeviceKey` 只授权手动模式的首次认领/全擦恢复，`EnrollmentTicket` 只授权 App 配网；二者绝不是
   `deviceId`、日常 MQTT 密码或 BLE controller 密码。成功认领后，设备按能力持久化 `CloudCredential` 和/或
   `ControllerCredential`；正常重启永远不再次依赖已过期 Ticket。
5. `physicalDeviceId` 是公开、稳定的物理实例标识；`logicalDeviceId` 是服务器生成的账号/页面/ACL
   主键；任何 secret 都不得直接作为 ID，MAC/广播名也不得作为任一 ID。
6. Ownership Claim、Cloud Credential Enrollment、Controller Enrollment 是三个独立事务。一次向导可以
   连续完成三者，但数据库状态、wire body、幂等键、失败恢复和撤销范围必须分开。
7. WiFi+BLE 是一台设备的两个运行会话，不是两台设备。MQTT/WebSocket 与 Direct BLE 各自维护认证、
   sequence、重放窗口和 session-local Field ID，共享业务 callback 与权威状态。
8. TLS 不是所有设备的强制前提：Education Plain 默认可用，但 ManualDeviceKey 只发送 key id + exact-body
   HMAC proof，不发送 raw Key。仍须明示长期可查看 Key 的替换授权风险。注册长期对称 secret 即使在 Plain transport 下也必须使用独立
   端到端保护或不传 raw secret 的 suite。日常 MQTT 可按资源档选择 Plain/TLS，但不得静默降级，也不得
   在明文链路上反复发送长期 bearer。
9. 无预置设备根的 Arduino/教育档只能证明“当前用户在配网窗口内控制了这一实例”，不能证明硬件不可
   克隆。需要防克隆、售后找回或可靠转让的产品必须使用每台唯一的 factory bootstrap key/cert。
10. 手动模式 App 先创建 logical device 并长期显示一个 `ManualDeviceKey`。同一 Key 第一次授权 generation 1，
    全片擦除后授权 server-signed `RecoveryGrant` 把新实例锁定到旧 logical ID 的 N+1；用户无需切换 Key。
    BLE profile 仍由登录 App 创建 recovery intent，不显示手动 Key。

## 2. 统一术语与数据所有权

| 名称 | 是否秘密 | 生命周期 | 生成/保存方 | 用途 |
|---|---:|---|---|---|
| `deviceInstanceId` | 否 | 物理实例生命周期 | DIY 设备首次启动生成 16 B 随机值并持久化 | physicalDeviceId 的二进制本体；不是认证 key |
| `physicalDeviceId` | 否 | 物理实例生命周期 | instance ID 编码或 factory identity 派生 | 发现后的稳定资产标识；不从 MAC 推导 |
| `logicalDeviceId` | 否 | 一次服务器所有权关系 | 手动模式创建设备页时生成；自动 profile 在 Claim 事务生成 | 账号、页面、ACL、历史和路由主键 |
| `ManualDeviceKey` | 是 | 手动 logical device 生命周期；可轮换 | App/Console 可长期 reveal/copy/refresh，通常写入 WiFi-only Sketch | 首次 Claim 或全擦 Recovery 的外围授权；不用于日常登录 |
| `EnrollmentTicket` | 是 | 5～15 分钟；一次会话 | App 登录后向服务器申请 | WiFi+BLE/BLE 首次配网和认领授权 |
| `RecoveryGrant` | 否但必须签名 | 一次 replacement transaction | Gateway 签发；设备 strict 验签 | 绑定 new instance、旧 logical ID、N/N+1 与 request digest |
| `OwnershipReceipt/Grant` | 否但必须签名 | 一个 ownership generation | 服务器保存 exact bytes；设备验签后提交规范化记录 | 证明服务器已接受绑定、owner 和代次 |
| `DeviceBootstrapCredential` | 是/私钥 | 出厂到吊销 | 每台量产设备唯一 | 证明 factory physical asset；不用于 MQTT |
| `CloudCredential` | 是/私钥 | 长期、可轮换/撤销 | 设备优先生成，设备持久化，服务器存 verifier/public key | 每次开机申请短期 cloud session |
| `SessionCredential` | 是 | 一次连接或数小时 | Gateway 签发；设备只放 RAM | MQTT/WebSocket 登录 |
| `ControllerCredential` | 是/32 B HMAC secret | 每个 App controller 独立 | App 生成并存 Keystore；设备只在已认证安装后持久化 | 日常 Direct BLE 认证与逐控制端撤销 |
| `productModelId` | 否 | 服务器配置生命周期 | enrollment intent 或 factory registry | 内部型号/模板归属；不由设备自报 |

禁止继续使用含义不清的 `key`、`authKey`、`deviceCode` 或 `setupKey` 指代以上多个对象。App 文案也应显示
“一次性认领票据”“设备云凭据”“本地控制器”，不能都显示为“设备 Key”。

### 2.1 ID 与 secret 的边界

- 不把 `ManualDeviceKey/EnrollmentTicket/CloudCredential` 原文或 hash 截断值当作 deviceId；否则会把
  可公开、稳定的路由标识与必须保密、可轮换的认证材料绑定在一起。
- DIY 设备首次启动用 CSPRNG 生成并原子保存 16 B `deviceInstanceId`，例如编码为
  `physicalDeviceId = "dev_" + lowercase_hex(deviceInstanceId)`。128-bit 随机碰撞在平台规模下可忽略；该
  ID 是公开 locator，不提供认证能力，也不是 secret/hash。常规升级、网络重置和应用重置保留它；全片
  擦除可能产生新实例，服务器不得靠 MAC 自动认回旧设备。
- Gateway 对 physicalDeviceId 建唯一约束。若同一 ID 已绑定另一 Active 资产/identity proof，返回
  `AlreadyClaimed/PossibleClone` 并要求用户恢复或售后处理，绝不能像旧 MAC 流程一样随机注册失败、覆盖
  原设备或自动生成另一 owner 关系。
- 量产设备的 physicalDeviceId 来自 factory registry，身份验证来自每台唯一 bootstrap key/cert，而不是
  ID 字符串本身。

### 2.2 设备侧 OwnershipRecord 边界

`OwnershipClaimGrant` 是 R4b-2 已冻结的 server-signed wire 对象；`OwnershipRecord` 是设备对“已经验证并
commit 的结果”的规范化持久记录，两者不能混为一个结构：

- Gateway claim journal 必须保留恢复 exact receipt/commit 所需的事务材料，并永久保存首次签发的
  exact bytes；App 只透明 relay，不重编码；
- Active 后设备只保存 `state=Active + generation + logicalDeviceId +
  SHA-256(exact signed grant bytes)`，不把 owner/tenant、时间、签名算法或 wire CBOR 塞入固定 flash ABI；
- 当前 32 位 ABI `OwnershipRecord` 为 104 B，`BOS2` 原子记录固定 112 B。CRC 只检测损坏，不提供防篡改；
  Education PlainFlash 档不得宣称防物理攻击，Enhanced/Manufactured 应使用受保护存储或启动时重验 receipt；
- 授权的 reset/transfer 不擦除记录，而以 generation CAS 写入
  `state=Unclaimed + nextGeneration` tombstone。它不含旧 logicalDeviceId/grant fingerprint，但保留防止旧
  receipt/controller 被重新安装的代次下限；只有整机退役、存储修复或全片擦除才调用物理 erase。

因此 active store 的 `installVerifiedActive/retireVerified` 都只接收上层已经完成 server signature、scope、
physical/logical ID、setup session 与 generation 校验的结果；store 自身不冒充 wire verifier。

## 3. 引导方式矩阵

| Profile | 首次授权 | App 是否在场 | 认领后持久化 | 默认安全保证 |
|---|---|---:|---|---|
| WiFi-only 手动 DIY | `ManualDeviceKey` + WiFi credentials | 否 | OwnershipRecord + CloudCredential + WiFi | 长期可恢复的教育档授权；不宣称防固件复制 |
| ESP32 WiFi+BLE DIY | App bootstrap intent + WiFiProv 物理配网窗口（SoftAP 默认，IDF-NimBLE 目标可选 BLE） | 是 | DeviceAccessStore + WiFi | server-authenticated App；无唯一 PoP 时设备真实性为教育档 |
| BLE-only 平台 | Ticket/OwnershipGrant + 物理配网窗口 | 是 | OwnershipRecord + ControllerCredential | 不需要 MQTT，不强制 SetupCode |
| LocalBLE | 本地物理确认/可选短码 | 可不登录 | ControllerCredential | 无服务器所有权和云历史 |
| Manufactured | DeviceBootstrapCredential + 可选 QR claim code | 视产品而定 | 按能力安装 Cloud/Controller credential | 每台资产可验证，可支持转让/售后 |

“一个统一协议”不等于“所有 profile 使用同一个 secret”。统一的是 Claim 结果、ownership generation、
receipt、凭据激活和重置语义；不同引导通道使用不同授权对象。

## 4. 共同状态模型

App 与服务器不得用一个 `Active` 同时表示绑定、联网、云在线和 BLE 可控：

| 维度 | 状态示例 |
|---|---|
| Ownership | Unclaimed / ClaimPending / Active / ResetPending / Revoked |
| Network | Unknown / CredentialsStaged / Connected / Failed |
| Cloud credential | Absent / Pending / Active / Revoked |
| Cloud connection | Offline / SessionReady / MQTTOnline |
| Local controller | Absent / Pending / Active / Revoked |
| BLE link | NotDiscovered / Provisioning / DirectReady / Connected |

~~~mermaid
flowchart LR
    A["SetupReady"] --> B["Bootstrap authorized"]
    B --> C["Ownership Claim pending"]
    C --> D["Ownership Active"]
    D --> E["CloudCredential Active"]
    D --> F["ControllerCredential Active"]
    E --> G["SessionCredential issued"]
    G --> H["Cloud online"]
    F --> I["Direct BLE ready"]
~~~

WiFi-only 通常走 `D -> E -> G -> H`；BLE-only 走 `D -> F -> I`；WiFi+BLE 同时走两条分支。任一分支
失败都不得伪造另一条成功，也不得创建第二个 logicalDeviceId。

## 5. WiFi-only：代码中写入网络和首次授权

适用于没有 BLE 配网通道、用户希望像 dev_edu 一样直接编译运行的教育设备。

统一 API 语义（`bdk_` parser/proof 仍待 R4b-2F 替换）：

~~~cpp
Blinker.begin(
    blinker::WiFi.onboard(WIFI_SSID, WIFI_PASSWORD, BLINKER_DEVICE_KEY),
    power, temperature);
~~~

手动 QuickStart 始终保留一个写法。已 Active 的板子先加载 Ownership/Cloud 并忽略 Key；Key 只有在 storage
全空时参与首次 Claim/Recovery。factory/pre-enrolled profile 可以使用无 Key 的 `blinker::WiFi` tag，但它不是
手动 DIY 的第二套示例。

1. App/Console 先创建 `AwaitingFirstClaim` logical device、PageSchema 和长期 `ManualDeviceKey`。Key 可在设备
   详情反复查看/复制，并可刷新轮换；此时不创建 Active physical relationship 或 CloudCredential。
2. 设备首次启动生成并原子保存 `deviceInstanceId`；Claim coordinator 把 generation、requestId 和 nonce
   写入独立 48 B `BOC2` journal，重启后重放同一请求。
3. 设备连接 WiFi，以 `Blinker-Manual-Key-Id` + exact-body HMAC proof 提交 Ownership Claim CBOR；raw
   `bdk_`、Manifest 和页面数据不进入 common Claim body。
4. Gateway 验证 Key/version并创建同一 claim transaction；Grant 返回 Key 已绑定的预创建 logicalDeviceId，
   Claim commit 才激活 physical relationship。
5. 设备先原子 stage 自己生成的 CloudCredential，再向 Gateway 注册 verifier/public key；服务器激活后
   设备原子提交 Active 记录。
6. 设备立即申请短期 SessionCredential 并连接 MQTT/WebSocket。
7. 以后每次重启只读取 WiFi、logicalDeviceId、ownership generation 和 CloudCredential。固件里残留或
   已轮换的 ManualDeviceKey 被忽略；设备正常上线不会发送它。

如果 Key 无效/已轮换，用户从当前设备页复制最新 Key；如果服务器已经接受但响应丢失，相同
`(logicalDeviceId, physicalDeviceId, requesterIdempotencyKey)` 必须恢复同一事务，不能生成第二台设备。

## 6. ESP32 WiFi+BLE：可选 WiFiProv bearer，运行期 Direct BLE

默认 Arduino/教育流程不要求把 SetupCode 写入 Sketch：

~~~cpp
#include <BlinkerWiFiBLE.h>

Blinker.begin(power, temperature);
~~~

1. 首次启动或明确 reset 后打开有时限的 WiFiProv provisioning window。SoftAP 是默认 bearer；只有底层
   Arduino Core 已启用 IDF-NimBLE 的目标才允许编译期选择 BLE。设备服务名为
   `BLINKER_<deviceInstanceId前6B十六进制>`；该名字只用于附近发现，不是 deviceId，也不提供认证。
2. EducationPhysicalWindow 使用 WiFiProv Security1（无 PoP）；默认 SoftAP 是开放临时 AP，BLE bearer 不使用
   AP `serviceKey`。Security1 建立临时 ECDH
   保护的 protocomm 会话，但没有唯一 PoP/factory root 时不宣称抗主动 MITM；UI 必须准确显示这个安全级别。
3. App 按设备声明选择加入临时 AP 或连接 provisioning BLE service，通过标准 WiFiProv endpoint 写 WiFi
   candidate，并通过 `blinker-config` 的 `GetInfo`
   读取 16 B deviceInstanceId 与当前 access 状态。设备在 DeviceKey + WiFi 都成功前不会结束配网。
4. App 使用用户 JWT、deviceInstanceId 和一次性 idempotency key 向 Gateway 申请短期 bootstrap intent。服务器
   创建/恢复同一 logical device，返回 43 字符 DeviceKey；App 本地生成 16 B Admin controllerId 和 32 B
   controller secret，raw secret 不上传服务器。
5. App 通过同一 Security1 session 调用 101 B `Bootstrap`，设备用一个 176 B atomic DeviceAccessStore 同时
   写入 DeviceKey、accessEpoch 与初始 Admin。相同请求幂等，不同 Key/controller 冲突；设备不把
   EnrollmentTicket 当作登录 secret。
6. WiFiProv 仅在 WiFi 成功、DeviceKey bootstrap 成功且 network journal 已提交后 deinit。设备随后以 DeviceKey
   challenge-HMAC 获取短期 SessionCredential，连接 MQTT/BBP2，并启动 NimBLE Direct。
7. 日常 Direct BLE 使用 controller challenge/HMAC；Cloud 与 Direct 共享 Manifest、状态与 callback，但拥有
   独立 session/sequence/replay。正常重启直接读取 DeviceAccessStore 与 WiFi，不再进入配网或依赖短期 intent。
8. App 取消、错误 WiFi、断电和响应丢失必须恢复同一 pending intent；不得重复创建设备。网络重置与所有权/
   access reset 是不同动作，不能用“再次配网”隐式抢占设备。

Enhanced/Manufactured 可给 SoftAP 增加唯一 serviceKey，并给两种 bearer 的 Security1 增加唯一 PoP/QR proof；
二者必须域分离，
且都不能当 DeviceKey。普通 Arduino 首发不要求把 SetupCode 写入 Sketch。

## 7. BLE-only 与 LocalBLE

### 7.1 需要平台账号的 BLE-only

~~~cpp
Blinker.begin(blinker::BLE(), power, temperature);
~~~

设备不能访问 Gateway 时，App 是 transport relay，不是授权根：

1. App 用广播 locator 进入物理 provisioning window，再从受保护payload读取deviceInstanceId/完整setupSessionId并申请Ticket；
2. App 与设备建立受保护本地会话，转发 server challenge/Ticket；
3. 设备产生可用的实例/factory proof；Gateway 验证后签发 server-signed OwnershipGrant；
4. 设备验证并持久化 Ownership grant/generation，App 中继 Ownership commit，使 Ownership 进入 Active；
5. 首个 controller 已由 Ticket exact 绑定 `controllerId/secretDigest/permissions`。App 在同一 protected
   LocalSetup transaction 中发送 raw secret，设备校验 digest 后执行 `InstallInitialController` 并返回 HMAC
   receipt；App 验 receipt 和 Method 2 后完成首个 controller bookkeeping。不能在首个 controller 尚不存在时
   反过来要求 authenticated local session，也不重复签一个 ControllerGrant；后续新增/轮换/revoke 才使用
   Ownership-domain ControllerGrant。

无 factory root 的 DIY 档可以省略硬件真实性 proof，但必须保留服务器签名 grant、物理在场、短 setup
window、重放保护和明确的低保证标识。SetupCode 是可选加强项，不是 BLE 协议必填字段。

### 7.2 完全本地的 LocalBLE

LocalBLE 不创建服务器 logical device，不需要 ManualDeviceKey/Ticket/CloudCredential。物理确认后为 App
安装独立 ControllerCredential；页面可只保存在 App。以后若升级为平台设备，必须显式执行服务器 Ownership
Claim，不能把旧本地 boolean 当作云端所有权。

R4b-6A 已冻结其独立 exact flow：Noise handshake 完成后，设备侧物理事件为当前 final hash 打开默认 60 s
窗口并生成 16 B random nonce；App 在 Noise transport 中提交独立 16 B controller ID/32 B secret，设备只按
自身策略授予权限，持久化 `Local/generation=0/version=1` 并返回 HMAC receipt。同窗口仅允许 exact replay；
optional short code 复用 Noise NNpsk0，不增加永久万能 SetupCode。完整字段和失败语义见
`DEVICE_V2_LOCAL_CONTROLLER_PAIRING_CONTRACT.md`。

R4b-6B 已把该合同接入现有 Noise endpoint，并冻结扫描分流：LocalBLE 普通/PSK 广播 capability 为
`0x0043/0x0061`，含 `LocalControllerPairingV1`，不含 `EnrollmentV1/WifiConfigV1`。平台 BLE-only 则继续
广告 `EnrollmentV1` 并走 Ticket/Saga。两者共用 locator、radio mode、断线恢复和 Direct BLE 数据面，但完成
来源分别是 `Local/generation=0 ControllerCredential` 与 `BSG2 Complete`，授权状态绝不互认。

R4b-6C 已将上述语义分别落到 ESP32/UNO R4 的 Platform BLE 与 LocalBLE internal package。Platform 图携带
server public signing ring，但没有 WiFi/CloudCredential/MQTT；Local 图连 signing ring 都不携带，builder
直接拒绝任何 server/cloud 配置。四图只共享 BLE radio、Noise/Direct、Controller method 2 和 caller-owned
workspace，不共享 Ticket、Ownership、completion 或持久域。这也确认了 BLE-only 不是 WiFi+BLE 图的运行时开关。

已配对 LocalBLE 的恢复/新增手机由设备产品物理入口重新打开 pairing mode；这个动作只恢复 Provisioning
广播。Noise 建立后仍须另一次设备侧确认绑定 exact transcript，BLE packet 不可代替。普通重启直接读取持久
controller credential 进入 Direct；App 丢失首次 receipt/secret 也不要求擦除设备或依赖服务器。

## 8. 长期上线凭据与重启语义

EnrollmentTicket 不是设备登录 key。完整生命周期为：

~~~mermaid
flowchart LR
    A["ManualDeviceKey / EnrollmentTicket<br/>bootstrap authorization"] --> B["Ownership Claim"]
    B --> C["持久 CloudCredential"]
    C --> D["每次重启请求 SessionCredential"]
    D --> E["MQTT / WebSocket session"]
~~~

设备 Active 记录至少包含：

~~~text
logicalDeviceId
ownershipGeneration
cloudCredentialSuite/version/secret-or-private-key
server/control-plane profile
integrity/version metadata
~~~

- `CloudCredential` 首版可设计为“长期直到服务器撤销”，不要求 Arduino 依赖准确 RTC 每月刷新；服务器可
  通过 generation/version 轮换。
- `SessionCredential` 短期且只放 RAM。正常重启重新申请即可，不需要 App 重新配网。
- App 永远不读取或展示 CloudCredential；Gateway 只保存 verifier/public key 或受保护的验证材料。
- 对 HMAC suite，普通 password hash 不能验证 challenge HMAC。Gateway 必须保存 KMS/HSM 加密的对称
  verification key，或保存由设备域隔离派生但仍等价于登录能力的 key material；不能把它误写成不可逆
  verifier。若要求服务器只存公开材料，应选择设备签名/public-key suite。
- 若使用 Plain MQTT，必须使用短期、限权 SessionCredential；长期 bearer 不得作为 MQTT password 反复
  暴露。TLS 是可选运行档，但选择结果必须显式且不可静默回退。

### 8.1 掉电和幂等

| 掉电位置 | 恢复要求 |
|---|---|
| Ticket/Key 尚未被 Gateway 接受 | 继续同一请求；若已过期才申请新授权 |
| Claim 已创建、receipt 未落盘 | 同一请求返回同一 claim/receipt |
| CloudCredential 已 stage、服务器响应丢失 | 使用同一 credential 与 idempotency key 重试 |
| 服务器已激活、设备 commit 响应丢失 | commit 幂等返回同一结果 |
| Active 后任意重启 | 忽略 enrollment 材料，直接用 CloudCredential 登录 |

不得出现“服务器已消费 Ticket，但设备只能重新生成秘密并创建第二台设备”的窗口。设备 credential journal
与服务器 transaction 必须共同保证 old-or-new 和 exact replay。

## 9. 重置、恢复和转让

| 操作 | Identity seed/factory | WiFi | Ownership record | Cloud credential | Controllers | 是否需新首次授权 |
|---|---:|---:|---:|---:|---:|---:|
| 普通重启/升级 | 保留 | 保留 | 保留 | 保留 | 保留 | 否 |
| Application Reset | 保留 | 保留 | 保留 | 保留 | 保留 | 否 |
| Network Reset | 保留 | 清除 | 保留 | 保留 | 保留 | 否；由已授权 App 重配网 |
| Controller revoke | 保留 | 保留 | 保留 | 保留 | 删除指定项 | 否 |
| Ownership Reset/转让 | 保留 | 按策略清除 | 写入 next-generation Unclaimed tombstone | 撤销/清除 | 全部失效 | 是，新 Ticket/Grant |
| 全片擦除后恢复旧页面 | DIY 产生新实例 | 清除 | Recovery 写 N+1 tombstone，再 Claim 旧 logical ID | cutover 时旧凭据撤销，新凭据随后安装 | cutover 时旧项全失效 | 是；同一 `bdk_` 或 App recovery intent |

Network Reset 绝不能复用已经消费的 EnrollmentTicket；现有 owner/controller 在物理配网窗口内获得
`NetworkProvisioningGrant` 即可写入新 WiFi。Ownership Reset 必须同时满足服务器授权、当前 generation
和物理在场，不能仅凭长按按键抢占云端所有权。

所有权转让默认结束旧 logical relationship：旧 logicalDeviceId 进入 Released/Archived，撤销全部 session、
controller 和页面访问；新 owner 完成 Claim 后获得新的 logicalDeviceId。physicalDeviceId 保持不变，用于
资产审计和防止一台物理设备并存两个 Active owner。若商业产品需要连同历史/PageSchema 转让，必须有双方
明确授权和独立数据迁移事务，不能把“换 WiFi”或普通重绑自动解释为数据转让。

全片 DIY 恢复不是 Transfer：当前 owner 明确授权新的 random deviceInstanceId 接管同一 logicalDeviceId，
页面、历史与 ACL 默认保留，generation 只能 N -> N+1。WiFi-only 用户继续使用设备详情长期显示的 `bdk_`
ManualDeviceKey；BLE profile 由登录 App/secure setup 创建同一语义的 recovery intent。设备先验证
signed RecoveryGrant 并建立 N+1 base，再复用 Ownership Claim；Gateway 在 Claim commit 的 generation CAS
中切换 physical relationship 并撤销旧 Cloud/Controller/session。Active boot 优先读持久记录并忽略 Sketch
中的 Key；空存储无码返回 `BLINKER_DEVICE_KEY_REQUIRED`，旧版本返回 `BLINKER_DEVICE_KEY_ROTATED`。
Key 合同见 `DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md`；Recovery exact wire 见
`DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md`。

## 10. 设备库执行方案

按以下依赖顺序执行，不能先写 facade 再让底层勉强匹配：

1. **R4b-0 隔离旧实验**：已删除 public 三 Config、伪 WiFi enrollment path 和旧 official BLE
   package/example；`EnrollmentContract` v1、`HttpEnrollmentDriver`、旧
   `ProvisioningCoordinator/ProductOrchestrator`、P256-derived developer ID 和永久
   `developerEnrollmentToken/setupKey/localAccessKey` 已从 Advanced 聚合头和发布 ZIP 隔离。
   仓内旧测试仅作为新 exact golden 落地前的历史对照，不得进入正式 QuickStart/production contract。
2. **R4b-1 建立持久域**：DIY `DeviceInstanceId`、Ownership、CloudCredential、ControllerCredential、
   NetworkCredential 必须是独立原子记录；Ticket 与 Active credential 绝不落入同一 blob。当前已完成
   16 B instance ID/28 B record、当前 32 位 ABI 108 B `CloudCredential`/116 B `BCS2`，以及 104 B
   `OwnershipRecord`/112 B `BOS2`，以及当时的 64 B `ControllerCredential`/四槽 268 B `BCT2`。该段是已撤回
   的历史图；当前 standalone BCT2 为两槽 140 B，P6 为 196 B schema v2 atomic DeviceAccessStore（兼容读 176 B v1）。Ownership store 已实现 Active exact replay、禁止直接换 owner、
   generation CAS retire 与 Unclaimed tombstone；ESP32 使用独立 `bl_v2_owner/binding`，UNO R4 使用独立
   `ow0/ow1` journal view。Controller table 使用 16 B 随机 ID、权限位、credentialVersion 与 Local/Ownership
   generation 分域；ESP32 为 `bl_v2_ctrl/table`，UNO R4 为 `ct0/ct1`。它不复用旧全局 authKey/SetupPSK，
   Direct BLE 日常 HMAC method 2 已在 R4b-2 第一片冻结；第二片已冻结 Ownership-domain
   ControllerGrant/receipt/commit 和 Install/Rotate/Revoke coordinator。LocalBLE 独立安装合同已由 R4b-6A
   完成；Noise application 与共享 BLE lifecycle 已由 R4b-6B 完成，双平台四个 internal graph 已由 R4b-6C 完成。
   UNO R4 bank 已在原 BlobId 0..5 后追加 `di/oc/ce/or/sg` 五个双槽域，补齐 DeviceInstanceId、Claim、Cloud
   Enrollment、Release 与 LocalSetup Saga journal，原有 key/编号保持不变。
3. **R4b-2 冻结三端合同**：Controller 日常鉴权/变更、Ownership Claim v1、Cloud Credential
   Enrollment v1 与 Ownership Reset/Transfer v1 已完成。Cloud 使用 X25519/AES-GCM sealed secret、
   signed Grant、HMAC Receipt/CommitAck 和独立 BCE2 掉电日志；Release 先写 generation tombstone、
   server-signed CommitAck 后再清凭据，不再沿用 raw-secret Activation 或旧 Reset。R4b-3 第二片已冻结
   旧 `EnrollmentKey` 文本/header outer、四 endpoint、strict error、Ownership signed CommitAck 与完整 WiFi
   onboarding flow 软件探针；其外围授权现必须改为 ManualDeviceKey id/proof/version/reveal/rotate。
   R4b-4a 已冻结绑定当前 BLE setup transcript 的独立 EnrollmentTicket v1，不与 ManualDeviceKey 合并。
   `physicalDeviceId` provider 不依赖 BLE/WiFi MAC，P-256/secure element 只作为增强身份 provider，
   不进入普通 API。
4. **R4b-3 WiFi-only**：Claim journal + Key HTTP outer + Claim → Ownership → CloudCredential flow 已完成；
   第三片已实现 1,414 B 共享 arena 与 40 B staged lifecycle，再进入现有 SessionCredential/MQTT；
   软件 provider 已补齐；official 接线前必须完成 ESP32/UNO R4 真机 server signature/envelope 向量与组合资源门禁，
   不得使用恒真 verifier 或 ESP32 专属公开入口；
   正常 boot 首先读取 Active ownership/credential，存在时忽略缺失、残留或已轮换的 ManualDeviceKey。
5. **R4b-4 BLE setup**：R4b-4a Ticket、R4b-4b shared Noise、R4b-4c LocalSetup application 与 R4b-4d
   durable saga 已完成。ConfigureNetwork 固定为 candidate-store 语义，Cloud 固定为 App exact relay；单一
   BSG2 覆盖 Ownership/Controller/Network/Cloud 重启恢复。Ticket 验证前不得写 WiFi 或提交 Claim。
6. **R4b-5 WiFi+BLE（仅 ESP32 候选）**：R4b-5A 平台无关 lifecycle 已完成，在同一 composition 中复用 R4b-3 cloud session
   和 R4b-4 setup；Managed cloud 与 External Direct BLE 共享一个 Client/State，各自保持
   session/auth/sequence。BSG2 Complete + confirmed WiFi 才允许切 Direct，活动 setup session 断开前不强切。
   ESP32 仅保留 P6 WiFiProv Security1 + NimBLE Direct 业务图；默认 SoftAP，IDF-NimBLE 目标可选 BLE scheme。
   UNO R4 历史双无线图已删除；其 exact ATT handle
   encrypted 状态只服务独立普通 BLE。HC-05/Bluetooth Classic SPP 暂不支持。
7. **R4b-6 BLE-only/LocalBLE**：6A/6B 已完成两种独立授权合同、Local application、共享 BLE setup
   lifecycle 与内部 BLE-only lifecycle；平台 BLE 使用 OwnershipGrant + per-controller credential，LocalBLE 无
   logical device/CloudCredential。Platform BLE 已绑定双平台统一 facade/package；LocalBLE 仍为 internal，
   二者不共享万能 setup key。
8. **R4b-7 正式 facade 的 U-ARCH 修订**：普通产品分别包含 `<BlinkerWiFi.h>` 或 `<BlinkerBLE.h>`，保持一个
   `Blinker.begin(...)` 和 `Blinker.run()`；模式不再作为 `begin()` 参数，也不暴露平台 SDK、MQTT client 或
   crypto graph。WiFi+BLE 仍是 ESP32 internal 候选。只有真实 coordinator 已实现的平台才提供 QuickStart；缺能力返回明确
   `UnsupportedFeature/NotConfigured`，不静默接受参数。
9. 每一步都需 Host、ESP32、UNO R4 的掉电/资源门禁；涉及无线、安全、并发的步骤还必须有真机结果，
   编译通过不能升级为产品准入。

`DeviceInstanceId` 是公开标识，因此允许存放在 PlainFlash；但它必须使用 CSPRNG 首次生成、原子保存，且
跨 Network/Application/Ownership reset 保留。只有明确的整机退役/全片擦除才允许改变它。Manufactured
profile 继续使用 factory registry 的物理 ID，不需要先生成 DIY instance ID。

## 11. App 执行方案

1. V2 不调用 legacy `POST /api/v1/devices` 创建 authKey；手动模式先创建 V2 logical device并展示长期
   ManualDeviceKey，自动 BLE 模式进入独立配网入口。
2. BLE 扫描使用 V2 Service Data、mode、setupSessionLocator 和 capability；MAC/名称只用于发现和去重当前
   扫描结果，不进入持久设备主键。
3. App 登录后按受保护payload中的 deviceInstanceId/完整setupSessionId 请求短期 EnrollmentTicket；只在内存和受保护会话中
   持有，禁止 localStorage、剪贴板、analytics、崩溃日志和 UI 明文展示。
4. WiFi credential 只在 Ticket 绑定的受保护 BLE session 中下发。UI 必须区分 `CandidateStored`、AP
   connected、network confirmed、cloud credential 与 cloud online；错误 SSID 时复用同一 durable
   transaction 重开 secure setup，不重新 claim 或创建 logical device。
5. ControllerCredential 只存 OS Keychain/Keystore；CloudCredential 永不经过 App。BLE-only 的 App relay
   只能搬运 exact signed bytes，不能用本地 `bound=true` 替代 server receipt/commit。
6. Network Reset 使用 owner-authorized NetworkProvisioningGrant，不重新生成 logical device；Ownership
   Reset/转让使用独立高风险 UX。
7. Legacy authKey 页面只服务 legacy device；V2 手动模式显示权限受限的 ManualDeviceKey，不把它描述成
   MQTT/authKey，也不向自动 BLE/量产 profile 强行显示。
8. 手动模式设备详情长期保留一个“设备 Key”卡片，提供显示/复制/刷新；同一 `bdk_` 用于首次烧录和
   全擦后恢复，不再要求用户切换 `bek_`/`brk_`。刷新只轮换 bootstrap Key，不让 Active 设备掉线。
   “重新配置 WiFi”“完全擦除后的重新接入”“删除设备及数据”仍是三个独立动作。

## 12. 服务器执行方案

建议职责表（不是强制 SQL 名称）：

~~~text
manual_device_key(key_id, logical_id, version, kms_secret, state, rotated_at)
manual_bootstrap_transaction(transaction_id, logical_id, key_version, operation,
                             physical_id, generation, request_digest, state)
enrollment_ticket(ticket_id, owner, physical_id, setup_session, setup_transcript_hash,
                  controller_key_digest, permissions, authorized_operations, expires, state)
physical_device(physical_id, identity_kind, model_id, bootstrap_verifier, state)
device_claim(claim_id, request_key, bootstrap_kind, physical_id, logical_id, generation, state)
logical_device(logical_id, physical_id, tenant, ownership_generation, capabilities, state)
ownership_receipt(claim_id, logical_id, generation, exact_bytes, state)
cloud_credential(logical_id, generation, version, suite, kms_encrypted_hmac_key_or_public_key, state)
cloud_credential_enrollment(logical_id, generation, request_id, request_digest,
                            exact_grant, grant_digest, receipt_digest, state)
controller_credential(logical_id, controller_id, role, generation, verifier_or_public_key, state)
device_session(logical_id, credential_version, short_session_credential, expires)
device_manifest(logical_id, revision, fingerprint, canonical_pages)
page_schema(owner, logical_id, revision, document)
~~~

服务器必须提供并区分：

1. ManualDeviceKey create/reveal/rotate/proof：面向手动 WiFi-only，绑定预创建 logical device并按状态分流
   InitialClaim/DeviceRecovery；
2. EnrollmentTicket 创建接口：要求 App JWT，绑定 deviceInstanceId/setupSessionId/setupTranscriptHash、
   controllerId、controller secret digest、设备权限位、authorizedOperations 和 TTL；服务器不持有 raw
   Direct BLE secret；
3. Ownership Claim start/proof/receipt/commit：手动模式激活预创建 logicalDeviceId，自动 profile 创建并激活；
4. CloudCredential enroll/commit 与 SessionCredential issue：不得与 ownership transaction 混表；HMAC
   suite 使用 pinned Gateway X25519 key 的 sealed envelope 注册，并以 KMS/HSM 保护等价 HMAC key，不能
   保存普通 password hash 后声称可验证 challenge HMAC；
5. Controller enroll/revoke 与 server-signed Ownership/NetworkProvisioning grant；
6. exact idempotency、TTL、attempt limit、generation CAS、审计和 secret/body 脱敏；
7. Legacy `/api/v1/devices`、`/auth`、authKey 和 JSON topic 与 V2 数据表/指标/限流完全隔离。
8. 独立 RecoveryGrant transaction：ManualDeviceKey/App intent、old/new physical、N/N+1、exact
   grant/receipt/ack、cutover 与审计不得塞入 generic key boolean。Claim cutover
   原子保留 logical ID/PageSchema/history并撤销旧 Cloud/Controller/session；不兼容 binding 只标记待处理。

`productModelId` 只由 ManualDeviceKey/Ticket 的 project/template intent 或 factory registry 决定。Manifest
fingerprint 用于校验实际能力是否满足模板 policy；设备 HELLO/Claim 不再自报 productId 来修正服务器配置。
对称 CloudCredential 的列实际是受 KMS/HSM 保护的等价 verification key，而不是普通 password hash；
非对称 suite 才能只保存 public key。

## 13. 安全档与诚实边界

| 档位 | 设备根 | 本地首次授权 | 适用 |
|---|---|---|---|
| Education/DIY | 持久随机 seed，无厂商证明 | 物理 setup window + server Ticket；手动 WiFi-only 用 ManualDeviceKey | 示例、开发板、低风险个人项目 |
| Enhanced DIY | 软件 key pair 或高熵 QR secret | Ticket + device proof/QR | 需要更强误绑定防护但无 secure element |
| Manufactured | 每台唯一 bootstrap key/cert/secure element | Ticket/claim code + factory proof | 商用品、转让、售后与防克隆要求 |

不能把 Education 档描述成强设备认证；也不能因为 Manufactured 更安全，就让 Arduino 初学者必须使用
工厂工具、Python installer 或每台烧录流水线。三档共享上层协议，通过 provider/capability 选择保证等级。

## 14. 正式 wire/API 冻结前门禁

- 三端接受本文件术语，删除所有“全部 Developer 设备必须传 SetupCode”的表述和接口候选；
- WiFi-only、BLE-only、LocalBLE 与仅 ESP32 的 WiFi+BLE sequence 和失败恢复表评审通过；
- 明确 Ticket 过期、claim 已接受、Active 后重启、Network Reset、Ownership Reset、全片擦除的 exact 行为；
- ManualDeviceKey/Ticket 不进入 CloudCredential blob，App/Gateway 日志不存在任何 raw secret/proof；
- 相同请求不会重复创建设备，不同 physical ID 不能消费同一一次性授权；
- App 无法伪造 server receipt/grant，设备能验证当前 setupSessionId、generation 和允许 scope；
- ESP32、UNO R4/Renesas 的 CSPRNG、原子存储、server signature verifier、BLE secure session 和资源实测准入；
- V2 logical device 不创建 legacy authKey，日常 SessionCredential 不依赖已消费/过期的 enrollment 材料；
- 再根据上述合同重写 Enrollment wire/golden 和 public facade，旧 Enrollment/1 不保留未发布兼容层。
