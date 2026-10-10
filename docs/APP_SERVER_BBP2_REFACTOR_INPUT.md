# App / 服务器对接 Device V2 的正式执行方案

> 跨模式上线与通信协议的同事沟通入口：`DEVICE_V2_MODES_ONLINE_AND_PROTOCOL_OVERVIEW.md`；手动设备 Key
> 权威合同：`DEVICE_V2_MANUAL_DEVICE_KEY_CONTRACT.md`。

> 状态：2026-08-17 设备侧正式修订输入。生产 App 与服务器尚未修改；设备仓中的 TypeScript/Node
> golden 只是独立 reference codec，不能当作生产接入已完成。
> WiFi/BLE/WiFi+BLE 身份、Claim 和 credential 边界已由
> `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 正式修订。本文件已同步取消“所有 Developer 设备统一使用
> SetupCode”的旧假设；WiFi-only Ownership/Cloud exact wire、旧 EnrollmentKey HTTP软件探针与 R4b-4a
> EnrollmentTicket signed common body、普通 DIY Noise NN secure-session Core wire、R4b-4c LocalSetup
> application envelope 与 R4b-4d durable setup saga 已冻结。生产 driver/composition 仍未完成，生产端
> 不得接入旧 Enrollment/1；ConfigureNetwork 与 App-relay Cloud exact body 以本文和 golden 为准。
> Platform BLE 已作为统一设备 facade/package 编译到 ESP32 与 UNO R4；LocalBLE 仍为双平台 internal package。
> Platform BLE 需要 App/Gateway relay Ticket/Claim/Controller，但没有 CloudCredential/MQTT；LocalBLE 完全不访问
> Gateway。App 不能再把“纯 BLE”当成一个授权语义未定的 transport 开关。
> UNO R4 产品边界已冻结为独立普通 WiFi 与普通 BLE；App/Service 不得为 UNO R4 实现 BLE 配 WiFi或
> WiFi+BLE 产品分支。
>
> **2026-08-18 DKEY-2D 覆盖说明：**普通 Education WiFi/WiFi+BLE 已改为长期无前缀 DeviceKey 直接
> challenge-HMAC 登录，不再以 ManualDeviceKey -> Ownership -> CloudCredential 作为默认链路。本文后续保留的
> ManualDeviceKey、Ticket exact relay、8-operation LocalSetup 与 CloudCredential 段仅是 Enhanced/历史合同。
> ESP32 WiFiProv custom endpoint 不接收 Ticket：P3 执行 `GetInfo + Install DeviceKey`，P6 首次执行
> `GetInfo + atomic Bootstrap(DeviceKey, accessEpoch, Admin)`，后续 Key 刷新再用 Install；权威语义以
> `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 为准。

> 2026-08-16 第五片更新：Ownership Reset/Transfer v1 已冻结，旧 Enrollment/1/Reset 实现已从设备仓
> 物理删除。App/服务器必须接入 `DEVICE_V2_OWNERSHIP_RELEASE_CONTRACT.md` 和共享
> `ownership_release_golden_vectors.properties`；WiFi EnrollmentKey HTTP outer 后续已被 ManualDeviceKey 决策取代，
> EnrollmentTicket 当时尚未冻结；当前 R4b-4a 已由后续更新替代该状态。

> 2026-08-16 R4b-3 第三片更新：设备公共层已实现 WiFi → Onboarding → Cloud/MQTT staged lifecycle 和
> 1,414 B 共享 arena。该时点缺少的真实 crypto provider 已由下一段第四片补齐软件/SDK 门禁；
> App/Gateway 仍不得据此假设现有生产路由或真机已经可联调，也不得为绕开设备验签而返回空 2xx、JSON
> 或 TLS-only 特例。

> 2026-08-16 R4b-3 第四片更新：设备已有三种窄 provider：ESP32 native X25519/AES-GCM/P-256 verify、
> UNO R4/Wio portable X25519/AES-GCM/Ed25519 verify，以及 UNO R4 可选 `SoftwareATSE` P-256 verify。
> 这完成的是软件/SDK 门禁，不代表生产 endpoint 或真机已经可用。

## 0. R5 正式实施主线（2026-08-17 代码审查后冻结）

> 2026-08-18 鉴权/分享修订：权威合同改为
> `DEVICE_V2_AUTHORIZATION_AND_SHARING_DECISION.md`。WiFi-only MQTT 不保存 owner 或 Direct credential，
> Gateway/Broker 按每用户每设备 ACL 转发；Direct 使用每设备 Admin/Shared 两槽，Shared 不是家庭全局 key。
> 删除成员先立即撤销 cloud ACL，再轮换该设备 Shared version；设备确认前 App 显示
> `DirectRevocationPending`。当前 Plain MQTT 缺少独立 cloud frame key/MAC/replay，三端 E2E 前不得生产准入。

本轮已对照生产仓实际代码完成审查：App 当前仍是 FFE0/FFE1 + 换行 JSON、MAC/deviceName 注册和一个
混合 BLE/MQTT/页面/在线状态的大 service。Gateway/Broker 已新增独立 V2 DeviceKey/session 与
Manifest/State 子链，但旧 authKey/JWT + JSON/TLV、含 widgetKey/widgetType 的 TLV schema 仍只服务 Legacy；
它们都不是 App V2 页面与数据面的基类。

R5 的代码边界冻结为：

~~~text
Device BBP/2 -- MQTT --> Gateway credential/ACL --> Broker BBP/2 terminator
                                                      |
                                                      +--> normalized stable-key state/router
App cloud <-- account MQTT-over-WebSocket JSON -------+

App Direct BLE <-- independent BBP/2 session --> Device
~~~

App cloud 使用 stable-key normalized JSON 是有意设计：浏览器/App 不受 MCU 的 400 B/CBOR 约束，可读性更重要；
它不是 Legacy JSON device wire。Broker 必须终止 device sequence/ACK/Field ID/replay，不能把 raw BBP/2 透传
给 App。Direct BLE 则保留原生 BBP/2，并通过 adapter 落到同一个 App reducer。

2026-08-18 设备 P4 曾新增单 Bluedroid Direct BLE internal graph，但它只证明 transport、BCT2 store 和
Controller Method 2 可以与 WiFiProv/DeviceKey cloud 共用一个 `Client`。设备侧 Controller Grant/receipt 已接入
BBP/2 control message family；R5-2 的 App/Service 仍须实现
`control-window nonce -> signed Grant + raw secret over encrypted lane -> durable receipt -> Method 2 confirm ->
idempotent Commit`；不得用 DeviceKey 派生 controller secret，也不得因 BLE 可发现就标记 Direct Ready。该 P4
图仅余 7.33%，P6 验证后无消费者的 device wrapper/profile/probe 已删除，App/Service 不应为 P4 新增产品分支。

2026-08-18 设备 P6 已把 Network Provisioning 的公共 transaction 与 BLE/SoftAP scheme 拆成独立链接单元。
唯一 ESP32 WiFi+BLE 业务图为 `WiFiProv Security1 -> DeviceKey cloud -> NimBLE Direct`：SoftAP 是默认 bearer，
IDF-NimBLE 目标可编译期选择 BLE。当前正式单 ZIP Plain 为 `1,406,115/62,848 B`，双 OTA slot Flash 余
28.48%；`1,395,187/62,848 B` 仅为历史快照。旧双平台 CloudCredential package/probe 已删除；UNO R4
永久不提供 WiFi+BLE 产品。App 必须支持设备声明的两种 WiFiProv bearer：SoftAP 路径需处理扫描/系统入网授权、
临时 AP 断开后的原网络恢复；BLE 路径需处理发现、连接和重连；二者复用 standard endpoint +
`blinker-config` 顺序、超时/重试/取消和云 Ready 回查。SoftAP WPA `serviceKey` 与 Security1 PoP 是独立凭据；
若产品只向用户展示一个 setup code，App/设备必须使用冻结的域分离派生，日志和错误码也要区分 AP join 失败与
Security1 proof 失败。BLE bearer 不使用 SoftAP `serviceKey`。Service wire 不感知 BLE/SoftAP，仍只管理 pending DeviceKey intent、challenge/session 和
状态。设备端虽已接入 Controller Grant control lane，App/Service、TLS 新图与真机仍未完成，不能因资源通过就
标记生产可用。

