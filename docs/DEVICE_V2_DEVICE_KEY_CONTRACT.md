# Device V2 DeviceKey、会话与强制轮换合同

> 状态：2026-08-18 三端权威语义基线。本文取代普通教育/DIY profile 中的
> `EnrollmentKey`、`ManualDeviceKey`、`RecoveryKey` 和“Key 刷新不影响已上线设备”的旧设计。
> 旧 `CloudCredential` 实现暂作为未发布实验保留，但不再是 Education profile 的目标登录根。
> 2026-08-17 已完成 DKEY-1 exact wire、C++/App TypeScript/Node shared golden 和 invalid corpus；
> DKEY-2C 已完成 strict decoder、派生、source/store、44 B 原子记录、非阻塞两步 HTTP auth client，
> 以及不依赖旧 Ownership/CloudCredential/crypto adapter 的 ESP32/UNO R4 internal manual-WiFi composition；
> UNO R4 32 位 ABI 实测 `DeviceKey/Locator/Store/ConfiguredSource/Provider/Lifecycle = 32/16/8/40/280/20 B`，
> DKEY caller-owned arena 精确为 1,260 B。
> DKEY-2D 已完成 transport-neutral DeviceKey provisioning endpoint；当前 v2 endpoint 在 32 位 ABI 下
> 不超过 16 B，并支持 WiFi-only Install 与 WiFi+BLE atomic Bootstrap。ESP32 `network_prov_mgr`
> Security1 薄适配器和 Blinker WiFi journal 单一持久化边界；ESP32 WiFi+BLE 公开设备 facade 已切换，真机 E2E 尚未完成。
> WP-P5/P6 曾完成 SoftAP Security1→NimBLE Direct 的 Plain/TLS internal resource probe；`min_spiffs` 分别余
> 30.02%/25.58% 是控制面接入前的历史基线。P6 当前已改为 260 B DeviceAccessStore schema v3（兼容读 176 B v1/196 B v2）、原子 access bootstrap
> 和 ControllerControl；当前正式单 ZIP 默认 SoftAP 为 `1,406,115/62,848 B`（28.48% Flash 余量），
> `1,395,187/62,848 B` 仅为历史快照。App 双 bearer UX、TLS 新图、动态资源和真机门禁仍未完成，
> 因此不准入生产发布；
> P4 单 Bluedroid 的 7.33% 失败数据只保留在文档，
> 无消费者的 product wrapper/profile/probe 已删除。
> 默认 official WiFi composition 已切换为 DeviceKey；旧 EnrollmentKey/WifiOnboarding/CloudSession 图已物理删除。
> 独立 CloudCredential enrollment 仍只作为有真实测试消费者的历史实验保留，
> 不能作为 App/Service 新实现依据。

## 1. 冻结结论

1. 用户只接触一个名称：`DeviceKey`，App 文案为“设备 Key”。
2. Key 是 32 B CSPRNG secret；外部文本固定为 43 字符、无填充 Base64URL，不带 `bdk_` 或任何其他前缀。
3. Key 不是 `deviceId`、`logicalDeviceId`、MAC、MQTT password、产品标识或页面标识。
4. Education/DIY 默认用 DeviceKey 完成 challenge-HMAC，并换取短期 `SessionCredential`；原始 Key 不在网络上传输。
5. App 可在设备详情重新查看当前 Key；服务器必须用 KMS/HSM/等价主密钥加密保存，而不能只保存不可逆 hash。
6. App 设备详情的独立“刷新 Key”是强制安全轮换：旧 Key、旧 challenge、全部旧版本 SessionCredential 和
   当前连接立即失效，不存在 grace。
7. 手动 WiFi、App BLE 配网和工厂烧录可以用不同方式安装同一种逻辑 DeviceKey；设备业务 API、BBP/2 和
   logical device 不因安装方式变化。
8. `EnrollmentTicket`/provisioning intent 仅是 App 与服务器间 5～15 分钟、单次 transaction 的授权，不替代
   DeviceKey，不进入普通 Education 设备存储，也不参与设备日常重启登录。
