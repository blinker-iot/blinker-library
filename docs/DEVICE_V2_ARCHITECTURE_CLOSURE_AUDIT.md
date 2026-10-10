# Device V2 架构收口审计与后续实施主线

> 状态日期：2026-08-28  
> 范围：设备库为主，同时核对 `blinker-app` 与 `aily-blinker-services` 的真实消费者。  
> 结论：WiFi 主链已经形成；ESP32、UNO R4 和 Wio 的纯 BLE 三端基础链均有真机证据，但 Wio BLE 仍是显式预览；ESP32 WiFi+BLE 也仍是预览能力。当前不得据此发布为生产库。

> 2026-08-30 Edge Hub 修订：早期无产品消费者的重复 `DeviceKeyStore` 曾被删除；D4 内部 Edge Hub
> 现在成为第一个确需“持久 Cloud DeviceKey、但不接受 Direct controller”的消费者，因此恢复单一 44 B
> `IAtomicBlobStore` 实现。普通 Direct 设备仍必须使用 260 B `DeviceAccessStore`，二者不得同时装进同一
> Product；architecture 门禁只撤销这一项旧的“无消费者禁止”规则，其余退役源码规则保持不变。

> 2026-08-28 ESP32 BLE 修订：XIAO ESP32-S3 已使用 ESP-IDF NimBLE 普通 GATT bearer 完成 Android + Service
> 真机认领、Direct 重连、Manifest/State 与 Power Command。应用安全由 Noise、Method 2 和
> DirectSecureRecord 提供；默认不再触发 SMP/pairing/bonding。现场“找不到当前板”是 Direct/Provisioning
> 扫描过滤与多块旧板同名叠加，不是设备未广播。App 已改为显示多候选地址后缀与 RSSI。

## 1. 本轮结论

当前正确的用户入口已经收敛为一个库、三个编译期 selector：

```cpp
#include <Blinker.h>          // 默认 WiFi；BlinkerWiFi.h 是显式同义入口
#include <BlinkerBLE.h>       // 纯 BLE
#include <BlinkerWiFiBLE.h>   // 仅 ESP32 WiFi+BLE
```

三者共享 `Property/Action/Event`、`Blinker.begin(...)`、`Blinker.run()`、Manifest、State 和 BBP/2，不再让用户接触
Product、Port、MQTT、HTTP、BLE frame、storage 或 workspace。这个方向正确，不恢复运行时 mode enum，也不拆成多个 ZIP。

但“公开头可编译”不等于“产品可交付”。正式单 ZIP 当前冻结为七个目标（Wio 只含 WiFi），仍有三个发布阻断：

1. C4 发布器已对缺 profile、development、placeholder host、测试 key 和 TLS 无 CA fail-closed；真实 production
   host/key/profile 尚未提供，不能以 CI development profile 代替。
2. Wio BLE 的 Noise/Method 2/DirectSecure 真机链已经闭合，且断电重连通过；但原生 bearer 不做 bonding，
   附近未认证连接可占用单连接资源。长稳、反复断连和认证超时/连接占用 DoS 尚未收口，因此必须保留
   `BLINKER_ENABLE_WIO_BLE_PREVIEW=1`，且不得进入正式 ZIP 编译矩阵。
3. ESP32 WiFi+BLE 的 WiFiProv App bootstrap、`WiFiProv deinit -> Direct` 循环、动态 heap/RF 与
   Cloud+Direct 同时在线仍未完成全部真机门禁。

因此下一主线必须是“产品合同和编译结构收口”，不是新增协议能力。

## 2. 本轮已执行的物理清理

已删除没有正式 Product 消费者或已被 DeviceKey/DeviceAccess 主线替代的实现：

- 旧 LocalControllerPairing 应用/合同/coordinator；
- 旧 DirectBleLifecycle 与 BasicWifiBleLifecycle；
- 未接入正式产品的 FactoryCredentialBundle C++ decoder；
- 当时无消费者的重复 DeviceKeyStore（D4 后仅由 cloud-only Edge Hub 恢复）和未使用的 `IKeyValueStore/IP256CryptoProvider/INetworkCredentialStore`；
- CloudEnrollmentKeyRing 候选实现；
- 无公开实现的 `resetOwnership()` facade/capability/diagnostics；
- OwnershipRelease contract/record/store/coordinator/cleanup/verifier、UNO R4 专用存储槽及三端黄金夹具。
- EnrollmentTicket、OwnershipClaim/Record、CloudCredential Enrollment/Record、LocalSetup 8-operation saga 全链；
- 无产品消费者的独立 `ControllerCredentialStore`、factory/pre-enrollment 生成器和 UNO R4 factory installer；
- ESP32/UNO R4/Wio 纯 BLE 中分离的 ownership/claim/controller/saga blob，统一为一个 176 B `DeviceAccessStore` 原子根。

