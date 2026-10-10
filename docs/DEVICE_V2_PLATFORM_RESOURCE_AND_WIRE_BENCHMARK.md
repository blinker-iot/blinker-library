# Device v2 多平台、资源与 400 B 心跳实施决策

> 状态：设备侧实施基线，2026-08-15 semantic API 修订
> 旧设备基线：`origin/dev_edu`
> 对照方案：`origin/dev_edu_tlv`（未发布测试实现）
> 新实现基线：当前标准根 `src/` BBP/2 + restricted CBOR

> 2026-09-10口径修订：下述404/174/94 B为历史组件/样式模型，不是所有当前语义消息的大小。
> 当前生产codec基准见 `D:/codes/aily-blinker-services/docs/DEVICE_V2_WIRE_SIZE_AND_ROUTING_REVIEW.md`：
> power Command设备13 B/App Route57 B；8路语义灯状态JSON222 B→设备33 B/App Delivery59 B。
> MQTT/安全记录另计，长文本不保证变小。外层Python脚本残留9 B分片头已改为当前4 B。
> Broker跨shard路由与BLE按ATT分片是两层，不能混淆；本批不改变设备wire或资源配置。

> 2026-08-16 说明：本文引用的 `WifiBleProductProbe` / `Esp32SecureProductProbe` 数值属于旧
> Enrollment/1 组合的历史资源样本；两个 probe 和对应 CI target 已删除。协议大小、紧凑槽、分页与
> 4 B BLE 分片结论仍有效，当前产品编译数字以 `DEVICE_V2_PACKAGING_CI_AND_PLATFORM_MATRIX.md` 为准。

> 2026-08-16 R4b-4a 增量：EnrollmentTicket canonical body 最大 268 B、签名 transcript/caller workspace
> 最大 216 B。UNO R4 Core 1.5.3 Small SizeProbe clean compile 为 52,268 B Flash / 13,632 B global RAM；
> ELF 对象精确为 Ticket 104 B、VerificationContext 72 B、Verifier 4 B。SizeProbe 故意让三者的
> `sizeof` 数组同时常驻，不能把这 180 B 全部当作最终 product 常驻；真正 coordinator 仍必须复用 setup
> application workspace。现有 NNpsk0 大 buffer 不因 Ticket 合同而成为普通 DIY 默认路径。

> 2026-08-16 R4b-4c 增量：LocalSetup request/response/body/workspace 上限为 529/273/512/512 B；
> workspace 是 caller-owned 256 B scratch + 256 B response-body storage，完整 Ticket 不驻留。UNO R4
> Small SizeProbe clean compile 为 52,284 B Flash / 14,384 B global RAM，32 位
> `LocalSetupApplication` 精确 216 B；Host x64 为 240 B。最终 Windows ASan 35/35 与 200,000 次
> deterministic fuzz 通过。该静态 probe 仍不包含 BLE/WiFi SDK runtime heap/stack。

## 1. 最终结论

1. **不移植当前 `dev_edu_tlv` 实现**。可以保留它验证过的“类型化值、schema、整数 field ID”思想，但不能保留“先生成 JSON、运行时学习 schema、再转 TLV”的实现路径。
2. **正式设备协议继续采用 BBP/2 固定帧 + 受限 CBOR**。Keyed 模式是无需字典的兼容/诊断基线；连接建立并确认同一 Manifest 后，7～8 组件的状态上报默认使用整数 field ID。
3. **不要把约 400 B 全组件 JSON 当作周期心跳继续发送**。MQTT keepalive/BLE link state负责链路存活；设备只发送变化字段或到期遥测，完整状态只在连接恢复、`StateRequest` 或一致性校验时发送。
4. **页面仍归 App/服务器所有**。设备 Manifest 只描述 Property/Action/Event、类型、访问权限和约束；布局、文案、图标和颜色不进入设备 Manifest。设备应上报语义状态（如 severity），由 App 映射样式。
5. **协议核心可多平台复用，Manifest 流式 fingerprint + 分页、紧凑静态 descriptor、packed state arena 已完成软件基线**；正式 semantic API 修订后仍必须重跑 ESP32、UNO R4、Wio 编译/ABI，历史编译数字不能自动继承。
6. **WiFi 与 BLE 共用一个 Core/StateStore，但每条连接保留独立 session**。既有 ESP32 composition/分区结果是修订前参考；当前仍需重新编译，并完成 RF/heap/阻塞/重连/24 h 真机门禁。UNO R4和Wio也不能把“两个库都能单独运行”写成“已验证可同时运行”。
7. **不改用 FlatBuffers**。Protobuf/nanopb 适合固定产品 schema，但对用户运行时自定义 key 并不天然优于“CBOR + Manifest/ID”；将它作为第二套主协议会重复维护帧、协商、鉴权、状态版本和 BLE 分片。

## 2. 400 B、8 组件的可复现实测模型

基准脚本为仓库外层的 `BLINKER_PROTOCOL_SIZE_BENCHMARK.py`。它使用 8 个组件、18 个动态 wire field 的 404 B 紧凑 UTF-8 JSON，包括按钮状态/样式、温湿度、滑条、RGB、状态文本和 `millis`。

统计包含：

- JSON：无空白 UTF-8 JSON；
- 当前 TLV：`origin/dev_edu_tlv` 的 3 B frame header、field/type/value 和 bool pack 规则；
- BBP/2：已实现的 10 B frame header，以及 PATCH `{0:mode, 1:revision, 2:values}`；
- ID 模式：当前真实实现，即 HELLO 缓存精确匹配或 ManifestAccept 确认完整 32 B SHA-256，数据帧内**不再重复 fingerprint/epoch**；
- 不包含 MQTT、WebSocket、TLS 和链路层开销；BLE 另行计算。

### 2.1 同语义全量快照

| 编码 | 单次字节 | 相对 404 B JSON | 说明 |
|---|---:|---:|---|
| Legacy compact JSON | 404 B | 基线 | 每次重复 key、属性名和静态展示值 |
| 当前 TLV 稳态 data | 187 B | 缩小 53.7% | 已经同步 schema 后；当前实现不支持 BLE/UNO R4 |
| 当前 TLV 第一次 | 835 B | 增大 106.7% | 187 B data + 648 B JSON schema |
| BBP/2 Keyed PATCH | 392 B | 缩小 3.0% | stable field text key 可自描述，但固定帧/版本开销抵消多数 CBOR 收益 |
| BBP/2 ID PATCH | 174 B | 缩小 56.9% | Manifest 确认后，整数 field ID；比当前 TLV 稳态再小 13 B |

结论不是“CBOR 自动比 JSON 小一半”。真正的压缩来自：

- field ID 不重复文本 key；
- `on/off` 规范化为 bool；
- 数字按数值编码，而不是文本；
- unit 等静态约束只出现在 Manifest；
- 多个变化合并进一个 frame，摊薄 10 B 帧头和 PATCH envelope。

Keyed 模式的价值是**无需字典仍能恢复字段身份**，不是极限压缩。高频 7～8 组件上报应在协商完成后使用 ID 模式。

### 2.2 实际应发送的动态心跳

把页面默认图标、默认标题、默认颜色移回 App Page Layout，把 unit 移入 Manifest 后，同一 8 组件仍保留开关、数值、RGB、动态状态文案/颜色和 `millis`：

| 编码 | 单次字节 | 相对原 404 B | 相对动态 JSON 246 B |
|---|---:|---:|---:|
| Legacy-shaped dynamic JSON | 246 B | 缩小 39.1% | 基线 |
| BBP/2 Keyed PATCH | 213 B | 缩小 47.3% | 缩小 13.4% |
| BBP/2 ID PATCH | 94 B | 缩小 76.7% | 缩小 61.8% |

更进一步，链路 heartbeat 不应携带这 94 B 全量状态：

~~~text
MQTT PING / BLE connection event     -> 只判断链路存活
本地状态发生变化                      -> PATCH Report(diff)
周期传感器到期                        -> 合并后的 PATCH Report(batch)
App/服务器重连或主动校验              -> StateRequest -> full snapshot
页面默认样式/布局                      -> 不进入设备状态上报
~~~

如果每次只有一个极小字段，BBP/2 固定帧可能比 JSON 大，例如 `{"switch":"on"}` 为 15 B，而 Keyed BBP/2 为 25 B、ID 为 19 B。因此设备应在短时间窗内合并遥测，而不是为追求“二进制”发送更碎的包。

### 2.3 schema/Manifest 的摊销

上述 8 组件：

| 元数据 | 大小 | 生命周期 |
|---|---:|---|
| 当前 TLV JSON schema | 648 B | schema 变化或对端丢失后重新发送 |
| BBP/2 Manifest frame（旧嵌套模型） | 588 B | 修正前的单元素 properties 嵌套模型 |
| BBP/2 Manifest frame（当前平坦 FieldSpec） | 439 B | 当前最小约束模型；按 fingerprint 缓存，不随心跳重复 |

按“元数据一次 + 原全量语义每次”计算，TLV 和 BBP/2 ID 都在第 3 次 404 B 上报时超过 JSON 的累计收益。若采用 94 B 动态 ID 上报，它相对旧 404 B 心跳在第 2 次已经回本；相对同样优化过的 246 B JSON 在第 4 次回本。HELLO 和登录控制面不计入此表，因为各协议连接本身都有控制开销。

平坦 FieldSpec 已把该代表模型从 588 B 降到 439 B，但这没有取消分页需求。当前实现已改为**流式 canonical encoder + SHA-256 sink + Manifest 分页**：fingerprint 不再写完整 scratch，page 受每 session remote frame cap 约束并按完整 FieldSpec 切分。