“上线”拆成四个事实：

| 状态 | 唯一成立条件 |
|---|---|
| `sessionIssued` | `/api/v2/device-sessions` exact 200 已签发短期 credential |
| `transportConnected` | Gateway/broker 已接受该 credential 的 MQTT CONNECT |
| `protocolReady` | HELLO、所需 auth、当前 Manifest 完整验证/接受；EndpointIds 被协商时再要求 ID gate |
| `stateFresh` | 同一 sessionEpoch/revision 的 StatePage 全部收齐并原子提交 |

因此 HTTP 200、MQTT CONNECT、单个 PATCH、BLE 可发现或 Key/Ticket 消费都不能直接写全局 `online=true`。
断连只结束当前 sessionEpoch 并使 state stale，不清 Ownership/CloudCredential。设备重启后 stateRevision 可
归零，App/服务器不得跨 session 或 Direct/cloud source 取 revision 最大值。

R5 顺序固定为：

1. **R5-1 production codec**：服务器建立单一纯 protocol package；App 建独立 `src/app/device-v2` codec，
   两边跑全部 exact/invalid golden 与 fuzz，零 Legacy import；
2. **R5-2 control plane**：Gateway V2 migration/repository、Key/Ticket、Claim/Cloud/Controller/Release exact
   endpoint 与 KMS key ring，并冻结 Controller control 的 BBP/2 message family、设备 dispatch 与 App Method 2
   confirmation；
3. **R5-3 cloud session**：SessionCredential、Gateway ACL、Broker CONNECT identity、TTL/revoke/断旧连接；
4. **R5-4 data plane**：Broker BBP/2 session、Manifest/State staging、normalized router 和 App account stream；
5. **R5-5 App coordinator/page**：secure BLE、四种 onboarding reducer、source-lane state、PageSchema renderer；
6. **R5-6 E2E**：fake device/App/Gateway/Broker 故障注入，再进入 ESP32/UNO R4 真机与 R4b-7 facade。

2026-08-19 当前已完成 R5-4 的 Broker HELLO/Manifest、State snapshot/latest-state、Report Patch 与
Gateway owner Manifest/State 读取子集；normalized App stream、控制下行、event/history 和 E2E 未完成，
所以 R5-4 仍是进行中。

详细落点分别见 App 与服务器仓的 `docs/DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md`。当前先完成设备侧
R4b-2F ManualDeviceKey/Recovery exact contract、codec 和 shared golden，再执行 R5-1；在 codec/invalid gate
前不得先写 route、ORM 或 UI happy path。

### Staged composition 对 App/Gateway 的新增约束

- 注册完成前设备不会启动 Cloud Session/MQTT；App/Gateway 不能把 MQTT 在线作为 Ownership/Cloud
  transaction 的前置条件；
- 同一设备断网重连只恢复 durable exact transaction；Active 后不再发送 ManualDeviceKey。Gateway 必须允许
  Bound/Pending 事务在 recovery TTL 内幂等收口，不能要求设备每次联网重新生成 Key；
- `ServerSignatureAlgorithm` 和 `envelopeAlgorithm` 必须按 wire 值进入生产 registry/metrics，未知值严格
  返回 UnsupportedAlgorithm，不能静默选择另一算法或把 transport TLS 当成 body 验证；
- Cloud envelope/Noise v1 仍固定 X25519/HKDF-SHA256/AES-256-GCM，`envelopeAlgorithm` 不因 MCU
  改变。server signature 已分配 Ed25519=1、raw low-S ECDSA-P256-SHA256=2；服务器按受信的
  product/platform registry 固定算法与 keyId，不能接受未认证设备自行选择。App 始终只 relay exact bytes；
- server signing keyId、公钥轮换窗口和 encryption keyId 必须在正式联调前准备好。设备 official package
  不会接受恒真 verifier、开发空 key ring 或未固定的远端公钥。

Gateway 对 Ed25519 必须把 exact transcript 的 SHA-256 结果作为 32-byte message 签名；对 P-256 必须按
pre-hashed SHA-256 digest 生成固定 64 B `r || s`，并规范化为 low-S。不得把 Ed25519 再内部 hash 一次后
却仍声称签的是相同 wire digest，也不得输出 DER ECDSA。unknown algorithm/keyId、非 canonical S/R、
重复 keyId 都必须 fail closed。App 不解释签名内容，但 BLE/HTTP relay 必须逐字节保留 algorithm、keyId
和 signature。

### Ownership Release 三端交接增量

- App 在已认证 controller/cloud 会话内发起 Reset 或 Transfer；Reset 可选择 Preserve/Clear Network，
  Transfer 只能 Clear。App 透明 relay exact Request/Grant/Receipt/Ack，不能重编码；
- App 在收到设备 Released/事务完成前保留当前 ControllerCredential，完成后才删除对应 Keychain/Keystore
  项；中途断线按 requestId/grantId 恢复，不能创建第二个 release；
- Gateway 对 `(deviceInstanceId,currentGeneration,requestId)` 做 generation CAS 与 exact-byte 幂等，保存
  requestDigest、exact Grant、receiptDigest 和状态；Grant 与 CommitAck 都必须用 pinned server key 签名；
- Gateway 只在已认证外层会话接受无独立 proof 的 Receipt；成功提交 registry 后返回 exact signed Ack。
  HTTP 2xx、App 本地 boolean 或 MQTT PUBACK 都不能替代 CommitAck；
- common body 禁止增加 owner/tenant/productId/MAC/JWT/Ticket。审计只记录 ID、generation、operation、
  policy、digest、keyId、状态和结果，不记录完整 body、凭据或签名 transcript。

## 1. 本次纠正

设备侧不再发布 WidgetCatalog，也不声明 Button/Switch/Slider/Text。三端模型冻结为：

~~~text
Device Manifest                       App / Server assets
  Property / Action / Event             PageSchema
  type / access / constraints            WidgetRegistry
  stable key / session Field ID           explicit bindings
              \                           /
               ---- normalized router ----
~~~

因此 App/服务器不得等待 Catalog frame，也不得从 field key 后缀猜组件。`0x07/0x08`、HELLO
key 10/11 和 feature bit 8 是保留值。所有旧 WidgetCatalog/typed widget 任务从 backlog 删除。

## 2. 数据所有权

| 数据 | 权威方 | 设备保存 | App 保存 | 服务器保存 |
|---|---|---:|---:|---:|
| Manifest 能力 | 设备/产品 | Flash descriptor | 已验证缓存 | immutable registry |
| 当前 Property 状态 | 设备 | 有界状态仓 | UI cache | latest state/history |
| Action/Event | 设备语义 | 不持久或业务自管 | 消费结果 | 路由/审计/历史策略 |
| PageSchema/layout/style | 用户/App | 否 | 工作副本/cache | 用户资产 |
| WidgetRegistry/renderer | App 版本 | 否 | 代码/签名资产 | 可选模板分发 |
| Binding | 用户/App | 否 | PageSchema 内 | PageSchema 内 |
| CBOR body | 当前消息发送方 | 临时 buffer | 临时 buffer | 临时 buffer/可选原始归档 |
| ManualDeviceKey | App/Console 创建 | 手动设备 Sketch可长期保留 | 设备页长期显示/复制/刷新 | KMS 加密 secret + version/state |
| EnrollmentTicket | Gateway 签发 | 只允许 Pending 事务暂存 | 只放内存/安全会话 | exact ticket/状态/TTL |
| CloudCredential | 设备 | 原子持久化 | 永不持有 | HMAC 首版保存 KMS/HSM 加密等价 key/版本；future signing suite 才只存 public key |
| ControllerCredential | App/设备 | 16 B ID + 32 B HMAC secret + permissions/version/generation | OS Keychain/Keystore | ID、role 到 permissions 映射、generation/version；不保存 Direct BLE secret |

CBOR 是 wire encoding，不是数据库格式，也不是阿里云式“设备生成页面”的物模型。服务器可以把
解码结果存成关系表、JSONB 或时序数据；设备端不保存页面。

## 3. App 页面与交互模型

建议 PageSchema 最小形态：

