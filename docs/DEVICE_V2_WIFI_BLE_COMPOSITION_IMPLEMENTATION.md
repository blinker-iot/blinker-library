# Device V2 WiFi+BLE 编排实施记录

> 状态：2026-08-18。下文 R4b-5A～6C 是 portable Noise/旧 CloudCredential 的 Enhanced/历史图，不是新的
> Education DeviceKey 默认主线。WP-P4 的单 Bluedroid 对照仅余 7.33%，未发布 product code 已删除；WP-P5/P6
> 已在同一共享 WiFiProv/DeviceKey 骨架中完成 WiFiProv Security1 + NimBLE Direct；SoftAP 是默认 bearer，
> IDF-NimBLE 目标可编译期选择 BLE。接线前 Plain/TLS 实编为
> `1,375,887/60,856 B`、`1,463,067/61,456 B`，双 OTA 余 30.02%/25.58%，并通过当时 ELF 单 Host 门禁。
> 当前已升级为 260 B schema v3 atomic access store（兼容读 176 B v1/196 B v2）、P-256 ControllerControl、PresenceKey/version、首次 Admin bootstrap 与三个 controller 槽；当前工作树默认
> SoftAP Plain 正式安装 ZIP 为 `1,406,115/62,848 B`，双 OTA 余 28.48%，`1,395,187/62,848 B` 仅为历史快照。
> XIAO S3 可选 BLE bearer 为 `1,303,487/59,684 B`，但真机切换未验证。它不复用
> 下文 8-operation LocalSetup saga。App 双 bearer UX 与真机仍未完成，因此下文只保留为
> 跨平台/Enhanced 记录，不能据此宣称新 App 已可使用 WiFi+BLE。
> 2026-08-22 清理修订：正式 ESP32 产品已由 `Esp32WifiBleProduct` 直接编排 WiFiProv、Cloud 与 Direct，
> 无发布消费者的 `BasicWifiBleLifecycle/WifiBleLifecycle` 已从 `src`、Advanced 聚合头、SizeProbe 和 host
> 测试物理删除。下文对该类型和旧 internal package 的描述仅是历史过程，不是现行依赖或待办。
>
> **2026-08-18 产品边界覆盖：**UNO R4 WiFi+BLE composition/profile/probe 已物理删除，后续也不再实现。
> UNO R4 只提供独立普通 WiFi 与独立普通 BLE。下文所有 UNO R4 WiFi+BLE 数字和“后续门禁”均是历史实验
> 证据，不是当前待办；ESP32 P6 是唯一正式的 WiFi+BLE device product graph。

## 1. 本批结论

设备端只能有一个 `Client / DeviceRuntime / EndpointStateStore`。WiFi 云会话和 Direct BLE 是同一设备的两个
session，不得分别创建 Client、Manifest、组件状态或 callback。

历史 R4b-5A 曾新增 `BasicWifiBleLifecycle`，并使用 `WifiBleLifecycle` 别名：

~~~text
                                      one Client
                                          |
                                     TransportHub
                           managed /              \ external
                 ManagedMqttTransport          BleFrameTransport
                           |                           |
                 Cloud Session + MQTT          BleModeCoordinator
                                                    /        \
                                      Noise LocalSetup      Direct BBP/2
~~~

- Cloud transport 由 `Client` 负责 `start/poll/stop`；
- Direct BLE 以 `TransportLifecyclePolicy::External` 注册，只由 `BleModeCoordinator` 负责生命周期；
- Provisioning endpoint 不进入 `TransportHub`，防止 Noise 与 BBP/2 同时占用同一 GATT callback；
- `Client::begin()` 只调用一次，切换 BLE mode 时不重建业务模型和状态仓；
- BLE 与 cloud 各自保留 HELLO、sequence、授权和 session-local Field ID。

## 2. 冻结的启动与切换规则

启动先读取 `BSG2`，再启动 WiFi lifecycle、Client 和 BLE mode：

| BSG2 | WiFi active profile | 启动 BLE mode | Cloud gate |
|---|---|---|---|
| 不存在或未 Complete | 任意 | Provisioning | 关闭 |
| Complete | 不存在或未确认 candidate | Provisioning | 关闭 |
| Complete | 已持久确认 | Direct | WiFi Online 后打开 |