9. 再次进入物理 BLE 配网可以执行“事务式刷新”：旧 Key 只保留到设备用新 pending Key 首次完成云端证明；
   成功提交时立即吊销旧 Key、旧 session 和当前连接。该短暂 prepare 窗口不是长期 grace。
10. `ControllerCredential` 仍独立用于 Direct BLE/LAN，但按每设备 Admin/Shared 访问组保存，不再每 App 一槽；
    不能拿 DeviceKey 直接作为 Direct 长连接 bearer。

## 2. 编码与派生边界

### 2.1 外部文本

规范格式：

```text
uGQh7X0g3BP-kavpjUBDMo6peKHewYD5bKLgA9oX2R4
```

严格 decoder 必须：

- 只接受 ASCII `A-Z a-z 0-9 - _`；
- 长度必须正好 43；
- 拒绝 `=`、空白、前后缀、标准 Base64 的 `+ /`、非 canonical 尾位和非 32 B 解码结果；
- 拒绝全零值；
- 不做 trim、大小写转换或“自动修复”；
- parser 错误不得把完整输入写入日志。

Core 内部只保存/借用 32 B raw key；Base64URL 只存在于 Sketch/App/HTTP 管理边界。固定前缀取消后，
secret scanner 应按字段名、43 字符长度和 alphabet 检测，日志层按 `deviceKey` 字段强制脱敏。

### 2.2 Locator、auth key 与 proof

设备登录时不得发送 raw DeviceKey。首版派生已冻结为：

```text
locatorDomain   = ASCII("blinker/device-key/locator/v1")
deviceKeyLocator = first16(SHA-256(locatorDomain || 0x00 || rawDeviceKey))

deviceAuthKey = HKDF-SHA256(
    salt = empty,              # RFC 5869 等价于 32 个 0x00
    IKM  = rawDeviceKey,
    info = ASCII("blinker/device-key/auth/v1"),
    L    = 32)

proof = HMAC-SHA256(deviceAuthKey, exactProofTranscript)
```

`first16` 固定取 digest 左侧 16 B。Locator 只用于服务器索引，仍视为敏感元数据；它不是
`logicalDeviceId`。设备实现通过 restricted-CBOR sink 直接流式写入 HMAC，不需要常驻完整 transcript scratch。

### 2.3 HTTP 与 exact restricted-CBOR

两步登录使用同一 contract header：

```text
Content-Type: application/cbor
Accept: application/cbor
X-Blinker-Contract: device-key-session/1
```

设备按 profile 选择 Plain TCP 或 TLS；一旦选择 TLS 不允许静默降级。任何请求 header/body 均不得携带
raw DeviceKey。

#### Challenge request

`POST /api/v2/device-auth/challenges`

```text
map(5) {
  0: 1,                    # contractVersion
  1: bstr(16),             # requestId，设备 CSPRNG，非全零
  2: bstr(16),             # deviceKeyLocator
  3: bstr(16),             # deviceInstanceId
  4: bstr(16)              # clientNonce，设备 CSPRNG，非全零
}
```

固定最大 75 B。`requestId` 是整个 challenge/session 事务的幂等键，不是 device/session ID。

#### Challenge response

```text
map(6) {
  0: 1,                    # contractVersion
  1: bstr(16),             # exact echoed requestId
  2: bstr(16),             # challengeId，非全零
  3: bstr(16),             # serverNonce，非全零
  4: uint32,               # credentialVersion，>= 1
  5: uint32                # expiresInSeconds，1..300
}
```

编码上界 69 B。服务器 challenge record 必须单次消费、短 TTL，并绑定以上所有字段；使用同一
`requestId` 的 exact retry 可返回同一结果，不同输入复用 requestId 必须报 conflict。

#### Exact proof transcript

proof 输入是 canonical CBOR `array(13)`；数组位置即协议，不得改成 JSON、map 或字符串拼接：

