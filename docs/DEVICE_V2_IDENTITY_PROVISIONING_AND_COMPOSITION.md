# Device V2 身份、配网、认领与官方 Composition 决策

> 2026-08-17 凭据修订：普通 Education/DIY 身份入口已统一为无前缀 DeviceKey，每次启动 challenge-HMAC
> 换短期 SessionCredential；刷新 Key 强制吊销旧 session/连接。ESP32 配网优先 WiFiProv Security1，
> Security2/Noise 为 Enhanced。本文中旧 Enrollment/ManualDeviceKey/CloudCredential 默认流程只作历史设计；
> 权威合同见 `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 与 `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md`。

> 状态：2026-08-15 正式修订。身份与配网语义以
> `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 为最高优先级；全片 DIY 恢复另见
> `DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md`。旧 P-256 Developer 默认身份、统一
> SetupCode、Enrollment/1 Activation 和 productId 自报路径已经撤回。本文件保留平台存储、WiFiProv
> 资源结论和 composition 边界。

## 1. 身份与凭据结论

1. MAC、广播名和 deviceName 只用于当前无线发现/诊断，不是设备身份、设备码或密钥。
2. `physicalDeviceId` 是公开、稳定的物理实例 ID；`logicalDeviceId` 由服务器生成（手动模式在设备页创建
   时预生成，自动 profile 在 Claim 事务生成），是
   ownership/page/ACL 主键；两者不能都叫 uuid/deviceCode。
3. App 配网下发的 `EnrollmentTicket` 是 5～15 分钟、单次会话授权，不是日常登录 key。设备成功认领后
   保存长期 CloudCredential；正常重启只使用 CloudCredential 申请短期 SessionCredential。
4. WiFi-only 手动模式使用长期可查看/复制/刷新的 `ManualDeviceKey`。它可以写入 Sketch，授权首次 Claim
   或全擦后的 Recovery；Active 后固件中的 Key 被忽略，运行登录只用 CloudCredential。
5. BLE-only 不因无法联网而伪造 MQTT credential；平台模式验证 exact OwnershipReceipt、提交规范化
   OwnershipRecord，并保存每个 App 独立的
   ControllerCredential，LocalBLE 则可以完全不创建服务器 logical device。
6. 量产预烧录的是每台唯一 physical identity 和 DeviceBootstrapCredential，不是账号 authKey，也不是
   整批共用 product secret。
7. 普通 Arduino/教育档不强制 P-256、secure element、QR SetupCode 或 Python 工位工具；这些是
   Enhanced/Manufactured provider，不应进入统一用户 API。
8. 手动网络重置不等于服务器解绑。Network Reset 保留 ownership/CloudCredential；Ownership Reset
   必须有 server grant、物理在场和 generation CAS。

## 2. 必须拆开的对象

| 名称 | 是否秘密 | 生命周期 | 设备用途 |
|---|---:|---|---|
| `deviceInstanceId` | 否 | DIY 物理实例生命周期 | 16 B 随机公开 ID，不用于登录 |
| `physicalDeviceId` | 否 | 物理实例生命周期 | Claim/registry 资产 ID，不从 MAC 推导 |
| `logicalDeviceId` | 否 | ownership relationship | 日常路由、页面、ACL、历史 |
| `ManualDeviceKey` | 是 | 手动 logical device 生命周期、可轮换 | 首次 Claim/全擦 Recovery outer；Active 后忽略 |
| `EnrollmentTicket` | 是 | App 配网 5～15 分钟、一次性 | 当前 setup/claim 授权，Active 后删除 |
| `RecoveryGrant` | 签名公开 wire 对象 | 一次 replacement transaction | 授权新实例建立旧 logical device 的 N+1 recovery base |
| `OwnershipReceipt` | 签名公开 wire 对象 | ownership generation | 设备验证服务器已接受绑定；Active 后提交规范化 OwnershipRecord |
| `DeviceBootstrapCredential` | 是/私钥 | 出厂到吊销 | Manufactured factory proof |
| `CloudCredential` | 是/私钥 | 长期、可轮换 | 每次开机申请 SessionCredential |
| `SessionCredential` | 是 | 一次连接/数小时 | MQTT/WebSocket；只放 RAM |
| `ControllerCredential` | 是/私钥 | 每个 App/controller | Direct BLE 日常认证 |

ManualDeviceKey/Ticket/CloudCredential 不得作为 deviceId。`productModelId` 也不是身份：它是服务器内部
项目/型号元数据，只来自 enrollment intent 或 factory registry；实际功能由 Manifest 描述。

