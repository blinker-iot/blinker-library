# Device V2 UNO R4 / Renesas 安全能力准入决策

状态：2026-08-18，基于 `arduino:renesas_uno@1.5.3`、ArduinoBLE 2.0.2 的源码、链接与资源实测修订；真机门禁尚未执行。
UNO R4 产品矩阵现只包含独立普通 WiFi 与独立普通 BLE；历史 WiFi+BLE 图已删除，本文相关资源只作算法/
架构证据，不再构成双无线产品准入计划。

## 1. 修订结论

旧结论“UNO R4 没有可用 P-256/X25519/AES-GCM provider”不再准确。已经验证过两条可链接路径：

- `BlinkerArduinoCryptoAdapter` 使用固定版本 `Crypto@0.4.0`，在 RA4M1 上提供 X25519、
  AES-256-GCM 和 Ed25519 server digest 验签；
- UNO R4 WiFi core 自带的 `SoftwareATSE` 可提供 P-256 server digest 验签。对应实验 adapter 因没有当前
  产品消费者已从发布源码、manifest 与 CI 删除；该结果只保留为 SDK 能力证据。

因此 UNO R4 普通 BLE 的当前产品只依赖 portable `Crypto@0.4.0`，不携带 P-256/SoftwareATSE adapter；它仍被真机 known-answer test、运行期
stack/heap、掉电和密钥静态保护阻断，不能据编译结果宣传为量产安全产品。WiFiS3/ArduinoBLE 并发已退出范围。

## 2. SDK 能力与边界

| 能力 | 当前证据 | 决策 |
|---|---|---|
| CSPRNG | RA4M1 SCE RNG 已由现有 adapter 实际链接 | 保留 `PlatformHardwareRandom`；真机故障/重复测试待做 |
| X25519 | RA4M1 FSP/WiFiS3 无公开原生接口；`Crypto@0.4.0` 软件实现已链接 | 可作为 portable fallback，不建立 UNO 专属 wire |
| AES-256-GCM | FSP 公共 wrapper 仍不可依赖；`Crypto@0.4.0` 软件实现已链接 | 不调用 FSP 私有 `*Sub` ABI |
| Ed25519 验签 | `Crypto@0.4.0` 已通过固定 server-vector 编译探针 | portable 默认候选；仅暴露验签，不使用其 RNG/签名 API |
| P-256 验签 | connectivity module 的 `SoftwareATSE::ecdsaVerify` 曾完成独立链接验证 | 当前无产品消费者，不发布对应 adapter；未来 profile 若需要必须重新准入生命周期和资源 |
| Data Flash/Preferences | 当前均不能证明密钥不可提取；Preferences journal 可提供应用层双槽恢复 | 如实报告 `PlainFlash`，不称为 Encrypted/Hardware-backed |
| WiFiS3 TLS | 可提供 TLS client | TLS 可选；不能替代 body 验签、Noise 或静态密钥保护 |
| BLE 当前连接加密 | ArduinoBLE 内部 ATT 按 connection handle 保存 controller encryption-complete 位；公共 `BLE.paired()` 仅是 any-peer | 固定 2.0.2，由独立 Renesas 窄 adapter 读取 exact handle；版本升级必须重审 |
| BLE bonding | 没有可依赖的 per-connection 公共事实 | 报告 `bonded=false`，不猜测；Controller Method 2 独立授权 |

RA4M1 FSP archive 缺少可依赖的完整公开 P-256/X25519/AES-GCM 组合这一点没有改变。修订点是：
协议所需能力不必全部来自 RA4M1 FSP；可移植软件实现和 connectivity-module 服务都可通过窄接口接入。

## 3. 当前实现选择

默认候选保持最小组合：

1. `PlatformHardwareRandom` 负责所有设备端熵；
2. `ArduinoCryptoProvider` 同时负责 Cloud envelope、BLE Noise 与 Ed25519 server Grant 验签；
3. server key ring 只借用 Flash/const public-key descriptor，不复制长期 key；
4. WiFi、BLE、storage、MQTT 与 crypto 仍是独立 adapter，不进入 Core，也不改变 `<Blinker.h>` 用户 API。

