# Device V2 Device RecoveryGrant Contract v1

> 状态（2026-08-17）：已按教育/手动模式可用性重新修订。上一版短期、只显示一次的 `RecoveryKey/brk_`
> 已撤回；手动模式统一使用长期可查看/复制/刷新的 `ManualDeviceKey/bdk_` 作为外围授权。独立 signed
> RecoveryGrant、N -> N+1、掉电恢复与 Claim cutover 语义保留。设备 codec/coordinator、shared golden、
> App/Gateway 生产实现尚未完成。

## 1. 合同边界

Recovery 处理“App 中的 logical device/page 仍存在，但 DIY 设备因擦除全部 Flash 丢失 identity、Ownership
和 CloudCredential”的情况：

- 用户继续使用设备页长期显示的同一个 ManualDeviceKey，不再切换到另一种 RecoveryKey；
- 新板端实例生成新的 random deviceInstanceId；服务器不能按 MAC 猜测；
- Gateway 必须签发 exact RecoveryGrant，授权旧 logical device 从 generation N 前进到 N+1；
- 新实例建立 N+1 recovery base 后复用现有 Ownership Claim 和 Cloud/Controller enrollment；
- 最终 Claim commit 原子切换 physical relationship，旧 credential/session 才撤销；
- logicalDeviceId、PageSchema、历史、ACL、自动化与兼容 binding 保留。

ManualDeviceKey 的格式、长期 reveal、HMAC proof、刷新和在线实例保护见
`DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md`。它只是 profile-specific authorization outer，不进入本合同的
canonical body，也不能替代 server signature。

## 2. 不适用情况

- 只丢 WiFi：走 NetworkProvisioningGrant；
- Active Ownership 尚在但 CloudCredential 单独损坏：返回 LocalStateCorrupt，不能自动扩大为全设备替换；
- Ownership Reset/Transfer：由当前 Active 实例执行 Release 合同；
- LocalBLE：没有服务器 logical device，擦除后重新本地配对；
- Manufactured：优先用 factory identity/bootstrap proof 或售后 repair；
- 账号丢失：走账号/售后流程，物理长按和 Device Key 不能绕过 owner 权限；
- 删除页面/历史：独立 destructive delete，不是 Recovery。

普通 DIY 无不可擦除硬件根，准确表述是“owner 允许新软件实例接管旧 logical device”，不是证明它仍是原
硬件。更高安全要求选择 Enhanced/Manufactured profile。

## 3. 启动判定

设备先读 durable storage，再处理 Sketch 参数：

| 本地状态 | 行为 |
|---|---|
| Active Ownership + Active Cloud | 忽略 ManualDeviceKey，正常上线 |
| storage 全空 + ManualDeviceKey | Gateway 按 logical device 状态选择 InitialClaim 或 Recovery |
| storage 全空 + no Key | `BLINKER_DEVICE_KEY_REQUIRED` |
| Unclaimed N+1 + recovery journal | 恢复同一 Recovery/Claim transaction |
| Pending Claim/Cloud | 只恢复原 exact transaction |
| Ownership/Cloud 局部矛盾或 CRC 错 | `BLINKER_LOCAL_STATE_CORRUPT`，不自动擦除 |

因此用户代码长期保留同一 `bdk_` 是允许的；正常运行既不发送也不校验它。用户换电脑后也可以从设备页
再次查看/复制 Key，或在未擦 storage 时省略 Key。

## 4. 外围授权

WiFi-only 手动设备以 `Blinker-Manual-Key-Id` 和 `Blinker-Manual-Key-Proof` 提交 exact Request/Receipt；raw
Key 不经过 Plain HTTP。Gateway 先验证 key version、logical ID 绑定、HMAC proof、rate limit 和 transaction
state，再进入 Recovery decoder。

WiFi+BLE/BLE-only 默认不显示 Device Key。App 在 secure setup session 内以用户 JWT、owner role、当前
logicalDeviceId、newDeviceInstanceId/setup transcript 创建 recovery intent，并 exact relay 本合同 bytes。

当前旧实例仍 ProtocolReady 时，Education manual key 单独不足以立即替换：Gateway 返回
`ReplacementConfirmationRequired`，App owner 确认后才签 Grant。旧实例离线时，手动档允许 Key 自动恢复，
这是明确的易用性/安全性取舍。

## 5. Exact canonical CBOR

