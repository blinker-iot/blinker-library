# BBP/2 Device Wire Specification

> 版本：freeze candidate 0.8，2026-08-26  
> 已冻结范围：Frame、HELLO、Property/Action/Event Manifest、Manifest 分页、ManifestAccept、ID gate、State snapshot 分页、4 B BLE 分片和三端 exact golden。  
> 本批新增冻结候选：账户/设备/服务统一 Route/Delivery body；完成跨语言 exact golden 与 Broker App lane 前不得标记 production frozen。  
> 尚未冻结：History。  
> 规范词：MUST/必须、MUST NOT/不得、SHOULD/应、MAY/可按 RFC 2119 理解。
> 2026-08-15 修正：HELLO key 5 的 productId 已在冻结前撤回；key 5 保留且 v2 不复用。

本文是设备、App、服务器实现 BBP/2 的线协议基准。其他设计文档若与本文的“当前已冻结”
条目冲突，以本文为准。尚未通过 exact golden 的 candidate 以及 reserved 条目不得被当作可互操作能力发布。

## 1. 分层与资源所有权

~~~text
BBP/2 Frame
  └─ restricted CBOR message body
       ├─ Field Manifest：字段能力、Field ID、ID gate
       └─ State/Command/Event：运行期数据

PageLayout：App/服务器资产，不属于设备线协议
~~~

Field Manifest 只包含 `FieldSpec`，其 fingerprint 决定 Field ID 映射。App 的 PageSchema、
widget registry、binding、位置、样式和用户拖拽结果始终不进入设备协议。BBP/2 v2 不提供
WidgetCatalog；默认页面建议由 App/服务器根据 Manifest 和产品模板生成。

完整 field key 是不透明稳定标识。任何接收端都不得拆分 `.` 推导 widget/slot。

## 2. BBP/2 Frame

所有多字节整数使用 big-endian。当前发送端固定 10 B header：

| Offset | 大小 | 字段 | 当前规则 |
|---:|---:|---|---|
| 0 | 2 | magic | `0x42 0x4B`（`BK`） |
| 2 | 1 | version | `0x02` |
| 3 | 1 | messageKind | 见第 4 节 |
| 4 | 1 | flags | 见下表 |
| 5 | 1 | headerLength | 当前发送值 10；接收端允许 `>=10` 并跳过扩展 header |
| 6 | 2 | sequence | request/response 关联；需要 ACK 的请求不得为 0 |
| 8 | 2 | bodyLength | CBOR body 字节数 |

总长度必须精确等于 `headerLength + bodyLength + checksumLength`，不得有尾随字节。

| Flag | 值 | 含义 | 状态 |
|---|---:|---|---|
| AckRequired | `0x01` | 请求显式 ACK | current |
| IsResponse | `0x02` | 响应 | current |
| IdMode | `0x04` | 字段 map 使用 uint16 Field ID | current |
| HasCrc32c | `0x08` | frame 尾部 CRC32C | reserved；当前必须拒绝 |
| `0x10..0x80` |  | 未知 flag | reserved；当前必须拒绝 |

传输层不得修改 Frame。MQTT/WS 传完整 frame；BLE 只在 frame 外分片并重组。

当前 MQTT profile 冻结为：一个 binary PUBLISH payload 恰好一个完整 Frame，上下行均 QoS 0，
设备下行不得设置 retained。可靠交付由 BBP/2 ACK/sequence/revision 提供，不依赖 MQTT QoS。
设备在建立 session 前必须确认 MQTT engine 的双向 packet capacity 可容纳 topic、协议开销和
协商的最大 Frame。

## 3. Restricted CBOR

- 只使用 definite-length item；indefinite-length 必须拒绝；
- 整数和长度必须使用最短编码；
- text 必须是合法 UTF-8；float 不允许 NaN/Infinity；
- 当前默认上限：text 256 B、bytes 4096 B、container 32 items、depth 6；
- 控制消息 map 使用递增 uint key；重复已知 key 必须拒绝，未知 key 必须完整跳过；
- keyed/ID value map 必须拒绝重复 key/ID，但应用写入顺序不参与 Manifest fingerprint；
- Manifest canonical encoder 固定递增 key、固定字段顺序和 float64 constraints。

本文的 `canonical Manifest` 指 BBP/2 自己定义的确定性 profile。由于约束数字有意固定为
binary64，即使数值可用 float16/float32 无损表达，也不得缩短。因此不得把它描述为
RFC 8949 的最短浮点 preferred serialization；三端必须按本规范和 golden bytes 实现。

