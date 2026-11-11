# Device V2 纯 BLE Enrollment 合同

> 状态：2026-08-24 G1A 设备/Service 合同与 DirectSecureRecord 已实现；App 生产 client 和真机闭环待实现，
> 验收前保持 preview。
> 取代：EnrollmentTicket、OwnershipClaim/Record、LocalSetup 8-operation saga、CloudCredential enrollment 和独立 ControllerCredentialStore。
> BLE Gateway 与日常 Direct 数据保护的权威后续方案见
> `DEVICE_V2_BLE_DIRECT_GATEWAY_ROUTING_ARCHITECTURE.md`。

## 1. 边界与目标

纯 BLE 设备不登录 MQTT，也不保存账号、owner、家庭、分享关系或逻辑页面 ID。设备只持久化：

- 随机 `DeviceInstanceId`；
- 随机非零 `accessEpoch`；
- 一个 Admin 和至多一个 Shared `ControllerCredential`。

账号认领、单设备分享、家庭 ACL、设备页面和 grant 消费记录全部由 Service 保存。MAC 只用于附近发现，不能作为全局设备 ID。

## 2. 唯一逻辑上线流程

逻辑发起方始终是已登录 App/account。BLE bearer 可以是 App 直接连接，也可以在 Gateway profile 中由已授权
网关透明转发 fragment；后者不得终止 Noise、重编码 grant、读取或保存 Admin secret。两种 bearer 复用完全
相同的 Hello、Noise、grant、receipt 和 Method 2 wire，因此“网关辅助发现”不是第二套 ownership 协议。

```text
设备广播 Provisioning profile + 8 B setupSessionLocator
  → App 直连，或由已授权 Gateway 透明转发 BLE fragment
  → bearer 建立加密 BLE link；Noise 仍保持 App 到设备端到端
  → Education/Nearby 完成 Noise NN；Product/OOB 完成 Noise NNpsk0
  → 会话取得 final transcript hash
  → App 发送 HelloRequest
  ← 设备返回 DeviceInstanceId、setupSessionId、locator、随机 accessEpoch
  → App 本地生成 controllerId 与 32 B controllerSecret
  → App 把 secretDigest 和 Hello/session 上下文提交给 Service
  ← Service 返回短期、单次、签名 BleEnrollmentGrant
  → App 在同一 Noise session exact relay：grant + raw secret
  → 设备验上下文、有效期（有可信时间时）、签名和 secret digest
  → 设备一次原子写入 DeviceAccessStore(accessEpoch + Admin)
  ← 设备返回 HMAC ControllerMutationReceipt
  → App 验 receipt，并以 Controller Method 2 直接认证
  → Method 2 派生 Direct session keys，切换 DirectSecureRecord
  → App 向 Service 幂等提交 receipt digest/Method 2 成功事实
  → Service 消费 grant，建立账号/设备/ACL 记录
  → 设备切换 Direct profile，后续使用 BBP/2
```

`controllerSecret` 只存在 App 内存、Noise 明文区和设备 access store；Service 只接收 SHA-256 digest。Receipt 的 HMAC 由 App
验证，Service 不需要取得 raw secret。App 必须在 Direct Method 2 成功后再提交最终 commit，不能仅凭 BLE 写成功更新 ownership。

当前代码默认 Noise NN。它提供应用层机密性，但 NN 与 no-I/O BLE Just Works 都不单独认证目标设备，不能宣称抗
主动 MITM。Education/Nearby 依赖明确物理 setup window、登录 App 和 transcript-bound server grant，并在 UI
展示该安全级别；生产 OOB profile 应使用随机 128-bit QR/setup secret 驱动 NNpsk0。6 位短码不能直接作为
高熵 PSK；若未来需要短码，必须另行引入 PAKE。

## 3. BLE application wire v1

所有消息使用 canonical CBOR map，公共 header 为：

| Key | 含义 |
|---:|---|
| 0 | contract version，固定 `1` |
| 1 | message type |
| 2 | 非零 `requestId` |

消息集合严格限定为：

