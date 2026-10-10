# Device V2 Ownership Reset / Transfer v1

> 冻结日期：2026-08-16。本文是设备、App 与 Gateway 实现归属重置/转移的正式合同；旧
> `ResetCoordinator`、`OwnershipResetGrantVerifier` 和 Enrollment/1 已删除，不再兼容。

## 1. 决策

- `Reset`：设备回到未认领状态，可选择保留或清除 WiFi；
- `Transfer`：把设备交给下一位所有者，必须清除 WiFi；
- 两者都只允许 `nextGeneration = currentGeneration + 1`，禁止跳代；
- 设备先持久化授权，再把 Ownership 写成 next-generation `Unclaimed` tombstone，最后回执；
- Cloud、Controller 和可选 Network 凭据必须等 Gateway 的 server-signed CommitAck 被持久化后再清；
- common body 不包含 owner、tenant、productId、MAC、JWT 或 EnrollmentTicket。这些属于外层已认证
  App/Gateway 会话；明文或 TLS 是 profile policy，不改变本合同的签名要求。

## 2. 四步事务

~~~text
authenticated App/Gateway session
  Device -> OwnershipReleaseRequest
  Gateway -> server-signed OwnershipReleaseGrant
  Device: persist GrantAccepted -> write generation tombstone
  Device -> OwnershipReleaseReceipt
  Gateway: commit ownership registry transaction
  Gateway -> server-signed OwnershipReleaseCommitAck
  Device: persist CommitAcknowledged -> clear Claim journal/Cloud enrollment
          -> clear Cloud/Controller/(Network)
          -> clear release journal last
~~~

App 只能 relay exact bytes，不得 decode 后重建 CBOR。Gateway 必须按
`(deviceInstanceId, currentGeneration, requestId)` 保存首个 exact Grant；同一请求重试返回同一字节串。

## 3. Canonical CBOR

所有 map 使用升序 unsigned integer key、最短整数编码、确定长度容器，拒绝 unknown、duplicate、缺失、
乱序字段和 trailing bytes。

| Body | keys | key 顺序对应字段 | 最大值 |
|---|---:|---|---:|
| Request | 8 | version, requestId, deviceInstanceId, logicalDeviceId, currentGeneration, releaseNonce, operation, networkPolicy | 134 B |
| Grant | 16 | version, grantId, requestId, deviceInstanceId, logicalDeviceId, currentGeneration, nextGeneration, releaseNonce, operation, networkPolicy, requestDigest, issuedAt, expiresAt, serverKeyId, signatureAlgorithm, signature | 288 B |
| Receipt | 10 | version, grantId, requestId, deviceInstanceId, logicalDeviceId, currentGeneration, nextGeneration, operation, networkPolicy, grantDigest | 175 B |
| CommitAck | 13 | version, grantId, requestId, deviceInstanceId, logicalDeviceId, currentGeneration, nextGeneration, operation, networkPolicy, receiptDigest, serverKeyId, signatureAlgorithm, signature | 250 B |

`requestId/releaseNonce/grantId/deviceInstanceId` 均为非零 16 B；digest 为 32 B；signature 为 raw 64 B。
`operation`：Reset=1、Transfer=2；`networkPolicy`：Preserve=1、Clear=2。Transfer+Preserve 非法。

Grant 签名 transcript 是 16 项 canonical array，domain 为
`blinker.ownership-release-grant.v1`；CommitAck transcript 是 13 项 array，domain 为
`blinker.ownership-release-commit-ack.v1`。支持 Ed25519 与 raw P-256 `(r||s)`；设备通过 pinned
`IServerSignatureVerifier` 校验。签名覆盖对应 request/receipt 的 exact SHA-256，不接受业务对象重编码 hash。

## 4. 掉电恢复与清理顺序

固定 168 B `BOR2` journal 保存 Prepared、GrantAccepted 或 CommitAcknowledged；它与 112 B `BOS2`
Ownership、116 B `BCS2` Cloud、Controller table 和 WiFi journal 分离。

| 掉电点 | 重启动作 |
|---|---|
| Prepared 后 | 重发 exact Request，不能换 requestId/nonce |
| GrantAccepted 后、tombstone 前 | 重做同一 generation CAS，再生成 Receipt |
| tombstone 后、Receipt/ACK 前 | 保留 Cloud/Controller/Network 通信能力，重发 Receipt |
| CommitAcknowledged 后、清理中 | 不再等待网络；幂等继续清 Ownership Claim journal、Cloud enrollment journal、Cloud、Controller，Network 最后清 |
| 全部清理后 | 最后删除 BOR2，进入 Released |

不能在 Receipt 前清 Cloud 或当前 Controller，否则可能切断唯一的 CommitAck 路径。任何 `Clear` 请求若未
提供 Network store，在执行任何清理前返回 `NotConfigured`。各 store 的 already-absent 视为幂等成功，
但非 `NotFound` 错误必须保留 journal 并等待恢复。

## 5. 资源与测试门禁

- 共用 operation workspace：384 B，不由 coordinator 持有；
- UNO R4 32 位 ABI 门禁：Record 156 B、Store 8 B、Cleanup 20 B、Coordinator 28 B、WiFi reset adapter 8 B；
- Small SizeProbe：52,228 B Flash、11,880 B global RAM；
- shared golden：C++ 11、App 6、Node 11 个 exact 值；
- Host 覆盖 Request 重启重放、Grant 后输出过小、先 tombstone 后 Receipt、伪 ACK、Reset Preserve、
  Transfer policy、CommitAck 后清理失败及重启续作；加入 Claim journal 后全量 ASan/fuzz 29/29 通过。

权威 fixture：`tests/golden/ownership_release_golden_vectors.properties`。三端生产 codec 和 transaction
实现必须直接纳入该 fixture；reference TypeScript/Node 文件不是可直接部署的业务代码。
