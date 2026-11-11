# Device V2 可移植安全配网决策

> 2026-08-17 新主线：本文继续作为 Security2 资源否决和 portable Noise 实验依据，但 Noise 已下沉为
> Enhanced，ESP32 初学者默认候选改为 WiFiProv Security1。Security1 尚无最终 composition 实测，不能从本文
> Security2 数据推导。当前执行以 `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md` 为准；本文中“ESP32 默认必须
> 复用 NimBLE Noise”“App 必须执行旧 LocalSetup saga”等冲突结论不再规范。

> 2026-08-15 变更提示：Noise secure channel 的底层结论仍有效；默认 Developer identity、单一 setup
> code 和三 profile 服务器注册流程已由 `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 取代。

> 2026-08-16 R4b-4b：本文 NNpsk0 流程只作为增强型历史依据。普通 DIY 已冻结为无需 PSK 的 Noise NN，
> suite/capability/握手尺寸与8 B locator/16 B session ID边界只以
> `DEVICE_V2_PORTABLE_NOISE_WIRE_CONTRACT.md` 为准；本文第4～6节不得用于生产实现。

> 状态：归档的历史实验记录，不是当前实施合同。旧 Enrollment Contract v1、`ClaimProofEndpoint`、
> `LocalProvisioningCoordinator`、setup PSK 公共入口及其源码均已删除；当前 Ownership/Cloud/WiFi outer
> 以 `DEVICE_V2_OWNERSHIP_CLAIM_CONTRACT.md`、`DEVICE_V2_CLOUD_CREDENTIAL_ENROLLMENT_CONTRACT.md`、
> `DEVICE_V2_WIFI_ENROLLMENT_KEY_HTTP_CONTRACT.md` 和 `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 为准。
> 本文只保留 Noise/BLE 安全通道取舍和资源实验依据。Arduino-ESP32 3.3.1 的
> `network_prov_mgr` BLE + Security 2 默认产品路径已被真实 Flash 门禁否决。本文取代此前
> “ESP32 默认接 WiFiProv Security 2、UNO R4 以后再另做 portable envelope”的顺序。
> 当前 LocalSetup application 与 R4b-4d durable saga 边界只以
> `DEVICE_V2_LOCAL_SETUP_APPLICATION_CONTRACT.md` 为准。
> 2026-08-14 后续状态：portable Noise Core、4 B local secure record、canonical application envelope、
> signed proof receipt gate与三语言exact vectors已实现；独立ESP32 mbedTLS provider已通过clean compile。
> BLE mode ownership/广告合同已通过设备侧软件门禁；UNO R4/Renesas 软件 crypto provider 后续也已完成。
> 真机 runtime 与正式 product composition 仍是发布阻断项。

## 1. 历史实验结论及保留边界

1. ESP32 默认配网与 Direct BLE 必须复用同一个 NimBLE-Arduino host，不能再并列链接
   Arduino-ESP32 预编译 Bluedroid provisioning stack。
2. Enhanced/Manufactured PSK profile 的本地安全会话采用标准 Noise pattern，而不是 Blinker 自创握手：
   当时的实验候选固定为
   `Noise_NNpsk0_25519_AESGCM_SHA256`。PSK 固定 32 B，来自高熵 QR/NFC setup secret；
   claim、WiFi credential 和状态消息只允许在 Noise handshake 完成后的 transport message 中发送。
3. 该 PSK profile 只支持不可预测的 32 B QR/NFC setup PSK。短数字/10～12 位手输码不能简单
   hash 成 Noise PSK，否则抓包后可离线穷举；手输 profile 必须另接 SPAKE2+/等价 PAKE 并通过
   独立 vectors、资源和审计门禁。在此前 UI 中不得显示“生产安全手输配网”。
