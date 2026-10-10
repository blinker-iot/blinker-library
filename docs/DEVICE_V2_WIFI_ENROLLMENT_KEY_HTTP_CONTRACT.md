# Device V2 WiFi EnrollmentKey HTTP Authorization v1（已被 ManualDeviceKey 取代）

> 2026-08-17 最终状态：一次性、只显示一次的 `EnrollmentKey/bek_` 产品决策已撤回。手动模式现在使用
> 长期可查看/复制/刷新的 `ManualDeviceKey/bdk_`，通过 key id + HMAC proof 授权首次 Claim 或全擦
> Recovery；权威合同见 `DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md`。本文只保留既有 C++ 软件探针、arena、
> endpoint 和掉电测试证据，不能再作为 App/Gateway 产品合同。后续实现应复用 Claim/Cloud/arena，替换本
> 文件的 Key parser/header/error/lifecycle，不保留未发布兼容层。

> 历史状态（2026-08-16）：R4b-3 第二片曾冻结设备侧 Key、HTTP header、四个 endpoint、成功状态、strict
> error body 和可断电恢复的 WiFi onboarding flow。Ownership Receipt 只有收到 server-signed CommitAck 才清
> journal；服务器生产实现、official composition 接线和真机 E2E 尚未完成。

## 1. 边界

本合同只适用于没有 App 本地配网通道的 WiFi-only DIY/教育设备。`EnrollmentKey` 是服务器生成的一次性
bootstrap capability，用于授权同一设备实例的首次 Ownership Claim 和随后的 CloudCredential Enrollment
事务。它不是 `deviceId`、MQTT 密码、CloudCredential、ControllerCredential 或设备重启登录凭据。

`EnrollmentTicket` 不使用本合同。R4b-4a Ticket 已绑定 BLE setup session、完整 provisional handshake
hash、controller digest/权限和短 TTL；App 登录态由 Gateway intent 绑定。把 Key/Ticket 抽象成万能 token
仍会丢失这些安全属性。

历史上曾计划另加短期 `RecoveryKey/brk_`；该双码 UX 也已撤回。当前同一 `ManualDeviceKey/bdk_` 作为
InitialClaim/DeviceRecovery 的外围授权，而 RecoveryGrant 仍是独立 signed common wire。不要从以下历史
`bek_` 章节推导当前产品行为。

## 2. 以下章节均为历史实现记录

从本节开始出现的 `bek_`、`Blinker-Enrollment-Key`、一次性消费和 `brk_` 提示，仅描述仓库中待删除/重构的
软件探针，不是当前产品 API、App 文案或服务器合同。新代码和示例必须使用
`DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md`；保留这些细节只是为了明确哪些 arena、endpoint、journal 与测试
框架可以复用，不能把它们重新发布为兼容层。

### 2.1 历史 Key 文本格式

~~~text
bek_<unpadded-base64url>
~~~

- 总长度 26..90 ASCII byte；固定小写前缀 `bek_`；
- 后缀只允许 `A-Z a-z 0-9 - _`，不得含 `=`、空格、控制字符、CR/LF 或 NUL；
- 最短后缀为 22 字符，对应服务器至少生成 128 bit 随机熵；设备端格式校验不能替代服务器熵检查；
- Key 比较、消费、过期和绑定由 Gateway 完成；设备只保存/引用 opaque text，不解析项目、用户或产品字段。

代码中写入的教育档可以让 `StringView` 指向 Flash/const 文本，不要求复制到常驻 RAM。动态写入 EEPROM/NVS
时也使用同一 view 接口。设备一旦具有匹配 generation 的 Active Ownership 和 CloudCredential，正常 boot
不得再校验或发送固件中残留的 Key。

## 3. HTTP 外层

每个 WiFi bootstrap POST 使用：

~~~http
Content-Type: application/cbor
Blinker-Enrollment-Key: bek_...
~~~

body 必须是现有 Ownership Claim 或 Cloud Credential Enrollment 的 exact canonical CBOR，Key 不得加入、
包装或重编码该 body。`EnrollmentKeyHttpExchange` 只增加一个 header，复用注入的 `IHttpClient`，不持有
Arduino Client、TLS、WiFi、CBOR buffer，也不复制 Key 到 body。

同一 HTTP request 进行中不得被下一请求覆盖。host、target、body、Key 和 response buffer 的 view 在请求
结束前必须有效。Plain/TLS 由 build/profile 显式选择，不得运行时静默降级。Education Plain 会暴露 Key，
只能声明可被窃听/抢占的受限安全等级。

