# Device V2 Cloud Credential Enrollment Contract v1

> **已退役（2026-08-23）：**设备实现、测试和工具已删除。WiFi 使用 DeviceKey session；纯 BLE 使用 `DEVICE_V2_BLE_ENROLLMENT_CONTRACT.md`。本文只保留历史决策，不得作为实施输入。

> 状态（2026-08-19）：本合同是保留的软件实验，不是 Education 默认登录图。设备侧平台无关 exact contract、sealed-envelope、grant verifier、掉电 journal、coordinator 与 C++/TypeScript/Node golden 已冻结；旧 `EnrollmentKey` outer 已删除，公开 WiFi 只使用 DeviceKey challenge/session。ESP32 native 与 UNO R4/Wio portable X25519/AES-GCM provider 已通过 SDK 链接和固定向量编译；若未来启用本合同，仍需独立 Product、App/Gateway 生产端点、真机向量和 E2E。

## 1. 目标与边界

Cloud Credential Enrollment 只完成：

~~~text
Active Ownership(logicalDeviceId, generation)
  -> Active CloudCredential(HMAC-SHA256, version, 32 B secret)
~~~

它不创建 ownership，不消费或解释 ManualDeviceKey/EnrollmentTicket，不写 WiFi，不安装 Direct BLE controller，也不签发 MQTT 凭据。WiFi-only、WiFi+BLE 与 App relay 共用完全相同的 canonical body；profile driver 只负责外围 authorization、transport 与重试调度。

BLE-only/LocalBLE 可以不进入本合同。正常重启只使用长期 `CloudCredential` 请求短期 `SessionCredential`，绝不重新使用已经消费或过期的首次授权。

## 2. 为什么不是旧 Activation

旧 Enrollment/1 `ActivationRequest` 直接携带 32 B `operationalSecret`，仅在 TLS/受保护 relay 下才不泄露。当前教育档允许 HTTP/MQTT 默认 Plain，因此不能把旧类改名后继续使用。

v1 使用设备生成 secret 和服务器静态 X25519 公钥形成 sealed envelope：

- raw secret 不出现在 HTTP/BLE canonical body、日志或 analytics；
- Plain transport 下被动窃听者和普通 MITM 不能取得长期 secret；
- TLS 仍可显式开启，作为 metadata、authorization 和流量的额外保护，但不是该 secret 保密性的唯一来源；
- 服务器 HMAC suite 必须保存 KMS/HSM 加密的等价 secret，普通 password hash 不能验证后续 challenge HMAC；
- 如果未来要求服务器只保存公开材料，应新增设备签名 suite，不能把 HMAC key 错称为 verifier。

## 3. 四步事务与掉电恢复

~~~mermaid
sequenceDiagram
    participant D as Device
    participant A as App/Profile Relay
    participant G as Gateway
    D->>D: stage Prepared journal<br/>requestId + eph private + cloud secret
    D->>A: exact sealed EnrollmentRequest
    A->>G: exact relay + outer authorization
    G->>G: decrypt envelope, reserve Pending credential
    G-->>A: server-signed Grant
    A-->>D: exact Grant
    D->>D: persist grantDigest, then install CloudCredential
    D->>A: HMAC Receipt (proof of possession)
    A->>G: exact relay
    G->>G: verify HMAC, mark credential Active
    G-->>A: HMAC CommitAck
    A-->>D: exact CommitAck
    D->>D: verify HMAC, clear transaction journal
~~~

顺序不可交换：

1. `prepare()` 使用 CSPRNG 生成非零 16 B `requestId`、32 B ephemeral private key 和 32 B cloud secret，先原子保存 `Prepared` journal。
2. `request()` 每次从同一 journal 重建完全相同的 ephemeral public key、nonce、ciphertext 和 request bytes；重启不生成第二个 credential。
3. Gateway 以 `(logicalDeviceId, ownershipGeneration, requestId)` 建幂等事务。相同 request digest 返回首次保存的 exact Grant；相同 ID 不同 digest 返回 conflict。
4. 设备验证 request/device/logical ID/generation/version/requestDigest/time（有可信时钟时）和 server signature。
5. 设备先把 `SHA-256(exact encoded Grant)` 原子写入 journal，使其变成 `GrantAccepted`，再 `installFresh()` Active `CloudCredential`，最后发送 HMAC Receipt。
6. Gateway 只有验证新 cloud secret 的 Receipt HMAC 后才把 credential 标成 Active，再返回 HMAC CommitAck。
7. 设备只有验证 CommitAck HMAC 和 exact receipt digest 后才清除 journal。

