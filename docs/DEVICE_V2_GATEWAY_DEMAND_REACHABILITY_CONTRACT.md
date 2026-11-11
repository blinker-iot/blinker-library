# Gateway 连接需求、可达租约与 Direct 交替合同

2026-09-09，当前实现为GATT §10.121 / LAN§21的新鲜进展回执与后续投递耗尽恢复，软件/隔离PG已验证，真机控制闭环待做；进展Status不是业务授权或设备在线状态。执行顺序仍按[LAN计划§19/21](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)，历史需求决策见§6/7及[共享连接实施方案](DEVICE_V2_GATEWAY_SHARED_CONNECTION_IMPLEMENTATION_PLAN.md)。
原单child交替100轮暂停，先转共享连接/有界多child；Mesh、History/Bulk不在本次范围。下文早期批次只作历史证据。

## 1. 五种不同事实

| 事实 | 所有者 / 证据 | 不能据此推断 |
|---|---|---|
| 有权代理 | Service Active topology、epoch/version、GatewayAccess | 物理连接存在、页面正在使用 |
| 需要连接 | 当前账户、页面或服务持有的有限期连接需求 | 已经连接、操作已执行 |
| Hub 路由可达 | exact MQTT 身份 + exact route generation + 有效 Hub 安全会话证明 | 子设备应用没有卡住、State 永远新鲜 |
| 状态新鲜 | 当前 Manifest、完整 State snapshot、连续 revision | 蓝牙会永久连接 |
| 可以控制 | 权限 + 可达 + 就绪 + 新鲜状态 + 可用发送窗口 | 操作一定成功；结果仍由原 requestId/ACK/ledger 确认 |

设备列表 Presence 是被动事实订阅，不能因为列表显示了几十个设备就建立几十条 BLE 连接。
MQTT keepalive、旧 State 缓存、BLE 广播、Active 绑定和普通上行消息均不是续租证明。

## 2. 执行顺序修正

先实现不依赖 idle 的 loss-all 安全底座，再开放按需连接。原来Admission与启动混在`GatewayChildRouteBridge.poll()`；
§10.76移除该隐式重连，§10.81已用唯一GATT策略替换product的Admission连接决策。
§10.82/10.83补齐Ready和实际App selector软件消费；现场配套尚未部署，仍须补离线手机公平与四端物理门禁，不能把源码接线等同真机验收。

已完成 **L2c-0a 语义与模块边界收口 + L2c-2a 可达租约软件实现 + L2c-2b 配套真机短门禁**。
真机补齐了真实 RouteClient 发送白名单漏接 proof 的问题；正常三周期、首 challenge/proof 丢失、loss-all、旧证明回放与恢复后一次Action通过。
版本、失败过程、计时与资源证据见 GATT §10.64。
§10.65 已完成 **L2c-0b1：App所有者生命周期、Broker有界聚合器及跨仓真实模块合同测试**。
§10.66 已完成 **L2c-0b2：App Session→北向BBP Route/Delivery→Broker身份/ACL/取消的软件接线**；
不是仍停在内部DTO。页面/AccountClient尚未自动申请。
§10.67 已完成 **L2c-0b3a：南向接收方时钟锚、双端exact codec与40 B执行租约核心**，见6.6。
§10.68 已完成 **0b3b1：实际Broker dispatch、owner变化通知、Hub RouteBridge收发和执行期限接线**，见6.7。
§10.69已配套部署并通过**App双owner/部分释放/全部释放/取消/session退役、force-stop且全丢zero的真机控制门禁**。
Hub executionWanted实际变化、消息未变成Action；普通断链恢复和App计数46→47通过，详见主线§10.69。
§10.70完成0b3b2的管理会话所有权软件子项（6.8）：命令本地截止时间、Prepare会话绑定、轮换后exact清理。
§10.71完成0b3b2准入/首Hello恢复分离软件子项（6.9），并保留失败投影之外的已确认authority和execution绑定。
§10.72已接页面/只读sync消费者；§10.73完成真实设备页短门禁与后台有界回收，见6.11。
聚合已驱动南向控制消息，但**尚未驱动物理连接调度**。下一步配套唯一调度与Direct/idle，
再整体激活；不重复实现北向所有者、南向codec或已拆开的预算。
Direct退让与idle尚未实施，不能称L2c全部完成，也不能以新增模块代替实际四端唤醒门禁。
§10.77完成Direct优先权语义、同一owner核心的目的隔离/排他预留及跨仓软件测试（6.15）；
未分配或发布新wire feature，现有cloud-only编码器明确拒绝Direct，App页面与Hub物理策略未切换。

## 3. 本批已实现：可达租约

### 3.1 exact wire

`GatewayRouteLeaseV1`：canonical CBOR 十项数组：

```text
[1, 3, acknowledgment(0/1), topologyVersion, childLogicalDeviceId,
 childDeviceInstanceId(bstr16), accessEpoch, gatewayCredentialVersion,
 generation(bstr8 big-endian), challenge(bstr8)]
```

- Broker → Hub：acknowledgment=0；Hub → Broker：=1；generation/challenge 均禁止全零。
- 只走已有 route topic，QoS 0、retain=false、强制 MQTTS；不进入 child BBP/2，不增加 App codec。
- closure 仍是 `[1,2,...]` 九项，admission 仍是 `[2,1,...]` 十项，业务 envelope 仍八项。
  编解码按 version/kind/shape 严格区分；不能仅凭“十项数组”认作 admission。
- Hub 的 synthetic Hello 新增 bit15 `FeatureGatewayRouteLease`；不让普通 child 声明此能力。
  新 Broker 拒绝缺少此能力的 Hub 投影，不能静默建立无限期 ready。Hub 与 Broker 须配套更新。

### 3.2 时间和重放

- 首次完整 Hello/Manifest/State ready 后开始 30 s 单调时钟租约。
- 10 s 时发 fresh random challenge；未收到 proof 时每隔 5 s 重发相同 challenge，首次租期最多 10/15/20/25 s 四次。
- proof 必须匹配 MQTT claims 对象、Active route、instance、epoch/version、generation 和当前 challenge。
  鉴权异步返回后再次检查全部对象和单调 deadline；事件循环迟到不能延长准入。
- 每个 challenge 只接受一次。新到期点为 **challenge 签发时刻 + 30 s**，不是 proof 抵达时刻 + 30 s。
  这样迟到/重复 proof 不会无限保活。下一次 fresh challenge 在前次签发后 10 s 到期。
- publish 或 authority lookup 卡住时没有 promise 队列、没有无限重发；独立到期计时仍有效。
- Command 原 3750 ms 预算不变，不借 Command 超时判断物理离线，也不靠 Command/ACK/State/Patch 续租。

### 3.3 Hub 的含义与资源

只有当前 `child.secure()`、已发该代 Hello、无 pending closure、route subscription/access revision 和凭据均匹配时才回 proof。
它是**受信任 Hub 对当前安全路由的声明**，并非新增加了一次 child 端应用 challenge/response。
它不能排除“蓝牙链路仍存活但 child 应用死锁”，该问题继续由有界读/Command deadline 和无线 supervision 处理。
物理断电到发现断链的时间也不包含在 Broker 的 30 s 租期中；不要宣传“实际断电后必定 30 s 内离线”。

设备复用已有 route output workspace，不增加字段、任务、队列、child BBP sequence 或 NVS 写入；不会干扰进行中的 Command、State 分页及管理事务。
请求不允许 start/connect，不产生需求，不阻止未来 idle。南向会话只需提供 secure/generation 边界，Broker 不依赖 BLE SDK。

### 3.4 到期行为

只退役 volatile child protocol session/State readiness/Presence 与发送窗口；保留 Hub MQTT、Service Active topology、durable GatewayAccess、generation 高水位。
已经发布的 Action 进入原 ledger 的 outcome_unknown，不能重新生成 requestId 重发。
迟到 proof、旧 Hello 和旧业务帧不能复活旧投影；fresh generation Hello 仍需完整协商。
**租约到期本身不自动 admission/reconnect**。因此设备没断线、只是证明全部丢失时，会保守停在离线，
后续由独立有界恢复/需求路径重新 admission；本批未用无限重连掩盖这个安全状态。

## 4. 下批连接需求合同：先补全控制路径，再开 idle

以下语义冻结；新增 wire 编号、时间默认值和资源上限须在实现前以跨端测试收口，不能当成已经发布的 API。

1. **独立 demand owner。** 每个页面/只读操作/服务消费者拥有自己的 demand token；绑定 authority、账户 sessionEpoch、target 和有限有效期。
   相同 target 的不同 owner 可以聚合，但 A 的 cancel 不得取消 B；停止账户、撤权、过期立即清理对应 owner。
2. **被动订阅不唤醒。** 列表 Presence、缓存 Manifest、普通状态缓存读取不持有 demand。打开控制页的主动同步或显式实时订阅才可申请。
   当前 `waitUntilReady` 只是观察者；未来由调用侧先取得 demand、再等待，离页/后台/取消统一释放，仍不得排队 Action。
3. **消费范围诚实。** 普通 WiFi 的 Telemetry 已有业务租约，但当前 Gateway bridge 只开放 Hello/Manifest/State/Command/ACK/Patch；
   不要把 Gateway Telemetry 当成现成需求源。先完成 page/read demand；Telemetry/自动化仅在对应完整路由实现后接入聚合器。
4. **Hub demand 与 admission 分开。** Admission 只证明可以访问；新增有序、有限期 execution demand 表达连接/保持/释放。
   release 与 acquire 必须有 request/generation fence；旧 release 不得释放新 lease，旧 acquire 不得重新占用 Direct。
   Hub 只用单调时钟保存 volatile demand，不把 App token/raw credential 写入 NVS。
5. **idle 是无需求的结果。** 普通 demand 归零并经过短防抖后，先封住普通重连，再退役投影和释放 ACL；保留绑定与凭据。
   管理 proof/revocation/renewal 有独立有限期所有权，不被页面关闭中断；管理也不能成为永久占用的借口。
6. **Direct 退让必须同批交付。** 在线 App 申请 Direct 优先时，应让出 Hub 普通连接并暂停其普通重连，完成 Direct 后才允许恢复。
   App 离线、崩溃、多个观察者及 management 争用时，仍必须有界退让。没有完成这些测试前不能启用“持续刷新页面 demand 永久占用 ACL”。
   continuous-hold/yield-window 的数值依实际连接时延门禁确定，不能通过增加固定几十秒等待解决。
7. **连接尝试有界。** acquire 的取消、超时、账号切换和权限失效必须能中止尚未完成的 wake；没有无限重试、多个未决连接或新 Action 队列。
   原业务 Action 只在当前 ready 时派发；它的结果不能用于延长 demand 或证明 lease。

模块归属：App 生命周期持有 token；Broker 独立 demand 聚合器做 ACL 和 owner 代次；Hub 普通调度器消费需求；
ChildSession/Link 只执行连接与安全会话；Reachability 只负责证据期限；Service 继续只维护 durable authority。
后续 ESP-NOW/Zigbee/Mesh 可复用 authority/demand/lease/ledger，但必须实现各自“已建立安全路由”的证明边界，不伪装成 GATT。

## 5. 验收状态与剩余门禁

1. **已通过（§10.64）：**新 Hub + Broker 配套部署，保持 MQTT、实际丢全部 closure/proof/新 Hello，在线投影到期失效、App ready拒绝；旧proof不能复活，额外5 s没有自动admission。
2. **已通过（§10.64）：**正常三个续租周期；丢首 challenge/proof 后约5 s exact恢复；旧generation/proof不影响新路由。租约门禁计数45不变；之后正常恢复与真实Action45→46。
3. **云端需求完整逻辑接线/页面短门禁已验，物理idle/Direct未完成（§10.69～10.77）：**owner、Route/Delivery、Broker聚合、Hub执行期限和页面作用域已接通；唯一连接机制已接线，但product仍按Admission获取连接。Direct核心隔离已验，不是线上Direct优先权握手或同绑定交替已完成。
4. **发布仍待验：**N2/N3/N4/N5、D4b加密生产板、SDK assertion首因、Wio与Hub长稳。短门禁/内部weak-storage/单次资源水位不能替代这些证据。

## 6. L2c-0b1/0b2 已实现的所有者与北向合同

### 6.1 最小职责与身份

- App：`core/protocol/device-v2/connection-demand.ts`，每个**精确认证session**一个管理器，页面/只读消费者各自持有独立handle。
  handle的`accepted`仅表示需求受理，**不表示已经连接、State新鲜或Action已执行**；后续仍须独立ready等待。
  `release()`幂等，AbortSignal取消只释放本owner；`closed`显式报告超时/拒绝/账号结束。sender不得透明切换账号重发。
- Broker：`protocol/v2/connection-demand.ts`，不依赖MQTT/Aedes、HTTP/DB、BLE、Arduino或UI。
  适配器注入当前session判断、单调凭据deadline、只读ACL鉴权及同步聚合变化通知；核心管理有界owner/到期。
  按授权返回的tenant与target聚合，只输出owner数量和**本进程单调deadline**，不输出秘密、App token或Command payload。
- Service仍只提供durable authority；不新增数据库表或OwnershipRecord。Hub后续只消费聚合后的execution demand，
  不保存各页面owner。后续ESP-NOW/Zigbee/Mesh可以复用这些核心边界，不需要伪装成BLE。

### 6.2 内部DTO（wire映射见6.5）

请求只有 `{ ownerId, revision, action, target }`；应答回显ownerId/revision及
`accepted | released | stale | denied | busy | capacity`。§10.65仅实现DTO；§10.66已按6.5接入BBP，不另建JSON旁路/topic。

| 字段/规则 | 已实现约束 |
|---|---|
| ownerId | session-local非零uint32，单调递增且不复用；耗尽后明确失败，不能回绕到1 |
| revision | 非零uint32；acquire为1，renew严格+1，release须高于现有revision；旧release不取消新revision |
| target | 不可变、1～128 B的ASCII标识字符；仅明确投影四个字段，不保留任意输入对象/业务字段 |
| session身份 | Broker比较精确对象，不仅比较clientId/accountId字符串；同一号码在新session可重新从1分配 |
| acquire | 同步预留owner/floor，然后才执行异步ACL；相同请求共享已有工作/结果，不延长deadline |
| release | 无需重新等待ACL；可抢先取消尚未完成的acquire。未知owner的release也推进floor，封住迟到acquire |
| 已删除owner | 不重新打开。session只保留一个floor，不为每次cancel累积墓碑；新owner需更大id |
| grant | 只能来自当前只读ACL解析，tenant≤128字符、version≤256字符；renew再鉴权，版本改变即退休原owner，wire还须单独限制编码字节数 |

同一session的新owner必须按分配次序进入发送/注册路径；乱序到达的较小新id明确stale，不能悄悄更换id重试。
因此MQTT接线时不可先并发等待鉴权再注册owner，否则较早的合法请求可能被较晚的floor拒绝。
release/旧应答不跨owner、target或账号代次；绑定Active与被动Presence不会创建owner。

### 6.3 时间、资源与取消

- Broker owner租期30 s，从请求受理时刻计算，且不超过当前账户凭据的单调deadline；鉴权耗时不会买到额外租期。
- App每10 s显式renew，仅一个请求进行中；每次请求5 s绝对预算。Broker异步鉴权同样最多等待5 s，
  超时/取消立刻结束本地owner和等待者；迟到结果在timer尚未执行时也会再检查deadline。
- renew鉴权中只保留原租期，不能提前续期。release丢失由Broker原租期回收，不靠后续消息或无限重试。
   **此处回收的是逻辑owner；§10.68虽已接Hub执行期限，仍不等于物理BLE连接已释放。**
- Broker最多256个owner、每session16个、每target32个；异步鉴权全局64个、每session4个。
  App每session16个owner，普通发送准入上限16个实际未结束请求，预留release后总上限32个。
  不因新需求驱逐其他合法owner，没有Action队列、连接重试队列或payload缓存。
- 不配合AbortSignal的底层Promise在实际结束前仍占工作槽，不能通过cancel→acquire绕过容量。
  这可能令该session显式capacity，而不是无限吃内存；真实HTTP/Session适配器还必须实现取消和自己的网络deadline。
- 账号停止/凭据到期/ACL撤销需要运行时同步调用closeSession/invalidate；核心同时在apply、snapshot、异步结果和计时回调复核。
  §10.66已接入真实App Session关闭与Broker claims清理/ACL入口；物理连接释放仍须南向实施。
- 聚合通知是同步、非权威输出；通知接收器抛错时本次acquire/renew拒绝，取消不等待通知成功。
  南向之后必须有独立execution期限，应对release或通知本身丢失，不能据此永久持有连接。

### 6.4 可复现证据与边界

Broker独立所有者测试17项；App生命周期测试13项。跨仓脚本直接编译加载两个真实TS模块，
检查双owner、cancel抢先于ACL、release丢失、账号更替、release先于acquire到达、renew/release全丢失、ACL撤销7项：

```powershell
# D:/codes/blinker-library；要求同级App/Service仓库和Service已有TypeScript开发依赖
node --test tests/integration/gateway_demand_contract.test.cjs
```

非同级目录可设置`BLINKER_SERVICE_ROOT`和`BLINKER_APP_ROOT`。脚本不读取登录凭据，不启动ADB/Docker/设备，
不修改源文件；这是**内部DTO/状态机联测**，不是CBOR golden、真实MQTT或四端真机证据。
单库CI不强制要求另外两个仓库；Broker测试已接入自身`test:device-v2`，App测试由标准Angular测试发现。

### 6.5 §10.66 northbound exact wire与生命周期

| 项目 | 冻结值 / 约束 |
|---|---|
| feature | `ConnectionDemand = 1 << 16`；**只承诺北向需求所有权，不承诺Hub已支持wake/idle** |
| 请求 | 内层kind `0x1c` ConnectionDemand；flags=0；canonical CBOR `[1, ownerId, revision, action]` |
| 应答 | 内层kind `0x1d` ConnectionDemandStatus；flags=IsResponse；CBOR `[1, ownerId, revision, status]` |
| action | acquire=0、renew=1、release=2 |
| status | accepted=0、released=1、stale=2、denied=3、busy=4、capacity=5 |
| 外层 | 沿用账户Route/AckRequired与Delivery/IsResponse，原requestId及LogicalDevice peerId关联；无新topic |
| 字段 | version仅1，owner/revision非零uint32；exact 4项；非canonical、尾数据、未知枚举/flags/direction均拒绝 |
| 身份 | target仅来自外层16 B LogicalDevice peerId；body不接受tenant、grant、MAC、DeviceKey或业务payload |

黄金字节：owner=1/revision=1的acquire为`8401010100`；owner=uint32最大值/revision=256的capacity为`84011affffffff19010005`。
此编号只在App↔Broker使用，不下发child，不要求普通BLE/WiFi固件增加相同feature。

实现与清理：

- App管理器已从上层`core/device-v2`迁入`core/protocol/device-v2`，旧文件删除、无兼容转发壳。
  `DeviceV2Session.acquireConnectionDemand()`同步返回handle；sender固定`this`，不经过AccountClient重新选session。
  ready和feature必须已协商；页面、AccountClient自动调用与只读sync作用域暂未激活。
- Session Route支持AbortSignal、所有完成/拒绝路径移除监听器；超时从提交publish开始，而非等待publish返回后开始。
  未完成底层publish普通上限16、含预留release最多32；取消/超时仅释放逻辑Route，原publish实际结束前不归还其容量。
  不对未结束publish再启动重试；已提交业务Command的不确定结果语义保留。Demand没有推送，故不积累notification peer索引。
- Broker在DB/异步ACL之前按peerId的32字符hex注册/取消owner；DB只在authorize hook内解析唯一逻辑身份。
  因此release不会被已删除/停用设备、尚未返回的ACL卡住；body不能另指定身份，未知peer的acquire仍拒绝。
  release可绕过已满的pending replay表，但同requestId异payload冲突仍拒绝；已缓存accepted必须回到owner核心重验，不续租/不复活。
