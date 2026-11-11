# Device V2 BLE 直连、BLE 网关与设备间路由架构

> 状态：2026-08-29。G1A 设备/Service/App 生产链路、App-kill intent 恢复合同及 Direct 产品会话已完成；
> Android + XIAO ESP32-S3 的 Education/Nearby 原生认领、持久重连、App kill、Manifest/State、Command/ACK
> 与正式产品页面闭环、XIAO 软件复位/真实断电后的持久重连及 active-session `resetAccess()` 同 owner
> 重认领均已通过。2026-08-26 复测修正 ESP32 单方面按协商 MTU 放大分片的跨平台回归；BLE v1 bearer
> 现与 UNO R4/Wio/App 一致固定为 20 B。
> Wio 的协议实现、共享 golden 和 Host 门禁已完成；此前 Android RPA + RTL bonding 会返回
> `HCI_ERR_KEY_MISSING`，闭源 `btgap.a` 仍不能可靠恢复 identity keys。`2.1.3-blinker.2` 当前只是失败复现工件，
> 不能以“生产 bonding 修复”发布或上游。
> 2026-08-27 回刷 Seeed 官方发布包 RTL `2.1.3` 并执行 `resetAccess()` 后，Android App、Service 与 Wio 已完成
> `Hello -> Noise NN -> Service grant -> App relay -> receipt/commit -> Direct -> Method 2 -> DirectSecureRecord ->
> Manifest/State -> Power/Button` 的同 owner 完整重认领。跨账号认领被 Service 拒绝；页面退出重入、App kill、
> 同一 Sketch 重刷后凭据持久恢复和再次控制也已通过。由此冻结：Wio 的应用安全基线不依赖自维护 RTL 固件，
> 官方 `2.1.3` 可作为当前发布依赖；这不代表其系统 pairing/bonding/RPA 冷恢复问题已经解决。
> Noise/Method 2/DirectSecureRecord 是应用身份、机密性、完整性与防重放的强制安全根；原生 BLE pairing/encryption
> 只是在平台能够可靠实现时启用的纵深防御，不再作为可移植协议的前置条件。UNO R4 + Android 已完成认领、
> Manifest/State、Command/ACK、App kill、固件重启、持久重连及 active-session `resetAccess()` 门禁。Wio BLE
> 仍保持 opt-in preview；未认证连接 15 s 主动退役、128 轮软件压力和官方 `2.1.3` 三端重认领/重刷恢复门禁
> 已完成；官方 `2.1.3` 的真实重新上电冷启动、App 冷启动、Direct 恢复与 Power/Button 复测也已通过，
> 2026-08-29 已完成真机 readiness/authentication deadline、32 次反复断连和资源水位门禁；只剩真实 owner
> DirectSecureRecord 的 24 h 长稳；
> 纯 BLE 交付不依赖 Gateway profile 的发布状态；G2A 的完成或失败都不能改变 Direct 产品合同。
> XIAO ESP32-S3 已改为 ESP-IDF NimBLE 普通 GATT bearer，并通过 Android 真机完整认领、Direct 重连、
> Manifest/State 与 Power Command 门禁；全程无系统 PIN/pairing。扫描误连根因是现场多块旧板同名
> `Blinker`，不是当前 XIAO 未广播；App 现展示候选地址后缀与 RSSI，不再自动取第一个候选。
> 2026-09-02 G1B 又完成同一 XIAO 的 BLE WiFiProv Security1 真机链：`resetNetwork()` 只清 WiFi journal，
> 保留 DeviceKey、DeviceInstanceId、accessEpoch 与 App Direct credential；App 以 DeviceInstanceId 解析并核对
> 原 logical device，不按 MAC 认领，也不刷新 Key。Direct 活跃时切入 WiFiProv 无需拔电，NimBLE Host 在有界
> GAP disconnect 后释放；App 只发现一个 `BLINKER_` 候选。`GetInfo(hasDeviceKey)` 作为既有身份就绪证明后，
> 标准 WiFi endpoint 配网成功，设备回到 `network=1/local=1/productError=0`，随后 Direct Method 2、Manifest/State
> 和两次 Power Command 通过。设备最终重编为 1,275,711 B Flash / 53,980 B globals。Cloud/Direct 同时在线
> 仍待本机防火墙开放 Gateway 宿主 `17000/18883` 后关闭；动态 heap/stack、循环与长稳仍不是已完成事实。
> 本文优先于旧文档中“Controller Method 2 已经派生 session key 并保护后续帧”的描述。
> G2-0～G2B 的 exact 批次、secret envelope、三端任务和发布门禁见
> `DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md`。
> 2026-09-01 已关闭 G2A 单 child 的设备/Edge Hub/Cloud/App 四端真实数据面：Active admission、
> Manifest/State、真实 Android account MQTT Action/ACK/Patch、Broker 重启恢复、连续两轮短期
> SessionCredential 刷新及 child 复位后的状态代际切换均通过。Hub 不再透明复用 cloud request sequence，
> 而是在同一 child session 内分配自己的单调 sequence，并在 ACK/response 回程映射回 cloud sequence。
> 同日又完成真实四端 detach 主路径：Prepare nonce、signed Revoke、child durable delete/receipt、Hub
> secret-free tombstone、Broker 三阶段 ledger、Service Detached/Finalize 与 Hub 重启恢复均已通过。Service 已增加
> 有界、不重入的后台 revocation reconciler；App 只负责发起/展示，不能再承担状态机正确性。全新 topology 已在
> credential version 2 重接后完成“App 只发起 detach 随即杀进程”的完整复测，worker 独立收敛为 Detached/Finalize，
> Hub 复位后无旧 access。pre-tombstone 掉电后的 nearby Admin recovery 已完成软件闭环和双板现场门禁：
> child 接受 Revoke 后、Hub 持久化 secret-free tombstone 前精确复位，Service 保持 Revoking；App 随后以 fresh Admin
> nonce 取得幂等 Revoke receipt，Broker recovered Apply 与 exact-active Finalize 收敛，Hub 再次复位后无旧 access。

本文后续使用两个不同术语，禁止继续用一个 `Gateway` 混指：

- **Edge BLE Gateway / Edge Hub**：部署在用户家中、具有 WiFi/Ethernet + BLE Central 能力的实体网关；
- **Cloud Gateway**：`aily-blinker-services/services/gateway` 中的公网控制面与 MQTT 入口服务。

## 1. 冻结结论

1. BLE 不是设备身份。MAC、广播名、bond/pairing 记录只用于发现或恢复 BLE 链路，不能作为
   `deviceId`、账号绑定或云端 ACL 的根。
2. 官方 BLE 的可移植安全基线不依赖 BLE Link Layer encryption：首次认领必须经过 Noise + server-signed
   transcript-bound grant，日常连接必须经过 `ControllerCredential`/Method 2，并把之后每个 BBP/2 frame
   封装为 DirectSecureRecord。平台可额外启用可靠的 pairing/bonding/link encryption，但不能把它当设备身份，
   也不能让其失败导致协议静默降级为裸 BBP/2。
3. 当前首次 enrollment 在 BLE 之上使用 Noise；X25519、HKDF-SHA256 和 AES-256-GCM
   由设备侧 crypto provider 计算。BLE controller 只提供 GATT bearer；产品若额外启用原生链路加密，
   它也只是纵深防御，不参与 Blinker 身份与授权判断。
4. 设备侧与 App 生产客户端均在 Method 2 成功后切换为 `DirectSecureRecord`；认证后的每个
   BBP/2 frame 都带方向、序号和 16 B AEAD tag。ESP32/XIAO、UNO R4 与 Wio 官方 RTL `2.1.3` 的纯 BLE
   三端核心门禁已经通过；Gateway 也已复用同一 exact wire 完成单 child 四端主路径及 pre-tombstone 双板真机
   crash recovery。Wio 仍是 opt-in preview，Gateway 的 encrypted-NVS 专用生产板门禁尚未关闭，因此不能宣称
   所有平台/Gateway 已 production ready。
   BLE v1 的 bearer packet 固定为 20 B；ATT MTU 变化不等于 BBP/2 分片协商，任一端不得据此单方面扩大分片。
   未来若需要大 MTU，必须先新增双方显式能力协商并冻结降级/重连行为。
5. 纯 BLE 设备、App 直连和 BLE 网关继续使用同一 BBP/2、Manifest、State、Command、ACK；
   网关只增加可信路由外层，不创造第二套组件协议。
6. Edge Hub 本身是正常联网设备，使用自己的 `DeviceKey -> SessionCredential -> MQTTS`；该产品 profile
   强制 TLS、pinned CA/受控 trust store 和 fail-closed，禁止 Plain MQTT 与静默降级。
   子设备不把 DeviceKey 交给网关，也不冒充 MQTT client；网关以已授权的 Gateway credential
   分别认证每个子设备，再代理其上下线和 BBP/2 数据。
7. `toDevice`/destination 语义保留。源设备可以提出目标设备或服务，但 Gateway/Broker 必须以
   已认证 session 覆盖 source、检查精确 ACL/route grant 后才转发；不得信任设备自报的
   `fromDevice`，也不得因同属家庭就获得整个家庭的设备访问权。
8. BLE GATT、BLE Mesh 和加密广播是三种不同产品形态。首版只实现 GATT 点对点；Mesh 和
   roaming beacon 不复用 GATT enrollment，不进入本轮代码。
9. 账号认领、Gateway topology attach 和当前可达路径是三个独立状态。没有网关时 BLE 子设备仍可由
   App 直连完成认领、控制和页面协商；增加或删除网关不得改变设备 owner，也不得清除 Admin/Shared。

### 1.1 广播识别与设备身份

当前 legacy BLE 广播合同已经足够让 App 判断“这是 Blinker 协议设备”，不再增加品牌字符串、MAC 派生值、
`logicalDeviceId`、ProductId 或 manufacturer data：

- 主广播携带固定 128-bit Blinker Service UUID；
- scan response 使用同一 UUID 的 13 B Service Data，格式为
  `formatVersion | mode | wireVersion | capabilities(le16) | modeLocator(8 B)`；
- App 必须同时匹配 exact Service UUID 并成功校验 versioned profile。只有名称相似，或者 UUID 相同但 profile
  非法的广播仍然忽略；
- Provisioning 的 `modeLocator` 是当前 setup window 的随机 `setupSessionLocator`；Direct v2 的 locator 为全零；
  Direct v3 的 locator 是 `nonce32 || tag32`。任何版本都不能把 MAC、广播名或平台 peripheral UUID 持久化成逻辑身份。

这一区分冻结为两个不同事实：

1. `BlinkerFamilyNearby`：扫描到合法 Blinker UUID/profile，只能驱动“发现附近 Blinker 设备”入口；
2. `LogicalDeviceNearby`：可由 Direct v3 authorized presence hint 私有匹配，用于更新某一设备卡片；若需建立
   authenticated/ready 状态，仍必须由对应 ControllerCredential 完成 Method 2。

2026-08-27 App 审计实际复现了一个 transport address 同时残留历史纯 BLE 与当前 WiFi+BLE 两份 logical credential；
旧列表扫描会把一条广播扩散成两张 nearby 卡片。当前止血规则冻结为：transport hint 唯一对应一个候选时才允许
列表显示 nearby；对应多个候选时全部保持未确认，不能任选最新、WiFiProv 来源或名称猜测。进入单设备页后以该页
ControllerCredential 执行 Method 2，成功才进入 ready。这个规则只作为 Direct v2 开发期兼容；Direct v3 正式启用
后，列表必须按 PresenceKey 匹配，不再依赖持久 transport address。

13 B profile 所在的 scan response 已达到 legacy 31 B 上限；当前主广播是 Flags + 128-bit Service UUID，约 21 B，
仍剩 10 B，最多再容纳一个带 AD 头的 8 B payload。该空间不是继续堆 ProductId、MAC 或明文设备号的理由，应保留给
Direct v3 authorized presence hint。它使用所有合法控制者都能获得的独立 PresenceKey 和轮换合同；不能直接复用
某一个 Admin/Shared controller secret，否则共享用户无法匹配且会把权限模型泄露到广播层。精确 transcript、golden
vector、key 生命周期和迁移顺序冻结于 `D:\codes\BLINKER_BLE_AUTHORIZED_PRESENCE_V1_CONTRACT.md`。

该 hint 只证明“持有授权 key 的 App 可把广播映射到某一 logical device”，仍不能替代 Method 2、
DirectSecureRecord 或控制权限校验。随机 nonce 不能证明新鲜性：没有可信时间、持久单调计数器或扫描 challenge 时，
捕获的 locator 可重放到 PresenceKey 轮换；因此 UI 不得据此显示“已连接”。

## 2. 当前实现的真实边界

### 2.1 `dev_edu`

`dev_edu` 的 ESP32/UNO R4 BLE 是 `FFE0/FFE1` GATT 透传：

- 手机直接连接设备；
- JSON 以换行、右花括号或超时判断完整性；
- ESP32 按 20 B 分片通知；
- 广播中携带从 MAC 生成的识别数据；
- adapter 没有配置明确的 BLE pairing policy，也没有 Controller challenge、消息序号、
  replay gate 或应用层加密。

它的优点是演示简单；缺点是 MAC 可能变化/冲突、消息边界不可靠、任意已连接 central 都可能写入、
没有稳定的账号/分享/网关授权模型。V2 不恢复这条 wire。

### 2.2 当前 Device V2

当前已经实现：

- 4 B BLE fragment header、定长 reassembly workspace 和 BBP/2 frame validation；
- ESP32 可把当前连接的原生 encrypted/bonded 状态作为纵深防御；UNO R4 正式 composition 不再依赖
  ArduinoBLE 2.0.2 不可靠的 Security Manager 配对，底层 GATT 可为明文 bearer；
- 首次 enrollment 使用 Noise NN/NNpsk0 secure record；
- `BleEnrollmentGrant` 将服务器授权绑定到 DeviceInstanceId、setup session、Noise transcript、
  accessEpoch 和 controller secret digest；
- 日常 Direct 使用 Controller Method 2 双向 HMAC challenge/proof；
- Method 2 成功后使用逐 session 派生的 DirectSecureRecord/AES-GCM 保护业务帧；
- Service 已实现双算法 signer、持久单次 intent/commit/cancel 和严格的 BLE enrollment HTTP 控制面；
- App 已实现 Noise enrollment、系统 secure storage、Method 2、DirectSecureRecord 及
  Manifest/State/Command/可靠 Patch ACK 产品会话；
- 设备只保存紧凑 access root，不保存 owner、家庭、账号或页面。

当前尚未实现：

- 同时容纳 Admin、Shared group 和 Edge Gateway 的第三个 credential slot；
- 子设备拓扑、网关代理 online/offline 和 route envelope；
- UNO R4 的 24 h 长稳、恶意断连/背压；Wio BLE 的认证超时/单连接占用 DoS、32 次重连和 30 min
  authenticated engineering soak 已闭合，只剩候选发布时真实 owner DirectSecureRecord 的 24 h 长稳；
