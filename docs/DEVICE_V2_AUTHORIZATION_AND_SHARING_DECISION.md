# Device V2 鉴权、直连分享与设备侧最小状态决策

> 状态：2026-08-20 正式修订基线。本文覆盖此前“每 App 一条 ControllerCredential”、设备保存完整
> OwnershipRecord、家庭全局共享密钥以及 Plain MQTT 仅凭 CONNECT 即视为可信的旧描述。设备 Core 与正式
> P6 composition 已按本文接线；DeviceKey -> MQTT 功能链 Host E2E 已闭环。Service 的
> Cloud account→device ACL 数据面及一次性分享邀请/管理入口已完成 AR-2B2；Service 也已完成首次 Direct Admin
> metadata 登记，但 App 接入、Direct secret 分发、
> Cloud Frame Authentication、Direct Secure Record 和真机尚未完成；公开 `<BlinkerWiFiBLE.h>` 只代表设备 API/静态门禁完成，
> 不代表生产安全与三端准入完成。
> BLE Gateway/设备间路由的后续权威边界见 `DEVICE_V2_BLE_DIRECT_GATEWAY_ROUTING_ARCHITECTURE.md`。

## 1. 冻结结论

1. owner、家庭成员、`logicalDeviceId`、PageSchema 和分享 ACL 的权威只在服务器，设备不保存完整
   OwnershipRecord，也不根据账号判断一条业务命令。
2. WiFi-only MQTT 使用 `DeviceKey -> 短期 SessionCredential`；不需要 Direct ControllerCredential。
3. TLS 是可选 profile，但 Plain MQTT 不能只依赖明文 CONNECT 用户名/密码。发布前必须增加独立的
   Cloud Frame Authentication：设备与服务器共享的 session message key、32 位序号、MAC 和重放窗。
4. BLE、LAN WebSocket/TCP/UDP 等 Direct transport 不需要 ownership 数据，但必须使用本地访问凭据完成
   challenge/proof。当前代码尚未派生/使用 Direct session key；生产目标是在 Method 2 后强制使用带方向、
   序号和 AEAD tag 的 `DirectSecureRecord` 保护后续 BBP/2。
5. Direct 凭据按“每台设备的角色组”保存，而不是每个 App 安装实例一槽：首发最多两槽
   `Admin + Shared`。`Shared` 是当前设备的共享组，不是家庭全局密钥。
6. 服务器分享关系固定为 `(userId, logicalDeviceId, role)`。用户只获得明确分享给他的那台设备的
   Shared secret，不能由一台设备推导任何其他设备。
7. `accessEpoch` 只是 4 B 本地访问代次，用于整机转让/恢复后拒绝旧 Grant 和旧 Direct 凭据；它不是 owner、
   ownership record 或账号 ID。

## 2. 三条互不复用的鉴权链

| 链路 | 长期根 | 短期状态 | 设备是否保存用户/owner | 主要用途 |
|---|---|---|---:|---|
| Cloud MQTT/WS | DeviceKey | SessionCredential；Plain 还需 cloud frame key | 否 | 服务器转发状态和命令 |
| Platform Direct BLE/LAN | Admin/Shared credential + accessEpoch | 当前 challenge；目标 session key、sequence/AEAD | 否 | 离线/近场直控 |
| Local-only BLE/LAN | Local Admin/Shared credential | 当前 challenge；目标 session key、sequence/AEAD | 否 | 不进入服务器的平台外设备 |

DeviceKey、Cloud session key 和 Direct credential 必须域分离，禁止把 DeviceKey 当 BLE/LAN secret，也禁止
拿 Shared secret 临时填补 Plain MQTT 的消息完整性缺口。

## 3. WiFi-only MQTT 完整链路

```text
设备持久 DeviceKey + WiFi profile
  -> DeviceKey challenge/HMAC
  -> 短期 MQTT clientId/username/password/sessionId
  -> MQTT CONNECT
  -> BBP/2 HELLO / Manifest / State
  -> 服务器按 account-device ACL 接收 App 请求并转换成设备命令
```

设备不区分 owner 与 shared user。Gateway/Broker 必须在发布到设备前完成登录、租户、设备 ACL、Manifest 和
命令权限检查。WiFi-only profile 因而只需要 DeviceKey store，不得为了家庭分享附带 Direct credential table。

