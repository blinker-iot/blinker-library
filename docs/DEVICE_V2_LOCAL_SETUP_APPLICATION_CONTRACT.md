# Device V2 Local Setup application contract v1

> **已退役（2026-08-23）：**8-operation application 与 durable saga 已删除。纯 BLE 只使用 Hello/Enroll；WiFiProv 使用独立 `blinker-config/2` endpoint。

> 状态：2026-08-17，R4b-4d Core 已完成。本文冻结 Noise 之上的 LocalSetup envelope、
> EnrollmentTicket 门禁、8 个 operation、App exact-relay Cloud 路径，以及单一掉电一致性 saga。
> 双平台 official internal BLE-only/WiFi+BLE composition 已于 R4b-5/6 完成软件与资源门禁；生产
> App/Gateway codec、公开 facade 与真机联调仍未完成，因此“R4b-4d 完成”不等于“BLE 配网产品可发布”。

## 1. 分层与边界

~~~text
BLE ATT/GATT
  -> 4 B fragment + reassembly
  -> B3 local secure record
  -> Noise NN / NNpsk0
  -> LocalSetup v1 canonical CBOR envelope
  -> ILocalSetupOperationHandler
  -> one durable Ownership / Controller / Network / Cloud saga
~~~

LocalSetup 是首次配网控制面，不是 BBP/2 日常数据面。CBOR body 只存在于 caller-owned RX/TX/
workspace 和在途 secure record 中；它不是页面、Manifest、数据库对象或重启登录凭据。设备页面仍由
App/服务器 PageSchema 管理，设备能力仍由 BBP/2 Manifest 描述。

`LocalSetupApplication` 不直接访问 flash、WiFi SDK、HTTP、MQTT 或平台 BLE API。平台和产品差异只在
crypto/link/storage adapter 与 `ILocalSetupOperationHandler` 后面，ESP32、UNO R4/Renesas 和其他 MCU
不得复制第二套 envelope/state machine。

## 2. 会话流程

1. 物理操作打开有时限的 setup window，并生成非零 8 B `setupSessionLocator`。同一窗口断连重试必须
   复用 locator；新窗口必须旋转 locator。
2. 产品 handler 的 `prepareSetupSession(locator, generation)` 只读检查 durable 状态：fresh 返回目标
   generation；存在可恢复 journal 时返回同一 generation；已经 Active 且无可恢复事务时返回
   `StateConflict`。application 不能仅凭 Ownership Active 提前拒绝，否则会封死“已写 Ownership、未装
   Controller”这一掉电窗口。
3. 新 locator 生成完整随机 16 B `setupSessionId`；同一 locator 的安全重连复用完整 ID。8 B locator
   绝不能补零、复制或 hash 后冒充 16 B ID。
4. App 把 `HelloRequest` 放入 Noise initiator handshake payload。普通 NN 的第一条 payload 是明文，但
   HelloRequest 不含身份或 secret；设备的 `HelloResponse` 位于 responder handshake 的受保护 payload。
5. responder handshake 成功进入 TX 槽后，endpoint 取得最终 32 B Noise handshake hash，并调用
   `secureSessionReady()`。hash 不能预先放进 HelloResponse，否则会形成依赖自身响应的循环。
6. App 生成独立 16 B controllerId 与 32 B controller secret，向 Gateway 提交 Hello context、最终
   handshake hash、secret digest、权限和 `ticketOperations`，获取 exact signed EnrollmentTicket。
7. App 在 Noise transport 中发送 exact `TicketOffer`。设备 exact-match 当前 device/session/hash/
   generation/controller/permissions/operations 并验 server signature；成功前任何 mutation 都不可达。
8. Ticket 接受后请求必须串行执行。handler 负责把下述 operation 绑定到一份 crash-consistent saga；
   application 只负责授权、顺序、幂等身份和有界 buffer。

## 3. canonical CBOR envelope

