# Device V2 实施状态

> **2026-09-09 §10.112 当前：S3a-1c唯一child executor/需求与Cloud adapter拆分已实现。**
> 每个活动child只有一份回调、南向序号、单在途/重放、管理事务与512 B workspace；Cloud route实际消费该executor并只保留自身准入/需求/投影/correlation。Host/ASan53、双owner背压/序号、架构/单库清单与ESP32 3.3.11 EdgeHub编译通过；context7552 B（+40 B/child）。下一批S3b有限授权；LAN生产通道/安全wire、S4无缝功能和S5体验未完成。未真机/烧录/部署/改凭据或暂存/提交/推送。

> **2026-09-09 §10.111 历史：S3a-1b按通道访问绑定/协商/分派及原子解绑已实现。**
> 删除Client/Runtime全局auth/control setter和成员；BLE-only/WiFi+BLE使用同一TransportHub绑定，Cloud不再声明BLE认证/控制能力。Host/ASan53及四种32位容量目标通过；SMALL默认Device1672 B、三槽1760 B，详见GATT§10.111。

> **2026-09-09 §10.110 历史：S3a-1a非鉴权路径故障隔离、逐peer完整退休、注册/回滚保护及三槽ABI已实现/验证。**
> 普通WiFi/WiFi+BLE的station/Cloud非鉴权Error不再停整个Client，新增Degraded诊断；AuthenticationRequired因含Key/device/session撤销仍按AccessFault关闭，不放宽安全边界。Runtime补无断开回调时control window退休，注册不同owner拒绝覆盖。
> Host/ASan53通过；Win32 SMALL 2/2→3/3，Device1664→1744 B，frame256 B/固定24-field存储824 B不变，lifecycle20 B。默认容量未改变，80 B不是完整LAN成本；架构/源码whitespace/单库发布清单通过。ESP32 3.3.11的WiFi/BLE/WiFi+BLE及R4/Wio WiFi五个编译目标通过，资源和依赖警告见GATT§10.110；不等于真机通过。
> 下一批S3a-1b按transport/session鉴权/control分派与组合容量，再1c唯一child执行/需求抽取；EdgeHub错误生命周期尚未改。LAN/auth wire、S3b/c及S4/S5未完成，100轮继续暂停。未改App/Service生产代码，未烧录/部署/改Key或暂存/提交/推送；详见LAN方案§9、共享方案6.9。

> **2026-09-09 §10.109 历史：通用LAN/WebSocket/发现架构分析与三端实施顺序修订完成，生产通道未实现。**
> LAN属于普通WiFi、WiFi+BLE、WiFi网关共同网络能力；网关只注入child路由。首选mDNS/DNS-SD，旧JSON WebSocket不复用；新方案见[通用LAN实施方案](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)。
> 下一源码批次先拆Cloud终态Fault与IP/整个Client生命周期、核对SMALL两transport及单鉴权provider限制，再接共享LAN模块；网关唯一child执行/需求抽取保持主线。先ESP32普通WiFi self，再WiFi+BLE/Hub，R4/Wio逐平台门禁。
> 本批只分析/改文档；未改生产源码/依赖/固件或Key/Access，未构建/真机/暂存/提交/推送。安全wire与默认资源配置待S3b，首次在线授权及有限离线边界不扩大；S4功能、S5体验验收不变，100轮继续暂停。

> **2026-09-08 §10.108 历史：两child满槽permit Busy、A Direct物理让出/B继续、MQTT身份换代恢复短门禁通过。**
> 真实控制：满槽后A4→5/B3→4，A让出时B4→5，新Hub MQTT身份恢复后A/B5→6；账户MQTT未换，最后scope=0，Inspector已关闭，原Key/Access/固件保持。
> 工具移除全Hub故障/恢复假设，33＋5＋3项回归通过；未改三端生产代码或重新烧录，无暂存/提交/推送。Direct物理Ready不代表手机BLE认证完成，MQTT故障恢复约9.6s不代表无缝。
> 无缝功能在S4、体验验收在S5。下一批S3a拆共享child执行/需求与Cloud adapter，S3b有限本地授权；S2管理变更/SDK reset资源在S4真机准入前收口。旧周期未删除，100轮暂停；详见共享方案6.6/6.7和GATT§10.108。

> **2026-09-08 §10.107 历史：S2e已本机配套部署；同一Hub下两子设备数据面及独立关闭短门禁通过。**
> Service bounded route集合与Broker逐child floor/恢复/命令/需求/撤权已接通；无Key子设备改为身份元数据、凭据NULL，未知Presence目标不再拖垮账户MQTT。
> 普通页面A 2→3、B 0→1；并行Cloud API A 3→4、B 1→2，SDK同时两条GATT连接；释放A后B保持Secure并2→3/ACK，单账户MQTT不变。
> Broker165项及主脚本/真实Redis、Service完整PG（最后28项）、协议22项、硬件工具3项通过。未重新烧录设备或修改App UI/APK；原Key/Access/绑定保留，无暂存/提交/推送。
> 旧10s占用/6s让出仍会中断持续需求，**并行短测不等于无缝/持续通信完成**。下一批S2d双占用permit/管理/Direct隔离及MQTT/SDK资源，随后S3本地有限授权→S4去周期；100轮暂停。
> SQLite升级前有一致性备份；仅本机Gateway/shard1候选升级，其余shard/线上未升级。含NULL身份的tenant迁移须两端同版，不能只切旧镜像冒充可安全回滚。

> **2026-09-08 §10.106 历史：S1 App实机页面通过，COM3第二child已正式绑定；发现Service/Broker仍单child，先补S2e。**
> A普通页面Cloud按钮1→2/ACK、无Direct抢占、退出scope归零；COM3只刷app0，真实Hello/Noise/relay/receipt/Method2/proof→Active，未重置Hub/A。
> B=`ble_GnO2jmRwLwgTQk-nL_ktdA`，没有DeviceKey；原清单误判Cloud能力的源码已修正并通过management18项、真实PG和typecheck，待与多route配套部署。
> Service `getActiveRouteForHub`仍拒绝两个Active，现场日志已复现；Broker每MQTT client仅一个RouteContext。**B数据面及双Secure尚未通过，不能把Active算可控。**
> 下一步共享方案§6.5：bounded route集合→Broker逐child authority/projection/late ACK隔离→软件门禁→Service/Broker配套部署→复用A/B双数据面真机。
> App42文件383测试、本地Web/APK构建与ADB安装通过；带真实PG的Gateway完整Device V2脚本通过。仅修测试1ms真实计时抖动，未延长生产deadline。Hub收尾heapMin38,600 B，loop2172/ipc01104/NimBLE2688 B，不冒充双child资源准入。
> 本批未替换生产Gateway/Broker镜像，未修改MCU生产源码或清原绑定；手机回列表。未暂存/提交/推送。S3/S4/100轮保持原边界，详见GATT§10.106。

> **2026-09-08 §10.105 历史：S2c-3b/3c空槽借还、目标隔离、两活动context接通，已开始四端真机。**
> 唯一GATT pool/permit按实际空闲槽借还，目标child独立保留准入；满槽Busy不驱逐A，删除固定A端口/访问入口。通用route/目录/管理不依赖GATT，未扩散SDK/队列/secret缓存。
> Host/ASan52、架构/347文件发布、两ESP32构建通过；内部产品Flash1,448,524 B/RAM75,140 B；已刷DualBoard Flash1,457,928 B/RAM75,228 B。
> COM11仅app0升级并校验，既有A的账户MQTT→Broker→Hub→BLE Manifest/State/Action/ACK通过，button计数0→1；permit扫描负测/exact replay/关闭通过。
> 单A与扫描短测heapMin72,812 B，ipc0/loop/NimBLE剩余1104/3640/2688 B；不是双child/TLS重连叠加准入。原Key/Access/绑定保留，COM12与COM3未刷，owner已释放。
> 下一批先对齐手机S1 APK（现场仍旧在线Direct路由），再用COM3建立B完成S2d双child门禁；S3/S4未完成，旧周期仍在，不恢复100轮。
> 本地Gateway公布地址/TLS代理已同步新网段；App/Service生产源码未改、未暂存/提交/推送。完整证据及边界见GATT§10.105。

> **2026-09-08 §10.104 历史：S2c-3a逐child管理与Hub permit生命周期解耦已接入实际Product。**
> 子mux/来源回包归context，删除其Hub permit操作；child与permit准入分开，整体reset仅一次permit清理，断网/订阅失败仍推进原超时和排空。
> Host/ASan52、架构/345文件发布、ESP32 3.3.11两构建通过；内部产品Flash1,446,272 B/RAM66,428 B（较上批-56/-8），context7512 B已含24 B mux。
> 下一步3b槽级策略与permit借用/目标准入→3c双业务工作区→S2d 1Hub+2child。实际Product仍单活动context，不把软件管理隔离称为双child已可控或无缝切换。
> 100轮暂停；未刷板/部署、操作ADB、改Key/绑定或暂存/提交/推送。App/Service仅同步文档；完整证据见GATT§10.104。

> **2026-09-08 §10.103 历史：S2c-2双原生Port槽及逐连接故障隔离的软件实现完成。**
> 只有SDK扫描/发起串行，已建立ACL独立保持；关闭/背压不影响另一Port，reset失效旧token并防止旧handle误断新连接。容量尊重SDK较小上限，无版本号绑定。
> Central29场景＋SDK单槽独立目标、Host/ASan52、架构/345文件发布、ESP32 3.3.11两构建通过。内部Flash1,446,328 B/RAM66,436 B（+344/0）；Host56 B/Port1068 B保持。
> Product/radio仍只接child0，尚不是双child业务或真机通过。下一步共享方案6.4：管理/permit作用域→逐slot radio→B纵向接线→1Hub+2child；不能复制两份旧radio覆盖同一admission。
> 100轮暂停，未刷板/部署、操作ADB/网络/Key/绑定或暂存/提交/推送；App/Service仅同步文档。详细资源/证据见GATT§10.103。

> **2026-09-08 §10.102 历史：S2c-1唯一Central Host及回调生命周期围栏已接入实际Product。**
> Port删除自有SDK生命周期/全局activePort，改为借用Host；7类SDK回调通过有界token登记解析，不保存Port地址，未关闭/未归还callback不释放容量。
> 中央Port18场景、Host/ASan51、架构/345文件发布及ESP32 3.3.11两构建通过。内部产品Flash1,445,984 B/RAM66,436 B（+1,540/+64）；Host56 B/Port1068 B，完整证据见GATT§10.102。
> 当前绑定容量2、Host登记容量2、原生活动容量1，三者不混淆。下一步S2c-2双原生槽/发起权隔离→B纵向接线→1Hub+2child真机；S3/S4与100轮暂停保持。
> 未刷板/部署、操作ADB/网络/Key/绑定或暂存/提交/推送；App/Service仅同步文档。本批不是双连接或无缝切换已通过。

> **2026-09-08 §10.101 历史：S2b-2b双记录后端与轻量binding已接入实际Platform/Product。**
> 保留0号NVS/codec，新增1号独立access/renewal；绑定接收不再依赖GATT工作区。resetAccess先清整个bank再清Hub Key；修正故障模型下清理重试NotFound遗漏commit确认。
> Host/ASan51、架构/344文件发布检查、ESP32 3.3.11两构建通过。内部产品Flash1,444,444 B/RAM66,372 B（+488/+424）；资源与证据边界见GATT§10.101。
> 下一步共享方案6.3：唯一Central Host→双原生槽→纵向B接线及1Hub+2child真机。当前2份持久记录/1个活动槽，B存储ACK不是Ready；100轮暂停，无刷板/部署或Key/绑定操作、暂存/提交/推送。

> **2026-09-08 §10.100 历史：S2b-2a目录管理分派与无副作用终态重投已接入实际Product/radio。**
> proof/revoke/renew先定位child，再交原协调器；Hub permit一份，来源child回复不走可变owner。全目录确认旧authority缺席才返回既有Finalized/Retired，无新账本/wire/MCU队列。
> Host/ASan50、Broker完整V2、架构/发布检查及ESP32 3.3.11两构建通过；产品静态RAM增加96 B，完整资源和journal阻断边界见GATT§10.100。
> 下一步S2b-2b平台多记录后端（保留0号NVS）→S2c共享Host/双原生槽→1Hub+2child验证。当前仍单记录对/单BLE槽，100轮暂停，没有部署、刷板或改Key/绑定、暂存/提交/推送。

> **2026-09-08 §10.99 历史：S2b-1记录目录、非秘密身份索引与initial/renewal定向下发已接入实际Product。**
> 复用原持久record/journal，不另建账本；墓碑和孤立renewal占槽，错误身份不覆盖其他child。索引在任何写/清除尝试前失效，route暖匹配不读Flash，Bridge实际鉴权保留。
> Host/ASan50、架构、Arduino发布文件检查及ESP32 3.3.11两项构建通过；静态RAM较S2a增加224 B，暖identity自身静态栈帧416→32 B，完整资源证据见GATT§10.99。实际平台与BLE仍单槽，没有真机双连接、部署、刷板、改Key/绑定或暂存/提交/推送。
> 下一步S2b-2有界多记录后端＋管理控制分派，先冻结墓碑清除后的迟到terminal幂等，不能猜测空槽；再共享Host/双活动槽。100轮仍暂停。

> **2026-09-08 §10.98 历史：S2a子设备上下文与共享MQTT路由端点已接入实际Product。**
> 原Product逐child字段/组装已移入无平台依赖的GATT上下文；共享一个MQTT订阅，每child独立有界入口，未知/歧义及跨child消息不分派。
> Host/ASan50、架构、Arduino发布文件检查通过；ESP32 3.3.11内部产品及带背压probe的双板Sketch均编译通过。MCU ABI共享route280 B、child入口736 B、活动上下文7544 B（含入口；不含SDK/TLS/Port）；构建尺寸见GATT§10.98。
> 仍是物理单BLE槽和单持久记录，下一批S2b目录/管理分派→共享Host/双活动槽；未刷板/部署/改Key或绑定，未暂存/提交/推送。100轮暂停。

> **2026-09-08 §10.97 历史：暂停100轮，启动网关共享连接与多子设备主线。**
> [正式方案](DEVICE_V2_GATEWAY_SHARED_CONNECTION_IMPLEMENTATION_PLAN.md)已冻结S0～S5。S0/S1软件完成：在线网关页默认Cloud代理、页面只清理自身需求；后续连接池/本地接入未实现。
> App通信/页面385＋服务边界32项、本地Web构建、设备Host重建/50项CTest及架构检查通过。未安装APK/部署Broker/刷板，未暂存/提交/推送。
> 下一步S2抽取被Product实际消费的child上下文、exact路由/管理分派，再接共享Host/2-child连接池。新增COM3已枚举，身份及固件待核对，不重置现有child。
> 旧Hub周期与有界离线Direct暂保留；不宣布7～16 s空档已消除，不部署或更换SDK/Key/绑定。以下§10.96为历史门禁。

> **2026-09-08 §10.96 历史：需求租约/物理承诺分离已实现；定向故障、持续页面、管理及完整10/30轮通过。**
> App根据Ready剩余期自适应续租，Broker GATT新承诺上限10 s，需求TTL/普通readiness/wire最大值仍30 s；旧承诺不提前清空。
> Host50/App380/工具126/Broker98+完整V2通过；APK dc755c23先安装，再部署Broker f5；设备71e82b72、SDK/Flash/RAM/Key/绑定不变。
> 单次/重复MQTT换代的原30 s请求均恢复，授权前断线无正Decision/Yielded/native连接；30轮457.509 s通过，Ready3.983～4.510 s、原Cloud owner逐轮恢复。
> 末尾取消native0次；原独立100（约25～30 min）安排已被§10.97暂停，不是发布长稳/D4b/N2～N5已完成。
> 最终App零scope/link/session、Broker owners0、Hub wanted0/permit0；ipc0/loop/NimBLE1104/3656/2700 B，临时观察器/串口均已结束，未暂存/提交/推送。
> 本批未新增协议字段、MCU状态或调度器；详见GATT§10.96/需求合同6.34。

> **2026-09-08 §10.95 历史：两个实现缺口已修正；短2/10通过，30在跨MQTT换代恢复时未通过。**
> 旧固件带追踪30轮前29轮通过，第30轮Cloud失败；Broker需求先于caller超时约21 s到期，未用加大timeout处理。
> App保留同一owner timer直到单调due；设备拒绝旧投影，不退役新attempt。均有旧实现失败/修正版通过回归。
> Host50/App376/工具122、架构及ESP32/Android构建通过；新Hub71e82b72/APKee981b3e原位更新，child与凭据未改。
> 新30第5轮Cloud renew持续accepted，但MQTT换代后旧Direct物理承诺仍到原deadline；只余约0.31 s完成连接/同步，触发30 s超时。
> 不可clear旧承诺破坏安全性；下一步定向换代测量→冻结恢复预算/取消合同→完整30→100。D4b/N2～N5/发布长稳仍未完成。
> 详细证据与下一步见GATT§10.95/需求合同6.33；不延长原readiness30 s或挑选避开刷新时间凑通过。
> 新版原管理窗口正向/取消两项35.907 s通过；Broker95+完整V2通过，最终App与Hub零连接需求残留，串口/临时观察器均结束。
> 本批没有Service/Broker生产修改、新协议、Action、重绑、暂存、提交或推送。

> **2026-09-08 §10.94 历史：F4真实管理窗口负测和10次通过；30次第9轮Cloud恢复超时，已停止。**
> 原permit-join两次实际Ready/relayActive，窗口关闭前不发Ready/不native连接；Direct先取消不关闭管理owner，窗口关闭后无迟到连接。
> 10轮普通页Ready4.011～4.193 s，单connect/close+OS释放+原Cloud owner恢复全通过；末尾另一次取消，counter0。
> 30次前8轮完整通过，第9轮App退出正常、Cloud在既有30 s预算内未恢复；保留原失败，不延长超时或自动重试。
> 资源ipc0/loop/NimBLE1104/3612/2708 B；本批Host50/工具122/Broker95/架构通过。
> 不改生产源码、固件、APK、SDK、绑定/Key/Access；没有Action、重刷child、暂存、提交或推送。
> 下一步凭readiness事实/错误码、Broker投影与Hub代次联合定位Cloud恢复；复现/修正后重跑30，再决定100。
> 详见GATT§10.94/需求合同6.32；management扫描窗口不等于所有grant传输负测，D4b/N2～N5/发布长稳独立保留。

> **2026-09-08 §10.93 历史：F4在线普通页两轮交替与Ready前取消四端短门禁通过。**
> 补齐Hub/Broker bit18准入、Gateway实际Admin epoch授权部署、Broker晚Hello初始化和设备DirectYield上行校验。
> 清理标准V2页外壳重复one-shot，子页单独拥有页面scope；隐藏同步取消，不新增SDK/调度器/Key/API。
> 真机Ready4.425/4.126 s；Cloud各约15.7/15.6 s后恢复。取消153 ms→Ready拒绝157 ms，native连接0次，counter全程0。
> Host/ASan50、工具115、App374、Broker95+完整V2、Gateway隔离PG和编译/架构通过。
> Hub78c8e6eb（Flash1,447,100 B/RAM65,628 B），APK81ee0809，原位安装；原绑定/Key/Access及child未改。
> **下一批先完成真实management占用/释放短负测，再10→30→100。** 工程bit18不等于公开产品/发布长稳准入。
> 详见GATT§10.93/需求合同6.31；现场手机首页零scope/link/session、Hub wanted0、MQTT管理正常；未暂存/提交/推送。

> **2026-09-08 §10.92 历史：F3b预算拆分、网络恢复及四端配对短测完成。**
> 唯一radio获取16 s/占用10 s/总26 s/真实空档6 s，无新Port API/协议/owner表，Flash+112 B/静态RAM+8 B。
> 手机普通离线页Ready9.988 s，Cloud完整可用窗口2.270→7.465 s；首次不可控仍13.837 s，同一owner/counter0保持。
> Host/ASan50项、工具112项、架构及ESP32编译通过。原计数RAM随USB断电归零，预检失败记录保留；未发Action凑基线。
> Hub当前F3b工程固件（8624a6c2）、mostfun2026已联网，手机Cloud页fresh/可控/退出零残留，子设备未重刷或重绑。
> **下一批F4：配套bit18准入与在线普通页单次交替，过短门禁再10→30→100。** bit18本批仍关闭，不冒称产品延迟/发布长稳通过。
> 详见GATT§10.92/需求合同6.30；App/Service业务源码与APK未改，未暂存/提交/推送。以下为历史记录。

> **2026-09-08 §10.91 历史：F3a首次四端配对短测通过；下一项F3b分阶段预算。**
> 独立电脑用户MQTT caller保持一个真实owner，手机真正无网普通页Ready9.948 s；Cloud不可控13.759 s后恢复，第二次周期让出后也恢复。
> 发现Cloud恢复后只剩2.270 s可用：当前同一10 s预算包含扫描/等待/认证。先修获取/占用分阶段，不能只加大常数。
> 112项工具及架构检查通过；新测试工具复用生产账户SDK/readiness，没有新增协议、owner表、Action或重绑。
> bit18关闭，F3性能/参数与长循环尚未完成。USB接回后已恢复Hub §10.87正常固件（4d6c522e，hash通过），手机账户ready/零残留。
> 现场改为mostfun2026/电脑192.168.1.141，旧基线尚未联网；下批先同步忽略的本机WiFi/服务地址/TLS证书，再做F3b，不宣称新网络四端恢复。
> 详见GATT§10.91/需求合同6.29；无暂存、提交、推送。以下为历史记录。

