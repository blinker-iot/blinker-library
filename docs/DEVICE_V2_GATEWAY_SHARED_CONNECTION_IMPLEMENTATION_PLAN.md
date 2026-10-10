# Device V2 网关共享连接与多子设备实施方案

2026-09-13 校正：基础路线 B3 的[真实南向授时 §19](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#19-b3-4c-4b-南向真机与本地发送背压修正2026-09-13)已完成本地 TX 背压修正、授时/控制和自然恢复，不改变本方案的南向连接策略。
现场仍有 10 秒占用/6 秒让出及窗口边缘控制失败；这是待收口的连续性问题，不是通过多跑循环即可消除的 SDK 限制。
B3 源不可用/恢复、活动断开及原固件恢复短门禁已补验，当前回 B4 通用 LAN/单槽 scope 与共享 executor 复用；持续南向连接、显式 Direct 让出及单连接子设备的离线可达性必须一起审查。
不得把 source=Controller 的业务时钟标签当手机身份，也不把新的 1 秒本地拒收退避解释为增加 MQTT 或网关时钟。

2026-09-09；取代 GATT §10.96 的“下一步100次交替”安排。最新实现为GATT §10.121 / LAN§21的新鲜进展回执与后续投递耗尽恢复，软件/PG已验证，下一步真机控制闭环；已过短门禁见6.6，无缝里程碑见6.7，通用LAN修订见6.8，S3a见6.9～6.11，有限授权历史见6.12～6.18。S0/S1历史入口为§10.97/6.35。

**执行依赖修正：** 当前先收口设备控制/Hello/ACK/自动恢复和真机控制闭环；随后直接做普通WiFi单caller/self的安全WS/mDNS纵向链路，再做多目标/满槽续期及S4。以[LAN计划§19](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#19-主线复核先收口控制闭环再做首条lan业务链路当前执行入口)为唯一顺序入口；下文较早的“先全部S3b-3再WS”安排不再沿用。此次仅修文档，不新增通过记录。

## 1. 决策与事实

采用“账户 MQTT + 网关本地接入 + 有界南向连接池”。一个App账户会话使用一条用户MQTT，
一个网关使用一条设备MQTT，以逻辑child身份路由多设备消息；demand token不是另一条MQTT连接。
App局域网接入优先评估安全WebSocket；它是普通WiFi、WiFi+BLE、WiFi网关共用的网络能力，不是网关专属实现。
网关只额外注入child目标/权限与南向执行；BLE网关代理、子设备双BLE会话是可选增强，不作为小MCU的通用前提。

§10.96已验证恢复正确性，而非交互连续性：30轮Cloud不可用7.108～16.252 s；
当前唯一GatewayGattRadio普通占用10 s、获取16 s、真实机会空档6 s，并非SDK不可改变的限制。
当前ESP32 Central Host/Port及内部Product/radio已组装2个独立child槽，S2e部署后S2d真实双child业务和独立关闭短窗口通过，持续通信仍受旧周期中断；Peripheral Port仍拒绝第二条连接，Central/Peripheral互斥持有Host。
不能复制多个现有Product、修改一个MAX_CONNECTIONS宏，就宣称已支持多子设备。

目标：切换App上游不拆掉仍在使用的Gateway→Child连接；不同用户共享同一child安全连接，
不同child独立故障隔离；SDK、无线协议、账户权限与业务状态分别拥有自己的生命周期。

## 2. 连接与身份合同

```text
App -- 用户MQTT -- Broker -- 网关MQTT -- Gateway -- 南向连接A -- Child A
App -------- 本地安全WebSocket ---------+        -- 南向连接B -- Child B
                                                -- 南向连接C -- Child C
```

- 无网关BLE设备仍支持Direct；普通WiFi/WiFi+BLE不因本方案被强制走网关。
- 网关子设备的普通在线页面默认Cloud代理，未来本地网关路径Ready后才切换。附近广播/存在旧凭据不能触发Direct抢占。
- MQTT属于账户/产品，不属于页面。页面拥有订阅和有界需求；最后一个本地使用者离开才关闭对应本地连接。
- 一个页面退出不释放其他页面、家庭成员、自动化或管理事务的需求；App关闭本地连接也不等于Gateway必须断开child。
- 绑定数量、同时连接数量、流量容量是三个不同上限；未连接/缓存/广播附近都不等于可立即控制。
- 路由目标始终是logicalDeviceId + 当前instance/epoch/binding；原生地址只属于Port，不能重新以MAC建立身份。
- 本地需要独立鉴权与child粒度授权；不得向App分发Gateway→Child secret，也不能通过网关访问扩大分享范围。
- Cloud/MQTT失效只退休该路径authority和需求。未来已有合法本地需求可以独立继续，但必须有期限和撤权合同，不能借本地路径延长旧Cloud授权。

## 3. 多设备容量与调度

普通持续控制/实时订阅有需求时保持连接，不因固定10 s到期而强制轮换；无需求后短idle宽限释放。
偶发读取在有界准备预算内获取；槽满明确busy/capacity，不驱逐其他有需求的合法连接，也不无限排队Action。
广播采集/低功耗唤醒是未来独立南向能力，需认证/防重放；本批不新增广播业务协议或Mesh。

首个真实并发目标为2个child，不承诺SDK最大连接数；槽位总量需同时核对SDK Host/controller、堆/栈、
每连接安全/可靠协议/分片buffer、连接间隔与WiFi共存。管理任务的连接容量要明确预留或返回busy，不能强拆其他child。
绑定目录容量不直接乘以完整ClientStorage/安全工作区：未连接条目保存有界元数据，活动工作区归连接槽。

## 4. 最小模块边界

| 模块 | 拥有的事实 | 不应拥有 |
| --- | --- | --- |
| Gateway核心/Child上下文 | exact身份、授权版本、业务关联、状态和结果路由 | SDK句柄、MQTT socket、永久全局online布尔 |
| 需求与容量管理 | 每child合法需求、期限、可分配槽位、取消 | 新Action重试队列或按页面重复BLE连接 |
| BLE适配器 | 一个Host/扫描管理、固定连接池、每连接attempt/关闭/credit | 账户ACL、组件定义、每child独立Host |
| 北向接入 | MQTT/未来WebSocket各自认证会话、流控和消息输入输出 | 因自己断线而全局reset所有child |

复用现有BBP/2、GatewayAccess、scope、generation、结果ledger和codec。
南向多连接必须同时完成RouteClient demux、ChildRouteBridge绑定、管理mux与产品持有关系的隔离；
不能只给Port增加数组而保留全局child指针。S2a已拆分route端点/child上下文；S2b已接目录、管理分派与双记录后端，S2c原生Port池和Product/radio双槽纵向软件完成；S2d双child短测范围与剩余项见6.6。
通用核心按能力/句柄执行；以后ESP-NOW/Zigbee替换南向适配器，不伪装成GATT状态机。

### 4.1 S2启动时的约束与替换进展

| 已确认的单child约束 | S2应实际替换的持有关系 |
| --- | --- |
| 原Central Port的`activePort`同时拥有SDK Host和单连接状态 | **S2c-1/2已替换：**Product持有唯一Central Host；全局角色claim仍唯一。Host按SDK容量提供最多2槽，只有扫描/发起权串行，A的ACL不阻止B建连 |
| 原`Esp32NimBleCentralPort`只有一份request/connection/GATT句柄/pending write | **S2c-2/3已实现逐Port隔离及B接线：**每Port仍满足`IBleCentralPort`单链路接口，共享Host登记；callback按token及native handle定位，Link不需要知道SDK全局对象。双native连接/业务短测已验，不等于长稳 |
| 原Product只有一个access/renewal blob | **S2b-2b已替换：**平台提供2对独立记录，保留0号原NVS名称；目录负责逻辑身份选择，每个store仍只管自己的记录，不以MAC/下标作权限身份 |
| 原`GatewayRouteClient`只有一个receiver和一个deferred ingress；Bridge构造/析构会设置/清除它 | **S2a已替换：**MQTT订阅与topic只保留一份；按exact child分派到各自端点，Bridge只注册自己的接收器；A的WouldBlock不阻塞B |
| 原Product把child buffer/session/connection/bridge与WiFi登录全部放在同一对象 | **S2a已替换：**活动child上下文被Product实际消费；Port由平台注入，WiFi、DeviceKey、MQTT、时钟和Host仍共享，不复制整个Product |
| permit-join复用同一个child Port，原control mux只按操作族串行分派 | **S2b/2c已替换：**先按child/operation/epoch定位，再交逐child操作族mux；Hub permit仅一份，借还物理空槽并独立保留已绑定目标child准入 |

一个活动槽独占安全序号、重组、握手/消息工作区和晚回调生命周期；未连接目录项不预分配整套这些buffer。
共享收发必须有逐slot配额和背压，不能用一个无限全局队列代替隔离。已取消但尚未物理关闭的槽仍占容量，
晚callback携带的上下文在排空前不能被下一次连接复用；SDK真实Host reset则报告所有受影响槽，不能假装只断一条。
这次是局部拆分现有实现，不新建另一套candidate Product或要求Sketch传入内部槽/工作区。

## 5. 连续性与安全边界

- 新路径先鉴权并同步，Ready后才切换后续请求；旧可用路径不能为后台握手而先拆掉。
- 页面生命周期与单条传输生命周期分离；未知结果Action不跨路径自动重发，不能双路径各发送一次。
- State按instance/当前协商/版本合并；缓存可用于显示，但不冒充fresh或控制成功。
- SDK close是异步物理结果；逻辑取消不能释放尚未关闭的原生槽。一个旧attempt不能关闭继任者或其他child。
- 升级期间仍必须兑现已发出的Direct物理承诺，不能借MQTT换代/新配置提前清空。
- 单ACL子设备若仍被网关占用，而手机又无法通过Cloud/LAN/网关BLE访问，无法同时保证立即Direct和网关不断线。
  必须有明确的维护接管/网关失效回退，或双连接增强；不能静默删掉原离线窗口并声称功能等价。
- 多连接增强要隔离每session密钥/序号/重放/重组/notify credit及未鉴权占位；共享一份设备状态，不运行多套完整Blinker。

## 6. 分批执行与退出条件

| 阶段 | 内容 | 准入/退出条件 |
| --- | --- | --- |
| S0 已完成 | 冻结本方案，修正三端入口 | 100轮暂停；历史恢复门禁不删除、不冒充多连接/低延迟 |
| S1 已部署，A/B页面短测通过 | 普通网关页在线默认Cloud，离页按自身需求清理 | 页面button/ACK、无Direct抢占与离页scope归零已验；S2e服务器单child限制已修正 |
| S2 进行中：S2a/b/c/e已部署 | 共享Host与2-child连接池纵向实现 | 双数据面、满槽Busy、A Direct让出/B继续、MQTT换代短测已验；管理变更交错、SDK reset与资源收尾未验，见6.6 |
| S3 | 通用WiFi LAN/发现/本地授权＋网关child代理扩展 | 先普通WiFi self，再同模块接WiFi+BLE/Hub；MQTT和本地共享child会话；有限离线；只分享A不能访问B；见6.8 |
| S4 | 持续连接策略与页面路径切换 | 完成本地/显式接管离线边界后，替换默认周期让出，删除被替代的自动交替接线；兑现旧承诺；不得长期保留两套默认调度 |
| S5 | 多子设备体验门禁 | 至少1网关+2child+App/Cloud caller；同时控制/订阅，关闭页面不影响他人，断A不影响B，槽满有界；planned switch不重建child连接 |

S2/S3可分批实施但必须分别标明“源码完成/未部署/真实多设备未验”。本地暖连接控制ACK先以P95≤300 ms为待验证目标，
冷启动、断电与外网故障分别统计，不把网络不可用情况下的0 ms恢复当承诺。
S1只消除App主动抢占：旧Hub周期调度仍会产生空档，不能据此宣布无缝完成。
设备API/Sketch、设备身份与凭据保持；不扩散History/Bulk、BLE Mesh或工厂eFuse流程。

### 6.1 S2分解与实际进展（§10.103）

- **S2a源码完成：**`BasicGatewayGattChildContext`实际替换原ESP32 Product中的工作区、link/session/connection、
  bridge和逐child管理协调器接线；注入Port、原子记录存储与共享时钟/随机/crypto。S2b-2b起DeviceKey source只供轻量binding接收使用，不再传入活动context。
  `GatewayRouteClient`只持有一份MQTT订阅/topic；`GatewayRouteEndpoint`逐child持有接收槽、matcher和receiver。
  未知/歧义目标不广播；按logical id＋instance＋access epoch＋credential version选择，Bridge仍校验完整权限/拓扑/期限和关联。
  入站和消费前重新检查当前记录，A的WouldBlock不延长原1 s预算、不阻塞B；销毁A只注销A的端点，不取消共享MQTT订阅。
  没有保留原Product大段对象组装或另建candidate产品；测试用背压probe已改为逐端点包装，不给生产代码加测试钩子。
- **S2b-1源码完成：**`GatewayChildRecords`独立拥有注入的原子access/renewal存储对，`GatewayChildDirectory`从既有记录重建有界目录，
  初始/轮换凭据通过实际Product目录入口定位后交给原认证处理器。无额外持久账本、记录格式变更或乐观ACK。
  非秘密身份索引在任意写入/清除尝试（包括失败）前失效；底层重开/外部维护显式失效。共享底层blob不能被第二个写入者绕过包装修改。
  六类route暖索引选择不读Flash，Bridge仍读取当前凭据做鉴权。撤销墓碑、孤立renewal日志均占容量；损坏、歧义、同id不同instance不能当空位。
  该批平台仍只有一对blob；S2b-2b现已接双记录后端，但尚未提供两条原生连接，软件记录隔离不能当物理连接证明。
- **S2b-2a源码完成：**实际management入口与radio接入`GatewayChildManagementRouter`，proof/revoke/renew先定位记录，再交原逐child协调器。
  一次只有一个管理owner，A的任务、原生所有权或待发布结果未结束时B不能接管；permit-join仍是单个Hub级操作。
  未知/歧义命令不reset原owner，匹配记录暂未配置活动control时返回WouldBlock；native回复仍直接回到其来源child mux。
  墓碑清除后的迟到Finalize/Supersede采用6.2的全目录缺席证明，返回既有终态；不增完成账本、wire字段或MCU重试队列。
- **S2b-2b源码完成：**`BasicEsp32GatewayStorage`在实际Platform/Product中拥有2对独立NVS记录，0号原名称/codec不变。
  `GatewayChildBinding`将凭据接收从活动GATT context移出；每绑定只增加记录/非秘密索引与小型处理器，不复制握手/分片缓冲或MQTT。
  Hub访问重置只有全部记录/日志清除成功后才清自身Key。实际NVS适配器边界覆盖旧数据、4处open失败、set/commit/erase失败与RAM重建隔离。
  补NVS清除重试的NotFound后commit确认；故障模型旧实现失败、修正版通过，不把模拟commit行为当硬件已复现。
  第二条目的delivery已接通，control暂为空、对应管理返回WouldBlock；没有为B分配Port/route活动上下文，不宣称B已经完成绑定或可控制。
- **S2c-1源码完成：**实际Product拥有唯一`Esp32NimBleCentralHost`，Port删除原全局owner/init/sync/deinit实现，停止只关闭自身活动并注销借用。
  SDK GAP/GATT arg不再保存Port地址，改用有界登记表查找不复用的进程期令牌；正在执行的callback持有借用，未归还不能复用/销毁Port。
  SDK reset通知全部登记者，sync不能覆盖各Port待处理错误；SDK清理失败保留角色，deinit重试不重复stop已停止任务。
  该批两个Port可登记不等于两个ACL；S2c-2现已分开发起权与逐连接占用，见下一项。
- **S2c-2源码完成：**删除覆盖整个ACL寿命的单`activity_`，Host只串行扫描/发起，已建立连接保留各自槽、token与credit。
  A的CONNECT完成被loop消费后，B可扫描/建连，A仍可发现GATT/收发；关闭/失败/借用排空只处理自己的槽，不能取消B的扫描或等待B断开。
  SDK reset失效旧token并让各Port先处理reset再清旧handle，避免B先恢复复用句柄时被A误断；错误handle的GAP不能清A队列/改安全状态/关闭B。
  固定上限2并受`CONFIG_BT_NIMBLE_MAX_CONNECTIONS`较小值限制，不修改SDK宏、引入SDK版本号白名单或新增Arduino参数。
  Central29场景、SDK单槽降容量独立目标、完整Host/ASan52及ESP32两构建通过；当前实际Product/radio仍一个活动context，B的业务接线与真机未完成。
- **S2c-3a源码完成：**逐child mux只接proof/revoke/renew，删除Hub permit的解码/轮询/清理/占用；mux与来源回复接线归入活动context。
  实际radio只安装自身child的准入，Hub router只安装permit专属准入；A的Direct/安静期不再成为B管理的全局闸门。
  全局管理owner仍串行，未发布结果/异步关闭仍阻止接管；整体reset只清一次Hub permit。断网及订阅失败由Product补一次Hub控制tick，不由每个radio重复poll。
  Host/ASan52、架构/345文件发布与ESP32两构建通过。实际Product仍一份活动context；permit仍临时借0号Port，3b/3c未完成。
- **S2d：**将目录、管理、Port、context纵向接通，先软件/SDK资源门禁再1Hub+2child真机。
  S2a的两个软件上下文不代表两条物理BLE连接，更不替代S3/S4连续性合同。

每步必须有生产调用者及回归，不把仅有接口、模拟测试或SDK连接宏当作S2完成。
S2a对象ABI基线：共享路由280 B、每child入口736 B、GATT活动上下文7544 B（已含736 B入口，不能重复相加）。
这些值不含SDK Port、NimBLE堆/任务栈、共享WiFi/TLS或未来绑定目录；Host ABI不能代替MCU资源准入。
S2b-1增加独立记录/索引包装208 B/绑定、目录对象12 B、entry8 B/绑定；GATT活动上下文仍7544 B，但不包含外部记录包装，二者需相加。
S2b-2a增加共享管理router88 B，entry增加一个控制指针（32位下由8变12 B）；最终产品静态RAM较S2b-1增加96 B。
内部产品Flash1,443,956 B/RAM65,948 B，双板背压probe Flash1,452,972 B/RAM66,004 B；不是2-child固件，详见GATT§10.100。
S2b-2b最新ABI：2槽存储bank704 B（含全部4个NVS blob对象和2份208 B records），轻量binding60 B/绑定，活动context由7544降至7488 B。
内部产品Flash1,444,444 B/RAM66,372 B，双板probe Flash1,453,568 B/RAM66,428 B；相对2a静态RAM各+424 B，Flash各+488/+596 B。
NVS打开句柄带来的SDK动态成本、真实双连接堆/任务栈另行度量；不能把静态bank704 B再次加上已包含的records416 B。
S2c-3a历史ABI：child context7512 B，已包含移入的24 B子设备mux；不能再重复加mux。共享router88 B、Host56 B、Port1068 B不变。
内部产品Flash1,446,272 B/RAM66,428 B，双板probe Flash1,455,336 B/RAM66,492 B；比§10.103分别-56/-8 B、-32/-8 B。
上述3a是仍只接child0的历史固件。3b/3c实际两活动槽probe静态RAM75,228 B；context7512 B/slot、Port1068 B/slot、radio52 B/slot、pool48 B及共享boot guard12 B。
已刷候选的A单连接短测heapMin72,812 B、ipc0最低1104 B、loop3640 B、NimBLE2688 B；不代表双连接、TLS重连叠加或长时间资源门禁通过。
暖identity函数静态栈帧从同函数冷路径的416 B降至32 B，冷refresh独立400 B；这不是整条调用栈峰值/任务水位，冷重建与真实双连接仍需测量。
S2a最终Host/ASan50项、架构、Arduino发布文件验证通过；ESP32 3.3.11内部产品及带背压probe的双板Sketch编译通过。
资源/日志见GATT§10.98；本批未运行真实双连接或烧录、改动设备记录。
COM3新增XIAO ESP32S3已在Windows串口列表确认，另有COM11/COM12；尚未读取板身份或烧录。
后续先确认1Hub+2child的角色/实例和固件，再用新板建立B；不得为增加B重置A或覆盖其Access记录。

### 6.2 S2b-2控制合同与剩余后端约束

1. 平台提供有界、独立的access/renewal记录对。保留原0号NVS namespace/key，无隐式搬迁、擦除或覆盖；新增槽分别原子提交。
   未连接条目只保留记录包装和非秘密索引，不按绑定上限创建GATT context/握手buffer。SDK池容量另行准入。
2. proof按logical id/instance/operation定位；revoke与renew控制消息没有logical id时，按各自exact operation/instance/epoch定位。
   墓碑只有撤销operation而没有完整child身份，不能用MAC或相邻数组槽补猜；下游协调器保留topology/version/receipt等全部校验。
3. **S2b-2a已冻结terminal：**Finalized/Retired说明Hub不再保有匹配旧authority，不代表重新取得child签名撤销receipt。
   每次重新读取整个受保护目录；目标record/墓碑仍存在则交原协调器做exact清除和校验；所有条目确认不含目标旧authority才返回既有Finalized/Retired。
   不选择空槽、不写Flash、不新建持久完成账本。Broker继续按Hub/tenant、operation/topology/phase ledger、digest和原期限校验，exact重投幂等。
   RAM重建、ACK丢失、槽被新epoch复用后可返回同一终态而不改新记录；同一authority换operation不能当缺席。
   任一损坏/不可读/歧义条目拒绝；与旧authority有关的可恢复renewal journal返回WouldBlock，不能只清access留下旧候选秘密。
   journal重叠情况下的专门前向恢复仍未收口；本批不擅自清journal、不声称全部撤销/轮换交错完成。
4. Hub级permit-join没有child身份，应独立保留一个。管理owner至少由child、operation与操作族共同定位；Ready已发出但原生关闭未完成仍占用。
   新的child管理事务不得reset另一child的协调器或关闭它的连接；单个MQTT与现有Broker有界重投保持，不新增MCU重试队列。
5. **S2b-2a已替换接线：**外层router只负责目录定位/串行owner，原mux保留为每child内部的操作族分派器，不再是Hub全局child指针。
   ControllerControl回复绑定来源child mux，不能按router当前owner转发。Cloud授权退休可reset整体Cloud控制；未来本地authority不受其无条件清空。
   S2b-2b平台后端之后，S2c实现共享Host/两个native slot，并逐slot验证逻辑reset后真实关闭屏障；S2d用COM3新增B做纵向验证。
   最新已是两对持久blob/两个Port/context/radio；设备纵向接线完成仍不抵扣Service/Broker多child路由及物理并发门禁。
6. 存储槽0保持`bl_eh_access/child`与`bl_eh_renew/pending`，槽1为相同namespace下的`child_1`/`pending_1`。
   首批绑定容量固定2、实际活动容量1，没有面向Sketch的容量开关。以后扩大容量要同时更新平台映射/组合与资源门禁，不改协议身份。
   降低活动容量不允许减少已存记录的枚举/重置范围；降级到只认识槽0的旧Hub固件可能遗留槽1，不能当安全回滚方案。
   正式回滚前必须由认识完整目录的版本完成授权清理或明确迁移；本批未执行任何实机降级/重置。

### 6.3 S2c：唯一Central Host与有界原生活动槽

S2c-1已移除`Esp32NimBleCentralPort::activePort`与Port自有SDK生命周期，生产Product声明Host在Port之前、销毁在Port之后。
S2c-2已删除Host的唯一`activity_`准入，3a已分离child管理与Hub permit；3b/3c已接入slot级permit借用、目标准入、逐child radio与两个活动context。S2d已部署App并正式绑定B，但发现Service/Broker仍单child，必须先§6.5再双业务验收。
按以下依赖顺序替换现有实现：

1. **S2c-1软件已完成、已随3b/3c部署：**一个Central Host拥有SDK初始化、全局回调与活动准入；Port只借用Host。
   中央Port回归覆盖18个场景，完整Host/ASan51项、架构/345文件发布检查与ESP32两构建通过，详见GATT§10.102。
   Host仅全产品显式`end()`或析构时停止SDK；登记未清/SDK stop失败返回WouldBlock，deinit失败保留角色等待重试。
   `Port::stop()`超时保持Error/WouldBlock；调用者必须保留其生命周期后重试。违反此约束强行析构会abort以阻止UAF，不是正常超时自动重启策略。
   callback令牌进程期单调且不因Host restart复用，耗尽拒绝；固定2条登记/有界在途借用，无逐attempt堆对象或新增消息队列。
   保持已有Central/Peripheral/WiFiProv互斥角色，不同时扩散双角色并行。Host56 B、Port1068 B为当前MCU对象ABI，不含SDK堆/任务栈。
2. **S2c-2软件已完成、已随3b/3c部署：**逐Port保存attempt/generation、连接/GATT句柄、待发送/接收credit；Host串行准入SDK扫描/发起连接。
   CONNECT到达SDK但未被loop消费时仍占发起权，消费后释放发起权而不是ACL槽；closing/callback未排空的槽仍占容量。
   SDK边界fake已改为两条native连接表及逐handle GATT回调；A/B同时GATT Ready、交错发现/发送、队列背压、独立关闭、旧token/错误handle/reset复用与SDK busy都有回归。
   槽满或另一发起过程未结束返回有界busy/WouldBlock，不新增无限工作队列、不驱逐已有合法会话；晚回调不能触碰继任者。
   SDK级reset影响所有登记槽并明确通知；普通A断开只清A。原生关闭完成前容量不可重用。
3. **S2c-3b/3c软件完成、候选已部署COM11：**两个context/route/control接入同一绑定目录；permit保持唯一Hub入口并借用物理空槽。
   A普通页面Cloud命令及permit窗口短测已通过；S1 App已部署、COM3 B已正式绑定。S2e已修正服务器单child限制并通过A/B数据面；当前以6.6剩余门禁为准，不恢复100轮。

只存有B凭据不是B在线；只有其原认证、proof/route协商成功才可Ready。S3本地授权/WebSocket、S4取消默认周期交替仍独立执行，
不能为了新池源码通过而提前删除旧离线物理承诺或宣称7～16 s空档已消失。

### 6.4 S2c-3上层所有权与剩余执行顺序（§10.105更新）

§10.103发现的全局management admission覆盖与逐child mux重复持有Hub permit已在3a删除，实际Product已消费新边界。
3b/3c已删除radio对Hub permit全局owns的引用、adapter固定0号Port和默认释放A；通过caller-owned有界pool实现物理槽借还与目标child隔离。
借还合同为GATT特定接口，不侵入通用网关管理协议；未来其他southbound复用route/目录/owner，而不复用GATT连接状态。

1. **S2c-3a 已实现，随3b/3c候选部署：**逐child mux只分派该child的proof/revoke/renew；Hub router唯一持有permit分派与全局管理事务串行owner。
   删除逐child中重复的permit解码/poll/reset责任；新命令准入按已定位的child/slot检查，不能由A的Direct/fairness决定B是否允许管理。
   来源回复保持绑定来源mux；未知/歧义命令、终态absence和原exact continuation规则不变。不新增wire或重试账本。
2. **S2c-3b 软件已完成：**每child独立需求/Direct承诺/关闭状态；全Hub只有一个有界permit窗口和发起资源仲裁。
   permit只借用实际空闲且已物理关闭的slot，结束后才归还；满槽明确busy，不能默认释放A或驱逐有合法需求的连接。
   共享boot安全屏障与逐child公平/安静时间分开；不能把B的连接计入A的离线窗口，或因A清理而reset其他slot。
   原周期/已发出的Direct承诺仍保留到S4，不能为了S2通过提前删除离线安全边界。每loop有界轮询，未获发起权不得无限重试或饿死另一个child。
   **实现约束：**adapter通过lease取得明确Port而不是构造时固定0号；只有自身停止且callback/credit排空后归还，后续Link启动恢复自身接收器。
   借用槽与目标child是两种身份：借到B槽不能绕开A的Direct承诺连接A。复用现有授权Presence/身份匹配检查目标；不以MAC、新队列或额外secret缓存替代。
   暂不能证明目标允许时必须拒绝/等待，不以“另一个空槽”作为许可。新Provisioning与已绑定Direct候选的准入分别测试，不能混为一类。
   双radio＋实际adapter Host回归已覆盖空闲B借用/A继续、A Direct期间B扫描、A关闭/B Ready、满槽Busy、异步归还/接收器恢复、错误槽归还与损坏记录拒绝。
   未选定候选的permit扫描最多连续占用2 s后向等待child让出发起权；只有存在等待者才让出。已选定opaque relay保持原窗口预算，不能跨连接续传。
   借B槽却选择已绑定A时，必须额外保留A的逻辑准入，直到借用Port物理关闭；不缓存额外secret，复用当前Access/授权Presence匹配。
3. **S2c-3c 软件已完成：**从同一绑定目录组装两个活动context与Port，各自route/control/policy；共享MQTT、clock、crypto、Host和permit。
   完整槽枚举参与stop/resetAccess/恢复，旧0号记录不搬迁；公开Sketch保持不变。预览/内部probe的状态读取按明确child选择，不继续硬编码A冒充Hub全局。
   删除六个固定child0内部访问入口；probe使用`child(index)`和显式slot，公开Sketch API不变。仍是内部两child产品，不把SDK支持两连接当作产品发布准入。
4. **S2d部分通过（§10.108）：**S1 App已部署，A/B普通页面Manifest/State/button/ACK及离页清理通过；COM3 B保持无Key纯BLE身份。
   S2e（§6.5）已完成本机配套部署，双native连接/并行Command-ACK和短窗口A关闭B继续通过；未重置/删除A来验证B。
   满槽permit不驱逐、A Direct物理让出/B继续、MQTT换代短测已补。继续管理变更交错和SDK reset资源，完整双child Direct数据面另验；旧周期影响持续需求，S3/S4仍独立未完成。

### 6.5 S2e服务器侧补齐及部署顺序（§10.107更新）

**当前：S2e-0～3软件与本机Gateway/shard1配套部署完成。** 普通A/B页面、两Cloud需求/两native连接/并行Command-ACK、短窗口A关闭B继续已通过。
另补无DeviceKey child的Broker身份元数据与NULL数据库迁移；不再由测试fixture预创建独立凭据。未知目标只拒绝该请求，不关闭账户MQTT。详见GATT§10.107。
§10.108补齐双占用permit Busy、A Direct让出/B控制及正常MQTT换代短测；管理变更交错、SDK reset及叠加资源门禁仍未完成。旧10s占用/6s让出仍会中断B合法持续需求，不得把短窗口通过标成S3/S4无缝通信。
本阶段route profile及每MQTT身份的不同child/floor预算均为2；退休floor不丢弃，第三个不同child须更换MQTT身份。不是未来网关绑定数量的产品上限。

设备双槽不是完整系统双child。真实B绑定暴露两处未清理的单child假设：Service按Hub查路由拒绝多个Active；
Broker一个MQTT client只持有一个RouteContext和连接级generation/admission/recovery。须一起替换，不能先删LIMIT让Broker覆盖旧context。

1. **S2e-0清单投影及本机配套部署完成：**保留`cloudEnabled`作为Cloud通信能力，取自身DeviceKey或有效网关绑定；
   设备登录凭据仍由`credentialVersion/locator`表达。owner列表/read/instance和已授权分享列表共用批量投影，不发新Key、不增加持久表。
   Active能力与Broker presence、fresh、Ready分别判断；查询不返回secret或扩大分享清单。新PG/management回归已过。
2. **S2e-1权威路由集合：**按Hub返回有界、确定性排序的route集合；按child查询仍返回exact单路由。
   列表上限与设备可支持profile显式校验，超限返回capacity错误、不静默截断；当前以2验证，不把绑定数等于永久在线数。
   保持Active/期限/轮换阻断以及tenant、instance/epoch、Hub Key边界；普通账号不能查询别人的目录。删除旧G2A单child查询和Broker单值解析接线。
3. **S2e-2逐child投影：**一个Hub MQTT owner持有有界child目录，每child独占route、generation floor、admission attempt、recovery budget/timer、
   BBP session、bootstrap、执行/Direct需求、命令相关及presence token；共享的只是MQTT身份、topic和管理通道。
   按exact wire child/instance/epoch及generation分派，不广播/取第一条；重投/迟到ACK/恢复timer只触碰原child及原owner。
   普通A失效只退休A；只有整个MQTT owner换代/Hub撤权才全量退休。不能让A新generation覆盖B，或销毁context后丢失同MQTT内的反重放floor。
   现有逐childACL、command ledger和codec继续复用，删除被替换的全局单child状态，不引入第二套Broker或账户MQTT。
4. **S2e-3软件门禁后配套部署：**覆盖两child集合解析、超限/歧义、交错Hello/State/ACK/late callback、A重协商B继续、
   单child撤权/轮换、全MQTT换代及partial admission失败。测试当前单child实现应明确失败，新实现通过。
   再部署Service/Broker和本批清单修正、刷新App inventory；复用当前A/B绑定，不重跑认领、不改原凭据、不对未知Action自动补发。
5. **回S2d/S5短门禁：**同一账户MQTT和同一Hub MQTT同时驱动A/B，分别得到Manifest/State/Action/ACK及真实双native连接证据；
   A关闭B继续、满槽permit Busy不驱逐、管理/Direct隔离、TLS/MQTT重连与SDK reset资源分别测。随后才S3有限本地授权/WebSocket、S4去周期交替。

§10.106现场的旧单child Broker已由§10.107本机候选替换，A/B原绑定保留、B无Key数据面已通过。100次交替仍暂停。
保留以下实施条目作为完成范围追踪；后续按上方尚未完成的S2d门禁与S3/S4推进，不重复实施已完成的路由拆分。

### 6.6 S2d容量、Direct目标与MQTT换代短测（§10.108）

- **满槽准入通过：**同一账户/Hub MQTT、A/B两条真实GATT连接并Secure时，随机测试permit operation得到`pending_open → busy`。
  无candidate选择或grant安装；exact Close终态确认。期间两个child均Ready，之后并行button A 4→5、B 3→4且两个ACK。
  SDK连接表在Busy前后均两条；permitError=13为容量拒绝历史诊断，不是子设备断连或SDK错误。
- **Direct精确物理让出通过：**释放A自身Cloud需求后，只给A申请真实Direct reservation并等待`ready/assertReady`。
  A Cloud reachable=false，SDK仅余B一条连接；B仍Secure/有需求，button 4→5/ACK，A保持5，账户MQTT不变。
  未建立手机→A原生ACL，不能据此勾选手机Method 2/DirectSecure完整双child数据面；本项专门证明Hub让出只影响目标。
- **MQTT换代短测通过：**在A/B Ready时仅关闭exact Hub客户端，保留Key/Access/数据库拓扑。
  Broker观察新claims身份与managementReady，两个child重新Hello/State Ready；离线到两者Ready采样约9.6 s。
  新连接随后各一次button A/B 5→6/ACK；账户MQTT未变，未发送未知结果重试。这是故障恢复，不是planned switch性能。
- **工具修正：**旧单child fault gate改为按消息child过滤；兄弟child原包直接通过，不消耗fault/重放缓存/计数。
  Direct控制单独解码，不能误作BBP envelope；恢复改用捕获的operation/topology精确退休，禁止全Hub fallback。
  33项工具回归＋5项双child调用工具＋3项permit工具通过；这些是工具测试，不冒充Broker完整回归或MCU编译。
- 所有临时owner/permit/observer已清理，主进程Inspector已关闭，手机回首页scope=0；A/B计数6/6。
  原绑定、Key、Access、固件/SDK不变。采样heapMin38,112 B、loop2172/ipc01104/NimBLE2688 B；串口部分日志截断不用于逐包时延结论。
  仍欠：真实管理renew/revoke与兄弟业务交错、SDK Host reset双槽恢复/资源、持续性能与发布长稳。

### 6.7 无缝通信的完成边界与S3/S4实施拆分

**首次具备完整功能在S4，确认体验达标在S5。** 当前S2只解决并发和隔离，旧周期未删除，不能承诺已经没有7～16 s空档。
无须先跑100次交替或24 h长稳才开始S3源码工作；但S2剩余故障隔离/资源门禁必须在S4真机准入前收口。
本次将原“所有S2d真机项目结束才可动S3”修正为：**可以推进S3a模块抽取和S3b合同/软件测试，硬件阻塞不扩散为整个主线停工**。

| 批次 | 实际改动与验收 | 不能提前宣称 |
| --- | --- | --- |
| S3a 1a/1b/1c已实现 | 6.9～6.11完成普通WiFi非鉴权隔离、peer退休、按通道access绑定/分派、容量ABI与唯一child executor/需求；Cloud adapter实际消费 | 软件假体不等于LAN可用；鉴权撤销范围未明时不放宽AccessFault；不复制child状态 |
| S3b 本地授权 | 通用endpoint/self授权，网关扩展exact child；会话证明、无UTC时的期限/撤权、一个安全profile与RAM预算；Service签发、设备验证、App消费 | 不能把旧ControllerGrant改名后当代理授权，也不让普通WiFi依赖GatewayAccess |
| S3c 最小纵向链路 | 先普通WiFi的mDNS＋认证WebSocket self链路；同模块接WiFi+BLE/Hub，再LAN控制A与Cloud保持B、同child双caller共享；详见6.8 | WebSocket连接成功不等于鉴权/业务Ready，ESP32通过不等于R4/Wio已发布 |
| S3d 离线与故障 | 已获合法本地授权在Cloud失效后可继续至原期限；撤权/过期只清对应需求，Hub重启/时钟不可信时fail closed；管理/SDK资源门禁收口 | 无网络时即时得知云端撤权、冷启动无限离线接入 |
| S4 持续策略与切路 | 用合法需求并集保持连接＋最后owner离开后的有界idle释放，删除默认10s占用/6s机会周期；App新路径鉴权/同步Ready后只切后续请求 | 双发Action、跨路径自动重放未知结果、取消已承诺Direct窗口 |
| S5 体验验收 | 至少1Hub+2child：Cloud↔LAN暖切换不重建南向连接、A故障/B继续、页面退出不影响其他用户/服务、冷故障单独统计 | 单次成功即P95达标、把物理断电恢复称为无缝 |

**S3a必须解决的真实耦合，而非再增加一层包装：**

1. 现有Bridge同时持有Cloud admission/execution/Direct控制、child receiver/session handlers、BBP序号及在途/最近请求。
   不能让MQTT和WebSocket各建一个Bridge并覆盖child callback，更不能各建一套child ClientStorage/安全session。
2. 唯一child执行模块负责南向序号和结果关联。每条上游会话仅持自身认证、有限需求、路由相关和小型发送窗口；目标始终exact逻辑身份/instance/epoch。
   Cloud重连只退休Cloud投影，不能清仍被本地使用的child收发/安全序号。SDK真实reset才影响所有相关物理槽。
3. 当前只有一份在途exchange；首版保持同child有界串行，忙时返回WouldBlock/Busy，不加入无限Action队列。
   ACK回原caller；与一个请求关联的重试仍用原requestId/ledger，同一Action不能由路径选择器复制成两个请求。
4. Manifest/State是设备业务事实，不属于某条MQTT；按instance/schema/revision合并。上游协商状态独立，不能把别的caller已Accept直接当本会话Ready。
   自动化、状态订阅和管理也是合法需求，不因App离页而停止全部通信。绑定目录不按每个caller再复制。

**S3b有限本地授权合同边界（字段编码/安全握手须软件审查后另行冻结，当前尚无本地生产实现）：**

- 身份认证和child授权分开：只能登录Hub不代表可访问Hub下所有child。能力必须覆盖Hub身份/当前Key代际、caller会话、exact child/instance/epoch/topology、Observe/Control权限及原始截止期限。
- 旧`ControllerGrant`是针对单设备凭据Install/Rotate/Revoke的签名变更合同，没有Hub代理目标集或授权期限；不得复用其权限位伪造“全网关控制权”。
  原Gateway→Child secret不下发App，不把其作为本地登录密码；也不使用长效DeviceKey做浏览器bearer。
- 初版只支持Cloud在线完成本地会话授权，已认证会话可以有限离线继续；不增加Flash中的家庭用户表，也不承诺冷启动离线重认证。
  首个profile以2个child、最多2个本地caller作为软件容量评估目标，资源不足须调小/返回Busy；这不是已验收配置。
- 截止时间用接收方单调时钟约束且不能由断网/重投/重连延长。在线撤权即时停止对应本地转发；Cloud完全断开时不能获知新撤权，只能承诺不超过原离线期限，App需诚实呈现。
  重新联网先复核authority再续期；Hub重启或失去可信时间不使用旧RAM授权恢复。不新增永久Local override绕过撤权。
- WebSocket仅为载体。证书身份验证的WSS，或复用已审查AEAD/认证模块的安全记录都须有明确会话与权限绑定；不得为先跑通而开放裸`ws`的明文密钥/未鉴权控制。
  S3b必须通过堆/任务栈/握手预算选择**一个**首发安全profile，不同时扩散两套TLS与自制密码握手。不能简单复用BLE域的密钥/nonce空间。
- 局域网地址/广播/mDNS是发现提示，不是权限或endpoint身份；地址变化重新验证同一实例，不以MAC合并设备。
  普通WiFi与WiFi+BLE可组合相同LAN模块，不强制增加TLS/网关状态内存；纯BLE无IP，不引入WebSocket。

**离线接管和目标体验：**有可用Cloud或已授权LAN时走Hub代理，不抢占单ACL子设备；显式维护Direct必须等精确物理让出确认。
若Cloud、LAN和其他Hub接入都不可用，无法同时保证网关无限占用与手机立即Direct；S4应使用有界授权到期释放/明确维护操作，不能许诺物理上做不到的零等待。
暖路径控制ACK仍以P95≤300 ms为待测目标；planned switch验收还要求南向连接代际不变、没有固定周期不可用、没有重复Action。
意外网络断开会有检测时间，冷扫描/认证、网关断电与SDK reset另列恢复预算，不给“任何故障0 ms”的承诺。

### 6.8 通用 LAN 能力与发现修订（§10.109，2026-09-09）

本批按用户要求先分析，未新增生产通道；完整合同及实现顺序见[通用局域网 WebSocket 与发现方案](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。
此前S3仅描述“本地网关”的范围不足，现修正为**普通WiFi/WiFi+BLE/网关复用LAN接入，网关仅增加代理目标**。

1. 沿用`IFrameTransport`、同一个Client/Manifest/State；LAN扩展引用监听/发现port、安全与目标分派，不拥有WiFi/MQTT重连。网关child executor仍唯一，不能各北向复制Bridge。
2. 下一源码批次先处理真实前置耦合：Cloud终态Error会`client.end/wifi.stop`，SMALL只有2 transport，单authorization provider易被后加模块覆盖。分别回归，不为三通道直接切STANDARD或用Sketch宏改变ABI。
3. 发现首选mDNS/DNS-SD，建议独立V2 service type；SRV给端口，TXT只放小型提示。MAC/IP/名称均非身份，Hub可发现不代表所有child在附近/Ready。不先增加自定义UDP广播。
4. App已有mDNS插件但尚未接V2；Android后端是JmDNS而非系统NSD。复用共享发现协调与精确watch生命周期，补网络代际/iOS服务权限；旧`ws://IP:81` JSON不是可复用的V2数据面。
5. 首个纵向链路从ESP32普通WiFi self开始，再同模块接WiFi+BLE/Hub；R4/Wio分别适配/测资源。普通WiFi也使用有限会话授权，不能套Hub可信UTC或GatewayAccess；首次在线、已认证会话有限离线的边界不扩大。

安全profile、wire/发现字段、平台容量与默认启用策略仍待S3b冻结。S3c只是本地可通信；S4去周期与Ready切路才具备无缝功能，S5验证体验。
本次不修改生产代码/固件/Key/Access、不部署或暂存；不把文档完成记成S3功能完成。原S2管理交错/SDK reset与发布长稳仍欠，100轮交替继续暂停。

### 6.9 S3a-1a 路径与资源前置改动（§10.110）

- 普通WiFi/WiFi+BLE共用lifecycle不再因非鉴权Cloud Error/station Fault停整个Client；状态为`Degraded`，其他transport继续poll。显式stop和启动失败回滚保留，Arduino诊断名称同步。
- 登录接口将Key/device/session撤销均映射成`AuthenticationRequired`。本批仍以`AccessFault`保留原关闭边界；S3b取得明确本地授权/撤权scope后才能进一步隔离，不把登录失败视作普通断网而保留未知权利。
- Runtime统一native断开和transport非Online的peer退休，补漏的control window清理，按transport/session定位；注册不同provider/control指针拒绝覆盖，BLE attach失败只释放已取得的对象。不新增并行组件/传输框架。
- Win32 SMALL统一编译参数：2/2→3/3，frame256 B、固定24-field存储824 B不变，Device1664→1744 B（+80 B），WiFi lifecycle20 B不变。修正默认大小断言误套自定义容量；新三槽测试独立上限1744 B。默认仍2/2，不新增LAN缓冲/依赖或ZIP。
- Host/ASan53、架构与源码whitespace检查通过；平台编译及失败尝试在GATT§10.110记账。本批无真机、部署、Key/Access/拓扑操作或暂存/提交/推送；登录freeze经审查升v31，wire不变。

下一批S3a-1b按通道鉴权/control分派和组合容量，随后1c唯一child执行/需求抽取；当前singleton注册保护不是分派实现。EdgeHub自身生命周期、BLE自身Fault影响Cloud、底层MQTT阻塞仍须单独收口。
完整实施/资源表见[通用LAN方案§9](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。S3b/c尚未完成，S4功能/S5体验不变；100轮不恢复。

### 6.10 S3a-1b 通道访问绑定与分派（§10.111）

TransportHub按原有槽位同时登记transport、authorization、control；Client/Runtime旧全局setter删除。移动/复用槽位时binding同步，运行期间不能改注册。BLE-only/WiFi+BLE正式接入原子绑定，删除分步回滚和BleOnlyRadio多余client指针。
Runtime按transport/session完成Hello能力/方法、鉴权、安全建立能力、控制与PresenceKey、queued/closed；Cloud不再声明BLE管理能力。同sessionId不共享权限，局部关闭不reset其他provider，全量reset按provider去重。

Host/ASan53项通过；新三通道安全分派、错方法/未知session、局部断开、重编号、启动回滚/析构解绑覆盖。Win32 SMALL 2/2 Device1672 B（本批+8 B）、3/3 1760 B（+16 B），frame/组件存储及默认容量不变；不是LAN整体成本或真机门禁。完整四配置ABI、Arduino结果见LAN§10/GATT§10.111；无新增源码文件/依赖，freeze审查升v32。

当时下一批1c现已按6.11完成；EdgeHub原生命周期/周期尚未变。S3b有限授权与S3c LAN仍待实现，不能将现有ControllerGrant/Access secret当LAN授权。S4功能/S5体验、S2剩余硬件门禁和100轮暂停不变。App/Service只改文档，未真机/烧录/部署/改凭据或暂存/提交/推送。

### 6.11 S3a-1c 唯一 child 执行与需求所有权（§10.112）

`BasicGatewayChildExecutor`现为每个活动child唯一南向所有者：一组回调、一条DirectSecure连接、一个BBP/2序号空间、
一个在途exchange、一个recent replay和一份帧workspace。Cloud adapter仅持自己的authority、lease、projection和
correlation；管理revoke/renew也直接使用executor。原Bridge内重复的child/connection/sequence/control状态已删除。

Cloud与Local需求使用固定bit，不复制scheduler或队列。Cloud退休仅取消Cloud owner的请求/需求；Local仍有需求时
executor保持wanted，SDK disconnect才结束共享南向会话。Host双owner测试已经证明Busy不占序号、ACK回原caller、
recent replay和后继sequence正确。ESP32活动context+40 B/child，两个槽BSS+88 B；详见GATT§10.112。

这只是复用接缝，不是LAN已实现：S3b仍须冻结self/child有限授权和单一安全profile，S3c才接通用LAN模块；
Manifest/State多上游事实分发与S4持续策略也不得由单executor回调测试提前宣称完成。

### 6.12 S3b-1 有限本地授权核心（§10.113）

已冻结一个与载体无关的`Noise_NNpsk0_25519_AESGCM_SHA256` profile和canonical CBOR
challenge/grant。普通WiFi用self policy；Hub注入exact-child policy，授权必须同时匹配logical id、instance、accessEpoch、
topologyVersion，不把GatewayAccess、ControllerGrant或Cloud admission改名复用。DeviceKey只在Service和目标设备侧参与
HMAC/HKDF；grant wire不含secret/PSK，App经认证HTTPS取得短期PSK且只存RAM。

一个产品共享`LocalAccessAuthority<2>`，不同socket只保留各自challenge；授权期限由接收方单调时钟从首次接受锚定，重连
不续期，Key/ACL代际提升、重启、过期、重放和篡改均fail closed。设备协议/密码/lease及Host/32位/三平台编译门禁已完成；
Service签发、App消费和真实WebSocket/mDNS属于下一批S3b-2/3，S4持续路径选择仍未完成。

### 6.13 S3b-2a 三端有限授权模块与多目标容量（§10.114）

三端共用golden、Service签发核心/HTTP边界、App无UI消费已完成软件验证，详见通用LAN方案§13。
Service真实持久authority/签发期限及精确撤权仍未接线，生产HTTP路由未注册；不能以应用进程Map代替跨重启权威。
App依旧一条账户MQTT，多child应共享一条App→Hub WS；连接认证和每目标权限独立，追加目标不得重新建连接或重置Noise状态。

当前2个lease槽按授权目标计数，不是2个任意多目标用户；过期隔离占槽15秒是防重放措施，不能成为满载正常业务的固定空档。
S3b-3须落实多目标授权增补/退休和续期容量。全局authorityRevision退休全部旧lease，与局部child/ACL退休分开设计；
不把“拒绝B越权”当成“撤A不影响B”已经验证。下一步S3b-2b持久事务/撤权，再socket/发现，S4/S5不变。

### 6.14 S3b-2b1 持久授权与精确退休事实（§10.115）

Service真实PG权威/幂等/固定期限与事务退休outbox已通过联合30项回归，修复签发与分享变更并发漏撤的隔离级别缺口。
普通self与Hub child共用repository，局部ACL变化只退休exact授权，不复制权限缓存或追加账户MQTT。
每recipient64活动授权/每授权64请求是服务器保护，不代表设备两槽或socket容量已扩大。

下一批S3b-2b2先补recipient权限版本认证同步及精确退休wire/送达ACK，必须同时阻断未安装的在途旧grant。
数据库保留B不是B真机持续通信已过；生产HTTP/WS/mDNS仍未接线，满槽续期和同连接多目标仍属S3b-3。
本批不改App/MCU生产代码，不部署、不改绑定；完整证据与下一步在LAN§14和Service计划§3.2。S4/S5里程碑不变。

### 6.15 S3b-2b2a 通用控制接收与挑战失效（§10.116）

设备/Service共用认证Hello、Enable/Disable、exact Retire及ACK。设备receiver和authority不依赖载体/平台，普通WiFi和Hub共用；
每个product一个controlSession，MQTT重连不重建。退休A只清A的lease，其他旧pending挑战须重取，B已建立的lease继续；
最新消息exact重传只回ACK，不打断后来新发的挑战。协议/HMAC/接收软件和32位ABI已验，详见LAN§15。

尚需下一批2b持久发送/ACK恢复及Broker/product业务权限接线，当前不能把“receiver应用成功”当成四端撤权/物理连接隔离通过。
自然过期满槽续期仍属S3b-3，S4持续切路/S5体验未完成；没有新增账户MQTT、secret表或撤销黑名单。

### 6.16 S3b-2b2b1 持久控制与签发就绪（§10.117）

当前完成单recipient持久pending/exact ACK/恢复和ready门禁；Hello要求当前云登录上下文，不再接受context-free旧MAC。
旧pending补ACK后才升revision；覆盖、放弃与精确送达分开留证。无新增MQTT、MCU持久槽或App页面改动。
真实PG联合34项及Host/ABI通过；下一批仍为有界投递worker、Broker/产品控制与主动到期释放，之后才HTTP/单socket多目标/WS/mDNS。
详见LAN§16、Service计划§8；S4/S5与真机无缝尚未完成，不以数据库恢复代替物理通信门禁。

### 6.17 S3b-2b2b2 投递预算与调度（§10.118）

先提交预约、8次/当前proof上限、有界退避/超时和清理模块已实现，发布/超时不伪装ACK；复用一个共享scheduler而非每child建队列/连接。
旧Hello/进程重启不重开预算，新证明恢复尚须接入产品自动流程；既有有限lease不因MQTT暂断清空。
真实PG联合47项、后台任务16项通过；真实Broker载体/产品权限与主动到期尚未接线，见Service§9、LAN§17。
S3c WS/mDNS、S4/S5无缝不提前标完成，本批不改MCU/App源码或部署。

### 6.18 S3b-2b2b2 Service/Broker控制接线（§10.119）

原物理MQTT数据topic用0x36控制载体，WiFi/Hub同路径、不新建连接；PG唯一账本，Broker在线QoS0、有界取消与可信上行。
内部HTTP和唯一start/stop已接源码，真实PG50项、Broker专项7项/既有165项回归通过；未部署，MCU只有enum占位、App不改。
下一批产品控制队列/ACK、Hello与自动proof恢复、同authority主动到期与真机门禁，再S3b-3/WS/mDNS、S4/S5；见Service§10。

## 7. 三端执行归属

- 设备库：S2连接池、逐child路由/管理隔离；S3通用LAN/发现与self/child授权消费；S4取消Cloud状态对整个radio的独占，保留SDK/安全边界。
- App：S1默认代理及scope清理；S3共用LAN发现/认证会话，分别路由self和Hub代理child；S4稳定页面会话/路径选择。不为每个child新建账户MQTT，不提前增加UI开关。
- Service/Broker：保留logical child路由和per-child ACL/ledger；S2检查多child同Hub隔离；S3提供通用有限本地会话授权，self/child分别解析权限与撤权范围。
  不让旧Cloud grant跨本地路径获得更长寿命；不用新的JSON组件协议替代BBP/2。

## 8. 参考与证据范围

- [ESP-IDF 5.5.3 ESP32-S3多连接指南](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32s3/api-guides/ble/ble-multiconnection-guide.html)：能力受内存与连接参数影响，不等于本产品已经通过多连接。
- [ESPHome BLE Proxy](https://esphome.io/components/bluetooth_proxy/#how-active-connections-work)：区分连接槽、持续/短连接和广播；其RAM数据不直接套用Blinker。
- [Tuya网关局域网接入](https://developer.tuya.com/en/docs/connect-subdevices-to-gateways/quick-start?id=Kd0g2gbi56ott)：参考本地鉴权访问网关的拓扑，不照搬协议/密钥算法或假设全部型号双连接。

S1验证：App通信/页面39文件385项，BLE服务/准入/网络/Manifest缓存4文件32项，共417项通过；
本地Web构建、设备Host重建及50项CTest、设备架构检查通过。没有修改MCU源码/SDK/Key/绑定或部署APK/Broker。
S1详细结果在GATT §10.97；S2a在§10.98，S2b-1在§10.99，S2b-2a在§10.100，S2b-2b在§10.101，S2c-1在§10.102，S2c-2在§10.103，S2c-3a在§10.104，S2c-3b/3c与A四端短测在§10.105。未通过的阶段不自动改为完成。
