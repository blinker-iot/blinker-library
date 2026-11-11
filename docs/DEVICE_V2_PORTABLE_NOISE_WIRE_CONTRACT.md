# Device V2 portable Noise wire contract v1

> 状态：2026-08-16 R4b-4b Core wire 已冻结。普通 DIY 使用
> `Noise_NN_25519_AESGCM_SHA256`；Enhanced/Manufactured 可由产品 composition 静态选择
> `Noise_NNpsk0_25519_AESGCM_SHA256`。两者共用一份 allocation-free `NoiseNnSession` 状态机，
> 旧的固定 `NoiseNnPsk0Session` 源码已经删除。设备 C++、App-compatible TypeScript 与独立
> Node/OpenSSL 的普通 NN exact vector 已一致。R4b-4c 已完成 BLE LocalSetup application envelope、
> EnrollmentTicket 接受流程和最终 handshake-hash callback；R4b-4d 已完成 durable saga。
> official BLE/WiFi+BLE composition 和真机门禁仍未完成。

## 1. 角色与 suite

- App 是 Initiator，设备是 Responder；不允许角色互换。
- suite 由受信的产品 composition 在开始广播前固定。连接内没有 suite negotiation、自动探测或降级。
- 普通 DIY：suite ID `1`，协议名精确为 `Noise_NN_25519_AESGCM_SHA256`，不需要 SetupCode/PSK。
- Enhanced/Manufactured：suite ID `2`，协议名精确为
  `Noise_NNpsk0_25519_AESGCM_SHA256`，要求 32 B 非零高熵 Setup PSK。
- 6 位数字、短字符串及其直接 hash 不得作为 NNpsk0 PSK；需要手输密码时必须另行引入 PAKE。
- MCU 只替换 `INoiseCryptoProvider` 和 `IRandom`；不得为 ESP32、Renesas 或 App 复制第二套状态机。

普通 NN 提供前向保密的链路加密，但单独使用时不认证“眼前设备就是目标设备”，也不能抵抗主动设备冒充。
普通流程必须把握手 hash 绑定进服务器签名的 `EnrollmentTicket`，并在 Ticket 验证、session 升级完成前禁止
WiFi credential、Ownership mutation 和 Controller 安装。需要配网前即认证设备的产品应使用高熵 PSK 档或
后续经独立评审的设备身份/PAKE档，不能把 Ticket、MAC 或短码临时充当 Noise PSK。

## 2. prologue

固定 ASCII 前缀为 `BLINKER-LOCAL-SETUP-V1`，随后追加：

```text
recordMagic(0xB3) || recordVersion(1) || suiteId
```

| pattern | 完整 prologue hex |
|---|---|
| NN | `424c494e4b45522d4c4f43414c2d53455455502d5631b30101` |
| NNpsk0 | `424c494e4b45522d4c4f43414c2d53455455502d5631b30102` |

Core API 必须显式传入 pattern 和对应 prologue。pattern、广告 capability 和 channel config 任一不一致时，
设备在广告启动前或 session arm 阶段失败，不能接受 peer 指定另一 suite。

## 3. BLE mode capability 与 setup 标识

Provisioning mode capability 位：

| bit | 含义 |
|---:|---|
| 0 | 4 B fragmented records |
| 1 | ordinary Noise NN |
| 2 | EnrollmentTicket v1 |
| 3 | WiFiConfig v1 |
| 4 | Direct BBP/2 |
| 5 | enhanced Noise NNpsk0 |

Provisioning profile 必须且只能设置 bit 1/bit 5 之一。普通 WiFi+BLE profile 为 `0x000f`；增强型
WiFi+BLE profile 为 `0x002d`；Direct BBP/2 保持 `0x0011`。未知 bit、同时设置两种 Noise、两者均未设置
或 Direct 携带 provisioning capability 都必须拒绝。

广播尾部的 8 B 已正式命名为 `setupSessionLocator`，只用于扫描缓存去重和定位当前 setup window；它不是
`EnrollmentTicket` 中的 16 B `setupSessionId`。完整 16 B ID 由 application coordinator 持有并在受保护的
握手/application payload 中交给 App；Ticket 只能绑定完整 ID。禁止把 8 B locator 填充、复制或补零后冒充
完整 ID。

## 4. local secure record

每个 logical record 使用 4 B big-endian header：

```text
byte 0     0xB3
byte 1     version[7:4] | type[3:0]
byte 2..3  bodyLength uint16 big-endian
byte 4..   body
```

| type | 值 | body |
|---|---:|---|
| InitiatorHandshake | 1 | App 第一条完整 Noise handshake message |
| ResponderHandshake | 2 | 设备第二条完整 Noise handshake message |
| Transport | 3 | 一条完整 Noise transport ciphertext + 16 B tag |

`bodyLength` 必须为 `1..1024`，总 record 最大 1028 B。未知 type/version、空 body、截断、尾随字节、
乱序、重复 handshake 或提前 transport 都 fail closed。record 再由通用 4 B `B2` BLE fragment 层分片；
产品 mode 明确决定 B3 secure record 或 Direct BBP/2，不按 magic 猜协议。