> **2026-09-08 §10.90 历史：F2b-C目录/Manifest隔离与独立离线普通页面短门禁完成；下一项F3。**
> 新目录投影限256项/128 KiB，按authority+auth.uuid隔离；不缓存秘密、用户资料、在线或State，不从凭据推断产品类型。
> 复用原模型转换；完整目录替换、共享撤销/页面退休、账号与请求代次、BLE旧回调有fence；旧无authority Manifest缓存不消费。
> 真实关闭手机两网络并撤除USB云通路后冷启动：目录恢复；两轮成功Ready3.318/3.025 s，到期SDK关闭53/55 ms，OS确认消失后空档6.039/6.049 s，零残留。
> 首次立即OS列表检查失败仍保留；新工具使用最多4 s观察，不混淆SDK回调/Android连接列表/空口时刻。
> 恢复网络后原Cloud页fresh/canControl/counter1，无Action、重置、重绑或NVS/Key修改；Hub仍10.87正常固件。
> 手机保留最终清理版新APK（SHA起始6f146fe2），不是回滚F2a旧包；原网络开关和USB API/MQTT转发均已恢复。
> App42文件357项、工具108项、类型/构建/架构通过。**F3持续Cloud caller+离线手机配对、bit18、10→30→100及发布门禁仍待验。**
> 详见GATT§10.90/需求合同6.28；无暂存、提交、推送。以下为历史记录。

> **2026-09-08 §10.89 历史：F2a 真实晚到/断开超时及重启恢复通过；F2b 接线完成，冷启动离线页面未通过。**
> 真实SDK成功后取消交付，原opening等待清理完成且不进入Ready；一次1 ms原生断开预算实际触发SDK超时，失败屏障阻挡继任者。
> force-stop/restart原App后Direct三轮恢复Ready2.910/3.796/3.653 s、每轮零残留；原Cloud页fresh/canControl/counter1。
> 新有限离线入口复用原caller group：明确原生无网、16 s获取、native获取起10 s占用、真实清理后6 s机会，重试/账号/网络恢复不退款。
> 清理原空Cordova网络实现，增加exact admission释放；无新协议/身份表/任务/MCU API。App33文件262/262、工具106/106、类型/构建/架构通过。
> 候选APK实测冷启动offline=true/account=idle，但设备目录为空，普通页面未挂载，30 s没有SDK connect；失败记录保留，不能算离线门禁通过。
> **下一步先做 F2b-C：authority+account 隔离的最小非秘密设备目录缓存，并修正ManifestCache命名空间；随后冷启动离线页面→F3→bit18/10→30→100。**
> 已恢复手机网络/USB转发、F2a已验证APK及原Cloud可控状态；Hub仍10.87正常固件，不含10/6工程周期。无新Action、清凭据或重绑。
> 10/6参数未冻结、F3/F4及D4b/N2～N5/完整WiFiProv/发布长稳仍待验。详见GATT§10.89/需求合同6.27。
> 本批无设备/服务器业务源码或镜像变化，无暂存、提交、推送。以下为历史记录。

> **2026-09-08 §10.88 当前：F1 本地 radio 公平机制及持续 Cloud 需求短测完成，F2a 关闭失败屏障已修正。**
> 唯一 radio 内增加有界占用/真实静默观察，不增任务、队列、协议、NVS 或第二套 owner；原管理事务仍按原预算排空。
> Host ASan 50/50（含 20 万 fuzz）、硬件工具 100/100、架构和 ESP32 3.3.11 编译通过；Flash/RAM 增量仅 +316/+8 B。
> 同一真实 Cloud 页、持续 executionWanted=1 已观察三次完整让出/恢复；但 10 s/6 s 工程候选导致每轮约 8.3～8.5 s 不可控。
> 因此参数未冻结发布，现场已恢复原正常 Hub 应用固件；原 child/Key/NVS/绑定保留，counter 仍 1，无新 Action。
> App 删除原生关闭失败被吞为成功的路径，取消 opening 等待晚到连接清理；失败屏障不因新对象或账号而清除。
> App 30 文件 239/239 与类型检查通过；真实 SDK 异常恢复尚未验证，不以软件负测冒充物理释放。
> 新 APK 已安装，三轮独立 Direct Ready 2.820/3.789/3.598 s，正常 close Promise 133/120/130 ms，全部零 link/session。
> 原 Cloud 页随后 fresh/canControl/counter1，离页零 scope、Hub Stopped/wanted0；原生晚到/拒绝仍仅软件负测。
> **接续顺序：F2a 异常恢复证据 → F2b 原 scope 有限离线机会 → F3 双向公平 → bit18/正常页 10→30→100。**
> F2b/F3 未完成，bit18 仍关闭；D4b/N2～N5/WiFiProv/发布长稳独立待验。详细证据见 GATT §10.88、需求合同 6.26。
> 本批无服务器业务/镜像/数据库变化，无暂存、提交或推送。以下为历史记录。

> **2026-09-07 §10.87 当前：Hub真实SDK关闭→授权广播恢复的三个有效短样本已完成。**
> 页面退出→广播1.024～1.156 s；GAP disconnect→手机收到通过PresenceKey验证的新nonce广播约37～106 ms。
> 采用前后时钟区间和真实SDK连接/扫描/连接过程查询，不以logical Stopped或固定广告字节冒充物理/身份事实。
> 已删除探针的固定nonce匹配；前两轮工具失败保留，后续3/3有效测量不是全部5次成功，也不是四端交替门禁。
> 仅Hub应用分区临时加入内部监听器，已恢复正常固件并验证原Cloud页fresh/canControl、counter仍1；Key/NVS/child绑定不变。
> 硬件工具100/100、架构及实际3.3.11编译通过。App/Service生产源码和APK/镜像不变，无Action/暂存/提交/推送。
> **下一批按需求合同6.25 F1→F2→F3：唯一radio有界让出、App有界离线机会、持续Cloud owner实测；随后bit18/正常页10→30→100。**
> 10 s占用/6 s空档仅工程测量候选，不是本批已启用的生产参数；完整WiFiProv/SDKstop负测/D4b/N2～N5/发布长稳仍待验。
> 详细证据见GATT§10.87；以下均为历史记录。

> **2026-09-07 §10.86 当前：WiFiProv名称/QR取消、扫描重启负测及十个正常Direct样本已完成。**
> 真机发现系统重启后仍限额且部分status6不回调；已用独立单调/boot小账本持久化5/30s，不保存身份/密钥，不给重启退款。
> 最终APK立即重启仍剩27.4s限额且未调用SDK；冷却后恢复。3+3+4批次共10个Direct样本Ready2.771～3.744s，中位3.598s，全关闭。
> 名称/QR取消/重叠Busy/交接与查找截止已验证；跨线程候选缓存保护完成。App272、Android7、工具93、类型/架构/构建通过。
> 原绑定Cloud页fresh/canControl、counter仍1；离页Hub Stopped/wanted0/managementSubscribed1。没有新Action、MCU固件/凭据/绑定或服务器部署变化。
> **不是连续count10或四端公平交替通过。下一步先测Hub SDK真实关闭→child广告恢复，冻结离线公平，再配套bit18/四端10→30→100。**
> 完整WiFiProv真实配网、SDKstop异常恢复和发布长稳仍独立待验；bit18仍关闭。无暂存/提交/推送，见GATT§10.86/需求合同6.24。
> 以下保留历史结果，旧“进程内账本”描述已由6.24修正。

> **2026-09-07 §10.85 当前：App共享扫描/原生失败及配额、WiFiProv扫描端口和附近状态期限已落地并真机短验。**
> App266、Android配额2、硬件工具86项通过；新APK已安装，设备/服务器未重新部署，账号/Key/Access/绑定不变。
> Direct5次成功后第6次50ms明确限额，独立恢复2782ms；原生WiFiProv/Direct互斥、旧stop隔离及Home后台停止通过。
> Cloud页fresh计数仍1、无新Action，离页Hub回Stopped/wanted0；不是10次全成功或四端公平完成。
> 下一步按需求合同6.23补原生失败/配网负测、真实广告恢复及离线公平，再bit18/四端10→30→100；bit18继续关闭。
> 无暂存/提交/推送；细节见GATT§10.85。以下为历史记录。

> **2026-09-07 §10.84 当前：新Hub/App/Broker已部署，按需Cloud物理唤醒/释放短测通过；Direct 10次门禁未通过。**
> 新网络下只刷Hub application，保留NVS/Key及child原绑定；进入页唤醒、唯一Button0→1、离页Stopped，再进页fresh1，再离页Stopped。
> 新App Direct基线前5次Ready2.845～3.859s，第6次Android报告扫描过频；Hub伴随观测未抢连，不是本次ESP32认证失败。
> 硬件工具82/82（新工具7项）、Broker151/151及架构/编译/安装通过；3.3.11 Flash1,446,616 B、RAM65,612 B，实测ipc0低水位1104 B。
> **下一批先修共享扫描start/stop屏障、原生失败通知及配额预算，再测10次和广播恢复，最后公平合同/bit18/四端10→30→100。**
> bit18仍关闭、离线公平和长稳未完成；没有改三端业务源码、重置child/清Access/重绑、暂存/提交/推送。详见GATT§10.84、需求合同6.22。
> 本条替代下面历史“尚未部署/idle未启动”的现场状态，以下保留为历史证据。

> **2026-09-07 §10.83 当前：实际App selector、共享Direct scope及native准入/关闭屏障软件完成。**
> 受管child先预留并等待Ready，再认证/同步；多页面/read复用原group的一个物理owner，最后退出才释放。
> 删除cloudReachable=false旁路及后台维护/command的受管child隐式重连；未知Action不切Cloud重放，账号/Session退休不迁移原工作。
> SDK迟到成功、重复关闭、旧generation回调及退出账号后异步snapshot均已覆盖；native模块只接中立准入检查，不耦合MQTT/UI。
> App44文件382/382、Broker151/151及完整回归、跨仓35/35、Host50/50、架构/类型检查通过，详见GATT§10.83/合同6.21。
> **下一步只补离线手机/在线Hub持续需求的本地公平测量与合同，再配套bit18激活和同绑定四端10→30→100门禁。**
> 设备及Service/Broker业务源码本批未改；bit18仍关闭，无部署/烧录/reset/重绑/暂存/提交/推送；软件测试不冒充真机。以下为历史记录。

> **2026-09-07 §10.82 当前：App Ready查询、Broker新鲜投影和原owner/scope消费的软件链路完成。**
> App/Broker bit19、0x28/0x29独立于reserved/accepted和设备bit18；复用原identity、owner、timer、replay及工作上限。
> Ready期限从query发出时间保守计算；renew accepted不续物理承诺，旧revision/账号/Hub/权限或迟到回应不能放行新获取。
> Angular274/274、Broker相关151/151及完整回归、跨仓35/35、Host50/50/架构与类型检查通过，见GATT§10.82/需求合同6.20。
> **gateway-child页面抢连保护仍在，实际BLE selector尚未接新入口；下一批只补selector/Direct scope、本地离线公平及配套四端门禁。**
> bit18仍关闭；无MCU/APK/镜像部署、reset/重绑或暂存/提交/推送，MCU源码本批未改；软件验证不冒充真机。以下为历史记录。

> **2026-09-07 §10.81 当前：唯一 GATT 物理策略/管理准入/离线释放软件门禁完成。**
> ESP32 product 删除 Admission 自动连接及断云无限持有；普通、管理、permit 与 native closing 使用同一策略。
> exact continuation 保留原事务预算；真实 quiescent 后锁获取并发 Yielded。MQTT 退休保留原剩余退让；冷启动增加一次 30 s 获取保护。
> receiver 56 B；Host50/50、跨仓34/34、200k fuzz/架构通过，MCU 资源详见 GATT §10.81。
> 下一批：App Ready 合同/消费，补手机离线但 Hub 在线持续 demand 的本地公平，再配套 bit18 激活和同绑定四端交替。
> bit18 仍关闭；未烧录/部署/reset/清 Access/改 Key、暂存/提交/推送；100 轮仅软件模拟，不冒充真机。以下为历史记录。

> **2026-09-07 §10.80 当前：Broker Direct生产driver、owner显式续期与准入生命周期接线完成。**
> 只读投影共用原owner/并发额度；新revision必须新Yielded，旧Ready不延长。控制计数保留在既有MQTT claims中。
> 同MQTT重新准入使旧binding失效但不重用编号；child Hello/closure/恢复保持原authority；错误Hub绑定拒绝。
> Broker/协议165项、完整回归、跨仓20项、Host49/49/架构通过；详情见GATT§10.80、需求合同6.18。
> 下一批直接做唯一product物理决策/permit入口/离线公平，再App Ready消费与四端交替；bit18仍关闭。
> 设备和App业务源码本批未变，无部署/烧录/reset/清Access/改Key、暂存/提交/推送；没有新真机结论。以下为历史记录。

> **2026-09-07 §10.79 当前：Hub Direct控制wire、receiver租约、RouteBridge适配与Broker单次交换核心完成。**
> 独立kind5/12项≤200 B；48 B receiver复用原lease，Probe绑定当前Hub incarnation，回执不续期/不复活取消状态。
> Host49/49、200k含新seed的fuzz、协议22+Broker70项、跨仓20项、架构与完整Broker回归通过；资源结果见GATT§10.79。
> 生产owner→Hub driver及renew、唯一物理决策/离线公平、App Ready消费仍未接线，bit18不启用，不宣称已释放或物理idle完成。
> 下一批只补这些生产接线与同绑定四端门禁，不重做wire/lease/owner；细则见需求合同6.17。
> 没有部署/烧录/reset/清Access/改Key、暂存/提交/推送，现场仍为§10.76 counter10/revision11。以下为历史记录。

> **2026-09-07 §10.78 当前：Direct预留wire、真实authority和App Session发送接线完成。**
> bit17/0x1e/0x1f独立于cloud；真实Direct Admin+Active child route/epoch+exact Hub MQTT/admission授权，共用owner生命周期。
> 未支持bit18的Hub拒绝；cloud分享不等于Direct权限；accepted不是Yielded，页面和设备物理调度尚未激活。
> Broker57项/整体回归、App66+44项、跨仓17项、Host48/48/架构、实际PostgreSQL隔离测试通过；详见GATT§10.78和需求合同6.16。
> 下一步是Hub receiver/真实释放回执→唯一物理决策/在线离线公平→App消费→同绑定四端Direct交替；不再重复预留wire。
> 本批未烧录、重置或部署，真机基线仍为§10.76 counter10/revision11；未暂存/提交/推送。以下为历史记录。

> **2026-09-07 §10.77 当前：Direct优先权语义及共享owner核心的软件子批完成。**
> App/Broker复用同一生命周期与容量治理，分离cloud与Direct；新增Direct单候选、绑定/权限期限及禁止用途转换。
> 旧ConnectionDemand编码和普通read authority明确拒绝Direct，避免未接线时静默降为cloud；UI/wire feature/Hub策略未启用。
> Broker51项及整体回归、App最终63项、跨仓13项、原设备Host48/48与架构通过；细节见GATT§10.77。
> 下一批完成Direct独立wire/真实authority/Hub Yielded回执，再配套唯一物理决策和App消费；不重写owner核心。
> 本批没有新固件/APK/镜像部署或真机Direct交替；沿用§10.76 counter10/revision11。未暂存/提交/推送。以下为历史记录。

> **2026-09-07 §10.76 当前：共享连接机制接线与四端短门禁通过。**
> ordinary/proof/revocation/renewal共享同一连接入口；RouteBridge不再自行重连，管理关闭屏障独立于命令reset。
> 修正仍Secure时越过退役屏障及同步关闭后同次调用隐式重连的边界；不新增任务、堆、Action队列或用户API。
> Host ASan48/48、20万fuzz、架构/11项工具测试通过；3.3.11最终Flash1,443,032 B（+260 B）、静态RAM65,492 B（+32 B）。
> 仅烧录Hub应用：普通permit relay、子设备主动断链Rejected及新窗口恢复通过；原准入同MQTT恢复generation3。
> 实际页面Button9→10，相同Command重传只执行一次；页面/后台/Session更换/one-shot通过，最终revision11、scope0/owner0/wanted0。
> Inspector已独立确认关闭，App回列表；未重置子设备、清Access、重绑或刷新凭据，未暂存/提交/推送。
> **下一批先冻结Direct独立有界优先权合同，再同批配套在线/离线公平及物理idle。** permit-join和清其他owner需求不是Direct退让。
> 当前仅保留product一处Admission连接决策；完整按需调度、Direct交替、N2～N5、D4b及发布长稳仍未完成。
> 详见GATT§10.76、需求合同6.14；当前counter基线10。以下为历史记录。

> **2026-09-07 §10.75 当前：permit-join双向异步交接及四端短门禁通过。**
> Opening按原命令期限等待；Closing先撤收发、后等原生完成归还；reset和无效管理消息不再阻塞清理。
> 临时radio占用保留有效路由准入，抽出不start/connect的`pollControlPlane()`，关闭窗口后不靠重新绑定或无限Admission重发恢复。
> Host ASan48/48、20万fuzz、架构及11项工具测试通过；最终3.3.11应用Flash1,442,772 B、静态RAM65,460 B（+8 B）。
> 只更新Hub应用，复用原App/Broker/child：普通relay同步、主动ACL退役后的Rejected与新窗口恢复均通过，三个短场景无连接/检查点残留。
> Broker收到旧Closure→新Hello，同MQTT自动恢复；最后真实页面Button8→9及完整scope回收通过，最终revision10/owner0/wanted0。
> App已回列表、scope0，Inspector确认关闭；无子设备reset/清Access、重新认领、Key或Gateway凭据轮换，未暂存/提交/推送。
> **下一批集中ordinary/proof/controller-control唯一物理编排，再配套Direct在线/无云公平退让。**
> 当前物理idle尚未启用；不是Direct→Gateway→Direct或10→30→100次长稳通过。N2～N5、D4b、SDK首因与Wio/Hub长稳仍待办。
> 详细版本、证据与边界见GATT§10.75、需求合同6.13。以下为历史记录。

> **2026-09-07 §10.74 当前：物理释放边界修正，Host 48/48、20万fuzz及架构检查通过。**
> GATT Link先退役记录、原生结束再通知会话释放；ChildSession故障保留exact attempt，旧回调不影响新代。
> ESP32修复取消与连接成功竞态、取消/终止失败的假Idle、stop遗漏native connecting；SDK桩测试实际生产Central实现。
> 最终ESP32 Flash 1,441,624 B（较§10.72 +168 B），静态RAM 65,452 B不变；仅Hub应用升级，未改child/Access/Key或三端wire。
> 真机版本与分场景结果见GATT §10.74；App/Broker沿用已验证基线，无业务源码/镜像/APK变更。
> 最终Hub普通断开后1.377 s观测Secure，真实页面Button 7→8及完整scope回收通过；最后owner0/wanted0、revision9/计数8。
> 两次烧录均只写Hub应用；手机已回列表、scope0，Inspector已关闭。不是物理idle/Direct或长稳通过。
> **物理调度未完成：下一批优先permit-join正反向异步交接，并让openWindow/WouldBlock在原命令期限内等待；**
> 再集中ordinary/proof/controller连接入口，与Direct在线/无云公平退让配套后才启用idle。需求合同6.12已注明不能独立修改的联动点。
> 不新增候选模式或Action队列；未暂存/提交/推送。以下为历史记录。

> **2026-09-07 §10.73 当前：真实页面需求消费/有界回收短门禁完成。**
> 最终APK在Android+双XIAO+本机Service/Broker通过Button 5→6、退出、后台/恢复、账户Session更换和one-shot只读。
> App修正pause提前取消、初始getState竞态及监听器清理；没有新增协议/owner核心/后台MQTT或更改MCU源码。
> 后台30.087 s观测owner0，39.481 s包含Hub串口取样；最后zero Decision与Hub wanted0确认，不能当作立即释放或物理idle。
> App35 suites / 272 tests、相关工具32项、跨仓21项、ESLint与最终APK构建通过；详细证据见GATT §10.73。
> **下一批直接实现唯一物理编排+Direct公平退让**，包括管理会话所有权、异步断链占槽、取消wake与Cloud刷新/离线分支。
> 不重做已完成的codec/owner/预算，不提前开启idle；N2～N5、D4b、SDK首因及发布长稳仍未完成。未重置两板、暂存/提交/推送。
> 以下为历史记录。

> **2026-09-07 §10.72 当前：App页面/只读同步需求作用域已接入，Hub/Broker已配套部署并恢复mostfun2026。**
> 页面持有既有Session owner至隐藏/后台/销毁；one-shot结束释放，列表/被动监听不再隐式连接；不重排/重发Action。
> App266、Broker Windows/Linux102、跨仓21、相关硬件工具27项及APK/ESP32编译通过。Hub/child真实Secure与route ready恢复，后台零需求负测通过。
> **手机仍锁屏；真实前台page/Action/退出回收未通过。物理idle与Direct退让仍未实施，不得提前开启。**
> 下一步解锁手机直接复测，不重置子设备；再实现唯一物理编排及Direct公平性。未暂存/提交/推送。细节见GATT §10.72与需求合同6.10。
> 以下为历史记录。

> **2026-09-07 §10.71 当前：0b3b2准入送达/首Hello恢复分离软件子项完成。**
> 初次Challenge确认authority但不制造ready；首Hello由实际安全会话锚定两轮23 s，Admission重复不重开耗尽预算。
> 初始确认最多两batch；协商失败保留已确认authority/execution、清理投影，非零恢复floor仍要求新Hello。
> Broker Windows/Linux102、Host ASan47、跨仓18、20万fuzz、架构及ESP32-S3编译通过；静态RAM不变、Flash增48 B。
> **下一步为物理调度+页面/只读sync需求+Direct退让配套实现。** 不能只开gate，否则当前页面没有owner而无法连接。
> 未烧录/reset/部署/暂存；最后现场版本仍为§10.69。详细边界见GATT §10.71、连接需求合同6.9。以下为历史记录。

> **2026-09-07 §10.70 历史：0b3b2管理会话占用软件子项完成。**
> 修正nonce无独立TTL；三个协调器共用8 B本地命令预算，Prepare按exact鉴权attempt保留连接，重发/发布不续期。
> mux区分busy、待发结果与会话所有权；轮换后exact清理支持背压/异步完成，不误断后来会话。无新wire/用户API/任务/队列。
> Host ASan47/47、20万fuzz、架构及ESP32-S3 3.3.11编译通过；静态RAM增32 B。详细范围见GATT §10.70和连接需求合同6.8。
> **Admission/首Hello预算拆分、物理调度、idle/Direct及页面作用域仍未完成。** 未烧录/reset/部署/暂存，现场仍为§10.69。
> 以下为历史记录。

