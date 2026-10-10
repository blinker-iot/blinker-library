# Device V2 身份/凭据存储与重置：已退出实验的历史记录

> 2026-08-16 最终状态：本文中的 `OperationalCredential`、Enrollment/1、`ResetCoordinator` 与
> `ProductOrchestrator` 已从源码和测试中物理删除。正式 Ownership Reset/Transfer 已由
> `DEVICE_V2_OWNERSHIP_RELEASE_CONTRACT.md` 取代；以下内容只解释旧设计为何退出，不能作为实现合同。

> 2026-08-16 变更提示：本文记录的 P-256 Developer/Activation 状态机和 148 B 单一
> OperationalCredential 已降为实验实现。正式方案不再使用统一 DeveloperSetupCode，而是区分
> ManualDeviceKey、EnrollmentTicket、OwnershipReceipt、CloudCredential 与 ControllerCredential；以
> `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 为准。Ownership Claim 与 Cloud Credential Enrollment 已由
> 独立 exact contract/coordinator 覆盖；本文旧状态机现在只剩 Ownership Reset/Transfer 的历史回归价值，
> 下一片替换后整组删除。

> 状态：2026-08-14。PAPI-2、UX-P1-6c exact contract及UX-P1-6d前两层已实施：平台无关凭据/Developer
> identity/reset verifier、ESP32 NVS与mbedTLS P-256 provider均已退出软件门禁；Gateway/App、真机crypto/
> power-cut及portable secure provisioning仍待接入；WiFiProv Security 2默认路径已被资源门禁否决。

## 1. 可保留结论与必须重构项

以下存储原则继续有效：

1. ManualDeviceKey/Ticket 都只用于 bootstrap authorization，不进入 Active CloudCredential、日志或设备日常
   登录；ManualDeviceKey 在手动档长期可查看/轮换，Ticket 才是短期单次会话。MCU 无可靠墙钟时由 Gateway
   判断 Ticket TTL，设备依赖 signed expiry + 当前 session/generation。
2. 任何会产生服务器状态的 credential 请求都必须先 `stagePending()`。重启加载同一 Pending 时禁止重新
   生成 credential 或 idempotency key；相同请求幂等恢复，不同 canonical body 返回冲突。
3. response 必须精确回显/绑定 request key、generation、logicalDeviceId 和 credentialVersion；先原子写
   PendingCommit，服务器 commit 成功后才进入 Active。
4. 现有 `IKeyValueStore::set()+commit()` 没有声明断电原子替换，不能直接承载 credential。新增
   `IAtomicBlobStore`：`replace()` 成功后新 blob 完整可读，失败或掉电后必须完整读到旧或新版本之一，
   不能暴露半条记录；`clear()` 同理。
5. 当前 `OperationalCredentialStore` 的 148 B big-endian 记录包含 magic、schemaVersion、
   状态、长度、generation、credentialVersion、固定容量字段和CRC-32。结构体 padding/MCU endian 不上盘。
   CRC-32只检测意外损坏，不提供真实性或防篡改。该布局只是旧 Activation 软件探针，不再是正式持久
   schema；重构时不得兼容未发布布局。
6. Factory identity 只通过 `physicalDeviceId()`、`proofAlgorithm()`、`publicKey()`与`prove()`使用；
   `publicKey()`仅供DeveloperSoftware首次enrollment，SymmetricKey返回NotFound。Core没有读取bootstrap私钥的API。
   其中 P-256 DeveloperSoftware 只是 Enhanced provider，不再是普通 DIY 默认依赖。
7. `NetworkReset`、`ApplicationReset`、`OwnershipReset` 明确分离。Ownership reset 必须把当前physical/
   logical ID、generation、本次RAM resetNonce及可信时间显式传给`IOwnershipResetVerifier`，验签成功后再交给
   可断电恢复的`IResetBackend`；Core不跨多个普通store连续remove后谎称原子完成。

正式存储必须拆分为 Identity/Factory、Ownership、Cloud、Controller、Network 五个域；另有只在
Unclaimed/Pending 存在的 Enrollment journal。`logicalDeviceId` 在 Ownership Claim 时产生，不再由旧
Cloud ActivationResponse 首次产生。正常启动只加载 Active Ownership/Cloud/Controller，完全跳过已经
消费或过期的 EnrollmentKey/Ticket。

## 2. 公开数据边界

以下 `ProvisioningCoordinator/OperationalCredentialStore` 图描述当前实验代码，不是正式对象合并方案。
正式实现应把 OwnershipStore、CloudCredentialStore、ControllerCredentialStore 和 EnrollmentJournal 分开，
但继续复用 `IAtomicBlobStore` 的原子替换要求。

~~~text
IFactoryIdentity
  physicalDeviceId() -> stable public ID
  prove(challenge)   -> proof only; bootstrap secret never leaves adapter

ProvisioningCoordinator
  load()                    -> NeedsEnrollment / PendingActivation / PendingCommit / Active
  prepare(claim,generation) -> persist Pending before network
  activationMaterial()      -> borrowed claim/key/secret for trusted TLS client
  accept(receipt)           -> compare-and-swap PendingActivation -> PendingCommit
  commitMaterial()          -> resume same server commit after reset
  confirmCommitted(receipt) -> compare-and-swap PendingCommit -> Active
  operationalMaterial()     -> borrowed Active login material for trusted client

OperationalCredentialStore
  IOperationalCredentialStore API
             |
  deterministic record codec + CRC
             |
       IAtomicBlobStore
             |
 ESP32 NVS transaction / Renesas dual-slot journal / Host fake
~~~

当前实验固定容量：Pending transaction 的 claim ID 16 B、activation key 16 B，全部状态的 secret 32 B，
logicalDeviceId 最多64 UTF-8 bytes。这些是PAPI-2设备持久格式上限，不是未来HTTP字段必须使用的文本
编码；Gateway可在wire层使用base64url。Active record 的 claim/key 对应区域必须为零。
logicalDeviceId额外禁止内嵌NUL，避免C/Arduino adapter截断。

`StorageProtection`只声明`PlainFlash / EncryptedFlash / HardwareBackedSealed`。本模型在TLS activation和
legacy-compatible登录派生期间会把operational secret读入受信RAM，因此不冒充“私钥永不导出”的
SecureElement模型；后者需要独立challenge signer/session credential协议后才能准入。

## 3. 状态与错误语义

本节是旧 Activation coordinator 的回归合同，只用于重构时验证幂等和掉电性质；正式 message/state 名称以
新的 Ownership/Cloud/Controller 分拆合同替换。

| 操作 | 当前状态 | 结果 |
|---|---|---|
| `load()` 无记录 | 任意 | `NeedsEnrollment` |
| `prepare()` | NeedsEnrollment | CSPRNG生成两项随机量并原子stage Pending |
| 相同 `prepare()` | PendingActivation/PendingCommit且同claim+generation | 幂等成功，不调用CSPRNG |
| 不同 `prepare()` | PendingActivation/PendingCommit | `StateConflict` |
| 旧/相同generation用于不同claim | Active | `SequenceConflict` |
| 更高generation直接重新enroll | Active | `StateConflict`；必须先走授权Ownership reset/rotation |
| `accept()`匹配 activation 回执 | PendingActivation | 原子写 PendingCommit |
| 重复相同`accept()` | PendingCommit/Active | 幂等成功 |
| `confirmCommitted()`匹配 commit 回执 | PendingCommit | 原子写 Active |
| 重复相同`confirmCommitted()` | Active | 幂等成功 |
| response claim/key/generation不匹配 | PendingActivation/PendingCommit | `SequenceConflict`或`StateConflict` |
| blob magic/version/CRC/UTF-8非法 | load | `InvalidEncoding`，绝不当成未注册重新生成身份 |

所有返回的 `ByteView/StringView` 都借用 Coordinator 缓存，只在下一次load/prepare/accept/
confirmCommitted/reset或析构前有效。
Coordinator和临时record在失败与析构路径使用`secureZero()`清除secret。

## 4. 重置执行边界

~~~cpp
reset(NetworkReset);      // backend只清Network域
reset(ApplicationReset);  // backend只清产品应用域
reset(OwnershipReset, serverGrant); // verifier通过后执行generation前进和三域清理
~~~

`IResetBackend::resetOwnership(expectedGeneration,nextGeneration)` 必须是幂等、可恢复操作：清
Ownership/Operational/Network并保留Factory。若平台无法提供跨namespace事务，adapter必须先持久化reset
journal，重启继续到完成。单纯长按、空grant、旧grant和`nextGeneration <= currentGeneration`都不能调用backend。
backend成功后`ResetCoordinator`必须通知`IResetObserver`；`ProvisioningCoordinator`据此立即清除RAM中的
Active secret并进入`NeedsEnrollment`。禁止只清Flash后继续使用进程内旧credential。

上述是当前实验接口的实际行为；正式目标还必须把 Controller 域纳入同一 generation reset，并把 Network
是否清除改为 grant/policy 明确字段。`Operational` 在新术语中是 Cloud 域。当前 backend 无法表达
Controller/Network policy，属于下一批必须重构项，不能直接进入生产。

## 5. 本批测试门禁

- deterministic codec round-trip、magic/version/CRC/长度/UTF-8拒绝；
- stage失败不暴露RAM Pending，activation/commit落盘失败仍可在重启后恢复同一secret；
- 同claim重试不重复调用CSPRNG，不同claim/generation冲突；
- 重复activation/commit receipt幂等，错误claim/key/generation/logical ID/version拒绝；
- activation落盘后断电恢复为PendingCommit；commit落盘失败仍使用同一材料重试；Active前拒绝登录材料；
- token字节不出现在record blob；
- Active record 的 enrollment claim/key 清零，重启仍可正常 cloud session；
- Network/Application reset不触及ownership路径；Ownership无grant/旧generation不调用backend；
- Host ASan、architecture、package、golden/fuzz和六平台Core compile回归。

最终UNO R4 32位ABI：`OperationalCredential=140 B`、`OperationalCredentialStore=8 B`、
`ProvisioningCoordinator=156 B`、`ResetCoordinator=12 B`；公共`Device`仍为1,460 B。ESP32、UNO R4、
ESP8266、Wio Terminal、RP2040、nRF52840的Advanced/Core入口均已编译通过。

## 6. 后续边界

PAPI-2退出不等于设备已经能联网认领。UX-P1-6c设备codec与三种独立参考golden已完成；Gateway/App生产接入仍
是E2E门禁。UX-P1-6d前两层已实现独立NVS storage与ESP32 mbedTLS P-256 provider；平台无关Core提供72 B
Developer key record、`P256DeveloperIdentity`、raw low-S规范、pinned server key ring和完整reset context校验。
record checksum只检测损坏；默认拒绝PlainFlash，生产仍须验证加密partition/Secure Boot。真机keygen、RFC6979
固定向量、自验签与power-cut待测；portable Noise composition继续按独立provider/产品层实施，不得把尚未实现的服务器/App
流程包装成可用QuickStart。
Legacy `authKey + dfp`登录继续隔离，不能把本批OperationalCredential偷偷映射为旧永久authKey。
