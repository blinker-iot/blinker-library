# Device V2 WiFi+BLE 产品编排器实施决策

> **已删除（2026-08-16）**：本文只保存旧编排器的状态机、失败回滚和资源证据。
> 旧 `ProductOrchestrator` 源码、测试及关联 Arduino product probe 已删除；下文所有
> “当前/唯一/现已实现”均描述当时实验状态，不得用于新代码或三端协议实现。

> 状态：2026-08-15，设备参考实现已落地。本文是旧 Setup-PSK/Enrollment-v1 产品生命周期的实验记录；
> 正式接入主线已经改为 ManualDeviceKey/Ticket → Ownership/Recovery → Cloud/Controller credential 分拆，
> `ProductOrchestrator` 必须重构后才可重新成为主线。
> App、Gateway、Broker 尚未完成的合同列入
> `APP_SERVER_BBP2_REFACTOR_INPUT.md`。旧 JSON/authKey 仍可由旧固件和旧服务器入口继续运行，
> 但不再属于 Device V2 源码、示例或回退路径。

> 范围修正：本文和当前 `ProductOrchestrator` 只描述安全配网、WiFi cloud 与 Direct BLE 并存的
> `WiFiBLE` 组合，不是所有 MCU/transport 的公共 facade。统一公共 API、WiFi/BLE/WiFiBLE profile、
> 平台 integration 和后续重命名/清理顺序以
> `DEVICE_V2_UNIFIED_PRODUCT_API_AND_PLATFORM_COMPOSITION.md` 为准。不得直接在本类外增加
> `Esp32Product` 用户 API。

## 1. 历史冻结结论

旧 `ProductOrchestrator` 曾是该实验 WiFi+BLE 组合唯一的生命周期所有者，顺序为：

```text
持久状态恢复
  -> Setup window + Provisioning BLE
  -> WiFi candidate connect + durable confirm
  -> activation/commit -> OperationalCredential Active
  -> Direct BLE
        -> device-cloud-session/2
  -> MQTT CONNECT + BBP/2
```

运行态 Direct BLE 与云 MQTT 可以同时在线；配网态只运行 Provisioning BLE。两个 BLE
协议复用同一个 `IBleModeLink`，切换时先停止旧 endpoint、重配广播/GATT，再启动新 endpoint，
不允许两个 BLE server 或两个组件分别控制同一 radio。

`Client` 管理云传输，负责 BBP/2 路由；Direct BLE 以
`TransportLifecyclePolicy::External` 注册，由 `BleModeCoordinator` 管理启停和轮询。
这是必须遵守的所有权边界，否则 `Client::begin/end` 和 BLE 模式切换会重复启动、重复停止同一对象。

编排器现已实现公共 `IProductLifecycle`。普通 `Product` 路径在 `begin()` 时延迟 attach `Device` 的
`Client`，并原子注册 Direct BLE 与 cloud；任一注册失败会撤回本轮已添加 transport。原有直接传入
`Client` 的构造方式仅保留给高级手工 composition，不作为最终 QuickStart。

## 2. 状态与错误行为

产品状态为：

- `Stopped`：所有组件停止；
- `Provisioning`：等待本地 claim/WiFi 信息；
- `Enrolling`：activation 或 commit 进行中；
- `ActiveOffline`：已有长期运行凭据，但 WiFi 不可用；
- `CloudConnecting`：云会话或 MQTT 建立中；
- `Online`：MQTT 已在线，Direct BLE 可同时工作；
- `EnrollmentError`：认领合同终止失败；保留本地恢复入口；
- `CloudError`：云会话/MQTT 终止失败；不因此关闭 Direct BLE；
- `Fault`：本地状态损坏、生命周期冲突或 BLE 模式切换无法恢复，停止所有组件。

公共 `ProductStatus` 将 `CloudError` 表达为 `Active + lastError + cloudReady=false`，将仍可通过本地
入口恢复的 `EnrollmentError` 表达为 `Provisioning + lastError`。只有停止所有本地能力的不可恢复
错误才映射为 `Fault`；否则上层会错误地关闭仍可用的 Direct BLE/配网入口。

BLE endpoint 的单次 `Error` 先按当前 profile 重启，不新建 setup window。只有首次进入配网态或
Ownership Reset 才生成新的 `setupSessionId`。WiFi 网络可用性同时 gate enrollment HTTP、
cloud-session HTTP 和 MQTT，避免关联、DHCP 尚未完成时触发 DNS/TCP/TLS 阻塞。

Ownership Reset 后，编排器必须在同一轮循环中停止 `Client`、撤销云会话、打开新的 setup
window 并切回 Provisioning BLE。普通 WiFi 断线不清除 ownership/operational credential，
只走 WiFi 重连。

## 3. Setup QR 与动态 session 的修正

静态工厂标签不可能预先包含每次配网窗口都变化的 `setupSessionId`。冻结后的职责是：

- 静态 QR/NFC：版本、`physicalDeviceId`、产品/transport/security suite、32 B setup PSK
  或其受保护载体；
- BLE 广播：当前窗口新生成的非零 8 B `setupSessionId`、模式、wire version、能力；
- App：同时持有静态标签信息与实时扫描结果，只能把 PSK 用于广播中精确匹配的当前 session；
- 动态屏幕二维码可选地携带当前 `setupSessionId`，但每次新窗口都必须刷新；
- 设备激活或窗口关闭后，`closeSetupSession()` 必须使旧 locator 立即失效。