### 2.1 DIY physical identity

设备首次启动使用合格 CSPRNG 生成并原子保存 16 B `deviceInstanceId`：

~~~text
physicalDeviceId = "dev_" + lowercase_hex(deviceInstanceId)
~~~

它是公开 locator，不是 secret 或身份凭据；128-bit 随机碰撞在平台规模下可忽略。普通重启、OTA、
Network Reset 和 Application Reset 保留 instance ID。全片擦除可能产生新 physical instance；旧
logical device 保持离线，用户用同一 ManualDeviceKey 或登录 App 获取独立 RecoveryGrant，授权新实例在
N+1 接管；服务器不能按 MAC 自动认回。

该档只证明“当前配网者控制了这份软件实例”，不证明不可复制硬件。P-256 software key pair
可以作为 Enhanced provider，但不再是所有 MCU 的准入条件。

### 2.2 Manufactured identity

工厂为每台设备登记 physicalDeviceId、productModelId、bootstrap suite 和 verifier/public key，并写入每台
唯一 bootstrap secret/private key 或 secure-element reference。可增加 QR claim/setup code 证明用户持有
包装/设备，但 QR code 不复用为 CloudCredential。

## 3. 首次引导 profile

| Profile | 公共入口语义 | Bootstrap | 认领后数据 |
|---|---|---|---|
| WiFi-only 手动 DIY | `<BlinkerWiFi.h>` + `begin(deviceKey, ssid, password, ...)` | DeviceKey | Cloud session + WiFi；Ownership 由服务器维护 |
| WiFi+BLE DIY | internal 候选，App 配网后仍复用 WiFi Product | EnrollmentTicket + physical window | Cloud + Controller + WiFi |
| BLE-only 平台 | `<BlinkerBLE.h>` + `begin(...)`，App 绑定 | OwnershipGrant + physical window | Controller/local access |
| LocalBLE | internal 候选 | physical confirmation/可选短码 | Controller only |
| Manufactured | 尚未公开；复用同一库的 Product/Port | factory proof + Ticket/claim code | 按能力安装 Cloud/Controller |

普通用户通过 `<BlinkerWiFi.h>` 或 `<BlinkerBLE.h>` 在编译期选择 Product，调用一次 `Blinker.begin(...)` 和循环
`Blinker.run()`；`begin()` 不重复传模式/profile。平台、MQTT client、TLS socket、storage、crypto provider
由库内 composition 选择，Advanced 才允许注入。

## 4. 持久存储边界

设备至少拆成五个擦除域：

| 域 | 内容 | Network Reset | Ownership Reset | 普通升级 |
|---|---|---:|---:|---:|
| Identity/Factory | DIY instance ID 或 factory physical ID、bootstrap key/cert | 保留 | 保留 | 保留 |
| Ownership | logical ID、generation、signed receipt | 保留 | 清除/前进 generation | 保留 |
| Cloud | CloudCredential、suite/version、控制面 profile | 保留 | 撤销后清除 | 保留 |
| Controller | 16 B controller id、32 B secret、权限位/version/generation | 保留 | 全部失效 | 保留 |
| Network | SSID/passphrase/network metadata | 清除 | 按策略清除 | 保留 |

ManualDeviceKey/Ticket 只属于 bootstrap provider，不得写入 Active Cloud blob；raw ManualDeviceKey 也不得
进入 Claim/Recovery journal。SessionCredential 只放 RAM。不能用一个 `clear()`
同时清掉上述所有域。

Core 普通 `IKeyValueStore::set()+commit()` 没有自动满足断电原子替换；credential/receipt 必须使用
`IAtomicBlobStore` 或等价双槽 journal，保证 old-or-new。Pending request 必须先落盘，再发送会产生服务器
状态的请求；收到严格匹配的 commit response 后才切 Active。

ESP32 可使用独立 NVS namespace；量产启用 Secure Boot/Flash/NVS Encryption。UNO R4/Renesas 使用平台
Preferences/EEPROM/外部 flash 时也必须实现有版本、长度、CRC/完整性和双槽恢复的相同语义。普通 Flash 只能
声称 DIY/开发级安全，不能冒充 secure element。

## 5. 生命周期与重启

~~~text
Unclaimed
  -> SetupWindow
  -> BootstrapAuthorized         (ManualDeviceKey/Ticket/FactoryProof)
  -> ClaimPending
  -> OwnershipActive             (logicalDeviceId + receipt persisted)
  -> CloudCredentialPending/Active       [WiFi-capable]
  -> ControllerPending/Active            [BLE-capable]
  -> Runtime
