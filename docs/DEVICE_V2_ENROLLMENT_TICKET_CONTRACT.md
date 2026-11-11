# Device V2 EnrollmentTicket v1 合同

> **已退役（2026-08-23）：**设备实现与三端 golden 已删除，由 `BleEnrollmentGrant` 两消息合同取代。本文只保留历史审计信息。

> 状态：2026-08-17 R4b-4a signed common body 已冻结。设备 C++、App-compatible TypeScript 与
> Server Node exact golden 已一致。R4b-4b 已冻结普通 DIY 的 Noise NN secure-session Core wire，
> R4b-4c 已完成 LocalSetup application envelope、Ticket relay/accept 与授权 operation gate，R4b-4d 已完成
> 掉电一致性 saga、ConfigureNetwork 与 App-relay Cloud path。official composition 尚未完成，不能据此宣称
> BLE 配网可发布。

## 1. 边界

`EnrollmentTicket` 是服务器对“当前物理 setup session 可以开始首次认领，并安装指定首个 controller”的
短期授权。它不是 deviceId、SetupCode、CloudCredential、MQTT 密码或日常 BLE 密码，也不进入设备 Active
记录。

Ticket 必须绑定：

- 当前 16 B `deviceInstanceId`；
- 当前 16 B 随机 `setupSessionId`；
- 当前 provisional secure channel 完成握手后的 32 B `setupTranscriptHash`；
- 当前 `ownershipGeneration`；
- App 本次生成的 16 B `controllerId`、`SHA-256(controller secret)` 和权限；
- BLE-only 或 WiFi+BLE 明确允许的首次操作；
- 服务器生成的 ticket ID、nonce、最长 15 分钟有效期和签名 key。

Ticket 不包含用户 JWT、owner/tenant 文本、productId、WiFi SSID/密码、raw controller secret、
CloudCredential 或 SessionCredential。owner 与 product model 由服务器的 ticket intent 记录绑定到
`ticketId`；不把可关联账号的稳定摘要复制到设备 wire。

## 2. canonical CBOR

body 是 16 对、按整数 key 递增的 definite-length canonical CBOR map：

| key | 字段 | 约束 |
|---:|---|---|
| 0 | version | 固定 `1` |
| 1 | ticketId | 16 B，非零 |
| 2 | deviceInstanceId | 16 B，非零 |
| 3 | setupSessionId | 16 B，非零 |
| 4 | setupTranscriptHash | 32 B，非零 |
| 5 | ownershipGeneration | `uint32`，非零 |
| 6 | controllerId | 16 B，非零 |
| 7 | controllerSecretDigest | 32 B，非零 |
| 8 | controllerPermissions | 现有四位 Controller permission mask，非零 |
| 9 | authorizedOperations | 仅允许下表 bit |
| 10 | ticketNonce | 16 B，非零 |
| 11 | issuedAt | epoch seconds，非零 |
| 12 | expiresAt | `issuedAt < expiresAt <= issuedAt + 900` |
| 13 | serverKeyId | `uint32`，非零 |
| 14 | signatureAlgorithm | `1=Ed25519`，`2=P-256 SHA-256 raw r||s` |
| 15 | signature | 固定 64 B |

`authorizedOperations`：

| bit | 含义 |
|---:|---|
| 0 | 允许后续 CloudCredential Enrollment |
| 1 | 允许在当前 authenticated setup session 配置网络 |

BLE-only 的值为 `0`；WiFi+BLE 首次配网为 `3`。Claim 和首个 Controller 安装是 Ticket 固有语义，不再
重复占 bit。未知 bit 必须拒绝，不能按“未来兼容”静默忽略。

最大 body 是 268 B。签名 transcript 是 16 项 canonical CBOR array：

~~~text
[
  "blinker.enrollment-ticket.v1", version,
  ticketId, deviceInstanceId, setupSessionId, setupTranscriptHash,
  ownershipGeneration, controllerId, controllerSecretDigest,
  controllerPermissions, authorizedOperations, ticketNonce,
  issuedAt, expiresAt, serverKeyId, signatureAlgorithm
]
~~~

transcript 最大 216 B；设备对其做 SHA-256，再使用发布配置的 server key ring 验签。P-256 继续要求 raw
`r||s` 与 low-S；App 不能让未认证输入选择算法或 key。

## 3. 验证与时间