所有 map 为 definite-length、整数 key 严格递增、整数/长度最短编码；不允许额外字段、重复字段、text、
indefinite length 或 trailing bytes。公共字段为 `0=version(1)`、`1=messageType`、`2=requestId`；
`requestId` 是非零 uint32。

| type | 名称 | exact map |
|---:|---|---|
| 1 | HelloRequest | `{0:v, 1:1, 2:requestId}` |
| 2 | HelloResponse | `{0:v, 1:2, 2:requestId, 3:deviceInstanceId(16 B), 4:setupSessionId(16 B), 5:setupSessionLocator(8 B), 6:ownershipGeneration, 7:ticketOperations}` |
| 3 | TicketOffer | `{0:v, 1:3, 2:requestId, 3:ticket(bstr)}` |
| 4 | TicketAccepted | `{0:v, 1:4, 2:requestId, 3:ticketId(16 B), 4:authorizedOperations}` |
| 5 | OperationRequest | `{0:v, 1:5, 2:requestId, 3:operation, 4:body(bstr)}` |
| 6 | OperationResponse | `{0:v, 1:6, 2:requestId, 3:operation, 4:body(bstr)}` |
| 7 | ErrorResponse | `{0:v, 1:7, 2:requestId, 3:error, 4:retryClass}` |

`HelloResponse.7` 是设备要求 Gateway 签入 Ticket 的 `ticketOperations`，不是尚未验证的
`authorizedOperations`。v1 只允许两个完整 profile：BLE-only=`0`，WiFi+BLE=`3`（CloudEnrollment bit0 +
NetworkProvisioning bit1）；`1`、`2`、未知 bit 和自动扩大/缩小权限均拒绝。

最大尺寸：request 529 B、response 273 B、operation request body 512 B、operation response body 256 B。
operation workspace 固定 512 B，由 256 B scratch + 256 B response-body storage 组成；Ticket verifier 在前
216 B 中复用 transcript workspace。所有 workspace 使用后清零，application 不保留完整 Ticket。

## 4. operation 与下一层合同

| operation | 请求 body | 成功响应 body | profile |
|---:|---|---|---|
| 1 BeginOwnershipClaim | empty | exact OwnershipClaimRequest（最大 63 B） | 两者 |
| 2 ApplyOwnershipGrant | exact signed OwnershipClaimGrant（最大 243 B） | exact OwnershipClaimReceipt（最大 165 B） | 两者 |
| 3 CompleteOwnershipClaim | exact signed OwnershipClaimCommitAck（最大 250 B） | empty | 两者 |
| 4 InstallInitialController | `{0:1, 1:controllerSecret(32 B bstr)}`（最大 38 B） | exact ControllerMutationReceipt（最大 145 B） | 两者 |
| 5 ConfigureNetwork | `{0:1, 1:ssid(bstr), 2:authentication, 3:credential(bstr), 4:hidden(bool)}`（最大 110 B） | `{0:1, 1:networkGeneration, 2:CandidateStored(1)}`（最大 10 B） | 仅 WiFi+BLE |
| 6 BeginCloudEnrollment | empty | exact CloudCredentialEnrollmentRequest（最大 228 B） | 仅 WiFi+BLE |
| 7 ApplyCloudEnrollmentGrant | exact signed CloudCredentialEnrollmentGrant（最大 248 B） | exact CloudCredentialEnrollmentReceipt（最大 190 B） | 仅 WiFi+BLE |
| 8 CompleteCloudEnrollment | exact CloudCredentialEnrollmentCommitAck（最大 190 B） | empty | 仅 WiFi+BLE |

operation 4 是首次 Controller 的专用安装合同，不再额外嵌套一份 `ControllerGrant`。EnrollmentTicket 已签名
绑定 controllerId、secret digest、permissions、generation 与当前 Noise transcript；raw secret 只在 Noise
保护的 body 中出现。设备先核对 SHA-256(secret)，再安装 version 1 Ownership-domain credential，并以
Ticket ID 作为该次安装 receipt 的 grantId。后续 rotate/revoke 仍使用独立 Controller Control 合同。