- grant只来自当前owner投影或fresh shared read ACL；续期重验tenant/version，ACL完成后再次校验target tenant。
  真正disconnect、same-clientId替换、clear、凭据到期与ACL invalidation均清理owner。没有Manifest/Presence gate也撤销；
  较旧grant通知不错误撤下更高版本的已受理owner，尚未鉴权完的owner仍取消。凭据单调deadline在claims注册时锚定。
- AbortSignal贯通Broker→GatewayShareClient→Node HTTP；取消会关闭网络请求，不触发全局30 s Gateway故障冷却。
  现有HTTP 3 s绝对预算保留，cancel不抹去实际尚未结束的工作容量。
- §10.68已把Broker owner changed接到南向执行控制，替换原空回调；accepted仍仅“记录成功”，不能据此更新ready/在线。

软件证据：Broker northbound16项 + owner17 + lifecycle/reachability47，共80项在Windows Node22和隔离Linux Node20通过；
App标准Angular 11套122项（含新增Session/wire13项）通过。跨仓新增`gateway_demand_wire.test.cjs`6项直接加载真实App Session/codec和Broker dispatch，
二进制channel传递Hello/Route/Delivery，另保留内部DTO7项。真实HTTP取消用loopback服务器验证。
这些是软件/帧级证据，**不是实际MQTT WebSocket/Android/Hub/child四端门禁**；未部署Broker/APK或修改固件。

```powershell
# 先在Service仓库完成Broker build；同级App/Service仓库或使用两个ROOT环境变量
node --test tests/integration/gateway_demand_contract.test.cjs tests/integration/gateway_demand_wire.test.cjs
```

部署顺序必须Broker先、App后：旧Broker的strict HELLO会拒绝未知bit16；新Broker继续接受未声明bit16的旧BBP/2 App。
不要在当前运行的旧Broker上单独安装此App来声称兼容已验证，也不为此恢复Legacy JSON。

### 6.6 §10.67 southbound：接收方锚定的执行租约（0b3a软件基础）

**选择依据：**“Broker发送30 s，Hub收包后计30 s”会让排队旧包重新唤醒设备；generation只能区分代次，
不能证明同代消息仍新鲜。Hub先发challenge并记录自己的单调时刻，Broker只回当前需求的剩余时长，
可在不同步两台机器时钟的情况下保守扣除通信延迟。沿用现有MQTTS认证，不增加签名、加密库或墙钟协议。
SNTP仍服务于现有TLS/长期凭据时间判断；本执行租约不依赖其trusted标志，不声称SNTP是认证时间源。

#### Wire与方向

`GatewayExecutionDemandV1`为canonical CBOR十一项数组：

```text
[1, 4, operation, topologyVersion, childLogicalDeviceId,
 childDeviceInstanceId(bstr16), accessEpoch, gatewayCredentialVersion,
 incarnation(bstr0或bstr16), challengeId(uint32), leaseMillis(uint32)]
```

| operation | 方向 | incarnation / challengeId | leaseMillis | 含义 |
|---|---|---|---|---|
| Probe=0 | Broker→Hub | 空 / 0 | 0 | 只提示发起fresh challenge，不能直接连接BLE |
| Challenge=1 | Hub→Broker | 非零16 B / 非零uint32 | 0 | exact本地期限锚，不表示已连接 |
| Decision=2 | Broker→Hub | 回显Challenge | 0～30000 | 0释放；正数授予有界执行需求，不代表ready |

- 固定version=1/kind=4；此kind属于独立route合同，不是新增child BBP kind。最多200 B，当前golden为55 B。
  topology/accessEpoch/credentialVersion均非零uint32；child instance为非零16 B；逻辑ID沿用现有限制。
  exact形状、方向、非canonical、超长、尾数据、全零incarnation、非法零字段均校验，不能只比较数组长度。
- 复用route topic、QoS 0、retain=false与exact Hub MQTTS身份。App不实现本codec，不把App owner token或secret传入Hub。
  `GatewayRouteClient`发送只允许Challenge；§10.68已接Broker dispatch与Bridge接收，详见6.7。
  无新增协商bit：新Hub在准入后主动发首Challenge，Broker只对已收到合法Challenge的exact Hub发送新控制。
  不等child Hello判断支持；未发Challenge的旧Hub不会因owner变化收到Probe。新固件仍要求先部署新Broker。

#### 时间、代次与丢包

1. `GatewayExecutionLease.begin(IRandom&)`生成非零16 B随机incarnation；只能使用平台密码学随机源。
   每个exact subscription/admission/access生命周期一份，retire/重启清除；旧包不跨生命周期。
   challengeId从1递增，不回绕，耗尽明确失败。incarnation不是secret，也不能替代MQTT/Active鉴权。
2. `request(now)`仅保存一个pending与签发时刻。相同未结束request返回AlreadyExists，不能重新锚定；
   后续新challenge至少间隔1 s，pending签发预算5 s。核心不主动发包或计时重试，适配器必须另做有界发送。
3. Broker在**当前authority/ACL检查通过后、实际提交发送前**计算：
   `remaining = clamp(floor(currentOwnerDeadline - sourceNow), 0, 30000)`。
   `connectionDemandRemainingMillis()`仅做这个纯计算，不提供鉴权；无owner、到期、非法时间均为0。
4. Hub只接受当前pending的exact incarnation/id。positive一次消费，deadline为
   **Hub challenge签发时刻 + remaining**，不是收包时刻加时长；收包时已过5 s或remaining期限则拒绝。
   在正常单调时钟进展假设下，challenge签发先于Broker计算，通信延迟只缩短可用执行期，不给旧包增加新租期。
   这里约束逻辑执行授权，物理ACL释放仍取决于下一步调度poll/断链完成，不能宣称到点瞬间物理断开。
5. 零Decision可以抢先于同id的positive，关闭该pending并释放不更新于它的active lease；随后positive拒绝。
   旧id release不能取消新id active/pending；未签发的未来id拒绝。没有逐请求墓碑集合。
6. 丢grant不唤醒；丢renew/release保留原deadline并到期，不因重发、业务ACK或cached accepted自动续期。
   `poll()`锁存到期；uint32 `millis()`正常回绕通过，异常回退保守到期。要求MCU运行时持续poll，
   不承诺整整一个uint32周期不调用poll仍可辨别时间，也不把计时器当作线程阻塞/死机恢复机制。

#### 实现、资源、验证及尚未激活内容

- `GatewayExecutionLease.h`只管理易失需求/新鲜度，Host实测40 B且ESP32-S3 ABI编译确认40 B；核心编译期限制≤40 B，无堆分配、任务、队列、回调、NVS写入。
  它不引用BLE、MQTT或平台SDK。instance/topology/access/session检查属于适配器，不为每个App owner在MCU存一份状态。
  §10.67仅核心；§10.68已在生产Bridge实例化，连同发送时刻/次数与对齐，内部Hub例程静态RAM增加48 B。
  它仍不是物理调度器，不能视为已实现idle；整机动态heap/stack须另行真机测量。
- 双端codec共享golden；新增Host门禁包含delay/loss/replay/release/随机失败/millis回绕；修复了本批发现的
  非零整数helper误用于合法零challengeId/leaseMillis，保留Probe/Challenge/零Decision完整encode→decode回归。
  真实RouteClient→CloudMux→MQTT发送链路也覆盖方向校验；fuzz包含新格式及三种零时长变体。
- 跨语言`gateway_execution_wire.test.cjs`五项直接调用真实Broker剩余时长函数、Service codec和C++可执行文件中的decoder/lease，
  覆盖不同uptime原点、延迟、零owner、到期、旧incarnation、未来编号与回绕；是**软件二进制边界**，不是实际MQTT或物理wake。
- 复现：先构建Service protocol/Broker及Host `blinker_gateway_execution_lease_tests`，执行
  `node --test tests/integration/gateway_execution_wire.test.cjs`。支持`BLINKER_SERVICE_ROOT`、`BLINKER_EXECUTION_LEASE_TEST`；
  Windows ASan构建需将对应MSVC ASan runtime目录加入PATH。不读取账号/密钥，不启动设备或修改真实数据卷。
- **已接控制，未激活物理行为：**Broker owner changed→Hub控制发送见6.7；Bridge物理demand调度、page自动申请、idle和Direct退让仍待完成。
  Admission自动启动仍在使用；仅在第7节配套替换时删除，不为收缩文件数提前破坏在线链路。

### 6.7 §10.68 已实现的南向控制接线（0b3b1）

- **设备：**`GatewayChildRouteBridge`在exact admission/access delivery revision/订阅代次下初始化租约；
  同准入重发与仅投影Hello换代不换incarnation。首次poll发Challenge；单次最多0/1/2/3/4 s五次exact发送，5 s到期停止。
  同一pending上的Probe不重置时刻或次数；收到合法Decision更新`executionWanted()`，不消耗child BBP序号，不调用start/connect/stop。
  有效执行需求时，距上次发送至少10 s可发fresh renewal；无需求不持续续租。取消订阅/准入失效清除逻辑租约。
  首次五包全部丢失后停止，**不假装已经完成能力确认**；§10.71/6.9已补一次由有效Admission重发触发的有界重新确认。
- **Broker独立执行控制器：**`EdgeGatewayExecution`注入current/authorize/remaining/send/monotonic clock，
  不引用Aedes、DB、BLE或App owner集合。每Hub最多一个真实未结束工作、一个最新challenge、一个合并变化标记，
  最多工作deadline与合并通知两个timer，无逐请求队列。鉴权5 s预算，重复challenge不重置首收包预算；Hub仍以更早的自己签发时刻最终判断。
  Challenge鉴权准入最多1次/s；变化通知最多1次/s，Probe不占用随后Challenge的限频时刻，避免人为再增加1 s正常往返等待。
  release可中断HTTP，底层Promise/publish未真正结束前仍占槽，不因超时归还容量；挂死时依Hub原执行期限兜底。
- **source授权：**仅positive需要fresh Active route鉴权；返回后再检查exact claims、当前准入/instance/epoch/version和source剩余期限。
  zero只移除权利、不等待HTTP，也不能在没有鉴权的零需求分支转成positive。变化只触发Probe或zero Decision，
  不用缓存accepted直接授予新时长。普通合并发送可能延迟≤1 s，不能声称release包必定瞬间到达。
- **composition：**Broker统一初始化App owner监听器与target snapshot getter；原`changed: () => undefined`已移除。
  执行状态按tenant+peer索引到当前已确认的Hub，并独立于volatile child投影。Hello换代、closure与reachability到期保留它；
  claims更替、撤权、真正断开、完整runtime清理关闭它及工作，迟到HTTP结果不再发送。Route身份字节独立拥有，不能被投影清理抹掉。
- **当前有意保留的边界：**executionWanted只是逻辑需求。Admission自动start/connect与原首Hello恢复仍在用；
  本批未把这些逻辑删除，未增加eager/demand双配置。旧Hub无Challenge不收到新控制；新Hub先发Challenge会被旧Broker严格拒绝，
  因此**Broker先升级、Hub后升级**。当前未部署，运行基线仍是§10.64。

软件证据：Broker新增执行模块9项、真实route/HTTP/投影6项、App owner通知1项，累计Broker97项；shared协议21项，
Windows/Linux均通过118项。Host ASan47项（含新增Bridge收发/重放/五次耗尽）、20万fuzz、原跨仓18项通过；
ESP32-S3 3.3.11内部Hub例程Flash1,431,784 B、静态RAM65,388 B，相比§10.67增加1,628/48 B；Host Bridge对象1600 B。
这是软件接线/编译门禁，不是实际MQTT无线、手机/Hub/child四端或动态资源水位；未烧录、reset或部署。

### 6.8 §10.70 管理会话占用（0b3b2已完成的软件子项）

**修正“nonce期限”表述：**现有 `ControllerControlChallengeBody` 只有 nonce，没有 TTL；child 端 nonce 随
`transportId/sessionId` 对应的 DirectSecure 会话失效。本批不新增 nonce wire 字段，也不改 child 安全协议。
Hub 以已鉴权 Prepare 命令的截止时间约束保留连接的窗口，而不是声称 child 会自行按时间清除 nonce。

- `GatewayManagementBudget` 8 B，仅保存首次接收时的单调时刻与剩余时长；proof/revoke/renewal 各一份。
  Service 既有阶段默认120 s、上限300 s；Hub要求可信 Unix 时间，将剩余≤300 s一次性钉住，同时继续检查 Unix 截止时间。
  超期、超上限、时钟不可用均不启动工作。相同命令重发/结果发布不重设预算，SNTP 回拨不能延长；正常 millis 回绕支持。
  到期由持续 poll 锁存终态，不提供线程卡死或整整一个 uint32 周期不 poll 的恢复保证。
- `IGatewayManagementControl.ownsChildSession()` 与 `busy()/pendingResult()` 分离：
  Prepare响应后busy可false，但exact非零鉴权attempt仍保持到原命令截止时间、会话丢失/换代、合法后续阶段或取消。
  poll及相同命令重传路径均失效旧NonceReady，不向新会话重放旧nonce；失效后不靠再次publish复活占用。
  mux除工作/待发结果外也检查此所有权，因此不同管理家族不能在Prepare→Apply间隙重置会话。
  Permit-join仍持有自己的adapter窗口，默认不声称拥有native child session。
- proof仍只清理自己发起的exact attempt；只观察已有安全会话作为证明，不取得该连接的断开权。
  renewal在Rotate后沿已有mutation attempt等待断开完成；背压/异步断开不能视为完成，也不清理后来新建的attempt。
  持久receipt回放只在当前鉴权仍是旧凭据版本时取得该旧会话的清理权，不因回放误断新凭据会话。
  清理待完成时保持管理占用；逻辑命令截止时间不是绕过底层断链完成/资源释放的理由。
- 迟到响应不能在命令预算外推进存储。renewal Apply保留不可逆journal并返回ForwardRecoveryRequired；
  revoke的Expired也**不证明child未执行**，不能据此盲目重发mutation，继续沿既有恢复/receipt流程处理。
- 新增8 B×3预算及两个4 B nonce attempt，MCU静态增量32 B；无新增任务、堆、队列、NVS写入或公开Sketch API。
  删除无代次、发起即结束的旧轮换清理接口，复用mux唯一dispatch。软件门禁及编译实测详见GATT §10.70。

**尚未接物理调度：**此查询已经被mux用于互斥，但composition尚未用其控制idle；仍沿用原admission自动连接。
本批未部署/烧录/修改App或Service生产代码，真实运行基线仍为§10.69。

### 6.9 §10.71 authority送达与物理首Hello恢复分离（软件已完成）

复用既有Admission/Execution/Hello wire，无新kind/feature/API。Bridge和Broker必须配套升级；不能把本节恢复保障
套用于旧Hub仅一轮Hello实现，也不因本次预算收口广播已支持按需唤醒。

| 阶段 | 有限预算及确认依据 | 失败后的事实 |
|---|---|---|
| 初次authority送达 | Broker原0/2/5/10 s exact Admission、30 s一轮、至多一次1 s后重试；exact MQTT身份/准入匹配的Challenge或合法新Hello确认 | 始终没确认时两轮后退役准入；Hub MQTT/management不关闭 |
| Hub execution确认 | 每batch五次exact Challenge；首batch耗尽后，有效重复Admission最多触发一次fresh challenge；incarnation不变 | 两batch耗尽后不在poll中无限重试；后续明确Probe或新authority/订阅生命周期可重新确认 |
| 已建立安全会话后的首Hello | Hub在实际DirectSecure且分配了投影时起算；0/1/3/7 s与12/13/15/19 s两轮，共23 s截止 | 保留authority；不伪造ready、不重新认领、不自行churn BLE；相同Admission不能重开预算 |
| Broker Manifest/State及投影恢复 | 沿用原bootstrap绝对预算和至多一次恢复，Admission带retiredGeneration请求更新投影 | 已确认authority与execution绑定保留，ready/命令/状态投影清理；不出现第三轮后台恢复 |

- **确认authority不是确认物理连接。** 首次零generation的Admission可由exact Challenge关闭送达重试，即使无owner、无Hello。
  此确认不授予positive执行权；positive仍走fresh Active鉴权和剩余owner期限。未匹配epoch/version/instance的Challenge不能确认。
- **非零恢复floor不能被旧Challenge确认。** Hub即使早已收到authority，也不代表收到新的retiredGeneration。
  因此投影恢复Admission仍等合法新Hello，不能见任意Challenge就关闭其重发。该阶段耗尽保持authority，但绝不保持ready。
- Bridge首Hello两轮复用同一frame/correlation/generation及已有timer；收到合法同generation下行后停止。
  Admission重发、owner续租和重复消息不能重锚；新安全会话/真实MQTT代次/合法投影恢复才建立新的交换。
  23 s是首Hello发送预算，不含之前的BLE扫描/认证耗时，不是整个页面打开耗时保证。
- Broker抽出唯一`deliverAdmission`发送路径，区分`deliveryConfirmed`与child projection。
  bootstrap失败清理旧Command并保持原unknown语义，不借恢复自动重发Action。两轮后旧generation仍被拒绝；
  真正新generation可重新完整协商，不能用旧State/Presence恢复ready。
- 修复恢复退避期的两处竞态：重复准入请求不取消已安排的唯一恢复；真实新Hello已开始恢复时，旧退避timer不再发多余Admission。
  所有重发仍检查exact claims/authority，撤权或身份更换后迟到HTTP不发送。
- 仅增加一个确认状态byte，利用现有对象对齐：Host Bridge仍1600 B，ESP32静态RAM仍65,420 B。
  没有增加MCU队列、frame workspace、任务或NVS写入。软件证据见GATT §10.71；未部署，现场版本仍为§10.69。

### 6.10 §10.72 App 页面/只读同步的生产消费者

App 作用域已接入，**不等于 Hub idle/Direct 调度已经激活**。不新增 wire、owner 引擎或 feature 位。

- `DeviceUiPort.openConnection(id, signal?)` 返回内部 UI connection scope：页面持有到离开、隐藏、后台、销毁或设备切换。
  `ready` 仅表示本次同步完成，不结束需求；`close()` 只取消自己的 owner。可同步检查 `signal.aborted`，
  不依赖异步 closed 通知来阻止已退役连接上的 UI Action。
- `connect(id, signal?)` 是只读 one-shot：复用同一入口，成功/失败/取消都释放；不把 Action 排在同步后自动执行。
  整个准备阶段（含 account login）最多30 s；它不是页面存活期限。准备完成后只由既有 Session owner 续租。
- 仅 `deviceType=ble && cloudEnabled=true` 且选择 Cloud 时申请 Gateway child 需求。普通 WiFi/Hub 不需要南向 wake；
  Direct 路径不申请 Cloud owner。当前 Direct/Gateway 选择限制仍沿用旧规则，优先退让是下一项，不宣称已解决。
- `AccountClient.acquireConnectionDemand()` 只通过 exact current Session；登录前/后都检查取消，旧 account generation 结果拒绝。
  owner 的 renew/release/ACL/Session retirement 完全复用现有模块；迟到 accepted 不复活旧页面。
  Session 更换关闭旧 scope，当前前台页面收到新的 account ready 才重新申请，后台不会自动恢复 owner。
- page scope 的关闭与只读完成分离；页面不再调用按 device 全局 disconnect 误取消其他调用方。最后一个页面 scope 才释放 Direct transport。
  AppVisibility 在 UI port 统一关闭所有后台 scope；页面仍负责自己的 visible/input 生命周期。
- 删除两个隐式副作用：`DeviceService.queryDevice/queryDevices` 的 V2 inventory 分支只观察 Presence；
  `watchConnection` 在 Direct→Cloud 切换时不再自行 ensureReady。真正前台页面收到连接变化后持有 scope 再同步。
  显式 `{get: state}` 的只读调用仍走 one-shot；历史 rt 查询不充当 Gateway telemetry owner。

软件门禁覆盖多个 owner、准备中/准备后取消、后台/销毁、只读失败、Session retirement、挂起登录及真实 App→Broker wire。
§10.72 已配套部署 Hub/Broker、安装 App 并恢复 mostfun2026；设备真实安全会话/路由 ready 和后台零需求负测通过。
当时手机锁屏阻断前台page/Action门禁；后续实测与修订以6.11及GATT §10.73为准。

### 6.11 §10.73 页面回收的判据与后续物理入口