| 索引 | 值 |
|---:|---|
| 0 | text `blinker/device-key/session-proof/v1` |
| 1 | uint `1` contractVersion |
| 2 | bstr(16) requestId |
| 3 | bstr(16) locator |
| 4 | bstr(16) deviceInstanceId |
| 5 | bstr(16) clientNonce |
| 6 | bstr(16) challengeId |
| 7 | bstr(16) serverNonce |
| 8 | uint32 credentialVersion |
| 9 | uint `2` BBP/2 protocolVersion |
| 10 | UTF-8 text firmwareVersion，1..64 B，无 NUL |
| 11 | uint requestedSecurity：`0=PlainTcp, 1=TLS` |
| 12 | uint proofAlgorithm：`1=HMAC-SHA256` |

最坏 215 B，但设备端默认流式 HMAC，不申请这 215 B scratch。

#### Session request

`POST /api/v2/device-sessions`

```text
map(8) {
  0: 1,                    # contractVersion
  1: bstr(16),             # requestId
  2: bstr(16),             # challengeId
  3: 2,                    # BBP/2 protocolVersion
  4: text,                 # firmwareVersion，1..64 B
  5: uint,                 # requestedSecurity：0/1
  6: 1,                    # proofAlgorithm
  7: bstr(32)              # HMAC proof
}
```

编码上界 147 B。服务器必须在“验证 proof、消费 challenge、签发 session”同一原子事务中校验
`credentialVersion == currentVersion`。

#### Session response

```text
map(15) {
  0: 1, 1: requestId, 2: credentialVersion,
  3: broker, 4: host, 5: port, 6: security,
  7: clientId, 8: username, 9: password,
  10: publishTopic, 11: subscribeTopic, 12: sessionId,
  13: expiresInSeconds, 14: refreshAfterSeconds
}
```

按各字段冻结上限计算，canonical response 最大 595 B；设备 HTTP body buffer 门槛因此固定为
request 147 B / response 595 B，不沿用旧 Cloud provider 的 256/640 B 余量。

字段 1/2/6 必须分别等于当前事务的 requestId、challenge credentialVersion 和设备请求的 security。
MQTT clientId/username/password/topic 只是短期 `SessionCredential` 内容，不得落入 DeviceKey store。

### 2.4 Shared golden 与 invalid corpus

权威向量为 `tests/golden/device_key_session_golden_vectors.properties`，由以下三个独立实现消费：

- C++：`device_key_session_golden_main.cpp`；
- App TypeScript：`app_device_key_session_golden.ts`；
- Service Node：`server_device_key_session_golden.cjs`。

当前逐字节冻结 9 项：Key text、locator、auth key、challenge request/response、proof transcript/proof、
session request/response；共享 invalid corpus 覆盖短长度、旧前缀、padding、标准 Base64 字符、非 canonical
尾位、全零和尾随空格。

## 3. 对象与生命周期

| 对象 | 保存方 | 生命周期 | 作用 |
|---|---|---|---|
| `DeviceKey` | 设备/Sketch；服务器 KMS 密文；App 按需 reveal | 直到刷新、删除或转让 | 设备根认证 |
| `deviceKeyLocator` | 设备可派生；服务器唯一索引 | 跟随 Key version | 查找认证记录，不是身份主键 |
| `credentialVersion` | 服务器权威；设备/session 携带 | 单调递增 | 轮换和强吊销 epoch |
| `Challenge` | 服务器 + 设备 RAM | 单次、短 TTL | 防重放登录 proof |
| `SessionCredential` | 服务器签发；设备 RAM | 一次连接或短 TTL | MQTT/WebSocket 登录和 ACL |
| `EnrollmentTicket` / provisioning intent | App/服务器 RAM/事务表 | 5～15 分钟、单事务 | 授权 App 创建 pending Key；不作为设备登录材料 |
| `Admin/Shared ControllerCredential` | App Keystore/Service KMS + 设备 Direct store | 每设备访问组 | Direct BLE/LAN；不用于 MQTT |
| `accessEpoch` | Direct-capable 设备 + 服务器 | 整机 Direct 访问代次 | 转让/恢复后拒绝旧组凭据；不是 owner 数据 |

普通 Education product graph 不再生成第二把长期 `CloudCredential`。WiFi-only MQTT 不保存 Direct
ControllerCredential；服务器根据每用户每设备 ACL 转发。现有 CloudCredential/Enrollment/Store
源码在 DKEY-4 前只能留在 Advanced/Enhanced 实验包，不得进入普通 QuickStart 或默认 official composition。
若以后 Factory profile 采用不可导出的非对称设备身份，应作为显式增强 profile 设计，不能再次向初学者暴露
多种“设备 Key”。