4. 旧 Enrollment Contract v1 已撤回；其中 App 搬运 exact canonical bytes、设备验证 server-signed
   receipt 后才开放 WiFi sink 的安全性质仍保留。原实验流程为 App 把 `ClaimChallenge` 放入安全 channel，Device Core
   返回canonical `ClaimProof`；Gateway验证后签发绑定setup session和exact proof digest的
   `ClaimProofVerificationReceipt`。设备验server signature后才开放WiFi sink；App布尔值无授权效力。
   Noise只保护本地链路，不替代factory identity、server claim或TLS activation。
5. WiFi credential success 仍不等于设备 Active。正式流程必须分别完成 Ownership、CloudCredential 和
   可选 Controller commit，原子持久化后才停止 Provisioning endpoint 并切换 Direct BLE。当前实验基线复用同一物理 GATT UUID和
   同一 adapter/host stack，但会完整停止旧 BLE link、断开 central、按新 Service Data profile重新启动；
   App必须重新扫描连接。不得宣称运行中无断线“原地改service”，也不得偷偷重启整个MCU。
6. Espressif Security 2 可在未来作为“大 Flash、明确无双 OTA或非原始 ESP32”的可选 profile重新测量，
   但当前不进入 release manifest、QuickStart、默认依赖或 CI 成功矩阵。

Noise Framework明确规定 PSK mode使用32 B共享密钥，并将PSK混入handshake key与hash；
`NNpsk0`的标准pattern是`-> psk,e`、`<- e,ee`。AES-GCM提供AEAD，SHA-256承担hash/HKDF。
本项目不缩写、改序或自定义该pattern。

## 2. 否决 Espressif Security 2 默认路径的证据

真实 probe 只包含 `BlinkerDeviceV2 + network_prov_mgr BLE + Security 2 + custom endpoint`，尚未加入：

- ESP32 P-256 Developer identity/storage；
- HTTP/TLS activation；
- PubSubClient/MQTT；
- NimBLE Direct BLE；
- 最终产品 composition。

Arduino CLI 使用 Arduino-ESP32 `3.3.1` / ESP-IDF `5.5` clean build，结果为：

| 项 | 结果 |
|---|---:|
| Flash | 1,682,475 B |
| static RAM | 58,052 B |
| 默认 app slot | 1,310,720 B，超出 371,755 B |
| 当前双 OTA app slot | 1,572,864 B，仍超出 109,611 B |
| `.flash.text` | 1,301,912 B |
| `.flash.rodata` | 238,028 B |
| `.iram0.text` | 116,047 B |

ELF/map 显示被拉入的是预编译 Bluedroid/Classic BT、protocomm protobuf、SRP/SHA-512和完整
network provisioning manager 等大块符号。Core 的数百字节 buffer 或 descriptor 优化不可能回收
109～372 KiB，更不可能再容纳现有 1,277,391 B 的 cloud + NimBLE 产品组合。通过 Huge APP、取消
双 OTA 或只改 board percentage 会掩盖问题，因此被禁止作为修复。

API 审计还确认 Arduino `WiFiProvClass` 的 `beginProvision(..., const char *pop, ...)` 把该指针原样作为
security params传给manager；公开类型没有安全地表达Security 2所需的
`network_prov_security2_params_t {salt, salt_len, verifier, verifier_len}`，也不暴露custom endpoint的
create-before-start/register-after-start生命周期。高级代码可以把结构体指针强制转换成`const char *`，
并在`initProvision()`前后混用底层manager API绕过这些限制，因此“完全不能使用”是不准确的；准确结论是
它不能作为本项目类型安全、完整且跨平台的公开适配边界。最终否决默认路径的决定性证据仍是完整架构耦合和
上述真实资源门禁。

## 3. 已落地的平台无关部分

新增 `IProvisioningClaimEndpoint` 与 `ClaimProofEndpoint`：

- request只接受 canonical Enrollment Contract v1 `ClaimChallenge`；
- response只产生 canonical `ClaimProof`；
- 设备内部先编码`blinker.claim-proof.v1` transcript，再 SHA-256并调用`IFactoryIdentity::prove()`；
- 不接受任意 digest或任意待签bytes，因此不是通用签名 oracle；
- 不保存claim ticket、setup secret、WiFi credential或业务状态；
- caller-owned workspace，无常驻大 buffer、无 heap。