| 掉电点 | 恢复行为 |
|---|---|
| stage 前 | 仍为 Ready，可创建新事务 |
| Prepared 后、请求前/响应丢失 | 重建并重发 exact request |
| Gateway 已保存 Pending、设备未收到 Grant | Gateway 返回首次 exact Grant |
| grantDigest 已保存、CloudCredential 未安装 | boot 时从 journal 恢复并 `installFresh()` |
| CloudCredential 已安装、Receipt/ACK 丢失 | 从 journal 重建 exact Receipt；不重签、不检查已接受 Grant 的新 TTL |
| Gateway Active、设备未收到 CommitAck | Gateway 对 exact Receipt 返回 exact/等价合法 CommitAck |
| CommitAck 验证并清 journal 后 | 只走日常 Cloud Session，不再进入 enrollment |

`restartPrepared()` 只允许删除尚未接受 Grant 的 Prepared 事务，例如服务器明确返回旧 encryption key 已撤销或请求已过期。`GrantAccepted` 不能本地取消并换 secret，否则会制造服务器与设备分叉。

## 4. Sealed envelope 算法

首版固定为 `X25519Aes256GcmSha256 = 1`：

1. 设备从持久 journal 的 32 B ephemeral private key 计算 32 B X25519 public key。
2. 构造第 5.1 节 exact envelope transcript，计算 `transcriptDigest = SHA-256(transcript)`。
3. `nonce = transcriptDigest[0..11]`。由于每个事务使用新的 CSPRNG ephemeral private key/requestId，派生 AES key 对每个事务唯一；同一事务重试必须复现同一 nonce/ciphertext。
4. `shared = X25519(ephemeralPrivate, pinnedServerPublicKey)`。
5. `salt = SHA-256("blinker.cloud-credential-envelope.hkdf-salt.v1")`。
6. `aesKey = HKDF-SHA256(salt, shared, transcriptDigest, 32)`，语义与 RFC 5869 一致。
7. `AES-256-GCM(aesKey, nonce, AAD = exact envelope transcript, plaintext = 32 B cloudSecret)`。
8. wire 的 `encryptedCredential` 是 `32 B ciphertext || 16 B tag`。

服务器 public key 必须来自固件/受保护配置中的 pinned rotation key ring，不能从当前未认证 HTTP 响应临时接受。持久请求引用 `encryptionKeyId`，所以轮换时必须保留 overlap private/public key，直到最长 enrollment TTL 与离线重试窗口结束。服务器 private key 应位于 KMS/HSM 或等价受控密钥域。

`IX25519AesGcmCryptoProvider` 是通用 MCU primitive seam；`INoiseCryptoProvider` 继承它，因此 ESP32 现有 Noise adapter 可复用，但 Cloud contract 不依赖 BLE/Noise session。没有 CSPRNG 或没有真实 X25519/AES-GCM 的平台必须返回 `NotConfigured/UnsupportedFeature`，不能退化成 XOR、固定 key 或 raw secret。

## 5. Exact canonical CBOR

共同规则：definite-length map/array、升序 unsigned numeric key、最短整数/长度编码、拒绝 unknown/duplicate/out-of-order/trailing data；ID/digest/key/ciphertext/signature/proof 为 bstr，logical ID 为 1..64 B UTF-8 text。

### 5.1 Envelope transcript

~~~text
[
  "blinker.cloud-credential-envelope.v1", 1,
  requestId, deviceInstanceId, logicalDeviceId,
  ownershipGeneration, credentialVersion, credentialSuite,
  envelopeAlgorithm, encryptionKeyId, ephemeralPublicKey
]
~~~

nonce 与 ciphertext 不进入该 transcript；nonce 由 transcript digest 派生，ciphertext 以 transcript 作 GCM AAD。