## 4. 三种安装方式

### 4.1 手动 WiFi

1. App 创建 logical device/PageSchema 和当前 DeviceKey。
2. 用户随时从设备详情查看、复制 Key，并写入 Sketch。
3. `Blinker.begin(...)` 解析 Key；设备用代码中的 WiFi 信息联网。
4. 设备执行 challenge-HMAC，取得短期 SessionCredential，再连接 MQTT/WS。
5. 擦除 Flash、换电脑或重新烧录后，只要仍使用 App 页面显示的当前 Key，就回到同一 logical device。

### 4.2 ESP32 App 配网

1. 用户让设备进入短时物理配网窗口；每次进入均表示首次安装或事务式刷新 DeviceKey。
2. App 使用 ESP32 WiFiProv Security1 建立受保护会话。已有 Key 时可从当前 Key 派生 PoP；全擦/首次设备仍需
   Sketch、Serial、标签或显示屏提供的 setup PoP，不能凭空获得设备真实性。
3. App 调用 `blinker-config` v2 的 `GetInfo`，取得 public `deviceInstanceId`、flags 和 accessEpoch，再以已登录用户身份
   向服务器创建短期 provisioning intent。服务器生成 pending N+1 DeviceKey，但暂不吊销当前 N。
4. WiFi-only P3 调用 45 B `Install(version, op, DeviceKey[43])` 并写 44 B DeviceKeyStore。Direct-capable P6
   首次调用 101 B `Bootstrap(version, op, DeviceKey[43], accessEpoch, adminGroupId[16],
   credentialVersion, adminSecret[32])`，设备固定 Admin=All，并通过当前 260 B DeviceAccessStore schema v3 一次原子提交。
   P6 已有 access state 后可用 Install 刷新 Key且保留 epoch/Admin/Shared；首次只发 Install 返回 AccessRequired。
5. App 再使用 WiFiProv 标准 endpoint 下发 WiFi。ESP32 adapter 把 credential candidate 写入 Blinker 双槽
   journal；Arduino WiFi 使用 RAM persistence，不在 SDK NVS 中再保存一份。
6. 设备联网后直接用新 Key 执行普通 challenge-HMAC。服务器第一次验证 pending Key 成功时原子激活 N+1，
   吊销 N、全部旧 session 并主动断开旧连接；设备不发送或保存 Ticket。
7. WiFiProv 完整 stop/deinit；需要 Direct BLE 的产品随后切换到独立 BBP/2 BLE profile。

掉电边界因此只有两种可恢复结果：本地替换前仍持有有效旧 Key；替换后持有服务器已登记的 pending 新 Key并可在
下次启动继续 challenge。禁止先吊销旧 Key 再等待 BLE 写 Flash，也禁止让 5～15 分钟 Ticket 到期导致已安装的新
Key 一个月后无法登录。

### 4.3 预烧录/通信模块

工厂工具可预烧录同一 DeviceKey；UNO R4 或普通 MCU 也可以把 DeviceKey、TLS/MQTT 和配网责任放入受控
Connectivity Agent/NCP。主 MCU 只通过有界 BBP/2/串口协议使用连接能力，不复制网络 secret。

## 5. 刷新 Key 的强制语义

### 5.1 App 独立刷新

App “刷新 Key”必须二次确认：

> 刷新后，使用旧 Key 的设备会立即离线，旧登录信息不能再次连接。请重新烧录或重新配网。

服务器在一个事务中：

1. 生成新 32 B Key、新 locator，并使 `credentialVersion = old + 1`；
2. 立即将旧 Key 标记 revoked，取消全部绑定旧 version 的 Pending provisioning/recovery transaction；
3. 吊销所有 `session.credentialVersion < current` 的 SessionCredential/JTI；
4. 使 Gateway/Broker 鉴权缓存失效；
5. 主动断开旧 version 的 MQTT、WebSocket 和其他长连接；
6. 写入不含 raw secret 的审计事件；
7. 只向已重新验证权限的 App 返回新 Key。