四个 endpoint 与成功响应冻结为：

| 阶段 | POST target | 成功响应 |
|---|---|---|
| Ownership request | `/api/v2/device-onboarding/ownership-claims` | `200` + exact signed `OwnershipClaimGrant` |
| Ownership receipt | `/api/v2/device-onboarding/ownership-receipts` | `200` + exact signed `OwnershipClaimCommitAck` |
| CloudCredential request | `/api/v2/device-onboarding/cloud-credentials` | `200` + exact signed `CloudCredentialEnrollmentGrant` |
| CloudCredential receipt | `/api/v2/device-onboarding/cloud-receipts` | `200` + exact HMAC `CloudCredentialEnrollmentCommitAck` |

任何其他 2xx、空 body、3xx redirect 或 HTML/JSON error 都是协议错误。尤其禁止用空 `204` 确认 Ownership
Receipt：Plain HTTP 攻击者可伪造 204，使设备提前清 journal 而 Gateway 仍 Pending。signed CommitAck 必须绑定
`SHA-256(exact OwnershipClaimReceipt)`。

Ownership/EnrollmentKey 阶段的 exact error body 是 canonical CBOR map `{0:1, 1:error, 2:retryClass,
3?:retryAfterSeconds}`，最大 15 B；unknown/duplicate/out-of-order/trailing 均拒绝。错误码冻结为
`4000 Malformed`、`4001 UnsupportedVersion`、`4002 AuthenticationRequired`、`4100 KeyExpired`、
`4101 KeyConsumed`、`4102 KeyConflict`、`4200 OwnershipAlreadyActive`、`4201 GenerationConflict`、
`4202 ClaimConflict`、`4203 RequestExpired`、`4500 RateLimited`、`4501 TemporarilyUnavailable`。
retryClass 为 `Never/SameRequest/Backoff/RestartTransaction/ReplaceEnrollmentKey/ReclaimOwnership`，只有
Backoff 携带非零 retryAfter。Cloud 阶段继续使用已冻结的 `CloudEnrollmentErrorBody` 3xxx，不再复制第二套
Cloud error enum。

当空存储的新 deviceInstanceId 使用已消费 `bek_` 时，Gateway 返回 `4101 KeyConsumed +
ReclaimOwnership`，且不得泄露被原 key 创建的 logicalDeviceId/owner。设备映射为
`BLINKER_RECOVERY_REQUIRED`。草案曾要求 App 另生成 `brk_`，但该交互未进入正式实现且现已撤回；当前行为
是提示用户从同一设备页复制仍可查看的 `ManualDeviceKey/bdk_`，由服务器签发独立 RecoveryGrant，也不能
偷偷创建第二台 logical device。

## 4. Gateway 消费语义

Gateway 应把 Key 消费建模为一项可恢复的 bootstrap transaction，而不是第一次 HTTP 到达即不可逆删除：

1. 首次合法 Ownership request 以 `(key, deviceInstanceId, generation, requestId)` 原子绑定事务；
2. 相同 exact request 必须返回相同 signed Grant；不同实例、generation 或 requestId 返回冲突；
3. Ownership Receipt 确认后，Key 仍只允许该已绑定事务继续完成 CloudCredential Enrollment；
4. Cloud CommitAck 完成后事务变为 Complete，Key 不再授权任何新请求；
5. Key 过期但服务器已经绑定事务时，允许同一 exact transaction 在有限恢复窗口内收口；不得生成第二个
   logicalDeviceId 或要求设备换 secret；
6. header、Key 和 raw secret 禁止进入 access log、APM、analytics、错误页面和普通数据库字段。

## 5. Ownership Claim 掉电 journal

此前 `OwnershipClaimCoordinator` 只把 requestId/claimNonce 放在 RAM，无法兑现文档中的断电幂等。本片改为
独立固定 48 B `BOC2` record：

| offset | 字段 | 大小 |
|---:|---|---:|
| 0 | magic `BOC2` | 4 B |
| 4 | schema = 1 + reserved | 4 B |
| 8 | ownershipGeneration，big-endian | 4 B |
| 12 | requestId | 16 B |
| 28 | claimNonce | 16 B |
| 44 | CRC32 | 4 B |