- Product/OOB 的 attestation/OOB verifier；
- 一个 BLE peripheral 同时面对 App 和网关时的连接仲裁。

任何平台都不得把 `encrypted BLE + HMAC login` 描述成完整端到端安全通道；只有
`Noise enrollment + Method 2 + DirectSecureRecord` 的完整链路才能作为本方案的数据安全声明。

## 3. 可借鉴的行业做法

| 平台 | 当前公开方案 | 应借鉴 | 不直接照搬 |
|---|---|---|---|
| Blynk | 旧版曾支持 App 直连 BLE/SPP；当前文档将其标记为 Legacy。当前产品主线是 Cloud、Edgent、NCP，以及 hub/gateway tunneling；节点消息携带各节点 AuthToken 供 Cloud 识别 | Arduino API 简单；NCP 把连接能力从业务 MCU 解耦；Gateway 下节点仍有独立身份 | 不把长期 DeviceKey/AuthToken 明文复制到每个普通网关，也不恢复裸 JSON BLE |
| 涂鸦 | BLE 设备可连接 App 或 BLE gateway；普通设备可由 App 直接配对，也可由 App 在网关面板开启 permit-join、由网关发现后绑定到云；云端维护 gateway/sub-device 关系；还支持无需指定网关、仅上行的 roaming beacon | App/网关双入口、按需连接、所有权与拓扑分层 | 不引入 DP/JSON 第二套业务模型，不把“扫描到广播”视为有权认领，也不把全部实现锁进平台模组 |
| 阿里云 | 网关自己登录云；子设备有独立身份/拓扑，由网关代理认证、上线、下线和数据。官方 SDK 明确不定义 gateway-sub-device 本地协议 | 云端拓扑、子设备独立逻辑身份、网关代理 lifecycle | 不把 ProductKey/ProductSecret 映射或子设备 DeviceSecret 全部压到 Arduino 设备/网关 |
| 小米 | BLE 标准认证区分注册与登录，可使用设备确认、RSSI、二维码/键盘/显示 OOB；登录后产生本次连接会话密钥，业务 GATT 数据再调用 session encrypt/decrypt | BLE pairing 之外必须有应用身份；认证后逐会话保护业务帧；物理确认/OOB 可分安全档 | 不复制闭源认证库、固定芯片清单和产品审批绑定 |

官方资料：

- Blynk current topology 与 NCP：<https://docs.blynk.io/en/commercial-use/supported-topologies>、
  <https://docs.blynk.io/en/blynk.ncp/overview>
- Blynk Legacy Bluetooth：<https://docs.blynk.io/en/legacy-platform/legacy-articles/bluetooth-2.0-spp-4.0-le>
- Tuya BLE device/gateway：<https://developer.tuya.com/en/docs/iot-device-dev/bluetooth_software_ble?id=Kc6fip6lx8xfc>、
  <https://developer.tuya.com/en/docs/iot-device-dev/tuyaos-gateway-bluetooth?id=Kc5xcjmm5gfnd>、
  <https://developer.tuya.com/en/docs/iot-device-dev/tuyaos-gateway-join?id=Kc3234gqbhjjt>
- Alibaba gateway/sub-device：<https://help.aliyun.com/zh/iot/user-guide/gateways-and-sub-devices>、
  <https://help.aliyun.com/zh/iot/developer-reference/connect-a-gateway-to-iot-platform>
- Xiaomi standard BLE auth：<https://github.com/MiEcosystem/miio_open/blob/master/ble/02-%E6%A0%87%E5%87%86BLE%E6%8E%A5%E5%85%A5%E5%BC%80%E5%8F%91.md>

## 4. 身份、凭据与权限

### 4.1 身份

| 对象 | 稳定身份 | 说明 |
|---|---|---|
| BLE 子设备实例 | 16 B random `DeviceInstanceId` | 首次启动生成并持久化；MAC 仅发现 |
| 云端设备 | `logicalDeviceId` | Service 权威；映射 DeviceInstanceId、页面、ACL 和拓扑 |
| Edge Hub | 自己的 logicalDeviceId + DeviceKey | 正常联网设备，强制 MQTTS，不共享子设备 DeviceKey |
| App 用户 | account/user id | 只在 App/Service；不写入子设备 |
| 路由目标 | logicalDeviceId 或 service route id | 必须经过 route ACL，不是任意 topic 字符串 |

### 4.2 三类 Direct credential

底层 record 和 Method 2 可复用，但 role/slot 必须明确分开：

| role | 持有方 | 权限 | 撤销粒度 |
|---|---|---|---|
| Admin | owner App 安全存储 | Observe、Control、ProvisionNetwork、ManageControllers | 每设备 Admin |
| Shared | 被明确分享该设备的 App | Observe、Control | 每设备共享组；成员删除轮换该设备 Shared |
| Gateway | 被授权代理该设备的 Edge Hub | Observe、Control | 首版每设备一个 active Edge Hub；不得 ProvisionNetwork/ManageControllers |

Gateway 不能借用 Shared secret。否则移除一台网关会迫使所有 App Shared 用户轮换，而且网关攻陷会泄漏
用户直连凭据。但设备侧不新增 `PlatformGateway` domain：当前 exact wire 继续使用 `PlatformAccess`，设备只按
`controllerId + accessEpoch + credentialVersion + permissions` 鉴权；`Admin/Shared/Gateway` 的业务分类由 Service
元数据和 topology 权威维护。这样不增加 Method 2、DirectSecureRecord 或 ControllerGrant 的分支。

Gateway credential 与 cloud route ACL 都冻结为 `Observe|Control`，不得为了 detach 给它增加
`ManageControllers`。`ControllerControlOpen/ControllerMutation` 是服务器签名 Grant 的安全中继：peer 必须先以
Method 2 获得 `Control`，真正的 mutation 权限再由 fresh nonce、服务器签名以及 exact
device/controller/epoch/version 绑定决定。PresenceKey mutation 没有这层 ControllerGrant，因此仍只允许
`ManageControllers`。这既允许 Gateway 自撤销/轮换，也不会把 Admin/Presence 管理能力授予 Gateway。

当前实现已升级为三个 64 B slot，允许一个 Admin 和最多两个 `Observe|Control` credential，可以同时支持
Shared group 与 Gateway。冻结规则为：

- 官方默认/Gateway-compatible：Admin + Shared group + Gateway 三槽；按当前布局 serialized record 从 196 B
  增至 260 B，使用户以后增加网关时不需要重刷另一套 child firmware；
- 未来可选 Small：Admin + 一个可选择的 Shared 或 Gateway slot，但必须明确声明不能同时分享和绑定网关，
  不作为官方初学者默认 profile；
- 槽位数量只改变 durable access record，不改变公开组件 API 和 BBP/2。

schema v3 已实现读取 176 B schema v1、196 B schema v2，并在首次成功 mutation 时原子写出 260 B v3；
32-bit ABI、Host fault injection、ESP32/UNO R4/Wio 编译与静态资源门禁已通过。Wio、ESP32 与 UNO R4 的真实
断电 journal 和 v2→v3 升级迁移已全部通过，G2-0C 已关闭。

### 4.3 Gateway credential 的权威与密钥可见性

Gateway credential 是平台托管、可撤销的基础设施凭据，不等同于用户 Admin/Shared credential：

- Admin/Shared raw secret 继续保持 Service 零知情；服务器只保存 digest/授权元数据；
- Gateway raw secret 由 Service CSPRNG 生成，KMS/HSM envelope encryption 保存；首版每个 child、每个 active
  Edge Hub 一份，不称为 group secret；
- `ControllerGrant` 的 signed body 仍只携带 digest，不把 raw secret 写入 grant、receipt、数据库普通列或日志；
- App 只在 pending attach 期间经 HTTPS 取回并在受保护内存中 exact relay，不能写入普通 storage/analytics；
- Edge Hub 经专用 MQTTS 管理通道接收面向自身密封的 `GatewayAccessEnvelopeV2`，原子持久化 Method 2
  credential 与 Presence locator 后回 access-material digest ACK；
- topology 只有在 Edge Hub durable ACK、child durable receipt、Edge Hub 对 child 的 Method 2 三项都成功后才 active。

MQTTS 只保护客户端到 TLS 终止点。为避免 Cloud Gateway、Broker command ledger、trace 或内部转发看到 raw
secret，MQTT payload 不直接放明文 secret；Service 使用从 exact Edge Hub DeviceKey/version 经 HKDF 域隔离派生的
wrapping key（或后续等价设备公钥）对 secret 加密。业务含义是“经 MQTT 下发”，wire 上始终是密封 envelope。
本地生成 secret 的零知情方案保留为后续 Enhanced/Local profile，不进入首版 G2。

## 5. 两层加密和一次认证

### 5.1 BLE Link Layer（可选纵深防御）

BLE pairing/encryption 由 BLE stack/controller 执行，不由 Sketch 逐包计算。冻结边界是：

- Noise 与 DirectSecureRecord 必须能安全运行在普通 GATT、串口/NCP 等不可信 bearer 上；
- 平台只有在配对、当前连接加密事实和断电 bond 恢复均可验证时，才可额外要求 encrypted
  read/write/notify；
- 若平台声明本次 session 使用 link encryption，后续发生 security downgrade 仍必须立即退役 session；
- App 不主动创建系统 bond 作为通用流程，不使用 bond/MAC/peripheral UUID 选择逻辑设备；
- 原生配对失败不得回退到裸 BBP/2，而应继续使用冻结的 Noise/Method 2/DirectSecureRecord，或在该平台
  明确声明不支持。

不同硬件上的计算位置不同：ESP32 通常由同一 SoC 的 BLE controller/host 完成；UNO R4/Wio 可能由
连接模块/NCP 的 BLE controller 完成。用户 Sketch 不逐包调用 BLE AES-CCM。

`Just Works` 无论使用 SC 还是 Legacy pairing，都不提供抗主动 MITM 的用户确认。bond 只是恢复
link key，不是账号 ownership 或业务授权；Blinker 的应用层认证仍由 Noise/Method 2 完成。

### 5.2 首次 enrollment secure channel

当前 Noise 使用 X25519 + HKDF-SHA256 + AES-256-GCM。这部分由设备端 crypto provider 计算并占用设备
Flash/RAM/CPU，不是 BLE 协议自动提供。

冻结两个安全档：

| 档位 | enrollment | 保证 |
|---|---|---|
| Education/Nearby | physical setup window + Noise NN + server-signed transcript-bound grant；原生 BLE encryption 可选 | 抵抗被动监听和未授权 grant；不宣称具有独立 OOB 设备认证 |
| Product/OOB | Noise NNpsk0，PSK 来自随机 128-bit QR/setup secret；或后续等价 PAKE/OOB；原生 BLE encryption 可叠加 | 提供明确目标设备认证，生产增强档 |

6 位数字不能直接扩展成“32 B 高强度 PSK”；若未来只允许短数字，应使用适合低熵口令的 PAKE，不能靠
hash/KDF 字节数制造虚假熵。无 OOB 的教育板可以保持简单模式，但 App 必须显示安全级别。

### 5.3 日常 Direct secure record（必须新增）

Method 2 继续作为统一 App/Gateway 登录，不重新发明第二套握手。认证完成后双方从 controller secret、
client/device nonce、accessEpoch、credentialVersion 和 exact auth transcript 通过 HKDF-SHA256 派生。当前
设备 ABI/Method 2 transcript 中该 `accessEpoch:u32be` 字段仍命名为 `ownershipGeneration`；二者是同一 wire
字段，不得在 App/Service 再增加第二个 epoch：

- App/Gateway -> Device AEAD key + nonce prefix；
- Device -> App/Gateway AEAD key + nonce prefix；
- 独立 session binding digest。

后续每个 BBP/2 frame 包装为固定 `DirectSecureRecord`。原草案遗漏长度字段，无法让 4 B BLE fragment
reassembler 在解密前确定完整 record 大小和存储上限，现修正并冻结为：

```text
magic 0xD3:u8
| version 1 / flags 0:u8
| plaintextLength:u16be
| sequence:u32be
| ciphertext(BBP/2 frame):plaintextLength
| AES-GCM tag[16]
```

固定 header 为 8 B，固定总开销为 24 B。`plaintextLength` 至少为 BBP/2 10 B header，产品最大值为
`min(BLINKER_DEVICE_FRAME_SIZE, 65511)`；outer record 上限为 frame size + 24 B。未知 version/flag、长度不一致、
trailing bytes 均 fail-closed，裸 BBP/2 与 secure record 可由首字节明确区分。

Method 2 KDF 固定为 HKDF-SHA256：salt 是
`SHA-256("BLINKER-DIRECT-AUTH/1" || exact common Method-2 transcript)`，info 是
`"BLINKER-DIRECT-SESSION/1"`，IKM 是 32 B controller secret。96 B 输出依次切分为 initiator send key、
responder send key、两个 8 B nonce prefix 和 16 B session binding。App/Gateway 是 initiator，设备是 responder；
AES-GCM nonce 为方向专用 prefix + `sequence:u32be`。AAD 是
`"BLINKER-DIRECT-RECORD/1" || direction || sessionBinding || exact 8 B header`。

要求：

- 双方向独立、从 0 开始的严格 sequence；
- nonce 由方向专用 prefix + sequence 构造，绝不复用；
- AAD 绑定 protocol version、direction、session binding 和明文长度；
- tag 验证和 replay gate 在 BBP/2 parser 之前；
- reconnect 必须重新 Method 2、重新派生 key，旧 record 不可跨 session 重放；
- 固定 caller-owned workspace，不增加 heap；优先复用当前 Noise 已使用的 AES-GCM/HKDF provider。
- capacity/queue preflight 失败不得消费 sequence；tag、方向、session binding 或 sequence 失败立即退役 secure session；
- 最终 `AuthResult(Authorized)` 仍是最后一个 pre-secure BBP/2 frame；只有它成功进入发送队列后才切换 secure record。

16 B tag 会增加 BLE 分片数，但换来统一覆盖 Just Works、恶意 relay/injection 和未来串口/NCP bearer 的
应用数据完整性与机密性。冻结 wire 前以真实 20 B ATT payload、常见 Patch/Command/ACK 做大小基准；不要
为了少一片先截短 tag。

### 5.4 Edge Hub 云链路与 access delivery

普通 Arduino WiFi 设备仍可按资源 profile 选择 Plain/TLS；Edge Hub 是不同产品档，必须满足：

- DeviceKey login 的 HTTPS 控制面和后续 MQTT 数据/管理面均使用 TLS；证书失败、时间/CA 无效或 TLS 内存不足
  均保持离线，不回退 Plain；
- access delivery 下行使用独立 management topic/消息类型、QoS 1、`retain=false`、固定大小与幂等 operationId；
  上行 durable ACK 接受 QoS 0/1，可靠性由 exact-envelope ledger 重投和设备幂等记录闭合；
- `GatewayAccessEnvelopeV2` 绑定 environment/tenant、operationId、gateway logicalDeviceId、DeviceKey version、
  child logicalDeviceId、child DeviceInstanceId、accessEpoch、controllerId、credentialVersion、permissions、
  PresenceKey version 与 expiresAt；
