# Device V2 Enrollment / Ownership / Credential Wire Contract（已撤回）

> 2026-08-16 最终状态：旧 Enrollment/1 codec、coordinator、driver 与 golden 已删除。本文件仅保留
> 历史审计信息；其中 Ownership Claim、Cloud Credential Enrollment 和 Ownership Release 随后也已退役。
> 当前纯 BLE 首次接入只使用 `DEVICE_V2_BLE_ENROLLMENT_CONTRACT.md`，禁止恢复聚合协议。

> 2026-08-17 注：下文历史 `bootstrapKind=RecoveryGrant` 行同样已撤回，不能作为当前 wire。全片 DIY 恢复
> 现由独立 `DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md` 定义；Ownership Transfer 仍由 Release 合同定义，二者
> 不能再合并成一个 scope。

> 2026-08-16 更新：Ownership Claim 已由 `DEVICE_V2_OWNERSHIP_CLAIM_CONTRACT.md` 冻结；Cloud
> Credential Enrollment 已由 `DEVICE_V2_CLOUD_CREDENTIAL_ENROLLMENT_CONTRACT.md` 冻结。本文件第 4 节
> 中仍包含的 `bootstrapAuthorization/manifestFingerprint/owner/tenant/ClaimCommitProof` 与 raw-secret
> Cloud Activation 是冻结前讨论稿，不再约束设备/App/Gateway。Bootstrap authorization 留在
> profile-specific 认证信封；Ownership Reset/Transfer 已由正式 Release 合同独立冻结。