OwnershipRelease 的删除是有意的合同纠偏。设备解除绑定应以服务器撤销 DeviceKey/session/accessEpoch 为权威；在新的
reset/transfer E2E 合同冻结前，不保留一套无产品入口的设备本地事务协议。

C1 后 `src` 曾为 231 个文件、其中 69 个 `.cpp`。C2 已将普通实现改为 16 个职责 `.cpp` + 69 个就地 `.inc` 片段；
architecture 和 release builder 精确冻结这一集合。旧 pre-enrollment/NVS generator 不再属于 CI。Wio storage schema 从 1
升到 2，测试阶段不迁移旧 ownership/saga image；首次写入新 image 会替换旧布局。最终 Host/Arduino 门禁证据以本轮末尾
复测结果为准，不能沿用删减前数字。

本轮 C1 软件证据：Windows ASan Host `25/25`、5,000 次 deterministic fuzz、保留的五组跨语言 golden、architecture、
Python syntax 和 single-library package boundary 均通过。UNO R4 32 位 SizeProbe 为 `52,408 B Flash / 14,556 B globals`，
剩余 `18,212 B` SRAM；`DeviceAccessStore` 多接口对象门禁为 `<=16 B`，当时 schema v1 持久 payload 为 176 B；当前 schema v2 为 196 B 且兼容读 v1。ESP32 BLE Product
clean compile 在超过 10 分钟后仍停留于递归 Core 处理，已主动终止。C2 后 Host ASan `25/25`；ESP32/UNO R4/Wio Core
分别为 `314,787/21,768 B`、`72,648/8,532 B`、`51,340 B Flash`。C3 曾在全新 sketchbook 完成八目标
clean compile；Wio BLE 改为 opt-in preview 后，正式发布矩阵冻结为七目标，其 preview 正向/默认拒绝门禁独立执行。
资源、依赖、ELF 裁剪和安装来源见 `DEVICE_V2_IMPLEMENTATION_UNIT_LAYOUT.md`。

## 3. 当前真实架构

```text
Sketch
  └─ Blinker.h | BlinkerBLE.h | BlinkerWiFiBLE.h
       └─ BlinkerFacade + Property/Action/Event
            └─ platform Product binding（仅选择组合）
                 ├─ WiFiProduct
                 │    └─ DeviceKey → HTTP session → PubSubClient → BBP/2
                 ├─ BleProduct（预览）
                 │    └─ Noise setup → Controller auth → Direct BBP/2
                 └─ Esp32WiFiBleProduct（预览）
                      └─ WiFiProv → DeviceKey/WiFi → Cloud + NimBLE Direct

Core
  ├─ model: Manifest / Endpoint / State
  ├─ protocol: CBOR / BBP/2 / BLE fragment / Noise
  ├─ runtime: lifecycle / scheduler / telemetry lease / outbox
  └─ transport: MQTT / BLE / hub

Arduino boundary
  ├─ internal: Product composition
  ├─ ports: SDK adapter；模式专用实现为 selector 引入的 .ipp
  └─ platform: esp32 / uno_r4 / wio_terminal binding
```

分层方向没有根本问题：Core 不依赖 Arduino SDK，平台实现不泄漏到 Sketch，新增 MCU 主要新增 Port 与 binding。
当前问题是每层仍保留过多历史候选合同，且物理 `.cpp` 粒度直接转化为 Arduino 首次编译成本。

## 4. 三种产品的真实上线流程

### 4.1 WiFi-only：当前唯一闭合主线

```text
App 创建/查看 DeviceKey
  → 用户粘贴到 Sketch
  → Blinker.begin(DeviceKey, SSID, password, ...)
  → 设备以 challenge-HMAC 换短期 SessionCredential
  → MQTT CONNECT/SUBSCRIBE
  → BBP/2 Manifest/State/Command/Event/Telemetry
```

服务器已实现 DeviceKey 创建、查看、轮换、session 与 Broker 投影；Wio Terminal 已做真实 Cloud 门禁。剩余发布问题是
正式 host/key 配置、全平台可靠性与 clean-build 时间，不是登录协议本身。

### 4.2 BLE-only：设备侧合同已闭合，三端/真机仍阻断发布

当前唯一设备图为：

```text
DeviceInstanceId + setupSessionLocator
  → Noise NN secure session + final transcript hash
  → Hello 获取 setupSessionId/accessEpoch
  → server-signed BleEnrollmentGrant + 32 B controller secret
  → 原子 DeviceAccessStore(accessEpoch + Admin credential)
  → HMAC mutation receipt
  → Direct BLE BBP/2
```