## 4. MessageKind 注册表

| 值 | 名称 | 状态 | 方向/说明 |
|---:|---|---|---|
| `0x01` | Hello | current | 双向协商 |
| `0x02` | ManifestRequest | current | App/Server -> Device |
| `0x03` | Manifest | current | Device -> requester，分页 body |
| `0x04` | Authenticate | current | 本地 App -> Device |
| `0x05` | AuthResult | current | Device -> App |
| `0x06` | ManifestAccept | current | App/Server -> Device；打开该 session 的 ID gate |
| `0x07..0x08` |  | reserved | 已删除的未发布 WidgetCatalog 实验；v2 不得复用 |
| `0x09` | Route | candidate current | App/Device/Server -> Broker，显式目标路由 |
| `0x0A` | Delivery | candidate current | Broker -> App/路由发起者，携带可信 source/correlation |
| `0x0B..0x0F` |  | reserved | 控制面扩展 |
| `0x10` | StateRequest | current | revision-consistent 分页请求 |
| `0x11` | Patch | current single-frame | Report/Apply |
| `0x12` | Command | current | 控制意图 |
| `0x13` | Event | current | 瞬时事件 |
| `0x14` | Ack | current | 响应 |
| `0x15` | Error | current | 响应 |
| `0x16` | StatePage | current | Device -> requester，快照分页响应 |
| `0x17` | TelemetryControl | current optional | App/Broker -> Device，租约控制 |
| `0x18` | TelemetryStatus | current optional | Device/Broker -> App，租约结果 |
| `0x19` | TelemetryData | current optional | Device/Broker -> App，实时采样 |
| `0x1A` | PresenceControl | candidate current | 账户 App -> Broker，订阅单个 logical device 在线状态 |
| `0x1B` | Presence | candidate current | Broker -> 账户 App，当前快照或变更通知 |
| `0x1C..0x1F` |  | reserved | 状态扩展 |
| `0x20` | HistoryRequest | reserved experimental | Runtime 未实现 |
| `0x21` | HistoryBatch | reserved experimental | Runtime 未实现 |

`Frame.h` 中存在枚举值不等于 Runtime 已实现；只有本表 current 项才可视为当前能力。

### 4.1 Error wire code

Error body 至少包含 `{0: errorCode, 1: relatedSequence}`。当前 code 为：

| code | 名称 | 处理 |
|---:|---|---|
| 1 | MalformedMessage | 丢弃请求；严重或重复违规可断开 |
| 2 | AuthenticationRequired | 重新完成当前 transport 的授权 |
| 3 | NegotiationRequired | 重新执行 HELLO/Manifest gate |
| 4 | UnsupportedMessage | 不重试该消息 |
| 5 | UnknownEndpoint | 重新获取 Manifest 后再决定是否重试 |
| 6 | CommandRejected | 向调用方返回确定性拒绝 |
| 7 | ResourceExhausted | 减少单帧内容或等待资源释放 |
| 8 | InternalError | 有界退避；不得 tight loop |
| 9 | SequenceConflict | 重新建立相关可靠窗口 |
| 10 | StateConflict | 丢弃分页暂存并从 cursor 0 重取 |
| 11 | ManifestConflict | 清除 ID gate 并重新协商 Manifest |
| 12 | RateLimited | 至少退避 1 s；state 保留 latest，过时 telemetry 不重放 |

Broker 限流时必须先投递 `RateLimited(12)`，再丢弃被拒绝的原始上行；只有同一连接在
10 s 内累计 8 次限流/超大 frame 违规时才断开。当前 Error body 没有 `retryAfter` 字段，
因此三端固定使用 1 s 初始退避；未来增加该字段必须保持 restricted-CBOR 向后兼容。

这里的反馈是发布连接上的顶层 `Error(IsResponse)`，`relatedSequence` 指向被拒绝的上行 sequence。
Broker 尚未处理被拒绝的 `Route`，因此不能把该反馈包装成 `Delivery`。设备和 App 在 HELLO 完成后都必须接受
这种顶层 Error：找到对应 pending publish 时以确定性错误结束该请求；无 pending 的 best-effort 上行则只执行
退避/计数。合法 `RateLimited` 不得被当作“unexpected frame”关闭整个协议 session。

`RateLimited` 是当前发布连接的临时背压反馈，不是路由撤销，也不是 Telemetry 生命周期操作：