~~~

Active 后的正常启动：

1. 加载 identity、OwnershipRecord 和相应 Active credentials；
2. 若 WiFi-capable，读取 WiFi 并连接网络；
3. 用 CloudCredential 请求短期 SessionCredential；
4. 连接 MQTT/WebSocket；
5. 若 BLE-capable，同时开放 Direct BLE，使用 ControllerCredential；
6. 忽略固件中残留或已轮换的 ManualDeviceKey，并且不读取已删除/过期 EnrollmentTicket。

因此设备关机一个月后重启不需要 App 再配网。只有尚未完成的 Pending transaction 才使用原始
request/claim/credential idempotency record 恢复。

## 6. WiFi+BLE 配网与 BLE-only relay

WiFi+BLE 推荐顺序：

1. 物理按键/首次启动打开短 setup window，广播随机 setupSessionId 和 capability；
2. App 生成不复用的 16 B controllerId 与 32 B HMAC secret，读取 physicalDeviceId/setupSessionId，使用
   用户 JWT 申请绑定 controllerId、secret digest 与设备权限位的短期 EnrollmentTicket；
3. 设备验证 server Ticket 并把安全本地会话绑定到当前 session；
4. App 写 WiFi candidate，设备测试成功后 commit；
5. 设备直接访问 Gateway 或 App TLS relay 完成 Ownership Claim；
6. 设备安装 CloudCredential，App/设备安装 ControllerCredential；
7. 分别报告 ownership/network/cloud/controller durable status；
8. provisioning GATT 关闭或切换为 Direct BLE，Cloud 与 Direct BLE 可同时运行。

BLE-only 跳过 WiFi/Cloud 分支。App relay challenge/proof/OwnershipGrant exact bytes，设备验证 server
signature 并 commit；App 本地 `bound=true` 不能代替 Gateway 状态。无 factory root 的 Education 档只能
提供物理在场/当前实例保证，SetupCode 是可选加强项，不是必填。

Provisioning 与 Direct BLE 可以共用 secure-record primitive和同一个 BLE host，但必须使用不同 mode、
scope、credential 和 replay state。首版允许 provisioning mode 独占 GATT，完成后切 Direct；不能把
WiFiProv/Bluedroid 与 NimBLE Direct 两套 host 无条件同时链接。

## 7. 重置与恢复

1. `ApplicationReset`：清业务状态/定时任务，不影响 identity、network、ownership 或 credential。
2. `NetworkReset`：只清 Network。当前 owner/controller 在新 setup window 获取 server-signed
   NetworkProvisioningGrant，写入新 WiFi；不重用 EnrollmentTicket、不创建 logical device。
3. `ControllerRevoke`：只撤销指定 controller；设备下次收到可信更新后删除。
4. `OwnershipReset/Transfer`：server grant 绑定 physical/logical ID、current/next generation、本次 nonce、
   scope 和 expiry；设备 generation CAS 后清 Ownership/Cloud/Controller 及按策略清 Network。
5. 全片擦除：Manufactured identity 应由受保护 Factory 域恢复；DIY seed 丢失时成为新 physical instance。
   WiFi-only 手动设备继续使用页面长期显示的 `ManualDeviceKey`，BLE profile 经 App 登录态创建 recovery intent，
   设备验证 signed `RecoveryGrant`，建立旧 logical device 的 N+1 recovery base，再复用现有 Claim 与
   Cloud/Controller enrollment；最终 Claim commit 才撤销旧 credential。页面/历史保留，兼容性失败的
   binding 标记待处理而不是删除。

未擦 Flash 且 Ownership/Cloud Active 时，固件中残留或已轮换的 `bdk_` 被忽略。全擦后可以从现有设备页
再次查看/复制当前 Key；旧版本返回 `BLINKER_DEVICE_KEY_ROTATED`。ManualDeviceKey 不等于可显示的
CloudCredential。精确合同见 `DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md` 与
`DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md`。

物理长按只打开 provisioning/recovery window，不能单独让服务器忘记旧 owner。账号丢失的售后恢复是独立
高风险流程，不能实现为“长按十秒即可抢占”。

## 8. Espressif WiFiProv 采用边界

Arduino/ESP-IDF Wi-Fi provisioning manager 可以作为特定产品的平台 transport，但不能成为 Core 或统一
public API 的依赖：