## 5. handshake 与 transport

标准 pattern：

```text
NN:      -> e
         <- e, ee

NNpsk0: -> psk, e
         <- e, ee
```

消息布局：

| pattern/message | body | 最小长度 |
|---|---|---:|
| NN initiator | `e[32] || clear payload` | 32 B |
| NN responder | `e[32] || encrypted payload || tag[16]` | 48 B |
| NNpsk0 both | `e[32] || encrypted payload || tag[16]` | 48 B |

portable BLE channel 统一按最坏开销限制 handshake payload 为 976 B，避免为首包增加第二套容量模型。
transport body 为 `ciphertext || tag[16]`，plaintext 最大 1008 B。每个 record 只承载一条完整上层 envelope；
不得在解密前解析业务字段，也不得把半条 CBOR 当成独立 Noise transport message。

AES-GCM nonce 为 `00000000 || uint64_big_endian`。每个 CipherState 从 0 开始，只在加解密成功后递增；
nonce 达到 `UINT64_MAX` 前关闭 session。`BufferTooSmall` 不推进 transcript/nonce，允许 caller 扩容重试；
认证失败、provider 错误、非法 X25519、顺序错误、超时和断连清零会话并进入失败/未武装状态。

## 6. 内存与 secret 生命周期

- `NoiseNnSession` 当前 UNO R4 32-bit ABI 为 272 B、64-bit Host ABI 为 280 B，并同时支持两种
  pattern；没有 heap。
- `BleNoiseProvisioningChannel` 当前 UNO R4 32-bit ABI 为 520 B，`BleNoiseModePreparation` 为
  16 B；SizeProbe 对三者分别实体化以锁定回归，因此其总 RAM 不能直接当作产品对象图常驻值。
- secure channel caller buffer 固定为 RX 1028 B + TX 1028 B + plaintext 1008 B + ATT scratch；密文直接在
  TX 槽构造，不再增加第三个 1028 B record buffer。
- 普通 NN 不读取或保存 PSK。增强档通过窄 `ISetupPskProvider` 按 8 B locator 短暂读取 32 B secret，arm 后
  立即清理 stack scratch；provider 不再包含从未被调用的 open/close lifecycle 方法。
- ephemeral private key、DH output、handshake/transport key 不持久化、不记录日志，并在 split/fault/stop/
  disconnect 时清零。
- `BleNoiseProvisioningChannel` 的 pattern 是 construction config，不接受 peer runtime 参数。

## 7. 当前实现与验证

- `protocol/noise/NoiseSuite.h`：两个固定 suite ID；
- `security/NoiseNn.*`：唯一共享状态机；
- `protocol/noise/LocalSecureRecord.*`：B3 record 与按 suite 固定 prologue；
- `protocol/ble/Mode.*`：互斥 capability、8 B locator；
- `BleNoiseProvisioningChannel`：固定 pattern、准确的首/次握手消息尺寸，并在 split 后借出最终 hash；
- `BleNoiseModePreparation`：普通构造不需要 PSK provider，增强构造才注入 provider。
- `BleLocalProvisioningEndpoint`：只有 responder handshake 确实进入 TX 槽后才把最终 hash 交给
  `LocalSetupApplication`；WouldBlock 重试不会提前升级 application session。

Host 覆盖 NN 首包无 tag、双向握手/transport/hash、pattern/PSK 参数拒绝、NNpsk0 wrong-PSK/tamper/replay、
BLE 背压/顺序/超时/断连、最终 hash callback 及 profile 不匹配。跨语言 ordinary NN golden 锁定
prologue、两条 handshake、handshake hash、双向 transport 和 B3 records；Node reference 另外继续验证公开
NNpsk0 Cacophony vector。R4b-4d 最终门禁为 Windows ASan 36/36、200,000 次 deterministic fuzz 和三端
LocalSetup/Noise golden。UNO R4 Core 1.5.3 Small SizeProbe 为 52,324 B Flash / 14,668 B global RAM；其中
probe 故意同时常驻 LocalSetup application/saga 测量数组、272 B session、520 B channel（内部已含一份
session）和 16 B preparation 尺寸数组，不能把该数值解释成产品运行峰值。
实际 SDK 编译还覆盖 ESP32 Core 3.3.1 mbedTLS crypto + ordinary NN（357,479 B / 20,816 B）、
Wio Terminal portable X25519/AES-GCM/Ed25519（44,316 B / 3,612 B `.data+bss`）与 UNO R4
SoftwareATSE P-256 verifier（57,196 B / 7,564 B）；这些是适配器/known-answer sketch，不是完整产品资源。

尚未完成：

1. official ESP32/UNO R4 BLE-only、WiFi+BLE composition；
2. WiFi candidate connect/confirm/错误网络恢复的产品 lifecycle；
3. 真机 radio、crypto known-answer、stack/heap watermark、断连/背压/WiFi 共存与长稳。

规范依据：[Noise Protocol Framework](https://noiseprotocol.org/noise.html)。