### 5.2 EnrollmentRequest（最大 228 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | requestId | 16 B 非零 |
| 2 | deviceInstanceId | 16 B 非零 |
| 3 | logicalDeviceId | 1..64 B UTF-8 |
| 4 | ownershipGeneration | uint32 非零 |
| 5 | credentialVersion | 首次为 `1`；uint32 非零 |
| 6 | credentialSuite | `1 = HmacSha256_32` |
| 7 | envelopeAlgorithm | `1 = X25519Aes256GcmSha256` |
| 8 | encryptionKeyId | uint32 非零 |
| 9 | ephemeralPublicKey | 32 B X25519 public key |
| 10 | nonce | 12 B |
| 11 | encryptedCredential | 48 B ciphertext + tag |

ManualDeviceKey/Ticket、user JWT、WiFi、productId、manifest、tenant/owner 和 raw cloud secret 均不得加入该 map。它们属于外围认证/业务事务。

### 5.3 server-signed Grant（最大 248 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | requestId | 16 B |
| 2 | deviceInstanceId | 16 B |
| 3 | logicalDeviceId | 1..64 B UTF-8 |
| 4 | ownershipGeneration | uint32 非零 |
| 5 | credentialVersion | uint32 非零 |
| 6 | requestDigest | SHA-256(exact EnrollmentRequest) |
| 7 | issuedAt | uint64 非零 |
| 8 | expiresAt | uint64，严格大于 issuedAt |
| 9 | serverKeyId | signing key ID |
| 10 | signatureAlgorithm | 1 Ed25519；2 ECDSA-P256-SHA256 raw `r||s` |
| 11 | signature | 固定 64 B |

签名输入是下列 canonical array 的 SHA-256：

~~~text
[
  "blinker.cloud-credential-enrollment-grant.v1", 1,
  requestId, deviceInstanceId, logicalDeviceId,
  ownershipGeneration, credentialVersion, requestDigest,
  issuedAt, expiresAt, serverKeyId, signatureAlgorithm
]
~~~

### 5.4 device HMAC Receipt（最大 190 B）

| key | 字段 |
|---:|---|
| 0 | contractVersion = 1 |
| 1 | requestId |
| 2 | deviceInstanceId |
| 3 | logicalDeviceId |
| 4 | ownershipGeneration |
| 5 | credentialVersion |
| 6 | grantDigest = SHA-256(exact Grant) |
| 7 | proofAlgorithm = 1 (HMAC-SHA256) |
| 8 | proof = 32 B |

proof 为 `HMAC-SHA256(cloudSecret, receiptTranscript)`：

~~~text
[
  "blinker.cloud-credential-enrollment-receipt.v1", 1,
  requestId, deviceInstanceId, logicalDeviceId,
  ownershipGeneration, credentialVersion, grantDigest, proofAlgorithm
]
~~~

### 5.5 server HMAC CommitAck（最大 190 B）

结构与 Receipt 相同，但 key 6 为 `receiptDigest = SHA-256(exact Receipt)`，proof transcript domain 为：

~~~text
"blinker.cloud-credential-enrollment-commit-ack.v1"
~~~

CommitAck 不能只依赖 Plain HTTP 2xx；HMAC 证明响应方实际持有刚注册的 cloud secret，防止伪造 ACK 使设备过早删除 journal。

## 6. 错误与重试

wire error 使用 3000 段，设备 strict decoder 已冻结：

- malformed/unsupported/auth/envelope/grant/proof invalid：`Never`；
- request/credential conflict：`SameRequest`，不得偷偷换 secret；
- encryption key not found、grant/request expired：`RestartEnrollment`，仅 Prepared 可清理重建；
- ownership inactive/generation conflict/device revoked：`ReclaimOwnership`；
- rate limit：`Backoff` 且必须有非零 retry-after；
- temporarily unavailable：`SameRequest` 或 `Backoff`。

HTTP 映射为 400/401/403/404/409/429/503。HTTP driver 尚未实现；实现时必须同时校验 HTTP status 与 exact error body，不能把任意 4xx 都映射成重新 Claim。

## 7. 持久化与资源