- Android现有Capacitor在`onStop`才通知`appStateChange(false)`。App同时监听`pause`提前关闭页面scope；
  `appStateChange(true)`恢复前台，初始化`getState()`不能覆盖已发生的生命周期事件；销毁时释放两个listener。
  这仍是best-effort：JS已冻结、网络失败或进程被杀时，不承诺释放包必达，不加后台常驻任务或第二套原生MQTT。
- 分开验收：**本地scope取消后不可控制/不再续租**；Broker由收到release、session退役或最后一次有效需求的原30 s期限清理；
  Hub由对应zero Decision或本地原执行期限归零。40 s是硬件观察预算（覆盖取样/宿主调度），不是新的wire租期。
  页面替换可能短暂残留一个已取消但release丢失的远端owner，必须在原期限内收敛，不允许持续续租。
- 最终验收必须等待Broker最后一个Decision为zero，再观测Hub wanted=0；只看owner=0或一条较早的串口zero不足以排除在途positive。
  单次Action沿用exact frame/ACK/ledger；重发同frame不能导致计数增加两次。one-shot结束也检查owner归零，无自动Action。
- 最终APK真实页面短门禁通过：单次Button 5→6，退出/后台释放、返回前台、新账户Session与one-shot（17 ms复用已新鲜状态）恢复。
  后台owner=0在30.087 s观测到，连同Hub串口取样为39.481 s；是有界回收通过，**不是主动release必达/立即释放通过**。
  全过程同Hub MQTT/同child投影；最后owner0、zero Decision、Hub wanted0且计数仍6。没有操作Access或重新认领。
- 原30 s期限是失联兜底，不是Direct连接的正常等待时长。下一批不能把“等页面租约自然过期”作为Direct优先方案。

下一批唯一物理编排须覆盖以下实际入口，不再重做codec/owner/预算：

1. `GatewayChildRouteBridge.poll()`删除普通Admission触发的持续start/connect，将ordinary execution意图交给唯一编排；
   仍负责数据/控制转发、序号和Hello/closure，不把MQTT/页面逻辑塞进ChildSession/Link。
2. 编排同时消费普通需求、management `ownsChildSession()`与permit-join独占窗口；`busy/pendingResult`不等于NonceReady后的会话所有权。
   管理原命令期限不被页面续租改变，轮换后的exact清理权保留到真正断开。
3. `Disconnecting`期间持续poll并占住物理容量；取消尚未完成的scan/connect/auth不能通过`stop()`假装SDK已释放后立刻再连。
   CloudConnecting/ManagementSubscribing当前保留BLE会话的分支也必须受有界所有权约束，不能绕过offline退让。
4. 在线Direct主动让出、无云有限连续占用/让出窗口、management争用和持续page renew一起验收；时值由真实连接时延验证。
   不引入eager/demand双配置，不增加Action等待/重排队列；配套通过后才启用物理idle并删除旧自动重连入口。

### 6.12 §10.74 记录退役不等于物理容量释放

物理调度实施前的三层合同修正，不改变身份、凭据、BBP/2或页面需求协议：

- **GattDirectChildLink：**记录层退役立即停止该记录会话；用原来的retirement字段保留closing attempt，
  只有Central Port确认Idle/对应完成后才向ChildSession通知断开。取消扫描/连接即使尚无记录会话，也必须完成同一通知。
  pending cancel期间禁止新connect；失败只重试cancel，不新建attempt、不重排Action；迟到旧代回调不能关闭新代。
- **ChildSession：**故障后connected/connecting=false不代表无原生资源；只要Link仍持有attempt便保持Disconnecting。
  管理coordinator原有exact attempt所有权因此能持续到真实释放，不再被提前Idle误导；旧代disconnect事件被过滤。
- **ESP32 Port：**cancel与CONNECT success竞态时必须收养并终止该ACL，不能把EALREADY等同“没有连接”。
  终止请求失败继续占槽并重试，接受后不反复发同一terminate；GATT discovery失败同样不能直接清理原生句柄。
  扫描取消失败不制造完成：显式cancel保留Scanning；扫描转连接失败也保留原attempt，等待后续广播重试或owner取消。
  stop等待还包括native connecting及迟到CONNECT，超时为Error，不假装Stopped；不因此重启NimBLE host。

生产Central源码通过SDK边界桩测试覆盖上述竞态。它证明本库的事件处理，不代替真实SDK控制器调度/长稳或ipc栈首因分析。

**§10.74时的composition交接待办（第1项由§10.75、第2项由§10.76接续；第3项仍待办）：**

1. 目前`BasicGatewayChildSession.stop()`/Link stop仍是同步拆卸入口；`acquirePermitJoinPort()`仍经它强制停数据面。
   应改为poll驱动的独占交接：保留旧handler/attempt直到原生关闭，确认Port Stopped之后才换handler给permit-join。
   `GatewayPermitJoinCoordinator`当前把openWindow的WouldBlock立即回为失败；必须同批支持原命令期限内等待交接，
   否则仅换成非阻塞cancel会破坏已有配网入口。关闭permit窗口的反向交接同样不得提前归还radio。
2. proof仍直接start/connect，revocation/renewal经RouteBridge.ensure，普通Admission经RouteBridge.poll；
   下一批集中为唯一物理编排，管理cleanup仍以exact attempt为准，不能只检查busy/pendingResult。
3. CloudConnecting/ManagementSubscribing保留子会话的分支仍没有offline公平窗口；与在线Direct主动让出、
   无云有界占用/让出、持续page renew及管理争用一起实现。没有这组配套，不启用物理idle、不宣称Direct交替完成。
4. 不新增候选/eager双模式、Action等待队列、平台特例用户API或新安全层。GATT §10.74记录软件/真机版本和证据边界。

### 6.13 §10.75 permit-join双向异步交接与准入保留

本批消费6.12第1项，不改变wire、用户API、身份/凭据或Action语义。普通连接入口的最终集中和Direct公平性仍属下一批。

- **Adapter负责物理生命周期：**open返回WouldBlock时保持Opening，由poll重试lease；未取得lease不得碰旧owner的Port或handler。
  只有旧ChildSession/Port均Stopped后才取得radio并绑定permit handler。close立即撤销候选/relay读写，但保留exact attempt和handler，
  Closing中poll/cancel直到Port真实Idle→Stopped再解绑/归还。取消失败只重试取消；Disconnecting期间不重复已接受的请求。
  原生状态未知/错误不能伪报Closed；正常交接不使用带活动ACL的同步stop，终端析构仍需解绑悬空callback。
- **Coordinator负责命令期限：**Opening不回Busy或Ready，保持同一operation等待；Ready仅在实际扫描就绪后发出。
  原命令剩余期限只钉住一次，Service已有900 s上限（默认300 s）；重发、SNTP回拨或uint32计时回绕不续期。
  Close/Expired/Rejected先禁止relay，待原生释放后才公布终局；同operation重发不提前公布终局或重新打开窗口。
- **管理重置不等于物理释放：**reset清旧MQTT命令/结果，但Adapter Closing继续占radio；mux即便active=None也继续poll清理，
  其他管理family不能抢占。无效管理消息也必须推进已持有的关闭；product停止后继续drain，restart不得越过未完成的permit关闭。
- **临时占用不清拓扑准入：**`GatewayChildRouteBridge.pollControlPlane()`维护有效准入、Execution和exact Closure，不start/connect。
  permit占用期间composition调用它；普通ChildSession断开可发Closure，关窗后沿用原准入恢复新Hello，不依赖Broker耗尽后的Admission重发。
  非Online仍清旧准入，旧MQTT代次不能因此跨代保留。普通eager调用仍在现有`poll()`，待唯一物理调度配套替换后删除。
- 不新增线程、缓存队列、安全层或可选eager模式；复用已有8 B管理预算组件，不把SDK生命周期搬进App/Broker。

Host/实际构建及真机证据以GATT §10.75为准。以上不等于Direct→Gateway→Direct、物理idle、离线公平窗口或长稳门禁通过。

### 6.14 §10.76 唯一连接机制已接线；Direct控制合同先行

6.12第2项的连接入口已收口，**不等于完整按需/公平调度完成**：

- `IGatewayChildConnection`仅暴露`ensureConnected/retire(exactAttempt)`；同一runtime实例被ordinary、proof及经RouteBridge的
  revocation/renewal共享。机制不认识MQTT、GATT SDK、owner表或具体凭据；各协议仍校验自己的授权，composition决定能否获取连接。
- RouteBridge的普通`poll()`只维护有效准入/Execution/Closure/Hello并返回准入状态，不再start/connect。
  Proof与ControllerControl不再各自操作物理连接。临时保留的Admission连接决策只位于product的一处，不设eager/demand双模式。
- exact关闭屏障独立于协议命令上下文：reset清掉结果后仍须完成旧attempt原生关闭；取消背压只重试取消，Disconnecting不重复发请求。
  同attempt的Fault不是空闲，迟到旧代清理不关闭新代。即使仍报告Secure，管理复用与proof也必须过屏障；
  一次调用里完成旧关闭后不得紧接着在secure复用检查中隐式启动新连接。
- permit-join仍使用6.13的独占交接。管理事务优先于普通连接；非Online禁止新获取。
  CloudConnecting/ManagementSubscribing保留旧BLE的策略暂未改变，尚无offline公平窗口。

**纠正执行顺序：**现有ConnectionDemand只表达云端连接需求；permit-join是配网/管理relay，不是App Direct优先权。
现阶段不能通过清除其他owner需求、长期打开permit窗口，或只移除App Direct准入限制来实现“退让”。下一批先冻结：

1. 同绑定App Direct请求的独立、可取消、有界优先权合同：已登录账户ACL、exact child/instance/topology及Hub会话关联，
   receiver单调期限；重传不续期，账户/ACL/会话失效撤销，持续page renew不能压过Direct。具体wire尚未冻结，不发布新feature。
2. Hub唯一决策同时考虑云端executionWanted、该Direct请求、管理exact所有权与异步closing；
   管理占用时显式等待/拒绝，不能伪报radio已释放。App收到真实释放后才尝试本地连接，并保证退出/失败可取消自己的请求。
3. 无云时仍有本地有界持有/让出；具体时间由连接恢复与业务门禁测量后确定，不用随意几十秒超时掩盖问题。
   同批封住CloudConnecting/ManagementSubscribing保留连接的旁路。
4. 配套通过后一次替换product中唯一Admission决策为按需/公平策略，删除该旧决策；不新增长期双配置、Action等待/重排队列，
   不用拓扑解除/重绑、更换凭据或设备reset代替物理交替。软件与同绑定Direct→Gateway→Direct均通过后才宣布物理idle完成。

### 6.15 §10.77 Direct优先权核心合同与实施切片

**§10.77历史状态：语义与共享owner核心完成。§10.78已接通northbound预留wire和真实authority，见6.16；Hub退让回执、App物理消费尚未接线。**
本批没有新增独立owner注册表、等待队列、SDK层或用户API；不将已有cloud ConnectionDemand改变为另一种含义。

#### 6.15.1 必须分开的四步

1. **Reserved/accepted：**当前账户得到这个逻辑目标的短期Direct优先权预留。不是Hub已释放，也不是BLE/State已就绪。
2. **Yielding：**Hub收到匹配当前MQTT与Active绑定的控制，停止普通获取并通过共享连接机制退役旧attempt。
   pending native close、管理proof/revocation/renewal及permit独占都不能提前报Yielded；管理原期限不因等待者而续期。
3. **Yielded：**Hub在普通连接确实释放、没有其他本库radio owner、且仍承诺不重新获取时给出exact回执。
   Broker再次确认账户/Direct权限、binding、Hub session和期限。该回执仅证明本Hub让出，不证明没有其他central占用。
4. **DirectSecure：**App才用原ControllerCredential执行真实Method 2/DirectSecure，再获取Manifest/State；此后才可控制。
   没有新设备身份、配网、清Access、重新绑定、刷新Key或Gateway secret下发给App。

release/到期撤销的是Hub的退让承诺，**不证明手机的ACL已经关闭**，也不能强制终止别的central。
App自己的连接作用域仍负责断链，child的鉴权/空闲/单连接DoS门禁仍独立；Gateway恢复只获得尝试权限，不自动ready或重发未知Action。

#### 6.15.2 已实现的所有者、身份及资源边界

- 在既有App/Broker owner核心中增加不可变`purpose: 'direct'`；不填写仍是现有cloud。ownerId同一账户session内统一递增，
  不另开ID空间、timer集合或队列。acquire/renew/release都保留purpose；改用途或target返回stale，不能改写/取消另一用途。
- Broker按规范化logical target预留至多一个Direct候选，**包括等待ACL的候选**；同步占位后才异步鉴权。
  第二个账户/手机得到Busy，不自动排队、更换owner重试或在前一个结束后自动继任。不同target独立；现有总量/每session/每target/异步工作上限继续共用。
  未鉴权候选最多占用原5 s鉴权预算；未来wire沿用真实账户流量限制，不把有界内存测试写成RF DoS发布证明。
- `snapshot.owners/deadline`仍只统计cloud；新增`directOwners/directDeadline`独立统计Direct，不把Direct当作正向cloud execution。
  关闭Direct不清cloud、持续cloud renew不延长Direct；账户停止/target ACL撤销能同时退休其两种用途，其他账户/目标不受影响。
- 普通read授权不能自动变成Direct权限。Direct authority adapter须提供当前账户/Direct权限、Active child绑定及Hub能力验证，
  并返回非secret `binding`（1～128 ASCII标识字符）和本进程单调`bindingDeadline`。缺失/无效/已过期都拒绝。
  binding由适配器绑定tenant、child逻辑ID/instance、Hub身份/当前MQTT对象、topology、epoch及Gateway credential版本；
  是内部opaque关联键，不接受客户端自报，不是凭据、MAC或可跨重启的deviceId。
- `grant.version`继续只表示现有ACL版本，binding单独比较；不能把它拼进ACL版本而破坏既有invalidate/keepVersion规则。
  renew重验两者：Hub/拓扑变更退休原owner，而非静默把旧请求迁到新Hub。关联表随账户/绑定生命周期清理，不建durable表。
- 逻辑优先权沿用30 s期限、10 s显式renew、5 s单次鉴权/请求预算，并裁剪至账户和binding剩余期限。
  期限锚在请求开始，不在ACL完成时；相同请求重放不续期，renew等待只保留原期限。取消不合作的Promise仍占原工作槽。
  **30 s只是已有逻辑lease预算，不是本批选定的BLE连续持有/退让窗口或页面等待时长。**

#### 6.15.3 防误接线与后续wire准入

§10.77时生产Session只发cloud；两端`encodeConnectionDemandBody`遇到purpose即拒绝，避免四项旧wire悄悄丢掉用途。
该cloud-only防护继续保留；§10.78已增加独立Direct编码、协商和真实authority入口。UI、Hub运行策略和已部署镜像仍未切换。
内部跨仓测试可注入正确Direct authority验证owner语义；它不是线上Direct已接受，更不是Hub已经退让。

下一批按以下约束完成wire/golden及实际适配，不再重做owner/timeouts核心：

1. Direct请求必须有独立、明确协商的消息合同；沿用账户Route/Delivery、target/requestId/revision及取消预算，不新增JSON/topic旁路。
   不复用配网permit窗口、不把cloud release=Direct请求；未协商端明确拒绝。northbound编号/形状已在6.16冻结并由两端golden验证；Hub合同仍未完成，不能混称三端wire通过。
2. Broker把当前唯一Direct owner转换成绑定当前Hub的非secret控制代次；Hub以接收方challenge锚定期限，确认原生关闭后回exact Yielded。
   Probe/accepted都不能触发Ready；old control、old Hub MQTT、old incarnation、已取消/到期/变更binding的回执无效。
3. App收到的Ready必须绑定当前owner/revision/target和当前控制代次；不能直接把底层owner.accepted暴露为ready。
   回执/查询要受原绝对期限限制，不能用每次收到的remainingMillis从接收时间重新计时，不能缓存重放已失效的Yielded。
   需要各端发送前/异步权限完成后再次校验，并以先于对端challenge的本地请求锚保守裁剪；各端时钟不互相比较。
4. 线上控制、普通execution、exact管理所有权、关闭屏障、离线公平由唯一product决策组合；不添加双模式候选scheduler。
   App仅在合法且真正让出后尝试Direct，失败/退出仅释放自己的优先权；cloud需求保持独立。
5. 无云时Hub必须本地有界持有/退让；同时修CloudConnecting/ManagementSubscribing保留连接旁路。
   单ACL占用时child可能不广播，不能把“当前未扫描到child”当作不需退让的永久理由。
   具体持有/退让时长必须由真实重新广播、发现、Method 2及管理争用测量收口，不能照搬30 s逻辑lease。

本批软件门禁：Broker类型检查、需求/真实app-lane/execution **51项**、Broker Device V2整体回归；App **63项**（含连接选择/账户客户端及误接线不发包）；
加载真实两仓模块的跨仓测试 **13项**；设备Host **48/48**与架构检查通过。
没有新部署/烧录/真机Direct交替结果；设备仍为§10.76，counter=10/revision11基线。后续验收不得复用该旧固件证据冒充Direct通过。

### 6.16 Direct northbound预留wire、真实权限和Hub绑定（§10.78）

本合同只实现 **Reserved**，没有隐含Yielding/Yielded/Ready，也不发起BLE连接或管理permit。
复用既有owner核心、单调期限、Route请求取消、replay/requestId预算和账户流量限制。

| 项目 | 确切合同 |
|---|---|
| App/Broker feature | bit17 `DirectPriority`，独立于bit16 cloud ConnectionDemand；必须双方Hello协商 |
| 请求 | inner kind `0x1e`，flags=0，canonical CBOR `[1, ownerId, revision, action]` |
| 回应 | inner kind `0x1f`，flags=IsResponse，canonical CBOR `[1, ownerId, revision, status]` |
| action / status | acquire=0/renew=1/release=2；accepted=0/released=1/stale=2/denied=3/busy=4/capacity=5 |
| 身份与关联 | 外层LogicalDevice Route/Delivery中的16 B peerId、requestId；target不在body重复声明 |
| owner / revision | 非零uint32，与cloud共用编号和高水位；用途由kind决定，不接受body附加purpose |
| 物理能力 | bit18 `GatewayDirectYield`仅预留、尚未在Hub/Broker设备lane协商或广播；缺少时Direct申请拒绝 |

request owner=1/revision=1/acquire golden为`8401010100`；status owner=max/revision=256/capacity为`84011affffffff19010005`。
body复用整数编码，不代表kind可互换；两端严格拒绝未知action/status、零ID、越界、非canonical及尾随字段。
`accepted`不会携带Ready/TTL。未来真实Yielded查询/回执仍需单独合同，不把status=accepted改成Ready。
bit17只说明服务器懂预留合同，不说明这个Hub支持退让；bit18在其真实receiver/回执完成前禁止宣告。

实际权限：既有内部`app-device-access`增加`purpose=direct`，仍验证内部shard、账户/tenant/设备归属及active状态。
目前Service只确认了主人的Direct Admin记录，没有分享Controller已落设备的完整合同：
主人也必须查询该记录并得到`directAccessEpoch`，缺失则拒绝；cloud viewer/operator不自动获得Direct权限，分享的cloud访问不变。
这是当前明确的支持边界；将来扩展分享Direct须先实现对应Controller凭据同步/撤销，不能仅把read/operator判断放宽。
不下发raw secret、controllerId/fingerprint，不增加数据库表或迁移。

Broker在原5 s总预算/AbortSignal内继续查询Active child route，核对Direct epoch与route.accessEpoch；
再匹配现有route admission投影、Hub能力及exact当前MQTT claims，不以旧State、MAC或仅“在线”作为授权。
`binding`是当前claims的随机进程内关联值和tenant/Hub/child/instance/operation/topology/epoch/credential到期的SHA-256，
只在内存使用，不作为设备ID或凭据传给App。复用原MQTT状态WeakMap，不新增owner表。
`bindingCurrent`捕获已有投影，并在接受/查询聚合/后续owner操作时重验；异常视为失权。
MQTT断线/同sessionId重新登记、投影或能力变化不能复用旧绑定。未来Hub控制发送仍必须重新验活，不能缓存预留快照。
绑定时长裁剪到Hub会话和Gateway credential，owner继续使用原请求锚；ACL版本沿用原owner ACL合同，epoch不拼入ACL版本。