- envelope 使用 96-bit 随机 nonce 和 AES-256-GCM，所有非密文字段进入 AAD；相同 operation 重发 exact envelope，
  operationId/nonce 不能跨不同明文复用；
- Edge Hub 只有在 envelope、expiry、exact identity/version 和 AEAD tag 全部通过后才原子保存 48 B access
  material，并以 `operationId + accessMaterialDigest + storageRevision` ACK；
- Broker/ledger 只保存 envelope ciphertext 与非敏感状态，日志只允许 operationId/digest 前缀，禁止 secret、
  DeviceKey、wrapping key、完整 envelope dump。

Edge Hub 的 MQTTS session 证明“这台 hub 当前在线”，envelope AEAD 证明“这份 secret 只交付给 exact hub”；
两者缺一不可。TLS/mTLS 不替代 topology ACL，envelope 也不授权尚未 active 的 child route。

**2026-08-30 合同修正已完成：**测试阶段 V1 已移除，单一 `GatewayAccessEnvelopeV2` plaintext 为 32 B
Gateway secret + 16 B PresenceKey，PresenceKey version 进入 AAD，ACK 使用 exact access-material digest。
PresenceKey 由 Service KMS 托管；attach 时 Service 内部解封 exact current/confirmed key，密封给 exact Hub 后清零
内存，App 不提交 raw key。MAC、广播名、Gateway secret 或“扫描后逐台盲试”都不是替代方案。

PresenceKey 可独立于 Gateway credential 轮换。轮换采用 active+pending 两 locator 的有界两阶段事务：先让 Hub
durable stage 新 locator，再让 child durable 安装，最后以新 locator + Method 2 proof 激活并删除旧 locator；失败
或取消只清 pending。这样不会因先覆盖 Hub 旧 key 而失去 child，也不需要保存 transport address。

## 6. 完整流程

### 6.1 纯 BLE 首次认领

```text
physical setup window
  -> App discovers locator (MAC is not identity)
  -> GATT bearer (native link encryption optional)
  -> Noise NN or Product/OOB NNpsk0
  -> Device Hello(DeviceInstanceId, setupSessionId, transcript, accessEpoch)
  -> App account asks Service for one-time BleEnrollmentGrant
  -> App relays grant + fresh Admin secret inside Noise
  -> Device verifies exact context/signature/digest and atomically stores Admin
  -> Method 2
  -> switch to DirectSecureRecord
  -> Manifest/State/Command over BBP/2
  -> App commits receipt; Service creates logicalDeviceId/ACL/Page records
```

正常重启只使用持久 Admin/Shared credential，不再依赖已过期 grant。

### 6.2 App 日常直连

```text
App account ACL says user may access this exact logicalDeviceId
  -> App loads this device's Admin or Shared secret from Keychain/Keystore
  -> GATT bearer (native link encryption optional)
  -> Method 2 challenge/proof
  -> DirectSecureRecord session
  -> BBP/2 HELLO + Manifest fingerprint negotiation + state/command
```

Service ACL 立即停止官方 App 的云端权限和新 secret 交付；纯 BLE 离线设备无法被服务器瞬时改写，已经缓存
旧 Shared secret 的客户端要等附近 Admin 完成轮换后才失效。这一物理限制保持明确。

#### 6.2.1 没有网关时

没有网关不是异常状态，也不改变设备类型：

- 未认领子设备按 6.1 由 App 直连认领；已认领设备按 6.2 直连控制；
- Manifest、State、Command 和页面协商均可在本地 BBP/2 Direct session 中完成；
- App 可在前台连接期间把数据同步到 Service，但这只是临时 phone relay，不宣称后台常在线；
- App 离开 BLE 范围后，设备继续执行本地逻辑，但远程控制、云自动化和实时云历史不可用；
- 日后增加网关只新增 topology/Gateway credential，不重新认领、不更换 logicalDeviceId，也不要求恢复出厂。

### 6.3 Edge Hub 自身上线

```text
Edge Hub DeviceKey + network
  -> DeviceKey challenge/HMAC
  -> short SessionCredential
  -> mandatory MQTTS CONNECT/SUBSCRIBE (no Plain fallback)
  -> Edge Hub HELLO/Manifest/State
  -> Service loads hub capability and exact child topology
```

Edge Hub cloud session 只证明“这台 hub”；不能自动证明它代理的任意 child。

#### 6.3.1 Edge Hub 自身配网

Edge Hub 是“普通联网设备 + BLE Central extension”，因此复用普通 ESP32 DeviceKey/WiFi onboarding，不定义
`GatewayKey`、网关专用 WiFi wire 或第二个 App 配网页面。首版提供两个入口，但只汇合到一个持久状态：

| 入口 | 用途 | 持久结果 |
|---|---|---|
| WiFiProv | 默认用户路径；ESP32-S3/IDF-NimBLE 默认 BLE Security1，SoftAP 为编译期回退 | protected DeviceKey store + WiFi credential journal |
| 手动 DeviceKey/SSID/password | Arduino 教育、调试、无 App 现场 | 同一 protected store/journal，不形成平行登录图 |

WiFiProv BLE 只在 provisioning window 中把 ESP32 作为 Peripheral；完成 DeviceKey + WiFi 各自原子且可恢复的
durable commit 后必须完整
deinit，再启动 Edge Hub 的 BLE Central child extension。BLE provisioning、BLE Central 和未来 Hub 自身 Direct
Peripheral 不得同时抢占 Host/RF 生命周期。手动入口不启动 WiFiProv，但必须经过同一 DeviceKey challenge、
MQTTS、可信时间和 capability admission。

重置继续拆成三个动作：network reset 只清 WiFi 并重新打开 provisioning，保留 DeviceKey、Hub identity、child
topology 和 Gateway secret；DeviceKey rotate 使旧 cloud session 立即失效但不自动删除 child topology；factory/
ownership reset 才清 Hub access root 和 per-child secret。不得把按钮长按统一解释为三者同时发生。

#### 6.3.2 App 控制 child 的路径选择

child 的 `logicalDeviceId`、Manifest fingerprint、State revision 和 requestId 不随路径变化；变化的只是认证 session
和 transport sequence。App/Broker 按以下顺序选择**已经认证且可用**的 lane：

1. App 已与 exact child 建立 DirectSecureRecord：直接 BLE；
2. 未来已与 exact device/Edge Hub 建立授权 LAN session：LAN；
3. 账户 MQTT-over-WebSocket 在线且 active topology 有可达 Hub：Cloud route；
4. 都不可用则明确离线，不因扫描到广播或同一 SSID 猜测可控。

“App 在网关附近”不等于 App 连接网关 BLE 后代理 child。G2A 首版是 App 直接连接 child；Hub 默认按需连接并在
App Direct window 让渡。远程命令才走 `App account MQTT -> Broker ACL/topology -> Edge Hub MQTTS -> child
DirectSecureRecord`。未来 LAN Hub proxy 必须有 exact child `LocalRouteGrant`，不能用 Hub 自身登录自动扩大到所有
child。

每个 mutation 在发送前选定一条 lane，并以同一 16 B requestId 固定到终态。发送后若 ACK 不确定，App 只能在
原 lane 查询/重试 exact request；不得改走 BLE/LAN/Cloud 再执行一次。读取类请求可并发探测多个 lane，但只接受
通过 Manifest gate 且 revision 最新的结果。

#### 6.3.3 未来 LAN Direct 边界

LAN 不新增 JSON/DP/REST 组件协议：

```text
UDP/mDNS discovery (untrusted hint only)
  -> WebSocket/TCP binary connection
  -> Method 2 / signed LocalRouteGrant
  -> DirectSecureRecord
  -> BBP/2 HELLO + Manifest + State + Command/ACK
```

- UDP/mDNS 只发现 `physicalDeviceId` hint、能力与端口；禁止携带 secret、权威 online 或控制 mutation；
- WebSocket 是 framing/背压层，不是授权；低资源设备可用明文 `ws://` + DirectSecureRecord，高资源产品可选
  `wss://`，二者应用层认证合同相同；
- DeviceKey、MQTT SessionCredential 不进入 LAN；设备复用 Admin/Shared controller domain，但 LAN 与 BLE 建立
  独立 session key、sequence 和 replay window；
- G2A 不实现 Hub LAN proxy/offline child ACL。它属于 G3，必须由 Service 签发 exact user/child/Hub/权限/epoch/
  expiry 的 `LocalRouteGrant`；需要断网控制时只能使用事先缓存、可验证、短期且不可扩权的 grant；
- UDP telemetry/broadcast control、跨网段发现、Mesh 和自动 roaming 分别立项，不能借 LAN 名义进入首版。

### 6.4 将 BLE 子设备加入网关：两种入口、同一个 ownership 结果

网关发现、账号认领和 topology 激活必须分开。网关可以扫描未认领广播，但在 App 明确选择、设备处于物理
setup window、Service 签发 exact grant 前，不得为设备创建 ownership 或 active topology。

#### 6.4.1 App 先认领、再绑定网关（G2A 最小实现）

适用于已在手机附近使用的设备，也是首个联调闭环：

1. 未认领设备先按 6.1 由 owner App 直连认领，安装 Admin；
2. owner 在 App 选择一台已上线、属于允许范围且具有 BLE gateway capability 的 Edge Hub；
3. Service 创建 pending topology，并为 exact child + exact Edge Hub 生成 controllerId/32 B Gateway secret；secret
   只以 KMS ciphertext 保存；
4. Service 通过专用 MQTTS management topic 下发 `GatewayAccessEnvelopeV2`；Edge Hub 将 Gateway secret 与
   current PresenceKey/version 原子保存后返回 access-material digest ACK；
5. App 通过 owner HTTPS pending-attach API 获取短期 relay package，在内存中持有 raw secret；
6. 已完成 Admin Method 2 的 App 打开 fresh ControllerControl window；Service 签发绑定 child DeviceInstanceId、
   accessEpoch、fresh controlNonce、controllerId/version/permissions/secretDigest 的 grant；
7. App 在 child DirectSecureRecord 内 exact relay grant + Gateway secret；child durable mutation 后返回 receipt；
8. App 提交 receipt，Service 保持 pending，并命令 Edge Hub 首次连接 child；
9. Edge Hub 使用 Gateway credential 完成 Method 2 + DirectSecureRecord，并上报 exact proof；三项证明齐全后
   Service 原子激活 topology，才允许 child route 和 `reachableViaGateway`。

App 可以在安装过程中短暂 relay raw Gateway secret，但不得持久化它。App kill、MQTT QoS 1 重投或任一 ACK
丢失都通过 operationId/receipt digest 恢复同一 pending operation，禁止另生成 secret 后覆盖半完成事务。

#### 6.4.2 网关辅助首次认领（G2B 正式网关发布前完成）

用户在 App 的网关页面点击“添加子设备”时，可以得到类似 Tuya permit-join 的体验，但信任边界保持不变：

```text
authenticated App -> Service opens short permit-join for exact Gateway
  -> physical child enters setup window and advertises unclaimed capability
  -> Gateway scans/connects and relays BLE fragments opaquely
  -> Noise and BleEnrollmentGrant remain end-to-end App <-> child
  -> child stores Admin and returns durable receipt
  -> Service prepares and delivers the managed Gateway credential as in G2A
  -> App uses the new Admin session to install that Gateway credential
  -> Gateway proves Gateway Method 2
  -> Service atomically changes pending topology to active
```

Gateway 在 enrollment 阶段只是 transport relay：不得终止 Noise、重编码 signed grant、读取或保存 Admin secret。
这使 G2B 继续复用现有 Hello、Noise、`BleEnrollmentGrant`、receipt 和 ControllerControl wire，不新增“Gateway
ownership grant”。两个持久步骤不强行伪装成一个事务：若 Admin 已安装而 Gateway credential 安装失败，设备
仍是 owner 可直连的正常设备，App 以后重试 attach；只有 Gateway credential durable ACK 且网关认证成功后，
topology 才能 active。

G2B 不是无人值守抢占。它仍要求登录 App/账号授权、指定网关、短 permit-join 和设备物理 setup window。
已被其他账号强绑定的设备即使被扫描到，Service 也必须拒绝认领/attach。未来真正 headless/bulk commissioning
应作为单独 Product profile，不得复用教育设备的默认流程。

G2B-0 已冻结 adapter-scoped 窗口管理 wire。它复用 Edge Hub 已认证 MQTTS management topic，canonical CBOR
fixed array 为：

```text
PermitJoinCommand = [1, 5, operationId:bstr16, adapterId:uint16, action, expiresAt]
PermitJoinResult  = [1, 6, operationId:bstr16, adapterId:uint16, status, expiresAt]

action: Open=1, Close=2
status: Ready=1, Closed=2, Expired=3, Rejected=4, Busy=5, Unsupported=6
adapterId: NativeGatt=1；其他非零 uint16 为后续 additive registry
```

两种消息最大均为 33 B，不携带 account、tenant、child identity、MAC、DeviceKey、ControllerCredential、
Admin secret 或候选广播。exact account/tenant/Hub 绑定属于 Service ledger + MQTT session；同一 operationId、
adapterId、expiresAt 的重投必须幂等，不同 operation 在活动窗口期间返回 Busy。Hub 重启后本地窗口为空，只有仍未
过期且由 Broker 重投的 exact Open 才可重新开启。App 不消费 management topic，因此不复制这套 CBOR codec；它在
G2B-3 只实现 HTTPS/relay 合同。

G2B-1 控制面已完成。公开入口固定为 owner 对 exact Hub/adapter 创建 300 s 窗口、读取 operation、幂等 Close；
Service 以 PostgreSQL partial unique index 保证单 Hub 单活动窗口，Broker 以 SQLite ledger + QoS 1、
`retain=false` 保存并重投 exact 33 B 命令。result 必须来自相同 tenant/Hub 的已鉴权 management session；
Service 与 Broker 都独立执行 expiry，终态不能倒退。该表刻意没有 child identity、MAC、候选列表、ownership、
topology 或 secret，因此“打开窗口/扫描到广播”仍不会创建设备或绑定关系。G2B-2a 前设备不会执行扫描。

G2B-2a 已完成设备软件闭环：management client/mux 消费 PermitJoin command，平台无关 coordinator 负责 exact
幂等/expiry/result；ESP32 `NativeGattPermitJoinAdapter` 在一个显式 southbound lease 内复用现有 NimBLE Central。
Adapter 最多缓存 4 个 rotating setup/presence-locator 候选，只允许 App 后续选择 locator 后连接；双向各 4 个、每个最多
20 B 的队列只搬运 BLE fragment，溢出即关闭窗口而不是静默破坏 Noise transcript。它不解析 Noise/Grant/receipt，
不暴露 MAC，也不持久化候选/窗口；`Closed/Expired` 对同一 operation identity 单调，旧 `Open` 重放不会重新开窗。
共享 Central 的 callback/stop 权仅属于当前 lease owner。G2B-2b 已冻结并实现 operation-bound northbound
candidate/select/fragment relay；在 G2B-3 App transport 和 G2B-4 失败/真机门禁完成前，设备扫描能力仍不作为
公开产品功能开启。