设备 verifier 必须把 Ticket 与本次 session 持有的全部 context 做 exact match，再验服务器签名。任一
device/session/transcript/generation/controller/permission/operation 不同都返回认证失败，并且不能开放 WiFi、
Claim 或 Controller mutation。

有可信时钟的平台同时检查 `issuedAt <= now < expiresAt`。没有可信时钟的 Arduino 不猜测 epoch：它依靠
随机 setupSessionId、当前握手 transcript 和服务器兑换时的强制 TTL；服务器始终必须检查过期、单次消费、
attempt limit 与 `(ticketId, deviceInstanceId, generation, requestId)` 幂等绑定。

## 4. secret 与持久化

- App 仅在内存和 OS 安全存储中持有 raw controller secret；Gateway 只接收 digest。
- 完整 Ticket bytes 只存在于当前受保护 setup session；不得写日志、analytics、剪贴板或普通状态仓。设备
  BSG2 只持久化 ticketId 与已验证 controller/generation/profile context，不持久化签名 body、nonce 或 TTL。
- Claim 被服务器接受后，恢复依赖同一 Claim request/journal 和服务器 transaction，不把 Ticket 复制进
  `OwnershipRecord`、`CloudCredential` 或 `ControllerCredential`。
- session 结束立即清除完整 Ticket。设备正常重启只使用已持久化 Ownership/Cloud/Controller credential；
  未确认网络修复仍需打开物理 setup window 并取得恢复 Ticket。
- session 丢失时旧 Ticket bytes 不能迁移到新 setup session。Gateway 对同一未完成 durable transaction
  重新签发绑定新 session/transcript/TTL 的 Ticket，但必须复用原 ticketId、generation、controller context
  与 operation profile；同一 session 的 exact retry 则必须返回原 bytes。

## 5. 当前实现与未完成项

设备侧已经实现 allocation-free codec 与 verifier：只借用输入 view，verifier 本身仅持有 server verifier
引用；caller 提供 216 B transcript workspace，成功或失败后均清零。

R4b-4c/R4b-4d 已完成：

1. strict Hello/Ticket/operation canonical CBOR envelope；
2. 完整 setupSessionId、最终 Noise transcript hash callback 与 Ticket exact gate；
3. request sequence/exact replay、固定 operation profile 和平台无关 mutation handler seam；
4. operation 4 InitialControllerInstall、operation 5 ConfigureNetwork 与 operation 6～8 App-relay Cloud path；
5. 248 B BSG2 单一 durable saga、inner-clear/outer-save 掉电恢复和未确认 network candidate 修复。

仍未完成：

1. ESP32/UNO R4 的 official BLE adapter/composition 与 BSG2 平台 storage 接线；
2. 真机 WiFi candidate connect/confirm/错误网络恢复、BLE/WiFi 共存、资源、掉电和断连测试；
3. 生产 App/Gateway codec、transaction reducer 与 E2E。

普通 Arduino `blinker::BLE`/`blinker::WiFiBLE` 已固定使用无需 SetupCode 的
`Noise_NN_25519_AESGCM_SHA256`；需要预置 32 B 高熵 PSK 的 NNpsk0 仅为显式 Enhanced/Manufactured 档。
两者共用 `NoiseNnSession`，不得通过把 EnrollmentTicket 当 PSK 来绕过本合同。

广播中的 8 B `setupSessionLocator` 与本合同 16 B `setupSessionId` 是两个字段。locator 只负责找到当前
setup window；完整 ID 必须由 application coordinator 生成/持有，并在受保护 payload 中交给 App。Ticket
不得绑定补零、复制或 hash 猜出的 locator 替代值。

参考实现与门禁：

- `src/BlinkerV2/provisioning/EnrollmentTicketContract.*`
- `src/BlinkerV2/provisioning/EnrollmentTicketVerifier.*`
- `src/BlinkerV2/provisioning/LocalSetupContract.*`
- `src/BlinkerV2/provisioning/LocalSetupApplication.*`
- `src/BlinkerV2/provisioning/LocalSetupOperationContract.*`
- `src/BlinkerV2/provisioning/LocalSetupSaga*`
- `tests/host/test_enrollment_ticket_main.cpp`
- `tests/host/test_local_setup_main.cpp`
- `tests/golden/enrollment_ticket_golden_vectors.properties`
- `tests/golden/local_setup_golden_vectors.properties`