App Session内新增`reserveDirectPriority()`，返回同一owner handle（只有accepted/closed/release，没有ready）；
发送、renew、cleanup保持Direct kind，未协商时连cleanup都不降级cloud。收到cloud status不能满足Direct请求。
此入口尚未接入account/UI连接选择，不可由accepted直接调用BLE；不改用户Sketch/API。

验证：Broker57项及完整Device V2回归；App预留/作用域66项和基础协议44项；跨仓17项；既有设备Host48/48、架构检查通过。
Service假库与实际PostgreSQL隔离schema验证Direct Admin注册前后、错误shard和cloud分享拒绝，测试schema已回收。
这些是软件/数据库证据；没有MQTT镜像/APK/MCU更新或新的物理Direct交替结果。

### 6.17 Hub Direct退让控制、receiver租约与Broker交换核心（§10.79）

**§10.79历史状态：软件合同及RouteBridge适配完成。§10.80已完成生产Broker driver/显式续期，见6.18；唯一物理决策和App Ready消费未接线，bit18继续不启用。**
本节不改6.16的四整数预留回应，accepted仍不是Ready。新增控制只走既有authenticated Hub route topic，
不进入child BBP lane、不新建身份/凭据/持久字段/owner表/任务队列，也不以permit-join代替Direct。

#### 6.17.1 exact wire与旧会话隔离

canonical CBOR为12项：
`[1,5,operation,topology,childLogicalId,instance16,epoch,credentialVersion,incarnation16,challengeId,leaseMillis,controlId]`。
这是Hub route-control kind5，**不是App inner kind5**；前导array为`0x8c`，最大200 B。沿用route身份和uint32/非零/长度约束。

| operation | 方向 | challengeId / leaseMillis | 含义 |
|---|---|---|---|
| 0 Probe | Broker→Hub | 0 / 0 | 请求本次receiver挑战；不能令Hub连接或宣布退让 |
| 1 Challenge | Hub→Broker | 非零 / 0 | Hub以本地单调时钟记录issuedAt，待决最多5 s |
| 2 Decision | Broker→Hub | 非零 / 0～30000 | 正数要求有限退让，0取消exact承诺；不是普通executionWanted |
| 3 Yielded | Hub→Broker | 非零 / 1～30000 | 原样回显已接受Decision；仅在真实quiescent且持续不重获radio时发送 |

四种消息均须带**当前Hub route的非零16 B incarnation**，包括Probe。
Broker从既有、当前MQTT/admission验证过的普通Execution Challenge取得它；Hub两个lease共用该route nonce，但挑战计数与用途独立。
不能仅用普通Probe的空incarnation：旧MQTT延迟Probe可能撞上新会话复用的controlId，既污染高水位，也破坏请求时间锚证明。
共享nonce不是共享权限，不产生新secret；旧kind4仍严格只有11项/op0..2，不能接受新kind5/op3。

`controlId`为非零uint32，在exact Hub incarnation的存活期内单调递增、不回绕/复用；不是ownerId或新凭据。
Broker driver必须把交换对象/高水位保留至该Hub authority真正退休，**不能因owner结束、child Hello/closure或可达投影重建而重置**。
更换MQTT/admission或Hub incarnation后旧控制全部失效；低编号、旧nonce、错route/epoch/credential及错方向均拒绝。
新Probe不能挤掉未到期pending challenge；相同Probe/Decision只能复用原tuple，不能建立新时间锚。

#### 6.17.2 期限、安全回执与有界发送

- Hub退让到期锚为`Hub challenge issuedAt + Decision.leaseMillis`；相同Decision不续期，zero先到则迟到positive不能复活。
  旧control的zero不能取消新承诺，uint32时钟回绕按有界差值处理；receiver核心48 B，无堆/无线/发布副作用。
- Broker以**首次Probe发送前的本地锚**保守计算`min(ownerDeadline, probeAnchor + Decision.leaseMillis)`。
  Probe确实先于Hub此次challenge，因而不需比较两端时钟。不能从Yielded收到时再加TTL；回执必须echo原duration，不是剩余时长。
- 首次Challenge及首次Yielded分别重新鉴权，并在异步结束后核exact owner token/revision投影、binding、MQTT、route和原期限。
  Yielded前不Ready；同tuple重复Yielded不重新鉴权/续期。撤销立即退休Ready，迟到成功也无效。
- 每阶段最多5次发送，间隔至少1 s；重试沿用原deadline和消息tuple，失败也消耗次数。Broker只有一个异步工作槽；
  不合作的HTTP/send即使超时，仍占槽至实际结束，不另起继任队列。取消最多排一个exact zero，丢失由receiver原到期收敛。
  **5 s是Probe/挑战及单次异步工作预算，不是整个原生断链的超时。** Decision接受后可在原保守承诺到期前等待真实关闭/回执，不能重开30 s。
- `pollDirectYield(quiescent)`只组装/发送当前Challenge或Yielded，复用RouteBridge原output缓冲。
  quiescent必须由唯一product决策在ordinary、management、permit和SDK异步closing全部结束后提供；
  `!secure`、零cloud需求、disconnect返回成功、Fault未结束或未扫描到广播均不成立。
  发出Yielded后，在该承诺失效前还必须阻止新management/permit/ordinary获取；只有当前没有owner而没有持续禁止获取，同样不成立。

零Decision/owner release撤销的是Hub承诺，不证明手机ACL已关闭。App仍须关闭自身作用域，child鉴权/空闲/DoS门禁独立。
§10.79的Broker对象只完成**单次有限交换**；§10.80已按6.18接通显式renew映射，不能用accepted或隐式延长旧Ready代替。

#### 6.17.3 实现与证据范围

设备`GatewayDirectYieldLease`组合原`GatewayExecutionLease`；旧/new codec共用严格编码骨架，RouteBridge复用admission、access验证和发送缓冲。
Broker `EdgeGatewayDirectYield`只消费现有owner的exact投影与注入的authority/send，不依赖BLE、数据库或第二套owner管理。
Host49/49、200k含新Direct有效seed的fuzz、协议22项+Broker70项、跨仓20项及Broker完整回归通过。
跨仓3项新增测试由真实Broker核心/Service codec调用C++ receiver进程，覆盖原时间锚、原生未释放及会话撤销；不是无线真机证据。
§10.79 时没有 product 调用 `pollDirectYield()`；§10.81 已接入物理策略，见 6.19，但仍没有部署后的线上 Ready。
不能以软件回执启用 bit18 或宣布 Direct 交替已通过。

### 6.18 生产Broker driver、版本续期和准入生命周期（§10.80）

复用6.17 wire/receiver，不增secret、topic、持久表或owner注册表。`getV2AppDirectReservation()`从原owner核心返回已确认版本的只读投影；
初次鉴权pending不投影，renew pending继续使用旧token/原deadline。确认新版本后以previousToken关联前一个版本，旧异步结果不能影响新版本。
投影的fresh Direct权限检查复用原全局64/每App会话4工作槽；非合作请求结束前不归还槽位，权限/binding/期限变化fail closed。

- `broker.ts`真实初始化已提供Direct投影查询；existing execution binding承载交换及单个1 s轮询。
  每次读取现有owner，不缓存继任者/新建队列；同revision交换耗尽后不自动重新开始，只有新显式确认版本可推进。
- 生产上行0x8c只允许已协商bit18的Challenge/Yielded，按exact route、incarnation、control/challenge/duration匹配；不进入child数据面。
  下行沿用原route publisher；发送/鉴权后重验同一current Hub claims和绑定。
  **owner.binding必须等于该driver捕获的exact Hub binding**；仅“同target当前有合法owner”不足以授权这个Hub。
- controlId高水位落在既有MQTT claims状态，强于只保留单次交换：同MQTT重新准入也不重用编号。
  准入真正退休更新进程内authorityId，旧binding永久失效；Hello/closure/可达投影恢复则保留authorityId和交换。
  同准入出现不同incarnation封闭Direct，不凭旧nonce重新启动；新准入可重建交换，但MQTT计数仍连续，溢出拒绝。
- 显式renew成功后，Broker旧Ready立即退休；稳定无在途工作的情况下，Hub旧有界承诺保留至原到期，同时开始新版本Probe。
  新版仍须新的Challenge/Decision和真正Yielded才能Ready。原promise不因新版accepted/收到回执或重复通知续期。
  新Probe阶段失败/release撤销此前可能生效的承诺；新Decision已发则以新zero同时覆盖旧promise，wire保持不变。
  renew与未完成HTTP/send相撞时取消旧交换，等实际工作槽结束后再读当前新版本；**可能有Ready空档，不保证无缝**。
- `edgeGatewayDirectReady()`只是内部exact token查询，不是App wire。设备尚未发真实Yielded，App尚未消费Ready，bit18依然不在生产Hello中启用。

证据：Broker/协议165项、完整Device V2回归、跨仓20项、Host49/49和架构通过。
新增真实App入口/HTTP适配/Hub route串联负测，包括child Hello/closure/recovery与同MQTT重新准入；BLE/publisher仍为夹具，不是四端真机。
本批设备及App业务源码不变，无MCU/APK/镜像部署、reset/清Access/改Key或暂存/提交/推送。

### 6.19 唯一 GATT 物理策略与承诺退休（§10.81）

`BasicGatewayGattRadio` 统一普通 execution、现有管理事务、permit port 和 SDK 关闭屏障；
ESP32 product 已删除 Admission 自动连接及断云无限持有分支。不新增 owner/任务/队列/持久字段或用户 API。
新管理准入在 mux 分派前动态求值，不仅依赖上个 poll 的快照；coordinator 判断 exact continuation，
不同操作/身份/版本/expiry 不能伪装为原工作，Prepare→Apply 保留原 monotonic budget。
新 Open 在 Opening 前被拒绝；已有窗口按原预算排空/Close。阻塞的下行仍使用 Broker 原 ledger 的剩余期限，不创建 MCU 队列。

- 普通获取需 Online + executionWanted + 无 Direct holding/管理/permit/native closing；
  已有管理事务可以在退让请求期间按原预算完成，但存在所有权时不能发送 Yielded。
- Yielded 仅在无任何逻辑 owner 且 child/port 均 Stopped 后发送；先锁获取再发回执。
  logical Idle、Fault 或断开请求成功都不等于 native 完成。
- **修正 6.17 的退休语义**：wire authority 退休不等于手机已停止依赖回执。
  MQTT/admission 更换后保留旧物理承诺的原剩余时间，新 wire incarnation 不能解除它；重复 reset 不延长。
  receiver 现为 **56 B**（原 48 B + 8 B retired hold），当前 exact zero 仍可提前取消当前承诺；不恢复旧 authority 或 receipt。
- 整机冷启动丢失 RAM，故第一次调度起有一次 `kGatewayExecutionMaximumLeaseMillis = 30000` 的获取保护，
  覆盖任一重启前回执；不写 Flash，不依赖 SNTP，之后 millis 回绕不重新触发。
  该安全等待只属于 Edge Hub 冷启动，可与登录重叠，不是每次连接/扫描等待。
- Hub 离线不创建新的普通或管理工作；原管理按原预算结束，普通链路立即请求释放并等待 native 完成。
  SDK 关闭卡住仍 fail closed。**手机离线而 Hub 在线持续有 cloud owner 的本地公平策略仍待测量/完成**，不能将两者混称完成。

Host50/50、跨仓34/34、200k fuzz/架构通过；GATT 100 轮为软件模拟，不是物理测试。
设备源码已消费 `pollDirectYield`；bit18 仍关闭，App Ready 消费未接线，未部署/烧录，后续见 GATT §10.81。

### 6.20 App Direct Ready 查询与原作用域消费（§10.82）

本节替代 6.18/6.19 中“App Ready 尚无 wire/消费”的历史状态；**并不表示页面已允许 gateway-child 抢连或真机验收完成**。
只增加 App↔Broker northbound 查询，设备 0x8c 控制协议、存储、Sketch/API 均不改变。

| 项目 | 冻结合同 |
| --- | --- |
| 协商 | App/Broker `DirectReady = 1 << 19`；必须同时有 bit17。Hub bit18 仍单独协商，本批不在设备 Hello 启用 |
| Query | inner kind `0x28`、flags=0；canonical CBOR `[1, ownerId, revision]` |
| Status | inner kind `0x29`、flags=IsResponse；canonical CBOR `[1, ownerId, revision, status, remainingMillis]` |
| 状态 | pending=0、ready=1、stale=2、denied=3；仅 ready 的 remainingMillis 为 1～30000，其余必须为 0 |
| 关联 | 原 Route/Delivery 的 LogicalDevice peer16、requestId，加 exact App claims/owner/revision；owner/revision 为非零 uint32 |
| 方向 | 只有 App 查询、Broker 回应；不是设备业务帧，不转发给 child，不复用 History 的 0x20/0x21 |

Broker 的查询只读现有 accepted Direct owner，不能创建/续期 owner 或重新启动 Hub Probe。
只有当前 exact Hub Yielded 为 Ready 时才调用 fresh Direct ACL，之后再检查 App claims、owner/token、binding、Hub authority 和期限。
返回 `floor(min(Hub conservative deadline, owner deadline) - brokerNow)`；权限/绑定异常或到期不得正向放行。
未 Yielded 返回 pending，错误 owner/session/revision 返回 stale，权限或工作容量拒绝返回 denied。
`edgeGatewayDirectRemainingMillis()` 替代仅返回 boolean 的旧 route 投影，避免消费者自行重建 TTL。

相同 requestId 在途只共享一次工作，body digest 不同拒绝；完成后的重复查询重新采样，**不能重放带剩余时间的 Ready 缓存**。
fresh ACL 复用全局64/每 App session4原工作槽，最多等待5 s；非合作工作实际结束之前不归还槽位。没有新 topic、队列、身份或 owner 注册表。

App 的 `DeviceV2DirectOwner` 复用 `DeviceV2DemandOwners`、单 owner timer、单在途请求与原 request quota：

- `accepted` 仍仅预留。`ready` 表示首次有效的物理退让回执，**不是已认证 BLE、Manifest/State 就绪或可以重试 Action**。
- 每个 accepted revision 查询 Ready；pending 回来后至少等待1 s再查，单次查询最多5 s。
  初次等待以原 acquire 发出起30 s为上限，即使每10 s renew被accepted也不延期这个首次等待预算。
- App期限为 `min(本次 owner deadline, 查询发出时间 + remainingMillis)`，不用收到响应的时间重新计时，不依赖两端墙钟同步。
- 续期开始即禁止凭旧 revision 新建 BLE；已经建立的链路只保留到上一物理承诺的原期限。
  新 accepted 只能延长预留，必须再收到该 revision 的新 Ready 才能延长物理期限；可能存在交接空档，不宣称无缝。
- `assertReady()` 在实际获取前检查当前 revision/原期限，防止 Promise 已 resolve 后 JS 挂起、取消或进入续期空档仍去抢连。
  `closed` 在到期/拒绝/断会话/取消时退役原调用者，丢失 release 靠原期限收口；不把旧工作搬到新账号。
- 原 `DeviceUiConnectionScope.hold()` 已能等待 Direct ready并检查，AccountClient/Service传递原 AbortSignal。
  **仍须在最终 BLE selector 接线，且保留原 Direct scope 的取消和断链责任**；priority release 不是手机 ACL 关闭证明。

软件证据：Angular正式测试入口274项、Broker相关151项、跨仓五套35项、Host50项及架构检查通过。
真实Broker入口/HTTP客户端/Hub route已串联，但无线回执为测试夹具；没有本批四端真机结论。
下一步先做原 transport selector/Direct scope接线与负测，补手机离线/Hub在线持续cloud demand的公平测量，之后再配套bit18激活。

### 6.21 实际 transport selector 与 native 准入（§10.83）

本节替代6.20及更早“selector尚未接线”的历史状态。实际页面与一次性read已消费Direct Ready，但Hub bit18仍未启用，软件测试不是四端无线证据。

- 受管child为inventory的 `deviceType=ble && cloudEnabled=true`；有凭据时先取得当前Direct owner/Ready，再完成认证/State后选BLE。
  Cloud不可达、广告nearby、旧状态或WiFiProv handoff不构成物理让位凭证；**删除旧cloudReachable=false放行规则**。
  纯BLE、普通WiFi/hybrid及Hub路径不改。尚无配套离线合同的受管child不能通过页面自动绕过Ready；这不是宣布取消离线直连目标。
- 原caller group共用一个物理Direct scope，page/read各自有生命周期，最后一个关闭才释放。新建group才允许新显式尝试；
  旧失败/取消不会自行抢连或把连接转到新账号。能力/权限/未发业务的连接失败可选择Cloud，Session退休不得跨会话fallback，未知Action不能重发。
- 原scope把中立准入检查传给BLE service/native link：新获取用 `assertReady`（当前revision和原期限），既有流量用 `assertActive`（原有效期限）。
  每个SDK connect前及每片发送/记录读取、通知均作即时检查；原owner timer仍负责到期，未新增第二套owner或定时调度。
- 取消不提前清除service opening；重复关闭等待原Promise；迟到SDK成功须再次关闭后才能归还原工作位置。
  generation隔离旧disconnect/notification，service/secure channel共享真实关闭完成边界，账号退出不能回写旧snapshot。
- 受管child后台PresenceKey维护只持久保存bundle，不自动开BLE；命令只用已经授权的连接，不能因expired/断线再ensureReady。
  显式Admin/入网/恢复事务保留原工作合同。普通page scope不是管理事务的所有权。

证据：App44文件382项、Broker151项及完整回归、跨仓35项、Host50项、架构/类型检查通过，见GATT§10.83。
仅App业务源码改变，无新wire、设备存储、用户API或部署；接下来先测量本地公平窗口及冻结离线获取条件，再配套bit18真机门禁。

### 6.22 现场测量后的扫描资源修正（§10.84，下一批先做）

本节优先于6.21及更早下一步。新Hub/App/Broker已部署，正常Cloud页面的wake→Button一次→idle→rewake→idle短测通过；
**这不等于Direct公平通过。** 隔离Hub抢连的10轮Direct测量第6轮被Android扫描过频拒绝，前5轮Ready2.845～3.859s；
App因依赖缺少onScanFailed只得到scan timeout。精确证据、固件/镜像/APK身份及资源见GATT§10.84。

#### 6.22.1 第一性边界：扫描不是连接权限

发现层只负责有界观察和物理扫描生命周期，不拥有Controller权限、MQTT owner、Action、页面状态或持久MAC身份。
授权仍是原账户/instance/PresenceKey匹配、必要的Ready和真实Method2；扫描成功/缓存结果不能代替它们。
失败类型需区分原生quota/busy/蓝牙不可用/权限拒绝/原期限到期/取消/成功扫描但未发现，不能全部折叠成“设备不在附近”。
扫描不可用时保留原nearby证据的自然到期，不刷新nearby，也不因本次扫描失败立即判定设备离线。

#### 6.22.2 S1：共享扫描生命周期与原生失败通道

1. 从 `ble-direct/transport.ts` 提取单一扫描资源入口，合并scanFor/scanForAll重复start/stop；上层仅保留筛选/收集。
   区分一次native scan与多个逻辑观察者，不为每个调用者创建新的scanner；兼容观察可共用当前扫描，非兼容请求有界拒绝/交接。
   不复制BLE GATT/认证栈或引入通用业务调度器。普通BLE、Presence、入网/恢复及工具页面都消费这个入口。
2. 每次物理start/stop有generation和真实完成屏障。取消发生在SDK start未返回、matcher未完成或stop尚未完成时，
   迟到回调只能结束自己的工作；旧stop不能停止后继扫描。所有观察者受各自原单调deadline、AbortSignal及前后台/账号边界约束。
   共享广告仅作有时效的发现提示，不持久化transport地址为身份，不延长原Ready/事务时间。