服务器保存 ownership/share ACL；设备不保存用户、家庭或 owner 身份。擦除 Flash 后以相同设备页面重新认领，仍由
服务器策略和用户确认决定，不让 MAC 充当全局 deviceId。grant 同时绑定 physical instance、setup session、Noise
transcript、accessEpoch、controller digest、权限、有效期和 server key。设备只接受首个 Admin，exact replay 只允许在同一
活动 session 内重放相同 bytes。详细合同见 `DEVICE_V2_BLE_ENROLLMENT_CONTRACT.md`。App/Service 和真机门禁完成前，
`BlinkerBLE.h` 保持 preview。

### 4.3 ESP32 WiFi+BLE：设备图已成形，App 接线未闭合

目标流程：

```text
App 选择 SoftAP 或 BLE WiFiProv bearer
  → Security1 会话
  → 标准 WiFi credential
  → custom endpoint blinker-config/2 安装 DeviceKey + access bootstrap
  → WiFi Cloud 上线
  → WiFiProv 完全反初始化
  → NimBLE Direct 启动并复用同一 Manifest/State
```

固件当前默认 SoftAP，只有 IDF-NimBLE 目标才可选 BLE bearer，原因是经典 ESP32 上 Bluedroid + NimBLE 双 host 的
Flash/RAM/生命周期风险。App 页面和固件默认 bearer 必须由一次产品决策统一；当前不能把 codec 单测或工具页 WiFi 写入
当作完整配网通过。

## 5. 仍需清理的源码问题

### C1（已完成）：纯 BLE setup graph

`BleEnrollmentGrant → DeviceAccessStore` 已接入 ESP32、UNO R4 与 Wio Terminal 的唯一纯 BLE Product。旧
CloudCredential/Ownership/EnrollmentTicket/LocalSetup saga、平台存储槽、SizeProbe、Host/golden、factory 工具与 CI
引用已物理删除。`DeviceAccessStore::clearAll()` 在 BLE-only 状态下会清除整个 access root，避免清空最后 Admin 后设备
既不能 Direct 认证、又不能重新 enrollment；存在 DeviceKey 时仍保留 cloud 恢复入口。

### C2（已完成）：收敛 Arduino translation units

目标不是 header-only，也不是多 ZIP。普通实现已按稳定职责收口为 16 个 `.cpp`：

```text
BlinkerApi.cpp
BlinkerControl.cpp
BlinkerCore.cpp
BlinkerModel.cpp
BlinkerProtocol.cpp
BlinkerIdentity.cpp
BlinkerProvisioningBle.cpp
BlinkerProvisioningController.cpp
BlinkerProvisioningWifi.cpp
BlinkerRuntime.cpp
BlinkerTransport.cpp
BlinkerSecurity.cpp
BlinkerArduino.cpp
BlinkerEsp32.cpp
BlinkerUnoR4.cpp
BlinkerWioTerminal.cpp
```

原实现以 69 个就地 `.inc` 片段保留，平台 SDK 继续使用 selector 可达 `.ipp`。C3 已在空 sketchbook 安装同一 ZIP 并完成：

- 单个目标 clean compile 时间；
- WiFi/BLE/WiFiBLE 实际依赖集合；
- map/ELF 中未选产品、其他平台和候选协议为零；
- Flash/RAM 不因对象合并而显著回归；
- Host 单元测试继续针对公开行为，而非依赖原文件边界。

本机全新隔离门禁约 85 分钟；旧的“空缓存单目标 180 秒/同平台 90 秒”没有真实依据，已撤销。门禁现直接记录安装、依赖、
逐目标编译与总耗时；只有取得稳定 CI 多次样本后才设置回归阈值。

### P1：收窄 Advanced 入口

`BlinkerV2Advanced.h` 当前聚合大量内部 interface、crypto、provisioning 和 storage 类型，容易把测试 seam 误认为稳定 API。
短期仅允许 tests/tools 使用；正式发布前应改成小型 `BlinkerPort.h`（只含被支持的 adapter interfaces），或者完全不发布
聚合头。普通示例和 Product 禁止包含它。

### P1：统一发布配置注入

正式 host、port、TLS/plain profile、server key ring 应由发布配置文件生成或 CI 注入，并有“placeholder 拒绝发布”门禁。
用户仍只写 DeviceKey/SSID/password，不接触 endpoint。Plain HTTP/MQTT 可作为资源受限默认，但必须明确网络威胁模型；
TLS 开启后不允许静默降级。Cloud Frame Authentication 继续留在后续安全批次，不阻塞本地开发，却阻断不可信网络上的
Plain MQTT 正式发布。