`RenesasUnoP256Verifier` 是可选产品策略：当服务器产品 profile 固定 P-256 时可利用 connectivity module，
但它会引入 `SoftwareATSE`/WiFiS3 服务生命周期和额外全局资源，不应无条件链接到所有 UNO R4 Sketch。

服务器不得让未认证请求自行选择验签算法。算法和 `keyId` 由服务器的 product/platform policy 决定，
设备按固件内 pinned key ring 严格校验。Ed25519 和 P-256 是同一协议字段的两个已分配值，不是两套
UNO/ESP32 私有协议；Cloud envelope 与 Noise v1 仍固定 X25519/HKDF-SHA256/AES-256-GCM。

## 4. 当前资源证据

以下是完整探针链接后的总量，不是 adapter 的净增量：

| UNO R4 WiFi probe | Flash | Global RAM | 已覆盖 |
|---|---:|---:|---|
| portable crypto | 65,060 B | 6,932 B | X25519、AES-256-GCM、Ed25519 verify vectors |
| native server verifier | 57,220 B | 7,564 B | `SoftwareATSE` P-256 verify vector |
| 历史 full WiFi+BLE composition | 211,332 B | 18,812 B | 已删除；仅用于资源变化对照，不是发布候选 |
| 当前普通 BLE package | 193,440 B | 16,896 B | Platform BLE、Direct BBP/2、exact-handle security；无 WiFi/HTTP/MQTT |

portable probe 相比尚未链接 Ed25519 验签的同一探针增加 6,404 B Flash、4 B global RAM。这个结果不能
代表调用时 stack 峰值；依赖文档也明确其曲线运算需要较大 stack，因此必须在真实组合中测 watermark。

## 5. 安全与发布门禁

公开发布 UNO R4 普通 BLE 前必须同时完成：

1. UNO R4 真机运行 X25519、AES-GCM、Ed25519/P-256 valid、mutated、non-canonical vectors；
2. 验证 AES-GCM 失败时输出被清零，低序/异常公钥、错误 tag 和错误签名均 fail closed；
3. 测量 ArduinoBLE + BLE crypto/Controller 组合的 stack/heap watermark；WiFi 单独按其产品包验证；
4. 对 connectivity firmware 版本、`SoftwareATSE` 生命周期和失败恢复做兼容测试；
5. 对 Preferences 双槽 journal 做真机掉电 old-or-new 矩阵；
6. 完成 App relay、Gateway signing/encryption key ring 和设备 exact-wire E2E；
7. 对固定的第三方 crypto 版本、许可证、已知漏洞和严格 Ed25519 验证行为做发布前安全审计。

在这些门禁完成前，两个 adapter 都是软件/SDK 候选，不是“生产安全认证”。Education profile 可显式使用
`PlainFlash` 并说明物理提取风险；要求抗物理提取的商业 profile 仍需 secure element、受保护模块存储或
经验证的加密存储，网络 sealed envelope 不能解决 at-rest 风险。

## 6. 禁止继续扩散的实现

- 不调用 Renesas FSP 私有 `*Sub` 符号；
- 不为 UNO R4 建立独有线上协议、facade 或组件 API；
- 不用恒真 verifier、TLS-only 特例或“编译成功”绕开 exact body 验签；
- 不把 `SoftwareATSE` 强行包装成需要导出/接收 raw private key 的完整 P-256 provider；
- 不使用 `Crypto@0.4.0` 的 RNG 或 Ed25519 signing API，设备熵继续由 `IRandom` 提供；
- 不把 `PlainFlash` 标记为 `EncryptedFlash` 或 `HardwareBackedSealed`。
- 不调用 any-peer `BLE.paired()`，也不恢复业务可写 `setSessionSecurity()`。

Platform BLE 与 LocalBLE composition 及自动化 fault gate 已完成软件接线，Platform BLE 也已绑定统一 facade。
下一步不是再发明 UNO 专属 crypto/BLE 协议，而是在真机完成 pairing、encryption-loss、Direct 恢复、
stack/heap watermark、掉电与三端 E2E；不再验证双无线共存或 BLE 配网。