运行时规则：

1. `WifiConnectionLifecycle` 暴露 `confirmedProfile()`；它表示 active profile 已经 durable confirm，不表示当前
   AP 一定在线。Direct BLE 因此可在 WiFi 重连期间继续工作。
2. Provisioning 每收到新 LocalSetup request 才重新读取 BSG2，禁止在每次 `loop()` 读取 flash。
3. Saga Complete 且 WiFi profile confirmed 后，只有当前 provisioning session 已断开才切换 Direct。这样最终
   operation response 不会被设备主动切 mode 截断；App 完成后应断开并重新扫描 Direct mode。
4. 未完成 setup 的连接断开或 secure-record fault 使用同一 `setupSessionLocator` 执行 `restartActive()`，不生成
   第二套业务事务，也不重复 WiFi/Cloud credential。
5. 错误网络使 candidate rejected 时继续 Provisioning；LocalSetupSaga 已允许 Complete 身份下替换未确认
   candidate。新 profile 只有连上 AP 且 `confirmActive()` 成功后才能离开配网。
6. Cloud gate 只在 `BSG2 Complete && WiFi Online` 时打开。BLE Direct 与 cloud 是否在线无关。

## 3. 安全语义纠正

`BleFrameTransport` 只提供 BBP/2 frame 校验和 4 B BLE 分片，不执行链路加密或对端认证。本批删除了它错误的
`TransportFeatureAuthenticated` capability。

Direct BLE 的真实准入条件是：

~~~text
Noise enrollment or Method 2 authenticates the application peer
  + DirectSecureRecord verifies AEAD and replay sequence
  + session ACL authorizes this BBP/2 operation
  = local command/state access
~~~

- transport capability、`BleSessionInfo.encrypted/bonded/authenticated` 和 BBP/2 authorization 是三个不同事实；
- WiFi+BLE lifecycle 显式向同一个 Client 安装 `IAuthorizationProvider`；
- Runtime 对 local 业务 frame 继续强制应用层 `encrypted=true`；该事实来自已验证的
  DirectSecureRecord，不由 GATT 是否启用 SMP 推导；
- MQTT 的 authenticated 状态仍来自 Cloud Session/MQTT adapter，不受 BLE mode 切换影响。

ESP32 NimBLE 仍可观察连接对象报告的 encrypted/bonded 状态，但官方基线不主动请求 SMP/bonding。R4b-5C
也关闭了 UNO R4 的软件缺口：通用
ArduinoBLE adapter 已删除 `setSessionSecurity()`，禁止调用 any-peer `BLE.paired()`；要求加密却没有
`IArduinoBleSecuritySource` 时直接 fail closed。独立 `renesas-uno-ble` adapter 固定 ArduinoBLE 2.0.2，
只把当前 central 映射为 ATT connection handle，并读取该 handle 的 controller encryption-complete 位。
bonded 仍因缺少 per-connection 事实而报告 false，Controller Method 2 继续负责 Device V2 身份授权。

这曾使 UNO R4 进入 internal official composition 和资源/CI 门禁；该双无线图现已退出。逐连接加密事实源
继续供独立普通 BLE 使用，其真机 pairing、加密丢失断开及 heap/stack watermark 仍须验证。

## 4. 存储与包边界

- ESP32 internal official composition 已为 Controller table 和 BSG2 分别使用 NVS
  `bl_v2_ctl/table` 与 `bl_v2_setup/saga`，不与 DeviceInstanceId、Ownership、Claim、Cloud、Enrollment 或
  WiFi credential 共用 namespace/key。`begin()` 任一步失败会关闭此前打开的所有 handle。
- WiFi-only 与 WiFi+BLE 不复制 Cloud/WiFi 对象图：二者共用 `WifiCloudStack<Platform>`，其中只存在一个
  platform、DeviceInstanceId、Claim/Cloud coordinator、WiFi lifecycle、control-plane arena、HTTP client、
  PubSubClient adapter 和 Managed MQTT transport。WiFi+BLE 只在其上增加 Controller/BSG2/BLE 对象。