| 对象 | 当前 32 位 ABI/固定记录 | 说明 |
|---|---:|---|
| `CloudEnrollmentRecord` | 128 B RAM | 仅操作栈/测试；coordinator 不常驻缓存 |
| `CloudEnrollmentRecordStore` | 8 B | vptr + storage 引用 |
| `BCE2` journal | 136 B | magic/schema/state/generation/version/keyId/request/private/secret/grantDigest/CRC32 |
| `CloudCredentialEnvelopeSealer` | 8 B | crypto + key source 引用 |
| `CloudCredentialEnrollmentCoordinator` | 32 B | 七个依赖引用 + state；不持有 secret/buffer |
| operation workspace | 256 B caller-owned | request/grant/proof transcript 共用 |

UNO R4 Small `SizeProbe` 实测 52,148 B Flash / 11,696 B 全局 RAM；强制保留新增对象比上一批增加 176 B RAM。普通 `CompileV2` 没有实例化 enrollment，UNO R4 仍为 69,296/7,664 B，Wio Terminal 仍为 48,236 B；ESP32 为 320,967/21,672 B。以上均为编译/ABI 门禁，不是真机 crypto heap、栈峰值或时延结论。

`BCE2` 必须位于独立原子 blob。教育档可显式接受 PlainFlash 的物理提取风险；增强/量产档应使用加密存储或 secure element。网络 sealed envelope 不等于 at-rest protection。

## 8. 三端执行要求

设备：

- profile driver 在 Active ownership 且 CloudCredential absent 时调用 coordinator；
- WiFi-only 以 ManualDeviceKey authorization proof 直接 HTTP；WiFi+BLE/App relay 在已认证 setup session 中传 exact bytes；
- 不把 server public key、HTTP、BLE 或 Arduino SDK 放入 contract/coordinator；
- ESP32 使用 native mbedTLS provider；UNO R4/Wio 可使用 portable provider。两者都必须通过真机向量、
  stack/heap 与组合资源门禁后才能进入 official support matrix。

App：

- 只 relay exact request/grant/receipt/ack；不得 decode 后 JSON 重编码；
- 不请求、读取、展示、缓存、备份或打印 cloud secret、ephemeral private key、服务器 encryption private key；
- UI 分开显示 Ownership Active、Cloud Credential Pending/Active 与 MQTT Online；Grant 到达不等于 cloud online。

Gateway：

- outer authorization 在进入 common decoder 前校验 ManualDeviceKey/Ticket/user/factory intent；
- X25519 server private key 与 Active HMAC secret 使用 KMS/HSM/等价 envelope encryption；普通 DB/log/trace 不得出现 raw secret；
- 保存 exact request/grant 或足以返回 exact bytes 的不可变事务记录；严格执行 generation/idempotency CAS；
- Receipt HMAC 成功后才 Active；CommitAck HMAC 必须基于当前事务 secret 与 exact receipt digest；
- Cloud Session 只接受 Active cloud credential/current version，不接受 ManualDeviceKey/Ticket。

## 9. 验证与剩余项

共享向量：`tests/golden/cloud_credential_enrollment_golden_vectors.properties`。

- C++：15 个 exact 值 + strict decode/HMAC；
- App TypeScript：5 个 exact relay wire 值；
- Node：真实 X25519/AES-256-GCM/HKDF、19 个 exact 值；
- Host：prepare/exact retry、Grant 验签、安装顺序、Receipt/ACK HMAC、伪造 ACK、掉电恢复、过期/restart、最大编码与 RFC 5869 vector；
- 全量 Host 31/31、20,000 次 fuzz、既有 Noise golden、ESP32/UNO R4/Wio core、ESP32 native 与
  UNO R4/Wio portable crypto compile/vector probes 已通过。

仍未完成：official composition 对 flow/provider 的产品接线、Ticket authorization、BLE relay endpoint
wiring、服务器/App 生产代码、真机掉电/攻击/heap/stack/时延/长稳。Ownership Reset/Transfer 已由独立
Release 合同完成，旧 `OperationalCredential/ProvisioningCoordinator/HttpEnrollmentDriver` 整组已删除。