G2B-2b 不复用要求 active topology 的 `ChildRouteEnvelope`，而在同一 authenticated Hub management session 上增加
kind 7..11：CandidateSnapshot、SelectCommand/Result、RelayBatch/Ack。所有消息绑定 exact 16 B operationId；
候选 token 为 1..16 B adapter-scoped 临时句柄（Native GATT 为 8 B setup locator），最多 4 项，不含 MAC。2026-09-02
四端门禁已否决逐 ATT20 packet 云端 stop-and-wait：现场 1.3～1.9 s/片会与 2 s record 重组及 15 s 未认证门禁
确定性冲突。修订后每个 batch 最多携带 16 个 1..20 B 原始 packet，sequence 从 1 开始，每方向只允许一个未 ACK
batch；最大 512 B logical record 至多两次云往返，完整 wire 保持约 400 B 上限。Gateway 仍只做 opaque packet pump，
不解析/拼接 Noise 或 BBP/2。Broker 是有界 stop-and-wait mailbox，不持久化 payload；Broker restart 后
`relaySessionId` 必须变化。由于应用协议没有远程强制断开 child GATT 的
旁路命令，App 必须立即废弃旧 Noise、关闭旧 operation，并以新 operation/新物理 BLE 会话从 Hello 开始；不能在
旧连接的未知 transcript 状态上继续。

Native GATT 与 Gateway relay 共用的 DirectSecure 发送器以“分配 sequence、AES-GCM 加密、完整 record 交付”为
一个串行临界区。receive loop 生成的可靠通知 ACK 不得与前台 request 并发加密或交错发送；否则既会复用 AEAD
nonce，也会在 Gateway 下争用单 down-batch mailbox。串行化属于 App 安全通道合同，不通过新增 wire ACK、放宽
Broker 单飞或无限重试掩盖。

App 只持久化无秘密的 permit-join 清理检查点（operationId/exact Hub/adapter），用于进程重启后 Close 遗留窗口；
candidate、MAC、Noise transcript 和 fragment 均不得落盘。若 child 已切换 Direct 但 App 尚未完成 enrollment commit，
恢复路径必须使用本地 pending Admin credential + authorized presence 重新发现并调用现有 resume，而不是重新假设
设备仍在 Provisioning。

Gateway attach 的恢复按 cloud/child/cloud 三段执行：sealed secret 尚未被 Hub durable ACK 时不连接 child；只有
`PendingChildInstall` 才开启有界 Admin BLE 会话；receipt 之后的 `PendingGatewayProof` 不再复用该会话。任何 UI
“继续确认”都从持久 operation 重新协调，不能复用上一次已关闭的 `BleDirectSession`。

#### 6.4.3 状态机

```text
Unclaimed --App direct enrollment------------------> AppOwnedDirect
Unclaimed --Gateway opaque relay + App enrollment--> AppOwnedDirect
AppOwnedDirect --install Gateway credential--------> GatewayAttached
GatewayAttached --detach/revoke Gateway------------> AppOwnedDirect
AppOwnedDirect/GatewayAttached --physical reset----> Unclaimed
```

`DirectConnected`、`GatewayReachable`、`Offline` 是瞬时 reachability，不是上述 ownership/topology 状态。对于通常
只允许一个 central 的 BLE peripheral，默认网关按需连接并在 App 请求直连时让渡；不能把 “GatewayAttached”
解释成网关必须永久占用连接。

### 6.5 网关代理通信

#### 6.5.1 南向插件边界（2026-09-01 冻结）

Edge Hub 的“核心可复用”不等于所有无线制式共享一个万能 `IChildLink`。真正稳定的边界分为三层：

```text
EdgeHubCloudCore
  DeviceKey/MQTTS + topology/ACL/ledger + ChildRouteEnvelope

SouthboundProtocolAdapter
  NativeBlinkerSecureAdapter   // BBP/2 + Method 2 + DirectSecure
  ZigbeeModelAdapter           // APS/ZCL + Zigbee 自身安全
  BleMeshModelAdapter          // Mesh Model + Mesh 自身安全

Bearer/PlatformPort
  GattRecordLink | EspNowRecordLink | SerialNcpRecordLink
  ZigbeeCoordinatorPort | BleMeshProvisionerPort
```

- GATT、ESP-NOW、串口/SPI 网络协处理器若承载 Blinker 原生 logical record，只替换 `Link + Selector`，复用 Method 2、
  DirectSecure、BBP/2、GatewayAccessStore、route bridge 和 northbound；
- Zigbee/BLE Mesh 节点保持各自 provisioning、网络密钥、地址、sequence 和标准 model，不实现假的 Method 2/GATT
  fragment；它们在 `SouthboundProtocolAdapter` 将属性、动作、事件映射到 Blinker canonical model；
- discovery/permit-join 属于 adapter-scoped policy。扫描结果只是候选，不能直接创建 ownership/topology；
- `EdgeHubProduct` 使用 composition 按能力装配模块，不从 BLE 网关类派生 Zigbee/ESP-NOW 网关，也不在核心中增加
  `if (bearer == ...)`；未被装配的 adapter 不链接进 Arduino 固件。

当前代码已将 raw BBP/2/DirectSecure logical record 从 BLE transport 抽到 `protocol/DirectRecord`，并将 native child
session 改为 `BasicGatewayChildSession<Link, Selector>`；`GatewayGattChildSession` 只是 ESP32 GATT 的具体组合。这关闭了
ESP-NOW/serial-NCP 复用的首个架构门禁，但 Zigbee/BLE Mesh 的 model adapter 尚未实施。

```text
BLE child <-> GATT + Method 2 + DirectSecureRecord <-> Edge Hub
Edge Hub <-> DeviceKey cloud session/MQTTS <-> Cloud Gateway/Broker
```

上行：

1. Gateway 从已认证 child session 得到 source DeviceInstanceId/accessEpoch/credentialVersion；
2. 解开 DirectSecureRecord 后验证 BBP/2；
3. 构造 server-facing 最小 `ChildRouteEnvelope`，source 只能来自 session，不能来自 child payload；
4. Broker 检查 gateway-child active topology、目标 route ACL、frame 类型、大小和速率后转发；
5. Service/Broker 仍按 child logicalDeviceId 存状态、历史和在线状态，不把所有数据记到 Gateway 设备下。

下行：

1. Broker 根据 child topology 选择在线 Edge Hub，并在 envelope 中绑定 target child；
2. Edge Hub 只向 active topology 中、已完成 Gateway Method 2 的 child 发送；
3. child ACK 经同一路径返回，correlation/ledger 仍由 BBP/2 与 Broker 管理；
4. Gateway 不得把云命令降级为未认证 GATT write。

首版 Edge Hub 对 child 使用按需连接，完成交换后断开，避免长期占住只支持一个 central 的教育设备。需要
低延迟的产品可选择 persistent profile，但必须实现 App 请求时的连接让渡、超时和恢复。广播上报、roaming
和多 Gateway 抢占延后。

### 6.6 设备到设备、设备到服务

保留 `toDevice` 思想，但冻结为受控 route request：

```text
RouteRequest {
  destinationKind: Device | Service,
  destinationId,
  ttl,
  correlationId,
  innerBbp2Frame
}
```

- source 不上 wire 或只作诊断；Broker/Gateway 必须用已认证 ingress session 覆盖它；
- destination 可以由用户代码显式选择，也可以使用 Service 预配的短 route id；
- source->destination、允许的 message kind/endpoint、方向和速率都必须有精确 ACL；
- 两台联网设备由 Broker 路由，各自只持有自己的 DeviceKey；
- BLE child 的本地自动化由 Gateway 路由，每个 source/target session 分别认证；
- 发给 IFTTT/Agent/automation 的消息使用 Service destination，不伪装成另一台设备；
- Gateway 可缓存签名的离线 automation policy，但断网时不得自行扩大目标集合；恢复联网后对账版本；
- 不向源设备分发目标设备 DeviceKey、Admin/Shared/Gateway secret。

## 7. 连接仲裁与在线状态

首版 BLE peripheral 假设最多一个 active central：

- App Direct window 优先；Gateway 默认 non-persistent，发现 App/physical-direct 标志后让渡；
- Gateway 连接使用短 lease，空闲或完成 command/telemetry exchange 后断开；
- App 退出/杀进程触发 BLE disconnect，设备立即清 session key、sequence、authorization 和临时 workspace；
- Gateway disconnect 只改变 child connectivity，不改变 ownership/topology；
- cloud online 状态区分 `reachableViaGateway`、`directAppConnected` 和 `lastSeen`，不能把广播可见当已认证在线；
- 多网关首版只允许 Service 选择一个 active relay；故障迁移需要新 Gateway session，不共享连接序号。

## 8. 重置、撤销与密钥轮换

| 操作 | Cloud | App Direct | Gateway Direct |
|---|---|---|---|
| 移除单个分享用户 | 立即撤该设备 ACL | Shared rotation 完成前 eventual | 无影响 |
| 移除 Gateway topology | 立即拒绝该 gateway-child route | 无影响 | Gateway credential rotation/erase 完成前 eventual |
| DeviceKey refresh | 旧 cloud session 主动断开 | 不改变 Admin/Shared | 不改变 Gateway role |
| network reset | Gateway/WiFi 重配 | Direct access 保留 | child topology 保留 |
| factory/access reset | 设备云/本地 access root 清除或按产品策略重建 | 旧 epoch 全失效 | 旧 epoch/credential 全失效 |

`access reset` 的设备合同冻结为：停止当前 Cloud/Direct session，原子擦除 `DeviceAccessStore`，保留
`DeviceInstanceId`、WiFi credential 和平台能够可靠保存的 BLE bond，然后重新启动到 Provisioning。BLE bond 只是可复用的链路密钥，
既不证明 ownership，也不需要用户在普通重启或 access reset 前去系统设置“遗忘设备”。同一 owner 可通过新的
enrollment 覆盖 App 端旧 `ControllerCredential`；换账号必须先完成 Service ownership release，物理按钮不能
绕过服务器 ACL。

设备公开入口冻结为两个正交操作：`Blinker.resetAccess()` 擦除设备 access root；
`Blinker.resetNetwork()` 只清可写的 WiFi credential journal 并重新进入配网，保留 DeviceKey、
DeviceInstanceId、Direct access 与 BLE bond。BLE-only 不具备 network reset；手动 WiFi-only 的网络信息来自
Sketch、不是可写 journal，两个 reset 均按其能力返回 `UnsupportedFeature`。DeviceKey refresh、ownership
release 与清 BLE bond 仍是独立动作，不能都塞进一个含糊的 `reset()`。

纯 BLE child 离线时不存在“服务器点击后立刻擦掉设备 secret”的魔法。UI 必须区分 Cloud revoke complete 与
Direct rotation pending。

### 8.1 Active Gateway credential renewal

Gateway credential 的续期不能复用 DeviceKey refresh，也不能把 PresenceKey 的 active+pending locator 当成
secret 轮换。三者分别保护 Cloud 登录、匿名发现和 Hub→child Method 2，生命周期与失败边界不同。

冻结的最小事务为：Service/KMS 生成 version `N+1` -> exact Hub sealed delivery -> Hub protected pending journal ->
Hub 以 active version `N` 取得 fresh nonce -> child signed `Rotate(N -> N+1)` -> Hub durable receipt/promote ->
version `N+1` Method 2 proof -> Service CAS commit/route reopen。开始 child mutation 前，Broker 必须暂停新业务
command；完成新 proof 前不得让旧 topology route 恢复转发。

失败语义以 child 的 durable mutation 为不可逆分界：此前可以丢弃 pending 并继续使用 version `N`；此后旧 secret
已失效，只允许依靠 Hub 持久保存的 candidate 和 exact Grant/receipt向前恢复，状态应称为
`ForwardRecoveryRequired`，不能显示为“已回滚”。active topology 同时只允许一个 renewal operation，所有 CAS 必须
绑定 operationId、Hub/child identity、accessEpoch、controllerId、old/new version、candidate digest 与 deadline。

为避免扩大所有 Arduino 产品的常驻资源，pending credential/receipt journal 与 coordinator 只属于 Edge Hub
composition；普通设备 API 与 child 的三槽 `DeviceAccessStore` 不变。首版 Hub replacement 采用
`detach/revoke old -> attach/install new` 的明确有人值守流程；无人值守的低中断 replacement 在上述 renewal
完成后再增加 new-Hub exact seal/proof，不引入同时 active 的多 Hub。

R1 wire 已冻结为独立 operation：密封 envelope 同时绑定 renewal/topology operation、Hub/child identity、
old/new version、当前 PresenceKey version 与 candidate material digest；版本只允许 `N→N+1`。management kind
固定为 `12=command`、`13=result`、`14=durable pending ACK`，不占用既有 permit-join 的 7～11。事务只增加
Prepare/Apply/Finalize/Cancel 四阶段，Apply 与 Finalize 之间复用既有 GatewayProof，而不是再创造一套 proof。
R2 当时已实现 Edge-Hub-only 360 B protected pending journal 与独立 renewal AEAD domain/open；journal 与 352 B active
record 分离，并在首次 Rotate write 前持久化不可逆 marker。candidate open 还绑定当前 PresenceKey 的 material
digest，QoS envelope replay 保留原始 delivery revision。该完成状态只允许 Hub 安全接收并恢复 candidate，尚未
实现 signed Rotate/coordinator 或 Cloud route pause/CAS，不能对外声明 active renewal 已可用。

R3 已补齐设备侧可恢复 transaction：renewal delivery 与 initial access 通过 exact-wire mux 解耦；Hub 校验 signed
Rotate binding，在首次 child write 前 durable 标记不可逆边界，验证 candidate-secret HMAC receipt 后依次
`ReceiptStored -> active CAS promote -> Promoted`。Revoke/Rotate 共用 route bridge 的唯一 ControllerControl side
channel 与 sequence allocator，由 management mux 分发响应。promote 后旧 child session 必须退役；Gateway proof 不再
只看 PresenceKey，而同时校验该 DirectSecure session 建立时实际使用的 credential version，杜绝旧会话冒充新 proof。
这仍只是设备侧事实：Broker route pause/phase ledger、Service/KMS CAS、新 version proof 编排和四端故障注入属于 R4，
完成前不能开放 active renewal 产品能力。

R4a 已将云端第一条权威边界落地：Service 使用独立 renewal operation ledger 持久化设备定向 exact envelope 与 KMS
ciphertext，active topology 不增加临时状态。非终态 operation 通过 active-route 查询的 `NOT EXISTS` 条件暂停
child/hub 路由，因此 Broker 每帧既有 route revalidation 会跨进程重启保持 fail-close；不使用可丢失的 Broker 内存
pause 标志。Hub 当前 DeviceKey version 用于新 envelope，旧 attach 时的 DeviceKey version 不再被错误当作续期前置。
当前该 ledger 尚未接公开入口；必须先完成 R4b Broker kind 12/13/14 durable delivery/phase ledger，再由 R4c Service
worker 完成 receipt 校验、新 version proof、topology+operation 原子 CAS、Finalize ACK 与 route reopen。

