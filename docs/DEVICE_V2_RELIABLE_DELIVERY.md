# Device v2 可靠请求幂等与 PATCH outbox 契约

> 状态：设备端已实现并通过主机、ESP32、UNO R4 编译验证；App/服务器接入必须遵循相同的 sequence、窗口和重试规则。

## 1. 解决的问题

MQTT QoS 1 只能说明消息至少一次到达 broker 或客户端，BLE notification/fragment 队列也不能证明业务响应已经被对端收到。若 ACK 丢失，发送方会重发；没有幂等层时，开锁、计费、步进电机或场景触发等 COMMAND 会被再次执行。

BBP/2 因此把可靠性放在 transport 之上：MQTT 和 BLE 使用完全相同的 sequence、ACK、ERROR、重复检测与重试状态机。

## 2. HELLO 协商

`FeatureReliableDelivery = 1 << 6`。声明该 feature 时，HELLO 必须同时包含：

~~~text
key 9: reliableReceiveWindow:uint8
~~~

窗口范围是 1..16。设备默认值由 `BLINKER_RELIABLE_RECEIVE_WINDOW` 决定，当前为 2；运行时配置不得超过编译期容量。

窗口表示接收方能保留多少个已处理请求结果。发送方对同一 `(transportId, sessionId)` 的未确认可靠请求数不得超过对端窗口。未协商该 feature 时，旧的 `AckRequired` 仍可获得 ACK，但不能假设接收方会抑制重复执行。

## 3. 可靠 COMMAND / PATCH Apply / EVENT

可靠请求必须满足：

- 已完成 HELLO，双方协商 `FeatureReliableDelivery`；
- `FlagAckRequired=1`、`FlagIsResponse=0`；
- sequence 是非零 `uint16`；
- 重试逐字节保持相同的 kind、sequence、语义 flags 和 CBOR body；
- sequence 在当前 session 内单调使用，不得在仍有未完成请求时复用。

设备按 `(transportId, sessionId, sequence)` 查找记录，并用 SHA-256 的前 128 bit 比较以下内容：

~~~text
"BLINKER-REQUEST-V1"
|| kind
|| (AckRequired | IdMode)
|| bodyLength(network order)
|| exactBodyBytes
~~~

处理结果：

| 情况 | 行为 |
|---|---|
| 新 sequence | 校验并执行业务回调，先记录结果，再发送 ACK/ERROR |
| 相同 sequence、相同摘要 | 不再执行回调，重放同一语义的 ACK/ERROR |
| 相同 sequence、不同摘要 | 拒绝，返回 `ERROR SequenceConflict(9)` |
| 记录已被窗口淘汰 | 作为新请求处理 |

ACK body 的 `acknowledgedSequence` 或 ERROR 的 `relatedSequence` 指向原请求。响应 frame 自身可以使用新的发送 sequence；调用方不得用响应 frame sequence 匹配请求。PATCH Apply 的 ACK/StateConflict ERROR 还会携带 stateRevision，详见 `DEVICE_V2_STATE_SYNC.md`。

记录在断连、transport 下线、Runtime stop 或新 session 建立时清除。不同 BLE central、MQTT session 和 transport 即使使用相同 sequence 也互不影响。

因此服务器/Broker 重启导致 MQTT/BBP session 重建后，不得自动重发上一 session 已发送但未确认的 COMMAND；
它只能把结果标为 `outcome_unknown`。跨 session exactly-once 需要另行冻结稳定 delivery ID，并由设备使用
持久去重 journal；16 位 session sequence 不能承担该职责。

## 4. 错误与部分执行边界

设备会缓存成功 ACK，也会缓存校验/业务拒绝产生的 ERROR。即使首次响应发送失败，完全相同的重试也不会再次调用业务代码。

幂等不等于事务。当前 map visitor 依次执行 endpoint；如果第一个 endpoint 已成功、第二个失败，Runtime 不会自动回滚。对不可逆副作用应采用单 endpoint COMMAND，或在业务层引入显式 transaction endpoint；在事务协议冻结前不要把多个不可逆动作塞进同一 map。

窗口只覆盖合理的在途重试。发送方收到 ACK/ERROR 后必须停止重发；不能在不断发送新请求并淘汰旧记录后，又重放很早的 sequence。

## 5. 设备主动可靠 PATCH

普通 `sendPatch()` 是 best-effort versioned Report。需要端到端确认时使用：

~~~cpp
uint8_t retryFrame0[512];
ReliableFrameSlot slots[1];
slots[0].storage = MutableByteSpan(retryFrame0, sizeof(retryFrame0));

ReliableOutbox outbox(hub, clock, slots, 1);
outbox.setHandler(onDelivery, context);
runtime.setReliableOutbox(&outbox); // start() 前

uint16_t sequence = 0;
Result accepted = runtime.sendReliablePatch(body, target, sequence);
~~~

每个 slot 的 buffer 必须互不重叠，并且至少等于 `DeviceRuntimeConfig::maxFrameSize`。Runtime 把完整 BBP/2 frame 复制进 slot，后续始终重发完全相同的字节；调用方可在 API 返回后立即复用原 body/transmit buffer。

可靠 PATCH：

- 只允许 `Transport` 或 `Session` 定向发送，禁止 broadcast；
- 要求目标 peer 已协商可靠 feature；
- 本地待确认数不得超过对端 HELLO 窗口；
- 返回 `Result::success()` 只表示已被 outbox 接受，不表示对端已处理；
- 最终结果通过 `ReliableDeliveryHandler` 报告。

终态为 `Acknowledged`、`PeerRejected`、`RetryExhausted`、`TransportError` 或 `SessionClosed`。来自错误 transport/session 的 ACK/ERROR 不会完成 slot；断连会立即释放对应 slot。

## 6. 默认重试策略

`ReliableRetryPolicy` 默认：

| 参数 | 默认值 |
|---|---:|
| initial delay | 250 ms |
| maximum delay | 4,000 ms |
| maximum attempts | 4 |
| overall timeout | 15,000 ms |

成功提交到 transport 后按指数退避；`WouldBlock` 不计作一次成功发送，但仍受总超时约束。`NotConnected` 等非临时错误在首次 enqueue 时直接返回，在已入队重试阶段则产生 `TransportError`。

## 7. App 与服务器要求

App/服务器作为 COMMAND 发送方必须保存未完成请求的完整 frame，超时后用相同 sequence 和 body 重发，并在 ACK/ERROR 后释放。作为 PATCH 接收方，也必须按自己 HELLO 宣告的窗口实现同样的重复检测和结果重放，不能只回 ACK 而重复更新 UI/自动化。

服务器不得在转发时重写 BBP/2 sequence、flags 或 CBOR body。若服务器终止协议并重新发起 COMMAND，则它是新的 peer，应建立自己的 session sequence 和 replay cache。

当前 Gateway/Broker 实现把 exact frame 和终态写入 SQLite ledger，在同一 session 内退避重传；Broker
重启后可重放已落盘的 ACK/ERROR 终态，遗留 inflight 则安全收口为 `outcome_unknown`，不跨 session 重发。
