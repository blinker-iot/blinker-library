# Device V2 实时查看与历史数据执行方案

更新：2026-10-11。**H1服务器标量State历史、范围读取和App只读查看已通过软件回归、自有WiFi设备及真实分享只读/撤权的限定真机短验收，合同与结果取§9.4～§9.6。** 已修正多位记录ID的排序缺陷，恢复并保留本机测试网络；其他Shard部署和长时间门禁未据此关闭，H2/H3/Bulk不自动启动。L1云端、L2b BLE及L2c普通ESP32 WiFi＋内部LAN profile的限定短验收已通过，COM3原固件/授权已恢复；原结果取§7～§8.4，普通LAN开关不变。H1不重开N1～N5、实时租约、网关或SDK工作，也不增加设备协议/Flash存储。最短交付说明、同源新ZIP及S3/R4 BLE差异编译已按易用性计划§10.7收口；下一步仅独立接收，R4新版实时例程实物保持未测。

## 1 目标和数据边界

用户进入设备页才产生页面专用实时需求，离页或 App 后台即停止续租并尽力 Close；异常断网和杀进程由连接生命周期及有限租约回收。退出页面不停止设备基础状态、自动化、告警、任务或独立历史采集。

| 数据 | 用途 | 页面退出后 | 可靠性和保存 |
| --- | --- | --- | --- |
| State | 当前事实、控制结果、既有自动化输入 | 按既有业务继续 | 原 revision、快照和增量合同 |
| Live Telemetry | 当前可见页面的实时数值和短曲线 | 释放该页面需求，其他消费者独立 | best-effort、丢旧样本、不逐样本持久化 |
| History | 状态变化记录、周期测量归档 | 独立政策继续 | H1为服务器确认时间；H2另定义采样时间，均有独立记录标识、保留期和查询 |
| Bulk | 文件、媒体、高频大对象 | 本阶段不实现 | 不借普通属性或实时样本扩展 |

同一个传感器可为几条数据路径提供共享缓存，但三条路径的生命周期不同。`onSample()` 不更新 State，当前自动化只消费已鉴权且新鲜的 StateSnapshot/StatePatch；不能让页面租约决定自动化是否有输入。

## 2 已有实现和本批缺口

设备已有 `realtime(minimumIntervalMs)`、`onSample()`、TelemetryControl/Status/Data 和固定租约槽；无租约不调用实时回调。默认 Small/Standard 为两个槽，允许 Cloud 与 Direct 需求隔离。回调需短小非阻塞，慢传感器自行刷新共享缓存。

App 已按页面可见和前台状态申请云端实时流。默认请求至少 1000 ms，租期 30 s，约 10 s 续租；Broker 聚合同设备观察者需求并逐消费者降频。实际间隔仍须按设备返回值展示。同流字段共同受所选字段最大 minimumInterval 约束；字段并集必须受设备/协议容量准入，不能把观察者数量当字段容量。

L1实施前UI只暴露active、间隔和当前值，缺少样本序号、接收时间及曲线。当时BLE本地实时入口尚未接通，LAN选中时实时也仍走Cloud，因此 **L1来源必须显示云端MQTT**，不根据控制通道图标伪称实时样本来自BLE/LAN。L2a补BLE入口后仅实际BLE租约显示BLE来源；当时LAN产品实时仍走Cloud。现L2c已接通实际LAN租约并完成§8.4的内部候选短验收，不能沿用旧Cloud映射或据此改变普通发布开关。

HistoryRequest/HistoryBatch仍是reserved experimental，无完整设备codec/runtime；H1不依赖这两个消息，已按§9增加服务器State归档和HTTP范围查询软件候选。`v2_latest_state`只表示当前值，不能当时序历史；App旧演示曲线与模拟配额已在H1页面中撤除。

依据：[原遥测合同](DEVICE_V2_DIAGNOSTICS_OUTBOUND_TELEMETRY_PLAN.md)、[规范消息集合](BBP2_NORMATIVE_WIRE_SPEC.md)、[App Telemetry manager](../../blinker-app/src/app/core/protocol/device-v2/telemetry.ts)、[页面入口](../../blinker-app/src/app/device/v2/device-v2.page.ts)、[UI port](../../blinker-app/src/app/core/device-v2/device-ui.port.ts)、[Broker 聚合](../../aily-blinker-services/services/broker/src/protocol/v2/telemetry.ts)。

## 3 L1 实时产品合同

### 3.1 生命周期和频次

仅订阅当前页面布局使用、可读不可写且声明 realtime 能力的 Property。普通开关、按钮和滑条继续使用原 Command/ACK/State；无实时字段不 Open。首版不增加新的频率选择、租约槽或后台采集框架。

- 请求通常为 1 s，若字段最低间隔更慢则遵从该能力；显示设备/Broker 确认的 effectiveInterval，而不是请求值。
- 30 s/约10 s续租沿用原链路。页面隐藏、后台、账号/会话结束立即停止本地展示续租并尝试 Close。丢失 Close 不承诺设备瞬时停发，设备 TTL 兜底。
- MQTT PING、用户仍登录或其他页面活跃都不能续本页面租约。多人查看时一人离开不关闭其他消费者需求。
- 拥塞不缓存过时样本，不进入 ReliableOutbox、不补发或追赶。暂时 RateLimited 不冒充 Close，也不关闭普通业务会话。
- 若通道/stream/epoch 更换，重新建立实时基线并清曲线，不跨不同流拼接序号或时间。

### 3.2 实际频率和新鲜度

底层 snapshot 传递 streamId、epoch、已接收 sampleSequence、设备 monotonicMs、App 接收 wall time 与 monotonic time、累计可观测序号缺口。设备 monotonicMs 是 uptime，不是 UTC；本机 wall time 只作接收时间显示，freshness 使用单调时间。

Opened 不等于有数据。无首样本显示“等待首次数据”；收到合法新样本后显示更新年龄；超过 `max(5000 ms, 3 × effectiveIntervalMs)` 显示“数据陈旧”。连接在线和数据新鲜度独立。重复/迟到样本、Status 或续租不刷新样本年龄。

Close、隐藏、reset 和失败终止必须停止展示，不能让旧值继续看起来实时。错误不改变普通 State；恢复使用新租约和新曲线。

### 3.3 有界曲线

图表作为页面展示模块，不写状态仓、服务数据库或设备 Flash。每字段最多300点、最多32个实时字段；数值必须 finite，bigint 只有安全转换时绘图，其他类型仍可显示合法当前值。序号缺口、缺字段、无效数值或接收空窗达到新鲜度门限后断线，不连成伪连续曲线；序号回绕跳过保留的0，不误记为缺包。可观测序号缺口还可能来自降频和发送压力，不等同网络丢包率。

横轴为本机接收单调时间的相对秒数，明确是本次连接的实时窗口，不叫历史 UTC。旧点淘汰，不扩成无限数组；无样本、单点和恒定值均有可理解的展示。

新鲜度刷新仅在前台可见实时流存在时运行一个低频页面计时器；隐藏、销毁和账号切换回收，不增加每字段计时器或每设备后台任务。

### 3.4 最小改动分工

| 模块 | L1责任 | 不扩展 |
| --- | --- | --- |
| Telemetry manager | 既有wire的样本元数据和生命周期清理 | 新消息、历史存储、每字段调度框架 |
| DeviceUiPort | 类型化元数据映射及真实来源 | 伪装本地实时已接通 |
| 页面展示模型和曲线 | 有界数据、缺口、新鲜度及SVG绘图 | 新图表依赖、全局遥测store |
| 设备/Broker | 复用现有采样与聚合、限流、Close/TTL | 无关服务器维护、SDK升级 |

## 4 历史方向和后续阶段

先实现服务器状态变化记录，再实现在线周期测量；离线记录后置可选。它们不是 L1 的依赖。

### 4.1 H1 状态变化记录

服务器对实际接收并验证的 State 变化追加保存，按设备/字段/时间范围查询。重连 Snapshot 是状态校准，不冒充用户操作；命令下发不是设备状态证据。首版只覆盖已有 State 字段，不包含纯 onSample 字段，不宣称等间隔采样历史。图表采用阶梯或事件时间线，不补造离线点。