R4b 已完成 Broker durable 传输层：renewal delivery 与四阶段 command/result 分表持久化，复用唯一 Hub management
topic、限流和 dispatcher，按 QoS 1/retain false 重投 exact bytes，并在 Broker restart 与 Hub reconnect 后恢复。
kind 12/13/14 的 ACK/result exact replay 幂等，identity/version/revision/digest 变化均 fail-close；不可逆 Apply 的
`ForwardRecoveryRequired` 可在 Broker expiry 后收敛。Broker 仅验证 receipt/Grant/command 的结构与 exact binding，
不持有 secret、不冒充 KMS HMAC verifier。Service worker、new-version proof、topology+operation CAS 和 Finalize
仍属于 R4c，完成前 route 继续由 R4a operation ledger 权威暂停。

R4c 已完成云端软件协调器：Service private API 只创建一次 KMS/envelope candidate，单 worker 逐状态推进 delivery、
Prepare、Apply、new-version GatewayProof、Finalize 与 route restore。`accessMaterialDigest` 固定为
`SHA-256(candidateSecret || PresenceKey)`，Controller Grant/receipt 的 `secretDigest` 固定为
`SHA-256(candidateSecret)`；Broker 不持有 candidate，只做结构绑定，Service 才负责 KMS 解封与 HMAC 最终验证。
GatewayProof 后由一个 `SERIALIZABLE` transaction 同时 CAS Active topology 和 renewal operation，并把新 KMS
locator/binding 与 credential/storage revision 一起提交。Finalize ACK 后先恢复权威 route 查询，再幂等执行 Broker
admission，最后才完成 operation。Broker 投影丢失/超时可按 immutable intent 和新 lease 重建；Apply 的
`ForwardRecoveryRequired` 必须先写 Service ledger，所有不确定的不可逆窗口只允许向前恢复。

当前真实 PostgreSQL、Linux Broker、Service/worker 与既有 Device V2 回归已通过；R4d 仍需在每个 durable 边界执行
Hub/child/Broker/Service 重启与真实双板续租。因此本能力仍是 internal，不能开放 App/owner 入口或标为 production ready。

## 9. 设备、App、Gateway、Service 真正执行顺序

以下不是可以并行任选的 backlog，而是依赖链。每一阶段必须删除被新实现取代的 candidate/旧路径并通过门禁，
才能进入下一阶段；不得先实现 Gateway wire 再回头修改 Direct session 语义。

| 顺序 | 批次 | 当前状态 | 完成定义 |
|---:|---|---|---|
| 0 | Repository baseline | 已完成 | 标准 Arduino `src/examples`、单 ZIP、architecture/release checks 和现有 host baseline 通过 |
| 1 | G0A DirectSecureRecord core | 设备 core/host 已完成 | exact wire/KDF、无 heap session、双方向 AEAD、strict replay 和 host 负测 |
| 2 | G0B BLE Direct integration | 设备侧已完成 | Method 2 成功原子切换；裸 post-auth frame 拒绝；断连/降级/tag/replay 退役 session |
| 3 | G0C cross-platform gates | 已完成 | App/device golden；ESP32/UNO R4/Wio clean compile、SizeProbe、20 B ATT benchmark |
| 4 | G1 pure BLE three-endpoint | Android + XIAO、UNO R4 已完成核心生产链路；Wio 官方 `2.1.3` 完成同 owner 三端重认领、App kill、固件重刷、真实重新上电、连接治理及 30 min authenticated soak，保持 opt-in preview | 补 UNO R4 长稳/恶意断连与 Wio 24 h authenticated DirectSecure release soak |
| 5 | G1B ESP32 WiFi+BLE | BLE WiFiProv、既有身份 network reset、无断电 WiFiProv→Direct、WiFi 自动恢复和 Direct Command 真机已过；Cloud 入口被本机防火墙阻断 | 开放 `17000/18883` 后补 DeviceKey session/MQTT/Cloud+Direct 同时在线，再补动态 heap/stack、循环与长稳 |
| 6 | G1C authorized presence | P1-A～P1-D2 与真实 migration 016/双账号分享 E2E 已通过；默认关闭的跨产品 v3 profile provider 已接入 | 预览固件完成三平台掉电/断连/轮换门禁，再正式启用 v3、复测资源并删除 v2 hint |
| 7 | G1D Wio/Direct release gates | 部分完成 | readiness/authentication deadline、32 次反复断连、资源复用和 30 min authenticated engineering soak 已过；只剩 24 h authenticated DirectSecure release soak，这是 Edge Hub 可用性的前置门禁 |
| 8 | G2-0 Edge Hub foundation | G2-0A/B1/B2/C、D4a、manual/BLE WiFiProv onboarding 与 D4b 只读预检软件门禁完成；D4b 待专用生产加密板 | V2 access contract/KMS/MQTTS ledger/schema v3 已关闭；设备解封、352 B active+pending atomic store、reboot/replay ACK、共享 MQTT side channel、ESP32 internal product compile 和四状态安全预检已过，待加密板真实配网/断电/replay ACK |
| 9 | G2A gateway attach | 单 child 四端主路径、移动 App、worker no-poll detach 与 pre-tombstone 双板恢复门禁已完成；encrypted-NVS 专用生产板门禁待补 | 已认领 child 安装独立 Gateway credential；单 child/单 Edge Hub 完成按需 GATT、Manifest/State、account Command/ACK，并通过 refresh/restart/revoke/crash-recovery 门禁 |
| 10 | G2B gateway-assisted enrollment | G2B-0～G2B-3 软件完成；G2B-4 已增加 exact Hub/child 无秘密 workflow、authority+account credential/checkpoint namespace 与 sessionEpoch 运行栅栏，覆盖 Pending、ownership 已提交/attach 未开始和 operation attach 三阶段恢复 | 完成 App kill/双账号/错误 Hub/restart/篡改/超时和四端真机门禁；ownership 与 topology 仍分离 |
| 11 | G3 controlled routing | 未开始 | 在 ChildRouteEnvelope 上扩展 device/service routes、ledger/rate/TTL 闭环 |
| 12 | G4 advanced topology | 后续决策 | broadcast telemetry、roaming、persistent/multi-gateway、Mesh、serial/NCP profile |

### G0：先修当前 Direct 安全事实（S1）

1. **G0A core**：冻结 Method 2 KDF 与 `DirectSecureRecord` exact wire，实现 allocation-free session codec；
2. **G0B integration**：在 BLE record transport 与 Runtime 之间接入 secure record，并让 authorizer 只在最终
   `AuthResult(Authorized)` 成功排队后激活 session；
3. **G0B fail-closed**：post-auth 裸 BBP/2、注入、篡改、重放、方向错误、reconnect 和 BLE security downgrade
   全部退役 session；pre-auth 只接受 HELLO/Authenticate 及其必要响应；
4. **G0C interoperability**：App/device exact golden，不由任一端各自解释 KDF/AAD；
5. **G0C resources**：重做 20 B ATT 分片大小、ESP32/UNO R4/Wio Flash/RAM 门禁。24 B secure overhead 必须
   计入 outer record、workspace、MTU 和队列预算。

G0 完成前不继续扩散 Gateway wire，也不把 BLE 标为 production。

#### G0B 设备侧实施记录（2026-08-24）

- `ControllerHmacSha256Authorizer` 在 credential/transcript 仍有效时只 `prepare` 密钥；Runtime 仅在最后一个
  raw `AuthResult(Authorized)` 成功进入 BLE TX 队列后 `commit`，排队失败则 `cancel` 且不授予权限；
- `BleFrameTransport` 是唯一 Direct 通道：pre-auth/raw 与 post-auth secure record 共用原 4 B fragment、
  RX/TX 队列和平台 crypto provider，不增加第二套 BLE reassembler 或 heap；
- secure TX 直接写入下一 TX queue slot；secure RX 复用 Noise/plaintext workspace。24 B overhead 已计入
  record stride，Small profile 仍只保留一个 session；
- post-auth raw frame、secure-before-commit、outer/tag/direction/sequence/replay 错误以及 BLE security downgrade
  都 fail closed，退役逻辑 session 并清除密钥与排队帧；
- host 端到端门禁覆盖最后 raw frame、commit 后双向密文、replay 与 raw downgrade；正式 Arduino 8 目标 clean
  compile 均通过。G1A 当前 BLE 编译为 ESP32 751,271 B/42,808 B、UNO R4 182,652 B/16,792 B、
  Wio 173,584 B Flash；ESP32 WiFi+BLE 1,400,371 B/63,320 B，ESP32-S3 BLE-WiFiProv
  1,298,031 B/60,156 B。

#### G0C 跨平台门禁记录（2026-08-24）

- 新增 data-only DirectSecureRecord fixture；Node/OpenSSL 生成并校验真实 AES-256-GCM，设备 C++ 与
  浏览器安全 TypeScript 独立计算 Method 2 transcript、HKDF、nonce、AAD、方向、sequence 与 outer wire，
  只 replay 已冻结的 AES primitive 输出；
- exact vector 覆盖 App→Device Command seq0、Device→App Ack seq0、Patch seq1，并冻结 Command 的
  3 个 ATT20 fragment；同一基准记录裸/secure Command、Ack、Patch 分别为 2/3、1/3、2/3 片；
- 派生函数改为流式 SHA-256 并让 HKDF 直接写 packed 96 B key material，删除 61 B transcript 和
  96 B material 临时栈副本；
- ESP32 3.3.1、UNO R4 1.5.3、Wio Terminal 1.8.5 的 Small SizeProbe clean compile 通过，32 位 ABI
  均为 key material 96 B、Direct session 112 B、BLE transport 320 B；Standard/Large transport
  另为 520/888 B，并由静态门禁限制；Small 编译分别为 ESP32 281,723/28,160 B、UNO R4
  52,392/14,924 B、Wio 31,292 B Flash（ELF `.data + .bss` 10,832 B）；
- G0 至此只冻结 Direct 安全和资源事实，未增加 Gateway credential、RouteEnvelope 或第二套 BLE transport。
- 2026-08-26 XIAO ESP32-S3 真机发现旧 NimBLE adapter 会把 Android 自动协商的 `MTU-3` 单方面用于 TX，
  而 App/UNO R4/Wio 均按 20 B 重组，触发 `BLE_DIRECT_FRAGMENT_INVALID`。修正后删除动态 MTU 状态，
  NimBLE RX queue 由 244 B/slot 收缩为 20 B/slot；公开 BLE 探针为 `715,175 B` Flash、`38,396 B`
  global RAM，并重新通过 Method 2、DirectSecureRecord、Manifest、Command/ACK、active-session access reset
  与同 logicalDeviceId 重认领。

G1 pure BLE 核心链路已在 UNO R4 和 Wio 真机通过。Wio 未认证连接占用 DoS、32 次重连与 30 min
authenticated engineering soak 已关闭；下一步只补 Wio 24 h authenticated DirectSecure release soak，
以及独立的 UNO R4 长稳/恶意断连和“连接中 reset”产品行为，
不提前扩散 Gateway wire。Wio 的历史断电失败证明 `2.1.3-blinker.2` 不能作为生产
bond 修复；该镜像仅保留为可版本化、可复现和可回滚的诊断依赖。Wio 设备库不承载
FTL 地址、底层栈替换、私有 key-entry ABI 或固件刷写逻辑，也不使用影子复制、固定延时或重复 stop/start 掩盖
协处理器问题。Android + XIAO/Wio 应用协议闭环不等于全平台 production，也不得据此跳到 Gateway wire。

### G1：完成现有纯 BLE 三端 enrollment

- Service 双算法 grant signer/key ring、单次 intent/commit；
- App Noise/enrollment/Method 2/DirectSecureRecord；
- ESP32/XIAO、UNO R4 与 Wio 官方 RTL `2.1.3` 的 production 核心真机链路已通过；Wio 已完成
  `resetAccess()` 后同 owner 三端重认领、App kill、同 Sketch 重刷与真实重新上电恢复，仍需长稳与抗连接占用
  门禁后才能离开 opt-in preview；
- Education/Nearby 与 Product/OOB 安全标识。

#### G1A 设备/Service 合同实施记录（2026-08-24）

- 设备 Hello 明确返回 `securityProfile + serverKeyId + signatureAlgorithm`。`BleEnrollmentGrant` 当前为 v3，
  三者中的 profile 和 signer tuple 均进入 canonical signed transcript，设备必须与 Hello 精确匹配，禁止把
  Product/OOB 降级成 Education/Nearby；
- Service 实现 Ed25519 与 P-256 双算法 key ring、短期单次 intent、exact grant replay、冲突/过期处理和
  幂等 commit。P-256 使用 raw `r||s` 并规范化 low-S；没有匹配 signer/profile 时 fail closed；
- App 管理面接口为 `POST /api/v2/ble-enrollment/intents`、
  `POST /api/v2/ble-enrollment/intents/{intentId}/commit` 和仅用于未安装恢复的
  `POST /api/v2/ble-enrollment/intents/{intentId}/cancel`。JSON 只承载登录 App 的控制面字段与 Base64URL
  opaque bytes；设备 wire 仍是 exact CBOR，raw controller secret 不进入 Service、数据库、日志或审计；
- 设备先 durable 安装 Admin，再返回含 HMAC proof 的 receipt；App 必须用本地 raw secret 完成 Method 2/receipt
  校验后才能提交 `method2Confirmed=true`。Service 只能校验 receipt 与首次 grant 的 exact 上下文，不能伪称
  自己验证了从未持有的 raw secret；
- PostgreSQL migration 11 保存首次 exact grant/receipt、request/commit digest 与状态。纯 BLE logical device
  的 `current_credential_version` 合法为 `NULL`，因为它没有 CloudCredential/MQTT 登录；Direct Admin 元数据
  复用 `v2_device_direct_access`，设备仍只持久化 access epoch 与 controller credential；
- device/service exact golden 覆盖 v3 transcript、PresenceKey version/digest、Ed25519/P-256 真签名；Host 负测覆盖 profile mismatch，
  PostgreSQL 隔离 schema 门禁覆盖 migration 11、exact replay、commit、BLE device 列表和 NULL cloud credential。
  Product/OOB 的 attestation/OOB verifier 尚未实现，因此默认配置只签 Education/Nearby，Product/OOB 请求拒绝；
- 纯 BLE 设备计入账号设备数量，但不触发 Broker 分配；账号删除时仍删除其 logical device/page/ACL，且不会创建或
  等待不存在的 MQTT credential invalidation。对它调用 DeviceKey 查看/刷新接口会稳定返回
  `DEVICE_CLOUD_CREDENTIAL_NOT_AVAILABLE`，而不是误报设备未激活；
- 本批没有增加 Gateway credential、RouteEnvelope、第二套 BLE transport 或新的 Arduino 用户 API。

