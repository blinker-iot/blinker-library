# Device V2 Controller HMAC-SHA256 鉴权 Method 2

> 状态：2026-08-30 exact transcript、Device C++ responder、Edge Hub C++ initiator、App TypeScript、Server Node
> 三端 golden、DirectSecureRecord KDF/AEAD 以及 ESP32/UNO R4 真机路径均已完成。Platform Direct Controller 的安装、
> 轮换和撤销见 `DEVICE_V2_CONTROLLER_CONTROL_CONTRACT.md`。

## 1. 边界

- method ID 为 `2`；旧的全局 authKey/PSK method 1 已删除，不参与兼容；
- 默认策略仍要求调用方已经处于加密 transport；BLE Direct composition 显式选择
  `EstablishDirectSecure` 策略时是唯一例外：允许在普通 GATT 上执行 pre-auth Method 2，但只有最终 proof
  成功、DirectSecure key material 已准备且 `AuthResult(Authorized)` 已进入发送队列后才原子激活安全数据面；
- `AuthResult(Authorized)` 是最后一个裸 BBP/2 frame；之后所有 frame 必须是 DirectSecureRecord。普通 GATT
  不是安全声明，Noise/Method 2/DirectSecureRecord 才是；原生 BLE link encryption 只可作为纵深防御；
- Platform Direct 的 `controllerId` 表示当前设备 Admin/Shared access group；LocalBLE 才表示物理配对 local controller；
- App 把获得的 secret 存在 Keychain/Keystore；当前设备图只使用 260 B DeviceAccessStore v3（兼容读 176 B v1/196 B v2），容纳 Admin 与两个 Observe|Control controller；旧 standalone BCT2 不属于发布图；
  服务器可 KMS 保存每设备 Shared，Admin 不上传；
- `Local` controller 固定 `ownershipGeneration=0`；Platform 字段必须绑定非零 accessEpoch（wire 字段名暂保留）；
- Runtime 只接受设备存储记录中的权限，App 不能在鉴权请求里自报或扩大权限。

## 2. BBP/2 外层

外层继续使用 canonical CBOR：

```text
Authenticate {0: method=2, 1: payload:bstr}
AuthResult   {0: method=2, 1: status, 2: payload:bstr?}
```

方法内 payload 使用固定二进制布局，避免在 ATT 小包内再嵌套 CBOR：

| 阶段 | 长度 | 精确布局 |
|---|---:|---|
| App Init | 42 B | `01 || controllerId[16] || domain:u8 || generation:u32be || credentialVersion:u32be || clientNonce[16]` |
| Device Challenge | 53 B | `81 || deviceNonce[16] || permissions:u32be || deviceProof[32]` |
| App Proof | 33 B | `02 || appProof[32]` |

两端 nonce 均为 16 B CSPRNG 且禁止全零。同一 session 对完全相同 Init 重发相同 challenge，不重新
消耗随机数；同一未完成 session 改变 selector 或 nonce 会被拒绝。Proof 无论成功或失败都消费 challenge，
因此不能重放。

## 3. exact HMAC transcript

`deviceProof` 和 `appProof` 使用不同 ASCII domain：

```text
BLINKER-CONTROLLER-DEVICE-PROOF/1
BLINKER-CONTROLLER-APP-PROOF/1
```

domain 后不插入长度或分隔符，依次拼接：

```text
transcriptVersion:u8        = 1
method:u16be                = 2
suite:u8                    = 1  // HmacSha256_32
controllerId                = 16 B
domain:u8
ownershipGeneration:u32be
credentialVersion:u32be
permissions:u32be
clientNonce                 = 16 B
deviceNonce                 = 16 B
```

```text
proof = HMAC-SHA256(controllerSecret, roleDomain || transcript)
```

设备生成 challenge 和验证 App proof 时都重新通过 `IControllerCredentialSource::loadActive()` 读取
准确的 controller/domain/generation/version；权限来自该记录。challenge 与 proof 之间若发生轮换、
撤销或权限变化，本次认证立即失败。

## 4. Runtime 权限

`IAuthorizationProvider` 返回 `AuthorizationDecision`，只有 `Authorized` 可以携带非零、合法的权限位。
Runtime 将权限保存在对应 transport/session，不跨 BLE 与 cloud 会话共享：

| 权限 | 当前 Runtime 行为 |
|---|---|
| `Observe` | 允许 Manifest、ManifestAccept、State snapshot，以及接收设备 Patch/Event |
| `Control` | 允许向设备发送 Command、Apply Patch、Event |
| `ProvisionNetwork` | 预留给 R4b-4 配网 coordinator，不授权普通数据面写入 |
| `ManageControllers` | 授权进入 controller control-plane，不授权普通数据面写入 |

已经由非本地 transport 验证的 cloud session 保持原行为。本地业务 frame 必须同时满足
`encrypted=true` 与相应 application permission；这里的 encrypted 可来自平台可信链路，也可来自已经成功
验签解密的 DirectSecureRecord。transport 不得在 Method 2 前伪造安全位。旧的裸
`AuthorizationHandler` 双入口已经删除，避免绕过 provider 生命周期或丢失权限信息。

## 5. 三端验收

冻结文件：

- `tests/golden/controller_auth_golden_vectors.properties`；
- `controller_auth_golden_main.cpp`：调用生产 C++ HMAC 实现；
- `app_controller_auth_golden.ts`：浏览器安全的独立 TypeScript SHA-256/HMAC；
- `server_controller_auth_golden.cjs`：Node `crypto` 独立实现。

golden 固定 Init、两份完整 transcript、两份 proof、Challenge 和 Proof payload。Provider/transport Host
测试另外覆盖重复 Init、session 绑定、默认未加密拒绝、显式 DirectSecure bootstrap、凭据缺失、存储错误、
challenge 中途轮换、proof replay、最终裸 frame、AEAD/tag/direction/sequence/replay 和验证后安全上下文。

## 6. 与控制面的关系及剩余范围

`ControllerGrant/receipt/commit` exact CBOR、Ownership generation/version CAS 以及设备侧
Install/Rotate/Revoke coordinator 已完成软件基线，见 `DEVICE_V2_CONTROLLER_CONTROL_CONTRACT.md`。
当前已完成 Platform BLE 的 Noise enrollment、Method 2、DirectSecureRecord、Manifest/State/Command；
Gateway 主动端 Method 2/DirectSecure 安全核心已经完成 Host 互操作，仍未完成的是 Gateway GATT Central/route、
Product/OOB verifier、平台长稳/恶意断连，以及 LocalBLE 产品是否继续
保留独立 physical-confirm profile 的最终发布决策。任何后续 bearer 都必须复用本 Method 2 与同一
DirectSecureRecord，不得重新引入 method 1、全局 authKey 或裸 post-auth BBP/2。