H1复用Broker现有SQLite State事务追加独立历史表及既有用户HTTP，不另建PostgreSQL异步双写链。Gateway负责新鲜用户/设备读鉴权及真实Shard路由，Broker负责已采纳State的原子记录、范围查询与有界保留。记录稳定tenant/logicalDeviceId/key及kind/type、Manifest指纹，不用MAC、昵称或单独revision去重。完整Snapshot建立独立观测段；sessionId是登录凭据，不保证每次重启不同。当前解绑退休旧逻辑ID，重新认领使用新ID，不串联旧历史；刷新DeviceKey不切断原设备历史。含历史的跨Shard迁移首版明确拒绝，不能沿旧迁移包静默遗漏。

### 4.2 H2 在线周期测量

独立归档政策决定采样与上传，页面关闭仍继续；不占用户定时任务槽。建议常规60 s采样，缓慢/低功耗场景300 s；上传可每周期或有界批次，保存粒度独立配置，不默认把1 Hz实时样本全部落库。重复测量值仍可能是有效采样，不能套用 State 的相同值过滤伪称完整采样。

数据包含真实采样时间、received time、time quality及记录代际。没有可信时间不伪造UTC。补历史不更新当前事实，不重放触发自动化。保存与重试幂等，ACK只能表示服务端已持久提交；不能把MQTT publish成功当保存完成。

历史查看复用名称解析器，提供今天/近7天/近30天、有限点数及按类型聚合；布尔为阶梯，普通测量可min/avg/max，累计量处理重置和增量，不能统一求平均。未记录或离线间隙明确展示，不回退演示数据。保留期和配额取真实服务配置，不填写虚构容量。

### 4.3 H3 可选离线记录

需要离线或断电补传时再评审有限storage、批次游标、持久提交ACK、Flash磨损及断电恢复。WiFi自行上传；BLE网关可代同步；无网关纯BLE只能在手机连接时同步，若设备未记录则不承诺连续云历史。Small默认不加入Flash队列，Bulk仍独立未实施。

## 5 执行顺序和验收

| 阶段 | 当前状态 | 关闭条件 |
| --- | --- | --- |
| L1a 元数据与展示 | 软件验收通过 | 无首样本/实际间隔/旧序号/缺口/回绕/陈旧/关闭及有界曲线回归通过 |
| L1b 真机云端短闭环 | 限定验收通过，见§7 | 匹配App备份部署；真实设备样本、离页/后台停止、返回恢复及原控制通过，保留请求日志 |
| L2a 共用Direct实时与BLE页面接入 | 软件及ESP32编译通过，见§8.2 | 复用现有租约管理器、可选Hello特性、独立样本分流及页面换流/关闭回归 |
| L2b BLE原生真机短闭环 | 限定验收通过，见§8.3 | 真实BLE来源与1 Hz节拍、曲线、离页/后台停采、返回新流及原控制ACK/State |
| L2c LAN产品接入与真机 | 内部匹配profile限定验收通过，见§8.4 | 普通ESP32 WiFi＋LAN；真实grant/profile、来源/节拍/曲线、离页/后台/恢复及原控制通过，原固件/授权与测试配置已恢复 |
| H1 状态变化历史 | 软件、自有WiFi设备及真实分享只读/撤权限定短验收通过，见§9.4～§9.6 | 真实追加、范围与小分页、名称、错误/生命周期恢复及原控制；真实viewer访问及撤权后查询拒绝、旧数据清空；不代替全部Shard/类型/长期门禁 |
| H2 在线归档 | H1之后 | 真实时间、有界批次、重复测量和提交ACK |
| H3 离线记录 | 按需后置 | 独立资源/持久化准入；不扩大为Bulk |

L1b 先核对手机网络、服务身份、设备身份/Manifest和现有样本能力。服务无变更则不重复部署。优先使用已有实时字段的运行板；只有缺少合适字段时，才在已获授权的COM3专用板完整备份后使用现有公开例程，并恢复原固件/授权，不清其他板或账号。

短验收覆盖：首样本前等待、连续样本及真实实际间隔、曲线来源；正常离页和后台后不续租、有限尾包后的设备停止；返回页面新流恢复；普通控制真实ACK/State仍工作。300点上限、长时间陈旧、seq gap和回绕优先确定性软件验证，不为证明容量启动100轮或24小时测试。强杀/断网/撤权若本批未实测则保持未测，不以软件回归代称新硬件证据。

## 6 官方参考