3. Android原生端补 `onScanFailed` 到当前generation的结构化失败事件；不能对已经resolve的request Promise再次reject后就认为通知有效。
   原生前后台生命周期需能停止扫描，即使WebView JS被挂起。不要直接修改node_modules当作可复现交付：
   先选择最小自有扫描bridge或可维护的依赖补丁边界，仅承载扫描，继续复用现有GATT插件。
4. 一并接管/互斥同App里的BLE debug、OTA和 `capacitor-wifiprov` 两处 `searchBleEspDevices`。
   WiFiProv可保留Espressif的协议SDK，但其物理扫描也须经过共用的准入/结束通知；只包裹JS BleClient不能宣称覆盖原生SDK。
   不修改WiFiProv Security1/端点或生产入网流程，不扩散UI重构。
5. 固定内存上限：观察者/待评估候选/事件缓冲均有界；连接前继续消费原Ready。
   不能为了减少startScan无限维持后台扫描，也不能换账号、取消、刷新页面就重置系统配额账本。

#### 6.22.3 S2：配额和原始时间预算

- Android的扫描启动限额是系统能力，不可由固件修复。AOSP所查实现默认30s/5次且厂商可配置；
  该数值仅作保守策略起点，不冒充系统剩余额度API或全机固定值。权威来源链接见GATT§10.84.3。
- 以实际物理start尝试记账，覆盖所有受管入口；取消/失败不擅自“退还次数”，系统明确拒绝时更新有界冷却事实。
  内存账本不代表进程重启后系统配额已清空，原生失败必须仍能正确上报。
- 允许共用仍有效的有界观察窗口；需要等待启动额度时只能使用原caller的剩余时间，并单独记录 `quotaWait`，
  余额不足立即返回明确限额结果，不重新开启5s/30s全新预算，不排队业务Action。
  **不保证短时间10次离散start都成功**：若产品合同拒绝第6次，测试必须验证精确拒绝和恢复，不把它改名为“10次连接成功”。
- 测量记录必须拆分排队/系统冷却、native扫描、广告恢复、GATT、Method2、State和关闭。
  不能仅在测试里加sleep后用成功轮耗时掩盖用户实际等待，也不能从5个正常样本承诺最坏公平窗口。
- 先软件覆盖共享/独立取消、start/stop迟到、scanFailed在start前后到达、跨账号/后台、窗口边界/厂商拒绝、
  WiFiProv与Direct交接、配额等待占用原预算、未找到与不可扫描区别，再生成APK实测。

#### 6.22.4 配套真机与剩余顺序

1. 完成S1/S2，仍用两块原绑定板和手机复测，保留失败样本。先10次基线及明确限额/恢复负测，不直接30/100。
   纯时间基线必须说明是否包含生产配额等待；不得把成功扫描数或只成功认证的轮数当作所有尝试数。
2. 测量真实Hub关闭完成→child恢复广播，随后覆盖在线让位和手机无云/Hub在线持续owner。
   原页面demand工具中的“离页仍同virtual session”假设需改为原绑定/状态新鲜/无重复Action/物理空闲，不恢复eager来迁就旧工具。
3. 据完整阶段证据在唯一 `BasicGatewayGattRadio` 制定并验证有界本地公平；冻结受管child离线手机的获取合同。
   无云或nearby本身仍不是Ready；不能恢复旧 `cloudReachable=false` 旁路。管理续作/permit交接/旧承诺仍用原边界。
4. 再成套启用bit18并执行正常页面Direct→Gateway→Direct的同绑定10→30→100、失联/撤权/取消及资源门禁。
   本批只验证短时Cloud物理wake/idle；多owner、公平交替、全release丢失、原生force-stop和发布长稳尚未完成。

### 6.23 S1/S2扫描基础已实现及下一步（§10.85）

6.22不是待从零实现的提案：共享扫描核心、Android原生失败事件/配额/后台回收、WiFiProv扫描端口注入、debug/OTA入口、
原期限与迟到关闭/匹配隔离及附近证据15s期限均已实现。模块、精确版本及证据集中在GATT§10.85。
扫描层不导入设备身份、MQTT/Ready或Action，外围发现不会自动获得连接权；仍由上层消费原权限和Method2。
原生配额限制的是实际startScan次数，不限制已建立链路上的数据消息。首次实现选择明确拒绝，不自动排队，也不修改设备侧发送策略。

验收边界：App266、Android配额2、硬件工具86项通过；新APK正常Direct5次、显式过频拒绝、独立恢复1次、
WiFiProv/Direct扫描互斥和旧stop隔离、真实Home后台停止、Cloud原绑定唤醒/释放均已通过。
第6次THROTTLED约50ms，不再错误等待5s；这不是10次连接全通过，也不是正常页面已取得Hub bit18/Ready。

接下来不重做scanner/owner/wire：

1. 补原生失败/重启恢复与WiFiProv按名称/QR取消及实际配网证据；当前扫码端口共享不等于完整配网流程重新通过。
2. 正常连接测量与过频负测分别验收；使用生产预算统计所有成功、拒绝和恢复，不删掉失败样本，不把后台/账号切换当成额度退款。
3. 测真实Hub关闭→广告恢复；用完整阶段证据冻结离线公平/管理争用合同，再实现唯一物理策略剩余项。
4. 最后成套启用bit18并做正常页面Direct/Gateway四端10→30→100；隔离诊断工具不是正常页面准入，不能替代此门禁。

### 6.24 扫描进程重启与WiFiProv查找验收（§10.86）

名称/QR查找取消、重叠Busy、资源交接和无匹配截止已真机通过；不是实际Security1配网完成。
进程重启真机证明系统可能保留UID扫描额度且只打印status6、不发送失败回调，因此修正6.23的进程内账本：
Android以单调时间/boot标记持久化最多5次扫描尝试和冷却，不带设备/账号/凭据，不给重启、后台或切账号退款。
同步commit失败不调用SDK；未知历史最多30s保守冷却。存储是本机资源防护，不是设备权限或云端限流合同。
本机5次实际扫描后第6次限额，立即重启仍约27.4s限额且未提交SDK；不宣称所有厂商静默限额都可提前获知。
失败回调/SDKstop异常软件测试与真实系统失败分别记账；不能把scan start resolve当作平台注册成功。
当前下一步和证据统一在GATT§10.86，仍按正常样本→SDK关闭/广播恢复→离线公平→bit18/四端的顺序。

### 6.25 SDK释放测量与离线公平实施边界（§10.87）

§10.86要求的物理测量已完成三个有效短样本，不再把它重复列为下一批的首项。
Hub真实GAP disconnect事件→手机收到**通过PresenceKey验证的新nonce广播**为约37～106 ms；
页面退出→广播1.024～1.156 s，其中页面到Hub断链约0.965～1.071 s。
这是包含手机扫描/回调交付的观察区间，不是空口嗅探得到的第一个广告时刻，也不是生产P99/最坏值。
在本次单连接夹具中，原SDK handle已不存在且扫描/连接过程均不活动的loop确认延迟为1.823～8.734 ms；
不是仅根据logical Stopped推断，也不是停止或释放整个NimBLE Host。
原始失败、时钟区间、版本和恢复记录集中在GATT§10.87。

#### 6.25.1 冻结职责，不增加一套协议或调度器

1. **本地公平是资源政策，不是授权。** Hub无法从云端推知附近有一台离线手机正在等待；
   没有第二条控制链路时，只能主动提供有界空档，不能承诺“离线手机一出现立刻让位”。
   广播只提供可尝试的机会，最终身份/权限仍由原account/instance/PresenceKey和Method 2决定。
2. 在唯一`BasicGatewayGattRadio`增加普通占用上限和释放空档。连接机制仍是`BasicGatewayChildConnection`，
   Port只报告SDK事实；不往协议层、路由层、owner聚合器各加一个重连timer，不增加NVS/wire/用户begin参数。
3. 普通扫描/连接尝试开始占用radio时开始计时，不等Secure才计时。page renew、新execution版本、
   同一普通连接反复重试、MQTT恢复均不能抹掉尚未兑现的让出义务。
4. 到上限先封新普通/新管理获取；原已准入的management/permit按原有限预算续作，不强拆Prepare→Apply，
   不因新事务排队不断延期。原所有权排空后退役exact attempt，等逻辑和native均关闭，再开始完整空档。
   关闭未完成不能按时间假装成功；Fault/SDK卡住继续隔离，不发Yielded，也不重新获取。
5. 云端Direct holding、退休旧promise、冷启动屏障与本地空档取共同约束，不相互提前取消，
   不重复累计已经真实空闲的时间。重复poll不延长空档；单调时钟回绕不重启旧承诺。
6. 空档结束只恢复获取资格；仍需现有fresh execution/管理权限。未知Action不迁移、排队、补发，
   Cloud ready/State投影在真实断链时退休，后续通过原Hello/Manifest/State恢复。

#### 6.25.2 手机无云不等于在线Ready失败后的旁路

- 在线受管child继续使用现有Direct owner→Ready→真实认证；pending/denied/ACL拒绝/限流不能降级为“离线模式”。
- 离线入口必须由调用侧明确拥有的前台page/read作用域发起，使用当前账号已保存且有效的本地Controller凭据，
  只选择本次扫描中经过授权匹配的Direct v3候选；不以MAC、名称、旧nearby或cloudReachable=false授权。
  此时没有Broker Ready，不能伪造Ready状态；须以同一中立admission接口表达独立的有限本地机会。
- 获取、认证和既有连接都有原单调期限，后台/账号变化/取消立即关闭；不得每个数据包或页面render自行续期。
  一次失败不隐式重试，不在未知Action结果后切换另一条链路；扫描THROTTLED保持明确失败和原预算。
- **只让Hub定时退让还不完整。** 手机成功Direct后也必须有界关闭并给Hub恢复机会；
  否则只是把永久占用从Hub换成App。先覆盖合作客户端，恶意持凭据central抢占/DoS仍属独立发布门禁。
- 离线端无法知道尚未送达的云端撤权；不承诺离线即时撤权。沿用设备本地凭据撤销/epoch合同，
  不新增共享的Gateway secret到App，也不绕过本地权限校验。
- 以上是**待实施合同**，本批没有开放受管child离线旁路。两端公平与负测配套后才能启用；
  普通纯BLE/普通WiFi/WiFi+BLE用户API不变。

#### 6.25.3 数值候选和有序退出条件

下一批工程夹具可先测普通占用上限 **10 s**、真实关闭后的机会空档 **6 s**：
6 s参考已有Direct Ready正常样本2.771～3.744 s与本次广播交付≤106 ms，并保留测量余量；
10 s是首次交接的交互预算候选，不是由三个样本推导出的安全常数。两者均**不冻结为发布默认/最坏延迟保证**。
应用获取总预算需覆盖等待下一空档、扫描和认证，不能把5 s旧扫描截止原样套上再用隐藏重试修补。
Android配额不足时没有“6 s一定能连上”的承诺；冷却不占用额外隐式队列。
原management/permit仍可能延长本次等待直到其原预算结束；不得把10 s普通占用候选宣传为所有场景的连接上限。

| 顺序 | 实施/测量 | 退出条件 |
| --- | --- | --- |
| F1 | 唯一radio的占用/排空/空档及Host负测 | renew不能无限占用；管理不被强拆；SDK慢关/失败/回绕/旧promise不越界；不新增Action重试 |
| F2 | 原page/read作用域的有界离线admission | 离线与拒绝分开；新鲜授权候选；获取与会话到期真实关闭；给Hub恢复机会；账号/后台/取消无残留 |
| F3 | 两块原绑定板实测持续Cloud owner+离线手机 | 同时记录等待/扫描/认证/State/关闭与Cloud失去及恢复ready，调整10/6候选；不得以清其他owner制造空档 |
| F4 | 配套bit18/正常页面在线交替 | 同绑定Direct→Gateway→Direct分阶段10→30→100；负测另记，扫描配额不退款，不以隔离工具代替生产入口 |

完整WiFiProv配网、SDK stop异常真实恢复、D4b/N2～N5和Wio/Hub发布长稳继续独立待验。

### 6.26 F1已实施、F2先补真实释放失败边界（§10.88，2026-09-08）

本节替代6.25“F1尚未实现”的状态；F2离线入口、F3离线手机和F4四端交替仍未完成。

#### F1：唯一radio的占用预算与机会空档

- `BasicGatewayGattRadio`新增两个单调时间戳和三个状态位，无任务/堆/owner表/持久字段。
  保留原唯一连接机制和management admission；删除普通分支中“只要executionWanted就能无限持有”的判定。
- 普通获取前开始预算；原生非静止状态以及management/permit原工作也纳入同一周期。
  10 s工程上限到达后封新普通/新管理工作，原exact管理续作仍由coordinator原预算决定；不按数据/renew/新版本延期。
- 原工作排空、exact attempt关闭、child与port都Stopped后才能开始完整机会空档。
  短暂断链不会重置预算；自然空闲或云端Direct promise已提供的完整6 s机会可以抵扣，不再额外叠加。
  stop/断云不抹掉债务，Fault/慢关不退款；clock wrap、当前zero及退休promise都不能提前重获。
- Host ASan50/50（含100轮Direct让出、100轮本地公平模拟及20万fuzz）；3.3.11构建总Flash +316 B、静态RAM +8 B。
  该RAM差值是完整Hub构建结果，不是宣称radio对象只增加8 B。

#### 工程数值的实测结论：机制有效，10/6不能定为成熟默认

原App普通Cloud页保持同一个scope，进入稳定后scopes=1、`executionWanted=1`；
三次完整“主动关闭→约6 s机会→重新Secure/Cloud fresh”已观察到，随后第四次主动关闭，再由离页释放需求。
没有关闭其他owner或发送Action，counter始终1。说明本地预算不依赖App停止renew才生效。

但页面每轮不可控期实测 **8.291/8.497/8.496 s**，不仅是6 s空档，还包含重新认证/Hello/State。
10/6下Cloud ready约仅占半个周期，不能从“成功让出”推断交互体验已合格。
代码保留这一工程候选用于下一步配套测量，**现场测试结束已恢复10.87正常固件**；不将候选打包发布。
下一步不能只放大占用上限隐藏问题：它会增加离线手机等待。须同时报告最大等待、Cloud不可控期、
扫描配额和真实SDK关闭，结合合作手机有限占用再决定取舍；若业务必须两端持续低延迟，需另立多连接/额外控制链路方案，不能靠单连接调度承诺同时可用。

#### F2a：先修已有关闭结果链，不能基于逻辑取消计算空档

审查发现`CapacitorBleDirectRecordLink.disconnect`、SecureChannel失败路径、Session和service逐层吞关闭失败；
`onClosed`本来就是立即退役逻辑作用域，不是SDK释放回执，不能用它开始手机让给Hub的6 s空档。
另外`service.disconnect`取消opening后原来未等其晚到连接收尾，也不能算真实释放。

已修复：

- SDK disconnect失败保持`BLE_DIRECT_NATIVE_RELEASE_FAILED`，Direct native transport进程内单一隔离标记阻止新link获取；
  换账号/页面/对象不能清除。停止通知失败不阻止继续执行真正disconnect，不把描述符失败等同ACL关闭失败。
- SecureChannel保留原业务首因，但`close()`暴露独立清理失败；Session仅成功关闭后通知release观察者，失败不清除上层资源证据。
- service保留失败closing屏障，不生成假的nearby/释放成功；取消opening须等待原Promise和晚到连接清理，不能把关闭失败改写为普通cancel。
- 没有新增管理/重连队列、SDK私有调用、失败自动重试或“清隔离”调试入口。
  这是Direct GATT关闭边界，不声称所有WiFiProv native资源也已过同样负测；SDK异常后的真实恢复仍独立待验。

新 APK 已安装并保留原 authority/账号/凭据；三轮独立 Direct Ready 2820/3789/3598 ms，
正常 record-link close Promise 完成 133/120/130 ms，每轮零 link/session；Cloud 原页随后 fresh/canControl、counter1，离页零 scope。
这是 SDK 正常关闭链（包含桥接/通知停止），不是空口精确时延或 F2b/F3 公平通过。晚到连接、关闭拒绝仍只有软件负测。

#### 下一步细化（替代不加区分地直接开放F2）

1. **F2a真机关闭/失败恢复与F2b作用域期限**：正常断链、晚到connect、关闭拒绝分别验收；
   限期依据真实完成，不能用`scope.closed`/`onClosed`。正常断链已由新APK三轮确认；晚到/拒绝的物理恢复仍待验。
2. **明确离线证据**：现有account `retrying`合并了网络、鉴权和配置错误，不可直接当离线授权。
   首版可只消费原生明确`connected=false`；状态未知/平台查询错误/WAN不通但网卡仍连通均不可凭猜测旁路。
   在线pending/ACL拒绝/限流继续禁止降级；已有本地Controller权限仍须完整Method2。
3. **F2b配对会话与恢复机会**：复用原page/read group、取消及中立admission，获取/既有会话分别有界，
   新页面/重试/后台/换账号不退款；只能在真正SDK释放后给Hub完整机会，不新增第二套owner注册表。
4. **F3再定工程数值，随后F4 bit18/正常页10→30→100**；本轮Cloud占用实验不冒充离线手机或四端完整交替。

### 6.27 F2a 真机负测收口、F2b 有限离线机会软件接线（§10.89，2026-09-08）

本节更新6.26后续顺序：**F2a已完成正常、受控晚到、真实SDK断开超时及进程重启恢复短门禁；
F2b源码/软件测试已接入；候选APK真实离线冷启动页面未通过，发现设备目录恢复缺口；F3持续Cloud需求仍待验。**

#### F2a：真实失败和恢复证据

- 保持原两板/账号/绑定，Hub Stopped/executionWanted=0；测试层只包装Capacitor桥，不修改SDK/生产入口。
- 晚到样本：SDK connect在932 ms成功，933 ms取消后才向调用者交付成功；两次幂等清理于995/1001 ms完成，
  原opening于1002 ms结束。没有Ready；Android连接列表确认目标已断开，零link/session/closing。
  这是受控桥接交付竞态，不冒充自然发生的SDK晚到；真实连接/清理均由SDK执行。
- 关闭负测：只将一次真正SDK disconnect参数设为1 ms；SDK实际返回`Disconnection timeout.`。
  App报告无秘密的`BLE_DIRECT_NATIVE_RELEASE_FAILED`，重复关闭和继任请求都被阻挡，SDK connect总数仍1。
  Android随后确认目标已不在连接列表，但这不清除失败屏障；保留1 link/1 session/closing是隔离证据，不伪装零残留。
- 真实force-stop后重开原App（不clear data），三轮Direct恢复Ready2910/3796/3653 ms、正常关闭121/124/123 ms、每轮零残留；
  原Cloud页fresh/canControl、counter1，离页零scope。证明本组合的进程重启恢复；不承诺所有OEM/蓝牙开关故障自动恢复。

#### F2b：唯一 caller group + 本地时间预算，不增加协议

1. **离线事实**：替换原NetworkService的空Cordova监听/无用依赖，使用已有Capacitor Network。
   只在原生明确`connected=false`、当前本地Controller凭据有效时选择离线分支；Web、未知、SDK失败、
   网卡已连接但WAN不可用都不推断离线。初始查询不能覆盖较新的native事件；失败后的迟到事件不能恢复授权依据。
2. **分支不可降级**：在申请Broker Direct owner之前决定分支；已经进入在线pending/denied/限流的调用，
   即使随后网络变化也不改成离线旁路。网络恢复或证据变成未知，立即退休原离线scope；不把未知Action切Cloud补发。
3. **仍是原group**：多页面/一次性read共用原`ConnectionGroup.direct`。仅增加一个无身份表的`BleOfflineOpportunity`
   管理本机时间预算，权限、account、PresenceKey/Method2、原SDK获取与释放继续由原模块负责；不增MQTT字段/owner表/Flash字段。
4. **配对工程候选**：单次扫描/获取预算16 s；实际native connect前首次`onAcquire`才开始10 s占用预算，
   后续尝试/流量不续期。两个预算按阶段使用：最迟16 s进入native后，仍最多再持有10 s，并受原scope总准备30 s约束。
   Timer只是唤醒机制，每次获取/收发都同步检查单调截止；后台、到期、换账号均取消原scope。