G1 的 Education/Nearby App 生产 client 已完成读取 Hello、建立 Noise、请求/转发 grant、验证 receipt、完成
Method 2 和切换 DirectSecureRecord；Android + XIAO、UNO R4 已完成真实认领、持久凭据重连、Manifest/State
与 Command/ACK。下一步不是扩散 Gateway，而是补齐 UNO R4 长稳/异常断连和运行期 reset UX；Wio RTL 仅保留
诊断工件，生产路线另行决策。Product/OOB 必须等
attestation/OOB verifier 独立通过后才能声明支持；它不阻塞 Education/Nearby 首个可交付版本，也不得被静默
降级为 Education/Nearby。

#### G1A-App 生产客户端实施记录（2026-08-24）

- App 以同一公开 client 串联 `Hello -> Noise_NN -> grant relay -> exact receipt/HMAC 校验 -> Method 2 ->
  DirectSecureRecord`。Provisioning 断连后设备才切换为 Direct 广播，因此实现明确使用两段 GATT 会话，
  Provisioning 扫描只用广播 `setupSessionLocator` 绑定 setup window；切换 Direct 后广播不暴露稳定身份，App
  对重新发现的候选执行 Method 2，只有持有对应 controller secret 的设备才能成为该 logical device，而不把
  MAC、iOS peripheral UUID 或广播名当身份；
- Capacitor BLE transport 只暴露 record send/receive，内部统一处理冻结的 service/characteristic UUID、4 B
  fragment、MTU 上限、通知队列、重组、超时、断连和 Provisioning -> Direct 重发现；通用路径不调用
  Android `createBond()`，也不把 iOS 系统配对作为协议状态。平台产品可以额外启用可靠的 LE Secure
  Connections/bonding，但上层不接触 BLE 分片、PIN 或 bond 记录。UNO R4、Wio 和 XIAO ESP32-S3 均已证明
  普通 GATT bearer 上的 Noise/Method 2/DirectSecureRecord 完整链路；Wio 仍因长稳门禁未完成而保持 preview，
  不再以 Android RPA bond 恢复作为应用安全或发布前提；
- Android MAC/iOS peripheral UUID 只作为兼容期扫描/连接 hint，不参与身份判断。当前 App 为避免每次全量逐个
  Method 2，会把 hint 暂存在 `ControllerCredential`；只有目标 logical device 完成 Method 2 后，才允许把本次 hint
  归给它并清除其他 credential 上相同的旧 hint。P1 authorized presence 上线并完成迁移后删除该持久兼容字段；
  Direct target 始终必须用 Method 2 device proof 证明自己持有对应 secret。协商出的 frame/reassembly 上限在 session
  中强制执行，AEAD、sequence、解码、超时或链路错误都会断连并清零派生密钥；
- 正式 `DeviceUiPort` 按 logical device 选择 Account Cloud 或 BLE Direct，产品页、组件、Manifest/State store
  不接触 MQTT、GATT、CBOR 或分片。首页可在页面可见期间执行一次有界 foreground presence 扫描，只更新图标，
  不建立多设备 GATT、持续扫描或复用扫描 promise 作为产品页连接门禁；进入产品页才按需连接，离开页面立即断开
  GATT。扫描到多个 Direct peripheral 时逐个执行 Method 2，失败候选只在本次扫描排除，不能按 MAC/广播名选设备；
  页面退出会取消尚未完成的扫描，避免重入时残留两个 BLE 操作；
- WebCrypto provider 实现 X25519、SHA-256、HMAC-SHA256、HKDF-SHA256 与 AES-256-GCM，没有明文或伪随机
  fallback。跨端 golden 已覆盖 Noise transcript、Method 2 proofs、Direct KDF/AAD/nonce、双方向 record 和 ATT20；
- raw controller secret 只进入原生系统 secure storage。App 在把 grant 交给设备前保存 `pending` credential 与
  可确定的 expected receipt，receipt、Method 2 和 Service commit 全部成功后才原子标为 `active`；HTTP 管理面
  只发送 digest、fingerprint、grant/receipt 等 opaque bytes，不发送 raw secret；
- pending credential 在设备已经切到 Direct 后可重新 Method 2 并幂等 commit，active credential 可直接重连；
- 短 issue TTL 只关闭新的安装窗口。设备已 durable 安装、App 又能以 pending secret 完成 Method 2 并提交
  exact receipt 时，Service 必须在有界审计保留期内允许幂等 commit；不能仅返回 `Enrollment intent expired`
  把设备留在 Direct、账号却未收敛的孤儿状态。只有显式 Cancelled/Superseded 或 generation 已推进才拒绝；
- 若 App 在设备 durable 安装前退出，重启后的 App 必须先完成同一设备的新 Noise Hello，并确认设备仍为
  Provisioning，再按持久 `deviceInstanceId` 找到本机 pending credential，幂等取消旧 intent、删除本机 pending，
  然后在当前 Noise transcript 上签发新 intent。若设备已经是 Direct，禁止取消，必须用旧 pending 完成 Method 2
  和 commit；客户端不得用超时、MAC、广播名或 HTTP 状态猜测设备是否已经安装；
- cancel 只允许原 owner 对 `grant_issued` 执行，`active` 永远返回冲突，`cancelled/expired` 幂等返回。服务器无法
  远程撤销已经签名且可离线验证的 grant，因此该接口不是一般撤销 API；安全前提是新 GATT/Noise 会话已经取代
  旧会话，设备仍明确返回 Provisioning。设备若已安装但本地 access record 随后损坏，进入独立 recovery/reset
  流程，不能把 active intent 回退为 cancelled；`cancelled/expired` requestId 是终态，后续 issue replay 永久
  返回 410，不能重新取回旧 grant；
- 当前交付候选仅启用 Education/Nearby `Noise_NN`。Product/OOB `NNpsk0` 即使底层 primitive 已实现，也会在
  attestation/OOB verifier 和授权 UI 完成前显式拒绝，禁止静默降级；
- Method 2 成功后不再向 UI 暴露 raw frame send/receive。单 peer Direct 产品会话串行化请求并复用统一
  `DeviceV2Store`/codec，返回前完成 canonical Manifest fingerprint、分页 ManifestAccept、分页 State；组件
  Command 使用 endpoint id 并校验 exact ACK。异步 Event 不写入持久 State，可靠 Patch 经 revision 校验后回
  `Ack(acknowledgedSequence,stateRevision)`，revision gap 自动触发 State 重同步；
- `AckRequired` 请求在首个响应超时后只重发一次完全相同的 BBP/2 frame/sequence。设备以 replay cache 返回已
  记录的 ACK/Error，不再次执行 endpoint 回调；普通请求不重试，底层 send 失败也不伪装成超时。BLE 本地可靠
  重试窗口为 1 秒，第二次仍使用完整 6 秒请求截止时间；
- 已认证产品会话的接收循环没有空闲截止时间，只由显式关闭、GATT 断开或协议/加密错误结束；握手、扫描、可选配对、
  单次请求仍使用有界超时，不能用一个通用 10 秒 timer 杀死正常空闲连接；
- Direct Hello 只声明客户端已经实现的 Manifest、EndpointIds、Authentication、Reliable 和 StateRevision。
  Telemetry codec/云端租约实现仍保留，但 BLE Direct 产品会话尚未提供租约 API，因此当前不协商 Telemetry，
  避免设备发送客户端无法兑现的合法数据；
- PageLayout 是按账号 + logical device 保存的惰性 UI 元数据，不是 Broker route。纯 BLE 设备不创建虚假 MQTT
  credential 或 tenant route：Service 直接用设备 owner/share ACL 授权 CRUD，App 在已认证 Direct Manifest 上做
  endpoint 兼容校验，Service 对 local-only candidate 做严格结构校验；云设备仍由 Broker 当前 Manifest 二次校验；
- Angular application/spec typecheck、定向 ESLint、App Device V2 49 项协议测试、设备 Noise/Method 2/
  DirectSecureRecord/enrollment host 测试及 Service signer/intent 测试已通过；HTTP 边界负测另确认 raw
  controller secret 不会进入 enrollment request；
- XIAO ESP32-S3 的正式 `BleQuickStart` clean build/烧录通过，Flash 710,935 B、静态 RAM 39,508 B。真实广播
  为 13 B Provisioning profile（capabilities `0x0007`），RX 为 `write/write-without-response`、TX 为
  `notify/read`。Android 真机随后完成系统 bond、Noise NN、Service intent/grant/commit、receipt、Method 2、
  DirectSecureRecord、Manifest/State 和 Command/ACK；App force-stop 后 GATT 立即断开，bond 与 secure storage
  credential 保留，重启可直接重新 Method 2，不重复配对或 enrollment；
- Windows CLI 原生门禁已使用 D 盘 Android SDK 36、Build Tools 35/36、Platform Tools、Microsoft OpenJDK 21
  完成 Angular build、Capacitor Android sync 与 `:app:assembleDebug`。OPPO PGEM10 已授权 ADB、安装并运行
  Debug APK；真实 `power=true/false` 命令均在设备状态修订只增加一次的前提下获得 replay ACK，可靠背压路径
  的完整命令耗时约 1.6 秒，随后空闲 15.5 秒 session 仍为 ready。正式 `/device/:id` 页面已用同一 session
  读取并持久化 PageLayout、渲染 `power` 开关、完成双向控制；返回首页后 Android 立即发起 GATT disconnect，
  再次进入使用持久凭据恢复到最新 State；XIAO 经串口控制线软件复位后，Android 保留的 bond 与双方持久
  `ControllerCredential` 可直接恢复 Method 2，设备从重启态重新同步 State，随后 `power=true` Command/ACK
  再次通过。普通重启不得要求用户在系统设置中“遗忘设备”；当前 OPPO 的 bond 记录没有展示名，系统 UI 可能
  不显示该项，恢复出厂不能把手工遗忘当产品 API；
- XIAO 真机调用 `Blinker.resetAccess()` 后只删除 access root，保留 `DeviceInstanceId` 与 BLE bond，状态进入
  Provisioning，软复位后仍保持该状态。同一 owner 重新添加设备时，Service 复用原 `logicalDeviceId`，数据库
  保持一条 logical device、一条 direct-access、一个 active intent，并把旧 active intent 原子转为
  `superseded`；accessEpoch 与 controller ID 均已轮换。旧 accessEpoch/controller ID 的 Method 2 负测返回
  `BLE_DIRECT_AUTH_CHALLENGE_INVALID`，新 credential 随后恢复 `ready`，正式页面 `power` Command 将 State
  revision 从 0 推进到 1 并获得可靠 ACK。随后对 XIAO 完成一次真实掉电—重新上电：BLE bond、设备 access
  record、App secure storage credential 与 Service active intent 均保持不变，账本仍为一个 active、一个
  superseded；页面重新进入/显式连接后直接恢复 Method 2 与 DirectSecureRecord，没有重复 enrollment，
  `power=true` 再次把 State revision 从 0 推进到 1 并获得 ACK。设备离线时仍停留在详情页的 App 会进入
  `stopped`，但只在页面可见且 App 前台时按 1,000/2,500/5,000 ms 执行最多三次有界重试；离页、切后台、
  恢复 ready 或销毁会取消后续重试。Wio 最终固件真实重刷已验证详情页无需退出即可重新 Presence 匹配、
  Method 2 和 State 同步。该 UX 不通过设备重复广播、重复 enrollment 或无限扫描补偿。UNO R4 核心真机门禁已通过；Wio BLE 的基础链
  也已通过，但其预览准入问题归属 `rpcBLE` port 的长稳与连接资源治理，不阻塞跨平台纯 BLE 核心协议。

#### G1-UNO R4 真机收口记录（2026-08-26）

- UNO R4 WiFi、Renesas core 1.5.3、ArduinoBLE 2.0.2 与 Android 真机完成
  `Hello -> Noise NN -> grant -> receipt -> Method 2 -> DirectSecureRecord -> Manifest/State`；正式无 Trace
  Probe 为 186,572 B Flash、16,788 B global RAM，保留约 15,980 B 静态 RAM 余量；
- ArduinoBLE Security Manager 在本次手机/固件组合中出现 DHKey mismatch，因此移除 UNO R4 composition
  对 native pairing/encrypted characteristic 的强制依赖。安全性没有降级到裸 GATT：Noise 保护 enrollment，
  Method 2 成功原子建立 DirectSecureRecord，之后每帧由 AES-256-GCM、方向、session binding 与严格序列保护；
- 真机暴露并修复两个跨层缺陷：Method 2 的“建立安全 transport”策略必须显式注入，不能伪造
  `rx.encrypted=true`；DirectSecure record 验签解密后，transport 必须把验证后的 frame context 标记为 encrypted，
  否则 Runtime 会正确地以 `AuthenticationRequired` 拒绝 Manifest/State；
- 精确 access reset、重新 enrollment、固件重刷后的持久 Active、App force-stop/relaunch 后从 secure storage
  恢复、1 endpoint Manifest/State 以及 `power=true/false` Command/ACK 均通过。设备复用原
  `logicalDeviceId=ble_nBVYKR02cHTPzjkjudJhhQ` 并轮换 accessEpoch；旧 controller credential 的 Method 2
  被拒绝；
- `Blinker.resetAccess()` 在 App DirectSecure 会话仍连接时完成受控 HCI Disconnect、精确擦除 access 并重启
  Provisioning 广播，真机约 1.56 s 返回 `accessReset=1 / ok`，未再进入 `NotConfigured` fault。根因是 ArduinoBLE
  的 `BLE.end()` 不主动断开 central，而 UNO R4 的带 timeout `BLE.poll(timeout)` 又可能等待到 central 主动关闭；
  adapter 现在显式 Disconnect 后只做无 timeout 参数的非阻塞排空。DeviceInstanceId 保留，重认领复用同一
  logicalDeviceId，并把 accessEpoch 从 2250090672 轮换为 1837184833；Manifest/State 与 `power=true/false`
  Command/ACK 再次通过。不得用整片清除 Preferences 代替该合同。

#### G1-Wio Terminal 真机收口记录（2026-08-27，preview）

- Android 与 Wio 已完成系统配对、加密 CCCD、Noise Hello 和 21 片 Enrollment Request 的真实收发；设备未返回
  Receipt 的根因不是 20 B ATT 分片数，而是签名信任根漂移：Portable Ed25519 公钥把 RFC 8032 向量中的
  `0xF3` 错写为 `0x3F`，本机 Gateway 又曾加载另一把随机私钥。`keyId=1` 相同不能证明密钥材料相同；
- 设备公钥已修正，本机联调 signer 已换成与其匹配的测试私钥。共享 BLE enrollment golden 现在同时冻结
  `serverKeyId` 与 exact Ed25519 public key；Service 端也从私钥派生公钥并与同一 fixture 比较。发布流水线后续
  必须把 `(securityProfile, algorithm, keyId, public-key digest)` 作为一个不可拆分的 trust artifact 校验，不能
  只同步整数 keyId；
- 修复信任根后，真机已完成 `Hello -> Noise_NN -> grant relay -> exact receipt`，设备将 access root 持久化并从
  Provisioning 切换为 `Active/Direct`。最初的 `BLE_DIRECT_SCAN_TIMEOUT` 不是 wire 或 access store 问题，而是
  `rpcBLE` 把 GATT host deinit 后仍保留静态 initialized 状态，下一次 init 未真正重建服务；Wio port 现改为在同一
  MCU 进程内只创建一次 GATT server/service/characteristic，模式切换只更新 service data 并重启 advertising；
