# Device V2：核心功能剩余执行计划

**当前入口（2026-11-06）**：后续顺序改取[有限支持交付与核心稳定完善方案](DEVICE_V2_CORE_DELIVERY_AND_STABILITY_EXECUTION_PLAN.md)。主要开发仍由我们负责，未交接；先DS-1失败显式恢复、再DS-2匹配交付，不默认续跑SoftAP深诊断。以下旧“当前／下一步／同事分工”仅作历史范围与证据参考，不覆盖新顺序及归属。

用户最新测试清单的后续顺序取[测试问题对照与基础能力渐进计划](DEVICE_V2_TEST_REPORT_RECONCILIATION_AND_CORE_PLAN.md)，默认当前内测基线，按接入差异→基础控制／状态→当前环境任务组合→自动化差异→App体验推进；不因旧报告把已过项重新排队。下段及本文原NEXT记录均保留为历史。

2026-10-31 现行顺序已转至[标准接入交付与自动化共存收口§11](DEVICE_V2_STANDARD_DELIVERY_AND_RULE_COEXISTENCE_PLAN.md#11-10收口后的最小修正与源码交付索引2026-10-31)：§10 两设备有限短验已收口，本批仅派生单位修正及必要交付说明。以下 NEXT-1／NEXT-2 及“当前／下一步”保留为 10-29 当时结论，不另启动配网、自动化或后置矩阵；发行 CI 仍 HOLD。

更新：2026-10-29。NEXT-1 软件修正及 42 项有限回归完成，结果见 §9；NEXT-2 匹配内测 App 安装和两页只读短验已完成，结果见 §10。当前两项队列结束，不自动启动后置阶段。

## 1. 目标与现行入口

目标是让已实现的核心功能在已声明范围内稳定可用，不再把后置扩展、发行工程或自然样本欠证混成核心功能待实现。
本文件管理当时 NEXT-1／NEXT-2 的顺序；[主线进度](DEVICE_V2_NEXT_EXECUTION_PLAN.md)保留现行入口及历史，专题文档保留具体支持合同和原始验收结论。
旧批次的“下一步”不自动进入当前队列；冲突时按上方较新的现行入口执行，不覆盖历史失败记录。

**现行判断：核心功能已在受限内测范围内大体收口，但不是全部平台、全部模式默认开放或正式发行完成。**
H1／H2 读取失败后返回前台、原刷新按钮仍被禁用的问题已按 NEXT-1 修正，并由 NEXT-2 匹配 App 真机短差异确认。
当前没有自动续跑项；CI／发行仍由用户暂缓。后续只按实际新缺陷或用户明确选定的独立阶段安排，不继续叠加已通过验证。

## 2. 已完成基线，不重新排队

下表是既有证据摘要，不表示本批重核了运行现场。源码实现、编译通过、部署启用、有限实物验收与正式发行是不同层级。

| 范围 | 已完成及冻结边界 |
| --- | --- |
| 普通接入与控制 | ESP32／R4／Wio 普通 WiFi、S3／R4 BLE、S3 WiFi+BLE 的既有接入、绑定／认领、控制 ACK／新鲜 State、名称及有限恢复证据保留；不扩成任意板型或 R4／Wio 双模式支持 |
| 业务授时与 Sketch Timer | 既有可选授时、周期／延时／取消与通信共存已实现；Sketch Timer 按合同不跨重启持久化，不据此重建另一套任务引擎 |
| 持久任务 | ESP32 指定设备四槽 TLS 已验；R4 匹配维护板型＋固定图＋逐设备准入的四槽 Plain 已有限收口。普通公开 Sketch 默认仍为 0 槽；不扩大为全平台开放，Wio 正式准入仍暂缓 |
| 最小自动化 | M1～M5／N5 及云端两设备闭环已完成，含命名、编辑、启停、退出 App 后执行、真实 ACK／State、查询及删除；不是复杂规则、多动作或任意类型支持 |
| 实时数据 | Cloud／BLE／内部 LAN 的既有来源、实际频率、新鲜度、有界曲线及离页／后台释放、恢复与租约兜底证据冻结；不把页面缺陷修复变成新一轮采样／BLE矩阵 |
| H1／H2a 历史 | H1 状态变化、基线及去重；H2a 在线周期选样、同值新样本、重连新段／缺口、停用及有界查询已有限通过。下述待修只涉及两页手动读取恢复，不撤销服务端已验链路 |
| 位置与地图 | 手动安装位置、地图交互已限定验收；Android 系统 NETWORK／GNSS 统一取点已实现，当前手机的系统网络来源已验，独立 GNSS 仍未验。统一设备输入、私有 Cloud 租约／latest 与 owner 按需 IP 城市参考已接入；IP 不覆盖安装位置，位置不绕入普通分享／历史 |
| D7c 只读活动 | 复用权威账本的 ACK／UNKNOWN 语义、时间、权限、有界分页和失败后手动恢复已有实现及有限证据；自然 UNKNOWN／多页 UI 欠证不靠造记录补齐 |
| 库与 Builder 工程 | aily-builder 扫描／缓存及代表目标完整链接已有证据；统一 `Blinker.location(...)` 输入与隔离安装／原 SDK 完整链接已完成。用户自接 GPS 解析器，不以模块型号或卫星精度阻塞 SDK 接口交付；正式发行仍另有 HOLD |

基线索引：[核心稳定](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md)、[任务／Timer §11.58](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md)、[最小自动化](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md)、[云端接入 D1～D4](DEVICE_V2_CLOUD_BETA_H2_DEPLOYMENT_PLAN.md)、[实时与历史](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md)、[H2a](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md)、[地图／手机定位](DEVICE_V2_MAP_SERVICE_ADAPTER_IMPLEMENTATION_PLAN.md)、[统一设备定位](DEVICE_V2_DEVICE_LOCATION_IMPLEMENTATION_PLAN.md)。

## 3. 当前最小顺序

| 编号 | 最小范围 | 进入／完成条件 | 本批状态 |
| --- | --- | --- | --- |
| NEXT-1 | 修正 H1／H2 前台可见性通知与按钮恢复，补真实模板回归 | 先复现，后修正；有限回归证明无自动重试、有效条件下按钮可用、一次手动读取和原保护保持 | 软件完成，42/42；后续实物见 NEXT-2 |
| NEXT-2 | 用户选择后，一次匹配 App 的两页只读短差异 | NEXT-1 通过；沿用现有服务／设备，核真实按钮和 HTTP；完成即结束当前队列 | 已完成，见 §10；匹配 beta 保留 |

两项结束后再按真实缺陷或用户选定的产品范围安排下一阶段；不自动转向 CI、Wio、LAN／网关、扩槽或性能长循环。

## 4. NEXT-1：两页手动恢复的软件修正

### 4.1 已证事实与待证机制

以[活动方案 §11](DEVICE_V2_DEVICE_ACTIVITY_EXECUTION_PLAN.md#11-h1h2相邻读取恢复的匹配app短验2026-10-27)为问题记录：

- 匹配 App 已构建并安装；H1／H2 真实读取失败后，后台再返回 3 秒均没有新增 GET，此保护已通过。
- 两页原刷新按钮仍保持后台的 disabled 状态，未能发出手动恢复 GET；因此手动恢复未通过。
- 原 17 项页面类测试没有覆盖真实 DOM 按钮启用，不能用它们替代新模板回归。
- 实施前源码提示 `canQuery` 读取普通 `visibility.active.value`，错误门槛跳过 refresh 后可能缺少响应式渲染通知；该判断先作为候选，后由 §9 的真实模板红绿差异确认，不把软件证据扩大为新真机结果。

先读取当前代码及本地修改，若已有同事修正则复核差异，不重复覆盖。项目使用 Zone／现有 OnPush 和 signals，不改全局变更检测模式。

### 4.2 最小实现范围

复用 D7c 已有的前台 signal 投影及 `AppVisibilityService`；让两页可见性回调先更新渲染依赖，再决定是否跳过自动读取。
`canQuery` 依赖可通知模板的前台状态；不新增轮询器、可见性服务或执行链，不靠强制 tick、DOM 改写或全局 eager 变更掩盖问题。

主要消费者：

- H1：[device-storage 页面](../../blinker-app/src/app/sections/device/device-storage/device-storage.page.ts)、同目录 HTML／spec。
- H2：[device-trends 页面](../../blinker-app/src/app/sections/device/device-trends/device-trends.page.ts)、同目录 HTML／page.spec 及已有 [render.spec](../../blinker-app/src/app/sections/device/device-trends/device-trends.render.spec.ts)。
- 参照：[D7c 组件](../../blinker-app/src/app/sections/device/device-log/device-log.component.ts)、[共享可见性服务](../../blinker-app/src/app/core/services/app-visibility.service.ts)。共享服务无实际缺陷则不改。

保持以下合同：

1. 失败后回前台不自动查询、不重试、不写入；仍由用户点击原按钮恢复。
2. 只有账号／设备／读取权限有效、页面前台且无在途请求时才允许按钮操作；不强行放开失效身份或无权限设备。
3. 成功后的正常前台读取、后台／离页清理及旧请求取消保持；不让取消后的回包污染新页面。
4. H2 读取失败与保存 UNKNOWN 分开；UNKNOWN 只读协调、旧 revision／CAS 保护保持，不自动重存策略或重放请求。
5. 不清账号、授权、恢复意图或完整请求日志；不改归档、任务、控制、设备固件及服务配置。

### 4.3 有限回归与停止条件

先用两页真实模板／原生按钮绑定建立失败测试，覆盖“失败→后台禁用→前台可用且查询数不增→点击一次恢复”。
H1 如缺渲染用例，只在同目录补最小模板测试；H2 复用现有 render fixture，不另造页面实现。
测试可按现有 harness 驱动正常生命周期，但不能只断言类属性、直接调用 refresh 或人为额外 tick 才让本应自动更新的按钮变可用。

只运行新增用例与受影响的错误门槛、正常成功恢复、取消、H2 UNKNOWN／CAS 回归；沿用原锁及工具链，不升级 SDK／依赖或启动全量矩阵。
完成记录必须区分红灯复现、修正后模板／请求次数、未执行范围；软件通过不写成真机通过。
本步骤结束即交付最小差异，由用户选择 NEXT-2；暂不部署或提交推送。

## 5. NEXT-2：匹配 App 的一次只读短差异

本节为 NEXT-2 的限定验收合同；用户已选择执行，实际结果与观察限制取 §10，不将下列预定序列本身当作通过证明。

### 5.1 前置与范围

NEXT-1 通过后，先保全现有账号、授权和全部请求／恢复意图及日志；不做整机／全 Flash 等无关备份，不卸载清数据。
仅在现有安装与修正源码不匹配时按原脚本构建、同包同签名覆盖安装一次；复用匹配制品，不另开重复队列。
沿用既有 Gateway／Broker、账号和 owner 设备；不刷板、创建设备、刷新 Key、改变 TaskStore／scope、启动任务或启停归档。
若出现登录失效或设备不可用，记录前置停点，不自动发验证码、切账号、创建样本或扩大测试范围。

### 5.2 唯一序列与判定

每页仅一次：真实读取失败→HOME／后台→解除客户端阻断→返回前台观察 3 秒→点击一次原刷新按钮。
临时阻断仅针对现有设备的精确历史 GET；H2 先本地核对规则，区分 `periodic-history` 与 `periodic-history-policy`，不得重复使用会误伤 policy 的裸前缀规则。
这是客户端网络失败，不声称服务器返回了 401／503，也不制造活动、历史或 UNKNOWN 记录。

| 检查点 | 必须证据 |
| --- | --- |
| 后台／返回 | 真实 inactive／hidden → active／visible；后台释放页面资源，返回不新增该页查询；有效前置下原按钮恢复可用 |
| 手动恢复 | 点击原按钮一次产生一次查询组；H1 一次历史 GET，H2 沿既有 history／policy 读取组，不误写为 H2 只有一笔 HTTP |
| 结果 | 真实 200／no-store、合法 DTO／来源及时间；允许空数据，不要求为曲线或分页造样本 |
| 保护与收尾 | 无 HTTP 写入或 UNKNOWN 重放；返回首页并撤销阻断、观察器及本批转发；原账号、授权、全部 journal／请求日志保留 |

不重复已过控制、采样／归档、地图／定位、BLE、断电或自动化矩阵。新首停保留原证据，不靠直接调用组件方法或强制修改 DOM“验过”。
仅记录两页新增差异和实际未验边界；若发现新问题，限定修正后再决定必要验证，不自动重跑全部流程。

## 6. 暂缓、可选与协作事项

以下不是当前串行队列，也不是 NEXT-1／2 的前置。用户另选范围后才能启动。

| 编号 | 状态与真实剩余内容 | 再启动边界 |
| --- | --- | --- |
| PAUSE-CI | 用户已暂缓；[发布方案 §14.9](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#149-现行结果与发行决定源码已交付私有-oracle-授权待补hold)保留源码已交付、私有服务 oracle 缺少只读访问、发行 HOLD／无匹配可发行 ZIP 的事实 | 用户恢复发行后，才处理专用只读授权、固定 checkout、真实 package 与同提交 CI；不复制私有源码、伪造 oracle、放宽门禁或重跑已知必败队列 |
| PAUSE-WIO | Wio 持久任务有维护／存储与有限链路证据，正式准入仍暂缓；R4 受限 Plain 已收口，R4 TLS 属另一组合 | 明确 Wio 实物、匹配图／profile、存储独占和有限故障支持合同后独立续接；不泛化 R4 证据或重新无边界审计 |
| PAUSE-ERROR8 | Wio 实时续租 InternalError 8 根因未确认；未复现不等于修复 | 有具体新线索和一次有界关联范围才续定位；不重复 180 秒采样、不重放旧 UNKNOWN |
| OPT-LAN | [LAN](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md)已有 ESP32 mDNS／鉴权 WebSocket／页面控制证据；默认未面向全部 WiFi 图开放 | 另定平台／模式与启用合同，复用功能注入，不把已验证内部链路写成全平台稳定可用 |
| OPT-GW | [网关 GATT](DEVICE_V2_EDGE_BLE_GATEWAY_GATT_IMPLEMENTATION_PLAN.md)已有双子设备及 A 关闭不影响 B、满载 Busy 等有限证据；[共享连接](DEVICE_V2_GATEWAY_SHARED_CONNECTION_IMPLEMENTATION_PLAN.md)持续连接／无缝切换未全部收口 | 明确多子设备场景与有限体验目标；复用账户常驻 MQTT 及原南北向模块，不承诺全场景无缝 SLA 或新增网关协议 |
| OPT-SHARE | Cloud 双账号只读／控制／撤权有证据，不等于所有 BLE Direct 分享权限均已产品化；精确位置仍 owner-only | 另定成员 BLE 权限／撤权合同，不下发 owner Admin 凭据替代细分权限，不借通用分享绕过位置隐私 |
| OPT-BLE | [分段优化](DEVICE_V2_BLE_FIRST_CONNECT_SEGMENT_EXECUTION_PLAN.md)已有协商／20 B 回退、Manifest 缓存和有限首连证据；可选窗口默认关闭 | 仅按真实新瓶颈安排有限测量；不承诺 iOS／Wio／任意 SDK 相同时延，不重复长循环 |
| COORD-1 | 自定义控制页完整 V2 与 Blockly V2 积木由同事负责，最终适配状态待确认；Builder C++ 编译不等于积木已完成 | 用户选择协作后再核固定提交、支持能力和匹配验收；不擅自重做同事功能或把“待确认”写成肯定未实现 |

地图仍为有预算／个人认证约束的受限服务，既有 4500 次／UTC 日不扩大为无限公开商用许可或企业认证额度。
IP 城市参考入口已完成，不再列作“IP 定位完全未实现”；手机 IP 自动取点、国内定位 SDK 及自动覆盖安装位置不在本批范围。

## 7. 后置扩展与自然覆盖欠证

- 后置扩展：H2b 真正采样 UTC／时间质量及设备端持久提交 ACK，H3 离线历史／Bulk、复杂自动化、OTA、新网关协议、轨迹／地理围栏和位置分享。
- 自然覆盖欠证：活动 UNKNOWN／多页 UI、真实在途取消、双账号精确位置撤权、独立 GNSS／跨机型等；没有样本就注明未验，不人为造记录或循环补齐。
- 用户 GPS／其他外设的驱动、解析器与源真实性由用户负责；SDK 提供统一输入及边界校验，不新开硬件适配门禁。
- 旧本机归档／通知意图等遗留状态仅在原实例与原日志可用时只读协调；新云端通过不代表已迁移、停用或解决旧 UNKNOWN，不搬库或重放。

## 8. 执行与收口规则

只做当前选定编号；实现复用原链路，优先修实际消费者，清理与本次差异直接相关的死代码，不顺手重构其他模块。
原授权、CAS、UNKNOWN、TaskStore／云 scope 与完整请求日志保持；不扩槽、不升级 SDK、不发真实验证码、不自动重试、不重复已通过项。
交付记录只写本批实际完成与首停，禁止用源码提交、历史编译或模拟观测替代部署／实物／同事签收。

初始规划批仅新增本文件、更新主线入口及发布分支的暂缓路由；当时未修改产品源码或运行回归。NEXT-1 软件差异见 §9，NEXT-2 匹配 App 安装／短验见 §10；没有服务部署、刷板、CI 授权或 Git 提交推送。
后续 NEXT-1／2 的结果在本文件对应状态处简要更新并链接专题证据，不再复制整段历史进度生成另一套待办。

## 9. NEXT-1 软件修正与有限回归（2026-10-29，完成）

### 9.1 先红灯，再修正实际消费者

复用两页既有 render fixture 和生产 HTML，仅添加 7 项模板用例。初次只运行这 7 项：
H1 读取失败、H2 history 失败、H2 policy 失败共 3 项均在返回前台的原按钮 `disabled === true` 处红灯；账号变化／设备停用的 4 项禁用反例通过，12 项原模板用例未运行。
返回本身没有新增查询，错误提示保留；问题不是错误门槛失效，而是门槛跳过 refresh 时缺少模板渲染通知。

H1 使用原 `DeviceV2HistoryService` 拒绝无效 DTO，H2 使用读取适配器拒绝；这是实际模板的软件复现，不称真实网络、服务器或本轮实物故障注入。
未 override 生产模板，新用例仅用现有自动渲染与 `whenStable()`，没有增加强制 `detectChanges`／tick、直接调用 refresh 或修改 DOM 的恢复捷径。

两页产品修正各限三处：新增私有 `foreground` signal；原可见性订阅首先更新它；`canQuery` 改为读取该 signal。
订阅的同步初值仍来自现有 `BehaviorSubject`，没有新生命周期、轮询器、共享服务修改或模板／样式变化。
H1 原错误门槛、H2 的 `readFailed`／`needsConfirmation`、保存／CAS／取消及迟到结果隔离代码均保持原样。
同时移除 H2 不再读取的构造器属性，仅保留订阅所需注入参数；不为本问题改动其他页面或执行链。

### 9.2 有限绿灯及边界

仅运行两份 render spec 与两份 page spec 的相关用例：**4 文件、42 项通过，39 项无关用例跳过**，未跑完整产品矩阵。
清理 H2 无用构造器属性后只重核新增 7 项模板用例，7/7 通过、12 项原模板用例跳过；不累加为 49 项独立测试，不重复原保护矩阵。

| 范围 | 软件结果 |
| --- | --- |
| 7 项新增真实模板 | 后台按钮禁用；有效账号／设备返回前台后原按钮可用、错误仍在、查询数不增；点击一次恢复 H1 一次读取、H2 一组 history＋policy 两次读取，H2 保存为 0；账号变化／设备停用仍禁用 |
| H1 相关既有保护 | 失败保持到手动刷新、重新进入与账号隔离、后台正常恢复、离页／销毁／账号／停用后的迟到结果丢弃通过 |
| H2 相关既有保护 | 读取失败无自动重试、失败分页清理、UNKNOWN 协调失败仍禁写、只读恢复新 revision 后显式保存的 CAS、中断保存不重发、后台取消及固定查询窗口保持通过 |
| 代码审阅 | 独立只读审阅四文件差异无阻断发现；没有新增强制渲染、放宽权限、自动读取或写入分支 |

使用当前已安装 Node 24.22.1 和本地 Angular CLI／Vitest 4.1.11；没有安装或升级依赖，`package.json`／锁文件字节保持，未通过全局 pnpm 改版本。
有限回归命令如下；初次红灯只包含两份 render spec，过滤 `manual recovery template`：

```powershell
node node_modules/@angular/cli/bin/ng.js test app --watch=false --include=src/app/sections/device/device-storage/device-storage.render.spec.ts --include=src/app/sections/device/device-trends/device-trends.render.spec.ts --include=src/app/sections/device/device-storage/device-storage.page.spec.ts --include=src/app/sections/device/device-trends/device-trends.page.spec.ts --filter='manual recovery template|manual read recovery|clears the actual rendered records on native background|shows share reads but no write controls|releases on native background|clears sensitive samples/settings and fences late results|blocks re-save after|blocks writes through retained old detail|aborts an uncertain save on background|keeps the exact applied window|fences a late old window result|aborts and discards a late response' --progress=false
```

没有运行 APK／发布构建、安装、部署、设备操作、真实账号／HTTP 请求或验证码；原授权、UNKNOWN、完整请求日志未操作。
App 原 19 个无关 dirty／未跟踪文件逐字节保持，原 HEAD／分支／索引及包／锁保持；库侧已有无关文档／partition.csv 保留。未提交推送。
**本节截止时仅 NEXT-1 软件收口；其后用户已选择 NEXT-2，结果见 §10。** [原 §11 真机手动恢复失败记录](DEVICE_V2_DEVICE_ACTIVITY_EXECUTION_PLAN.md#11-h1h2相邻读取恢复的匹配app短验2026-10-27)继续保留，不改写原失败。

## 10. NEXT-2 匹配 App 安装与两页只读短验（2026-10-29，完成）

### 10.1 保全、唯一构建与安装

手机原内测 App 已在线；先通过既有只读保全方式完整保存原 session／全部 SecureStorage、加密 XML、localStorage、两份 IndexedDB intent store 及原生日志。
保全文件和全过程原始请求／响应、截图仅位于 App 忽略目录 `build/next2-history-recovery-20261029`，不把账号、token、Key 或完整业务日志写入 Git。
没有整机／Flash 备份、卸载清数据、登录／验证码、账号切换或授权修改。

原安装包未包含 NEXT-1 修正，仅沿用现成脚本与工具链完成一次 Web→Capacitor copy→离线 APK 队列。
实际 Node 22.22.3／pnpm 11.19.0／esbuild 0.28.2、JDK 21.0.12.1、原 Android SDK 与 Gradle 缓存保持；单 worker，不升级或安装依赖，不做无必要的原生 sync。
三阶段分别约 35.753／4.702／91.384 秒成功，2130 项冻结源码及包／锁、HEAD／分支／索引、dirty 状态和原生注册表前后保持；没有重跑 §9 软件回归。

| 匹配核对 | 结果 |
| --- | --- |
| 内测包 | `iot.diandeng.tech.beta`；76,518,128 B；SHA256 `f839a8fce8f82c82edc5e26a25dc69aa16285659572c8bc8aa1dae49df19eac2` |
| 签名及安全配置 | 与既有 beta signer 相同；HTTPS、禁明文、logging none，原 19 项原生注册保持，未新增后台定位权限 |
| 实际代码来源 | 1155 份打包 Web 资产与本批构建字节一致；两页唯一 lazy chunk 的前台 signal、先通知再判断门槛及原错误／UNKNOWN 保护共 12 条检查通过 |
| 安装 | 同包同签名 `install -r` 一次成功；安装后／最终手机 APK 摘要均匹配上列；原系统定位仅核仍被打包，不重复定位验收 |

### 10.2 唯一只读序列与真实结果

沿用既有云端服务和原 COM11 owner 设备身份；工具不发业务 HTTP，只通过生产路由进入页面、点击原 `button.refresh-action` 一次。
每页只做一次失败→HOME→后台解除阻断→返回 3 秒→手动刷新→回首页；没有组件 refresh 直调、强制改 DOM、伪造响应或故障后自动补查。
只阻断该设备历史 URL 的查询分隔符形式 `endpoint + '?*'`，预先核对不匹配 H2 policy；没有再次使用裸 endpoint 前缀。

| 范围 | H1 状态历史 | H2 在线趋势 |
| --- | --- | --- |
| 首次失败 | 1 次历史 GET 被客户端真实阻断，无服务器 HTTP 状态；安全错误可见 | history GET 被阻断；并行 policy GET 由原请求组自然取消，不是被阻断 |
| 后台 | 原生 inactive 与 DOM hidden 均有证据；按钮禁用，行数据为空 | 同样观察原生／DOM 后台；按钮禁用，行／策略／草稿为空 |
| 返回前台 3 秒 | 原生 active／DOM visible；原按钮恢复可用、错误保留；历史 GET 仍 1 | 原按钮恢复可用、错误保留；history／policy GET 均仍各 1，不自动查询 |
| 原按钮点击一次 | 仅新增 1 次历史 GET，真实 200／private,no-store，约 390 ms；合法 broker-state／serverAcceptedAt，显示既有 50 行 | 仅新增 1 组 history＋policy 两次 GET，均真实 200／private,no-store，约 380／283 ms；合法 broker-telemetry／serverReceivedAt，策略恢复且 history 为空，未造点 |
| 退出页面 | 已回首页，原页不再呈现，目标读取／待取 body 均 0 | 同样回首页并清理，未保存或启停归档 |

上述“不自动查询”仅指两页目标读取；App 原正常目录读取不误算历史自动重试。
期间观察到的业务写方法、地图／位置请求均为 0；没有任务、自动化、Key、TaskStore／scope 操作或设备刷写，也没有重建／配置服务。
结果为本机这一次有限真机差异，不宣传网络时延 SLA、全部权限／故障矩阵或所有手机显示适配。

**保留观察限制：** 初次取消的 H2 policy GET 所关联的 OPTIONS，在本次 CDP 观察中没有终态，原始记录保留为 pending 1；不能称所有 CORS OPTIONS 生命周期完整。
目标 GET 均有失败／取消／成功的实际终态，成功响应 body 均完整，目标 GET／body 待取均 0、掉项 0；这项非 GET 观察限制不替代或否定两页按钮／无重试／手动读取结果，不为补它重复测试。

### 10.3 收尾与执行决定

两页的精确阻断、原生／DOM 监听、CDP binding／Network 观察与本批转发均已回收，ADB forward／reverse 恢复原空基线；最终前台首页。
安装后与最终原账号、完整 session、SecureStorage 明文快照摘要与原加密 XML 字节一致，两份完整请求意图库的键／值与摘要保持；原 UNKNOWN 及完整原生日志没有删除、回滚或重放。
完整原始请求及失败元数据、响应、两页截图和上述 OPTIONS 未终态均私有保留；匹配新版 beta 留在手机。
独立只读证据终审无阻断发现，App 2130 项冻结源码／无关修改、原包／锁／HEAD／索引终核保持。无服务端、设备端源码或运行配置变更，未暂存、提交或推送。

**NEXT-1／NEXT-2 至此有限收口，本队列结束。** CI／发行、Wio／Error 8 及扩展仍按 §6～7 暂缓／另选，不自动继续构建、采样或补自然样本。
