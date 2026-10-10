# Device V2 ESP32 安全产品组合（Developer profile）

> **归档（2026-08-16）**：本文记录的 `Esp32SecureProductProbe`、P-256 Developer identity、
> Setup-PSK/Enrollment-v1 与旧 ProductOrchestrator 已从 active CI、发布清单和工作树产品探针删除。
> 2026-08-16 收口：永久 32 B PSK 的 `StoredSetupPskProvider` 源码和专属测试也已删除；仅保留可由未来
> Ticket/临时会话实现注入的 `ISetupPskProvider` seam。
> 文中的现在时均按历史实验结果阅读，不是当前实施合同。当前主线仅见
> `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 与 `DEVICE_V2_REFACTOR_PROGRESS.md`。
> R4b-4b 后普通 BLE/WiFiBLE 已冻结为共享 `NoiseNnSession` 的 NN 档且无需 Setup PSK；NNpsk0只属于
> 显式增强档，最新 wire 见 `DEVICE_V2_PORTABLE_NOISE_WIRE_CONTRACT.md`。

> 2026-08-15 纠偏：本文的 protected Setup PSK + P-256 Developer identity 是未发布的 Enhanced
> ESP32 实验 profile，不是普通 Developer 默认入口，也不得据此要求 UNO R4 或 BLE-only 都预置
> SetupCode。正式默认流程见 `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md`。

状态：2026-08-15 已完成实验软件静态门禁；未完成真机、App/Gateway/Broker E2E 和量产安全验收。

## 1. 本批结论

设备侧已有的 Core 与独立 adapter 可以组成一条完整且不依赖 legacy JSON/authKey 的 ESP32 路径：

```text
protected Setup PSK + P-256 Developer identity
                  |
NimBLE -> 4 B fragment -> Noise NNpsk0 -> local provisioning
                  |                         |
                  |                  Wi-Fi journal + durable claim
                  |                         |
                  +-> Direct BBP/2 <- ProductOrchestrator
                                            |
                            Wi-Fi -> TLS enrollment/commit
                                            |
                         TLS cloud session -> PubSubClient MQTT