Broker 只在 CONNECT 时检查版本是不合格实现；刷新事务必须能定位并关闭现有连接。短 TTL 只是纵深防御，
不能代替主动吊销。设备收到 `DeviceKeyRotated/CredentialRevoked` 后必须清除 RAM session、停止重试旧
SessionCredential，并进入明确的 `AuthorizationRequired` 状态；禁止回退 legacy authKey 或残留 CloudCredential。

独立刷新不自动把新 Key 推给旧设备，否则无法可靠驱逐疑似泄漏/复制实例。用户需要重新烧录，或重新进入物理
配网窗口安装 App 当前显示的 Key。

### 5.2 BLE 再配网的事务式刷新

BLE 再配网是另一个明确操作，目标是避免设备在写入/联网中途掉电而永久失联：

1. 要求 owner App 登录、物理 setup window 和 Security1 会话；
2. 服务器创建 pending N+1，N 在 prepare 阶段仍可用；
3. 设备原子保存 N+1 后，只用 N+1 challenge；
4. N+1 proof 成功即在一个服务器事务中激活 N+1、吊销 N 和全部 N session，并断开 N 连接；
5. 同一 intent 重试返回相同 pending Key；取消/另开事务必须显式处理，不能静默生成多个候选。

它最终与刷新具有相同强吊销结果，但不会把“BLE 写入成功”“WiFi 连接成功”或 custom endpoint 2xx 冒充云端
提交。若用户的目标是立即驱逐疑似泄漏副本，仍应使用 5.1 的独立刷新，而不是事务式再配网。

## 6. 重置、擦除与 logical device

- Network Reset：只清 WiFi，不改变 DeviceKey/version/ownership。
- 应用升级或普通重启：保留 DeviceKey，重新申请 SessionCredential。
- 全片擦除：设备丢失 Key；用户重新粘贴/配网当前 Key 后回到原 logical device。
- 独立 Key 刷新：logicalDeviceId/PageSchema/历史不变，只变 credentialVersion；旧实例立即全部离线。
- BLE 再配网：最终同样前进 credentialVersion，但在新 Key 首次 proof 前存在可恢复的 pending prepare 状态。
- Ownership Transfer/Delete：必须另走账号授权和 generation/ACL 清理；不能用刷新 Key 冒充转让。
- MAC、广播名、`deviceInstanceId` 只用于发现/诊断，不能绕过当前 DeviceKey 重新上线。

在初学者默认模式中，不再要求用户理解独立 RecoveryKey。服务器可以在内部做同 logical device 的实例替换
风控，但对持有当前 DeviceKey 的合法用户应给出清晰确认流程，而不是随机新建设备页。

## 7. Plain/TLS 安全边界

TLS 继续是可选 build/profile，不强制所有 MCU：

- Plain profile 仍必须使用 challenge、随机 nonce、HMAC、短时 session、消息级 MAC、序号和防重放；当前
  DeviceKey session v1 没有独立 cloud frame key，故 Plain internal graph 只算资源候选，不具备生产准入；
- raw DeviceKey 不得出现在 HTTP/MQTT、普通 BBP/2 或未加密 BLE 报文；唯一例外是短时 Security1 provisioning
  session 内的 `blinker-config.Install`，并且不得进入日志、analytics 或 crash dump；
- Plain 只能提供设备认证/消息完整性，不能宣称业务数据保密；被动监听者仍可能看到业务数据和短期 session；
- 手动写入 Sketch/普通 Flash 的 DeviceKey 可被固件读取或复制，属于 Education 便利性取舍；刷新可驱逐副本，
  但不能把普通开发板升级成防克隆硬件；
- TLS profile 使用平台 `WiFiClientSecure/WiFiSSLClient` 或 NCP，并禁止已选择 TLS 后静默降级；
- Direct BLE 的可移植默认基线为普通 GATT + Noise enrollment + Method 2 + DirectSecureRecord；可靠的平台
  link encryption/bonding 只能作为显式增强能力，不能替代 ControllerCredential、应用 AEAD 或授权。

## 8. 三端职责

### Device library

