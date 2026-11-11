# Device V2 Edge BLE Gateway（GATT）实施方案

> **2026-09-22 执行入口：[新进度与后续计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md)。** 普通核心版优先，网关保持 preview/internal；下方历次“当前/下一步”不是执行队列，不自动扩多 child/新南向。

> **当前执行入口（2026-09-11）：Service跨分片§30.3。** R3b分享路径软件收口：主体停用/注销/设备停止衔接原持久投递和退休，9项实际PG/双Broker组合、14项PG专项、86项相关回归通过；Gateway正式清单通过（最后组99项）。下一步R3c公开分享入口一致性→R4最小真机→普通设备基础B1～B4，不默认Hub/S4。范围与证据边界见[基础收口路线§7](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)，详情见[Service§30](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md)。源码开关仍关，无本批手机/MCU/部署，SSID仍mostfun2026。下方为历史快照。

> **历史现场入口（2026-09-10，§10.143）：首次账户/分享路由准入、竞态及错误分类已修，真实新账号MQTT/BBP ready。** 10项软件、13项隔离PG runner通过；Gateway已更新，shard0备份后对齐shard1联调镜像，原账号恢复。receiver normal route0、目标normal route1，分享409此次为真跨shard；无grant，测试邀请已撤销，板未动。按[LAN§44](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)/Service LocalAccess§34.3先补内部跨shard合同/门禁，再真实ACL→Retire ACK→提前关闭/恢复，最后Hub/S4。源码LAN关闭，P3未整体通过，不强改SQL路由、不宣称无缝、不跑100轮。下列入口为历史快照。

> **历史实现入口（2026-09-09，§10.127）：可信self身份查询、LAN组合与原页面/账户/网络owner已接线。** 云端立即准备，LAN验证ready后首选；命令不跨路重发，State/Event及Manifest复用原接口/缓存。Service专项17、隔离PG/正式路由30项通过；App验证见LAN计划。默认开关关闭、未部署/刷板，不声明无缝。下一步按[LAN§27](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#27-原页面owner与可信self身份接线gatt10127)部署隔离backend/手机可信HTTPS，再普通WiFi隔离身份真机；不扩网关协议。

> **历史实现入口（2026-09-09，§10.126）：开发HTTPS终止、BLE/LAN共享self BBP核心、LAN安全open到Hello/Manifest/State及原生关闭结果传播已完成源码/软件验证。**
> App关联250项、Service原专项24项及新增真实TLS4项通过。公开开关仍关闭，未部署/安装APK/刷板；按[LAN§26](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#26-可信https与共享直连bbp核心gatt10126)挂原页面/账户/网络路由owner，满足backend隔离/手机证书信任，再普通WiFi隔离身份真机。不混用账户Route/Delivery与设备self Session，不扩网关协议、不宣称无缝完成。

> **历史实现入口（2026-09-09，§10.124）：ESP32 SDK WebSocket端口、独立mDNS和普通WiFi可选extension已接源码并交叉编译，默认不开LAN。**
> 57项Host/ASan与20,000 fuzz通过；含端口源码+mock SDK，不是真实HTTPD/lwIP/手机互通。按[LAN§24](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#24-esp32实际监听器mdns与普通wifi扩展gatt-10124)继续公开HTTPS签发、App受限原生WS、隔离身份真机闭环/任务与堆峰值；不新增网关协议或100轮交替，无生产LAN/无缝完成声明。

> **历史实现入口（2026-09-09，§10.123）：单self LAN安全消息传输、Runtime权限上限与App最小会话客户端已实现，软件门禁通过。**
> 按[LAN§23](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#23-单self安全消息链路与runtime权限接入gatt-10123)继续实际有界WS端口/mDNS/产品接线，不扩散控制协议或网关多目标。尚未开放LAN生产入口；假消息端口不是实际WebSocket，真实断流/Key刷新/无缝仍待验收。

> **历史实现入口（2026-09-09，§10.122）：真实PG→Broker→ESP32 Hub self控制闭环通过，修复Query早于BBP Hello的启动误断线。**
> Enable/Disable/精确Retire、ACK提交丢失、新proof自动恢复、Broker/MCU重启已实测；174项Broker关联回归通过。A/B绑定未改，临时故障已清除。下一步按[LAN§22](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#22-真实控制闭环与启动query竞态修正gatt-10122)进入普通WiFi单caller/self安全LAN纵向链路；普通WiFi产品、真实caller断流与实际刷新Key后的旧登录负测仍须独立验收，不能以Hub self或Broker版本检查替代。当前无生产LAN/无缝完成声明。

> **历史实现入口（2026-09-09，§10.121）：新鲜Query/Status与后续投递耗尽恢复已接设备、Service、Broker，软件/隔离PG通过。**
> 默认不开LAN或额外控制队列；不改变原MQTT/PG预算，不以查询替ACK或扩大精确撤权。下一步直接真PG→Broker→MCU控制闭环，再普通WiFi LAN；新WiFi已确认mostfun2026，板卡角色/服务地址仍需核对。当前合同与证据见[LAN§21](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)，收口顺序仍按LAN§19，不能视为LAN/无缝完成。

> **当前顺序复核（2026-09-09，仅文档）：先完成§10.119剩余设备控制/Hello/ACK/自动恢复与真机控制闭环，再直接做普通WiFi单caller/self的安全LAN纵向链路。**
> 必要控制面前置保留；多目标/满槽无空档续期移至网关扩展与S4，不再阻塞第一条LAN业务。退出条件与执行依赖统一见[LAN计划§19](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#19-主线复核先收口控制闭环再做首条lan业务链路当前执行入口)。以下批次描述保留历史证据，不代表LAN或无缝已完成。

> **历史执行入口（2026-09-09，§10.119）：S3b-2b2b2的Service/Broker控制载体及生命周期已接入源码，设备产品接线待做。**
> 复用原物理MQTT数据topic的0x36/在线QoS0；唯一PG预算、可信会话上下文、双向HTTP、取消/容量与清理start/stop已接通。PG联合50项、Broker专项7项及既有165项回归通过；仅loopback模拟器，未部署/刷板。下一批普通WiFi/Hub产品队列/ACK、Hello和自动预算恢复、主动到期/真实控制门禁，再HTTP/WS/多目标续期；Service§10、LAN§18为当前入口。100轮暂停，无缝未完成。

> **历史执行入口（2026-09-09，§10.118）：S3b-2b2b2有界投递/预算/清理核心完成，真实载体与产品接线待做。**
> 预约先提交、每当前proof最多8次且退避有界，重启/旧Hello不重置预算，迟到ACK仍可恢复；复用既有scheduler，不加MQTT或内存重试状态。真实PG联合47项通过，但发布端仍是测试载体。下一批Broker/产品接线及自动预算恢复、主动到期和真机故障门禁，再HTTP/多目标续期/WS/mDNS；见Service§9、LAN§17。100轮暂停，无部署/刷板。

> **历史执行入口（2026-09-09，§10.117）：S3b-2b2b1持久命令/ACK恢复与签发ready门禁已实现，Hello补当前云登录绑定。**
> 一个recipient一个pending，重连不换字节/序号，旧Hello不能回切启动会话；废弃/全局屏障覆盖与精确送达分别留证。下一批有界投递worker、Broker/产品接线及主动到期释放，再多目标续期/WS/mDNS。详见Service计划§8、LAN§16；生产LAN未开放、100轮暂停，本批未部署/真机测试。

> **历史执行入口（2026-09-09，§10.116）：S3b-2b2a权限同步/精确退休/ACK wire和设备接收核心已完成软件门禁。**
> 使用独立HMAC域、设备级control session和严格序号；精确退休删除A的lease并使旧pending challenge失效，保留B既有会话；exact重传不重复打断握手。下一批2b持久发送/序号恢复、Broker及产品接线、清理调度，再单socket多目标/满槽续期、WS/mDNS。生产LAN接口仍未开放，100轮仍暂停；详见LAN§15与Service计划§7。本批未部署或真机测试。

> **历史执行入口（2026-09-09，§10.112）：S3a-1c唯一child executor/需求与Cloud adapter拆分完成。**
> child回调、南向序号、单在途/重放、管理事务与原512 B workspace归入唯一executor；Cloud只保留准入、需求租约、投影及correlation。Host/ASan53、架构/单库清单及ESP32 3.3.11 EdgeHub编译通过；下一批S3b有限授权，LAN安全wire/真实通道、S4/S5尚未完成。未烧录/部署/改凭据或暂存/提交/推送。

> **历史入口（2026-09-09，§10.111）：S3a-1b按通道访问绑定/协商/分派、原子解绑和容量ABI完成。**
> 删除全局auth/control setter；BLE-only/WiFi+BLE复用原有TransportHub槽位，Cloud不再声明BLE认证/控制能力。Host/ASan53及四种32位配置通过；平台编译证据见§10.111。

> **历史入口（2026-09-09，§10.110）：S3a-1a非鉴权路径故障隔离、peer退休、注册保护与容量ABI完成。**
> 普通WiFi/WiFi+BLE非鉴权Error不再停整个Client；鉴权失败含撤销语义仍AccessFault关闭。Runtime补缺失control window清理，BLE attach回滚不动其他owner；Host/ASan53通过，32位SMALL三槽结构较默认+80 B，默认容量不变。
> 下一批当时为S3a-1b/1c；当前完成状态及后续以§10.112为准。

> **历史执行入口（2026-09-09，§10.109）：通用LAN/WebSocket/mDNS方案分析完成，修正S3仅面向网关的范围。**
> 普通WiFi、WiFi+BLE、WiFi网关组合相同LAN模块，网关只增加child目标/权限与唯一南向执行；发现、鉴权、BBP业务分离。详见[通用LAN方案](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)、共享方案6.8、需求合同6.47。
> 下一源码批次先处理Cloud终态Fault停止整个Client/IP、SMALL两transport及单鉴权provider边界，再共用LAN与child执行模块；S3b冻结共同权限/一个安全profile，S3c先普通WiFi self再WiFi+BLE/Hub。未实现LAN生产通道或进行新真机门禁；S4功能/S5体验不变，100轮暂停。

> **历史执行入口（2026-09-08，§10.108）：双占用permit Busy不驱逐、A Direct物理让出/B继续控制、MQTT身份换代恢复短测通过。**
> 本批清除测试工具的全Hub拦截/恢复假设，补33＋5＋3项工具回归；未改生产固件/SDK/协议或Key/Access。账户MQTT不变，最终A/B计数6/6、scope归零、Inspector关闭。
> 无缝功能在S4实现、S5体验验收；当前旧10s/6s周期未删除。下一批S3a抽取逐child执行/需求与Cloud adapter，S3b有限本地授权细化；S2管理变更交错/SDK reset资源在S4真机准入前收口。详见共享方案6.6/6.7；100轮继续暂停。

> **历史执行入口（2026-09-08，§10.107）：S2e已配套部署，A/B普通页面、双native连接/并行ACK及独立关闭短门禁通过。**
> 已补无Key child的Broker身份元数据、NULL安全迁移与未知目标请求级错误；不生成子设备MQTT凭据。Broker165/真实Redis、完整PG、协议22与硬件工具3项通过。
> 继续S2d双占用permit/管理/Direct隔离、MQTT/SDK换代资源，再S3有限本地授权→S4去周期。旧10s/6s周期仍中断持续需求，不能把短测称为无缝通信；100轮暂停，原绑定保留。

> **历史执行入口（2026-09-08，§10.106）：S1页面已部署验证、COM3 B正式绑定；先补S2e Service/Broker逐child路由。**
> A普通页面Cloud button/ACK和scope清理通过；B真实relay/receipt/proof已Active，无DeviceKey。清单Cloud能力遗漏已补源码/PG测试，暂未单独部署。
> 现场日志确认Service仍拒绝同Hub多Active，Broker仍单RouteContext。先共享方案§6.5软件隔离和配套部署，再复用现有A/B做双数据面；不清绑定、不恢复100轮。

> **历史执行入口（2026-09-08，§10.105）：S2c-3b/3c空槽借还、目标隔离与两份活动context软件完成，已开始四端真机。**
> COM11候选保留原Key/Access/绑定，现有A的Cloud Manifest/State/Action/ACK、permit扫描负测及关闭通过；Host52/架构/347文件发布与ESP32两构建通过。
> 当前手机尚非S1页面路由版本。下一批先对齐App，再用COM3建立B并验A/B同时业务、独立关闭/管理/资源；不重置A，不恢复100轮，不把单child短测当无缝切换完成。

> **历史执行入口（2026-09-08，§10.104）：S2c-3a子设备管理与Hub permit作用域已拆分并接入实际Product。**
> 子设备mux/回包归各自context，删除重复permit生命周期；逐child准入与permit准入分开，全局管理串行及原关闭屏障保留。
> Host/ASan52、架构/345文件发布和ESP32 3.3.11两构建通过。下一步共享方案6.4的3b槽位/目标准入与permit借用→3c双业务槽→1Hub+2child真机；100轮暂停，未刷板/部署。

> **历史执行入口（2026-09-08，§10.103）：S2c-2双原生Port槽与独占发起权的软件实现完成。**
> Central29场景、SDK单槽容量独立测试、Host/ASan52、架构/345文件发布及ESP32两构建通过。Host56 B、Port1068 B保持，实际Product仍只接1个child。
> 下一批按共享方案6.4拆管理/permit/radio的全局child假设，再接B的业务上下文；不直接复制两份旧radio。1Hub+2child真机未验，100轮仍暂停，未刷板/部署。

> **历史执行入口（2026-09-08，§10.102）：S2c-1唯一Central Host及回调生命周期围栏已接入实际Product。**
> 删除Port自有SDK生命周期与raw Port指针callback；登记2个借用者仍仅准入1个原生活动，不把登记容量冒充双ACL。
> 中央Port18场景、Host/ASan51、架构/345文件发布检查及ESP32 3.3.11两构建通过。下一批双原生槽/发起权隔离→B纵向接线→1Hub+2child真机，100轮暂停，未刷板/部署。

> **历史执行入口（2026-09-08，§10.101）：S2b-2b双记录后端、轻量绑定与全目录访问重置。**
> [共享连接实施方案](DEVICE_V2_GATEWAY_SHARED_CONNECTION_IMPLEMENTATION_PLAN.md)6.1～6.3已更新；实际Platform/Product消费2对持久记录、1个GATT活动上下文，原0号NVS名称保留。
> Host/ASan51、架构/发布检查和ESP32 3.3.11两构建通过；修复故障模型下NVS清理重试遗漏commit确认。下一步唯一Central Host→双原生槽→1Hub+2child真机。100轮暂停，未刷板或部署。

> **历史执行入口（2026-09-08，§10.100）：S2b-2a管理命令定向分派与无副作用terminal重投。**
> Host50、Broker完整V2、架构/发布检查及ESP32 3.3.11两构建通过；实际Product/radio接入目录管理router，无新终态协议或持久完成账本，详见§10.100。

> **历史执行入口（2026-09-08，§10.99）：S2b-1记录目录、非秘密身份索引与凭据定向下发。**
> 两个记录对的软件隔离/重建通过，实际Product接入目录，不增持久账本或改变记录格式；本批结果见§10.99。

> **历史执行入口（2026-09-08，§10.96）：需求租约/短物理承诺分离已实现；定向故障、持续续租、管理、10/30轮工程门禁通过。**
> App按Ready实际剩余期自适应续租；Broker仅在GATT接入层裁剪新承诺，核心/wire仍允许30 s，旧承诺不提前清空。
> Host50/App380/工具126/Broker98+完整V2通过；设备固件不变，新APK dc755c23及Broker f5已原位部署，原30 s请求在单次/重复换代中恢复。
> 完整30轮457.509 s通过，下一阶段100；D4b/N2～N5与发布长稳仍独立保留。未新增协议字段、MCU状态或重试队列，详见§10.96/需求合同6.34。

> **历史执行入口（2026-09-08，§10.95）：修正Cloud owner提前唤醒丢失续租，以及旧投影数据中断新连接两个边界。**
> 两项均有旧实现失败/修正版通过的回归；Host/ASan50、App376、工具122、Broker95+完整V2、架构与ESP32/Android构建通过。
> 新Hub71e82b72与APKee981b3e已原位更新，child/绑定/Access/Key不动；短2和10通过，30在第5轮跨MQTT换代恢复超时，100未执行。
> 不增加超时、队列、调度器或新协议。带追踪旧固件30轮在第30轮失败，保留原证据，详见§10.95/需求合同6.33。

> **历史执行入口（2026-09-08，§10.94）：F4管理负测与10次通过；30次在第9轮Cloud恢复超时，已停止并定位。**
> 复用原permit-join Open/Get/Close与普通App页面；两次串口均证实permitState=Ready/relayActive=1。
> 原管理owner关闭后才Ready/native connect；取消Direct不关闭管理窗口，原窗口关闭后继续观察，无迟到native连接。
> 本批不改生产源码/固件/APK、SDK、凭据或绑定；最终工具122项、Host/ASan50项、Broker95项和架构检查通过。
> 10→30→100逐阶段执行并单独记录，不自动重试或把管理短测当作发布长稳；详见§10.94/需求合同6.32。

> **历史执行入口（2026-09-08，§10.92）：F3b预算拆分、新网络恢复与四端配对短测完成。**
> 唯一radio获取16 s/原生占用10 s/总上限26 s/真实空档6 s；无新Port API、wire或owner表，Flash+112 B/RAM+8 B。
> 手机离线页Ready9.988 s；Cloud完整可用窗口由2.270 s增至7.465 s，首次不可控仍13.837 s；同一owner、counter0保持。
> Host/ASan50项、工具112项、架构/ESP32编译通过；Hub已恢复mostfun2026联网，手机实际Cloud页fresh/可控，离页零残留。
> **下一批进入F4配套bit18准入与在线普通页单次Direct→Gateway→Direct，过短门禁再10→30→100。** 工程参数不是发布承诺，bit18本批未开启。
> 详见§10.92/需求合同6.30。无App/Service业务源码或APK改动，未暂存/提交/推送。以下为历史记录。

> **历史执行入口（2026-09-08，§10.91）：F3a原两板/手机/独立Cloud caller配对短测通过，性能预算仍需修订。**
> 手机离线普通页Ready9.948 s；Cloud不可控13.759 s后恢复，持续owner和wanted=1保留；第二次正常让出后也恢复。
> 发现扫描/等待/认证均消耗同一个10 s预算，Cloud恢复后只余2.270 s窗口。**下一步F3b分阶段获取/占用预算，不启用bit18或10→30→100。**
> 工具修复只观察第一次恢复的误判；112项工具与架构检查通过，未新增生产协议/owner表/Action。详见§10.91/需求合同6.29。
> USB接回后已校验身份并恢复Hub §10.87正常固件，手机账户ready且零连接残留。现场改为mostfun2026，旧基线尚未联网；下批先同步本机网络/TLS配置再做F3b。以下为历史记录。

> **历史执行入口（2026-09-08，§10.90）：F2b-C目录冷启动/Manifest隔离及独立离线普通页短门禁完成。**
> authority+account有界非秘密目录，在线/恢复复用模型转换；账号/请求代次、共享删除、旧BLE回调均有隔离。
> 最终清理版普通离线页Ready3.025 s，到期SDK关闭55 ms，Android确认目标消失后继续6.049 s空档，零残留；恢复Cloud fresh/counter1。
> App42文件357项、工具108项、类型/构建/架构通过。**下一步F3：独立持续Cloud caller与离线手机配对，不是直接10→30→100。**
> Hub仍10.87正常固件、bit18关闭；10/6参数及四端公平未冻结。详见§10.90/需求合同6.28。以下为历史记录。

> **历史执行入口（2026-09-07，§10.85）：共享扫描、Android失败/配额、WiFiProv扫描注入及附近状态期限已落地并安装。**
> App266项、Android配额2项、硬件工具86项通过；真机Direct前5次成功，第6次50ms内明确THROTTLED，不再误等5s设备超时。
> 原绑定Direct恢复2782ms、WiFiProv/Direct扫描互斥与旧stop隔离、真实后台原生停止、Cloud唤醒/释放均通过；无新Action、无板端烧录/重置。
> **扫描过频负测通过≠10次连接全部通过。下一步补原生负测/配网交接证据，再测Hub关闭到广告恢复、冻结离线公平及配套bit18。**
> bit18仍关闭，SDK厂商失败回调强制负测、实际WiFiProv完整配网、四端公平10→30→100及发布长稳仍独立记录。见§10.85/需求合同6.23。

> **历史执行入口（2026-09-07，§10.84）：新 Hub/App/Broker 已配套部署；按需唤醒→控制→释放→再次唤醒短测通过。**
> 新网络下保留原绑定/凭据，仅烧录 Hub application；两次离页均回 Stopped/wanted=0，MQTT 管理连接保持，唯一 Button 为 0→1。
> 隔离 Hub 抢连的 Direct 10 次测量在第6次失败：前5次 Ready 2.845～3.859 s，ADB 明确报告 Android 扫描过频。
> **下一步先统一 App 物理扫描占用/关闭/失败通知与配额，再复测10次基线；不能据这5次冻结公平窗口或继续30→100。**
> bit18仍关闭，离线手机/在线Hub持续需求的公平策略、正常页面四端Direct交替及发布长稳均未完成。
> 硬件工具回归82项（新工具7项）、Broker151项及编译/安装通过；完整证据和剩余边界见§10.84、需求合同6.22。以下为历史记录。

> **历史执行入口（2026-09-07，§10.83）：实际 App selector、原 Direct scope 与原生连接/关闭准入的软件接线完成。**
> 受管 child 先预留并等待物理 Ready，再认证/同步；原 caller group 共用一个物理 owner，没有新增连接引擎或用户 API。
> Presence 离线不等于 Hub 已让位；取消/账号退休/旧回调不能启动、复活或接管后继连接，原生关闭未结束不复用工作位置。
> App 44文件382项、Broker151项及完整回归、跨仓35项、Host50项与架构/类型检查通过；详情见§10.83、需求合同6.21。
> **下一步：测量并完成手机离线/Hub在线持续需求的本地公平策略，再配套 bit18 激活及同绑定四端 10→30→100 门禁。**
> bit18仍关闭，无本批MCU/APK/镜像部署、reset/重绑或新真机结论。以下为历史记录。

> **历史执行入口（2026-09-07，§10.80）：Broker owner→Hub生产控制接线、显式续期和准入重放边界完成。**
> 复用owner只读投影/原并发额度；新revision必须取得新Yielded，旧Ready不延长。控制编号高水位提升到既有MQTT claims状态。
> 真实App入口→HTTP权限→Hub路由软件测试通过；覆盖child Hello/closure/恢复、同MQTT重新准入、错误Hub绑定及旧回执。
> Broker/协议165项、完整Device V2回归、跨仓20项、Host49/49与架构通过；无MCU/APK/镜像部署或新真机证据。
> **下一批直接做唯一product物理决策/离线公平，再补App Ready消费与四端门禁；bit18继续关闭。** 不重做owner/wire/driver。
> 详见§10.80、需求合同6.18。以下为历史记录。

> **历史执行入口（2026-09-07，§10.79）：Hub Direct控制wire、48 B receiver租约、RouteBridge适配及Broker有限交换核心完成。**
> 修正Probe必须绑定当前Hub incarnation，exact回执不得从接收时间续期；沿用已有owner、准入、缓冲与连接机制。
> Host49/49、含新seed的200k fuzz、协议22+Broker70项、跨仓20项与完整Broker回归通过；资源编译结果见§10.79。
> **生产owner→Hub driver/显式renew、唯一物理决策/离线公平、App Ready消费尚未接线，bit18继续关闭。**
> 下一批从这些接线开始，不重做wire/lease；没有新部署/烧录/reset或新的Direct交替真机证据。需求合同6.17给出精确边界。

> **历史执行入口（2026-09-07，§10.78）：Direct northbound预留wire、App发送与真实authority接线完成。**
> 独立0x1e/0x1f、bit17协商；复用owner核心，当前Direct Admin/Active route/epoch/exact MQTT投影共同授权。
> cloud分享不能推导Direct权限，未支持bit18退让能力的Hub明确拒绝。accepted不是Yielded，App页面/Hub调度未切换。
> 下一步只推进Hub receiver挑战与真实释放回执、唯一在线/离线物理决策及App消费，不再重做预留wire。
> 详见§10.78、需求合同6.16；没有新固件/APK/镜像部署或新的Direct交替真机结果。以下为历史记录。

> **历史执行入口（2026-09-07，§10.77）：Direct优先权语义与共享owner核心隔离完成，软件门禁通过。**
> 一套owner/id/timer/容量治理区分cloud与Direct；Direct单候选、独立到期、精确binding与权限期限，改用途/绑定不静默迁移。
> 现有cloud-only编码明确拒绝Direct，未开启UI或新wire feature；设备固件/已部署App与Broker仍为§10.76基线。
> 下一批完成Direct独立wire/golden与Hub真实Yielded回执，再配套唯一物理决策和App消费；不再重写owner/取消核心。
> 详见§10.77、需求合同6.15；**本批不是新的四端Direct联调通过，idle/在线与离线公平仍未完成。** 以下为历史记录。

> **历史执行入口（2026-09-07，§10.76）：共享连接机制已接入普通路由与全部管理连接入口，四端短复测通过。**
> RouteBridge不再自行重连；exact关闭屏障不随proof命令reset丢失，修正同步关闭后隐式重连的边界。
> Host ASan48/48、20万fuzz、架构/工具测试通过；3.3.11固件Flash+260 B、静态RAM+32 B。
> 三个relay短场景与实际设备页Button9→10/逻辑需求回收通过；没有清Access、重绑或更换child固件。
> 下一批先冻结Direct独立有界优先权合同，再同批实现三端主动退让、离线公平和按需idle；详见§10.76、需求合同6.14。
> **尚未启用物理idle，也不是Direct交替或发布长稳通过。** 以下为历史记录。

> **历史执行入口（2026-09-07，§10.75）：permit-join双向异步交接及四端短复测通过。**
> Opening按原期限等待，Closing保留原生资源所有权；重置/无效消息不再阻塞清理，临时交接不清有效路由准入。
> 复用现有App、Broker与绑定；不改wire/API/凭据，不重置子设备。完整边界及证据见§10.75与需求合同6.13。
> 三个relay短场景、故障后新窗口恢复及真实页面Button 8→9/scope回收通过；最终owner0/wanted0，Inspector已关闭。
> 下一批仍需集中普通/管理连接入口并配套Direct在线/无云公平退让；物理idle尚未启用。以下为历史记录。

> **历史执行入口（2026-09-07，§10.74）：物理释放边界与SDK竞态修正，Host及双板/App短复测通过。**
> 记录退役不再冒充ACL关闭；取消尚无record session的连接也有完成通知，exact管理清理权保持到原生释放。
> 修复CONNECT/cancel竞态、SDK拒绝取消/终止时提前Idle、stuck connect被stop误判等；不改变wire/API/凭据。
> **下一批仍是唯一物理编排、permit-join双向异步交接和Direct公平退让**，具体组合修改点见需求合同6.12。
> 当前Admission eager仍运行，idle/Direct未启用；不要将本批底层修正记成完整调度完成。以下为历史记录。

> **历史执行入口（2026-09-07，§10.73）：真实设备页需求消费与有界回收短门禁通过。**
> 最终APK在双XIAO+Android+本机Service/Broker通过Button 5→6、退出、后台、返回前台、账户Session更换与one-shot只读。
> 修正Android pause提前取消及初始化状态竞态；后台约30.1 s观测owner0、约39.5 s取样确认Hub wanted0，不能宣传立即释放。
> 仍使用§10.72的Hub/Broker，未重置/烧录两板或修改绑定；**物理idle/Direct退让未实施**。
> 下一批直接做唯一物理编排+Direct公平性，具体改动入口见需求合同6.11；不再重复实现owner/codec/管理预算。以下为历史记录。

> **历史执行入口（2026-09-07，§10.71）：0b3b2准入送达/首Hello恢复分离软件子项完成。**
> 初次exact Challenge确认authority，不制造ready；Hub首Hello改为实际安全会话后两轮23 s，删除Admission重发无限重开Hello预算的路径。
> 首批五Challenge全丢可由有效Admission重发触发一次fresh batch；Broker协商失败保留已确认authority/execution，非零恢复floor仍须新Hello确认。
> Broker Windows/Linux102、Host ASan47、跨仓18、20万fuzz及ESP32-S3编译通过；Flash增48 B、静态RAM不变。详见§10.71。
> **下一步配套物理调度、页面/只读sync需求与Direct退让。** App当前未自动申请需求，不能仅打开物理gate；未烧录/部署，现场版本仍为§10.69。
> 以下为历史记录。

> **历史执行入口（2026-09-07，§10.70）：0b3b2管理会话占用软件子项完成。**
> 修正nonce无独立TTL；proof/revoke/renewal按已鉴权命令钉住本地截止时间，Prepare绑定exact会话，重发不续期。
> 修复轮换后发起断开即丢弃清理权的问题；等待exact attempt结束，背压/异步断开和后续新会话分别处理。
> mux已消费独立所有权并精简重复dispatch；无新wire/API/任务/队列，静态RAM增32 B。验证与边界见§10.70。
> **下一步仍是0b3b2 Admission送达/物理首Hello预算拆分，再唯一物理编排和idle/Direct。** 本批未启用idle，未烧录/部署；现场仍为§10.69。
> 以下为历史记录。

> **历史执行入口（2026-09-07，§10.69）：L2c-0b3b1配套四端控制短门禁通过。**
> 已部署新Broker shard-1、新Hub应用、新Android调试包；真实App两个owner、部分/全部释放、ACK前取消、session结束均接通。
> 真机force-stop且所有zero Decision被丢弃：Broker约1.7 s观测owner=0，Hub自行到期，约30.2 s观测executionWanted=0。
> 普通BLE断链后1.199 s secure、2.605 s观测ready，App Action计数46→47；子设备Access摘要前后相同。
> **这是执行需求控制，不是物理idle/Direct交替通过。** 下一步仍为0b3b2：拆Admission送达/首Hello预算及管理事务占用，再配套物理调度。
> 当前权威字段为topologyVersion=4、credentialVersion=185、epoch494518246；历史“version4”不能理解为凭据版本。
> 本批没有修改生产调度器/公开API/页面UI，也未暂存/提交/推送。详细范围、工具中断及证据见§10.69。以下为历史记录。

> **历史入口（2026-09-07，§10.68）：L2c-0b3b1实际南向控制接线完成。**
> App owner变化→Broker独立有界执行控制器→真实route dispatch→Hub Bridge期限核心；不再是空changed回调或只测codec。
> 执行权限状态与volatile投影分开；closure/Hello/可达租约到期不抹掉执行控制，claims/authority退出清理。
> Host ASan47/47+20万fuzz、协议21+Broker97（Windows/Linux）、跨仓18/18、ESP32-S3 3.3.11编译通过；内部例程静态RAM增48 B。
> **物理连接调度、page自动需求、idle/Direct仍未激活；不是四端wake完成。** 未烧录/reset/部署，运行基线保持§10.64。
> 下一批0b3b2先拆Admission送达/首Hello恢复预算和management有界所有权，再配套替换自动重连与idle/Direct。
> 控制链路、容量及不可跳过的恢复门禁见[连接需求合同6.7～7节](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。以下为历史记录。

> **当前执行入口（2026-09-06，§10.64）：L2c-2b 新 Hub/Broker 配套真机可达租约门禁通过。**
> 修正真实 RouteClient 发送白名单漏接 lease proof；正常三周期、丢首 challenge、丢首 proof、全部通知/证明/Hello 丢失均已验证。
> loss-all 到期撤下投影，MQTT/Active 保留；旧证明不能复活，显式恢复后 App Action **45→46**，未重复执行。
> Host ASan46/46、现有fuzz20万、Linux Broker47/47、工具62/62（Windows/Linux）、ESP32编译与架构检查通过。
> **下一步 L2c-0b：实现有 owner 的 demand/wake/release，再同批验证 idle 与 Direct 退让；当前尚未启用。**
> 这不是发布长稳或 D4b 加密生产板验收。合同见 [连接需求与可达租约](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。以下为历史记录。

> **当前执行入口（2026-09-06，§10.63）：L2c-0a 边界收口 + L2c-2a loss-all 可达租约软件实现。**
> 先解决“全部失联通知丢失时无限在线”，再启用 idle；连接需求、可达证明、状态新鲜和业务 deadline 分离。
> 新 Hub/Broker route challenge/proof 不触及 child BBP、不建立连接、不写凭据；30 s 单调租约，重复/陈旧证明不续期。
> **尚未启用 idle、页面连接需求或 Direct 自动退让；配套部署与真机 loss-all 待测。** 不把软件通过写成发布完成。
> 合同见 [连接需求与可达租约](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。以下为历史记录。

> **当前执行入口（2026-09-06，§10.62）：L2b-4c2 App共享30 s ready等待、页面取消及真实恢复/耗尽门禁完成。**
> 同目标共享预算、独立取消、账号代次隔离；离线保留缓存但恢复后重读State，unknown控制不新建请求重放。
> 真机发现并修正主动State与推送State争用分页槽：单所有者+最高revision，复用最多两轮补读，无payload队列。
> Angular31套件224/224、定向106/106、工具54/54、本地构建/APK通过；真机等待2619/2521 ms，耗尽30006 ms超时，Action43→45。
> **下一批L2c-0先冻结idle/按需唤醒与loss-all可达租约，再实现释放及同绑定Direct↔Gateway；不要重复L2b等待基础实现。**
> 设备/Broker生产版本本批未改，未reset Access/烧录/重认领。L2b子项通过不等于发布长稳完成；详见§10.62。以下为历史记录。

> **当前执行入口（2026-09-06，§10.61）：L2b-4c1 exact link-loss通知及四端短门禁完成。**
> 新增独立GatewayRouteClosureV1，复用route topic/发送workspace/Hello计时空间；不增加业务队列、BBP kind或静态RAM。
> Hub断链先退役exact云端投影；丢首个receipt后的相同通知重发、旧通知/receipt回放、新ready及App Action 41→42→43通过。
> Host ASan46/46+20万现有fuzz、Broker36/36（Windows及Linux镜像）、协议19/19、工具53/53（Windows/Linux）通过。
> **下一步L2b-4c2：App共享且可取消的30 s独立ready等待，保持ensureReady快速失败与Action预算不变，再L2c idle/Direct交替。**
> 本批App源码/UI/APK未变，不声称等待UX、全部通知丢失的最终离线保障或发布长稳已完成。新Hub/Broker须配套部署，详见§10.61。
> 以下为历史记录。

> **当前执行入口（2026-09-06，§10.60）：**完成L2b-4b真实MCU入口1000 ms到期负测，以及L2b-4c的“两轮耗尽保持离线”子门禁。
> 单Action四次exact发送、两次入口过期1000/999 ms，实际计数40不变，ledger保持outcome_unknown；没有断开MQTT/BLE来恢复容量。
> 两轮协商各4次请求，45秒内无第三轮/无Action，App离线ready拒绝。故障仅在独立测试receiver/工具，不加生产队列/错误码。
> Host ASan46/46+20万fuzz、硬件工具52/52（Windows/Linux）通过。正常固件恢复与最终现场见§10.60.4。
> **下一步L2b-4c先实现authority/generation约束的失联通知，再App独立有界ready等待，最后idle/Direct↔Gateway。**
> 两轮失败的核心通信已验收，不等于完整App错误UX、通知盲区或整个L2b关闭。未resetAccess/重认领，未暂存/提交/推送。
> 以下为历史记录。

> **当前执行入口（2026-09-06，§10.59）：**L2b-4b2完成独立Broker冷恢复/首次ManifestRequest丢失、runtime State/Command ACK组合丢失门禁。
> 修复两个实际缺陷：Broker运行期State未复用有界exchange；Hub在MQTT重连但BLE未断时错误重置child sequence，可能误用旧ACK。
> 复用现有同步槽/ledger，删除错误序号重置，无新增设备队列、wire、API或RAM。Host ASan46/46+20万fuzz、Broker32/32、工具50/50通过。
> 最终Hub/Broker/APK组合三次冷恢复ready2513～2655 ms，控制/回读3194～3389 ms；首次ManifestRequest丢失3619/4333 ms。
> MQTT重连保留BLE时首条Action33→34；组合丢包34→36，冷恢复36→40，各Action仅执行一次。root/Active拓扑不变。
> **下一步先补真实1000 ms ingress占满到期负测，再L2b-4c提前失联通知/独立有界ready等待，最后idle/Direct↔Gateway。**
> 不把Broker边界丢包等同射频丢包或Hub入口到期，不关闭整个L2b/发布门禁；未重置Access、未暂存/提交/推送。以下为历史记录。

> **当前执行入口（2026-09-06，§10.58）：**L2b-4b1 已完成三项低复杂度修正：App State-only/单目标同步去重，
> Hub 在途请求冲突改为既有单槽背压，ESP32 Central 明确请求15 ms连接间隔。MTU23、20 B packet、可靠GATT和安全层均不变。
> 同连接A/B：50→15 ms时Action ACK1372→581 ms（各一个样本），不以此认定p95/长稳。App同步订阅重入竞态也已补回归修正。
> Host ASan46/46+20万fuzz、App8套件74/74、硬件工具49/49（Windows/Linux）、架构与ESP32编译通过。
> 最终APK三次App账户观测冷重连通过：在线2272～2472 ms，控制/回读结束2955～3186 ms，button21→24且离线ready均拒绝。
> **L2b-4b尚未全部关闭：**先补Broker新投影/丢包及真实runtime State/Command竞争门禁，再L2b-4c失联通知/有界ready等待，
> 最后idle/同绑定Direct↔Gateway；不把软件通过或热路径变快当作完整恢复/发布通过。部署与真机证据边界见§10.58。
> 未重置Access/重新认领，未暂存/提交/推送。以下入口为历史批次。

> **当前执行入口（2026-09-06，§10.57）：**L2b-4a 完成单一Host单调时钟冷重连基线，以及App“缓存≠可控制”的最小修正。
> 双XIAO+真实Android三次正常冷重连，新Broker ready为7039～7969 ms，控制/回读结束9065～9968 ms；
> 首个ManifestRequest丢失样本为8117/10170 ms。不是p95或发布长稳，不把这些耗时塞进3750 ms命令预算。
> 已知Presence=false时拒绝新Command，但保留页面缓存；已发送指令仍由ACK/ledger裁决，不新建离线Action队列。
> 新debug APK已保留数据安装；真实离线ensureReady拒绝、恢复后button15→16通过，完整冷恢复控制9945 ms。
> 下一批 **L2b-4b** 先查GATT分片往返/重试及State与Command仲裁，再冻结提前失联通知/恢复等待语义，最后才启用idle/交替。
> 本轮部署、复测和未完成边界见§10.57；以下入口均为历史批次。

> **当前执行入口（2026-09-06，§10.56）：**L2b-3c 已配套部署Admission V2 Hub/Broker并完成双XIAO+真实Android恢复子门禁。
> 首个admission/Hello/ManifestRequest丢失、同MQTT context退役、Action已执行但ACK丢失及一次bootstrap耗尽自动恢复通过。
> 旧ACK/旧Action跨代回放没有重复执行，unknown ledger保持unknown；绑定/root摘要不变，未resetAccess。
> 测试钩子全部在独立hardware工具内，已撤除并关闭Inspector；未增加生产接口、未改App源码/UI/APK。
> 下一批进入 **L2b-4完整ready测时/等待语义**，再做idle/Direct↔Gateway。正常冷BLE认证3621 ms、Hello→State ready4427 ms
> 来自不同观察点，不能冒充已精确测得全链路冷连接耗时；16.7秒故障恢复也不能塞进3750 ms业务命令预算。
> 以下为历史批次；本轮现场、证据边界及后续顺序以§10.56为准。

> **当前执行入口（2026-09-06，§10.55）：**L2b-3b admission/Hello恢复已完成软件实现及回归，尚未四端故障注入。
> Admission升级为V2（200 B上限，+9 B），显式携带已退役代际；Hub/Broker必须配套更新，不保留V1降级。
> Hello最多4次原样发送；同MQTT的context恢复不清凭据、不跨代重放Action，重复旧admission不打断新投影。
> 公共有界exchange调度已抽出，admission与分页协商不再各维护一套重试器；一次失败最多自动重新准入一次。
> Host ASan **46/46**、20万fuzz、架构/C++↔Server golden、Broker **30/30**（Node22/隔离Node20）、
> 协议 **18/18**通过；Redis专用本批SKIP。当前运行环境仍§10.53，未部署、烧录或重置绑定。
> 下一步按§10.55.4先配套部署与真实恢复注入/冷连接测时，再收口ready/命令预算，最后实现idle/交替。
> 以下入口均为历史批次。

> **当前执行入口（2026-09-06，§10.54）：**L2b-3a 完成 Broker 首次协商的有界请求重试与响应隔离，
> 不等于完整 bootstrap 恢复闭环。仅一个 ≤64 B 请求及16 B correlation；四次发送、全分页30秒总期限，
> 不缓存/重放Action、不改变设备API/wire。Broker回归 **25/25**（Node22.22.3 / 隔离Node20.20.2）、
> 既有Broker套件及协议 **18/18**通过；Redis专用测试本批SKIP，未冒充通过。
> **本批没有部署、烧录或操作真机，没有重置绑定/凭据。** 当前运行版本仍为§10.53，不把软件故障注入当真机验收。
> 下一步按§10.54.4处理首个Hello丢失及同MQTT context退役后的显式恢复，然后协调冷连接预算，最后启用idle。
> 以下入口均为历史批次。

> **当前执行入口（2026-09-06，§10.53）：**L2b-2 冷Hello/路由代际已完成软件及一次真实断连回归。
> correlation仍16 B，前8 B为单调投影代际；synthetic Hello声明bit14，Hub/Broker配套更新，无旧语义降级。
> 新代际退役旧命令为unknown，旧Action不跨代重发；重新Manifest/State后才reachable，旧回调/State/ACK隔离。
> Host ASan **46/46**、20万fuzz、架构/工具 **31/31**，Broker **14/14**（Node22/Node20）、实际Redis、
> 协议 **18/18**通过；Hub **+248 B Flash/+8 B globals**，仅应用更新，child固件/APK不变。
> 保留同一 `45b92507779a943cc939cdac4a25323f` Active/version4、epoch494518246；没有resetAccess或重认领。
> 热路由button **4→5**；正常BLE断连后 **3516 ms**再Secure，同MQTT内child protocol session换代，控制 **5→6**。
> App无残留、child access摘要不变。**idle/按需唤醒、协商丢包和预算、Direct↔Gateway交替仍未完成。**
> 下一步按 §10.53.4 先补bootstrap恢复，不重置当前绑定；本次不是丢ACK真机注入、长稳或SDK assertion修复证明。
> 以下入口均为历史批次。

> **当前执行入口（2026-09-06，§10.52）：**L2b-1 Broker 命令发送槽生命周期已收口。通用 virtual channel
> 显式 beginCommand/send/close；ACK、拒绝、超时、取消、发送失败统一释放自己的槽，旧异步回调不能清新命令。
> 路由先校验 correlation + sequence，最多保留一条终态关联见证，不缓存其 frame；unknown Action 不自动重发。
> 新增回归 **9/9**（Node22/部署 Node20）、既有 Broker/Redis 限流、协议 **18/18** 通过，已更新本机 shard1。
> 按本轮明确授权仅 reset COM12 child access 一次，Hub 凭据/网络未清；新 epoch494518246，原生 Direct
> button **2→3**。同身份 attach 到 `45b92507779a943cc939cdac4a25323f` Active/version4，真实 account MQTT
> button **3→4**，App 无恢复残留。双板固件与 APK 不变；本轮未复现 assertion，不等于已修复。
> **未启用 route idle，冷 Hello/状态代际、跨 BLE session 的未确认 Action、Direct↔Gateway 交替仍未通过。**
> 下一步从 §10.52.4 的冷恢复失败测试开始，不再 reset 当前已认领子设备。以下入口均为历史批次。

> **当前执行入口（2026-09-06，§10.51）：**本批先实现 L2a 有界 ingress 交接，不启用普通 route idle。
> 唯一 MQTT 槽仅在下游接受/拒绝后清除，WouldBlock 保留 exact bytes，最长 1 秒；MQTT 换代、解绑、
> reset 清理旧帧。路由逐次复核 admission generation/durable revision，成功接受才消费 child sequence。
> 按本轮明确授权只 reset 一次 COM12 child access，Hub 未清身份/网络/access；同身份原生 Direct
> 已恢复 active、button **0→1**。首次 Method2 超时，随后串口捕获 assertion/reboot，不能算稳定性通过。
> Host ASan **46/46**、20 万确定性 fuzz、架构/工具 **31/31** 通过；最终 Hub **+324 B Flash/+8 B globals**。
> 新 `9bf02e5a0a3ff936963b937782e408c2` Active/version4，真实 account MQTT button **1→2**；App 无恢复残留。
> 完整部署和当前恢复点见 §10.51；本批不关闭普通 route 占用、冷恢复、Action 结果未知或整体发布门禁。
> 以下同日入口均为历史记录。

> **当前执行入口（2026-09-06，§10.50）：**仅按本轮新授权 reset 一次 COM12 child access；同身份手机
> 原生认领/控制 **2→3**，再 attach 至新 `4f84ebee1d286b51020dd659b2947521` Active/version4、epoch3032216448。
> L1 proof 所有权子项已 RED→GREEN；Hub 应用已更新（**Flash +196 B / globals 不变**），未清 Hub access。
> 真机 proof 结束后 child 恢复 adv1/acl0、连接/断开 **11/11**；Active route 随后重连，真实 account MQTT
> 控制 **3→4**。**普通 route 仍长期占用，手机 Direct 15,038 ms 扫描超时，交叉门禁未通过。**
> Host ASan **45/45**、20 万确定性 fuzz、架构/工具 **31/31** 通过；App/Service/Broker 源码和部署不变。
> App 无恢复残留，不要重复 reset。下一批从 §10.50.2 的 route 需求/缓冲/冷恢复合同开始，仍不关闭
> L1 全部、L2/L3、间歇安装失败或 N3/N4/N5/发布门禁。以下同日入口均为历史记录。

> **当前执行入口（2026-09-06，§10.49）：**继续 Gateway 主线，但优先补“授权与物理连接占用解耦”，
> 不先做全平台 WiFi 回归或扩大长稳。按本轮授权仅 reset 一次 COM12 child access，COM11 Hub 未重置。
> 同一 V2 BLE 固件已完成手机原生 Direct 认领/控制 **0→1**，再给已有设备 attach 至新 Active/version4，
> 真实 App account MQTT→Hub Gateway credential→child 控制 **1→2**；attach 不 reset、不再 enrollment。
> **绑定后的手机 Direct 扫描超时：Hub 持续占用唯一 BLE ACL，尚未实现默认空闲释放。交叉门禁未通过。**
> test-only GAP listener 生命周期已修；App 首个错误被清理断连覆盖的问题先红后绿修复，相关 **47 tests** 通过。
> 首次安装仍曾失败，正常恢复成功不等于其底层原因已解决。当前 topology=`8cb2bcdda4da10afaff0ed1e36e8bf2d`，
> epoch2145337234，App 无残留；不要重复 reset。结果、部署和后续分批准入见 **§10.49.3～§10.49.5**。
> 以下“最新”条目均为历史批次，不替代本条现场状态。

> **最新执行入口（2026-09-06，§10.48）：**ADB 已恢复；按本轮明确授权只 reset 一次 COM12 child access，
> COM11 Hub 不变。重新认领→旧绑定退役等待→安装→独立 Gateway Method 2/proof→新 Active 已完成，
> 当前 operation=`5ee0a95343dee821ad8ff3ddaa0481ee`，epoch3279301365/version4，无恢复残留。
> 带阶段观测的独立正常连接 **10/10**、button ACK/State **3→4**；本轮未复现 §10.47 的两次失败，
> **不等于已定位/修复，也不关闭 N3/N4/N5**。既有重连工具补认证前帧头/错误分类，自测 **31/31**、架构通过。
> 双板固件/APK/服务镜像均未变。当前 child 已重新绑定，不再为“继续”重复 reset；下一批优先按 §10.48.3
> 收口失败观测与 test-only listener 生命周期。以下批次为历史记录，不替代当前现场状态。

> **最新执行入口（2026-09-06，§10.47）：**已正常恢复 §10.46 原绑定至 Active，再按授权仅 reset COM12
> child access。Service/App 的精确 retirement-pending 合同已完成软件及真机等待验证，未新增设备 wire/状态。
> App 58/58、Service 隔离 PostgreSQL/HTTP/access、构建通过；去掉首次绑定的重复 create。
> 真机等待时 force-stop，重启保留同一 checkpoint；后续正常恢复达到新 topology Active/version4。
> 随后独立正常连接/同步/关闭 **3/3**，真实 button ACK/State **2→3**，结束无 link/session/checkpoint 残留。
> **恢复中仍有间歇连接失败，不能算整条链路一次通过**。先收口普通连接/Method2 失败，再 N3/N4/N5。
> 本批只更新 Service/APK，双板固件不变；当前 child 已绑定，不应再 reset。完整证据与边界见 §10.47。
> 以下带日期的批次为历史记录；历史“不要 reset”等现场指令不代替本条当前状态。

> **最新执行入口（2026-09-06，§10.46）：**再次按授权 reset 现有 child，真机定位到管理调度器把“已发 Ready”
> 当成任务完成，Supersede 抢占并 reset 仍工作的 permit。现以独立 `busy()` 合同保护模块任务/租约，
> Host 旧代码 RED、新 ASan **45/45**，3.3.11 Hub 已更新（Flash +212 B / globals 不变）。同 APK/child
> 实测 commit 后同步 **5.522 s 成功**，原约 50 s 断连已在本场景修复；未改 App API 或延长超时。
> 后续 attach-create 暴露旧 topology 尚在退役的真实冲突；原 checkpoint 已继续到 **PendingAccessDelivery**，
> 手机随后离开 ADB，安装/proof 尚未完成。**不要再 reset/删 checkpoint**；接回手机恢复同一操作，随后
> 收口“退役等待”与真实绑定冲突的 Service/App 合同，再回 N3 envelope/N4/N5；详见 §10.46。

> **最新执行入口（2026-09-06，§10.45，优先于历史待办）：**用户明确授权重置现有测试 child；已走
> resetAccess→同 instance 重认领。真机暴露并修复 provisioning endpoint 关闭 channel 后残留 sessionId，
> 不再因已安装凭据又重启 Provisioning 而进入 StateConflict/Fault。Host ASan **45/45**、架构、3.3.11
> 编译通过；重放工具 Windows/Linux **28/28**。最终真实 requestId exact replay/changed-request 拒绝通过，
> 三个请求合计仅一次 NVS set/commit；正常分阶段恢复至 Active，正常固件已恢复、持久化摘要未变。
> **尚有 commit 后同步断连需要收口**，不能把恢复成功算首次无故障；下一批先定位该交接，再做真实 MQTT
> envelope 注入和 N4/N5。N2 双 Hub、D4b、发布长稳仍独立阻断，完整证据与边界见 §10.45。

> **最新执行入口（2026-09-06，§10.44）：**N3 enrollment requestId 重放注入工具已补齐并复用 CDP helper，
> 自测 Windows/Linux **20/20**、设备 Host ASan **45/45**；不新增产品接口/重试，不改 App/UI/SDK/wire。
> 现场只有 Active child，无待恢复 enrollment；本批保留绑定，未执行 reset/re-enrollment 或该项真机负测。
> 现有四端正常连接/同步/清理 **1/1**，topology/version 与 child access hash 未变。
> 下一步按 §10.44.3 选择未绑定测试子设备，或先确认正常重置对现有绑定的影响；N3 未整体关闭。

> **最新执行入口（2026-09-06，§10.43）：**N3 的 envelope 校验已补真实 ESP32 mbedTLS + 独立 NVS 门禁：
> Service 固定测试密文与正式设备解封器互通；空/已保存状态下篡改、错上下文/Key、失信/过期均拒绝零
> replace，合法重封保持 ACK，软件重启 exact ACK 零重写。只写隔离测试 key，原 Hub 凭据未变，已清理
> 并恢复 §10.41 Hub 固件；真实 App 认证/同步恢复 **2/2**。Host ASan **45/45**，产品源码与 wire 未改。
> 本项不是 MQTT/四端传输篡改、生产加密存储或物理断电门禁；enrollment requestId device replay、
> 四端 envelope delivery 注入及 N4/N5 仍待执行，具体边界/顺序见 §10.43。

> **最新执行入口（2026-09-06，§10.42，优先于以下历史批次）：**Broker 的有界接受/下发边界已修，
> 无新增队列、wire 或 App 重试；软件 RED/GREEN 与三端回归通过。仅 shard-1 更新、同 APK/双板不 reset，
> 实际 Up 等 ACK 时关闭 MQTT socket 后，同 operation 认证/同步完成，再 fresh 成功，**2/2** 无残留。
> 自然 SessionCredential refresh 在途门禁也独立通过，随后正常 **10/10**（128,802 ms）全部 Closed、
> App 无残留，NVS/topology 未变。下一批回 N3 剩余 envelope/enrollment replay，再 N4/N5。
> 完整接受/送达合同及证据见 §10.42；不把 socket 恢复算作 Broker restart 耐久性或发布长稳。

> **最新执行入口（2026-09-06，§10.41）：**已修正 MQTT 间隙清 relay 序号而本地窗口仍存活的问题，保留
> 同窗口/同授权的有界进度，过期/授权变化/ACL 失败结束窗口；Host ASan 45/45、架构及 3.3.11 编译通过，
> 同 probe 生产修正净增 **272 B Flash / 8 B globals**。真机还定位并修复 App 迟到 ManifestAccept ACK
> 被错当 State 响应的问题，App 30 tests 与构建通过。受控 MQTT 中断暴露 Broker 上行 ACK 依赖立即下发，
> App 收到 HubOffline/503 后正常清理但不能续传；该门禁仍失败。下一批先收口 Broker pending/ACK 接受合同，
> 再做在途刷新/重连与独立 10 轮，不把软件通过或清理成功算作四端连续性/长稳完成。完整证据见 §10.41。

> **最新执行入口（2026-09-06，§10.40，优先于下方历史批次）：**已复现并修复 Service 仍见 Ready、Broker
> 已 Rejected 时的 Close 竞态；旧实现明确 Conflict，新 Broker 保留 exact 终态并幂等返回，不新增 App 重试。
> Broker/Service（含真实 PostgreSQL）与 App 25 tests 通过；同 APK/双板固件、不 reset，真实拒绝清理→fresh
> 认证恢复 **2/2**，App 0 残留、NVS/topology 不变。仅本机 shard-1 部署新镜像。
> 新只读连接事件确认自然 MQTT 凭据换代有约 **570 ms** management 可达性间隙；另发现 relay 在云离线时
> 清序号但本地窗口可仍存活的代码边界。下一批优先确定性复现并修正该生命周期不一致，再回 N3/N4/N5，
> 不先加 App 重试或运行 30/100/发布长稳；本批未验证 active permit 跨换代，不宣称 Hub offline 全部解决。

> **最新执行入口（2026-09-06，§10.39）：**ESP32 child 的 indication 错误误升级为 Host fatal、native handle
> 跨任务交接丢失 disconnect 两项已先红后绿修复；原生适配器回归加入全量 ASan **45/45**。内部 probe 提供
> 被动 GAP/广播只读观测，正式 API/wire/Hub/App/Broker 不变。新 child 独立正常 **10/10（137,587 ms）**；
> 另一次 selected-acl 正确 Rejected 但清理请求失败、留 checkpoint，正常 recovery 后清除；修改前还有 Hub
> 离线失败，均保留为独立证据。先定位清理/Hub 可达性与原生停播剩余边界，再回 N3/N4/N5，不扩大长稳结论。

> 状态：2026-08-30，G2-0A、G2-0B1、G2-0B2 已完成；G2-0C child 三槽 access schema v3
> 的源码、v1/v2 兼容迁移、Host/三平台编译与资源门禁已完成。Wio、ESP32 与 UNO R4 真实断电/
> 重新上电门禁均已通过，G2-0C 正式关闭。G2-0D 的 D0～D3 已完成：设备端 wrap KDF/AES-GCM 解封、
> 单 child 352 B 受保护原子 access 记录、重启/重封/精确重投幂等 ACK，以及复用现有 MQTT 连接的 management
> side channel 已通过 Host/Broker 门禁。D4a 的 ESP32 product binding、MQTTS/SNTP admission、受保护 NVS
> fail-closed 与 XIAO 真机探针已完成；D4b 仍需一块按生产流程启用 Flash Encryption 的专用 ESP32，关闭
> sealed delivery/物理断电/Broker redelivery/same ACK 门禁。
> G2-0B2 已冻结专用 MQTTS management topic、58 B durable ACK、
> Broker ciphertext delivery ledger、QoS 1 exact-envelope 重发/过期，以及 Cloud Gateway 的幂等
> topology CAS 对账。
> 本文是 `DEVICE_V2_BLE_DIRECT_GATEWAY_ROUTING_ARCHITECTURE.md` 的 G2 执行分解。
> Wio BLE 的反复断连、认证超时与单连接占用 DoS 已于 2026-08-29 真机关闭；当前优先级只剩真实 owner
> DirectSecureRecord 的发布前长稳。该长稳决定 Wio BLE 能否由 preview 晋级 production，不是纯协议合同、
> Service 软件或 ESP32 Edge Hub 骨架的架构前置条件；因此不再阻塞 G2-0A～G2-0C。
> 2026-09-01，G2A-2g 的设备/Hub/Cloud/App 四端业务数据面已通过 Broker 重启、连续两轮 60 s
> SessionCredential 刷新、Presence、Manifest/State、真实 Android account MQTT Action/ACK/Patch，以及 child
> 复位后的状态代际切换门禁。该结果关闭最小单 child route bridge，不放宽 D4b 生产加密存储门禁。
> G2A-3a 服务端撤销闸门也已完成：owner `detach` 将 Active/RollbackRequired 原子推进到 Revoking，Active route
> 查询立即失效，并主动要求 Broker 退役虚拟 child session、Presence 和在途 Command。Revoking 不是 Detached；
> G2A-3b 的软件主路径也已完成：Prepare/Apply/Finalize exact wire、Broker durable ledger、child signed Revoke、
> Hub secret-free tombstone、Service receipt 校验与 Detached CAS 已通过 Host/Node/PostgreSQL 门禁。真实 Android、
> 双 XIAO、Broker 与 Service 的 topology v14 已完成 Prepare=`nonce_ready`、Apply=`revoked`、Finalize=`finalized`，
> PostgreSQL 收敛为 Detached；Hub 复位后旧 access 不复活。Service 后台 reconciler 已接入生产 runtime，并在没有
> App 调用时依据持久 Finalize result 自动恢复缺失的 finalization 标记。随后全新 topology
> `rAnm8BWpH2DJOKrzgCR_EQ` 完成 version 2 重接；App 只发起 detach 后立即强制停止，worker 独立完成三阶段撤销，
> Broker ledger、Detached/finalized 与 Hub 复位清除 tombstone 全部通过。“child 已删除凭据但 Hub 尚未写入
> tombstone 即掉电”的 nearby Admin recovery 软件闭环以及双板精确掉电/复位注入均已完成。
> 2026-09-04，ESP32 Arduino Core 3.3.11 / ESP-IDF 5.5.5 已完成正式 product-level 资源与短稳门禁：
> `ipc0` 在 WiFi+NimBLE 运行态仍有 1108 B 水位，完整 Edge Hub 为 1,410,740 B Flash / 64,460 B globals；
> 无串口监视器运行超过三个 MQTT keepalive 周期后，App→Service→Broker→Hub→BLE child 的
> 10→30→100 次 Action 均全部 ACK，child revision 与执行计数分别精确增加 10、30、100。该结果关闭
> 3.3.11 SDK/任务栈准入和分阶段短稳，不替代 30～60 min 工程长稳、24 h 发布长稳、G2B-4 负测或 D4b
> 生产加密板门禁。
> 2026-09-05，补齐 child `resetAccess()`/重新 enrollment 的 access-root supersession：新的 enrollment
> receipt 与 accessEpoch 在同一 PostgreSQL 事务中把该 physical child 的旧 live topology 推进到 Revoking；
> Service 立即撤销 Broker route，Broker 原子过期旧 access/proof/renewal/revocation pending ledger，再向 Hub
> 下发 terminal `Supersede`。Hub 只在 operation、child instance、旧 epoch/controller/version/delivery revision
> 精确匹配时清除旧 Gateway credential，并返回 `Retired`；不再尝试连接已清空旧 Admin/Gateway slot 的 child。
> 跨语言协议、Hub ASan、Broker 全套、Service 内存与真实 PostgreSQL、现有数据库 migration 26/两 shard SQLite
> 升级均已通过。真实 child reset→re-enrollment→Hub retire→Detached 四端注入也已完成：Broker phase 4 ledger
> 为单次 `Retired`，Service topology 为 `Detached`，Hub 硬复位后旧 access 未复活，App 的新 Direct credential
> 仍可连接同一 logical device。该结果关闭 N5 的 access-root supersession 子门禁，不替代 N2～N4 与 N5 其他
> durable boundary/组合重启负测。
> 2026-09-05，N3 的 permit-join 选择负测已在恢复后的官方 3.3.11 Hub 上连续完成 4 轮：真实候选单 bit
> 篡改返回 NotFound、exact Select 重放返回相同结果、同操作变更候选返回 409，Service/Broker 最终均 Closed。
> 见 §10.30；enrollment/attach 的篡改、过期与重放以及 N4/N5 其余边界仍未关闭。
> 2026-09-05，继续完成 N3 enrollment/attach 的软件边界修订与回归：按阶段区分“新授权过期”与
> “已完成安装的恢复提交”，补齐 App Gateway receipt 的 secretDigest 绑定；Host ASan、Service 真实 PostgreSQL
> 与 App 专项见 §10.31。该批未烧录/修改现场 access，不替代剩余四端故障注入。

## 1. 目标与非目标

首版目标：

- 一个 ESP32-class Edge Hub 通过 WiFi/Ethernet + 强制 MQTTS 上线；
- 一个已认领 BLE child 可由 owner App 绑定到一个 active Edge Hub；
- child 保留原 logicalDeviceId、页面、Manifest、State、Command、ACK 与 Direct App 控制；
- Edge Hub 使用独立 Gateway credential，通过 Method 2 + DirectSecureRecord 连接 child；
- Service/Broker 只允许 active topology 的 exact Edge Hub 代理 exact child；
- 目标为默认按需 GATT 连接，避免长期占用 child 唯一 central 槽；**当前实现尚未完整满足**：
  §10.49 真机确认 Active route 会持续维持连接，按 §10.49.5 补空闲释放/冷恢复后才算完成；
- App 先认领再 attach（G2A）完成后，再实现 gateway-assisted enrollment（G2B）。

本轮非目标：

- BLE Mesh、SIG/Vendor Model、Relay/Friend/Proxy；
- 广播 telemetry、roaming beacon、多网关同时在线；
- persistent child connection、批量入网、无人值守抢占；
- 任意 device-to-device/service route；G2 只冻结 child proxy 必需的最小路由外层；
- 在普通 BLE/WiFi Arduino QuickStart 中强制 TLS。TLS 强制只属于 Edge Hub product profile。

## 2. 术语

| 术语 | 含义 |
|---|---|
| Edge Hub | 用户现场的实体 BLE 网关；首版为 ESP32-class WiFi+BLE 产品 |
| Cloud Gateway | `aily-blinker-services/services/gateway` 公网控制面/MQTT 入口 |
| BLE child | 现有 Blinker BLE Peripheral 设备 |
| Gateway credential | Edge Hub 对某一 child 执行 Method 2 的 32 B HMAC secret |
| topology | Service 权威维护的 `child logicalDeviceId -> active Edge Hub` 关系 |

源文件、表、API、日志和指标禁止只写含糊的 `gatewayId`；必须明确是 `edgeHubLogicalDeviceId` 还是 Cloud Gateway
服务实例/shard。

## 3. 决策：Service 托管、MQTTS 下发、应用层密封

### 3.1 两个候选

| 比较项 | Edge Hub 本地生成 | Service/KMS 生成并下发 |
|---|---|---|
| Service 是否知道 raw Gateway secret | 否 | 是，仅 KMS 解密边界可见 |
| App 前置连接 | 必须同时安全连接 Edge Hub 与 child | 只需 HTTPS 取 pending package，并 Direct 连接 child |
| App kill/半事务恢复 | Edge Hub 必须保存并重新暴露 pending secret | Service 以 operationId 恢复同一 KMS ciphertext |
| Edge Hub 全擦恢复 | 若本地 secret 丢失，通常需重新靠近 child 轮换 | Service 可向恢复后的 exact Hub 重新密封下发同一 active secret |
| 换 Hub/轮换 | 需要本地协调生成、导出和双端提交 | Service 作为 topology 权威统一生成版本和撤销 |
| 多 Hub 扩展 | 仍需新增云端分发/协调 | 已具备托管与分发基础 |
| 首版三端代码量 | Edge Hub/App 本地控制面更复杂 | Service/KMS 和专用管理消息更直接 |

冻结选择：**首版使用 ServiceManagedSealedDelivery**。Gateway credential 是平台基础设施凭据，不继承
Admin/Shared 的 Service 零知情要求；Admin/Shared raw secret 仍不得上传服务器。Edge Hub 本地生成保留为后续
Enhanced/Local profile，不与 G2 首版并行实现。

### 3.2 为什么不能只靠 MQTTS 发送明文 body

MQTTS 只保护 Edge Hub 到 TLS 终止点。Cloud Gateway、内部代理、Broker、command ledger、trace 和 crash dump
可能在 TLS 终止后看到 payload。因此冻结为：

1. Edge Hub 的 HTTPS 登录和 MQTT 数据/管理面均强制 TLS，禁止 Plain fallback；
2. Service 生成 raw secret，以 KMS/HSM envelope encryption 保存；
3. MQTT 中发送 `GatewayAccessEnvelopeV2`，48 B access material 是 envelope ciphertext 的明文，不是 MQTT
   payload 明文；
4. exact Edge Hub 使用 DeviceKey 派生 wrapping key 解密；Broker/ledger 只看到密文；
5. App 经账户 HTTPS pending-attach API 短暂获得 raw secret，用完立即清内存，不落普通 storage。

因此业务上仍是“secret 通过 MQTT 下发到网关”，但安全边界不是“TLS 已经足够，所以可裸发”。

## 4. 凭据与设备持久化合同

### 4.1 设备侧不新增 Gateway domain

当前 `ControllerCredentialDomain` 保留：

- `Local`
- `PlatformAccess`

Admin、Shared、Gateway 均使用 `PlatformAccess`。设备只执行权限，不保存账号角色：

| credential | permissions |
|---|---:|
| Admin | Observe + Control + ProvisionNetwork + ManageControllers |
| Shared | Observe + Control |
| Gateway | Observe + Control |

Service 根据 controllerId 元数据识别 Shared/Gateway；ControllerGrant、Method 2 和 DirectSecureRecord 不增加 role
分支。Gateway 无 `ManageControllers`，不能自行安装、轮换或删除其他 controller；它只可在已有 `Control` 安全
session 内中继 fresh-nonce 绑定的服务器签名 Grant，用于自己的 attach/detach/rotate 事务。

### 4.2 access store schema v3

当前真实实现已升级为三个 64 B slot、260 B schema v3；它同时容纳：

1. Admin；
2. Shared group；
3. Gateway。

按当前布局 schema v3 为：

```text
64 B header/device key/presence
+ 3 * 64 B controller slots
+ 4 B CRC
= 260 B
```

exact byte layout 冻结为：

| offset | size | 字段 |
|---:|---:|---|
| 0 | 4 | magic `BAS2` |
| 4 | 1 | schema version `3` |
| 5 | 1 | slot capacity `3` |
| 6 | 1 | active slot count `0..3` |
| 7 | 1 | flags：bit0 DeviceKey、bit1 PresenceKey，其余必须为 0 |
| 8 | 4 | accessEpoch，big-endian |
| 12 | 32 | DeviceKey 或全 0 |
| 44 | 4 | PresenceKey version，big-endian |
| 48 | 16 | PresenceKey 或全 0 |
| 64 | 192 | 三个 canonical 64 B controller slot |
| 256 | 4 | 前 256 B 的 CRC32，big-endian |

G2-0A 只冻结了该布局；G2-0C 已完成 `DeviceAccessStore` 实际扩容、v1/v2 解码和三平台 storage capacity。
读取旧记录不会在启动路径制造 flash 写入；第一次成功的状态 mutation 才以一次原子 replace 写出 v3。Host
故障注入、32 位 ABI 和三平台 clean compile 已通过。Wio、ESP32 与 UNO R4 均已完成“旧记录启动—
mutation—断电—重新上电—v3 精确恢复”，G2-0C 硬件门禁正式完成。

Wio 门禁由 `tests/arduino/WioStoragePersistenceGate` 执行：空 access 区先写入可识别的 v2 fixture，软件
复位后安装 Shared/Gateway 并形成三槽 v3，随后停在 `MIGRATED_V3_POWER_OFF_NOW` 等待真实断电；重新
上电只有 exact Key/epoch/Admin/Shared/Gateway 全部恢复才输出 `V3_POWER_CYCLE_OK`。发现任意非 fixture
access record 时只输出 `ACCESS_RECORD_CONFLICT_NOT_MODIFIED`，禁止为了测试静默覆盖用户凭据。
本次 COM6 真机在明确授权后以仅当次编译开关清除 access blob，已完成 v2 fixture 写入、软件复位与
v3 原子迁移；真实物理断电/重新上电后返回 `V3_POWER_CYCLE_OK`。验证后已只清除 exact 测试 fixture，
并在再次启动时返回 `ACCESS_RECORD_EMPTY`。该结果关闭 Wio 真机门禁，不代替 ESP32/UNO R4 验证。

ESP32 门禁由 `tests/arduino/Esp32StoragePersistenceGate` 执行，并与 Wio 复用同一个 test-only
`DeviceAccessMigrationFixture` Arduino library。XIAO ESP32-S3 COM12 的 runner 使用隔离键
`bl_g2c_gate/access`，不读写生产 `bl_v2_access/root`；真实断电后 exact 三槽恢复并输出
`V3_POWER_CYCLE_OK`。只有完全匹配 fixture 时 cleanup 才删除该测试键，重启后已确认
`ACCESS_RECORD_EMPTY`。

UNO R4 门禁由 `tests/arduino/UnoR4StoragePersistenceGate` 执行，同样复用 canonical fixture。
UNO R4 WiFi COM9 的 runner 使用隔离 Preferences namespace `bl_g2c_gate`，不读写正常
`bl_v2` namespace；真实断电后输出 `V3_POWER_CYCLE_OK`。cleanup 只对 exact fixture 写入 tombstone，
重启后持续输出 `ACCESS_RECORD_EMPTY`。

实施要求：

- 官方 BLE/WiFi+BLE profile 默认使用三槽，用户以后添加网关不应要求重刷另一套 child 固件；
- decoder 继续读取 176 B v1 与 196 B v2，并在下一次成功 mutation 时原子迁移到 260 B v3；
- v3 允许 exactly one Admin、最多两个 Observe/Control controller；Service 负责将二者分类为 Shared/Gateway；
- 编码仍按 controllerId canonical 排序；重复 id、跨 epoch、权限越界、第四槽全部拒绝；
- 更新 ESP32、UNO R4、Wio storage capacity 和断电恢复 fixture；
- 重跑 32-bit ABI、SizeProbe、UNO R4 SRAM/stack、Wio QSPI 与真实断电门禁。

Small profile 若未来保留两槽，必须明确声明“Shared 与 Gateway 不可同时存在”，不能作为官方默认体验。

### 4.3 Gateway credential 生命周期

- 每个 child 只有一个 active Gateway slot；controllerId 可作为该 child 的稳定 Gateway slot id；
- 每个新 topology 都向不存在的 Gateway slot 执行 `Install`；历史 version 继续单调递增，因此 detach 后重接可以是
  `Install version 2+`。`Rotate` 只用于同一 live topology 内确知槽仍存在的原地 secret refresh，不能由
  `credentialVersion > 1` 推断；
- raw secret 32 B，由 Service CSPRNG 生成；普通数据库只保存 digest、metadata 和 KMS ciphertext；
- 删除 topology 时 Cloud route 立即拒绝；Edge Hub 立即擦除；child 本地 revoke 在下一次 Admin Direct 会话执行，
  这是明确的 eventual 边界；
- child access reset 轮换 accessEpoch，旧 Gateway credential 和 topology proof 全部失效。

## 5. GatewayAccessEnvelopeV2

### 5.1 wrapping key

首版使用 exact Edge Hub DeviceKey 做域隔离派生，不直接拿 DeviceKey 当 AES key：

```text
saltTranscript =
  "BLINKER-EDGE-GATEWAY-WRAP/2\0" ||
  u16be(len(environmentId)) || environmentId ASCII ||
  u16be(len(tenantId)) || tenantId ASCII ||
  u16be(len(edgeHubLogicalDeviceId)) || edgeHubLogicalDeviceId ASCII ||
  u32be(deviceKeyVersion)

salt = SHA-256(saltTranscript)

wrapKey = HKDF-SHA256(
  ikm = EdgeHubDeviceKey,
  salt = salt,
  info = "BLINKER-GATEWAY-ACCESS-ENVELOPE/2",
  length = 32)
```

禁止把三个可变长标识符直接无分隔拼接；那会产生字段边界歧义。上面的长度前缀 transcript 是 exact
合同，G2-0B1 已用固定派生向量冻结。

DeviceKey refresh 后，未完成的旧 envelope 失效；Service 用新 DeviceKey version 重新密封同一 pending/active
Gateway secret。active child credential 不因 Edge Hub DeviceKey refresh 自动轮换。

### 5.2 envelope 字段

exact wire 已由 device/service golden 冻结为 canonical CBOR fixed array；数组长度固定为 17，位置不可重排：

```text
0  version = 2
1  operationId bstr[16]
2  environmentId ASCII tstr[1..32]
3  tenantId ASCII tstr[1..128]
4  edgeHubLogicalDeviceId ASCII tstr[1..128]
5  deviceKeyVersion uint32, non-zero
6  childLogicalDeviceId ASCII tstr[1..128]
7  childDeviceInstanceId bstr[16]
8  accessEpoch uint32, non-zero
9  controllerId bstr[16]
10 credentialVersion uint32, non-zero
11 permissions = Observe|Control = 3
12 presenceKeyVersion uint32, non-zero
13 expiresAtUnixSeconds uint64, non-zero；这是 Gateway credential expiry，不是 MQTT 投递 TTL
14 nonce bstr[12]
15 ciphertext bstr[48]
16 tag bstr[16]
```

标识符只允许可打印非空 ASCII，并拒绝 `\\ / # +`，避免 Unicode normalization 与 MQTT/topic 分隔歧义。
完整 envelope 最大 587 B。AES-256-GCM 的 AAD 是数组长度 15 的位置 0～14 canonical encoding，最大
520 B；AAD 生成不要求 ciphertext/tag 已存在。ciphertext plaintext 固定为
`gatewayControllerSecret[32] || presenceKey[16]`。要求：

- nonce 由 Service CSPRNG 产生；不同明文/operation 不得复用；
- QoS 1 重发同一 operation 时重发 exact envelope，不重新加密生成第二个 nonce/ciphertext；
- `retain=false`，不得进入 retained message；
- management message 有独立固定上限，不占普通 child BBP/2 frame 上限；
- Edge Hub 校验 exact identity、DeviceKey version、credential expiry、permissions、AEAD tag 后才写 storage；
- durable ACK 固定绑定 `operationId + accessMaterialDigest + storageRevision`；重复 envelope 返回同一逻辑 ACK；
- access material/envelope plaintext/DeviceKey/wrapKey 禁止进入日志、metrics、普通 DB、crash report 和 packet dump。

未来量产 Edge Hub 可改用设备公钥 sealed box/硬件安全单元；不能在同一 wire version 下悄悄更换 KDF/AAD。

### 5.3 management topic 与 durable ACK

每个已认证 Edge Hub session 只获得两条 session-local topic：

```text
/edge-hub/{clientId}/management/down  Broker -> Edge Hub
/edge-hub/{clientId}/management/up    Edge Hub -> Broker
```

普通 Device V2 session 没有这两条 ACL。management 下行固定 `QoS=1`、`retain=false`，Broker ledger 只持久化
exact `GatewayAccessEnvelopeV2` ciphertext、digest 和投递元数据；1/2.5/5/10/30 s 封顶退避始终重发相同
envelope，Broker 重启后从同一 ledger 恢复。`deliveryExpiresAt` 是通常不超过 15 分钟的投递截止时间，必须早于
或等于 envelope 中长期 `credentialExpiresAt`，两者禁止复用为同一业务期限。投递过期后停止投递并拒绝 late
ACK，但不能据此把已经 durable 安装的 Gateway credential 变成 15 分钟凭据。

management 上行 ACK 允许 `QoS=0|1` 且强制 `retain=false`；官方 PubSubClient 使用 QoS 0。这里的可靠性
不依赖上行 MQTT QoS：若 ACK 丢失，Broker durable ledger 会重发 exact envelope，设备从原子记录返回
同一 `operationId + accessMaterialDigest + storageRevision`。因此无需为了 58 B ACK 引入第二套 MQTT 客户端；未来支持
QoS 1 publish 的 adapter 可以选择 QoS 1，但不能改变应用 ACK 合同。

`GatewayAccessDeliveryAckV2` 是 canonical CBOR fixed array，长度固定为 4：

```text
0 version = 2
1 operationId bstr[16], non-zero
2 accessMaterialDigest bstr[32]
3 storageRevision uint32, non-zero
```

最大编码 58 B。`accessMaterialDigest` 是 exact 48 B plaintext 的 SHA-256，必须恰为 32 B，但不能额外规定“非零”；协议校验器
不应给密码学摘要创造不必要的值域例外。ACK 不携带 success bool 或错误字符串：只有原子 durable store 成功
才发送 ACK，失败由不发送、重试和最终 expiry 表达。Cloud Gateway 只有在 tenant、exact Hub、operation、digest、
expiry 全部一致时，才以 CAS 将 topology 从 `PendingAccessDelivery` 推进到 `PendingChildInstall`；ACK replay
不得再次递增 topology version，变化的 storageRevision 必须拒绝。

### 5.4 Gateway proof command/result

G2A-2f 继续复用上述同一对 session-local management topic，不增加第二个 MQTT client 或第二套 bearer。下行
`GatewayProofCommandV1` 是 canonical CBOR fixed array，长度 13、最大 252 B：

```text
0  version = 1
1  kind = ProofCommand = 1
2  operationId bstr[16]
3  topologyVersion uint32
4  childLogicalDeviceId ASCII tstr[1..128]
5  childDeviceInstanceId bstr[16]
6  accessEpoch uint32
7  gatewayCredentialVersion uint32
8  presenceKeyVersion uint32
9  accessMaterialDigest bstr[32]
10 accessStorageRevision uint32
11 proofNonce bstr[16], non-zero
12 expiresAtUnixSeconds uint64
```

Hub 必须逐字段匹配 protected `GatewayAccessRecord`，并且 command expiry 不得晚于 credential expiry。只有 exact
Presence locator 建连、Hello、Gateway Method 2 与 DirectSecure 全部成功后才能回 `Secure`。短暂扫描/GATT 失败
不立即形成失败事实，而是在 command TTL 内以有界节奏重试；record 变化、身份不匹配为 `Rejected`，可信时间失效
或 TTL 到期为 `Expired`。

上行 `GatewayProofResultV1` 固定 7 项、最大 48 B：

```text
0 version = 1
1 kind = ProofResult = 2
2 operationId bstr[16]
3 topologyVersion uint32
4 proofNonce bstr[16]
5 status = Secure(1) | Rejected(2) | Expired(3)
6 presenceKeyVersion uint32
```

最后一项是 command 绑定版本的回显；仅当 status=`Secure` 时它才同时表示实际匹配版本。Proof 的信任根是
authenticated exact Edge Hub MQTTS session、Broker durable exact-command ledger 和 pending topology CAS。Gateway
secret 已发给 Hub，因此再让 Hub 用该 secret 计算一个所谓“child proof HMAC”不能提供独立 child 证据，反而会
制造虚假安全边界，本方案明确不增加该 MAC。

Service 只有在 access durable ACK、经既有 Admin/DirectSecure 验证的 child durable receipt、以及 exact Hub
`Secure` result 三项齐全时，才从 `PendingGatewayProof` CAS 到 `Active`。`Rejected` 进入
`RollbackRequired`；`Expired` 清除当前 nonce 并推进 topologyVersion，允许显式生成新 proof command。重复
command/result 返回 exact 语义，Broker/Hub 重启后从 ledger/同一 durable access record 恢复，不因重复消息重复
推进 topologyVersion。

Proof 的 `Secure` 是该任务验证时刻的证明，不是永久占用 child BLE 的授权。任务只释放自己建立的 exact
connection attempt；已存在且由其他模块持有的合格会话可以被验证，但不因 proof 回执/缓存清理而断开。
自建连接的断连尚未完成时 `busy()` 保持 true，避免管理任务换手早于链路释放。回执仍可独立 publish/exact
replay；新命令须等旧任务不 busy。全 composition 的 stop/reset 仍负责最终物理资源关闭。具体实现和测试见
§10.50；此合同不替代普通 route 的 idle/cold-resume，不能把 proof 释放等同手机 Direct 已可交替使用。

## 6. topology 与 attach 状态机

```text
Created
  -> PendingAccessDelivery
  -> PendingChildInstall
  -> PendingGatewayProof
  -> Active

Created（尚未投递）
  -> Cancelled

任何已进入 Broker 投递的 pending
  -> Expired | RollbackRequired

Active
  -> Revoking
  -> Detached
```

wire/API 状态码固定为：`Created=1`、`PendingAccessDelivery=2`、`PendingChildInstall=3`、
`PendingGatewayProof=4`、`Active=5`、`Expired=6`、`Cancelled=7`、`RollbackRequired=8`、`Revoking=9`、
`Detached=10`。终态不得回到 pending/active；Active/RollbackRequired 只能进入 Revoking，Revoking 只能进入
Detached。

约束：

- `PendingAccessDelivery`：Service 已密封 Gateway secret + PresenceKey，Edge Hub 尚未 durable ACK；
- `PendingChildInstall`：Hub 已保存，App/child 尚未返回 durable mutation receipt；
- `PendingGatewayProof`：两端均保存，Hub 尚未对 child 完成 Method 2；
- `Active`：三项证明齐全后唯一允许 child route 的状态；
- Service 一旦向 Broker stage 过 sealed delivery，即使尚未收到 Hub ACK，也不能证明 command 没有在途；显式 cancel
  因此保守进入 `RollbackRequired`，不得直接复用该 topology/secret。`Cancelled` 只保留给尚未投递的 Created；
- child 已安装但 attach 失败时进入 `RollbackRequired`，Cloud 永不转发；下一次 Admin Direct 会话精确 revoke/rotate，
  不能假装服务器能在离线 child 中立即擦除 secret；
- 所有迁移使用 operationId/idempotency key 和 compare-and-swap version；App kill、Hub reboot、Cloud Gateway/Broker
  reboot 后必须恢复同一状态，不创建第二份 secret。

## 7. G2A：已认领 child 绑定 Edge Hub

```text
1. Edge Hub DeviceKey -> HTTPS session -> mandatory MQTTS online
2. Owner App selects exact child and exact Edge Hub
3. Service validates owner/admin + tenant + hub capability
4. Service creates pending topology and KMS Gateway secret
5. Service -> MQTTS GatewayAccessEnvelopeV2 -> Edge Hub durable ACK
6. App -> HTTPS fetch pending relay package (protected memory only)
7. App -> child Admin Method 2 + DirectSecureRecord
8. child -> fresh ControllerControl nonce
9. Service signs digest-bound ControllerGrant
10. App exact-relays grant + raw secret to child
11. child atomically stores and returns durable receipt
12. App commits receipt to Service
13. Service asks Edge Hub to connect child
14. Edge Hub -> child Gateway Method 2 + DirectSecureRecord
15. Edge Hub reports proof; Service CAS topology Active
16. Broker enables exact child route and reachableViaGateway
```

App 在步骤 6～11 之间被杀：重新进入时按 operationId 获取同一 relay package、fresh control nonce 和必要的新
grant；不能重新生成 secret。grant 可因 nonce/expiry 更新，secret/controllerId/version 不变。

## 8. G2B：Edge Hub 辅助首次认领

G2B 在 G2A 完成后实施：

1. owner App 为 exact Edge Hub 开短 permit-join；
2. physical child 进入 setup window；
3. Edge Hub 扫描并只做 BLE fragment opaque relay；
4. Noise、BleEnrollmentGrant、Admin secret 和 receipt 保持 App <-> child 端到端；
5. child 成为 AppOwnedDirect 后，按 G2A 步骤 3～16 安装 Gateway credential；
6. ownership 成功但 Gateway attach 失败时，设备保持正常 AppOwnedDirect，不回滚 ownership。

Edge Hub 不终止 Noise、不读取 Admin secret、不因扫描到广播自行创建 ownership/topology。

### 8.1 G2B-0：窗口控制合同（完成）

设备与 Service 共享协议包已冻结 33 B 上限的 canonical CBOR command/result：

```text
[1, 5, operationId:bstr16, adapterId:uint16, Open|Close, expiresAt]
[1, 6, operationId:bstr16, adapterId:uint16, status, expiresAt]
```

当前 registry 只登记 `NativeGatt=1`，但 decoder 接受任意非零 uint16，使旧 Hub 能以 `Unsupported` 明确拒绝
additive adapter，而不是把 wire 当成损坏。消息中没有 secret、child identity、MAC 或候选 locator。App 不消费
MQTT management wire，故本批不增加无用途的 App codec。

### 8.2 G2B-1：exact Hub 短窗口控制面（完成）

Service/Broker 已完成窗口资源与可靠 management 下发，仍不启动 BLE 扫描：

- owner App 通过 `POST /api/v2/edge-gateway/permit-joins` 指定 exact
  `edgeHubLogicalDeviceId + adapterId`，`Idempotency-Key` 是 canonical 16 B operationId；
- 第一版窗口固定 300 s，不允许 App 任意放大 TTL；同一 tenant/exact Hub 只允许一个
  `pending_open | ready | pending_close` 窗口；
- Service PostgreSQL 只保存 owner/tenant/Hub/adapter/window state，不保存 child、MAC、候选广播、ownership、
  topology 或 secret；
- Broker SQLite ledger 保存 exact 33 B command/result，以 QoS 1、`retain=false` 投递；Hub 离线或 Broker
  重启后只重投同一未过期帧；
- Hub result 只能由该 tenant/exact Hub 的已鉴权 management session 回写；状态图固定为
  `PendingOpen -> Ready|Expired|Rejected|Busy|Unsupported`、`Ready -> Expired|Rejected`、
  `PendingClose -> Closed|Expired|Rejected`。`Ready` 是持有扫描/GATT 资源的活动态，不是终态；
  `Closed/Expired/Rejected/Busy/Unsupported` 终态不能倒退；
- `GET /api/v2/edge-gateway/permit-joins/{operationId}` 对齐 Broker durable result，
  `POST .../{operationId}/close` 幂等关闭；到期由 Service 与 Broker 各自硬截止。

G2B-1 没有产生 child ownership/topology，也没有把 App 接入 Hub MQTT 凭据。设备尚未消费 Open/Close，需等
G2B-2 adapter/coordinator 完成后才可开启真实扫描入口。

### 8.3 G2B-2a：设备窗口执行器与 Native GATT fragment pump（完成）

设备侧已把 G2B-0 wire 接入现有 authenticated management client，并实现以下边界：

- `GatewayPermitJoinCoordinator` 只负责 exact operation/adapter/expiry、Open/Close 幂等和
  `Ready/Closed/Expired/Rejected/Busy/Unsupported` result，不读取候选或 enrollment payload；
- `Closed/Expired` 对同一 operation identity 是单调终态；延迟重放的旧 `Open` 只能得到原终态，不能重新开窗；
- `NativeGattPermitJoinAdapter` 复用唯一的 `IBleCentralPort`，只接受合法 Provisioning + EnrollmentV2 或
  Direct v3 + AuthorizedPresenceV1 广播；native address 仍留在 Port，向上只暴露 8 B rotating setup/presence
  locator、wire/capability 和 RSSI；Legacy 全零 Direct locator 不进入候选；
- 第一版固定最多 4 个临时候选、单连接、每片最多 20 B。App→child 不设 Hub TX ring：只有 southbound Port
  已消费/复制 exact fragment 后才 ACK Broker，`WouldBlock` 时不 ACK、不推进 sequence，由 Broker 重投同一片；
  child→App 的 GATT notify 没有反向 cloud-credit 机制，必须能暂存一个最大 512 B logical record，因此默认保留
  32 个 ATT20 fragment 槽。RX 溢出立即使窗口 fail-closed，不丢片后继续 Noise transcript；
- App 选择 exact locator 前只扫描不连接；选择后仍把 BLE fragment 原样转发，不解析 Noise、Grant、receipt、
  ControllerCredential 或 BBP/2；
- permit-join 与既有 Gateway Method 2 共用同一个 NimBLE Central。打开窗口时通过显式 lease 停止 child route/session，
  关闭、到期或故障后释放，Edge Hub product 才恢复普通 child data plane；适配器未持有 lease 时不得 stop 或改写
  共享 Port callback；没有第二个 NimBLE host/扫描栈；
- Hub reboot 不保存窗口或候选；只有 Broker 对尚未过期 exact Open 的重投可重新开启。扫描、候选和连接都不会创建
  ownership/topology，也不会落 Flash。

设备内 bounded fragment pump 已由 G2B-2b 的独立 relay client 接入 northbound wire。仍不得把 App 用户 MQTT
凭据交给 Hub、把 raw fragment 塞入普通 ChildRouteEnvelope，或让 Service 解析 Noise。

后续严格拆批：

1. G2B-3：App 以现有 enrollment client 驱动 relay transport，校验 `relaySessionId`，成功后主动 Close，再调用
   既有 G2A attach；
2. G2B-4：双账号、错误 Hub、过期、篡改/重放、App kill、Hub/Broker reboot 四端门禁。

### 8.4 G2B-2b：operation-bound northbound relay（批次化修订中）

#### 2026-09-02 四端真机纠偏：云端不能逐 ATT20 stop-and-wait

真实 App/Service/Edge Hub/XIAO 子设备门禁证明，旧的“一个 1..20 B ATT packet 对应一次
HTTPS -> Broker -> MQTT -> Hub -> ACK 往返”不是可发布的数据面：

- 单片现场间隔约 1.3～1.9 s；25 片 enrollment record 需要约 35～45 s；
- Provisioning 的 2 s 片间重组上限首先在第 17 片附近稳定失败。将配网通道的片间空闲上限修正为
  10 s 后，25 片请求完整到达，设备成功持久化 Admin 并从 `Provisioning` 进入 `Active`；
- 随后的 Direct/Method 2 会话仍受 2 s record 重组和 15 s 未认证会话门禁约束，在 7 down / 7 up
  后断开，App 最终得到 `EDGE_GATEWAY_PERMIT_JOIN_RECEIVE_TIMEOUT`；
- 继续放宽所有 Direct 超时会扩大单连接占用 DoS 窗口，却不能减少 HTTP/MQTT 消息数量，因而不是
  根因修复。

主线改为“有界不透明 packet batch”：每个 northbound batch 最多包含 16 个、每个 1..20 B 的原始
ATT packet；Hub 仍不解析或重组 Noise、Grant、receipt、BBP/2/DirectSecureRecord，只在本地按顺序把
batch 写入 child，或把已收到的 packet 合并为 batch 上送。按 CBOR array 计入 operation/direction/
sequence 后仍控制在约 400 B wire 预算内；一个最大 512 B logical record 最多两次云往返。可靠性仍是
每方向单槽 stop-and-wait，但 ACK 的原子单位从 packet 改为 batch：Hub 仅在 batch 内所有 packet 都获得
ATT Write Response 后 ACK，下行 `WouldBlock` 时保留当前位置继续；App 仅在 batch 内所有上行 packet
按序进入 reassembler 后 ACK。任一 packet 非法、部分批次越界、operation/session/sequence 不匹配均
fail-closed。

这项修订不增加 child RAM，不改变 4 B BLE fragment、Noise 或 BBP/2 wire，也不降低 15 s 未认证连接门禁。
Hub 复用现有 32×20 B 上行容量，下行新增至多 16×20 B 的固定 pending batch；Broker 仍只保留一上一下
两个瞬时 batch，不持久化 payload。旧逐 packet northbound wire 尚未发布，因此直接删除，不保留双协议分支。

首次认领尚无 child logical identity 和 active topology，因此不得复用 `ChildRouteEnvelope`。relay 继续走 exact Hub
的 authenticated management session，但由独立 relay client 处理，窗口 coordinator 不读取候选和 fragment。

canonical CBOR fixed-array wire 冻结为：

```text
CandidateSnapshot = [1, 7, operationId:bstr16, revision:uint32,
                     [[candidateToken:bstr1..16, wireVersion:uint8,
                       capabilities:uint16, signalQuality:0..100] x 0..4]]
SelectCommand      = [1, 8, operationId:bstr16, candidateToken:bstr1..16]
SelectResult       = [1, 9, operationId:bstr16, candidateToken:bstr1..16,
                     status, maxPacketSize:uint16]
RelayBatch         = [1, 10, operationId:bstr16, direction, sequence:uint32,
                     [packet:bstr1..20] x 1..16]
RelayAck           = [1, 11, operationId:bstr16, direction, sequence:uint32, status]
```

`candidateToken` 是 adapter-scoped 临时句柄；Native GATT 使用当前 8 B rotating setup locator 或授权 Direct
presence locator，MAC 不进入 wire。
`signalQuality` 只用于本次窗口内排序，避免把原始射频标识误当身份。direction 固定 `Down=1`、`Up=2`；sequence
每方向从 1 开始。Select 状态固定 `Connecting/Connected/NotFound/Disconnected/Rejected`，ACK 状态固定
`Accepted/Rejected`。

可靠性采用每方向一个 **batch** 的单槽 stop-and-wait；ATT packet 仍是南向原子单位，但不再把每个 packet
放大成一次云往返：

- App→child：Broker 只保留一个未 ACK batch，以 MQTT QoS 1 重投；Hub 保存 exact encoded batch 和当前 packet
  下标，依次调用 `IBleCentralPort::sendPacket()`。只有 batch 内所有 packet 都获得远端 ATT Write Response 才
  返回 batch ACK；把 packet 放进本地 Controller 队列不算成功。child 返回 `INSUFFICIENT_RES`、本地
  `WouldBlock` 或链路 `NotConnected` 时不 ACK、不推进 sequence，恢复后从尚未确认的 packet 继续，不重写
  已确认前缀；当前发布端口必须提供 GATT Write，Write Without Response 不能承载这条可靠控制链路；
- ESP32 的 NimBLE Host/Controller 在首次 permit window 时惰性初始化，并在产品实例生命周期内保持单例；关闭窗口
  只停止 scan、断开 ACL 并清理 operation 状态，不反复 deinit/init Controller。ESP-IDF Controller 不是窗口级资源，
  把其生命周期错误绑定到窗口会造成第二次初始化时的 IPC/Controller 状态损坏；析构只作为最终 shutdown 边界；
- child→App：网关专用 GATT 订阅必须使用 Indication，child 依据 CCCD 选择 Indication，普通手机直连仍可使用
  Notification；ESP32 Central RX 固定容纳一个最大 512 B record 的 32×20 B packet。Hub 以 20 ms 有界聚合或
  16 packet 上限形成 batch，Broker 去重并保留；App 仅在 batch 内所有 packet 均被重组器接受后 ACK。不能把已
  进入本地 Controller 的 Notification 当成端到端可靠交付，也不能用小型 burst queue 冒充完整 record 容量；
- duplicate batch sequence 返回同一 ACK；已接受 batch 的延迟 exact duplicate ACK 即使在下一 sequence pending
  后到达，也按幂等 replay 处理且不得清除新 pending；字节不一致、gap、operation 不匹配、空 batch、超过
  16 packet、单 packet 超过 20 B、encoded batch 超过 363 B、窗口非 Ready 或超过额度均 fail-closed；
- candidate snapshot 最多 4 项，仅变化或低频 heartbeat 上报；流量额度仍按实际 ATT packet 而非 batch 数计算，
  每 operation 最多 4096 个 packet；Broker/App 之间继续使用 batch 单槽 backpressure。GATT indication 到云 ACK
  之间不存在端到端 credit，故 Hub 的 child→App 队列只补齐“一个最大 logical record”这一必要边界，不能继续
  扩成无界流量缓冲；
- Broker relay mailbox 是窗口期瞬时状态，不写 Service PostgreSQL，也不进入 analytics/log；每次 Broker relay
  runtime 启动生成新的 16 B `relaySessionId`。App 发现该值变化必须废弃旧 Noise、关闭旧 operation，并以新
  operation/新物理 BLE 会话从 Hello 重启；Broker restart 不改变 ownership；
- Select 不创建 ownership/topology。只有既有 BLE enrollment receipt commit 成功后设备才成为
  `AppOwnedDirect`，随后再进入 G2A attach。

G2B-2b 只实现 shared codec、Hub relay client、Broker bounded mailbox 和 owner HTTPS relay API；App 的
`BleDirectClient` transport adapter 属于 G2B-3，不在本批复制 Noise/Grant/receipt 逻辑。

本批实际落地边界：

- C++/JavaScript 共享 kinds 7～11 exact codec；candidate token 非空且不得全零，operationId/sequence/direction/
  status、1～16 packet/batch、20 B/packet 和 363 B encoded 上限均在 decoder fail-closed；
- `IGatewayPermitJoinRelayAdapter` 把候选、选择和 opaque packet pump 从 Native GATT 中抽出；未来 Blinker-native
  ESP-NOW/串口 NCP 只实现该南向合同，不改 MQTT、Noise 或 enrollment；Zigbee/BLE Mesh 仍走 standards-native
  commissioning adapter，不伪装成 raw BLE fragment；
- `GatewayPermitJoinRelayClient` 复用现有 authenticated management topic 与唯一 MQTT client；使用调用方可见的
  固定上限 workspace 保存一个 exact batch 和一个最大 logical record，不解析 Noise/Grant/receipt、不分配 heap；
- Broker 每 operation 固定一个 down pending 和一个 up pending，最多 64 个活跃 mailbox、每方向 4096 packet；下行
  QoS 1 exact 重投，上行必须等 owner App 的 explicit ACK 才 ACK Hub；
- owner API 为 `GET|POST /api/v2/edge-gateway/permit-joins/{operationId}/relay`。GET 返回当前候选/选择结果/
  ACK/上行 batch 和 `relaySessionId`；POST 只接受 operation-bound Select、Down batch 或 Up ACK；Service 每次都重新
  校验 owner、tenant、exact Hub 和 Ready 窗口。

软件门禁：C++ Host `42/42`、共享协议 Node `17/17`、Broker Device V2-only、Gateway permit-join Service/HTTP
均通过。ESP32 Arduino composition 编译和真实 App/Hub/child 四端 Noise 闭环分别属于本节发布编译门禁与
G2B-3/4，不能用 Host 结果冒充真机结论。

### 8.5 G2B-3：App relay transport 与 G2A 串联（软件完成；真机待完成）

App 已在现有 `BleDirectRecordLink` 边界增加 Gateway HTTPS relay 实现，没有复制 Noise、EnrollmentGrant、receipt、
Method 2 或 BBP/2 状态机：

- HTTP client 严格校验 `no-store`、exact response shape、operationId、16 B relaySessionId、revision 和每类 frame
  上限；任意 operation/session 换代、revision 倒退或 non-canonical CBOR 均 fail-closed；
- `GatewayPermitJoinRecordLink` 只负责开窗、读取最多 4 个候选、选择 exact token、最多 16×20 B 的 batch
  stop-and-wait 和
  主动关窗；不保存 raw fragment，不解析 enrollment payload；
- 控制请求仍使用 15 s 有界超时；record 使用 45 s **无进展超时**，每接受一个 exact batch 后续期，
  Permit Join 窗口的 `expiresAt` 才是整次操作的硬上限。最大 512 B logical record 在 ATT20 下需要 32 个 packet、
  至多两个 cloud batch，不能给它设置会惩罚正常进展的固定总时限，也不能无限续期越过服务器窗口；
- 每个 Permit-Join operation 只承载一段物理 GATT 会话。Provisioning receipt 后先 Close 旧窗口，使设备完成
  Provisioning -> Direct 切换；随后新建 operation 发现 Direct v3，并用刚签发的 PresenceKey 校验 8 B locator 后
  才连接。协议不增加 `Disconnect/Deselect` 命令；
- 多候选不会“扫描后取第一个”。业务入口返回同一窗口内的候选，由既有 UI/调用方明确选择；取消会 Close；
- 本地 Capacitor BLE 与 Gateway relay 共用同一 fragmentation/reassembly、`BleDirectClient` 和 Direct presence
  matcher，避免两套行为逐渐分叉。

安全通道的发送边界必须覆盖“加密 + 完整 logical record 交付”，并在 App 内单飞串行。BBP/2 可靠 Patch 的
response ACK 可由 receive loop 发起，同时前台 request 也可能发送；若只串行化 HTTPS 请求，两个
`sendRecord()` 仍会交错争用 Broker 的单 down-batch 槽，直接链路还可能让两个异步 AES-GCM 调用读取同一个
send sequence。正式实现因此在共用 `BleDirectSecureChannel` 串行化整条发送事务；Gateway HTTP 层继续只负责
请求次序，Broker 的 `EDGE_GATEWAY_PERMIT_JOIN_RELAY_BUSY` 保留为冲突保护，不作为正常流控让 App 无限重试。
该修正同时覆盖 Native GATT 与 Gateway relay，不新增 ACK-of-ACK、wire kind、设备端状态或 RAM。

当前软件门禁已覆盖跨端 canonical golden、双窗口 Provisioning/Direct、上下行 batch/ACK、共用安全通道并发
发送单飞、
`relaySessionId` 换代和 Native GATT authorized Direct 候选。产品入口已收敛到 owner Edge Hub 设置页的
“添加蓝牙子设备”；页面固定展示 exact Hub，始终要求用户明确选择临时候选码/RSSI，不显示或使用 MAC；
`enrollAndAttach()` 在 ownership 成功后复用当前 DirectSecure Admin session 安装 Gateway credential，关闭该窗口，
再以新的 Gateway relay link 做 Gateway Method 2 proof，并把 exact receipt 交给既有 G2A CAS。attach 失败只关闭
session/窗口，不删除 Admin credential、不回滚 ownership。

为避免 App kill 后丢失 operationId 导致 exact Hub 被旧 300 s 窗口阻塞，App 在发送 Open 前只保存
`operationId + edgeHubLogicalDeviceId + adapterId` 清理检查点；确认 Closed/Expired 后删除。重新开始同一 Hub 的认领前
先幂等 Close 遗留窗口，再创建新 operation。检查点禁止保存 candidate、MAC、Noise state 或 raw fragment；关闭结果
不确定时保留检查点，不能假装已经释放。

Enrollment 的签发 TTL 与安装后恢复必须分开。TTL 到期后 Service 把 intent 标为 `expired`，拒绝再把该 signed
grant 用于一次新的首次安装，但不能立即删除 sealed pending PresenceKey：设备可能已经持久安装 grant，而 App 恰好
在 receipt commit 前退出。持有同一 pending credential 的 App 必须先经 Presence locator 找到 exact child，并完成
Method 2，再提交原 exact receipt；Service 仅在 owner/tenant、intent、receipt、epoch/controller 及 pending
PresenceKey 全部一致时允许 `expired -> active`。若同一 physical instance 签发了更新 intent，旧 expired intent
立即转为 `superseded` 并删除其 pending PresenceKey；cancelled/superseded 永不可恢复。这样既不靠延长短 TTL 掩盖
故障，也不会把已经安装的 Active child 留成无法认领的孤儿。

G2B-4 的软件恢复入口不再把所有 Pending 凭据盲目归入用户当前打开的 Hub。App 在 BLE pending credential 已经
安全落盘、但 enrollment grant 尚未发给 child 之前，先持久化只含 exact Hub/child logical id 的无秘密 workflow
标记。若 App 被杀，Pending credential 继续 `BleDirectClient.resume()`；若 child ownership 已 commit、但 attach
checkpoint 尚未产生，则以同一标记和 PresenceKey 经 exact Hub authorized Direct 重连后开始 G2A attach；若 attach
checkpoint 已存在，以更精确的 operation 恢复并隐藏重复 workflow 项。终态清理固定先删 workflow、再删 attach
checkpoint，因而两次删除之间掉电仍保留可恢复的精确 operation。缺少 exact Hub 标记、PresenceKey 或凭据的记录
均 fail-closed，禁止退回 MAC、广播名或“第一个候选”。尚未完成的是这些路径的真实 App force-stop 四端复测，以及
双账号/错误 Hub、超时/篡改/replay、Hub/child/Broker reboot 门禁；软件编译与分层单测不能替代发布结论。

所有 App durable Direct/恢复记录还必须绑定 `(service authority, accountId)`，运行中的扫描、HTTP、GATT 和恢复
事务绑定登录 `sessionEpoch`。登录/退出或切换账号会中止扫描、关闭 Direct session、清空内存 snapshot；旧账号请求
即使稍后成功返回，也不得被新账号消费，返回的 grant、PresenceKey、Gateway secret 和 relay frame 必须先清零再
fail-closed。access token 的同账号刷新不推进 `sessionEpoch`，不能无故断开本地 Direct。历史上没有 account/authority
字段的测试期 credential/checkpoint 不允许按“当前账号”自动归属，必须忽略并重新 enrollment；这是一次测试数据
失效，不新增 wire、设备槽或 Service schema。

Attach 恢复还必须按资源所有权分段：`Created/PendingAccessDelivery` 只推进 Service→Broker→Hub 的 sealed
delivery，不得提前打开并占用 child GATT；只有状态到达 `PendingChildInstall` 后才建立 Admin DirectSecure session，
安装完成立即关闭，再用独立 Gateway Method 2 会话证明新 credential；`PendingGatewayProof` 又回到纯云端推进。
App 每次重入先执行无 BLE 的 cloud reconciliation，若 Hub delivery 尚未完成则返回“等待确认”，而不是让一个
已经认证的 child session 跨越任意长的 MQTT/Hub 等待。该分段也确保 retry 只重放 exact operation，不依赖内存中的
旧 session。

## 9. 最小 ChildRouteEnvelope

G2 不能等待完整 G3 才定义路由。G2-0 先冻结只服务 child proxy 的最小外层：

exact wire 是 canonical CBOR fixed array，长度固定为 8：

```text
0 version = 1
1 topologyVersion uint32, non-zero
2 childLogicalDeviceId ASCII tstr[1..128]
3 childDeviceInstanceId bstr[16]
4 accessEpoch uint32, non-zero
5 gatewayCredentialVersion uint32, non-zero
6 correlationId bstr[16]
7 innerBbp2Frame bstr[1..4096]
```

最大编码为 4,280 B；实际 route admission 还必须按目标 child profile、BBP/2 kind、frame size 和 rate 收紧。
codec 只负责 canonical shape，不替代 Broker/Edge Hub 的 BBP/2 语义与授权检查。

上行 source 不接受 child payload 自报。Cloud Gateway/Broker 使用已认证 Edge Hub session + active topology 覆盖
source。下行只允许 envelope 指定 active topology 中的 child，Edge Hub 只有在本地 Gateway Method 2 session 与
envelope epoch/version 一致时才发送。

G2-0 只允许 Manifest、State、Command、ACK/Error、受限 Event/Telemetry；任意 destination、service route、
device-to-device、group、offline automation、TTL/loop policy 留到 G3。

## 10. Edge Hub 设备实现

首版只实现 ESP32-class target，普通 Arduino child API 不变。Edge Hub 在产品语义上是“普通 WiFi
云设备 + BLE Central 网关能力”；代码层使用 composition 复用 WiFi cloud core，不从普通
`WiFi+BLE` product 继承。后者的 BLE 是 WiFiProv/设备端 Peripheral Direct，而网关需要 Central，两者的
连接所有权、扫描、会话与资源边界不同。内部结构冻结为：

```text
EdgeHubProduct
  WifiCloudCore                 // DeviceKey session + MQTTS + 自身 BBP/2
  BleGatewayExtension
    GatewayManagementClient
    GatewayAccessStore          // 网关持有的 per-child 出站凭据与 Presence locator
    ChildSessionCoordinator
    GattDirectChildLink          // BLE Central
    ChildRouteBridge
```

### 10.0 南向模块边界（2026-09-01 修订）

原结构中的 `BleGatewayExtension` 只是一种产品装配，不是未来全部网关的基类。冻结以下两类扩展点：

1. **Blinker native record bearer**：GATT、ESP-NOW、串口/SPI NCP 可实现相同的完整 logical-record link 和
   bearer-specific selector，直接组合 `BasicGatewayChildSession<Link, Selector>`；不复制 Method 2、DirectSecure、
   BBP/2 或 cloud route bridge。
2. **standards-native network**：Zigbee、BLE Mesh 使用自身 commissioning/security/model，由独立 protocol adapter
   将 ZCL/Mesh Model 映射到 Blinker Property/Action/Event；只复用 northbound topology/ACL/ledger/route 语义，不伪装
   成 GATT record link。

当前已删除误导性的 `Bbp2BleRecordFormat`：raw BBP/2/DirectSecure record codec 位于 `protocol/DirectRecord`；
`GatewayChildSession` 不再 include GATT/BLE discovery，而由 `GatewayGattChildSession` 组合
`GattDirectChildLink + GatewayPresenceMatcher`。该组合仍以 template 静态绑定，普通产品和未选 adapter 不承担 virtual
dispatch、固定 slot 或额外 workspace。

不得把 child 的三槽 `DeviceAccessStore` 复用为 Hub 的 `GatewayAccessStore`。前者表达“哪些 controller
可以进入控制这一台设备”；后者表达“这一台 Hub 可以以什么凭据访问哪些 child”，是可按 child
数量扩展的反向关系。G2-0D 首个闭环固定一个 child/一个 record，测量后再扩容，不预留未验证的
大表。

凭据归属冻结如下：

| 设备能力 | Cloud DeviceKey | 本机 controller 三槽 | Hub per-child secret store |
|---|---:|---:|---:|
| WiFi-only | 需要 | 不需要 | 不需要 |
| BLE-only Peripheral/child | 不需要 | 需要：Admin + Shared + Gateway | 不需要 |
| 普通 WiFi+BLE Peripheral | 需要 | 需要：Admin + Shared + Gateway | 不需要 |
| Edge Hub（WiFi + BLE Central） | 需要 | 默认不需要 | 需要：每个 active child 一份 |

“默认不需要”的边界是：Edge Hub 自身通过 Cloud/Broker ACL 受控。只有未来明确增加“App 通过
BLE Direct 控制 Hub 自身”能力时，才为 Hub 叠加本机 controller access extension，不因它具有 BLE 硬件就
默认分配三槽。WiFi-only 未来若新增 LAN Direct，也按同一能力规则叠加，不把凭据绑定到传输名称。

约束：

- 新增独立 BLE Central port；不得把当前 child peripheral `IBleLink` 塞入双向条件分支；
- `GattDirectChildLink` 复用现有 fragment、Method 2、DirectSecureRecord 和 BBP/2 codec；
- 首个 E2E 固定单 child、单 active GATT session、固定 workspace；证明后再测量并提高 bound；
- 默认 non-persistent：认证、交换、ACK 完成或 idle lease 到期即断开；
- 未认证连接、错误 presence、错误 Method 2、fragment stall 均有短 deadline/backoff；
- PresenceKey 只用于在授权列表中定位 child，不是认证；MAC/名称不得持久映射 logical device；
- `reachableViaGateway` 只在近期成功 Method 2 + authenticated exchange 后成立，扫描到广播和 topology active
  都不能单独标在线；
- App Direct window 优先；Edge Hub 必须能响应 Cloud/App release-child 请求并在 bounded time 内让渡；
- secret store 使用平台受保护原子存储；重启后不经 MQTT 明文重放即可恢复，完整擦除时由 Service 重新密封下发。

### 10.1 G2-0D 当前设备实现

已完成的 D0～D3：

- `GatewayAccessEnvelopeSecurity` 流式计算 V2 exact salt transcript，与 Service 固定向量得到同一 HKDF key；
- `GatewayAccessStore` 使用 352 B schema v2，仅保存一个 child，包含 operation/envelope/access-material digest、
  child identity、epoch/version/expiry、32 B Method 2 secret、active+pending Presence locator、独立 storage/delivery
  revision 与 CRC；PlainFlash fail-closed；
- exact QoS replay 不写 flash；DeviceKey refresh 对同一 operation 重新密封时只更新 envelope binding，保持
  delivery revision 和 ACK；本批同一 accessEpoch 只实现相同 Method 2 credential 下将更高 PresenceKey stage 到
  pending，credential rotate/detach 的事务与设备 mutation 在 G2A-3 闭合；
- `GatewayAccessDeliveryProcessor` 严格执行 decode、当前 DeviceKey version、可信 Unix time、AEAD tag、原子写入、
  read-back verify，之后才编码 58 B ACK；失败路径清零 workspace 且不产生 ACK；
- `GatewayManagementClient` 通过 `MqttFrameTransport` 单一 side-channel handler 复用官方 PubSubClient；MQTT callback
  只拷贝 bounded envelope，crypto/NVS/publish 均延后到 `poll()`，避免 callback 重入和第二 MQTT 栈；以 4 B
  connection generation 识别新 MQTT session，即使未观察到中间 Backoff，也会重新订阅非持久 management topic；
- Host 覆盖明文存储拒绝、CRC 损坏、写失败、identity/expiry/tag 错误、exact replay、rewrap、rotation、重启 replay、
  MQTT reconnect resubscribe 和 QoS0 ACK retry；Broker 已允许 ACK QoS 0/1，仍拒绝 QoS 2/retain/rate overflow。
- 最终单库 clean compile 已覆盖 ESP32-S3 WiFi+BLE、UNO R4 WiFi、UNO R4 BLE 与 Wio Terminal WiFi，确认
  Edge Hub 内部模块没有改变普通产品公开 API，也没有引入第二个 Arduino library/ZIP。

目标绑定不把 tenant/logicalDeviceId/environment 复制进所有 DeviceKey session arena。它由“唯一 DeviceKey + 当前
credentialVersion + exact KDF/AAD + 专用 Edge Hub Broker ACL”强制；envelope 内三字段始终被 AEAD 认证。产品若已有
期望值可额外 equality check。这样避免让 UNO R4/普通 WiFi 设备为 Edge Hub 固定增加约 300 B credential RAM。

D4a 已完成内部绑定：ESP32 Edge Hub product 提供 MQTTS-only cloud profile、可信 Unix time 和非 PlainFlash 的
`GatewayAccessStore` backing，并且没有新增公开 `BlinkerEdgeHub` facade。D4b 尚未完成真实
sealed delivery—物理断电—Broker redelivery—same ACK 门禁。由于当前没有可用于 Flash Encryption/eFuse 生命周期
验证的专用可报废 ESP32，D4b 物理项调整为**发布阻断门禁**：允许先实现并以 Host/编译门禁验证与平台无关的
单-child Central 安全核心，但不得据此启用公开 Edge Hub API、使用 PlainFlash fallback 或宣称 production ready。

#### 10.2 D4 ESP32 安全绑定冻结

生产 v1 只接受硬件 Flash Encryption 保护的 NVS partition。运行时必须同时验证：

1. `esp_flash_encryption_enabled()` 为真；
2. `esp_partition_find_first(..., ESP_PARTITION_SUBTYPE_DATA_NVS, label)` 找到 exact partition；
3. `esp_partition_t::encrypted` 为真。

任一条件失败，Edge Hub platform 在打开 `GatewayAccessStore` 前返回 `NotConfigured`，不得把普通 NVS、编译进
固件的固定密钥、DeviceKey 派生软件包裹或“测试 verifier 返回 true”标成 `EncryptedFlash`。默认开发板不自动
烧写 eFuse，也不提供 plain fallback；完整擦除后的恢复路径是 Service 使用当前 DeviceKey 重新密封下发。

普通开发板的工程门禁分为两部分：Host/Arduino compile 验证完整 composition；真机 probe 验证未启用硬件保护时
必定 fail-closed。只有使用已按生产分区表和 Flash Encryption 流程预置的 ESP32，才执行并关闭
`sealed delivery → physical power loss → Broker exact redelivery → same ACK` 门禁。工程 probe 的拒绝结果不能冒充
生产持久化通过。

Edge Hub cloud admission 同时冻结为：HTTPS DeviceKey session 与 MQTT 都必须是 TLS、两个 CA 均非空、WiFi 上线后
先完成 SNTP，再允许 HTTPS/MQTT；时间未可信时 management 不启动且不得回退明文。SNTP、MQTT management 和
Gateway secret storage 仍是内部 composition，不新增用户 Sketch 参数或第二套 MQTT client。

#### 10.3 D4a 实测结果（XIAO ESP32-S3）

2026-08-30 使用 ESP32 Arduino Core 3.3.1 与 XIAO ESP32-S3 `default_8MB` 完成：

- 内部 Edge Hub Advanced-entry compile/link 通过：Flash 1,079,751 B（32%），静态 RAM 50,896 B（15%）；
- Host 全量 31/31 通过，DeviceKey freeze v24 与 architecture/diff gate 通过；
- 真机只读 probe 识别默认 `nvs` 未受 Flash Encryption 保护，`protected_nvs=0`；
- `Esp32NvsAtomicBlobStore(encrypted=true)` 返回 `NotConfigured`，未打开、未写入、未降级普通 NVS；
- WiFi 上线后 `Esp32SntpClock` 只有在本次启动观察到 SNTP completed 后才输出可信 Unix time；
- probe 使用隔离 namespace 且未写入正式 identity/network/gateway-secret namespace；测试凭据不进入仓库。

因此 D4a 的退出条件是“完整 composition 可编译 + 默认板 fail-closed + 可信时间可达”，已经满足。D4b 不得在这块
默认开发板上通过伪 verifier 或自动烧 eFuse 完成；需要显式准备生产分区表、Secure Boot/Flash Encryption 生命周期
和可报废的专用 ESP32，再执行密封下发、断电、重复投递与 ACK 字节一致性测试。

#### 10.4 Edge Hub onboarding 与 northbound lane 冻结

D4 产品绑定补充以下边界：

- 默认 onboarding 复用现有 `Esp32WifiProvAdapter` 和 `DeviceKeyProvisioningEndpoint`；IDF-NimBLE 目标默认 BLE
  Security1，SoftAP 仅作编译期 bearer 回退；
- 同时保留手动 DeviceKey/SSID/password，供 Arduino 调试和无 App 场景使用；两者写入同一个受保护
  `DeviceKeyStore`/WiFi journal，不能维护两套 cloud lifecycle；
- provisioning adapter 完整 deinit 后才允许创建 BLE Central child data plane；首版不让 Hub 同时充当 App BLE
  proxy Peripheral；
- App 近场优先直连 child BLE；外网使用 account MQTT/Broker/Edge Hub route；未来 LAN 使用 WebSocket binary
  + BBP/2 + DirectSecureRecord，UDP/mDNS 只负责非权威发现；
- lane selector 属于 App/Broker，不进入 Arduino child API。Edge Hub 只执行 Broker 已授权的 exact child envelope，
  不自行判断“BLE 比 MQTT 更优”；
- mutation requestId 在首次发送时绑定 lane。结果不确定时不得跨 lane 重放；否则同一个按钮操作可能在 child
  执行两次。

实现顺序因此修正为：先提供 allocation-free、原子、受保护的窄 DeviceKey store 并让内部 Edge Hub 同时支持
manual/WiFiProv onboarding；随后实现传输无关的单-child Method 2 initiator、DirectSecure initiator 和 Central
coordinator，并以 Host/Arduino compile 门禁推进。ESP32 GATT Central 端口可以编译和做普通开发板功能测试，但只有
D4b 生产加密板通过后才能关闭 secret 持久化与完整 Edge Hub 发布门禁。LAN transport 只冻结合同，排到 G3，
不在本批增加 socket/UDP 依赖。

2026-08-30 本批实现证据：恢复的窄 `DeviceKeyStore` 固定持久记录 44 B、32 位对象 8 B，CRC/readback/
幂等 clear 的 Host 负测通过；内部 Edge Hub 默认选择 BLE WiFiProv，手动入口仍收敛到相同 DeviceKey/WiFi
storage 和 cloud lifecycle。XIAO ESP32-S3 Core 3.3.1 编译为 `1,341,027 B Flash / 58,848 B globals`；
Host ASan `32/32`、200,000 次 deterministic fuzz、DeviceKey freeze v25、architecture/diff gate 通过。
该证据只关闭 onboarding composition/software gate，不替代 D4b 加密板、真实 App WiFiProv 或 G2A BLE Central。

#### 10.5 D4b 只读生产预检（软件完成，物理门禁未完成）

为避免用笼统 `NotConfigured` 误判生产板，也避免测试工具暗中改变安全状态，ESP32 port 已提供只读
`Esp32ProtectedNvsStatus`：

| 状态 | 含义 | 是否允许继续 D4b |
|---|---|---:|
| `FlashEncryptionDisabled` | SoC Flash Encryption 未启用 | 否 |
| `PartitionMissing` | exact `BLINKER_EDGE_HUB_NVS_PARTITION` 不存在或不是 NVS subtype | 否 |
| `PartitionNotEncrypted` | 分区存在但未标记 encrypted | 否 |
| `Ready` | Flash Encryption 已启用且 exact NVS partition 标记 encrypted | 是，仅允许进入后续真实写入门禁 |

`tests/internal/arduino/Esp32EdgeHubSecurityPreflight` 只调用该 attestation 并输出
`partition/status/ready/readonly`；不打开 NVS、不写应用 secret、不烧 eFuse。Host 覆盖四种状态，Arduino
matrix 使用真实 ESP32 Core 3.3.1 编译该 probe。普通 XIAO `default_8MB` 即使输出非 Ready 也只是正确拒绝，
不能修改 verifier 或改用 PlainFlash 使其“通过”。

2026-08-30 软件证据：Host ASan `33/33`、200,000 次 deterministic fuzz、architecture/diff gate 通过；
XIAO ESP32-S3 probe clean compile 为 `307,187 B Flash / 20,344 B globals`。执行时未发现已连接串口，
因此没有上传固件；D4b 仍等待专用生产加密板的 `ready=1`、真实 WiFiProv、sealed delivery、物理断电、
Broker exact redelivery 与 same ACK。

#### 10.6 G2A-2a 主动端安全核心（软件完成）

设备端已补齐 allocation-free、单会话 `ControllerHmacSha256Initiator`，不复制 App 的 TypeScript 协议，也不把
BLE/GATT 依赖放进 security：

- 输入一个已验证 `ControllerCredential`，生成 exact 42 B Init，验证 53 B Device Challenge 后生成 exact 33 B
  proof；最终裸 `AuthResult(Authorized)` 到达前绝不启用安全数据面；
- 复用现有 `computeControllerAuthProof()` 和 `DirectSecureSession(Initiator)`；没有第二套 HMAC/KDF/record 实现；
- 错序、wrong method/status、全零 nonce、权限变化、wrong device proof、Rejected/Locked 均清零 transcript、secret
  和 DirectSecure session；deadline/retry/断连仍由后续 `ChildSessionCoordinator` 负责，避免安全核同时承担调度；
- `makeGatewayControllerCredential()` 只从已 durable 的 `GatewaySecretRecord` 生成 PlatformAccess 凭据，并要求
  trusted Unix time 严格早于 expiry；candidate、无可信时间和过期记录不能进入 Method 2；
- Host 将主动端与现有设备响应端直接互操作，校验既有 golden app proof、两端 DirectSecure binding、错误 proof、
  错序及全零随机数。MSVC ASan 全量 `34/34` 与 200,000 次 deterministic fuzz 已通过；ESP32-S3 内部
  Edge Hub clean compile 为 `1,341,331 B Flash / 58,848 B globals`，未上传或改变任何硬件安全状态。

该批只关闭 G2A-2 的安全状态机子门禁；尚未实现 Hello negotiation、独立 BLE Central port、GATT record link、
Manifest/State/Command/ACK bridge，也不改变 `reachableViaGateway`。

#### 10.7 G2A-2b 单子设备会话协调器（软件完成）

设备端已新增 transport-independent、固定内存的 `ChildSessionCoordinator`，将 G2A-2a 的主动端安全核接入
完整 BBP/2 帧级握手，但仍不依赖 NimBLE、ArduinoBLE 或 Wio RPC：

- 发送 `Hello(App role, seq=1)`，要求 child 返回 Device role、BBP/2 v2、Method 2、Manifest、EndpointIds、
  Authentication、ReliableDelivery 和 StateRevision；Telemetry 只作为可选协商能力；
- 固定执行 `Hello -> Auth Init(seq=2) -> Challenge -> Proof(seq=3) -> Authorized`，只有最终 Authorized
  通过后才暴露 DirectSecure session；Gateway 的授权身份来自 exact credential/topology，不新增 wire-level
  Gateway role；
- 保存 negotiated feature/max-frame/reliable-window 与 child manifest revision/fingerprint，所有帧均受本地或
  已协商 max-frame 限制；错序、错误 flags、缺少 Manifest、能力不满足、超长帧和任意认证失败都会清零
  transcript、secret 与 secure session；
- 认证使用一次总计 15 s deadline，而不是每个阶段重新续时；u32 millis wrap-safe，32 次 timeout/recovery
  循环均未保留旧安全状态；对象不分配堆内存，64-bit Host 上保持不超过 384 B；
- Host 使用生产 BBP/2 codec、现有设备端 `ControllerHmacSha256Authorizer` 与 DirectSecure 双端 binding 做
  帧级互操作，不另写 mock 协议实现。

2026-08-30 门禁证据：MSVC ASan 全量 `35/35`、200,000 次 deterministic fuzz、architecture/diff gate 通过；
XIAO ESP32-S3 Core 3.3.1 Edge Hub clean compile 为 `1,341,703 B Flash / 58,848 B globals`，相对 G2A-2a
仅增加 `372 B Flash / 0 B globals`。该数字不包含尚未实现的 BLE Central/GATT workspace，不得用来估算最终
Gateway data plane 资源。

该批关闭“单 child 传输无关会话”的软件门禁。下一批必须先建立独立 `IBleCentralPort` 和只搬运完整
BBP/2 record 的 `GattDirectChildLink`，再把它们接到协调器；不得把 BLE SDK、扫描器、多 child scheduler、
route/presence 或 Manifest cache 塞进协调器。ESP32 Edge Hub product 在 Central port 出现前仍不声称拥有
BLE child data plane，D4b 生产加密板也继续作为发布阻断门禁。

#### 10.8 G2A-2c 独立 Central 合同与单子设备 GATT record link（软件完成）

本批先关闭 BLE SDK 之上的最小传输层，不把平台扫描/GATT 细节塞入会话协调器：

- `IBleCentralPort` 是独立 Central 合同，不复用 Peripheral `IBleLink`，也不向上暴露 MAC、Arduino/IDF handle
  或平台对象；advertisement 只提供 Blinker mode service-data 与 RSSI，目标选择由调用方注入的授权 matcher 完成；
- port 只有在连接成功、目标 service/characteristic 已发现且 notify 已启用后才报告 `Ready`；`start/connect/send`
  不允许同步回调，所有平台事件由 `poll()` 有界交付，避免 Arduino loop 重入；
- `GattDirectChildLink` 内部以很窄的 `IBleLink` adapter 复用既有 `BleRecordTransport`。这只是复用唯一的 4 B
  fragment/reassembly 实现，不是把平台 Peripheral port 改造成双向大接口；
- `Bbp2BleRecordFormat` 成为 Peripheral/Central 共用的完整 record 判定器，只接受裸 BBP/2 或
  `DirectSecureRecord`，长度、magic、version、flags 和 trailing bytes 都 exact 校验；
- 首版固定一个连接、一个发送队列和 caller-owned RX/TX/packet workspace；connect 使用一次总 deadline，fragment
  stall、协议错误或底层失败只上报一个 fault 并物理退役连接，普通主动/远端断开不伪报协议故障；
- 当前 port state 是权威事实，即使平台漏送 disconnect callback 也会清除会话；自发断开产生的迟到回调和已取消
  generation 的 stale connect callback 均不能污染下一次连接；32 次连接/断开恢复不保留状态；
- 20 B ATT packet 下 45 B BBP/2 frame exact 分成 3 片并正确重组；64-bit Host 上 link 对象保持不超过 640 B，
  最大 512 B frame 的两个 record workspace 与 20 B packet scratch 由 product 显式提供，不隐藏 heap。

2026-08-30 软件门禁：MSVC ASan 全量 `36/36`、200,000 次 deterministic fuzz、architecture 和 diff gate
通过；XIAO ESP32-S3 Core 3.3.1 Edge Hub clean compile 为 `1,342,519 B Flash / 58,848 B globals`，相对
G2A-2b 为 `+816 B / +0 B`。该结果只证明平台无关合同、分片和恢复语义；尚未包含
`Esp32NimBleCentralPort`、真实扫描/GATT/CCCD、`ChildSessionCoordinator` composition、
Manifest/State/Command/ACK bridge、多 child 或硬件长稳。当前资源增量也不能替代后续 Central SDK workspace
和真实连接峰值 RAM 测量。

#### 10.9 G2A-2d access material 合同修正（软件完成）

实现 G2A-2c 时确认现有 `GatewaySecretEnvelopeV1`/308 B `GatewaySecretStore` 只包含 32 B Gateway Method 2
secret，没有 Direct v3 `PresenceKey`。因此 Hub 即使有认证凭据，也无法在不持久依赖 MAC/广播名、且不对附近
所有 Blinker 设备盲试 Method 2 的前提下定位 exact child。两种绕过都不接受：Gateway secret 不能充当
PresenceKey，扫描到名称/地址也不能成为 durable identity。

真实 ESP32 Central E2E 前必须完成以下跨端修正：

1. 新增 `GatewayAccessEnvelopeV2`，不静默改变 V1 语义。V2 plaintext 固定为
   `gatewayControllerSecret[32] || presenceKey[16]`；`presenceKeyVersion` 与既有 child/epoch/controller/
   credential metadata 一同进入 exact AAD；
2. ACK 改为 `operationId + accessMaterialDigest + storageRevision`，digest 是 exact 48 B plaintext 的
   SHA-256，不继续用含义错误的 `secretDigest` 名称；Hub 原子保存 Method 2 credential 与 locator 后才 ACK；
3. PresenceKey 已由 Service 以 KMS envelope 托管。attach 时 App 只提交 child、Hub 和授权意图；Service 在
   serializable transaction 内部解封 exact child/current/confirmed PresenceKey，校验 accessEpoch/version 后立即
   密封给 exact Hub，并在 `finally` 清零可变 Buffer。App 不提交 raw PresenceKey，Broker、普通 DB、日志、
   analytics 和其他账号也不能得到它；
4. Gateway credential version 与 PresenceKey version 独立单调。初次 attach 先实现单 active locator；轮换必须是
   `stage new locator at Hub -> child durable install -> Hub proves new locator + Method 2 -> Service activate` 的两阶段事务。
   在 activate 前 Hub 只保留有界的 active+pending 两个 locator，失败/取消清 pending，不能覆盖旧 locator 后留下
   无法重新发现的 child；
5. `GattDirectChildLink` 继续只接收 matcher，不读取 secret store。新的 access-material store/composition 负责从
   exact PresenceKey/version 构造 Direct v3 matcher，再把匹配后的连接交给 GATT link 和会话协调器。

实现结果：测试阶段 V1 代码路径已直接移除，没有兼容分支。Service/device 现在共享单一
`GatewayAccessEnvelopeV2`、V2 KDF/AAD 与 48 B plaintext；设备使用 352 B protected atomic
`GatewayAccessStore`，保存一个 active 与一个 pending locator。投递 ACK 的 revision 与本地 storage revision
分离，因此 locator activate/discard 后 exact MQTT redelivery 仍返回原 ACK 且不重复写入。Service/device exact
CBOR、AAD、KDF、ACK golden 已通过；Host MSVC ASan `36/36`，Gateway access Service、protocol `13/13`、Broker
Device V2 与 TypeScript typecheck 均通过。XIAO ESP32-S3 internal Edge Hub 单库 clean compile 为
`1,343,379 B Flash / 58,872 B globals`，相对 G2A-2c 为 `+860 B / +24 B`；普通产品 API 与依赖清单未改变。

该批只关闭 access material 的 wire、密封、持久化和单槽轮换原语。Service 的 child-install/proof/activate
事务编排，以及 `Esp32NimBleCentralPort`、真实 Central GATT 和 route bridge 仍属于下一批；在这些闭环前不能把
pending locator 显示为 active，也不能宣称网关数据面完成。

#### 10.10 G2A-2e ESP32 Central 与 exact 单 child composition（软件完成，真机待完成）

本批把 G2A-2a～2d 的平台无关能力接入内部 ESP32 Edge Hub，但没有增加公开 `BlinkerEdgeHub` facade：

- 新增独立 `Esp32NimBleCentralPort`，直接使用 ESP-IDF NimBLE Central/Observer C API。应用侧只保留固定
  4 项扫描队列、4 项 RX packet 队列和 ATT20 packet；native address、connection/GATT handle 始终留在 Port
  内部，matcher 只看到 exact Blinker 128-bit service-data 与 RSSI；
- Port 在 Arduino `poll()` 中有界调用 matcher 和上层 callback；Host task 只复制固定大小事件。扫描、连接、
  service/RX/TX characteristic discovery、CCCD subscribe、取消和断开按 attempt generation 收口；底层在
  record session 建立前失败时，`GattDirectChildLink` 立即回报正确 generation，不再等待外层 deadline 后报告
  `attemptId=0`；
- `GatewayPresenceMatcher` 只从 protected `GatewayAccessStore` 复制 exact child instance/accessEpoch 及
  active+pending Presence locator。轮换期间先尝试 pending、再回退 active；MAC、广播名和盲试附近设备均不参与
  durable identity；
- `BasicGatewayChildSession` 组合 exact discovery、Hello、Method 2 initiator 与 DirectSecureRecord。它只在可信
  Unix time 下装载 credential，并在 Connecting/Authenticating/Secure、发送 frame 和激活 pending locator 时
  持续执行 expiry gate；过期、时间失信、错误 sequence/tag/frame 或不确定 TX enqueue 都退役当前 session；
- ESP32 Peripheral 与 Central 共用唯一 NimBLE runtime owner。WiFiProv 仍由 SDK 管理，但 product 必须在其
  完整 deinit 后才能启动 Central；两种 Blinker Port 不能各自误建一个 Host；
- Edge Hub 固定持有一个 child 的 RX/TX/reassembly/handshake/plaintext/secure-record workspace。Central 不在
  product `start()` 时抢先启动：只有 WiFi、可信时间、HTTPS session、MQTTS 和 management subscription 全部
  Online 后才启动；任一 cloud admission 丢失会先清 child session 并关闭 Central，为 TLS 恢复释放 runtime
  heap。Online 也不自动扫描，后续必须由 authenticated management proof command 显式调用 `connect()`。

Host MSVC ASan 已增加 composition seam 门禁：protected access record、错误/exact locator、Hello 起始、15 s
认证 deadline、credential 到期前/期间拒绝及断开恢复；全量为 `38/38`，另过 200,000 次 deterministic fuzz、
architecture 和 diff gate。XIAO ESP32-S3 Core 3.3.1 internal Edge Hub clean compile 为
`1,365,595 B Flash / 62,848 B globals`，相对 G2A-2d 为 `+22,216 B / +3,976 B`。该静态增量包含
Central/session 对象和固定 workspace，但不包含 NimBLE host、WiFi、HTTPS/MQTTS 同时运行的动态 heap 峰值；
因此仍不能替代真实 simultaneous heap/stack watermark、真实 GATT/CCCD、断电或长稳证据。

本批刻意不实现三项内容：Service proof 指令、active topology route admission、Manifest/State/Command/ACK
bridge。直接把 `ChildRouteEnvelope` 接到当前 session 会缺失可信的 topologyVersion、operation/nonce 和
proof/activate 时序，并可能让 durable access ACK 被误当作 active topology。因此下一顺序必须先冻结
G2A-2f proof control contract，再进入 G2A-2g 最小 route bridge；不能用内部 `childSession()` accessor
绕过控制面。

#### 10.11 G2A-2f proof 控制面（核心软件与 G2A-1b 真机闭环完成）

- device/service 共享上述 command/result exact CBOR 与 golden；device 新增固定内存
  `GatewayProofCoordinator`，只由 authenticated management command 驱动 `GatewayChildSession`；
- coordinator 在每次尝试前复核 operation、child identity、accessEpoch、credential/Presence version、material
  digest、durable delivery revision 和双重 expiry；transient BLE 失败有界重试，cloud admission 丢失时清 task
  并退役 child；进行中的 proof 禁止换 command，终态 result 可 exact replay，而新的 authenticated
  topologyVersion/nonce 到达时会退役旧 child、释放旧 replay cache 并接管新 proof，Expired retry 不要求重连云端；
- `GatewayManagementClient` 在原 up/down topic 内按 exact message shape 分派 access delivery 与 proof，不扩大
  587 B 收包上限，不新增 MQTT client；结果发布失败由 Broker exact command 重投和 Hub RAM replay 收敛；
- Broker 新增独立 durable proof ledger、QoS1/retain=false exact 重投、restart/duplicate/expiry/result conflict
  校验；Service repository 新增 child receipt CAS、proof nonce 持久化、Secure activate、Rejected rollback 和
  Expired retry CAS；
- 同批修正了原实现把短 `deliveryExpiresAt` 当作长期 credential expiry 的错误。Service 默认长期 Gateway
  credential policy 与不超过 15 分钟的 delivery ledger 分离，Broker 只要求 delivery deadline 不晚于 envelope
  credential expiry；proof deadline 在 Service 建账时规范化为 wire 可表达的整 Unix 秒，repository 同时拒绝
  credential/delivery/proof 时间倒置。Broker expiry worker 与 Hub `Expired` result 同时到达时只允许补记同一
  exact terminal result，不能借竞态改成迟到的 `Secure`/`Rejected`。

最终软件门禁为 Host `39/39`（含 deterministic fuzz smoke）、独立 200,000 次 fuzz、architecture、protocol
`14/14`、Broker Device V2、Gateway Service（含独立 PostgreSQL schema 下的 017→020 migration/CAS）与跨语言
Edge Gateway golden 全部通过。XIAO ESP32-S3 / ESP32
Core 3.3.1 internal Edge Hub compile 为 `1,370,979 B Flash / 63,168 B globals`，相对 G2A-2e 为
`+5,380 B / +320 B`。该数据只表示静态链接结果，不替代真实 WiFi+MQTTS+NimBLE Central 动态 heap/stack
watermark。

G2A-1a 已在下一小节完成 owner HTTP/runtime wiring；G2A-1b 后续已按 10.13 完成双 XIAO 真机闭环，并以此为
前置进入 10.14 的 G2A-2g route bridge。这里记录的是实施顺序，不再代表当前未实现。

#### 10.12 G2A-1a owner attach 控制面（完成）

production Gateway runtime 已实例化 `EdgeGatewayRepository + EdgeGatewayAccessService + EdgeGatewayHttpHandler`，
公开账户 API 冻结为：

```text
POST /api/v2/edge-gateway/attachments
GET  /api/v2/edge-gateway/attachments/{operationId}
POST /api/v2/edge-gateway/attachments/{operationId}/resume
POST /api/v2/edge-gateway/attachments/{operationId}/receipt
POST /api/v2/edge-gateway/attachments/{operationId}/cancel
```

create 的 `Idempotency-Key` 与 path `operationId` 均为 canonical 16 B Base64URL。create body 只允许 exact Hub logical
id、child logical id 和 child physical instance；`accessEpoch` 从已确认 DirectAccess 读取，physical instance 必须解析回
同一 owner/tenant/child，Gateway `controllerId` 由 environment/tenant/child 确定性派生，credential version 由 durable
child receipt 历史分配。App 不得提交这些权威字段。

create 只返回脱敏 topology 并 stage sealed Hub delivery；raw secret 只在 topology 已进入
`PendingChildInstall` 后，由 `resume` 携带 fresh 16 B `controlNonce` 时短暂返回。签名 key id/algorithm 属于 Service
受信配置，禁止由 App 选择；Service 优先使用当前官方 P-256 Controller Grant key，并把实际 key id/algorithm 写入
signed Grant，child 再按 trust ring 独立验证。Service 从 KMS 临时解封，同步复核 exact envelope AAD binding、当前
PresenceKey version 与完整 48 B material digest，然后使用现有 Controller Control v2 exact contract 签
当前新 topology 签 `Install` Grant；只有确知 live slot 存在的原地更新才签 `Rotate`。grantId 对同一 operation/controller
稳定，因此 App kill 后可用 fresh nonce 获取新签名而 durable receipt identity 不变。P-256 和 Ed25519 共用官方 server
trust ring；raw secret、KMS plaintext、grant 不进入 audit metadata 或普通数据库。

receipt endpoint strict-decode canonical ControllerMutationReceipt，逐字段匹配 operation/grant/instance/accessEpoch/
controller/version/permissions/secret digest，并由 Service 以 KMS Gateway secret 再验 HMAC；同时强制 App 提交
`method2Confirmed=true`。通过后先 CAS `PendingChildInstall -> PendingGatewayProof`，再 stage exact Hub proof；最终只有
Hub `Secure` result 才能 Active。receipt digest 是同一 topology 的 exact 幂等边界，改变 bytes 会冲突。

取消采用保守边界：当前 create 会立即 stage Broker，因此所有已持久化 pending cancel 都进入
`RollbackRequired`，不会谎称 Hub/child secret 已擦除。migration 020 将历史 controller/version 唯一约束限定在 live
topology；只有 durable child receipt 才消耗 child credential version，单纯 delivery expiry 不会错误推进版本。
历史 version 大于 1 的新 topology 仍签 `Install(expectedVersion=0)`，不得错误生成 `Rotate`。真实 PostgreSQL
017→020、HTTP strict/redaction、P-256/Ed25519 signing、HMAC 篡改、exact replay、完整
Gateway Device V2 suite 与 production wiring 已通过。

#### 10.13 G2A-1b 双 XIAO 控制面真机闭环（完成；业务数据面未完成）

2026-08-31 使用两块 XIAO ESP32-S3 完成真实 App/Service/Broker/Hub/child 控制面闭环：

1. App 创建 exact topology，Service 组合 KMS 中的 Gateway secret 与已确认 PresenceKey，并密封给 exact Hub；
2. Broker durable ledger 投递 envelope，Hub 解密、原子保存并 ACK `storageRevision=1`；
3. App 以 child Admin DirectSecure 会话 exact relay signed Grant/secret，校验 durable receipt；
4. App 关闭 Admin session，再以新 Gateway credential 独立完成 Method 2 + DirectSecure，之后才提交 receipt；
5. Service 进入 `PendingGatewayProof`，Hub 扫描 exact Presence locator、连接 child 并返回 `Secure` proof；
6. App resume 对账后 topology 进入 `Active` 并删除 checkpoint。

这里的“完成”严格限定为 attach/proof 控制面。它没有验证 child Manifest/State 经 Hub 上行、App Command 经
Hub 下行及 child ACK/Error 返回。Broker 中已有的 child Manifest/State 可能来自此前手机 BLE 直连，只是历史
缓存，不是 Gateway 数据面证据；G2A-2g 必须以新的 route session 重新完成协议协商并刷新权威状态。

最终权威状态为 PostgreSQL `Active/version=4/storageRevision=1/childInstalled=true/gatewayProven=true`；Broker
delivery 为 `acknowledged/revision=1`，proof 为 `secure/presenceKeyVersion=2`。另一个在调试期间过期的 operation
通过同一幂等入口收敛为 `Expired`，证明恢复路径不会因 wall clock 已超过 deadline 而失去 durable 对账能力。

联调发现并立即关闭了四类会继续放大的实现问题：

- 普通 DeviceKey WiFi 默认 packet capacity 不变；只有 Edge Hub 将 PubSubClient packet capacity 编译期提升到
  `maximumManagementTopic + 587 B envelope + MQTT overhead = 724 B`，避免为 UNO R4/普通 WiFi 常驻付费；
- internal Advanced Client 显式把 runtime `maximumFrameSize/maximumReassemblySize` 绑定到
  `BLINKER_DEVICE_FRAME_SIZE`，不再让 256 B storage 与默认 400 B runtime 合同冲突；
- PostgreSQL migration 021 条件式归一化测试期旧列名、PresenceKey version 与 587 B envelope constraint；Broker
  SQLite 启动时在事务中重建旧 ledger、复制所有行并把 `secret_digest` 归一为 `access_material_digest`；
- Broker exact stage 先读取 durable operation，再校验不可变字段；相同 operation 即使已过期也可返回原 terminal
  fact，只有新 operation 才要求 future expiry。

安全边界没有因真机调试而放宽：本次 Hub 使用仅位于 `tests/internal` 的 disposable Plain-NVS adapter，并只对该
测试实例谎报 storage protection，从而验证跨语言 HKDF/AAD/AES-GCM、MQTTS management、真实 NimBLE Central 和
完整状态机。生产 `Esp32EdgeHubPlatform` alias 仍固定 `BasicEsp32EdgeHubPlatform<true>`，默认板仍 fail-closed；
D4b 必须在专用 Flash-Encryption + encrypted-NVS 板上完成断电/redelivery/same-ACK，才能公开 Edge Hub product。

#### 10.14 G2A-2g 最小单 child route bridge（软件与 Hub/child/cloud/App 真机完成）

2026-08-31 已完成 Active topology 下最小业务数据面，且不引入多 child、Mesh 或 Gateway facade：

- Service 提供只返回非 secret Active route 的权威查询；Broker 在 Hub route subscribe 和 topology Active 后均可
  推送 canonical `GatewayRouteAdmissionV1`，每次下行命令前再次查询 Active route，detach/expiry 不依赖旧缓存；
- Hub 使用两槽 `GatewayCloudMux` 将 management 与 route 复用同一 MQTTS session。普通 WiFi 产品不包含这些槽位；
  route 只允许固定 512 B inner BBP/2、最大 696 B envelope，MQTT packet capacity 只在 Edge Hub 提升到 833 B；
- 本地 child Hello/Method 2 不冒充云会话。Hub 从已认证协商结果生成一次 canonical Cloud Hello；Broker 为
  `gateway-child` 虚拟会话无条件重新请求 Manifest，因此手机 Direct 历史缓存不能产生假通过；
- Manifest/State/Command/ACK/Error 复用既有 BBP/2 lane、Manifest cache、State store 和 durable command ledger。
  source identity 只取已认证 Hub ingress 与 Active topology，不接受 envelope 自报 source；
- bridge 将 Hello/Manifest/State/Command 视为请求，将 ACK/Error 视为终止帧；响应必须按 sequence 精确匹配。
  child 在命令期间主动上报 Patch 时使用独立 correlation，不能消费 command slot；Broker 也只允许相同 correlation
  的 ACK/Error 完成 ledger，其他合法 Patch 独立处理。

双 XIAO ESP32-S3 现场结果：Hub 通过本机 HTTPS+MQTTS 测试入口登录 shard 1，Broker 在本次启动后刷新 child
Manifest（`power/buttonPresses/button`）与 State revision 1，并发布 cloud Presence；随后服务器下发 `power=true`
得到 sequence 5 ACK，durable ledger 为 `acknowledged/attempt_count=2/http=200`，child Patch 将 State 推进到
revision 2、`power=true`。首次命令还真实暴露并修复了 Broker 错误拒绝独立 Patch correlation 的问题。

2026-09-01 的生命周期复测又暴露并关闭两项不能依靠重试掩盖的根因：

- `GatewayManagementClient::configure()` 过去会执行完整 `reset()`，SessionCredential 刷新因而错误清除 terminal
  proof 并停止已经安全建立的 child。现在 credential/session 轮换只清理 session-local cloud binding；只有产品停止、
  网络信任丢失或显式 full reset 才退役 proof 与 child；
- MQTT 可在一个外层 `poll()` 内完成断线、换凭据和重连，产品状态机不一定观察到中间态。因此 route session
  边界以 `GatewayRouteClient::subscriptionGeneration()` 为权威；generation 改变时 bridge 保留 child secure
  session、清理旧 cloud correlation 并重发一次 Cloud Hello，同一 generation 的重复 admission 保持幂等；
- Broker 必须先写入 exact Hub/child route admission context，再发布 `RouteAdmission`。否则已经 secure 的 Hub 可在
  publish 调用栈内同步返回 Cloud Hello，而 Broker 此时尚无授权上下文，形成真实可复现的重入竞态。

Broker 回归测试会在 `RouteAdmission` 的 publish 调用栈内立即回送 child Cloud Hello；只有 admission context
已经可见才会通过。该测试与完整 Broker Device V2-only 套件已在发布镜像的 Node 20 运行时通过，避免把现场修复
退化为依赖异步调度时序的偶然成功。

修正后在不重启 MCU、不重新 proof 的前提下连续观察超过两轮 60 s credential refresh，child 始终保持 Secure，
Presence 的 `lastSeen` 每轮推进；Broker 重启后 topology 与虚拟 child session 自动恢复。随后真实服务器 Command
得到 sequence 5 ACK，State revision 推进到 3、`power=false`。这先关闭了 G2A-2g 的设备/Hub/Cloud 三端门禁；
下文真实 Android 复测随后关闭第四端。

本机开发 Compose 的内部 HTTP/MQTT upstream 是明文，端口号本身不代表 TLS。为避免给生产 Edge Hub 增加明文
降级，本轮使用独立 development-only TLS 终止器；当前忽略的本地配置把 Hub HTTPS/MQTTS 映射到 TLS 7000/8883，
内部 upstream 为 17000/18883，App account MQTT WebSocket 为 8884。临时证书、私钥、WiFi 与 DeviceKey 均不进入
仓库。该门禁证明真实 TLS wire 和业务数据面，但 disposable Plain-NVS 仍不替代 D4b encrypted-NVS 门禁。

2026-09-01 手机接回 ADB 后，真实 Android Debug APK 从设备页经 account MQTT 对当前 child 下发 Button Action；
Hub/child 返回 ACK/Patch，App 与 Broker 同步从 State revision 2 / `buttonPresses=1` 推进到 revision 3 /
`buttonPresses=2`，关闭此前仅由内部 HTTP command 覆盖的四端缺口。

随后保持设备页打开，以不擦写 Flash 的硬复位重启 child。新运行会话的完整 StatePage 从 revision 1 /
`buttonPresses=0` 开始，App 无需重开页面即原子替换旧 revision 3；再次执行 Action 后 App 与 Broker 同步推进到
revision 2 / `buttonPresses=1`。现场同时验证并冻结两项边界：

- Hub 是 cloud session 与 child peer session 的序号代理。cloud sequence 不能直接透传；Hub 为 child 单独分配单调
  sequence，exact retry 复用映射，response/ACK 再映射回 cloud sequence。这样 SessionCredential 刷新后 cloud
  sequence 重新开始也不会与仍存活的 child session 冲突；
- revision 不是跨设备重启的永久计数器。Patch 在同一运行会话内保持单调；Broker 接受的完整 StatePage 是权威
  状态代际边界，可从 0 或较低 revision 开始。Broker 以无 requestId 的分页 StatePage 通知已授权 App，App 在
  cursor 0 开启 staging，完整页提交后才替换旧快照，后续 Patch 继续按新 revision 校验。

G2A-2g 至此完成。内部 Broker HTTP command 继续只作为诊断入口，不能用于替代上述真实 App 证据。

#### 10.15 G2A-3a detach 云端闸门（软件与本地服务栈完成；设备删除待 G2A-3b）

首批撤销只实现能独立成立的安全边界，不把设备尚未确认的事实提前写成完成：

- `POST /api/v2/edge-gateway/attachments/{operationId}/detach` 只接受当前 owner；repository 以 CAS 将 Active 或
  RollbackRequired 推进到 Revoking 并增加 topologyVersion。重复请求返回同一 Revoking/Detached 事实；其他状态
  fail-closed；
- repository 的 Active route 查询只接受 Active，因此 Revoking 落库是云端授权的线性化点。即使 Broker 主动通知
  失败，每次 child 上行和 Command 下行仍会重新查询权威 topology 并拒绝转发；
- Service 同时调用 shard 内部 revocation endpoint。Broker 按 tenant + exact Edge Hub 找到当前连接，立即退役
  route context、虚拟 child protocol session、Presence token、在途 Command 和 frame/correlation 缓存；上下文不存在
  返回 `revoked=false`，作为安全幂等结果而不是错误；
- PostgreSQL CAS、Gateway HTTP/Service/Broker client、Broker route reentrancy/revocation 与 Linux Node 20 完整
  Device V2 套件均已通过；新镜像重启后现场 Active Hub/child 仍可自动恢复，新增入口未改变正常数据面。

G2A-3b 必须复用现有 `ControllerMutationOperation::Revoke`，把 signed revoke grant 经 authenticated management/child
路径送达，校验 exact revoke receipt 后才执行 Revoking -> Detached。该批还需定义 Hub 本地
`GatewayAccessStore` 清除的 durable ACK/重放顺序，并完成掉电、重复 revoke、child 离线、Hub 离线和 receipt 丢失门禁。
在此之前，UI 必须显示“正在移除网关/等待设备确认”，不能显示“已移除”。

#### 10.16 G2A-3b 三阶段设备撤销（主路径四端完成；receipt-loss recovery 软件完成、真机注入待完成）

- `Prepare` 让 Hub 经既有 Method 2/DirectSecure 私有 side channel 向 child 取得 fresh control nonce；管理帧不进入
  普通 route，也复用 route bridge 的唯一 child sequence allocator；
- Service 用该 nonce 签发 `ControllerMutationOperation::Revoke` Grant。Broker 以
  `(operationId, topologyVersion, phase)` 持久化 1～280 B exact command，QoS 1 重投并接收至多 208 B result；
- `Apply` 由 child 原子删除 Gateway credential 并返回 canonical receipt。Hub 验证 binding 后先以同一 352 B protected
  store 把 active access 替换为不含 secret 的 receipt tombstone，再向 Broker 返回 exact receipt/digest；Hub 重启可从
  tombstone 重放同一 result；
- Service 逐字段验证 stored signed Grant、child instance、accessEpoch、controller、credentialVersion 和 receipt
  SHA-256，只有这一步完成才以 CAS 进入 `Detached`；随后 `Finalize` 清除 Hub tombstone。Finalize 的 Broker retry
  deadline 可重开，但设备命令 identity/digest 不变；
- 跨 child Flash 与 Hub Flash 不存在原子提交。若 child 已删除 credential、Hub 尚未 commit tombstone 时掉电，Cloud
  会安全停在 `Revoking`，不会恢复 route 或伪报 `Detached`，但旧 Gateway Method 2 已无法取回 receipt。正式关闭该
  窗口需要 nearby owner/Admin 建立新安全会话，取得 fresh nonce，对“credential 已不存在”执行幂等 Revoke 并把新
  receipt 交回同一 operation；不能靠超时自动改写为 Detached。

pre-tombstone recovery 已冻结并实现为既有状态机的严格分支，不新增 Recovery topology state：

- `POST .../{operationId}/recover` 只接受当前 owner 的 16 B fresh Admin control nonce；repository 以 CAS 增加
  `topologyVersion`，原子替换 recovery nonce/Grant/deadline。相同 nonce/Grant 是 exact replay，不同 nonce 显式
  supersede 旧 Apply；
- App 只在既有 owner/Admin DirectSecure 会话内 relay 新的 signed Revoke Grant。Revoke body 的 controller secret
  必须为空；receipt 必须为 permissions=0、zero secretDigest、proofKind=None，且 exact 绑定 child instance、epoch、
  controller 与 credentialVersion；
- Service 将 canonical receipt 编码为正常 `Apply/Revoked` result，通过 internal-only Broker recovery endpoint
  提交。Broker 对当前 frozen Apply command 重新验证全部 receipt binding/digest，停止更低 topologyVersion 的 pending
  ledger；客户端不能直接写 Broker ledger；
- Service 复用正常 `verifyRevocationReceipt -> Detached -> Finalize`。Hub 收到更高 topologyVersion 的 Finalize 时可
  抢占卡住的旧 Apply；若本地是 tombstone 则按原路径清除，若仍是 active access，则只有 operation、child instance、
  accessEpoch、controller、credentialVersion、deliveryRevision 全部 exact match 才原子清除。任一不匹配均保留 access
  并 fail-closed；空存储仍为幂等成功；
- Host ASan 41/41、200,000 fuzz、Broker、Protocol 15/15、Gateway production-KEK + real PostgreSQL 25/25、App
  52 files/260 tests 已通过。尚需在双板现场精确注入“child delete 后、Hub tombstone commit 前掉电”，再执行 owner/Admin
  recovery、Hub 复位和 ledger/DB 对账；完成前仍不把该硬件 crash gate 标记为关闭。

2026-09-01 的真实四端门禁补充冻结以下实现边界：

- management plane 不依赖 Active route admission；Revoking 后 Hub 可以按 durable AccessRecord 建立临时 child
  secure session，但普通 child frame 不向 Cloud route 转发；有效的非 control frame 本地丢弃，畸形 frame 才断连；
- ControllerControl 请求使用 `FlagNone`。Method 2 的 `Observe|Control` 只允许中继服务器签名 Grant；fresh nonce、
  exact binding 与签名才是 mutation authority。Gateway 不取得 `ManageControllers`，PresenceKey mutation 仍要求它；
- BLE link 的同步 disconnect callback 可以在 `disconnect()` 调用栈内把 session 推进到 Idle，因此状态必须先写
  Disconnecting，不能在 callback 返回后再覆盖成旧状态；
- Service 后台 worker 每 5 s（可配置）扫描至多 20 条，最多并发 4 条，且同进程轮次不重入。App 的 `resume` 只用于
  查询/显式恢复 UX，不再驱动安全正确性。每轮先幂等退役 route；若 Service topology CAS 已提交但对应 Broker
  phase ledger 尚未落盘或已丢失，则从权威 topology 重建相同 Prepare/Apply/Finalize phase。多实例竞态继续由
  exact Broker ledger 与 repository CAS 收敛。
- 全新 topology 真机重接暴露并修正两个跨端假设：Service 不能用 `credentialVersion > 1` 推断 Rotate，因为前一
  topology 已 durable revoke、槽当前不存在；App Method 2 AuthInit 也不能硬编码 version 1，必须携带当前凭据版本。
  修正后 version 2 Install、独立 Gateway Method 2 与 Hub proof 成功进入 Active。
- App 对该 topology 只发送一次 owner detach 并立即强制停止。Service worker 在无 App HTTP 调用下将 topology
  version 5 推进至 Detached/finalized；Broker Prepare/Apply/Finalize 分别为 `nonce_ready/revoked/finalized`，Hub
  软复位后持续为 `childState=1, secure=0, revokeActive=0`，证明 Finalize 已清除 tombstone 且旧 access 未复活。

#### 10.17 G2B-4 恢复路径与 ESP32-S3 SDK 门禁修正（2026-09-02）

真实四端恢复暴露的三个问题必须按所属边界修复，不能在 HTTP 层增加无界重试：

- App 的 DirectSecure 发送必须串行覆盖 `encode -> AEAD encrypt -> record send` 整个事务。否则后台可靠
  Patch ACK 与前台 enrollment/attach request 可并发使用同一 sequence/nonce，并同时占用 Broker 单槽 relay；
  Broker `RELAY_BUSY` 继续作为硬冲突保护，不增加 ACK-of-ACK 或新的 wire kind；
- attach 恢复冻结为 `Cloud -> child -> Cloud`。`Created/PendingAccessDelivery` 只推进 Service/Broker/Hub 的
  sealed delivery，不占用 child GATT；只有 `PendingChildInstall` 才建立一次有界 Admin DirectSecure 会话，
  `PendingGatewayProof` 再回到 Cloud/Hub。不得跨 Hub secret delivery 长时间持有手机到 child 的连接；
- XIAO ESP32-S3 在 child credential durable mutation 后刷新 Direct advertising 时，Arduino-ESP32 3.3.1
  触发 `ipc0` stack canary。与现场固件一致的 ELF 解码为
  `btdm_intr_alloc -> esp_intr_alloc -> heap_caps_malloc -> ipc_task`，并非 Blinker NVS、控制工作区或 BLE
  callback 越界。3.3.1 的预编译 S3 SDK 将 `CONFIG_ESP_IPC_TASK_STACK_SIZE` 固定为 1024 B，低于 ESP-IDF
  对 S3 的 1280 B 默认值；只升级 SDK 仍不是完整发布证据。2026-09-03 同一块 XIAO 的修正 A/B 证明：此前
  3.3.10/3.3.11 的 TLSF/watchdog 探针没有声明 BLE 正在使用，Arduino Core 已在 `initArduino()` 释放 BLE
  内存。3.3.11 / IDF 5.5.5 使用 2048 B IPC stack；外部 NimBLE-Arduino、Core 自带 `BLEDevice` 和加入
  `esp32-hal-alloc-ble-mem.h` 的直接 C API 均成功首启/广播，`ipc0` 最低约 1104～1108 B。3.3.11 因此恢复为
  product-level 候选；3.3.10 仍只有 1024 B IPC stack，且旧崩溃结果作废、未按正确合同重测。

设备库不再用整套 `stop -> nimble_port_deinit -> nimble_port_init` 刷新 Presence locator。内部
`IBleModeLink::refreshBleProfile()` 是可选能力；ESP32 仅在无连接时停止 advertising、原位替换 13 B mode
service data 后恢复 advertising，GATT、NimBLE Host 和 controller 均保持存活。连接竞争只标记待刷新并在空闲时
重试；UNO R4/Wio 等未实现原位刷新的 Port 仍走既有模式切换回退。该修正不改变 BBP/2、RelayBatch、
DeviceAccessStore layout 或公开 Arduino API。

3.3.6 仍带 1024 B IPC stack，故它只保留为历史功能联调基线。3.3.11 在当前 Port 补齐内存保留合同后进入
product-level 复测；正式 10→30→100 循环只允许通过前置资源门禁的 profile。发布退出门禁必须
真实覆盖：冷启动、首次 enrollment、凭据安装后的 Direct 广播刷新、15 分钟 locator 轮换、连续重连，以及
WiFiProv→Direct 的所有权交接；任一流程再次初始化 controller 即视为架构缺陷。

上游层级复核现已纠正：本轮失败来自 Blinker 裸 ESP-IDF C API Port 未接入 Arduino Core 的启动期 BLE 内存
保留合同，不是 `esp32c3-bt-lib` controller 或 `esp-nimble` Host 缺陷。因此仍不建立 NimBLE fork，也不再
自建 3.3.6/2048 B Core。正式用户继续只安装 Blinker 与官方板卡 Core；WiFi-only 不受影响，3.3.11 作为
BLE/WiFi+BLE/Edge Hub 候选完成 production matrix。详细依据和退出条件以
`DEVICE_V2_BLE_DIRECT_GATEWAY_ROUTING_ARCHITECTURE.md` 的“ESP32-S3 NimBLE 上游归因与 SDK 维护边界”为准。

2026-09-04 product-level 复测补充：隔离安装的官方 3.3.11 / IDF 5.5.5 在同一 XIAO ESP32-S3 上运行完整
Edge Hub composition，资源探针得到 `CONFIG_ESP_IPC_TASK_STACK_SIZE=2048`、idle `ipc0=1284 B`、
WiFi+NimBLE `ipc0=1108 B`、`nimble_host=2812 B`、`btController=2908 B`，无 canary/watchdog；完整固件为
`1,410,740 B Flash / 64,460 B globals`。一度观察到的 Broker `keep alive timeout` 不是 PubSubClient、TLS
或 NimBLE 回归：临时 wire 探针证明每 30 s 的 2 B PINGREQ 完整写入且约 80 ms 收到 PINGRESP；关闭串口监视器
后，内部 `Esp32EdgeHubDualBoard` probe 的长 `Serial.print()` 堵塞 USB CDC 并停止主循环。probe 已把 ESP32
诊断 TX 设为非阻塞，正式库没有添加业务心跳或第二套 MQTT keepalive。

修正后在不打开串口监视器的条件下先跨越三个 keepalive 周期，再依次执行 10、30、100 个真实 Android
account Action。三批均为 100% ACK；最终 child revision `52→62→92→192`，`buttonPresses`
`51→61→91→191`。最后 100 条 durable command ledger 全部为 `acknowledged`；其中 54 条发送 3 次、
46 条发送 4 次，但 child 状态只精确增加 100，证明 exact-frame retry、child replay 抑制和 replay ACK
共同成立。先前 Hub 已被阻塞时返回的 `DEVICE_V2_PROTOCOL_NOT_READY` 是 Broker 在虚拟 child channel
撤销后的正确 fail-closed 行为，不作为新协议错误处理。

#### 10.18 G2A-3c 重启门禁收口与 Gateway credential 轮换边界（2026-09-04）

3.3.11 完整 Edge Hub 固件复位后重新取得短期 SessionCredential、恢复 MQTTS 与同一 active child route；
真实 Android account Action 随后使 child revision `192→193`。单独重启 Broker shard 后，Hub 使用同一
运行期 session 身份重新订阅 management/route topic，另一条 Action 继续得到 ACK，revision `193→194`。
结合 G2A-2g 已完成的 child reboot/低 revision StatePage 代际替换，以及本轮 100 次命令中 3～4 次传输重投但
业务只执行一次的证据，`Hub/Broker/child reboot + QoS/exact-frame duplicate` 不再重复列为 G2A-3c 未完成项。

剩余项必须准确命名为 **active Gateway credential renewal/replacement transaction**，不能与以下已有能力混淆：

- DeviceKey refresh 只轮换 Hub 的 Cloud 登录根，不改变 child Gateway slot；
- PresenceKey rotation 只更新 child 的匿名发现 locator，当前 352 B active record 的 `active+pending` 只保护该过程；
- detach 后重新 attach 使用 `Install`，因为旧 Gateway slot 已经 durable revoke；它能完成有人值守的 Hub
  replacement，但不是 active credential renewal；
- active credential renewal 必须对现存 slot 使用 `Rotate(expectedVersion -> newVersion)`，不能在 Service 注释、
  UI 或测试中继续笼统称为 `credential rotate/rollback`。

首版 active renewal 冻结为独立于 topology 主状态的有界 operation ledger，避免把现有 1～10 topology enum
继续扩张成难以审计的混合状态机：

1. Service 生成 version `N+1` 的独立 Gateway secret，以 KMS 保存，并将 exact candidate access material 密封给
   当前 active Hub；Broker 先暂停该 child 的新 route command，只允许 management；
2. Hub 把 candidate 写入独立 protected pending journal 后 ACK。不得覆盖当前 352 B active access record，也不能把
   raw secret 放入 topology 普通列、Broker ledger、日志或 App durable storage；
3. Hub 仍以 version `N` 完成 Method 2，取得 fresh child control nonce，再中继服务器签名的
   `Rotate(expected=N, credential=N+1)`；child 原子替换同一 controller slot，exact Grant 重放保持幂等；
4. Hub 在 protected pending journal 保存 exact receipt/digest。随后原子 promote candidate 为 active，并以 version
   `N+1` 重新 Method 2/proof；Service 只有在 receipt、Hub durable promote ACK 与新 proof 都匹配后才 CAS 提交
   credential metadata、恢复 route，并销毁旧 KMS material；
5. child mutation 前的失败可安全取消：清 pending、恢复旧 route。child 已返回 durable receipt 后不存在“回滚到旧
   secret”，因为旧 secret 已失效；此阶段必须标记为 `ForwardRecoveryRequired`，重启后以 pending version `N+1`
   重放 exact Rotate/receipt 或重新 proof，禁止错误地恢复旧 route；
6. 同一 active topology 同时最多一个 renewal；operationId、old/new version、child instance、accessEpoch、
   controllerId、Hub identity、candidate digest 和 deadline 全部进入 CAS/idempotency binding。

为了保持模块边界，pending journal 与 renewal coordinator 只装配进 Edge Hub product；普通 WiFi/BLE/WiFi+BLE
设备、BBP/2、组件 API 和 `Blinker.begin()` 不增加分支。Hub replacement 的首版可用路径仍是
`revoke/detach old -> attach new`，要求 owner/Admin 在 child 附近完成新 Install；无人值守、尽量无中断的
replacement 复用上述 renewal 原语，但必须另行增加 exact new-Hub seal 与切换证明，不在本批偷渡多 Hub/roaming。

#### 10.19 G2A-3c/R1 active renewal wire 与跨端 golden（2026-09-04）

R1 已冻结且通过 Device C++ / Service JS literal golden。它只定义协议边界，不把尚未实现的 protected journal、
AEAD open 或 coordinator 冒充为可发布续期能力。既有 topology operation 与新的 renewal operation 必须是两个
独立 16 B 标识；拓扑 1～10 状态码保持不变。Service management kind 7～11 已被 permit-join discovery/relay
占用，因此新增 kind 固定为：`12=RenewalCommand`、`13=RenewalResult`、`14=RenewalDeliveryAck`。

密封下发 `GatewayCredentialRenewalEnvelopeV1` 是 canonical CBOR array 20：

```text
[1, renewalOperationId, topologyOperationId,
 environmentId, tenantId, edgeHubLogicalDeviceId, deviceKeyVersion,
 childLogicalDeviceId, childDeviceInstanceId, accessEpoch, controllerId,
 expectedCredentialVersion, credentialVersion,
 permissions, presenceKeyVersion, accessMaterialDigest,
 expiresAtUnixSeconds, nonce12, ciphertext32, tag16]
```

其中 `credentialVersion` 必须严格等于 `expectedCredentialVersion + 1`；ciphertext 只包含 32 B candidate Gateway
secret，现有 PresenceKey 不重复密封。`accessMaterialDigest` 绑定
`SHA-256(candidateSecret || currentPresenceKey)`，从而让 envelope、pending journal、management transaction 与
后续 GatewayProof 指向同一候选材料。AAD 是上述前 18 项；上限分别为 Envelope `627 B`、AAD `576 B`。

Hub 只有在 candidate 已写入 protected pending journal 后才能发：

```text
[1, 14, renewalOperationId, topologyOperationId,
 accessMaterialDigest, pendingStorageRevision]
```

上限 `76 B`。管理命令是 canonical array 17，上限 `337 B`：

```text
[1, 12, renewalOperationId, topologyOperationId, topologyVersion,
 childDeviceInstanceId, accessEpoch, controllerId,
 expectedCredentialVersion, credentialVersion,
 pendingStorageRevision, activeStorageRevision, accessMaterialDigest,
 phase, grant, receiptDigest, expiresAtUnixSeconds]
```

`phase=Prepare` 只带 deadline；`Apply` 只带不超过 `193 B` 的 signed Rotate Grant；`Finalize` 只带已 promote 的
activeStorageRevision 与 32 B receiptDigest；`Cancel` 不带 grant/digest/deadline。Finalize 之前的新凭据安全确认不
新增重复消息，而是复用已经发布门禁通过的 `GatewayProofCommand/Result`。

管理结果是 canonical array 13，上限 `274 B`：

```text
[1, 13, renewalOperationId, topologyOperationId, topologyVersion,
 phase, status, pendingStorageRevision, activeStorageRevision,
 credentialVersion, accessMaterialDigest, payload, receiptDigest]
```

状态固定为 `NonceReady / Rotated / Finalized / Cancelled / ForwardRecoveryRequired / Rejected / Expired`。
`NonceReady` payload 必须恰为 16 B nonce；`Rotated` 必须带不超过 `145 B` 的 exact durable receipt、32 B digest
和非零 activeStorageRevision；`ForwardRecoveryRequired` 只能属于 Apply 且不得伪造 receipt 或 active revision。
是否允许 Cancel 仍由 journal 的 irreversible marker 判定，不能仅凭 wire phase 判断。

本批门禁：Service protocol 18/18 通过；Device Release host 42/42（含 fuzz smoke）通过；ESP32 交叉编译器以
`-Wall -Wextra -Wpedantic -Werror` 编译 `BlinkerProtocol.cpp` 与 C++ golden 通过。下一批严格进入 R2：独立
Edge-Hub-only protected pending journal + renewal AEAD domain/open；不得先修改 352 B active store 或接入 Broker
coordinator。

#### 10.20 G2A-3c/R2 protected journal 与独立 AEAD open（2026-09-04）

R2 已完成，但仍不宣称 active renewal 端到端可用。设备新增独立
`GatewayCredentialRenewalStore`，使用 Edge Hub 专属的 `bl_eh_renew/pending` 原子 NVS blob；生产 composition
继续要求 encrypted NVS，`PlainFlash` fail closed。现有 `GatewayAccessStore` 的 352 B active record 没有被扩大或
覆盖，普通 WiFi/BLE/WiFi+BLE product 也没有获得 renewal 状态或公开 API。

journal 固定为一个 360 B canonical record，只保存向前恢复所必需的 candidate secret、双 operation id、child
instance/access epoch/controller、严格 `N→N+1` version、PresenceKey version、material digest、
grant digest、exact receipt 和 promote 后的 active storage revision。它不复制 child logical id、当前 PresenceKey 或
完整 active record。状态固定为：

```text
Staged -> MutationAttempted -> ReceiptStored -> Promoted -> clear
```

`MutationAttempted` 必须在首次向 child 写 Rotate Grant **之前** 原子落盘；在 mutation 结果未知期间 `Cancel` 必须拒绝。
重启后不能把绑定旧 control nonce 的 Grant 直接发进新会话，而应先用 pending candidate/active credential 建立 fresh
Method 2：认证为 `N+1` 时继续向前恢复；认证仍为 `N` 时将 journal 原子标为 `Abandoned`，稳定返回 `Rejected`，再由
Service 显式 `Cancel` 清理。`deliveryRevision` 与后续本地 `storageRevision` 分离，因此 envelope 的 QoS
重放即使发生在后续状态也不写 flash，并继续返回最初的 `pendingStorageRevision`。receipt 与 promote 操作均做
exact replay 校验；Finalize 只有在 operation/material/receipt digest/active revision 全部匹配时才清 journal。

renewal 密封没有复用 initial access AEAD key。Service 与 Device 先按既有 Hub identity context 派生
`GatewayAccessWrapKeyV2`，再执行第二层 HKDF：

```text
salt = "BLINKER-GATEWAY-RENEWAL-WRAP/1\0"
info = "BLINKER-GATEWAY-CREDENTIAL-RENEWAL/1"
IKM  = GatewayAccessWrapKeyV2
```

Device open 后还必须常量时间校验
`SHA-256(candidateSecret || currentPresenceKey) == accessMaterialDigest`；失败时清 candidate、AAD 和临时 key。
跨端 literal key 为
`012a02f4fe369d52a6eacd3f8fe998e9d502824f50853625583df9e24dc64f37`。

门禁结果：Device Release host `43/43`（含 20k fuzz smoke）通过；journal 的 reopen、write failure、CRC corruption、
exact replay、不可逆 Cancel 拒绝和完整状态迁移通过；Service protocol `18/18` 与 Gateway access/KDF 测试通过；
XIAO ESP32-S3 `esp32-edge-hub-internal` clean compile 通过（flash `1,398,295 B`，静态 RAM `63,012 B`）。SDK
自身的 missing-field-initializer warning 仍存在，但本批没有新增库 warning。架构检查仅报告本轮之前已有的
WiFiProv login-freeze hash 漂移，不能误记为 renewal 回归。

`topologyVersion` 不进入 journal：sealed envelope 没有该字段，且唯一 topology operation id 已绑定本地凭据事实；
version 只属于后续 authenticated management command 与 Service CAS。要求 delivery 阶段凭空持久化它会造成无法
构造的合同，已经在进入 R3 前删除，记录由 364 B 收敛为 360 B。

R2 结束时冻结的下一批是 Edge-Hub-only renewal coordinator：校验 active record 与 pending envelope、在 journal
marker 后中继 signed Rotate、receipt 后原子改写 active record、复用 GatewayProof 验证新 version，并输出可重放
结果；该项现由 10.21 的 R3 完成。Broker/Service operation ledger、route pause/CAS 与真实四端轮换仍是后续 R4，
不能用设备内存状态替代。

#### 10.21 G2A-3c/R3 设备侧可恢复 Rotate transaction（2026-09-04）

R3 已完成设备侧事务，但仍不代表四端 active renewal 可用。management delivery 现由小型
`GatewayManagementDeliveryMux` 按 exact wire 分发：initial access 继续使用原处理器，renewal 使用独立
`GatewayCredentialRenewalDeliveryProcessor`。后者先校验 Hub identity context 与当前 active topology，再用独立
renewal AEAD domain 打开 candidate，只把 `Staged` 写入 360 B protected journal。相同 envelope 在
`MutationAttempted/ReceiptStored/Promoted` 后重投仍返回最初的 `pendingStorageRevision`，不重写 Flash；journal 已清而
active 已是新版本时拒绝迟到 envelope，避免把已 Finalize 的 operation 重新创建为 pending。

`BasicGatewayCredentialRenewalCoordinator` 只做四阶段设备编排，没有复制 BLE、Method 2、ControllerControl 或 proof：

1. `Prepare` 复用 route bridge 的唯一 child sequence allocator，在旧 version `N` 的 DirectSecure session 中取得 fresh
   control nonce；
2. `Apply` 先校验 exact pending/active identity、严格 `N→N+1`、signed Rotate Grant 的 secret digest，再在首次 child
   write **之前**持久化 `MutationAttempted + grantDigest`；
3. child 返回的 Rotate receipt 必须逐字段绑定 Grant，并通过 candidate secret 的 HMAC proof。Hub 先持久化 exact
   receipt，再以 CAS 原子更新 352 B active record，最后将 journal 标成 `Promoted`；任一步中断均可由同一 Apply 从
   durable state 向前恢复；
4. promote 后强制退役旧 DirectSecure session。`GatewayChildSession` 现在把建立 Method 2 时使用的
   `credentialVersion` 保存为 session fact；`GatewayProofCoordinator` 同时校验 PresenceKey version 与该 session
   credential version，禁止 active store 已更新后把旧安全会话冒充成新凭据 proof；
5. `Finalize` 仍只在 Service 已完成现有 GatewayProof 后清 journal。Cancel 允许 `Staged`，以及 fresh Method 2 已证明
   child 仍为 `N` 后写入的终态 `Abandoned`；`MutationAttempted` 结果未知时仍返回 `ForwardRecoveryRequired`。
   Cancel/Finalize 已清 journal 但 MQTT 结果尚未发布时，即使 Hub 重启也会幂等重放原终态。

Revoke 与 Renewal 共用一个私有 ControllerControl 接收口。Edge Hub product 在构造完成后只把该接收口绑定到
`GatewayManagementControlMux`，mux 按当前 authenticated management command 家族分发响应；两个 coordinator 不再因
构造顺序互相覆盖。普通 child route 仍不能发送 ControllerControl，普通 WiFi/BLE/WiFi+BLE product 与公开 Arduino
API 均无变化。`Blinker.resetAccess()` 会先清 renewal journal，再清 active child access 与 DeviceKey，避免 access
reset 遗留 candidate secret。

门禁结果：Release host + 20k deterministic fuzz 共 `44/44`；新增测试覆盖 sealed delivery、promote 后 envelope
exact replay、Prepare→Rotate→HMAC receipt→promote→Finalize、mux response dispatch、空 journal 的重启终态重放，
以及旧 credential session proof 拒绝。当前本机安装的 ESP32 Arduino Core 3.3.6 对
`esp32-edge-hub-internal` 当前源码编译通过，输出为 `1,414,799 B Flash / 63,788 B globals`；这只是交叉编译门禁，
不替代既有 3.3.11 真机资源/稳定性门禁。architecture gate 仍只有本批之前已记录的两条 WiFiProv login-freeze hash
漂移，release-profile gate 通过。

Hub 只校验 Grant 与本地 pending 的结构/digest binding，不在 Hub 建立第二个服务器签名信任根；Grant 的 server
signature 仍由 child 现有 `ControllerControlCoordinator` 验证。这样 renewal 没有复制或弱化既有授权边界。

下一批严格进入 R4：Service/KMS operation ledger 与 exact envelope、Broker 在首次 delivery 前暂停该 child 的新业务
command、durable phase command/result 重投、new-version GatewayProof、Service CAS commit、Finalize 后恢复 route，并
完成 Hub/child/Broker 在每个 durable 边界的掉电/重启注入。R3 不在设备端伪造 route pause 或 Service commit 状态。

#### 10.22 G2A-3c/R4a Service 权威 renewal ledger 与 route pause（2026-09-04）

R4a 已完成可独立验证的 Service 基础层，但尚未开放 active renewal 产品入口。迁移 24 新增独立
`v2_edge_gateway_credential_renewals`，不向既有 topology 十态状态机追加临时状态。一个 active topology 同时只允许
一个非终态 renewal；operation durable 绑定 renewal/topology operation id、Hub/child identity、DeviceKey version、
access epoch、controller、严格 `N→N+1`、PresenceKey version、topology CAS version、candidate material digest、
deadline 和全部 KMS/envelope bytes。

candidate Gateway secret 只生成一次：Service 使用独立 renewal HKDF domain 将其密封为最多 627 B 的设备定向 exact
envelope，并以 renewal operation id + canonical AAD digest 作为 KMS locator/binding 持久化第二份 ciphertext。数据库和
日志均不保存 raw secret；`requestDigest = SHA-256(canonical renewal AAD)`，因此 exact identity、candidate digest 与
envelope 是同一个幂等事实。Hub DeviceKey 可以在 topology 建立后正常刷新；创建 renewal 时校验的是 Hub 当前
DeviceKey version，而不是错误地继续要求旧 attach envelope 的 DeviceKey version。

route pause 不在 Broker 建立易丢失的内存开关。Service 的 child/hub active-route 查询均以 `NOT EXISTS` 排除
`pending_delivery / pending_prepare / pending_apply / pending_proof / pending_finalize /
forward_recovery_required` operation。Broker 既有上下行路径会在每个业务 frame 前重新解析该权威 route，所以
operation 先落盘就立即 fail-close，Service/Broker 重启后也不会误恢复；只有 `completed/cancelled/expired` 终态才允许
再次返回 active topology。

当前只实现 `storePending / acknowledgeDelivery / get / listWork` 与 pause 查询，未接 HTTP、worker 或公开入口。这是
刻意的安全边界：在 Broker durable envelope/phase ledger、Cancel ACK、new-version proof、topology+operation 原子 CAS
和 Finalize ACK 完成前，不能产生一个无法收敛却永久暂停 route 的线上 operation。隔离 PostgreSQL schema 已验证迁移、
exact replay、delivery ACK CAS、暂停与测试清理后的恢复；Gateway Device V2 测试和当前 Docker 开发栈重建均通过。

下一批 R4b 先实现独立 Broker renewal delivery/phase ledger，并把 management kind 12/13/14 接入 QoS1 exact 重投；
之后 R4c 才接 Service worker/KMS receipt 校验、new-version GatewayProof、单事务 CAS commit 和 Finalize 恢复 route。

#### 10.23 G2A-3c/R4b Broker durable renewal delivery/phase（2026-09-04）

R4b 已完成 Broker 传输事实层，但仍未开放 active renewal 产品入口。Broker SQLite 新增彼此独立的
`v2_edge_gateway_renewal_delivery_ledger` 与 `v2_edge_gateway_renewal_phase_ledger`：前者只保存 exact 627 B
sealed envelope、material digest、pending revision 与 ACK；后者按
`renewalOperationId + topologyVersion + phase` 保存 exact command/result、版本、revision、receipt digest 与重投状态。
两表均不保存 candidate secret、PresenceKey、DeviceKey 或 KMS plaintext。

management kind `12=renewal command`、`13=renewal result`、`14=renewal delivery ACK` 已接入既有唯一 Hub management
topic、MQTT policy、每连接限流与 500 ms dispatcher。下行始终 QoS 1、retain false；application retry 只重发 durable
exact bytes，并以 `dup=true` 标识后续尝试。Broker restart 从 ledger 恢复 pending envelope/phase；Hub management ready
时只唤醒该 Hub 的到期任务，没有增加第二个 timer、第二条 topic 或 Broker 内存 route pause。

Broker 对上行执行 canonical decode、Hub session/tenant、operation/topology/phase、storage revision、严格 `N→N+1`、
device instance、epoch、controller、权限和 digest 绑定。Rotate result 还必须携带结构正确的 HMAC receipt，并与 signed
Grant 和 command exact 绑定；Broker 不持有 raw candidate secret，因此不越权宣称 HMAC 已验证，最终 proof 校验仍归
Service/KMS。相同 ACK/result exact replay 幂等返回；变化的 replay fail-close。Apply 已过期后仍允许唯一的
`ForwardRecoveryRequired` durable 事实收敛，不能因 Broker 定时 expiry 丢失 child 已完成不可逆 mutation 的信号。

内部 HTTP 和 Gateway `broker-client` 只暴露 stage/get delivery 与 stage/get phase 四个窄操作，严格校验响应结构；业务
orchestrator 不直接拼 URL 或写 Broker SQLite。容器构建、Broker Device V2 全套门禁与 Gateway Broker-client 测试已通过，
覆盖 QoS1、exact replay、ACK/result 幂等、Broker restart、retry `dup` 和 forward recovery。

Broker ledger 是可重建的 transport projection，不进入 tenant migration bundle；Service operation/KMS envelope 才是
权威源。R4c worker 必须按 Hub 当前 shard 查询并幂等 restage 缺失的 exact delivery/phase，才能覆盖 shard cutover 与
Broker 数据丢失，不能把旧 shard SQLite 行复制成第二个权威状态机。

下一批严格进入 R4c：Service private API + 单 worker 编排 delivery→Prepare→Apply→new-version GatewayProof→
topology/operation 单事务 CAS→Finalize，Service 必须用 KMS candidate secret 校验 Rotate receipt。R4c 完成并通过四端
durable boundary fault injection 前，R4a 的 route pause 仍是权威边界，不能公开 renewal 按钮或把 operation 标成可用。

#### 10.24 G2A-3c/R4c Service renewal coordinator 与原子提交（2026-09-04）

R4c 软件闭环已完成，但仍未开放 App/owner HTTP 入口。Service 新增独立
`edge-gateway-renewal-service/repository/worker`，只装配到 Gateway 服务内部：`prepareCredentialRenewal()` 从 Active
topology、Hub 当前 DeviceKey 与 child 当前 PresenceKey 生成一次 candidate，随后单一、有界、不可重入 worker 每轮只推进
一个 durable 状态：delivery → Prepare → Apply → new-version GatewayProof → Finalize → route restore。普通设备登录、
permit-join、revoke 与业务 route 不依赖或导入该协调器。

为 R4d 真机/故障注入提供的唯一触发面是受 `GATEWAY_ADMIN_TOKEN` 保护的 internal create/read/cancel HTTP 适配器；调用者
必须显式提供彼此不同的 16 B renewal/topology operation id 以获得 exact idempotency。响应只投影状态、版本、revision 与
时间，不返回 sealed envelope、KMS 字段、Grant、receipt 或任何 secret。它不是 App API，也不代表 owner 已获得续租权限。

迁移 25 增加 `pending_cancel/pending_route_restore`，并为 Active topology 增加成对的
`gateway_secret_kms_locator/gateway_secret_kms_binding`。这是必要的 KMS 生命周期信息：初次 attach 的 secret 以 topology
operation/AAD 为上下文，续租后的 secret 以 renewal operation/canonical request digest 为上下文；只覆盖 ciphertext 而不保存
新上下文，会导致下次 reveal、revoke 或 renewal 无法正确解封。旧 topology 两列为 NULL 时继续使用初次 attach 上下文，
不需要批量解密迁移。

Service/KMS 明确区分两种不可互换的 digest：`accessMaterialDigest = SHA-256(candidateSecret || PresenceKey)` 用于
Hub/Presence/GatewayProof 绑定；Controller Grant 与 receipt 的 `secretDigest = SHA-256(candidateSecret)` 用于 child Rotate
和 HMAC receipt。Broker 只校验 canonical 结构、Grant/receipt/command exact binding；Service 临时从 KMS 解封 candidate，
验证 secret digest、receipt HMAC、Broker receipt digest 与当前 PresenceKey 后立即清零明文。Broker 不持有 secret，不能冒充
最终 verifier。

new-version GatewayProof 成功后，PostgreSQL `SERIALIZABLE` transaction 同时锁定 renewal operation，并以冻结的
topologyVersion/identity/version/epoch/controller/PresenceKey 条件 CAS 更新 Active topology 和 operation：credential 严格
`N→N+1`、storage revision/KMS context/proof 一次提交，任一条件变化则全部回滚。Hub Finalize ACK 后 operation 先进入
`pending_route_restore`；此时权威 topology 已可查询，worker 再调用 Broker admission，成功后才标记 `completed`，从而覆盖
Service 在 commit、Finalize、admission 任一点重启的恢复窗口。

Broker ledger 被严格限定为可重建 transport projection。Service 会在记录缺失或无结果超时后用同一 immutable intent 和新
transport lease 幂等 rearm；Prepare 超时不会误取消。Apply 的 `ForwardRecoveryRequired` 必须先落 Service 权威状态，下一轮
才重投 Apply，禁止在同一轮覆盖不可逆边界证据。Cancel 仅允许在 Apply 前，或设备明确拒绝 Apply、能证明没有 durable mutation
时执行；其余失败只能向前恢复。

本批门禁包括：renewal Service 正常链与 transport-expiry/forward-recovery 专项 `2/2`、worker `2/2`、真实 PostgreSQL
store→delivery ACK→Rotate receipt→proof→topology/operation CAS→Finalize→route restore→后续 revoke、Gateway Device V2
完整回归、Broker TypeScript，以及 Linux `better-sqlite3` 下 Device V2/restart/exact replay，均通过。测试同时抓获并修正了
异步 transaction 返回前过早清零 operation id 的生命周期错误，以及把 access-material digest 误当 Controller secret digest
的边界错误。

下一批 R4d 只做 durable boundary fault injection：分别在 Hub pending save/ACK、Prepare、child Rotate 前后、receipt/promote、
GatewayProof、Service CAS、Finalize ACK、Broker admission 前后重启 Hub/child/Broker/Service，并核对 PostgreSQL、Broker ledger、
Hub journal、child active slot 只收敛到旧版本可用、向前恢复新版本或明确 cancelled 三类合法结果。在该门禁和至少一次真实双板
续租闭环完成前，仍不增加 App 按钮、公开 owner API 或宣称 active renewal 可用。

#### 10.25 G2A-3c/R4d-a durable boundary 自动故障注入（2026-09-04）

R4d 已先关闭不依赖射频时序的确定性部分，且没有在产品状态机中加入 test hook。故障注入器只包裹既有 repository/Broker
端口：每个边界先模拟“调用发生前进程退出”，再模拟“副作用已经 durable、调用方尚未收到返回时进程退出”；每次退出后销毁并
重建 Service 实例，继续使用原 PostgreSQL/Broker/设备持久事实。共覆盖 28 个 Service/Broker 切点：operation create、delivery、
delivery ACK、Prepare command/result、Apply command/Rotate receipt、proof nonce、GatewayProof、topology+operation CAS、Finalize
command/ACK、Broker admission 与 operation complete。最终 topology 只提升一次 `N→N+1`，route admission 的有效副作用也只发生
一次。candidate 在 PostgreSQL commit 前允许重新生成；一旦 operation 已落盘，重启只重放原 sealed envelope/KMS ciphertext，
不得产生第二份 candidate。

设备侧补齐了同一 durable boundary 矩阵：sealed envelope 在 Hub journal 已写但 ACK 丢失后，重启返回逐字节相同 ACK 且不重写
Flash；Prepare 发出后重启只重新取得 fresh control nonce；Apply 分别从 `MutationAttempted`、`ReceiptStored`、active slot 已提升但
journal 未提升、journal 已 `Promoted` 四个边界重建 coordinator。同一 DirectSecure 会话内，child 在 Rotate 已提交但 receipt
丢失后可由相同 signed Grant 返回相同 HMAC receipt；跨 Hub 重启时旧 Grant 的 control nonce 已失效，必须以后续 fresh Method 2
认证的 credential version 判定 child 是否已提交。Hub 最终只存在 version `N+1` active slot 与 `Promoted` journal；相同
Apply/Finalize 在再次重启后重放原终态，不再次写 child。

真实持久层门禁也已补齐：Linux `better-sqlite3` 关闭/重开后，acknowledged delivery、Prepare、Rotate receipt、Finalize result
均仍可读取，pending command 仍只重投 exact bytes；真实 PostgreSQL 对 proof CAS、Finalize ACK、route-complete 分别模拟“提交成功
但响应丢失”，相同请求均返回 replay，topology version 不二次递增。相关 targeted Host、Node、Linux Broker 与隔离 PostgreSQL
门禁通过。

本节完成 **R4d-a 自动软件故障注入**。真实双板与物理复位结果见 10.26。真机测试继续复用 internal admin API，并使用只存在于
测试 sketch 的原子存储故障适配器；不得向公开 Arduino API、普通 product 或协议增加 fault 参数。

#### 10.26 G2A-3c/R4d-b 真实双板续租与 `MutationAttempted` 复位闭环（2026-09-04）

R4d-b 已完成一条正常链和风险最高的 receipt 前掉电链。正常链将真实 child credential `1→2`、Hub active storage
revision `1→2`、topology `4→5`；第二条链在 child 已 durable 提升到 version `3`、Hub 仍为 active version `2` 且
journal 仅为 `MutationAttempted` 时物理复位 Hub。恢复后 Service operation 最终 `completed`，PostgreSQL topology
`5→6`、credential `2→3`、active storage revision `2→3`，Broker delivery/Prepare/Apply/Finalize ledger 均保留 exact
结果；完成后再次硬复位 Hub，MQTTS、management subscription 与 version `3` child DirectSecure route 均自动恢复。

这次物理门禁纠正了 R2/R3 的一个过强假设：signed Grant 绑定 Prepare 会话的 control nonce，跨复位不能盲目重放。
设备侧因此只增加三个内部、可复用的最小原语，没有增加 wire 或公开 API：

1. `GatewaySessionAccessSource` 在 durable unknown 状态优先使用 pending candidate 建立 Method 2，认证失败后才回退 active；
2. `GatewayChildRouteBridge::ensureControllerControlSession()` 在 Service 已撤销普通 route 时显式启动 **management-only**
   DirectSecure，不分配 route、不打开 control nonce，也不授权业务转发；
3. coordinator 读取该 fresh session 的 immutable credential version：`N+1` 证明 child 已提交，随后由 exact Grant 与
   candidate secret 构造 canonical receipt 并继续 promote；`N` 证明 mutation 未提交，持久化 `Abandoned` 并返回可重放
   `Rejected`，Service 的 exact Cancel 才清 journal；其他 version 一律 fail-close。

receipt 的 HMAC 只证明 candidate secret possession，并非设备不可否认签名。Gateway 本来就持有待下发 secret，因此端到端权威
证据仍是 Rotate 后的 fresh Method 2 加 Service 校验的 new-version `GatewayProof`，随后才允许 topology/operation CAS。
这一区分同时解释了为何可以在 `N+1` 已认证后确定性重建丢失的 receipt，而不能仅凭 Hub 自己构造的 HMAC 宣称 child 已更新。

软件门禁更新为 MSVC ASan `44/44` + `20,000` deterministic fuzz；renewal coordinator 与 route bridge targeted tests
覆盖 `N+1` 前滚、`N`→`Abandoned`→Cancel、无 route 的管理会话拉起和 exact result replay。ESP32 Arduino Core 3.3.11
的 XIAO ESP32-S3 Edge Hub clean compile/upload 为 `1,430,872 B Flash / 65,316 B globals`，新增恢复路径没有增加
global RAM。此前第一次真机链停滞还确认是运行中的 Broker shard 使用旧 image，重建 shard 后 expired phase rearm 正常；
这属于部署镜像漂移，不以设备协议兼容分支掩盖。

R4d 规定的最小双板续租与危险物理边界至此关闭，但 active renewal 继续保持 internal-only。其后 10 次连续真实 renewal
功能门禁也已完成：credential `3→13`、operation 使用的 topology version `6→15`，权威 topology 最终为 `16`，active storage
revision `3→13`，每轮严格只增加一次。PostgreSQL 恰有 10 条对应 `completed`；Broker shard-1 恰有 10 条
`acknowledged` delivery（attempt `1..2`）、10 条 `nonce_ready` Prepare（`1..2`）、10 条 `rotated` Apply（`2..3`）和
10 条 `finalized`（恒为 `1`），没有 cancelled、expired 或重复 commit。

10 次功能循环完成后，在不清 NVS 的前提下换入只增加诊断输出的 internal sketch，静态结果为 `1,431,748 B Flash /
65,316 B globals`；重新上线并恢复 version `13` child route 后的稳定快照为 free/min/largest heap
`105,416 / 98,240 / 63,476 B`，loop/ipc0/NimBLE Host 最小栈分别为 `3,684 / 1,112 / 2,808 B`。由于 10 次循环前
没有同版插桩基线，这组数值只能作为 30 次阶段的起点，不能倒推并宣称 10 次资源无下降。

#### 10.27 G2A-3c/R4d-c route pause 竞态修复与新基线门禁（2026-09-05）

真实续期在 `26→27` 暴露了一个跨端时序缺陷。Hub 必须先 durable promote `GatewayAccessRecord`，Service 才能用
new-version proof 提交 topology；在这段合法窗口内，child 已使用 credential `N+1`，Broker 内存中却仍可能保留 credential
`N` 的旧 route projection。旧实现先拿新 envelope 与旧 projection 比较，因此把正常的 route pause/版本切换误判成严重伪造，
断开并临时封禁整个 Hub，management transaction 也被连带延迟约 60 s。

修复保持控制面、数据面和 transport 三层分离，没有增加公开 API、wire 字段或第二套 pause 状态：

1. `GatewayChildRouteBridge` 在 admission 时记录 durable `deliveryRevision`；本地 access promote 后立即退役旧数据面 admission，
   但保留已认证的 DirectSecure child management session，使 proof/Finalize 可以继续；新 topology admission 到达时重建
   BBP/2 projection。
2. Broker 先查询 Service 的 authoritative active route。权威 route 已暂停或前移时，只退役旧 virtual child projection 并丢弃
   竞态帧，不处罚已认证 Hub；只有权威 route 与缓存仍一致、envelope 却不匹配时，才保留原有 severe rejection。
3. Host/Broker 回归同时冻结两条边界：合法 `N→N+1` 竞态不能关闭 Hub；稳定权威 route 上伪造 epoch/version 仍必须拒绝。

Host Release `44/44`、Broker build 与 Device V2/telemetry/rate/recovery 回归均通过。部署新 Broker 后，原先停在
`pending_proof` 的 `26→27` operation 未修改数据库即自动恢复为 `completed`；随后 `27→28`、`28→29` 两轮 smoke 和
正式 `29→39` 十轮门禁均完成。十轮总耗时 `548.8 s`，单轮 `53.1～55.9 s`；PostgreSQL 十条 operation 全为
`completed`，credential/storage revision 每轮严格 `+1`，权威 topology 收敛为 version `42`。Broker 十条 delivery 均
一次投递即 `acknowledged`，每个 operation 的 Prepare/Apply/Finalize 均有唯一终态，未出现 envelope mismatch、ban、
cancelled、expired 或重复 commit。

资源结论必须与功能结论分开。十轮后稳定 free heap 仍约 `105.4 KB`，但历史 minimum free heap 下探到 `55,760 B`，
当前 largest block 曾降到约 `55 KB`。追加 `39→40`、`40→41` 两轮同步串口采样后，两轮均完成，最终稳定快照为
free/min/largest heap `105,432 / 53,252 / 45,044 B`，loop/ipc0/NimBLE Host 最小栈为
`1,352 / 1,112 / 2,708 B`；第二轮没有继续下降，故当前证据是重连/发现路径首次达到更深水位并形成碎片化平台，
不是持续字节泄漏；在该两轮观察点仍不能据此关闭 30/100 次门禁。

随后从 credential `41` 执行 30 次连续 renewal/reconnect，并在每轮 Service 完成后额外观察 12 s route 恢复。30 轮全部
`completed`，总门禁时间 `1,951.5 s`，credential `41→71` 严格连续。PostgreSQL 为 30 条 operation、0 非完成、0 非
`+1`、0 链断点，权威 topology 最终为 version `74`、credential/storage revision `71`；Broker 为 30 条 acknowledged
delivery 和 90 条 Prepare/Apply/Finalize 终态、0 异常 operation，application exact retry 次数范围 `1..3`。Broker/Gateway
日志没有 envelope mismatch、ban、fatal 或 error，门禁后 child 为 Secure，route/management error 均为 0。

资源方面，基线为 free/min/largest heap `105,432 / 53,252 / 45,044 B`；第 3 轮 minimum 单次下探到 `52,056 B`，
随后 27 轮不再下降，largest block 反而恢复并最终为 `55,284 B`。最终稳定快照为
`105,436 / 52,056 / 55,284 B`，loop/ipc0/NimBLE Host 最小栈始终为 `1,352 / 1,112 / 2,708 B`。
这关闭 30 次资源/重复性门禁：没有线性泄漏、持续碎片化或任务栈侵蚀证据。

随后以同一固件和部署从 credential `71` 执行 100 次门禁。每轮不仅等待 Service `completed`，还等待 child 重新进入
Secure 并采集 heap/stack；脚本对非 `+1`、链断点、非完成终态、150 s 事务超时、20 s Secure 恢复超时、route/
management error、任务栈越线和连续资源恶化均立即停止。100 轮全部通过，总门禁时间 `5,549.3 s`，credential
`71→171` 严格连续。PostgreSQL 为 100 条 completed、0 非 `+1`、0 chain gap，权威 topology 最终为 version `174`、
credential/storage revision `171`；Broker 为 100 条 acknowledged delivery、300 条唯一阶段终态、0 异常 operation，
application exact retry 次数范围仍为 `1..3`。Broker/Gateway 日志没有 envelope mismatch、ban、fatal 或 error。

100 轮资源基线为 free/min/largest heap `105,424 / 50,772 / 55,284 B`。minimum 在第 21 轮小幅下探到
`50,576 B`、第 38 轮下探到 `45,448 B`，此后 62 轮不再变化；stable free heap 始终在约 `103.3～105.4 KB`
之间恢复，largest block 在 `45,044 / 55,284 / 57,332 B` 三个平台间可逆切换，最终稳定快照为
`105,444 / 45,448 / 55,284 B`。loop stack 在第 16 轮由 `1,352 B` 一次下探到 `1,336 B`，此后 84 轮稳定；
ipc0/NimBLE Host 始终为 `1,112 / 2,708 B`。因此 100 次 active renewal 资源/重复性门禁关闭：没有线性 heap
泄漏、持续碎片化、任务栈侵蚀或 route generation 残留证据。

该结论只关闭 R4 transaction gate，不自动公开 owner/App renewal 入口，也不替代 D4b encrypted-NVS 专用板、G2B-4
真实错误 Hub/篡改/超时/restart 负测、Wio child 长稳和候选发布 24 h soak。触发 API 继续保持 internal-only，直到产品
策略、权限与恢复 UX 另行冻结；下一设备主线应在现有阻断中选择可执行项，而不是增加 Mesh、roaming 或多 child 调度器。

#### 10.28 G2B-4 App 账户/环境隔离收口（软件完成；真机负测未完成，2026-09-05）

App 复核发现原 `ControllerCredential`、permit-join、enrollment workflow 和 attach checkpoint 只按 logical device 或
operationId 存储；Service ACL 虽能阻止错误账号的云调用，同一 App 安装内已经建立的 Direct session 仍可能跨账号
存活。这不是 UI 问题，而是本地授权主体缺失，因此在进入双账号真机门禁前先修正：

- 新增统一 account context，持久记录 key 同时绑定 API authority 与 accountId；ControllerCredential 使用 v5，
  三类 gateway checkpoint 使用 v2。list/load/find/remove 都只能枚举当前 namespace；
- Direct active/open/presence-sync 与 Edge enrollment/attach/permit relay 都捕获同一 `sessionEpoch`。账号切换立即
  abort opening/presence、detach/close active session、清空 snapshot；每个异步边界返回后再次核对 context；
- Enrollment intent、PresenceKey bundle、attach relay 和 permit relay 若在账号切换后才返回，先清零所有 byte buffer
  再抛出 `DEVICE_V2_ACCOUNT_CONTEXT_CHANGED`；checkpoint 访问同样在调用前后检查；
- v1..v4 无 namespace 的 App 测试 credential 与 v1 checkpoint 无法证明属于哪个账号，禁止静默迁移。升级后的旧
  测试设备需重新 enrollment/attach，不能用“当前登录用户”猜测归属；
- App local build 成功，全量 Angular/Vitest 为 92 files / 713 tests；本批相关文件单独 ESLint 无错误。Service 既有
  permit-join、HTTP 与 access 软件门禁通过；N1 发现的 recovery 命名语义已同时重复内存仓库与真实 PostgreSQL 测试。

该批只关闭 G2B-4 的 App 软件账户边界。下一批按风险顺序执行真实双账号/错误 Hub，再做 expiry、tamper/replay、
App force-stop，以及 Hub/child/Broker restart。测试必须核对旧账号 Direct session 已断开、旧 namespace 不可见、
Service topology/Broker ledger 未产生跨账号推进；未完成前 G2B-4 和 production release 仍保持阻断。

| 顺序 | 真机注入 | 必须同时成立 |
|---|---|---|
| N1 | 已连接 child 时从账号 A 切到 B，再切回 A | 切换瞬间 GATT 关闭；B 不枚举 A 的 credential/checkpoint，也不能推进 A 的 operation；切回 A 后只能从 A namespace 恢复 |
| N2 | 同一 owner 用 Hub-B 恢复绑定 Hub-A 的 workflow/operation | App 不展示为 Hub-B recovery；Service 对不可变 Hub 不匹配 fail-closed；child/controller slot 与两台 Hub access store 均不变化 |
| N3 | permit/enrollment/attach 分别注入 expiry、单 bit tamper、exact replay 与 same-id/different-bytes replay | 按 §10.31 分层判断：拒绝新过期授权、保留已完成事实；非法篡改零写入，完成后的 exact replay 不重复创建 ownership/topology 或写 Flash；合法重封不等于业务重放冲突 |
| N4 | 在 workflow save、pending credential save、child install、attach checkpoint、Hub delivery/receipt 各边界 force-stop App | 每次只恢复同一 exact operation；无依据记录不猜测 Hub/child；App 不跨越 Cloud 等待持有 child GATT |
| N5 | 分别重启 Hub、child、Broker，再组合重启 | 从 durable record/ledger 收敛；旧 session/nonce 不复活；最终最多一个 Active topology 和一个对应 Gateway credential |

2026-09-05 已关闭 N1 的真实 Android/XIAO ESP32-S3 门禁。COM12 在不擦除 DeviceInstanceId 的前提下执行
`resetAccess()`，App 的 A 账号重新 enrollment 后仍得到原 logicalDeviceId
`ble_8Zc1CXfpwQtN1NqFWR6BvQ`，安全 session 为 ready 且同步 3 个 endpoint。保持该 child GATT ready 时执行账号切换，
App 内部状态从 `sessions=1, links=1, state=ready` 收敛为 `0, 0, closed`；B 账号只枚举自己的 1 台设备，对 A child
返回 `active=false, pending=[]`。切回 A 后 v5 credential 可见并重新建立 ready Direct session。PostgreSQL 中既有 Active
topology 的 operation、Hub、child 与 `updated_at` 未被本次切换推进，说明账户切换没有冒充 gateway operation。

该次重 enrollment 同时发现 Service 会把已有设备名覆盖为 App 的默认“New device”。访问恢复只应轮换授权材料，
不应隐式修改用户元数据；commit 已改为：logical device 首次创建时写入 `displayName`，同 owner recovery 仅恢复
`state/updated_at` 并保留原名。内存仓库和真实 PostgreSQL BLE enrollment 测试均通过。N2～N4 与 N5 其余故障边界
仍未关闭；尤其 N2
需要两台归属同一 owner 的独立 Hub，不能用同一 Hub 的别名伪造。

N2 软件预门禁已继续收紧：`EdgeGatewayAttachRelay` 在任何 child GATT 副作用前，逐次核对 Cloud topology 的
`operationId + edgeHubLogicalDeviceId + childLogicalDeviceId + childDeviceInstanceId` 与 durable request 完全一致；不匹配
时先清零 topology/relay/recovery 的字节缓冲，再返回 `EDGE_GATEWAY_TOPOLOGY_CONTEXT_MISMATCH`。新增测试证明把 Hub-A
响应注入 Hub-B recovery 时不会打开 child Admin control。当前 PostgreSQL 只有一台 Active Edge Hub，因此该项只关闭
N2 软件边界，不冒充双 Hub 真机门禁；待第二台 Hub 可用后还需同时核对 child slot 和两台 Hub access store 零变化。

#### 10.29 child access-root supersession（软件、持久层与四端注入完成，2026-09-05）

`resetAccess()` 是一次本地授权根替换，而不是普通 Gateway Revoke。它会原子清除旧 accessEpoch、Admin/Shared/Gateway
credential 与 PresenceKey；因此普通 Prepare→child Revoke 已失去可认证对象。若 Service 仍保留 Active topology，Cloud
会错误显示可路由，Hub 也可能继续持有已经失去权威性的旧 Gateway secret。冻结以下单一恢复路径：

~~~text
child resetAccess + new BLE enrollment commit
  -> same physical DeviceInstanceId, new accessEpoch/receipt digest
  -> same PostgreSQL transaction marks every old live topology Revoking
  -> Service worker revokes Cloud/Broker route first
  -> Broker atomically expires old pending delivery/proof/renewal/revocation work
  -> Broker sends terminal Supersede to the exact old Hub
  -> Hub exact-clears the old outbound Gateway credential without child GATT
  -> Hub returns Retired(binding digest)
  -> Service CAS topology to Detached/supersessionFinalizedAt
~~~

不可放宽的不变量：

- 触发依据是同一 16 B `DeviceInstanceId` 的新 server-accepted enrollment receipt，不是 MAC、广播名或 App 猜测；
- 新 accessEpoch/DirectAccess 与旧 topology 的 `access_superseded_at` 必须同事务提交，失败时两者一起回滚；
- `Supersede` 复用原 topology operationId，携带更高 topologyVersion、旧 child/epoch/controller/credential 与 32 B
  supersession receipt digest；grant 为空、wire expiry 为 0，不建立 child 连接；
- 正常情况下 Hub 还应 exact-match `accessStorageRevision`。只有 Service 从未收到原 durable Hub ACK 时该值才允许为 0，
  0 仅在其他全部不可变绑定已精确匹配后表示 revision wildcard；
- mismatch 必须保留 Hub access。exact match 后只清该 topology 的 outbound credential；不得执行 Hub factory reset，
  不得清 WiFi/DeviceKey，也不得触碰其他 southbound adapter 状态；
- `Retired` 只证明 Hub 已持久清除旧 access。只有 Service 校验 operation/version/digest 后才能写 Detached；重复命令和
  重复 result 返回同一终态；
- superseded topology 禁止进入 nearby Admin revocation recovery。旧 Admin slot 已不存在，签发 Revoke Grant 既无效又会
  与 supersession 状态机竞争；
- 当前 G2A product profile 是每 Hub 单 child，Broker route revoke 因而仍为 Hub-scoped。扩展到多 child 前必须先改为
  per-child virtual route admission/revocation，不能直接复用这个 Hub-scoped 假设。

本批已经通过 C++ exact store/coordinator、共享协议 18/18、Broker Device V2 全套及 stale pending
delivery/proof 退役、Gateway Service/真实 PostgreSQL transaction 测试。本机已有 PostgreSQL 数据已应用 migration 26，
两个已有 Broker SQLite ledger 均自动升级并健康启动。ESP32 Arduino Core 3.3.11 的 Edge Hub clean compile 为
`1,424,968 B Flash / 65,308 B globals`。

真实注入使用 COM12 child 保留 DeviceInstanceId 执行 `resetAccess()`，App 重新 enrollment 后得到同一
`ble_8Zc1CXfpwQtN1NqFWR6BvQ` 与新 Direct access。Service 在同一事务把旧 operation
`2a6cd779b076710efc13c7332d90cad1` 推进为 Revoking/version 175。该路径首先暴露 Gateway Broker client
仍把 revocation phase 写死为 `1..3`，虽共享协议与 Broker 已支持 `Supersede=4`，请求仍在本地边界被
`INVALID_EDGE_GATEWAY_REVOCATION` 拒绝；响应校验也遗漏 `retired`。实现已改为直接复用共享协议 phase 枚举并接受
terminal `retired`，stage/get HTTP client 回归和真实 PostgreSQL access 测试均通过。

修复部署后没有手工改库：worker 自动重试，COM11 Hub 收到 terminal command；Broker shard 1 账本形成唯一
phase 4 `retired`（attempt 1、59 B result、32 B digest），相关 delivery/proof/renewal/revocation 均无 pending；Service
以同一 version/digest CAS 为 Detached，并写入 `supersession_finalized_at`。Hub 随后硬复位，启动后
`childState=0/revokeActive=0/managementSubscribed=1`，没有重新认证旧 child；App 新 Direct session 仍为 ready，打开
同一设备页并同步 3 endpoint。由此关闭 N5 的 access-root supersession 四端与 Hub reboot 子门禁。N2 错误 Hub、N3
expiry/tamper/replay、N4 force-stop，以及 N5 在其他 pending durable boundary 的组合重启仍独立阻断发布。

#### 10.30 N3 permit-join 选择负测与 SDK 基线校正（2026-09-05）

本批先纠正测试方法，而不是扩展 production 协议：

- 上批临时诊断后的默认 CLI 编译实际选择了 Core **3.3.6**，不是已准入的 3.3.11；编译数据库明确引用
  `esp32s3-libs/3.3.6`。因此该轮低 IPC 水位不能用来否定 3.3.11 已完成的资源门禁，也不能据此认定
  NimBLE scan 有未修复缺陷。临时广播 trace 已从 Port 与 sketch 清除；
- 当前使用隔离配置 `D:/codes/.arduino-cli/esp32-sdk-3311-ab.yaml` 与 CLI 1.5.2-rc.1 编译同一实际 Hub
  sketch，保留分区表/NVS 烧录 COM11。Core 3.3.11、IDF 5.5.5、IPC stack 2048 B；本批固件为
  **1,432,572 B Flash / 65,332 B globals**，app bin SHA-256 为
  `84702d9bc5a910c864174565f7951a3c542abfa41f1a9614447065a4ad1f0cc2`；
- `Open` 返回 pending_open 是异步控制面正常状态。App 正式 link 已通过 operation GET 对账至 Ready 后才
  读 relay；手工测试也必须遵守这一顺序。仅轮询 `/relay` 不会推进 Service 对 Broker Ready 的投影；
- 新增 `tests/hardware/edge_gateway_permit_join_gate.mjs`，复用 Service 唯一共享 codec，通过真实登录 Android
  WebView 调 owner API。凭据不导出，所有等待有界，失败也关闭 exact window，不再手工连续开窗口后等待失效。

真机揭示并已修正的 Broker 问题：旧 mailbox 收到 SelectResult 并清空 selectCommand 后，只对“相同 token”
返回幂等结果，却允许“不同 token”落入再次下发分支。现规定**同一未重启 relay session、同一 operation 的
selection 不可替换**：有结果时原 token 只返回原结果，变更 token 一律 `EDGE_GATEWAY_PERMIT_JOIN_RELAY_CONFLICT`。
软件回归同时覆盖 Connected 和 NotFound，明确断言两类重放均不会再次 publish 到 Hub。Broker 重启导致的
`relaySessionId` 换代仍遵守已有 App fail-closed 合同，不把易失 mailbox 冒充跨重启 durable ledger。

四轮实际 operation（hex）：

| operation | 单 bit token | exact Select replay | 改回真实 token | 最终 Service/Broker |
|---|---|---|---|---|
| `0e2157ebeed423ea843cd6438a615690` | NotFound，maxPacket=0 | 原结果逐字节一致 | 409 | Closed / Closed |
| `2d823ae62c91c54c5d8bb66374fc691f` | NotFound，maxPacket=0 | 原结果逐字节一致 | 409 | Closed / Closed |
| `98334cf953a44d8421fc1fd278fd2fbd` | NotFound，maxPacket=0 | 原结果逐字节一致 | 409 | Closed / Closed |
| `65430e7172e87a2019f335cb8e04cbcb` | NotFound，maxPacket=0 | 原结果逐字节一致 | 409 | Closed / Closed |

每轮均由真实 Hub 发现 1 个候选，并验证 exact Open 不延长 expiry、同 ID 更换 adapter 返回 409。PostgreSQL
四条记录都有 ready_at/closed_at，Broker 四条均为 Closed/attempt 1/27 B terminal result。从首轮开始核对
**新增 enrollment intent=0、topology update=0**。Hub 收口为 `permitState=0/permitConnected=0/relayActive=0`，
managementSubscribed=1；运行态 ipc0 最低 **1112 B**、nimble_host **2816 B**、loop **3684 B**，没有通过扩大
队列或新增 workspace 来绕过问题。这是 4 次有界选择负测，不是 authenticated 重连或长稳证据。

上批自然 300 s 过期 operation `db592f24c4569e484f6effcf0c171204` 也已核对为 Expired：exact Open 返回
expired/replayed，不延长窗口；同 ID 变更 adapter 为 409。该过期证据早于此次 3.3.11 固件恢复，不合并成
“新固件全矩阵通过”。App 过期/不可用窗口错误已映射为重新扫描提示，未知 Gateway 错误不再直接显示内部
英文错误；本批 link/page 为 **2 files / 13 tests**，targeted ESLint 通过。新文案尚未安装 APK 验证，源码测试
与 APK 真机显示验证分别记录。

下一执行顺序：先补 N3 **enrollment grant/receipt 与 attach envelope/receipt** 的 expiry、单 bit tamper、
exact replay、same-id/different-bytes，并核对持久写入不重复；再执行 N4 各 durable boundary 的 App force-stop、
N5 对应 pending boundary 的 Hub/child/Broker restart。N2 真正双 Hub 仍等待第二台 Hub，D4b 与发布长稳独立
保留。不增加 Mesh、多 child scheduler、私有 NimBLE fork 或新 API；本节不关闭整个 G2B-4。

#### 10.31 N3 enrollment/attach 分层边界（软件门禁，2026-09-05）

原 N3 表将所有 expiry 都称为“单调终态”、所有 same-id/different-bytes 都称为“零写入”过于宽泛，
会误伤已经安装但尚未云端提交的恢复，以及既有合法 Gateway envelope 重封。修订验收口径如下，不新增 wire：

| 层/阶段 | 到期或重放时的正确行为 | 校验方与边界 |
|---|---|---|
| permit window | 到期即不可重新 Open；exact retry 不续期 | Service/Broker/Hub 的窗口控制面；§10.30 已有选择负测 |
| enrollment 首次安装 | 有可信 Unix 时间时检查 `[issuedAt, expiresAt)`，失败零 Flash 写入 | child 校验签名、设备/epoch/Noise transcript、controller/Presence digest；无可信时间的 MCU 不宣称独立验证墙钟过期 |
| enrollment 已安装、同 session 原 EnrollRequest | 即使现在到期，也返回原 receipt，不重复写入；同 requestId 改 bytes 拒绝 | child 已完成请求的 exact digest；新 Noise session 不继承此例外 |
| enrollment 云端未提交或提交响应丢失 | grant 过期不自动形成 ownership；但 owner 在原 credential 完成 Method 2 后可提交原 receipt；Active exact commit 零更新 | Service 的 intent/commit digest 与 serializable transaction；cancelled/superseded 不恢复 |
| Gateway envelope 首次接收 | 无可信时间、credential expiry 到达或 AEAD 校验失败，零写入/零 ACK；已保存记录也不放宽过期 envelope admission | Hub mandatory-time + 当前 DeviceKey + AEAD + protected store；不要混淆 credential expiry 与 Broker 的短 delivery TTL |
| Gateway envelope 重放/重封 | exact envelope 返回原 ACK 零重写；合法重封同一 access material 可更新 envelope metadata，ACK identity 不变；业务材料变化必须走对应 version/rotation 合同 | 既有 GatewayAccessStore；不能把 nonce/ciphertext 改变一概判为业务冲突 |
| child Gateway install 回执 | 同 operation、child/epoch/controller/version/permissions、secretDigest、HMAC 全部匹配才推进；无效回执不进入 Method 2/Service commit | App 与 Service 均有 Gateway secret，分别验证；receipt 恢复与控制 nonce 的会话有效性分开判断 |

信任模型必须如实记录：**Admin secret 不上传 Service**。Admin receipt 的完整 bytes/HMAC 由 App 校验，
App 在首次安装前保存可确定计算的预期 receipt，之后还必须做 Method 2 才能提交或恢复。Service 只核对
已登录 owner、intent 与 receipt 的不可变字段、幂等 digest 及 `method2Confirmed` 声明；它不具备独立验证
Admin HMAC 的能力，不能把该声明称为设备硬件证明。Gateway receipt 则因 Service 托管 Gateway secret 而
可独立验证 HMAC。此处不改变 EducationNearby 的信任假设，也不引入上传 Admin secret 的“简化方案”。

本批实际实现与清理：

- App `EdgeGatewayAttachRelay.validateReceipt` 补齐 `receipt.secretDigest == SHA256(gatewaySecret)`。此前仅
  验证 receipt HMAC 和其他上下文，缺少这项材料绑定；即使 HMAC 正确，错误 digest 也应在 Method 2 前拒绝。
  新增临时摘要在 finally 清零，不新增常驻状态、transport 分支或 MCU workspace；
- 设备 enrollment Host fixture 不再“只按签名长度就接受”，改为 exact digest/signature fixture，并明确它不是
  Ed25519 实现。覆盖未生效/恰好过期、签名/controller secret/PresenceKey 单 bit 篡改零写入、安装一次、
  到期后 exact replay 原回执/零重写、同 requestId 不同 bytes 拒绝；
- Gateway envelope Host 测试补齐空 store 首次 tag tamper/恰好过期零写入、无 ACK；既有 exact replay、合法重封、
  业务材料冲突、重启 ACK 与已保存记录过期拒绝继续通过；
- App client 新增真实生产协调器的 4 个负向/边界测试：grant context 变化不落 pending；receipt proof/epoch
  变化不进入 Direct Method 2、不 commit；原 receipt 仅允许进入 Method 2 下一阶段，不能提前 commit。保留可恢复
  pending 安全存储，清除临时 secret。这里 Noise/link 是替身，不能标成真机加密握手；独立 Noise/Method 2 golden
  测试继续覆盖实际 WebCrypto 路径；
- Service 补内存与真实 PostgreSQL 断言：无 Method 2 声明/字段不匹配零修改、Active exact commit 到期后零更新、
  同 commitId 改 receipt bytes 冲突；另用独立测试 owner 验证 expired intent 的已安装凭据迟到提交和 exact replay。
  Ed25519/P-256 使用 Node 真签名验证的单 bit signature tamper 均失败；Gateway secretDigest 变化即便重算合法
  HMAC 仍被拒绝，topology 不变化。

验证：MSVC Debug **ASan 2/2**（enrollment/application、Gateway access delivery），Gateway enrollment 与 Edge
Gateway access 专项均包含**隔离 schema 的真实 PostgreSQL**并通过。测试用只读源码挂载的临时 Compose
容器，结束删除容器与测试 schema；没有修改生产业务记录。App 专项 **3 files / 20 tests** 与 targeted ESLint
通过。未更新手机 APK，未改设备 production 源码/固件，不增加 SDK 版本锁、私有 NimBLE 或新持久状态。

下一步仍是 N3 **同一冻结 App/3.3.11 Hub/child** 的真实加密 grant/receipt/envelope 注入与持久写入核对，
软件 fixture 不替代这一门禁。完成后再进入 N4 force-stop 与 N5 pending restart；N2 双 Hub、D4b 专用加密板和
发布长稳继续独立保留。

#### 10.32 N3 真实加密 enrollment 负测与恢复（2026-09-05）

本批只关闭以下子门禁，**不关闭完整 N3/G2B-4**。App/Service/Broker/Hub/child 全部实际参与，
没有向生产固件或 Service 加入注入接口，没有上传 Admin secret，没有改 wire。

基线：

- Android debug APK 已保留登录态更新。发现缺陷时 APK SHA-256 为
  `a325aaab1a52c2a9c660500a19b03eb8ba6b83bf6dfd9a7e725ffc205cf32c37`；修复后本表三项统一使用
  `f0cff7d7c79381b558cec344ff42b85632cbe7a0624626e5f5ef6d661c736f4a`；
- Hub/COM11 继续使用 §10.30 官方 Core 3.3.11 / IDF 5.5.5 固件；child/COM12 按相同 SDK 显式重新编译烧录，
  保留 NVS/DeviceInstanceId。child `Esp32BleAccessResetProbe` 为 **694,128 B Flash / 34,848 B globals**，
  bin SHA-256 `596e0c1f6024a486383f5bf4c76d3850947198a45473adf4eb3cf00e46507d41`；
- child instance 为 `fb42653e067ea9e4ab3b627db5975846`，不是 MAC 推断身份。内部 probe 新增只读 `s` 查询
  `bl_v2_access/root` 的状态/长度/SHA-256，临时 buffer 清零，不输出 raw NVS 或密钥；本批没有执行 access reset。

真机发现并修正的确定缺陷：设备 enrollment 对无效签名返回内部 AuthenticationRequired 并断开，**合同不要求
发送加密错误回包**。Hub 已通过既有 SelectResult 发布 Disconnected，App 却只在 connect() 阶段消费该状态，
之后忽略断开而等满 receive deadline。第一轮注入据此判为未通过，而不是把超时当作认证拒绝的充分证据。
App `GatewayPermitJoinRecordLink` 现在统一在收/发 relay view 时核对 exact operation/session/selected token，
消费断开并立即停止后续读写；已连接时不能偷偷改变 packet size。不新增协议或重复连接状态查询 API。
新增收中断、发中断、错误 token、变化 packet size 四项回归；相关 App **4 files / 29 tests** 与 ESLint 通过。

| 真实注入 | permit operation（hex） / intent（hex） | 结果 |
|---|---|---|
| P-256 grant signature 单 bit | `20d18722aeff4bf4698ca5d4da6155c4` / `ff478a36cf80053ec02fc1c3235164ab` | 2 次 record 发送，只有 Hello 握手回包；明确 Disconnected，0 Method 2、0 commit；凭据根仍 NotFound。完整扫描/握手/取消约 15,663 ms |
| Noise request AEAD tag 单 bit | `7f3c2d64530a35127be166bd4f0678af` / `4bf8b4407eca4471cf03ac8c073549f0` | 正常签发 grant、真实加密后破坏 tag；明确 Disconnected，0 Method 2、0 commit；凭据根仍 NotFound；整轮约 15,804 ms |
| Admin receipt proof 单 bit | `30c34f6195538219a202bcb15be210bf` / `aada55045434e86a9940484156324135` | 真实 AEAD 解密之后才修改 proof；App `BLE_DIRECT_RECEIPT_MISMATCH`，0 Method 2、0 commit；保留 pending credential/workflow，约 18,267 ms |

前两项的独立 NVS 查询均为 `4354/ESP_ERR_NVS_NOT_FOUND`；Service 相应 intent 由测试端正常 cancel，
`commit_id IS NULL`。原 Detached topology `2a6cd779b076710efc13c7332d90cad1` 仍为 version 175，
`updated_at=1788583665503`，没有误创建/激活新 topology。旧 APK 的首次失败 intent
`31094179fd3670947b2e590888b62410` 也在确认 NVS 为空后用正常 API 取消，未修改数据库绕过状态机。
三项 permit 最终 PostgreSQL/Broker 均 Closed，Broker `attempt_count=1`。

第三项中 child 已真实保存 **260 B** access root，SHA-256 为
`25eeb58549d44b2b5fdbafc399944d5263497c53df1c584b2280ecba328b8e43`，而 Service 仍为 grant_issued/no commit。
这里证明的是 **App 拒绝错误 receipt**，不是 on-air AEAD 拒绝；后者由第二项验证。只读摘要是 durable 内容证据，
不是物理 Flash 写次数计数器，也不能替代 Host 零 replace 调用断言。

随后使用当前 App 正常 recovery，不重置、不发新 grant：

1. 原 pending credential 完成 Method 2 后，原 intent 变 Active；
2. 首次 enrollment→attach 交接发生一次 `BLE_DIRECT_SESSION_CLOSED`，但 exact attach checkpoint 已保存；
3. `finish_attachment` 重试完成 child Gateway install、独立 Gateway credential Method 2，随后 cloud proof 变 Secure；
4. 同一 topology `6b52af6922ff7f289b12eae37e610cc0` 达到 **Active / version 4**，App ready、recoveries=[]；
   `access_delivered_at=1788609857389`、`child_installed_at=1788610131898`、`gateway_proven_at=1788610136891`；
5. 原 Admin commit `83bda09c2db198a81e1b4b1ee5836fb0` exact replay 返回 `replayed=true`；回执改 1 bit 后同 commit
   返回 `DEVICE_V2_BLE_ENROLLMENT_CONFLICT`，intent `updated_at=1788609848644` 两次调用前后不变。

独立 Broker SQLite 核对：该 topology delivery=`acknowledged`、attempt=1、storage revision=1；
proof=`secure`、attempt=3、result=40 B。结束时 App tracked links/sessions 都为 0，临时 client/connect hooks
均已恢复原方法；没有遗留故障注入或待处理 permit window。

结束时 Hub `childState=4/secure=1`、permit/relay 已关闭、无运行错误；heap free 105,440 B、largest 57,332 B、
heap minimum 53,248 B；loop/ipc0/NimBLE 最低水位分别 **2,648/1,112/2,816 B**。Hub 的 Plain-NVS 内部测试
适配器没有变成生产加密存储，D4b 仍独立阻断。

下一批优先级修订：先收口上述**首次 enrollment→attach 的 session 生命周期**，再继续 N3 Gateway receipt/
sealed envelope 篡改、真实 device exact request replay 等未完成项。代码审查发现初次 `attachEnrollment` 仍持有
enrollment session 等待 cloud delivery，而 `resumeAttachment` 已先推进 cloud 再打开 child；应基于证据统一为
同一恢复路径，减少两套编排，补云端等待期间 session 关闭的回归及同 APK 真机首轮成功门禁。现有证据证明
重试恢复可用，**尚不足以确定这次 SESSION_CLOSED 的唯一触发原因，不能声称首次交接问题已修复**。
完成 N3 后再进入 N4 force-stop/N5 组合 restart；N2 双 Hub、D4b、发布长稳依旧保留。

可复现脚本及其清理边界见 `tests/hardware/edge_gateway_enrollment_negative_gate.mjs` 和同目录 README。
测试 hooks 仅存在于单次 debug WebView 内，正常结束恢复；不进入 Arduino 发布包或 App 生产源码。

#### 10.33 首次 enrollment → attach 生命周期收口（2026-09-05）

本批落实 §10.32 的优先修订：**认领会话不是 Gateway 绑定事务的资源**。云端 delivery/proof 等待与短期 GATT
安装必须分离。只改 App coordinator/relay 和回归，不改设备/Service/Broker wire、凭据 schema 或公开 Arduino API。

统一路径如下；首次 enrollment、已认领但未 attach 的恢复、已有 attach checkpoint 的恢复共用同一安装编排：

1. enrollment commit 后先关闭旧 session/permit window；重新核对 account context，回收该 exact Hub 遗留窗口；
2. `beginAttachment` 保存不含秘密的 exact checkpoint，再调用云端 create。响应丢失仍恢复同一 operation；
3. `resumeAttachment` 只以 `advanceCloud` 对账；PendingAccessDelivery/PendingGatewayProof 各至多推进一次，
   仍 pending 就返回等待，**不得调用 child control**；
4. 仅 PendingChildInstall 通过 checkpoint 的 exact Hub 新建 child session，执行 nonce/grant/receipt 校验；
   Admin 安装后关闭，再独立 Gateway Method 2，最后回到纯云端 proof/Active 对账；
5. 新 session 由安装路径 finally 关闭；终态移除 checkpoint。跨账号/epoch 的响应与敏感材料沿用既有拒绝/清零边界。

删除 `connectAndAttach` 和首次 `attachEnrollment` 的重复安装编排，移除 `resumeAttachment` 多余 GET 与 generic
`cloud.resume` 回退。后者存在确定竞态：第一次推进仍为 PendingAccessDelivery、第二次推进变 PendingChildInstall
时会使用传入的手机 Direct controller，绕开 exact Hub transport。现在 cloud-only 方法不可能执行安装。
`EdgeGatewayAttachRelay.resume` 仍保留直接调用方所需的安装职责，但不再对已 pending 的 proof 连续推进两次。

回归先在旧代码复现两项失败：首次云端 mutation 前旧 session 未关闭，以及二次 cloud resume 错用 child controller。
修订后定向 **4 files / 38 tests**、扩展 BLE client/session/account 回归 **7 files / 62 tests**、targeted ESLint、
local build、Capacitor copy 与 Android assemble 均通过。测试间 prototype spy 统一恢复，避免用前一用例替身污染
下一用例。手机保留登录态安装新 APK，SHA-256：
`27c9e324dc736b8642367bc74e40b903a6db5dfff8ea646c509064713c0fed74`。
Hub/child 继续使用 §10.30/10.32 的官方 Core 3.3.11 / IDF 5.5.5 固件，未重编译或刷写。

真实首轮门禁准备与边界：

- 正常 owner detach 将上一轮 topology `6b52af6922ff7f289b12eae37e610cc0` 收敛为 Detached/version 5；
  随后仅对 COM12 测试 child 执行 `resetAccess()`，保留固件与 DeviceInstanceId；只读 witness 返回 4354。
  这是重建“首次认领”前提，不是清除 pending recovery 来掩盖失败；旧 access 已由本轮正常 enrollment 重建；
- 实际 Android 在 exact Hub 页面扫描并选择 provisioning locator `d6887b5b42c296b4`，签发前校验 child instance
  `fb42653e067ea9e4ab3b627db5975846`。调用正常 `selectCandidate/continueWaiting`，失败则停；没有修改协议帧、
  grant、receipt、数据库或权限，也未发明新的 retry operation；
- 新 intent `33865b72b4fc2f03706872c7a9abff0f` Active；新 topology
  `8278fe212f2c3dc4804ae360bbbbc267` **Active/version 4**。

同一 App 时钟观察到 enrollment session 于 `1788612967173` 关闭，首次 create 于 `1788612967264` 开始，
当时 tracked links/sessions 均为 0；安装使用与 enrollment 不同的 fresh/ready session。整轮 **89,430 ms**，
页面仅经历正常 waiting → ready，无 failed/retry。该时间含 enrollment、云端 delivery、两次身份证明与 proof
等待，不是 BLE 单次建连耗时，也不作为性能优化完成的指标。

独立持久化核对：PostgreSQL `access_delivered_at=1788612962591`、`child_installed_at=1788613012114`、
`gateway_proven_at=1788613016978`；Broker delivery acknowledged/attempt=1/storage revision=1，proof secure/
attempt=3/result=40 B。手机和服务器时钟不同，不能混用上述绝对时间推导跨端因果顺序。
结束时该 Hub 无活动 permit operation，App ready、recoveries=0、links=0、sessions=0；五处仅用于观测/身份
保护的临时 WebView hook 均与原 prototype 方法一致，未遗留注入。

Hub 结束只读资源：childState=4/secure=1，运行/management/route/relay error 均为 0，permit/relay 关闭；
heap free **105,448 B**、minimum **52,920 B**、largest **55,284 B**，loop/ipc0/NimBLE 最低水位
**2,296/1,112/2,768 B**。这是本轮资源快照，不替代长期水位趋势或发布长稳。

本批关闭“首次交接使用统一 cloud-first/session 路径并真机首轮完成”门禁；不能回溯声称 §10.32 的一次
SESSION_CLOSED 已找到唯一底层触发原因，也不关闭完整 N3/G2B-4。下一批继续 Gateway receipt/sealed envelope
篡改及真实 device exact-request replay，之后才进入 N4 force-stop/N5 pending 组合 restart；N2 双独立 Hub、
D4b 专用加密板及发布长稳仍独立保留。内部 Hub Plain-NVS 适配器未获得生产安全准入。

#### 10.34 Gateway receipt 与 ControllerMutation 重放真机门禁（2026-09-05）

本批按 §10.33 继续 N3，但**只关闭回执/ControllerMutation 以下子项**。设备、App、Service/Broker 生产源码与
wire 均未改；沿用 §10.33 的 APK 和官方 Core 3.3.11 双 XIAO 固件。新增独立 debug WebView 门禁脚本
`tests/hardware/edge_gateway_attachment_negative_gate.mjs`，不把故障注入放入产品 API、固件或服务器。

准备：正常 owner detach 上一轮 `8278fe212f2c3dc4804ae360bbbbc267`，Service 收敛 Detached/version 5 后才用正常
App create 建立新 topology `d01103b0b3880397268c139a492785fd`。**本轮没有 resetAccess、刷写或清除 Admin**，
没有新 enrollment intent；child 仍为 `ble_8Zc1CXfpwQtN1NqFWR6BvQ`，instance
`fb42653e067ea9e4ab3b627db5975846`。交付 ACK 后 state=PendingChildInstall/version 2。

测试先验证 exact Hub/child/instance/operation/checkpoint，控制链路始终是 App→Service/Broker→Hub→child GATT。
故障注入位于真实 DirectSecure 解密、receipt decode **之后**，所以这里检验 HMAC/绑定校验，不冒充 on-air AEAD
tag 负测。以同一错误 receipt 单独调用真实 Service 接口，确认服务器也拒绝，不靠 App 提前拒绝来推断云端行为。
该次有意提交错误回执属于测试请求；下表“0 commit”指生产 App relay 没有越过校验发起正常 confirmReceipt。

| 冻结组合上的通过项 | permit operation（hex） | 结果 |
|---|---|---|
| Gateway receipt proof 单 bit | `8f233a0a672d41f17e716ec6f17de39f` | App `EDGE_GATEWAY_RECEIPT_CONTEXT_MISMATCH`；Service HTTP 409 / `EDGE_GATEWAY_RECEIPT_MISMATCH`；0 Gateway Method 2、0 正常 commit，checkpoint 保留；28,170 ms |
| 同 BBP/2 ControllerMutation 重放、同 sequence 改 secret | `aceefd6c7321f7666c5ac0f06f1ef0d2` | exact kind/flags/sequence/body 返回逐 byte 相同 receipt；secret 改 1 bit 返回 wire AuthenticationRequired(2)。随后坏 receipt 仍被 App/Service 拒绝，0 Method 2/正常 commit；39,424 ms |

第二项保持 **BBP/2 请求 bytes 相同、DirectSecure 外层计数器使用新值**，不是复用密文或 AES-GCM nonce。
ControllerMutation 的持久幂等归属是 signed grant/controller/version/secret 与设备槽内容，不是建立永久 BBP/2
sequence journal；它也不证明任意“同 sequence、不同但另获合法签名的操作”都必须冲突。Enrollment requestId、
MQTT QoS replay、DirectSecure 密文防重放是另外的边界，不因本项通过而全部关闭。脚本 `installs` 统计成功收到
mutation receipt 的 API 次数，**不是 Flash 写次数**。

持久化证据：

- detach 后 child root 260 B，SHA-256 `43502984dcee18de1aad4a3ef9f2c50b5939d7472076b089bb2c5222a990fc5a`；
  首次真实 Gateway install 后为 `390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`；
- 后续 receipt 负测、exact/changed-request 重放、正常恢复前后均保持后一个摘要。只读 NVS witness 不证明物理
  Flash 写计数；因此补充 `DeviceAccessStore` Host 断言：满三槽时 exact Gateway install、重建 store 后 exact
  install 都不增加 `replaceCalls`；同 id/version 改 secret 返回 StateConflict、内容与写次数均不变；
- 负测后 Service 一直为 state 3/version 2、`updated_at=1788614835131`，没有 child receipt digest/proof；
  说明拒绝不提前更新权威绑定状态，而不是要求已经完成的 child 安装回滚。

随后恢复原 checkpoint，不 reset、不 cancel、不替换 operation：fresh Admin nonce/grant→幂等安装→独立 Gateway
Method 2→Hub proof，**52,726 ms** 内 state 序列 `4→4→4→5`，最终 Active/version 4。
PostgreSQL `child_installed_at=1788615327892`、`gateway_proven_at=1788615332648`；Broker delivery acknowledged/
attempt=1/storage revision=1，proof secure/attempt=3/result=40 B。App recoveries/links/sessions 均为 0，六个本批
permit operation 均 Closed，临时 control/confirm/read hooks 已恢复原方法。结束 Hub secure=1，运行/route/
management/relay 无错误；heap free/min/largest 为 **105,300/52,920/45,044 B**，loop/ipc0/NimBLE 水位
**2,296/1,112/2,768 B**。largest block 低于上一批快照，单点差异不能判定泄漏或证明无碎片，后续需趋势测量。

如实保留未通过的前置尝试：第一轮 `69d459830e282a40572c04ac82a6c78e` 在任何 mutation 前 CONNECT_TIMEOUT
（16,583 ms）；窗口已 Closed、NVS 未变化。该次没有足够的 SelectResult 观测确认唯一原因，不能用后续成功
抹去，须保留为连接稳定性问题。第二轮 `20cf3f700b1aa9f21604114b3b0ea8cd` 已安装，但测试脚本错误地按
HttpErrorResponse 读取错误，而 App 已统一抛 GatewayHttpError；修正读取 `code/httpStatus` 后才得到上表有效结果。
不改生产错误合同来迁就测试。修正后两轮均记录真实 Connecting→Connected SelectResult。

软件验证：新增硬件脚本通过 Node syntax check；重新构建 MSVC Debug ASan，device access store/controller
control/Gateway access delivery/child session/child route bridge **5/5** 通过。首次 CTest 缺少 ASan DLL 导致
0xc0000135，在当前命令 PATH 加入已安装 MSVC runtime 后重跑通过，没有复制 DLL 入库或关闭 sanitizer。

下一步仍是 N3：先补低侵入 SelectResult/SDK connect-discovery 时序观测，复现上述连接超时（不提高 timeout
或无限重试来掩盖）；继续 Hub sealed envelope 的真实 tag/context/expiry/replay 与 durable ACK 门禁，以及
enrollment requestId 的真实设备级 replay。然后进入 N4 force-stop/N5 pending restart。N2 双 Hub、D4b 和发布
长稳继续独立保留；本轮不能把回执篡改或 Host envelope 测试称为真实 Hub envelope 负测完成。

#### 10.35 当前 Gateway 架构复核（2026-09-05）

基于当前实现与 §10.33～10.34 证据，**核心分层合理，暂不整体重写或增加第二套协议**；可复用的是职责与窄
接口，不是要求所有无线标准套用同一认证和 packet 格式。当前检查结论与约束如下：

| 边界 | 当前实现/职责 | 必须保留的约束 |
|---|---|---|
| Service 授权与权威状态 | `edge-gateway-access-service` + repository/CAS/KMS | owner/tenant/instance/epoch 决定授权；App 的 Method 2 标志不是独立硬件证明，Active 仍需真实 Hub proof；不把扫描、HTTP 2xx 或 receipt 单独当 Active |
| Broker 交付与路由 | ciphertext delivery/proof ledger、permit mailbox、Active route projection | 耐久 ACK/重发与临时 opaque relay 分离；不维护第二份 owner 权威，不用 mailbox 代替 topology journal |
| App 事务与短会话 | `DeviceV2EdgeGatewayService` + `EdgeGatewayAttachRelay` + account-scoped checkpoints | 只在需安装时取得 exact Hub session；保留成功事实、拒绝错误回执、恢复同 operation；不能因失败直接 reset/cancel 已安装材料 |
| Hub 可移植运行时 | `BasicEdgeHubLifecycle`、delivery processor、proof/revoke/renewal、`BasicGatewayChildSession<Link, Selector>` | 不依赖手机 UI、MAC 身份或具体 ESP32 API；通过接口获得时钟/存储/密码学/南向 link；WiFi session refresh 不清除持久 topology/child 身份 |
| 平台与子设备 | `Esp32EdgeHubProduct` 组装 WiFi product、Central port、radio lease；child `DeviceAccessStore` | composition 只装配资源与生命周期；入站 Admin/Shared/Gateway 三槽不复用为 Hub 出站 child credential store，未选功能不占普通产品空间 |

保持 §10.0 的两条扩展路径：ESP-NOW/串口 NCP 若仍承载 Blinker native records，新增 Link/Selector/permit adapter，
复用安全会话与 route bridge；Zigbee/BLE Mesh 则实现自己的 commissioning/security/model adapter，只复用北向
逻辑身份、ACL、状态/动作语义。**不承诺后者可以零协议适配**：现有 native credential/envelope profile 不应强套
ZCL/Mesh 密钥模型，未来需独立冻结其 profile/admission，而非在 GATT 核心加入各种 `if(mcu/protocol)`。

当前不合理的是把“单 child 测试通过”扩大成通用网关已发布，或把连接等待当成密码学问题去改 SDK/超时。
当前明确仍是单 child、一个活动 southbound session；多 child 调度、容量表、Mesh/roaming 不在本批实施。
长 HTTP 往返属于 permit/enrollment 控制面，其耗时不应被误称为稳定数据面的 MQTT 转发吞吐；既不因此复制一套
App MQTT 管理协议，也不应把高频 telemetry 放入 permit relay。先完成失败路径、资源趋势和安全存储门禁。

#### 10.36 选中 ACL 的失败边界与连接短循环（2026-09-05）

按 §10.34 的前置待办，本批先处理 CONNECT_TIMEOUT，没有进入下一项 sealed-envelope 注入。
新增 `tests/hardware/edge_gateway_connection_gate.mjs`：使用真实 App 的 exact Hub permit link，按已存 child
instance/授权 locator 选择目标，执行 Method 2 与同步，关闭后核对 tracked links/sessions。不认领、不 detach、
不写 credential，不改变 child access；每组首错即停，输出只包含公开 SelectResult 和阶段耗时。

**确认并修正的逻辑缺陷，与尚未确认的射频/SDK 原因分开记录：**

- 后续无 trace 复测定位到本次交接超时的直接原因：候选表按插入顺序返回，旧 locator 在 TTL 内排在持续观测的
  新 locator 前。两者都可通过同一 child 的 PresenceKey 验证，鉴权成功不等于旧广播仍可连接。
  `readRelayCandidate()` 改为按现有 `lastSeenMillis` 的新鲜度排序（相同时间按槽号稳定排序、支持 millis 回绕），
  不用 RSSI/MAC 代替身份、不新增存储/接口/wire 字段，也不偷偷将选中的 token 改成另一个。4 槽有界排序由 Hub
  adapter 承担；App 仍复用现有授权 matcher。更晚发生的正常轮换/真实断连仍按失败窗口处理，不能无界重试。
- 旧 NativeGattPermitJoinAdapter 在选中候选后收到断连（或 poll 观察到端口 Idle）会自动 rescan；连接从未 Ready
  时 `false→false` 也不会生成新的 SelectResult。更严重的是，已开始的 opaque Noise/DirectSecure 分片不能在
  新 ACL 上接着传。因此选中的连接尝试失败、已连接 ACL 丢失、sendPacket 返回 NotConnected 均结束窗口；
  利用既有 adapter Fault→coordinator Rejected→释放 radio lease，不增加 wire 状态、平台接口或重试层。
- WouldBlock 只表示同一 ACL 的背压；未选中时扫描可恢复。exact Open replay 仍返回该窗口终态，新 operation
  才可重新打开；不扩大为取消账户 enrollment/attachment checkpoint，凭据事务与临时链路继续解耦。
- App `disconnect()` 原先只接受 Closed/Expired，遇到已结束的 Rejected/Busy/Unsupported 会掩盖原错误并留下
  无效 permit checkpoint。现在与已有 recovery 路径一致，确认 exact window 的任一合法终态后清理；网络请求
  失败、未确认终态时仍保留恢复记录。此项仅改变清理，不把拒绝转为接入成功，不影响 topology 权威状态。
- 清除临时 Central Serial trace 与旧 Host 中“NotConnected 后透明重连”的断言/开关；没有新增常驻缓冲或字段。

复测前提：Hub COM11 / `8c:bf:ea:1b:96:14`，child COM12 / instance
`fb42653e067ea9e4ab3b627db5975846`，既有 topology `d01103b0b3880397268c139a492785fd`。
使用隔离 Core 3.3.11 / IDF 5.5.5 配置，不使用默认 CLI；只重新烧录 Hub app/既有分区，不擦 NVS。
手机沿用 §10.33 APK（本批 App 清理改动尚未安装）；child 沿用 §10.34 固件与 access root。

| 阶段 | 证据 | 判定范围 |
|---|---|---|
| 旧逻辑基线 | operation `3dad2f1745ea7b6bf1c8ed8320717196`，1.748 s 开始 connect，16.793 s CONNECT_TIMEOUT，仅 Connecting | 首轮失败，不重试冒充首轮成功 |
| 原生诊断第一轮 | `a931e24ecbee40159f9973453fe7330d`，2.455 s 开始 connect，17.495 s 同样超时；串口未捕获 connect/discovery 事件 | 未建立唯一原生原因，不能据此要求更换 SDK |
| 修正后诊断固件 | 首轮 `9a3e76e675e9463e5b12e5920385fb4e`，Connected 3.825 s，全流程 21.185 s | 原生 connect 193 ms、discovery 1,199 ms；正常成功 |
| 随后连续 3 轮 | Connected 为 3.051/2.972/3.245 s，全流程 19.615/18.966/18.543 s；每轮 links/sessions=0 | 原生建链 130～221 ms、discovery 999～1,099 ms，未以 HTTP 全流程耗时冒充 BLE 延迟 |
| Host 最终源码 | MSVC Debug ASan **44/44** | 含 selected Connecting/Ready × 有/无 disconnect callback、NotConnected 写失败、同 ACL WouldBlock、新窗口恢复 |
| App 最终源码 | Angular 正式测试入口 **8 files / 48 tests** + ESLint | 含三种新增终态清理、重复 close、网络错误保留；直接调用 Vitest 缺 Angular 测试初始化不作为有效门禁 |

诊断后三轮 Hub heap free/min/largest = **105,440/91,304/63,476 B**，loop/ipc0/NimBLE minimum =
**3,684/1,112/2,724 B**。仅失败边界修正的无 trace 中间版 Hub 编译 **1,432,712 B Flash / 65,332 B globals**，比原基线
Flash 增 140 B、globals 不变；SHA-256 `c361e731ef32dee1c33ff8a5eb97c704cd0efa2247864a67248b244c1d9cb657`。
诊断固件结果不能替代去除 trace 后的门禁；短循环也不能宣称原超时唯一根因或资源长期稳定已得到证明。

无 trace 中间版 10 轮门禁在第 1 轮 `5e1f15ad9fe21ae7a5e2f9b512520cae` 再次 CONNECT_TIMEOUT（16.778 s），
立即停止，**不是 10/10 通过**。不再归因于已修正的 selected-disconnect。增加仅公开 CandidateSnapshot 观测、
保持固件不变并只重启 Hub 后，operation `7b3d6416a3ba43ded0f53e860b9ea17c` 在 1.246 s 返回两个候选：
`d798537da0c96375`（先插入、RSSI 不再更新）和 `85c0b67063f61f9c`（持续更新）。App 选中前者，只收到
Connecting；10.854 s 前者从 TTL 表消失，剩余后者。这两个公开 locator 经 App 原有 matcher **均匹配同一
child**，检查过程没有连接/导出密钥/创建新窗口。由此定位“旧缓存优先”问题，而非假定两个 MAC/两块板。
保留该轮失败与证据；不能用前四次成功覆盖它，也不能把一次 locator 竞态扩大为所有 SDK 故障的唯一原因。

加入新鲜度排序后的最终无 trace Hub：**1,432,832 B Flash / 65,332 B globals**，相对原基线 Flash +260 B、
globals 不变；SHA-256 `779ffb3648d0016d17cf312c20ad601531999b0ee58033fec2ff8e5cd5aea3f7`。
Host 全量 Debug ASan 重建 **44/44**，新增固定 4 槽排序/回绕/刷新/同时间稳定顺序/TTL 断言。最初测试循环
initializer-list 的 uint8 窄化警告由显式 uint8 数组修正，没有关闭 warnings-as-errors。

最终无 trace 固件从 Hub 已有 Secure child session 开始交接，**连续 10/10** 完成真实 App→Service→Broker→Hub→
child 认证/同步/关闭；总计 **204,494 ms**。每轮 Connected（相对开窗前）为
`4153/3241/4249/3252/3309/4241/3492/3931/3232/3450 ms`，整轮为
`19881/20123/20786/20337/19929/20705/21421/21052/20702/19558 ms`。这些包含 HTTP 控制面等待，不是纯 BLE
radio latency。首窗口 `b82df2f6d769921a44be56a97824c09f`，末窗口 `b4f0eeff665909ced35929f0c973e827`。
该轮首次 snapshot 只有一个 locator；新旧共存排序已由 Host 矩阵锁定，后续仍可补定向共存交接，不伪称这 10 轮
每次都复现了双 locator。它是连接短循环通过，不是所有轮换竞态或原生错误均被消除。

PostgreSQL 十个窗口均 Closed，原 topology **Active(5)/version 4/updated_at=1788615332648** 未变化；child
NVS 260 B SHA-256 仍为 `390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`。
App ready、busy=false、workflow=false、links/sessions/permit checkpoints 均 0，readRelay observer 恢复。
最后 Hub heap free/min/largest **105,448/92,944/63,476 B**，loop/ipc0/NimBLE minimum
**3,684/1,112/2,812 B**，permit/relay/control 关闭；未做新的 credential mutation、access reset、commit 或 push。

另如实记录：循环期间多次出现 `managementError=7 (InvalidEncoding)`，关闭后为 0；本批未确认其消息来源，
不能因连接成功将其标记已解决或隐去。下一批应检查 management/control/relay 消息分类与错误归属，不先改超时。

本批后续顺序：安装 App 终态清理改动、完成真实选中 ACL 丢失的终态/新窗口恢复，并核对上述 managementError；
如需再现 locator 问题，记录新旧共存 snapshot/选择/native connect-discovery，首错即停。再回到 N3 Hub sealed envelope 的 tag/context/expiry/replay/durable
ACK 和 enrollment requestId device replay，之后 N4/N5。N2 双 Hub、D4b encrypted-NVS 与发布长稳继续独立阻断。
不扩展 Mesh/多 child/History，不将本批正确失败边界等同于全部 G2B-4 完成。

#### 10.37 MQTT 异步缓冲所有权与真实 ACL 拒绝恢复（2026-09-05）

本批完成 §10.36 的前置待办，不扩展管理协议、MCU 缓冲、重试层或 SDK 绑定。

**`managementError=7` 已定位到 Broker，而不是设备 demux/NimBLE：**

1. exact Hub 诊断轮 `a6838c96a969b97e358e154ed6836af6` 正常完成 20,749 ms，但管理端收到 29 B Select、
   23 B ACK 及若干 batch，其头部全为 `000000`，因此返回 InvalidEncoding。诊断只记录长度、3-byte 头与错误，
   没有输出消息内容或凭据；随后彻底移除临时 Arduino/Serial trace。
2. 本地 Aedes `Packet` 保留原 `payload` Buffer 引用。Broker `publishToHub()` 将请求 scratch 或 mailbox
   Buffer 交给异步 QoS1 发送，而 `submitPermitJoinRelayDown()` 的 finally 或 mailbox 更新/回收立即清零。
   因此首包被破坏，后续应用层重传可能成功；“流程可完成”不能证明首包正确。旧 Broker 测试同步深拷贝消息，
   掩盖了该所有权错误。它也会增加控制面等待，但不能据此解释所有历史连接延迟。
3. 修正仅在 `publishToHub()` 交界执行 `Buffer.from(payload)`，将独立 Buffer 所有权交给 Aedes；请求和
   mailbox 继续清零自身存储，不修改已经交付网络队列的数据。MQTT callback 不是所有引用均已释放的证明，
   不在该回调再清零发送副本。secret 仍由 Noise/DirectSecure/envelope 等上层安全边界保护，未增加 raw secret
   持久化。此修复不依赖 BLE，适用于该管理 relay 的其他适配器。
4. Broker 测试保留真实引用而不提前复制：旧镜像在 Select 清理后精确复现全零失败；新实现验证 Select、
   DownBatch、UpAck、duplicate upstream 的 exact ACK，以及 mailbox shutdown 后已排队消息不被更改。
   Linux Broker Device V2 suite 通过。Host 同时锁定合法三类 relay 不进入 access delivery、零串仍报
   InvalidEncoding/零持久 replace，**没有通过忽略错误来“修好”设备**。

**最终冻结组合与软件门禁：**

- Hub 恢复无 trace 固件，SHA-256 `779ffb3648d0016d17cf312c20ad601531999b0ee58033fec2ff8e5cd5aea3f7`，
  Core 3.3.11、Flash **1,432,832 B**、globals **65,332 B**，与 §10.36 最终基线完全相同。
- 新 App 已安装并保留登录态，包含 §10.36 终态清理；APK SHA-256
  `e9ad22f1707472f6eba624003e44cb522531ba2a0fd2f2bbda43bd636dcd7a95`。
  Angular local、Capacitor copy、Android assembleDebug 通过；未新增 App 生产 API/UI。
- 本机 exact Hub 所属 shard-1 已更新且 healthy，镜像
  `sha256:0543353da9c76aaaa21a3d4672ae4f31a17d8e1cebfcbdeabab68742769a936a`。
  其他 shard 没有在本批滚动更新，不声称整个部署已升级；不删除数据卷/修改业务数据库。
- Host 全量重建 Debug ASan **44/44**；App Angular **8 files / 48 tests**；Broker TypeScript 和 Docker
  workspace build 通过。child 沿用 §10.34 固件与 access root，不重新烧录、不 resetAccess、不 detach/re-enroll。

**真实 selected ACL 失败 → 新窗口恢复：**扩展已有 `edge_gateway_connection_gate.mjs`，用可选
`--fault selected-acl --count 2` 在首轮 Method 2/同步成功后，经正常 fragment/opaque relay 发送一个无效应用
record。真实 child 按既有安全规则关闭已认证 BLE peer；测试没有伪造 GAP callback、修改 Service 状态或绕过
App credential。独立 GET 必须确认 Rejected，再验证 close/checkpoint 清理和不同 operation 的正常恢复。

| 轮次 | exact operation | 结果 |
|---|---|---|
| 真实拒绝 | `99e5e78cbecc6e53fd06a7d2c30200fd` | Connected 2,604 ms；13,737 ms 注入；14,133 ms 确认 Rejected（396 ms）；14,198 ms 清理完成 |
| fresh recovery | `ca2febd9ae013cfd8daf43d85f7e631e` | Connected 2,598 ms；12,483 ms Method 2/同步；13,231 ms 关闭完成 |

每轮 App links/sessions/permit checkpoints 均 **0**。PostgreSQL 分别 **Rejected/Closed**；原 topology
`d01103b0b3880397268c139a492785fd` 保持 **Active(5)/version 4/updated_at=1788615332648**。
观察串口 `managementError=0`，没有此前 state error=7 转换。此项是 child 主动拒绝产生的真实 ACL 断连，
不是射频断电、App force-stop 或完整 N3 ciphertext/tag/replay 矩阵；不以它替代其他负测。

**后续正常短循环没有全过，须保留失败证据：**本批同一最终组合执行 10 轮，前 9 轮成功，末轮
`e09e32312db5a5eab578b3b304892c40` 在 Connected **2,607 ms** 之后、`connectUsing()` 完成之前失败，
**21,314 ms** 完成清理。前九轮总耗时分别为
`13711/14064/13535/13008/13714/13727/13242/14424/13275 ms`；含失败轮总计 **144,014 ms**。
这不是 10/10，也不能与此前全成功组作等价性能比较。首次 operation 为
`67d96048a9eceb66fb4e4840aa9bcc56`；所有十轮 PostgreSQL 最终 Closed、App links/sessions/checkpoints=0。
Closed 只证明窗口清理，不证明该轮认证/同步成功。

全程无此前 `managementError=7` 状态转换，末轮出现独立 **relayError=17 (SequenceConflict)**；当时
relayDown=14、relayUp=20、relayWaitingUpAck=0。Host 中 `processUpAck()` 对已确认/延迟 ACK 的严格检查
是下一步核对点，但尚未捕获 exact ACK/sequence，因此不将其直接认定为末轮失败的唯一根因。
旧测试错误白名单只接受字符串 code，使未覆盖的静态错误/numeric BBP2/HTTP 错误变为 `UNEXPECTED_FAILURE`；
已补安全归类（固定消息映射、numeric wire code、HTTP status/规范 error code，不输出原 payload/任意错误内容），
语法检查及五项白名单断言通过。不能反向推断旧失败的原始错误，也未重跑覆盖该失败。

末尾 Hub heap free/min/largest **105,432/87,100/61,428 B**；loop/ipc0/NimBLE minimum
**3,684/1,112/2,816 B**，窗口/relay 已关闭。child 只执行 `s` 读取摘要：260 B SHA-256 仍为
`390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`。摘要相同仅证明内容一致，不冒充
Flash 写入计数为零；没有 resetAccess、重认领或业务 DB 修改。App idle、busy=false、workflow=false。

**修订后的下一批顺序：**

1. 在同一冻结组合上记录第十轮类型失败的静态错误/HTTP status、公开 relay direction/sequence/ACK status；
   不记录 opaque 密文/secret、不提高 timeout、不取消安全失败。
2. 对照 Hub `processUpAck`、Broker duplicate upstream/ACK 和 App serialized request，补
   exact duplicate、前一 ACK 晚到下一 pending batch、wrong operation/direction/future sequence 的矩阵。
   先证明需修之处再改最小状态机；重复 ACK 不应破坏当前 batch，错上下文不能因此被接受。
3. 回归与真实失败恢复通过后重做 10 轮，不能只补第十轮或将多组成功拼接成一组。然后回到 N3 Hub sealed
   envelope 的 tag/context/expiry/replay/durable ACK 与 enrollment requestId device replay，再进入 N4/N5。

N2 双 Hub、D4b encrypted-NVS、30～60 min/24 h 发布长稳继续独立保留。当前所有权修正无需重写网关：
App 负责凭据编排，Service/CAS 是权威，Broker 管理投递，Hub 南向 adapter 负责 radio/session；保持现有
单 child 范围，不增加 Mesh/多 child/History。本批未 commit/push，也没有新增暂存。

#### 10.38 ACK 幂等边界与新组合短循环（2026-09-05～06）

按 §10.37 先核对 ACK，不直接扩大超时或另加重试器。确认并修正两个可复现的状态机缺陷：

- **Hub：**原 `processUpAck()` 在 ACK 已消费、或下一批已 pending 时，将前一 exact ACK 判为
  SequenceConflict。MQTT 重投与 Broker 对 duplicate upstream 的重 ACK 都可合法产生它。
  现在只接受当前 operation、Up 方向、最近一次已确认的相同 sequence **和 status**；不清理/推进当前新批次。
  sequence 由现有 `nextUpSequence_` 和 `upstreamPending_` 推导，仅保留一个 uint8 status，不另建历史队列。
  原 Rejected 的重复仍是 ProtocolError，不把失败改成成功；改 status、错上下文、未来或无历史序号仍拒绝。
- **Broker：**原 App ACK 必须匹配 upstreamPending，第一次处理成功但 HTTP 响应丢失时，原样重试会返回
  `EDGE_GATEWAY_PERMIT_JOIN_RELAY_MISMATCH`。现在在校验 exact operation/direction 后复用已有 lastUpAck
  做 exact bytes 比较，只重投既有 ACK 并返回最新 view，不更新 revision、不消费下一 pending batch。
  不增加 mailbox 字段，继续遵守 §10.37 的独立发送 Buffer 所有权。
- **App：**保留现有 serialized request、duplicate upstream 重 ACK 与 record 去重，未新增生产 API/UI/
  自动重试层。故障注入只在 Host/硬件测试，正式设备没有临时 Serial trace、特殊 SDK 开关或绕过鉴权入口。

| 边界 | 回归结果 |
|---|---|
| exact ACK 再到、下一 batch pending 后旧 ACK 晚到 | 旧 Host/旧 Broker 分别明确失败；新实现幂等且保留当前 batch |
| wrong operation/direction、future sequence、改变已确认 status | 拒绝；当前 pending 不变、无额外投递/存储 |
| Hub Rejected 原样重放、Rejected 改 Accepted、runtime reset 后旧 ACK | 分别保持 ProtocolError、拒绝变化、拒绝无历史 ACK |
| 验证范围 | Host 全量 Debug ASan **44/44**；Linux Broker Device V2 suite；App permit link + Direct session **2 files / 25 tests**；TypeScript/Docker workspace build |

**冻结组合：**Hub COM11 / `8c:bf:ea:1b:96:14` 使用 Core 3.3.11，Flash **1,432,852 B**、globals
**65,332 B**（相对 §10.37 **+20 B / +0 B**）；SHA-256
`cb31fb6a4973e2810d5c86347f9da2f23f0c1224eb5bcb96af6e7f48cc4aa67e`。
App 沿用 §10.37 APK `e9ad22f1707472f6eba624003e44cb522531ba2a0fd2f2bbda43bd636dcd7a95`。
本机 shard-1 healthy，镜像 `sha256:cdf3cef9e15213b05d9e21b88b177af995e09f5abd81c31d3eb3f262625d2d23`；
其他 shard 未在本批升级。child COM12 固件未改，没有重新认领、resetAccess、detach 或改业务数据库。

**先如实记录独立的扫描失败：**原组合 operation `e32e7874d0f26935c28966bfe66c34ee` 在 21,503 ms
SCAN_TIMEOUT，首次 snapshot 为空；新 Hub 固件/重启后 `c39de0ad9f126ba3899385cc36ab8b37` 同样空候选，
16,477 ms 超时，均未进入 ACK 阶段、均正确 Closed/App 清理。child 串口可读且 NVS 摘要未变。
随后使用 esptool read-mac 核对 exact child `98:3d:ae:ee:7c:6c` 并 hard-reset（不擦写 Flash/NVS），广播
和连接恢复。因此仅证明 child 运行态复位可恢复本次现场，不证明停发根因、自动恢复或长期稳定已解决。
本轮没有捕获 §10.37 末轮的原始同步错误，不能将下面两个已复现的 ACK 缺陷称为该历史失败的唯一原因。

**真实 ACK 注入：**既有 `edge_gateway_connection_gate.mjs --fault duplicate-ack --count 2` 在每个上行
ACK 的 HTTP 处理成功后，向同一 operation 再发 exact ACK；不重放/解析 Noise 或 DirectSecure record。
`e933d58c9208fb0bd41b176063a94031`、`cacd09268aceba615206f77db71362f5` 各重复 **20 个 ACK**，
分别 **15,127/13,582 ms** 完成认证/同步/关闭，PostgreSQL 均 Closed。Hub managementError/relayError 均 0。
测试追加有界 32 条公开 kind/direction/sequence/status 观测（仅失败输出），不保留 opaque packet 内容；
语法与边界/去重/容量白名单断言通过，finally 恢复 read/send hooks。

**独立正常循环 10/10：**没有在轮间 reset/re-enroll/retry，也不拼接前面的失败组。每轮 Connected（含
HTTP 控制面，不是纯 radio latency）为 `3778/2482/2755/2544/2664/2513/2534/2671/2760/2739 ms`；
整轮 `14778/12882/14835/13564/12571/13799/13314/13044/13917/13280 ms`，总计 **135,984 ms**。
首 operation `f3d69b8f1cc1768db5a8d2d69a46b8fd`，末 operation `f28eb985a90141b9c6df5816a95feb1f`。
十个窗口均由独立 PostgreSQL 查询确认 Closed；每轮 App links/sessions/permit checkpoints=0，结束 idle。

原 topology `d01103b0b3880397268c139a492785fd` 仍 **Active(5)/version 4/updated_at=1788615332648**；
child access root 260 B SHA-256 仍为 `390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`。
串口观察 managementError/relayError=0，最后 heap free/min/largest **105,476/87,900/63,476 B**，
loop/ipc0/NimBLE minimum **3,684/1,112/2,812 B**。此资源数据属于重启后的新基线，不与复位前水位混合。

**后续执行边界：**ACK 幂等与本轮短循环子门禁关闭；先为上述 child 空广播补低侵入运行态/连接回收观测，
不以多次复位代替修复、不改 MAC 身份规则或全局 SDK。广播正常的冻结组合仍可继续 N3 Hub envelope 的
tag/context/expiry/replay/durable ACK 与 enrollment requestId device replay，但不能同时宣布 BLE 发布稳定。
之后 N4 force-stop/N5 组合 restart；N2 双 Hub、D4b encrypted-NVS 与工程/发布长稳仍单独保留。
App/Service 权威、Broker 投递、Hub relay 与 native adapter 边界不变，无需重写网关或扩展 Mesh/多 child。
本批未新增暂存、commit 或 push。

#### 10.39 ESP32 child 原生连接回收边界（2026-09-06）

承接 §10.38，先补能够在实际适配器上确定复现的状态机测试，不增加 SDK fork、全局重启器或云端重试层。
`Esp32NimBleLink` 同时供 ESP32 BLE/WiFi+BLE peripheral 使用；此次不改 Hub Central、平台装配、鉴权或 wire。

- **indication 错误作用域：**旧实现将当前 peer 的 `NOTIFY_TX` 非零错误（非 EDONE）写入 host fatal，
  下一 poll 提前退出，后续 GAP disconnect 无法消费、广播无法恢复。新实现只标记该 native handle 待关闭，
  阻止其继续收发，一次 poll 至多执行一次失败连接终止请求，实际 GAP 回收后再广播；不重建 Host。
  状态 0 仅代表已发送，EDONE 才释放 indication 槽；失败后的晚到 EDONE 不得重新打开数据流。
- **跨任务交接：**旧实现先清 `connectPending_`，解锁后才发布 `connectionHandle_`，另一 CPU 在两步间
  发出的 disconnect 看不到任何归属而被忽略。现在在同一个短临界区交接 native handle 与 pending 标志；
  SDK 调用与业务回调都留在临界区外。回收时先清公开 session，再调用上层授权/重组回收，最后开始广播。
- **已消失 ACL：**统一 `terminateConnection()`，把 SDK 的 ENOTCONN 纳入现有 pending disconnect 回收；
  EALREADY 只表示终止已请求，其他错误不冒充成功，TX 失败的待关闭标记保留到物理回收。只新增一个 uint16
  native handle，无队列扩容、无线重传协议或用户 API。真实 Host/广告启动失败仍失败，不能一律忽略错误。

Host 新增 `blinker_esp32_nimble_adapter_tests`，用仅在测试目标可见的窄 SDK fake 编译**实际适配器**，不复制
生产状态机。旧实现的 indication/handoff 两项分别明确 RED；新实现覆盖交接前/交接时/交接后断连、立即连断、
晚到失败/成功、错误 peer/attribute/notification 类型、异步断连、SDK busy/ENOTCONN 与真实广播启动错误。
全量 Debug ASan **45/45**、架构检查通过；这是事件顺序回归，不替代双核调度、RF 或 SDK 的真机稳定性验证。

内部 `Esp32BleAccessResetProbe` 增加只读 `b`：被动 SDK GAP listener 在锁内累计连接/断开/indication 错误
及公开原因码，Arduino 任务按需输出 uptime、Host synced、adv、最近已观测 handle 的 ACL 和 heap。没有 native
回调 Serial、密钥、包内容或 MAC 日志；正式库不新增诊断字段/公开 API。各项非原子 SDK 快照，listener 注册前
的首次连接可能漏计，因此零计数或 `acl=0` 不能单独证明无连接；有 ACL 时 `adv=0` 本来正常。

**冻结组合：**只更新 child COM12 / `98:3d:ae:ee:7c:6c`，保留相同分区与 NVS，使用 Core 3.3.11，
Flash **695,240 B**、globals **34,896 B**，相对旧 probe **+1,112/+48 B**（含 test-only listener/打印开销，
不是纯库增量）；bin SHA-256 `538c521f1f0a016b3f13f782689e5e956791bcd10579c9fc19fdc0f3992552f9`。
Hub、App、shard-1 沿用 §10.38 冻结版本，本批不重启/更新它们。刷写必然复位 child 运行态，所以新观测不能
倒推此前空广播的唯一原因；未做 resetAccess、detach、重新认领或业务数据库写入。

**分开保留两个失败边界：**

1. 修改前组合 `f97fb8f87f47056c86d31a83f9b44e2c` 能扫描并 Connected（3,860 ms），11,468 ms 遇到
   `EDGE_GATEWAY_PERMIT_JOIN_RELAY_HUB_OFFLINE`，12,643 ms 清理完成，PG Closed/App 0 残留。
   这是非空广播的独立 Hub/MQTT 可达性失败；不能归因到本批 peripheral 修复，也不能算成功循环。
2. 新 child 的 selected-acl 注入 `c6b918e2675044bfc0c02029388b9121` 在 13,284 ms 完成 Method 2/同步，
   无效 record 后 13,661 ms 确认 Rejected，但一次自动 disconnect 请求失败，permit checkpoint 残留 1。
   门禁正确停止（不是 2/2）；之后单独调用既有 `recoverPermitJoinWindows()` 经正常 close/确认终态恢复到 0。
   不直接删除存储、不把 recovery 拼成首次成功。测试脚本为 disconnect 失败追加既有白名单错误码记录，
   原失败未捕获具体错误码，后续需重现确认是否为瞬时 HTTP/Hub 不可达或状态机错误，不凭猜测改 App。
   child 142,201 ms 观测 synced=1/adv=1、indicationErrors=0；仅证明这次实际 ACL 关闭后的广播恢复。

**新 child 独立正常循环 10/10：**没有在轮间 reset/re-enroll/retry，App、Hub、Broker 没有新代码或重启。
首 operation `8ad869265eebc2dcb4dab3b471c2c8d6`、末 `68c9436275f3cec078f168ad54230eef`，各轮耗时
`12875/13309/14343/14142/13746/14084/13497/13698/13376/14517 ms`，总计 **137,587 ms**；Connected
（包含 HTTP 控制面）`2356/2532/2555/2584/2372/2779/2530/2572/2781/2796 ms`。独立 PG 查询十个窗口
均 Closed，每轮 App links/sessions/checkpoints=0。child 持续被连接/释放、Host synced=1，运行态观测到
indicationErrors=0、heapMin=244,588 B；这不是人为制造 indication timeout 的硬件门禁。
原 topology 仍 Active(5)/version 4/updated_at=1788615332648，NVS root 260 B SHA-256 仍为
`390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`（只证明内容未变）。

**后续边界：**确定性软件缺陷已修，不宣称 §10.38 的历史空广播已唯一归因，也不宣称真实 indication 超时
故障已注入通过。继续冻结组合短循环和低侵入 native 观测；优先补上述清理失败的静态错误码以及 Hub MQTT
丢联证据。正常短循环独立记录，再回 N3 envelope tag/context/expiry/replay/durable ACK 与 enrollment requestId
device replay；N4/N5、N2 双 Hub、D4b 与 30/100/发布长稳仍各自保留，不以复位、擦凭据或加全局重试代替。

**补充拒绝→恢复复测：**增加静态错误观测后、不改 App/APK/固件、不 reset，再独立执行 selected-acl 两轮。
`18a87e814cd5c6d5a57883c4b8535166` 在 12,698 ms 同步后注入，13,071 ms 确认 Rejected、13,160 ms 清理；
fresh `242a325712368e06ecab80ba937fb607` 在 13,225 ms 同步、13,915 ms Closed。PG 分别 Rejected/Closed，
两轮 links/sessions/checkpoints=0，结束 App idle。此前一次关闭请求失败这次未复现，不能由此声称已修复。
最后 child uptime **731,013 ms**，synced=1/adv=1/acl=0、indicationErrors=0，heap free/min **244,852/244,588 B**，
NVS/topology 复核仍不变。本批未新增暂存、commit 或 push；App/Service 只同步实施文档。

### 10.40 Close 终态竞态修复与 Hub 云连接换代证据（2026-09-06）

**本批范围：**沿 §10.39 先处理清理失败与 Hub 可达性，不扩散 Mesh、多 child scheduler、App UI 或认证
协议。仅修改 Broker 两处生产边界及对应测试；App、Service、Hub 和 child 的生产实现保持原版本。

#### 10.40.1 根因与最小修正

Service 持久窗口仍为 Ready 时，Hub 可先通过 MQTT 将 Broker ledger 推进到 Rejected；App 随后发 Close，
原 `stageEdgeGatewayPermitJoin()` 只允许 Open/pending_open/ready 转 Close，误把“窗口已结束”当作操作冲突。
这是跨端状态投影的时序问题，不是 App 未调用 disconnect，也不是 NimBLE SDK 版本问题。

修正仅落在 Broker 的既有 staging 边界：

- 先验证 exact operationId、tenant、Hub、adapter、expiresAt；同 action 仍先比较 exact command。
- 新 Close 对同窗口 Closed/Expired/Rejected/Busy/Unsupported 返回已有记录、`replayed=true`；
  不覆写原 Open/terminal result，不清理 ledger，不增加 attempt，不向已经结束窗口的 Hub 重发。
- 非终态继续原状态机；错绑定、变化 command、非法状态继续 Conflict。Service 使用现有 stage response
  更新自身投影；App 使用现有“确认 exact 终态后清理 checkpoint”，不增加 GET、全局重试或旁路删缓存。

旧冻结 Broker 镜像挂载新增测试明确在 terminal Close 报 Conflict；新构建 Linux Broker Device V2 suite
通过 Rejected/Expired/Busy/Unsupported、重复 Close、错绑定、零新增 publish 与完整 metadata 保持断言。
Service 专项同时验证 stale Ready→Broker terminal→持久终态→第二次 Close 零 Broker 请求；内存与真实
PostgreSQL 仓储/HTTP 专项通过。TypeScript build、Docker workspace build 通过；App 既有 permit link +
Direct session 正式 Angular **2 files / 25 tests** 通过。本批未改 C++，未重跑或冒领新的 Host/固件编译结果。

#### 10.40.2 真机先失败、修复后通过

固定 Android Debug APK、COM11 Hub、COM12 child；未 reset、擦 access、detach 或重新 enrollment。
使用既有 `edge_gateway_connection_gate.mjs --fault selected-acl --count 2`，只在第一轮安全同步后注入
非法应用 record，第二轮 fresh operation 正常同步/关闭。认证操作不写 child credential。

| 组合/operationId | 独立结果 |
|---|---|
| 旧 Broker / `c421c8927a126f3fd078e6a0b2c4b1dc` | 13,942 ms 同步，14,616 ms disconnect 报 `EDGE_GATEWAY_PERMIT_JOIN_CONFLICT`，14,618 ms 确认 Rejected；checkpoint=1，门禁按失败停止，不继续拼第二轮 |
| 新 Broker / `3fd9ef7a5831ba9caaca4ee000d5e8c2` | 13,976 ms 同步/注入，14,978 ms disconnect 完成，15,007 ms Rejected 确认；links/sessions/checkpoints=0 |
| 新 Broker / `3493cf7dc62bb491a70ba3a48ca9a6ee` | fresh operation 13,259 ms 同步、13,973 ms Closed；links/sessions/checkpoints=0 |

旧失败窗口通过现有 `recoverPermitJoinWindows()` 正常 Close/查询确认终态后清理为 0，不手工删除
checkpoint。新第一轮 PostgreSQL 为 Rejected，Broker 仍是 **action=Open/status=Rejected/attempt_count=1**，
updated_at=1788627603757，证明终态未被伪造为 Close；第二轮 PG/Broker 为 Closed/action=Close/attempt=1，
updated_at=1788627617572。原 topology `d01103b0b3880397268c139a492785fd` 仍 Active(5)/version 4/
updated_at=1788615332648。

末次只读复核 App idle、busy=false、links/sessions/checkpoints=0；child uptime=2,662,377 ms、synced=1、
adv=0/observed ACL=1，同时 Hub childState=4/secure=1、permitState=0、relayActive=0，符合既有 Gateway
业务连接已恢复的观测，不把“被连接时不广播”当作停播。child indicationErrors=0、heap free/min=
244,852/244,588 B。Hub management/route/relay error=0、managementSubscribed=1，loop/ipc0/nimble 水位
3684/1112/2764 B；这些是存量运行态观测，不是新的长稳准入。child NVS root 260 B，SHA-256 仍为
`390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`；只证明内容未变，不证明物理写入次数为零。

**冻结组合与部署范围：**

- Hub 固件 SHA-256：`cb31fb6a4973e2810d5c86347f9da2f23f0c1224eb5bcb96af6e7f48cc4aa67e`。
- child 固件 SHA-256：`538c521f1f0a016b3f13f782689e5e956791bcd10579c9fc19fdc0f3992552f9`。
- APK SHA-256：`e9ad22f1707472f6eba624003e44cb522531ba2a0fd2f2bbda43bd636dcd7a95`。
- 新本机 `broker-shard-1` 镜像 ID：`sha256:469ad0771d320a95c4888550e140f5981ee17f60ba7684895630bd926f9c0f8e`，
  healthy；仅该 shard 重建部署，其他 shard/Gateway/Controller 未滚动，不能宣称集群全部升级。
- 本机部署临时指定 `BROKER_LOG_LEVEL=info`，未改变 compose 文件的默认日志级别；不启用 packet debug。
  将来发布需完整回归并滚动全部 Broker shard。本批未新增暂存、commit 或 push。

#### 10.40.3 Hub offline：已确认的事实与未解决的边界

Broker 新增只在 Edge Hub 连接转换时记录的结构化日志：connected/disconnected/close_requested/
management_ready/management_unsubscribed；字段仅 logicalDeviceId、非秘密 connectionId、managementReady、
距 credential 过期毫秒数及固定 close reason。不记录 packet/topic/账户/DeviceKey/SessionCredential，
不新建连接历史缓存，也不修改 session registry。正常 WiFi/BLE 不增加设备 RAM/Flash。

实际自然换代（以下为 UTC）：旧连接在 **2026-09-05 16:59:29.590641** 断开，仍距过期约 118 s；
新连接 **16:59:30.041143** 完成连接，**16:59:30.159921** management ready，间隙约 **569 ms**。
同期 SessionCredential TTL 约 600 s、创建间隔约 481 s，符合正常提前刷新；该次未看到 Broker
close_requested，不能把断连归因于 token 已过期或 BLE Host fatal。

历史 §10.39 `f97fb8f87f47056c86d31a83f9b44e2c` 的 HubOffline 发生在 1788624687469，而新 credential
创建于 1788624682246：时间相关支持调查刷新边界，但历史没有同等连接日志，**不声称唯一根因已证实**。
本批成功两轮在上述换代完成后才开始，**不算 active permit 跨换代通过**。

代码审查另发现明确的不一致路径：`GatewayPermitJoinRelayClient::poll()` 在 cloud 非 Online 或
management 未 subscribed 时调用 `clearOperation()`，清除候选 revision、已选择 token、上下行 batch/序号；
但 `GatewayManagementClient::configure()` 的 credential 换代刻意只清 cloud binding，保留本地 coordinator。
因此原 permit 窗口/ACL 可能尚存活，而 relay 再次开始同 operation 时序号退回 1；Broker 的 mailbox
却仍保持旧 sequence/candidate revision。实际走到哪一分支必须以确定性故障测试与真机验证确认，
不能仅凭此次 570 ms 的可达性观测，断言已经复现了在途序号冲突。

#### 10.40.4 下一批顺序与退出条件

1. **先冻结并测试生命周期不变量。**窗口/selected ACL 才拥有 relay operation 生命周期，MQTT socket
   generation 不是同一个概念。对同 operation、同授权上下文且同一存活 ACL，恢复若被允许，必须保留
   已提交序号、exact pending batch、包内进度、候选 revision 和 ACK 幂等；绝不在活窗口中清零后从 1 重用。
   不能证明上下文连续、ACL 丢失/重建、时钟失信或窗口过期时，须结束 exact 窗口并保持可对账终态，
   再用 fresh operation；不能把所有连接更换都当作续传许可。
2. **先补 Host RED 再改最小代码。**覆盖已完成 Down、部分 packet 在途、Up 等 ACK、Select/候选阶段、
   离线期间到期/失信/ACL 丢失、重新订阅与 changed credential epoch；检验不重复南向写入、不漏 ACK、
   不复活旧窗口、不改变原 rate/fragment/deadline 上限。临时 cloud pause 的保留策略与授权失效的
   fail-close 分开设计，由 coordinator 管窗口、relay 管有界分片、lifecycle 管云准入，不新增全局重试器。
   未证明可安全续传的分支先收敛为终态，不能只把 `clearOperation()` 换成无条件 return。
3. **四端精确验证。**同冻结组合，分别在在途 batch 时覆盖自然 credential refresh 和受控 MQTT 重连；
   对照连接事件、operation/sequence、App 清理、Broker/PG ledger、NVS/topology。若返回 HubOffline，
   明确按原操作安全恢复还是终态后 fresh operation，不能吞 HTTP 503 伪装成功；不通过擦凭据/复位 child
   掩盖问题，也不以任意延长 TTL 避开刷新。
4. 修正后独立正常 10 轮通过，再回 **N3 envelope tag/context/expiry/replay/durable ACK 与 enrollment
   requestId device replay**。N2 双 Hub、N4/N5 其余边界、D4b 专用加密板与 30/100/发布长稳仍独立待办。

### 10.41 relay 生命周期修正、迟到应用 ACK 与 MQTT pending 边界（2026-09-06）

#### 10.41.1 设备：窗口不再从属于 MQTT socket

旧实现的 Host RED 已确定性复现：存在 active permit、已有下行写入时，仅停 MQTT transport，relay 即
`active=false`，而 coordinator/window 未结束。修正保持模块职责，不加队列、协议字段或 SDK 特例：

- management 维护非秘密 `authorizationGeneration`：只有 DeviceKey version、environment/tenant/Hub
  context 改变或重新建立失效配置时递增；同授权的 SessionCredential/clientId 换代不递增。空的可选 context
  也按值比较，不把两个空值当作授权变化；A→B→A 配置切换不能假装连续。
- relay 保留所属窗口的授权 generation。仅云离线/尚未重新订阅时暂停发送，保留候选 revision、选中 token、
  Down 包内进度/已提交序号、Up exact pending batch、最近 ACK、rate/fragment 计数，不重置成 sequence 1。
- 云不可用时继续轮询本地 coordinator 的有界 deadline/原生 ACL；过期与失信在 native pump 前检查。
  原 selected ACL 丢失仍沿既有 Native adapter Fault→Rejected，不能在新 ACL 上续传 opaque 分片。
- 授权变化/配置失效或显式 relay reset 通过 coordinator 的 `rejectWindow()` 关闭窗口并保留 exact 终态；
  清 relay 数据不再留下同 operation 可从 1 复活的活动窗口。完整 product stop/admission reset 仍遵循既有
  生命周期，不把这批的瞬时终态缓存冒充掉电持久化。在线管理和离线 relay 各自覆盖 coordinator 调度空档。

新增矩阵覆盖同 MQTT credential 重连/新 clientId 同授权换代 × 已完成 Down、部分 Down、Up 等 ACK、
候选、selected Connecting 共 **10** 种连续性场景；另 **10** 种终止场景覆盖 expiry、clock untrusted、
adapter Fault、DeviceKey version、tenant/environment/Hub 变化、无效配置、显式 reset 和 ABA。
验证 revision 不倒退、迟到 ACK 不清新 batch、rate credit 不重置、不重复南向写入、终态 Open 重放不复活、
fresh operation 可重新开始，测试存储 replace=0。Host Debug ASan **45/45**、架构检查与 diff check 通过。

Core **3.3.11 / IDF 5.5.5** 同旧 probe 编译为 **1,433,124 B Flash / 65,340 B globals**，相对 §10.40
净增 **272/8 B**；bin SHA-256 `3bdd3d1aea12a3a85dcb7af2e5be7ccdfe4cb8104f1ee2724499de3c4f533261`。
随后仅内部 probe 利用既有 platform 注入 seam 增加串口 `c`：30 s 内、Up 正等 ACK 时单次关闭 MQTT socket，
不触碰 WiFi/BLE/product reset/access。含该测试 hook 为 **1,433,348 / 65,348 B**，额外 **224/8 B**
属于测试代码，不计作正式库资源增量。该固件 SHA-256：
`26d702b8f6560b6a61db8769a5ddb45f933cd3c819a2e39e93b4daa272480b13`。
已烧录 COM11/MAC `8c:bf:ea:1b:96:14`，bootloader/分区表未变化，仅正常更新 app，未擦 NVS。
child/SDK/Broker 镜像继续 §10.40 冻结组合，未更改 child 或重部署服务器。

#### 10.41.2 真机新发现：迟到应用 ACK，不是 relay sequence 倒退

新 Hub + 旧 APK 的前置两轮独立结果为 **1/2**：

- `26a326286222f16a521816399b98522e` 15,196 ms 完成认证/同步，15,909 ms 清理；
- `7bd34bd874ed7a8f42d542af43facf5c` 已 Connected，13,253 ms 起因 `BLE_DIRECT_RESPONSE_MISMATCH`
  关闭，13,907 ms 清理。两窗口 PG Closed、App 全部 0 残留；失败不是接入通过。

此时未注入 socket 故障。给既有硬件 gate 增加最多 32 条已认证 BBP/2 **kind/flags/sequence 与 pending
metadata** 观测，不记录 frame body/credential，也不改 APK；hooks 最终恢复。随后独立正常两轮
`59324d10186b3927490ecb3ca6928d6f` / `0f0aaa1a61774061c21d35c7af1253e8` 分别 13,237/15,092 ms
通过。再一次 fresh `9d82b57699a31ead0a0a37f0c2a2d41f` 在 14,920 ms 失败，15,642 ms 清理，捕获到：

| 时刻（该 operation 相对时间） | 真实 App 事件 |
|---|---|
| 10,918 ms | 发送 ManifestAccept(kind 6)/sequence 5，等待 Ack(kind 20) |
| 12,800 ms | 同一 ManifestAccept/sequence 5 重试 |
| 12,916 ms | 收到第一 Ack，device frame sequence 35 |
| 12,917 ms | 请求 State(kind 16)/sequence 6，等待 StatePage(kind 22) |
| 14,917 ms | 收到迟到 Ack(kind 20)/device sequence 36，旧 App 按 State 类型不匹配关闭 |

App `BleDirectSession` 已最小修正：只记最近一次**已合法确认的** `DeviceV2Ack`，按 canonical decoded
acknowledgedSequence + 可选 stateRevision 幂等；device frame sequence 可以不同，因为回复使用 fresh
DirectSecure record。只忽略不属于当前 pending sequence 的该 exact ACK，不提前完成新请求，不接受错
flags、变化 revision、更早/未来 ACK；Error 继续独立校验，关闭清理缓存。不延长超时、不新增重试、不改 UI。
旧实现新增“ManifestAccept duplicate ACK 插入 State 等待中”测试明确 RED；新 App **2 files / 30 tests**
与 ESLint 通过，包含四种非 exact 负测；local Angular build、Capacitor sync、Gradle assembleDebug 通过。
新 APK SHA-256 `786dfb77424aa9bc98915c5700a9d1c626ca9b8068d3d4e181dba072173f24cb` 已覆盖安装，
未清应用数据/登录与 Direct credentials；旧 APK 的上述失败不追改为成功。

新 APK 同 Hub 固件独立正常 **3/3**：`dd49c0bb50a6175216d2d8303b2f3ea6`、
`2b694ce5f5f7979c4ae18d051313eaff`、`92137f6b80a1d0fee6e62521a79b2242` 分别
13,749 / 13,110 / 12,741 ms 完成；PG 均 Closed，每轮 App links/sessions/checkpoints=0。
另用 `--fault delayed-ack --count 1` 对 `795a779df973d84a4fe783cd68102939` 延迟处理首个应用 ACK
1300 ms，触发 App 原有可靠重试经真实 Broker/Hub/child 产生迟到 ACK，而非伪造解密回复。
观测到 **1 个 ACK 到达非 ACK 等待阶段**，14,509 ms 同步完成、15,229 ms 清理、PG Closed/updated_at=
1788630953952、App 0 残留。该通过关闭此迟到 ACK 真机子门禁，不算 MQTT 掉线或自然延迟统计。

最终 App idle/busy=false/links/sessions/checkpoints=0，原 topology 仍 Active(5)/version 4/
updated_at=1788615332648；child NVS root 260 B SHA-256 仍为
`390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`（内容一致，不等于物理零写）。
child uptime=5,496,662 ms、Host synced=1、adv=1、最后观测 ACL=0，heap free/min=244,852/244,588 B；
本批间出现一次 indication error（native status=7），之后未 reset 仍完成新连接/同步/广播恢复，不能写成
“零 indication 错误”，也不能仅凭最后错误值指定其原因。Hub 最终 permit/relay idle，management/route/relay
error=0，heap free/min=105,408/55,360 B，loop/ipc0/nimble 水位=3684/1112/2708 B。这些是运行态快照，
不替代完整长稳。未新增暂存、commit 或 push。

#### 10.41.3 受控 MQTT 中断：四端续传门禁仍未关闭

新 Hub/旧 APK，operation `e27fff8fbb91961af727bae4fb9cc113`：内部 probe 明确记录
`mqtt_socket_closed_upstream_pending`。2,704 ms Connected；Down sequence 1 已 Accepted、Up sequence 1
已进入 Broker；App 在 3,589 ms 发送 Up ACK、3,642 ms 收到 `EDGE_GATEWAY_PERMIT_JOIN_RELAY_HUB_OFFLINE`
（HTTP 503），正常 Close 至 9,958 ms 清理完成，links/sessions/checkpoints=0，PG Closed/updated_at=
1788630059390。该两轮命令按首轮失败停止，不伪造后续恢复成功。

Broker 同 clientId `d2c_wCjlSn6NubstXXBL8wJrfQ` 在 UTC **17:40:53.377476** disconnected，
**17:40:59.198812** management ready，间隙 **5,821 ms**；socket 重连不是新 DeviceKey/credential epoch。
本批未以人工暂停 App ACK 或吞 503 改造生产客户端来制造成功，也未把这次中断等同于自然 SessionCredential
refresh。设备 Host 层的连续性修复已过，**实际生产 App 的在途连续性仍失败**。

确认代码边界：Broker `submitPermitJoinRelayDown()` 的 Select/Down 先写有界 pending 再尝试 publish，
而 Up ACK 在 publish 成功后才更新 lastUpAck/清 upstreamPending。无 management connection 时 publish
同步抛 HubOffline，因此“数据已入队”和“尚未接受 ACK”可能返回同一失败；App 按既有传输失败合同关闭
window。这不是单靠保留设备序号能够解决的问题。

#### 10.41.4 下一批收口顺序

1. **Broker 先 RED 再最小修正接受合同。**严格校验 window/authority/relaySession/sequence 后，若 Select/
   Down 已确实进入现有有界 pending，可返回 pending view，由现有 timer exact 重投；不能伪造 Connected
   或 Down Accepted。Up ACK 应评估复用现有 lastUpAck/Hub exact upstream retry 完成“已接受”与“已送达”
   分离，不增加无界队列；只有已保留可恢复 ACK 事实后才能返回接受成功。仅缺 management connection 是
   暂停，不放过 runtime 不可用、过期、错绑定、序号/内容冲突、容量、撤销或其他 publish 错误。
2. 覆盖“入队前失败、入队后离线、ACK 接受后响应丢失、重连 exact 重投、重复/变更 ACK、过期清理、
   Broker 重启 relaySessionId 变化”等矩阵；内存 mailbox 不冒充 durable，Broker 重启依旧 fail-closed。
   不先给 App 叠加泛化 503 重试，也不对尚未入队的请求谎称成功。
3. 用新 App/同冻结 Hub，分别复测 socket 中断与自然 SessionCredential refresh 的在途操作，独立记录
   真实完成/清理、sequence、Broker/PG 与 NVS。之后独立正常 10 轮，再回 N3 其余 envelope/enrollment
   replay、N4/N5；N2 双 Hub、D4b 与发布长稳不变。本批不进入 30/100 或宣称透明续传已经完成。

### 10.42 Broker 有界接受与暂时离线恢复（2026-09-06）

本批执行 §10.41.4：不再把“已保留、可重投”误报为“未接受”，也不把它冒充 Hub 已确认。
设备、App 生产源码/固件/APK 均保持 §10.41；仅 Broker relay 和既有 Broker suite 增量修改。

#### 10.42.1 合同与最小实现

| 输入 | 接受事实 | Hub 暂无 management connection | 真正完成的证据 |
|---|---|---|---|
| Select | 既有 selectCommand + selectedCandidateToken | 返回原 pending view，既有 500 ms timer exact 重投 | Hub 的 SelectResult，不伪造 Connected |
| Down | 既有单个 downPending + sequence | 返回原 pending view，同一 timer exact 重投 | Hub 的 Down ACK，不伪造 Accepted |
| Up ACK | 先保存既有 lastUpAck，再清 upstreamPending、递增 nextUpSequence/revision | 返回接受后的 view；不新增 ACK 定时队列 | Hub 重发原 Up batch 时收到缓存的 exact ACK，之后才推进自身序号 |

只有缺 management connection 被视为可暂停的交付条件。runtime unavailable、错 operation/tenant/Hub、
未 Ready/过期窗口、候选冲突、Down busy/sequence、变更或未来 Up ACK、64 mailbox 容量仍拒绝。
把 Select/Down 的 **decode fallback** 与 **提交/发布** 的异常作用域分开，避免普通 publisher Error 被
误当成“不是这个帧类型”而吞掉；真实 publish 故障仍向调用者失败，已保留的内容不凭空撤销。
异步 Aedes payload 继续独立 Buffer 所有权，HTTP scratch、终态及 runtime cleanup 不会清零待发 packet。

timer 每次重投前重新验证 Ready window 与 exact tenant/Hub；Close/pending_close、Rejected 或 expiry
使 pending 失效并清理，不能绕过窗口状态继续下发。该检查不等于新增了独立 DeviceKey epoch 持久 ledger；
设备 §10.41 的 authorizationGeneration 终止边界及 Service 现有鉴权仍必须保留。
临时 socket replacement/同授权新 clientId 从 registry 解析当前订阅 topic，不缓存旧 MQTT connection。

内存 mailbox 不提供跨 Broker 重启耐久性。重启丢弃 pending/lastUpAck 并更换 relaySessionId；该 ID 在
response 中由 App 既有 link 校验，不宣称 Broker 请求已带 expected relaySessionId 或增加了新 wire 字段。
旧 App link 遇 ID 变化仍 fail-closed；不能拿这批作为 Broker restart 透明续传的证据。

#### 10.42.2 软件证据

新增定时器可控的同一 Broker suite 矩阵，旧发布镜像明确 RED：offline Select 入队后抛
`EDGE_GATEWAY_PERMIT_JOIN_RELAY_HUB_OFFLINE`。新实现通过：

- 入队前错绑定/未 Ready/ACK 无 pending/序号拒绝，offline Select 与 Down exact 幂等、变更内容拒绝；
- 离线 timer 不发布，重新订阅 exact 重投；同授权 clientId 换代使用新 topic；
- Up ACK 接受后响应丢失、offline exact retry、Hub 原 batch 重试补 ACK；旧 ACK 不清下一包；
- 普通 publisher Error 不被解码 fallback 吞掉；Close/Rejected/expiry 后不重投；
- 64 个有界 mailbox、第 65 个拒绝；runtime 不可用拒绝；重启 ID 换代且不能恢复旧 ACK/pending；
- 独立 Aedes Buffer 在 scratch/清理后内容仍一致。

TypeScript build、Linux **Broker Device V2-only suite**（含新增矩阵）、新镜像内再次回归均通过；
Service permit-join 专项的内存与独立真实 PostgreSQL schema 两次通过；App session/permit link
**2 files / 30 tests** 通过。本批未新增 App 重试、timeout、UI 或依赖。
仅本机 shard-1 部署镜像 `sha256:ef44e103b1c61958d07307c692196036b1c8229f24ad5b0642b1f797320a0d28`，
health=healthy；其他 shards/Service 不重部署。info 连接元数据临时观测，仍无 packet/secret 日志。

#### 10.42.3 四端 socket 故障门禁

复用既有 Hub 串口 `c`，明确记录 `mqtt_socket_closed_upstream_pending`，未 reset、清 NVS 或改 APK。
随后同一 operation `aead69f566d06e844acd62069cbc093a` 在 **17,781 ms 同步 / 18,488 ms 清理**完成；
fresh operation `a03f594a534278253b4fe6e0d79bdfe6` 在 **12,698 / 13,406 ms** 完成，结果 **2/2**。
每轮 links/sessions/checkpoints=0；Broker ledger 均 closed/attempt_count=1，PG closed/updated_at 分别
1788652012941、1788652026340。不把上一批的失败记录改写成通过。

Broker 同 clientId `d2c_dTcAUdnFfw-VoppafeTm_A` 的断开与 management_ready 分别为 UTC
23:46:38.304209 / 23:46:43.738528，间隙 **5,434 ms**；是同 MQTT credential socket 恢复。
App 相对耗时使用手机时钟、Broker 连接事件使用服务器时钟，不直接用两者绝对时间相减。
child 只读 witness：uptime=26,491,791 ms、synced=1/adv=1/ACL=0、connects=39/disconnects=40；
indicationErrors 仍为历史 1、native status=7，heap free/min=244,852/244,588 B。
NVS root 260 B SHA-256 仍为 `390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`，
原 topology 仍 Active(5)/version 4/updated_at=1788615332648。哈希相同只证明内容未变。

#### 10.42.4 自然 SessionCredential 换代（独立通过）

依据只读 session created_at 定点启动原 gate，不修改服务器 TTL/refresh、设备时钟、App 超时或操作流程。
operation `750a57cabbdc3abcbaf266fcef0c2d74` 在 **14,917 ms 同步 / 15,628 ms 清理**完成，App 三项
残留均 0。执行器起止时间为 1788652379460～1788652395636；PG window created_at=1788652380926、
closed/updated_at=1788652395092；Broker window created_at=1788652380968，同样 closed/attempt_count=1。

窗口进行中，旧 clientId `d2c_dTcAUdnFfw-VoppafeTm_A` 在 UTC **23:53:08.554919** disconnected，
新 clientId `d2c_-iCEJ6G5mU6zV1x3wIv8EA` 在 **23:53:08.984107** management_ready，间隙 **429 ms**。
新 session created_at=1788652388506，在上述窗口内；两者 credential_version 均为 **2**，没有吊销或
DeviceKey epoch 变化。此证据验证同授权自然 credential generation 换代时的在途连续性，不混同
§10.42.3 同 clientId socket 中断，也不推广为断电/Broker restart 的耐久恢复。

#### 10.42.5 独立正常循环与最终 witness

socket/自然刷新之后另起正常 **10/10**：总墙钟 128,802 ms，单轮耗时合计 128,695 ms，范围
12,392～13,779 ms。不与前述故障或历史批次拼接统计；全部认证/同步成功，App 每轮
links/sessions/checkpoints=0，PG/Broker 均 closed，Broker Close attempt_count 均为 1。

| 轮次 | operationId | 清理完成耗时 ms | PG Closed updated_at |
|---|---|---:|---:|
| 1 | `0cf96238ed22cf850db2f5f84ace1460` | 13024 | 1788652464818 |
| 2 | `233c3366cc121275eb0f94251dcc8dc5` | 12496 | 1788652477309 |
| 3 | `f401cffa3a8aedcb7653f4ae00cce49a` | 12954 | 1788652490317 |
| 4 | `1179bde603127d0974d1d5cca0dc6f27` | 13166 | 1788652503521 |
| 5 | `0de45e3fc06350d1e406716d74075f26` | 12878 | 1788652516324 |
| 6 | `06a4abd3104500c17ac67f95e79038e9` | 12392 | 1788652528760 |
| 7 | `35d2e924419e83bf59f38e10bcbac831` | 12855 | 1788652541616 |
| 8 | `0bc2300831d608b07d69193c09a28c40` | 12495 | 1788652554212 |
| 9 | `b28b4814553c61cb943de54816c0ed10` | 13779 | 1788652567931 |
| 10 | `15986242bf20ff2082e31bb30224d49f` | 12656 | 1788652580571 |

最终 App idle、available=true、busy/workflow=false、三项残留均 0。Hub managementSubscribed=1、
permit/relay idle、management/route/relay errors=0；heap free/min/largest=105,412/47,216/57,332 B，
loop/ipc0/nimble stack minimum=3544/1112/2708 B。min heap 包含本批自然 TLS/session 刷新，不只采样
空闲 free heap。原 Active topology/version/updated_at 未变。child 最终 uptime=26,991,173 ms，
synced=1/adv=1/ACL=0、connects=51/disconnects=52；indicationErrors 仍为历史 1、status=7，无本批新增；
heap free/min=244,852/244,588 B。NVS root 260 B hash 再次等于 §10.42.3，未执行 reset/擦除/重新 enrollment；
内容一致不等于物理零写。
本批未新增暂存、commit 或 push；只更新现有工作树的 Broker 实现/测试与三端方案状态。

#### 10.42.6 下一批

§10.41.4 的 Broker 有界接受、真实 socket/自然刷新在途恢复及独立 10 轮已完成。下一批回到 N3 剩余
envelope context/tag/expiry/replay/durable ACK 与 enrollment requestId replay 的既有合同和故障矩阵，
先软件确证，再做有明确恢复路径的四端注入；不以重复正常循环替代负测。随后处理 N4/N5 剩余边界。
N2 双 Hub、D4b 生产加密存储及发布长稳仍独立待办，不扩大到 30/100 或宣称全面发布通过。

### 10.43 N3 Service→ESP32 真 AES-GCM 与隔离 NVS（2026-09-06）

#### 10.43.1 本批范围与架构决定

§10.31 的 Host delivery fixture 是确定性 authenticated transform，用于校验 KDF/AAD/存储顺序，不能把它
当成真实 MCU AES-GCM。此次沿用正式模块 `GatewayAccessDeliveryProcessor`、`GatewayAccessStore`、
`Esp32MbedTlsCryptoProvider` 与 `Esp32NvsAtomicBlobStore`，不新增生产 sealer、管理 API、credential 槽或重试。
未发现需修改产品实现的新缺陷；不为测试强行改写正确分层，也不删掉仍承担独立回归作用的 Host fixture。

当前现场 child 已 Active。直接修改 acknowledged delivery ledger、拿真实 Key 重签测试 envelope，或先
清 child access，会引入与密码学校验无关的绑定变更。因此本批使用**公开合成凭据、固定 fixture、独立
`nvs:bl_n3_env/record`**，诊断固件不连接 WiFi/BLE/MQTT。测试结束恢复原 Hub app。此取舍只补上实际 SDK
与 NVS 证据，不将隔离处理器注入写成四端传输注入通过。

新增仅在测试目录：

- `tests/golden/server_gateway_envelope_probe.cjs` 调用**正式 Service `sealGatewayAccess`**，固定 public
  DeviceKey/secret/PresenceKey/nonce，产生首次、合法重封、业务材料冲突三份 real AES-GCM envelope 和
  expected ACK。无参数核对 `EnvelopeVectors.h`；`--header` 仅 stdout 生成供审查。vault 是明确的 stub，
  此处没有验证 KMS，也不接触真实账号或用户 secret。
- `tests/internal/arduino/Esp32GatewayEnvelopeProbe` 只提供 e/v/r/s/x 诊断命令。`e` 做矩阵，`r` 软件重启，
  `v` 验证 persisted exact fixture/ACK 且不写，`s` 只读原 Hub access 的 size/hash，`x` 匹配精确 fixture
  后清除该测试 record。未知内容 fail-closed，不清其他 key、namespace 或 NVS 分区。
- `tests/hardware/README.md` 写明执行前原固件/分区/端口校验、无活动事务、精确清理与恢复检查，避免把
  diagnostic sketch 安装流程传播给 Arduino 初学用户或混入正式 examples/package。

`ProbeBlob` 的 protection override 与旧 DualBoard probe 一样，**只为未加密开发板诊断而存在**，不证明
Flash Encryption/NVS attestation；D4b 仍需专用生产加密板。时间失信/expiry 使用 fixture IClock 注入，
不改变 MCU/SNTP/服务器时钟或有效期，不冒充真实产品墙钟到期测试。

#### 10.43.2 实际矩阵与结果

以下 22 种拒绝分别在**空 record**和**已保存 record**下执行，共 44 个负向场景：

| 组别 | 场景 | 断言 |
|---|---|---|
| envelope valid canonical 字段篡改（15） | environment、tenant、Hub、DeviceKey version、child ID、epoch、credential version、Presence version、expiry、operation、instance、controller、nonce、ciphertext、tag | 不重封，正式解封返回 AuthenticationRequired；没有将错误 CBOR 当作 AEAD 负测 |
| 本地 expected context（4） | 错 environment/tenant/Hub/DeviceKey version | 对原合法密文拒绝；不以 optional context 省 RAM 为由跳过 KDF/AAD 绑定 |
| 根 Key（1） | 换为不同的有效 DeviceKey bytes | 原合法 envelope 拒绝 |
| 时钟（2） | unixTime untrusted、恰好到 expiresAt | 不论是否曾成功保存均拒绝，不发旧 ACK |

每次负测前后核对真实 NVS blob 内容摘要、present 状态与 `replace()` attempt 计数完全不变；ackSize=0，
整个 ACK 输出缓存清零。另以合法 Service 重签但同 operation 改 secret 的 envelope 验证 SequenceConflict，
同样零 replace/内容不变，共 **45** 个拒绝场景。合法 rewrap 更换 nonce/ciphertext 而材料不变则成功，
仅更新 envelope metadata，原 ACK 的 identity/bytes 不变（本 fixture revision=1 时为 54 B，编码上限
58 B）；恢复 synthetic 原 metadata 便于重启核对。

真实 COM11 / MAC `8c:bf:ea:1b:96:14`、官方 Core **3.3.11 / IDF 5.5.5** 的输出：

```text
envelopeGate command=e pass=1 checks=3021 failedLine=0 attempts=4 writes=3 ackSame=1
// r: actual esp_restart(), then v in the new process/boot:
envelopeGate command=v pass=1 checks=6 failedLine=0 attempts=0 writes=0 ackSame=1
envelopeGate command=x pass=1 checks=6 failedLine=0 attempts=0 writes=0 ackSame=1
```

`checks` 是断言数，不是 3021 个独立测试；首次 e 的 4 次 attempts 包含 1 次有意的写入失败（不会触及
NVS、不生成 ACK）和 3 次成功 replace（首次 commit、合法 rewrap、恢复原 metadata）。重启后 v 的 counters
从 0 开始，读取持久记录并解封同一 fixture 后仍全 0，ACK 与 Service expected bytes 完全一致。
`writes` 是成功的 NVS adapter replace 次数，不等于底层 Flash program/page 操作次数。x 是精确测试 key
清理，不在 replace 计数里；空 namespace 元数据可以保留，不以擦整分区“清理”。

原 Hub `bl_eh_access/child` 352 B 在 e 前后、软件重启后、x 后摘要均为：
`2283d3064b58a2a9ed88e0fb548e451d469cd955b75717d62281f46eb40dc5a3`。
Service fixture 一致性校验、ESP32 编译通过；现有 Host Debug ASan 全套 **45/45**（28.68 s）通过。
诊断固件仅测试资源：**344,101 B Flash / 22,920 B globals**，bin SHA-256
`76f3a018e7defe267f1b8710e279d06a8e46462679235fe5fb0fe21a9d659301`，不计入正式库增量。

#### 10.43.3 现场恢复

诊断和恢复均仅写 app `0x10000`，没有 erase_flash、改 eFuse、擦 NVS、更新 bootloader 或分区表；两构建
分区表 SHA-256 都是 `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`。
已恢复原 §10.41 Hub bin `26d702b8f6560b6a61db8769a5ddb45f933cd3c819a2e39e93b4daa272480b13`，
esptool hash 校验通过。Broker UTC 00:31:17.814926 恢复 management_ready。诊断期间旧 MQTT keepalive
timeout 是移除 Hub app 的预期离线，不算零断线长稳；本批没有重编译/刷写 child、更新 APK 或部署服务器。

恢复后真实 App→Service/Broker→Hub→child 认证/同步/清理独立 **2/2**：

| operation | elapsed ms | PG Closed updated_at |
|---|---:|---:|
| `7ffacbd0f270e088e508529f1adf5a1d` | 14796 | 1788654742999 |
| `090650b563f00f056c06b8609ace7647` | 12746 | 1788654755796 |

每轮 App links/sessions/checkpoints=0；原 topology 仍 Active(5)/version 4/updated_at=1788615332648。
Broker 两窗口也均 closed/attempt_count=1。最终 Hub managementSubscribed=1、permit/relay idle、相关错误=0，
heap free/min/largest=105,416/92,784/63,476 B，loop/ipc0/NimBLE stack minimum=3688/1112/2776 B；
这是恢复后新 boot 的水位，不与前批长时间运行的 min heap 直接比较。child 未 reset，uptime=29,255,783 ms，
synced=1/adv=1/ACL=0，connects=56/disconnects=57，历史 indicationErrors=1/status=7 没有新增，
heap free/min=244,852/244,588 B；root 260 B SHA-256 仍为
`390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`，内容一致不等于物理零写。
这两轮只验证回到原产品组合后的正常功能，不将诊断 firmware 冒充生产 Hub 全量资源/并发门禁。
未新增暂存、commit 或 push；仅测试源码/fixture 与三端文档有本批变更。

#### 10.43.4 下一批与不能关闭的项目

1. **N3 enrollment requestId 的实际设备级 replay** 仍未覆盖，不复做已经通过的 ControllerMutation
   sequence replay。先核对当前可恢复 enrollment checkpoint 与实际设备状态；如果只有 Active child，
   必须先列明正常 owner reset/re-enrollment 对绑定/页面/现有 topology 的影响和恢复路径，再安排注入，
   不直接抹 access 或改 DB 状态。保持 fresh AEAD counter，exact EnrollRequest replay 与 changed bytes
   是请求语义验证，不是复用旧密文/nonce。
2. **真实 MQTT management 路径的 envelope 篡改/丢 ACK/redelivery** 仍待定点注入。使用未完成操作的
   正常交付边界或隔离测试层拦截发送副本，不修改已完成权威 ledger、不新增生产后门/无条件重发 API。
   分别核对 MQTT 到达、SDK 拒绝、无业务 NVS 变更、无 durable ACK，以及合法恢复后的账本对账。
3. 本批软件重启不是物理断电；D4b encrypted-NVS、N2 双 Hub、N4 force-stop、N5 剩余 pending restart
   和发布长稳继续独立待办。保持主线分层退出条件，不因单模块通过而关闭完整 N3/G2B-4。

### 10.44 N3 enrollment requestId 门禁准备与恢复边界（2026-09-06）

#### 10.44.1 实现与验证范围

- 在既有 `tests/hardware/edge_gateway_enrollment_negative_gate.mjs` 增加 `request-replay`，不新增同类
  probe/生产接口。删除该脚本重复 CDP 连接实现，复用 `gateway_app_gate.mjs`；所有临时 client/link/crypto
  hooks 退出时恢复原属性，复制的秘密清零，清理失败不能误判通过。
- 真实执行时仍由正式 App 完成 Hello/Noise/grant issue/pending save。注入层先按正式客户端已计算保存的
  HMAC receipt 验证完整 canonical EnrollResponse，再以 **同 requestId=2、同明文、新 TX counter=1**
  重发；RX counter=1 解密后的完整响应必须相同。之后仅改 PresenceKey 内容、TX counter=2，必须拒绝。
  首次请求/响应 counter=0。云端 intentId、设备 requestId、Noise counter 不是同一序号空间。
- 测试额外计数器不回灌正式客户端；负测随后终止 Noise，禁止 Method 2/commit，正常 Close permit，保留
  已安装的原 pending credential/enrollment checkpoint，交给正式 `finish_enrollment` 恢复。不能把
  `request-replay` 归入旧 signature/tag 的“未安装可取消”分支，也不能因普通网络错误删除恢复凭据。
- 新 Node 自测 **20/20**（Windows Node 22 与 Linux Node 20.20.2 Broker 镜像）覆盖四种 fault、wrong first/replay
  receipt、bad AEAD tag、changed request 被接受、普通网络失败、错 counter、Close 失败、已有事务及误选，
  并验证关闭失败/取消响应身份不匹配时不删除恢复凭据。
  运行序列化 WebView action，实际 AES-GCM；App/link/storage 是模型，**不是实际 Noise/MCU/NVS 证据**。
  CI 增加无硬件自测，不改变发布包或 SDK 版本。
- C++ `test_ble_enrollment_main.cpp` 补三种同 ID 内容冲突（通过 decoder 定位 grant/secret/Presence 字段），
  每次拒绝后再次 exact replay，原回执/存储字节不变、replace 仍为一次；新 application 实例不能在已安装
  access 上重新开放 enrollment。Host Debug ASan **45/45（10.64 s）**、架构检查、脚本语法通过。
  这不表示设备具备可信时钟：过期边界仍是 Host 模拟时钟门禁。

#### 10.44.2 现场只读准入与正常链路基线

本批未烧录、未 reset/eraseAccess、未更换 APK/服务镜像，也没有提交/推送。ADB `196f65f9`、COM11 Hub、
COM12 child 保持 §10.43 的运行组合。App 开始时 idle，links/sessions=0、pending enrollment=null、
recoveries=[]、permit checkpoint=0；因此没有可继续的首次安装 Noise 事务。

另执行原 `edge_gateway_connection_gate` 一轮，无故障注入：operation
`cd67cae03016abc9399fa08867126c56`，App startedAt=1788656897745，14,536 ms 完成 Method 2、同步、Close，
links/sessions/checkpoints=0；PG permit=closed，updated_at=1788656906486。时间耗时取 App 同一时钟，
不与数据库绝对时刻相减。原 topology `d01103b0b3880397268c139a492785fd` 保持 Active(5)/version 4，
updated_at=1788615332648。COM12 只读 access witness 仍为 260 B：
`390794348759419250ce2af10a19e3ae3e25e27cb4d8b072b0e1e1dbfea8e4c9`。
这轮只是现有绑定的连接基线，不是新的 enrollment replay 或发布稳定性验收。

#### 10.44.3 下一步与重置影响（必须先明确）

1. **优先未绑定测试 child**，用 exact instance + 当前 setup locator 执行上述脚本，避免影响现有 Active
   业务链路。没有未绑定板时，先确认是否允许通过正常设备 owner reset 流程重置当前测试 child；不能为了
   跑门禁直接串口清 access、擦 Flash 或改 DB。硬件 reset 按钮重启不等于 resetAccess。
2. 现有 child 正常重置会失去本地 Admin/Gateway/Presence 凭据；保留 deviceInstance 的同 owner/tenant
   重认领由 Service 复用 logicalDeviceId（不会因此自动新建设备页面），但 **不会保留旧安全上下文**。
   commit 更新 accessEpoch/Presence/Admin，并让旧 topology 进入 supersession 的 Revoking→Detached；
   旧 Hub credential 必须退役，随后通过正常 attach 重建 Gateway credential/topology，不能手改回 Active。
   页面身份可沿用，但页面内容/在线状态及四端实际恢复仍需验收，不能承诺无缝控制不中断。
3. 注入后先核对真实设备拒绝、首次持久化与重放间的 NVS 内容/写入次数、Service intent/permit，再恢复
   原 pending Admin 的 Method 2→commit→attach→同步/控制。**单凭断连和脚本 PASS 不能关 N3**；receipt
   可能已安装时禁止删除 checkpoint/intent。跨 Noise session 不能继续旧 requestId 的 exact replay 例外。
4. 真机 enrollment 前置条件未满足时，可先推进 §10.43.4 的真实 MQTT management envelope 发送副本
   注入，仍不得更改已完成 ledger/新增生产重发后门；不要继续重复已通过的普通连接循环代替剩余负测。
   D4b、N2/N4/N5 与发布长稳边界保持不变。本批只关闭“enrollment 注入工具与自测准备”，不关闭真机 N3。

### 10.45 授权重置、真实 Enrollment replay 与 endpoint 会话回收（2026-09-06）

本批用户明确说明只有两块板，并授权重置当前测试子设备，取代 §10.44 的“待重置授权”前置条件。
仅对 **COM12 / XIAO ESP32S3 / 98:3d:ae:ee:7c:6c** 执行内部 probe 的 `a`→公开 `Blinker.resetAccess()`。
这会删除原 Admin/Gateway/Presence access root，不能恢复原秘密；由正常重认领重建。未擦整片 Flash、
未修改 DB/已完成 ledger，COM11 Hub 的 DeviceKey/WiFi/固件不变。设备 instance 始终为
`fb42653e067ea9e4ab3b627db5975846`，Service 复用 logical child `ble_8Zc1CXfpwQtN1NqFWR6BvQ`；
没有用 MAC 合并账号设备。App 页面布局不在本次断言范围。

#### 10.45.1 先红后绿：释放资源时也必须释放所属会话身份

首次真实重放返回相同 receipt、NVS 仅一次安装，但 changed-request 之后 child **Fault / StateConflict(18)**、
停止广播。原因是 `BleLocalProvisioningEndpoint::poll()` 的多个失败分支只 stop channel；底层 stop 并不保证
回调 onDisconnected，endpoint 自己的 `activeSessionId_` 残留。上层据此继续重启 Provisioning，而 durable
enrollment 已完成，`beginSession()` 正确拒绝重复开启，于是卡入 Fault。

最小修正收拢重复 poll/flush 失败出口：stop channel 后清本层 sessionId。模式选择仍由原生命周期基于 durable
completion 完成；没有放宽 grant/receipt 校验，没有新状态字段、超时、重试/API 或 ESP32 特判。
Host 补“真实断连/握手应用拒绝/完整 Noise 后 transport 应用拒绝”三路径：旧实现明确 RED，新实现
Debug ASan **45/45（最终复跑 9.64 s）**、架构检查通过。证据不能外推到所有断连历史的唯一根因。

内部 `AccessWriteWitness.h` 通过链接器包裹五个公开 ESP-IDF NVS 函数，仅观察单 RW handle 的
`bl_v2_access/root`；原参数/返回值不变、不打印 secret。仅测试 flag 打开；正常 build 不链接它。
该观测是 **NVS API set/commit 次数，不是物理 Flash 页写次数**；要求 observed=1、ambiguous=0、无重启。
计数与 NVS 摘要互补，不能把摘要不变单独算零写。

#### 10.45.2 三次独立尝试，不拼接为全过

1. 修复前：intent `e4d70dc79d9cbd3d0b4575a26233276a`，exact receipt 相同、首次安装一次，
   但 device Fault(18)，工具只报 UNEXPECTED_FAILURE，**失败**。保留 pending，刷入修复后正常恢复；
   新 topology `76ce36d5525727b63d62cf601eb3e16e` 达到 Active/version 4。
2. 修复后、旧工具断言：intent `8b5d0a88133a2a1a12a1a540b73e6498`，不再 Fault、NVS 增量一次；
   末尾 HTTP 收到 `EDGE_GATEWAY_PERMIT_JOIN_RELAY_NOT_READY`，旧工具仍要求 DISCONNECTED，**失败**。
   未在清理前取权威终态，不追认 PASS。正常恢复 topology `19c99b781e0338084cfdb7bdd9b3c46b` 至 Active。
3. 修复后、修正观测工具：仅前项完全恢复、无 pending/window 后再重置。setup locator
   `f4564f4c9282136d`；intent `e0d0133ff8630304d2c3f118e834a1f4`，accessEpoch **386054652**；
   permit `4a6cc35b3423aae5b2e77f2984c795d2`。完整负测 **24,205 ms，PASS**。

最终工具结果：issued=1 / sends=4 / receives=3 / decrypts=3，exactResponse=true，freshCounters=true，
changedRequestRejected=true；TX counter=0/1/2、RX=0/1，不重复 nonce/ciphertext。Method 2/commit=0；
installed pending/checkpoint 保留，links/sessions/permit checkpoints=0。

最后 terminal window 抢先于 relay 最后 ACK；工具 **Close 前**正常 account-bound GET 确认 exact
operation/Hub/adapter 的 `rejected`。仅 NOT_READY 不够：ready/expired、错身份、查询失败均失败，
Windows Node22 / Linux Node20 自测 **28/28**。changedWriteCompleted=false，不能说末包 HTTP ACK 已收到；
也不能单凭 Rejected 区分所有 ACL 失败原因。此次结合 canonical receipt、fresh counter、Host 语义与下列
MCU/NVS 证据验收，未给 App/Broker 添加特殊接受或重试逻辑。

最终 reset 后基线 attempts/writes/commits/erases=**3/3/5/2**，root 不存在；负测后三个请求合计为
**4/4/6/2**，observed=1 / ambiguous=0：只有一次 `set_blob` 和一次 `commit`，无额外 erase。
root=260 B，SHA-256=`91d80dcb7f9e2fab307c9e24000d16444b459d91bcdcbed8098127378d86966f`。
child uptime 1,102,509→1,168,558 ms 未重启，最终 Provisioning→Active，无 Fault/indication error。
PG 独立确认 intent grant_issued/未 commit、permit rejected（updated_at=1788659592621）；不是取消 intent。

#### 10.45.3 正常恢复、冻结与剩余交接问题

三轮均保留原 pending Admin，使用现有 recovery 完成 Method 2→commit→Gateway install/proof；未删除恢复
状态来绕过问题。**三次 commit 后同步均断连**（最后一次恢复调用 57,377 ms），但 intent 已 Active，
App 凭据已 active，checkpoint 转为 finish_attachment。随后正常推进 attachment 与 cloud proof 收敛，
不是一次连续成功。第二轮首次 attach 另有一次未精确分类错误、checkpoint 保留；本批不声称消除该项。

最终 topology `b462633c456a355bb1a33da0fccedf54` 正常进入 Active/version 4；原始
`d01103b0b3880397268c139a492785fd` 及中间两个 topology 均 Detached(10)/version 5。
最终 App recoveries/links/sessions/permit checkpoints 全为 0。新 Gateway 安装后 witness **5/5/7/2**，
root 摘要=`7f1666462df01891d8ddc3c0047e8cc72eaca2943a36ab543fe0f756c0fc1557`。

同官方 Core **3.3.11 / IDF 5.5.5** 编译并核对分区表，恢复不带写入观测的修复版正常 child：

| 固件 | SHA-256 | Arduino Flash / globals |
|---|---|---|
| fixed NVS witness（本次注入） | `23d5b9bf13f6f3956e03877285fd1cec1226f533d244b8e5f432fd40e8923650` | 695,776 / 34,920 B |
| fixed normal child（已恢复） | `fa9fc0b0e251d0c9c14008727a5deb06592d2d7bd98462256c7bdc32339a826d` | 695,220 / 34,896 B |

正常 Flash 比批前 695,240 B 少 20 B，globals 不变；不把测试 observer 开销算生产开销。只刷 app @0x10000，
分区表 SHA=`1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c` 不变，NVS 摘要刷前后相同。
APK 保持 §10.41 的 `786dfb77424aa9bc98915c5700a9d1c626ca9b8068d3d4e181dba072173f24cb`，Hub 保持
`26d702b8f6560b6a61db8769a5ddb45f933cd3c819a2e39e93b4daa272480b13`；Service/Broker 没有生产改动/部署。

恢复后原定两轮普通连接，**第一轮 Open 等 Ready / Close 均超时，首错即停，不算 2/2**；operation
`894481c05d8d5c69a09a57395aa0970f` 留一个 permit checkpoint、PG pending_close。原流程
`recoverPermitJoinWindows` 对账后 Closed/清理成功，没有重启、擦除凭据或直接删 checkpoint。
随后独立正常认证/Manifest+State 同步/关闭 **1/1（12,865 ms）**，operation
`1e0157a2c80380195032eab2b83bdfed`=Closed，App links/sessions/checkpoints=0，child access hash 仍不变。
再独立用正式 App `connectUsing` 经 Hub relay 执行 `command('button', null)`，ACK sequence=7，刷新 State
确认 **buttonPresses 0→1**；整轮 21,852 ms，关闭后 links/sessions/checkpoints=0。没有注入该业务帧，
这是 App→HTTP/Broker→Hub GATT→child 的真实控制；不是重新验收 account MQTT ChildRouteEnvelope 数据面。
两件事必须分开记录；前次开窗超时尚未定因，不能把恢复成功包装为该缺陷已修复。
Hub 只读快照 childState=4/secure=1、managementSubscribed=1，无 child/route/management error；
heapMin=47,916 B / largest=57,332 B，loop/ipc0/NimBLE 最低水位 **2,736/1,112/2,744 B**。
该快照不代表新一轮持续资源趋势或长稳通过。

**下一批顺序修订：**

1. 先收口重认领 commit 后的同步/交接：`BleDirectClient.enroll/resume` 在 commit+active 保存后仍同步，
   而旧 topology supersession 同期会改变 Hub 资源；这是待验证的竞争边界，不能仅据时间接近断言根因。
   补 exact operation 的 commit/terminal/SelectResult/session 时序和回归；区分所有权完成与可选数据同步，
   让 Gateway 编排复用统一职责，不扩大 timeout、不吞错误、不让 UI 要求用户再次 reset。
   另独立取普通开窗超时的 Hub management/Broker delivery/终态证据，不能只盯 BLE 扫描。
2. 本批关闭“同 Noise session 的 Enrollment requestId exact replay/changed content + 单次 NVS 安装”子项；
   继续 §10.43.4 的**真实 MQTT management envelope 发送副本**篡改/重投/ACK 门禁，再 N4/N5。
3. N2 双 Hub、D4b 加密存储/物理断电、Wio/Hub 发布长稳仍独立待办。软件编译与短恢复不代表四端完整
   N3/G2B-4 或生产可发布；本批未新增多 child 调度、协议、SDK fork 或恢复后门。

### 10.46 管理模块任务准入与重认领交接（2026-09-06）

#### 10.46.1 证据先于改 API

现场仍只有 COM11 Hub（`8c:bf:ea:1b:96:14`）和 COM12 child（`98:3d:ae:ee:7c:6c`）。
按本轮用户再次明确的 reset 授权，只清 child access，保留 instance `fb42653e067ea9e4ab3b627db5975846`
及逻辑身份 `ble_8Zc1CXfpwQtN1NqFWR6BvQ`。未整片擦除、未重置 Hub 配置、未改数据库或已完成 ledger。

原 APK/双板固件第一次正常重认领（无密文注入）得到 intent `c44e4a31e96198079f59216a49ee15bc`，
accessEpoch=2316502204；App 同一时钟观察：

| 阶段 | 开始→结束，相对 ms |
|---|---|
| Method 2 | 18,725→22,808 |
| 云端 commit | 22,810→23,060（250 ms） |
| Manifest/State synchronize | 23,106→72,935（49,829 ms 后 DISCONNECTED） |

commit 后 App credential 已 active，但 attach 尚未创建；原 checkpoint 正常恢复到 topology
`efcdd5f0451aabc5e2e1ecabcc0c42fd` Active/version 4 后才继续下一轮。观测只在 debug WebView 包裹
正式 client 的 issue/commit/authenticate/synchronize/close 和 attach-create，最终恢复全部方法；不修改
响应、设备协议、grant 或 retry，也未记录秘密。单次观测脚本当时在本机 `.local/gateway-enrollment-handoff.js`
（§10.48 已合并到 `.local/gateway-lifecycle-trace.js`，旧脚本不再保留），
不进入库发布包；精确 instance 在 issue 前核对，扫描候选不能代替身份。

源码及 Host 确认根因：`GatewayManagementControlMux` 仅检查 `pendingResult().empty()` 就调用
当前 handler.reset()。permit 发出 Ready 后该值为空，但窗口仍占用 GATT；commit 触发的旧 topology
Supersede 切换到 revocation family，静默抹掉了正在使用的窗口。空发送缓冲不等于任务结束。

#### 10.46.2 最小设备端修正

`IGatewayManagementControl::busy()` 是强制实现的内部职责合同，独立于 result 是否待发送：

- permit：窗口仍 open 即 busy；发 Ready、重复 Open 的回执均不释放租约。
- proof / revocation：当前 command 未完成即 busy。
- renewal：command 未完成或仍有延后 session-retire 清理即 busy。
- mux：只有 **!busy 且 pendingResult 为空** 才 reset 旧模块并交接；同 family 原有 duplicate/Close/
  recovery 处理不变。全局 stop/reset/授权失效仍有原显式清理路径。

其他 family 在此时返回原 StateConflict，**不缓存命令、不伪造成功 ACK**；Broker 复用既有 exact durable
重投。当前任务仍被 poll，Closed/Rejected/expiry 都可释放窗口；终态回执发送前仍禁止交接。这里不把
Cloud ACL 退役延后：云端先阻断旧 route；仅 Hub 本地任务交接等待现有有限窗口/command deadline。
没有新增 MCU 特判、wire 字段、队列、SDK fork 或用户 API；所有模块仍只依赖同一小接口。

Host 覆盖 published Ready→Supersede 必须拒绝且不 close、原任务继续 poll、同 family duplicate/Close、
Close/expiry/rejected 后先发送终态再可切换，以及 proof/revoke/renewal 的 busy 边界。旧实现明确 RED，
新 Debug ASan **45/45（最终 6.77 s）**、架构检查/diff check 通过。

只更新 Hub 应用分区 @0x10000，分区表仍为 §10.45 SHA；官方 Core3.3.11 / IDF5.5.5，
Flash **1,433,560 B（+212）**、globals **65,348 B（不变）**。新 Hub SHA-256：
`c4d2c20acbd8dbb8a9eebfc420fdaf119d21c9ba0611b1cb3f333f14b9d35f1d`。
child 继续 §10.45 normal `fa9fc0b0...`，APK 继续 §10.41 `786dfb77...`；App/Service/Broker 生产源码/部署未改。

#### 10.46.3 真机修复范围、当前恢复点和下一批

在旧失败完全恢复后，第二次授权 reset child。setup locator=`c2a72081a74be308`，intent
`94ca9215f8f20eee8e3c7f892d5a8b34`，accessEpoch=438288719。新 Hub、同 APK/child：Method2 ready
21,410 ms，commit 21,410→21,478；**synchronize 21,523→27,045（5,522 ms）正常完成**，
session/window 27,890 ms 关闭，最后 permit `746f580c07f7bb38c5ad66e292ceb9a4`=Closed。
因此关闭的是“Supersede 错误抢占入网窗口导致同步断连”子项，不能外推所有 BLE/开窗超时均已修复。

紧接着 attach-create 28,005→28,173 ms 返回 **EDGE_GATEWAY_TOPOLOGY_CONFLICT**，本轮整体交接仍未通过。
Service 的 live-topology 唯一约束正确阻止旧记录尚在退役时创建第二条；不得放宽索引/强改旧 state。
PG 旧记录 superseded_at=1788661353192、finalized_at=1788661362926；Broker 同旧 operation/version5/
phase4 正常 exact retry，最终 **retired / attempt_count=4**。这些为服务器侧同一时钟事实，不与 App
绝对时间相减；不声称四次 publish 都已被 Hub 接收。

原 checkpoint（没有换 operation）一次正常恢复后创建
**`7f7caeaf7da69fa2de810bc5a17c6dc2` / PendingAccessDelivery(2) / version1**，created_at=1788661462745。
手机此后从 `adb devices` 消失，下一次 CDP 请求尚未发出即 ECONNREFUSED。最后 App 可读时 links/sessions/
permit checkpoints=0，保留 `attachment:f3yur32mn6LegQvFoXxtwg` / finish_attachment。
这不是 force-stop 门禁，也没有完成新 Gateway credential install/proof/Active。
child root=260 B，SHA=`63bb52d69b870f2495825c4ddc201367f1dc08248460da592a75904befce903f`，
uptime1,839,295 ms、synced/adv=1、无 ACL/indication error，只有新 Admin access，**不要再次 reset**。

下一批按以下顺序，不把普通重连循环当成交接验收：

1. 手机接回 ADB 后，正式 `resumeRecovery` 继续上述 exact checkpoint；若过期按现有明确终态处理，不能
   删 pending/改 DB。核对同 operation 的 delivery/install/proof/Active、旧 Detached 和真实 button/State。
2. 先冻结 Service/App 的 **retirement-pending** 合同再实施：仅同 owner/tenant/instance 的新 accessEpoch
   正在取代旧 epoch，才可表示“等待旧绑定退役”；普通 Active、不同 Hub/owner、幂等内容冲突必须继续硬拒绝。
   复用现有非 secret checkpoint/等待入口，不伪造新 topology、使用泛化 409 重试或增加 sleep。
3. 普通 Open/Close 超时仍需 exact delivery/Hub 准入证据；本批未完成修复后完整正常循环。
   然后继续 N3 真实 MQTT envelope 发送副本注入、N4/N5；N2 双 Hub、D4b 和发布长稳继续独立阻断。

### 10.47 旧绑定退役等待合同（2026-09-06）

本轮 ADB 恢复后，§10.46 的同一 operation `7f7caeaf7da69fa2de810bc5a17c6dc2` 已经正式
resumeRecovery → PendingGatewayProof/version3 → Active/version4；checkpoint 正常清除。真实 Hub relay
执行 button 后 buttonPresses **1→2**，未重置、改 DB 或替换 operation 来绕过恢复。

冻结最小合同，不新增 topology 状态或设备 wire：

- Service 仍先验证当前 owner、tenant、Hub profile、child instance 和已确认 Admin epoch。创建新绑定时，
  仅 live-topology 唯一索引冲突且旧记录为同环境/tenant/owner/Hub/instance/controller 的 Revoking，
  有 supersession receipt、尚未 finalized、旧 epoch 不同且 supersededByAccessEpoch 精确等于当前新 epoch、
  credentialVersion 前进，才返回 **HTTP 409 / EDGE_GATEWAY_RETIREMENT_PENDING / data:null**。
- 在数据库冲突边界再次检查当前 confirmed Presence 和双方 owner；已存在的 operation、其他唯一约束、
  普通 Active/撤销、不同 Hub/owner/epoch/instance 或幂等冲突，不转换为等待。既有唯一索引不变。
- 409 不代表新 operation 已被 Service 接受或预留；不创建占位 topology、不下发新凭据、不提前 Detached。
  旧退役 worker/Hub exact ACK 路径不变，查询不返回旧密文或 secret。
- App 编排层只将上述精确 code + HTTP 状态映射为有类型的 retirementPending 结果；保留原非 secret
  checkpoint，复用“继续确认”，再次使用相同 operation/body 正常 create。低层 attach API/协议保持不变。
  不泛化重试 409，不增加定时 sleep，不重新认领，不为等待持有 BLE 窗口。账户切换仍立即拒绝旧结果。
- 合并首次绑定和恢复的 cloud-first 入口：先保存 checkpoint，再走既有 advanceCloud，去掉重复 create。
  页面明确显示等待旧绑定退役；只有真实 Active 才可显示已接入。

实施验收：Service 真实隔离 PostgreSQL 冲突矩阵 + HTTP；App 首次/恢复等待、同 operation 恢复、账户隔离、
普通冲突仍失败及页面测试；之后按本轮授权只 reset 当前 child access（不擦 Flash/Hub），完成正式重认领交接。
#### 10.47.1 软件结果与部署

- Service：真实隔离 PostgreSQL 冲突矩阵通过；测试进程仅禁用分类分支时 RED，恢复后 GREEN。覆盖身份/
  Hub/controller/version、普通 Active/Revoking/rollback、旧/新 epoch、已有 operation 和真实 finalized 后准入。
  复用 test-postgres-helper 独立 schema 并清理，不改联调数据。既有完整 access PostgreSQL、HTTP 和 tsc 通过。
- App：9 files / **58 tests** 全通过，含首次/恢复等待、相同 request 恢复、普通冲突/错误 HTTP 状态拒绝、
  账户切换、窗口/relay/checkpoint、页面等待；定向 ESLint、local build 和 assembleDebug 通过。
  测试曾修正 structuredClone 的跨 realm typed-array 断言及 finally 清零后的 mock 参数观察时点，不放宽生产检查。
- Service `storePending` 只在指定唯一冲突边界查询公开元数据，不额外解密；创建失败已有 finally 清零新密封材料。
  `ownerUserId` 仅作为内部准入上下文传递，不加入 device envelope。无 schema migration、轮询队列或新设备内存。
- App `completeAttachment` 统一编排层的 waiting 结果；checkpoint 保存后只走一次 advanceCloud。低层 API/
  relay 不混入页面状态，页面只复用既有 waiting/continue，Active 仍来自真实 topology。
- 仅本机 Gateway 更新，部署前比较原容器与 Compose 的环境值，差异为零；新镜像
  `sha256:d28dbf101d35a4e06419515d845eb3f7dff1cfc954cbcd6da90116379e2ba18c`，health=healthy。
  Broker 仍 §10.42 `ef44e103...`；Hub 仍 §10.46 `c4d2c20a...`，child 仍 §10.45 `fa9fc0b0...`。
  APK（保留数据覆盖安装）SHA=`8a1a79671dc0c8eed61bb7fbf9f049c7d68ede9f0701b97a8e62539305dccee2`。
  Docker 构建报告既有依赖审计 3 moderate / 5 high，未在本批执行自动升级；不能据此宣称依赖发布审计完成。

#### 10.47.2 授权 reset、真实等待与恢复边界

先核对原 Active、App 无 checkpoint/link/session/permit、exact instance，再串口 `a`：resetAccess=1/error=0，
access root 不存在（NVS 4354），重新广播 Provisioning；不整片擦除、不重置 Hub。本批仅执行这一次 reset。
正常重认领 intent=`8ddec624ff56e7fa0cc72a444ae92704`，accessEpoch=2351953302，instance/logicalId 不变。

同 App 时钟：Method2 ready=22,711 ms，commit=22,712→22,804，synchronize=22,836→28,394（**5,558 ms**），
session/window 于 29,106 关闭；attach-create=29,191→29,300 返回 **EDGE_GATEWAY_RETIREMENT_PENDING**。
29,351 ms 整个调用返回有类型的 waiting、credential active、links/sessions/permit=0，不抛一般绑定错误。
真实 DOM 为“设备已认领，等待旧绑定退役”，新 topology 尚不存在；checkpoint 为
`attachment:GCHt2rPETuUNxPZCvx1u-w`，operation=`1821eddab3c44ee50dc4f642bf1d6efb`。

旧 `7f7caeaf7da69fa2de810bc5a17c6dc2` 正式 supersede→Detached/version5，Broker phase4/retired/attempts3；
PG superseded_at=1788667224478，finalized_at=1788667232340。没有人工补 ACK 或修改 ledger。

在新 create 尚未成立的等待边界，真实 `am force-stop` 测试 App，再正常启动：登录与同 checkpoint 保留，
正式页面 resume（280 ms）建立同 operation 的 PendingAccessDelivery，未重新 enrollment。
后续**不是一次顺畅成功**：

1. continueWaiting 16,477 ms 报 PERMIT_JOIN_DISCONNECTED；当时 PendingChildInstall/version2，窗口正常 Closed。
2. 增加本机只读时序观察，下一次恢复 7,279 ms 在 Method2 阶段失败，初版观察器未识别具体错误码；不能臆测
   为同一断连或设备鉴权失败。checkpoint 保留、links/sessions/permit=0，无新增认领/重置。
3. 再次正式恢复，完整 Admin Method2/同步/安装 receipt、独立 Gateway Method2/同步/关闭和 receipt commit
   **32,657 ms** 到 PendingGatewayProof/version3；继续确认到 **Active/version4**，checkpoint 正常清除。
   期间 session.close 后 receive pump 报 DISCONNECTED 是已结束会话的清理，不等于第 1 项在途失败。

这证明 retirement 等待、force-stop 前创建空隙的 checkpoint 持久化和同 operation 最终恢复；不证明整个 N4
或首次 onboarding 稳定性通过。只读观察脚本留本机 `.local`，不进入 Arduino 库/App 发布包。

后续优先级：先带 exact window/BBP header（无 body/secret）观测收口普通连接与 Method2 间歇失败，再继续
N3 MQTT envelope 真实发送副本注入及其余 N4/N5。N2 双 Hub、D4b 专用加密板、发布长稳仍独立待办。

#### 10.47.3 最终独立基线复测

正常 gate `--count 3` 全部成功：13,022 / 13,568 / 12,947 ms（合计 **39,537 ms**）。permit operations
`5d482058978abbf0fa5279503e44b3f7`、`ba3968bdd4a541a38a789f9c05a2b14a`、
`c4e98434c6a761dfd61594918a780ca4` 均由正式 Close 收口；每轮 links/sessions/checkpoints=0。
然后独立 Hub relay `command('button', null)` 返回合法 ACK，refresh 确认 buttonPresses **2→3**（20,987 ms）。
这不是 account MQTT ChildRouteEnvelope 负测，也不能覆盖/撤销 §10.47.2 的两次失败。

最终 App phase=ready、credential=active、recoveries/links/sessions/permits=0；新 topology 仍 Active/version4。
child root260 B SHA=`c9199eaaadec4bb6625d9080b286d3ef436ff29d7be4708baa40b6e7b83563ed`，
heap244,680/min240,624；该摘要在本轮第一次恢复失败后的观察与最终一致，不由此推导实际 Flash 写次数。
test sketch 的累计 BLE witness 计数在 access reset 后未变化，本轮不把这些计数当作连接次数或零错误证据；
连接成功以 App/Hub exact operation、真实 ACK/State 为准。后续诊断必要时先核对 test-only listener 生命周期。
Hub observed heapFree105,360/min52,008、loopStackMin2,736、ipc0=1,112、NimBLE=2,736，management 已订阅且无错误；
只是本轮水位观察，不替代长稳。未新增设备产品代码、未烧录双板、未暂存/提交/推送。

### 10.48 授权子设备重置、阶段观测和正常十轮（2026-09-06）

#### 10.48.1 本轮范围与实际恢复

按用户**本轮新的明确授权**，仅在已核对身份的 COM12 XIAO child 上发送一次 `a`（resetAccess），
COM11 XIAO Hub 未重置。保留 deviceInstanceId/同 owner/logicalDeviceId；不 erase flash、不改 Hub NVS、
不删除 App checkpoint、不人工更新数据库/ledger。前一批“不要再次 reset”是当时 pending 的保护措施，
不能视为之后每次继续的固定测试步骤。本轮重置后已重新绑定，下一批不再重置。

- reset 后 `resetAccess=1/error0`；read-only access witness 返回 NVS not found，进入 Provisioning。
- 走 App 正式 enrollment，intent=`111af53dc44be09b341012a99cc781df`、新 accessEpoch=`3279301365`；
  ownership commit 后初次 Manifest/State 同步 **5.774 s**，最后准确返回 retirementPending，未吞掉错误。
- checkpoint=`attachment:XuCpU0Pe6CGtj_PdqgSB7g` 保留。旧
  `1821eddab3c44ee50dc4f642bf1d6efb` 按正式 supersession 变为 Detached/version5，
  `accessSupersededAt=1788669291696`、`supersessionFinalizedAt=1788669302750`。
- 原 checkpoint 正常继续到新 `5ee0a95343dee821ad8ff3ddaa0481ee`：PendingAccessDelivery→
  Admin Method 2/Manifest/State→fresh control nonce/安装→独立 Gateway Method 2/Manifest/State→
  receipt→PendingGatewayProof→Active/version4；安装阶段约 **31.509 s**，没有失败后重跑。
  `childInstalledAt=1788669600287`、`gatewayProvenAt=1788669605121`。
- 本轮不是 force-stop 门禁，也没有重做 enrollment replay 注入；只是验证授权重置后的真实正常路径。

身份仍为 Hub `device_4af104c0-4a84-4e85-9009-4c2390c9e5a3`，child
`ble_8Zc1CXfpwQtN1NqFWR6BvQ`，instance=`fb42653e067ea9e4ab3b627db5975846`。
双板固件延续 §10.45/§10.46，App/Gateway 延续 §10.47，未烧录、重装或重新部署。

#### 10.48.2 工具收口与独立验证

保持产品层不变，只补既有 `tests/hardware/edge_gateway_connection_gate.mjs` 的诊断边界：

1. `authenticate-start/ready/failed` 包住真正的 Method 2，旧 `method2-ready` 改为
   `connect-synchronized`：`connectUsing()` 已包含初次同步，不能把同步失败误说成认证失败。
2. 认证前记录只保存 record 长度和 BBP/2 固定公开帧头；认证后的原帧级观测仍不保存正文。
   失败最多保留 32 条 record 元数据；正常成功删除详细 trace，错误即停止本批，不隐式重试。
3. 将已知静态 CBOR 解码错误映射成固定 gate code，未知 HTTP/crypto message 继续隐藏；不记录
   nonce/proof/key/body，不修改 wire、超时、App 响应或设备参数。
4. 记录 receive 结束时是否已 `closing`。正常主动关闭中的 receive-pump DISCONNECTED 不是异常断线，
   必须结合之前是否已同步成功及 Close 时序判断。临时方法 hook 全部恢复。
5. 新增可导入、可序列化的 observer 自测，覆盖阶段顺序、认证解码失败、首次同步失败、秘密不输出、
   失败停批及 hooks 恢复。和原 enrollment negative 工具合计 **31/31**；加入同一 CI Node job。
   这些是工具模型自测，不能冒充真 BLE/MCU 测试。架构检查通过。

本机两个重复的一次性 enrollment/recovery observer 合并为 `.local/gateway-lifecycle-trace.js`，
通过 exact Hub/child/instance 和 enroll/resume 模式使用，不内嵌某个旧 pending operation。
正式重连门禁继续只有一个入口，不创建第二套生产连接/恢复实现。

真实正常 `--count 10`（无 fault）通过：逐轮耗时 ms 为
`12580,14330,16036,12696,12798,12727,12950,13241,12645,12926`，累计 **132.929 s**。
每轮唯一 operation、认证/同步/Close 成功、links/sessions/checkpoints=0；PG 按 exact 十个 operation 核对
**10 个 Closed**（不可用 App/Server 时间戳截取代替 exact ID 对账）。第一/最后 window 分别为
`0ac97d6ed682297b283472e404ca0c48` / `19952ed1a9925426f260c5f6ccd46120`。

随后独立真实 button command ACK sequence7，State `buttonPresses:3→4`；整次连接/读取/控制/刷新/关闭
**21.419 s**。这是 App→HTTP permit relay→MQTTS Hub→BLE child，不是 account MQTT
ChildRouteEnvelope 负测，也不是手机直连 BLE 耗时。

最终 App 无 recovery/link/session/permit 残留，新 topology Active/version4。child root260 B 的 SHA
`601e2256598d3756bbcc263d02ab416db2f0c3ea884d782d7f18e36a9cb5a8dc` 在按钮操作前后相同；
不据此推断 Flash 写次数。child heap244,680/min240,624，最终 adv1/acl0；Hub observed
heap105,380/min50,668/largest57,332，loopStackMin2,736、ipc0=1,112、NimBLE=2,736。
这些是短时读数，不是 release soak。无新增产品代码、无暂存/提交/推送。

#### 10.48.3 未关闭项与下一批

本轮没有复现 §10.47 的普通连接断线/Method2 未分类失败；不能用后来的十轮通过抹去历史失败或宣称根因修复。
当前无需扩大 SDK fork、RAM、协议、重试次数或长时间测试。下一批优先：

1. 核对 **test-only GAP listener 在 host stop/start 后的注册生命周期**。现有累计 connects/disconnects
   仍停在4，不能当作十次连接/零 indication 错误的证据，也不能由旧 `lastDisconnect` 推断当前断线原因。
   修订观测必须与正式 BLE adapter 解耦，先确认注册边界，再补真机阶段对应证据；不要为诊断再次清 access。
2. 使用本批准确阶段/公开 header 观测，针对上轮 handoff/首次恢复边界做有界复现；错误立即保留
   exact window/阶段/状态。定位后增加确定性回归与最小修正，不以无限普通重连替代根因分析。
3. 再回 **N3 真实 MQTT envelope 副本注入**和剩余 N4/N5；N2 双 Hub、D4b 专用加密板、发布长稳
   继续独立待办，不把本批正常控制面的通过扩大为全部 Gateway production ready。

### 10.49 Direct 与 Gateway 共用设备合同及交叉门禁（复用通过；连接交替未通过，2026-09-06）

#### 10.49.1 主线优先级与复用边界

本轮继续 G2B/Gateway 主线。只补与其直接相关的 BLE Direct 基础交叉回归，不先重做 ESP32/UNO R4/Wio
全部 WiFi/BLE/WiFiProv 验收，也不扩张协议或多 child 调度。Hub 的 WiFi/MQTTS 登录与实际子设备 route
交互属于本轮环境基线；普通 WiFi 手动 Key、BLE 配网和其他 MCU 在相关模块变更或发布前另行回归。

当前 `BlinkerBLE.h`→`PlatformBleOnlyComposition` 已使用统一 DeviceAccessStore、ControllerControl、
Method 2/DirectSecure/BBP2/Manifest/State/Command。Gateway child 不是另一种 sketch 或产品身份：

- 未绑定网关：手机持 Admin（分享按 Shared）凭据直接认证、读取、控制。
- 绑定网关：owner 为原 logicalDeviceId/deviceInstanceId 创建 topology，通过签名 grant 安装独立
  Gateway 凭据；Hub 持有这台 child 的 Gateway secret/PresenceKey，完成独立证明后路由才 Active。
- 绑定后：Admin/Shared/Gateway 的授权材料独立，手机不使用 Gateway secret 做普通直连，Hub 不持
  手机 Admin secret；组件、manifest、logical identity 不变，不重新创建设备页面，不以 MAC 授权。
- 既有 Direct 设备正常 attach **不 reset、不重新 enrollment、不换固件**。本轮 reset 是用户明确授权的
  首次接入测试；reset 会换 access epoch 并退役原 Hub 绑定，不能用来“转换模式”。
- 单连接 child 是两条路径按需轮流使用，不承诺手机与 Hub 同时占有 BLE；云端 online/topology Active
  和当前 BLE ACL 是不同状态。本轮要分开记录手机直连与 account MQTT Gateway route，不能用
  App Admin 经 permit relay 的成功冒充 Hub Gateway credential 数据面成功。
- 此结论只适用于当前 V2/三槽 access/control 合同，不覆盖 dev_edu JSON、HC05 SPP、第三方任意 BLE
  或 BLE Mesh；Wio 的 preview/发布长稳边界仍独立有效。

#### 10.49.2 执行顺序与诊断边界

1. 只修内部 probe：官方 IDF5.5.5 的 esp-nimble commit
   `685675c0128deafdd201c9eb82e61d227364646c` 在 `ble_gap_init()` 清空 listener list，
   原 setup 一次注册会在 access reset/Provisioning→Direct 的 Host 重启后失效。
   使用测试专用 link-time `--wrap=ble_gap_init` 在成功 init 后、Host task 启动前注册，保持原返回值；
   不在 loop 与 Host 的 listener 遍历并发修改，不 fork SDK、不增加生产生命周期接口或后台定时器。
   代码来源：[官方 ble_gap.c](https://github.com/espressif/esp-nimble/blob/685675c0128deafdd201c9eb82e61d227364646c/nimble/host/src/ble_gap.c)。
2. probe 启用 `BLINKER_TEST_GAP_WITNESS` 时记录 Host 次数、注册结果、累计连接/断线和公开原因；
   不打印 payload/key/address；普通编译删除旧一次性 listener，明确 `observed=0`，不输出失效计数。
   先编译/link-map/实际 Host restart 验证，再把计数用于测试证据。
3. 保留两板身份、Hub 配置/原凭据，先核对 application/partition；只给 child 更新 probe application。
   按本轮授权只做一次 resetAccess，手机正式 Direct enrollment 后先控制，再 attach 这个已有设备。
4. 完成独立 Gateway proof/Active，手机 Direct 和 account MQTT route 分别控制并验证同一状态。
   错误即停在原 operation/credential 恢复，不增加通用 retry、清库或重复 reset。实测发现连接占用缺口后，
   优先按 §10.49.5 修正，再回 N3 MQTT envelope 副本负测及 N4/N5；N2/D4b/长稳不能据此关闭。

#### 10.49.3 本轮实际交叉结果与失败边界

固定两板仍为 COM11 Hub `8c:bf:ea:1b:96:14`、COM12 child `98:3d:ae:ee:7c:6c`。这些地址仅用于烧录
定位，不作授权/设备身份。Hub logicalDeviceId=`device_4af104c0-4a84-4e85-9009-4c2390c9e5a3`，
child logicalDeviceId=`ble_8Zc1CXfpwQtN1NqFWR6BvQ`，instance=`fb42653e067ea9e4ab3b627db5975846`。

| 步骤 | 本轮证据 | 结论 |
| --- | --- | --- |
| 精确 child reset | 仅 COM12 `resetAccess` 一次，NVS access record NotFound；witness Host starts 1→2、注册结果 0 | 按授权清除当前 child access，不擦整片 Flash、不重置 Hub；重新认领前旧 access 不可恢复 |
| 手机原生 Direct enrollment | intent=`5c49bd19ff993aa3e46ca763d18e3ea7`，新 epoch2145337234；Method2 859 ms，commit 后同步 895 ms；button sequence7、计数 **0→1**，含发现/认领/控制总计 16,338 ms | 相同 logical identity；手机 Admin 原生 BLE，不经过 Hub relay |
| Host 重启观测 | Provisioning→Direct 后 hostStarts=3、observed=1、init/listener=0、连接/断线均可继续计数 | 修复后的 test-only witness 经实际 Host 重启验证，不再使用一次注册的失效计数 |
| 旧绑定退役 | `5ee0a95343dee821ad8ff3ddaa0481ee` → Detached/version5 | 未手工改 DB、删 checkpoint 或放宽唯一绑定约束 |
| 已有 Direct child attach | public `DeviceV2EdgeGatewayService.create` 创建 `8cb2bcdda4da10afaff0ed1e36e8bf2d`；原 `attachment:jLK83aTaEK-v8O0eNui_LQ` 正常 resume | 不再次 reset/enrollment/刷 child 固件；单独安装 Gateway 凭据，不替换 Admin 凭据 |
| 安装首次失败 | window=`2648255fb7efaa974181c7dacadde7ed`；Method2/同步成功，254 B mutation 发出后主动清理先发生，App 未收到 176 B receipt，17,050 ms 失败 | child access 摘要已改变，PG 仍 PendingChildInstall/version2；不是“凭据未安装”，也不能宣布原始断连原因已找到 |
| 同操作正常恢复 | Admin window=`97266f986087cac4a4d7dca7c95de7a3` 安装/receipt 完成；独立 Gateway window=`137739cbbb421ebd3a55d1a4e27c68a6` Method2/同步完成；32,415 ms 到 PendingGatewayProof/version3，再云端继续到 **Active/version4** | 原 checkpoint 正常清理；恢复成功不能拼成首次无故障 |
| 绑定后手机 Direct | `DeviceV2BleService.ensureReady` 原生扫描 **15,043 ms** 后 `BLE_DIRECT_SCAN_TIMEOUT`；child adv=0/acl=1，Hub childState4/secure1 | **未通过**；不是手机与 Hub 同时连接能力承诺，而是空闲 Hub 没有释放连接 |
| 真实 account MQTT 子设备控制 | `DeviceV2Service.ensureReady/command`，ready 350 ms，sequence14、button **1→2**；总计 2,177 ms | App 用户 MQTT→Broker→Hub MQTTS route→Gateway credential BLE→child 成功；Hub 当时已连接，不能当作冷连接耗时 |

最后只读对账：PG 新 topology Active5/version4，childInstalledAt=`1788671776729`、
gatewayProvenAt=`1788671781616`；旧 topology Detached10/version5。App links/sessions/permits/recoveries
均为 0。child uptime1,304,474 ms 时仍 synced1/adv0/acl1，hostStarts3、connects6/disconnects5、
indicationErrors0、heap244744/min240688 B，access record260 B、SHA256
`df970f694ea52625530d15e9f4609f1758f27c14206c5c661077b04c802059bb`，与安装后一致。
其中 acl 是 SDK 当前连接观测，不能把 App 无会话误当设备无连接。
Hub 同期只读输出 childState4/secure1、controlPending0、permitState0、relayActive0，managementSubscribed1；
heapFree105360/min50668/largest55284 B，loopStackMin2736、ipc0StackMin1112、nimbleStackMin2736 B。
这证明本现场没有正在使用 permit/control 来合理解释持续占用，不是新的任务栈不足结论。

App 首个异常可能被 `fail()` 的异步 disconnect 所触发的并发 receive 异常覆盖。本轮补确定性回归：
发送失败→disconnect 拒绝接收→排队发送，三者必须保留同一首因；旧实现 RED，新实现 GREEN。
仅在 secure-channel 内保存首个终态错误，不新增重试、延长 timeout 或跳过加密/receipt 校验。
测试中的 `DOWN_ACK_MISMATCH` 是模拟首因，**不是本次首次安装失败的已确认原因**；真实失败仍待再次捕获。

UI 验收边界：本轮通过已安装 App 的正式服务与原生 BLE/MQTT 运行，不是伪造协议或读取缓存冒充控制。
但现有 `EdgeGatewayEnrollmentPage` 主要提供未认领设备发现/认领；“从已认领 Direct 设备列表选择并绑定
Hub”尚无对应页面入口。本轮借 CDP 调用正式 create/resume 验证服务合同，不算完整点击路径已完成。

#### 10.49.4 最小改动、验证及部署冻结

- 设备仅修改内部 `Esp32BleAccessResetProbe`：将诊断独立到 `BleGapWitness.h`，删除 setup 一次性 listener。
  正式 `src`、Hub composition、设备 wire/SDK 未改。可选观测使用
  `compiler.cpp.extra_flags=-DBLINKER_TEST_GAP_WITNESS` 与
  `compiler.c.elf.extra_flags=-Wl,--wrap=ble_gap_init`；不能覆盖 board 的 `build.extra_flags`，否则会丢失
  USB/core 等默认定义。发现该配置问题的首份 build 已丢弃，未烧录。
- Core3.3.11/IDF5.5.5 diagnostic build **695,380 B Flash / 34,912 B globals**，仅 application@0x10000
  烧入 COM12；事先核对 MAC/partition，未改 bootloader/partition/NVS。当前运行的 child bin SHA256：
  `fabf5648b127af6e96b685bff40679d6df017a7f3f699bf81d4a70813c6d146a`。
  普通无 hook 编译 **694,796 / 34,848 B**，SHA256
  `7bb11d23b802695ce78ded3d5adebba766687c805cf0a88d15e7ab9863fc5b34`，**仅编译、未烧入**。
  两个 build 的差额为可选测试观测成本，不是正式库 RAM/Flash 增量。
- Hub 未更新，仍 §10.46 bin SHA256
  `c4d2c20acbd8dbb8a9eebfc420fdaf119d21c9ba0611b1cb3f333f14b9d35f1d`；Service/Broker 未改源码/镜像。
  Service 仍 `sha256:d28dbf101d35a4e06419515d845eb3f7dff1cfc954cbcd6da90116379e2ba18c`，
  shard-1 仍 `sha256:ef44e103b1c61958d07307c692196036b1c8229f24ad5b0642b1f797320a0d28`。
- App secure-channel/session/permit-join-link/gateway service **47 tests**、ESLint、local Web/Android build
  通过；保留数据安装 APK SHA256
  `e716dcce7b6c9d8c45aea142045fd09f9764a92a029b5474a3b567b78149c5ca`。
  原 APK 的 HTTPS WebView 对本机返回的 `ws://` 正确拒绝（`DeviceV2TransportConfigError`），此前一次
  account 测试没有到达 Broker。本轮使用既有 `BLINKER_LOCAL_CLEARTEXT=1` 本机调试构建和 ADB reverse，
  不修改生产 HTTPS/WSS 准入，不改变 Hub 必须 MQTTS；登录/Direct 凭据经原 SecureStorage 保留。
- 连接观测工具自测 **31/31**、设备架构检查通过。未重跑全套设备 Host ASan，不把历史45/45算本轮新增证据。
  无自动提交、暂存或推送；本批不关闭 N3/N4/N5、N2 双 Hub、D4b 加密存储和发布长稳。

#### 10.49.5 修正后的主线：先连接占用解耦，再继续负测

**根本边界是授权不等于资源占用。** topology/route admission 说明允许转发；不能因此永久拥有 child
唯一连接。当前 `GatewayChildRouteBridge::poll()` 在 admitted+subscribed 时直接 start/connect 并持续
保持 secure；`GatewayProofCoordinator` 成功结果缓存也不释放 child，`Esp32EdgeHubProduct` 在线轮询
继续维持这套策略。两者共同导致空闲后手机 Direct 仍不可发现。只给 proof 加 disconnect 不能修复，
因为 route 下一次 poll 会重连。

也不能直接给现有 poll 增加一个固定断连 timer：当前 route 的下行在 child 非 secure 时直接拒绝，缺少
冷恢复的有界首请求交接；新 BLE session 的 Hello、云端协商代际、请求相关性和 Action replay 必须一起
保持正确。否则只是用“手机可见”换来“云端首条控制丢失/重复执行”。以下分批执行，不新建通用无限队列：

1. **L1：冻结需求/资源合同并先补失败测试。** protocol-independent runtime 区分 route authorization、
   正在使用连接的业务/管理任务、允许释放的空闲态。proof 回执缓存不拥有链路；管理事务/可靠请求未完成时
   不切断。用 fake clock 覆盖 idle 释放、无需求不重连、在途 ACK/control 保护、proof exact replay 不重复
   连接、过期/撤权终止。优先复用已有调度/时钟/单 child 状态，平台适配器仅负责 connect/disconnect。
2. **L2：实现有界冷恢复与错误语义，再启用空闲释放。** 先验证 exact Hub/child/epoch/version/digest/
   admission，合法的新需求才可唤起连接。首请求只使用明确上限的单槽/既有 workspace 或明确 backpressure
   配合 Broker 现有 ledger；冻结容量、deadline、清零和同请求重投合同后实施。认证/协商完成才转发，不跳过
   Manifest/State 校验，不让 receipt/Action 的未确认结果跨新 session 被盲目重放。若现有 BBP2 replay cache
   不能覆盖 ACK 丢失后的重连，先定义“结果未知”的正确恢复边界，不能宣称无条件 exactly-once。
3. **L3：三端配合与两板交叉门禁。** topology Active 保留授权，不等同当前 BLE ACL 或新鲜 online；
   App/Server 按路由新鲜度处理冷恢复/暂忙，不复制 logicalDeviceId、页面或改用 MAC 匹配。修正本轮首因
   观测后复现安装异常；在原绑定上完成 **手机 Direct→Gateway account MQTT→空闲释放→手机 Direct**，
   每段实际 Action/ACK/State 验证，无 reset/清库/临时停 Hub/开 permit 强制让路。先单轮，再有界重复，
   测量冷/热路径耗时及 heap/stack。持续流量下的公平占用/抢占策略另行明确，不能靠任意 timer 切在途请求。
4. **L4：补已有 Direct 设备的 App attach 入口。** 复用现有 account-bound create/resume/checkpoint，
   从当前 owner 已认领的合格设备选择 exact child 和 Hub；不把绑定误做重新认领，不增加 child wire。
   明确分享用户权限、已绑定/退役等待/恢复态，补真实 UI 点击测试，不扩张页面编辑器。
5. L1～L3 软件与交叉链路通过后回 **N3 MQTT envelope delivery tamper/expiry/replay、N4/N5**，L4 并入
   产品接入验收；全平台 WiFi/Direct 发布回归按受影响模块补齐。N2 需额外 Hub、D4b 需专用板、发布长稳仍
   各自独立待办，不能因两块板跑通或本轮 cloud 控制成功而关闭。

§10.49 当时仅完成观察修正、首因保护与交叉问题定位；后续 **L1 proof 子项进度见 §10.50**。普通 route
释放/冷恢复和交叉门禁仍未完成。不要将历史现场 reset 指令重复执行；本轮新授权与实际次数以最新批次为准。

### 10.50 L1 proof 资源所有权与回执缓存解耦（软件/真机子项完成，整体交替未通过，2026-09-06）

本批先修一个能独立验证的边界，不新增通用 lease 框架、后台 timer、SDK hook 或队列。现有 protocol-independent
`BasicGatewayProofCoordinator` 复用 `ChildSession::attemptId()`，记录且仅记录自己成功 connect 的 attempt：

- 成功、拒绝或过期结束 proof 时立即请求释放自建连接；最后一次 durable record 校验放在报告 Secure 前，
  防止认证过程中撤销/替换 access 后仍错误证明成功。
- 请求断连不等于已断开：保持 `busy()` 直到相同 attempt 进入 Idle/Stopped/Fault 或换代；Disconnecting
  不重复下发 cancel，WouldBlock 保留原回执/所有权，在正常 poll 中继续清理。同步 callback 也正确结束。
- cached result、markResultPublished、exact command replay、旧任务 reset 不重新建连，也不能断开后来由
  route/control 获取的其他 attempt。只验证既有匹配会话的 proof 不冒领该会话所有权。
- 删除旧“换 proof 命令即无条件 retireChild”的交叉副作用，统一终态清理出口。全 product stop/reset 的
  强制资源收尾仍由 composition 负责，不用返回回执代替物理关闭，不在用户 API 暴露 attempt 或 lease。

软件门禁先 RED（旧实现成功后 disconnectCalls=0），再 GREEN。Fake clock/session 覆盖：终态立即请求释放、
异步断连期间阻止新任务、回执保持 exact、30 s 空闲不重连、同步回调、断连 backpressure、时间失信/过期、
认证期间 access clear、旧 credential session 拒绝、无任务 reset/借用会话/新 attempt 不被误关。
另覆盖单调时钟 uint32 wrap 后仍等待完整 1,000 ms 的重连间隔、deadline 后不再重连；补测同样 ASan 通过。
本批 Host ASan **45/45**、独立 deterministic fuzz **200,000**、架构检查和工具自测 **31/31** 通过。

#### 10.50.1 本批重置与恢复点

硬件仍为 §10.49 的 COM11 Hub、COM12 child。本轮 reset 前 child access SHA256
`df970f694ea52625530d15e9f4609f1758f27c14206c5c661077b04c802059bb`，仍 adv0/acl1。
按用户新授权只发送一次 `resetAccess`；成功 error0，Host starts3→4、adv1/acl0，access NotFound4354。
只清 child access，未擦整片 Flash/Hub 凭据。手机正式 Direct enrollment intent
`95d829648c5b7b8bcc453d540011b815`，新 epoch3032216448，Method2 **914 ms**、commit 后同步 **969 ms**，
button sequence7/计数 **2→3**，含发现/认领/控制/关闭总计 **18,188 ms**。设备未 reboot，因此计数延续，
不能把 resetAccess 误当整机 reset。App active credential、link/session/permit/recovery 全部0。
原 `8cb2bcdda4da10afaff0ed1e36e8bf2d` 经正式退役到 Detached/version5；未改 DB/删 checkpoint。

后续只更新 COM11 Hub application，未再次 reset child。正式 create 建立
`4f84ebee1d286b51020dd659b2947521`，checkpoint=`attachment:T4Tr7h0oa1ECDdZZspR1IQ`。
同操作正常 resume 的 Admin window=`3b0215da1fb29424d2055f6f1f14a605`，254 B mutation/176 B receipt 成功；
独立 Gateway window=`3a9af8add864e5187cfae69308b7f2264`，Method2/同步成功。总计32,316 ms 到
PendingGatewayProof/version3，无传输异常重试；主动 close 时 receive 的 DISCONNECTED 是清理观测，不能误记为失败。

本次双串口只读采样实际记录到：

- 两次 permit relay 连接/关闭后 child connects10/disconnects10；Hub 自建 proof 进入 Secure，随后 Idle。
- 采样相对47,751 ms，Hub 输出 childState4/secure1 后紧接 childState1/secure0；相对48,091 ms child
  adv1/acl0、connects11/disconnects11，随后至少至54,088 ms 仍广播。**独立 proof 连接已释放**，不是停止 Hub
  或开启新 permit 强行让路；字段来自普通 Hub 状态与上一批 child test-only GAP witness。
- 后续仅云端 resume **223 ms** 到 Active/version4、清理 checkpoint；PG childInstalledAt=`1788674965427`、
  gatewayProvenAt=`1788674970232`。Active route admission 使 Hub 按当前旧策略重新连接 child。
- 真实 App account MQTT control ready1,038 ms、button sequence14、计数 **3→4**，总计 **3,012 ms**；
  不是 Admin permit relay。Hub 当时已连 child，此时间不算 route 冷恢复通过。
- 同绑定随后原生 Direct **15,038 ms** `BLE_DIRECT_SCAN_TIMEOUT`。child uptime3,616,944 ms 时 adv0/acl1、
  connects12/disconnects11、hostStarts5、indicationErrors0；这次仍由 Active route 保持连接，**交叉门禁失败**。

结束 App link/session/permit/recovery 全部0，PG 新 Active/version4、旧 Detached/version5。child access260 B，
SHA256=`3cb7c19e10e7639a751a67c88d12cec5897176c3e621caf0884c16653046588a`；安装前 Admin-only 摘要为
`6e8bce08bd25103d4fc4dcdab1f086d36d408d1517c4b9edad486814a0ad1cd8`，变化对应本次正常安装 Gateway credential。
当前仍同 logicalDeviceId/instance，不再次 reset。§10.49 的首次安装间歇异常本批未复现，**未定位/未宣称修复**。

部署冻结：Core3.3.11/IDF5.5.5，Hub Flash **1,433,756 B**、globals **65,348 B**，相对 §10.46/49
为 +196/0 B。artifact=`.build/esp32-edge-hub-l1/Esp32EdgeHubDualBoard.ino.bin`（位于 D:/codes 下），SHA256
`892214a2703ada33422818006e95f557289366afd2057c022004534aa3376a2a`。
烧录前验证 COM11 MAC 及分区摘要一致，仅写 application@0x10000、esptool hash 校验通过；保留原
`esp32-edge-hub-g2b` restore artifact，未写 bootloader/partition/eFuse、未清 Hub NVS。此内部 Hub probe 的
弱存储 override 仍不是 D4b 生产加密存储证据。
child 仍 §10.49 的 diagnostic bin `fabf5648...`；App 仍 `e716dcce...`（完整摘要见 §10.49.4），
Service/Broker 仍 §10.49 镜像。实际 Hub heapFree105360/min95296/largest63476 B、loopStackMin2604、
ipc0StackMin1112、nimbleStackMin2780 B；child heap244672/min240608 B。单轮水位不替代长稳。
本批无 App/后端生产源码变动，也无提交/暂存/推送。

#### 10.50.2 后续批次边界

本批只关闭 L1 的 proof 所有权子项；普通 route/admission 的共享占用、在途业务保护和冷恢复依然属于 L1/L2。
在启用 route idle 之前，必须先处理三个既有实现事实：

1. `GatewayRouteClient::poll()` 回调后清除唯一 ingress buffer；直接在 bridge 等待冷连接会丢失首请求。
   应先冻结“谁保留该有界请求、何时接受/清除”的明确合同，不能新增无界缓存或把 WouldBlock 当成功。
2. Broker `admitEdgeGatewayRoute` 仅投递 admission，实际 child virtual channel 由首次 Hello 建立；
   `isOpen` 目前只检查 Hub/context，不能证明 child 正在连接。首轮 bootstrap 与后续按需连接须分开处理，
   冷 Hello 的 session/state 代际及未确认 Action 的 ledger/replay 语义必须一起验证。
3. 现有 bridge 的 disconnected 回调会清除 recent request 映射；因此必须单独处理“Action 已执行、云端未收到
   ACK”再断连的情况。不能依赖一个固定空闲时间来保证 ACK 已被 Broker 接受，也不能未经证明跨 session
   重放该 Action。先测试 exact correlation/结果未知的有界恢复，再选择缓存或显式失败语义。

下一批从上述 route 合同与失败测试开始，再做最小有界实现；最后用同一绑定完成 Direct→Gateway→Direct。
不先增加页面重试、强停 Hub 让手机连接或继续长稳；已有 Direct attach 的 UI 入口、N3/N4/N5、N2/D4b/长稳
仍按 §10.49.5 独立排期。本批已关闭 proof 的软件与单次真机释放子项，不用它替代整个交叉门禁。

### 10.51 L2a 有界 ingress 所有权交接（2026-09-06）

本批对应 §10.50.2 第 1 项。先解决“谁持有尚未被接受的请求”，不将 admission 改为定时断连。
协议授权、物理会话生命周期、消息缓冲分别由各自模块负责，不新增第二份 frame/动态队列或公开 Arduino 参数。

#### 10.51.1 冻结内部合同

| 边界 | 实现 |
| --- | --- |
| MQTT → RouteClient | 只复用既有最大 envelope 单槽，复制原帧；槽满时不覆盖，重复到达不延长等待时间 |
| RouteClient → Bridge | receiver 返回 `Result`；Ok=已接受，WouldBlock=未接受/可 exact retry，其余=明确拒绝 |
| 等待上限 | 从首次收包起最多 1,000 ms，uint32 差值支持回绕；到期先清槽、不再调用 receiver，lastError=WouldBlock |
| 到期语义 | 只是本地 ingress 容量上限，不是业务执行超时/成功 ACK，也不替代 Broker 的 command ledger/重传期限 |
| 缓冲生命周期 | 成功、硬拒绝、到期、reset/reconfigure、换 receiver/context、离线或 MQTT generation 变化时 securely clear |
| 异步/重入 | receiver 仅借用 ByteView；允许 reset/unbind，但不递归 poll；不能保存指向该槽的长期引用 |
| Bridge 鉴权 | 每次尝试重新加载 durable record，复核 epoch/version/instance、admission 所属 MQTT generation 和 deliveryRevision |
| child sequence | 普通请求和管理请求均在 sendFrame 成功接受后才前移，pre-enqueue WouldBlock 不消耗 sequence |
| 已接受的密文 | GatewayChildSession 内部已持有待发密文时仍返回成功；不能把已加密/接受的记录当“未接受”重发 |

最初评估 5 秒，但本批核对 Broker `command.ts` 的 250/500/1000 ms 重试和最终 2000 ms 等待，
合计约 3750 ms；5 秒可能在调用方已 outcome_unknown 后才首次交给 BLE，因此已修订为 1 秒。
这是当前热连接发送背压的固定内部资源策略，**不是已冻结的冷连接等待预算或端到端过期保证**。
冷恢复引入前需结合 Broker 请求期限明确新合同，不能直接套用此窗口后增加 scan/retry 超时。
时钟通过已有 IClock 注入；不把 SDK、BLE 类型、时间 API 或拓扑解析加入 GatewayCloudMux/MQTT 通用层。

同时补齐快速 MQTT 换代的边界：RouteClient 的订阅/入站/出站按当前 generation 判断；Bridge 不允许旧
admission 转发新 MQTT 会话的数据，也不把旧 child 上报投向新会话。durable revision 变化后，尚未接受
的帧不能依赖上一 poll 的鉴权结果继续发送。这里没有增加跨 BLE session 的 Action 自动重放。

#### 10.51.2 软件证据

- 新增独立 `test_gateway_route_client_main.cpp`，使用真实 MqttFrameTransport/GatewayCloudMux/RouteClient，
  仅 MQTT 驱动与接收方是 fake。旧 poll 在 `hasPendingEnvelope()` 断言明确 RED，新实现 GREEN。
- 覆盖 exact bytes、不覆盖、接受/拒绝清除、1 秒边界/uint32 回绕、快速换代且未经过 offline poll、
  重新订阅失败、重新订阅前迟到数据、解绑/reset、非法 QoS/retain/空/过大输入及释放后槽可重用。
- Bridge 补普通/管理请求连续 **100 次 pre-enqueue WouldBlock** 不耗 sequence；换代和持久记录被替换后
  拒绝旧帧。旧 Bridge 的 fast-generation 用例 RED 后修正，旧上行也不泄入新 MQTT 会话。
- 最终全量 Host ASan **46/46**、**200,000** 确定性 fuzz、架构检查及既有联调工具 **31/31** 通过。
  不将 mock 背压计为真机注入；本批没有增大 MCU ingress buffer，也没有新动态队列。

#### 10.51.3 本批硬件恢复（不能隐去首次失败）

仅两块 XIAO ESP32S3。按本轮新授权，COM12 child `a` 执行一次：resetAccess=1/error0、access NotFound，
adv1/acl0；COM11 Hub 没有清 access、DeviceKey 或 WiFi。child 固件仍为 §10.49 test-only GAP witness。

原生 Direct enrollment intent=`c0c8b5251aab9233eea5cf6c01a1ef42`，新 epoch **213376463**，
logicalDeviceId 仍 `ble_8Zc1CXfpwQtN1NqFWR6BvQ`，instance 仍 `fb42653e067ea9e4ab3b627db5975846`。
首次在 22,568 ms 结束，首因 `BLE_DIRECT_RECEIVE_TIMEOUT`（Method2），凭据保留 pending。
随后串口读到 `assertion:ret == pdPASS` 和重新启动的日志，uptime 降低、button 计数回到 0。
尚无完整堆栈/故障时刻证据，**不能据此认定是 ipc0、某个 SDK 缺陷或将重启当作修复**。

没有再次 reset 或签发新 intent。使用既有 `resumeUsing`/client.resume、pending 凭据及 authenticated
Presence 匹配恢复，commit 后 active，真实 button **0→1**；links/sessions=0。本机 native recovery
观察适配仅把新 link 的 waitForMode 接到 discoverDirect(matcher)，因为原 native waitForMode 要求该
link 曾连接过目标；不能把“新 link 无 target”误判为设备未断开，也不以 MAC/首个扫描结果选择设备。
这是服务/native 联调，不是新增 App UI 恢复入口的验收。

恢复后 child access 260 B、SHA256=`98f9c1b41f80a88df44dd801845f54a626a4ec7f5dc07e2a07aad450b1f0e239`，
adv1/acl0、连接/断开3/3；旧 topology `4f84ebee1d286b51020dd659b2947521` 正常 Detached/version5。
当前子设备已认领，不再重复清 access。

**最终构建和同身份 Gateway 复测：**

- Core **3.3.11**，FQBN `esp32:esp32:XIAO_ESP32S3`，最终增量构建确认使用收紧为 1 秒的完整新源码。
  `D:/codes/.build/esp32-edge-hub-l2-ingress/Esp32EdgeHubDualBoard.ino.bin`，
  SHA256=`6d72802574a8c58b3bbf316df6777406a57a1035c31ec81b7d681dd9adc31b0a`。
  Flash **1,434,080 B**，globals **65,356 B**；较 §10.50 **+324 B/+8 B**。前一次编译不是最终部署产物。
- 烧录前核对 Hub COM11 MAC `8c:bf:ea:1b:96:14`、实际分区表与构建一致，SHA256=
  `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`。
  仅写 `0x10000` 应用，esptool hash verified；未改 bootloader/分区/NVS，保留 Hub DeviceKey/WiFi/access。
  检查和应用更新有正常软件/RTS 重启，不等于执行 Hub resetAccess。旧 §10.50 bin 留在原路径作恢复基线。
- 子设备不换固件、不再 enrollment，App 正式 create/resume API 创建新 topology
  `9bf02e5a0a3ff936963b937782e408c2`。Admin 安装/receipt、独立 Gateway Method2/同步及确认共 **32,461 ms**，
  进入 PendingGatewayProof/version3；后续仅 cloud resume **431 ms** 达到 Active/version4，无新 BLE 连接。
  PG childInstalledAt=`1788680390001`、gatewayProvenAt=`1788680394828`，epoch213376463。
- proof 后 child adv1/acl0、连接/断开 **6/6**；Active route 随后重新占用 ACL。
  真实 App account MQTT→Hub→child button **1→2**、ACK sequence14，ready **1125 ms**、总计 **2761 ms**。
  这不是 permit relay 控制，也不是冷连接/链路交替的证明。结束 App phase=ready，links/sessions/permits/
  recoveries 全为 0；没有强停 Hub 或再次 reset 来制造手机可见。
- 最后 child adv0/acl1、连接/断开 **7/6**，root 260 B、SHA256=
  `5540c085f8114a9e7b938fcaea7ea3e90865a13e87058058bd480f40d1bd3a0e`。
  Hub secure1/routeError0/managementError0；heapFree **105408**、heapMin **92748**、largest **63476**，
  loopStackMin **2736**、ipc0StackMin **1104**、nimbleStackMin **2820 B**。只是本次运行样本，不替代长稳/SDK准入。
- App/APK、Service/Broker 生产源码和镜像均未改；子设备仍是 test-only witness 固件，Hub 仍是既有弱存储
  override 的内部 probe，**不代表 D4b 加密存储生产准入**。未提交、推送或新增暂存。

#### 10.51.4 下一批准入

1. 保留同一绑定，不把每次“继续”作为再次 reset 授权；独立补首个 Method2/SDK assertion 的完整被动证据。
2. L2b：先给 Broker virtual channel 的 bootstrap、冷 Hello/state generation、命令已接受但 ACK 丢失的
   Action 结果未知写失败测试；再选有界恢复/显式失败语义，不能跨 BLE session 无条件重放。
   同时验证 command.ts 的 failPending/outcome_unknown 与 route context.commandCorrelation/commandFrame
   的退出关系：目前后者主要由响应或整个 context 退役清除，不能假定 ledger 超时自动释放路由关联槽。
3. 以上通过后实现 route 正常空闲释放，验证 Direct→Gateway→Direct；不强停 Hub 制造手机可见。
4. L4 已认领 Direct attach/恢复的 UI 入口及 N3/N4/N5、N2/D4b/长稳仍独立排期，不因本批缓冲门禁通过而关闭。

### 10.52 L2b-1 Broker 命令任务与传输槽解耦（2026-09-06）

本批关闭 §10.51.4 中的超时关联槽退出子项，不把 Broker 槽可复用等同于 BLE 冷恢复完成。
旧实现 command ledger 已结束，但 route 的 commandCorrelation/commandFrame 只在响应或 context 退役
时清除；因此超时后新命令仍被 busy 拒绝。先写真实 route bootstrap + command ledger 测试，旧实现
在“应产生第 5 次发送”断言 **4 != 5** 明确 RED，再修正任务所有权。

#### 10.52.1 内部合同与清理

- 通用 `V2VirtualDeviceChannel.beginCommand(frame)` 返回单次命令的 `send/close` reservation；不包含 BLE、
  MQTT 或 SDK 类型。dispatch 在建 ledger 前获取容量，拿不到返回 busy，不为未准入请求制造 inflight/unknown。
- `command.ts` 统一终态出口释放 reservation：ACK/Error、超时、取消、发送失败以及建 ledger 失败/竞争。
  close 同步、幂等、不抛异常。timer/publish callback 校验 exact PendingCommand 对象，不只校验16位 sequence。
- Gateway adapter 独占一个活动命令及其 exact frame/correlation；每次重试仍发同一 frame/correlation。
  释放时清 frame；最多保留一条已发布终态的 correlation/sequence/late-ACK 标志，**没有第二份终态 frame 队列**。
  原先靠收到响应手动清除两个字段的分支已移除，不保留新旧两套 send API。
- 每次异步 authority lookup 返回后重新核对 exact route context/command。已取消、超时或替换的任务不得
  再发布，也不能借迟到结果退役新 context；close 不撤回已发布消息，不证明设备有没有执行。
- route 在进入通用 BBP/2 lane 前校验 ACK/Error 的 correlation + relatedSequence。错误/迟到关联丢弃，
  不用同 sequence 误结算下一请求，不因此强退 Hub 管理连接。bootstrap 的 ManifestAccept ACK 保持独立。
- 同一协议 session 内最近一次 unknown 的匹配 ACK 可按原 ledger 合同补为 acknowledged；迟到 Error
  不把 unknown 改为“确定拒绝”。已拒绝命令的矛盾 ACK 不采纳。只有一条见证，不能承诺任意旧 ACK 都可恢复。
- exact Action requestId/idempotencyKey 的 unknown 重试仍返回原未知结果，**不自动重新下发**。新 requestId
  是新业务命令，不用于冒充上一 Action 的安全重试；更不能跨 BLE session 假定已有 replay cache 可用。

#### 10.52.2 软件与部署证据

- 新独立 `services/broker/test-edge-gateway-command-lifecycle.js`：真实 HTTP authority responder、隔离
  SQLite、真实 RouteAdmission→Hello→Manifest→ACK→State、command/route/lane；仅 Aedes 发布端为 fake，
  定时器由测试推进。未改生产重试期限或 HTTP timeout 来让测试通过。
- **9/9** 覆盖超时释放及 unknown exact retry、错 correlation/sequence、异步 lookup 超时、迟到发送失败
  与16位 sequence 复用、容量先于 ledger、拒绝与矛盾 ACK、发送失败、取消、close 幂等及旧 close 不清新槽。
- 本机 Node **22.22.3** 与新 Docker 镜像 Node20 均 **9/9**；既有 Broker Device V2/permit relay、telemetry
  capacity/ACL/slow-consumer、publish rate 测试通过。主脚本 Redis 因未设置 URL 先 SKIP，随后在 Docker
  隔离随机键前缀/DB15 下补跑实际 Redis 测试通过。协议包 **18/18**、Docker 全 workspace 构建通过。
- 仅更新本机 `broker-shard-1`，无清卷/手改数据库。镜像
  `sha256:7e88bfcd8df334faef35500d9a38b095acd16b5b65d5d5171b70e6ff5242b0a4`，容器 healthy。
  shard0/2、Gateway、APK、Hub/child 固件未更新；因此本批部署证据仅覆盖该测试分片。
- Docker npm ci 报既有依赖审计 **3 moderate/5 high**，未运行自动 audit fix/升级；不把本批功能门禁计为安全发布。

#### 10.52.3 两板真实链路与当前恢复点

- 按本轮新授权 COM12 child `a` **一次**，resetAccess=1/error0，root NotFound、adv1/acl0；COM11 Hub
  DeviceKey/WiFi/access 保留。不是整片擦除/重新烧录；旧 access 已失效，不回滚旧秘密。
- 原生 Direct enrollment intent=`773648780a59bf33dd04b4834a28aa76`，epoch **494518246**，仍是
  `ble_8Zc1CXfpwQtN1NqFWR6BvQ` / instance `fb42653e067ea9e4ab3b627db5975846`；
  正常 Method2→commit→active、button **2→3**，共 **17,037 ms**。没有第二次 reset、换 intent 或 pending 恢复。
  重置后的90秒串口捕获没有 assertion/reboot；后续 uptime 连续，**仅未复现 §10.51，底层根因仍未关闭**。
- 旧 `9bf02e5a0a3ff936963b937782e408c2` 正常 Detached/version5。真实 App 服务 create/resume 接入
  同身份 child，不换固件、不再 enrollment，新 operation=`45b92507779a943cc939cdac4a25323f`。
  Admin 安装/receipt、独立 Gateway Method2/同步 **34,541 ms** 至 PendingGatewayProof/version3；
  后续 cloud-only resume **326 ms** 达 Active/version4，无新增 BLE 连接。
  PG childInstalledAt=`1788682563636`、gatewayProvenAt=`1788682568355`，epoch494518246。
- 真实 App account MQTT→已更新 Broker→Hub→child：button **3→4**、ACK14，ready **1076 ms**、总计
  **2656 ms**。不是 permit relay 命令，也没有做真机丢 ACK/冷恢复注入；此结果只证明热路由回归。
  App phase=ready、busy/workflow=false、links/sessions/permits/recoveries=0。
- 最后 child uptime3854602、adv0/acl1、连接/断开13/12、indicationErrors0；access260 B，SHA256=
  `e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`。仍有普通 route 持有 BLE ACL。
  Hub secure1/routeError0/managementError0；heapFree105412、heapMin53252、largest61428、loopStackMin2736、
  ipc0StackMin1104、nimbleStackMin2820 B。此次包含 Broker 重启/重连；不把历史最低堆值归因为已确认泄漏或 SDK 问题。
  仍为 §10.51 Hub 内部弱存储 probe、§10.49 child test-only GAP witness，不替代 D4b/长稳/SDK生产准入。
- 本批 App/设备无生产源码改动，没有新增暂存、提交或推送；仅服务端任务所有权修正及三端文档同步。

#### 10.52.4 下一步：先冻结冷恢复，再启用 idle

1. 保留当前绑定，不为普通“继续”重复 reset。Broker 初次 virtual channel bootstrap 与下一次 BLE Hello
   分开建失败测试：旧 session/stateFresh、迟到旧 ACK、MQTT/context 替换，明确何时重新宣布 reachable。
2. Bridge 的 pending/recent 在断连时清除；因此先测试“Action 已执行而 ACK 丢失”，明确保留 exact 有界见证
   还是显式 outcome_unknown。不能因为 Broker 槽已释放，就假设设备侧也可立即接收/安全重放下一帧。
3. 冷连接预算须与 Broker 3750 ms command 期限、现有1秒 ingress 容量限制一致设计；不单纯加长扫描或排队。
   新代际禁止跨 session 自动重放 Action；Property 与 Action 语义也不能用同一“再发一次”规则掩盖。
4. 上述软件边界通过后，才实现普通 route 的正常 idle release，并用同一绑定做 Direct→Gateway→Direct。
   L4 已有 Direct attach/pending 恢复 UI、N3/N4/N5、N2/D4b/长稳独立保留；本批不关闭这些门禁。

### 10.53 L2b-2 路由投影代际与冷 Hello（2026-09-06）

本批先关闭“同 Hub/MQTT/拓扑中 BLE 重连后的协议身份隔离”子项，不直接启用 idle。仅清 Broker claims
并放行第二个 Hello 不足以阻止旧 State/Action 混入新连接：旧 envelope 没有物理会话代际，而 Bridge
断连后会清 pending/recent、重置 child sequence。必须在进入通用 BBP/2 lane/child sender 前判定所属代际。

#### 10.53.1 冻结 generation profile

- 复用现有 `ChildRouteEnvelopeV1` 的16 B correlation：前8 B为 **非零 uint64 大端投影代际**，后8 B为
  单次交换 nonce。不增加 CBOR 字段或帧长度，不增加 frame 队列；generation 不是认证材料、MAC 或设备身份。
- Bridge 自建 synthetic Hello 时前移代际：首次为1，BLE重新认证、MQTT/admission重新投影都前移。
  allocator 在 bridge 的整个对象生命周期保持单调，`reset()` 不清；达到 uint64 上限停止准入，不回绕。
  析构/重建 bridge 必须同时关闭所属 MQTT connection。Hub 重启自然产生新 Broker connection/context，
  generation 无须写 Flash；不能让重建的 bridge 在原 MQTT context 中从1重新发送。
- 必须在 synthetic Hello 声明 **FeatureGatewayRouteGeneration = 1<<14**。这不是 BLE child 的能力，
  不要求普通 WiFi/BLE固件或App支持新bit。旧Broker会拒绝未知bit，新Broker拒绝缺该bit的网关Hello；
  **Hub/Broker须成对更新**，不能启发式识别旧随机correlation并静默降级。外层CBOR形状/golden不变，语义已修订。
- 每个下行生成/重试/回包保持同一个完整correlation；自主上报使用当前代际和新nonce。Bridge逐帧校验
  generation及原有MQTT generation、durable revision、topology/epoch/instance；旧代际只丢弃，不误断新连接。
- Broker收到更大代际的有效Hello后退役旧virtual channel/命令槽/关联见证及presence token，保留Active拓扑；
  为新投影生成独立protocolSessionId，不能只用同MQTT sessionId+topologyVersion重复命名。
  generic lane继续执行严格初始Hello；不在通用协议实现中增加“允许反复Hello”的BLE特例。
- 新投影不继承旧stateFresh/manifestReady；重新Manifest/State，允许设备重启后State revision从0开始。
  只有protocolReady且stateFresh才重新宣布reachable，不在仅收到Hello时宣称可控制。
  相同/旧generation Hello、旧State/ACK/命令均不进入新lane；冷Hello可使尚未结束的旧异步发布失效。
- 已接受但未收到ACK的Action明确保持 **outcome_unknown**。相同requestId不重发，旧代际重传不再交给
  child。这里只保证代际隔离，不声称持久exactly-once、跨session恢复旧ACK，或新requestId能恢复旧执行结果。
- 本批无App wire/UI/设备用户API改动；gateway bearer/security、拓扑授权、projection生命周期与通用BBP/2
  继续分层。仅Bridge新增一个8 B计数器（最终globals变化以构建为准），无新增持久记录/密文缓存。

#### 10.53.2 软件门禁

- Broker新冷Hello用例先 RED：`strict initial device HELLO is required before BBP/2 data`。
  设备Bridge旧实现接受旧代际Action，`StateConflict`断言 RED；修正后均GREEN。
- Broker lifecycle总 **14/14**（原9项+冷Hello/旧State/结果未知、旧查询和retire回调、异步Hello乱序、
  旧bootstrap发布未结束、新能力/零代际/错误flags）：本机Node22与实际部署Node20镜像均通过。
- Bridge测试覆盖已接受而未ACK时断连后exact旧Action不再send、会话保持Secure、generation大端golden、
  自主Patch/响应的前缀关联、MQTT/admission换代与原有100次pre-enqueue WouldBlock不耗sequence。
- 完整Host ASan **46/46**、**200,000**确定性fuzz、架构检查通过；既有Broker通信/permit relay/
  telemetry/ACL/限流通过。Redis主脚本先SKIP，另以Docker隔离随机键前缀/DB15补实际Redis测试通过。
- 网关内部probe增加 `d`：只调用正常childSession.disconnect()，不清access、不关MQTT、不重启设备；
  用于观察新的Method2/Hello/Manifest/State，不能将主动断连本身算作自动idle释放。

#### 10.53.3 部署与真机记录

本批保留 §10.52 同一 child/Hub/epoch494518246/Active拓扑。没有resetAccess、重新enrollment、清NVS、
更换Key或擦除子设备。以下是一次真实冷断连回归，不扩大为整个按需连接/自动让路门禁。

- Hub仍COM11/MAC `8c:bf:ea:1b:96:14`。烧录前核对实际分区表3072 B与构建一致，SHA256=
  `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`，仅写application@0x10000，
  esptool hash verified；没有写bootloader/分区/NVS/eFuse。MAC/分区读取及烧录有RTS/应用重启，不是恢复出厂。
- Core3.3.11/IDF5.5.5、XIAO_ESP32S3；最终artifact为
  `D:/codes/.build/esp32-edge-hub-l2-generation/Esp32EdgeHubDualBoard.ino.bin`，SHA256=
  `23abc126652dd5fe921cf156f79fde134f4fb970a3e82ee6d6c335f9ae58cb12`。
  Flash **1,434,328 B**、globals **65,364 B**，较§10.51/52为 **+248/+8 B**（包括内部d探针分支）。
  原§10.51/52 `esp32-edge-hub-l2-ingress` artifact保留，但回退时必须与旧Broker镜像配套。
- 仅更新本机`broker-shard-1`，镜像
  `sha256:eabc4d43d4db08a5453e6daef9b4cd65f766b9dacd56f688e61ed1876b6cb72d`，healthy；其余分片、
  Gateway、APK及COM12 child固件不变，数据库卷保留。镜像Node20.20.2的14项回归和Redis测试已补跑。
- 热路由：真实App account MQTT→Broker→Hub→child，button **4→5**、App ACK14，ready2509 ms，
  总计4401 ms，ledger acknowledged。不是用permit relay控制。
- 在无在途操作时，仅给Hub内部probe发送一次`d`：66 ms收到childDisconnect=0；73 ms Idle、333 ms
  Connecting、1397 ms Authenticating、**3516 ms Secure**。WiFi lifecycle5、managementSubscribed1保持，
  没有关闭MQTT/重启Hub或重置child。该3516 ms只是DirectSecure就绪，不是完整Manifest/State或命令预算。
- 重同步后真实App button **5→6**、App ACK14，ready997 ms、总计2823 ms。两次Broker ledger命令都为
  sequence5/acknowledged，但protocolSessionId后缀分别为 `6c328e376e2960676a6899037e1dbd72` 和
  `4f44fcaa8358509aa5d24bc8aff955e1`；MQTT session前缀相同。`v2_latest_state`已属于后一session、revision7。
  这证明同MQTT中的子设备投影换代，不是借MQTT重连清空旧状态后偶然成功。
- 最终PG仍`45b92507779a943cc939cdac4a25323f` Active/version4、epoch494518246；App phase=ready，
  busy/workflow=false，links/sessions/permits/recoveries=0。child uptime5750788、adv0/acl1、连接/断开17/16、
  indicationErrors0；root260 B、SHA256仍为
  `e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`。
  Hub最后heapFree105300、heapMin90220、largest61428、loopStackMin3688、ipc0StackMin1112、nimbleStackMin2780 B。
- 仍为内部弱存储Hub probe和child GAP witness，不代表D4b加密存储/正式发布；无新assertion/reboot证据
  不等于此前SDK assertion根因已修。没有新增暂存、提交或推送。当前保持绑定，普通route仍占child ACL。

#### 10.53.4 后续仍需先完成

1. 本批以新Hello作为可观察的连接代际变化；BLE断开到新Hello之间，Broker仍不能即时感知物理不可用。
   下一批明确idle/offline与按需唤醒合同，不能将旧stateFresh当作当前BLE仍占用的证据。
2. 新Hello/Manifest/State的丢包、QoS0单槽背压与冷连接预算仍未完成；已有3750 ms命令期限和1秒ingress
   不能覆盖15秒认证上限。先建立有界bootstrap恢复及过期测试，不给Action增加无界排队或自动跨代重放。
   同MQTT/同topology的Broker context丢失后，仅重发相同RouteAdmission不会让当前Bridge再次Hello；
   这也属于bootstrap恢复合同，不能将重复admission无条件解释为新session并打断在途Action。
3. 软件和真机冷恢复通过后才实现route正常idle release，完成同一绑定Direct→Gateway→Direct。
   L4已有Direct attach/pending UI、SDK assertion首因、N3/N4/N5、N2/D4b/长稳仍独立保留。

### 10.54 L2b-3a 首次协商单请求重试与过期（2026-09-06）

#### 10.54.1 边界与设计

本批将§10.53.4的恢复问题拆开执行：先收口 **Broker已接受Hello后，到首次State完整快照就绪** 的协商；
首个Hello本身丢失、admission丢失及同MQTT context重建属于下一批。本批不启用自动idle、不扩大命令期限。

- `edge-gateway-bootstrap.ts`仅负责协议请求、匹配、定时器及清理；不依赖Aedes、数据库、拓扑、BLE SDK。
  `edge-gateway-route.ts`负责封装route、实时authority复核及精确context代际；通用lane继续负责CBOR、分页、
  canonical manifest hash和状态。这是职责拆分，不引入通用任务队列或第二套命令runtime。
- 模块只接受ManifestRequest、ManifestAccept、StateRequest，每次保存 **≤64 B inner frame + 16 B correlation**；
  这是payload上界，不是含Node对象/Timer开销的总内存数。Command和超限请求直接拒绝。
- 当前请求最多4次发送（首次加3次重试），成功或失败的publish回调完成后分别等待1/2/4秒重试，
  第4次后再等4秒仍无匹配响应则退役。回调立即完成时单步约11秒；异步耗时也受独立总期限约束。
- 首个ManifestRequest创建时启动 **30,000 ms总期限**，覆盖本轮全部Manifest/State分页。
  页推进、重复包、StateConflict导致快照重新开始，均不能延长总期限。一次仅一个未完成publish/authority lookup；
  publish回调永久不返回也到期退役，迟到回调不能重新调度。不是“每页重新给30秒”。
  旧尝试回调未完成时即使收到其响应，也只保留下一请求并等待旧操作结束后发送，不能并发推进更多请求。
  此处退役是逻辑隔离，不是取消Aedes已经持有的底层publish；底层回调未返回时原awaiter/临时封装仍等待，
  不能宣称主动中止了socket发送。新代际不接受旧回调结果，传输本身的永久挂起治理仍属Broker连接生命周期。
- 每次重试重新校验Active topology/credential expiry，await后再次核对精确context和request对象；
  新Hello、撤销、过期、断开会关闭旧重试器，清除buffer/timer。没有低层错误触发无限重试。

#### 10.54.2 数据正确性与清理

1. 只把当前请求的对应kind/flags/sequence/**完整correlation**响应交给严格lane；重复或迟到Manifest/StatePage
   不再次推进cursor/hash。删除了旧ManifestAccept ACK仅靠sequence放行的特殊分支。
2. 同一请求重发保留完整wire bytes、sequence及correlation。QoS0下下行请求或上行响应被丢弃均可触发下一次重试；
   正在处理上一publish而丢弃的快速响应也由相同机制恢复，不追加无界入口队列。
3. 真正的StateConflict按既有lane规则从cursor0重取快照，但仍占用同一总预算；未匹配Error不扰动协商。
4. 完成首次State快照后移除重试器；普通State刷新保留既有路径，专门回归确保不会被新增重复页过滤误删。
   **后续运行期State刷新与在途Command单槽仲裁尚待审查**，不能宣称本批为所有刷新都提供重试。
5. 过期只退役虚拟child protocol session及其准备状态，不主动关闭Hub管理MQTT、不更改Access/Topology。
   未就绪时Command不进入该协商槽或创建新的执行ledger。已有Action继续原reservation/unknown规则。
6. 设备端Bridge、ChildSession、通用API与App源代码未改；本批没有新增MCU RAM/Flash、外部依赖或wire字段。

#### 10.54.3 验证与现场

- 先写失败测试：旧实现不重试，丢包/到期/挂起publish三项均RED，再实现。最终真实route+lane+HTTP authority+
  临时SQLite lifecycle **25/25**，其中本批新增11项，含多页Manifest/State、丢双向帧、wrong correlation、
  重复页、发送失败、旧异步lookup、新代际、撤权、总期限不续期、禁止Command与幂等清理。
- 完整Broker `test:device-v2`通过，包括Device V2、permit relay、telemetry容量/ACL/slow consumer和限流；
  可选Redis专用用例因未指定TEST_REDIS_URL而SKIP（本批未修改Redis实现，不引用历史结果作为本批证据）。
- 协议包 **18/18**；Node22.22.3本机及Node20.20.2隔离容器均 **25/25**。后者以§10.53镜像只读挂载
  本批dist与测试、network none执行，没有重建/替换运行中的Broker容器。目标文件diff检查通过。
- **未进行真机复测或部署**：Hub/child/APK及运行Broker仍保持§10.53版本；未重新认领、resetAccess、擦除存储，
  未更新数据库拓扑、未暂存/提交/推送。§10.53的同绑定button5→6是历史证据，不算本批真机结果。

#### 10.54.4 下一批顺序与未通过项

1. **L2b-3b：首个Hello与失去context后的恢复。** 当前Hub synthetic Hello仅发送一次；Broker context超时退役后，
   相同MQTT/topology下重发RouteAdmission仍不会使Hub重新Hello。本批超时已经有明确终态，但尚无自动重新准入闭环，
   因此先保留测试代码不部署。本项必须先冻结有界、幂等的重新投影请求/确认合同，再补Hub与Broker联动测试。
   重复admission不能无条件重建session；恢复只重建协议状态，不重置绑定、刷新secret或跨代重放未知Action。
   需要覆盖请求/确认/Hello分别丢失、延迟重复恢复请求、MQTT换代、在途Action及已完成新context不被旧请求打断。
2. **冷连接预算独立处理。** 本批30秒是Hello后首次协商上限，不包含15秒BLE认证，也不是3750 ms业务command的
   新超时。先ready再提交Action，不靠无界命令排队掩盖连接耗时；冻结App等待/服务失败语义后再做真机故障注入。
3. 软件及真机恢复闭环通过，才实施普通route idle release/按需唤醒和同绑定Direct→Gateway→Direct。
   期间继续保留只有两个测试设备的现有绑定，不因“下一步”重复reset。
4. L4既有Direct attach/pending UI、SDK assertion首因、N3/N4/N5、N2/D4b及长稳仍独立保留。
   本批不是上述发布门禁的替代证据，也不扩大到History/Bulk或新网关bearer。

### 10.55 L2b-3b Admission V2 / Hello原样重试与投影恢复（2026-09-06）

#### 10.55.1 冻结合同：拓扑准入与临时协议状态分开

不新增secret或长期凭据，不用“重新配网/认领”修复协议会话，不为每次重复投递生成新session。
本批废弃GatewayRouteAdmissionV1，唯一入口改为 **GatewayRouteAdmissionV2**：

```text
[2, 1, operationId, topologyVersion, childLogicalDeviceId, childDeviceInstanceId,
 accessEpoch, gatewayCredentialVersion, expiresAtUnixSeconds, retiredGeneration]
```

- 最后字段是8 B big-endian投影代际水位；允许全0表示初始准入，不能省略。JS以Uint8Array表达，避免Number
  丢失uint64精度；C++视图为uint64_t。固定数组从9项变10项，最大191→200 B；普通ChildRouteEnvelope仍8项，
  correlation仍16 B，bit14 generation profile和业务帧长度不变。
- 水位是Broker在当前MQTT claims身份下已经接受过的最高Hello代际，独立于可退役的child context；
  不存数据库/Flash，不从MAC或物理广播推导。重建context以该水位为下界，只有更大的有效Hello才能准入。
  若连这份MQTT级水位也丢失，必须关闭对应MQTT连接后恢复；进程重启天然建立新MQTT身份，不能保留socket却把水位归零。
- Hub先核对durable access、operation/topology/epoch/credential/expiry及MQTT generation，再处理水位：
  当前代际等于水位时清除旧投影映射，下一次poll只创建一次新Hello；已大于水位时保持当前投影；
  小于水位说明Broker声称见过本Bridge尚未分配的代际，拒绝而不跳号。初次admission仍先认证child。
- 新Hello即恢复确认。迟到旧Hello/ACK/State不能复活旧投影；旧admission重复到达不回退或再次推进代际。
  新MQTT claims不继承旧水位，也不复用旧claims对象的context；Bridge对象自身代际仍不得在同MQTT内归零。
- 在途Action即使设备已执行但ACK丢失，结果仍可能unknown；恢复只丢弃旧映射，不撤销已发生的执行，
  **不把Action搬到新投影重发**。通用command ledger/replay规则保持不变。
- V1编码/解码及TS类型已替换，没有兼容别名/静默缺省。Hub/Broker必须配套部署；纯WiFi、纯BLE、App
  account/direct协议与Sketch API不变，App不解析该内部admission。

#### 10.55.2 有界重试与职责

1. Hub复用已有128 B Hello workspace和待请求correlation workspace，保留一份相同Hello；增加计时/长度/次数状态，
   没有新帧队列。以monotonic计时，在0/1/3/7秒原样发送，11秒截止；大步推进时间也不会补发过期包，uint32绕回有效。
   本地publish失败不触发BLE重连风暴、不递增代际。合法同代际下行确认Broker已收到Hello后停止重试；
   Bootstrap先后顺序由Broker lane负责，Bridge不复制第二套Manifest状态机。
2. Hello耗尽保留有界帧且停止自动发送，不主动清Access或断开BLE。若Broker没收到任何Hello，后续有效admission
   可重启这份已耗尽Hello的有限发送窗口，仍使用同代际/同bytes；活跃窗口不会被重复admission续期，已确认的投影也不重发。
3. Broker admission在publish回调立即完成时名义发送时刻为0/2/5/10秒，全阶段30秒上限（包括15秒child认证等待）；
   已验证的新Hello关闭admission重试器。后续Manifest/State继续§10.54的4次/全分页30秒期限。
   每次定时重发重新检查Active authority，过期/撤权/旧callback都不能继续发包。
4. admission或bootstrap超时，退役context，等待1秒后最多自动重新准入一次；再失败保持离线。
   真正ready后才重置本次故障预算。安全撤销、authority失效、连接替换不触发自动恢复；
   其他明确context退休可以由现有已授权 `admitEdgeGatewayRoute` 服务入口恢复，不监听任意数据来重新开放撤销路由。
5. 新 `bounded-exchange.ts`只负责单请求所有权、串行发送、计时和清理，不依赖MQTT、BLE、CBOR、数据库或secret。
   原bootstrap模块保留协议种类/大小/响应匹配；route adapter保留拓扑准入及代际。旧重复定时器实现已删除。
   新admission lookup带可取消的attempt fence，context为空时收到撤销也能阻止迟到lookup重新准入。
6. 这些阶段上限不是用户命令的新timeout；一次自动恢复仍可能经历较长离线窗口，不能称为低延迟完成。
   和§10.54一样，超时隔离不代表能够取消Aedes已持有的底层publish；旧await可能等到传输回调返回，结果不会污染新context。

#### 10.55.3 软件证据与编译记录

- Host先加入Hello丢失断言，在旧Bridge上RED；最终ASan **46/46**，20万确定性fuzz通过。
  fuzz新增Admission V2有效seed/变异及canonical往返检查；20万是全部类别总数，不是全部都为admission。
- Bridge覆盖Hello原样发送/耗尽/同admission有限恢复、发送失败、uint32时间绕回、同MQTT水位恢复、
  pending Action旧frame拒绝、重复admission幂等、合法下行停止重试及不可能的未来水位拒绝。
  Child为Host模型，表示传输接受/序列隔离，不冒充真实设备执行次数或真机丢ACK。
- Broker真实route/lane+HTTP authority+临时SQLite lifecycle **30/30**（Node22.22.3及隔离Node20.20.2），
  新增同MQTT context丢失/unknown Action、admission/Hello丢失、一次自动恢复耗尽、无context时撤销lookup及新MQTT身份门禁。
  完整Broker套件通过；Redis专用因无TEST_REDIS_URL而SKIP。
- 协议包 **18/18**、C++↔Server Edge Gateway golden、架构检查通过；包括大于2^63水位保持精度、V1拒绝、
  短水位/截断拒绝及200 B最大长度。本批没有修改App源代码/UI、持久凭据或服务数据库。
- ESP32 XIAO S3编译产物使用独立目录 `D:/codes/.build/esp32-edge-hub-l2-admission`，不覆盖§10.53历史固件。
  Arduino ESP32 3.3.11 / ESP-IDF 5.5.5编译通过：Flash **1,434,832 B**、全局RAM **65,372 B**，
  相比§10.53分别增加 **504 B / 8 B**；没有新增帧队列。这是编译静态占用，不代表运行期堆/任务栈门禁通过。
  `Esp32EdgeHubDualBoard.ino.bin` SHA-256：`689eb65e948911eb2bbd02d993182986a7f3684a4058de54c7f8639fe98a96f6`；
  partitions SHA-256：`1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`，与§10.53一致。新固件尚未烧录。
- 硬件门禁辅助脚本单测 **31/31**；仅验证测试工具行为，不替代真实故障注入。

**现场未变更：**本批不部署Broker、不烧录双板、不操作手机、不resetAccess/重新认领；运行版本仍§10.53。
未暂存、提交或推送。当前是软件门禁通过，不是四端配套部署或正式发布。

#### 10.55.4 下一批执行顺序

1. 保持现有两个测试设备的绑定；先核对板卡/ADB与运行镜像，再配套部署Admission V2 Hub/Broker，不能只换一端。
   子设备DirectSecure固件与App协议不必随该内部格式升级；先跑热控制和正常冷重连基线。
2. 补真实故障注入：首个admission丢失、首个Hello丢失、恢复确认丢失、同MQTT退役后恢复及在途Action结果未知。
   测量“触发→BLE认证→Manifest→State ready→控制完成”，而不是仅报告Secure时间；保留access/root摘要验证未重置绑定。
3. 基于测量收口L2b-4 ready/失败可见性、冷连接预算和App等待语义：3750 ms业务命令及1秒ingress不能吞入
   15秒认证/30秒协商，不给Action加无界队列，不靠换requestId掩盖unknown。
4. 软件与真实恢复门禁通过后再实现route正常idle/按需唤醒以及同绑定Direct→Gateway→Direct。
   运行期State刷新和Command的单槽仲裁、L4 UI、SDK assertion首因及N2/N3/N4/N5/D4b/长稳仍独立保留。

### 10.56 L2b-3c Admission V2配套部署与真实恢复门禁（2026-09-06）

本批执行§10.55.4前两步。只保留现有两个测试设备，不新建/重置设备，不改ownership、Access、拓扑或credential。
**L2b恢复子门禁通过，不等于L2全部完成，也不等于Gateway正式发布。**

#### 10.56.1 部署与模块边界

- Hub COM11 / MAC `8c:bf:ea:1b:96:14`，核对实际3072 B分区表SHA256与§10.55构建一致后，仅写application@0x10000，
  esptool hash verified。没有写bootloader、分区、NVS或eFuse；进入下载器/应用重启不是恢复出厂。
- Hub使用§10.55的 `esp32-edge-hub-l2-admission/Esp32EdgeHubDualBoard.ino.bin`，SHA256=
  `689eb65e948911eb2bbd02d993182986a7f3684a4058de54c7f8639fe98a96f6`，Flash1,434,832 B / globals65,372 B。
  本批未改生产C++，无需为测试重新构建一套有条件分支的固件；仍是内部弱存储probe，不是D4b生产加密板。
- 本机仅更新broker-shard-1，Docker inspect镜像ID为
  `sha256:07f528ccecce89cc2675c688b3c67b19f36227a9c68dd7f70a5e804ca29ccd00`，healthy。
  其余分片、Gateway、COM12 child固件及App APK不变，数据库卷保留；没有临时修改生产JS/TS。
- 新增 `tests/hardware/edge_gateway_route_recovery_gate.cjs`：通过容器内部Node Inspector调用既有runtime边界，
  只拦截exact Hub MQTT claims/route topic/child，不访问management secret，不注入SQL或更改Active authority。
  测试中丢弃真实消息、按生产入口退役临时context；恢复仍经真实HTTP鉴权、MQTTS、Hub和DirectSecure/GATT。
  这不是空口射频丢包，而是Broker收发边界的确定性丢包；它能测试真实设备反应，不冒充无线干扰实验。
- 钩子最多120秒、日志最多128条，只记录公开header/correlation/hash和测试计数；`close`恢复原函数、清理帧和定时器。
  Inspector仅容器loopback可达，没有发布9229端口；测试结束已显式关闭且验证ECONNREFUSED。
  工具/9项自测在tests，不进入Arduino ZIP或Broker镜像；复用现有App账户路径，不新增公开测试API。

#### 10.56.2 双板与真实Android证据

正常基线：真实App account MQTT→Broker→Hub→child，button **6→7**（App ready3067 ms/总计4942 ms）。
随后对Hub内部probe发送一次`d`，2 ms确认disconnect，21 ms Connecting、1520 ms Authenticating、**3621 ms Secure**。
Broker观测新Hello→State ready **4427 ms**；同MQTT内投影代际1→2，重新同步后App控制 **7→8**
（ready1004 ms/总计2735 ms）。后一次App测试在设备已重连后启动，不是完整冷连接时间。

下表耗时均在**同一Broker观察点**计算，从显式context退休到新投影ready，不含BLE重新认证；这些用例保持已有Secure BLE。

| 实际故障 | 关键观测 | 退休→ready |
|---|---|---|
| 丢弃恢复轮的首个admission | floor2；2006 ms后重发SHA256一致的原包，Hub只分配generation3 | 6516 ms |
| 丢弃首个恢复Hello | generation4；1007 ms后相同generation/correlation/bytes的Hello重新到达 | 5554 ms |
| 丢弃首个ManifestRequest（Hub看不到Hello已被接受的下行确认） | generation5；1005 ms后相同下行重发；重复Hello被拒绝，不创建第二投影 | 4414 ms |
| Action真实执行但ACK被丢弃，立即退休context | generation5→6；button8→9一次；旧ACK不能结算，新投影不重放Action | 3418 ms |
| 丢弃一份ManifestRequest的全部4次发送 | generation8在0/1/3/7秒窗口耗尽；约11秒后失败、再等1秒自动admit floor8；generation9恢复 | 16653 ms |

最后一项只手动触发初次context退休/准入，第二次admission由生产Broker自动触发。没有第二次人工recover、
重启MQTT、断开BLE或重置child。Hello四次耗尽时Hub曾报告routeError21（NotConnected），BLE保持Secure；
后续新admission恢复为0。这里验证一次自动恢复成功；**两轮都失败后持续离线**仍只有§10.55软件证据，不扩大真机结论。

Action用例的独立证据：

- 原Action真实下行3次，均为同sequence5/correlation/bytes；设备计数只从8→9。
- 丢弃真实匹配ACK后，ledger为 `outcome_unknown`、attempt_count3、`DEVICE_V2_COMMAND_DISCONNECTED`，
  App得到失败而非虚构成功；测试助手将异常脱敏为REDACTED，不将此当作App错误提示UI验收。
- 新session ready后重新投递真实旧ACK，Broker返回null；再向真实Hub注入同一旧Action envelope，计数仍为9。
  此后正常新Action **9→10**、ACK/状态正确，证明旧请求没有在子设备上再次累加。
- 自动恢复测试结束后真实App再控制 **10→11**（ready1078 ms/总计2727 ms）；最新State revision12。
- 前几项使用MQTT session `ud1h-vtV2jV2rjcJnqARmA`；自动恢复用例开始前已观察到新的
  `q8IrSM9pUIwMV9mf80Hlxw`。每个用例内均核对claims对象未变，**不声称整批始终是同一MQTT连接**，
  也不凭此次观察猜测中间连接更换原因。各用例从当时已ready的真实连接开始。

#### 10.56.3 最终现场及软件门禁

- PG `45b92507779a943cc939cdac4a25323f` 仍Active（5）/version4/epoch494518246。
  child逻辑身份仍 `ble_8Zc1CXfpwQtN1NqFWR6BvQ`，instance `fb42653e067ea9e4ab3b627db5975846`。
- child root260 B，前后SHA256均为
  `e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`；未重新认领/清Access。
  最后witness uptime10897754、hostStarts7、acl1、connects/disconnects20/19、indicationErrors0。
  摘要相同证明内容未变，不是物理flash写次数证明。
- Hub最后heapFree105352、heapMin55728、largest55284、loopStackMin3688、ipc0StackMin1104、nimbleStackMin2824 B；
  本批不是长期资源趋势门禁，不用瞬时heap恢复或未看到assertion来关闭历史SDK风险。
- App仍phase=ready、busy/workflow=false，links/sessions/recoveries/permits均0；Inspector关闭、所有故障钩子恢复。
- 新镜像自身Broker lifecycle **30/30**；hardware工具共 **40/40**（新增9项、Windows Node22及隔离Linux Node20均通过），
  CI已纳入无硬件/无Service checkout依赖的工具自测；架构检查通过。
  新镜像的真实Redis限流测试也通过（独立随机测试key前缀、DB15），不再沿用上一批SKIP结论。
  Host46/46、20万fuzz及协议18/18是§10.55同一生产源码的既有证据，本批没有冒充重新执行。
- 未暂存、提交、推送。生产源码未新增测试分支；仅测试工具、CI清单和三端实施文档增加本批内容。

#### 10.56.4 下一批：先L2b-4，再idle

1. 先统一测量“触发→BLE认证→Hello→Manifest/State ready→App控制结果”；Host/Android/容器时钟可能偏移，
   不相减它们各自的Date.now来制造精确耗时。使用单一monotonic观察点或明确标注各段耗时/采样误差，
   补重复冷连接样本与失败样本；目前3621/4427 ms分别只属于BLE和Broker观察点。
2. 冻结并实施ready/失败可见性与App等待语义：先检查现有Presence/状态合同能否承载warming/recovering，
   不足时只补最小状态；建链/同步等待与命令提交解耦。3750 ms业务命令和1秒ingress不承担15秒认证/30秒协商，
   未ready不建立待重放Action队列，unknown不换requestId自动重发。
3. 结合实测复核重试节奏及运行期State刷新/Command单槽仲裁；本批正常路径已出现ManifestRequest/Accept重发，
   需要区分真实慢响应、过早重试和单槽调度影响，不能仅凭“最终成功”忽略延迟。
4. 再做route正常idle release、按需唤醒和同绑定Direct→Gateway→Direct。N2/N3/N4/N5、D4b、L4 UI、SDK assertion首因、
   双轮失败离线/复杂断网组合真机门禁及候选发布长稳仍独立保留；不扩展Mesh、多child或History/Bulk。

### 10.57 L2b-4a 冷连接测时与App缓存/可控制边界（2026-09-06）

本批执行§10.56.4前两项的最小子集。保留现有Hub COM11、child COM12、Android及Active/version4/epoch494518246；
只发送内部probe的普通断连`d`，不resetAccess、不改root、DeviceKey或topology。Hub/child固件及Broker镜像沿用§10.56。

#### 10.57.1 单一观察点与实际样本

新增独立 `tests/hardware/edge_gateway_cold_start_gate.mjs`：注入式测时编排，复用既有Broker故障工具与真实App账户客户端。
同一Host `performance.now()`记录触发、USB状态、新投影ready返回、App下发及回读返回。包括USB/CDP/Docker调度开销，
不是空口时间；Broker/App内部耗时各自单调计时，绝不跨时钟相减。Broker只读waitReady每50 ms检查、45秒独立上限；
必须同MQTT且新protocol session ready后才提交一次计数Action，失败/未知不自动重试Action。该上限只属于测试观察器。

以下四次使用修正前的同一App APK，MQTT session `cqBmxKuHBakZVs9gk6U4rQ`，各用例核对claims不变，generation12→15：

| 样本 | 触发→BLE Secure | 触发→Host观察到新Broker ready | 触发→App控制及回读结束 | button |
|---|---:|---:|---:|---|
| 正常1 | 3363 ms | 7969 ms | 9968 ms | 11→12 |
| 正常2 | 3368 ms | 7039 ms | 9065 ms | 12→13 |
| 正常3 | 3227 ms | 7789 ms | 9562 ms | 13→14 |
| 首个ManifestRequest丢失 | 4042 ms | 8117 ms | 10170 ms | 14→15 |

丢包在Broker边界注入，不是射频干扰；新一轮仍由真实固件处理。丢包轮1009 ms后下发完全相同的ManifestRequest，
重复Hello没有创建第二投影。这个样本证明可恢复失败，不代替两轮耗尽后持续离线的真机验收。
三次正常样本不能推导p95/上限；本批没有提高原命令3750 ms、ingress 1秒、认证15秒、bootstrap总30秒预算。

#### 10.57.2 已实施的最小App修正

实际恢复中Store持续出现 `cloudReachable=false, manifestAccepted=true, stateFresh=true`，约3.5～4.4秒。
缓存完整有利于离线展示，但原 `ensureReady()` 只检查缓存，错误地允许继续尝试新控制。先加回归测试，三个用例在旧代码失败，
再修正生产代码，而非通过修改测试或虚构warming状态掩盖问题：

- `isDeviceV2TargetReady()`由Session及UI适配层共用；已知不可达时UI不把缓存标为实时可控制，组件及值保留。
  `null`仅代表Presence未知/未协商，仍由现有服务器权威闸门判定，不把null当作物理在线证明，也不影响BLE Direct缓存。
- `ensureReady()`在入口与只读同步完成后检查目标可达性；新Command在await之后、请求ID分配之前再检查一次。
  本地类型化错误 `DEVICE_V2_TARGET_UNREACHABLE` 不新增BBP/2 wire错误码。
- 已发布请求不因后来的Presence变化取消、改口为“未发送”或换requestId。ACK/ledger/timeout继续决定结果。
- 离线目标的后台State补同步不触发整个账户MQTT重连。没有增加离线队列、持久状态机或重复重试器；
  Store的缓存事实、账户连接状态、目标可达性继续分层，不删除可供离线页面使用的Manifest/State。

#### 10.57.3 验证与部署边界

- Hardware工具 **49/49**（Windows Node22.22.3、隔离只读/无网络Linux Node20.20.2），架构检查通过；CI已纳入新测时自测。
- App协议/Session/Store/UI port/账户客户端/BLE Direct共 **66/66**，含本批8项回归；本批5个TS文件ESLint、TypeScript检查与local构建通过。
- 设备生产源码/Broker生产源码没有本批变更；不将历史Host46/46、fuzz、Broker30/30冒充本批重跑。
- 四次基线后PG仍Active/version4/epoch494518246；child root260 B SHA256仍
  `e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`，connects/disconnects24/23、indicationErrors0。
  Inspector关闭且ECONNREFUSED；App无Direct连接、无测试观察器。这里只是短样本，不证明资源长期无退化。

新App验证：local构建、Capacitor copy及Android `assembleDebug`通过，包名为 `iot.diandeng.tech.debug`，
使用 `adb install -r` 保留数据安装；本机显式 `BLINKER_LOCAL_CLEARTEXT=1`，生产HTTPS/WSS策略和Hub MQTTS不变。
APK SHA256=`e06240ec8c3a39cc6666943d0c352fad60a5b960b6d8b15afa627e1cd7ec2d3d`。
测试应启动 `iot.diandeng.tech.debug/iot.diandeng.tech.MainActivity`，不能误开并存的生产包；未清登录或设备凭据。

同两板追加一次新APK冷门禁：3494 ms Secure、8076 ms新Broker ready、9945 ms控制回读结束。
App实际收到不可达通知、缓存仍fresh/accepted时，生产 `ensureReady()` 明确拒绝，记录 `offlineReadiness.rejected=true`；
观察器只调用ready检查，没有尝试离线Action。重新ready后真实Action **15→16**、ACK17，状态正确。
本用例内MQTT `kw0HclVxLf-rIfh-f8YW7w` 不变；不声称与较早四个样本为同一个session。
新App的实际冷时间没有变快：本批只收口错误准入，不将它包装成延迟优化。
最终App phase=idle、available=true、busy/workflow=false，links/sessions/recoveries/permits均0，测试观察器已删；
Inspector再次验证ECONNREFUSED。child root摘要不变、connects/disconnects25/24、indicationErrors0。
最终Hub Secure、routeError=0、controlPending=0；heapFree105360、heapMin53168、largest55284 B，
loop/ipc0/NimBLE栈最低水位3688/1104/2796 B。只是本批结束观测，未关闭SDK风险或长稳门禁。
未暂存、提交、推送；没有新增生产测试端点、没有改设备public API或扩展wire状态。

#### 10.57.4 后续顺序（不能跳到idle）

1. **L2b-4b 延迟归因：**正常首个Manifest响应约1.2～1.7秒，因此1秒重试已经触发；部分ManifestAccept→匹配ACK约2秒。
   先分别测MTU/分片数、Write With Response/indication往返、重复请求与单槽调度，复核运行期State刷新和Command仲裁。
   不凭三个最终成功样本认定SDK/NimBLE无问题，也不直接调大超时。
2. **L2b-4c 可达/等待合同：**当前Presence只有bool+时间戳，没有warming/recovering；正常物理断连后直到新Hello触发退役前，
   Broker仍可能保留旧ready。本批App闸门只能处理“已经收到不可达”的部分，不能消除这段通知盲区。
   先定义模块化link-loss/投影退役通知和独立有界ready等待；可复用现有合同则不加wire状态，否则只补必要状态。
   建链与Action提交分开，等待失败不创建Action；已提交unknown不自动重放。补真实双轮耗尽/保持离线及App错误可见性门禁。
3. 完成上述后再L2c idle release/按需唤醒、同绑定Direct→Gateway→Direct；仍不扩散多child/Mesh/History/Bulk。
   N2/N3/N4/N5、D4b生产加密板、SDK assertion首因、复杂断网与发布长稳继续独立待完成。

### 10.58 L2b-4b1：短GATT往返、单目标同步与容量背压（2026-09-06）

#### 10.58.1 分层归因与冻结边界

§10.57 的问题不是一个“大超时”：需要分别处理应用重复协商、单槽容量竞争、GATT分片往返和恢复状态通知。
本批只改前三项中已有证据的最小机制，不新增协议状态、离线Action队列、MTU协商、TX ring或通用调度框架。

- **App Session**负责同一逻辑设备的一次同步；UI不感知GATT连接参数，不复制服务器的Manifest权威。
- **Hub RouteBridge**负责认证/代际检查、单个在途请求及sequence/correlation映射；容量不足不等于认证失效。
- **Central Port**负责无线链路参数；BBP/2、Gateway topology、业务API和其他MCU端口不变。
- **Broker**继续独立拥有bootstrap、State投影、ACL和command ledger；这次App刷新不是强制传感器重新采样。

#### 10.58.2 已实现与清理

1. `DeviceV2Session.refresh()`只将State标为stale，保留已确认Manifest、旧值和页面展示；ManifestConflict或
   NegotiationRequired才触发至多一次完整重协商。后者覆盖服务器accepted gate/授权版本失效，不能只处理fingerprint变化。
   `ensureReady/refresh`共用单目标同步入口，多个刷新及刷新期间的新Command共享同一Promise。
   先登记任务，再异步发出Store通知；新增同步订阅回调重入测试在修正前复现
   `StatePage does not continue the active transfer`，修正后通过。已知离线在失效缓存前拒绝；执行前/同步后仍复核可达性。
   删除原无条件Manifest失效路径和重复State transfer清理，不新增定时器/请求队列/页面API。
   StateRequest从Broker读取当前已同步投影；不能据此向用户承诺立即读取物理传感器。
2. `GatewayChildRouteBridge`在已有请求等待child响应、又收到另一条有效请求时返回WouldBlock，保留当前请求和认证链路，
   不再走StateConflict→disconnect。复用RouteClient既有单个exact ingress缓冲及1000 ms容量等待上限；不延长3750 ms业务期限。
   原请求的exact重传仍复用child sequence，损坏/失效authority与跨代校验不放宽。
   100次竞争请求Host回归验证不额外发送、不丢原correlation、不重置连接，后续原响应仍正确路由。
   **这不是完整排队服务保证：**缓冲满时其他下行仍可能被拒绝，等待到期交回既有上游重试/ledger，不能靠加大队列掩盖。
3. `Esp32NimBleCentralPortConfig.connectionIntervalUnits=12`（1.25 ms单位，即15 ms）；建立连接时明确传入min=max，
   保持原scan参数、latency=0和supervision timeout。配置校验同时遵守区间和supervision约束。
   配置属于平台端口，不要求初学者在Sketch中传入参数；不是绑定某一外部NimBLE库。
   原先传空参数，实机协商为40 units/50 ms；上游默认值可查
   [Apache NimBLE ble_gap.c](https://github.com/apache/mynewt-nimble/blob/master/nimble/host/src/ble_gap.c)，
   但本项目实际间隔以SDK3.3.11真机`ble_gap_conn_find`读数为准，不从上游版本推断所有SDK行为。
   更短间隔会增加保持连接时的无线活动频率；本批没有测电池功耗，也没有解决持续占用连接问题。
4. 内部双板probe保留只读串口`g`，输出interval/latency/supervision/MTU；临时修改连接参数的`i`已删除。
   硬件观察器新增`frameBytes`（仅长度、不打印body/secret），便于区分分片成本与协议重试。
   MTU23下ATT payload20，4 B分片头后每片16 B；DirectSecure增加24 B，理论片数为`ceil((BBP2FrameBytes+24)/16)`，
   这是编码推算，不是空口抓包或实测吞吐率。

#### 10.58.3 A/B、构建与部署证据

同一已认证连接、同一个App/Hub/child、连续两次无副作用计数Action，只修改连接间隔：

| 实际间隔 | MTU | Action ACK | 含随后回读 | button计数 |
|---|---|---|---|---|
| 50 ms（40 units） | 23 | 1372 ms | 1584 ms | 16→17 |
| 15 ms（12 units） | 23 | 581 ms | 783 ms | 17→18 |

这是每组一次的热路径归因样本，不是p95、冷连接或长期性能承诺。没有改Write With Response、indication、
DirectSecure、BBP/2重试预算或RX缓冲；保留安全协议也能显著减少这次短包往返时间。

- Device Host：先由新竞争回归复现旧实现失败，再ASan46/46及200000次确定性fuzz通过；Windows不冒充UBSan/LSan。
- App：新增State-only/并发合并/两种gate失效/离线缓存/同步订阅重入共6项回归；本次8个相关套件**74/74**，
  ESLint、TypeScript与local build通过。套件选择与§10.57不同，不直接用总数差值代表新增用例数。
- hardware helper：49/49分别在Windows Node22及只读、无网络的隔离Linux Node20通过；架构检查通过。
- SDK3.3.11/IDF5.5.5的XIAO ESP32S3内部Hub固件Flash **1,435,352 B**，globals **65,372 B**。
  比§10.57内部probe增加520 B Flash，globals未增加；新增配置不等于所有运行时结构零增量，未重跑其他MCU资源探针。
- Hub最终应用SHA256：`90162eb29e30970ef8d4f7f0dd40f5d6bbac75a1c96a6c00b4f9e3346a53118e`。
  烧录前核验COM11基准MAC及实际分区摘要，只有0x10000应用分区写入；未改bootloader/partition/NVS/eFuse。
  新连接只读检查确认15 ms、latency0、supervision256、MTU23。仍为weak-storage内部probe，不是D4b生产加密板。
- child COM12固件未变。Service/Broker生产源码/镜像与数据库卷未改；沿用§10.56的镜像。
  测试无resetAccess、无重新认领、无凭据轮换。最终APK与收尾真机结果见下方追加记录。

最终APK（包含同步订阅重入修正）SHA256：
`b495f081e756b8ac603e873e471c2ef2b1038d8fb68a43bfa50df17ccd4f0684`。
包名`iot.diandeng.tech.debug`，已通过保留数据覆盖安装；不是手机上并存的生产包。

最终组合再完成三次**App账户Presence观测**的普通冷重连：复用`appColdAction`的前置/离线检查/控制/收尾，
串口只发送普通断连`d`（DTR/RTS=false），Host单调时钟记时；等App确实收到false→true后才进行控制。
没有启用Broker Inspector、修改Broker函数或注入报文，亦未建立手机Direct BLE连接。

| 样本 | USB观察Secure | Host观察App重新在线 | 控制及回读结束总计 | App控制段（含同步/回读） | button |
|---|---|---|---|---|---|
| 1 | 1061 ms | 2272 ms | 2955 ms | 664 ms | 21→22 |
| 2 | 1097 ms | 2472 ms | 3130 ms | 640 ms | 22→23 |
| 3 | 1104 ms | 2472 ms | 3186 ms | 694 ms | 23→24 |

三次真实离线`ensureReady`均拒绝，恢复后Action各执行一次。表中冷总计统一从本机发送断连触发开始，包含USB/CDP调度；
App内部观测ms不与Host相减。**App Presence读数不是独立Broker新session审计**，与§10.57 Inspector观测定义不同，
不据此计算严格跨批性能百分比，也不冒充丢包、同MQTT claims证明或真实State/Command竞争验收。

收尾：child root仍260 B，SHA256仍为
`e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`；
operation `45b92507779a943cc939cdac4a25323f`仍Active(5)/version4/epoch494518246。
child `hostStarts=7`、connects34/disconnects33、indicationErrors0；这是自启动累计值，不是34次本批压力测试。
App busy/workflow=false，links/sessions/recoveries/permits均0，观测器已释放；Broker loopback9229为ECONNREFUSED。
Hub自然保持单ACL，所以本批结束不等于idle release完成。未暂存/提交/推送。

#### 10.58.4 尚未通过与下一批执行顺序

1. **L2b-4b2先补验证，不继续扩散实现：**在最终Hub/APK组合补齐独立Broker投影冷恢复观测、首次ManifestRequest丢失和
   runtime State同步与Command竞争。区分App↔Broker缓存刷新和Broker↔child物理State同步，验证已有1秒入口缓冲到期时
   原请求仍完成、未接受指令不执行、unknown不重放。完整Inspector故障脚本本轮在中间APK前置阶段退出，最终APK未重跑该门禁；
   也不把中间APK运行结果拼接为最终组合验收。需要保留失败阶段证据，不能只报笼统成功/失败。
2. **L2b-4c失联/恢复合同：**物理断连至新Hello退役旧ready的盲区仍在。补最小authority/代际绑定的link-loss通知、
   独立有界ready等待和两轮耗尽保持离线的App错误可见性；优先复用已有合同。等待失败不分配Action，unknown不自动重放。
3. 再L2c idle release/按需唤醒和同绑定Direct→Gateway→Direct。短间隔只降低连接中的延迟，不释放ACL、也不代替互斥策略。
4. N2/N3/N4/N5、D4b专用加密板、SDK assertion首因、Wio及Gateway发布长稳继续独立待完成；不开始多child/Mesh/History/Bulk。

### 10.59 L2b-4b2：运行期State恢复与MQTT/BLE序号生命周期（2026-09-06）

#### 10.59.1 已确认问题与最小修正

本批先按§10.58.4补四端门禁，不先启用idle，也不重复reset已有效绑定的child。发现并修复两项独立问题：

1. **Broker运行期State只发一次且缺少精确exchange隔离。**原来只有首次Manifest/State使用`EdgeGatewayBootstrap`；
   ready后的revision gap发出StateRequest，却没有相同的重试。旧运行期StatePage还只有sequence检查，错误correlation也能进入lane；
   继续收到Patch会反复重启快照游标。现所有只读协商请求复用既有有界exchange，不再保留运行期的单发分支。
   每次仍只持有≤64 B请求+16 B correlation，最多4次原样发送（0/1/3/7秒，单请求11秒截止），整个分页操作30秒总期限。
   等待快照时重复Patch不替换请求/延长期限；StatePage必须匹配correlation+sequence，游标/字段/版本仍由通用lane校验。
   已提交Command的精确ACK/Error继续由独立ledger处理，不消费State exchange；新Command在not-ready时拒绝且不创建ledger。
   耗尽继续复用原有退役/至多一次重新准入，不增加重试队列或Action自动重放。
2. **MCU把Cloud admission与活着的child协议会话混为一个reset生命周期。**首次组合丢包样本出现两条Command均ACK，
   但后续物理快照仅增加1。Broker重启会令Hub重新登录，composition保留BLE/DirectSecure连接，却循环调用RouteBridge.reset；
   其中`nextChildSequence_=1`使新Action复用了该child会话旧sequence/相同body，可能命中设备可靠请求缓存并返回旧ACK。
   删除这一行重置：构造时初始化，只有child断连回调才重新置1；MQTT准入重置仍清旧correlation、pending和recent请求。
   序号分配归属于南向child协议会话，不归属于MQTT登录。未新增字段/队列，也没有靠主动断BLE来掩盖状态耦合。

以上第一项由新Broker回归复现旧实现错误接受correlation、重复Patch替换请求；第二项由Host连续100次cloud reset后
child sequence不前进的回归复现。均先RED后修正为GREEN。首次组合故障样本不能解释为“只是不够等待”：真实执行缺失已定位。
测试工具另外增加`waitPatch`（最多5秒），确认首份上报被丢弃后才发第二个有意Action，避免报告合并使预期revision gap不存在。
这是故障前置观察，不是自动重试Action；任何unknown/失败先检查真实计数与ledger，不盲目重跑。

#### 10.59.2 软件、部署与资源

- Device Host：ASan **46/46**及**200000**次确定性fuzz通过，架构检查通过；Windows不冒充UBSan/LSan。
- Broker lifecycle：**32/32**分别在Windows Node22.22.3和隔离Linux Node20.20.2通过；`test-device-v2.js`及协议**18/18**通过。
  本批未重跑Redis专用门禁，不把相关历史结果计入本批。
- 四个hardware helper套件：**50/50**，Windows和无网络/只读挂载的Linux均通过；没有新增生产测试接口。
- ESP32 SDK3.3.11/IDF5.5.5 Hub构建：Flash **1,435,344 B**、globals **65,372 B**，相对§10.58 **Flash减少8 B、globals不变**。
  应用SHA256：`2aab4740052308fe4e962a43c2f0458f8c6a0ff988d8e7c49c00fc04caa89a45`。
  COM11核对基准MAC和实际分区摘要后仅写0x10000应用；未改bootloader/partition/NVS/eFuse。仍为内部weak-storage probe，非D4b生产板。
- 本机shard1已部署镜像`sha256:ec949b0e6b5696f8601c6555d9795a4b79d0851bd8aafb081ceb0c9537767408`，healthy。
  Gateway、数据库卷与其他Broker分片未改。重启shard1用于MQTT生命周期复测，不是重建Service账户或拓扑。
- child COM12固件及App源码/UI/APK本批不变；沿用§10.58 debug APK
  `b495f081e756b8ac603e873e471c2ef2b1038d8fb68a43bfa50df17ccd4f0684`。
  新Hub只读GATT witness为15 ms、latency0、supervision256、MTU23；安全协议/ATT缓冲/公开Arduino API未变。

#### 10.59.3 最终组合四端真机门禁

均为两块XIAO ESP32S3+真实Android账户客户端+本机MQTTS Broker，App不使用设备MQTT凭据、不走手机Direct链路。
先在§10.58 Hub基线上完成三次独立Broker冷恢复（ready2464～2581 ms）和首次ManifestRequest丢失（3556 ms）；
该组仅是前置基线，不能替代新Hub部署后的验证。最终新Hub、新Broker、原APK结果如下：

| 场景 | USB观察Secure | Host观察新Broker ready | 控制及回读结束总计 | 实际button |
|---|---|---|---|---|
| 普通冷BLE恢复1 | 1173 ms | 2527 ms | 3223 ms | 36→37 |
| 普通冷BLE恢复2 | 1086 ms | 2513 ms | 3194 ms | 37→38 |
| 普通冷BLE恢复3 | 1270 ms | 2655 ms | 3389 ms | 38→39 |
| 首次ManifestRequest丢失 | 1172 ms | 3619 ms | 4333 ms | 39→40 |

每个样本均验证sameMqtt、新child protocol session，以及App实际收到离线时ensureReady拒绝；恢复后才创建一个Action。
总耗时统一从Host发送普通断连`d`算起，包含USB/CDP/Docker调度；Broker内部ms另算，不跨时钟相减。这不是p95或长稳。
Manifest丢失样本在Broker内部1501 ms丢弃，2505 ms原样重发；两份请求SHA256均为
`cbda896e5375deb850ab2898383c1c95cab56031d9902a4855985f4f85504a40`。
重复旧Hello被拒绝，不新增投影；丢包点位于Broker报文边界，不是射频/分片注入。

另外两项独立门禁：

- **Broker重启、BLE不重连：**新Hub基线Action32→33；重启shard1后首个新Action33→34，ledger为acknowledged。
  前后child witness均`connects=41/disconnects=40/hostStarts=7`，实际BLE没断。证实序号修正不依赖重新认证或清请求缓存。
- **运行期State与Command组合丢失：**最终Hub在相同MQTT/child protocol session下34→36，两个Action都只执行一次。
  Broker内部579 ms丢首Patch；第二次Patch使revision gap触发StateRequest，1914 ms丢该请求；2045 ms丢第二个Action首ACK。
  同一Action重发后ACK于2662 ms被接受，此时State仍未返回；2919 ms才原样重发StateRequest，3340 ms接受StatePage并ready。
  两份StateRequest SHA256均为`2f5b0a731d3af914557b2aea5ab8b248c505aad3c8b2565f24e681993fe51080`。
  此处证明ACK/State互不误消费、exact Command重试不重复执行；不声称覆盖所有持续上报/分页期间变化竞态。

收尾：operation `45b92507779a943cc939cdac4a25323f`仍Active(5)/version4/epoch494518246；child access仍260 B，
SHA256仍`e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`，未resetAccess/重新认领/主动轮换凭据。
child witness `hostStarts=7/connects=45/disconnects=44/indicationErrors=0`，heap244728、heapMin240688；是累计读数，不是45轮本批长稳。
Inspector已关闭（loopback9229 ECONNREFUSED），临时hooks/观察器释放；App available=true、busy/workflow=false，
links/sessions/recoveries/permits均0，无残留观察器；没有清用户账户/页面缓存。
未暂存/提交/推送。

#### 10.59.4 下一批固定顺序与退出条件

1. **先补L2b-4b剩余入口边界：**制造真实Hub单槽WouldBlock持续跨1000 ms，证明原在途请求仍完成、过期未接收指令不执行、
   Broker ledger与App错误一致，unknown不换requestId重放。本批丢StateRequest发生在Broker，未进入Hub，不能冒充入口到期验收。
   先只用测试专用端口/故障工具实现注入；不增加生产队列、延长命令总期限或全局关闭BLE安全。已有Host边界回归保留。
2. **L2b-4c：**收口物理link-loss/投影退役的最小通知及authority/generation隔离，App独立有界ready等待；
   建链与Action提交分开，等待失败不创建指令，两轮协商耗尽保持离线及真实App错误可见性必须验收。
3. 再启用L2c idle release/按需唤醒、同绑定Direct→Gateway→Direct；仍不以热路径和恢复样本代替连接互斥/释放设计。
4. N2/N3/N4/N5、D4b专用加密板、SDK assertion首因、Wio/Gateway发布长稳继续独立待完成；不扩散多child/Mesh/History/Bulk。

### 10.60 L2b-4b入口负测与L2b-4c双轮终态门禁（2026-09-06）

#### 10.60.1 修正测试前提，保持模块职责

§10.59要求的“真实入口到期”不应通过直接修改生产状态或扩大缓存来制造。需区分三个事实：

- `GatewayRouteClient`拥有收到但尚未接受的唯一入口槽，1000 ms到期只清该槽，不等于设备执行/ACK期限。
- RouteBridge/child协议负责已接受请求与响应关联；Broker在State不fresh时不接受**新提交**Action，但此前已发送的Action仍可能在途。
- Broker未收到ACK时只能报告unknown，不能从自己看不到的MCU入口状态推断“确定未执行”；测试观察的计数不能成为生产自动重放依据。

因此门禁按层拆开：已有Host验证在途请求+WouldBlock竞争，§10.59真机验证已提交ACK与State互不误消费；本批验证真实MCU接收槽
持续受压时的过期、原样重传和最终不误执行。没有强行让App在State闸门关闭后又创建新Action，也不声称覆盖所有自然射频拥塞排列。

内部`BasicEsp32EdgeHubProduct`增加默认`RouteClient=GatewayRouteClient`的静态注入参数，替换此前写死的类型使用；
正常产品仍用原实现。此依赖边界使composition不承担测试故障策略，不增加虚函数/运行时配置/全局测试开关/公开Arduino选项。
`IngressCapacityProbe.h`仅在tests：显式编译`BLINKER_TEST_INGRESS_CAPACITY=1`时用`q`武装，在接收器接受前对Command返回WouldBlock。
真实MQTT回调、订阅、缓存、单调时钟、到期清理均仍由GatewayRouteClient执行；非Command继续转交。不改SDK/安全层/闪存凭据。
首个Command起6秒后且槽已清空才释放；未收到Command则15秒解除。probe没有保存/输出帧body或secret。

#### 10.60.2 真实入口到期与不确定结果

条件：两块XIAO、真实Android账户客户端、本机shard1，当前绑定不变。专用probe Flash **1,435,936 B**、globals **65,404 B**；
相对§10.59正常Hub的+592 B/+32 B仅是测试仪器，不能计为正式库RAM增长。应用SHA256：
`45d771090b1220e4e6f4591ae41abc66c59d4d275f84a538b954ffc7f5f8a624`。

App仅提交一次button Action。Broker观察到同一sequence5/correlation/digest的四份下行，约445/692/1192/2194 ms，
SHA256均为`4105dc38afb80ca1adc00534b5b281ce0382762cd45f414644b1f1d2084b1936`。
设备接收器实际阻塞两次槽生命周期，witness为`heldMs=1000`及`999`，随后`released count=2`；
其计时起点是第一次receiver回调，可能比MQTT收包晚1 ms，生产deadline仍按收包时刻1000 ms判断。
probe释放后再读状态，button **40→40**；sameMqtt、同一child protocol session、ready均保持。没有晚到Action被误执行。

SQLite ledger为`outcome_unknown / attempt_count=4`；App收到BBP/2 Error code **8（Internal）**，没有ACK，也没有换requestId重试。
**unknown正确，但错误呈现还不够具体：**本批只验证核心客户端失败返回，不声称用户已经看到专门的“执行结果未知”交互；
L2b-4c需让App保留unknown语义且不诱导自动重发，不能把code8直接翻译为“确定未发送”。

#### 10.60.3 两轮协商耗尽后的真实终态

沿用§10.59正常Hub及Broker镜像，尚未切入口probe时先执行此项：只退役一次临时projection，再允许生产逻辑自行恢复。
测试工具丢弃所有ManifestRequest，不丢admission/Hello，不调用第二次recover，不触发重绑定/凭据轮换。

- generation `0b`的4次请求：约801/1805/3810/7814 ms；digest
  `602f15bb92d79861221684ae4bdb5dc129e904fb430fc15e96815ae30b071d65`。
- 生产超时后在12820 ms第二次admission；generation `0c`的4次请求：12930/13935/15939/19944 ms；digest
  `f3142ac6454fe231d718c9453a35ba69da134c67243bcff86ccc3bbe1169f828`。
- 45秒独立观察结束仍sameMqtt、ready=false、没有第三次admission、没有Command；App `ensureReady`明确拒绝。
  当前generation的重复Hello被拒绝，不延长重试预算。关闭故障hook不会偷偷重新准入；后续应用更新带来的新MQTT会话正常恢复。

这是“两轮失败后的核心离线终态”通过，不是手机UI恢复倒计时/手动重试入口、物理失联即时通知或无线链路全场景验收。
测试工具统一复用已有恢复观察器与App账户客户端，仅新增终态模式和入口模式，不增加生产服务接口。

#### 10.60.4 回归、收尾与下一步

- Host ASan **46/46**、**200000**次确定性fuzz通过；新增probe自身的双次到期、不误执行、非Command透传及释放回归。
- hardware helper **52/52**分别在Windows Node22与只读/无网络隔离Linux Node20通过；新增终态定向注入/不自动重发用例。
- 架构检查通过；Service/Broker生产代码与镜像、本批App源码/UI/APK、child固件均不变，不冒充重跑历史Broker/Redis/App套件。
- 烧录核验COM11 MAC及实际分区表有效3072 B摘要，仅0x10000应用更新；初次比较4096 B读取与3072 B产物被检查拒绝，
  按相同有效长度复核一致后才烧录，没有“忽略分区不一致”或写入分区表。
- 正常无故障Hub已恢复（仍为weak-storage内部开发板，不是D4b生产加密板）：Flash **1,435,336 B**、globals **65,372 B**，
  相对§10.59 -8 B Flash / RAM不变。SHA256 `6ea6b88cf1defb268a3907e8c8b0afdc7c6d9217eb5f706aa1a4088b86ef54a6`，应用写入hash校验通过。
  二进制无`ingress=armed`标记，串口`q`不再触发故障。只读GATT为15 ms/MTU23，未修改可靠GATT/加密层。
- 最终正常固件再次冷BLE断连后：USB Secure1135 ms、Host新Broker ready2451 ms、App控制/回读结束3219 ms；
  sameMqtt/new protocol session，实际离线ready拒绝，随后**40→41**。新Action ledger acknowledged，前一个负测Action仍outcome_unknown，
  没有通过覆盖旧ledger或重新执行unknown来恢复。这一个短样本不是长稳或p95。
- 收尾operation仍`45b92507779a943cc939cdac4a25323f` Active(5)/version4/epoch494518246；child root260 B摘要仍
  `e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`。child累计hostStarts7、connects49/disconnects48、indicationErrors0，
  heap244728/heapMin240688；不是49轮本批压力测试。App available=true、busy/workflow=false、links/sessions/recoveries/permits均0，观察器清理。
  Inspector loopback9229为ECONNREFUSED；未resetAccess/重认领，未刷写或擦除NVS/分区表/eFuse，未暂存/提交/推送。

下一批从**L2b-4c失联通知及独立ready等待**开始，不重复做已通过的入口/双轮基础门禁：

1. 通知的唯一职责是退役exact当前child projection，不改durable Active topology、不重置Access、不改写已提交Action结果。
   authority（Hub会话/tenant/child/epoch/version）与generation都必须匹配；旧离线通知不能撤销新ready。需覆盖丢失/重复/重连竞态。
   不借用Command Error、业务Patch或账户Presence冒充Hub控制通知；先核对最小路由控制合同与跨端golden，再配套实现。
2. App分离建链/同步等待与Action提交；一个目标共享有界wait、可取消、失败不创建Action，unknown不新建requestId重放。
   Account保持共享连接，页面缓存继续保留；先回归真实离线/恢复/终态，再评估明确的用户错误提示，不扩大业务命令deadline。
3. 完成上述后再L2c idle释放/按需唤醒与同绑定Direct↔Gateway；当前Hub仍占用单ACL，不提前开放该能力。
4. N2/N3/N4/N5、D4b专用加密板、SDK assertion首因、Wio/Gateway发布长稳继续独立待完成；不扩散Mesh/多child/History/Bulk。

### 10.61 L2b-4c1：独立路由关闭合同、代次退役与四端复测（2026-09-06）

#### 10.61.1 职责与最小合同

断链事实属于南向会话，云端可达事实属于Broker投影，Active绑定属于Service权威拓扑。三者不应互相冒充：

- `GatewayRouteClosureV1`只在现有exact Hub MQTTS `route/up`、`route/down`流转，QoS0、retain=false。
- canonical CBOR为`[1, 2, acknowledgment(0/1), topologyVersion, childLogicalDeviceId,
  childDeviceInstanceId(bstr16), accessEpoch, gatewayCredentialVersion, generation(bstr8 BE)]`。
  固定9元素、非零epoch/version/generation；不包含MAC、secret、业务值或操作结果。与8元素ChildRoute及10元素Admission严格区分。
- notice为0、receipt为1，receipt复述相同身份/代次；Hub只接受receipt，Broker只接受notice。
  示例`ble_child`/topology7/instance01..10/epoch3/credential4/generation9共**43 B**，两端独立literal golden一致：
  `890102000769626c655f6368696c64500102030405060708090a0b0c0d0e0f100304480000000000000009`。
- 不新增普通BBP/2 MessageKind、不把notice包装成Command Error/Patch/账户Presence，也不下发到child；
  App无需新codec，继续接收Broker已有的Presence=false/true。
- 合同解析在gateway codec，MQTT大小/方向在RouteClient与Broker入口，运行退役在RouteBridge/Broker context。
  `envelopeMatches`共享最小身份字段校验，不共享BLE实现、业务请求、credential修改或拓扑状态机。

#### 10.61.2 设备与Broker实现

设备断开回调仅标记待通知并清理旧child exchange；在poll中重建小通知到已有`output_`，不在SDK回调中联网。
与Hello互斥时复用Hello计时空间：0/1/3/7 s最多4次exact尝试，11 s绝对到期；publish失败也消耗尝试，重复断开回调不延长预算。
匹配receipt停止重传；新Secure/Hello直接推进generation并取代旧关闭交换，不等待receipt，不延长重连。
MQTT身份、admission、durable revision变更会清掉旧通知，旧receipt晚到只丢弃，不中断新BLE会话。

Broker重新查询Active权威，并在await后重新核对exact Hub MQTT claims/tenant/context、child identity、topology/epoch/credential及generation。
关闭可抢先退役尚在异步publish/authority lookup中的旧投影；pending Action按已有ledger语义收敛，不伪造“确定没执行”。
保留无handle/无ready的admitted占位和generation高水位，等新Hello恢复；重复notice只回相同receipt，不重复Presence、不重置恢复预算。
旧notice不能撤销新ready；新MQTT不能继承旧投影。关闭处理与普通lane分离，rate limiter仍按control维度计费，
不返回错误格式的BBP rate Error，不绕过设备/tenant/包大小预算。

清理方式是复用既有身份比较、投影退役、队列和ledger，删除“物理断链只能等下一次Hello才让旧ready失效”的隐式依赖；
没有另建消息队列、全局重连器、持久路由状态或平台特例。

#### 10.61.3 验证与实际部署

- Host ASan **46/46**、现有确定性协议fuzz **200000**次；新增closure编码/截断/错方向/零代次、四次到期、时钟回绕、
  重复断开不续期、receipt校验、新Secure立即接替、MQTT换代清理。fuzz数量不冒充全部新合同随机覆盖。
- Broker生命周期 **36/36**：Windows Node22、旧镜像隔离挂载候选代码、最终新镜像Linux Node20均通过；新增4组loss/duplicate/
  identity/generation/并发lookup/未完成bootstrap/revocation门禁。协议 **19/19**、publish rate基础测试、架构检查通过。
- 硬件工具 **53/53**，Windows/Linux无外网只读挂载通过；新增独立closure receipt定向丢包、旧notice/receipt回放，工具不进入产品镜像。
- ESP32 SDK3.3.11/IDF5.5.5目标：Flash **1,436,900 B**（较§10.60 **+1,564 B**），globals **65,372 B**（不变）。
  正常非故障Hub app SHA256 `9eea4b9c64f85b045e8eeb170dbd6638247abfafd4f9912dc91140849bb7ddca`。
  COM11 MAC `8c:bf:ea:1b:96:14`、分区有效3072 B摘要`1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`一致后，
  只写0x10000应用并由esptool验证hash；不刷child、不写分区表/NVS/eFuse。no-reset只读连接首次失败，正常进入bootloader后完成核验；非运行BLE故障。
- 仅本机`broker-shard-1`重建/替换，镜像SHA256 `9412468095dda9ad4f62489f39c26112493671eb5e881825808334cb5748f22b`。
  其他shard、Gateway、Controller及卷未替换。**旧Broker不认识9元素closure，因此升级Hub前必须配套升级Broker**；不能声称任意旧组合兼容。

真实双XIAO+Android账户客户端完成两轮：

| 场景 | 同一Broker时钟证据 | Host时钟Secure / 新ready / 控制回读结束 | 实际Action |
|---|---|---|---|
| 丢首个receipt、回放旧notice/receipt | generation1 notice465 ms，473 ms旧ready=false；重发1402 ms相同digest；新Hello1797 ms才出现 | 1401 / 2679 / 4074 ms | 41→42 |
| 无故障冷断连 | generation2 notice421 ms，443 ms旧ready=false；新Hello1580 ms；只发送一次notice | 1223 / 2677 / 3430 ms | 42→43 |

不同进程时钟的原点不同，不能跨列相减。首轮两份notice digest均为
`fed4bfcbd2e4a809260152f41fa411a590205ab7075a5171b8126219124ca2af`；两份receipt digest均为
`6d8a421c57c6546956747efcdf455e8276d15486ec5f3202777428067a0d4576`。
replayClosure在新ready后拒绝旧notice并向Hub注入旧receipt，后续Action/回读成功；未增加重认领或新Action重试。
两轮sameMqtt、新protocol session，真实App均观察到离线且ensureReady拒绝，然后恢复。不是p95或长稳。

收尾计数43，新两条Action ledger均acknowledged（each attempt2/seq5），§10.60旧unknown仍attempt4未被改写。
operation `45b92507779a943cc939cdac4a25323f`仍Active5/version4/epoch494518246；child root260 B摘要仍
`e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`。
child累计hostStarts7、connects54/disconnects53、indicationErrors0、heap244728/min240688；不是54轮本批长稳。
Hub收尾只读采样Secure=1、routeError=0、heapFree105364/min95292/largest63476 B；任务栈历史最小余量
loopTask3656/ipc0 1104/nimble_host2740 B。这是当前固件短门禁资源见证，不推断所有SDK负载下都已修复历史水位风险。
App available=true、busy/workflow=false、links/sessions/recoveries/permits/observer均无残留；Inspector9229关闭为ECONNREFUSED。
Hub仍是weak-storage内部测试板，不是D4b生产加密板。未暂存、提交、推送。

#### 10.61.4 边界与下一批：L2b-4c2

**通知不是无损网络保证**：所有4份notice均丢失且再无Hello时，单凭有限重传不能证明Broker最终获知失联。
新Hello/MQTT断开已有退役路径；全部通知丢失但Hub MQTT持续存活的最终收敛，需要L2c可达租约/保活合同补齐，
不能靠无限通知、无限重连或把Action timeout当作离线证明。当前保留明确发布风险，不把本子门禁等同整个L2b关闭。

下一批优先App，不再重复已经完成的closure基础实现：

1. 增加独立`waitUntilReady(logicalDeviceId, signal)`，账户/连接准备层一个目标共享一个**30 s绝对等待预算**，
   覆盖账户准备、Presence及Manifest/State同步，不把30 s塞进3750 ms业务Command预算；后加入调用者不延长原预算。
   现有`ensureReady`保持快速失败语义，command保持先校验后分配requestId，便于负测和程序调用。
2. AccountClient/Session管理共同等待、会话代次和只读同步；DeviceUiPort只负责选择传输、页面进入/离开取消。
   复用既有单目标synchronizing任务。每个调用者独立取消；最后调用者取消/超时/账户切换则释放监听器与计时器，
   晚到只读结果不得唤醒已取消页面或提交Action。不能停掉共享account连接或清除页面Manifest缓存。
3. 只读等待根据Presence/同步事件推进；明确权限拒绝直接失败。尚未ready时不缓存Action参数、不自动替用户发送，
   不把单设备离线扩大为全账户重连。先覆盖共享等待、离线→恢复、双轮耗尽、取消/换账户、晚响应、unknown不重放的软件矩阵。
4. 接入页面的“连接/同步”阶段后真机复测：等待失败应说明本次操作未提交；**已提交Command但未获ACK**则保留“未确认结果”，
   不把通用Error8无条件翻译为确定未发送。当前App代码/UI/APK未改，这部分本批未完成。
5. 然后L2c idle释放/按需唤醒/同绑定Direct↔Gateway，并同时冻结可达租约的loss-all收敛；不先扩展多child/Mesh/History/Bulk。
   N2/N3/N4/N5、D4b、SDK assertion首因、Wio/Gateway发布长稳仍独立待完成。

### 10.62 L2b-4c2：共享 ready 等待、页面取消与 State 同步互斥（2026-09-06）

本批完成 §10.61.4 的 App 等待实现、软件矩阵和真实正/负恢复门禁。设备/Broker **生产源码、固件与镜像没有变更**；
设备仓仅扩充内部硬件测试工具。没有重置 Access、重新认领、写分区/NVS/eFuse，也没有增加业务消息队列或扩大 Command 期限。

#### 10.62.1 模块职责与本批清理

| 层 | 本批职责 | 明确不做 |
|---|---|---|
| `DeviceV2ReadyWaits` | 每 logicalDeviceId 一个共享任务，最多16个目标；首次调用起30 s绝对预算，覆盖账户准备/Presence/只读同步；每调用者独立AbortSignal | 不持有Action参数、requestId或重放队列 |
| AccountClient | 注入当前代次的Session；stop/reset退役所有等待；不再等待挂死的旧HTTP才能退出账号；晚到旧登录不进入新账号 | 单页面取消不关闭共享MQTT；旧账号控制不能在新账号继续 |
| Session / Store | Presence订阅去重，保留现有单目标同步；离线→在线保留页面值但重新验证State；状态同步期间只保留通知最高revision | Presence=true不等于缓存State仍新鲜；主动同步与推送不争用分页槽 |
| DeviceUiPort / DeviceV2Page | 显式连接使用等待；离页/后台/销毁取消；连接失败仍显示缓存，但禁用控制；晚BLE失败不再进入Cloud fallback | 不自动重放Action；被动连接观察不新建长等待 |
| 页面兼容桥 | get-only允许连接等待，get+set直接按现有控制通道校验 | 不把混合读写中的Action排在30 s等待后 |

保留 `ensureReady` 的已知离线快速拒绝及现有只读协议超时，不以新wait替换所有调用点。
后加入者不续期；最后取消/超时/Session关闭/账户换代释放**等待器自己的**监听器和定时器。
已发出的共享只读协议任务仍按原有有界生命周期收尾，不因某个页面取消而关闭账户或清除Manifest。
Browser延迟触发timer时，晚结果也先比较绝对deadline。通过等待后仍由用户显式触发控制，不保存“等设备上线再执行”的操作。

Command在发布路径后遇到Internal、超时、网络丢失或无效ACK，统一为
`DEVICE_V2_COMMAND_OUTCOME_UNKNOWN`，页面提示“操作结果尚未确认，请先检查设备状态，勿重复操作”。
明确的CommandRejected/RateLimited等仍保持wire错误；提交前的离线/参数校验失败不冒充unknown。
不生成新requestId补发unknown；现有同frame/requestId传输重试和明确协商拒绝的处理不变。
**3750 ms是既有Broker Command执行预算，30 s只属于App连接准备；两者不合并。**

#### 10.62.2 真机暴露的并发问题及修正

初版等待软件测试通过，但真机三次均在控制前失败，counter始终43。不是NimBLE/射频/密钥故障。
脱敏调用顺序明确为：

`Presence(false→true) → 主动beginState → 推送beginState/applyStatePage → 主动applyStatePage失败`。

服务器在子设备新ready后推送完整State，与App的StateRequest响应合法并发。
旧Session让两条路径共用并覆盖Store分页槽，导致 `StatePage does not continue the active transfer`。
不能通过吞掉这个错误、增加重连时间或省略真实State读取“修复”。

修正为：主动同步持有唯一分页接收槽；期间State/Patch通知完成解码校验后只记录最高revision，不覆写该槽。
主动快照已覆盖该revision即完成；否则复用既有StateConflict的**最多两轮**补读，
持续变化则返回明确冲突并保持State不新鲜。无新增通知payload缓存或无限同步。
不在途时推送仍按原路径处理；Event/Telemetry不被丢入State等待。
已增加同revision State/Patch并发、较新通知补读和持续变化终止回归。现场临时调用跟踪已撤销。

#### 10.62.3 验证结果与版本

- Angular正式测试入口：**31套件224/224**，包含协议、Direct、安全/凭据、网关API/checkpoint、Account、等待与页面生命周期。
- 直接Vitest定向8套件 **106/106**；新增等待16项、Account隔离2项、页面取消/unknown/超时5项等。
  直接跑全部目录曾缺Angular TestBed初始化；已改用项目正式Angular入口执行完整矩阵，不以配置失败代替业务失败结论。
- TypeScript/Angular编译、定向ESLint、local构建、Capacitor copy及Android assembleDebug通过。
- 四个硬件工具套件 **54/54**（本机Windows）；本批没有重跑Linux镜像/Host ASan，因为未改设备/Broker生产实现，不冒称新增通过。
- 安装并验证的最终debug APK SHA256：
  `648358dbae2f6973e119be4b74aac8d7c3afaaf02204e633921bfd0d3e1d09c3`，
  包名 `iot.diandeng.tech.debug`，保留原登录与凭据。
- Hub沿用§10.61闭合通知固件SHA `9eea4b9c64f85b045e8eeb170dbd6638247abfafd4f9912dc91140849bb7ddca`；
  Broker shard-1沿用镜像SHA `9412468095dda9ad4f62489f39c26112493671eb5e881825808334cb5748f22b`。
  这次未烧录或重建Broker。

真实两块XIAO（Hub COM11/child COM12）+Android ADB +本机Broker/Gateway：

| 场景 | App真实等待 | 其他证据 | Action |
|---|---|---|---|
| 普通BLE断链恢复 | 一个调用者取消，另一个2619 ms ready | sameMQTT，新子设备session；Host新ready2789 ms、控制回读结束3560 ms | 43→44 |
| 两轮bootstrap全部丢ManifestRequest | 取消调用者不影响另一调用者；后者30006 ms timeout | 两代各4次exact请求；45 s无第三轮/无Command，ensureReady明确拒绝 | 保持44 |
| 负测后恢复 | 仅关闭exact Hub MQTT连接以走正常重连；只读等待5055 ms恢复 | 未重启MCU/重绑/改key，恢复读回revision45、counter44 | 无Action |
| 丢首个closure receipt并回放旧notice/receipt | 取消一方后另一方2521 ms ready | 2份exact notice；旧回放被拒绝，新ready保留；Host新ready2631 ms、控制回读结束3930 ms | 44→45 |

以上App和Host时间使用各自单调时钟，不跨时钟相减，不宣称p95或长稳。
等待是**实际离线时**由生产App客户端开始，不是Broker已ready后才调用ensureReady。
工具新增 `readyWait:true`，在任何控制之前检查共享结果；失败立即终止，不自动重新发送Action。
负测只在Broker边界丢包，不冒充射频丢包。

真实设备页也已进入：云端在线、canControl=true、无sync/error、buttonPresses=45；
离页动画完成后页面销毁、waits=0/connections=0，**共享账户仍ready**。
后台/销毁期间取消挂起等待和unknown提示以软件测试覆盖；本批不冒称完成真实手机杀进程全过程。
收尾Hub secure=1/routeError=0，heapFree105360/min50356/largest55284 B，
loopTask/ipc0/nimble历史最小余量3656/1104/2740 B。
heapMin是包含本次强制MQTTS重连的累计水位，不与上批仅短链路样本混为同负载比较。
Inspector9229已关闭（ECONNREFUSED），没有遗留临时Store跟踪。
最终App available=true、busy/workflow=false，links/sessions/recoveries/permits/waits/connections均0；
测试observer/trace均已移除，正常共享账户仍ready、child reachable/stateFresh=true、counter45。未暂存/提交/推送。

#### 10.62.4 当前结论与下一批准入

**L2b-4c2本子项通过，下一步可以进入L2c合同收口；整个网关发布门禁仍未关闭。**
此次没有启用idle release，因此Hub仍可能保持单ACL；`waitUntilReady`仅等待现有就绪证据，**并不负责唤醒休眠/已释放route**。

下一批顺序：

1. **L2c-0先冻结需求与唤醒/可达租约合同。**明确哪些读、页面兴趣或服务订阅会申请连接，
   区分“业务Action deadline”“连接需求有效期”“可达证明有效期”；MQTT仍连接、旧State缓存或单纯扫描到广播都不能无限延长ready。
   复用现有能力时必须证明能表达取消/代次/权限，不能直接让业务Command承担唤醒或排队语义。
2. **L2c-1实现正常idle释放及按需恢复。**保持Active拓扑与两种凭据槽，只退役volatile route并释放物理ACL；
   需求再到来才有界唤醒。App Direct与Gateway争用同一child的准入、退让和恢复必须一起验证。
3. **L2c-2补loss-all最终收敛。**所有closure通知和receipt丢失、且不再出现Hello，但Hub MQTT仍在时，
   Broker必须基于带代次的可达租约到期退役在线投影；不靠无限notice/重连，也不从Command超时推断确定离线。
4. 软件矩阵通过后，复用当前同一绑定做Direct→Gateway→Direct、取消唤醒/离页、租约过期与陈旧续租拒绝的真机门禁。
   不扩散多child、Mesh、History/Bulk。N2/N3/N4/N5、D4b、SDK assertion首因、Wio/Gateway长稳继续单独记录。

### 10.63 L2c-0a / L2c-2a：需求边界与 loss-all 可达租约（2026-09-06）

本批复核发现：Admission 当前仍会持续重连；列表 Presence 是会话级被动订阅，没有 cancel/lease；
Gateway bridge 也尚未开放 Telemetry 转发。不能把三者拼成“现有 demand”，否则 idle 会连回、列表会占用 BLE，
或者误把未实现的实时功能写成可用。已修正 `Esp32EdgeHubProduct` 对 demand-driven 的过度注释。

因此将 §10.62.4 的顺序调整为：**需求语义收口 → loss-all 独立安全底座 → 有 owner 的 demand → idle/Direct 交替**。
详细合同与后续实施入口统一到 `DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md`，不在多份文件重复维护数值。

本批实现：

- `GatewayRouteLeaseV1` 独立 challenge/proof，复用 closure identity 编解码辅助函数；严格区分 admission/closure/业务 envelope。
- Hub 仅在当前 exact secure/generation/access 下回 proof，复用 output workspace；无新增持久字段、任务或 child BBP 消息。
- Broker `EdgeGatewayReachability` 独立单调时钟租约：30 s；10 s 请求、未答每 5 s exact 重发；单 challenge、单 in-flight，无重连/Action 队列。
- proof 接收和异步 authority 返回后均校验 exact context/generation/nonce/deadline；到期只退役 child 投影，保留 Active、Hub MQTT 和代次高水位。
- 已发布 Action 保持 outcome_unknown；旧 proof/Hello 不复活；fresh Hello 可以重新协商。首次 ready 后普通业务消息不延长租期。
- synthetic Hub Hello 增 bit15；新 Broker 缺失能力时 fail-closed，必须配套部署。App 与普通 child wire/API 不变。
- 硬件故障工具修正十项数组分类，避免把 lease 当成 admission 或 BBP；不添加生产诊断接口。

软件验证：Host MSVC ASan 46/46、20 万次既有 deterministic fuzz；协议包 20/20（新增 C++/JS 相同 exact golden）；
Broker lifecycle + reachability 47/47、既有 `test-device-v2.js` 通过；硬件工具 4 套 55/55、架构与 TypeScript build 通过。
这里的 fuzz 是既有入口覆盖，不单独宣称新 lease 解码器完成了 fuzz 门禁。

ESP32 Arduino 3.3.11 / XIAO_ESP32S3 完整 Hub 编译通过：Flash **1,437,648 B**（较 §10.61 基线 +748 B），
globals **65,372 B**（不变）。输出 `.build/esp32-edge-hub-l2c-lease/Esp32EdgeHubDualBoard.ino.bin`，
SHA-256 `480e2fc3e581ddfa94fc01151ef3105f028521b5ead70cd663dbb394820df60f`。
此为 internal weak-storage 测试 Sketch，不是 D4b 生产加密板证据；未烧录或替换运行中 Broker。
globals 不变不代表任务栈峰值已经验收；新版本真实 heap/stack 水位仍随配套真机门禁采集。

尚未完成：新版本真机配套验证、owner demand wire/聚合、idle release、Direct 优先退让/无云退让。
本批只证明 Hub 持有安全路由的有限期声明，不证明 child 应用永不死锁，也不把“租期 30 s”写成物理断电后 30 s 的绝对上限。
未 reset Access、重认领或清除子设备；未修改 App 源码、未暂存/提交/推送。下批应先配套部署并验证正常续租/loss-all，
再完成 demand/idle/Direct 交替；不得把新租约当成已经实现按需连接。

### 10.64 L2c-2b：配套真机可达租约与真实发送路径修正（2026-09-06）

#### 10.64.1 真机发现的遗漏与修正边界

先部署 §10.63 候选后，Hub MQTT/management/route subscription 正常、child secure=1，
但租约到期后 App 报 `DEVICE_V2_TARGET_UNREACHABLE`，串口 `routeError=13`。
原因不是 NimBLE、SDK 或等待时间：`GatewayChildRouteBridge` 已编码 proof，
实际 `GatewayRouteClient::validateProfile()` 却只接受业务 envelope/closure；十项 lease 被误送进业务解码器，返回 CapacityExceeded。
此前 bridge 的 FakeRoute 测试没有覆盖这条实际发送过滤路径。

修正仅在真实 RouteClient 增加严格 lease 解码及 uplink acknowledgment=1 校验，拒绝反方向 challenge、截断和尾随数据。
`test_gateway_route_client_main.cpp` 补真实 RouteClient→CloudMux→MQTT exact bytes 回归，避免只测试局部 mock。
没有增加公开 API、设备字段、任务、队列、NVS 写入或 child 消息。

硬件工具复用已有 exact Hub/child 拦截与 App 只读状态入口，新增独立 `edge_gateway_lease_gate.mjs`；
单次安装最多120 s、最多保留一份 proof/显式回放两次、退出清理 hook/Inspector。
loss-all 只屏蔽目标路由上行及 closure receipt，保持真实 Hub MQTT；串口仅普通 `d` 断开 child。
`accepted` 仅表示生产 handler 已返回，不能把 lease handler 的 null 当失败或 BBP ACK；工具另用 `bbpAccepted` 描述 BBP 结果。
没有新增生产测试端点；显式 `recover` 是工程工具对 volatile admission 的恢复，不是已交付用户 wake API。

#### 10.64.2 四端实测结果

硬件为 COM11 Hub / COM12 child 两块 XIAO ESP32S3，Android `196f65f9` 使用既有 §10.62 APK。
四个租约门禁均不发送 Action，App计数保持45；全程相同 Hub MQTT 身份。

| 门禁 | 观测结果 |
|---|---|
| 正常续租 | 三个不同 challenge，proof往返约80/129/46 ms；同一 ready 投影保持，已消费 proof 回放不改变路由 |
| 丢首 challenge | 4,997 ms 后 exact 重发，随后三个 proof 周期通过，App仍可读45 |
| 丢首 proof | 5,016 ms 后收到相同 proof 重发，随后三周期通过；没有靠业务消息续租 |
| 全丢失 | proof、两次 closure、fresh generation Hello 均被实际拦截；Broker计时27,544 ms观测离线，即首个丢失proof后19,990 ms；App ready检查拒绝 |
| 离线保持与恢复 | 旧proof不能复活，额外5 s无自动admission；显式解除故障并恢复准入后1,484 ms观测ready；旧generation proof对新投影无作用，计数仍45 |
| 正常恢复后真实控制 | 另做一次普通断链，物理触发后1,169 ms secure、2,528 ms观测Broker ready；共享App ready等待2,399 ms，单独取消不影响另一调用者；Action只执行一次，45→46 |

上述耗时分别来自各 gate 自己的单调时钟，不跨 Host/Broker 时钟相减。
30 s 租期锚定 challenge 签发/首次ready；不能将27,544 ms描述成物理断电的固定检测上限。
这是受信任 Hub 对安全路由的证明，仍不是 child 应用层 ping 或“应用不会卡住”的证明。

#### 10.64.3 部署、资源和软件证据

- 仅替换本机 `aily-iot-dev-broker-shard-1-1`，镜像 `aily-iot-dev-broker-shard-1:l2c-lease`，
  实际 image ID `sha256:6da65337b3a9c18cd0508795eaacaa0253d55d95f226ba4ecfccff9296f3e6a3`。
  旧镜像保留为 `aily-iot-dev-broker-shard-1:before-l2c-lease`；其他服务/分片/数据卷未替换。
- Hub SDK Arduino ESP32 3.3.11 / IDF5.5.5，修正后 Flash **1,437,704 B**，globals **65,372 B**；
  较 §10.63 候选 +56 B Flash、静态RAM不变。最终应用 SHA256
  `ed9fca4fffd2a22af9abfa6cf0e1d215ead6d4aa3c3b0204343fe55299c11e62`。
  §10.63 原候选曾实际失败，已由此版本替换；同一 build 输出路径现指向修正版。
- 烧录前读回实际 partition，3072 B SHA256 `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`；
  两次均仅写 application@0x10000，工具校验通过。未改 bootloader/partition/NVS/eFuse，未烧录或重置 child。
  Sketch仍为 internal weak-storage 工程配置，**不是 D4b 生产加密板证据**。
- 收尾串口 child secure=1、routeError=0、无pending控制/管理事务；heapFree105,332 B、heapMin52,464 B、
  largest55,284 B、loopStackMin3,656 B、ipc0StackMin1,104 B、nimbleStackMin2,728 B。
  这些是本次运行水位，不据此宣称 SDK 栈首因或长时间内存稳定性已验收。
- 修正后 Host Debug MSVC ASan **46/46**、**200,000次既有 deterministic fuzz**、架构检查及完整ESP32编译通过；
  Broker lifecycle/reachability **47/47** 在实际候选 Linux Node20 镜像通过；硬件工具五套 **62/62** 在 Windows Node22 和 Linux Node20 通过。
  App源码/APK本批不变，未将此前Angular测试算作本批重跑。
- 收尾只读查库确认原 operation `45b92507779a943cc939cdac4a25323f` 仍 Active5/version4/epoch494518246；
  App cloud ready/fresh、计数46，临时observer/Direct links为0；Broker healthy、Inspector9229关闭。
  未重新读取 child root，不能冒称其本批完成了哈希比对；本批没有重认领或清除Access。

镜像构建的既有锁定依赖审计提示8项（含dev依赖：3 moderate、5 high），未做无关依赖升级；
正式发布前另做 production-only 可达性审计，不能直接把此计数等同线上漏洞数量。

#### 10.64.4 下一步与未完成项

L2c-2a软件与L2c-2b短周期真机门禁已通过；**L2c整体未完成**。
下一批实施 L2c-0b：App owner生命周期→Broker独立有界demand聚合/ACL代次→Hub volatile execution demand。
先补 acquire/cancel/release/wake 的 exact 合同和取消/过期/账号切换测试，再启用 idle；
Direct在线/无云退让与 management 独立有限期所有权须一同验收。
被动Presence、可达proof和Action均不产生连接需求，不引入Action等待/重放队列；
当前自动连回行为不能仅靠添加idle定时器修正。细则单点维护于连接需求合同第4节。
N2/N3/N4/N5、D4b、SDK assertion首因、Wio和Hub长稳仍独立待验，本批不扩大到Mesh/多child/History/Bulk。
没有暂存、提交或推送；既有三端用户变更保留。

### 10.65 L2c-0b1：所有者核心、有界异步工作与跨仓合同（2026-09-07）

本批没有直接启用idle。复核实际路径后确认：Admission仍会触发持续连接，App的waitUntilReady只观察ready，
页面消费者的独立需求尚不存在。先把需求所有权核心做成可测试模块，不能把“页面当前打开”直接塞入BLE/路由类。

已完成三个软件工作项：

1. **Broker `connection-demand.ts`：**与MQTT/BLE/DB解耦的有界owner聚合器；
   精确session+ownerId+revision+immutable target；acquire在异步ACL前注册，release抢先取消pending；
   单floor封住已取消owner，不累积墓碑。renew重验权限，旧身份/版本/超时结果不能复活。
   定时回收与Promise返回分离：即使鉴权不返回，本地等待也5 s结束；原工作槽仍保留至实际结束，防cancel churn。
2. **App `connection-demand.ts`：**每个精确session一个管理器，各消费者独立handle；
   30 s本地保守租期、10 s续期、5 s请求预算；abort/release/账号结束清理各自owner和计时器。
   accepted不代表ready。发送器固定旧session，不通过currentSession透明换号，也没有Action/自动acquire重试。
3. **真实模块合同联测：**`tests/integration/gateway_demand_contract.test.cjs`直接加载两端源文件，
   验证7类交叉取消/到期/丢失/账号代次行为，不以两个独立fake状态机冒充互操作。
   它不经过CBOR/MQTT/设备，证据级别明确为内部DTO联测。

进一步修正：只投影四个请求字段，不保留调用者可变对象/额外Action字段；非法target（含尾换行）拒绝。
通知适配器失败不阻塞取消或留下未结束的公开等待；上下游实际未结束工作均有容量上限。
Broker单目标多owner是消费者聚合，不等于此次实现了多child无线调度。

**接线仍未完成：**上述模块尚未注册到App MQTT dispatch、AccountClient/页面或Hub scheduler。
没有增加BBP编号/feature声明、HTTP端点、NVS字段或设备用户API，也没有部署新Broker/APK、烧录/重置任何设备。
当前运行版本与§10.64一致；不能把本批软件的owner回收写成“手机退出后物理BLE已释放”。
仍在使用的旧Admission连接编排暂不删除；应在唯一接线入口配套切换后删除，不同时维护两套永久模式。

执行路径已细分到连接需求合同第7节：**0b2 northbound exact wire/ACL/session生命周期 → 0b3 Hub execution demand → L2c-1 idle/Direct配套激活**。
其中“收包后TTL+generation”本身不能排除迟到的新acquire，必须先收口期限新鲜度锚并负测。
现有SNTP完成标志不是加密时间证明；不能为赶进度跳过该边界或通过固定长等待掩盖。
N2/N3/N4/N5、D4b、SDK首因、Wio/Hub长稳继续独立待验；不扩散History/Bulk/Mesh。
本批未暂存、提交或推送，三端既有用户更改保留。

软件验证：Broker TypeScript完整build通过，Windows Node22的owner/lifecycle/reachability **64/64**；
新owner **17/17** 在隔离Linux Node20容器通过，既有lifecycle/reachability **47/47** 也在本批Linux容器复测通过。
既有 `services/broker/test-device-v2.js` 在隔离Linux容器完整通过，临时测试数据库与运行数据分离。
App标准Angular CI四套 **52/52**（owner13、ready-waits16、account-client8、device-ui.port15），ESLint通过；
跨仓真实模块内部合同 **7/7**、设备库架构检查通过。Linux测试仅启动隔离的一次性容器，未替换运行中Broker或挂载真实数据卷。
本批没有设备C++运行逻辑变化，未重跑或冒用历史Host/MCU编译/真机门禁作本批结果。

### 10.66 L2c-0b2：北向协议、真实取消与身份清理接线（2026-09-07）

本批推进五项：

1. 冻结App/Broker独立ConnectionDemand `0x1c`、Status `0x1d`、feature bit16与四整数canonical CBOR；
   target来自Route，响应回显requestId/peer/owner/revision；不复用Presence、Action或设备登录凭据。
2. Broker dispatch在异步ACL之前进入owner核心；owner投影/fresh shared read ACL、当前claims与单调credential deadline；
   release不依赖活跃DB行，不受满pending replay表阻塞；旧accepted缓存不复活已结束owner。
3. 真正disconnect、session替换、clear、expiry与ACL invalidation接入同一清理路径；
   HTTP AbortSignal贯通且取消不触发全局Gateway冷却，不保留等待旧ACL结束才能取消的旁路。
4. App owner模块及测试迁入协议运行层，删除原上层位置；Session固定sender，Route取消/计时器清理与实际publish容量上限统一。
   未结束publish仍占额度；普通最多16、含预留release最多32；不增加Action队列、自动换账号重发或取消target索引积累。
5. 软件门禁：Broker新增16项（真实HTTP取消、重放、权限及身份），App新增13项；跨仓新增6项真实源码帧级联测，
   不再只以内部DTO互操作代替北向wire。既有owner/route/ready/UI边界测试继续回归。

**范围与剩余主线：**北向owner是记录需求，不是Hub唤醒成功。Broker聚合通知尚未接Hub executor，
页面/AccountClient自动申请尚未激活，Admission仍是实际连接入口。没有为了减少文件数提前删除在用编排，
也不复制一套新长期candidate。下一批0b3先确定接收时可验证的新鲜度锚（不能只用收包TTL+generation），
然后独立execution owner驱动Hub连接/释放，保留management有界需求，完成时再删除被替换的自动连接入口。
最后配套完成idle/Direct交替、取消/后台/失联/撤权，实际账户MQTT及四端验收。

**部署限制：**新bit16会被旧Broker strict HELLO拒绝，必须Broker先、App后；不单独升级手机。
bit16只宣告northbound ownership，不能被UI解释为物理wake/ready。child未增加BBP kind/feature，普通WiFi/BLE用户API不变。

验证结果：Windows Broker新增16/16+既有64/64；隔离Linux Node20四套80/80与`test-device-v2.js`完整通过；
Angular标准CI 11套122/122、ESLint、TypeScript、设备架构检查通过；跨仓内部7/7与真实帧级6/6。
测试只使用临时数据库/loopback HTTP和隔离容器，不使用真实账户凭据或修改真实数据卷；没有部署Broker/APK、烧录/reset设备。
无C++运行修改，未把旧Host/MCU结果当成本批重测；运行基线仍是§10.64，N2/N3/N4/N5、D4b、SDK首因和长稳不关闭。
未暂存/提交/推送；既有三端用户变更保留。精确wire、时序/容量及下一步见连接需求合同6.5、7节。

### 10.67 L2c-0b3a：南向接收方期限锚、紧凑租约与跨语言门禁（2026-09-07）

#### 10.67.1 方案决定与本批实现

本批完成四个软件工作项：

1. **冻结迟到包的新鲜度证明。** Hub发fresh challenge，以本地单调签发时刻为期限锚；Broker返回当前owner剩余时长。
   不用收包后TTL，不把SNTP trusted当作安全证明；普通WiFi/BLE产品API、child BBP/凭据/存储不变。
2. **双端exact wire。** route控制`[1,4,operation,...]`十一项CBOR，Probe/Challenge/Decision三种方向；
   16 B易失incarnation+不回绕uint32 challengeId，0～30000 ms，最大200 B、golden55 B。
   RouteClient真实CloudMux/MQTT发布只允许Challenge，不新增业务topic/队列或App codec。
3. **平台无关执行租约核心。** `runtime/GatewayExecutionLease.h`无BLE/MQTT/SDK/墙钟、无堆/任务/回调/NVS；
   一份40 B状态、一次pending、5 s challenge预算、新challenge最短间隔1 s。重复不续期，zero release抢先封住positive，
   旧release不取消新代，丢renew/release依原期限回收，millis正常回绕和随机失败保守处理。
   Broker复用现有owner模块新增纯剩余时长计算，不另建通用TTL/调度框架；authority检查仍属于适配器。
4. **双端与跨语言验证。** C++/JS exact golden、真实发送方向、deadline/loss/replay/release/clock/entropy；
   新跨仓脚本把真实Broker函数和Service编码输出交给C++ decoder/lease执行，防止只各测一份固定golden而漏掉合法零字段。

本批曾发现C++使用nonzero整数helper错误拒绝Probe、Challenge及zero Decision，已改为现有optional读取并依operation校验，
保留三种encode→decode回归及fuzz变体。另纠正一条无效负测（将本来为4的字节再次写成4）；不能把负测自己失败算成生产缺陷。
无旧实现被替代前不删除在用逻辑；去掉新增路径的冗余空指针检查，不复制candidate composition。

#### 10.67.2 软件门禁与边界

- Host C++11 strict warnings + ASan **47/47**，含新增execution核心及真实RouteClient；新格式纳入**200000次deterministic fuzz**。
  ASan首次运行环境缺少runtime DLL，报0xc0000135；加入已安装MSVC runtime目录后完整重跑通过，未关闭ASan规避。
- Service/Broker TypeScript build通过；shared protocol **21/21**，Broker owner18+northbound16+lifecycle42+reachability5，**81/81**。
  Windows Node22与隔离Linux Node20均通过；Windows `test-device-v2.js`总门禁另通过。
- 跨仓DTO7+App Session/Route/Delivery真实帧6+新增Service→C++5，**18/18**通过；架构检查通过。
  新测试不需要手机、账户或设备；Linux只读挂载本批源码/产物，network=none，不部署服务或挂载真实数据卷。
- ESP32-S3 Arduino **3.3.11**，`CompileEsp32EdgeHubInternal`编译通过；MCU ABI `static_assert`确认租约对象**40 B**，
  未额外实例化。该内部例程Flash **1,430,156 B**、静态RAM **65,340 B**；非实测heap/stack水位，非完整双板运行固件。
  可复现命令：CLI `compile --fqbn esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB --library D:/codes/blinker-library`
  指向`tests/arduino/CompileEsp32EdgeHubInternal`，本机用既有隔离`esp32-sdk-3311-ab.yaml`；无需上传设备。
- **40 B是执行核心对象，不是整机运行内存增量。** 本批未在生产composition实例化执行租约，不据此宣称MCU idle或四端唤醒通过。
  App源码/UI/APK未变，未把历史Angular/真机证据计为本批重测；未烧录/reset、清Access、提交、暂存或推送。

#### 10.67.3 下一批准入：0b3b实际接线，然后配套idle/Direct

完整合同集中于[连接需求合同6.6～7节](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)，本批明确不重复拆分另一套协议：

1. Broker把owner变化与当前Hub claims/Active target关联；异步authority返回后、实际发送前重读聚合与剩余期限。
   有界单工作/取消/重试，当前access/订阅代次退出即退休；不要为迟到Promise增加新队列或无限恢复。
2. Bridge核验exact admission/access/session，接Probe→Challenge→Decision与40 B核心；区分authority与volatile投影，
   admission重发不重新begin，旧包不启动连接。proof/revoke/renewal继续持有独立有界会话所有权。
3. 在唯一composition中替换Admission自动重连，配套实现普通idle与Direct主动/无云退让；持续page renew不能永久占ACL。
   当前自动连接逻辑仍在服务，提前删掉会断开管理/业务链路；完成替换后删除旧路径，不长期保留双模式。
4. 最后接页面/只读sync demand作用域、配套部署，验同绑定Direct→Gateway→Direct、取消wake、杀进程、撤权、乱序/丢包和management争用。
   **北向accepted、execution wanted、路由可达、State ready、Command结果仍是五件不同事实。**

本批不关闭完整L2c、N2/N3/N4/N5、D4b专用加密板、SDK assertion首因及Wio/Hub发布长稳。

### 10.68 L2c-0b3b1：实际控制接线与权限/投影生命周期解耦（2026-09-07）

本批推进四项：

1. **Hub收发接线。** Bridge实际消费Probe/Decision、初始化并更新40 B执行核心；复用route output workspace，
   加发送时刻/次数后静态RAM增48 B。初次Challenge无需child已连上；五次exact重发上限、重复Probe不重置预算，
   expiry/replay/旧release/订阅更替均校验。执行消息不调用start/connect/stop，不占child业务sequence。
2. **Broker控制接线。** 新`edge-gateway-execution.ts`只处理单Hub的有界控制工作，注入current/authority/remaining/send/clock，
   不依赖Aedes/DB/BLE/页面。正授权异步返回后重新计算source剩余期限；零回复不等待HTTP，且不能从未鉴权的零分支变成positive。
   最多一个实际未结束工作；通知只合并一个标记。HTTP取消贯通既有3 s网络预算，模块还有5 s预算；底层不结束不回收槽位。
3. **真实composition与生命周期。** App owner原空changed回调改为单一监听器，由Broker composition接到exact tenant/peer索引。
   route validate/dispatch实际识别Challenge，只向已合法发过Challenge的Hub发新控制，旧Hub不因owner变化收到未知包。
   执行绑定独立拥有identity字节；Hello换代、closure与可达租约到期不再随volatile投影删除它，完整authority/claims退出才关闭。
   未引入App→Gateway实现依赖、循环import、逐owner MCU槽或新的持久化记录。
4. **有针对性的回归。** 新增Broker模块9项、实际route/HTTP/投影6项、App owner通知1项；Host扩展真实Bridge接收、
   exact重发/耗尽、NVS准入代次、旧包与无物理调用检查。旧业务测试的fake route按控制/业务分别记账，仍保留原业务断言，
   不用删除失败断言适配新增控制流量。

**软件结果：**Host ASan47/47、200000次fuzz；Broker owner18+northbound17+lifecycle48+reachability5+execution9，**97/97**；
shared协议21/21，合计**118/118**在Windows Node22与隔离Linux Node20通过。跨仓18/18及Windows Broker总门禁通过，TS/架构通过。
Linux只读挂载本批源码/产物、network=none，未使用真实数据卷。新模块已接入Broker `test:device-v2`。

**资源：**ESP32-S3 Arduino3.3.11 `CompileEsp32EdgeHubInternal`通过，Flash **1,431,784 B**、静态RAM **65,388 B**；
相同例程/SDK相较§10.67增加**1,628 B Flash / 48 B静态RAM**。Host Bridge对象1600 B。没有新增任务/队列/NVS，
但不把编译结果说成无线并发heap/stack测量。普通非网关设备API和App源码/UI/APK本批未变。

**关键修订与下一步0b3b2：**控制接线不等于物理调度已切换。当前Admission仍同时承载authority送达与等待首Hello，
原自动连接仍在用。本批不能简单见首Challenge就关闭Admission重试，否则首Hello丢失会失去已验的恢复入口；
也不能让未来零需求idle因等不到Hello而失去有效authority。先拆这两个预算，再让proof/revoke/renewal报告独立有界会话所有权，
最后在唯一composition替换自动重连，配套idle/Direct与页面作用域。初次五个Challenge全部丢失后的确认/恢复也必须覆盖，
不能偷偷改成无限重发。详细合同与门禁集中于连接需求文档6.7～7节，不另建候选协议。

本批未启用物理wake/idle/Direct退让、未部署/烧录/reset或清Access、未暂存/提交/推送；运行基线仍为§10.64。
未关闭完整L2c、实际MQTT/四端验收、N2/N3/N4/N5、D4b、SDK首因与发布长稳。下一批无需重复codec/owner基础。

### 10.69 L2c-0b3b1：配套四端执行需求、取消与失联兜底（2026-09-07）

#### 10.69.1 本批顺序与实现边界

按本轮“尽快真机联调”调整为：先将§10.66～10.68已完成的软件配套部署、验证真实控制路径，
再进入0b3b2物理编排。没有跳过管理占用/首Hello恢复依赖，也没有为了演示按需连接而提前启用idle。
普通设备API、子设备固件、生产App UI和生产调度器本批未改。

- `Esp32EdgeHubDualBoard`仅增加内部`executionWanted`诊断及变化触发输出，复用已有周期资源行；不新增公开API、任务或存储。
- `edge_gateway_execution_gate.mjs`调用真实App `Session.acquireConnectionDemand()`，不伪造Broker owner，也不借设备MQTT凭据登录。
  复用原App/Inspector工具，单轮≤120 s，精确Hub/child，结束清理hooks、owner、observer及Inspector。
- 原route工具增加exact execution codec分类，避免把11项控制误当child BBP；zero丢包只影响execution Decision，
  不丢positive、Probe、可达证明或业务帧。测试工具独立于发布库/生产服务。
- 严格区分“先release再stop”与真正session退役；后者不预先release。
  部分释放必须看到剩余owner取得新的positive challenge，不能用历史positive数量凑通过。
- 一次中间runner在两个owner被接受后异常退出，原工具未保留失败快照，**不足以归因SDK/协议或宣称该次通过**。
  已增加清理前有界快照与白名单错误码，之后严格整轮、正常断链控制和force-stop/loss门禁分别通过。

#### 10.69.2 实际部署与权威基线

硬件COM11 Hub / COM12 child，Android `196f65f9`。仅更新Hub application@0x10000，未写bootloader、partition、NVS或eFuse；
烧录工具正常复位Hub。未烧录/复位/清Access/重认领child。

- Hub Arduino ESP32 **3.3.11 / IDF5.5.5**，`CONFIG_ESP_IPC_TASK_STACK_SIZE=2048`、SDK NimBLE；
  `build/execution-hardware-esp32-3311/Esp32EdgeHubDualBoard.ino.bin` SHA256
  `8983ebe42d6959a91ff8659d3b1f572bb768a0fce5152adfd64d7c2db9054e39`。
  Flash **1,440,236 B**、globals **65,420 B**，对§10.64同类DualBoard基线增加2,532/48 B；不是与CompileInternal混比。
- 烧录前实际partition读回3072 B，SHA256仍为`1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`；
  app0位于0x10000、容量3264 KiB，应用校验通过。仍为internal弱存储工程配置，**不算D4b加密生产板证据**。
- 只替换`aily-iot-dev-broker-shard-1-1`，镜像tag `aily-iot-dev-broker-shard-1:l2c-execution`，
  Docker实际image ID `sha256:64bc099ebfb415f25653c3490b91609c365ef712c397f6578df60971187fe399`；
  旧镜像保留为`before-l2c-execution`，其他服务/分片/数据卷不替换。
- App从当前源码构建local调试包，Capacitor copy显式`BLINKER_LOCAL_CLEARTEXT=1`匹配既有ADB反向本机HTTP/WS环境，
  不改变生产HTTPS默认。APK SHA256 `adf923838a0d8939f22dc8451ea40f2b5e9a970dc8eda6b968ba38e34af31979`，
  `adb install -r`保留账号/Direct凭据。最终恢复App到原Hub页；没有页面自动demand接线。
- 权威SQL澄清历史字段混称：operation `45b92507779a943cc939cdac4a25323f`是
  **Active(5)、topologyVersion=4、credentialVersion=185、accessEpoch=494518246**，更新时间仍为
  `2026-09-06 08:16:08.355 UTC`；历史“Active/version4”指拓扑版本，不能当作Gateway凭据版本4。
  child Access前后均260 B、SHA256 `e556d9280e8cad4c73a9b891c799051d96ba91398e6685e3fc3d9c4d60684318`。

#### 10.69.3 真机结果

| 门禁 | 实际证据 | 边界 |
|---|---|---|
| 两个App owner | 严格整轮两个accepted共69 ms；Broker owners=2，Hub wanted=1 | accepted不代表新建物理连接 |
| 部分释放/续租 | owners 2→1，剩余owner获得fresh positive，未误发zero；正常约10 s周期可见续租 | 不是页面后台/可见性集成 |
| 全部释放/取消 | owners 1→0、exact zero、Hub wanted=0；ACK前abort返回cancelled且不留owner | 未物理断开BLE |
| 会话退役 | 不预先release，真实App stop关闭账户session；owner返回session-ended、Broker清零、Hub wanted 1→0 | 不是仅删除工具数组 |
| force-stop + release全丢 | 真正ADB杀App；约1700 ms观测Broker owner=0；所有zero均被丢弃，Hub约30206 ms观测wanted=0 | 耗时含轮询/串口观测；不是30 s精确物理断连保证 |
| 业务互不干扰 | 需求整轮计数46不变；loss gate计数47不变；可达proof继续、Hub MQTT及当轮投影保持 | 证明控制消息未变成Action |
| 正常断链/真实Action | 普通`d`仅断BLE，1199 ms secure、2605 ms观测Broker ready；App ACK13、button **46→47**，总3458 ms | 延续旧eager自动恢复；不是按需wake |

Broker自身时间：严格整轮首Challenge→Decision约9 ms；Probe→收到Challenge约114 ms。
各段使用各自单调时钟，不与Hub/手机时钟相减。Hub实际串口分别确认wanted转换，不能仅用Broker发包当设备接收证据。
loss gate正租约停止后仍可见单challenge最多五次exact发送，但不能突破既有正租约期限。

收尾App读回counter47、fresh/可达true，临时Direct links/sessions为0，Broker healthy、Inspector9229关闭。
Hub最后heapFree105,308 B、heapMin82,704 B、largest63,476 B；loopStackMin3656、ipc0StackMin1104、nimbleStackMin2736 B。
这些是本轮短测试水位，不替代发布长稳或SDK首因结论。

#### 10.69.4 软件验证与下一步

- 候选实际Linux Node20 Broker镜像97/97；硬件工具六套66/66在Windows Node22与Linux Node20均通过。
- App需求模块/Session两套26/26，local构建、APK assemble与安装通过；Host已有ASan构建CTest47/47重跑通过、架构检查通过。
  本轮未重新跑20万fuzz，不把§10.68证据混成本轮新结果。构建保留既有Gradle兼容警告和npm依赖审计提示，不做无关升级。
- **0b3b2仍是下一项，不重复codec/owner基础。** 拆authority送达和“物理启动后的首Hello”恢复；覆盖初始五Challenge全丢、
  全Hello丢失的原有两轮有界恢复、零需求idle不丢authority，再接唯一composition的连接/保持/释放。
- 管理占用必须由协调器报告，不能直接复用`busy()`：Prepare返回NonceReady后busy可能false，
  但后续signed mutation仍依赖同一DirectSecure nonce/session。需显式持有到Prepare命令截止时间/Apply/取消（§10.70纠正：nonce本身没有TTL），
  proof退出与renewal不可逆恢复分别处理；缓存结果/已Active不得无限持有物理链路。
- idle/Direct有界退让、页面/只读sync自动作用域、真实撤权组合仍未完成；本轮force-stop只关闭需求控制子门禁。
  不提前删除仍在服务的eager路径，也不新增长期eager/demand双模式。N2/N3/N4/N5、D4b、SDK首因、Hub/Wio长稳仍独立待验。

### 10.70 L2c-0b3b2：管理会话所有权、单调期限与exact轮换清理（2026-09-07）

#### 10.70.1 复核后的执行路径

0b3b2有两个相互配套、但可独立验证的前置工作：管理事务对连接的需求，以及authority送达/首Hello恢复预算。
复核发现“nonce期限”并非现有wire事实，因此本批先纠正并实现管理子项，再接其余预算；不重复已经通过的0b3b1四端控制门禁。

`ControllerControlChallengeBody`不含TTL，`ControllerControlEndpoint`以transport/session结束清nonce，没有独立计时器。
必须由Hub对已鉴权Prepare的剩余有效期作有限占用，而不是新增child协议字段或借“收到过消息”无限保活。
唯一规范补入[连接需求合同6.8～7节](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。

#### 10.70.2 实际修改与删除

1. 共用8 B `GatewayManagementBudget`：首次接收锚定monotonic与剩余期限，上限沿用Service既有300 s，默认阶段120 s。
   持续Unix校验与本地期限同时成立才允许工作；相同command/结果重发不重置预算，SNTP回拨不得续期。
2. proof/revoke/renewal提供 `ownsChildSession()`。Prepare缓存NonceReady并发布后仍保留exact已鉴权attempt依赖；
   到期、会话断开/换代在poll及duplicate入口均转换Expired，清除nonce结果；无新会话复用旧nonce。
   mux把此查询加入跨家族互斥，不能仅依据busy=false抢占；同家族后续合法phase仍按原持久合同验证。
3. renewal删除无参数、只发一次disconnect即归还容量的旧清理方式。复用既有mutationAttemptId，由Bridge返回
   **该attempt已经结束**而非“请求已接受”；背压可重试、Disconnecting期间不重复发、后来attempt不能被旧结果清理。
   Rotate结果发布后仍阻止新管理阶段覆盖未结束清理；持久恢复不能断开已使用新凭据的无关会话。
4. budget外迟到receipt不推进存储；renewal不可逆阶段保留journal供ForwardRecoveryRequired恢复。
   revoke的Expired不代表child没执行，不能把该状态解释为安全重发mutation许可。
5. mux各状态查询复用唯一dispatch，删除重复选择分支；无新SDK依赖、线程、队列、heap或存储格式。
   仍在服务的admission自动连接没有删除；此时删除会破坏尚未迁移的Hello恢复，不属于无用旧代码。

#### 10.70.3 验证与边界

- Host严格构建及ASan CTest **47/47**通过；既有协议fuzz **200,000**轮通过，架构检查通过。
  新增预算边界/正常millis回绕/异常回退、proof重复与SNTP回拨、revoke七类nonce失败和迟到mutation receipt、
  renewal十条正常/过期/失会话/错kind/无鉴权/迟到receipt/清理背压路径，以及真实Bridge清理状态门禁。
  Prepare已发布仍互斥、Expired待发布仍不被覆盖、释放后能转交家族均覆盖。
- 初次Host回归发现一条旧proof测试构造了400 s阶段，超出现有Service上限；改为合同内300 s，保留独立超上限拒绝测试，
  没有放宽生产预算或删除旧proof场景。fuzz首次命令参数格式错误未执行；按`--iterations 200000`重跑通过。
- ESP32-S3 Arduino 3.3.11 `CompileEsp32EdgeHubInternal`最终编译通过：Flash **1,432,984 B**、静态RAM **65,420 B**；
  相比§10.68同例程增加1,200/32 B。末轮mux去重使Flash比本批中间构建再减少92 B。
  预算8 B×3、nonce attempt4 B×2，共静态增量32 B。不是普通WiFi/BLE产品增加32 B，也不混比DualBoard与CompileInternal例程。
- **未烧录、未reset、未操作Access/真实拓扑、未部署Broker/APK、未暂存/提交/推送。** 当前真机仍是§10.69版本组合。
  本批软件测试不替代真实撤销/轮换及idle/Direct争用门禁；独立D4b、N2/N3/N4/N5、SDK首因、Hub/Wio长稳继续保留。
- 用户随后将现场网络切到`mostfun2026`，仅同步被Git忽略的本机Hub WiFi更新配置，沿用此前提供的该网络密码；
  DeviceKey保持不变，公开示例无SSID/密码。不把文件更新宣称为设备已写入新WiFi或已恢复MQTT。

#### 10.70.4 下一批准入条件

1. authority送达预算与物理启动后首Hello恢复拆开：无需求不因没Hello撤authority；全Hello丢失仍保留有界两轮恢复。
   初始五Challenge全丢必须有有界重新确认入口，不使用无限Probe或依赖持续App流量。
2. 时间锚必须来自实际物理attempt；positive Decision只表示需求授权，不证明Hub已执行连接，owner续租也不是新的启动时刻。
   在真实Route/Bridge消费路径中整体替换，不增加未被使用的候选计时器或长期eager/demand双模式。
3. 随后唯一composition消费executionWanted和本批management所有权：连接、保持、取消尚未完成的wake、空闲释放。
   配套Direct有界退让与页面/只读sync作用域后，执行真实四端wake/idle/持续renew争用和撤权门禁。
   未通过前不宣称0b3b2或L2c全部完成，不广播按需唤醒feature。

### 10.71 L2c-0b3b2：准入送达、首Hello和失败投影恢复分离（2026-09-07）

#### 10.71.1 本批完成范围

1. **Broker送达与ready分开。** exact已鉴权Hub Challenge可确认初次authority Admission，不需要有owner或等child Hello。
   不改变positive执行需求的fresh ACL。零需求且无Hello超过原30 s窗口，不再误丢已确认准入。
2. **Hub自主首Hello两轮。** 实际DirectSecure及投影建立后，在0/1/3/7、12/13/15/19 s发送同一Hello，23 s停止。
   删除旧“重复Admission重启已耗尽Hello计时”分支。没有增加frame队列、定时任务或新wire；正常millis回绕与合法下行确认保留。
3. **首批Challenge丢失的有界恢复。** Admission生命周期内最多两batch，每batch五次；第二batch仅由有效Admission重发触发，
   使用原incarnation和fresh challenge/本地时刻；重复包不能重锚pending。两batch全丢后poll不无限重试，明确Probe/新生命周期仍可恢复。
4. **失败投影不等于撤authority。** Broker保留已确认身份与execution绑定，清理ready/状态/Command，原未知执行语义不变。
   Manifest/State恢复仍最多一次，非零retiredGeneration仍等待新Hello；旧Challenge不能冒充收到该floor。
   两轮协商全丢后离线，无第三轮background恢复；有真实新generation时才可重新完整协商。
5. **收口恢复竞态与重复实现。** 抽出唯一`deliverAdmission`，避免正常准入/恢复两份发送图；重复准入不取消退避恢复，
   真实新Hello已到达时取消多余重发；撤权或claims替换后迟到lookup仍被拒绝。

完整时序与三个预算边界见[连接需求合同6.9](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。
注意：23 s只从可发首Hello的实际安全会话起算，不包括扫描/认证。没有把Broker发positive视作Hub已开始连接。

#### 10.71.2 验证

- 真实Broker route模块新增五项组合回归：零owner/无Hello两分钟仍保authority、初次确认全丢两轮耗尽、
  bootstrap失败保execution且旧Challenge不能ACK恢复floor、退避期间新Hello竞态、保留authority恢复的撤权/迟到lookup。
  相关五套合计 **102/102**，Windows Node22和Linux Node20均通过；Linux使用现有镜像依赖及只读挂载本批dist/测试文件的临时容器，
  **没有替换运行中的Broker或修改数据卷**。首次Linux命令未挂载其余测试文件，未执行完整测试；补齐只读挂载后完整通过。
- Host严格构建/ASan **47/47**通过，Bridge完整新增/更新：两轮八次exact Hello、Admission重复不重锚、23 s后不再发、
  两batch确认上限、明确Probe恢复与zero Decision后不再因重复Admission自发重试。Host Bridge对象仍 **1600 B**。
- 跨仓实际App/Broker模块、wire、C++执行期限合同合计 **18/18**通过；既有protocol fuzz **200,000**轮及架构检查通过。
  没有新增wire，沿用既有golden。App UI/生产源码、Service HTTP/DB schema本批未修改。
- Arduino ESP32 **3.3.11** / XIAO ESP32S3 `CompileEsp32EdgeHubInternal`：Flash **1,433,032 B**、静态RAM **65,420 B**，
  对§10.70同例程分别 **+48 B / +0 B**；新增一个确认状态byte被原对象对齐吸收，无新堆/任务/NVS写入。
  此为编译证据，不与DualBoard真机镜像混比，也不代表动态heap/stack或物理切网已验证。
- 未烧录/reset、未修改Access或真实拓扑、未部署Broker/APK、未暂存/提交/推送。现场最后部署版本仍为§10.69，
  新网络`mostfun2026`本机配置沿用§10.70；本轮没有把该文件状态宣称为设备已联网。

#### 10.71.3 下一步及不可提前激活项

0b3b2两个前置软件子项（管理所有权、预算分离）已完成；**物理调度本身仍未完成，L2c没有整体关闭**。
当前App页面/只读sync未自动获取连接需求；仅把Bridge连接条件改为executionWanted会使正常页面无法访问，这是不能独立打开gate的直接原因。

下一批以一个配套目标实施：

1. 唯一composition消费executionWanted/管理所有权，取消wake、保持/释放连接与异步关闭资源占用明确分层；
   管理nonce/不可逆mutation不被idle打断，普通Action不由新调度器排队或重发。
2. 页面/只读sync申请与释放既有Session需求，覆盖进入/离开/后台/销毁、多个owner、取消和Session更换；不复制owner核心。
3. 同绑定Direct→Gateway→Direct及持续page renew下有界退让，软件门禁通过后配套升级Hub/Broker/App，再执行真实四端短门禁。
   新旧Hub/Broker的混合部署不能冒充本节全丢恢复保障；不新增长期eager/demand候选开关，也不广播尚未支持的feature。
4. 配套切换时再删除仍在用的eager连接入口，保持普通WiFi/BLE API及现有安全协议不变。N2/N3/N4/N5、D4b、SDK首因、Hub/Wio长稳不因此关闭。

### 10.72 App 页面需求作用域接入、配套部署与新网络恢复（2026-09-07）

**完成的是0b3b2的App消费者和配套环境恢复，不是物理idle/Direct交替完成。**
本批不新增owner核心、协议kind、调度开关、公开Sketch API或MCU任务；设备生产源码沿用§10.70/10.71。

#### 10.72.1 实施与清理

- 新增小型 `DeviceUiConnectionScope`，只绑定调用方生命周期与既有 Session `DemandOwner`；不维护第二套续租/replay/ACL。
  准备总预算30 s包含登录；成功后页面继续持有，one-shot只读则立即释放；后台/销毁/取消/Session retirement均关闭。
- `DeviceUiPort.openConnection` 用于页面；`connect` 保持只读 one-shot。仅Gateway child的Cloud路径申请需求；
  WiFi/Hub及Direct不额外发送需求。AccountClient登录代次和signal均检查，迟到登录不能复活页面。
- 页面不再用全局device disconnect取消其他owner；关闭自己的scope，最后一个页面scope才释放Direct连接。
  canControl还同步检查scope signal，避免closed Promise与ready Promise交错导致误启用控件。
- 移除V2 inventory查询的隐式connect，以及被动watchConnection的隐式ensureReady。列表/Presence不会唤醒BLE；
  显式只读同步有独立短作用域，Action不进入wake队列、不因新Session自动重发。
- 新硬件工具 `edge_gateway_page_demand_gate.mjs` 直接使用真实设备页/公共UI port：exact child检查、进入/退出、
  元数据状态、只读同步及单次Button。原WebView工具复用可选pagePath，不复制App/Broker owner。
  测试只返回状态/计数，无令牌、凭据或业务payload导出。

#### 10.72.2 软件门禁与部署边界

- App正式Angular测试入口：34 suites、266 tests通过；覆盖UI port、页面、账户、BLE、配网、协议及既有owner。
  一次直接全目录Vitest调用因没有Angular测试初始化而出现describe/JIT加载错误；改用既有 `ng test` 入口后全部通过，
  没有为迁就错误启动方式修改测试框架或绕过断言。
- Broker五组102项在Windows及最终Linux测试镜像均通过；跨仓真实App/Broker/MCU lease二进制边界21项通过（增加3项真实UI scope跨wire门禁）。
  本批相关硬件工具27项通过；这些数字不是所有仓库完整测试总数，也不代替无线真机。
- 本批核心App作用域/UI port/账户/页面文件ESLint通过；Git diff whitespace检查通过。
- Arduino ESP32-S3 3.3.11内部DualBoard编译：Flash1,441,456 B、globals65,452 B。
  对比§10.69同一DualBoard增加1,220/32 B，主要来自§10.70/10.71此前未部署的设备变更；本批没有新增MCU生产代码。
  Hub应用SHA256：`43eb02fd73833b3eb915f39b38516efc2c3ef1baa5f8edfd21096a6e8ace9b8f`。
- Broker shard-1测试镜像 `aily-iot-dev-broker-shard-1:l2c-page-scope`，镜像ID
  `sha256:2e17323553cdecfe60d6e8daa34d0aaaad9269b60a1f6c5a94eb4a538bfa3669`；
  在已有l2c-execution依赖镜像上仅替换本机编译dist。Compose使用被忽略的
  `build/broker-page-scope.override.yaml`，不覆盖远端/生产镜像；旧镜像保留用于回滚。
- 最终APK覆盖安装成功，SHA256：`95b7e7ce657cc47881e2a34a45eea19c7abf122706eebc4692efffbfa5fa2032`。
  页面先关闭、同设备read后完成的组合也会释放最后的Direct连接；不让两者的结束顺序留下孤立连接。
- 仍为**内部弱存储工程Hub**，不代表生产encrypted NVS/eFuse准入已通过；D4b继续独立阻断。

#### 10.72.3 本机网络与真机事实

- COM11先read-mac确认Hub `8c:bf:ea:1b:96:14`，读取0x8000分区仍匹配
  `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`；仅烧录0x10000应用区。
  未操作COM12子设备、未清除Access/NVS、未创建或轮换真实topology。
- mostfun2026下电脑当前IP为192.168.1.152。原本机登录地址与Gateway public host仍为192.168.2.76，
  测试证书SAN也未含新IP；修正被忽略的本机配置、Gateway环境和本机TLS代理叶证书。
  新叶证书由已有测试CA签发，继续验证CA和IP，HTTPS health实测200，不使用insecure/明文降级。
  CA不变；叶证书仅3天，测试CA本身到2026-09-11，不是发布信任链。
- ADB `tcp:7000→本机17001`、`tcp:8884→8884`恢复真实账户HTTP/MQTT，账号和12条库存设备恢复；APK覆盖安装保留数据。
  TLS代理旧容器停用并保留为 `blinker-device-v2-tls-proxy-before-20260907`，原证书/私钥仍保留可回滚。
  Gateway同一应用镜像只重建本机public-host环境；没有更改数据库结构或业务源码。
- Broker preflight：Hub MQTT/management/route与child均ready；Hub串口 `secure=1, routeError=0, executionWanted=0`，
  IP192.168.1.120。当前child计数器新基线0，不能继续套用§10.69历史47。
  资源观察：heapFree105,344、heapMin96,152、largest61,428、loopStackMin3,652、ipc0Min1,104、NimBLEMin2,816 B。
- 后台负测：手机系统keyguard showing=true，AppVisibility=false；普通UI read立即拒绝，scope=0、Broker owner=0。
  同MQTT/同child session在120 s观察窗口内保持ready，可达proof继续工作，Button计数仍为0。
  **这不是前台打开/退出后的租约释放门禁，也不是物理idle。** 已提示用户解锁并保留前台；不伪造AppVisibility=true。
  本轮Inspector观察钩子已关闭；未发送新Action，未暂存/提交/推送。

#### 10.72.4 下一批从这里继续

1. 手机解锁后直接复用已安装App、已联网Hub和现有Active绑定，做真正设备页进入→owner1→fresh State→一次Button ACK/Patch，
   再退出/后台→owner0与Hub executionWanted0、恢复前台→新owner；补Session更换及one-shot只读。无需再次认领或重置子设备。
2. 然后实施唯一物理composition：ordinary execution demand、管理会话所有权、异步断链占槽与取消wake。
3. Direct优先退让、无云有界让出、持续page renew不能永久占ACL必须配套；完成软件矩阵后再启用idle并删除旧eager入口。
   当前页面不会自动通知Hub为Direct让路，仍不声称同绑定Direct→Gateway→Direct完成。
4. 之后再做分阶段循环及长稳；N2/N3/N4/N5、D4b、SDK首因、Wio/Hub发布长稳继续保留，暂不扩散Mesh/多child/History。

### 10.73 真实设备页短门禁、后台语义校正与下一批入口（2026-09-07）

本批关闭的是§10.72.4第1项的**页面需求与有界回收**，不是BLE物理释放、Direct交替或发布长稳。
继续复用COM11 Hub、COM12 child、现有Active绑定及mostfun2026；没有烧录、reset、清除Access、轮换Key或更改真实topology。

#### 10.73.1 问题、修正与清理

- 首次前台报`HTTP_0`：手机重连USB后ADB reverse为空，App无法访问本机账户接口。
  恢复`7000→17001`与`8884→8884`后页面自动恢复fresh State及可控，不能把这次错误归因于BLE或Broker的恢复逻辑。
  可复跑门禁先恢复明确的本机映射，不导出登录令牌或设备凭据。
- App原本只监听`appStateChange`，Android实现要等`onStop`；增加`pause`时的提前取消。
  两个listener在销毁/迟到注册时释放；初始化`getState()`不能覆盖已收到的生命周期事件。
  以一个boolean记录是否已有事件，不引入生命周期计数器、后台任务、原生MQTT或另一套owner核心。
- 初版硬件脚本把15 s内收到release当作必需条件，出现后台/页面替换后远端owner尚在原租期内的失败。
  **不更改30 s协议期限**：区分本地取消、best-effort释放和失联到期兜底；在40 s有界观察预算内检查远端最终收敛。
  `pause`改进不代表Android冻结JS后还能可靠发包，也不声称主动release已稳定必达。
- 扩展现有`edge_gateway_page_demand_gate.mjs`为可复跑场景：只调用真实页面导航/Button、UI只读及账户start/stop；
  不伪造可见性、owner或设备MQTT。未知Action立即停止，不生成新requestId重试。
  Broker观察钩子复用旧工具，默认120 s不变，页面整段允许上限180 s；结束/异常均关闭，不新增生产诊断接口。
- 最终额外检查one-shot之后的**最后zero Decision**，再取Hub串口wanted0；防止把在途positive之前的一条旧zero误记成最终释放。
  临时App发送观察只记录action/revision/status，已恢复原方法；没有保留源码旁路或调试owner。

#### 10.73.2 最终真机证据

- 最终APK SHA256：`447ff724c8c815e6e7a5cc226d1c4f29ae1b28cf3d305dbacd53a7c4eb849cb9`，覆盖安装保留账号与库存。
  Hub应用、Broker镜像继续使用§10.72基线；本批不重新编译/烧录MCU，不更改Service/Broker业务源码或数据库。
- 最终门禁本机证据：`build/page-demand-verified-hardware-20260907.jsonl`（忽略、不发布）；整段退出码0。
  进入真实child页：owner1、fresh State、canControl；Button **5→6**，存在ACK/Patch，重发帧摘要唯一，没有第二次物理执行。
- 离页：本地scope0，Broker owner0，Hub executionWanted0。重新进入恢复owner1。
- Android HOME后台：**30,087 ms观测owner0**；包含随后10 s周期串口取样共**39,481 ms**观测Hub wanted0。
  此轮依靠原期限兜底；不是“回收期限39 s”，也不是BLE已物理断开。
- 返回前台重新申请；真实账户Session stop关闭旧scope且`canControl=false`，Broker owner0/Hub wanted0；
  start后新页面scope恢复fresh/可控。one-shot读取17 ms复用已经新鲜的状态，并释放自己的owner；不冒充一次全量无线State重新拉取。
- 最后一个Decision确为zero，Hub wanted0；同Hub MQTT、同child投影持续ready，State revision7、计数仍6。
  最终heapFree105,360 B、heapMin52,948 B、largest59,380 B；loopStackMin3,652、ipc0Min1,104、NimBLEMin2,816 B。
  这是短门禁水位，不是长期泄漏/SDK首因/生产encrypted-NVS的证据。
- 早期失败及较弱断言运行记录保留在`build/page-demand-*-hardware-20260907.jsonl`，不覆盖为成功。
  本轮全部人工/独立场景共执行6次已知Button，最终基线6；不能据此声称完成6次连接循环或10→30→100长稳。

#### 10.73.3 软件门禁与后续顺序

- App最终正式Angular入口35 suites / 272 tests、生命周期文件ESLint及local bundle/APK构建通过。
  跨仓真实App/Broker/C++执行租约21项、相关硬件工具32项通过；硬件工具测试不等于生产全部测试。
- **下一批：**按`DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md` 6.11集中替换唯一物理入口：
  ordinary demand、management exact会话所有权、permit-join互斥、异步Disconnecting占槽、取消wake及Cloud刷新/离线分支。
- Direct在线主动让出与无云有界公平性必须一起设计/测试；不能让持续page renew永远占ACL，也不能让App等30 s兜底才尝试Direct。
  配套软件与四端验证后再删除仍服务当前固件的Admission eager入口；不另加长期candidate开关、Action队列或MCU特例API。
- N2/N3/N4/N5、D4b、SDK首因、Wio/Hub长稳仍独立待办。没有暂存/提交/推送；不将本地证书/日志加入发布文件。

### 10.74 物理释放边界修正及SDK竞态门禁（2026-09-07）

本批从§10.73的物理编排前置审查开始，先修复真实资源生命周期错误。**不是唯一composition或idle/Direct公平性已完成。**
不改变公开Arduino API、三端wire、身份/Access/Key、Action ledger或需求租期。后续入口补充到需求合同6.12。

#### 10.74.1 三层修正与精简

- 记录层disconnect/协议错误会先退役记录会话，原`GattDirectChildLink`据此提前向ChildSession通知断开；
  但SDK ACL可能仍在Disconnecting。现在复用原retirement字段保存closing attempt，先停记录收发，
  原生完成后才通知上层释放；扫描/连接取消没有record session时也补齐完成通知，不再等外层超时或遗漏回调。
  保留record层错误后的清理，避免修复取消路径时遗漏已存在的记录会话。取消失败只重试cancel，不新建attempt/Action。
- `GatewayChildSession.processFailure()`不再把`connected=false && connecting=false`解释为物理空闲。
  Link有attempt便保持Disconnecting，继续poll/取消；迟到旧attempt断开回调不能误清当前会话。
  这样现有proof/revoke/renewal的exact清理权不再因上层提前Idle而丢失。
- ESP32取消与CONNECT成功竞态：收养实际ACL并terminate，收到真正完成/原生表确认不存在后才清理。
  `ble_gap_conn_cancel()`返回EALREADY可能是CONNECT已排队，不制造合成完成；扫描cancel失败不挂入完成事件。
  GATT discovery失败、terminate被拒绝均保持占槽并重试，已接受的terminate不反复发送。
- 扫描转连接时SDK拒绝停止扫描：保持Scanning和原attempt，后续广播可重试；不再completeAttempt后假装Idle。
  stop等待同时覆盖native connecting和迟到CONNECT，超时保留Error，不伪报Stopped；没有因此反复销毁/重建NimBLE host。
- 没有新增运行时任务、Action队列、eager/demand双配置或安全协议；Gatt link复用已有字段，Central只增加一个请求已接受标志。
  新SDK桩放`tests/host/stubs/esp32_nimble`，实际生产Central编译进Host测试，不复制其状态机或进入发布源码。

#### 10.74.2 软件门禁

- Host ASan全量48/48、20万次协议fuzz、架构检查通过；GATT link/ChildSession新增延迟取消、拒绝取消、扫描/连接超时、
  丢失/迟到回调、错误后占槽及代次隔离断言。原32次link恢复测试保留。
- 新原生Central测试覆盖显式scan取消失败、扫描转连接取消失败、正常connect取消、成功事件在cancel前/后到达、
  discovery失败时SDK拒绝terminate后重试、stop期间迟到成功事件、stuck connect不能伪报Stopped。
- SDK边界桩证明本库处理，不代表控制器/任务调度仿真；不据此关闭ipc/SDK首因、发布长稳或D4b。

#### 10.74.3 最终构建与四端真机证据

- 最终Hub应用SHA256 `5e5ba8f12c635d06bd772009eba447959d74f355f2f50429b616e4e6c5b08ddd`，
  `build/physical-close-hardware-esp32-3311/Esp32EdgeHubDualBoard.ino.bin`；3.3.11/现有SDK配置，
  Flash 1,441,624 B（对§10.72 +168 B），静态RAM仍65,452 B。仍是内部weak-storage工程probe，非D4b生产加密板。
- COM11核对Hub MAC `8c:bf:ea:1b:96:14`与实际3072 B分区表摘要
  `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`后仅写0x10000应用，esptool验证通过。
  未写bootloader、partition、NVS或eFuse；未烧录/重置COM12 child、清除Access、重新认领或刷新Key。网络仍mostfun2026。
- 第一版（最终扫描转连接错误分支补充前，SHA `ea1d0c474bbdfd68c1ee943d20be47d9952a3a67fcdb9388980152210124a27c`）
  普通serial `d`后1,551 ms观测Secure；Broker在观察起点后3,166 ms观测新child投影ready，同Hub MQTT，计数仍6。
  `physical-close-broker-before/after-20260907.json`及`physical-close-disconnect-hardware-20260907.log`保留此版本证据；
  页面整段6→7通过，后台30.108 s观测owner0/39.480 s包含串口确认，不冒充最终二进制的运行。
- 最终版再烧录/验证后，普通`d`：11 ms观测Disconnecting，28 ms开始新连接，683 ms认证中，**1,377 ms观测Secure**；
  随后持续secure/error0/routeError0。证据`build/physical-close-final-disconnect-hardware-20260907.log`。
  这些是主机串口观察时刻，不是无线抓包精确时刻；人工断开后旧eager自动重连，不等于物理idle已启用。
- 最终真实Android页面完整门禁`build/physical-close-final-page-hardware-20260907.jsonl`退出码0：
  进入owner1/fresh/canControl，**单次Button 7→8**，唯一Command帧摘要及ACK/Patch；离页scope0/owner0/Hub wanted0，
  后台、前台恢复、真实账户Session stop/start、one-shot只读均通过。后台29,828 ms观测owner0，39,632 ms包含Hub串口取样。
  沿用原30 s期限兜底，不代表主动release必达或即时断开；one-shot 17 ms复用fresh state，不冒充全量wire拉取。
- 最后zero Decision、owner0、Hub wanted0；同Hub MQTT/同child投影在页面整段保持ready，State revision9/计数8。
  最终heapFree105,232、heapMin94,548、largest61,428 B；loopStackMin3,512、ipc0Min1,104、NimBLEMin2,792 B。
  重烧录重置了Hub统计，不能把本次min与旧版长期min比较后声称内存改善。
- App沿用§10.73 APK，Broker/Gateway沿用§10.72基线，没有业务源码/镜像/APK或数据库管理操作。
  最终App回设备列表、scope0/active=true，Broker Inspector确认关闭。本轮两次明确Button共6→8，不算长稳/循环门禁。
  本地证据/证书仍忽略，不进发布包；没有暂存、提交或推送。

#### 10.74.4 实施边界与下一批

1. 仍需将同步stop式permit-join交接替换为poll驱动独占交接，正向和反向都要等原生资源释放后再换handler。
   **必须同步修改permit-join coordinator对openWindow/WouldBlock的处理**，在原命令期限内等待；不能只改调用处造成配网失败。
2. 集中ordinary/proof/controller-control的start/connect至唯一物理编排，继续尊重exact管理清理权和permit-join独占。
3. 配套Direct在线主动让出、无云有界占用/让出、持续page renew与管理争用，封住CloudConnecting/ManagementSubscribing保留ACL的旁路。
   在这组验证前仍保留当前服务固件需要的Admission eager入口，**不声称按需物理idle或同绑定Direct→Gateway→Direct完成**。
4. 不重做codec/owner/管理预算，不扩散Mesh/多child/History。N2～N5、D4b、SDK首因及Wio/Hub长稳仍独立待办。

### 10.75 permit-join双向异步交接与四端恢复门禁（2026-09-07）

本批落实§10.74.4第1项和需求合同6.13。完成临时radio交接前置，不宣称唯一物理调度、idle/Direct公平性已完成。

#### 10.75.1 实现与解耦

- `NativeGattPermitJoinAdapter`新增Opening/Closing状态，保留旧Stopped/Ready/Fault数值；只在lease成功且Port Stopped后换handler。
  open/WouldBlock保持原事务，poll推进获取；取消未获取的Opening不动原owner。反向关闭立即清候选和收发队列，
  保留exact native attempt/callback，取消扫描、在途连接或ACL，直到真实完成才解绑并release。
  普通关闭不再强制同步stop活动ACL；析构的终端清理仍保留，避免悬空callback。
- Coordinator把命令上下文与adapter物理状态分开：原命令有界等待/幂等、开窗实际Ready、关闭释放后终局；
  复用`GatewayManagementBudget`钉住剩余期限（permit沿用Service的900 s上限），重复Open、时钟回拨不能延长。
  reset清除旧结果，不假设异步radio已归还；mux在无active command时仍处理剩余关闭，拒绝另一个管理family抢占。
- Product原同步stop交接改为disconnect/poll至ChildSession和Port真实Stopped；Opening旧owner驱动原handler，
  Ready/Closing只有permit adapter poll radio；非Active仍drain，restart不越过未结束关闭。
  `GatewayManagementClient`即使收到无效消息也继续poll已有控制任务，防止关闭清理被持续无效输入饿死。
- 同批纠正旧交接清`RouteBridge.reset()`的问题：临时radio占用不是路由鉴权失效；Broker准入重试有界，不能指望它永远重发。
  抽出`pollControlPlane()`维护Execution/Closure/准入而不连接BLE，permit期间可公布旧投影关闭，归还后沿用原准入恢复。
  Cloud非Online仍清旧代准入；没有跨MQTT代保留权限，也不自动重放未知Action。
- 未新增wire、公开Sketch API、安全协议、任务或Action队列。App/Service业务代码、APK/镜像未变。

#### 10.75.2 软件门禁

- Host ASan 48/48、20万次协议fuzz、架构检查通过；复用真机工具的11项Node测试通过。
- 覆盖等待lease不换handler、未获取前取消、scan/connect/ACL三态延迟关闭、取消被拒绝后重试、接受后不重复、
  Closing迟到connect/packet/旧disconnect不恢复收发、不提前release；同window重发不提前公布终局。
- 覆盖Opening到Ready/到期/Close、900 s上限、monotonic回绕+Unix回拨不续期、reset后mux清理和跨family排他。
- RouteBridge维护路径独立发送exact Closure且不start/connect，原准入保留后可恢复普通连接；无效管理消息仍推进关闭。
- 续期测试原来把四个mux入口都别名到同一coordinator，现改为独立拒绝入口与renewal实例，保留NonceReady/exact所有权断言。
  这是纠正不真实的fixture，不放松生产互斥。

#### 10.75.3 构建与真机记录

- 最终应用`build/permit-handoff-esp32-3311/Esp32EdgeHubDualBoard.ino.bin`，SHA256
  `9d4e31d15cb1e25b32d6afc07d09462031d6ec8bb95d63c25cdebdfb8f388a38`。
  SDK 3.3.11，Flash **1,442,772 B**（较§10.74 +1,148 B），静态RAM **65,460 B**（+8 B）。
  初次全量构建后又做最终增量构建，只有此最终版本进入硬件；仍为内部weak-storage工程probe，非D4b生产加密板。
- COM11实测MAC `8c:bf:ea:1b:96:14`；读取0x8000处3072 B分区表，SHA256仍为
  `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`。仅烧录0x10000应用，esptool验证成功。
  没有写bootloader/partition/NVS/eFuse，没有烧录、重置或清Access的COM12 child，也没有重新认领、刷新Key或轮换Gateway credential。
  读取/烧录会复位Hub，本批不把新启动的min值当作长稳改善。网络mostfun2026，继续用本机TLS测试入口。
- 使用实际Android已登录账户和既有`edge_gateway_connection_gate`，按child逻辑ID/instance和授权presence选择目标，
  **没有按MAC、名称或候选列表第一个连接**。普通窗口operation `2c3cd2e06db654b90e676a96edb56504`：
  App计时3,043 ms relay Connected、5,470 ms鉴权就绪、7,899 ms Manifest/State同步、8,555 ms关闭清理完毕；
  links/sessions/checkpoints均0。这是HTTP/MQTT relay端到端计时，不是手机原生BLE建链时延。
- 同轮Hub串口观察：12,274 ms时child Disconnecting/permit Opening，12,299 ms child Idle，12,322 ms child Stopped/permit Ready；
  18,560 ms关窗后普通Connecting，19,771 ms Secure。观察点之间的1,211 ms是恢复连接到Secure取样差，不是RF精确计时。
  Broker观察到exact Closure并把旧投影置非ready，再收generation2新Hello/Manifest/State恢复ready，**同Hub MQTT**，计数仍8/revision9。
  手机/PC/Broker计时各有起点，不能跨日志相减推导精确时延；证据在`permit-handoff-radio-20260907.jsonl`、
  `permit-handoff-connection-20260907.json`、`permit-handoff-broker-before/after-20260907.json`。
- 故障门禁operation `9282c661635ea6129a9140d8be9f10b3`：真实安全会话后发送一条无效应用记录，真实子设备主动退役ACL；
  7,284 ms观察window Rejected。没有伪造SDK断开事件、写凭据或重置设备。随后新operation `5bbc23e632fc8cbf41e3d7d9407d3972`
  再次通过鉴权/Manifest/State，7,398 ms完成关闭；两个operation各自links/sessions/checkpoints=0，脚本退出码0。
  连续窗口结束后Hub原准入自行恢复generation3、同MQTT且计数仍8。证据`permit-handoff-selected-acl-20260907.json`、
  `permit-handoff-broker-negative-20260907.json`。这是三个短场景，不是10→30→100次门禁。
- 最后执行实际设备页完整门禁`permit-handoff-page-20260907.jsonl`，退出码0：页面进入owner1/fresh/canControl，
  **单次Button 8→9**；Broker记录两次相同Command摘要
  `6554acaa06e73d177f92a882ca0522b129e86467959b665686b62937c8074ea5`，计数只增加一次并收到ACK/Patch。
  退出、后台/前台恢复、账户Session stop/start、one-shot只读均通过；页面整段同Hub MQTT/同child投影，最终revision10/计数9。
  后台29,898 ms观察owner0，39,556 ms含串口wanted0确认，仍依赖原30 s上限兜底；one-shot 15 ms复用fresh状态，非全量wire测试。
- 最后App回列表、scope0/active=true，Broker owner0/最后zero Decision、Hub wanted0，Inspector确认closed。
  Hub末次heapFree105,232、heapMin91,088、largest61,428 B；loopStackMin3,652、ipc0Min1,112、NimBLEMin2,708 B。
  App沿用§10.73 APK，Service/Broker沿用§10.72基线；无App/Service业务源码、镜像/APK更换或直接数据库修改。
  运行既有API会产生真实permit窗口ledger，不等于手工改库。证书/二进制/运行日志仍忽略，未暂存/提交/推送。

#### 10.75.4 下一批

1. 集中ordinary/proof/controller-control连接入口为唯一物理编排，保留exact管理清理权和本批permit独占。
2. 同批实现在线Direct主动让出、无云有界占用/让出和持续page renew的公平性，封住CloudConnecting/ManagementSubscribing旁路。
3. 以上配套通过后再启用物理idle、删除被替代的Admission eager连接代码；不保留长期双配置，不增加Action等待队列。
4. 同绑定Direct→Gateway→Direct、N2～N5、D4b、SDK首因与Wio/Hub长稳仍独立待办；不扩散Mesh/多child/History。

### 10.76 连接入口收口及管理关闭屏障（2026-09-07）

本批落实§10.75.4第1项，修正第2项执行顺序，详见需求合同6.14。
先完成生产接线和针对性回归；Direct主动退让需要独立控制合同，不能把permit-join或清空云端需求当作替代。

#### 10.76.1 实现与职责

- 新增一个无堆、无队列的`BasicGatewayChildConnection`运行时机制，通过两方法接口注入proof与RouteBridge。
  ordinary/proof/revocation/renewal实际复用同一实例；没有候选未接线的scheduler，也没有增加平台用户API。
- 从RouteBridge删除普通start/connect，从proof删除重复start/connect/disconnect，从product删除旁路disconnect；
  终端stop与permit确认quiescent后的stop仍由composition负责。GATT Link/Port继续负责真实native完成和radio handler交接。
- 关闭记录只存exact attempt，独立于命令上下文；reset不能遗忘尚未完成的原生退役。取消拒绝可重试，接受后等完成；
  同attempt Fault不冒充空闲，旧attempt清理不影响新连接。proof与ControllerControl即使看到Secure也不能越过关闭屏障。
- 新测试发现并修正：secure复用检查若同步完成旧关闭，不能在同一次调用里紧接着start/connect；
  返回WouldBlock让调用方重新评估状态，避免出现未被当前操作记录的新attempt。
- 普通连接策略目前仅保留在product一处：有效Admission且管理未占用才调用共享入口。
  这是已有行为的集中，不是按需wake/idle已启用；CloudConnecting/ManagementSubscribing仍保留原已连接子会话。
  所有新获取由Online及permit独占门控制，不改变身份、DeviceKey、三槽、wire或业务Action语义。

#### 10.76.2 软件与构建门禁

- 最终Host ASan 48/48、20万次确定性协议fuzz、架构检查及11项真机工具Node测试通过；新增用例在现有test target内运行，没有增加重复测试工程。
- 覆盖默认禁止获取、Stopped启动/单次connect、重复ensure不重开、secure复用、cancel背压、异步关闭不重复、
  同代Fault占槽、permit禁止获取、迟到旧代cleanup、proof reset后跨调用屏障、仍Secure时proof和管理发送均不越过屏障。
- 架构门禁禁止RouteBridge/proof重新直接start/connect/disconnect/stop，也禁止product旁路start/connect/disconnect。
  授权检查留在协议，连接机制留在runtime，SDK留在port；不通过合并成大平台文件达到表面“文件少”。
- Host证据：`build/connection-owner-host-20260907.log`；最终SDK 3.3.11构建退出码0，见`build/connection-owner-compile-20260907.log`。
  Flash **1,443,032 B**（较§10.75 +260 B）、静态RAM **65,492 B**（+32 B）；没有新线程、堆分配或平台专用用户初始化入口。

#### 10.76.3 新固件四端短门禁

- 最终应用`build/connection-owner-esp32-3311/Esp32EdgeHubDualBoard.ino.bin`，文件1,443,184 B，SHA256
  `ecfe7e73c5cbb76a57889b7e55e92eea24e5a3c67f19c6ef3df603bae84cb2c4`。构建对象晚于最终源码修改，烧录后未再改生产源码。
  COM11 MAC核对为`8c:bf:ea:1b:96:14`；分区表实读哈希与§10.75一致。仅写0x10000应用并通过esptool hash验证。
  烧录未覆盖bootloader/partition/NVS、未写eFuse；未改COM12 child、未reset/清Access/重绑或轮换凭据。
  Hub读取/烧录会正常复位，应用启动仍执行正常登录/存储流程；仍是内部weak-storage工程probe，非D4b生产加密板。
- 真实Android已登录账户、现有App/Broker镜像、原Hub/child绑定及mostfun2026网络；App按logicalDeviceId/instance/授权presence选目标。
  普通relay窗口`9b46027d1d5a3289d067cb27333af673`：3,303 ms Connected、5,779 ms authenticated、8,434 ms同步，9,091 ms完成清理。
  时间为App经HTTP/MQTT relay的端到端阶段，不是手机原生BLE建链时延。
- 故障窗口`6d9e08b2cd3fc92cd3c93cb32ca30922`在真实鉴权/同步后发送无效记录，真实child退役ACL；7,309 ms观察Rejected。
  下一窗口`16019fad2ffe3a84ae30456165aaa85c`重新鉴权/同步并在7,213 ms清理完毕。
  三个operation均links/sessions/checkpoints=0，工具退出码0；不是10→30→100或长稳。
- Broker收到exact Closure后撤旧投影，关窗沿用原准入恢复新Hello，generation1→2→3；整段同Hub MQTT，计数仍9/revision10。
  证据：`connection-owner-connection-20260907.json`、`connection-owner-selected-acl-20260907.json`、
  `connection-owner-broker-before/after/negative-20260907.json`（均在忽略的build目录）。
- 最后实际设备页门禁退出码0：单次Button **9→10**、相同Command摘要两次下发但只执行一次，ACK/Patch正确；
  摘要`1965aa6c18355b15e8dfd8bfecc383d97587ac0568d35fd6aa18b0f4e259241e`。
  页面退出、后台/前台、账户Session stop/start、one-shot只读通过；页面整段同Hub MQTT/同child投影，最终revision11/owner0/wanted0。
  后台29,625 ms观察owner0，39,413 ms含串口wanted0确认，仍依赖原30 s上限；one-shot 16 ms复用fresh状态，不是全量wire同步。
  证据`build/connection-owner-page-20260907.jsonl`。App恢复列表、scope0/active=true，Inspector由工具关闭后独立确认closed。
  末次串口heapFree105,336/heapMin94,120/largest59,380 B，loopStackMin3,592/ipc0Min1,112/NimBLEMin2,764 B；
  routeError=0/secure1/wanted0，见`build/connection-owner-final-20260907.log`。这些短时取样不替代长稳。
- App/Service本批仅更新交接文档，业务源码/wire/APK/镜像不变；既有API产生真实短期permit ledger，不是手动SQL改库。
  子设备计数基线现在为10，后续不可继续用9判定重复执行。未暂存/提交/推送；不把新Hub启动后的资源min值当作长稳改善。

#### 10.76.4 下一批与范围边界

1. 先冻结需求合同6.14的Direct有界优先权/取消/ACL与exact关联，再同批实现三端消费和唯一物理决策；不复用配网窗口。
2. 在线主动让出、持续page renew公平、离线有界持有/让出及Cloud刷新旁路必须一起收口；管理exact cleanup不能被idle打断。
3. 配套通过后删除product唯一Admission决策，启用物理idle；不长期双模式，不增加未知Action自动重试或等待队列。
4. 同绑定Direct→Gateway→Direct、N2～N5、D4b、SDK首因与Wio/Hub长稳仍待办。未完成项不会因本批连接入口统一而自动通过。

### 10.77 Direct优先权合同、共享owner核心与跨仓负测（2026-09-07）

#### 10.77.1 本批结论与实现

§10.76接线后的下一步必须区分“已受理Direct意图”和“Hub原生连接已经让出”。
本批冻结这项语义和authority/生命周期边界（需求合同6.15），实现其owner核心并防止误走现有wire；
**尚未接通线上Direct请求、Hub Yielded回执或App自动物理消费**，不将软件核心当成完整功能。

- 复用现有App `DeviceV2DemandOwners`和Broker `ConnectionDemandOwners`，只增加不可变purpose及Direct绑定信息；
  不建立第二份注册表/ID分配/到期器/等待队列，不新增设备端代码、任务或用户Sketch API。
- cloud与Direct分别聚合，Direct不能唤醒cloud，也不能通过另一用途的release取消cloud；同session共用ownerId高水位及既有容量限制。
  多手机竞争同一逻辑目标时，首个Direct候选在异步ACL开始前预留，后者Busy；释放或超时后不会有排队者自动继任。
- Direct授权必须携带非secret exact binding及其本地单调deadline；ACL版本单独保存，不破坏现有keepVersion失效规则。
  renew遇到Hub/拓扑关联变化即退休，不迁移旧owner；期限裁剪至原请求锚、账户与绑定期限，迟到授权不能增加时间。
- App从acquire到renew/release都保留purpose；取消、拒绝、失联/账号切换沿用原生命周期，不自动变成cloud请求或创建新owner重试。
- 为避免未接线时静默误用，App/Broker现有四项ConnectionDemand编码器拒绝purpose，Broker普通read authority也拒绝Direct。
  当前生产Session、页面、feature、MQTT部署仍只走已有cloud路径；既有App/Broker cloud codec/golden不改变。

#### 10.77.2 验证与证据边界

- Broker TypeScript编译/类型检查通过；owner、真实app-lane及Execution **51项**通过；`test-device-v2.js`整体回归通过。
- App需求核心、Session wire、页面作用域、连接选择和账户客户端最终 **63项**通过；
  特意绕过公开API把Direct意图交给当前生产Session时，也不产生任何cloud acquire/cleanup报文，证明不会静默降级。
- 跨仓脚本加载两仓真实TS核心，**13项**通过：新增双用途、双手机Busy、Direct release丢失且cloud继续renew、
  取消抢先于迟到ACL、target ACL双用途撤销、Hub binding变更等。没有复制状态机或启动虚构的MQTT/MCU替代物。
- 设备源文件未改；重跑原Host **48/48**和架构检查通过。未重复烧录/重置设备，未更新App APK/Broker镜像或修改真机联调数据库。
  Broker测试使用独立临时测试库；既有真机child仍沿用§10.76计数10/revision11基线，不能称本批真机Direct通过。
- 忽略目录证据：`build/direct-priority-contract-20260907.log`、`direct-priority-broker-20260907.log`、
  `direct-priority-broker-regression-20260907.log`、`direct-priority-app-20260907.log`。

#### 10.77.3 后续唯一执行顺序

1. **下一批先完成Direct wire/golden与真实authority适配：**沿用账户Route/Delivery和existing owner核心，独立明确协商；
   新wire编号/shape尚未发布。Broker映射当前Direct owner到exact Hub控制，不给未支持Hub或普通read权限误发优先权。
2. **Hub退让回执与物理决策：**receiver挑战/单调锚、旧控制/旧会话/取消/到期拒绝；ordinary停止获取并等待原生关闭，
   管理/permit占用时不能假报Yielded。只复用§10.76连接机制，App accepted不等于Ready。
3. **在线与离线公平、App消费配套：**同时替换唯一Admission决策，封住CloudConnecting/ManagementSubscribing保留旁路；
   App按exact Yielded再Method 2，退出/失败取消自己的优先权。未完成前保留当前服务策略，不能另开eager/demand双模式。
4. **同绑定四端门禁：**Direct→Gateway→Direct、持续cloud页面需求下让出、管理争用、双手机、取消wake、失联/撤权、
   全丢release及恢复。逻辑30 s期限不证明手机ACL已关闭，也不是物理公平时间默认值。
5. 本批不扩散Mesh/多child/History/Bulk；N2～N5、D4b、SDK首因与Wio/Hub长稳仍独立待办。未暂存/提交/推送。

### 10.78 Direct预留northbound、当前权限/Hub绑定及跨仓wire验证（2026-09-07）

本批完成§10.77.3第一项，不把预留和原生资源释放混为一谈。Hub Yielded、在线/离线公平、App物理消费尚未完成。

- **消息：**冻结bit17与inner0x1e/0x1f，采用canonical四整数请求/状态；复用Route/Delivery关联、owner编号、取消/重放预算。
  cloud旧kind/encoder不接受Direct用途，新旧purpose不能互相release。两端互解golden、frame/peer/requestId及非canonical负测通过。
- **App：**Session新增`reserveDirectPriority()`并复用既有owner sender、renew与cleanup；明确返回accepted而非Ready。
  未协商时不发包、收到cloud status拒绝；页面/account连接选择尚未消费，现有cloud需求不受影响。
- **实际authority：**内部app-device-access的purpose=direct验证当前账户/tenant/shard和已登记Direct Admin，回非secret accessEpoch。
  当前cloud viewer/operator没有Controller已同步的完整证据，因此Direct拒绝，cloud分享保持不变；不是假定read权限足够。
- **exact Hub：**查询Active child route并核epoch，再捕获已有MQTT/admission投影；复用原claims WeakMap生成进程内binding，不另建owner表。
  同sessionId MQTT重新登记、断线、能力/投影变化使原绑定无效；实时谓词异常fail closed。ACL version与binding保持分离。
  bit18 GatewayDirectYield仅预留，设备lane/Hub尚未协商或广播，所以当前现场旧Hub不会被误当成支持退让。
- **清理：**撤掉§10.77的Direct一律拒绝临时分支，以真实authority替代；复用既有HTTP route resolver支持child方向，
  未新增登录/配网/secret合同、持久表、任务或另一套调度器。保留cloud-only编码防串用，不为删文件提前移除仍在服务的Admission调度。

验证：Broker类型编译、owner/真实app-lane/execution57项和完整Device V2回归；App预留/作用域66项与基础协议44项；跨仓17项。
Service假库与真实PostgreSQL测试覆盖Direct Admin注册前后、分享/错误shard/非法用途；使用独立随机schema并已清理。
设备源码本批未改，重跑既有Host48/48与架构检查通过；没有MCU、APK或Broker镜像部署，也没有测试设备重置或凭据修改。
证据保存在忽略目录`build/direct-priority-wire-*-20260907.log`，不得称为四端真机Direct退让通过；counter仍沿用§10.76的10/revision11。

**下一批：**Hub控制代次/receiver挑战/期限和exact Yielded回执（协议、实现与负测一并冻结）；
随后在唯一product同时组合cloud execution、Direct、management、permit、native closing与离线公平，封住CloudConnecting/ManagementSubscribing保留旁路。
App只在真实Yielded后做Method2/Manifest/State，再做同绑定Direct→Gateway→Direct、持续cloud需求、双手机及丢包撤权门禁。
N2～N5、D4b、SDK首因、Wio/Hub长稳仍独立待办；不扩散多child、Mesh、History/Bulk。未暂存/提交/推送。

### 10.79 Hub Direct控制与回执软件合同、receiver和Broker交换核心（2026-09-07）

本批推进四个相连子项：控制wire与跨语言golden、MCU receiver租约、实际RouteBridge解析/有界发送适配、Broker单次有限交换。
**它们是物理退让的控制底座，不是产品调度或真机Direct交替完成。** 完整合同集中在需求合同6.17，不复制为另一套owner/身份方案。

#### 10.79.1 实现与纠正

1. **独立kind5/12项控制：**Probe→Challenge→Decision→Yielded，最大200 B；现有kind4普通execution wire不变。
   共享编码/身份验证骨架，strict方向、canonical、长度/整型范围和跨kind拒绝；不发往child BBP，也不增加App MQTT登录或secret合同。
2. **先消除旧Probe碰撞：**Direct Probe必须带当前Hub route incarnation，不能沿用普通Probe空nonce。
   从当前authenticated Execution Challenge获取；两种receiver共享route incarnation、独立计数/语义。
   controlId在该incarnation内递增至uint32最大后停止，旧会话Probe不能匹配新锚或污染高水位。
3. **设备48 B核心与Bridge：**复用40 B `GatewayExecutionLease`，新增两个uint32保存当前/active控制编号；无任务/堆/队列。
   RouteBridge新增解析、same admission复用、退休清除、exact挑战与回执发送，复用原output；不自行连接/断开无线。
   重传不续期、不重置发送预算；zero先到不复活，旧zero不取消新承诺。真实native未关闭/其他radio owner仍存在时不能报Yielded。
4. **Broker纯交换核心：**只持一个现有owner token/binding/deadline投影、控制计数、单工作槽和最多一个zero取消；
   从原Probe本地锚裁剪Ready，首次Challenge/首次Yielded各自fresh ACL并重验当前authority。
   撤权、换Hub、错tuple、过期/迟到或不合作异步工作均fail closed；重复回执不延长Ready。不新建owner表、Action队列或定时继任者。

#### 10.79.2 验证与未激活边界

- Host ASan **49/49**；新增receiver/golden/时钟回绕/取消/旧nonce及真实连接机制fake-native关闭屏障测试。
  RouteBridge测试校验同一当前准入nonce、错epoch拒绝、zero独立于cloud、无连接副作用、重复消息发送预算不重置。
- 新协议 **22项**、Broker owner/app-lane/execution/Direct **70项**及完整Device V2回归通过；类型编译通过。
- 跨仓 **20项**通过，其中3项串联实际Broker类/Service codec/C++ receiver进程，验证保守期限、无释放不回执及MQTT退休。
  C++进程使用确定性时钟/随机值和显式quiescence输入，不等于真实BLE端到端。
- **200,000**次fuzz已加入kind5有效seed与decoder/roundtrip路径；架构检查通过。
- ESP32-S3 Arduino **3.3.11**、`XIAO_ESP32S3:PartitionScheme=default_8MB`编译通过：
  Flash **1,444,328 B**（较§10.76 +1,296 B），静态RAM **65,556 B**（+64 B，含48 B lease及发送状态）。
  产物为`build/direct-yield-esp32-3311/Esp32EdgeHubDualBoard.ino.bin`，SHA-256
  `7476da9486c33458ca0d67ccd8db608ddbd09f3d3354c52f82cdd43a1837c386`。
  当前编译包含receiver解析，但未被product调用的发送/调度路径仍可被链接器裁剪；完整激活后必须重新测量，不视为最终功能体积。
  静态RAM不替代运行heap/任务水位；旧§10.76固件产物保留，未覆盖或烧录。

Broker exchange尚未注册到生产device lane，product尚未调用`pollDirectYield()`，App尚未消费Ready。
因此bit18不广播、不在Hub lane启用，旧现场不能误收退让预留。没有APK/镜像部署、烧录/reset、Access/Key变动、暂存/提交/推送。
现场继续沿用§10.76 counter10/revision11；本批日志为忽略目录`build/direct-yield-*-20260907.log`。

#### 10.79.3 下一批顺序（不再扩散新底座）

1. **生产Broker接线：**把现有owner exact token/revision和当前Hub Execution incarnation接到单次交换；保持高水位至exact Hub authority退休，
   不因child Hello/closure/可达投影重建或owner释放而重新编号。完善显式renew到新revision的回执关联、旧Ready退休/连续性及取消，
   不把accepted当Ready，不用偷偷续旧timer掩盖renew空档；复用既有quota、current claims和发送路径。
2. **唯一product决策：**同时组合cloud demand、Direct承诺、management、permit及native closing。
   既有管理事务按原预算结束；新owner不得在已Yielded承诺期间抢回radio。清掉CloudConnecting/ManagementSubscribing长期保留连接旁路，
   配套无云有界持有/退让；物理时长由真机发现/鉴权测量收口，30 s逻辑lease不是默认公平窗口。
3. **App Ready接线：**补绑定原request/owner/revision/target的可取消Ready回应/查询，保留原请求时间锚；再用原ControllerCredential做Method2/Manifest/State。
   不增加第二套页面owner，不移除保护后直接抢连，不重试unknown Action。
4. **配套激活及四端真机：**软件负测通过后才启用bit18并删除被替换的旧Admission连接决策；同一绑定测试Direct→Gateway→Direct、
   持续cloud需求、管理争用、取消wake、双手机、全丢release/断云/撤权及恢复。无需清Access/换Key/重新绑定。

N2～N5、D4b、SDK首因及Wio/Hub发布长稳仍独立待办，不扩散Mesh/多child/History/Bulk。

### 10.80 Broker生产Direct driver、显式续期与MQTT准入生命周期（2026-09-07）

本批完成§10.79.3第一项，并补充真实消息入口、权限额度及准入恢复的联动负测。
**Broker源码已接线，但设备物理策略/App Ready尚未接线，不能宣布同绑定Direct交替或四端真机完成。**

#### 10.80.1 完成内容

1. **现有owner的只读投影：**`ConnectionDemandOwners.direct()`只返回已确认revision的token、binding、原deadline和验活/重验权限方法。
   初次鉴权未完成没有投影；renew等待时保留旧投影/旧期限，成功后原子替换token并携带previousToken关联。
   不增加owner表、id空间或持久记录。旧投影异步鉴权迟到，不能确认或删除新版本。
2. **复用原并发额度：**Challenge/Yielded的fresh HTTP检查同样受既有全局64、每App会话4个工作槽限制。
   ignored AbortSignal不提前归还槽位；不足则本次交换不能Ready，不增排队器。新grant的权限/binding/到期不可弱于该投影原期限。
3. **生产driver：**既有route execution binding承载Direct交换，owner changed及单个1 s定时器重新读取原registry；
   不缓存继任owner，不自动申请/续约，耗尽的同一revision不因轮询或重复通知重新开始。
   0x8c上行通过生产validate/dispatch，校验capability、方向、exact route与控制tuple；下行沿用原Hub route publisher。
   owner.binding必须等于**这个Hub的当前绑定**，不能只确认“某个同target owner仍有效”，否则会向旧/错误Hub发退让控制。
4. **显式renew：**同owner相邻已确认版本、同binding且无在途工作时，在Hub旧有界承诺仍有效期间发新Probe，不先取消旧承诺；
   Broker旧Ready立即退休，新revision须重新Challenge→Decision→fresh ACL→Yielded。
   新Probe失败/到期或release，取消最后可能生效的exact承诺；新Decision已发送后zero新challenge也能取消receiver旧承诺。
   若renew撞上未完成的HTTP/send，撤销旧交换、保留工作槽至真正结束，再读取现存新revision；可能存在Ready空档，不假称无缝。
5. **清除重放边界：**controlId从现有MQTT claims状态分配，贯穿owner结束、Hello/closure/可达恢复乃至同MQTT重新准入；耗尽不回绕。
   真正准入退休会更换进程内authorityId，使旧binding不可因相同route再次出现而复活；child投影退休则不更换authorityId。
   同一准入收到不同incarnation时Direct封闭，不能凭重复旧nonce重启编号；普通execution/management不因此被冒充为Direct就绪。
   新准入可重建交换，但同MQTT编号仍向前；没有新增wire字段、secret或数据库迁移。

#### 10.80.2 验证与证据

- Broker类型编译通过；owner/app-lane/execution/Direct、command生命周期、reachability及共享协议合计 **165/165** 通过。
- 真实App Route入口、真实HTTP适配、MQTT claims/route模块串联：预留、两次fresh ACL、exact Yielded、renew/release、
  错epoch、Direct权限撤销、未协商/错方向、旧MQTT、错Hub binding、非合作工作槽均通过。
- 实际child Hello→closure→新Hello保留同一Direct authority/Ready；同MQTT删除准入再建，相同route不复活旧token且controlId递增。
  外部HTTP、MQTT publisher与BLE对端仍为测试夹具；**不是实际Broker镜像/无线端到端测试**。
- Broker完整Device V2回归、跨仓 **20/20**、既有Host ASan **49/49** 与架构检查通过；标准Broker测试脚本已纳入Direct套件。
  日志为忽略目录`build/direct-driver-*-20260907.log`。
- 设备/child/App业务源码和wire均未改变，无新MCU资源数字、烧录/reset/清Access/改Key、APK/镜像部署、暂存/提交/推送。
  现场继续沿用§10.76 counter10/revision11，不复用旧证据宣称本批物理退让完成。

#### 10.80.3 下一批准确入口

1. **唯一product物理策略：**`Esp32EdgeHubProduct::poll()`目前仍两次设置获取许可，末尾按Admission连接；
   `acquirePermitJoinPort()`仍仅调用release。下一批必须一起处理这两个获取入口及管理原所有权，
   在实际关闭屏障结束后才调用`pollDirectYield(true)`，并在已Yielded期间防止新ordinary/management/permit重新取得radio。
   已在途管理事务沿原预算退出，不能靠`!secure`或disconnect返回值假报释放。
2. **在线/离线同一决策：**配套收口CloudConnecting/ManagementSubscribing保留ACL的旁路与本地有界持有/退让；
   只替换原Admission决策，不新增长期双模式scheduler。具体窗口由真实广播、发现、Method2恢复测量收口，不能照搬30 s逻辑lease。
3. **App Ready合同/消费：**在原owner作用域上关联request/owner/revision/target和原本地时间锚，取得真实回执后才发起Direct；
   不把accepted改为Ready、不重发unknown Action。priority取消不等于手机ACL关闭，仍由原Direct作用域负责断链。
4. **配套激活与真机：**设备策略与App消费软件负测通过后再启用bit18，删除被替换的Admission旧决策，执行同绑定Direct→Gateway→Direct、
   持续cloud需求、管理争用、取消/全丢release/断云/撤权恢复；不以reset/重绑替代交替。

N2～N5、D4b、SDK首因及Wio/Hub发布长稳仍独立待办；本批不扩散多child/Mesh/History/Bulk。

### 10.81 唯一 GATT 调度、管理准入与退让生命周期（2026-09-07）

本批落实 §10.80.3 的设备物理入口，并修正 MQTT/整机重启后的承诺边界。
**以下是源码及软件门禁进度；bit18 仍关闭，未部署固件，不能宣布四端 Direct 交替完成。**

#### 10.81.1 实施与精简

1. `BasicGatewayGattRadio` 是唯一 GATT 物理策略：普通连接只消费 `executionWanted()`；
   Direct holding、原管理所有权、permit 和 native closing 一起参与决策。没有新增 owner 表、任务、堆分配、Action 队列或用户 API。
   `GatewayChildConnection` 仍只负责 exact attempt 的连接/退休机制；协议与 SDK 不进入调度模块。
2. ESP32 product 改为调用该模块，删除 Admission 自动连接、CloudConnecting/ManagementSubscribing 无限保留旧 ACL 的分支，
   合并重复释放代码。其他 southbound 可复用 route/owner/管理合同，**不必复用 GATT 状态机**。
3. 管理 mux 在分派前实时检查物理准入，包含同一 MQTT poll 中刚收到的 Direct Decision。
   新 permit Open 必须在 adapter 进入 Opening **之前**拒绝；拒绝不产生 MCU 等待队列，仍由 Broker 原有有界 ledger 重试。
   coordinator 自己判断 exact continuation：同 family 不等于同事务。已持有的 Prepare→Apply 必须保持身份、版本及原 expiry，
   且保留原单调时钟预算；不同事务/延期命令不能借原所有权进入。现有 permit 重放/Close 可以完成原窗口的退出。
4. Yielded 必须满足 child/port 均 Stopped、无管理/permit owner、无 busy/closing；
   logical Idle、Fault、disconnect accepted 都不能替代 native barrier。发回执前锁住获取权限，
   回执之后普通、管理和 permit 的物理入口均不能重新取得 radio，直到原承诺到期或当前 exact zero。
5. MQTT/admission 退休：旧 wire authority 清除，但原剩余物理退让时间保留；重复 reset 不续期，新准入不能取消旧承诺。
   新 authority 的 zero 只能取消其自身当前交换。receiver 从 **48 B 增至 56 B**，新增 8 B 仅保存旧承诺的本地剩余期限，不写 NVS。
6. 整机冷启动另需一次保护：首轮调度起等待协议最大承诺 **30 s** 后才允许获取子设备链路，
   覆盖重启丢失 RAM、手机仍持旧回执的边界；可与网络登录/配网时间重叠。
   这是一次性 protocol-safety barrier，**不是每次 BLE 连接延迟，也不是给离线公平窗口随意设置 30 s**；millis 回绕不能重新触发它。
7. 无 Cloud/有效普通 demand 时，不制造离线业务 owner，也不周期性盲连；现有管理按原预算排空，随后关闭并等待原生完成。
   SDK 若永不回报关闭，继续 fail closed，不能用超时宣称资源已释放。

#### 10.81.2 验证范围

- Host ASan **50/50**；新测试直接执行生产 GATT 调度、连接机制、mux 与真实 Direct receiver（BLE/部分 coordinator 为夹具）。
  覆盖 100 次**软件模拟**获取/退让、取消 wake、busy/原生错位状态、同 poll 准入、permit Closing/reset、离线排空、zero、MQTT 恢复与冷启动回绕。
- 实际 proof/revocation/renewal/permit coordinator 增加 continuation/错误身份负测；
  Prepare 99 s 后切 Apply、墙钟回拨，仍在原第 100 s 到期，不延长事务。
- 实际 RouteBridge 测试包含新准入/旧 zero 与原物理退让期限；跨仓五套协议测试 **34/34**，20 万次 fuzz、架构检查通过。
- ESP32 Arduino 3.3.11 / XIAO ESP32S3 / default_8MB 最终编译通过：Flash **1,446,616 B**、静态 RAM **65,612 B**，
  相比 §10.79/§10.80 编译基线分别 **+2,288 B / +56 B**。无新任务/堆分配；未重新测量运行期 heap/stack 水位。
  产物 `build/gatt-radio-esp32-3311/Esp32EdgeHubDualBoard.ino.bin`，
  SHA-256 `420ab76326f93ecde79b2d62f85d2cd10fc7a8d86d99f591a82c5433c6b5470d`。
  最终日志 `build/gatt-radio-compile-final-20260907.log`；其他测试日志为 `build/gatt-radio-*-20260907.log`。
  软件循环不是 SDK/无线长稳证据，三仓 `git diff --check` 通过。
- 未烧录、部署镜像/APK、reset、清 Access、改 Key、重绑或暂存/提交/推送；现场仍沿用 §10.76 基线。

#### 10.81.3 下一批准确入口

1. **App Ready 回应/查询与消费**：使用既有 owner 的 request/revision/target 和原本地锚；
   accepted 不能触发 Direct，只有 exact current Yielded/fresh ACL Ready 才行。续期、取消、掉线、迟到回应不重建 owner/时间锚。
   App 自己的 Direct 作用域仍负责断开 ACL，不重试 unknown Action。
2. **剩余本地公平场景**：本批解决 Hub 断云释放；“手机离线、Hub 仍在线且另有持续 cloud owner”不是同一场景，尚未通过。
   如需无云请求也能抢到 Direct，须按真实广播/发现/Method2 的时间测量确定本地退让窗口，并合入这一个调度器，不能另建 eager/demand 双策略。
3. **配套启用和四端验证**：上述软件门禁齐备后才打开 bit18 并部署。使用同一绑定做 Direct→Gateway→Direct、
   持续 demand/管理争用、断云、冷启动旧回执、全丢 release、撤权及 10→30→100 次循环；不以 reset/rebind 替代交替。

N2～N5、D4b、SDK 首因、Wio/Hub 发布长稳仍独立待办；本批不扩散 Mesh/多 child/History/Bulk。

### 10.82 App Ready wire、Broker 新鲜投影与原 owner 消费（2026-09-07）

本批推进 §10.81.3 第1项的软件链路，并明确“底层消费完成”和“实际页面允许抢连”不是同一完成条件。
**bit18仍关闭，没有烧录/部署，不能宣称四端 Direct→Gateway→Direct 完成。**

#### 10.82.1 已实施

1. 冻结独立 App/Broker bit19、inner `0x28/0x29` Query/Status；exact canonical 数组与合法 duration 范围见需求合同6.20。
   复用原 target peer、requestId、owner/revision；accepted不改义，不与History冲突，不把查询传给设备。
2. Broker 通过 `directFor()` 查询当前 session 的 exact accepted revision，经真实 Hub driver 的剩余物理退让投影、fresh ACL及最终再次校验，才回应Ready。
   原 boolean route 查询被剩余时间接口替代；重复Query不重启Probe，不续租，不重放旧TTL。共享原 replay/pending 和64/4鉴权工作上限，5 s超时。
3. App 在原 owner内部追加 Direct Ready状态，复用同一 timer/request slot；无平行owner表/轮询任务队列。
   acquire原30 s首次等待、query发出时间锚、每revision新Ready、取消与非合作RPC保留容量均落实。
   accepted renew不能延长旧物理期限；新revisionReady前不新建BLE，旧链路至多按旧期限继续。
4. Session/AccountClient/Service与原 `DeviceUiConnectionScope` 已可等待/消费Ready；`assertReady()`拒绝已resolve后过期、取消或revision交接期间的新获取。
   原账号关闭传播到scope；没有跨账号重发、未知Action重试或云端需求降级。
5. 没有为本功能增加MCU状态/存储/SDK适配；设备源码本批未改，继续使用§10.81的唯一GATT策略。

#### 10.82.2 验证与边界

- Angular正式 `ng test`：**33文件、274/274**，包含17项Ready生命周期新测试及真实Session错kind/flags/peer/request/owner/revision，账号切换和原UI scope回归。
  最初直接Vitest运行Angular测试缺TestBed/JIT初始化，已改为项目正式入口；旧Hello golden已更新并保留reservation-only协商负测。
- Broker相关六套 **151/151**；真实App入口、真实HTTP权限适配、MQTT claims/Hub driver夹具串联，覆盖Ready重查、续期、撤权、Hub/账号更换、重复/超时/非合作容量。
  `test-device-v2.js`完整回归通过。无线/GATT仍为夹具，不将软件HTTP测试称为真机。
- 跨仓五套 **35/35**。旧内部reservation夹具现在明确只返回pending，不伪造物理Ready；实际App/Broker canonical查询/回应与外层关联golden通过。
- Host ASan **50/50**、架构检查及App/Broker类型检查通过。MCU源码未变化，不重复宣称新增资源/SDK/长稳证据。
- 日志：`build/direct-ready-app-angular-final-20260907.log`、`direct-ready-broker-20260907.log`、
  `direct-ready-broker-regression-20260907.log`、`direct-ready-cross-wire-final-20260907.log`、`direct-ready-host-20260907.log`。
- 未部署MCU/APK/镜像、reset/清Access/改Key/重绑或暂存/提交/推送；保留三仓原有工作区变更。

#### 10.82.3 下一批准确入口（不重做 wire/owner/driver）

1. **实际 transport selector 和 Direct scope 接线**：目前 gateway-child 页面保护仍在，尚未调用新的Ready预留入口。
   在已有 selector内串联 `reserveDirectPriority → scope.hold → assertReady → BLE认证/State`，不得由accepted触发BLE。
   只读sync/Page取消、后台/杀进程、renew空档和跨账号必须关闭原链路；未执行的连接失败可由原策略选择Cloud，unknown Action绝不重发。
   软件接线需以明确的配套准入条件控制，不能简单删除旧保护或新增第二套owner/连接引擎。
2. **手机离线、Hub在线且有持续cloud owner**：现有Hub断云释放不能覆盖此情形。
   在同一个GATT策略中用真实广播/发现/Method2恢复时间确定公平退让窗口；不凭空指定30 s，不另建长期eager/demand双配置。
3. **配套激活及四端门禁**：软件入口与公平策略齐备后才打开Hub bit18，部署一致固件/App/Broker。
   用同一绑定做Direct→Gateway→Direct、持续需求/管理争用、全丢release、断云/撤权/冷启动旧承诺，分阶段10→30→100轮；不用reset/rebind替代交替。

N2～N5、D4b、SDK首因、Wio/Hub发布长稳仍独立待办；本批不扩散Mesh/多child/History/Bulk。

### 10.83 实际 App Direct selector、原生准入与关闭屏障（2026-09-07）

本批完成§10.82.3第1项的软件接线和取消负测，不重做wire/owner/Hub driver，不新增MCU状态或存储。
**软件已能消费有效Ready；现场Hub bit18尚未启用，不能将本批测试表述为真实无线交替通过。**

#### 10.83.1 已实施及纠正

1. `DeviceUiPort` 在原页面/只读连接入口识别受管 child（`deviceType=ble && cloudEnabled=true`）。
   有本地凭据时使用 `reserveDirectPriority → scope.hold(accepted+ready) → BLE准入 → Method2/Manifest/State`，成功后才选择Direct。
   App bit17+19和Broker检查Hub bit18/真实Yielded仍是配套准入，未协商不能旁路直连。
2. 修正旧 `cloudReachable=false` 自动放行：Presence只是路由可达快照，不能证明Hub已关闭无线链路。
   受管 child 无有效Ready时不以nearby、旧缓存、WiFiProv handoff或缺云Presence绕过准入。
   未发送业务的能力/权限/连接失败可走原Cloud路径；取消、账号/Session退休不转到新会话；unknown Action不跨路径重试。
   纯BLE、普通WiFi/WiFi+BLE和Hub自身保留原有路径。本批不声称解决受管 child 的离线手机直连。
3. 原 `connections` caller group 只增加一个物理Direct scope；多页面/一次性读取共用它，最后一个调用者退出才释放。
   Cloud需求仍按原调用者独立管理；失败尝试不会在同一group隐藏重试。没有另一张owner表、调度器、轮询timer或native重试队列。
4. 无线层仅接收 `signal + assertAcquire + assertActive` 准入合同，不导入MQTT/UI owner。
   新获取在异步凭据读取、发现之后及实际SDK connect前检查当前revision/期限；已有链路用 `assertActive` 检查原有效期限。
   renew到新Ready之间只允许既有业务到旧期限，不允许新获取；发送每片、读取/通知也检查有效期，避免JS恢复后先执行迟到业务。
5. 原 service opening 保留到SDK实际工作结束；取消不提前清槽。native connect/服务发现/通知启用迟到成功都被关闭，
   原generation隔离disconnect/notification回调，重复disconnect共享原关闭Promise；旧工作不能误伤新连接。
   service→session→secure channel→native link统一等待实际关闭，不能因`closed`布尔值提前报告释放。
   认证后初始同步失败会关闭尚未交给service的session；账号退出后的关闭不能回写旧账号snapshot。
6. 删除受管 child 的两条隐式获取路径：后台PresenceKey维护只保存pending bundle，需既有授权连接才写入；
   command不能先ensureReady再隐式重连。页面使用connected-only发送，Direct scope过期/原生断线退役原调用者。
   显式入网/恢复/Admin事务仍走各自原工作合同，不混同普通页面连接。

#### 10.83.2 验证与边界

- 正式Angular入口 **44文件、382/382**：含实际selector18项、生产BLE服务准入8项、native transport21项，
  以及secure channel、初始同步失败清理、页面21项、状态桥、账户/后台、原协议与scope回归。原生SDK使用可控制迟到/关闭的夹具，不是真机。
- Broker相关六套 **151/151**，完整 `test-device-v2.js` 通过；跨仓五套 **35/35**；Host ASan **50/50**；架构检查、App app/spec和Broker类型检查通过。
- 首轮回归发现并修正账号退休后异步关闭回写旧snapshot；另修正取消测试的无效凭据夹具，最终无unhandled rejection。
- 证据：`build/direct-selector-app-final-20260907.log`、`direct-selector-broker-20260907.log`、
  `direct-selector-broker-regression-20260907.log`、`direct-selector-cross-20260907.log`、`direct-selector-host-20260907.log`。
- 仅App业务源码及三端方案文档变化；设备/Service/Broker业务源码本批未改。没有部署/烧录、重置、清Access/Key、重绑、暂存/提交/推送。
  不复用历史SDK资源数值冒充本批编译，也不将软件取消/后台测试称为真实手机force-stop门禁。

#### 10.83.3 下一批：剩余公平策略与配套真机

1. 不再重复实现selector/wire/owner。先用两块原绑定设备和手机测量：Hub物理关闭完成→child恢复广告、
   手机前台发现→Method2→首次有效State。记录最大值/长尾、原始阶段与SDK/固件身份，避免只测总耗时掩盖关闭卡住。
2. 对“手机无云、Hub在线且有持续cloud owner”在唯一 `BasicGatewayGattRadio` 内确定有界让出/重新获取规则。
   窗口须容纳上述实测阶段并明确恢复条件；无广告恢复证据就不能宣称可接管。
   本地公平是物理调度而非权限旁路：现有Controller鉴权不可省略；App受管 child 离线获取须有配套明确合同/负测，
   **不得通过恢复 `cloudReachable=false` 旁路来实现**。Hub断云释放和冷启动30 s保护不是该场景的完成证据。
3. 公平准入、原生关闭、管理续作/permit争用和取消软件门禁通过后，配套启用Hub bit18并部署一致App/Broker/Hub，
   以同一绑定执行Direct→Gateway→Direct，验证未知Action不重放、全部release丢失、撤权、Hub断云/冷启动旧承诺。
   按10→30→100阶段推进，前一阶段失败先定位，不能靠reset/rebind或扩大长测掩盖问题。

N2～N5、D4b、SDK首因、Wio/Hub发布长稳继续独立记录；本批不扩散多child、Mesh、History/Bulk。

### 10.84 一致部署、按需物理短测与 Android 扫描限额首因（2026-09-07）

本批已执行§10.83.3的现场测量，而不是仅延续软件模拟。**10次 Direct 基线未通过，不能继续30/100或冻结本地公平窗口。**
当前优先级改为需求合同6.22的 App 扫描资源边界；不改 child 认证、身份、凭据或用户 Sketch API。

#### 10.84.1 实际部署与可恢复边界

- Docker 经用户退出/重开后恢复；不清卷、不重置数据库。电脑新网段地址 `192.168.2.76`，Hub `192.168.2.63`。
  WiFi 信息只更新本地测试配置，Key不变；Gateway公开地址与本地TLS入口同步。TLS仍验证证书/主机名，未降为明文。
  旧TLS代理保留为 stopped 的 `blinker-device-v2-tls-proxy-before-20260907-ssid2`，可恢复；本地测试CA于9月11日到期，不能作为生产证书。
- **Hub COM11（MAC尾号96:14）**：Arduino ESP32 **3.3.11**，XIAO ESP32S3 / default_8MB；Flash **1,446,616 B**，静态RAM **65,612 B**。
  已读回并核对相同分区表，仅写 `0x10000` application 后正常复位；不写NVS、bootloader、partition或eFuse。
  bin SHA-256 `4d6c522ed56ff77326d53e007b1f3f7501270b4f3d5524af7ee88b9975665055`。
  §10.81的唯一 demand/idle 物理策略现在已真实部署，**不是仍在跑§10.76 eager固件，也没有新旧策略配置并存**。
  仍为内部 weak/plain-NVS 双板probe，不能替代D4b加密生产板门禁。bit18继续关闭。
- **child COM12（MAC尾号7c:6c）**：未烧录、未复位/清Access/重绑；MAC仅用来确认串口物理目标，不是授权身份。
  Direct测量再次校验本地凭据的exact instance并完成真实Method2，Hub恢复也仍使用原Active绑定。
- **App**：local build、Capacitor copy、JDK21 Gradle debug及ADB覆盖安装成功；账号/凭据/设备清单保留。
  APK SHA-256 `7e6b0e9afb317f3a377db526e51005fedcb98f65cc350a364c3ac6e77a9f4cb2`。
  §10.83的selector/native准入与关闭屏障现已安装；本批没有更改App业务源码。
- **Broker**：先升级App时发现旧镜像 `KNOWN_FEATURES` 只到bit16，拒绝新App Hello的bit17/19，表现为MQTT channel closed。
  保持协议严格校验，配套部署当前源码构建的 `aily-iot-dev-broker-shard-1:l2c-direct-selector` 后账户channel恢复ready，
  没有移除新feature、恢复Legacy JSON或降级登录。镜像config digest `6b0f97f4873a9a14f18ae54d0dc89d3260b0689b5152176d16b7fa290a356a80`。
  使用既有基础镜像+当前dist/协议源码，不改数据库或secret合同；旧镜像保留。部署override在本地build，不是已推送发布镜像。

#### 10.84.2 短测事实与资源

1. App无页面需求时Hub在线、managementSubscribed=1，child `Stopped / secure=0 / executionWanted=0`。
2. 打开原子设备页，通过正常账户Cloud路径获取新鲜Manifest/State，Hub变为 `Secure / wanted=1`。
   **只派发一次Button，计数0→1，error=false且State fresh。** 进入本轮前真实计数是0，不能沿用历史§10.76的10作为当前基线。
3. 离页scope归零，串口见 `Secure → Closing → Stopped`、wanted=0，MQTT保持管理订阅。
4. 再进页重新唤醒、读到fresh计数1；未追加Action。再次离页后scope/Direct links/sessions均0，Hub最终Stopped/wanted=0。
   这是同一绑定上的Cloud按需wake/idle短门禁，不是正常页面Direct→Gateway→Direct，也不是多owner或force-stop验收。
5. 短测末尾 `heapFree=104956 / heapMin=55356 / heapLargest=47092 B`，
   `loopStackMin=3652 / ipc0StackMin=1104 / nimbleStackMin=2720 B`。
   与首次wake相比largest/free峰值有变化，不能用两次恢复宣布无碎片/无泄漏；后续同基线记录峰值及每轮回收值。
   radio尚未初始化时NimBLE水位0不等于栈耗尽。此3.3.11基线没有复现76 B，不代表所有SDK首因已关闭。

#### 10.84.3 Direct分阶段测量与失败证据

新增 `tests/hardware/edge_gateway_direct_timing_gate.mjs`，复用已安装App的发现、GATT、Hello/Method2和初始State，不复制协议、不发送Action。
必须显式 isolated-baseline + Hub物理不抢连、exact child/instance及无其他调用者；该中立诊断准入不是Broker Ready或离线授权。
记录单调阶段，不记录raw地址/locator/Key/State值；SDK并发关闭排空后才进入下一轮，迟到完成归原轮次；失败立即停止并恢复observer。

| 新APK前5次成功的阶段 | 实测范围 |
|---|---|
| discovery调用→匹配授权广告 | 145～1000 ms |
| native connect→GATT准备完成 | 747～810 ms |
| Hello/Method2 | 852～1012 ms |
| 首次Manifest/State同步 | 932～968 ms |
| 整轮开始→fresh Ready | 2845、3685、3625、3859、3691 ms |
| native close开始→完成 | 114～128 ms |

请求10轮，**第6轮在发现阶段5s超时，实际只5轮成功**；每轮结束links/sessions均0。
ADB同时报告 `App is scanning too frequently` 与scan registration failed。Hub伴随48s观测保持Stopped/wanted=0/management在线，
故本次首因是手机扫描启动被系统拒绝，不是Hub抢占或ESP32认证失败。不能据此排除其他场景仍存在的抢连/恢复问题。

源码复核：App `transport.ts` 的 scanFor/scanForAll 各自start/stop，前者stop fire-and-forget；
安装的 `@capacitor-community/bluetooth-le` Android `DeviceScanner` 未覆写 `onScanFailed`，startScan调用后立即resolve。
因此Android失败没有传给App，只表现为设备未发现超时。Debug/OTA工具也直接调用BleClient，WiFiProv还有独立原生SDK扫描入口，
只在DeviceUi service加计数不能覆盖整个App扫描资源。

Android官方将过频定义为 `SCAN_FAILED_SCANNING_TOO_FREQUENTLY=6`，由onScanFailed上报；
[Android ScanCallback](https://developer.android.com/reference/android/bluetooth/le/ScanCallback)。
AOSP所查版本默认是30s窗口5次启动，且可由DeviceConfig调整，**不是可对所有手机承诺的固定配额**；
[AOSP AdapterService](https://android.googlesource.com/platform/packages/modules/Bluetooth/%2B/b94e4d791f/android/app/src/com/android/bluetooth/btservice/AdapterService.java)。

旧APK另有两组3轮观察（第二组排空关闭后Ready2694/3710/3586ms），只保留诊断证据，不累计成新APK门禁通过数。
原page demand工具对“离页后保留相同virtual child session”的eager假设也需更新；物理idle后重建session是预期，
应验原绑定/单Action效果与状态新鲜，而不是为了通过旧工具恢复永久占用。

#### 10.84.4 验证、证据和下一批

- 本批硬件工具八套 **82/82**，其中新工具 **7/7**（含异步准入复查、迟到并发关闭、失败清理/脱敏）；
  Broker相关 **151/151** + TSC、架构检查和三仓已跟踪差异空白检查通过。
  Hub真实3.3.11编译/应用烧录校验、App local/Gradle编译和安装均成功。§10.83的App382/Host50/跨仓35是上批结果，不冒充本批重跑。
- 原始证据都在本地 `build/`：`direct-timing-current-20260907.jsonl`、`direct-timing-current-hub-20260907.log`、
  `direct-timing-android-throttle-20260907.log`、`direct-fairness-page-{wake,release,rewake}-20260907.log`、
  `direct-fairness-final-idle-20260907.log`、`direct-fairness-hardware-tools-20260907.log` 及本批compile/upload/app/apk/broker日志。
- 下一批严格按需求合同6.22：**App扫描资源/错误通道→10次隔离基线→真实Hub关闭到广告恢复→离线公平合同/唯一调度→bit18及四端10/30/100**。
  不用人工sleep、蓝牙开关/重启、重绑、放宽身份/Ready或重发Action使门禁“通过”。
- 未修改三端业务源码、未暂存/提交/推送；当前留下正常应用、App设备列表和Hub空闲。工具不进入Arduino发布源码。
  N2～N5、D4b、SDK首因、Wio/Hub发布长稳继续独立阻断，不扩散Mesh/History/Bulk。

### 10.85 App扫描资源单一化、明确限额及原生互斥真机（2026-09-07）

本批实施需求合同6.22 S1/S2的核心及Android接线，不改BBP/2、Controller凭据、Ready或设备Sketch。
**修复的是重复启动扫描的资源管理与错误语义，不是限制已连接后的控制消息/Telemetry速率。**

#### 10.85.1 模块边界与清理

- App `core/bluetooth/scan-coordinator.ts` 是不依赖UI/MQTT/设备身份的单物理扫描核心，`scan.ts`仅适配平台。
  相同服务过滤器共享当前扫描，最多8个观察者；不兼容/关闭中请求明确Busy。每个观察者保留原单调deadline/AbortSignal，
  最后一个离开才等待原start和stop完成；旧generation回调与stop不能作用于后继。未知关闭失败保留资源隔离，不静默继续获取。
- 删除 `transport.ts` 两套独立start/stop，保留授权广告筛选/候选收集；crypto候选累计≤64、待匹配≤16，收集≤64。
  初始化也消耗原发现预算，迟到异步locator匹配不能把已超时发现变为成功。模式/PresenceKey/Method2鉴权保持原合同。
- Android只新增扫描模块 `ble/AndroidBleScanner + ScanBudget + BlinkerBleScanPlugin`；沿用原BleClient GATT/notify/write与安全客户端，
  不fork整个NimBLE/Android BLE插件、不直接修改node_modules。主线程串行SDK调用；native阶段≤30s，待上报广告≤64并有界合并，
  SDK失败用独立带scan id的事件上报，不能对已resolve的start再次reject冒充通知；后台由MainActivity原生生命周期关闭，即使JS暂停。
- 实际start尝试的进程级单调账本采用保守5/30s，取消/失败/换账号不退还。额外厂商拒绝记30s保守冷却；
  额度不足直接 `BLE_SCAN_THROTTLED` 并提供可用的retryAfterMs，不隐藏排队/重试或扩大原预算；系统真实额度仍以平台结果为准。
- BLE debug/OTA页也改用统一入口，删除原生直调和重复停止timer；异步初始化期间取消、离开缓存页面均阻止后续start。
  Web手动选择仍用Web Bluetooth，iOS使用原BleClient后端；本批不是iOS原生后台/失败回调认证。
- `capacitor-wifiprov` 新增可注入 `BleDiscovery` 扫描端口，两处SDK搜索统一转交；App `WiFiProvDiscovery` 接入同一Android资源/配额，
  与普通扫描互斥。stop/clearState等真实关闭完成后再确认；候选≤64。Espressif Security1、GATT、SSID/Key载荷不改。
  独立使用插件时保留SDK默认端口，App通过MainActivity在插件加载前注入；这不是让WiFiProv依赖App类或新增另一套配网协议。
  本地file依赖已通过离线pnpm更新和Capacitor update进入实际APK，交付需同步插件源码/锁文件，不能只复制App TS。
- 附近状态改为原连接记录内的15s单调证据期限、全服务一个到期timer；扫描取消/限额/失败不刷新原证据，不立即改判离线。
  成功扫描未匹配则结束本次发现，匹配则刷新；账号退休清除期限。还修正凭据准备阶段异常的清零路径及异步匹配后的账号复核。

#### 10.85.2 软件与真机证据

- 正式Angular **32文件266/266**，含共享扫描9项、transport22项、附近状态与账号期限回归；
  Android `ScanBudgetTest` **2/2**、类型检查/local build/Gradle assemble/架构通过，硬件工具九套 **86/86**。
  首轮原生构造初始化错误及测试对只读模块导出的spy错误已修正；最终编译/测试通过后才安装。
- 仅ADB覆盖安装新APK，SHA-256 `4581d53bfd137a3ad452b804a644368451ebee85bf1f4eaa7df641654620e493`。
  Hub/child固件、NVS/Access、Key/绑定、服务器镜像均未改变，沿用§10.84现场。未暂存/提交/推送。
- 真实新APK连续Direct：Ready **2916、3850、3618、3713、3713 ms**；第6次在发现起点后约21ms、整轮约50ms内
  得到 **BLE_SCAN_THROTTLED**，没有GATT获取、认证重试或残留links/sessions。旧版的5s设备扫描超时误分类已消除。
  原始count10工具仍退出1，因为实际并非10次成功；**本轮只证明5次正常连接+一次明确过频拒绝，不改成功口径**。
- 随后单独显式恢复调用Ready **2782ms**、正常关闭2912ms，未经蓝牙开关、进程重置、擦除或重新绑定。
- `ble_scan_resource_gate.mjs` 真机：Direct扫描阻止WiFiProv扫描；WiFiProv阻止Direct；非owner/旧id的stop不误停占用者；
  正常释放后另一入口可启动。此门禁仅扫描，不选择外围设备，不执行配网/密钥安装。首轮工具误读message为detail已修正并补3项工具单测。
- 真实按Home再回App：MainActivity原生发出 `BLE_SCAN_BACKGROUND`，测试扫描关闭；不是靠JS倒计时或改变设备权限。
  本次未force-stop整个App来验证所有入网事务，也未强制触发更严格厂商的原始onScanFailed路径；这些仍需独立负测。
- 最后普通Cloud页 `ready/fresh/canControl=true`，counter仍 **1**，没有新Action；Hub随页面wake到Secure/wanted1，
  离页scope/links/sessions均0，最终Hub Stopped/wanted0/managementSubscribed1。
  末尾heapFree104984、heapMin52460、largest47092 B，loop3652/ipc01104/NimBLE2720 B；这仍是短测，不宣称资源长稳。
- 本地证据：`build/scan-s1-app-final.log`、`scan-s1-apk.log`、`scan-s1-hardware-regression.log`、
  `scan-s1-direct-timing.jsonl`、`scan-s1-direct-recovery.jsonl`、`scan-s1-native-resource.jsonl`、
  `scan-s1-native-background.jsonl`、`scan-s1-cloud-state.jsonl`、`scan-s1-final-idle.log`。

#### 10.85.3 后续，不重复建设扫描或协议核心

1. 补实际系统更严格限额/扫描失败、关闭失败与进程恢复的负测，WiFiProv另验按名称/QR入口取消及完整配网；
   当前已测普通startBleScan端口互斥，不能扩大为完整配网协议已重新验收。
2. 成功连接基线与过频负测分开统计；按原生产调用预算和真实可用配额收集10次成功测量，所有拒绝/冷却时间另列。
   不能仅添加测试sleep后声称用户体验已改善，更不能把过频负测期望改成无限重启scanner。
3. 实测真实Hub关闭完成→child恢复广播，补手机无云/在线Hub持续owner的离线公平合同与唯一物理策略。
   系统扫描配额不是Hub已让位、也不是授予离线权限；不恢复cloudReachable=false旁路，不因测量工具能直连就开放正常页面抢连。
4. 成套bit18激活及同绑定四端10→30→100、管理/permit争用/撤权/失联门禁仍在后续。
   D4b、N2～N5、Wio/Hub发布长稳继续独立记录，不扩散Mesh、History/Bulk或新存储协议。

### 10.86 WiFiProv查找取消、重启扫描额度与真机修正（2026-09-07）

本批继续6.23，不重新设计scanner/协议/owner。先验证名称及QR取消，再根据真实系统失败修正资源账本。

#### 10.86.1 根因与最小实现

- 原进程内5/30s账本在App重启时归零，但系统UID扫描额度未必归零。实际PGEM10重启测试：
  `BluetoothLeScanner`打印 `onScannerRegistered() - status=6`、`registration failed because app is scanning too frequently`；
  JS start已resolve，2s观察0广告、无失败事件。**此手机路径没有回传onScanFailed，不能保证只靠回调把所有空扫描识别为限额。**
  之后不改设备/绑定的独立Direct调用Ready2893ms。不是NimBLE认证故障，也不能把此观察推广到所有Android版本。
- `ScanBudget`保留唯一实际start尝试计数，新增可注入的很小Store；`AndroidScanBudgetStore`只持久化
  boot计数、单调保存时间、最多5个start时间及冷却期限。与账号、DeviceId/地址、凭据、MQTT和UI解耦。
  每次真实start前同步commit，失败明确`BLE_SCAN_BUDGET_STORAGE_FAILED`且不调SDK；取消/后台/登录切换/进程重启不退款。
  已知不同boot不沿用旧单调时间；损坏或无法解释的历史保守等待最多30s，不以损坏历史换取立即免费额度。
  仍不是厂商真实额度查询，第三方进程/旧版本遗留或更严格厂商静默拒绝不能全部提前知道；不新增空结果自动重扫。
- WiFiProv候选缓存以同步map+快照迭代，≤64条检查和写入原子化；SDK/回调在缓存锁外执行，删除contains/get两步选择竞争。
  不新增另一套扫描器、GATT、安全协议或入网凭据。独立插件SDK后端仍在，需另验其完整生命周期，不能借宿主测试宣称其通过。
- 新工具 `ble_scan_lookup_gate` 只用随机不存在的配网名，测名称/QR取消、同入口重叠Busy、Direct交接与10s查找截止；
  `ble_scan_restart_gate` 分prepare/after-restart/recover，不内置kill、不自动等待冷却重试，不输出发现身份或秘密。
  只有operator确认debug包后才能force-stop重开，禁止`pm clear`。测试软件模型不是SDK真实故障注入。

#### 10.86.2 已完成证据及边界

- Angular33文件 **272/272**；新增原生事件桥6项覆盖resolve后的失败、冷却透传、旧id、迟到start/stop、关闭失败隔离和坏广告。
  Android账本 **7/7**，包含重启、厂商冷却、过期/不同boot、损坏历史、commit失败；Gradle assemble通过。
  测试最初在终止态单例之后继续复用导致1项失败，已修正测试生命周期并完整重跑；未重置生产私有状态来跳过隔离。
- WiFiProv查找旧/修正缓存APK均实测：名称stop取消、QR clearState取消、重叠Busy、没有迟到selection、Direct可接手；
  自然无匹配截止10034/10041ms。一次启动后复测最后阶段限额，原始失败记录保留，工具现报告THROTTLED而不是泛化互斥失败。
  最终持久账本APK在原窗口结束后的完整同门禁也通过，截止10033ms（`scan-s2-lookup-persist-passed.jsonl`）。
  此项没有选择/连接真实WiFiProv外围、没有发送SSID/Key，也不是完整配网重验。
- 最终账本APK SHA-256 `76e60ea7e33056cfb68ba4489a2fa1fed345c85c6be878b01f82933dfc55c979`。
  真机5次均收到广告，第6次未提交SDK、THROTTLED剩29292ms；立即force-stop重开后仍未提交SDK、剩27432ms，关闭确认通过。
  没有清数据/切蓝牙/擦NVS/重置MCU/换Key/重新绑定。Service/Broker镜像与两板固件不变。
- 最终APK冷却后的独立恢复确实收到广告；随后 **3+3+4三个明确批次、10/10隔离Direct样本** 均通过，
  Ready 2771～3744ms，中位3598ms，每次关闭后links/sessions=0，无Action。
  批次完成时刻为23:12:48、23:13:43、23:15:19；间隔内进行软件复核/文档工作，不在工具中sleep或重试。
  **不是同一个连续count10门禁，也不是生产正常页受管child公平交替已通过；扫描拒绝样本另列，不承诺任意频率重新连接。**
- 普通Cloud页恢复 `ready/manifest/fresh/canControl=true`、direct=false、buttonPresses仍1；离页scope/links/sessions=0。
  Hub伴随记录Stopped→连接→Secure/wanted1→Closing→Stopped/wanted0，managementSubscribed=1；没有发送控制Action。
  末尾heapFree103348、min52460、largest47092 B；loop3652/ipc01104/NimBLE2720 B，短测不能替代长稳。
  离页后立即再跑lookup有一次前置Busy（列表附近扫描仍活跃），随后四个交接start成功但截止用的第五start限额；
  这些诊断失败保留，不切掉生产Presence或清账本来制造成功。
- 硬件工具最终 **93/93**、App类型检查和架构门禁通过。CI由陈旧的5个固定工具文件改为所有`*.test.mjs/cjs`自测，避免漏掉新工具；不运行真实硬件操作。
  原生真实SDKstop抛错未注入；关闭失败隔离仍仅软件负测。
- 本地证据前缀 `build/scan-s2-`：lookup-before/final/recovery、restart-prepare/probe、direct-restarted、
  persist-prepare/restarted、app-final、persist-apk；限额失败与后续正常恢复分开记录，不能删失败样本或声称连续10次无拒绝。

#### 10.86.3 下一步

1. 正常Direct十样本及原绑定Cloud唤醒/释放已完成；后续优先进入第3项物理测量，不重复重跑此批扫描核心。
2. 名称/QR扫描取消已过，不再重复建设；完整WiFiProv选择→Security1→配网及实际SDK关闭失败恢复仍独立待验。
   只用两块已绑定Hub/child时不通过重置它们临时制造配网夹具。
3. 后续优先测Hub **SDK真实关闭完成→child恢复广告**，不是只看logical Stopped；再冻结手机无云/Hub持续需求的离线公平合同。
   bit18继续关闭，受管child仍需真实Ready；最后才成套四端Direct→Gateway→Direct 10→30→100。
4. D4b生产加密板、N2～N5、Wio/Hub发布长稳独立待办。没有暂存、提交或推送。

### 10.87 真实SDK断链→授权广播测量及公平实施边界（2026-09-07）

本批消费10.86.3第3项，完成内部测量工具、三轮有效物理样本、正常固件恢复和离线公平执行合同。
**没有实现本地公平调度/开放bit18，也没有完成正常页Direct→Gateway→Direct四端循环。**

#### 10.87.1 探针保持在测试层

- `tests/internal/arduino/Esp32EdgeHubDualBoard/NativeRadioProbe.h`仅在`BLINKER_TEST_NATIVE_RADIO`下编译。
  复用官方`ble_gap_event_listener_register`；固定4事件队列，NimBLE任务内不打印、不改radio策略，
  不替换应用GAP callback、不依赖SDK私有符号、不修改SDK或去初始化Host。
- 成功connect/disconnect记录同一SDK handle的内部sequence；loop分别查询`ble_gap_conn_find`、
  `ble_gap_disc_active`和`ble_gap_conn_active`，全部空闲才记录idle。采样延迟/成本、丢队列或重叠均可见。
  `t`串口命令只读micros，不能触发reset/清Access。正常Sketch不包含监听器/时钟命令。
- `tests/hardware/edge_gateway_radio_recovery_gate.mjs`先做原绑定Direct认证基线，断开后进入普通Cloud页，
  先收到真实native扫描结果证明观察已开始，再退出页等待授权广告；不发送Action、不开第二条GATT。
  手机和MCU使用前后请求/回包时间界交集校准，不直接相减不同设备的时钟。
- 首版误把一次已认证的完整13 B广播当固定身份：定位值实际是`nonce32 + tag32`，正常轮换后不再相等。
  已删除该匹配与`discoverDirect`临时替换，复用App生产`presence.ts`/`wire.ts`纯函数在内存编译做验证。
  凭据只在WebView内读取/清零，不从手机导出；不添加App调试API、不复制HMAC/协议实现、不按名称/MAC匹配。
  原生事件到达时间与异步HMAC完成时间分开；解析/匹配容量/账号异常均有界失败并等待清理。

#### 10.87.2 真实结果及限制

现场仍为两块XIAO ESP32S3：Hub COM11、child COM12与原PGEM10手机，网络“带翅膀的蜗牛”。
只在Hub的`0x10000`应用分区烧录诊断版本，child固件、原Key/NVS/Access/绑定不变；没有重新认领或Action。
App沿用10.86最终APK，Service/Broker镜像不变，Hub bit18仍关闭。

| 有效样本/本地文件 | 页面退出→授权广播 | 页面退出→Hub GAP事件（区间） | Hub GAP事件→授权广播（区间） | GAP→SDK全空闲确认 |
| --- | --- | --- | --- | --- |
| `native-radio-round3.jsonl` | 1156.1 ms | 1049.7～1071.0 ms | 85.1～106.4 ms | 1.823 ms |
| `native-radio-round4.jsonl` | 1055.5 ms | 980.6～998.5 ms | 57.0～74.9 ms | 8.734 ms |
| `native-radio-round5.jsonl` | 1024.4 ms | 964.6～987.9 ms | 36.5～59.8 ms | 2.607 ms |

- SDK查询单次成本27/66/44 μs，最近busy到idle采样跨度2.857/9.132/4.332 ms；事件队列dropped均0，无重叠。
  手机校准区间宽16.8～21.1 ms、MCU1.2～2.8 ms，已体现在表中范围。广告时间包括Android扫描和回调交付，
  不是空口首广告时刻；三个样本不能推导发布最坏/P99保证。
- `round1/round2`失败记录保留，不计入成功样本：round1只有不透明App失败，不能补写不存在的阶段结论；
  round2明确卡在advertisement/`GATE_WAIT_DEADLINE`。两轮SDK均已断链并空闲，但错误固定nonce匹配无法验收恢复。
  修正后3/3只代表上述隔离测量；不是把总5次工具尝试都写成成功，也不是四端10次交替。
- 三个有效样本buttonPresses均1，离页links/sessions/scopes=0；没有未知Action重放。
- 诊断固件SHA-256 `5d4cfd0e22a32575556f83afb8a1d262d0f7186809cfcb6839d6352bc39f81b0`；
  Arduino ESP32 3.3.11编译Flash1,447,644 B、静态RAM65,676 B，较正常增加1028/64 B，只属于内部测试。
- 已应用分区恢复正常固件SHA-256 `4d6c522ed56ff77326d53e007b1f3f7501270b4f3d5524af7ee88b9975665055`，写入校验通过；
  Flash1,446,616 B、静态RAM65,612 B。再次打开原Cloud页fresh/canControl=true、direct=false、counter仍1，随后离页。
  不包含诊断监听器的固件是最终现场状态，不把测试开销留给普通用户。
  末尾childState=Stopped/secure0/wanted0/managementSubscribed1，App links/sessions/scopes=0；
  heapFree105088、min95024、largest63476 B，loop3656/ipc01104/NimBLE2792 B。重启后的短测水位不替代长稳。
- 工具新增7项（旋转nonce、时钟区间、异常清理、占用隔离、持有时不应广播等），完整硬件工具 **100/100**，
  架构检查和实际SDK编译通过。App/Service生产代码本批未修改，不声称重新跑过全部App/服务器回归。
- 原始证据均在本地`build/native-radio-*`；不包含密钥/广告定位值，不进入发布ZIP。

#### 10.87.3 下一批不再重复物理测量

测得释放后广播恢复较快，但**在线Hub持续Cloud需求+无云手机**的主动让出仍缺代码；
已经释放得快不等于会主动释放。按需求合同 **6.25 F1→F2→F3→F4** 继续：

1. 在唯一radio实现普通占用上限→原管理排空→真实关闭→完整空档，renew/重试不续占用，旧promise不提前取消。
2. 原App前台scope实现有限离线机会及真实关闭；不把ACL拒绝/pending或旧nearby变成旁路，也不给手机无限占用。
3. 工程候选10 s占用/6 s空档实测持续owner与手机无云，记录Cloud恢复成本后再定生产参数，不是本批默认值。
4. 两端边界通过后配套bit18，执行正常页同绑定10→30→100；不扩散Mesh、多child、History/Bulk。

完整WiFiProv实际配网、SDK stop异常真机恢复、D4b加密板/N2～N5及Wio/Hub发布长稳仍独立待验。
没有暂存、提交或推送。

### 10.88 F1本地有界radio、持续需求真机子门禁与F2a关闭失败修正（2026-09-08）

本批按需求合同6.25推进F1，检查F2时发现SDK关闭异常被吞，先修正此前错误事实边界。
**F1机制和持续Cloud需求子门禁已完成；F2离线入口/F3离线手机/F4四端交替未完成，bit18继续关闭。**

#### 10.88.1 设备实现和软件门禁

- 唯一`GatewayGattRadio.h`消费10 s工程占用上限/6 s机会空档；只加单调时间状态，不增任务、堆、队列、wire、NVS或Sketch参数。
- 普通尝试开始计时，认证失败/短暂重连/renew不退款。新管理准入在MQTT回调内也实时检查；
  原exact management/permit按原预算排空，不能拆事务或让新操作持续抢占。只有逻辑和native都Stopped才计算完整机会。
- natural idle/旧Direct promise可以提供相同机会，不重复累计6 s；clock wrap、stop、断云、Fault、慢关和zero不能绕过边界。
  未知Action仍走原错误/ledger语义，不新增自动补投；本轮没有进行真实Action贴近切换边界的测试。
- Host ASan **50/50**，含100轮原Direct让出+100轮本地公平模拟、管理/异步屏障负测、20万fuzz；硬件工具100/100、架构通过。
  第一遍Host进程因PATH缺少ASan DLL以0xc0000135失败；仅在命令环境补现有MSVC运行库路径后重跑全部通过，未修改/放松测试。
- Arduino ESP32 3.3.11实际编译Flash **1,446,932 B**、静态RAM **65,620 B**，相较10.87正常固件 **+316/+8 B**。
  F1工程固件SHA-256 `a011d6a36abce5fbfb76e5202aaae7616583e1b0415482e280082e7da0df5871`。

#### 10.88.2 原绑定真机持续Cloud需求

Hub COM11 MAC经esptool核对，child COM12与原PGEM10；只烧Hub应用0x10000并校验，保留NVS/Key/Access/绑定。
App保持10.86原APK，服务器/数据库/镜像未变。60 s普通Cloud页观察，另80 s串口记录，未发送Action。

- 页面稳定后同一个scope（generation始终1）、scopes=1，Hub在每次让出及恢复期间executionWanted一直1。
  三次完整恢复和第四次主动关闭已观察到；末尾由真实离页才归零，不以清owner或重建页面制造公平。
- 串口交付时刻：Connecting 10739→Closing20733→Stopped20778→Connecting26771 ms；
  下一轮36773/36798→42797；再下一轮52798/52814→58823。接近10 s占用和6 s空档，
  **串口是宿主接收采样，不将5993 ms等交付差当作MCU精确时钟或独立SDK监听测量**。
- Cloud fresh/canControl恢复到false再true三次，间隔 **8291/8497/8496 ms**；counter始终1。
  10/6候选下每周期Cloud有约一半时间不可控，机制有效不等于体验可接受，参数没有冻结为发布默认。
- 测试末尾heapFree105112/min94948/largest63476 B；loop3656/ipc01112/NimBLE2704 B；仅短测，不代表长稳。
- 结束已应用分区恢复10.87正常SHA `4d6c522ed56ff77326d53e007b1f3f7501270b4f3d5524af7ee88b9975665055`并校验；
  原Cloud页重新fresh/canControl、counter1，随后离页。**F1源码已实现，最终现场仍为不含F1工程周期的原固件。**
  不发布候选参数，不恢复旧源码分支来形成两套实现；后续配套F2/F3验证同一实现。

#### 10.88.3 App F2a：关闭异常不得伪装成释放

沿实际SDK源码确认Android disconnect成功路径来自真实断链处理（或原Gatt为空），超时会reject。
原代码在record link→secure channel→session→service吞掉失败，可能提前给继任者释放资源；
此外service取消opening后原来没有等晚到连接真正清理。已删除这些成功化路径：

- native close失败使用无秘密错误码并隔离本进程Direct新link；不允许新对象/账号清标记。
- 原业务错误不被清理错误覆盖，但close调用者仍能看到失败；Session只在成功清理后通知订阅者。
- service失败closing继续阻挡继任者，重复disconnect仍返回失败；取消opening等原Promise及晚到清理，不把native失败覆盖为cancel。
- `onClosed`明确仅负责立即逻辑退役，不能用于计算给Hub的机会空档。没有伪造新的SDK回执或扩散UI/API。
- App **30 文件 239/239**、类型检查、local 前端构建及 Android debug assemble 通过。
- 新 APK SHA-256 `a17eaa8f780afd3c7bbfca8d99c2e9e5277a00707e2ddb103ef7456a693f5479` 已覆盖安装；
  WebView 仍为 `http://localhost`、authority 仍为 `http://127.0.0.1:7000`，仅恢复 USB 重连丢失的原端口映射，账号/凭据命名空间不变。
- Hub 真实 Stopped/executionWanted=0 前提下，原测试工具的三轮独立 Direct 基线均通过：
  Ready **2820/3789/3598 ms**，record link close Promise 首次调用到完成 **133/120/130 ms**，每轮 links/sessions=0。
  工具记录的重复 close 调用共享幂等 Promise，不是两条 SDK 断链；该时长包含 stopNotifications/SDK/桥接，不能称为空口精确断开时长。
- 之后真实进入原 Cloud 页，fresh/canControl=true、direct=false、counter=1；离页后零 scope/link/session/opening/closing，
  Hub Stopped/executionWanted=0/managementSubscribed=1。没有 Action，也没有重置、清凭据或重新认领。
- **只证明新 APK 的正常关闭/恢复，不是在线 Ready 准入、F2b 离线入口或公平交替通过。**
  晚到 connect/关闭拒绝已过软件负测，但本批未向真实 SDK 注入这些故障，异常后的物理恢复仍独立待验。

#### 10.88.4 下一批与原始证据

1. 按需求合同6.26，正常关闭边界已真机确认；继续 SDK 异常/晚到清理的恢复证据，再做原scope有限离线admission。
   不使用account retrying/cloudReachable=false充当手机无网证据，拒绝/pending/配置错误不能旁路。
2. F2b必须让手机也有界释放；F3同时测离线等待和Cloud不可控期，不能仅放大Hub占用上限掩盖8.5 s恢复成本。
3. 之后bit18/正常页面10→30→100；完整WiFiProv、D4b、N2～N5、Wio/Hub长稳仍独立待验。

本地证据`build/fairness-f1-{host-build,host-tests-runtime,compile,mac,flash,continuous-app,continuous-serial,restore,restored-cloud}*`、
`build/fairness-f2a-*`；错误Host环境记录保留。无重置子设备、清Access、换Key、Action、暂存、提交或推送。

### 10.89 F2a 真实关闭负测/恢复与 F2b 有限离线机会接线（2026-09-08）

本批沿需求合同6.26执行，完整合同与下一步见新增6.27。**F2a本组合短门禁收口；F2b软件接线完成、真实离线冷启动页面未通过；F3/F4未通过。**

#### 10.89.1 F2a 真机负测，不注入假的SDK结果

使用原APK `a17eaa8f780afd3c7bbfca8d99c2e9e5277a00707e2ddb103ef7456a693f5479`、原两XIAO/账号/绑定，
先由COM11证实Hub Stopped/wanted0。新增独立测试工具`edge_gateway_direct_release_gate.mjs`，不在生产代码增加测试开关。

- `late-connect`：先等待真实SDK connect成功，再在交付原调用前取消；时间932→933 ms，原opening到1002 ms才结束。
  真实SDK两次幂等清理995/1001 ms完成，没有Ready；Android原生getConnectedDevices确认目标已断开，零link/session/closing。
  不声称这是自然发生的SDK竞态；它是对真实连接的受控桥接交付负测。
- `close-timeout`：一次真正disconnect指定1 ms预算，SDK实际超时拒绝。App保留失败屏障，重复关闭/继任连接失败，
  没有第二次SDK connect；即便Android查询已无目标连接，也不清除隔离。保留1 link/1 session/closing符合失败边界。
- 真正force-stop/restart App后（不clear data），三个Direct样本Ready **2910/3796/3653 ms**、正常关闭 **121/124/123 ms**，每轮零link/session。
  随后真实Cloud页fresh/canControl、counter仍1，离页零scope。没有Action、板端reset/清Access或新绑定。
- 工具6项新回归覆盖真实调用转发、非超时不能假通过、清理/恢复hook、preflight重验及错误脱敏；硬件工具全量 **106/106**。
  证据`build/fairness-f2a-{late-20260908,timeout-20260908,restart-three-20260908,fault-cloud-recovery}*`。

#### 10.89.2 F2b 生产代码接线与清理

- 原`NetworkService`空Cordova监听已替换为已有Capacitor原生网络观测，删除无用Data/Notice/Platform依赖、旧timer/注释代码。
  只接受明确无网络；query/event竞态、SDK失败和迟到回调保持保守。不是用MQTT retrying或cloudReachable=false授权。
- `BleOfflineOpportunity`仅维护一个本地budget和真实清理屏障，不持有logicalId/secret/owner表；复用原caller group/admission。
  工程候选16 s获取、首次native获取起10 s占用、实际释放后6 s机会；重复获取、页面/账号/网络恢复不能退款。
  新进程离线入口先留6 s；原扫描配额不变。收发同步检查截止，native失败保持busy，取消等晚到opening。
- Record link只新增内部`onAcquire`通知；service转发并支持exact admission清理，旧cleanup不得断开同id的后继连接。
  页面selector在申请Broker owner前选择离线分支；网络证据丢失即退休，已在线pending/denied不可转离线。
  不改页面组件/UI、Arduino API、服务端协议或Action重放规则。
- App **33文件262/262**，覆盖离线多caller、到期、启动空档、慢关、失败、晚到、换账号/后台、网络恢复及旧清理隔离；
  TypeScript/local前端/Android debug构建及架构通过。设备/Service生产源码本批不改，无需重烧MCU或部署镜像。

#### 10.89.3 部署边界与接续步骤

F2b APK SHA-256 `afb6275b4c1783f3b3b8c5add9575e03fcd3e1ceaf9022d32a4e1256495226ce`已生成。
最初备份前ADB掉线，空失败备份已移除。用户接回后从手机重新备份并核对F2a SHA，再试装候选。

- 真实关闭手机WiFi/移动数据，撤除API/MQTT reverse并force-stop/restart；原生offline=true，account=idle，auth保留。
- 30 s观察没有SDK connect/close、没有Ready；失败`GATE_OFFLINE_NOT_COMPLETED`，不是BLE认证/占用超时失败。
- 根因：冷启动`DataService.device`为空，`UserService.getAllInfo`需要在线当前用户和设备列表。
  持久Controller凭据仍可读，但普通设备页拿不到设备目录而没有挂载；不能用测试工具凭空生成设备类型/身份绕过。
- 已保留原始失败`build/fairness-f2b-offline-page-20260908.jsonl`；测试finally恢复WiFi/移动数据（均1）及原两条reverse。
  **最终已覆盖恢复F2a已验证APK**，账号/凭据/原两板不变；原普通Cloud页fresh/canControl/counter1、离页零scope。
  证据`build/fairness-f2b-baseline-restored-cloud.jsonl`。Hub仍10.87正常固件，不含F1工程周期。

1. **先按需求合同6.27 F2b-C补最小非秘密设备目录缓存及ManifestCache的authority隔离**；目录不能当权限或在线证据。
   凭据不能决定产品类型，缺失cloudEnabled不能默认为普通BLE。覆盖冷启动、账号/authority隔离、撤销/删除、损坏与迟到回填负测。
2. 再过真实普通页面有限离线Direct：原生无网+撤除测试USB云通路，验证页面挂载、真实到期关闭；finally恢复原网络与reverse。
3. 再配套F1做手机离线/Hub持续Cloud owner的F3；候选10/6已有约8.5 s Cloud不可控成本，必须同时评估，不能直接冻结发布。
4. F3通过后bit18/正常页10→30→100；D4b/N2～N5/完整WiFiProv/发布长稳仍独立待验。

本地软件证据`build/fairness-f2b-{targeted,regression,final-types,build,cap-copy,gradle}*`、`build/fairness-f2a-tools-20260908.log`。
本批无暂存、提交或推送，未将软件门禁写成四端完成。

### 10.90 F2b-C最小目录与Manifest隔离、普通页无网冷启动短测（2026-09-08）

已按需求合同6.27完成目录缺口；正式行为/下一步见6.28。本批不扩展UI布局、协议、MCU功能、服务器owner或凭据结构。

1. App新增独立`device-catalog-cache.ts`投影模块；DataService只接存取及统一模型转换。
   authority+auth.uuid、schema1、256项/128 KiB UTF-8；显式类型/cloudEnabled，白名单、不缓存Ready/State或秘密。
2. 完整列表替换，撤销/删除清目录并退休scope；部分列表撤下旧缓存。退出/换账号同步清视图，
   SecureStorage异步读取/当前用户uuid写回和目录请求代次受fence保护。Manifest旧无authority缓存不复用。
3. 补缓存隔离/损坏/越界/重复ID/缺字段、共享无凭据、撤销、旧账号及同账号迟到列表、BLE迟到snapshot/event回归。
   清理DataService未使用的布局字符串转换函数；不引入第二套页面/连接模型。
4. 第一次真机离线Ready2990 ms但立即OS列表观察未清空，严格保留失败，不把SDK成功当最终通过。
   工具改为最大4 s查询OS列表后，普通离线页Ready3318 ms、到期SDK关闭53 ms；
   SDK close完成后1084 ms观察到OS目标消失，再观察6039 ms空档，零links/sessions/opening/closing/scopes。
   不发Action，counter1不变；网络/USB转发恢复后普通Cloud页fresh/canControl/counter1，离页零scope。
5. 软件App42文件357/357、硬件工具108/108、类型/local/Android构建/架构通过。
   初始验证APK SHA-256 `193fb8efcb98d74007d7c291b32caa98d4607b1c7e05b048e02a45084a64ed18`。
   日志：`build/fairness-f2bc-{final-regression,types,build,gradle,tools,architecture}*`；
   真机`build/fairness-f2bc-{online-prime,offline-page,offline-page-bounded,cloud-recovery}.jsonl`。

最终删除死代码/统一换行后重建并覆盖安装APK（仍保留账号/目录/凭据），SHA-256
`6f146fe27fa243f1efeb4259070afe00d6dd5ee9f5ff4dd9ecaeb4babd679985`。
再进行一次真实离线冷启动：Ready3025 ms，SDK connect393→1089 ms；到期关闭10481→10536 ms（55 ms），
页面退休10604 ms、Android确认目标消失11674 ms，再完整空档6049 ms；counter1、零残留。
原网络和API/MQTT reverse恢复后Cloud页fresh/canControl/counter1，离页零scope。
最终证据`build/fairness-f2bc-final-{offline-page,cloud-recovery}.jsonl`及`build/fairness-f2bc-final-hub.log`；
最终构建`build/fairness-f2bc-final-{build,copy,gradle}.log`。没有清App数据或回滚掉新缓存实现。

**当前主线推进到F3入口，不宣称F3通过。**现场Hub仍10.87正常固件/无持续需求，child与NVS/Key/绑定不变。
下一批按需求合同6.28：部署既有F1候选，仅启动一个独立真实Cloud caller持续需求，再与手机真实无网普通页配对，
报告双方等待、Cloud不可控期及SDK释放；失败停在单周期，不做10→30→100掩盖失败。
无新Action/重置/重绑/设备或服务器业务源码修改，无暂存、提交或推送。

### 10.91 F3a首次独立Cloud/离线手机四端配对及预算根因（2026-09-08）

已执行需求合同6.28配对；具体流程、指标和下一批冻结边界见6.29。

- 用生产App账户SDK在电脑进程内建立新的用户MQTT session，仅一个真实execution owner；原renew/readiness实现复用。
  账户短期MQTT credential只经当前App provider→测试进程内存，不保存/打印，不用设备登录信息，不注入Broker owner。
- 手机真正无网冷启动、普通页、当前凭据/Presence/Method2；Hub为已核对app0分区的F1 a011d6a3候选，child/NVS/绑定保留。
- 初次观测工具只处理第一轮恢复，错误地把第二次正常周期让出判为未恢复；已删除该一次性恢复逻辑，改为按真实loss复用生产readiness wait。
  初次失败仍保留，未改生产设备算法掩盖测试错误。
- 修正后手机native等待7164 ms、页面Ready9948 ms、SDK关闭50 ms、OS确认消失后6060 ms空档，零残留。
  Cloud不可控13759 ms后恢复，仅2270 ms可用便再次让出；第二轮8400 ms后再次fresh。
  一个owner从acquire至最终cleanup保持，wanted=1、MQTT ready、ipc0低水位1112 B，counter1无变化/无Action。
- 根因在唯一radio的获取前起计时：逻辑Connecting包括等待手机释放/扫描/发现，吃掉大部分10 s。
  下一步F3b先审查Port阶段证据，拆分有界获取/连接占用及真实空档，保留总上限和不退款/管理/boot/wrap边界。
  **不是直接把10改成大常数，不新增协议或owner表；不是NimBLE版本/IPC栈问题。**
- 工具112项及架构检查通过。源码新增限于测试观察器/runner及测试；App/Service业务源码和设备生产算法本批未改。
  证据`build/fairness-f3-{partition,flash,boot,paired,paired-corrected,tools}*`，可复现配置`build/fairness-f3.config.json`不含秘密。

测试已执行手机网络/reverse恢复及电脑caller释放关闭；USB断开后用户已重新接回。
再次read-mac确认COM11为原Hub，仅写0x10000应用区恢复§10.87正常固件，写后hash校验成功；
SHA256为`4d6c522ed56ff77326d53e007b1f3f7501270b4f3d5524af7ee88b9975665055`，证据`build/fairness-f3-restore.log`。
手机原两网络开关均为1，重建7000→17001及8884→8884后，首页账户ready、active=true，
scopes/links/sessions=0、opening/closing=false。这不是新网络下child Cloud可控通过。
**新现场网络为mostfun2026，电脑IP192.168.1.141；基线仍含旧WiFi配置，Hub wifiStatus=6/IP0.0.0.0、managementSubscribed=0。**
下一批先同步被忽略的本机WiFi/登录地址、Gateway public host及由原测试CA签发的新IP叶证书，再编译部署F3b候选；
保持CA/IP校验、原DeviceKey/Access/绑定，不为恢复联网关闭TLS，不把手机账户ready当作Hub已上线。
无child重置/重绑/NVS修改、无Action/暂存/提交/推送。F3b未完成前不开放bit18、不进入10→30→100。

### 10.92 F3b分阶段预算、完整可用窗口及网络恢复（2026-09-08）

- `GatewayGattRadio`内替换原同起点10 s预算：获取16 s、原生Connecting后占用10 s、绝对总上限26 s、真正释放6 s。
  在接受晚到连接前检查到期；attempt重试、短空档、MQTT重连不退款，原管理continuation/boot/wrap/关闭屏障保留。
  已审查Port事件顺序：成功启动连接可观察，同步拒绝无ACL且仍受获取上限，无需新增SDK/Port/user/wire接口。
- Host/ASan50项、工具112项、架构及3.3.11编译通过；Flash1,447,044 B、静态RAM65,628 B（对F1 +112/+8 B）。
  固件SHA256 `8624a6c2fa6ed245c9bc15d6dac9822997386176c2a9b788b1136bad2eb5200e`；只写已验证COM11的0x10000应用区。
- 新网络mostfun2026、电脑192.168.1.141、Hub192.168.1.144；更新忽略的本机配置/网络记录及Gateway public host，
  原CA签发新IP叶证书，Node CA/hostname验证及HTTPS healthz200通过。未关闭设备TLS，未清Access或轮换DeviceKey。
  旧测试CA缺KeyUsage，不符合Python3.13额外X509_STRICT且将于9月11日到期；这不是生产PKI，新叶证书到9月10日。
  原TLS代理保留为`blinker-device-v2-tls-proxy-before-f3b`。App/Service业务代码及APK不变。
- 首次预检actual counter0≠历史expected1而停止，未进入手机页面；子设备测试计数为开机归零的RAM变量。
  根据真实fresh读数明确新基线0，保留失败日志，再执行同绑定独立owner/离线手机配对。
- **通过结果**：手机Ready9988 ms；SDK关闭58 ms，OS确认释放后空档6048 ms，单connect/单close及零残留。
  Cloud不可控13837 ms后恢复，**完整可用窗口7465 ms（原2270 ms）**；再次让出后8710 ms恢复，持续owner/wanted1与counter0保持。
  工具增加完整“恢复→让出→再次恢复”观测，避免只采恢复瞬间。ipc0低水位1104 B；没有Action/child重刷/清数据/重绑。
- 恢复手机两网络开关及reverse后，真实普通Cloud页fresh/可控/counter0，返回首页scope0；Hub idle wanted0且MQTT管理订阅正常。
  **现场保留上述F3b工程候选，bit18关闭；不是回到含旧网络的10.87二进制，不宣称发布长稳。**
- 证据`build/fairness-f3b-*`，完整合同/时序见需求合同6.30。F3b短门禁完成；16/10/6保留为F4工程对照基线。
  下一批核对并配套启用bit18，先在线普通页一次Direct→Gateway→Direct及取消/管理占用短测，再按10→30→100推进。
  初次Cloud仍约13.8 s不可控，未消除单连接交替成本；D4b/N2～N5/完整WiFiProv/Wio/Hub发布长稳独立待验。

### 10.93 F4真实在线Direct接线、发送校验与取消门禁（2026-09-08）

本批原位推进F4，不扩展调度器、SDK、Key或用户API。完整合同见需求合同6.31。

- 工程Hub外层Hello显式bit18，Broker仅对authenticated edge-hub协商；普通设备/虚拟child默认不声明。
  Gateway同步既有`purpose=direct`/confirmed Admin epoch实现；旧运行镜像缺epoch时预约被正确拒绝。
- Broker补“execution先于Hello”的晚准入初始化，不复活fenced authority。独立回归证明修复前失败。
- 真机进一步证明Probe已下发，但设备`GatewayRouteClient`上行校验漏0x8c；补Challenge/Yielded白名单，
  继续拒绝Probe/Decision、截断/尾随数据，使用真实RouteClient→CloudMux→MQTT回归，不再仅依赖Bridge假Route。
- Host/ASan50、工具115、Broker针对性95+完整V2、Gateway隔离PG授权/生产接线、架构检查通过。
  App正式Angular入口38文件374测试通过；不修改页面样式或新增UI连接策略。
- Hub Flash1,447,100 B、静态RAM65,628 B（对F3b +56/+0 B）；3.3.11，固件SHA256
  `78c8e6ebd298b77570a2f979025eafc54bc109fcbeff9d1904ac0b055b14c863`。
  COM11只写0x10000应用区且hash通过。F3b已ROM完整备份并校验8624a6c2；未改child、分区、NVS、Access或Key。
- **已通过的物理子项**：同一Cloud owner不退订，两次普通在线页Ready4017/4085 ms，
  physical Ready1168/1159 ms均先于SDK connect1413/1413 ms；SDK关闭29/46 ms，OS确认后零残留，counter0不变。
  Cloud分别在21066/37353 ms恢复，完整不可控区间15336/15936 ms；这是单连接交替成本，不宣称低延迟或无中断。
- **本轮发现的取消缺口**：第三次144 ms请求离页后，旧App仍在1492 ms启动native连接，随后关闭。
  Gate正确失败并保留`fairness-f4-online-final.jsonl`；正向两轮通过不能覆盖取消负测。
  `viewActive`改为同步Input setter，直接复用原scope取消/telemetry回收，删除依赖下一次ngOnChanges的旧触发逻辑。
  新单元用例先证实旧实现不能立即abort，再验证修正。该修正单独仍未通过真机：
  `cancel-scope`证实NavigationStart161 ms、父/子页163 ms均已隐藏，却仍存在外壳发起的独立one-shot。
  **进一步删除标准V2页外壳的重复connect，子页成为唯一页面scope；复用组件解析，保留自定义页入口。**
  原位安装最终APK SHA256 `81ee08092aea7f9764d928d042536fc09346d50e1dcc9d92fb6c3a6ceeea14c3`，
  不卸载/清应用数据；最终取消复测结果在下方记录。
- 证据`build/fairness-f4-*`：保留首次缺epoch、随后无Ready、wire五次Probe无上行以及完整四步交换的不同失败/成功记录。
  本机Gateway `l2c-direct-f4`、Shard1 `l2c-direct-f4-ready`；不代表公网部署或生产加密NVS准入。

**最终原位APK四端短门禁通过（`fairness-f4-online-scope-final.jsonl`）：**

| 项目 | 最终冻结组合实测 |
| --- | --- |
| 两轮普通在线页fresh/可控 | 4425 / 4126 ms，counter均0 |
| physical Ready → SDK connect | 1315→1745 / 1214→1479 ms，没有提前native acquire |
| SDK关闭、OS释放 | 36 / 43 ms；两轮均单connect/单close，OS确认消失、页面idle |
| 同一个Cloud owner恢复 | 20350 / 36362 ms，分别不可控15743 / 15574 ms；未另建owner或发送Action |
| Ready前取消 | 请求153 ms→NavigationStart154 ms→Ready拒绝157 ms→scope回收216 ms；native connect/close均0 |
| 取消后的Cloud恢复 | 39846 ms；本次不可控3007 ms，counter0；整体gate39847 ms通过 |
| Hub空闲与资源 | Stopped/wanted0/managementSubscribed1；ipc0最小1104 B、loop3656 B、NimBLE2708 B |

最终wire记录`fairness-f4-direct-wire-scope-final.json`：前两轮完整四步及exact零Decision，
取消轮在正Decision后立即零Decision，未形成Yielded/SDK连接。测试只观察原消息，不注入Ready/owner。
App首页账户ready，scopes/links/sessions=0且无opening/closing；本机服务健康，Inspector观察器已移除并关闭。
本次boot观测heapMin52868 B、largest47092 B，空闲heap约103584～105228 B；不将短样本冒充无泄漏/发布长稳。
没有Action、child重刷/重绑、清凭据、暂存、提交或推送。现场保留本节Hub工程固件+81ee0809 APK，
旧F3b固件及旧APK、镜像保留可回退；bit18仅此完整工程composition启用。

**下一批F4剩余：真实management占用/释放短负测，再10→30→100。**
用原同绑定管理事务/permit窗口占用唯一radio（不reset/enroll/detach）观察：
占用未结束时不得发送Yielded/App native connect；只让原管理事务按自身deadline/正常close结束，
结束后仍有效的Direct才可Ready；若App先取消，迟到Ready不得启动native，也不得中止别的管理owner。
该项有Host覆盖但尚无本轮物理证据；不能把MQTT管理订阅正常等同于管理争用测试通过。
D4b、N2～N5、WiFiProv、Wio/Hub发布长稳独立保留；禁止用循环次数替代未通过的取消/管理门禁。

### 10.94 F4真实management所有权负测与分阶段循环（2026-09-08）

延续§10.93完整冻结组合：Hub78c8e6eb、App81ee0809、Gateway `l2c-direct-f4`、
Shard1 `l2c-direct-f4-ready`。本批没有生产源码、SDK、固件、APK或镜像修改，
没有重置/重刷child、修改Access/Key、发送Action或重绑。网络mostfun2026，双XIAO+真实Android。

**管理短负测已通过，证据不是“管理订阅正常”：**

- 新门禁只调用原owner HTTP的Open/Get/Close；不选候选、不发送enrollment/attach/revoke、
  不注入Broker owner/Ready。普通页面连接与管理窗口分别由原调用方拥有，工具只观察时序。
- 保持同一个独立生产账户Cloud owner，两次窗口串口均实际出现`permitState=1 / relayActive=1`，
  从原child会话释放到扫描窗口；`permitConnected=0`，本轮覆盖已占用radio的扫描窗口，
  **不是已选candidate并传输中的enrollment/grant事务**，后者仍属于N2～N5等独立门禁。
- 第一轮预约132 ms被接受，窗口继续Ready。原owner在页面开始后约3069.9 ms请求Close，
  physical Ready3254 ms→SDK connect3561 ms→fresh/canControl6183 ms；SDK关闭42 ms、OS确认释放，零残留。
  第一轮Cloud不可控11869 ms后恢复；同一counter0，没有用Action补造基线。
- 第二轮113 ms预约接受，146 ms取消→147 ms导航→150 msReady拒绝→209 ms页面回收。
  页面回收后原管理窗口仍Ready；独立owner约3024.2 ms才请求Close。
  观察至约7213 ms，覆盖终态之后约3.78 s，SDK connect/close均0，无迟到Ready。
- 第二轮Cloud先6383 ms后恢复，随后按原预算让出，8491 ms后再恢复；保留这段短可用窗口证据，
  不以测试过程把管理预算/公平周期退款，也不宣称无中断或稳定低延迟。整体32906 ms通过。
- `build/fairness-f4-management.jsonl`、`fairness-f4-management-wire.json`、
  `fairness-f4-management-serial.jsonl`保留证据。wire第一轮延迟Yielded到Close之后；
  第二轮只有Probe/Challenge/正Decision/零Decision，未出现Yielded。

测试代码复用既有`observeOnlinePage`和生产账户Cloud observer，没有平行连接客户端；
只增加有界取消观察时长、同一WebView单调时钟关联与精确原操作finally Close。
在线循环入口仅接受默认2和显式10/30/100，每轮单connect/单close/OS释放/fresh/counter/同一owner，
阶段末再做Ready前取消。失败即停，不能静默重试或自动提升阶段。
工具119项、Host/ASan50项、Broker针对性95项与架构检查通过；App/Service生产实现无变更。

**首阶段10次已通过（`fairness-f4-online-10.jsonl`）：**

- 一个独立Cloud owner，10轮普通页逐轮通过fresh/canControl/counter0、Ready后单次native connect、
  单次关闭与OS释放；10轮均等待原owner实际Cloud恢复，再进入下一轮。
- 页面就绪4011～4193 ms，平均4105.6 ms；SDK关闭37～53 ms，平均44.2 ms。
- 最后另一次取消149→导航150→Ready拒绝152→回收216 ms，native连接/关闭0次。
- gate167814 ms通过，167821 ms完成清理；结束Hub wanted0/permitState0/relayActive0。
  资源ipc0/loop/NimBLE最低1104/3612/2708 B，空闲heap105232 B。
- 串口分段文件`fairness-f4-management-serial.jsonl`和`fairness-f4-online-10-serial.jsonl`跨阶段采集，
  读口切换存在短间隔；不把它们当成连续逐包记录或24 h无泄漏证明。

**30次未通过：**10次清理和资源复核后独立启动30次，前8轮完整通过；第9轮App Ready4178 ms、
正常单次连接/关闭且OS释放，但该轮Cloud从133734 ms失去fresh，到163745 ms仍未恢复，
触发既有30 s read-only readiness deadline。163755 ms停止清理，未进入第10轮，未自动重试。
`fairness-f4-online-30.jsonl`与`fairness-f4-online-30-serial.jsonl`保留失败。
Hub期间仍wanted1、MQTT管理在线，多次Secure后又退让；出现一次routeError18（StateConflict），
随后恢复0，不能单凭这个错误码断言根因。App/Hub最终清理为空闲，bit18仍限工程。
先补不含业务payload的readiness事实/错误码观测，区分Presence未恢复和Manifest/State同步未完成；
不加大30 s超时、不修改SDK/调度预算掩盖失败，30/100阶段不得宣称通过。

已补工具白名单诊断：`reachable/manifest/stateFresh/revision`按实际Store变化记录，失败只记录已知readiness/协议错误码。
不打印任意异常文本、Store values或credential；新增三项防泄漏/类型回归，最终工具122项通过。
**下一批先定位本轮30 s恢复缺口，至少对齐三层证据：**
账户caller的Presence与同步阶段、Broker exact route generation/lease/closure、Hub安全会话及radio退出原因。
当前失败记录不足以区分缺失Presence、旧代投影/晚回包或预算内bootstrap未完成，禁止猜测后直接增大常数。
复现后用最小回归修正实际责任模块；通过短复核后重跑完整30次，最后单独准入100次。

**诊断与现场收尾：**补白名单观测后只做两轮短复核，未重新启动30次：
`fairness-f4-online-diagnostic.jsonl`两轮页面Ready4006/4089 ms、Cloud恢复20287/36476 ms，
最后138 ms取消、201 ms回收、无native连接，38289 ms通过。
投影可区分不可达与State重同步，但本次未复现原超时，**不是根因修复或30次通过**。
后续runner与只读串口记录增加host epoch起点，方便联合Broker时间线；不增加任何device协议字段。
最终App首页账户ready、scope/link/session0，无opening/closing；Hub Stopped/wanted0/permit0/relay0，
资源ipc0/loop/NimBLE1104/3612/2708 B、heapMin52832 B、largest47092 B、空闲heap约105220 B。
本boot小幅水位变化与短采样不作为泄漏或无泄漏结论。串口读取和测试调用均已结束，未暂存/提交/推送。

### 10.95 F4恢复链边界修正与分阶段复核（2026-09-08）

本批先对§10.94失败补Broker只读元数据追踪，再修改已复现的责任模块。没有增加30 s readiness预算、
radio预算、自动Action重试或第二套连接调度器；未修改Service/Broker生产代码、镜像、schema与协议。

**旧组合诊断，不改写历史失败：**`fairness-f4-recovery-30.jsonl`前29轮完整通过，第30轮App
Ready4093 ms、SDK关闭44 ms且OS释放正常；Cloud在既有30 s内未恢复，502811 ms结束等待。
失败Store为reachable=false、manifest=true、stateFresh=true、revision1：缓存State不是可达/可控证据。
Broker记录491733 ms时cloud owners已为0，换算runner481733 ms，比caller超时早约21.08 s；
Hub同步wanted0。它不是caller finally release后的正常零需求，优先检查续租，而非增加BLE等待常数。
Broker/serial/runner epoch分别1788844736350/1788844736675/1788844746350。
旧观察器wire按初始MQTT client过滤，末条wire83730 ms后未继续记录，也未记录MQTT会话变更，
因此存在观测盲区；registered/projection/owners仍持续有效。不能把缺失wire当作设备停止上报证据。
后续诊断按当前exact Hub session取样、记录非秘密incarnation，并只读记录目标owner的action/revision/status；
有界事件缓存、自动恢复原函数，不打印body/token/secret，也不注入owner/Ready。

**两个最小修正，各自有独立回归：**

1. App `DeviceV2DemandOwners.schedule`：计时器延迟的小数精度可能早于单调截止点唤醒。
   旧逻辑在nextRenew之前调用Direct-only readyQuery；普通Cloud owner直接返回，未续租、未再安排timer，
   本地accepted owner还在，Broker则按30 s正常到期。新增提前0.5 ms的回归在旧实现只得到acquire、
   没有后续renew；修正为未到due时重排同一个timer，并对延迟向上取整。Cloud/Direct均覆盖，
   不提前续租或重复query、不延长lease、不重建owner、不添加轮询。376项App测试通过。
2. 设备 `GatewayChildRouteBridge`：有效旧投影StateRequest/Command在后继Connecting/Authenticating阶段
   到达时，旧`!secure`检查先调用fail并退役当前attempt，早于投影generation检查。
   将secure检查归入原inactive/stale projection拒收分支；仍先验证帧、Active access与身份，
   拒绝迟到数据但不关闭后继连接，不排队/重发。两状态×两消息回归在旧实现disconnectCalls失败；
   新实现还验证新Secure阶段拒绝旧代、当前代正常转发。Host/ASan50项通过。

第一项解释了本次“服务器需求已到期、客户端未关闭”的可复现实现缺口；第二项是审查时独立复现的错误。
旧wire盲区与§10.94原第9轮证据不完整，因此不声称两个历史现场失败均已逐包证明为同一原因。
`fairness-f4-demand-before.log`和`fairness-f4-stale-before.log`保留红灯证据，修正版软件记录分别为
`fairness-f4-demand-fixed-app-tests.log`、`fairness-f4-stale-fixed-host.log`。

**冻结复核组合：**Hub SHA256 `71e82b72e1e53063bfb1da4c04dca4d64f24b4637d267c142653f8d06c3e60b1`，
Flash1,447,104 B（+4 B）/static RAM65,628 B（不变）；仅COM11 app区0x10000原位烧录并校验。
APK SHA256 `ee981b3e8e799c533bb54e6ccc0458c48c22971bf5573b3341d57a84be23a50b`，原位安装保留数据。
其余沿用§10.94镜像、ESP32 SDK3.3.11和测试网络mostfun2026。未烧录/重置child，未改绑定、Access、Key，
未发送Action。工程bit18及未保护存储夹具不等于D4b或公开产品准入。

**新组合短2轮及10轮已通过，30轮未通过：**短2轮Ready4337/4110 ms，39994 ms通过。
独立10轮152631 ms通过、152638 ms清理；一个Cloud owner，没有提前retire，每轮counter0、单次连接/关闭与OS释放。
页面Ready4000～4319 ms（均4101.4）、SDK close19～46 ms（均35.5）；Cloud逐轮实际恢复。
末尾取消141→导航142→Ready拒绝144→回收196 ms，native连接/关闭0次。证据为
`fairness-f4-fixed-2.jsonl`、`fairness-f4-fixed-10.jsonl`。
按短2轮→10轮→30轮逐阶段验收；每轮真实普通页面Ready/单次native连接关闭/OS释放、
同一个独立生产账户Cloud owner恢复、counter0，阶段末Ready前取消。任何阶段失败即停，不自动重试。
100轮、management已选candidate传输负测、D4b/N2～N5和发布长稳继续独立保留。

**30轮新失败已定位到另一条合同边界，不是续租修正失效：**
`fairness-f4-fixed-30.jsonl`前4轮完整通过；第5轮App Ready3986 ms、SDK关闭23 ms、OS释放正常，
Cloud在runner99140 ms触发原30 s deadline，99152 ms清理停止。Broker追踪
`fairness-f4-fixed-30-wire.json`记录同一Cloud owner从revision1 acquire到2～10 renew全部accepted，
约10 s一次；owner直到caller结束后才移除，不再出现本地假存活/服务器先到期。

| Broker相对时刻 | 实际事件 |
| --- | --- |
| 69.306～69.439 s | Direct Challenge→29892 ms Decision→真实Yielded，Cloud原投影关闭 |
| 71.123→71.525 s | Hub MQTT会话断开并以新会话恢复，route admission与execution challenge换代 |
| 73.156 s | 手机原Direct owner release被Broker接受 |
| 81.322 / 91.335 s | 原Cloud owner继续renew accepted；新Hub execution Decision仍为正 |
| 约99.134 s | Hub才再次进入Connecting；距caller deadline仅约0.31 s，无法完成GATT/Manifest/State |
| 99.446 s起 | caller失败清理，随后Broker owners变0，Hub恢复空闲 |

Broker epoch1788846276113，runner1788846276419，serial1788845935977；三个时钟只用于本机证据换算，
不能把Broker收到Challenge的时间当成设备精确计时锚。此次没有主动注入断网；捕获的是实际MQTT会话换代，
未新增刷新原因日志，不能仅由这张表区分定时SessionCredential refresh与其他网络触发。

`GatewayDirectYieldLease.retire(now)`有意保留已经对手机发出的承诺到**原**deadline，
同时销毁旧wire authority；新会话的零Decision不能取消旧承诺。既有`retiredPromise`回归明确覆盖它。
这条安全性是正确的：MQTT断开/重新登录不证明手机ACL已释放，不能为了30轮通过直接clear租约、
跨代接受旧零Decision或让旧Ready自动成为新Ready。真正的缺口是**最大30 s旧物理承诺与整个Cloud恢复也只给30 s相冲突**，
尚未有可执行的换代恢复体验/准入合同。本轮保留失败，不把“安全等待是预期”改写为恢复已通过。

**下一批唯一主线：换代恢复专门门禁，先确定责任/预算再修改生产实现。**

1. 保持当前2项已证实修正，先定向覆盖Yielded后MQTT换代＋Direct release、手机仍持有ACL、
   grant前换代与重复断线。复用真实原owner和同一子设备，不做rebind/Key刷新，不发送Action。
2. 分别记录原物理承诺终点、MQTT恢复、native可获取、Manifest/State就绪；独立观察器可在一次30 s
   read-only请求超时后保留其自身Cloud需求到有界恢复测量结束，但必须记录第一次失败，
   任何第二次只读请求必须作为显式新attempt，不伪装原请求成功或静默Action重试。
3. 据此冻结“安全等待/重新获取/只读同步”的分阶段恢复预算，以及调用方何时提示可重试；
   原普通readiness30 s保持不变，不直接把它改成60 s。若产品要求跨MQTT换代也必须原30 s内恢复，
   必须另证exact旧承诺的安全取消/连续性或物理租约策略；不能仅凭新MQTT已连接提前抢占。
   优先复用现有owner、generation和deadline；无真实收益不加持久表、第二调度器或新wire。
4. 正常路径和换代路径分别通过后，重跑完整30，再独立执行100；不能通过挑选避开刷新时段来宣称整个30门禁完成。

本批Broker针对性95项、完整V2回归也通过。所有失败与软件红灯文件保留在ignored build；未暂存/提交/推送。

**新版管理争用短回归与收尾：**`fairness-f4-fixed-management.jsonl`两条原Open/Get/Close路径35907 ms通过，
35915 ms清理。正向原窗口Close后physical Ready3251 ms→SDK connect3481 ms→页面6145 ms，SDK关闭33 ms、OS释放；
负向取消152→导航153→Ready拒绝155→页面回收208 ms，观察至约7215 ms，native连接/关闭0次。
Cloud原owner恢复、counter0；只测试扫描窗口，没有选candidate或执行grant传输。该独立通过不覆盖跨MQTT换代失败。
最终App首页账户ready、scope/link/session0，无opening/closing；Broker当前Hub/admission/Active binding正常且owners0。
Hub Stopped/wanted0/permit0/relay0，最终heap105224 B、heapMin50612 B、largest47092 B，
ipc0/loop/NimBLE最低1104/3656/2700 B。本boot包含TLS换代/多轮测试，水位不是无泄漏或发布结论。
900 s只读串口采集已正常结束，Broker临时观察函数已恢复且Inspector关闭，没有遗留循环、重刷child或变更凭据。

### 10.96 F5换代恢复实测与物理承诺分离（2026-09-08）

按§10.95先测量，再执行需求合同6.34。没有重置/烧录Hub或child、修改Key/Access/绑定、发送Action；
本轮设备保持71e82b72，测试仍为双XIAO/真实Android、mostfun2026。网络故障只关闭已确认的exact测试Hub MQTT连接，
不停止其他Broker/设备，也不注入owner/Ready。其他任务的测试容器没有被修改。

**原30 s组合定向证据：**

| 场景 | 第一次readiness结果 | 后续显式只读恢复 | BLE/故障事实 |
| --- | --- | --- | --- |
| Yielded后断MQTT，手机正常退出 | 34663 ms触发原30 s timeout | attempt2在36871 ms成功，耗2208 ms | 单connect/close，counter0 |
| 手机页面Ready后继续持有5 s，同承诺内断MQTT两次 | 34618 ms触发原30 s timeout | attempt2在37088 ms成功，耗2470 ms | hold5030 ms始末OS均确认连接；两次新MQTT连接均实际断开 |
| Challenge后、正Decision前断MQTT | 原Cloud请求在19875 ms前恢复 | 无第二次读请求 | App Ready拒绝1177 ms，1323 ms回收；native连接/关闭0次；wire无正Decision或Yielded |

文件`fairness-f5-{release,held,ungranted}.jsonl`及对应`-wire.json`保留证据。两条timeout没有被改记为首次成功，
观察器保持原Cloud需求，自身通过显式`recoverAfterTimeout()`只增加一次read-only尝试；fatal/原失败保留，
owner退休/取消/已关闭/非timeout均不允许继续。既有30轮门禁不调用此方法，不能用它凑循环通过。
额外Host覆盖授权前换代不产生物理承诺、原30 s上限跨wrap、近期限重复retire也不延时。

**修订的第一性原则：**需求表示“调用方还在”，物理承诺表示“网关在这一小段时间内不重新获取radio”，
不应因为需求TTL是30 s就总承诺让出30 s。也不应把上层超时改成60 s，或取消已对手机发出的安全承诺。
选择短、显式可续期的物理分片，继续复用原nonce/revision/Ready/deadline与单一radio编排。

- App `DeviceV2DemandOwners`：Ready验证后按**query发出时间**和实际剩余deadline计算半期续租点，
  与原nextRenew取较早值；30 s grant仍最多10 s一次，10 s grant通常约5 s一次，短grant也无需板型常数。
  accepted renew不能延长旧物理deadline，只有新exact Ready可延长；未知/拒绝/过期仍关闭同一scope。
- Broker交换核心增加可选、严格1～30000 ms的transport policy，默认仍30000；
  GATT实际driver配置10000 ms工程候选，发新Decision时再与owner剩余期取min。
  不改变Cloud/Direct owner TTL、协议最大值、身份/授权、退役的原终点或MCU存储。
- 候选10 s沿用当前普通native占用的10 s量级，用于缩短取消链路丢失后的安全尾巴；
  不是所有网络条件下恢复一定小于30 s的承诺。应通过真实换代/管理/连续续租/循环门禁验证，失败继续保留。
- 发布顺序必须App自适应续租先于Broker短grant；旧App固定10 s续租不满足新10 s物理分片。
  此条件属于三端组合准入，不用强迫用户更换MCU SDK或重写Sketch。

软件回归：Host/ASan50、App38文件380项、工具126、Broker98+完整V2、架构通过。
旧实现红灯见`fairness-f5-adaptive-before.log`（4/10 s grant不会提前renew）和`fairness-f5-physical-before.log`；
新App测试兼容4/10/30 s grant，覆盖短续租未得到新Ready仍原期关闭。长grant测试精确timer边界的观察另补1 ms，
不能把这个测试时序修正说成生产缺陷。核心默认30 s的既有wire/跨端golden没有改成10 s。

**已部署候选：**APK SHA256 `dc755c23b8c5eb5addb8b77145c98e2efb935555b5d7e5468fefff83eb06901d`，
先原位安装；再仅重建Shard1为`aily-iot-dev-broker-shard-1:l2c-direct-f5-slice`。
Gateway仍`l2c-direct-f4`，其他shard/服务不动。镜像只替换两个JS：交换核心5增/1删、driver3增；
先与运行版逐文件比较，未夹带其他Service修改。设备固件、Flash/RAM、SDK和wire不变。

**候选定向与短门禁：**

| 场景 | 新组合实测 | 边界 |
| --- | --- | --- |
| Yielded后单次关闭exact Hub MQTT | 实际Decision10000 ms；原请求在总测量18779 ms时恢复 | 没有attempt2，counter0 |
| 同承诺内两次关闭MQTT | 两次真实断开；原请求在21246 ms时恢复 | 没有第二次读请求，不把新MQTT当作旧承诺取消 |
| Challenge后、正Decision前关闭MQTT | 原Cloud恢复20034 ms | wire无正Decision/Yielded，native连接/关闭0次 |
| 页面Ready后持有8 s，两轮 | 38473 ms完整通过；Ready4100/4108 ms，每轮一个native连接 | 每个同一owner revision1→2→3→4分别为acquire/renew/renew/release；六份10000 ms正Decision均有新exact Yielded，hold始末OS保持连接 |
| 原管理窗口正向及取消 | 33183 ms通过、33187 ms完成清理 | 正向Close请求后才Ready4242→SDK4748→页面7370 ms；取消148→Ready拒绝151→scope回收204 ms，native0次 |

持续持有完整工具末尾还独立执行Ready前取消：147 ms取消、150 msReady拒绝、204 ms页面回收，无迟到native。
首轮持续工具虽然两次持有均通过，但把hold8000参数错误传给末尾cancel场景，最终配置拒绝；
保留`fairness-f5-sustained.jsonl`失败，仅修测试调用处显式holdMillis=0，再完整重跑为`-sustained-final.jsonl`。
不把工具配置错误算作生产故障，也不把首轮部分通过冒充完整门禁。
管理只覆盖原Open/Get/Close扫描窗口，未选candidate或传grant；定向失败/通过的wire与观察器结果全部保留。

**10轮阶段已通过：**`fairness-f5-online-10.jsonl`在166793 ms通过、166799 ms清理，10轮均一次connect/close、
OS释放、原Cloud owner恢复，counter0；页面Ready4088～4403 ms，SDK关闭27～58 ms。
Cloud不可用窗口15096～16034 ms（均值15616.4 ms），是单连接交替的真实成本，不宣称无中断。
末尾取消149→Ready拒绝152→scope回收205 ms，native0次；最终App首页scope/link/session0、Broker owners0，
Hub Stopped/wanted0/permit0/relay0，heap105240 B、heapMin50612 B、largest47092 B，
ipc0/loop/NimBLE最低1104/3656/2700 B。通过上述收尾后才启动完整30。

**完整30轮通过：**`fairness-f5-online-30.jsonl`在457509 ms通过、457521 ms完成清理，进程exit0。
30轮全部一次connect/close、OS释放、原Cloud owner恢复、counter0；无第二次read-only请求或隐式Action重试。
页面Ready3983～4510 ms（均值4144.9 ms），SDK关闭25～51 ms（均值39.8 ms）；
Cloud不可用7108～16252 ms（均值14576.9 ms）。末尾取消148→Ready拒绝149→scope回收165 ms，native0次。
Broker观察1494条未截断，正Decision均10000 ms，同一Cloud owner45次renew均accepted，无demand-rejected。
本次自然MQTT切换在3.107→3.608 s的初始阶段，另一次483.864→484.164 s已晚于门禁终点；
不能把后者算进30轮或冒称30轮专门覆盖了正承诺期间断线。该负测由前述单/重复exact断线实验独立证明。
旧§10.94/10.95失败以及本批旧30 s组合的首次timeout均保留，不被新结果覆盖。

**最终清理与资源：**App回到首页，账户ready，scope/link/session0、无opening/closing；Broker当前Hub、Active binding、
管理与route均正常，owners0。Hub Stopped/wanted0/permit0/relay0，最终heap105236 B、heapMin50268 B、largest47092 B，
ipc0/loop/NimBLE最低1104/3656/2700 B。采集区间largest曾降至31732 B，heap历史低水位较10轮后下降344 B；
不把这些短期样本判为泄漏，也不宣称无泄漏。曾出现瞬态routeError18(StateConflict)/22(WouldBlock)后归0，
child error保持0；保留原诊断码，不通过消音或修改SDK来使门禁看似干净，100轮继续联合追踪。
原900 s串口采集自然结束后换为720 s有界采集，后者完整覆盖30轮；按换行积累、限制长度并丢弃4条不完整/越界诊断行，
不将缺失值当成0。720 s采集已结束、COM11释放；Broker临时观察函数已恢复、Inspector确认不监听。
未留下循环进程，未重刷child或修改NVS/Key/Access/绑定；本批未暂存、提交或推送。

**原下一步（已被§10.97暂停）：**维持Hub71e82b72/APK dc755c23/Broker f5组合，先确认原账号、绑定和零残留，再独立执行100轮。
预计约25～30 min，沿用原普通30 s deadline、单Cloud owner、逐轮完整恢复与最后取消；失败即停，不跳轮、不清计数、不reset后重记。
通过100也不等于发布长稳或无泄漏；D4b/N2～N5、Wio发布长稳和更复杂故障仍独立准入，不扩散Mesh/多child/History/Bulk。

### 10.97 S0/S1 网关共享连接主线修订（2026-09-08）

用户明确拒绝将7～16 s交替不可用作为正常体验，暂停100轮。正式决策/分批步骤见
[DEVICE_V2_GATEWAY_SHARED_CONNECTION_IMPLEMENTATION_PLAN.md](DEVICE_V2_GATEWAY_SHARED_CONNECTION_IMPLEMENTATION_PLAN.md)。
App账户MQTT、Gateway MQTT、Gateway→Child BLE是不同层；每个child不增加App MQTT。
绑定目录、并发连接槽、业务流量分别限额；多用户共享同一child连接，未来上游MQTT/本地WebSocket共用南向会话。

S1先改普通网关子设备页面在线默认Cloud，不查本地凭据来决定是否抢占、不申请Direct owner、不误断已有本地业务；
保留原有界离线Direct和普通WiFi+BLE行为。S1不能消除旧GatewayGattRadio的10/6周期；
Host连接池、本地鉴权/需求、WAN与radio生命周期拆分和显式离线接管必须分阶段完成后才替换此策略。
旧在线Direct页面工具记录保留作历史证据，不可用它要求新默认页面继续制造Direct；新的默认路径回归应断言无native连接。
Broker交换协议和旧物理承诺安全底座不是本批删除目标；没有通过扫描/凭据推断授权，也不添加测试后门或新UI开关。

**S0/S1软件完成，未部署：**

- App `DeviceUiPort`在线/网络未知时直接使用现有账户Cloud需求，不等待本地凭据、不申请Direct owner或native连接。
  Cloud拒绝/超时/会话结束不会隐式进入离线Direct。被动状态观察仍可读取本地凭据以显示权限，不参与路径接管。
- Cloud页面只释放自身owner；在创建scope时记录本地清理责任，目录删除后也不误断其他原生会话。
  离线Direct保持当前凭据、共享物理预算、真实关闭后的空档及epoch/取消边界；凭据读取期间网络恢复则终止该请求。
- 删除旧在线自动Direct接线、未使用的`DeviceV2DemandError`依赖、离线预算里已无生产调用者的`assertReleased/used`，
  以及条件不可达的被动disconnect。旧页面测试替换为默认Cloud与有界离线测试；协议层Direct Ready/承诺测试保留。
- 新默认路由13项先在旧版得到7项失败，修正后全部通过；通信/页面39文件385项＋BLE服务/准入/网络/缓存4文件32项通过。
  本地Web构建、设备Host重建/50项CTest、架构检查通过。日志在忽略的`build/shared-s1-*.log`，不是发布产物。
- 本批没有MCU/Service/Broker生产修改、SDK变更、Key刷新、清记录、刷板、APK安装或100轮；现有脏改动保留，未暂存/提交/推送。
  现场仍是上一轮安装组合；软件通过不等于新默认页面或多child真机已验收。

**下一批S2：**按共享连接方案4.1抽取当前Product实际消费的child上下文，消除单receiver和单出站记录的分派限制，
随后共享Host/扫描与第二活动槽纵向接通。Windows当前确认COM3/COM11/COM12，COM3为用户新增板；
尚未识别/改变其固件，不将串口数量当2-child成功证据。先核对角色，再使用新板B，不重置现有A。
原Hub周期/离线接管与S3/S4连续性边界仍待完成，D4b/N2～N5及发布长稳不据此关闭。

### 10.98 S2a 子设备上下文与独立路由端点（2026-09-08）

**本批代码：**

1. 新`BasicGatewayGattChildContext`接管逐child工作区、GATT link、安全session、connection owner、bridge与proof/revoke/renew协调器。
   注入`IBleCentralPort`、两个原子记录blob、`IDeviceKeySource`、clock/random/crypto和共享route client；不持有WiFi/SDK Host或MQTT socket。
   原ESP32 Product实际改用该上下文，删除旧组装和字段；仍把所有连接调度委托给原radio，不新增另一套调度器。
2. `GatewayRouteClient`只持有共享MQTT订阅/topic；新`GatewayRouteEndpoint`拥有各自696 B消息buffer及有界消费状态。
   A阻塞时B仍可入站/消费，原1 s入口预算不因重投延长；清理只擦除实际占用字节，不在每次空闲poll重复擦整块buffer。
   未知/歧义身份不广播，上下行都不能借另一端点发送；离线/MQTT换代清所有Cloud入口，但单端点更换/销毁不影响其他端点。
3. `GatewayRouteBinding`复用既有六种canonical decoder，以logical id、instance、epoch、credential version选择当前记录；
   消费前重新选择，防止队列等待期间换记录。选择不是鉴权，Bridge仍检查operation、topology、expiry、projection、sequence。
4. `IngressCapacityProbe`改为端点包装，串口观察由测试Sketch显式调用；旧“继承整个MQTT client拦截单receiver”的实现删除。
   默认产品不增加诊断回调/虚函数。架构门禁同步约束新上下文不能持有平台SDK/WiFi/MQTT或自行启动child。

**验证与资源：**

- Host/ASan 50个CTest目标通过；路由目标内新增双端点背压/取消/销毁、未知/歧义目标、换代/记录改变、双真实上下文admission隔离、
  六种消息绑定与畸形编码、选中后仍拒绝错误operation、跨child上行拒绝。这里的A/B是软件夹具，不是COM3真机。
- Arduino-ESP32 3.3.11对象ABI：共享路由280 B、每child端点736 B、活动上下文7544 B（包含端点，不能再次相加）。
  不含NimBLE堆/任务栈、SDK Port、WiFi/TLS和未来目录；`CompileEsp32EdgeHubInternal`的只读尺寸见证已确认被ELF链接器剔除，不占运行RAM。
- 最终源码使用Arduino CLI 1.5.2-rc.1、Arduino-ESP32 3.3.11和`XIAO_ESP32S3:PartitionScheme=default_8MB`完成两项构建：
  `CompileEsp32EdgeHubInternal` Flash 1,440,264 B / 静态RAM 65,628 B；
  `Esp32EdgeHubDualBoard`（`BLINKER_TEST_INGRESS_CAPACITY=1`）Flash 1,449,368 B / 静态RAM 65,684 B。
  包含最终上行唯一端点选择修正；前一轮中间二进制不作为最终证据。静态RAM不包含运行时SDK/TLS堆与任务栈。
- 架构检查、单Arduino库文件验证（336个文件）及release-profile检查通过。证据为本地忽略目录中的
  `build/shared-s2-final-host.log`、`build/shared-s2-esp32-final.log`、`build/shared-s2-dual-final.log`。
  较早系统旧CLI构建被停止，未计入通过结果；未替换用户SDK/依赖。
- 无上传、串口复位、Key刷新、清记录或新绑定。App/Service/Broker生产代码未改；未暂存、提交或推送。

**边界与下一批：**

- 当前实际产品仍仅一个Port和原有access/renewal blob；只是为每child提供独立上下文及北向入口，不能宣称原生并发、目录、管理路由或持续低延迟已实现。
- S2b实现有界child目录/存储选择、按child/operation定位管理请求。未知/槽满必须拒绝，新增B不得覆盖A。
  当前小规模matcher读取旧受保护record；目录应提供有失效语义的非秘密身份投影，量化每帧Flash读取/loop栈成本，不能把逐帧全目录读取扩展到大容量。
- 然后S2c共享NimBLE Host/扫描与2个活动槽→S2d纵向与1Hub+2child真机。上下文销毁/重用必须等待对应原生槽真正关闭。
  COM3保留作新增B，先核对身份/角色；本批未操作COM3/COM11/COM12。S3/S4、D4b/N2～N5及发布长稳继续保留。

### 10.99 S2b-1 可重建记录目录与非秘密身份索引（2026-09-08）

**实际代码与边界：**

- `GatewayChildRecords`拥有注入access/renewal blob的唯一写入包装，GATT context改为借用记录，方便绑定目录与活动工作区分离。
  索引仅保存logical id、instance、operation、epoch/version和记录种类，不缓存secret、PresenceKey或完整journal。
  写入/清除尝试在调用后端前使索引失效；失败写入不会形成乐观新身份。后端重开/外部维护需要显式invalidate，禁止多owner绕过包装写相同blob。
- `GatewayChildDirectory`以调用方有界条目复用原record codec。无额外持久化账本，也不在选择时保留/占用空槽；只有选中处理器通过AEAD、context、期限及原子写验证后生成ACK。
  已有logical id、instance或operation冲突拒绝；未知renewal不分配；墓碑和孤立renewal journal保留容量；损坏/不可读条目阻止不能安全判重的下发。
- ESP32 Product的真实`GatewayManagementClient`改用目录处理initial access/renewal envelope；删除原直接指向唯一child delivery的接线。
  原proof/revoke/renew控制命令与Hub permit-join仍由单child mux处理；不把这两类envelope分派等同于整个management完成。
- `GatewayRouteBinding`删除按帧读完整受保护record的选择实现，改用失效驱动索引；Bridge授权检查不删。
  新条目重建、换记录后的第一次选择仍需Flash读取；暖选择不读Flash，不等于整个BBP/2交互不读Flash。

**验证：**

- Host/ASan 50目标通过，新增两槽AEAD失败/容量/冲突、重建RAM并反序枚举后的exact ACK、墓碑占槽及finalize后重用、孤立renewal日志占槽、
  损坏记录、失败写入、Presence激活与credential promotion后索引失效等回归；现有六类route各100次暖匹配不增加存储读取。
  同一目录中renewal只修改目标journal；未知topology/child不分派。所有A/B和断电后重建场景是软件夹具，不是真机断电或双BLE证明。
- 架构、单Arduino库340文件与release-profile检查通过。ESP32 3.3.11 MCU ABI：记录/非秘密索引包装208 B/绑定、目录12 B、entry8 B/绑定；
  GATT活动上下文仍7544 B，但现为借用记录，需另加外部208 B，不能沿用原单context总成本。共享路由280 B和每child入口736 B不变。
- 从实际ESP32对象反汇编复核，拆开冷路径后`identity`自身静态栈帧从416 B降至32 B，`refresh`为400 B；
  目录`process/select`自身帧为320/96 B。此为单函数frame，必须再计调用者/被调用decoder及SDK，不代表Arduino loop真实高水位。
  暖匹配零Flash读取由六种消息回归证明，冷active索引仍需读access与renewal各一次；无24h/100轮/双BLE硬件证据。
- 最终构建使用Arduino CLI 1.5.2-rc.1、ESP32 3.3.11、`XIAO_ESP32S3:PartitionScheme=default_8MB`，
  内部产品Flash 1,441,808 B / 静态RAM 65,852 B；比S2a分别增加1544/224 B。
  双板背压probe（`BLINKER_TEST_INGRESS_CAPACITY=1`）Flash 1,450,896 B / 静态RAM 65,908 B，构建通过；比S2a增加1528/224 B。
  两项均含最终冷/暖路径拆分；只读sizeof见证已被最终ELF剔除，不占运行RAM。静态RAM数不包括动态SDK/TLS堆与任务栈。
  本地证据：`build/shared-s2b-host.log`、`build/shared-s2b-esp32.log`、`build/shared-s2b-dual.log`，均属忽略目录。

**下一批：**S2b-2见共享连接方案6.2。尤其补齐撤销墓碑清除后的迟到Finalize/Supersede：不能把未知operation路由到任意空槽，
也不能用一概拒绝导致原幂等恢复退化。平台0号旧记录必须保留，不能为新增B擦掉A。随后共享Host/native双槽与1Hub+2child纵向门禁。
当前未开放第二个持久/原生槽，没有操作COM3/COM11/COM12、Key或绑定；无App/Service/Broker生产改动、部署、暂存、提交或推送。

### 10.100 S2b-2a 目录管理分派与终态幂等收口（2026-09-08）

**边界与实现：**

- `GatewayChildManagementRouter`从canonical proof/revoke/renew命令提取身份/operation/epoch，遍历已配置目录后分派到来源child的原协调器。
  目录entry只增加可选control指针；未知、重复记录/control、多个匹配或绑定不符拒绝，已匹配但无活动control返回WouldBlock，不回退第一个槽。
  初始/轮换envelope仍走S2b-1的认证delivery入口；外层router不更改持久记录、不依赖SDK、不创建socket/task/动态队列。
- 单个Hub permit-join与逐child操作族mux分离。A任务、原生所有权或未发布结果未结束时，B不能抢占；非法命令不reset A。
  物理准入关闭时，仅原owner认可的exact continuation可继续；未知请求仍留在Broker原有有界ledger。
  实际Product的management与radio已消费router，旧直接管理指向已替换；ControllerControl回复仍绑定来源child mux，不经过可变current-owner指针。
  原操作族mux仍有实际用途，未另建一套coordinator或candidate Product。
- **terminal不增账本：**现有store在空记录上本已允许finalize/retire幂等。扩展为整个受保护目录新鲜确认旧authority缺席，编码原Finalized/Retired结果，
  64 B有界待发布buffer仅保存当前结果，不是完成日志；不挑选空槽、不发送新的child请求、不把Hub清除结果说成child签名receipt。
  匹配record/墓碑仍交原协调器校验和处理。槽被新epoch复用且RAM重新构建后，旧命令可获相同结果，不更改新凭据。
  同authority换operation、损坏/不可读/歧义记录不能当缺席；关联的renewal journal阻止terminal（WouldBlock），不留下可恢复旧秘密后误报清理完成。
  关联journal的交错前向恢复尚未完成，不在本批自动清除它。所有响应继续受Broker原Hub/tenant、operation/topology/phase、digest和TTL约束。

**验证与资源：**

- Host/ASan 50目标全部通过（5.11 s）；原测试目标中新增双child管理互斥、exact continuation、未发布结果背压、Hub permit关闭屏障、
  未配置control/歧义拒绝、真实revocation coordinator终态与目录缺席响应逐字节一致、最大uint32 topology、RAM重建/槽复用、同authority换operation、
  损坏记录/孤立renewal阻断，以及两套真实renewal coordinator的Prepare/Cancel定向和journal隔离回归。它们是软件测试，不是真机双连接或断电证明。
- Broker仅新增测试：错误digest、错误Hub和未知topology均拒绝，Finalized/Retired exact重复接受；TypeScript编译与完整`test-device-v2.js`通过。
  使用本机Node22.22.3匹配既有better-sqlite3 ABI；默认Node24曾因ABI127/137不匹配不能运行，未重装依赖或改变运行服务。
  无Service/Broker生产源码、wire、schema、镜像或部署变化；App源码/APK未改。
- 架构检查覆盖来源child回复接线和router无持久写/SDK依赖；Arduino发布文件清单342文件、release-profile检查通过。
  ESP32 3.3.11、`XIAO_ESP32S3:PartitionScheme=default_8MB`两构建通过：内部产品Flash **1,443,956 B** / 静态RAM **65,948 B**，
  相比§10.99增加2148/96 B；双板背压probe（`BLINKER_TEST_INGRESS_CAPACITY=1`）Flash **1,452,972 B** / 静态RAM **66,004 B**，增加2076/96 B。
- MCU对象sizeof见证：管理router **88 B**；外部records208 B、目录12 B、GATT活动上下文7544 B、共享route280 B、child入口736 B不变。
  entry多一个32位指针，由8变12 B。只读sizeof数据段被最终ELF剔除；静态RAM不含动态SDK/TLS堆和任务栈。
  反汇编单函数栈帧：router handle64 B、select112 B、目标decode144 B、matches416 B、finishAbsent176 B。
  terminal必须冷读目录且调用record decoder；这些是各函数frame，不能把单一frame当整个调用栈峰值。真实双槽仍需单独测loop/SDK水位。
- 本地忽略目录证据：`build/shared-s2b2-host.log`、`shared-s2b2-esp32.log`、`shared-s2b2-dual.log`、`shared-s2b2-broker-build.log`、`shared-s2b2-broker-test.log`。

**下一步/不宣称完成：**

S2b-2分为本批2a控制分派与下一批2b平台多记录后端。后端必须消费到真实Product，保留0号NVS namespace/key和原记录格式，
分别配置绑定目录与活动工作区容量，先验证旧记录、槽间失败/重启隔离；不堆未消费的候选storage类。
随后S2c共享Central Host/扫描＋两个native slot，补逐槽异步关闭与晚callback隔离，再S2d用1Hub+2child纵向真机。
当前仍只有一对持久blob和一个native槽，既有radio周期/离线承诺未改，7～16 s体验空档尚未消除；100轮继续暂停。
未操作COM3/COM11/COM12、手机网络、Key、Access或绑定，未部署、刷板、暂存、提交或推送。

### 10.101 S2b-2b 双记录存储后端与绑定/活动工作区分离（2026-09-08）

**实现与清理：**

- `BasicEsp32GatewayStorage`被实际Platform/Product消费，持有2对独立access/renewal原子blob和各自失效驱动的非秘密索引。
  0号旧名称、分区策略与记录codec不变；没有数据搬迁、分区擦除、MAC索引或额外持久化账本。新增槽名称明确冻结：

  | 绑定槽 | Access namespace/key | Renewal namespace/key |
  | --- | --- | --- |
  | 0（原记录） | `bl_eh_access/child` | `bl_eh_renew/pending` |
  | 1（新增记录） | `bl_eh_access/child_1` | `bl_eh_renew/pending_1` |

- `GatewayChildBinding`拥有轻量凭据接收处理器，目录中的A/B都可分别接收经原AEAD/context/期限验证的初始/轮换envelope。
  删除活动`GatewayGattChildContext`中的delivery组装和DeviceKey source参数；它不再因“有绑定”就必须分配握手/分片工作区，也不接触Hub根Key source。
  仍复用原delivery processors/mux，没有复制协议、MQTT或另一套Product。旧Platform单blob访问器和Product私有单条records已移除。
- 当前实际组合仅0号绑定接一个活动GATT context，1号control为空：匹配到B的管理命令返回WouldBlock，B没有route活动端点。
  存储ACK仅表示凭据持久化，不是proof完成、绑定Active、在线或可控制；共享Host/原生池完成前不部署多child能力声明。
- `resetAccess()`通过bank逐条清renewal/access，仅全部成功后才清Hub DeviceKey。中途失败不回滚已清槽、不继续清root；重试包含空槽。
  begin/end在外部后端生命周期变动前失效全部索引；4处open任意失败都关闭已打开句柄，不通过擦分区“修复”失败。
- 顺带修正通用`Esp32NvsAtomicBlobStore::clear()`：erase成功但commit失败后，重试即使返回NotFound也必须确认commit成功，再返回NotFound供store归一化。
  新故障模型在旧代码上于“已返回清理成功、durable仍有B”断言失败，修正版通过。它覆盖NVS边界保守故障语义，不宣称真实SDK/Flash上已复现该故障。

**验证与资源：**

- Host/ASan **51目标通过，5.20 s**。新增NVS多handle/key目标调用真实NVS适配器和record codec，验证：旧0号access＋pending原位读取无写入，
  新B不更改A、out-of-range不别名0号、损坏/读取失败不当空槽、RAM完全重建及exact重投、set/commit失败后模拟掉电、部分clear失败/未重启重试、
  四处open失败资源回收、保护验证拒绝/明文记录拒绝，以及Hub root在子记录清理失败时保持。旧通用NVS测试与其他协议目标全部保留。
  原目录加密/容量/重建测试已消费生产`GatewayChildBinding`，删除测试内重复的处理器组装；物理掉电/擦除一致性仍需以后真机，不用Host模拟替代。
- 架构检查补齐“全部bank清理成功才清root”“轻量binding独立容量”“活动context不含delivery/root source”；单Arduino库**344文件**及release-profile验证通过。
- 最终Arduino CLI / ESP32 **3.3.11** / `XIAO_ESP32S3:PartitionScheme=default_8MB`：
  内部产品Flash **1,444,444 B** / 静态RAM **66,372 B**，相对§10.100 **+488/+424 B**；
  双板背压probe（`BLINKER_TEST_INGRESS_CAPACITY=1`）Flash **1,453,568 B** / 静态RAM **66,428 B**，相对§10.100 **+596/+424 B**。
  “双板probe”仍指历史Hub＋child测试Sketch，不代表两条同时活动的child连接。
- MCU对象ABI：2槽storage bank **704 B**（含4个NVS blob＋2份208 B records）、轻量binding **60 B/绑定**；
  GATT活动context **7488 B**，比原7544 B少56 B；共享route280 B、入口736 B（已含于context）、管理router88 B、目录12 B不变。
  不按绑定容量乘以完整7488 B工作区。sizeof只读见证被链接器剔除；静态RAM不含NVS打开句柄、SDK/TLS堆和任务栈，未来双槽仍需真实资源准入。
- 证据均在忽略目录：`build/shared-s2b2b-build.log`、`shared-s2b2b-host.log`、`shared-s2b2b-esp32.log`、`shared-s2b2b-dual.log`。
  本批App/Service/Broker无生产源码或协议/schema变化，只同步文档，未重跑Broker或App测试；§10.100的Broker结果不冒充本批新执行。

**下一步与发布边界：**

共享方案6.3冻结S2c-1唯一Central Host生命周期→S2c-2双Port活动槽/晚callback隔离→S2c-3/S2d实际B的context/radio/management接线。
当前源码绑定容量2、活动容量1，不加Sketch容量开关；SDK仍原配置，原有radio交替/有界离线承诺保留，100轮继续暂停。
降低活动容量不能减少完整持久目录的枚举/重置范围；只识别槽0的旧固件不具备安全清理槽1的能力，不作为未经处理的降级回滚方案。
后续先编译和堆栈预算，再1Hub+2child真机；7～16 s交替空档尚未消除。本批没有刷板、操作COM3/COM11/COM12、改手机网络/Key/Access/绑定、部署、暂存、提交或推送。

### 10.102 S2c-1 唯一Central Host与有界回调借用（2026-09-08）

**实现与删除的旧持有关系：**

- `Esp32NimBleCentralHost`被实际ESP32 Product持有，声明在Port之前、析构在Port之后。它独占SDK init/sync/reset/stop/deinit及全局角色claim；
  `Esp32NimBleCentralPort`通过构造注入借用Host，删除`activePort()`、自有Host任务/sync/shutdown、初始化标记和本机地址副本。
  SDK ready预算归Host（默认2000 ms），Port只保留本连接close预算（默认1000 ms）；不改变公开Sketch/API或SDK配置。
- 当前Host固定登记2个Port，**只有一个原生活动owner**，覆盖扫描、发起、ACL及关闭；实际Product仍一份Port/context。
  A扫描/Connecting/Ready时B不能取得活动权；空闲B停止不调用全局scan cancel，也不终止A的ACL。停止Port不停止SDK，Host只在整个产品结束时关闭。
  这不是双ACL实现，更不是协议Ready。下一批才将串行扫描/发起权与逐ACL占用拆开，并纵向接入B。
- SDK的7类GAP/GATT callback全部携带非复用的进程期整数token，不携带Port地址。Host在固定登记表内解析并借用owner；
  token在attempt结束时失效，计数未排空前不复用/注销。Host restart不归零sequence，UINT32耗尽拒绝而不是回绕复用；没有逐attempt堆对象或新队列。
  旧Port已经销毁、下一个Port复用native handle=7及attempt=1时，旧CONNECT/DISCONNECT/indication/写ACK仍不能影响新owner。
  原生句柄与token都不是device身份，不涉及MAC、Key、GatewayAccess或协议字段。
- SDK reset同时通知全部登记者，sync只恢复Host就绪，不覆盖Port错误。延后逻辑完成也不能把待处理reset变回Idle；必须明确stop/start恢复。
  stop遇到原生未关闭或在途callback返回Error/WouldBlock并保持占用。只有完全排空才注销，关闭A期间completion启动B也不能让A继续等待/取消B。
  全Host停止失败保留角色；stop成功而deinit失败后，仅重试deinit，不重复停止已结束的Host任务，也不借出半关闭Host。
- 复核补齐原生CONNECT完成屏障：SDK的initiating标记可能先清除、连接已在原生表内，而CONNECT尚未送达Port。
  不能用“扫描/发起过程已停止＋Port尚无handle”判断空闲；新loop侧待完成标记从发起到CONNECT消费或SDK reset为止保持占用。
  延迟/缺失回调都不释放未知ACL，迟到成功先收养并终止。新增测试在修正前失败（`late CONNECT adopted and closed before release`）、修正后通过；
  这是SDK边界故障模型，不声称本批在真实控制器已复现。Port原多用途Host result枚举/回调已删除，仅保留SDK reset锁存位。
- 生命周期合同：调用方在stop未完成时必须保留Port/Host并重试；强行析构仍被SDK借用的对象会`abort()`，避免释放后使用。
  这是非法销毁的fail-fast，不是正常超时触发整机重启或自动清凭据策略。产品日常窗口只停止Port，不析构Host；Central/Peripheral/WiFiProv互斥角色边界仍保留。

**验证与资源：**

- 真实Central Port实现＋SDK边界fake累计**18场景**：保留原8项取消/关闭/迟到CONNECT回归，新增SDK建好ACL但CONNECT尚未交付的延迟/缺失两项、共享Host/登记溢出、旧回调跨对象/handle/Host restart、
  在途借用阻止复用、延后完成排空及reset隔离、SDK reset→sync通知、角色冲突/init无内存/sync超时/stop与deinit失败恢复、completion启动另一借用者。
  fake仍只模拟一个native handle的活动表，不将它称为双ACL/radio测试。当前ACK和indication正向路径同测，防止“全部丢弃就通过负测”。
- 完整Host/ASan **51目标通过，5.18 s**。架构检查约束Host生产持有顺序、Port不再初始化SDK、7类callback令牌解析及Host不依赖凭据/MQTT/业务工作区；
  单Arduino库**345文件**、release-profile检查通过。只新增内部Host头文件，没有新增独立库/候选Product或用户配置层。
- 最终Arduino CLI / ESP32 **3.3.11** / `XIAO_ESP32S3:PartitionScheme=default_8MB`：

  | 编译目标 | Flash | 静态RAM | 相对§10.101 |
  | --- | --- | --- | --- |
  | CompileEsp32EdgeHubInternal | 1,445,984 B | 66,436 B | +1,540 / +64 B |
  | Esp32EdgeHubDualBoard（ingress capacity=1） | 1,455,016 B | 66,500 B | +1,448 / +72 B |

  后者仍是历史Hub＋child双板probe，不代表1Hub＋2child。对象ABI新增共享Host **56 B**，单Port **1068 B**；
  bank704 B、binding60 B、GATT活动context7488 B、route280 B/入口736 B、管理router88 B、目录12 B保持。
  sizeof只读见证在最终ELF中被剔除；这些是静态对象/链接结果，不包含SDK堆、任务栈、双连接运行资源或射频性能，不能推断ipc水位/长稳已验。
- 日志保留于忽略目录：`build/shared-s2c1-build.log`、`shared-s2c1-ctest.log`、`shared-s2c1-esp32.log`、`shared-s2c1-dual.log`。
  关闭屏障修正前的失败为`shared-s2c1-pre-fence-test.log`，与修正后结果分开保留。
  App/Service/Broker只同步文档，本批未改其生产源码、重跑其测试或部署。没有操作串口/ADB/网络、刷板、重置、改Key/Access/绑定、暂存、提交或推送。

**下一步：**共享方案6.3的S2c-2（串行发起权＋双native slot/handle隔离）→S2c-3/S2d（B的context/route/control和有界radio消费）→1Hub+2child短真机。
S2c-1不能抵消S3本地有限授权/WebSocket和S4取消默认交替的剩余工作；7～16 s空档未消除，旧离线承诺保留，100轮继续暂停。

### 10.103 S2c-2 双原生槽、串行发起与逐连接故障隔离（2026-09-08）

**本批实现与清理：**

- 用Host的逐Port活动槽＋唯一`initiating_`替换原覆盖整个ACL寿命的`activity_`。
  扫描/发起仍串行；CONNECT被loop消费后释放发起权，A保留ACL并可继续GATT发现，B可开始扫描/建连。
  仍使用原Port、Link接口、token、工作区和公开Sketch，不新增另一个Host/Product、任务、消息队列或协议字段。
- `Port::stop()`仅在持有发起权时取消SDK全局扫描/发起过程，且只等待自己的连接、待交付CONNECT与在途callback。
  A的关闭不能取消B的scan/connect，也不等B关闭；A原生关闭失败保持A占用，B收发与自己的槽复用继续。
  已被接受的terminate不重复请求；SDK拒绝B的scan/connect时不覆盖A。原先CONNECT尚未交付的关闭屏障继续保留。
  若CCCD成功结果已排队但调用方先stop，直接终止对应ACL、不再回调迟到Ready；故障测试修正前失败、修正版通过，另一条已Ready连接不受影响。
- SDK reset在Host中失效全部旧token并退休发起权；Port先消费reset、丢弃旧handle再操作SDK。
  新增“B先恢复并复用旧A句柄、A尚未poll就stop”的回归，证明A不会误断B；未完成callback仍阻止其对应Port注销。
  错误native handle的DISCONNECT不清本Port队列，ENC_CHANGE不覆盖另一连接的安全事实；未在等待CONNECT时不接受额外CONNECT。
- 容量是`min(2, CONFIG_BT_NIMBLE_MAX_CONNECTIONS)`，SDK只有1槽则第二登记明确CapacityExceeded，仍可单槽运行；没有版本号白名单或修改SDK配置。
  本机3.3.11/ESP32S3的预编译配置为NimBLE max connections=3、controller max activities=6；这只是静态准入，不是可用堆/射频吞吐证明。
  现有内部ESP32 probe显式检查该目标支持2槽；不因SDK宏存在就声明实际Product已可双child。
- SDK边界fake的Central路径由单个native handle/全局GATT回调改为2条独立原生连接记录、逐handle发现/写回调和背压结果。
  Peripheral原有测试行为保留；没有新生产测试钩子。错路回调负测同时验证当前连接的正向ACK/indication，不能靠丢弃所有数据通过测试。

**验证与资源：**

- Central **29场景**：保留18个旧关闭/生命周期场景并调整其串行范围，新增双发现交错、A/B同时GATT Ready、独立写ACK、满RX队列/发送背压、
  A关闭时B扫描/连接、A关闭失败、旧token复用、SDK reset后交叉复用句柄、SDK scan/connect拒绝、A在途callback时B复用自身槽。
  另一个独立编译目标设置SDK max connections=1，验证容量拒绝不停止原owner及释放后的顺序复用。
- 完整Host/ASan **52目标通过，5.12 s**；架构、单Arduino库**345文件**、release-profile通过。
  架构门禁禁止回到覆盖整个ACL的单activity owner，要求SDK容量边界、发起权交接、reset token失效及原有7类回调解析。
- Arduino CLI / ESP32 **3.3.11** / `XIAO_ESP32S3:PartitionScheme=default_8MB`：

  | 编译目标 | Flash | 静态RAM | 相对§10.102 |
  | --- | --- | --- | --- |
  | CompileEsp32EdgeHubInternal | 1,446,328 B | 66,436 B | +344 / 0 B |
  | Esp32EdgeHubDualBoard（ingress capacity=1） | 1,455,368 B | 66,500 B | +352 / 0 B |

  MCU对象ABI：共享Host56 B、单Port1068 B保持；sizeof只读见证不进入最终ELF。当前实际Product仍分配一个Port/context，
  因此“静态RAM不增加”不能外推为真实双连接没有动态成本；下一批第二份7488 B活动context/1068 B Port、SDK堆/任务栈与WiFi共存仍需准入。
  “DualBoard”是历史Hub＋child测试Sketch，不是两child同时业务通信的证明。
- 忽略目录证据：`build/shared-s2c2-build.log`、`shared-s2c2-ctest.log`、`shared-s2c2-esp32.log`、`shared-s2c2-dual.log`。
  `shared-s2c2-ready-before-test.log`保留迟到Ready修正前的失败，不用后续通过覆盖它。
  无设备/ADB/网络操作、刷板/重置、Key/Access/绑定修改、部署、暂存、提交或推送。App/Service/Broker只同步文档，未改源码/重跑其测试。

**审查后冻结的下一步：**

实际Product仍固定child0，不能据本批GATT Ready声称B的认证/proof/route Ready完成。共享方案6.4明确S2c-3a→3b→3c→S2d：
先移除逐child mux中的全局permit生命周期、按目标slot准入；再将radio的逐child策略与Hub唯一permit/发起仲裁分开，
最后接通B的context/route/control并进行1Hub+2child真机。两个radio不能对同一router覆盖`setAdmission`；permit不得固定释放A。
原有离线承诺、S3本地有限授权/WebSocket、S4取消默认交替和100轮暂停保持；7～16 s空档尚未因此消除。

### 10.104 S2c-3a 子设备管理、来源回包与Hub配对窗口解耦（2026-09-08）

**实现与清理：**

- `GatewayManagementControlMux`只接该child的proof/revoke/renew；删除permit解码、引用、轮询、reset和全局占用判断，以及旧的可选renewal/四参数构造。
  一份24 B mux与ControllerControl回包接线归入`BasicGatewayGattChildContext`，Product不再另行持有和组装。销毁context只注销自己的接收器。
- `GatewayGattRadio`固定接自己的child mux，不再通过可替换Management模板把全局router当成某个child；Hub router改为独立`setPermitAdmission`。
  A的准入关闭不阻止B的新管理，permit准入关闭也不阻止允许的child；原exact continuation、全局管理串行owner、待发布结果与原生未释放屏障保留。
  不把“管理串行”扩成所有child普通数据串行；不过真正两份radio/permit物理槽归属仍要在3b接通。
- 子设备切换/reset不清理Hub permit；整体reset由router仅清一次permit。子mux reset若协调器仍声明异步占用则保留当前分派，继续poll，不能靠清枚举伪造释放。
  原子设备来源回复不跟随Hub可变owner；目录未知/歧义、全目录terminal absence与可恢复journal阻断不改协议。
- 删除radio内部的管理poll。在线管理客户端负责router tick；Product在断网、停止或订阅失败时补充Hub tick，订阅已成功但route未就绪时不重复poll。
  原permit超时/异步关闭不会因为拆掉child→permit引用而停止推进，也不会因未来增加radio数量而成倍轮询。

**验证与资源：**

- 完整Host/ASan **52/52**；覆盖A准入关闭/B允许、permit独立准入、子mux拒绝permit、子reset不影响permit、全局reset只清一次、异步占用继续poll，
  以及真实renewal NonceReady发布后仍排斥Hub permit、permit四种关闭路径排空/终态结果消费后才能回全目录absence。测试的旧Hub四族mux路径已改用生产router。
- 既有Central29场景、SDK单槽容量目标、GATT周期/关闭/时钟与fuzz回归保持。架构检查防止重新把permit放回child mux或把radio准入绑到全局router。
- `check_device_v2.py`、单Arduino发布345文件、release-profile通过；没有新公开API、协议字段、持久账本、任务或队列。
- ESP32 Arduino **3.3.11** / XIAO ESP32S3 / default_8MB：

| 编译目标 | Flash | 静态RAM | 相比§10.103 |
|---|---:|---:|---|
| CompileEsp32EdgeHubInternal | 1,446,272 B | 66,428 B | -56 B / -8 B |
| Esp32EdgeHubDualBoard，ingress capacity=1 | 1,455,336 B | 66,492 B | -32 B / -8 B |

- MCU ABI：活动context **7,512 B**（已含移入的24 B mux，原7,488 B不含）；Host56 B、Port1,068 B、共享router88 B、route280 B、入口736 B、存储bank704 B及binding60 B保持。
  readonly sizeof见证只在对象文件，不占最终固件存储；静态RAM减8 B是当前单活动context产品的布局结果，不是双连接动态成本证明。
- 本地日志：`build/shared-s2c3a-build.log`、`shared-s2c3a-ctest.log`、`shared-s2c3a-esp32.log`、`shared-s2c3a-dual.log`。
  没有刷板/部署、操作ADB、修改Key/Access/绑定或暂存/提交/推送；App与Service只同步文档，未改其源码或重跑其测试。

**下一步边界：**

实际Product仍只有child0 context；permit仍引用0号Port，全局permit占用尚未改为slot级借用，不能声明双child业务或7～16 s空档已消失。
下一批3b明确物理槽借还和目标child准入，再3c连接两个业务工作区、S2d资源与真机；借空闲B槽不意味着允许连接正在履行Direct承诺的A。
不得为新增B重置已有A。当前用户确认网络为“带翅膀的蜗牛”；密码只用于后续本地测试配置，不写进发布示例/文档。
S3有限本地授权/WebSocket、S4移除默认强制交替及D4b/N2～N5/发布长稳仍独立未完成，100轮仍暂停。

### 10.105 S2c-3b/3c：有界GATT空槽借还、双context接线及现有A四端短测（2026-09-08）

**边界与实现：**

- `GatewayGattBootGuard`只保留一个Hub启动安全屏障；`GatewayGattRadio`只消费自身需求、Direct承诺、管理与native关闭状态，删除全局permit占用依赖。
- `GatewayGattPool`由调用方提供有界slot表，调度发起权、借还空闲Port；不拥有SDK/MQTT、secret缓存、队列或另一套凭据。
  新`IGattPermitJoinPortLease`是GATT专用能力；通用permit协议接口删除旧固定Port lease，保持其他southbound可独立实现。
- 唯一`NativeGattPermitJoinAdapter`在开窗时取得Port；满槽返回Busy而不是驱逐A。物理停止并解除callback后才归还；Link下次start重新装入接收器。
  无lease/已归还时clear selection不访问空指针；Error/异步关闭仍要排空。旧固定A端口、强制释放A和兼容重载均删除。
- 物理借用槽不等于目标child：候选选定后复用当前Access和授权Presence匹配已绑定目标；借B连接A仍检查A的Direct/需求/管理，并额外保留A准入到原生关闭。
  不用MAC授权；损坏记录、歧义身份、无法排除既有承诺时拒绝。新Provisioning仍必须完成原grant/receipt/proof，不因借到槽获得权限。
- 每loop有界轮询；只有扫描/发起连接串行。未选定候选的permit扫描在占用2 s且另一child等待时让出发起权，原窗口不延长；选定后的opaque relay不跨连接续传。
- 实际ESP32内部Product组装2个Port/context/radio，目录第2项接入自身管理mux；WiFi、DeviceKey、MQTT、clock、crypto、Host与permit共享。
  删除六个固定child0内部访问入口，probe改为`child(index)`及串口0/1选择。公开Arduino API不变，不增加用户参数或多套产品ZIP。
- 旧16 s搜索上限/10 s占用/6 s机会窗和已发出Direct承诺仍保留到S4；本批不是“无缝切换”实现。SDK容量不足的内部两child构建拒绝，不按版本号猜能力。

**软件证据：**

- Host/ASan完整52/52（13.85 s）；真实radio＋adapter测试覆盖A保持/B借用、Direct隔离、满槽Busy、异步归还、错误Port归还、回调恢复、扫描轮换、A停止/B继续及记录损坏拒绝。
  Central既有29场景/SDK单槽目标仍在完整门禁中。架构检查、单Arduino发布347文件及release profile检查通过。
- SDK3.3.11编译两活动工作区：DualBoard ingress=1 Flash1,457,928 B，静态RAM75,228 B；较3a分别+2,592/+8,736 B。
  内部Product编译Flash1,448,524 B，静态RAM75,140 B；较3a分别+2,252/+8,712 B。
  MCU对象ABI：context7512 B/slot（含mux）、Port1068 B/slot、radio52 B/slot、pool48 B、共享boot guard12 B；Host56 B、router88 B不变。
  这些数值不含SDK动态分配和整条TLS/crypto调用栈，不能代替真实双连接压力测量。

**现场与真实短测：**

1. USB身份核对：COM11原Hub `8C:BF:EA:1B:96:14`，COM12原A `98:3D:AE:EE:7C:6C`，COM3新增板 `8C:BF:EA:8E:F1:2C`。USB/MAC仅定位烧录对象，不用作协议身份。
   手机PGEM10/ADB `196f65f9`登录态可用。Hub应用先ROM备份；stub读Flash在434,176 B重复中断，改ROM读取完成且镜像checksum/hash有效，未改SDK来绕过。
2. 只写COM11 app0 `0x10000`，Flash hash校验通过；固件SHA256 `c658ce8b1aef629fe6a5bc0f1528ec9761ac7f047f770ed45af3042cbc79225d`。
   原分区表SHA256 `f4e3b6cfe370c81dca210d57f3c463f924a539842a19fff6eda4615278ee97a2`，未改bootloader/分区表/NVS/eFuse，未重置A或刷COM3。
   Probe默认读取持久DeviceKey，旧手动测试key仅显式`BLINKER_TEST_INSTALL_DEVICE_KEY`才安装；本次未启用。仅更新已授权WiFi配置。
3. 当前电脑`192.168.2.76`；本地Gateway公布地址/代理/忽略的设备CA与WiFi配置同步新网段。TLS cert到2026-09-15；旧代理停止并改名备份，未删除数据库/密钥卷。
   只重建本地Gateway容器配置，不重建Broker镜像/协议，不改变secret/ACL/拓扑。手机保持自己的账户MQTT，经ADB reverse连接本地服务；不是新建设备MQTT连接。
4. COM11复用原Key上线，managementSubscribed=1；App真实账户需求获accepted后，Hub→A经过原鉴权进入Secure。
   使用App实际Cloud API一次`button`，Manifest已接受、State fresh，计数0→1并收到ACK；没有未知结果后另发第二个Action。
   **普通页面验收未通过：**已装APK仍缺当前S1的`isGatewayChild`分流，手机native网络在线却仍走Direct；本次Cloud控制证据来自真实账户API，不冒充新页面已验。
5. 持有A Cloud需求期间，真实owner permit窗口完成Open/Ready/候选发现、错误token NotFound、exact replay、改变payload 409及Close。
   未选择真实设备/提交新绑定；序列采样未捕获A Secure与permit Ready同一行，故不据此宣称双原生同时状态已完整验收。
6. 单A及permit短测最小heapFree92,948 B、heapMin72,812 B、采样最大连续块最低51,188 B；`ipc0`剩余1104 B、loop3640 B、NimBLE2688 B。
   没有重现76 B，但仅单child短测，不覆盖B并行/TLS重连/SDK reset长稳。结束释放测试owner并退出页面，executionWanted=0、child Stopped、permit Stopped，Hub MQTT管理订阅保持。

证据位于忽略的`build/shared-s2c3bc-{build,ctest,architecture,esp32,dual,backup-rom,flash,app-before,cloud-command,cloud-close,permit-gate,permit-serial,demand-serial,close-serial}*`。
App/Service生产源码未在本批修改；没有暂存/提交/推送；未清除设备身份或绑定。测试使用既有内部未加密NVS诊断后端，不是生产加密存储发布证据。

**下一批S2d顺序：**先构建/安装当前S1 App，验证普通在线页面无Direct抢占且Cloud可控；再给COM3准备child B并完成正式绑定，保留A。
然后同一Hub/同一账户MQTT测A/B同时Manifest/State/Action/ACK、A关闭B继续、各自管理和Direct承诺、满槽Busy不驱逐、SDK reset及动态堆/栈。
完成这些后再推进S3有限本地授权/WebSocket与S4去周期交替；不要先恢复100轮，也不要把单child成功抵扣双child门禁。

### 10.106 S2d实机部署、第二child正式绑定及Service/Broker单child遗漏（2026-09-08）

**已执行：**

1. 手机安装当前S1本地debug APK，保留账号数据；安装前备份旧APK并核对签名一致。普通A页面在线选Cloud，无Direct抢占，
   Manifest接受、State fresh，真实页面一次button计数1→2/ACK；离页后自身scope归零。不是额外账户MQTT或另持测试owner抵扣页面验收。
2. COM3身份核对后仅烧录app0，保留原bootloader、分区表和NVS；旧应用备份有效。child固件Flash694,844 B/staticRAM34,848 B，
   镜像694,992 B小于原app分区容量。COM11/COM12本批未刷/未重置，A与Hub原Key/Access保留。
3. 实际App permit扫描、Hello→Noise→grant relay→receipt→Method2→网关proof完成B绑定。
   B=`ble_GnO2jmRwLwgTQk-nL_ktdA`，instance=`13993cc102d4933f2fb4e5e3d5cf80c1`，
   operation=`88580a66b7fd5bee5f8dc0f791d31258`；权威topology=Active(5)、Hub stored/child installed/gateway proven均为true。
   恢复始终使用同一operation；未重建事务或删除A。结束时恢复checkpoint已清、手机回设备列表。

**发现并修正的主线遗漏：**

- B绑定后普通控制页前置检查失败：原Service `cloudEnabled`只看自身DeviceKey版本。真实B没有DeviceKey是正确设计，不能靠给它启用设备MQTT凭据解决。
  当前源码已改为“自身Cloud凭据或有效网关绑定”，在owner列表、read、instance解析及已授权分享列表统一投影。
  批量查询最多100项，只返回被授权清单内的logical ID，不读取secret/envelope；检查tenant、Active、期限、当前Access epoch、Hub有效状态和当前Key版本。
  `credentialVersion/locator`保持null，App Key/配网入口仍需真实DeviceKey上下文。能力不等于在线/Ready，不能替代Broker逐child ACL/route校验。
- 更关键的是Service `getActiveRouteForHub()`仍LIMIT 2并拒绝第二条Active；Broker `routeByHub: WeakMap<AedesClient, RouteContext>`仍只存一个context，
  generation/admission/recovery也仍挂整个连接。现场Gateway日志已明确报`G2A Edge Hub has more than one active child route`，解释了B已Active而App恢复仍报失败。
  **S2d不能只部署设备双槽后直接测试：必须先完成S2e服务器多child路由。** 不允许取第一条、覆盖A、临时撤销A或新建第二条Hub MQTT绕过限制。
  原文“下一批直接双child验收”的前置条件不完整，已纠正共享方案§6.5。B尚无Manifest/State/Command/ACK数据面成功证据。
- 清单修正暂未部署，须与S2e配套测试后部署，防止App提前选择尚未支持的双child Cloud路径。生产Broker、Gateway镜像本批未替换。

**验证与资源：**

- App正确Angular test builder：42文件383项通过；修正两个候选扫描测试依赖真实1 ms计时的偶发失败，改fake timer，仅测试改动，不延长生产发现deadline。
  本地Web与Android debug构建、ADB安装成功。最初直接调用Vitest未加载Angular环境的失败不作为生产回归；最终以builder结果为准。
- Service session/management18项、Gateway typecheck、真实PostgreSQL Edge Gateway access套件通过；随后带PG完整Device V2测试脚本通过（含分享E2E/repository/worker，最后node:test批28/28，无PG跳过）。
  新增纯BLE无Key、owner/read/resolve/share一致、跨tenant/清单隔离、100项分批、过期/撤销/epoch/Hub Key变化等回归。
  真实数据库只读查询证实A/B网关能力均true，B自身Key仍无；没有修改生产数据库记录去伪造能力。
- Hub绑定时heapMin38,612 B，收尾采样降至38,600 B；loop最低剩余2,172 B，ipc0=1104 B、NimBLE=2688 B；含本批绑定与较长运行观察，
  不应继续用上一批heapMin72,812 B冒充本批最低值；尚未完成真实双Secure/TLS重连叠加测试，也未把低堆原因直接归为泄漏。
- 证据在忽略的`build/shared-s2d-*`：page-gate、page-serial、child-build、b-app-before、b-flash、enrollment-hub、service-session、service-pg、service-suite-pg等。
  Windows完整脚本因没有DB环境停止，随后在现有Linux容器独立临时源码目录/随机测试schema跑全套通过；没有替换容器运行中的dist。
  未新增MCU生产API/协议/Key，未暂存/提交/推送；100轮暂停，原周期让出仍保留。

**下一批：S2e先软件后配套部署**（共享方案§6.5）：bounded Hub route集合→Broker逐child projection/authority/晚回调隔离→既有A/B双数据面短测。
再测A断开B继续、满槽Busy不驱逐、管理/Direct隔离、MQTT换代及SDK reset/资源；全部通过后才S3/S4。

### 10.107 S2e多child路由、无Key子设备冷启动及四端并行短测（2026-09-08）

**本批完成的软件与部署：**

- Service删除旧`getActiveRouteForHub`单值合同。内部`GET /internal/v2/edge-gateway/routes`只接受Hub端点，返回确定排序、最多2条Active路由；
  `/route`只接受child端点。共享profile容量2，超限/重复child/重复instance拒绝，不截断取首条。校验所属shard、tenant、有效Hub Key、child epoch、期限和轮换阻断。
  SQL只选路由元数据，不再把topology整行及KMS材料带入路由层；增加`deviceKeyVersion`与当前Hub MQTT claims精确匹配。内部HTTP与Broker须配套升级。
- Broker保留一个Hub MQTT、一个路由topic对，把RouteContext、generation floor、admission/recovery、BBP session/bootstrap、Command/ACK、execution/Direct、presence逐child隔离。
  每MQTT身份最多记录2个不同child，包括已退休的floor；不静默淘汰反重放记录。换入第三个不同身份须新MQTT身份，当前不是无限绑定目录/通用大容量调度器。
  删除旧Hub级单child撤权入口；撤权现在携带exact child、operationId、topologyVersion，迟到旧operation/version不能退休后继或兄弟child。
- 首次部署暴露额外冷启动问题：Service inventory已正确显示无Key的B，但Broker原`v2_devices`要求credential_version非空，B尚无元数据。
  App订阅B的Presence时抛`logical device is unknown`，导致整个账户MQTT断开；此前软件fixture提前provision了B，掩盖了实际路径。
  已改为有效路由准入建立**仅身份元数据**，`credential_version=NULL`，不生成DeviceKey/会话/token、不声明在线；登录接口仍拒绝无凭据记录。
  后续显式启用Cloud可在同一行登记真实凭据，保留身份、Manifest、State；已有Key的A不被覆盖。未知/歧义目标返回该请求的有界Error，不关闭整个App账户连接。
- Broker旧SQLite父表通过事务重建支持NULL，保留主键、数据、索引/触发器和外键；本机升级前已用SQLite一致性备份保存到忽略的build目录。
  tenant migration导出/导入同步支持NULL元数据，仍为现有v3结构；**迁移两端须部署支持NULL的版本，不能向旧Broker迁移含此类child的租户**。
  本次只部署本机Gateway与shard1候选镜像，不宣称其余shard/线上已升级。回退不能只切旧镜像或盲目恢复旧数据库而丢弃后续业务记录。

**验证：**Broker完整Device V2脚本通过（165项node:test＋原主脚本、tenant migration、telemetry/rate与真实Redis）；协议22项、Service完整真实PG脚本通过（最后批28项）。
新测试覆盖两个子设备交错Command/ACK、A换代/撤权/迟到查询不影响B、独立需求、部分admission重投、错误Hub Key/重复/超限、退休floor容量、
真实无Key B冷启动、拒绝无Key独立登录、后续启用Cloud、旧SQLite迁移/FK保留、未知Presence目标不终止账户。硬件App gate另有3项自测。
没有修改设备生产源码/SDK或重新烧录，没有改App UI/APK；本批新增测试工具消费现有Cloud API，不添加产品测试入口。

**真实四端结果（COM11 Hub、COM12 A、COM3 B、ADB手机）：**

1. 普通B页面Cloud Manifest/State fresh、一次button/ACK计数0→1；普通A页面同路径2→3。两者均`direct=false`，退出scope=0。
2. 同一手机账户MQTT同时持有A/B的真实Cloud需求，二者Manifest/State均Ready；并行各发一次button，A 3→4、B 1→2，两个ACK成功。
   Hub `g`直接读取SDK连接表同时输出两条GATT连接，均interval=12、latency=0、supervision=256、MTU=23；不是两份虚拟状态冒充双物理连接。
3. 短窗口释放A后，串口确认A Stopped/secure=0/executionWanted=0，SDK仅余一条连接；B仍Ready/secure=1/executionWanted=1，
   B随后一次button/ACK计数2→3，A保持4，账户MQTT未更换。测试owner全部释放，手机回首页，串口选择恢复0；原Key/Access/绑定未清除。
4. **持续性未通过，不能包装成无缝切换：**两次较慢观察跨越了旧`MaximumOccupancyMillis=10000`，B即使仍有需求也进入自己的周期让出，
   后续Action因未Ready而未发送。缩短到当前有效窗口后的独立关闭短测通过，只证明作用域隔离，不抵扣S3/S4或长稳。
   旧6s机会窗口/10s占用仍为明确产品缺口，不是再加100次循环即可解决的问题。
5. 本批Hub采样heapMin38,552 B、loop2172 B、ipc0=1104 B、NimBLE2688 B；无76 B重现。
   这是当前固件生命周期累计最小值和短测证据，不能代表双child+TLS重连/SDK reset资源发布准入。

证据：`build/shared-s2e-{broker-full,service-pg,cold-child-tests}*`、`build/shared-s2e-evidence.json`及本轮工具输出。
候选镜像`aily-iot-dev-gateway:s2e-multichild` / `aily-iot-dev-broker-shard-1:s2e-multichild`，旧镜像和一致性备份保留；无暂存/提交/推送。

**下一步：**S2d补双占用时permit Busy不驱逐、管理/Direct exact目标隔离、MQTT换代与SDK reset资源短门禁。
然后按共享方案S3补有限本地授权/WebSocket与明确离线接管合同，再S4取消默认强制交替；可以先细化S3合同，但不能无条件删除当前离线安全屏障。
100轮继续暂停；D4b/N2～N5、Wio/Hub发布长稳仍独立保留。旧单child fault工具不得直接对双child Hub安装全topic拦截器，后续使用前先补exact-child过滤回归。

### 10.108 S2d满槽准入、Direct目标隔离、MQTT换代与无缝里程碑（2026-09-08）

本批继续使用COM11 Hub、COM12 A、COM3 B及ADB手机，未重新烧录、重置板卡、清Key/Access、修改绑定或生产部署。
先修正旧测试工具，再进行三项有界真机门禁；未恢复100轮交替，也未借延长occupancy掩盖旧周期问题。

**实现与清理：**

- `edge_gateway_route_recovery_gate.cjs`移除针对共享topic的`GATE_CHILD_MISMATCH`断言，兄弟child的admission/execution/lease/closure/BBP/Direct原包直接交原处理者。
  Direct单独解码，不当成BBP；兄弟消息不消耗fault或进入重放缓存。恢复不再调用全Hub `removeEdgeGatewayRoute`，而使用当前捕获的exact operation/topology退休目标child。
  新runner从内部权威route读取非秘密目标，旧/缺失目标拒绝，无全Hub fallback；保留有界自动清理。33项工具测试通过。
- `edge_gateway_multi_child_app_gate.mjs`增加A专属Direct reservation/Ready测试及确定性释放；B一次Action无未知结果补发。
  `edge_gateway_permit_busy_gate.mjs`仅调用现有owner HTTP Open/read/Close，随机operation、最多4s状态轮询、请求超时；Open响应丢失也尝试exact Close，不选择候选、不创建拓扑。
  双child工具5项、permit工具3项通过。工具留在tests，不新增Sketch参数、产品测试HTTP接口、第二套MQTT或SDK宏。

**真实结果：**

1. 两child Ready且SDK两条native连接时，permit `pending_open → busy`，终态Close确认。
   拒绝前后SDK均两条连接，A/B安全/需求持续；随后并行各一次button/ACK，A 4→5、B 3→4。
2. A Cloud需求释放后，仅A申请Direct reservation并等物理Ready；A停止、Cloud reachable=false，B仍Secure/有需求且SDK仅余其一条连接。
   B一次button 4→5/ACK，A保持5，同一账户MQTT。未连接手机BLE，**不冒充完整Method 2/DirectSecure双child数据面门禁**。
3. 仅主动关闭exact Hub MQTT，Observer记录旧连接退休、新claims managementReady、两child重新Ready，账户MQTT未换。
   50ms轮询事件中：旧连接offline=1066ms，新managementReady=6642ms，A Ready=9751ms，B Ready=10653ms（均从observer起点计）。
   离线到两childReady约9.6s；之后A/B各5→6/ACK。未制造或自动重投未知Action；这是故障恢复短测，不是无缝/暖切换延迟。
   首次试跑因容器没有`kill`命令而未启动Inspector，未注入断线/发Action，owner已清理；修正为Node发送SIGUSR1后重新执行，失败记录保留。
4. 采样heapMin38,112 B、loop2172 B、ipc01104 B、NimBLE2688 B。无76 B重现；不能当SDK reset/长稳/叠加资源准入。
   串口部分行因CDC诊断丢字/截断，不用于逐包时序或完整无错误日志断言；业务成功以App/Broker/原生连接证据交叉确认。

**收尾与范围：**手机回首页、A/B UI scope=0、两个测试demand owner及Direct owner/permit均释放，Broker临时observer与主进程Inspector关闭。
A/B计数6/6，B credentialVersion仍null；原部署、Key/Access/拓扑不变。没有提交/推送/暂存。证据保存在忽略的`build/shared-s2f-{permit,direct,mqtt}*`。
本批生产三端代码未改，原Broker165/Service PG/App383是前批证据，不标为本批重跑。

**主线修订与下一步：**共享方案6.7明确“S4功能具备→S5体验通过”。S3不是给现有Bridge旁边再接一个WebSocket：
现有Bridge拥有唯一child callback、Cloud admission、BBP序号/在途映射，必须先拆出共享执行/需求与Cloud adapter，避免一个北向掉线清另一北向。
下批可立即推进S3a生产模块抽取和S3b有限本地授权合同/软件门禁；剩余管理renew/revoke交错、SDK reset与资源门禁在S4真机准入前完成。
不为了S3a源码工作等待24h/100轮，也不提前删除现行离线/Direct物理承诺。S3c/d本地纵向/有限离线→S4去周期→S5至少1Hub+2child体验；详见共享方案6.7。

### 10.109 通用WiFi局域网能力、mDNS发现与S3范围纠正（2026-09-09）

本批按用户要求先分析具体实现，核对三类产品、传输/鉴权/生命周期和App现有发现代码，并查阅官方平台/RFC资料。
新增[通用LAN/WebSocket/发现方案](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)，共享方案6.7/6.8、需求合同6.47和三端入口同步。

**架构决策：**LAN属于IP网络能力，不属于Gateway。普通WiFi、WiFi+BLE、Hub共用传输/安全/发现，Hub只增加child路由。
同一Client复用组件/Manifest/State；一个账户MQTT常驻，多child共用一条App→Hub LAN，南向执行/序号仍唯一。
首选mDNS/DNS-SD，SRV提供端口、TXT小型提示，不能用MAC/IP/名称建立身份，也不能由Hub被发现推断所有child附近/在线。

**前置问题：**当前Cloud Transport Error经WifiCloudLifecycle结束整个Client并stop WiFi；SMALL只有两个transport，STANDARD还同时增大frame等容量；
Client仅一个authorization provider/control endpoint，通用ArduinoClock无UTC。须分别解决故障域、容量ABI、权限分派和有限授权新鲜性，不能直接往旧Product加第三条通道。
Gateway Bridge不能再复制一份给WebSocket，也不能把ControllerGrant单设备凭据变更当成代理目标集授权。

**可复用基础与边界：**ESP32有ESPmDNS/NetworkServer，R4 WiFiS3有Server/UDP multicast，Wio有官方rpcmDNS；不等于本库LAN已编译/验收。
App已有capacitor-mdns，Android是JmDNS、iOS是NetService，V2尚未消费；旧`ws://IP:81` JSON接线需要调用审计后删除，不删除页面/编辑器或有效OTA功能。
WebSocket候选引擎需审查分片、慢客户端、分配和TLS server能力；首版安全profile/发现字段/容量尚未冻结，不追加不必要依赖。

**执行修订：**下一源码批次先IP/Cloud故障隔离、容量/鉴权分派与唯一child执行抽取，分别回归；S3b通用self授权及网关child扩展，
S3c先ESP32普通WiFi self纵向、同模块接WiFi+BLE/Hub，再R4/Wio各自门禁及S3d有限离线。S4才去周期并切后续请求，S5确认无缝体验。
首次在线授权、已认证会话有限离线继续的边界保持；不承诺冷启动完全离线或云端新撤权即时穿透断网。

**验证范围：**本批是源码/RFC/平台能力审查及文档一致性检查，不是构建/性能/真机结果；生产源码、插件、依赖、固件、Key/Access/拓扑均未改。
没有操作ADB/板卡或部署，也未暂存/提交/推送。原S2管理变更交错、SDK reset资源与发布长稳仍欠，100次交替不恢复。

### 10.110 S3a-1a 路径故障、peer退休、注册回滚与容量准入（2026-09-09）

本批从通用LAN方案进入生产代码前置收敛；不启动新的WebSocket/发现实现，不烧录、不改现场Key/Access/绑定，不跑100次交替。

**实现与清理：**

1. `WifiCloudLifecycle` 删除“任意运行期故障停整个Client”的接线。station/Cloud非鉴权错误分别保留NetworkFault/CloudFault和Degraded诊断，继续轮询其他transport；provider负责自身恢复，没有新增自动重试旧凭据。stop与启动失败回滚保留。公共WiFi和ESP32 WiFi+BLE使用同一个修订模块。
2. 全局审查修正了上一批过宽的“登录错误也一律独立继续”推论：provider把DeviceKeyInvalid/Rotated、Credential/Session/DeviceRevoked映射到同一AuthenticationRequired。尚无独立本地authority时仍AccessFault关闭，不借重构扩大授权寿命；S3b应消费已有wire细分类并验证本地scope，而不是另造错误码。
3. Runtime原生disconnect与transport非Online统一退休目标peer，补controller control window清理，去掉重复auth清理。相同sessionId在不同transport仍隔离，重复poll不重复退休；未停其他通道或清所有需求。
4. provider/control单注册入口拒绝不同owner覆盖，同指针幂等、停止后显式注销保留。BLE-only/WiFi+BLE attach只回滚成功取得的注册/transport，修复失败时误删他人对象。没有增加全局注册表、队列或新产品文件；此保护不冒充按通道分派实现。
5. Device默认ABI断言补transport/peer精确默认条件，防止自定义容量误用旧上限。生产SMALL仍2/2、frame256 B；新增独立构建参数与真实core容量测试，三槽预算单独验证，不用Sketch局部宏或切STANDARD。
6. 登录/composition freeze明确审查升v31，仅更新两个已改文件摘要；DeviceKey bytes、proof/session/MQTT wire、SDK默认和配置入口未变。Arduino诊断增加Degraded名称，保留既有状态数值。

**软件与资源证据：**

- Host/ASan完整53项（含20,000次确定性协议fuzz smoke）通过。新增/扩展：WiFi及Cloud故障后继续poll、Cloud/Local双向失败后另一通道继续发BBP、相同sessionId隔离、控制窗口退休、注册/回滚冲突、显式stop/恢复、AuthenticationRequired关闭及MQTT不再publish/connect。
- Win32 SMALL core与测试统一编译：2/2时Client668/Runtime404/TransportHub56/Device1664 B，3/3时748/472/68/1744 B；固定24-field ClientStorage824 B、WifiCloudLifecycle20 B均不变。嵌套对象不能相加；+80 B只是表结构，不含LAN/SDK/加密资源，也不是MCU堆/任务栈准入。
- 容量probe最初使用Runtime默认512 B帧，与SMALL测试工作区256 B不匹配，已显式匹配；随后3/3暴露默认Device断言适用条件不完整，按上项修正并补1744 B预算，不放宽默认1664 B门禁。
- 架构检查、修改源码whitespace检查及单Arduino库发布清单检查通过（347文件）；本批未生成发布ZIP。复现入口：`tests/host/README.md`、`test_resource_profile_main.cpp`；当前Host使用Debug/ASan，Win32容量树使用Release无ASan。
- Arduino编译：UNO R4 WiFi（Renesas 1.5.3）与Wio Terminal WiFi（Seeeduino SAMD 1.8.5）的CompilePublicWiFi均通过；R4 Flash136,880 B/static RAM12,968 B，Wio Flash140,048 B（CLI未提供静态RAM汇总）。日志在`build/lan-s3a1-wifi-platforms/<target>/compile.log`；外部SDK/依赖仍有编译警告，不能写成零告警。
- ESP32 Arduino 3.3.11/XIAO ESP32S3/default_8MB：WiFiQuickStart通过（Flash940,476 B/static RAM51,256 B）；WiFiBleQuickStart通过（1,280,192 B/55,492 B）；BleQuickStart通过（691,112 B/34,848 B）。连同R4/Wio WiFi，本批五个Arduino目标全部通过。构建树`build/lan-s3a1-esp32-3311/<example>`；这些是现有产品示例的编译资源，不是LAN增量或运行期最低堆/任务栈。
- 编译过程修正：先尝试旧matrix时发现其ESP32基线固定3.3.6，未改此基线冒充3.3.11；直接CLI使用已安装3.3.11。首次缺PubSubClient是隔离SDK配置未带共享libraries，改用已存在的依赖目录，没有新安装或修改SDK。首次WiFi编译发现诊断switch缺少Degraded分支，已补齐，最终WiFi重编译通过。

**主线下一步：**S3a-1b按transport/session鉴权/control分派与组合容量，1c唯一child执行/需求抽取并由Cloud adapter实际消费，再S3b有限授权与一个安全profile。
EdgeHub自身生命周期、BLE自身Fault对Cloud的影响与底层MQTT阻塞仍需单独审查；本批不把普通WiFi修正写成网关全部隔离。S3c普通WiFi self→WiFi+BLE/Hub→各平台门禁，S4去周期/Ready切路，S5体验确认；原S2管理交错/SDK reset资源与发布长稳仍欠。
App/Service本批仅文档同步，未改生产协议/代码或部署，未操作ADB/板卡或暂存/提交/推送。完整合同/表格以LAN方案§9与共享方案6.9为准。

### 10.111 S3a-1b 每通道访问绑定、协商和分派（2026-09-09）

本批完成通用LAN前置的多通道访问隔离，不提前实现WebSocket/发现/新安全协议；完整合同见LAN方案§10、共享方案6.10、需求合同6.49。

**实现与删除：**

1. TransportHub原有槽位增加`TransportAccess`两个非拥有指针，transport/auth/control原子添加/移除；移动和复用槽位同步所有绑定，运行期间禁止修改。无独立全局注册表、dispatcher类或无限队列。
2. Runtime删除两个全局成员及Client/Runtime的四个setter实现/声明，逐transport查找Hello功能/方法、协商交集、安全建立能力、Authenticate、Controller/PresenceKey和关闭回调。方法不匹配在进入provider前拒绝，sessionId仍是transport内身份；没有全局fallback。全量reset按provider去重，断开只清目标session。
3. BLE-only与ESP32 WiFi+BLE正式composition直接绑定；删除分步auth/control回滚。BleOnlyRadio删除多余client指针及手动析构接线，由lifecycle在停止后移除本transport；detach不清其他对象，运行期不支持热销毁。
4. 普通MQTT的Hello不再误声明本地BLE认证/ControllerControl/PresenceKeyControl。DeviceKey bytes/proof/session wire不变；AuthenticationRequired包含撤权的原安全关闭仍保留，未授权LAN不能借用BLE/Cloud权限。

**验证：**

- Host/ASan53项通过（含20,000次确定性协议fuzz smoke）；新增Cloud+两个Local真实Runtime/Hub、假IO/假鉴权的独立方法/权限/安全能力、同sessionId不串权、Controller/PresenceKey分派、错方法/未知session拒绝、局部/native关闭、剩余路径可控、重编号/复用无残留、重复provider reset去重、启动回滚及BLE析构解绑。
- 原Hub用例曾允许运行时remove，已修订为先验证拒绝，再stop/remove；不通过弱化锁修复测试。原双传输BBP业务回归保留，安全退休断言移入明确的三通道access用例，Cloud不再伪挂本地provider。
- Win32 SMALL2/2：Client676/Runtime396/Hub72/Device1672 B；SMALL3/3：764/464/92/1760 B；STANDARD3/4：944/644/92/3460 B；LARGE4/8：1888/1564/116/11004 B。固定24-field存储824 B、Wifi lifecycle20 B不变。相对上一批SMALL增加8/16 B；精确默认门禁1672/3464/11008 B，SMALL三槽1760 B，默认容量/帧/schema未增加。
- 以上对象嵌套不能相加；不是WS/mDNS/加密/SDK socket和最低堆/栈证据。复现入口为tests/host/README.md，ABI仅运行专用容量目标，不宣称Small完整运行53项。
- 架构、源码whitespace与单Arduino库发布清单检查通过（347文件、无新增生产文件/依赖）；登录freeze经审查升v32，仅更新ESP32 WiFi+BLE绑定实现摘要，不绕过检查。SizeProbe删除与Device.h重复的Device上限断言，使用同一精确默认门禁，保留真实大小符号和其他模块门禁。
- ESP32 Arduino3.3.11/XIAO/default_8MB：WiFiQuickStart通过（Flash941,052 B/static RAM51,264 B）；WiFiBleQuickStart通过（1,280,404/55,500 B）；BleQuickStart通过（691,376/34,848 B）。构建树`build/lan-s3a1-esp32-3311/<example>`；不是LAN增量或运行期堆/栈。
- Arduino矩阵：UNO R4 WiFi/Renesas1.5.3通过（137,296/12,976 B），Wio Terminal WiFi/Seeeduino1.8.5通过（Flash140,464 B，CLI无静态RAM汇总），UNO R4 BLE通过（193,196/16,960 B）。连同ESP32，本批六个产品目标全部通过。依赖/SDK仍有警告，未写成零告警；日志`build/lan-s3a1-wifi-platforms/<target>/compile.log`。最初误用不存在的matrix目标名被参数校验拒绝，已使用现有`uno-r4-product-ble`，未改矩阵放宽准入。
- UNO R4 SizeProbe编译通过（探针整体Flash52,416 B/static RAM14,976 B，不是产品占用），ELF大小符号实测Client676/Device1672/Runtime396/固定24-field存储824/Wifi lifecycle20/BLE lifecycle48 B，确认SMALL与Win32 ABI相符；`build/lan-access-abi-arduino/uno-r4-size-probe/SizeProbe.ino.elf`，以工具链nm的`--print-size --radix=d`读取。未烧录或测运行期最低堆/栈。

**下一步：**S3a-1c抽取唯一child执行/需求并由现有Cloud adapter消费，保持南向连接/序号/回调单一所有权；再S3b通用self有限授权和child扩展，S3c实际通用LAN。EdgeHub生命周期/旧周期、BLE自身Fault是否影响Cloud及底层MQTT阻塞仍须分别处理。S4功能/S5体验与剩余S2硬件门禁不变，100轮继续暂停。
本批App/Service仅文档同步；未真机、烧录、部署、改Key/Access/绑定或暂存/提交/推送。

### 10.112 S3a-1c 唯一 child executor 与 Cloud adapter（2026-09-09）

本批完成LAN接入前最后一项南向所有权拆分；没有新增WebSocket/mDNS、安全wire、公开产品API或第二条child链路。

1. 新增`BasicGatewayChildExecutor`，每个活动child只有一份。它独占child receiver/session/fault回调、DirectSecure连接、
   南向BBP/2序号、单个在途exchange、最近可重放exchange、ControllerControl事务和一份512 B帧改写workspace。
   Cloud与未来LAN以owner指针提交；另一owner在途时返回`StateConflict`，不会排队、分配新序号或覆盖响应接收者。
2. `GatewayChildRouteBridge`收缩为Cloud adapter：只保留route admission、Cloud execution/direct-yield lease、投影代次、
   Cloud correlation/digest和envelope发布。Cloud downlink实际调用executor；ACK/ERROR/Manifest/State响应由executor按原owner
   返回，再映射回原Cloud sequence。旧Bridge中的child回调、child/connection引用、child sequence与control状态已删除。
3. `BasicGatewayGattChildContext`显式按`session -> connection -> executor -> proof -> Cloud adapter`持有对象。
   revoke/renew协调器与management response直接依赖executor，不再借Cloud adapter访问南向管理通道。
4. 需求为executor中的固定bitset：Cloud与Local可独立置位；Cloud reset/route retirement只清Cloud位和该adapter拥有的
   pending/recent映射，不重置child序号、DirectSecure session或未来Local需求。真实SDK disconnect才清全部南向exchange并
   重置该child会话序号。现有radio仍通过Cloud adapter门面读取聚合需求和Direct-yield约束；S4才删除旧周期策略。
5. executor复用迁出的原Bridge 512 B workspace，没有增加第二份frame buffer、动态分配、任务或队列。新增架构门禁禁止
   Cloud adapter重新安装child回调/序号，并要求context只持有一个executor。

**验证与资源：**Host MSVC/ASan 53/53通过；新增双owner用例验证Cloud+Local需求并集、单在途背压、失败请求不消耗
sequence、ACK只回原caller、recent replay复用原sequence及后继请求单调分配。架构检查与单Arduino库发布清单通过
（348文件）。ESP32-S3 Arduino 3.3.11 `CompileEsp32EdgeHubInternal`通过，Flash 1,450,448 B、静态RAM75,228 B。
与同一SDK/config的上一构建相比ELF text+data增加1,924 B、`.dram0.bss`增加88 B；活动child context由7,512增至
7,552 B（+40 B/child），executor本体580 B（含迁入的512 B workspace）；当前两个活动槽贡献+80 B，
其余8 B来自产品对象对齐/接线。SDK/网络基线与§10.111不同，因此不拿75,228与较早66,428跨配置相减。
未烧录、部署、改Key/Access/绑定或运行真机/100轮。

**下一步：**进入S3b，只冻结并实现一种通用self有限本地授权及exact child扩展；随后S3c先接普通WiFi的
mDNS+认证WebSocket，再复用到WiFi+BLE/Hub。Manifest/State事实共享、多个上游observer mux及S4去周期在后续批次完成，
不能把本批executor抽取写成LAN已经可用或无缝体验已经实现。

### 10.113 S3b-1 通用有限本地授权核心

已完成载体无关的canonical CBOR challenge/grant、DeviceKey派生的流式HMAC和独立域
`Noise_NNpsk0_25519_AESGCM_SHA256` PSK。wire不传DeviceKey、Gateway secret、ControllerCredential或PSK；
15 s challenge、1～300 s lease均由接收方单调时钟约束，相同authorization重连不续期。Key/ACL换代、重启、重放、
篡改、错误child和第三个授权目标槽申请均拒绝（§10.114纠正：不是按用户数量计槽）。

产品级`LocalAccessAuthority<2>`统一Key/ACL事实，每个未来socket只持challenge session；普通设备使用self policy，Hub
通过同一接口注入exact current child target，不复制executor或南向连接。32位实测authority 324 B、challenge session 48 B、
admission 168 B；三平台SizeProbe与Host golden/fuzz通过。当前未把授权接入GATT child执行需求，也未实现Service/App/WS/mDNS；
下一步先完成Service签发和App HTTPS消费，再接普通WiFi self socket，不能据此恢复100轮或宣称无缝完成。

### 10.114 S3b-2a 跨端授权模块与下一步权威接线（2026-09-09）

设备/Service/App使用同一LocalAccess challenge/grant黄金文件；Service签发核心与事务port分离，App消费类与BLE/UI分离，
普通self和Hub exact-child复用代码。清理Host重复黄金值、抽出BLE内通用Base64URL。
过期lease保留挑战窗口tombstone，阻止第一条拒绝清槽后第二条fresh response复活旧授权；Win32结构大小不变。
组件级受限分享无法映射whole-target Control时拒绝，不能悄悄扩大权限。

Host/ASan54、20,000 fuzz、协议25、Service签发/HTTP12、App19文件204项及TS/架构/单库354文件检查通过。
没有真实PG/HTTP注册/WS/mDNS/真机证据；未部署、烧录、重置、暂存、提交或推送。

下一批按LAN§13及`aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md`补持久authority/幂等期限、
真实ACL/Key/拓扑事务和精确撤权。不能用全局revision假装仅撤A、不影响B；2槽也不代表2caller任意访问多child。
S3b-3补单socket多目标授权与满槽续期策略，再S3c普通WiFi纵向联调；S4持续路径切换/S5体验门禁不提前标为完成。

### 10.115 S3b-2b1 持久权威、签发事务与精确撤销事实（2026-09-09）

Service增加migration 027四表与独立repository，普通WiFi self和Hub exact-child共用签发事务与ACL策略。
新鲜已验证DeviceKey session激活建立LAN instance/revision，不从MAC或历史session补身份；同instance重登不升代，
新instance/Key换代退役旧授权。代际饱和关闭LAN但不阻断普通MQTT登录，不引入新的硬件绑定。

请求幂等、绝对期限、不可变scope、签发后禁止自动重试、临时Key清零和跨Node进程恢复均通过真实PG测试。
发现并修复真实竞态：签发使用READ COMMITTED时，仅行锁不足以让更早开始的SERIALIZABLE分享事务看见新授权；
现使用SERIALIZABLE/SSI及签发前有界重试。ACL/Key/设备/拓扑/tenant变更同事务写精确退休与outbox，未重建权限缓存。

每recipient64个活动授权、每授权64个request为服务器容量上限，不是MCU槽位；不可延长授权期限。
提供按保留期有界清理，尚未挂调度。真实PG新增13子测试+父测试；连同现有登录、分享、网关访问/退休和签发HTTP，
联合30项通过、无跳过；Gateway TS通过。只使用唯一隔离schema，未改变原账户/绑定/正式schema或部署服务。
App/MCU源码本批不变，不重复计入旧Host/App门禁；未刷板、暂存、提交或推送。

**仍未完成：**outbox送达设备、匹配ACK、authorityRevision认证同步、尚在传输中的旧grant退休、满槽无缝续期、
单socket多目标及实际WS/mDNS。数据库撤A保留B不等于真机B不中断；HTTP工厂仍未注册。
下一步按LAN§14与Service计划§3.2执行S3b-2b2，再S3b-3/S3c；S4功能/S5体验和发布长稳仍单独验收。

### 10.116 S3b-2b2a 认证控制与精确退休接收核心（2026-09-09）

新增通用Control codec/设备receiver，Service协议包同形编码/认证与exact ACK匹配；没有新增网关专属身份、密码库或MQTT。
Hello/Command/ACK共用≤112 B canonical数组，独立`BLINKER-LAN-ACCESS-CONTROL-MAC/1`域，命令与ACK不可反射。
controlSession从本次运行随机生成，普通MQTT重连不重建receiver；唯一主循环应用严格递增序号，ACK重传不重复状态变更。

精确撤A时删除A授权、推进pending challenge代际，覆盖“旧grant尚未到达”的情况；B已建立lease的权限/期限保留。
其他尚未完成的握手需新挑战，不承诺所有pending无影响，也不引入固定等待。Disable保留revision屏障，不可同代重新Enable。
复用grant的流式HMAC/HKDF基础，未新增撤销黑名单/secret表；检查ACK缓冲区容量及输入输出重叠，失败不应用命令。

软件门禁：Host/ASan54项、20,000确定性fuzz、Service协议26项和Gateway TS通过；架构/357文件单库检查通过。
Win32 authority328 B（+4）、challenge52 B（+4）、新增control receiver84 B，lease144/admission168不变。
两挑战配置较前批模块ABI合计+96 B，不等于普通QuickStart静态RAM自动增加，也不是MCU运行资源验收。
ESP32/UNO R4/Wio SizeProbe交叉编译通过，显式实例化全部receiver方法并保留测量符号，三平台ELF均确认328/52/84 B。
UNO R4整份探针RAM15,752 B/32 KiB；SDK原有警告保留。此为编译/ABI证据，未运行设备上的控制链路或测量运行时栈。

当前receiver ACK只证明授权状态已应用；产品仍需把业务准入/转发/需求消费接入同一authority，不能把它解释成物理断开或撤回已执行Action。
下一批S3b-2b2b补每recipient一个持久在途命令、exact重发/ACK与重启恢复，再Broker/共用产品控制入口及清理调度。
未改App生产代码/UI、部署、刷板、改绑定或暂存/提交/推送；生产LAN、S3b-3/S3c/S4/S5仍未完成。详见LAN§15、Service计划§7。

### 10.117 S3b-2b2b1 持久控制、精确ACK与新鲜Hello（2026-09-09）

Service新增独立`local-control-repository`及migration028：一个recipient一个pending，命令不可变、先提交后返回发送字节，
只有exact ACK才推进checkpoint并完成对应outbox。同boot重连/新云登录复用旧命令；新boot放弃旧pending不等于退休成功。
更高revision或身份/Key屏障有认证ACK才能把旧记录记为`superseded`，保留证据引用、区别于`delivered_at`。
签发在同步/退休尚未恢复时返回`LOCAL_ACCESS_SYNC_REQUIRED`，无旁路；新增批量完成日志清理，保留pending和仍被引用的ACK证据。

发现并纠正旧Hello仅证明持Key、不能证明当前启动会话新鲜的问题：Hello MAC追加当前云登录proofSessionId16，
不加wire字段、不改Command/ACK字节、不增加MCU槽。接收器`hello(proofSessionId, ...)`要求云登录结果；
Broker后续传已认证session上下文，不接受App/消息体指定。候选context-free Hello移除，无双协议兼容分支。

真实PG联合34项通过（LocalAccess17个子测试+父测试，其余登录/ACL/网关及HTTP），包括并发准备、独立进程恢复、回滚清Key、
Hello水位不冒充ACK、旧Hello/ACK隔离、撤A保B、旧pending先补ACK再升revision、覆盖证据及序号/清理边界。
真实PG曾暴露并发旧快照唯一冲突，已改为有界事务重试且回归通过。Host/ASan54、20,000 fuzz、协议26、Gateway TS、
Win32 ABI与架构/357文件单库检查通过；receiver84/authority328/challenge52 B不变。
本批未重跑三平台交叉编译、不冒充Broker/MCU实机ACK；App仅文档同步，无部署/刷板/重置或暂存/提交/推送。

下一批S3b-2b2b2：有界持久attempt/退避worker→Broker/普通WiFi与Hub共用入口→主循环应用/主动到期释放→真实控制面故障门禁。
完成后才开HTTPS；再单socket多目标/满槽续期、WS/mDNS、S4持续路径与S5体验。详细顺序见Service计划§8、LAN§16。

### 10.118 S3b-2b2b2 有界投递、重试恢复与清理核心（2026-09-09）

新增migration029的单command投递记录，签名字节仍不可变。`claimNext`持久预约后才允许发送；外层DB事务中拒绝领取，
避免嵌套SAVEPOINT释放后仍持锁发布。每当前proof最多8次、6/12/24/30秒封顶退避、5秒carrier取消；重启/旧Hello不清预算，
耗尽保留pending及新签发屏障，迟到exact ACK仍可完成；新已验证登录+Hello才重新开启该proof预算，累计计数不降。
发布结果按command/proof/attempt隔离，不能覆盖新尝试或ACK；这不是业务消息限流周期或新切路空档。

`local-control-worker`注入发布函数、复用既有有界scheduler；每分钟各清32条过期授权/无引用完成命令，清理失败独立隔离。
共享scheduler补超量列表截断和非有限间隔保护，删除仅依靠list自律控制批次的假设；不增加MQTT、权限副本或内存重试Map。
拒绝用Promise.race抛弃仍在发送的网络操作；载体须遵守abort结束I/O，真实载体取消仍待验。

真实PG联合47项、后台worker/失效任务16项及Gateway TS通过；第二条DB连接验证publish前预约已提交、原行锁已释放，
测试载体发布过程中可接受ACK。覆盖崩溃、提交失败、并发领取、耗尽、换proof、迟到结果/ACK和有界清理。
本批不改设备/App生产代码或wire，无新Host/跨平台编译/真机证据；未改正式schema、账户、Key/绑定，未部署/刷板/暂存/提交/推送。

剩余按Service计划§9.2：真实Broker载体及worker start/stop→普通WiFi/Hub产品队列与ACK→自动新证明恢复（禁止要求用户重置）→
主动到期/权限消费与真实控制面故障门禁，再开HTTPS和多目标续期/WS/mDNS。S4/S5无缝未完成，100轮仍暂停。

### 10.119 S3b-2b2b2 Service/Broker载体、取消与可信上行（2026-09-09）

普通WiFi与Hub不共用Hub专属management图，而共用原MQTT数据连接/两个topic；独立模块持有有界I/O，不拥有第二份权限或重试日志。
新增0x36固定10 B BBP头、flags/sequence=0，body仍是exact控制CBOR；0x34/0x35保留给BLE PresenceKey，不可复用。
MCU仅同步enum和静态合同检查，产品接收、SDK队列与ACK尚未接线；不新增Sketch参数或开启功能声明。

Service内部控制HTTP、Broker认证claims→Service relay、在线current proof下行和唯一start/stop已注册源码。
Broker最多8下行/16上行，每连接各1；上行4/s、既有HTTP3秒绝对期限，下行写3秒期限，Service5秒I/O窗口。
取消卡住的写只销毁捕获的socket，迟到字节/ACK不能当作未执行；单次HTTP接受不是应用ACK，PG仍唯一持久预算/ready来源。
一起修正4xx误触发共享鉴权熔断、null路由误转shard0、过大HTTP响应未释放和控制调试日志脱敏。

真实PG联合50项、Broker专项7项与既有165项回归、完整Broker脚本/迁移HTTP客户端/两端TS通过。
Host/ASan54项及20,000确定性fuzz、设备架构/357文件单库清单通过；本批未重跑三MCU交叉编译或任务栈测量。
网络证据是临时loopback HTTP→Aedes→TCP MQTT模拟器，不是MCU或四端真机；未部署、刷板、改绑定/Key或暂存/提交/推送。
详细合同见Service计划§10；原接收器ABI未增加字段，不把enum或Host检查算作运行时资源/跨平台验收。

下一批接普通WiFi/Hub唯一产品控制队列、真实登录proof、Hello初发/丢失/重连及预算耗尽的自动限频恢复，
再同authority消费/主动到期和真PG→Broker→MCU故障门禁。之后才开App LAN签发、S3b-3同socket多目标/满槽续期、S3c WS/mDNS、S4/S5体验；100轮暂停。

### 10.120 设备控制组合层与首次同步恢复（2026-09-09）

按LAN§19先收口设备控制，没有跳到WS或新网关协议。可选`LocalAccessCloudTransport`复用原MQTT，112 B单inbox、主循环签名验证/应用/ACK；
普通WiFi/WiFi+BLE/Hub通过共用stack类型组装，默认不实例化此扩展。成功登录proof现保留16 B，刷新成功才换代，失败/stop清空。
boot控制会话跨普通重连保持；有限Hello与首次authority未应用时的限频新proof恢复已实现，主动到期/回绕锁定有Host覆盖。
Win32 provider296 B、单槽wrapper452 B；原lease144/双槽authority328/challenge52/receiver84 B不变。Host/ASan54及20,000 fuzz通过。

**完整恢复与真机仍未完成**：已配置后的正常空闲与后续命令全部丢失不能靠无消息判定；需要最小的、绑定当前proof和新鲜请求的进展/恢复依据。
不能借`revokeSession()`强行恢复，它会触发authority全局失效并可能扩大精确撤权。下一步先补该缺口，再真PG→Broker→MCU闭环，然后普通WiFi LAN。
当前内部开关只启用self控制载体，不开放LAN/child授权/生产HTTPS；无Service/App源码变更或部署/刷板。跨平台编译与完整边界见[LAN§20](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。

### 10.121 进展回执与投递耗尽自动恢复（2026-09-09）

新增Query6/Status7，独立HMAC域绑定当前proof、boot与随机请求nonce，原0x36上限不变；设备以此替换旧产品Hello/bootstrap定时器。
Service共用原receiver checkpoint与PG投递预算；只有exact命令ACK完成工作。Broker将HTTP携带的Status回给同一物理MQTT client，不建新连接或队列。
每轮最多4次查询/10秒期限，稳定连接60秒一轮；有签名恢复要求或240秒无进展时沿原登录限频取新proof，最短300秒尝试间隔。
恢复不调用revokeSession、不清authority/B；真实PG验证A耗尽后新proof第9次投递仍为原字节、ACK完成且B不受影响。
软件证据：设备Host/ASan54及20,000 fuzz，PG/HTTP/worker31，协议27与Broker专项8、相关Broker联合173及TS；ESP32 WiFi和R4 WiFi实际编译。
单槽wrapper488 B（+36），provider296/receiver84 B不变。具体大小、时序和复现以[LAN§21](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)与[Service§12](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)为准。
下一步真实控制闭环，再普通WiFi安全WS/mDNS；本批未部署/刷板或修改App源码，不重计前批五组合编译/历史真机证据。现场WiFi已变，参数需重核。

### 10.122 真实控制闭环、启动Query竞态与恢复（2026-09-09）

本批实际部署开发Gateway/Broker shard 1与027/028/029迁移，更新mostfun2026网络下的本地IP/TLS证书；
备份后只烧录COM11 Hub应用区，COM12/COM3仍为A/B，NVS/Key/拓扑不重置。
真机发现Query可能先于BBP Hello，Broker误判SESSION_STALE并触发退避；改为合法当前设备Query在Hello前计限额后丢弃，
不relay/不开放下行/不置ready，复用设备原重试。内部旧Bridge诊断改读Executor，不新增兼容层。

Enable→Disable→Enable均真实exact ACK入PG。精确Retire在220秒ACK提交故障下耗尽原proof的8次预算，自动新proof仍保持原boot/sequence/bytes，
第12次累计发送后入账；另一测试LAN scope不被退休。Broker重启及MCU重启恢复通过，旧Key版本下行在真实Broker被拒。
此处scope未安装为LAN caller，未真正刷新Hub Key，故不能宣称WS断流/旧登录拒绝/无缝已通过。A/B仍Active5，故障trigger/function/table已清除。

Hub实编译Flash1,467,952 B/静态RAM75,820 B；Broker专项9和关联174项通过。空闲ipc0水位不替代BLE并发资源门禁。
完整证据、故障语义、回滚及剩余门禁统一见[LAN§22](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#22-真实控制闭环与启动query竞态修正gatt-10122)。
下一步是普通WiFi单caller/self正式安全LAN纵向接线，不新开一轮控制协议准备，也不先跑100次或扩展更多网关协议。

### 10.123 单self LAN安全传输与业务权限（2026-09-09）

`LocalAccessFrameTransport`在一个可注入binary消息端口上完成Challenge→exact Grant→LAN Noise NNpsk0→原BBP Runtime，
仅self，400 B默认帧/一个待发record/3秒写期限/15秒准入；精确重试不推进nonce，撤权/期限/Key/物理换代清会话不清其他产品lease。
Runtime本地authenticated分支新增transport会话权限上限，入站控制与主动状态发送均检查，不能把Observe-only变成全权。
App新增最小可取消会话，无UI/MQTT/账户改动；晚响应清理、单发单收与期限关闭已有测试。原Noise假体抽为共用测试支持，不增加生产密码实现。

Host/ASan55+20,000 fuzz、App关联72测试、TS/lint通过；Win32/R4/ESP32 transport448 B另加caller-owned820 B缓冲，原生wire RX/SDK成本另计。
实际WS/mDNS/公开签发与产品extension还未接入；候选WebSockets库的阻塞读取/按报文分配不能靠薄包装自动满足预算。
完整合同、资源/交叉编译和下一批顺序以[LAN§23](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#23-单self安全消息链路与runtime权限接入gatt-10123)为准。
本批无烧录/部署/身份重置/提交推送，Hub/A/B继续保留原角色；不重跑100次交替来替代实际LAN接线。

### 10.124 SDK监听器与普通WiFi可选扩展（2026-09-09）

`Esp32LocalWebSocket`复用SDK HTTPD，补有界HTTP/WS准入、固定mailbox、loop-only短写/期限、网络代际；
`Esp32LocalDiscovery`独立管理自身mDNS service，随机locator不是身份，不停全局mDNS。普通WiFi经template extension加入同一Client和authority，
Cloud先poll撤权、Local再flush；默认用户API和依赖不变，没有第二个MQTT/状态表或新ZIP。

57项Host/ASan、20,000fuzz和ESP32/R4关闭候选的编译通过；实际端口也在本机ESP32 Core3.3.11交叉编译。
必须另计HTTPD6,144 B/mDNS4,096 B任务栈、双RX/TX及socket/密码峰值；SDK假体并非线程/无线硬件门禁。
登录freeze v34只审查更新两处产品接线hash，wire/provider/Key/golden不变。具体预算、端口边界和下一批以[LAN§24](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#24-esp32实际监听器mdns与普通wifi扩展gatt-10124)为准。
本批App/Service只同步文档，未部署/刷板/清身份/提交推送。下一步原生App+公开签发后做隔离普通WiFi MQTT/LAN真机，不破坏现有Hub/A/B。

### 10.125 公开签发与Android原生消息端口（2026-09-09）

Service已将原issuer接到精确设备路由，单self、复用同事务/ACL/Key/ready/退休仓库和唯一worker，默认开关关闭。
TLS只认真实socket或明确可信终止peer，现有本机TLS透传工具不能当作可信HTTP代理。
Android新增分离的地址规则、受限WS draft、单socket消息端口与network/foreground bridge；TS适配原LocalAccessMessageLink并复用mDNS。
真实JVM socket负测补正了上游decoded frame丢失mask信息的边界，原始头检查不另造codec；保持420 B/有界片数/期限、取消和迟到清理。

App关联232、Android全部JVM16（LAN8项）、Service正式路由/隔离PG28和签发/载体专项24项通过。
本批未改变设备源码、原账户MQTT、UI或Hub/A/B身份；未部署/安装APK/刷板/暂存/提交/推送。
原native/client尚未挂到页面/账户/网络/BBP owner，不把原生端口软件通过当作LAN业务完成。
下一步严格按[LAN§25.4](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#254-下一批顺序取代245中已完成的端口任务)完成可信HTTPS与原owner接线，再普通WiFi隔离身份真机和资源/撤权/Key刷新负测。

### 10.126 可信HTTPS与共享self BBP会话（2026-09-09）

纠正§10.125“原Session”歧义：账户MQTT Session带Broker Route/Delivery，不能直接接LAN设备Runtime。
BLE业务引擎提取为唯一`DirectDeviceSession`，BLE保留凭据/PresenceKey专属操作；LAN复用该核心和原Store/codec完成安全open后Hello/Manifest/State。
Hello显式controller/transport两种安全前置，BLE Method 2不变，LAN不重复鉴权；协商帧大小、2个有界TX位置、不确定写关闭与原序号重传已测。
native关闭结果逐层返回，失败不能伪报物理释放；Service开发HTTPS解析并覆盖scheme，固定目标/有界流式转发，MQTTS保持独立二进制载体。
App250项/App及spec TS/变更lint、Service24+4项通过；设备只改Host Hello输入fixture，产品源码/身份/拓扑不动。
尚未挂DeviceUiPort owner或部署HTTPS/手机证书，不将内部session软件闭环当作App无线可用；继续[LAN§26.4](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#264-下一批可直接执行的主线)，不恢复100轮交替。

### 10.127 原页面owner与可信self身份接线（2026-09-09）

修正App设备目录缺少可信instance的缺口：同一HTTPS路径GET复用实时ACL/authority/exactACK屏障返回最小身份与整机权限，不解密Key、不增加授权表。
纯端口`LocalAccessConnector`、Android小组合层与原DeviceUiPort负责各自生命周期；原账户MQTT并行准备，verified-ready后首选LAN。
页面/账户/网络/后台退休关闭本地，不跨路重发未知Action；Observe/组件限定写走cloud，State/Event和verified Manifest走原UI接口/账户缓存。
有界发现流式消费，单native槽清理失败不重开；普通发现失败成功清理后可在未来页面重试。
Service专项17/隔离PG+路由30项通过；App最终项目回归51文件/507项、App/spec TypeScript与变更lint通过，详细证据见LAN§27/App§6。设备源码/SDK/身份/拓扑未改，当前SSID“带翅膀的蜗牛”，密码仅限本地临时配置。
下一步部署隔离backend与手机可信HTTPS，安装本地测试APK，再普通WiFi隔离身份真机。公开与App开关仍关闭，不增加100轮、网关多目标或LAN telemetry；无缝仍是S4/S5。

### 10.128 本机部署与隔离普通WiFi首轮真机（2026-09-09）

部署了可信peer精确准入的HTTPS及独立账户WSS；手机debug-only CA、Release manifest隔离和账户WSS ready实测成立。
真实App发现local-access漏带Bearer，已统一URL/原鉴权路径并以95项回归验证；未另写LAN登录层。
ESP32 SDK双栈listener的v4-mapped地址被原IPv4-only sockaddr误拒，已规范化，Native WS观察114/251 ms打开；Host Debug/ASan和SDK编译通过。
后续安全Challenge receive仍15秒超时，没有LAN-ready/Property/Action证据；App账户MQTT未换session，但设备MQTT曾重连，不能声称双路稳定。
COM12完整8 MiB备份后隔离测试，结束已精确恢复原NVS/OTA/app0并校验hash；原Hub/A/B服务器拓扑不改。
执行位置、证据/限制和下一批诊断链见[LAN§28](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#28-本机部署真实手机准入与首轮wifi测试gatt10128)。不增加100轮、网关多目标、页面编辑器或新协议。

### 10.129 默认明文修订与SDK兼容收尾（2026-09-09）

用户明确LAN参考普通MQTT，默认明文、可选加密。按[LAN§29](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#29-默认明文修订与可选加密路径收尾gatt10129)执行P0最小模式/鉴权合同→P1共享设备模块→P2原App/Service接合→P3普通WiFi真实闭环；基础授权/期限不取消，加密模块保留可选，禁止静默降级。现有LAN代码仍是加密候选，不能把文档决策记为Plain已经实现。
HTTPD实际IDF5.5.5在Upgrade后不调用旧GET handler，port现通过公开WS状态幂等提交session；保留SDK HTTP parser和请求/资源负测。ESP32 GCM后端空长度仍要求非空指针，仅port修正，实机空payload/两种AAD/坏tag已知向量通过，不修改SDK或维护新密码库。
实际手机经mDNS/native WS、HTTPS Grant、Noise到Hello/Manifest/State Ready，Property ACK和状态更新通过，账户MQTT保持同一session。无参Action的门禁脚本误传整数而被编码层拒绝，改null后尚未真机复跑；没有Action完成或无缝声明。
Host端口Debug/ASan与真实SDK原始socket门禁9/9通过；最终固件Flash1,129,564 B、静态RAM57,432 B，运行heap约195 KB/loop栈3472 B，只是局部观察，不是加密净成本或全部任务准入。
COM12恢复原app0及本轮测试前NVS/OTA，写后hash通过；Hub/另一child、原Key/拓扑不动。临时trace清理，代码保留必要测试。App/Service执行计划同步，当前不扩BLE/Gateway新协议、100轮或页面编辑器；Hub云端秘密交付MQTTS不因LAN默认Plain自动取消。

### 10.130 P0 Plain合同与P1记录模块解耦（2026-09-09）

三端共用新profile 2及37 B双向证明golden，原profile 1字节不变；复用现有有限Grant、authority、ACL和期限，不增加所有者记录或另一套协议业务核心。
设备`LocalAccessFrameTransport`改为注入Plain/Noise会话，队列/鉴权/Runtime只有一套，Plain不保留Noise状态。Runtime明文准入需要transport显式认证能力与实时权限，不能凭单个authenticated标志放行业务；原敏感变更仍要求加密。
ESP32内部LAN默认为Plain，mDNS提示随模式，LAN总开关仍关；旧App/Service签发暂时严格拒绝Plain，下一批才演进原HTTP接口和byte session，不把协议包可解码等同完整功能可用。
本批Host/ASan及三端专项通过；SDK对照编译、资源数字与明确未测边界统一见[LAN§30](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。未部署、刷板或改Key/绑定；先P2接线再P3真实self闭环/动态资源，随后才WiFi+BLE/Hub及暖切换。

### 10.131 P2可信模式与有限授权重开（2026-09-09）

复用原proof绑定的Progress，v2只增加受MAC保护的profile，最大105 B仍用112 B控制body；v1只表示Noise。Service原receiver表增加可空模式列，当前Key/proof认证Query才声明，同boot拒绝变更、新boot按原登录屏障重新声明；精确ACK和原ACL/撤权不能被模式声明替代。
原GET/POST统一HTTP v2，返回sessionKey及原授权剩余期限；旧Noise专用wrapper/消费已删除，两模式共用事务/权限。App只拆记录算法，socket/期限/DirectDeviceSession/队列/ACK和账户MQTT仍各一份。同账户进程生命周期复用公开caller ID，缓解快速重开撞单槽，授权不续期；进程重启/账户变化等单槽行为仍待真机。
App88项、协议29项、Service专项27项、真实PG25项、Broker9项及Host/ASan/Win32通过；SDK3.3.11 Plain与Noise静态RAM仍56,600/56,832 B，Flash1,027,812/1,036,736 B。日志、具体范围和未测边界统一见[LAN§31](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。未安装APK/部署/刷板或改原绑定，下一批P3先Plain self真实Property/Action(null)/ACK与有限生命周期/资源，再同脚本Noise，后续才WiFi+BLE/Hub/S4。

### 10.132 P3 Plain真机与受阻ACK定位（2026-09-10）

本机Gateway/测试账户broker-shard-1、migration030、debug APK和隔离COM12固件配套更新后，取得真实profile2声明、HTTPS v2、Plain证明、Hello/Manifest/State及Property/无参Action ACK。五次开页、三组控制，Action计数1→2→3；快速重开复用同authorization和原期限，账户MQTT持续同一session。有限期限附近回Cloud有观察，精确撤权/Key刷新仍待实测。
有状态变化时ACK约1.09～1.25秒，无状态变化约112/177 ms；抓帧确认先收到Patch，约一秒后同Command序号重发才收到ACK。根因是公共Runtime先settle/fan-out状态占满单槽，初次ACK受阻后依赖peer重试，不是加密或重复执行Action。下一批先补有界待发响应，保留原执行结果/关联、动态鉴权和去重，再复测原默认重试间隔；不扩大队列或缩短App计时掩盖。
门禁资源和恢复hash统一见[LAN§32](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)；13任务快照中ipc0/ipc1最低余量1284/1276 B，httpd3340 B，完整过程heap最低144,576 B，只是当前Plain/Cloud组合的观察，非长期准入或加密净成本。COM12已恢复本轮原应用/NVS/OTA，其他板/原绑定未改，新测试服务及APK保留。
备份脚本误选账户库的事实及迁移后补存纠正见同节/Service§23，不能伪称Gateway迁移前备份。本轮未修改生产Runtime来修ACK，也未跑旧全套测试冒充新证据；P3仍是当前主线，未进入100轮或Hub/S4扩展。

### 10.133 P3公共ACK补发、Plain复测与扫描退休测量（2026-09-10）

公共Runtime复用原去重枚举记录AckPending，成功Command/Event/Patch仅WouldBlock才自动补发；每在线transport每poll最多一次，检查原session和实时Control权限，不新增帧队列/静态RAM，不改变协议或App1000 ms重试间隔。保留原Patch执行revision、同帧重放和状态通知；Error等其他响应没有扩展成新通用队列。
真实单TX槽Host在旧Runtime复现后修正通过；Win32核心、6组ASan、三种32位资源配置及SDK3.3.11通过。诊断固件Flash+320 B、RAM仍58144 B。沿原手机/App/Service对隔离COM12执行3组Property/null Action，ACK120～185 ms，每个Command只发一次，计数1→2→3且账户MQTT保持同一session。
另一次无控制开页分段：session建立344 ms，随后scan取消至返回2031 ms，之后5 ms才LAN Ready；现可定位额外2秒在发现清理阶段。下一批评估浏览停止/资源退休与已鉴权WebSocket就绪解耦，先补取消、close汇合、cleanup失败及网络/账号owner门禁；不能直接删await或永久占用mDNS。真实期限/撤权/Key、新caller、Noise和其余产品仍待验证，P3优先、不做100轮。细节/资源/恢复见[LAN§33](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。COM12恢复本轮原备份并校验，其他板/绑定未改，未部署或暂存/提交/推送。

### 10.134 P3扫描退休/Ready生命周期解耦（2026-09-10）

App connector在完成原设备鉴权与BBP同步后不再等待扫描清理才返回；通过byte link关闭屏障组合native和scan退休，立即停止native IO、完整成功后才释放槽。未匹配候选仍可继续扫描，已开始关闭的link禁止转移owner；清理失败保持槽占用。协议、SDK、native插件、账户MQTT及UI不扩展；没有新的连接池/重试队列。
旧代码延迟清理回归失败，新代码8文件106项、App/spec TypeScript及lint通过，debug APK已安装且登录态保留。设备复用§10.133同一门禁binary，不冒充新MCU编译/Host/资源测试。Docker启动因dockerInference IPC文件访问错误失败，Cloud retrying、设备WiFi在线但未完成云登录，故**本批真机性能/控制未通过**；已请求Quit后重启而非恢复出厂，不改数据库/Key/拓扑。COM12恢复本批原备份且hash验证通过，其他板未刷。
主线/证据/下一步详见[LAN§34](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)：先服务恢复，再测试scan未退休时真实控制和close汇合/重开，然后精确期限、撤权/Key、新caller与Noise。page scope.closed不是物理退休证明，清理中的二次open仍BUSY，不把该方案宣称任意快速重开无缝。未暂存/提交/推送。

### 10.135 P3后台扫描退休、关闭重开与App期限真机验收（2026-09-10）

用户恢复Docker后沿原Gateway/Broker/可信代理、已签入APK和同一ACK修复binary复测，不新增协议/产品代码/插件、部署、迁移或设备身份。前三次Ready1660/1437/1603 ms，session返回后4～5 ms；扫描退休2029～2032 ms仍完整执行。两组LAN Property/null Action在scan未返回时得到ACK135/87、38/136 ms，序号各只发一次，计数1→2，原账户MQTT保持同一ready session。

清理期间取消：6 ms开始native close，18 ms返回，精确session.close直到2001 ms、scan退休后才释放槽；之后重开控制成功。第四次观察原授权剩余145343.4 ms，只延长私有测试页面watchdog、不改真实授权/时钟/timer；App在自身期限后3.4 ms开始关闭、37 ms退休，页面仍开、路由回Cloud；过期LAN refresh拒绝且无新发送。不能据此宣称MCU独立到期拒绝或所有快速重开无缝。补发新Cloud控制被测试页面watchdog在发送前拦下，未执行/重试，该业务ACK仍待测。

本批App8文件106项、设备架构和三仓diff通过，未冒充新MCU编译/Host/ASan/Service测试。COM12先完整ROM备份、结束恢复原应用/NVS/OTA且hash校验通过，COM3/11和原绑定未改。gate/trace/临时close hook已恢复；源码LAN仍关闭，未暂存/提交/推送。详见[LAN§35](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)、App LAN§14及Service LAN§26。

下一批P3先补MCU独立期限与新Cloud业务ACK，再真实ACL/Key、新caller及Noise；不回到100轮或扩Hub/S4、新网关协议。

### 10.136 P3接收端期限对照、Cloud业务恢复及预发现设计（2026-09-10）

Android原生WS也有300秒硬期限，因此用正式HTTPS签发30/15秒Grant作接收端隔离测试，仅在debug会话停止JS主动timer，设备/原生/逐操作限制保留。既有LAN连接分别在Grant发送返回后约30066/15439 ms被底层关闭，页面仍开；正常App30秒对照在自身期限后3.7 ms主动关闭。该网络观察不代表MCU硬实时误差，也未注入过期业务帧。旧session拒发后，三条**新**Cloud power命令ACK165.7/158.6/158.8 ms，原账户MQTT不变，无Action重放；新caller在旧期限/墓碑窗口后复用单槽成功，不代替Key/ACL路径。

新增分段首候选865～968 ms、native建连206～302 ms、session681～842 ms，解释本轮约2.0～2.2秒冷Ready与旧1.5秒样本波动。决定前台首页共享有界候选目录、开页再建连鉴权，不预连所有设备。先统一目录/browser所有权，不能首页与connector各起独占scan；只缓存公开提示，不把随机locator当逻辑身份或已在线。具体设计、缓存/网代/清理准入见[LAN§37](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)，**尚未实施预发现或改UI**。

本轮3组x64 Host/ASan、19项Service及架构检查通过；App106项为前批，未冒充重跑。同APK/binary、原服务/身份，未迁移/部署/改生产代码。结束COM12原区域恢复并hash通过，COM3/11及绑定不动，debug hook全部恢复，未暂存/提交/推送。下一批真实ACL/Key及恢复，再Noise；不复跑已通过的到期轮次凑进度，不扩Hub/S4。

### 10.137 P3真实Key失效/恢复与撤权证据分层（2026-09-10）

沿原服务与已签入App，仅COM12隔离身份正式rotate 1→2；未改原Hub/A/B Key/绑定。旧LAN在rotate响应后5859.8 ms退休，页面未退出、授权还剩284094.5 ms，旧session再读无新发送，账户MQTT仍同一ready session。MCU明确鉴权故障，HTTP context409；不是到期测试或仅数据库状态。

保持隔离NVS、只更换固件Key后，同一逻辑设备版本2登录，新Enable exact ACK同步；13条旧授权退休由新代际证据标记superseded而非旧Retire delivered。App重新完成LAN鉴权/BBP同步，新power经LAN ACK233 ms，关闭精确LAN后另一新power经Cloud ACK108.3 ms；无Action重放。

纠正“云端刷新立即使所有本地连接失效”：ACL变化且Key不变走指定Retire/ACK；Key刷新使旧proof失效，走设备明确登录拒绝→authority清退/AccessFault。离线接收端只能遵守原剩余期限，不能把暂时MQTT不可达当撤权。单次约5.86秒不是零延迟或SLA上界，也没有测传播窗口内各时点的业务拒绝。

只补强原Host composition回归，不加生产API、状态、队列或协议。4组x64 Host/ASan、Service19项与隔离PG/worker/carrier33项全部通过；SDK3.3.11新Key固件Flash/RAM仍1124828/58144 B（含960 B诊断workspace）。App未改源码/重建APK；原COM12区域恢复且hash通过，新隔离Key保持版本2，未暂存/提交/推送。详细证据、新binary及后续刷写材料见LAN§38、App LAN§16、Service LocalAccess§28。

下一步是真实第二账户正式分享、撤销、重新分享及设备Retire ACK/期限前断开。此次Key恢复不是该门禁；不使用伪造token/直接插入分享行替代。之后Noise，再按LAN§37推进单一前台预发现目录，不转Hub/S4或100轮。

### 10.138 P3 Noise真实控制、拒绝、期限与模块复用（2026-09-10）

第二真实账户材料尚未确认，前移独立Noise维度，ACL发布门禁仍未通过。沿原App、Service及COM12隔离Key2，使用独立私有Noise配置/binary；合法登录/Query声明profile1及exact Enable ACK后，原HTTPS context/Grant、native link、LocalAccessClient和BBP同步全部复用，无第二连接栈、MQTT或新产品接口。

同一正式90秒授权/原expires_at内完成五场景：LAN power/button ACK314/305 ms；只读tag篡改和已用密文重放各一次均提前断开、旧session再读0发送；精确关闭重开；停隔离JS主动timer后的接收端期限回收。最后新Cloud power=false ACK168.1 ms，原账户MQTT不变，Action只执行一次。Ready1381～2281 ms；异常send到native close开始183/38 ms，完整退休约1.6秒仍包含扫描清理，不能当密码开销或移除屏障。

SDK3.3.11诊断Flash/RAM1131664/58376 B，相比Plain增加6836/232 B；含Noise额外自检与960 B任务workspace，不代表纯算法/最小产品净成本。短采样ipc0/1余量1276/1284 B、loop3392 B、httpd3344 B；不声明长稳或多child资源准入。App仅新增两个取消后迟到crypto缓冲清零/不续发回归，8文件108项及类型/lint通过；5组x64 Host/ASan、Service19项和架构通过，PG33项未重跑。

结束恢复COM12原固件/NVS/OTA并hash校验，其他板/绑定不动；私有注入/计时/trace清理，隔离Key2、服务器最后profile1，不手改DB回Plain。无生产代码/API/协议、APK、部署或迁移变更，未暂存/提交/推送。详见LAN§39、App LAN§17、Service LocalAccess§29。

后续优先真实第二账户分享→Retire ACK/提前关闭→重新分享，确认owner不受影响；若条件仍缺可做LAN§37候选目录/共享生命周期软件部分，再冷热真机对照。首页扫描未启用，本批不等于Noise全部撤权门禁、P3、Hub/S4或全产品无缝通过。

### 10.139 前台共享LAN目录、原接线与软件回归（2026-09-10）

实施LAN§37：raw `LocalLanDiscovery.browse`只做插件适配与精确清理，移除旧独占scan中的重复策略；`LocalLanDirectory`统一10秒共享轮次、16候选/读者上限、15秒单调缓存、服务替换/removed与最后读者退休。原LAN service持有账户/网络/前台代际，前台一次预热，无永久后台轮询/预取Grant/预连设备socket。页面退订不停止别人的发现；旧代际关闭/迟到结果与新代际隔离。

缓存只是公开提示，仍走可信context、真实实例、正式Grant及BBP同步。只允许native open指定不可达错误在close确认成功/未交出link时成为可换候选标记；最多4候选/30秒，取消/cleanup失败/Grant/Noise/业务均不重试。没有新增业务队列、MQTT或网关协议。

App15文件201项、App/spec类型、改动lint和设备架构检查通过。无APK安装、MCU刷写/源码、Service代码/部署/迁移或身份变更，未暂存/提交/推送。具体边界及证据见LAN§40、App LAN§18、Service LocalAccess§30。下批先手机首页冷热/过期对照及实际控制，核实native退后台/JS挂起的扫描清理；当前10秒不是Android硬实时上限，默认LAN=false。双账号ACL仍待验，P3/S4/全产品无缝尚未完成。

### 10.140 手机预扫描实测、推测性合并及网络阻断边界（2026-09-10）

真实Android的foreground与network事件相距约21 ms，立即预热导致无效watch→约2秒unwatch→重扫。仅在service将推测性预热合并250 ms，显式开页即时、代际失效即时，目录/connector清理屏障不改。修订APK已安装；手机首页只一轮watch、10秒后发起退休，正常HOME后清理最终watch/listener为0且原账户MQTT同一ready session。不将约2秒native退休称为密码耗时，也不宣称JS挂起后的硬实时上限已过。

App15文件203项、App/spec类型、lint、设备架构检查与SDK3.3.11诊断构建通过。内部Sketch只增加WiFi断连reason及配置相等性日志，无生产设备API/协议修改。mostfun2026有2.4/5 GHz，COM12重新烧入后已实际连信道6并取得IP；云端仍未登录、手机到设备TCP未通，根因未定，需要2.4 GHz网络对照。未测本批冷热Ready/控制收益，未重跑Host/Service/PG历史门禁。

本机Gateway地址与TLS代理leaf按新网段更新，原CA/DB/身份/Key2不变。COM12已用**本批**原备份恢复并写后hash校验，其他板不动；App观察hook清理，源码LAN=false，未暂存/提交/推送。详见LAN§41、App LAN§19、Service LocalAccess§31；先补数据路径和真实控制，不扩网关或100轮。真实第二账户ACL/Retire ACK、原生挂起、P3整体/S4仍未通过。

同批用户再切YYSC，PC服务地址改192.168.50.13。产品候选认证失败后回退旧网络，不能把其后全部日志当新网；官方WiFi-only最小对照随后扫到YYSC（2.4 GHz/信道11/−55 dBm/WPA2-WPA3），仍两次reason2、未获IP，证明此次失败不要求Blinker上层参与，尚不能归因于SDK/硬件/认证兼容性。最小测试不擦绑定，结束再次恢复原整区/hash通过。下一步需确认独立WPA2网络，具体证据及后续边界见LAN§41.4，不为错误网络状态继续跑LAN时延循环。

### 10.141 独立WPA2对照、手动网络意图与冷热LAN闭环（2026-09-10）

i3water下同COM12/SDK3.3.11官方WiFi-only程序取得IP172.20.10.4，MCU→PC及手机→MCU TCP通过；随后正式Plain产品network/local/cloud均1，context200/profile2。此前YYSC失败不能据此直接归因WPA3/SDK：本批AP与信号也不同。原CA/隔离身份/Key2保留，只更新本机Gateway地址及TLS代理leaf为172.20.10.3；无Service协议/DB/App源码或APK修改。

真实DeviceUiPort冷/首页预发现/15秒缓存过期开页分别Ready3657/969/1574 ms，三次verified Manifest/fresh State通过；热页power/button ACK249/374 ms，后一次实读power=true/buttonPresses=1，无业务跨路重发。原账户MQTT同一ready session、watch峰值1；最后connector/round/watch/listener清零。各一次样本，冷/热native建连与session耗时也不同，不把全部差值归为发现，不宣称性能上界或已无缝。

设备只在公共WiFiConnectionLifecycle注入候选失败策略：手动WiFi/Hub保留所选网络重试，事务配网仍默认回滚。已排入RetryBackoff的同步启动失败返回异步受理，避免外层begin提前终止；云端仍等待durable confirm。原重试状态/存储格式复用，移除重复station.stop，无额外队列/用户API。生命周期12场景、6组x64 Host/ASan、Win32及架构检查通过，普通WiFi SDK3.3.11构建/硬件通过；RAM58144 B未变、Flash增加60 B。其他平台硬件未据此宣称通过。

COM12保存测试NVS后恢复本批原区域3371008 B并写后hash通过，其他板不动；观察hook清理、Key2不变、源码LAN关闭，未暂存/提交/推送。另补同SDK的CompileEsp32EdgeHubInternal完整构建通过（Flash1451040 B/RAM75244 B），仅作修改后Hub/WiFiProv composition编译准入，未刷Hub、非LAN/GATT硬件通过。详见LAN§42、App LAN§20、Service LocalAccess§32。下一步真实分享主体的ACL撤销→durable Retire/exact ACK→提前关闭→重新分享及owner隔离；可独立补原生JS挂起门禁。P3这些边界完成后才接Hub共享LAN/S4，不能拿普通页面亚秒样本替代多child或故障切换门禁。

### 10.142 Android原生扫描守卫及新账户路由前置问题（2026-09-10）

独立capacitor-mdns仓为watch增加可选timeoutMs/stopOnPause；复用原串行执行器、精确session退休、取消期限任务清理，旧队列请求不能在pause/resume后启动或覆盖新watch。App仅在原LAN discovery启用10秒/退后台停止，原目录期限/15秒候选及JS owner清理不变，不新增扫描器、业务协议或UI。其他watch/publisher不受影响；iOS未实现，不承诺OS暂停整个进程时的墙钟硬上界。

先复现旧版实际暂停JS后约8.6秒组播锁仍占用，再验证修订版同PID暂停JS期限回收、暂停JS后HOME在30秒期限前回收，以及force-stop后的锁释放。独立原生Instrumentation4项13.365秒通过；App关联14文件197项、TS/lint/local build、APK及设备架构检查通过。Gradle connected runner受UTP依赖下载TLS阻断，实际原生通过结果来自同APK的ADB标准runner；不是将构建成功当真机通过。

按用户授权创建一个本机普通测试主体，走真实认证密码登录及App既有账号切换；不是公共注册/SMS/UI点击门禁。新主体ME200，无分享时目标context403；但正式账户connection503、接受owner邀请409。只读PG证实新tenant active而route/shard为空，目标normal/shard1、无grant：现有只读route查询及分享错误分类暴露首次共享用户的准入缺口，不能拿跨分片错误当真实跨分片证据。邀请已正式DELETE撤销，原账号ready/context200/connector空闲，手机重新i3water；原生测试包已卸载，Blinker数据/新debug APK保留，板本批未动。

下一批先复核并修Service首次账户/合法分享需求如何进入原allocator，保留容量/资格/幂等/迁移屏障，区分缺路由与真跨shard；不SQL赋路由或建假设备。通过软件/隔离PG后继续真实分享→撤权→durable Retire/exact ACK→提前关闭→重新分享及owner隔离，完成P3才进入Hub/S4。具体证据/顺序见LAN§43、App LAN§21、Service LocalAccess§33。源码LAN关闭，无提交/推送，不把本批资源回收结果当MCU ACL或全产品无缝通过。

### 10.143 首次Broker准入、并发分享与真实账户链路（2026-09-10）

Service以小型注入式route-admission复用原allocator；首次合法账户/分享需求不要求先有设备，已接受分享纳入持久usage。邀请预校验后事务外分配，原事务重验并授权；不放宽ACL、隐式迁移或增加手机MQTT。补被动轮次不吞显式需求、static首次route只创建不覆盖、缺route/真跨shard错误分类；移除抛错后回滚的无效过期写入。10项软件、13项隔离PG runner与Gateway TS/设备架构通过。

本机Gateway已部署；新主体分到normal shard0，connection200后仍受shard0旧镜像阻断。SQLite备份后对齐shard1联调镜像，保留SHARD_ID=0/volume，实际手机新账号MQTT/BBP ready，未共享目标403；原账号恢复ready/context200。目标normal shard1，正式分享接受仍真跨shard409；测试邀请已撤销、无grant或Retire。App源码/APK与MCU本批未改。

下一批按LAN§44、Service LocalAccess§34.3先梳理权限与物理shard分离、内部Route/Delivery/ledger/结果与撤权回流的完整合同，不能迁移测试账号绕过；同版隔离Broker验证后继续真实双主体LAN未到期撤权/设备ACK/恢复，再Hub/S4。P3未整体通过，源码LAN关闭，无提交/推送。

### 10.144 跨分片R0权限/位置分离与生产codec大小复核（2026-09-10）

按LAN§45/Service LocalAccess§35推进，完整步骤收敛到Service `DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md` R0～R4。本批完成R0只读snapshot、fresh有界client及源/目标route校验；修复null route误归shard0/缺source检查，不提前解除本地lane与share同shard限制。Broker client4项、本地8项、隔离PG20项与TS通过；尚非跨Broker执行/回流门禁。

新生产codec基准确认常见增量数据几十字节：8路语义灯JSON222 B→设备33 B/App59 B，power控制13/57 B；大文本不保证变小、MQTT与安全层另计。旧Python9 B BLE头修正为4 B，“服务器shard”与“BLE fragment”分别说明。MCU/App源码、部署和真实账户/Key不改，无本批真机或CPU收益声明。后续R1目标runtime/执行入口→R2回流→R3撤权→R4原双账号LAN P3，再Hub/S4；未暂存/提交/推送。

### 10.145 R1a/b目标执行解耦与共用bounded字节传输（2026-09-10）

Broker已抽取并复用纯target协商/分页/命令结果模块，保留唯一command ledger，删除旧内联实现；JWT的shard/routeVersion保存到App claims，每连接新sessionGeneration，关闭/撤权/gate替换不误报命令成功。JSON内部client改用共用bounded byte HTTP，无重复超时/socket路径。192项定向及Device V2/Telemetry/限流回归、TS与wire基准通过；App/MCU/PG未重跑，无部署/真实账号修改。

R1c仍缺源端远端peer权威解析、signed binary envelope/目的端fresh授权及remote session接线，不能把底座测试算成两个Broker闭环。R3同时补received-list/share操作隐藏的同shard查询约束。详情LAN§46/Service LocalAccess§36及跨分片方案§7；保持share限制，完成后返回真实LAN P3，再Hub/S4。未暂存/提交/推送。

### 10.146 R1c身份和目的端查询；复用HMAC replay修正（2026-09-10）

沿LAN§47/Service LocalAccess§37推进：同一SQL快照权威解析canonical peer两候选，目的端同时绑定原logical ID/peer、两端routeVersion与ACL，拒绝候选替换；原HTTP查询处理与Broker fresh client复用。共享identity移除Broker重复实现。原HMAC满表驱逐及future-skew nonce提前过期已复现并修正，没有另起签名/业务账本。

198项Broker定向、identity2项、HMAC4项、PG/真实Gateway HTTP27项及原协议/凭据/签名/Telemetry/限流、TS/wire门禁通过。只是R1c查询边界完成，源MQTT远端执行、signed binary exchange/active origin/remote gates仍未接线，不是两Broker或MCU闭环。Service跨分片方案§8～9冻结下一步，特别不能用3s ACL期限裁断原3750ms command窗口。分享仍关闭，之后R2/R3→真实LAN P3→Hub/S4；本批无部署、真实账号/硬件、暂存/提交/推送。

### 10.147 R1c签名请求闭环与R2通知前置（2026-09-10）

Service R1请求/响应软件基线完成：原字节signed binary relay、真实source generation/exchange proof、目的端fresh ACL/placement与有界target context、原command ledger已接线；共用HTTP/HMAC归一，无第二App MQTT、假AedesClient或新MCU协议。取消不提前释放仍执行中的command槽和迁移write fence。新增relay21项、PG/HTTP27项、Broker定向199项及协议/Telemetry/限流/TS/wire回归通过；补正本地/远端命令错误缓存遮住晚ACK结果和轻微钟差误拒绝。不是MCU真机门禁。

后续以Service跨分片方案§10.5为准：R2通知sink、State/Presence与有界interest，再Telemetry/ConnectionDemand；R3双端撤权/分享inventory/设备Retire exact ACK，之后原双账号LAN P3，再Hub/S4。新请求路径默认关闭、跨shard分享未开放。详见设备LAN§48、Service LocalAccess§38、App LAN§26。无App/MCU源码、部署/真实账号/硬件操作，未暂存/提交/推送。

### 10.148 R2a通知底座完成；Presence与页面owner分开（2026-09-10）

Service已将本地target通知生产、有限prepare/send队列及MQTT序号适配分开，所有现有State/Presence/Event/Telemetry入口复用；删除旧数组发布器和无界Presence Promise链。准备按会话有界串行，旧授权/换gate/换登录不投递，合并不倒退外层sequence；Snapshot整批准入，Patch/Event不合并，期限/重复callback/取消不提前还槽均覆盖。

新增21项、Broker定向220项、R1两Broker21项及原Device V2/Telemetry/限流/签名/TS/wire回归通过；本批未重跑PG、未部署或动App/MCU。尚非远端通知完成。修正App实际合同：Presence为Subscribe建立的会话级在线订阅，Manifest不是页面租约，unsolicited Manifest也未支持。下一批Service跨分片方案§11先R2b远端Presence watch/lease/cancel，再R2c实际owner与其余通知/原终端租约；R3→原双主体LAN P3→Hub/S4不变。分享仍关闭，无暂存/提交/推送。对应LAN§49、Service LocalAccess§39、App LAN§27。

### 10.149 R2b远端Presence最小闭环；先收敛单目标故障（2026-09-10）

Service已接通原Subscribe、独立watch proof、30 s/15 s租约续期、exact取消和反向Presence Delivery；共用原target context/R2a有界sink，无新App MQTT或MCU协议。补首次响应交接、乱序/续期快照不回滚、旧incarnation取消隔离和非协作工作配额保留，并归一HTTP路由清单与MQTT发布适配。Windows/Linux各40项relay/两Broker门禁、Broker定向220项及原协议/签名/TS/wire回归通过；未重跑PG或做真实账号/MCU联调。

当前watch丢失仍使原账户重连，安全基线不等于多目标无缝体验。下一批以Service跨分片方案§12为准，先R2c-1 App单target lost/unknown与有界恢复及实际owner，再State/Event/Manifest、原终端租约跨shard，随后R3→真实LAN P3→Hub/S4。分享与开发路径仍默认关闭，未部署/动账号或硬件，未暂存/提交/推送。对应设备LAN§50、Service LocalAccess§40、App LAN§28；新现场WiFi为“带翅膀的蜗牛”，软件门禁不代表现场已复测。

### 10.150 R2c-1a目标级Presence失效/有限恢复完成（2026-09-10）

Service源watch失效不再关闭原账户MQTT；App协商bit20后识别`PresenceLost(0x2a)`，以原Subscribe requestId隔离旧通知，仅目标变unknown/清协商状态。独立Presence manager替代重复集合，有限重试Subscribe且不重做Command；新页面wait可只重试耗尽目标。修正AccountClient将普通目标错误当成账户断线的旧行为，其他设备继续通信。MCU/LAN/BLE协议未修改。

Windows/Linux各44项relay/两Broker、Broker220项、App34 suite/425项及类型/构建/原wire回归通过；旧Angular浏览器suite、PG及真机未算本批通过。权威合同见Service跨分片§13，下一批R2c-1b实际页面/业务owner，再State/Event/Manifest与原租约→R3→真实LAN P3→Hub/S4。仍非完整R2或零延迟无缝通信，分享关闭，未部署/动账号/MCU、暂存/提交/推送。对应设备LAN§51、Service LocalAccess§41、App LAN§29。

### 10.151 R2c-1b显式State/Event页面owner基础完成（2026-09-10）

App/Broker新增StateInterest bit21及0x2b/0x2c控制，复用原有限owner引擎的独立实例；普通WiFi页面不触发Hub wake，Cloud子设备仍独立持有原ConnectionDemand。页面scope删除单owner限制，多个消费者独立释放，Cloud观察失效不关闭已就绪LAN或账户MQTT。首页Presence、Telemetry保持原生命周期，MCU/本地业务协议不改。

本地通知按accepted gate＋连续观察token＋fresh权限筛选；重叠页面不丢Patch，全部释放后的旧队列不能复活。App旧StatePage跨观察间隙在写Store前被隔离，复用一次StateConflict重读。跨分片owner控制已复用R1签名HTTP及context，release不创建/延长context，accepted不能从旧缓存复活；不调设备Command、不造Aedes会话或第二租约表。

Windows/Linux各47项relay、Broker225项、App34 suite/436项及类型/构建/原wire回归通过；证据build/lan-cross-shard-state-interest-20260910。完整Angular浏览器/APK、真实账号/PG和MCU未算本批通过，未部署/改账号/刷板、暂存/提交/推送。权威细节以Service跨分片§14为准，对应设备LAN§52、Service LocalAccess§42、App LAN§30。

下一批R2c-2先冻结并实现远端State/Event反向投递、目标级失效/Manifest变化及有限恢复，再原Telemetry/ConnectionDemand/DirectPriority/DirectReady跨shard→R3→原真实LAN P3→Hub/S4。当前完成的是owner控制，不是远端持续State/Event推送或无缝通信；分享仍关闭，不新增100次BLE交替测试。

### 10.152 R2c-2a模型变化通知与App有限恢复基础（2026-09-10）

先修复“换Manifest后旧页面只被fanout静默跳过”的前置缺口，再做远端推送。已校验/持久化的Manifest产生统一lane事件，普通设备与网关虚拟子设备走同一producer/sink入口；删除两棵重复通知条件树。App/Broker协商bit22及ManifestChanged(0x2d)，hint仅提示重读，必须重新SHA-256验证、Accept/ACK和State同步，不包含页面布局。

App复用现有StateInterest观察组，自动恢复最多3次/实际并发4，首次立即、失败后1/3 s，续期/成功不重置预算；旧Manifest页/hash/ACK、State字段表和观察周期都隔离，pending期间新变化不被吞掉。不自动重演Command，不新增MQTT、MCU/API/LAN/BLE字段。Broker同步取消旧gate、检查fresh ACL/连续owner/最新notice，只合并模型提示，不整批清Presence/Telemetry。

新增App13项/Broker15项测试及最终软件回归见ignored build/lan-cross-shard-manifest-change-20260910。完整合同在Service跨分片§15，对应设备LAN§53、Service LocalAccess§43、App LAN§31。未做PG真分享、手机浏览器/APK或MCU测试，未部署/改账号/刷板、暂存/提交/推送。

**本批是恢复基础，不是完整可靠观察流。** 初次hint ACL临时失败且无新模型事件时，App可能仍不知需要同步；远端主动State/Event/Manifest和StateInterest丢失重获尚未接通。下一批从R2c-2b持续source proof＋目标观察丢失合同及两Broker反向投递门禁开始，再原租约跨shard→R3→原真实LAN P3→Hub/S4。分享继续关闭，不以长循环绕过前置问题。

### 10.153 R2c-2b1持续观察证明与只读投影（2026-09-10）

远端StateInterest现在同时受原源端真实账户owner约束，不再只有目的context中的owner。源Relay通过注入端口取得连续capture，原route entry保存不可变context摘要；`state-current`走已有签名HTTP、fresh ACL和placement，独立于已结束的R1请求。目的端stateContext只读，不新建/续命context。共用hash实现清除Presence重复逻辑，不扩MCU/设备API或账户MQTT。

新增19项定向测试和双Broker HTTP重启边界；Windows/Linux各67项relay、Broker240项、App35 suite/449项及类型/构建/原回归通过。完整合同和限制统一见Service跨分片§16；设备LAN§54、Service LocalAccess§44、App LAN§32同步。证据ignored build/lan-cross-shard-state-proof-20260910，无PG真分享、手机/MCU测试、部署或暂存/提交/推送。

下一批R2c-2b2首次响应→通知交接、远端State/Event/Manifest、单目标观察丢失/有限恢复，再原业务租约跨shard→R3→原真实LAN P3→Hub/S4。源证明不代表目的context、设备或数据新鲜度；尚非远端可靠推送或无缝完成，分享关闭，不回到100轮BLE交替测试。

### 10.154 R2c-2b2a共享观察与目标级有限恢复（2026-09-11）

App原caller组聚合为每target一个StateInterest网络owner，移除重复acquire/续期；原引擎管理租约，3次/4实际任务预算同时服务模型同步和观察重获。新增bit23后复用StateInterestStatus的exact stale/denied通知，区分暂不可用与明确拒绝；老owner/晚续期不能复活旧观察。Broker本地Manifest通知授权异常、State准备缺口已接原sink，源持续证明随原owner立即失效，不扩MCU/LAN/BLE或设备API。

App461项、Broker249项、Windows/Linux各68项relay及实际App Session↔Broker lane软件互通2项通过；身份/ACL/设备仍为fixture，不是真机。完整合同及未完成项见Service跨分片§17，联动设备LAN§55、Service LocalAccess§45、App LAN§33；证据ignored build/lan-cross-shard-state-recovery-20260911。未部署、改账号/硬件、暂存/提交/推送。

下一步R2c-2b2b首次同步/通知交接、远端State/Event/Manifest、context失联/序号缺口，再原业务租约→R3→原真实LAN P3→Hub/S4。sink总体过载/超时/发布失败尚未全部目标级处理，远端可靠观察仍未完成；分享继续关闭，不新增100轮BLE交替。

### 10.155 R2c-2b2b1同步发布确认与交接栅栏（2026-09-11）

目的Broker不再只能知道“同步响应已生成”：源账户MQTT成功发布后，通过原signed envelope的published/published-current独立用途确认。原current不复活，原连续观察、Manifest gate与State轮次共同决定只读ready；发布不等于App处理ACK，ready也不证明交接窗口无数据丢失。

复用原16/4操作槽及5.5 s总期限，稳态续期不重复首次确认；非协作取消不提前归还实际槽。新14项源/目标、2项两独立Broker HTTP和1项16并发签名池测试，覆盖乱序/旧轮次/超时/拒绝与池隔离。完整回归App461/Broker249/Windows与Linux各85项relay/App↔Broker2项通过；合同见Service跨分片§18，联动Service LocalAccess§46、App LAN§34、设备LAN§56。证据ignored build/lan-cross-shard-publication-20260911。

下一步R2c-2b2b2窗口有界数据与反向State/Event/Manifest、静默context失联/序号缺口，再原租约→R3→真实LAN P3→Hub/S4。未新增订阅表、业务恢复器、App/设备API；无部署、账号/Key/硬件修改及暂存/提交/推送，分享继续关闭。

### 10.156 R2c-2b2b2有界窗口与远端State/Event/Manifest回流（2026-09-11）

源accepted发布前先完成目的原Target的state-open，重叠caller共享这一步。窗口State按目的权威快照补齐、最多31 Event保持顺序；5.5 s期限不因重复同步而延长，超限/模型冲突/在途Event切基线明确stale。沿原pure producer和账户sink推送，内部sequence/baseline栅栏防重复/错代；必须等真实publisher回调，不能只确认入队。

fresh ACL拒绝只发送无数据的denied控制，原App不重试；可恢复故障沿三次预算重获/同步。静默context丢失依原App每10 s续期、单次5 s请求发现，不是Presence的15 s，不额外增加heartbeat。真实App定时器软件测试及另一target不受明确拒绝影响的测试通过；共享socket/整体sink故障仍可关闭账户，不能宣称全部故障目标隔离。

本批新增15项源/目标/合同和5项两独立Broker HTTP子项；Windows/Linux各105项relay、App461/Broker249、原本地App↔Broker2项、实际App→source→target内存承载互通6项及构建/原回归通过。权威约束见Service跨分片§19，联动Service LocalAccess§47、App LAN§35、设备LAN§57；ignored证据build/lan-cross-shard-state-stream-20260911。

下一步R2c-3a原业务租约跨shard，先ConnectionDemand再DirectPriority/DirectReady、Telemetry，同时补多target容量/取消/慢消费者组合验收，然后R3→原真实LAN P3→Hub/S4。没有新增MCU消息或第二MQTT，无部署/账号/Key/硬件/暂存/提交/推送；源码LAN/分享开关不变。软件基线不代表真实PG分享、手机/MCU无缝完成。

### 10.157 R2c-3a连接需求跨shard与共享活动汇总（2026-09-11）

原活动registry提取为中性app-demand模块，本地App/目的Target共用一套实例，原Hub驱动消费汇总；源另以原owner引擎保存有界reservation/receipt，不能触发源端Hub。沿原账户MQTT/signed R1，未增加MCU wire、第二MQTT、虚构设备client或第二调度器。

release先于源route lookup失效，源在途证明绑定exact owner/revision；最终授权再查。单请求结束不再退休其他有效owner，真实ACL拒绝仍清理匹配Target。本地+remote源需求合计16/session、目的32/target和256全局活动配额；旧回执不续租，丢失cancel依30 s有限TTL收尾，R3立即失效/Retire exact ACK仍待完成。

新增12项Broker/原Hub驱动软件测试、3项两独立Broker签名HTTP子项、3项实际App核心内存互通（含原10 s续期检测丢context）。软件驱动中的physical transport/authority为fixture，不等于真实Hub唤醒通过。最终验收及容量/时间边界见Service跨分片§20；联动设备LAN§58、Service LocalAccess§48、App LAN§36，ignored证据build/lan-cross-shard-demand-20260911。

下一步R2c-3b原DirectPriority/DirectReady跨shard，保留owner DirectAdmin epoch、exact Hub绑定及真实Yielded/Ready，再Telemetry/R2组合验收→R3→真实LAN P3→Hub/S4。需求accepted不等于可直连，失效不自动重演Command。无部署/账号/Key/硬件/WiFi/暂存/提交/推送，分享/源码LAN开关不变。

### 10.158 R2c-3b Direct范围纠正及R2c-3c-1 Telemetry目标控制（2026-09-11）

原计划要求“跨shard Direct成功”，但owner-only DirectAdmin与tenant placement使合法两端始终同shard；迁移中拒绝、迁移后一起移动。PG owner/share/迁移门禁确认后，保留原本地Direct及真实Hub Ready，远端在创建context/ACL查询/Hub工作前拒绝。不为完成表格创造云端分享提权、远端凭据或第二套射频调度。

Telemetry解码/模型门禁/状态编码提入现有app-target，原app-lane立即复用并移除旧内嵌实现；原采样引擎未复制。新增exact消费者/租约/gate有效性证明，防止Close、过期、重开、模型/授权变化后缓存Open/Renew冒充有效；重复不续租、不重演Action。远端Telemetry消费者/回流尚未接通。

Broker262、Windows/Linux relay各121、隔离PG31、App461/类型检查及原State6/需求3/本地2项互通通过；完整权限/测试/下一步见Service跨分片§21，联动设备LAN§59/App LAN§37/Service LocalAccess§49，ignored证据build/lan-cross-shard-direct-scope-20260911。下一步R2c-3c-2原消费者身份端口及Status/Data反向接线，再R2组合→R3→真实LAN P3→Hub/S4；不扩MCU/协议，不开放分享，无现场/部署/暂存/提交/推送变更。

### 10.159 R2c-3c-2a Telemetry消费者身份与本地队列隔离（2026-09-11）

消费者使用实际session对象及current/expiry/有界通知端口，原Telemetry引擎不再依赖MQTT clientId或全局App回调；本地注册/控制/清理/通知立即复用。保留32/设备、1024全局配额及唯一设备采样汇总，删除旧入口、重复终止通知和未使用导入。

修复同ID新旧登录迟到投递、租约超出认证期限、Close/expiry后队列样本继续发布、旧终止状态清掉新epoch数据，以及单消费者同步适配器异常打断其他消费者。原sink复核exact消费者/claims/gate；已发字节不可撤回，异步传输仍需自己的有界端口。中性Target端口为fixture，远端生产Telemetry未启用。

Broker278、Linux Telemetry23、双平台relay各121、App461/类型与原11项互通和构建回归通过；Service跨分片§22/设备LAN§60/App LAN§38/Service LocalAccess§50为同步入口，ignored证据build/lan-telemetry-consumer-20260911。下一步R2c-3c-2b签名远端消费者/Status/Data与接受回执发布顺序闭环，再R2组合→R3→真实LAN P3→Hub/S4。不扩MCU/协议，不开放分享，无部署、账号/Key、硬件/WiFi或暂存/提交/推送操作。

### 10.160 R2c-3c-2b-1发布交接前置（2026-09-11）

已完成原Consumer的5 s单调交接窗口、单条最新值合并、exact响应publisher确认、旧回调/终止状态隔离，以及源relay前Telemetry能力/方向校验。权威模型变更通过原SQLite身份查询截断旧样本，不解析整份模型或新建缓存。

完整机制、验证清单与剩余唯一入口见[Service跨分片§23](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#23-r2c-3c-2b-1发布交接前置与源准入gatt10160)。本地生产已接线，但远端Telemetry尚未成功接通；下一步仍为2b-2 signed回流/源证明/原App互通，再R2→R3→真实LAN P3→Hub/S4。不得以软件fixture替代四端/真机，不扩新MCU/API或100轮。

### 10.161 R2c-3c-2b-2原Telemetry跨分片回流（2026-09-11）

目的Target注入可选路由适配器，复用原唯一消费者/采样汇总和有界sink；源仅保存exact原始Open、stream/epoch、期限及发布关联。沿原signed Gateway/账户publisher返回原Status/Data，内部绝对deadline避免缓存重传偷偷续租，App/MCU wire不变。Close不新建Target；早到样本/终止状态等待原5 s发布窗口；跨stream借证明、旧回执迟到失败与慢发布已补回归，不新增模型副本/采样引擎/MQTT。

正式Broker428、双平台relay各138、Linux Telemetry52、App461/类型及新增原App Telemetry互通2项通过，另保留State6/需求3/本地2回归。完整口径、资源上界和未完成事项见[Service跨分片§24](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#24-r2c-3c-2b-2原telemetry跨分片小闭环gatt10161)。证据在ignored `build/lan-remote-telemetry-20260911/`；无本批MCU/APK/无线四端或部署证据。

下一步集中完成R2混合业务/实际池满额/非协作依赖、丢cancel/回执及Broker重启/有限回收、原App失效恢复，不扩协议。R3主动撤权及设备Retire exact ACK不可被有限TTL替代；之后真实LAN P3→Hub/S4。同步设备LAN§62/App LAN§40/Service LocalAccess§52；无暂存/提交/推送或现场Key/Flash/WiFi操作。

### 10.162 R2d-1资源所有权与组合故障门禁（2026-09-11）

源/目的复用小型OperationWork，只在调用方和实际依赖都结束时释放原16/4槽及migration fence；无新队列、定时器或权限。源在原maps内限制1024 Telemetry关联（含在途Open），回收旧位置/到期会话，不复制采样引擎。真实HTTP丢包暴露共享客户端迟到error导致worker退出，现已修正request/response幂等错误处理并通过连续reset恢复测试。

混合通知池满额时forward/reverse仍可完成；两worker丢cancel/5 s发布交接、source/target runtime重建和原App续期失败后的显式恢复通过。完整验证数字、证据粒度和未完成项见[Service跨分片§25](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#25-r2d-1实际资源所有权http断开与第一组组合门禁gatt10162)。证据为ignored `build/lan-relay-resource-faults-20260911/`，不是进程断电/MCU/手机验收。

下一步R2d-2实际多target通知生产者/账户sink拥塞隔离、进程级故障及退出矩阵→R3→真实LAN P3→Hub/S4。同步设备LAN§63/App LAN§41/Service LocalAccess§53。设备/API/wire和App生产源码未动，无部署/Key/Flash/网络/暂存/提交/推送；现场SSID保持mostfun2026。

### 10.163 R2d-2软件组合收口与R3入口（2026-09-11）

真实Source/Target、Presence、原Telemetry生产者与账户sink验证：单target准备暂停不阻塞其他target；同一账户publisher被堵时本账户排队、另一账户继续；Event顺序不被最新值策略合并，溢出明确stale而非静默丢事件。原16准备槽被实际混合生产者占满时有界拒绝，实际工作settle后才恢复；模型替换/关闭只清对应目标或账户。

原两Broker signed HTTP新增已应用Open/published返回丢失、强制进程退出/重开SQLite。原成功Action仍受fresh ACL/两端placement/迁移屏障约束，合法exact重传只读持久结果；旧Telemetry epoch/源session不能复活。不是物理MCU断电或真实登录/无线测试。完整软件退出矩阵及验证见[Service跨分片§26](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#26-r2d-2多目标混合生产者实际进程故障与软件退出矩阵gatt10163)，证据`build/lan-relay-mixed-restart-20260911/`。

R2软件功能/有界故障基线完成，进入R3实现；容量峰值/发布/无缝验收另列，不继续滚动R2或100轮交替。当前原invalidate只接本地lane，Gateway分享只指向一个shard，received/leave仍限同shard，故下一步先R3a双端生命周期，再可靠失效与设备Retire exact ACK，之后才统一开放受控分享并恢复真实LAN P3。同步设备LAN§64/App LAN§42/Service LocalAccess§54；本批无业务源码/API/部署/账号/Key/Flash/网络或暂存/提交/推送改动。

### 10.164 R3a定向撤权生命周期与下一步可靠投递（2026-09-11）

原Broker invalidate接线先清relay原所有者，再清本地lane与失效排队通知；保留原认证与exact响应六字段。共用grant版本比较，已证明同版/新版和owner不被旧失效删除，迟到cancel核对context＋版本；在途lookup/authorize/proof也可取消，但真实依赖未settle前继续占原有界槽。未解析到授权的同账户/peer查询保守取消、后续fresh重试，不凭MAC补身份或新建权限缓存。原Telemetry/State/Presence/活动需求各自收尾，不改Action账本或设备已执行动作。

正式Broker470项、Linux relay179项及App461项/类型、Telemetry4/State6/需求3/本地2互通通过，口径重叠不相加。稳态模型变更测试补等待目的实际publication ready；另测首次交接前模型变更应失效、迟到receipt不复活，未降低安全校验。完整证据/边界与R3b审计见[Service跨分片§27](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#27-r3a原撤权入口双端生命周期和版本隔离gatt10164)。

下一步R3b：Gateway目前仍只尝试单shard失效，重启无持久任务；已有LocalGrant trigger/outbox/worker只负责设备exact ACK，不能当双Broker确认。复用其事务和有界scheduler，补独立最小投递状态及两端placement确认，再R3c统一received/share/leave，最后真实双主体LAN P3→Hub/S4。TTL、丢cancel的负证明、HTTP200均不代替exact设备ACK。本批仅Broker业务源码、测试/方案变更，无新App/MCU源码或部署/账号/Key/Flash/网络/暂存/提交/推送；开关仍关，证据ignored build/lan-relay-invalidation-20260911。同步设备LAN§65、App LAN§43、Service LocalAccess§55。

### 10.165 R3b-1持久双端投递及原设备退休隔离（2026-09-11）

Service migration031把share INSERT/权限UPDATE/DELETE与最小失效意图同事务提交，复用原有界scheduler/Broker client和原LocalGrant退休路径；删除handler单次单shard尝试。按account＋logicalDevice合并最新revision，各placement独立确认，旧版本/claim/route回执不能覆盖新任务；最多4个job×4个placement实际HTTP，超时仍等待真实I/O结束。相同shard可合并调用，账号仍单MQTT。

隔离PG专项runner60项通过，含真实share/repository/index到HTTP fixture、原设备退休/控制及有界worker；Broker470、Linux relay179和App461/类型、15项原互通通过（口径有重叠）。完整Gateway正式device-v2清单也已通过。ACL回滚同时回滚两种投递；Broker结算不改变设备退休待ACK行。软件权限/设备fixture不是MCU无线或真实双Broker与PG的一次闭环，证据ignored `build/lan-relay-durable-invalidation-20260911/`。

三种完成条件及资源/迁移边界见[Service跨分片§28](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#28-r3b-1事务持久投递双端确认与设备退休解耦gatt10165)。同版invalidate保留合法对象，不能把它当旧placement drain；原迁移屏障负责实际工作结算，永久退役端不能凭网络失败算成功。下一步R3b-2把PG、两个实际Broker和原Retire ACK连测，再R3c/R4，不继续扩协议/100轮。同步设备LAN§66、App LAN§44、Service LocalAccess§56；本批无App/设备业务源码、部署/真实账号/Key/Flash/网络/暂存/提交/推送。

### 10.166 真实PG/两Broker撤权串联与回到普通设备基础主线（2026-09-11）

本批生产PG事务、原投递worker和签名中继、两个实际Broker进程/SQLite串联通过7项；原两Broker/PG投递/LocalAccess回归85项。设备Retire ACK在原repository入口由协议夹具送入，未声称MCU或完整控制MQTT运输已测。去重抽取原Broker进程管理测试工具，无生产API或设备协议改动。细节见[跨分片§29](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#29-r3b首组真实pg两broker退休ack串联与基础主线收敛)。

当前R3还剩组合/主体收尾、分享入口统一两类工作，不是承诺两轮完成。R4真机之后优先B1普通WiFi/BLE接入、B2绑定解除、B3统一授时、B4 LAN产品化；owner解绑App DELETE/Service GET-only、普通设备unixTime=false、LAN组合/多平台缺口已记录[基础收口路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)。Hub/S4、100轮和Wio24 h不作为普通基础能力的前置；未确认旧Broker任务仍pending，不跳过安全屏障。源码开关保持关闭，无本批部署/账号/Key/Flash/网络/暂存/提交/推送；证据ignored `build/lan-r3-baseline-closure-20260911/`。

## 11. Service/Broker 实施

### 11.1 Service

- Edge Hub product capability 与 mandatory MQTTS admission；
- pending/active topology repository、CAS version 和 operation ledger；
- per-child active-hub Gateway credential metadata、digest、KMS ciphertext、KEK version；
- `GatewayAccessEnvelopeV2` 的 PresenceKey version/digest 校验、48 B access material 密封和 active+pending
  locator 两阶段轮换；raw PresenceKey 与 Gateway secret 均不得进入普通持久化或日志；
- owner attach/create/resume/cancel/commit/revoke API；
- MQTTS sealed secret delivery、durable ACK、retry/expiry；
- ControllerGrant 使用现有签名合同，Gateway raw secret 不进入 signed body；
- Edge Hub proof 校验和 topology activation；
- DeviceKey refresh、Hub reimage、Hub replacement 与 rollback-required recovery；
- secret 字段结构化日志拦截、response redaction、数据库列级审计。

### 11.2 Cloud Gateway/Broker

- Edge Hub SessionCredential 带明确 product capability、`tlsRequired=true` 和 wire version；
- Plain MQTT endpoint 拒绝 Edge Hub clientId/product；不能只依赖客户端自律；
- management topic 与普通 child route topic 分离；
- management 下行只保存 sealed envelope，QoS 1、retain false、exact size/rate limit；上行 ACK 接受 QoS 0/1、
  retain false，并继续由 application ledger/idempotency 保证最终交付；
- ChildRouteEnvelope 校验 exact Edge Hub session、active topology、epoch/version、kind/size/rate；
- child command ledger 仍按 child logicalDeviceId/requestId 保存，不记到 Hub 设备；
- broker restart 重放 envelope ciphertext/业务 command，不能重放已消费 plaintext workspace。

## 12. App 实施

- 在已认领 child 的管理页选择 exact Edge Hub；
- 展示 Created/Delivering/Installing/Verifying/Active/RollbackRequired，不把 pending 显示为在线；
- pending relay package 只放内存/平台 protected transient buffer，禁止 localStorage、普通 SQLite、analytics；
- child Direct authenticated 后请求 fresh control nonce，再获取/刷新 exact grant；
- exact relay、receipt commit、App kill resume 和 cancel；
- 移除 Hub 时明确显示“云端已阻断、本地 credential 待附近撤销”的 eventual 状态；
- G2B 后再增加网关面板 permit-join；不能在 G2A 同批扩散 UI。

## 13. 分批执行顺序

| 批次 | 内容 | 退出条件 |
|---|---|---|
| G1D-R（发布轨，并行待完成） | Wio/Direct authenticated soak | 日常改动先过 30～60 min 工程长稳；候选发布前完成一次连续 24 h 及恢复报告，之后 Wio 才可退出 preview |
| G2-0A（完成） | exact contracts/golden | schema v3 layout 冻结；envelope/route 由 device+Service exact golden 验证；topology state 由三端验证。App 不实现不消费的 MQTT management codec |
| G2-0B1（完成） | Service KMS + device-targeted seal + TLS admission | 固定 KDF 向量；secret 只以 KMS/envelope ciphertext 持久化；Gateway 与 Broker 均拒绝 Edge Hub plain/profile mismatch |
| G2-0B2（完成） | mandatory MQTTS management topic/ledger | 下行 QoS1/retain=false；上行 ACK QoS0/1、retain=false；重启、重复 envelope、retry/expiry 与 topology 幂等对账通过 |
| G2-0C（完成） | child 三槽 schema v3 | v1/v2 lazy atomic migration、Host/三平台编译与资源门禁已过；Wio/ESP32/UNO R4 真实断电恢复全部通过 |
| G2-0D（D4a + onboarding 软件门禁完成，D4b 待专用板） | ESP32 Edge Hub skeleton | MQTTS/SNTP/protected-NVS、manual/BLE WiFiProv composition 与默认板 fail-closed 已过；生产加密板需完成真实配网、sealed delivery、物理断电、Broker redelivery、same ACK |
| G2A-1a（完成） | Service owner attach control plane | production runtime、create/read/resume/cancel/receipt HTTP、owner/instance/accessEpoch 鉴权、KMS relay、Controller Grant、receipt HMAC 与 exact CAS 已通过 |
| G2A-1b（控制面完成） | App Admin attach relay | 非 secret checkpoint 恢复、fresh nonce exact relay、receipt HMAC、独立 Gateway Method 2 与 Hub proof 已通过双 XIAO 控制面真机 E2E；权威 topology 为 Active，旧超时 operation 正确收敛为 Expired；不包含 Manifest/State/Command/ACK 数据面 |
| G2A-2a（软件完成） | 单会话主动端安全核心 | Gateway record expiry gate、Method 2 initiator 与 DirectSecure initiator 和现有 responder 互操作通过；不包含 GATT Central |
| G2A-2b（软件完成） | transport-independent 单 child session | exact Hello/Method 2/DirectSecure 帧级状态机、总 deadline、协商与 32 次恢复门禁通过；不包含 BLE Central |
| G2A-2c（软件完成） | 独立 Central 合同 + 单 child GATT record link | Fake Central 下 scan matcher、ATT20 分片、stall/timeout/disconnect、32 次恢复通过；不包含 ESP32 port |
| G2A-2d（软件完成） | access material 合同修正 | 单一 V2 同时密封 Gateway secret + PresenceKey；初次 durable install、active+pending 原子轮换、ACK replay 与跨端 golden 通过；Service activate 编排留在 G2A-3 |
| G2A-2e（短稳完成，连接交替/长稳待完成） | ESP32 Central + exact 单 child composition | 固定队列 Central、exact Presence、Hello/Method 2/DirectSecure 与 cloud-first NimBLE 生命周期已通过双 XIAO proof；3.3.11 的任务水位、三个空闲 MQTT keepalive 周期及真实 10→30→100 route Action/replay ACK 已通过；§10.49 新确认空闲释放/Direct 交替缺口，先按 L1～L3 修正，再继续长稳 |
| G2A-2f（完成） | proof control contract | exact command/result、Hub coordinator、Broker ledger、Service HTTP/CAS 与 App relay 已完成真实 Hub/child Secure proof 和 Active CAS |
| G2A-2g（四端热路径完成，按需释放/冷恢复未完成） | 最小单 child route bridge | Active topology 下 Manifest/State、真实 Android account Action/ACK/Patch、durable ledger、Broker 重启、两轮 SessionCredential refresh、child 重启后的低 revision 完整快照替换与后续 Patch 已闭环；§10.49 的同固件 Direct→attach→Gateway 控制通过，但 Hub 持续占用 ACL，L1～L3 交叉准入仍未通过 |
| G2A-3a（软件完成） | detach 云端闸门 | owner detach 原子进入 Revoking，Active route 立即失效，Broker 主动退役虚拟 session/Presence/Command；PostgreSQL 与 Linux Broker 门禁已过 |
| G2A-3b（四端完成） | device revoke + Detached | 正常 nonce/revoke/finalize、no-poll worker 与双板 pre-tombstone 掉电注入均闭环；App nearby Admin 幂等 recovery 后 Service/Broker/Hub 收敛 Detached/Finalized/无 active access |
| G2A-3c（renewal R1～R4d transaction gate 完成；产品入口未开放） | active Gateway credential renewal | R1 exact wire、R2 protected journal、R3 可恢复 Rotate、R4a Service/KMS ledger+route pause、R4b Broker durable projection、R4c proof/CAS/Finalize，以及 R4d 软件故障矩阵、真实双板掉电恢复均已通过。R4d-c 已修复 stale Broker projection 竞态；新基线 10→30→100 轮均完成，最后 100 轮 `71→171` 中 PostgreSQL/Broker 单调且唯一，heap/stack 无持续退化。R4 transaction gate 关闭；owner/App 入口仍等待产品策略/权限/恢复 UX，D4b、G2B-4 真机负测和发布长稳继续独立阻断。Hub replacement 首版继续使用 detach→attach，不冒充无中断轮换 |
| G2A-4（完成） | 南向模块边界 | native logical record codec 脱离 BLE；`Link + Selector` 静态组合；GATT concrete composition 与 Zigbee/BLE Mesh model adapter 边界冻结，Host ASan 41/41 + 200k fuzz |
| G2B-0（完成） | adapter-scoped permit-join wire | device/Service 33 B exact golden、可扩展 adapterId、无 secret；App 不复制未消费的 management codec |
| G2B-1（完成） | exact Hub 短窗口控制面 | owner HTTP、PostgreSQL window CAS、Broker QoS1/reboot replay、Ready 活动态的 fail-close/expiry 与终态/identity gate；不扫描、不创建 ownership/topology |
| G2B-2a（设备软件完成） | window executor + Native GATT fragment pump | management Open/Close/expiry 已接入；4 candidate、单连接、App→child 按 Port credit ACK 且无 TX ring、child→App 32×20 B（一个 512 B logical record）固定 RX 队列；MAC/Noise/secret 不出各自边界 |
| G2B-2b（软件完成；真机收口中） | operation-bound relay | kind 7..11、每方向单 batch stop-and-wait、4 candidate、1～16×20 B packet/batch、Broker 瞬时 mailbox；不复用 ChildRouteEnvelope；逐 packet 云往返已由真机证据否决并删除，Host/跨端 exact golden、Service/Broker 门禁和 ESP32 EdgeHub 编译通过 |
| G2B-3（软件完成） | App enrollment + attach orchestration | owner Hub 设置页生产入口、exact HTTP/CBOR、双窗口 Provisioning→Direct、authorized presence、明确候选选择、`relaySessionId` fail-closed，以及 relay child-control 串联 G2A 已实现 |
| G2B-4（软件恢复、App 账户边界、N1 与 supersession 四端子门禁完成；其余负测待完成） | failure/recovery gates | exact Hub/child 无秘密 workflow 在 grant 前落盘，覆盖 Pending、ownership 已 commit 但 attach 未开始及 operation attach 三阶段恢复；Direct credential/checkpoint 已按 authority+account 隔离，运行事务绑定 sessionEpoch 并在跨账号响应时清零敏感缓冲；真实 A→B→A 已证明 GATT 立即关闭、B 不见 A credential、A 可恢复。child 新 access root 已由真实 reset→enrollment→terminal Supersede/Retired→Detached→Hub reboot 证明旧 Gateway credential 不复活且新 Direct 不受影响；仍需错误 Hub、expiry/tamper/replay、force-stop 与其他 pending boundary/组合 restart 四端门禁 |
| G3 | controlled routes | 完整 device/service route、ledger/rate/TTL 闭环 |
| G4 | advanced profiles | broadcast telemetry、roaming、多 Hub、Mesh 分别立项 |

不得并行先写 Mesh、roaming 或多 child scheduler；它们会掩盖首个 exact credential/topology 闭环的问题。

### 13.1 长稳门禁分层

24 h 不应成为每次源码修改后的同步等待，也不能被删除或用 Host mock 冒充。冻结为三层：

1. 每次相关改动：既有 Host/架构/编译门禁，加 32 次重连和 deadline 负测；
2. 硬件阶段收口：30～60 min authenticated engineering soak，尽早发现资源单调下降、会话不换代和恢复失败；
3. 候选发布：同一冻结固件、App、RTL/SDK 组合执行一次连续 24 h authenticated release soak。

G2-0A～G2-0C 是数据合同、服务持久化和 child schema 工作，可与 G1D-R 并行。G2-0D 及其后的硬件闭环
可以开发和做短门禁，但在 Wio child 长稳、ESP32 Edge Hub 自身长稳均未通过前，不得宣称 Gateway production
ready，也不得借此把 Wio BLE 从 preview 提升为正式支持。

## 14. 发布门禁

### 14.1 TLS/secret

- DNS/TCP/TLS/CA/hostname/证书/时间/内存任一失败不回退 Plain；
- 服务端 Plain MQTT 入口主动拒绝 Edge Hub profile；
- packet capture 只能看到 TLS；Broker/ledger 只能看到 GatewayAccessEnvelope ciphertext；
- DB 普通查询、日志、metrics、trace、crash dump、App durable storage 均搜索不到 raw Gateway secret/DeviceKey；
- wrong hub、wrong tenant/environment、wrong DeviceKey version、expired/replayed/tampered envelope 全部拒绝；
- QoS 1 duplicate 只产生一次 durable access record，返回幂等 ACK。

### 14.2 topology/authorization

- 两项证明完成但第三项缺失时 topology 仍不是 Active；
- Edge Hub 不能代理未绑定 child、旧 epoch/version、同家庭其他 child；
- source 永远来自 authenticated ingress + topology，不信任 payload；
- detach 后 Cloud 立即阻断；旧 Hub 即使仍持有本地 secret也不能通过 Broker；
- Hub replacement 轮换 child Gateway slot 后旧 Hub Method 2 失败；
- Admin/Shared 不受 Hub attach/detach 影响。

### 14.3 BLE/availability

- 未认证 central 在 deadline 后断开；连续抢占有 backoff 且不泄漏 workspace/session；
- Edge Hub non-persistent lease 不阻止 App 在 bounded time 内 Direct；
- App kill、Hub reboot、child reboot、Cloud Gateway/Broker reboot 均可恢复同一 operation/session事实；
- online 状态严格区分 nearby、topology attached、reachableViaGateway、direct connected；
- 单 child 24 h soak、反复连接、WiFi/MQTT drop、BLE fragment/tag/replay 负测通过。

### 14.4 资源与发布结构

- 普通 BLE/WiFi QuickStart 不链接 Edge Hub Central/coordinator/KMS management 代码；
- child 增加第三槽后的 ESP32/UNO R4/Wio Flash/RAM/stack 与断电报告更新；
- Edge Hub 是同一 Arduino library 内的高级 product target，不拆成独立 BLE/WiFi ZIP；
- 首版只声明 ESP32-class Edge Hub production support；UNO R4/Wio 继续作为 child，不暗示它们已具备网关能力。

## 15. 后续 Mesh 边界

GATT 网关完成后，Mesh 仍只复用 northbound：logical identity、topology、ACL、Property/Action/Event 语义、页面、
Cloud Route/State/History。Mesh southbound 必须另建 `BleMeshModelAdapter + BleMeshProvisionerPort`，实现 provisioning、
NetKey/AppKey、地址、sequence/IV、SIG/Vendor Model、Proxy/Relay/Friend；它不是 native record `ChildLink`，不得把
GATT fragment、Method 2 或完整 Manifest 原样塞进 Mesh。

是否进入 Mesh 的产品触发条件：明确需要几十/上百节点、多跳覆盖、照明/群控或设备间本地广播。普通 Arduino
教育设备、传感器和少量家居 child 继续优先使用 GATT + 按需 Edge Hub。