~~~json
{
  "pageId": "main",
  "widgets": [{
    "id": "ui-power",
    "type": "switch",
    "layout": { "x": 0, "y": 0, "w": 2, "h": 1 },
    "style": { "title": "Power" },
    "bindings": {
      "value": { "key": "power" },
      "write": { "key": "power" }
    }
  }]
}
~~~

一个统一 WidgetRegistry 定义每种 App 控件所需 slot 与兼容规则，例如 switch.value 要求 bool
Property Read，switch.write 要求同一或另一 bool Property Write；button.invoke 要求 Action；
chart.value 可绑定数值 Property；toast.event 可绑定 Event。规则只读取 Manifest kind/type/access/
constraints，不读取 MCU 型号、SDK、field key 命名规则。

拖拽编辑器、产品模板和 Agent 生成器都输出同一个 PageSchema。前端可采用 JS/TS registry 与
renderer，但只执行随 App 发布或服务器签名的受信代码；设备不得下发任意 JS。设备数据只能作为
受 schema 校验的值、样式 token 或资源标识，不进入 `eval`/动态脚本。

自动页面建议由 App/服务器实现：按 Property 类型与 access 选择安全默认 widget，再允许用户修改。
建议不是设备协议资产，Manifest 更新不得覆盖用户页面。binding 永久保存 stable key；Field ID 只在
当前已验证 session 内缓存。

PageSchema 还必须保存 `sourceManifestFingerprint/sourceCapabilitySummary/pageRevision`。新 fingerprint
触发 `stale-compatible/stale-breaking`，按 fingerprint 只提示一次：新增 endpoint 进入可添加区，删除
endpoint 禁用旧 binding，type/access/constraints 改变标记 `needsReview`。用户确认新 PageSchema revision
前不切换 source fingerprint；选择稍后继续使用兼容 binding。手动拖拽、模板和 Agent 必须调用同一个
reducer/WidgetRegistry，Agent 只提交受校验的声明式 patch，不产生可执行 JS。

### 3.1 状态到组件的更新链路

App 在 PageSchema 校验后建立 `stable key -> widget slot[]` 反向索引。一个 Property 可以绑定多个
组件或非 UI 消费者，一个组件也可拥有多个 Property slot。例如：

~~~text
power       -> ui-power.value, ui-status.value, voice.power
temperature -> ui-temperature.value, ui-summary.subtitle
~~~

收到 PATCH Report `{revision, values}` 时必须按以下顺序处理：

1. 用当前 session 的 ManifestAccept map 将 Field ID 还原为 stable key；keyed baseline 可跳过此步；
2. 校验 kind/type/access、revision 连续性和整个 values map；
3. 将全部 Property 原子提交到 normalized device state；
4. 通过反向索引收集受影响 widget，合并成一次 renderer transaction；
5. 若 revision 缺失/跳跃或 Manifest fingerprint 改变，丢弃增量并重新请求完整 State snapshot。

首次连接和重连不能边收 StatePage 边改 UI；只有同一 revision 的所有页完整 staging 并校验后才能
一次发布。Direct BLE 与 cloud 路径执行同一流程，差别仅在独立 session/Field ID map。

组件写入时，App 根据 `write` binding 发送 Property COMMAND，并显示 pending/错误状态；请求值不是
权威状态。设备 callback 驱动硬件并 `report(actual)` 后，Report/State 才更新组件。`invoke` binding
发送 Action COMMAND；`event` binding 消费 Event，不混入 latest Property state。

动态文字/颜色不使用设备组件命令：设备上报 `mode/faultCode/temperature` 等语义 Property，受信的
PageSchema/WidgetRegistry 负责格式化、本地化和 style token 映射。设备不得下发任意 JS/CSS。

## 4. App 协议实施

生产代码应建立独立 `bbp2` 模块，不继续扩展旧 JSON/TLV service：

~~~text
protocol/bbp2/
  frame-codec
  restricted-cbor
  messages
  ble-fragment-codec
  manifest-staging
  state-staging
  session
  limits
device-model/
  manifest-store
  page-schema
  widget-registry
  binding-validator
  state-store
~~~

执行顺序：

1. 接入仓库 exact golden，并让生产 codec 通过全部 valid/invalid cases；
2. 为 Direct BLE 实现 HELLO、限制协商、Manifest 分页 staging、SHA-256 验证、ManifestAccept 与可选的
   session-local ID gate；cloud adapter 只消费服务器已经规范化的 stable-key Manifest/state；
3. 实现 StatePage 的 revision-consistent 原子提交和 key -> widget slot[] 反向索引；不允许逐页或
   多字段 PATCH 逐字段污染 UI；
4. 将 COMMAND、PATCH Apply、PATCH Report、EVENT 分成不同内部类型；
5. BLE 使用 4 B header 重组完整 BBP/2 frame，再进入与 MQTT 相同的 decoder；
6. Direct BLE 独立维护 auth、sequence、replay、ID gate；cloud device session 由 Broker 维护，App 只维护
   account request correlation 和独立 source lane，二者不共享 revision/session 状态；
7. 建立 PageSchema/WidgetRegistry/binding validator，最后替换页面数据源。

App 状态至少分开显示 `localReady`、`cloudReady`、设备业务状态和配网/激活阶段。HTTP 取得凭证、
BLE 广播可见或写入 WiFi 密码都不等于设备已经 cloud online。

## 5. 服务器拓扑与实施

服务器必须终止设备 BBP/2 session，不能把多个 App 的 raw sequence/ACK/ID session 原样透传：

~~~text
Device <-- BBP/2 device session --> Broker canonical field router
                                         |
App    <-- account BBP/2 CBOR session ---+
Direct BLE: App <-- independent BBP/2 session --> Device
~~~

建议模块：

~~~text
protocol/bbp2/       frame/cbor/messages/session/staging/limits
protocol/normalized property-report/property-write/action/event
protocol/legacy/     JSON adapter / TLV adapter
registry/            manifest versions / state / page assets
~~~

服务器阶段：

1. Cloud Session credential 显式绑定 `wire=bbp2,pv=2`；HTTP 200 只代表 `sessionIssued`，MQTT authenticated
   CONNECT 只代表 `transportConnected`，完成 HELLO/auth/Manifest 后才是 `protocolReady`；
2. 新连接第一帧严格识别 HELLO，一旦绑定 wire family 就不得逐包 sniff；
3. 分页完整收齐、重建 canonical Manifest、复算 SHA-256 后事务写入 registry；
4. router 以 Property/Action/Event 语义、Manifest fingerprint 和当前 Field ID gate 工作，不认识 legacy
   TLV fieldId；App/Device/Service 显式目标使用 Route/Delivery，普通 Report/Event 只进入一次 cloud ingest；
5. App COMMAND 与设备 COMMAND 使用不同 sequence，服务器维护有界 correlation，收到设备 ACK/Error
   后才完成 App 请求；
6. latest state 以 `(device, manifestFingerprint, stateRevision)` 原子提交；Event 单独进入事件流；
7. decoder 在分配前执行 frame、CBOR depth/item/text/bytes、分页、速率和 pending request 上限；
8. MQTT payload 一个 PUBLISH 恰好一个 BBP/2 frame，QoS 0、设备下行 retained=false；App 与设备 frame
   header/sequence 分属独立 session，Broker 可复用已验证 inner canonical CBOR body，但不得 raw header relay。
9. 实现 `device-cloud-session/2`：固定 9 项 request map 与 8 项 HMAC transcript；请求不再携带
   `physicalDeviceId`。Gateway 先按 `logicalDeviceId/credentialVersion/generation` 找到唯一 Active
   credential，再从 enrollment registry 解析 physical asset；未发布的 `/1` 不保留双 decoder。
10. 在 product/device registry 保存日常 cloud-session HTTP 与 MQTT transport profile。当前设备默认 Plain，TLS 为
    可选安全档；response key 5 返回 `0 = PlainTcp` 或 `1 = TLS`，必须与目标固件一致。禁止因握手失败
    自动改发另一种模式，并分别统计 Plain/TLS 在线量与失败原因。

建议数据表职责，而非强制数据库形态：

