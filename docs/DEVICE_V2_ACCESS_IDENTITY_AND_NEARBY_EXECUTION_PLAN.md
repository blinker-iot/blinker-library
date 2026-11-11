# Device V2：同板接入身份、发现与附近状态执行计划

更新：2026-11-03。归属：原设备主会话。**当前取[独立接入注册方案§14.6](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#146-匹配固件及两卡有限真机结果)及[接口合同](DEVICE_V2_ACCESS_REGISTRATION_CONTRACT_V1.md)：同实例Cloud／BLE Direct分卡分ID分额度，WiFiProv BLE／SoftAP不分卡。** Cloud→BLE两卡注册／只读恢复／独立删除、正向附近及实际Cloud通道分离已有限真机通过；早期§13的缺Admin／维护固件停点已由后续批次处理，不能再据旧页首启动重复扫描或清根。源码隔离及匹配CI接线取新计划§18，远端CI与发行尚未完成。下一阶段为[BLE→Cloud有限计划](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md)，反向仍未开放。

**优先级说明：下文“同板同ID／Cloud存在即拒绝BLE／BLE转Cloud复用ID”及旧DA-1发布顺序均为历史合同，不再执行。** §15的扫描／选择取消事实保留，但其中维护固件、缺凭据和现场状态只代表当批；当前候选／授权／附近事实以新计划§14.6为准。保留owner、防重放、原根、TaskStore及UNKNOWN，不重复采样；软件S1～S3仍交[独立软件会话](../../blinker-app/docs/APP_CORE_SOFTWARE_OPTIMIZATION_HANDOFF_PLAN.md)。

## 1. 目标与边界

同一个有效设备实例，在声明支持的接入方式之间保持明确的身份和权限连续性；发现、附近、已连接、可控制和云在线不能互相冒充。复用原WiFiProv、Direct BLE、DeviceAccessStore、Cloud、CAS及只读协调，不再造绑定／扫描／控制执行链。

这里仍是设备接入主线，但修正可能跨SDK、App和Gateway，**不等于只改硬件库**。不得与另一会话同时修改Data／User服务、路由、API或共享模型；交叉文件编辑前先明确归属。设备库、配网／BLE服务、nearby、接入身份与相关Gateway模块由本会话负责；软件会话不接管这些路径。

保留账号、授权、TaskStore、云scope、CAS、UNKNOWN和完整日志；不默认解绑、清根、擦NVS、复制scope或合并历史设备。无必要不做整片备份；真正需要固件／存储写入时先明确原身份和恢复合同，只保护实际受影响数据。不发验证码、不调用模型、不自动重发接入或控制、不扩槽、不升级SDK，不自动部署或提交推送。

## 2. 必须先分清的产品与状态

| 路径 | 实际用途 | 不得推断 |
| --- | --- | --- |
| WiFiProv BLE／SoftAP | ESP32安装身份及WiFi；当前是互斥的编译选择 | 看见配网广播不等于日常Direct BLE已可用；不是所有目标同镜像动态双路配网 |
| ESP32 WiFi+BLE | 该产品图配网交接后运行Cloud＋Direct BLE | 不把两个会话建成两台设备；配网SDK与Direct owner不能同时占链路 |
| BLE-only | 独立BLE enrollment建立Admin／presence，之后Direct控制 | 不天然已有Cloud Key，不能把纯BLE入口当WiFiProv |
| 普通手动WiFi | 手工DeviceKey和WiFi的Cloud产品图 | 不默认承诺存在日常Direct广播或Admin根 |
| R4／Wio WiFi或BLE | 目前各自独立产品选择；存储连续性需按平台核 | 不从ESP32证据推出R4／Wio同固件WiFi+BLE或WiFiProv支持 |

| 用户可见事实 | 权威证据／语义 |
| --- | --- |
| 配网设备被发现 | 匹配当前配网bearer／协议的扫描结果，仅说明可尝试该接入入口 |
| 蓝牙在附近 | 最近收到、可由当前账号有效presence凭据匹配的Direct广播；不是强活体证明，不授控制权限 |
| 蓝牙已连接／可控制 | 原认证会话成立；控制还需当前权限、已接受Manifest和新鲜State，不能只靠GATT连接或RSSI |
| 云端在线 | 服务端cloudReachable；最后出现时间不等于当前连接或新鲜State |
| 当前控制通道 | 原DeviceUiPort的实际选择／会话事实，Cloud、Direct BLE、LAN分别判断；失败／UNKNOWN不跨通道自动重放 |

同板连续性以有效持久instance、账号authority及已认证的epoch／根为依据，**不按MAC、广播名、昵称、相同邮箱或物理外观自动认领／合并**。整区擦除会改变instance，但显式原DeviceKey恢复仍可能对应原logical ID，不能简单规定“换instance必建新设备”。

## 3. DA-0：当前代码事实与既有结果冻结

起始状态：**READ_ONLY_CODE_REVIEW_DONE；当时现场版本／身份／网络未新鲜核对**。后续现场结果取§9～11，不能用起始状态覆盖已安装App及一次发现的结果。

### 3.1 已有机制，不从零重做

- **BLE→WiFi复用已存在**：App `esp32-provision.page.ts:1627`先resolve instance，核原账号、本机active Admin、instance／epoch，用户确认后enable-cloud，不默认CREATE／Rotate；原测试已有对应断言。
- **ESP32保留根切换产品已有连续基础**：WiFi与BLE平台共享`bl_v2_id/instance`，相关产品共享接入根；已有根的BLE-only从Direct模式启动，不应再次走Add BLE首装。
- **实例解析有账号围栏**：Gateway repository的resolve优先当前Direct关联，无Direct才查Cloud session；enable-cloud只给唯一原Direct logical ID增加Key。多ID歧义必须显式失败，不能随便选最新记录。
- **附近及主通道已拆分**：首页以当前通道显示Cloud／BLE／LAN，附近为独立提示，控制另需fresh State；QA-2已有软件和有限手机结果，不重判“全部未实现”。
- BLE／SoftAP既有根和空根最小入网、COM11固定四槽、最小自动化及两种AI采用均已有有限结论，继续冻结；不要借本计划再做完整配网／Start／模型矩阵。

### 3.2 容易误判的配置与证据

§65／§10已安装cloud-beta制品当时只开启automation／tasks，继承`directAccess=false`。BleService的hasActiveCredential返回false、refreshPresence直接退出，ensureReady拒绝Direct，adapter监听也不启动；首页缺少bleEnabled=true便不会产生附近扫描需求。WiFiProv是另一配网链，不应与此flag混为一谈。因此**不能在该旧包未开放Direct／附近能力时，把没有附近标记直接诊断为设备广播Bug**。用户已明确这是必要功能，§12已将cloud-beta源配置改为true，**§13已匹配安装并证实实际加载，不能仅因是内测版继续关闭**。权限、有效账号凭据、真实设备能力及新鲜State仍各守原合同，启用功能不授予控制权。

当前附近实现使用约15秒观测TTL；首页有可见需求时按至少10秒节拍发起约2.5秒扫描。它不是持续连接，也不保证进入页面即刻可见；实际仍受权限、蓝牙开关、原生扫描预算及WiFiProv owner占用约束。这些是当前源码参数，本批不调频或升级SDK。

附近匹配还要求Direct-v3及对应presence key；SDK没有presence key时可回退Direct-v2，**可受认证连接不代表具备首页附近识别能力**。设备locator默认15分钟轮换，与App的15秒观测TTL是两个不同参数。已有Direct连接／opening时首页扫描可能跳过，其它设备的附近证据便会自然过期；这不证明它们已断电。Android当前共享滚动预算为30秒最多5次实际启动，code6冷却30秒，不能靠每个页面各加一次扫描绕开。

旧`DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md`中的早期Ownership／ManualDeviceKey和“SoftAP首次接入未完成”记录不能覆盖当前DeviceKey／DeviceAccessStore及入网方案§26／30的结果。以当前实现、匹配profile和新证据为准，不搬旧实验协议。

## 4. 最小执行顺序

| 阶段 | 最小范围 | 当前状态 |
| --- | --- | --- |
| DA-0 | 本地入口、产品、profile和既有证据核对 | 上述静态梳理完成；现场库存未核 |
| DA-1 | 同实例：手动WiFi先接入，再首次BLE enrollment是否另建ID | **已离线复现；防重复源码及App提示通过有限回归，自动复用不在本批支持范围** |
| DA-2 | 附近TTL／权限／生命周期与实际通道的差异 | **有限软件收口**：观察时刻TTL、后台及本地凭据变更失效，见§7；非真机正向广播验收 |
| DA-3 | 首页扫描、Add BLE、WiFiProv与Direct连接的owner交接 | **单场景有限软件收口**：WiFiProv原start未返回→stop→选择，见§8；非全owner矩阵 |
| DA-4 | 匹配单板的最短差异验收 | **常规WiFiProv BLE选择取消有限通过**：§15仅app切换、一次发现／Security1连接／14项目标WiFi及返回关闭确认；保留候选和原存储。罕见pending-start竞态、第二owner及Direct附近正向不代签，不以缺Admin阻塞纯扫描 |

### 4.1 DA-1：先处理身份方向性，不先调扫描

**初始候选及复现依据，结果已由§6补齐**：普通CREATE按owner＋幂等键新建logical ID，本身不接instance；原BLE enrollment优先复用该instance的active enrollment，没有对应分支时生成新的`ble_*` ID。由此不能保证“手动WiFi-only已有Cloud关联→第一次BLE enrollment”会复用原Cloud设备。

resolve优先Direct关联，所以一次resolve返回一个ID也不足以证明库存没有另一个旧Cloud ID。此处要核**库存数量、原logical ID、关联记录及权限连续性**，不能只测新BLE连接是否成功。

先固定同authority／账号、持久instance和合法首装证明，在原Gateway enrollment／repository与App入口的离线替身中构造最小反例：

1. 已有Cloud-only设备和该instance的会话关联，但无active BLE enrollment／Direct根；首次BLE接入是否产生第二logical ID。
2. 现有active BLE enrollment应仍复用旧ID；已过BLE→WiFi逻辑只在本次共享修改确有影响时做相邻回归，不重新跑整套入网。
3. 换账号／tenant、instance或epoch不符、历史多ID歧义、在途UNKNOWN均不得自动接管、覆盖或创建“替代成功”。

复现后先选最小支持合同：能在原owner／首装证明／唯一实例关联和事务围栏内安全复用时复用；不能安全支持的组合应明确阻断重复添加并引导原设备的受支持入口，不凭MAC或公共instance直接授权限。**不自动合并／删除已存在的双记录，不迁任务、历史或scope**；迁移另列明确授权合同。未复现则记录条件与NOT_REPRODUCED，不按静态风险重构全部身份系统。

### 4.2 DA-2：附近是有时效的观测，不是在线总开关

只用现有presence、凭据和连接快照做有限离线差异，先核：

- 有效匹配广播出现／刷新／TTL自然到期；无命中、权限拒绝、扫描受限分别呈现，不把扫描错误当设备离线。
- 首页离页／后台、蓝牙关闭、账号切换，或收到权威撤权／epoch／presence key更新后，旧证据是否失效；迟到广播／HMAC结果不能污染新scope。权限未知与已知撤权分开，不承诺离线立即感知远端变化。
- Cloud在线＋BLE附近、Cloud离线＋BLE附近、BLE已认证但State未新鲜、无Direct能力／profile关闭的组合；附近标记不得替代当前通道或解锁控制。
- 旧Direct-v2／缺presence key与真正扫描未命中分开；已有连接占用无线时其它设备的附近过期不能当整机离线，不借兼容需求改成按名称／MAC匹配。
- 已认证连接与扫描证据各守其事实，不能把connecting／ready枚举直接写成“附近”；也不因附近过期把真实Cloud连接标离线。

已有watchConnectivity的bleAccess初始化读取与后续凭据／撤权传播仍须核其实际生命周期；不凭一个旧快照当永久权限，也不为修提示而新增后台权限轮询或放宽原认证。

先用原service／home模板补缺失反例；已有通过项仅在共享路径修改影响时有限回归。没有正向真机广播证据不等于必须改TTL、循环扫描或重刷板造样本；必要的Direct／附近功能应按用户要求正常开启，权限与凭据门禁保持，不能因未完成真机验收而长期关闭功能。

### 4.3 DA-3：发现与连接的窄交接

现有ScanCoordinator已能在WiFiProv独占前排空原扫描；原生停止失败必须保留隔离。后续只选一项明确组合，核旧observer退休、配网SDK释放、新owner连接以及离页／后台清理，不能用固定等待、ready快照、忽略全部后台或任意custom请求解除围栏。

区分WiFiProv配网service、BLE-only Provisioning和Direct profile；扫描到不等于能连错service。旧GATT缓存／BLE_SCAN_BUSY／选择取消／S3服务失败只在当前版本有具体复现时续查，不整体重迁已过补丁。BLE数据长度协商／20 B回退既有结论不重测，性能有新瓶颈才另做分段测量。

### 4.4 DA-4：真正需要时才做单板短差异

先核实际USB设备、固件图、持久instance／root、App授权、服务环境和profile，禁止把§65恢复后的离线测试逻辑身份当作当前板身份。先读后写；TaskStore／scope／UNKNOWN保持，不Start、不搬表。

一个批次只选DA-1或DA-2／3的一个未覆盖差异，完整记录发现／关联／连接／实际ACK或新鲜State的层级。若只需要附近验证，不重复下发WiFi；若只需要身份复用，不重跑日期／星期或自动化。广播未出现先核固件是否声明Direct、profile是否开启及权限，不能默认清根／解绑／重建设备或反复发码。

## 5. 当前源码索引与退出规则

| 职责 | 现有入口 |
| --- | --- |
| 同实例WiFi复用 | App `esp32-provision.page.ts`的recoverExistingInstance／enableCloudForExistingBleDevice，`wifiprov-allocation.ts`及`wifiprov-admin.ts` |
| 纯BLE认领与附近 | App `device-v2-ble.service.ts`、`ble-direct/credential-store.ts`、`presence.ts`、`presence-refresh.ts` |
| 当前通道与首页投影 | App `device-ui.port.ts`、`home/components/tab-device/tab-device.ts`、`deviceblock.ts` |
| 无线owner与原生预算 | App `core/bluetooth/scan-coordinator.ts`、原WiFiProv transport／插件及原生scanner |
| 服务身份及唯一性 | Gateway `device-v2/repository.js`、`management.js`、`ble-enrollment.js`及现有迁移／测试 |
| 设备持久实例与根／广播 | SDK `Esp32WiFiPlatform.h`、`Esp32BLEPlatform.h`、`Esp32WiFiBLEProduct.h`、`DeviceInstanceIdStore`、`DeviceAccessStore`、`BleSetupLifecycle`及`BleDirectProfile` |

每项只对真实消费者确认的缺陷做最小修正，明确“源码候选、离线复现、有限回归、当前配置、真机结果”五层，不相互代签。保持模块边界，清理仅与本次差异直接相关的死代码；不新增全局设备合并器、第二套扫描器／身份表或控制执行器。无新缺陷／无自然条件则停止，不把任务扩成无边界审计。

DA-1～DA-3软件结果见下，DA-4只读前置见§9，匹配App安装及运行入口证明见§10，一次有界发现见§11；匹配单板的选择连接／pending-start差异仍未覆盖。另一会话S1～S3另行启动，本会话不代签其进度；两个会话都不自动合并分支、提交推送或修改对方文件。

## 6. DA-1执行结果：已知Cloud关联的新BLE首装防重复（2026-11-02）

### 6.1 离线复现与最小支持决定

采用真实Gateway BLE enrollment repository、原签发／vault及原合成receipt夹具，已有一条Cloud-only逻辑设备和同instance的持久session、无BLE enrollment时，旧`issue → grant → receipt → commit`实际将库存从**1条变成2条**。修前目标用例失败，保留原红测；这是离线repository复现，不是本批真机安装证明。

不能直接把原Cloud `device_type='diy'`行当BLE行复用：原`activeLogicalDevice`及恢复／commit合同要求`ble`类型。放宽类型或迁移授权会扩大权限及恢复范围。因此本批选择**拒绝新的重复首装**，不做自动合并、Cloud→BLE原ID接入或历史双记录迁移；既有BLE→WiFi及保根Direct链保持。

### 6.2 服务与App窄修

- Gateway仅在原`requireFreshClaim`的Direct／退役／reset检查之后，查该instance的持久Cloud session，JOIN仍active且`current_credential_version IS NOT NULL`的逻辑设备。命中返回`409 DEVICE_V2_BLE_CLOUD_IDENTITY_CONFLICT`，在新签发／库存写入前退出。
- session包括`pending_registration`、`active`、`revoked`、`expired`；这些关联已完成Key证明，不将challenge-only当归属。旧会话离线、过期或Key版本变化不抹去关联。查询不限调用账号，防止换账号绕过；仅`SELECT 1`，不泄漏原ID、名称或owner。
- 已有BLE身份恢复、已签intent重读／commit和UNKNOWN只读协调保持原合同；不对已签请求追溯新增拒绝，不改Method 2、device_type、CAS或DeviceAccessStore。**只保证新事务读取时已有的Cloud关联被围栏，不宣称跨Cloud／BLE并发全局唯一**。
- App只在BLE guide按HTTP409＋专用code显示“该设备已有云端接入记录，请核对并使用原设备记录；不会自动合并或重置。”，隐藏该原因的重试按钮并保留返回。普通失败／RESET及错误status继续原行为；离页、账号变化和显式新操作清理旧原因。现有pending恢复入口不删除、不自动重试／跳转。11个已注册语言各补一个专用键，未改通用鉴权、Data／User、路由或软件会话入口。

错误反馈仅采用UI／UX指引中的明确原因与恢复方向，不做页面重设计、不引导清根／解绑。服务合同同步在[API说明](../../aily-blinker-services/docs/API.md#已有cloud实例的纯ble首装围栏da-1源码候选)。

### 6.3 有限验证与证据层级

| 验证 | 实际结果 |
| --- | --- |
| 旧repository真实issue→commit反例 | 目标1项失败：库存2≠1，修前缺陷已复现 |
| 新Cloud连续性回归 | **12／12通过**：四session状态、跨owner不泄漏、不同instance、退役墓碑不复活、challenge-only、旧Key关联、原BLE恢复及已签pending例外 |
| 旧BLE signer／intent CLI相邻回归 | 通过；其中真实PostgreSQL因未设置TEST_DATABASE_URL明确跳过，不计数据库通过 |
| 原BLE guide模板红测 | **5失败／4通过**，专用分类／按钮／单键未满足；无fixture编译错误 |
| App新模板9项＋原page15项 | **24／24通过**；精确错误映射、译文碰撞、无额外认领及生命周期清理 |
| 原依赖App严格模板／类型检查 | `ngc --noEmit`通过；不是APK或发布构建 |

服务新增有限回归已接入`test:ble-enrollment`及原`test:device-v2`入口，不要求运行整个历史套件。各队列只运行一次，期间生产输入和两仓锁文件哈希保持；旧红测不覆盖。独立只读审阅未发现必须追加的修改。

证据在Service忽略目录`build/da1-cloud-ble-continuity-20261102/`的red／green／adjacent、App red／green及aot意图、日志和结果。全部是离线合成夹具；未接真实数据库、手机、设备或服务器，不读取账号／Key、不发送验证码／模型请求、不部署、刷板、提交或推送。SDK、锁／依赖版本、原授权、TaskStore、scope、UNKNOWN和完整私有日志未改，无关dirty保留。

### 6.4 剩余边界与下一步

**本批软件防重复已完成，不能写成同板所有接入方式自动统一已完成。** 部署前只补一次隔离PostgreSQL事务／实际查询计划核对；当前`v2_sessions`无device_instance_id前导索引，是否需要窄索引以实际计划决定，不连接业务库或自动开Docker，不扩成资源审计。随后需要匹配App／Gateway受限验证时另定最短差异，不为凑样本重配或改原根。

该批之后的DA-2软件差异已由§7执行；DA-1真实PostgreSQL／匹配部署欠证不因此关闭。不默认串行启动真机矩阵或自动身份迁移。

## 7. DA-2：附近证据过期、后台及凭据切换的软件收口（2026-11-02）

### 7.1 已复现问题与不变合同

使用原DeviceV2BleService、实际本地凭据Store、原Direct广播解析／匹配及受控扫描／HMAC替身，确认三类缺口：

1. 后台只取消扫描／连接，既有附近证据仍可保留至原TTL；显式后台refresh也可进入存储／扫描。
2. TTL从HMAC完成时起算；广告验签排队超过15秒仍能被显示为新附近，延迟5秒也会获得额外15秒。
3. 同账号成功删除、改pending、更新epoch或presence key后，无本地变更通知，旧附近／在途旧Key仍可发布；watchConnectivity的bleAccess只读一次也会沿用旧值。

原账号切换、adapter关闭、权限拒绝、miss／busy／throttle不续TTL及停止失败隔离已有机制。本批保留它们，不重写认证／控制、不将附近当Cloud在线或可控制证明，不更改15秒TTL、10秒需求节拍、2.5秒扫描窗口和原生额度。仍保留Direct-v2／缺presence key及profile关闭的支持边界。

### 7.2 三个现有模块内的最小实现

- `credential-store.ts`：在原saveForScope／remove持久写**成功后**发送无重放、进程内的窄失效通知，仅authority、accountId、logicalDeviceId。replacePresenceKeys沿save单发，失败不发，跨Store实例共享；不携带或持久化Key、secret、epoch、receipt，不新增数据库／身份系统。通知ID取同步封存且实际写入的stored记录，不在await后重读可变caller对象。
- `device-v2-ble.service.ts`：先检查Direct profile，再核当前账号／authority；仅清受影响ID的附近证据，取消包含该ID的在途presence batch，复用原abort／account／finished及HMAC排空后抹钥围栏。后台同步清证据并拒绝新的后台refresh；返回前台不自行发起扫描，仍由原首页可见需求决定。root服务销毁时释放订阅／旧观察。
- 同Service在广播observer回调入队前记录performance.now，截止为**收到该观测时刻＋15秒**；验证完成时已过期则不发布，否则只保留剩余时长，也不缩短更晚观测。这里是App收到回调时间，不是卫星时间或设备强活体证明。
- `device-ui.port.ts`：消费Service的局部失效信号，先将旧bleAccess置unknown，再读取当前**本地**凭据。read序号＋closed阻断旧结果回填，退订后不再读；不新增HTTP、扫描、连接、控制或业务请求重试，不改变原纯BLE初始投影／实际通道选择。收到附近失效也不擅自终止仍有效的已认证Direct会话。

仅原模板状态投影的事实语义变化，未改HTML／CSS、主题或语言文案。UI／UX技能用于避免旧状态误导，不引入新的页面设计、重复live region或移动焦点。

### 7.3 有限回归及结果

| 阶段 | 实际结果 |
| --- | --- |
| 原port的本地访问变更用例 | **3失败／1通过**：旧值不退休、没有新读，确认缺口 |
| 原Service的生命周期用例 | **8失败／2通过**：后台、观察时刻TTL及4种同账号变更失败；scope隔离／原账号事件通过 |
| 最小修正后的8文件唯一队列 | **116／116通过**：新增14项＋原相邻102项，包括凭据存储、presence、刷新需求、首页和DeviceUiPort |
| 审阅发现的可变caller-ID通知归属 | 仅补1项，**修前失败→修后1／1通过**，前10项未重复执行 |
| 最终源码原依赖严格模板／类型检查 | **ngc --noEmit通过**，无APK／发行包构建 |

首次联合红测的Service文件因hoisted mock中的编译spread helper未初始化而未注册用例；原日志保留，**不计作产品红测**。只将mock展开改为等价Object.assign，再单独执行此前未运行的Service文件；port未重复。其后原用例预期保持。116项在固定通知ID表达式修正前通过，随后只对该窄归属修正做1项定向验证；最终严格编译使用最终源码，不冒称再次跑完全部116项。

意图、红测／夹具停点、green、单项归属red／green及aot结果在App忽略目录`build/da2-nearby-lifecycle-20261102/`。生产源码及锁在各队列运行期间哈希保持；保留无关dirty及凭据Store原首装修复。未安装依赖、升级SDK、连接手机／设备／服务／数据库、发送验证码、部署、刷板、提交或推送；账号、授权、TaskStore、云scope、CAS、UNKNOWN和完整私有请求日志未操作。

### 7.4 收口边界与下一批

**DA-2本批有限软件差异完成，不等于真实无线附近／远程撤权全平台验收。** 事件只覆盖此App进程通过现有Store完成的本地写入，不轮询远端权限、不承诺离线立即知道其它手机／设备上的撤权，也不证明GPS／MAC或广播强活体。失败存储不伪造成功通知；真实权限及控制仍由原认证链决定。

当前匹配cloud-beta的directAccess仍false，未为测试强开；只有声明支持Direct-v3且有匹配presence凭据的后续候选才可签正向广播。DA-4需要时另按实际profile／单板选最短差异，不重配或清根造样本。

DA-3已按§8选择并完成一个未覆盖的原WiFiProv扫描→选择交接；原首页presence→中央排空已有测试，不重复作为新缺口。下一范围为DA-4现场前置核对，不默认同时部署DA-1或启动真机矩阵。

## 8. DA-3：WiFiProv原扫描启动到选择连接的窄交接（2026-11-02）

### 8.1 选定场景及修前证据

原中央ScanCoordinator已覆盖排空、晚start、停止失败及外部lease，原WiFiProv页也覆盖离页时排空启动；本批不重写它们。新增的唯一组合是：**同页扫描已产生设备结果，但WiFiProv.startBleScan的JS Promise未返回，用户立即停止或选择设备**。

Android插件先调用startBleDiscovery，发现回调可以notify设备；start调用回执是另一桥接结果，并无“发现事件必在该Promise返回之后”的合同。原App已phase=scanning且允许选择，但stopBleScan只调用stop，没有等待原discoveryStarting；原start稍后仍可安装12秒计时器，重复stop还会各发一次native stop。这里是已确认的App异步时序缺陷，**不据此宣称已定位历史S3 GATT失败或证明原生必然重叠扫描**。

原页面／原方法的4项有限红测全部失败：start未排空即stop；已停止的旧start遗留计时器；pending stop被重复调用；离页同样提前stop。夹具仅模拟插件回执／发现结果，connectSession以替身截断，未触及身份或WiFi写入。

### 8.2 单文件最小修正

仅改`esp32-provision.page.ts`的扫描薄接线：

- 公共stopBleScan共用本次discoveryStopping Promise；首次调用捕获原starting及页面generation，原start排空后才调用现有WiFiProv.stopBleScan。选择仍需原stop成功、同generation及selecting阶段，失败继续closeFailed，不以等待时长／ready判断释放。
- pending stop纳入原operationBusy，原模板已有disabled绑定，阻止停止未确认时重复启动／选择；没有新增按钮或改布局。异步操作期间禁入采用UI／UX技能的防重复操作指引，未重做整页。
- start续体核原Promise身份、generation、当前scanning且没有pending stop，才安装原12秒计时器；停止排空后再清计时器。新扫描仍保留自己的完整12秒预算，不改扫描频率或原生配额。
- 离页清理顺序补等待pending stop，再走原native close／clear／lease release；未知物理停止仍保留原隔离，不自动启动、连接或重发。

不改ScanCoordinator、scanner、WiFiProv插件／SDK、Direct transport、身份、CAS、TaskStore或控制执行链。扫描独占lease没有被扩大成全局GATT连接权限；其它组合没有新复现就不加全局gate或新owner系统。

### 8.3 有限验证及下一步

**4项新红测→修后4／4通过；连同原discovery6、lifecycle13、ScanCoordinator13，共36／36通过。最终原依赖ngc严格模板／类型检查通过。** 测试仅核原select→connectSession调用边界，不是实际GATT、安全会话、配网、云上线或ACK验收。旧停止失败、离页／缓存重入、中央lease排空等相邻保护通过，不重跑DA-1／DA-2、完整配网或自动化。

证据在App忽略目录`build/da3-scan-owner-handoff-20261102/`：red、green、aot意图／原日志／结果。源码／锁在队列期间保持，HTML、中央扫描器、Direct transport及DA-2服务／凭据实现、profile和锁均未改。保留该页原有dirty，不升级SDK、不安装依赖、不构建APK、不部署、连接手机／设备／服务器、发验证码、提交或推送；账号、授权、TaskStore、scope、UNKNOWN及私有请求日志未操作。

DA-4的只读现场前置已按§9核对；匹配安装随后已由§10完成，真实bearer／授权仍待条件。具备BLE配网候选后才单独安排扫描→停止／选择→取消，不下发WiFi、不解绑／清根或刷板造样本；附近正向另需Direct-v3／presence及对应profile。DA-1隔离PostgreSQL及部署前欠证保持，不并成全矩阵。

## 9. DA-4：现场只读前置核对与匹配门禁（2026-11-02）

状态：**PREFLIGHT_DONE_RUNTIME_GATES_NOT_MET**。本批仅核现场和制品，不改产品代码、不做扫描／GATT／配网或控制，不把前置完成写成真机验收通过。

### 9.1 新鲜现场证据

| 项目 | 本批实际结果与限制 |
| --- | --- |
| 手机／App | ADB在线，beta进程在，package为iot.diandeng.tech.beta、versionCode1／1.0-beta；版本名不能代替源码匹配 |
| base APK | 通过pm path＋手机sha256sum，精确为`846a6126fa3027b4590c7e520b99bdeec35e518ccbbe6e2db09cd6673056b5e7`，即§65制品，未复制APK作备份 |
| 原生输入 | 复用该精确APK的既有制品／ZIP证明：WiFiProv注册在包内、SDK2.4.4及原证书匹配；不冒称本批重新编译DEX |
| Android当前用户0 | BLUETOOTH_SCAN、BLUETOOTH_CONNECT、ACCESS_FINE_LOCATION均已授权，bluetooth_on=1；不将包信息中其它用户的拒绝状态混到当前用户 |
| 页面可见性／活跃Web资源 | 初次窗口核对为NotificationShade遮挡；一次只读CDP资源入口超时，未取得活跃页面资源哈希，不关闭通知栏／导航或重试，不能排除另有Web OTA |
| USB | 仅识别COM11，Espressif USB VID303A／PID1001；这只证明串口设备在线，不证明具体固件、instance或根 |
| 固件／原授权 | 未打开串口、复位、读Flash／NVS或连配网设备，因此当前bearer、instance及Admin均未新鲜核实，不读取秘密或借旧scope补造结果 |

CDP只创建一个自有临时forward，超时后已精确移除，收尾forward列表为空；未操作用户已有会话或转发。原账号、授权、TaskStore、scope、UNKNOWN及完整请求日志未操作，不需要为这次元数据读取做额外账号／整片备份。

### 9.2 版本与固件门禁结论

精确旧APK的公开配网页资源为`chunk-Djm0r4GR2.js`／SHA`4281a536…2c22fbd`，存在discoveryStarting但没有DA-3新增的discoveryStopping／stopDiscovery。因此**旧base APK不能作为DA-3匹配验收包**；活跃Web资源核对超时又没有补足此证据，不能直接签当前页面已运行修正。

本机当前必要源码／HTML、原扫描器、Direct transport、DA-2服务／凭据、profile及锁仍与DA-3最终严格编译的冻结输入精确一致；本批未重跑回归、重新构建或安装。源码cloud-beta仍只覆盖automation／tasks，directAccess=false；运行时flag本批未取得，不能凭没有附近标记认定广播故障，也不顺手开放Direct。

COM11最新可追溯的公开恢复记录仍为§55.7的原程序及完整20 KiB NVS校验；该恢复BIN来源是SoftAP＋四槽运行候选，**不是BLE WiFiProv候选，也不是当前新鲜固件证明**。§63／65 Development临时设备已不由该板承载，不以其Manifest／logical ID冒充当前身份；普通编译夹具也不能刷入或作为恢复目标。

证据仅在App忽略目录`build/da4-access-preflight-20261102/`：live-inspection保留已通过的包／权限读取及CDP超时，preflight-summary保留冻结输入匹配和未过门禁。未读取secure／journal或账号凭据，没有发码、登录、业务HTTP、服务器／数据库操作、依赖升级、Git写入或恢复旧UNKNOWN。

### 9.3 当时的下一批最小范围：第1项见§10，第2项进展见§11

1. 先另行安排**一次当前源匹配cloud-beta构建及同签名保留数据安装**；沿原脚本和SDK，冻结DA-1～3必要输入，核打包配网页、原生WiFiProv、HTTPS／注册表及签名。安装前才保护实际会影响的账号／完整journal，不清数据／自动登录；§9只读批未执行，后续单次构建安装结果见§10。
2. COM11先保持原状。匹配页面可用后，先区分实际bearer与当前账号本地Admin；**DA-3只读扫描／停止／选择／取消不要求Admin，不能让缺Admin触发登录、补绑或阻塞这一独立差异**。已有匹配owner自然可用时才经固定GetInfo读取instance／epoch，Cloud隔离关闭后才同账号resolve／Admin GET核连续性；不按MAC／名称合并。没有活跃通道可仅作一次原WiFiProv有界发现，不凭未命中断言SoftAP或硬件故障。若实际仍是SoftAP，不能拿它签BLE差异，不默认换固件、清根、解绑或搬scope；确需固件变更须先明确必要范围及原NVS／授权合同。
3. 条件齐备后只选DA-3扫描→停止／选择→取消的最短差异，不安装身份或发送WiFi、不重复已通过的完整配网／任务／自动化。仍无自然匹配条件则明确欠证，不靠刷板造样本或旧包销项。DA-2正向附近及DA-1隔离PG各守自己的前置，不自动并行展开。

## 10. DA-4：匹配App单次构建、保留数据安装及运行入口确认（2026-11-03）

状态：**MATCHED_APP_INSTALLED_RADIO_ACCEPTANCE_PENDING**。本批完成§9.3第1项，不将制品／安装证明代替扫描、GATT、身份或WiFi真机验收；COM11保持原状。

### 10.1 单一队列与制品

安装前经正常前台入口保全当前账号的完整7项SecureStorage及两份IndexedDB store，并保存完整阶段logcat。账号／区域动态取自当前会话，不沿用旧脚本的测试账号常量，不要求近期认证或发送验证码。

复用原prebuild→cloud-beta Web→Capacitor sync→`:app:assembleBeta --offline --no-daemon --max-workers=1`，各阶段仅一次；不重跑DA-1～3已通过的回归／AOT，不下载依赖或清理缓存。14项必要源码、11语种、原插件输入、SDK和锁均冻结；全工作树／HEAD／索引在队列期间保持，原生生成文件无须恢复EOL或覆盖原修改。

| 制品核对 | 实际结果 |
| --- | --- |
| APK | `iot.diandeng.tech.beta`，77,894,008 B；SHA256 `0cd3c16e6e702e8facb6f2afedfa4dd8781a5e50e1d7bf41ca771c2cae461564` |
| SDK／签名 | WiFiProv仍2.4.4，esbuild仍0.28.2；同原证书`a76639ec…5d5d210`，未升级SDK或改锁 |
| 原生及公开配置 | WiFiProv注册表、必要DEX入口、HTTPS／禁止明文及原云地址核对通过；诊断仍关闭 |
| Web入包 | 1,158项APK／原生同步／Web资源逐字匹配；配网页`chunk-CtGHB29k2.js`／SHA`0404b53a…0fd95e7`含DA-3的starting／stopping／stopDiscovery／select屏障 |
| profile | cloud-beta仍只覆盖原automation／tasks，Direct仍false；本批不开放附近／Direct |

### 10.2 唯一安装及实际运行证明

安装前再次读取当前旧APK精确SHA、账号／区域、安全存储及完整journal，确认与本批保全一致后，**仅一次`adb install -r`**保留数据安装；手机原位新APK哈希与上表精确一致。不卸载／清数据、不导入旧token，不登录或调用配网／控制写接口。

安装后初次CDP入口尚未就绪，`fetch failed`停点原样保留。随后一次显式只读协调已取得前台首页及完整存储，但Page.getResourceTree仅列classic脚本、图片和字体，没有列模块main；**该工具停点不判产品故障，也不据此宣称仍用旧OTA**，没有重装或重建。

仅补一个独立只读入口：在当前顶层frame的默认执行上下文中，通过Debugger.scriptParsed／getScriptSource读取**已经加载的VM主脚本**，不fetch、导航、reload或注入产品逻辑；75个既有脚本事件内精确找到一份候选main。原始UTF-8源码不做trim／换行修整，SHA与不可变APK中的主脚本精确一致：`main-E5UZLH2E.js`，798,742 B，`cbc5d901af3aec3c25e7d9eeac473e6c59e339d9759f374772f04cd3cb8fb7a4`。因此当前App主入口已与候选匹配；**配网页只核入包，未导航／执行，不能代签其无线行为**。

### 10.3 保留、收尾与下一范围

- 同一账号及区域保持，7项SecureStorage逐字一致，包括原有授权／持久意图；两份原IndexedDB store全部保留。本批原store共0行、结束仍0行，如实记录，不把空日志库说成已有大量请求样本。前／安装／协调／收尾完整logcat各自私有保存，未清logcat或删除旧证据。
- 正常保留匹配App；临时常亮恢复原值0，自有CDP／Debugger／Runtime监听及forward全部释放，最终forward列表为空。COM11未打开串口、复位、读写固件／NVS，TaskStore／云scope未动；没有服务器部署、验证码、模型调用、Git提交或推送，无关本地修改保留。
- 证据在App忽略目录`build/da4-matched-app-20261103/`：candidate-cijVzS保留原单队列／制品；phone-private-VKF7xF保留ACL私有快照、全部停点、VM源码哈希证明和收尾；`installation-summary.json`汇总两条独立证明。build-only报告仍标installed=false，不改旧报告来代替实际安装证据。

**§10完成时的下一范围（当前进展见§11）只接§9.3第2项**：复用已安装App，分别核COM11实际配网bearer及当前账号原Admin。原GetInfo／同账号resolve只读条件成立才读取，不调用身份或WiFi写。**Admin限制的是原身份连续性／WiFi写，不是DA-3扫描选择本身**；无活跃通道时最多一次原页面有界BLE发现，不为补Admin重新登录或建身份。若仍SoftAP，不能签DA-3 BLE差异；先明确必要候选／原NVS授权合同，不默认换固件或清根。条件齐备再只做扫描→停止／选择→取消，不重跑完整入网、任务或自动化。DA-1隔离PG／Gateway发布及DA-2正向附近仍分别欠证，不自动展开全矩阵。

## 11. DA-4：当前账号／通道前置、有界发现及前置条件纠偏（2026-11-03）

状态：**ONE_DISCOVERY_STOPPED_MATCHING_BEARER_NOT_CONFIRMED**。沿用§10已安装App，无构建／安装或设备存储写入；本批只签下列实际读取与发现，不把0命中或缓存元数据当设备身份。

### 11.1 新鲜只读事实

| 事实 | 结果及限制 |
| --- | --- |
| 手机／USB | ADB手机在线，仅COM11／Espressif USB VID303A、PID1001；未打开串口／ROM、复位或读写Flash／NVS |
| App | 手机原位APK仍为§10精确SHA，DOM主入口仍main-E5UZLH2E.js；本批不重复构建或VM源码核验 |
| 当前账号／凭据 | 区域Development，安全存储7项；**整个App凭据前缀下记录0项，不仅是当前scope为0**。因此没有可用本地Admin供本批核原根连续性；不推断板上根已丢、凭据损坏或需要重认领 |
| 原插件能力 | getCapabilities确认BLE、SoftAP及Android手动固定读取可用；仅是插件能力，不代表COM11当前固件图 |
| 当前选择／网络 | 原getDeviceInfo正常返回“No ESP device is selected”；没有选择或连接。网络快照softApAttempted=false、processUnbound／internet／validated=true，仅是手机进程事实，不作为隔离已解除、Cloud可达或认证成功的证明 |
| 固定GetInfo／Cloud协调 | 没有自然已连接的匹配owner，因此未调用GetInfo、resolve、Admin GET或Key接口；instance／epoch／实际bearer仍未新鲜确认 |

历史§55恢复BIN的源码确为SoftAP＋四槽，但不能冒充当前固件。该运行Sketch只有串口`A`诊断入口，没有只读身份命令；旧“被动”串口脚本会设置DTR／RTS，不能承诺不复位。因此没有为读身份进入ROM、发送`A`、重新开启配网窗口或调用resetNetwork；后者还会清WiFi，超出本批读取范围。

### 11.2 关键纠偏：Admin不是DA-3扫描选择前置

复核原page的startBleScan／selectBleDevice／connectSession：普通工具路由先走权限、原发现lease及WiFiProv扫描；选择时校固件PoP、排空start／stop，再原select／Security会话及目标WiFi只读扫描。**这一段不读Admin、不创建身份、不reveal Key，也不因Direct开关关闭而失效。** 身份、CAS及WiFi写在后续provision／installBlinkerIdentity才进入，不能混成扫描的前置。

因此缺Admin只限制“原身份／根连续性或后续WiFi写”的结论；不能让这次只读差异反复要求登录、补绑或获取Key。§9.3及§10.3已同步纠正。实际选择连接的前置是**可确认的WiFiProv BLE固件与匹配Security／PoP**，不是Direct-v3／presence或控制Admin。

### 11.3 一次原页面有界发现

蓝牙原已开启，原checkPermissions的BLE必需权限已满足，未弹授权或更改系统权限；非必需字段的granted遵循插件合同，不外推为对应OS权限全授。经普通`/tools/esp32-provision`路由，只点击一次原“扫描 ESP32”，未更改前缀、PoP或设备能力，也未使用重新配网路由。

原WiFiProv事件为**started → completed → stopped**：SDK完成事件观测约10.044秒，原页面最终stopped事件约12.039秒，均为同一App performance.now的接收时间差，不是射频仪器测量。保留原预算，不调频或靠固定等待宣布停止。设备列表0项、结果事件0；未选择／连接设备，没有进入GetInfo、身份或WiFi写入入口。随后正常离页回首页，移除本批两个观察listener并关闭自有CDP；原页面停止／离页完成，不再重扫。

结论仅为**一次原页面有界发现和停止／离页通过**。不能由0命中断言COM11为SoftAP、蓝牙故障、板未供电或固件损坏；配网窗口也可能未开启。没有设备结果，故不能签DA-3“结果先到、start Promise仍pending”的真机竞态、选择／GATT、安全连接、完整入网或历史S3连接失败已关闭。没有确认新产品缺陷，本批不为了产出修改重写扫描器或身份链。

### 11.4 保留及下一批最小范围

7项安全存储及两份完整IndexedDB store前后逐字保持，原0行、结束0行；本批只落盘指纹／非秘密摘要，内存快照用于比较，不复制一份新的账号秘密备份。完整阶段logcat保存在ACL私有目录，未清日志。未更改常亮、账号、网络、原授权、TaskStore、scope、CAS或UNKNOWN，未发送验证码、升级SDK、部署服务、提交或推送。最终forward列表为空，无关本地修改保留。

证据在App忽略目录`build/da4-bearer-admin-20261103/phone-private-4Ig42V/`：scope-summary、native-context-summary、discovery-result／observation、page-left、preservation及完整私有logcat。独立只读核对与上述范围一致，不读取或公开token、controllerSecret、DeviceKey或locator。

**下一批只准备一份真实匹配的COM11 WiFiProv BLE运行候选及切换合同，不再重复本批发现。**

1. 复用原SDK及既有产品执行链，仅选择BLE WiFiProv bearer；不是BLE-only／Direct，也不是编译夹具。冻结实际Security、PoP来源和配网窗口触发方式，不从MAC／名称认领或猜PoP，不为扫描创建Admin。
2. 明确原NVS中的instance、Key、根、TaskStore／scope保持和恢复合同；四槽原Manifest、签发公钥／profile及任务生命周期不能因换bearer被顺手替换。真正刷写前才保护实际受影响NVS，不做整片或账号重复备份；不在本批默认刷板、清根或执行任务。
3. 只在候选与实际板、空间／profile及原存储兼容条件齐备后，另作一次扫描→停止／选择→取消；允许原连接后的目标WiFi只读扫描，不安装身份或下发WiFi。正常无pending-start时如实记录，不人为延迟原桥接或为凑竞态循环采样。

DA-1隔离PostgreSQL／Gateway发布及DA-2正向附近分别保留，**不同时展开第二条队列**。本批不能作为同板身份自动统一或所有owner组合通过的证明。

## 12. DA-4：BLE四槽运行候选、非破坏开窗及必要功能启用（2026-11-03）

状态：**BLE_RUNTIME_CANDIDATE_LINKED_NOT_FLASHED**。本批仅准备和离线验证；原SDK完整链接及消费者核查通过，不连接手机／开发板、不扫描、不刷板或部署。旧NVS、账号、TaskStore、UNKNOWN及所有原日志未操作；正式准入未改变。

### 12.1 不能仅换一个BLE宏

以§55已验`f8783fbe…835eaee` SoftAP四槽隔离库／Sketch为基线，不用当前整棵dirty源树替换。审阅确认两个原阻点：原TaskConfig／生成profile只准SoftAP；原产品有Key＋active WiFi时直接进入Active，10分钟不是每次重启必开的窗口。不能用resetNetwork、resetAccess、空NVS或假造hasActiveProfile绕过它们。

最小必要接线：

- 原profile工具增加显式`--bearer ble`，要求原内部selector与`BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS`均精确为1，仍固定ESP32／WiFiBLE／四槽／HTTPS＋MQTTS／P256 keyId1／双根；普通Sketch仍0槽，原SoftAP默认及其它平台边界不变，未开放服务器准入。
- `InitialProvisioningWindowIntent`为1字节、无I/O／heap／clock的RAM意图。产品内部`requestInitialProvisioningWindow()`只在首次attach前、Stopped且无pending stop时允许；重复及运行中拒绝。start通过原基本门禁后消费一次，显式绕过已配置网络的Active捷径，随后仍走原provisioner、TaskRootOwnerGate及确认停机。没有公开Facade新API、串口开窗命令或第二套生命周期。
- Sketch仅切BLE bearer，并在原enableTasks／begin之前请求一次窗口；请求失败不begin。原`power`及`uptimeSeconds`声明、配置顺序及未arm的原诊断A代码保持。设备不内嵌DeviceKey、SSID或WiFi密码。

### 12.2 PoP、窗口与任务停驻合同

| 项目 | 固定合同 |
| --- | --- |
| PoP | 原已验私有`PrivateProvisioningConfig.h`逐字复制且校SHA，非空、Security1；不新造或猜测，不在Git、公共文档或输出中公开。将来仅经正常App PoP输入框使用，不需登录验证码或读取DeviceKey |
| 窗口 | 从原provisioner成功begin后计600,000 ms；候选每次boot的setup均显式请求一次，**不是永久只开一次**，不作为普通长期固件 |
| 取消／到期 | App取消只关手机链路，不冒充板端窗口已关；到期仍原Fault／NotConnected和guarded停机，不自动重试／宣称成功，也不以固定等待或ready解除owner隔离 |
| 四槽 | 原四槽、存储策略及双根不变；维护Provisioning阶段原extension将taskReady=false，任务存储／管理／执行不准入，不Start或改任务状态。属性声明保持不等于本批取得新Hello／wire Manifest，后续还需核原指纹及scope |
| 普通运行恢复 | 本候选不自行重新arm。显式end→同绑定begin只有原native停机／任务owner排空后才可恢复；本次不新增自动恢复逻辑。最终恢复指定原应用后，按最新保留NVS正常启动，不靠清WiFi退窗 |

签发使用原公开P256 keyId1，SPKI SHA`02879e20…b6d061`；原host／CA／TLS及签名point／ring正文保持，仅内部候选版本及构建身份变化。这是**冻结编译输入**，不冒称本批新鲜核过实际Gateway签发或正式准入。

### 12.3 有限验证及唯一候选完整链接结果

- App当前cloud-beta只新增`directAccess:true`，原账号／权限／能力／State门禁、扫描频率及其它功能开关不动。既有配置模板先1红／4绿，修后5／5通过；本批没有重建／安装App，手机旧包仍false。**不能因为是内测版关闭必要功能，也不能用源配置已改代替装机已启。**
- 新启动意图8项行为用例通过（MSVC原工具链、1个CTest目标），覆盖默认不请求、重复拒绝、启动边界、一次消费及消费后不可再arm。原profile8项有限用例通过，含显式BLE及原SoftAP／零槽／TLS围栏，没有跳过预处理器；这不是native开窗真机证据。
- 原Core3.3.11／XIAO ESP32S3 default_8MB、jobs1首次编译，被真实`BlinkerFacade.h`旧SoftAP入口围栏拒绝，首失败原日志／prepared保留。仅补同一显式BLE selector分支，原manual／SoftAP／R4／Wio／BLE-only边界保持；新增真实Facade预处理矩阵1方法／19子例通过，未重跑前8项。
- 第一次显式续编仍在真实Facade入口拒绝：基线f878的Root未在Facade之前引入OfficialConfig，内部BLE selector尚未可见。复用当前源码中**既有**的`BlinkerWiFiBLE.h`正确顺序，不新增全局宏或提前污染无产品选择的库翻译单元。新增真实Root→生成profile→真实Facade回归1方法／BLE及SoftAP两子例通过，没有重跑前8项或19子例。
- 第二次显式续编完成**原SDK完整链接及原消费者核查**。两次续编均沿同候选、同build目录、同CLI参数和缓存，无clean、第二份候选或并发重复队列；旧prepared／失败日志／runner不覆盖。相对f878仅5个必要源码头及4个生成头，最终逐项SHA核对通过；原依赖和分区容量不变。

| 最终制品／边界 | 实际结果 |
| --- | --- |
| 内部profile／SDK | `esp32-ble-task-da4-v1`／`da4-ble-task-link-20261103`；原ESP32 Core3.3.11，CLI1.5.1，XIAO ESP32S3 default_8MB，jobs1 |
| 原始应用BIN | 1,413,296 B；SHA256 `5f9a9637a506f4ab4eff18bf4b27c9a93250b3c5910cf5792c17349dd5734990`；不是合并刷机包 |
| 编译资源 | 编译器Flash占用1,413,144 B，静态RAM60,492 B；应用起点0x10000，编译容量3,342,336 B，未放宽门限 |
| 实际消费者 | BLE配网scheme存在、SoftAP scheme不在本候选；原四槽TaskSnapshotStore／任务执行／TaskServiceConfirmation、双根生命周期及唯一产品实例1均通过核查；新开窗意图与实际Root／Facade已被消费 |
| 不能代签 | 仅冻结原标量属性声明，不是新鲜Hello／wire Manifest；runtimeSigningMatched=false、formalAdmission=false、deployable=false。未新鲜核板／线上签发，不是BLE、附近或任务真机通过 |

独立只读复核最终BIN／ELF／MAP／prepared、9项stage差异与全部失败链SHA一致，容量及恢复合同保持；未重复编译或消费者审计。最终报告为`build/da4-ble-runtime-20261103/candidate-report.entry-order-correction.json`。本候选的构建队列已完成，下一批复用，不再重编已通过目标。

证据：Library `build/da4-ble-runtime-20261103/`的validation-private-FtPXhJ、candidate-private-AFjIiE及原失败／显式续编链；App `build/da4-direct-profile-20261103/private-5SYlrF/`保留真实红／绿。私有PoP与候选仅在ACL保护的忽略目录，原事实不覆盖，未升级SDK、安装依赖或提交推送。

### 12.4 原NVS／四槽保持及恢复方案

1. **现在不备份／读板**。真正刷写前才核当前COM11芯片／Flash、实际分区及NVS，并私有保护实际受影响NVS；历史20 KiB／revision18／count1不能当当前值。缺失或不兼容的instance／Key／根／TaskStore须停，不让候选补造新实例、scope或任务。
2. 保持`bl_v2_id/instance`、`bl_v2_access/root`（Key／Admin／presence）、`bl_v2_net`的wifi_sel／wifi_0／wifi_1及`bl_v2_tasks/snapshot`。核关键blob的hash、版本、epoch及任务状态／scope；不复制DeviceKey到Sketch、不搬云scope、不重放UNKNOWN。存在启用／到期任务时，须先评估维护窗口暂停影响，不清任务凑前置。
3. 将来仅写app，编译门限仍0x10000起、3,342,336 B；还须同时满足新鲜实读slot范围。历史实际factory为5 MiB不允许放宽编译门限或改表，不写bootloader、分区、SPIFFS，不擦NVS／清根。**不能直接使用默认IDE／arduino-cli upload或merged.bin**：原Core3.3.11上传recipe还会写bootloader、0x8000分区表及0xe000 boot_app0，与历史实板表不相同。后续须核准仅写原始app的既有入口，不让普通上传覆盖分区。
4. 指定恢复应用为已验f878镜像，**它不是当前未知主程序的新鲜备份**。恢复保留验收后最新NVS；正常任务／消费／网络变化一旦发生，不得用旧NVS快照覆盖。原NVS保护件用于只读协调与必要的明确恢复，不默认回滚后续状态。先收回App连接再处理固件恢复，未知停机保隔离。

### 12.5 当前App功能开关决定

`directAccess=false`是之前cloud-beta部署候选配置，不是硬件不支持的证据；用户已明确Direct／附近为产品需要，**本批已在当前使用的cloud-beta源配置开启**。它仅提供入口／执行链可用性，不跳过权限、当前账号active凭据、Manifest、presence验签或新鲜State，也不自动连接／控制。未顺手开放消息／推送等其它功能。

§12完成时已装`0cd3c16e…e461564`仍沿旧false配置；此装机差异已由§13单次匹配构建／保留数据安装覆盖。不得把“尚未真机签收”继续变成长期默认关闭，也不因启用开关就假称附近或全部Direct能力已真机通过。

### 12.6 同板ID和“附近”目前到底做到哪里

设备持久instance／根共享，不等于服务器logical ID自动合并；WiFiProv配网BLE与日常Direct BLE也不是同一入口。

| 方向 | 当前实际处理及欠证 |
| --- | --- |
| BLE-only／Direct先接入，再加Cloud | 既有enable-cloud可给同账号唯一原BLE logical ID补Key；App核原Admin、instance／epoch并让用户确认。**限匹配入口**，不是所有bearer自动升级；当前隔离SoftAP拒绝hasAccess=true／hasKey=false的纯BLE部分根，不是其直接升级入口 |
| WiFiProv BLE与SoftAP互换／原设备重配 | 共用身份、allocation和CAS链；保留instance／根并走合法原上下文时恢复原ID，不因换bearer创建新身份。未知根、错误账号或已有Key无合法上下文时拒绝，不任意新建／合并 |
| manual Cloud先接入，再首次独立BLE enrollment | DA-1新源码在已有已证明Cloud session关联时409防重复，**不是自动复用diy ID**。本会话尚未补隔离PG／部署该Gateway，不能宣称当前云端防重已生效；challenge-only和跨并发全局唯一不在该保证内 |
| 已有active BLE／旧pending／历史双ID | 原BLE恢复及已签UNKNOWN只读协调保持；不把已签请求改为fresh重发。历史双ID没有自动合并／删行／迁TaskStore或scope，resolve一个ID也不证明库存无旧重复 |

附近方面，DA-2已修后台旧状态、HMAC完成时刻错误续TTL及本地凭据变化后的旧观察复活，有限116＋1及匹配App入包证据保持。**§13已实际启用功能，但不能答“附近已完整真机验收”**：正向还需真实Direct-v3广播、当前账号active凭据和匹配presence key。Development本地凭据0项不足以验授权附近；无presence可回退Direct-v2连接，但不能因此按名称／MAC伪造附近。

§11的一次WiFiProv配网发现0命中既不能代签，也不能否定Direct附近。本批不为此清根、补绑或加循环扫描；后续功能必须开启，在自然合法凭据／广播条件下补一次正向附近，而不是用关闭开关代替修复或验收。

### 12.7 下一批最小范围

1. 先仅构建一次当前cloud-beta并同签名保留数据安装，使用户要求的Direct／附近功能真正对现场可用。沿原SDK／脚本，安装前只保护实际受影响账号、授权及完整请求日志；不清数据、登录、发码或顺手改其它开关。核包内和实际运行主入口的配置，不以源码true冒充手机已启用。
2. BLE四槽候选已经完整链接，不另开重复编译。COM11继续保持；本批“暂不刷板”未解除。后续明确允许切换时才做§12.4的新鲜兼容／NVS检查，仅写app；旧根／TaskStore不符合时停止，不造新身份。
3. 无线差异只补一次WiFiProv扫描→停止／选择→取消，允许原目标WiFi只读扫描，不写身份／WiFi或重复完整入网。Direct正向附近另需自然有效presence凭据与真实广播，不拿配网发现代签、不以缺Admin阻塞纯扫描。DA-1隔离PG与匹配Gateway发布仍单列，不自动并行展开全矩阵。

## 13. DA-4：Direct启用的匹配App安装与运行核验（2026-11-03）

状态：**MATCHED_DIRECT_ENABLED_APP_INSTALLED_AND_RUNTIME_VERIFIED**。本批只接§12.7第1项，不刷COM11、不主动扫描／配网、身份安装或设备控制；不部署服务器、发码、登录或升级SDK，不提交推送。首次安装后CDP停点与后续显式只读协调分别保留，不能将整条队列写成无停点首轮全绿。

### 13.1 冻结及保全

相对§10的2210项App冻结输入，当前仅cloud-beta配置及对应spec两项变化，原WiFiProv输入未变；§12配置green5项的冻结与当前逐项匹配，不重复回归。继续原prebuild→production,cloud-beta Web→Capacitor sync→offline单worker assembleBeta，不下载、clean或另开重复队列。

安装前已确认原位APK仍`0cd3c16e…e461564`，同Development账号、7项SecureStorage和两份完整IndexedDB store已保全，完整阶段logcat私有保存。全凭据前缀当前0项是新鲜本地存储事实，不推断板上根已丢或授权附近已通过，不为补凭据重认领。

本批只禁止harness主动测试扫描／连接，不关闭系统蓝牙，也不拦截正常App生命周期或Direct启用后的自然presence需求；不能把“没有主动扫描调用”写成“零射频活动”。运行配置需同时核不可变APK、实际主入口及实际已加载profile模块，避免shared chunk与main混淆；不fetch、reload或注入产品逻辑。

### 13.2 单队列及实际匹配制品

原prebuild／Web／sync／APK分别32.883／39.121／11.287／180.273秒、各exit0；每阶段全App、外部WiFiProv实际源码／消费者、ignored构建配置、HEAD及索引保持。沿原offline单worker和SDK，不清缓存、不下载、不另开队列；复用§12通过的5项配置回归，不再跑DA-1～3旧矩阵。App本批没有额外产品源码修改。

| 项目 | 结果 |
| --- | --- |
| APK | `iot.diandeng.tech.beta`，77,894,008 B；SHA256 `a9e3e4eb10ee16e5be2532c6c21f3e998522ddf4c5443faad068eb5bc7cac7c1` |
| 原依赖／签名 | WiFiProv2.4.4、esbuild0.28.2、原证书`a76639ec…5d5d210`保持；HTTPS／禁止明文、原注册表及1,158项APK／同步／Web资源逐字核对通过，诊断关闭 |
| Direct配置 | 真实main静态import图内唯一profile，严格AST及原冻结spread helper解析，不执行bundle；directAccess、automation、tasks=true，其它开关仍false，原host／LAN／bootstrap配置保持 |
| 实际主入口 | `main-GENRCY5Y.js`，798,738 B，SHA256 `4a7ed1598dd11d2a7ebf9274587968969ff0e2babbc5b3d769bbab7ad6fe4ef5` |
| 实际配置模块 | `chunk-BYAGOBAl.js`，SHA256 `5836f149d858119d2fb714d3b4e713f2107794020647d9f2e0ec248110fd440d`；配置不在main本体，不能混写两种证明 |
| Direct原生部分 | APK的BluetoothLe注册及BlinkerBleScanPlugin／AndroidBleScanner DEX核对通过；实际WebView只读isPluginAvailable确认BluetoothLe与BlinkerBleScan均true，没有initialize／扫描／连接调用 |

### 13.3 安装、只读协调与保留

原位旧APK／账号与完整日志再次核对后，**仅一次`adb install -r`，返回Success**，原位新APK SHA匹配。正常am-start之后首次CDP fetch failed，尚未读运行模块；该已知新进程调试入口未就绪的harness停点原样保留，不据此判产品失败，也未重装或重建。

随后一次显式`reconcile`只读协调成功：当前顶层默认上下文既有VM主入口及profile模块分别getScriptSource，原始UTF-8 SHA均与不可变APK精确相同。不是靠DOM脚本名／本机www或源码true代签运行；没有fetch、导航、reload或注入产品代码。原配网页仅入包，未执行，不签DA-3选择连接或附近广播。

安装前／前置复核／协调／收尾同账号及Development区域保持；7项SecureStorage逐字一致，两份完整IndexedDB store保留，原0行、结束0行，无新增自然行，不制造请求样本。全部阶段logcat及首停点私有保留，不清日志，不恢复旧token或journal。临时常亮已恢复，Debugger／Runtime／socket／自有forward均释放，最终forward列表为空；App正常保留新版本，COM11、NVS、授权、TaskStore、scope、CAS和UNKNOWN未操作。

证据在App忽略目录`build/da4-direct-enabled-app-20261103/`：candidate-VpMtEc保留唯一队列及静态核查，phone-private-VN4nG1保留ACL私有保全、首停点、运行及收尾；`installation-summary.json`汇总实际安装结果。build-only报告仍installed=false，不改旧报告来代替真实安装证明。

### 13.4 后续只补未完成的设备差异

Direct／附近现在已在匹配App实际启用，**不得再因是内测版将其关闭**。本批没有自然presence凭据／正向广播样本，不假称附近全部解决；不为补样本清根、补绑或循环扫描。

§12 BLE四槽候选原SDK完整链接已经通过，不再重编。COM11仍未刷写；下一批先核当前固件／实际分区及NVS、instance／根／TaskStore兼容，候选切换必须保持原身份与最新存储且仅写原始app，不用默认upload覆盖分区。原“暂不刷板”未解除，必要切换按明确的新批范围执行。条件齐备后只补一次扫描→停止／选择→取消及原目标WiFi只读扫描，不安装身份／下发WiFi、重跑入网／任务或自动化。DA-1隔离PG／Gateway发布和授权附近正向仍分别保留，不并发扩大范围。

## 14. DA-4：COM11原程序、实际存储及维护停驻的只读核对（2026-11-03）

状态：**READONLY_STORAGE_STRUCTURE_AND_PARKING_CHECK_PASSED_NOT_FLASHED**。本批只补§13.4前置，不编译、安装App或刷固件、不扫描／连接、下发WiFi或任务，不部署服务、发码或改账号。读取工具无Flash写入；ROM读取会复位并短暂中断通信，不称为无侵入观测。

### 14.1 唯一受控读取及精确范围

USB枚举匹配COM11原serial及VID303A／PID1001后，复用既有esptool5.3.1的`--no-stub`只读入口。首次default-reset进ROM，中间no-reset，最后一次hard-reset请求恢复原程序；9项ROM操作全部返回0，没有失败重试。只允许read-mac／get-security-info／flash-id／read-flash／verify-flash，不发串口A、1200触摸、WiFi或任务命令。

| 新鲜证据 | 实际结果 |
| --- | --- |
| 芯片／安全 | ESP32-S3、8 MiB；Secure Boot及Flash Encryption disabled |
| 实际分区表 | 仅0x8000／4 KiB读验；表MD5、范围及无重叠检查通过，SHA256 `5f4513918ed1cb4ccf0ecc7928254baa4357bb8ddac616cc6e5043dc68d37e7a` |
| 实际布局 | NVS 0x9000／20 KiB；唯一factory 0x10000／5 MiB；SPIFFS 0x510000／2 MiB，未用默认CSV冒充实表 |
| 原NVS | 只读并ROM verify完整20 KiB，SHA256 `aad6111234551e131a1728cc8647a2ea476e310e86188fb8fdbd7e5c26f4179e`；仅ACL私有保护实际受影响范围，不做8 MiB整片备份 |
| 当前应用 | ROM verify与已验f878的1,437,360 B精确匹配，SHA256 `f8783fbe6c66144af2acfb4184e94523483fda007ff8de5ee12cef6ba835eaee`；不是新鲜读出一份当前主程序备份，不匹配时也不会扩大读取 |
| 已编BLE候选 | 原始应用5f9a…734990／1,413,296 B，同时满足原编译门限3,342,336 B及实际factory 5,242,880 B，起点均0x10000；没有放宽编译门限或改表 |

读取后一次原程序复位请求成功；**没有据此宣称业务已恢复在线、Admin握手或任务运行已通过**。读取期间MCU保持ROM，原应用没有并行修改此次NVS；恢复运行后可能发生合法新状态，因此此保护件也不能成为以后回滚最新NVS的理由。

### 14.2 离线结构与任务停驻风险

在上述已读验保护件上仅运行一次离线检查，复用原NVS页／item／payload CRC、blob重组、实例／AccessStore内CRC及TaskSnapshot元数据解析；仅输出哈希、结构／角色布尔及非payload任务状态，不导出Key、控制器秘密、PoP、SSID或任务目标值，临时Buffer清零。新增适配器只在忽略目录，不改产品执行链或解析器。

- instance应用记录schema1／内CRC有效；AccessStore schema3、260 B、epoch1，DeviceKey存在、1个结构有效Admin、无Shared。**这不是当前App持有Admin或真实认证已完成的证明**。
- 网络selector与active profile存在且generation相符、confirmed；未输出网络名／密码，不据此宣称当前路由可达或云在线。
- TaskStore为BoundSnapshot／schema2／count1／tableRevision18，绑定instance与设备相符，scope及rootDigest仅记录哈希，不换scope。唯一id1为10秒Countdown、disabled／Completed／startSequence1，没有enabled、CountdownArmed或Claimed记录，结构性维护停驻检查通过；不重放已完成任务。
- **presence key未安装（version0）**。这与§13当前App本地BLE凭据0项是两个独立缺口；不能仅靠App开关或WiFiProv发现得到授权附近。Direct-v2回退连接与v3附近匹配分开，不按名称／MAC伪造附近、不清根来凑样本。

检查明确`completeTaskCodecSemanticsVerified=false`、`actualAdminOrServerScopeVerified=false`、`runtimeSigningMatched=false`、`formalAdmission=false`。本批只签存储结构及停驻风险，不签完整Codec语义、真实授权、当前云scope／签发、wire Manifest或运行准入；未知／腐坏／不安全状态不能推断Empty或自动修复。

证据在Library忽略目录`build/da4-com11-readonly-20261103/`：board-private-r3oc2L保存20 KiB原件、唯一ROM日志／返回、离线脱敏投影及复位请求；`inspection-summary.json`汇总新鲜见证。独立只读核对工具适配、9份code0及原制品／容量门禁，没有再次操作串口或复制秘密。没有操作App账号／请求日志、原授权、TaskStore、CAS或UNKNOWN，不回滚旧NVS，不提交推送，无关本地修改保持。

### 14.3 下一批的最小准入与恢复合同

原“暂不刷板”边界本批未解除。**允许COM11有限候选切换后**，才复用已编译5f9a应用、匹配App与原非空Security1 PoP，不再编译／重装／登录；仅写0x10000原始app，不用默认IDE／CLI upload或merged.bin，不写NVS、bootloader、分区、SPIFFS。

真正写入前仍需核原程序／分区及最新NVS：若此次20 KiB与最新值不同，保护并协调合法变化，不拿旧备份覆盖；启用／armed／Claimed任务或根变化则停。候选10分钟维护窗口沿原guarded owner及taskReady=false，手机取消不表示板端窗口已停；只做一次扫描→排空停止→选择／目标WiFi只读扫描→取消，不安装身份、写WiFi或启任务，不重跑完整入网。结束仅恢复f878应用并核最新NVS保持，不恢复旧任务消费／网络快照。

授权附近正向另按合法当前账号Admin／presence同步入口安排，不默认新建身份、迁TaskStore或补造凭据；WiFiProv选择差异不因此阻塞。DA-1隔离PG／Gateway发布仍单列，本批不扩大测试矩阵。

## 15. DA-4：COM11应用切换与一次BLE选择取消短验（2026-11-03）

状态：**ONE_BLE_SCAN_SELECT_CANCEL_PASSED_CANDIDATE_RETAINED**。用户明确允许必要刷板测试，按“验收后不恢复原应用”执行；本节覆盖§14.3的暂不刷板及原程序恢复约定，不扩大到重绑、清NVS或WiFi／任务写入。

本批固定沿用§12已编译5f9a原始应用及§13的a9e3内测App、原SDK和匹配非空Security1 PoP，不重复编译／安装或发码。手机冷预检及全部入口备齐后才写应用、启动600,000 ms维护窗口；按正常原页面一次扫描→唯一目标选择→原连接自动一次目标WiFi只读扫描→返回取消，不点发送／刷新，不人为延迟start Promise或为竞态循环采样。

刷写唯一地址0x10000；写前新鲜读验4 KiB分区及20 KiB NVS并核原f878，变化则保护新字节并停、不回滚；写后应用／NVS／分区全verify通过才启动。写入结果未知时留ROM、不重刷／自动恢复或启动。成功保留5f9a及最新原授权／任务，不恢复f878或旧NVS；本候选每次启动请求10分钟窗口、到期仍原Fault／guarded停机，手机取消不代表板端恢复Cloud或长期正常运行。

账号／授权／完整journal及阶段logcat保持；仅使用可关闭、有界且不记录PoP／原始参数的观察，拒绝身份、custom、WiFi写，不放宽原owner及后台门禁。本批不是Direct附近或完整入网／任务／自动化验收，保留CAS／UNKNOWN，不升级SDK、不提交推送。

### 15.1 零写入停点与最新校准保留

手机原入口／权限／Security1／PoP冷预检与7secure／两份完整journal指纹齐备后才进ROM。首轮最新NVS raw从aad611…变为54a45d…，门禁在**writeAttempted=false**时停并请求原程序启动，原停点／新20 KiB完整保留，没有刷错或回滚。

离线比较全部35项逻辑记录，唯一变化为`phy/cal_data`；所有身份、接入根、网络及TaskStore记录保持，停车检查仍通过。原SDK的esp_phy_init.h明确校准数据可保存到NVS；本批仅把这一精确键的合法演进与受保护业务变化分开，不放宽整个phy namespace或任意未知键。

显式续批再次新鲜读验分区及NVS，完整key集合／类型／长度与全部非cal_data值hash必须相同，且最新Access／Task blob与原值一致、无启用／armed／Claimed任务，才执行**唯一一次write-flash 0x10000原始app**。应用、该次最新NVS及分区三项verify全部通过才启动，11项工具返回0；无默认upload／merged.bin或NVS写入。启动前NVS SHA为`54a45dd8e6ce4f3ae233414fc67e4bd8ad917939355930e830a124f69063089e`，应用SHA为§12的5f9a…734990；原表5f451391…d37e7a不变。首批／续批分开保存，不将首停点抹成首轮全绿。

### 15.2 唯一真实原页面短验

复用现装a9e3及原WiFiProv2.4.4。PoP只在内存读取原已冻结私有头、填原masked输入框，不打印或持久化其值。原页面一次点击扫描，唯一匹配fixture结果到达即点击该行，未故意延迟start桥接或扫描12秒后再重扫；native start与stop均resolve后才select。

| 实际路径 | 单次结果 |
| --- | --- |
| BLE发现／选择 | startBleScan1、selectDevice1、结果事件1；按原PoP／Security1路径，不靠Admin或手工调用页面私有方法 |
| 安全配网连接 | connect1成功，原connectionState与页面均显示BLE／Security1安全会话；不是Admin／Direct控制握手 |
| 目标WiFi只读列表 | 原connectSession自动scanWifiNetworks1成功，返回14项且原UI显示14行；不再点击刷新、SSID、remember或发送配置 |
| 返回取消 | 仅原header返回，停止／clearState回执已完成，观察到disconnected，pending0；辅助getDeviceInfo明确No ESP device is selected，不单靠该快照判物理关闭 |
| 观察回收 | 原生命周期走已确认关闭后的lease释放路径；本批3个观察listener及nativePromise／fetch／XHR包装均还原，不通过第二次扫描证明释放、不以计时器或ready解除隔离 |

所有时延来自同一App performance.now的桥接观测，不是射频仪器、设备CPU或跨进程测量：start回执24.7 ms；扫描调用→select调用240.2 ms；select16.0 ms；connect1,193.3 ms；目标WiFi扫描6,326.8 ms；扫描调用→WiFi列表回执共7,777.8 ms；取消原生stop／clear窗口20.9 ms。连接后约6.3秒主要在**目标WiFi扫描**，不能混写为控制／任务下发慢，也不为做性能分布重复采样。

罕见“结果已到而start Promise仍pending”没有人为构造或单独取证，本批仅签常规路径及实际start／stop→select顺序，不将其外推全owner或历史全部GATT故障。身份／custom／WiFi／任务写调用0、写HTTP0、blocked0；没有创建新logical ID、scope或重放UNKNOWN。

### 15.3 保留、观察歧义与收尾

原账号／Development、7项安全存储、两份完整journal及已存WiFi信息指纹前后保持，原journal0行、结束0行；完整阶段logcat私有保留、不清日志或恢复token。App留在`/home/tools`，取消后原选择已清，所有观察／CDP／forward已回收，临时常亮恢复。

原取消记录`maskedPopCleared=false`保留：旧观察表达式把“输入框不存在”也返回false。随后仅只读DOM说明记录为`/home/tools`且配网页host0；因此只签字段／页面卸载，不将原bool改true，也不据此诊断PoP持留或重跑UI。原页面及恢复意图未被补造。两个辅助命令分别因本机路径与shell引号错误未进入其目标检查；随后用正确路径／文件脚本完成核对，没有增加无线动作，不计入产品失败、无线重试或第二次配置。

证据：Library忽略目录`build/da4-ble-select-live-20261103/`的board-private-vFh0rG保留首轮零写停点，board-continuation-k2bEZl保留显式协调／唯一写入／三校验；App同名目录的phone-private-recA4C保存原UI观察、截图、完整logcat及收尾，`trial-summary.json`汇总真实结果。独立只读终核部署及桥接SHA链一致，没有新串口／原始秘密读取。

按用户要求**保留BLE维护候选及最新NVS，不恢复f878**。NVS精确保持只确认到写后启动前；没有为结束核对再进ROM、复位或读取射频后的NVS，不能冒称后者已二次验证。手机取消不关闭板端10分钟窗口，到期仍沿原Fault／guarded停机合同，本批未等10分钟重测，也不签普通常驻Cloud／Direct运行或正式任务准入。

### 15.4 本范围收口与下一最小范围

DA-4常规选择取消差异有限收口，不再默认重刷、重扫、恢复原程序或跑完整配网／任务／自动化。Direct开关继续保持开启；真正授权附近仍缺合法当前账号凭据／板端presence材料，不清根或补建ID造样本。

下一优先补DA-1已有“Cloud先接入→首次BLE”防重复源码的隔离数据库有限回归及匹配部署前差异核对，先确定安全的独立测试库方式，不连接业务库写样本、不自动发布Gateway或合并历史ID。附近的合法owner／presence同步另列，不与数据库或整套配网同时开新队列。

## 当前主线指针（2026-11-03）

当前进度统一见[独立注册执行方案§14](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#14-ip-4-新身份-cloud-首装及保根注册差异2026-11-03)，以上历史结果不改。用户允许COM11覆盖、无需旧恢复；测试Auth本账号设备额度10留存。新Cloud首装、两卡独立ID／2→3→2计量、显式恢复、附近不串旧卡及仅删BLE保Cloud／根已有限通过。

f878任务owner内同步Presence写自拒已修正，新BIN e48cffa…28f07及授权保留。最新[§15源码审阅与静态对齐](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#15-必要源码审阅选择器与发布冻结对齐2026-11-03)已关闭三项freeze／选择器差异，架构及App有限95项通过；下一仅在明确交付时冻结匹配WiFiProv旁仓及同提交CI。首次提交前未分类错误、BLE→Cloud及删源writer边界保持，不重采已过矩阵、不自动推送。
