# Device V2 Wi-Fi 凭据日志与 BLE 安全配网组合

> **范围状态（2026-08-17）**：Wi-Fi 双槽 journal、station lifecycle 与 BLE radio handoff
> primitives 仍是 active 底座；本文涉及的 Enrollment-v1、Setup-PSK、旧 ProductOrchestrator 和
> `WifiBleProductProbe` 已归档或删除。后半段的旧组合过程仅作历史证据，不能决定下一实现顺序。
> 当前 LocalSetup 入口、R4b-4d durable saga 与下一批 official composition 只以
> `DEVICE_V2_LOCAL_SETUP_APPLICATION_CONTRACT.md` 和 `DEVICE_V2_REFACTOR_PROGRESS.md` 为准。

> 2026-08-15 纠偏：WiFi 双槽 journal、staging/test/commit 和 BLE mode handoff 结论保留；
> `ISetupPskProvider/Noise NNpsk0/Enrollment v1` 组合是 Enhanced/Manufactured 实验，不是普通
> WiFi+BLE 默认接入合同。正式 App EnrollmentTicket 流程需替换其 bootstrap 部分。

历史状态：实验设备软件实现曾通过 Host/ESP32 编译门禁；断电、真实 AP 切换、双无线峰值和 24 h
从未完成真机验证。本文不是当前 BLE bootstrap 或产品 composition 合同。

## 1. 本批结论

设备端不采用“一个 NVS blob 直接保存当前 SSID/密码”的实现。最终分层为：

```text
LocalProvisioningCoordinator (borrowed Wi-Fi body)
        |
        v
IWifiCredentialSink
        |
        +-- WifiCredentialStore (portable two-slot journal)
                |
                +-- selector: IAtomicBlobStore
                +-- slot 0:   IAtomicBlobStore
                +-- slot 1:   IAtomicBlobStore
                         |
                         +-- ESP32: Esp32NvsAtomicBlobStore x 3
                         +-- Renesas/SAMD/RP2040: platform blob adapter
```

`WifiCredentialStore` 不包含 ESP32、WiFi、BLE、MQTT 或文件系统 API。新 MCU 只需
提供三个独立、单 blob 原子替换的 `IAtomicBlobStore`，不重新实现 journal 和 wire。

## 2. 为什么单 blob 不够

ESP-IDF NVS 能保证一个 key 在突然掉电后是旧值或新值，而不是半条记录；但
`nvs_commit()` 返回失败时，当前进程的 working view 仍可能读到刚写的新值，重启后
却恢复旧值。仅有一个 Wi-Fi blob 时无法同时满足：

1. 新配置完整持久化后才确认；
2. 新 AP 不可达时仍有上一配置可回退；
3. 写 slot 或 selector 任一步失败/掉电都不产生半条 SSID/密码；
4. 调用方不需要猜测一次失败的 commit 是否已经生效。

因此 journal 固定为“先写 inactive 完整槽，再切 16 B selector”。selector 切换前
旧槽仍是 active；切换后新槽 active、旧槽成为一次 rollback target。失败的
`commit()` 不更新当前对象的 active 状态，调用方按失败处理并可重试同一请求；重启
只会看到指向某个完整、CRC 和 generation 均合法的槽。

## 3. 固定记录

### 3.1 Profile record：116 B

```text
magic BWF2 | schema | auth | flags | ssidLen | credentialLen
reserved(3) | generation(be32) | ssid[32] | credential[64] | crc32(be32)
```

- 未使用 SSID/credential 字节必须为零，避免旧密码残留和多种编码；
- generation 从 1 开始，0 非法；
- auth、长度、hidden flag 和 WPA2/WPA3 密码语义复用
  `validateWifiNetworkConfig()`；
- CRC32 只检测损坏，不提供真实性。生产保密性依赖加密 flash/NVS、Secure Boot 和
  正确的物理安全发布流程。

### 3.2 Selector record：16 B

```text
magic BWS2 | schema | activeSlot | reserved(2) | generation(be32) | crc32(be32)
```

`activeSlot=0xff,generation=0` 是 network reset tombstone；其余只允许 slot 0/1 和
非零 generation。

## 4. 状态与失败语义

### Commit

1. 严格校验 SSID/auth/credential；
2. 与 active profile 语义完全相同时幂等成功，不写 flash；
3. 编码 `generation+1` 到 inactive slot 并原子 replace；
4. slot 成功后原子 replace selector；
5. 只有两步都返回成功才更新 RAM active 状态并允许返回 WifiConfigAck success。