- Renesas Preferences bank 已在原有 BlobId 之后追加 `BlobLocalSetupSaga`，key 为 `sg0/sg1`。旧 BlobId 与
  key 顺序不变；双槽 generation/CRC/tombstone 规则继续复用，最大 scratch 仍由 268 B Controller record
  决定，没有增加 adapter 栈数组上限。UNO R4 internal composition 直接复用同一 bank 的 `ct0/ct1` 与
  `sg0/sg1`，没有新增第二个 Preferences handle。
- LocalProvisioning/Noise endpoint 是否进入 release 只由当前 BLE-only/Enhanced 消费者决定；已删除的
  `WifiBleLifecycle` 不再构成保留理由。未使用代码仍由链接器裁剪，不把第三方 BLE SDK 塞入 core。

## 5. R4b-5A ～ R4b-5D 历史软件门禁

- 当时的 host 状态机曾覆盖：首次 Provisioning、Complete 时等待 session 断开、未完成断开原 profile 重启、
  Complete+confirmed 重启直接进入 Direct、Direct external transport 不被 Client 重复 poll；这些只为退休
  lifecycle 存在的测试现已删除，正式 `Esp32WifiBleProduct` 使用独立产品门禁。
- BLE mode 测试锁定普通 `BleFrameTransport` 不得声明 authenticated；
- Renesas storage adapter 覆盖完整 248 B BSG2、隔离、容量上界与既有双槽行为；
- Windows ASan host 36/36、200,000 次 deterministic fuzz、pre-enrollment exact verification、架构检查、
  C++/TypeScript/Node 全量 golden 与 15-package boundary 已通过；
- ESP32、ESP8266、nRF52840、RP2040、UNO R4、Wio Terminal 六个 Core 目标均生成 ELF；
- 本批受影响的 UNO R4 storage clean compile 为 68,660 B Flash / 7,744 B global RAM，Small SizeProbe 为
  52,332 B Flash / 14,668 B global RAM；
- ESP32/UNO R4、Plain/TLS 四种 official 聚合包结构与可复现 ZIP 已通过；Plain QuickStart 当前复验为
  ESP32 1,020,659 B Flash / 50,576 B global RAM，UNO R4 最近一次全源 clean compile 为
  159,148 B Flash / 13,360 B global RAM。
- R4b-5B 新增独立 `esp32-wifi-ble-internal` 聚合 profile，只包含内部
  `WifiBleCompositionProbe`，没有公开 `lifecycle(WiFiBleTag)`，也不会把 BLE 依赖带入 WiFi-only 包；两次构建
  ZIP digest 完全一致。
- R4b-5C 新增 `uno-r4-wifi-ble-internal` 聚合 profile，只包含内部
  `UnoR4WifiBleCompositionProbe`；ArduinoBLE 精确固定 2.0.2，WiFi-only 包仍不携带 ArduinoBLE。
  完整图实编为 210,804/262,144 B Flash、18,812/32,768 B global RAM，静态 singleton 为 9,008 B；
  剩余 51,340 B Flash / 13,956 B RAM。
- R4b-5D 在平台无关 record/lifecycle 层补齐 security-loss、活动 sessionId 冲突、未准入包、漏失 disconnect、
  启动失败回滚和 Direct link Error 门禁。没有增加公开 API、动态分配或常驻字段；当前完整图为 UNO R4
  211,332 B Flash / 18,812 B global RAM，ESP32 1,297,371 B Flash / 65,448 B global RAM。
- R4b-6B 当时删除了 `BasicWifiBleLifecycle` 内重复的 completion journal 读取、locator 生成、profile 构造、
  request 观察、session-disconnect 切换和 recovery restart，统一委托给 `BasicBleSetupLifecycle`。WiFi+BLE
  仍固定 `PlatformEnrollment + acceptsWifiConfig=true + LocalSetupSagaCompletion`，不会因复用生命周期而接受
  Local pairing。当时 32 位 `WifiBleLifecycle` 为 `52 B`；该类型现已删除。历史完整图为 ESP32
  `1,299,043/65,464 B`、UNO R4 `211,708/18,828 B`。