~~~text
broker.device_manifest(device_id, revision, fingerprint, canonical_bytes, fields)
broker.device_runtime_state(device_id, session_epoch, manifest_fingerprint, state_revision, state)
gateway.page_schema(owner_id, device_id, schema_revision, document)
physical_device(physical_id, product_model_id, identity_kind, bootstrap_verifier, status)
logical_device(logical_id, physical_id, owner_id, ownership_generation, status)
~~~

## 6. 身份、配网与密钥

### 6.1 必须拆开的对象

三端统一使用以下名称：

| 对象 | 生命周期 | App | 服务器 | 设备 |
|---|---|---|---|---|
| ManualDeviceKey | 手动 logical device 生命周期、可轮换 | 设备页长期 reveal/copy/refresh | key id + KMS encrypted secret + version | 首次 Claim/全擦 Recovery 来自 Sketch；Active 后忽略 |
| EnrollmentTicket | BLE 配网 5～15 分钟、单次会话 | 只放内存并 exact relay | 签发、校验、幂等锁定 | 只在 Pending transaction 暂存 |
| OwnershipReceipt/Grant | 一个 ownership generation | 缓存公开状态/透明 relay | 签发并保存 exact bytes | Pending 时验 exact bytes；Active 后持久化规范化 OwnershipRecord |
| RecoveryGrant/CommitAck | 一次 N -> N+1 replacement | exact relay/公开状态 | 签发 exact bytes、generation CAS、cutover 审计 | 验签后建立 recovery base；不等于 cloud ready |
| CloudCredential | 长期到轮换/撤销 | 永不读取 | 首版 KMS/HSM 加密 HMAC key + version；future public key | 原子持久化 |
| SessionCredential | 一次 MQTT/WebSocket session | 不需要 | 短期签发 | 只放 RAM |
| ControllerCredential | Platform Direct：每设备 Admin/Shared/Gateway；LocalBLE：物理配对 local controller | OS Keychain/Keystore 保存获授权设备的 secret | 每设备 Shared 可 KMS 密文；Admin 不上传；Gateway 由 Service/KMS 托管；ACL/epoch/version/status | P6 使用 260 B DeviceAccessStore v3（兼容读 176 B v1/196 B v2）；设备只识别权限，不保存 Shared/Gateway role |

### 6.1.1 已冻结的 Controller 日常鉴权对接项

设备侧已冻结 `Controller HMAC-SHA256 method 2`，这不是待讨论草案。App 和服务器必须把
`tests/golden/controller_auth_golden_vectors.properties` 纳入各自生产 CI；仓内 TypeScript/Node 文件只是
独立 reference，不应直接成为业务模块。

App 实施项：

1. 在 OS Keychain/Keystore 原子保存 `controllerId + 32 B secret + domain + generation + version`，数据库只留引用；
2. 默认只在本地 secure channel 已确认 encrypted 后协商 method 2；BLE Direct 的冻结例外是在普通 GATT 上
   运行 pre-auth Method 2，并在 Authorized 后原子切换 DirectSecureRecord。每个 session 生成新的 16 B CSPRNG
   clientNonce，App 不以系统 bond/MAC 代替该过程；
3. 严格编码 42 B Init，验证 53 B Challenge 的设备 proof，再发送 33 B App proof；失败、断连或完成后清除 nonce/临时 secret；
4. 以 Challenge 返回、且已经被 device proof 覆盖的 permissions 更新 UI 能力；`Observe` 只读，`Control` 才允许写入；
5. BLE 与 cloud 分别维护 auth、sequence、Field ID map，不因同一 logicalDeviceId 共享 session 状态。

服务器实施项：

1. Controller registry 保存 secret digest/verifier、permissions、ownership generation、credentialVersion 和状态，禁止保存/记录 raw secret；
2. ControllerGrant/receipt/commit 必须按已冻结 exact contract 绑定上述字段，并在 rotate/revoke 时推进 version 或撤销记录；
3. Gateway 不参与日常 method 2 challenge-response，但必须使 revoke/permission change 能可靠同步到设备 BCT2；
4. 审计事件只记录 controllerId、版本、权限变化和结果，不记录 payload、proof、Ticket 或 secret。

精确日常 payload、HMAC role domain、字段顺序和 Runtime 权限表见
`DEVICE_V2_CONTROLLER_AUTH_METHOD.md`。

### 6.1.2 已冻结的 Controller 安装、轮换与撤销对接项

设备侧已冻结 Ownership-domain `ControllerGrant / ControllerMutationReceipt /
ControllerCommitRequest` v1，权威合同和 key 表见 `DEVICE_V2_CONTROLLER_CONTROL_CONTRACT.md`，三端必须
把 `tests/golden/controller_control_golden_vectors.properties` 纳入生产 CI。

App 实施项：

1. 每次控制事务读取设备生成的 16 B controlNonce；App 不生成、不替换该 nonce；
2. Install/Rotate 在 Keychain/Keystore 生成并暂存新 secret，只向服务器发送 digest；raw secret 只通过
   已加密本地 record 交给目标设备；
3. 验证设备 receipt HMAC 后，必须以新凭据完成 Method 2，再提交 `Method2Confirmed` Commit；
4. Commit 失败时持久保存 `grantId + receipt + idempotencyKey` 并重试，不要求设备回滚已经 durable 的凭据；
5. Revoke 没有目标 secret proof，UI 必须显示其保证来自当前管理会话/云端授权和 server-signed Grant，
   不能显示为“设备已用被撤销密钥签名”。

服务器实施项：

1. registry 保存 digest，不保存 raw secret；Grant 绑定当前 DeviceInstanceId、logicalDeviceId、generation、
   version CAS、permissions、device controlNonce 和短 TTL；
2. 事务至少区分 `Issued / DeviceApplied / Committed / Cancelled / Superseded`。短 issue TTL 到期只禁止新安装
   尝试，不能把已签发且可能已 durable 安装到设备的 grant 直接变成不可提交终态；同一 idempotency key +
   相同 body 返回相同结果，相同 key + 不同 body 返回冲突；
3. server-signed Grant 已授权设备变更。Commit 是 registry/审计收敛，不是设备侧激活条件；
4. 服务器无法验证 HMAC receipt，因为没有 raw secret。`Method2Confirmed` 是已认证 App 在本地完成 receipt
   HMAC + Method 2 后的声明；需要更高保证的量产档应另加设备签名，不得假装 digest 可验证 HMAC；
5. Revoke 只对拥有 `ManageControllers` 或等价云端权限的主体签发，并使用 `AuthorizedRevoke` Commit；
6. 所有日志只保留 grantId/controllerId/version/permissions/result，禁止记录 Grant raw body、receipt proof、
   secret 或临时安全通道内容。

Ownership Claim、Cloud Credential Enrollment、Ownership Reset/Transfer、ManualDeviceKey/Ticket outer 和
LocalBLE 本地安装/application exact contract 均已冻结。LocalBLE 使用独立
`DEVICE_V2_LOCAL_CONTROLLER_PAIRING_CONTRACT.md`：物理确认绑定当前 Noise transcript，安装 Local domain /
generation 0 credential，不调用服务器 Grant。R4b-6B 已接入真实共享 Noise endpoint，R4b-6C 已完成双平台
LocalBLE internal product graph；App 仍须实现生产
coordinator/E2E，Gateway 不需要也不得接收该 application body。不得用旧 authKey、method 1 或旧
Enrollment/1 临时填补。

V2 Service Data 中平台 `EnrollmentV1` 与 `LocalControllerPairingV1` 是互斥 application capability：App
扫描即分流，不能仅按 `Provisioning` mode 猜测。Local 普通 NN/NNpsk0 固定为 `0x0043/0x0061`；Local 流程
不得请求 Ticket，平台流程不得把 Local generation 0 当 Ownership credential。两类流程只共享 BLE record、
Noise suite 与后续 Direct BBP/2 数据面。

不再存在一个覆盖所有 profile 的 `DeveloperSetupCode`：

- WiFi-only 手动 DIY 没有 App 本地通道，使用长期可查看/刷新的 `ManualDeviceKey`；
- WiFi+BLE 由登录 App 针对当前 physicalDeviceId/setupSessionId 申请短期 `EnrollmentTicket`；
- BLE-only 平台设备由 App relay Ticket/OwnershipGrant，不强制预置 SetupCode；
- LocalBLE 不进入服务器控制面，只安装本地 ControllerCredential；
- Manufactured 使用每台唯一 DeviceBootstrapCredential，可另加 QR claim/setup code。