```

真实 clean link 为 `1,321,659 / 1,572,864 B` Flash、`67,280 B` static RAM；双 1.5 MiB OTA
slot 剩余 `251,205 B / 15.97%`，通过 10% 余量门禁。它比预注册 Active、无真实 Noise/identity/NVS
的 `WifiBleProductProbe` 增加约 24.4 KiB Flash 和 5.0 KiB static RAM。

这证明方案可组成、可链接且资源可接受，不证明无线并发峰值、TLS heap、掉电物理特性或 24 h 稳定性。

## 2. 复用边界

平台无关、可直接复用于 UNO R4/Renesas 等 MCU 的部分：

- `StoredSetupPskProvider`：`IAtomicBlobStore + IRandom -> ISetupPskProvider`；
- `P256DeveloperIdentity`、`ProvisioningCoordinator`、`LocalProvisioningCoordinator`；
- `NoiseNnPsk0Session`、BLE secure-record、4 B BLE fragment、BBP/2；
- Wi-Fi 双槽 journal、Wi-Fi connection lifecycle、HTTP enrollment/cloud session；
- `ProductOrchestrator` 和 `Client`。

ESP32 独有的部分只在 adapter 包：NVS、mbedTLS P-256、mbedTLS X25519/AES-GCM、NimBLE、WiFi station、
ESP-IDF hardware RNG。其他 MCU 不复制协议状态机，只实现等价窄接口。

## 3. 四个持久域

| 域 | ESP32 probe namespace/key | 最大值 | 所有权与要求 |
|---|---|---:|---|
| Developer factory identity | `bl_v2_factory/dev_identity` | identity record | 软件 P-256 私钥；受保护存储；不是硬件抗克隆身份 |
| Setup PSK | `bl_v2_factory/setup_psk` | 32 B exact | 量产/开发工具预置；Sketch 无写入 API；不随重启过期 |
| Operational credential | `bl_v2_op/credential` | 148 B | claim/activation/commit 三阶段持久状态；ownership reset 可清除 |
| Network journal | `bl_v2_net/wifi_sel,wifi_0,wifi_1` | 16/116/116 B | candidate/confirmed、掉电 old-or-new 与一次 rollback |

Setup PSK 本体长期位于 Factory 域。`StoredSetupPskProvider::begin()`只用 32 B 栈空间验证 exact length、
非全零和存储保护级别，然后立即清零；对象常驻只保存当前随机 8 B `setupSessionId`。每个 setup window
只授权该 ID，关闭窗口或进入 Direct 后清除 ID，但不删除永久 PSK。因此设备重启后仍可重新生成新 session 并配网，
不存在“短期 token 过期后设备永远不能登录”的问题。

UNO R4 的 32-bit ABI 实测该 provider 为 `24 B`，低于 `32 B` 门限；同一探针中的
`WifiCredentialStore` 为 `28 B`、`ProductOrchestrator` 为 `40 B`。整个 SizeProbe 使用
`52,176 B` Flash、`12,676 B` static RAM，未把平台无关密钥边界换成大对象缓存。

## 4. NVS 保护语义已 fail-closed

旧实现允许调用方只设 `encrypted=true` 就让 `protection()`返回 `EncryptedFlash`，无法证明底层 NVS 真的加密。
这会错误放行 Setup PSK、Wi-Fi 和 identity，现已纠正：

1. `encrypted=true` 还必须提供 `Esp32NvsEncryptionVerifier`；
2. `begin()` 在打开 NVS 前调用 verifier，缺失或返回 false 均为 `NotConfigured`；
3. 只有验证通过且 NVS 打开成功后，`protection()`才报告 `EncryptedFlash`；`end()`恢复为未证明；
4. callback 只做证明，不负责开启加密。产品启动层仍必须真实初始化安全 NVS，并由 release/efuse/partition
   证据支持 callback，不能把 callback 写成无条件 true。

当前 compile probe 的 `BLINKER_PROBE_NVS_ATTESTED` 默认是 0，普通开发板启动会在任何 NVS 写入前失败，避免探针
被误当成量产程序。Setup PSK 也没有测试值或运行时 install 命令。

## 5. 启动顺序

正式组合必须保持以下顺序，失败时停止，不能 fallback 到 plain storage 或 JSON authKey：

1. 安全 NVS 初始化和产品 attestation；
2. identity blob `begin()`，再 `P256DeveloperIdentity::begin()`；
3. Setup PSK blob `begin()`，再 `StoredSetupPskProvider::begin()`；
4. operational blob 与 Wi-Fi journal `begin()`；
5. Direct BLE 以 `TransportLifecyclePolicy::External`加入 Client，cloud transport 由 Client 管理；
6. `ProductOrchestrator::start()`决定进入 Provisioning 或 Active/Direct；
7. loop 中只调用 `ProductOrchestrator::poll()`。

HTTP enrollment 与 cloud session 在生命周期上互斥，因此共享 320 B request 和 640 B response buffer；不为两阶段
重复常驻 960 B。Noise RX/TX/plaintext、local operation/response、Direct BLE RX/TX 仍由产品显式持有，便于按 MCU
profile 审计，不藏进 heap。

## 6. 身份等级不能混淆

本组合使用 `P256DeveloperIdentity`：首次在受保护存储中生成软件私钥，适合 DIY/教育和开发者产品。它可以防止网络上
伪造 claim proof，但私钥可被有物理提取能力的攻击者克隆，不能标记为 `ManufacturedHardware`。

商业 hardened SKU 必须另接工厂证书/secure element/eFuse 派生 signer，并让私钥不可导出。Core 的 claim、receipt、
enrollment 和 cloud contract 不变，只替换 `IFactoryIdentity` adapter。

## 7. 构建与 CI 门禁

当时的 `Esp32SecureProductProbe` 是完整静态对象图和资源探针，不是用户 QuickStart。历史 CI 曾独立编译：

- `esp32-preenrolled-product` / `uno-r4-preenrolled-product`：跨平台可比较的 pre-enrolled 连接性与运行态资源基线；
- `esp32-secure-product`：ESP32 Developer identity + protected storage + Noise provisioning 实验链。

以上三个 target 已删除；当前 CI 不再执行它们。

Arduino ESP32 core 会复制 sketch 自定义 `partitions.csv`，但其 size recipe 不自动更新板卡默认
`upload.maximum_size`。compile matrix 现显式设置 `1,572,864`，verifier 同时校验该属性必须等于 CSV 中的 OTA slot，
并继续执行 10% 余量、Small profile、NimBLE 宏和实际 adapter 依赖门禁，不能只改一个数字绕过检查。

## 8. 尚未退出的门禁与下一主线

1. 真机：安全 NVS 初始化/证明、预置 PSK、P-256/Noise runtime self-test、断电和 factory/ownership reset；
2. ESP32：Wi-Fi + NimBLE + TLS/MQTT 并发 heap/stack watermark、最长阻塞、断线重连与 24 h soak；
3. UNO R4/Renesas：实现或否决安全原子存储、P-256、X25519/AES-GCM 的量产 adapter；不能因 Core 可编译就宣称
   安全 BLE 配网；
4. App/Gateway/Broker：真实 scan+QR、claim proof receipt、activation/commit、cloud session、ACL/revocation 与
   BBP/2 首屏 E2E；
5. 先实现平台无关`Product + IProductLifecycle`，再把本对象图接成首个ESP32 integration proof；不建立
   ESP32公共facade。统一API、三种连接profile与UNO R4同入口门禁见
   `DEVICE_V2_UNIFIED_PRODUCT_API_AND_PLATFORM_COMPOSITION.md`。

因此当前可以发布 Core/adapter 开发预览，但仍不发布“开箱即联网”的正式 QuickStart。