### P1：单库依赖安装成本

Arduino `depends=` 不能按 selector 条件化，统一库会安装 WiFi/BLE 的依赖并集。这是安装成本，不等于固件闭包污染。
保持单 ZIP；优先减少重复/可由 core 提供的依赖，并固定版本。只有 Library Manager/IDE 实证表明不可接受时，才评估
预编译架构归档，不能退回每 MCU/模式一个 ZIP。

## 6. 三端遗漏清单

| 优先级 | 设备 | App | Service/Broker |
|---|---|---|---|
| P0 | BLE enrollment endpoint + DeviceAccessStore 已完成；待真机 | BLE 发现、服务器 grant、Noise relay、失败恢复 | 发行/消费一次性 BleEnrollmentGrant、ACL 权威记录 |
| P0 | WiFiProv custom endpoint 已有 | 把现有 codec/transport 接入正式页面；统一 SoftAP/BLE bearer | DeviceKey/access bootstrap API 与幂等/撤销 |
| P0 | 发布配置 placeholder gate | 显示明确环境/错误 | 正式 control host、signing key rotation、session endpoint |
| P1 | 16 `.cpp` 与单 ZIP 七个正式目标门禁已收口；Wio BLE 独立 preview gate | 无 | CI 保存 build time/ELF artifact |
| P1 | Manifest/version 已有 | 页面为空时手动/Agent 生成；fingerprint 变化提示更新 | 保存 PageLayout 与 Manifest fingerprint，不把样式写入设备 Manifest |
| P1 | telemetry D1～D4 已完成 | 真实移动前后台/kill 最终门禁 | ACL revoke、lease sweep 与 fanout 已有软件/本地门禁 |
| P1 | 纯 BLE Shared Rotate/Revoke endpoint 已有底座 | Admin 近场同步后为其余成员更新 Shared secret | ACL 立即停云转发；不得把离线 Direct 撤权描述为立即 |
| P2 | Wio Noise-only bearer 已通过真实断电、认证 deadline、32 次重连与 30 min authenticated engineering soak；待 24 h authenticated DirectSecure release soak | Android/iOS 权限和网络切换 | 生产容量、观测、密钥轮换 |

History/Bulk 是明确延后项，不是遗漏。只有上述 P0/P1 闭合后，才重新评估 D5/D6。

## 7. 修订后的执行顺序

1. **C1 BLE 合同收口（设备侧已完成）：**冻结 `BleEnrollmentGrant → DeviceAccessStore` exact wire，删除旧 cloud/ownership saga。
2. **C2 编译结构收口（已完成）：**69 个普通 `.cpp` 已合并为 16 个稳定 translation unit；用户 API/wire 未改。
3. **C3 单 ZIP 门禁（已完成）：**七个正式目标隔离安装 clean compile、ELF/依赖/来源与资源审计全部通过；Wio BLE 只走 opt-in 内部门禁。
4. **C4 发布配置（当前）：**注入器与 placeholder/test-key fail-closed 已完成；待真实 production host/key/profile 后对精确 ZIP 重跑七目标。
5. **C5 三端最小接入（BLE 已完成）：**BLE enrollment/Direct 已完成；WiFiProv custom bootstrap 不同时重构页面渲染。
6. **C6 真机：**ESP32 WiFiProv → Cloud + Direct 仍待；Wio BLE 已冻结为 Noise/Method 2/DirectSecure 应用安全根并通过真实断电、未认证 deadline、32 次重连和 30 min authenticated engineering soak，下一步只补 24 h authenticated DirectSecure release soak。
7. **C7 页面：**Manifest fingerprint 与 PageLayout 手动/Agent 流程；之后才考虑 History/Bulk。

每批都先删除旧消费者和旧合同，再新增唯一正式图；禁止长期并存 `candidate/internal/official` 三套同义 composition。

## 8. 当前可对外承诺边界

- 可以承诺：单 Arduino 库、统一 API、WiFi DeviceKey/MQTT/BBP2 主链、ESP32/UNO R4 纯 BLE 三端基础链、Wio WiFi 真机证据、设备端 telemetry 核心和纯 BLE 唯一 enrollment 合同。
- 只能标记 preview：ESP32 WiFi+BLE；Wio BLE 已关闭单连接占用 DoS，但 24 h authenticated DirectSecure 长稳未完成，继续保持显式 opt-in。
- 不能承诺：正式云 endpoint/key、App 正式 WiFiProv bootstrap、ESP32 Cloud+Direct 真机、Wio BLE 生产级抗滥用与长稳。

这份边界高于历史进度文档中的“已实现”字样；历史条目只说明某一时点的代码或测试存在，不自动构成当前产品准入。