共同规则：definite-length map、升序 unsigned numeric key、最短整数/长度编码；拒绝 unknown、duplicate、
out-of-order、invalid UTF-8 和 trailing data。ID/nonce/digest/signature 为 bstr；logical ID 为 1..64 B UTF-8。

### 5.1 RecoveryPreparationRequest（最大 59 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | requestId | 16 B，非零 |
| 2 | newDeviceInstanceId | 16 B，非零 |
| 3 | recoveryNonce | 16 B，非零 |
| 4 | protocolVersion | `2` |

Request 不携带 logicalDeviceId、owner、ManualDeviceKey、MAC、productId 或 Manifest。目标 logical device 和
current generation 来自已认证 manual key/app recovery intent，避免未认证探测账号资源。

### 5.2 RecoveryGrant（最大 286 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | grantId | 16 B |
| 2 | requestId | 16 B |
| 3 | newDeviceInstanceId | 16 B |
| 4 | logicalDeviceId | 1..64 B UTF-8 |
| 5 | currentGeneration | uint32，非零 |
| 6 | nextGeneration | exact `currentGeneration + 1`，禁止 overflow/跳代 |
| 7 | recoveryNonce | 16 B |
| 8 | requestDigest | SHA-256(exact Request)，32 B |
| 9 | issuedAt | uint64，非零 |
| 10 | expiresAt | uint64，`expiresAt > issuedAt` |
| 11 | recoveryPolicy | `1 = PreserveLogicalDeviceReplaceInstance` |
| 12 | serverKeyId | uint32，非零 |
| 13 | signatureAlgorithm | `1` Ed25519；`2` ECDSA-P256-SHA256 raw `r||s` |
| 14 | signature | 64 B |

签名输入是下列 15 项 canonical array 的 SHA-256：

~~~text
[
  "blinker.device-recovery-grant.v1", 1,
  grantId, requestId, newDeviceInstanceId, logicalDeviceId,
  currentGeneration, nextGeneration, recoveryNonce, requestDigest,
  issuedAt, expiresAt, recoveryPolicy, serverKeyId, signatureAlgorithm
]
~~~

### 5.3 RecoveryReceipt（最大 173 B）

| key | 字段 |
|---:|---|
| 0 | contractVersion = 1 |
| 1 | grantId |
| 2 | requestId |
| 3 | newDeviceInstanceId |
| 4 | logicalDeviceId |
| 5 | currentGeneration |
| 6 | nextGeneration |
| 7 | recoveryPolicy = 1 |
| 8 | grantDigest = SHA-256(exact Grant) |

### 5.4 RecoveryCommitAck（最大 258 B）

| key | 字段 | 约束 |
|---:|---|---|
| 0 | contractVersion | `1` |
| 1 | grantId | 16 B |
| 2 | requestId | 16 B |
| 3 | newDeviceInstanceId | 16 B |
| 4 | logicalDeviceId | 1..64 B UTF-8 |
| 5 | currentGeneration | uint32 |
| 6 | nextGeneration | uint32 |
| 7 | recoveryPolicy | `1` |
| 8 | receiptDigest | SHA-256(exact Receipt)，32 B |
| 9 | committedAt | uint64，非零 |
| 10 | serverKeyId | uint32，非零 |
| 11 | signatureAlgorithm | 1 或 2 |
| 12 | signature | 64 B |

签名输入是下列 13 项 canonical array 的 SHA-256：

~~~text
[
  "blinker.device-recovery-commit-ack.v1", 1,
  grantId, requestId, newDeviceInstanceId, logicalDeviceId,
  currentGeneration, nextGeneration, recoveryPolicy,
  receiptDigest, committedAt, serverKeyId, signatureAlgorithm
]
~~~

CommitAck 只证明 N+1 recovery base 已被服务器接受，不等于 Ownership/Cloud/Controller/ProtocolReady。

## 6. 持久化与切换顺序

设备新增独立 recovery journal；实现 ABI 待 C++/golden 实测冻结，至少 old-or-new 保存：

~~~text
state, requestId, newDeviceInstanceId, recoveryNonce,
logicalDeviceId, currentGeneration, nextGeneration,
grantId, grantDigest, recoveryPolicy
~~~

不得保存 raw ManualDeviceKey。journal 与 Claim `BOC2`、Ownership `BOS2`、Cloud/Controller/Network 分域。

1. 新实例先保存 requestId/recoveryNonce，再发 exact Request；
2. Grant 验签后保存事务材料，通过 recovery-only store operation 安装 `Unclaimed(N+1)` tombstone；fresh
   store 不能因普通 Claim 自行接受任意高 generation；