> 状态：2026-08-15，未冻结、不得用于生产兼容承诺。旧 Enrollment/1 把 Developer token、factory proof、
> ownership 和 cloud activation 绑在一个状态机中，已经撤回。checked-in codec/golden 仅证明实验实现，
> 不能约束正式 App/Gateway。正式数字 key、path、签名 transcript 和 wire major 必须按本文与
> `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 重新生成。

## 1. 合同拆分

正式控制面拆成四个相互独立、可组合的合同：

| 合同 | 适用设备 | 结果 |
|---|---|---|
| Bootstrap Authorization | 按 profile 选择 | 证明本次请求可开始认领，不创建设备运行凭据 |
| Ownership Claim | 所有平台设备 | `logicalDeviceId + ownershipGeneration + OwnershipReceipt` |
| Cloud Credential Enrollment | WiFi/cloud-capable | 持久 `CloudCredential` 激活记录 |
| Controller Enrollment | Direct BLE-capable | 当前 App 独立 `ControllerCredential` 激活记录 |

一次 App 向导可以连续完成多个合同，但不得用一个 `ActivationResponse` 同时表示 ownership、cloud 和
controller 已成功。BLE-only 不创建虚假 MQTT credential；WiFi-only 不创建虚假 controller。

## 2. Bootstrap Authorization union

ClaimStart 必须显式声明一种 `bootstrapKind`，不得根据字段是否存在猜测：

| bootstrapKind | 授权材料 | 使用场景 | 服务器/设备验证 |
|---|---|---|---|
| `EnrollmentKey` | 一次性高熵 key | 无 App 本地通道的 WiFi-only DIY | Gateway 校验 key hash、intent、TTL/quota |
| `EnrollmentTicket` | 短期、设备可验证的 server-signed ticket | WiFi+BLE、BLE-only App 配网 | 绑定 owner、physical ID、setup session、双方临时 key digest、controllerId、controller secret digest、设备权限位、TTL |
| `FactoryProof` | challenge proof | Manufactured | Gateway 按 factory registry 验证每台唯一 bootstrap credential |
| `RecoveryGrant`（历史撤回） | server-signed grant | 曾错误合并 ownership 恢复/转让 | 不得实现；恢复用 Device Recovery，转让用 Ownership Release |

`SetupCode` 不再是所有 Developer profile 的公共字段。量产/加强档可以在 Ticket 之外使用 QR claim/setup
code，但它属于 possession proof 或本地会话认证，不能复用为 CloudCredential。

## 3. 共同编码与安全规则

正式 wire 继续使用受限 canonical CBOR，但数字 key 在新 golden 完成前不冻结：

1. definite-length map/array、升序 unsigned integer key、最短编码；禁止 unknown、duplicate、乱序、tag、
   indefinite、float 和用 null 代替缺省。
2. 每个事务携带 contract major、16 B 非零 request/idempotency key 和明确 bootstrap/credential suite。
3. `physicalDeviceId`、`logicalDeviceId` 是公开文本 ID；所有 nonce、ticket、secret 和 proof 使用 bstr，
   不在 JSON/base64/hex 间来回转换。
4. exact signed object 通过 BLE relay 时必须作为原始 bstr 搬运，App 不得解码后重建或改写为 boolean。
5. body、Ticket、EnrollmentKey、WiFi、CloudCredential、ControllerCredential 不进入 access log、APM、
   analytics、崩溃回显或自由文本错误。
6. Education Plain 可显式允许一次性 Enrollment bearer 走明文并接受窃听/抢占风险；raw 长期 secret 即使在
   Plain transport 下也只允许出现在独立端到端保护中，或改用不传 raw secret 的 suite。TLS 是可选档，不得静默降级。
7. MAC、广播名、deviceName、productId 不进入 proof 主键。setupSessionId 只绑定当前无线窗口，不是身份。

## 4. Ownership Claim 语义

### 4.1 ClaimStart

语义字段：

~~~text
contractMajor
bootstrapKind
bootstrapAuthorization       // EnrollmentKey/Ticket/Factory/Recovery union
physicalDeviceId
requesterIdempotencyKey
setupSessionId?              // BLE setup 时必填
manifestFingerprint
capabilitySummary
deviceProofPublicMaterial?   // 仅选定 suite 需要
~~~

- `productId/productModelId` 不得由设备提交。Gateway 从 EnrollmentKey/Ticket intent 或 factory registry
  取得 model/project policy。
- 相同 owner、physicalDeviceId 和 requester key 必须返回同一 claim transaction；同一个 requester key
  换 physical ID、bootstrap kind 或 manifest digest 必须冲突。
- EnrollmentKey/Ticket 的“一次性”锁定一个 claim transaction，而不是让丢包后的 exact retry 失败。

### 4.2 Challenge 与 device proof

Gateway 返回短期 challenge，至少绑定：

~~~text
claimId
physicalDeviceId
ownershipGeneration
serverNonce
setupSessionId?
bootstrapKind
manifestFingerprint
expiresAt
sealedClaimTicket
~~~

设备 proof 必须覆盖上述 canonical transcript。不同 bootstrapKind 的 proof 可以不同：

- EnrollmentKey：受保护通道内以域隔离 KDF/HMAC 或等价机制证明持有 key；
- EnrollmentTicket：设备验证 server signature/scope，并证明持有当前实例/临时或持久设备 key；教育档若无
  factory root，必须明确其 proof 只证明当前 setup instance，不宣称硬件真实性；
- FactoryProof：`DeviceBootstrapCredential` 对 transcript 签名/HMAC，Gateway 使用 registry verifier；
- BLE App relay：只搬运 exact challenge/proof bytes，不能生成 device proof。

### 4.3 OwnershipReceipt 与 ClaimCommit

Gateway 验证 proof 后预留 logicalDeviceId，并返回 server-signed OwnershipReceipt：

~~~text
claimId
physicalDeviceId
logicalDeviceId
owner/tenant binding
ownershipGeneration
manifest/capability policy digest
bootstrapKind/securityLevel
issuedAt/expiresAt
serverKeyId/signatureAlgorithm/signature
~~~

设备必须匹配当前 claim/setupSession/generation、验证 server signature，并先原子持久化 receipt，再发送
ClaimCommitProof。Gateway 收到 commit 才把 Ownership 标记 Active，并原子消费 EnrollmentKey/Ticket。
相同 exact proof/commit 返回首次保存的相同 receipt/result，不重新取时间或创建另一 logical device。

## 5. Cloud Credential Enrollment

CloudCredential 是认领后的长期设备认证根，不是 EnrollmentTicket。首版优先由设备 CSPRNG 生成并在请求前
以 Pending 状态原子保存：

~~~text
logicalDeviceId
ownershipGeneration
credentialRequestId
credentialSuite
verifier/publicKey-or-protected-secret
deviceProof
~~~

规则：

1. Gateway 只接受 Active ownership/current generation；同 request 必须注册同一验证材料。
2. 对称 raw secret 若必须首次提交，即使 HTTP/MQTT 选择 Plain 也要经独立端到端保护；Gateway 立即转换为受保护 verifier/material 并
   禁止日志；优先采用无需传输 raw secret 的 public-key 或经审计 challenge 方案。
3. Gateway 返回 `logicalDeviceId + generation + credentialVersion + policy`。logicalDeviceId 已由 Ownership
   Claim 产生，Cloud enrollment 不得再次创建它。
4. 设备先保存 PendingCommit，再以 CloudCredential 对 exact response 做 commit proof；双方成功后才 Active。
5. Active 记录删除 enrollment-only Ticket/Key/claim payload，只保留日常登录所需 ID、generation、version、
   credential 和控制面配置。
6. 正常重启、长期断网、Network Reset 和 SessionCredential 过期都使用 Active CloudCredential；只有
   ownership reset、服务器撤销、凭据损坏或全片擦除才重新 enrollment。

### 5.1 SessionCredential

日常 cloud-session 合同独立于本文件，输入为 Active
`logicalDeviceId + ownershipGeneration + credentialVersion + challenge proof`，输出短期 MQTT/WebSocket
SessionCredential。请求不携带 physicalDeviceId、EnrollmentKey 或 EnrollmentTicket。SessionCredential
只放 RAM；设备每次重启重新申请。

## 6. Controller Enrollment

Direct BLE-capable 设备在 Active ownership 下安装每个 App 独立的 controller：

~~~text
logicalDeviceId
ownershipGeneration
controllerId
controllerId                  // 16 B CSPRNG；全局不复用
controllerSecretDigest        // SHA-256(App 生成的 32 B HMAC secret)
credentialVersion
permissions                  // device permission bitmask，不传 role 字符串
requestId
serverAuthorization
deviceCommitProof
~~~

首个可移植套件固定为 HMAC-SHA-256。App 在 OS Keychain/Keystore 保存 raw secret，设备在独立
`ControllerCredentialStore` 的 `BCT2` 表中保存同一 secret；Gateway 只保存 digest、权限、代次、版本和状态，
不得保存 raw secret。`BCT2` 是设备内部 4 槽原子持久格式，不是三端 wire。controllerId 必须由 CSPRNG 生成且
永久不复用；撤销后的旧 install/rotate 请求还必须由本合同的 request freshness、版本 CAS 和 ownership
generation 拒绝，不能把 CRC 或“表中已删除”误当成防重放授权。

- App private key/secret 只存 OS Keychain/Keystore；设备保存有界 controller record；服务器保存 public
  key/verifier、role、generation 和状态。
- Setup Ticket、EnrollmentKey、CloudCredential 和全设备共享 `localAccessKey` 均不得作为日常 BLE 密码。
- Controller revoke 只删除指定 controller；Ownership generation 前进使全部旧 controller 失效。
- BLE-only 可只完成 Ownership + Controller，服务器不得因缺少 CloudCredential 把设备标为异常。

## 7. BLE 本地安全 envelope

Provisioning 与 Direct BLE 复用 secure-record primitive，但 mode、密钥和生命周期分开。local envelope 至少
包含 contract major、message type、requestId 和 exact body bstr。建议消息族：

| 方向 | 消息 |
|---|---|
| App → Device | EnrollmentTicket / ClaimChallenge / OwnershipReceipt / WiFiCandidate / ControllerGrant |
| Device → App | DeviceSetupInfo / ClaimProof / ReceiptAccepted / WiFiTestResult / CommitProof |
| 双向 | StableError / RetryStatus |

WiFi candidate 使用 SSID bytes、auth mode、credential bytes、hidden flag；平台不支持的 auth mode 必须明确
拒绝，不能降级。App/设备先用临时 ECDH/Noise 建 provisional encrypted channel；设备只有在 Ticket/grant
与当前 physicalDeviceId、setupSessionId、双方临时 key、controllerId、controller secret digest 和设备
权限位匹配后才把会话升级为
authenticated 并打开 WiFi sink。无预置 device root 的 Education 档必须在文档/UI 标明无法完全证明硬件
真实性；
Manufactured/Enhanced 通过 factory proof 或 QR secret 增强。

## 8. NetworkProvisioningGrant

已绑定设备仅清除/更换 WiFi 时，不得重新使用已经消费的 EnrollmentTicket。当前 owner/controller 请求短期、
server-signed `NetworkProvisioningGrant`，至少绑定：

~~~text
physicalDeviceId
logicalDeviceId
ownershipGeneration
setupSessionId
controllerId
scope = NetworkProvisioning
nonce
issuedAt/expiresAt
server signature
~~~

设备验证 grant 和物理 provisioning window 后接受新 WiFi candidate。该事务不创建 claim、不改变 owner、
不轮换 logicalDeviceId，CloudCredential 默认保留。

## 9. Ownership Reset / Transfer Grant

Reset/transfer grant 必须绑定 physical/logical ID、当前与下一 generation、本次设备 nonce、owner/tenant、
scope、过期时间和 server signature。设备通过 generation CAS 写 reset journal，再清 Ownership、Cloud、
Controller 和按策略清 Network；Factory/identity 域保留。仅按键或擦除本地配置不能单方面改变服务器 owner。

Transfer commit 后旧 logicalDeviceId 默认进入 Released/Archived；新 owner 的 Claim 创建新 logicalDeviceId。
physicalDeviceId 保持不变且只能关联一个 Active ownership。页面/历史/ACL 的跨 owner 迁移不属于本 wire，
必须由独立、双方授权的数据迁移合同处理。

## 10. 事务恢复规则

| 场景 | 规定响应 |
|---|---|
| 同 request + 同 canonical body | 返回同一 transaction/receipt/result |
| 同 request + 不同 body/physical ID | Conflict + 审计 |
| Ticket 已锁定且 claim 进行中 | 允许同一 claim 在 recovery TTL 内完成 |
| Ticket 已消费且 ownership Active | 不再用于登录；返回 AlreadyActive/现有公开状态 |
| credential stage 后断电 | 用同一 credential/request 继续，禁止重新生成 |
| commit 响应丢失 | exact commit 幂等成功 |
| Active 后重启 | 跳过 enrollment，进入 cloud session/controller runtime |
| Network Reset | 使用 NetworkProvisioningGrant，不创建 claim |
| Ownership generation 已改变 | 旧 Ticket/grant/controller/session 全部拒绝 |

服务器错误必须使用稳定 numeric code + retry class；自由文本只供 UI，不能驱动状态机。需要至少区分
Never、SameRequest、Backoff、RestartBootstrap、UserAction。

## 11. 旧实验 wire 的处理

以下内容不进入正式 major，也不提供未发布兼容 decoder：

- ClaimStart 固定 `developerEnrollmentToken` 字段；
- DeveloperSoftware 必须 P-256/Ed25519 且 physical ID 只能由公钥派生；
- ClaimProofVerificationReceipt 成功后直接进入包含 logicalDeviceId 的 Activation；
- Activation 同时创建 ownership 与 CloudCredential；
- BLE 必须从统一 SetupCode 派生 `Ksetup`；
- App 用 `proof_verified=true` 或本地 `bound=true` 表示服务器成功；
- productId、MAC、广播名进入 claim 身份。

旧 `enrollment_golden_vectors.properties` 与三端 reference tests 已删除。Ownership Claim、Controller
Control、Cloud Credential Enrollment 与 Ownership Release 均已有独立正式合同；App/Gateway 生产实现
不得引用本旧合同。

## 12. 尚未完成的正式产物

1. WiFi-only EnrollmentKey、WiFi+BLE Ticket、BLE-only relay、Network Reset、Ownership Transfer 的外围
   sequence 与掉电恢复矩阵；
2. 每个 bootstrapKind 的 exact canonical transcript、signature/HMAC suite 和 security-level 声明；
3. Ownership Reset/Transfer 的 request/grant/receipt/commit exact CBOR、invalid vectors 与原子跨域擦除合同；
4. Ticket 到期前后、claim 已锁定、Active 后重启的 Gateway/App/device 集成测试；
5. server key rotation、controller revoke、generation CAS、日志脱敏、限流和 replay 测试；
6. ESP32、UNO R4/Renesas 的 CSPRNG、原子存储、server verify、BLE secure record 和资源真机门禁。