## 3. 当前 TLV 的实际优点与不足

### 3.1 优点

- 稳态 field ID 对多组件数据非常有效，本例 404 B 降到 187 B；
- 类型码简单，bool 连续字段可 pack；
- 对 MCU 的纯数据帧解码逻辑可以很小；
- schema 能让 App/服务器把 session ID 还原成 stable Property/Action/Event key。

### 3.2 不能进入正式方案的原因

- 设备发送仍先走旧 Widget JSON 聚合；`learnJsonFields()` 使用 `DynamicJsonDocument(2048)`，`encodeTlvFromJson()` 又使用 `DynamicJsonDocument(1024)`；接收 TLV 后还会转回 JSON 再调用旧 callback。
- schema 由首次/实际发送的 JSON 运行时学习，field ID 和类型被发送顺序及“最先出现了哪些字段”影响；它不是固件启动时已经封存的能力契约。
- 当前代码明确只为 ESP32/ESP8266 WiFi 开启 TLV；UNO R4 与 BLE 保持 JSON。它不是平台无关 codec，也没有统一 BLE 二进制分片。
- 固定表上限为 32 var、64 JSON field，名称/属性/unit 使用多组定长 char array；扩容会线性增加每台设备 RAM。
- 当前 32 位布局模型中 `BlinkerTlvCodec` 约 9,400 B；另有约 1 KB 旧发送 buffer、原 JSON `String`、1～2 KB动态 JSON heap，以及 896 B frame 或 1,216 B schema 临时栈缓冲。若原样移植到 32 KB SRAM 的 UNO R4，仅 codec 就约占 29%。
- schema 自身仍是 JSON。本例第一次 835 B，前三次才开始优于累计 JSON；BLE 若再分片会放大首次连接延迟。
- 它是 Blinker 私有格式；嵌套类型、未知字段、canonical 编码、跨语言验证和工具生态都需要自行长期维护。

因此应保留“显式 schema + ID”而不是保留这份 TLV codec。

## 4. 当前 BBP/2 的资源实测

### 4.1 编译结果

`CompileV2` 实际调用 restricted CBOR、Manifest、SHA-256/HMAC、登录 JSON、HTTP 控制面、MQTT 抽象、可靠交付和 Runtime。表中 delta 是相对同一 Core 的空 Sketch：

| 平台 | 空 Sketch Flash / static RAM | CompileV2 Flash / static RAM | 增量 |
|---|---:|---:|---:|
| UNO R4 WiFi / Small | 51,792 / 6,740 B | 103,036 / 8,604 B | +51,244 / +1,864 B |
| ESP32 / Standard | 281,175 / 20,760 B | 344,803 / 22,616 B | +63,628 / +1,856 B |
| Wio Terminal SAMD51 / Standard | 30,956 / 3,424 B | 82,540 / 5,284 B | +51,584 / +1,860 B |

上表是 2026-08-14 旧 typed-widget 批次的历史 CompileV2 数据，不是正式 semantic API 当前结果。
它证明 Core 可跨 ESP/Renesas/SAMD 编译；Widget/Catalog 代码已经删除。2026-08-15 原 CorePortable
结果见 4.4 节，Wio 的网络/BLE adapter 仍需按实际硬件单独实现和验证。

restricted CBOR + HELLO + Keyed writer + Registry + Manifest 本身的已记录增量更小：UNO R4 为 +4,152 B Flash / +308 B static RAM，ESP32 为 +6,176 B / +328 B。全功能增量主要来自控制面、安全、可靠交付和 Runtime，而不是 CBOR codec。

这些是编译静态量，不等于峰值 RAM。外部 WiFi/MQTT/BLE 库的 heap、TLS buffer、任务栈、调用栈，以及调用者选择的 frame/reassembly/outbox/state arenas 都必须另外计入。

### 4.2 P1-4 双 transport 产品组合静态门禁

本轮新增 `WifiBleProductProbe`，同一固件实际链接一个 `Client`、PubSubClient、TLS client、
`MqttFrameTransport`、`BleFrameTransport` 和目标平台的真实 BLE adapter。默认不联网，因此下表只证明
源码、依赖和链接组合，不证明 broker、RF、协处理器、heap/stack 或 24 h 稳定性：

| 目标/组合 | Flash | `.data + .bss` | 静态判定 |
|---|---:|---:|---|
| UNO R4 / 直接测试凭证数据面 / WiFiSSLClient + PubSubClient + ArduinoBLE | 135,584 / 262,144 B（51%） | 12,180 / 32,768 B（37%） | 只作对照，不代表生产登录路径 |
| UNO R4 / **完整登录路径** / Arduino Client HTTP + Managed MQTT + ArduinoBLE / Small / Arduino CLI | 145,296 / 262,144 B（55%） | 14,108 / 32,768 B（43%） | aily为145,288/14,108 B；低于16 KiB static门禁2,276 B；运行时仍为实验能力 |
| ESP32 / 直接测试凭证数据面 / Arduino-ESP32 Bluedroid BLE | 1,777,971 / 1,310,720 B（135%） | 65,240 / 327,680 B（19%） | **默认 4 MB 双 OTA 分区链接失败**；不得作为默认产品路径 |
| ESP32 / 直接测试凭证数据面 / NimBLE-Arduino 2.5.0 | 1,288,263 / 1,310,720 B（98%） | 60,456 / 327,680 B（18%） | 只作 NimBLE 对照 |
| ESP32 / **完整登录路径** / Arduino Client HTTP + Managed MQTT + NimBLE | 1,298,915 / 1,310,720 B（99%） | 62,440 / 327,680 B（19%） | 仅余 11,805 B Flash；**链接通过但发布门禁不通过** |

P1-4a 把 product probe 的 cloud/BLE 选择提升为 build-wide composition，避免仅在 sketch
隐藏对象但仍链接依赖。安全闭环修正前的同源归因结果为：

| ESP32 3.3.1 / Small | Flash | static RAM | 归因 |
|---|---:|---:|---|
| cloud-only：HTTP login + TLS MQTT | 1,043,535 B | 49,280 B | 完整云路径 |
| BLE-only：NimBLE | 663,667 B | 38,840 B | 完整 BLE 路径 |
| dual | 1,298,971 B | 62,440 B | 两路径及共享项 |

由 inclusion/exclusion 得到共享基线 408,231 B、cloud 相对 BLE-only 增量 635,304 B、BLE
相对 cloud-only 增量 255,436 B。由此修正原“继续从 Device V2 裁约 120 KiB”的假设：协议和
Runtime 只有数十 KiB增量，主要体积来自完整 WiFi/TLS 与 BLE stack，不能通过删协议正确性或
安全能力解决。

以下数据来自当时启用 no-I/O LE Secure Connections + bonding 的历史 ESP32 NimBLE 归因构建，不能代表
2026-08-28 后的官方普通 GATT Port。当前安全根已经统一为 Noise + Method 2 + DirectSecureRecord，官方基线
不再为未使用的 SMP/bonding 路径付出资源；平台若启用 native link encryption，只作为纵深能力。历史
Standard-profile default NimBLE dual 为 1,299,355 B / 62,448 B；当时 build-wide compact profile 只删除
未使用 central/observer、复用已链接 mbedTLS 并把连接数限制为 1，历史结果为：

| 正式 ESP32 候选 | Flash | static RAM | 结果 |
|---|---:|---:|---|
| Small compact NimBLE + default 1,310,720 B slot | 1,277,391 B | 60,648 B | 仍仅余33,329 B（2.54%），不通过10%门禁 |
| Small compact NimBLE + 双 `0x180000` OTA slot / Arduino CLI | 1,277,391 / 1,572,864 B（81.21%） | 60,648 / 327,680 B（18.51%） | 余295,473 B（18.79%），通过静态门禁 |
| 同配置 / aily-builder | 1,277,567 / 1,572,864 B（81.23%） | 60,648 / 327,680 B（18.51%） | 多176 B `.flash.rodata`，余18.77%，仍通过 |

当时测试专用、现已删除的 BLE-only `AuthorizedPresencePreviewProbe` clean compile 为 690,475 B Flash、
38,564 B global RAM（XIAO ESP32-S3，ESP32 core 当时安装版本，default 8 MB partition）。该历史数字验证删除
未使用 SMP/bonding 生产路径后的纯 BLE 入口，但不包含 HTTP/MQTT/WiFi。2026-08-29 删除 Presence 预览宏后的
公开 `BleQuickStart` clean compile 为 692,943 B Flash、38,612 B global RAM；公开 `WiFiBleQuickStart` 为
1,283,647 B Flash、60,036 B global RAM。WiFiProv、Direct、Cloud 同时在线及 heap/stack 真机门禁已通过；更长
时间的 RF/平台稳定性仍属于发布级耐久门禁，不应与本次 profile 默认选择混为一项。

在相同Standard profile下，feature strip实际减少21,392 B Flash和440 B static RAM；section差值为flash text
15,896 B、rodata 5,224 B、IRAM 256 B、data 16 B、bss 424 B。正式分区仍是两个 OTA
slot，不采用取消 OTA 的 Huge APP；代价是 SPIFFS 从 `0x160000` 缩到 `0x0e0000`。产品若确实
需要超过896 KiB文件系统，必须重新做分区/外部存储决策，不能挤占静态余量。