> **2026-09-07 §10.69 历史：需求控制真实四端短门禁通过，下一项仍为0b3b2物理编排。**
> 新Broker shard-1/Hub应用/Android已配套部署；两个owner、部分释放后续租、全部释放、ACK前取消、session结束通过。
> 真机force-stop并全丢zero Decision，Broker约1.7 s观测owner=0、Hub约30.2 s观测执行期限归零；业务不受影响。
> 正常断链1199 ms secure、2605 ms观测ready，App Action46→47；child Access摘要不变，未重认领/清存储。
> **不是物理idle/Direct完成。** 下一批拆Admission/首Hello预算及NonceReady后的管理会话占用，再替换eager编排。
> 软件Broker97、工具66（Windows/Linux）、App26、Host ASan47通过；DualBoard Flash1,440,236 B/globals65,420 B。
> 当前权威topologyVersion=4、credentialVersion=185；勿混用“version4”。详细哈希、失败边界和水位见GATT §10.69；未暂存/提交/推送。以下为历史记录。

> **2026-09-07 §10.68 当前：L2c-0b3b1实际南向控制接线完成。**
> owner通知、Broker dispatch与Hub Bridge期限核心已接通；权限/执行状态独立于Hello/closure/可达投影，无新增持久化或任务队列。
> Host ASan47/47+20万fuzz、shared protocol21+Broker97（Windows/Linux）、跨仓18/18、ESP32-S3编译/TS/架构通过。
> 内部Hub例程静态RAM65,388 B（增48 B），Flash1,431,784 B（增1,628 B）；不是无线运行资源测量。
> **物理调度/page自动需求/idle/Direct仍未启用。** 下一步0b3b2拆Admission送达/首Hello预算、management独立有界所有权，再配套替换自动重连。
> 本批未部署、烧录/reset或暂存，运行仍为§10.64。控制/容量/顺序见[连接需求合同6.7～7节](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。以下为历史记录。

> **2026-09-06 §10.64 当前：L2c-2b 配套真机可达租约短门禁通过。**
> 发现并修正真实 RouteClient 发送白名单漏接proof；正常三周期、首challenge/proof丢失、loss-all及旧proof回放通过。
> 全丢失到期撤下投影，MQTT/Active保留；显式恢复后真实App Action45→46，没有重复执行。
> Host ASan46/46+20万既有fuzz、Linux Broker47/47、工具62/62（Windows/Linux）、架构/ESP32编译通过。
> 最终Hub Flash1,437,704 B、globals65,372 B；配套Broker shard-1已部署，详细hash/水位/失败修正见GATT §10.64。
> **下一步L2c-0b owner demand/wake/release，再idle/Direct退让；尚未完成L2c整体和发布长稳。**
> App源码/APK未改，未清除Access/重认领/写eFuse/暂存/提交/推送。合同见[连接需求与可达租约](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。以下为历史记录。

> **2026-09-06 §10.63 当前：**L2c-0a 边界收口、L2c-2a loss-all 可达租约软件完成；先安全底座，再 demand/idle。
> 独立 Hub/Broker challenge/proof，不增加 child BBP、持久字段或连接需求；30 s 单调租约到期退役投影，不重放 Action。
> Host ASan46/46+20万现有fuzz、协议20/20、Broker47/47及既有总门禁、硬件工具55/55、架构/TS通过。
> 新 Hub/Broker 必须配套；尚未部署或做新版本真机验证。App源码/普通child API不变，idle/Direct交替仍待实施。
> XIAO ESP32S3 / Arduino3.3.11编译：Flash1,437,648 B（+748），globals65,372 B（不变）；固件hash见GATT §10.63。
> 主入口：GATT §10.63；细则：[连接需求与可达租约](DEVICE_V2_GATEWAY_DEMAND_REACHABILITY_CONTRACT.md)。以下为历史记录。

> **2026-09-06 §10.62 当前：**L2b-4c2 App共享30 s ready等待及真实恢复/失败门禁通过，设备/Broker生产代码与版本不变。
> 本批收口：每调用者取消、账号代次隔离、缓存页面控制门禁、unknown不重放、主动/推送State单槽所有权与最高revision补读。
> Angular31套件224/224、定向106/106、工具54/54、ESLint/编译/APK通过。真实等待2619/2521 ms、两轮耗尽30006 ms超时；Action43→44→45。
> 真机三次控制前失败查明为State并发覆盖，修正后通过；不延长Command预算、不吞错误。细节和版本证据见GATT §10.62。
> **下一批L2c-0先冻结idle/连接需求/唤醒/可达租约，再L2c-1释放与Direct↔Gateway、L2c-2全部通知丢失收敛。**
> waitUntilReady不等于唤醒接口。Hub仍保持ACL；多child/Mesh/History/Bulk及D4b/SDK根因/长稳不提前宣称完成。未暂存/提交/推送。以下为历史记录。

> **2026-09-06 §10.61 当前：**L2b-4c1独立GatewayRouteClosureV1及四端短门禁完成；通知只退役exact volatile projection，保留Active/Access。
> Hub按0/1/3/7 s、11 s截止重传，复用既有workspace/Hello计时；新Hello不等待receipt，旧notice/receipt不伤新ready。
> Host46/46+20万现有fuzz、协议19/19、Broker36/36（Windows/Linux）、工具53/53（Windows/Linux）、架构/ESP32编译通过。
> 新Hub Flash1436900 B（+1564）、globals65372 B（不变）；仅配套替换本机Broker shard-1及COM11 Hub应用，固件/镜像hash见GATT §10.61。
> 真实丢首个receipt后相同notice重传，旧通知回放及新Action41→42通过；正常冷恢复42→43。App离线ensureReady拒绝，root/拓扑不变。
> **下一步L2b-4c2 App共享可取消30 s独立ready等待，后续L2c idle/按需/Direct交替及loss-all租约收敛。**
> App源码/UI/APK本批未改，未宣称全部丢包时仍保证离线或完整L2b/长稳完成；未暂存/提交/推送。以下为历史记录。

> **2026-09-06 §10.60 当前：**完成真实Hub入口1000 ms到期及两轮协商失败保持离线两项负测。
> 单Action四次exact发送、两次槽到期1000/999 ms，button40不变；MQTT/BLE不重连，ledger保留outcome_unknown而非伪造拒绝。
> 两轮Manifest请求各4次，45秒无第三轮/无Action，App离线ready拒绝；未清root/重新认领。
> composition的RouteClient改为默认静态依赖，故障receiver仅在tests显式编译；无生产队列/超时/wire/API变更。
> Host ASan46/46+20万fuzz、工具52/52（Windows/Linux）及架构通过。正常固件恢复与哈希以GATT §10.60.4为准。
> 已恢复正常Hub（Flash1435336 B、globals65372 B）；再冷恢复ready2451 ms、控制/回读3219 ms，40→41且旧unknown不变。
> 下一批L2b-4c：exact失联通知、独立有界ready等待及unknown错误语义，再idle/Direct交替；不将本次终态子门禁视作整体发布通过。
> 本批未暂存/提交/推送，App/Broker生产源码和部署不变。以下为历史记录。

> **2026-09-06 §10.59 当前：**L2b-4b2完成独立Broker冷恢复/首次ManifestRequest丢失，以及运行期State/Command ACK组合丢失门禁。
> Broker统一复用有界只读exchange，精确隔离State与Command响应；Hub删除MQTT reset时错误重置child sequence，避免新Action误用旧ACK。
> 序号归南向会话、准入归MQTT，未新增wire/API/设备队列。Host ASan46/46+20万fuzz、Broker32/32、工具50/50、协议18/18通过。
> 最终Hub/Broker/APK三次冷恢复ready2513～2655 ms，控制/回读3194～3389 ms；首次ManifestRequest丢失3619/4333 ms。
> Broker重启但BLE保持时Action33→34；组合丢包34→36，后续四次冷恢复36→40。Active拓扑及root摘要不变，无resetAccess。
> Hub Flash1435344 B（-8 B）、globals65372 B（不变），仅应用更新；child/App源码和APK不变。新shard1及哈希见GATT §10.59。
> 下一步先补真实1000 ms入口占满到期，再L2b-4c失联通知/有界ready等待，最后idle/Direct交替；不关闭整条L2b或发布门禁。
> 未暂存/提交/推送。以下为历史记录。

> **2026-09-06 §10.58 当前：**L2b-4b1完成App State-only/并发及同步重入去重、Hub在途容量背压、Central15 ms参数。
> 不增加wire/队列/ATT缓冲，保持MTU23、Write With Response、indication及DirectSecure。热连接A/B ACK1372→581 ms（各1次）。
> 最终Hub+APK三次App账户Presence冷重连：2272～2472 ms重新在线，2955～3186 ms完成控制/回读，计数21→24；
> 每次离线ready拒绝、凭据摘要/Active拓扑不变。该观测不是独立Broker投影/同MQTT审计，不代替丢包或长稳门禁。
> Host ASan46/46+20万fuzz、App8套件74/74、hardware49/49（Windows/Linux）、架构/类型检查/local build/ESP32编译通过。
> Hub Flash1435352 B、globals65372 B；仅应用烧录，child未改，无Access重置。最终APK哈希和测试定义详见GATT §10.58。
> 下一步L2b-4b2补Broker冷恢复/丢包与runtime State/Command竞争，再失联通知/ready等待，最后idle/Direct交替。
> 未完成门禁不提升发布状态，未暂存/提交/推送；以下为历史记录。

> **2026-09-06 §10.57 当前：**L2b-4a 已实现Host单一单调时钟冷连接门禁，三次正常新ready7.039～7.969秒，
> 控制/回读总计9.065～9.968秒；首次ManifestRequest丢失8.117/10.170秒。是短样本，不是p95或发布长稳。
> 修正App缓存完整却忽略Presence=false的问题：拒绝新Command、保留页面缓存、在途结果不改写、不增加Action队列。
> App66/66、类型检查/local构建，硬件工具49/49（Node22/隔离Node20）与架构检查通过；设备/Broker生产代码不变。
> 新debug APK已保留数据覆盖安装，真实离线ready检查拒绝；恢复后15→16，冷控制/回读9945 ms，凭据及root不变。
> 完整安装复测记录及边界见GATT §10.57；下一批L2b-4b先查分片往返/重试及State/Command仲裁，随后收口失联通知/等待，
> 再idle/Direct↔Gateway。未清Access/重认领、未暂存/提交/推送；以下为历史记录。

> **2026-09-06 §10.56 当前：**L2b-3c Hub/Broker已配套部署，双XIAO+真实Android通过admission/Hello/首次下行丢失、
> 同MQTT投影退休/恢复、在途Action ACK丢失及一次bootstrap耗尽自动恢复。旧Action跨代注入没有重复执行；
> 最终button11、revision12，Active/version4/epoch494518246及child root摘要不变，未resetAccess。
> 没有增加生产测试接口，App源码/UI/APK不变；工具40/40（Node22/Node20）、新镜像Broker30/30、实际Redis及架构门禁通过。
> 故障钩子已恢复，Inspector已关闭。**下一步L2b-4完整ready测时/等待与失败语义，再做idle/Direct交替**。
> 本批不是正式发布/长稳/SDK assertion修复证明；现场与证据边界详见GATT §10.56。未暂存/提交/推送，以下为历史记录。

> **2026-09-06 §10.55 当前：**L2b-3b软件门禁通过：Admission V2携带MQTT级retiredGeneration，重复请求不重建新投影，
> Hello原样4次/11秒截止，Broker admission30秒/一次自动恢复；不改变Access或重放unknown Action。
> 抽出公共有界exchange调度，旧独立重试器/V1类型已清理。Host ASan46/46、20万fuzz、架构/golden、
> Broker30/30（Node22/隔离Node20）、协议18/18通过，Redis专用SKIP。
> **未配套部署、烧录或重置绑定**，当前运行仍§10.53；不是四端真机恢复验收。admission最大200 B（+9 B），
> 数据帧及公开API不变；ESP32编译结果见GATT §10.55.3。下一步先部署/故障注入/冷连接测时，再ready预算，最后idle/交替。
> 以下为历史记录。

> **2026-09-06 §10.54 当前：**完成L2b-3a Broker首次Manifest/State协商单请求重试：≤64 B frame +16 B correlation，
> 四次发送、全分页30秒总期限，精确匹配/重复页丢弃、权限和旧回调隔离；不缓存Action。
> lifecycle **25/25**（Node22/隔离Node20）、完整Broker套件、协议 **18/18**通过；Redis专用测试本批SKIP。
> 设备/App源码和wire不变，无MCU资源增量；未部署、烧录、操作真机或重置绑定，运行版本仍§10.53。
> **下一步先补首个Hello丢失/同MQTT context退役恢复，再协调冷连接预算，最后idle/交替。**
> 本批只完成协商中的重试，不宣称完整bootstrap自动恢复。详见GATT主线§10.54；以下为历史记录。

> **2026-09-06 §10.53 当前：**L2b-2 先收口路由投影代际：复用16 B correlation 的前8 B作单调代际，
> synthetic Hello 以bit14显式声明；不增帧长度/队列，Hub/Broker须配套更新。新代际退役旧命令为unknown，
> 不跨代重发Action；重新Manifest/State后才reachable。旧State/ACK/异步回调不能污染新projection。
> Host ASan **46/46**、20万fuzz、架构/工具 **31/31**，Broker lifecycle **14/14**（Node22/Node20）、
> 既有Broker/实际Redis及协议 **18/18**通过。当前绑定保留，未resetAccess；部署/真机结论以
> [GATT §10.53](DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md) 为准。
> 仍未启用idle：同代bootstrap丢包/context恢复、冷连接预算、Direct↔Gateway交替及发布门禁不能提前关闭。
> 最终Hub应用与shard1已配套更新，Flash/globals **+248/+8 B**。同绑定热控制 **4→5**，正常BLE断连后
> 3516 ms再Secure；同MQTT内protocolSessionId换代、控制 **5→6**，App无残留、child access摘要不变。
> 以下为历史记录。

> **2026-09-06 §10.52 当前：**完成 L2b-1 Broker 命令 reservation/终态清理；旧回调与错误 correlation
> 不清新槽，unknown Action 不自动重发。新增 **9/9**（Node22/Node20）、既有 Broker/Redis 限流及协议
> **18/18** 通过，已更新本机测试 shard1；未更改设备/API/wire，不增加 MCU RAM。
> 本轮按明确授权仅 reset COM12 child access 一次；同身份 epoch494518246，原生 button **2→3**，
> 新 `45b92507779a943cc939cdac4a25323f` Active/version4，真实 account MQTT button **3→4**；App 无残留。
> Hub/child 固件、APK 不变。仅未复现上批 assertion，未关闭；普通 route 仍占 ACL，**冷恢复/交替未完成**。
> 当前现场及下一步以 [GATT §10.52](DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md) 为准：
> 先补冷 Hello/state generation 和跨 BLE session 未确认 Action 失败测试，再做 idle release，不再次 reset。
> 以下为历史批次。

> **2026-09-06 §10.51 当前：**继续 GATT L2a，修复 MQTT → Bridge 的有界消息交接：WouldBlock 不清原帧，
> 只复用单槽，1 秒本地等待上限；换代/reset/解绑清理，每次重试校验 admission generation/durable revision。
> 普通/管理请求只有成功接受才消费 child sequence，合并 pending/recent exact replay 的重复发送分支。
> **未启用 route idle/cold-resume**，不宣称 Direct/Gateway 交替或 Action 未确认恢复完成。
> 按本轮授权只清 COM12 child access 一次，Hub 身份/网络/access 保留。同身份新 epoch213376463，
> 初次 Method2 超时且串口有 assertion/reboot；保留 pending 后正常恢复 active、原生 button **0→1**，
> 不能将恢复成功算作首次稳定通过。当前现场/最终构建和测试以 [GATT §10.51](DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md) 为准。
> 最终 Host ASan **46/46**、20 万 fuzz、架构/工具 **31/31** 通过。新 Hub 已 application-only 更新，
> **+324 B Flash/+8 B globals**；新 `9bf02e5a0a3ff936963b937782e408c2` Active/version4，account MQTT
> button **1→2**，App 无残留；child adv0/acl1，普通 route 仍占用，不是自动切换/冷连接验收。
> App/Service/Broker 本批不改生产代码；下批先处理 Broker 冷恢复/超时关联槽退出与 Action 结果未知合同。
> 以下为历史记录，不应重复 reset 已认领的子设备。

> **2026-09-06 §10.50 当前：**按本轮新授权只 reset COM12 child access 一次，原生 Direct 重新认领/
> 控制 **2→3**，新 epoch3032216448；旧 §10.49 topology 已正常 Detached，Hub access 未清除。
> L1 的 proof 所有权子项已先 RED 后 GREEN：成功/拒绝/过期释放自建 exact attempt，异步断连期间仍 busy，
> 回执 replay/cache reset 不误关其他模块连接；Secure 前再次校验 durable record。仅复用已有 runtime
> 合同，无新 wire/API/队列/SDK hook。Host ASan **45/45**、20 万确定性 fuzz、架构、工具 **31/31** 通过。
> 新 Hub 已 application-only 更新：Flash **+196 B**、globals 不变。真机 proof 结束后 child adv1/acl0、
> 连接/断开11/11；新 topology `4f84ebee1d286b51020dd659b2947521` Active/version4 后真实 account MQTT
> 控制 **3→4**。但 Active route 又持有连接，手机 Direct **15,038 ms 扫描超时**，App 无恢复残留。
> **普通 route idle/cold-resume 和交叉门禁仍未完成**，不宣称 L1 全部或 L2/L3 已关闭；间歇安装失败也未关闭。
> 当前恢复与部署
> 以 [GATT §10.50](DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md) 为准；以下为历史记录。

> **2026-09-06 §10.49 当前入口：**仅按本轮授权 reset 一次 COM12 child access，Hub 未重置。
> 同一 BLE Direct 固件已实际完成手机原生认领/控制 **0→1**，随后已有设备 attach（无再次 reset/enrollment）
> 到新 Active/version4；真实 App account MQTT→Hub Gateway credential→child 控制 **1→2**。
> **交替使用未通过：Hub 空闲时持续占用唯一 ACL，手机 Direct 扫描超时。** 主线优先改授权/链路占用解耦，
> 先 L1 fake-clock 合同测试，再 L2 有界冷恢复/释放、L3 两板交叉、L4 已有 Direct attach 的 App 入口，
> 然后回 N3/N4/N5；不先扩大全平台 WiFi 或长稳。测试计数不等于 UI 接入/发布完成。
> 已删除内部 probe 的失效一次性 GAP listener，诊断按每次 Host init 注册；App 只修首因被清理断连覆盖，
> **47 tests**、ESLint/Web/Android、两种 child 编译、工具 **31/31**/架构通过。首次安装曾失败，正常恢复
> 不代表该失败根因已修。当前 child 运行 test-only witness 固件；Hub/Service/Broker 不变，App 无恢复残留。
> 当前 operation=`8cb2bcdda4da10afaff0ed1e36e8bf2d`，epoch2145337234，不要再 reset；完整证据与执行计划
> 见 [GATT 主线文档 §10.49](DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md)。以下为历史记录。

> **2026-09-06 §10.48 最新：**按用户本轮明确授权仅 reset 一次 COM12 child access，Hub 未重置。
> 同身份重新认领→退役等待→安装→独立 Gateway 认证/proof→新 Active/version4，原 checkpoint 已正常清理。
> 带阶段观测的正常重连 **10/10**、PG 十窗口 Closed、button ACK/State **3→4**；无 session/link/permit 残留。
> 既有工具补认证前公开帧头、区分 Method2/初次同步/主动关闭，自测 **31/31**、架构通过；无生产源码/部署变动。
> §10.47 两次间歇失败本轮未复现，**不宣称修复**。下一批先核对 test-only GAP listener 生命周期并有界诊断，
> 再 N3 MQTT/N4/N5。当前子设备已重新绑定，不再为继续测试重复 reset；完整现场/证据见 GATT **§10.48**。

> **2026-09-06 §10.47 最新：**上批原 pending 已恢复 Active；随后只 reset 一次 COM12 child access，
> 同身份重认领。Service 精确 retirement-pending + App checkpoint/等待入口已落地，删除首次绑定重复 create。
> App **58/58**、Service 真实隔离 PG/HTTP/access、构建通过；仅更新本机 Service/APK，设备固件不变。
> 真机同步 **5.558 s**、正确显示退役等待；等待时 force-stop 后保留同 operation，最终 install/proof/Active。
> 最终独立正常连接/同步/关闭 **3/3**（39.537 s），button ACK/State **2→3**；不抵销恢复过程中的失败。
> **恢复中两次间歇连接失败仍未定位**，不算一次顺畅完成或 N4 全过。当前 child 已绑定、无恢复残留，勿再 reset。
> 下一批先收口普通连接/Method2 失败，再 N3/N4/N5；完整合同/部署/时间线见 GATT **§10.47**。以下为历史记录。

> **2026-09-06 §10.46 最新：**设备管理 mux 错将发出 Ready 视为任务完成，Supersede reset 了仍工作窗口。
> 新内部 busy() 合同区分任务/租约与发送结果；旧 Host RED、新 ASan **45/45**，3.3.11 编译/架构通过。
> Hub 已更新 **+212 B Flash / globals 不变**，App/child/服务器生产实现不变；真机 commit 后同步
> **5.522 s 成功**，不再等待约 50 s 后断连。随后 attach-create 暴露旧拓扑退役尚未结束的独立冲突。
> 原 checkpoint 已继续到 **PendingAccessDelivery**，手机随后离开 ADB；新 Gateway install/proof/Active
> **尚未完成**，不要再 reset child。手机接回后恢复同一 operation，再收口 retirement-pending 精确等待合同。
> 主线恢复标识/证据/后续步骤见 GATT **§10.46**；普通 Open/Close 超时、N3/N4/N5、D4b/长稳仍未整体关闭。

> **2026-09-06 §10.45 最新：**按用户授权重置现有 child，保留 instance、同 owner 重认领复用 logicalDeviceId。
> 真机发现并修复 provisioning endpoint stop 后残留 sessionId，避免已安装凭据又重开 Provisioning 导致
> StateConflict/Fault；合并重复失败出口，无新 RAM/API/SDK 依赖。Host ASan **45/45**、架构/3.3.11 编译通过。
> 最终真实 requestId exact/changed replay **通过**；三个请求仅一次 NVS set+commit，原 pending 正常恢复至
> Active；正常无写入观测固件已恢复、access hash 不变。工具 Windows/Linux **28/28**，旧两次失败不追认。
> **commit 后同步断连仍需收口**，分阶段恢复不是首次顺畅完成；普通连接复测另有一次 Open/Close 超时，
> checkpoint 已经由正式恢复清理，不靠 reset/删除记录。完整证据和后续准入以 GATT **§10.45** 为准。
> 正常固件随后独立认证/同步/关闭 **1/1（12.865 s）**，再经 Hub relay 真实 button 控制并确认计数 **0→1**，
> 结束 App 无残留；不与前次失败拼成全过。
> 下一批先查交接/窗口时序，再继续真实 MQTT envelope 与 N4/N5；不关闭整个 N3、D4b 或发布长稳。

> **2026-09-06 §10.44 最新：**N3 enrollment requestId 重放工具已补，复用 CDP transport、恢复全部临时
> hooks 并保留已安装 pending credential；Windows/Linux 自测 **20/20**、Host ASan **45/45**。
> 无产品源码/SDK/wire/API 变动。当前 child 已 Active，本批不重置、不重新认领；真实普通连接/同步/清理
> **1/1（14.536 s）**，topology/version 与 child access hash 未变。重放真机负测仍待未绑定 child 或先确认
> 正常 owner reset/re-enrollment 影响；执行合同、恢复路径及下一步见 GATT **§10.44**，不能宣称 N3 完成。

> **2026-09-06 §10.43 最新：**N3 已补 Service 正式 sealer 固定向量→ESP32 正式 mbedTLS/processor/NVS
> 诊断门禁。空/已保存状态下共 45 个拒绝场景零 replace/零 ACK/内容不变；合法重封 ACK 不变，实际软件
> 重启后 exact replay 零重写（attempts/writes=0）。Host ASan **45/45**、fixture/3.3.11 编译通过。
> 使用独立测试命名空间，原 Hub access hash 不变；测试 record 已清理，原 Hub 固件已恢复，四端正常认证/
> 同步恢复 **2/2**，App 无残留，topology/child 凭据未变。产品源码、SDK/wire、App、服务器未修改。
> 本项不是四端 MQTT 篡改注入、物理断电或 D4b；下一批 N3 enrollment requestId device replay 与真实
> management envelope 注入，先明确现场绑定的恢复路径。完整证据及边界见 GATT **§10.43**。

> **2026-09-06 §10.42 最新：**Broker 已收口“接受/已送达”边界：Select/Down 离线保留原有界 pending，
> Up ACK 先保留 lastUpAck，由 Hub exact retry 补交付；不再抛仅表示暂时断线的 HubOffline。
> timer 重投前检查窗口，Close/Rejected/expiry 后不发；publish 故障仍失败，无新增队列/协议/App 重试。
> 旧镜像 RED，新 Linux Broker 全套/构建、Service 内存+真实 PG、App **30 tests** 通过；仅 shard-1 更新。
> 同 APK/双板固件不 reset，真实 MQTT socket 中断→同操作同步/清理→fresh 同步 **2/2**，App 无残留、
> topology/NVS 未变。自然凭据换代在途门禁独立通过（management 间隙 429 ms），之后正常 **10/10**
> 全部 Closed/无残留。下一批回 N3 剩余 envelope/enrollment replay，再 N4/N5；详见 GATT **§10.42**。

> **2026-09-06 §10.41 最新：**relay 已区分 MQTT generation 与窗口/授权生命周期，同授权云间隙保留 exact
> pending/sequence，过期、失信、ACL 失败或授权变化结束窗口；Host ASan **45/45**、架构与 3.3.11 编译通过。
> 生产修正净增 **272 B Flash / 8 B globals**；内部 socket 注入 hook 另计，已更新 COM11 Hub，child 不变。
> 真机另定位 App 重试 ManifestAccept 后迟到 ACK 被当作 State 响应，已严格按最近已确认 ACK 内容幂等修正，
> App **30 tests**/ESLint/APK build 通过，新 APK 独立正常 **3/3**，另一次真实迟到 ACK 注入/同步/清理通过。
> socket 故障仍因 Broker 上行 ACK publish
> 抛 HubOffline/503 而结束，清理成功不是续传成功；下一批先收口 Broker 有界 pending/ACK 接受语义，
> 再做在途重连/自然刷新、独立 10 轮与 N3/N4/N5。不宣称四端连续性或长稳完成，完整证据见 GATT **§10.41**。

> **2026-09-06 §10.40 最新（优先执行）：**Broker 已修复 stale Service Ready 与 Hub terminal 抢先到达的
> Close 竞态；严格验证 exact binding 后幂等返回原终态，不覆写 result/attempt、不追加下发。旧实现 RED，
> 新 Linux Broker suite、Service 专项（真实 PostgreSQL）及 App 25 tests 通过。原 APK/双板固件不 reset，
> selected-acl 拒绝清理→fresh 认证恢复 **2/2**，App 无残留，child NVS/Active topology 未变。
> 仅本机 shard-1 升级；设备和 App 无生产改动。连接事件证明自然 MQTT 刷新约 570 ms management 间隙，
> 另发现 relay 云离线清序号但窗口可存活的生命周期不一致。下一批先 Host 故障矩阵和最小修正、四端跨
> credential generation 验证，再回 N3/N4/N5。不可将短循环扩张为跨刷新/发布长稳通过；详见 GATT **§10.40**。

> **2026-09-06 §10.39 最新：**ESP32 原生 BLE peripheral 修复 indication 失败误杀 Host 和连接事件交接丢
> disconnect 两项；旧实现 RED，新 SDK 边界回归及全量 ASan **45/45**、架构/3.3.11 编译通过。只新增一个
> 待关闭 native handle，未改用户 API、协议、Hub Central 或三端鉴权。内部 probe 新增被动 GAP/广播只读观测。
> child 固件已更新、NVS/topology 未变，独立正常循环 **10/10（137,587 ms）** 全部 Closed/无 App 残留。
> 一次 selected-acl 正确 Rejected 但清理请求失败并留 checkpoint，正常 recovery 清理；修改前另有 Hub offline。
> 两项失败不拼成成功、历史空广播不宣称唯一归因。下一步先补清理错误/Hub 可达性证据，再回 N3/N4/N5；
> D4b、双 Hub、30/100/发布长稳仍独立待办。冻结 hash 与完整证据以 GATT 主线 **§10.39** 为准。
> 补充：同组合不 reset 再做 selected-acl 拒绝→fresh 认证恢复 **2/2**，PG Rejected/Closed、App 0 残留；
> 之前的清理请求失败未重现，未把它标记为已修复。最后 child 运行 731,013 ms、Host 同步且广播开启。

> **2026-09-06 §10.38 最新：**Hub 与 Broker 的 exact duplicate ACK 幂等缺陷均先红后绿修复；旧 ACK
> 不清理新 batch，改状态/错上下文/未来序号继续拒绝，无新 wire/API/重试层。Flash **+20 B**、globals 不变。
> Host ASan **44/44**、App **25 tests**、Linux Broker suite 与构建通过；真机两轮各注入 **20 个重复 ACK**
> 成功，随后独立正常循环 **10/10（135,984 ms）**，全部窗口 Closed/无 App 残留，topology/NVS 未变。
> 另有复位前两次空候选 SCAN_TIMEOUT，Hub 重启不恢复、child 不擦存储复位后恢复，尚须定位广播停发；
> 不因此将 §10.37 历史 9/10 改为全过或关闭发布长稳。下一批低侵入排查广播/连接回收，继续未完成 N3/N4/N5。
> 具体冻结 hash、失败/恢复证据和执行边界统一见 GATT 主线 **§10.38**。

> **2026-09-05 §10.37 最新：**已修复 Broker 将即将清零的 Buffer 交给 Aedes 异步发送的所有权错误；
> 设备 `managementError=7` 未再复现，未改设备校验/wire/RAM。新版 App 的真实 selected ACL 拒绝→Rejected
> 清理→新 operation 认证恢复通过，原 topology/NVS 不变。Host ASan **44/44**、App **8 files / 48 tests**、
> Linux Broker Device V2 tests 与三端相关构建通过。后续十轮短循环 **9/10**，末轮 Connected 后同步未完成，
> 出现 `relayError=17`，不能宣称通过。十窗口均清理、App 无残留；下一批先定位 ACK/序号和原始安全错误码，
> 补矩阵/复测，再回 N3/N4/N5。完整证据和修订顺序见 GATT 主线 **§10.37**，优先于以下历史待办。

> **2026-09-05 permit-join 失败边界收口：**已删除 selected ACL 失败后的内部透明重连；Connecting/Ready 丢失或
> NotConnected 写失败经既有 Fault→Rejected 关闭窗口，opaque 分片不能跨 ACL，正常 WouldBlock 仅重试同包。
> App 确认 exact window 的合法终态后清理 permit checkpoint，不掩盖原接入错误；网络失败仍保留 recovery。
> Host Debug ASan **44/44**，App Angular **8 files / 48 tests** 与 ESLint 通过。后续无 trace 复现定位到新旧
> 授权 locator 缓存的插入顺序问题，已改为现有观测时间的新鲜度排序，不用 MAC 或 RSSI 代替身份、无新增 RAM
> 字段。证据与剩余条件见 GATT 主线 §10.36；不将短循环成功扩张为 N3/发布长稳完成。
> 最终无 trace Hub **10/10** 真实认证/同步/关闭，PostgreSQL 十窗口 Closed、原 Active topology/child NVS 不变，
> App links/sessions/checkpoints=0。Flash 相对原基线 +260 B、globals 不变。下一步安装 App 清理改动，验证真实
> ACL 丢失恢复并核对循环中的 `managementError=7`，再回 N3 envelope/device replay；不扩大重试或 SDK 绑定。

> **2026-09-05 N3 Gateway receipt/ControllerMutation 真机子门禁：**同 §10.33 APK/双 XIAO 固件，错误回执被
> App 和 Service(409) 独立拒绝；子设备 exact BBP/2 mutation 返回同 receipt，改 secret 返回 AuthenticationRequired。
> 同 operation 保持 PendingChildInstall/version 2，正常 recovery 后 Active/version 4，无 access reset、残留连接或
> checkpoint。NVS 摘要保持一致，新增满三槽 replay 零 replace Host 断言，相关 Debug ASan **5/5**。详见 GATT
> 主线 §10.34。前置一次 CONNECT_TIMEOUT 保留待查，封包 tag/expiry 与 enrollment device replay 真机仍未完成。
> §10.35 已复核架构：保留 Service 权威/独立 Broker ledger、App 编排、Hub native Link/Selector 与平台装配边界，
> 不整体重写、不把 GATT profile 强套 Zigbee/Mesh；N2/N4/N5、D4b、资源趋势和发布长稳仍未关闭。

> **2026-09-05 首次 enrollment→attach 生命周期收口：**App 删除重复 `connectAndAttach`，首次与恢复统一为
> 关闭 enrollment → checkpoint/cloud-first → exact Hub fresh session 安装 → 独立 Gateway Method 2 → cloud proof。
> 移除第二次 generic resume 可能切入手机 Direct 的分支；pending cloud/proof 不打开 GATT。旧代码两项失败已
> 回归锁定，新代码 **7 files / 62 tests**、ESLint、APK 构建通过；真实 Android/双 XIAO/Service/Broker 首轮
> **89.43 s** 达到 Active/version 4，无 failed/retry，结束无连接/恢复检查点/活动 permit，临时 hooks 已恢复。
> 设备固件与三端 wire 未改。证据见 GATT 主线 §10.33；下方“下一批先收口生命周期”是已完成的上一批待办。
> 下一步回到 N3 Gateway receipt/envelope/device replay，N2/N4/N5、D4b 与发布长稳仍未完成。

> **2026-09-05 N3 真实加密子门禁：**同一新版 Android APK + 官方 3.3.11 双 XIAO 完成 grant signature、
> Noise request tag、Admin receipt proof 三项单 bit 注入；前两项不安装，后者设备已安装但 App 拒绝提前提交。
> 修复 App 连接建立后忽略 Hub Disconnected 更新、白等接收超时的问题；4 files / 29 tests、ESLint、APK 构建与
> 真机通过。pending credential 经正常 Method 2、原 commit 和 attach checkpoint 重试后达到 Active，exact commit
> 零更新、变化 replay 冲突也已验证。详细证据见 GATT 主线 §10.32。首次 enrollment→attach 曾有一次
> SESSION_CLOSED，重试恢复通过，不冒充首次成功；**下一批先收口这两个阶段的生命周期与重复编排**，再完成
> Gateway receipt/envelope 等剩余 N3。未关闭 G2B-4、未改生产设备协议、未扩展 Mesh/History。

> **2026-09-05 G2B-4 N3 enrollment/attach 软件门禁：**已纠正“所有 expiry 都是终态”的验收描述，明确新
> 授权过期与已安装 credential 的恢复提交不同，并保留 Gateway 同材料合法重封合同。App attach receipt 补齐
> secretDigest 与实际 Gateway secret 的绑定，不匹配时在 Method 2/commit 前拒绝。Host ASan 2/2；Service
> enrollment/access 两专项含真实 PostgreSQL 通过，覆盖拒绝零修改、过期后恢复、exact commit 零更新、变化重放
> 冲突。App 专项 **3 files / 20 tests** 与 targeted ESLint 通过。详见 GATT 主线 §10.31；本批设备生产代码/固件
> 未改变，手机 APK 未更新，仍需冻结组合的 N3 四端故障注入，未关闭 G2B-4。

> **2026-09-05 G2B-4 N3 permit-join 选择负测完成 4 轮：**已纠正临时诊断后默认 CLI 误用 Core 3.3.6
> 的测试基线，保留 NVS 恢复官方 3.3.11 Hub 固件（1,432,572 B Flash / 65,332 B globals）。真实 Android
> owner API→Service/Broker→Hub 扫描 1 个 child：单 bit token 返回 NotFound，exact Select 返回原结果，同操作
> 换 token 返回 409，4 轮均 Closed；新增 enrollment intent 与 topology update 均为 0。修正 Broker 在已收到
> SelectResult 后允许替换候选的分支，并补 Connected/NotFound 零重发回归。Hub ipc0 最低1112 B，临时 Port
> trace 已清理；新增可复现硬件脚本。详见 GATT 主线 §10.30。未把此次结果扩张为完整 N3：下一步仍是
> enrollment/attach 的过期、篡改、重放，再进入 N4 force-stop/N5 其他 pending restart；N2 双 Hub 与 D4b 未关闭。

> **2026-09-05 G2B-4 N1 双账号真机门禁完成，并修正 BLE 恢复改名副作用：**XIAO ESP32-S3/COM12 保留
> DeviceInstanceId 执行 `resetAccess()` 后，A 账号重新 enrollment 仍映射原 logicalDeviceId，并建立含 3 endpoint 的
> ready Direct session。保持 GATT ready 时 logout，App 的 tracked session/link 从 `1/1` 收敛到 `0/0`，旧 session
> 明确进入 closed；B 仅看到自己的 1 台设备，对 A child 为 `active=false, pending=[]`；切回 A 后 v5 credential 与
> ready Direct 均恢复。该门禁同时暴露出旧实现没有在 access-root 更换时使既有 Active topology 失效；此问题已由
> 下述 supersession 状态机修正。现场还发现 recovery 会把原设备名
> 覆盖成默认 “New device”，Service commit 已改为首次 claim 写名、同 owner recovery 保留原名；内存仓库与真实
> PostgreSQL BLE enrollment 测试通过。App 全量为 92 files / 713 tests。N2 错误 Hub、N3 篡改/超时/重放、N4
> force-stop、N5 restart 仍阻断 G2B-4 发布。

> **2026-09-05 child access-root supersession 软件、持久层与四端注入闭环完成：**`resetAccess()` 后，同一
> DeviceInstanceId 的新 BLE enrollment commit 会在同一 PostgreSQL 事务中轮换 accessEpoch/DirectAccess 并把旧 live
> topology 标为 Revoking。worker 先撤销 route；Broker 再原子过期该 topology 的待投递 access/proof/renewal/revocation，
> 向 exact Hub 发送 terminal `Supersede`；Hub 无需连接已经清空旧 Admin slot 的 child，只按旧不可变绑定精确清除
> Gateway credential 并返回 `Retired`，Service 最终 CAS 为 Detached。正常路径 exact-match storage revision；仅未收到
> 原 Hub ACK 时允许 revision 0 wildcard，且仍必须匹配 operation/child/epoch/controller/version。跨语言协议 18/18、
> C++ ASan、Broker 全套、Gateway 内存/真实 PostgreSQL、migration 26 与两个现存 SQLite shard 升级均已通过；真实
> ESP32 Arduino Core 3.3.11 Edge Hub clean compile 为 `1,424,968 B Flash / 65,308 B globals`。真实 COM12 child
> 保留 DeviceInstanceId reset 后由 App 重新 enrollment，旧 topology 原子进入 Revoking/version 175。现场发现 Gateway
> Broker client 仍把 phase 写死为 `1..3` 且响应状态遗漏 `retired`；现已改为共享协议枚举校验并补 stage/get HTTP 回归。
> worker 随后在无数据库人工修改下自动收敛：Broker shard 1 唯一 phase 4 ledger 为 `retired/attempt 1`，Service 为
> Detached/finalized，所有相关 ledger 无 pending。Hub 硬复位后 `childState=0/revokeActive=0`、旧 access 不复活；App
> 的新 Direct session 仍可打开同一 logical device 并同步 3 endpoint。该结果关闭 N5 的 supersession 子门禁，不替代
> N2～N4 与 N5 其他 pending boundary/组合重启负测。superseded topology 同时禁止进入旧 nearby Admin revoke recovery。

> **2026-09-05 G2B-4 N2 错误 Hub 软件预门禁完成：**App relay 在打开 child control 前强制绑定
> `operationId/Hub ID/child ID/DeviceInstanceId`，Cloud topology 任一字段不一致即清零 topology、relay 与 recovery
> 缓冲并 fail-closed；新单测确认 Hub-A 响应注入 Hub-B request 时不会打开 child GATT。当前现场与 PostgreSQL 只有
> 一台 Active Hub，仍需第二台同 owner Hub 才能关闭 N2 真机门禁，禁止用数据库伪造 Hub 冒充完成。

> **2026-09-05 G2B-4 App 账户/环境软件边界完成：**复核发现 App 的 Direct ControllerCredential 与三类
> Edge Gateway checkpoint 原只按 device/operation 存储，已改为 `(API authority, accountId)` namespace；运行中的
> Direct/open/presence/enrollment/attach/permit 事务同时绑定 `sessionEpoch`。切换账号会立即关闭 Direct session、
> 清空 snapshot 并使旧异步响应 fail-closed；grant、PresenceKey、Gateway secret 和 relay buffer 在抛错前清零。
> 无账号字段的旧测试记录禁止自动迁移，需重新 enrollment/attach。App local build、全量 92 files / 713 tests 与
> 本批 ESLint 均通过；Service permit-join/HTTP/access 软件门禁通过，N1 后新增的 recovery 命名修正也已重复真实
> PostgreSQL 专项。该结果不替代错误 Hub、force-stop、expiry/tamper/replay/restart 四端门禁，G2B-4 仍阻断发布。

> **2026-09-05 Edge Gateway renewal route-pause 竞态已修复，100 轮 transaction gate 通过：**Hub 在 Service
> topology commit 前 durable promote child credential 是协议要求，不是攻击。旧 Broker 却先用缓存的旧 route 比较新
> envelope，导致合法 `N→N+1` 帧被当作 severe mismatch，并连带封禁 management session。设备端现以
> `GatewayAccessRecord.deliveryRevision` 栅栏旧数据面 admission，同时保留 DirectSecure management session；Broker 先查询
> Service authoritative active route，权威 route 已暂停/前移时仅退役旧 virtual child projection，稳定权威 route 上的伪造
> envelope 仍 fail-close。Host `44/44`、Broker build/回归通过；原 `26→27 pending_proof` 无人工改库即恢复，随后
> `27→29` smoke、`29→39` 正式十轮和 `39→41` 两轮资源观察均 completed。正式十轮总计 `548.8 s`，PostgreSQL revision
> 与 Broker delivery/三阶段 ledger 全部单调且唯一。最终稳定 free/min/largest heap 为
> `105,432/53,252/45,044 B`，loop/ipc0/NimBLE Host 最小栈为 `1,352/1,112/2,708 B`。随后正式 30 轮
> `41→71` 全部 completed，PostgreSQL 0 非 `+1`/0 链断点，Broker 30 条 delivery 与 90 条 phase 均唯一终态，最终
> topology/storage revision 为 `74/71`。minimum heap 仅第 3 轮下探到 `52,056 B`，后 27 轮不变；最终 stable
> free/largest 为 `105,436/55,284 B`，三任务栈全程不变。随后同一固件/部署完成 `71→171` 的 100 轮门禁，
> 总计 `5,549.3 s`；PostgreSQL 为 100 条 completed、0 非 `+1`、0 chain gap，topology/storage revision 最终
> `174/171`；Broker 为 100 条 delivery、300 条唯一阶段终态、0 异常 operation，日志无 mismatch/ban/error。
> 资源最终为 free/min/largest heap `105,444/45,448/55,284 B`；minimum 第 38 轮后 62 轮稳定，loop stack
> `1,336 B` 第 16 轮后 84 轮稳定，ipc0/NimBLE Host 始终 `1,112/2,708 B`。R4 transaction gate 已关闭；
> owner/App 产品入口仍未开放，D4b、G2B-4 真机负测与发布长稳继续独立阻断。

> **2026-09-04 Edge Gateway active credential renewal R4d 双板/故障边界闭环：**R4c 的 internal-only renewal
> repository/service/单 worker，按 delivery→Prepare→Apply→new-version GatewayProof→Finalize→route restore 逐状态推进。
> candidate 仅以 Hub sealed envelope 与 KMS ciphertext 持久化；Service 区分
> `SHA-256(secret || PresenceKey)` material digest 和 `SHA-256(secret)` Controller digest，并在 KMS 边界验证 Rotate HMAC
> receipt。GatewayProof 后由 PostgreSQL `SERIALIZABLE` transaction 原子 CAS topology+operation，同时写入新 credential、
> storage revision 和 renewal KMS locator/binding；Finalize ACK 后幂等恢复 Broker route。Broker 缺失/无结果过期投影可以
> 新 lease 重建，Apply forward-recovery 先落 Service 权威状态再重投。真实 PostgreSQL、Linux Broker restart/exact replay、
> Gateway Device V2 全量回归均通过。测试触发仅提供 admin-token internal create/read/cancel，并对 envelope/KMS/Grant/
> receipt/secret 全部 redaction。新增 28 点 Service/Broker 调用前与 durable-after 故障矩阵；Hub 从
> `MutationAttempted/ReceiptStored/active-promoted/journal-promoted` 重建；同一会话支持 exact Grant/receipt replay，跨复位则以
> fresh Method 2 认证出的版本判定：`N+1` 确认 child 已提交并确定性重建 canonical receipt，`N` 持久化 `Abandoned`、稳定
> `Rejected`，由 Service Cancel 清理。Linux Broker restart 与真实 PostgreSQL exact replay 均已通过。真实双板已完成 credential
> `1→2` 正常链，以及 child 已为 `3`、Hub 仍为 `2/MutationAttempted` 时的物理复位恢复；最终 topology `5→6`、active
> revision `2→3`，再硬复位后 MQTTS、management 与 child route 均恢复。ESP32 3.3.11 编译上传为
> `1,430,872 B Flash / 65,316 B globals`，新增恢复路径无 global RAM 增量。随后 10 次连续真实 renewal 已从
> credential `3→13` 全部 completed，PostgreSQL/Broker ledger 数量与 revision 单调性一致。换入只增加输出的 internal 资源
> 探针后稳定基线为 free/min/largest heap `105,416/98,240/63,476 B`，loop/ipc0/NimBLE Host 最小栈
> `3,684/1,112/2,808 B`；因循环前没有同版插桩值，这只是 30 次阶段起点。下一步为 30→100 次资源/重复性门禁；此前
> 仍不开放 App/owner API，也不宣称 production ready。

> **2026-09-03 ESP32 BLE SDK 准入结论已由同板三路径 A/B 纠正：**同一块 XIAO ESP32-S3（MAC
> `98:3d:ae:ee:7c:6c`）上，Arduino-ESP32 3.3.11 / ESP-IDF `v5.5.5` 的外部 NimBLE-Arduino 2.5.0 与 Core
> 自带 `BLEDevice`（S3 上同为 NimBLE backend）均完成首次初始化、GATT service 和 advertising；二者
> `ipc0` 都从约 1280 B 降至 1104 B。加入 Arduino Core 官方 `esp32-hal-alloc-ble-mem.h` 保留标记后，直接
> ESP-IDF C API 路径也以 `nimble_port_init=0` 成功，`ipc0=1108 B`。
>
> 此前 3.3.10/3.3.11 的 TLSF/watchdog 证据来自**没有声明 BLE 正在使用**的直接 C API 探针。Core 3.3.10/3.3.11
> 会在 `initArduino()` 中提前释放未声明使用的 BLE 内存；随后在已释放区域调用 `nimble_port_init()` 必然可能
> 落入 controller heap/watchdog。它不是 3.3.11 controller 首启回归，也不能据此否决 SDK。3.3.10 的旧崩溃
> 结论同样作废，后续如需评价该版本必须带相同内存保留合同重测；但其 IPC stack 仍只有 1024 B，不能解决
> 3.3.6 已知的窄水位问题。
>
> 3.3.6 / ESP-IDF `v5.5.2-249-gf56bea3d1f` 是当时唯一完成 Blinker 功能联调的临时版本：最小启动和 WiFi
> STA 均为 `ipc0=260 B`，最小 NimBLE 路径为 84 B，完整产品 Active 为 76 B；它能首启但只具 1024 B IPC
> stack，故保持 `test-only`，不是生产基线。上游另有 3.3.7/XIAO 的同类 BLE/TLSF 报告；3.3.8、3.3.9 本轮
> 未逐版真机测试。3.3.11 后续已让 `Esp32NimBleLink` 遵守 Arduino BLE 内存保留合同，并通过完整 Edge Hub
> product-level 资源门禁及 10→30→100 Action 循环；该历史待办已关闭，不能继续用 3.3.6 的窄水位否决 3.3.11。
>
> **2026-09-03 上游归因与维护边界已修正：**当前 Blinker ESP32 Port 直接使用 ESP-IDF NimBLE C API，但必须
> 同时遵守 Arduino-ESP32 的启动期 BLE 内存保留合同。3.3.11 已用三种 API 路径证明 Host/controller 可正常
> 首启，故不再建立“等待未知 controller 修复”的错误前提。Controller、Host、Port 和 Arduino HAL 仍保持
> 分层；以后所有裸 ESP-IDF BLE 探针也必须先声明 BLE 使用，禁止再把已释放内存造成的崩溃归因到 controller。
>
> 不建立 Blinker NimBLE Host fork，也不把自维护 Arduino Core 变成普通用户依赖。当前优先复用官方 3.3.11
> 完整 Core；外部 NimBLE-Arduino 只保留为 A/B 参考，不进入 Blinker 发布依赖。是否从直接 C API Port 迁移到
> Core 自带 `BLEDevice`，由完整 peripheral/central、WiFiProv 交接和资源门禁决定，不由本次误诊驱动。
>
> 用户侧仍采用“一个库、统一 API、按能力准入”：WiFi-only 不受 BLE 门禁影响；ESP32 BLE/WiFi+BLE/Edge Hub
> 现在以官方 3.3.11 为候选完成产品级复测，而不是要求用户安装外部 NimBLE 或自维护 Core。
> 未来首个通过门禁的官方 Core 以 `minimum supported core + CI tested range` 接入，不按 exact patch version
> 锁死。自维护 Core 只可作为自有硬件/BSP 的兜底发布物，不能成为 Arduino Library Manager 默认路径。
>
> **2026-09-02 ESP32 WiFi+BLE G1B network-only reset、模式交接与系统任务快照完成：**XIAO ESP32-S3、
> Android 和 Arduino-ESP32 3.3.6 已连续两轮完成 Direct 活跃状态下 `Blinker.resetNetwork()`、无需断电切入
> BLE WiFiProv、Security1/GetInfo exact DeviceInstanceId 核验、保留 DeviceKey/access 的 WiFi 重配、Cloud
> 登录及 NimBLE Direct 恢复。App 已修正配网页销毁竞态：成功后先幂等释放 WiFiProv transport，再刷新
> inventory 并进入设备页；真机无需刷新页面即显示“蓝牙已连接”，Direct Command 将 State 1→2。
> 设备同时删除了重复 manager deinit：Arduino-ESP32 的 `NETWORK_PROV_END` handler 拥有 deinit，库等待
> `NETWORK_PROV_DEINIT` 后才启动 Direct。此前的 `Provisioning manager not initialized` 已消失；最终 GATT
> 断连仍可能由 Espressif protocomm 打印 `rc=30/BLE_HS_EDISABLED`，但产品诊断为零 operation/transport failure
> 和零 drop，不以 sleep 或第二套状态机掩盖该上游清理期日志。
>
> 未插桩 composition 基线仍为 1,279,291 B Flash / 53,988 B globals，steady/min free heap
> 159,560/135,076 B、largest block 116,724 B、loop-task 4,296 B。新增内部门禁探针枚举 13 个任务；完整
> Provisioning→Cloud→Direct 与 Cloud/Direct 下发后，关键最小剩余栈为 `ipc0=76 B`、`ipc1=232 B`、
> `nimble_host=1,652 B`、`wifi=3,996 B`、`btController=2,668 B`、`sys_evt=2,280 B`、loop task
> `4,344 B`，且 `ipc0` 在活动前后未继续下降。ESP-IDF 的 high-water mark 单位为字节；76 B 因此只能记为
> 3.3.6 SDK 内部窄余量，不得写成充足 headroom，也不能由 Arduino 库擅自修改 SDK task stack。该历史风险后来由
> 3.3.11 的 2048 B IPC stack 与 `ipc0=1,108 B` product-level 复测关闭；10→30→100 Action 也已完成，SoftAP/RF 共存与
> 发布长稳仍按各自门禁保留。

> **2026-09-01 Edge Gateway G2B App 产品入口与恢复原子性软件完成：**owner Edge Hub 设置页现提供“添加蓝牙
> 子设备”，固定 exact Hub 并要求用户明确选择临时候选码/RSSI；App 不显示或以 MAC 建立身份。新增无秘密
> Hub/child workflow checkpoint，并在 pending credential 落盘后、grant 发出前写入，覆盖 Pending resume、ownership
> 已提交但 attach 尚未开始、以及现有 operation attach 三阶段。终态先删 workflow、再删 attach checkpoint，任意
> App-kill 窗口都至少保留一条可验证恢复路径；无 exact Hub 标记的孤立 Pending 不会再被盲目归到当前 Hub。
> Angular 类型/Lint 与针对性 `15/15` 已通过。真实双 XIAO 四端、双账号/错误 Hub、篡改/超时与 reboot 门禁仍未
> 完成，不能据此开放 Gateway-assisted enrollment。

> **2026-09-01 Edge Gateway pre-tombstone recovery 软件闭环完成：**针对 child 已 durable 删除 Gateway credential、
> Hub 尚未 commit receipt tombstone 即掉电的唯一剩余窗口，App 现复用 owner/Admin DirectSecure 获取 fresh nonce 并
> relay 幂等 Revoke；Service 以 CAS 增加 topologyVersion、签发/验证 exact Grant/receipt；Broker internal-only recovery
> 将 Apply ledger 收敛为 Revoked 并停止旧版本 pending 重投；Hub 的更高版本 Finalize 可抢占旧 Apply，且只有本地 active
> access 的 operation/child/epoch/controller/credentialVersion/deliveryRevision 全匹配才清除。Host ASan `41/41`、
> 200,000 fuzz、Protocol `15/15`、Broker、Gateway real PostgreSQL + production KEK `25/25`、App `52 files/260 tests`
> 均通过。G2A-3b 目前仅剩双板在“child delete 后、Hub tombstone 前”的精确掉电/复位注入；D4b encrypted-NVS 与长稳
> 仍独立阻断 production 发布。

> **2026-09-01 Edge Gateway G2A-3b 四端 no-poll 完成：**全新 topology
> `rAnm8BWpH2DJOKrzgCR_EQ` 在历史 Gateway credential 已撤销后，以单调递增 version 2 重新 attach/Active。
> 真机发现并修正 Service 将 `version > 1` 错判为 Rotate、App Method 2 AuthInit 硬编码 version 1 两个跨端缺陷；
> 新 topology 必须对不存在的槽执行 `Install(expectedVersion=0)`，App 必须发送实际 credentialVersion。App 只发起
> 一次 detach 后立即强制停止，Service worker 独立完成 Prepare/Apply/Finalize；Broker ledger 为
> `nonce_ready/revoked/finalized`，PostgreSQL 为 Detached/finalized。Hub 复位后持续
> `childState=1/secure=0/revokeActive=0`，旧 access 未复活。该主路径之后的 pre-tombstone recovery 软件实现见上；
> 本段不再作为“尚未实现 recovery”的当前状态。

> **2026-08-31 Edge Gateway G2A-2g 软件与 Hub/child/cloud 真机完成（移动 App 仍待补）：**Service Active
> route lookup/admission、Hub 两槽 MQTTS mux、固定内存 route bridge、Broker transport-neutral virtual child、
> 强制 Manifest refresh、Presence、Active ACL revalidation 与既有 durable command ledger 已接通。双 XIAO 现场在
> 本次 Hub 启动后刷新 child `power/buttonPresses/button` Manifest 和 State revision 1；server `power=true`
> 得到 sequence 5 ACK，ledger 为 `acknowledged/attempt_count=2/http=200`，随后 Patch 将 State 推进到 revision 2。
> 首次命令暴露并修复了 child 主动 Patch correlation 被 Broker 误判为 command mismatch 的问题；bridge/Broker
> 现在只以同 correlation 的 ACK/Error 完成命令，异步 Patch 独立处理。Host ASan `40/40`、200,000 fuzz、Broker/
> Gateway 测试、两个 Broker 分片与 Gateway 镜像构建通过；ESP32 Hub 为 `1,380,479 B Flash / 65,096 B globals`。
> 本机使用 development-only HTTPS 7443/MQTTS 8885 终止器，未放宽 production TLS；Plain-NVS probe 也不替代
> D4b encrypted-NVS。Android 当前未接入 ADB，App account/MQTT/session 17 项测试虽通过，真实设备页 Command
> 仍是关闭“四端 E2E”的最后证据，不能用内部 server command 冒充。

> **2026-08-31 Edge Gateway G2A-1b 双 XIAO 控制面 E2E 完成（业务四端 E2E 未完成）：**真实 App、
> Service/Broker、XIAO ESP32-S3 Edge Hub
> 与另一块 XIAO ESP32-S3 BLE child 已完成 `create -> sealed delivery -> durable ACK -> Admin grant relay -> child
> install receipt -> 独立 Gateway Method 2 -> Hub proof -> Active`。最终 PostgreSQL 为 `state=Active/version=4/
> storageRevision=1/childInstalled=true/gatewayProven=true`；Broker delivery 为 `acknowledged/revision=1`，proof 为
> `secure/presenceKeyVersion=2`。App 非 secret checkpoint 已删除，旧超时 operation 也通过 exact replay 正确收敛为
> `Expired`。本轮同时修正 Edge Hub MQTT packet capacity（仅 Hub 为 724 B）、Advanced Client runtime frame 配置、
> PostgreSQL migration 021、Broker SQLite 无损 schema normalization 及过期 exact replay。该闭环使用 internal-only
> disposable Plain-NVS 测试适配器验证协议与真机 Central；生产 alias 仍强制 encrypted NVS，D4b 专用加密板门禁
> 继续阻断公开发布。上述闭环只证明 topology/credential 控制面，不证明 child 的 Manifest/State/Command/ACK
> 数据面。Broker 中可能存在手机直连留下的 child Manifest/State 历史缓存，不能把缓存命中当作 Gateway route
> 已完成。下一主线为 G2A-2g 最小 Active-topology route bridge，不扩散到多 child/Mesh。

> **2026-08-31 Edge Gateway G2A-1a 完成（G2A-1b 的历史前置）：**Gateway production runtime 已接入 owner
> create/read/resume/receipt/cancel API。create 只接受 Hub/child logical id 与 child physical instance；Service 从
> owner DirectAccess/Presence 权威记录取得 accessEpoch，稳定派生 Gateway controllerId 并分配 credential version，
> 不再接受 App 自报这些字段。只有 Hub durable ACK 后的 resume + fresh controlNonce 才从 KMS 临时解封 raw
> Gateway secret，并以官方 P-256/Ed25519 trust ring 签现有 Controller Control v2 Grant。receipt 经过 exact CBOR
> 字段、secret digest、HMAC 与 `method2Confirmed` 四层校验后才进入 PendingGatewayProof；Hub Secure proof 仍是
> Active 必需条件。所有已 stage 的 cancel 保守进入 RollbackRequired。migration 020 修正了 delivery-only expiry
> 错误消耗 child credential version 的问题。protocol `14/14`、完整 Gateway Device V2 suite、production wiring、
> cross-language golden 和真实 PostgreSQL 017→020 全通过。后续 G2A-1b 双板 E2E 已按上方记录完成。

> **2026-08-31 Edge Gateway G2A-2f 核心软件完成（其 E2E 已由 G2A-1b 关闭）：**冻结 canonical
> `GatewayProofCommandV1`（13 项、最大 252 B）和 `GatewayProofResultV1`（7 项、最大 48 B），复用唯一 MQTTS
> management up/down topic。设备新增固定内存 `GatewayProofCoordinator`，逐字段绑定 protected access record，
> 只在 exact child 完成 Method 2 + DirectSecure 后回 Secure；暂态 BLE 失败有界重试，record/expiry/cloud 变化
> fail-closed；终态 result 保留 exact replay，但新 topologyVersion/nonce 可无云重连地接管，避免 Expired retry
> 被旧 RAM task 永久阻断。Broker 新增 durable exact-command proof ledger；Service repository 新增 child receipt、proof nonce、
> activate/reject/expire CAS。同步修正短期 delivery TTL 与长期 Gateway credential expiry 被混用的问题。
> G2A-1a 已补齐 owner attach/receipt production HTTP/runtime wiring；真实 ESP32 proof 后续已由 G2A-1b 完成。
> 在该闭环完成前没有进入 route bridge。最终回归为 Host `39/39`、独立 200,000 次 fuzz、
> architecture、protocol `14/14`、Broker、Gateway Service（含 PostgreSQL 017→020 migration/CAS）与跨语言 golden
> 全通过；XIAO ESP32-S3 / ESP32 3.3.1 compile 为 `1,370,975 B Flash / 63,168 B globals`。proof deadline
> 已在 Service 规范化到整 Unix 秒，并覆盖 Broker expiry worker 与 Hub `Expired` result 的竞态。

> **2026-08-31 Edge Gateway G2A-2e 软件完成、真机待完成：**新增 ESP-IDF NimBLE fixed-queue
> `Esp32NimBleCentralPort`、exact active+pending `GatewayPresenceMatcher` 与单 child
> `GatewayChildSession` composition。Hub 只从 protected `GatewayAccessStore` 装载 PresenceKey/Method 2
> credential，不以 MAC/广播名定位；完成 Hello → Method 2 → DirectSecure 后才允许业务 record，并在扫描、认证、
> Secure、发送和 pending activate 全程执行可信时间/expiry gate。Peripheral/Central 共用唯一 NimBLE owner；
> Edge Hub 只有在 WiFi/SNTP/HTTPS/MQTTS/management 全部 Online 后启动 Central，cloud admission 丢失即退役
> child 并关闭 Central，为 TLS 恢复释放 runtime heap。Host MSVC ASan `38/38`、200,000 fuzz、architecture/diff
> gate 通过；XIAO ESP32-S3 Core 3.3.1 clean compile 为 `1,365,595 B Flash / 62,848 B globals`，相对
> G2A-2d 为 `+22,216 B / +3,976 B`。尚未完成真实 GATT/CCCD、动态 heap/stack/长稳、G2A-2f 真机 E2E
> 与 G2A-2g Active-topology Manifest/State/Command/ACK bridge；没有公开 Gateway facade。

> **2026-08-30 Edge Gateway G2A-2d 软件完成：**测试阶段的 `GatewaySecretEnvelopeV1`、旧 store/processor
> 路径已移除，三端只保留 `GatewayAccessEnvelopeV2`。Service 将 KMS 托管的 exact confirmed PresenceKey 与
> 32 B Gateway Method 2 secret 在内存中组成固定 48 B access material，并只向 exact Hub 密封；App 不提交或
> 持久化 raw PresenceKey。设备端 352 B protected atomic `GatewayAccessStore` 保存 active+pending locator，
> activate/discard 只推进本地 storage revision，不改变最后一次 delivery revision，因此 MQTT exact redelivery
> 始终返回原 durable ACK。C++/Node envelope、AAD、KDF、ACK golden 一致；Host MSVC ASan `36/36` + fuzz smoke、
> protocol `13/13`、Gateway access Service、Broker Device V2 与 TypeScript typecheck 已通过。下一批才实现
> Service child-install/proof/activate 编排和真实 ESP32 Central port；当前不宣称网关数据面完成。XIAO ESP32-S3
> internal Edge Hub clean compile 为 `1,343,379 B Flash / 58,872 B globals`，相对 G2A-2c 为
> `+860 B / +24 B`。

> **2026-08-30 Edge Gateway G2A-2c 软件完成：**新增独立 `IBleCentralPort`、共享
> `Bbp2BleRecordFormat` 与固定内存单-child `GattDirectChildLink`；Central 平台合同不暴露 MAC/SDK handle，
> GATT link 复用唯一 4 B fragment/reassembly 实现，并覆盖 connect deadline、ATT20 三片重组、fragment stall、
> 底层失败、漏 disconnect callback、stale generation 与 32 次恢复。Host MSVC ASan `36/36`、200,000 次 fuzz、
> architecture/diff gate 通过；XIAO ESP32-S3 Edge Hub clean compile 为 `1,342,519 B Flash / 58,848 B globals`，
> 相对 G2A-2b 为 `+816 B / +0 B`。尚不包含 ESP32 Central port、真实 GATT、协调器 composition 或 route bridge。
> 同批发现并冻结前置修正：V1 只下发 Gateway Method 2 secret，缺少 PresenceKey；真实 E2E 前必须以
> `GatewayAccessEnvelopeV2` 原子密封 48 B access material，并实现 active+pending locator 轮换，禁止以 MAC/名称
> 或盲试所有附近设备绕过。

> **2026-08-30 Edge Gateway G2A-2b 软件完成：**新增 transport-independent、固定内存的
> `ChildSessionCoordinator`，使用生产 BBP/2 codec 完成 exact Hello、Method 2 Init/Challenge/Proof/Authorized
> 和 DirectSecure 激活。默认强制 Manifest、EndpointIds、Authentication、ReliableDelivery、StateRevision，
> Telemetry 可选；协商 max-frame/reliable-window 和 Manifest identity 会被保存。一次总计 15 s deadline、
> 超长帧、错序/flags、缺少能力或 Manifest 及所有认证失败均 fail-closed，32 次 timeout/recovery 不保留旧
> secure state。该层不包含 BLE SDK、扫描、路由或多 child scheduler；下一步为独立 `IBleCentralPort` 与
> `GattDirectChildLink`，ESP32 Edge Hub 在接入前仍不声称具备 child data plane。Host MSVC ASan `35/35`、
> 200,000 次 fuzz 和 architecture/diff gate 通过；XIAO ESP32-S3 clean compile 为
> `1,341,703 B Flash / 58,848 B globals`，相对 G2A-2a 为 `+372 B / +0 B`，尚不包含 Central/GATT workspace。
>
> **2026-08-30 Edge Gateway G2A-2a 软件完成：**新增固定内存、单会话
> `ControllerHmacSha256Initiator`，复用现有 Method 2 proof 与 `DirectSecureSession`，与设备 responder 完成 exact
> Init/Challenge/Proof/Authorized 互操作；错误 proof、权限不一致、错序、全零 nonce 和拒绝结果均 fail-closed。
> durable `GatewaySecretRecord` 只有在 trusted Unix time 未过期时才能投影为 PlatformAccess controller credential。
> Host MSVC ASan `34/34`、200,000 次 fuzz 与 architecture gate 通过；ESP32-S3 内部 Edge Hub clean compile 为
> `1,341,331 B Flash / 58,848 B globals`。D4b 因暂无专用可报废加密板调整为发布阻断门禁，不再阻塞
> transport-neutral 软件工作；Hello、独立 BLE Central port、GATT record link 和 child route bridge 仍未实现。
>
> **2026-08-30 Edge Gateway G2-0D D0～D3：**设备端新增独立 `GatewaySecretEnvelopeSecurity`、308 B
> 单-child `GatewaySecretStore`、`GatewaySecretDeliveryProcessor` 与共享现有 `MqttFrameTransport` 的
> `GatewayManagementClient`。Service/device 固定 wrap KDF 向量一致；只有 current DeviceKey version、可信时间、
> AEAD tag 和 protected atomic read-back 全通过才生成 58 B durable ACK。exact replay 零 flash 写，DeviceKey
> rewrap 保持 storageRevision，重启后返回 same ACK；明文 backing、CRC/identity/expiry/tag/sequence/atomic failure
> 均 fail-closed。官方 PubSubClient 保留：下行 envelope 仍为 QoS1，ACK 改为允许 QoS0/1（官方发 QoS0），丢失
> 由 Broker exact-envelope ledger 重投恢复；4 B connection generation 确保 MQTT 重连后 management topic 必定
> 重新订阅。设备 ASan 32/32 + 20,000 fuzz 与 Broker Device V2/容量恢复/ACL/
> 限流门禁通过。最终单库 Arduino clean compile 继续通过 ESP32-S3 WiFi+BLE（Flash 1,284,779 B / 38%，
> static RAM 60,028 B / 18%）、UNO R4 WiFi（136,792 B / 52%，12,968 B / 39%）、UNO R4 BLE
> （192,684 B / 73%，16,960 B / 51%）和 Wio Terminal WiFi（139,944 B / 27%；core 不输出 SRAM 汇总）。
> D4 尚需 ESP32 MQTTS-only product、可信 Unix time、受保护 storage backing 及真机断电/redelivery；BLE Central
> child data plane 仍未提前开始。
>
> **2026-08-30 Edge Hub D4 onboarding 基础完成：**内部 ESP32 Edge Hub 现在复用普通 WiFi 产品的
> DeviceKey cloud lifecycle，提供手动 `DeviceKey + SSID + password` 与默认 BLE WiFiProv 两个入口；二者
> 收敛到同一加密 NVS WiFi journal 和独立 44 B DeviceKey root。WiFiProv 结束后完全释放 provisioning
> bearer，后续 BLE Central 数据面不与配网 Peripheral 并存。XIAO ESP32-S3 Core 3.3.1 门禁为
> `1,341,027 B Flash / 58,848 B globals`；Host ASan `32/32` 与 200,000 次 fuzz 通过。该入口仍是内部
> 编译门禁，不是公开 facade，也不替代生产加密板与真实 App 配网。

> **2026-08-30 Edge Hub D4b 只读预检完成：**ESP32 protected-NVS attestation 现在区分 Flash Encryption
> 未启用、分区缺失、分区未加密和 Ready；内部 probe 不打开 NVS、不烧 eFuse。Host ASan `33/33` 与
> 200,000 次 fuzz 通过，XIAO ESP32-S3 clean compile 为 `307,187 B / 20,344 B`。当前没有枚举到串口，
> 未上传固件；D4b 物理门禁仍未完成。

> **2026-08-30 Edge Gateway G2-0C 完成：**`DeviceAccessStore` 已从两槽 196 B schema v2
> 升级为三槽 260 B schema v3，允许 exactly one Admin 与最多两个 Observe|Control controller，继续读取
> 176 B v1/196 B v2，并只在下一次成功 mutation 时原子写出 v3。当前 Host ASan 32/32、architecture、ESP32/
> UNO R4/Wio SizeProbe 已通过；Small `BleFrameTransport` 在会话活动修订接入后仍严格保持 320 B，未放宽
> 资源门禁。ESP32/UNO R4/Wio 的真实断电/重新上电迁移门禁全部通过，G2-0C 已获得发布关闭；
> 下一主线为 G2-0D Edge Hub skeleton。Wio COM6 先以默认安全门禁识别并保护了既有非测试 access
> record；获得明确授权后只清除 access blob，未触及 WiFi journal或其他 QSPI 域。随后真机已完成
> `196 B v2 fixture → mutation → 260 B v3/3 slots`，物理断电/重新上电后实测返回
> `V3_POWER_CYCLE_OK`。测试 fixture 随后已限定范围清除，重启确认 `ACCESS_RECORD_EMPTY`。Wio G2-0C
> 真机门禁已关闭。XIAO ESP32-S3 COM12 使用隔离 NVS 键 `bl_g2c_gate/access`完成同一
> v2→v3 mutation 与真实断电恢复，实测 `V3_POWER_CYCLE_OK`；cleanup 后重启返回
> `ACCESS_RECORD_EMPTY`，未修改生产 `bl_v2_access/root`。UNO R4 WiFi COM9 通过隔离 Preferences
> namespace `bl_g2c_gate` 完成同一 migration 和真实断电恢复，cleanup 后持续输出
> `ACCESS_RECORD_EMPTY`，未修改正常 `bl_v2` namespace。
>
> **2026-08-30 Edge Gateway G2-0B2：**专用 session-local MQTTS management up/down topic 已进入 Broker exact
> ACL；下行固定 QoS 1/retain=false，SQLite ledger 只保存 sealed envelope ciphertext，并以 1/2.5/5/10/30 s
> 封顶退避重发 exact envelope，Broker 重启、重复 stage、retry/expiry 和 late ACK 均有门禁。设备/Node 已共享
> canonical 58 B `GatewaySecretDeliveryAckV1` golden。Cloud Gateway migration 18 保存 storageRevision/ACK time，
> 并只在 tenant/Hub/operation/digest/expiry 全匹配时以 CAS 将 topology 推进到 PendingChildInstall；replay 不重复
> 增加 topology version。G2-0B 已收口；G2-0C 三平台软件与真实断电门禁也已全部完成。
>
> **2026-08-29 Edge Gateway G2-0B1：**wrap salt 已冻结为域标识、u16 长度前缀 ASCII 字段和 u32be
> DeviceKey version，并有固定 KDF 向量。Service 使用 KMS envelope 和 exact-Hub HKDF/AES-256-GCM delivery
> envelope；普通表无 raw/plaintext secret。`edge-hub` profile 在 Gateway admission 与 Broker registration
> 两层拒绝 plain/profile mismatch，普通 Device V2 profile 的可选明文策略不变。

> **2026-08-29 Edge Gateway G2-0A：**新增独立、零堆分配的设备侧 `protocol/gateway/Contracts` 与 Service
> `edge-gateway` codec，冻结 `GatewaySecretEnvelopeV1`、AAD、`ChildRouteEnvelopeV1`、十个 topology 状态及
> 合法迁移。C++/Node exact golden 分别通过，Service protocol 13/13、App TypeScript 全量 typecheck 通过；
> App 只共享 topology 状态，不解析只属于 MQTTS management/Edge Hub 的 envelope/route。260 B access store
> schema v3 的 byte layout 已冻结；真实扩容和 v1/v2 原子迁移已在 G2-0C 完成。
>
> **2026-08-29 阶段门禁纠偏与账号恢复：**Android debug App 的既有 refresh token 已通过本机 Gateway
> 成功刷新，`/auth/me` 与 `/api/v2/devices` 鉴权通过；未绕过验证码，token 未进入仓库或日志。Wio BLE
> 24 h authenticated soak 调整为候选发布门禁：日常相关改动使用 30～60 min engineering soak，G2-0A～G2-0C
> 可并行推进。24 h 未完成前，Wio BLE 仍是 opt-in preview，Edge Gateway 也不得宣称 production ready。
>
> **2026-08-29 Wio BLE G1D 连接治理真机门禁：**官方 RTL `2.1.3` 上已完成三次“连接但不订阅”
> deadline（14.454～14.487 s）、三次“订阅但不认证”deadline（15.096～15.124 s）和 32/32 次完整 GATT
> 建连/订阅/断连。FreeRTOS heap 在测试前后均为 `free=17,120 B / minimum=17,072 B`，newlib
> headroom 均为 `109,816 B`；architecture、Windows ASan 30/30、200,000 fuzz 和 Wio clean compile
> 同步通过。未认证 central 不能无限占用唯一连接，短门禁已闭合；Wio 仍保持 opt-in preview，唯一剩余
> G1D 发布项为真实 owner App + DirectSecureRecord 的 24 h soak，但它不再阻塞 G2-0A 合同实施。详见
> `DEVICE_V2_WIO_BLE_G1D_RELEASE_GATE_REPORT.md`。

> **2026-08-28 XIAO ESP32-S3 BLE 真机收口：**官方 ESP32 Port 已统一为 ESP-IDF NimBLE 普通 GATT
> bearer，默认不请求 SMP/pairing/bonding；Noise、Method 2 与 DirectSecureRecord 继续强制提供应用身份、
> AEAD 和防重放。Android + 本机 Service 已完成 Provisioning、grant/receipt/commit、Direct、Manifest/State
> 与 Power Command，协商 MTU 256 但 BLE v1 分片仍固定 20 B，全程无系统 PIN。当前 BLE-only presence
> probe clean compile 为 690,475 B Flash、38,564 B global RAM；Host ASan 30/30 + 5,000 fuzz、App 241/241
> 回归通过。clean probe 重新烧录且保留 NVS 后，App 重新发现 Direct 广播、恢复 Method 2/Manifest/State，
> Power 下发使 State 0 更新为 State 1。Wio preview 与 ESP32 WiFi+BLE 的剩余门禁状态不因此改变。
> 同一源码的公开 `WiFiBleQuickStart` clean compile 为 1,281,187 B Flash、59,988 B global RAM，证明
> WiFiProv 与 ESP-IDF NimBLE C API 可以在一个 ESP32 产品中链接；这只是编译门禁，不替代其剩余模式切换、
> RF/heap 和同时在线真机门禁。

> **2026-08-27 Wio Terminal WiFi/BLE 真机复核：**Wio WiFi 已通过 DeviceKey、MQTT、初始
> Manifest/State、Power Property 与 Button Action 双向链路。Wio BLE 改为显式
> `BLINKER_ENABLE_WIO_BLE_PREVIEW=1` 的实验入口，不再依赖 RTL8720DN 原生 bonding：首次认领使用
> Noise + server-signed grant，日常连接使用 Method 2 + DirectSecureRecord。已通过 Android 首次认领、
> Provisioning -> Direct 广播切换、Manifest/State、Power/Button、同版重刷持久恢复，以及真实 USB
> 断电后的同设备自动重连。rpcBLE 断连自动重启旧广播造成的 mode race 已由读取实际 GAP 状态的非阻塞
> 重启状态机修复。Wio BLE 仍不进入正式发布矩阵；反复断连和单连接占用 DoS 已于 2026-08-29 真机闭合，
> 当前只剩真实 owner DirectSecureRecord 的 24 h 长稳；
> 闭源 RTL bond fork 只保留诊断证据，不再作为业务安全根。
>
> **2026-08-26 ESP32/UNO R4 基础真机稳定化：**UNO R4 WiFi、UNO R4 BLE、XIAO ESP32-S3 WiFi 与
> XIAO ESP32-S3 BLE 的公开 Product 均已完成真机基础链。XIAO BLE 复测发现并修正未协商 MTU 导致的
> 跨平台分片回归：BLE v1 统一固定 20 B bearer packet，删除 ESP32 动态 MTU 状态后全局 RAM 从
> `39,516 B` 降至 `38,396 B`；Method 2、DirectSecureRecord、Manifest/State、Command/ACK、
> active-session `resetAccess()` 与同 logicalDeviceId 重认领通过。XIAO WiFi 为 `974,927/49,768 B`，
> DeviceKey/MQTT、Manifest/State、Command/ACK 及主动断网恢复通过。尚未完成的重点仍是 ESP32
> WiFi+BLE 模式切换/同时在线、WiFiProv 真机和长稳，不得用本批基础链替代这些门禁。

> **2026-08-23 架构收口复核：**当前最高优先级已从 D5/D6 或继续扩展 WiFi+BLE，调整为
> `DEVICE_V2_ARCHITECTURE_CLOSURE_AUDIT.md` 的 C1～C4。无正式 Product 消费者的 LocalControllerPairing、
> DirectBleLifecycle、Factory bundle C++、重复 DeviceKeyStore、OwnershipRelease 全链及 UNO R4 专用 blob 已物理删除；
> `resetOwnership()` 伪公开 API 同步撤回。删减后 package 为 292 文件，Host ASan 36/36、pre-enrollment exact、
> 5,000 fuzz、architecture/package check 通过。
>
> 本轮同时纠正两个过度结论：第一，纯 BLE 与 ESP32 WiFi+BLE 只有设备图/静态入口，App/Service enrollment/bootstrap
> 尚未闭合，因此改标 preview；第二，模式 `.ipp` 只隔离第三方 SDK 发现，不能消除 Arduino 对 91 个普通 `.cpp` 的
> 递归编译。删减后单 ZIP 已完成隔离安装与依赖版本校验，但 ESP32 WiFi clean compile 在本机 6 分钟仍未结束并被主动
> 中止，8 目标不得记为本批通过。下一设备批先完成 BLE access-grant 合同与旧 Cloud/Ownership saga 删除，再收敛
> translation unit；History/Bulk 继续暂缓。

> **2026-08-23 最新执行点：**`DEVICE_V2_DIAGNOSTICS_OUTBOUND_TELEMETRY_PLAN.md` 的 D1～D3、D4 软件核心及
> Wio Terminal 1 Hz Cloud 真机门禁及 Cloud/Direct 双 route Core 隔离已完成，当前继续收口 D4 的 ESP32 正式
> WiFi+BLE composition/真机同时在线、移动端 kill 和真实 ACL revoke，
> 不进入 D5 History/D6 Bulk。设备端已补齐 `telemetrySamples / telemetryDropped / leaseExpired`，并冻结
> best-effort 丢旧语义：失败 sample 不重放，下一成功帧以 sequence gap 暴露丢失。
> Host ASan `41/41`、pre-enrollment exact、200,000 fuzz、架构检查、325 文件单库发布检查均通过；32 位
> Small/Standard/Large `Device` 为 `1664/3448/10980 B`；Small 以 36 B 固定 RAM 将 lease 从 1 槽修正为 2 槽。
>
> 2026-08-23 正式安装 ZIP QuickStart：ESP32 WiFi `990,571/50,192 B`、UNO R4 WiFi `134,176/13,028 B`、
> ESP32 BLE `772,863/43,384 B`、UNO R4 BLE `201,308/17,216 B`、ESP32 WiFi+BLE
> `1,406,115/62,848 B`。Wio 最近门禁为 WiFi `138,872 B Flash`、BLE
> `192,368 B Flash`。Wio WiFi 的 DeviceKey/MQTT/BBP2、Gateway outage、跨 session expiry、无复位恢复，以及
> Telemetry Open/连续 1 Hz Data/Close/设备 30 s lease expiry 已通过；下一设备真机项是 ESP32 正式组合产品的
> Cloud/Direct 同时在线。
> 同一安装 ZIP 的 ESP32 `CloudSmokeTest` 为 `1,009,455/50,224 B`，dependency、ELF defined-symbol 与
> 安装源码 provenance 门禁通过，可继续作为最小服务器联调例程。
> XIAO ESP32S3 子门禁已完成正式 `<BlinkerWiFiBLE.h>` 产品图编译（`1,323,651 B Flash / 59,316 B global RAM`）
> 和 COM3 Flash 逐段 hash 校验。设备仍枚举为 ESP ROM 下载口且疑似损坏，未观察到配网服务或应用日志，
> 所以运行期、动态 heap 与 RF 门禁仍未通过；D5 History/D6 Bulk 不因此提前。软件侧另已通过 XIAO S3
> BLE WiFiProv 可选图（`1,303,487/59,684 B`）的 clean link/ELF 门禁。

本轮真机使用当时的 `tools/build_device_v2_interop_package.py` 生成仓库外单库配置，以公开 `CloudSmokeTest` 烧录
COM6 Wio Terminal；正式示例为 `141,512 B` Flash。App 仓现有 `DeviceV2Session` 经账户 MQTT 识别
`uptimeSeconds` 的 1000 ms realtime Manifest 并收到连续秒值，随后显式 Close。Broker 暂停超过设备固定 30 s
租期后，Wio 的 `leaseExpired` 从 0 增至 1、`telemetrySamples` 停在 38；Broker 恢复后设备回到
`active/cloud=1`。联调同时发现并修正两项全局缺陷：`Product` 现在先 attach lifecycle 的 transport/clock 再
prepare realtime schema；Broker 按设备 `monotonicMs` 而非服务器到达墙钟降频，并新增 999 ms 抖动回归，避免
1 Hz 被错误折半。

本轮补齐双 route 资源和公平性：Small 的两个 peer/transport 不再争用唯一 lease 槽；Runtime 在一次 `poll()` 中
有界处理最多 `BLINKER_TELEMETRY_LEASE_SLOTS` 个 due/expired 项。Host 已验证 Cloud 与 Direct 使用相同
session-local `streamId` 时仍拥有独立 epoch、sample sequence 和 exact target，Direct session close 不影响 Cloud。
旧 `WiFiProduct.h` 中没有生产调用方的 ESP32 internal lifecycle 声明已删除。ESP32 WiFiProv+Direct 已从内部
probe 收口为标准 `src` 中的唯一业务 composition 和公开 `<BlinkerWiFiBLE.h>`；配网承载默认为 SoftAP
WiFiProv Security1，IDF-NimBLE 目标可编译期选择 BLE WiFiProv，日常 Direct 仍使用 NimBLE；二者都不再错误
要求 BLE EnrollmentTicket composition。
无正式消费者的旧 `BasicWifiBleLifecycle/WifiBleLifecycle` 已从 runtime、Advanced 聚合头、SizeProbe、架构正向
检查和专属 host 测试物理删除；`Esp32WifiBleProduct` 是唯一 WiFi+BLE 产品编排。retired-path 反向门禁与保留的
BLE-only/WiFi-cloud 测试已通过完整 41/41 ASan 和 200,000 fuzz；UNO R4 Small Advanced SizeProbe clean build
为 `52,632 B Flash / 16,152 B global RAM`。

> **2026-08-19 主线重排：**用户凭据已统一为无前缀 `DeviceKey`，默认 Education 登录改为
> `DeviceKey challenge-HMAC -> SessionCredential`，Key 刷新强制吊销旧 session/连接。现有
> EnrollmentKey/WifiOnboarding/CloudSession 旧图已物理删除；CloudCredential/Noise 相关代码状态仍在本文记录，
> 但只代表仍有测试消费者的软件实验，不等于默认发布方向。DKEY-1～DKEY-2D、ESP32 WP-P3/P5/P6 的设备软件门禁已经完成；
> Controller control v2 Core、BBP/2 family、Runtime 权限门禁以及正式 P6 接线也已完成。设备不保存
> ownership binding，改用 4 B accessEpoch、两槽 Admin/Shared 和 176 B atomic DeviceAccessStore。当前设备侧
> 最高优先级改为稳定并联调 `DeviceKey/WiFi -> SessionCredential -> MQTT -> BBP/2` 功能主线；该链 Host
> 模拟 E2E 已完成；登录合同、provider/MQTT 和 ESP32/UNO R4 内部 composition 已进入 SHA-256 漂移门禁，
> 相邻服务仓的零依赖 Debug Gateway/Broker 已通过真实 HTTP/MQTT socket E2E，ESP32-S3/Wio WiFi 真机基础链也已通过；
> production Service/Broker、Telemetry 与长稳仍待验证。Cloud Frame Authentication 移入后续安全批次，不阻塞
> 当前功能互操作，但仍阻断 Plain MQTT 生产安全档；见
> `DEVICE_V2_DEVICE_KEY_CONTRACT.md`、`DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md`。

当前稳定化门禁证据（2026-08-19）：删除 3 个旧图专属测试后 Host MSVC ASan `40/40`、pre-enrollment exact、
`20,000` 次 deterministic fuzz 通过；四个公开 Product 的 Core 闭包包分别实编为 ESP32 WiFi
`980,451/49,856 B`、ESP32 BLE `764,423/43,064 B`、UNO R4 WiFi `125,488/12,696 B`、UNO R4 BLE
`193,432/16,896 B`（Flash/global RAM）。263 文件 Core ZIP 解压后在 Wio Terminal 为 `62,980 B` Flash；
architecture 与 schema 4 全配置/确定性 package gate 通过。当时的单库联调配置工具为
`tools/build_device_v2_interop_package.py`；该重复生成器已于2026-09-24退役，当前改用
`tools/build_arduino_release.py --profile <development-profile.json> --allow-development-profile`。
历史测试结果不因此改写。具体边界与真实联调清单见
`DEVICE_V2_CLOUD_LOGIN_STABILIZATION.md`。
> UNO R4 产品矩阵已冻结为独立普通 WiFi 与独立普通 BLE，不实现 BLE 配网或 WiFi+BLE。ESP32/UNO R4
> 普通 BLE 的 App/Service/设备真机基础链已闭合。Wio Terminal WiFi 已真机；Wio BLE 源码保留在同一库中，
> 但必须显式 opt-in 且不属于正式发布矩阵，详见本文顶部 2026-08-27 更新。

> 更新：2026-08-18。状态按“代码存在、软件验证、真机验证、三端生产接入”分开记录，避免把
> reference 实现写成已发布功能。
> 最新纠偏：productId 公共/Advanced/HELLO/ClaimStart 入口已撤回；WiFi-only、WiFi+BLE、BLE-only、
> LocalBLE 的身份/凭据语义已修订，Controller、Ownership Claim 与 Cloud Credential Enrollment exact
> wire 已冻结，Ownership Reset/Transfer 已完成；WiFi-only EnrollmentKey HTTP outer 曾完成软件探针，但
> 已被长期可查看/刷新的 ManualDeviceKey + 独立 RecoveryGrant 决策取代。Ownership Claim
> 掉电 journal、四 endpoint、strict error、完整 WiFi onboarding flow、1,414 B 共享 arena 与 40 B staged
> lifecycle、真实软件 crypto provider、统一 public onboarding profile 与 ESP32/UNO R4 official composition
> 已实现；R4b-4a Ticket common contract/verifier、R4b-4b shared Noise secure wire、R4b-4c
> LocalSetup application envelope、R4b-4d durable setup saga、R4b-5A 平台无关 WiFi+BLE lifecycle 与
> R4b-5B ESP32、R4b-5C UNO R4 internal official composition、R4b-5D session/fault gate、R4b-6A
> LocalBLE exact pairing、R4b-6B Local application/shared BLE lifecycle，以及 R4b-6C 双平台 Platform
> BLE/LocalBLE internal official package 与资源门禁已完成。
> 普通 BLE 的设备 facade/package 已完成；WiFi+BLE 只保留 ESP32 internal 候选。真机验证与
> 生产三端接入尚未完成。当前已重排为 R4b-0～R4b-7：
> 先隔离旧 Enrollment/1，并完成与 wire 无关的身份/存储底座，再冻结三端合同、实现 coordinator，最后
> 才冻结 facade 和 official composition。

## 当前结论

| 范围 | 状态 | 说明 |
|---|---|---|
| Core/BBP2 | 软件基线完成 | 无 heap/JSON/Arduino 依赖；Frame、restricted CBOR、Manifest、State、Command、Event、ACK/Error |
| 公共业务语义 | 正式修订已实现 | 仅 Property/Action/Event；旧 Widget/WidgetCatalog 已删除 |
| 用户/Agent QuickStart | **普通 WiFi/BLE 设备 API 已完成** | 全局 `Blinker`；WiFi include `<BlinkerWiFi.h>` 后使用 `begin(DeviceKey, SSID, password, ...)`，BLE include `<BlinkerBLE.h>` 后使用 `begin(interactions...)`。transport 不再作为 begin 参数；UNO R4 不提供 WiFi+BLE |
| 状态/资源 | 软件基线完成 | compact slots、共享 arena、流式 Manifest SHA-256、Manifest/State 分页 |
| BLE wire | **跨平台软件与 ESP32/UNO R4 真机基线完成** | 4 B header、固定 20 B bearer packet、重组和 secure-record seam；MTU 扩展必须显式协商，仍需长稳/恶意断连 |
| MQTT/cloud | **DeviceKey 三端链与 ESP32/UNO R4/Wio 真机已闭环** | challenge/HMAC、短期凭据、CONNECT/SUBSCRIBE、BBP/2 State/Command/Telemetry 已通过本机 Gateway/Broker；Broker/AP 断开和设备自动重连已过。Cloud Frame Authentication 后续实施 |
| WiFi composition | **手动 ESP32/UNO R4 已公开；ESP32 WiFi+BLE 正式唯一图完成** | 共享 DeviceKey Product 注入 Platform/Key source；public WiFi/WiFi+BLE facade、同一 ZIP 和 QuickStart 已实编。旧 WiFi+BLE CloudCredential/Ownership 图及 package/probe 已删除；P3 仅是历史资源 probe，P6 是正式设备图 |
| ESP32 WiFiProv | **默认 BLE 的 G1B 核心与 Cloud+Direct 真机已完成，完整发布门禁未完成** | XIAO 已过 network-only reset、BLE Security1、GetInfo identity、WiFiProv→Direct、WiFi 重试、DeviceKey challenge/session/MQTT、Cloud fallback 与 Direct 自动抢回；当前 `1,279,291/53,988 B`，steady/min heap `159,560/135,076 B`、largest block `116,724 B`、loop stack watermark `4,296 B`。SoftAP 是编译期回退；仍需 SoftAP、循环/系统任务水位/RF/长稳 |
| UNO R4 产品矩阵 | **普通 WiFi 与普通 BLE 真机基础链完成** | 不做 BLE 配 WiFi、不做 WiFi+BLE。WiFi 使用手动 DeviceKey；BLE 已通过认领、持久重连、Command/ACK、active-session access reset 与同身份重认领，仍需长稳/恶意断连 |
| Wio Terminal 无线产品 | **WiFi 真机完成；BLE opt-in preview 连接治理完成** | WiFi 已完成 DeviceKey、HTTP session、MQTT/BBP2、初始 State、Property/Action、Gateway/Broker 恢复与 1 Hz Telemetry；BLE 已完成 Noise/Method 2/DirectSecure、Manifest/State、Property/Action、同版重刷、真实断电、认证 deadline 与 32 次重连，只待 24 h authenticated DirectSecure soak |
| 制造凭据 | Cloud/WiFi 软件链完成 | 平台无关 `education-plain` 输入、228 B BFB2、ESP32 NVS executor、UNO R4 USB Serial executor；BFB2 不含 Ownership，服务器生产 API/工位与真机未接入 |
| EnrollmentTicket | R4b-4a common contract 完成 | 268 B body/216 B transcript；绑定 setup session/hash、generation、controller digest/权限与 operation mask，三端 exact golden 已通过；不等于 BLE 配网 coordinator 已完成 |
| BLE provisional secure wire | 软件实现完成、下沉 Enhanced | 普通NN/增强NNpsk0共用`NoiseNnSession`并有 exact golden；不再是 ESP32/UNO R4 初学者默认配网前提，保留作 Enhanced/跨平台实验 |
| LocalSetup application | R4b-4c Core 完成 | strict Hello/Ticket/8-operation envelope、最终 Noise hash callback、Ticket exact gate、sequence/replay 与产品 handler seam 已实现；216 B 32 位对象 + 512 B caller workspace |
| LocalSetup durable saga | R4b-4d Core 完成 | 248 B BSG2 串联 Ownership/Controller/Network/Cloud；ConfigureNetwork 与 App-relay Cloud path 已冻结；32 位 record/store/coordinator 为 240/4/40 B，未确认 network candidate 可安全替换 |
| BLE/WiFi+BLE composition | **普通 Platform BLE 三端基础链完成；WiFi+BLE 仅 ESP32** | UNO R4 历史 WiFi+BLE 源码/目标已删除。ESP32/UNO R4 使用统一 facade；XIAO/UNO R4 BLE 三端 E2E 已完成。ESP32 WiFi+BLE 无断电模式切换、Cloud+Direct 同时在线、BLE-first/Cloud fallback/Direct 自动抢回已过；长稳仍未完成 |
| Device instance identity | 首批代码已实现、host 已验证 | 16 B CSPRNG ID + 28 B atomic/CRC record + `dev_` lowercase-hex；允许 PlainFlash，因为 ID 公开；记录损坏不静默换 ID |
| Cloud credential | 未发布实验合同/代码完成 | BCS2/BCE2/X25519 envelope 仅作 Enhanced/历史候选；Education 默认 graph 将移除 enrollment/store，改用 DeviceKey 直接申请短期 session |
| Ownership record | Claim、Claim journal 与 Release v1 已冻结 | 32 位 OwnershipRecord 104 B/固定112 B BOS2；Claim 使用 36 B RAM/48 B BOC2 journal 与 20 B coordinator，只有验 server-signed CommitAck 后才清；Release 使用 168 B BOR2 journal、156 B record、28 B coordinator，先写 next-generation tombstone，server-signed CommitAck 后再清 Claim/Cloud/Controller/Network |
| DeviceKey + 强制轮换 | DKEY-1～DKEY-2D/P6 access + 设备公开 cutover 完成 | provisioning v2：P3 45 B Install；P6 101 B Bootstrap；当前 260 B schema v3 原子保存 Key+epoch+Presence+Admin/Shared/Gateway 三槽并兼容读 176 B v1/196 B v2。DeviceKey session 已驱动 Wio 真机 MQTT/BBP2，Broker 断路/恢复已过；KMS/pending rotate 仍待完成 |
| Server signature crypto | 软件/SDK 门禁完成 | 窄 `IP256DigestVerifier`、`IEd25519DigestVerifier`、Flash-borrowed key ring；ESP32 mbedTLS P-256、UNO `SoftwareATSE` P-256、portable Ed25519 均有 valid/mutated/canonical vectors。硬件运行与发布前审计未完成 |
| Controller 鉴权与控制面 | **v2 Core/BBP Runtime + 正式 P6 已接线** | method 2、193 B Grant、231 B Mutation、encrypted local + ManageControllers 门禁保持；P6 使用 accessEpoch 和 Admin/Shared/Gateway 三槽，Security1 首次 Admin bootstrap、P-256 verifier、最后 Admin 保护与静态资源重测已完成。设备只识别权限，Shared/Gateway 角色由 Service 区分；LocalBLE 继续独立 physical-confirmation 合同 |
| ESP32/UNO R4/Wio Core | **profile-tag、公开 Product 编译及主要真机基础链通过** | ESP32/UNO R4 的普通 WiFi/BLE 已真机；Wio WiFi 与 opt-in BLE 也已完成真机基础链，但 Wio BLE 尚未获正式发布准入；UNO R4 与 Wio 不提供 WiFi+BLE |
| App/服务器 | **V2 核心通信链已接入并完成本机三端联调** | production 路径已具备账户 MQTT、Route/Delivery、Manifest/State/Command 与 Telemetry 聚合；当前 App `DeviceV2Session` 已直连 Wio 验证。页面完整 UX、移动端 kill、真实 ACL revoke 和 production 部署仍待门禁 |
| Legacy JSON | **三端不再兼容** | 旧 JSON 连接、协议 adapter 与迁移承诺均不在 V2 发布范围；App 既有页面布局代码只作为 V2 页面改造基础，不保留 JSON 数据链 |

R4b-5A 当前软件门禁：Windows ASan 36/36、200,000 次 deterministic fuzz、架构检查与三端全量 golden、
15-package boundary、六平台 Core ELF 均通过。UNO R4 storage 为 68,660 B Flash / 7,744 B global RAM，
Small SizeProbe 为 52,332 B Flash / 14,668 B global RAM；official Plain QuickStart 当前复验为
ESP32 1,020,659/50,576 B，UNO R4 最近一次全源 clean compile 为 159,148/13,360 B。四种 Plain/TLS
聚合包结构与确定性通过。
32 位 ABI 仍以各类型静态门禁为准；SizeProbe 常驻测量数组，以上 sketch 摘要均不代表 WiFi+BLE 产品峰值。

DKEY-2C 收口后，Windows `/W4 /WX` + ASan host 为 41/41，deterministic fuzz 为 200,000 次；
ESP32/Wio Terminal Core clean compile 为 `324,343/21,672 B`（Flash/RAM）与 `48,236 B Flash`；
UNO R4 Small SizeProbe clean compile 为
`52,428 B Flash / 16,616 B global RAM`。最终 ELF 中 DeviceKey、DeviceKeyLocator、DeviceKeyStore、
ConfiguredDeviceKeySource、HttpDeviceKeySessionProvider、DeviceKeySessionArena、WifiCloudLifecycle 分别为
`32/16/8/40/280/1260/20 B`；SizeProbe 故意同时常驻全部 ABI 数组。provider 的
request/response/credential arena 由 caller 提供，proof transcript 流式进入 HMAC，不另存完整 scratch。
request/response body 门槛已由旧 provider 沿用的 256/640 B 收紧到 wire 精确上限 147/595 B，caller
最低需求减少 154 B；恒定 HTTP header 改为共享只读描述符，又从 provider 移除 32 B。280 B 已作为
静态上限，而非用更宽松估算掩盖增长。

DKEY manual-WiFi official internal composition 使用相同模板，仅由平台最小 network/storage class 提供
WiFi station、DeviceInstance blob 和两个 Arduino Client。Plain clean compile 为 ESP32
`986,287/48,360 B`、UNO R4 `118,164/11,200 B`；旧兼容图同轮为 `1,023,823/50,576 B` 与
`159,140/13,304 B`。新包不包含 ESP32 Crypto/Arduino Crypto adapter 或 CloudEnrollment public key 配置，
公开 WiFi 已切到 DeviceKey；生产发布仍由 Service route、Broker ACL/主动断线和真机门禁阻断。

DKEY-2D 不复用旧 Ticket/Ownership/CloudCredential saga。Core `DeviceKeyProvisioningEndpoint` 的 32 位
ABI 上限为 16 B，只接受 2 B `GetInfo`、45 B `Install` 与 P6 101 B `Bootstrap`，响应最大 24 B；同一 setup session 不允许第二个不同 Key，相同 Key
可幂等重试。ESP32 adapter 使用 `network_prov_mgr` Security1，收到 WiFi credential 后写入现有 Blinker
双槽 journal，并把 Arduino WiFi 设置为 RAM persistence，避免 SDK NVS 与 Blinker NVS 重复保存网络密码。
本批普通 Host 42/42、Windows ASan 42/42、architecture check 与 official aggregate package boundary 均通过；
UNO R4 Small SizeProbe 的历史 clean compile 为 `52,436 B Flash / 16,628 B global RAM`；当前端点门槛已随
可选 `DeviceAccessStore` 接线修订为 `<=16 B`，旧 12 B 数字不得继续作为当前 ABI 断言。

WP-P3 将 ESP32 WiFiProv 与持久 DeviceKey source 接入现有 cloud stack：缺少 Key/WiFi 或启动时显式 force 才进入
Security1，成功后 deinit provisioning manager，再启动 DeviceKey challenge/session、PubSubClient 与 BBP/2。
最终 P3 Plain 链接为 `1,758,907/62,964 B`；同轮手动回归为 ESP32 `986,483/48,368 B`、UNO R4
`118,156/11,208 B`。ELF 无旧 CloudCredential、EnrollmentKey、Ownership 或旧 Crypto provider 符号。

WP-P4 曾验证 WiFiProv 与 Direct BLE 共用 Arduino-ESP32 Core 3.3.1 Bluedroid 的单 Host 边界，最终为
`1,822,043/65,776 B`，slot 余量只有 7.33%，未通过门槛。P6 通过且没有任何发布消费者后，P4 的 product
wrapper、platform、package profile 与 probe 已物理删除，避免失败产品图继续成为维护/API 负担。最终 profile
收口时确认旧 Bluedroid `esp32-ble` adapter 也没有产品消费者，已从源码、manifest 与 CI 删除；ESP32 普通 BLE
产品只使用 `esp32-nimble`。

WP-P6 将 Network Provisioning manager、transport scheme 与 Direct BLE extension 解耦。P6-S 是只链接
SoftAP Security1 与 NimBLE Direct 的回退图；接入 access/control 前历史 Plain 为 `1,375,887/60,856 B`。历史工作树 Plain 为
`1,406,115/62,848 B`，双 OTA app slot 余 `559,965 B`（28.48%）；`1,395,187/62,848 B` 仅为
历史快照。ELF defined symbols 证实
`network_prov_scheme_ble`、protocomm BLE、Bluedroid、旧 CloudCredential/Ownership/Noise product graph 为 0；
完整静态 link 已通过。P6-B 是当前默认，只在 Arduino Core 已启用 IDF-NimBLE 的目标开放：历史 XIAO S3 为
`1,303,487/59,684 B` 且 ELF 不含 SoftAP/Bluedroid；经典 ESP32 会形成 `1,899,723/73,052 B` 的双 Host 图，
已拒绝。2026-09-02 当前隔离 ZIP 真机 probe 为 `1,279,291/53,988 B`，`WiFiProv deinit -> Direct BLE` 及
active Direct→WiFiProv→Direct 与 Cloud+Direct BLE-first/fallback/自动抢回已过；SoftAP App、增强档 PoP、
100 次循环、RF 共存与长稳仍是发布阻断。系统任务已可观测且两轮没有累积下降，但 `ipc0` 最低仅 76 B，
因此系统任务门禁仍未通过。当前未插桩 composition 的 heap/loop-task 快照已通过：steady/min free heap
`159,560/135,076 B`、largest 8-bit block `116,724 B`、loop stack watermark `4,296 B`。

WP-P5 使用相同 P3/P6 composition，仅由 release config 注入 `WiFiClientSecure` 与 CA，不复制 product graph。
P3 TLS 为 `1,845,907/63,556 B`，只余 `120,173 B`（6.11%），因此拒绝作为该目标默认配置；P6 TLS 为
`1,463,067/61,456 B`，余 `503,013 B`（25.58%），是接线前历史门槛。Plain/TLS 都是编译期显式选择，运行时不得
失败降级；默认是否启用 TLS 仍由目标板资源档决定，而不是全库一刀切。

R4b-5B 是早期 internal ESP32 WiFi+BLE 聚合包历史证据；其当时资源档为
NimBLE-Arduino 2.5.0 + `Minimal SPIFFS (1.9MB APP with OTA)`：1,296,571/1,966,080 B Flash、
65,448/327,680 B global RAM。默认 1.25 MiB APP 的初始实编只余 14,197 B，不作为发布档；Core Bluedroid 同图为
1,795,395 B（默认分区 136%），不进入 official。此结论仍不替代真实 GATT/WiFi 共存、配对、掉电与长稳。

下列 R4b-5C UNO R4 WiFi+BLE 结果仅是历史架构证据；对应 graph/package/probe 已删除且不再是产品待办。
该实验曾通过确定性聚合、ArduinoBLE 2.0.2 exact-handle security probe 与完整
composition 编译。通用 ArduinoBLE adapter 已移除 any-peer `BLE.paired()` 和业务可写安全状态；缺少
`IArduinoBleSecuritySource` 时加密 profile fail closed。完整图为 210,804/262,144 B Flash、
18,812/32,768 B global RAM，静态 singleton 为 9,008 B；WiFi-only 回归为 159,156/13,304 B 且不链接
ArduinoBLE。13,956 B 剩余 RAM 当时未经过真机 stack/heap watermark，不能作为当前产品准入证据。

R4b-5D 已完成无真机 fault gate：record transport 只接受已连接准入的 session，同一活动 ID 二次宣布、
安全位回退、漏失 disconnect 均清除 RX/TX 和上层授权状态；所有产品启动阶段失败均回收到 `Stopped` 并可重试，
Direct endpoint 物理 Error 明确进入产品 Fault。Windows ASan 36/36、200,000 fuzz、架构/15-package/official
确定性门禁通过。R4b-5D 完成时完整图为 ESP32 1,297,371/65,448 B、UNO R4 211,332/18,812 B
（Flash/global RAM），比上片分别增加 800/528 B Flash、0 B static RAM。其后 R4b-6A/6B 已完成
LocalBLE exact pairing、application 与共享 lifecycle；动态 heap/stack 与双无线共存仍须真机。

R4b-6A 新增独立 LocalBLE pairing contract/coordinator，不引用 EnrollmentTicket、ControllerGrant 或
OwnershipSource。Noise final hash + 设备侧物理确认 + CSPRNG nonce 打开短窗口；设备策略固定权限，持久化
`Local/generation=0/version=1`，流式计算 HMAC receipt。同窗口 exact replay 可恢复，Store 成功即锁 digest，
response loss 后 different request 被拒绝。最终 ASan 38/38、200,000 fuzz、exact golden、架构/15-package/
official package 通过；UNO R4 ELF map 为 124 B coordinator，ESP32/UNO R4/Wio Core clean compile 分别为
322,023/21,672 B、69,304/7,664 B、48,236 B Flash（前两项为 Flash/global RAM）。这些只准入 R4b-6A
Core；其 application/lifecycle 缺口由 R4b-6B 关闭，双平台 internal composition 由 R4b-6C 关闭；真机仍未完成。

R4b-6B 固定 Local application Hello `7/36 B`、Challenge `7/91 B`、Pairing `141/177 B` 与错误
`14 B`；Local capability `0x0043/0x0061` 与平台 Enrollment 互斥。共享 `BasicBleSetupLifecycle` 仅统一
locator/mode/completion/断线恢复，平台读取 BSG2、Local 读取 generation 0 credential。32 位
application/coordinator/BLE-only/WiFi+BLE lifecycle 为 `56/124/44/52 B`。最终 WiFi+BLE 回归图为 ESP32
`1,299,043/65,464 B`、UNO R4 `211,708/18,828 B`；随后 R4b-6C 已完成 internal BLE-only package/resource。

R4b-6C 将共享部分严格收敛为 `BleOnlyRadio + BleWorkspace + platform radio glue`，没有从 WiFi+BLE 图保留
PubSubClient、HTTP、WiFi client、CloudCredential 或 Network journal。Platform BLE 独占 Ticket/Ownership/
server-signature 路径；LocalBLE 独占本地物理确认路径，生成配置没有 server/cloud 字段。四个 official internal
profile 的确定性双构建和 clean compile 已通过；ESP32 Platform/Local 为 `760,639/41,584 B`、
`731,287/40,792 B`，UNO R4 Platform/Local 为 `186,060/15,420 B`、`159,976/14,840 B`。完整 singleton
依次为 `6,960/6,176/5,632/5,072 B`。共享 glue 后 WiFi+BLE 回归为 ESP32 `1,299,055/65,464 B`、
UNO R4 `211,716/18,828 B`，相对 R4b-6B 只增加 `12/0 B` 与 `8/0 B`。公开 facade 继续被
exact-encryption、配对恢复、动态内存与长稳真机门禁阻断。

## R4b-4d 正式修订

- 新增 `LocalSetupOperationContract`：首次 Controller 直接发送 Noise-protected 32 B secret，Ticket 已签名绑定
  controller metadata/digest，不再套一份冗余 ControllerGrant；ConfigureNetwork 冻结 SSID/auth/credential/
  hidden canonical body 与 CandidateStored response。
- LocalSetup 扩展 operation 6～8，选择 App exact relay CloudCredential Enrollment。设备不先联网，App/Gateway
  relay exact request、signed Grant、receipt 与 HMAC CommitAck；raw cloud secret 只存在于 sealed envelope。
- 新增单一 `LocalSetupSaga` 与 BSG2 journal，使用 ClaimCommitVerified/CloudCommitVerified checkpoint 封闭
  “ACK 已验证但 inner journal 尚未清/已清但 outer state 尚未推进”两类掉电窗口。
- 修正 network replay 与恢复：相同 ConfigureNetwork body 不再重复 commit；未确认 candidate 可在 Cloud 开始
  前替换，Complete 后首次 AP 失败也可在同一 durable ticketId 下重开 secure setup；confirmActive 后关闭。

- 新增 allocation-free `LocalSetupApplication` 与 strict canonical CBOR envelope，连接
  `BleLocalProvisioningEndpoint` 的最终 Noise handshake hash；Ticket exact 验证前 mutation 不可达。
- 请求使用非零 uint32 串行 sequence；exact replay 由 request digest 识别，同 ID 不同 bytes fail closed。
  BLE-only/WiFi+BLE operation profile 仅允许 0/3，完整 Ticket 不驻留，workspace 由 caller 复用并清零。
- 产品 mutation 收敛到 `ILocalSetupOperationHandler`；其 `prepareSetupSession` 负责识别 fresh/可恢复/Active，
  避免 Ownership 已落盘但 Controller 未安装时被永久挡在 setup 外。单一 durable saga 已由本批实现。
- official WiFi composition 已注入 DeviceInstanceId、Claim/Cloud journals、server signature key ring、
  sealed enrollment、共享 arena 与 staged lifecycle；删除原来仅 `WifiCloudLifecycle` 的 pre-enrolled-only 图。
- 新增 `WifiOnboardingProfile`：普通 Sketch 仍只 include `<Blinker.h>`，通过同一 `begin()` 选择
  `WiFi.onboard(...)`；没有公开 ESP32/UNO crypto、storage、HTTP 或 MQTT 对象。
- 官方包配置新增最多四把 server signing public key 与 Cloud X25519 encryption public key，ESP32 固定
  P-256，UNO R4 固定 Ed25519。它们是发布期公钥，不是设备密钥；私钥禁止进入包。
- UNO R4 official 包增加 `Crypto (=0.4.0)` 精确依赖；ESP32 只用 core mbedTLS。两平台继续使用
  PubSubClient，不引入 ArduinoMqttClient 或第二套 MQTT runtime。

- 新增 `WifiControlPlaneArena`，以 max(request/response/workspace) 复用 Onboarding 与 Cloud Session 的
  非重叠阶段，固定 1,414 B；不再按 1,414 + 734 B 同时常驻两套控制面 buffer。
- 新增 `WifiOnboardingCloudLifecycle`：注册前强制关闭 cloud network gate，按 WiFi → Onboarding →
  Cloud/MQTT 排序；断网、Backoff、RestartTransaction 和永久 Key/Ownership 错误有明确状态。实现直接读取
  flow 状态，删除了四个重复布尔量，UNO R4 32 位对象保持 40 B。
- UNO R4 Preferences bank 追加 DeviceInstanceId、Ownership Claim、Cloud Enrollment、Ownership Release
  四个 crash-consistent 双槽域；原 BlobId 0..5 与 key 顺序保持不变，不需要破坏式迁移。
- official composition 的软件接线已经完成；没有真机 known-answer、stack/heap watermark、真实服务和
  无线长稳证据，因此仍不能声明生产准入。不接受恒真 verifier、跳过验签或 ESP32 专属公开入口。

- 重新审计 public facade → official composition → onboarding coordinator 的实际依赖，确认当前 WiFi
  QuickStart 接受 `developerEnrollmentToken` 却没有执行自注册，BLE QuickStart 则仍要求永久
  `localAccessKey`；二者均退出正式入口候选，不能靠改名继续扩展。
- 已从 public facade 删除 `WiFiConfig/BleConfig/WiFiBleConfig` 与旧 configure seam；早期“接收 token/SSID
  但只保存 WiFi”的伪入网路径先收敛为诚实 pre-enrolled 探针，现已由真正执行 Claim/Cloud Enrollment 的
  `WifiOnboardingProfile` 替换。ESP32/UNO R4 BLE 旧 composition、release profile 与旧 QuickStart 已撤回；
  底层 BLE adapter 不删除。
- 主线拆成 R4b-0～R4b-7，先做身份/独立持久域，再做 exact wire/coordinator，最后发布 facade；纠正原
R3“发布树无旧引用”的错误状态。该残留随后已由 Ownership Release 替代并在 R4b-0 物理删除收口。
- 新增平台无关 `DeviceInstanceId`：首次启动用 `IRandom` 生成 16 B 非零随机 ID，通过
  `IAtomicBlobStore` 保存固定 28 B `BDN2`/version/reserved/ID/CRC record。已存在记录时完全不调用随机源；
  CRC/长度/版本损坏时硬失败，不自动换一个 physical device。
- `physicalDeviceId` 文本固定为 `dev_` + 32 位 lowercase hex；它不是 secret、MAC、ManualDeviceKey 或
  CloudCredential，因此 PlainFlash 可接受，但必须 crash-atomic 并跨 Network/Application/Ownership reset 保留。

- `EndpointKind` 收敛为 Property=0、Action=1、Event=2；ValueType 新增 Null=9。
- 删除 Components、TypedWidget、WidgetCatalog 及其 HELLO/frame/runtime/golden 路径。
- `0x07/0x08`、feature bit 8、HELLO key 10/11 保留，不在 BBP/2 v2 复用。
- 新增 `Property<T>`、`Action<T>`、`Event<T>` 与 `onWrite/onInvoke/report/emit`。
- PageSchema、WidgetRegistry、layout/style/binding 明确归 App/服务器。
- State snapshot ordinal 仍按完整 Manifest 计数；Action/Event 是无状态洞，不改变稳定 Field ID。
- 三端 reference golden 更新为 29 valid outputs / 6 invalid inputs。
- 旧 `ProductOrchestrator` 曾验证 `IProductLifecycle` 的延迟绑定和 BLE/cloud 原子注册；该实现现已退出
  public/release，只保留 `IProductLifecycle` seam，等待新 coordinator 替换。
- `Client::prepare()` 与 data-plane `begin()` 分离；prepared/offline Property 和 `StateUpdate` 直接提交
  现有 StateStore，Event 离线明确返回 `NotConnected`，未增加 pending map/heap。
- 早期 `WifiCloudLifecycle` 用于验证 pre-enrolled WiFi 的最小生命周期；完整
  `WifiOnboardingCloudLifecycle` 现已同时覆盖 Active fast-path 与首次认领，旧类及专属测试/SizeProbe 已删除，
  不再对 Advanced 用户暴露两套 WiFi lifecycle。
- cloud session 直接修订为 `/2`：删除每次登录重复发送的 `physicalDeviceId` 与
  `HttpCloudSessionProvider` 的 factory identity 依赖；physical asset 绑定仍由 enrollment/服务器
  registry 持有，未发布 `/1` 不兼容。
- 删除 DeviceConfig/Device/BLINKER_DEVICE、BBP/2 HELLO 和 ClaimStart 的设备自报 productId；HELLO
  key 5 保留不复用。productModelId 改为 setup intent/factory registry 的服务器内部元数据。
- 2026-08-17 前历史方案曾让手动/自动 profile 在 Ownership 后分别激活 CloudCredential/ControllerCredential；
  该默认 cloud root 已被 DeviceKey 合同取代。ControllerCredential 继续独立，LocalBLE 仍不要求服务器。
- 新主线明确：联网设备每次启动都以 DeviceKey challenge-HMAC 申请短期 SessionCredential；Key 刷新让
  credentialVersion 前进，旧 Key、旧 session 和当前连接立即失效。Network Reset 只清 WiFi，不轮换 Key。
- App 与服务器仓分别新增 `docs/DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md`，记录当前 Legacy 边界、
  V2 数据模型/API、配网 UX、事务和发布门禁；本批没有修改生产代码。
- 冻结 `ControllerGrant/ControllerMutationReceipt/ControllerCommitRequest` v1：服务器只签 secret digest；
  设备持有 control-window nonce，先 durable mutation 再回 receipt；App 验证 HMAC + Method 2 后幂等 Commit。
  Revoke 明确没有被撤销 secret proof，Commit 不作为设备激活开关。平台无关 coordinator 不依赖无线或 HTTP。
- 冻结独立 `Cloud Credential Enrollment v1`：设备先保存随机 secret/ephemeral private/requestId，再把
  secret 以 pinned server X25519 public key + AES-256-GCM sealed envelope 发送；Grant 由服务器签名，
  Receipt 与 CommitAck 分别以新 secret HMAC，Plain HTTP 下也不会裸传长期根或接受可伪造 2xx。
  136 B `BCE2` 与 116 B Active `BCS2` 分离，GrantAccepted 后掉电可恢复 exact Receipt。
- 将 Noise 内部 HKDF 提取为共用 RFC 5869 `hkdfSha256()`，并让 `INoiseCryptoProvider` 继承通用
  `IX25519AesGcmCryptoProvider`；既有 Noise 三端向量保持完全一致，Cloud contract 不依赖 BLE session。

## 已通过的软件验证

- 当前 MSVC C++11 `/W4 /WX` 与 ASan 全量 31/31、20,000 次 deterministic fuzz 通过；新增 staged lifecycle
  已覆盖 arena 同址、注册前 cloud gate、正常上线、断网不重认领、2 秒 Backoff 重试和永久 Key Fault。UNO
  R4 Core 1.5.3 storage probe 为 68,636/7,732 B（Flash/global RAM），并验证 40 B lifecycle ABI 与新增四个
  Preferences 持久域。Small SizeProbe 在显式常驻新增 1,414 B arena 和 40 B lifecycle 测量数组后为
  52,260/13,472 B；ELF symbol 分别为 `0x586/0x28`。该总 RAM 是全对象测量探针，不是最终 composition
  常驻估算。此前 WiFi onboarding
  完整四 POST、三处断电恢复、Active 重启零 HTTP、Backoff/transaction restart、transport retry、错误
  Ownership CommitAck 不清 journal 与 buffer overlap 覆盖。Ownership exact golden 为 C++/Node 11 项、
  App 7 项；其余三端 golden 无回归，当前 15-package boundary 与 ESP32/UNO R4 Plain/TLS 聚合包确定性通过。
- 删除 public 三 Config 后的同一 WiFi-tag facade 编译通过：ESP32 319,107 B / 22,320 B，UNO R4
  69,880 B / 8,292 B，Wio Terminal 48,820 B Flash（Wio Arduino 输出不提供 global RAM）。这是
  facade/source portability 门禁，不是 WiFi onboarding 功能门禁。
- 重生成并编译完整 official WiFi onboarding package：ESP32 1,017,199 B / 50,632 B，UNO R4
  158,892 B / 13,348 B。两者均实例化 Claim、Cloud Enrollment 与真实验签/封装 provider；包内同时保留
  `WiFiOnboardingQuickStart` 和诚实的 pre-enrolled `WiFiQuickStart`。Arduino CLI 的残留非法 board
  目录提示与第三方 core/library warning 不影响所选目标成功退出码。
- MSVC C++11 host 核心测试；
- 公开 interaction C++11 compile probe；
- C++、App-compatible TypeScript、Server Node exact golden 一致；
- 既有 identity、enrollment、Noise、BLE mode、WiFi credential journal、cloud session、adapter host tests。
- 当前 schema 4 package boundary：1 个 Core 工程归档 + 4 个可归档公开 Product + 4 个只可 staging 的
  internal test profile，以及 13 个内部 Port/26 个精确源文件；Port 不再是
  独立 Arduino 包，Core 包不含 ports/docs/tests，Product 包只包含 manifest 选择的 Port；
- Aily Builder 原 CorePortable（现 AdvancedCoreModel 的历史基线）：ESP32 330,031 B Flash / 24,000 B global RAM；UNO R4 WiFi
  82,780 B / 9,984 B；Wio Terminal 61,764 B / `.data+.bss` 6,664 B。
- 本批 `CompileV2` 小型资源 profile 也已通过；加入 BFB2 后 ESP32 clean Arduino CLI 为 322,371 B Flash / 21,672 B
  global RAM；UNO R4 WiFi 73,788 B / 7,644 B；Wio Terminal 52,780 B Flash（该次 Aily 输出
  未打印 global RAM，不能据此填 0）。这仍是 Core/API 编译门禁，不是最终 facade 或无线 product 峰值。
- R4b-2 Method 2 合入后的 `CompileV2` 六平台源码门禁通过：ESP32 318,415/21,672 B、UNO R4
  69,296/7,664 B、Wio Terminal 48,236 B Flash；ESP8266 为 251,108 B IROM + 26,399 B IRAM /
  29,540 B global RAM，RP2040 为 72,448/10,496 B，nRF52840 为 46,808/4,020 B。前三项由
  Aily Builder 完成，后三项由官方 Arduino CLI 完成；Aily 在 ESP8266 core 3.1.2 生成了不存在的
  `core/build.opt` 响应文件路径，因此不能把该构建器失败算作库编译失败。
- Ownership Claim v1 后重新实编 `CompileV2`：ESP32 319,459/21,672 B、UNO R4 69,296/7,664 B、
  Wio Terminal 48,236 B Flash，三者均取得独立 Arduino matrix 成功退出码；ESP32/UNO CLI 扫描到的
  非标准残留 core 目录提示不影响所选合法 core 版本与目标退出码。
- Cloud Credential Enrollment v1 后重跑：ESP32 `CompileV2` 320,967/21,672 B、UNO R4
  69,296/7,664 B、Wio Terminal 48,236 B；ESP32 Noise adapter 356,651/20,800 B。UNO R4 Small
  SizeProbe 为 52,148/11,696 B，新增 `CloudEnrollmentRecord/Store/Sealer/Coordinator` 的 32 位 ELF
  实测为 128/8/8/32 B。全量三端新增 C++ 15、App 5、Node 19 项 Cloud exact 值，其中 Node 使用真实
  X25519/AES-256-GCM；这些仍是软件/编译证据，不是真机 crypto 性能结论。
- 本批 `CompileFacade` fake lifecycle 三 tag 矩阵均通过。WiFi：ESP32 318,835 B / 22,336 B、
  UNO R4 70,032 B / 8,308 B、Wio 49,012 B Flash；BLE：ESP32 318,831 B / 22,336 B、
  UNO R4 70,016 B / 8,308 B、Wio 49,012 B；WiFiBLE：ESP32 318,831 B / 22,336 B、
  UNO R4 70,024 B / 8,308 B、Wio 49,012 B。普通 `Blinker` 对象在 32 位 ABI 上不超过 8 B；
  Property/Action callback view 采用单指针 union + mode，不建立 registry/heap，门禁不超过 16 B。
- UNO R4 active Small SizeProbe 本批为 52,244/12,020 B（Flash/static RAM），包含 48 B
  `ControllerControlCoordinator`、20 B Claim coordinator、36/8 B Claim record/store、36 B Key exchange、
  60 B `WifiOnboardingFlow` 与 20 B release cleanup 门禁。本轮相对 52,228/11,880 B 的 +140 B RAM 中，
  80 B 来自修复此前未实际引用而可能被链接器裁掉的 Claim record/store/Key exchange probe，新增 flow 为 60 B。
  ESP32 Core 为 319,511/21,672 B，Wio Terminal Core 为 48,236 B Flash。Standard/Large 上批为 52,148/13,772 B、
  52,148/22,964 B，本批未重复。`Product` 12 B，Property/Action/Event 16/16/8 B，
  CloudCredential/Store 108/8 B，OwnershipRecord/Store 104/8 B，ControllerCredential/Store 64/8 B。旧 ProductOrchestrator 等
  Enrollment/1 对象已从 probe 移除。
- 独立 Arduino fixtures 也已清除旧 `IOperationalCredentialStore`：ESP32 storage 的历史结果为
  314,699/21,360 B；UNO R4 十域 storage 本批为 68,636/7,732 B。ESP32/UNO R4 cloud-session 历史结果为 586,672/28,832 B、85,736/11,304 B
  （Flash/static RAM）。
- 历史 `WifiCloudLifecycle` 启停/上线/掉线行为已由当前 staged lifecycle 的 Active fast-path 覆盖；旧类已删除。
- ESP32/UNO R4 official WiFi 共享 composition 已通过完整 onboarding 聚合包与 Plain QuickStart：ESP32
  1,017,199/50,632 B，UNO R4 158,892/13,348 B。TLS 组合也已实编：ESP32
  1,103,471/51,224 B，UNO R4 162,068/13,380 B。UNO R4 storage
  adapter 使用 connectivity-module Preferences 双副本 journal，不占用 RA4M1 EEPROM/Data Flash；22 项
  Windows ASan host/fuzz-smoke 与 5,000 deterministic fuzz 通过。BFB2 C++/Python exact、模拟 USB Serial
  transcript 通过，UNO R4 工厂草图为 66,700/7,924 B（Flash/static RAM）。
- 2026-09-02 新增可复用的 `tests/internal/esp32_wifi_ble_cycle_gate.cjs`，严格执行“先离开控制页进入重置
  引导 -> network-only reset -> Security1/GetInfo -> Wi-Fi -> Cloud -> Direct -> Property Command -> 设备资源
  探针”。App 路由页现在显式把 exact control-route activity 传给 V2 子组件；离开页面会取消 timer、telemetry
  和该 logical device 的在途 BLE open，即使业务 transport 已先回退 Cloud。修正前全新 App 进程首轮 Direct
  接管固定退化约 49 s；修正后两轮连续实物门禁均在进入设备页后约 4.27 s 达到 Direct ready，且命令 ACK、
  Cloud 同时在线、13 个 task 采样均通过。两轮最小 free heap 为 135,124 B、最小 largest block 为
  104,436 B；`ipc0` 仍只有 76 B，所以这是循环门禁基础设施与竞态修复，不是 100 次/RF/长稳发布结论。
- 2026-09-09 S3a-1c完成网关南向所有权拆分：新增每child唯一`BasicGatewayChildExecutor`，Cloud route
  bridge仅保留Cloud准入/需求/投影/correlation；child回调、南向BBP/2序号、单在途/重放、管理事务与原512 B
  workspace不再位于Cloud adapter。Host/ASan 53/53、双owner序号/回调/背压、架构与单库清单通过；ESP32-S3
  Arduino3.3.11 EdgeHub编译为1,450,448 B Flash/75,228 B globals。同一SDK/config上活动context
  7,512→7,552 B（+40 B/child），executor为580 B且已含迁入的512 B workspace，两个槽整机BSS+88 B。
  未实现LAN transport/auth/discovery或S4去周期，
  未烧录、部署、改凭据或运行100轮。

上述数据是当前 Core/API compile gate，不是 WiFi/BLE product peak。真机状态仍以以后实测与 CI
为准，不沿用旧文档中历史成功数字。

## 当前发布阻断

1. R4b-0 已完成：旧 Enrollment/1、OperationalCredential、ClaimProof/LocalProvisioning、Reset 与
   ProductOrchestrator 源码/测试/golden 已删除；R-ARCH-5 又删除了无消费者的独立 P-256 developer identity，
   通用 BLE/Noise seam 保留；
2. R4b-1 软件基线已完成：DeviceInstanceId、CloudCredential、OwnershipRecord、当前两槽
   ControllerCredential table 与 Network journal 均为独立持久域；
3. R4b-2～R4b-4d control-plane/LocalSetup、R4b-5A lifecycle、R4b-5B/5C 两平台 internal composition、
   R4b-5D fault gate、R4b-6A/6B LocalBLE pairing/application/shared lifecycle 与 R4b-6C 四个双平台 pure BLE
   package/resource 已完成软件门禁；R4b-7 facade 仍不得在真机门禁前发布；
4. 生产 App/服务器完成 BBP/2 codec/session/PageSchema 接入；
5. 完成 R4a 剩余门禁：Agent 生成评测；三 profile fixture 已通过，普通 API 已不暴露
   `Device/Product/Result` 或手工 transport/buffer 组装；
6. 真实 broker、凭据、ESP32 WiFi+BLE 核心并发链、3.3.11 系统任务栈准入、100 次模式循环与 active renewal
   100 轮资源/重复性门禁已通过；仍需 RF 共存，以及四种 BLE product 的剩余断线/长稳门禁；
7. UNO R4 Preferences journal/USB installer 的真机掉电、端口重枚举、固件兼容和首次登录；其 security admission 仍是受限 PlainFlash，
   不用 MCU 名称或编译结果假装具备安全能力；
8. ESP32/UNO R4 internal WiFi+BLE 与 Platform BLE/LocalBLE 对象图均已完成；继续禁止以静态 compile 或
   伪安全状态冒充公开可用。HC-05/Bluetooth Classic SPP 暂不加入。

History、任意设备下发 JS、WidgetCatalog、设备生成页面和 ArduinoMqttClient 强制适配都不在当前主线。