`physicalDeviceId` 是公开物理实例 ID，`logicalDeviceId` 是服务器所有权/页面主键。任何 Enrollment 或
credential secret 都不得直接或截断后作为 ID；BLE MAC/广播名只用于当前扫描发现。

### 6.2 App 配网与重启生命周期

WiFi+BLE 首次配网：

~~~text
BLE discovery + physical setup window
-> App reads physicalDeviceId/setupSessionId/capabilities
-> App JWT requests short-lived EnrollmentTicket
-> protected BLE session writes exact Ticket + WiFi candidate
-> device/Gateway complete Ownership Claim
-> device atomically installs CloudCredential
-> App/device install per-controller ControllerCredential
-> device obtains short SessionCredential and connects MQTT
~~~

EnrollmentTicket 只用于第一次 Claim。设备 Active 后清除 Ticket，并分别持久化
`OwnershipRecord + CloudCredential`。OwnershipRecord 是
`state/generation/logicalDeviceId/SHA-256(exact signed grant bytes)` 的规范化结果；服务器仍保存 exact
grant/receipt，不能把 112 B `BOS2` 当成 API body。一个月后重启时，设备读取这些 Active 数据，
重新申请短期 SessionCredential 并上线；Ticket 已过期或已消费完全不影响正常登录。若实现仍要求重启时
重新提交 Ticket，就是把 enrollment 与 runtime login 混在一起，必须拒绝合并。

同样要区分三种恢复：Network Reset 只清 WiFi，使用当前 owner/controller 获得短期
NetworkProvisioningGrant；Ownership Reset 才由当前 Active 实例前进 generation 并结束/转让 ownership；
全片擦除可能使 DIY identity 丢失，此时不是重新执行首次 Claim，而是由同一 ManualDeviceKey 授权独立
recovery intent，允许新 instance 在 N+1 接管同一 logical device。服务器不能靠 MAC
自动找回；Key 只是外围授权，仍必须签发独立 RecoveryGrant。
Ownership Reset/Transfer 成功后设备必须留下 next-generation Unclaimed tombstone；服务器重发旧
receipt/controller 时应因 generation 低于该 floor 被拒绝。

### 6.3 Gateway 控制面与事务

> **2026-08-30 Edge Hub discovery 合同修正：**已有 `GatewaySecretEnvelopeV1` 只下发 Method 2 secret，无法让
> Hub 按 Direct v3 PresenceKey 定位 exact child。真实 GATT Central E2E 前，App/Service 必须实现
> `GatewayAccessEnvelopeV2`：owner App 只提交 exact child/Hub attach intent；Service 从 KMS 内部解封
> current/confirmed PresenceKey 并校验 accessEpoch/version，再把 `Gateway secret[32] + PresenceKey[16]` 密封给
> exact Hub；raw material 只允许存在于有界内存/KMS worker，不写普通 DB/日志。ACK 对 exact 48 B access material
> 取 digest。PresenceKey
> 独立轮换使用 active+pending 两阶段提交；App/Service 不得让 Hub 持久依赖 BLE MAC/名称，也不得让 Hub 扫描后
> 对所有附近设备盲试 Method 2。 

> **2026-08-31 G2A-2e 设备侧边界：**内部 ESP32 Edge Hub 已具备 fixed-queue NimBLE Central、exact
> active+pending Presence matcher，以及 Hello → Method 2 → DirectSecure 的单 child composition；只有
> MQTTS management 已 Online 才启动 Central，cloud admission 丢失会退役 child/关闭 Central。该能力没有
> 自动扫描，也没有把 durable access ACK 当作 topology Active。Service 下一步必须先冻结并实现 G2A-2f
> proof command/result：绑定 operationId、exact child/Hub、topologyVersion、accessEpoch、Gateway credential/
> Presence version、fresh nonce、expiry 与幂等结果；只有 access durable ACK、child install receipt 和 Hub
> secure proof 三项齐全才 CAS Active。随后 G2A-2g 才能下发/接收 `ChildRouteEnvelope`。App 不解析 Edge Hub
> management codec，也不以设备凭据登录 MQTT；它只驱动 attach/resume/cancel 状态和展示 Service 权威结果。

Gateway 需要独立 endpoint/transaction family：

1. ManualDeviceKey create/reveal/rotate/proof：用于手动 WiFi-only；先预建 AwaitingFirstClaim logical device/
   PageSchema，不创建 Active relationship/authKey；
2. EnrollmentTicket issue：要求 App JWT，绑定 owner、physicalDeviceId、setupSessionId、16 B
   controllerId、`SHA-256(controller secret)`、设备权限位、TTL 和 nonce；Gateway 不保存 Direct BLE raw
   secret；
3. Ownership Claim start/proof/receipt/commit：手动模式激活预创建 logicalDeviceId 的 physical relationship；
   自动 profile 在受控 intent 中创建并激活，不得由设备自报 ID；
4. CloudCredential enroll/commit：只接受 Active ownership/current generation；设备 secret 只出现在发给
   pinned Gateway X25519 key 的 AES-GCM sealed envelope，Gateway 以 signed Grant、HMAC Receipt/CommitAck
   完成四步事务；
5. SessionCredential issue：只验证 Active CloudCredential，不接收 ManualDeviceKey/Ticket/physicalDeviceId；
6. Controller enroll/revoke、NetworkProvisioningGrant、OwnershipReset/TransferGrant；
7. ManualDeviceKey/App recovery intent、signed RecoveryGrant/CommitAck 与 Claim cutover；
8. exact idempotency、generation CAS、attempt limit、审计和 secret/body 脱敏。

factory identity 只参与 enrollment、claim proof 和 ownership reset；日常 cloud session 从
`logicalDeviceId/credentialVersion/generation` 找到唯一 Active credential，再解析 physical asset。服务器
不得因为 cloud-session 请求省略 physicalDeviceId 就新建或猜测资产关系。

Ticket/Key 在 claim 被接受后要锁定同一 transaction：相同 request/physical ID 的丢包重试返回同一
   exact grant；同 key/ticket 换 physical ID 必须冲突。设备持久 grant fingerprint 统一为
   exact signed grant bytes 的 SHA-256，不允许对解析后 map/JSON 重新编码再 hash。CloudCredential 在发送 enroll 前由设备 stage，响应丢失时
以同一 request/credential 重试，不能重新生成第二把 credential 或第二台 logical device。

首版 CloudCredential 使用 HMAC。Gateway 不能只保存普通 password hash 后声称能验证 challenge HMAC；
必须保存 KMS/HSM 加密的等价 key。注册请求已使用 X25519/AES-GCM sealed envelope，服务器 encryption
private key 同样必须置于 KMS/HSM/等价受控域。只有 future public-key suite 才能做到服务器只存公开
verifier。数据模型和安全评审必须准确写出这一差别。

制造期 pre-enrollment 增加一条独立生产路径：服务器/受控 batch job 为每台 physical asset 创建唯一
Active CloudCredential，先事务落库，再一次性导出平台无关的 `cloudCredential` 对象
（`logicalDeviceId/generation/credentialVersion/secret`）给工厂系统。Active record 不包含
enrollment-only `claimId/activationKey`。工位再按 `platformProfile` 生成 ESP32 NVS image，或通过受控
USB/串口 installer 写入 UNO R4 connectivity-module Preferences；服务器不得把 NVS 布局当成设备协议。

当前设备侧已冻结 `schemaVersion: 1 + securityProfile: education-plain` 输入，以及固定 228 B 的 `BFB2`
工位记录。BFB2 不包含 platform/storage 字段；工位根据服务器制造任务中的 `platformProfile` 选择
ESP32 NVS executor 或 UNO R4 USB Serial executor。服务器不得直接生成 BFB2/NVS key，也不得要求 App
转发制造 secret。116 B `BCS2` 是设备持久化格式，不是服务器 API 或 App wire。完整字段和 receipt 见
`DEVICE_V2_FACTORY_CREDENTIAL_BUNDLE_AND_UNO_R4_INSTALLER.md`。