版本基线为 Arduino-ESP32 3.3.1、Arduino Renesas UNO 1.5.3、PubSubClient 2.8、
ArduinoBLE 2.0.2 和 NimBLE-Arduino 2.5.0。ESP32 NimBLE 独立 adapter 编译探针为
608,519 B Flash / 35,664 B static RAM，说明超限/贴线来自完整 WiFi+TLS+BLE 产品组合，
不能只用 adapter-only 数字判定。

完整路径会先通过旧服务器兼容 HTTP JSON 登录取得动态 MQTT 凭证，再进入 BBP/2 MQTT 数据面；
没有把登录 JSON带入 BBP/2 frame。R4-Small 为该兼容边界显式使用 192 B request target、
512 B response、256 B credential arena 和紧凑 HTTP header 容量；Host test 锁定 16-byte
device/user/token/owner、7-byte product 与最长 125-byte normalized MQTT host恰好能放入
256 B credential arena。服务器若扩大字段，必须修改三端契约和资源门禁，不能依赖静默截断。

ESP32 新增的 NimBLE adapter 是隔离的可选 Arduino library；Core 不包含 NimBLE 头，也不强制
无 BLE 或其他 MCU 用户安装它。旧 Bluedroid adapter保留为兼容/诊断路径。当前自定义分区是基于
实际 stack 归因作出的显式产品决策，保留双 OTA；不得偷换为取消 OTA 的 Huge APP。

编译使用本地 `aily-builder` 1.3.0 完成 ESP32 全量及增量验证。ESP32 必须显式固定
`--sdk-path .../esp32/3.3.1`；否则存在多个 core 目录时工具可能选择另一版本。其依赖分析器不能直接解析
NimBLE 2.5.0 的 `#if (__has_include(...))`，通过构建宏
`NIMBLE_CPP_ARDUINO_STRING_AVAILABLE=1` 选择同一官方 Arduino 分支后使用未修改的依赖源码完成；
R4 四并发曾生成零头`core.a`；复用build path时，`aily-builder clean`也未清除旧
`core/main.cpp.o`，会与variant的`main.c.o`产生重复`main`。只有全新build path、单并发且禁用
archive cloud cache后完成真正Small的145,288/14,108 B干净构建；Arduino CLI独立结果为
145,296/14,108 B。此前
145,464/15,468 B实际使用了无效的`BLINKER_V2_PROFILE_SMALL`并落回Standard；产品探针现以
编译期断言拒绝非Small profile，校验器只接受真实`BLINKER_RESOURCE_PROFILE=1`。正式CI还要保留原版依赖的Arduino CLI
编译，不能让builder特定宏或不稳定缓存变成唯一发布依据。

本轮还确认 aily-builder 1.3.0 的两个报告/并发缺口：prebuild hook虽记录复制
sketch-local `build_opt.h`，但依赖分析与hook并行时最终文件可被置为0 B，因此真实Small
profile宏和compact宏必须同时通过`--build-macros` build-wide传入；工具生成并合并了自定义`partitions.csv`，但最终
size提示仍使用FQBN default的1,310,720 B上限。发布门禁必须校验实际分区表中的app slot并以
1,572,864 B比较，后续应修复builder本身；本批不修改其独立仓库。

### 4.3 24 field 配置暴露的 RAM 问题

以下是移除 WidgetCatalog 前最后一次 32 位 ABI `SizeProbe` 对比，用于证明紧凑存储方向；正式
semantic API 的精确值必须由当前 `SizeProbe` 重跑后更新，不能把本表“当前”列当作本次实测：

| 对象 | 修正前 | 当前 |
|---|---:|---:|
| `FieldSpec` | 嵌套 descriptor/property，未单独探针 | 16 B，application-owned const/ROM |
| `EndpointSlot` | 176 B / field | 8 B / field |
| `EndpointStateSlot` | 16 B / field，不含 value arena | 6 B / field，不含 value arena |
| `DeviceRuntime` | 844 B | 当前 Small/Standard/Large 296/540/1,356 B |
| `Client` | 1,000 B | 当前 Small/Standard/Large 448/704/1,532 B |
| `BleFrameTransport` | 9 B 原型未单独保留 32 位探针 | 当前 Small/Standard/Large 160/240/368 B |
| `NoiseNnSession` | 未实现 | 共享 NN/NNpsk0；UNO R4 32 位 272 B，64 位 Host 280 B |
| `BleNoiseProvisioningChannel` | 未实现 | 当前 shared NN/NNpsk0 通道 520 B；大 buffer 均由 caller 持有 |
| `LocalSetupApplication` | 未实现 | UNO R4 32 位 216 B、Host x64 240 B；另需 caller-owned 512 B operation workspace + 273 B response storage |
| `ClientStorage<24,256,192,32>` | 5,920 B | 816 B（32 B 是总 arena） |
| Public `DeviceStorage` | 未实现 | Small 976 B；Standard 2,496 B；Large 9,088 B |
| Public owning `Device` | 未实现 | 当前 Small/Standard/Large 1,436/3,212/10,632 B |
| `Property/Action/Event runtime` | 未实现 | 当前 32 位实测 16/16/8 B；正式门禁相同 |
| `CloudCredential / CloudCredentialStore` | 旧模型未拆分 | 当前 32 位 108/8 B；持久 BCS2 固定 116 B |

修正前 24-field storage 加一个 `Client` 达 6,920 B，约占 UNO R4 32 KB SRAM 的 21.1%。上一轮
Advanced `ClientStorage<24,256,192,32>` 与 Client 合计为 Small 1,272 B、Standard 1,528 B；Public
Device Small 当前为 1,436 B。正式修订已删除 Catalog 指针和 widget runtime，字段 key/type/access/
constraints 仍不复制到 SRAM slot；精确新值以当前平台重跑为准。

descriptor/runtime 修正达到 `≤32 B/field` 元数据门禁：上一轮 `EndpointSlot + EndpointStateSlot =
14 B/field`，不含 value arena。`BleFrameTransport` 对象尺寸不包含 caller-owned RX/TX/packet scratch；
R4-Small 产品仍需计入这些外部 buffer。共享非等长 arena和 peer/transport/reliable/BLE queue上限已
实现；当前 Property/Action/Event 32 位 static_assert 已通过。2026-08-16 R4b-4c UNO R4 Small
SizeProbe 为 52,284 B Flash / 14,384 B static RAM；LocalSetup application 与 Noise
session/channel/preparation 的 ELF 精确值为 216/272/520/16 B。probe 为 ABI 回归故意同时实体化 channel
及其内含的 session，不能把总 RAM 当作产品峰值。
剩余资源工作是真机 SizeProbe 串口核对与真实 WiFi+BLE 峰值测量。

2026-08-15 原 CorePortable（现 `AdvancedCoreModel`）历史编译结果：

| 目标 | Flash | 全局/静态 RAM | 边界 |
|---|---:|---:|---|
| ESP32 Dev Module | 329,979 B / 25% | 24,000 B / 7% | Core/API；非 WiFi+BLE product |
| UNO R4 WiFi | 82,780 B / 32% | 9,984 B / 30% | 余 22,784 B 给 stack/heap |
| Wio Terminal | 61,764 B / 12% | 6,664 B (`data+bss`) | portable Core；不宣称 BLE |

### 4.4 必须实施的紧凑存储

当前已落地：

- descriptor、key 和 constraints 使用 `const` 表并允许进入 ROM，不复制字符串；Harvard MCU 通过平台 accessor 处理，不在 Core 硬编码 `PROGMEM`；
- Runtime callback slot为 8 B，连同 6 B state slot为 14 B/field；以后增加 metadata仍不得超过 32 B/field；
- scalar/text/bytes/container均保留 canonical CBOR bytes并紧密存入一个 caller-owned arena，不再强制 `N × StateValueSize`，也不维护第二份 typed shadow state；
- 多字段更新以最终总量做容量预检，再统一删除被替换值并追加新值；超限不改旧状态，“先扩后缩”最终可容纳即可成功；
- arena按“同时保留值的 canonical CBOR总量”预算：bool 1 B、float32 5 B、float64/最坏整数 9 B，text/bytes再加 CBOR长度头；`current()` view只借用 arena，任何状态更新前都必须解码或复制；
- 代价是一次 k-field替换最坏需要多次 `memmove`，约为 `O(k × arena)`；Small/R4 的几百字节权威状态可接受，大历史块/文件/日志不得塞入 StateStore，应走独立流式资源通道；
- Manifest encoder 直接遍历静态表；field ID 继续由封存 Manifest 数组位置 + 1 得到，fingerprint 绑定该顺序；
- session 不复制 32 B fingerprint，只保存归一化 remote frame limit/idReady；peer/replay数量由一个 build-wide profile统一决定；
- `StateUpdate` 当前仍使用显式 patch buffer；在没有完成 buffer借用/重入证明前，不把“删除 staging”误写成已完成；
- 动态 key 产品若需要复制，使用独立可选 key arena并明确预算。

24 field 的目标预算：descriptor/state bookkeeping（不含实际 text value）不超过 1 KB；一个 512 B frame、一个 512 B BLE reassembly、一个 192 B reliable slot和共享控制面 buffer均显式列入配置。

## 5. 多平台适用性

“Seeed Wio”不是单一 MCU，必须按具体板卡/无线芯片判断：