当前 DeviceKey session v1 只返回 MQTT 登录字段和 sessionId，`MqttFrameTransport` 也没有消息 MAC。TLS profile
可依赖 TLS 的机密性/完整性；Plain profile 当前只能视为资源与互操作实验，不能宣称抗同网段篡改。正式 Plain
合同至少还需冻结：

```text
cloudFrameKey = KDF(DeviceAuthKey, exact challenge/session context)
CloudEnvelope = sessionId + txSequence32 + BBP2Frame + HMAC tag
```

key 不经 MQTT 明文发送；Broker/Gateway 从同一 DeviceKey/session transaction 派生。接收端在交给 Runtime 前
先验证 session、方向、序号和 tag。response proof、派生 transcript、tag 长度、重连序号和 MQTT packet 增量需
三端 golden 后实施，不能由设备单方猜测。

## 4. Direct Admin/Shared 两槽模型

设备内部继续复用 `ControllerCredential` 记录和 Method 2；一条记录表示一个当前设备的访问组：

| 槽 | 典型权限 | secret 分发 | 用途 |
|---|---|---|---|
| Admin | Observe、Control、ProvisionNetwork、ManageControllers | 首次配网 App/owner | 配网、恢复、凭据管理和控制 |
| Shared | Observe、Control | 只下发给该设备 ACL 中被分享的用户 | BLE/LAN 普通控制 |

`controllerId` 就是该设备某个 Direct 访问组的稳定 ID；它不是手机、用户或家庭 ID，也不再同时使用
`adminGroupId/accessGroupId` 别名。没有分享时 Shared 槽不存在。
Small P6 冻结为最多两槽。完全不接服务器的 LocalBLE 仍通过物理确认安装 local controller，当前 standalone
BCT2 同样限制为两槽，但不套用服务器 household/share 语义；是否改成 Local Admin/Shared 另行冻结。

首次 Security1 provisioning 由 App 在受保护内存生成 Admin secret；Service 分配非零 accessEpoch，只保存
Admin controllerId/version/fingerprint/状态，不接收 raw Admin。App 将 DeviceKey、accessEpoch 与 Admin exact relay，
设备一次原子安装。后续 Shared 必须由已认证 Admin 会话和服务器签名 Grant 安装/轮换。Shared 永远不能取得
`ManageControllers` 或 `ProvisionNetwork`。

三端 fingerprint 固定为
`SHA-256("blinker/direct-admin/fingerprint/v1" || 0x00 || controllerId[16] || version[u32be] || secret[32])`。
它只用于 Service 核对元数据，不能替代 Direct challenge/proof，也不能从中恢复 secret。

## 5. 只分享某个人、某一台设备

```text
device_share(user_id, logical_device_id, role, direct_enabled, state)
```

服务器当前 Cloud 实体已经冻结为 `v2_device_access_grants(logical_device_id, grantee_user_id)`，并单独保存
`viewer/operator`、operator 命令 endpoint 白名单、active/revoked 和单调 version。它只控制 App 账户经
Broker 访问设备，不保存或下发 Direct secret；上面的 `direct_enabled` 是产品/协调状态，不能代替 Cloud
ACL，也不能复用 Legacy 设备→设备分享表。Cloud 分享已冻结为 owner 创建短期一次性码、接收者在登录态
领取：不提供手机号/邮箱枚举，不让用户填写内部 userId，明文码只进入 HTTPS POST body，数据库只保存
SHA-256 verifier。公开管理只暴露随机 `shareId` 与不可逆 `memberRef`。

例如 A 只获得灯，B 获得空调和门锁：服务器只给 A 下发灯的 SharedCredential；每台设备的 Shared secret 独立，
不存在可下载到 App 的“家庭根密钥”。多设备批量分享只是服务器一次写入多条 ACL、分别交付每台设备 secret。

首发便利模式允许 Service 生成每设备 Shared secret、KMS 加密保存，并只通过重新认证的 App HTTPS API 交付给
有效 ACL 用户；App 写入 OS Keychain/Keystore，禁止进入普通 Redux/Vuex、日志、analytics 和 crash dump。
Admin secret 不需要为分享上传服务器。

移除成员时：

1. 服务器立即撤销该用户对该设备的 cloud ACL；
2. 若该用户曾取得 Direct secret，服务器创建该设备 Shared version N+1；
3. 首版由附近的 Admin Direct session 将 N+1 原子写入设备；当前 Runtime 明确拒绝 cloud transport 上的
   ControllerControl。未来若增加 cloud-management lane，必须先完成 Cloud Frame Authentication 并另行冻结授权；
