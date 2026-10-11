# Device V2 调试、发送治理、实时遥测与大流量传输方案

> **2026-10-11 实时产品结果见[实时与历史方案§7～§8](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md#8-l2最小实施范围2026-10-11)。** 云端真实App短验收及L2a共用Direct/BLE页面接入软件与ESP32编译通过；下一批只做L2b BLE原生真机短验收，LAN产品、R4新增示例编译和History后置，离线storage/Bulk不实施。本页旧现场状态和历史“下一步”不覆盖新入口。

> **2026-09-22：执行入口仅见[主线 §6](DEVICE_V2_NEXT_EXECUTION_PLAN.md)。** onSample/页面 lease/发送限流继续各司其职，不兼任用户定时；新增云自动化的数据需求由独立有界消费者持有，停用释放且不放宽总体限流，见[任务合同 §6](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md)。本页历史“下一步”不自动扩大任务。

> **2026-09-12 D1 背压事实修订：** [B2b-5 真机](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#121-发现并修正的测试阻塞不扩-ble-协议)发现开启 `debug(Serial)` 的测试固件在 USB 无人读取时，状态转换 Print 可阻塞 BLE 首次 Hello；业务回调打印还曾造成约 6 s ACK。持续读取的原固件及移除连续输出后关闭串口的修订固件均通过。标准 QuickStart 不开启日志，本批只清理硬件测试 Sketch；**`debug(Print&)` 仍为同步 formatter，§9.3 的独立日志预算、§9.4 的背压不影响通信是尚未完成的保障，不能从 D1“代码完成”推导已满足。** 下一步先约束可检测输出容量/短写丢弃及 sink 适配，资源不足时丢诊断不等通信；通用 Print 不保证容量/非阻塞，不能无依据宣称任意平台都可抢占阻塞写。保持默认关闭日志，不新增通用大 ring buffer/后台任务，不用调大协议 deadline 代替修复。

> 状态：三端实施主线，更新至 2026-08-23；D1～D3、D4 设备核心、Broker/App 软件核心、V2 基础页面接线，
> 以及 Wio Terminal 1 Hz Cloud Open/Data/Close/设备 lease expiry 真机门禁已完成。ESP32 WiFi+BLE 正式设备
> composition 的源码/安装 ZIP 静态门禁也已完成。App 后续由独立维护主线负责；当前设备主线优先替换板
> Cloud/Direct 同时在线与 WiFiProv 生命周期门禁，不扩展 D5 History 或 D6 Bulk。
> Telemetry wire/软件核心已经启用；本地真实 Gateway/PostgreSQL/Broker/Wio 链路已通过受控双账号撤权即停流，
> 运行正式 App V2 session 代码的独立 OS 进程也已通过强杀断连回收。真实 Android/iOS 前后台与 kill、线上账号和
> ESP32 正式组合产品的 Cloud/Direct 真机同时在线完成前仍不作为完整
> production-ready 发布能力；当前代码不得宣称支持 History 或 Bulk。
> XIAO ESP32S3 已完成正式产品图实编和 Flash hash 校验；设备当前仍处于 ESP ROM 下载口，尚未观察到
> 配网服务或应用日志，所以这不是运行期配网证据；当前板随后判断疑似损坏。下一动作改为在替换板验证
> SoftAP/BLE WiFiProv 生命周期、heap 与超时复位，
> 不是开始 History/Bulk。
> 设备侧 `RateLimited(12)` 闭环已经补齐：Cloud 限流会让 State/Event 调度和同一 Cloud source 的
> TelemetryLease 退避 1 s；Direct lease 不受影响，期间不排队或补发过时 sample，也未增加固定运行时槽。
> 2026-08-23 Wio 压力门禁又修正两项平台无关缺陷：StateRequest settlement 不再先向请求 peer 发送 Patch，
> keyed Snapshot/Patch 也不再依赖用户按 CBOR canonical key 顺序注册组件。服务端 Telemetry 独立 lane 与
> state-resync readiness 随后修正。本轮正式双字段 `CloudSmokeTest` 又把限流和 Close 拆开复验：单个 State frame
> 被硬丢弃后 MQTT/App session/设备 Cloud 均保持在线，1 s 后 Telemetry 连续恢复 5 个样本；最后 consumer Close
> 后设备样本计数停止。持续 8 次违规会断开，Wio 在约 9 s 后自动重连，符合 abuse fence。
> 同次门禁发现并修正了 App `DeviceV2Session` 对顶层 `RateLimited Error` 的处理：按 `relatedSequence`
> 结束对应 pending Route，不关闭账户 session，也不自动重发 Command/Event；迟到或重复 Error 不影响在线会话。
> 修正后的正式 App 代码已与真实 Broker/Wio 复验：正常 Command 先完成，在租户预算临时降为 1 frame/s 后
> 4 个并发 Command 中 3 个精确收到 `RateLimited(12)`，账户状态始终只有 `connecting → ready`；预算恢复为
> 100 frame/s 后，同一 MQTT/App session 的后续 Command 收到 ACK。测试结束已校验 Broker 持久策略恢复。
> 当前 BBP/2 Frame、Property/Action/Event、State revision、Route/Delivery 的已冻结部分仍以
> [BBP2_NORMATIVE_WIRE_SPEC.md](BBP2_NORMATIVE_WIRE_SPEC.md) 为准。新增消息必须通过三端 exact/invalid
> golden、资源门禁和真机联调后，才可进入规范的 `current` 集合。

本文解决四个彼此相关但不能混为一谈的问题：

1. 小 MCU 如何用低 Flash/RAM 成本提供可用且不泄密的调试信息；
2. 如何防止初学者在 `loop()` 中无节制上报，同时不阻断状态、告警、自动化、设备间通信和设备到服务通信；
3. App 页面打开时如何按需获取 1 Hz 或更高频实时数据，页面退出、断网或崩溃后如何自动停止；
4. 历史采样、波形、日志、文件、图片、音视频等数据为什么不能继续复用普通 Property/Event，以及应采用什么传输边界。

### 0.1 当前实施状态

D1 已落入标准 Arduino `src`：

- [Diagnostics.h](../src/BlinkerV2/core/Diagnostics.h) / `Diagnostics.inc` 提供 16 B 固定数值事件、固定计数、
  单 pending 槽、重复抑制和 Debug/Trace 100 ms 输出节流；不使用 heap、动态字符串或虚接口；
- [ArduinoDiagnostics.inc](../src/BlinkerV2/arduino/ArduinoDiagnostics.inc) 是唯一文本格式化器，只有 Sketch 调用
  `debug()` / `printDiagnostics()` 时才进入最终链接；
- [BlinkerFacade.h](../src/BlinkerV2/api/BlinkerFacade.h) 已提供 `debug()`、`noDebug()`、
  `printDiagnostics()`、`diagnosticCounters()` 和 `telemetryCounters()`；
- `Client` 统一记录 Property/批量 State 的 changed/no-change/error、Event 成功/error 和 `WouldBlock`；
- 旧的未接线虚接口 `ILogger` 已删除，不再保留第二套日志抽象。
- `CompilePublicWiFi`、正式 `BleQuickStart` 和 `compile_matrix.py --group products` 直接编译
  `<BlinkerWiFi.h>` / `<BlinkerBLE.h>` 的 ESP32、UNO R4、Wio Terminal 完整产品图，禁止再用注入假
  lifecycle 的 `CompileFacade` 代替产品依赖门禁。

当前 D1 定义已经有真实生产点的 `library/product/state/event` 组件和 8 个基础计数；D2 提供实际发送调度计数；
D4 已增加 `telemetrySamples / telemetryDropped / leaseExpired`。WiFi、HTTP session、MQTT、BBP/2、BLE 等更细
事件仍必须在对应状态机有真实生产点后再增加，不得加入永远不变化的占位计数。

D2 已落入跨平台 Core，而不是各平台 adapter：

- [OutboundScheduler.h](../src/BlinkerV2/runtime/OutboundScheduler.h) / `OutboundScheduler.inc` 使用 caller-owned
  dirty/selection bitmap，不复制 Descriptor、Manifest 或完整 Property value，也不使用 heap；
- Property 立即更新 `EndpointStateStore`，默认在 250 ms 窗口内 latest-wins 合并，最多形成 4 个 State revision/s；
  多字段超出共享 Patch scratch 时按 Manifest 顺序拆成连续 revision，不静默丢字段；
- Command 完成、远端 Patch/StateRequest 和 lifecycle stop 都是 settlement boundary，禁止出现“旧 revision + 新
  snapshot”；StateRequest 结算时只向其他已就绪 peer fanout Patch，请求 peer 直接取得包含最新 revision 的
  StatePage，避免 Patch 抢先改变其 snapshot sequence；某 peer 发送失败不回滚本地事实，并撤销该 peer 的增量基线；
- keyed Snapshot、选中字段 Patch 和 keyed StatePage 在编码时按 deterministic CBOR text-key 顺序遍历；组件注册
  顺序只定义 field ID/Manifest，不再隐含要求 Arduino 用户手工按 key 长度和字节序排序；实现不保留排序表或 heap；
- 普通 best-effort Event 使用 5/s、burst 8 的固定令牌桶，超限明确返回 `WouldBlock`；它仍不是 Telemetry，未来
  durable alarm 也不能复用这个普通 Event lane；
- `Blinker.printDiagnostics()` 可输出 State accepted/coalesced/flush/deferred/transport-error 和 Event
  accepted/dispatched/throttled/transport-error 计数；不输出 payload、Key、WiFi 密码或 credential；
- 官方 WiFi/BLE product 在内部注入同一单调时钟，所以 ESP32、UNO R4、Wio 和 BLE 共享相同规则。Advanced
  `Client` 若不注入 `IClock` 则保留旧的立即 State/不限 Event 行为，避免伪造不可前进的时间源；
- D2 完成时 Wio/SAMD 32 位交叉编译得到 Small `sizeof(Device) == 1560 B`，当时资源门禁冻结为 1568 B；该数值是
  引入 TelemetryLease 之前的历史基线，不是当前 D4a ABI。

Host 门禁已覆盖同 field 合并、多字段合帧、Patch scratch 超限拆分、StateRequest 强制结算、transport
`WouldBlock`、Event burst/refill/限流、Telemetry 丢旧/sequence gap 和 32 位 `millis()` 回绕。当前 ASan 41/41、
pre-enrollment exact verification 与 200,000 例确定性协议 fuzz 已通过。平台完整产品结果由
`compile_matrix.py --group products` 保留，不在本主线重复维护容易过期的构建数字。

资源实测（Small profile，`CompileFacade`，2026-08-21）：

| 平台 | logs off | Debug formatter | 增量 |
|---|---:|---:|---:|
| ESP32 | 310,747 B Flash / 22,424 B RAM | 327,491 B / 22,440 B | +16,744 B / +16 B；包含首次引用 `Serial` 的 ESP32 core 成本 |
| UNO R4 WiFi | 71,520 B / 9,176 B | 74,128 B / 9,176 B | +2,608 B / +0 B |
| Wio Terminal | 50,236 B Flash | 52,972 B Flash | +2,736 B；该 core 的 CLI 未报告 RAM |

UNO R4 的默认 Info 与显式 Debug 复用同一 formatter；最终 Debug 为 `74,128 B / 9,176 B`，且前一轮同源
Info/Debug exact build 结果相同，证明级别只是运行时
阈值。link map/ELF 同时确认 logs-off 不保留 Arduino diagnostics formatter 符号。ESP32、UNO R4、Wio 的 off 与 Debug 均已
编译/链接通过；`compile_matrix.py --group diagnostics` 保留三平台 Info/Debug 可复现目标。

Wio Terminal（COM6，RTL8720 eRPC v2.1.3）已经完成 DeviceKey challenge/session、MQTT CONNECT、HELLO、
Manifest/指纹、State snapshot/周期 Patch、COMMAND/ACK、handler 后 Report 与持久 ledger replay。Gateway 停止、
跨 session expiry、Gateway 恢复也已在不复位设备的情况下通过，WiFi 全程保持在线。因此 Plain WiFi 基础 CloudSmoke
和 Gateway outage/recovery 门禁已通过；此前“仍被主机防火墙阻塞”的描述已经失效。

联调修复保持平台无关：`ArduinoClientHttpAdapter` 对数字 IPv4 使用 `IPAddress`，优先调用带 timeout 的
`Client::connect()`，仅在 Client 不提供该 overload 时编译期回退；`HttpDeviceKeySessionProvider` 把 malformed
`408/425/429/5xx` 和 outage 期间的本地 session expiry 保持为 transient backoff，不误报 credential 被撤销。
Seeed rpcWiFi 的 RTL RPC 在目标端口静默丢包时仍可能阻塞，因此部署侧必须由稳定 ingress 快速 accept 并返回定长
503，或快速关闭 MQTT；设备库不 fork Seeed SDK，也不增加 Wio 专用公开 API。

D4 本批新增三个固定资源计数，并冻结发送失败语义：due sample 无论发送成功还是 `WouldBlock` 都消费本采样周期；
失败样本不进入 ReliableOutbox、不热循环重试，下一成功 sample 通过 sequence gap 暴露丢失。`Device` 的 32 位 ABI
为 Small/Standard/Large `1664/3448/10980 B`。Small 从一个租约槽修正为两个槽，增加 36 B 固定 RAM，使其与
两个 peer session/Cloud+Direct 的最小能力一致。Runtime 每次 `poll()` 以槽数为硬上限处理全部 due/expired，避免
较慢 `loop()` 永远偏向第一个槽。Host 已验证 Cloud/Direct 可使用相同 session-local `streamId`、独立 epoch/sequence/
target，Direct 断开只删除 Direct lease，Cloud 继续采样。Wio 通过正式 interop 单库和公开 `CloudSmokeTest` 完成
DeviceKey/MQTT/BBP2、Telemetry Open、连续 1 Hz Data、显式 Close；Broker 暂停超过 30 s 时设备侧
`leaseExpired` 从 0 增至 1，采样计数停止增长，Broker 恢复后 Cloud 自动重连。过程中修正两项全局缺陷：
`Product::begin()` 必须先让 lifecycle attach 时钟/transport 再 prepare realtime schema；Broker 按 consumer 降频必须使用
设备 `monotonicMs`，不能用有到达抖动的服务器墙钟。八个公开产品目标均已通过源码 clean build；ESP32
WiFi+BLE 又通过单 ZIP 安装编译与 ELF/Map 闭包。XIAO ESP32S3 已完成真机烧录校验，但尚未退出 ROM 下载模式，
因此配网、动态 heap 和 Cloud/Direct 仍等待替换设备运行与 App coordinator。
Wio 后续 250 ms State 压力门禁以 `power / uptimeSeconds / rateProbe` 的非 canonical 注册顺序复现并修复
StateRequest 前置 Patch 和 keyed Patch 顺序问题；修复后设备连续保持 `state=active / cloud=1`，无新增固定 RAM。
标准双字段 `CloudSmokeTest` 修复后为 142,200 B Flash（27%，Seeeduino SAMD 1.8.5），较修复前 141,656 B
增加 544 B，其中 canonical 无分配遍历为主要增量。
该产品的首次配网协议是 WiFiProv Security1：SoftAP 为跨 ESP32 默认，Arduino Core 已使用 IDF-NimBLE 的
目标可编译期选择 BLE bearer；运行期本地承载才是 NimBLE Direct。App/Service 的一次性 bootstrap intent 不作为
设备登录 secret。不能再把“WiFi+BLE 产品”误写成“必须使用 BLE 配网”，也不能用 Wio 的两个独立产品伪造同时在线。

---

## 1. 冻结结论

1. **V2 不移植 dev_edu 的全局 `isAlive` 发送门。**“最近收到任意消息后 30 秒才允许上行”会阻断自主
   状态、告警、自动化、Device→Device 和 Device→Service，并且无法正确表达多个并发消费者。
2. **MQTT Keep Alive 只负责判断连接是否存活。**它不得授权业务发送，也不得作为实时页面活跃状态。
3. **保留旧方案中“有请求才持续响应”的合理意图，但改成有作用域的合同：**
   - 单次请求使用零常驻槽的同步 `RequestScope`，只允许向原请求者返回 ACK/Error/结果；
   - 连续实时数据使用 `TelemetryLease`，只允许指定字段、指定速率、指定消费者和有限租期；
   - 设备状态、告警和已授权业务事件不依赖 App 页面是否打开。
4. **所有 transport 共用一个设备侧 `OutboundScheduler`。**WiFi、BLE、WiFi+BLE、UNO R4、Wio Terminal
   和后续串口网络模组不得各自复制限流逻辑。
5. **状态、事件、遥测、历史和大流量必须分流：**
   - `Property Report`：当前事实，版本化、可恢复、latest-wins；
   - `Event`：一次性瞬时语义，不等于连续采样流；
   - `Telemetry`：有租约的短生命周期实时采样，不写 latest-state，不逐样本持久化；
   - `History`：带采样时间、可批量、可断点继续的记录数据；
   - `Bulk`：文件或连续大流量，使用独立块/窗口传输。
6. **限流必须双端执行。**设备端用于防止误用、控制 RAM 和提供平滑退化；Broker/Gateway 端才是防恶意、
   多租户隔离和成本控制的安全边界。
7. **普通 Arduino 用户不需要学习调度器、队列或 Python 工具。**常规 API 仍是 `report()`、`emit()`、
   `begin()`、`run()`；实时字段只增加一个可选声明和采样 callback。

---

## 2. 当前实现审计

### 2.1 dev_edu 的 30 秒 `isAlive` 实际做了什么

`dev_edu:src/Blinker/BlinkerConfig.h` 定义：

```text
BLINKER_MQTT_MSG_LIMIT = 1000 ms
BLINKER_MQTT_KEEPALIVE = 30000 ms
每分钟普通 print 上限 = 30
```

`dev_edu:src/Functions/BlinkerWiFiESP.h` 和 `BlinkerWiFiUNO.h` 的流程是：

1. 收到 MQTT 消息后，按 `fromDevice` 修改全局 `isAlive`、`_sharerFrom` 或 `UUID_EXTRA`；
2. owner 消息会刷新 `kaTime`，部分 server/extra source 分支只设置 `isAlive`，却没有一致刷新 `kaTime`；
3. 普通 `print()` 要求 `isAlive=true` 且距离上次发送约 500 ms，并再受每分钟 30 条限制；
4. 30 秒后 `checkKA()` 将 `isAlive=false`；但 `printTime==0` 又能绕过该条件；
5. 下次普通输出的 `toDevice` 可能由最后一个入站发送者决定。

这个设计在“用户刚打开页面/按下控件，设备在短时间内响应这个用户”的早期单消费者模型下有一定合理性：

- 减少设备无人查看时持续向 App 发送 UI 刷新；
- 降低初学者把 `print()` 放在 `loop()` 里的破坏面；
- 将一次 App 操作后的响应限制在短时间内。

但它不能进入 V2，原因是：

- **传输健康、授权、目标选择和发送频率混成一个 bool；**
- **任意来源消息可开启全局发送窗口；**不能证明后续消息属于哪个 request/consumer；
- **多个 App、家庭成员、服务或设备并发时存在 last-sender-wins；**
- **自主告警可能因为之前没有入站而发不出去；**
- **周期状态、历史上传、自动化和 Device→Service 被 App 活跃状态错误阻断；**
- **30 秒窗口不能表达字段、速率、字节数、可靠性和接收目标；**
- **ESP/UNO/BLE 分散实现不同规则，难以验证和扩展。**

因此 V2 只继承“请求响应有边界”和“实时数据按需发送”的意图，不继承实现。

### 2.2 当前 V2 已有能力

- D1 已用 [Diagnostics.h](../src/BlinkerV2/core/Diagnostics.h) 的固定数值事件替换原先完全未接线的 `ILogger`，
  并由公开 facade、State/Event 发送路径和可选 Arduino `Print` formatter 共用一条链路。
- [Client.inc](../src/BlinkerV2/api/Client.inc) 的 `commitState()` 已过滤相同值；普通重复值不会重复 Patch。
- [BlinkerFacade.h](../src/BlinkerV2/api/BlinkerFacade.h) 已支持一次 `Blinker.report(p1,v1,p2,v2...)` 合并多字段。
- [PubSubMqtt.h](../src/BlinkerV2/arduino/ports/arduino/PubSubMqtt.h) 和
[BleRecordTransport.inc](../src/BlinkerV2/transport/BleRecordTransport.inc) 在底层容量不足时返回 `WouldBlock`。
- [ReliableOutbox.h](../src/BlinkerV2/runtime/ReliableOutbox.h) 已为可靠请求提供固定槽、重试、超时和结果计数。
- `Event` 已能完成 Device→Broker→App 的 best-effort 瞬时 fanout，且不修改 state revision；但当前 Broker
  会发送给所有符合 ACL/Manifest gate 的在线 App session，不存在页面级订阅租约。
- S1 已将 Gateway `tenant_limits` 投影到 Broker，并使用分 lane、多维 token bucket 执行 packets/s、bytes/s
  和 packet-size 限制；managed 模式通过 Redis Lua 保证跨实例原子共享。

### 2.3 当前最危险的发送路径

```cpp
void loop() {
    temperature.report(analogRead(A0));
    Blinker.run();
}
```

固定值会被当前状态仓过滤，但真实传感器噪声会造成持续变化，PubSubClient 又会立即调用 MQTT publish。
仅在 BLE 队列或底层 socket 满时返回 `WouldBlock` 不是发送治理：消息很可能已经在到达底层前轰炸 Broker，
而用户通常也不会检查每次 `report()` 的返回值。

---

## 3. 必须冻结的术语

| 名称 | 判断的问题 | 状态所有者 | 是否允许业务发送 |
|---|---|---|---:|
| Transport Keep Alive | TCP/MQTT/BLE 链路是否仍存活 | transport | 否 |
| Authentication Session TTL | 当前登录/控制会话是否仍获授权 | Gateway/Broker/session | 只作为前置条件 |
| Request Deadline | 某次 request 是否仍可产生关联响应 | request ledger | 仅关联响应 |
| Telemetry Lease | 是否有人正在订阅指定实时字段 | Broker/Direct session/Device | 仅指定 stream |
| Rate Budget | 某 lane 当前还可发送多少 frame/byte | Device + Broker | 是，且受消息类别约束 |
| Reliable Retry Window | 可靠消息能否继续重试 | ReliableOutbox/ledger | 仅原消息重试 |

必须禁止用同一个 `keepalive`、`isAlive` 或 `active` 字段同时表示其中两个以上概念。

MQTT 3.1.1 Keep Alive 表示客户端发出两个 MQTT Control Packet 之间允许的最大间隔；没有其他包时客户端发送
PINGREQ，Broker 在约 1.5 倍间隔没有收到 Control Packet 时断开连接。它和“App 页面是否打开”没有关系。
当前 V2 MQTT 默认 30 秒可以保留，并按网络稳定性后续调整；PINGREQ/PINGRESP 不计入 BBP/2 业务消息配额。

---

## 4. 消息分类和“是否需要先收到消息”

| Lane | 示例 | 是否需要先收到消息 | 持久/可靠语义 | 拥塞策略 |
|---|---|---:|---|---|
| Protocol Control | HELLO、Manifest、StatePage、PING | 协议决定 | 必须保证协商可推进 | 保留预算，不被遥测饿死 |
| Response | ACK、Error、Command result | 是，绑定原 request | requestId/sequence 幂等 | 最高业务优先级，固定窗口 |
| State | 电源、模式、温度当前值 | 否 | revision + snapshot 可恢复 | 去重、合并、latest-wins |
| Event | 按键边沿、普通瞬时通知 | 否 | 当前 best-effort | token bucket；不合并语义事件 |
| Durable Event | 安防告警、计费触发 | 否 | eventId + 服务端持久 ACK，待设计 | 可靠 outbox；不得静默丢弃 |
| Telemetry | 页面可见期间 1 Hz 温度/功率曲线 | **需要 TelemetryLease** | best-effort、有 sequence、不可跨重连恢复 | 降频、批量、丢旧样本 |
| History | 离线采样、能耗记录 | 不依赖页面；由计划或请求触发 | timestamp + cursor/ACK | 批量、后台低优先级 |
| Bulk | 文件、波形、日志、图片、OTA | 需要 job/stream grant | block checksum + resume | 独立窗口，最低优先级 |

核心规则：**只有 Telemetry/Bulk 这类高持续成本流量需要显式需求租约；普通事实状态和业务事件不能等待 App 先发消息。**

---

## 5. keepalive 替代方案及扩展业务影响

### 5.1 单次请求使用零常驻槽的同步 RequestScope

这里需要修正早期方案：Cloud `Route/Delivery` 在 Broker 终止，设备 MQTT session 只接收 Broker 转发的内部
`Command/Patch/...`。App `requestId`、来源账户和 command deadline 属于 Broker command ledger，不应复制到 MCU。
Direct BLE/局域网同样直接处理内部消息。

当前 Arduino handler 是同步合同，因此 Device 不新增常驻 `RequestContext[]`。一次处理调用栈中的有效作用域为：

```text
SynchronousRequestScope = {
  source = RxContext(transportId, sessionId, link-security facts),
  relatedSequence = BBP/2 frame.sequence,
  authorization = matched PeerSession permissions,
  responseTarget = exact source transport/session
}
```

- `RxContext`、frame body 和 endpoint value 只在 callback 内有效，不得保存指针/引用；
- ACK/Error/结果在 callback 返回前回到 exact source，不读取“最后一个发送者”；
- 一个请求不能打开通用状态、Event 或 Telemetry 发送权限；
- callback 返回即结束作用域，不需要 MCU deadline/timer/RAM slot；断开只清除对应 `PeerSession`；
- `SendTarget` 只是地址，不是授权；Runtime 发送前仍检查 peer、HELLO、feature 和 permission；
- 重放由 `(transportId, sessionId)` 隔离的 reliable receive window 和服务器 command ledger 处理；
- 将来若出现真正异步 Command/Job，必须另行冻结含 `sessionGeneration + deadline + responseState` 的固定槽协议，
  不能把 `RxContext` 延寿，也不能复用 TelemetryLease。

### 5.2 连续实时数据使用 TelemetryLease

候选默认值：App 页面请求 30 秒租期，每 10 秒续租；具体值在功耗、断网和 Broker 压测后冻结。

```text
TelemetryLease = {
  streamId,
  source/consumer,
  targetDevice,
  manifestFingerprint,
  fields[],
  effectiveIntervalMs,
  maxSamplesPerFrame,
  expiresAt,
  routeAuthorization
}
```

它只能授权列出的 field 和协商后的速率，不能授权任意 Event、State 或 Route。

设备公开 API 冻结为“声明实时字段 + 按需采样回调”，不再要求用户在 `loop()` 中维护上传定时器：

```cpp
BLINKER_PROPERTY(temperature, float, blinker::realtime(1000U));

float sampleTemperature() {
    return latestTemperature;
}

void setup() {
    temperature.onSample(sampleTemperature);
    Blinker.begin(/* credentials */, temperature);
}

void loop() {
    Blinker.run();
}
```

- Runtime 仅在授权 lease 有效且到达协商周期时调用 `onSample()`；无消费者时不采、不编码、不发送；
- callback 必须短小、非阻塞，不调用 `report()`、网络 API、`delay()`；
- 普通快速传感器可直接读取；慢速/共享总线传感器由自己的非阻塞状态机刷新缓存，callback 只返回缓存；
- 多字段若来自同一次物理采集，先共享缓存，不为此阶段新增 batch callback 或常驻任务抽象；
- `Property.report()` 仍表示可恢复当前状态，告警使用 Event；两者不能用来模拟页面实时流。

### 5.3 多消费者聚合

Cloud 模式下 Broker 不应把每个 App 页面的订阅逐个复制给设备：

1. Broker 按 `(device, stream capability)` 聚合 App、自动化服务和设备消费者；
2. 设备只看到一个聚合需求和一个有效采样率；
3. Broker 对同一份 sample 按 ACL fanout 给各消费者；
4. 最后一个消费者离开或全部租约到期后，Broker 发 Close 或让设备租约自然到期；
5. 更慢的消费者由 Broker 采样/降频，不能要求设备重复采相同数据。

### 5.4 对未来业务的影响

#### Device→Device

- 普通设备状态先进入 cloud ingest 一次，由服务器完成规则/订阅 fanout，不为每个消费者重复上报；
- 显式 Device→Device Command/Event 必须有 DeviceRouteGrant、明确 target、独立 rate budget 和 request correlation；
- 设备订阅另一设备的实时流时，该设备是 `TelemetryLease` consumer，不借用 owner App 的页面租约；
- 对端离线或租约过期只停止该 route/stream，不关闭发送设备的 MQTT 会话。

#### Device→Cloud Service

- 告警、自动化触发、日志摘要等自主业务可以直接发送给已注册 CloudService，不等待下行 keepalive；
- 连续算法输入由服务建立 TelemetryLease；服务租约可以由服务器代为续期，不依赖手机页面；
- 周期归档走 History，不伪装成实时页面；
- 服务返回的单次结果使用 requestId/Delivery，不能改变设备普通上报目标。

#### Direct BLE/LAN

- BLE 连接存在不等于订阅所有实时字段；仍需 StreamOpen；
- 断开连接立即撤销该 session 的租约；租约超时用于 App 崩溃但链路未及时释放的情况；
- BLE 与 Cloud 同时在线时分别维护 source session 和 streamId，设备采样可共享，编码/fanout 独立。

#### 无 App 在线

- 设备仍可报告状态、事件、告警、计划 history 和服务消息；
- 只有“为了正在显示的实时曲线而产生的额外高频采样”停止。

---

## 6. 统一 OutboundScheduler

### 6.1 放置位置

```text
Property / Event / Telemetry / History / Bulk API
                        |
                 OutboundScheduler
                        |
             BBP/2 Runtime / Router
                        |
        MQTT / BLE / LAN / serial-module transport
```

限流不得写在 `Esp32WiFi`、`UnoR4WiFi`、`WioTerminalWiFi`、BLE link 或 PubSubClient adapter 中。
transport 只报告 `Online/NotConnected/WouldBlock/BufferTooSmall` 和自身 frame/queue 能力。

### 6.2 固定资源结构

```text
OutboundScheduler
  ├─ reserved protocol/control budget
  ├─ ReliableOutbox / request responses
  ├─ Property dirty bitmap + shared patch scratch
  ├─ Event token buckets + counters
  ├─ fixed TelemetryLease slots + shared transmit scratch
  └─ optional one-window History/Bulk state
```

- 不为每条消息 `new/malloc`；
- 不复制完整 Endpoint descriptor 或 Manifest；
- pending Property 最新值继续复用 `EndpointStateStore`，每 field 只增加 dirty/调度元数据；
- Small profile 默认 1 个实时 stream，Standard 2 个，Large 才考虑 4 个；最终尺寸以 32 位 ABI probe 为准；
- History/Bulk 功能关闭时必须通过链接裁剪退出正式 ZIP 的二进制。

### 6.3 优先级与公平性

```text
P0 transport/protocol progress
P1 ACK/Error/reliable command receipt
P2 durable event（未来）
P3 state patch
P4 best-effort event / telemetry
P5 history / bulk
```

P0/P1 使用保留预算但仍有固定 outstanding/window 上限，防止攻击者用控制消息耗尽 RAM。P5 每次 poll 只消耗
剩余 budget，不能阻塞 `Blinker.run(totalBudgetMicros)`。

### 6.4 Property 合并和 revision

限流不能放在 `advanceStateRevision()` 之后，否则丢弃一个 Patch 会制造 revision gap。候选实现：

1. `report(value)` 立即更新本地 `EndpointStateStore` 并标记 field dirty；相同值成功 no-op；
2. 合并窗口内同一 field 只保留最新值，多 field 合并到一个 Patch；
3. flush 时把全部 dirty field 视为一个本地事实事务，只推进一次 revision；
4. StateRequest 到来时先结束当前 pending transaction，再返回相同 revision 的 snapshot；
5. 即使当前没有网络 peer，窗口到期也完成本地 revision，保证以后 snapshot 自洽；
6. Command 完成后的关键事实可请求 immediate flush，不等待普通窗口；
7. 某 peer 发送失败不回滚硬件事实；该 peer 清除增量资格并在恢复后重新 StateRequest。

`dirty` 期间必须被实现为一个尚未对远端提交的 open transaction：本地 `current()` 可以读取新值，但任何
StatePage、Patch Apply 基线校验、携带新 revision 的 ACK 或其他远端观察点，都必须先原子结束该 transaction。
实现绝不能向远端返回“旧 revision + 新 values”。这会调整当前 `report()` 立即推进 revision/立即 fanout 的
实现时点；在 D2 完成、状态同步文档与 golden 同步修订前，现有行为仍是当前合同，不能只加入 transport
限速便宣称完成合并。

普通用户仍可忽略返回值：进入 pending/coalesced 队列视为成功。Advanced diagnostics 可区分：

```text
accepted / coalesced / no-change / dropped / would-block / transport-error
```

### 6.5 候选初始参数

以下只用于第一轮压测，不是生产冻结值：

| 参数 | Small | Standard | 说明 |
|---|---:|---:|---|
| Property 合并窗口 | 250 ms | 100～250 ms | 同 field latest-wins |
| Cloud State 最大 frame 率 | 4/s | 5/s | 一个 frame 可含多 field |
| 普通 Event token | 5/s，burst 8 | 10/s，burst 16 | 安防等可靠事件另走 durable lane |
| Telemetry 默认间隔 | 1000 ms | 1000 ms | 满足常规实时图表 |
| Cloud Telemetry 候选最小间隔 | 200 ms | 200 ms | 最快 5 Hz，需服务端 grant |
| Direct BLE 候选最小间隔 | 200 ms | 100 ms | 仍受 MTU/queue/连接间隔限制 |
| App Telemetry lease | 30 s | 30 s | 候选每 10 s renew |
| 总业务上行软预算 | 2 KiB/s | 8 KiB/s | control reserve 不计入此软预算 |

有效限制始终为：

```text
min(device resource profile,
    transport capability,
    server/account/device grant,
    subscription request)
```

不能让用户通过修改一个 Arduino 宏绕过服务器硬限制。也不能照搬 dev_edu 的“全部消息 30 条/分钟”，否则
1 Hz 实时曲线本身就无法工作。

### 6.6 Broker/Gateway 强制项

- 真正执行 `max_publish_per_second`、`max_publish_bytes_per_second`；
- 维度至少包含 connection、logical device、account/tenant、target service；
- 按 message lane 分桶，ACK/协议控制不得和 Bulk 共用一个桶；
- frame size 在解析前限制，CBOR item/field/sample 数量在分配前限制；
- 慢 App consumer 使用 bounded queue，Telemetry latest-wins/降频；普通 Event 溢出明确中断该 stream/session；
- 暴露 accepted/coalesced/dropped/throttled/bytes 指标和可追踪错误码；
- 超限必须在协议处理、持久化和 fanout 前丢弃当前帧，同时返回/投递明确 `RateLimited`；不得只静默丢包；
- 单次超限不关闭 MQTT、路由或 TelemetryLease；10 s 内累计 8 次违规才按持续违规策略断开；
- `TelemetryClose` 只处理 consumer/ACL/session/lease 生命周期。最后一个 consumer 消失时先停止 fanout，再发
  Close；不能把 Close 当作临时拥塞控制。

---

## 7. 实时 Telemetry 协议

### 7.1 不复用当前 Event

当前 `Event(0x13)` 已冻结为瞬时、不合并、无 state revision 的 endpoint value map。连续采样若复用 Event：

- 无法携带 streamId、lease、sample sequence、采样时间和 batch；
- Broker 不知道哪些 App 页面真正订阅；
- 普通按钮/告警 Event 会与高频 sample 竞争同一语义和队列；
- 无法在不改变现有 Event 合同的前提下安全降频或覆盖旧值。

因此保留 Event 语义，新增独立 Telemetry message family。2026-08-22 冻结为三个 kind，避免把六个操作拆成六种
消息而放大三端状态机：

```text
TelemetryControl  0x17  {operation=open|renew|close, ...}
TelemetryStatus   0x18  opened/renewed/closed/expired
TelemetryData     0x19  单个 best-effort sample
FeatureTelemetry  bit 11
```

Feature bit 8 仍保留给已撤回的 WidgetCatalog，禁止复用。Telemetry 必须同时协商 Manifest 和 EndpointIds，三个
消息都只使用 ID mode。Cloud 使用现有 Route/Delivery 的 trusted source、target 和 requestId；Direct BLE/LAN
使用本 session。2026-08-22 D4b 软件核心完成后，Broker/App 才同时启用 bit 11 和三个 Route inner kind：Broker
使用有界 consumer registry 与单 device aggregate demand，App 使用页面可见性 lease manager；不是无资源上限透传。

### 7.2 Manifest 能力

Manifest 只增加设备数据能力，不增加 App 布局。首版每个 Field 只增加可选 key 11：

```text
11: telemetryMinimumIntervalMs  // 0/缺失表示不可实时采样，范围 100..60000
```

默认/最大间隔、batch 和 deadband 暂不进入首版 Manifest；设备全局上限和 Broker grant 已能完成安全 clamp，待真机
流量数据证明必要后再扩展。

App 的 Chart、数值卡片或 Agent 页面读取能力后决定是否显示“实时”入口。页面样式仍属于 PageSchema。

候选 Arduino API：

```cpp
BLINKER_PROPERTY(
    temperature,
    float,
    blinker::realtime(1000));

float readTemperature() {
    return sensor.readTemperature();
}

void setup() {
    temperature.onSample(readTemperature);
    Blinker.begin(..., temperature);
}
```

- 没有有效 lease 时不调用 `onSample()`；
- 未注册 `onSample()` 时可读取该 Property 已保留的 current state；未初始化则跳过本采样周期；
- 实际 interval 由设备 clamp 后返回；
- callback 不允许阻塞网络 poll；D4a 当前不伪造无法可靠测量的 callback deadline；
- D4a 使用每 field callback。group callback 只有真机数据证明重复采样成为问题后才增加，不能先扩大公开 API；
- 普通用户仍可手动 `temperature.report()`，这属于持久状态而不是实时 sample。

### 7.3 冻结 wire body

```text
TelemetryControl Open  = {0:0, 1:streamId, 3:leaseMs, 4:intervalMs, 5:[fieldId...]}
TelemetryControl Renew = {0:1, 1:streamId, 2:epoch, 3:leaseMs}
TelemetryControl Close = {0:2, 1:streamId, 2:epoch}
TelemetryStatus        = {0:streamId, 1:epoch, 2:status, 3:effectiveIntervalMs, 4:leaseMs}
TelemetryData          = {0:streamId, 1:epoch, 2:sampleSequence, 3:monotonicMs, 4:{fieldId:value}}
```

- 首版固定一个 sample/frame，不支持 batch；
- streamId 只在 session/route epoch 内有效；重连不得继续旧 ID；
- sample sequence 允许 App 识别 gap，但不要求重传已经过时的实时 sample；
- 设备没有可信 UTC 时使用单调时间/间隔，Broker 可附加 ingest time；不得伪造 UTC；
- scalar 可多 field 合帧；首版不编码等间隔 value array；
- QoS 0/best-effort，latest-wins；不能进入 ReliableOutbox；
- 每 10～60 秒或超过 deadband 时可另外发送普通 Property Report，保持 latest state/自动化可用；
- 页面关闭、App background、账户 MQTT 断开、ACL 撤销或 lease 到期都必须停止对应 fanout；最后一个 consumer
  消失后停止设备额外采样。
- Telemetry consumer/demand 不持久化，也不进入 command ledger。Broker 重启必须关闭旧 MQTT session、清空全部
  lease，并让 App/Device 重新完成 HELLO、Manifest gate 和 State 同步后用新 stream/epoch Open；旧 Renew、Status、
  Data 和 replay 一律忽略或拒绝。Broker 运行期 stream/epoch 从非零随机起点分配，避免重启后机械复用 `1`。

D4a 的 32 位 ABI 实测门禁为：Small/Standard/Large `Device` 分别 1664/3448/10980 B，对应 2/2/4 个 lease；
`Property<bool>` 保持 16 B，`EndpointSlot` 保持 8 B 并复用 command/sample callback union，没有新增逐组件 RAM 指针。
Small 的第二槽用于 Cloud 聚合流与 Direct 流并存，代价为 36 B；不会复制 sampler、payload 或 transmit scratch。
Large 档位不面向 32 KB SRAM 的 UNO R4，Large ABI 改由 ESP32 资源目标验证，避免测试名称暗示错误的平台能力。

### 7.4 App 生命周期

```text
page enter / chart visible  -> TelemetryControl(Open)
TelemetryStatus(Opened)     -> 开始绘图
foreground + visible        -> 定期 TelemetryControl(Renew)
page leave / chart hidden   -> TelemetryControl(Close)
app background / logout     -> 关闭本 session 全部页面 stream
disconnect/crash            -> Broker 删除 consumer；设备 lease 到期兜底
```

App 不得因为普通 MQTT PING、收到其他设备消息或用户仍登录，就续期当前页面的 telemetry。

---

## 8. History 与大流量 Bulk

### 8.1 History 不是实时流

设备可在本地按自己的采样计划记录数据，即使没有 App 在线。上传时使用当前 reserved experimental
`HistoryRequest/HistoryBatch` 的方向，但在正式实现前重新冻结：

- 每条记录含 field、采样时间/相对时间和值；
- batch 带 cursor/range，服务端提交后 ACK 下一 cursor；
- 失败保留未确认记录，稍后继续；
- 允许按时间窗、最大样本数和最大 frame 分页；
- Broker 可生成降采样/聚合，不要求设备保存无限历史；
- History 使用后台低优先级预算，不依赖 App TelemetryLease。

### 8.2 何时进入 Bulk

满足任一条件就不能继续伪装成 Property/Event：

- 单对象超过协商 `maxFrame`；
- 数据必须拆为多个 block 才能完成；
- 需要断点续传、文件校验或版本一致性；
- 连续速率超过 Telemetry grant；
- 数据是日志包、文件、图片、音频、视频、固件或高频波形。

### 8.3 Bulk 控制面与数据面

```text
BBP/2 control:
  BulkOpen / BulkAccept / BulkProgress / BulkClose / BulkAbort

data plane:
  streamId + objectId/version + offset + length + block + checksum
```

- 下载优先采用设备 pull：设备一次请求一个/少量 block，天然限制 RAM；
- 上传采用小窗口 push，服务端 ACK contiguous offset，断线后从确认位置继续；
- 每 block CRC/AEAD 完整性，完整对象使用 SHA-256/签名；
- 设备只需要一个 caller-owned block buffer，不缓存完整对象；
- Cloud 大对象优先走专用 HTTP/object storage 或受控 MQTT file stream；
- LAN WiFi 可协商 TCP/WebSocket；BLE 只承载小文件/低速 fallback，保持一个很小的 in-flight window；
- Bulk 必须有 object/stream grant、长度上限、过期时间和目标权限，不接受任意 URL/路径；
- Bulk 低优先级，可被 Command、ACK、State 和连接维护抢占。

第一阶段只冻结边界，不立即把音视频、通用文件系统或对象存储客户端塞进 Arduino 核心库。

---

## 9. 设备调试与诊断方案

### 9.1 对外 API

```cpp
void setup() {
    Serial.begin(115200);
    Blinker.debug(Serial);  // 默认 Info
    Blinker.begin(...);
}
```

高级级别：

```cpp
Blinker.debug(Serial, blinker::LogLevel::Debug);
Blinker.printDiagnostics(Serial);  // 一次性状态和计数
const blinker::DiagnosticCounters& counters = Blinker.diagnosticCounters();
blinker::TelemetryCounters telemetry = Blinker.telemetryCounters();
```

不要求用户实现 `ILogger`，也不要求为诊断安装 Python。

### 9.2 内部事件而非动态字符串

```text
DiagnosticEvent {
  level,
  componentId,
  eventCode,
  timestamp,
  argument0,
  argument1
}
```

当前 D1 component 为 `library/product/state/event`。后续在真实状态机接入时再扩展
`storage/wifi/http/session/mqtt/bbp2/manifest/route/ble/scheduler/telemetry/bulk`。核心和平台 port 只产生固定
事件码；Arduino `Print` formatter 在用户启用 debug 时才链接文本表。

当前已实现的无字符串计数：

```text
lifecycleTransitions / operationFailures / wouldBlock
stateChanges / stateUnchanged / eventsSent
outputSuppressed / outputDropped
telemetrySamples / telemetryDropped / leaseExpired
```

`telemetrySamples` 使用 32 位饱和计数；异常路径 `telemetryDropped/leaseExpired` 使用 16 位饱和计数。后两者达到
65,535 后保持最大值而不回绕，以 8 B 固定记录保持 Small `Device` ABI；它们是启动期诊断，不是持久计量数据。

D2 及后续按真实实现增加：

```text
wifiConnectAttempts / sessionRefreshes / mqttReconnects
txAccepted / txCoalesced / txNoChange / txDropped / txWouldBlock
eventThrottled
reliableRetries / reliableFailed
bleFragments / bleReassemblyTimeouts / protocolRejects
```

### 9.3 日志规则

- 默认发布固件关闭连续输出；`lastError/status/counters` 仍可读取；
- `Info` 只记录状态迁移，不在每次 `run()` 重复打印；
- 相同 Warning/Error 使用抑制窗口，并在下次输出 `suppressed=N`；
- Debug/Trace 使用 100 ms 最小输出间隔；pending 只保留最新同级 verbose 事件，不建立通用 ring buffer；
- 不在 ISR 打印，不在持锁、MQTT callback 或 BLE stack callback 中做长格式化；
- formatter 不使用动态 `String` 拼接，不建立 JSON DOM，不复制完整 frame；
- Trace 即使启用也只显示 kind/flags/sequence/length/field count，原始 body 必须另有显式开发开关；
- Serial 输出耗时计入独立 log budget，不能破坏 `Blinker.run()` 的网络预算。

永远不得输出：

- DeviceKey、WiFi password；
- Enrollment/Recovery/Controller credential；
- MQTT/session password、JWT、HMAC proof；
- WiFiProv PoP、Security2 salt/verifier；
- 完整认证 frame、含 secret 的 URL/header/body。

允许输出脱敏 ID：固定前/后少量字符、不可逆 digest 前缀、credential version、generation 和错误码。

### 9.4 资源门禁

每个平台必须比较 `logs off / Info / Debug` 的 Flash、静态 RAM、峰值栈和 `run()` 延迟：

- logs off 不应链接 formatter/大字符串表；
- Small profile 不增加通用日志 ring buffer；必要时只保留最近事件码和计数；
- Debug 串口阻塞不得导致 MQTT heartbeat、BLE reassembly 或 reliable deadline 失败；
- 生产 ZIP 示例只展示 `Blinker.debug(Serial)`，不默认打开 Trace。

---

### 9.5 D1b 最小背压实施边界（2026-09-12，尚未实施）

本节保留立项时边界；同日实施结果及剩余平台限制见 §9.6，不再以本节标题判断当前进度。

B2b-6 已完成 App 普通 pending 恢复；D1b 是下一小工作包，不把可选日志问题改写成 BLE 需要更多重试或新安全协议。

本机 SDK 源码核查：`Print::availableForWrite()` 默认返回 0，并注明单次 write 可能阻塞；HWCDC 的 `availableForWrite()` 自身也会按 `tx_timeout_ms` 等待锁。因此“容量检查一次后照旧逐字段 Print”不构成可证明的非阻塞输出。`debug(Print&)` 同步 formatter 的事实不变，不能据日志重复抑制/100 ms verbose 间隔声称有执行时长保障。

最小实施约束：

1. 核心继续只记录原 16 B DiagnosticEvent、计数和有界 pending；传输/协议不解释日志。平台适配负责输出准入，formatter 只负责事件到文本，不增加设备组件字段、日志网络协议、大 ring buffer 或后台任务。
2. 明确“可证明无等待的 try-write”与“未知/同步 Print”两类能力。不能在库内偷偷改变用户共享 Serial 的全局 timeout，也不能把普通 Print 强转某一 MCU 类型；优先适配一个已核对语义的平台输出，再增加 R4/Wio 对应实现。保留用户启用日志的简便入口，但未适配输出的能力限制必须可见，不能默认为受保护。
3. 支持容量不足、0 写入和短写；每次 run 的日志工作有界，无循环等待/立即重试。资源不足可以丢诊断并计数，不影响通信。若为分步输出保留游标，只复用单个 pending 事件，不预留任意长度文本队列；ABI、栈和代码大小需实测后确定，不提前承诺数值。
4. 先 host fake sink 证明满/断连/短写/重入的行为，再做 ESP32 有读者与无读者的短对照（Hello、Property、Action、run 延迟）。日志 off 的行为与资源不能回退；其他平台未通过前不写“多平台全支持”。原 stdout/Print 模拟测试不能替代真实 USB 背压。

本批只完成原因与边界核对，未新增 sink API 或更改 Diagnostics ABI；接口最终签名和具体平台实现须在该工作包核对后冻结。B2b-5 静默测试固件保持，不重新开启连续打印来制造长超时。

### 9.6 D1b 首个平台闭环：单次输出、显式能力与真实背压（2026-09-12）

**本工作包完成可移植核心/formatter/首个显式 IDF USB 适配及 ESP32-S3 短真机闭环；不等于任意 `debug(Serial)` 已非阻塞，也不要求普通 Arduino 用户切换 USB 驱动。** 主线现在返回混合 WiFi+BLE 联合退休/B1，R4/Wio/Arduino HWCDC 的自动安全适配独立列后续，不继续扩日志系统。

实现与边界：

1. 原 `DiagnosticSink` 改为返回 `bool`（完整接收=true）；拒绝计入原 `outputDropped`，不重试。低层自定义数值 sink 需补返回值，普通 `debug(Serial)` Sketch 无需改签名。核心仍只有原 pending/重复摘要事件，每次 `flush()` **最多一次** sink 调用，摘要留到后续 flush；输出前消费旧状态，回调内记录/disable/configure 不被旧回调尾部覆盖，递归 flush 被挡住。核心仍要求单应用执行上下文，不是新建跨任务队列或线程安全日志器。`outputDropped` 同时包含 pending 竞争与输出拒绝，不等于 USB 包或业务消息的丢失数量。
2. Arduino formatter 只向栈内 192 B `DiagnosticLine` 写文本，完整格式化后才交给输出一次；溢出在触碰 I/O 前丢弃。旧逐字段外设 Print 输出已移除，Print 与新输出共用同一 formatter，不维护两套文本。`Print::write(buffer,size)` 内部仍可能逐字节阻塞，不能据“外层一次调用”承诺延迟。
3. 保留简便兼容 API `Blinker.debug(Print&, level)`，**明确同步、无时间上界**；`printDiagnostics(Print&)` 是用户主动调用的同步快照，不能作为连续 loop 日志。新增 `Blinker.debug(IDiagnosticOutput&, level)`，只有一个平台无关 `tryWrite(bytes,size)` 合同；适配者保证不等待锁/容量、不保存指针、不 flush/循环重试、不修改共享 I/O 配置。输出对象存活至 noDebug/替换。短写丢剩余字节并计一次事件丢失，可能出现不完整诊断行，不追加后缀重试；这不是 BBP/MQTT 消息队列，也不是远端收到日志的 ACK。
4. `Esp32UsbDiagnosticOutput` **仅借用已安装的 ESP-IDF USB Serial/JTAG driver**，未安装时拒绝；一次 `usb_serial_jtag_write_bytes(...,0)`。库不安装驱动、不接管 Serial、不修改全局 timeout；驱动生命周期不能与 tryWrite 并发。Arduino HWCDC on boot 与该适配器显式编译互斥，手动 HWCDC 也不得占同一外设。只适用于具备 USB Serial/JTAG 的 MCU，不是 ESP32 全系列/UART/TinyUSB/R4/Wio 通用适配。
5. `DiagnosticOutputGate` 是 `tests/arduino` 内的专用纯 BLE 测试固件，显式关闭 Arduino CDC on boot，自行安装 IDF driver（TX 256 B、RX 128 B）并制造约 500 次/秒数值日志；**不是把这套安装步骤加给 QuickStart 用户**。driver 自身缓冲和锁有动态资源，未宣称“整条输出链零堆内存”；库没有新增常驻文本 ring、后台任务、协议字段或云端日志服务。

SDK 依据：本机 Arduino ESP32 3.3.11 的头文件为 **ESP-IDF 5.5.5**；核对 [IDF v5.5.5 driver 源码](https://github.com/espressif/esp-idf/blob/v5.5.5/components/esp_driver_usb_serial_jtag/src/usb_serial_jtag.c)，write 的 mutex 与 ring send 都使用调用方等待值，零值失败即返回；不经过 Arduino HWCDC 的共享 timeout。对照 [Arduino 3.3.11 HWCDC](https://github.com/espressif/arduino-esp32/blob/3.3.11/cores/esp32/HWCDC.cpp) 说明为什么没有把 `availableForWrite()` 包装成通用安全方案。本批没有修改 SDK 或新增 NimBLE fork；未来其他 SDK 仍需按相同 driver 合同编译/核对，不按一个版本号推断全部串口安全。

软件/资源门禁：

- Win32 strict warnings host **57/57 目标通过**；新增诊断专项包含 **1,889 条断言**，不是 1,889 个独立故障场景。覆盖满/未安装/断连/0/短写、无立即重试、重复摘要单步、重入及重新配置、millis 回绕、formatter 最大值与溢出、原同步 Print bulk 交付、IDF adapter 零等待参数。
- 32 位 `DiagnosticEvent=16 B`、`Diagnostics=92 B`，后者与修改前相同；Win32 stub 下 `DiagnosticLine` 对象 204 B（其中文本 192 B），只在日志输出栈上存在，真实 Arduino Print 基类大小由 SDK 决定。已知码最大文本 169 B。ESP32 `-fstack-usage`：outputEvent 自身 256 B、flush 48 B、deliver 32 B；**不是完整调用链/中断栈上界**。
- logs-off 的 BleLifecycleGate 仍为 **693,124 B 程序 / 34,848 B 静态 RAM**，与原静默固件一致；最终 ELF 无 formatter/DiagnosticLine 符号。IDF 专用日志固件为 **689,248 B / 34,648 B**，但串口驱动与编译设置不同，不能用两者差值声称开启日志节省资源。
- 架构门禁通过。Facade freeze v36 仅纳入 debug 重载/注释，不改 begin、Key、登录 wire。全量 Win32 构建发现原 GatewayChildExecutor 容量比较有 signed/unsigned 告警，最小修正为 size_t 减法边界，避免先加 header 的溢出表达式；未扩网关行为。

真实 COM11 / Android 原账号、原授权：

| 条件 | Ready（扫描/认证/Hello/同步合计） | Property / Action ACK | 输出证据 |
| --- | --- | --- | --- |
| 同一串口句柄暂停读取 25 s | 2,906 ms | 242 / 270 ms | 新增丢弃 12,407；outputMax=114 μs，runMax=4,859 μs |
| 保持句柄，恢复读取 28 s | 2,893 ms | 236 / 247 ms | 读取 14,014 行，新增丢弃 2；outputMax=179 μs，runMax=4,672 μs |
| 恢复最新静默测试固件 | 2,832 ms | 239 / 241 ms | 原逻辑 ID/授权直接连接，Manifest 3 字段，Action=1 |

配对测量两阶段 Action 计数严格 **0→1→2**，没有阶段内复位或重复 Action；runMax 含 BLE 协议工作，不等于日志时长。之前独立开关串口的样本出现计数归零，因此不拿它证明同次启动的对比。测试 helper 初次命名 `serial.py` 与 pyserial 冲突，所谓 reader 样本当时实际没有读者；保留原证据但不计为有读者通过，已改名并由同句柄配对门禁替代。不能把一次最大值称为硬实时/长期稳定性保证。

先核对 COM11 MAC、系统/分区和原静默应用，随后只刷 app0；没有擦 NVS、清授权、重新认领/SQL 改权或触碰 COM3/COM12 固件。测试后恢复包含本批核心的静默固件 SHA-256 `4f1c3379861a659be0a55e60dc4b94e903cbf0ed90a5808b5c294c5745fec780`，手机回 home/cloud ready。COM3 独立 MQTT Property **117 ms**、Action **123 ms**，原账户 MQTT 观察段不变、见证已关闭；这是独立 WiFi 回归，不是同一 MCU 的 WiFi+BLE 日志组合压测。

私有证据：`build/d1b-output-20260912/`（host/构建、SDK 参数、应用备份核查、固件 SHA、配对测量、App ACK、WiFi 见证），不进入发布包。App/Service 无本批源码或部署变化，无暂存/提交/推送。

### 9.7 普通 WiFi 同步 Print 示例收口（2026-09-13）

CloudSmokeTest 原来默认 debug 并每 10 秒 printDiagnostics，属于通用同步 Print，不是 §9.6 的 IDiagnosticOutput。
COM11 同版本/身份/profile 对照复现：停止串口读取后控制失败，恢复读取见云端重连；这不是授时 TTL 太短，
也不能通过延长 keepalive 掩盖。现删除默认 debug、周期输出和旧定时变量，仅显式串口 `d` 请求一次诊断/UTC。
请求期间仍需读取，不声称任意 Print 非阻塞；没有新增日志队列、SDK timeout 设置或第二 USB driver。

修订例程在关闭读取 45 秒后真实控制通过；无读取期间采样启停/恢复 7→7→14→14、设备计数 14、UTC 有效，
productError=0。单 ZIP 编译/链接通过，程序减少 1,400 B、全局 RAM 不变。详细有限对照及边界见
[授时 §9](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#9-b3-3b日志对照与受控离线2026-09-13本批执行)。
这只修正公开示例的默认行为；§9.6 各平台非阻塞输出准入仍独立，不把两种 logger 的证据混合。

## 10. 平台方案对照与可借鉴点

### 10.1 Blynk

官方文档明确警告不要在 `loop()` 中直接 `virtualWrite()`，建议使用 `BlynkTimer`；超过约 10 values/s 可能触发
Flood Error 并断开硬件。库配置同时有独立 heartbeat 和每秒消息上限。Blynk MQTT/HTTP 支持多 Datastream
batch 和 timestamped batch，用于减少消息和 TCP 开销。

可借鉴：

- 对初学者给出明显的安全默认行为和诊断错误；
- 设备软限制 + 云端硬断开；
- 多字段批量和带时间批次；
- heartbeat 与业务 flood limit 分开。

不直接照搬：

- 把正确性主要交给用户定时器；
- 所有消息使用一个固定频率上限；
- 每次 `virtualWrite()` 立即发送而不做状态 latest-wins 合并。

### 10.2 涂鸦

涂鸦 DP 模型区分主动/被动上报、Cloud/Panel/两者和是否需要 ACK。WiFi MCU 方案建议状态变化时上报、过滤
相同 DP；确需重复上报时建议不短于 1 秒，并支持多个 DP 合成一个包。TuyaOS 还区分实时 DP 与 record-type
数据，失败记录需要缓存后重试。MQTT Keep Alive 独立配置为 30～1200 秒，常见 60 秒。IPC 音视频则使用独立
ring buffer、P2P/WebRTC/cloud-forward 体系，不塞入普通 DP。

可借鉴：

- change-only、批量 DP、异步队列；
- 数据目标（panel/cloud/both）和可靠性显式化；
- 实时、记录、大媒体分层；
- transport keepalive 不控制业务上行；
- 模块化日志级别和输出 sink，发布版本裁掉 Debug/Trace。

不直接照搬：

- 非传感器 300 次/天、传感器 3500 次/天等产品建议不能成为 Blinker 通用协议硬限制；
- Raw DP 不是通用大文件通道；
- 产品 PID/云端预定义 panel 不适合直接替代当前用户页面与 Manifest/PageSchema 分离模型。

### 10.3 小米/MIoT

MIoT 使用 Property/Action/Event 和 notify 能力，Mesh 网关在 Mesh Spec 与 MIoT Spec 间转换。官方 Mesh 文档
要求自定义 Vendor Service 只上报有意义的数据、保持合理间隔、状态变化时上报，不允许周期性无意义 publish。
公开插件 SDK 的属性变化订阅具有字段列表、显式 `unsubscribe` 和最长 3 分钟的 `expire`。

可借鉴：

- Manifest 继续保持 Property/Action/Event 语义，而不是绑定具体 widget；
- App/服务订阅自己关注的字段，避免轮询全部设备；
- 短租约 + 显式取消，崩溃后自动回收；
- Gateway 完成本地协议与统一模型转换。

边界说明：公开的小米订阅 API证明了“消费者订阅应有限期”这一设计，但不能据此断言小米固件会在 App 页面关闭后
停止所有属性上报。Blinker 将短租约用于**额外实时采样和定向 fanout**，不是照搬或推测其内部设备实现。

### 10.4 AWS IoT（补充）

AWS Device Shadow 将 desired/reported/delta 与递增 version 分开，说明可恢复设备状态不应和实时流混合；
MQTT file delivery 用 stream ID/version、DescribeStream/GetStream 和按 block 请求，说明大对象应使用独立、可恢复的
传输合同。

可借鉴：

- State revision/snapshot 与临时 telemetry 分层；
- Bulk 先协商 stream 元数据，再由小缓冲设备按块拉取；
- object version 防止断点恢复时文件已被替换。

---

## 11. 三端实施方案

### 11.1 Device library

| 批次 | 内容 | 完成门禁 |
|---|---|---|
| D1（代码完成） | 用紧凑 DiagnosticEvent + optional Arduino Print formatter 替换未接线 ILogger | secrets/host gate、off/info/debug size、三平台完整 WiFi 产品编译已过；Wio diagnostics、WiFi ready 和 Plain CloudSmoke 真机已通过，RTL8720 为官方 v2.1.3 |
| D2（设备代码完成） | 统一 OutboundScheduler、dirty state batching、普通 Event token bucket、lane counters | Host 已过同值/噪声/loop flood、revision/snapshot、scratch 分页、transport backpressure 和 millis 回绕；StateRequest 请求 peer 排除前置 Patch、非 canonical 注册顺序的 keyed Snapshot/Selection/Page 已有回归；Wio 三字段压力 Cloud active 已通过 |
| D3（代码完成） | 禁止 inbound-driven `isAlive`；冻结零常驻槽同步 RequestScope | 多 source、错误 target、peer close、replay 与主动广播 Host 门禁已过 |
| D4a（设备核心与 Wio Cloud 真机完成） | 三 kind wire、固定槽 TelemetryLease、Property sampler、transport-independent 调度 | exact CBOR、open/renew/close/expiry、session close、BLE/cloud 相同 streamId 双 source、逐 route epoch/sequence/target、单 route 断开隔离、公平处理全部 due 槽、固定槽/位图、丢旧/sequence gap、Cloud RateLimited exact-source 退避且不阻塞 Direct、诊断计数，以及 Wio 1 Hz Open/Data/Close/断路 expiry/Cloud 恢复已过；单次 State RateLimited 持续在线和第 8 次断开后自动重连均已真机通过 |
| D4b（页面软件链与本地真实协议通道完成） | Broker 聚合租约、App 可见性 lease manager、三端 exact codec、Route/Delivery 与 V2 typed 页面 binding | 已启用 FeatureTelemetry；App `DeviceV2Session` 经账户 MQTT 通道完成 Wio Manifest/realtime 字段识别与连续 1 Hz Data。页面退出/前后台组合测试、独立 App 协议进程强杀、受控双账号公共分享 API 撤权即停流，以及 1/8/32 consumer、慢消费者和 Broker 重启恢复均已过；真实 Android/iOS UI、线上账号与真机 Cloud/Direct simultaneous 仍是发布门禁 |
| D5 | HistoryBatch 实现 | 时间/cursor/重试/断电恢复/服务器幂等 |
| D6 | 可选 Bulk adapter，不进入 Small 默认闭包 | 单 block RAM、CRC/SHA、resume、优先级抢占 |

### 11.2 Broker/Gateway

| 批次 | 内容 | 完成门禁 |
|---|---|---|
| S1（代码与 Wio 真机完成） | packets/s、bytes/s、packet size 多维/分 lane 硬限制 | Redis 原子共享计数、SQLite 租户策略投影、RateLimited(12)、metrics 与跨实例/重启门禁已过；TelemetryData 已进入独立有界 lane，TelemetryStatus 使用 response reserve；state-sync readiness 与 telemetry readiness 已分离；Wio 单次拒绝持续在线、恢复采样及 8-strike 重连已过 |
| S2（代码完成） | per App MQTT session consumer registry + 5～60 s lease expiry | session remove/ACL invalidate/lease sweep 会清 consumer；pending Open 被取消后保留一个新的 device-lease 周期，迟到 `Opened` 必须补发 exact Close；慢 consumer 的同 stream 待发 sample 为 latest-wins |
| S3（软件容量/恢复与 Wio 1 Hz 门禁完成） | 多 consumer 合为单 device demand，字段并集、最快间隔，按 consumer 降频和 ACL fanout | 1/8/32 consumer、第 33 个拒绝、慢队列 latest-wins、lease expiry、两个隔离账户只撤销指定 consumer、重启恢复已过；Gateway 权威 ACL 撤销通知带 grant version，旧/乱序通知不能拆掉较新的 gate；fanout 权威复核失败会同步删除 consumer。降频使用设备 `monotonicMs` 并有 999 ms 到达抖动回归，避免误降为 0.5 Hz；线上账号和 automation consumer 仍待联调 |
| S4（代码完成） | Telemetry ingest 与 latest-state/history 隔离 | sample 不推进 state revision、不写 latest DB；History repository 后接 |
| S5 | durable event ingest 和 History repository | eventId/cursor/idempotency/retention |
| S6 | Bulk grant、object metadata 和 block endpoint | length/quota/version/checksum/resume |

S1 当前实现边界：connection 使用进程内 token bucket；tenant、logical device、target 使用
Redis Lua 原子共享 bucket。managed Broker 启动时必须连接共享 Redis，运行期 Redis 异常则
fail-closed；standalone 无 Redis 时明确使用内存 bucket，重启会重置计数。Gateway 在每次
DeviceKey session 注册时把 `tenant_limits` 投影到 Broker SQLite；Broker 重启会恢复策略。
tenant 迁移包暂不复制该策略，目标 Shard 在下一次 session 注册前使用安全默认值。

2026-08-23 Wio 门禁冻结的两个服务端修正项已完成代码和软件回归，设备库不得用更长 sleep 或平台专用逻辑掩盖：

1. `PublishLane` 已增加明确 `telemetry` lane，`TelemetryStatus` 作为响应/控制，`TelemetryData` 使用 8 frame/s、
   32 KiB/s 的独立有界预算；持续采样不再占用 `control` reserve，ACK/Error/HELLO/租约控制仍受保护。
2. State Patch 被 RateLimited 丢弃后，后续 revision gap 会触发 `startStateTransfer()`。此时 Manifest 和已建立的
   TelemetryLease 仍有效；Broker 已把 state-sync readiness 与 telemetry readiness 分开。首次 State snapshot 前仍拒绝
   遥测，完成过初始化后则允许重同步窗口内的既有 TelemetryData 和 Renew/Close 继续处理。软件回归已覆盖
   “StatePage ready → revision gap → state not-ready/telemetry ready → TelemetryData 接收”。Wio 单次 State 拒绝时
   Broker 指标仅增加一个 state throttled，command/telemetry 不增加；设备、MQTT 和 App session 保持在线并恢复采样。

S1 限流动作现正式冻结为两条互补防线，而不是“关闭转发”和“命令设备停止”二选一：Broker 对每个超限 frame
执行 hard drop，正常客户端依据 `RateLimited(12)` 对当前 Cloud 连接的非关键业务退避 1 s。设备 State 保留 latest，
Event 不补发，Telemetry 丢旧并延后同 source 下一采样；ACK/Error/租约控制保留。普通超限不发送
`TelemetryClose`。页面退出、App 断连、ACL 撤销和 lease expiry 才删除 consumer；最后一个 consumer 消失时
Broker 立即停止 fanout，并向设备发送 Close，30 s device lease 仅作为丢包兜底。

当前 Error body 只有 `errorCode + relatedSequence`，不携带 lane/scope/retryAfter；设备因此采用固定 1 s 的
connection-level 保守退避。State/Event 调度器是逻辑设备级，罕见 Cloud 超限期间可能让 Direct State/Event 同样
延后最多 1 s；Direct Telemetry 已按 source 隔离。该限制不影响 WiFi-only Wio S1 门禁，但必须在正式
WiFi+BLE Cloud/Direct 真机发布门禁中单独验证，不能宣称所有业务 lane 已 exact-source 隔离。

### 11.3 App

| 批次 | 内容 | 完成门禁 |
|---|---|---|
| A0（完成） | 同步 Error(1～12)、512 B 上限和可扩展 header 解析 | live session 已接受 HELLO 后的顶层 `Error(IsResponse)`；按 `relatedSequence` 只结束对应 pending Route，保持 session ready，Command/Event 不自动重试。匹配、迟到/重复和非法 metadata 回归已通过；真实 App MQTT/Broker/Wio 在 1 frame/s 注入下命中 3 个关联错误，恢复预算后同 session Command ACK 已通过 |
| A1（前后台软件链与外部进程断连门禁完成） | transport-independent lease manager 提供串行 Open/Renew/Close；共享 AppVisibilityService 经 typed port 组合 page enter/leave、App background/foreground、账户断线和重连 | 组合生命周期及 Close 尚未完成即恢复可见的竞态门禁已过，后台不停止账户 MQTT；使用正式 App V2 session/MQTT 代码的独立进程被 OS 强杀后，Broker socket 与 consumer 立即回收。真实 Android/iOS background/kill/network switch 仍待设备实验室门禁 |
| A2（基础 binding 完成） | 当前 PageLayout 中明确展示的只读 realtime Property 自动进入页面 lease | 首版使用 `max(1000 ms, field minimum)`；Chart ring buffer、手动拖拽/Agent 的刷新率配置仍待实现 |
| A3（部分完成） | stream/epoch/sequence 去重与每 lease 最新 sample store | gap UI、图表有界 ring buffer 与慢手机真机门禁待完成 |
| A4（Telemetry 分离完成） | 普通 Property/Event store 与 Telemetry lease store 分离 | 实时 sample 不污染当前状态和事件通知；History 后接 |
| A5 | 用户可导出的脱敏诊断报告 | 不包含 Key/password/token/raw auth frame |

---

## 12. 测试矩阵

### 12.1 发送治理

- `loop()` 每次 report 固定值：只产生首次有效状态；
- `loop()` 输入噪声：按窗口合并，frame/byte 不超过 profile；
- 8 个 Property 同周期：一个 batch Patch、一个 revision；
- pending 期间 StateRequest：强制形成一致 revision，再分页；
- MQTT 在线、BLE 队列满：Cloud 不被 BLE 阻塞，BLE peer 后续 resync；
- Broker 返回 RateLimited：Host 已验证设备退避且保留 state 最新值；同一 Cloud telemetry source 延后采样、
  不重放过时 sample，Direct source 继续发送，1 s 后 Cloud 从下一 sequence 恢复；Wio 单次门禁只增加一个
  state throttled，设备/MQTT/App session 持续在线并随后连续接收 5 个 1 Hz sample；持续 8 次违规断开后设备自动重连；
- 单次 RateLimited 只丢当前 frame，不生成 TelemetryClose、不关闭 MQTT/路由；页面/ACL/lease 结束的 Close
  必须作为独立门禁测试，不能用一次压力测试同时证明；
- ACK/Error 在 telemetry/bulk flood 下仍满足 deadline。

### 12.2 keepalive/lease/路由

- 仅 MQTT PING 不创建 TelemetryLease；
- owner App 的消息不授权 share App 或 DeviceRoute；
- 两个 App 不同字段/速率订阅，设备只采一次并由 Broker fanout；
- 页面 Close、App kill、账户断开、ACL revoke、lease timeout 都能停止对应流；
- 自动化服务订阅不因手机页面退出而停止；
- 无入站消息时设备仍可发送状态和告警；
- Device A→B 和 Device→Service 使用明确 target，不受最后 App sender 影响。

### 12.3 Telemetry/History/Bulk

- sample sequence gap 可见但不触发 state resync；
- telemetry 不改变 state revision；
- periodic state summary 仍可恢复最新值；
- History cursor 重试不重复持久化；
- Bulk 中途断电/换连接后按 confirmed offset 继续；
- 大流量进行中仍可处理 Command、MQTT keepalive 和 BLE control；
- 所有长度、sample 数、lease 数、block/window 在分配前拒绝越界。

### 12.4 资源与日志

- ESP32、UNO R4 WiFi、Wio Terminal 的 logs-off/Info/Debug Flash、RAM、stack 对比；
- Small profile 1 stream + state batching 不突破资源文档门禁；
- 连续 Trace 输出不会导致 MQTT heartbeat timeout；若无法保证，必须丢日志而不是丢控制消息；
- fuzz frame/payload 不会把 secret 写入日志；
- 设备日志、Broker correlation、App session 可用非敏感 event/request ID 对齐。

---

## 13. 执行顺序和当前不做项

正式顺序：

1. D1 diagnostics；它是后续真机限流/租约调试的基础；
2. D2 + S1 发送治理，先关闭现有无限变化上报和服务器限流缺口；
3. D3 清除全局 keepalive 歧义，冻结 request context；
4. 冻结 Telemetry wire candidate 和 shared golden，再并行 D4/S2/S3/A1；
5. 完成 1 Hz 真机三端和多账号/服务订阅后，再评估 5～10 Hz；
6. History 独立实施；
7. Bulk 先实现一个明确用例（建议 OTA/file download 或诊断日志 upload），不做无边界通用流。

当前执行点：**第 5 项的 Wio 1 Hz Cloud、设备断路 expiry、Cloud/Direct 双 route Core 隔离、页面退出、
App 协议进程强杀回收、本地真实 ACL 撤权即停流，以及 6 个独立 WiFi/BLE 产品目标和 2 个 ESP32
WiFi+BLE composition 安装编译门禁已完成；D1～D3 不再扩展，D5/D6
暂缓。ESP32 WiFiProv + WiFi Cloud + NimBLE Direct 唯一正式业务 composition 的设备源码与静态门禁已完成；
默认 SoftAP，IDF-NimBLE 目标可选 BLE bearer。
当前只有 Wio Terminal 可真机测试：设备侧已完成 StateRequest 顺序、canonical keyed 编码、单次 RateLimited
持续在线、第 8 次违规重连，以及独立的最后 consumer Close 门禁；其中 Telemetry 是 exact-source，State/Event
仍是当前逻辑设备级保守退避。下一批保持标准 CloudSmoke 和跨平台编译稳定，不进入 D5/D6。
**App 由独立维护主线继续 WiFiProv coordinator、页面与移动生命周期；ESP32 替换板到位后再验证
SoftAP/BLE WiFiProv → Cloud、NimBLE Direct 同时在线、heap、超时复位和反初始化。服务端 Telemetry lane 与
state-resync readiness、Wio 限流持续在线以及 App 顶层 RateLimited live-session 门禁已经完成；后续只做移动端
生命周期与替换 ESP32 板门禁，不再继续扩散协议。**
移动门禁仍需移动设备和 ADB/Xcode 环境，但不阻塞设备侧清理，也不以扩展协议或 History/Bulk 代替它。

> **2026-08-23 主线收口补充：**D1～D4 的既有结论保持，但下一批不再继续扩展 telemetry，也不进入 D5/D6。
> 全库复核发现纯 BLE 三端 enrollment、WiFiProv custom bootstrap、生产 host/key 与 Arduino 91 个普通 `.cpp`
> clean-build 成本仍是更高优先级阻断。执行顺序以 `DEVICE_V2_ARCHITECTURE_CLOSURE_AUDIT.md` C1～C4 为准；
> BLE/WiFi+BLE 在三端闭合前统一标记 preview。

当前不做：

- 不把 Event 改名后冒充 Telemetry；
- 不让 App 使用设备 MQTT credential；
- 不允许设备通过 MQTT keepalive 或“刚收到任意消息”获得业务授权；
- 不为每个平台复制 timer/limiter；
- 不在 Arduino 核心库引入数据库、文件系统抽象、音视频/WebRTC；
- 不默认持久化每个 1 Hz sample；
- 不用一个永久订阅替代 App 页面、自动化服务和 DeviceRoute 的独立 ACL；
- 不在 exact golden 前给新 MessageKind 分配 production 编号。

---

## 14. 官方参考资料

- Blynk：[Limitations and Recommendations](https://docs.blynk.io/en/blynk-library-firmware-api/limitations-and-recommendations)、
  [Message Usage / Batch](https://docs.blynk.io/en/message-usage)、
  [MQTT Datastreams and timestamped batch](https://docs.blynk.io/en/blynk.cloud-mqtt-api/device-mqtt-api/datastreams)、
  [Debug](https://docs.blynk.io/en/blynk-library-firmware-api/debug)、
  [BlynkConfig.h](https://github.com/Blynk-Technologies/blynk-library/blob/master/src/Blynk/BlynkConfig.h)
- 涂鸦：[Open MQTT Protocol](https://developer.tuya.com/en/docs/iot-device-dev/MQTT-protocol?id=Kb3nzwbv28sgk)、
  [DP Development Considerations](https://developer.tuya.com/en/docs/iot-device-dev/TuyaOS-iot_abi_dp_attention?id=Kd1m6unb25gck)、
  [DP Data Processing](https://developer.tuya.com/en/docs/iot-device-dev/DP_WiFi?id=Kawmjgn3w3mem)、
  [BLE DP Data Communication](https://developer.tuya.com/en/docs/iot-device-dev/bluetooth_software_map_bt_dp_data?id=Kcmeae40r8zdq)、
  [Log Service](https://developer.tuya.com/en/docs/iot-device-dev/TuyaOS-iot_abi_log?id=Kcspz3t0azbqd)
- 小米：[MIoT Spec 文档仓库](https://github.com/MiEcosystem/miot-spec-doc)、
  [Mesh 与 MIoT Spec 映射/上报约束](https://github.com/MiEcosystem/miio_open/blob/master/ble/04-%E8%93%9D%E7%89%99Mesh%E6%8E%A5%E5%85%A5%E5%BC%80%E5%8F%91.md)、
  [有限期属性订阅 API](https://github.com/MiEcosystem/NewXmPluginSDK/blob/master/pluglib/src/main/java/com/xiaomi/smarthome/device/api/XmPluginHostApi.java)、
  [Xiaomi Home cloud/local subscription model](https://github.com/XiaoMi/ha_xiaomi_home)
- OASIS：[MQTT 3.1.1 Keep Alive](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/errata01/os/mqtt-v3.1.1-errata01-os-complete.pdf)
- AWS IoT：[Device Shadow document/version](https://docs.aws.amazon.com/iot/latest/developerguide/device-shadow-document.html)、
  [MQTT-based file delivery](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt-based-file-delivery-in-devices.html)
- Seeed Wio Terminal：[Network overview / RTL8720 eRPC firmware prerequisite](https://wiki.seeedstudio.com/Wio-Terminal-Network-Overview/)、
  [rpcWiFi releases](https://github.com/Seeed-Studio/Seeed_Arduino_rpcWiFi/releases)、
  [RTL8720 firmware releases](https://github.com/Seeed-Studio/seeed-ambd-firmware/releases)

---

## 15. 最终判定

旧 keepalive 方案的正确部分不是“30 秒内允许所有发送”，而是：**持续高成本数据必须由一个仍有效的消费者需求驱动。**
V2 应把这个原则落实为有 target、field、rate、lease 和 ACL 的 TelemetryLease；把单次响应绑定同步 RequestScope；
把 MQTT Keep Alive 留在 transport；把普通 State/Event、History、DeviceRoute 和 CloudService 业务交给各自的
revision、权限、可靠性和发送预算。

这样既能保护初学者不会在 `loop()` 中压垮设备/云端，也不会为了省消息而让无人打开 App 时的告警、自动化、
设备间通信和服务器业务失效。
