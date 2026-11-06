# Device V2 接入稳定性与普通首装收口执行计划

日期：2026-11-04。目标：已登录用户可正常添加支持的设备，App 如实区分附近、实际连接和 Cloud 可用，失败时能定位阶段而不重发写入。沿用现有 WiFiProv、设备注册、凭据库和单扫描 owner；不按 MAC 注册、合并或授予权限。

**当前入口（2026-11-06）**：转至[有限支持交付与核心稳定方案§18](DEVICE_V2_CORE_DELIVERY_AND_STABILITY_EXECUTION_PLAN.md#18-ds-5一次匹配app交付及两页只读差异2026-11-06)。DS-5两处修正已一次构建／原位安装及必要真机差异收口，手机现为`ae5d7736…176c2f`；分享每次入页一轮，H2草稿查询保留／离页清除，无策略写入。账号、授权、TaskStore、UNKNOWN及完整日志保持；未改服务或刷板，不重跑接入／附近矩阵、不续接§26深诊断。下一批仅DS-6必要交付清单；原自然结果待样本、SoftAP超时未修复及发行HOLD继续保留，暂未提交推送。

**历史进度（2026-11-05）**：§26已完成有界脱敏诊断、64项host回归及COM3一次短验，未发送WiFi；原App、固件及服务已恢复、诊断关闭。Android callback／actor／guard仍欠证，同板WiFiProv正常联网／配网窗口两态仍待验，不因改顺序写成已通过。

## 1 已有证据与不重复范围

- 普通 WiFiProv BLE 和手动 SoftAP 空根首装、OEM 自动 SoftAP 保根重配已有匹配 Android／ESP32 真机证据。历史成功不代替所有 OEM 或正式发行；详见[入网完成方案](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md)。
- [两卡与附近](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#146-匹配固件及两卡有限真机结果)：同板 Cloud／BLE 分卡、分额度及附近不串卡已经实测。WiFiProv BLE／SoftAP 是同一 Cloud 产品的不同传输，不再为传输新增卡。附近不解锁控制、不修改 Cloud 状态。
- [受限换网 §7.5](DEVICE_V2_WIFIPROV_REKEY_EXECUTION_PLAN.md#75-全新意图单次-com11-ble-换网2026-11-04)：保留同卡、轮换 Key、一次 Apply、新会话与新鲜 State 已验；关窗后一个板端样本不证明持续在线。SoftAP 换 Key、BLE→Cloud 的 Wi-Fi-only 恢复、公司版签收仍各自独立，不借本批默认放行。

## 2 执行顺序及退出标准

| 顺序 | 最小实施 | 退出标准 |
| --- | --- | --- |
| ARF-1 Cloud 波动只读分析 | 关联 §7.5 既有手机回执、网关关窗、Broker 会话／State 与板端单点；分开时钟，区分 Key 提升主动撤销旧版本与新版本自然掉线。 | 能定位停点才给最小修正；无法判定则列缺失事件，另安排一次有上限只读观测。不为补样本重配、改心跳／重试、跑实时续租 Error 8 或重部署。 |
| ARF-2 普通首次绑定去重复登录 | 先限定 Android／ESP32 WiFiProv **空根新建**，有效账号完成一次性安装专用交付；复用现有临时公钥、Vault、创建幂等、Security1／PoP、Admin、CAS 和安装回执。普通手填 Key、已有根重配、BC‑5 不动。 | 有效但没有近期验证的会话完成一次普通首装；没有通用明文读取、重发或多扣额度；未知结果同 owner 只读协调。分内核、原入口接线和真机三个证据层，不以离线通过代签上线。 |
| ARF-3 附近与 owner 交接 | 只用自然具备 Direct-v3／有效 presence 的卡，验一次首页命中 → 后台清除 → 返回按需发现；条件具备再看原首页与 WiFiProv 扫描 owner 交接。 | 实际只归匹配授权卡；无权限／缺材料／未发现不等同设备离线；不重绑、不清根、不改 TTL／节拍。特定 GATT 失败仅自然复现时保留原生阶段。 |
| 后置 | SoftAP 连接前设备热点发现、更多 OEM 及公司版验证；新安全换 Key 默认准入单列。 | 不把连接前热点发现与连接后目标 Wi‑Fi 扫描混为一谈；不因欠证关闭内测 Direct。 |

## 3 ARF-2 第一批：先接可验证的离线薄层

1. 新建专用请求只有原身份和额度允许时才分配新的 Cloud 记录；用独立 contract／domain、请求 ID、实例、逻辑 ID、版本、locator 和双方临时公钥绑定安装密文。当前 GetInfo 实例是 App 从其真实配网 owner 观察的值，服务端不把它当硬件签名或对已有根的授权；已知关联用于拒绝，不授予任何旧卡的 Key。
2. 只在原 `createDevice` 事务中对**新生成**的 v1 Key 做一次封装；结果等数据库成功及原 Broker 准备通过后才返回。重复只认原 metadata，不重新封装、解密或交付；`Idempotency-Key` 冻结合同、名字、实例及接收公钥。通用创建／reveal／rotate 的权限与响应完全保持。
3. App 的全新意图复用现有账号范围下的分配 journal 与 CAS，只有收到精确、当前的密文才在内存解封并核 locator；不给 App UI 显示明文，不在 journal 存 Key。密文接收方是 App，不宣称 Gateway 到设备的端到端密文或 Key 不进入 App 内存；设备仍用原 Security1／非空 PoP 下的精确 Bootstrap。后续接原页面时必须先识别这类意图，再分流旧 `resume/inspectExisting`，不能在失败、重启、旧记录或版本不匹配时转入明文 reveal 或旧 Bootstrap 重发。
4. 本批先接服务内核与 App 可测试薄层，**不开放 HTTP／不启用默认页面**。落地前还要把“数据库已提交但 Broker 准备失败／返回丢失”和“设备安装 ACK 未确认”分开：保留原意图、可见设备卡和原额度；只读查同一请求的权威结果，禁止第二次发放、自动删除或再次 CREATE。没有可安全执行的继续路径时给用户明确提示。只在此合同实现并通过反例后再接普通首装页面及真机，不先换协议或升级 SDK。

## 4 本批执行结果与下一步

### 4.1 ARF-1：只读核对完成，瞬时 Cloud 波动待定因

- 服务器同一时轴（北京时间）为 `20:29:31.057` 交换 Key、`20:29:44.451` 签发新 v2 会话、`20:30:25.882` 收到匹配的新鲜 State、`20:31:34.470` 终核该会话仍有效。Broker 通知按 `< v2` 撤销旧凭据与连接，旧会话 12 条撤销，未直接撤销 v2。Broker 版本推进时暂置的在线标志与板端状态不是同一观测，不能用它解释板端 `j`。
- 关窗文件写入时间不是容器切换时刻；现存本机材料没有板端 `j` 可对齐时间、v2 MQTT 断开／恢复边沿或续期请求结果。只确认板端有过 `Wi-Fi ready / Cloud Starting` 单点，不能归因关窗、续期或历史 S3 自然掉线，也不能宣称持续在线。无新样本、未连接服务器、未改重连。若下一次自然出现，只安排一段同卡只读窗口，记录重启原因、Wi-Fi/IP、HTTPS 授权／续期、MQTT 连断及匹配 State；缺证就保持待定位。

### 4.2 ARF-2：离线薄层完成，未接产品入口

- Gateway `createWiFiProvInstallation` 复用原设备创建、额度、Vault 与 Broker 准备，使用现有 X25519／HKDF／AES-GCM 封装。固定新建 v1、实例和临时公钥；同意图重复只给公开结果，不再封装或写 Broker。已关联设备、换租户、幂等冲突、无效接收公钥、Broker 晚失败或凭据变化均不交付。普通创建及明文 Key 路由未接此方法。
- App `createFreshSealed` 复用已有密文解封端口及 fresh-only `prepareNew/bind`。只绑定公开上下文，禁止能力缺失／已有记录、晚到回包、换账号或写 journal 不明时回退旧流程。未接页面、未变更原生插件和固件。
- Gateway 定向及相邻 **33／33**（6 项本批新增），App 定向及相邻 **36／36**，App 定向类型检查通过；包括独立加封／解封和拒绝反例，使用测试端口，不是实际 Broker／数据库或真机。额外原明文鉴权 HTTP 测试在源码入口因 `interface-workflow` 没有对应编译产物而未执行到业务断言，保留为匹配构建验证，不把它记为通过，也不为此改原鉴权。
- **下一批只补同 owner／原请求的无密文只读协调，以及原页面在旧 Admin 恢复前隔离新意图。** 先验证写前 checkpoint 的原生持久边界、重启后的读回和已创建但响应丢失的卡／额度保留。只有有效会话、真实空根 setup owner 与新意图同时成立才接一次新建；不把旧 `create/reveal` 用作新流程失败后的回退。通过后再建匹配候选做一次无需重登的真实首装。本批不部署、不装 App、不刷板或提交推送。

### 4.3 ARF-3：等自然匹配条件，不借首装或换 Key 代签

沿用已开启的 Direct 和现有测试。普通 Bootstrap 装入 Admin／Key 不等于有 presence，不能用 COM11 换网成功推定可显示附近，也不为补样本重配。先只读找一台当前账号具备匹配材料的设备；满足时做 §2 的一次差异，未满足则明确待验。

## 5 ARF-2：回包丢失只读协调与原页面接线

1. 只读协调不调用原 `create`，也不读取 Vault。按当前有效会话解析同 owner／tenant，核对原请求 ID、物理实例和冻结请求摘要（包含接收公钥与原名称摘要）。仅返回 `absent`／`created` 的公开卡上下文／`unavailable`；`absent` 不证明没有仍在执行的请求，不能解除写入围栏。密封 POST 只返回一次 `201`，不按 Auth 失败重放，也不转通用明文读取。
2. App 在创建和 Bootstrap 各自的实际发送之前，持久保存同一 scope 的非秘密阶段；Android 用明确返回成功的同步提交／CAS，不能把 SecureStorage 的异步 `apply()` 或同进程读回等同进程重启后持久。只读列表及核对复用原配网页恢复区。即使当前候选开关关闭，识别到该新意图也不能落到旧 `resume`；不自动发请求、不清旧记录。
3. 本批实际产品接线只准 Development Android、WiFiProv BLE、Security1／非空 PoP、实际空根、当前原 owner，复用原 Bootstrap、配网提交、官方终态和 Cloud State 判定。SoftAP、已有根重配、手工 Key 管理、BLE→Cloud、任务均不改变。原启用开关和 Gateway 准入分开；验收需同一已登录账号且无近期认证证明、可初始化的独立实例、匹配候选。现场已有根的板不默认清除；真机验收不重复配网。

### 5.1 软件及真实装机前置

- Gateway 在原创建入口仅识别显式 `provisioning-install-v1`，返回一次 `201/no-store` 密文；精确 `GET /api/v2/devices:install-status` 只读现存账号／租户及原冻结请求。已创建只返回逻辑 ID、版本和 locator，不解密、不准备 Broker、不给设备安装／联网结论。当前仅允许显式开关加逐 owner，且只限 Development；常规 CREATE 和明文 Key 鉴权保持。源回归 **15/15**、Gateway TypeScript 与匹配编译消费者测试 **9/9**。
- Android `BlinkerWiFiProvAllocation` 在匹配 HTTPS 内测包可读，只有显式候选可以写。记录七个单向阶段，原生 `commit()` 确认后才能越过发送围栏；记录不含 Key、密文、PoP 或 Wi-Fi。App 内核与相邻 **83/83**、Native **4/4**、原首装页面 **24/24**、Profile **11/11**；原页面的新用例覆盖丢失回包、关闭候选后阻断旧分支、手动只读查询一次。既有已确认 Wi-Fi 回执不因本地终态写失败而撤回。
- 一次隔离构建和原位安装成功，原签名、HTTPS、WiFiProv 2.4.4，APK SHA-256 `ee67531c05588c343f6af3652c8977f08fbf97a04709844427f146dcb3dbe873`；不带 BC-5／旧换 Key 选择器。真实 App 原账号和 bearer 未变，原 6 张可见卡不变；129 只读确认该 bearer 没有近期明文 Key 认证证明。手机从原添加入口到 BLE 配网页，原生持久记录可只读、当前 0 条，没有重新登录或发验证码。
- 用户授权选择一块测试板，只选 COM11。已新鲜核原 v2 应用及实际 factory 分区，完整 20 KiB NVS 双读一致，备齐将被覆盖的精确应用扇区。沿用此前已在同一实际分区上跑过的 `5f9a…4990` 空根候选；仅应用区和 20 KiB NVS 写入并独立校验，分区未动，COM3 未操作。先保持 ROM，待测试 Gateway 确认健康及限定入口后只开一个原配网窗口。

### 5.2 真机短验通过与收尾（2026-11-04）

- 129 仅临时启用 Development 的一个 owner；原 schema 064、原 Broker 及其它容器不动。只读确认手机的同一有效会话没有近期 Key 认证证明。真实 App 从「添加设备」进入原 BLE 配网页，COM11 一个窗口，实际 Security1 会话由板端扫到「带翅膀的蜗牛」。
- 只点击一次：GetInfo 24 B 空根、一次专用创建 201、原 Admin 201、Bootstrap 精确 ACK `[2,3,0]`、WiFiProv `config-sent`／`config-applied`／`success` 各一次，页面确认上线；没有 `device-key:reveal`、退出、登录、验证码或自动重发。七个原生 `commit()` 阶段依次完成，从点击到 Wi-Fi 写前围栏约 1.85 s，至 Wi-Fi 确认另约 7.72 s（同一手机时钟，仅此单样本）。
- 同一账号可见卡 **6→7**、新卡仅一张、v1；真实同请求 GET 200／`no-store` 返回 `created` 及精确公开上下文，无 Key／密文。现网不人为丢包或重发；丢回包和返回后的不重发由软件反例覆盖。之后冷启动 App，仅原生读回同一 `wifi-confirmed` 非秘密记录，实际会话摘要不变，未恢复执行任何写入。
- Gateway／Broker 独立只读核实同原请求冻结摘要、一个独立 Cloud v1 卡、同实例的一个已认证新会话及匹配的新鲜 State；原 v2 卡、Key、Admin 均保持。不跨手机／服务器原始时钟推导提交顺序。随后关闭试点，恢复原 Gateway／Nginx，专用状态入口直连与 HTTPS 均 404，完整日志保留；新测试卡保留作证，未撤权或删除。
- COM11 精确恢复受影响原应用扇区 **1,417,216 B** 与完整 **20,480 B NVS**，原分区不变；一次普通启动后只读确认原网络和 Cloud 就绪、无控制写入。COM3 未操作，手机临时保持亮屏配置已恢复。未升级 SDK、修改生产环境或提交推送。

**范围决定：ARF-2 受限 Android／BLE 空根首装完成。** 尚不默认开放给所有用户，也不替代 SoftAP、已有根换 Key 或明文 Key 管理的各自合同。后续按 §4.3 补 ARF-3 的自然附近差异，不重跑这条成功链。

## 6 ARF-3：当前账号和首页已核对，缺自然广播正样本（2026-11-04）

- 沿用 §5.2 已装内测包和 Development 原会话；同 owner 的鉴权与设备列表只读核对成功。首页实际 7 张现役卡（5 Cloud、2 BLE Direct），其中 5 张有本地 active Direct 管理材料及同 access epoch 的 v1 `current` Presence（3 Cloud、2 BLE）。这只证明手机存有候选材料，不证明设备持久安装或正在广播。原 App 流程可先缓存 bundle、再等待已认证 Direct 会话写入及确认，不能把本地 `current` 当成设备 ACK。
- 现场只识别 COM3、COM11。COM3 原 BLE 卡的 Presence 有历史持久安装证据，也与当前 App 的同一物理实例相符；但已安装 Cloud Key 的现有 BC-5 应用不是普通 Direct 广播固件，原 BLE-only 首次交接入口不再成立。COM11 当前为 §5.2 恢复的原 v2 WiFiProv 实例，手机确有该卡的 v1 Presence 缓存，但同一运行身份没有可复用的板端已安装并广播证据；此前 IP4 正向广播属于换根前的另一个实例。
- 未手动启动扫描或发起 BLE 连接。手机蓝牙开启，原首页约 12.3 s 被动观察 13 个快照，没有任何卡命中附近；终点两张 BLE 卡均为「蓝牙未发现」，5 张 Cloud 卡继续显示自己的 Cloud 图标及状态（4 张「离线」、1 张「状态未知」）。这是观测结果，不证明 Cloud 持续离线，也不以全阴性证明不串卡或状态互不影响。没有前台命中，不切后台补空跑；清除、返回按需恢复和正向对照均保留待验。
- 后续只需一台当前账号已授权、原运行固件正在发匹配 Direct-v3 Presence 的设备。优先已有 Cloud＋Direct 卡，一次命中同时观察同卡 Cloud 主状态与附近次标记，再做后台清除及返回恢复，旁证其他卡不被标记。纯 BLE 只可验其附近生命周期及其他 Cloud 卡不受影响，不能代替同卡双状态证据。不为补样本重跑首装、重绑、清根或改节拍。本批未刷板、改 App／服务源码或设置、改账号授权及记录、提交或推送。

### 6.1 COM3 保根切换后：原 BLE 卡附近生命周期通过，同卡双状态待验（2026-11-04）

- 同一内测 App、Development 原账号及 7 张卡前置复核：COM3 原 BLE 卡的 active Admin、同 access epoch 的 Presence v1 与现有材料相符；原首页短观察没有找到正在广播的匹配 Cloud＋Direct 卡。按授权只处理 COM3：ROM 核对实机、8 MiB、分区表、原 BC-5 应用、实际 OTA_0 选择；在 NVS 核实相同实例、Admin、Presence 和原 Cloud Key 已存在。仅写已冻普通 `BleQuickStart`（`987f…12ac3`，699,536 B）到 `app0 @ 0x10000`，独立核对新应用、原分区和 OTA 选择。启动前与验收结束后的 **20,480 B NVS 均与写前完全一致**；临时原始 NVS 读件已移除。COM3 保留普通 BLE 应用，COM11 不动。
- 真实首页一次 HOME／返回，保持同一 App 进程：原 BLE 卡显示「蓝牙在附近」；原生 `pause` 时附近已消失，正在返回结果的同一轮原生扫描以 `BLE_SCAN_BACKGROUND` 结束，随后系统报告 `isActive=false`。返回同一首页，先为「蓝牙未发现／正在查找蓝牙」，收到新一轮原生扫描结果才恢复「蓝牙在附近」（距本次原生 `resume` 约 0.29 s，单次样本，不作性能承诺）。观察器只订阅与读页面，不调用任何扫描、连接或设备写入接口。
- 附近只出现于原 COM3 BLE 卡，其他 6 卡无误标；5 张 Cloud 卡的主通道图标和状态文字始终保持（4 张「离线」、1 张「状态未知」），因此只确认这些卡的 Cloud 展示不被本次附近发现和回收改写。**未覆盖同一 Cloud＋Direct 卡的双状态，也不据此判断在线 Cloud 会话稳定性。** App 同一会话和原 BLE 凭据前后摘要一致，设备卡不增不减。不重绑、不清根、不动扫描节拍，不改产品源码或服务配置，不提交推送。

### 6.2 COM11 同卡：Cloud 在线已核对，附近无自然正样本（2026-11-05）

- 原签名内测包 SHA 与 §5.2 一致。Development 原账号下，原 Cloud 卡及 Admin 的两个 `GET` 都返回 `200/no-store`。卡仍 active、原 Cloud Key 版本 **v2**，设备实例、原卡、手机 active Direct Admin 与 Gateway 的 access epoch／Admin 版本匹配；手机本地有 epoch 1 的 Presence v1。没有读取设备明文 Key、进入会自动同步 Presence 的设备详情页或发起写请求。
- 板端仅用原维护固件的 `j` 只读查询，实机匹配，`state=4 / error=0 / networkReady=1 / cloudReady=1 / powerWrites=0`。原恢复固件 `47d48…41e64c` 的 ELF／map 有 Direct 广播及 Presence 派生路径；普通运行若没有有效持久 Presence，可继续用 Direct-v2，并不影响 Cloud。§5.2 真正恢复的两份 20 KiB NVS 逐字节一致，实例、Admin、Key 及记录 CRC 核对通过，但其中 **Presence 不存在（版本 0）**。这是恢复基线，不冒充本轮新鲜 Flash 读回；没有之后的精确安装回执或新鲜匹配广播。
- 同一原卡在首页 13 个被动快照（约 12.3 s）始终显示「云端已同步」，未出现独立的「附近」标记。没有正向同卡样本，因此不切后台空跑；同卡附近清除、返回恢复、Cloud 主状态与附近边沿互不影响仍待验。COM3 不操作，未刷写、未重启 COM11、未重绑或清根、不改节拍、不提交推送。
- 继续只补一个前置：若不等现成样本，经明确授权后，仅对原卡、同实例和 epoch 走现有已认证 Direct Presence 安装及精确回执路径（仅此 Presence 持久记录允许改变；不换 Cloud Key、Admin、实例或逻辑卡），确认新广播和 Cloud 同时命中后再做一次 HOME／返回。不要用手机缓存、仅 Gateway 已分配、直接改 NVS 或纯 BLE 卡替代这项证明。

### 6.3 COM11 同一 Cloud 卡：匹配 Presence 安装及首页短验通过（2026-11-05）

- 按 §6.2 续办。匹配 owner、tenant、原逻辑卡、实例、Admin、epoch、Cloud Key v2。服务器已有与手机完全匹配的 Presence v1，`previous=null`，`deviceConfirmed=false`。只从原首页打开一次原设备控制页，复用既有 Direct-v2 候选发现、Method 2 及 Presence 安装合同；独立 `no-store` 读回同材料的 `deviceConfirmed=true`，版本仍 v1。没有重复进页、刷新 Cloud Key、配网、重绑、刷板或清根。
- 从控制页退出后，原 App 的根级 Direct owner 仍保有真实 GATT，首页显示「蓝牙已连接」，且未启动附近扫描；这是现有路由保链策略，不是确认回执或广播匹配失败。为单独验首页观察，正常重启 App 释放该会话，不清数据、不重登。**返回首页时有 Direct 会话而阻塞其它卡的扫描**是可复现的交接欠项；重启仅为本轮准备，不作为产品解决方案。
- 在同一进程完成一次正常首页→HOME→返回。原 COM11 Cloud 卡同时显示「云端在线」和独立「附近」；原生 `pause` 时只清掉附近，主状态仍「云端在线」，同一扫描以 `BLE_SCAN_BACKGROUND` 结束。随后在无附近时主状态自行变为「云端已同步」。返回前台后，新一轮的 Direct-v3 广播 HMAC 命中原卡，约 0.25 s 恢复「附近」，主状态仍「云端已同步」。原卡始终为 Cloud 主图标；其余 Cloud 卡状态未变，无附近误标；独立授权 BLE 卡的自然命中不作为本轮样本。单次时序不是性能承诺。
- App 会话和本地凭据摘要、7 张卡及原 owner 保持，末次只读核对同一 v2 Cloud Key 上下文、Admin、epoch 和已确认 Presence v1。唯一允许的设备持久变化是原卡 Presence 安装；未读取原始 NVS 作为本轮精确全区证明。COM3 不动，不改扫描节拍、不改产品源码或服务配置、不提交推送。

## 7 同一物理设备的双卡与模式切换：单列验收

纯 BLE 卡和 WiFiProv Cloud 卡仍是两个逻辑 ID，各自授权；MAC、广播名或相同实例本身不能归属另一张卡。日常 Direct-v3 广播证明物理根附近，**不声明当前运行的是哪种产品固件**。只有两卡各自具有有效、明确的同根绑定时，同一经过认证的广播才可以合法点亮两张卡。

| 物理设备运行状态 | 日常首页 | 设备接入入口 |
| --- | --- | --- |
| 普通纯 BLE，发匹配 Direct-v3 | BLE 卡显示「蓝牙在附近」。Cloud 卡仅在自身也有有效同根授权时显示独立附近；是否 Cloud 在线仍看真实 Cloud 状态。 | 不把日常 Direct 广播当成 WiFiProv 配网候选。 |
| WiFiProv 固件正常联网，发匹配 Direct-v3 | Cloud 在线／已同步可与附近并存；BLE 卡只有在明确共根授权且匹配时才显示附近。两张无共享授权的卡不互相借标记。 | 不进入配网流程。 |
| WiFiProv BLE 配网窗口 | 配网广播不产生新的日常「附近」；前一次合法命中按现有 TTL 过期，App 后台立即清除。 | 由原 WiFiProv 扫描与选择，不代替 Direct 控制鉴权。 |

§6.1、§6.3 分别完成纯 BLE 和 Cloud＋Direct 的单卡生命周期，不能拼成同一物理双卡跨模式的真机结果；旧 IP4 同根两卡有独立证据但未覆盖当前跨模式。下一批先找保留两张现役卡、同物理根且材料匹配的现成样本，核对两卡权威绑定后，分别观察纯 BLE 正常运行、WiFiProv 正常联网和仅开配网窗口不写配置。重点查控制页退出后扫描 owner 交接；若用户期望严格“只亮当前固件对应卡”，需另定义可认证产品标识，不从 MAC、信号强弱或扫描顺序推断。保留 Key、NVS 和原两卡，不为补样本重绑或重配，不改现有 TTL／节拍。

现有双卡匹配、BLE 附近生命周期及首页扫描需求三份定向离线用例 **46／46**（只加载 Angular 编译器与原测试初始化，未改产品依赖或锁文件）。常规 Angular 全类型检查入口仍受当前工作树的非目标规格类型与 WiFiProv 声明不匹配阻断；定向通过不替代整包构建，也不替代双卡跨模式真机证据。

## 8 退出控制页的普通 Direct 释放与双卡准入（2026-11-05）

- 修复位于 App 的页面连接需求和 BLE 原生会话之间。普通页最后一个 scope 退出才申请回收；另一个活跃 scope、同卡 WiFiProv 交接、已发控制或正在确认的 Presence 安装仍持有连接。等待后复核同账号、连接代次和活跃需求；同卡新页面接续时继承回收责任，即使它只走 Cloud 就离页，也不会遗留旧 Direct。网关专用离线 admission、普通 Direct-v2 首装兼容、Cloud 兴趣／State 与配网发现均不改变。
- 首页沿用原有扫描需求；遇到正在关闭或已取消但未释放的 GATT，等待原生释放屏障。失败不伪装成可用；旧释放不覆盖后继状态。不改扫描间隔、TTL、发现策略或页面布局，也不为了即时附近主动打开额外扫描。已有连接仍在用时，首页按现有节拍等待下一轮。
- 连接端口、BLE 生命周期、注册 Presence 和首页扫描四份定向回归 **91／91**；匹配本机 WiFiProv 源的生产模块定向 TypeScript 检查通过。覆盖最后一个 scope、同一时刻／较晚的后继、命令待回执、原生释放成功／未知失败、迟到 opening、网关专用 owner 不被误关，以及普通 Direct 结束后保留原 Cloud State。未生成或安装 APK，未推送。
- 现场只有 COM3、COM11，未满足同板原两卡的三态真机准入。COM3 原 BLE 卡和 BC-5 Cloud 卡曾在同根下提交，但没有原 Cloud 卡 WiFi 已确认、匹配运行 TLS 或两卡在手机均有有效同根 Presence 的证据；现运行普通 BLE。COM11 的原 Cloud＋Direct 已单卡验收，旧 IP4 同根 BLE 别名已正常删除，不能用旧记录或另一块板代替。没有复用旧 UNKNOWN、刷板、开配网窗口、重绑、清根或写 NVS。后续先核精确两卡及匹配运行态，再按 §7 做一次差异短验，不以离线回归代替真机。

### 8.1 COM3 只读准入核对：服务端双卡成立，手机与 WiFi 未齐（2026-11-05）

- 沿用 Development 原会话，从 §25 的原 BLE 源锁定物理实例，按 `ble_direct`／`cloud` 分别做既有只读实例解析，两次均为 200。命中两张现役、不同逻辑 ID 的原卡，Cloud 卡仍为 v1。两个 `no-store` 注册 Presence 查询均为 200：实例、epoch、根绑定、绑定修订及材料所有者一致，当前已确认的 Presence v1 相同；BLE 源可管理，Cloud 别名只读。
- 手机原 BLE 卡 active、实例／epoch 与 v1 对得上，但仍是独立凭据记录，尚未迁入该共享根引用；原 Cloud 卡没有本地有效凭据、共享根引用或 Presence。云端解析也未给出可用 Cloud 在线证明；现场 COM3 仍运行普通 BLE 候选，原 BC-5 WiFi 终态未确认，无可复用的已确认 WiFi profile／该卡匹配的新鲜会话。因此未做同板三态，未同步写本机凭据、未进配网、未重放旧请求或刷板。后续须单列授权引用接线与 WiFi-ready 前置，不能把服务端双卡关系代替手机及硬件证据。

### 8.2 匹配内测 App 与 COM11 正常返回短验通过（2026-11-05）

- 唯一隔离队列基于 §5.2 封存输入，只替换 §8 的 `device-ui.port.ts` 和 `device-v2-ble.service.ts` 两份生产源码；沿用原锁文件、WiFiProv 2.4.4、包名、版本、原签名及 HTTPS 配置。校验 APK 内 1,163 项 Web 资源与构建输出一致，BC-5／旧换 Key 入口未启用。APK SHA-256 `31c6bc46641430a031867b7c7a033f1994b6560a8ae2b53203164a2f9d319c7d`，只原位安装一次，核实实机包摘要。
- 从原首页的 COM11 卡（「云端在线」＋「附近」）普通进入控制页，实机显示「蓝牙已连接」，不下发控制。一次正常返回后，PID 和 WebView 时间原点相同、无后台事件、无重启。在同一 Android GATT client 上观察到成功断开回调及 `close/unregister`（距返回约 **0.27 s**）；原 Cloud 卡恢复「云端已同步」，随后新一轮原生扫描的 Direct-v3 广播通过该卡 HMAC，约 **1.33 s** 恢复附近。释放前短暂显示的「蓝牙已连接」对应仍存在的真实连接，释放后未误报 Cloud 离线。两项时间仅为本次样本。
- 末次只读复核同一账号会话、原 COM11 凭据摘要及两类请求 journal 摘要完全一致，Cloud Key 仍为 v2。保留已安装的匹配内测包，恢复手机原亮屏设置；未刷板、改 NVS、换 Key、重绑、开配网窗口或推送。此结果只关闭 §8 的普通返回缺口，不替代 §8.1 待补的同板双卡三态。

## 9 COM3 双卡：先锁定反向引用恢复与 WiFi-ready 门禁（2026-11-05）

- 不复用原 Cloud→BLE 的本地引用写入口：其源必须有 `cloudContext`，而 COM3 的原 BLE 源没有；反向绑定的 Cloud 版本属于目标卡，不能伪填源凭据。两张注册 Presence GET 只能证明现役同根及当前材料，不能合成旧 intent、commitId 或安装 ACK，也不能把 Presence 只读权当作 App 的 Direct 管理凭据。
- 已增加独立显式的 **BC-5 已提交 Presence 预检**，不接首页、正常恢复或控制入口。复用原同账号私有 checkpoint 和同 intent 只读 `committed` 校验，排除未提交／UNKNOWN。核精确目标、Cloud 版本与安装 locator、原实例／Admin v1 指纹和 access epoch，再沿原认证 `no-store` 接口顺序读这两张卡各一次。要求根／修订一致、源是唯一材料 writer、Cloud 别名只读、两边已确认材料相同且匹配原 BLE 本机的 current。返回非秘密核验结果；不返回 Admin 或 Key、不写本地引用、不申请／轮换 Presence、不重发旧请求。取消或换账号即停，现有私有入口继续关闭。
- 定向软件回归 **57／57**，匹配 WiFiProv 源的生产 TypeScript 检查通过。覆盖确切 committed 后才调用、错目标／租户／locator、本机 Admin、同实例异根、修订、角色、epoch、材料、取消及原 UNKNOWN 保留。原 BC-5 Node 密码学夹具用 Node 环境（不跨 jsdom 的 `Uint8Array` realm）；其余邻接套件沿用原前端环境。无整包构建或真机证据。
- 物理候选核对：`7afb…b348` 双 OTA 分区表与 COM3 既有实读匹配；COM11 的 `47d48…41e64c` factory 版不匹配。`7afb` 普通启动可能对未确认的 WiFi profile 执行确认或回退，不能作为只读检查；维护窗口的 Apply 还要求换 Key，不能用于保原 Key 的 WiFi-only。后续先只读核原 `bl_v2_net` 选中槽、generation／CRC／confirmed 和旧 Apply 证据，不输出 SSID／口令；未确认则不启动该候选，不重放，另行评审限定的保根 WiFi-only 入口。
- 下一步只做两个独立门禁：在能安全取得原 checkpoint 和精确 committed 读时，接现有单根存储的方向化本地 CAS（既有原卡凭据不覆盖，Cloud 只读归属，断点失败不授予半成品别名）；同时做上述只读 WiFi 核验。两个门禁齐备后才跑 §7 同板三态。当前不动 COM3／COM11、App 和服务器运行态，原授权、NVS、Key、UNKNOWN 和日志保留，暂不提交推送。

## 10 COM3 原 checkpoint、单根 CAS 和 WiFi 实读（2026-11-05）

- 原位内测包无需降级。仅从同包 `run-as` 读取专属 BC-5 checkpoint 文件，在内存匹配原 scope 和 exact intent 键；原 v1 checkpoint 仍在且已 armed。原 129 PostgreSQL 以 `BEGIN READ ONLY`／`ROLLBACK` 精确核对原 owner／tenant、intent、commitId、源／目标、根、nonce、目标 v1 locator、发行摘要及提交字段：**同一提交确实已 committed，两张原卡仍现役同根，根修订 1**。不读取 Vault、密钥或原请求正文，不导出 XML 或原始 checkpoint，也未开放旧 HTTP。SQL 结果只作为本轮独立只读证据，不作为 App 可注入的授权回包。
- 已在现有本地凭据及引用库接方向化 **nearby-only Cloud 别名 CAS**：确切已 armed checkpoint 与权威 commitId 相同、原 §9 双卡授权仍匹配后，才形成短时、可取消的证明；写前在同 scope 串行锁内复核源凭据及目标空槽。原 BLE 独占唯一的 Admin／Presence 材料，原记录不迁移或覆盖；只写一个非秘密别名，原样重复零写，冲突不覆盖。读回时重核原源凭据摘要，源消失／变化后别名不可用；目标不会从普通 `load()` 获得 Direct 凭据或 Presence 写权，授权删除只清目标引用。此处是进程内 CAS 和原存储读回，不冒称 Android `commit()` 或跨键持久原子性。
- 反向预检／CAS、原凭据、单根引用及 Presence 存储四份定向回归 **56／56**；匹配原 WiFiProv 源的生产类型检查通过。覆盖不匹配 commitId、未 armed、源材料变化、账号切换、写前取消、已有独立目标、存储失败、幂等及目标删除保源。
- **未在手机写入此别名。** 旧私有 committed 投影本身不回传 commitId，不能供新 CAS 使用；现装包和外部路由不变。不把已验 SQL 拼成产品回包；下一步如继续实际恢复，需同 intent 的受限、只读、可精确核原 commit 的 App 权威入口，并复用现有附近投影接线，不能伪造 v7、改控制或默认扫描。
- COM3 对同一 USB／ROM、真实分区、OTA 选择及原普通 BLE `app0` 独立核对后，仅 ROM 读 **20 KiB NVS**，按原解析器验证结构及记录 CRC。原 Key、Admin、Presence v1 保持；实际 **没有 `bl_v2_net/wifi_sel`**，因此没有选中槽、generation 或对应记录 CRC 可核，`confirmed` 不成立。这不是 CRC 错，也不能据此宣称设备所有 WiFi 数据均为空。只退回原 BLE 固件启动，未启动 WiFi 候选；ROM 进出有短暂中断，无 Flash/NVS 写入，原始临时读件已清理。
- 两项真机门禁未齐，不做同板三态、不用 COM11 代签。下一批若补 WiFi，只走保原 v1 Key／实例／epoch 的显式新 network-only 意图与一次确认，不使用原 BC-5／UNKNOWN 或 `7afb` 的换 Key Apply。原卡、授权、TaskStore、UNKNOWN 和完整日志保留；未改扫描节拍、部署、刷板或推送。

## 11 COM3：精确提交只读接线与 WiFi-only 边界（2026-11-05）

1. Gateway 源码增加独立默认关闭的 `POST /api/v2/internal/bc5-committed/<intentId>`。私有短窗只固定原账号、租户、区域、源／目标卡及 intent；独立 TLS 代理标记，App 不持有标记。请求只带原 checkpoint 的 canonical `commitId`，无通用读写动作。绕开会同步账号／触发协调的原身份 loader，直接用当前 Aily 会话做新鲜所有者确认，拒绝缓存回退；随后单条参数化 SELECT 同快照核原提交、现役双卡、共享根／修订、两侧 Admin 和现役目标 v1 locator。只返回原 commitId 与最小 committed 投影；不读 Vault、不改 ledger／额度、不开放原 BC-5 写入口。POST 只是为了不把 commitId 放进 URL，业务数据不写；独立有界请求预算仍走 Redis。
2. App 源码显式 `restoreCommittedBc5PresenceAlias` 在同一次前台账号操作里读取原 armed checkpoint、一次精确读取、核对原 intent 和 commitId，再依次读现役两卡 Presence，最后调用 §10 本地 CAS。不接受调用方拼装证明，无旧入口回退、自动重试或 checkpoint 清除；原 BLE 凭据与控制权不变，目标仍不能从 `load()` 取得 Direct 凭据。未开放普通页面入口，也未在手机落盘。首页现有候选组装尚未消费这类 nearby-only 引用；验收前只接附近投影，必须同时验证源和目标现役归属、共同根／修订及同份 confirmed material，不能把 Cloud 别名作为第二份控制凭据或把 WiFiProv 广播当 Direct Presence。
3. WiFi-only 另走全新显式意图：先只读核原 `wifi_0/1` 和 selector；缺 selector 不等于槽空，首次写可能覆盖 `wifi_0`，若有遗留槽或未决证据即停。确认空网络基线且权威同一 committed 后，由原 BLE 源的 Admin 经 Method 2 认证同一 instance／epoch／已装 v1 locator，再复用已有 Direct owner 释放、隔离 Security1 SoftAP、固定 Set／Apply 与 WiFi 存储。只准一次新网络写，不重装／刷新 Key，不写 Admin、Presence 或 TaskStore；用独立用途与新 operationId，不放宽旧 BC-5 空 Key 门禁。Apply 排队、选中槽持久确认及同一卡新鲜 TLS／State 是三件事；缺回包只读，不重发。此项仅定合同，未实现或启动候选。

有限回归：Gateway 20 项通过；独立 PostgreSQL 夹具已补同一提交、错 commit／租户、未提交及撤销 Key 后不可读的断言，本机 Docker 未运行、无隔离测试库配置，本批未执行实库用例。App 原目录三份定向回归 69／69，匹配 WiFiProv 的生产 TypeScript 核查通过。没有打开服务入口、安装 App、写手机别名、刷板或推送；保留原 NVS、Key、授权、两卡、UNKNOWN 和完整日志。

下一步先补首页 nearby-only 投影的离线反例及隔离实库查询验证，再开一次私有只读窗口，用原 App checkpoint 写入并核原两卡附近；WiFi-only 就绪后再补正常联网和配网窗口两态。不得以只读回包、一次本地写入或别的板代签三态真机结论。

## 12 首页 nearby-only 投影及 PostgreSQL 回归（2026-11-05）

- 首页沿用原 Direct-v3 扫描和歧义拒绝。只在同一轮清单同时含本人现役、非分享／预览／禁用的原 BLE 源与 Cloud 目标，且目标无独立 Direct 记录时读原本地引用。存储在同一 scope 锁内核源摘要与确切 current epoch／version；扫描器再核该候选的实例、材料与已有根引用。只把已通过 HMAC 的**同一个 current 候选**投影给目标，不复制控制凭据，不另发起扫描或 HMAC；错误、旧材料、歧义或 WiFiProv 广播不投影，原卡匹配规则不变。
- 别名证据独立于 `bleAccess`、普通 `bleNearby` 和 BLE 连接状态，只点亮现有「附近」徽标；主 Cloud／LAN 状态与控制通道不动。沿用原 15 秒观察期限，后台、账号更换、权限失效、源／目标凭据变化和源／目标不再符合清单条件都会收回。徽标保留原可读文字和「非当前控制通道」无障碍标签，不新增样式、控件或扫描节拍。
- App 定向回归 **136／136**，独立 Angular AOT 设备卡真实模板 **42／42**，匹配原 WiFiProv 源的生产 TypeScript 核查通过。覆盖同广播只验证原候选、同 MAC 的配网广播不误投、旧／不匹配材料、不可用／共享／缺失卡、源离开、后台收回及 Cloud 主状态不动。无真机界面或 RF 证据。
- 在 129 专用 `blinker_test` 的临时 schema 内跑原激活 PostgreSQL 套件，**9／9**（8 个子项及外层）。精确读原提交通过；错 commit／租户、未提交、现役 Key 撤销后均不可读，ledger 不变。首轮该新增用例已通过，历史迁移断言把后续 063／064 误计入 061→062 范围；只对齐断言范围后全套通过，未改迁移。只使用私有测试库隧道及专用 schema，不连接业务库、不发布服务，runner 正常回收本次 schema 并保留专用测试库。

下一批：备齐一次**仅恢复**的私有 App 入口与匹配 Gateway TLS 入口，旧 BC-5 写入口保持关闭。沿用原账号、原 armed checkpoint 与 COM3 当前普通 BLE 固件，显式恢复一次本地 nearby-only 引用，短验原两卡、非目标卡、后台收回与 Cloud 主状态；结束关闭私有入口。此批未安装 App、未写手机引用、未刷板、未改 Key、NVS、原卡、UNKNOWN 或请求日志，未推送。WiFi-only 与同板另两态仍按 §11 的独立前置推进，不作为本批通过项。

## 13 COM3 一次仅恢复短验：入口及验收约束（2026-11-05）

- App 复用原 Android checkpoint 存储，不复制或补造安装记录。新增仅原位内测包显式开启的私有恢复子路由；原安装页、原生 checkpoint 写方法和 WiFiProv BC-5 写候选均不随该 profile 开启。原 checkpoint 可过原写入期限；当前服务器受限读窗是另一道独立门禁。页面只允许显式恢复一次，明确本地引用完成不代表设备已发现或已上线；离页取消等待，不自动重试。
- 只允许原 owner／tenant／region／源／目标／intent，入口需真实 TLS、已核验手机出口和短期服务端标记，App 不接触标记。旧写入口、其他意图和直连无标记均关闭。先核已装内测包、原 checkpoint、原账号与两张卡；仅在匹配 APK 和网关都就绪后开窗，先验闭式反例，再做一次恢复。完成或任一结果不明即关闭窗口并核原镜像／代理配置与健康，不重放或补写。
- 验收分别记录：原 checkpoint 与会话保留、本地只读引用 CAS、原 BLE／Cloud 卡附近、非目标卡不串、后台收回和返回按需恢复、Cloud 主状态不变。COM3 保持原普通 BLE，不接串口写入、不换 Key、不重配。若运行候选仍受临时恢复 profile 限定，结束原位恢复上批匹配包，不卸载或清数据；是否看到双卡附近以本次候选在手机上的观察为准，不写成普通发布已验证。

### 13.1 一次真机恢复停在原 BLE 卡 Presence 校验

- App 打包／路由 **20／20**、BC-5 回归 **25／25**、恢复页真实模板 **3／3**；Gateway 定向 **11／11**、短窗收尾替身 **6／6**。只构建一次 Android 候选：原 SDK 2.4.4、原锁文件、原包名及签名，1,164 份 Web 资源逐项匹配；确认原生只读 checkpoint 和配网安全读桥存在，旧写候选关闭。匹配包 `d4b6…7729` 仅原位安装，没有清数据。
- 真机核同一 Development 账号、原 armed checkpoint、原源凭据、原两卡及请求日志，Cloud 目标无 Direct 凭据及本地别名。只以唯一匹配的新鲜手机出口日志放行 exact HTTPS 路由；开窗时旧写与无标记直连仍 404，匿名 401、窄预检 204。未通过的冷探测和过期见证均未用来开放入口。
- **只点击恢复一次**。App 返回「未确认」。同页 Resource Timing 显示原 intent 的 exact read **HTTP 200（约 2.17 s）**；只出现一条普通 Presence 读取，129 原访问日志确认是原 BLE 源，**HTTP 200（距点击约 7.19 s）**，未到 Cloud 目标读取。当前原生 SoftAP 标记为 false。表明精确提交／会话关已通过；目前只把停点限定在首个 Presence 响应合同或源比对边界，未拿到该响应的脱敏分类，不能指认具体字段或将 HTTP 200 当作校验通过。未写本地引用，不做双卡、后台或 WiFi-only 真机销项，不重发。
- 已先关闭 TLS 路由，再恢复原 Gateway 镜像、环境、挂载和代理配置，旧／新私有路由均关；原日志留存，22 个其他容器不变，仅清理本窗生成的两个秘密文件。手机复核原会话、原源凭据、原 checkpoint、原卡数量及两类 journal 摘要一致，目标仍无别名或 Direct 凭据；已原位恢复 `31c6…c7d` 和原 USB 亮屏设置，复查 checkpoint 仍 armed。未刷板、换 Key、重配、推送或改业务数据。

下一步只定位首张 Presence：先离线核对匹配 Gateway 的 `no-store`、返回 schema、当前根／修订与原手机材料摘要；必要时加只记录阶段和失败类别的私有诊断，不输出 key、原始回包、账号或日志正文。没有定位前不再次点击恢复，也不扩大到 WiFi-only 或同板三态。

## 14 首张 Presence：证据对照及有限脱敏诊断（2026-11-05）

- 冻结 APK 与当前对应的 HTTP 端口、拦截器、Presence 校验及 BC-5 证明代码在诊断前一致；Gateway 的 HTTP、Presence、根权限与材料封装四个运行模块与冻结编译字节一致。129 现有 Nginx 普通 Presence 路径唯一，配置 `private, no-store` 且不代理重试，私有 BC-5 路径关闭。源码与运行配置核对不等于保存了 §13.1 那一条实际响应头。
- 用已保留且与 COM3 实读对过的原手机 Presence 摘要做一次严格 owner／tenant／源／目标／intent 的 `READ ONLY` 元数据对照。现役同根修订 **1**，instance／epoch／Admin 元数据匹配；只有一条已确认 current **v1**，没有 previous，材料摘要相等。未读 Key、Vault 或回包正文；只输出版本与相等结果。由此排除**当前库**空 current、版本间隙和材料不一致，不能据此倒推上轮单个 HTTP 200 的实际 App 校验结果。
- 只增加诊断，不放宽校验：HTTP 状态／包络、`no-store`、schema、摘要、本机验证异常与请求失败分别分类；首张和目标按阶段标记，根／修订／角色以及 current 缺失、版本、材料或双卡比对各有固定类别。原账号／取消检查保留，首张失败不读目标，不进入 CAS。私有恢复页最多输出一条 `BC5_RECOVERY_STOP`、阶段和类别，不记录异常对象、ID、密钥、摘要或正文，页面文案、普通入口与重试策略不变。
- 有限回归：App **114／114**、恢复页 Angular AOT／真实模板 **4／4**，Gateway 普通 Presence／生命周期与 Nginx **51／51**。Gateway 和 Nginx 未改；未构建 APK、安装、开窗、请求手机、重跑恢复或推送，原账号、授权、Key、NVS、UNKNOWN 和日志保持。

下一步只取**一条源卡只读诊断**：走原 App HTTP 读取及同一校验，输出脱敏分类；不访问目标、不打开 exact-commit 窗口、不执行别名 CAS。看到确切差异后再决定最小修正，不把已有 HTTP 200、配置正确或材料匹配当作双卡真机验收。

## 15 源卡只读差异诊断（2026-11-05）

- 沿原私有路由及原生 checkpoint 读权限。只有明确的内测构建选择才加载源卡诊断页，此包不提供恢复按钮。保留同账号、原 armed checkpoint、原 App HTTP `no-store`／不自动重试及前台取消；与恢复共用源卡角色、物理根、当前版本和材料校验。源卡诊断**不持有**精确提交返回的 rootBindingId／revision，不产出 proof，不能进入目标读取或 CAS；原恢复仍必须持有该精确绑定。
- 离线回归：App 五组 **117／117**，真实 Angular 模板两页 **8／8**，打包及路由 **10／10**。反例明确源卡诊断即使通过也不能代替精确绑定校验。真机驱动只保留源／目标／私有请求计数、源响应的 HTTP 状态和 no-store／JSON／CORS 布尔值、以及固定诊断分类；不记录报文或 ID。

### 15.1 原 App 源卡单读通过

- 唯一源卡候选 `20b2…7071` 按原 SDK、锁文件、签名和包名完整构建，1,164 份 Web 资源核对；原位安装一次。预检同一原账号／会话、原 armed checkpoint、源凭据及两类请求 journal 未变，目标无 Direct 凭据或别名。通过真实页面显式点一次，**只有源卡 GET 1 次和 OPTIONS 1 次**；无目标、exact commit 或旧 BC-5 请求。
- 页面与固定日志均为 `source-presence / passed`。浏览器可见及网络层 GET 均为 **200**，no-store、JSON、CORS 布尔值全真；App 完整响应合同、物理根／角色与 current 版本／材料检查均通过。未写任何别名，返回原设备列表后摘要与卡数一致；原位恢复 `31c6…c7d`、原屏幕设置并复查 checkpoint 仍 armed。Gateway、代理及 COM3 未操作。
- 该新证据不能追认 §13.1 未留分类的旧失败，也不验证精确提交绑定。两条生产投影从同一原 intent／现役根取 ID 和修订、均做 canonical base64url 与安全数字处理，未找到代码上的类型或编码缺陷；不为猜测根因放宽比较。

## 16 双卡附近有限收口范围（2026-11-05）

基于 §15.1 新证据，仅安排一次带 §14 明确分类的原双卡恢复差异验收。仍使用原 checkpoint，不重放任何设备写或旧 Key／WiFi 操作；单独新的服务器**只读窗口**与 App 诊断序号，不删除上轮结果或重用旧放行见证。唯一操作为 exact committed read、两条原 Presence GET 和已证明的本地 CAS；失败后必须有界留类别并关闭，成功后只验两卡附近、后台清除／返回与 Cloud 主状态，不推进 WiFi-only。原 Gateway 候选镜像可离线复用，不重复构建；结束恢复原服务配置和原 App。

### 16.1 已取到目标读取前的明确缺陷，不继续盲采（2026-11-05）

- 使用一个新的只读窗口、复用原 Gateway 诊断镜像；匹配 App `d661…0e8e` 按原锁文件、SDK、签名构建。真实页面只点击一次，exact committed POST **200**、原 BLE Presence GET **200**，两者各有一次预检。明确终态为 `target-presence / request`，浏览器与服务器均无目标 GET；未进入本地 CAS，无别名或 Direct 目标凭据。
- 代码及非秘密 ID 形状对照确认：反向 Cloud 注册生成器产生 `device_` 加规范的 16 B base64url（29 字符），普通创建产生 `device_UUID`。App 错把 Presence 元数据 ID 送入仅支持 UUID／BLE 路由的 `logicalDevicePeerId`，因此在发请求前抛错；鉴权白名单、回包校验及别名存储索引同样存在后续阻点。已有用例只用 UUID 目标，未覆盖真实反向生成器。该证据不追认 §13.1 的未分类旧故障。
- 窗口已关闭，原 Gateway 镜像、挂载、代理与环境恢复，22 个其他容器未重启，完整日志保留。App 原位恢复 `31c6…c7d` 与原屏幕设置；账号／会话、原源凭据、原 armed checkpoint、卡数和 journal 摘要一致。COM3／COM11、NVS、Key 与任务区未操作。

## 17 元数据逻辑 ID 与通信路由解耦：最小修正范围

1. 增加独立、纯校验的逻辑引用 ID helper，精确兼容已有 UUID、BLE base64url16 和服务器实际生成的 Cloud base64url16；后两种要求规范尾位且非全零。不输出路由字节，不接受任意 opaque 字符串、URL、空白、padding 或路径。
2. 仅接 Presence 请求、原鉴权 URL 白名单、回包元数据校验和原账户分区的存储索引。保留原前缀、ID、角色／根／修订／材料比对及 CAS；附近引用仍为非秘密单独记录，不能成为目标 Direct 凭据。普通凭据保存／解码、Enrollment 与 BBP 编解码仍走原严格路由校验。
3. 使用真实反向生成形状复现后覆盖请求／鉴权、两卡证明、本地保存／读回／独立删除、扫描投影及取消／换号。证明目标 `load` 无凭据、`save` 和伪造持久凭据被拒，源授权不变；不为测试重绑或更换原 ID。
4. 单独保留**账户 MQTT 路由**合同欠项：共享 wire codec 尚不识别旧 Cloud base64 ID，附近引用成功不等于该卡 Cloud 控制可用。后续先统一未来生成器使用既有 UUID，再评估旧 ID 双端兼容；不在本批迁移旧记录或仅放宽 App codec。

通过有限回归后，只允许一个匹配修正候选及一次同原 intent 的只读恢复差异；仍不发 Key、Bootstrap 或 WiFi，不以目标 GET 200 代替双卡完整校验。成功才补首页两卡及后台回收，失败留新分类即停止。未通过前保持双卡三态待验，不扩展 WiFi-only。

### 17.1 有限修正及交付消费者

- 首先用真实29字符目标复现三项本地恢复／读取失败，再修正索引；原 Presence API 的同形状反例也确认在发 HTTP 前失败。元数据校验现已统一到独立 helper，未改 BBP codec、Direct 保存／解码、根权限或手机控制页。
- 七组定向及相邻回归 **141／141**：含真实生成格式、两卡证明、别名幂等与独立删除、源凭据精确保持、伪造目标 Direct 凭据拒绝、取消／换号与附近投影。源码选择及隔离准备 **20／20**；固定回归入口已补 helper、Presence HTTP 及 BC-5 证明三组（定向 **45／45**，与前述有重叠，不累计成额外覆盖）。
- 源码提交清单补齐 helper／规格及 BC-5 证明依赖；混合 API 投影只选精确导入和 Presence 判定，原位置改动与另一批 `INSTALL_STATUS` 保持给定基线，不偷偷并入。缺导入、重复或未知差异仍失败关闭。未提交推送。

### 17.2 匹配候选及 COM3 原两卡真机通过（2026-11-05）

- 唯一修正队列完成完整 Android 构建，APK SHA-256 `dbe941af3f10bb422efcfd0170082cd949a6dd1cd6e78b3d8b3f0ca24d6274b4`；原 SDK 2.4.4、锁文件、签名、包名与原生源码不变，1,164 项 Web 资源逐项匹配。仅覆盖五份元数据相关生产输入，未改通信路由、设备库或服务签发实现。原位安装后核同一会话、原 checkpoint、源凭据、原两卡及 journal 摘要。
- 复用既有 Gateway 只读候选镜像，新建独立短窗；原写入口关闭，匿名401、窄预检204、其他22个容器不变。真实页面**仅点击一次恢复**：exact committed POST、原 BLE Presence GET、原 Cloud Presence GET 各一次，均 **200**；各一次 OPTIONS **204**，无重试、旧入口、其他意图或额外 Presence 请求。两卡合同及材料校验完成，终态 `saved`，只写目标的非秘密 nearby-only 本地引用；目标 Direct 凭据仍不存在。
- 回原首页后，同一 COM3 普通 BLE 广播匹配原 BLE 卡及有明确共根授权的 Cloud 卡。两卡约在观察器启动后 **0.45 s** 同时显示附近；原生 `pause` 时两者均清除，返回 `resume` 后约 **0.37 s** 同时恢复。全程 Cloud 卡保留 WiFi 主图标与「状态未知」，没有因附近命中而改写为云在线；其他卡没有新增误归属，已有其他板的自然附近保留。时间仅是该候选本次样本，不作性能保证。
- 随后关闭短窗并恢复原 Gateway 镜像、环境、挂载与 Nginx，完整日志保留，其他容器未重启，本窗临时秘密文件按所有权移除。原位恢复普通内测包 `31c6…c7d`，手机亮屏设置恢复原值0。最终只读复核：同账号／会话、源凭据、原 armed checkpoint、卡数及任务／自动化 journal 均精确保持；目标只多出已验证附近引用，没有 Direct 凭据。普通包诊断插件按设计关闭，checkpoint 终核改用同包 `run-as` 只读原文件，未开启插件、改写记录或再次请求服务。
- COM3／COM11 固件、NVS、Key、授权与 TaskStore 全程未写，不配网、不重绑、不发码、不升级 SDK、不提交推送。**本批关闭原两卡附近引用恢复与普通 BLE 状态下的双卡前后台欠项**；不是普通发行包已包含该修正的声明，也不是 Cloud 在线／控制或 WiFiProv 三态整体通过。

## 18 下一批最小范围：先排实际阻点，再补剩余两态

1. 不再重跑源卡诊断、精确提交、普通 BLE 双卡附近或后台采样。保留本批真实引用与原卡，不因服务器生成格式差异改名、重绑或合卡。
2. 先离线梳理反向 Cloud 生成器与共享账户 MQTT 路由的 ID 合同：未来新 ID 对齐原有 UUID 形状；旧 base64 Cloud ID 保持原身份，评估双端唯一映射及碰撞拒绝，不能只改 App 解码。明确最小兼容范围并补回归后再决定匹配部署；不要以附近通过掩盖实际 Cloud 控制欠项。
3. 回 §11 第3项，COM3 当前无已确认 selector，不能用启动 WiFi 候选替代槽位核验。仅就**原已安装 Key 但首次网络未完成**评审一次新 network-only 恢复，保留原 Key、双卡、scope 与 UNKNOWN；此受限恢复不是普通换网跳过 Key 轮换。具备条件后再补 WiFiProv 正常联网和配网窗口两态，验证入口与日常附近不混淆。未齐备不刷启动候选，也不重放原 Apply。
4. 修正进入普通匹配内测包时复用已核源码清单与固定回归，不携带本批私有 recovery profile；安装验证只补发行差异，不再为相同成功链造样本。继续暂不提交推送。

## 19 Cloud ID 路由兼容：冻结最小合同（2026-11-05）

本批一次覆盖生成器、服务端路由、App 账户通道和有限回归，不重新运行 §17.2 附近链。

- **未来生成**：反向 Cloud 目标使用原有随机16 B对应的 `device_UUID` 形状，复用共享路由工具；只影响全新未发行意图。已存 prepared／issued／UNKNOWN／committed 原 ID、Key、nonce、quota、scope 与幂等结果不重命名、不迁移、不再分配。
- **旧卡兼容**：服务端共享 wire 工具精确支持规范非零 `device_<base64url16>`；同一 peer 的反查候选为原 UUID、原 BLE、旧 Cloud 三种。Gateway 必须同一数据库快照先统计所有 active 候选，恰好一张才检查 owner／分享／tenant／shard／route revision，不能先按权限过滤再挑卡。Broker 本地反查同样保持唯一性，不按前缀或顺序优先。
- **App 边界**：仅账户 Cloud 通道用独立 peer 适配，保持请求ID＋peer关联和未订阅通知拒绝；完整已知卡集中的冲突不得串卡。普通 Direct codec、Enrollment、控制凭据保存／解码仍严格，旧 Cloud ID 不因账户兼容获得 BLE 管理权限。不得仅在 App 放宽原 decoder 后宣布完成。
- **验证范围**：复现新生成格式不匹配；覆盖旧ID幂等保持、三种候选的唯一／碰撞／撤权、同／跨分片方向及代次、Manifest／State／Presence／控制回执的真实编译消费者。只使用隔离替身／临时数据库与本地回环服务，不伪造用户活动、不重放真实请求。通过后再决定匹配部署，不自动开旧写入口。
- **并行准备**：继续只读梳理 §11 第3项的初次网络未完成恢复。旧 `7afb` 普通启动会写确认／回退，维护 Apply 强制换Key，均不能代替保Key的只读检查；缺 selector 尚不足以确认 `wifi_0/1` 为空。该条未满足前不为补三态直接启动候选。

### 19.1 必要实现及消费者已对齐

- Gateway 新建目标复用原注入随机源，设置 UUIDv4 version／variant 位后用共享工具格式化；同意图重入在生成前返回旧行。新增反例先复现旧生成格式，现已覆盖旧 opaque prepared→issue→UNKNOWN 的 ID／材料不变、幂等不再随机分配、不多扣额度。
- 共享协议正反向支持三种规范身份，Gateway 三个绑定参数的 SQL 与声明同步。全局 active 唯一性仍先于权限；本地 Broker 动态候选、跨分片方向、通知和撤权消费同一 helper，不迁移旧卡、不按 MAC 或相同物理实例合卡。
- App 增加账户专用 `account-peer-id`，通过可选薄端口接 AccountClient、Session、Store、Presence 和 ReadyWaits。原构造调用默认仍严格，Direct 凭据／Enrollment／LAN codec 不放宽。账户仍以原逻辑 ID 保存 Manifest／State／Presence，pending 必须同时匹配 requestId 和 peer；未绑定通知不能猜卡。
- 完整已知目标清单预检及单卡操作共用碰撞围栏：同 ID 去重，异 ID 同字节则关闭并冻结当前账户通道，退役等待及未确认操作，不让先订阅的卡继续可写。新清单只在旧会话已退役后解除围栏；迟到连接／取凭据／PresenceLost 回调不能再修改新会话。默认空清单不会偷偷将同一 live peer 改绑另一张卡。
- 源码交付清单和固定回归入口补齐账户适配及默认端口依赖；Service 原跨分片准入测试入口纳入新 SQL carrier。未改依赖版本、锁文件、设备固件或旧 checkpoint，保留所有无关本地修改及私有 golden 的授权 HOLD。

### 19.2 有限验证与实际证据

- 共享协议原编译通过；Gateway 完整编译及 Broker 原 TypeScript 完整重编均通过。协议＋新 SQL carrier **7／7**；新生成／Key借用／提交合同 **33／33**。
- Broker 八组影响范围共 **169项**：原长耗时跨分片／租约组不重复跑，153项保持同生产输入通过；新碰撞反例最初误写 replay冲突码9，实际正确沿用原 `CommandRejected=6`，只修正测试期望。受影响的两个文件复核 **16／16**，含旧ID本地 Manifest、State、合成命令回执、Presence、撤权和跨分片精确ID，绝非真机设备ACK。
- 复用129既有独立 PostgreSQL16测试库，`test-device-v2-app-device-route.js` 最终 **18／18、0skip**（含父项，约47.2秒）。首次暴露原032生命周期触发器递增分享授权版本，而测试使用固定旧版本的假设；只修fixture，先断言旧快照拒绝，再获取新快照做后续独立负控，未放宽产品。真实SQL覆盖三形状、所有碰撞组合、目标替换、分享撤权与两侧代次。随机schema在finally清理，隧道正常关闭，不连业务库、不重建服务或使用本机Docker。
- App 五组 **90／90**；原 TypeScript 对六份生产模块和三份相应规格的严格 `noEmit` 检查通过。反例包括全零UUID、非规范base64、初始／动态冲突、空清单、取消、旧会话和迟到通知，不授予目标Direct凭据。交付投影及隔离准备 **20／20**，不与旧批次重叠累计。
- **本批未部署、安装App、刷板、发码、重配或提交推送。** 运行环境仍为§17.2收尾状态，账号、授权、Key、TaskStore、scope、UNKNOWN与完整日志不变。源码及隔离回归通过不替代匹配部署、COM3实际Cloud上线或剩余两态真机结果。

## 20 下一批：COM3 已安装 Key 的首次网络恢复，范围到此为止

1. 不再审计或重跑已完成的 ID 形状、两卡普通 BLE 发现及精确提交。先复用原 ROM／NVS只读脚本和CRC解析器，有限核对 **`wifi_sel`、`wifi_0`、`wifi_1` 三键**及原分区／实例；输出仅存在性、schema、generation、confirmed，不输出SSID、口令或原始NVS。当前仅知 selector 缺失，不能据此假定两槽空。任一遗留槽、格式不明、CRC异常或未决写证据则停止并记录分类，不擦除或启动旧候选替代检查。
2. 三键均无记录且原已安装 locator／同实例／epoch／Admin 与 committed 归属匹配时，补独立默认关闭的 **installed-Key initial-network** 窄用途，使用全新 operationId 和 owner，Method2认证及一次消费；不重装Key，不刷新原Key，不复用旧BC-5意图或UNKNOWN，不放宽通用换网必须轮换Key的规则。
3. 复用 `DeviceAccessStore::verifyInitialCloudWifiRoot()` 只读根核验、`Esp32PreCloudDirectRetirement` 真正释放、`InitialCloudWifiOnly::stage/apply/take` RAM排队、主loop `commitInitialCloudWifiOnly()`、原隔离Security1 SoftAP及 `WifiConnectionLifecycle`。不能调用无Key首装专用 `Esp32PreCloudWifiProduct::start()`，也不能伪造本次owner已完成Key安装的handoff。丢回包保留未知，仅只读协调，重启后不自动再写。
4. 先完成上述窄用途离线合同／有限回归，再单队列原SDK编译task0、COM3原双OTA分区的匹配候选，并准备同源Gateway／Broker及不带私有恢复profile的App。届时只补新网络写／持久确认／新鲜TLS与State及WiFiProv正常运行／显式配网窗口两态；不重复§17普通BLE样本、不自动开窗或重发、不升级SDK，暂不提交推送。

## 21 首次网络恢复：三键门禁与设备端窄用途（2026-11-05）

### 21.1 实际前置与最小合同

- COM3 一次 ROM 只读检查：原普通 BLE `app0`、原分区及启动选择精确匹配；完整 **20,480 B NVS** 读取并独立 ROM 校验，原 instance／epoch／Admin／Presence 与已安装 Key 匹配。`wifi_sel`、`wifi_0`、`wifi_1` **全部不存在**，不是只看 selector。原始私有读件清理，板返回原 BLE 程序，串口已释放。没有擦除、写 Flash、调用产品命令或操作 COM11。脱敏证据在忽略目录 `build/arf3-com3-wifi-three-GKUCIP/proof.json`。
- 三键空只证明当前空网络基线，**不授权写入、不解决旧 UNKNOWN**。旧安装 intent／issued／committed 记录、Key、两卡、scope 及日志保持；此次如继续，必须是新的非零 16 B operationId 和显式 owner，不重装或刷新原 Key。
- 专用 `0x3B InitialCloudNetworkHandoff` 请求、`0x3C InitialCloudNetworkAccepted` 回显，body 严格为 **`[1, operationId:bstr16, originalLocator:bstr16]`，36 B**；原 10 B header 后总长46 B。请求 flags=0、sequence非零；响应 flags=IsResponse、同sequence／operation／locator。仅显式 PreCloudDirect、完成 Hello＋真实 Method 2、已提交原加密 session binding、全权限 PlatformAccess Admin 的窄 owner 消费；普通 Direct／Cloud／通用 custom 不接此消息。不会在报文中携带 Key、SSID 或口令。
- 新用途 AP／口令／PoP 复用原 HMAC 派生，domain 独立为 `blinker.initial-cloud-network-handoff.v1`（包含结尾NUL），其余输入顺序及purpose 1／2／3沿原合同。旧安装用途仍是原 domain，字节向量不变；不允许跨用途借用。
- Accepted 仅表示精确授权回复入队，**不等于 App 收到、WiFi 提交或 Cloud 上线**。丢回包仍 UNKNOWN；不重发本次请求、不从旧安装回执伪造新的授权。显式取消、超时、根变化或首个已消费尝试失败均终止该 owner；重握手也不复活。原安装端点继续要求真实安装及其精确回执，未被旁路。

### 21.2 模块化最小实现

- `InitialCloudNetworkRecoveryEndpoint` 只核原根、locator、真实会话和空网络；复用原认证 provider 的一次消费／消费后复验，回调前消费，借用参数先复制，不持有 installer。`IInitialCloudWifiHandoffSource` 只是旧安装端点与新窄端点的共同薄端口，不新造执行链。
- `WifiCredentialStore::verifyInitialNetworkEmpty()` 每次读取 **全部三键**，不依赖 `begin()` 的缓存、不写／清任何键。仅全部 NotFound且无返回字节成功；任一孤儿槽、旧 selector、零长记录、损坏或读错误均失败关闭。supervisor 在交接前及实际 commit 前各自新鲜复核，与原 instance／epoch／Admin版本／locator 核验共用同一主loop owner。
- 原 `PreCloudWifiHandoffSupervisor` 保留真实 native 释放证明、窗口及写前围栏；新用途必须注入空网络门禁，否则不能启动AP。复用原隔离 Security1 SoftAP、RAM stage／apply／take 和主loop单次 journal commit；不以ready、App断开或固定等待当作释放证明。
- `WifiConnectionLifecycle` 只新增可选 **Stop** 候选失败策略：未确认候选的关联失败／超时或 `confirmActive` 持久结果不明后进入Fault，不自动回滚、重连或再次确认写。默认 Rollback／Retry保持；已确认网络仍沿原正常重连，不因本批禁止一般Cloud恢复。
- 私有 `Esp32InitialNetworkRecoveryProduct` 只允许显式内部宏、XIAO ESP32S3、task0、HTTPS＋MQTTS；普通选择器不引用。先读原身份、原根及三键，再开原stack，身份存储只读代理禁止 `loadOrCreate()` 的失败回退写入；无TaskInstaller／TaskStore或Key安装。私有期不attach Client／Cloud；交接前先按原poll排空回执队列，防止有界发送剩余分片被停机丢弃，不固定延时，也不把本地排空当App收到。真实 BLE／HTTP／AP释放及单次commit后才交给原WiFi／Cloud生命周期。未知native关闭保留回调图并锁故障，不盲目析构／重启。

### 21.3 有限验证与证据分层

- COM3 三键分类器内存反例13项通过，涵盖孤儿槽、CRC、generation及未知写证据；不作额外真机读取或业务重放。
- 新 baseline／Stop生命周期／network-only认证／supervisor用途隔离，以及相邻原安装报文、原交接与runtime共 **7个CTest入口**通过；新端点含7组有限host反例，另原AP派生向量 **2组**通过。新增runtime分支覆盖Hello前、认证前、认证后明文退役、错误flags／响应方向拒绝及正确专用分派。host替身与确定性AEAD不是实机认证或持久化证明。
- 新测试初次暴露两处fixture假设：BLE-only根不能用普通replace补Key，以及认证后明文会按原安全规则退役会话。仅改测试初始化／隔离样本，未放宽任何产品校验。
- 原 SDK 定向完整链接通过：隔离冻结当前库、原 **ESP32 Core 3.3.11／PubSubClient 2.8**、`XIAO_ESP32S3:PartitionScheme=default_8MB`，唯一队列、jobs=1；首轮后只将回执排空修正同步至同一stage，对受影响TU增量重链，无并行重复队列。最终CLI报告程序 **1,342,548 B**、静态全局 **58,124 B**，可容原 `app0=0x330000` 双OTA分区；不把静态全局报告当动态RAM／栈或真机运行证明。
- `.d`确认实际消费冻结stage的新Product／Endpoint／Baseline／wire；11份关键生产输入逐字匹配。最终ELF实际含新用途端点、会话消费／复验、空网络门禁、原Security1配置处理／HTTPD及NimBLE共7项必要消费者；无TaskSnapshotStore／TaskEngine／TaskInstaller、rekey wrapper或新任务图。保留原SDK自身BT／panic／日志wrapper，不将它们误判为本批rekey接线。ELF SHA-256 `5090570ec5fb7f55366aade27c3cfd921643ddea1bd506e40277785e657ba431`；BIN SHA-256 `60f0ffda243ff9ddc5c6c79bce963afcc8e994e3c0d43b6e284b320dd0568117`。
- `Esp32InitialNetworkRecoveryProbe` 仅含 `.invalid`、非秘密dummy operation／locator和`linkOnly=false`，**禁止烧录，不是匹配运行候选，也未作运行验收**。下一批仍需同源App入口及实际profile，不能以host或链接通过宣布首次联网／Cloud／剩余两态完成。
- 本批没有刷板、安装App、部署服务、打开恢复入口、写网络或重发旧意图；账号、授权、Key、TaskStore、scope、UNKNOWN及完整日志保持，暂不提交推送。

### 21.4 下一批只补必要产品入口与剩余两态

1. 不再读取COM3三键凑样本、不重做§17附近生命周期或§19路由反例。复用本批实读及已核源码，准备原SDK、原双OTA分区的**匹配运行候选**，不是烧录编译probe；默认不自动启动恢复、不带Key安装路径，新的operation与原locator精确绑定。
2. App只接显式新用途薄端口：原Admin＋Method2、一次0x3B、精确0x3C及新purpose派生、真实owner交接、一次WiFi提交。保留原产品导航、账号、CAS、持久意图及UNKNOWN只读协调，不复用旧Key安装handoff或通用换网绕过轮换。匹配Gateway／Broker先部署§19兼容再测旧Cloud卡，普通App不携私有恢复profile。
   - 已只读定位具体缺口：`types.ts`、`ble-direct/secure-channel.ts`／`client.ts` 当前仅认旧0x38／39／3A；新用途需独立purpose策略与一次exchange，复用原Method2与ATT协商。`cloud-registration-carrier.ts`只抽取共用派生算法并选新domain，不改旧向量。
   - 独立非秘密operation记录复用现有checked commit／CAS内核，绑定当前账号、原instance／Admin版本、原卡及locator、phase与dispatch时间；不覆盖旧BC5 checkpoint，不借用强制Key版本+1的rekey记录，不存secret／binding／AP凭据。私有旧记录只读配置不放宽，应选匹配的新用途边界。
   - 原`holdInitialCloudWifiCarrier()`、`createInitialCloudWifiProvConsumer()`、`InitialCloudWifiContinuation`、原生WiFi-only提交／关闭及Cloud observation均可复用；不新造native协议、HTTP发Key入口或安装／commit链。本地Accepted、WiFi结果及新鲜Cloud观察分别记账，不互相覆盖为成功。
3. 具备条件后一次短窗补网络持久确认、新鲜TLS／Cloud State以及WiFiProv正常运行／显式配网窗口两态；分清SoftAP发现入口、普通Direct附近和Cloud主状态。保持原两卡、授权、Key及日志，不清根／重绑／迁scope／改扫描节拍；结果不明只读协调，不自动重发或开新窗口。

## 22 App薄接线、匹配运行候选及一次写前停点（2026-11-05）

### 22.1 本批完成的最小实现

- App在原Direct客户端内增加独立`installed-key-network`用途，仅发送一次0x3B并严格核对0x3C、sequence、operation及原locator；原Method2／ATT与连接owner复用。旧安装用途和普通Direct不能跨用途消费。新AP派生共用原算法，仅选独立domain，旧向量不变；没有Key发行、安装、轮换或注册端口。
- 独立非秘密checkpoint复用原原生checked commit／CAS，绑定账号、租户、原两卡、根五元组、原locator和新operation。分别记录prepared、授权发送／接受、WiFi发送／接受／拒绝／未知及独立Cloud观察时间；原BC-5记录只读。CAS不明或恢复出的记录只能显式只读，不能借重入重放。记录不包含Key、session binding、AP口令或WiFi凭据。
- 复用原页面表单、一次显式操作、取消和只读入口，不新增安装／注册流程。新私有native profile只给原记录读取及新记录的精确写权限；普通包默认无此profile。WiFi提交前等待新发送围栏的持久确认，已经确认的设备结果不会因随后本机日志失败而撤回。
- Cloud观察仅在新用途显式采用metadata-reference，兼容原旧Cloud逻辑ID；默认allocation／Direct解码保持严格。仍核同卡、instance、locator、版本、真实Local释放及本次WiFi之后的新会话／新鲜State，不以HTTP200、附近或旧在线标志当作上线。

### 22.2 有限回归与匹配产物

- 部署前五份核心源规格 **62／62**、原页面Angular真实模板 **6／6**；独立原生策略／存储 **9／9**，profile **24／24**，源码交付投影及准备 **22／22**。匹配WiFiProv源码的TypeScript检查通过；相关协议／观察规格另有定向结果，存在重叠，不相加冒称总覆盖。
- 唯一匹配APK为`8e7b231ceeac20cc65ca4f5fcd7e6bdefe7c262ad02b9f811352432777ba446c`，原包名、签名、锁文件及WiFiProv **2.4.4**，HTTPS、日志默认关闭。1,166份Web输入与包内资源逐项匹配，原位安装不清数据。profile验证首次因测试子进程继承新候选环境而STOP；只隔离该测试环境后在同一队列继续，不升级SDK或另起重复构建。
- COM3匹配运行候选沿用原 **ESP32 Core 3.3.11／PubSubClient 2.8／default_8MB双OTA**，task0；程序报告 **1,347,352 B**、静态全局 **58,148 B**。BIN SHA-256 `dae7da1cd297d902a131d6773fce413bf0d2d6bb163283ea628231ac9c0f01ca`，ELF `f74af9d89955f7c083d6d80ce7b803b92d4e01be6d4f62067e0bb3bfac06bfe9`。真实profile、原locator及全新operation匹配，默认关闭；不是§21编译probe，也不是日常WiFiProv＋Direct完整产品图。
- Gateway／Broker仅临时接入§19旧ID运行兼容及原精确提交只读入口；没有启用旧BC-5写入口、迁移业务库或改Auth／AI。入口要求原会话与精确原记录；匿名拒绝、窄CORS及其他容器不变均实际核对。未来ID生成器没有借本次最小镜像一并发布。

### 22.3 真实一次尝试：未到设备配置，不能算联网通过

- 原账号、原两卡、原armed checkpoint和源授权先核对。COM3只替换原app0并独立校验，分区、启动选择器、NVS及Key保持，COM11不动。仅显式开启一次10分钟窗口、点击一次「连接目标Wi-Fi」，没有重试或重开。
- 同一手机Resource Timing记录原exact committed读取 **HTTP200、约1.305 s**；随后页面停在预检，无法从该状态码推断响应合同／卡归属或源发现校验已经通过。本轮未捕获该请求的requestId，不补造关联字段。
- 原生读取本次checkpoint返回null，显式只读按钮显示「没有本次联网记录，未发送任何配置」。没有进入后续0x3B授权或WiFi提交，没有Key安装／刷新、网络写入、Cloud确认或三态结果。**null只证明没有读到持久记录，不单凭它宣称从未尝试prepare／CAS；未取得持久确认就不能越过发送围栏。** 旧UNKNOWN不因此得到解决。
- 现存证据将停点限定在精确读取后的权威／卡核对、源凭据／发现或prepare边界，尚不能指认具体字段。当前网络有Android validated及一次独立匿名HTTP到达证据，App没有绑定Local；不把另一个诊断路径的CORS失败误判为断网，也不以此倒推具体业务失败原因。
- 此后补源码级固定阶段诊断`INITIAL_NETWORK_PREFLIGHT_STOP`（checkpoint／carrier／cloud／authority／source／prepare），不保留原异常、ID或回包。页面明确「尚未发送本次设备配置」、只读查询及无自动重发；已有发送后的UNKNOWN合同不改。该诊断版本原模板 **7／7**，没有再次构建安装或采样。
- **离线随后找到一个确定的必然阻断**：新Service入口误调用原Cloud→BLE的`isAccessRegistrationSource()`，该predicate要求`cloudContext`存在，而同一条件又拒绝`credential.cloudContext`，任何凭据都会被拒绝。新用途应读取原BLE-only enrollment源，不是方向相反的Cloud源；此项不是等待、网络或SDK问题。已改为独立窄判定，沿原BC-5 enrollment缺省、active／全Admin／无Cloud与轮换条件，并核原逻辑源、instance、epoch、controller及版本；原Method2仍验证精确根及指纹，Cloud源／别名不获新权。定向核心 **13／13**、生产接线`noEmit`通过，含方向及各根字段负控。**源码确定缺陷已修正，不冒称已证明它就是这次最早停点，也不冒称修正APK或首次网络已通过。**
- 同批发现冷页前置缺口：`Page.navigate`全页进入tools会失去此前首页内存；`getDevice()`只同步读内存，不加载目录，账号恢复后的异步全量读取也不被tools路由等待。缓存可能不存在，且展示缓存本身不是新鲜ACL。因此不能靠等首页、固定延时或ready快照消除此依赖。
- App已移除该重复展示缓存门禁，仍先等同一次exact committed及精确绑定校验。Gateway在原单次SQL里补两卡有效网关路由的排除，复用原`listCloudCapableChildren`语义：相同tenant、Active topology、同一观测时间下未过期、现役child及匹配epoch、现役edge-hub及当前Key版本；任一侧有效路由即拒绝。原SQL已覆盖当前owner／tenant、两卡现役、BLE-only／Cloud角色、双Admin、原Key及locator，未使用分享身份；没有新增HTTP、回包字段、目录请求或迁移。**这是私有exact恢复仅支持非网关路由卡的窄收紧，普通分享、读取和网关路由不变；App与新reader必须匹配部署，不能复用旧operator冻结字节冒称已含此修正。**
- 真实Service消费者与原模板 **8／8**，同配置`noEmit`通过。保留真实协调器及Service创建的source选择，enrollment／legacy达到目标选择端口，Cloud源拒绝；冷目录`getDevice`设为抛错spy且从未调用，三路均清零凭据／释放连接，prepare及无线发送均为0。前两次STOP是Angular不支持该spy／mock夹具方式，仅改夹具，不算产品失败；未追加产品测试基础设施。Gateway相邻四文件 **32／32**，新增SQL实库验证按同一维护测试库的独立schema执行，不连接业务库。
- 复用原`run-gateway-isolated-postgres.py`、原Node22及129维护PostgreSQL16，单文件、单队列、独立schema。首轮原8个子项通过，新fixture因同owner新建第二tenant违反唯一约束而停；只改为复用已有独立owner的tenant，不放宽数据库或产品。显式复核时，**新增源／目标共26个路由场景全部通过**（该子项约20.24 s），包含Active拒绝、到期边界、非Active、错epoch／tenant／hub状态／类型／Key版本放行，并核原ledger／拓扑未被读取改写。
- **整文件不记全绿**：复核前8个子项通过，运行最后一个旧不可变性子项时触及父测试120 s预算，after清理关闭observer，继而出现Client closed断言；总约121.2 s、exit1，统计8pass／1fail／1cancel／0skip（含父项）。该旧末项在首轮、相同生产reader字节下通过。受影响的新SQL场景实库通过与整队列超时分别保留，不合并成10／10；不涨预算或第三次重跑原项。独立进程／隧道已结束，维护测试库保留，无业务库／服务部署操作。reader冻结SHA-256 `e7ab02ff8787c687073aa08bd7f6f34df8d40172cdb0bfd916abcde8d14de0b0`，修正后PG规格`542e1fba38f76e4ea1b41514a39eac17a4678b1b4f69a927de707147b4c90c1e`；测试预算欠项不当作业务根因。

### 22.4 收尾与下一批唯一范围

- 已显式取消唯一板端窗口并关闭串口。COM3原普通BLE BIN `987f108e17c5fb5aade42ae1b951cdbfaabaabc46c3227dbb20ec5ab33012ac3`已精确恢复及独立校验，原分区／selector不变；没有新读写NVS、任务或身份。恢复证明：`build/arf3-com3-initial-network-restore-ctmjEv/proof.json`；被动USB／串口正常不冒称业务通过。
- 已先关闭私有TLS入口，再恢复原Gateway／Broker镜像及完整配置；原HTTP／TLS临时入口均关，其他服务未重启，完整日志保留，只清本窗所属临时秘密文件。最终回执位于服务仓库忽略目录`build/initial-network-service-preflight-20261105/snapshot-4NIUir/operator-initial-network-close-1793866318713619100.private.json`。
- 手机原位恢复普通内测包`31c6…c7d`，终核同账号／会话、原源凭据、原checkpoint、**7张原卡**、nearby-only别名、目标无Direct凭据及任务／自动化journal摘要均精确保持；恢复原USB亮屏／超时设置。候选、未知意图与私有日志保留，没有提交推送。
- 下一批在上述源码与数据库验证完成后，重新冻结匹配App／Gateway和新operation运行profile；原取消owner不复活，旧请求不重放。仅在全部入口就绪后开一个新窗口，若预检仍停则按固定阶段收集分类，不再无分类点击，不要求重新登录／发码／认领或重做附近链。不能因发现这两处阻点便跳过原精确提交／所有者／CAS前置。
- 网络持久确认与新鲜TLS／State通过之后，才用真正匹配的日常WiFiProv＋Direct图补「正常联网」「显式配网窗口」两态。当前维护候选不具备完整日常图，不能用它代签；不因本次预检欠证关闭既有Direct或改扫描节拍。

## 23 匹配部署：预检阻点已跨过，交接接受持久化已验（2026-11-05）

### 23.1 新冻结及有限部署

- 复用§22已验stage，只覆盖5份必要App输入：新用途coordinator、Service、原页面及两份规格；其余2064项来源逐项保持，未混入无关dirty修改。唯一APK SHA-256 **`9e878b7e350c5003cd29b72facd262dcf45702b9791fb08b8200e2adc08ec4a2`**；原签名／包名、SDK2.4.4、锁文件及原生源不变。新profile24／24、隔离类型核查、完整Web／原SDK APK打包及1,166份Web资源核查通过，没有重跑已过的大矩阵。
- 新operation独立于原取消owner，SHA-256 `6b04595fd2d91c3d01810c5cadf2da325ee7e9978019fb156834985ab4a60708`，仍绑定原两卡／根／locator。wrapper SHA-256 `d82867fd027dfe522897929056b190cba8c8b0f42430449accd7320e4bb2f573`；8小时只是构建配置有效期，实际设备仍一次显式10分钟窗口，交接及RPC沿原独立截止，不延长。
- 原SDK3.3.11／PubSubClient2.8唯一队列完整链接，94项真实编译命令及消费者核对通过。BIN **`cb1b13db79d09366bf9fc40c0b05054d7b361758f3afc00c7be0f41da7364d1c`**、1,347,504 B；CLI程序1,347,352 B、静态全局58,148 B，task0／原双OTA保持。新operation在ELF／BIN实际存在，旧operation不在BIN；不取缓存中的旧输出当作新候选。
- 129先做新鲜只读快照，再独立冻结新reader及旧ID兼容；新Gateway `75c42518…`、Broker `7fcf0151…`，只切换这两容器。新reader字节在镜像中核验，原Auth／AI、业务库及其他服务不改，旧写入口仍关闭；真实手机匿名来源见证后才开原intent的600秒只读TLS入口。新批次工具与备份独立，不覆盖旧freeze／operator／日志；未重跑数据库场景。

### 23.2 一次真实路径与退出点

- 安装前普通31c6原包及原会话／7张卡／旧checkpoint／源凭据／别名／两类journal均核对。首次前台门禁停止后，仅纠正USB亮屏及真实Activity前台，再显式复核通过，未弱化门禁。原位安装9e878，不清data、退出重登、发码、创建卡或读取／发行Key；冷预检的同profile／原记录／账号／前台／WiFi-only能力／权限全部通过。
- COM3仅app0写入新候选并独立校验，原分区／selector不变，ROM工具未读写NVS；产品运行仍执行原只读根／空网络门禁。COM11未操作。仅发送一次`INR-OPEN`。首个串口工具因非PTY的stdin EOF释放端口；随后**只读观察**证实同一个已消费owner仍在Provisioning，未第二次OPEN、复位或重发。
- 原页面仅点击一次。exact committed POST **200**、对应OPTIONS **204**各一次；没有其他intent、旧写／Key端点或自动重试。新持久记录为created `1793872402725` → dispatched `1793872404761` → accepted `1793872404868`。最后两标记相差 **107 ms**，仅表示同一App时钟下发送围栏到精确接受记录，不冒称纯无线RTT或WiFi完成。
- 实际跨过原两个预检阻点，完成原Admin／Method2及精确新0x3C；派生的设备热点实际出现在Oppo的`NetworkRequestDialogActivity`。**未证明该热点的Security1连接完成**。PC观察约64.17秒后页面显示未确认；本轮系统列表的一次选择发生在该交接期间，没有重新提交或点击WiFi确认。PC／App／设备时钟分别记录，不将总耗时强行等同SDK某个deadline。
- 原生checkpoint只读确认 **`phase=accepted`，无`wifiDispatchedAt`及Cloud观察字段**，没有越过WiFi发送持久围栏，不把交接接受当作网络配置／Cloud成功。设备后续状态Fault／StateConflict（18）、networkReady=0、cloudReady=0；该状态不证明密码错误，也不单凭它指认停止原因。随后一次页面只读按钮仍返回未确认且无HTTP，未保存该时刻完整前台证据，**不记为页面恢复通过**。
- 既有Android主进程／worker环缓没有可判定的WiFiProv固定code；目前只能定位在精确接受后、WiFi dispatch前，不能确认连接超时、真正后台取消或owner释放。取证曾误将CDP requestId前缀当Android PID，已更正：主PID以本批ready记录23937为准，requestId是不透明标识，不从其前缀推断进程；追加正确PID只读日志后结论仍欠证。相关原日志及更正记录私有保留，无rawargs输出。

### 23.3 收尾与保留

- 已显式取消并释放观察串口，精确恢复COM3原普通BLE `987f…ac3`及独立读回；原分区／selector不变，未进行ROM NVS写入或其他板操作。证明：`build/arf3-224-com3-restore-k39SAF/proof.json`。
- 先关TLS，再恢复原Gateway／Broker及完整配置，其他服务不重启，原日志保留，仅清本批所属秘密gate／marker。新服务批次证据在服务仓库`build/initial-network-service-224-20261105/snapshot-PBhT9W`，回执closed／originalGatewayAndBroker／originalConfig／logsPreserved均true。
- 手机原位恢复31c6，结束核同账号／会话、原两卡及总7卡、源授权、旧armed checkpoint、附近别名和两类journal摘要保持。新accepted记录在恢复App后的精确原文件只读核对仍在，admission及4个阶段字段与此前一致，hash `fca47e5864fc97de075ceb5377496e5f6c1c8d50ebc9a01999ae0a75823931d6`；没有补造／清理／重放记录，旧UNKNOWN不因此解决。原USB亮屏及超时设置已精确恢复。
- 未生成Cloud成功proof，**没有执行**已准备的postconfirmed ROM／三键核验／7afb日常固件切换，也不以候选准备代签剩余两态。未升级SDK、提交或推送。

### 23.4 下一批最小范围

1. 不再重验精确提交、原源选择、Method2、0x3C或accepted持久化。**本批收尾已补源码**：仅原carrier／其规格／Service三文件，默认关闭、最多两条固定阶段／原因诊断create／connect／submit／close；八种固定reason，不保留raw exception／code／cause／body／secret。只在新用途接`INITIAL_NETWORK_WIFI_STOP`，普通BC-5不接；同步sink失败不改变原outcome、真实release或发送次数。原carrier有限规格 **16／16**、匹配生产接线`noEmit`通过，无native／SDK／超时／UI／执行语义变更。诊断仅覆盖原消费try/catch及close，非所有外围异常的完整追踪；未重新构建／部署，本次9e878不含此后补源码，也不据此倒推根因。下一匹配包及手机观察器须同时核对该固定prefix、阶段和reason白名单。
2. 在窗口外备齐OEM系统选网动作及取证，选择依据必须来自本次WiFiProv请求的精确目标及当前系统请求页；不按MAC／任意BLK前缀猜设备，不在60秒连接期间临时排查脚本。不增加SDK等待预算、不将系统ready当owner释放、不忽略真正后台；连接后仍须原App明确确认才越过WiFi发送围栏。
3. 本次owner已取消且记录只能读，不用旧accepted记录重发0x3B／WiFi或恢复secret。后续仅在新显式owner／operation、原Key及空网络条件重新具备时补未完成后半；全部入口先准备好，不重新认领／发Key／重登／重跑首装或普通BLE附近链。
4. 只有网络真实持久确认和新鲜Cloud通过后，才按已准备的`--post-confirmed`入口只读核选中槽CRC／generation／confirmed及原根，再切日常匹配图补正常联网／显式配网窗口；不执行缺proof的捷径。原7afb无主动cancel，此后也不能拿复位／到期冒充主动取消通过。

## 24 SoftAP交接固定分类与最小补证（2026-11-05）

### 24.1 匹配诊断及一次真实差异

- 复用§23隔离stage，只覆盖原carrier、规格及Service三份必要输入；匹配APK SHA-256 **`95ff1862012568880fb6113688e8977e88b10b0e311a75c35d72b0874c73caf0`**。原SDK、WiFiProv 2.4.4、锁文件、包名及签名不变；profile **24／24**、类型检查、Web／Android完整构建及包内资源核对通过。§23.4的默认关闭诊断已包含于本包，不重复原大矩阵。
- 使用新的独立operation，仍精确绑定原两卡、根与locator。原SDK增量仅重编必要Sketch并完整链接，BIN **`36107402b058cdf6fc7a862fc4fa5420795a5299e2d76c1d451330570517b59e`**，task0及原双OTA不变；旧产物另存，不能以已更新的缓存路径冒称旧产物。App只原位安装；COM3只替换app0并独立校验，原分区／selector不变，ROM不读写NVS，COM11不动。
- Gateway／Broker复用已验镜像，新的原intent只读短窗独立留痕；Auth／AI、数据库与其他服务不改。真实同一账号及原资料预检通过，不退出重登、不发验证码、不创建设备、不读取或发行Key。只开启一次板端owner、点击一次原页面操作；exact committed POST **200**及OPTIONS **204**各一次，无自动重试。
- 新记录为`createdAt=1793880242435`、`dispatchedAt=1793880244511`、`acceptedAt=1793880244647`。实际通过原Admin／Method2、精确0x3C及接受持久化；**这些不是WiFi或Cloud确认**。固定诊断明确为 **`INITIAL_NETWORK_WIFI_STOP / connect / timeout`**；`wifiDispatchedAt`与Cloud字段均不存在，未点击后续WiFi确认、未发送配置。
- OEM选择工具只观察真实本次WiFiProv请求的精确SSID，并核当前系统NetworkRequest页面、启用条目及屏幕范围；不按MAC或任意BLK前缀猜设备。本轮没有满足选择条件，未执行系统选网点击。超时后系统页显示未找到可用网络；随后PC检查也未见目标，但已经过板端交接窗口，**不能据此倒推整个连接期间AP从未出现**。板端最后Fault／StateConflict（18）可能包含退出清理结果，不能直接归因密码、派生或射频。

### 24.2 已排除的离线反例与仍缺证据

- 将**实际执行的**选网函数放入纯I/O替身，原Oppo系统Activity及精确目标的正控可选择，含原空列表的七项反例均不点击；没有找到语法、屏幕范围或精确匹配的确定缺陷。观察桥只透传原Native Promise及参数，在内存提取非秘密SSID／候选标识，结束移除，不改插件、返回值、生命周期或调用次数；观察桥有限规格 **3／3**、选择器规格 **5／5**。
- 用同一合成非秘密向量分别运行真实App派生实现及真实冻结C++实现，SSID／AP口令／PoP三项摘要完全一致；未使用或导出真机秘密。独立用途domain、分隔零字节及输入长度一致。该对照只排除算法层差异，**不证明真机实际输入或AP启动正确**。
- 已只读核交接参数的用途／长度和同步复制边界，未发现确定传参缺陷。当前维护Sketch状态只含owner、终态及network／cloud布尔，缺少连接期间AP初始化、模式、配置、netif及DHCP的同期证据。不能因这些离线对照通过便延长deadline、放宽后台或再次盲开owner。

### 24.3 本批收尾与下一批唯一范围

- COM3原普通BLE **`987f…ac3`**已恢复并独立校验，原分区／selector不变；正常复位后串口可被动打开并释放，未观察到panic，不把零字节串口当完整启动或业务通过。证明：`build/arf3-234-com3-restore-GOnlW9/proof.json`。未执行缺少网络确认proof的postconfirmed读取、日常WiFiProv固件切换或剩余两态。
- 手机原位恢复普通包 **`31c6…c7d`**并核实际已装SHA。终核同账号／会话、7张原卡、原源授权、旧armed checkpoint、nearby-only引用、目标无Direct凭据及两类journal摘要保持；原亮屏设置恢复。新accepted记录在普通包下经原文件精确只读核对，admission、phase及三个阶段时间与采样一致，未记录WiFi dispatch／Cloud；摘要`d79396233fa07b4637885795ce58aa408b8530fb056e0e41e5773038ca726a09`。保留全部原记录，不清理或重放UNKNOWN。
- 服务关闭先回原TLS配置，再恢复原Gateway／Broker。新鲜独立预检及收尾检查确认原镜像、健康、compose／Nginx原字节、完整环境／挂载事实、runtime、关闭路由与其他容器不变。工具随后在raw配置摘要断言处STOP：备份用有序数组hash而旧事实按唯一键记录，G／B raw hash不等；**不把该STOP误报为仍在运行候选，也不冒称原raw字节已还原**。独立收尾只规范化Env／Mount数组次序，拒绝重复键及任何字段值变化，离线 **6／6**；真实前后完整配置逐值核对通过。原raw次序未备份，因此不声称已观察到具体哪一数组换序；原摘要差异作为工具限制保留。随后核定本窗两份临时秘密文件的固定路径、0600、普通非链接、内容摘要、当前无Nginx引用且没有任何运行容器挂载，才精确移除；日志及旧证据保留。最终`closed／originalGatewayAndBroker／originalConfigBytes／allConfigurationValuesOriginal／ownedSecretsDisposed／logsPreserved`全真，无额外服务重启／代理reload／SQL。失效旧窗不复活，必要时另建新窗。
- 下一批只准备维护Sketch级、**显式INR-STATUS**触发的被动AP观测：原IDF getter成功后才读AP配置，SSID仅长度及摘要，netif存在才读up／DHCP；不读出密码／PoP、不初始化或启停WiFi、不增设事件订阅／后台轮询。各状态只用于定位，不授予权限，也不等同RF广播、可连接或owner释放。先有限替身回归及原SDK增量核查，再决定一次同期观测；不改产品SDK、注册协议、扫描节拍或deadline。
- 上述观测已在独立忽略目录`build/arf3-native-ap-observer-20261105`准备：Sketch局部header加5行适配补丁，原已执行文件／产物不变，**10项stub通过**（未初始化、STA不读AP、空netif、DHCP错误、SSID长度、完整配置清零、脱敏及输出上界）。header SHA-256 `8a675f593ea1aac7e3033395ecf5cd5bd594e42cd10efdb6ee88396cb52b07ee`。原getter可能用已有netif IPC，故只显式采集并记录耗时，不宣称硬实时；**尚未应用补丁、原SDK编译、生成新operation或部署**，不能将stub代签真机。下一批复用此薄适配，不再设计新诊断框架。
- 本批没有SDK升级、扩槽、Key／身份／任务写入或git提交推送。首次网络及后续两态仍未通过；下一步应取设备同期AP事实，不重复首装、附近、精确提交或accepted链验收。

## 25 原SDK链接及COM3连接期AP同期观测（2026-11-05）

### 25.1 单一编译队列及匹配输入

- 应用§24已审Sketch局部header与5行薄适配，生成全新独立operation，仍绑定原两卡／根／locator。原已执行361制品、头文件及编译来源先按摘要单独留存；原执行证据不覆盖。新profile SHA-256 `1bede6796ae269b994885f44eac74464969448d85683182f64e44a96fd518a25`；8小时仅为构建配置期限，设备实际一次10分钟、交接与连接deadline均不延长。
- 原ESP32 Core **3.3.11**／PubSubClient **2.8**／XIAO ESP32S3／`default_8MB`，同路径唯一jobs1队列完整链接通过，约17分26秒。**实际更新94个对象及2个archive**：Arduino因新include扩大依赖失效，不能写成仅重编1个TU；未改库／SDK源码、升级版本或另开队列。程序BIN **1,348,784 B**，比361增1,280 B；静态全局仍 **58,148 B**，task0、原TLS及双OTA不变。
- 新冻结BIN **`60bb4b63850274b190ad3c3b4e77fbf7d4d528f1fed1a8cc8932bd9942b5c91f`**、ELF **`a7dbe1c1a39841024268d367246de1a1ae1bada0d6f074d39530785a7178da1d`**。完整链接后，额外consumer断言曾因依赖表使用Arduino复制头路径而STOP；未重编、未改旧runner。独立终审核实际`.d`路径、唯一`#line`回指、复制头去该行后字节精确相同，以及capture对象的7个getter引用、ELF符号和BIN fixture均通过，再冻结候选。此为工具路径误拒，不是产品或编译失败。
- 匹配App只换新profile，复用§24的三份诊断源，原SDK2.4.4、锁文件、签名、包名及原生源不变。profile **24／24**、类型、Web／Android构建及 **1,166份www／1,168份包内资源**核对通过；APK **`814d8b2a4a73da9bf59cddcc7a8d4ca4e0396cb0d863aa0314511c5f6f8e478d`**。原位安装不清data；同账号、原7卡、旧checkpoint／源材料／附近引用及journal先留基线。
- COM3仅app0写入并独立校验，先核原987应用、原分区／selector与物理端口；无ROM NVS读写、无全片擦除、Key或任务写命令，COM11不动。部署后保持closed，无无线操作。Gateway／Broker只复用75c／7fc镜像；新245短窗采用完整配置值恢复合同，离线 **10／10**，不SQL／build／变更Auth或AI。旧来源见证过期后不用旧值，只做一次新的匿名来源见证再开本次只读窗。

### 25.2 一次连接与AP快照

- 新串口入口只接显式`status／open／cancel／close`，原字节另存，逐次接收墙钟／单调时钟独立记录；有限输入／计数／白名单回归 **9／9**，不接自动采样或后台定时器。实际一次OPEN、一次CANCEL；串口入口4次STATUS（closed基线＋连接期3次），另有部署后的1次closed核验，**本批共5次状态读取**。未重新打开owner、重放0x3B／WiFi或新增RF扫描。
- 原会话无需退出重登／发码／认领。新原生profile、原recovery、同账号、前台、WiFi-only及权限冷预检全真。原页面点击一次，exact committed POST **200**及OPTIONS **204**各一次；新checkpoint `createdAt=1793885276155` → `dispatchedAt=1793885278270` → `acceptedAt=1793885278383`，仍无WiFi dispatch或Cloud字段。
- OPEN前`esp_wifi_get_mode`返回 **12289（NOT_INIT）**，未探测netif；连接期间3次均为 **mode=2/AP、mode/config返回0、SSID长度20、netifCount=1、AP存在且up、DHCP返回0且状态1/STARTED**。三份SSID摘要均`b57a783b64d48e0946e04d6522224ea2854073c97a8417be78bfa882118e93fe`，与本次真实Native `createDevice`参数中的精确SSID摘要相同。各次getter耗时 **219／211／218 µs**，仅是本样本、不含串口输出，不承诺硬实时或跨设备RTT。
- 系统精确目标选择条件始终未满足，没有点击OEM网络条目。等待末段通过`cmd wifi list-scan-results`**仅读既有缓存**，25行中有本次精确目标1行；未发新扫描。该次结果留在工具回执，未独立留原行及扫描时间，**不据它证明当时已关联／当前持续广播或UI筛选正确**。取消后另一个只读采集的目标0行是事后快照，不覆盖此前1行。
- App终态仍为 **`connect / timeout`**，没有按后续WiFi发送确认；板端取消前仍Provisioning／error0且AP上述状态保持。本批证明所采样时刻AP模式／配置及SSID相符，netif／DHCP已就绪；不据这些getter单独证明RF发射或关联，也不证明Security1、IP绑定、HTTP proto、WiFi配置持久化或Cloud上线。

### 25.3 定位边界：转到Android请求／回调，不猜测修复

- 实际`initialCloudCandidate`分支位于原WiFiProv插件的隔离Local owner内：`connectInitialCloud`使用精确`WifiNetworkSpecifier.setSsid(name)`与WPA2、移除INTERNET后`requestNetwork`，不是SDK会记录口令的`connectWiFiDevice(ssid,password)`重载。SDK2.4.4仍服务后续proto／Security1；不把当前缺口泛化为整个WiFiProv或SDK缺陷。
- 原生源码未见错误SSID、前缀／BSSID／MAC过滤或AP交接内存提前清除的确定缺陷。当前source按本owner callback、epoch及精确SSID筛选，`onUnavailable`失败退出，不选其它AP；不能因手机缓存有目标就放宽这些检查。
- 系统日志同期确有NETWORK_REQUEST界面的`LAUNCH_SINGLE_TOP`／TO_FRONT，并复用上轮同一ActivityRecord及task1006；**UI实例复用不是旧请求身份的证明**，尚未关联本次系统NetworkRequest的SSID／ID及callback序列，不能直接认定旧过滤器残留。应用也不接管外部OEM Activity的`onNewIntent`。
- 父worker IPC在发送connect前起60秒计时，Local owner稍后另起60秒，故现有固定`timeout`不能区分parent IPC与native等待，也不能确认是否／何时收到Android`onUnavailable`或到达后续proto；该回调源码返回`CONNECT_FAILED`，不直接将它列为已确认timeout来源。另见worker-control调用异步`owner.close()`后kill的既有边界：不保证显式unregister先执行；原隔离合同靠Binder死亡释放，**不等于已证明系统请求残留／OEM对话框关闭**。没有根因证据，本批不猜测修改释放、deadline、网络权限或执行链。
- 剩余最小范围：仅补原WiFiProv的可关闭、有界、脱敏请求／callback关联，区分parent发送、worker接收、`requestNetwork`调用前后、同owner的available／capabilities是否精确匹配／unavailable／lost及关闭；关联用非秘密短期trace，禁止记录口令、PoP、Key、raw参数或回包。系统requestId只能从合法只读证据关联，不用反射／ready快照猜造。必要时同一新请求期间只读核具体系统request，而非重复首装／附近或继续改设备AP。**这些原生诊断尚未实施／构建／部署，不冒称已确认Android根因或修复通过。**

### 25.4 收尾及证据位置

- 本次窗口已取消并释放串口，COM3原普通BLE **987f…ac3**恢复并独立校验，分区／selector不变；无NVS清理或配置／Key／任务写入。证明：`build/arf3-245-com3-restore-H8JmM2/proof.json`。串口被动打开不出现panic不等于完整业务或冷启动验收。
- 服务245窗口已关闭，原Gateway／Broker、配置值及代理恢复，其他21个容器不重启，日志保留；仅精确移除本窗两个已卸载临时secret，原短窗不复活、秘密无明文备份。`closed／originalGatewayAndBroker／configurationValuesOriginal／ownedEphemeralSecretsDisposed／noDatabaseQueries`均真；原raw数组指纹差异按预先声明的规范化合同保留，不再误报恢复失败。
- App原位恢复 **31c6…c7d**并核实际SHA，同账号／会话、7卡、原源授权、旧armed、附近引用、目标无Direct凭据及两journal摘要精确保持；亮屏设置恢复。本次accepted记录经普通包下精确原文件只读保留，摘要 **`7e5eecf6c03b329382c967d83f8f4eb5cb7454d0f376129d3ef17cfc74aa8860`**，admission／phase／3时间未变且无WiFi dispatch。§23／§24两份旧accepted的原始摘要另精确核对均未变；不清理、重放或补造UNKNOWN。
- 核心私有证据：`build/arf3-initial-network-runtime-20261105/runtime-native-private-WAt9gB/`内的`compile-report.json`、`audit-refinement.json`、`candidate-operation-audit.json`、`serial-live.private.log`、`serial-host-receive.private.ndjson`及`ap-observation-analysis.json`；手机资料在其`phone-evidence/`。服务证据位于服务仓库`build/initial-network-service-245-20261105/snapshot-ET2Mfx/`。一次事后只读采集工具先后遇到Windows Python stdin编码及ADB小缓冲限制，只修采集器后显式重读；未重开owner、重发写入或把工具STOP算产品失败，事后证据不倒推整个窗口。
- 本批无SDK升级、扩槽、身份／Key发行、任务／scope改动、git提交或推送。**AP观测补证已完成；Android连接超时根因与首次网络／后续两态仍待完成。**

## 26 WiFiProv父IPC／原生网络请求诊断及一次短验（2026-11-05）

### 26.1 最小实现及支持合同

- 复用唯一现有`plugins.WiFiProv.lifecycleDiagnostics`，默认false，不新增环境旗标、purpose或HTTP。只有显式initial-cloud candidate route才将配置经严格Boolean的HELLO传到worker；普通非candidate沿用旧生命周期诊断。业务COMMAND DTO仍拒绝注入此开关。原Specifier、权限、身份、CAS、UNKNOWN、结果、重试及Binder-death隔离语义不变，父／子60秒deadline均未延长。
- 新增独立纯Java `CandidateNetworkTrace`，其余只薄接原Plugin、Client、Channel、Service、LocalOwner及Protocol，共 **7份生产Java**。`WiFiProvLifecycle` WARN固定`traceKind=candidate-network-v1`，短期`ipcOwner`和`ipcRequestId`用于父子关联，**不是Android系统requestId或授权材料**。create／connect之外的写入不纳入此诊断；关闭行关联最后受跟踪请求，不是提交或设备完成回执。
- 固定阶段涵盖父发送／返回／结果／IPC超时、worker接收、`requestNetwork`前／返回／抛错、同owner原guard内处理的available／capabilities／unavailable／lost、原生connect计时器与关闭。capabilities只记录wifi／vpn／internet／info／SSID可读／精确匹配布尔，SSID仅已有name的SHA-256；不输出SSID／BSSID明文、口令、PoP、Key、raw参数／回包／异常。
- 父／子分别最多每请求 **8／12行**、每owner **12／20行**，共享候选生命周期预算，不叠旧重复行。各进程硬上限64，普通／终态／清理分别在56／60／64处收紧；固定去重并预留局部终态／关闭额度，`lost`按是否匹配网络分别去重。**日志仍可能因预算、sink失败或进程结束缺失，预留不等于可靠送达；缺行不能证明回调未发生。** 诊断异常不改变业务结算，不据日志、等待或ready解除围栏。
- 新增及相邻host有限回归 **64／64**：helper6、Channel29、Client21、InitialCloudOwner4、Bridge指定4。覆盖默认关、HELLO严格类型及业务注入拒绝、父／子timer分类、原结果／调用数、sink异常、洪水上限及stale关联。曾有测试命令缺既有依赖／源码路径错误的STOP，仅修运行夹具后通过，不冒称产品失败。Client marker fixture不执行真实Service／Android OS；本次真机另补了Service接收与网络API调用的正证，不据此推广所有故障路径。
- 6份改动前原字节逐SHA对齐§25冻结版本，新增helper及7份after、diff、测试回执独立保留；17份其他冻结native输入未动。两次独立源码审阅未见阻断受限诊断的问题。插件README补齐预算、字段及非证明语义；未清理仍被普通路径使用的旧trace或兼容构造器。

### 26.2 匹配构建与测试入口

- 仅新operation配置换为本批独立值，AP／observer／Sketch正文保持§25字节；原SDK **3.3.11**同路径jobs1唯一完整链接通过，实际只更新 **1个Sketch对象、0 archive**。BIN **`8bb68b7ff8e7b175006598fd835bea5eb439f1f07df867c0067b4b518280270a`**、1,348,784 B；ELF **`96d9e84bfbe21303aa23bcd8ec406eb60e7d8c32e116e9ed6163603a8088dec5`**。原关键制品先独立按摘要留存，未复活已取消owner或重放旧0x3B／WiFi。
- 唯一匹配APK **`a0648da946fe3b9edf9b252176ef49f04122809009958b6d0054224ccedd354d`**。复用已验Web／三份App诊断输入，仅纳入审阅的7份native差异；其余来源、原SDK **2.4.4**、锁文件、包名、签名、HTTPS及原生注册表逐项保持。新profile24／24、类型、Web／Android完整构建及 **1,166份www／1,168份包内资源**核对通过。只在该私有候选生成的Capacitor配置中将原开关置true，普通产品配置没有新增或默认启用。
- root关联profile SHA-256 `ea18edc031064f212f1d5a6e2223b70cb6b3134eb8bc10d7827c9443eeffe9ff`，同原账号／根／两卡／locator，只有新operation及构建expiry变化。COM3只写app0并独立校验，原分区／selector保持，无ROM NVS读写／全片擦除，COM11不动；先closed核验，入口齐备后一次OPEN。Gateway／Broker只复用已验镜像并新建256原intent只读窗，未build／SQL／改Auth或AI；差异工具3／3，未重跑原矩阵。
- 原账号、7卡、旧授权／checkpoint／引用及journal先留基线，匹配App原位安装不清data。冷预检同profile／原记录／账号／前台／权限／WiFi-only全真，无退出重登或验证码。确认WiFi发送的测试mode继续禁止，只在必要时可点原「停止本次操作」，不另建产品取消链。
- 读取器先按**手机自身时间**排旧日志，并以ready的真实主PID及本次新parent-send锚定owner，缓冲乱序行而不借`-T 1`尾行认owner；固定tag／schema／role／event白名单与64行／180秒读取预算，12个离线反例通过。系统快照读取器只做合法只读查询：完整Literal SSID、当前区块、UID／package／WIFI且无INTERNET，再与factory请求及specifier一致才给联合ID；不读MAC鉴权、不反射、无整行dump保存，9个离线反例通过。两项读取器均不授权任何操作，解析不匹配只留空，不能放宽网络匹配。

### 26.3 真机事实与分段耗时

- 真实原页面只点击一次，exact committed POST **200**及OPTIONS **204**各一次；无Key、Bootstrap、WiFi提交、Start、首装或附近复测。新记录 `createdAt=1793892296944` → `dispatchedAt=1793892298965` → `acceptedAt=1793892299042`；accepted不是网络或Cloud确认。
- 捕获同一owner的 **14行**固定诊断，stale／foreign均0、无未锚定行，180秒仅日志读取预算自然到期；不把读取器到期当RPC原因。父主PID与worker PID分别记录，不由CDP requestId推PID。下面全部取自**同一手机的Android uptime**，不混服务器／PC墙钟：

| 分段 | 本次耗时 |
| --- | ---: |
| 父connect发送 → worker接收 | 5 ms |
| worker接收 → `requestNetwork`调用前 | 5 ms |
| `requestNetwork`函数前 → 返回 | 5 ms |
| 父发送 → 首个已记录`parent-timeout` | 60,003 ms |
| `requestNetwork`返回 → `parent-timeout` | 59,988 ms |
| 父超时 → worker记录close-posted | 7 ms |
| 父超时 → 实际Binder death路径 | 42 ms |

- 生命周期pause在父发送后约46ms被记录，它本身不是stop／真正后台或失败原因的证明。`worker-kill`是调用自杀前的事件，不是实际退出；真正释放正证为之后的`parent-death`。本次首先记录到的终态来源明确是**父IPC定时器**，已证明命令到达worker及网络API正常返回，不是卡在IPC发送或该函数体内。
- 连接等待期间一次只读系统快照，**当前Requests表**有真实行 **9250**，精确SSID、UID10050、App package、WIFI及无INTERNET均匹配；**IPC编号3与系统表编号9250不同**，未相互转换。factory投影为空是读取器未匹配到其格式，**不是factory实际为空**；因缺联署证据，`observedUniqueCurrentSystemRequestId`仍null。保留真实表行与联合关联欠项，不能仅凭同一OEM Activity断言旧过滤器残留。
- 本批未记录available／capabilities／unavailable／lost／local-timeout／close-enter／unregister；这些**不等于事件未发生**。回调诊断目前在原owner／epoch guard后的actor上，缺行还可能是入口未达、排队、guard丢弃或日志损失。异步close-posted后很快kill的既有路径也不保证清理行可见，不能把它倒推为最初连接超时根因。WiFi发送按钮未点击、发送mode被禁止，checkpoint无`wifiDispatchedAt`及Cloud字段。

### 26.4 收尾及下一步唯一缺口

- 本批一次CANCEL并释放串口，没有第二次OPEN／重发。COM3原普通BLE **987f…ac3**精确恢复及独立校验，原分区／selector不变；证明 `build/arf3-256-com3-restore-pIoMJc/proof.json`。无NVS清理、Key／任务写入、迁scope或配网凭据提交；不把被动串口无panic代签业务恢复。
- 私有256服务窗已关闭，原Gateway／Broker、配置值及代理恢复，其他21容器不重启，完整日志保留；仅精确删除本窗两份已卸载临时secret，原随机secret不恢复、必要时另建新窗。回执：服务仓库`build/initial-network-service-256-20261105/snapshot-iOqc3S/operator-256-close-1793892533810503500.private.json`；closed／原双镜像／配置值／临时secret清理／无SQL均真，原raw数组次序差异按预先声明合同保留。
- App原位恢复 **31c6…c7d**并核已装SHA，诊断关闭。终核原账号／会话、7卡、源授权、旧armed、附近引用、目标无Direct凭据及任务／自动化journal摘要精确保持，原亮屏设置恢复。本次accepted记录在普通包下经精确原文件只读保留，admission／phase／3时间未变且无WiFi dispatch，摘要 **`ace16c758ce180fa90d4de6e13164b77889d7ad85234fb230f48b4eff6f49c87`**；§23／§24／§25三份旧accepted原始摘要另行精确核对均未变。UNKNOWN、授权及完整请求日志保留。
- 源码及host证据在插件`.build/request-trace-native-20261105/after.json`，SHA-256 **`7c8d950dfbaf479c994c5ab4d24171e5f8ef70e13b7d22eff7d066f868820c03`**；本批私有运行证据位于库`build/arf3-initial-network-runtime-20261105/runtime-android-diag-private-7kDYTV/`，重点为`native-trace-analysis.json`及其`phone-evidence/native-network-trace.private.ndjson`、`system-request-read.private.json`、`native-trace-capture.result.json`。APK冻结位于App`build/arf3-native-network-cloud-beta-20261105/`；无账号凭据／网络口令进入源码或本文。
- **本次诊断实现、有限回归及一次短验完成，连接超时本身尚未修复。** 下一批仅收窄「Android callback进入 → actor执行 → 原guard接受／丢弃」的有界脱敏区分，并在任何新短验前先只读核实际factory输出格式，不以合成格式用例代替现场格式。不要把当前缺行或9250表行当作根因；不改设备AP、Specifier、权限、deadline、身份、CAS、UNKNOWN或释放链，不重复首装／附近／AP采样，不重放旧owner。不新增默认诊断、不自动重试、不升级SDK、不提交推送。