| Type | 名称 | 额外字段 |
|---:|---|---|
| 1 | HelloRequest | 无 |
| 2 | HelloResponse | 3=deviceInstanceId，4=setupSessionId，5=locator，6=accessEpoch，7=securityProfile，8=serverKeyId，9=signatureAlgorithm |
| 3 | EnrollRequest | 3=encoded grant，4=32 B raw controller secret |
| 4 | EnrollResponse | 3=ControllerMutationReceipt |
| 5 | ErrorResponse | 3=wire error，4=retry class |

上限为 request `320 B`、response `160 B`、operation workspace `256 B`。应用层不再存在 Ticket offer/accept、八种 operation、
authorizedOperations bitmask 或 durable saga record。

## 4. BleEnrollmentGrant v2

Grant 是 16 项 canonical CBOR map：

| Key | 字段 | 约束 |
|---:|---|---|
| 0 | version | `2` |
| 1 | grantId | 16 B，非零 |
| 2 | deviceInstanceId | 16 B，exact Hello |
| 3 | setupSessionId | 16 B，exact Hello |
| 4 | setupTranscriptHash | 32 B，exact Noise final hash |
| 5 | accessEpoch | exact Hello，非零 u32 |
| 6 | controllerId | 16 B，非零 |
| 7 | controllerSecretDigest | SHA-256，32 B |
| 8 | controllerPermissions | 首次安装必须为完整 Admin |
| 9 | nonce | 16 B，非零 |
| 10 | issuedAt | epoch seconds |
| 11 | expiresAt | 大于 issuedAt，最长 15 分钟 |
| 12 | securityProfile | Education/Nearby 或 Product/OOB，必须 exact Hello |
| 13 | serverKeyId | 非零，必须 exact Hello |
| 14 | signatureAlgorithm | Ed25519 或 raw P-256/SHA-256，必须 exact Hello |
| 15 | signature | 64 B |

签名 transcript domain 为 `blinker.ble-enrollment.v2`，不包含 signature 本身。最大 grant `270 B`，流式/调用方工作区上限
`217 B`。ESP32 产品使用 P-256 key ring；UNO R4/Wio portable 产品使用 Ed25519 key ring，wire signature 均固定 64 B。
profile 与 signer tuple 都在签名内并由设备按 Hello 精确核对，客户端不能把 Product/OOB 请求降级为
Education/Nearby。

## 5. 重放、掉电与恢复

- `setupSessionId` 和随机 `accessEpoch` 绑定一个物理 setup window；相同 locator 的短暂重连复用二者，新 window 重新生成。
- requestId 必须递增；只允许同一活动 Noise session 内相同 requestId、相同完整 bytes 的 exact replay。
- 同 requestId 不同 bytes 返回认证失败；跳号返回可纠正 sequence error。
- access root 已存在时设备拒绝新的 enrollment session。设备不额外保存 grant ledger，避免重复 Flash 状态。
- 原子写成功后重发相同 EnrollRequest 会产生相同 receipt，不重复变更状态。
- 若写成功后 App 在收到响应前退出，App 仍可用保留的 secret 执行 Method 2 并恢复 commit。
- 若 App 同时丢失 secret，必须经明确物理 reset 清除 access root，再签发新 grant；不能由网络包静默重置。
- `clearAll()` 对 BLE-only root 执行完整清除，使设备重新进入 enrollment；若同一 root 还保存 DeviceKey，则保留 cloud 恢复入口。

设备没有可信 Unix 时间时，不能单独判断 grant 是否过期；此时依靠同一 Noise transcript/session 的强绑定和 Service 单次消费。
有可信时间时设备同时执行 `[issuedAt, expiresAt)` 检查。该限制必须在威胁模型中明确，不能宣称所有 MCU 都在离线状态验证时效。