- 修复后 Android 真机已完成 Provisioning -> Direct 重新发现、Method 2、DirectSecureRecord、1 endpoint
  Manifest/State，并在 active credential 下完成 Direct 重连。BLE transport 由 App 发起 HELLO，设备只响应，避免
  20 B ATT 链路上双方同时主动 HELLO 造成协议竞争；
- `rpcBLE` 的 `onStatus(SUCCESS_NOTIFY)` 在 `server_send_data()` 返回后同步触发，只表示 RTL 栈接受了数据，并非
  空口发送完成信用。因此没有加入伪造的“一片在途回调”或固定延时；
- 历史 bonding 实验确认：未修改的 Seeed `2.1.3` 会返回 `SM_ERR_KEY_SAVE_FAILED`；补 FTL 后虽可恢复
  `bondCount=1`，闭源 `btgap.a` 仍无法可靠恢复 Android RPA 所需 identity keys，并最终触发
  `HCI_ERR_KEY_MISSING`。`2.1.3-blinker.2`、FTL 地址与私有 key-entry ABI 只留在独立固件仓作为诊断证据；
- 回刷 Seeed GitHub Release 的原始 `2.1.3` 三镜像后，`rpc_system_version()` 返回 `2.1.3`。先用既有
  ControllerCredential 完成 Direct 恢复、Manifest/State 和 Power/Button，再执行 `resetAccess()` 完成同 owner
  `Hello -> Noise NN -> Service grant -> relay -> receipt/commit -> Direct -> Method 2 -> DirectSecureRecord` 重认领；
  非 owner 账号在发放 grant 前被 Service 拒绝，旧 active intent 在 commit 时变为 superseded，同一
  logicalDeviceId 保持不变；
- 上述官方固件链路随后通过页面退出重入、App force-stop/relaunch、同 Sketch 重新烧录、凭据恢复、
  Manifest/State 和 Power/Button 再控制。它证明 Blinker 的应用安全与持久身份不需要自维护 RTL 固件；
  它仍不证明官方固件支持 Android RPA bond 冷恢复；
- Wio 真实重新上电后直接恢复为 `Active/Direct`（`state=4, local=1, error=0`），RTL 仍为官方 `2.1.3`；
  App 冷启动无需重新认领即可在首页标为“蓝牙在附近”，进入设备页后恢复 DirectSecureRecord、Manifest 1、
  Layout 1、State 1，并再次完成 Power Property 与 Button Action 下发/状态回报；
- App 设备列表的 BLE presence 扫描现由 `/home/device` 的 `NavigationEnd` 重新触发；修复后 Wio 重刷导致
  设备页离线，再返回已缓存的首页即可恢复“蓝牙在附近”，不再要求杀掉 App。该触发只复用现有 presence
  service，不新增扫描器或另一套设备身份判断；
- 正式预览 Port 删除 pairing/bonding 与 encrypted-characteristic 强依赖，准确报告 native
  `encrypted=false / bonded=false`。这不是业务明文降级：Noise 保护 enrollment；Method 2 成功后所有业务
  BBP/2 frame 必须进入 AES-256-GCM DirectSecureRecord，未认证连接不能获得 Manifest、State 或 Command 权限；
- rpcBLE 断连回调会自动用旧参数启动 advertising，而 Product 同时要从 Provisioning 切到 Direct，且上层 API
  不返回启动结果。Port 现使用 5 s 非阻塞状态机读取真实 `GAP_ADV_STATE_IDLE/ADVERTISING`，只在 idle 时写入
  当前 mode service-data 并启动，消除了 `BLE_DIRECT_SCAN_TIMEOUT`，没有加入 `delay()` 或重复初始化 GATT；
- Wio 当前可复核真机证据已经覆盖清空 access 后的 Noise/grant/receipt/commit、Method 2、
  DirectSecureRecord、Manifest/State、Power Property、Button Action、App kill、同 Sketch 重刷与真实重新上电
  恢复；三端日志和 RTL `2.1.3` 版本均已确认。尚未完成的是长稳和恶意 central 占用，不再把 bonding
  修复混入这些应用层发布门禁；
- Wio BLE 仍由 `BLINKER_ENABLE_WIO_BLE_PREVIEW=1` 显式开启且不在正式发布矩阵。Port 对“已连接但未订阅”
  central 设置 15 s readiness deadline，Frame/Noise 层也对未完成认证、协议失败和超时执行主动物理断开；
  128 轮连续未认证连接的 Host ASan 门禁证明逻辑槽位可复用。正式升级前仍需在官方 RTL `2.1.3` 上完成真机
  空连接超时、反复断连/重连和长稳；
- App 已删除允许调用方传入任意 target 的旧 `resumeEnrollment()`；恢复统一经安全存储中的 Pending enrollment
  元数据、exact Hub 新窗口与 PresenceKey matcher 进入现有 client resume。产品页仍应增加“恢复待提交认领”，而不是
  要求设备重复 enrollment；这项 UX 缺口不通过设备端协议分叉补偿。

#### G1-XIAO ESP32-S3 普通 GATT 真机收口记录（2026-08-28）

- 基准 Wi-Fi MAC 为 `98:3D:AE:EE:7C:6C`，对应 BLE 地址后缀为 `EE:7C:6D`。设备启动日志确认
  `begin=1 / lastError=ok`，扫描到 `EE:7C:6D` 且近场 RSSI 约 `-40 dBm`，排除“当前固件没有广播”；
- 设备已有 AccessEpoch 时广播 Direct，添加设备页只筛 Provisioning，因此不会显示；执行精确
  `Blinker.resetAccess()` 后才进入可认领广播。现场另有多个同名 `Blinker` 旧板，旧的“扫描后取第一个”交互会
  连接错误目标。正式 App 改为多候选选择，并显示地址后缀与 RSSI；设备公开广播名仍保持 `Blinker`，不把 MAC
  编进公开 API 或稳定身份；
- Android 已完成 GATT connect/service discovery、MTU 256、notify、Noise、grant relay、receipt、Method 2、
  DirectSecureRecord、Manifest/State 以及 `power=true` Command/报告。BLE v1 bearer packet 仍固定 20 B，
  MTU 不会单方面改变协议分片；
- 移除临时唯一名称、串口 debug 和测试 reset 入口后，将 clean probe 重新烧录且不擦 NVS；App 首页恢复
  “蓝牙在附近”，进入设备页恢复“蓝牙已连接 / Manifest 1 / State 0”，再次下发 Power 后得到
  `true / State 1`。因此正式结论不依赖诊断固件；
- ESP32 Port 不主动启用 SMP/bonding，也不使用 encrypted characteristic permission；真机全程没有系统 PIN 或
  pairing 对话框。应用 payload 不是裸明文：Provisioning 由 Noise 保护，Direct 业务必须通过 AES-256-GCM
  DirectSecureRecord；
- 联调中出现的 `BLE_DIRECT_INTENT_RESPONSE_INVALID` 来自本机 Docker Gateway 仍运行旧镜像，响应缺少当前
  `presenceKeyVersion/presenceKey`；重建 Gateway 后闭环通过。`adb reverse` 在 USB 重连后也需重新建立，二者均是
  联调环境门禁，不是 BLE wire 或设备广播缺陷。

2026-09-02 在 Arduino-ESP32 `3.3.6` 上复测了原位 advertising profile 刷新后的同一条生产链路：清空设备
access 后，Android 无过滤扫描只发现一台名为 `Blinker` 的目标，地址后缀 `EE:7C:6D`、近场 RSSI 约
`-34 dBm`；随后完成 `Provisioning -> Noise -> intent/grant -> receipt -> Direct -> Method 2 ->
DirectSecureRecord`，设备没有退出并重启 NimBLE Host/controller。Manifest 1 与 State 同步后，关闭 Direct
会话再由产品页重连耗时约 `3.6 s`，三次 Power Command 使 State revision 单调推进至 3。强制杀死移动 App 后，
安全存储中的账号和 ControllerCredential 均能恢复，重新进入设备页显示“蓝牙已连接”且保留
`power=true / State 3`。本机因设备 TLS 代理占用宿主 `7000`，App HTTP 必须使用
`adb reverse tcp:7000 tcp:17000` 指向 Docker Gateway；错误映射会在 GATT 前表现为 enrollment intent
`status 0`，不得误判为 BLE 失败。

同轮后续完成认领后的物理 reset：设备从持久 `DeviceAccessStore` 恢复 Direct v3，App 使用既有
ControllerCredential 自动完成 Method 2，重启后第一条 Power Command 在约 `313 ms` 内返回并将 State
revision 从 0 推进到 1。随后不擦 NVS 重刷正式 `BleQuickStart`，同一逻辑设备和页面继续工作；正式示例立即
上报 `power=false / buttonPresses=0`，Power 与 Action 下行将 State 单调推进到 3。固件 Manifest 从诊断
probe 的 1 个字段扩展为正式示例的 3 个字段时，App 精确提示 2 个能力待加入；用户确认后 Layout revision
从 1 更新到 2，未新建设备，也未使用 MAC 关联页面。

因此纯 BLE 的认领后 reset、Sketch 重刷不擦 NVS、Manifest 变化提示/页面更新、App kill 与 Command/ACK
门禁已经关闭。2026-09-02 又以真机关闭了 G1B 的 network-only reset、Direct 活跃时 WiFiProv 交接、
既有 DeviceKey/access 保留、Cloud+Direct 同时在线、BLE-first/Cloud fallback/Direct 自动抢回、动态 heap/
  loop-task 快照和可复现完整模式切换。最终 App route/session ownership 修正后，全新 App 进程的连续两轮
  都在设备页出现后约 4.27 s 完成 Direct，WiFiProv GATT cleanup 不再与 Direct scanner 重叠；Property
  Command/ACK 与 13-task 观测通过。Arduino-ESP32 内部 `ipc0` 最低仍仅 76 B；100 次循环、RF 共存和长稳
  尚未关闭，不能以当前局部结果宣称 G1B 已发布完成。

2026-09-03 的 3.3.6 隔离探针进一步确认：最小 SDK 启动和 WiFi STA 的 `ipc0` 均约 260 B，官方
`nimble_port_init()` 使其降至 84 B；完整产品从自己的 pre-begin 252 B 降至 76 B，净降幅同为 176 B。
因此不在 transport-neutral BLE、Gateway routing 或 Product composition 中加入特例，也不删除其必要能力来
掩盖 SDK 栈问题。后续同板 A/B 纠正了 3.3.10/3.3.11 的首启归因：裸 ESP-IDF C API 探针没有声明 BLE 正在
使用，Arduino Core 已在 `initArduino()` 提前释放 BLE 内存。3.3.11 上，外部 NimBLE-Arduino、Core 自带
`BLEDevice` 以及加入 `esp32-hal-alloc-ble-mem.h` 的直接 C API 均成功，后者 `ipc0=1108 B`。因此 3.3.11
恢复为 product-level 候选；正式 10→30→100 仍要等当前 Port 补齐内存保留合同并通过 WiFiProv→Direct/RF
门禁。循环脚本按任务分别校验，禁止再以全局最小栈阈值把 SDK task、WiFi、NimBLE 和 loop task混为一项。

### G2-0：Edge Hub 基础合同

- Edge Hub 是独立 release profile，HTTPS/MQTTS 强制启用且 fail-closed；
- 冻结 Service/KMS 托管的 per-child/per-active-hub Gateway credential 与单一 `GatewayAccessEnvelopeV2`；
- 设备 access store 升级为三槽 schema v3，仍兼容读取 v1/v2；不新增 `PlatformGateway` domain；
- 冻结 `PendingAccessDelivery -> PendingChildInstall -> PendingGatewayProof -> Active/Failed/Expired` 状态机；
- 先实现只满足 child proxy 的最小 `ChildRouteEnvelope`、topology ACL、幂等 ACK 和在线状态，不提前开放任意
  device-to-device/service route；
- 实体 Edge Hub 与服务器 Cloud Gateway 在代码、表名、日志和文档中使用不同命名。

### G2A：最小 BLE Gateway（先认领、后 attach）

- Edge Hub 作为强制 MQTTS 的 WiFi DeviceKey product 上线；
- 增加第三个 Gateway credential slot，不改变组件 API、Method 2 或 DirectSecureRecord；
- Service topology + KMS-wrapped per-child/per-active-hub Gateway secret + exact-hub sealed MQTT envelope；
- App Admin 在场安装；
- 单 child、单 Edge Hub、按需 GATT 连接的上/下行和 ACK 联调。

G2A-2g 已完成：ESP32 fixed-queue Central、exact Presence、单 child Method 2/DirectSecure、
proof/activate 控制面和最小 route bridge 均已接入内部 Edge Hub。只有 MQTTS management 与 Active route admission
同时成立才启动代理；cloud route 丢失会停止新转发，但短期 SessionCredential 刷新只重建 cloud route，不能清除
已经持久化并验证的 terminal topology proof。设备/Hub/Cloud 已完成 Manifest/State/Command/ACK/Patch、Broker 重启
和连续 credential refresh 真机门禁；真实 Android 设备页也已通过 account MQTT 下发 Action 并观察到同一 child
State/ACK。child 复位后，即使状态 revision 从旧会话的 3 回到新会话的 1，Broker 推送的完整 StatePage 仍作为
权威运行代际边界替换 App 缓存，随后 Patch 从 revision 1 单调推进到 2。不得让内部测试入口绕过 topology，也不得
在此时扩展多 child、Mesh 或任意 device/service route。

G2A-3a 已先关闭云端撤销闸门：owner detach 只将 topology 推进到 Revoking，Service/Broker 从该 CAS 起停止并
主动退役 route；它不声称 child 已删除 Gateway credential。G2A-3b 的真实四端主路径现已完成
Prepare/Apply/Finalize、signed revoke、durable receipt、Hub secret-free tombstone/replay 和 Detached CAS。
Hub 在 Finalize 后复位，持久状态恢复为无 active access、无 child secure session，旧凭据没有复活。

