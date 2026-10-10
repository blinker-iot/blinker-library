# Device V2 Controller Credential Control Contract v2

> 状态：2026-08-18 修订。Core v2 已完成软件验证；产品语义改为每设备 `Admin + Shared` 两个访问组，
> 设备不保存 OwnershipRecord。P6 改用 4 B `accessEpoch` 和单原子 DeviceAccessStore；公开 facade、App/Service、
> Plain MQTT MAC 与真机仍未完成。权威边界见 `DEVICE_V2_AUTHORIZATION_AND_SHARING_DECISION.md`。

## 1. 决策与边界

- 日常 Direct BLE 鉴权继续使用 Controller Method 2；本合同只负责已认领设备的
  `Ownership` controller Install/Rotate/Revoke。
- `LocalBLE` 使用独立的物理确认配对合同，不复用服务器 Grant。
- 一条 `ControllerCredential` 表示当前设备的访问组而不是一台手机。Small profile 最多两槽：Admin 为全权限，
  Shared 仅 Observe/Control；Shared 可由 Service KMS 密文保存并按每用户每设备 ACL 交付。
- 服务器签发的 Grant 不携带 raw secret。Install/Rotate 由 App 在加密本地链路中同时传送
  signed Grant 与 raw secret；设备校验二者 digest 后再原子落盘。
- `logicalDeviceId`、owner 和 share ACL 都是服务器关系，不进入设备。Grant 绑定唯一
  `DeviceInstanceId + accessEpoch`；wire 字段 `ownershipGeneration` 暂保留编码名，设备语义严格是访问代次。
- Arduino 设备不被要求持有可信 epoch。Grant 的 TTL、单次签发与事务状态由服务器检查；设备以当前
  16 B CSPRNG `controlNonce` 绑定一次本地控制窗口，防止旧 Grant 跨会话重放。
- 运行时管理入口只接受：已协商 `FeatureControllerControl`、Local transport、encrypted link、
  已授权 `ManageControllers` 的同一 session。签名不能替代链路机密性或调用方授权。

## 2. BBP/2 message family

| Kind | 值 | 方向 | Body |
|---|---:|---|---|
| `ControllerControlOpen` | `0x30` | App -> Device | `[wireVersion=1]` |
| `ControllerControlChallenge` | `0x31` | Device -> App | `[1, controlNonce(16)]` |
| `ControllerMutation` | `0x32` | App -> Device | `[1, grantBytes, controllerSecretBytes]` |
| `ControllerMutationReceipt` | `0x33` | Device -> App | raw receipt bytes |

HELLO capability 为 `FeatureControllerControl = 1 << 9`；bit 8 保留给已撤回实验，不得复用。
Open/Mutation 是请求且 flags 必须为 `0`；Challenge/Receipt 是 `FlagIsResponse`，沿用请求 sequence。
Revoke 的 secret 为 empty bstr；Install/Rotate 必须恰为 32 B。

这些消息不承担首次 controller 引导。首次引导必须在已认证的 provisioning secure channel 中完成，
或由另一份明确的物理确认合同完成；不得为了消除“先有权限还是先有 controller”的循环而允许匿名 Open。

## 3. ControllerGrant exact CBOR

Grant 是固定 14 项 canonical CBOR array：

```text
[
  contractVersion=2,
  operation, grantId, deviceInstanceId, ownershipGeneration,
  controllerId, expectedCredentialVersion, credentialVersion, permissions,
  secretDigest, controlNonce, serverKeyId, signatureAlgorithm, signature
]
```

字段约束：

- operation：Install=`1`、Rotate=`2`、Revoke=`3`；
- `grantId`、`deviceInstanceId`、`controllerId`、`controlNonce`：非全零 16 B；
- `ownershipGeneration`、`serverKeyId`：非零 `uint32`；
- Install：expected version 为 0，目标版本非零；
- Rotate：expected version 非零，目标版本严格增大；
- Revoke：目标版本等于 expected version，permissions 为 0，digest 为 32 B 全零，raw secret 为空；
- Install/Rotate：permissions 为当前合法非零权限位，secret digest 为非全零 SHA-256；
- signature algorithm：Ed25519=`1` 或 P-256 raw `r||s`=`2`；signature 为 64 B。

签名 transcript 同样是固定 14 项 canonical array：

```text
[
  "blinker.controller-grant.v2",
  contractVersion, operation, grantId, deviceInstanceId,
  ownershipGeneration, controllerId, expectedCredentialVersion,
  credentialVersion, permissions, secretDigest, controlNonce,
  serverKeyId, signatureAlgorithm
]
```

服务器签名 `SHA-256(transcript)`。设备必须 exact-match 自身 instance、当前 Active ownership
generation、当前 endpoint 持有的 nonce，并验证 secret digest 与 server signature。

## 4. Receipt 与 Commit

Receipt 保持 canonical 数字键 map，version 改为 `2`：

```text
0 version, 1 operation, 2 grantId, 3 deviceInstanceId,
4 ownershipGeneration, 5 controllerId, 6 credentialVersion,
7 permissions, 8 secretDigest, 9 proofKind, 10 proof
```