operation 5 的成功只表示未确认 candidate 已写入 `WifiCredentialStore`，不表示已连上 AP，更不表示 MQTT
online。相同 body 重放只校验当前 profile，不增加 generation；在 Cloud 开始前可以用新的 operation request
替换未确认 candidate。Cloud Enrollment 已完成但首次 AP 连接失败时，也可用绑定同一 durable transaction 的
新 Noise session/Ticket 替换未确认 candidate；`confirmActive()` 成功后该恢复入口关闭。连接、超时、确认和
fresh-device 失败策略属于 R4b-5 composition，不得塞进 operation 5 的同步响应。

Cloud 路径已固定为 App exact relay：设备无需先联网，App 把 operation 6 的 exact request 交给 Gateway，
把 exact signed Grant 作为 operation 7 body，再把 Gateway 对 exact receipt 生成的 HMAC CommitAck 作为
operation 8 body。长期 cloud secret 只以 X25519/HKDF-SHA256/AES-256-GCM envelope 出现在 request 中，
不会 raw 发送给 App。该路径不复用 WiFi-only ManualDeviceKey proof，也不把 Ticket 持久化为日常 cloud credential。

## 5. 顺序、重放与错误

- 每条连接从 requestId 1 开始，新的请求只能是上一项 `+1`；跳号返回 error 1003 /
  `CorrectSequence`，不消费序号。
- 相同 requestId + 相同 SHA-256(encoded request) 是连接内 exact replay。application 再调用 handler，并设置
  `exactReplay=true`；handler 以 durable Ticket context、当前 saga state 和各阶段 exact body digest/内层
  journal 重建同一逻辑结果，不重复 mutation。application 的连接内 sequence window 不写入 flash。
- 相同 requestId 但 bytes 不同属于认证冲突，立即 fail closed；不能把它当作可覆盖的重试。
- handler 的可重试语义错误可编码为 ErrorResponse；认证失败、malformed、buffer alias/越界等通道级错误
  终止 session，避免向未认证 peer 提供解析 oracle。
- error 为 1000 Malformed、1001 UnsupportedVersion、1002 UnsupportedOperation、1003
  SequenceConflict、1004 AuthenticationRequired、1005 StateConflict、1006 Busy、1099 Internal；retry
  class 为 0 Never、1 CorrectSequence、2 Backoff、3 RestartSession。
- Ticket 有可信墙钟时每次 operation 前检查 expiry；无可信 RTC 时仍绑定随机 session/transcript，Gateway
  必须执行 TTL、单次消费、attempt limit 和 durable transaction recovery。

## 6. 掉电一致性硬约束

`ILocalSetupOperationHandler` 是唯一产品 mutation seam，不是“依次调用四个 coordinator”的便利接口。
R4b-4d 的 `LocalSetupSaga` 使用一份 248 B `BSG2` durable record，覆盖：

~~~text
Fresh
  -> Authorized
  -> ClaimPrepared
  -> OwnershipGranted
  -> ClaimCommitVerified
  -> OwnershipCommitted
  -> ControllerInstalled
  -> NetworkCandidateCommitted (WiFi+BLE)
  -> CloudEnrollmentPending (WiFi+BLE)
  -> CloudCredentialGranted (WiFi+BLE)
  -> CloudCommitVerified (WiFi+BLE)
  -> Complete
~~~

`ClaimCommitVerified` 与 `CloudCommitVerified` 是刻意存在的外层 checkpoint：先持久化“ACK 已验证”，再清
内层 transaction journal，最后推进外层状态。这样在 inner clear 与 outer save 任一处掉电都可幂等恢复。
Controller、WiFi 与 Cloud store 仍各自负责原子写；Saga 只保存 context 和 digest，不复制 raw Controller、
WiFi 或 Cloud secret。每一步先 durable 再返回成功，并能在任意写入、响应发送、BLE 断连或重启后恢复同一
generation 与 exact 结果。