5. **真实释放后计时**：等原opening（包括晚到连接）结束，再按exact admission清理；
   只有成功后才从该时刻开始完整6 s Hub机会。关闭失败、已退休opening的native失败都保持busy，不能改成逻辑成功。
   新scope/账号/重试不退款；网络恢复后的在线Direct也不得跳过这段空档。失败不自动重试/排队。
6. **进程重启**：新进程的离线入口先留6 s机会，避免把RAM重新初始化当作离线立即抢占许可；
   既有Android扫描5/30 s持久配额仍独立执行，不修改或清空。在线Ready首入不受这个额外启动等待影响。
7. **exact清理**：BLE service的内部disconnect接受可选原admission引用；旧scope的晚到清理不得关闭同logicalId的新连接。
   `onClosed`仍仅代表逻辑退休，`onAcquire`仅通知实际SDK获取边界，不把时间策略塞进GATT协议或MCU公开API。

App33文件262/262、类型/local构建/Android assemble、硬件工具106/106、架构检查通过。
F2b候选APK生成后先因ADB掉线暂停，用户接回后已试装并执行一次真实离线冷启动：
关闭WiFi/移动数据、撤除本次API/MQTT reverse并force-stop/restart，保留CDP/账号/凭据。
原生offline=true、account=idle，但30 s内没有SDK connect，普通页面未创建，不能算F2b通过。
根因是DataService只恢复了auth，device目录初始为空，UserService.getAllInfo依赖在线当前用户/列表；
持久Controller凭据/Manifest并不能恢复设备目录。工具按原样保留失败，未临时拼设备或改cloudEnabled来过门禁。
测试finally已恢复两网络开关与原reverse，且覆盖恢复F2a已验证APK；Hub保持10.87正常固件。

#### 下一批：先补目录冷启动，再验原生离线页面/F3

- **F2b-C最小目录缓存（当前首项）**：新增按authority+account隔离、有schema版本/大小/数量界限的非秘密设备目录投影。
  只保存服务器确认的logicalDeviceId、deviceType、cloudEnabled、显示名和必要状态/共享角色引用；
  不保存DeviceKey/token/controller/presence secret、旧在线/Ready/State事实、完整用户资料或MQTT凭据。
  类型/cloudEnabled必须显式验证：不能从MAC、凭据存在或缺失字段推断成普通BLE，否则可绕过网关公平。
- 成功登录恢复后读取该账号目录供页面挂载；这不是重新鉴权。离线通信仍需当前本地凭据、新鲜PresenceKey匹配和Method2。
  不伪造loadGatewayUser，不把缓存当服务器成功响应。在线完整列表到达后原子替换，并删除已移除/撤销项；
  401/账号删除/换账号先退休旧scope和视图，迟到读取不得回填继任账号。目录缓存不得使无凭据的共享设备可控。
- 同批复核现有ManifestCache：当前key只有accountId+logicalId、缺authority；应复用统一缓存命名空间，
  不迁移来源不明的旧缓存，也不把缓存能力当已同步状态。目录损坏/版本不明/越界/跨账号/跨authority都保守忽略。
- 软件门禁先覆盖上述隔离、删除、损坏、无凭据、过期共享及迟到响应，再保留原账号/authority/绑定，复测普通页面离线Direct：真实关闭手机WiFi/移动数据，
  撤除本次测试的USB API/MQTT reverse（保留CDP forward），否则USB仍可能提供Cloud通路，不能声称真实离线。
  记录原手机网络开关，测试finally恢复；不清账户、不改SSID/DeviceKey、不发Action。先让Hub无持续需求，仅验F2b入口、到期、native释放和完整机会。
- 然后部署同一F1工程候选，设置独立真实Cloud caller持续需求；验证离线手机有限等待与Hub恢复，
  同时报告Cloud不可控期、扫描额度和最大等待。没有通过这一配对实测前，不能把10/6定为发布默认。
- F3之后才启用bit18，做正常页Direct→Gateway→Direct的10→30→100；D4b/N2～N5/WiFiProv/Wio及Hub长稳独立待验。

### 6.28 F2b-C 目录冷启动与独立离线页面短门禁（§10.90，2026-09-08）

本节替代6.27的“下一批先补目录”：**最小目录缓存、Manifest authority隔离及F2b独立离线页面已完成；
当前下一项为F3持续Cloud caller与离线手机配对。**Hub仍使用10.87正常固件、无持续执行需求；不是F3或发布公平通过。

#### 缓存边界与复用

- `device-catalog-cache.ts`只负责有界非秘密目录投影；DataService在线/恢复共用一个`applyDeviceInventory`，
  不伪造CurrentUser/API响应，不重做连接、ACL、页面渲染或凭据存储。删除未使用的旧布局字符串转换函数。
- 共用`deviceV2AccountCacheScope`和既有account storage prefix，要求真实auth.uuid；键包含完整authority+account，
  不从历史user.id获得缓存访问资格。目录envelope同时校验scope/version1；最多256项、128 KiB UTF-8，单条字符串有界、ID不可重复。
- 白名单只有logicalDeviceId、显示名、显式deviceType/cloudEnabled、设备state与共享shareId/role；
  不落token/DeviceKey/Controller/Presence秘密、完整profile、MQTT参数、State值、Manifest内容或在线事实。
  恢复后cloudReachable=null、enable=false、无State；没有本地当前Controller凭据仍拒绝离线连接，缓存role不是授权。
- 完整在线目录原子替换；撤销shared条目过滤、空列表覆盖旧目录。已有scope在设备被移除/禁用时退休。
  部分列表（例如shared接口失败）可展示本次在线结果，但撤下旧缓存，不把部分列表写成完整目录。
  越界/不支持的新完整快照也撤下旧缓存；存储失败保守尝试清理，不能用写失败后的旧目录声称最新权限。
- SecureStorage冷启动读取有session epoch fence；登录/换账号立即清旧内存，logout清本账号目录并保留原auth tombstone。
  在线目录同时检查session及请求代次；较早列表不能覆盖较新的删除，旧账号响应不能填入继任账号。
  在线确认uuid写回原安全存储并对迟到写入修复；BLE旧账号的snapshot/event不再更新继任视图或Manifest缓存。
- ManifestCache改用相同authority+account命名空间，来源不明的旧account-only key不迁移、不消费。
  Manifest仍只是能力缓存，fresh/State/Ready必须由本次协议同步取得；未收到离线撤权的既有本地凭据仍是原eventual边界。

#### 真机证据与观测修正

保留原两板/账号/USB authority/绑定。新APK正常Cloud页先同步、返回首页，再关闭手机WiFi及移动数据、
撤除本次API/MQTT reverse并force-stop/restart（保留CDP/安全存储/扫描配额）。
native明确offline=true；account显示retrying，**仍只使用native事实，绝不把retrying当离线证据**。

- 第一轮已恢复12项目录，正常页面Ready2990 ms；SDK connect一次、disconnect一次，关闭调用10472→10541 ms。
  立即查询Android manager连接列表仍有目标，工具严格未判通过；零App link/session/scope。失败原样保留。
- 检查已安装SDK：disconnect来自GATT disconnected回调，`getConnectedDevices`另查Android BluetoothManager。
  两者不是同一观察；测试工具改为最多4 s有界查询，不改SDK、不把SDK成功直接等同OS列表为空。
- 修正观察工具后，新一次真实冷启动普通页面Ready3318 ms，SDK connect441→1241 ms；
  到期关闭10529→10582 ms（53 ms），页面退休10731 ms，Android目标消失11666 ms，
  再观察6039 ms完整空档，没有第二次connect；counter仍1，零link/session/opening/closing/scope。
  这是OS连接列表证据，不是空口精确断开时间；不改变生产6 s从成功SDK释放计时的合同。
- `edge_gateway_offline_page_gate.mjs`只观察真实普通页面和真实桥接，无目录注入/ensureReady捷径/Action；
  检查显式受管child目录、当前凭据及精确实例，再检查native offline、到期、SDK/OS释放及完整静默。
  finally离页清理；外层测试保存/恢复原网络开关与两条reverse，失败不盲目重试。
- 恢复原网络后Cloud页再次fresh/canControl/counter1，离页零scope。新缓存代码未修改设备固件、凭据、服务器协议或数据库。
- 最终清理版APK再次真实冷启动短测：Ready3025 ms、到期SDK关闭55 ms，Android目标消失后空档6049 ms，
  仍单次connect、counter1、零残留；网络恢复后的普通Cloud页也通过。两轮成功是独立F2b样本，不是持续需求配对F3。

软件：App42文件357项、硬件工具108项、类型/local/Android构建和设备架构检查通过。
失败/成功证据分别`build/fairness-f2bc-offline-page.jsonl`、`build/fairness-f2bc-offline-page-bounded.jsonl`，
Cloud恢复`build/fairness-f2bc-cloud-recovery.jsonl`；完整执行与最终制品见GATT§10.90。

#### 紧接的F3，不扩散成新协议

1. 保持当前账号/凭据/child，仅部署已记录F1工程Hub application；先核对真实持续Cloud caller能形成executionWanted=1。
   Cloud caller必须独立于被断网的手机，不能用同一离线App或测试内存owner伪造。
2. 手机仍从普通离线页进入；同时记录Hub真实yield/恢复、手机native获取/认证/释放、OS观察、扫描预算和双方最大等待。
   第一次先做单周期配对；无证明不得推进10→30→100，也不开放bit18。
3. 必须报告既有10/6候选约8.3～8.5 s Cloud不可控成本与新增手机等待，不只报成功次数。
   若持续caller无法恢复、某方饥饿或关闭失败，停止并保留首因；不重播未知Action，不以长稳掩盖策略缺口。
4. F3通过再决定参数/bit18/F4；D4b、N2～N5、完整WiFiProv/Wio/发布长稳仍独立排期。

### 6.29 F3a首次四端配对通过，F3b预算阶段修订先于长循环（§10.91，2026-09-08）

**更新6.28下一步：F3a原两板+手机+独立Cloud caller已完成一次配对短门禁；F3整体性能/参数未冻结。
下一步先做F3b获取/占用预算分阶段，不能据一次配对通过直接启用bit18或10→30→100。**

#### 真实独立caller，不制造owner

- `edge_gateway_cloud_observer.mjs`复用App生产`DeviceV2AccountClient`、MQTT channel、readiness wait。
  在手机仍在线时通过既有账户credential provider签发一个**新的用户侧MQTT session**，仅传入电脑测试进程内存，
  不复制手机当前MQTT clientId、不取设备凭据、不导出用户access/refresh token、不写磁盘/日志。
- 电脑通过真实账户MQTT建立且仅建立一个execution owner，原runtime自动renew；只做Presence/Manifest/State同步，无Action。
  手机关闭WiFi/移动数据并撤除两条USB云reverse、force-stop/restart，保持CDP；然后从缓存目录进入真正普通离线页面。
- Cloud每次真实失去fresh时复用原readiness wait重同步，同一owner不释放/重建。
  观察器只消费状态，不注入Broker owners、Ready或设备generation，不新增生产协议/HTTP调试入口。
- Hub仅更新application为已记录F1候选（SHA起始a011d6a3），先读取MAC/分区确认正确COM11与app0 0x10000范围；
  child、NVS、分区表、DeviceKey/Controller/绑定均未动。原boot guard/管理预算保持；bit18关闭。

#### 首因与修正后的单周期证据

第一轮手机Ready10142 ms，Cloud失去fresh21675→35501 ms（13826 ms），随后37649 ms再次进入正常周期让出。
测试观察器当时只处理第一次恢复，导致在第二次让出时错报`GATE_CLOUD_NOT_RECOVERED`；
这是**测试观察器缺陷**，不能据此判设备恢复失败。失败原样保留，没有改设备算法伪造通过。
修正为每次真实loss重新等待原生产只读同步，且测试覆盖连续两次loss；状态摘要区分有用窗口与不可控窗口。

修正后的真实配对：

| 观察项 | 结果 |
| --- | --- |
| 原生离线手机等待→SDK connect | 7164 ms |
| SDK connect→完成发现 | 750 ms |
| 页面入口→Method2/Manifest/State fresh | 9948 ms |
| 到期SDK关闭 | 17276→17326 ms，50 ms |
| Android列表确认目标消失 | 页面观测起18414 ms；不是空口精确时刻 |
| 确认释放后持续空档 | 6060 ms，无第二次connect |
| Cloud首次不可控 | 20635→34394 ms，13759 ms |
| 恢复后Cloud有用窗口 | 34394→36664 ms，2270 ms |
| 第二次正常周期不可控/恢复 | 36664→45064 ms，8400 ms后恢复fresh |
| 持续执行及资源 | 一个accepted owner，wanted=1保持；MQTT account=ready，ipc0低水位1112 B |
| 业务/清理 | counter1不变、无Action；手机零link/session/opening/closing/scope，caller最终释放并关闭 |

上表手机自身ms与电脑观察器ms是各自单调时钟的相对值；只在同一时钟内相减，不据它们虚构跨端空口精确时延。
工具`edge_gateway_fairness_gate.mjs`复用离线普通页观测器并进行聚合；没有另建协议栈。
失败`build/fairness-f3-paired.jsonl`，通过`build/fairness-f3-paired-corrected.jsonl`；硬件工具112项和架构检查通过。
本批未改App/Service生产源码、未重跑上批357项App回归，不把历史结果称作新一轮软件/移动平台发布门禁。

#### F3b：分开“找不到设备”与“已经占住连接”，而不是只增加一个大常数

本次确证`GatewayGattRadio::noteAcquisition()`在扫描/连接之前开始唯一10 s预算。
Hub26638 ms进入逻辑Connecting（含原生扫描/发现），32999 ms Secure，34394 ms云端fresh，36643 ms即开始让出。
因此等待手机释放及认证花掉大部分预算，剩余约2.27 s Cloud可用；不是SDK crash、NimBLE版本或新鉴权失败。

下一批顺序：

1. **唯一radio内先分阶段**：获取阶段有独立上限，native获取后开始连接占用上限；先以App既有16 s获取/10 s占用/6 s真实空档作对照候选，
   不是发布常数。获取不是权限；管理原事务仍按既有有界预算排空。普通业务的总等待也必须有绝对上限。
2. **先确认中立阶段可观测**：使用IBleCentralPort的Scanning/Connecting/Discovering/Ready边界，而非等到Cloud fresh才起计时。
   必须审查同一poll中Connecting→失败→Scanning等短暂转换，避免漏记实际native尝试；若需要，只补每attempt粘性的内部阶段证据，
   不暴露SDK到radio、不新增Arduino用户API/云字段/owner表。不能凭一个晚到回调给预算退款。
3. **不退款规则继续保持**：失败重试、短空档、重复owner、MQTT重连、账号/新页面不得重置阶段起点；
   原生未关闭不算空档；真正逻辑+native静默满6 s才开启新周期；时钟wrap/boot guard/管理continuation均保留原测试。
4. **Host准入优先**：覆盖长期Scanning、晚到native获取、扫描后的完整占用、短暂失败折返、晚到关闭、management占用、
   reconnect/wrap与自然idle，不让扫描独占无上限，也不通过延长Secure状态绕开最大占用。
5. **编译/资源后复测同一四端短门禁**：记录手机获取是否仍在16 s预算内、Cloud不可控期及恢复后的有用窗口。
   改善2.27 s窗口不等于消除单BLE连接必须交替的13 s级成本；不得只报次数、不报不可控时段。
6. **完成F3b后再定参数/bit18/F4**；10→30→100、D4b/N2～N5/完整WiFiProv/Wio长稳仍独立，不提前打勾。

现场收尾：USB已接回；COM11 read-mac确认原Hub后，只写0x10000恢复§10.87正常固件（SHA起始4d6c522e，写后hash通过）。
手机两网络开关均为1、原reverse已恢复，首页账户ready，scopes/links/sessions=0、opening/closing=false。
**现场已改mostfun2026/电脑192.168.1.141；Hub基线含旧WiFi，当前wifiStatus=6、IP0.0.0.0、managementSubscribed=0，尚未联网。**
下批F3b真机前先同步忽略的本机网络/登录地址、Gateway public host与原测试CA签发的新IP叶证书，保持TLS校验；
不因本次网络切换重置Access/换Key/重绑，不把账户ready写成child恢复。恢复日志见`build/fairness-f3-restore.log`。
没有清数据、重绑、发Action、暂存、提交或推送。

### 6.30 F3b 获取/占用分阶段，保留唯一radio及原生边界（§10.92，2026-09-08）

用两个问题替代上一版同一个计时起点：**等不到设备多久必须放弃？开始原生连接后多久必须让出？**
这两者不等于Cloud fresh；鉴权、Manifest和State同步也必须消耗占用预算。

- 唯一`GatewayGattRadio`内获取上限16 s、原生连接占用10 s、单轮绝对上限26 s、真实空档6 s，仍为工程候选。
  获取从第一次工作尝试开始；Port首次Connecting/Discovering/Ready起占用，Disconnecting保守计入。
  到期表示请求让出并阻止新工作，不是保证SDK已物理关闭；原有exact关闭屏障不变。
- 先检查获取/总期限，再接受新的连接阶段；16 s边界晚到Ready不续命。
  失败、换attempt、短断线、新需求或MQTT重连不重置任何阶段；只有逻辑与native均Stopped且无人占有满6 s才开启新轮。
  管理工作从占有radio即计时，原来已准入的有界事务可排空，新事务不可串联延长；boot guard、云端Yield承诺与wrap保持。
- **无需新增Port API或粘性事件**：ESP32 Port先处理原生事件，再最多启动一个匹配candidate的native connect；
  成功提交后Connecting可被本轮radio观察，异步结果在下一轮处理。scan-cancel失败仍Scanning，
  同步connect拒绝没有获得ACL，仍受原16 s获取绝对期限约束；不是每次失败重新给16 s。
  `GatewayChildSession`与record link每轮只推进一次Port；普通路由/管理coordinator没有另一处child poll。
  这是当前ESP32调用链证据；未来Port实现也需满足阶段可观察合同，不能直接假定所有SDK都一样。
- 新Host场景覆盖长期Scanning、15 s后获取仍有完整10 s、晚到Ready、同步拒绝与短暂失败、
  短空档/MQTT恢复不退款、跨wrap期限；原100次云端让出及100次本地公平**模拟**和管理/关闭门禁保留。
  Host/ASan50项、硬件工具112项、架构检查通过。没有改App/Service生产源码、wire/owner表或Arduino用户API。
- 3.3.11最终编译Flash1,447,044 B、静态RAM65,628 B；比F1增加112 B/8 B。
  BIN SHA256 `8624a6c2fa6ed245c9bc15d6dac9822997386176c2a9b788b1136bad2eb5200e`。
  `fairness-f3b-{host-tests,tools,compile-final,flash,boot}*`保留本机证据。

配对工具继续使用同一个真实用户MQTT owner、普通离线手机页和原绑定，无Action；
新增“恢复→下一次让出”的完整窗口观测及再次恢复，避免在fresh瞬间结束测试而误报可用时长。
总门禁仍有绝对截止，USB/扫码配额等失败必须保留，不隐式重复设备命令。

现场网络同步只修改被忽略的配置：mostfun2026、电脑192.168.1.141、Gateway public host，及原测试CA签发的新IP叶证书。
Node HTTPS严格验证CA/主机名且`/healthz=200`；本机旧测试CA缺KeyUsage，Python3.13额外X509_STRICT检查拒绝，
没有为此关闭设备/服务TLS验证或改明文。该CA仍到2026-09-11，不能当作生产PKI；新叶证书到2026-09-10。
旧TLS代理保留为`blinker-device-v2-tls-proxy-before-f3b`可恢复。只有Hub应用区被重刷，原分区内容exact匹配，child未动。

#### F3b 真机结果与下一批准入

首轮因旧配置期望counter1、实际fresh State为0而在手机页面开始前fail-stop；日志原样保留。
child测试Sketch的`pressCount`为RAM变量、开机为0，与本次USB断电重接一致；没有为匹配期望发送Action。
明确将本次测试配置基线设为已读到的0，第二轮要求Cloud/手机全过程保持0，原logical child/instance/绑定不变。