- Broker 必须在协议处理、持久化和 fanout 前丢弃**当前被拒绝帧**，但不得因单次超限关闭整条路由或 MQTT session；
- 发送端收到反馈后暂停该 Cloud 连接的非关键业务；ACK、ERROR、TelemetryStatus 和协议控制继续使用保留预算；
- State 在退避期间继续更新本地事实并只保留 latest，恢复后重新收敛；Event 不补发已经被拒绝的瞬时事件；
  Telemetry 消费当前采样机会并延后下一次采样，不缓存或 replay 旧 sample；
- 普通超限不得生成 `TelemetryControl.Close`。Close 只表示对应 consumer 消失、ACL 被撤销、session 关闭或
  lease 生命周期结束；最后一个 consumer 消失时 Broker 必须先停止 fanout，再发送 Close，设备 lease timeout
  是丢失 Close 时的兜底；
- App/设备收到 `RateLimited` 后不得自动重试 Command/Event，以免形成重试风暴。可靠帧收到该 ERROR 后结束
  当前 delivery；上层只能在重新确认业务意图后创建新请求。

当前 1 s 退避是 connection-level 的保守合同，不承诺按 lane 动态调速。Broker 已按 lane 保护控制流；如果未来
需要服务器指定 `retryAfter`、lane 或 scope，必须作为 Error body 的可选扩展统一修改三端 codec，不能用
`TelemetryClose` 冒充拥塞控制。

## 5. HELLO

HELLO body：

| Key | 类型 | 必选 | 含义 |
|---:|---|---:|---|
| 0 | uint | 是 | role：Device=0、App=1、Server=2 |
| 1 | array&lt;uint8&gt; | 是 | 支持版本，非空且不得重复 |
| 2 | uint32 | 是 | feature bitset |
| 3 | uint16 | 是 | maxFrame；必须 `>=10` |
| 4 | uint32 | 是 | maxReassembly；必须 `>=maxFrame` |
| 5 | reserved | 否 | 已撤回的 productId；发送方不得发送，接收方必须拒绝 |
| 6 | uint32 | 否 | Field Manifest revision |
| 7 | bytes(32) | 否 | Field Manifest SHA-256；必须精确 32 B |
| 8 | array&lt;uint16&gt; | 条件 | Authentication 方法 |
| 9 | uint8 | 条件 | reliable receive window，1..16 |
| 12 | uint16 | 否 | Direct BLE鉴权后确认的ATT value上限，20..244 B；包含4 B分片头，不是maxFrame |

Key 6/7 必须成对出现。其 role 语义：Device 应宣告自己当前 Field Manifest；Direct BLE/LAN
单设备 App/Server session 可宣告其已完整验证并缓存的该设备 Manifest。一个账户级 App MQTT
session 会复用给多台设备，connection HELLO 必须省略 key 6/7，并由 Broker 按
`(appSession, logicalDeviceId)` 维护第 5.1 节的 virtual route Manifest gate。单设备 session 的
revision+fingerprint 与设备当前值精确相等时，设备可在 HELLO 后直接设置该 session
`idReady=true`；否则必须走第 7 节流程。

当前 feature bit：

| bit | 名称 | 依赖/含义 |
|---:|---|---|
| 0 | FeatureManifest | Field Manifest 下载与缓存 |
| 1 | FeatureEndpointIds | 允许完成 Manifest gate 后使用 Field ID；依赖 Manifest |
| 2 | FeatureFragmentation | BBP 层目标能力；当前 BLE 分片位于 Frame 外 |
| 3 | FeatureCrc32c | reserved；当前不得协商 |
| 4 | FeatureHistory | reserved experimental |
| 5 | FeatureAuthentication | HELLO key 8 必须存在 |
| 6 | FeatureReliableDelivery | HELLO key 9 必须存在 |
| 7 | FeatureStateRevision | StateRequest/StatePage 和版本化 PATCH |
| 8 |  | reserved；已删除的未发布 WidgetCatalog 实验，v2 不得协商 |
| 9 | FeatureControllerControl | 受控本地 controller 管理；依赖独立授权合同 |
| 10 | FeatureRouting | Route/Delivery；账户级 App 与显式 Device/Service 路由 |
| 11 | FeatureTelemetry | 实时采样租约；依赖 Manifest 和 Endpoint IDs |
| 12 | FeaturePresence | 账户 App/Broker 在线状态；不由设备协商，不依赖 Manifest |

HELLO key 10/11 当前同样保留；发送端不得生成，接收端按未知扩展 key 跳过但不得据此启用能力。