服务器需接收 `platformProfile/artifactKind/artifactSha256/toolVersion`、工位写入结果和首次 cloud
authentication 结果，并明确区分 issued/installed/verified/revoked。失败 artifact/bundle 的 secret 不得
分配给另一台设备；工位日志和 receipt 必须脱敏。App 不参与制造 secret 的生成、传输或保存。

BLE-only 设备仍需要 physical identity 和 ownership claim，但不需要 CloudCredential/MQTT。若自身不能
访问服务器，App 是 exact-byte relay；设备必须验证 server-signed OwnershipGrant 并安装当前 App 独立的
ControllerCredential。无 factory root 的 Education 档只提供物理在场/当前实例级保证，App UI 必须诚实
显示；更高保证由 factory proof 或 QR secret 提供，不能通过强制所有 BLE 设备使用 SetupCode 假装获得。

ESP32 WiFiProv 只能作为底层 provisioning transport 的候选。其高层 API 不能完整表达当前
Security2 salt/verifier、自定义 secure endpoint、durable claim handoff 和完整模式生命周期，因此
不得让 Core 或三端合同依赖 WiFiProv。平台适配器可在确实满足合同后内部使用它。

## 7. 旧设备与迁移

### 6.1 Ownership Claim v1 exact handoff（2026-08-16）

设备侧已冻结 `DEVICE_V2_OWNERSHIP_CLAIM_CONTRACT.md` 和共享 golden。App/Gateway 必须按以下边界实现：

- exact request 只含 `requestId/deviceInstanceId/ownershipGeneration/claimNonce`；ManualDeviceKey proof、EnrollmentTicket、用户 JWT、factory proof 在 profile-specific authenticated envelope 中传递和校验，不得增塞进 canonical body；
- product/model policy 从 ManualDeviceKey/Ticket intent 或 factory registry 取得，不能要求设备上报 productId，也不能把 Manifest fingerprint 绑定成永久 ownership identity；
- Gateway 必须按 `(deviceInstanceId,generation,requestId)` 事务保存首次 exact grant bytes，重试不能更新时间戳、换 logicalDeviceId 或重签另一份 grant；
- App 只能 relay exact grant bytes，不得 decode 后重建；设备 receipt 没有独立 bearer proof，Gateway 只可在已认证 device HTTP 或 App JWT + 当前 setup session 上下文中接受；
- Gateway 收到 exact receipt/commit 才把 server transaction 标 Active，并返回签名绑定 exact receipt digest 的
  `OwnershipClaimCommitAck`；设备虽在 verified Grant 落库时本地已 Active，但只有验该 Ack 才清 Claim
  journal。空 204/普通 2xx 不可替代，Ack 丢失必须幂等返回；
- CloudCredential 与 ControllerCredential 均不由 Ownership Claim 创建。WiFi-capable 在下一独立 Cloud enrollment 继续；BLE-only 可直接进入 Controller enrollment。

共享现为 11 个 C++/Node exact 值与 7 个 App wire 值，已包含 CommitAck transcript/body；生产 decoder 仍需补 unknown/duplicate/out-of-order、错误长度、错误 version、nonce/generation/digest mismatch、并发 claim 和认证上下文丢失用例。

### 6.2 Cloud Credential Enrollment v1 exact handoff（2026-08-16）

设备侧已冻结 `DEVICE_V2_CLOUD_CREDENTIAL_ENROLLMENT_CONTRACT.md`：

- common request 为 12 对 canonical CBOR，最大 228 B；只含 request/device/logical ID、generation/version、
  suite、envelope algorithm/keyId、ephemeral public、nonce 和 48 B encrypted credential；
- envelope 固定 `X25519 + HKDF-SHA256 + AES-256-GCM`，AAD 为 exact envelope transcript，nonce 为其
  SHA-256 前 12 B；App 只 relay，不能读取 secret；
- Gateway 以 `(logicalDeviceId,generation,requestId)` 和 exact request digest 幂等建账，返回最大 248 B
  server-signed Grant；设备先 durable `grantDigest`、再 install Active CloudCredential；
- 最大 190 B Receipt 是新 secret HMAC proof；Gateway 验证后才 Active。最大 190 B CommitAck 同样由新
  secret HMAC，并绑定 exact receipt digest，不能以 Plain HTTP 2xx 替代；
- 设备 `BCE2` journal 固定 136 B，与 Active `BCS2` 116 B 分离。App/Gateway 不得接收或导入这些本地
  blob，只认 exact wire；
- Gateway encryption private key 与 Active HMAC key 使用 KMS/HSM/等价 envelope encryption。日志只留
  keyId/requestId/digest/state，不留 request ciphertext 之外的解密材料、proof 或 secret；
- 共享向量新增 C++ 15、App 5、Node 19 个 exact 值；Node 参考实现使用真实 X25519/AES-GCM。App/服务器
  生产实现仍未落地，不得把 reference golden 当成端点已完成。

- 旧 JSON 设备继续走明确的 legacy connection/topic/service；新固件不携带 JSON/TLV codec。
- 当前未发布 TLV/WidgetCatalog 实验无需兼容，不建立双栈探测。
- JSON/TLV/BBP2 在 normalized router 前收敛；业务、页面与历史层不得复制三套逻辑。
- 完全停止旧 JSON 的日期应由在线设备量、强制升级能力和支持成本决定，不写死在设备协议。
- 切换时按产品/账号灰度，保留可观测回退；不能在同一 MQTT/BLE session 内混发两种 wire family。

### 7.3 DeviceKey + Device Recovery handoff（2026-08-19 修订）