`Complete` 表示身份与 Cloud enrollment 已完成，不冒充 AP 已确认。WiFi+BLE 的 active profile 尚未
`confirmed` 时，`prepareSetupSession()` 仍允许同一事务的网络修复；确认后才返回 `StateConflict`。BLE-only
的 Complete 没有此恢复例外。

## 7. App 实施输入

1. 实现独立 production LocalSetup codec/state reducer，并把
   `tests/golden/local_setup_golden_vectors.properties` 纳入 CI；reference TypeScript 不能直接冒充业务模块。
2. 按 Service Data suite bit 固定 ordinary NN 或显式 NNpsk0；完成 responder handshake 后使用最终 hash
   请求 Ticket。不得从 locator 猜 sessionId，也不得把 Hello 阶段标为“设备已认证”。
3. controller secret 只进 Keychain/Keystore 和受保护内存；向 Gateway 只发 digest。Ticket 与 WiFi 密码
   不写 localStorage、analytics、崩溃日志、剪贴板或普通 Redux/Vuex 持久仓。
4. request 严格串行；超时重发 exact bytes/requestId，收到 CorrectSequence 时修正序号，RestartSession 时
   重新握手并向 Gateway 获取绑定新 transcript、但复用同一 durable ticketId 的恢复 Ticket。
5. UI 分离 Setup secure、Ticket accepted、Ownership、Controller、Network、Cloud、Online，不以 BLE ACK、
   HTTP 2xx 或 MQTT online 代替各事务 CommitAck。

## 8. Gateway 实施输入

1. Ticket issue 必须按受信 product/profile registry 固定 `ticketOperations`、signature algorithm 与 keyId；
   签名绑定 exact device/session/hash/generation/controller context，TTL 不超过 900 s。
2. 以 ticketId + physical instance + generation 建立 durable transaction。同一 Noise session 的 exact retry
   返回首次 Ticket bytes，不刷新时间戳或重签；断连或过期后的新 transcript 可签发新的 session-bound Ticket
   bytes，但必须复用原 ticketId、generation、controller context 和 operation profile，直到 Saga 完成且网络
   已确认。不得创建第二个 logical device。
3. Ownership、Cloud 与 Controller receipt 使用各自冻结的 exact codec、验签和 digest；LocalSetup relay 不得
   decode 后重建这些 body。首次 Controller 安装是上述 operation 4 专用 body，不能再要求一份冗余 Grant。
4. 生产日志只保留 ID、digest、状态、错误类别和 keyId；删除 Ticket、JWT、controller secret、WiFi credential
   与解密后的敏感 body。
5. 实现 operation 6/7/8 的 App-relay API 与事务状态：request、signed Grant、receipt、HMAC CommitAck 均保存
   exact bytes/digest；不得套用 legacy authKey、WiFi-only ManualDeviceKey 或空 2xx。

## 9. 当前验证与发布边界

- Windows ASan：36/36；deterministic protocol fuzz：200,000 次；架构边界检查通过；
- LocalSetup C++/App-compatible TypeScript/Node exact golden 已覆盖 initial Controller、ConfigureNetwork、
  Cloud begin envelope；Saga Host 测试覆盖 BLE-only 与 WiFi+BLE、outer-save/inner-clear 掉电和未确认网络修复；
- UNO R4 Core 1.5.3 Small SizeProbe：52,324 B Flash / 14,668 B global RAM；32 位
  `LocalSetupApplication/SagaRecord/SagaStore/Saga` 精确为 216/240/4/40 B，BSG2 flash record 为 248 B。
  相比上一 probe 的 +284 B global RAM 是三个新增 `sizeof` 测量数组常驻造成，不是产品自动分配第二套 buffer；
- 同一源码已生成 ESP32 Core 3.3.1、UNO R4 Core 1.5.3 与 Wio Terminal Core ELF。

仍阻断发布：R4b-5 official platform composition/package、目标板 radio/crypto/heap/stack/掉电/断连/WiFi+BLE
共存/错误网络恢复/24 h，以及生产 App/Gateway E2E。HC-05/Bluetooth Classic SPP 不在当前范围。