inactive slot 写失败时旧网络不变。selector 写失败时设备仍按旧 active 状态运行；
重启可能选择旧或新 selector，但两槽都完整，且另一槽仍是回退目标。

### Load 与 rollback

启动时先读 selector，再读取它指定且 generation 完全相等的 slot。magic、schema、
reserved、zero padding、CRC、网络语义或 generation 任一不符都拒绝启动该 profile，
不能“尽量解析”。`loadPrevious()` 只接受另一槽的 `activeGeneration-1`；
`rollbackToPrevious()` 通过原子 selector 更新完成，不复制密码。

产品 Wi-Fi connection manager 的正式顺序应为：

```text
load active -> 尝试联网/限时重试
    | success -> activation/login
    | failure and previous exists
    v
rollback selector -> load previous -> 尝试旧网络
```

连接超时、退避、AP auth 错误分类和何时 rollback 属于后续 platform Wi-Fi lifecycle，
不应塞进 credential journal。

### Clear

network reset 先写 selector tombstone，再清两个 slot。tombstone 成功后即使 slot 清理
失败，active 仍保持清除并返回清理错误；重试可继续物理清理。不得把 network reset
扩大成 factory identity、ownership 或 operational credential 擦除。

## 5. ESP32 实现

`Esp32NvsWifiCredentialSink` 位于既有 `esp32-storage` adapter：

| namespace/key | 最大值 | 用途 |
|---|---:|---|
| `bl_v2_net/wifi_sel` | 16 B | active selector |
| `bl_v2_net/wifi_0` | 116 B | profile slot 0 |
| `bl_v2_net/wifi_1` | 116 B | profile slot 1 |

adapter 不包含 `WiFi.h`，不连接 AP，也不拥有 BLE/Wi-Fi radio；它只把三个 NVS key
组合成 portable store。默认拒绝 plain flash：

- `encrypted=true` 不会替用户打开 flash/NVS encryption，也不能单独构成保护证明；
- 必须同时提供`Esp32NvsEncryptionVerifier`；`begin()`只在callback证明目标partition后打开并报告
  `EncryptedFlash`。callback必须由真实安全NVS初始化与发布证据支持，不能无条件返回true；
- `allowPlainFlash=true` 只允许开发/compile probe，不得进入量产 QuickStart。

真实 ESP32 SDK compile probe 强制链接 `commit/load/loadPrevious/rollback/clear`，避免
只有头文件通过、平台实现未被编译的假门禁。

## 6. 其他 MCU 的复用方式

UNO R4/Renesas、SAMD/Wio Terminal、RP2040 等平台继续使用同一个
`WifiCredentialStore`。平台 adapter 必须证明：

1. 每个 blob 的 replace 在掉电后只暴露旧完整值或新完整值；
2. 三个 blob 使用独立 key/page/record，不能把它们再次拼成一次非原子大写入；
3. `protection()` 如实报告 PlainFlash/EncryptedFlash/HardwareBackedSealed；
4. PlainFlash 只有显式开发 opt-in；
5. 覆盖每个写点掉电、磨损/满盘、损坏、重启、rollback 和 clear 重试。

若平台底层只提供整页擦写，应在 adapter 内实现双页/commit marker，不能让 Core
依赖某家 EEPROM/Flash API。

## 7. BLE 安全配网组合修正

`IBleModePreparation` 不能只调用 `Noise::arm()`。正式实现现在由：

- `ISetupPskProvider`：按 exact 8 B `setupSessionId` 从 factory/受保护域临时输出恰好
  32 B 高熵 PSK；
- `ILocalProvisioningSession`：开始/结束绑定同一 setup session 的 claim/proof/Wi-Fi
  application 状态；
- `BleNoiseModePreparation`：先清旧 Noise 和 application session，再取 PSK、arm、
  beginSession，并在所有失败路径清理；栈上 PSK 随即 `secureZero()`；
- `BleModeCoordinator::restartActive()`：BLE 断连、Noise fault 或认证失败后，由产品
  loop 显式停止物理 endpoint、重新 prepare 同一 profile、再恢复广告。

不能仅依赖 `switchTo(sameProfile)`：它按合同是幂等操作，不会重启。也不能在 BLE
SDK callback 中同步读取 NVS/factory secret。callback 只置 recovery flag，Arduino
`loop()` 观察到 Provisioning channel 未 armed 后调用 `restartActive()`。

