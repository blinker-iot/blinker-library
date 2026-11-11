# Device V2 Ownership Claim Contract v1

> **已退役（2026-08-23）：**设备不再保存 OwnershipRecord，相关 contract/coordinator/journal 已删除；账号与 ACL 只保存在 Service。

> 状态（2026-08-16）：设备侧 exact contract、48 B `BOC2` 掉电 journal、server-signed CommitAck 与
> coordinator 已冻结；
> App/Gateway 生产实现尚未接入。本文取代旧 Enrollment/1 中“Activation 同时创建 ownership 与 cloud
> credential”的实验语义。后续 Cloud Credential Enrollment、Ownership Reset/Transfer 已冻结；旧 WiFi-only
> EnrollmentKey outer 已由 `DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md` 取代。

## 1. 边界

Ownership Claim 只完成一个状态变化：

~~~text
Unclaimed(DeviceInstanceId, generation)
  -> Active(logicalDeviceId, same generation, SHA-256(exact OwnershipClaimGrant))
~~~

它不创建 CloudCredential、ControllerCredential、MQTT token、页面、productId，也不保存 ManualDeviceKey/Ticket。`DeviceInstanceId` 是设备 CSPRNG 生成并持久化的公开 16 B 实例 ID；MAC、广播名和 secret 都不是 device ID。

通用 ownership body 不携带 profile-specific bootstrap material：

- WiFi-only 手动模式：设备向 Gateway 提交 exact request，ManualDeviceKey id/proof 放在 HTTP authorization envelope；App 已预创建 AwaitingFirstClaim logicalDeviceId；
- WiFi+BLE / BLE-only：App exact-byte relay request，并用用户登录态、短期 EnrollmentTicket 和当前 setup session 向 Gateway 授权；
- Manufactured：factory proof/registry 在 Gateway 的认证信封中验证；
- Device Recovery：已验证 RecoveryGrant 的新实例必须先建立 exact N+1 tombstone；随后 common ClaimRequest
  不增加字段，但 Gateway 的 recovery outer 只能返回原 logicalDeviceId，并在 Claim commit 时完成 N -> N+1
  physical relationship cutover；
- LocalBLE：不进入本合同，使用 Local controller domain。

这样 ESP32、UNO R4/Renesas 和其他 MCU 共用完全相同的 ownership codec/coordinator，BLE、WiFi 只负责承载与认证。

## 2. 流程与恢复

1. 设备读取 `OwnershipRecord`。全新存储使用 generation 1；reset 后使用持久 tombstone 的 generation。
2. `beginClaim()` 生成非零 16 B `requestId` 和 16 B `claimNonce`，先写入独立 48 B `BOC2` journal，再形成 exact `OwnershipClaimRequest`。同一运行窗口或掉电重启后均返回相同 bytes。
3. Gateway 验证外围 ManualDeviceKey/Ticket/factory/user authorization，按 `(deviceInstanceId, generation, requestId)` 建幂等事务。手动模式返回 Key 已绑定的预创建 logicalDeviceId；自动 profile 才预留新 logicalDeviceId。两者都返回同一份 server-signed `OwnershipClaimGrant`。
4. 设备匹配 request/device/generation/nonce；有可信时间时再校验 `issuedAt <= now < expiresAt`；无可信墙钟时仍必须校验当前 claim window 与 server signature。
5. 设备先原子保存 Active `OwnershipRecord`，其中 fingerprint 为 `SHA-256(exact encoded grant)`，但保留 Claim journal，再发送 `OwnershipClaimReceipt`。
6. App/Gateway 对 receipt 做 strict decode、关联当前认证会话并幂等提交，返回绑定
   `SHA-256(exact receipt)` 的 server-signed `OwnershipClaimCommitAck`；设备验签、匹配 request/device/
   logical ID/generation/digest 后才由 `completeClaim()` exact-clear journal。receipt 本身不是 bearer credential，
   空 204/普通 2xx 不是提交证明。
7. 若设备在 Grant 前、Ownership 落库后或 Receipt 响应前掉电，重启使用 journal 重放同一 request；Gateway 返回 exact grant 后，设备以持久 fingerprint 重建完全相同的 receipt，即使 grant 此时已过期。不同 request/grant 一律冲突。

设备写入 Grant 后本地 Ownership 即 Active，但 Claim journal 在 signed CommitAck 前始终保留。Gateway 只有收到
合法 receipt/commit 才把其事务标成 Active；在此之前必须重发首次保存的 exact grant，不能重新分配 logical ID
或更新时间戳。CommitAck 响应丢失时，重复 receipt 必须返回同一语义、仍可验签的 Ack。

## 3. Exact canonical CBOR

共同规则：definite-length map、升序 unsigned numeric key、最短整数/长度编码、拒绝 unknown/duplicate/out-of-order/trailing data。ID/nonce/digest/signature 为 bstr；logical ID 为 UTF-8 text。

### 3.1 OwnershipClaimRequest（最大 63 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | requestId | 16 B，非零 |
| 2 | deviceInstanceId | 16 B，非零 |
| 3 | ownershipGeneration | uint32，非零 |
| 4 | claimNonce | 16 B，非零 |

### 3.2 OwnershipClaimGrant（最大 243 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | grantId | 16 B，非零 |
| 2 | requestId | 16 B，非零 |
| 3 | deviceInstanceId | 16 B，非零 |
| 4 | logicalDeviceId | 1..64 B UTF-8 |
| 5 | ownershipGeneration | uint32，非零 |
| 6 | claimNonce | 16 B，非零 |
| 7 | issuedAt | uint64，非零 |
| 8 | expiresAt | uint64，必须大于 issuedAt |
| 9 | serverKeyId | uint32，非零 |
| 10 | signatureAlgorithm | 1 Ed25519；2 ECDSA-P256-SHA256 raw `r||s` |
| 11 | signature | 固定 64 B |