- 提供 no-heap strict Key decoder、raw holder/source/store、locator/proof 派生和 session client；
- session request/response/credential buffer 由 composition 注入；body 精确门槛为 147/595 B，provider 自身不持有大 body scratch，32 位对象
  固定门禁为 280 B；恒定 HTTP header 使用共享只读描述符，proof transcript 直接流式写入 HMAC；
- 手动 Sketch 与 BLE/NCP 安装最终汇入同一 `IDeviceKeySource`；
- 默认 product graph 移除 CloudCredential enrollment，启动只走 DeviceKey challenge；
- 所有 session 与 `credentialVersion` 绑定；revoked/rotated 失败不回退；
- 原子存储只保存必要 raw Key/version，不保存 App JWT/Ticket/WiFiProv PoP 日志。

### App

- 设备详情长期 reveal/copy/refresh 当前 Key，完整显示需要重新验证用户权限；
- 刷新前显示强制离线影响，刷新后立即清除剪贴板提示和本地旧值；
- ESP32 WiFiProv coordinator 在受保护内存短暂持有 Key/Ticket，不进入 analytics/crash log/普通 state store；
- 通过服务器 onboarding status 判断 Cloud Ready，不用 BLE 2xx/本地 boolean 冒充上线。

### Gateway/Broker

- KMS 密文保存 raw Key，locator 唯一索引，版本单调递增；
- challenge 单次、短 TTL、绑定 locator/version/device instance 和 server nonce；
- SessionCredential wire 含 credentialVersion、session ID、TTL、MQTT/transport scope；Gateway 由
  DeviceKey locator/version 在服务器侧解析 logicalDeviceId，设备响应不复制该逻辑路由字段；
- rotate 事务同时完成旧 Key、session、缓存和连接强吊销；
- Legacy authKey 与 V2 DeviceKey 使用不同表、route、metric 和 broker namespace。

## 9. 最小错误语义

| wire 值 | HTTP / retryClass | 含义与设备行为 |
|---:|---|---|
| `3000 Malformed` | 400 / `Never(0)` | exact body 非法；停止该请求 |
| `3001 UnsupportedVersion` | 400 / `Never(0)` | 协议版本不支持；不得降级 legacy |
| `3002 UnsupportedAlgorithm` | 400 / `Never(0)` | proof 算法不支持；不得静默换算法 |
| `3100 DeviceKeyInvalid` | 401 / `AuthorizationRequired(4)` | locator/proof 非法；停止自动重试并提示检查 Key |
| `3101 DeviceKeyRotated` | 403 / `AuthorizationRequired(4)` | Key version 已落后；清 session 并提示查看当前 Key |
| `3102 CredentialRevoked` | 403 / `AuthorizationRequired(4)` | 当前设备访问被撤销；等待用户操作 |
| `3103 ChallengeExpired` | 409 / `NewChallenge(3)` | challenge 已消费/超时；只重新申请一次新 challenge |
| `3104 ChallengeConflict` | 409 / `Never(0)` | requestId/exact input 冲突；不得用同 ID 改 body |
| `3105 SessionRevoked` | 401 / `AuthorizationRequired(4)` | 清 RAM session，不用旧 token 重连 |
| `3106 DeviceRevoked` | 403 / `AuthorizationRequired(4)` | logical device/instance 被撤销；停止自动重连 |
| `3500 RateLimited` | 429 / `Backoff(2)` | 只按有界 `retryAfterSeconds` 退避 |
| `3501 TemporarilyUnavailable` | 503 / `SameRequest(1)` 或 `Backoff(2)` | exact retry，不生成新事务 |

`ProvisioningAuthorizationRequired`、Ticket expiry/scope 等属于独立 WiFiProv/EnrollmentTicket contract，
不得混入 DeviceKey session error body。

## 10. 实施批次与冻结门禁

1. **DKEY-1 三端 wire freeze（完成）**：派生 domain、challenge/proof/session exact body、错误码、
   C++/TS/Node golden 和 invalid corpus 已冻结。