2026-09-05 验收补充：上述时间检查针对首次安装，不得阻断同一活动 session 中**已完成请求的 exact replay**。
后者返回相同 receipt 且不重写 Flash；同 requestId 改 bytes 不继承这个例外。App 已保存的 pending credential
通过 Method 2 确认安装后，原 intent 即使 grant 已过期也允许恢复云端 commit；Active exact commit 不更新
ownership/Presence/时间戳。cancelled/superseded 记录不适用。Admin receipt 的 HMAC 在 App 校验，Service 无
Admin secret，只检查 owner、原 intent 的字段绑定与提交幂等，`method2Confirmed` 不是硬件远程证明。

## 6. App 实施清单

1. 根据 advertising capability 选择 BLE enrollment v1，不再加载旧 Ticket/LocalSetup codec。
2. 建立平台要求的加密 BLE link，再完成 Noise NN；缓存 locator、sessionId、transcript hash。
3. controller secret 必须使用系统 CSPRNG 生成，只放内存/安全存储，不写日志、埋点或普通偏好设置。
4. 调用 Service grant API，exact relay 返回 bytes；不重编码服务器签名 body。
5. 验证 HMAC receipt，执行 Method 2，完成 Direct session key 派生并切换 Secure Record 后幂等 commit。
6. 区分“可重传同一帧”“重启 Noise session”“需要物理 reset”三类错误。
7. 页面创建仍由 Manifest/PageLayout 流程负责，不能把页面 JSON 塞入 enrollment grant。
8. Gateway 辅助认领时 App 必须把 Gateway 当作不可信字节 relay；Admin 安装成功后，再通过独立
   ControllerControl 流程安装 Gateway credential，不能让 Gateway credential 代表 owner。

## 7. Service 实施清单

1. 仅向已登录用户、有效 nearby setup intent 签发最长 15 分钟的一次性 grant。
2. 唯一键至少包含 grantId；同时记录 account、deviceInstanceId、setupSessionId、accessEpoch、controller digest、状态和到期时间。
3. 同一 intent 的重试返回相同 grant bytes；冲突参数不得覆盖原记录。
4. commit 必须幂等，并要求 App 声明已完成 receipt 校验与 Method 2；超时 intent 不自动形成 ownership。
5. ACL/页面记录只在服务器建立；设备端 accessEpoch 是撤销世代标签，不是账号或家庭 ID。
6. reset/recovery 创建新 setup window 和新随机 accessEpoch；旧 grant、旧 commit、旧 controller grant 均不得跨世代生效。
7. device/service exact golden 已完成；App 必须复用同一 v2 fixture，未完成前不开放生产 BLE 认领入口。

### 7.1 纯 BLE 撤权的物理限制

纯 BLE 设备离线时，Service 不能主动修改设备中已经持久化的 Shared secret。服务器 ACL 可以立即停止官方 App 的授权和
所有云端转发，但已经缓存 secret 的恶意/旧客户端，在设备收到 Rotate/Revoke 之前仍可能完成 Direct Method 2。

当前“两槽 Admin + Shared group”是明确的资源/撤权粒度折中：

- 撤销某一个分享用户时，Service 先立即撤销该用户 ACL；
- 下一次 Admin 或仍获授权的 App 靠近设备时，relay server-signed Shared Rotate/Revoke；
- 设备 durable ACK 后，再向其余共享用户分发新 Shared secret；
- 若产品要求真正远程即时撤权，必须让设备具备网络，或改为每用户 credential 槽并承担额外 SRAM/Flash。

因此不得对纯 BLE 宣称“服务器 ACL 撤销后设备端立即失效”。发布文案必须区分 server authorization immediate 与
offline direct credential eventual。

## 8. 发布门禁

- Host：签名/context/time/digest、sequence、exact replay、原子故障、clear/re-enroll。
- Arduino：ESP32、UNO R4、Wio Terminal 三个纯 BLE selector clean compile 与 SizeProbe。
- 真机：加密连接、Noise、首次安装、断电、响应丢失、App kill、重连 Direct、物理 reset 后重新认领。
- 三端：grant 单次消费、双账号冲突、单设备分享、ACL 立即停云转发、附近同步后 Shared secret 轮换、旧 epoch 在设备完成 reset/re-enrollment 后失效。

任一项未通过时，`BlinkerBLE.h` 只能标记 preview。