- R4b-6C 抽出共享 BLE workspace、ESP32 NimBLE radio、UNO R4 ArduinoBLE radio 与通用 Arduino
  clock，WiFi+BLE 和 Platform BLE/LocalBLE 共用这些无业务语义的底座。原 composition 内的第二份
  workspace/direct config 以及两平台 `.cpp` 内重复 link wrapper 已删除；Ticket/Ownership/Local authorization
  没有被放入共享层。clean regression 为 ESP32 `1,299,055/65,464 B`、UNO R4 `211,716/18,828 B`，
  相对 R4b-6B 仅增加 `12/0 B` 与 `8/0 B`。

ESP32 WiFi+BLE 的 BLE 栈与分区已按实测冻结：

| 组合 | Flash | global RAM | 结论 |
|---|---:|---:|---|
| 初始聚合图：NimBLE-Arduino 2.5.0 + 默认 1.25 MiB APP | 1,296,523 / 1,310,720 B | 65,544 B | 虽能链接但仅余 14,197 B，不作为发布分区 |
| 当前聚合图：NimBLE-Arduino 2.5.0 + Minimal SPIFFS | 1,299,055 / 1,966,080 B | 65,464 / 327,680 B | R4b-6C clean regression；余 667,025 B Flash 与 262,216 B RAM，保留 OTA |
| Arduino-ESP32 Core Bluedroid + 默认分区 | 1,795,395 / 1,310,720 B | 70,344 B | 136%，拒绝作为 official 底座 |

因此 official ESP32 WiFi+BLE 固定使用 NimBLE-Arduino 2.5.0，4 MB Flash 板选择 Arduino-ESP32 的
`Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)`。这不是用大分区掩盖实现错误：同一功能的 Bluedroid
实测多占约 499 KB，且默认分区根本无法链接；本产品只使用 NVS，不依赖大 SPIFFS。internal composition
singleton 的 ELF 静态对象为 10,752 B，其中 Provisioning/Direct 互斥阶段复用 record RX/TX/packet 数组，
没有复制第二个 Client/runtime。

Core `Esp32BleLink` 仍保留为非默认兼容适配器，但已经删除“业务代码可伪造 encrypted/bonded”的旧语义：
链路事实只取 ESP32 BLE 栈回调，且 encryption 与 notification subscription 均完成后才发布 Device V2
session。最新加固源码的独立 compile/link 探针为 1,662,323 B Flash / 59,360 B global RAM（默认分区
126%，资源门禁按预期拒绝）；它不得与 NimBLE owner 同时进入一个产品。

Arduino CLI 输出中的 invalid version directory、Renesas Core、Crypto 与 PubSubClient warning 来自本机已安装
SDK/外部依赖；本批目标退出码均为 0。上述数字是各自 sketch 的静态摘要，不是 WiFi+BLE 产品峰值。

这些结果只证明平台无关对象图和存储接缝，不能替代 GATT 配对、WiFi/BLE 共存、掉电、heap/stack、服务器/App
E2E 或 24 h 真机门禁。

## 6. R4b-5 后续顺序

1. R4b-5B 已完成：ESP32 internal official graph、独立持久域、共享 workspace、确定性聚合包、NimBLE 资源档
   与 CI profile 已落地；公开 `WiFiBleTag` 继续保持未绑定。
2. R4b-5C 的 UNO R4 exact-handle 加密事实源继续保留给普通 BLE；历史 WiFi+BLE graph/package 已删除，
   `WiFiBleTag` 永不绑定到 UNO R4。
3. R4b-5D 已完成：无真机可执行的 security-loss/session-admission/startup/direct-link fault gate 已落地；
   静态 RAM 未增加。动态 RAM 无法由 host/ELF 证明，真机到位后仍须补 radio coexistence、encryption/bonding、
   掉电、heap/stack 与长稳。
4. R4b-6A～6C 已完成 LocalBLE 独立 pairing、Noise application、共享 BLE setup lifecycle 与 BLE-only
   lifecycle；随后 ESP32/UNO R4 Platform BLE 已接回统一 `<Blinker.h>` facade，LocalBLE 仍为 internal。

HC-05/Bluetooth Classic SPP、ArduinoMqttClient 第二套 runtime 和 MCU 专属业务 API 均不进入本批。