Service 现在按 `updated_at + operation_id` 有界扫描 Revoking 及未 Finalize 的 Detached 拓扑，限制 batch/concurrency，
同进程不重入；单操作失败不阻塞其他拓扑。每轮先幂等退役 route，若 Service CAS 已提交而 Broker phase 尚未
落盘或 ledger 丢失，则从权威 topology 重建同一 phase。Broker exact ledger 与 PostgreSQL CAS 仍是幂等/并发
安全根。现场已在
不调用 App 的情况下清除派生 `revocation_finalized_at`，后台 worker 依据 Broker 持久 Finalize result 自动补回；
随后全新 topology 已完成 version 2 attach/Active，并在 App 仅发起 detach 后强制停止 App；后台 worker 独立完成
三阶段 ledger、Detached/Finalize，Hub 复位后无旧 access，no-poll 门禁因此关闭。child 已删除 credential、Hub 尚未
commit tombstone 前掉电时仍只能保持 Revoking，不能由 Cloud 超时推断为 Detached。该窗口的 nearby Admin recovery
已完成软件与双板真机闭环：fresh Admin nonce 产生更高 topologyVersion 的 signed Revoke，子设备对 absent slot 返回
canonical receipt，Broker internal endpoint 将新 Apply ledger 收敛为 Revoked 并停止旧版本重投，随后既有 Finalize
仅在 Hub active record 的 operation/instance/epoch/controller/version/revision 全匹配时清除。现场精确在 child receipt
accepted 与 Hub tombstone durable write 之间复位；恢复后 App 取得幂等 receipt，Service 写入 Detached/Finalize，Broker
Apply/Finalize ledger 分别为 Revoked/Finalized，Hub 再次复位后 `revokeActive=0`。联调同时修正两项边界问题：
Edge Gateway HTTP 编排按 account 而非 device 维度限流；App Revoke receipt 解码允许协议规定的 32 B 零 secret digest。
这一路径不再需要增加协议状态或 UI。

### G2B：网关辅助首次认领（正式 Gateway profile 发布门禁）

- G2B-1 已完成 App/Service exact gateway 300 s permit-join 控制面和 Broker durable QoS1 重投；
- G2B-2a/2b 已完成设备 Native GATT 窗口、operation-bound kinds 7～11 relay、Broker 双向单槽易失 mailbox 与
  owner HTTPS relay API。Broker runtime 重启会轮换 `relaySessionId`，App 必须废弃旧 Noise 事务；mailbox 不进入
  PostgreSQL、analytics 或 topology；
- G2B-3 已完成 App exact HTTP/CBOR transport、owner Hub 设置页产品入口、明确候选选择、双窗口
  Provisioning→authorized Direct、现有 BleDirectClient 复用和 G2A relay child-control 串联；
- G2B-4 已将 exact Hub/child 无秘密 workflow 标记放在 pending credential 落盘之后、grant 发出之前，覆盖 Pending
  resume、ownership 已 commit 但 attach 尚未开始，以及精确 operation attach 三阶段恢复；只以持久 PresenceKey
  识别 Direct v3，枚举不解码 raw controller secret。终态先删 workflow、再删更精确 attach checkpoint，任一步
  App kill 都保留恢复依据；无 exact Hub 标记或缺少 PresenceKey 的旧记录 fail-closed。App durable credential/
  checkpoint 还必须按 `(service authority, accountId)` 分区，所有运行事务绑定登录 `sessionEpoch`；切换账号立即
  中止扫描/会话并清零迟到响应中的 secret/relay bytes。缺少 namespace 的测试期旧记录不得猜测迁移；
- Gateway 只做 enrollment BLE fragment/Noise opaque relay，不接触 Admin secret；
- 复用现有 BleEnrollmentGrant 与 receipt，再由 Admin 安装 Gateway credential；
- 失败保持 AppOwnedDirect、pending topology 可安全重试；
- G2B-4 尚需用真实四端证明双账号抢占、错误网关、窗口过期、relay 篡改/重放、pending enrollment resume、
  App kill、Gateway/child/Broker reboot 均 fail-closed；软件恢复入口完成不等于这些门禁已通过，完成前不得公开
  Gateway-assisted enrollment。

G2B-4 的恢复实现进一步冻结为以下共享边界：App 的 DirectSecure record send 在一个 session 内必须串行，
包括 AEAD sequence/nonce 分配；attach 必须按 `Cloud sealed delivery -> bounded child Admin install -> Cloud Hub proof`
分段恢复，等待 Hub 时不得持有 child GATT。Broker 的单 relay batch 仍是冲突闸门，不承担客户端排队。

ESP32-S3 的 SDK 门禁必须同时检查“首次 controller 启动”和“生命周期是否需要 controller 重启”。3.3.1/3.3.6
的 1024 B IPC stack 会在 BLE 初始化路径留下过窄余量；3.3.10 也仍为 1024 B。3.3.11 已提高到 2048 B，
并在正确声明 BLE 内存使用后，以三种 API 路径完成首启和 advertising，`ipc0` 约 1104～1108 B。此前
3.3.10/3.3.11 的 TLSF/watchdog 不是 Controller 版本结论，而是直接 C API 绕过 Arduino 内存保留合同的结果。
3.3.11 由此成为当前正式候选，但仍不能把最小 API 成功冒充完整 Product 发布合格。

架构修正是消除不必要的 controller 生命周期：Direct Presence locator 更新只在无连接时停止 advertising 并
原位替换 service data，不再停止 GATT/NimBLE/controller；并发连接使本轮延后而不是使 Product Fault。未提供
原位更新能力的平台仍由 coordinator 回退到完整模式切换。最终发布仍必须用冷启动、enrollment、
WiFiProv→Direct、credential mutation、定时 locator 轮换和连续重连的真机矩阵证明没有再次初始化 controller；
3.3.11 必须通过这些 Product 门禁后才能由候选升级为生产基线。

### 9.1 ESP32-S3 NimBLE 上游归因与 SDK 维护边界

这里的“NimBLE 问题”必须拆成四层，不能用一个库名代替故障边界：

1. Blinker 的 `Esp32NimBleLink`/Central Port 只负责 GATT、队列和模式生命周期；
2. ESP-IDF `esp-nimble` 提供 Host；
3. ESP-IDF port/HCI 把 Host 接到 controller；
4. ESP32-C3/S3 HCI 以下的 controller 由
   [`esp32c3-bt-lib`](https://github.com/espressif/esp32c3-bt-lib) 的预编译 archive 提供。

本轮同板对照证明，失败虽落在 `esp_bt_controller_init -> btdm_controller_init`，但根因不是 Host 或
controller 二进制：Arduino-ESP32 3.3.10/3.3.11 会在启动期释放未声明使用的 BLE 内存。官方 Core
`BLEDevice` 和 NimBLE-Arduino 会通过 `esp32-hal-alloc-ble-mem.h` 构造器保留内存；直接 C API 使用者也必须
遵守同一合同。补齐该标记后 `nimble_port_init()`、Host task 与 advertising 均成功。

截至 2026-09-03 的官方 Git 状态是：

- IDF `v5.5.2-249-gf56bea3d1f`（Arduino-ESP32 3.3.6）使用 controller
  [`9b50531537e7`](https://github.com/espressif/esp32c3-bt-lib/commit/9b50531537e755792ac827d00d233eab499a0b37)，
  当前真机可首次启动，但 stock IPC stack 只有 1024 B；
- IDF `v5.5.4` 使用 controller
  [`ebd6043a8e3c`](https://github.com/espressif/esp32c3-bt-lib/commit/ebd6043a8e3c3bbde45ee483895303b9c1229ab5)，
  旧 TLSF 探针缺少 Arduino BLE 内存保留标记，不能作为 controller 否决证据；其 1024 B IPC stack 仍不满足
  当前候选资源要求；
- IDF `v5.5.5`、`release/v5.5` 和当前 `master` 使用同一个 controller
  [`0a08c4b32f36`](https://github.com/espressif/esp32c3-bt-lib/commit/0a08c4b32f3666003080b662a1a61794da24ff0f)，
  该 controller 提交列出 coded-PHY/Wi-Fi coexistence 改进；3.3.11 在正确保留 BLE 内存后已通过三路径首启；
- master 新增的
  [`esp-nimble` mempool deinit 修复](https://github.com/espressif/esp-nimble/commit/916be244a9c646bc16fd65507478cf3fe717d8ed)
  和 [IDF 聚合提交](https://github.com/espressif/esp-idf/commit/13e18a6) 针对 deinit/reinit 资源回收；聚合提交
  更新的是 ESP32-H4/S31 controller，与本轮 Arduino 启动期内存保留问题无关；
- Arduino-ESP32 的 [XIAO TLSF 报告](https://github.com/espressif/arduino-esp32/issues/12357) 被标记为无法复现，
  [BLE WiFiProv 3.3.7 回归](https://github.com/espressif/arduino-esp32/issues/12436) 仍无关联开发项；ESP-IDF 也有
  [同一 `btdm_controller_init`/TLSF 签名](https://github.com/espressif/esp-idf/issues/13666)，但没有公开修复关系。

修正后的冻结决策如下：

- 不维护 Blinker NimBLE Host fork，不把 controller 二进制跨 IDF 版本拼接，也不在 Device V2 Core 中加入
  ESP32-S3 业务特例；Arduino 内存保留只属于 ESP32 Port 的启动合同；
- 保持 `IBleModeLink`/`IBleCentralPort` 与平台 Port 边界，未来官方 Host/controller 可整体替换而不改
  BBP/2、DeviceAccessStore、Gateway routing 或公开 Arduino API；
- 官方 Arduino-ESP32 3.3.11 是首个当前候选，不再自建“3.3.6 + 2048 B IPC stack”Core；
- 当前直接 C API Port 先补齐 BLE 内存保留合同，再依次完成 WiFiProv→Direct、credential mutation、
  10→30→100、RF coexistence 和长稳；任一门禁失败都回到最小、正确保留内存的三路径 A/B，不再用错误
  裸探针评价 SDK。

把 `nimble_port_init()` 放入一个较大栈的 Core 0 临时任务不能替代内存保留合同，也不能改变 IPC task 水位。
官方 ESP32-C3/S3 controller
`interrupt_alloc_wrapper()` 在双核构建中无条件把 `btdm_intr_alloc()` 交给 `esp_ipc_call_blocking(cpu_id, ...)`；
ESP-IDF IPC 只有当前调用者本身就是目标 `ipcX` task 时才原地执行，普通 pin-to-Core-0 task 仍会进入固定栈
`ipc0`。试图绕开它需要修改 SDK 的 controller port，而不是 Blinker Arduino 库。

### 9.2 降低用户 SDK 绑定的正式策略

目标不是让 BLE 二进制脱离平台 SDK——ESP32 内置 controller 决定这在物理上做不到——而是把依赖限制在
平台 Port 内，使普通用户不接触 IDF、controller archive 或 sdkconfig：

1. `Blinker.h`、组件 API、BBP/2、Manifest、状态和鉴权合同不感知 ESP32 Core 版本；
2. `Esp32BleBackend` 只暴露 `Available/Unavailable` capability，内部以编译能力和经过真机门禁的 Core 范围判定；
3. WiFi-only 始终走 WiFi Port，不因为 BLE backend 不合格而拒绝编译；
4. WiFi+BLE 在 backend 不合格时只允许**显式、安全降级**为 SoftAP provisioning + Cloud，Manifest/capability
   准确声明 Direct BLE 不可用，App 不显示“附近”或尝试 Direct；禁止静默宣称 WiFi+BLE 完整可用；
5. 纯 BLE 和 Edge Hub BLE 没有可替代 transport，故在 production backend 缺失时编译期失败，并给出一条用户
   可执行的 Boards Manager 升级建议。内部 probe 可用显式 preview macro 解锁，但正式示例不使用；
6. 3.3.11 通过完整 Product 门禁后，支持策略按 `minimum supported Core + 同 major/minor 的 CI tested range` 发布，
   不要求用户固定到唯一 patch；新 major 在真机矩阵通过前保持 Unknown/preview；
7. 自维护完整 Core 只服务内部 A/B 或自有预装硬件。若商业上必须使用定制 SDK，可提供独立 Blinker BSP
   Boards Manager 包，把 SDK pin 隐藏在板包更新中，但它必须承担 Core 安全更新、工具链和所有 ESP32 目标的
   维护成本，不能塞入普通 Arduino library ZIP。

因此默认用户路径仍是“安装一个 Blinker 库 + 官方板卡 Core”。当前 ESP32 WiFi 正常开放；ESP32 BLE/Edge Hub
以 3.3.11 作为候选完成 production matrix，在此之前保持 preview。这样不要求用户安装外部 NimBLE，也不把
ESP32 Port 的启动合同扩散到 UNO R4、Wio 或未来串口/NCP backend。

### G3：设备间/服务路由

- 在 G2-0 的最小 ChildRouteEnvelope 上冻结完整 RouteEnvelope 和 route ACL；
- Broker 以 ingress session 设置 source；
- cloud device->device、child->service、child->child 三条 E2E；
- command ledger、rate limit、loop/TTL 防护和离线 policy version。

### G4：高级拓扑（后续独立决策）

- persistent child connections 与 App 抢占；
- 多 gateway failover/roaming；
- 加密 authenticated advertisements；
- BLE Mesh；
- 串口/SPI connectivity agent。

这些能力不得以在现有 GATT 代码中增加条件分支的方式提前混入 G0～G2。

## 10. 发布门禁

- 未认证的普通 GATT 只能承载 Noise enrollment 或 pre-auth Hello/Method 2；Method 2 成功后裸 BBP/2
  无法进入业务数据面。平台若声明 native link encryption，安全位回退同样必须退役 session；
- Just Works/Nearby UI 不宣称主动 MITM 防护；OOB profile 有 exact golden 和真机负测；
- Method 2 成功前业务 frame 拒绝，成功后裸 BBP/2 frame 也必须拒绝；
- 已连接但未完成 notify/readiness 或应用认证的 central 必须在有界 deadline 后被物理断开；连续恶意连接不得
  泄漏 session、reassembly 或 peer 槽位；
- AEAD tag、sequence、direction、session、epoch/version 任一改变均 fail-closed；
- MAC 改变不改变逻辑设备，MAC 冲突不能绑定错设备；
- Gateway 不能代理未在 active topology 中的 child，也不能伪造 source；
- cloud request sequence 与 child peer sequence 属于不同 session；Hub 必须独立分配并映射，exact cloud retry
  复用同一 child sequence，child session 重建后才重置该序号空间；
- Patch revision 只在同一设备运行会话内要求单调；Broker 接受并完整验证的 StatePage 是新的权威状态代际，
  可以从较低 revision（包括 0）替换 App 缓存，分页未完整时不得暴露半快照；
- Gateway 扫描到未认领设备不能自行创建 ownership；辅助认领时不能看到 Admin secret 或终止 Noise；
- Edge Hub 的 DNS/TCP/TLS/CA/证书/内存任一失败均不得尝试 Plain MQTT；Cloud Gateway 公告 `mqtt` 或错误
  trust profile 时该 Edge Hub 必须拒绝上线；
- Gateway secret 在 MQTT 抓包、Broker ledger、数据库普通列、日志、trace、crash dump 和 App durable storage
  中均不得以明文出现；错误 hub、错误 DeviceKey version、过期/replay/tampered envelope 必须 fail-closed；
- topology 只有在 Edge Hub secret durable ACK、child durable receipt 和 Edge Hub->child Method 2 proof 都完成后
  才能 active；任何两项成功都不能提前代理数据；
- 分享 A 的设备不会取得同家庭 B 的设备或 secret；
- Gateway 删除、Shared 撤销和 access reset 分别验证云端立即阻断与本地 eventual 边界；
- 单 ZIP 的非 Gateway quickstart 不链接 Gateway coordinator/KMS/route 实现；
- 无 heap、固定 workspace、断电恢复、App kill、Gateway reboot、Broker reboot 和 replay 全部通过。
