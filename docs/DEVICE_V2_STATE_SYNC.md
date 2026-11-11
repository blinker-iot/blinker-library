# Device v2 状态同步、PATCH 与 EVENT 契约

> 状态：设备端协议、Runtime 和主机向量已实现；2026-08-19 Broker 已实现 ManifestAccept 后的分页
> snapshot、完整提交、latest-state 持久化及 Report Patch 合并，Gateway 已提供 owner 只读 State API。
> App PageSchema reducer、控制下行和三端 E2E 仍未完成。此处的 `stateRevision`
> 是 BBP/2 设备事实状态版本，不是旧 TLV 的 alias `stateVersion`，也不是
> App 页面 `controlVersion`。

## 1. 三类消息的边界

| 消息 | 含义 | 是否持久状态 | 设备状态版本 |
|---|---|---:|---:|
| `COMMAND` | 控制意图，例如“打开灯” | 不直接代表最终事实 | 不改变 |
| `PATCH Report` | 设备报告当前事实或事实增量 | 是 | 携带结果版本 |
| `PATCH Apply` | App/云端按期望版本写设备状态 | 是 | 成功后递增一次 |
| `EVENT` | 瞬时事件，例如按键、告警边沿 | 否 | 不改变 |

`COMMAND` 与 `Apply` 不应混用：动作、场景、脉冲、校准等使用 COMMAND；
可读写属性的目标状态同步使用 Apply。设备完成 COMMAND 后，用 Report
发布最终事实状态。

## 2. wire schema

双方通过 `FeatureStateRevision = 1 << 7` 协商。版本化 PATCH body 为：

~~~text
{
  0: mode:uint,       // 0 Report, 1 Apply
  1: revision:uint32,
  2: values:map       // endpointKey:value 或 FlagIdMode 下的 endpointId:value
}
~~~

- Report 的 `revision` 是这些值所属的设备状态版本。
- Apply 的 `revision` 是发送方期望设备当前仍处于的版本。
- `values` 是 stable fieldKey:value，例如 `power`、`temperature`。key 是完整不透明标识；
  App 通过 PageSchema 显式绑定 Property/Action/Event，不拆分 key，不在设备端保留组件对象 DOM。
- EVENT body 仍是直接的 endpoint map，没有状态外层和 revision。
- Apply 成功的 ACK 可增加 `{1: stateRevision}`。
- 状态冲突使用 `ERROR StateConflict(10)`，ERROR 可增加
  `{3: stateRevision}` 返回设备当前版本。

State snapshot 不再复用 PATCH Report，而使用独立分页消息：

~~~text
StateRequest initial       = {0: 0}
StateRequest continuation  = {0: cursor, 1: observedRevision}
StatePage                  = {
  0: revision,
  1: cursor,
  2: nextCursor,
  3: totalFields,
  4: values
}
~~~

cursor 是 Field Manifest 的零基 ordinal。`values` 只包含当前 ordinal 区间内已经
初始化的 field；未初始化 field 被 cursor 区间覆盖但不占 map entry。StatePage 使用
`MessageKind 0x16` 和 `IsResponse`，不得伪装成可立即应用的 PATCH。

编码使用 definite-length canonical CBOR。低层 Runtime 不构建 DOM，也不持久化
整段 PATCH body；发送时写入调用者/Runtime buffer，接收时以 `ByteView` 引用
transport 接收 buffer。canonical `Client` 会把通过校验的“单个 field 完整 CBOR
编码值”复制到调用方提供的共享 packed state arena；`EndpointStateSlot` 只保留 6 B
offset/length/type/initialized 元数据，用于 `current()` 和后续 StateRequest。也就是说：
frame/body 是临时的，权威状态值是有界保留的，页面布局仍不在设备状态仓中。

## 3. 会话流程

~~~text
App/Server                         Device
    | -------- HELLO -------------> |
    | <------- HELLO ---------------|  协商 StateRevision/ReliableDelivery
    | -------- StateRequest(c=0) -->|
    | <------- StatePage(r=7,0..4)--|  暂存，不更新权威 UI/cache
    | -------- StateRequest(4,r=7)->|
    | <------- StatePage(r=7,4..8)--|  final；原子提交完整快照
    | -------- PATCH Apply -------->|
    |          expected r=7         |  先校验全部 endpoint/type/access
    | <------- ACK(new r=8) --------|  handler 成功且只执行一次
    | <------- PATCH Report(r=9) ---|  设备本地状态再次变化
~~~

Apply 必须满足：

- 当前会话已经成功收齐同一 revision、连续 cursor、相同 Keyed/ID mode 的全部 StatePage；
- 已协商 `FeatureStateRevision` 和 `FeatureReliableDelivery`；
- `FlagAckRequired=1`、sequence 非零；
- mode 必须为 Apply；
- revision 必须等于设备当前 revision；
- keyed/ID values 必须通过 Manifest 类型、约束和 `AccessWrite` 校验。