签名输入为下面 canonical array 的 SHA-256；最大 199 B：

~~~text
[
  "blinker.ownership-claim-grant.v1", 1,
  grantId, requestId, deviceInstanceId, logicalDeviceId,
  ownershipGeneration, claimNonce, issuedAt, expiresAt,
  serverKeyId, signatureAlgorithm
]
~~~

### 3.3 OwnershipClaimReceipt（最大 165 B）

| key | 字段 |
|---:|---|
| 0 | contractVersion = 1 |
| 1 | grantId |
| 2 | requestId |
| 3 | deviceInstanceId |
| 4 | logicalDeviceId |
| 5 | ownershipGeneration |
| 6 | grantDigest = SHA-256(exact encoded grant) |

### 3.4 OwnershipClaimCommitRequest（最大 183 B）

| key | 字段 |
|---:|---|
| 0 | contractVersion = 1 |
| 1 | grantId |
| 2 | requestId |
| 3 | deviceInstanceId |
| 4 | logicalDeviceId |
| 5 | ownershipGeneration |
| 6 | receiptDigest = SHA-256(exact encoded receipt) |
| 7 | commitIdempotencyKey，16 B 非零 |

### 3.5 OwnershipClaimCommitAck（最大 250 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | grantId | 16 B，非零 |
| 2 | requestId | 16 B，非零 |
| 3 | deviceInstanceId | 16 B，非零 |
| 4 | logicalDeviceId | 1..64 B UTF-8 |
| 5 | ownershipGeneration | uint32，非零 |
| 6 | receiptDigest | 32 B，`SHA-256(exact encoded receipt)` |
| 7 | committedAt | uint64，非零 |
| 8 | serverKeyId | uint32，非零 |
| 9 | signatureAlgorithm | 1 Ed25519；2 ECDSA-P256-SHA256 raw `r||s` |
| 10 | signature | 固定 64 B |

签名输入为下面 canonical array 的 SHA-256；最大值不超过共享 256 B workspace：

~~~text
[
  "blinker.ownership-claim-commit-ack.v1", 1,
  grantId, requestId, deviceInstanceId, logicalDeviceId,
  ownershipGeneration, receiptDigest, committedAt,
  serverKeyId, signatureAlgorithm
]
~~~

## 4. Generation 与持久化不变量

- fresh storage 只接受 generation 1；不能从 1 跳到任意更大值；
- authorized reset/transfer 将 Active N 原子变为 Unclaimed tombstone N+1；禁止跳代，新 claim 必须与 tombstone exact 相等；
- full-erase recovery 只有在独立 signed RecoveryGrant 验证并写 recovery journal 后，才允许 fresh store 安装
  Grant 指定的 N+1 tombstone；普通 Claim/Enrollment 绝不能自行选择高 generation。详细时序见
  `DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md`；
- Active record 只允许同一 logical ID、generation、fingerprint 的 exact replay；
- 普通 Network Reset 不改 ownership；全片维修擦除必须走显式 `eraseAll()`，不能伪装成用户解绑；
- `BOS2` 是设备本地 112 B Active/tombstone record，`BOC2` 是 48 B Claim journal；二者都不是 App/Gateway wire 或数据库导入格式。

## 5. 资源与实现

- `OwnershipClaimContract`：纯 restricted-CBOR codec，无 transport/Arduino/heap 依赖；
- `OwnershipClaimGrantVerifier`：通过注入的 `IServerSignatureVerifier` 验 Grant 与 CommitAck；
- `OwnershipClaimCoordinator`：依赖 `IOwnershipRecordStore + IOwnershipClaimRecordStore + IRandom`，不依赖 HTTP/BLE；
- `BOC2` 固定 48 B；32 位 record/store/coordinator 门禁分别为 36/8/20 B；
- operation workspace 固定 256 B，由调用方在验签时共享；
- `OwnershipRecordStore` 继续为 8 B adapter object，记录 104 B、序列化 112 B。

共享向量 `tests/golden/ownership_claim_golden_vectors.properties` 现锁定 C++/Node 11 个 exact 值与 App
7 个 wire 值，包含 CommitAck transcript/digest/body；生产 decoder 仍需补齐 shared invalid corpus。设备 Host
已锁定 250 B 最大编码，并覆盖错误签名不得清 journal、
Grant 前重启、Active 后 exact request/grant/receipt 重放与 signed CommitAck 后清理。

## 6. 安全声明

server-signed Grant 防止未授权方伪造设备 ownership 变更；server-signed CommitAck 防止 Plain HTTP 上的伪造
2xx 诱导设备提前清 journal。无密钥 receipt 仍不能独立证明发送者身份；发送者认证来自 WiFi profile
ManualDeviceKey proof、Noise setup session 或 App/Gateway 登录态。Education Plain 默认允许明文 HTTP，以
换取更低资源和更简单接入；raw ManualDeviceKey 不发送，但其长期替换授权仍只能声明受限安全等级，不能
宣称防阻断、流量分析或硬件真实性。TLS 是显式可选档，不能静默降级。任何 raw ManualDeviceKey、Ticket、Cloud/Controller secret 均不得
进入 canonical ownership body、日志或 analytics；后续长期 Cloud secret 即使在 Plain transport 下也必须
使用独立端到端保护或不传 raw secret 的 suite。

Cloud Credential Enrollment 与 Ownership Reset/Transfer 使用各自独立合同、journal 和 coordinator，不复用 Claim grant。BLE-only 可跳过 Cloud，直接在 Active ownership 下进入 Controller Enrollment；WiFi-only outer 见 `DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md`。