2. **DKEY-2A/B Device Core（完成）**：Key decoder/source/store、exact 1,260 B arena 与非阻塞 HTTP auth client。
3. **DKEY-2C manual WiFi composition（完成）**：平台最小网络层、统一 WiFi cloud lifecycle、ESP32/UNO R4
   公开 facade、official Product graph 与聚合包门禁；release example 仍使用占位 host，不冒充生产 route。
4. **DKEY-2D device provisioning（设备软件门禁完成）**：v2 endpoint 的 32 位 ABI 上限 16 B，P3 使用
   DeviceKeyStore，P6 使用 260 B DeviceAccessStore schema v3（兼容读 176 B v1/196 B v2）；ESP32 WiFiProv Security1 adapter 与早期
   `DeviceKeyStore + WifiCredentialStore` compile/resource probe 已完成；默认 1.31 MB APP
   分区超限，`min_spiffs` 为 `1,689,811 B Flash / 58,268 B global RAM`。真机/APP 联调仍未完成；UNO R4 先维持
   manual DeviceKey；不实现 BLE 配 WiFi。
5. **WP-P3/P5/P6 设备图**：ESP32 WiFiProv 写入的 NVS DeviceKey/WiFi 直接进入共享
   challenge/session、PubSubClient 与 BBP/2 stack。P3 Plain/TLS 为 `1,758,907/62,964 B`、
   `1,845,907/63,556 B`，TLS 只余 6.11%，拒绝作为默认；P6 SoftAP+NimBLE Plain/TLS 为
   `1,375,887/60,856 B`、`1,463,067/61,456 B`，双 OTA 余 30.02%/25.58%，这是 access/control 注入前历史数字。
当前历史 P3 Plain 为 `1,762,543/62,980 B`；当前正式单 ZIP P6-S Plain 为 `1,406,115/62,848 B`，
`1,395,187/62,848 B` 仅为历史快照；其 ELF 审计只含
   SoftAP、NimBLE、DeviceKey 与 Controller HMAC，不含 CloudCredential/Ownership/Noise/Bluedroid。IDF-NimBLE
   目标可编译期选择 P6-B BLE WiFiProv；XIAO S3 为 `1,303,487/59,684 B`，但仍待真机生命周期门禁。
   TLS 新图仍须重测。
6. **DKEY-3 Service/Broker（可延后但生产阻断）**：实现 KMS record、challenge issuer、versioned session 和
   rotate+disconnect E2E。
7. **DKEY-4 Education composition cutover**：默认 ESP32/UNO R4 WiFi 已使用 DeviceKey；旧 WiFi+BLE
   CloudCredential/Ownership product graph、package 和 probe 已物理删除，不作为 Enhanced 变体保留。
8. **DKEY-5 App UX**：reveal/copy/refresh、强制离线提示、状态恢复和日志/剪贴板门禁。
9. **DKEY-6 E2E**：旧 Key、旧 MQTT credential、已连接 socket、缓存 broker 节点、掉电和全擦重装均验证。

DKEY-2D 当前已通过三端 shared golden、deterministic fuzz、architecture check、official aggregate package
boundary 和真实 Arduino compile/resource probe。R-ARCH-6 删除 3 个旧登录图专属测试后，当前 Host MSVC
ASan 为 `40/40`。
当前完整 UNO R4 Small SizeProbe 为 `52,436 B Flash / 16,628 B global RAM`；其中
`DeviceKeyProvisioningEndpoint` 的历史测量为 12 B；P6 access pointer 接线后当前冻结为 `<=16 B`，旧值
不再是 ABI 断言。DKEY-2C 新增的
`ConfiguredSource/Arena/WifiCloudLifecycle = 40/1260/20 B` 三个测量数组恰好贡献 1,320 B，不能解释为产品图
重复开销。Plain internal composition 实测：ESP32 `986,287/48,360 B`、UNO R4 `118,164/11,200 B`
（Flash/global RAM）；相对同轮旧图分别减少 `37,536/2,216 B` 与 `40,976/2,104 B`。默认公开 WiFi facade
现已只指向 DeviceKey 图；DKEY-3/5/6 的 production Service、App 刷新 UX 和旧连接强吊销 E2E 仍是发布阻断，
因此不得把 release example 的占位 route 或“刷新 Key”描述成生产可用。