| 平台 | 当前 TLV | BBP/2 Core | WiFi/MQTT | BLE | 建议 profile / 结论 |
|---|---|---|---|---|---|
| ESP32/ESP32-S3/C3 | 仅当前特定 ESP WiFi 路径 | 已验证 ESP32 | 默认/参考为 PubSub adapter；没有需求不新增第二 MQTT adapter | NimBLE 为默认候选；Bluedroid 仅兼容/诊断 | 当前 ESP32 经典款完整登录组合占默认 app slot 99%；先减 Flash 并做 heap/RF soak，再扩展 S3/C3 矩阵 |
| UNO R4 WiFi / RA4M1 | 当前明确退回 JSON | 已验证 | WiFiS3 Client + PubSub adapter 已验证 | ArduinoBLE adapter 已验证编译，要求 ESP32-S3 connectivity firmware | Small；完整登录组合static RAM 14,108 B，24 h并发未过前仍标实验 |
| Wio Terminal / SAMD51 + RTL8720DN | 旧库顶层不支持 | 本次已编译验证 | 需要 `rpcWiFi`/Client adapter contract | 需要 `rpcBLE`/`IBleLink` adapter | Standard；192 KB RAM足够，协处理器并发能力仍需验证 |
| Seeed XIAO ESP32 系列 | 视为 ESP | 与 ESP profile 相同 | ESP adapter | ESP adapter | 不新建协议 fork |
| Seeed XIAO nRF52840 | 不支持 | 平台无关核心应可复用，待 CI 编译 | 无原生 WiFi；需要外接网络/网关 | 适合 `IBleLink` adapter | BLE Standard；不为了“MQTT”在核心写板级特例 |
| RP2040/Pico W | 不支持 | 应可复用，待 CI | Arduino Client adapter | 依具体板/Core | Standard；补 clock/random/storage/无线 adapter |
| Wio-E5/STM32WL | 不适用 | codec 可复用 | 设备本身不是 MQTT/WS 终端 | 非 BLE 主链路 | 需要 LoRaWAN constrained transport 与更小 ID batch，不等同 Wio Terminal |
| ESP8266 | 当前 WiFi TLV 路径 | 待 CI | MQTT/WS 可适配 | 不适用 | Small/Standard，重点测 heap/TLS |