| 观察项 | F3b 本轮结果 |
| --- | --- |
| 手机普通离线页Ready | 9,988 ms；SDK connect7289→8005 ms |
| 到期SDK关闭/OS确认/后续静默 | 17385→17443 ms（58 ms）；OS列表18517 ms确认消失；再静默6048 ms |
| Cloud首个不可控区间 | 20414→34251 ms，13,837 ms；同一owner/wanted=1保留 |
| 完整Cloud可用窗口 | 34251→41716 ms，**7,465 ms**，F3a为2,270 ms |
| 再次让出后恢复 | 41716→50426 ms，8,710 ms；fresh State/counter0 |
| 资源观察 | ipc0最小余量1104 B，loop3656 B，NimBLE2764 B；本次启动样本，非发布长稳 |
| 最终恢复 | 手机网络/reverse恢复；实际普通Cloud页fresh/可控/counter0，再返回首页scope0 |
| Hub空闲 | Stopped/wanted0、managementSubscribed1、WiFi已连接/IP192.168.1.144 |

证据：失败`build/fairness-f3b-paired.jsonl`、通过`build/fairness-f3b-paired-counter0.jsonl`，
最终Cloud页`build/fairness-f3b-cloud-page-restored.jsonl`，空闲`build/fairness-f3b-idle-final.log`；配置`build/fairness-f3b.config.json`。
普通Cloud页复核后的最终空闲heapFree105144 B、heapMin54856 B、largest49140 B；这是本次启动的峰值/空闲观测，不是长期无泄漏证明。
手机零link/session/opening/closing/scope；无Action、child烧录、Access清除、Key轮换、重绑或App/APK改动。
**现场保留F3b已通过短测的工程固件，不回刷含旧网络的§10.87二进制；bit18仍关闭。**

F3b软件与配对短门禁完成，16/10/6仅冻结为下一阶段的工程对照基线。约13.8 s的Cloud不可控成本仍在，
不能宣称无中断或产品级低延迟；SDK关闭异常、两端相位/弱信号与长稳仍需后续场景证据。
**下一批进入原F4，而非继续增加F3旁支：核对三端bit18准入及真实Yielded消费后，配套启用工程能力，
先一次正常在线页Direct→Gateway→Direct和取消/管理占用短测，通过再按10→30→100分阶段推进。**
不把离线配对短测冒充在线主动退让；D4b、N2～N5、完整WiFiProv、Wio及Hub发布长稳继续独立。

### 6.31 F4 在线 Direct 配套准入与真实发送路径（§10.93，2026-09-08）

本批保持16/10/6工程预算、原用户API和唯一radio，不新增owner表、凭据或wire合同。
只有内部`Esp32EdgeHubDualBoard`完整composition显式声明外层Hello bit18；普通WiFi/BLE默认配置和
synthetic child Hello不声明。Broker只对已鉴权的`edge-hub`协商该位，不能由普通设备自报取得让出权限。

实际接线必须同时满足以下四层，不能只凭单个状态机的Host测试判定可联调：

1. Hub外层Hello和Broker能力掩码一致。本批补齐命名位及profile准入，增加六种真实Hello lane组合回归。
2. Gateway实际容器支持`purpose=direct`，返回已确认Direct Admin的`directAccessEpoch`；
   App预约必须匹配当前Active topology的epoch。旧本机容器虽返回owner/read/command，却缺少epoch，
   首轮预约被正确拒绝。同步已有源码的两个编译模块，不创建新授权，也不放宽Broker判断。
3. MQTT路由的首次execution challenge可能早于外层Hello。Broker不得永久缓存当时缺失的Direct能力；
   仅在同一、未退休、未fenced的authority准入满足后创建一次驱动。新增晚Hello可恢复及fenced不能复活回归。
   该软件竞态已独立复现；不能把它单独当作本轮全部真机故障的解释。
4. `GatewayRouteClient`真实上行校验必须允许DirectYield Challenge/Yielded，拒绝Probe/Decision及畸形帧。
   本轮观测到Broker每批发送五次exact Probe、没有上行Challenge；设备发送校验漏掉0x8c合同，
   导致其被误当作ChildRouteEnvelope并拒绝。修正真实RouteClient→CloudMux→MQTT路径，
   不仅验证Bridge的假Route。新回归先在旧实现失败，再在修正实现通过。

测试工具提取共用的生产App账户SDK加载/新会话获取，保留凭据仅在进程内存；不使用设备MQTT登录、
不伪造Broker owner/Ready、不发Action凑counter。新在线门禁观察普通页的
Reserved→physical Ready→SDK connect→fresh State，正常离页后验证SDK关闭和OS连接消失，
并保持同一个独立Cloud owner完成恢复；第三次进入在Ready前取消，要求无迟到native acquire。

取消负测进一步定位：NavigationStart后父页/子页均已隐藏，但外层`DevicePage`还发起了一次不受页面取消控制的
`deviceUi.connect()`，与子页persistent scope叠加，直到Ready才自行结束。标准V2页删除这条重复one-shot入口；
组件名称解析抽成同一个方法，默认/显式/未知回退V2都由子页拥有连接，自定义页入口保持原行为。
同时`viewActive`使用同步setter取消，不再等待ngOnChanges；同步setter单独不能消除重复调用方，两个缺口分别验证。

最终软件验证：Host/ASan50、工具115、App正式Angular入口38文件374测试、Broker针对性95及完整V2回归、
Gateway编译和隔离PostgreSQL下access-grants/production-wiring通过。最初未配置测试DB以及容器默认production
导致的工具启动失败保留日志，不算通过；最终测试显式使用隔离schema和test环境，没有修改运行中数据库凭据。

最终Hub候选Flash1,447,100 B、静态RAM65,628 B，对F3b +56/+0 B；3.3.11 XIAO ESP32S3。
SHA256 `78c8e6ebd298b77570a2f979025eafc54bc109fcbeff9d1904ac0b055b14c863`。
仅COM11的0x10000应用区更新，写后hash通过；F3b实际Flash经ROM完整读回，与8624a6c2基线hash一致并保留。
stub连续读取在0x7a000附近中断，ROM读取正常；不能据此声称Flash损坏，也未改SDK或分区/NVS。
子设备、原账户/绑定/Access/Key均不变；App APK原位更新，旧6f146fe2包保留，没有卸载或清数据。
真机结果及后续准入见GATT§10.93，不用软件测试代替物理门禁。

### 6.32 F4原管理owner与Direct独立所有权真机验证（§10.94，2026-09-08）

原permit-join窗口拥有radio，不因为另一个Direct caller预约或取消而被关闭。
本次用真实Open/Get/Close和普通页面观察，不新增测试授权、owner表或调度分支：

1. 原窗口Ready、实际扫描占用时预约可以Accepted，但不得Yielded/App native acquire。
   原owner发起精确Close、完成设备SDK释放后，仍有效的Direct才能Ready。
2. Direct先离页，原窗口仍Ready、expiry不变；同一个窗口由原owner稍后关闭，
   观察继续覆盖Close终态至少1 s；取消不能复活native，也不能中断其他管理owner。
3. HTTP Close响应可能晚于真实设备释放，门禁使用同一WebView的单调时钟，
   比较原Close请求起点而非错误地要求SDK必须等HTTP终态返回；结合真实wire与串口审计。
4. 本次覆盖Ready扫描窗口，未选择candidate（不产生新凭据/绑定）。
   不能把它写成grant/enrollment传输中、所有管理操作的物理故障矩阵已完成。

两项实测通过：正向Ready3254→SDK3561→页面6183 ms；取消146→页面回收209 ms，
原窗口继续Ready、稍后正常Close，结束后约3.78 s内无迟到Ready/native。两次串口permitState1/relayActive1。
同一Cloud owner/counter0；原管理预算与radio公平周期不被测试退款。详细时序/恢复成本见GATT§10.94。
本批最终Host50、工具122、Broker95及架构检查通过，生产源码/固件/APK保持§10.93组合。

循环门禁复用原在线页面工具，显式单阶段10→30→100；每次保留真实Cloud owner、
逐轮OS释放与Cloud完整重同步，最后再取消一次。失败原样保留、先复核再决定下一阶段。
10次已通过，30次前8轮完整通过、第9轮App退出正常但Cloud超过原30 s deadline，已停止，100次未执行。
新增观察只记录reachable/manifest/stateFresh/revision和白名单错误码，不能输出任意Store/exception/凭据；
先联合Broker投影与Hub代次定位恢复缺口，不能提高超时或清凭据后丢掉旧失败，再声称阶段通过。
循环不是命令压力测试（本轮不发Action），不是发布长稳或D4b加密存储准入。

### 6.33 续租计时器和旧投影隔离修正（§10.95，2026-09-08）

旧组合带只读Broker/Hub/account追踪的30轮，前29轮完整通过，第30轮App退出正常但Cloud超时。
caller超时前约21.08 s Broker owners已为0、Hub wanted0；不是finally release导致。
实现回归确认：timer稍早于nextRenew触发时，旧Cloud owner走Direct-only query后不再调度，
留下本地accepted假存活、服务器按30 s过期。计时器只是唤醒提示，单调deadline才是判断依据：
未到due时重排同一timer，延迟向上取整。不得提前renew/query或用新的owner/重试队列掩盖。

设备另一个独立回归确认：过期投影的有效下行不能拥有后继native/auth attempt；
先保留身份/access校验，再按inactive/secure/generation拒收，不调用fail关闭新连接。
两项修正不增加状态字段、RAM、wire或SDK接口。Host50/App376/工具122与构建/架构通过。
新Hub71e82b72（Flash+4 B、RAM不变）/APKee981b3e已原位更新，child/Access/Key/绑定不动。
短2和10已通过；30在第5轮跨MQTT换代恢复超时，100未准入。旧失败证据与因果边界见GATT§10.95，不把独立回归等同全部历史根因。

新失败追踪确认Cloud revision1～10持续accepted；Hub在Direct Yielded之后MQTT换代，
旧手机owner release不能跨新authority提前取消已经发出的物理承诺。设备`retire`保留原deadline是安全合同，
不应清空；到旧承诺终点后才重新Connecting，原30 s请求仅剩约0.31 s，恢复失败。
**未完成项是换代恢复预算/消费合同**，不是第二个续租丢失或SDK兼容性问题。
下一步按GATT§10.95四步：定向负测→分别度量旧承诺/native/同步→冻结分阶段恢复→完整30→100。
保留原普通readiness30 s，不静默延长/重试；若独立观察器继续测量，首次timeout仍必须计为失败。
正常路径与换代路径不能通过挑选测试时间或重新命名门禁来混淆。

### 6.34 换代定向测量与物理承诺分片候选（§10.96，2026-09-08）

先使用§10.95原固件/APK/镜像完成两次定向实测：真实Yielded后关闭exact Hub MQTT，
一次手机正常退出；另一次手机在页面Ready后仍持有ACL约5 s，并在同一承诺内再次断开新MQTT会话。
两次首次30 s请求均明确失败；不释放原Cloud owner，第二次显式只读尝试分别约2.208/2.470 s后恢复。
第二次两次断开都真实发生，没有借MQTT重连把旧承诺重新延长30 s。细节/剩余准入见GATT§10.96。

**执行决策：分开调用方需求租约与单次物理承诺，不放宽普通readiness30 s。**

- Cloud/Direct owner存活租期继续30 s，Cloud正常每10 s续租不变；wire最大物理承诺继续允许30 s，旧设备无需新codec。
- App根据已验证Ready的实际剩余期限，在其一半之前/原nextRenew二者较早处发起同一owner下一revision。
  以query发出时间为保守锚，不能用收到时间延寿。续租accepted不延长旧物理期限，必须收到新exact Ready；
  未成功续期则仍在原物理deadline关闭。兼容旧30 s及更短grant，无SDK/板型常数。
- Broker对新发出的GATT Direct物理承诺先使用**10 s工程候选上限**，仍受owner剩余期限裁剪。
  候选沿用现有普通native占用10 s量级，把失去cancel链路后的安全尾巴从30 s降到最多10 s。
  这不是承诺所有网络条件下总恢复一定小于30 s；必须经过换代/管理/持续持有与完整30轮门禁后才确认。
- 已发出的旧30 s承诺不缩短、不重置；MQTT换代仍retire旧authority并保留原承诺终点。
  不加旧session取消旁路、持久表、第二调度器或自动Action重试。
- 顺序为App自适应续租回归/部署→Broker新grant裁剪回归/部署→定向换代复测→短2/管理/10→30→100。
  尚未支持自适应续租的旧App不作为新10 s profile的兼容目标；不能先部署Broker再假定旧App每10 s续租来得及。
  该候选不修改MCU烧录、Key、绑定、Active access或已发出的物理承诺。

软件已通过Host50/App380/工具126/Broker98+完整V2。App dc755c23先安装，后部署Broker
`l2c-direct-f5-slice`；Gateway和设备保持原版。已对新组合实测单次及两次MQTT断线，
实际正Decision都是10000 ms；原30 s请求均成功（总测量18.779/21.246 s），没有第二次读请求。
授权前断线反例无正Decision/Yielded、无native连接，Cloud原请求恢复。
两轮普通页面各保持8 s，原owner均两次renew并获得新exact Yielded，native不重连；末尾Ready前取消也通过。
管理窗口正向/取消两项33.183 s通过；10轮完整166.793 s通过，原Cloud owner逐轮恢复、counter0、最后零连接需求残留。
随后完整30在457.509 s通过，30次均恢复、最后取消native0次，同一Cloud owner45次renew均accepted。
30轮Ready3.983～4.510 s、Cloud不可用7.108～16.252 s，是单radio交替成本，不宣称手机/Cloud无缝并行。
100及发布仍独立准入；自然换代与定向正承诺后断线的证据边界、旧失败与最终清理见GATT§10.96。

### 6.35 共享连接与多子设备修订（§10.97，2026-09-08）

需求不是网络连接：账户MQTT一条，网关MQTT一条；每child有限需求聚合到其物理会话。
普通网关页在线默认代理、局域网接入Ready后才切换，不因附近/旧凭据主动拆掉Gateway→Child。
关闭页面只释放其自身需求/订阅；其余合法调用方仍可保持连接。小MCU不被要求双BLE会话。
计划以共享Host/扫描与有界连接池代替单childradio编排；连接槽满明确busy/capacity，不周期性强拆活跃child。
原6.29～6.34的10/6交替候选不再是正常体验目标；它的授权、旧承诺、关闭与失败证据仍有用。
S1只改App默认选择，旧Hub周期/离线机会还在，不能提前宣布低延迟；完整边界与三端任务见共享连接方案。
S0/S1软件已完成：默认路由新13项旧版7失败→修正版全通过；App相关417项/Web构建、Host重建/50项与架构通过。
未部署或修改设备固件；下一项为共享方案4.1的S2纵向拆分与双child池，不先运行原100轮。

### 6.36 S2a上下文/共享路由端点落实（§10.98）

一个MQTT订阅＋逐child有限入口已实际接入ESP32 Product，两个软件上下文的admission和背压独立。
入口按当前身份记录选择，Bridge仍执行完整授权；更换/销毁一个端点不取消另一端点的投递，MQTT换代则退休所有Cloud入口。
未引入第二账户/Hub MQTT、原生调度器、协议字段或自动Action重发；50项Host/ASan与架构检查通过。
实际Port/持久记录仍单child；S2b目录/管理定位和S2c共享Host/双槽尚未实现，旧radio周期与离线合同未修改。
资源及最终编译证据见GATT§10.98；本批没有部署或新板绑定，不以软件双上下文冒充真实双BLE连接。

### 6.37 S2b-1记录目录/身份投影与凭据选择（§10.99）

实际Product初始/轮换凭据下发通过可从原记录重建的目录选择；无新账本、wire字段、secret缓存或未经认证的占槽。
墓碑/孤立轮换journal仍占容量；损坏/歧义拒绝，新增B不能覆盖A。identity投影在写入/清除尝试前失效，暖route选择不读取Flash，授权仍由原Bridge验证。
软件双记录隔离、重建和exact ACK回归通过；平台/原生仍单槽。控制消息的operation分派及多记录后端未完成，不能宣布S2b或双连接整体完成。
下一步特别处理墓碑清除后的Finalize/Supersede重投：未知operation不能进入任意空槽；先冻结幂等terminal合同，再替换真实管理控制入口。
不改变现有radio周期/离线承诺、SDK/MQTT所有权或Action重发规则，100轮继续暂停。

### 6.38 S2b-2a逐child管理owner与终态重投（§10.100）

目录先按exact身份/operation/epoch定位，再交原proof/revoke/renew协调器；Hub级permit保留一份，不是每child再开扫描/MQTT。
A事务、原生所有权或结果未消费前，B不能接管；准入关闭时仅exact continuation可继续，未知/歧义消息不能reset当前owner。
native控制回复仍绑定来源child mux，不能跟随可变current owner。原radio预算/异步关闭屏障继续保留，不把管理分派当双物理槽。
Finalize/Supersede在整个目录新鲜证明匹配旧authority缺席时返回既有Finalized/Retired，不挑选空槽或新增持久完成账本。
这是Hub旧authority清理证明，不是新的child撤销receipt；Broker继续验证原ledger身份/digest/TTL并接受exact重投。
损坏/不可读/歧义和同authority换operation拒绝；关联renewal journal阻断而不乐观ACK，交错前向恢复另行收口。
Host50、Broker完整V2、架构/发布检查及两份ESP32构建通过；实际仍单记录对/单BLE槽，无新增协议字段、MCU队列或部署。
S2b-2b多记录后端仍未实现，其后才是共享Host/2-child池；旧10/6交替候选和100轮暂停状态不变。

### 6.39 S2b-2b持久绑定容量与物理容量分离（§10.101）

实际Platform/Product拥有2对NVS记录和轻量凭据接收binding，但只有1个GATT活动context；已有0号NVS名称和codec保留。
绑定B收到存储ACK不等于proof通过、Active或可控制；B未配置control时返回WouldBlock，不抢占A、不新增MQTT/Host或完整协议工作区。
访问重置必须成功清完全部record/journal再清Hub root；中途失败不能跳过。故障测试补齐clear重试NotFound的commit确认，不借此宣称真机掉电已验。
Host/ASan51、架构/发布检查及ESP32两构建通过；下一批唯一Central Host/双Port/radio纵向实现，旧物理承诺与100轮暂停保留。
降低活动槽数不能隐藏持久记录或跳过重置；仅认识槽0的旧固件不能作为无处理的安全回滚。没有新wire/需求token/重试队列或部署。

### 6.40 S2c-1共享Central Host与回调生命周期（§10.102）

实际Product拥有唯一SDK Host，Port借用并仅关闭自身扫描/ACL；不能以停止空闲B取消A，也不随permit/route窗口反复初始化SDK。
固定2条登记不代表2个并发ACL：本批仍只有一个原生活动owner，A的Ready也占用；B容量忙返回WouldBlock，无新队列/驱逐或乐观Ready。
GAP/GATT context改为进程期不复用的opaque token，由Host查登记再借用Port；旧Port销毁、同handle/attempt复用和Host restart不能让旧ACK/数据进入新会话。
正在执行的回调未归还、原生扫描/连接未关闭时仍占容量；stop超时保留Error/WouldBlock，必须保留对象重试，不能强行销毁callback owner。
SDK reset广播到登记者，后续sync或延后完成不能掩盖错误；SDK关闭失败保留角色，不释放给Peripheral/WiFiProv，正常角色边界不扩散。
中央Port18场景、完整Host/ASan51、架构/发布和ESP32两构建通过；未部署或真机验证，不把这批称为双连接或无缝切换。
下一步分离SDK发起权与逐ACL占用→2个原生槽隔离→B的context/radio/管理纵向接线。旧需求/物理承诺、100轮暂停状态、单账户MQTT均保持。

### 6.41 S2c-2逐连接容量与上层作用域边界（§10.103）