因此“在固定印刷 QR 中包含 setupSessionId”的早期描述已废止。App 不得仅凭 BLE MAC、旧扫描
缓存或 setup PSK 猜测当前会话。

## 4. 当前代码组成

核心层保持平台无关且无堆分配：

- `ProductOrchestrator`：产品状态和唯一启停顺序；
- `WifiCredentialStore` + `WifiConnectionLifecycle`：候选凭据、断电一致性、确认和回滚；
- `ProvisioningCoordinator` + `HttpEnrollmentDriver`：PendingActivation/PendingCommit/Active；
- `BleModeCoordinator`：Provisioning/Direct BLE 独占切换；
- `HttpCloudSessionProvider` + `ManagedMqttTransport`：CBOR/HMAC 短期 MQTT 会话；
- `Client` + `DeviceRuntime`：Manifest、State、Command、可靠传输和 BBP/2。

平台通过接口适配：`IWifiStation`、`IBleModeLink`、`IAtomicBlobStore`、
`IOperationalCredentialStore`、`IFactoryIdentity`、`INoiseCryptoProvider`、`IRandom`、
`IHttpClient`、`IMqttClient`。PubSubClient 只是当前精简 MQTT adapter，不进入核心 API；应用也可以
传入其他符合 `IMqttClient` 的实现。

Device V2 已删除以下旧链路：`LoginContract`、`LegacyLoginJson`、`HttpControlPlane`。
V2 不解析旧登录 JSON、不读取 Sketch `authKey`、不在失败时 fallback。旧设备兼容由旧服务器
endpoint 与旧固件承担，避免把迁移成本永久写进新设备库。

## 5. 平台准入现状

- ESP32：WiFi、NimBLE、CSPRNG、NVS、P-256 identity 和 Noise crypto 已组成完整Developer静态产品链；
  仍需真实安全NVS/PSK量产注入、ManufacturedHardware SKU和真机 E2E 才能发布。
- UNO R4 WiFi：WiFi、ArduinoBLE、CSPRNG adapter 可编译；完整安全配网仍缺与 ESP32 相同
  Noise suite 的生产 crypto/identity/secure-storage 适配和真机资源门禁。不能用 Just Works 代替。
- Wio Terminal/其他 MCU：核心 BBP/2 可复用；只有补齐该平台接口并通过资源、安全和真机门禁后，
  才能声明完整 WiFi+BLE 产品支持。

`WifiBleProductProbe` 当前故意从一个易失、预注册的 Active credential 启动，用于证明
ESP32/UNO R4 上编排器、Direct BLE、WiFi、CBOR cloud session 与 MQTT 的链接/资源组合。
它不执行完整 Noise enrollment，不是 QuickStart，也不是可复制的生产凭据存储实现。

2026-08-15 Arduino CLI Small profile静态结果：

| 目标 | Flash | Static RAM | 静态结论 |
|---|---:|---:|---|
| ESP32 core 3.3.1 + compact NimBLE 2.5.0 | 1,297,291 B / 1,572,864 B双OTA slot | 62,304 B / 327,680 B | slot余275,573 B，17.52%，通过10%门禁 |
| ESP32完整Developer安全组合 | 1,321,659 B / 1,572,864 B双OTA slot | 67,280 B / 327,680 B | slot余251,205 B，15.97%，通过10%门禁 |
| UNO R4 WiFi core 1.5.3 + ArduinoBLE 2.0.2 | 167,696 B / 262,144 B | 15,916 B / 32,768 B | SRAM余16,852 B，静态通过，运行时待真机 |

UNO R4 32位ABI的`ProductOrchestrator`对象为48 B（增加 `IProductLifecycle` vtable 与延迟绑定指针），
仍通过64 B回归门禁；生命周期协调本身不是
SRAM压力来源，主要常驻开销仍是HTTP caller buffers、短期credential arena、Client storage和无线库。

第一、三行是跨平台pre-enrolled基线，第二行来自独立`Esp32SecureProductProbe`。完整ESP32行额外包含受保护storage、
P-256 identity/server receipt verifier、Noise和local provisioning application。所有数字已经替代旧JSON/authKey
product probe结果；完整链仍未完成真机和三端E2E。

基线数字包含两套TLS Client、CBOR
enrollment/cloud-session、hardware random、WiFi journal/lifecycle、Managed MQTT、Direct BLE、
`Client`和编排器；不包含完整Noise enrollment crypto/identity/storage composition。

## 6. 发布门禁

设备侧还必须完成：

1. ESP32真实安全NVS初始化/attestation、Factory/Operational/Network量产注入、PSK预置和真机掉电证明；
2. UNO R4已明确降级：官方core只保留`uno-r4-preenrolled-product`连接性基线；安全配网SKU必须外接完整
   protected-storage/identity/Noise provider，否则不开放Provisioning mode；
3. 真机 AP 失败、候选回滚、response loss、commit 恢复、ownership reset、MQTT refresh/revoke；
4. WiFi+BLE 并发流量、最大 loop latency、heap/stack watermark 和 24 h soak；
5. App/Gateway/Broker 按冻结合同完成 codec、幂等、ACL、撤销、setup 扫描关联和 E2E。

UNO R4的证据、外接provider要求和禁止依赖的private SCE ABI见
`DEVICE_V2_RENESAS_UNO_SECURITY_ADMISSION.md`。

在这些门禁完成前，可以发布核心/adapter 的开发版本，不得把产品探针包装成最终用户
QuickStart 或声称 UNO R4 已具备完整安全 BLE 配网。