journal 先于首个 request durable；Grant 验签并安装 Active Ownership 后仍保留。只有服务器确认 Receipt 后
`completeClaim()` 才 exact-clear。这样 Grant 前重启、Ownership 已落盘但 Receipt 丢失、服务器已收 Receipt
但响应丢失三种情况都能重放同一 request/grant/receipt。取消只允许发生在 Ownership 尚未 Active 时。

Ownership Reset/Transfer 的 signed CommitAck 到达后会先幂等清除该 journal，再清 Cloud enrollment、Cloud、
Controller 和可选 Network；Release journal 仍最后删除。

当前 32 位 RAM 门禁：`OwnershipClaimRecord` 36 B、store 8 B、coordinator 20 B、
`EnrollmentKeyHttpExchange` 36 B、`WifiOnboardingFlow` 不超过 60 B。flow 不持有 secret/body 副本，四步共享
caller-owned request 228 B、response 250 B、workspace 256 B；后续 official composition 必须与 Cloud Session
分阶段复用 arena，不能同时再常驻一套 onboarding buffer。

第三片已经把该要求落实为固定 1,414 B `WifiControlPlaneArena`：Onboarding 借用 256/640/256 B，完成并
释放 view 后，Cloud Session 在同一地址借用 256/640/518 B。与当前 Cloud Session 的 1,414 B 相比不增加
第二套静态大 buffer；若独立叠加 Onboarding 228/250/256 B，则会额外浪费 734 B。生命周期对象在 32 位
UNO R4 上为 40 B。

设备 Host 已覆盖完整四 POST、Active 重启零 HTTP、Grant 前/Ownership Receipt 后/Cloud Receipt 后断电
重放、429 Backoff、Cloud RequestExpired 重建、transport retry、错误 CommitAck 不清 journal 与 buffer overlap。
本批最终软件门禁为 MSVC C++11 `/W4 /WX` 与 ASan 30/30、20,000 次 deterministic fuzz、全量三端
golden、12-package boundary 和 ESP32/UNO R4 Plain/TLS 聚合包确定性全部通过。Arduino Core 实编为
ESP32 319,511/21,672 B、Wio Terminal 48,236 B Flash；UNO R4 Small SizeProbe 为 52,244/12,020 B，
其中 `WifiOnboardingFlow` 的 32 位 ABI 实测 60 B。以上仍不是联网、真实掉电或长期运行准入。

## 6. 已实现 flow、staged lifecycle 与下一片

`WifiOnboardingFlow` 已实现：

~~~text
durable Claim request -> signed Ownership Grant -> Receipt -> signed CommitAck
-> sealed CloudCredential request -> signed Grant -> HMAC Receipt/CommitAck
-> existing Cloud Session -> MQTT
~~~

它启动时总是先查询 Claim journal；Active Ownership 但 Claim 未收口时仍先重放 Claim，不能跳到 Cloud。
Claim 收口后按 `Ready/Prepared/AwaitingCommitAck/Active` 恢复 Cloud journal。网络或 SameRequest 错误调用
`retry()` 重放 exact transaction；Backoff 在 `retryAfter` 前返回 `WouldBlock`；只有尚未 Active 的 Claim 或
Prepared Cloud transaction 才允许 `RestartTransaction`。ReplaceEnrollmentKey/ReclaimOwnership 不会偷偷擦
身份，而是停在可诊断 Fault。

第三片已新增平台无关 staged lifecycle：先完成 onboarding，再允许现有 Cloud Session → MQTT 使用共享
arena；断网时关闭 cloud gate，已经 Active 的 flow 不重新发送 EnrollmentKey。它尚未替换 official WiFi
composition；此后虽已补齐真实软件 provider，产品路径仍缺真机运行和完整组合门禁。

下一片原计划先完成 crypto portability 决策与目标基准；该项现已用不改 wire 的 portable
X25519/AES-GCM/Ed25519 verify、ESP32 native P-256 和 UNO `SoftwareATSE` P-256 provider 完成软件/SDK
门禁。下一步是目标真机 known-answer、stack/heap 和 official composition 接线。Cloud envelope 不新增
算法，仍固定 X25519/HKDF-SHA256/AES-256-GCM；Ed25519/P-256 只是 server signature profile。
不得用恒真 verifier、空签名、TLS-only 假设或 ESP32 专属公开 facade 绕过门禁。服务器/App 先按本文件
更新实施合同，不代表当前生产路由已经存在。