官方硬件约束：UNO R4 的应用 MCU 是 48 MHz RA4M1、32 KB SRAM；板载 ESP32-S3 的 512 KB SRAM不属于 RA4M1 Sketch 可分配内存。[Arduino UNO R4 WiFi](https://docs.arduino.cc/hardware/uno-r4-wifi) / [datasheet](https://docs.arduino.cc/resources/datasheets/ABX00087-datasheet.pdf)。Wio Terminal 为 120 MHz SAMD51、192 KB RAM，并用 RTL8720DN 作为无线核心。[Seeed Wio Terminal](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)；其 BLE 需要独立 rpcBLE 软件栈。[Seeed BLE overview](https://wiki.seeedstudio.com/Wio-Terminal-Bluetooth-Overview/)。

ESP32 的 WiFi/BLE 共存是官方支持场景，但共享一个 2.4 GHz RF，通过时分复用，吞吐和扫描窗口会互相影响；不能按两条独立无线链路估算性能。[Espressif RF coexistence](https://docs.espressif.com/projects/esp-idf/en/v5.2-rc1/esp32/api-guides/coexist.html)。ArduinoBLE 已正式列出 UNO R4 支持，但官方说明本身没有证明 WiFi + BLE 的 Blinker 双业务并发；该结论必须由本项目实测给出。[ArduinoBLE](https://github.com/arduino-libraries/ArduinoBLE)。

## 6. BLE/WiFi 同时支持的设备流程

~~~text
                         one EndpointStateStore + one stateRevision
                                      |
                 +--------------------+--------------------+
                 |                                         |
          MQTT session A                            BLE session B
      HELLO/auth/ID mapping A                   HELLO/auth/ID mapping B
                 |                                         |
       cloud report / command                    local report / command
                 +--------------------+--------------------+
                                      |
                       one business transaction/callback
~~~

规则：

1. 一个 transport 可有多个 session；协商、鉴权、sequence、可靠窗口和 ID 能力按 session 保存。
2. BLE 命令只执行一次，ACK 回 BLE 来源；硬件状态提交后使用同一 revision 向云端和其他已授权 peer 报告。
3. 不把某个 session 的原始 ID body直接广播，因为另一个 session 可能没有确认同一 Manifest；安全默认是重新编码 Keyed snapshot，或确认 fingerprint 一致后按目标 session 编码 ID。
4. 云端上报设 primary-uplink policy，避免 WiFi 和 BLE gateway 同时把同一 telemetry 重复送云；本地 BLE 状态同步不等于第二云上行。
5. TransportHub、Registry、Manifest、StateStore 只保留一份；每条链路只增加必要的 RX/reassembly/queue。
6. 平台不支持真正并发时，仍使用同一协议和两个 adapter，但产品策略切换 active transport，不复制协议实现。

## 7. BLE 4 B 分片已完成并冻结

旧 BBP/2 BLE 原型 header 为 9 B：magic、version、flags、16-bit frameId、totalLength、offset。
P0-3 已将其替换为 4 B session-local header；ATT payload 20 B 时每片可承载 16 B。

| 8 组件 frame | frame 大小 | 旧 9 B / ATT20 | 当前 4 B / ATT20 | 当前 4 B / ATT244 |
|---|---:|---:|---:|---:|
| full Keyed | 392 B | 36 片 | 25 片 | 2 片 |
| full ID | 174 B | 16 片 | 11 片 | 1 片 |
| dynamic Keyed | 213 B | 20 片 | 14 片 | 1 片 |
| dynamic ID | 94 B | 9 片 | 6 片 | 1 片 |

已冻结格式：

~~~text
[magic][version + START/END flags][frameId8][fragmentIndex8]
~~~

- 每 session、每方向同时只允许一个 wire frame；
- 首片至少包含完整 10 B BBP header，total length 直接由 BBP header 得出，不在每片重复；
- 固定本帧 payload capacity P，offset=`index*P`；MTU 增大保持 P，MTU 下降换新 frameId
  从 index 0 重启；
- 8-bit index 取值 0..255，共 256 片；ATT20 下 `256 × 16 = 4096 B`，作为 Large profile 精确上限；需要更大 frame 或更强回绕隔离时评估 5 B/frameId16；
- frameId 为 direction-local `1..255,1..`；严格顺序、错误 stride、timeout、disconnect
  clear、长度上限和 backpressure 已通过 Host vectors。

同时主动协商较大 MTU（典型 ATT payload 244），但绝不能把大 MTU 当作正确性的前提。默认 20 B 仍必须通过所有 golden/error vectors。

### 7.1 DirectSecureRecord 加密后的 ATT20 实测

G0C 不再沿用“只计算裸 BBP/2”的片数。`DirectSecureRecord v1` 固定增加 24 B：8 B outer header
与 16 B AES-GCM tag；ATT payload 20 B、4 B fragment header 时每片仍只有 16 B record payload。
跨语言 fixture 使用真实且可解析的单字段 Command、Ack 与单字段 Patch：

| 消息 | 裸 BBP/2 | 裸片数 | secure record | secure 片数 |
|---|---:|---:|---:|---:|
| Keyed bool Command | 17 B | 2 | 41 B | 3 |
| Ack | 15 B | 1 | 39 B | 3 |
| Keyed bool Patch | 23 B | 2 | 47 B | 3 |

因此 24 B 安全开销不能在 BLE 预算中忽略，尤其 15 B ACK 会从 1 片变为 3 片；但它不应成为关闭
应用层 AEAD 的理由。正确策略是批量 Patch、避免无意义 ACK/高频 loop 发送，并尽量协商较大 MTU，
同时始终保留 ATT20 正确性。精确 KDF/AAD/双向 record 与 Command 三片数据冻结在
`tests/golden/direct_secure_golden_vectors.properties`，C++、浏览器安全 TypeScript 和 Node/OpenSSL
使用同一份 data-only fixture，任一端不得自行解释字段顺序。

G0C 的 32 位 Small SizeProbe 在 ESP32、UNO R4 与 Wio Terminal 得到完全相同 ABI：

| Small SizeProbe clean compile | Flash | static/global RAM |
|---|---:|---:|
| ESP32 3.3.1 | 281,723 B | 28,208 B（Arduino memory regions） |
| UNO R4 1.5.3 | 52,424 B | 14,968 B（Arduino memory regions） |
| Wio Terminal 1.8.5 | 31,292 B | 10,876 B（ELF `.data + .bss`） |

这些数字故意让所有 ABI probe 数组同时常驻，只用于回归比较，不等于任何一个 WiFi/BLE product
的常驻 RAM；产品资源仍以正式 product compile 和真机峰值为准。

| 对象 | Small 32 位精确大小 |
|---|---:|
| `DirectSecureKeyMaterial` | 96 B |
| `DirectSecureSession` | 108 B |
| `BleFrameTransport`（1 session） | 320 B |
| `DeviceRuntime` | 404 B |
| `ClientStorage<24,256,192,32>` | 824 B |
| `Device` | 1,664 B |

其中 `BleFrameTransport` 的 320 B 包含原 160 B record transport、一个 116 B secure slot 及外层
回调/引用；RX、TX、packet 和 plaintext buffer 仍由 product workspace 持有，未隐藏到对象或 heap。
Standard（2 session）与 Large（4 session）的 transport 精确为 520 B 与 888 B。派生实现已改成
流式 transcript hash，并让 HKDF 直接写 96 B key material，删除原 61 B transcript 与 96 B material
临时栈副本。

## 8. CBOR、Protobuf/nanopb、FlatBuffers 的选择

| 方案 | 400 B 动态 key 场景 | 小 MCU RAM | schema/演进 | 结论 |
|---|---|---|---|---|
| 当前私有 TLV | 稳态很小 | 当前实现很差；纯重写可很好 | 需自建所有跨语言规范和工具 | 不保留实现 |
| restricted CBOR + Manifest/ID | Keyed 灵活，ID 与 TLV 同级 | caller-buffer、无 DOM，可控 | 标准类型系统；Manifest 处理产品字段 | **主方案** |
| Protobuf + nanopb | 固定、编译期 field number 时可能略小；动态 map 会重复 key/union tag | nanopb 可静态分配 | `.proto`/代码生成强；DIY 动态 key 仍需另做 Manifest/ID | 固定产品可做实验，不作统一主协议 |
| FlatBuffers | 小包的 offset/vtable/alignment 开销不占优 | builder 要连续工作区，32 KB MCU收益低 | 固定表结构较好 | 不采用 |
| 全新 custom TLV v2 | 可比 CBOR ID 再省少数字节 | 可以极小 | 需要重新定义 nested、canonical、未知字段、安全解析和三端工具 | 只有 LoRa/极端带宽 profile另行评估 |

Protobuf 不会替代 BBP/2 外层的 HELLO、Manifest、revision、ACK/ERROR、鉴权、可靠交付和 BLE 分片。若使用运行时自定义用户 key，它同样需要 `key + typed value` 或字典；若使用真正的 proto field number，则每种产品/固件要生成并同步 schema。nanopb 是优秀的固定 schema MCU codec，但不是当前 DIY 页面/key 模型的自动答案。

FlatBuffers 的优势是大型已构建 buffer 的随机访问和 zero-copy；Blinker 的 20～400 B 增量状态通常只顺序读一次，vtable/offset 和 builder 工作区没有回报。

## 9. 正式设备发送模型

启动和连接流程冻结为：

1. 固件用静态 descriptor 与 Property/Action/Event API 注册 fields；页面布局不在设备端。
2. Registry seal；流式计算 canonical Manifest SHA-256，不生成完整 Manifest 临时 DOM/buffer。
3. 每个 MQTT/BLE/WS session 交换 HELLO：版本、能力、max frame/reassembly、Manifest revision/fingerprint。
4. 对端有 fingerprint 缓存：直接协商 ID；没有缓存：请求 Manifest pages，验证完整 fingerprint 后启用 ID。
5. `StateRequest` 返回同 revision 的完整当前动态状态；若单帧放不下，按字段边界分页，而不是扩大所有常驻 buffer。
6. 之后只发送 diff/batched telemetry；静态 UI defaults 不重复发送。
7. App 用 Manifest stable key/当前 session ID 更新数据模型，再由页面 binding 驱动一个或多个组件。

CBOR body 只存在于调用者提供的 TX buffer或 transport RX/reassembly buffer中；decoder 用 `ByteView` 读取。需要跨消息保留的仅是每个 field 的当前有界 CBOR scalar/value，存放在 `EndpointStateStore`。Manifest descriptor 优先放 Flash，Page Layout 始终在 App/服务器。

## 10. 建议资源 profile

| Profile | Fields | Frame | Patch | 共享 state arena | Transports | 32 位 Public Device | 适用/约束 |
|---|---:|---:|---:|---:|---:|---:|---|
| Small | 24 | 256 B | 192 B | 192 B | 2 | 1,436 B | ID常态的小节点；Keyed/Manifest/State必须分页 |
| Standard（默认） | 32 | 1,024 B | 512 B | 512 B | 3 | 3,212 B | SAMD51/RP2040/nRF52840/普通ESP32 |
| Large | 64 | 4,096 B | 2,048 B | 2,048 B | 4 | 10,632 B | ESP32大批次；不要作为小板默认 |

UNO R4需要容纳392 B完整Keyed帧的产品可使用Small对象布局并以build-wide
`BLINKER_DEVICE_FRAME_SIZE=512`覆盖frame；BLE reassembly、ReliableOutbox及原生adapter
buffer仍单独预算。Large在UNO R4上的成功编译只用于32位ABI探针，不是产品推荐。

对象布局上限由 `ResourceProfile.h` 统一：

| 编译 profile | Transport | Peer session | 每 peer可靠接收 replay | BLE RX session | BLE queued frame |
|---|---:|---:|---:|---:|---:|
| Small | 2 | 2 | 1 | 1 | 1 |
| Standard（默认） | 3 | 4 | 2 | 2 | 4 |
| Large | 4 | 8 | 4 | 4 | 8 |

R4-Small使用 Small对象布局；Public Device 需要512 B frame时用build-wide
`BLINKER_DEVICE_FRAME_SIZE=512`覆盖，Advanced Client则显式选择容量。ReliableOutbox发送队列仍由
调用者提供：R4默认一个 slot和一个 frame buffer。profile会改变类布局，必须使用全局编译参数
`-DBLINKER_RESOURCE_PROFILE=1/2/3`或修改唯一配置头默认值；只在 sketch单个翻译单元定义不合法。

无论 profile 多大，Manifest 都要分页。当前已删除完整 Manifest scratch；后续仍不得让 TX、PATCH staging、State page staging 各自复制一份同样大小的 buffer。

### 10.4 当前身份与 Cloud 对象资源边界

2026-08-16 UNO R4 32 位 ELF 的 active public/release 符号如下；旧 OperationalCredential、
ProvisioningCoordinator 与 ProductOrchestrator 已退出本 probe：

| 对象/记录 | 尺寸 | 生命周期/位置 |
|---|---:|---|
| `DeviceInstanceId` | 16 B | 公开实例 ID；不含 secret |
| `DeviceInstanceIdStore` | 8 B | interface vptr + storage 引用；持久 BDN2 固定 28 B |
| `CloudCredential` | 108 B | HMAC 登录时的短期栈对象，完成后清零 |
| `CloudCredentialStore` | 8 B | interface vptr + storage 引用；不复制 blob |
| Cloud BCS2 record | 116 B | Active 长期凭据的固定持久布局 |
| `OwnershipRecord/Store` | 104 / 8 B | Active 结果或 generation tombstone；BOS2 固定 112 B |
| `ControllerCredential` | 64 B（历史 ABI） | 单条短期栈对象；旧 standalone BCT2 已退出当前发布图 |
| `DeviceAccessStore` record | 260 B persistent；兼容读 176 B v1/196 B v2 | schema v3 原子保存 DeviceKey + accessEpoch + PresenceKey/version + Admin + 两个 Observe/Control 槽；设备不保存 Shared/Gateway role、owner/logical ID |
| `ControllerControlCoordinator` | 待本批 32 位重测 | 仅引用 DeviceInstanceId、IAccessEpochSource、store/verifier并持有 16 B nonce；不常驻 Grant/secret |
| Controller control workspace | 256 B caller-owned | Grant transcript 最大 156 B；完成后清零，可与其他互斥控制阶段复用 |
| `OwnershipClaimRecord/Store` | 36 / 8 B | transaction-only 栈对象/store；BOC2 journal 固定 48 B |
| `OwnershipClaimCoordinator` | 20 B | 32 位 ABI：五个依赖引用；request ID/nonce 已移入 durable journal，不常驻 Grant |
| Ownership claim workspace | 256 B caller-owned | request/grant/receipt/commit 最大 63/243/165/183 B；Grant transcript 最大 199 B |
| 旧 `EnrollmentKeyHttpExchange` | 36 B | 仅为待替换 `bek_` 软件探针；R4b-2F 的 ManualDeviceKey proof/Recovery 必须重新测量，不得把 36 B 当最终门禁 |
| `CloudEnrollmentRecord/Store` | 128 / 8 B | transaction-only 栈对象/store；BCE2 journal 固定 136 B |
| `CloudCredentialEnvelopeSealer` | 8 B | 通用 X25519/AES-GCM provider + pinned key source 引用 |
| `CloudCredentialEnrollmentCoordinator` | 32 B | 七个依赖引用 + 状态；不缓存 secret/journal/workspace |
| Cloud enrollment workspace | 256 B caller-owned | request/grant/receipt/commitAck 最大 228/248/190/190 B，可与其他互斥控制阶段复用 |
| `HttpCloudSessionProvider` | 248 B | 不含 caller-owned request/response/credential buffers |
| Public Small/Standard/Large `Device` | 1,436 / 3,212 / 10,632 B | 身份对象不塞入每字段槽 |

上述对象大小是当前 32 位 ABI，不是 wire ABI；跨平台用 `sizeof` probe 和门禁，持久互操作只认固定
BCS2/BOS2/BCT2 布局。Controller table 不把四条记录常驻 RAM，但 mutation 峰值仍需后续真机 stack watermark。
R4b-2 第二片后的全源码 `CompileV2` 为 ESP32 318,999/21,672 B、UNO R4 69,304/7,664 B、Wio Terminal
48,236 B Flash；Aily 本次实际解析 ESP32 Arduino core 3.3.0。UNO R4 Small active SizeProbe 加入
48 B coordinator 门禁后为 52,124/11,464 B。

R4b-2 Ownership Claim v1 后重跑为 ESP32 319,459/21,672 B、UNO R4 69,296/7,664 B、Wio Terminal
48,236 B Flash；UNO R4 Small SizeProbe 加入额外 56 B Claim coordinator 符号后为 52,124/11,520 B。
ELF `nm` 实测 Claim/Controller coordinator 分别为 56/48 B。

上述 56 B Claim coordinator 是 RAM-only request window 的历史实现。R4b-3 第一片改为 48 B `BOC2`
持久 journal 后，32 位 Claim record/store/coordinator 为 36/8/20 B，Key HTTP exchange 为 36 B；UNO R4
Small 为 52,228/11,880 B，Wio Core 为 48,236 B。该结果取代旧 Claim 对象资源结论。

R4b-2 Cloud Credential Enrollment v1 后重跑为 ESP32 320,967/21,672 B、UNO R4 69,296/7,664 B、
Wio Terminal 48,236 B；ESP32 Noise adapter 356,651/20,800 B。UNO R4 Small SizeProbe 为
52,148/11,696 B，ELF `nm` 实测 Cloud journal record/store/sealer/coordinator 为 128/8/8/32 B，较上一
probe 强制保留 176 B 静态对象；普通 `CompileV2` 不实例化 enrollment，故 UNO R4/Wio 常驻结果不变。
这些数字不包含尚未实现的 UNO R4/Wio 真实 X25519/AES-GCM adapter 和 profile HTTP/BLE driver；不能据此
宣布产品端首次注册资源已准入。

UNO R4 的门禁目标：

- 24 field 的 Blinker descriptor/state bookkeeping ≤ 1 KB（不含实际 text arena）；
- Blinker 自有常驻 RAM（含 512 B frame/reassembly、一个 outbox 和控制面 buffer，不含外部库）≤ 6 KB；
- 完整 WiFi + BLE 产品 Sketch 的 `.data + .bss` ≤ 16 KB，并在 24 h 并发测试中保留至少 8 KB 实测 stack/heap watermark；
- buffer overflow、Manifest page 缺失和 BLE backpressure 必须整帧失败，不允许静默丢字段。

### 10.5 BLE mode ownership增量

32位UNO R4 ELF与实际adapter编译结果：

| 对象/目标 | 当前值 | 说明 |
|---|---:|---|
| `ble::ModeProfile` | 12 B | profile按值复制，不含secret |
| `BleModeCoordinator` | 40 B gate | 含link、两个lifecycle、preparer、active owner/profile/state |
| Small `BleFrameTransport` | 160 B | 通用lifecycle后仍为单虚表 |
| UNO R4 Small SizeProbe | 52,092 B / 11,232 B | active 对象；probe故意实体化全部尺寸数组 |
| UNO R4 Standard SizeProbe | 52,100 B / 13,588 B | Public Device 3,212 B |
| UNO R4 Large SizeProbe | 52,100 B / 22,780 B | Public Device 10,632 B；不推荐用于 UNO R4 产品 |
| UNO R4 + ArduinoBLE mode adapter | 90,392 B / 9,740 B | Flash / static SRAM |
| Wio Terminal Core Small | 83,436 B Flash | 当前无rpcBLE adapter |
| ESP32 NimBLE mode adapter | 615,159 B / 35,688 B | 首选产品BLE stack，尚非完整产品composition |
| ESP32 Bluedroid mode adapter | 1,114,387 B / 40,464 B | 兼容/诊断路径 |

SizeProbe 故意把所有 `sizeof` 数组同时实体化，不能把 11,232 B 直接解释为产品常驻峰值。完整产品
还须加入新 setup/Ownership/Controller coordinator、crypto/application workspace、WiFi sink、TLS/MQTT和stack watermark。13 B Service Data与
31 B scan response不占常驻大buffer；ArduinoBLE adapter仅持有31 B scan-response数组，ESP32在start栈上编码。

## 11. 设备侧执行顺序

### P0：协议冻结前完成

1. 把本文 404/246 B 向量固化为 C++/TypeScript/Node golden vectors；修正文档中旧的“每帧 8 B fingerprint”草案，真实规则为 fingerprint 只在 HELLO/Manifest 会话确认。
2. 流式 Manifest encoder/SHA sink、Manifest 分页、ID gate、revision-consistent
   StateRequest/StatePage、4 B BLE header 和三端共享 golden 均已完成当前代码退出条件。
3. `StaticEndpointTable`/Flash descriptor、紧凑 runtime slots、共享 packed arena和资源 profile已完成；维持 ABI/原子更新门禁。
4. Property/Action/Event 直接注册独立 stable field 并直接写 CBOR；不得经过 JSON。
5. 4 B BLE fragment header 已冻结，ATT20/244、丢片、重复、乱序、wrong-stride、wrap、
   timeout、disconnect、MTU change和背压测试已完成。

### P1：平台和 transport 闭环

1. ESP32：含 HTTP 登录的 cloud-only/BLE-only/dual 历史归因与双 OTA 分区已完成；2026-08-28 已在 XIAO
   上通过普通 GATT + Noise/Method 2/DirectSecureRecord 真机闭环。旧 1,277,391 B / 60,648 B 是保留
   SMP/bonding 时的历史数值；当前 Port 需要重新记录 clean compile、free heap/stack watermark、重连、并发吞吐
   和长稳结果，不能把删除未使用原生配对误写为删除应用安全能力。
2. UNO R4：含HTTP登录的WiFiS3 + MQTT + ArduinoBLE Small产品探针由Arduino CLI测得145,296/14,108 B；下一步固定connectivity firmware版本，验证MQTT心跳、BLE command、断连重连和24 h watermark。未通过前产品能力表标为“实验/切换”，不虚报并发。
3. Wio Terminal：实现 rpcWiFi `Client`/MQTT adapter和 rpcBLE `IBleLink` adapter；先过独立 contract，再做共存测试。
4. 增加 ESP8266、XIAO nRF52840、RP2040 CI compile；只有缺失的 clock/random/storage/link 才新增 port，不复制协议。
5. PubSubClient 已冻结为默认/参考 adapter；P1-4 不接入 ArduinoMqttClient。只有 QoS 1/2 发布或入站 retain/QoS metadata 成为硬需求时，才以同一 `IMqttClient` contract 建立第二 adapter。

### P2：发布门禁

- host strict warnings、ASan/UBSan、frame/CBOR/BLE fuzz；
- 400 B/8组件、最大字段、最大文本、中文/emoji、断网 outbox、WiFi/BLE同时 command vectors；
- 每个平台保存 empty/core/full/dual-transport map和峰值 RAM；
- payload 目标：404 B full snapshot 的 ID frame ≤ 190 B，动态 heartbeat ID ≤ 110 B；
- R4 24-field/双 transport 通过资源门禁后，才把该 profile 标为正式支持。

## 12. 当前应立即修改的实现清单

| 当前实现 | 问题 | 设备侧动作 |
|---|---|---|
| `computeManifestFingerprint()` 先编码完整 Manifest | 已修正为流式 canonical sink/hash | 保持 exact cross-language vectors |
| `encodeManifest()` 只支持完整单帧 | 已新增 cursor page；代表模型仍为 439 B logical bytes | App/服务器补同一 page 状态机 |
| `EndpointSlot` 176 B | 已修正为 8 B；EndpointStateSlot 6 B；24-field ClientStorage 已由 5,920 B 降至 816 B | 保持 ABI 和 packed arena原子更新门禁 |
| `ClientStorage` 同时有 frame + patch buffer | 复制峰值仍存在 | 先设计 buffer借用/重入规则；在证明前不宣称已删除 staging |
| BLE fragment header 4 B | ATT20 每片 16 B；最大 256 片/4096 B | 已冻结并通过异常 vectors |
| 周期全量组件 JSON 心跳 | 重复静态页面字段 | link keepalive + diff/batch +按需 snapshot |
| Keyed 文本模式作为高频默认 | 400 B例仅缩小 3% | Manifest确认后 ID 默认，Keyed保底 |
| App 样式由设备每次上报 | 页面/状态职责混合 | 样式归 Page Layout；设备只上报 severity/mode 等业务语义 |

## 13. 判定

当前应该做的不是“把 TLV 扩展到 UNO/Wio”，也不是“把 CBOR 换成 Protobuf”。应当把已经实现的 BBP/2 架构完成到可发布状态：

~~~text
restricted CBOR 标准值编码
+ Manifest 描述字段能力
+ session-negotiated integer field ID
+ 页面/默认样式不进设备 Manifest 或状态心跳
+ 同一 Core/StateStore 上挂 MQTT、BLE、可选 WS
+ static descriptor、流式 Manifest、调用者缓冲
~~~

在404 B历史实例上，BBP/2 ID 模式稳态全量为174 B、动态状态为94 B；它达到私有TLV的压缩级别，
同时消除ESP专用、JSON中间层和BLE业务编码分叉。descriptor复制、完整Manifest scratch、固定大槽和
BLE 9 B分片头已修正；WidgetCatalog/typed Widget 则不是“已完成能力”，而是已从正式模型删除。
当前主要风险是本次多平台重跑、生产App/服务器codec/session，以及真实双无线heap/stack、同步阻塞、
重连和24 h稳定性。

## 20. UNO R4 official WiFi composition 冻结点（更新于 2026-08-16）

UNO R4 official WiFi 已不再使用易失或 pre-enrolled-only probe。新包复用与 ESP32 相同的
`WifiComposition<Platform>`，平台层只提供 `WiFiClient/WiFiSSLClient`、`RenesasUnoWifiStation`、硬件随机数
和 credential storage/crypto。目标普通 Sketch 仍只有 `<Blinker.h>`，DIY 入口为
`Blinker.begin(blinker::WiFi.onboard(ssid, password, deviceKey), ...)`；预注册入口为
`Blinker.begin(blinker::WiFi, ...)`。普通 Sketch 不自报 productId。ManualDeviceKey 授权首次 Claim 或全擦
Recovery；Active-first 重启读取持久 Ownership/CloudCredential 并忽略 Sketch 中的 Key，不把它用于运行登录。

RA4M1 Data Flash 实际为 8 KiB，Arduino EEPROM 没有分区保留合同且以 1 KiB 擦除块改写，因此 Blinker
不占用它。`RenesasUnoPreferencesBlobBank` 改用 WiFiS3 ESP32-S3 connectivity module 的独立
Preferences namespace。当前 bank 在保留 cloud/selector/wifi0/wifi1/ownership/controller 原 BlobId 0..5
不变的前提下，又为 deviceInstance/ownershipClaim/cloudEnrollment/ownershipRelease 追加四个逻辑域；十个
blob 各提供两个物理 key。
generation + CRC + tombstone + 写后读回提供软件 old-or-new 恢复，始终报告 `PlainFlash`；真实不同 key
故障隔离和 reset-during-write 仍需真机验证。

| 目标 | Flash | static RAM | 判定 |
|---|---:|---:|---|
| UNO R4 storage adapter | 68,636 B | 7,732 B | 十域 Preferences bank，含新增 `di/oc/ce/or` 双槽 |
| UNO R4 official Plain | 158,892 B | 13,348 B | 完整 Claim/Cloud/portable crypto；余 19,420 B |
| UNO R4 official TLS | 162,068 B | 13,380 B | 完整 TLS 组合；余 19,388 B |
| ESP32 storage adapter | 314,699 B | 21,360 B | 含独立 Ownership 与 `bl_v2_ctrl/table` |
| ESP32 official Plain | 1,017,199 B | 50,632 B | 完整 Claim/Cloud/native crypto |
| ESP32 official TLS | 1,103,471 B | 51,224 B | 默认 app 分区占 84% |

上述 TLS 数字是当前完整静态组合，但仍不能替代协处理器 TLS runtime、RA4M1 stack/heap 或 WiFiS3
固件兼容测试。平台无关 228 B BFB2 和 UNO R4 USB Serial 工厂草图现已通过 C++/Python
exact、模拟 transcript 与 core 1.5.3 编译；草图为 66,700 B Flash / 7,924 B static RAM。真实写入/掉电/
联网仍属硬件门禁；该时点 WiFi-only 软件主线刚到 R4b-3。当前 R4b-4a～4d 已完成 Ticket/Noise/
LocalSetup application/durable saga，下一片是 R4b-5 official composition，并继续禁止把 WiFi-only
composition 改名为 WiFi+BLE。

## 21. R4b-4d LocalSetup durable saga 资源（2026-08-17）

`LocalSetupSaga` 只持有八个依赖指针、一个虚表指针和授权会话状态；不内嵌 WiFi/Cloud secret、SDK 对象、
HTTP/MQTT client 或 protocol buffer。UNO R4 32 位 ELF 精确值：

| 对象 | 大小 | 说明 |
|---|---:|---|
| `LocalSetupApplication` | 216 B | 连接内 sequence/Ticket context，不持久化完整 Ticket |
| `LocalSetupSagaRecord` | 240 B | operation 时的 context/digest record；不是长期全局 singleton |
| `LocalSetupSagaStore` | 4 B | 一个 `IAtomicBlobStore` 引用 |
| `LocalSetupSaga` | 40 B | 平台无关 mutation coordinator |
| `BSG2` serialized record | 248 B flash | schema/state/context/digest/CRC，不含 raw secret |

Small SizeProbe 为 52,324 B Flash / 14,668 B global RAM；上一片为 52,284/14,384 B。+40 B Flash 来自
probe 引用，+284 B RAM 精确等于 240 + 4 + 40 三个新增测量数组。真实 composition 可以按生命周期把
240 B record 放在串行控制面 stack/workspace，并只常驻 40 B Saga + 4 B Store；不能把 probe 的 +284 B
直接记作产品 permanent RAM，也不能忽略 operation 调用期间的 stack 峰值。

本批同源 Core 已在 UNO R4 1.5.3、ESP32 3.3.1 与 Wio Terminal 生成 ELF。以上仍不覆盖 BLE host、WiFi
共存、crypto stack、GATT buffers 或 product composition 峰值；这些属于 R4b-5 真机门禁。

## 14. Wi-Fi journal批次资源增量（2026-08-14）

新增portable credential journal后，UNO R4 32位ABI为：

| 对象 | 大小 | 说明 |
|---|---:|---|
| `WifiCredentialProfile` | 104 B | generation/auth/length + 32 B SSID + 64 B credential |
| `WifiCredentialStore` | 28 B | 三个blob引用与active元数据，不内嵌profile |
| `BleNoiseModePreparation` | 16 B | channel/provider/local-session三个引用与虚表 |
| `LocalProvisioningCoordinator` | 144 B | 增加session lifecycle虚表后由140 B变为144 B |

Small SizeProbe变为52,136 B Flash / 12,116 B `.data+bss`。相对20.14增加的152 B BSS等于探针故意实体化的
104 + 28 + 16 B新对象数组及既有local coordinator增加的4 B；它用于读取ELF ABI，不是产品同时常驻两份
Wi-Fi profile的要求。journal执行commit时栈上使用116 B encoded profile和104 B candidate/current对象，平台
connection manager仍应串行执行，不能在多个task各保留一套密码副本。

ESP32 Core clean compile为351,907/22,616 B，ESP32 storage probe（含三key sink所有方法链接）为
310,651/21,120 B，Wio Core仍为83,436 B。上述不是完整`cloud + secure provisioning + Direct`产品数字；
当前软件缺口已精确收敛为secure BLE application endpoint和Wi-Fi/TLS activation driver，之后必须重新测完整
ESP32双OTA以及UNO R4 32 KiB stack watermark。

## 15. 归档：旧 secure local endpoint资源增量（2026-08-14）

新增`IEnrollmentClaimSink`引用、endpoint ABI探针和endpoint对象后，clean SizeProbe从52,136/12,116 B变为
52,160/12,184 B。68 B静态RAM增量由local coordinator新增引用及故意实体化的endpoint `sizeof`数组构成；
实际32位endpoint由编译期gate限制为不超过64 B。

以下 258/264 B 是已删除 Enrollment/1 application 的历史值，不得用于当前 LocalSetup composition；当前
最小值为 512 B operation workspace + 273 B response storage。

历史产品还必须显式预算两个caller-owned、互不重叠的工作区：258 B operation + 264 B response = 522 B。response只在
单TX槽背压时保留，进入Noise TX record后立即清零；operation每次handle返回即清零。这522 B不能与当前Noise
plaintext callback buffer重叠，也不能与response同时复用。proof cache以及Small Noise channel的RX/TX/plaintext/
ATT固定buffer仍另计，所以该批没有证明UNO R4 32 KiB产品峰值可接受。

本批其他clean结果为ESP32 Core 352,415/22,616 B、ESP32 storage 311,183/21,120 B、Wio Terminal Core
83,436 B。ESP32相对上一批约增加0.5 KiB Flash、static RAM不变；UNO R4探针增加24 B Flash/68 B static RAM。
这些仍是Core/adapter/ABI结果，不是包含TLS、Wi-Fi、BLE stack与product object graph的最终数字。

## 16. 真实WiFi adapter与组合探针增量（2026-08-15）

ESP32与UNO R4已使用独立`IWifiStation` adapter替换组合探针中的直接WiFi调用。最终认证能力校验后的Arduino CLI
clean结果如下：

| 目标 | Flash | 静态RAM | 判定 |
|---|---:|---:|---|
| ESP32 WiFi adapter standalone | 904,951 B | 43,352 B | 编译/链接通过；包含WiFi平台栈基线 |
| UNO R4 WiFi adapter standalone | 56,524 B | 7,528 B | 编译/链接通过；包含WiFiS3基线 |
| ESP32 legacy product probe | 1,288,083 B | 60,720 B | 实际1,572,864 B app slot余284,781 B（18.11%） |
| UNO R4 legacy product probe | 148,400 B | 14,208 B | 32 KiB SRAM扣除static后余18,560 B |

两个adapter对象的32位ABI均为8 B，credentials只增加`start()`期间33+65 B临时栈buffer并随后清零。产品静态RAM没有
因最终认证模式检查增加。UNO R4的18,560 B不是可自由分配预算，还必须容纳WiFiS3/ArduinoBLE/TLS/MQTT运行期heap、
任务栈和回调峰值；ESP32的18.11%是Flash app-slot余量，也不能替代OTA镜像、heap或双无线压力门禁。

该probe仍使用legacy JSON authKey control plane，只证明当前对象图可编译和链接，不是v2 operational credential云会话、
正式orchestrator或完整安全配网证据。下一次有意义的完整资源冻结点是`device-cloud-session/2`和正式product orchestrator
落地之后。

## 17. V2 ProductOrchestrator 正式静态冻结点（2026-08-15）

第16节的legacy探针已被替换，不再进入当前发布判断。新`WifiBleProductProbe`实际链接
旧实验组合 `ProductOrchestrator + WiFi journal/lifecycle + enrollment/1 + device-cloud-session/2 + hardware CSPRNG +
Managed MQTT + Direct BLE + BBP/2 Client`，并从易失的预注册Active credential启动：

| 目标 | Flash | Static RAM | 结论 |
|---|---:|---:|---|
| ESP32 core 3.3.1 / Small / compact NimBLE 2.5.0 | 1,297,291 / 1,572,864 B | 62,304 / 327,680 B | 双OTA slot余275,573 B（17.52%），通过10%门禁 |
| UNO R4 core 1.5.3 / Small / ArduinoBLE 2.0.2 | 167,696 / 262,144 B | 15,916 / 32,768 B | 静态SRAM余16,852 B（51.43%），运行期门禁待真机 |

探针复用一组320 B request、640 B response buffer给互斥的enrollment/cloud-session阶段，避免重复常驻
buffer；Direct BLE以`External`注册，避免`Client`和`BleModeCoordinator`重复保存或控制第二套transport。
它尚不链接完整Noise enrollment crypto/identity/secure-storage composition，因此以上数字不能外推为完整量产
安全配网的最终上限；也不能用UNO R4剩余static RAM代替真实heap/stack watermark。

## 18. ESP32完整Developer安全组合冻结点（2026-08-15）

独立`Esp32SecureProductProbe`在第17节基线上加入受保护NVS边界、P-256 Developer identity、server receipt key ring、
Stored Setup PSK、Noise NNpsk0、secure local provisioning endpoint及完整编排启动顺序。Arduino ESP32 3.3.1 clean结果：

| 目标 | Flash | Static RAM | 结论 |
|---|---:|---:|---|
| ESP32 core 3.3.1 / Small / compact NimBLE 2.5.0 | 1,321,659 / 1,572,864 B | 67,280 / 327,680 B | 双OTA余251,205 B（15.97%），通过10%门禁 |

相对第17节ESP32基线增加24,412 B Flash和4,976 B static RAM。探针默认NVS attestation为false、没有PSK写入路径或
TLS CA自动启动，因此普通开发板fail-closed。结果证明完整软件对象图可链接且资源可接受，不证明安全NVS、无线/TLS
动态heap、stack watermark、掉电和24 h。UNO R4 后续已有 portable crypto 与可选 native P-256 verifier，
但仍没有等价的受保护密钥存储和真机组合证据，不能外推该结论。

## 19. R4b-3 第四片真实 crypto adapter 编译门禁（2026-08-16）

本批将控制面所需 crypto 从测试 seam 落为三个窄 adapter。结果是完整 probe 的链接总量，不是 adapter
净增量，也不包含最终 WiFi+BLE product 的所有并发对象：

| 目标/后端 | Flash | Global/static RAM | 编译内置向量 |
|---|---:|---:|---|
| UNO R4 / portable `Crypto@0.4.0` | 65,060 B | 6,932 B | RFC 7748 X25519、AES-256-GCM、Ed25519 verify、mutated reject |
| UNO R4 / `SoftwareATSE` P-256 verifier | 57,196 B | 7,564 B | raw low-S P-256 verify、mutated reject；R4b-4b复测 |
| Wio Terminal / portable `Crypto@0.4.0` | 44,316 B | 3,612 B `.data+.bss` | 与 UNO portable 相同 |
| ESP32 / core mbedTLS native | 357,479 B | 20,816 B | X25519、AES-256-GCM、raw low-S P-256 verify、ordinary NN；R4b-4b复测 |

UNO portable 在同一 X25519/AES-GCM probe 加入 Ed25519 验签后增加 6,404 B Flash、4 B global RAM；Wio
增加 6,480 B Flash、`.data+.bss` 不变。该低 static RAM 增量不代表调用时 stack 很小；真机必须测曲线
运算 stack watermark、失败路径输出清零、耗时、看门狗、WiFiS3/ArduinoBLE/MQTT 并发。

Core 只保存 borrowed Flash/const public key descriptor 与小型 key-ring view。P-256 server-only 后端使用
`IP256DigestVerifier`，不再被迫实现私钥生成/签名；portable provider 只暴露 Ed25519 验签，不调用依赖的
RNG 或 signing API。旧未发布 `esp32-noise` 包名及空目录已删除，当前 package boundary 为 Core + 13
adapter，共 14 个确定性包。

准入结论仍是“软件/SDK 可行，硬件待验证”。在真机 known-answer、stack/heap、掉电、长稳与第三方依赖
安全审计完成前，不把这些数字写成 production secure 或 official support。

## 22. DeviceKey WiFiProv P3/P5/P6 当前资源冻结点（2026-08-18）

> 2026-08-18 鉴权接线后修订：本节数值采集于 P6 仍使用独立 DeviceKey/Controller store、尚未注入
> P-256 ControllerControl endpoint 时，只能作为历史对比基线。当前分支已演进为 260 B DeviceAccessStore
> schema v3（兼容读 176 B v1/196 B v2）和
> server-signed control lane；2026-08-23 正式单 ZIP Plain 已完成完整 link/Flash/global RAM 重测，TLS 仍只能
> 使用历史对比，不能用于发布准入。

以下是 Arduino-ESP32 3.3.1、Small profile、`min_spiffs` 双 OTA（单 app slot `1,966,080 B`）的完整 DeviceKey
challenge/session + PubSubClient + BBP/2 图，不是 adapter-only 数字：

| Probe | 配网 / 日常 BLE / cloud transport | Flash / slot | Static RAM | 判定 |
|---|---|---:|---:|---|
| P3 Plain | BLE WiFiProv Security1 / 无 Direct / Plain | 1,758,907 B；余 207,173 B（10.54%） | 62,964 B | WiFi-only 静态候选；余量贴线 |
| P3 TLS | BLE WiFiProv Security1 / 无 Direct / TLS | 1,845,907 B；余 120,173 B（6.11%） | 63,556 B | 低于 10%，拒绝作为默认 |
| P6-S Plain | SoftAP WiFiProv Security1 / NimBLE Direct / Plain | **正式安装 ZIP 1,406,115 B；余 559,965 B（28.48%）** | **62,848 B** | dependency、ELF symbols 与 provenance 门禁通过 |
| P6-B Plain（XIAO S3） | BLE WiFiProv Security1 / NimBLE Direct / Plain | **1,303,487 / 3,342,336 B（38%）** | **59,684 B** | IDF-NimBLE 目标静态候选；真机切换未通过 |
| P6 TLS | SoftAP WiFiProv Security1 / NimBLE Direct / TLS | 历史：1,463,067 B；余 503,013 B（25.58%） | 历史：61,456 B | 当前需重测 |

P6 的关键优化不是删功能：WiFiProv 公共 adapter 与 SoftAP/BLE scheme factory 位于编译期互斥的
`Esp32WiFiProv.ipp`，由产品入口在同一 translation unit 内联实例化。这样 Arduino 依赖扫描和最终链接都只
看到选择的一种；NimBLE 还服务日常 Direct 数据面。默认 P6-S ELF 存在 SoftAP、NimBLE、DeviceKey、
PubSubClient、BBP/2、Controller Method 2；BLE provisioning、protocomm BLE、Bluedroid 和旧
CloudCredential/Ownership/Noise product graph 均为 0。TLS profile 还必须存在 `mbedtls_ssl_`。

P4 单 Bluedroid Direct 的历史结果为 `1,822,043/65,776 B`、余 7.33%，低于门槛。P6/P5 冻结后，其无消费者的
product wrapper/profile/probe 已删除；这里只保留决策证据，不再把 P4 列为当前构建目标。

这些结果只完成 compile/link/static admission。P6 仍要测两种 provisioning bearer 与 NimBLE 日常数据面的 heap
回收、largest block、任务栈、WiFi/RF 共存、100 次循环和 24 h；Controller control 已接入并完成本次
link/resource 重测，App 双 bearer UX 仍缺，
因此 28.48% 静态余量不等于 production release。Plain/TLS 都是显式编译期配置，禁止运行时静默降级。

## 23. 标准 Arduino 单 ZIP 公开产品闭包（2026-08-23）

本节取代“所有 `src/*.cpp` 都进入固件”的错误判断。Arduino 构建器会编译单库 `src` 中的 translation unit，
`dot_a_linkage=true` 会先形成 archive，但 Core 可能按成员抽取，也可能 `--whole-archive` 后依赖 section GC。
正式门禁同时检查 `libraries.cache`、最终 ELF defined symbols、安装源码 provenance 和资源摘要；link map 的
archive inclusion table 只用于诊断，不能单独证明代码进入固件。Aily Builder 结果不用于资源准入。

| 安装 ZIP 目标 | Flash | Static RAM | Blinker ELF symbols/闭包 |
|---|---:|---:|---|
| ESP32 WiFi | 990,571 B | 50,192 B | 无 BLE/Noise/NimBLE；依赖仅 Blinker/Network/WiFi/PubSubClient |
| ESP32 BLE | 772,863 B | 43,384 B | 无 DeviceKey/MQTT/HTTP cloud；依赖仅 Blinker/NimBLE |
| ESP32 WiFi+BLE | 1,406,115 B | 62,848 B | SoftAP WiFiProv + DeviceKey/MQTT + NimBLE Direct；无 BLE WiFiProv/Bluedroid/Noise |
| ESP32 Cloud smoke | 1,009,455 B | 50,224 B | 正式联调例程；包含多属性上报所需 `StateUpdate`，无 BLE/Noise |
| UNO R4 WiFi | 134,176 B | 13,028 B | 无 ArduinoBLE/Crypto/Noise |
| UNO R4 BLE | 201,308 B | 17,216 B | 无 PubSubClient/cloud |
| Wio Terminal WiFi | 148,072 B | 67,680 B | 34；仅 rpcWiFi 侧闭包 |
| Wio Terminal BLE | 207,760 B | 65,100 B | 67；仅 rpcBLE/Crypto 侧闭包 |

UNO R4 BLE 的静态 SRAM 仍只剩 15,552 B；这不是运行期 heap/stack 通过证据，真机长稳仍是发布阻塞项。
Wio RAM 由 ELF `.data + .bss` 计算。单一发布 ZIP 为 325 entries、448,846 B；两次确定性构建 SHA-256 均为
`7F5D16CC47E03AC4798C75A3E32C932E0CCAF7958C55B57B65D9FC01701E09AD`。
