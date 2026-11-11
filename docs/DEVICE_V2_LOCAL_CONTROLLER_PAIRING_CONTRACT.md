# Device V2 LocalBLE Controller Pairing Contract v1

> **已退役（2026-08-23）：**无服务器签名的 LocalBLE pairing 产品图已删除。公开纯 BLE 统一使用服务器签名 `BleEnrollmentGrant`；本文只保留历史威胁模型比较。

> 更新：2026-08-17。R4b-6A exact pairing 与 R4b-6B Noise application、共享 BLE setup lifecycle、
> BLE-only product lifecycle 已完成软件门禁；R4b-6C 又完成 ESP32/UNO R4 的 Platform BLE/LocalBLE
> 四个 internal official composition。真机与公开 facade 尚未完成。

## 1. 结论

LocalBLE 不进入 Ownership/Cloud/服务器控制面，也不能伪造 `ControllerGrant`。设备只在当前 Noise secure
session 已完成、用户于设备侧完成物理确认后，打开一个最长 5 分钟的 volatile pairing window。窗口内安装：

~~~text
domain = Local
ownershipGeneration = 0
credentialVersion = 1
controllerId = App CSPRNG 16 B
secret = App CSPRNG 32 B
permissions = 设备策略，不由 App 自行扩大
~~~

安装成功后设备返回以该 secret 计算的 HMAC receipt；App 验证后才把 secret 写入 OS
Keychain/Keystore。普通重启直接使用持久 `ControllerCredential` 和 Controller Method 2，不重新配对，也不依赖
任何过期 Ticket。

## 2. 安全窗口

`LocalControllerPairingCoordinator::beginConfirmedPairing(setupTranscriptHash)` 是产品的物理在场边界：

- 只能由按键、触摸确认等设备侧可信事件调用，BLE request 不得直接或间接打开窗口；
- 必须传当前 Noise handshake final hash，确认动作因此只授权当前 secure session；
- coordinator 用平台 CSPRNG 生成 16 B `pairingNonce`，拒绝全零或 RNG failure；
- 默认窗口 60 s，配置范围 1～300,000 ms；超时、session 断开或产品取消时清零 nonce/transcript/replay 状态；
- 默认权限为 `Observe | Control`。设备在 Challenge 中公布并要求 Request exact echo，App 不能请求权限升级；
- 可选短码增强使用既有 Noise `NNpsk0` profile。普通 Arduino/教育设备可以只用 Noise NN + 物理确认，
  不增加永久 SetupCode 或写入 Sketch 的 bearer key。

物理确认只能证明“用户在短窗口内授权当前近场 session”，不能等价为 factory hardware identity。需要抗主动近场
冒充的产品应增加 NNpsk0/QR/factory proof，并由 App 如实显示安全等级。

## 3. Canonical CBOR

所有 map key 必须按 unsigned canonical 顺序出现；unknown、duplicate、out-of-order、错误长度、错误 version 和
trailing data 均拒绝。以下字段值均为 wire value，不是 C++ struct ABI。

### 3.1 PairingChallenge

~~~text
{
  0: version = 1,
  1: deviceInstanceId bstr16,
  2: pairingNonce bstr16,
  3: setupTranscriptHash bstr32,
  4: permissions uint32,
  5: expiresInMillis uint32
}
~~~

固定向量编码 80 B；保守输出上限 86 B。

### 3.2 PairingRequest

~~~text
{
  0: version = 1,
  1: deviceInstanceId bstr16,
  2: pairingNonce bstr16,
  3: setupTranscriptHash bstr32,
  4: controllerId bstr16,
  5: credentialVersion = 1,
  6: permissions uint32,
  7: controllerSecret bstr32
}
~~~

固定向量编码 131 B；保守输出上限 135 B。`controllerSecret` 只允许位于 Noise transport plaintext，调用者在
处理后清零其 plaintext workspace；禁止广播、日志、analytics、剪贴板或服务器 relay。

### 3.3 PairingReceipt

~~~text
{
  0: version = 1,
  1: deviceInstanceId bstr16,
  2: pairingNonce bstr16,
  3: requestDigest bstr32,       # SHA-256(exact canonical PairingRequest)
  4: controllerId bstr16,
  5: credentialVersion = 1,
  6: permissions uint32,
  7: secretDigest bstr32,        # SHA-256(controllerSecret)
  8: proof bstr32                # HMAC-SHA256(controllerSecret, transcript)
}
~~~

proof transcript 为 canonical array：

~~~text
[
  "blinker.local-controller-receipt.v1", 1,
  deviceInstanceId, pairingNonce, requestDigest, controllerId,
  credentialVersion, permissions, secretDigest
]
~~~

固定向量 transcript/receipt 为 160/166 B；保守输出上限 172/174 B。设备端通过 CBOR sink 直接流入
`HmacSha256`，不创建完整 transcript scratch。

### 3.4 Noise application envelope

R4b-6B 冻结一个只承载 Local pairing 的 application envelope。它复用现有
`BleLocalProvisioningEndpoint`/Noise record，不另建 GATT service，也不复用平台 `LocalSetup` 的 Ticket 操作：

~~~text
request  = {0: version=1, 1: type, 2: requestId, 3?: body}
hello    = {0: 1, 1: 129, 2: requestId, 3: deviceInstanceId, 4: setupSessionLocator}
success  = {0: 1, 1: 130|131, 2: requestId, 3: challenge|receipt}
error    = {0: 1, 1: 255, 2: requestId, 3: wireError, 4: retryClass}
~~~

请求 type 为 Hello/Challenge/Pairing `1/2/3`，响应 type 为 `129/130/131`，错误为 `255`；`requestId`
必须非零，只有无法恢复 requestId 的 malformed request 可用零值错误关联。Pairing request 的 body 是 3.2 的 exact
canonical bytes，Challenge/Receipt body 分别是 3.1/3.3 的 exact bytes，禁止解码后重编码。