Direct mode preparation 总是取消 Noise 并结束本地配网 session。模式协调器、广播
profile 和 setupSessionId 都不保存 raw PSK。

## 8. 仍未完成的产品组合

本批没有伪造“完整配网产品已经完成”。还缺两个正式层：

1. `BleNoiseProvisioningChannel <-> LocalProvisioningCoordinator` 的无分配 application
   endpoint，负责 handshake/transport payload、响应发送、backpressure 和 fault recovery；
2. Wi-Fi connection lifecycle 与 TLS activation/commit driver，把
   `ProvisioningCoordinator` 的 material/receipt 状态机接到真实 HTTP，再在 Active 后
   切 Direct BLE、启动 cloud MQTT。

当时的 `WifiBleProductProbe` 是 legacy authKey/cloud + Direct BLE 资源探针，不能改名
冒充安全 provisioning QuickStart。上述两层完成前，设备库只能声明“协议、存储和
mode composition primitives 已具备”。

## 11. 2026-08-14 endpoint与ACK语义修订（覆盖第8节第一项待办）

`BleNoiseProvisioningChannel <-> LocalProvisioningCoordinator`之间的无分配application endpoint现已完成，
正式类型为`BleLocalProvisioningEndpoint`。它处理handshake/transport响应、单TX槽背压、active-session隔离、
disconnect/fault recovery及operation/response workspace清零；完整合同见
`DEVICE_V2_BLE_LOCAL_PROVISIONING_ENDPOINT.md`。

实现过程中同时纠正了原文“selector切换成功即可WifiConfigAck”的不完整语义。现在success ACK必须同时满足：

1. `IWifiCredentialSink::commit()`成功，网络profile可掉电恢复；
2. `IEnrollmentClaimSink::prepare()`成功，同一已验证`claimId/generation`已由
   `ProvisioningCoordinator`持久化为`PendingActivation`。

若第1步成功而第2步失败，设备不ACK成功，保持可用同一exact WifiConfig重试；相同Wi-Fi不会重复写flash，
相同claim不会重新生成secret。ACK仍不代表AP连接、activation/commit、Direct BLE或MQTT成功。

因此该阶段第8节只剩 Wi-Fi lifecycle + TLS activation/commit driver。旧 `WifiBleProductProbe`
后来已删除。


## 9. App 与服务器必须同步的合同

App：

- WifiConfig body 只在 Noise transport 内短暂存在；响应丢失时重试 exact
  `{requestId,body}`，不能复用 ID 更换 SSID/密码；
- 只有设备返回 durable success ack 才进入“联网中”；存储/校验失败显示可重试错误；
- 设备切换或 restart profile 会断开 BLE，App 必须按相同 setupSessionId 重扫重连，
  不能在旧 GATT session 等待；
- 不把 Wi-Fi plaintext 写入 Ionic storage、日志、analytics 或 crash report。

Gateway：

- 不接收、不保存 Wi-Fi credential 或 raw setup PSK；
- signed proof receipt 仍绑定 exact setupSessionId/proof digest；
- activation success 先是 pending commit，只有设备 TLS commit 完成才置 Active；
- BLE retry、Wi-Fi rollback 或 App 重连不得生成第二份 claim/activation 事务。

## 10. 发布门禁

- Host strict warning + functional/fault tests；
- ASan，Linux ASan+UBSan，20,000+ deterministic fuzz（含 profile record decoder）；
- ESP32 real SDK storage/core compile；
- UNO R4 32 位 `WifiCredentialProfile <=104 B`、`WifiCredentialStore <=32 B`、
  `BleNoiseModePreparation <=16 B`；
- 三平台 power-cut fault matrix 与 encrypted partition attestation；
- 真实 AP 新/旧凭据切换和 rollback；
- Provisioning/Direct 断开、重扫、重连、wrong PSK、重复请求和 fault restart；
- ESP32 `cloud + provisioning + Direct NimBLE + identity/store` 双 OTA 至少 10% headroom；
- UNO R4 32 KiB SRAM stack watermark，未过前不宣称安全 BLE 配网可发布。

## 12. 2026-08-15 candidate/confirmed 与联网激活修订（覆盖第4、8节旧描述）

实现 Wi-Fi lifecycle 前复核发现，仅保存 `active/previous` 无法表达“新配置尚未真实连通过 AP”。如果每次启动都在
两槽间自动回滚，两个历史配置都不可用时会跨重启反复切换。现冻结为以下状态语义：