4. 只向仍有该设备权限的用户交付 N+1；
5. 完成前 UI 显示 `DirectRevocationPending`，因为被移除用户仍可能离线使用旧 secret。

组凭据的取舍是移除一个成员会轮换这一台设备的全部 Shared 用户。若产品要求离线逐人立即撤销，必须选择
显式的 per-user credential profile 或短期 server-signed AccessTicket；不能在两槽模型上虚假承诺。

## 6. accessEpoch 与 version

- `accessEpoch`：整台设备转让、所有权恢复或安全清除时递增；旧 epoch 的 Admin/Shared/Grant 全部失效；
- `credentialVersion`：同一 Admin 或 Shared 槽的轮换 CAS；家庭成员变更只前进 Shared version；
- DeviceKey credentialVersion：Cloud 根的独立版本；普通 Key 刷新不应隐式修改 Direct accessEpoch；
- Network reset：只清网络，不改变以上三者；factory reset 才清整个 access store。

设备只需比较非零 epoch 和 version，不需要可信 RTC、owner ID、logical ID 或历史图。

## 7. 设备持久化矩阵

| Profile | 持久化 |
|---|---|
| WiFi-only cloud | DeviceKey + WiFiCredential |
| WiFi + Direct BLE/LAN | 单原子 DeviceAccessStore：DeviceKey + accessEpoch + Admin/Shared；另存 WiFiCredential |
| Platform BLE-only | accessEpoch + Admin/Shared；无 DeviceKey/MQTT |
| Local-only | 最多两条 Local controller，generation=0；物理确认配对，无 server ownership/share |

P6 使用单原子 DeviceAccessStore，避免首次写入 DeviceKey 与 Admin 时出现跨 blob 半成功。standalone
ControllerCredentialStore 继续服务 LocalBLE/旧 Enhanced 组合，不强行携带 DeviceKey 或 accessEpoch。

## 8. 三端实施门禁

设备已完成：两槽 allocation-free table；standalone store 固定 `140 B`；P6 `DeviceAccessStore` 固定
`176 B`；Security1 `Bootstrap` 原子安装 DeviceKey + accessEpoch + Admin；Method 2；server-signed
ControllerControl；Shared 权限钳制；最后 Admin 不可由普通 mutation 删除。Method 2 后的
`DirectSecureRecord` 尚未实现，是 BLE production S1，而不是已完成能力。公开 facade 保持不切换。

App：每设备 ACL/secret，不建立 household master；区分 Admin/Shared；成员删除展示 Direct rotation pending；
Direct secret 只进系统安全存储。

Service/Broker：Cloud 每用户每设备 ACL、viewer/operator、endpoint capability、撤销 version 与 fail-closed
解析已经接入 App BBP/2 lane；公开 invitation/share 管理 API、当前 Broker session gate/replay 的主动失效和
首次 Direct Admin metadata 幂等登记已实现。当前仍逐 Route 查询 Gateway 权威记录，没有 allow TTL cache；
跨实例 durable grant-change outbox 尚未实现。每设备 Shared KMS record/version 仍未实施；Cloud/Direct 撤销状态继续分离；Plain MQTT 在
cloud frame key/MAC/replay E2E 前不进入生产 profile。

本批软件证据：Host 目标严格编译；DeviceAccess/Provisioning/ControllerControl exact 与故障测试；official
aggregate package 验证；ESP32 P3/P6 composition translation unit 使用已安装 3.3.1 工具链编译。Aily Builder
完整 P6 link 被 NimBLE 2.5.0 的 `__has_include` 预处理表达式阻断，因此旧 P6 Flash/RAM 数字已失效，必须在
Arduino 全量 link 后重测，不能沿用为本批结论。

执行顺序修订：先完成并联调 DeviceKey/WiFi、短期 SessionCredential、MQTT CONNECT/SUBSCRIBE 和普通
BBP/2 功能链，再进入 Cloud Frame Authentication 安全批。后者保持 Plain production profile 发布门禁，
但不再阻塞前者的设备功能稳定化。功能链的当前证据、缺口和真实联调清单见
`DEVICE_V2_CLOUD_LOGIN_STABILIZATION.md`。