固定向量大小为 Hello request/response `7/36 B`、Challenge request/response `7/91 B`、Pairing
request/receipt `141/177 B`、physical-confirm error `14 B`；设备常量上限为 request `148 B`、response
`188 B`、operation workspace `174 B`。状态固定为 `AwaitingHello -> AwaitingSecureSession -> Ready`：只有
Hello 后的同一 Noise final hash 能进入 Ready，BLE Challenge/Pairing request 不能打开物理窗口，session end
无条件清 locator/transcript 并结束 coordinator window。

## 4. 状态、重放与掉电

1. 无活动物理确认窗口：返回 `AuthenticationRequired`；
2. context 中 device ID、nonce、Noise hash 或设备权限策略不一致：返回 `AuthenticationRequired`；
3. 首次有效 Request：原子 `installVerified(Local, generation=0, version=1)`；Store 成功即锁定 request digest，
   不能等 receipt 编码/发送成功后才锁窗口；
4. 同窗口 exact Request 重试：Store 的 same-credential 语义和保存的 32 B request digest 产生相同 receipt；
5. 同窗口不同 Request：返回 `SequenceConflict`，不能安装第二个 identity；
6. 若持久化已成功但 response buffer/链路失败，different request 已被拒绝，App 可在窗口内重发 exact bytes；
7. 超时或断开清窗口，但不回滚已持久化 credential。再次配对必须重新物理确认并获得新 nonce。

本 coordinator 不提供 rotate/revoke。后续 controller 管理必须在 Method 2 已认证、具有对应权限的独立控制合同下
执行，不能把首次物理 bootstrap 变成长期万能管理入口。

## 5. 资源与跨平台边界

- Core 只依赖 `IControllerCredentialStore`、`IRandom`、`IClock`、restricted CBOR/SHA-256/HMAC；无 BLE SDK、
  Arduino、服务器签名、Ownership、JSON、heap 或平台分支；
- 32 位 coordinator ABI gate 为 `<=128 B`；UNO R4 ELF map 的
  `blinker_size_local_controller_pairing_coordinator` 精确为 `0x7c = 124 B`。最大临时栈为 credential、三个 32 B
  digest/proof 及小 struct，不持有 Request body；
- 32 位 application/BLE-only/WiFi+BLE lifecycle 精确为 `56/44/52 B`；平台 Saga completion 与 Local
  credential completion 都是 `8 B`。共享 `BasicBleSetupLifecycle` 只负责 locator、广播 mode、完成态和断线恢复，
  不解析或合并两种授权合同；
- ESP32、UNO R4/Renesas 或其他 BLE MCU 复用同一合同与 coordinator，只替换 BLE、clock、random、atomic storage
  adapter；
- `BCT2` 是设备本地两槽、140 B flash ABI，不是 App wire body。容量满返回明确错误，不覆盖旧 controller。

Provisioning 广播也明确分域：普通 Local pairing capabilities 为 `0x0043`，NNpsk0 为 `0x0061`；两者都包含
`LocalControllerPairingV1`，且绝不包含 `EnrollmentV1` 或 `WifiConfigV1`。App 必须按 capability 选择本地配对
codec，不能看到 Provisioning mode 就一律请求服务器 Ticket。

## 6. App 与服务器执行边界

App：按 `LocalControllerPairingV1` 选择本流程 → Hello → 完成 Noise handshake → 等待设备物理确认 → strict
decode Challenge → CSPRNG 生成 controllerId/secret → 发送 exact Request → 验
requestDigest/secretDigest/HMAC receipt → 先用 Method 2 验证 direct session，再写 Keychain/Keystore 和提交
`paired` reducer。response 丢失时保留内存 secret 并 exact replay；若 App 已丢失 secret/receipt，用户通过产品侧
动作重新打开 Local pairing mode，再对新的 exact Noise session 单独物理确认并安装新 controller。

服务器：LocalBLE 不调用 Ticket、Claim、ControllerGrant、device registry 或 PageSchema API；不得上传 raw secret、
receipt 或本地 `paired` 状态冒充 logical ownership。若设备以后升级为平台 BLE-only，必须走一次独立
EnrollmentTicket + Ownership Claim，Local generation 0 credential 不自动转换为 Ownership domain。

## 7. R4b-6B/6C 完成边界与下一门禁

R4b-6B 已实现窄的 LocalBLE Noise application、Local credential completion、平台 Saga completion、共享
BLE setup lifecycle 和可承载两种合同的 `BleOnlyLifecycle`。已配对设备可由产品侧动作重新进入 Local pairing
广播；该动作只切换 radio mode，不替代绑定当前 Noise transcript 的第二次物理确认。平台 BLE-only 继续使用
`LocalSetupApplication + LocalSetupSaga(ticketOperations=0, network/cloud=null)`，不会落入 Local generation 0。

R4b-6C 已按此边界组装四个独立对象图，并复用同一 `BleOnlyRadio/BleWorkspace`：LocalBLE package 的
generated config 没有 server key、host、transport 或 cloud 字段，Platform package 则只携带公开 signing ring。
ESP32 LocalBLE 为 `731,287 B Flash / 40,792 B global RAM`、singleton `6,176 B`；UNO R4 LocalBLE 为
`159,976/14,840 B`、singleton `5,072 B`。没有增加第二套 endpoint、MCU 专属业务 API 或公开 QuickStart。
R4b-7 之前仍不发布 `Blinker.begin(BLE...)`；必须先用真机验证 exact connection encryption、重新配对、
断连恢复、动态内存和长稳。