Central Host的原生活动不再全局串行：扫描/发起仍唯一，CONNECT被loop消费后释放发起权、保留该ACL独立槽。
槽关闭/在途callback/SDK失败仅占自身容量；B扫描或Ready不能阻止A关闭，A也不能取消B。reset失效旧token，旧handle不得误断已恢复的新连接。
源码容量最多2且受SDK较小上限约束，无新增MQTT、需求表、wire字段或重试队列；52项Host/ASan和两ESP32构建通过。
这只是原生Port隔离，实际Product/radio仍一个child。普通GATT Ready不等于认证/proof/route Ready，App/Broker不得提前显示B可控。
下一步先按共享方案6.4拆逐child管理与Hub permit、目标slot admission；不能让A的管理/Direct承诺变成B的全局阻塞，也不能复制radio覆盖同一router的准入指针。
再接双工作区/有界radio与B业务链路，重新验证SDK堆栈和1Hub+2child真机。旧物理承诺和100轮暂停保持，S3/S4连续性仍未完成。

### 6.42 S2c-3a逐child管理与Hub permit作用域（§10.104）

子设备mux只分派自身proof/revoke/renew，归各自context；来源回包不通过全局可变owner。删除重复permit解码/轮询/reset和Product旧组装。
新child管理按目标child准入；permit有独立准入，不用A的Direct承诺关闭整个Hub的新管理。全局管理owner仍串行，exact continuation、结果消费和异步关闭屏障保持。
子mux reset不影响Hub permit；Hub整体reset只清一次permit。在线管理客户端或离线Product推进单个Hub tick，不随radio数量复制管理poll。
Host/ASan52、架构/发布和ESP32两构建通过，未部署。实际业务仍child0；3b继续拆permit的物理槽归属和目标child准入，借B空槽不能绕过A的Direct承诺连接A。
然后3c双业务上下文与S2d 1Hub+2child真机；不重置A来新增B，不改变单账户/Hub MQTT、BBP/2、secret或离线承诺。100轮暂停，S3/S4仍待完成。

### 6.43 S2c-3b/3c两活动槽与实际A四端短测（§10.105）

实际Product已接入两个context/Port/radio；共享MQTT/Host/permit不复制。逐child需求、Direct承诺、管理/关闭与公平计时独立，共享一个boot guard。
GATT pool借出物理空槽；满槽Busy，不默认释放A。目标child与借用Port分离：选择已有A时即使借用B，也必须验证A的授权Presence及准入并持有A逻辑预留到原生排空。
空闲借还不创建权限，不以MAC匹配；损坏/歧义/无法排除原Direct承诺时拒绝。未选定扫描在2 s后可让给等待者，原permit窗口和选定relay预算不续期。
Host52/架构/347文件发布/两ESP32构建通过；COM11仅app0升级、原A/Key/绑定保留，实际Cloud账户API的Manifest/State/Action/ACK及permit负测关闭通过。
结束owner已释放、Hub executionWanted=0。单A与扫描阶段ipc0最低剩余1104 B；不抵扣双child资源、故障隔离或无缝切换门禁。
现场APK仍是旧在线Direct选择器，不能将这次Cloud API短测称为S1普通页面验收。下一步先安装当前S1，再用COM3建立B（不重置A）完成S2d。
16 s搜索/10 s占用/6 s机会窗保留到S4；S3有限本地授权/WebSocket未实施，100轮继续暂停。

### 6.44 S2d真实第二child与服务器路由前置条件（§10.106）

S1 APK已部署，普通A在线页面Cloud Manifest/State/button/ACK和离页scope归零通过，无Direct抢占。
COM3 B经真实permit/relay/receipt/proof绑定Active，未重置A或Hub；但B数据面未通过。
Service原`cloudEnabled`只看DeviceKey，漏掉纯BLE网关子设备；当前已补owner/read/instance/share共用的有界能力投影及真实PG测试，待配套部署。
能力不等于在线，`credentialVersion/locator`仍null，不给B发设备MQTT凭据、不扩大分享ACL。

真实Gateway日志与源码确认：按Hub查询仍拒绝第二Active，Broker每Hub只存一份route/authority。
新增**S2e为S2d的前置修正**：权威有界route集合→逐child projection/generation/recovery/ACK隔离→三端软件门禁→配套部署→双child真机。
MQTT身份与物理连接可共享，child安全代际/状态不能共享；退役A不能清B，整Hub身份换代才全量退役。
禁止取第一条/覆盖旧route、清A/撤B或另建MQTT绕过单值设计。绑定保留，详见共享方案§6.5；100轮仍暂停。

### 6.45 S2e逐child权威与无凭据身份（§10.107）

一个账户MQTT、一个Hub MQTT；Hub内部有界child集合共享传输，不共享floor/recovery/Command/Direct/执行需求。
Service返回至多2条确定排序的Active元数据，不读secret；Broker精确校验当前Hub Key、child/instance/epoch/version。
撤权指向child+operation+topology，不得关闭兄弟child；反重放floor在同MQTT身份内保留，超出2个不同child需新MQTT身份，不静默覆盖。
无DeviceKey的BLE子设备由有效路由建立Broker身份元数据，凭据版本NULL，不具备独立MQTT登录能力；不存在的目标只得到请求级Error。
同一网关A/B普通页面、两native连接与并行ACK、短窗口A关闭B继续已真机通过。B仍受自身旧10s/6s周期限制；
两次跨越周期的控制前置检查未Ready，未发送Action，不能声称持续并行/无缝体验已完成。165 Broker测试、真实PG/Redis及3项硬件工具自测通过。

### 6.46 满槽/Direct/MQTT短测与连续性边界（§10.108）

真实两child占用时permit返回Busy、不驱逐A/B，之后双Action/ACK通过；A Direct物理让出Ready不阻塞B Cloud Action/ACK。
没有连接手机BLE，不能当完整Direct数据面。整Hub MQTT换代才全量退休两child投影，新身份恢复后A/B均重新Ready并可控；账户MQTT保持。
本次故障采样offline到两者Ready约9.6s，不是planned switch指标；旧10s/6s周期仍在，未完成无缝。
测试工具已逐child过滤与exact topology恢复，不再对共享Hub整个topic/所有route做单child故障注入。

现有Bridge还拥有child callback/BBP序号与Cloud映射。S3a必须抽取唯一child执行/需求模块，MQTT与本地adapter分别拥有会话authority；
不得直接复制Bridge、接第二个receiver或复用Cloud admission当本地权限。有限本地授权与Hub/child权限范围、到期/撤权绑定见共享方案6.7。
初版在线授权后有限离线继续，不承诺Hub冷启动离线认证；离线无法收到新撤权时只保证原期限内失效，不能声称云端撤权即时穿透断网。

### 6.47 通用LAN的需求与身份边界（§10.109）

WebSocket是普通WiFi/WiFi+BLE/Hub共同IP数据通道，网关代理只是目标扩展；不是每child增加一个MQTT或另建整个Product。
mDNS仅报告服务候选，不能产生Cloud/local authority或child物理需求；Hub可发现不等于child在附近/Ready。详见[通用LAN方案](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。

LAN session按caller持有限授权与需求，self/child逐目标检查；页面退出只释放自身引用，最后一个本地使用者退出关闭socket，账户MQTT保持。
Cloud会话退休只能清Cloud需求，不能清仍合法LAN需求；已知Key/ACL失效必须同步清受影响权限，不能将故障隔离当越权保活。
同child仍唯一执行/序号，ACK回原caller；切路只影响未来请求，未知Action不跨路双发。Hub自身LAN和普通设备LAN不依赖child绑定表。

当前Cloud终态Error还会结束整个Client/WiFi，SMALL只有两个transport、Client单鉴权provider；S3a须先修这些接入边界。
通用ArduinoClock无UTC；S3b需fresh challenge/boot/session与接收方单调期限，重投/重连不能延长。首次在线授权、已有会话有限离线继续保持，不扩散冷启动离线权限。
本批仅文档分析，没有新增生产通道或真机证据；S3c本地可用不等于S4持续切路完成，100轮仍暂停。

### 6.48 非鉴权路径隔离与安全保留（§10.110）

普通WiFi/WiFi+BLE的station/Cloud非鉴权Error现在只报告Degraded，Client继续推进其他transport；原生断开/非Online轮询统一清相应peer授权和control window。
不同transport同sessionId不串清，另一路仍能发送BBP。单provider/control注册拒绝不同owner覆盖、回滚只释放已取得的对象；尚非多通道权限分派。

AuthenticationRequired目前包含Key/device/session多种撤销，仍按AccessFault关闭整个现有Client，不得由路径隔离保留未知权限。
将来只有明确绑定scope/期限的本地授权才能独立于Cloud authority退休；同一Key代际已知失效时，相关LAN需求仍须撤销。
SMALL的3/3相对2/2实测Device结构+80 B、frame/组件存储不变，默认容量未改；Host/ASan53及架构检查通过，非真机/无缝证据。
下一批按LAN方案S3a-1b实际鉴权/control分派与组合容量、1c唯一child执行/需求；EdgeHub自己的错误/周期尚未变，100轮继续暂停。

### 6.49 按通道访问服务与会话退休（§10.111）

transport/auth/control按同一槽位原子绑定/移除，Runtime逐transport/session协商和分派；不使用全局provider/control fallback。Cloud不再声明BLE认证/管理能力；同sessionId的两个本地peer不共享权限，故障只关闭目标provider/control窗口，不reset兄弟对象。
运行期间不可重排注册，stop后才可移除；移动槽位同时移动访问服务，重新开始不沿用旧peer权限。旧全局setter和分步回滚已删除，BLE-only/WiFi+BLE正式消费。Host/ASan与三通道假体/ABI通过，尚无生产LAN或新的有限授权；DeviceKey撤权安全关闭不变。
当时下一批1c现已按6.50完成；EdgeHub需求周期仍未改，不得由access注册推导child应该保持连接。S3b域/期限与self/child权限、S3c实际LAN、S4/S5不变，100轮仍暂停。

### 6.50 唯一 child executor 与来源需求（§10.112）

活动child的连接、回调、南向BBP/2序号、单在途/最近重放与管理事务已归入唯一executor。Cloud route bridge不再拥有
这些状态，只把通过自身准入的请求连同owner回调提交executor。不同owner同时请求时保持单在途并返回Busy语义，
不得隐式排队、改序或跨路径重发；完成/重放响应只交原owner。

需求是固定来源位的并集：Cloud retirement只清Cloud位，Local退出只清Local位，最后来源释放才变为not wanted。
该逻辑尚未替换radio的旧周期/Direct承诺；S4仍须在聚合需求、management owner和已签发yield承诺之间作唯一物理决策。
native disconnect清共享会话序号和exchange，普通Cloud重连不得这样做。Host/ASan与ESP32编译/尺寸通过，详见GATT§10.112。

### 6.51 有限本地授权与需求的接缝（§10.113）

S3b-1已冻结一个通用local authority：self与exact-child共享challenge/grant和Noise profile，目标差异只由
`ILocalAccessTargetPolicy`校验。授权只决定某caller在限定期限内能否Observe/Control，不直接设置executor需求；S3c socket
认证成功后才取得Local需求位，socket退出/lease失效/Key或ACL换代只释放对应caller，最后来源消失才允许南向idle。

deadline从设备首次接受同一authorization起算，重连不续命；设备重启不恢复RAM lease。Hub的child grant必须精确绑定当前
logical id、instance、accessEpoch和topologyVersion，因此退役A不会授权B，也不能借共享MQTT身份扩大范围。设备核心与golden
已完成；Service/App签发消费和实际WS/mDNS尚未接入，S4去周期及S5体验门禁不变。

### 6.52 S3b-2a 有限授权跨端模块与目标退休边界（§10.114）

Service签发/HTTP工厂、App解码/HTTPS消费和共用golden已通过软件测试，但真实持久授权表和生产API尚未接线。
设备过期lease增加原槽tombstone，阻止在途旧挑战二次复活；在线续期容量与单WS多目标控制仍属接线前必补合同。
全局authorityRevision不能替代局部ACL/child撤权，后者需要精确退休授权/需求且不干扰其他owner。
仅有合法grant也不等于建立业务需求或设备Ready；nonce/BBP sequence/Action ledger依旧各自由原会话拥有。
详见通用LAN§13和Service LAN实施计划；先S3b-2b真实事务/撤权，再S3b-3/S3c，不能恢复旧100轮或提前声明S4完成。

### 6.53 S3b-2b1 持久权限不等于需求送达（§10.115）

真实PG已实现新鲜session派生的LAN authority、固定期限/幂等签发、目标ACL/拓扑原子退休与outbox，联合30项回归通过。
局部退休不升整个recipient代际；Key/身份换代才整体退休。没有新增南向连接或设备需求，未改App/MCU生产代码或部署。
下一步S3b-2b2补认证revision同步与exact退休ACK，接收方同时处理现有lease和在途旧grant；再接真实载体/多目标/满槽续期。
不能将数据库已撤权/存在待投递行当成设备已停止、B不中断或页面已Ready；不能恢复旧100轮。见LAN§14及Service计划§3.2。

### 6.54 S3b-2b2a 精确退休、既有租约与pending挑战（§10.116）

通用控制wire/HMAC和设备receiver已实现；精确退休删A授权并阻断旧pending挑战，不清B既有lease，不增撤销ID表。
其他pending握手要重新取挑战；exact控制重传不重复应用。控制session按设备运行而非MQTT连接持有，断网不重启接收状态。
ACK仅代表授权状态应用，不代表已执行Action可撤回或SDK断开；下一批2b把单在途持久投递/恢复和实际转发/需求对authority的消费接通。
Host/ASan与Service协议通过不等于四端已撤权/页面Ready；自然过期满槽续期仍需S3b-3。完整结果见LAN§15及Service计划§7。

### 6.55 S3b-2b2b1 持久命令不等于业务撤权完成（§10.117）

Service按recipient落库一个pending和exact ACK证据；未同步/待退休时阻断新签发，不重置其他既有lease。
Hello追加当前proofSession认证上下文，只用于恢复核对，不是退休ACK；普通MQTT重连保留MCU控制状态。
放弃旧pending、全局屏障覆盖、exact送达分别留证，过期不伪装成功。当前DB/Host门禁通过，实际转发和需求释放尚待同authority接线。
下一批有界worker/控制面消费、主动到期与millis回绕门禁；生产HTTP/WS/mDNS和S4持续切路仍未完成。详见Service计划§8。

### 6.56 S3b-2b2b2 重试预算不是新的业务交替周期（§10.118）

Service已实现持久attempt、退避与8次/当前proof上限；只影响权限控制面的故障重试，不让页面消息按6～30秒交替发送。
耗尽保留pending/签发屏障，接受迟到ACK；旧Hello/重启不重开预算，新证明恢复需库/服务自动接线，不允许要求用户擦Flash或改Key。
有界worker和清理复用既有机制，真实PG联合47项通过；实际转发/需求消费、主动到期和Broker→MCU仍未验证，详见Service§9。

### 6.57 S3b-2b2b2 控制载体不创建第二份需求/路由权威（§10.119）

Service/Broker共用原物理MQTT数据topic，0x36控制和组件帧分派；原PG唯一重试/签名ACK权威，不建Broker离线控制队列。
可信上下文、取消/容量、start/stop已接线，PG50项/Broker专项7项及既有165项回归通过，但设备产品消费与主动到期仍待实施。
这不是新路由切换窗口，不能以传输accepted标记LAN ready；下一步Hello/ACK队列、自动预算恢复及同authority消费，见Service§10。

## 7. 当前执行顺序与准入条件（§10.119更新）

1. S0修正文档入口；停止原100轮计划。
2. S1普通在线网关页默认Cloud代理，保留有界离线Direct；多调用方/取消/换号/未知结果不得跨路重发。
3. S2a/S2b/S2c与S2e已部署；A/B数据面、满槽permit Busy、A Direct物理让出/B继续、MQTT换代短测通过。剩余管理变更交错、SDK reset/资源在S4真机准入前收口；不复制MQTT/Host、不清A给B腾位，不用长循环掩盖旧周期体验问题。
4. S3a、有限授权/持久事实、认证接收、命令/ACK恢复、有界投递/清理和Service/Broker载体已实现；保留模糊AuthenticationRequired安全关闭。下一批2b2b2剩余产品队列/ACK、Hello/自动恢复与主动到期接线，再真机控制门禁；之后S3b-3安全socket/多目标/续期容量、S3c普通WiFi self发现/认证WebSocket，再同模块接WiFi+BLE/Hub及R4/Wio门禁、S3d有限离线。不冒充生产准入已完成。
5. S4补明确离线接管边界后移除默认强制交替，页面路径切换不重建child连接；旧承诺不能提前清空。
6. S5至少1Hub+2child体验门禁，分别报告暖连接延迟、冷恢复、容量与故障隔离；D4b/N2～N5/发布长稳独立保留。

## 8. 历史执行顺序（截至§10.96，不能作为当前下一步）

1. **L2c-0b2 northbound软件完成：**6.5的exact codec/协商/关联、当前claims、ACL/replay和取消入口已通过。
   §10.69配套实际账户MQTT→Hub控制与force-stop/loss门禁已通过；§10.72/10.73补页面作用域与真机验收。
2. **0b3a/0b3b1完成；0b3b2管理占用与恢复分预算软件已完成：**复用6.6～6.9，不再重做codec/owner/管理租约/预算。
   6.14已收口唯一连接机制，6.15/6.16已实现Direct owner和northbound预留wire/真实authority，6.17完成Hub控制wire/lease/Bridge与Broker单次交换核心。
   6.18已接通生产owner/renew/取消到Hub交换并修正MQTT准入高水位；6.19已在唯一 GATT 调度消费 executionWanted 与管理所有权，
   替换 Admission 决策并处理取消 wake/释放/MQTT 退休/冷启动屏障。6.20 已完成 App Ready wire、Broker新鲜投影和原owner/scope消费，
   6.21已接实际selector、native准入与关闭屏障。6.22已完成一致部署和Cloud物理wake/idle短测。
   6.23/6.24已实现App共享扫描/失败/持久配额，并通过名称/QR取消和重启负测；6.25已测真实SDK关闭/授权广播恢复，
   6.26已完成F1及持续Cloud需求短测，6.27补F2a真实负测/恢复及F2b接线，6.28补目录/Manifest隔离和独立离线页面，
   6.29发现F3a Cloud恢复后窗口仅2.27 s；6.30完成F3b分阶段预算及四端短测，完整窗口7.465 s，
   6.31完成F4 bit18配套、真实发送路径及标准V2页单一scope，最终在线两轮交替/Ready前取消四端短测通过；
   6.32已完成真实management扫描窗口占用/释放及Direct取消负测；旧30次恢复超时保留。
   6.33已修正Cloud续租timer与旧投影中断后继attempt；新短2/10通过，30在跨MQTT换代时恢复超时。
   6.34已定向证明旧30 s承诺的安全尾巴，完成App自适应续租＋GATT新10 s承诺候选和换代正反复测；
   持续页面、管理及完整10/30已通过，下一阶段维持原组合独立100；以GATT§10.96实测为准，
   不再重复扫描核心或用长稳替代缺失策略。
   管理事务不可被idle打断，未知Action不能被调度器重新排序/重试；SDK异步关闭真正结束前不能释放物理资源槽。
   **App作用域及真实前台page/read有界回收已按6.10/6.11通过；6.31/6.32的Direct退让短门禁已通过，长循环仍按阶段验收。**
   唯一调度入口已部署到内部Hub测试板；无长期 eager/demand 双配置，bit18仅本轮完整工程composition启用。
3. **L2c-1 idle/Direct与App消费接线：**页面进入/离开/后台/销毁和只读sync已有生产作用域，再完整验收唯一物理编排。
   完成同绑定Direct→Gateway→Direct、在线主动让出、无云时有界让出、多owner与management争用，
   并验证取消wake、杀进程/失联、撤权、重复/乱序release。持续page renew不能永久阻止Direct。
   Cloud物理idle已在本地短测验证；在线bit18工程验证见6.31，普通设备默认仍不声明，完整公平门禁前不开放产品或宣称production ready。
4. **配套激活与清理：**保持用户Sketch/API不变；在唯一编排入口切换后删除被替代的Admission自动启动逻辑，
   不长期保留两套eager/demand candidate模式。源码旧 Admission 编排已删除，新固件已替换现场旧固件；
   也不扩散多child调度、Mesh或History/Bulk。通过Host/三端软件后，再做真实四端门禁。