重连后 App/服务器必须重新 StateRequest，不能沿用上一个会话缓存的
revision。这样即使设备重启后 `initialStateRevision` 重新从零开始，旧队列
中的 Apply 也不能在未重新观察快照时误执行。需要跨重启保持单调版本的产品
可从持久存储恢复 `DeviceRuntimeConfig::initialStateRevision`。

分页期间 revision 变化时，设备清除所有 peer 的未完成 snapshot/观察资格；旧续页返回
带当前 revision 的 StateConflict。App/服务器必须丢弃整个暂存事务并从 cursor 0 重启，
不得跨 revision 拼页。重复最后一个续页 cursor 可用于恢复丢失响应，但跳页、倒退到更早
cursor 或切换 Keyed/ID mode 都是冲突。只有最后一页成功发送后，该 session 才取得 Apply
资格。

Cloud App 不直接占用设备连接重复拉取同一份快照。Broker 在设备完成上述同步后保存最新完整
authoritative state；App 先通过 `Route(ManifestRequest/ManifestAccept)` 建立 per-target gate，再用
`Route(StateRequest, IdMode)` 从 Broker 分页读取该快照。Broker 返回
`Delivery(StatePage, IsResponse|IdMode)`，沿用完全相同的 cursor/revision 合同。该只读路径不进入设备
command ledger；同一 App session 只做 requestId exact replay，MQTT 重连后重新协商并读取最新快照。

设备后续只需向 Cloud 上报一次 Report Patch。Broker 在 Patch 通过 Manifest/revision 校验并写入最新状态后，
向已经收齐同 fingerprint StatePage 的在线 App session 发送无 requestId 的
`Delivery(LogicalDevice, Patch, IdMode)`；不得把设备原 frame sequence/header 透传给 App。App 只接受前一
revision 的连续 Patch。断线、revision 跳号、frame 超限或 Broker 慢消费者保护触发时，丢弃该设备的增量
假设并重新 StateRequest；不得跨 gap 猜测状态。Event fanout 使用独立合同，不复用 Patch revision 队列。

设备 Event body 是非空 endpoint map；每项必须对应当前 Manifest 的 `kind=Event/access=0x08`，不带
revision、不写 latest-state、不可合并。当前 Device→Cloud 只发送 best-effort Event（`flags=0`，或
已建立 ID gate 时使用 `IdMode`）；Broker 转为无 requestId 的
`Delivery(LogicalDevice, Event, IdMode)`。App 完成该 target 的 ManifestAccept 即可接收，不要求先收齐
StatePage。Patch/Event 共用每 App session 的 1 个发送中项和最多 32 个待发项；Event 队列溢出或发送
失败会断开 session，但不会跨重连补发。它不能用于需要审计、历史或可靠告警确认的业务。

## 4. 可靠重放与冲突

Controller→Device 的 COMMAND、可靠 PATCH 和可靠 EVENT 共用 per-session 请求重放窗口。当前
Device→Cloud Event 不设置 AckRequired，因此不进入该窗口。请求摘要为
SHA-256 前 128 bit：

~~~text
"BLINKER-REQUEST-V1"
|| kind
|| (AckRequired | IdMode)
|| bodyLength(network order)
|| exactBodyBytes
~~~

相同 session/sequence/摘要只重放之前的 ACK 或 ERROR，不再调用 handler；相同
sequence 但摘要不同返回 `SequenceConflict(9)`。Apply 的成功 ACK 和冲突
ERROR 都会缓存对应的 stateRevision。

Runtime 在调用业务 handler 前先完整解析并校验整个 values map，避免后部字段
格式错误造成前部 handler 已执行。canonical `Client` 使用
`StateApplyTransactionHandler` 把整段 values 一次交给 `EndpointStateStore`：先预检
全部 endpoint、类型、权限、约束和槽容量，再执行不可失败的复制提交。因此数据状态
是原子的。低层 Runtime 仍保留旧的逐 endpoint handler；使用它的高级组合必须自行
承担业务副作用无法自动回滚的问题。

## 5. 设备 API 与 BLE/WiFi 同时发布

Public component 的普通单次状态变化使用 `report()`；多 endpoint 原子事务目前是 Advanced
`Client + EndpointHandle` API：

~~~cpp
temperature.report(23.5f);

// Advanced：powerState/temperatureValue 是已绑定的 EndpointHandle。
blinker::StateUpdate update(client);
update.begin(2);
update.set(powerState, true);
update.set(temperatureValue, 23.5f);
update.commit();
~~~

它先提交同一状态仓，再推进 revision 并以同一 revision fan-out 到合格 peer；发送失败不回滚设备
事实。以下 `DeviceRuntime` 方法属于不使用 canonical Client 时的低层接口：

~~~cpp
runtime.sendPatch(values, target);             // best effort Report
runtime.sendReliablePatch(values, target, seq); // 定向可靠 Report
~~~