Install/Rotate proof：

```text
HMAC-SHA256(newControllerSecret,
  canonical-cbor([
    "blinker.controller-receipt.v2",
    version, operation, grantId, deviceInstanceId, ownershipGeneration,
    controllerId, credentialVersion, permissions, secretDigest, proofKind
  ]))
```

App 验证 receipt HMAC 后，必须再用新凭据完成 Method 2，才可向服务器提交
`Method2Confirmed(1)`。服务器没有 raw secret，不能声称自行验证了 HMAC。Revoke 使用
`proofKind=None(0)` 和空 proof，Commit 使用 `AuthorizedRevoke(2)`。

Commit 仍是 10 项 canonical 数字键 map，version 为 `2`：

```text
0 version, 1 grantId, 2 operation, 3 deviceInstanceId,
4 ownershipGeneration, 5 controllerId, 6 credentialVersion,
7 SHA-256(receipt), 8 verification, 9 idempotencyKey[16]
```

## 5. 状态机与失败恢复

1. 已具备管理权限的本地 session 发送 Open；设备生成 nonce，并对同一 session 的重复 Open 返回同一值。
2. App 向服务器提交 nonce、instance、generation、controller digest/权限/version；服务器检查 owner、
   TTL、CAS 与事务幂等后签 Grant。
3. App 发送 Mutation。endpoint 只接受拥有控制窗口的同一 transport/session。
4. 设备验证 Grant，先完成 deterministic receipt 的 proof/编码，再原子
   Install/Rotate/Revoke；任何回执准备失败都不得修改持久化状态。
5. App 验 receipt、执行 Method 2，并幂等 Commit；设备落盘不依赖 Commit 成功。
6. 同一加密 session 内保留控制窗口以支持 exact retry；断连或 Runtime stop 时清零
   nonce/workspace，新 session 必须取得新 nonce。

如果持久化已成功但响应发送失败，App 必须重放完全相同的 Grant/secret；store/coordinator 返回相同
receipt。不得更换 grantId、nonce 或版本。ownership generation 前进后，旧 Grant/controller/Commit
全部失效。

## 6. 资源冻结

- Core 不依赖 BLE、WiFi、HTTP、MQTT 或平台 SDK，可在 ESP32、UNO R4、Renesas/Wio 复用。
- coordinator 只常驻 nonce 与依赖引用；endpoint 只增加 session owner 元数据。
- signature/HMAC/canonical 操作复用 caller-owned 256 B workspace，结束后清零。
- 最大 Grant `193 B`；最大 transcript `156 B`；Receipt/Commit 上界仍为 `145/126 B`。
- 最大 BBP/2 Mutation body `231 B`，加 10 B base header 后为 `241 B`，可装入 Small profile 的
  256 B frame；不引入 controller 专用分片或第二套 reassembly 状态。
- Runtime 仅在设备 `maxFrameSize >= 241 B` 时发布该能力；App 的 HELLO `maxFrameSize` 至少为最大
  Receipt frame `155 B`，否则本 session 不协商 ControllerControl。容量检查必须发生在任何 durable
  mutation 之前。

## 7. P6 产品接入状态与剩余阻断

本批已关闭设备侧三项：

1. Coordinator 只依赖最小 `IAccessEpochSource`，设备不保存 logical ID、owner 或 Ownership graph；
2. ESP32 Security1 首次 transaction 通过 `DeviceAccessStore::bootstrap()` 一次原子安装 DeviceKey、
   accessEpoch 和 Admin；Runtime ControllerControl 处理后续 Shared 安装/轮换；
3. P6 固定 Admin/Shared 两槽，Shared 只能 Observe|Control，角色轮换不可升级权限，普通 mutation 不可删除 Admin。

ControllerControl endpoint 已注入正式 P6 composition，并使用 P-256 server key ring 验 Grant。P3 WiFi-only
不保存 Direct credential，因此不注入该 endpoint。正式 QuickStart、单 ZIP 安装编译和 link/resource 闭包已过；
仍需完成 App/Service codec 与事务、Cloud Frame Authentication 和真机。普通用户 API 不公开 controller secret、
Grant、workspace 或 transport 参数。

## 8. 验收证据

- `tests/host/test_controller_control_main.cpp`：Install/Rotate/Revoke、重放、secret/nonce/signature、
  缓冲重叠/不足且不落盘、Commit guard、endpoint session 独占与断连换 nonce；
- `tests/host/test_main.cpp`：BBP/2 最大 241 B frame、HELLO capability、未授权/未加密拒绝、
  ManageControllers 成功路径、sequence/response 与 stop 清理；
- `tests/golden/controller_control_golden_vectors.properties`：12 个 v2 exact 值；
- C++、App-compatible TypeScript、Server Node 三个实现独立消费同一 golden。
- `tests/host/test_device_access_store_main.cpp`：176 B exact blob、原子 bootstrap、Key/domain 独立清理、
  Admin/Shared 角色与故障恢复；
- `tests/host/test_device_key_provisioning_main.cpp`：P3 Install 与 P6 Bootstrap/refresh/掉电半写拒绝。