3. 设备发 Receipt，验证 signed CommitAck 后标记 RecoveryBaseReady；Gateway 锁定 transaction，但不在此时
   踢掉旧实例；
4. 设备复用 Ownership Claim，使用 newDeviceInstanceId/N+1；Gateway 只能返回原 logicalDeviceId；
5. Gateway 接受 Claim Receipt 时，以 generation CAS 原子切换到新 instance/N+1，并撤销旧
   CloudCredential、ControllerCredential 和全部 session；
6. 新实例继续 Cloud/Controller enrollment；App 按 profile readiness 判断完成；
7. 全部完成后清 recovery/claim/enrollment journal。

第 3～5 步掉电时，旧实例可以继续在线，新实例用同一 journal 重试。不得生成 N+2、第二个 logicalDeviceId
或不同 Grant。第 5 步完成后不可自动回滚旧 credential。

## 7. ManualDeviceKey 刷新与 Pending Recovery

App 刷新 Device Key 不修改本合同 body：

- 未绑定 Recovery 使用旧 key version 立即失败，用户复制最新 `bdk_`；
- 已绑定 exact Recovery 可在受限 grace/recovery window 内按 transactionId/request digest 收口；
- 刷新不能签发不同 Grant、改变 N/N+1 或创建并行 transaction；
- 取消 pending recovery 只在 Claim cutover 前有效；旧实例保持 Active，新实例保留 N+1 journal，后续必须
  由新的明确恢复继续，不能本地降回 generation 1；
- Claim cutover 后刷新 Key 只阻止未来 bootstrap，不恢复旧 session。

## 8. App/Console UX

手动模式设备页长期显示统一卡片：

~~~text
设备 Key   bdk_••••••••7HkP   [显示] [复制] [刷新]
首次烧录或完全擦除设备存储后使用；正常运行不会发送该 Key。
~~~

不再显示“首次接入码 `bek_`”与“恢复码 `brk_`”两个入口。App 仍必须把用户操作分开：

- 重新配置 WiFi：保留 ownership；
- 完全擦除后的重新接入：保留 logical device/page/history，可能替换 old instance；
- 删除设备及数据：不可恢复 destructive delete。

新 Manifest 与旧 PageSchema 不兼容时，binding 标记 disabled/needs-review，不自动删除组件、自动化或历史。

## 9. Gateway 事务不变量

- recovery intent 绑定 logical ID/current generation/manual key version 或 authenticated App context；
- 与 Release/Transfer/另一 Recovery 互斥；
- exact retry 返回首次 Grant/Ack bytes；不同 instance/request conflict；
- 在线旧实例需要 owner confirmation；离线自动恢复属于 Education policy 并必须审计；
- Claim cutover 原子更新 relationship/generation并撤销旧 credentials/sessions；
- raw key/proof 不进入日志；recovery 安全事件只含 masked key ID 和公开 transaction metadata；
- PageSchema/history/ACL/automation 保留，Manifest compatibility 产生显式 review 状态。

## 10. 错误与门禁

至少需要以下稳定语义，数字 error 与 exact CBOR body在 codec/golden 批次冻结：

| Error | 动作 |
|---|---|
| ManualDeviceKeyInvalid | 重新复制当前设备 Key |
| ManualDeviceKeyRotated | 从 App 查看最新 Key |
| ReplacementConfirmationRequired | App 确认是否替换在线旧设备 |
| RecoveryAlreadyPending | 恢复 exact transaction |
| GenerationConflict | 刷新 App 状态并重新开始 |
| LocalStateCorrupt | 不自动擦除，先诊断 |
| RateLimited | 按 retryAfter backoff |

退出门禁：

1. C++ restricted-CBOR codec/verifier、journal/coordinator 与 ManualDeviceKey HMAC outer；
2. shared exact/invalid/proof golden 覆盖四 body、两 transcript、key version 和刷新竞态；
3. Host fault test 覆盖所有掉电点、在线旧实例、伪 proof/signature、generation conflict、取消和 cutover；
4. App/Gateway reveal/refresh/confirmation/audit/rate limit 与 PageSchema 保留；
5. ESP32、UNO R4/Renesas/Wio 的 erase-all、重新烧录、Plain/TLS、WiFi+BLE/BLE relay 和真机长稳；
6. 完成前三端 E2E 前不得标记 production-ready。