- [Blynk Datastream设置](https://docs.blynk.io/en/blynk.console/templates/datastreams/datastreams-common-settings)：最新值、分钟平均和RAW历史策略分开。
- [Blynk带时间戳批次](https://docs.blynk.io/en/blynk.cloud-mqtt-api/device-mqtt-api/datastreams)：减少逐点消息开销。
- [米家字段订阅](https://miecosystem.github.io/miot-plugin-sdk/module-miot_Device.IDeviceWifi.html)和[历史范围查询](https://miecosystem.github.io/miot-plugin-sdk/module-miot_service_smarthome.html)：消费者订阅和历史读取分离，不据此推断全部固件退出页即停采样。
- [涂鸦DP上报说明](https://developer.tuya.com/en/docs/iot-device-dev/TuyaOS-iot_abi_dp_attention?id=Kd1m6unb25gck)：缓存补传携带发生时间，不把云接收时间当发生时间。
- [Home Assistant Recorder](https://www.home-assistant.io/integrations/recorder)和[长期统计](https://www.home-assistant.io/docs/configuration/long-term-statistics/)：状态记录与测量汇总分层，聚合遵从数据语义。

## 7 本批结果

### 7.1 软件与改动范围

- 元数据/端口/页面限定回归81/81通过；Angular页面模板及模型/曲线21/21通过。独立协议关联回归85/85是覆盖重叠的验证，不累加成业务测试数。
- 没有新增依赖、wire、租约槽或设备/服务业务实现。实时展示拆为页面模型与SVG组件，删除旧双缓存和页面Manifest/Layout/State调试summary；复用原名称、控制和租约链路，保留无关修改。
- 停止后不返回旧live值，回落普通State；普通inactive可保留明确标为“实时已停止”的有界旧曲线。正常离页、后台、销毁及换流清缓存。重复Status不刷新样本年龄；缺号、回绕、陈旧、晚包与300点上限有软件证据。

### 7.2 本机真实App与COM3短验收

原App APK、停止进程后的完整账号/请求日志、COM3全8 MiB Flash及本机服务数据均fresh备份并校验。新版App隔离构建后核对原签名、CA、HTTPS origin和普通profile，再覆盖安装；现有schema51 Gateway/Broker沿用原匹配镜像，只临时调整本机测试地址，没有迁移或数据库回滚。COM3使用含N3/整数step的已安装候选库与未改动的公开[CloudSmokeTest](../examples/CloudSmokeTest/CloudSmokeTest.ino)，SDK3.3.11，实际bin947,600 B小于实物app0容量1,310,720 B；其他板未刷写。

| 检查 | 真实结果 |
| --- | --- |
| 当前值、实际间隔、来源、曲线 | 真实`uptimeSeconds`在设备页更新，显示“云端 MQTT · 实际1秒/次”和“刚刚更新”；手机截图核对曲线可见，无演示/注入样本 |
| 8 s被动观测节拍 | 去重9个样本、8个间隔；设备monotonic间隔全部1000 ms，App接收间隔最小/平均/最大994.10/1000.55/1011.70 ms；不能把轮询快照次数当样本次数 |
| 离页停采与返回 | 设备`telemetrySamples=37`保持15.490 s；返回使用新stream/epoch，从序号1与1点重新开始 |
| Home后台与恢复 | 两次观察分别58保持11.526 s、84保持30.438 s；实际模型`appActive=false`、timer关闭、展示/模型字段均为0；恢复使用新epoch和新曲线，不带旧点 |
| 普通控制不受影响 | 真实App开关false→true→false；独立Broker两条本批新增ledger均ACK200、一次attempt，候选内部State revision1→2→3。普通State只有power，实时uptime未写入State或触发归档 |
| 设备诊断 | 9份有限公开诊断均cloudReady；telemetryDropped、stateTransportErrors、operationFailures、leaseExpired均0。上述稳定窗口是显式Close后的停止，而非等到TTL过期 |
| 账号与请求日志 | before/after-update/after-test账号和既有3条任务请求、空自动化请求记录内容指纹一致；没有清账号、清日志或回滚数据库 |

后台WebView可能冻结最后一帧DOM，曾有一次只读观察超时；不能把不可见旧帧当仍在展示/采样。后台模型回收与设备计数均另有证据，前台恢复后新曲线已验证。

### 7.3 恢复与未测边界

COM3原NVS/otadata/app0范围`0x9000`起1,339,392 B已从本次完整备份写回并独立ROM校验，随后再次仅做该精确范围ROM复核及正常启动，原固件与授权恢复。后续全8 MiB RAM-stub读回在417,792 B处遇USB串口中断；失败轨迹保留，没有重复写入，**不宣称恢复后全盘逐字节读回已完成**。严格写入范围外未刷写bootloader/分区表，也未清其他设备授权。

临时Gateway网络、设备登录代理及本批ADB端口已撤回；匹配镜像保留，Broker实例未改，规则6条/执行14条与完整内容保持，未回滚数据。新版App保留，原APK与全量私有备份留在Git忽略目录；本批没有暂存、提交或推送。

L1a/L1b按本节有限范围关闭。300点/32字段资源上限、等待/陈旧/回绕及晚包主要是确定性软件回归；强杀、断网、撤权、多观察者和BLE/LAN原生实时不是L1批次新增真机测项，不能借L1结果宣称全部完成。历史仍仅为H1～H3方案，离线存储/Bulk未实施。L1关闭时确定的后续顺序为L2a→L2b→L2c，再另行启动H1；现有L2a/L2b/L2c结果取§8，不重复派发。原内部交付说明/独立接收待办保持独立。

## 8 L2最小实施范围（2026-10-11）

BLE和LAN已共用`DirectDeviceSession`，设备已有按transport/session隔离的Telemetry租约。第一性原理下，本批只补未接通的协议适配和页面生命周期，不另起BLE采样框架，不改wire、密钥或设备槽数。

- 共用Direct会话复用`DeviceV2TelemetryManager`；Hello可选请求已有FeatureTelemetry。旧peer仍可普通控制，但未协商实时时明确不支持。Hello当前没有租约槽协商字段，不新增臆造字段或容量承诺。
- Observe权限、已认证会话及已接受Manifest是准入条件；Control沿现有单请求队列发送，flags为IdMode，响应Status严格关联序号。Data及非响应Status独立分流，不抢Command/Manifest响应、不写State。
- BLE服务只使用当前ready会话；申请、Close、离页不得隐式扫描或重新连接。会话失效即reset管理器，停止续租和清样本；晚到Open或旧epoch不能写入后继页面。
- UI来源由实际承载租约固定。实际实时carrier发生ready→ready变化也必须释放旧租约并清曲线后重新申请；旧peer不静默云回退伪装Direct。L2a/L2b当时LAN仍沿L1使用云端实时；L2c在§8.4接通实际LAN租约，Cloud→LAN必须清退旧Cloud流，来源显示LAN。软件兼容回归不等于LAN产品真机完成。
- 一页一个实时carrier，共用现有有限设备容量；不为本批扩槽，不新增MQTT连接、不做网关子设备代理实时，不启动历史存储或Bulk。

先完成确定性软件回归，再核对当前ADB、设备身份、服务/网络及候选固件资源。L2b只做有限真机短闭环，先备份、后部署，并保留账号/日志；需要改变COM3固件时沿已批准专用板范围执行，其他板不动。软件与实物证据分别登记，未完成项不得沿用L1证据关闭。

### 8.1 已实现的最小链路

1. Direct Hello可选请求既有bit11；旧peer仍可普通控制，`openTelemetry()`显式返回不支持。能力提示不是权限证明：BLE ATT20旧路径的Hello在认证前，真正Observe权限来自Method2校验，数据和Status仍走既有安全records及设备端准入。
2. `DirectDeviceSession`复用原manager、唯一接收循环和有界请求队列；Data/异步Status独立分流，响应Status先校验flags/关联序号/结构。正常Renew/Close业务拒绝只退实时流，不触发BLE服务的载体错误回调；真实载体故障、超时和畸形有效响应仍终止会话。
3. Manifest失效时原租约立即本地退役，清样本和续租，并best-effort发送原Close；晚到Opened也只清退远端租约，不重启本地。Data先筛stream/epoch，再按租约固定的字段ID描述解码，旧epoch不对新Manifest解码。Close失败仍由既有TTL回收，不承诺远端瞬时停止。
4. BLE服务只开放当前账号、当前ready会话及有效物理许可的租约入口，Open返回后再复核；晚到旧账号/旧会话租约清退。UI adapter固定真实来源，实际实时carrier的ready→ready变化也关闭旧租约、清曲线，代际隔离晚回调。L2a/L2b的LAN选中仍使用Cloud；L2c按§8.4改为当前已认证LAN会话的独立真实租约，不沿用Cloud流。
5. [BleQuickStart](../examples/BleQuickStart/BleQuickStart.ino)增加`uptimeSeconds`与`realtime(1000U)`/`onSample()`，显式传给begin；不主动report实时值，loop仍只有run。原power/button注册顺序和State路径保留，不新增依赖或采样框架。

### 8.2 本批验收范围

| 门禁 | 本批结果与边界 |
| --- | --- |
| 共用协议与UI回归 | 13文件258/258通过，包含旧peer控制、Observe准入、请求交错、异步Status、真实来源、晚Open、换Manifest及后台回收 |
| Angular页面/曲线 | 3文件59/59通过；与上一行页面/模型用例重叠，不相加当作独立业务数。DOM验收使用Angular unit-test builder，不用无Angular初始化的裸Vitest代替 |
| 类型与模板 | App/spec的tsc noEmit及App ngc noEmit通过 |
| 设备既有能力 | 当前源码重新构建后，Device V2/Telemetry/Local Access Transport三个host目标通过；architecture检查通过。不把host结果等同新BLE实物采样 |
| ESP32 BLE示例 | 当前repo源码、SDK3.3.11编译通过；Flash统计694,820 B、静态RAM36,000 B，实际bin694,976 B，小于COM3既有app0 1,310,720 B。编译前后源码指纹一致，不宣称已测运行时heap/任务栈 |
| UNO R4 BLE新版示例 | 本轮单次编译仍在依赖预处理，超过短批次预算后有界停止并保留轨迹；未报代码错误，但不称编译通过。旧ZIP/R4实物证据不代替这次新增实时字段的编译或实物门禁 |

本批仅修改App共用协议、服务/UI接入与回归、设备公开BLE例程和实施文档；设备生产协议/槽数、服务业务及SDK/原生插件没有改动，没有刷板、App安装或服务部署，也没有Git暂存、提交或推送。无关本地修改保留。ESP32编译证据在Git忽略的`build/l2a-ble-realtime-20261011/s3/result.json`，普通编译bin使用源码默认开发profile，不是可直接认领的本机匹配固件。

L2a当时的下一批为L2b，现已按§8.3完成限定短验收，后续L2c取§8.4。普通编译候选不能未经profile核对直接用于认领；软件、实物和正式交付仍分别记录。R4新增例程编译及H1保留独立待办，不因本节关闭自动启动历史、Bulk或R4/Wio整组复测。

### 8.3 L2b BLE原生真机短验收（2026-10-11）

本批只部署匹配App和COM3专用XIAO ESP32S3 BLE候选，复用既有Direct安全会话、Telemetry租约及公开例程回调。没有新增生产代码修改、依赖、wire或槽数；现有schema51 Gateway/Broker沿用，没有服务部署、迁移、测试网络修改或数据库回滚。COM11/COM4见证板未刷写。

**备份与匹配。** COM3身份经USB及ROM MAC核对，全8 MiB Flash fresh备份并ROM校验。App原APK和停止进程后的账号/请求日志完整私有目录归档、列出校验；同签名、相同CA/HTTPS origin的匹配新版APK以`install -r`覆盖安装，再核对已安装文件hash，不卸载、不清数据。验收后另保留停止进程后的完整归档。Android Keystore不属于目录归档，由同签名覆盖安装保留，不宣称它已被导出备份。

候选取当前冻结源码与SDK3.3.11，静态RAM36,000 B，实际bin698,096 B，小于实物app0容量1,310,720 B。P256公开验签材料与当前本机服务核对一致；不把源码默认开发profile的普通编译产物直接拿来认领。此处是构建资源统计，不是运行时heap/任务栈准入结论。

**授权隔离。** 原COM3 BLE授权与当前账号无匹配凭据。虽获准临时重置，本批实际未清除原授权，而只在私有冻结构建副本中将身份/授权NVS namespace由`bl_v2_id`/`bl_v2_access`改为`bl_l2b_id`/`bl_l2b_access`；该头文件其余内容与生产源码逐字一致，生产库的namespace和公开API不变。有限诊断Sketch保留公开例程组件、采样与控制路径，仅增加`onSample`调用计数和Serial诊断，不含自动清授权。真实App正常发现、认领新测试实例；不注入凭据或绕过Hello/Noise/Method2。原服务器绑定不改，验收后仅通过App正常移除本次新建的临时设备。

| 检查 | 实物结果与证据边界 |
| --- | --- |
| 实际来源、频率和曲线 | 显示“蓝牙直连 · 实际1秒/次”“刚刚更新”；真实`uptimeSeconds`及SVG曲线经手机截图核对。样本未注入、未以State或模拟曲线代替 |
| 8 s被动观测 | 去重9个样本，seq7→15、stream1/epoch1；设备8个monotonic间隔全部1000 ms，App接收间隔最小/平均/最大983.90/997.98/1026.00 ms。该窗口可观测序号缺口0，不把16次轮询快照当16个样本 |
| 普通控制ACK/State | 真实App开关false→true、按钮执行、开关true→false；Command11/13/14分别收到同序号ACK，独立State通知revision2/3/4。最后power=false、buttonPresses=1与设备诊断一致。ACK本身没有stateRevision，不混同两类证据 |
| 采样与State隔离 | 实时uptime只走Telemetry；普通State仍为power/buttonPresses。按钮经原Action链路，未向State写入实时值 |
| 正常离页停采 | 回首页后页面/Direct会话释放；设备`sampleCalls=82`在19.667 s窗口保持不变。此前未打开页面时调用计数0，不把BLE产品计数器的0当停采证明 |
| 返回页面恢复 | 首次返回新epoch2，从sampleSequence1、曲线1点开始，不拼接旧流或旧曲线，普通State同步恢复 |
| Home后台停采 | 模型appActive=false，实时timer关闭，展示/缓冲字段为0、样本为空、Direct释放；设备`sampleCalls=136`保持41.745 s，窗口超过30 s租期 |
| 前台恢复 | 再返回新epoch3，从序号1和1点重新开始；随后ready、实际BLE样本及普通State revision4正常。后台曾保留隐藏的session closed提示，前台恢复后清除，不宣称所有隐藏帧都无提示 |
| 账号、日志和历史设备 | before/after-update/after-test严格比较嵌套`.result`：原账号、既有3条任务请求及空自动化请求的完整内容指纹不变。最终只读清单与原17个设备的ID/名称集合一致，新临时设备已移除 |

设备诊断在退出过程中`telemetryDropped`累计至3，因此不宣称全程零丢弃。实时为best-effort；上述稳态8 s窗口的可观测序号缺口为0，停采由实际采样回调计数的恒定窗口证明。没有以重复Status刷新新鲜度、以轮询制造样本或用稳定旧值代替设备停采。离页/Home实测正常释放路径，不借此关闭强杀、异常断网、撤权、多消费者或TTL异常兜底门禁。

日志留存校验器本批修正了旧工具取错嵌套字段的缺陷，现有判定要求`.result`存在、自洽且完整内容相等。另独立复核L1原始`.result`证据确实一致；不沿用比较两个`undefined`的假阳性，也不重跑L1现场。

**恢复与收口。** 已从本次全Flash备份写回原NVS/otadata/app0精确范围：`0x9000`起1,339,392 B，独立ROM digest通过，恢复文件hash与原备份对应切片一致，随后执行hard-reset。未写bootloader/分区表；原固件与授权字节恢复，包括撤除测试namespace。恢复后没有再次测试原固件业务上线，不将esptool身份读取/hard-reset当作业务在线证据，也不宣称恢复后全8 MiB逐字节读回完成。

原账号/日志及17个历史设备保留，新版匹配App保留；测试新增的ADB reverse及CDP forward已按归属移除，没有全局remove-all。全部私有Flash、账号目录、APK、诊断和构建证据留在Git忽略的`build/l2b-ble-realtime-20261011`，不进入源码提交。本批无Git暂存、提交或推送，无关本地修改保留。

L2b按本节范围关闭。300点/32字段上限、缺号/回绕、长时间陈旧、畸形响应与晚包等仍取L2a确定性软件回归；L2b没有新增这些硬件门禁，也没有测试强杀、ACL撤权、MTU失败回退、多消费者、R4/Wio、LAN或网关实时。后续L2c实物结果只取§8.4；R4新版例程、历史及Bulk仍另列待办，不扩大L2b范围。

### 8.4 L2c普通WiFi与LAN实时接入

本批仅普通ESP32 WiFi＋可选LAN，不扩WiFiBLE、Hub、其他平台、历史或Bulk。设备已有LAN extension将有界WebSocket transport注入同一个Client/Runtime，按真实transport/session授权及回包；不增加采样器、MQTT连接、租约槽或凭据仓。App补当前ready LAN会话的Telemetry入口和页面carrier映射，复用Direct租约管理器、来源适配及有界曲线。

LAN实时只使用页面持有的已认证会话及真实Observe许可。Open前后复核账号/authority、当前entry/session、前台与所选carrier；晚返回的旧租约只清退。申请或Close不隐式发现、重连，旧peer未协商Telemetry时明确不支持，不以Cloud样本伪装LAN。carrier变化先退休旧流再清曲线，普通Command/ACK/State不变。

准入按现有`local-access/2`合同：设备当前DeviceKey version、instance、authority revision、proof session，以及合法Query/profile与exact Enable ACK同步。真实HTTP context取`local-access-context/2`、HTTPS及no-store；self目标为空、accessEpoch/topology为0，Observe=1、Control=2。Grant必须与挑战、caller、目标、profile、权限及期限逐项一致。15 s挑战窗口、最长300 s会话授权与实时30 s租约是不同生命周期，不混用、不手工写库绕过。

本轮候选显式使用profile2：HMAC双向握手鉴权，后续业务为明文BBP/2，不提供内容保密或逐帧密码学完整性。profile由设备声明并与服务/App核对，失败不静默降级。普通库及App源码默认LAN开关不变，仅私有匹配验收候选启用。App账户与grant获取继续HTTPS；设备本机登录沿既有显式明文测试profile，不把LAN开关变成全局安全策略改变。

先备份COM3全Flash、App APK/停止进程后的账号与日志、服务运行配置及存储；本机服务仅必要测试地址调整，不升级业务镜像或迁移数据。原COM3身份不能安全复用时，仅冻结构建副本的`bl_v2_id/bl_v2_net`改为`bl_l2c_id/bl_l2c_net`，通过真实App新建本轮临时WiFi设备，原身份/后台关联不改。测试结束仅移除此新设备，恢复原NVS/otadata/app0精确范围、ROM校验并复位，撤回本批测试网络与调试资源。

短验收记录真实LAN样本与截图、唯一样本序号、设备monotonic和App到达间隔；实际页面开关操作关联Command序号、ACK及独立State，核对设备回调。实际离页/Home后按`onSample`计数恒定窗口证明停采，返回新流清旧曲线；账号、完整请求日志及原设备ID/名称集合保持。软件/构建通过不代替实物通过。

**软件与部署准入。** 首版App有限回归5文件130/130及Angular页面/展示/曲线3文件59/59通过；前台网络代际修订版增补14条回归后为144/144，另复核既有发现/连接器48/48、Angular59/59及App/spec类型与模板检查。各组存在覆盖重叠，不累加作独立业务数。生产仅补`DeviceUiPort`的LAN入口、现有页面group网络代际清理及页面carrier映射，复用三个既有spec，未新增依赖、wire或槽数。首版APK79,975,753 B与修订B版79,976,789 B分别隔离构建、核对原签名/CA/HTTPS配置及源码/编译资产，再以`install -r`覆盖并核对已安装文件hash；没有卸载或清数据。B版仅私有local-device profile启用LAN，普通仓库默认仍关闭。两次覆盖前后各保留停止进程后的新鲜APK及完整私有目录备份，Android Keystore由同签名覆盖保留，不宣称被目录归档导出。

COM3全8 MiB fresh备份已完成并通过独立ROM digest。SDK3.3.11/jobs1候选编译及指纹复核通过，实际bin1,034,128 B低于实物app0 1,310,720 B，静态RAM56,704 B；已仅刷app0并ROM校验、核对启动前NVS未改及正常复位。这是构建资源统计，不是运行时heap/栈准入结论。现有schema51业务镜像沿用，仅临时修正本机明文登录/MQTT测试地址；其他Broker实例与6条原规则/14条执行保持。

**发现与修正。** 首版稳态、控制、离页及Home停止通过，但前台恢复遇原生网络generation晚到：旧LAN候选已取消仍占页面group，页面只恢复Cloud。原8 s全Cloud失败记录保留，不把它改称LAN通过。仅修正现有生命周期：当前前台、有有效页面scope且已有LAN候选时，每有效网络generation至多替换一个候选；等待旧准备及真实session.close清理后才接入，旧callback不能退休后继。清理不确定时fail-closed，不增加timer/轮询、新manager、同generation业务重试或Action重放。无WiFi进入页面后首次入网的自动候选创建不属于本批新增承诺。

| 真机门禁 | 结果与证据边界 |
| --- | --- |
| grant/profile及实际来源 | 当前DeviceKey/instance/authority/proof与Enable ACK同步，真实grant的17项元数据检查通过；实际Direct session ready、Observe/Control=3。页面显示“局域网直连 · 实际1秒/次”“刚刚更新”，真实uptime及SVG曲线经手机截图核对。元数据审计本身不替代物理会话证据 |
| 首版8 s稳态 | seq5→13，9个去重样本，设备8个间隔全部1000 ms；App接收间隔最小/平均/最大853.60/1000.43/1145.10 ms。仅该观测窗序号缺口0，不把16次页面快照当16个样本 |
| B版8 s稳态 | epoch8、seq19→26，8个去重样本，设备7个间隔全部1000 ms；App接收693.20/979.29/1248.30 ms。实际到达有抖动，不宣称每个网络包严格1秒到达 |
| 普通控制 | 首版Command10/13同序号ACK191/231 ms，独立State1→2→3；B版Command9/11 ACK195/182 ms、State3→4→5。最后前台恢复后Command11/13 ACK348/248 ms、State5→6→7及设备power true→false再次对应。ACK本身无stateRevision；普通State仅power，实时uptime不写State |
| 离页停采及返回 | 首版78保持30.666 s，返回新LAN epoch4、首个可观测seq2重建1点曲线，未捕到seq1；B版504保持26.490 s，页面/LAN owner释放，再返回新epoch10、曲线不带旧点 |
| Home后台停采 | 首版118保持58.451 s；B版两轮520保持72.783 s、603保持43.096 s，均超过30 s租期。停止依据为真实onSample回调计数，不用telemetrySamples或旧DOM的0替代 |
| 原生暂停回收 | B版一次后台CDP读取超时，未执行页面观察动作。第二轮在Home前仅被动订阅既有appActive，记录真实true→false→true原生回调；false时appActive=false、timer关闭、字段/曲线缓冲0、sample=null、LAN owner消失，前台读回后移除观察订阅。没有调用getState、定时轮询或注入生命周期事件 |
| B版前台LAN恢复 | 两轮实际entry/network/discovery generation均更新一致为2及3，session ready；最终新epoch14的8 s窗口seq12→19、8个样本、曲线11→18点，设备间隔1000 ms、App接收922.40/991.91/1100.70 ms，可观测序号缺口0。旧epoch12的79点未拼到新曲线；没有将Cloud流改标签冒充LAN，未捕到最终新流seq1 |
| 账号与请求日志 | before/after-update/after-test严格比较嵌套.result，原账号、既有3条任务请求及空自动化请求完整内容指纹一致；最终原17个设备ID/名称集合一致。仅正常移除本轮新建临时设备，没有回滚服务数据库 |

页面准备LAN期间可能短暂显示真实Cloud实时，LAN ready后退休旧Cloud租约并清曲线，来源随后才切LAN。后台WebView可以冻结，不把隐藏旧帧当成展示或采样证据。设备在关闭/换流期间telemetryDropped累计至6（最后恢复控制诊断），因此不宣称全程零丢弃；稳态可观测缺口0与实际停采分别有独立证据。实时仍为best-effort。

**恢复与收口。** 原COM3身份/授权没有清除，候选仅使用隔离namespace。已从本次完整fresh备份写回原NVS/otadata/app0精确范围：0x9000起1,339,392 B，恢复文件与原备份对应切片hash一致，独立ROM digest匹配并完成hard-reset；没有写bootloader/分区表、erase-all或刷其他板。不宣称恢复后全8 MiB读回或原固件业务上线重测。临时新设备已通过真实App正常移除，移除提示保留本地权限未确认的边界；候选本地namespace随后随精确范围恢复撤除，原设备后台关联未改。

测试Gateway网络、明文设备登录代理及本批ADB reverse/CDP forward已按归属撤回；Gateway恢复原配置与端口、沿用同一业务镜像，Broker及无关容器不改。schema51、原规则6条/执行14条及完整内容保持，无数据库回滚。原账号/日志保留、匹配B版App保留。独立离线关项校验已验证原17设备清单、两次新鲜APK/停止进程归档、原恢复范围及ROM/hard-reset证据、完整journal和网络/调试清理；实时报告另验实际样本与生命周期，不让资源校验替代业务验收。

全部私有Flash、账号归档、临时DeviceKey/网络输入、APK、构建与现场证据在Git忽略的`build/l2c-lan-realtime-20261011`；有限实物报告`realtime-evidence-1791708540076.json`与恢复关项`final-check-1791708591539.json`分别记录，不上传私有内容。本批无Git暂存、提交或推送，无关本地修改保留。

L2c按本节普通ESP32 WiFi＋内部LAN profile有限范围关闭。生产LAN默认启用、加密/逐帧完整性、无WiFi首次入网自动候选、强杀/异常断网/撤权、多消费者、最大曲线资源和长时间稳定性等不据本次短测关闭；R4/Wio、WiFiBLE/Hub/子设备LAN实时也未新增实物验收。R4新版例程及原内部交付说明/独立接收仍另列待办。H1由本次独立用户请求启动，取§9，不反向扩大L2c验收；离线存储和Bulk仍不实施。

## 9 H1 状态历史最小实施合同

H1只做服务器真正采纳的State记录、范围读取和App只读查看。无需增加设备消息、HistoryRequest/HistoryBatch、Flash队列或实时采样政策。软件实施及回归取§9.4，匹配部署与自有设备短验收取§9.5，本机网络恢复、必要排序修复及真实分享只读/撤权取§9.6；各批范围独立，不把软件fixture当实物结果，不自动提交推送。

### 9.1 记录事实与边界

- 仅完整合法StateSnapshot及合法连续StatePatch进入现有State持久化事务。首版归档标量Property（类型0～5：开关、整数、浮点、文字）的原key/kind/type及完整V2StateValue；字节与容器类型6～8不归档，不改变它们既有State接收行为。既有Manifest禁止Property类型9，不能宣传null状态已可用。每条标量state JSON有界4096 B，不记录Command、ACK、Event、Telemetry或App展示缓存，不把当前值反填成既往历史。
- Snapshot创建独立观测段，条目标为`baseline`（初始/重新观测）；Patch只追加type＋canonical CBOR实际不同的值，标为`change`。重复Reliable frame由原Replay ACK链处理，不重复记录。登录sessionId及revision可在重启后复用，不用它们单独作唯一键。
- 时间是服务器采纳时间`observedAt`，不是设备采样或操作发生时间。段落之间不补造离线变化，不画成等间隔测量；先采用事件列表。
- 纯BLE Direct及未到Broker的LAN本地状态不上传App伪历史；只有Broker收到的状态才可查。名称修改不改变历史身份，64位整数保留字符串、容器/字节不猜测标量语义。

### 9.2 存储、查询与鉴权

Broker独立模块建表，在`putV2DeviceState()`原事务中与latest State共同提交；历史失败不得返回已成功提交的State ACK。历史录入使用lane显式来源，普通内部数据库调用不默认创造历史。每设备最多10,000条字段记录，读取窗口最多30天；写入时清理该设备超龄/超量记录，查询按保留窗口过滤。闲置设备的超龄记录立即不可读取，物理清理暂在后续写入时执行；尚未增加周期清理任务，不承诺到期瞬间删除磁盘字节。该限制不是30天完整性承诺，高频State会先达到条数上限，禁止承诺无限保存或虚构存储配额。

公开只读`GET /api/v2/devices/:id/state-history?from=<UTCms>&to=<UTCms>&limit=50[&key=<原字段key>&before=<游标>]`。时间范围必填且不超过30天，每页1～50条；稳定倒序游标绑定设备、租户、字段和固定范围/上界，不因同时写入或相同时间戳重复/漏页。内部读取复用现有Gateway→Broker鉴权HTTP，单次响应有界，不新增消息转发协议。

返回`{message:1000,detail:{logicalDeviceId,from,to,items,nextCursor,retention:{maxAgeMs:2592000000,maxRecords:10000},recordedTypes:[0,1,2,3,4,5],timeBasis:'serverAcceptedAt',source:'broker-state'}}`。条目包含`id/segmentId/key/kind/type/state/revision/manifestFingerprint/observedAt/event`；`state`沿用既有V2StateValue。时间为半开范围`from <= observedAt < to`；响应最大256 KiB。Gateway从当前账户owner/有效share的读鉴权决定真实tenant/shard，读取前后复查权限、版本和路由，撤权或换所有者失败关闭；App不能指定租户或Shard。分享命令字段限制不擅自改写为历史读字段限制。当前无字段级读ACL，沿用设备级读语义。

含H1历史的tenant迁移在源导出、验证及cleanup事务内明确拒绝，返回`UNSUPPORTED_TENANT_HISTORY`；旧迁移ledger也不能级联删除新历史。不修改既有迁移包版本、不静默丢历史。后续确需迁移时单独增加可验证的历史迁移合同，不在H1同时扩散维护功能。

### 9.3 App与执行顺序

复用现有`device-manager/:id/storage`入口并改名“状态历史”，删除演示曲线、固定MB配额和猜测单位。首版仅最近1小时/24小时/7天、实际字段筛选、手动刷新及下一页，页面最多累计200条并提示上限，不自动轮询。字段名称/布尔标签复用现有解析器，只有key/kind/type兼容时使用当前别名；类型变化保留原key/type，不误贴新含义。

离页、后台、销毁和账号变更取消查询并清理页面数据，晚响应丢弃；查询失败不能冒充空历史，403/404清先前数据。无记录明确为“服务器尚未观测到状态”，不承诺BLE/LAN本地、离线或实时采样已归档。

顺序：原子记录与有界查询 → Gateway读鉴权/范围接口 → App真实只读页 → 有限软件回归 → 匹配版本备份部署后的真实State变化/不变值/重连基线/撤权及原控制短验收。软件回归需包含事务回滚、重连revision重置、Manifest类型变化、租户隔离、相同时间分页、并发新增、保留上限、账号/生命周期晚响应、名称兼容和真实错误恢复；不以测试fixture冒充硬件证据。

### 9.4 H1软件结果与下一批有限验收（2026-10-11）

**本节记录软件实施批次，当时尚未完成真机验收；后续结果取§9.5。** Broker独立`state-history.ts`负责原子录入、标量过滤、保留及游标；原lane仅显式传入snapshot/patch来源，事务失败清除暂定ready状态，不假ACK。Gateway独立service/HTTP adapter复用现有Bearer/shard客户端和设备读ACL；App独立协议校验与只读查询service接入原storage页，复用账号拦截器与名称解析器。没有设备库生产代码、wire、SDK、槽数、Flash或新依赖改动。

必要修正已纳入回归：Snapshot使用独立UUID而非可复用登录session；固定游标上界与同时间戳倒序ID；旧迁移cleanup失败关闭；64位整数使用十进制字符串；合法浮点`-0`在JSON归一化为`0`时仍接受，并保留原CBOR。历史API路由识别剥离query/hash后复用现有鉴权刷新流程，不增加第二套登录链。

| 软件检查 | 结果与证据边界 |
| --- | --- |
| Broker H1 | 新增18/18通过，包含真实SQLite事务、State lane、重传/不变值、重连基线、容量/范围/游标及迁移保护；不是已部署设备证据 |
| Broker交叉回归 | H1＋自动化observer＋App target lane限定组36/36通过，包含上行18项，不相加为独立业务数；原设备/迁移大回归及Broker类型检查通过 |
| Gateway H1 | 7/7通过，包含严格查询/响应、实际HTTP客户端请求、前后ACL/路由复核、断开取消及错误语义；HTTP fixture不是生产Broker部署 |
| Gateway相关回归 | H1＋名称metadata组12项通过，另1项需要`TEST_DATABASE_URL`的PostgreSQL用例跳过；Gateway类型检查通过，不宣称此PG用例已验收 |
| App真实接口/页面 | 正式Angular runner共8文件169/169通过，覆盖CBOR与DTO、查询服务、页面生命周期及实际service DOM、设置入口、鉴权拦截器、名称及管理服务；测试组有覆盖重叠 |
| App静态检查 | app/spec类型检查、Angular模板编译及限定ESLint通过；未安装新APK，软件DOM不代替手机截图或真实设备记录 |

复现入口：Broker与Gateway各自运行`npm run test:state-history`，新用例也已纳入各自常规`test:device-v2`入口；本批不据此宣称重新执行全部常规清单。Broker交叉组为构建后`node --test test-state-history.js test-automation-observer-lane.js test-app-target-lane.js`。App在仓库根使用现有Angular CLI：

```text
ng test --watch=false --include=src/app/core/device-v2/state-history.spec.ts --include=src/app/core/services/device-v2-history.service.spec.ts --include=src/app/sections/device/device-storage/device-storage.page.spec.ts --include=src/app/sections/device/device-storage/device-storage.render.spec.ts --include=src/app/sections/device/device-settings/device-settings.spec.ts --include=src/app/core/injectable/server.interceptor.spec.ts --include=src/app/core/device-v2/display-metadata.spec.ts --include=src/app/core/services/device-v2-management.service.spec.ts
```

原页面的演示数据、固定配额及猜测单位已删除。`CloudStorageService`仍被现有customizer/widget-chart使用，予以保留；不为本批扩成编辑器/旧图表迁移。后台/离页/换账号取消并清数据，403/404不得展示旧记录；读取最多200条后引导缩小范围/筛字段，错误不伪装无记录。

本批未部署App或服务、未操作硬件/数据库环境、未暂存/提交/推送，账号、网络及请求日志未改，无关本地修改保留。源码回归的fixture不作为“真实设备已经产生历史”的证明。

**软件批次确定的后续短闭环步骤已按§9.5执行，不启动H2/H3/Bulk。**

1. 先核对现场手机/网络、当前账号、已有自有在线WiFi设备及State Manifest。优先复用运行固件的普通开关/数值Property，不为了历史重刷COM3、不清授权、不补造以往记录。没有合适设备时再明确专用板备份/恢复范围。
2. 新鲜备份原APK、停止进程后的账号/完整请求日志，以及Gateway PostgreSQL、Broker SQLite与当前运行镜像/配置。构建并部署同一冻结输入的App/Gateway/Broker；Broker仅应用本批历史表初始化，Gateway沿用现有schema，不升级无关服务或迁移。不删原规则/设备，不以数据库回滚掩盖验收期间业务变化。
3. 通过真实App控制产生新State：验证初始/重连baseline、实际变化与不变值不重复、范围/字段/分页、名称与来源/服务器确认时间，关联原Command ACK和独立State。重连基线不展示成用户操作，Telemetry不入历史；没有真实State的BLE/LAN设备明确无云记录。
4. 验证离页/后台/返回与查询失败恢复；有真实分享测试条件时复核只读访问及撤权，缺条件则单列未测，不能用fixture关闭。保留完整账号/请求日志和实际截图、原State与历史行证据；仅清理本轮新建测试对象，必要临时固件/授权与网络按备份恢复，服务数据不倒退。

### 9.5 H1匹配部署与真机短验收（2026-10-11）

**本次自有WiFi设备的H1短验收通过。** 仅使用现有COM11，不新建设备、不刷新DeviceKey、不清BLE授权；COM3与R4保持不动。新鲜备份包括原APK、停止进程后的App完整私有目录与完整请求journal、Gateway PostgreSQL、三实例Broker SQLite及原运行配置。新版App以同签名覆盖安装并读回核对hash，没有卸载、清数据或回滚业务数据库。Android Keystore由同签名覆盖保留，不宣称目录归档导出了Keystore。

App/Gateway/本设备所在Broker shard1按冻结输入构建、部署并核对实际编译资产，健康检查通过；其他Broker shard0/2未升级，不能据此宣称所有Shard具备H1。Gateway沿用schema51，原50设备、6条停用规则及14条执行保持。App候选79,987,977 B，沿用原已安装的内部LAN-enabled配置，不改变普通源码发布开关。本轮临时关闭手机BLE，实际控制与State取云端MQTT；App账户及历史查询仍走原HTTPS。临时明文设备登录代理只开放既有两条设备POST，不新增认证策略。

**网络恢复与授权保护。** 原有效网络为已确认的`mostfun2026`，现场该网络不可用，旧profile才是“带翅膀的蜗牛”；已确认的失联网络不会自动回退。用户授权只更新WiFi后，完成新鲜8 MiB Flash读取及独立ROM digest校验。临时维护程序复用原冻结C3库的网络journal，通过完整显式串口指令只提交一次gen3现场配置；不开WiFi/BLE、不登录、不清NVS、无开机自动提交，不新增H1设备代码或生产依赖。

校验分两段保存：全备份恢复运行后、维护写入前，SDK已更新一次`phy/cal_data`；因此不宣称全备份之后所有非网络字节从未改变。维护写入前独立NVS快照与提交后严格语义比较，仅`bl_v2_net/wifi_0`和`wifi_sel`变化，14个namespace及40个受保护逻辑键保持，身份/授权namespace与全备份也一致，没有放宽例外名单。随后仅写回原app0完整3,342,336 B范围并通过ROM digest，恢复时NVS和分区表分别验证未改，保留获授权的新WiFi。未erase-all、写bootloader或分区表。原固件重新登录及重启后恢复MQTT/State均实际通过，DeviceKey仍为版本1；正常短session到期不是Key失效。Gateway warn日志条数不用于推断请求是否到达。

| 真机门禁 | 结果与边界 |
| --- | --- |
| 初始记录与不回填 | 部署后历史为0；原固件真正上线后才追加2条baseline（power=false、buttonPresses=0），不是旧latest State回填 |
| 真实变化与原控制 | 真实页面开关false→true→false及按钮动作使计数0→1，独立State修订1→2→3→4；新增3条change，历史累计5。三个命令均有App ACK、设备侧ledger acknowledged/result200与独立State对应，不以ACK单独替代状态 |
| 同值控制不造史 | 另以现有生产command接口发送一次当前false，收到新App ACK，设备侧第4条命令同样ACK200；独立读取确认历史仍5、latest修订/更新时间不变。该请求非DOM点击，未伪造State；原设备会抑制同值report，因此本门禁不冒称Broker实际收到同值Patch后去重，后者取§9.4软件回归 |
| 重启基线 | 不刷写的真实重启后恢复MQTT，修订回到1、计数回0；新增2条baseline，累计7条/2个独立UUID观测段。相同power仍可作新段基线，不记成用户操作，也不以重启推断新登录session |
| 范围、字段与小分页 | 实际页面1小时/24小时及power筛选通过；生产读取接口验证from包含、to排除。固定范围/key的limit=1查询返回4页power记录，ID为6、4、3、1，无重复且末页无cursor。这不是默认50条UI“查看更早记录”按钮的实物验收 |
| 名称与时间来源 | 原设备名保持；真实名称设置暂将power命名为“电源开关”，筛选和记录行共用别名/开关标签。手机截图核对UTC服务器确认时间、baseline“不是操作”和change；没有把App到达时间、设备采样时间或Command时间当历史时间 |
| 非空后台与返回 | 真实Android Home事件true→false→true：4条记录在后台清为0、ready/policy关闭且不可查询；返回发起新范围读取，恢复4条。未注入生命周期事件 |
| 非空离页与返回 | 从7条记录页面真实Back后，被动读取原页面对象证明destroyed=true、visible/canQuery=false、rows=0且无待查询；再次正常进入恢复7条。旧隐藏DOM不存在的首次探针失败保留，不计为通过 |
| 查询失败恢复 | 仅暂撤本批拥有的ADB HTTPS反向端口使真实刷新失败：ready=false、rows=0、明确错误，不伪装空历史。恢复同一端口后实际刷新恢复4条及真实policy；不注入HTTP fixture或改库制造错误 |
| 名称恢复与历史保留 | 通过正常UI恢复原空capability别名，metadata修订正常递增4→5→6，不回滚修订。恢复后相同7条记录完整内容保持，只显示名回到原key |

App侧四次command返回ACK的同机观测耗时为144～245 ms；App会话序号23/24/25/27与设备侧ledger序号5/6/7/8分开记录，不当成同一个连接的序号，也不跨手机/服务器时钟计算时延。生产链在实际设备协议ACK入账后完成dispatch，再由Broker转换为App序号ACK，并非仅路由受理；仍须结合State验证，不承诺GPIO物理副作用。设备侧四条ledger均acknowledged、result200、attempt_count=1；最终State为power=false、buttonPresses=0、修订1。

**保留与恢复。** 原账号、既有3条任务请求及空自动化请求的完整内容指纹，在安装前、覆盖后和验收后完全一致；原17个设备ID/名称集合一致。结束保留新鲜的停止进程后完整App目录归档（6,876,160 B），没有清数据。手机蓝牙恢复原开启状态；透明command/history观察器、有限生命周期订阅及本批ADB reverse/CDP forward均按归属移除，不作全局remove-all。设备保留精确原应用与原身份/授权，现场WiFi按用户授权保留；COM3/R4不改。

服务测试网络已撤回并通过独立运行审计：保留匹配新版镜像，不恢复旧数据库、不删除volume；Broker shard1实例及启动时间保持，原50设备、6条规则、14条执行与无关容器保持。短控制窗口的7条历史/4条设备ACK完整保留；之后、网络恢复操作开始前又收到一个合法Snapshot，追加2条baseline。最终共9条/3段（6条baseline＋3条change），不把额外基线当成控制变化，不推断它由物理重启或端口恢复造成。撤回本批17002/18884及唯一代理后，独立查询确认设备MQTT离线、Key版本1仍有效；这不是H1或Key故障。下一次联调需显式核对并启用当前本机测试地址，不能据恢复前证据承诺原网络配置持续在线。

私有Flash、账号目录、运行配置、网络输入、APK及现场证据留在Git忽略的`build/h1-state-history-20261011`，不进入源码提交。主要独立证据为`COM11-nvs-timeline-1791714698995.private.json`、`witness-change-1791714755819.private.json`、`witness-unchanged-1791714764480.private.json`、`witness-reconnect-1791714786670.private.json`及`history-range-1791714798611.private.json`/`history-pagination-1791714799860.private.json`；服务收尾取`services/restore-config-verification.json`和`services/history-ledger-audit-1791715401549.private.json`。生命周期、失败恢复、名称及恢复资源另有完整私有记录和真实手机截图。本批无Git暂存、提交或推送，无关本地修改保留。

H1按本节自有设备短验收范围收口。真实分享只读/撤权、其他Shard部署、UI超过50条加载/200条上限实物、各类型完整矩阵、强杀和长时间保留/稳定性没有新增硬件验收；相关软件回归不改称实物通过。H2在线周期采样、H3离线存储及Bulk仍不实施，后续先决定是否补上述有限验收或交付，不自动扩展采样协议。

### 9.6 H1真实分享只读与撤权收口（2026-10-11）

**本批补齐真实viewer历史访问及撤权后的短验收。** 复用现有COM11、已安装匹配App、Gateway schema51及Broker shard1，不刷板、重置设备、刷新Key或清授权；COM3/R4不动。§9.5的“分享未测”是该批结果，本节补齐该项，不扩大为全部分享通信门禁。

先完成新鲜原APK与停止进程后的完整App私有目录备份、Gateway PostgreSQL可验证归档、三实例Broker在线SQLite备份及容器配置/编译资产核对。App仍为原匹配79,987,977 B候选，本批没有重新安装或改App生产源码。恢复当前已确认的现场WiFi测试地址，保留仅开放既有两条设备POST的17002明文登录代理及18884设备MQTT入口；App账户/历史读取仍走原可信CA校验的HTTPS。原固件真正恢复登录、MQTT和新鲜State后才继续，不伪造在线。

**必要缺陷修正。** 原所有者真实历史查询返回502；同一真实固定范围含13条合法记录，但Broker `SELECT CAST(id AS TEXT) AS id ... ORDER BY id DESC`使用了文本投影别名，返回`9…2、13…10、1`，Gateway严格数值倒序校验正确拒绝。生产源码仅将查询排序限定为`ORDER BY v2_state_history.id DESC`，对外ID仍为字符串，保留64位游标、查询上界及原Gateway/App校验；未改表结构、历史记录、保留策略或协议。另检查retention子查询，其无CAST别名，不作无谓重构。

扩展回归在旧编译Broker上先真实复现19项中2项失败；修正后限定Windows组78/78、Device V2回归、构建/类型检查通过，匹配Linux候选的H1组19/19通过。新增113个同时间戳记录覆盖9→10及99→100、多页50/7、游标冻结期间追加、半开范围，以及10,000条保留后的数值查询顺序。测试组有覆盖重叠，不能累计为独立业务数。复制真实备份只读查询的原13行还通过未改动的生产Gateway validator，不将其单独当成现场部署证明。

部署前再次备份当前shard1 SQLite（2,637,824 B，integrity正常）及运行配置；新冻结输入相对原候选只有上述一个生产源码文件变化，镜像编译树只有`state-history.js`及对应map变化。仅替换Broker1镜像为`b895b8a08a31eabd6360109e687f0731b3ebb492878c41f5593b910cf02fb96c`，保留原environment、volume与网络；其他Broker不部署。真实Gateway读取和原owner HTTPS历史GET随后成功，原历史前缀完整保留，不回滚数据库。

| 真实分享门禁 | 结果与证据边界 |
| --- | --- |
| 正常邀请与账号 | owner在真实App按邮箱创建唯一的定向“仅查看”邀请；接收者是已有真实注册账户，通过App发码/登录UI进入。验证码只读代收本机Auth实际生成值，未种账号/验证码或伪造、注入token，不宣称外部邮件投递已验收 |
| 领取前拒绝 | receiver尚未领取、设备不在其清单时，生产历史读取接口真实404，不把不可查询页面或空数组当成通过 |
| 真实接受与访问 | 实际点击“接受分享”，保留唯一pending invitation→单次accepted receipt→exact active viewer shareId关联；正常进入共享设备。数据模型为viewer、`canCommand=false`；真实历史页与生产读取均取得同一21条合法记录、数值倒序及原设备名，包含分享前的合法历史。当前合同是设备级读，非仅分享后或字段级历史权限 |
| 精确撤权 | receiver保留21条非空历史活页；原owner保留的真实合法会话先GET `/auth/me`验证本人及TLS，再通过生产HTTPS DELETE仅撤本批新增share。初始0 grant、唯一pending invitation及接受receipt均核对，真实200后读回revoked；不删除原设备、历史或审计行。此撤权不是owner手机DOM点击验收 |
| 下一次查询与清空 | 撤权后receiver实际点击原页面“刷新”，被动记录生产读取真实404；另一次生产读取明确`DEVICE_V2_HISTORY_NOT_FOUND`。页面`rows=[]/ready=false/nextCursor=null/policyConfirmed=false`并显示权限变化错误，不伪装无记录；正常刷新设备清单后共享设备消失 |
| owner恢复 | 通过正常App邮箱登录回原owner，原17个设备ID/名称集合与3条任务、空自动化请求journal完整内容指纹相同。撤回测试分享flags后owner真实App历史再次读取23条，设备MQTT与新鲜State仍正常 |

本次证明的是**撤权后的下一次真实查询失败关闭**，不是服务器推送即时抹去已显示内容；当前历史页不自动轮询，也没有新增分享通知订阅。既有Gateway前后ACL/version/route复核保持，不能把短测改称覆盖所有在途并发撤权。

附加管理HTTP禁控探针因测试helper未带Bearer实际返回401 `AUTH_TOKEN_MISSING`，失败记录保留，不算viewer角色拒绝或MQTT/BBP Error6通过。H1本批不为该附加探针重新建分享、扩展跨Shard MQTT或修改控制页面。viewer控件呈现/生产MQTT角色禁控完整短闭环仍是独立待验，不由`canCommand=false`元数据替代。

**保留与结束状态。** 临时Gateway跨Shard请求/分享两flags仅为本机这两个实际账户启用，已恢复原absent；Broker0/2的跨Shard请求配置未开启。当前测试网络、唯一设备登录proxy及修复Broker1保留，不能像§9.5结束时撤掉设备入口再承诺持续在线。最终独立服务审计为23条历史/10个观测段，其中原9条保持、本批额外14条均为真实baseline，无新change；不将正常重新观测推断为物理重启或操作。原19条设备command ledger完全保持、本批新增command为0，Key版本1仍有效；schema51、原50设备、6条停用规则、14条执行与其他设备分享保持。唯一临时grant为revoked v2，accepted invitation与撤权审计保留，不通过删除记录或数据库恢复清理痕迹。

结束再次保存停止进程后的完整App目录归档。原生安全存储38个键清单保持，36个受保护加密条目内容完全一致；正常账号切换仅更新session与既有通知installation两条，不宣称整个目录字节相同或导出了Android Keystore。透明接受/历史读取观察器均恢复原方法；仅撤回本批拥有的ADB reverse/CDP forward及手机截图临时副本，本机证据仍保留，未全局清调试端口。结束App已停止以保存一致归档；后续手机联调需重新建立所需ADB反向端口，服务/设备测试网络则保持。

私有账号、完整目录、实际请求/返回、数据库/运行配置、候选及截图留在Git忽略的`build/h1-share-read-revoke-20261011`。主要证据：`app-evidence-audit.json`、`owner-revoke-*.private.json`、`ui-history-queries-unwatch-1791718166739.private.json`、`native-credential-retention.json`及`services/history-order-fix/prepared.json`/`apply-verification.private.json`、`services/flags-restore-verification.json`、`services/sharing-audit-final-1791718311449.private.json`。独立离线关项`h1-share-independent-final-check.private.json`逐项核对真实返回、权限/数据及原生归档，共21项通过，不将关项断言累加为业务测试数。原自有设备控制ACK/State验收取§9.5，本批不重复发送控制；无Git暂存、提交或推送，无关本地修改保留。

**H1按§9.4～§9.6的最小产品合同与限定实物范围收口。** 其他Shard部署、默认50条UI翻页/200条上限实物、全部标量类型、强杀/长时间保留、分享控制与在途撤权仍未新增实测。随后必要源码审阅、最短使用/匹配交付说明及例程差异已按[易用性计划§10.7](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#107-最短使用匹配来源与例程差异2026-10-11)收口：新单库ZIP同字节重建、S3/R4 BLE安装编译通过，原H1 App/服务沿用，未重复部署。下一步仅同事独立接收，不将发送方测试当签收；R4新实时例程实物保留未测。不自动开启H2在线周期归档、H3离线存储或Bulk。