- profile flags bit 0 为 `hidden`，bit 1 为 `confirmed`；记录尺寸仍为116 B，32位
  `WifiCredentialProfile`仍为104 B；旧测试记录的bit 1为0，按candidate处理；
- `commit()`写入的新profile固定为candidate；真实AP连接成功后，`confirmActive()`原子重写当前slot的
  confirmed bit。确认写失败时不开放网络给activation/cloud，并在保持连接时重试持久化；
- `rollbackToPrevious()`只允许`unconfirmed active -> confirmed previous`。confirmed网络断线只退避重连，
  绝不回滚到历史槽；因此掉电后不会在两份配置间来回翻转；
- WifiConfig success ACK仍只表示“Wi-Fi candidate + PendingActivation均已持久化”，不表示candidate已连通。

新增平台无关`IWifiStation`与`WifiConnectionLifecycle`。平台adapter只实现非阻塞
`start/poll/state/lastError/stop`，不得把DNS、TLS或MQTT塞入Wi-Fi连接接口。Core负责连接超时、candidate确认、
受限回滚、confirmed退避重连和credential generation热切换；104 B profile由产品提供workspace，生命周期对象不复制
第二份密码。

新增`HttpEnrollmentDriver`，严格受`setNetworkAvailable()`门控：离线时不调用HTTP poll，因此不发生
DNS/TCP/TLS工作；在线后按`PendingActivation/PendingCommit`持久状态分别执行：

```text
candidate AP connected -> confirmActive(profile)
-> POST /api/v2/device-activations
-> persist PendingCommit
-> POST /api/v2/device-activations/commit
-> persist Active
```

请求固定TLS、canonical CBOR、`Content-Type/Accept: application/cbor`及
`X-Blinker-Contract: enrollment/1`。activation body含一次性raw operational secret和factory proof；commit只含
HMAC proof。此前`ActivationCommitMaterial`遗漏operational secret、无法计算合同HMAC的问题已修正。请求/响应使用
caller-owned 256+160=416 B最小buffer，取消、完成和失败均清零。429/可重试EnrollmentError按retry class和
retry-after退避；redirect、非canonical成功体和畸形错误体fail closed。

软件门禁已通过Host/ASan 14/14、20,000 deterministic fuzz、UNO R4 SizeProbe 52,160/12,348 B、ESP32 Core
353,411/22,616 B、Wio Terminal Core 83,524 B及UNO R4 HTTP 64,848/8,712 B。当前仍未完成真实平台
`IWifiStation` adapter、正式product orchestrator、真实AP/掉电/双无线/24 h门禁；当时的旧`WifiBleProductProbe`不能
作为安全配网QuickStart。UNO R4 32位实测`WifiConnectionLifecycle=44 B`、`HttpEnrollmentDriver=120 B`。

## 2026-08-15 真实ESP32/UNO R4 station adapter（覆盖上方待办）

ESP32 与 UNO R4 WiFi 在同一个 Arduino 库中各有独立 Port，实现同一个 `IWifiStation` 边界；Core 没有引入
平台 WiFi 头文件。Port 对象在 32 位 ABI 均为 8 B，只在 `start()` 期间借用配置并使用 33+65 B 临时栈 buffer，SDK同步消费后立即清零，
不形成第二份长期密码。`WifiConnectionLifecycle`继续独占candidate确认、回滚、超时与重连策略。

UNO R4通过`WiFi.setTimeout(0)`移除`begin()`默认10 s association等待，但WiFiS3单次协处理器命令仍可能因模块
无响应而触发内部同步超时；ESP32在netif启动阶段仍可能有约1 s有界等待。接口和发布门禁按cooperative/bounded
transaction描述，不再作“绝对非阻塞”承诺。

安全能力已显式化：ESP32支持Open/WPA2/WPA3及transition minimum-security floor；UNO R4只接受Open/WPA2，
对WPA3、transition和64-hex raw PSK返回`UnsupportedFeature`，不静默降级。完整实现、证据及Wio限制见
`DEVICE_V2_WIFI_PLATFORM_ADAPTERS.md`。

该阶段曾实现旧 `ProductOrchestrator` 与 v2 cloud session，并把 `WifiBleProductProbe` 改成预注册
Active credential 的运行态组合探针；该探针现已从工作树和 CI 删除。当前边界见
`DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md`。