2026-10-10 N3已实现的可选扩展：key12仅在DirectSecure鉴权后的BLE Hello请求/响应中确认；启动、Noise、Method2及Hello响应发送均保留20 B。其他bearer不生成该字段，旧peer忽略或不回复该字段时保持20 B。发送端收到认证确认后仅从新完整记录启用最小共同上限，接收端以START实际payload冻结该记录stride；MTU提升自身不授权升级。失败回退不得重新加密或在未知结果下重执行业务，细则和软件/本机限定真机状态见[易用性后续计划§7](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#7-n3-esp32-ble-包长协商)。本扩展不改变Frame或4 B分片版本，不复用key10/11。

发送给某 session 的 logical frame 必须满足：

~~~text
frameSize <= min(local maxFrame, local TX capacity, remote maxFrame)
~~~

HELLO 已强制 `remote maxReassembly >= remote maxFrame`，因此设备会话只需保存归一化的
`remote maxFrame`，无需每个 session 再复制 uint32 reassembly 值。该归一化实现等价地
满足两项远端能力，同时减少小 MCU SRAM。

### 5.1 账户级 MQTT 与显式 Route/Delivery（candidate current）

App 使用用户登录态换取的短期账户 MQTT credential，不使用设备 SessionCredential。App、Device
和 Server 是独立 BBP/2 session；Broker 必须分别维护 sequence、replay、limits、Manifest gate
和 correlation，不得把一个 session 的完整 frame header 原样作为另一个 session 的 header。

普通 Device `Patch Report`、`Event` 和 snapshot 进入 cloud ingest 后，由 Broker/服务端完成
latest state、history、App fanout 和 automation；它们不得为了每个消费者重复发送 Route。只有
App→Device、显式 Device→Device、Device/App→registered Service 和受控 group 才使用 Route。

`RoutePeerKind`：LogicalDevice=0、CloudService=1、DeviceGroup=2、Account=3、Platform=4。
Route target 只允许 0..2；Account/Platform 只允许 Broker 在 Delivery source 中使用。peer ID
统一为 16 B binary UUID；不得发送 MAC、MQTT clientId、Topic、显示名称或 Legacy 特殊字符串。

控制面/API 当前可显示 `device_<uuid>`，但 Route/Delivery 的 LogicalDevice ID 只编码 UUID
的 16 B：不含 `device_`、连字符或 UTF-8 文本。该映射必须可逆；接收端不得 hash/截断任意设备名，
不符合 `device_<uuid>` 的历史标识不能进入本路由合同。

Platform Delivery source 的 candidate 保留 ID 为 `00000000-0000-0000-0000-000000000001`；
它只表示 Broker/平台生成的错误或通知，不得作为 Route target，也不得分配给 logical device。

Route (`0x09`) body：

~~~text
{
  0: targetKind:uint,
  1: targetId:bytes(16),
  2: requestId:bytes(16),
  3: innerKind:uint8,
  4: innerFlags:uint8,
  5: innerBody:encoded canonical CBOR value
}
~~~

- 六项全部必选且 key 必须递增；requestId 是跨重连稳定的幂等 ID，不等于 frame sequence；
- inner kind 只允许 ManifestRequest、Manifest、ManifestAccept、StateRequest、Patch、Command、
  Event、Ack、Error、StatePage、TelemetryControl/Status/Data、PresenceControl/Presence；不得嵌套
  Hello、Authenticate、Route/Delivery 或 ControllerControl；
- inner flags 只允许 AckRequired/IdMode；不得设置 IsResponse、HasCrc32c 或未知位；
- outer Route 必须只设置 AckRequired，sequence 非 0；
- innerBody 直接嵌套一个 canonical CBOR item，不得包装为 JSON、text 或 base64。

Delivery (`0x0A`) body 与 Route 使用相同 key，其中 key 0/1 表示 Broker 认证得到的 source；
key 2 requestId 对非请求触发的 Patch/Event 可省略，其余 key 必选。Delivery inner flags 可使用
AckRequired/IsResponse/IdMode，但仍不得使用 CRC 或未知位。客户端不得自行声明可信 source。

当前 Cloud 主动 Patch fanout 的外层 Delivery 固定 `flags=0, sequence=0`，不含 requestId；inner Patch
固定 `IdMode`、mode=Report，并携带权威 state revision。它是 best-effort 状态通知，不进入 command
ledger。App 只在已持有同 fingerprint、前一 revision 完整状态时应用；断线或 revision gap 必须重新
StateRequest。

当前 Device→Cloud Event 只允许 `flags=0` 或 `IdMode` 且 frame sequence 非 0；`AckRequired` Event
不属于当前 Cloud ingest 合同。body 是非空 endpoint value map，最多 32 项；Broker 必须按当前
Manifest 验证每项均为 `kind=Event, access=0x08`，并校验 ValueType 和 constraints。Event 不带 state
revision、不进入 latest-state、不得合并，也不触发设备 ACK。

Cloud 主动 Event fanout 同样使用无 requestId 的
`Delivery(LogicalDevice, Event, IdMode)`，外层固定 `flags=0, sequence=0`。App 只需完成该 target 的
ManifestAccept；Event 不依赖 StatePage baseline。Broker 每次 fanout 都重新确认 owner/share read 权限，
并与 Patch 共用每 App session 的 1 个发送中项和最多 32 个待发项。Event 不合并；frame 超限、发布失败
或第 33 个待发项会断开慢 session，使客户端明确知道实时流已中断。

本合同是实时 best-effort 事件，不定义跨重连 durable eventId、补发、history 或 automation exactly-once。
断开前后的 Event 可能丢失，StateRequest 也不能恢复 Event。审计、告警确认、历史和自动化必须进入后续
独立的持久 event ingest 合同，不能把当前实时 fanout 当作可靠业务事件总线。

账户级 App 的 `IdMode` gate 按 target 隔离：routed ManifestRequest/Delivery Manifest pages/
routed ManifestAccept 完成后，Broker 才把对应 `(appSession,targetId)` 标记为当前 fingerprint 的
`idReady`。一个设备 Manifest 变化不得清除同一 App connection 上其他设备的 gate。

Broker 校验 ACL/Manifest/requestId 后，为目标 Device session 分配新的 sequence。收到目标
ACK/Error、确定性拒绝或超时后，Broker 用原 requestId 生成 Delivery；相同 source+requestId
重放必须得到同一结果，不得再次执行 mutation。Device→Device 还必须有 DeviceRouteGrant、
server automation policy 或 group policy；同 tenant/owner 本身不构成授权。

### 5.2 账户 App 在线状态 Presence（candidate current）

Presence 是 Broker 根据已鉴权 Device MQTT 会话生成的账户元数据，不是设备属性，也不属于 Field
Manifest。设备 Runtime 不发送、不保存、也不协商 `FeaturePresence`。它只回答“该 logical device 当前
是否存在有效 cloud session”，不得把 App 页面是否已同步、Direct BLE 是否在附近或局域网是否可达混入
同一个布尔值。

账户 App 在 HELLO 协商 `FeaturePresence` 后，对库存中的每个 logical device 发送可靠 Route：

~~~text
Route(
  target = LogicalDevice,
  innerKind = PresenceControl,
  innerFlags = 0,
  innerBody = {0: 0}        // Subscribe
)
~~~

Broker 必须先按当前 owner/share read ACL 鉴权，然后原子登记该 App session 的订阅并返回带相同
`requestId` 的 `Delivery(Presence, IsResponse)`。该流程不建立 Manifest gate，也不得触发 Manifest/State
下载。未协商 feature、目标不存在或 ACL 不允许时必须确定性拒绝，不能静默登记订阅。

Presence body 固定为：

~~~text
{
  0: cloudReachable: bool,
  1: cloudLastSeenAt: uint | null
}
~~~

- `cloudLastSeenAt` 使用 Unix epoch 毫秒和 canonical uint；大于 uint32 时必须使用最短 uint64；
- `true + null` 非法；`false + null` 只表示 Broker 从未观察到该设备的 cloud transition；
- 每次实际 online/offline transition 的时间戳必须严格递增；重复的相同状态不得生成新 transition；
- 主动通知使用无 `requestId` 的 `Delivery(LogicalDevice, Presence, flags=0)`；它不使用 `IdMode`；
- Broker 每次 fanout 都必须重新核验 ACL/version。撤销分享后须立即移除订阅、待发 Presence 和相关 replay；
- 每个 App session 的订阅数必须有界；慢消费者队列按 logical device 只保留最新 Presence；
- App 必须忽略更旧时间戳并拒绝同一时间戳上的冲突状态；账户 MQTT 断开时当前可达性转为 unknown，
  但保留最后时间戳，重连后重新订阅并用原子快照恢复；
- HTTP inventory 返回的同字段只用于首屏 seed；连接建立后以 BBP/2 Presence 为实时来源，不得再创建
  JSON/WebSocket/polling 第二条状态通道。

当前 exact golden：

| 内容 | canonical CBOR hex |
|---|---|
| Subscribe `{0:0}` | `a10000` |
| offline、从未见过 | `a200f401f6` |
| online、`1700000000000` ms | `a200f5011b0000018bcfe56800` |

## 6. Canonical Field Manifest

逻辑 Manifest（不是分页 body）：

~~~text
{
  0: manifestRevision,
  1: fields[]
}
~~~

Field map：

| Key | 类型 | 必选 | 含义 |
|---:|---|---:|---|
| 0 | text | 是 | 完整 fieldKey |
| 1 | uint | 是 | capability kind |
| 2 | uint | 是 | ValueType |
| 3 | uint8 | 是 | access bitmask |
| 4 | uint16 | 是 | Field ID = fields 下标 + 1；0 非法 |
| 5 | float64 | 否 | minimum |
| 6 | float64 | 否 | maximum |
| 7 | float64 | 否 | step |
| 8 | uint32 | 否 | maxLength |
| 9 | text | 否 | unit |
| 10 | array&lt;text&gt; | 否 | enum text |

`kind` 当前只允许：Property=0、Action=1、Event=2。`ValueType` 为 Boolean=0、
SignedInteger=1、UnsignedInteger=2、Float32=3、Float64=4、Text=5、Bytes=6、Object=7、
Array=8、Null=9。access 位为 Read=`0x01`、Write=`0x02`、Notify=`0x04`、Event=`0x08`、
Command=`0x10`，并满足：

- Property 只允许 Read/Write/Notify，Notify 必须同时有 Read，且类型不得为 Null；
- Action 必须且只能是 Command；
- Event 必须且只能是 Event；
- Action/Event 不进入状态仓，但仍占 Manifest ordinal 和稳定 Field ID；
- 数值、长度、unit、enum 约束属于设备能力，不是 widget presentation。

~~~text
fingerprint = SHA-256(canonicalLogicalManifestBytes)
~~~

fingerprint 固定 32 B。hash 必须由 canonical encoder 流式更新，不得要求完整 Manifest
scratch。分页大小、BLE MTU、MQTT packet size 均不得改变 fingerprint。

## 7. Manifest 分页与 ID gate

### 7.1 ManifestRequest (`0x02`)

Body：`{0: cursor:uint16}`。cursor 是 fields 的零基下标；首次为 0。请求 flags 必须为 0，
sequence 必须非 0。请求必须来自 HELLO 完成、已授权并协商 `FeatureManifest` 的 session。

### 7.2 Manifest (`0x03`)

Body：

~~~text
{
  0: revision,
  1: fingerprint32,
  2: cursor,
  3: nextCursor,
  4: totalFields,
  5: fields[]
}
~~~

- 响应设置 `IsResponse` 并复用请求 sequence；
- `0 <= cursor <= nextCursor <= totalFields`；
- `fields.length == nextCursor - cursor`；
- 非终页必须至少包含一个完整 FieldSpec，字段不得跨页拆分；
- `nextCursor == totalFields` 表示最后一页；无额外 `more/pageCount` 冗余字段；
- 每页必须符合当前 session 的发送 frame 上限；若一个 FieldSpec 都放不下，返回
  ResourceExhausted，不发送部分 field；
- 接收端必须拒绝缺口、重叠冲突、revision/fingerprint/totalFields 变化；
- 接收端按 totalFields 重建第 6 节逻辑 Manifest，并验证完整 SHA-256。

分页是无状态 cursor 请求；设备 Runtime 不为每个 peer 保存下载 buffer/cursor。

### 7.3 ManifestAccept (`0x06`)

Body：`{0: revision:uint32, 1: fingerprint:bytes(32)}`。请求必须只设置 `AckRequired`，
sequence 非 0。设备精确匹配当前 revision+32 B fingerprint 后：

1. 设置当前 session `idReady=true`；
2. 返回 Ack；
3. 才允许该 session 收发 `FlagIdMode` body。

不匹配时清除 `idReady` 并返回 `ManifestConflict(11)`。未 Accept 前 ID 消息返回
NegotiationRequired。disconnect、重新 HELLO 且缓存不匹配、或 Manifest 变化都必须清除
该 session 的 `idReady`。

设备只保存“该 session 已验证当前 Runtime Manifest”的布尔状态，不复制 32 B digest；
Runtime 启动后 Manifest 不可变，完整 digest 只保存一份。App/服务器可为缓存/审计保存
完整 digest。

## 8. State snapshot 分页

### 8.1 StateRequest (`0x10`)

首次请求 body 必须是：

~~~text
{0: 0}
~~~

续页请求 body 必须是：

~~~text
{0: cursor:uint16, 1: observedRevision:uint32}
~~~

- `cursor` 是 Field Manifest 的零基 field ordinal；首次必须为 0；
- `cursor == 0` 时必须省略 `observedRevision`，`cursor > 0` 时必须携带；
- 请求必须来自 HELLO 完成、已授权且协商 `FeatureStateRevision` 的 session；
- sequence 必须非 0；flags 只允许可选 `IdMode`；
- 使用 `IdMode` 前，该 session 必须已经通过第 7 节的 ID gate；
- 同一次 snapshot 的所有续页必须保持相同 Keyed/ID mode；
- 重发刚才的续页 cursor 必须得到同 revision、同 cursor 范围的等价页面，用于恢复丢失响应。

### 8.2 StatePage (`0x16`)

Body：

~~~text
{
  0: revision:uint32,
  1: cursor:uint16,
  2: nextCursor:uint16,
  3: totalFields:uint16,
  4: values:map
}
~~~

- 响应必须设置 `IsResponse`，复用请求 sequence，并与请求保持相同 `IdMode`；
- `totalFields` 必须等于当前 Field Manifest field 数；
- `0 <= cursor <= nextCursor <= totalFields`；非终页必须满足 `nextCursor > cursor`；
- `values` 只允许包含 `[cursor, nextCursor)` ordinal 范围内的 field；Keyed 模式使用完整
  field key，ID 模式使用 `ordinal + 1`；
- 尚未初始化的 field 仍由 cursor 范围覆盖，但必须从 `values` 省略，因此
   `values` map 的条目数 `<= nextCursor - cursor`；
- 页面只能在完整 field value 边界切分，不得拆分一个 CBOR value；每页 logical frame
  必须不超过该 session 的归一化 remote frame 上限；
- `maxContainerItems` 是每页 `values` map 的条目上限；达到该上限时必须在前一个完整
  field 边界结束本页，不得把“整个 snapshot 的字段数较多”误判为 `CapacityExceeded`；
- 若连一个 field 都无法与 envelope 一起装入上限，设备必须失败，不得发送部分 value。

设备在首次请求时固定当前 `stateRevision`。续页只接受同 session、预期或刚发送过的 cursor、
相同 mode 以及相同 `observedRevision`。分页期间任何设备状态变化都必须撤销未完成 snapshot；
旧续页返回 `StateConflict`，ERROR key 3 携带当前 revision。请求方随后丢弃全部暂存页并以
`cursor=0` 重启。

### 8.3 接收端原子提交

App/服务器必须把页面暂存在独立 snapshot transaction 中，并检查 revision、mode、
totalFields、连续 cursor 和 Manifest 范围。只有 `nextCursor == totalFields` 后，才可把完整
逻辑 snapshot 一次提交到 UI/latest-state cache；不得把单页直接混入权威状态。一个 snapshot
transaction 不得跨 session、Manifest fingerprint 或 state revision 复用。

设备只有在最后一页成功发送后，才把该 session 标记为“已观察当前 revision”并允许可靠
`PATCH Apply`。设备 revision 推进会撤销所有 peer 的观察资格；Apply 来源 peer 可通过成功
ACK 中的新 revision 保留连续写资格，其他 peer 必须重新完成 snapshot。

## 9. BLE characteristic packet 映射（已冻结）

BLE 不重新编码 CBOR，也不修改 BBP/2 Frame。一个 characteristic packet 是 4 B
session-local 分片头加一段连续 BBP/2 frame bytes：

| Offset | 大小 | 字段 | 约束 |
|---:|---:|---|---|
| 0 | 1 | Magic | 固定 `0xB2` |
| 1 | 1 | Version + Flags | 高 4 bit 为版本 `1`；低 bit0=`START`、bit1=`END`，bit2..3 必须为 0 |
| 2 | 1 | Frame ID | 当前 BLE session、当前方向内的非零循环 ID |
| 3 | 1 | Fragment Index | 当前 frame 内从 0 严格递增，不得回绕 |
| 4 | N | Data | 非空、连续的 BBP/2 frame bytes |

因此无 flag、START、END、START|END 的第二字节分别为 `0x10/0x11/0x12/0x13`。
旧 9 B `version/flags/frameId16/totalLength/offset` 原型不是兼容格式；设备、App 和 BLE gateway
不得同时嗅探或发送两种未发布格式。

### 9.1 长度和 offset 推导

1. `START` 当且仅当 `index=0`；首片必须至少包含完整 10 B BBP/2 base header。
2. 接收端从 BBP header 的 `headerLength/bodyLength/flags` 得到完整 logical frame 长度，并在
   写入 reassembly buffer 前检查本地 profile 上限。
3. 每个非单片 frame 的首片 Data 长度固定本 frame 的 `fragmentPayloadCapacity=P`。
   后续非终片 Data 长度必须等于 P；终片 Data 长度必须为 `1..P`。
4. index `i` 的 logical offset 为 `i * P`。`END` 当且仅当该片正好覆盖剩余 logical bytes；
   完成后仍必须执行完整 BBP/2 frame 校验。
5. index 范围为 `0..255`，一个 frame 最多 256 片且不得在 frame 内回绕；必须满足
   `ceil(frameLength/P) <= 256`。ATT payload 20 时 P=16，因此 logical frame 精确上限为
   `4096 B`。BBP/2 BLE profile 不得宣告更大的 maxFrame。

### 9.2 Session、Frame ID 与 MTU

- 每个 BLE session、每个方向同时最多一个正在传输/重组的 frame；两个方向的 Frame ID
  空间彼此独立。
- Frame ID 顺序为 `1..255,1..`，0 非法；正常新 frame、BBP 层重发和 MTU restart 都必须
  分配新的 ID。回绕只能发生在 frame 边界。
- START 之后 MTU 增大时，当前 frame 必须保持原 P，可以发送小于新 ATT 上限的 packet。
- MTU 减小到无法继续原 P，且 Adapter 能确认当前长度拒绝未消费 packet 时，发送端放弃当前
  wire frame，使用新 Frame ID、index 0 和新 P 重分片同一 logical record；DirectSecure 必须保留
  原密文及 nonce，不重新 seal。未知原生发送结果必须关闭会话，不借包长回退重执行业务；
  不得在一个 frame 内混用两个 stride。
- 新的合法 START/新 Frame ID 可替换当前未完成 reassembly；相同 Frame ID 的重复 START、
  已完成 frame 的立即重复 START、重复 index、缺口、乱序、错误非终片长度、越界、未知
  version/flag 均必须丢弃并终止当前 reassembly。只有后续合法 START 可以恢复。
- reassembly timeout 和 disconnect 必须清空未完成 frame；重连产生新 session，不继承
  Frame ID、认证或 reassembly 状态。

### 9.3 背压

底层 `sendPacket` 必须原子消费一个完整 characteristic packet。返回 `WouldBlock` 时，上层
保留当前 frame/index/offset，后续 poll 重试同一片；不得忙等或插入协议层 delay。广播入队
必须全成功或全失败。Adapter 必须报告当前真实 ATT payload 上限；实现可以协商 244 B，
但 ATT payload 20 仍是正确性基线。

## 10. 跨语言一致性基线

唯一 UTF-8 fixture 由生产 C++、App-compatible TypeScript 和独立服务器
Node 实现生成相同的 29 个 exact outputs，同时一致拒绝 6 个异常输入。覆盖 Manifest
完整 bytes/SHA-256/三页分页、StateRequest、Keyed/ID StatePage、合法 StatePage 响应
Frame、ATT20/244 BLE packets、非最短整数、重复 key、未知 flag、截断和 BLE START/index
冲突。fixture 与运行方法位于 `tests/golden/`。

golden 是线格式变更门禁，不是 Python benchmark 的输出缓存。任何 `expected.*` 变化都必须
视为显式 wire revision 评审；App/服务器生产 codec 后还必须把同一 fixture 接入各自 CI。
Route/Delivery candidate 必须另增 C++、App TypeScript 与 Broker TypeScript exact bytes、round-trip
和拒绝向量；在这些向量进入同一门禁前，0.7 不得替代已冻结的 0.6 发布基线。

Property/Action/Event Manifest 与设备 public API 已完成 reference 实现。MQTT 默认 adapter
决策与 capacity/超时加固已完成；静态产品组合、managed
network gate、local-first poll 和 ESP32 1.5 MiB dual-OTA partition 余量门禁已完成。当前仍缺
实机 WiFi+BLE 资源、阻塞与 24 h 门禁；History 在完成独立规范和新 golden 前继续保持
reserved experimental，不得作为已发布能力使用。