权威合同为 `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 与 `DEVICE_V2_DEVICE_RECOVERY_CONTRACT.md`。普通 Education/
DIY 只保留一个 32 B DeviceKey：外部是 43 字符无前缀 Base64URL；`ManualDeviceKey/bdk_`、EnrollmentKey 和
独立 RecoveryKey 均已撤回。

- App 设备页允许长期 reveal/copy/rotate；Gateway 用 KMS/HSM/等价主密钥加密 secret，普通 DB/API/log
  只显示 masked value；
- 设备发送派生 locator 与 exact transcript HMAC proof，不发送 raw Key；手动 Sketch、BLE 配网和工厂烧录
  只是安装同一种 Key 的不同方式；
- rotate 返回新 Key 前必须完成 Broker 强吊销；旧 Key、旧 challenge、旧 SessionCredential 和在线连接立即
  失效。BLE 重新配网刷新采用 pending/commit，提交成功即切换，不存在长期 grace；
- 全擦后当前 DeviceKey 通过 Recovery 取回同一 logical ID，不要求用户理解第二个 RecoveryKey；
- 旧实例仍 ProtocolReady 时要求 App replacement confirmation；离线时 Education policy 可自动恢复并产生
  owner 安全事件；
- Recovery Ack 只建立 N+1 base；Claim commit 才 CAS 切换 relationship并撤销旧 Cloud/Controller/session；
- PageSchema/history/ACL/automation保留，不兼容 binding 标记 needs-review；
- production codec/golden/KMS reveal-rotate/confirmation/cutover E2E 完成前，R5-1/R5-2 不得绕过该缺口。

EnrollmentTicket 不复用该 Key 或 header。R4b-4a signed body 继续绑定 setupSessionId、完整
setupTranscriptHash、generation、controller digest/权限和 operation mask。

### 7.4 EnrollmentTicket v1 exact handoff（2026-08-16）

设备侧已冻结最大 268 B 的 16-pair canonical CBOR Ticket 和最大 216 B 的签名 transcript。App/Gateway
必须遵守：

- App 在当前 provisional handshake 完成后取得 `setupTranscriptHash`，生成独立 controllerId/32 B secret，
  只把 secret digest、权限和当前 setup context 交给 Gateway；raw secret 只留 Keystore/受保护内存；
- Gateway 从 JWT 绑定 owner/product intent，但 signed device body 不包含账号文本或稳定 owner digest；数据库
  通过 ticketId 保留该关联，避免设备 wire 泄漏跨设备可关联标识；
- BLE-only `authorizedOperations=0`；WiFi+BLE 首次配网为 CloudEnrollment bit0 +
  NetworkProvisioning bit1。未知 bit 必须拒绝，不能静默扩大授权；
- Ticket 生命周期硬上限 900 s。无可信 RTC 的 MCU 可跳过本地 epoch 判断，但 Gateway issue/redeem 必须
  强制 TTL、单次消费、attempt limit 和 `(ticketId,deviceInstanceId,generation,claimRequestId)` 幂等；
- App exact relay signed bytes，不 decode/re-encode；设备 exact-match device/setup/transcript/generation/
  controller/permissions/operations 后才验签并升级 session；任何失败都不得写 WiFi 或提交 Claim；
- Ticket 不进入 OwnershipRecord、CloudCredential、ControllerCredential、日志或 analytics。Active 后清除；
  正常重启不再依赖它；
- 当前 C++/App-compatible TypeScript/Node 为 3/2/3 项 exact golden。它们是 reference codec，不代表生产
  endpoint 已完成。

R4b-4b 已冻结两档互斥 secure suite：普通 DIY 为 suite 1
`Noise_NN_25519_AESGCM_SHA256`，无需 SetupCode；Enhanced/Manufactured 为 suite 2
`Noise_NNpsk0_25519_AESGCM_SHA256`，只接受 32 B 高熵 PSK。App 从 Service Data capability bit1/bit5
选择固定实现，禁止连接内协商或降级；两位同时存在/均不存在必须拒绝。普通 NN 首包为
`e[32] || clear payload`，第二包才有 16 B tag；App 不得继续按旧“两包都至少48 B”解析。

Service Data 尾部 8 B 已更名为 `setupSessionLocator`，不是 Ticket 的 16 B `setupSessionId`。App 用 locator
寻找当前广播，随后从受保护 HelloResponse 获得完整 ID，并把完整 ID 与 handshake hash 交给 Gateway。
禁止补零/复制 locator 冒充完整 ID。在设备验证 Ticket 并返回受保护 accept 前，App 不得发送 WiFi
credential 或认定设备身份已认证。

### 7.5 LocalSetup application + durable saga v1 exact handoff（2026-08-17）

设备侧已冻结 7 类 strict canonical CBOR envelope、8 个 operation、非零 uint32 requestId、exact replay、
错误/retry class 与 248 B BSG2 durable saga。
权威 map、尺寸和流程见 `DEVICE_V2_LOCAL_SETUP_APPLICATION_CONTRACT.md`，共享 fixture 为
`tests/golden/local_setup_golden_vectors.properties`。App/Gateway 必须遵守：

- HelloResponse 字段 7 是设备要求签入 Ticket 的 `ticketOperations`；只能是 BLE-only `0` 或 WiFi+BLE
  `3`，不是服务器可任意裁剪的已授权权限；
- ordinary NN 的 HelloRequest 在 initiator handshake clear payload 中且不含身份/secret；HelloResponse
  在 responder handshake 受保护 payload 中。Ticket 必须绑定 responder message 完成后的 final hash；
- App request 严格串行，重试复用 exact bytes/requestId；同 requestId 不同 bytes 必须终止 session；
- operation 1..3 复用 Ownership Claim exact body。operation 4 是专用 InitialControllerInstall
  `{0:1,1:controllerSecret(32 B)}`；Ticket 已签名绑定 controllerId/secret digest/permissions，不能再要求
  一份 ControllerGrant；raw secret 只经 Noise 发给设备；
- operation 5 ConfigureNetwork 固定为
  `{0:1,1:ssid(bstr),2:authentication,3:credential(bstr),4:hidden(bool)}`，响应为
  `{0:1,1:networkGeneration,2:CandidateStored(1)}`。CandidateStored 不是 AP connected/MQTT online；
- operation 6/7/8 固定为 App exact relay Cloud Enrollment：empty → exact sealed Request，exact signed Grant →
  exact HMAC Receipt，exact HMAC CommitAck → empty。不得使用 JSON、legacy WiFiConfig、已撤回的 `bek_` header、空 2xx
  或由 App/Gateway decode 后重建 body；
- 服务器以 ticketId + physical instance + generation 建立 durable transaction。同 session exact Ticket retry
  返回原 bytes；断连/过期后的新 transcript 可重签，但必须复用同一 ticketId、controller context、generation
  与 operation profile。不能因 Ownership 已 Active 就新建 logical device；
- 未确认 WiFi candidate 可以在 Cloud 开始前替换；Cloud enrollment 已完成但首次 AP 失败时，也允许在同一
  transaction 下重新进入 secure setup 替换 candidate。服务器/App 只有在设备 network confirm/online 后才
  将事务标为最终完成，不能把 operation 5 response 当成联网成功；
- Gateway 必须持久化 exact Cloud request/grant/receipt/ack 或其规范要求的 exact bytes + digest/state，确保
  `ClaimCommitVerified`/`CloudCommitVerified` 恢复不会发出第二份 Grant 或清错 transaction。

## 8. 三端退出门禁

App/服务器进入可联调状态必须同时满足：

- 生产 codec（不是 reference script）通过 BBP/2 29 exact / 6 invalid 以及所有 onboarding/controller/cloud/
  release/recovery/local shared golden；
- Manifest/State 分页的缺页、乱序、revision/fingerprint 冲突不会污染 cache/UI；
- Property write、Action invoke、Event 与 authoritative report 端到端语义正确；
- Direct BLE 与 cloud 同时在线不会共享 session-local sequence/ID/auth；
- App 在 Provisioning 最终响应后断开并按同一 physicalDeviceId 重扫 Direct mode；若最终响应丢失，以 Gateway
  durable transaction + 广播 mode 收敛，不创建第二台 logical device；
- App 启动新 Provisioning 扫描前必须先检查本机 pending ControllerCredential。若设备已进入 Direct，调用
  `resumeEnrollment()` 完成 Method 2、commit 与同步；只有用户明确取消、服务器确认 intent 不可恢复或设备已
  重新进入新的 setup generation 时才清理 pending。注册页的“重试”不得永远只扫 Provisioning，否则模式切换后
  一次扫描超时就会把已安装 access root 的设备留在无 UI 恢复入口的状态；
- Service 对已经签发 exact grant 的 intent 保留可恢复 commit 记录：App 能以 pending credential 完成 Method 2、
  提交匹配 receipt/commitId 时，即使短 issue TTL 已过也应在有界审计保留期内幂等收敛为 Active，除非 intent
  已明确 Cancelled/Superseded 或 ownership generation 已推进。设备 durable 安装后再返回单纯
  `Enrollment intent expired` 会制造无法由 App/设备自行修复的孤儿 access root，禁止作为正式行为；
- BLE 分片能力不代表认证。Direct BLE 必须执行 Controller Method 2，并在 Authorized 后只发送/接收
  DirectSecureRecord；App 不要求通用 `createBond()`，也不接受设备/adapter 在 AEAD 验证前伪造
  authenticated/encrypted。平台可额外要求 native encrypted GATT，但这是声明并验证过的纵深能力；
- 每次 BLE connect 都是新逻辑 session，即使底层库或设备再次给出相同数值 sessionId。App 在断连、加密丢失
  或 mode 切换时必须清除分片、sequence、Method 2、Field ID map 和 pending command；重新连接后从
  HELLO/Manifest/Auth 开始，禁止继承旧 session 状态。该规则只影响 App 本地连接状态，不要求服务器创建新
  logicalDevice，也不改变 cloud session；
- 服务器断开旧 MQTT session、credential generation 轮换、ownership reset 和 Recovery N -> N+1 cutover 有集成测试；
- Gateway 的 `device-cloud-session/2` 通过共享 `expected.cloud_session_*` golden，严格拒绝 v1、
  错误 version、缺字段、额外字段与 HMAC transcript 不一致；
- App/Server 生产实现通过共享 Controller method 2 的 Init/transcript/proof/Challenge golden，App 按
  `Observe/Control` 禁用越权入口，rotate/revoke 后旧 proof 不可继续建立会话；
- legacy JSON 与 BBP/2 有独立指标、限流、日志和回滚开关；
- PageSchema 能由拖拽和 Agent 同构生成，Manifest 更新只标记失效 binding，不删除用户页面。

设备正式发布还额外要求 ESP32/UNO R4/Wio Terminal 编译门禁、目标真机 WiFi/BLE、断线、峰值 SRAM
和长稳测试通过。当前 reference device core 完成不代表生产三端已完成。

## 9. DKEY-2D App/Service 延后实施输入（2026-08-18）

本节覆盖本文中普通 ESP32 Education profile 的旧 Ticket exact-relay/Ownership/CloudCredential 流程；不改变
Platform BLE、LocalBLE 或 Enhanced Noise 的独立合同。

### 9.1 App coordinator

1. 用户显式进入首次配网或“重新配置设备”；后者在 UI 中说明成功后旧 Key 和旧连接都会失效。
2. App 连接 WiFiProv Security1。已有设备使用当前 DeviceKey 派生 PoP；首次/全擦设备使用 Sketch、Serial、
   标签或显示屏提供的 setup PoP。不能以空 PoP 显示“设备已验证”。
3. App 先调用 `blinker-config.GetInfo`：请求 `[1,1]`；成功响应
   `[1,1,0,hasDeviceKey,deviceInstanceId(16)]`。
4. App 以账号登录态、logicalDeviceId、deviceInstanceId 和幂等键向 Service 创建 provisioning intent；Service
   返回 intentId、pending credentialVersion 和只在受保护 App 内存中短暂存在的新 43 字符 DeviceKey。
5. App 调用 `Install`：`[1,2,DeviceKeyText(43)]`。只有 3 B status=Success 后才发送标准 WiFiProv
   network-config；相同请求可重试，不得在同一 setup session 换另一个 Key。
6. App 不能把 install 2xx、WiFi connected 或 provisioning end 当作 cloud ready；必须等待 Service/Broker
   报告 pending 新 Key 已 proof/activate 且设备新 session online。

### 9.2 Service/Broker 事务

1. 创建 intent 时生成随机 N+1 DeviceKey/KMS ciphertext/locator，状态为 `AwaitingProof`；当前 N 仍 Active。
2. 5～15 分钟 intent TTL 只限制 App 创建/领取/下发操作。DeviceKey 一旦签发为 pending，其
   `AwaitingProof` 记录不能因 Ticket 到期而消失，否则设备断电一个月后将无法上线；它只能由 owner 取消、
   新事务显式 supersede 或 proof 成功提交。
3. 普通 challenge route 同时按 active locator 和唯一 pending locator 查找。pending N+1 proof 成功时，以单一
   数据库事务激活 N+1、撤销 N、推进 credentialVersion，并使全部 N SessionCredential/JTI 和缓存失效。
4. 提交后 Broker/Gateway 必须主动断开 N 的 MQTT/WebSocket 连接；仅禁止下次 CONNECT 不合格。
5. App 设备详情的独立“刷新 Key”仍是立即强吊销，不创建 continuity pending。它与 BLE 再配网的掉电安全
   prepare/commit 必须是两个明确操作和两类审计事件。
6. raw DeviceKey 不进入日志、analytics、push、URL 或 ordinary BBP/2；Service 只通过受保护 App response
   返回，设备只在 Security1 custom endpoint 内接收并原子保存。

建议最小状态表：`device_key_version(logical_id, version, locator, kms_ciphertext, state,
expected_instance_id, intent_id, created_at, activated_at, revoked_at)`；同一 logical device 至多一个 Active 和一个
AwaitingProof，避免多个并行候选导致设备端无法判断。Legacy JSON/authKey 的连接、route 与 broker namespace 不再进入
V2；既有页面布局只能迁移为 V2 PageSchema 消费者，不能保留旧数据链。

## 10. Controller Control v2 生产输入（2026-08-18）

设备 Core 已把后续 controller 管理接入 BBP/2 和正式 P6 device composition。App/Service 实施不得继续使用
未发布的 v1 Grant：

- HELLO feature 使用 bit 9；消息为 Open `0x30`、Challenge `0x31`、Mutation `0x32`、Receipt `0x33`；
- Open/Mutation 只允许已加密 Local session，并且必须先以 Method 2 取得 `ManageControllers`；
- v2 Grant 是固定 14 项 canonical array，删除 logicalDeviceId、domain、suite、issuedAt、expiresAt；
  logicalDeviceId/TTL/单次事务由 Service 管理，设备绑定 instance + accessEpoch + fresh nonce；wire
  `ownershipGeneration` 暂保留编码名，但不得再解释为设备 OwnershipRecord；
- 最大 Grant/Mutation/完整 frame 为 `193/231/241 B`。App BLE 层复用已有 4 B fragment/record，不能新增
  controller 专用分片；
- 设备只在本地接收上限至少 `241 B` 时发布该能力；App HELLO 必须声明至少 `155 B` 接收上限以容纳最大
  Receipt frame，低于此值时不得显示或调用 Controller 管理；
- Receipt/Commit map 结构不变但 version/domain 更新为 v2；共享 exact fixture 已更新，v1 无兼容义务；
- App 对同一 Open/session 缓存 nonce；response 丢失可重发相同 Open。Mutation 成功但响应丢失时必须重发
  完全相同的 Grant/secret，不得生成新 grantId 或版本；
- Service 在签 Grant 前检查 owner、ManageControllers、当前 generation/version、短 TTL、单次/幂等事务；
  设备不依赖 RTC。Install/Rotate 只有 App 验 HMAC receipt 并用新 secret 完成 Method 2 后，才提交
  `Method2Confirmed`；Revoke 使用 `AuthorizedRevoke`；
- First controller 不走上述 Runtime 消息。P6 已冻结 `blinker-config` v2 Bootstrap：101 B request 为
  `version, operation, DeviceKey[43], accessEpoch[u32be], adminGroupId[16], credentialVersion[u32be],
  adminSecret[32]`，Admin 权限由设备固定为 All；设备通过单个 176 B atomic store 一次提交。GetInfo 返回
  public instance、flags 和 accessEpoch。禁止开放匿名 ControllerControl 作为捷径。

Service 仍保存 logical device 与 physical instance/accessEpoch 的权威映射；设备不需要为了云端路由常驻
logicalDeviceId/owner。App/Service 尚未实现生产 codec/事务，因此 UI 仍不得提前展示“添加/轮换 Controller”。

## 11. Edge Gateway proof/activate 实施输入（2026-08-31）

1. App 仍只负责 owner 明确选择 Hub、在既有 Admin DirectSecure 会话中 exact relay Gateway grant/secret，并提交
   已验证的 durable child receipt；App 不解析 Hub MQTT management command/result。
2. Service 必须先以 receipt digest 将 topology 从 `PendingChildInstall` CAS 到 `PendingGatewayProof`。未验证
   receipt 的 HTTP body、access delivery ACK 或“扫描到附近设备”均不得触发 proof/Active。
3. Service 为 exact topologyVersion 生成 16 B proofNonce 与短 TTL，持久化后才向 Broker stage exact
   `GatewayProofCommandV1`；deadline 必须先规范化为 wire 可精确表达的整 Unix 秒，重复 create/resume 返回
   同一 command 身份。
4. Broker 只向 authenticated exact Hub MQTTS session 投递，QoS1/retain=false，并持久化 exact command；上行
   只接受相同 operation/topology/nonce 的 result。Secure late result、不同 result replay 和错误 Hub 必须拒绝。
5. Service 仅对 Secure + exact Presence version 执行 `PendingGatewayProof -> Active` CAS；Rejected 进入
   RollbackRequired，Expired 清 nonce 并推进 topologyVersion 后才能显式重试。
6. `GatewayAccessEnvelopeV2.expiresAtUnixSeconds` 是长期 Gateway credential expiry。MQTT access delivery 和 proof
   command 各有独立短 deadline；不得再用 5～15 分钟投递 TTL 让已经安装的 Gateway credential 很快失效。
7. G2A-1a 已完成 Gateway production runtime wiring，以及 owner create/read/resume/receipt/cancel API。Service
   从 owner DirectAccess/Presence 权威记录取得 child/accessEpoch，稳定派生 controllerId，并只在 Hub durable ACK 后
   临时解封和 relay raw Gateway secret；App 不能提交这些权威字段。receipt 必须同时通过 exact CBOR 字段、secret
   digest、HMAC 和 `method2Confirmed`，随后才进入 `PendingGatewayProof`。下一工作批只做 G2A-1b 最小 App exact
   relay、App-kill resume 与真实 ESP32 proof E2E；通过前不得进入 child route bridge。