- Arduino `WiFiProvClass` 高层接口不能类型安全、完整地表达 Security 2 salt/verifier、自定义 endpoint
  生命周期、Claim receipt handoff 和 provisioning/Direct BLE mode ownership；混用底层 manager 后已不再是
  一个稳定高层适配边界。
- 历史 Security2 probe 链接 Bluedroid/Classic BT、protocomm protobuf、SRP/SHA-512，不适合 Education 默认；
  Security2 仍只作 Enhanced profile。
- 当前 Security1 adapter 已把公共 manager/DeviceKey transaction 与 BLE/SoftAP scheme 分到独立目标文件。
  P3 Plain/TLS 为 `1,758,907/62,964 B`、`1,845,907/63,556 B`，TLS 只余 6.11%，拒绝作为默认；P6
  SoftAP+NimBLE Plain/TLS 为 `1,375,887/60,856 B`、`1,463,067/61,456 B`，双 OTA 余 30.02%/25.58%，
  且 ELF 无 BLE provisioning/Bluedroid，成为 ESP32 WiFi+BLE 领先候选。P4 单 Bluedroid 历史图仅余 7.33%，
  未发布 product code 已删除。
- P6 只是静态准入，不是产品发布：SoftAP App UX、AP service key/Security1 PoP 域分离、Controller control、
  动态内存、WiFi/BLE 共存和真机长稳仍是阻断。
- UNO R4/Renesas 复用上层 state/wire，提供自己的 BLE/WiFi/storage/crypto adapter，不伪装成 WiFiProv。

参考：

- [ESP-IDF Wi-Fi Provisioning](https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/provisioning/wifi_provisioning.html)
- [Protocomm Security 0/1/2](https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32/api-reference/provisioning/protocomm.html)
- [Arduino-ESP32 WiFiProv API](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiProv/src/WiFiProv.h)
- [Noise Protocol Framework](https://noiseprotocol.org/noise.html)

## 9. 官方 Composition 分层

~~~text
User Sketch: <Blinker.h> + begin(profile, interactions...) + run()
                         |
Product Composition: ownership / network / cloud / direct BLE lifecycle
       |                         |                         |
Cloud service              Direct BLE               Provisioning
HTTP + MQTT                BBP/2 BLE                 secure record + claim
       |                         |                         |
Platform WiFi / TCP/TLS / MQTT / BLE / random / crypto / atomic storage adapters
~~~

- Core `Device` 只负责 Manifest、Property/Action/Event、状态和 BBP/2；
- composition 决定服务何时 start/stop/retry/reset，不解析平台 SDK；
- 默认 MQTT 继续以窄 adapter 支持 PubSubClient；Advanced 可传入 application-owned client，Core 不复制第三方库；
- profile 不支持某能力时必须编译期或明确运行时失败，不静默降级或链接未选择的大依赖；
- ESP32、UNO R4、Wio Terminal 等平台实现同一 capability seam，不创建平台专属业务 facade。

## 10. 当前代码与下一步

现有 P256DeveloperIdentity、Enrollment/1 codec、ProvisioningCoordinator、BLE 底层探针、NVS store 和
golden 只保留为窄接口/掉电恢复/资源探针证据。以下公共候选必须在继续实现前改正：

- 已删除 `developerEnrollmentToken/setupKey/localAccessKey` 三个模糊 public 入口、伪 WiFi enrollment
  QuickStart 和旧 official BLE package，不替换为一个万能 SetupCode；
- DeviceInstanceId、CloudCredentialStore、OwnershipRecordStore 与当前两槽 ControllerCredentialStore 已完成；
  Controller 日常/变更、Ownership Claim 与 Cloud Credential Enrollment exact wire/coordinator 已冻结；
  旧 EnrollmentKey outer 已物理删除；Education WiFi 已由 DeviceKey reveal/rotate/challenge-HMAC 合同取代；
- normal boot 优先加载 Active credential，存在时完全跳过 enrollment；
- 自动 profile 的 Ownership Claim 产生 logicalDeviceId；手动模式由 App 预创建 AwaitingFirstClaim logical ID，
  Claim 只激活 relationship。Cloud 单独激活；Controller 在 Ownership Active 后由
  server-signed Grant 安装，App 验证后 Commit 只收敛服务器 registry；
- 完成 Reset 与外围 authorization exact golden 后，再实现 WiFi-only → WiFi+BLE → BLE-only → factory；
- 每阶段通过 Host、ESP32、UNO R4 的编译、资源、掉电、重放、真机和长稳门禁后才进入 QuickStart。
