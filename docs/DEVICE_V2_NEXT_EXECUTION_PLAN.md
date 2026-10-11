# Device V2：核心稳定版进度与后续执行计划

> 当前H1最小状态历史按限定范围收口，最新结果取[实时与历史方案§9.6](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md#96-h1真实分享只读与撤权收口2026-10-11)：自有WiFi设备历史及真实viewer读取、精确撤权后的404/页面清空已通过；多位记录ID排序已有限修正，仅更新目标Broker1并保留本机测试网络、账号、授权和日志。其他Shard与全部历史/分享通信门禁不据此关闭。既有L1、L2b/L2c限定短验收及COM3恢复保持关闭，普通LAN开关不变。必要源码/最短说明与差异安装编译已收口，取易用性计划§10.7；**顺序4按用户要求改为三仓Git源码及五插件固定远端提交，由同事自行编译、安装和短验收**，取[§10.9](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#109-三仓git源码交付与独立验收2026-10-11)和[Git验收手册](DEVICE_V2_INDEPENDENT_ACCEPTANCE.md)。§10.8本地交接包只留历史摘要，不再交付或保留；独立本人签收仍待完成，不把本批软件门禁当接收者实物通过。R4新版实时例程编译已通过，实物仍未复测；不自动开启H2、设备离线存储、Bulk或服务器长期维护。

历史基线：2026-10-10（第86批M5本机产品验收）。**最小自动化M1～M5按限定范围收口：命名／编辑／统一入口的软件实现和新版App、Gateway迁移050、两普通WiFi设备真机短闭环均通过。** 执行取[产品方案§8](../../blinker-app/docs/APP_DEVICE_V2_MINIMAL_AUTOMATION_PRODUCT_PLAN.md#8-m5本机新版本验收)及§8.86；第82～85原证据保留。复用执行链路，不扩复杂规则、网关、100轮或整组维护；本机验收与独立交接／正式发布分开。

最新执行（2026-10-11）：**M1～M5及N1～N5各自限定本机短验收已收口；后续设备通用整数step源码/软件对齐及COM3短差异验收也已完成。** N5两设备uint32 50→0自动化及恢复取[§9.6](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#96-n5-本机数值短验收2026-10-11收口)，共享EndpointValue整数余数及回归取[§9.7](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#97-设备通用整数-step-有限对齐)。最新[§9.8](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#98-设备与app整数step短差异验收2026-10-11收口)仅临时刷COM3：非法step值实际到设备、Error与Command序号对应，合法ACK/State及原App开关/数值控制通过；首合法raw RX漏测记录保留。原固件/授权从本次fresh备份恢复并逐字节读回，App/服务沿用匹配版本，临时网络配置撤回，规则/执行fresh逐行不变。修复候选已验收但现场原固件未永久升级，浮点边界及正式交付仍独立待办。N3首次进入约2.5秒、N4禁用任务未Start等原边界保持；不扩复杂规则、网关或服务器维护，本批无Git暂存/提交/推送。

**既有实时及H1各自限定验收保持关闭，当前历史结果只取上方共同方案§9.4～§9.6。** 真实分享历史撤权不代替全部平台LAN、生产默认启用、强杀、多消费者、MQTT分享禁控或全部历史能力；离线存储/Bulk不实施，不重开N1～N5、复杂自动化或服务器维护。[核心稳定与最小内部交付§10](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#10-核心稳定与最小内部交付)顺序1～3已关闭：原单库ZIP七目标结果取[§10.6](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#106-n3与整数step单库候选冻结及安装编译2026-10-11)，现行README/BLE实时例程候选、433项同字节重建、35项回归及S3/R4 BLE两目标差异编译取[§10.7](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#107-最短使用匹配来源与例程差异2026-10-11)。其他五目标仅输入等价与原证据，不称本批重新跑七目标。顺序4同事独立接收尚未完成。新ZIP没有永久升级现场板或完成全部实物接入验证。本文件保留总进度和历史依据；[原核心稳定计划](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md)保留C0～C4历史。生产发行、LAN/P1默认启用、复杂任务/自动化和服务器长期维护不作为本轮前置。

## 1. 结论：现在应交付什么

目标是**保留已收口的普通通信、有限任务/自动化与实时/H1，交付可重建、可安装且说明一致的内部候选**。当前不扩展通道、任务引擎或历史采样，不把授时等同于定时、消息路由等同于自动化，也不把发送方验收当同事签收。

当前状态应表述为：

- 普通接入、属性/动作控制、状态同步及有限故障恢复的 **C1～C3 冻结范围已收口**；未覆盖用户定时与自动化，不能据此宣称产品基础功能全部完成。
- “真实失效 BLE 候选不拖慢 Cloud”、Manifest 缓存协商和损坏缓存恢复均已完成，不再列为待办。
- 三端最新源码已于 10 月 10 日上一轮同步，三个修改过的原生插件保留 9 月 21 日基线；本批另完成匹配A2本机候选与有限实物，但**源码同步 ≠ 生产部署 ≠ 正式发布**。
- P1/T0～T2/A1是既有阶段背景，后续M/N、实时及H1限定结果按本页开头的现行入口，不能从旧阶段顺序重新派发。未准入的平台/生产默认保持原边界。
- 当前只核对应交付的源码/制品、公开profile、最短说明及独立接收，不自动新增功能或以闭包整理重写架构。
- 大范围服务器维护、网关多子设备、通用 LAN 产品化仍暂停。仅核对核心版本所必需的配置与依赖，不自动启动整组服务升级。

原实施顺序S1→P1/T0→T1→T2→A1→S2～S4仅作历史背景；现行交付顺序只取易用性计划§10.4/§10.9和Git手册。已过阶段不重新排队，独立接收未完成不以新功能替代；旧C4整组维护继续暂停。

## 2. 当前版本基线

下表三端提交是本轮Git交付前、2026-10-10已推送的历史基线，不含本地后续N/L/H1修改；本轮新提交及推送完成状态由最终Git报告核对，接收者记录实际拉取的三仓HEAD。五插件采用[Git手册](DEVICE_V2_INDEPENDENT_ACCEPTANCE.md#1-拉取同一组源码)列出的固定远端提交；来源可获取不代表已经部署或由同事验收。

| 模块 | 仓库 / 分支 | 基线提交 | 说明 |
| --- | --- | --- | --- |
| 设备库 | blinker-iot/blinker-library / device_v2 | `350f4474b88b8a414028316da478d4b43354e2fe` | M1～M5方案/验收更新；含 src、标准示例及 tests/tools/CI；不是正式服务 profile ZIP |
| App | blinker-iot/blinker-app / i3w-dev-v2 | `b26cb91f34cf66a781e88b7a19062cef244f07b0` | M1～M5命名/编辑/统一入口及测试；普通 LAN/P1 默认关闭 |
| IoT Service | ailyProject/aily-blinker-services / dev_v2 | `8f0bca23c52e3488f5f698559c782c72dd302fbd` | M1～M5管理合同/迁移050及测试；源码同步不等于生产部署 |
| WiFiProv 插件 | i3water/capacitor-wifiprov / main | `a21e87642a853242677f7dccd8847e9785837ab0` | BLE 发现委托及扫描协调 |
| mDNS 插件 | i3water/capacitor-mdns / main | `00f994f2896327781c64f93851932f1acbddcb8e` | 原生扫描生命周期回收 |
| 通知插件 | i3water/capacitor-ntfy / main | `83b57ee2efdd829db025d37fb898d349e6cd6a35` | 通知生命周期和点击入口 |

三个 fork 的 `origin` 已为 i3water，原 coloz 仓库为 `upstream`。App 的 `package.json` **仍未改为引用这些 fork**，它仍使用 `file:../...`。

此外还有两项既有相邻目录依赖，不能只列三个改动过的插件：

- `coloz/capacitor-android-shortcuts`：本地 `2c187e447542843dba1264bd95c297686ade6c49`。
- `coloz/capacitor-wechat`：本地 `ba0bfc5677b2a0dfe11d231c43b52e07af0bf487`。

定向邮箱邀请还依赖独立 `aily-services` Auth 的 `/users:resolve-email` 与内部鉴权；它不是上述 IoT 仓库的一部分。当前工作树存在该接口，但匹配部署版本、配置和外部权限仍需列入依赖合同。不能因“三端已推送”断言 Auth 已同步，也不处理该仓库中无关积分修改。

## 3. 能力进度：已有、剩余与明确不承诺

状态只取下表和证据的限定范围；不以完成百分比代替验收。

| 能力 | 当前证据/实现 | 核心版还需做什么 | 暂不纳入 |
| --- | --- | --- | --- |
| 单 Arduino 库/API | 一个src、4个标准示例；原七目标取§10.6，README/BLE例程差异及S3/R4 BLE编译取§10.7；现行Git交付35项打包/4目标host回归取§10.9 | 同事从Git构建一个标准库并独立接收；先核可达profile/信任；不交本地包，不称本批七目标重编或新实物已升级 | 每 MCU/模式一个 ZIP、自动全局组件注册 |
| WiFi 手动模式 | DeviceKey→session→MQTT→BBP/2；S3/R4/Wio 基础链路已测 | 配置真实接入地址；必要差异回归；明确 SDK socket 等待风险 | 新登录协议、恢复永久 EnrollmentKey |
| 普通 BLE | S3/R4 认领、Method 2、控制/状态、有限重启恢复 | 从当前候选证实不会遗漏插件/配置；只回归受影响路径 | Wio BLE 正式发布、任意 SDK 全覆盖 |
| ESP32 WiFi+BLE | 默认 BLE WiFiProv、同逻辑 ID 双通道、重配网；C1 进程中断恢复 | 匹配候选一条 WiFiProv→Cloud/BLE→恢复链 | UNO R4 WiFi+BLE；新增工厂体系 |
| 绑定/解绑/Key | 创建、重认领、刷新 Key 与旧凭据失效有实现/分批证据 | 把 owner 移除、分享退出、物理 reset 分开说明；候选必要断点补证 | 网关拓扑删除等价于普通设备 DELETE |
| Manifest/页面 | Property/Action/Event 描述能力；页面独立；缓存协商已过 S3 真机 | 固定合同，检查首次无缓存及固件摘要变化回退；已有确定性测试复用 | 编辑器重构、设备端描述整页、直接执行任意 JS |
| 日志/限流/实时采样 | 默认静默；onSample/租约/配额；L1云端、L2 BLE与内部LAN的来源/实际间隔/曲线及退出恢复各有有限实物 | 现行说明已区分采集、成功发送和State；只做实际差异回归，不重开租约主线 | 无限流、所有平台高频图表、周期历史/Bulk |
| H1状态历史 | 服务器已采纳State、数值ID倒序/有界分页，真实自有WiFi及viewer只读/撤权后下一查询清空，取共同方案§9.4～§9.6 | 当前匹配入口已列出；仅shard1已部署，独立接收及未测边界明确保留 | H2在线周期归档、离线存储/Bulk、即时撤权push、全部shard/类型实物 |
| 业务授时 | 可选 enableTime/time，已有平台/组合分别取原有限结果，不外推所有模式 | 保持原时钟保持/精度合同：默认最多一小时，断电后需有效来源；不重新派发已定稿T0 | 将业务UTC用作密钥有效性依据；无源长期离线准时保证 |
| 本机定时 | T2c时间/4槽/Start保护、原意图/未知根拒绝/退休有限真机；公开入口、安装及COM3实物通过；最小独立源码重建/安装编译闭合 | 已分享operator缺同片环境；写额度仅软件证据，4槽单目标不是全平台准入，未知根仍不能恢复；正式材料/三端完整构建未交付 | 通用脚本、无限任务、非幂等动作恰好一次 |
| 自动化 | M1～M5限定布尔实物及N5匹配新版两设备uint32 50/0短闭环收口；编辑/启停/退出App执行/查询/删除通过，Broker整数step已修正；设备共享校验源码/软件取§9.7，COM3非法值真实到达设备的短差异及原App控制取§9.8 | 保持匹配版本和原历史/unknown；COM3已恢复原固件，正式交付仍须包含修复及对应版本声明，不重跑旧门禁 | int32/float目标、多条件/动作、按钮Action、网关自动接管、全平台/生产 |
| 首次进入 | 缓存协商已收口；P1显式参数候选保持默认关闭；N3受保护20～244 B包长协商/旧端20 B回退及S3限定实物已验收，二者职责不同 | 当前ZIP含N3，编译结果取§10.6；iOS/Wio及独立接收不冒称通过，有限样本/中断/冷却限制保留 | 秒开承诺、100次交替、新抢占协议、未鉴权就增大分片 |
| 分享/通知 | 公开链接/二维码/粘贴、定向邀请、接受/拒绝、权限与撤销已有代码和分批证据 | 若候选开放：匹配 Auth、Gateway/所有 Broker 开关，双账号定点验收；否则明确不开放 | 为所有家庭复制新鉴权系统、拓扑扩张 |
| LAN WebSocket | ESP32 self/internal，有 mDNS、局部认证/撤权/页面证据；App 默认 false | 本阶段仅保留/回归共享模块；清楚标 preview | 宣称所有 WiFi 产品均正式 LAN 可用 |
| Edge Hub/子设备 | 已有模块化管理、南向会话、路由及四端部分证据 | 保留实现、修明确安全缺陷，不扩大产品声明 | 多 child 完整发布、ESP-NOW/Zigbee/Mesh、生产 eFuse 门禁 |

核心目标平台：ESP32 普通 WiFi；XIAO ESP32-S3 的 BLE/WiFi+BLE 基线；UNO R4 WiFi/BLE；Wio Terminal WiFi。平台代码覆盖不等于该家族所有型号均实测。

### 3.1 两个已完成项不再重开

- [真实失效候选对照](DEVICE_V2_FIRST_CONNECT_OPTIMIZATION_REPORT.md#4-真实失败最小修正与最终闭环)：Cloud **371 ms** ready，早于实际 BLE 候选 **5,294 ms** 失败；退出取消、重进、原固件恢复已通过。
- [缓存协商闭环](DEVICE_V2_FIRST_CONNECT_OPTIMIZATION_REPORT.md#6-manifest-缓存协商实现与同设备闭环2026-09-18)：同设备最终对照 Manifest **717→481 ms**，命中整页 **2,958 ms**，240 项回归；不把整页差值全部归因于缓存。
- [C3 汇总](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md#126-最终冻结结论)：冻结基本矩阵已收口。9 月 16 日 JSON 的 `staleBleCloudHardwareComparisonPending=true` 是历史快照，9 月 18 日报告已覆盖；**不修改原始证据 JSON 来伪装它当时已完成**。

## 4. 本次确认的交付缺口

### D1：源码仓库与最终 Arduino ZIP 必须区别

当前 `OfficialConfig.h` 默认 `replace-with-control-host.example`，`PortableServerKeys.h` 明确为 build/test key。Git 源码和标准结构可用于开发/编译，但当前下载源码 ZIP 不代表用户只填 DeviceKey 就能连接正式平台。

必须由维护者生成包含**明确服务地址/公开验签材料/安全模式**的单一候选库。私钥、用户 Key、WiFi 密码不进入库。普通 Arduino 用户无需 Python，不要求修改内部 composition。不同测试 profile 的 ZIP 只属证据，不能变成不同 MCU 的安装产品。

### D2：CI 引用源码已可获取，远端运行与交付闭包独立验收

此前“workflow已推送而tests/tools缺失”的可获取缺口已由上一轮同步关闭；原逐项检查记录保留。2026-10-11只读`git rev-parse`确认三端HEAD仍与§2一致，`git ls-tree`确认设备库`350f447…`含`.github/workflows/device-v2-ci.yml`、`tests/`、`tools/`。这不含本地后续未提交的N/L/H1修改、不等于本批重新跑远端CI；不再把旧`bd0c913…`写成当前HEAD，也不删门禁制造通过。

- App `e9e1880…` 的 Git 树已含 `vitest.config.ts`、自动化 client/page spec、`device-template/tests`、授时公开 golden fixture 和 `tools/source-handoff.mjs` / 自测，原“新增测试与 Vitest 修订尚未上传”不再是当前源码状态。
- Service `7bd1007…` 的合同、Broker 观察与 Gateway worker/版本策略测试及 package 脚本源码仍在 HEAD 内；需要数据库的测试必须按真实环境运行，不能把 skip 算 pass。
- 这里只闭合已引用源码的可获取性。本轮未查询远端 Actions，未新获取仓库，也未把历史本地 240/240 或其他软件结果改称远端 CI 成功。干净 checkout 的实际构建、精确来源/锁/外部工具链闭包、旧 S4 接收验收及生产 profile/签名仍分别取证，不由三端推送或本段树检查代替。
- 最小确定性测试、CI/打包工具与公开 fixture 属于维护源码；**不因此进入用户 Arduino ZIP**。APK、现场日志、设备/账号秘密与私有备份仍不属于这些公开源码。

### D3：App 相邻插件与构建产物依赖

五插件必要实现已有固定远端提交，现行来源与相邻目录布局取Git手册。fork推送只解决源码可获取，不能单独证明lockfile、插件dist、Capacitor同步及固定工具链匹配。Android本机`capacitor.settings.gradle`的pnpm缓存路径差异不能手工复制到其他机器；接收者按原锁先构建插件再安装App，记录实际来源，不交旧node_modules或冻结副本。

本阶段建议保留现有五个插件布局，先用**一份维护者 checkout 清单锁定全部提交**，在新目录完成插件构建→App 安装→Capacitor 同步→APK。避免本轮同时引入 monorepo/私有包注册表/依赖大升级。

Git SHA 依赖或版本化 npm 包是后续可选分发方式；变更前验证 prepare/prepublish、Android/iOS 产物是否齐全。不得简单把 `file:` 改成 Git URL 就宣称问题解决。

第六十三批实现[App Android源码交接](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md)，第六十四批纠正安装合同：App/模板唯一pnpm锁，五个独立插件保留各自npm锁；必要工作树输入逐文件SHA冻结，在新目录明确构建插件和App。原“六份npm锁”仅描述第63批诊断包，不再作为执行入口。生成原生路径不拷贝，HEAD不冒充已提交闭包；iOS、完整SDK/Maven离线闭包、远端最小CI/生产签名仍不在已通过范围。

### D4：配置/部署覆盖与产品声明

Service 的跨片 requests/shares、LocalAccess、通知安装/投递以及 App LAN 默认关闭。声明“已实现”必须同时列出是否编译、部署、启用和验收。分享如开放，requests 必须在 Gateway、源/目标 Broker 及动态模板保持一致；不靠只改 Gateway 放行。

初始梳理仅取配置快照；第三十批已对本机Gateway/Broker单设备试点先备份后迁移/部署，见§8.30。Auth与线上环境未升级，正式域名/信任材料及生产部署批准仍不是已完成事项，不能拿本机准入外推生产开放。

### D5：文档中的 SDK/时效信息需纠正

[C4 §4](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md#4-sdk纠正私有-sdk-误解)已有本机与官方归档逐字节比对证据：S3 3.3.11 为官方 stock，直接使用 IDF NimBLE，不需要额外 NimBLE-Arduino。早期 B1 对其来源保留的不确定性已由该证据补齐。

这不是“所有 SDK 均无资源风险”，也不是本轮又查过最新 SDK。固定已验证基线，不按版本号臆断旧 IPC 栈风险全部消失；候选保存 sdkconfig/依赖摘要，运行峰值与长稳边界独立列出。

## 5. 保留的模块边界：按职责而非文件数量重构

基本不变量：身份不依赖 MAC；只有一套设备语义；授权与承载分开；连接关闭有唯一 owner；缓存不能制造 ready；错误分类决定能否重试；一次性 Action 不跨通道自动重放。

| 层 | 唯一职责 | 不应承担 |
| --- | --- | --- |
| Arduino facade / 示例 | 组件、begin/run/report、可选诊断与业务授时 | Broker/HTTP 对象拼装、workspace 参数、平台 SDK 暴露 |
| 设备协议/运行时 | BBP/2、Manifest/State、ACK、命令/事件、可选时间/遥测 | WiFi SDK、App 页面样式、服务器账户查询 |
| 产品 composition | 按平台能力组装现有 runtime/ports，管理有界生命周期 | 再写一套协议、为每个 MCU 设计新用户 API |
| ports / transport | 网络、存储、时钟、BLE GATT、WebSocket 字节承载 | 以 MAC 决定所有权、未经上层授权自动重试业务 |
| App 页面 / DeviceUiPort | 页面生命周期、现有连接组、已就绪通道选择、展示 | 直接组装加密帧、管理 SDK 扫描资源、另开账户 MQTT |
| App 协议/Direct session | 编解码、认证后的协商、缓存验证、ACK/State | Angular 页面操作、账户缓存 key 拼接、系统扫描 |
| 原生插件 | OS 权限、扫描/GATT/网络/通知资源 | 决定设备业务身份、修改 ACL、偷偷接替其他 owner |
| Gateway / Auth | 账号、所有权、分享、签发与撤销、服务入口 | UI 布局决定 MCU 能力、在数据库事务内等待 MCU |
| Broker | 已授权路由、会话、配额、可靠转发/必要 ledger | UI JSON 全量转换、第二套 DeviceKey/分享数据库 |
| 网关可选模块 | 北向会话/管理与南向连接/协议适配的组合 | 把普通 WiFi 核心复制一份、现在就实现所有南向协议 |

当前方向合理，**不建议再次重写整库或把多文件合成一个“大 facade”**。已有 `IFrameTransport`、`ITimeSync`、共享 Direct session、`IGatewayChildConnection` 等是可复用边界，不为“简洁”删除。

将来 LAN 应通过能力扩展挂到 WiFi/WiFi+BLE/Hub，复用同一模型和云登录；目前 ESP32 internal extension 已示范该边界，不能据此宣称全部产品已接线。将来其他网关南向复用管理/路由合同，新增端口或适配器；不提前做无实际消费者的万能总线。

### 5.1 现有用户 API 保持稳定，新能力按需启用

- `<Blinker.h>` 默认 WiFi：`Blinker.begin(key, ssid, password, properties...)`。
- `<BlinkerBLE.h>` 普通 BLE：`Blinker.begin(properties...)`。
- `<BlinkerWiFiBLE.h>` ESP32 App 配网：`Blinker.begin(properties...)`。
- 当前公开 WiFi+BLE facade **不接受手动 key/ssid/password 重载**；早期讨论不是现有能力。手动接入由 WiFi 模式满足，是否补混合手动重载另行确认，不能悄悄扩 scope。
- `BLINKER_PROPERTY` / `BLINKER_ACTION` / Event 是语义；switch/button/slider/text 为 App 控件，Manifest 不是页面文件。App 手工/agent 生成的布局独立保存，能力变化提示更新。
- 定时入口已按执行器§8.17实现：维护者准入候选下定义`BLINKER_ENABLE_TASKS 1`并在begin之前调用`Blinker.enableTasks(properties...)`，自动复用授时；默认源码关闭。无新begin参数、不暴露composition/workspace；第四十四批已过安装/最小实物。onSample和OutboundScheduler继续独立负责采样/发送。

### 5.2 身份、安全与状态不变量

- DeviceKey 是可重新查看/粘贴的长期手动入口，逻辑 deviceId 与 secret 分开；刷新按服务器 credential version 失效旧 Key 及其派生登录信息。
- 设备重启不要求重新配网。WiFi 云端凭据、本地授权、配网 pending 是不同职责，不能为省存储随意互相替代。
- owner 移除、被分享者退出/撤销、手机清本地记录、设备物理 reset 分开验收。云端撤销完成不冒充 MCU 已清根；离线本地授权按既有凭据/租约及退休机制处理，不承诺不可达设备瞬时失效。
- 普通 WiFi 仍默认明文、显式可选 TLS；明文不提供传输机密性/完整性，登录成功不等于每帧防篡改。Cloud Frame Authentication 后置，不用“已鉴权”掩盖该边界。
- BLE 配网 Security1 与普通 Direct 应用层认证/安全 record 属于不同阶段，不因 MQTT 默认明文而一起删除。网关秘密下发的 TLS 要求保留。
- Manifest 必须验证 canonical hash＋真实设备 ACK，fresh State 必须新取；离线预览不等于控制准入。业务 UTC 不反向改变鉴权/租约单调时间。
- App升级/本机构建保持既有WebView origin；localStorage缓存和IndexedDB事务意图按origin隔离。新origin下的空journal不等于原操作没有发生，不能自动重发/新建ID或清旧数据绕过。MQTT设备默认明文与App是否切换origin是两个独立问题。

## 6. 新执行队列

本节 S/P/T/A 队列为先前实施与范围记录，不是当前重新执行清单。当前 **N1～N5限定本机范围收口** 取[易用性后续计划§9.6](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#96-n5-本机数值短验收2026-10-11收口)，不等同全平台或生产发布。后续结果在新计划汇总，不再追加整段重复的历史“下一步”。

各包要求独立可验收。P1/T0 可独立推进；缺硬件时保留 P1 待测并进行任务纯核心软件工作，不转去扩另一条架构。新增功能的详细合同/各端职责/门禁统一维护在[任务与 BLE 方案](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md)。

| 顺序 | 范围与实施内容 | 完成条件 | 不做 |
| --- | --- | --- | --- |
| S0/F0 进度与功能合同 | 本文、版本/缺口核对、定时/自动化原则与三端分工 | 本轮文档完成；kind/精度/API 等实现细节仍待 T0 | 宣称定时/自动化已实现、重跑 C1～C3 |
| S1 最小依赖准备 | 本地完成：核对三端＋5 插件 HEAD、相邻依赖入口与本批测试；未改依赖 | 本批构建/测试可用；D1～D5 与完整干净构建仍在 S2 前闭合 | 为准备无限扩大维护；未经确认推 docs/tests |
| **P1 BLE 有界优化** | 实际 GATT/认证/分片阶段和参数测量，只做有证据的兼容小修 | 对照可归因、取消/授权/保守端不退步；无明确收益则结束 | 直接删 30 ms、单边增 MTU、换 SDK/库、自动启动 P2 |
| **T0 任务实现合同定稿** | 根缓存、唯一Session/Engine、严格wire、adapter/生命周期、WiFi carrier、权威scope/签发/结果与退休已接；Broker显式白名单试点已真实协商，默认仍关闭 | 普通WiFi有限合同、用户opt-in/profile责任已明确；不重做已接线管理协议，不冒充生产开放 | owner由App自称、业务UTC改安全时钟 |
| T1 本机任务核心 | 核心/四档资源、COM3隔离NVS断电窗口；4槽真实WiFi产品共存及Start/时间/容量policy有限真机已过 | 写额度仍软件证据；其他平台/正式签发profile/长期运行未准入，不重跑已过policy | 万能调度器、脚本、每任务线程、伪造网络会话 |
| T2 定时三端闭环 | owner有限实物、公开入口安装/COM3退出执行与认证退休、最小设备源码独立重建/安装编译通过 | 已分享operator同片实物；写额度、最大目录、BLE任务和正式profile未准入；未知根尚不能恢复 | 云端同时按设备任务执行、无限离线准时声明 |
| A1 最小云端自动化 | A1-1～6限定软件/匹配本机部署/真实App/两S3闭环通过 | 同owner/tenant/shard普通WiFi、boolean上升沿→固定boolean属性；四次ACK及有限恢复 | 多步骤、网关迁移、任意事件恰好一次、将有限本机测试当正式生产准入 |
| S2 匹配候选制品 | 一份 maintainer profile 映射服务地址/公开信任材料；同源单 ZIP、App APK、Service 镜像与迁移清单 | 记录源码/工具链/profile/制品摘要；正式和 preview 开关表；全新目录构建验证 | 改现网、维护栈整体升级、每 MCU 一个 ZIP、development key 当生产 key |
| S3 定点业务验收 | 复用 C3 和缓存证据，按 S1/S2 差异选择关键链路；有限回归见下表 | 同候选/同真实服务边界；命令 ACK＋真实 State/计数；退出回收；失败修在原模块 | 为凑次数跑 100 轮、无差异全平台反复重刷、Action 自动跨路重发 |
| S4 核心候选交接 | 简洁 QuickStart、支持矩阵、诊断/重置说明、已知限制、维护者构建入口 | 同事可从清单独立安装和复现实例；标清“内部候选”与“正式发布” | 将传输 preview、维护工具完成度包装成正式生产准入 |

**第33～38项、第84最小执行流程与第85～86[产品收口M1～M5](../../blinker-app/docs/APP_DEVICE_V2_MINIMAL_AUTOMATION_PRODUCT_PLAN.md#8-m5本机新版本验收)按限定范围完成，不重跑A2-4或M5。** 命名／编辑和简化页面已沿文档→管理合同与迁移→页面／入口→联合软件验证→本机新版本短闭环收口。没有新执行器、协议或MQTT连接；下一步只按用户试用反馈修复现有范围的已证实缺口，或明确要求后执行独立交接。正式profile/签名、P1/LAN/网关、长循环及服务器整组维护仍独立后续；原历史与未知结果保留，T2及第32项缺环境项不自动重开。以下记录只作历史依据：

1. **已完成T2b-1**：原Read通道返回实际容量及最多32项固件调度白名单；流式fingerprint锁定分页，完整帧最大183 B。普通Manifest可写仍不等于调度许可，属性约束不在目录重复建模。当前目录需已初始化的任务authority；页面必须让用户显式启用，不能查询时偷偷Initialize，不支持/读取失败也不能回退为所有可写属性。
2. **已完成T2b-2软件接线**：原列表/编辑器使用真实taskId、认证目录及同revision记录；删除旧JSON/160ms/900ms猜成功和loop/默认动作。创建不Start、启用不恢复；未知写精确重试，只有新鲜查询后明确确认才允许停止本地追踪（不取消远端任务）。账号/页面取消、旧弹窗拒绝、持久写前存储及请求节流已测。Manifest缓存只作提示，MCU验当前约束；不把页面编译当真机验收。
3. **已过T2c-1限定链路**：真实App原owner创建专用设备、显式启用→4槽/单power目录→20秒倒计时安装/单独Start→退出/force-stop后执行→认证查询Done；原ID严格日志跨force-stop恢复，historical确认不产生第二次Start；软件重启后Aborted且超过期限不执行。过程发现三项App缺陷已修正，不能把旧localStorage写成功当作持久提交。
4. **已过T2c日历/显示**：第三十一批在“带翅膀的蜗牛”完成最终成功列表、一次日期/固定偏移星期任务、重启UTC无效→到期前授时恢复→实际执行，执行后再重启不重放；同一账号MQTT普通Property/Action的ACK和State也通过。编辑器写确认后不再先完整重读，唯一新鲜查询由返回的列表完成；未决写仍保留，限额未放宽。
5. **第三十二批已过有限时间/policy**：仅暂停COM3专用登录代理后软件重启，真实UTC无效跨过到期再恢复，旧任务未补放；新未来4任务同刻全部完成。第5条未写入且原表/修订不变；运行中全局30秒Start冷却生效，限制解除后显式重提同一原ID执行一次。不将这些结果外推32目标/10任务或寿命。
6. **第三十三批已补冷启动/正常Key闭环**：最终候选重启约5/13秒明确为连接不唯一，约23秒认证Read可用后Start未写入/执行；窗口结束后原ID执行一次。连接选择/协商与任务执行拒绝分层，不选最新socket。刷新Key后旧连接失效，旧Key软件重启仍不能使用任务云通道；新Key上线须显式同步任务根，原taskId/定义/完成记录保留。6秒前云通道并未就绪，未把此段当真机Flash额度证据，也不为抢窗口加入旁路。
7. **第三十四批软件恢复、第三十五批限定真机已完成**：旧Start认证响应到达App客户端结算前被受控阻断，force-stop后保留原意图；换Key上线后显式同步，再丢同步确认并重启，双记录保留。恢复同步原ID只清同步项，旧Start仍held；新鲜表＋明确停止本地追踪后，新Start执行一次。Service三条mutation各1attempt；真实非owner的catalog/initialize/start均403。此处Service已经确认旧Start，不外推到Service未决的prepared/issued组合，它们仍只有第三十四批软件证据。
8. **第三十六批软件已完成**：Result新增Rejected(7)，只认证写额度/Start冷却两个写前拒绝；独立fixture、77 B最坏帧、原ID未知→拒绝→恢复、坏证明/提交回滚/历史保留/无自动重发均已回归。schema043只改原attempt约束，request保持pending；普通Failure仍unknown。App沿原journal与提示，不另建恢复队列。真实MCU和常驻schema尚未使用此候选。
9. **第三十七批有限真机完成**：备份后升级本机schema043/匹配Gateway和App；COM3真实运行中Start冷却拒绝时快照5→5、回调仍1，Service原request pending而attempt rejected；App杀进程原意图保持且没有新attempt/执行，明确原ID恢复后只新增一次成功attempt、Start序号1→2、任务回调2。普通Property/Action的ACK/State及退出回收也过。写额度无可靠早启动云窗口，仍保留软件证据，不为抢窗口放宽限额。
10. **第三十八批有限profile评审及首个恢复限制完成**：执行器§8.14集中定义普通WiFi owner、4槽/单目标封闭试点，不代表最终容量。首次根未确认且旧Initialize已签发后换Key，在新账本/签名前明确拒绝并保留历史；App阻止无效重试/同步。只prepared、同根恢复及已确认根的正常单次同步不误阻断。viewer/operator全阶段隔离回归不代替真机；现有真实账号仍分属shard1/0，无同片条件，不改账号归属/开启跨片。
11. **第三十九批连续未知根软件边界完成**：已确认A→已签发B未确认→当前C，按持久变更previousRoot=A明确拒绝；过期attempt的Grant仍算证据，不能用新prepared盖掉。相反，只prepared的B不阻断A→C；认证Read已确认B，即使原A→B请求pending且Read历史被回收，仍允许B→C。复用首次未知根同一检查/错误，无新根缓存、迁移、自动恢复或新增签发入口。App提示同时涵盖初始化/同步，保持旧业务意图及同步记录。
12. **第四十批匹配候选差异真机完成，退休未通过**：在真实Broker回传之后、Service验收之前一次性截断COM3的B根结果；58 B结果独立HMAC通过，Service仍issued/无结果/确认A。Key变C后真实App重启、查询/再次查询均明确拒绝且原日志保留，服务器三条历史不变；普通MQTT属性/按钮通过。云端owner删除202，但MCU退休无Grant/无结果/无快照写入，不计为设备清理成功。过程、备份和恢复证据见执行器§10.39；未直接写业务DB或操纵其他两块板。
13. **第四十一批跨根退休闭环完成**：Session用当前根认证、generation锁根代际，expected仍固定完整旧表绑定；instance/scope相同且签名有效才沿原Engine终止。软件覆盖旧请求/错误证明、换根窗口、存储失败、新句柄/新根查询及MQTT丢回执恢复。真实A→B结果在Service验收前丢失、再换C后普通管理仍拒绝；正常owner删除取得74 B认证终止回执，旧A/B Key验证均失败，MCU仅一次终止写。未伪造Rebind、把旧同步改记成功或恢复未知任务；原删除合同将旧issued attempt关闭为unknown。详见执行器§10.40。
14. **第四十二批首次未知根差异实物完成**：真实首次Initialize的58 B结果在Service验收前丢失；MCU安装空表，Service确认根始终为空。刷新Key并真实重启App后，两次查询均拒绝、原日志/账本不变、没有新任务写；普通按钮/属性控制通过。owner删除取得74 B认证终止回执，旧Key验证失败，仅一次终止写。六种实际编译器模式检查符合原guard，无需重复实现组合限制。已分享operator只读复核仍无同片条件，保留待测。
15. **第四十三批最小入口/profile软件收口完成**：`enableTasks`只接受明确的Property白名单，复制有限绑定并自动复用授时；重复/错误配置不覆盖，begin前验证成员与回调。默认不链接任务图，普通facade仍12 B；仅普通ESP32 WiFi，两个probe迁移，旧内部宏/配置旁路删除。原打包器只接受命名development试点profile，不将测试材料提升为production。70项主机、Win32四项、15种真实编译器准入与四档实际链接通过；没有新协议、Engine、连接或服务改动。
16. **第四十四批候选安装＋公开入口最小实物已完成**：匹配4槽profile经原单库打包器生成、隔离安装；普通ESP32 WiFi示例、S3最小任务Sketch和S3公开API测试固件完整编译，逐文件ZIP/来源/链接核验通过。真实owner/App/COM3显式初始化、20秒倒计时安装但不执行、单独Start、退出/force-stop后回调一次、认证查询Completed；普通Action/Property ACK192/92ms与设备计数一致。正常owner删除取得74 B认证退休回执，12条MCU结果独立HMAC通过。COM3整8MB/授权恢复并verify，COM11/12不动；本机只改试点ID后恢复、临时代理移除，同镜像/schema43，无生产源码或APK改动。brokerCleanupPending/localAccess未确认仍分别保留，不扩大其他准入声明。
17. **第四十五批最小设备源码交接完成**：原库白名单、维护工具/公开任务消费者及PubSubClient 2.8源码组成433项清单；从仓库外目录重建ZIP，与第四十四批真机候选逐字节一致。交接物自带安装门禁完整编译通过，来源/依赖字节/链接均核对，Flash984,948 B、静态RAM53,936 B。任务目标显式选择、不扩普通七目标；SDK/CLI须另行预装，未交付完整CI/App/Service构建闭包。9项交接自测在原目录和独立目录均通过，19项打包回归通过。失配旧联调生成器退役并留备份；无生产代码/硬件/部署/Git写入。已分享operator只读复核仍缺同片条件，保持待测。
18. **第四十六批A1-0设计和源码复核完成**：唯一规则/执行权威在Gateway PG，Broker只观察已验收lane/调用原command ledger；两张业务表和有界worker、基线/field变化新鲜度/epoch、原ID/未知结果、发送前逐次业务准入与TTL、撤权竞态均落到[专项方案](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md)。首轮收敛为同owner两台WiFi、bool源false→true→固定bool属性，不自动采样、不借手机身份、不实现Service RPC。源码重建及既有隔离Broker套件、10项签名/worker回归通过；没有A1生产实现/部署或新真机证据。旧scene页面仍有消费者，不盲删。
19. **第四十七批A1-1纯contract/判定核心已完成**：严格bool规则/policy、baseline/change/reset、显式watch激活与11字段绑定、false→true及冷却消费、容量/一跳图约束集中一个无I/O模块。24项确定性测试包含512组布尔序列；修正冷却中发生但迟到的边沿不能在到期后补触发。返回nextState＋候选副本，未来仓储同事务提交；尚无DB/HTTP/MQTT/Angular或后台timer接线，不算真实自动化已可用。具体合同与本批27通过/2skip的既有回归见[专项§12](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#12-a1-1纯合同及判定实现2026-09-28)。
20. **第四十八～四十九批A1-2软件完成**：有界watch/lease/record、真实accepted lane与连接生命周期、reset/ACK已接；签名内部观察、精确shard/owner/请求摘要、当前授权端口、取消/真实I/O容量、单调剩余新鲜预算和Gateway消费检查已通过。新19项控制＋4项编码，联合既有184项通过；最大字段＋四记录响应正文1,762 B，不是设备BBP帧。HTTP200不自动ACK，旧epoch不自动激活；无真实PG权威默认拒绝、生产未挂载。详见[专项§14](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#14-a1-2b签名观察控制与剩余预算2026-09-28)，不宣称自动化已真机可用。
21. **第五十批A1-3a隔离PG完成**：migration044两表及小仓储，当前owner/设备/Key/route、CAS、图/容量、持久claim/fence、显式epoch及原子runtime＋execution＋目标预约。23项PG场景覆盖并发、COMMIT失败、过期、重复和unknown保留；合并回归263节点通过。Gateway没有实时Manifest，能力receipt检查口缺失默认拒绝，真实实现待A1-3b；不能将测试替身当生产权限。详见专项§15，源/目标设备ID暂不可编辑、无规则退役/动作发送。
22. **第五十四批A1-3b-1～4限定软件包完成**：当前PG授权、Broker实时能力/两端连接、同事务生命周期失效和原驱动worker已组合；每条链独立能力闭包，预检→能力→claim→观察→基线→consume→提交后ACK。新epoch先换fence，取消等待实际I/O，ACK丢失按持久游标去重。相关331节点通过；详见专项§19，生产未挂载，不算设备动作已闭环。
23. **第五十八批A1-4限定软件包完成**：原Command准入/status、PG单次认领/结果及有界执行驱动已组合；只有本次claimed=true才dispatch，已有dispatching在原截止前不抢发/抢查，避免missing误伤合法投递。截止后或unknown只读恢复，未认领prepared可按原截止过期，未知预约不清除；取消等待真实I/O。详见专项§23，尚未生产挂载或实物验收。
24. **A1-5匹配测试部署/真实账号App通过**：第六十二批备份并独立恢复验证后，仅更新必要Gateway/Broker1/contracts及schema043→049，五用途新密钥按shard配对；实跑关闭503、缺配置启动拒绝。修正App automation URL未带Bearer的真实缺口，真实ME/列表200、Android杀进程原意图保留/只读404不重发通过。两S3上线后用同一个原请求恢复停用创建，原生日志确认清除，再按CAS启停；不以换ID绕过失败。详见[专项§27](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#27-a1-5匹配测试部署真实账号app与实物准备2026-10-08)。旧scene仍有消费者，不重做编辑器或升级整组服务。
25. **A1-6两普通WiFi限定实物通过**：YYSC下COM3源/COM11目标、普通公开WiFi API，匹配fingerprint/fresh State。四条执行均ACK，实际回调与State分开验证；App杀进程后继续、两个服务分别重启不补发/新边沿可执行、源单独复位重建基线、冷却结束后的停用拒绝均通过。串口工具早期关闭导致的额外复位保留记录，修正控制线后用连续uptime/回调复验；不能把最终累计3写成4。原规则最终revision5/disabled、pending0，保留四条历史；不扩大长稳、Action或R4/Wio自动化实测声明。
26. **第六十四批S2b本机源码/依赖包通过**：App/模板统一pnpm11.19.0及唯一锁，五独立插件仍按各自npm锁构建；旧App npm锁备份后移除。补模板importer、漏报的已测Core/Buffer依赖及公开授时fixture，保留实际版本，不升级框架/开全量hoist。最终1,642输入、16步Android重建、9项工具自测、2项真实锁失配对照、31文件391项相关回归通过；69项直接版本零差异，五插件运行时/Android源码与原已测安装一致。第63批漂移包及本批首次缺依赖/fixture失败均保留，不改历史为通过。设备同ZIP/Service来源复用S2a，没有新增实物证据；详见[候选清单§5.1](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#51-第64批s2b依赖合同与最终源码重建)。
27. **第66批普通Cloud短链关闭**：保留第65批匹配HTTPS App/测试CA，复用已过BLE，不重装App。现场YYSC、服务地址/原DeviceKey版本核对后，仅COM3新鲜读取系统区＋整8MB重构备份核验；原镜像含旧网络且不含当前Key原文，仅作为旧配置线索，不断言唯一根因。416项库文件与同一候选ZIP逐字节相同，标准WiFiQuickStart只替换私有Key/网络参数，编译/定点烧录后实际上线、Manifest/fresh State、Property/Action ACK及按钮0→1通过。一次普通复位后新State约38.6秒才到，旧State不算新启动证据；恢复后Property ACK/真实State通过，不重放Action。最后原固件/授权范围校验恢复，A1只读仍revision5/disabled/4条ACK；详见[清单§5.3](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#53-第66批s3普通cloud控制重启恢复与收尾)，无服务部署/迁移、Key刷新或其他板操作。
28. **S4发送方材料完成，接收验收待办**：第67批[干净交接目录](../../local-maintenance/core-candidate-handoff-20261008/README.md)及[清单§5.4](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#54-第67批s4干净交接目录与离线验收)不变；安装/API/重置/诊断取清单§7。只由接收方在包外填写真实安装/短链记录，不代签、不继续重复打包。mostfun2026不等于冻结实验地址可达，变更profile须新建变体/差异门禁。第68批用户明确同意保留待验收并先继续P1，不代表生产profile/签名/远端CI或缺环境项已通过。
29. **第68批P1限定门禁通过**：修正实验工具将可选hint拒绝当认证失败、固定150ms猜取消阶段及UI吞错误记ACK的问题；默认观察不改策略，显式HIGH仅bootstrap，fresh State就绪请求BALANCED。原已绑定Android/S3实际4轮Property/Action ACK和State/计数、无缓存ManifestRequest、拒绝故障注入、认证途中取消/6.4秒观察和正常重进通过；14/14工具测试、0skip，详见共同方案§7.4。Cloud仍retrying、原页面Cloud错误保留，不算云验收；本批无APK/MCU/服务更新，不修改冻结S4包。
30. **第69批有界可选策略及限定APK门禁完成**：小型纯窗口策略由原native link持有，原client的Manifest接受＋fresh State同步结束窗口；Android有效GATT后HIGH，250ms无响应提前收窗、5秒最长本地窗口，正常结束请求BALANCED；退出/换代取消计时器、不向后继补发。独立native提示避开BleClient共用GATT队列，不以Promise.race掩盖队列阻塞。普通配置false，只有明确的ble-priority-test构建开启；217项相关回归/类型/新模块lint与匹配APK有限真机通过，详见共同方案§7.5。没有协议/MCU/插件/Service变动，不把本地候选写入冻结S4；手机已恢复原APK。HIGH/BALANCED为best-effort参数请求，本地截止不代表已发native操作可取消或实际空口强制切换。
31. **第70批同候选有限配对收益判断完成**：同1,646输入、同手机/固件/接受缓存与20B/4B/30ms合同，关闭/开启各3个有效样本。GATT后配对减少272/271/216ms；中位配对271ms是相对关闭组中位1660ms的16.3%，不混同两组中位数之差241ms。真实Property/Action各模式一次、按钮11→12→13；后四轮只读、原生实际15→30ms和退出回收均核对。原预热两次中断，第一原因不全、第二在扫描阶段；续测新增固定35秒安装前冷却，全部失败记录保留，因此只算有限收益信号，不称严格连续实验、稳定整页提速或兼容性全通过。详见共同方案§7.6；普通配置关闭、手机恢复原APK，不改wire/SDK/固件/服务。
32. **可用环境有限兼容性证据完成，缺实物部分仍待测**：保留第69批窗口参数与普通配置关闭，不再实现新策略。Android/S3既有有限证据、UNO R4缓存/一次控制/正常退出/认证取消取第72批；健康原账户MQTT与BLE前/中/后Cloud新鲜只读短链取第73批。Wio仅保留preview待测，不提升正式BLE支持；iOS跳过逻辑有软件证据，未有实机不得称实测。每种可用环境只做受影响有限场景，现有通过项不重跑整组C3/100轮；不为缺环境扩大服务器维护或添加P2/网关功能。默认启用、并入新交付候选及S4接收验收仍须另行评审，不覆盖原冻结目录。

第72批继续第32项：R4通过正式认领、缓存命中/缺失、一次Property/Action、退出、真实认证取消和只读重进的有限场景；首次认领失败定位为打包测试公钥`0x3F/0xF3`笔误，修正的是隔离profile与打包工具/测试，不删验签。一次GATT失败未到认证不能记取消通过，已补工具拒绝误报并保留原记录。窗口内同一健康账户MQTT已核对；Cloud设备前后控制、iOS实物及Wio preview仍待证。R4原应用及手机原APK恢复收尾完成，不以本批小样本宣称稳定提速或正式发布；见§8.72。

第73批继续第32项：用户许可COM3后，仅公开WiFi示例临时更新网络/profile，用现有DeviceKey版本2上线；COM11/COM4不刷写或清授权。原账户MQTT在BLE就绪前/保持连接中/退出后均收到新StateRequest→StatePage（不同序号，非缓存），三次状态读取往返各60ms；BLE只读ready3393ms、退出213ms归零，不发Property/Action。原板及App恢复完成，恢复旧配置后见证板离线如实记录。详细恢复/端口例外取§8.73及[共同方案§7.9](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#79-第73批健康cloudble只读短链及恢复2026-10-09)。旧S4是历史普通WiFi交接证据，**不得直接拿其中错误Ed25519公钥包验证当前服务BLE认领**；新修正组件包取[清单§5.5](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#55-第73批公开公钥修正组件交接及有限并行短链)，不覆盖旧证据。

33. **已完成P1候选与交付边界收口（第74批）**：保留显式Android测试候选，不默认启用，不新建策略；[共同方案§3.4](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#34-p1候选冻结与使用边界2026-10-09)冻结作用路径/窗口/配置及支持限制，[清单§3.1历史比较](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#31-当前匹配组件索引第83批收口)区分原S4、公开公钥修正设备组件与独立P1 App。1,646项当批App/插件输入与原P1冻结源匹配，原S4/两个P1 APK及修正组件摘要未变；当批交接工具明确拒绝P1配置，不能声称已做该变体新目录依赖重建。iOS/Wio及接收待办保留，不阻塞未启用P1的普通候选，也不提升正式支持。无输入变化不重建/刷全部板；后继合成包、默认开启、Git/部署须各自差异验收/授权，旧S4不覆写、不代签。
34. **已完成P1显式实验的新目录构建（第75批）**：仅修改交接工具/测试，把`local,ble-priority-test`加入固定白名单；普通P1仍关闭，仍拒绝裸开关/production混合/任意组合。新1,646项快照与原P1仅这两项差异，由包内入口完成五插件/App/模板安装构建、类型/Web/sync/Android debug共16步；12项工具/隔离目录41项相关回归均通过。依赖版本/冻结锁/插件发布内容及HTTPS origin/实际P1导出核对通过，原8项制品未变。新APK `f54b78…4209`无测试CA、未安装，不能写为旧实物包/接收或生产通过；详细新manifest/制品取§8.75，不扩运行时/协议/SDK/部署。
35. **已完成新构建来源的匹配CA有限实物（第76批）**：在第34项原隔离work显式加入已验证公开测试CA，App build独立输出，新APK `cebd57…2b85`；原无CA APK和八项旧制品不改。原签名/HTTPS origin、1,580项assets/16项native lib和22个App Java类匹配；两个新资源改变生成R及项目DEX，不称整DEX同字节。Android/XIAO缓存命中、一次Property/Action ACK、退出、认证途中取消、只读重进/不重放通过；17项测量工具测试0skip，不重复累计旧App业务回归。手机原包实际恢复读回，授权/缓存/3条任务意图及空自动化journal摘要不变、零连接/观测；不刷板、清授权、部署或修改源码。仅发送方新包有限差异链闭合，不代签第28项或外推iOS/Wio/稳定提速，详见§8.76。
36. **完成A2-0/1合同与纯条件核心**：按[Service§28](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#28-a2实用条件扩展与执行顺序2026-10-09)明确布尔双方向、数值阈值/正回差、基线/重置、版本与持久锁定；shared落无I/O模块，A1原上升沿复用；103节点联合回归、三项类型检查通过，见§8.77。公开A1输入/输出、观察/能力验签、DB和设备wire保持原合同，新条件只作纯核心候选，不部署/开放。
37. **完成A2-2内部版本化底座（第80批历史准入）**：2a类型观察、2b原事务/证明/执行、2c完整V2两worker＋五用途签名HTTP＋PG提交/ACK＋Command/ledger、并发/恢复与公开版本边界通过§8.80。当批默认production/public仅A1；第81批源码公开准入取下一项，不将旧历史描述当当前runtime。graph全量配额不缩，软件fixture不是真实MQTT/MCU，不重复该门禁或据此自动部署。
38. **A2-3软件＋A2-4限定实物收口（第82批）**：原V1四字段/V2六字段合同与同一观察/PG/Command保持；匹配新目录App、Gateway/Broker1及普通WiFi库候选，真实同owner/tenant/shard两S3下降沿、uint32上阈值/回差边界、重复、基线、App退出、停用/源重启恢复已通过。原ID最终revision8/disabled/pending0，4条ACK对应真实连续回调0→4；错过首次冷却窗口后的合法第3次保留，第二窗口真实抑制且不补发。两板原固件/授权恢复及临时入口清理通过。保留匹配A2 App/服务与历史，不能无感回退旧A1组合。页面不冒称已有条件编辑器；完整条件/权限矩阵、正式profile/交接/生产产品独立保留，不扩任意表达式、多步骤、跨账号/片、周期采样或网关离线。详见§8.82。

这条WiFi owner链及最小设备交接已具备有限证据；不同时扩BLE/混合根/网关任务、页面重写或服务器维护。未知写意图/权威历史占满时仍明确拒绝；有限回收不是长期生产账本或寿命准入。A1设计进入下一包不等于取消T2发布限制。

S1 原有依赖清单/源码白名单/干净 checkout 目标仍保留；若用户保持 tests/tools 不上传，采用明确可交接的本地维护工件，远端 CI 留“未闭合”，不能删除校验使其绿灯。上传、现网部署与不可逆测试仍需原有授权，不由方案自动扩大。

推进修订：T2c已提供真实owner/App/MCU有限证据，不等于正式部署或T2整体关闭。原session相同的重连也换attempt/nonce；失败保留原ID/意图。64请求/8尝试不扩大；只允许用户核对后的显式本地追踪放弃，不删除服务器未知记录来绕过额度。大目录短期反复全量读取仍可能触及账本上限，UI报额度错误。WebView日志已改为IndexedDB strict事务提交屏障，真实force-stop原意图恢复已过；仍不保证卸载、清数据、存储驱逐或多窗口并发。第三十批切网后删除的localTasks未确认保留为历史事实；第三十一批新设备在线删除已认证退休，但Broker清理仍待确认，不宣称所有退休/清理已完成。

不需要为本计划立即升级部署。S2 所需正式域名/公钥或 S3 所需隔离测试服务若未准备好，可先产生明确标识的内部候选；正式发布必须另有批准和环境准入，不能自动带过。

### 6.1 S3 最小差异验收表

| 触发差异 | 最小验收 | 已有证据如何复用 |
| --- | --- | --- |
| 仅依赖安装/插件打包变化 | Android 干净构建，S3 BLE 命中/无缓存、控制/退出；WiFiProv 若改到扫描插件则补配网→页面 | 沿用 9/18 协议/取消回归，不改 MCU |
| 构建配置/端点变化 | 显式核对WebView origin、CA/端点及原意图可见；未经迁移准入不切origin | 不能用新origin的空缓存/journal证明旧数据不存在；保持原origin优先，不复制/删除日志 |
| 库 profile/依赖版本变化 | 单 ZIP 七目标安装编译；受影响模式短真机，默认入口不含内部 probe | 未改功能模块保留 C3 因果证据；若 SDK/固件改动则不能免去受影响实物验证 |
| 登录/迁移/Key 配置变化 | 手动 WiFi 上线、状态/动作、普通重启；刷新 Key 后旧信息拒绝、新信息可用 | 复用原 session/ledger 测试；不清其他设备授权 |
| 配网/删除关联变化 | 同 ID 重配网、显式删除后重新认领，必要时一个中断恢复点 | C1 三个窗口已过，无关联变更不全部重做 |
| 开放分享/通知 | 两真实账号接受/只读/控制/撤销，Gateway＋Broker 配置一致；通知到达/点击独立验收 | 原 ACL/退休证据保留；无第二账号时标待测，skip 不能当通过 |
| 开放时间/遥测 API 声明 | 相应模式有源/无源/恢复；页面退出或租约到期停流，普通业务继续 | 不要求每个平台全组合；必须标明尚未覆盖者 |
| 开放本机定时 | 安装确认、页面退出仍执行、待授时/重启、配置竞争/掉电/权限与所有权变化 | T1/T2 新证据独立列；原 C3 不覆盖新增功能 |
| 开放云自动化 | 状态基线/字段新鲜度、目标控制、独立观察需求、TTL、停用/授权变化与崩溃去重；周期采样另评审 | A1 新证据；原路由/ledger 只能复用部分，不代表完整执行器通过 |

S3 现场网络和 COM/设备身份必须重读；文档里的 SSID、IP、COM 不是下一轮默认值。先确认专用测试设备及恢复材料；不以清授权/擦 Flash 作为普通失败的第一步。

## 7. 清理和后续功能规则

清理不是在本轮按文件名批量删除。每次删除/合并前：

1. 查真实 imports/includes、构建清单、公开 API 和测试消费者；源码未默认启用不等于未使用。
2. 有重复实现才合并到现有共享模块；不引入“协调器管理协调器”的新层。
3. 保持内存上界、取消/权限/ACK/幂等边界，补最小回归；纯搬目录不算推进功能。
4. 日志/现场 fixture/临时脚本与生产源码隔离；历史失败证据归档，不为整洁删掉。
5. 没有实际需求，不新增公共 overload、平台宏、协议字段、数据库表或无限队列。

后置功能队列不再按历史章节自动排序：BLE 大包/节奏协商 P2、LAN 正式扩展、网关多 child 与规则迁移、其他无线南向、Wio BLE 长稳、History/Bulk、复杂自动化/脚本、编辑器增强、C4 整组维护/回滚。P1 有界优化及本机定时/最小云自动化已明确纳入新队列，不再全部后置。

其中权限绕过、明文泄密配置错误、确定性内存破坏等已证实缺陷始终应优先修复；“暂停 preview 扩展”不能成为保留已知安全漏洞的理由。

## 8. 汇报与交接格式

每包只更新本文件状态表和对应专项证据，不继续叠加几十个“最新入口”。至少记录：范围/commit、改动职责、软件结果（pass/skip 分开）、实际制品、真机对象与未测项、当前阻塞、唯一下一步。

普通需求完成与质量边界分开描述：**能编译、能安装、能上线、能控制、能恢复、能发布**六件事不能互相替代。本轮不做“完成 95%”判断，也不因源码已推送宣布正式发布。

### 8.1 首批执行结果（2026-09-22）

- S1：上述三端/五插件 HEAD 未变，五个 `file:../` 依赖的本地 main/types/安装入口均存在。只证明本机可继续实施，不是新目录构建或远端 CI 闭合。
- T0a/T1a：新增 `core/TaskTiming`，只计算一次 UTC/固定偏移周历的下一次时刻，以及易失单调倒计时。无网络、堆分配、线程、存储或新公开 facade。原 `WallClock`、采样和发送调度职责不变。
- 软件：Win32 严格构建 **62/62**；新增时间测试覆盖 **26,670** 个日历组合及边界；x64 ASan 时间测试通过；ESP32-S3 编译器编译核心/测试翻译单元通过（**不是整固件或真机定时验收**）。Win32 `TaskTiming=24 B`、`TaskCountdown=8 B`，不是最终任务槽/总资源预算。App 相关回归 **42/42**，BLE 测量工具 **7/7**。
- P1：同一已绑定混合设备、同一已装 App，三组基线/高连接优先级重连。连接后认证＋Manifest＋State 的阶段合计减少 **293～407 ms**；第三组受发现延迟影响，整页反而略慢。全部保留，不能声称默认产品已提速。见[专项 §7.3](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#73-首批实施证据2026-09-22)。
- 现场：手机实际为 mostfun2026；既有 Cloud 状态 retrying、页面带 HTTP 错误。本批 BLE 真实认证/缓存 ACK/fresh State 与退出回收通过，但未发送控制命令，**不算完整控制闭环或 Cloud 对照通过**。未修改正式 App/Service，不清除缓存或绑定。

本段为首批结果；当前下一步取 §6 和最新实施结果，不把 BLE 实验直接作为默认参数上线，不重开 C3/C4。

### 8.2 第二批：先明确执行器架构（2026-09-22）

- 新增[本机执行器设计](DEVICE_V2_TASK_EXECUTOR_DESIGN.md)，明确 TaskEngine / TaskStore / TargetAdapter / CommandAdapter 分工；首版一张小型原子快照，定义、管理回执、执行进度一起提交；倒计时剩余时间不逐秒落盘。
- 到期先持久 Claim，再调用属性，再保存结果；掉电未完成结果标 Unknown，不自动重放。ESP32 复用 NVS；R4 的 260 B 上限和 Wio 独立扇区/磨损需实际接线门禁，不能宣称现在已支持任务持久化。
- 补 `WallClockSnapshot.sampleRevision`，区分接受新样本与正常 tick；未改授时 wire/安全时钟。现有 owner 管理 proof 不足的 Cloud/WiFi-only 入口保持未开放，不用普通 Control 代替。
- 验证：Win32 全套 **62/62**（13.38 s），WallClock/TaskTiming x64 ASan 通过；ESP32-S3 核心及时钟测试翻译单元严格编译、架构检查通过。测得 WallClock 40 B、快照 24 B；没有整机执行器资源或新增真机结论。
- 第二批结束时安排先实现 cursor 与原子快照 codec/fault injection；第三批进展见下节，当前下一步以 §6 为准。管理 scope/wire 未完成之前禁止公开能力，旧 timing 页面等 T2 替换后再删。

### 8.3 第三批：日历游标与原子快照底座（2026-09-22）

- `TaskCalendarCursor` 32 B：区分自然到期与校时，不补跑过期任务；时间失效后重新定基线，保留消费/跳过水位；不调用属性、不写 Flash。
- `TaskSnapshotCodec/Store`：复用既有 CBOR/属性标量校验和 CRC32；整表校验后发布；后端失败锁定且不读回脏 handle 假装提交成功。一个候选快照容纳定义与进度，不增加网络协议、文件系统依赖或平台分区。
- 固定 golden、逐位/截断/语义损坏、提交前失败/提交后报错/脏 handle 掉电三类模型通过。4/8/10 条编码占 588/1,100/1,356 B；含解码表、工作区、Store 和游标的小计为 1,304/2,456/3,032 B（Win32），不包含未来 Engine/平台/栈。
- 详细格式、实现边界与测试见[执行器设计 §10.2](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#102-第三批游标与存储底座2026-09-22)。TaskSnapshotStore 不负责 CAS、请求去重、配额或执行动作；第三批结束时它们尚未实现，后续 Engine 结果见 §8.4，不把底座测试冒充完整任务闭环。
- 验证：Win32 全量严格构建及 CTest **64/64**（14.18 s）；x64 ASan 新增两组通过；ESP32-S3 Core/Runtime 与新增两份测试翻译单元严格跨编译通过；架构及差异检查通过。未执行 UBSan/LSan，未链接整固件或新增真机结果，未修改 App/Service。

### 8.4 第四批：管理、执行与写入预算（2026-09-22）

- `runtime/TaskEngine` 复用原游标/快照，不另加事务管理器。时间和当前授权代际由 `ITaskEnvironment` 一次观察；目标通过 `ITaskTarget` 验证/调用。没有网络、平台 SDK、线程或堆分配。
- 创建/替换/删除/启停/Start 统一 CAS 和设备自算请求摘要；最近请求重传不落盘、不重启倒计时；删除不复用 ID、不影响其他任务截止点；启用不等于 Start。
- 每次最多处理一个到期候选；先持久 Claim，复查时间/目标/所有权后再调用，再持久结果。写入不确定则锁定，Claim 无结果不重放；重启倒计时 Interrupted。失败不会自动换通道/执行 owner。
- 冷启动空令牌桶限制提交平均速率和突发量；Start 有全局最小间隔，到期预留两次提交预算。额度及提交耗时上限必须由平台实测提供；不是严格滚动窗口计数，也不是 Flash 耐久性已验收。
- Win32 严格构建/CTest **65/65**（最终回归 26.43 s）；任务相关 x64 ASan **5/5**；新增执行器 **17 组行为＋4/8/10 容量**，含超过一个完整 tick 回绕后不复活 Start 冷却；ESP32-S3 Core/Runtime/执行器测试翻译单元严格编译通过。未运行 UBSan/LSan、未链接完整固件或做真机掉电测试。
- Win32 Engine 124 B、单条运行槽 48 B；含表/编码工作区/Store 的 4/8/10 条小计 **1,432/2,648/3,256 B**，另计平台缓存、命令对象及调用栈。详细边界见[执行器设计 §10.3](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#103-第四批纯-taskengine2026-09-22)。
- 未改 App/Service、未启用公开任务 API。合并了游标与执行前复查的连续时间判断，并复用快照记录校验，未复制旧 JSON 定时器。尚有消费者的旧页面保留至 T2 真正替换。

### 8.5 第五批：真实属性、时间与 ESP32 任务组合（2026-09-22）

- `Property<T>` 的普通/checked 回调合并为一条类型化应用函数，网络控制和本机任务复用，不伪造 RxContext，不自动 report。未增加每个 Property 或 EndpointSlot 的 RAM 字段，也未改变现有 onWrite/begin 用法。
- 新增内部 `TaskPropertyTarget` 显式允许列表：验证稳定 key、真实 Client 绑定、属性读写/type/数值约束，以及当前网络回调仍属于该 Property；任意低层 onCommand 不自动获得任务执行能力。新增准入只有内部组合消费者，不宣告网络任务权限。
- `TimeSyncService::observeTime` 与 `TaskTimeEnvironment` 同次采样 tick/UTC；无有效 UTC 时倒计时可继续，未配置时钟/无当前可信 scope 则拒绝。没有第二个 RTC/授时服务；scope 生产来源仍待 T0b-2，不使用测试 epoch 作为产品证明。
- `Esp32TaskModule<N>` 复用原 NVS 端口，固定独立 `bl_v2_tasks/snapshot`、按容量设置上限；不格式化 NVS、不改凭据布局，默认产品不实例化。真实端口经主机 NVS stub 验证了 4/8/10 满表写入/恢复、身份/网络区不变、提交失败锁定和启动错误；不是实际断电测量。
- 验证：严格全量构建/CTest **66/66**（10.74 s）；受影响 API/授时/执行器/组合 ASan **4/4**（0.55 s）。ESP32-S3 stock 3.3.11 工具链严格编译 API/Runtime、真实 SDK 头下的 NVS 端口与新测试翻译单元通过；只生成 `.o`。架构/差异检查通过。
- Win32 内部 Module 4/8/10 条为 **1,512/2,728/3,336 B**，绑定 12 B/属性、Target 20 B；不含 TimeSyncService、NVS 内部缓存、命令对象及栈。完整资源/真实写耗时未验收，T1b-3 不标全完成。详见[执行器 §10.4](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#104-第五批属性时间和-esp32-内部组合2026-09-22)。
- 现场只记录用户已把电脑/手机切换至“带翅膀的蜗牛”；本批未操作网络或硬件，也未确认 MCU 同网。后续真机仍须核对，不把 WiFi 密码写入方案/库。

### 8.6 第六批：链接资源、管理消息预算与 COM3 准入（2026-09-22）

- 采用一个 compile-only sketch 和 runner 比较同源、同 WiFi/授时图的 0/4/8/10 容量；不添加正式任务 facade，不把测试专用 scope 变成 owner。复用现有 Product extension，不放开私有属性绑定；容量只改变 sketch 模板实例，不改变所有库翻译单元的宏或 ABI。结果统一见[执行器 §10.5](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#105-第六批整固件探针管理预算与硬件准入2026-09-22)。
- 四档均已完整编译/链接并过符号准入。4/8/10 条相对同一未启用基线的 Flash 增量 **11,448/11,452/11,464 B**、静态 RAM 增量 **1,624/2,840/3,448 B**；未启用档未保留任务图。空表与满表使用同一固定分配；尚未测 SDK NVS 堆缓存、调用栈及管理适配器，不把静态数值当整机峰值或 R4/Wio 结果。
- 新增真实 CBOR/Route/Delivery 包长回归：最坏单包签名操作 **295/307 B**，超过 Small 256 B；候选 Prepare **182 B**、引用摘要的 Grant **223/235 B**。优先评审有界两阶段管理，不扩所有设备 buffers。尚未实现生产 wire、授权验证或三端 golden；不是协议闭环通过。
- 权限源码审查确认 Cloud 的 authenticated 不等于用户 owner、BLE 握手材料在 prepare 后已清理、两种 accessEpoch 来源不能直接当同一所有权代际。后续在已有身份/会话生命周期中接最小已验证元数据与 scope 映射，不新建账号/会话系统。三端 kind 占用也必须共同核对，不从单端空位分配。
- 本地回归 **67/67**，任务相关 x64 ASan **3/3**，资源证据解析 **3/3**，Arduino 发布结构 **400 files**、发布工具 **17/17**；未运行 UBSan/LSan，未代表远端 CI。新增工具不进入用户 Arduino ZIP，正式 API/示例保持不变。
- 用户已授权 COM3 新增 XIAO ESP32S3 做下一批真实 NVS 测试；必须先核对身份、备份并校验恢复材料，测试后恢复原固件/授权，COM11/COM12 不动。当前仅枚举到端口，未重置、烧录、清授权、写真实 NVS 或做断电测试；未改 App/Service、网络、未暂存/提交/推送。

### 8.7 第七批：COM3 NVS 与恢复门禁（2026-09-22）

本节保留第七批结束时的历史状态；断电及恢复结果见 §8.8，不再是当前待拔插事项。

- 先核对身份、读取 8 MB 原 Flash 并进行设备端全范围摘要比对；保留本批完整备份及 `0x9000..0x14ffff` 恢复区域。原分区实际为较小的 app0/app1，不覆盖分区表或 bootloader，只刷入能够容纳的测试应用并校验。备份含授权材料，仅留本地。
- 复用原 TaskEngine、真实 Property 回调和 ESP32 NVS 端口，隔离 `bl_task_gate/snapshot`；测试计时/断点只在 tests，不改正式库、不造生产 owner 来源、不引入网络/任务线程。原 `bl_v2_tasks` 和凭据入口未用于测试。
- 4/8/10 条实际表长（588/1,100/1,356 B）写入、执行、复位读取通过；4 条的替换/启停通过。相同 Start 当场及重启后重放不再写/执行；100 次 idle 不写；满 10 条拒绝新增且修订不变。
- Armed、Claim 前、回调后/结果前的**有供电复位**分别得到 Interrupted、Interrupted、Claimed/Unknown，后续 replay/recover/idle 不补跑。COM3 现已停在 Claim 后/回调前，等待用户只断开该板所有供电再接回；物理断电结果和原固件恢复仍待完成。
- 48 次 snapshot replace 的样本为 **1.390～54.707 ms**；4 次完整执行 poll **53.155～64.013 ms**（含测试日志）。隔离无网络固件最小堆 347,732 B、loop 栈最小剩余5,108 B，不是联网产品峰值。长尾不能用平均值掩盖；未证明 NVS 空间压力/耐久上界，不设最终写入默认。
- 回归 **67/67**（23.32 s），驱动离线 **4/4**，发布工具 **17/17**，单库结构400 files、架构检查通过；核心生产源码本批未改。完整边界、恢复材料、当前停止点见[执行器 §10.6](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#106-第七批com3-隔离-nvs-与复位恢复2026-09-22)。未改 App/Service、网络或其他板、未暂存/提交/推送。

### 8.8 第八批：断电收尾、恢复现场与退休清理（2026-09-22）

- COM3 拔插后先核对 USB 身份，inspect/replay/recover/idle 均通过：10 条记录、revision=15、第一条 start=5/Claimed，callbacks=0、writes=0。未知结果不重放；仅为该窗口补物理证据，不改写先前有供电复位记录。
- 校验备份后仅恢复原 `0x9000..0x14ffff` 的 NVS/OTA/app0，启动前整片 8 MB 摘要与测试前完全一致。测试 namespace 随原 NVS 恢复移除，原授权字节恢复而非清除授权。两次被动串口观察无输出，不能宣称已重新联网。
- 修正生命周期缺口：保留 RAM 立即停止的 `retire()`，新增 `clearRetired()` 经原 Store/原子 blob 清理；仅成功进入 Cleared 终态，重复不写，失败锁定，回调内不嵌套清理。没有新格式、持久 key、线程或会话表，fixture 也去掉绕过 Engine 直接清 key 的旧路径。
- 这是内部底座，**尚未接入正式产品移除/重置**，不能宣称生产解绑已清理任务。具体顺序和失败重启边界见[执行器 §8.4](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#84-生命周期顺序与本批落地的最小清理接口)。
- Win32 构建/CTest **67/67**（最终 24.28 s），x64 ASan **3/3**（最终 0.78 s），驱动 **4/4**、发布工具 **17/17**、架构检查通过；ASan 首次缺 DLL，补本进程工具链 PATH 后通过。新增清理实现没有再刷 COM3；S3 编译证据及剩余项见[执行器 §10.7](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#107-第八批实际断电恢复现场还原与退休清理2026-09-22)。未改 App/Service/网络/其他板、未暂存或推送。

### 8.9 第九批：任务的可选产品生命周期（2026-09-22）

- `TaskProductLifecycle<Module>` 复用已有 Product 生命周期：通信 poll 在先，任务最多一次；stop/reconnect/resetNetwork 保留表，resetAccess 先停任务和通信，再清任务，最后进入原产品的授权清理。没有复制 WiFi/BLE composition，没有新线程或默认 facade 字段。
- Target 改为可在 attach 时绑定原 Client，拒绝跨 Client 重绑；删掉资源探针的 `ProbeExtension/attachedClient/taskModule()` 绕路和 loop 中第二次任务 poll。原有绑定构造仍有 NVS fixture/host 消费者，保留，不以清理之名删除有效测试入口。
- 新增清理前/后失败、根提交前/后失败、重复 reset、重新认领后再次 reset、网络/属性回调中 reset、scope 本轮失效等门禁。清任务失败不动根；清根失败不假定回滚。未启用/损坏任务不阻断普通控制，但不能跳过未验证存储进行清根。
- 32 位可选生命周期 **24 B**；原 Product 无变更，4/8/10 Module **1,512/2,728/3,336 B** 和 Target **20 B** 不变。完整固件与最终测试结果统一见[执行器 §10.8](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#108-第九批可选产品生命周期与回归2026-09-22)。
- 最终主机回归 **67/67**，相关 ASan **3/3**；S3 stock 3.3.11 同源 0/4/8/10 四档完整编译、符号和源摘要检查全部通过。启用档静态 RAM 增量 **1,584/2,800/3,408 B**，包括本批组合；未启用档无任务图。没有把静态数值当作联网运行峰值或公开功能准入。
- **剩余边界**：默认产品未实例化模块；手动 WiFi 仍不提供 AccessReset；Controller Admin/Shared 的变化不是物理 reset。可信 scope、网络 pending/proof、服务器退休和故障启动恢复尚未闭合，不能宣称公开任务或设备解绑全流程已通过。本批不打开任何串口、不刷写/清授权、不动 App/Service/网络/Git index。

### 8.10 第十批：统一任务管理 Grant（2026-09-23）

- 修正早期 Cloud/Direct 双准入分支：首版统一服务器 owner 检查后的签名授权，BLE App 在线取票再转发；普通控制不变，安装后离线执行仍受原时间合同约束。选用独立 128-bit task scope，不让两套 accessEpoch 数字碰巧相等就继承任务。生命周期表与 tradeoff 见[执行器 §8.6](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#86-第十批统一签名管理与独立-task-scope2026-09-23)。
- 已实现 Grant 的固定 CBOR 编解码、流式 SHA-256、复用服务器验签接口及精确 pending binding 比对；最大 body **176 B**、路由 BBP 帧 **235 B**。清理包长测试中未采用的 u32/单包候选重复编码。不新增硬件依赖、会话状态或默认任务资源。
- Win32 全量 **68/68**，相关 ASan **5/5**；独立 Node Ed25519/P-256 向量验签/篡改拒绝、架构与发布工具通过；实际责任单元 Xtensa C++11 严格交叉编译通过。具体范围见[执行器 §10.9](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#109-第十批grant-合同流式摘要与拒绝门禁2026-09-23)，不等于整固件/真机/三端验签已完成。
- 尚未持久化新 scope、建立 root 连续性、消费挑战或接网络管理；旧 Engine trusted seam 仍禁止外部调用。没有重复 COM3 测试，未动 App/Service/网络/Git index；当前唯一下一步见 §6。

### 8.11 第十一批：持久授权绑定与恢复边界（2026-09-23）

- 用一份 64 B 自有 binding 替换旧 u32 epoch：instance16/scope16/rootDigest32 与任务定义、消费水位、最近回执保存在同一原子快照。schema 2 每表增加56 B，4/8/10 条 **644/1,156/1,412 B**；拒绝旧 schema，不添加整数兼容或自动格式化。
- 区分 Pending 与明确 Retired；任务可等待后恢复，恢复不补跑日历间隙、不重开倒计时。可信同 owner 根 CAS 保留定义和管理修订/去重；当前绑定不同则不能执行/读取，存储故障仍终止。原 Product 继续轮询等待态，不重建 NVS/执行器。
- 修正“提交已成功、复查时授权失效”窗口：同步内部表头/数量与已提交记录，再停用外部读取/回执；保持单表不增加回滚副本。写入超时与 Pending 同时发生仍 Fault，不用可恢复等待掩盖资源风险。
- 更新所有 host/compile-only/NVS fixture 消费者与 golden，清除任务旧 scope 入参。实际 Grant wire/普通通信/默认 Arduino API 不变，未刷板或擦授权。软件/编译和资源证据统一见[执行器 §10.10](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1010-第十一批持久绑定等待恢复与换根门禁2026-09-23)。
- Win32 全量回归 **68/68**、相关 ASan **5/5**；stock S3 3.3.11 同源0/4/8/10整固件静态资源及符号门禁全部通过。4/8/10条静态 RAM 增量 **1,704/2,920/3,528 B**，禁用档无任务图；隔离 schema 2 NVS 固件仅编译未刷写。
- **该批剩余**：上述 binding 是可信内部元数据，不是签名证明；当时根指纹、初始化/换根签发验证、pending 与三端安装尚未接线。后续进展见 §8.12；联网运行堆栈/共存及 schema 2 实物验证不能沿用旧值冒充完成。

### 8.12 第十二批：根来源与初始化/换根签名子合同（2026-09-23）

- 原凭据源读取并计算有域摘要：WiFi 取 DeviceKey、BLE 取已安装 PlatformAccess Admin、混合固定同时取两根。Shared 改动不影响摘要，缺根/错误角色不降级。没有复制凭据表或每轮 poll 读 Flash。
- 跨端审查发现 Service 不持有 Admin raw secret，已修正为复用现有 Admin 指纹。当前版本1兼容；后续高版本轮换须同步 App 指纹版本及 Service 已验证元数据，不能直接签 App 自报的 root。
- 初始化/换根操作 codec/摘要及验证接缝复用 Grant v1；完整路由帧 **147/180 B**，Grant仍235 B。无新签名封套、public kind/feature、存储 key或默认 API。验签函数不消费挑战、不执行 Engine，不能冒充生产完整授权。
- Win32 **68/68**、ASan相关 **4/4**；独立 Node 根/操作 golden与实际签名/篡改拒绝通过；架构、406文件单库边界、发布工具 **17/17** 通过。S3严格 `.o` 交叉编译通过，非整固件/真机资源门禁。证据及界限见[执行器 §10.11](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1011-第十二批根来源初始化换根与跨端校验2026-09-23)。
- 下一步仅接已验证根缓存与统一 pending/一般 Prepare/挑战消费，然后三端签发/查询回执/安装闭环。未改 App/Service/网络、未操作开发板、未暂存/提交/推送。

### 8.13 第十三批：根缓存与统一有界管理流程（2026-09-23）

- `TaskAuthorityCache` 只缓存身份/根承诺与已确认 scope；独立读根后核对 Engine 已验证表头才离线恢复。根变更前 invalidate，provider 调用期间代际变化拒绝发布，明确退休为终态；poll/恢复核对均不重复读取 Flash。
- 一份 `TaskManagementSession` 承接 Initialize/Rebind/Mutate/Read，不建四套队列。安全随机 nonce、单调 TTL、挑战冷却、单次验签消费、通道取消、提交前后根代际复查均已实现；底层写后未知仍按 Engine CAS/去重处理，不自动再启动倒计时。
- 类型化修改编码/摘要复用 Engine 校验；读取按 tableRevision＋单条 index 或仅表元数据授权。删除包长测试的手写重复 Prepare 编码，实际修改路由帧最大仍 **182 B**，Grant **235 B**。
- Win32 严格全量 **69/69**，相关 x64 ASan **5/5**；新增跨语言修改/读取摘要 golden、原 Ed25519/P-256 oracle、架构、单库408文件边界、发布工具 **17/17** 通过。S3 stock3.3.11 两个责任单元严格交叉编译通过。缓存 **76 B**、单笔槽 **336 B**（Win32 ABI；不是联网总 RAM/栈），未默认实例化。
- 软件闭环覆盖安装、退出管理连接后执行、Start 丢回执重试不重开、换根、恢复、过期/重放/重入拒绝，以及写后根失效/超时。此处验签器为精确摘要测试替身；真实算法由独立 oracle 覆盖，不能合并声称 MCU 三端真实验签已过。详见执行器 §8.9/§10.12。
- 未改普通产品、App/Service、存储格式或网络；没有刷写/擦授权。网络 adapter、生产生命周期事件、可信 Installed 回执及真实三端管理仍待接线，下一步以 §6 为准。

### 8.14 第十四批：严格管理 wire 与事务绑定回执（2026-09-23）

- 提取无状态 `TaskManagementContract`，从 Session 移走重复责任；修改/读取摘要、编码/解码复用同一字段实现与 Engine/Store 校验，解码结果自有，不借用 RX 缓冲。仍编入原责任单元，不新增线程、堆、队列、凭据库或默认产品资源。
- 完成一般 Prepare/Read、Challenge 和授权单条查询结果；HMAC 绑定完整设备挑战与结果，拒绝旧 nonce、错根/角色/请求以及正文篡改。Session 成功提交才返回实际已消费的上下文；编码失败不假定已落盘修改回滚，重试沿用原 CAS/去重。
- 明确两类验证者和限制：DeviceKey→Service，Admin→owner App；Service 只有 Admin 指纹时只能记录 App 确认，不能冒充设备独立证明。明文 MQTT 的任务内容不保密，HMAC 不改变默认传输策略；未扩展一般 Cloud Frame Authentication。
- 既有69个测试目标内增加截断、规范性、数值越界、字段篡改、输出清空及8组跨语言结果向量；Win32 **69/69**、相关 ASan **5/5**、独立 Node oracle、架构/410文件发布边界、发布工具 **17/17** 通过。挑战路由帧 **197 B**、带认证单条记录最大 **236 B**，均在256 B预算内；修改182 B、Grant235 B不变。
- S3 stock3.3.11 实际责任单元严格交叉编译通过；缓存76 B/Session336 B（Win32）不变。调用栈和整固件/联网共存未复测，证据与精确边界见[执行器 §10.13](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1013-第十四批严格-wire认证回执与跨语言门禁2026-09-23)。本批未改 App/Service、操作硬件/网络或 Git index；真实消费者/签发/产品生命周期仍待接线，不宣称用户已可安装任务。

### 8.15 第十五批：来源绑定适配器、实际密钥回执与生命周期（2026-09-23）

- `TaskCommandAdapter` 共用唯一 Session，接收严格编码的 Authority/Mutation/Read/Commit。只保存当前事务来源及不复用的本地句柄，无 RX/回执队列；来源包含 transport、连接代际、session、路由类型/peerId 和由产品固定的密钥角色。错来源不消耗合法 pending；匹配断连/整通道退休、停止、超时、根变化均取消。
- 实际回执从固定 DeviceKey/Admin 来源各读取一次，完整重算 root 后才签名；复用原加载/摘要校验，临时密钥统一清零，不重复读根再读签名密钥。缺根不降级，Shared 不冒充 Admin。提交后取密钥失败、根改变或断连不发布成功回执，已经落盘的修改不回滚。
- 原可选 `TaskProductLifecycle` 注入同一 adapter，负责启动恢复、轮询超时、停止取消和 resetAccess 先退休任务再清根；不复制平台产品。普通 Product/API 不变，可选生命周期自身从24 B到28 B；adapter64 B、缓存76 B、Session336 B（Win32），不是整机动态峰值。
- Win32全量 **69/69**、相关ASan **5/5**、跨语言golden、架构/412文件发布边界、发布工具 **17/17** 通过。严格S3责任单元编译通过；实际BBP入口、平台根写入通知和真实签发/安装仍未完成。完整证据见[执行器 §10.14](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1014-第十五批来源适配实际根回执与生命周期门禁2026-09-23)。没有刷板、改网络、改App/Service或Git index。
- 同源S3 0/4/8/10完整链接、扩展符号和源码摘要门禁通过。禁用档Flash941,160 B/静态RAM51,304 B不变；各启用档比此前增加8 B静态RAM和3,388 B Flash。探针未实例化完整管理对象，不能当作生产管理/联网堆栈峰值；完整数值与工件见上述证据节。

### 8.16 第十六批：可选 WiFi BBP 入口与共享 carrier（2026-09-23）

- `TaskCloudTransport` 接现有 MQTT/Runtime，而非再增加一条连接或 Task Session。唯一注入的212 B工作区在请求、回执之间复用；RX回调只复制，退出原生poll后才解码/验签/写Flash/签回执。对象Win32为96 B，默认产品不实例化。
- Runtime只允许明确安装的物理云扩展宣告bit25；Hello成功协商且双方frame预算足够才收任务。复用MQTT真实connectionGeneration和session断连；重协商/停止/离线取消pending，收包后的根代际变化禁止执行，提交期间变化抑制响应。一次WouldBlock只重发已编码回执，不重进Engine；缓冲绝对有效期1秒，过期取消，不建可靠消息队列。
- 共享包同步 `TaskManagement=0x37`、`FeatureTaskManagement=1<<25` 和 `[1,operation,bstr(inner)]`。失败只是相关联的非认证提示，不证明未写盘，更不能触发擦表/重新Start。物理任务帧实际最大Prepare138 B、Grant191 B、Challenge153 B、单条认证结果192 B；接受预算上限212 B。不含MQTT/TCP/IP，历史Route估算不是本轮实际载体。
- Broker识别新kind/feature但不协商任务业务，App Route/Delivery和虚拟子设备仍拒绝。避免扩展设备Hello被误当非法而中断普通接入；没有在签发、scope持久化和回执验证缺席时提前开放网络命令。App/现有页面未改。
- 严格主机全量69/69、相关ASan6/6；共享包32/32、Broker相关26/26；独立密码学oracle、架构/413文件单库边界及发布工具17/17通过。生产S3 Protocol/Runtime/Controller责任单元及真实ManagedMqtt模板严格交叉编译通过；未链接新整固件或刷板。精确证据与未闭合项见执行器§10.15，Service下一批见§6链接。
- 回归修复的是feature兼容与新代码边界，没有复制旧JSON定时入口。移除adapter已不使用的include，回执角色在回调前取稳定快照。没有部署/迁移、改网络、清授权或操作Git index。

### 8.17 第十七批：Service typed任务合同、真实签名与认证回执（2026-09-23）

- Service共享包实现Authority/Mutation/Read/Challenge/Grant的严格codec及完整摘要，HMAC结果验证覆盖DeviceKey/Admin角色与四种结果；Service消费角色固定DeviceKey。解码返回自有字段，保留标量原始宽度，不把任意CBOR/客户端hash直接送去签名。
- 同步MCU的时间范围、星期偏移、JS安全整数/有限浮点、记录进度、长度与容量限制；补齐既有CBOR组合器遗漏的bool/null/负数/f32/f64，不新增第三方依赖或第二套CBOR引擎。
- Controller/Task共用密钥环和签名路径，清除Controller原地重复的实现；Task profile固定来自服务端，真实Ed25519签digest、P-256对transcript哈希一次并low-S规范化。已有Controller对外接口/错误类型保留。
- 纯操作绑定层校验完整challenge与可信instance/scope/root，重算完整操作摘要；HMAC通过后仍校验kind/CAS/taskId/查询index。该层不是owner鉴权，不为Map或调用方自报authority背书，当前没有HTTP签发入口。
- MCU既有golden逐字节对照、全字节篡改与截断/错误角色/错误操作测试通过；Gateway/Broker构建和相关软件回归通过。精确验证与限制见执行器§10.16及Service§6。未修改App/设备源码、迁移部署、刷板或Git index。
- 下一步S-T1b：受控短事务中的owner/当前session/actual root读取、独立scope和原ID请求账本；该步完成前Broker任务仍不协商。其后依次S-T2、D-T3、T2，不转回网关扩展或自动化。

### 8.18 第十八批：持久权威、完整意图与认证结果事务（2026-09-23）

- 新增窄任务repository和migration039的scope/request/attempt三表，复用原DeviceKey vault、数据库事务及签名器；不新增raw secret、MQTT连接、线程或云端执行器。默认空白名单，只准入手动WiFi固定Key测试根，已注册Admin的混合根明确拒绝；LAN是否启用不作为任务权限。
- owner/tenant/key/指定session/route受锁复核，完整意图与challenge重算绑定，原ID不同内容拒绝；只在COMMIT成功后返回签票或verified结果。丢回执保留原ID、新challenge恢复；Key刷新需显式换根，owner移除/转移同事务退休，不把离线设备说成已停任务。
- 新增16项真实隔离PostgreSQL子门禁；连同登录、LocalAccess、Direct Admin reset、解绑、任务密码学共85/85通过、0 skip。共享协议41/41，Broker相关26/26；共享包/Gateway/Broker构建通过。设备回执为真实secret测试端生成，不是真机执行证据。
- 本批无App/设备源码、网络、硬件、常驻服务数据和Git index操作。一次性测试库不代表业务库已迁移。新入口、真实连接绑定、产品根事件、公开前账本保留策略及三端真机仍待完成；准确边界见[Service§7](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#7-第十八批持久权威原id账本与提交边界)，下一步按§6的S-T2执行。

### 8.19 第十九批：S-T2a封闭物理任务交换（2026-09-23）

- 复用原Aedes客户端、`BoundedExchange`和Service broker-client；独立`TaskCarrier`处理连接代际/有界收发，不包含业务权限、签名或执行器。默认空维护白名单，实际Hello/frame预算达标才允许试点；App/Hub/虚拟子设备不开放。
- Prepare/Commit使用同一物理连接上下文，错proof/Key版本/代际、重复候选、撤权、断连及迟到响应拒绝；QoS0单次发送不自动重发，单次3秒、每连接1笔、全局8笔、每秒4次。持久事务仍由Service管理。
- 新传输8项与Hello准入3项通过，相关回归114/114、0 skip，Broker/Gateway构建及原Device V2-only脚本通过。真实网络测试使用正式Broker认证/分发/HTTP路由与Service端口，MCU仅为协议测试端；尚未串接真实任务仓库、签名/Engine，不能合并声明安装成功。
- 无新依赖/wire迁移、App/设备源码变更、现场部署/刷板或Git操作。下一步S-T2b明确见§6和[Service§8](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#8-第十九批s-t2a物理连接代际与真实网络交换)。

### 8.20 第二十批：S-T2b内部完整管理链路（2026-09-23）

- 新增一个薄管理调用，复用原任务仓库/签名器/Broker端口；提取共享owner/route准入，不新增用户协议、MQTT连接或执行器。migration040固定attempt连接代际，旧无代际在途材料保留为unknown。
- 并发同ID不再发Prepare/Commit；签发前后连接、Key、owner、route变动拒绝。取消/丢回执不提前释放持久期限、不宣称回滚；原ID、完整意图及新挑战恢复。任务局部SQL等待有界，不更改整体数据库策略。
- 最终任务41/41、0 skip，含真实PG及正式HTTP/MQTT/签名/HMAC集成；Broker相关114/114；Gateway/Broker构建通过。独立测试端原ID Start恢复执行计数为1，但不是MCU Flash/TaskEngine真机证据。详细实现、组合回归与边界只维护在[Service§9](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#9-第二十批s-t2b有界管理调用与原id恢复)。
- 无App/设备源码、现场服务数据库、硬件/网络和Git index修改。默认关闭，下一步D-T3设备产品与根事件；账本回收和T2仍明确待做，不插入服务器整体维护。

### 8.21 第二十一批：D-T3a可选WiFi产品接线（2026-09-23）

- ESP32手动WiFi通过一个小型内部extension选择原任务生命周期和云载体；不复制WiFi状态机、不新增Client/MQTT、时钟、任务线程或凭据库。默认关闭，既有begin/run/组件用法不变；任务与LAN/LocalAccess组合明确拒绝，其他平台不能误开启此ESP32试点。
- TaskTimeEnvironment由具体TimeSyncService依赖改为同一个ITimeSync的coherent observation，晚绑定现有enableTime服务；无实现/未绑定时不执行任务。目标只借用显式属性白名单，不给普通组件增加任务状态。
- 修正上一批“Key成功写入/变化/清除接线”的笼统表述：手动Key来自Sketch，产品只在启动配置一次，没有运行时EEPROM清Key路径。本批覆盖同Key/instance重新加载、换Key暂停等待签名换根、无根拒绝、不擦旧表；尚未实现可信服务器退休的持久闭环。
- 资源、回归、复现入口及未测范围统一记录在执行器§10.20；不将编译/主机测试冒充COM3任务安装、执行或NVS时延准入。本批未改App/Service业务源码、现场网络/设备/数据库或Git index。

### 8.22 第二十二批：D-T3b-1持久退休底座（2026-09-23）

- 在原TaskSnapshot/Store中增加132 B终态标记，不建第二个NVS key、回执队列或凭据槽。活动schema 2不变；schema 3仅终态、零任务，旧读取器拒绝，不把退休当空表自动初始化。
- Engine可信内部退休先停执行，再原子落盘；精确请求重试不写Flash。失败锁住原Store，不读回猜成功；新运行时按实际完整快照恢复，旧表未替换就不能声称退休已持久完成。
- 覆盖截断/破坏、写前/写后失败、编辑额度耗尽、重入、旧Key/换Key重启，以及退休前已签出的Read/Mutation/Rebind失效。原物理清除路径有真实消费者，保留且与网络退休分开；清理一处过时“未分配BBP kind”注释。
- 回归与静态资源证据统一见执行器§10.21。未增加退休wire命令/公开API，不改App/Service业务源码或现场数据库，不刷板、不清授权、不暂存/提交/推送；D-T3/T2仍未整体完成。

### 8.23 第二十三批：D-T3b-2a签名退休与终态查询（2026-09-23）

- 复用唯一Session、Grant、根来源、结果HMAC与MQTT载体，新增Retire(7)/RetirementRead(8)。精确命中当前instance/scope/root；查询返回原退休ID，普通任务权限不恢复；同ID重试和查询不重复写Flash。旧操作号和212 B上限不变，不新增线程、密钥或持久队列。
- MCU/Service严格codec、独立向量、完整意图/回执绑定及Broker请求/响应分类同步；删除重复TaskCommand枚举，以唯一wire定义为准。新增操作不能借数字大于4混入上行响应。
- 冻结首版为“撤权前有界尝试、失联云撤权仍生效、本地状态保持未知、重用需显式维护重置”。Service普通execute/账本仍只接受原0..2，owner删除尚未调用退休；不把纯合同和传输接通说成全链路完成。
- 具体测试、字节/资源与未测范围集中见执行器§10.22。没有App变更、现场数据库迁移、服务部署、刷板/清授权或Git暂存/提交/推送。下一步唯一入口为上文D-T3b-2b，不再展开新的恢复协议。

### 8.24 第二十四批：D-T3b-2b持久退休与删除前编排（2026-09-23）

- 沿原scope/request/attempt三表新增migration041，不重写旧迁移、不加凭据或回执队列。`retiring`冻结普通管理，`retirement_verified_at`单独表示认证设备证明；云端`retired`不等于MCU已停。
- 原ID退休重试与新鲜Query仍核对当前owner/Key/session/连接；Query证明必须匹配原退休ID。满普通64请求账本仅预留1个退休＋1个查询，不删未知记录或通过换scope绕过容量；公开前回收策略仍待做。
- 原删除入口先鉴权、持久冻结，SQL外一次有界尝试，再执行原云端撤权。离线/丢结果/坏证明仍撤权并保留本地未知；不存在撤权后特权登录或旧Key查询。普通无任务设备不创建scope，不改变原删除响应。
- 三包构建及最终相关回归177/177、0 skip，具体范围和计数见Service§11。使用临时PG、正式HTTP/MQTT、实际签名和独立回执端；不是MCU Flash或三端安装证据。本批不改App/设备源码，不动现有服务库、网络、板卡或Git index。
- D-T3/T2不可整体标完成。下一步唯一入口见§6：专用COM3真实产品共存，完成后回到用户任务闭环，不继续扩服务器细节。

### 8.25 第二十五批：D-T3b-3真实产品、回执恢复与有限测量（2026-09-23）

- 专用COM3在mostfun2026完成真实DeviceKey登录、Broker/MQTT/Hello、4槽TaskEngine签名安装、3秒倒计时及UTC任务跨软件重启执行；属性/按钮ACK、State和授时保持原链路。没有新增公开API或第二条MQTT。
- 在真实Commit响应后注入一次Service侧丢回执：scope冻结、重启后原退休ID恢复与新鲜Query验真、task snapshot零新增写入、到期不复活；原owner删除结果与撤权后拒绝均验证。无真实App账号/页面或新物理断电证据。
- 测量工具只放tests/tools；提取既有Broker fixture，删除重复组装，并用真实注册HTTP路径补测试。修正工具漏传HTTP超时及测试ID格式；未放宽生产检查。最终任务软件回归61/61、0 skip。
- Flash988,604 B、静态RAM54,000 B；样本snapshot写入最大46.305 ms，Blinker.run调用最大298.324 ms，二者不能互相归因。完整资源、失败/限制及原固件恢复见执行器§10.24；默认开关不变、不作寿命/SDK全栈/长期时延保证。
- 下一步严格按§6的账本与试点policy→T2→A1。COM11/COM12未操作，没有部署现有服务、改App或Git暂存/提交/推送。

### 8.26 第二十六批：T2账本回收与共享试点policy（2026-09-23）

- 复用原scope/request/attempt及短事务，普通Read过期可回收，已确认Mutation保留24小时后先提升持久CAS门槛再回收。未知写入、nonce/重试额度、权威意图和退休证明保留；旧Start同/新ID均不得绕过门槛。migration042解除任务对历史session的额外外键，不放松当前session鉴权，也不扩全局Auth清理。
- 删除内部真机示例重复的16写/分钟、1秒Start、1秒提交宽松配置，改用一份私有T2 policy头，host测试/生成工具共用。容量10、每6秒补1写、冷启动空桶，全局Start冷却30秒、commit故障阈值100ms；不加入生产默认或宣称寿命/实时保证。
- 任务服务最终71/71、0 skip；Win32全量69/69。覆盖迁移保留、查询TTL/容量恢复、时钟/无权回收、旧CAS在发消息前拒绝、未知写保留、提交回滚，以及设备冷启动/四任务到期/超时不执行。详细边界统一见Service§13及执行器§10.25。
- 无App/设备生产src/wire变更，无刷板/清授权、业务库升级或Git index操作。旧COM3测量不冒充新policy实测；下一步仅§6的T2用户闭环，不继续服务器维护或A1扩展。

### 8.27 第二十七批：T2a owner HTTP与App原请求恢复（2026-09-23）

- 新增窄JSON管理接口，复用原用户认证、限流和唯一任务服务。账号从认证上下文取得；初始化、查询、创建/替换/移除/启用/Start均经严格类型转换，App不提供scope/root/grant。响应仅投影安全字段，历史认证确认返回空result，不伪造当前状态。设备仍用原BBP/2/CBOR，不复活Legacy JSON设备链路。
- App新增独立任务合同及HTTP/恢复客户端，复用原HttpClient和鉴权拦截器。写前保存账号隔离的原ID/完整意图；取消、超时、账号变化、损坏响应保留未知写；新鲜Read不删除旧写。不新建MQTT连接、定时线程或通用状态库。
- Service组合77/77、0 skip，含真实临时PostgreSQL、HTTP→Broker/MQTT和Start丢回执原ID恢复执行一次；生产路由接线1/1，共享协议42/42，Gateway/共享包构建通过。App完整spec TypeScript检查及Angular相关50/50通过。身份解析和MCU为测试端，未冒充真实用户/硬件结果。
- 发现并冻结页面前置：普通可写属性不等于可调度属性，当前固件白名单没有对外枚举；T2b先补最小能力识别，再替换旧TimerService/页面消费者。删除旧的“服务器每分钟扫描cron”页面设计文档，保留尚未被替换的运行时代码，避免提前断开消费者。
- 本批不改设备生产src、不部署/迁移业务库、不操作板卡或Git index。未完成App页面、真实存储重启、COM3新policy验收或T2整体闭环；详细下一步仅见§6。

### 8.28 第二十八批：T2b-1真实目标目录（2026-09-23）

- 固件显式TaskPropertyTarget白名单成为唯一目录来源；流式canonical hash锁定最多32目标/分页、实际1..10槽容量，普通Manifest可写不自动成为可调度。描述不调用用户回调、不写Flash，不复制属性约束模型。
- 复用原认证Read/Grant/HMAC、Service账本和HTTP客户端；仅增加严格目录选择器/结果，原表Read字节不变。App逐页验证fresh、指纹、数量、容量/重复key和账号代际，查询不覆盖未决写，不自动初始化、缓存、重试或换协议。
- 完整目录metadata/最长目标帧114/183 B，小于212 B。Win32 Session仍336 B；临时Read/Result为60/288 B，不是常驻每任务增长。S3 stock3.3.11真实WiFi产品0/4/8/10槽编译4/4通过，静态RAM及product大小相对上一基线均不增长，启用档Flash约增1.8 KB；不是运行峰值或新真机证据。
- 最终设备host69/69、架构检查；Service79/79、共享44/44、wiring1/1，App相关52/52及完整spec TypeScript通过，均无skip。独立oracle/C++/Service精确向量与最大预算通过；软件MQTT设备oracle不等于MCU。
- 同步三端合同、修正API文档中误放在任务节的owner移除说明。没有第二套管理服务/凭据/迁移；保留仍有消费者的旧TimerService，下一批替换后再清除。一次性PG测试容器及临时数据已清理，既有业务数据、板卡、授权、网络和Git index未动。
- 当前唯一下一步为§6的**T2b-2原任务页面**，随后T2c专用COM3安装→退出→执行→查询及新policy验证。T2整体未完成，不转入A1或服务维护。完整合同与资源表见执行器§10.27。

### 8.29 第二十九批：T2b-2原页面接线与旧实现清理（2026-09-23）

- 保留原路由、列表/编辑器和主题，将TimerService替换为窄页面适配器；根级唯一客户端复用HTTP/鉴权，WebView按账号/服务/设备存原意图。没有新连接、后台执行器、协议或凭据库。
- 真实taskId、同revision表、认证调度白名单、typed标量及三种任务模式；显式启用/Start/原ID恢复，写后认证再读，不乐观修改device.data。页面离开/账号代际变化和迟到弹窗取消，不清已安装任务。
- 删除旧JSON timing/loop/countdown、先启动再暂停的160ms等待、900ms假刷新、数组索引身份、20条固定假设、猜switch/toggle动作与旧样式；设置页不再用data.timer冒充V2已启用。
- HttpTaskApi补550ms可取消请求间隔匹配既有每秒4交换预算，不放宽Broker、不猜成功。目录/表查询不清未知写；用户核对新鲜表后可明确停止本地追踪，说明不取消设备/服务器原操作。
- 相关Angular90/90、完整spec TypeScript及AOT整应用构建通过；包含规范化GatewayHttpError、原意图恢复、CAS、错误/取消、分页、稳定ID及节流。证据/限制见App readme和执行器§10.28；无ADB/真实账号/本批MCU结果。
- 本批无设备/Service生产源码和wire变动，无部署/迁移业务库、刷板/清授权或Git暂存/推送。下一步唯一为§6的T2c；T2整体仍未收口，不推进A1或其他通道。

### 8.30 第三十批：T2c-1真实最小闭环及三项App修正（2026-09-23）

- 真实App既有owner创建专用WiFi设备，COM3完成DeviceKey→MQTT→授时、显式Initialize/4槽单目标目录、20秒倒计时安装/单独Start、退出并force-stop后执行、重进认证查询。再测软件重启Aborted且到期不续跑；不是物理断电或最大规模准入。
- 修复tasks:execute漏带鉴权；真实force-stop发现localStorage写返回不是持久屏障，替换为唯一IndexedDB strict事务端口，跨进程保留原ID/完整意图，显式historical恢复不重复Start。修复异步逻辑ready但OnPush页面不更新，以单signal状态驱动原页面/子组件；没有新连接/队列/SDK或设备wire变更。
- 最终App相关75/75、完整spec类型检查与APK构建通过。真实原请求恢复与重启由前一IDB版完成；最终响应式APK已安装验SHA，网络变更后真实错误页面正确，但成功列表视觉仍需新网补证。证据与测量集中见执行器§10.29，不累计历史测试或宣称T2整体完成。
- 先备份Gateway库再应用039～042，仅本机Gateway/Broker shard 1对新建单设备准入；Auth/其他Broker不整体升级。新测试设备原owner删除返回202，云撤权已生效，Broker清理待完成、本地任务未确认，未伪造退休成功。
- COM3整8MB恢复并校验原固件/NVS；COM11/COM12未动。临时HTTP代理已移除，备份保留，候选镜像/schema42保留但单设备名单目标已删除；无设备/Service生产源码修改，无Git暂存/提交/推送。当前网络“带翅膀的蜗牛”，下次只调整必需测试映射。
- 剩余主线按§6：T2c-2成功页面/日历/授时→有限policy/权限→T2收口，再A1；不把已过的原ID进程恢复继续列作缺失能力。

### 8.31 第三十一批：日历真机、完成记录恢复与重复查询收缩（2026-09-23）

- 新网络下真实owner/App/COM3通过一次日期、固定偏移星期、重启待授时再到期执行、完成后再重启不重放；真实成功列表与最终APK已验证，普通属性/按钮沿同一账号MQTT完成ACK/State。详细时间、计数和边界见执行器§10.30。
- 发现连续创建时触发TASK_RATE_LIMITED：编辑器写后读一遍，返回列表又读一遍。只删除前一遍冗余读取；确认才导航、目标列表仍认证查询、未知写留原页/原ID，不新增缓存/连接或放宽限流。真机替换＋返回列表6调用、删除＋返回列表5调用；连续新建两项通过，稳定ID不复用且没有隐式Start。
- 相关Angular78/78、完整spec类型检查/AOT/Android构建通过。仅App两个生产文件和两个测试修改，设备/Service生产源码、wire、schema和默认开关未改；本机仅更新Gateway/Broker单设备名单和新IP，COM11/COM12未动。
- 新测试设备经原owner DELETE返回202，cloudAuthorization=revoked且localTasks=retired_verified，brokerCleanupPending=true仍保留。临时代理已移除，备份保留；COM3原固件/NVS整8MB恢复及独立校验通过，恢复验证顺序修正与证据见执行器§10.30。无Git暂存/提交/推送。
- 主线只剩§6列出的跨到期失时、policy/容量、Key/非owner边界，未转入A1或其他通道；最大目录与鉴权预算适配不算已验收。

### 8.32 第三十二批：失时、容量与Start边界（2026-09-23）

- 真实App原owner、专用COM3、原4槽/policy/固件源码，完成失时跨到期不补放、满表不写第5条、4任务同刻完成、运行中Start冷却、显式原ID恢复后只执行一次。新旧请求/任务表/串口证据独立核对，详见执行器§10.31。
- 新发现的显示误导只在原模型/确认组件修正：calendar None是“尚无执行记录”，不是承诺未来执行；原ID重提仍可能首次执行，不是只读查询。未新增队列、时钟、协议或重试器，未知意图和鉴权不放宽。
- App79/79、完整spec类型检查；设备重新构建后x64 ASan70/70、0 skip。启动9秒的两次请求未取得可归因的引擎拒绝，冷启动门禁保留；不把服务503算作通过。
- 本机候选服务只换专用名单，schema42先备份不迁移；专用记录已取得任务退休证明后删除，临时代理移除，COM3原固件/NVS整8MB恢复并独立校验通过。最终APK、未确认的Broker清理及证据集中见执行器§10.31。无Git暂存/提交/推送。下一步严格按§6，不转A1或服务器整体维护。

### 8.33 第三十三批：冷启动归因与正常Key恢复（2026-09-23）

- 连接唯一性与协商就绪分层，移除旧合并入口；最终真机5/13秒为多个物理候选而非WiFi断网，23秒鲜活Read后Start未写入，冷却后显式相同原ID只执行一次。前6秒写额度没有经云端实测，不伪造覆盖。
- 修正新Key登录后任务页无法同步旧根的问题：Service拒绝无效Read并返回已有AUTHORITY_REQUIRED，原UI显式同步保留原表；已签发Rebind丢回执仍支持Read核对。真机刷新Key、旧Key重启不恢复任务云通道、新Key同步保留记录和后续执行均过。
- 最终App82/82、Gateway81/81、Broker11/11，完整spec类型检查及APK构建/安装SHA通过。仅改任务链必要模块/提示，设备核心、wire/schema及默认开关不变，不累计上一批ASan。
- 专用设备原owner正常删除且localTasks=retired_verified；Broker清理仍待确认。COM3原Flash/NVS整8MB恢复并校验，COM11/COM12不动。临时DeviceAuth与回环PG代理均移除，无卷数据删除，备份留存。证据`build/task-startup-live-20260923/verified-evidence.json`，详见执行器§10.32。
- 下一步仅真实非owner、未决写叠加Key变化恢复和认证拒绝语义；正常Key场景不外推到所有故障组合，不重复时间/容量门禁或启动A1。未暂存/提交/推送。

### 8.34 第三十四批：换Key叠加未决写的软件恢复（2026-09-23）

- 解除单槽依赖死锁：原意图＋最多一个显式授权同步同键原子保存；旧意图held不重提，同步成功不代替原操作确认，新鲜查询后才允许明确停止本地追踪。保留旧日志读取，不增加队列/worker/凭据或设备协议。
- App92/92、完整spec类型检查/AOT通过；Gateway82/82、0skip，新增真实repository轮换Key＋旧未决prepared/issued记录保留、旧ID跨root拒绝。服务测试使用隔离PG/HTTP/MQTT替身，不冒充MCU或真实第二账号。
- 认证拒绝最小合同已落执行器§10.33，明确attempt拒绝不等于request成功/历史未执行，现有Failure仍unknown。首次Initialize未确认便换Key、同步未确认时再次换Key独立未准入。
- 未刷板、安装APK、改部署/schema或暂存/提交/推送；仅临时回环PG测试代理创建后移除，无业务数据删除。下一批按§6第7项做组合真机及真实权限门禁，不重复已过冷启动/正常Key或扩A1。

### 8.35 第三十五批：未决写/换Key组合真机与权限边界（2026-09-24）

- 真实owner/App/COM3完成旧Start结算前丢确认→杀进程→单次换Key→显式同步再次丢确认→杀进程→恢复同步原ID；原Start和同步日志均保留，旧Start始终held。核对Done后明确停止本地追踪，新Start序号从1增至2且只执行一次。丢失发生在真实认证响应到达App之后，不冒充射频丢包、Service结果未知或物理断电。
- Service仅创建/旧Start/新Start三条mutation，各1attempt；新根阶段同步恢复后MCU callbacks=0、snapshots=1，明确新Start完成后为1/4。真实第二账号非共享catalog/initialize/start均403；operator分享accept因跨片未开放而409，仍未完成其任务权限门禁，失败邀请已撤销、手机已回原owner。
- 仅在原TimerService区分授权同步与业务操作的确认提示，补fresh/historical断言；最终App92/92、完整spec类型检查、AOT/Android通过。最终APK验证旧日志保持、明确结束追踪及新Start；完整双故障链使用前一候选，最终改动只有提示语义，不重复声称全链路重跑。
- 本机同镜像单ID试点，schema42先备份不迁移。测试记录正常删除并认证任务退休；Broker清理仍待确认。COM3原Flash/NVS整8MB独立验证恢复，临时代理移除，COM11/COM12不动。详细哈希/边界见执行器§10.34，证据`build/task-key-pending-live-20260923/verified-evidence.json`。无Git暂存/提交/推送。

### 8.36 第三十六批：认证拒绝三端软件闭环（2026-09-24）

- 沿现有Engine→Session/Result→Service账本→App严格解析实现写额度/Start冷却认证拒绝。无新队列、任务线程、secret或恢复日志；不改普通Arduino API，未知存储/网络结果仍未知。
- schema043为attempt增加带exact proof的rejected终态；request仍pending，保留原意图/先前unknown和回收floor。App只增加有限原因提示，不因HTTP 200清日志/自动重发。
- 设备x64 ASan69/69、共享协议45/45、Gateway88/88、Broker11/11、App95/95均通过；ESP32 stock3.3.11编译Flash990,524 B/静态RAM54,000 B、App testing AOT及结构/发布检查通过。没有将模拟MCU或编译写为真机通过。
- 本批未烧录、安装APK、迁移常驻库、部署或暂存/提交/推送；一次性测试schema/临时PG代理已回收。COM3原固件和授权未动。
- 下一步只做§6第9项：匹配候选的有限真机确认，然后评审余下权限/profile限制。完整合同和证据见执行器§8.13、§10.35；T2不据此整体关闭，不转A1。

### 8.37 第三十七批：认证拒绝及原ID恢复真机收口（2026-09-24）

- 原owner/App/COM3真实完成5秒倒计时→再次Start冷却拒绝→force-stop→原意图恢复→明确原ID恢复。拒绝快照5→5、回调仍1，Service request pending/attempt rejected；进程重启无新attempt/执行；明确恢复后序号1→2、任务回调总2、快照8，原拒绝证明保留。
- 普通账号MQTT下button及power ACK/真实State/设备计数通过；观察退出释放。App本批local/local-device AOT、Android构建和实际安装SHA核对，ESP32编译/烧录读回通过；无生产源码修订，不重复累计前批软件测试数。
- 仅本机备份后迁移schema43、更新匹配Gateway和单设备名单；Auth/其他服务不升级。专用设备正常删除并取得任务退休证明，Broker清理独立待确认；COM3原8MB/授权恢复并独立校验，COM11/COM12不动，临时代理移除，备份保留。
- 合同和证据见执行器§10.36。写额度认证拒绝仍软件证据；下一步§6第10项有限profile与权限收口，不重复已过组合，不转A1/维护扩张。未暂存/提交/推送。

### 8.38 第三十八批：有限准入与首次未知根前置拒绝（2026-09-24）

- 当前封闭试点准入集中到执行器§8.14，不另造总计划。清理执行器中已过期的“WiFi carrier/App仍未接线”当前指引，历史结果保留；不将四槽测试值当公开最终容量。
- 首次Initialize已签发未确认后换Key，在原仓储/事务内明确拒绝，HTTP透传有限错误，App沿现有日志/错误状态停用无效同步或重试。无新凭据表、队列、wire、设备RAM/Flash开销；正常单次换Key、同根Read恢复和owner退休不受影响。
- 最终Gateway93/93、App96/96、0skip；共享协议/Gateway编译、App完整spec类型检查及testing AOT通过。详见执行器§10.37；隔离viewer/operator矩阵不能替代真实已分享账号门禁。真实两个账号仍在不同shard，不扩跨片或改账号归属。
- 本批无现场部署/迁移、安装APK、刷板/清授权或Git暂存/提交/推送；临时测试代理/schema已回收。T2未整体关闭，唯一下一步取§6第11项。

### 8.39 第三十九批：连续未知根与正常历史收口（2026-09-24）

- 首次/连续未知根在同一处有界授权检查中拒绝；按已确认根的持久变更出边判断，不猜旧根、不用时间排序、不以pending作安装结论。合并原两段SQL，无新增migration、wire、设备资源或恢复队列。
- 正反对照均过：已签发中间根后再次换Key停止管理；未签发中间根可跳过；Read已确认根后，旧pending与Read历史回收不误阻断后续正常同步；退休独立。App原业务/同步双记录和按钮保护保持，文案涵盖两类未确认授权。
- Gateway97/97、App97/97、0skip；Gateway编译、App完整spec类型检查/AOT通过，详细软件证据及未测边界见执行器§10.38。未刷板、部署、改业务数据或Git暂存/推送。
- 下一步唯一为§6第12项匹配候选差异真机；已分享operator受支持同片条件、写额度真机、最大目录等仍保留。未知根是明确拒绝，不是已恢复，T2未整体关闭。

### 8.40 第四十批：真实Service未确认根拒绝及退休缺口（2026-09-24）

- 真实owner/App/COM3：A根初始化并保存未启动任务，刷新B后显式同步；在真实Broker响应后、Service验收前一次性丢失结果。58 B结果独立HMAC通过且revision1/count1，设备快照1；Service仍issued/无结果/确认A。不是App收到已验收响应后再阻断，也未写DB伪造未知状态。
- 原设置页再刷新C，App杀进程重开、C固件上线；首次和手动再次查询均明确RECOVERY_UNSUPPORTED，原同步ID保留、写/初始化/重试禁用。服务器原三条历史完全一致，设备零任务写/零自动回调；同一账号MQTT按钮/属性ACK为224/242ms，真实State/计数一致。
- owner删除202、云授权撤销及App列表移除通过；**MCU任务退休未通过**：localTasks=not_confirmed，退休无Grant/Result、设备无终止写入。定位原Session把待退休存储根与当前认证根绑定为同一个完整值；不删安全检查强行通过，也不把DB scope retired误报实物成功。
- 本机Gateway/App匹配候选、schema43先备份不迁移；无生产源码或wire变更。临时钩子不进镜像/发布并已撤除，临时登录代理已移除，COM3原8MB/授权恢复且独立校验通过，COM11/12不动。制品/备份哈希、7项临时钩子检查和实物证据集中见执行器§10.39、私有`build/task-root-continuity-live-20260924/verified-evidence.json`。
- 下一步唯一为§6第13项跨根终止路径，未知根恢复仍不做；其他未准入项目不自动扩张。未暂存/提交/推送，T2未整体关闭。

### 8.41 第四十一批：跨根任务终止的最小修复与实物收口（2026-09-24）

- 仅在原Session分离当前认证根与精确旧存储绑定；沿已有generation/签名摘要/nonce和Engine终止标记检查，不新增字段/队列、旧secret或Rebind旁路。移除测试中的过期说明，复用原夹具覆盖两类Cloud终止；API、wire/schema及App/Service生产实现保持不变。
- 最终设备x64 ASan69/69、Win32 Session/Budget2/2、Broker11/11、Gateway97/97，均0skip；首次并行时Broker两项超时取消、串行复测全过，日志保留。Session336 B不增长，S3三次固件Flash990,464 B/静态RAM54,000 B，分别比上一批-60 B/不变；不等于峰值或其他平台准入。
- 真实App/COM3重现A确认→B安装但Service未验收→C，未知根管理仍拒绝、原日志和服务三条历史不变；普通MQTT button/power ACK170/204ms且真实State/计数一致。owner删除取得74 B当前C认证退休证明，旧A/B Key验证失败、MCU仅一次终止写且无额外回调，localTasks=retired_verified；未把B补记为成功或恢复任务。
- 原删除合同保留brokerCleanupPending/localAccess未知；COM3原固件/NVS整8MB恢复并独立verify，COM11/12不动，临时故障钩子/代理撤除、Gateway/Broker健康，备份保留。本机schema43不迁移，无Auth/线上升级和Git暂存/提交/推送。集中证据见执行器§10.40及私有`build/task-cross-root-retirement-live-20260924/verified-evidence.json`。
- 下一步唯一取§6第14项有限准入差异，不重开本次终止窗口或普通任务矩阵。首次Initialize未知根、已分享operator缺环境及其他未准入边界仍明确保留，T2未整体关闭，不自动进入A1。

### 8.42 第四十二批：首次未确认根实物及用户入口缺口收口（2026-09-24）

- COM3首次Initialize实际写入空表，58 B认证结果在Service验收前丢失；Service确认根始终为空。刷新B并实际App进程重启后，首次/再次查询明确拒绝，原journal及账本保持；普通button/power ACK224/251ms、State/设备计数一致，任务无新写。
- owner正常删除取得74 B认证终止回执、localTasks=retired_verified；B Key验证通过、A Key失败，仅一次终止快照，没有补记Initialize/Rebind或恢复任务。Broker/localAccess未知状态独立保留，不冒充全部撤权完成。
- 六种实际stock3.3.11编译器组合检查符合现有guard，架构及发布profile检查通过，无生产代码改动。两份固件均990,464 B Flash/54,000 B静态RAM，烧录读回通过；没有重复累计前批软件测试数。
- 当前测试Sketch仍需内部composition接线调度目标，明确不能宣称普通Arduino用户入口已完成；新增执行器§8.16退出条件。已分享operator只读复核仍缺同片条件，不改账号/ACL或开启跨片。同步纠正本表仍写“Broker未接”和“policy未真机”的过期当前描述，历史记录不改写。
- COM3原8MB/授权恢复并独立verify，COM11/COM12不动，临时故障钩子/代理撤除、Gateway/Broker健康、备份保留。详细证据见执行器§10.41及私有`build/task-first-root-live-20260924/verified-evidence.json`；无Git暂存/提交/推送。
- 下一步仅§6第15项最小用户入口/profile，不再重跑已过未知根差异，不扩大通道/A1/服务器维护。T2及正式交付仍未整体关闭。

### 8.43 第四十三批：最小Arduino任务入口与维护者配置（2026-09-24）

- 新增显式编译opt-in＋`Blinker.enableTasks(properties...)`，自动复用授时、不改begin；类型化白名单复制到有限静态绑定，重复/错误目标不覆盖，begin成员/回调在任务存储启动前核验。复用原校验/生命周期/Engine/Session，不添连接/线程/协议。
- 原打包profile可选命名development任务试点，仅映射已明确4槽policy，生产仍拒绝。两个probe迁移新入口，旧内部宏和configureTargets旁路删除；普通用户无需Python或内部composition，默认源码未准入时明确失败。合同执行器§8.17，README/canonical API同步。
- x64 ASan70/70、Win32四项、实际编译器15组、打包17/17及架构/profile检查通过；四档实际固件链接通过。关闭版本Flash/RAM与原基线相同，开启4/8/10的静态RAM亦不增加，facade仍12 B；不把资源测量变成更大容量/平台准入。
- 最终development ZIP/stage字节一致，用户写法针对生成profile语法检查通过；未做该新入口的安装后完整编译或硬件测试，未部署/改App/Service生产代码、未暂存/提交/推送。证据与精确资源数统一见执行器§10.42。
- 下一步只§6第16项安装/最小COM3差异闭环；不重复已过故障全集、不扩大维护/A1，缺环境权限与正式发布仍保留。

### 8.44 第四十四批：单库候选安装及公开入口实物（2026-09-24）

- 同一development ZIP隔离安装，三种消费者完整编译并逐文件/来源/链接核验；无`--library`引用工作区、无内部启用接线。最小S3任务Sketch为984,948 B Flash/53,936 B静态RAM，实际测试Sketch为987,328 B/53,976 B。关闭对照为普通ESP32，不能跨板相减宣称任务资源增量。
- 真实App初始化后目录4槽/单power；安装不Start、回调0；显式Start后退出及停止App，20秒到期时MCU回调1。重进认证查询同taskId、startSequence1、progress3；普通按钮/属性控制与实际计数通过。正常owner删除取得retired_verified，云撤销/列表移除，Broker清理pending及LocalAccess未确认仍保留。
- 12条真实MCU结果独立HMAC核验，创建与Start各一条；74 B退休证明count0。没有借内部API/SQL伪造成功或新建第二MQTT。COM3原8MB/授权独立恢复verify，本机同镜像/环境变量恢复、临时代理撤除，COM11/12不动。
- 新增简洁试点交接说明；App/Service同步证据和下一步，无生产源码变更或新增依赖。本批不重复累计前批70/4/15/17项软件测试；无新断电/耐久/其他MCU/共享operator实物，不宣称T2正式整体关闭。
- 集中证据：执行器§10.43，私有`build/task-public-entry-live-20260924/verified-evidence.json`。下一步仅§6第17项可重建交接闭包；不提交/推送、不扩服务器维护/A1。

### 8.45 第四十五批：独立源码交接及可复用安装门禁（2026-09-24）

- 原发布器/profile仍唯一生成用户ZIP；小型维护者交接脚本只做白名单导出、完整性校验与调用原发布器重建。433项清单含PubSubClient源码/许可证，不含现场凭据、`.git`、SDK或硬件夹具，不新增用户产品或协议。
- 仓库外重建ZIP与第四十四批实物候选完全一致，再隔离安装完整编译通过：984,948 B Flash/53,936 B静态RAM；来源/依赖字节/链接均验证。摘要和边界见执行器§10.44、交接说明§5～6及`build/task-handoff-20260924/verified-evidence.json`。
- 交接9/9在原目录及导出目录各通过，不累计为18种用例；打包19/19、架构/profile检查通过。任务安装目标纳入原维护脚本、默认七目标不变。失配旧联调生成器退役，完整文件在`build/task-handoff-20260924/retired/`保留可恢复备份；历史结果不改写。
- 现有两账号只读复核仍分属shard0/1。无硬件/ADB、部署、账号/ACL变更、App/Service生产修改或Git暂存/提交/推送；不累计历史测试为本批结果。SDK/CLI未随包，三端完整构建/远端CI/生产准入未完成。下一步仅§6第18项A1最小设计。

### 8.46 第四十六批：A1最小设计、复用边界和失败合同（2026-09-24）

- 完成accepted State/持久快照、原command ledger/重传/重启未知、内部鉴权、PG权威/worker及App旧scene消费者复核；具体依据统一见Service自动化方案§2，不新增第二套协议/命令执行器。
- 纠正四个关键点：整份State updatedAt不证明字段新鲜；未变化不应靠keepalive伪装样本；7天ledger为保留期不是动作TTL；内部Bearer/签名不是规则owner授权。规则触发去重和命令投递去重分层，发送未知不跨会话补发，停用不冒充能召回已准入网络包。
- 冻结首轮reported-change bool范围、两张业务表/claim-fence、Broker有界观察、有限执行/保留、三端职责及A1-1～6门禁；数值/Telemetry需求、多跳/网关/其他平台任务均未扩散。旧scene仍有页面消费者，暂不无依据删除。
- 从当前源码重建contracts/Broker通过；原Broker隔离SQLite套件通过；签名重放/有界worker10/10、签名HTTP5/5，均0skip。这些验证复用底座，不是A1新功能验收。无生产源码/迁移/部署、账号/手机/板卡操作、Git暂存/提交/推送。
- 下一步唯一§6第19项A1-1纯contract/判定核心与确定性测试；不再次仅梳理主线，也不抢先接DB/HTTP或完整云执行器。

### 8.47 第四十七批：A1纯规则、连续观察与冷却决策（2026-09-28）

- Service新增单个automation-contract纯模块：严格bool定义、有限policy、图/配额验证、显式watch激活和边沿判定；无依赖、时钟读取、网络、后台timer或设备变更。只产候选意图，不发送、不伪装owner授权；未来仓储负责锁/当前能力与状态＋执行原子提交。
- 重连/换代/失连续性只重建基线，11字段不匹配及旧seq不推进状态；值不变可以长期静默。冷却用源acceptedAtMs判定，迟到记录不能在冷却结束后补发；时间/序号/计数有界。禁止目标再作源即可同时拒绝多跳/环，不预建通用图/表达式引擎。
- 新软件24/24、0skip，含512组8步布尔序列共4,096次转移；新模块Node覆盖行/分支/函数100%，不代表持久化/鉴权通过。Gateway原编译检查通过；既有相关回归27通过、2项隔离PG父测试跳过、0失败，不计未跑子用例。
- 文档同步A1-1真实内部数据及故障合同，纠正共同方案仍把温度阈值写作首轮范围的过期描述。SceneService仍有消费者，保留；未新增空仓储/worker或第二命令链，无可证明失用的旧自动化源码可删。
- 无数据库迁移/部署、账号/授权变更、手机/板卡操作、Git暂存/提交/推送。下一步唯一§6第20项A1-2有界观察；不是重做T2、扩大服务器维护或宣称自动化真机闭环完成。

### 8.48 第四十八批：有界观察、真实lane和生命周期（2026-09-28）

- A1-2a新增纯同步观察store和小型Broker投影适配，watch类型/校验/policy按真实复用需求抽到contracts；Gateway重复实现删除，规则判定留原处。无App/MQTT订阅副本、新服务或每规则timer，清理复用原5s tick。
- 最多16 watch、tenant4、每watch4记录；精确续租、读不消费、提交后ACK、2,048 B实际JSON页面限额。唯一当前物理连接与read＋notify bool才建基线，持久State不能冒充live；双连接瞬态、断连、完整重同步、溢出/过龄和时钟累计漂移均阻止旧变化继续执行。观察异常不阻断原Patch ACK/App路径。
- 新store22/22、真实Hello/Manifest/State/Patch软件12/12，加原纯核心24项共58/58、0skip。Broker新旧合并132/132含该34项，不能重复累计；原隔离协议套件与签名HTTP5项通过，contracts/Broker完整构建、Gateway检查通过。测试中数据库为隔离SQLite，不是实际设备或业务PG门禁。
- 当前没有生产watch创建者、观察HTTP、PG自动化表/worker或自动命令发送；不提前宣称规则运行。无设备/App生产源码变化、部署、账号/授权/硬件操作、Git暂存/提交/推送。已过T2矩阵不重复。
- 下一步仅§6第20项剩余A1-2b签名pull与授权端口隔离测试。需区分服务签名与业务授权；异步授权取消/超时不得复活观察，源时间/消费ACK不能被网络重试刷新。详见Service专项§13，A1-2整体尚未关闭。

### 8.49 第四十九批：签名观察、异步授权边界与消费预算（2026-09-28）

- A1-2b新增小型共享codec、Broker控制适配和Gateway拉取适配，复用SignedBinaryClient/Server及原store/纯判定。精确正文/响应摘要绑定每次操作，所有操作需当前授权；无provider拒绝、生产未挂载，不新建MQTT/设备wire或worker。
- 同watch不排队，tenant4/全局16在途；取消/超时不释放尚未结束的真实授权I/O，迟到acquire/renew不能复活。租约不超过当前权威剩余或30s；重启/过期旧claim检查责任明确留当前PG权威，无无限墓碑。
- page保留源acceptedAtMs、附单调剩余新鲜预算。Gateway扣整个RTT及本地等待，冻结page/收据并匹配当前完整watch；新epoch须显式激活，提交前再验证，HTTP200绝不自动ACK。精确外层计入2,048 B正文；最大字段/四记录夹具1,762 B＋32 B MAC，不是设备报文大小。
- 新控制19/19、编码4/4；Broker新旧151/151＋contracts/纯核心33/33合计184/184、0skip。contracts/Broker源码完整构建、Gateway检查通过。软件HTTP/SQLite证据不替代PG或实物；未操作业务库/账号/授权/手机/板卡，无App/设备生产代码、Git暂存/提交/推送。
- 页类型及owner比较集中到共享合同，重复定义删除；保留有消费者的SceneService，无新增依赖。同步三端方案，修正文档队列仍指第19项的陈旧提示。A1-2软件包关闭，下一步仅§6第21项A1-3a持久权威/原子消费，真实授权接线和云自动化尚未关闭。

### 8.50 第五十批：两表、持久claim及原子观察消费（2026-09-28）

- 新增migration044与单一仓储，复用原DB/纯判定；创建停用、稳定ID正文幂等、定义CAS、owner/Key/route现行权威、图/容量和唯一目标预约。锁顺序复用user/tenant前缀，加一个16条pilot全局锁后按设备ID/rule锁定；不加第三套队列/任意工作流。
- 30s持久claim/fence；不同opaque epoch必须新fence后显式安装，重复activate不清基线/游标，冷却/occurrence保留。观察/候选/预约同事务，ACK只在外层COMMIT后返回；写入或提交失败整体回滚，相同seq重传不新增动作。
- 修正Gateway收据有效期跨await漂移，固定原deadline并继续单调校验；候选同时受源session/能力receipt/claim限制，SQL期间自然过期也不提交。unknown不因编辑/停用/新规则或7天保留期消失；confirmed每次最多回收32、tenant1024硬上限。
- **实时能力不是PG页面缓存**：独立同步receipt检查口，无provider默认拒绝，真实Broker签名证明/唯一ready连接接线尚待下一批。本包设备ID不可编辑，无退役、执行发送/结果收敛；测试SQL未知记录不当真机动作。
- 新PG23子场景＋1父，与既有task/removal合跑79/79；Broker观察/通信151/151，共享签名/纯核心33/33，共263节点、0skip（含3父）。contracts/Broker完整构建、Gateway检查通过。仅一次性PG容器随机schema，无业务库迁移/部署、App/设备源码、手机/MCU操作或Git暂存/提交/推送。
- 同步Service专项§15、共同合同和App接线说明。A1-3a封闭仓储包收口，下一步仅§6第22项；尚未完整实现A1-3或自动化真机闭环，已过T2不重复扩测。

### 8.51 第五十一批：当前PG观察授权与反向签名（2026-09-28）

- A1-3b-1在原仓储增加只读授权，复用锁序/owner/Key/route/claim校验；Broker反向客户端与Gateway handler只包装原签名HTTP。原观察请求精确绑定用途/owner/fence/操作和截止，无长期allowed标记或新增表/发送器。
- ACK额外校验PG已提交的当前epoch/lastSeq，未提交记录不能因Broker已交付就被删除。旧claim/释放/新fence、当前状态变化和停用后的所有新操作均拒绝；短期既有许可不冒称可瞬时召回，生命周期写入口仍待接。
- 反向HTTP独立16槽；取消后实际PG工作未结束不回收，整段RTT和原绝对期限共同裁切授权，迟到/错误响应不能复活观察。先预检/取能力再claim，纠正原先“先claim再首次取能力”的执行顺序；PG授权不能回调Broker读Manifest造成循环等待。
- 新7项PG、8项签名和1项codec共16项；合跑PG86/86、Broker159/159、共享/纯核心34/34，共279节点、0skip（含3父）。完整源码构建和Gateway检查通过；双HTTP链使用真实PG/HMAC/store，但能力/连接投影仍为明确测试替身，不算MCU闭环。
- 三端方案同步，唯一下一步仍第22项的-2，之后-3生命周期、-4worker。未挂生产路由、迁业务库、改App/设备源码、操作账号/手机/板卡或Git提交推送。无新依赖，不删除仍有消费者的旧scene页面。

### 8.52 第五十二批：实时能力收据与PG连接绑定（2026-09-28）

- A1-3b-2补齐真实Broker两端Manifest/唯一ready物理连接投影，observer改为共用同一检查函数；无第二份Manifest缓存。Gateway先完成PG只读预检，再事务外签名HTTP取能力，回PG核对两端session/Key/route/当前owner。
- 能力receipt绑定完整定义摘要、原deadline与两端属性/会话/物理连接；私有WeakMap防伪造及滑动续期。物理ID不等于登录session，同凭据重连不能用新能力继续旧watch。源连接钉入runtime，目标连接钉入prepared binding；5s快照不是连续在线证明，后续A1-4仍须发送前复核。
- migration045仅新增源连接列、失效旧claim/prepared，保留冷却/occurrence/游标和未知结果；只在隔离PG运行。抽取真实协议测试helper，删除重复source检查及私有canonical hash；不引入sender、线程/额外消息总线或新依赖。
- 新5项PG＋7项能力/签名＋2项codec；相关合跑PG91、Broker166、共享/纯核心36，共293节点通过、0skip（含3父）。真实lane＋PG＋三类签名HTTP已到原子意图/提交后ACK，未发动作、不是MCU闭环；完整源码构建及Gateway检查通过。
- 同步Service专项§17、共同合同和App接线说明；下一批唯一-3生命周期，之后-4worker，后续才原Command/最小App/两WiFi真机。业务库/部署、设备/App源码、账号/手机/板卡及Git暂存/提交/推送均未改动。

### 8.53 第五十三批：同事务生命周期失效（2026-09-28）

- A1-3b-3沿原数据库生命周期模式，以单一函数覆盖源/目标Key、设备owner/tenant/state/type、tenant/route与账号删除；原事务内清claim/source连接、取消未发prepared，保留定义enabled、fence、冷却/occurrence/游标及dispatching/unknown历史。无新表/字段、清扫任务或第二套发送器。
- 修正锁设计：只收回资格的触发器不反向申请pilot/user/tenant锁；Key刷新入口先锁真实owner/tenant再锁device，失效统一rule→execution。授权恢复必须新claim/fence和基线，不能继承旧watch或追补旧边沿。
- 新12项PG场景覆盖实际Key刷新/解绑/账号删除/route cutover、幂等、回滚/提交失败、迁移和两种并发顺序；通过PG真实Lock等待验证并发。相关PG/Gateway111、Broker166、共享/纯核心36，共313节点通过、0skip（含3父及2脚本），完整源码构建/Gateway检查通过；修正旧observer测试随时间变化的TTL相等误断言。
- migration046只在隔离库运行；清理一次性PG容器及可重建数据，无业务部署、App/设备源码、账号/手机/板卡或Git操作。旧远端watch仍按原≤30s租约回收，未放宽撤权后的release，也不承诺召回已发消息；公开规则退役/查询仍留最小管理合同，未知结果不清除。
- 最新合同见[Service专项§18](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#18-a1-3b-3同事务生命周期失效2026-09-28)。唯一下一包为第22项的**A1-3b-4有界worker组合**；之后原Command准入/status、最小App及两WiFi真机，尚不能宣称自动化已可用。

### 8.54 第五十四批：有界worker接线，A1-3限定软件包收口（2026-09-28）

- 复用原轮询驱动，有限16规则/4并发/每页4记录；只读选ID→当前PG预检→签名实时能力→claim/观察→基线→consume外层提交→ACK。每条处理链独立能力检查闭包，无新队列、发送器或每规则调度timer。
- ACK丢失保留同watch/fence并按持久游标去重；重连/换epoch及撤权恢复须新fence/基线，不补旧边沿。取消不回收仍在运行的I/O槽；stop等待真实结束后有界清理，先远端release再清PG，撤权时仅原TTL回收。
- 新13项worker测试＋5项PG/真实lane/三签名HTTP组合；相关合跑129＋166＋36＝331节点通过、0skip（含3父及2脚本）。完整源码构建、Gateway检查通过。合法协议fixture不是MQTT/MCU真机，没有自动动作发送。
- 同步[Service专项§19](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#19-a1-3b-4有界worker与签名链组合2026-09-28)、共同合同及App接线说明；纠正共同合同残留的“下一步第20项观察”。临时PG/可重建数据清理，无业务部署/迁移、App/设备源码或硬件、Git写入。
- A1-3a/b限定软件包已齐，下一步第23项**A1-4原Command受限投递/status**；之后最小App、两WiFi真机。生产入口仍未挂载，不能把本包收口写成自动化已上线或T2正式限制已解除。

### 8.55 第五十五批：原Command逐次准入与只读status（2026-09-28）

- 第23项A1-4a在原Command增加可选内部约束：首次/每次重传前等待权威，异步前后校验原session/Manifest/物理投影；UTC＋单调截止、取消及迟到回调保护。原帧/sequence/ledger与有限重传复用，不增加发送器、协议字段或迁移。
- 纯status只读原账本，缺失不等于未发；可证零次publish的终止为not_sent，发过后不确定保留unknown。补活跃pending及崩溃历史的零attempt ACK保护，防止预留序号被当作执行成功；保留实际已发命令的原迟到ACK收敛。
- 新14项测试；相关Broker/App relay/网关命令332项、共享/Gateway纯核心及worker49项，共381通过、0skip；另Device V2原脚本通过。完整源码构建及Gateway检查通过。与前批不是同一测试集合，本批未重跑PG；准入回调为本地替身，不称PG/签名投递已完成。
- 同步[Service专项§20](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#20-a1-4a原command逐次准入与只读结果2026-09-28)及共同/App接线边界；清未用import，不删仍有消费者的模块。无业务库/部署、App/设备源码、硬件或Git写入。
- 下一包仅A1-4b持久执行/逐次签名准入合同，随后A1-4c窄Command适配/执行驱动组合；Gateway观察仍只产prepared，不提前发动作或进入页面。A1-4后才最小App与两WiFi真机，T2限制保留。

### 8.56 第五十六批：持久单次认领与逐次签名准入（2026-09-28）

- 第23项A1-4b-1为原execution持久绑定源物理连接，原子认领prepared→dispatching并发只一人成功；派发fence一次赋值，响应丢失/进程重启也不再认领发送。它不是第二套观察租约，原watch fence不变。
- 独立签名用途路径复用共有HTTP/严格codec；每次重查owner/Key/route、规则/源证据和两端PG session。首次钉住ledger/frame/sequence，attempt仅同次幂等或+1、最多4；1s请求预算与原候选截止共同约束，提交前后取消/超时不给许可。准入次数不是publish次数，失败不伪装设备拒绝或释放未知预约。
- 迁移047只扩原表并保守退休缺证旧prepared/dispatching；仅隔离PG运行，无业务部署。执行模块复用仓储鉴权/锁序，签名交换/strict session校验去重复；无新依赖、队列、发送器或生产worker。
- 新15项PG＋3项签名＋1项codec；相关Gateway/PG/共享118、Broker/命令/路由335，共453项通过、0skip；完整contracts/Broker构建和Gateway检查通过。测试不等于MCU闭环，物理连接检查仍须在原Command适配器前后接齐。
- 同步[Service专项§21](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#21-a1-4b-1持久单次认领与逐次签名准入2026-09-28)、共同合同和App边界。下一包合做b剩余结果收敛与c的窄dispatch/status适配，防止脱离实际账本冻结错误恢复合同；之后有界组合、最小App和两WiFi真机。不扩大通用网关/无缝通信或整组服务器维护，未改设备/App源码或硬件，未暂存/提交/推送。

### 8.57 第五十七批：原Command窄适配与历史结果收敛（2026-09-28）

- 第23项A1-4b结果及4c窄dispatch/status接齐：签名客户端与控制适配只包装原Command，复用两端实时能力投影、原帧/sequence/ledger/重传/ACK，不增加设备协议或发送器。
- 固定intent/execution/ledger/frame四层证据；准入bindingDigest绑定完整execution（包括目标值/fence/两端连接/原截止）。纯status从原存储帧核对，不依赖当前Manifest或PG先有ledger摘要；无记录仍missing，认领后崩溃不通过POST补建。
- 原PG执行模块新增历史查询和结果落库；当前授权撤销后仍可接原迟到ACK，但不能恢复发送资格。missing/unknown保留预约，乱序非终态不擦除已知发布次数，终态不回退；准入次数与实际publish次数分开。迁移048只扩原表结果，无新队列/清扫/猜测回填。
- 新11项Broker、10项PG结果组合与1项codec；相关Gateway/PG/共享129＋Broker/命令/路由346＝475节点通过、0skip，另原Device V2脚本通过。完整源码构建/Gateway检查通过；组合使用真实PG/HMAC/lane/SQLite，发布口和ACK为显式测试替身，不是真机。抽取共享组合夹具、摘要/状态投影复用，无无依据删除旧scene消费者。
- 同步[Service专项§22](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#22-a1-4b结果收敛与4c窄command适配2026-09-28)、共同合同/App边界。下一包仅有界执行驱动，再最小App和两WiFi真机；生产未挂载，A1-4整体不标完成。不改业务库/部署、设备/App源码、手机/板卡或Git index；一次性PG及可重建数据回收。

### 8.58 第五十八批：有界执行驱动，A1-4限定软件包收口（2026-09-28）

- 原PG pending记录仍是唯一队列，原Command是唯一发送器；新小型执行worker复用原有限轮询驱动/签名客户端，4并发、1s周期、single-flight，事务外取能力、唯一claim后才dispatch。结果失败/实例替换不重发；取消等待实际I/O，不清原fence/未知预约。
- 修正并发恢复窗口：已dispatching且原截止未到，其他驱动不能抢查missing误伤合法首次发送；到期后仅status，已unknown继续历史查询。未认领prepared到原截止可过期，无新源边沿也不永久占槽；未知不释放，不新建租约/清扫队列。
- 新16节点纯驱动＋9项真实PG/签名链组合；PG87＋Gateway/共享67＋Broker346＝500节点通过、0skip；原Device V2脚本、构建/编译检查通过。测试复用既有lane/HTTP夹具，发布/ACK为显式替身，实例恢复不冒充真机/真实服务进程断电。
- 同步[Service专项§23](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#23-a1-4c有界执行驱动与组合收口2026-09-28)、共同合同/App；纠正Service总表仍把A1-3b列进行中的旧状态。核对五种签名用途、未挂载事实和后续启动/停止顺序，没有提前部署。
- 第17～23项限定包完成，唯一下一步第24项A1-5最小管理/App及匹配候选，再A1-6两WiFi真机。不改设备/App生产源码、现场数据库/账号/板卡或Git index；一次性PG及可重建数据回收。A1生产及T2正式限制仍保留，不转服务器整组维护、网关扩张或重复发送内核测试。

### 8.59 第五十九批：最小管理API、写入恢复及有界退役（2026-09-28）

- 管理合同/仓储/应用服务/HTTP分层；复用用户身份、原PG事务锁、实时能力签名检查和执行器。补有限列表/详情/最近结果、默认停用创建、CAS启停与同设备端点编辑；请求不能自报owner/shard/fence，公开结果不暴露worker/session/连接凭据。
- stable ruleId创建重试返回当前版本，提交响应丢失先GET核对，同版本同正文可确认但不覆盖新修订。退役只取消prepared/清claim、修订+1，dispatching/unknown保留；迟到原ACK仍可历史收敛，新规则不能绕过未知目标预约。不增加自动重试发送按钮。
- migration049新增retired_at/查询索引及禁用约束；非退役定义4/tenant、16/pilot，含退役完整记录64/tenant、256/pilot。ID不自动删除复用；满额只拒绝新建，读/停用/退役/结果恢复不受新建配额阻断。
- 最终PG95＋Gateway/共享73＋Broker346＝514节点通过、0skip；原Device V2脚本、构建/编译和三仓diff检查通过，122处本地文档链接有效。中间并行回归准入STALE与测试Promise拒绝的修正/重跑情况见[Service专项§24](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#24-a1-5最小管理合同写入恢复与退役2026-09-28)，没有放宽截止。仅隔离PG运行新迁移并回收；认证口/设备发布和ACK有显式替身，不冒充账号/MCU实测。无新依赖、设备wire、现场服务部署、固件或Git写入；旧scene仍有真实消费者，当前不删。
- A1-5整体未关闭。下一步第24项最小App与候选生产接线，再A1-6两普通WiFi真机，不继续扩管理/发送内核、通用编辑器或服务器整组维护。

### 8.60 第六十批：最小App自动化页面与持久请求恢复（2026-09-28）

- 原scene入口增加独立`automation-manager`，避免与旧场景名称动态路由冲突；停用创建、启停、退役、有限列表/最近16条结果及原请求查询接齐。只选已有能力缓存中的所属普通WiFi boolean Property，服务端仍作最终权限/实时能力准入；旧多动作/延时场景明确分开展示，不自动转换。
- 合同、账号HTTP、持久请求客户端、页面状态与模板各司其职。发送前strict IndexedDB提交原ID/正文/CAS；超时/取消保留，恢复先GET，只有用户明确选择才重提原请求。较新版本必须显式核对，不自动抬revision；账号切换/离页阻止迟到结果和确认框再发写入，没有第二MQTT连接或后台触发器。
- 修正首个请求明确提交前拒绝也永久占用日志的问题：仅已知状态码/业务码可清该次本地意图；403/404/503及未知错误仍保留。历史未知请求的后续重试被拒绝不证明原请求没提交，不能据此清记录或换ID。每账号最多16个独立未决规则、仅一个未决新建；允许其他已有规则正常停用/退役。
- 将原任务专用IndexedDB适配移动为共享`IndexedDbIntentStorage`，删除旧实现路径/重复存储接口；原任务数据库名、表、键、格式不变，自动化使用独立数据库。不保留兼容空壳、不新增依赖/协议/存储迁移；旧SceneService仍有真实消费者，不删普通页面。
- App相关**117项通过、0失败/skip**（含自动化、共享存储、任务客户端/页面、账号HTTP拦截器）；TypeScript和完整开发构建通过。是软件/替身证据，不是原生持久性、真实账号HTTP、APK或MCU闭环；本批未重跑历史PG514、不部署/迁业务库、不改设备源码/授权或Git index。
- 详细边界见[App接线说明](../../blinker-app/src/app/sections/scene/README.md)及[Service专项§25](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#25-a1-5最小app接线及候选装配边界2026-09-28)。A1-5未整体关闭；唯一下一步是默认关闭的生产装配与启停门禁，再两WiFi实物，不回到服务器整组维护、T2重测或网关扩展。

### 8.61 第六十一批：默认关闭服务装配、配置与停机回收（2026-10-08）

- Gateway单一composition组合现有管理服务、三个签名客户端、两个反向授权handler及两worker；Broker单一composition接原runtime/routes，raw签名正文先于Fastify解析。配置关闭不启自动化观察/客户端/worker，普通协议不变；无新增设备wire、发送器、队列、存储表或依赖。
- 五用途key按shard显式配对，严格origin/64位hex/独立用途校验，缺配置启动失败，不回退DeviceKey或管理bearer。实际环境变量及最小部署取[Service专项§26](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#26-a1-5默认关闭的服务装配与启停2026-10-08)，不再将其列为待设计项。
- 反向HTTP监听就绪后才启两worker。停机先封闭公共管理、同时停worker并等待真实HTTP/PG，再关授权/存储；共享签名server增加dispatch drain。原raw body解析抽成可取消的小模块，移除内嵌旧实现并清监听，半个JSON正文不再卡停机；不把取消当业务写入已回滚。
- **PG96＋Gateway/共享84＋Broker扩展596＝776节点通过、0失败/skip**；原Device V2及实际Gateway index wiring脚本通过。新组合验证公开CAS→边沿→原Command ACK→持久结果、停机释放、重启新fence/基线、停用/退役；认证/publish/ACK有明确替身，非App/MCU/真实服务部署证据。过程失败及同一时钟断言修订保留在专项§26.3，未放宽原生产截止。
- contracts/Broker完整构建、Gateway noEmit/语法和三仓本批差异检查通过；147处本地文档链接有效。一次性PG及tmpfs测试数据已回收，未动业务库/常驻服务。
- 当前仍未部署自动化测试镜像/业务迁移、安装APK或刷板；App本批只同步文档，不重复累计上批117测试。第24项软件装配收口，下一步匹配必要测试组件及账号App，再第25项两普通WiFi实物；之后回候选交付，不扩服务器维护、网关或100次循环。未暂存/提交/推送。

### 8.62 第六十二批：匹配测试部署、App鉴权与两WiFi有限闭环（2026-10-08）

- 备份原PG/SQLite并在独立tmpfs PG实际恢复、验证同源043→049及设备/路由数保留，再只部署必要Gateway/Broker1。冻结289输入及镜像摘要见[Service专项§27](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#27-a1-5匹配测试部署真实账号app与实物准备2026-10-08)；实跑默认关闭503/缺配置拒绝后，仅为双方配对五用途新测试密钥、开启shard1，不升级Auth/Controller等整组服务。现场schema049与健康检查通过，不等于生产已开放。
- 用户确认本轮YYSC；只修必要测试地址/代理绑定。旧测试leaf和CA均过期，更新测试证书及Android debug CA overlay，仍严格验证证书，正式配置不变。原部署/数据库/APK回退材料保留；不能盲回灌043或沿用旧网络/过期CA，需要暂停先关闭现候选配对开关并保留账本。
- 真账号ME200而自动化401，定位App原鉴权URL识别漏接；仅补精确automation路径，复用Bearer/单次401刷新/Request-ID，外域及相似前缀不附凭据。先复现2个失败，修正后相关App **120/120、0失败/skip**，TypeScript、完整local Web及Android debug构建/更新安装/摘要校验通过。旧scene仍有消费者，不删页面或另造会话。
- 真App列表200/`private,no-store`及页面ready通过；上线前两端离线创建503保留原请求。Android force-stop/restart后同ID/CAS/正文仍在；GET404只读恢复未重发、不换ID、不清日志。该时点规则/执行均0不能证明历史安全可删；后续两板ready后明确恢复**同一原请求**成功，保存停用配置并确认清日志，再CAS启停，不把离线阶段冒充成功。
- COM3/COM11核对身份；ROM全读超时及RAM stub失败保留为失败证据，后续用新读系统区＋已校验静态材料重建并对当前设备整段verify。COM3完整8 MB、COM11仅`[0,0x340000)`通过，后者不冒称全片备份；COM12不动。普通开发profile单ZIP/空sketchbook/SDK3.3.11正式WiFi例程编译通过，416文件，Flash971,848 B/静态RAM52,288 B，不扩大T2或全平台准入。现有两端Key通过App正常reveal取得，未创建新设备/刷新Key/清授权。
- 两S3最终完整构建及app0刷写校验通过。四条执行均ACK；App杀进程后继续、Gateway/Broker分别重启不补发且新边沿可执行、源单独复位后新基线/边沿、冷却结束后的停用及初始true不触发均通过。ACK/State/回调分开；串口早期关闭控制线导致额外复位，修正后后续三次回调0→1→2→3，不能伪称累计4或实物GPIO验收。完整边界见[专项§27.4](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#274-a1-6两普通wifi有限实物闭环与收尾)。
- 最后原规则revision5/disabled、pending0，四条已确认记录保留；App观察回收/回首页activeLinks=0。COM3仅恢复`[0x9000,0x150000)`并校验；COM11写入时Hash通过、启动后整段后验失败，隔离确认boot/app一致而仅4个NVS页变化，再ROM保持下仅恢复NVS/OTA并对`[0x9000,0x340000)`完整校验后启动。原固件/授权材料已恢复，不能说启动后的可变NVS永不改变或原网络已重新联通；COM12不动。
- 三端文档同步、架构及差异检查通过；源码功能改动仅原App鉴权接线和测试，没有新增设备wire/发送器或额外schema/依赖。第24～25项限定闭环收口，唯一下一包第26项S2同源候选/依赖/支持范围，再S3差异验收/S4交接；不重测旧内核、扩网关或100次循环。未暂存/提交/推送。

### 8.63 第六十三批：S2a源码交接/完整重建与依赖差异阻断（2026-10-08）

- 复用设备原发布器/交接工具，扩为普通或显式任务development候选，production仍拒绝、旧task-pilot manifest兼容。普通WiFi433项交接在新目录重建，与第62批690,345 B实测ZIP逐字节一致；交接11及原打包19自测通过。未改变用户src/API/profile/ZIP字节，无需重复刷板。
- App新增独立维护工具，六仓必要源码/锁/原生配置1,639输入，排除签名材料/本机路径/已生成资源；8项确定性自测通过。保持五file插件，隔离安装→明确插件build→模板/类型/Web→Capacitor sync→Android debug，16步骤全部成功；Gradle冷构建14分11秒/439构建任务，非长稳/测试次数。
- 新目录Angular原测试入口61/61、0skip；首次直接Vitest缺启动/globals失败，未计pass，改回原ng test而非改业务断言。五自有插件dist全字节一致、file链接只指隔离目录。但实际pnpm工作区与npm锁有20项直接版本不同，含Angular/BLE/Ionic/CLI/Vitest；首轮APK只作诊断，不安装/不冒充第62批已测包。
- 有界锁对齐300秒超时、20项对齐peer冲突、独立全精确版本解析180秒超时均保留，不使用force/legacy-peer-deps；原package.json/package-lock/node_modules不变。单包registry查询正常，不把失败笼统归为离线或跳过版本核对。
- Service仅导出289输入/迁移001～049并比原来源摘要，核对两镜像ID/source标签及Node base RepoDigest；无重建镜像/迁移/部署/业务库修改。三端制品、支持/试点/preview及外部SDK责任统一到[候选清单](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md)，不扩运行时架构。
- **S2a限定包完成，S2整体未完成；唯一下一包S2b已测App版本/锁/包管理合同闭合并重建**，再S3差异实物/S4交接。当前无板/手机授权操作、无暂存/提交/推送；P1/T2/生产/iOS/远端CI等原限制保留。

### 8.64 第六十四批：S2b已测依赖合同、缺失输入修正与最终重建（2026-10-08）

- 按实际已测pnpm11.19.0收敛App/模板安装，不再强行把旧npm解析结果当同一基线。保留五相邻file插件及其独立npm锁，清除workspace占位build策略，明确禁用第三方install hooks；App旧npm锁完整备份到仓外后移除，原node_modules未改。
- 首次隔离类型检查暴露App直接导入却未声明的Ionic Core；显式补原工作区实际8.8.17及Buffer6.0.3，保留@ionic/angular8.8.19内部Core8.8.19的原组合，不顺带升级或全量hoist。扩大回归又暴露漏带公开授时golden：30文件386项通过、1套件加载失败，不计整组通过。修白名单并核对与设备canonical镜像一致，不改协议/断言或跨包借原目录。
- 最终六仓快照1,642输入，新manifest schema2；新目录16步完整构建通过，Android4分17秒/439构建任务，不称439测试。9项工具自测、2项真实pnpm锁失配拒绝对照和原Angular入口31文件**391/391、0skip**通过，覆盖BLE认证/退出、协议、缓存、MQTT、授时、自动化及页面动画；不是全部App/真机验收，不累计前批61/386。
- 逐项核对69个实际直接版本零差异，模板三版本匹配；既有lock包/peer snapshot条目不变，只补必要importer与原直接Core条目。旧Windows ntfy file specifier规范化为同一相邻目录。五插件新dist与仓库dist全字节一致、JS/type/map及Android main/build.gradle与原App实际安装一致；仅旧ntfy安装缺docs.json，现补文档元数据，不是运行时改动。
- 制品/摘要/失败证据统一见[候选清单§5.1](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#51-第64批s2b依赖合同与最终源码重建)。新APK只作development构建验证，未继承测试CA、未安装。S2b本机构建包闭合不等于现场信任配置、S3实物或正式发布完成；下一包第27项，不扩服务器/网关/协议。
- 本批仅修改App依赖/交接工具和对应文档；设备/服务器生产源码、SDK/profile/固件字节与原实物证据不变。仅只读枚举ADB/串口，无刷板、清授权、迁移/部署、暂存/提交/推送。

### 8.65 第六十五批：S3测试信任/构建origin修正与有限BLE实物（2026-10-08）

- 核对YYSC、主机/手机与健康服务；显式检查公开测试CA期限/fingerprint、限定域和实际TLS链，备份旧APK/私有App数据，更新时保留账号授权。未写开发板、清授权、升级SDK、重部署服务、改业务库或暂存/提交/推送。
- 初始HTTP诊断包有限BLE通过，但发现交接工具以local配置自动切WebView origin，隔离旧Manifest缓存/IndexedDB写意图。不是数据删除，也不能用新origin的pending0冒充恢复。只移除维护工具自动设置，补环境隔离回归；默认Capacitor/通信API不改。旧包、HTTP制品及失败记录均保留，不把诊断结果换标最终通过。
- 修正后1,642输入仅两维护文件变化，69直接/模板版本零差异；冻结包10/10工具测试，15步Web/sync＋独立明确CA的Android构建通过，原生4分42秒/439构建任务。复用原391项业务证据，未重跑/累计。初次全资产字节检查拒绝后定位SCSS构建路径注释导致chunk/hash/debug元数据变化，精确核对而非豁免业务差异。
- 最终HTTPS候选已安装/签名及摘要核对。旧账号/授权/Manifest缓存可见；命中3107ms、显式无缓存3677ms ready，均认证Manifest ACK后fresh State；Property323/287ms、Action300/302ms，按钮3→4→5、状态revision一致。退出126/123ms；早退222ms后6.4秒无残留连接，账户MQTT同一会话。最后恢复power=false并确认状态，不回退按钮计数或重放Action。
- ME200、A1原停用规则/pending0可读，DB revision5/disabled、4条已ACK历史/0未确认不变。但匹配包的普通设备Cloud仍offline，有限等待失败并释放观察、未发Cloud命令。旧profile包含不同服务地址只是线索，未读取本轮设备网络配置，不能断言离线单一根因。
- 精确制品/证据取[清单§5.2](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#52-第65批s3显式测试信任origin修正及有限实物)。S3仅剩第27项普通设备上线与ACK/State，完成再S4；不重测已过BLE，不扩服务器维护、网关/定时协议或100轮。含私有备份的整个build目录不得分发。

### 8.66 第六十六批：普通Cloud ACK/真实State、重启恢复及S4最小交接（2026-10-08）

- App现有账号MQTT ready，原COM3设备active/Key版本2；旧恢复镜像NVS和app0均含旧SSID而不含当前YYSC/Key原文。这些只是配置差异线索，未解码当前有效存储或唯一定位历史离线原因，不据此重构登录/鉴权。
- 仅专用COM3：新鲜读取64KB系统区，与历史静态字节重构后整8MB设备校验通过。同一普通候选ZIP的416文件与安装目录逐字节相同；标准WiFiQuickStart只替换私有参数，公开API/库/SDK不变。Flash942,976B、静态RAM51,296B，应用bin943,120B，装入原app0而不改bootloader/分区/其他应用。
- 真实App原账户MQTT：首次Cloud ready227ms，Property ACK287ms、Action ACK1104ms，实际按钮0→1、State revision1→2→3，最后power=false/revision4。非幂等Action只发一次；没有第二账户MQTT或debug伪造状态。
- 一次普通复位后先看到旧revision4/计数1；此时不发控制，也不把缓存ready计为恢复。新revision1/计数0相对复位工具结束38,582ms到达后，再开有限观察，Property ACK182ms、真实状态一致、最后power=false；未重放按钮。只通过功能恢复，连接/状态更新延迟未优化，不宣称无缝/低延迟准入。
- 原固件及NVS/授权仅恢复`[0x9000,0x150000)`，保持ROM直到1,339,392B整范围校验完成后启动；COM11/12不动。恢复旧配置后COM3 Cloud offline如实保留，不把测试候选成功冒充原旧镜像在线。App摘要与第65批匹配，账号/同一MQTT保持、观察已关、group/BLE link/session均0；原A1只读仍revision5/disabled、4ACK/0未确认。
- S3本机限定差异包收口，S4安装/模式/绑定重置/诊断说明取清单§7；接收方独立验收和生产准入未代签。没有改生产源码、清授权/刷新Key、新建A1规则、迁移/部署、暂存/提交/推送；已通过软件/其他MCU证据复用不重跑。私有证据目录为`build/core-candidate-cloud-20261008`，不得整体分发；原失败日志/备份未覆盖。

### 8.67 第六十七批：S4干净交接目录与离线验收（2026-10-08）

- 汇总原冻结Arduino433/App＋五插件1,642/Service289项输入、同ZIP/HTTPS debug APK、仅三文件公开测试overlay与脱敏历史摘要；总清单固定2,376文件，不含清单自身。无私钥、现场Key、数据库、Flash备份、node_modules或现场build目录，原件不覆盖。
- 复制后原组件工具校验通过，普通ZIP新目录重建逐字节一致。只读总清单校验自测8/8、0skip（含7项拒绝）；2项已知现场私密值的UTF-8/UTF-16LE检查及ZIP/APK共3,158条解压文件检查零命中，非通用秘密扫描或发布者签名。冻结目录前后不变，详见清单§5.4。
- 接收验收表仍为空白。当前mostfun2026，包保留已测实验profile；不重建App/服务、不刷板、不改网络/授权或Git，不把本地整理算接收方/生产通过。下一步第28项接收记录，无新功能包或重复长循环。

### 8.68 第六十八批：用户确认返回P1，限定控制/兼容/取消门禁（2026-10-08）

- S4接收记录保留待办，明确转回既有P1，未更改交接目录或追加打包验收。修复的是测量层：hint拒绝非致命、ready返回BALANCED、按实际阶段取消、记录原sendCommand真正结算；页面方法吞掉错误不能算ACK，Action闩锁不重试。
- 14项工具测试及原Android/已绑定S3四轮真实Property/Action、无缓存/拒绝注入、途中取消与重进通过。命中基线3050ms、短窗口2635ms，连接后认证/Manifest/State合计1872→1440ms；不把扫描/GATT差异或单组结果当SLA。原生日志确认HIGH 15ms→BALANCED 30ms；无缓存3015ms、提示拒绝注入2807ms均可控制。
- 认证途中退出导航12ms，100ms时仍有1条closing link/opening；6.4秒观察结束后零残留，重进2985ms，计数9未重放。末尾power=false、原授权/新鲜缓存保留、所有wrapper/观测已恢复。原Cloud仍retrying且页面错误未隐藏，不外推Cloud ready/无缝通信。
- 本机证据`build/p1-bounded-20261008/audit.json`；只动两测量/测试文件及文档，不刷板、清授权、更新APK/SDK、改服务或Git。R4/Wio/iOS/真正不支持hint的手机与挂起native回调未实物覆盖，正式运行时尚未启用；下一步第30项，不再重跑本批场景凑次数。

### 8.69 第六十九批：P1原生命周期候选、有限APK实物及闲置镜像清理（2026-10-08）

- 在App原BLE link中接入小型窗口函数，只有已绑定设备的正常bootstrap消费构建开关；不新建协调器/协议会话或修改授时、定时和自动化。独立原生请求避免可选hint卡住BleClient队列，非Android跳过，关闭/旧回调保留原代际和native-release quarantine。217项相关回归、类型检查、新模块lint通过，旧391项全套未重跑。
- 从1,646项输入导出独立快照，在现有工作区构建显式`local,ble-priority-test`＋原公开debug CA，HTTPS origin/签名一致。候选APK摘要`1a5b6457fa7e3791bc3e030dd7fbb3cdbfa99fe556a8bbb1070680e67ae4861c`，输入manifest摘要`1add06089151e5a7bd33cabfa80631464d2eb35e2ff85a63fbda849a9ab2e7df`；不是新目录依赖重建或S4替换。没有改依赖锁、插件、MCU或服务实现。
- 缓存命中/无缓存ready3054/2983ms，真实Property/Action ACK与State计数9→10→11通过，退出128/118ms；认证途中实际退出14ms，100ms仍在关闭，6.4秒结束零残留；重进3037ms、计数11不重放。原生Promise逐轮完整留存，审计时Android共享logcat环已覆盖前两轮，仅保留重进轮实际15→30ms参数证据。不凭这组跨批样本宣称稳定提速/功耗或全平台兼容。末尾power=false、原授权/新鲜缓存保留，所有wrapper/观测和ADB转发移除，手机已更新恢复原APK并读回核对`05071e…7606`。
- 按用户追加请求，仅删除42个无容器引用、无当前Compose引用且非默认latest的历史Gateway/Broker标签，涉及33个镜像ID；Docker镜像统计57→24、5.532→5.377GB，约减少155MB，共享层未重复计费。26个容器/运行镜像/挂载身份保留，120个持久卷和数据库不动，不执行global prune、卷/缓存清理或重配部署。首次收尾比较被Mounts枚举顺序误报，独立只读规范化核对通过；历史镜像需重建/重新拉取恢复，没有镜像tar备份。
- 证据：`build/p1-policy-20261008/audit.json`、`checks.json`、`build.json`和`docker-cleanup-verification.json`；原清理记录保留其误报，不改写成成功。目录含私有Manifest缓存备份，不整体分发。Cloud仍retrying、R4/Wio/iOS及健康Cloud并行未实物覆盖；S4不变，不暂存/提交/推送。下一步第31项先判断收益，不以长循环代替判断。

### 8.70 第七十批：P1同源码双包、有限配对及收益决策（2026-10-08）

- 在第69批同一1,646输入下生成关闭包`2dba09450cda1bfd0bd960d524a11af0585d30f6d1944c41bd1f4d2a93eca988`，复用开启包`1a5b6457fa7e3791bc3e030dd7fbb3cdbfa99fe556a8bbb1070680e67ae4861c`；相同更新签名、HTTPS origin/公开debug CA、DEX内容集合及非Web运行时字节，不宣称整APK字节相同或干净目录重建。构建/实测后逐项核对原输入摘要未变；不是拿旧源码原APK当关闭对照。
- 事前固定3组/6个有效样本，顺序A1/B1/B2/A2/A3/B3，每次安装一轮只读预热；主指标为GATT resolved至fresh State resolved。阈值同时要求中位配对改善≥100ms、≥关闭中位10%、至少2对改善，默认启用始终不允许。11/11分析工具测试通过，不累计或冒充已复用的217项App回归。
- 两次预热未就绪导致实验中止并恢复原APK。第一轮缺少断言前完整观测，原因不确定；另一次诊断确认安装后立即读PID有启动竞态，修正为只读有界就绪探测，但不把它冒充第一中断根因。第二轮记录扫描快速拒绝、无GATT；已有原生5次/30秒额度，尚无现场错误码证明唯一原因。最后四次安装新增35秒冷却后完成；中断、额外一次只读连接与条件修订全部保留，不筛掉失败称严格配对通过，也未绕过扫描额度。
- 六个有效样本三组GATT后1660→1388、1746→1475、1635→1419ms；中位配对减少271ms（16.3%）。两组各自中位1660/1419ms，差241ms；整页中位2733/2631ms，第一对整页反而慢13ms。三轮开启的实际参数均确认HIGH 15ms→BALANCED 30ms，关闭轮无请求，原生观测未丢弃。仅支持保留有界可选候选，不证明稳定整页提速、功耗或SLA。
- A1/B1各一次真实Property/Action ACK，305/249ms与246/269ms，按钮11→12→13；后四轮只读不重放，末尾power=false。六轮退出108～215ms，原授权/接受缓存保留；收尾实际读回原APK`05071e…7606`、零连接/测量wrapper/ADB转发。无生产源码、协议、MCU/SDK/插件/依赖变动，无服务/Docker或Git写入。
- 证据`build/p1-paired-20261008/audit.json`、不可覆写`plan.json`、两次失败`run.json`/`continuation/run.json`及`continuation-two`最终样本/恢复记录；不整体分发含现场标识的build目录。普通配置仍关闭；下一项32只补最小兼容性，R4/Wio/iOS/健康Cloud未测及S4接收待办不变，不扩P2或100轮。

### 8.71 第七十一批：新R4准备、原固件保全与有限软件检查（2026-10-09）

- 用户确认其中一块已换为R4。重新枚举为COM4、USB序列`F412FA9FA488`；不是历史`F412FA6F2F88`，不复用旧备份/绑定。COM3/COM11不动。先备份本板完整262,144 B RA Flash；首次30秒超时留下221,184 B部分文件，立即恢复运行，第二次延长读取完成，失败记录保留。备份不等于桥接ESP32/NVS备份，未升级或刷写桥接固件。
- 复用冻结单库ZIP `3ea54b…abfc`，隔离安装/依赖/编译来源与链接闭包检查通过：Renesas1.5.3、标准BleQuickStart，程序194,524 B、静态RAM16,964 B；实际bin194,532 B，SHA256 `72c0453bb26c10ea5147c5a9cadcca51684f4c07d4052ecfac8aba725f8b22d3`。整个安装门禁814.0秒；不是新增SDK或复制内部probe作为公开示例。
- 标准Arduino上传在握手阶段报`Device unsupported`，不盲重试、不清授权。之后完整读回262,144 B，SHA256 `0d84ad51cd18fe5b04797273bbcecdc02f6c5116f7b644136874f386e479bd60`与本轮原备份一致，复位回原程序。**R4 BLE缓存/控制/取消均尚未执行，不能记pass。** 已请求只拔插COM4，原二块S3和手机不动。
- 首连测量工具增加显式纯BLE模式；默认仍要求hybrid，真实授权和BLE能力不能绕过。就绪/发命令同时检查精确页面身份、已接受Manifest与fresh State，避免旧页面/缓存误报；只改两份工具/测试，16/16、0skip，不累计既有217项App回归。1,646项P1源码输入逐项摘要零差异。
- 手机USB reverse映射起初丢失，恢复17000/8080/18885后HTTP200；原账户MQTT曾在等待SUBACK阶段失败，`ErrorWithSubackPacket`缺少packet，**不证明Broker明确拒绝，也未确认根因**。App更新/重启后账户ready，恢复原APK后仍ready；没有改ACL/服务代码或部署。WiFi见证设备仍离线，账户ready不算Cloud设备控制通过或同会话BLE并行通过。
- 仅短暂安装原P1开启APK核验后恢复原`05071e…7606`并实际读回；账号/授权/Manifest缓存保留，group/link/session/opening/closing及测量钩子均零。没有BLE认领、属性/按钮命令、授权重置、Flash写入、SDK/依赖/服务/Docker/Git变更；保留必要USB测试网络映射，测量CDP转发收尾移除。
- 本机证据`build/p1-compatibility-20261009/plan.json`、`audit.json`、`unit-tests.log`、`source-audit.json`、原始上传/备份/读回日志及安装报告。含私有完整Flash备份，不整体分发。下一步仍第32项，不覆盖冻结S4，不新开实现包。

### 8.72 第七十二批：R4有限实物、公钥笔误修正与剩余边界（2026-10-09）

- 拔插后原冻结ZIP标准BLE固件成功烧录/读回，但正式认领在grant阶段断开并保存pending；沿原pending走一次生产resume，Direct扫描超时。继续只读核对发现该包测试Ed25519公钥第20字节为`0x3F`，源码默认及当前Gateway实际签发公钥为`0xF3`。这是确定的配置差异；修正后正常认领成功，不能归因于P1性能或通过删验签解决。
- 对433项冻结源码输入核对后生成隔离development profile，仅`OfficialConfig.h`测试版本标记及`PortableServerKeys.h`公开验签材料不同，416项安装字节匹配；原ZIP/S4不覆盖。新ZIP SHA `b44ab824204285f0992d6949bf9e6b0300c077aa2b0596ec12abc88b1988d72f`，R4增量编译程序194,524 B、静态RAM16,964 B，与原相同；实际194,532 B bin SHA `430abf99244b4e5a66051142c7f928eba7abdfc51913285eab783608cad16e2a`，标准上传后读回一致。没有MCU/SDK/协议/服务变更，COM3/COM11不动。
- 通过生产guide和精确测试板locator认领；原客户端按deviceInstance/access上下文调用cancel→issue→commit，旧pending正常撤销，仅新签发一次，receipt/Method2/Manifest/State真实完成。没有直接删除数据库、伪造credential/ready或清桥接NVS。临时观测只保存方法名和结算，不保留grant/secret/HTTP原文。新授权保留在当前账号/桥接存储，不宣称已恢复测试前NVS。
- 命中ready7,241ms（发现3,831、GATT1,441、认证1,174、Manifest430、State259ms），Property/Action各328ms真实ACK，计数0→1；无缓存ready3,452ms，实际kind2请求，Manifest748ms；两次只读重进3,128/2,980ms，计数仍1，无按钮重放。命中仍发送ManifestAccept/State，不拿缓存当fresh State。正常退出182/169/169/164ms，group/link/session/opening归零。实际运行时开关true且ready后窗口清除；后续只读样本保存实际15→30ms参数更新，首次样本不把API返回冒充空口证明。
- 第一次指定authentication取消在GATT阶段实际拒绝，8秒期限未达到认证，退出回收虽完成也**不是认证取消通过**；保留其两次运行时连接尝试及原记录。工具新增`cancelFailed`并补负例。资源归零/间隔后只补一次认证取消，实际phaseReached=true，导航74ms，100ms仍closing link/opening，6.4秒终点归零；该连接仅一次HIGH，不晚发BALANCED。随后只读重进通过，不自动重放Action；不称连续稳定性/SDK问题已解决。
- 修正`tools/arduino_release_profile.py`及development fixture的公钥；生产拒绝实际公开测试key，错误旧fixture在development/production均拒绝。新增profile与源码默认公钥一致性检查。30项打包/交接测试、17项工具测试通过、均0skip；不累计原217项App业务回归。本批未改App/插件生产源码、依赖或SDK；旧冻结源码仍可历史验证，不因当前工具纠正重写其manifest。
- 四个就绪窗口、正常退出、取消的健康账户MQTT保持同一现有session；不新增第二条App MQTT。WiFi见证设备离线，**Cloud设备前后控制仍未通过**。iOS缺实机、Wio仅preview边界、第70批小样本/中断限制和S4接收待办均保留，P1普通配置继续false。
- 手机已恢复原APK `05071e…7606`并实际读回；手机新授权/Manifest缓存保留、旧pending及旧服务器设备条目正常退休，零连接/观测，移除测量CDP转发、必要reverse保留。R4恢复原应用首次标准上传握手失败；用户仅拔插COM4后，重新核对USB序列`F412FA9FA488`，上传备份0x4000起的245,760 B并读回一致，SHA `514057216c60c3aaf65f2acb9feb83ece9f5bccd50dcd9b83a9a265786aad98b`。未写bootloader、刷桥接固件或主动清桥接授权；不夸称完整NVS备份/恢复，也未在原固件上重新执行Method2。恢复后再次只读核对手机原APK、账号ready、原pending退休、新授权/缓存及零资源，测量转发已移除。
- 最终证据`build/p1-compatibility-20261009/continuation-audit.json`、`current-ble-service/{prepared,compiled}.json`、原始失败与逐轮`corrected-*`、测试日志、`continuation-post-restore-final-*`及`r4-restore-*`；恢复前待办快照`continuation-before-restore-audit.json`原样保留，不覆盖为成功。再次核对1,646项App/插件实际输入与冻结快照无差异，S4 manifest不变。本地完整Flash/缓存备份不整体分发，没有暂存/提交/推送。

### 8.73 第七十三批：健康Cloud/BLE只读短链、公钥组件交接与恢复（2026-10-09）

- 用户许可仅COM3 WiFi见证板。身份核对后首次USB stub全读失败，未写Flash；新读64KB可变前缀与本板历史静态字节重构，整个8MB与设备校验匹配后才刷写。不是新鲜全量读取，失败/补救记录保留。公开WiFiQuickStart仅私有网络/Key参数和独立测试profile变化：416项安装文件匹配，SDK3.3.11、PubSubClient2.8；程序942,976B、静态RAM51,296B、bin943,120B，SHA `cd577e68b8a1ee217d1cc8c77fe0bc4ee515cf39759075308351d4fbea64cb8a`。不新建/刷新Key，原设备active/版本2；COM11/COM4不刷写、清授权。
- 本轮mostfun2026，PC `192.168.1.146`；旧Gateway下发`192.168.50.13`，原HTTP测试代理不存在。仅临时改两个公开host及对应device-only端口映射，复用原镜像/环境其余字段/原key卷；单独启动同实现的只读无状态device-auth-only HTTP代理，不绕过鉴权、改签发密钥/业务库/协议或升级服务。首次准备因存在可写key卷拒绝，随后精确保留原卷身份和访问模式，不删除或重建key卷。
- 同一真实App账户MQTT，无第二App登录：BLE连接前/保持连接中/退出后重新获得StateInterest，三次新StateRequest→StatePage序号14/20/24，读取往返均60ms、fresh ready241/87/97ms，revision1、power=false、按钮0。释放观察后fresh失效，再次获取收到新回复，非缓存ready。中间请求起止均有一条既有BLE连接；不发Property/Action，不触发A1自动化。这里只证明两个真实设备的有限Cloud/BLE并行读链，**不代表同一设备通道无缝切换、Cloud控制重测、多子设备压力或长期稳定性**。
- 实际P1开启APK、COM11原授权/固件：只读BLE ready3393ms、GATT后1745ms，退出213ms后group/link/session/opening归零；原生优先级请求两次均结算，观测零丢失。本批没有空口参数测量或新A/B收益判断，250ms/5秒和20B/4B/30ms原合同不变。21项通信测量工具、11项交接工具测试通过且0skip，不累计旧217项App回归。
- 修正公开Ed25519组件包433项输入，新目录重建ZIP一致，690,345B、SHA `1e05deba4f1965d28db7f128706cc64d39343d826587cea8e0028ff191b423c1`；与旧用户ZIP仅公开验签头不同。维护输入另含profile、fixture、校验工具及交接文档差异。新组件交接`local-maintenance/core-arduino-ed25519-fixed-20261009`不含现场Key/当前网络overlay/Flash；仅检查已知COM3 Key零命中，非通用秘密扫描。1,646项P1源码输入零差异，旧S4 manifest不变，详见清单§5.5；不是完整BLE发布矩阵或独立接收通过。
- COM3恢复原NVS/应用范围后，保持ROM直到整8MB与本轮完整镜像一致，SHA `36125acd7e0d02afcf9038e1c47ddf4d0694ce7d25df798a0b2399d824a15fbb`，再启动。随后64KB抽查boot/分区不变、NVS/OTA变化各0B；普通启动以后仍可能写NVS，不以抽查替代整范围校验。手机恢复原APK `05071e…7606`并读回，原账号/授权/Manifest缓存保留、原账户ready、零连接/观测、测量CDP移除；恢复旧设备配置后COM3离线如实保留。
- 精确网络回滚曾失败：旧`192.168.50.13:18884`绑定在当前电脑不存在，Gateway停在created。旧实际published映射本就为空；恢复时仅移除该未生效绑定，保留device listener内部可达、原镜像/完整环境/卷/loopback HTTP，健康后删本轮无状态代理。**配置端口映射未完全原样恢复**，失败/原配置/修复配置分别保留，未扩大明文暴露。后续设备实测需要按实际网络另核对地址，不默认本轮临时profile仍可达。
- 最终证据`build/p1-cloud-compatibility-20261009/audit.json`、`AMENDMENT.md`、逐次Cloud/BLE读取、完整备份/恢复、网络失败/修复及交接摘要；私有build目录禁止整体分发。P1默认关闭，iOS实物/Wio preview/独立接收保持待办。下一步第33项边界收口，现有场景不继续重复、不开100轮/新策略；无生产运行时代码、SDK、服务镜像升级或Git写入。

### 8.74 第七十四批：P1支持、默认开关及交付边界收口（2026-10-09）

- 第33项完成：共同方案§3.4冻结作用路径/窗口/普通关闭及显式Android实验；清单§3.1区分历史S4、公开公钥修正设备组件和独立P1 App。不再把已补的R4/健康Cloud并行写成未测；iOS实物/Wio preview仍待测，不称正式支持、稳定整页提速或同设备无缝切换。
- 只读核对八项manifest/APK/ZIP摘要、原S4全部2,376文件、修正433项组件及两份App快照原verify，均通过。当前1,646项App/五插件输入匹配冻结P1；相对原S4新增4项、修改9项、删除0项，仅App文件不同，五插件/依赖锁未变。旧S4及修正组件未覆写，无输入变化不重新生成全部制品。
- 实际调用原交接工具，三种P1配置均在创建work/依赖安装前拒绝；只支持原production及local,local-device。原工作区的P1构建/有限实物成功不冒充新目录依赖重建。后继合成包如需交付，仍须新manifest/匹配构建及实际差异验收，不借用旧S4摘要或代签第28项。
- 本批四文件**41/41相关App回归、10/10交接工具测试、0skip**；配置3、窗口11、transport12、client15项，不累计历史217/391项或称全部App回归。原命令与结果取`build/p1-candidate-closeout-20261009/{audit.mjs,audit.json,checks.json}`，这些是本地检查记录，不进入用户Arduino ZIP。
- 只更新现有主线/共同合同/候选清单/App交接并补本地只读审计，无生产运行时/协议/插件/SDK/服务改动，无APK/ZIP重建、手机/开发板操作、清授权、Git暂存/提交/推送。没有确认的不必要运行时代码可删，不为默认关闭删除仍在显式路径/测试中使用的模块。下一步优先真实接收验收/实际差异；缺环境待办保留，不另开重复维护包、P2或长循环。

### 8.75 第七十五批：P1显式维护入口及新目录Android构建（2026-10-09）

- 用户继续主线后补明确的P1新目录构建缺口，不代签独立接收。只改App `tools/source-handoff.mjs`/`.test.mjs`：一个固定配置数组统一准入/说明，新增已测`local,ble-priority-test`，拒绝裸开关、production混合和任意组合；记录实验标记，不新增管理器或安装器。12项工具自测通过、0skip。
- 独立目录`local-maintenance/core-app-source-p1-clean-20261009`仍1,646输入；manifest SHA `2918f27d044404dda2726f052c99737edc0f2e122fb937723c22e63e5433dd5c`。与第69批冻结输入仅上述两项不同，App/三端运行时、五插件及锁不改，旧S4/旧P1不覆写。包内工具在`core-app-rebuilt-p1-clean-20261009`完成16步独立构建，不借原dist/node_modules或继承私有CA。
- Node22.22.3/npm10.9.8/pnpm11.19.0；JDK21.0.12.1、SDK36及原Gradle/AGP。缓存/临时目录置D盘。实际App/模板直接依赖版本与原S2b报告一致；四份App锁输入及五插件package/锁字节不变。五插件npm安装在新work内，dist/声明发布的TS/Android main及Gradle字节与新工作目录构建内容一致；原生单元测试仍在源码包，不要求出现在npm运行时安装包。
- 新APK **79,949,462 B**、SHA `f54b7881d08c3d4eaa1dbe7656d5ac74e159f1c1b55f6cd9126ff35410b34209`，只作显式实验构建证据，无测试CA、未安装/实物验收。APK内Capacitor HTTPS/cleartext=false及实际导出的P1=true覆盖核对通过，已知测试CA条目0。Android3分26秒/439构建任务不是测试；隔离目录四文件41/41相关回归、0skip，不累计原217/391项。
- 本地诊断先误把继承的false、引用flag的JS和npm未发布测试/raw TS当差异；按实际导出环境及既有package files纠正检查，未改构建/业务断言或冻结输入以凑通过，记录`diagnostic-corrections.json`。最终源码/插件/依赖/生成配置/制品审计通过，原八项制品摘要未变。证据`build/p1-clean-rebuild-20261009/{PLAN,audit,checks,diagnostic-corrections}.json`及新work原始日志；准备时JDK目录查找错误也保留说明。
- 未改普通默认开关、协议、插件、MCU/服务；未操作手机/开发板、清授权、部署或Git暂存/提交/推送。新的构建证据不取消iOS/Wio、真实接收及生产/T2/A1限制；下一步按实际交接或匹配新制品的差异验收，不自动扩新功能。

### 8.76 第七十六批：新目录P1公开CA变体与有限实物闭合（2026-10-09）

- 复用第75批1,646项隔离源码/依赖/Web/sync结果，只显式加入冻结公开debug CA，在新`local-maintenance/core-app-p1-test-ca-20261009/app-build`输出App模块。原无CA APK `f54b78…4209`不覆写。TLS实连`127.0.0.1:17000`正常验链/域名，CA指纹与冻结材料一致，无私钥、信任绕过、origin迁移或服务/网络配置修改。2分14秒/439构建任务中36执行、403复用，不冒充第二次全依赖冷构建或439项测试。
- 新APK **79,951,127 B**、SHA `cebd57ecc8f497a7df1bc77c3d5a4b31d4ece3866832fc6d6dba4df090c22b85`，更新签名/HTTPS/cleartext=false正确。与第75批包1,580项assets、16项native lib、22个编译App Java类一致，18份合并DEX中17份一致；仅新增`raw/blinker_local_ca`、`xml/blinker_local_network`资源与生成R所在项目DEX变化。首次工具误要求所有DEX摘要一致，安装前拒绝；保留纠正说明，未改源码/业务断言，**不称实际DEX同字节或证明APK位级复现**。
- 手机原账号/精确已绑定COM11目标，不刷三块板/清授权。更新前App停止后备份4,597,248 B私有数据，原HTTPS下3条任务intent及空自动化intent只读摘要分别保留；安装新包后均一致。实际P1已绑定路径开启、窗口在ready后结束，缓存命中ready2,756ms、GATT后1,374ms；Property/Action真实ACK309/323ms，按钮0→1，power恢复false。一次正常退出222ms，组/link/session/opening归零。
- 35秒以上扫描间隔，不加速/绕开原生额度。认证阶段真实取消，导航18ms；100ms观察仍有closing连接，6.4秒终点归零，只有一次HIGH请求、没有迟发BALANCED。只读重进ready2,590ms、GATT后1,450ms，按钮仍1、power=false，不重放Action；退出220ms归零。原生Promise观测均零丢弃，有限窗口账户MQTT同一原session；本批不测空口参数、不做A/B性能推断、Cloud设备控制或同设备无缝切换。
- 17/17两文件测量工具检查、0skip，不把第75批41项或旧217/391项累计为本批业务回归。前后1,646项源码/插件/锁全部匹配，新/旧八项制品摘要仍匹配。手机实际恢复原APK `05071e…7606`并读回，账号/授权/Manifest缓存/两个journal摘要不变、账户ready、零连接/观测，测量CDP转发删除，既有USB reverse保留。
- 证据`build/p1-clean-device-validation-20261009/{PLAN,prepared,apk,audit,final}.json`、`variant-build.log`、逐场景JSON及`CHECK_CORRECTION.md`；私有App备份不可整体分发。仅本地验证脚本/既有文档变化，无生产源码、SDK、插件、依赖、服务器/Docker或Git写入。**第35项发送方有限新包链闭合；第28项独立接收/iOS/Wio待办和T2/A1/生产限制不变。** 没有实际新差异/环境时不继续增编号打包或重跑，通过项冻结，下一步由接收验收或明确需求触发。

### 8.77 第七十七批：A2条件合同与无I/O核心（2026-10-09）

- 第36项/A2-0/1完成：新增共享条件模块及测试，支持布尔上升/下降、数值上下阈值/正回差的纯语义；首次状态/新基线不动作、回差带保持锁定、到重新布防区才允许再次触发。严格校验有限数/边界，未引入通用表达式、设备规则表或新依赖。
- Gateway A1原判定点复用上升沿核心，删除重复硬编码；A1输入/输出、boolean存储、原权威/fence/冷却/事务及Command不变。下降沿/数值规则和number观察仍被A1入口拒绝，**纯核心完成不代表公开自动化功能已增加**。
- 原A1合同24/24作为对照；新纯条件21＋A1合同25为46项子集。最终9文件联合回归103节点（99顶层）、0fail/skip；shared构建与contracts/Gateway/Broker三项TypeScript检查通过。计数不与前批776或这两组子集累加，不把fake端口/loopback HTTP测试当PG、部署或实物。
- 三端底座缺口已落[Service§28](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#28-a2实用条件扩展与执行顺序2026-10-09)：既有能力/观察/运行态/App均按boolean绑定，后续须精确版本化typed链路与独立锁定，不能仅放宽比较或管理输入。共同方案纠正原A1“链路待接”的旧状态，当前队列指向第37项，再38项App/有限实物。
- 本地证据`build/automation-a2-condition-20261009`保存PLAN、原/新/联合测试、类型检查和审计。本轮无App/设备源码更改、硬件/ADB/Flash/网络/数据库/Docker操作，不提交/推送；289项Service与S4/候选摘要不覆盖，新源不冒用原实测制品证明。P1、T2、独立接收及生产限制均保持。

### 8.78 第七十八批：A2显式类型合同与同一有界观察（2026-10-09）

- 第37项的A2-2a软件门禁完成，**不是第37项或A2完整关闭**。新增小型scalar校验，显式V2 Owner绑定policyVersion/definitionDigest/valueType；源boolean/int32/uint32/float32/float64对应实际Manifest type 0～4，目标仍boolean。V1精确字段/签名字节/默认codec保持，V2必须显式选择，类型/版本/摘要错配拒绝，不coerce或舍入样本。
- Broker复用原能力投影、accepted-lane、单一store/分页/control/反向权威客户端，不增加第二队列、sender、采样定时器或依赖；删除固定bool转换和重复分支。Gateway真实持久权威在进入SQL前明确拒绝V2；公开规则、execute/admission仍V1，不修改设备BBP/2 wire、固件或App。
- 最终18文件联合回归**242测试节点（236顶层），0fail/skip**；shared/Broker构建及contracts/Gateway/Broker三项类型检查通过。覆盖A1原合同/worker/执行/管理与V2精确绑定、5类型真实协议fixture、数值Patch重传/物理重连新基线/旧epoch ACK拒绝。最终审查修正摘要正则可接受尾换行的缺口，收紧精确64位与字符集合，补owner/watch/观察与能力请求回归。软件fixture经过生产CBOR/Hello/Manifest/State路径，但不是MCU或空口实测。
- 容量不放宽：仍16总watch/tenant4、每watch4 records、外层body 2,048 B。合成最大字段/JSON转义/有限极值测试的4-record回执为1,996～2,000 B，3-record为1,864～1,867 B；这是wire预算，不是设备RAM或全流量压测。fake端口、临时loopback HTTP和每fixture临时SQLite均为隔离软件门禁，无真实Gateway PG、设备/手机或部署操作。
- 证据`build/automation-a2-typed-20261009`保留PLAN、首次失败日志、最终联合回归和审计。首次scalar类型收窄失败已修；初次并行Broker检查使用旧shared声明，改为shared成功后再构建Broker，不把中间失败/子集累计成最终通过。此前执行中断/审批超时如实保留；未暂存/提交/推送，新源码不冒用旧289项Service/S4/镜像的实物证明。
- 下一包**仅A2-2b**：精确规则/摘要及原Gateway JSON runtime显式版本，raw值与latch分离，同事务消费/冷却/执行，definition/type/version贯穿candidate→execution→每次admission；随后2c补隔离PG与版本门禁。旧graph全量读取/worker无版本过滤的混部署风险须先解决，不能静默降级或先落A2行。第38项App/必要两设备实物仍在后，不扩多步骤/网关/通用表达式或全量维护。

### 8.79 第七十九批：A2事务内锁定、触发证明与原Command接线（2026-10-09）

- 第37项2b内部候选完成，原10字段rule采用显式policy2/条件对象；runtimeVersion2分离rawValue/latched，完整rule digest/type贯穿Watch、candidate、execution及admission。A1旧字段/默认V1回执保持。新基线不触发、冷却触发消费锁定不补发，同revision改定义和policy降级拒绝；目标仍单个固定bool。
- 复用原JSONB表、同一事务/全局锁、观察/执行worker及Command/ACK/ledger，未新增依赖/表/队列/sender/采样定时器。consume同事务提交raw/latch/cursor/冷却/发生计数及candidate；每次执行重验触发证明、定义/类型/occurrence、owner/Key/route/session/物理连接和TTL，不仅删source=true。proof只保存前值和锁定3字段，删除被完整规则摘要重复覆盖的condition，避免极小浮点展开浪费存储。保留一个execution-ID ledger命名空间，换版本不能绕过去重。
- 最终22文件联合软件回归**303节点（288顶层），0fail/skip**；shared/Broker构建及三项编译器检查通过。真实签名loopback、5类型生产BBP/2 fixture、Command精确frame/ACK/重传及默认/错配校验属于软件证据。Gateway常规测试入口纳入新文件，test:device-v2复用test:automation，清理重复清单。
- 现有PostgreSQL16/原Gateway镜像Linux Node20.20.2依赖、候选只读挂载的一次性容器中，最终隔离PG回归**107节点（106子测试），0fail/skip**，约57.6秒；前一次106是子集/修改前证明，不累加。包含延迟COMMIT失败回滚后精确重试、cursor冲突、5类型、冷却不追补、重算外层hash仍拒绝坏proof、V1/V2授权错配、逐次admission、编辑后的原绑定历史晚到回执及混版本graph限制。A2能力/观察替身明确，不把原A1全组合或分层A2当完整V2/手机/MCU闭环。
- 预算未扩大：compact执行/准入合成压力样本1,971/1,460 B≤2,048；PG实际8组最大元数据/quote与backslash转义、MAX/MIN浮点指数展开，**该压力集合实测最大**definition2,035 B≤2,048、runtime1,762 B≤2,048、candidate3,088 B≤4,096。集合中的definition仅13 B余量，不是全域最坏上界（disabled定义多1 B、其他合法previousValue改变candidate长度）；2c组合及后续加字段必须预算复测，tenant按现PG64字符，不顺带扩列。这不是设备400 B或MCU内存结论。
- 本地证据`build/automation-a2-transaction-20261009`含PLAN、最终/中间日志、只读挂载复现脚本及审计。两个客户端初测失败是fixture漏传显式V2回应编码，修正后通过，日志保留。随机测试schema已删除/核对0，测试容器已移除；现有服务镜像/业务schema/网络/授权、App/设备源码未改，未暂存/提交/推送。旧289项/S4/已部署摘要不覆盖。
- **第37项整体仍开，下一包2c**：完整V2两个worker/签名五用途/PG/Command组合及断点恢复/并发；默认repository A1-only，allowTypedRules仅私有候选准入并显式传递，不增加环境部署开关。旧public列表遇A2会明确unsupported，故不承诺公共混版本兼容，须在2c/38冻结响应/匹配升级策略。之后才第38项App条件选择与两设备有限实物，不扩Action、多步骤、网关/通用表达式或服务器整组维护。

### 8.80 第八十批：A2完整V2软件组合、并发恢复与公共版本收口（2026-10-09）

- **第37项内部底座完成**。两个production worker、五用途真实loopback签名HTTP、actual accepted BBP/2、隔离PG及原Command/ledger组成V2链，下降沿false与float32阈值/回差均持久ACK；重复Patch、dispatch/status不多发。冷却触发消费不补发，worker重启/同session物理重连重建fence/epoch基线；丢观察ACK重试原cursor，丢dispatch返回后按原deadline仅查询status收敛。
- 新14项PG并发/恢复验证观察/执行争抢、同cursor双消费/冲突、deferred COMMIT原子回滚与丢返回、旧fence、逐次attempt、编辑/停用、真实业务Key刷新和tenant撤权恢复、混型共享容量/full graph、公开A1投影及private历史晚ACK。新helper只分责任，不造新执行器/队列或复制业务状态；设备/App源码未改，无新依赖/表/协议/环境开关。
- 修正默认public A1 list被内部A2拖垮：SQL分页前过滤policy1，所有版本仍共享graph/额度。public retire原锁定事务中绑定policy1、任何写入前校验，拒绝A2而不先提交再失败；unsupported明确409，不增加A2公开receipt。第38精确四/六字段写入、closed-union response和原意图摘要恢复冻结到Service§28.10，当前public仍A1-only。
- **23文件联合309节点（294顶层）、0fail/skip**；shared/Broker构建与三项目检查通过（Gateway checkJs=false）。**真实隔离PG125节点（124子项）、0fail/skip，约80.8秒**；首次完整运行直接通过。各自包含既有回归，软件子集、上批107及本批新增18子项不重复累加。测试Account/MQTT送达/设备ACK是显式替身，不称真实用户、网络MQTT、手机/MCU闭环或物理exactly-once。
- 16个JSONB压力样本（原8＋新增8）的实测集合最大definition含disabled **2,036 B**、runtime **1,762 B**、candidate **3,122 B**，原上限2,048/2,048/4,096 B不变；定义样本只余12 B，不是全域数值最大证明。后续变字段必须预算复测，不能扩列/容量或当设备RAM结论。
- 证据`build/automation-a2-composition-20261009`含PLAN、日志、拒绝覆盖旧日志的复现脚本、摘要和审计；原PG16/原镜像依赖＋只读candidate源码，随机schema清理后0，测试容器移除。未改业务schema、部署/网络/手机/授权/Flash、未暂存/提交/推送；新源码不冒用原S4/289项/镜像证明。
- 下一步**只第38项A2-3 public/App**，软件闭合后A2-4两设备有限差异实物。固定目标bool、同owner/tenant/shard，不扩多步骤/Action、自动采样、网关/无缝通信或整组维护；T2/P1、独立接收/iOS/Wio及正式发布限制独立保留。

### 8.81 第八十一批A2公开合同、App与原意图恢复软件收口（2026-10-09）

- **第38项A2-3软件完成；第38项整体/A2产品仍未关闭。** 沿原公开管理/原PG/两worker/Command接入四/六字段closed union，正式runtime源码统一配对A1/A2，未组装repository/service默认A1；未部署、没有新开关/表/依赖/设备wire。最新详细合同与准入取[Service§28.12](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#2812-第81批a2公开合同app与最终软件验收)，历史第80批默认仅A1状态不当作新runtime现状。
- App五类型Manifest源/固定bool目标，布尔双方向及上下阈值/正回差，明示基线/再布防/冷却不补发。当前UI新建/启停/退役/详情/恢复，不冒称已有条件编辑器。首次与retry回执均核原policy/condition/type/value/revision；修复HTTP成功便清pending的旧漏洞，原namespace/账号隔离/原ID/CAS不改，changed保留原意图、不按当前表单重组。首发明确拒绝与先前unknown重试区分。
- Gateway V2状态验证raw/typed Watch完整绑定，0/false不是missing；分页前固定版本投影、全graph共享额度不缩，public retire原锁内写入前guard。允许1→2 CAS、禁止2→1更新，原create精确重提可查询升级后的规则但绝不重新应用。删除被替代A1-only硬编码、多余catch和过时注释；旧SceneService仍有消费者保留，未重写解释器/执行器。
- 最终23文件Service **315节点（300顶层）**，App三文件 **40项**，两validator合同对照 **59项**，最终隔离PG **134节点（133子项）约103.7秒**，各自0fail/skip、不累加中间子集/重复PG。shared/Broker构建、三项项目检查（Gateway仍checkJs=false）、App/app-spec类型与严格Angular模板均通过；不是APK或真机点击。公开save→真实五用途签名/原两worker/BBP/2 accepted→原Command ACK也只是软件fixture，不是实际MQTT网络或MCU。
- PG仅随机schema、原镜像Linux依赖、只读候选mount；残留schema0、临时容器移除，现有服务/业务schema不改。16样本JSONB预算最大2,036/1,762/3,122 B，未增字段/预算，不外推全域上界或设备400 B/RAM。证据`build/automation-a2-public-app-20261009`含复现脚本、86门禁输入/87最终源码文档摘要和原/最终日志；门禁后代码零漂移，仅验收文档更新。非完整新目录闭包，原289项/S4/APK/设备证据不继承。
- 中间UI两处TS错误修正后全项目检查通过；文案调整使旧文字断言一项失败（39/40），修正对应语义断言后40/40，失败日志保留。未安装App、ADB/刷板/切网、部署、Git暂存/提交/推送；原脏工作树与已有功能保持。
- **当批下一步A2-4已由第82／84批完成，不再派发**：原准备范围为匹配候选与两普通WiFi有限实物，原操作与恢复证据保留。当前队列取§8.85，不自动重跑P1、100轮或服务器维护。

### 8.82 第八十二批A2-4匹配候选、两设备有限实物与安全收尾（2026-10-10）

- **第38项限定范围收口，不宣称自动化完整产品或正式发布。** 本批只执行第81批指定的A2-4，未新增生产运行时代码/协议/依赖/数据库表；保留原模块职责，不为“清理”删除有消费者的任务、Direct或SceneService。前批软件、旧S4/289项/APK证据保持原样，不以推送代替部署/实物。
- 新目录Service冻结291项，源码摘要`15477d2be5775f085b54ad44fc7c6c8e97f97dcca6ec9a72d13a287da5e0fab8`；本机Gateway镜像`1bd321f51654243da56e6a783ff5fbaf50971701e3cd9421a3f62691a1777b28`、Broker镜像`57501d068e588bea3995491ebcc3b24206ccc15c5a166ae3b666497d65d7b681`。只替换Gateway/Broker shard1；业务schema49、原五用途密钥/其他服务配置不改。PG/SQLite先备份并隔离恢复校验，未覆盖业务库。
- App冻结1,646项，源码摘要`9fa45279444874820350083a81cc2953d06753179476684752c73be66c73140c`；固定原锁/五插件在新目录构建，不借旧dist。APK **79,959,579 B**、SHA `66b47aaa416085561759be7fdd0358f8dcd775cd10b4a5a5899fa8c101c42e5d`，实际安装并手机读回一致。使用明确的公开debug CA、原HTTPS origin/原账号，P1/LAN=false；Android构建439任务不是439项测试，非正式签名/生产包。
- 单Arduino ZIP标准安装WiFi编译通过；stock ESP32 SDK3.3.11下两XIAO S3实际fixture编译、Flash设备端校验、原DeviceKey登录/MQTT通过。实际固件分别943,248 B，编译占用program943,092 B/static RAM51,312 B；不是峰值RAM/全平台声明。只用普通Property/report增加有限uint32样本，无自动loop采集或设备规则表。COM3源Key version2/COM11目标version1未刷新；COM4不动。新Manifest `dcddcd4b05303f0414fb982c620895ae9fa58818a199fa494f54c739b52bb997` 经真实协商，非沿用旧缓存。
- 全程一个原ruleId，真实App页面组件新建下降沿/启停；数值编辑沿App公开客户端原ID/精确CAS，不假称手工触屏或UI已有条件编辑器。revision2的true初值只建基线，真正force-stop App后true→false产生ACK1；重复false无新增。revision5初值12只建锁定基线，8再布防→10阈值边界产生ACK2，12→10锁定区不多发。
- 第一次冷却测试中10的acceptedAt=`1791607699953`，比原deadline=`1791607697243`晚2,710 ms，故合法产生ACK3；不删除、不记为抑制通过。只补一次有限窗口：10在`1791607715908`接受/`1791607716256`评估，均早于新deadline=`1791607730252`，游标/锁定被消费，执行/回调保持3；截止后9,502 ms仍保持10，无补发。原误命名快照保留并由本节纠正解释，不改时间或业务断言凑次数。
- 冷却结束后revision6停用时8→10不动作；revision7重启用时10只建基线。只复位源板，新物理连接/session、fence4及raw0基线不触发；之后0→10才ACK4。目标全程不复位，15份快照uptime单调、同一MQTT session，实际回调连续 **0→1→2→3→4**。四条PG执行与原Broker ledger逐项ACK，sequence5～8、各attempt1/status200，fresh State和回调独立取证；最后相同false写入不强求State revision增加，不称GPIO/物理exactly-once。
- 原规则最终**revision8/disabled/claimed=false/pending0/targetOutcomeUnknown=false**，全部历史保留。结束后ROM校验恢复原固件/授权：COM3从0x9000恢复1,339,392 B，COM11从0x9000恢复3,371,008 B；两块范围不同，不能写成全部重新读/回滚8 MB。原Flash备份、APK/私有数据与数据库备份均保留，未清设备Key/账号历史。
- 原APK `05071e…7606`的实际ZIP模块仅支持policy1/boolean；用实际停用revision8规则在提取的真实decoder中仍拒绝，新APK接受，A1正对照两者接受。这是字节/内存核验，不是旧App真机运行。原A1镜像也不兼容non-retired A2 graph，因此**修订原APK/服务无条件回退计划，保留匹配A2 App/Gateway/Broker作为本机候选**；不删typed记录或整库覆盖以强行降级。
- 两板恢复后移除本批独立HTTP代理、17002及新增18884绑定，Gateway原环境/映射/mount恢复，保留匹配A2镜像。续处理时Docker引擎已停，首次restore连接失败未写；用户手动启动后恢复成功。最终无关容器ID/image不变，但14个StartedAt因引擎重启已变，不能声称跨停机完全不变。原并行启动时代理占用Gateway静态IP的失败证据保留；仅修测试编排为Gateway/Broker健康后再开代理，非生产代码修补。
- 收尾实际原账号HTTP200、账户Cloudready、公开详情4ACK/停用/pending0；返回首页，释放测试State interests，停止App进程并移除本批ADB转发。**恢复原板后旧fixture状态已非fresh**，不能把收尾缓存当新的硬件运行证明。最终只读审计：schema49、47设备/7route、2rule/8execution总历史、enabled0/unresolved0，四条试点ledger仍ACK1次；备份/恢复摘要不变。
- 本地忽略证据`build/automation-a2-live-20261010`保留冻结输入、真实样本/CAS/PG/ledger、安装/恢复日志及`network-restore-verification.json`、`apk-compatibility-audit.{cjs,json}`、`closeout-audit.{cjs,json}`；准备审计的`liveDeployment=false/installed=false`只是原阶段，不代表最终状态。私有Flash/数据/网络Key不可上传或进入Arduino包。两次独立核验确认限定门禁；本批只更新既有文档，无Git暂存/提交/推送。
- 未外推below/其他scalar/中间回差带、真实ACL全矩阵/跨账号、Action/多步骤、物理传感器、BLE-only/网关/iOS/Wio、长稳或生产容量。下一步回现有S2～S4候选索引/升级回退说明和真实接收；正式服务profile/签名未给定保持待办，不重新打包或开启P2/新自动化包。

### 8.83 第八十三批匹配组件索引与升级回退交付收口（2026-10-10）

- 回到既有S2～S4，不新开自动化/服务器维护包。修正候选清单仍称“A2未部署/实物待办”和App交接仍推荐原05071包的陈旧入口；[候选§3.1](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#31-当前匹配组件索引第83批收口)成为唯一匹配组索引，App交接§8指向它，Service保留自身实物/合同并链接[升级暂停回退§7.5](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md#75-匹配候选升级暂停与回退)。原S4/旧manifest/历史失败及原脏工作树保持，不为“清理”删除有消费者的运行时模块。
- 本批实际只读复核：第82ZIP `09900bfd…bbd7` 690,345 B/416文件与stage同字节；与第73公钥修正ZIP `1e05de…23c1`相比新增/删除0、415文件同字节，唯一差异OfficialConfig.h生成profile。App包内原verify通过1,646项，manifest `9fa452…140c`/已测APK `66b47…2e5d`实际文件SHA一致；Service291项context逐项与原inputs相同，聚合来源标记 `15477d…fab8`。不重新构建/复制/安装/刷板/部署或运行真机；第82实测仅按原范围引用，不把校验数量称业务测试。
- 索引分别列Git基线、冻结来源、制品及实际环境；明确第82设备没有独立source-bundle/新目录交接重建证据、两个Service镜像ID不是registry地址/OCI归档、App公共CA overlay不在普通生产签名包。白名单只允许选择公共ZIP/profile、App冻结源/manifest/APK/三份public overlay、Service context/inputs和脱敏说明；现场build父目录含Key/Flash/手机数据/DB备份，不整体交付。结构校验不是通用秘密扫描或发布者签名。
- 升级先停用/核无未决并备份，匹配App/所有参与Broker/Gateway后才显式创建disabled A2；不声称已有生产多客户端维护写锁。暂停优先保留匹配组合/typed历史，停用或退役不能推定旧A1/旧P1兼容；不靠删unknown/规则或整库回灌凑回退。没有新增安装器、管理层、sender、运行时开关或工具框架。
- 当前设备profile为已关闭的本机临时入口`192.168.1.142:17002`，App为USB本机HTTPS公开CA调试变体，不能推荐给初学者直接“填Key上线”。正式服务/profile/信任/签名与接收环境需确定；第28项接收由同事包外实录、不代签。现有功能/来源/索引完成项冻结，只因实际输入变化或接收失败做必要差异修正，不扩P2、100轮、条件编辑器、网关或整组维护。
- 本地可复核记录`build/delivery-index-20261010/{audit.cjs,audit.json}`通过，复用现有App verify与来源清单，公共overlay证书字节/期限单独记录；它不执行Docker/ADB/Flash/网络探测/DB/Git写入，不冒称本批查过部署健康或证书链。仅更新现有文档与忽略目录审计，无暂存/提交/推送；本批源码工具链/锁/运行时代码不变。

### 8.84 第八十四批本机真实App最小自动化用户闭环（2026-10-10）

- 用户已明确以功能可用性为主线；第82限定A2-4证据不重写，第83交接索引保留，交接/生产条件不再阻挡本机功能验收。本批目标不是再证明一次引擎可发ACK，而是确认普通用户经真实App控件能完成最小流程。
- 顺序：核当前App/设备/网络及原未决状态 → 备份并恢复受限本机测试入口 → 真实界面新建布尔/数值停用规则 → 显式启用/建立基线 → MCU报告触发、退出App后执行 → App查询ACK与fresh State/回调 → 停用、重进及恢复。必须区分ADB/CDP控件交互与手工触屏；禁止直接调用页面save/client/管理HTTP代替界面验收，辅助只读PG/ledger与串口报告可独立取证。
- 原page→AutomationService→Client→Gateway仓储/worker→Broker原Command链保持；没有证实的缺陷不猜测重构。先补真实模板控件/数值转换与管理流程软件回归，软件HTTP/storage替身不得当手机/MCU实物。完整条件编辑器并非本包前置。
- 仅COM3/COM11两XIAO普通WiFi，COM4 R4不动；先精确MAC/分区/当前恢复范围备份核验，复用第82已测二进制及DeviceKey、不清授权。所有现场证据放新忽略目录`build/automation-app-ui-live-20261010`，不覆写旧wx记录、不分发私有备份。网络准备只改测试所需device-plain下发/17002和18884，不升级其他服务/迁移业务库或回退旧A1。
- **最小本机用户流程通过。** 手机原账号在实际Android WebView按“Me → Automation → 设备自动化（V2试点）”进入，所有新建/选择/输入/保存/启停/退役/查询来自生产DOM控件。select实际打开Ionic单选框并确认，number输入经原生input/change事件和生产值访问器；未调用Angular页面方法、管理client/API或写业务SQL代替交互。这是ADB/CDP程序化真实UI验收，不冒称人手触屏/键盘测试。
- 复用匹配APK `66b47aaa…2e5d`及第82两个943,248 B设备候选，无新APK/服务镜像构建；保持一个原用户MQTT、原账号HTTP。普通设备源只report/目标走原Property回调，规则持久权威/两worker仍在Gateway，Broker只观察和复用Command/ledger；设备没有新增规则NVS、执行器、权限槽或自动采样。COM3/COM11实际WiFi连接及active session/presence另查，App选择缓存不充当实时能力权威。

| 本轮真实UI与两设备门禁 | 实际结果 |
| --- | --- |
| 布尔下降沿新建 | `64b2bced-2033-4122-b51f-ebc7ac019d2c`，源power/目标power=true；保存默认disabled，明确启用后false自动快照只建基线；true重新布防→false产生一条ACK，COM11回调0→1、power=true |
| App不作为执行器 | 触发前工具依次force-stop App成功、pidof为空，随后真实源样本/PG执行/目标回调；ACK后另一次文件化PID观察仍为空。重新正常启动并通过菜单进入，详情显示该ACK。触发前PID证据在本轮原始工具顺序，不能把触发后JSON冒称触发前采样 |
| 数值控件与下阈值 | `015588e2-ba3f-484f-8658-9d7cdf6fdbc0`，sensor:uint32/below/阈值10/正回差2→power=false；实际输入零回差或空阈值时禁保存，修正为2/10才能创建；启用时0处于触发区只建基线，12再布防→10触发一条ACK，回调1→2、power=false |
| 保持触发区与停用 | 数值降至8仍在锁定区，没有第二条执行；通过页面停用后分别再送bool true→false、sensor12→10，执行各保持1，目标回调保持2。数值保持区测试已越过原冷却截止，不把它冒称窗口内冷却抑制；该窗口的历史证据仍取第82批 |
| 退役、历史与重进 | 两新规则仅通过UI确认退役，revision1停用→2启用→3停用→4退役；退役列表可查两条且没有重新启用按钮；实际离页/重进表单归空、无未确认请求，原两条停用规则保持。原有scene消费者未删除 |

- 一次保存碰到本轮Gateway配置重建，App出现原ruleId未确认、PG当时不存在该行。待服务健康后先点击“查询原请求”，仍保留；再用户确认“恢复同一请求”才创建同ID/revision1，未生成替代ID/直接PUT/删journal。这个现场中断处理通过，不算生产代码故障或重新实现恢复系统。
- 观察工具一次将初始同值false报告记为`accepted=false`：启用后的自动Snapshot已先建立false基线，同值report去重未刷新时间，PG实际seq1/occurrence0/execution空。这是工具要求“采样时间晚于begin”的局限，未把它修成设备重复发送、重新启用或触发补发；后续不同值的真实报告和ACK分别取证，原记录不覆写。
- 新增唯一代码文件是**维护测试**`blinker-app/src/app/sections/scene/automation/automation.dom.spec.ts`，生产运行时代码/依赖/wire未变：保留真实模板、Forms/Ionic与生产service/client，仅替换API、能力缓存、存储。四方向参数化流程＋无效/清空/切源共5项；同一次联合四文件45项全部通过，最终App spec TypeScript检查通过；不与原40重复累加，也不当手机/MCU证明。审查未发现应删的重复引擎；旧A1版本读取、未知意图及SceneService仍有消费者，不为“简洁”删除安全或使用中的分支。
- 状态解释：5秒是已验收样本的近期显示预算，30秒是动作冷却/租约；值不变可无新样本。因此“已启用 · 等待观察恢复”可能只表示没有近期样本，也可能worker租约失效，不能单凭它推定设备离线或规则停用。无新增样本时也没有凭空新触发；实际新变化可继续执行。此状态回读只读，不应为让页面一直显示绿色而新增心跳/改协议。
- 只读终态独立核验：schema49、47设备/7route、4rule/10execution、enabled0/unresolved0；两个新execution各1ACK，对应原Broker ledger各attempt1/result200、sequence4/5；原两条规则revision5/8、旧8执行与本轮fresh备份逐行字节一致，旧8ledger全行摘要一致。不恢复数据库、不删除历史或绕过未知预约。ACK只证明协议，串口fresh属性/回调是独立MCU软件证据，未接GPIO/物理传感器；本轮没有把旧App State缓存当新StatePage。
- 设备已按本轮已验的原始范围恢复：COM3 `0x9000+1,339,392 B`、COM11 `0x9000+3,371,008 B`，app/NVS授权材料全范围verify与备份一致并启动复位，system/分区/COM4不动。原固件被动串口无指标，因此**未独立确认原固件重新云登录**，不能把字节恢复冒称新登录通过。两份PG/Broker备份本轮仅校验导出格式/完整性及复制SHA，不宣称新的全量恢复演练。
- 收尾：两新规则已软退役、历史保留；手机回首页后停止进程，本批ADB9334/17000/18885转发已移除。仅撤本批测试网络overlay，保留匹配A2 App/服务与typed历史，不回退旧A1或迁移其他服务。`network-restore-verification.json`及恢复后只读审计确认匹配服务healthy、env/ports/mount回baseline、本批17002/18884入口消失，23个原有无关容器ID/image/StartedAt全批不变，旧代理保留；最终enabled0/unresolved0，未恢复DB或移除volume。
- 所有现场记录在忽略目录`build/automation-app-ui-live-20261010`，旧wx证据未覆盖，Flash/数据库/网络秘密不发布。**没有发现需重写产品实现的阻塞，本机最小用户目标已达成；不宣称完整产品、已有条件编辑器、所有scalar/平台、跨账号/片、Action/多步骤、BLE-only/网关、长稳或生产发布。** 当批未Git暂存/提交/推送；后续用户反馈的产品缺口已成为§8.85，不再使用本段历史“下一步”限制新需求。

### 8.85 第八十五批最小自动化产品收口（2026-10-10）

- 当前执行入口为[App产品收口方案](../../blinker-app/docs/APP_DEVICE_V2_MINIMAL_AUTOMATION_PRODUCT_PLAN.md)。参考IFTTT和米家的条件／动作交互，明确列表、创建／编辑和详情分层；主入口统一，旧场景只读归档，不冒称其保存配置已进入云规则执行器。
- 名称只进入HTTP管理合同及规则表管理元数据，迁移050与原创建名称用于同请求核对；不改设备协议、判定definition/digest、观察或原Command。旧无名称请求／记录兼容，省略名称的旧编辑保留现有名称，未知意图不补写或按新表单重建。
- 编辑沿原规则ID和精确CAS版本，只修改名称、条件和目标布尔值；已有源／目标端点保持。已启用规则修改需明确确认，初次／编辑后基线、回差、冷却和未知结果保护继续沿原实现。
- 启停以enabled为准，超过5秒没有新观察不冒称离线。默认不显示完整ID、类型／版本及内部状态；执行未知仍明显提示，技术恢复与原请求细节折叠保留。首页删除旧场景计数与默认10条假数，不新增规则数量猜测。
- M1～M4软件收口完成：App最终联合5文件59项、spec类型检查与development完整模板／资源构建通过；Service确定性相关集合117项、隔离真实PG最终139项分别通过且无skip，Gateway类型检查通过。此前各子集／中间重跑不累计；DOM的API／能力缓存／存储明确替身，不能冒称手机或设备验收。
- 原场景编辑器、动作选择器和SceneService在路由替换后经全源码消费者检索确认失用，删除7个文件；只读归档和旧用户数据保留，源码可从Git恢复。未命名设备按排序展示编号区分，不把编号或MAC作为身份。陈旧A2-4当前待办和源事实链接已纠正，历史批次／失败不删除。
- M5新版本真机短闭环仍待执行：只备份并升级匹配Gateway管理代码／迁移050和新App，真实页面做命名规则创建／编辑／启用／两设备触发／查询／停用／删除。旧APK严格模型不保证识别新命名响应，不能只升级一端；第84历史ACK不替代本批证据。设备固件及执行链无需新设计，不扩复杂规则或整组维护。
- 临时PG测试容器已移除；没有现场部署／业务库迁移／安装APK／刷板／授权清理或Git提交推送。当前现场与冻结制品仍是第84匹配组合，源码新修改不冒称已经上线。

### 8.86 第八十六批M5本机新版App与两设备产品验收（2026-10-10）

- **M1～M5最小产品包按限定范围收口。** 只执行用户指定的备份、匹配部署与真实App两设备短闭环；没有新增生产运行时代码／协议／依赖、自动化执行器或服务器维护包。原脏工作树和第82～85失败／测试记录保留，无Git暂存／提交／推送。
- 手机原APK和停止进程后的完整数据先备份（6,662,656 B，tar可读，含原IndexedDB和shared preferences）；Gateway PG 3,355,203 B与Broker SQLite 2,637,824 B fresh备份校验，SQLite integrity=ok。PG独立完整恢复后验证050及旧行保留，临时容器移除，不回灌业务库。两板按MAC／实际分区新读并verify当前app／NVS恢复范围，COM4不动，不刷新DeviceKey或清授权。
- 新Gateway镜像`468e06bb4e898193238ebac7d11fb9e35ace7a903da53ebe7690edbf59608a41`只改变6个预审管理／050文件，292项冻结输入；业务schema50。Broker仍`57501d068e588bea3995491ebcc3b24206ccc15c5a166ae3b666497d65d7b681`，原容器未重启。App新冻结来源1,641项／摘要`8a36e0576faa6d88333dd4a0dcff3137d39b756469764e3edde489c94ecce934`经新目录依赖／类型／Web及Android构建；APK **79,913,838 B**／SHA`7284b0673517a52eb8b76d4ec5ce1581ff772748ebe89495659c0e1f37ed514c`实际安装读回相同，新生产模板字节、原debug签名／公开CA、AppID／HTTPS origin匹配，P1／LAN仍false。不是release签名包或独立接收方验收。
- 复用第82两个943,248 B普通WiFi固件候选，不新造设备协议。两板原Key版本2／1登录、active session／MQTT online及最新State只读确认；源只使用普通report，目标仍原Property回调。App通过“我 → 自动化”进入，Ionic选择框／原生input事件／保存、确认及查询均为生产DOM；禁止直接调用页面／管理client或业务SQL写入替代流程。这是ADB／CDP程序化真实UI，不冒称手工软键盘。
- 同一新规则`49785c73-1f4d-4161-ba1a-b0f2a5bbd776`：revision1“M5 开关联动”停用创建→2“M5 开关自动联动”停用改名→3显式启用→4经确认改名“M5 关灯联动”、falling／目标false且继续启用→5停用→6软删除。原端点／fingerprint保持，create_name不变；编辑沿原ID／CAS而非删除重建。revision3 false基线零执行，true变化ACK1；revision4 true新基线保持仅一次执行及原冷却截止，false变化才ACK2。
- 触发第二次前独立`phone-stop`记录空PID时间`1791617285056`，随后真实源sample开始`1791617285117`、服务accepted`1791617285507`／execution创建`1791617286058`；ACK与目标回调2之后才重启App`1791617301062`。重开真实入口及刷新查得名称、条件、已启用和两条“设备已确认”。目标uptime单调，回调 **0→1→2**、power **false→true→false**；协议ACK＋应用回调不宣称物理负载动作。
- 停用及软删除后分别源1→0，样本均晚于第二次冷却截止`1791617316058`，执行／目标回调保持2，不误算冷却抑制。Broker最终源revision7／power=false，更新时间晚于负向样本，终态再核无新增执行。新规则删除历史详情有两ACK，仅返回／刷新无编辑／启用。一次历史first误点只读旧数值详情，随后明确查询新规则，不把旧记录冒充新结果。
- 首条逻辑execution **1,373 ms**内ACK、Broker ledger创建至ACK **339 ms**；原250ms ACK等待后同pending.frame有一次有界重试，PG与Broker尝试数 **2**，第二条 **1**。两条13 B frame与各PG frameDigest一致、每key仅一条ledger／原sequence4、5；均未超dispatchDeadline，没有第三条执行或重复应用回调。无逐次包日志，不能推断丢包或具体ACK延迟原因，不修改retry参数凑“单尝试”。原terminal的attempt1预期失败记录保留，改为精确核2／1及原命令绑定，不宣称物理exactly-once。
- 终态独立审计：schema50、47设备／7route、5rule／12execution、enabled0／unresolved0；新规则rev6 retired／不被worker认领，两执行ACK200；旧4rule／10execution和旧10ledger逐行／哈希与fresh备份一致。新版软件59／117／139项不重新累计；本批另有原始UI／设备／账本独立复核、只读10项证据核查及实际Android截图，不把校验项数当业务测试数。
- 恢复原板app／NVS授权并ROM verify：COM3 `0x9000+1,339,392 B` SHA`307c2e7f…514f6`，COM11 `0x9000+3,371,008 B` SHA`643cc58c…6ac7`；COM11本批fresh材料不同历史备份，不借旧SHA。system／分区／COM4不变，启动复位已发；安静原固件无登录指标，不能宣称恢复后新云登录。只撤本批proxy／17002／18884和ADB9334／17000／18885，保留新App／Gateway50／命名退休历史，不恢复DB／清App数据或降级旧APK。
- 恢复后Gateway／Broker健康，env／ports／mount回baseline，原TLS／plain代理和CA保留，24个原非Gateway容器（含停止容器）ID／image／StartedAt不变。HTTPS healthz200、未认证自动化请求401且证书可信。证据保存在忽略目录`build/automation-product-m5-live-20261010`，含私有Flash／App数据／业务库／网络参数，不能整体上传／交付。
- 后续仅修实际用户反馈或明确要求的独立交接／正式profile差异；不重新派发M5或自动扩多条件／多动作、Action、BLE-only／网关、全部scalar／平台、iOS、跨账号／片、长稳及生产发布。