它们在形成有效 Report 时自动推进一次 revision。一个逻辑变化需要发到多个
BLE/MQTT peer 时，版本只能推进一次：

~~~cpp
uint32_t revision = 0;
runtime.advanceStateRevision(revision);
runtime.sendCurrentPatch(values, bleTarget);
runtime.sendCurrentPatch(values, mqttTarget);

// 可靠定向发送也复用同一个 revision：
runtime.sendCurrentReliablePatch(values, bleTarget, bleSequence);
runtime.sendCurrentReliablePatch(values, mqttTarget, mqttSequence);
~~~

如果硬件状态会在离线时也发生变化（物理按键、传感器采样、本地自动化），应在
状态真正提交时先调用 `advanceStateRevision()`，网络可用时再用 `sendCurrent*`
报告；不能让 transport 是否在线决定事实状态是否推进。

`broadcastPatch()` 现在只是兼容命名：Runtime 内部会枚举已经完成 HELLO、授权并
协商 StateRevision 的 peer 后定向发送，不再把状态裸发给 TransportHub 的全部
session。没有合格 peer 是成功 no-op。可靠发送不能广播，因为每个 peer 有独立
ACK、窗口、重试和断连状态；应使用上面的
`advance + sendCurrentReliablePatch`。

ID 模式对应 `sendPatchById`、`sendCurrentPatchById`、
`sendReliablePatchById` 和 `sendCurrentReliablePatchById`，只允许定向到已经
协商 Manifest/EndpointIds 的 peer。

低层 Runtime 入站处理分别注册：

~~~cpp
runtime.setStateEncoder(encodeSnapshot, context);
runtime.setStatePageEncoder(encodeSnapshotPage, context);
runtime.setStateApplyHandler(applyDesiredState, context);
runtime.setEventHandler(handleEvent, context);
~~~

`StateEncoder` 仍用于向其他 peer 做 best-effort keyed full-state fan-out；
`StatePageEncoder` 专用于受 remote frame cap 约束的 StateRequest 分页。canonical
`Client` 已把自己的 `EndpointStateStore` 固定注册为两个 encoder 和 transaction apply
handler，不应再覆盖这些 hook。产品固件还必须在 `begin()` 前
注册 `Client::setStateWriteHandler()`；状态仓完成容量预检后先让产品事务接受硬件/业务
变化，成功才提交事实状态。没有 handler 的变化 Apply 返回 UnsupportedFeature，避免
把远端期望直接伪装成设备事实。

Report 值要求 field 具有 `AccessRead` 或 `AccessNotify`；Apply 要求
`AccessWrite`；COMMAND 优先要求 `AccessCommand`（暂时兼容旧
`AccessWrite` descriptor）；EVENT 要求 `AccessEvent`。EVENT 不会递增
stateRevision。

## 6. App 页面与状态数据

页面 schema 继续由 App/服务器保存：组件布局、颜色、文案以及组件到 endpoint
key/property 的绑定不进入 PATCH。设备 Manifest 只描述能力、类型、访问权限
和约束。App 收到 Report 后按绑定更新组件；用户操作转换为 COMMAND 或 Apply。

因此本方案不是把阿里云式完整物模型强制塞进固件，也不是让设备代码成为页面
布局的唯一真相。Manifest 可以帮助自动生成默认页面，但用户拖拽后的页面仍是
独立资产，同一 endpoint 也可同时被页面、自动化、语音和 BLE 本地控制绑定。

Broker/Gateway 暴露的 latest state 必须携带 `manifestFingerprint`。App 只在它等于当前 Manifest
fingerprint 时提交到页面 state store；不相等时先重新读取 Manifest/State。PageSchema 的
`sourceManifestFingerprint` 不等于当前 Manifest 时，状态同步仍可完成，但页面进入 stale 并提示迁移，
不能由服务器自动覆盖用户拖拽布局或 Agent 已确认的修改。

## 7. 当前边界

- revision 为 uint32；到达最大值后 Runtime 明确返回资源耗尽，不静默回绕。
- Runtime 只提供结构、schema 和版本原子性，不自动回滚业务 handler 的硬件
  副作用。
- 设备当前不主动发起 StateRequest，因此收到 StatePage 只会作为设备侧无消费者的响应
  忽略（若错误设置 AckRequired 则拒绝）；设备间
  shadow 同步若有需求，应另行冻结角色与所有权规则。
- outbound EVENT 当前是 best effort；Broker/App 实时 fanout 不提供跨重连补发。需要强业务确认的动作
  使用 COMMAND；history/automation 需另行实现持久 event ingest。后续若增加可靠 EVENT 发送 API，必须
  同时冻结 Cloud ACK、幂等 identity 和持久消费语义，不能只打开 AckRequired 标志。
- PATCH Apply 成功先 ACK 来源，再向其他合格 peer best-effort 发送同 revision 的
  完整 keyed snapshot；来源不重复回显，ID-mode body 不跨 session 转发。