固定结构上限：

| buffer | 上限 |
|---|---:|
| ClaimChallenge request | 332 B |
| claim transcript | 258 B |
| ClaimProof response | 233 B |

Host tests覆盖最大256 B ticket、64 B physical ID、对称HMAC identity、Developer public-key identity、
malformed/oversize/undersized workspace和独立proof重算。

## 4. 新配网流程

```text
Factory/first boot
  -> 广播统一Device V2 service UUID + Provisioning mode/wire/capability + 8 B setupSessionId
App scan QR/NFC
  -> 得到 physicalDeviceId/factoryClaimRecordId/32 B setup PSK/套件
App read BLE advertisement/session
  -> 得到本次随机 setupSessionId，并与所选 physicalDeviceId 绑定
App -> Gateway
  -> ClaimStart，取得 canonical ClaimChallenge
App <-> Device over one NimBLE GATT
  -> Noise_NNpsk0_25519_AESGCM_SHA256
  -> encrypted ClaimChallenge / ClaimProof
App -> Gateway
  -> 原样 relay ClaimProof，接收signed ProofVerificationReceipt
App -> Device over Noise transport
  -> relay signed receipt，设备验签并打开proof_verified gate
  -> encrypted WiFiConfig -> atomic platform sink
Device -> Gateway over TLS
  -> Ownership commit -> CloudCredential enroll/commit -> Active
Device
  -> 停止Provisioning endpoint并断开BLE，切换广告为Direct BBP/2后等待App重连；Cloud MQTT可同时运行
```

BLE MAC、广播名和setupSessionId都只是locator。广播不得出现setup PSK、claim ticket、physical
bootstrap proof或operational credential。优先使用自有128-bit provisioning service UUID的Service Data
承载version/capability/setupSessionId，不能滥用未分配的Manufacturer Company ID。

## 5. QR / setup 语义（QR wire 尚待 golden）

QR semantic fields先冻结，具体canonical CBOR编号必须与App reference golden一起落地后才称wire frozen：

| 字段 | 约束 |
|---|---|
| version | `2` |
| factoryClaimRecordId | 服务器 factory registry 的公开引用；由其解析 productModelId |
| physicalDeviceId | 1..64 UTF-8 bytes |
| setupSessionId | 不在静态 QR；8 B，设备每次打开 claim window重新生成并只从实时广播/动态码取得 |
| transport | BLE capability bit；MAC不在身份字段中 |
| securitySuite | `Noise_NNpsk0_25519_AESGCM_SHA256`的固定numeric ID |
| setupPsk | 32 B CSPRNG输出，只在QR/NFC和设备Factory域存在 |

设备Factory域保存setup PSK，量产ESP32必须受Flash/NVS Encryption + Secure Boot保护；它与factory
signing identity、operational credential分域。服务器正常claim不需要也不得收集raw setup PSK。App只在
claim coordinator内存中短暂持有，不能进入Ionic storage、analytics、clipboard、crash report或日志。

## 6. 设备实现分层

```text
BlinkerDeviceV2 Core
  ClaimProofEndpoint
  NoiseNnPsk0Session + LocalSecureRecord (已完成Core/exact vectors)
  LocalProvisioningCoordinator + canonical application envelope (已完成)
  ClaimProofVerificationReceiptVerifier (已完成)
  IWifiCredentialSink + portable two-slot journal (已完成)
  ESP32 three-key NVS sink (软件门禁已完成；真机断电待测)
        |
portable Noise crypto seam
  X25519 + AES-256-GCM + SHA-256/HMAC/HKDF + CSPRNG
        |
platform adapters
  ESP32 mbedTLS/esp RNG (compile-gated，真机待测) + existing esp32-nimble
  UNO R4 audited provider + arduino-ble (资源通过后)
        |
ProductComposition
  provisioning / activation / cloud / direct BLE lifecycle
```

Core只实现Noise规定的state/wire和bounded buffers，具体原语由窄crypto provider注入；不得把
NimBLE、ArduinoBLE、mbedTLS或MCU header放回Core。也不得复制一份“ESP32 Noise状态机”和一份
“Renesas Noise状态机”。平台只补crypto、WiFi credential apply/storage和GATT adapter。

## 7. 已完成与下一批退出条件

已完成：suite ID `1`、固定prologue、4 B record、handshake/transport nonce/close/error规则；
C++ production Core、App-compatible TypeScript与独立Node/OpenSSL对同一official + Blinker vectors
逐byte一致；Host覆盖wrong PSK、AEAD tag、replay、乱序、重复handshake与容量重试。exact合同见
`DEVICE_V2_PORTABLE_NOISE_WIRE_CONTRACT.md`。

此前批次完成：`LocalProvisioningContract`、`LocalProvisioningCoordinator`、
`ClaimProofVerificationReceiptVerifier`与`IWifiCredentialSink`边界；同一requestId仅exact request可重放，
ClaimProof使用caller-owned 233 B cache保证逐byte响应，WiFi credential不进入常驻对象。Enrollment三语言
golden现为25项，含signed receipt、6项local envelope/body及Provisioning/Direct的Service Data/完整scan response。

本批又完成同一物理BLE link的显式mode ownership：`BleRecordTransport`只在成功`start()`后拥有callbacks，
未启动实例的`stop()`不会拆掉当前owner；`IBleModeLink`只在Stopped状态接受版本化广告profile；
`BleModeCoordinator`执行校验、幂等、停止、切换和失败回滚；`IBleModePreparation`在广告启动前及回滚时
重新武装Noise短期状态，不让setup PSK进入协调器。三种BLE adapter共用13 B Service Data编码与统一GATT UUID。
完整合同见`DEVICE_V2_BLE_MODE_OWNERSHIP_AND_ADVERTISING.md`。

后续仍须：

1. 无分配BLE application endpoint接通Noise handshake/transport与`LocalProvisioningCoordinator`；
2. WiFi connection lifecycle与TLS activation/commit driver接通`ProvisioningCoordinator`；
3. ESP32量产factory setup PSK provider、encrypted partition证明，以及真实X25519/AES-GCM provider的
   invalid key/tag/runtime vector和secret清零真机门禁；
4. UNO R4/Renesas补平台atomic blob和真实Noise provider；portable journal本身已复用，不重复写协议；
5. ESP32完整`cloud + provisioning + Direct NimBLE + identity/store`满足1.5 MiB双OTA且至少10% headroom；
6. UNO R4完整组合满足32 KiB SRAM与stack watermark，未通过前不宣称安全BLE配网；
7. 实机验证Provisioning -> Direct的断开/重扫/重连、同profile restart/re-arm、WiFi新旧配置rollback、
   逐写点断电、24 h并发与secret日志扫描；
8. App生产scanner严格消费13 B mode profile；Gateway完成signed receipt/activation事务；
9. 以上完成前，设备Core只是wire/primitive freeze candidate而非“现网已可用”，官方QuickStart不得提前发布。

## 8. App / Gateway 修订

App删除“必须接Espressif Security 2 native bridge”的要求，改为共享Noise suite：Android/iOS可以使用
审计过的native实现，但必须由共享vectors证明同一wire。App仍负责QR、claim coordinator、不可信relay、
WiFi输入和严格secret hygiene。

Gateway的Enrollment Contract、P-256/HMAC factory proof、activation/commit事务和Broker边界不因本地
Noise改变。Gateway不解Noise、不收setup PSK；setupSessionId现为claim与signed receipt的强制绑定字段。
Gateway只在验证exact ClaimProof后允许claim进入`proof_verified`，并返回签名receipt，不再返回204后依赖
App自报布尔状态。

参考规范：

- Noise Protocol Framework: https://noiseprotocol.org/noise.html
- SPAKE2（短密码profile候选）: https://datatracker.ietf.org/doc/html/rfc9382
- SPAKE2+（verifier型短密码profile候选）: https://datatracker.ietf.org/doc/html/rfc9383
