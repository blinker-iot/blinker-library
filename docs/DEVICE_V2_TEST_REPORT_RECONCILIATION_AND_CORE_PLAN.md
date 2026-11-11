# Device V2：测试问题对照与基础能力渐进完善计划

**现行顺序与归属（2026-11-06）**：取[有限支持交付与核心稳定完善方案](DEVICE_V2_CORE_DELIVERY_AND_STABILITY_EXECUTION_PLAN.md)，从DS-1接入失败显式恢复开始。当前主要开发仍由我们负责，软件范围尚未实际交接；以下旧“当前主线／软件另交”保留为历史，不自动重开已验队列或接续特殊SoftAP深诊断。

更新：2026-11-03。**当前主线仍是核心功能稳定可用，不是继续扩大安全审计或重复验收。QA-3已按§30～32收口**：固定COM11四槽组合执行、受限Arduino包装／安装编译、实际单ID签发及新鲜目录／Read分别完成，结束名单已关、配置恢复。关闭是明确收尾要求，不是功能失败；新包装尚未刷机也不撤销既有运行图的通过结论。

**当前取[独立接入注册方案§13](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#13-ip-3-共享根附近投影数据库及匹配发布)：IP-3单一presence writer及逐卡附近投影有限通过，真实隔离PG 7／7，匹配Gateway／059及060迁移和App一次构建安装已完成。** 手机原生前后台及无自动业务请求有限通过；当前账号本机Admin为0且COM11仍是维护固件，故正向两卡注册／附近仍按§13.5具体条件待补，不以模板或数据库代签。新只读预留与原可安装grant的UNKNOWN分开处理，不换ID、不搬scope、不伪造安装回执；原授权、CAS、任务围栏及UNKNOWN保持，不重扫、清根或推送。

**最新真机执行仍取§77：按新授权仅COM11 app切换，WiFiProv BLE一次扫描／选择／Security1连接／14项目标WiFi及返回取消有限通过。** 首轮因PHY校准变化零写入停，显式协调后实际app仅写1次，最新NVS／分区在启动前精确保持；保留5f9a候选、不恢复原程序，不发身份／WiFi／任务配置。原App、7secure／两份完整journal保持，资源回收已核。Direct功能仍开启，附近正向仍缺原账号凭据／presence；不以本次配网会话代签，不重扫／重刷或重排QA-3／QA-5／AI。

**现装App取IP-3 `45d74e2b…baec3`，Direct／tasks／automation实际为true；Gateway现为`7d29d6e5…395c5b`，schema60及18项运行消费者匹配。** Auth／Broker和其它21个容器未重建重启，正式服务器未操作；§72旧false及§75旧装机仅作历史。跨产品新增沿显式保根合同，旧同ID升级不作为新入口；BLE→Cloud任务区证据门禁仍保持。软件S1～S3另交，不接管设备库／配网／nearby；公共文件先协调。Development不与China／Global混用；保留授权／UNKNOWN，不重放、不发码、不升级SDK。历史失败、自然条件和同事分工继续保留。

## 1. 总体判断与状态口径

清单不是“所有项目在当前版本都仍未解决”：其中混合旧包现象、源码已修、特定组合有限通过、当前环境未准入和新功能。不能一律重新开发，也不能拿一块S3通过将其它平台全部销项。

本次最重要的纠偏：

- BLE／SoftAP已有根重配及空根首装的几条最小链路已有限通过；扫描互斥／选择取消也有修正与证据。保留未覆盖场景，不再整体写“SoftAP／BLE配网不可用”。
- 同目标开／关自动化共存已按匹配Gateway／App及两台物理设备验收，不再重新排一轮配网和自动化闭环。
- 定时已有成功路径及分段时延证据，当前正式Auth内COM11固定组合也已按§30／32通过；**仍不等于公司任意设备或全部Arduino Sketch已获准入**，不再把这个区别混写为当前任务功能未完成。
- 首页实际通道、个人页收到数量和H1／H2本地时区已按§11修正，并由§12在现有自然样本内有限短验；正样本欠证另列，不继续概括为全部未修。
- 同镜像双路配网、纯BLE任务管理、8266持久任务、OTA及热力图属于独立能力，不应混作现有基础链路bug。

统一状态用语：

| 状态 | 含义与关闭条件 |
| --- | --- |
| 源码已修 | 有实际消费者和有限回归；不等于已安装／上线／真机通过 |
| 有限通过 | 有指定版本、设备、模式和环境的证据；只在该范围关闭，不重复已过项 |
| 待差异复测 | 指定场景尚无证据，或公司实际包未匹配；只补这一段，不重跑全链 |
| 缺陷候选 | 当前代码存在可疑路径，先通过真实模板／日志复现，不能先断定现场根因 |
| 准入／组合缺口 | 引擎或组件已有，但当前身份、profile、容量、签发、开关或组合未匹配 |
| 后置扩展 | 当前不支持或另需产品合同，不作为基础接入修复前置 |

“历史通过”与“当前源码／当前安装／当前服务”分开；未复现不等于修复，源码疑点不等于已证实根因。特别是GATT服务失败和S3间歇掉线，当前仍没有可直接归一的根因。

## 2. 基线与支持范围先固定，不重新审计全部历史

默认沿用户要求，以**当前内测执行版本**为复查基线，不等待取得公司旧包才开展当前明确缺陷修正。公司版另设一列“匹配状态”；没有实际APK／提交，不把本机结论写成公司验收完成。

- 当前手机App取§36最终`1dd4ce59…6c524`，沿用原包名／签名／HTTPS／WiFiProv SDK2.4.4。服务schema58及匹配Gateway／Broker、COM11最后已核应用`f8783fbe…eaee`仍沿§32（本批未重新部署服务或新鲜Flash读回），三处任务名单已关。§32 App `10a6a218…fdffba`及更早包仅属历史，不能反复作为当前待部署版本或下一轮核验入口。
- IoT `iot.yiyu.pro` 对接统一正式Auth `api.yiyu.pro`。旧测试Auth／本机scope和任务准入不复制；已登录、套餐权益、设备创建额度、任务准入是四个不同条件。
- 当前公开cloud-beta仍只声明ESP32手动WiFi／TLS／0槽。配网私有候选、逐设备四槽、R4维护板型／Plain内部支持单独记录；Wio正式任务暂缓。
- 配网复用`capacitor-wifiprov`的BLE／SoftAP及现有身份／Admin／allocation／CAS／Cloud只读协调，不另造第二套配网。WiFiProv和BLE Direct不是同一条GATT实现。
- 当前App／插件有必要的未提交源码。依赖闭包、插件先构建及058回退边界取[标准交付计划§11](DEVICE_V2_STANDARD_DELIVERY_AND_RULE_COEXISTENCE_PLAN.md#11-10收口后的最小修正与源码交付索引2026-10-31)，不把旧插件pin／旧dist当成当前完整实现。

## 3. 测试清单对照与精确剩余内容

证据简称：AR＝[接入修复方案](DEVICE_V2_DEVICE_ACCESS_REPAIR_EXECUTION_PLAN.md)；ON＝[全流程入网方案](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md)；STD＝[标准交付与自动化共存](DEVICE_V2_STANDARD_DELIVERY_AND_RULE_COEXISTENCE_PLAN.md)；T＝[任务／Sketch Timer方案](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md)。这里只引用原结果，本轮未复测。

### 3.1 基础配网与联网

| 原清单问题 | 当前核对结论 | 剩余的最小范围 |
| --- | --- | --- |
| BLE_SCAN_BUSY／离页扫描释放 | AR§8／§17已修owner取消、有序释放并有匹配S3有限证据 | 冷重入／特定OEM若重现，只补该场景；不整体恢复“缺完整补丁”表述 |
| 选择设备误报BLE_SCAN_CANCELLED | AR§8／§17已修正常stop／select取消归类 | 真正后台、意外取消和关闭失败仍须可见，不能吞掉所有取消 |
| WiFi扫描未结束仍可发送 | 当前Page的operationBusy／canProvision及provision入口有双重门槛，AR已有列表后下发证据 | 匹配包若仍复现，查调用／按钮绕过与原生接口版本，不先增加固定等待 |
| 冷启动重配上下文丢失 | AR§9已实现新鲜owner GET、no-store、首次账号恢复、代次取消及重配路由意图 | **待指定冷启动差异短验**：不先刷新首页也能进入，取消后不写入／晚回包不污染 |
| 身份安装后断连／重载重复安装 | 正常首装／保根链已过，保根WiFi-only不重复Bootstrap；AR§37有持久checkpoint软件合同 | 新版自然UNKNOWN跨重启、同账号显式只读及清理正样本仍欠证；不从旧allocation造记录，不重装／重发凑样本 |
| Android GATT缓存／S3 provisioning service连接失败 | 当前匹配S3 WiFiProv已有多次成功；历史特定失败根因未闭合。Direct的服务检查不等于WiFiProv同一路径已修 | 新失败才关联连接、服务发现／匹配、Security1会话等实际SDK分支；保留泛化失败无法细分的证据缺口，不无条件refresh／重连 |
| SoftAP连热点后Cloud HTTP0 | Local独立进程与Cloud阶段交接已接线；ON§17／19／26的手动、当前OEM managed保根及手动空根有限通过 | 不外推所有OEM、C3／8266、所有保存AP组合；旧失败保留，不能继续概括当前SoftAP全不可用 |
| 连接前设备热点需手填 | 独立发现入口体验仍未完善；连接后目标WiFi扫描列表存在 | 后续沿原WiFiProv增设备热点选择，不误当“目标WiFi列表丢失”或已证实合并误删 |
| WiFi失败／UNKNOWN结果 | ON§21有明确WiFi失败差异；ON统一结果区分官方回执与Cloud可用 | 超时不误报配置失败，Cloud上线不改写原UNKNOWN；不自动重发。自然正样本欠证不阻塞已过成功链 |
| 公司TLS信任根 | 本机／内部修正与当前TLS联网不证明公司库已合入 | 对照库提交、实际profile、CA指纹及服务证书链；不关闭验证或用公网Plain代替TLS问题 |
| S3间歇掉线 | 原根因仍待定位，短验上线不等于长稳 | 若当前仍重现，做一次有上限的断线窗口，关联reset reason、WiFi、TLS/session、MQTT与服务日志；无新证据不归因／盲改重连 |
| 新版遗漏旧补丁／阶段延迟日志 | 不能作为单个永久开放总bug；已拆为上述具体消费者。阶段日志已有，原生GATT仍可能只给泛化失败 | 每个旧补丁映射到当前入口／测试／安装包；日志有界、可关闭、关联请求且不记录Key、PoP、密码 |

### 3.2 基础控制、状态、位置与历史

| 原清单问题 | 当前核对结论 | 剩余的最小范围 |
| --- | --- | --- |
| 主页BLE附近与实际通道混淆 | **QA-2软件已修，§12已安装**：主状态按activeTransport；现有4卡Cloud／离线及WiFi图标对照通过 | 当前无自然附近正样本，LAN／BLE主通道仅引用§11软件证据，不开功能造样本或重做配网 |
| WiFi／BLE同板身份与任务归属 | 当前接入已按instance／epoch／root及CAS复用；旧双记录通用迁移未证明完成 | 不按MAC／名称自动合并，不跨Auth／fixture搬scope。先列具体重复记录，只有同根权威证据才做显式修复 |
| 设备位置／权限／拖动限额 | [地图方案§13～18](DEVICE_V2_MAP_SERVICE_ADAPTER_IMPLEMENTATION_PLAN.md)已有地图、保存／取消、署名、回收及系统NETWORK取点有限验收 | 独立GNSS、权限拒绝／永久拒绝、跨机型另列；个人地图4500／UTC日和用户限额保持，不以企业预计额度替代实际配置 |
| 在线趋势／云端样本待验 | [H2a方案§8.6](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md)已有在线周期、同值、离线缺口／新段、停用及查询证据 | 当前环境功能开关／策略需匹配；H2b采样UTC、时间质量与设备持久提交ACK是后续能力，不重复H2a首次实现 |
| 状态历史手机本地时间 | **QA-2软件已修，§12已安装短验**：H1自然行本地显示、两页范围／来源、H2本地输入映射及失败手动恢复通过 | H2当前没有自然行；不启归档造样本，不改时间来源或升级H2采样时间质量 |
| power显示“功率” | STD§4已修类型化名称：布尔“电源开关”、数值“功率”，显式标题／别名优先；§11另修派生单位 | 存量用户显式title不猜测迁移。§11仅源码未安装且非已证实可见W缺陷，不重改整套布局 |
| 地图热力图／设备GPS协议 | 热力图后置；“设备GPS协议待定”已过时，[统一输入API](DEVICE_V2_LOCATION_INPUT_API.md)已有`Blinker.location(...)`及WGS84／精度／时间合同 | 用户自行接GPS驱动与解析；真实用户GNSS集成另验，不重造协议／硬件驱动，不把受控测试观测当卫星定位 |
| 活动／运行日志 | [D7c](DEVICE_V2_DEVICE_ACTIVITY_EXECUTION_PLAN.md)是已有权威command ledger的owner只读活动，非完整设备运行日志 | 自然UNKNOWN行、多页UI等欠证留档，不造记录；不为此另搭日志后端 |

### 3.3 定时与自动化

| 原清单问题 | 当前核对结论 | 剩余的最小范围 |
| --- | --- | --- |
| 定时约10秒／忙锁／到期同步 | T§11.31已有App提交至保存确认735.8ms、App提交至Start新鲜确认607.4ms（非设备引擎处理时间）；设备Start进入→接受5.221ms，接受后10062.086ms进入10秒回调，非固定5～10秒下发周期 | 同时区分App确认、设备Start接受、执行回执／新鲜State、列表刷新；旧样本不作公司当前版本性能承诺 |
| 列表刷新／新版无成功结论 | T§11.33热目录单任务2HTTP，手动刷新1791.3ms／保存后刷新2028.7ms；已与确认解耦 | 只对当前新时延问题再测，不降保护间隔／并发重放；旧样本不是严格A/B或p95保证 |
| 公司任务查询403／试点未开 | **§30～32当前COM11固定组合已收口**，实际签发／双根／scope、保存／执行及新鲜目录／Read已通过，结束按要求关闭单ID，原scope／停用Completed任务保持 | 名单关闭是运维状态，不是功能失败。持续开放或公司设备准入须明确指定ID，不再默认重查已过断连／UTC／签发；不套其它产品图或把拒绝403当成功时延 |
| 增改删、启停、倒计时／定点至少3轮 | 这是公司匹配包的额外独立签收要求，本固定组合有§17／27／30／32有限成功证据，未冒称全部3轮已过 | 记录为外部签收欠证，不自动变成当前开发循环。只有明确安排公司该范围签收时才有限执行，不能每次小改都重跑引擎／断电／日期星期 |
| 纯BLE定时管理 | 当前任务本地执行与Cloud管理分开，纯BLE管理未产品验收 | 独立后置，复用原Task协议，不成为现Cloud任务收口前置 |
| 配网固件启用持久定时 | **固定COM11／XIAO ESP32S3／SoftAP／TLS／P256双根四槽已按§24／26／27／30／32有限收口**，原尾窗／分页显式恢复、UTC质量、倒计时及日期／星期已各取证 | 不再将本组合断连／UTC泛写待补；其它ESP32、BLE配网任务、任意用户Sketch属于不同支持范围，不同时铺板或重跑已过链 |
| 同目标开／关规则不能共存 | **STD§10已匹配真机有限收口**：两规则同时enabled，各一次真实ACK／新鲜State，最终停用／退役 | 更新清单而非再跑两设备配网。保留未知结果、目标在途唯一、CAS／fence及已证明互斥的支持边界 |
| 自动化数量／更复杂规则 | 当前每租户4条未退休（含停用）／全库16条，10台设备额度不等于规则额度；§33仅修容量提示及四条说明，源码／模板回归通过，未安装 | 规则额度不变，不重跑共存；未覆盖权限／自然样本另记。扩规则／多动作／复杂条件另开阶段 |

### 3.4 板型、固件组合与发布边界

| 原清单问题 | 当前核对结论 | 后续决定 |
| --- | --- | --- |
| UNO R4配网／持久定时／掉电未验 | T§11.52～58固定四槽Plain、维护板型、有限执行／恢复已有证据；当前正式Auth R4却是普通TLS零槽 | 区分两种产品图，不把内部Plain证据外推为四槽TLS／任意WiFiProv；本批不重开资源审计 |
| ESP32配网Direct依赖已拆 | **不能沿原报告判已合入**：当前`Esp32WiFiBLEProduct.h`仍组合Direct、NimBLE与WiFiProv | 对照公司“只配网候选”与完整WiFiBLE图，先明确是否需独立纯配网产品；不能直接删Direct破坏双模支持 |
| 同固件动态切换BLE／SoftAP | 当前宏编译时互斥，只证明两个候选分别可用 | 后置产品阶段，明确单镜像状态／资源／PoP／切换入口；不拿两个BIN代签 |
| BLE镜像1.90MB超过1.25MB | 属具体候选／分区；当前COM11已验BLE应用约1.37MB、factory5MiB，不是全部S3阻塞 | 逐目标实际BIN／分区核对；不盲写旧app0、不自动改分区／清NVS |
| ESP8266配网与持久定时 | 不在当前ESP32／R4已支持组合内，8266无原生BLE | 单列后续平台阶段，配网合同与存储分别准入；旧云端开关通过不代替配网＋任务支持 |
| ESP8266存储原子性 | 原故障模拟未过／未准入结论保持 | 未通过前不开放持久任务，不跳过掉电合同以赶交付 |
| C3单槽OTA空间／设备固件更新 | 当前特定分区没有双槽能力；Device V2固件签名、安装及回滚全链仍非已交付能力 | 与App更新分开，后置独立确定首发板型／分区／签名／失败恢复，不能夹入配网bug修复 |
| 公司合入与公开发布 | 内部通过不等于公司包包含修复或全模式发行 | 按STD§11补齐三端＋原生插件源码闭包；发行CI保持HOLD，当前不提交推送 |

### 3.5 App、账号、分享与体验

| 原清单问题 | 当前核对结论 | 最小后续 |
| --- | --- | --- |
| “已共享0台设备” | **QA-2软件已修收到数，§12已安装短验**：真实空收到数0，读取失败省略数字，手动恢复0 | 非零／去重正样本仍仅软件证据；发出总数缺完整权威投影，不伪造或N+1查询 |
| 分享撤权／过期 | Cloud双账号读取／控制／撤权已有有限证据，不等于当前全部分享模式和自然到期已过 | 明确当前开关与读／控权限；未覆盖到期／计数差异另验，位置owner-only不旁路，BLE权限不复制owner Admin |
| GitHub登录无法使用 | 当前`auth.service.ts`仍有旧OAuth GET＋storeLegacyAuth路径，缺完整浏览器回跳证据 | 独立核Auth支持、state／PKCE等实际合同及redirect；不是只补一个按钮。先区分未支持／取消／回跳失败 |
| 微信登录无法使用 | 已有原生start→SDK→login及state／截止校验，不等于后台AppID／包名／签名正确 | 后续按平台匹配配置及一次正常授权／取消验收；不在基础接入批中扩第三方登录 |
| 子页冷启动登录保持 | 全局已有账号恢复；旧报告不能直接断定新版丢登录 | 固定已登录＋进程退出＋指定子页深链，核初始化先后／401／authority，不靠放行guard或静默重登 |
| 消息中心／通知推送 | 旧本机Android邀请→通知→详情／拒绝有有限证据，当前beta不因此开放全部消息通知 | 另核当前环境开关，前台／后台／冻结／进程死亡、iOS分别记录；通知不是授权 |
| 手动布局卡顿／配置预览不可见 | 有模板与上下文测试，不等于手机拖动性能通过；控制页由同事负责的范围先协作 | 固定组件数量／手机／手势，先测主线程和布局读写，再局部修正；键盘打开后预览可见，避免重写Layouter |
| 数值键盘／独立下拉 | 步进器已有，mode等选择变体不等于独立下拉产品 | 与控制页负责人对齐，保留空输入≠0、范围／整数step与真实ACK；长枚举选择另验，不新造Command链 |
| WebView99／多机型兼容 | 部分API和布局已有局部修正，不能宣称所有最低版本通过 | 按固定最低WebView及一台代表机先验关键页；iOS独立，不能以全局polyfill／SDK升级代替具体修复 |
| 重复创建／offsetHeight跨页 | 旧修正与当前模板测试不证明全部跨页；同名属性仍存在也不能证明同一bug | 固定路由、草稿、订阅销毁和实际堆栈，先复现再改，不凭全局搜索删除正常测量 |
| 反馈入口过小 | 当前统一menu-list样式最小55px，未证明是旧报告同一入口 | 先核具体入口／截图／命中区域，不无证据重复放大UI |
| 自建服务器 | 当前有维护部署入口，不等于面向用户的自建完整产品已验 | 后置固定HTTPS/WSS/MQTT、Auth、CA及账号隔离合同；不把一条自定义URL当完成 |
| App自动更新 | 当前更新检查主干仍停用，旧公司Web OTA3.0.27不代表本仓完整实现 | Web资源更新、原生APK更新、设备固件OTA分别规划；网页更新不能替换原生插件／设备固件 |

## 4. 渐进实施顺序与每阶段退出条件

基础App控制必须先于定时／自动化，因为后两者依赖同一Command／ACK／State；复杂控制页编辑体验放在后面。因此顺序是“基础接入 → 基础控制／状态 → 定时 → 自动化差异 → App编辑体验”，不是先做完全部界面才验证执行。

| 阶段 | 最小实施范围 | 退出条件／不做什么 |
| --- | --- | --- |
| QA-0 版本与清单校准 | 复用现有manifest记录App/Web／原生插件、库／profile、服务镜像／schema、Auth及设备模式；本批实际核对范围见§9.1 | 匹配表已补，未重核云端／固件现场；不收集秘密、不全Flash备份／全量重编，缺公司包不阻塞当前缺陷复现 |
| QA-1 基础接入差异 | 四项软件差异见§9.2；冷启动读取、同页前后台返回及显式重读有限真机结果见§10，未确认生产缺陷 | 不先刷新首页可取得新鲜owner上下文；同页返回不自动读取，显式重读仅一次。实际新路由实例正常初始化与缓存重入分开，不凑新配网 |
| QA-2 基础控制／状态 | §11三项软件修正／188项有限回归及类型核查；§12一次匹配安装及自然范围内只读短验完成 | 无新生产缺陷；附近／非零收到数／H2自然行正样本欠证留档，不造数据、不重做配网 |
| QA-3 现环境定时可用 | **已收口，见§30～32**：当前固定组合执行、实际P256签发／双根／TaskStore-scope／单ID准入及原App只读短验完成；受限Arduino包装隔离安装／链接通过 | 不再默认Read／Start／日历／UTC／资源重测。名单已按要求关闭；新包装运行仅在明确交付选择时另列，不扩槽、不搬scope或全局开放 |
| QA-4 自动化与分享差异 | **§33最小修正完成，已随§35／36包安装**：两处提示＋3项本地回归，不另触发容量上限或执行闭环 | 已有实现不重写、不造分享过期／非零样本，不重做两设备触发或扩额度；欠证不冒称全部权限真机通过 |
| QA-5 App控制编辑与兼容 | **§46本轮共享控件／组件库有限收口**：§45合并安装及新差异完成，已过范围冻结；软件／原UI／隔离演示／设备ACK分开 | 待自然条件、同事整页交付／特殊适配及证据限制按§46分类；不称整页或全平台完成，无新条件／具体复现不默认追加微修、安装或采样 |
| QA-6 可选产品能力 | GitHub／微信、自建服务、消息推送、三种更新分别独立 | 明确Auth／第三方／签名／平台合同后逐项验收，不用这些后置项阻塞基础接入 |
| QA-7 平台与高级扩展 | 8266、Wio正式任务、R4四槽TLS、同固件双路、纯BLE任务管理、H2b、OTA／Bulk／热力图 | 各自独立支持合同，不跨板型套结论，不因一个内部候选通过立即全局开放 |

QA-1发现当前阻断接入的真实缺陷时优先修复；没有新复现就保留精确欠证，进入QA-2，不无限等待GATT／UNKNOWN自然样本。S3掉线仅在有当前线索时独立设有界观察窗口，不成为所有阶段的无限前置。

## 5. 初始最小合同：QA-0必要核对＋QA-1冷重配差异

以下是初始最小合同；用户已选择本批执行QA-0／QA-1，实际结果及未做范围取§9。**仍不自动开始部署或真机写入**，步骤本身不作为通过证明：

1. 沿用当前内测和现有设备／服务，复用已有版本证据；只记录和本次差异有关的源码／APK／原生WiFiProv对应关系，不重建全部材料。
2. 用现有真实模板及路由模拟已登录但首页未加载的冷启动重配；核原新鲜owner GET／no-store／账号恢复、同instance／epoch／root及CAS，不先执行WiFi写入。
3. 覆盖取消／返回／重入时的单owner与扫描释放、正常选择不误报取消；若现有用例已覆盖且无新缺陷，引用原证据，不照抄测试再计一遍。
4. 只有复现明确缺口才做最小修正、受影响回归；软件通过后再决定是否需要一次匹配App的只读／取消短验。完整配网只在新差异确需写入时进行，不能为补样本重放Bootstrap／WiFi。
5. 没有新增接入阻断后，进入QA-2的三项小修：**首页实际通道 → 分享计数口径 → H1／H2手机本地时间**。各项可独立实现和回退，复用现有服务，不新增一层总协调器。

## 6. 最小修正合同

### 6.1 首页与分享计数

首页主状态取实际activeTransport及该通道的新鲜状态；BLE附近仅表达可发现／可尝试，不等于已认证连接或当前发送通道。Cloud ready＋BLE nearby、纯BLE、LAN、无权限、无新鲜State、后台恢复均进入有限矩阵，不以图标或toast代替ACK。

分享先固定用户语言：“分享出去的设备数”按拥有设备且有active有效成员的设备去重；“收到的设备数”按有效收到授权的设备去重。待认领邀请和成员数量另计，不混入设备数；使用现有权威加载投影和失败态，不加载成功前伪装为确定0，不增加每台设备查询。实际投影不足时保留未知／加载态，再另核最小接口，不从旧缓存猜总数。

### 6.2 历史本地时间

共享最小时间格式／范围转换函数即可；显示与datetime-local输入按手机时区，API／持久化仍UTC epoch毫秒。范围保留`[from,to)`与分页游标，覆盖跨日、时区变化、DST和非法输入；保留服务器确认／接收时间来源，不把显示时区调整伪装为真正采样时间升级。H1／H2读取失败→后台→返回→手动刷新已过合同保持，不引入自动重试。

### 6.3 任务准入与测量

准入先核“当前Auth环境＋逻辑ID＋实际固件图＋签发材料＋scope”完整匹配，既有数据不重新Initialize，不复活退役scope，不重放UNKNOWN。若当前没有可用组合，应明确新增一个限定试点的范围，而不是批量放开所有设备。

每笔操作分别记录：用户提交→管理请求接受→设备确认、设备Start接受→到期回调／执行回执→新鲜State、后台列表读取及UI收尾。确认后立即反馈，刷新失败不撤回确认。刷新仍读新鲜进度，能力目录可按fingerprint缓存，CAS与原保护间隔不以“提速”为由移除。历史735.8／607.4ms是App确认段、1791.3ms是列表刷新段，仅作有限参考，不是设备引擎处理时间、新环境验收数字或固定SLA。

## 7. 证据与实施入口

- 接入：AR§8／9／17／37～39；ON§15／17／19／21／26／30，分别保留成功、失败与未测边界。
- 当前部署及固定ESP32任务：本计划§30／31／32分别为执行、包装安装编译和单ID实际只读结果；不重开该队列。STD§10／11的自动化共存／源码闭包仍有效，版本仅按其历史范围引用。
- 任务历史对照：T§11.30～33分段体验，§11.52～58仅R4固定Plain有限支持；[旧D4](DEVICE_V2_CLOUD_BETA_H2_DEPLOYMENT_PLAN.md)不替代当前正式Auth／新scope，当前最小组合的新证据取本计划§30～32。
- [H2a](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md)、[地图／手机定位](DEVICE_V2_MAP_SERVICE_ADAPTER_IMPLEMENTATION_PLAN.md)、[设备统一定位](DEVICE_V2_DEVICE_LOCATION_IMPLEMENTATION_PLAN.md)、[活动记录](DEVICE_V2_DEVICE_ACTIVITY_EXECUTION_PLAN.md)保持各自来源／权限边界。
- 当前源码疑点入口：[首页卡片](../../blinker-app/src/app/home/components/deviceblock/deviceblock.ts)的`canControl/status/blePrimary`；[个人页](../../blinker-app/src/app/home/components/tab-profile/tab-profile.ts)的`sharedDeviceNum`及[菜单](../../blinker-app/src/app/home/components/tab-profile/profile-menu.config.ts)／中文文案；[H1](../../blinker-app/src/app/sections/device/device-storage/device-storage.page.ts)／[H2](../../blinker-app/src/app/sections/device/device-trends/device-trends.page.ts)的`utcText`和范围模板。
- 板型组合：[ESP32 WiFiBLE产品图](../src/BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h)的编译时互斥及Direct组合；不能沿旧报告直接判定已拆依赖。

## 8. 执行纪律与本批产出

每个问题保留：实际版本／环境／设备模式、预期与复现步骤、当前证据、首个失败阶段、最小修正、有限回归、匹配安装／真机范围和剩余边界。复用原模块，删除代码仅限与本次缺陷直接相关且消费者已确认不再使用的分支，不顺手清理备份或同事修改。

不自动重发配置／任务／控制，不用等待或ready快照解除owner隔离，不发送验证码、不升级SDK、不扩槽；保留账号、授权、TaskStore、scope、CAS、UNKNOWN与完整日志。只有实际新版本或新缺陷才做差异验证，不要求每批重新配网／断电／两设备自动化三轮。

UI审阅技能仅影响“实际状态与用户文案一致、加载／失败不伪装完成、等待反馈与分段结果分开”的验收口径，未重设计页面。首次状态检索无直接匹配，缩小为loading feedback后得到相关等待反馈建议；未把检索当作项目缺陷证据。

初始规划批产出为本对照计划及主线入口更新；当时没有产品源码变更、新回归执行、远端访问、账号／权益调整、App安装、刷板、构建清理或Git提交推送。后续QA-0／QA-1实际执行见下节，不自动启动本表所有阶段。

## 9. QA-0／QA-1：版本核对与冷重配软件差异（2026-10-31）

### 9.1 当前内测与WiFiProv匹配范围

本批先做只读ADB元数据核对，手机PGEM10在线，实际包`iot.diandeng.tech.beta`：

| 层次 | 实际结果与准确边界 |
| --- | --- |
| 当前安装APK | 手机`pm path`精确匹配唯一base.apk，再实际`sha256sum`为`086d66c15137eba3b4e119ebee4a8f087c6fdcbf4c4920ea1dd98a2af8386aad`，与§10内测及现有本地77,893,930 B候选一致；不是只看同名版本号 |
| 包元数据 | versionCode1／versionName1.0-beta、targetSdk36，lastUpdateTime为2026-10-31 01:41:39。实际WebView155.0.8059.43，**本批不是WebView99回归** |
| App选定构建输入 | 既有10-31候选`source-before.private.json`与本批新增测试前的provisioning目录及package／pnpm锁共66项hash比较，0差异。新增测试不改变这些生产逻辑的匹配结论，不宣称整个脏工作树等于旧APK |
| WiFiProv输入 | 冻结的6项TS／Java选定输入与当前hash比较0差异；相邻源码与App安装副本的WiFiProvPlugin、ProvisioningSdkLocalOwner、SoftApWorkerService及AndroidManifest逐字一致。只对这些实际核查项作结论，不冒称全部插件输入均独立冻结 |
| 原生注册／SDK | 当前本地原生注册表包含`capacitor-wifiprov → com.wifiprov.capacitor.WiFiProvPlugin`；候选原Gradle使用相邻原生源码override。既有制品核验记录HTTPS、注册表及1157项Web资源匹配；ESP WiFiProv SDK保持2.4.4，未改SDK／重新构建原生插件 |
| 源码来源 | App HEAD`8b19ac011b3acf0ec9070666d1b6ec255c30a54c`／i3w-dev-v2、插件HEAD`1240a0771b746022c06825684fbe75f15c38d783`／main；必要dirty／未跟踪源码仍存在，HEAD不是完整发行闭包，未暂存或切分支 |
| 云端／固件 | 沿STD§10／11原证据边界，本批没有SSH、HTTP业务探测、串口或Flash操作，**没有新鲜重核**Gateway／Broker／COM11实际运行状态，也没有借软件结果声明它们重新通过 |

原生注册和APK来源复用App忽略目录`build/automation-coexistence-live-20261031`中的既有制品核验／输入记录；手机APK摘要及包元数据是本批新读。当前类型入口有一项工具链注意：插件源码为新接口，但相邻及App安装副本的`dist/esm/definitions.d.ts`仍旧。**这不等于已装APK原生实现过旧**；本次Angular队列继续使用既有`build/access-ar4g-20261030/tsconfig.spec.json`映射相邻`src/index.ts`，不升级依赖或修改锁。未来全新构建仍须按README先显式构建五插件，不能把source alias当发行安装步骤。

### 9.2 新增的四项差异，不重跑已有矩阵

既有reconfigure／lifecycle用例已涵盖冷目录、初次账号恢复、取消／切号／authority、15秒截止、CAS／UNKNOWN保留及晚回包等。本批不把这些旧结果重新累计，只补下列交叉组合：

| 新用例 | 实际入口与断言 |
| --- | --- |
| 初次账号恢复与真实模板 | 新`ui6-reconfigure.render.spec.ts`使用原生产模板及真实ngOnInit，不mock初始化、不手工填phase：等待账号→唯一新鲜详情读；原扫描按钮读取期间disabled／main aria-busy，完成后自然恢复；没有首页目录刷新或身份／WiFi写入 |
| 取消旧上下文后缓存重入 | 真实leave取消原signal，晚旧响应不解禁；enter不自动GET；点击原模板“重新开始”按钮才再读恰一次，完成后按钮自然恢复。无Key创建／reveal／rotate、connect／custom／provision |
| 旧权限跨重入晚返回 | 在原lifecycle.spec补权限pending→leave→新代enter→旧权限回包，不能取得扫描lease或启动旧扫描；只有新显式操作才可启动 |
| 原生关闭pending时立即重入 | 原关闭未证实时保持围栏／不放lease／不重新初始化；旧连接及扫描listener前后均不能污染新代。只在close Promise明确结算后允许新显式扫描，不以等待时长或ready快照放行 |

UI技能仅用于等待反馈、原按钮disabled及恢复的真实DOM断言；检索`disabled loading feedback`匹配相关反馈规则。未改布局、组件设计、全局变更检测或原owner生命周期，也不将模板测试称为手机视觉验收。

### 9.3 分批执行结果及原失败保留

- 唯一初始Angular队列选定4项新增差异：**3通过、1失败**；原生命周期11项按名称未选择，不算本批通过数。失败出现在第二模板用例的首次GET调用数断言（0次），尚未进入leave／reentry被测段。
- 仅给新模板harness补已有的有界微任务排空，再等待fixture稳定；没有改生产代码、强制改DOM／phase或用固定延时解除隔离。该失败归为测试尚未到达异步前置的观察点，**未证明更深层精确根因，不计产品缺陷修复**。
- 随后只选这一个未通过模板用例，**1／1通过**，原3项未重复。四个不同新增用例至此分批通过，不能写成单次4／4全绿。
- 两次队列都沿Node22.22.3、原Angular／Vitest及源码alias；日志分别为忽略目录`build/qa01-20261031/four-difference-tests.log`和`reentry-template-test.log`。Angular构建／spec编译通过；本批没有另开生产全量typecheck／APK构建队列。
- 两个测试文件的差异／新文件行尾检查通过。生产App、插件／SDK、库、服务实现及原锁无本批改动；原本地无关修改保留。

### 9.4 收口层级与唯一后续

**QA-0本批指定的安装版本／选定插件输入已核；QA-1新增软件差异完成，未确认新的生产缺陷。** 不为得到“修复”记录修改已通过执行链，也不重建／安装相同APK或重复配网／自动化。当前手机仅执行包元数据与摘要读取，未导航、冷重启／停止App、清数据／清日志或读写账号凭据；本批工具未主动触发业务HTTP、扫描连接、身份安装或WiFi下发，原授权、UNKNOWN、TaskStore／scope及完整请求日志不动。

“手机进程冷启动→原重配路由→取消／缓存重入”的整段真机操作**本批NOT_RUN**，不能由上述模板或旧普通重配代签。后续若需要实物补证，仅复用当前匹配App与同一设备做只读上下文／取消短验，不发码／刷板／连接或下发配置；无必要差异不先重复构建。该欠证与自然UNKNOWN／GATT根因独立，不延长成新配网矩阵。

按§4退出规则，当前未发现新增接入阻断，可将**QA-2：首页实际通道、分享数量口径、H1／H2本地时间**作为下一批最小实现；需要时将上述只读短验放在一次必要匹配安装中独立记录。当前不自动执行QA-2、不提交推送、不升级SDK或扩大准入。

## 10. QA-1：冷启动与同页返回的只读真机短验（2026-10-31）

### 10.1 沿原版本保全，不构建或重配

用户选择§9.4只读真机差异后，复用原PGEM10／`iot.diandeng.tech.beta`及原COM11逻辑设备，不连接开发板、不重置网络或读取Flash。实际APK SHA仍为`086d66c15137eba3b4e119ebee4a8f087c6fdcbf4c4920ea1dd98a2af8386aad`，未安装或改运行代码。

先通过原SecureStorage只读接口和IndexedDB readonly事务保留当前账号、全部7项secure及2个完整journal store（含原UNKNOWN）；logcat不清空，原始正文仅存ACL私有目录。读取旧logcat时首次碰到工具默认1MiB输出缓冲，停在冷启动前；原账号快照已保存，增大工具读取缓冲后再次只读保全并逐字核对一致。该工具首停不计产品故障、不清日志、不覆盖原证据。

本地核access token剩余有效期足够完成短验；本次owner详情读取**不需要5分钟敏感Key近期认证**，没有登录、发码、refresh／reveal／rotate。沿原账号和原逻辑ID读取，不创建新身份／scope或修改权益。

### 10.2 一次进程冷启动及新鲜owner上下文

保全后对App进程执行一次force-stop，实际确认PID为空，再正常启动原MainActivity；PID由25912变为16973，非仅WebView reload。CDP连接后从`/`直接导航原路由：`/tools/esp32-provision?mode=reconfigure&logicalDeviceId=<原COM11逻辑ID>`，不点击刷新首页，不调用产品私有方法或伪造上下文。

首次原owner详情GET返回**HTTP200／private,no-store／非磁盘缓存及非ServiceWorker**。完整正文中logicalDeviceId精确匹配、state=active、cloudEnabled=true、credentialVersion和locator合法；原页面标题“设备重新配网”、aria-busy=false、原扫描按钮正常恢复。请求关联`89bccef4d0e64aa3e9491079eeec5fca`；这只是owner上下文读取，不是新鲜物理instance／root读取或设备在线／控制ACK证明。

App冷启动自然读取账号／目录等仍存在，不能声称整个App启动零目录请求；配网页没有被工具调用首页刷新，也没有Key或身份写。CDP是在启动后连接，**此前启动早段没有完整wire覆盖**；logcat和原journals保留，不补造缺失网络事件。下文只对实际捕获的请求作正文完整性结论。

### 10.3 首个路由观察停点：新页面不是缓存恢复

首段采用真实路由跳转离开配网页再进入同URL。原页面host已销毁，重新进入生成新host／ngOnInit，并产生第二次正常详情GET200／no-store，关联`4b2903a88ee151387e857436aeabd17d`。工具要求sameHost的断言没有成立，于是立即保留首停并回首页，**没有点击重新开始、重试原请求或将第二GET删去**。

只读复核源码后明确：当前idle配网页只有原返回按钮，没有面向用户的前推其它页面入口；正常back会pop销毁当前页，重新打开需要新鲜读取。此次工具通过pushState／popstate导航同样没有取得保留host，不能把合法新实例初始化判为“缓存页自动重试”，也不能通过改history navigationId、调用私有NavController或制造隐藏页面来取得通过。

因此本段证明冷进入及离页清理／新实例可读，**不关闭Ionic路由缓存ionViewWillEnter正向真机项**。该交叉组合仍由§9软件证据覆盖，真机NOT_RUN；不为这个当前idle UI没有可达forward入口的组合再扩一轮导航或配网。

### 10.4 最小续段：同进程／同host的真实前后台返回

不再冷启动，只显式进入原同URL一次作为续段setup，取得一次GET200／no-store（关联`cec70e020ceb4bb10619c8af29ffc21e`）。此后完全沿真实App生命周期和原按钮操作：

| 步骤 | 实际证据 |
| --- | --- |
| HOME后台 | Android原生焦点确实位于App外，后台1523ms；不在挂起WebView执行JS，不把这段观察时长当owner死亡证明 |
| 正常返回原Activity | PID保持16973，DOM host与离开前同一个；原页面显示暂停／重新开始，扫描按钮禁用；不是重新创建页面 |
| 有界不自动读取 | 从返回启动到终核1682ms内，owner详情读取数没有从setup的1增加；没有工具补查或点击；仅代表本次窗口，不外推所有OEM／任意长时间 |
| 显式重新开始 | 只点击原`.status-card > .retry-button`一次，没有调用私有Page方法；随后恰新增一次原详情GET，HTTP200／no-store（关联`2d0ab819287fadb3f36bac7f505ec656`），等待态解除／扫描按钮恢复 |
| 结束 | 正常离页回`/home/device`，原stopBleScan／clearState Promise均明确成功；不以页面ready替代清理证明，未新增扫描、连接或WiFi写入 |

两段合计实际owner详情GET为**4次**：冷启动1＋新路由实例1＋续段setup1＋显式restart1，不能压缩写成全程仅2次。离线终审独立核四个完整响应均200／no-store／非缓存，精确ID、active、cloudEnabled、credentialVersion和locator组成的上下文摘要完全相同；owner/context均来自原接口，不是工具注入。续段目标中的“同页返回0读、显式一次操作1读”成立。

### 10.5 保留、回收与收口决定

- 多个前后快照中，全部**7项secure逐字一致、2个journal store全部key／row逐字一致**，原账号、授权、UNKNOWN及请求意图未被清理或覆盖；没有导回旧session。
- 实际捕获13个业务HTTP（首段11、续段2）均GET，响应正文全部留存；OPTIONS／204预检不误报缺正文。新日志只在ACL私有目录，不进入Git。不能据此宣称CDP接入前的所有启动网络也完整捕获。
- WiFiProv守卫只允许原getProvisioningNetworkState／getCapabilities等读取、监听及stopBleScan／clearState清理；实际连接／扫描启动／custom／Bootstrap／WiFi写入均0，HTTP写入0，守卫拦截0。两段各4个stop／clear调用均明确结算成功；监听callback的settled标记不是关闭证明，不混算为失败。
- 原guard均恢复、Fetch观察关闭、CDP转发回收，App终态在首页；保留原APK／SDK、设备固件、TaskStore、云scope和服务配置。未构建、部署、安装、刷板、重跑配网／自动化、发送验证码或提交推送；原无关本地修改不动。
- UI技能仅用于真实DOM等待／禁用／恢复及原按钮确认，本批未改布局或执行手机视觉矩阵。原生焦点、同进程／同host与HTTP正文才是本批运行证据，不将它们混成GATT连接或配网成功测试。

本批工具、原首停、全部前后私有快照、两段HTTP／native／logcat、原生焦点与独立审计留存在App忽略目录`build/qa1-readonly-live-20261031`的ACL私有子目录。`final-audit.json`分别列出四次读取、同页返回0读、显式一次重读、清理及保留结果；首段“cached-return”期限失败原样保留，不改写成产品失败或单次全链通过。

**有限收口：QA-1冷启动owner读取、真实同页前后台返回不自动读、显式重读一次及资源清理已验，未发现需修生产代码的新缺陷。** Ionic路由缓存交叉真机NOT_RUN、在途HTTP取消／晚回包仍仅有软件证据；不为了缺样本重做此链。下一批可按原顺序进入QA-2三项小修，不再停在配网／UNKNOWN循环；本批不自动开展QA-2。

## 11. QA-2：主通道、收到分享计数与本地时间的软件收口（2026-10-31）

### 11.1 本批最小范围与先复现

用户已选择QA-2，限定App显示与输入投影，不操作账号、设备或服务。先沿原Node22.22.3／Angular／Vitest、原锁与WiFiProv源码alias，以外部`TZ=Asia/Shanghai`只选新增或修正合同用例建立红测；没有先改生产逻辑。

首队列15项失败：其中14项直接暴露主通道被附近覆盖、BLE已连但无新鲜State误ready、分享口径／未知误0、本地时间显示／输入不匹配等目标合同问题，**不是14个不同根因**。另1项新测试误要求普通读取失败时内部`writeBlocked=true`，原合同实际靠detail失效／canSave=false禁写；只纠正该测试断言，未改生产保护去迎合用例。原红日志完整保留。

### 11.2 首页：实际通道与附近状态分开

只修改`deviceblock.ts/.html/.scss`及相关两个spec：

- 主状态、图标及文字按`activeTransport`，Cloud／LAN不会被BLE权限、nearby或ready覆盖。没有改变端口选择、订阅、鉴权或`canControl`。
- 实际BLE已连接而State不新鲜时，仅显示“蓝牙已连接，数据待同步”／reachable，不冒充绿色ready；新鲜State后才恢复原ready语义。
- “附近”是独立小标识，只在当前主通道非BLE、bleAccess=true、适配器未关闭、bleNearby=true时出现；不由ready推断nearby。保留可见文字和蓝牙图标，完整可访问名称说明“非当前控制通道”，不添加竞争live region或只靠颜色／hover解释。
- 保留原header和24px主状态区，只增加小范围nowrap／语义色样式，不重排控制卡、修改控制见证或引入额外扫描。

真实模板回归覆盖Cloud＋nearby scanning／ready、LAN＋nearby、BLE陈旧／射频关闭、权限／附近证据丢失、前后台后新投影及原控制／编辑事件保护。模板是软件证据，不是375px／横屏／大字体或真机视觉矩阵通过。

### 11.3 分享：修正为收到数，不猜发出总数

当前只有收到列表具备现有完整投影；`share.byDevice`是管理页按设备懒加载的局部结果，失败可保留旧值，不能据此计算全局发出数或拿缺项当0。本批因此不新增总数API或逐设备N+1查询：

- 个人页和11种语言统一为`RECEIVED_DEVICE_COUNT`／“收到X台设备”，不再用received标成“已共享”。管理页原逐设备成员／分享入口不变，**全局发出设备数仍未实现完整统计**。
- 新窄纯函数`profileReceivedDeviceCount`只对可用received投影的active合法逻辑ID去重，排除revoked；非法active ID／不合法投影返回unknown，不静默减成0。不猜grant到期时间，也不将待认领邀请或成员条数计为设备数。
- 账号未恢复／退出、当前完整inventory未就绪，或user／device／config读取失败时返回null，由原菜单省略数字；只有已确认空received数组才显示0。使用原加载／错误标记，不加新的状态层或网络读取。
- 这是**最近完整权威投影的收到数**，不是实时全局计数。`share-manager`自己的局部读取失败没有发布共有失败标记，本批不改它或宣称覆盖所有其它页面的失败；保守的config错误门槛也可能因区域失败隐藏计数。

有限回归覆盖active／revoked、重复ID、合法空数组、非法ID、未恢复／退出／加载失败、菜单导航和原套餐模板；11个JSON语言文件均验证新key及`{{count}}`占位、旧key不存在。没改套餐、额度或服务接口。

### 11.4 H1／H2：手机本地显示与输入，UTC执行合同不变

复用现有`history-window.ts`，增加共享本地文本，修改两页最薄投影／模板，未新增时区服务、依赖或SDK：

- 样本及范围文本显示手机本地时间并附该瞬间的UTC偏移；H1仍为服务器确认时间、H2仍为服务器接收时间，**不是设备采样／实际发生时间**。`<time datetime>`继续使用UTC ISO，HTTP和持久化仍为UTC epoch毫秒。
- H2 `datetime-local`结束时间按本地解析，保留秒精度、固定`[from,to)`、宽度、游标及200条边界；刷新读已应用UTC窗口，只有显式应用才改窗口。
- 严格roundtrip拒绝非法日历及DST跳空；最多采三个相邻日偏移识别重复本地时刻，不扫描整天、不假定转换总为1小时。新歧义输入拒绝；未改动的已应用输入保留其原UTC映射，避免重复小时的第二次01:30静默变成第一次。
- H2只为未编辑的已应用输入保存格式stamp；手机时区变化后可重投影输入，不改UTC窗口／游标或用户草稿。读取失败返回前台只投影、不重试；正常成功返回及UNKNOWN原只读恢复合同保持，未改CAS、写入围栏或保存执行链。

真实模板验证本地文本／原时间输入及未应用前的UTC范围不变；上海跨日、纽约跳空／重复小时及已应用第二fold、Lord Howe半小时变化、UTC零偏移由**不同外部TZ进程的独立子集**核对。页内`getTimezoneOffset`窄mock仅验证投影变化，不冒充手机系统设置或真实DST；没有在测试worker内修改TZ伪造结果。

### 11.5 有限回归、原失败与独立审阅

所有队列严格串行，不重复通过项，原始日志在App忽略目录`build/qa2-software-20261031`：

| 队列 | 实际结果与说明 |
| --- | --- |
| `red-main`／上海 | 15失败，先复现；14项目标合同不符＋1项新测试内部状态期待过严，均保留原结果 |
| `green-main`／上海 | 178通过、5失败，另5个异时区用例过滤未选；失败为4个旧P3名称期待及1个datetime-local零秒规范化期待，不回退生产逻辑 |
| `green-five-differences`／上海 | 只复测上述5项，5通过；名称按既有P3的温度／功率／电源开关合同对齐，int64精确值、不匹配别名、错误提示及控制保护断言保留；零秒接受浏览器合法省略`:00`，UTC查询断言不变 |
| `green-new-york` | 3通过：helper的DST／已应用fold，以及H2重复小时不漂移；原成功用例未重跑 |
| `green-lord-howe` | 1通过：30分钟跳空／重复时刻 |
| `green-utc` | 1通过：手机零偏移的原UTC瞬间 |

合计**188个不同用例分批通过**（178＋5＋3＋1＋1），不是单次全绿或累计红绿重复执行数。过滤未选不算通过；本批不重复原配网、任务／自动化、BLE或断电矩阵。另一次原source-alias配置`tsc --noEmit`通过，11种语言JSON检查、必要文件差异／行尾检查通过。

独立交叉审阅没有发现必须新增修正：控制／权限链未变，未知投影不伪装0，UTC、DST映射、草稿、CAS及失败后不自动重试均保持。仅补测试中此前旧名称期待，不把这些旧期待算成QA-2引入的产品回归。

### 11.6 当前交付状态与下一步

**§11软件批结束时三项最小修正完成，尚未安装或真机验证；后续实际安装与短验结果已在§12补齐，以§12为准。** §11仅改App必要源码、回归、11个翻译条目及方案索引；原DataService／Gateway／Broker／设备库运行代码和协议不变。当时未访问手机或云业务接口、未动账号／授权／UNKNOWN／TaskStore／scope／完整请求日志，未构建Web／原生候选、部署、发码、升级SDK、提交或推送；无关本地修改保留。

后续若选择匹配App安装，只短验首页主通道／独立附近、收到数已知／未知、H1／H2本地时间及原范围查询／失败手动恢复。沿现有服务和设备，不为显示造分享／历史样本、改手机系统时区或重复配网；UI视觉与原生实际交互单独记录，不将本批模板或外部TZ测试代签。

之后再按QA-3明确**当前正式Auth内一个ESP32配网＋四槽组合的准入前置**，不搬旧scope／重放UNKNOWN、不扩槽。QA-1不可达Ionic缓存路由与自然样本欠证继续保留，不因本批三项小修重开配网循环；本批不自动部署或启动QA-3。

## 12. QA-2：匹配内测安装及只读真机有限收口（2026-10-31）

### 12.1 保留与唯一构建队列

用户选择§11.6的最短差异，只更新App，沿用现有云服务及设备；不配网、控制、刷板、改分享／历史策略或任务准入。

- 安装前保留当前同账号、全部7项secure记录、2个完整IndexedDB journal store及完整logcat；UNKNOWN所在原记录不筛选、不删除。再次读取逐字核对，保留私有ACL目录。仅复用与手机旧APK精确同SHA的本地包作覆盖安装回退材料，不重新备份设备或服务。
- 一条原工具队列串行完成prebuild、Web、cloud-beta同步及Android打包，分别36.889／39.333／13.031／181.461秒；均退出0，源码／原生消费者／HEAD／Git索引无漂移，未发生行尾恢复。未重装依赖、改变锁文件或另开重复队列。
- 沿用Node22.22.3、esbuild0.28.2、原WiFiProv Android SDK2.4.4及原Java／Gradle。现有工作树插件dist较旧，复用已验TS source alias与原生source.gradle指向相邻插件源码，不将旧dist误当本次消费者。
- 新APK SHA256为`7e88a39038dc5381ef713b7915ddb5df167cda0ab4fccbb36ae591b61b14a0f0`；包名`iot.diandeng.tech.beta`、原签名、HTTPS、无明文例外／serverURL、原生WiFiProv注册及诊断关闭均核对。1157份打包Web资源与本次同步输出逐字匹配；独立核包内首页独立附近标识、H1／H2本地时间来源及11种语言新收到数key／占位，旧key均不存在。
- 仅一次`adb install -r`，没有卸载／清数据。安装后7项secure、2份完整journal与安装前逐字相同；运行结束再次相同，不以“同账号”代替完整授权／UNKNOWN保护。

### 12.2 首页及收到分享计数

手机原时区实际为`America/Los_Angeles`，未改系统时区。公开beta的Direct仍关闭；不临时开放附近扫描或LAN。

| 短验 | 实际证据与边界 |
| --- | --- |
| 首页实际通道 | 当前4卡按原`data-id`与本轮原账号刷新取得的权威inventory精确关联：2台Cloud可达、2台离线，均为WiFi图标。首次画面两台显示“云端已同步”；后续自然State变陈旧时可显示“云端在线”，不冒充新鲜ready，未修改通道选择或控制链 |
| 蓝牙附近 | 4卡均无附近标识，原生guard记录调用0／阻止0，没有为测试扫描。独立nearby正向、BLE／LAN主通道真机NOT_RUN，保留§11模板证据，不用本批Cloud画面代签 |
| 已确认收到数 | 原个人页账号刷新取得真实空received列表，菜单显示“收到0台设备”；不把拥有4台或分享成员数量误算为收到数 |
| 未知及显式恢复 | 仅将下一笔received GET在本机CDP层失败一次，原config失败后数字隐藏，不显示假0；再次点击原账号刷新才恢复真实0。没造HTTP响应、分享记录或新读取接口；本场景无独立全局错误notice是原产品合同，不为测试强加notice |

额外首页离线审阅工具最初误要求`cloudReachable=true`必为“云端已同步”，在自然“云端在线”时断言停下；这是工具期待过严，不是产品错误。已保留首停，改为区分Cloud在线与新鲜State的既有合同后，仅再次读取当前首页DOM并对照已保留inventory，**未重发网络请求或重跑首页业务链**。

### 12.3 H1／H2本地时间、范围及失败手动恢复

使用现有COM11所有者逻辑设备，不产生State、启归档、改策略、Start任务或发送配置：

| 项目 | H1状态历史 | H2在线趋势 |
| --- | --- | --- |
| 自然样本 | 初始24条，机器`datetime`均保留UTC ISO `Z`；逐行本地文字及`UTC-07:00`与同一UTC瞬间一致 | 当前记录未启用、返回0条；空态／来源通过，样本行及曲线本地显示正向真机NOT_RUN |
| 时间来源 | 明确“服务器确认时间 · 手机本地时间”，不是设备发生时间 | 明确“服务器接收时间 · 手机本地时间”，不是设备采样时间 |
| 范围 | 原选择框24小时→1小时，实际GET宽度3600000ms，保持`[from,to)`；恢复后自然8行 | 原本地结束输入减1小时，输入草稿不发查询；点击原应用按钮一次后实际UTC `to=1793446696000`，24小时宽度不变。本地`04:38:16`映射正确，不手工直接fetch代替页面 |
| 受控读取失败 | 下一笔history GET仅本机失败一次，原刷新按钮可用，错误可见 | 下一笔history GET仅本机失败一次；同次策略读取由原并行取消链`ERR_ABORTED`，不是新增故障注入或服务错误 |
| 真实后台／返回 | HOME后原Activity返回，错误保留、刷新按钮可用；约1.45秒后台及随后1.3秒观察内目标自动GET增加0 | 同样通过；约1.40秒后台及随后1.3秒观察内history／policy自动GET均增加0 |
| 原按钮手动恢复 | 点击1次→history GET1成功；1小时宽度不变，滚动`to`重新锚定当前时间是原合同 | 点击1次→policy／history各GET1成功；失败前后已应用`from/to`逐字一致，不漂移窗口 |

请求／响应和DOM终态双重关联后才判通过，不用旧画面或固定等待当完成。原生焦点证明真实离开App；没有将普通路由重建冒充Ionic缓存同实例，也不覆盖QA-1原未测项。

### 12.4 终核、保留边界及下一步

本次完整wire共24笔业务GET：20份完整200正文、3次限定本机读取失败（received／H1／H2各一次），另1次上述H2 policy联动取消；无正文留存缺失。H2 policy合计4读为3成功＋1取消，手动恢复policy与history各一次成功；不把取消算作上游失败或第4次故障注入。HTTP写入0、WiFiProv调用0；无发码／登录／token刷新、配网、控制、任务／策略写入或样本生成。

全部账号／授权／UNKNOWN／完整journal保持；服务、固件、TaskStore和scope未操作。原页面guard／CDP观察器／9479转发及logcat采集进程均已回收，App停首页。当前截图已实际查看：首页、个人页和两页首屏纵向布局；没有声称H1屏外记录已截图、所有尺寸／横屏／大字体／旧WebView矩阵均通过。UI审阅只核原标签、来源、输入和手动恢复，没有新增页面设计。

工具、唯一构建日志／APK核验、安装前后私有快照、完整网络正文／logcat、截图、原始额外工具首停及独立审阅位于App忽略目录`build/qa2-live-20261031`的ACL私有子目录，不提交账号、网络头、日志或构建产物。无生产源码新增修改，不重跑§11已过188项软件回归、不升级SDK、提交或推送；保留所有无关本地修改。

**QA-2在本批自然可观察范围内有限收口，未发现需新增生产修复的缺陷。** 欠证仅保留：附近正向／BLE或LAN主通道、非零收到数及去重、H2自然样本行／曲线；不为此造数据或重开配网。下一批按QA-3先核当前正式Auth内一个ESP32“配网＋四槽”实际组合、签发材料及逐ID准入，再决定最小任务短验；不搬旧scope、不重放UNKNOWN，本批不自动启动QA-3。

## 13. QA-3：正式环境COM11组合前置与双根准入源码（2026-10-31）

### 13.1 新鲜事实，不把旧D4当当前组合

本批用户选择QA-3。只读核当前手机、正式云容器／数据库与本地实际消费者，沿用§12内测APK，不构建／安装App，不操作开发板：

| 核对项 | 本批事实 |
| --- | --- |
| App及账号 | 已装beta仍`7e88a390…14a0f0`；同账号的inventory／COM11详情各GET1、均200。本批前后既有7项secure与2份完整journal新鲜保留，逐字一致；不reveal Key、登录、发码或调用task execute |
| 正式环境 | 四容器健康；Gateway `8b353f33…f5699a`、Broker `c6500784…bb2bc8`、schema058，API及内部Auth均实际指向`https://api.yiyu.pro`。本批未重建／部署服务 |
| COM11权威身份 | `device_b60437fc-dfe2-4745-91cc-57efbf6bf024`，同当前owner、active diy、DeviceKey版本1有效，云会话仍同`215af06b…9a5e40c` instance，Broker MQTT在线；不是新建设备或旧测试Auth身份 |
| 已安装Admin事实 | Direct表实际有epoch1／Admin版本1及非空controller／fingerprint，与Task根必须共同绑定；不删除该行／设备授权来冒充手动WiFi |
| 两端逐ID准入 | Gateway任务管理名单0、Ed25519名单0，Broker任务名单0，COM11均不在名单中；设备10台创建额度不代表任务准入 |
| 实际签发 | 运行Gateway未配置任何BLE／Task签发材料，签发ring为空、Task signer未构造；因此当前没有可核对的实际algorithm／keyId／公钥，不能写成“P256已匹配”。代码P256优先规则不是当前已启用签发证据 |
| 当前云Task数据 | 仅该ID查询：scope0、request0、attempt0；没有本批可恢复／重放的任务请求。不将这些云端0推断为设备NVS TaskStore空 |
| 设备组合证据边界 | 最近已验恢复源为原Core3.3.11、SoftAP零槽`c05195d7…36b96eff`，Manifest为`power`及`uptimeSeconds`；本批没有ROM读回／读取运行Hello，故不把恢复记录／Manifest当新鲜固件SHA或协商容量证据。当前NVS TaskStore仍须后续只读核对 |

旧候选静态P256 keyId1、SPKI SHA256 `57f69d14769fd9a099c1d4cc11fac64f8389ee975200f9fa65f53e18b58a6025`仅作历史材料索引；当前ring为空，**不是已完成公钥匹配**。不读取／输出DeviceKey、私钥、token或完整环境。

### 13.2 已明确的最小缺口与支持合同

当前“配网”和“手动WiFi四槽”分别已有证据，但没有可直接部署的组合；不是再开一次名单或复跑配网即可收口：

1. `BlinkerFacade.h`明确拒绝WiFiBLE＋Tasks，`Esp32WiFiBLEProduct`未挂任务扩展／carrier，`TaskSupport`只接手动WiFi；原D4发布guard也明确排除配网。保持原公开0槽和D4手动图，不放宽原guard伪装组合。
2. Gateway原任务Repository主动拒绝所有已登记Direct Admin身份，`withRoot`固定`admin:null`；必须新建**逐ID显式受限profile**，原手动图仍拒混根。配网身份需要DeviceKey＋实际Admin共同根，缺任意根不降级，不迁移旧scope。
3. 设备协议已支持共同根，但现有`WifiTaskExtension`只填DeviceKey；任务环境读的是缓存，不会自然察觉Admin变化。必须接实际Admin selector及根变化前invalidate／主owner变更后refresh，不能每poll读Flash或在WiFiProv原生回调里刷新TaskAdapter。
4. 实际签发材料／逐ID名单目前都缺，不能在设备图、同根与现有Store状态未核对前开放；Broker bit25／frame预算／唯一连接等原门槛保持。

**固定最小目标：COM11 XIAO ESP32S3＋原SoftAP配网身份＋四槽Cloud TLS任务，P256。** 先复用最近原身份与原bearer，不重复BLE首装／SoftAP完整配网；本阶段不处理同固件动态双路、纯BLE任务、R4／Wio、扩槽或SDK升级。任务引擎、Codec、TaskStore、管理请求／回执、CAS、UNKNOWN协调均沿原实现。

### 13.3 本批实施边界：先补Gateway显式共同根，运行保持关闭

本批只实施可独立回归的云侧源码前置，不将其称作COM11正式任务已准入：

- 新增维护者配置`DEVICE_V2_TASK_PROVISIONED_DEVICE_IDS`，默认空，必须是任务管理名单的精确、唯一子集，不与Ed25519逐ID名单重叠；客户端请求不能选择profile、算法或根。
- 新窄模块`task-provisioned-profile.js`校验名单及实际Admin的epoch、版本、16字节controller、32字节fingerprint。固定suite1／PlatformAccess domain2／全权限15，共用原`taskAuthorityRootDigest`；复制非秘密材料，不导出／新增Admin secret。
- opt-in缺失时，仍拒绝已有Direct；opt-in存在但实际Admin缺失／畸形也拒绝，绝不回退Key-only。显式配置必须实际选中P256签发器；只检查“不在Ed名单”不足。
- `ownerAuthority`统一user→tenant→device advisory、tenant行→device行→Key／Direct行锁序，与原owner／根管理次序对齐。原SERIALIZABLE、`maxRetries:0`、1000ms锁等待和3000ms语句上限保持。
- prepare／issue／confirm沿原新鲜authority检查，并将同事务锁下Admin传入共同根；`beginRetirement`独立构造根的入口也传同一材料。Cloud回执仍按原DeviceKey角色验真，不创造Admin云秘密或新执行链。
- Admin变化后旧scope／request不能静默Rebind；保留原`ROOT_CHANGED`／`AUTHORITY_REQUIRED`、CAS及UNKNOWN语义。运行`.env`、两端名单、镜像、签发材料和业务数据库本批均不改。

### 13.4 有限验证及当前停点

新增纯回归及一个独立PostgreSQL文件，只覆盖本批新合同。真实PG使用既有云主机上**全新随机临时数据库**，经原pinned SSH隧道、本机原Node执行；没有本机Docker、部署候选、复制业务数据或旧scope。

PG唯一队列已通过：5个新增子场景（TAP6项含父测试）、失败0／跳过0，约60.063秒；覆盖默认拒绝、缺Admin／非owner、共同根Initialize→Read→DeviceKey回执→退休、issue／confirm阶段Admin替换拒绝且旧意图逐字保持、真实Admin行共享锁使另一连接UPDATE在200ms锁预算内被拒，以及Admin删除不降级。测试中的独立签名／MCU回执是新随机隔离fixture，不是真机成功证据，不造业务活动记录。测试结束仅删除所建数据库，核精确OID及连接已释放；没有删除业务库、服务容器或旧卷。

纯回归首次7项中6过、1项夹具把metadata回执kind写错，纠正测试后只复测失败项通过；另强化客户端extra字段测试，先证明合法body可解析再证明extra字段拒绝，定点通过。实际启动表达式新增1项通过：“显式provisioned但无key”必须进入真实factory失败保护，不能静默跳过；仅求值原表达式，不启动服务。合计8个不同新增纯用例分批通过，不把重复定点计作新增；受影响原签发测试仅适配新config字段／advisory及Direct查询夹具后，必要回归10项通过。原TAP保留，不称单次全绿；新增两文件已接入原`test:task-management`，没有为加入口重跑整组。

独立定点审阅未发现阻断。锁序结论是源码与SQL审阅，真实PG只验证Admin行共享锁和另一连接UPDATE的有限竞争，未执行完整resetAdmin／BLE enrollment race；200ms是测试`lock_timeout`配置，不是端到端耗时承诺。没有以此冒称所有根变更并发矩阵通过。

**当前仍未开放COM11任务。** 未刷板、读取或初始化TaskStore、生成／配置签发材料、创建云scope、下发任务或Start；原授权／UNKNOWN／完整请求日志保留。源码及有限回归是Gateway前置，不冒称设备组合已链接或App任务真机已通过；暂不提交推送。

终核再次读取App完整7项secure和2份journal，均与本批前置逐字一致，已装APK未变；未发送认证、reveal Key、重放或任务请求。原账号两笔GET／完整响应、logcat及私有快照位于App忽略目录`build/qa3-admission-20261031`，云只读核验／PG完整TAP与精确OID清理在Service同名忽略目录的ACL私有子目录；纯回归原TAP另在该Service批目录。新增源码／测试、README和原测试入口已整理，所有无关本地修改保留；无私有资料加入Git。

### 13.5 下一批唯一最小范围

先补设备组合，而不是重跑配网或重新做资源大审计：

1. 新内部受限profile，只为原SoftAP＋ESP32／P256／四槽接原`WifiTaskExtension`，一个Client、一个Cloud carrier、同一时钟／TaskStore；原公开及D4图不变。
2. 从一次真实Store读取取得已安装Admin selector（id／epoch），以DeviceKey＋Admin构造实际根。配网／重配阶段任务保持暂停／Pending，END／deinit真实完成后仅由cooperative owner刷新；Active期根变更接统一invalidate→owner延后refresh，未接全前不能继续用旧cache执行。
3. 原SDK单一候选定向完整链接通过后，仅保留必要NVS／授权和请求证据、只读核TaskStore与本批云scope是否一致。非空／旧根／UNKNOWN一律停在原协调合同，不清Store、不复用旧scope或强行Initialize。
4. 再将实际私有签发材料与候选公钥／keyId精确匹配，有限部署匹配服务，仅为该ID设置两端名单和App既有开关；新scope只能由该设备真实Empty证明后按原链建立。
5. 条件齐备后只补一次真实App任务短验：查询、保存一条停用定义、启停、一次Start／到期回执和新鲜State，区分确认与列表刷新；按实际新Store／scope一致性收尾，不把零槽旧固件恢复到活跃新scope上。没有必要不再配网、发码、换身份、重复BLE／断电或已过自动化。

## 14. QA-3：SoftAP四槽设备双根与生命周期围栏（2026-10-31）

### 14.1 本批范围与不变项

本批按§13.5只改设备库必要源码、有限回归及独立内部编译工具。目标固定XIAO ESP32S3／原SoftAP图／P256／Cloud TLS／四槽；原TaskStore namespace `bl_v2_tasks/snapshot`、Codec、任务执行器、同一Client／Cloud carrier、时钟与管理协议不变，不新增第二条执行链。

不访问手机、COM11或云业务，不读取／复制真实Key、PoP、网络密码、TaskStore或旧scope；原账号、授权、UNKNOWN及完整请求日志不操作。§13运行签发ring与名单为空的事实仍沿原证据，本批不重读或修改，不称已正式准入。原公开0槽、D4手动四槽及其禁止配网的发布guard不放宽；R4／Wio、BLE管理、同镜像动态双配网及扩槽不在本批。

### 14.2 实际双根及写前保护

- `IDeviceAccessStore`新增默认Unsupported的非秘密Admin selector；实际`DeviceAccessStore`一次`loadState`取唯一已安装PlatformAccess Admin的controller ID／epoch，不循环读Flash，也不从请求、旧任务binding或MAC猜根。
- 新内部扩展始终固定DeviceKey source＋controller source＋实际selector／epoch，沿原`taskAuthorityRootDigest`读取真实两根。缺Key或Admin即失败，不能降级为单根；Admin秘密不进入日志或外部输出。
- 可选`IDeviceAccessChangeObserver`默认不安装。实际持久写前以无等待原子CAS排除任务owner；忙时返回`WouldBlock`且不写。精确幂等／验证失败不产生虚假的根变化事件；已获准write／clear及读回验证成功或失败均返回最终结果。
- 原Store调用者仍负责串行化其自身操作；这个gate只补“根commit与任务owner”互斥，不宣称整个Store新增任意多线程事务能力。共享controller／presence写亦保守围栏，避免漏掉单blob变更。
- SDK observer只操作单次尝试的C++11 `atomic_flag`门闩、dirty标记和门闩保护的uncertain状态，不读Flash、不调用TaskAdapter、不等待或执行回调重入。cooperative owner取得gate后先停管理／invalidate原根，再单次读取selector；只有该owner恢复原管理生命周期，旧scope根不一致仍Pending，不自动Initialize／Rebind。

审阅中特别修正了不确定写合同：`nvs_commit`失败后，同一打开句柄可能仍能读到未提交视图。**获准写／清除／读回失败即锁死本产品实例的任务恢复**；即使后续load／selector／精确重试看似成功，也不以同一failed handle确认durable，不自动重写或commit。stop/start同实例不解除uncertain；须冷恢复的新存储生命周期再协调，保留原任务和未知意图。

### 14.3 原生命周期的最小接线

- `TaskProductLifecycle`增加默认NoOwner空基策略及默认ready=true的窄门槛；普通手动WiFi图保留原路径和32位大小门槛。内部组合初始ready=false，配网期间不开TaskStore、不启动任务管理、不执行任务，原transport仍poll。
- 原产品转Active只标dirty，不在该转换tick执行任务；下一cooperative owner持有gate后读取selector，再由原TaskModule／TaskCommandAdapter开始或恢复。TaskStore故障独立于普通控制，不靠清空／新建Store掩盖故障。
- 只有WiFiProv原`Complete`已确认真实deinit，且`end()`后确为Stopped，才允许转Active。end超时／请求结束不代表native owner已释放；新组合保留pending stop，后续只观察真实deinit及排他gate后关Store，不重复阻塞的stop请求。
- `Product.run()`仅为已停止但仍有pending teardown的产品继续清理poll，不恢复普通控制或任务；pending销毁无法安全继续时禁止释放仍被原生回调借用的上下文，不以ready／固定等待冒充回收完成。
- 本内部候选明确**不承诺Active Direct在线根轮换成功**：owner cycle内根／shared／presence写会遇忙拒绝，普通属性控制不受影响；实际根变更须在显式停机／维护时段经原Store进行。没有用布尔“可重入”放行其他SDK线程，也没有为方便测试绕过根围栏。默认零槽原产品不安装此observer，不新增该限制。

### 14.4 有限回归证据

只执行本批新用例和实际受影响的默认生命周期，不重跑原配网／自动化／断电矩阵：

| 分组 | 证据及准确边界 |
| --- | --- |
| Access Store | 5个不同新增场景分批通过：单次selector、所有持久写围栏／幂等、拒写与不确定结局、同failed handle可见新值仍保持uncertain、实际selector到共同根及缺Key／Admin拒绝。outcome签名修正后只复核受影响新项，不重复旧9项 |
| 共同根独立对照 | 合成Key／Admin／instance经独立JS协议计算，保留原Admin fingerprint的NUL分隔；真实C++ Store selector生成摘要`493c7124…0adbb5`一致。不是生产凭据或实际设备授权证明 |
| Task产品 | 6个不同新增场景分批通过：延后Store／管理、owner与根写互斥、uncertain不解除、真实native关闭观察、Product.end后仅drain cleanup、未准入／未知Store不能先擦授权。原子门闩实现变化后仅定点复核受影响2项，不重复计新增 |
| 默认NoOwner | 仅7个受影响生命周期回归通过：普通运行／网络重置、重置事务、回调中退休、准入失败、scope观察、终态snapshot、权限恢复。没有重跑原容量8／10及其他未变矩阵，不扩槽 |
| 内部profile／guard | 5项新release测试通过，覆盖新内部身份、TLS／签发来源拒绝、隔离生成、正反向include顺序与错误图／policy、原公开profile及D4 guard不变；不启动SDK或硬件 |

上述是分组有限证据，不是一次全绿或完整ESP32线程调度／真实断电／设备执行验收。先期agent日志摘要明确来源于实际tool输出，不将摘要伪作原始stdout；Root新增根向量／默认回归等完整stdout另存`build/qa3-device-graph-20261031`，Store批记录在`build/qa3-device-store-host-20261031`。

### 14.5 单一原SDK候选链接记录

独立内部profile为`esp32-softap-task-qa3-v1`；沿原renderer生成隔离库的全TU配置，固定四槽及原policy。原`arduino_release_profile.py`、公开cloud-beta与D4 guard未改。候选仅含非秘密测试PoP和合成Sketch，不读真实网络／Key；公开P256材料只作静态编译输入，标记`deployable=false`、`runtimeSigningMatched=false`、`formalAdmission=false`。

固定原CLI1.5.1、ESP32 Core3.3.11、PubSubClient2.8，FQBN `esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`、jobs=1。原SDK platform／sdkconfig／CLI摘要须匹配既有基线，不加platform.local或force-include，不更改分区或开放串口。

首轮仅在Arduino库发现阶段失败（16.608秒）：测试Sketch先include嵌套OfficialConfig，库尚未被CLI发现，未开始完整链接。原因明确后只修测试入口为先定义三项受限选择宏、include原根入口`BlinkerWiFiBLE.h`；所有容量／policy仍由已生成库配置约束。原source／intent／失败日志保留，在同一候选及build目录显式续队列，不自动重跑已通过测试、不另开重复队列，不改生产源码或SDK。

修正入口后的续段在223.763秒暴露原SDK实际编译差异：Xtensa工具链的`ATOMIC_INT_LOCK_FREE=1`，不能通过“32位整数原子始终无锁”的断言。未删掉保护强行编译，而是只将根门闩换为C++11保证无锁的`atomic_flag`单次test-and-set／clear；无spin、等待或弱化遇忙拒绝。uncertain在独占门闩内访问，dirty单独原子标记。2个受影响门闩场景定点通过；独立只读审阅确认该串行合同下无已确认的数据竞争、丢dirty或错误释放。原断言失败、旧门闩源码及原日志保留。

仅更新同一隔离库中的该头并重新生成匹配库身份摘要，同一build目录显式继续；原SDK、policy、容量和分区不变，不另开重复队列。最终编译／完整链接退出0，419.686秒。后置审计工具又有两项期待错误：原模块实际存储类是`TaskSnapshotStore`，不是名为`TaskStore`的类；内联静态产品对象在本工具链标为弱对象`V`，不能只匹配b/B/d/D。实际源码与ELF已确认后，仅离线纠正审计期待、排除独立guard变量并检查现有产物，**未再编译或改变冻结源码**；原审计停点保留。

| 最终产物／核查 | 实际结果 |
| --- | --- |
| BIN | 1,434,544 B，SHA256 `c4670ccd9899df843831644a75e3436e859c7d2c73687285de2c2900760a4f5a` |
| 编译器资源报告 | Flash 1,434,400 B；静态RAM59,588 B，链接区内剩余268,092 B，不是动态heap／栈运行保证 |
| 分区 | 与原编译基线逐字同SHA `ab2f33c1…79cb0e2`；NVS `0x9000/0x5000`、app0 `0x10000/0x330000`不改，BIN小于3,342,336 B。不是本批新鲜读回的COM11 Flash布局 |
| 实际消费者 | SoftAP存在／BLE provisioning不存在；TaskSnapshotStore、TaskEngine、TaskCommandAdapter／Session、TaskCloudTransport、实际Admin selector、根摘要与门闩observer均在ELF；只有一个主产品实例，未计独立guard |
| 输入一致性 | 442份仓库source、443份隔离安装文件、Sketch／依赖及CLI／platform／sdkconfig与最终checkpoint一致；原SDK无override，配置消费者`.d`指向同一stage，无force-include |
| 交付边界 | `deployable=false`、`runtimeSigningMatched=false`、`formalAdmission=false`；静态P256材料不代替当前云签发、TaskStore、真实Hello／容量或App验收 |

独立只读产物审阅复核BIN／ELF／map／分区hash、全部checkpoint及单实例／实际消费者，无阻断。最终记录为`build/qa3-device-softap-20261031`下唯一候选的`prepared-atomic-flag.json`、`softap/candidate.json`及原始三段编译／后置审计记录；不是三份运行候选或一次首轮全绿。共16个不同新增场景（Store5＋Task6＋profile5）及7个受影响原回归分批通过；定点复测不重复计数，不代真SDK调度／硬件持久化或现场并发。

### 14.6 关闭范围与后续

本批没有部署、刷板、配网、发码、任务请求、签发私钥生成／配置或准入开启，不触碰原授权／UNKNOWN／完整日志，不搬旧scope、不重放、不升级SDK，暂不提交推送。链接候选不等于COM11真实任务已可用。

后续只剩§13.5的运行前置：必要NVS／授权保护后只读核当前TaskStore、真实云scope及UNKNOWN；按实际签发材料匹配公钥／keyId、匹配服务和逐ID名单，再决定一次真实App任务短验。非空／旧根／不确定状态先协调，不能为赶验收清Store或套旧scope；不因此重开基础配网或资源大审计。

## 15. QA-3：运行签发与COM11只读前置及离线部署接线（2026-10-31）

### 15.1 新鲜运行事实与签发停点

按§14.6仅在原pinned SSH主机执行有界、固定目标只读核验：PostgreSQL事务显式READ ONLY，Broker SQLite只读／query_only，不调用业务API、签发、MQTT任务或旧请求。检查进程内部读取原进程环境以重建实际配置，仅返回固定公开投影；未将DeviceKey、私钥、token或完整环境输出到终端／证据。本批实际ring为空，没有读取签发私钥文件。

| 核对项 | 新鲜结果与边界 |
| --- | --- |
| 运行服务／Auth | Gateway、Broker、PostgreSQL、Redis均healthy；API／内部Auth仍为`https://api.yiyu.pro`，本批未重建或部署 |
| 实际签发 | ring为0、目标selected为null；**当前没有实际algorithm／keyId／公钥可匹配**。配置keyId默认1及源码P256优先，不等于已构造实际Task signer |
| 共同根运行接线 | 运行dist尚无provisioned配置识别、专属模块及Repository接线；仍是拒绝已登记Direct Admin、固定`admin:null`的旧运行合同。§13源码不能当作已部署 |
| 准入 | Gateway管理0／Ed25519 0／provisioned 0，Broker任务0，目标均未获任务准入；未修改运行名单 |
| 原设备身份 | 原COM11逻辑ID `device_b60437fc-dfe2-4745-91cc-57efbf6bf024`／原owner、DeviceKey版本1 active；云instance仍`215af06b…9a5e40c`，Direct Admin仍epoch1／版本1，controller／fingerprint与§13保持 |
| 云任务数据 | 该ID全部历史scope 0、request 0、attempt 0（含prepared／issued／unknown／verified分类），不是仅查request漏掉attempt UNKNOWN；无搬迁／重放或SQL业务写 |

实际消费仍是Gateway既有`DEVICE_V2_BLE_P256_PRIVATE_KEY_FILE`／`KEY_ID`，Task复用同一签发ring；不新增第二套签发接口或从客户端接受算法选择。本内部profile继续固定algorithm2／P256-SHA256-RAW、keyId1。§14静态SPKI SHA256 `57f69d14…8a6025`只是历史编译输入，**没有证据表明对应私钥已在当前服务配置**；不能用JWT密钥、公开keyId或旧Auth环境推导匹配。

### 15.2 COM11实际TaskStore／授权与恢复

只操作USB序列号／ROM MAC均匹配的COM11，COM9等其它板不动。复用原esptool及严格离线NVS auditor：进ROM后只读安全状态、4 KiB分区表、20 KiB NVS，并用ROM verify再次比较NVS；无8 MiB全片备份、stub写入、Flash程序／擦除或串口业务命令。ROM读取会中断云会话并reset，不称完全非侵入读取或整板断电测试。

- 安全状态仍Secure Boot／Flash Encryption disabled；实际Flash分区表SHA `5f451391…d37e7a`保持，NVS仍`0x9000/0x5000`。这是实际二进制表，不与§14编译CSV hash混用。
- 新鲜NVS为20,480 B、SHA256 `73d94901ec0ff205f5ac172f70a6e086f7060b0f7eaa4ae4a57b282d71728a51`；instance record及260 B根记录逐字匹配原授权基线，DeviceKey存在、epoch1、一个controller。未另建身份、清根或写WiFi。
- 严格NVS页／item／payload CRC及blob重建通过：`bl_v2_tasks` namespace不存在、`snapshot`不存在，离线观察为**Empty／未初始化**；不是已有零任务绑定表，也不是UNKNOWN、损坏或退休标记。未创建namespace、Initialize或重置Store。
- 原SoftAP零槽程序在`0x10000`经ROM verify匹配`c05195d7…36b96eff`，没有烧录§14夹具。保持原NVS及分区，最后仅reset回该程序。
- 一次后置云只读核对确认：同原instance、reset之后新issued的active云session，Broker MQTT在线；scope／request／attempt仍全0，Broker任务准入仍关闭。不用旧online快照代替恢复；不重复配网或断电。

此处Empty是本次持久字节观察，不证明新四槽候选的运行Hello、容量、时钟或原TaskStore.begin已经成功。未来首次scope仍须走原Initialize／签发／真实设备回执确认链；不得因本批Empty而直接写scope表。

### 15.3 必要接线修正及有限回归

发现的实际交付缺口：原`deploy/beta/compose.tasks.yaml`仅为手动D4接管理名单／签发，未设置新provisioned名单；旧checker通过不能证明配网共同根已接入。保持原D4 overlay／checker逐字不改，不改Task引擎、Store、签发器或App。

新增独立`compose.tasks-qa3.yaml`只在原任务overlay后增加同一`BETA_TASK_DEVICE_ID`的Gateway provisioned名单。新`check-tasks-qa3-config.cjs`先复用原checker，再要求Gateway管理／provisioned和Broker三处同一唯一ID；显式非秘密证据核实际selected P256／keyId1／同编码SPKI摘要、运行共同根config／module／wiring及匹配可部署运行候选。空ring／null、共同根未部署、缺项／不匹配、link-only／不可部署均拒绝；16 KiB证据和2 MiB stdin有界、固定错误不回显输入。

检查器只验证运维提供证据的形状与一致性，不认证来源／新鲜度、不读取私钥／环境／DB／网络，也不代替TaskStore／scope协调；通过也返回`formalAdmission:false`。规范化当前完整运行配置及全部既有功能必须保留，新增overlay不是base-only重建命令。不得将§14夹具的`deployable:false`改成true冒充已生成真实候选。

仅执行`node --test deploy/beta/tasks-qa3-config.test.cjs`一次：**5项新纯回归全部通过、0跳过**，Node duration 1360.9206 ms。正向为明确合成的离线形状fixture，不是真实运行材料。覆盖唯一ID／旧mount保护、严格schema、空ring／共同根未部署、材料不一致、不可部署夹具及CLI大小／脱敏错误；未重跑原D4、配网、SDK链接或自动化测试。原D4两文件Git blob与HEAD一致。

另外从本批真实只读结果及§14原checkpoint生成公开投影，在明确标为**拟议形状而非实际rendered Compose**的输入上检查，预期停止于`INVALID_QA3_TASK_CANDIDATE_RUNTIME_SIGNER`。这只证明真实空ring被拦，不称本批已验证实际合并Compose或完整运行发布。

### 15.4 下批唯一最小范围与短验合同

已补齐“当前设备Store是否为空”的证据，不再默认重复ROM读取、配网或旧引擎回归。剩余工作依次为：

1. **实际私有签发材料**：先确认可用的专用P256材料；当前ring为空，需按原私有挂载合同配置或安全准备材料，禁止拿MQTT JWT／旧Auth库替代。仅投影实际algorithm2、keyId1、SPKI SHA，并与设备输入逐字匹配；缺材料不先开名单。本批未生成、配置或上传任何签发私钥。
2. **真实运行候选与默认关闭的Gateway**：沿§14单一库／原SDK／同SoftAP四槽共同根图，生成含真实匹配PoP／公开签发输入的运行候选；继续从原NVS取原身份／WiFi，不重新配网、不写旧scope。§14非秘密夹具不能直接刷入。部署包含§13共同根消费者的匹配Gateway时，先保留全部运行配置、账号／授权／完整日志，名单仍空；再核真实dist和公开签发投影，不能仅凭healthy通过。
3. **限定短验前置**：同owner／instance／实际双根、原任务字节仍无变化或有合法只读协调结论；新候选真实Hello／四槽容量／时间／Cloud carrier满足；Gateway管理＋provisioned及Broker名单只限该ID，App沿既有功能开关，不扩大正式准入。以上齐备后再进行一次原App查询→保存一条停用定义→可逆启停→一次Start／到期回执／新鲜State，分别记录确认与刷新耗时。

新scope只能由原链建立，不搬旧scope、不补造UNKNOWN、不重放请求。若出现非空／旧根／不确定结果，停在原显式只读协调合同。新四槽Store／云scope建立后，不自动恢复零槽固件或原空NVS；先保留一致的候选／Store／scope，收尾须按实际退休合同，不造成新旧状态分叉。

本批正式准入继续关闭；未操作App账号／授权／journal，原UNKNOWN和完整请求日志未删改。没有业务任务请求、Bootstrap、WiFi下发、发码、依赖／SDK升级、服务部署、Flash写入或提交推送。云端读与ROM原日志／必要NVS位于两仓库忽略的`build/qa3-runtime-preflight-20261031`私有ACL子目录；5项回归工具输出来源摘要在Service `build/qa3-offline-deploy-20261031`，注明事后tool摘要而非原始stdout，未为落盘重跑。全部无关本地修改保留。

## 16. QA-3：专用P256材料、匹配运行候选及默认关闭Gateway（2026-10-31～11-01）

### 16.1 真实材料与最小发布范围

用户授权执行§15.4前两步，本批不开放第三步任务短验。只在原pinned主机新建root私有目录及专用P256签发文件，目录0700／文件0600；私钥从未下载到电脑、Sketch、App、Git或构建包，不复用MQTT JWT、Auth或历史静态签名私钥。准备阶段没有改变运行配置。

实际公开投影为algorithm2／P256-SHA256-RAW、keyId1，DER SPKI SHA256 **`02879e20a6ad6e021ed7e6c857f0a351db14b1568c467db519aac09920b6d061`**。这是新专用材料，不是§14旧静态`57f69d14…8a6025`；只注入本批内部生成配置，公开`cloud-beta.json`／D4原guard及支持范围不改。该变量沿既有BLE／Task共同ring，不能据此宣称所有旧固件已取得新公钥或完成签名轮换。

### 16.2 Gateway有限构建及实际部署

复用原`freezeBuildContext`冻结源码，使用现有锁定依赖仅编译Gateway至独立输出，成功；无npm安装、依赖升级或本机Docker。对照运行JS hash，差异严格为5项：`config.js`、`index.js`、`device-v2/task-grant-profiles.js`、`task-management-repository.js`、`task-provisioned-profile.js`。原协议／contracts、其它Gateway模块／资源和Linux依赖均沿现运行镜像，不捎带其它本地源码变更。

采用已用的`compiled-runtime-delta`，不是声称完整Linux依赖重建：原镜像`8b353f33…f5699a`为固定base，离线、无pull构建并逐文件核整棵dist／packages。新Gateway镜像为`1856b7bcde675a88761227244022bc93706a6e92429de57fe834339f6c0b6ad9`，冻结源摘要`bdeeb9cc…b45d10`；44,402 B包只有5份编译JS与manifest／Dockerfile，不含PoP或私钥。

保留原规范化Compose、四容器inspect和完整可用日志；切换前对旧Gateway日志附着follow保留退出尾部。一次Gateway-only recreate，原Broker／PostgreSQL／Redis容器ID与镜像不变，原端口／已有挂载、账号／授权、Auth、其它环境及功能保持。只改变Gateway镜像、增加专用签发文件只读bind和P256路径／keyId1配置，并显式置provisioned名单为空；原管理／Ed25519／Broker任务名单也仍空。原规范化运行文件原位原子更新、权限／owner保持，原配置和旧镜像保留，不自动rollback或重试重建。

### 16.3 运行核验与工具停点边界

- 四容器healthy；新Gateway实际dist／packages整棵hash与审计镜像一致，共同根config／module／wiring均存在。不能仅凭模块在磁盘就声称实际任务请求通过。
- 由运行进程实际环境和挂载私钥在容器内推导公钥，ring1，P256／keyId1及上述SPKI／65 B公开点逐字匹配准备材料；仅返回公开投影，不输出完整环境／私钥。管理／Ed25519／provisioned及Broker任务名单全0，**Task服务仍未启用、没有签发Task grant**。
- API／内部Auth仍`api.yiyu.pro`，声明和实际schema均058；COM11云scope／request／attempt仍0。切换前后全部task UNKNOWN行数量及有序完整行摘要相同；Broker不重建，原命令ledger／请求日志不删除。
- 内部`/healthz`200。一次公网`/healthz`返回404；原公开Nginx合同不开放此路由，不能为了工具的200期待改Nginx或放宽入口。保留真实404、内部200及容器健康三个不同证据，不把404冒充公网业务验收。

核验工具先有两处Broker配置导出／stdout解析假设错误，随后公网health期待也过严；原退出及stderr全部保留。只修只读核验工具，成功Gateway／数据库阶段落盘后从checkpoint继续，Broker输出按固定前缀解析，后续仅补现有镜像整树核对；没有再次构建／部署、自动重试公网探针、业务请求或任务签发。不是生产缺陷修复，也不写成一轮全绿。

### 16.4 COM11单一运行候选与独立匹配确认

仅在库忽略目录`build/qa3-runtime-candidate-20261031`新建ACL私有stage，442份源码与§14最终checkpoint一致。原Core3.3.11／CLI／PubSubClient2.8、XIAO ESP32S3 default_8MB、jobs1、分区及唯一产品／Cloud carrier不变。旧c051已校验的私有PoP header仅在本机原样复制到私有Sketch，不输出值、不生成新PoP，不内嵌DeviceKey或WiFi；从原NVS沿原链加载原身份／网络，禁用测试自动reset／配网命令。

独立Node公钥导出确认本批新材料SPKI，renderer仅替换生成stage的公开签发输入；§14不可部署夹具、原输入和证据均不改标签。唯一原SDK完整编译退出0，730.449秒；保留原intent、返回记录及完整compiler日志，没有重编／第二队列。442份source、443份隔离库文件／依赖及原CLI／SDK摘要保持；实际TaskSnapshotStore／TaskEngine、双根selector／门闩、管理Session／Cloud carrier、私有PoP header消费者和唯一主产品实例均通过审计。

| 产物 | 本批真实结果 |
| --- | --- |
| BIN | 1,434,544 B，SHA256 `0d955883458cb8c47b3aa65a7b6735e7483a5e92bc7013003434face69908a3b`；与§14夹具不同，不替换旧记录 |
| 资源／分区 | 编译Flash 1,434,400 B／静态RAM59,588 B；不是运行heap／栈保证。原CSV SHA `ab2f33c1…79cb0e2`、app `0x10000/0x330000`保持，未改分区或容量 |
| 独立公开材料确认 | 对实际BIN检出65 B公开点，Node独立重算DER SPKI，与输入、冻结候选及真实running投影一致；不是实际Task grant签名／真机验签或新鲜Hello证据 |
| 记录合同 | 原`candidate.json`全部false标签及hash不改；独立`runtime-signing-confirmation.json`只确认`runtimeSigningMatched=true`，`deployable=false`／`formalAdmission=false`保持，仍需单ID受限部署复核。没有为了匹配标签再次编译 |

Root定点只读核候选返回码、BIN／ELF／map／分区metadata及独立确认工具合同，无新阻断；未读取PoP值、展开私有编译日志、重跑SDK／原配网／host回归。此次为生成候选与运行材料收口，未将未刷板产物称作COM11四槽已可用。

### 16.5 后续与不变项

本批未访问App账号或journal、未操作串口／COM11 Flash／NVS、未建scope、发Task／Bootstrap／WiFi、重放UNKNOWN或发送验证码，正式准入继续关闭，暂不提交推送。原授权和完整日志保留；新Gateway继续提供原服务，原PoP／必要NVS不放进Service构建包或Git。

候选完成且匹配确认后，下一批才考虑仅COM11有限部署及真实Hello／四槽／时钟／共同根、原控制ACK／State与原App任务短验；同owner／instance／原NVS保护和单ID三端名单必须分别核对。不是重跑配网，不把本批默认关闭部署称为任务已可用。新Store／scope建立后不自动回退到零槽或旧空NVS，仍沿§15.4收尾合同。

源冻结、编译、材料公开投影和部署／核验原记录在Service `build/qa3-runtime-release-20261031`、设备库 `build/qa3-runtime-candidate-20261031`的忽略私有目录；服务器私钥、旧规范化配置、四容器日志及切换checkpoint在root受限操作目录保留，不清理或覆盖旧证据。

## 17. QA-3：COM11原身份四槽任务最小真机闭环（2026-11-01）

### 17.1 前置、保护与唯一刷写

沿原内测App `7e88a390…14a0f0`，不构建／安装App或升级依赖。原账号inventory／设备详情各GET1、均200；7项secure及2份原journal前后逐字相同（现场原intent行均0，不冒称非零UNKNOWN并发矩阵）。请求／响应、完整可用logcat和后续wire均私有保留，不发码、登录或reveal Key。

新鲜云前置确认原owner／tenant／instance／DeviceKey版本1／Admin epoch1及版本1，schema058、scope／request／attempt0，实际P256／keyId1／SPKI仍`02879e20…6d061`；先保留规范化配置、四容器事实和日志，不以旧材料代当前事实。

仅COM11 USB序列号及ROM MAC匹配后，读安全状态、原分区表及20 KiB NVS并verify。真实Store为Empty，原instance／根授权记录保持；原c051程序先verify确认。只在`0x10000`写§16已完整链接的`0d955883…9908a3b`主程序并读回，启动前NVS及分区仍逐字相同；不写bootloader、分区或NVS，不清根、不启动配网、不建新身份。随后仅reset运行，新Cloud会话／Manifest／初始power=false State同原instance。COM9等其它板不刷写或发命令。

### 17.2 实际观测顺序与能力边界

现有协议下，Broker任务名单为空时不会协商bit25，TaskCarrier context因此503；Task Read／catalog也不返回UTC质量，原冻结Sketch没有时钟诊断命令。因此**不能声称先在全部名单关闭时取得真实四槽／UTC证据**，本批按已说明的分段执行：

1. 闭名单阶段先核固件／NVS身份、Cloud／Manifest及初始State；只读context一次返回`TASK_CARRIER_STALE`是预期门禁，没有因此重发Task。
2. 临时仅同一ID加入Gateway管理＋provisioned、Broker allowed，Ed名单仍空；相同镜像只重建必要Gateway／Broker，PostgreSQL／Redis身份和其余配置／端口／挂载保持。正式准入不扩大。
3. 通过**实际Broker进程的既有鉴权HTTP context**返回200，证明唯一真实物理连接、Hello完成、任务bit及下行订阅／最小212 B帧门槛；没有新Node空registry，未抓原始Hello或精确frame值，不冒称完整原帧抓包。
4. 原App正常确认Initialize，经实际双根及签发链成功，再取真实目录容量4／唯一允许目标power；仅在这些结果成功后保存／启停／Start。新scope走原链建立，不复制旧scope或SQL初始化。

本轮时钟证据仅为配置10秒的monotonic倒计时实际完成；不是UTC状态／来源／uncertainty测量，也不是精确MCU callback延迟或日期／星期任务验收。后者仍需独立窄诊断证据，不能从服务器发送了TimeResponse或HTTP时间倒推出设备已经正确校时。

### 17.3 原App操作及真实结果

生产包无Angular调试接口，全部通过原DOM控件／正常导航和确认框操作，不调用私有组件方法或直接fetch任务API。App首次catalog得到需Initialize的409并显示原授权入口，这是新空Store的正常引导，不算异常重试。

| 原UI操作 | 实际新鲜设备结果 |
| --- | --- |
| 确认启用／同步任务 | 新scope建立、Initialize verified／fresh、revision0，目录显示已用0/4；不是搬旧scope |
| 保存定义 | 一条10秒countdown、power=true、明确disabled；verified／fresh，revision1，taskId1、progress0／startSequence0。保存不启动 |
| 启用→停用→再启用 | 三笔各一次、均verified／fresh且`replayed=false`，revision2→3→4；未Start时仍progress0／sequence0 |
| 唯一Start | 一笔新请求，verified／fresh且非replay，revision5；后续新鲜Read为progress1／startSequence1。未点击恢复原请求或第二次Start |
| 退到系统桌面后执行 | Android topResumed为Launcher，原App任务视图已回收；后台期间服务器收到power=false／revision1→true／revision2。WebView `document.hidden`当时仍false，不能用它代替Android实际前后台证明 |
| 执行结果查询 | 原App后续新鲜Read返回revision5、progress3（Completed）、startSequence1，与真实power=true State相互印证。此处是查询得到持久执行进度，不冒称独立push回执或GPIO实测 |
| 最终停用 | 一笔新请求verified／fresh，revision6；最终Read仍progress3／sequence1、definition.enabled=false。保留该定义／TaskStore／scope，不删除设备或还原空NVS |

合计7笔不同管理写：Initialize1、Create1、Enable4、Start1；均有真实新鲜确认，全部操作及请求号私有保留，没有同请求重放。最终原App显示“已用1/4条”“定义已停用”“本次执行已完成”。截图已查看并保留`37-screenshot.png`／`41-screenshot.png`，仅为当前竖屏可见区域，不声称多机型或完整布局矩阵通过。

### 17.4 一次真实刷新UNKNOWN与只读恢复

revision2启用已确认后，其自动catalog刷新返回HTTP409 `TASK_OUTCOME_UNKNOWN`。原WARN精确关联为该新请求、operation2、**commit**段、`TASK_CARRIER_DISCONNECTED`、1088 ms；当时原attempt为issued，而非可判定失败的写操作。底层为何断连仍未定位，不等同于历史InternalError8，也不从断连推断原目录请求是否完成。

立即停止后续写入；原App保留“设备已确认本次管理操作”并显示查询入口，没有撤销启用或自动重发。随后明确点击原“重新查询”，全部为全新请求号，真实目录／Read恢复revision2、enabled=true，再继续剩余限定步骤。没有重放原catalog、旧UNKNOWN或任何管理写。

该Read的issued快照、exact prepare／grant、原wire及完整Gateway／Broker日志已保存。原Repository对Read/catalog有60秒过期回收，后续正常请求可清理这些短期Read；未改此合同／手工删库，原私有证据不删除。最终数据库unknown计数0不抹掉这次真实UNKNOWN，也不能用它声称未出现断连；未知**写**及原非目标UNKNOWN保护保持。

另有工具停点：首读在手机休眠时未取得账号快照；UI助手后台先读原生存储会等待暂停的回调，后调整为先唤回Activity；返回时原页面已有Completed，所以不存在的恢复按钮被工具拒绝。均保留原停点、未引发重复写或Start，未修改生产App／固件源码。最后离线汇总首次按文件名而非CDP事件时间排序，纠正汇总顺序后只重核已有证据，不重复业务验收。

### 17.5 收尾、保留状态及下一步

App停回首页、确认新请求均已结算后，显式撤回临时单ID名单。原规范化配置逐字恢复SHA256 `bd228082…f6f9fdf`，相同镜像／签发文件及挂载保持；Gateway／Broker因准入切换重建，PostgreSQL／Redis容器不动。切换前后完整目标ledger快照及原非目标UNKNOWN摘要一致。终核两端任务名单全0，真实context再次503为已关闭的预期结果，四服务健康、同owner／instance／Admin；保留scope1／当前11份已verified请求及attempt，无prepared／issued／unknown未结算行，不代表本批累计只有11笔请求。

**保留COM11匹配四槽程序、原身份／授权、新TaskStore与新scope一致状态。** 不回退零槽、还原旧空NVS、退役／删除设备或重放旧意图；原NVS保护副本仍私有留存。App 7项secure／2份journal完全相同，原现场intent为0；所有CDP／网络观察器和自有转发回收，临时USB常亮设置已恢复原值0。无生产源码新增修改、重建SDK／配网／自动化／断电循环、发码、提交或推送。

结论：**COM11／原SoftAP身份／TLS／P256／四槽／原App最小倒计时组合有限通过，正式准入仍关闭**；不是完整定时矩阵或稳定性全绿。剩余最小缺口仅保留：本次`TASK_CARRIER_DISCONNECTED`的底层连接变化原因、UTC时钟质量，以及如需要的原始Hello精确观测。优先用已保存日志只读分析断连，不为补证再Start、重跑配网或重开资源大审计。

原账号及完整可用logcat、原UI操作／HTTP正文／未知结果及截图在App忽略目录`build/qa3-task-live-20261101`的ACL私有子目录；必要NVS／唯一刷写证据在库同名私有目录；运行前置／每阶段完整scope、request、attempt字节及G/B日志在服务器root私有`/opt/blinker-qa3-task-live-20261101`，本机Service同名忽略目录留全部操作结果／原stderr。没有把私有材料加入Git或覆盖旧记录。

## 18. QA-3：目录刷新断连的已存证据分析与有界续期避让（2026-11-01）

### 18.1 只读时间线与可确认的原因

只读取§17既存远端证据，并保存49份Gateway／Broker原日志、任务ledger与会话快照的ACL私有副本，逐一校验SHA256（143188 B）。不请求业务API、不重新采样、不操作COM11、账号、准入或数据库。原日志及旧issued／UNKNOWN材料不删改。

| 已存事实（服务器UTC） | 关联结果 |
| --- | --- |
| 16:24:50.667旧proof签出，16:34:50.667过期 | 失败attempt绑定这一旧proof；不是把新proof补入旧Grant |
| 16:32:53.540目录attempt进入issued | 前一笔Enable已真实确认，故本次不是Enable或Start失败 |
| 16:32:54.210同instance／credentialVersion1的新proof签出 | 早于原断连WARN约397 ms；旧proof此时仍有约116秒有效期 |
| 16:32:54.607原WARN | operation2、commit、`TASK_CARRIER_DISCONNECTED`、1088 ms，保留UNKNOWN语义 |
| 下一proof于16:40:57.542签出 | 相邻换代间隔483543／483332 ms，与原480秒定期续期加HTTP耗时相符 |

原Broker进程的启动与退出分别在16:30:53及16:43:15附近，未在本次失败点重启；16:43收尾时的后端关闭错误不能倒灌为16:32失败原因。旧proof和新proof在保存快照中均可同时active，Broker的签出／校验实现也不按“最新proof”撤销旧有效proof。

实际消费者定位到设备`ManagedMqttTransport::poll()`：先`sessionProvider.poll()`，成功后generation变更，随后`applyCredentials()`立即stop旧MQTT、configure并start新MQTT。这里原来没有避让已Prepare的Task会话；provider成功同时还会覆盖旧MQTT所借用的凭据arena。因此**仅延迟apply、却照常消费provider响应，并不安全**。Broker的对应错误确实来自该次exchange所持物理socket的`close`，不是3秒TIMEOUT、SEND_FAILED或整体clear的CANCELLED。设备Read/catalog处理自身没有关闭MQTT的路径。

结论：**续期与Task交换的竞态是可复现、且与此次现场换代时间线一致的直接代码缺陷，本批限定修复它。** 原WARN日志级别下连接／关闭原因仅有DEBUG，且没有MCU续期起点／原生close日志；故不声称已取得此次每一个TCP FIN、WiFi、TLS或重启的物理归因，也不把本问题泛化为历史InternalError8。后者仍待定位。

### 18.2 最小实现合同

- `ISessionProvider::canDeferRefresh()`为可选const能力、默认false；Http实现独立检查started／网络、实际旧凭据及真实monotonic expiry、非Failed／Stopped。原`credentials().ok()`只证明视图存在，**不**用它冒充未过期。已开始的HTTP续期也可短暂避让，避免在Commit前覆写arena。
- `ManagedMqttTransport`保留原单参数poll行为；新增显式避让参数，仅在**已应用同一generation、MQTT仍Online、provider确认旧凭据仍有效**时跳过provider poll，MQTT收发和keepalive继续。首次登录、换代已发生、真实到期、已失败或网络失效不获得宽限；不延长凭据、不增重试或复制secret。
- `TaskCommandAdapter::pending(source)`只匹配当前完整source；`TaskCloudTransport`先age原Session，再只为本source的Prepare→Commit或原1秒Request／Reply窗口提出避让。原产品Session15秒／帧1秒期限不重置；Result发出、超时、root／连接变化后释放。尚未在设备接收到Prepare的远端在途窗口不在此保护范围，UNKNOWN合同保持。
- `LocalAccessCloudTransport`只薄传递这一参数，并在避让窗口不主动发起自己的周期refresh；没有第二连接、队列、任务执行器、固定等待或ready快照解禁。不改TaskStore、Codec、CAS、Grant、账号授权和读写回执语义；无结构成员增长。

### 18.3 有限回归与冻结边界

复用既有`build/qa3-device-store-host-20261031`，增量编译／链接3个受影响host目标（C++11，严格告警），全部通过：

- `blinker_task_management_session_tests`：测试图改用真实Managed＋MQTT＋Runtime＋TaskCarrier，只替代网络和provider。旧不支持避让的策略确定性复现pending读被换代取消；支持策略保持同连接到Result后再续期。另覆盖已开始refresh、回包WouldBlock、Session到期、旧凭据失效、根变化、实际socket丢失及7种错source不借用pending。均为离线合成fixture，不是真机重放或新Start。
- `blinker_http_device_key_session_tests`：实际Http provider在续期到点时暂缓、HTTP成功正文已到但尚未消费时保持旧generation／proof；释放后换代一次。真实expiry覆盖避让，已确认吊销仍停MQTT，LocalAccess薄转发及普通poll续期不变。
- `blinker_device_v2_tests`：受影响普通Managed及原基础消费者回归通过。最终补充错source断言后，仅增量重编并复核该Task目标通过，未另开重复队列。

独立源码审阅未发现本批阻断。Cloud-login冻结仅更新本批实际修改的Http provider与Managed头／实现4项（v50）；**全量架构脚本仍报Facade、WiFiProduct、Esp32WiFiProv、Esp32WiFiBLEProduct这4项既有冻结差异**，这些文件本轮未改，也没有直接刷新旧hash掩盖差异。其它架构检查未报告失败；不将该结果记为全量门禁通过。后续匹配候选交付前应只审阅这4项既有QA-3接线，再做必要冻结核对。

本批没有原ESP32 SDK重建／新候选完整链接、App构建安装、刷板、服务部署或现场复测，不用host通过替代这些结论；Core仍3.3.11，未升级依赖。

### 18.4 保留状态与下一批最小范围

COM11仍为§17的旧匹配BIN `0d955883…9908a3b`，原身份／授权、scope1、revision6／disabled／Completed／startSequence1保持；本批源码修复**尚未运行在板上**。原账号、日志、UNKNOWN及私有证据保留，准入名单沿§17继续关闭，UTC质量仍待补，不重放旧请求／配网／Start、不扩槽、不发码、不提交推送。

下一批只做匹配修复候选的必要冻结审阅及原SDK定向链接准备；不为验证续期再重跑配网／Start或开启正式准入。是否安排一次新的只读续期差异实测另按届时执行范围，不能把旧记录变成新版成功证据。完整分析副本及时间线在Service忽略目录`build/qa3-disconnect-analysis-20261101`的ACL私有子目录，不提交原日志／ledger或任何凭据。

## 19. QA-3：既有冻结差异审阅与续期修复候选原SDK完整链接（2026-11-01）

### 19.1 四项冻结差异的归属

逐段对照当前diff、原冻结实现及实际消费者，并完成独立只读复核。这4项不是§18新增的续期算法，也不是扩大产品准入：

| 文件 | 必要既有变更及边界 |
| --- | --- |
| `api/BlinkerFacade.h` | 仅为显式内部ESP32／SoftAP／非BLE配网任务图保留入口；普通BLE任务仍拒绝，不改Facade成员或begin合同 |
| `arduino/internal/WiFiProduct.h` | 为NoExtension补无状态钩子；默认路径不新增授权、连接或执行器 |
| `arduino/ports/esp32/Esp32WiFiProv.h` | const读取真实manager状态；END请求、超时或ready快照不是native deinit证明 |
| `arduino/platform/esp32/Esp32WiFiBLEProduct.h` | 原单stack／Client／carrier下接TaskSupport、双根维护围栏、延后任务启动及pending-stop；另有此前批准的PoP配置消费，缺省仍nullptr，不在本批创建或输出PoP |

未发现需再改生产源码的缺陷或无关变更；仅将以上4项摘要对齐为cloud-login冻结v51，并写明各自归属。v50续期修复4项及其它冻结条目保持。全量`tests/architecture/check_device_v2.py`通过，不以直接放宽检查器代替审阅。BLE任务管理、同镜像动态双配网、Active Direct根轮换及任意设备任务准入均未因此获得支持。

### 19.2 单一候选与编译输入

复用§16已有编译／消费者审计器及`tools/esp32_provisioned_task_profile.py`，不改旧冻结脚本、旧候选或已消耗队列。新批次只调整基线为§16成功运行候选，强制当前442份设备源码相对该基线**恰好只有§18的8项修复差异**、无文件增删；使用新的ACL私有stage保存输入。既有4项接线与上一成功候选完全相同，不重复实现。

- 原CLI 1.5.1、Core3.3.11、SDK platform／sdkconfig摘要及PubSubClient2.8依赖核对不变；不修改SDK或增加platform.local／build_opt覆盖。
- 原`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`、`jobs=1`，只启动一个全程序编译／链接队列，不只做头文件编译或复用旧BIN充新结果。
- 同一内部SoftAP＋HTTPS/MQTTS＋DeviceKey/Admin双根＋P256／keyId1／4槽profile；复用既存公开SPKI `02879e20…6d061`，不读取服务器私钥、不重新签发或改准入。这里是匹配已保存的公开材料，不冒称新一次运行服务器核验。
- 原Sketch及标量Manifest `power`／`uptimeSeconds`、原私有PoP头逐字保持；不嵌入DeviceKey、WiFi或命名空间覆盖。私有头只在受保护stage复制并核hash，不输出内容。
- 原app0 offset `0x10000`、容量`0x330000`及分区CSV保持；本批没有串口、ADB、云端业务请求或数据库操作。

### 19.3 完整链接、实际消费者与保留合同

唯一队列退出0、完整链接及原审计器均通过，耗时833799 ms（约13分54秒）；没有失败后重试或第二队列。

| 产物核对 | 本次结果 |
| --- | --- |
| BIN | 1434848 B；SHA256 `818dfb45bff67c6733a6c2f137b0bc3d3a7fdb55e4d1c1f8d93e3812a98f4480` |
| compiler Flash | 1434700 B，较§16运行候选1434400 B增加300 B；不把BIN对齐字节数当成同一指标 |
| 静态RAM | 59588 B，与上一候选相同；这是链接统计，不冒称动态RAM／栈重新实测 |
| 安装／源检查 | 442份冻结源码、443份隔离库文件；原SDK／依赖及已冻结输入在链接前后均保持 |
| 产品／任务图 | 唯一`esp32WifiBleProduct()::instance`、SoftAP而非BLE配网scheme、原TaskStore／Engine／TaskCarrier／双根维护消费者均实际链接 |
| 续期修复消费 | 实际ELF定义`HttpDeviceKeySessionProvider::canDeferRefresh() const`及Managed两参数poll；真实Sketch对象有对`ManagedMqttTransport::poll(unsigned long, bool)`的未定义调用引用，不仅是符号碰巧编入 |
| profile／身份边界 | 同Sketch与PoP头、同分区CSV／容量、同公开签发材料；无DeviceKey／WiFi／namespace注入，无命令行PoP或forced include |

另用实际`BlinkerControl.cpp.d`、`BlinkerTransport.cpp.d`及Sketch `.d`核对来自该隔离库的provider实现／接口、Managed实现、TaskCloudTransport及TaskCommandAdapter，并比对其冻结hash。LocalAccess在此profile关闭，不能将8项源差异全部记作当前ELF执行；其薄转发仍仅有§18的host证据。

输入、一次性intent、完整编译日志、BIN／ELF／map及摘要、定义符号、实际调用引用和`refresh-consumer-audit.json`，均保留在库忽略目录`build/qa3-refresh-link-20261101`的ACL私有子目录。`candidate.json`明确保持`deployable=false`／`runtimeSigningMatched=false`／`formalAdmission=false`：本批只匹配旧公开材料，没有重新读取运行服务或修改准入，也不发布含私有PoP的BIN。

本批仅新增必要冻结记录及文档，没有再改生产源码；§18已通过host目标不重复。沿§17保留COM11原运行BIN `0d955883…9908a3b`、TaskStore、scope1、停用Completed定义、身份和授权；新BIN尚未烧录。App、Gateway、Broker、账号、UNKNOWN及完整请求日志均未操作；无配网／Start、发码、SDK升级、部署、提交或推送。正式准入继续关闭、UTC质量待补，不将新候选链接当作真机差异验收。

本批最小范围已完成。后续若进入部署差异验收，只消费此已冻结候选及全新只读请求，不重建候选、不重放旧UNKNOWN、不重复配网或Start；仍需保持任务名单的限定范围与原Store／scope一致。当前“暂不部署”的边界不因编译通过而解除，UTC质量另列待补。

## 20. QA-3：冻结修复候选跨Cloud续期只读差异短验，未通过（2026-11-01）

### 20.1 前置、有限部署与保护

本批用户已明确授权仅COM11部署§19既有候选并临时开放单ID；不是自行解除§19限制。沿用原内测App `7e88a390…14a0f0`，未构建／安装App或重建固件、SDK、Gateway镜像。电脑在“带翅膀的蜗牛”；手机ADB及原账号可用。现有所有者inventory／detail各GET1、均200；不登录、reveal Key、发码或改任何额度。

新鲜服务前置确认schema058、原owner／tenant／instance、DeviceKey版本1、Admin epoch1／版本1及原scope1；运行签发仍为P256／keyId1／SPKI `02879e20…6d061`，实际refreshAfter480秒、TTL600秒。先完整留存配置、四容器、目标ledger和原日志，再仅临时开启原单ID的Gateway management／provisioned及Broker allowed，Ed名单仍空；同镜像只重建必要G/B，PostgreSQL／Redis及其余配置不动。

COM11 USB及ROM MAC匹配后，仅保留并verify当前20 KiB NVS和分区表，严格NVS CRC及TaskStore为BoundSnapshot／1条／revision6；原instance、根授权及控制器保持。原主程序`0d955883…9908a3b`先verify通过，再只在`0x10000`写§19 BIN `818dfb45…98f4480`并读回。启动前原20 KiB NVS及分区仍逐字相同，不写bootloader／分区／NVS、不清根、不初始化任务、不搬scope；正常复位运行，不配网或Start。冻结候选JSON原标签不改，新的运行签发匹配及用户操作依据另存，不把历史`deployable=false`直接改为正式发布。

### 20.2 一次窗口、三笔新请求与实际结果

实验最大范围为三次原App查询动作：基线、自然续期边界一次、确认换代后一次；遇到UNKNOWN／错误立即停止。本批实际只执行前两次动作、共**3笔**全新Task HTTP（catalog、read、catalog），没有管理写、重放或自动重试。

| 阶段 | 实际证据与结论 |
| --- | --- |
| 启动／基线 | 真实Broker context证明原物理连接Hello／任务feature及帧门槛就绪；原App正常任务入口产生catalog＋read，各200／verified／fresh，显示1/4条、tableRevision6、taskId1／停用／Completed／startSequence1，与原Store相符 |
| 安排边界 | 原页一次性触发原刷新按钮，非私有服务调用／直接Task fetch。后台／路由变化可取消、不自动重试；实际点击仅1次。窗口前Android确认为内测Activity在前台，USB常亮。所有时间基准及原始请求保留 |
| 边界请求 | 全新catalog `OvHmFAoCllvyKc5Em59dFA`（hex `3af1e6140a02965bf229ce449b9f5d14`），HTTP409 `TASK_OUTCOME_UNKNOWN`；App显示未取得可验证结果及显式重新查询入口，没有自动撤回或重发 |
| 精确WARN | 该请求operation2、**commit**、`TASK_CARRIER_DISCONNECTED`、Gateway717 ms；浏览器完整HTTP约1184.658 ms。原attempt为issued，exact prepare／grant及未知结果证据保持，不能把此请求算作失败回滚或已确认完成 |
| 续期观察 | 旧proof `1df0ffe1…89688518`于01:38:09.523 UTC签出；新proof `4d0cb78e…f49d10a1`于01:46:12.663签出，间隔483140 ms。随后真实context已换proof／connection，并出现该新连接的power=false新鲜State；仅证明换代已应用，不能代替Task读取成功 |
| 停止 | 发现UNKNOWN后停止；不点“恢复”或“重新查询”，不执行第三次换代后Read、不追加第二周期、不进行其它控制或任务操作 |

更细时间线：边界attempt于01:46:12.363创建并绑定旧proof；300 ms后新proof签出，01:46:13.066记录断连WARN（新签出后约403 ms）。新签出本身不是设备消费／apply的精确时点，旧proof当时未到期；不以这个先后单独断定内部gate何时为true／false。此候选没有新增内部避让／MQTT发送完成诊断，因此**避让成功未验收，换代后读取因停止而未测**。

时钟工具有一处单位问题：服务器`date +%s%3N`实际返回19位纳秒数，首次工具投影误作毫秒；保留原始采样后离线规范化，未重新采样或影响业务请求。最终调度跨进程时钟误差界约45 ms，实际点击比计划晚5 ms；设备从消费HTTP响应起算的monotonic续期点仍未直接测得，不将server-issued＋480秒当精确MCU起点。另有准备期旧client的keepalive timeout，属于刷写前的旧proof，不与本次01:46的commit断连混为一因。

### 20.3 收尾与证据保全

原App回到首页后回收CDP／Network观察器、自有ADB转发、一次性点击及临时JS标记；USB常亮恢复原值0。7项secure及2份journal前后逐字一致，两份原intent均0行；没有把非零UNKNOWN并发矩阵写成已测。截图`08-screenshot.png`已查看并保留，展示本次未知结果；不作为多机型布局结论。

临时单ID名单全部撤回，原规范化服务配置逐字恢复SHA256 `bd228082…f6f9fdf`，同镜像／签发文件及挂载保持，四服务健康；PostgreSQL／Redis容器不动，原scope／身份／授权及非目标UNKNOWN摘要保持。撤回后context503 `TASK_CARRIER_STALE`为关闭后的预期门禁，不触发新Task。当前目标ledger为7份verified＋本次1份issued，**unknown列计数0不代表本次UNKNOWN已解决**；未手工结算、删除或重放。

正常新Read会按原retention回收过期旧Read：目标计数11→9→8，不能当作本轮只有8笔或丢失写操作。原写记录保持，本批3笔及旧Read的完整快照／原wire都私有保留。当前保留新修复程序 `818dfb45…98f4480`与原TaskStore／scope，不退回零槽或还原旧空NVS；停止后未为了“终核”再补一次任务Read，也不冒称有UNKNOWN之后的新鲜任务结果。

完整原App账号／HTTP正文／logcat／截图及离线结果在App忽略目录`build/qa3-refresh-live-20261101`的ACL私有子目录；20 KiB NVS及刷写原日志在库同名私有目录；规范化配置、每阶段原ledger／context及G/B完整日志在服务器root私有`/opt/blinker-qa3-refresh-live-20261101`。其中39份既存日志／ledger／context已只读取回本机Service同名ACL目录并逐一核hash（104330 B），旧批证据不覆盖；汇总曾误读journal的rows对象为数组而输出null计数，已保留原汇总并附加正确0行投影，原始数据及深度一致性断言未受影响。

### 20.4 结论及下一批最小范围

**本轮验收不通过；§18的host／§19的链接成功不能推翻本次新失败。** 本批不修改生产源码、重建或再部署，不继续试探其它周期；正式准入仍关闭，UTC质量待补，Error8仍单列，不提交推送。

下一批先仅只读关联本次新请求的已有证据与实际消费者，区分三处窗口：进入同步HTTP前Task尚未入窗、pending时实际gate状态、以及Result本地QoS0 publish成功后到Broker／Gateway收讫并确认的尾窗。当前修复在本地send成功后clear pending、下一tick可apply新凭据；PubSub native成功不是远端已收结果。Broker还有“Result已暂存、但下行publish回调未结束”的分支，close仍可能先结算DISCONNECTED。尾窗是需核对的结构性缺口，**尚非此次唯一根因的直接证明**；已有exact_result为null也不能区分设备未发出与Broker尚未settle。原SDK HTTP poll的operation预算不是整次wallclock上限，但纯粹Prepare尚未接收的头窗通常应停在challenge，不能单凭它解释本次commit。先用最小离线时序复现区分这些路径、明确分界证据／修正合同，再另按范围实施；不直接增加固定等待或新协议，不重开资源大审计或升级SDK，不重放本次issued或旧UNKNOWN，不为补证重复配网、Start或第二轮真机采样。

## 21. QA-3：离线时序复现、排队RX头窗最小修正及确认边界（2026-11-01）

### 21.1 已存证据与三类时序的区别

本批不连接ADB／串口、云端或本机Docker；只读§20已存材料。39份原日志／ledger／context再次逐一核SHA256，104330 B全部保持，不改账号、授权、NVS、scope、旧／新UNKNOWN或原结果。现场attempt仍以已保存的issued／旧proof、已有challenge／grant、`exact_result=null`为依据；没有新现场结果可以填补内部gate／本地发送时刻。

用实际Managed＋MQTT transport＋TaskCloud＋Adapter／Session及Runtime组成host图，只替代socket、provider及外部I/O；新用例明确为**离线合成顺序，不是重放真实请求或Start**。

| 顺序 | 离线结果与本批处理 |
| --- | --- |
| Task帧已排在旧MQTT中，provider将换代 | 新用例在旧源码确定失败：provider先执行、连接换代、Task pending未建立。已作最小修正，先预收后重判，旧代完成该新读，之后恢复续期；红→绿 |
| 同步provider调用进行中才有字节到达 | 该调用不可被事后门控抢占；模型仍换代且没有成功challenge，不捏造已确认。此纯头窗不足以单独解释现场已到commit；本批不重造异步HTTP／升级SDK |
| 已有本source pending或原Reply阻塞 | 原有限门控仍阻止provider消费，deadline／根／连接／expiry边界保持；已有pending与阻塞Reply回归通过 |
| 本地Result publish返回成功，远端尚未确认 | 合成延迟peer观察的用例可在换代时丢失未确认Result；本批保留该负向合同，不将本地bytes／HMAC验算当远端收讫，也不加固定grace判成功 |
| Broker先收opaque Result，但下行callback尚未完成 | 7项新顺序断言确认slot仍BUSY；callback成功才返回，close／失败／取消／撤销／deadline仍fail-closed，晚callback不重放或触碰replacement |

因此本批只修**已排队RX被过早换代漏过**这一有直接红测的缺口。没有证据证明§20现场走的是头窗、真实pending条件失效还是Result尾窗；不能宣布717 ms问题已解决。

### 21.2 最小修正与未改变的合同

`ManagedMqttTransport`原单参数poll保持provider优先的普通行为；只有带本地`SessionRefreshGate`的合作路径，才在**同代、Online、provider确认旧凭据未过期**时，先用原transport预算的一部分服务当前MQTT，再查询gate并重新核有效性，随后推进provider和剩余MQTT预算。control＋预收＋剩余transport的**传入预算总和不增加**，但这不是native同步调用的wallclock保证。预收或gate查询后若stop／网络关闭，立即退出，不继续控制面。

`TaskCloudTransport`用即时回调替代早采样bool：在预收之后重新age原Session，仍只检查当前完整source、根generation和原1秒帧窗口。gate只在本次调用中借用，不存储callback、不新增对象成员、队列、任务执行器或等待计时器；原Session15秒、expiry、UNKNOWN及CAS合同不改。`LocalAccessCloudTransport`只薄传递同一gate，并在其自己的主动refresh前复查；普通LocalAccess路径不引入预收。删去本轮开发中的旧bool内部重载及其调用，未改公开Sketch的`Blinker.begin/run/enableTasks`接口。

Broker生产`task-carrier.ts`没有改动，SHA256仍`157835e6…8c47c80`。提前resolve早到的opaque Result会释放未完成writer，且Service仍有result-context／验签检查；本批不为了得到“成功”跳过这些fence、取消或授权检查，不自动重放，也不返回伪造的已验证回执。

### 21.3 有限回归、冻结与工具边界

1. 新queued-RX用例先在原源码退出1（`connects == 1 && pending`断言失败），不是编译错误；修正后`--renewal-order-only`的5条正／负顺序路径通过。
2. 最后补齐gate查询后的stop／network再检查，再用既有`build/qa3-device-store-host-20261031`增量编译／链接3个受影响MSVC host目标并运行，全部通过：`blinker_task_management_session_tests`、`blinker_http_device_key_session_tests`、`blinker_device_v2_tests`。未启动Arduino／Xtensa队列或重新编译固件。
3. Broker仅新增7个`TaskCarrier callback fence:`测试，固定既有Node22.22.3、当前TS内存转译后挂到原dist模块位置，7/7通过（TAP约820 ms）；不写dist、不build服务、不运行文件尾TCP集成。首次误用默认Node24因SQLite ABI137／127不匹配，在fixture前失败，后改用已安装Node22；没有安装或重建依赖。原stdout保留在会话工具结果，另存精确provenance，不冒称生成了原始日志文件。
4. cloud-login冻结v52只更新本批实际修改的Managed头／实现2项，其他冻结条目保持；全量架构检查通过，必要diff whitespace检查通过。没有再审或修改§19已经对齐的4项接线。

新增host测试已接入默认test main；选择器仅供有限离线运行，不隐藏负向尾窗。完整host构建／测试输出、红测断言、来源hash和原证据核验、Broker内存转译provenance，保留在库忽略目录`build/qa3-offline-order-20261101`的ACL私有子目录。原SDK3.3.11、App、服务及设备均未操作，不把host尺寸／链接当新固件资源或真机证据。

### 21.4 未收口项与下一步最小范围

§20的现场失败仍未关闭；板上仍是§19／§20的`818dfb45…98f4480`，**不包含本批新源码**。旧冻结stage／BIN不改、不冒充新候选；沿§20保留原TaskStore、scope、授权及全部未确认记录，正式准入保持关闭，UTC质量和Error8继续单列。无真机采样、配网／Start、发码、SDK升级、固件重建、部署、提交或推送。

下一步应先明确“设备本地发出→Broker收到→writer完成→Service验签及持久确认”各边界的最小可观察合同。当前没有设备内部gate／provider/apply／Result时刻，不能在缺证时反复跑无诊断候选；若要保证尾窗不被主动换代打断，必须明确**什么现有可验证事件允许释放该保护**，而不是把QoS0成功、固定等待或空日志当确认。本批不擅自扩展wire协议、扩大准入或继续修改尚未定位的分支。

## 22. QA-3：持久确认尾窗合同、最小接线及离线回归（2026-11-01）

### 22.1 四个完成点与唯一正向释放条件

本批承接§21的尾窗缺口，不重做§20真机、不重放已存请求。以下四点不可互相代替：

| 完成点 | 能证明什么 | 不能证明什么／连接保护 |
| --- | --- | --- |
| 设备 `cloud.send(Result)` 成功 | 原连接的本地QoS0发送接口接受了这帧 | 不证明Broker收到，不释放新协商的尾窗 |
| Broker收到原socket／sequence的Result | 当前物理来源交付了opaque结果 | 未验HMAC、未提交DB；原下行writer未完成时仍保持BUSY |
| Broker原publish callback完成 | 原下行writer fence可结束，Result可交Service | 仍无持久确认；新能力保留同socket、同proof的有界ticket |
| Service验签及事务COMMIT成功返回 | 同owner／root／attempt下的Result成为持久`verified` | 不是任务到期执行完成；只有后续精确确认被设备实际收到，才正向释放尾窗 |

权威持久点仍是Gateway repository的`acceptResult`：原事务内核对authority／proof／attempt、验Result HMAC、写attempt／request／scope，数据库`COMMIT`成功后才可把确认交给Service。COMMIT之后的过期／取消检查仍可能抛错，出现“API UNKNOWN但DB已verified”；不降级DB、不补发Commit、不宣称回滚。确认发送失败也不撤回已经成功返回的verified结果；`sent:true`只表示Broker本地writer完成，不假定设备已经收到了确认。

### 22.2 最小wire与单workspace接线

原bit25、op0～8及未协商端保持原语义。新增**独立协商**的`TaskServiceConfirmation`（bit26，必须同时有bit25、物理authenticated Cloud及212 B帧预算）；Runtime和Broker Hello都执行依赖检查。不是普通BBP Ack，也不是默认任务准入或新增执行操作。

专用op9 `ServiceConfirmed`继续使用`[1, operation, bstr]` carrier；inner必须精确32 B，flags=0、sequence必须是原Commit序号。token为：

`HMAC-SHA256(当前32 B DeviceKey, ASCII("blinker.task.service-confirm.v1", 不含NUL) || BE16(Commit sequence) || exact inner Result)`。

Result自身已经认证完整challenge／requestId／nonce／instance／scope／root／operation及结果；确认再绑定完整Result和原序号，不能只复制其摘要或旧HMAC冒充Service持久确认。两端共享独立vector（key为32个01、seq=1234hex、opaque测试输入83010203→`3e68e274…1269615`）；该vector不是合法任务请求或真机结果。

设备沿用同一个212 B工作区：原Result的WouldBlock重试只重试该帧、不重复Engine commit；本地发出后在合作点重新加载并核对完整当前双根，仅保留32 B预期token，进入`AwaitConfirmation`。native RX只做有界解析、原source／generation／sequence／root及constant-time token比对，无Flash、签名、回复或任务执行。原Session增加只读remaining观察，不增加Session对象、nonce、任务槽、存储键、线程或队列；TaskCloud仅增加剩余预算值并压实内部状态，原32位96 B静态上限不放宽。本批未做原SDK／32位目标编译，不将host验证冒充固件资源结论。

### 22.3 有界释放、writer围栏及失败语义

- **正向释放**：设备在当前根／物理连接及原期限内，实际收到同Commit序号的精确Service token。允许恢复主动续期，不发送确认的确认、不再次Commit。
- **未确认终止**：原Prepare Session实际剩余lifetime耗尽、单帧1秒界限、真实凭据过期、根变化、断线、stop／新Hello，或可信根加载／发送失败，结束相应保护并保留未确认语义。不把固定时间、`managementError=Ok`或本地bytes当远端成功。产品原Session为15秒，host夹具为1秒；不从Result发送时重新计满一轮，不延长凭据有效期。
- 凭据有效性沿现有provider合作点复查，不声称native同步收包内部逐字节expiry原子性。若恰在预收过程中到期，MAC仍只证明此前结果已持久化，不授权新执行；随后provider必须立即处理到期，不能被确认延寿。独立新增这一交错用例通过，未再重复其他已过顺序。
- Broker确认ticket复用原Commit exchange的**原3秒绝对截止**、同physical client／proof／connection generation／credentialVersion／sequence／exact Result摘要。与原pending合计最多8槽、每client单槽；未完成writer不提前返回或释放，确认只发送一次，不另择replacement socket。deadline／abort／close／撤销仍fail-closed；迟到callback不重放、不触碰新连接。
- Gateway只在事务成功返回`verified`、非historical replay且再次核对原上下文之后尝试确认。`rejected`不是verified，不生成成功确认；设备最多等完原剩余期限，不引入“拒绝即成功”捷径。DB失败、提交后过期、取消／换根／换连接均不发送成功确认。
- 正常确认完成无需额外release往返；失败路径finally最多一次有界内部只清理请求，使用精确原ticket、不继承已abort信号，不发MQTT、不自动重试。若撞上尚未完成的确认writer，必须cancel并终止原held writer；不能直接解锁让第二个writer插入。清理失败由原3秒timer兜底，不改持久结果或UNKNOWN。

内部token不可枚举、不进入App JSON，在发送／异常后清零；复用现有内部Broker鉴权、owner/CAS及只读协调，不新增公开管理入口。授权、TaskStore、scope、ledger和请求日志均不迁移或清除。

### 22.4 已完成的有限离线验证

1. 复用唯一既有host目录增量构建／链接：Session、HTTP DeviceKey provider、Device V2三目标及直接Task grant／budget两目标，**5/5通过**；新确认选择器同时通过。覆盖精确确认前续期保持、错token／序号／flags、原期限不续满、WouldBlock不重commit、根／真实expiry／网络／stop／新Hello／非法tick、根读取失败及回调中止、bit26依赖、旧bit25行为。没有升级SDK或启动固件队列。
2. Gateway新确认与client合计**13项通过**；原诊断／换代选择器17项（含子项）通过，新11条确认用例已接既有Service CI入口。覆盖持久提交前／失败不确认、提交后过期、replayed／rejected、确认与cleanup失败保留verified、取消独立清理、正常确认无冗余release。DB故障是repository端口模型，**没有运行真实PostgreSQL提交故障测试**。
3. Broker原7条writer callback围栏＋8条tail共**15项通过**：ticket／同连接／一次确认、全局容量、分配失败不悬挂、清理与unfinished writer竞态、迟到callback及replacement隔离。当前TaskCarrier TS内存转译挂到原dist模块路径，其他依赖沿用既有dist；无dist写入或服务build，不计作TCP／MQTT集成。
4. Broker和共享protocol的`tsc --noEmit`通过；库全量架构检查通过，未放宽冻结清单。§20原39份证据（104330 B）逐一重新核SHA256不变。首次新增client夹具因配置模块缓存失败，修正fixture独立所有权后通过；不把该失败藏成产品已验。

host原输出／来源hash／证据核验保存在忽略目录`build/qa3-tail-confirmation-20261101`的私有子目录；Service精确命令、source SHA及工具输出来源说明在`aily-blinker-services/build/qa3-service-confirmation-20261101/provenance.json`。后者是provenance，不冒称原始TAP日志文件。

### 22.5 状态与下一批边界

本批完成**源码级尾窗合同与有限离线接线**，不是§20现场717 ms根因已实证或真机已修复。板上仍是§19的`818dfb45…98f4480`，不含§21／§22；现有Gateway／Broker／App、账号、授权及UNKNOWN不变。正式准入继续关闭，UTC质量待补，Error8继续单列。无新Start／配网／采样、发码、SDK升级、固件重建、部署、提交或推送。

下一批最小顺序：先审阅并冻结设备＋共享协议＋Broker＋Gateway的完整消费者差异；如安排实际持久边界补证，先做隔离数据库的COMMIT／提交后失败回归。之后再单独批准匹配候选原SDK定向链接和一次只读差异验收；不能拿旧BIN验证新能力、重放旧UNKNOWN或反复Start补样本，不重新扩成资源审计。旧端bit25路径仍无持久确认尾窗保证，不能把兼容回退称为新能力已生效。

## 23. QA-3：必要消费者冻结与隔离PostgreSQL提交边界（2026-11-01）

### 23.1 审阅结论与冻结范围

已审阅§21／§22必要差异及真实消费者，**未发现需要继续修改生产代码的阻断**，本批不再扩大实现：

- 设备本批10项：Messages、DeviceRuntime、TaskCloud、TaskManagementContract头／实现、TaskAuthorityContract头／实现、TaskCommandAdapter头／实现、TaskManagementSession；并纳入§21门控的6项前置（Managed头／实现、LocalAccess薄转发、ISessionProvider及HTTP provider头／实现）。不改槽位、SDK或公开Sketch执行接口。
- 服务9项：共享task-management协议／类型；Broker TaskCarrier、CBOR已知位、Hello lane、内部routes；Gateway broker-client、TaskManagementService、TaskManagementRepository。额外记录未改的实际DI入口、database事务封装及PG迁移／Key／vault／签发依赖，不把“端口可用”当作实际入口已接。
- Repository禁止嵌套事务、SERIALIZABLE／maxRetries=0；Service确认只在真实COMMIT成功返回、非历史replay并完成原上下文检查后发送。Broker保留原socket／ticket／sequence／Result摘要、writer fence及3秒原截止，设备沿用原Session剩余期限。原生RX内expiry非逐字节原子性的支持边界不变。

库冻结记录为`tests/freeze/task_service_confirmation.json`，共19个输入（16个生产／前置文件、2个host回归及原login冻结）；服务记录为`services/gateway/test-fixtures/task-service-confirmation.freeze.json`，共102个必要生产／消费者／PG输入及回归、核验脚本。按**SHA256＋仅CRLF→LF标准化**冻结，未忽略代码、空白或include差异，不包含凭据、网络Key、私有备份和产物。

复用Node22运行只读核验：在服务仓执行`node scripts/check-task-service-confirmation-freeze.js --library-root ../blinker-library`，服务102＋库19项全部匹配；该脚本无自动更新功能，后续必要修改必须先审阅再显式更新对应冻结。原login v52清单未放宽，库架构检查通过。未触碰无关本地修改或Git暂存区。

### 23.2 隔离资源与真实数据库边界

本机Docker未启动。本批仅在现有测试服务器使用已有镜像创建**一次性PostgreSQL 14.24及Node测试进程**，没有构建镜像／服务或部署产品：

1. 独立PG容器`network=none`，不映射端口，数据全在独占tmpfs；测试Node仅加入该隔离network namespace，使用新fixture数据库／用户。两进程各192 MiB、0.5 CPU上限；未连接现有业务库或复制其账号、Key、scope、UNKNOWN。
2. 上传的只是限定源代码闭包、迁移及测试包别名，无`.env`、账号库、私备或固件产物；远端逐一核对91个输入SHA。复用原`setupPostgresTest`和实际全部迁移；临时P256／keyId1材料完全在夹具生成，不使用生产签名私钥。
3. 实际调用Gateway database、vault／Key repository、TaskManagementRepository／Service。独立`pg.Client`在确认端口中读取attempt／request，证明结果已对第二连接可见且`db.inTransaction()=false`；不是只把repository返回值mock成提交成功。
4. Broker／设备I/O仍为独立合法HMAC／P256 crypto port，不新增MQTT、HTTP或真机采样；因此本批是**真实PG持久边界回归**，不是新固件、Broker实网或COM11组合已验收。

### 23.3 五个新场景的结果

新文件`services/gateway/test-device-v2-task-confirmation-postgres.js`已薄接既有Service CI入口；只有显式配置`TEST_DATABASE_URL`时运行。每场景新建合成身份，只进行全新Authority／Commit，不含Start、旧请求重放或配置下发。

| 场景 | 第二连接看到的attempt／request | API与确认边界 |
| --- | --- | --- |
| 正常COMMIT | verified／verified，exact_result存在 | 返回verified；确认调用1次、清理0次；发送前已提交 |
| 延迟约束在COMMIT抛P0001 | issued／pending，exact_result为NULL | 返回UNKNOWN；确认0次、只清理ticket1次；事务结果不泄漏 |
| 真实COMMIT后、repository返回前到期 | verified／verified，exact_result保留 | TASK_ATTEMPT_EXPIRED→UNKNOWN；确认0次，不把DB降级或重发 |
| 真实COMMIT后取消 | verified／verified，exact_result保留 | TASK_CANCELLED_OR_EXPIRED→UNKNOWN；确认0次，不宣称回滚 |
| 确认投递失败 | verified／verified，exact_result保留 | API仍verified；确认仅尝试1次、清理1次，无Commit重发 |

**5/5通过**（Node TAP计父项共6项，0 skip），约1.8秒；正向确认使用独立createHmac核对exact Result／BE16序号，P256签名独立验证。真实repository在每例只派生一个确认MAC，所有成功／失败终态均检查清零，token不进入App JSON。每例保留一个**仅存在于该隔离夹具**的过期UNKNOWN，其原request／attempt所有字段逐字不变；从未读取或重放业务UNKNOWN。

两个准备失败均如实保留，不计作产品缺陷或通过：首轮source-only打包漏掉多行require引用的迁移，在进入用例前停止；补全扫描及迁移全集断言。第二轮UNKNOWN哨兵错误缩短expires_at触发真实CHECK约束，5例均停在setup；仅修夹具为保留原lease字节、推进其独立时钟，不放宽生产约束。随后显式新建独立夹具完成上述5例，未重复已通过的真机／host／Broker测试；无自动循环重试或旧请求重放。

### 23.4 核验、清理及证据

修正后91个上传输入与本地源码再次匹配，服务102＋库19项冻结复核通过；§20原39份证据（104330 B）逐一SHA256保持。原在运行的业务容器ID／镜像／StartedAt／running／restart count／health在夹具前后完全一致；未修改准入或服务配置。

每轮只删除带本批独占label、精确ID核对的两个临时容器；独立schema已正常清理（剩余0），临时PG数据随tmpfs销毁，上传目录／运行目录也按已验证的`/var/tmp/blinker-qa3-commit-*`精确删除。不删除任何业务数据、旧授权、日志或备份；被删除内容均为可重建的隔离测试夹具。

原始Node／TAP、wrapper stdout／stderr、镜像摘要、全部输入SHA及前后容器状态保存在服务仓忽略目录`build/qa3-confirmation-pg-20261101`的ACL私有子目录；`directory.json`、`directory-confirmed-closure.json`分别指向两次失败，`directory-fixed-sentinel.json`指向本轮通过结果。失败原记录没有覆盖，不把provenance当作原始日志。冻结记录及核验器为必要交付源码；私有日志与source-only测试归档不提交。

### 23.5 下一批最小范围

§22列出的**消费者审阅／冻结与实际PG COMMIT补证已完成**；本批无新增生产修正，不再重复这批或重开资源审计。下一批只需核对冻结输入及原profile匹配条件，再单独安排原SDK定向固件链接、匹配服务候选和一次全新只读差异验收，观察确认前后主动续期与真实Service持久点；不要再次Start或用旧BIN代验bit26。

本批遵守暂不重建固件、部署或提交推送；无配网、发码、真机采样、SDK升级或扩槽。板上仍是`818dfb45…98f4480`，不含§21／§22新接线；§20现场717 ms／UNKNOWN尚未实证销项。正式准入继续关闭，UTC质量待补，Error8单列；现有App、账号、授权、TaskStore／云scope、UNKNOWN和完整业务日志保持。

## 24. QA-3：匹配候选完整链接及一次只读续期差异验收（2026-11-01）

### 24.1 冻结输入、原SDK链接与有界观测

承接§23，仅执行一次新只读验收，不重复Start、配网或资源审计。库19＋服务102项冻结核验通过，原login v52不放宽；本批**未继续修改生产源码**。复用当前App、同一COM11身份、原四槽SoftAP／双根组合及TaskStore／云scope，不创建或迁移身份。

原Core **3.3.11**、`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`定向完整链接通过。BIN **1,437,360 B**，SHA256 **`f8783fbe6c66144af2acfb4184e94523483fda007ff8de5ee12cef6ba835eaee`**；compiler Flash 1,437,208 B、静态RAM 60,108 B，app offset `0x10000`／capacity 3,342,336 B。同一产品实例、四槽、DeviceKey＋Admin、TaskCloud消费者及bit25／26接线核对通过；不据此新增动态资源准入结论，不改SDK、分区或槽位。

为区分“Broker本地发送确认”与“设备实际收讫”，只在忽略的私有stage增加诊断：每boot需USB串口显式arm一次，固定32条**总量饱和**记录；native回调仅记定长事件，合作loop按UART实际可写字节输出，每轮最多2行、每行不超过48 B。只含事件、设备millis、sequence、剩余预算及generation，无Key、MAC、坐标或任务正文；不发任务、不改变执行链。服务候选只在私有编译输出加同样有界、默认关闭的关联点，不把诊断overlay当作发行源码。

准备问题保留，不计作产品通过：最初私有trace采用96 B可写门槛，与原SDK 64 B TX缓冲不匹配，停止本批拥有的唯一CLI进程后，只修私有输出门槛；沿同一build缓存、jobs=1显式续编，约995秒退出0。旧prepared摘要因此被原审计拒绝，随后用新checkpoint对**同一现有ELF／BIN**完成消费者审计，没有在成功后另编一轮。原意图、失败／编译日志、两个checkpoint和补审记录均保留。

### 24.2 实际profile、部署及单ID范围

运行Gateway仍为P256、keyId **1**，实际SPKI SHA256 **`02879e20a6ad6e021ed7e6c857f0a351db14b1568c467db519aac09920b6d061`**，与候选匹配；共享协议、Gateway实际DI及Broker实际Hello／carrier消费者都进入镜像，固定MAC vector与导出类型核对通过。原refreshAfter **480秒**／TTL **600秒**及Session **15秒**不改，不用延长等待或更密轮询取得成功。

匹配Gateway镜像 **`e209b6b2…37e35b`**、Broker镜像 **`14a2a8e5…1db5`**已部署；保留原Auth、签发私钥挂载、数据库、Redis及其他配置。部署前两镜像实际已构建完成，首次smoke因dotenv前缀不能整段JSON解析而停止；显式续步复用这两个现成镜像，按固定前缀解析后核验通过，**没有重复构建镜像或重试业务请求**。准备失败原输出保留。

COM11仍为`device_b60437fc-dfe2-4745-91cc-57efbf6bf024`、instance `215af06bebe01744006b9348d9a5e40c`、scope `6d8abd98bf4d8dafdc813643c6127caa`。只在备齐前置后临时开放此ID；没有开放Ed25519名单、账号额度或其他设备。只写app区域，原20 KiB NVS及分区前后逐字核对。App沿用已装beta **`7e88a390…14a0f0`**，没有重复构建／安装、登录或发码。

### 24.3 一次自然续期附近的只读结果：尾窗通过，整页未通过

无基线任务请求，等待原续期节拍后只进入现有任务页**一次**。该页自然产生两次不同用途的catalog HTTP；第二次是分页，不是首个请求重试：

| 请求／完成点 | 实际证据 | 结论 |
| --- | --- | --- |
| 首次catalog `oITJ…`／`a084…` | HTTP 200，`fresh`／`verified`；capacity 4、revision 6、count 1；原Commit seq5／MQTT g2 | 新读取结果真实验签持久化，非旧缓存／replay |
| 本地Result→Await | 设备millis 487804→487806；沿用Commit时保存的14,866 ms预算，并按原receivedAt继续扣时 | 该值不是Await当刻剩余值；不从Result重计15秒 |
| Service确认→设备收讫 | Broker同seq5发出一次；设备487934实际匹配精确MAC，gate同刻1→0，之前无expiry／reset | **正向释放由真实确认触发**，不是本地QoS0、固定等待或空日志 |
| 续期应用与新连接 | 487944应用provider generation2；489577新Hello进入MQTT g3，仍协商bit25／26 | 确认后才主动应用；不把服务端提前签发proof等同设备已应用 |
| 第二catalog `KEV…`／`2845…`，index0 | HTTP 503 `TASK_DEVICE_OFFLINE`，其request／attempt均未进入ledger，未向设备再发carrier | 重连窗口的连接检查拒绝；**整页列表及任务记录读取仍未通过**，不是新UNKNOWN |

同一设备时钟：local Result→精确ACK **130 ms**、Await→ACK **128 ms**、ACK→gate释放 **0 ms**、ACK→apply **10 ms**、apply→新Hello **1,633 ms**。同一服务时钟：Broker收到Result→Gateway持久调用成功返回 **17 ms**，该返回→确认send **9 ms**，确认send→writer完成 **1 ms**。`durable-result`是repository COMMIT成功后的返回点，不冒称SQL实际COMMIT时间；`original-writer-complete`是观察到fence／ticket成立的点，不能反推更早publish callback的精确时刻。

App/CDP自身时钟测得两次请求至`responseReceived`分别 **1,161.606 ms／105.899 ms**，不是完整body读取或UI可用耗时。手机、本机、服务器存在已记录的时钟偏差，**不跨时钟直接相减**或拿串口host接收时间当设备事件时间。完整seq／proof／request关联在私有原日志，不公开凭据。

第二读失败后页面显示容量未确认、云端不可用及显式“重新查询”；观察期内无自动查询。未为获得成功再点刷新、Start或补跑；结束返回首页并移除观察器。只证明本次尾窗条件真实成立，不能把本次首读成功改写为§20旧UNKNOWN成功或“所有续期问题已关闭”。

### 24.4 数据保留、自然只读retention及收尾

1. **20 KiB NVS、instance／双根／授权、TaskStore及分区逐字不变**。原任务仍revision6／count1、停用Completed、progress3／startSequence1；本批没有Start或任务写入。板上保留本次已核验匹配BIN，结束readback／hard reset后trace默认未arm；不声称又做过一次业务恢复测试。COM9及其他板未操作。
2. 7条旧verified request及7条attempt逐字段不变；scope的ID／owner／instance／根／状态／replay-floor保持，仅正常`updated_at`更新。设备、tenant、route、Key及Direct授权快照保持，无scope搬迁或旧请求重放。
3. **必须单列retention例外**：旧只读请求`3af1…`在前快照为pending／issued、`exact_result=NULL`且已过期约660万ms。新读进入时被原repository的**读请求过期60秒保留策略自然回收**，不是本批主动删除、resolved或执行成功；完整旧request／attempt原字节和§20日志已留存。不能概括为“所有旧ledger记录逐字未变”。其他UNKNOWN摘要不变，本批第二读没有新UNKNOWN；不恢复数据库旧行或补造成功。
4. 临时准入及诊断均已关闭：Gateway三名单、Broker名单为空，诊断环境变量恢复原缺省；健康检查四角色通过，PostgreSQL／Redis容器ID不变。保留本批匹配Gateway／Broker镜像和COM11候选，不恢复到缺少确认能力的旧镜像；最终配置除已批准匹配镜像外保持原值。
5. App原7项secure及两个完整journal store逐字保持，最终首页、后台观察器／CDP／串口／SSH均回收，手机原stayon=0恢复。云端完整限定日志、数据库前后快照、部署／配置核验等**84文件、1,205,537 B**已逐项SHA256拉回私有目录；不含签名私钥或配置秘密导出，不提交产物。

证据入口分别为忽略目录：库`build/qa3-tail-link-20261101/handoff.json`、`build/qa3-tail-live-20261101/directory.json`；服务`build/qa3-tail-runtime-20261101/directory.json`及其私有`acceptance-result.json`／`collected-manifest.json`；App `build/qa3-tail-live-20261101/directory.json`。独立复核与自动artifact核验结论一致；失败原记录均保留，不用摘要代替原始日志。

### 24.5 当前结论及下一批最小范围

本批完成**原SDK完整链接、匹配部署和一次新只读差异验收**；精确持久确认前保护、确认后释放及续期应用这条路径有限通过。**QA-3整体仍未关闭**：分页第二读在重连窗口报`TASK_DEVICE_OFFLINE`，完整任务页未恢复；UTC质量待补，正式准入继续关闭，Error8继续单列。本批不扩槽、不升级SDK，无发码、配网、Start、旧请求重放、提交或推送。

下一批仅离线分析现有证据中的**第二catalog与续期重连的连接注册／Hello／订阅就绪时序**，核对OFFLINE分类、已确认首读结果保留及显式手动恢复合同，先明确最小修正再实施。不要直接延长固定保护时间、自动重发读／写、调低轮询或立刻补跑一轮真机；不重开已经通过的COMMIT、尾窗或资源审计。

## 25. QA-3：重连分页离线分析及手动恢复修正（2026-11-01）

### 25.1 离线证据能确认的原因与边界

仅分析§24已保存的84份服务证据、App/CDP及串口记录，不联网读取业务数据、不重放旧请求。旧证据逐项SHA256复核保持；没有新真机样本或部署。

1. 两次HTTP都是catalog，但首个为`index=null／fingerprint=null`，第二个为`index=0`并携带首读返回的同一fingerprint，requestId不同：**第二个是后续目录页，不是自动重试**。同一CDP时钟中首读`responseReceived`→第二次请求为 **558.571 ms**；原`HttpTaskApi`响应后550 ms发送节拍解释该间隔，不把它计为服务端等待。原节拍本批不改。
2. SDK `ManagedMqttTransport::applyCredentials()`原本就先`stop()`旧MQTT，再configure／start新连接。§24精确Service确认已结束的是**第一个管理RPC**的保护，不是整个App多HTTP `snapshot()`的读事务；当前没有跨目录分页的连接租约。因此确认后主动续期可与下一页相撞，不能推导“已确认一页便保证全页一直在线”。本批不添加跨页租约或固定保护等待。
3. Broker `TaskCarrier.connection()`只有在`getPhysicalV2Connections()`返回0候选时抛`TASK_CARRIER_OFFLINE`；Gateway在`repository.prepare()`之前将其映射成503 `TASK_DEVICE_OFFLINE`。该registry筛选同tenant、未过期及真实物理client，**不筛Hello或订阅就绪**。唯一物理连接存在但Hello／下行订阅未齐，应走`NOT_READY`；多候选为`AMBIGUOUS`，不任择新连接。
4. 新proof在服务端于`1793504468761`签发，早于旧Result收讫`1793504469157`；这只是凭据注册到数据库，**不是MQTT物理注册**。实际`registerV2Connection()`在MQTT authenticate成功分支；Hello再协商能力，下行订阅分别核对。旧proof当时仍active且未过期，不能把本次失败归为凭据到期、换根或任务拒绝。
5. 本次错误及第二请求未入ledger，足以确认**当次Broker未找到有效物理候选，读取在发往设备之前失败**。后快照已有新context和新鲜State，证明后来恢复。WARN日志没有成功authenticate／注册／Hello／订阅逐事件时间，故**不能进一步确定503瞬间究竟停在新authenticate之前、client关闭／移除等哪个子点**，也不能用不同进程／设备时钟反推。没有证据要求修注册表、放宽Hello或订阅检查。

结论：这是每RPC正确释放之后，多请求分页撞上实际MQTT换代的**暂时不可用边界**，不是本批首读未持久化或新UNKNOWN。最小范围不改Broker分类、SDK保护、CAS、轮询／发送节拍或执行链；不延长等待、不偷偷重试第二页。可复现的App缺陷是：首读容量提示被后页失败整体抹掉，以及失败后前后台切换会再次`open()`自动查询。

### 25.2 已实施的最小App修正

只改既有`DeviceTaskClient`及页面级`TimerService`，无新缓存、队列或后端执行器：

- `snapshot()`增加可选、同步的**仅容量观察**：只有原合同校验后的`fresh` catalog metadata、同account／epoch及未取消signal，才通知本次页面一次；通知后再次核对signal／account。不会把半份targets、任务表revision或记录交给页面作为可写数据，也不放入能力缓存。后页失败仍抛原错误并清除不完整目录；下一次显式读取仍从新metadata开始。
- 容量观察仅是页面当前读的展示提示，和原可写`capacity`分开。仅四种既有连接错误可保留它：`DEVICE_OFFLINE／CONNECTION_NOT_READY／CONNECTION_CHANGED／CONNECTION_AMBIGUOUS`；仍需本账号的owner入口有效。显示“设备容量 N 条；任务列表尚未读完”，**ready=false、可写capacity=null、revision=0、records／targets为空**，不显示“当前没有任务”，不允许创建／编辑／Start或核销UNKNOWN。
- 原`refresh()`开始、离页、新账号／epoch、权限／合同／UNKNOWN失败会清除该提示；迟到传输错误先复查账号，不能让旧账号容量留在新账号页面。未缓存已读任务进度，完整恢复后仍按新鲜目录锚点读取records并通过原revision／count／重复ID校验。
- 抽出原取消动作`stopView()`，失败后后台仍abort并释放controller、busy及阶段；前台只核原账号／owner入口，**不再自动inspect、snapshot或重提请求**。返回后的“重新查询”建立新signal，从头显式读取；反复后台／前台也保持此合同。离页、后台期间静默账号／入口变化均清除提示，不把旧取消signal复用给手动请求。
- 同样处理`open()`首次本地journal读取失败：不能因为还没进入snapshot便在返回前台自动重读存储。恢复原请求按钮只在前台且存在有效controller时开放；本批未改变其“显式确认后可能执行”的语义。
- **保留原中断写只读协调**：开始显式mutation时重置旧的读取失败标志。正在写入时被后台中断，仍按原合同保留UNKNOWN并在恢复时仅新鲜读取，不能被“此前失败不重试”的门禁错误拦掉，更不会自动重放写入。

复用既有任务摘要、错误`role=alert`和“重新查询”按钮，没有新增页面、样式、权限、API或任意数据观察端口。UI/UX指引仅用于“已确认部分与未读完状态分开、明确手动恢复”的反馈，不扩大视觉改版或推断设备成功。

### 25.3 有限离线回归与冻结核对

1. 新选择器在原实现出现**4条预期语义失败、1条保护通过**，缺失容量观察／提示及手动恢复边界可复现。首轮直接运行Vitest未带项目globals且误匹配build备份，停在测试收集；随后复用既有UI runner配置并明确排除build，不将工具准备失败算成产品红测。
2. 修正后App **17项相关回归通过**，覆盖容量窄观察、historical／取消／账号拒绝、观察后立即取消、部分读取失败及手动恢复、静默epoch／owner入口丢失、UNKNOWN原journal不变；并核对原新鲜进度读取、CAS／count／重复ID、写确认与后续读取分离、后台中断写等受影响边界。独立审阅发现首次`inspect()`失败漏口，最小补线后只补其**1条新测试通过**，未为此重复17项。
3. Broker只加 **1条顺序合同测试并通过**：新凭据签发仍使用旧物理context→旧连接关闭为OFFLINE→新物理注册但无Hello为NOT_READY→Hello完成但无订阅仍NOT_READY→订阅后才有新context；旧disconnect不能删除新client，原context不能在新socket偷偷发送。实际当前registry／TaskCarrier TS在内存转译后挂到原dist模块位置，其他依赖沿用既有dist；无dist写入、Broker build、MQTT联网或重做尾窗测试，不冒称真实握手时间补证。
4. 原始App `tsc`遇现有安装WiFiProv包的旧dist声明而失败；保留日志，未改依赖／锁文件或编译插件。复用既有源码映射的App配置，加仅包含两份本批spec的类型配置，**App及有限回归类型检查均通过**。不把此映射说成默认安装包声明已经修复，也没有构建APK。
5. 服务冻结102项中**只显式更新已审阅的Broker新增测试摘要及review说明**；其余输入、库19项及原login v52不变。只读freeze checker通过。设备、共享协议、Gateway、Broker生产文件本批均未修改，不能将旧Git dirty差异归成本批变更。

命令、当前输入SHA和原始stdout／stderr在服务忽略目录`build/qa3-pagination-offline-20261101`：`run-BnAh8Q`保留Broker／App通过及原类型检查失败；`resume-UNLosa`为显式补充测试、源码映射类型检查及freeze通过结果。前期红测及工具失败的原输出在会话工具记录，另有来源说明，未伪造为捕获文件。App有限类型配置在忽略目录`build/qa3-pagination-offline-20261101`，不进入发布包。

### 25.4 当前停点及下一批边界

本批完成**离线原因分类、最小App修正与有限回归**；不把未观测的握手子阶段说成已定位，不改§24“整页真机未通过”的结论。现装App尚不含本批修正；设备、服务、NVS、身份、授权、TaskStore、云scope、UNKNOWN及完整业务日志均未操作，原准入保持关闭，UTC质量和Error8继续待补。无发码、配网、Start、真机采样、SDK升级、部署、提交或推送。

下一批若安排验证，只需匹配App构建／安装及**一次只读页面失败→后台→前台不自动请求→手动恢复**的短差异验收；沿用现有服务／设备，必要单ID准入单列，结束关闭。无需再重编设备SDK／服务、重跑自然480秒续期、Start或资源／COMMIT／尾窗矩阵，也不要求为了凑相同OFFLINE再制造设备断线。完整分页／任务进度恢复必须有新鲜结果才可销项；正式任务准入本批不开放。

## 26. QA-3：匹配App安装及只读分页失败恢复真机有限收口（2026-11-01）

### 26.1 保留记录、匹配构建及单ID前置

本批按§25只补App恢复差异，不再等待480秒续期、制造设备断线或重做配网／Start。先保留本机App原**7项secure、2个完整请求journal store及logcat**；原账号及COM11详情读取均200。电脑仍在“带翅膀的蜗牛”，手机ADB在线；仅唤醒／解除系统滑动遮挡，保留原stayon=0，未登录或发码。

复用现有cloud-beta的prebuild→源码映射ng→sync→assembleBeta，**单队列一次构建及安装**：Node22.22.3／pnpm11.19.0／esbuild0.28.2／原WiFiProv SDK **2.4.4**，Gradle offline、单worker、原签名。沿当前工作树及相邻WiFiProv TS／Android源码，不使用旧stage；补齐插件**21项**源码／Manifest／IPC／配置输入摘要，原SDK JAR及Gradle init固定摘要保持。各阶段App源码、HEAD、index和原native输入前后不变，无依赖／锁文件升级。

新内测APK SHA256 **`10a6a218d128d8b97027b55456412672602b29141fa24f83d623f53849fdffba`**；包名`iot.diandeng.tech.beta`、证书`a76639ec…5d210`、HTTPS／cleartext=false／logging=none及原生注册表通过，**1157个**打包Web资源与本次www／同步资产逐字匹配。§25两份runtime源码hash及新任务chunk `580cf156…8b92`另行核对，确认不是旧App。普通安装包WiFiProv旧dist声明问题仍仅按既有源码映射处理，不冒称本批修复默认依赖。

沿用原Gateway／Broker镜像、P256／keyId1及SPKI `02879e20…d061`，同owner／instance／Admin／原scope成立，原任务ledger8条verified。只临时开放`device_b60437fc-dfe2-4745-91cc-57efbf6bf024`的既有名单，同镜像重建必要Gateway／Broker；不新建身份、scope或设备，不改Key／授权，诊断保持关闭。COM11及其他板**未开串口、未刷写、未做NVS或任务写入**。

### 26.2 一次窄故障夹具与真实恢复的边界

为验证§25指定错误分支而不重复续期长循环，仅在本次真实beta WebView的CDP Request阶段，拦截**一次**精确COM11任务POST：`operation=catalog／index=0`、fingerprint必须等于刚刚真实fresh metadata，唯一新requestId。返回带本地fixture标记、no-store及精确CORS合同的503 `TASK_DEVICE_OFFLINE`；该请求**未发往Service或设备**，随后立即关闭Fetch拦截。

这是明确标记的**手机本地失败夹具**，不是新的服务端OFFLINE、MQTT握手或持久确认尾窗证据，不伪造成功、ACK或ledger。首metadata及后续显式手动完整分页全部走原真实Service／COM11；预检OPTIONS正常通行，未替换fetch／XHR、调用私有Angular方法或改产品代码。匹配不成即停止，不扩大拦截或自动再试。

| 阶段 | 实际观察 | 判定 |
| --- | --- | --- |
| 原任务页进入一次 | 真实metadata 200／verified／fresh，capacity4、revision6、count1；唯一index0命中本地503 | 正常首读确认与本地失败分开，非Service失败补样本 |
| 失败页 | “设备容量4条；任务列表尚未读完”，0任务行，无新建／编辑／Start入口；重新查询可用 | 已确认容量不撤回，未完整表不冒充空表或授予写权限 |
| HOME后台→同进程返回原页 | hidden false→true→false；失败后至返回截图task HTTP始终 **2次**，提示不变，无新请求／重新导航 | 未自动inspect／query／重发；原取消signal未阻塞后续手动入口 |
| 点击原刷新一次 | 拦截已移除；新增真实metadata、target、record **3次HTTP200／verified／fresh** | 显式新读取，不重用失败请求或旧表 |
| 完整列表 | “已用1／4条”，原10秒停用Completed任务1条；Start仍禁用 | 新鲜目录／完整任务记录真实恢复，非缓存／伪造结果 |

浏览器总共**5次任务POST：4次真实服务读取＋1次本地失败夹具**。所有真实请求均为catalog／Read，没有mutation、Start、初始化或旧UNKNOWN重放。手动读取records携带 **`expectedRevision=6`**，结果同revision6／count1；原定义停用、progress3、startSequence1保持。CAS本批只核**半表禁写UI／源码围栏及新鲜读取修订锚点**，没有发送stale revision写请求，不能冒称新增真实CAS写拒绝测试；原负向继续沿用有限离线回归。

### 26.3 真实持久记录、UNKNOWN与资源收尾

1. 服务两表核对：4个真实新Read request／attempt均verified且有exact_result；本地夹具`GAja…`两表均**无行**。scope仍原ID／owner／instance／根／状态／replay-floor，正常updated_at可更新；device／tenant／route／Key及Direct授权投影保持，没有新UNKNOWN或旧请求协调／重放。
2. **7条旧verified request及attempt逐字段不变**。§24的旧已确认Read `a084…`在本批前已过期，匹配attempt expires_at=`1793504483695`、前快照at=`1793508150819`；本批新读按既有60秒只读retention自然回收它。因此总数8→11（保留7＋新增4），不是所有旧行不变，也不是UNKNOWN resolved；完整旧行／回执字节已在前快照保留。其他UNKNOWN摘要保持，不恢复或补造旧数据库行。
3. 结束已关闭临时单ID：原三Gateway名单及Broker名单恢复为空；配置精确恢复 **`e11252c9…c48`**，原镜像、签发材料／挂载及PostgreSQL／Redis容器保持，四角色健康。开通／关闭操作自身前后完整任务ledger保持；关闭准入不删除scope、撤销任务或更改设备任务区。
4. App验收前、安装后、UI结束及终核**7项secure／2个journal store深度一致**，新内测保留。离页动画完成后稳定首页、可见任务页0；后续被动观察任务查询0。一次性Fetch拦截、Network监听、CDP转发及SSH均回收，原stayon=0恢复；不把动画中短暂可见的旧任务页当首页终态。
5. 限定云端完整日志／数据库快照等**57文件、626172 B**拉回ACL私有目录并逐项SHA256核验；远端原配置保留root-only，不导出私钥或env。原失败页、后台返回、手动恢复截图、完整CDP请求／响应及logcat均保留，无请求记录制造。

两项工具准备问题均保留原输出，未导致业务或构建重试：附加APK产品标记最初用字面UTF-8找中文，但生产JS转义了中文，改为只读规范化搜索后对**同一个已构建APK**核验通过；离线artifact审计最初误在request而非匹配attempt行读取expires_at，仅改审计字段后通过。没有重编、重新安装或再跑一轮真机。

证据入口：App忽略目录`build/qa3-read-recovery-live-20261101/directory.json`，其私有`07-screenshot.png`为返回后失败态、`10-screenshot.png`为真实恢复列表；服务`build/qa3-read-recovery-20261101/directory.json`及私有`acceptance-result.json`／`collected-manifest.json`。原始失败、完整记录及源码／产物摘要均保留，不进入Git。

### 26.4 当前结论与后续顺序

**§25的App只读分页失败→前后台无自动查询→手动完整恢复，真机差异有限通过，可冻结，不再默认重跑。** §24的真实尾窗结果与本批本地失败夹具分开；本批不新增服务端OFFLINE／握手证据，不修改历史失败或旧UNKNOWN结论。

QA-3整体仍未开放：**正式准入继续关闭，UTC质量待补，Error8单列待定位**。下一批先梳理UTC质量既有证据与最小补证前置，再决定QA-3剩余验收／准入；不重开本次App、已过配网／Start／COMMIT／尾窗／资源矩阵。现装App已匹配，服务／固件沿用；无发码、刷板、扩槽、SDK升级、任务写入、提交或推送。

## 27. QA-3：UTC既有证据核对及当前组合只读质量补证（2026-11-01）

### 27.1 既有证据与本批最小范围

问题不是“尚未实现授时”。`DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md` §9.2已记录同MAC普通WiFi的首次样本、离线holdover及原300秒刷新；§16.1／16.2记录旧WiFiProv＋BLE／Plain开发组合的Server来源及不确定度。它们不属于本轮**原SoftAP＋HTTPS/MQTTS＋P256／keyId1＋DeviceKey/Admin双根＋四槽**组合。§17仅完成过monotonic倒计时，§24 Q3串口又只有尾窗事件；TLS在线、服务器已发送TimeResponse、App时间或`uptimeSeconds`都不能替代设备已接受UTC的证据。

本批仅补**当前组合中原业务时钟的已接受样本、来源／state／age／uncertainty及同clock推进**，不重测Start、日期／星期执行、断电、480秒续期、配网或App失败恢复。正式准入不需临时开放；没有任务定义、任务管理请求或新scope。

### 27.2 实际授时及质量合同核对

- `Blinker.enableTasks(power)`沿原路径调用`enableTime()`；`Blinker.timeStatus()`读取原`TimeSyncService`，同一`ITimeSync`注入任务`TaskTimeEnvironment`，后者用同一`observeTime(snapshot, observedMillis)`取得一致UTC／单调时间。没有新增时钟owner、绕过任务质量门禁或用App时间覆盖设备时钟；返回false不等于“有效UTC=0”。
- 原TimeSyncCoordinator核对认证Cloud peer、transport／session／generation、sequence、nonce及RTT≤2000 ms；接受时取服务端UTC＋半程RTT，不确定度增加向上取整的半程RTT。原WallClock最大age **3600000 ms**、最大uncertainty **5000 ms**、漂移 **1000 ppm**保持。超龄／超预算进入Stale，不能当日期任务可用；monotonic倒计时与此分开。
- 实际运行Broker的`dist/protocol/v2/time-sync.js` SHA256 `5bcfa6ba…792d`核对为当前physical authenticated session／Hello能力／sequence／nonce路径，使用`Date.now()`及**声明1000 ms**上游不确定度；原5秒限流、设备首次可请求／远端或未知失败30秒／正常300秒刷新均未修改，本地`WouldBlock`仍为原1秒退避。任务准入名单关闭不禁止原TimeSync能力。
- 云端前后`NTP=yes／NTPSynchronized=yes`、时区Asia/Shanghai；实际Gateway仍P256／keyId1、SPKI `02879e20…d061`。NTP状态与1000 ms声明属于运行合同，**不是绝对精度鉴定**；不跨减手机、主机与MCU时间来冒充RTT或时钟误差。

### 27.3 最小测试接线、有限回归及唯一原SDK链接

当前匹配Sketch没有外部UTC读取口，因此新增`tests/internal/support/UtcSnapshotSerial.h`及一份host测试，CMake仅注册新目标；**生产SDK／协议／App／Gateway／Broker源码均不改**。测试薄适配只复制原快照，不拥有采样、时钟或任务：每boot最多两次显式`T`，每笔分两条≤63 B短行，固定seq关联；对象40 B、待输出不覆盖、容量不足不丢行，短写即停止诊断且不重传。私有Sketch每轮先原`Blinker.run()`，只在现有UART可写容量足够时排出短行，不加固定等待／flush或自动查询。

原host Debug树仅构建并运行新目标`blinker_utc_snapshot_serial_tests`，**六个合同通过**：无效与UTC0分开、原TimeSyncService接收合法样本、原Stale策略、窄buffer／精确写出边界、pending及两次上限、极值短行。原portable core依赖随目标重编，不重复旧测试；工具原输出及来源说明保留，`host-provenance.json`是摘要而非伪造原日志。

复用原私有候选生成脚本，移除旧Q3诊断overlay、仅接上述只读观察；保留公开profile、原非空PoP、两属性Manifest／处理器、双根、TaskStore及实际P256公钥。**唯一队列／单worker／原Core 3.3.11**完成定向完整链接及消费者审计：

- FQBN `esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`；BIN **1437152 B**，SHA256 **`69bc5fafbeaef159a57917a53d471a6de58399cb68db3b06ed34f79a8fe387d5`**；compiler Flash 1437004 B、静态RAM 59628 B。
- app offset`0x10000`／capacity3342336 B，唯一产品实例；`TimeSyncService::readTime／observeTime`及`TaskTimeEnvironment::observe`实际消费者成立。没有SDK／分区／槽位变动，不重开动态资源审计，也不将测试BIN发布为普通发行包。
- 服务102项＋库19项原冻结输入在本批后只读核验通过，没有为测试薄适配改写生产冻结或覆盖旧stage／BIN。

### 27.4 COM11两次真实只读观察

每次工具操作先核COM11／MAC及VID303a/PID1001；新鲜读取并verify原分区及20 KiB NVS，核原instance、Admin根、BoundSnapshot revision6／count1及任务blob SHA `06650c53…8eecf`，verify当前`f8783fbe…eaee`应用后，才仅写临时app。未擦NVS、重配、更新身份／Key或开放任务管理。

| 显式读取 | valid／state／source／revision | MCU before＝after | UTC ms | age ms | uncertainty ms |
| --- | --- | --- | --- | --- | --- |
| T1 | true／Synchronized(1)／Server(1)／1 | 18489 | 1793510783175 | 12249 | 1078 |
| T2 | true／Synchronized(1)／Server(1)／1 | 27735 | 1793510792421 | 21495 | 1087 |

两笔均为原认证Cloud来源的已接受样本，age／不确定度低于原质量上限；**UTC差＝age差＝monotonic差＝9246 ms**，UTC-age锚点同为1793510770926。四条原始行按`UTC1/UTC2`同seq1、再同seq2完整配对（27／32 B），无reader error，UART已关闭；host接收时间没有代替MCU采样时间。两次调用窗口为0 ms仅表示该次毫秒分辨率，不推出最坏执行预算。

结论是**当前组合UTC接受／来源／质量及同clock推进有限通过**，不是日期／星期到期回调、未同步启动、1小时过期、断网holdover或绝对授时精度的新验收。不因两次正常就改5秒质量阈值、刷新频率或声称真实误差≤1087 ms。

### 27.5 精确收尾、证据及后续停点

1. 关闭UART后新读NVS并verify，原完整20 KiB逐字相同；只恢复原匹配BIN **`f8783fbe6c66144af2acfb4184e94523483fda007ff8de5ee12cef6ba835eaee`**并readback，NVS／分区再次verify。instance、授权及原停用Completed任务／TaskStore保持，云scope未搬迁；没有NVS或任务写入，COM9等其他板未操作。恢复后仅read-MAC／hard-reset启动，不补跑业务或伪称又测过UTC。
2. 服务全程没有部署、切名单或改配置；原Gateway／Broker镜像、四角色容器ID、签发材料及配置SHA **`e11252c9…c48`**保持，前后四角色健康。**scope1、11条request及11条attempt全部逐字段保持**，本批没有新增业务任务请求、retention例外或UNKNOWN协调／重放；其他UNKNOWN摘要同样不变。
3. 原App APK `10a6a218…fdffba`保持，不构建／安装；账号7项secure、2个完整journal store前后逐字相同，完整logcat保留，最终首页／CDP转发关闭、原stayon=0恢复。没有登录、验证码、自动重试或自动重发。
4. 云端完整限定日志、数据库前后快照及时间运行证据共**21文件／1023135 B**已拉回ACL私有目录并逐项SHA核验；无env、私钥或配置秘密导出。候选、原始串口、编译／主机回归来源、失败输出及恢复记录保留在忽略目录，不进入Git。一次只读Node摘要命令的括号笔误只影响工具打印，已对同一产物修正读取；没有因此重编／重刷或重采样。

证据入口：库`build/qa3-utc-quality-20261101/candidate-directory.json`及`host-provenance.json`、`build/qa3-utc-live-20261101/directory.json`及私有`acceptance-result.json`；服务`build/qa3-utc-quality-20261101/directory.json`及`collected-manifest.json`；App `build/qa3-utc-quality-live-20261101/directory.json`。独立只读审阅及最终artifact审计均通过。

**UTC本项有限收口，不再把“当前组合是否接受UTC”列为未知。正式准入仍按本批要求关闭，Error8继续单列待定位，不冒称QA-3已全部开放。** 下一批仅依据§17／§24／§26／本节既有证据收敛四槽组合的支持边界及逐设备准入决策；日期／星期真实执行等未覆盖项应明确标NOT_RUN，再决定是否属首发必要条件，不自动扩成新测试矩阵。无需重跑配网、已验倒计时、尾窗、分页恢复或资源审计。本批不Start、不发码、不重放、不扩槽、不升级SDK，未stage／提交／推送，保留无关本地修改。

## 28. QA-3：四槽支持合同、剩余准入条件及最小日历补证计划（2026-11-01）

### 28.1 首发必要项的明确决定

**分开两种交付，不能互相阻塞或替代：**

1. **普通公开库首发仍为默认0槽。** [发行计划§9.1／§9.3](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#9-r3-首发边界与公开配置冻结)明确公开ESP32手动WiFi／TLS profile的任务容量为0；本节不把QA-3四槽试点追加为普通库发行门槛。
2. **若开放当前QA-3四槽组合，日期与星期是现有首版功能，必须有该组合的有限到期证据。** [执行器设计§1](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1-先限定定时做什么)、任务／Sketch Timer方案§1／§4与实际App均包含Countdown／UtcOnce／Weekly。App `device-timer.page.html:41`公开三类说明，`timing-edit.html:22`提供三类选项；Gateway `task-http-contract.js:33–36`及SDK `TaskTiming.h:9–10`同样接收三类。
3. 当前allowlist只决定设备能否管理任务，catalog也没有schedule-kind mask（`TaskManagementContract.h:70–77`）。**不能只改名单或隐藏UI，就声称可靠开放“仅倒计时”。** 真正分类型收窄需App／Gateway／MCU一致策略，会增加新接线，本批不做。最小路径是保留既有三类合同，只补下述两次到期事件。

这里的“必须”仅指**当前四槽组合有限开放的产品完整性**，不是重新要求各平台／任意Sketch／全部时区及掉电矩阵。日期／星期本批仍为NOT_RUN；本节只是计划，不因写入计划便视为通过或自动开名单。

### 28.2 固定支持边界与已满足条件

| 范围／条件 | 本次收敛口径与可复用证据 |
| --- | --- |
| 硬件与产品图 | 仅已测COM11 XIAO ESP32S3、原Core3.3.11、`esp32-softap-task-qa3-v1`内部四槽图；SoftAP完成并真实deinit后走原Cloud管理。不外推任意ESP32、BLE配网＋任务、纯BLE／LAN管理、同镜像动态双配网或任意Sketch |
| 运行材料与身份 | 原HTTPS/MQTTS、P256／keyId1／SPKI `02879e20…d061`，实际DeviceKey＋Admin双根及原owner／instance／scope；§17、§24、§27已核。后续只在执行前确认未漂移，不重新生成Key、scope或复制旧身份 |
| 任务与目标 | 固定4槽；当前已验目录仅`power`布尔赋值及原回调。不得用引擎标量能力外推本固件已准入数值／任意Action／脚本；“每日”是Weekly全7位，星期按周一bit0、周日bit6，固定UTC偏移，不承诺自动夏令时／旅行调整 |
| 存储与生命周期 | 复用`bl_v2_tasks/snapshot`、原Codec／Engine及双根owner围栏。固定policy为60秒10次持久写、Start最小间隔30秒、commit上限100 ms（配置，非硬实时保证）。根或持久结果不确定时fail-closed；不承诺Active Direct在线换根，维护期显式处理，不添加降级单根 |
| 基础任务闭环 | §17原App查询／保存停用／启停／唯一倒计时Start／后台到期／fresh Completed＋State已有限通过；不重复Start或把“保存确认”说成“到期执行确认” |
| 管理连接与恢复 | §24精确Service持久确认前保护／确认后续期，§26后页失败保留容量、无自动查询、手动完整读取已有限通过。多HTTP分页仍可能遇暂时OFFLINE，支持显式恢复，不承诺不断连或固定端到端时延 |
| 时钟 | §27当前组合Server UTC接受、质量与同clock推进有限通过；历史有效期1小时／不确定度≤5秒保持。日期／星期依赖有效业务UTC；离线不等于无限期准确，不是安全凭据时钟或硬实时调度 |
| 默认准入与发行 | Gateway管理＋provisioned及Broker任务名单仍关闭；普通库0槽、公开profile及既有R4有限结论不变，Wio继续暂缓。内部组合的有限开通不等于已发布供所有用户安装的四槽Arduino包 |

§27的材料、版本及记录是**该批既有事实**，本批没有联网或上板重读，不能把它们写成“本批再次新鲜核验”。现存原`f8783fbe…eaee`应用、beta `10a6a218…fdffba`及匹配服务可作为下批起点；有漂移先停点说明，不默认重编／重装／刷板。

### 28.3 复用旧日历证据，不重开测试矩阵

- [执行器设计§10.30](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1030-第三十一批日历真机与重复查询收缩)已在旧COM3手动WiFi图验证一次日期、固定偏移星期、真实回调／认证进度／State，以及完成后软件重启不重放。私有`build/task-calendar-live-20260923/verified-evidence.json`与旧文档一致；不是当前COM11／SoftAP／双根／TLS图的验收。
- [§10.31](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#1031-第三十二批失时跨到期满表与start边界)已另补无UTC跨到期后恢复不补执行。不能因前批摘要的`notCovered`仍列该项，就再次派发同样失时／重启测试。固定偏移转换、质量失效／迟到跳过、消费水位与不重放也有原host源码及旧执行记录支撑；本批只读核对，没有重跑或冒称当前源码全套回归。
- 旧普通图共同核心证据与§17／§24／§26／§27可以继续复用；**仅剩当前产品组合真实走日历分支→原属性入口→持久结果与State的两条正向证据**。不额外测满槽、长时间离线、跨周等待、DST、复位、任意目标、BLE、资源、COMMIT或480秒续期。

### 28.4 后续唯一最小补证范围（待明确允许实际执行后进行）

日历任务启用后会自行到期执行，**不调用Start不等于没有执行风险**。本批不Create／Enable任何任务；以下是后续窄范围，不是绕过当前“不Start、只梳理”的权限。

1. **只读前置，不重建。** 沿匹配App、Gateway／Broker、COM11和原scope，保留账号及完整journal／请求／attempt／服务日志。核实际owner、instance、Admin双根、签发材料与唯一已就绪物理context；必要时仅为COM11临时开原名单。先用原App新鲜catalog／完整records核原停用Completed任务、剩余槽及revision；不是Initialize，不复用缓存revision6或旧request。若仍是1/4，新增两项后为3/4；不足两空槽或有未知活跃意图即停，不删旧任务腾槽。
2. **正常App保存两个全新停用定义。** 一项UtcOnce、一项Weekly，各使用新的原journal请求及每次新鲜CAS修订；保留原任务ID、内容、startSequence及原scope。动作按真实State基线设为先切换`power`、再切回，不用同值上报冒充变化；同目标若有其它活跃任务／自动化或并行控制干扰则先停点，不擅自停用无关规则凑样本。独立计算手机输入的一次日期UTC及Weekly固定偏移／周位图对应的预期occurrence；不改手机时区、不按电脑时区猜测。
3. **仅各一次显式Enable，Start请求0。** 确认到期点仍在将来且留足正常配置时间，再启用各定义；两时刻错开至少一分钟，建议一次日期在启用确认后约2分钟、Weekly再选其后分钟边界，避免同刻争写。一次短窗口完成两个事件，不等待下一周。原10写／60秒、100 ms commit及CAS门槛不改；时间预算只是准备裕量／观察上限，不能用“等够时间”判成功。若错过，不自动改时间、再Enable或追加样本追跑。
4. **不为复读UTC再刷观察固件。** 原`TimeSyncService::observeTime`在无有效UTC时仍提供monotonic，保存／Enable不以UTC有效为前提；原cursor会等时，恢复后仅安排严格未来。§27继续作既有组合证据，不声称本批／下批原App能读新鲜时钟状态；某次因无有效UTC而未执行，就按实际待时／跳过结局保留，不拿服务器时间推定设备已校时或伪造通过。
5. **每事件只要求同一条完整证据链。** 原管理确认说明定义／Enable已接受；到期后新的已认证`fresh Read`须对应同taskId／definitionRevision、`progress=Completed`且`consumedUtcMillis`精确等于独立预期occurrence，并有原回调`power.report`产生的对应新鲜State变化。按同设备／instance／State revision／源上下文关联，后台或离页可在该窗口内一次覆盖，不另开退出App矩阵。查询持久进度不是独立执行push ACK，不承诺GPIO动作、精确回调次数、物理时延或恰好一次物理副作用。
6. **异常即止写，不重放。** UNKNOWN／换根／会话或scope不一致／CAS冲突／质量不足／持久失败均保留原意图和完整证据；仅用原显式入口、新请求号做只读核对。不能以新定义或新request替代旧写来追求成功，也不能把同值或普通在线State当旧UNKNOWN已解决。
7. **只清理本批定义，不回滚任务区。** 两事件确认后，通过原App停用（尤其Weekly）并正常删除本批两个taskId，每次取当前revision及真实确认；原停用Completed任务保持。表revision自然向前、完整新ledger保留，不恢复旧NVS／旧revision／旧scope。**关闭云端名单不会撤销设备中已启用任务**，不能把关名单当停用；清理结果未知则保留未知并明确停点，不声称已停止／已删除。最后恢复本批测试配置、关闭临时单ID并收回页面／连接观察器。

若版本未变且正常沿原固件执行，**不做全Flash备份／重编／刷板／固件回滚**。只保留本批必要账号／意图／日志和任务前后证据；真有固件变更需求再单列，不默认扩大本节。

### 28.5 准入决定的收口门槛与本批停点

剩余只保留三项，不再滚动添加无边界审计：

| 门槛 | 当前状态／通过条件 |
| --- | --- |
| 当前组合日期＋星期正向执行 | **NOT_RUN，唯一新增运行证据缺口**；各一个到期事件满足§28.4第5项即可，不要求三轮、跨周或全部时区 |
| 本批数据与任务安全收尾 | 下批两条测试定义的停用／删除真实确认、原任务／授权／scope／UNKNOWN保留、配置及连接回收；不能靠回滚NVS抹掉历史 |
| 维护者明确有限开通范围 | 以上通过后，仅就“匹配COM11／固定图／四槽／三类时间／power目标／原质量及故障合同”给出可有限开放结论；执行开名单仍需明确安排，不自动开放账号、同型号所有设备或普通下载包 |

Error8继续单列待定位，不把它无条件扩成所有任务有限开通的无限阻点；若本次最小路径实际遇到它且影响结果，再按新证据停止，不掩盖失败或扩大承诺。普通公开0槽发行、R4/Wio适配、纯BLE任务、公开四槽安装交付、其它QA自然欠证均不混入这两个事件的验收。

**本批完成支持合同、首发必要性决策和最小补证计划，仅更新文档。** 没有运行新测试／编译、连接手机／COM11、访问云业务、切准入、Create／Enable／Start／清理任务、发码、升级SDK或stage／提交推送；原授权、UNKNOWN及完整日志没有被读取后改写或恢复覆盖。下一批只消费本节的两事件计划，不重开已通过项；在执行获安排及证据齐备前，QA-3正式准入继续关闭。

## 29. QA-3：日历短验准备停点及正常清理（到期执行未运行，2026-11-01）

### 29.1 实际前置与两条停用定义

沿用原beta **`10a6a218…fdffba`**及已恢复的`f8783fbe…eaee`固件，不构建／安装／刷板或开业务串口。只读核COM11 USB为原XIAO ESP32S3，COM9不动；电脑仍在“带翅膀的蜗牛”。先保留App账号7项secure、两个完整journal store与logcat，原账号inventory／设备详情各200；没有登录、发码、取Key或重配。沿§27恢复事实使用原固件，**本批未新鲜读回应用或NVS**，不能声称又做了全片或任务区字节校验。

云端新鲜前置仍schema58／原owner、instance、DeviceKey／Admin／scope1，任务ledger11／attempt11，实际P256／keyId1／SPKI `02879e20…d061`匹配。仅将COM11加入原Gateway管理＋provisioned及Broker名单，同镜像重建必要Gateway／Broker；无镜像／签发材料／数据库迁移／新身份或scope。实际context200证明物理连接、Hello及原任务订阅／帧门槛就绪，原App新鲜完整读取为容量4／revision6／原停用Completed任务1条。

通过原可见DOM控件输入，未用私有Angular方法、直接任务fetch或虚构成功。手机实际时区 **America/Los_Angeles**，当时偏移 **−420分钟**，未改手机时区；操作前冻结：一次日期`2026-11-01 00:40`→UTC **1793518800000**，星期为周日bit6／minuteOfDay42／offset−420→本次预期UTC **1793518920000**。这些是计划值，不是已消费水位。

| 实际已确认写入 | 结果 |
| --- | --- |
| Create一次日期 | 新taskId2，`enabled=false`、`power=true`、UtcOnce1793518800000，CAS6→revision7／count2；HTTP200、verified／fresh、replayed=false |
| Create星期 | 新taskId3，`enabled=false`、`power=false`、Weekly bit64／minute42／offset−420，CAS7→revision8／count3；同样真实新鲜确认 |

新表单默认启用，本批在保存前显式取消“启用定义”；原始Create报文及两条新任务的8次fresh Read记录均证明停用／progress0／startSequence0／consumedUtcMillis0，未用默认值碰运气。

### 29.2 工具失败原因及未执行边界

本轮**没有完成两次到期验收**。四个测试工具停点原件保留，不能列为产品失败或UNKNOWN：

1. 工具用`已用 1/4`匹配页面实际`已用 1 / 4 条`，容量空白差异导致等待超时；真实目录／Read已经成功。
2. `执行动作 select:last-of-type`在不同label下匹配两个select，原“唯一控件”保护拒绝，尚未保存；随后只对同一草稿用精确label范围完成输入，没有重发业务请求。
3. 第32步仅点击原#2“启用”，打开**“启用任务？”确认弹窗**。工具误将弹窗背后的旧列表当已恢复，未点确认便尝试定位#3；原aria-hidden／唯一可见控件保护拒绝。**Enable HTTP为0**；后来正常点击“取消”关闭原弹窗，不把点开确认框当成设备已接受启用。
4. 清理时用innerText等待“删除任务”，但原删除入口是带aria-label的图标按钮，导致工具等待超时。随后使用原图标及真实“删除任务？”→“删除”确认完成清理，非业务重试。

仅显式重新连接观察器并保留各段wire，不重新进路由补同一HTTP，也没有替换设备或Service结果。准备耗时使一次日期的预留时间不足；按§28没有改期、追加定义或追跑。本批全部30笔任务HTTP按business requestId唯一，**catalog9／Read17／Create2／Remove2**，每笔均HTTP200、verified／fresh；无同requestId对应多次网络发送、HTTP UNKNOWN或写重放。Enable／Start／Initialize／Replace均0。

因此这不是“设备漏执行日期”或“星期已执行但State丢失”：两条定义根本没有启用。没有认证Completed或预期消费时间，Cloud `power=false`也不能算本批执行证据。日期、星期仍分别 **NOT_RUN**，不修改§28门槛或§27 UTC有限结论。

### 29.3 正常删除、安全收尾与记录保留

- 原确认弹窗取消后，两条定义仍停用，无需额外重复SetEnabled(false)来制造写样本。正常App先删除#2（CAS8→revision9），再删除#3（CAS9→revision10）；均HTTP200／verified／fresh／replayed=false。最终新鲜目录容量4／count1、唯一Read为原task1；**9次fresh Read中的原record全字段一致**：definitionRevision1、停用Completed、startSequence1、consumed0，原10秒定义不变。
- 只正常推进表revision及nextTaskId，不恢复旧NVS、旧表、旧scope或数据库旧行。未重置根、清授权、退休设备、调用Start或更换固件；同一scope的身份／根／状态／replay-floor保持，正常updated_at可变化。
- 服务保留旧**7条非Read**（1 Authority＋6 Mutation）及对应attempt逐字段，原非目标UNKNOWN摘要保持。前置4条已过期verified Read按原60秒retention自然回收，完整前字节保留；本批历史Read也受原策略影响，最终ledger19／attempt19不是全部30笔HTTP的数量。无新UNKNOWN／issued／prepared未结算行，不用自然回收声称UNKNOWN已解决。
- 本批新增定义删除确认、App回到稳定首页后才撤回单ID。原配置SHA **`e11252c9…c48`**精确恢复，Gateway／Broker原镜像、签发材料与挂载及PostgreSQL／Redis保持；终核名单全关、四角色健康，context503 `TASK_CARRIER_STALE`为关闭后预期门禁。开关名单自身前后完整目标ledger不变，不把关名单当本地停用。
- App原7项secure／两个完整journal前后逐字相同，原APK保持；完整logcat、原停点及截图保留，最终首页、UART未打开、CDP／SSH回收，原stayon=0恢复。云端限定完整日志／前后ledger／配置核对等 **69文件、769584 B**已拉回ACL私有目录并逐项SHA核验，不导出env或私钥。

一次本地摘要工具把尚在远端的`preflight-broker.private.json`误当成本地文件，得到ENOENT；没有因此重试服务／设备，用原status及最终收集取得实际证据。该工具输出与四份UI stop均保留，不掩盖准备失败。

### 29.4 只修测试入口，不改产品执行链

失败归属是本轮测试脚本，不支持修改SDK／App业务／协议／轮询的结论。只在忽略目录补纯`dom-ready.mjs`：就绪按实际page／容量数字／唯一可用控件及aria-label判断；有未关闭确认框不得当作列表就绪，下一模板可按明确taskId核启用状态。补读task-row和控件快照，不读私有组件或替换网络。

以本批留存的真实DOM快照进行**6项离线工具合同校验通过**：容量空白、正确编辑页、确认框未关闭拒绝、全局“启用”多控件拒绝、图标删除aria-label、清理后真实容量。不是产品测试、重跑真机或到期执行。下一入口模板额外在真正确认Enable前核预期occurrence还剩至少60秒，不足即停止；没有自动确认、改期或重发。只生成模板及摘要、做语法检查，**未运行模板／创建新批次**；原实际执行的`ui.mjs`及失败数据保持不变。

证据：App `build/qa3-calendar-live-20261101/directory.json`及私有`acceptance-stop-result.json`／原始wire／4份`ui-stop`；服务同名忽略目录及`collected-manifest.json`。后续模板为App同目录`ui-reviewed-next.mjs`／`next-entry-checkpoint.json`，须在明确安排的新批次用独立目录、账号快照及新计划实例化，**不能直接消费本批旧intent或原已删taskId**。独立只读审阅与最终离线artifact核对一致。

**当前结论：停用定义保存及正常清理已核实；两次到期执行未运行，QA-3正式准入仍关闭。** 下一批只需使用修正后、先冻结并冷预检的原DOM确认入口，重新明确安排两事件窗口；不得把本批失败藏起来、自动改期追跑或把NOT_RUN改为通过。§28支持范围及最小门槛不扩展，Error8继续单列。未重配、发码、升级SDK、回滚NVS、stage／提交或推送；保留全部无关本地修改。

## 30. QA-3：新批冷预检、两类日历真实执行及受限支持收口（2026-11-01）

### 30.1 沿用版本及无写入冷预检

新批独立目录`qa3-calendar2-live-20261101`，不续用§29旧intent、计划时刻或已删taskId。沿§27已恢复的原`f8783fbe…eaee`、实际beta **`10a6a218…fdffba`**、原Gateway／Broker镜像及scope；本批不构建／安装／刷板、不读取或回滚NVS、不开业务串口，COM9不操作。新鲜核COM11 USB身份、原账号及API详情、P256／keyId1／SPKI `02879e20…d061`、原owner／instance／Admin／scope1及schema58。准入前ledger19／attempt19，其中11条非Read＝1 Authority＋10 Mutation，另8条旧verified Read；先保留全部原记录与配置。

只临时开放COM11原管理＋provisioned及Broker名单，同镜像重建必要Gateway／Broker；四角色健康、实际context就绪。原任务为revision10／count1，仍是停用Completed的task1。没有新建身份、scope、Key或发码。

**先冷预检，后生成计划：**

- 原task1“启用任务？”实际打开后明确点取消；编辑页原删除图标的“删除任务？”也明确取消，校验原记录不变。新建页只做日期／星期草稿字段冷填、取消启用勾选、读取实际布尔选项及周位图／固定偏移，再通过原返回入口离开，**冷预检成功段10笔HTTP均为只读、mutation0**。
- 预检前两次工具停点原件保留，均没有plan或任务写入。首段在缓存页返回、下一catalog尚未完成时原控件操作被拒绝，原异常只留通用提示，不能冒称某个原生分支已唯一定位；为此等待本次新鲜Read完成及可用控件，而不是缓存ready。次段确认当前Ionic刷新按钮的aria-label不留在host上，改用原已验`slot=end`工具栏唯一按钮。只改测试薄适配，未改产品或SDK。
- 最终通过后生成`entry-final-freeze.json`，UI／DOM薄适配及就绪谓词摘要冻结，时间计划稍后才创建；**计划后的执行窗口内没有改脚本、重启控制器、重发业务请求或改期**。显式确认Enable前仍检查至少60秒余量。未将冷预检失败藏进“一次全绿”。

### 30.2 两条停用定义、显式启用与真实到期

手机仍为America/Los_Angeles，本次固定偏移−420分钟，未改手机时区；一次日期输入通过手机Date往返校验，Weekly再独立核固定偏移、星期位和分钟数。计划只生成一次：

| 任务 | 原App保存定义 | 精确预期UTC／北京时间 | 实际新鲜持久结果 | 对应Broker State |
| --- | --- | --- | --- | --- |
| 新task4／一次日期 | disabled、`power=true`，手机`2026-11-01 01:15` | **1793520900000／16:15** | 同taskId、definitionRevision1、**Completed／consumedUtcMillis=1793520900000／startSequence0** | revision2、power=true、updated_at1793520900296 |
| 新task5／星期 | disabled、`power=false`，周日bit64、minuteOfDay77、offset−420 | **1793521020000／16:17** | 同taskId、definitionRevision1、**Completed／consumedUtcMillis=1793521020000／startSequence0** | revision3、power=false、updated_at1793521020283 |

两条Create先分别获HTTP200／verified／fresh／replayed=false，随后各通过对应taskId的原“启用”→精确确认框→“确认”**一次**。等待确认框消失、原新鲜列表及同taskId启用状态，不以点击或旧列表文本当成接受；没有Initialize、Replace、Start或恢复原请求。

到期后各通过原刷新按钮**一次显式新查询**取得认证Completed与精确消费水位，不以时间等待或管理确认判定执行。每个occurrence在后续正常刷新／停用／删除流程中共有5份匹配fresh Completed Read，这是**同一发生的多次读取**，不代表执行5次，也不据此测精确回调次数。原回调`power.report`产生的新鲜State与目标值、同设备／instance及预期occurrence对应；前置原State为false，真实变化为**false→true→false**。

两事件之间连接按原480秒机制自然换代，context从`t59Q…／dNx…`变为`Fh60…／bG0…`，同owner／instance／credentialVersion保持；不能写“同一连接贯穿”或据此重复派发续期矩阵。服务端State updated_at与计划UTC相差296／283 ms仅为关联时刻，**不是MCU回调／物理输出延迟或绝对精度保证**。执行前后两次有界READ ONLY查询均无其它启用的目标power规则或prepared／dispatching／outcome_unknown执行；只证明这两个快照为空，不宣称连续监控全部潜在writer。

### 30.3 正常停用、删除与CAS／原任务保护

| 新写入顺序 | 新鲜CAS与结果修订 |
| --- | --- |
| Create task4、Create task5 | 10→11、11→12，均明确disabled |
| 启用task4、启用task5 | 12→13、13→14，各一次true |
| 日期完成后停用task4、星期完成后停用task5 | 14→15、15→16，各一次false；持久Completed及消费水位保留 |
| Remove task4、Remove task5 | 16→17、17→18，均原App正常删除确认 |

8笔变更全部HTTP200／verified／fresh／replayed=false；只操作本批新task4／5，不复用或修改原task1。最终目录capacity4／count1／tableRevision18；**全部捕获的21份原task1已认证Read全字段一致**：definitionRevision1、原10秒倒计时／power=true定义、disabled／Completed／startSequence1／consumed0。定义表revision及nextTaskId正常向前，不恢复旧快照、旧revision或旧scope。

成功控制器共66笔已完整捕获HTTP（catalog19／Read39／Create2／SetEnabled4／Remove2），均HTTP200／verified／fresh：其中冷成功段10笔只读，**计划后56笔＝catalog14＋Read34＋8笔变更**。合并全部冷停点共捕获71个唯一business requestId、70份完整fresh响应；首段冷停点另1笔catalog响应未完整捕获且无loadingFailed，保持未分类，不写成产品失败或UNKNOWN。不存在同business ID多次网络发送、响应冲突或写重放；没有将请求条数当执行次数。

### 30.4 准入关闭与证据终核

1. 两条新定义真实停用及删除、App返回首页后才关闭临时单ID。原配置SHA **`e11252c9…c48`**精确恢复，原镜像、签发材料／挂载、PostgreSQL／Redis及schema保持；四角色健康。关闭不退休scope、不修改设备任务区，不把关名单当本地停用。
2. 原scope1的身份／根／状态／replay-floor保持，正常updated_at可变化；原11条非Read及对应attempt逐字段保持，原UNKNOWN摘要保持。前置8条旧verified Read按原60秒retention自然回收并保留完整前字节；本批旧Read同受原策略影响，不手工删库。最终保留35份request／35份attempt均verified，无prepared／issued／unknown未结算行。中间一次只读快照碰到正常inflight的1条issued，后来已真实verified，不将其写成全过程从无在途请求。
3. App实际APK、原7项secure及两个完整journal store逐字保持，完整logcat、分步意图／HTTP／响应／截图及冷失败保留；最终稳定首页，无遗留任务页／弹窗，CDP及SSH回收，原stayon=0恢复。没有登录、发码、reveal Key、重配或NVS回滚。
4. 云端完整限定日志、前后ledger、运行事实和两次writer只读投影共 **101文件／1062927 B**已拉回ACL私有目录并逐项SHA核验；不导出env或私钥。输入冻结在运行结束后仍一致。独立只读审阅与离线artifact审计一致，不将工具审计当作额外真机样本。

证据入口：App `build/qa3-calendar2-live-20261101/directory.json`及私有`acceptance-result.json`／`entry-final-freeze.json`／`calendar-plan.json`／最终wire；其中`10-refresh`为日期结果、`13-refresh`为星期结果、`18-screenshot`为清理后原任务。Service同批`directory.json`、`status-6-result.json`（日期State）、`status-7-result.json`（星期State）、`close-result.json`及`collected-manifest.json`。所有原始证据／私有配置及测试产物留忽略目录，不进入Git。

### 30.5 QA-3本组合的有限支持结论与后续边界

**§28的当前组合日历正向证据及安全收尾门槛已满足，QA-3这一固定组合的最小功能验收可有限收口。** 连同§17倒计时、§24持久确认保护、§26分页显式恢复及§27 UTC质量，不再把“当前组合日期／星期尚未执行”列为待补，也不默认重跑已通过矩阵。

结论只覆盖**COM11实测XIAO ESP32S3／原Core3.3.11／固定SoftAP后Cloud TLS／P256 keyId1／DeviceKey＋Admin双根／4槽／power布尔目标／原质量及故障合同**。不外推任意ESP32／Sketch、BLE或LAN任务管理、R4／Wio、数值目标、GPIO物理输出、下一周／DST、无限离线、任意并发或公开四槽安装包；普通公开库默认0槽及原发行边界不变，旧有限核心证据仍按各自范围复用。

**技术上具备按此合同单设备有限开通的条件；实际准入已按本批要求关闭，不自动保持或扩大名单。** 后续进入维护者的有限开通／交付决策：只需在明确安排时复核匹配版本、逐设备身份／双根／scope和配置范围；公开profile／安装交付若需调整另列最小批次，不假装内部BIN已公开发行，也不自动构建、部署或推送。Error8仍为独立待定位项，本轮未复现，不声称其已解决；不再把它无条件扩大为本组合无限审计前置。

本批没有产品生产源码修正、Start、重配、刷板、发码、SDK升级、NVS回滚、stage／提交或推送；仅测试薄适配、有限实际验收及必要文档更新，无关本地修改保留。§29原失败及未完成结论保持历史原样，本节是经新授权、新计划取得的成功证据，不回填旧记录。

## 31. QA-3：逐设备有限开通与普通Arduino交付接线（2026-11-01）

### 31.1 最小范围与不再重复的前置

沿§30已经有限通过的固定图，不重跑倒计时、日期／星期、Start、续期、UTC、配网或资源矩阵。
本批只完成三件事：**明确沿现有名单的单ID开通合同、补维护者受限打包入口、提供普通Arduino固定例程及隔离安装编译入口**。
不新造准入API／签发器，不把SDK维护层搬到ESP32，不实际部署或打开名单。§30的技术可开通结论与实际仍关闭状态分开。

固定范围是XIAO ESP32S3／原Core3.3.11／`default_8MB`／SoftAP WiFiProv后HTTPS＋MQTTS／4槽／P256 keyId1／DeviceKey＋Admin。
仅`power`布尔可写目标及`uptimeSeconds` 1000ms实时属性，不加入数值Action、BLE配网＋任务、纯BLE／LAN任务管理、其它板型或任意Sketch。
普通公开`cloud-beta.json`、七目标和源库默认0槽不变。Wio继续后置，Error8独立待定位。

### 31.2 逐设备开通／撤回合同

服务源码现有owner／tenant／currentKey／唯一instance／scope／CAS、Admin双根和Broker Hello／订阅就绪围栏足够，
不需要新管理接口。逐设备开通仍须新鲜核版本、身份、双根、scope、实际算法2／keyId1／SPKI，
再以**同一逻辑ID**显式配置Gateway管理名单、Gateway provisioned名单及Broker任务名单，Ed25519名单空。
沿原Compose叠加及`check-tasks-qa3-config.cjs`核完整配置；它只是离线一致性，不证明证据来源、新鲜度或板上BIN／Store。

本批只发布公钥投影SPKI **`02879e20a6ad6e021ed7e6c857f0a351db14b1568c467db519aac09920b6d061`**，
来源是§30已存运行事实，不冒称本批新鲜联网核验。不含私钥、PoP、网络凭据、DeviceKey或准入ID。
原私钥及实际服务配置均未读取／改动。当前三名单继续关闭；以后明确开通时只需健康／context及一次fresh catalog／Read，
不默认重做已通过矩阵。关名单不等于停本地已启用任务，仍须原正常停用／删除或有证退休；不回滚NVS、搬scope、清UNKNOWN或重放。

### 31.3 必要接线及普通Arduino步骤

详见新[四槽有限交付说明](DEVICE_V2_ESP32_SOFTAP_TASKS_LIMITED_DELIVERY.md)。

- 新`tools/build_esp32_softap_tasks.py`只处理固定`profiles/esp32-softap-tasks-limited.json`，
  复用原单库`validate→stage→archive`和内部QA-3库级renderer，全部实际配置消费者共用，三份生成头都加XIAO板型guard，重算库内容身份；
  不放宽原通用profile parser、公开零槽或D4手动图guard。输出目录非空／重叠／源码覆盖拒绝，归档确定性及静态receipt独立。
- `BlinkerWiFiBLE.h`只补一行：在Facade及默认配网选择之前加载库级OfficialConfig。
  避免生成SoftAP／任务宏晚于Sketch消费者；不靠Sketch三宏伪装整个库已配置。
- 新`Esp32SoftApTasksQuickStart`只用原`enableTasks(power)→begin(power, uptimeSeconds)→run`和短`power.report`回调。
  头部拒错误板型／Core／profile／槽数／TLS及未填、空或过长PoP；不清根、不Initialize／Start，不加入其它任务目标。
  私人PoP只在用户Save As后的个人Sketch填，示例和ZIP只允许精确占位头及三份显式文件，拒绝私人额外头。
- 原安装验证器增加**显式单目标**`esp32-softap-tasks`和配套manifest参数，不进入七目标默认队列。
  先核原Core3.3.11／jobs1或2、ZIP／profile／库内容身份，复制安装示例到独立compile-fixture填公开非秘密测试PoP；
  安装目录不改字节，编译BIN不可部署。原库、依赖、provenance及链接消费者检查复用，不升级SDK或重造执行链。

普通Arduino用户只需一份受限ZIP＋原板型／SDK＋固定示例个人PoP，原App接入身份后再逐ID明确开通，
不改SDK、不写DeviceKey／SSID／WiFi密码、不安装多个同名Blinker库。安装、编译、运行证据和服务准入分别登记，
`limited-profile.json`保持`deployable/runtimeSigningMatched/formalAdmission/installationCompileVerified/hardwareVerified=false`，
不能将其改标签交给Service的runtime-candidate checker或据此扩大公开发行范围。

### 31.4 有限验证记录

新例程合同7／7通过。交付合同首轮7项中6项通过、1项夹具断言过宽：普通零槽guard本来会提及`BLINKER_TASK_CAPACITY`，
不能据此断言存在四槽定义；改为只拒绝实际`#define BLINKER_TASK_CAPACITY`后该项单独通过，未修改产品策略。
补CLI禁自定义profile和receipt拒替换2项通过，合计新增**16项已分别通过**；既有包确定性／文档白名单／profile-gated矩阵／任务范围报告4项及R3公开冻结1项定向通过。
不重复运行不变的内部renderer5项、原七目标或真机样本。独立只读审阅发现的自定义profile不可由安装器验证问题已收敛为固定单profile入口，
不为此新建多profile支持。服务四份README／API只对齐已过期当前结论及单ID合同，历史证据保留。

唯一新增归档位于`build/qa3-limited-arduino-20261101/artifacts/`：ZIP SHA256
**`019bc5fbd937bec6f9b78c69cf3425872c94239810157c81806f3bf6ca886194`**；公开profile摘要`72f65265…19ca9`，
库内容身份头SHA256`70a572cc…1644`。原CLI已从该ZIP安装到独立sketchbook，已核PubSubClient2.8和Core3.3.11；
同批唯一原SDK定向完整链接已退出0：**单目标605.9秒／全gate616.2秒**，compiler Flash **1436556 B／3342336 B**，
静态RAM **59588 B／327680 B**；BIN1436704 B／SHA256`68de4315c4e60bb73eac84f2ef85b5764db00a7c05227befc4a250485bd2375d`。
原gate的库／依赖闭包、任务和授时／WiFiBLE链接消费者、无替代平台及仅隔离安装库provenance检查均通过，
编译前后ZIP／安装字节／profile／身份均一致。此BIN只含公开compile-only PoP，不能刷作设备运行候选；
compiler静态RAM不代表新增动态资源／并发或任意Sketch准入，低于旧测试图的字节数也不是新运行性能验收。
独立实际ELF／map／deps审计再次确认：唯一`esp32WifiBleProduct()::instance`、四槽`Esp32ProvisionedTaskExtension`及361 B stack，
原Store／Engine／Management／Cloud、P256／Admin selector／双根digest消费者在链；SoftAP scheme存在，未见BLE配网scheme、手动产品或R4／Wio／LAN替代图。
94个实际TU＝fixture1＋隔离库17＋隔离PubSub1＋原SDK75，没有仓库`src`泄入。
实际模板／容量消费者在Sketch TU由`.d`证明读取隔离配置；其余17个库implementation为配置中性，不伪称94个TU都直接包含配置头。
完整SDK输出在`installed/build/esp32-softap-tasks/compile.log`，命令／结果及校验值见同批`verification-result.json`；
构建产物全在忽略目录，不交付账户或运行凭据。新增包装未刷机，不用§30旧f878 BIN替代它的运行证据。

### 31.5 当前决定与下一道门

本批不操作手机／开发板、账号／授权／NVS／TaskStore／scope／UNKNOWN或服务配置；完整旧日志继续保留，
不新建任务、发码、升级SDK、开名单、部署、stage到Git、提交或推送，无关本地修改保留。
固定交付图隔离安装编译已通过；后续只按明确单ID安排完成必要只读开通核验或匹配运行候选交付；
新增Arduino包装的编译证据与§30旧BIN真机功能证据各保原身份，不互相代签，不再无边界资源审计。

## 32. QA-3：COM11单ID有限开通与新鲜目录／Read只读收口（2026-11-01）

### 32.1 实际运行前置，不刷编译夹具

仅COM11，Windows新鲜只读核USB parent `VID_303A/PID_1001/8C:BF:EA:1B:96:14`，未打开串口、复位或读取／改写Flash。
最后已核应用仍取§27恢复证据的`f8783fbe…eaee`，随后§29～31未刷板；**不是本轮新鲜Flash SHA**。
Hello没有固件SHA，旧候选与§31编译夹具又共用固件标签，不能据版本名精确区分两个BIN；本批不刷`68de4315…bd2375d`夹具。
运行合同由本轮唯一物理Broker context、同owner／instance／双根／scope、新鲜四槽目录和认证Read交叉核对，不替旧BIN或新包装补造字节证明。

云端新鲜核schema58、原四角色健康、统一Auth仍`api.yiyu.pro`，实际P256／算法2／keyId1／SPKI
`02879e20a6ad6e021ed7e6c857f0a351db14b1568c467db519aac09920b6d061`，配置／模块／接线齐全。
原owner／tenant／instance／当前Key版本1、Admin epoch1／credentialVersion1／controller及fingerprint、唯一active scope1与confirmed root匹配。
实际运行代码只读inventory为Gateway243份／Broker339份；不导出env或私钥。
前置完整保留35 request／35 attempt：**19条非Read＝1 Authority＋18 Mutation，另16条verified Read**，不自动初始化、清scope或协调旧UNKNOWN。

### 32.2 临时开通及一次原App进页

只将同一COM11逻辑ID加入Gateway management／provisioned及Broker allowed名单，Ed25519名单仍空。
同镜像仅重建Gateway／Broker，原PostgreSQL／Redis容器、镜像、签发文件／挂载、其余配置不变；开通前后完整账本逐字保持。
原运行Broker HTTP context真实返回200并核proof session／connection／credentialVersion及同instance，不以数据库在线标志或固定等待代替。
工具停点前后两次context／账本观察均无新业务请求；它们是控制面只读前置，不算重复任务Read。

沿原beta APK `10a6a218…fdffba`，同账号7项secure和两个完整journal原样保留。
只进入一次原任务路由，不点击任务控件、不清缓存、不手动刷新或重放请求；自然冷目录沿原链路产生：

| 请求 | 实际新鲜结果 |
| --- | --- |
| catalog metadata，一次 | HTTP200／verified／fresh，capacity4／count1／tableRevision18 |
| catalog target page，一次 | HTTP200／verified／fresh，唯一power布尔目标，原目录fingerprint一致 |
| Read index0，一次 | HTTP200／verified／fresh，原task1全字段一致：disabled／Completed、definitionRevision1、startSequence1、consumedUtcMillis0及原10秒倒计时定义 |

三笔唯一requestId、各一个新attempt，wire operation均2；无Initialize／Create／Replace／Enable／Start／Remove／Retire、新scope或Key操作。
Read结果没有mutation专用`replayed`字段时不补造false；不重放由唯一请求／attempt及完整账本关联证明。
随后通过原导航返回首页，结束观测内无新增任务请求，资源／CDP回收、stay-on恢复原0；不再跑后台、配网或断电矩阵。

### 32.3 实际签发字节与关闭终核

本批三份`exact_grant`及`exact_challenge`从完整账本采集，逐笔关联原App prepare和request／attempt。
使用公开SEC1点构造SPKI、按原SDK transcript进行P256／SHA256／P1363离线验签，**三笔实际签名全部通过，均算法2／serverKeyId1**；
instance／scope／nonce／requestId／operationDigest精确绑定，challenge root与原request.root_digest及scope.confirmed_root一致。
不是仅有P256配置的声明；不读取DeviceKey或私钥做离线审计，也不声称独立重算了设备结果HMAC或捕获原始MQTT dispatch包。

App离页后立即关闭临时准入，原配置SHA **`e11252c94973f7395029af7b32df939eccd8c868ce8f8eef6ce74bbd036ecb48`**精确恢复，
实际三名单均空，原镜像／签发文件／挂载、PG／Redis及schema不变，四角色健康。
最终22 request／22 attempt均verified＝原19条非Read＋本批3条Read，无prepared／issued／unknown未结算行；
原scope／根／replay floor／授权及其它设备UNKNOWN摘要保持。旧16条verified Read由既有60秒retention自然回收，
完整原字节已在前置保存；不禁prune、不改时钟、不回灌数据库，也不将回收写成UNKNOWN解决。
关闭前后目标账本恰好全等；工具仍保留只允许有证据的过期verified Read回收保护，不要求以后禁止正常retention。
关闭后context返回STALE仅按当次关闭状态记录，未再发任务HTTP，不为证明关闭制造额外请求或据此判新故障。

App本批前后及对照§30末快照的7项secure／两个完整journal均逐字一致，未登录／发码、读取Key或回写token。
COM11原NVS／TaskStore未烧录或回滚；Read证明原表／记录保持，不宣称新鲜读取了整个NVS字节。
COM9及其它板不操作；正常Cloud续期可自然换context，不声称全程同连接，也不把它计作额外续期矩阵。

### 32.4 工具停点完整保留，不改写成首次全绿

1. 最初App预检在CDP初始化、账号快照和测试HTTP之前停止，原events／task HTTP均0；之后调试器能力读取期间WebView进程21343发生SIGTRAP。
   完整logcat／crash buffer已私有保存，**具体根因未唯一定位、未写成产品已修复**。本轮不继续探测新CDP／Fetch接口或改App源码，
   只正常启动原App，显式校验零业务前置后更换为已验Network只读观察；不宣称有事前网络拦截保护。正常恢复后账号及业务证据单独核实。
2. 第一UI等待进程的非交互终端stdin不可写，尚无open intent；服务端此时新请求仍0。
   仅停止本地等待Node，不停止App；原UI intent移入私有`stdin-unavailable-tool-stop/`且SHA逐字保持。
   同一冻结UI源改由TTY交互后才首次进入任务页。两次工具续步均无旧业务请求可重发，不覆盖旧停点或冒称原失败段成功。

完整云端证据**82文件／1145494 B**已收集到ACL私有目录并逐项SHA核验；排除env／私钥及配置备份内容外传。
入口为Service `build/qa3-admission-read-live-20261101/directory.json`、私有`collected-manifest.json`／`offline-signing-audit.json`，
App同批pointer、`app-artifact-audit.json`／`wire-final.private.json`／`task-page.png`／`phone-cleanup.json`及原工具失败原件。
所有测试薄适配、日志、账号记录和产物仅在忽略目录，不进入Git。

### 32.5 当前决定

**§31安排的COM11单ID有限开通及新鲜catalog／Read合同已通过，可收口；实际准入已关闭，不自动保持开放。**
复用现有业务链，无新增产品源码修正、构建、安装、刷板、任务写入、SDK升级、NVS回滚、提交或推送，无关本地修改保留。
本批不改变普通0槽、公开七目标、§31静态receipt或新Arduino包装尚未刷机的证据边界；不据此宣布新包公开发行或任意设备／Sketch准入。
后续仅在明确设备／交付安排下沿既定单ID合同核验，不重复本轮目录／Read或既过任务矩阵；Wio与Error8继续另列。

## 33. 主线校准与QA-4容量提示最小修正（2026-11-01）

主线仍是让用户正常接入、控制并确认真实状态、使用定时和最小自动化。近批内容虽属于定时稳定可用，
但QA-3功能、包装和准入验证已经分别完成，汇总表却保留§13／17旧待补与空ring、UTC、scope、短验未完成表述，
造成已完成子项继续排队的风险。本批直接对齐头部、§2／3.3／4／7及唯一进度入口，不改历史失败正文，不再新增任务／配网／签名验证队列。
名单关闭是§32明确收尾；普通0槽、固定四槽支持、尚未刷机的新Arduino包装和公司独立签收是不同层级，不互相制造功能未完成。

QA-4只读核对沿现有源码与原证据：开／关规则共存已过；四条创建门禁、类型化冲突提示、收到分享计数和失败保护均已有实现。
自然非零分享／过期／其它通道等仍属欠证，不为补样本制造记录，也没有据此确认新的执行链阻断。
唯一本批小修来自明确源码语义：Gateway的`AUTOMATION_CAPACITY`同时用于全库16条和每租户4条限制，
两者均计未退休规则（含停用），原App一律提示“可先停用并退役”，未说明停用不释放，也不能指引未满账号靠删自身规则解决全站容量。

只修改两处原消费者：

- `automation.models.ts`：保留原错误码／分类，改为中性“自动化容量已达上限”，说明停用不释放，账号未满时联系管理员核对服务容量；不猜套餐、不引导自动重试或删除历史。
- `automation.page.html`：原四条满额提示补“包含停用规则”，原删除腾位、按钮门禁及`role=alert`不变。不改布局、数量、服务协议、CAS／UNKNOWN或任何执行器。

有限回归复用原Angular／Vitest及WiFiProv源码alias，无SDK或依赖变更：先选新增纯函数1例红测，准确命中原容量文案；
另外40例因名称筛选未选，不算通过，也未声称两项UI用例已红测。
两处修正后只选**3例全部通过**：错误码／Error对象的容量文案和独立历史容量提示、四条停用规则仍禁新建且提示计数口径、
真实生产模板`role=alert`的新文案。38例未选不累计，状态替身无规则写／查询／重试，未创建云端第17条规则。
红／绿原日志、命令及结果保存于App忽略目录`build/qa4-capacity-copy-20261101/`。

UI/UX技能只用于准确、可感知且可操作的错误反馈：检索命中错误需宣告的规则，现有alert已保留；
容量含义取实际源码，处理建议按quick-reference的error-clarity／error-recovery一般原则收敛，不把技能检索当业务事实。
没有视觉重设计、手机／多机型视觉验收或新安装通过声明。当前手机仍§32的旧匹配APK，这两处仅源码已修，不为小文案单独部署或重测自动化。

下一步按原顺序转QA-5：先对齐同事负责的自定义控制V2／布局编辑范围，再选择一项当前有具体复现的输入、预览或性能问题；
没有复现就不猜测重写，不为推进数字开启全量测试。IP／国内SDK、Wio、Error8、调试器异常、OTA／Bulk及复杂规则不自动插队。
本批没有设备／服务／账号操作、任务写入、权限／额度调整、安装、SDK升级、提交或推送；原授权、TaskStore、scope、UNKNOWN及完整日志不动，无关本地修改保留。

## 34. QA-5共享文本输入提交一致性（2026-11-01）

### 34.1 当前实现与本批边界

仅核对本地`i3w-dev-v2`的HEAD `8b19ac011b3a`及现有工作树，不声称已取得同事远端最新进度或重新约定人员分工。
本地xwd合并锚点为`5574c243cc43`／`ff9873ab32d8`，本批不再合并。
现有自定义V2路径是编辑器／HTML schema→sandbox host→原DeviceUiPort；`device-template`是另一套可信同源模板，不能混作V2 sandbox。
`src/custom-panel/main.ts`实际挂载共享`PageWidgetComponent`并沿原action桥接，标准控制页也消费同一叶子组件。

同事的自定义编辑器、HTML协议、AI生成、拖动布局与配置预览保持原范围；本批只修共享`TextWidgetComponent`的标量提交门禁，
不新增传输、不接管页面，也不把当前明确“预览不会发送设备指令”误判为功能丢失。
相关既有名称／单位映射等dirty保留，未覆盖或重排；未重新生成自定义运行时或声称当前安装包已匹配。

### 34.2 真实复现与最小修正

可写自由文本（无enum非空约束）从原内容清成`''`或空格后，原真实模板的发送按钮统一使用`!draft.trim()`禁用，
但Enter／多行Ctrl或Meta+Enter仍调用`send()`并发出commit；同一个草稿因此只能通过键盘提交。
原App `DeviceV2Page.commit`对text保持原串，协议codec只约束最大UTF-8长度及可选enum，并没有全局自由文本非空规则。
因此不应借用数值空输入保护禁止用户显式清空文字，也不能把文本trim后再发送。

只调整`widgets/text/text-widget.ts`及原模板：

- 增加一个`canSend`门禁，按钮和`send()`共用；只读／disabled一律禁发，数值要求非空，text保留空串与空格。
- 不改draft事件／宿主草稿所有权，不在输入或新State时自动发送；每个显式点击／快捷键只发一次commit。
- 不改宿主数值／enum／长度校验、原Command／ACK／State、CAS、UNKNOWN，也不改布局或iframe协议。

### 34.3 有限证据与下一步

新增`widgets/text/text-widget.spec.ts`，使用真实Angular组件及生产模板，不替换`send`或注入假按钮：

- 单行空串／空格、多行Ctrl／Meta+Enter，与真实input草稿回传及显式按钮提交一致；原输入模型不被修改。
- integer／number空串与空格的按钮及Enter均禁发；显式`0`仍沿原commit传递，不做类型转换替代宿主校验。
- 空文本的只读、disabled及共享编辑预览inert边界仍禁写。

同一组9项先红后绿：原实现6项准确失败（4项文本按钮误禁用、2项空数值Enter绕过组件门禁），另3项原边界通过；
最小修正后**9／9通过**。复用原Angular／Vitest、现有WiFiProv源码alias及单worker，未重跑已过矩阵。
原始命令／退出码／红绿日志保留在App忽略目录`build/qa5-text-submission-20261101/`；没有APK／原生或SDK构建，也不作手机／设备ACK验收声明。
UI/UX技能仅用于触摸／键盘同一提交语义及只读反馈的局部检查，业务合法性以现有宿主／codec为准；没有视觉重设计或多机型验收。

本批单项源码收口，不等于QA-5整体完成。若下一批选择匹配App安装，只短验这一个共享输入消费者及预览禁写，
可合并携带§33容量文案；不因此重跑配网、任务或两设备自动化，也不默认扩展长枚举、性能或最低WebView专项。
本批无账号、服务或设备操作；授权、TaskStore、scope、UNKNOWN及完整日志原样保留，无关本地修改不动；未扩槽、升级SDK、提交或推送。

## 35. QA-5匹配安装与自然能力范围内的手机短验（2026-11-01）

按§34构建并以`adb install -r`安装一次匹配内测App，不清数据。APK SHA-256为`e1e50edf64a641ba5956635daa8c801b0b935b4fdd9d2737927797e3ed7d2a5b`，
包名`iot.diandeng.tech.beta`、原签名、HTTPS、原WiFiProv SDK2.4.4及1157项同步Web资源通过核对；
共享自定义运行时与标准页均含本批`canSend`，没有只更新主Web遗漏原子运行时。
复用原串行prebuild→Web→sync→APK与验证入口，未升级SDK／依赖；源码、索引和分支在构建期间不变。

先保留同账号7项secure及两份完整IndexedDB请求journal、原logcat，前后精确一致。
一次库存及四份真实Manifest读取均200：三台为`power(bool RW)`＋`uptimeSeconds(uint RO)`，另一台仅`power(bool RW)`；
**没有自然可写text／数值字段**。不修改COM11 Manifest、固件、授权、TaskStore或scope来凑输入样本。

实际完成范围：

- 真实手机App原“组件库”明确标注的**演示数据**：OLED文字从`HELLO ESP32`清空，输入本身不提交，按钮可点；有效点击后演示值清空。
  再填三个空格，Enter后原串保留；不是设备ACK／新鲜State验收，不能据此关闭真实text执行链。
- 原COM11“自动适配”真实配置预览：明确“预览不会发送设备指令”，panel及两控件inert；
  在稳定可见区域有效点击后power保持不变，观测设备Command及HTTP写入均0。未应用界面、改布局或写配置。
- 数值TextWidget空输入／显式0，以及自然text的设备确认，因实际能力不具备**保留待测**；只引用§34已有9项源码回归，不把演示滑条或只读uptime伪装成数值输入。

工具边界原样留档：首次文字测试因滚动点位未命中在点击前停止，0网络／0提交；改用原IonContent滚动接口后显式接续，未重建或重发业务。
早期预览点位超出视口不计点击通过；只采纳后续可见矩形、实际命中及零写入的结果。最终稳定截图完整正常，
不将该短暂几何现象认定为编辑器固有横向布局缺陷。一个追加制品检查曾先于基础验证文件就绪而停止，待原验证完成后执行，无第二次构建或业务重试。

收尾回首页、关闭本批被动Network观察及CDP转发，手机常亮恢复原0；原账号、7项secure、两份journal及UNKNOWN不变，TaskStore未操作。
原始意图、分段构建结果、失败／接续、私有日志及截图位于App忽略目录`build/qa5-input-live-20261101/`的ACL私有子目录；
没有新配网、发码、任务操作、SDK升级、提交或推送。UI/UX技能只用于提交、命中与禁用状态一致性，不作多机型视觉验收。

用户随后指出组件库滑动及卡片内部高度问题。本批已完成项不重复；下一步只复现这两类共享控件问题，
不接管HTML／AI／拖动编辑器，也不为补自然输入能力改变现有设备。实际修正及差异验收接§36。

## 36. QA-5组件库滚动与统一网格行高（2026-11-01）

### 36.1 固定范围与尺寸合同

本节承接用户新反馈：从控件上起手无法滚整页，以及改变预设尺寸后控件内部意外上下滚动。
仍属于共享控件／组件库适配，不接管自定义HTML、AI生成或拖动编辑器，不改Manifest、执行链或设备任务数据。

**行高继续只有一个来源**：沿`panel-grid`原`--row-h`与`--fill-h`，2列×2行卡片保持正方形，矮块用同一个1行高度。
若单元行高为`u`、间距为`g`，2行高=`2u+g`，1行高=`u`，而不是把2行卡含间距的高度直接除2。
当前320 CSS px手机内面板294 px、间距12 px，`u=64.5 px`，2×2卡141×141 px；不另造64／78 px的矮块高度或增加stored rows。
组件内容须按可用高度排布，优先减留白、改横排并保留至少48 px触摸目标；不能以竖向内滚、缩小按钮或裁掉选项兜底。
真正长文本的textarea／长内容滚动与意外卡片滚动分开，保留必要的内容访问。

### 36.2 已复现的因果链及首批修正

- 旧APK中，控件大触摸区`touch-action:none`，共享卡片又统一`overscroll-behavior:contain`。
  同一150 px纵滑，从色相滑条或WiFi卡标题起手页面位移均0；同一工具从外部段落标题起手为+141.9 px，排除注入工具完全不能滚动。
- 原ADC／日志模式分段按钮以64 px基础宽度换两行，内容104 px／可用81 px，产生内部纵向滚动。
  已改单横排、48 px最小目标、仅极窄／更多选项水平兜底；所有选项与grid不变。
- 组件库host给大触摸区提供`pan-y`，普通控制页仍保留原默认策略；共享卡片允许滚动继续传给整页。
  同时分离取消与正常松手：Range／Dial／Picker／Slide的取消不commit，Range及重复按钮恢复本次开始前local草稿、清理计时并隔离旧owner；Hold／Dpad继续安全释放。

首批有限回归：尺寸／滚动链6项红绿通过；手势初始10项中7红3原保护通过，首修10绿；
新增action Stepper取消丢草稿准确红测后，最终13项通过（含linear／dial原local保护），不累加重复项。
原SDK匹配APK `bb0efa87…fc6a2`安装后，原生可信事件路径确认色相滑条／普通WiFi卡起手，页面分别+161.8／+167.1 px，色相未误改。
早期工具目标未稳定、或CDP触点没有落到控件的记录均保留，不计有效通过，不降低滚动阈值凑结论。

### 36.3 单行内部预算补段

同次原生后测确认Hold虽安全释放，却被宿主约1 px微溢出截住手势：宿主client63 px，按钮64 px，局部scrollTop约1.185，页面位移0。
进一步只读测量仅限当前48个演示控件：9个单行宿主存在64～76 px内容挤入63 px的情况，另两行串口文本139／182 px。
这不是网格行高不统一，而是旧固定padding／窄卡竖排／多行输入最小内容预算与新行高不匹配。

本批最后补段固定为：从原`--fill-h`推导内边距预算，原卡片及必要叶子消费同一预算；单行Stepper保持横排，
紧凑两行多行文本将textarea和counter／send并列，保留按钮、草稿及长文自身访问。只补这些已量测边界，
不把整库支持、其它设备／主题／WebView矩阵加入前置。最终有限回归及未过项手机补验结果取本节后续记录。

### 36.4 最终有限结果与停止扩展

内部预算沿原`--fill-h`推导`clamp(0px, (fill-h - 60px) / 2, 14px)`，60 px为56 px滑动轨道＋4 px边界预留，
不是新的行高。基础卡片和Toggle／Slide／Hold／Stepper消费该预算；单行Stepper横排，紧凑两行textarea与counter／send并列，三行不改。
原生手机触控补段（仍为组件库演示数据）中，Hold页面位移+162.4 px且安全释放为false；水平色相210→287、页面位移0，实际trusted起点／终点均匹配原控件。
这次后续只读尺寸检查准确留下Action约1 px、三行Keys约2 px两项余量失败，不把整轮标绿，也不重发已通过手势。

最后仅修这两处：Action内部`.op`消费同一padding预算，删去单行固定14 px覆盖；Keys用独立`--ui-keypad-rows`表达实际按键行数，
从原高度扣除48 px键、8 px行间距及34 px标题／gap／边界预算，不覆盖几何`--rows`。全部按键、目标大小、原invoke／select保留。
新增有限用例分别为滚动／模式6项、取消13项、尺寸预算9项及最后余量7项，**35个不同用例分批已有通过记录**，不累加重跑；
尺寸预算首轮CSSOM把0规范为0px的测试断言修正后只复验受影响3项，原夹具类型错误及所有有效红／绿日志保留，不当业务红测。

最终唯一尺寸候选APK SHA-256为`1dd4ce59935ddf19c2449f2ad4bae123126929106f7207adf4143e8d25e6c524`，
原包名／签名／HTTPS／SDK2.4.4及1157项匹配Web资源通过；没有为同一冻结源另开重复构建队列。
各中间包仅为新增、真实复现的缺陷更新，不作全套验收通过；最终安装后**仅只读复查尺寸**，不重放此前手势或控制。
当前320×704 CSS px手机内48个演示控件：宿主`scrollHeight > clientHeight`计数0，内部非textarea／input的可纵滚溢出计数0；
所有单行外框高64.5 px，2×2方块仍141×141 px，ADC／日志模式从104／81变为81／81，4／5个选项完整；
六键遥控仍六键，模式与按键目标均不少于48 px，两行串口文本宿主139／139。长文textarea自身滚动未移除。

证据按批保留在App忽略目录`build/qa5-{height,scroll-cancel,height-fit,height-remainders}-20261101/`及
`build/qa5-{catalog,row-budget,final-fit}-live-20261101/`的ACL私有目录。
早期工具点位不就绪／触点未落在控件不算通过；有效原生事件、两项真实余量失败及最终零溢出读数均保留。
截图只作对应时刻的辅助记录，部分转场截图不作整页视觉通过；最终单行截图已检视。
本批没有多机型、横屏、深色或最低WebView验收声明。UI/UX技能用于纵向滚动与控件意图分离、48 px触点和尺寸内的内容可达性，不重设计界面体系。

手机已回首页、工具Network／CDP转发已关闭、常亮恢复原0，最终App保留；同账号7项secure及两份完整请求journal前后精确一致。
没有设备／服务／任务写入、刷板、配网、验证码、UNKNOWN重放、扩槽、SDK升级、提交或推送；TaskStore、云scope及原授权不动。
至此这两类组件问题按当前演示目录有限收口；不继续无边界补审计。§35所列真实text／数值设备执行仍需匹配能力，
不可拿UI演示替代，也不自动刷板。后续仅按新的具体复现或明确选择的编辑器交付范围推进，不默认重跑已过矩阵。

## 37. QA-5已知空文本的反馈纠正（2026-11-01）

### 37.1 本批最小范围

§36已收口，不恢复旧几何文档的BLE／布局队列，也不为§35缺少可写字段而改设备Manifest或TaskStore。
本批只处理§35原手机记录和当前真实模板可复现的一项反馈缺陷：`card.value === ''`被`TextWidget.last`显示为“尚未发送”。
空串是合法的已知文本值，不等于没有读数；现有宿主的value、draft及ACK是分开的，不增加任何推导状态。

最小合同：仅当`valueType === 'text' && value === ''`时显示“当前（空文本）”；undefined／null、数值0、空格原串及旧数值空模型保持原语义。
空草稿、点击发送或ACK本身不得把原当前值改为空；只有宿主传入的value变化才更新当前提示，不写“已清空／已确认”。
只改共享getter及新增真实模板回归，不改布局、发送、校验、权限、CAS／UNKNOWN、服务或固件。
先定向红测，再最小修正和有限绿测；本批不单为这条提示重建／安装App，不重跑§36手势／尺寸或既有配网／任务。

### 37.2 实施与有限结果

只调整`widgets/text/text-widget.ts`的`last`两处分支：undefined／null仍显示“尚未发送”，
空字符串按原字段类型区分；text显示“当前（空文本）”，旧数值空模型保持原文案。
普通文本、空格原串与0沿原`display`展示，不trim、不修改value或draft，不宣称“已清空／已确认”。
原`DeviceV2Page.value`仍从实时／事件／字段值取得读数，原发送完成流程不把draft或ACK写成State，未新增第二套状态。

新增`widgets/text/text-feedback.spec.ts`8项，使用真实Angular模板：已知空串、undefined／null未知、空格、0、旧数值空模型、
草稿→显式commit→宿主新值的分离，以及共享只读预览的同义显示／无发送按钮。
有效红测3失败／5原保护通过；最小修正后**8／8通过**。无初始化、传输替身或手机ACK通过声明；
宿主新值只是组件输入模型，不伪造设备回执。复用原Angular／Vitest、单worker与既有WiFiProv源码alias，不安装或升级依赖。
日志、原命令及退出码保留于App忽略目录`build/qa5-text-feedback-20261101/`；独立只读审阅及`git diff --check`通过。

UI/UX技能的定向检索首项不匹配，窄化后命中Forms／Submit Feedback；仅用于当前读数与提交反馈语义清晰，
value／draft／ACK的业务边界仍以源码为准。未改HTML、布局或样式，不作新视觉、多机型或手机安装声明。
App两份较早几何／尺寸文档为已入HEAD的历史记录，不将其中旧BLE／发布next或78／84 px尺寸恢复为当前队列。

本批源码到此冻结；手机仍为§36最终`1dd4ce59…6c524`，新提示尚未安装，不为一条文案独立排全量打包。
下一次有必要的匹配App构建可合并携带，仅补这个当前值显示短验，不重复提交／滚动／48项尺寸矩阵。
未操作账号、授权、TaskStore、scope、UNKNOWN、原完整日志、手机、开发板或服务；未部署、刷板、发码、重放、升级SDK、提交或推送，无关本地修改保留。

## 38. QA-5组件库演示时钟的缓存／后台回收（2026-11-01）

### 38.1 本批最小范围

§36滚动／尺寸及§37文本反馈结果不重跑。当前源码的组件库在构造时启动3秒演示时钟，只在销毁时清理，
Ionic缓存离页不销毁，因此仍会推进演示数据、有界历史并markForCheck；后台也未接入现有可见性服务。
这是可复现的无效工作，不据此宣称所有卡顿的唯一根因或无界内存泄漏。

只复用`AppVisibilityService.active`：页面已进入且App前台才持有一个演示时钟；离页／后台立即清理，
返回从一个新的3秒周期开始，不追赶补播。重复进入／恢复不叠加定时器，销毁退订并围栏迟到回调。
保留原sections、草稿、选中项、设置及历史；不接DeviceUiPort或设备通信，不重构tick渲染，也不改原视频实现或声称媒体全部回收。
先用实际页面类及假时钟复现再补有限回归；必要的一次匹配App包合并携带§37，只验演示时钟生命周期及空文本提示，
不重跑48项尺寸／手势、不造真实设备ACK，不操作固件、Manifest、TaskStore、scope、UNKNOWN、授权或服务。

### 38.2 软件实现与有限回归

只改组件库页面：移除构造时的常驻interval，直接订阅既有`AppVisibilityService.active`，由entered／active／destroyed共同决定时钟。
原3秒频率、tick算法、120条历史上限及视频leave／enter调用不变；停止时递增代次并clear，恢复创建新周期，
重复前台通知不重启计时；销毁退订并封住旧回调。没有引入通用调度器或设备／账号依赖。

新增`component-preview-lifecycle.spec.ts`10项，用空模板隔离下的**真实Page类／生命周期与演示数据**：
入页前无工作、前台原周期、缓存离页、后台暂停／新周期、隐藏页不随App恢复、后台进入、重复信号、草稿／选择／设置／历史保留、旧代回调及销毁退订。
有效红测8失败／2保护通过，最小修复后10／10通过；旧销毁原本已clear，不把红测泛称为全部销毁失败。
这不是48组件真实模板、原生媒体或设备ACK验收。复用原Angular／Vitest、SDK及单worker；没有安装或升级依赖。
日志与原命令保留于App忽略目录`build/qa5-catalog-lifecycle-20261101/`；独立只读终审无阻断。

UI/UX定向检索及一次窄化均未找到本问题的有效匹配，只采用技能内置的状态保留／主线程预算一般原则；
实际生命周期合同以Ionic缓存和现有App可见性源码为准。不改视觉结构、触点、主题或尺寸，不补新视觉矩阵。
匹配App仅安排一个构建队列，携带§37；安装及短验结果单列后段，不用软件通过代签手机。

### 38.3 匹配安装与有限真机结果

账号7项secure、两份完整请求journal及logcat先私有保留；与§36最终账号／journal精确一致。
沿用既有单队列prebuild→production/cloud-beta→sync→原SDK Gradle，四阶段均成功，冻结源码、原生输入、HEAD及index不变。
唯一新APK SHA-256为`41bd3ae863d634686ad82ed0c44db6c4d10c293236a595ac163db055e2b2de34`，
原beta包名／签名／HTTPS／SDK2.4.4和1157项匹配资源核验通过，原数据保留安装并读回摘要；未重建第二个包。

只做组件库演示：透明透传观测器窄匹配本页3秒时钟，保持原回调和调度，不调用tick或更改等待条件，
有界记录3个自然前台周期；路由离页和真实Android HOME后台分别观察超过3秒，active为0且无新增回调，
返回均创建一个新周期、没有即时补播，最大并行时钟1。原串口演示的已知空串提示为“当前（空文本）”，未提交或模拟设备ACK。
本次离页原DOM最终不再isConnected，**缓存同实例重入真机NOT_RUN**；仅离页停止／重新进入和原生后台恢复有限通过，
缓存页、重复通知及旧回调围栏取§38.2软件结果，不追加重路由或旧手势／尺寸矩阵凑通过。

证据保留在App忽略目录`build/qa5-catalog-clock-live-20261101/`的ACL私有目录，完整Network与logcat未删。
最终回首页、目标时钟0，透传观测器恢复、Network／CDP转发关闭、常亮设置精确恢复原0；
账号7项secure和两份完整请求journal前后精确一致，设备HTTP写0。没有刷板、配网、设备／任务变更、授权修改、UNKNOWN重放、发码、SDK升级、提交或推送。
原TaskStore、scope、授权及无关本地修改保留；没有宣称视频资源、真实text／数值ACK、多机型或新视觉矩阵通过。

本批按最小范围收口，不重新排队§36已过项目。后续仍只处理新的具体复现或明确的编辑器交付范围；
真实text／数值执行需要现有设备具有对应可写能力，不为补样本改Manifest／TaskStore，不自动转入QA-6／7或旧发布／BLE队列。

## 39. QA-5剩余缺口与输入法确认键的误提交保护（2026-11-01）

### 39.1 剩余状态与本批选择

§34～38已通过的输入、滚动／行高、空文本反馈及演示时钟不重新排队。剩余项按性质分开：

| 项目 | 当前状态 | 最小后续条件 |
| --- | --- | --- |
| 真实可写text／数值输入的设备确认 | 缺自然能力，不能由演示数据销项 | 现有设备自然提供匹配能力时再单列；不改Manifest／TaskStore凑样本 |
| 缓存同实例重入的原生证据 | 软件覆盖；§38手机路由销毁，真机NOT_RUN | 有真实缓存路径时只补此差异；不重复离页／后台或重路由凑缓存 |
| HTML／AI／拖动编辑器整体交付 | 保留同事既有分工 | 只有明确范围或具体复现才安排，不接管整页重写 |
| 共享TextWidget组合输入误触发送 | §39.2源码／10项模板通过，§40匹配安装及本机候选流程有限通过 | 精确DOM组合分支仍取软件；不外推所有手机IME，不接设备执行链 |

当前单行Enter、多行Ctrl／Meta+Enter均直接`send()`，只判断`canSend`；已有配置inspector的组合输入保护不覆盖共享TextWidget。
本批最小合同：组合输入`isComposing`或旧环境确认键229不产生commit，不吞输入法默认行为，
`compositionend`只结束输入、不自动发送；之后用户正常Enter／快捷键／按钮仍复用原`canSend`及commit。
保留原draft／value／ACK分离、空文本／空格、空数值／显式0、只读／disabled及预览禁写，不trim或替换草稿。
只做必要键盘参数接线、共享提交门控及有限真实模板回归；不改布局、传输、权限、CAS、TaskStore、scope、UNKNOWN或账号。
先复现再修，不为这一小项单独全量打包／安装，不重复§36／38手机矩阵；本批结果仅软件，后续必要App包可携带。

### 39.2 实施与有限结果

共享`TextWidgetComponent.send`只增加可选键盘事件及`isComposing || keyCode === 229`早退；
原单行Enter、多行Ctrl／Meta+Enter共三个绑定传入原事件，按钮仍无参`send()`。不新增composition状态机，
不preventDefault、不设置固定冷却、不在compositionend补发；原`canSend`、草稿回传及宿主校验不变。
该修正同时服务原标准页及`src/custom-panel/main.ts`挂载的共享PageWidget消费者，不增协议或传输。

新增`text-composition.spec.ts`10项，用实际生产TextWidget／PageWidget模板，不override按钮或提交函数：
三个键盘入口分别覆盖组合中确认及compositionend后的229确认、原默认行为、组合结束不自动发送和后续正常Enter／按钮；
另覆盖readonly、disabled、显式整数0及共享预览inert。草稿仍由宿主echo，原输入对象／当前value不变，不伪造ACK。
有效红测**7失败／3原保护通过**，最小修正后**10／10通过**；这只证明事件报告isComposing／229时的组件合同，
不是所有手机输入法、多机型或真实设备text／数值ACK验收。§34／37／38已过矩阵未重跑。

复用原Angular／Vitest、WiFiProv源码alias和单worker，无依赖或SDK变更。原命令、红绿日志、退出码及独立审阅结果取
App忽略目录`build/qa5-text-composition-20261101/`，`git diff --check`通过。独立审阅确认现有布局／空串门禁等dirty是既有变更，不算本批新增修正。
UI/UX两次定向检索未找到有效的IME专门匹配，仅采用内置输入意图／反馈一般原则；事件及业务边界以本地源码和真实模板红绿为准。
没有HTML结构／样式或可见几何变化，不新增视觉矩阵。

本批软件小项收口，**手机仍为§38 `41bd3ae8…2de34`，新IME门控尚未安装**。
下一次必要App构建按原prebuild同时携带共享自定义运行时与主Web，只补这一输入法差异，不重跑滚动／尺寸／演示时钟或设备业务。
其余剩余条件仍取§39.1，不将自然能力欠证、缓存NOT_RUN或同事编辑器范围转成当前阻断，也不宣称QA-5所有场景全部验收。
本批未操作手机、开发板、账号、授权、服务、TaskStore／scope、UNKNOWN及原完整日志；未部署、刷板、发码、重放、升级SDK、提交或推送，无关本地修改保留。

## 40. QA-5匹配安装与输入法演示差异短验（2026-11-01）

### 40.1 本批执行合同与构建停点

按用户明确选择执行§39匹配安装，只在现有组件库演示检验输入法确认、组合结束不自动提交及后续显式Enter／按钮。
先私有保留账号／授权／两份完整请求journal及logcat，不改设备能力、TaskStore、scope、UNKNOWN或服务，不发验证码。
被动记录输入／组合／按键事件及当前演示值；必须区分手机原生IME、CDP受控输入和合成事件，不将任一演示结果写成设备ACK。

原单队列首次prebuild：device-template构建成功，但custom-panel严格AOT在三个键盘绑定处报TS2345：修饰键事件类型为Event，入口仅收KeyboardEvent。
这是§39原模板测试未覆盖的实际发行消费者类型问题，不把旧包或未完成APK算匹配。保留原失败日志／freeze，不自动重试。
最小修正仅将send可选入口接收Event，内部窄读KeyboardEvent的isComposing／keyCode；原三个绑定、门控、按钮及业务语义不变。
明确从custom-panel失败阶段续接，不重复已成功device-template；新freeze只允许这一个源码文件差异，原SDK及构建／校验入口复用，结果单列。

### 40.2 匹配发行消费者与安装结果

首次prebuild失败及原freeze保留，显式类型修正后续接custom-runtime→主Web→sync→APK均成功；
未重新执行device-template，也未重复§39原10项模板用例。两个实际严格AOT消费者通过，构建中源码、原生输入、HEAD及index保持各自冻结版不变。
唯一APK SHA-256为`adfc75e5c44dde1935a32a14d69cf4e8cbd8a00a403946f70e0161c5f1ab20a3`，
beta包名、原签名、HTTPS、SDK2.4.4、1157项同步资源及标准组件chunk／自定义运行时中的isComposing与229门控均核验通过。
原数据保留安装并读回同摘要，不清账号或日志，不为校验另构建APK；本批仅新增Event参数边界对齐，不变更业务门控、HTML结构或样式。

### 40.3 组件库原生输入流程的有限结果

只用原“组件库”明确标注的演示数据及单行“OLED文字”，不打开设备控制或改变能力。
首次转场控件横向位置尚未稳定，记录hit／visible=false且未点击；后续只读截图确认实际目标可见命中后才聚焦，不把转场截图认定为新布局缺陷。
保留原搜狗OEM九键输入法，不切换系统IME／语言，不替换send、onAction或事件处理器；被动记录目标输入、按键及原按钮事件。

| 操作及驱动来源 | 草稿 | 演示当前值 | 有限结论 |
| --- | --- | --- | --- |
| ADB实点手机键盘ni并按确认键 | HELLO ESniP32 | HELLO ESP32 | 输入法确认只改草稿，没有更新演示值 |
| 后续显式CDP Input Enter | HELLO ESniP32 | HELLO ESniP32 | 正常Enter仍更新演示值 |
| ADB实点hao并选中文“好” | HELLO ESni好P32 | HELLO ESniP32 | 原生候选输入结束未自动提交 |
| CDP触控实际命中的原发送按钮 | HELLO ESni好P32 | HELLO ESni好P32 | 显式按钮仍更新演示值 |

捕获11个可信浏览器事件、0溢出；isTrusted本身不等于手机键盘驱动，以上按真实驱动区分。
本机原生确认仅产生`Unidentified/229`＋`insertText`，**没有DOM composition事件、isComposing=true或Enter+229**；
所以只能关闭当前Sogou候选／输入完成不自动发送的用户流程，不能宣称compositionend事件或特殊Enter门控精确原生分支已覆盖。
这些分支保持§39软件结果，不在手机注入合成IME补签，不扩多行Ctrl／Meta矩阵，也不凭DOM更新宣称精确commit次数或设备ACK。

原账号7项secure、两份完整请求journal及授权精确保留；结束回首页，输入观察器移除、Network／CDP转发关闭、常亮恢复原0。
设备HTTP写0、MQTT Command0且捕获帧完整解码，原TaskStore、scope、UNKNOWN及完整日志未动，未操作开发板或服务。
原构建停点／续接、双消费者摘要、私有Network／logcat、11项事件及截图保留于App忽略目录`build/qa5-text-composition-live-20261101/`的ACL私有目录；
`demo-analysis.json`与最终终核分别记录证据边界及源码／HEAD／index／原生输入不变，没有删除旧失败或备份。
独立只读审阅一致。UI/UX技能仅用于输入意图、实际命中及反馈分离的一般检查，未找到专门IME检索匹配，不作为原生事件事实依据；
截图仅为对应演示时刻的辅助记录，不补视觉／设备矩阵。

本批限定安装和当前手机演示流程收口；自然text／数值设备ACK、缓存同实例原生欠证及同事编辑器范围仍按§39.1另列，不自动追加重复队列。
不改设备能力、不发验证码、不重放、不升级SDK、未提交或推送，无关本地修改保留。

## 41. QA-5剩余缺口与开关确认上下文的有限收口（2026-11-01）

### 41.1 剩余缺口与本批最小合同

§40安装及当前Sogou候选流程冻结，不为DOM组合事件补采样。真实可写text／数值ACK仍需自然能力；
缓存同实例真机欠证须真实路径；同事HTML／AI／拖动编辑器保持原范围，三者均不转成重跑已过矩阵的前置。

本批只选择新的共享开关复现：confirm首次点击准备“开”，3秒内宿主State已变“开”，旧armed仍在，
下一点击却可直接发“关”；短暂disabled或更换绑定也没有使旧确认失效。Tile／Toggle共用同一base，现有回归没有覆盖这组变化。
最小合同是将确认限定于当前控件／端点及报告状态：状态、绑定、禁用或confirm设置变化时撤销旧确认，不自动发送；
恢复后要重新显式确认，同状态的普通对象刷新、文案／显示变化不应重置有效窗口。保留原3秒超时、正常两击及滑动自身显式确认。
只修共享确认所有权，不接App／设备鉴权、不改State／ACK／命令链或设备能力。

用户追加的界面名称清理同时完成：只将主题caption的“米家浅色／深色”改为中性“浅色／深色”，
保留natural／electronic／dark ID、主题名称、旧映射、视觉样式及保存配置。检索到的LAN发现Home Assistant等是实际协议服务标签，
登录及真实协议／服务商名称不当作外观对标文案混删，协议标识不改。
先补真实模板红测与少量主题兼容检查，修正后有限回归；不为这批源码小修单独重建安装，不操作账号、授权、TaskStore、scope、UNKNOWN或完整日志，不升级SDK、提交推送。

### 41.2 最小实现与有限证据

仅共享`SwitchWidgetBase`保存本地确认快照（widgetId、绑定key、字段id／key、原报告value）；
继承的ngOnChanges与原toggle入口撤销不匹配快照，禁用和关闭confirm也清理timer／armed，不发送动作。
相同状态／绑定的普通对象刷新、昵称或display变化不延长或取消原3秒窗口；正常二击及滑动已有confirmed入口保留。
超时闭包只清自身快照，集中清理用于提交／失效／销毁，没有新增授权服务、传输或通用状态机。
这里是本地UI确认意图，不是设备授权、持久任务或CAS；测试里的新State只是宿主输入模型，不伪造设备ACK。

新增`switch-confirmation.spec.ts`13项实际Tile／Toggle／Slide／PageWidget模板，未替换提交函数：
状态反转、禁用往返、控件／端点／字段ID重绑、confirm关闭恢复、同状态外观刷新保留原窗口、超时／销毁、滑动显式确认和预览inert。
初轮9项受测试环境缺matchMedia影响，原夹具失败日志保留、不计业务红测；只补齐浏览器特性替身并复验这9项，未重跑另外6项。
两段有效红测覆盖9项业务失败和6项原边界；生产最小修正后，13项开关＋2项主题检查**15／15通过**，不累计重复执行次数。
旧timeout回调identity隔离由源码独立审阅确认，没有额外声称已手动触发该迟到回调用例。

主题仅两处caption改为“浅色 · 圆润磁贴”“深色 · 低光舒适”，`natural/electronic/dark`、中文名称及legacy归一化保持。
源码界面检索未再发现米家／小米等对标名称；不扫描删除历史文档、回归断言或实际协议／服务名称。
2项新检查覆盖中性caption／无对标引用及保存ID／名称／旧映射，不代表已装手机显示新文案。

原Angular／Vitest及既有WiFiProv源码alias复用，不安装依赖；只对受影响共享运行时做一次原工具链严格AOT，
输出至本批独立忽略目录，不pack、不覆盖默认dist、不sync、不生成／安装APK，不把它称为匹配发行包或主App完整链接。
日志、原命令／退出码、独立审阅及源码freeze保留在App忽略目录`build/qa5-confirm-and-theme-20261101/`，
严格编译及最终终核确认源码、HEAD／index、原生输入及其它App本地修改保持；`git diff --check`通过。
UI/UX检索命中禁用状态一般规则，仅用于不可交互与确认意图一致性；实际失效合同以源码及真实模板红绿为准，无样式／几何或新视觉矩阵声明。

### 41.3 当前收口与下一批边界

本批通用源码问题和主题对标文案可有限收口，不等于QA-5所有平台／场景已验收。**手机仍为§40包，本批两项尚未安装**。
下次确有必要的匹配App包合并携带开关确认及中性主题，只补新差异，不重采IME、滚动／尺寸、演示时钟或设备业务；
真实设备text／数值ACK、缓存同实例欠证及同事编辑器范围仍取§39.1，不作为追加无边界微修或默认QA-6／7队列的理由。
本批未操作手机、开发板、账号、授权、服务、TaskStore／scope、UNKNOWN及原完整日志；未部署、刷板、发码、重放、升级SDK、提交或推送，无关本地修改保留。

## 42. QA-5匹配安装与开关演示可达性短验（2026-11-01）

### 42.1 本批最小合同及入口边界

按用户明确要求保留账号、授权及完整请求journal后，构建安装一次匹配内测App，只验§41新差异，不重采IME、不改设备能力。
先核对实际入口：原组件库只有外观inspector，绑定字段标为只读，没有State／disabled输入；Boolean demo无live／fires，
普通tick还保留该卡对象引用，不能借其它卡刷新冒签同值OnChanges。改variant销毁叶子也不能代签同实例绑定／禁用失效。
原主题按钮只显示自然／电子／暗夜，不显示caption，中性caption先以匹配APK的实际资源检查记录，不谎称该页已展示caption。

正常UI能做的是在原inspector启用confirm、原同一Tile首次确认后修改标题形成同状态新card，再在原3秒窗口内二次确认，
过程中只用原DOM事件／控件，不替换私有Angular对象、处理器、Card元数据或计时器。操作时限／可见性不满足则原样停止，不放宽或重复凑通过。
State／绑定／禁用差异如需完整真机覆盖，须明确使用临时隔离演示夹具、复用包内公开sandbox输入，与普通组件库路径分开；
已向用户列出该选择，未确认前不加载夹具、不增加生产测试入口，也不把§41软件结果改签手机。

### 42.2 一次匹配构建与保留数据安装

沿原脚本完成一次prebuild→production,cloud-beta→原生同步→offline单worker beta APK队列，没有重复构建。
安装前私有保存7项secure及两份完整IndexedDB请求journal、原logcat，核对同一账号后以保留数据方式安装，没有清数据、切号或发码。
APK包名`iot.diandeng.tech.beta`，SHA256为`f1ed9b69477568a0b31efd654fc70802e58d4cf32301e7589a5c53a4979160ce`；
签名仍`a76639ec…d5d210`，HTTPS、原生注册表和1157份匹配Web资源核对通过；WiFiProv仍SDK2.4.4，esbuild仍0.28.2。
标准控件chunk及共享sandbox runtime均含§41确认快照实现，APK资源含中性`浅色 · 圆润磁贴`／`深色 · 低光舒适`，不含旧两处品牌caption。
这是制品匹配检查；没有重跑§41的15项、IME、尺寸、滚动、配网或设备业务。

### 42.3 原组件库短验与准确覆盖边界

仅打开现有组件库演示页，对原“继电器 1”Tile使用原contextmenu、inspector的二次确认／标题控件及click；
事件由DOM受控驱动（`isTrusted=false`），没有读取／替换Angular私有对象、handler、Card或计时器，没有新生产入口。
各交互点先通过可见性／命中检查，同一Tile DOM及widgetId保持；首次点击后仍false／“再次点击确认”，
原标题input形成同状态外观刷新后仍false／“再次点击确认”，第二次点击才变true／“已开启”。
二次点击距首次约165.5ms、结果约179.8ms，在原3秒窗口内；只有演示值，不宣称设备ACK或精确动作发送次数。

| 项目 | 本批结果及限制 |
| --- | --- |
| 同状态外观刷新保留确认 | 原UI受控事件PASS；不等于人手触控／长按验收，也不单独证明expiry没有延长 |
| State／绑定／禁用变化后重新确认 | 普通组件库无相应输入，原UI路径NOT_RUN；后续获准隔离演示取§42.5，不能混签普通UI |
| 中性主题文案 | 匹配APK资源PASS；该页只呈现自然／电子／暗夜，不显示caption，caption可见短验NOT_RUN |
| 动作及设备隔离 | 被动采集范围内设备HTTP写0、MQTT Command0，MQTT解码完整；不接设备、不改能力 |

截图、逐步时间／几何／同DOM记录及完整Network／logcat保存在忽略且受限ACL的本批私有目录，不纳入Git交付。
没有为了完成表格切换variant销毁控件、注入私有状态或加载未选择的隔离夹具。

### 42.4 收尾及下一步

结束返回首页，停止Network观察并关闭CDP／自身9479转发，USB常亮恢复原0；安装后、离页后及最终账号7项secure／两份完整journal与安装前精确一致。
最终终核App全部非忽略源码、原生输入、HEAD及index与构建前相同；本批没有生产代码新修改，未暂存、提交或推送，保留全部无关本地修改。
没有开发板、服务、设备能力、TaskStore／scope／UNKNOWN写入或SDK升级，没有重采IME和重复已过项目。

**初批可达短验已收尾，不标成全部请求已验收。** 后续用户已选择临时隔离演示，执行结果取§42.5；
与普通组件库路径分别记录，不重新构建、不接设备、不扩展产品测试入口，不默认追加矩阵或进入QA-6／7。
UI/UX技能仅用于确认意图、禁用语义与可见反馈的检查；没有新视觉设计或扩大编辑器范围。

### 42.5 获准隔离演示补测及证据分级

复用同一已安装APK的`custom-panel-runtime/runtime.json`，实际资源SHA与§42.2匹配；临时iframe明确标注“隔离演示／非设备ACK”，
使用原`allow-scripts` opaque sandbox及restricted CSP（`connect-src 'none'`），只经父窗口真实`postMessage`传入两张虚构Boolean card、
原子组件公开`field`属性切换绑定，confirm经公开options启用；不接DeviceUiPort、不调用`window.blinker.setValue/invoke`、不自动回填State。
控制为`isTrusted=false`的DOM点击，保留同一atom、PageWidget及Tile DOM；不读取／替换Angular私有对象，不改组件、计时器或正式页面。

| 隔离演示项目 | 实际结果及证据限制 |
| --- | --- |
| State反转 | 首击A=false→公开输入A=true→旧确认撤销→新首击无action→新次击仅A/toggle/false，第一轮顺序断言已执行；逐步数据未在后续停点前落盘，有限证据，不声称有逐步时间／截图 |
| 绑定A→B | A/B全程存在、同Tile通过公开field切换→旧确认撤销→首击无action→次击仅B/toggle/true，第一轮顺序断言已执行；同上保留逐步记录欠证，不重跑补样本，不拆称单一字段比较项独立通过 |
| 禁用及恢复 | 第一轮夹具要求disabled也必须hit，违反现有`pointer-events:none`而停止，不是新产品缺陷；保留原源代码、停点和清理记录，仅修夹具门禁并显式补此一项，完整逐步证据PASS |

禁用补验：旧首击后约99.8ms即`aria-disabled=true`／`pointer-events:none`且确认撤销，不是自然3秒到期；
对禁用Tile的受控程序click无action（不冒称人手命中），恢复后仍未确认，新首击仅等待确认，约332ms后次击才产生唯一`qa.demoA/toggle/true`演示消息。
全程同DOM，新确认不改变报告值；没有设备ACK／真实State、物理手势、完整视觉矩阵或expiry不延长的新增声明。
首轮停点及有界Network／logcat完整保留；补验按步持久保存checkpoint及两张截图，不覆盖失败、不伪造首轮逐步数据，不自动重试或重跑State／绑定。

实际opaque／localStorage阻断通过，未注入Capacitor JS runtime；只读typeof发现Http／Cookies原生接口对象仍存在，**不声称原生桥物理不存在**，
夹具未调用原生方法。两次采集各自设备HTTP写0、MQTT Command0且解码完整；两次均移除iframe及消息监听器、回首页、关闭自身CDP／9479转发并恢复常亮原0。
最终账号7项secure／两份完整journal、App非忽略源码／原生输入／HEAD／index及已安装APK均与前批匹配；TaskStore／scope／UNKNOWN未操作。
只有忽略目录测试夹具及本节文档变化，不重建／安装、不发码、不改设备能力、不升级SDK、提交或推送，无关修改保留。
**本批有限差异补验结束**；原组件库入口、隔离演示、软件模板和设备ACK四层证据不混用。既有精确欠证保留，不为日志补样本默认追加队列。

## 43. QA-5剩余缺口与动作长按的最小收口（2026-11-01）

### 43.1 剩余状态及本批范围

§42.5不重采：State／绑定保留顺序断言及逐步记录欠证，禁用补验和原UI同状态刷新结果冻结；中性caption只有制品核验，不为展示造入口。
真实可写text／数值设备ACK仍需自然能力，缓存同实例仍需真实缓存路径；HTML／AI／拖动编辑器整体交付保持同事分工。
这些不是追加刷板、设备能力变更或无边界日志矩阵的前置，本批只处理已有共享控件的具体新复现。

选定Action长按：原`holdEnd`只清共享holding布尔，不取消旧800ms回调；t0按下→t400取消→t500重新按下，旧回调可能在t800把新手势提前提交。
最小合同是每次长按归属独立待提交意图，取消／释放／离开及销毁撤销；绑定、禁用或hold选项失效后不能恢复旧长按。
同绑定外观刷新仍保留原截止，正常按满800ms及普通轻点复用原invoke链，不改动作／ACK／State协议及既有视觉反馈。
先用原模板事件和受控时钟复现，再修Action局部并补有限回归／定向编译；不重复开关、IME、滚动、尺寸或手机日志采样。
Range／Stepper跨绑定草稿是只读发现的另一个候选，尚未红测、不混称已修，涉及多个手势消费者，本批不捆绑扩大范围。
保留账号、授权、TaskStore／scope／UNKNOWN及完整日志；不操作手机／设备／服务、不升级SDK、不提交推送，无关修改保留。

### 43.2 原模板复现、最小修正及有限回归

新增`action/action-hold.spec.ts`，只通过真实按钮模板事件、公开setInput／output及受控时钟验证，不调用私有run、不接设备。
初次11项检查中7项业务断言失败：circle／card取消后新手势各提前提交1项，widget／endpoint绑定上下文／field-id变更各1项，
disabled往返和hold关闭／恢复各1项；不是测试夹具失败，其余4项保护原有行为。

仅`ActionWidgetComponent`新增PendingHold及独立timer：每次长按保存widget／endpoint／field身份，旧回调须匹配本次归属和当前有效输入；
holdEnd清timer及归属，原pointerup／leave／cancel全部复用；ngOnChanges在绑定、禁用或hold选项失效时撤销，销毁同样撤销。
重复down不新建计时；同绑定改标题／display不重计时，仍在原799／800ms边界触发。原invoke、800ms门槛、650／1600ms本地反馈及模板／CSS保持不变，
没有新状态机、公共协议或动作重试，反馈不冒充设备ACK。endpoint回归同时改endpointKey和field.key，不拆称fieldKey独立分支已测。

修正后11／11通过：两种外观取消后须新按满800ms；绑定／禁用／hold选项失效后须新手势；
同绑定原截止、重复down、销毁无invoke及普通轻点行为保持。共享标准页与自定义运行时复用同一Action实现，未接管同事编辑器。
沿原脚本完成共享运行时单目标隔离严格AOT，输出只在`build/qa5-action-hold-20261101/runtime-aot`，无pack、默认dist覆盖、sync或APK安装。
原红绿日志、命令／退出码、源码摘要及独立审阅保留在本批忽略目录；终核除选定Action源码外App其它非忽略输入、HEAD／index均不变，新测试未暂存。
UI/UX专门检索未获得匹配此旧timer问题的有效结果，仅采用内置可取消交互一般原则；业务结论来自模板红绿，不新增视觉／多机型通过声明。

### 43.3 本批退出及下一批最小范围

本批源码问题已收口，**手机仍§42包，本次Action修复尚未安装**；下次确有必要的匹配App包携带即可，不为这项离线修复单独重复安装采样。
自然能力ACK、缓存同实例、§42逐步记录欠证及同事编辑器分工继续按原口径单列，不扩大设备能力或借销毁重建销项。
下一批首选先用原动作Stepper模板复现“原字段草稿41→换绑到同报告40的新字段→发送继承旧草稿”的候选，
确认后只处理旧绑定草稿不可跨目标；不顺带改变正常State跟随或重写全部拖动消费者。此项尚未执行红测，不提前标成已修。
Hold按压即发true的安全释放与Action待触发timer不同，仍需先明确原目标释放合同，不照搬取消本地状态，不与本批捆绑。
本批没有操作手机、账号、授权、设备能力、TaskStore／scope／UNKNOWN、服务或完整日志；没有SDK升级、发码、重放、提交或推送，无关修改保持。

## 44. QA-5动作Stepper跨绑定草稿的最小收口（2026-11-01）

### 44.1 执行合同

按§43.3先用原动作Stepper按钮模板复现：A报告40，经增加形成未发送草稿41，保留同一控件DOM换到同报告40的B，再显式发送不能继承A的41。
范围只含Stepper旧／新模式有一端为action时的绑定身份变化，保留同绑定外观刷新草稿及原报告值变化的State跟随；不改其他Range消费者的自动更新策略。
需要一并撤销旧Stepper连按及settle回调，避免只清local后旧400／90／1500ms回调再次影响新草稿；旧sent提示也不跨目标继承。
先真实模板红测，确认后只在Stepper接入身份失效，基类最多提供被动取消钩子；不新增协议、公共执行器或编辑器能力，不改其他手势合同。
不将旧pointerup在新手势之后到达的跨指针矩阵纳入本批；没有手机／设备采样，不改能力、账号、授权、TaskStore／scope／UNKNOWN或完整日志，不升级SDK、提交推送。

### 44.2 原模板红测、最小实现及有限结果

新增`stepper/stepper-binding.spec.ts`，以原增加／发送按钮、公开setInput及output复现，同一增加按钮DOM保持，不改设备能力或内部草稿值。
初次10项中7项业务失败、3项通过；将前三失败用例的显式发送断言前置，仅定向复查这3项，确实收到`range41`而非新目标的`range40`，
其余7项未在该轮重复执行。原失败日志保留；这是实际组件提交值复现，不只是显示残留，也不是设备ACK。

Stepper保存widgetId／endpointKey／field id／key及action模式的标量快照；仅旧／新有一端为action且身份或模式变化时，
调用Range新增的protected `resetInput()`清旧settle、repeater timer及归属、linear／dragging和local，并清sent及其timer，然后复用`super.ngOnChanges()`。
钩子不挂入Range通用更新，实际唯一调用为Stepper；不改其他消费者、原commit／send／repeatStop、模板／CSS、正常同绑定State策略或编辑器。

修正后10／10通过：三种绑定变化不继承41，同绑定标题／display刷新保留41；换绑后旧连按及旧释放不提交，之后新按压／显式发送正常；
action→property不把旧按压转成属性指令，property→action的新草稿不被旧1500ms回调清除，两个模式同绑定报告更新仍跟随55；旧sent提示不跨目标。
endpoint样本组合修改endpointKey／field.key／id，property→action样本也同时换绑，不拆称每个字段／角色方向都独立覆盖。
旧pointerup在新pointerdown之后到达的跨指针场景未测、不外推；不增加其它手势矩阵。

原脚本复用完成共享运行时单目标隔离严格AOT，输出仅`build/qa5-stepper-binding-20261101/runtime-aot`，未pack、覆盖默认dist、sync或安装APK。
本批忽略目录保存红测、定向发送红测、绿测、原命令／退出码及输入摘要；独立只读审阅通过。
终核App仅选定Range／Stepper两源码和本批测试变化，原有本地修改、HEAD／index及锁文件保持；`git diff --check`通过。
UI/UX本地检索未命中有效的跨绑定草稿专门规则，只采用内置输入／状态保留的一般原则；业务合同及通过结论取真实模板，不作新视觉或多机型声明。

### 44.3 收口边界

本批限定源码问题收口；最近核验手机仍§42包，§43 Action及本批Stepper修复均未安装，软件output和设备ACK证据分开。
后续若需要匹配App短验，一次构建合并携带这两项，只验新差异，不为每个小修单独安装、不重跑§42.5或采集旧日志。
自然能力ACK、缓存同实例及同事编辑器分工仍按原状态单列；不因这批局部修复自动扩大到所有Range手势、Hold安全释放或新功能队列。
本批未操作手机、设备、服务、账号、授权、TaskStore／scope／UNKNOWN及完整日志；未发码、重放、升级SDK、暂存、提交或推送，无关修改保留。

## 45. QA-5合并匹配安装与Action／Stepper新差异短验（2026-11-01）

### 45.1 本批最小执行范围

按§44.3一次构建合并携带§43 Action及§44 Stepper修复，先保留当前App账号、授权、两份完整请求journal及logcat；
核对原SDK／锁文件、包名／签名／HTTPS和共享消费者，保留数据安装，不操作服务、开发板、TaskStore／scope或UNKNOWN。
真机仅复用已批准的临时隔离演示方式，虚构Action／Stepper字段经包内公开sandbox输入，动作有界记录但不转发设备。
只验长按取消后新手势不被旧800ms回调提前提交，以及Stepper同绑定草稿保留、换绑不继承旧草稿／显式提交仍正常。
Action用原模板受控pointer事件及自然有界时钟，Stepper可用原spinbutton的ArrowUp与原发送click，不覆盖IME，不替换原计时器／原生捕获／Angular私有对象。
逐步证据在断言前落盘；卡顿错过有效观察窗口则原样NOT_RUN并保留，不自动重试、不重跑§42.5、配网、开关或既有回归矩阵。
结束移除夹具／消息监听器，回首页并恢复原常亮配置，终核账号／journal、源码／HEAD／index及自身调试转发；暂不提交推送。

### 45.2 一次合并构建与匹配安装

先以ACL私有目录保留7项secure、两份完整请求journal及logcat，当前账号／授权及journal与§42终核精确一致。
复用原脚本单队列prebuild→production,cloud-beta→原生同步→offline单worker APK，各阶段源码／原生输入／HEAD／index均保持；没有重开队列或自动重试。
新APK包名`iot.diandeng.tech.beta`，SHA256 `df0dc68be51ede9e2427558b3bfb68b2d8a12c060f1740d71d615c9216a8cdae`；
签名仍`a76639ec…d5d210`，HTTPS、原生注册表、1157份匹配Web资源及标准／sandbox两处共享实现核对通过。
原WiFiProv SDK仍2.4.4、esbuild仍0.28.2；保留数据安装后读回APK摘要一致，未清数据／切号／发验证码，没有设备或服务部署。

### 45.3 限定隔离演示结果

仅用包内实际runtime及公开state／field接口，虚构Action／两个数值字段，保持同Action／Stepper／PageWidget DOM；
原模板pointer事件、ArrowUp及click均明示`isTrusted=false`，不读私有Angular对象、不改时钟／PointerCapture、不接DeviceUiPort、不自动回填设备State。
执行前只修夹具对卡顿超窗的结果分类：先核自然观察窗再断言，错过窗记NOT_RUN，不放宽阈值。本轮只执行一次且PASS，无重试。

- Action：第一次约400ms取消、约100ms后新按；中间观察旧手势已876.3ms、新手势347.6ms，仍holding且零动作。
  新手势876.0ms的后续观察为等待结束／本地run状态，只收到一条invoke；这是自然观察点，**不称精确第800ms发送或设备执行确认**。
- Stepper：A原ArrowUp形成41无动作，同绑定实际标题／display刷新仍41；公开field切换到B，同DOM下aria及可见读数回到40且不自动发送。
  原发送按钮仅记录B/range40；再原ArrowUp形成41无新增动作，显式发送仅记录B/range41。共三条消息均为演示事件，不是设备ACK／真实State。

每步在业务断言前持久记录，保留两张演示截图、完整Network及logcat；证据仅在App忽略的ACL私有目录，不随源码交付。
opaque／localStorage阻断及禁网CSP核对通过；Http／Cookies原生对象仍可见但未调用，不冒称原生桥物理不存在。
被动采集范围内设备HTTP写0、MQTT Command0且解码完整；不重采IME、开关、配网、日历／Start或其它已过矩阵，不作新视觉／多机型声明。

### 45.4 收尾及QA-5剩余边界

夹具及消息监听器移除，回首页、关闭自身CDP／9479转发并恢复常亮原0；安装后、离页及最终7项secure／两份完整journal均与安装前精确一致。
终核已安装APK与制品一致，全部App非忽略源码、原生输入、HEAD／index与构建前相同；本批没有生产代码新修改，保留无关本地修改，不提交推送。
账号、授权、TaskStore／scope／UNKNOWN及原完整日志保持，不操作开发板、设备能力、服务、SDK或其它权益。

**Action／Stepper两项本批有限收口**，不将隔离演示签成普通组件库完整操作或设备ACK。QA-5剩余仍分为自然能力text／数值ACK、
缓存同实例原生证据、原精确IME／多机型泛化欠证及同事编辑器交付，不因缺样本再造能力、改路由或补采§42日志。
下一批先整理QA-5整体有限收口及上述待自然条件／同事验证项；没有新的具体复现，不默认追加微修、安装／采样队列或进入QA-6／7扩展。
UI/UX技能仅用于取消意图、草稿归属和可见反馈检查，没有扩大布局或编辑器设计范围。

## 46. QA-5整体有限收口与剩余执行边界（2026-11-01）

### 46.1 决定与基线

**本轮共享控件／组件库范围有限收口，不等于自定义控制V2整体交付或全平台验收。** 本节仅汇总§34～45已保存的证据，未重新运行测试、构建、手机／设备采样或云端检查。
最后已核安装为§45 beta `df0dc68be51ede9e2427558b3bfb68b2d8a12c060f1740d71d615c9216a8cdae`，原签名、HTTPS、WiFiProv SDK2.4.4、esbuild0.28.2及1157份资源的核验沿用该节，不冒称本轮新鲜核对。
§43 Action和§44 Stepper“尚未安装”是历史停点，已被§45合并安装完成覆盖；不再排安装待办。软件、普通UI、受控隔离演示及真实设备ACK／State互不代签，不累计重复用例生成总通过数。

### 46.2 已完成并冻结的有限范围

| 项目 | 已完成及证据入口 | 不外推的边界 |
| --- | --- | --- |
| 共享输入提交一致性 | §34九项真实模板；§35手机空串／空格演示、原配置预览禁写，修正已随后续包携带 | 数值空输入／显式0的组件保护有软件证据，未因此补签真实可写text／数值设备ACK |
| 组件库纵滚、取消与统一行高 | §36共享滚动／取消／尺寸的有限回归及原生演示短验；当时320×704 CSS px下48个演示控件零意外纵向溢出 | 只覆盖原记录中的机型、控件及尺寸；不签所有主题、平台、媒体回收或编辑器拖动性能 |
| 已知空文本反馈 | §37八项真实模板；§38已安装并显示“当前（空文本）” | 当前值、草稿和ACK保持分离，不把提交本身显示为设备已清空 |
| 组件库演示时钟生命周期 | §38十项真实页面类软件回归，手机离页／真实HOME后台停止及返回新周期 | 实测路由销毁；缓存同实例只保留软件结果，不声称视频等全部资源回收 |
| 输入法误提交保护 | §39软件门控、§40发行消费者类型修正及本机Sogou候选流程有限通过，修复已安装 | 原生精确composition／isComposing／Enter+229分支欠证，不把合成事件当原生IME |
| 开关确认归属及中性主题 | §41模板／文案检查；§42同状态原UI、禁用隔离演示及制品核验，§42.5保留State／绑定有限证据 | 原UI与夹具分开；State／绑定逐步记录欠留、中性caption未在组件库显示，不升级为完整真机通过 |
| Action长按取消／归属 | §43十一项真实模板及隔离AOT；§45匹配安装和自然观察窗隔离演示 | 不是精确第800ms提交测量或真实设备执行ACK |
| 动作Stepper跨绑定草稿 | §44十项真实模板及隔离AOT；§45同DOM下草稿保留／换绑清理／显式发送演示 | 限涉及action模式的已选绑定路径；不宣称其它Range手势、跨指针尾事件或所有绑定字段独立分支已测 |
| 两处共享消费者及安装收尾 | §45标准页／sandbox同源实现随一个匹配APK安装，账号／完整journal、资源回收及冻结输入终核通过 | 不等于公司版本已合入、远端CI／发行完成、同事整页或真实字段ACK签收 |

这些项不因本轮只做文档而失效，也不重新执行§34～45已过输入、滚动、时钟、开关、Action／Stepper或旧配网／任务／自动化矩阵。

### 46.3 待自然条件的精确待验项

| 待验项 | 当前证据缺口 | 激活条件与最小补证 |
| --- | --- | --- |
| 自然可写text／数值设备执行 | 初始依据§35；后续§47已一次只读重核4台已发布能力，仍无匹配字段，未读回运行固件。演示值／共享output不能替代真实ACK和新鲜State | 现有设备自然提供合法、可安全验证的字段，并有匹配App／固件／服务及原授权后，只选一个字段、一个现有通道、一轮显式操作关联ACK／新鲜State；不刷固件或改Manifest凑能力 |
| 缓存同实例原生重入 | §38软件覆盖；实际手机路由离页已销毁，没有同实例原生证据 | 出现真实缓存复用路径后只补同实例停止／恢复差异，不改路由强造缓存，不重做已过离页／HOME后台矩阵 |
| 原生IME精确分支 | §40仅关闭本机Sogou用户流程，未捕获DOM composition／isComposing=true或Enter+229 | 自然使用的IME／机型出现相应事件或具体误提交复现后，保留驱动来源只验一个差异；不注入合成事件补签、不为日志重复采样 |

以上是能力／路径／证据条件，不是已确认功能失败，也不要求为了结束QA-5新增硬件能力、改授权或制造回执。

### 46.4 同事分工、独立范围及非阻断限制

| 分类 | 责任与状态口径 | 后续所需输入 |
| --- | --- | --- |
| 我们的修复范围 | 共享控件、宿主薄适配及已选执行链中的真实缺陷由我们定位并最小修复；不是把Bug转交测试同事 | 一条明确复现及预期／实际结果；按原链路修正、有限回归，未经复现不重写 |
| 同事整页交付范围 | 自定义编辑器、HTML schema、AI生成、拖动布局及配置预览的整体交付保持§34.1既有分工；本轮未读取远端最新进度，不代签完成或给同事重新分派人员／期限 | 明确提交／APK／Web版本、界面模式、一个可复现流程及必要截图／脱敏请求ID；不提供密码、token或设备Key |
| 功能验证／特殊适配 | 多机型、最低WebView、横屏／深色、iOS及拖动性能需明确平台与场景后由对应负责同事验证，我们处理确认的实际缺陷 | 固定目标设备／版本及必要性能判据后单独选一个代表差异；不默认新增全量矩阵 |
| 已知证据限制，非补日志任务 | §42 State／绑定顺序断言已执行但逐步数据欠留；中性caption只有实际制品核验且不在组件库展示 | 原样保留限制，不重跑§42、不补造数据或增加UI入口来凑可见通过 |
| 未选支持合同／风险候选 | 本节整理时Hold尚未选；用户随后已在§48明确选择并完成有限软件修正，结论只取§48。§44其它Range消费者及未选跨指针组合继续不在修复范围内 | 不把Hold取消简单等同Action丢弃；其它风险只有明确选择后才先离线复现，不混成待补样本 |

同事分工不代表本机完成了独立签收。`device-template`可信同源模板与V2 sandbox是不同路径，共享叶子修复不代替任一路径的整页交付。
§3.5原报告中的独立下拉／长枚举、配置中心预览可见性／手动布局性能、重复创建／offsetHeight跨页等，凡未被§46.2的具体证据覆盖，仍按原状态待明确版本、复现或交付范围；不随本轮有限收口统一标成已修，也不默认新增实现队列。
QA-6可选能力和QA-7高级平台／协议仍独立，不因本次收口自动开放任务准入、扩大设备模式或升级SDK。

### 46.5 下一批最小范围与停止条件

**本次整体整理已完成，不再将“再整理一次QA-5”排成默认下一轮。当前没有新增构建／安装／采样队列。** 后续按以下触发顺序只激活一项：

1. 自然匹配可写字段具备时，优先§46.3首项：先只读核对原能力／版本，选一个安全可逆字段、一条现有通道做最短ACK／新鲜State差异。
   数值0仅在字段允许且安全时选择；空数值保持App禁发，不绕门禁或把演示当设备。无新增能力、刷板、配网、旧任务／UNKNOWN重放或多通道矩阵。
2. 没有上述自然能力、但用户／同事提供了明确版本与一个具体复现时，先只读定位它属于共享控件还是整页编辑器；只对确认且在协作范围内的一项真实缺陷实施最小修正。
   不因此自动拉取／合并分支、接管编辑器或安装新包；只有修正确有新原生差异时才另定一次必要匹配短验。
3. 真实缓存／IME条件自然出现时，只补对应差异；多机型和其它支持合同须明确选择，不与上述单项捆绑。

若条件／具体复现尚未到达，保留当前有限收口及待验表，不重复采样、修改设备能力或追加无边界微修。另行明确选择的核心主线可以单独规划，本节不自动启动QA-6／7、发行或推送。
每项退出都要明确软件／UI／夹具／设备ACK证据层级，保留原失败和完整请求日志；不把条件不足或NOT_RUN记作PASS，也不为欠日志无限循环。

本轮仅更新本执行文档与唯一当前入口，未改产品代码、测试、锁文件或SDK，未操作手机、设备、服务、账号、授权、TaskStore／scope／UNKNOWN及完整日志；未暂存、提交或推送，无关本地修改保留。

## 47. QA-5自然可写能力的只读准入核对（2026-11-01）

### 47.1 本批最小合同

按用户本次明确选择激活§46.5首项前置：沿§45匹配App与当前账号，先保留原授权／两份完整journal及日志，
只GET一次现有库存、每台已发布且active的设备最多一次Manifest，使用原入口与无自动认证恢复的fetch，不读取Key或任务API。
只有Property、Read与Write齐备、text／数值类型且语义安全／约束合法，才作为后续真实ACK／新鲜State的候选；bool开关和只读uptime不算替代样本。
HTTP no-store仅跳过浏览器缓存，服务端持久Manifest不等于运行固件读回；active不等于在线，revision／fingerprint不冒充固件版本。
遇权限／网络／schema问题保留未知；没有自然候选则不发控制、不改能力、不重配或追加通道采样，保留待验，不升级SDK、提交推送。

### 47.2 实际版本与能力核对

沿现有登录账号及原App读取，不安装或构建新包；当前APK读回SHA与§45的`df0dc68b…8cdae`完全相同，
原生App信息为`iot.diandeng.tech.beta`／`1.0-beta`／build1，SDK2.4.4依据该匹配制品，不将项目package版本当原生版本。
只读库存1次、每台已发布active设备Manifest各1次，共5笔HTTP200，均保留request-id、始末时间及响应原文；没有认证恢复、请求重试或设备采样。
库存active4／已发布4／未发布0，覆盖本次已发布子集；D1～D4仅为本批库存顺序索引，类型均为diy，不据此映射COM口或板型。

| 本批目标 | 服务端已接受能力 | 目录版本与在线元数据 |
| --- | --- | --- |
| D1～D3，共3台 | `power` Boolean／Read＋Write＋Notify（access7）；`uptimeSeconds` UnsignedInteger／Read＋Notify（access5），不可写 | Manifest revision均1，fingerprint与各自库存相同；cloudReachable报告1台true、2台false |
| D4，共1台 | 只有`power` Boolean／Read＋Write＋Notify（access7） | Manifest revision1，fingerprint与库存相同；cloudReachable报告true |

按现协议text／数值type1～5及Write／Command权限识别，可写此类字段0；进一步要求Property且Read＋Write的候选也是0。
`uptimeSeconds`是只读数值，布尔power虽可写却不是本次缺口，均不拿来重跑已有控制充当text／数值验收。
这些是**本次HTTP取得的服务端最后已接受快照**；no-store不使其变成现场固件读回，presence的2true／2false也不是新完成的设备握手或新鲜State。
库存与Manifest均没有firmwareVersion字段；没有读Flash／串口／新Hello，因此运行固件版本／摘要明确留为未核实，不用revision1替代。

### 47.3 条件判定与保留终核

只读核对已完成；写验判定为`NOT_RUN_NO_PUBLISHED_MATCHING_PROPERTY`，**未执行真实ACK／新鲜State短验，不是功能失败，也不记通过**。
没有控制、任务、Key或能力写入请求，没有为缺候选而打开BLE／LAN、改变通道、修改Manifest／权限／额度、刷板或重配。
被动记录范围内设备HTTP写0、MQTT Command0且解码完整；不泛称后端缓存、路由或审计等常规维护物理零写。
原账号7项secure与两份完整请求journal前后精确一致，路由仍首页，APK摘要保持；Network观察／CDP及自身9479转发已回收，常亮恢复原0。
每笔响应、能力摘要、完整Network及前后logcat保存在App忽略目录`build/qa5-natural-capability-20261101/`的ACL私有目录，凭据不进入公开文档或Git。
独立只读复核核对了5笔响应的路径／设备关联、目录revision／指纹和字段权限，没有追加网络采样；原授权、TaskStore／scope／UNKNOWN及完整旧日志未修改。

### 47.4 下一步停止边界

本次只读准入核对到此结束；§46待自然条件及同事分工继续保持，没有新的默认代码／安装／采样队列。
只有收到自然匹配字段的新能力／版本材料或一个明确具体复现后，才按§46.5重新激活一项最短差异；
不重复读取本批未变化的4份目录，不为获得text／数值样本改固件或能力，不以现有布尔或演示事件代签ACK／State。
本批没有产品源码／测试／锁文件／SDK变更，没有刷板、部署、发码、任务／授权写入、暂存、提交或推送，无关本地修改保留。

## 48. QA-5 Hold原目标安全释放（2026-11-01）

### 48.1 最小支持合同与实施顺序

1. 一次有效按压冻结原字段、widget及控制会话；同会话换绑A→B时只尝试释放原A一次，绝不把false解释为B的Stop。普通State回显和同绑定刷新不取消按压。
2. pointerup／pointercancel／lostpointercapture／blur／销毁共用一次终态；其它pointer或键盘尾事件不得结束当前pointer按压。忙态不等于失权：按下在途时保留原队列，只有按下确认后才继续false；UNKNOWN／拒绝不自动续发。
3. 通道、页面、账号／设备owner或权限失效时撤销本地手势，拒绝旧会话尾事件；恢复后不重启true、不补发false，不伪造State、ACK或“已停止”。本批是App尽力释放，不保证失联后的物理停止，安全关键负载仍需设备侧独立保护，本批不新增执行协议。
4. 标准panel不再用当前绑定覆盖原目标；sandbox只携带内部手势关联，父frame以权威card／当前scope复核，只让已接受press对应的一次release穿过pending忙态，不放宽preview、active、ACL或任意false。字段快照／scope不是额外授权。
5. 按“合同→原模板红测→最小修正→有限绿测／原SDK严格AOT”顺序执行；不改设备能力、服务协议、分配器或SDK，不操作手机、设备、账号、TaskStore／scope／UNKNOWN或已有完整日志，不提交推送。

### 48.2 本批源码停点

原Hold只有pressed，无原目标或pointer归属；标准panel和custom runtime都在释放时读取当前card／field，存在A→B误投风险。
custom frame直接拒绝card.disabled，又会挡住按下pending后的正常false。DeviceV2Page已有确认后释放队列及UNKNOWN／生命周期围栏，复用而不重造。
先补真实组件／panel模板、frame消息边界及page会话围栏的离线差异，未运行前不标通过；AI页面排查待Hold完成后单列只读结果。

### 48.3 已确认缺陷与最小修正边界

- 原panel模板已红测复现`A,true → B,false`，其它pointer及键盘尾事件可提前终结按压；旧scope亦未被page拒绝。新合同的interactive输入尚不存在也作为独立红项保留，不把新增接口缺失另算一个历史产品Bug。
- frame消息边界独立红测确认busy释放被拒绝、带关联但无已接受press的false会退回普通写入、旧owner恢复后缺乏配对退休。首轮filter未选中frame，后续只补该目标红测，没有覆盖或删除原失败记录。
- 修正保留原目标快照与单次终态，page的控制scope含设备／账号会话版本／原command代际／Hold代际／Manifest。ready→ready通道切换、就绪或新鲜度丢失等旋转Hold代际；排队false也带原scope并在实际续发前复核，普通非Hold指令不改变原跟随策略。
- 新绑定B不可用不代表原A失权：同scope重绑先提出原A释放意图，最终仍由原A的当前ACL／字段／通道门禁决定；不能借此绕过通道失效。旧接口Legacy消费者只补最窄scope／目标不符拒绝，不能再按当前widgetId误投B；旧binding消失后原A是否停止保持未确认，不重造旧协议ACK或管理通道。
- 严格AOT首轮发现Angular键盘模板的`Event→KeyboardEvent`类型不匹配；入口改接Event、只窄读repeat，不升级依赖或放宽模板检查。失败及随后有限验证分别保留，不把JIT绿测代替严格编译。

软件结果、最终编译及无关修改保护终核完成后再追加终态；本批不以测试替身返回值签为真实设备ACK，也不保证失联／撤权后的物理Stop。

### 48.4 软件有限收口结果

最终冻结版本5文件有限测试**42项通过**：Hold原模板10、frame真实window消息边界18、page9（含原确认／UNKNOWN队列围栏）、原scroll取消1及Legacy public act／生命周期4。
其余61项由本批filter未选中，不冒称全套通过；首轮34绿、原模板红6／frame红16及新增carrier红1均保留，不累计成“新增通过数”或历史Bug数。
原SDK的`custom-panel-runtime`隔离严格AOT最终成功；首轮Event类型失败保留。最终编译期间App非忽略源码、HEAD及index逐项未变，不改锁文件／SDK、不pack/sync/APK/install。

V2标准panel可在同owner重绑时一次释放原A（包括B不可用）而不写B；busy仍沿原按下确认队列，pointer／键盘尾事件和重复取消退休一次。
ready→ready通道往返、就绪／新鲜度失效及旧scope排队尾事件均不向恢复后的会话续发。自定义runtime按原目标发消息，frame只使用权威字段及已准入的有界配对；这不是新设备协议或新授权。
Legacy只补目标／本地owner不符拒绝，不补已移除binding的原A写入，不冒称有协议session代际或ACK。失联／权限撤销后的设备物理停止仍不作保证；真实多端竞争、iframe原生沙箱与硬件ACK不在本批软件证据中。

日志及隔离AOT输出保存在App忽略目录`build/qa5-hold-release-20261101/`；没有连接设备、发控制／任务、读取Key、切账号、操作TaskStore／云scope／UNKNOWN或原完整请求journal。
Hold本批软件有限范围到此收口，当前手机仍§45原包，未部署或提交推送；接下来仅按用户追加只读梳理AI生成页面，不自动把它转为产品实现队列。

## 49. AI生成页面只读初查及最小后续范围（2026-11-01）

### 49.1 真实入口与已有保护

本节在§48绿测及严格AOT结束后才开始，未编辑AI相关源码、运行生成／保存接口、联网调用模型、构建安装或部署。
当前有两条不同链：`device-manager/:id/uic/ai`的组件模式使用`DeviceUicPage → InterfaceGenerationService`；
`uic/custom`及`device/:id/edit/custom`由同页承载`CustomPanelEditor`，生成HTML再进入sandbox。旧`device-template`同源Penpal宿主不混同sandbox交付。
Gateway实际`/api/v1/interface/workflow／refine／generate`及`packages/device-ui-agent`就在现有Service仓库，不另造生成服务或换Auth。

- 组件模式已有epoch／Manifest围栏、AbortController、草稿恢复、有限总截止／2 MiB体积及本地source／字段类型／尺寸校验；自定义模式已有190秒HTTP截止、128 KiB文档合同及显式取消。
- 两种预览均默认不交互且不接设备action；共享编辑权限仍走`canEditConsole`及服务复核，模型输出不是设备权限。原非Cloud `saveCandidate`和Legacy保存的expectedRevision／CAS保持。
- 本节只读源码，不重跑已通过的预览、配网、IME或设备ACK，不把历史测试未运行说成这次通过。

### 49.2 候选缺口、证据层级及优先级

1. **优先：生成owner与取消生命周期不一致，需先离线复现。** 自定义生成器仅以`fields()`序列判断能力变化（`custom-panel-editor.component.ts:67`），初始化后不以设备／账号／Manifest owner重置；生成响应`next:123`没有同一owner核对。同能力换设备或账号后的迟到结果是否污染当前草稿，尚无本批反例，不签为已复现Bug。
   父页`device-uic.page.ts:217`的后台事件只处理预览遥测，离页取消的是父页自己的生成，未把活跃状态传给custom子组件；两模式“后台继续还是取消”需明确统一产品合同，不先把后台继续本身定为协议失败。
   Gateway `interface-workflow.ts:29～41`先等待鉴权／限流／请求体，直到77行才订阅res.close；取消若落在前置等待窗，可能仍启动80行上游。现有旧generate取消测试不覆盖此窗，需deferred准入＋close的有限反例。
2. **随后：采用／保存收尾归属及云Console并发合同。** `DeviceUicPage.apply():477`的非Cloud保存后缺统一device／view／destroyed核对，Cloud分支仅核账号epoch，后续仍更新配置并导航；先用延迟保存→离页→完成离线核对，不能据源码直接宣称已发生错误导航。
   `DeviceConsoleService.save():124`和Gateway `device-v2/console.js:121／162`实际不接受expectedRevision，明确last-successful-commit-wins；它有事务和权限复核，但不等于陈旧草稿CAS。并发覆盖风险待离线复现，若要改合同需单列兼容方案，不冒称原非Cloud CAS失效，也不在本批顺手改服务器。

### 49.3 下一批最小实施与停止条件

本节为当时的最小顺序，用户随后已在§50完成App部分，不再作为默认下一轮重复实施／采样入口。

只先选第1项的**App生成owner／取消**：用现有真实模板及Http测试替身复现同能力换owner、取消／后台后的迟到返回；
冻结device＋账号代际＋Manifest＋本次generation，明确默认后台取消且不自动恢复、保留上次已确认草稿的合同，再只修确认缺口并有限回归。
服务前置取消窗可先补一个本地deferred用例确认，若独立成立再修其窄abort接线；不调用模型或真实邮件，不引入生成器、SDK、自动重试或新编辑器。
第2项先保留源码候选与合同事实，之后另选采用／CAS最短差异；不与本轮捆绑多模型、长稳、拖动性能、自动应用、设备能力、额度或整页接管。
若离线不能复现，记录NOT_REPRODUCED和停点，不为补样本改设备／反复真机生成；保留现有账号、授权、TaskStore、UNKNOWN及完整日志，暂不部署／提交推送。

## 50. AI生成归属与取消生命周期收口及云端部署诊断（2026-11-02）

### 50.1 冻结合同及本批范围

- 一次生成属于发起时的设备ID、账号会话代际、已接受Manifest／能力及本次generation。字段恰好相同不代表owner相同；普通token刷新或同owner State变化不取消有效生成。
- 同owner显式取消、离页或进入后台时立即退休当前生成并取消传输，保留上一份完整有效草稿；返回前台或缓存页只恢复显示，不自动生成、重试或应用。部分／迟到进度、成功或错误均不得覆盖退休后的草稿或忙态。
- 设备、账号或Manifest归属改变时旧生成和未应用草稿失效，不把旧owner的草稿还原给新owner；新设备可从自身已保存配置／默认模板初始化。账号事件先于新device输入时先清旧草稿，不从仍在输入槽里的旧账号device重新载入。
- 复用现有epoch、AbortController／RxJS unsubscribe、账号／可见性通知和预览组件，不修改设备权限、执行协议、SDK、采用／CAS管理合同或整页布局。生成预览继续禁写，是否生成和是否应用仍是两次显式用户操作。
- 先用原模板、Http测试后端及生成服务替身完成红测，再最小修正及有限回归；不调用真实AI、验证码、设备或业务写接口。本批暂不安装／部署／提交推送，保留账号、授权、TaskStore、UNKNOWN及原完整日志。

### 50.2 服务可用性独立只读诊断

用户同时反馈云端AI不可用；只核实际运行容器、Gateway生成路由、内部health和Nginx入口，不POST生成、不凭GET成功代签模型调用成功。
只读核验时间**2026-11-02 00:00:33（北京时间）**，结论为当前云端AI入口不可用，已确认两层部署缺项，不是仅据App超时猜测：

1. 当前服务器`129.204.34.159`的`/opt/blinker-iot-cn-20261030/compose.runtime.json`只有Gateway、Broker、PostgreSQL及Redis，四容器healthy；compose及全机容器清单均没有`device-ui-agent`。Gateway已部署dist包含workflow／refine／generate和相应模块，但内部GET `http://device-ui-agent:7200/healthz`为`EAI_AGAIN`。
2. 实际Nginx `iot.yiyu.pro`白名单没有`/api/v1/interface/*`，default为404；公网TLS GET三生成路径均404，服务器本机强制定点到127.0.0.1、保留iot域名SNI／证书校验的GET同样404（sslVerifyResult=0）。未向生成路径POST，配置与GET证据不冒称已跑模型。

完整私有诊断保存在Service忽略目录`build/qa5-ai-cloud-readonly-20261101/private-b30e2aa8/`，包含只读inventory／compose服务名／有界健康证据，未输出密钥或复制凭据。本批未改云端配置、容器、数据库、模型权益或Auth，未部署／重启。
模型配置、内部签发材料及上游权益没有本批取证；不能承诺只补上述两项就代表端到端生成通过，亦不能把四个IoT容器healthy说成AI已可用。

### 50.3 原模板复现、最小实现与有限验证

- 两套真实原模板共14项初次行为红测为7红／7绿；其中5项是同能力换设备／账号、custom后台及组件模式后台／账号恢复前围栏的实际缺口，另外2项明确为新active／Manifest输入的接线合同，不算新增历史Bug。初次测试夹具`any.querySelectorAll<T>`的TS错误已单独修正并保留日志，不把未运行算行为红测。
- 自定义生成器复用原HttpClient／Subscription、host会话及可见性流，新增最窄active／Manifest输入。每次请求冻结本地owner、能力、主题及generation代际；取消仅一次终态回收。账号事件先清旧preview／prompt并隔离旧device引用，直到新device输入，不能同步从旧账号对象重新载入saved；同会话token通知、同owner State刷新不取消正常请求。
- 父页复用DataService既有userDataLoader和sessionEpoch，在账号reset的false阶段而非等库存true时退休；组件生成回调核对同一owner／代际／前台，custom子页接既有可见性。后台、缓存离页及销毁取消，不自动恢复生成；同owner保留完整好草稿，换归属则清旧草稿。两条路径只共享一个纯owner键函数，不新增生成执行器／全局状态服务，不改采用保存／CAS正文或预览禁写。
- 最终4文件**43项全部通过**：新增custom原模板10、组件原模板5，原页面工作流22及原生成传输6。组件模板以受控生成port延迟返回，custom用真实Http测试后端验证取消，不伪造closed RxJS observer；迟到成功／进度及错误、同owner草稿与正常恢复分别核对。此处不是43次模型或硬件执行。
- 原SDK的App `ngc --noEmit`严格模板／DI编译通过，覆盖本次父子模板；沿既有WiFiProv源类型映射解决安装副本的陈旧声明，不改SDK或锁文件，不生成发行包、sync、APK或安装。编译期间App非忽略源码、HEAD及index一致。
- 原红／绿／编译及变更范围材料保存在App忽略目录`build/qa5-ai-owner-20261102/`；未操作真实手机、账号、授权、设备能力、TaskStore、scope或UNKNOWN，原完整请求日志未删改。UI技能检索没有命中精确的异步归属条目，故取消与草稿细则采用本项目冻结合同，而非宣称来自通用设计规则。

### 50.4 下一步最小范围及停止边界

App生成owner／取消的软件有限范围到此结束，不重复本批或Hold／配网／设备采样。手机及云端仍是原已部署版本，本批源码修正尚未上线。
**优先单列云端AI部署恢复**：明确允许部署后，复用现有`device-ui-agent`、Gateway协议和Auth，核对必要私有配置／模型可达性，补内部Agent服务及Nginx窄入口，先health／未认证权限边界，再另行明确一次真实生成短验。未获后续部署指令前不改运行配置，也不自动调用模型凑通过。
采用保存的迟到收尾、云Console CAS及Gateway前置await取消窗口仍按§49.2留作后续独立最短差异，本批未修改服务产品源码，不捆绑编辑器整页、UI重设计或新模型。
本批没有部署、安装、发码、自动重试、SDK／依赖升级、Git暂存／提交／推送；保留无关本地修改、账号、授权、TaskStore、UNKNOWN及完整日志。

## 51. 云端生成Agent恢复与一次真实App生成（2026-11-02，部署完成／生成被上游404阻断）

### 51.1 用户授权范围与停止条件

仅在`129.204.34.159`补现有`device-ui-agent`及Nginx三个生成入口；先核私有配置、冻结匹配镜像和现有Auth，再依次健康／鉴权、匹配App同包更新及一次真实UI生成。
不更换Auth或供应商／SDK，不新建账号／设备，不改套餐、额度、DeviceKey、TaskStore、scope或UNKNOWN，不升级依赖，不自动重试或提交推送。
Agent先实际健康，必要时仅Gateway同镜像重建一次，Broker／PostgreSQL／Redis／原数据卷不动；保留完整旧配置及新旧请求证据。模型调用前有一次性意图，任何失败保留原回执，不重复点击凑通过。
生成只到预览，**不点击应用、不保存控制台或发设备指令**。原工作流最多两个正常模型回合（默认配置工具→设计）不等于网络重试；`maxRetries:0`保持。

### 51.2 冻结输入及部署准备

- 实际Gateway沿原镜像`e209b6b2…e35b`；China／Global／Development三个origin本批均核为现有`https://api.yiyu.pro`，Agent逐项沿用，不借补服务切Auth。原Agent token缺失，服务器新建独立256bit内部token，仅Gateway／Agent运行配置共用，不入镜像、App、公开JSON或Git，不复用Auth／JWT／设备签名密钥。
- 公开必要源码只新增`compose.interface.yaml`、空占位`.env.interface.example`、有限部署测试，并在原renderer加严格布尔`interfaceGenerationEnabled`。默认关闭且输出不变；true只开三条精确Gateway路径、POST／OPTIONS，generate64KiB／原60秒，workflow／refine1MiB／局部200秒，禁buffer/cache/upstream retry；不公开7200、`/v1/*`或Agent health。Agent无host端口／业务卷，read-only、32MiB tmpfs、128pid、512MiB／heap384MiB、backend＋egress，egress的gw_priority=1沿现Gateway。
- 本地新旧Nginx有限集合13项通过；网络默认路由对齐后仅复核对应单项通过，不重新跑设备／生成。实际Nginx只插入渲染的BEGIN／END块，不能整份覆盖已有maps/location/tasks/sharing等配置。
- 远端首次离线编译因构建包装heap256MiB OOM停止，业务未动；获准的较高heap候选未启动，因可用RAM＋swap约837MiB未达1GiB门禁。两者证据保留，不盲重跑、加swap或停止业务。
- 改为本机原锁定工具严格`tsc → rslib --no-env`，唯一构建20.892秒退出0，Node22.22.3、8个工具／依赖版本与原锁一致。ESM制品无native／本机绝对路径，归档只有`dist/index.js`（57,508B／SHA`5f012f74…d0a03e`）及`package.json`（659B／`f1f09814…6b9681`），tgz SHA`402afecd…37ceba8`。远端沿已部署Gateway Node22.23.3镜像仅COPY这两公开文件，公开目录0755／文件0644，USER node；不npm install或拉新基础镜像。
- App先保留7项secure授权与两份完整journal、原logcat；旧APK读回仍`df0dc68b…8cdae`。唯一匹配cloud-beta队列及核包完成，新APK 77,893,930B／SHA`948d92116e6befccdd2853e082c0523a7dd957f139c5ddf7605d7acb99cbf9f8`，原debug签名、beta包名、HTTPS、19插件（含五自有插件）及1,157项Web资产一致；包含§48 Hold与§50 owner修复。SDK2.4.4／esbuild0.28.2／锁不变，App源码2202项、6项WiFiProv输入及HEAD/index每阶段未变。尚待云端门禁后才允许安装，不将已构建写为已安装。

### 51.3 实际部署及权限边界结果

- 原Gateway基础镜像的公开`/app`及依赖树权限为0700／0600，首次USER node只读门禁失败，未启动Agent或改业务。命名扫描的56项均属公开SDK credentials实现，两个PRIVATE KEY字符串只在dotenv文档，无实际密钥／私有配置；不删除公开模块或放宽为root运行。
- 全树chmod包装在90秒CLI截止被实际取消，原服务／配置未变，未重复启动旧队列。最终从原image导出已核公开依赖、只在tar元数据规范化权限后ADD，不再逐文件overlay copy-up；归档约62.8MB，目录／文件／原执行位保留，symlink不跟随。最终原SDK运行依赖及两文件制品摘要匹配，无install／pull／compiler。
- 实际Agent image为`sha256:42eb31f7dc3bc7d2a6150c35cb4c4f20d093692addac1946401dd2fcd84387db`，Node22.23.3、UID1000、read-only、cap_drop ALL、no-new-privileges、512MiB／heap384MiB、无host端口或业务卷；backend／egress及egress GwPriority1实读一致，health通过。
- Gateway仅以原`e209b6b2…e35b`镜像重建一次，health通过且可达Agent。新内部token／三个正式Auth origin逐项一致，原Auth保持；Broker／PostgreSQL／Redis三个container ID与原样一致，原数据卷未替换。重建实际短断时间没有完整测值，不能将预计10～30秒当实测。
- Nginx仅在原HTTPS server插入冻结marker块，剥离该块后原字节一致，`nginx -t`／reload通过。紧接reload的首探针仍到旧worker而404，原证据保留；实际新worker GET405明确后才完成最终边界，不重建服务。三个精确路径分别POST401、GET405、OPTIONS204、suffix404，均校验证书；`https://localhost`预检204、Authorization／Content-Type及原ExposeHeaders合同通过。以上鉴权探针没有用户token或模型调用。

### 51.4 唯一真实App生成结果及保留终核

- 一次`adb install -r`后读回匹配APK `948d9211…bf9f8`。原所有者设备正常进入`uic/ai`，原模板两张card、生成按钮可用、预览inert；冷预检没有生成或设备写入。只通过原输入框和原“生成界面”按钮发起一次component workflow，不使用私有controller或替身，不点击应用／微调。
- 唯一POST `/api/v1/interface/workflow`为HTTP200，但正文是`stage(generate,round1) → error(INTERFACE_AI_GENERATION_FAILED)`，**没有complete，不是生成成功**。原UI显示“生成未完成”，Apply禁用，保留原两卡预览，不把旧默认模板当新生成结果。
- workflow `98a7995b…20dca`与Agent日志精确匹配：仅一个model-round，在北京时间2026-11-02 01:00:39收到`https://api.yiyu.pro/v1/chat/completions`的HTTP404，随后`MODEL_REQUEST_FAILED`。收到该origin响应不等于进入Aily模型handler或供应商；不能据404归因为账号额度、token失效或App取消，不修改权益／Auth／供应商。本轮无第二回合或第二次生成，Gateway stdout无本请求关联行如实保留，不补造日志。
- 两段被动Network观察均Device HTTP写0、完整MQTT解码Command0；未发Apply／Console写、控制／任务／Key请求，不操作开发板、TaskStore、scope或UNKNOWN。生成成功后的后台往返因成功前已停而NOT_RUN，不用离线43项或失败页面代签；不为补该样本再调用模型。
- 终核同账号、7项secure（含session）及两份完整journal均与原记录一致，完整新旧logcat／HTTP／NDJSON保留；App回首页，本批CDP／Network及9479转发已回收、常亮值精确恢复。Agent仍healthy，新服务／精确入口和匹配App保留，无SDK／锁变更，无Git暂存／提交／推送。

### 51.5 下一步最小范围

IoT部署缺项已补齐，不重新构建Agent／App、重跑配网／任务／自动化或资源审计。**只先核对正式Aily的模型API路由与实际开放范围**：以本次404、现有AI／Kong／Nginx源码及部署归属定位，不凭改URL猜测，不切测试Auth，不向另一个地址自动再发模型请求。
本地Aily仓库只读核对确认该路径就是现有规范：`services/ai/src/api/openai_compat.py:106`定义POST、`main.py:1963`注册，`gateway/kong.yml:233`及`nginx/aily-service-kong.conf:33`保持同路径，没有路径迁移证据。`docs/api/blinker-agent-aily-adaptation.md:3／35`及2026-10-15发布记录写明当时仅测试环境已发布、生产未发布，且测试与生产token不通用；这是历史本地证据，不能代替当前生产部署读回。按本地实现单纯关闭OPENAI_COMPAT开关应403而非404，故不能只翻开关或改Agent路径猜修，应查生产入口／Kong／实际AI revision的缺失层。
若需要修改正式Aily网关／服务，应单列明确部署范围；本次对IoT Agent／Nginx的授权不自动扩大为接管正式Aily环境。路由修正及必要门禁后，新的真实生成需单列一次明确验收，不沿本批失败自动重试。
采用保存收尾、云Console CAS及Gateway前置await取消窗仍按§49.2独立后置，本批没有顺手改变它们。

本批私有证据保存在App `build/qa5-ai-live-20261102/`、Service `build/qa5-ai-enable-20261102/`与`build/qa5-ai-cloud-enable-20261102/`受限目录；部署及请求关联分别取后者`tar-status-6`／`tar-status-7`。凭据、完整日志、运行配置和构建产物不进入Git；未复用健康结果冒充模型成功，原停止／失败证据保持。

## 52. 正式Aily路由只读核对与访问前置（2026-11-02）

### 52.1 冻结范围及证据层级

按用户指令继续§51.5，先核正式Nginx／Kong／实际AI发布版本，确定HTTP404出自哪层后再选择最小修复。沿现有正式Auth，不切测试、猜URL或直连供应商，不重放旧workflow、不自动重试、不升级SDK或提交推送。
本次没有连接手机、设备、生产数据库或服务器SSH，没有发登录／验证码／生成／控制／任务请求，原账号、授权、TaskStore、scope、UNKNOWN及§51完整日志均未操作。不把本地历史发布说明代替实际生产证据。

### 52.2 本地规范只读核清，不另跑旧回归

本地`aily-services`为`stao`／`5370a4d4009b736d6bfb111adb6df67e7527a070`，工作树既有未跟踪内容保留；未改本地Aily产品源码或配置。

- Nginx `nginx/aily-service-kong.conf:33`：精确`/v1/chat/completions`转Kong、原路径保持，禁buffer／cache／upstream retry，200秒；不能用旧直连`aily-service.conf`替代。
- Kong `gateway/kong.yml:224`：`ai-service-openai → ai-service:8000`，路由`~/v1/chat/completions$`只匹配POST／OPTIONS，`strip_path:false`、`retries:0`；`:601`要求路由级`aily-session-auth mode:required`。**GET404不能独立证明此路由未发布**，不重复GET凑诊断结论。
- AI `services/ai/src/api/openai_compat.py:106`及`main.py:1963`注册POST，前置核可信身份。无身份401，服务对象缺失503，当前实现的功能开关／名单关闭应403 `model_access_denied`，不是404。
- 本地协议及2026-10-15发布记录均无路径迁移证据；当时仅测试发布、不代表2026-11-02实际生产版本。现有Agent路径符合本地合同，不能因为404就将正式token发到测试服务。

### 52.3 初次访问前置（后续已由用户指定Tabby补齐）

只做一次公开HTTPS DNS查询，`api.yiyu.pro`的A记录为`114.132.150.141`（TTL600），与已授权IoT主机`129.204.34.159`不同；本机系统解析另返回`198.18.0.15`，未据此登录。**DNS入口不等于实际源站／SSH主机证明**，仍需维护侧确认入口归属。
仓库`AGENTS.md:40`要求正式`aily-services-cn`运维凭据通过运行时1Password CLI `op`取得。本机仅发现ssh，无PATH／常见目录中的op、正式SSH config别名或公开运维脚本中的既有凭据引用；两个WSL实例均未运行，未为找工具而启动Docker／编译VM、安装工具或搜索凭据值。
此初次阶段未建立生产SSH，也未读取生产Nginx／Kong或AI实际版本，只有§51的HTTP404证据，未提前定根因。随后用户明确指定Tabby及129测试边界，实际结果取§52.6～52.8，不再以本节初次访问停点作为当前阻点。

### 52.4 按缺失层选择的最小修复合同

| 实际只读证据 | 最小范围 | 必须保留的边界 |
| --- | --- | --- |
| Nginx缺精确入口，后端合同已具备 | 只在实际域名server补精确转Kong，先校验再reload | 不覆盖其它location／域名，不绕过Kong，不重建App／IoT服务 |
| Kong缺服务／路由／Session插件，而AI已匹配 | 只补该服务／路由及required Session插件，先解析配置 | 同一Auth／region、原路径、200秒与零重试；不降为optional或直连AI |
| 实际AI版本／挂载缺API注册或兼容消费者 | 发布匹配完整AI消费者，不只复制一个API文件或翻开关 | 先只读核025及前置迁移、当前Auth契约、容量／真实用量／Credits outbox与未决恢复依赖；若涉及迁移／跨服务发布，另明确备份及实施范围，不盲整栈升级 |

当前表格是分支合同，不是已选定的生产修复。尚无需要改Agent URL、SDK、App或设备执行链的证据，故本轮不加产品补丁／新测试或重新构建。运行入口／版本证据齐备后，只实施实际缺失的分支，不全部执行。

### 52.5 下一步入口

原先缺少正式入口，用户随后确认`api.yiyu.pro`已给用户使用，允许通过现有Tabby连接，只将129上的Aily用于后续测试。按此补充授权只读操作正式机，不导出任何凭据、不改终端确认设置，也不在正式执行模型／发布。下述实际核对完成后，后续入口已收敛至§52.8，不再询问旧访问前置或重复已有检查。

### 52.6 正式只读结果：兼容入口未发布

通过现有Tabby MCP13001及原配置连接`114.132.150.141`，不读取／导出密码或私钥。原会话保持；为独立检查建立一个本批会话，确认无启动脚本／端口转发，最后只关闭该新会话并核原会话ID全部保留。
首个idle guard因SerializeAddon的VT尾控制字符误停，尚未发远程命令；本地用Node原生`stripVTControlCharacters`规范化，3个纯本地guard用例后才读取，不改变服务、SDK或确认策略。原SFTP不可用及两个0命令停点分别保留，不算生产故障。

实际主机`VM-24-16-ubuntu`，项目`/data/www/aily-services/docker-compose.prod.yaml`。AI `aily/ai-service:1.0.98`／image `f946f781…363d06`，Auth `1.0.81`／`380f2d3d…6598`，Kong3.4／`0383cf45…a9bde2`，均healthy；前后容器ID保持。AI无应用源码bind，只有只读configs及原日志／state挂载，故检查的是实际image文件而非本机候选。

| 层 | 实际只读结果 | 结论 |
| --- | --- | --- |
| Nginx | `nginx -T`校验通过，实际`/etc/nginx/conf.d/api.yiyu.pro.conf`无专用`/v1/chat/completions`；`location /`原路径转`yiyu_kong_gateway`，通用读预算60秒 | 不是“完全没转发”；兼容接口上线时仍需独立200秒、禁重试／buffer的窄配置，不能整份替换域名／证书配置 |
| Kong | DB-less，实际挂载配置SHA`f0335d02…ffd`无兼容service／path；按实际9001端口，以宿主Python只读GET当前容器私网Admin `/routes`一次，37条完整无分页，目标路径的可能POST匹配为0 | 当前运行路由未注册该入口，与§51的404直接一致；不能把健康或GET404当发布成功 |
| AI | 实际image中4个兼容模块不存在，main亦无兼容注册；未声明兼容开关 | 即使只加Kong路由也不能完成接入；不把“env未声明”单独冒称开关故障，关键是实际消费者缺失 |

首次尝试容器内curl读取Admin因该二进制不存在退出127，未发出Admin请求；改用宿主既有Python及已读实际端口后仅一次GET成功，不安装工具、不重试任何模型。未读生产账号库／迁移表，不reload或重建，不登录、改权益、调用模型或重放§51请求。
已确认的是**正式发布链缺项**，不是Agent路径迁移、App owner、账号额度或SDK版本故障；本轮没有修改正式Aily来试错。

### 52.7 129测试栈实际输入及差异

129上已有独立项目`aily-services`，路径`/home/ubuntu/www/aily-services/docker-compose.yaml`，18个相关容器，入口为`api-test.yiyu.pro → 127.0.0.1:8080 → Kong`。Kong3.4 healthy；AI／Auth／aimodels为dev镜像＋RW源码bind，未定义Docker healthcheck，不能把running签为完整健康。
当前挂载repo为`stao-update`／`756034bde7d0bd9e4c24a5fe346d6c5ea5af4bd1`且tracked dirty；实际`gateway/kong_dev.yml` SHA`f91ecf51…8e8`无兼容路由，4个AI兼容模块也不存在。源码挂载读回不冒充已加载进程版本；不能只切Agent origin或覆盖该dirty目录。
这与本地实现的差别属于**发布输入／版本差异**：本地`kong_dev.yml:112／545`已包含service／route及required Session／响应头，文件git clean，来自`5cdd24c…`；prod／dev配置合同已有源码和回归，不重新发明接线、不为制造补丁重复跑旧测试。已有Session配置测试依赖Docker，默认skip不等于本轮通过。
IoT与测试Aily无共享Docker网络／命名卷；当前IoT Gateway／Agent的Auth及三个模型origin仍为正式`api.yiyu.pro`，没有切到测试。旧`api-test.yysc.tech`或旧42测试profile也没有被当作129使用。129本批没有HTTP／账号库／模型／部署操作，SSH已关闭。

### 52.8 下一批最小实施范围：先准备129匹配候选

1. **正式停止在只读结论**，本批不发布／开启功能或做模型测试；不重建已验IoT Agent／Gateway／App，不改它们的正式Auth。正式AI／Kong／Nginx的未来发布仍需独立实施窗口，不借129测试变更顺带上线。
2. 129先保留实际dirty修改并核匹配base，在独立发布目录冻结现有完整兼容消费者、两块Kong接线及测试域名Nginx窄入口；不整份覆盖远端repo、不盲部署本地整分支。**先复用129现有测试Auth**，核Session／region合同，只有实际消费者缺失时才决定最小兼容更新，不默认升级Auth或复制正式数据／密钥。
3. 发布前核025与前置迁移、兼容能力配置、原Credits／outbox及未决恢复依赖；需要迁移时先必要备份及隔离克隆演练，保留账号和历史事实，不手改版本表或重置账本。本轮未执行这些数据库步骤，也没有构建／安装／部署候选。
4. 匹配后先静态合同／`kong config parse`及健康、未认证／拒绝边界；不把PyYAML解析代替真实Kong解析。真实生成另用同一测试栈的测试账号／匹配入口及一次显式新请求，不发送正式token到测试Auth，不自动续发旧失败。当前App及现有正式账号不自动迁移或切区。

本批产品源码无新缺陷证据，故没有另写执行器、改SDK或复制API文件。仅两份执行文档更新；正式／129服务及账号、授权、TaskStore、scope、UNKNOWN和完整日志保持，无暂存／提交／推送。正式脱敏只读证据取Service忽略目录`build/qa52-aily-routes-20261102/`；129证据为上一批受限目录中的`inventory-129`／`inventory-129-versions`，不入Git。

## 53. 129独立兼容候选与恢复分支基线核对（2026-11-02）

### 53.1 范围及旧适配文档的含义

按§52.8只准备129测试候选，不发布到正式，不切换当前App、IoT Agent或正式Auth，不调用模型、登录／验证码，不自动重试或升级SDK。原账号、授权、TaskStore、scope、UNKNOWN及完整请求日志均不操作。
用户提供的`blinker-agent-test-quickstart.md`与本地公开同名文档逐字相同（SHA`3b22fc79…26ff4c`）：写于2026-10-15，对应旧`42.193.22.41:8180`独立测试环境，且明确生产当时未开放。`POST /v1/chat/completions`、原Auth及Credits接口合同没有从源码删除；历史发布状态不能证明129迁移后的入口或正式发布已完成。本批不运行其两次真实计费示例，不向旧地址探测或发送任何token。

### 53.2 分支方向：恢复修复已是候选基线，并非已合入stao

- 129实际挂载分支名为`stao-update`，HEAD为`756034bde7d0bd9e4c24a5fe346d6c5ea5af4bd1`；本地`i3w-operation-recovery`及其现有`origin`引用恰好同一提交，恢复分支工作树只读核对为clean。本批没有fetch或改变分支。
- 本地`stao`仍为`5370a4d4`。两支共同祖先`d036270d`，相对该分叉`stao`独有4提交、恢复分支独有55提交，双方不是祖先关系。`stao`独有的是`5cdd24c5`／`ed665675`两个兼容功能及两份历史文档；近期的一次受保护resume、step／语义进度、事件过期完成文本及未claim step恢复均已在`756034bd`内。
- 实际执行是**独立detached候选以恢复分支为基线，再增量移入两个兼容功能**，不是用10月整树覆盖11月代码，也不是已经把分支合入`stao`。手工冲突只在候选处理，保留新版Pro校验、上游凭据替换、metrics、V2未决用量恢复及恢复围栏。
- 按用户随后补充的团队约定，**测试服务器统一跟踪`stao`**，整合方向明确为`i3w-operation-recovery → stao`，保留近期恢复修复和`stao`原有兼容功能，验证匹配提交后再部署。本轮仍只准备独立候选，不执行分支合并、提交或推送；不能把候选清单或已暂存的冲突消解称作原分支已合并。
- 初次基于129现有挂载分支提出的`stao-update`测试入口建议已被上述团队约定覆盖，不再作为后续部署指令。当前129分支名、实际HEAD和本地origin引用分别核对；**先整合代码，不直接pull旧stao覆盖129**，也不因统一分支而覆盖远端dirty内容或切换现有App／正式Auth。

### 53.3 原远端修改保护与实际迁移／依赖

原repo不覆盖。已在`/opt/aily-openai-candidate-20261102-qa53`建立独立0700目录，私有快照与公开候选分开；只新鲜保留获准的dirty源码`services/ai/requirements.pip`原字节和三份限定patch，唯一依赖语义差异为既有`sqlalchemy==2.0.36`。另一项私有／未批准内容留在原处，未读取、复制或声称已备份。保护前后原HEAD／index字节／status及requirements均精确保持。

数据库检查使用显式READ ONLY事务、5秒statement／1秒lock截止并rollback／close，只读取迁移、schema及必要状态聚合，不导出账号或业务内容，不建库／DDL／回填、手改迁移表或恢复意图。

| 项目 | 129实际证据 | 候选启用前仍需完成 |
| --- | --- | --- |
| AI迁移 | 当前024=`e7f9a1b3c424`，无`ai_openai_requests` | 候选025=`f8a0c2e4b525`直接接024；独立演练及业务迁移尚未执行 |
| 测试Auth | 现有Session／Pro链存在，实际verify／Kong缺新region合同 | 仅候选叠加签名region＋实时Session兼容，不默认换Auth或认为旧token跨环境有效；Auth迁移保持自身较新图，无新增Auth迁移 |
| Credits恢复 | Auth当前迁移`adminmenus001`；Credits账户／桶／账本、`quota_reservations`及`session`表存在 | 不因初查错误猜测的表名缺失就宣称恢复模块不存在；不执行恢复脚本或重排旧请求 |
| AI用量链 | admissions及outbox状态聚合为空；49条receipt处于`usage_recorded`；实际outbox开关false | `usage_recorded`不等于已结算或UNKNOWN为0；发布仍核原receipt／evidence／outbox／capacity合同，不能只开启HTTP入口 |
| 配置所有权 | 新base要求服务配置归属自身env_file | 新开关只在AI模板／原env_file，移除新旧相关Compose遮蔽默认值；不复制实际provider配置或凭据 |

### 53.4 最小实现及有限离线回归

候选新增公开窄Nginx片段，只转精确`/v1/chat/completions`到129原Kong `127.0.0.1:8080`，保持200秒预算、禁缓存／buffer／上游重试，不覆盖实际域名、证书或其它location。Kong两配置和完整插件、AI／Auth消费者及025一起冻结，兼容／维护开关默认关闭；不另造执行或计费链。

有限回归累计**141个不同用例通过**：首轮AI 103通过／1失败，原因是新测试用整段substring把合法`$server_name`变量误判为禁止的站点声明，不是产品Nginx故障；修为指令行匹配后仅复核该1项，再运行此前未执行的Auth24及SQLite恢复／CAS／去重13项，38项全部通过，不重跑已过103项。原失败与续批日志分别保留。初次harness在pytest前因无关公开env fixture的严格路径守卫停止（0用例），仅排除两个精确无关路径，不读取其内容或放宽运行env守卫。

测试复用原venv，无真实网络、Auth、模型、PostgreSQL或app lifespan；`src`均来自候选，源码hash未变，守卫违规0。SQLite28个连接仅内存／受限临时目录；13个PostgreSQL参数明确排除，不签为通过。**本机SQLAlchemy2.0.52不代替129的2.0.36运行证据**，迁移图检查及SQLite拒绝丢去重身份不等于真实PostgreSQL025演练。未调用原cleanup、requeue或任何恢复写脚本。

### 53.5 公开候选及原运行时检查

公开候选已传入上述独立目录：578份必要源码／测试／迁移／模板及额外MANIFEST，payload 5,270,878 B、单一tgz 1,080,516 B，SHA`427be10bec02c0786381f4f248e66e1e9e89e847c1691cb4c1bcc2d668acc99e`，MANIFEST SHA`572c9050…67a57c`。逐文件字节／SHA及无额外成员核对通过；没有运行env、实际provider配置、账号凭据、备份、日志、数据库、`.git`或构建缓存。两个公开模板仅在包中派生脱敏，清除个人邮箱／示例敏感值，标记原文件SHA及精确key；原source／远端运行配置不改。扫描的人审例外仅限10个精确文件中的公开mock／说明值hash，不放宽私钥、JWT或供应商Key规则。

原immutable镜像、无网络／只读公开源／无业务env或卷的静态检查已通过：AI **192**、Auth **282** 个Python文件compile，未import app／lifespan或连接数据库。AI原运行依赖SQLAlchemy2.0.36、FastAPI0.115.12、Pydantic2.11.4、Alembic1.13.3；Auth为2.0.36／0.115.3／2.14.2／1.13.3，无安装／升级。AI完整25迁移单头025、Auth完整59迁移单头`adminmenus001`，无缺父／环；Auth图没有需要顺手应用的新迁移。

首轮验证器仅处理AST Assign而漏掉AnnAssign，误报既存011父节点缺失；原文件在包内，未删迁移或修改产品。修正验证器并补5项纯AST回归后，才在独立新日志批次补核。初次还曾误判Auth为两个head，同样由注解遗漏造成；最终完整图是上述**单头**，不据初次误判执行DDL。其后Kong检查在tmpfs前缀初始化被cap-drop ALL阻止chown而停止，尚未到配置解析；这不是已确认的Kong schema错误，也没有放宽业务权限。

最后仅一次补未完成的Kong／Lua／Nginx检查，改隔离容器为UID65534并继续cap-drop ALL，**两份真实Kong配置解析、17＋8项纯mock Lua及独立prefix的Nginx `-t`全部通过**。Pro、签名region、上游凭据替换及metrics仅核handler接线，不冒称真实Auth／Credits／指标后端验收；Nginx只测试公开harness，未reload实际配置。已过AI／Auth编译只校原结果hash、不再执行，没有重新传包或解包，也无build／pull／install／SDK升级。

最终原26容器（包括原停止的beta）的ID／image／StartedAt／status精确保持；129原HEAD／index字节／status／dirty requirements四项保持，原私有配置不动。本地主`stao`的HEAD、索引条目、原status及tracked工作树保持，恢复分支不改；独立候选的暂存区不冒称原主树无暂存操作。全部自有临时验证容器已回收，SSH及队列关闭，初次停止／失败证据完整保留。

本批可关闭的是**独立公开候选准备及上述有限回归／原运行时静态／配置解析**，不是模型服务已上线。业务025迁移、入口启用、实际Auth／模型生成均NOT_RUN；当前App／IoT仍使用正式Auth，正式服务器本批未连接或修改，没有分支合并、提交或推送。

### 53.6 下一批最小范围

1. 按团队约定先完成`i3w-operation-recovery → stao`的必要源码整合，保留`stao`现有Chat Completions及两侧历史，不覆盖远端dirty内容。以本批候选和有限回归为对照，不能直接部署当前旧`stao`，也不能把本批141项签为整个55提交的全项目回归；是否提交／推送仍需明确后续指令。
2. 下一发布前置仅做**独立PostgreSQL的025迁移及恢复边界演练**：从实际024前置进入025，验证约束、COMMIT及非空去重身份拒绝降级；先用隔离fixture，不直接改现有业务库／版本表、重置账本或重放UNKNOWN。Auth本图单头与当前只读版本均为`adminmenus001`，不顺带跑历史旧迁移／教育补丁或盲升Auth。
3. 配对启用前明确测试Auth旧会话处理：新合同拒绝无签名region的旧token，需正常认证取得新会话，不从Header／注册地域补造签名claim、不导入正式token。保留现有Session／Pro／上游凭据及原恢复链；Credits outbox／capacity／模型权限未就绪仍fail-closed，不只翻HTTP开关。
4. 此后才单列测试栈的匹配AI／必要Auth／Kong／Nginx有限发布、健康和未认证边界。真实模型另以同一测试Auth账号、隔离入口及一次显式新请求验收，**不沿当前正式Auth App重发旧workflow**，不自动切换现有App或正式服务。暂不追加设备、配网、TaskStore、自动化或已过组件验证。

本批机器证据位于Service忽略目录`build/qa53-ai-candidate-20261102/`及`build/qa53-129-candidate-20261102/`的受限子目录；最终远端验证取`validation-remaining-nonroot`，本地续批取`finite-continuation`。公开制品及清单与私有日志／原修改快照分离，均未加入Git。

## 54. stao整合、推送及129测试切支部署（2026-11-02）

### 54.1 明确授权及最小范围

用户允许在`stao`合并`stao-update`与`i3w-operation-recovery`，并明确允许推送`origin/stao`供129正常拉取部署，覆盖§53当时的暂不合并／推送限制。正式服务器、当前App／IoT Agent及其正式Auth仍不切换，不调用模型或验证码，不升级SDK、不重放UNKNOWN，不重跑已过设备／配网／自动化矩阵。
本批仅更新必要AI／测试Auth／Kong及`api-test.yiyu.pro`窄入口，不将整分支合并解释为重建所有服务或发布正式。兼容、维护及Credits outbox保持关闭，capacity与模型权限另行准入；健康／未认证边界通过不表示真实生成可用。

### 54.2 真正的Git合并与原本地修改保留

fresh fetch三支仍为`stao/5370a4d4`、`stao-update/572c1cf0`、`i3w-operation-recovery/756034bd`。后者包含前者全部更新、仅多3项恢复提交，因此只做一次真正的merge，不重复合并或cherry-pick。
隔离工作树以`stao`为第一父，合入恢复分支，六处冲突按§53实际已验working tree对齐；保留Pro／region、上游凭据替换、metrics、V2恢复围栏、原接口及`stao`两份历史文档。限定配置所有权修正、SQLAlchemy2.0.36 pin及两份公开模板脱敏一并纳入；没有带入运行env、私有备份、日志或构建产物。
合并提交为**`3e2991c9319af237c1588d88fceabcd1fa6c3b1c`**，两个父提交恰为`5370a4d4`和`756034bd`，祖先检查同时包含两支。主工作树只做fast-forward，原97个未跟踪路径全部仍在、无覆盖碰撞，tracked工作树保持干净。已正常推送`stao → origin/stao`并以`ls-remote`核同一SHA；没有force或推其它分支，现有镜像发布workflow只监听`deploy`，本次不触发正式发布。
独立终审及差异检查通过；578份公开消费者与§53制品全部一致（仅规范化CRLF／LF），故沿用141项业务／合同和原运行时／25项Lua结果，不重跑整套。合并相对旧stao的475路径包含原已发布恢复分支历史，不签为全部子项目新回归；相对129实际756的服务部署只取必要差异。

### 54.3 fresh服务器前置及部署顺序

只读预检确认129仍为`stao-update/756034bd`，尚无本地stao分支；原SQLAlchemy2.0.36 requirements 1073B及1项未读取私有修改保持，无未跟踪文件。AI／Auth为RW源码bind及`start.sh`入口，故切支前要优雅停止相关worker，不能热覆盖后冒称受控发布。原26容器及源码／index／status终核保持。
实际PostgreSQL为14.24，原immutable镜像`0472f529…fdb8`；AI024／无025、Auth`adminmenus001`，49条`usage_recorded`及空admission／outbox状态保持。测试Nginx为`/etc/nginx/conf.d/api-test.yiyu.pro.conf`，尚无兼容窄入口；测试Auth不是当前App使用的正式Auth。
按顺序执行：独立原版本PG演练 → 原修改／必要配置及AI专库保护 → 拉取明确stao提交并核私有dirty路径的Git blob不变 → 受控切支 → 原Alembic直连单次025迁移 → 三服务匹配部署及测试域名窄入口 → 健康／未认证检查。Auth无需新迁移，避免重跑默认账号初始化；新合同下无region的旧测试access／refresh需正常重新登录，不补默认region或导入正式token。
移除Compose旧遮蔽后，必须在AI自己的运行env显式保留compat／maintenance／outbox关闭，不能让旧env中的值意外激活worker；其它配置和密钥保留。025非空时回退只关入口／恢复代码配置，保留新表和所有身份，不恢复旧dump丢弃新事实。实际执行结果在下节补记，以上顺序不是已部署证明。

### 54.4 原版本PG演练、传输及保护结果

独立PG14.24／原AI镜像环境完成 **15／15** 项025演练：真实001→024→025、DDL与版本共同回滚、真实COMMIT、数据库唯一／检查约束、非空去重身份拒绝降级、原registry CAS，以及COMMIT回包丢失后持久结果不重复claim。首次fixture在模型aggregate引入无关应用模块时被隔离守卫阻止，保留失败，仅修正fixture的包入口；续批仍加载原四个ORM和registry，未连接业务库或调用应用／模型。两个自有临时容器及独立网络均已回收，原26容器和源码元数据保持。

129 HTTPS Git fetch因尚无非交互凭据停止，未停服务、切支或写DDL。没有搜索或复制个人token，也没有生成／添加新SSH密钥；改用本地已推送提交的最小Git bundle，只有48个新增对象、11,762 B，以原5370和756两个父提交为前置，SHA`b7ee54184b01cd3ecfd40978ad774083c567741def241be8bda7f5be8fd41a62`。远端校验SHA、两前置和唯一stao head后导入`origin/stao`，origin URL不改。**长期正常pull仍需维护侧配置只读仓库凭据**，本次不是已解决Git认证。

切支前已私有保护requirements、原index、AI运行env及测试Nginx原字节／owner／mode；未读的原私有dirty项只核Git目标blob与状态一致，不复制其内容。完整保留三消费者Docker stdout／stderr，另在停止时补齐末段日志。AI专库custom dump为268,461 B，已核SHA及同版本`pg_restore --list`，**不是全量还原演练**；实际沿用现有`aidb`／`postgres`角色，未新建专用最小权限角色、授予权限或操作Auth账号库。上述完成时HEAD仍756、024、原26容器不变；后续部署取实际结果，不提前签收。

### 54.5 受控切支及唯一025迁移实际结果

实际测试域名配置先做纯解析insert→strip逐字还原预检，确认后才优雅停止AI／Auth／Kong；三者均exit0，没有OOM或强杀。停止及完整末段日志后再次核源码／index和配置未被并发修改，再切为本地`stao`并跟踪`origin/stao`，HEAD=`3e2991c9319af237c1588d88fceabcd1fa6c3b1c`。原requirements pin与提交一致，原私有dirty元数据保持；AI自身env仅显式对齐Retry1及三个关闭开关，owner／mode和其它字节保留。

一次性原镜像writer使用真实Alembic迁移对象，经专库5432直连执行024→025；没有导入应用、模型、`env.py`或启动worker。迁移前后的独立只读快照确认`ai_provider_attempt_receipt`、`ai_credit_call_admission`、`ai_usage_ledger_outbox`及`ai_model_usage_event`完整行指纹一致，不只比较数量。COMMIT后新连接确认head=`f8a0c2e4b525`、新表存在且0行、约束及索引匹配，writer首轮exit0且结果明确；没有重试、手改版本表、回放、降级或恢复dump。Auth没有迁移／初始化。阶段守卫确认另外23个原容器及切支后源码精确保持，下一步三服务及实际入口的部署／健康结果另记，不以迁移成功代签。

### 54.6 三消费者部署及Nginx检查的只读协调

三目标仅以原immutable镜像重建，未build／pull／install，实际均running／restart0／无OOM，Kong healthy；AI／Auth挂载目标stao源码，Auth直接gunicorn跳过原`init_data`与历史迁移。实际测试域名只插入公开窄include，其它原配置逐字保留，`nginx -t`及唯一一次reload均exit0。

部署验证器随后在`NGINX_NEW_WORKER_DEADLINE`首停，未执行health或自动重试，也没有把首轮报告改成成功。独立只读核对确认：原函数`ps -C nginx`采了全机worker，含本次不应被宿主reload的Docker Kong进程，所以前后集合交集不会清空。`/run/nginx.pid`对应宿主master27131，本次11:06:16的实际error日志明确SIGHUP→reconfiguring→启动628745／628746及旧worker退出；627984／627985属于Kong的另一master及PID namespace。实际配置hash与预停机candidate一致，故这是**检查器进程归属错误**，不是Nginx或产品路由失败。

只读协调单独保留，不补造首轮未保存的workers-before集合。后续验证器应按已确认的宿主master及直接PPID筛选，不按UID或全机进程名；本批无需二次reload、重建三服务、重跑025或改产品源码。唯一原定health及鉴权边界检查的最终结果续记下节。

### 54.7 唯一健康／未登录边界结果

原health脚本未修改，仅执行一次即通过：AI及Auth各一次GET `/health`均200；测试域名HTTPS使用实际证书及loopback解析访问，唯一无凭据POST `/v1/chat/completions`返回401／`invalid_api_key`，`Cache-Control: no-store`，TLS校验有效。没有提供用户token、发码、登录、调用模型或重试HTTP。此证据覆盖实际域名配置／Kong拒绝边界，不外推公网各网络或真实模型成功。

三新容器ID分别以AI `7b899161d838`、Auth `4b4534298e6b`、Kong `7d3eb34e2a84`开头，仍用原immutable镜像；AI／Auth及Kong实际bind对应stao新提交，Auth没有`init_data`入口。AI Retry1、compat／maintenance／outbox三个显式false核实，Auth未继承开启值。再以只读事务确认025及空新表，rollback／close后源码与另23容器守卫保持。

最终独立只读终核确认服务器`stao`与upstream `origin/stao`同为3e2991c9，原私有dirty元数据保持、requirements等目标blob、AIenv等本批switch hash；三目标均running／restart0／无OOM，另23原容器不变。旧完整日志、停止末段及三个新容器的完整启动双流日志均私有保留，后者合计29,890 B；只读协调结果单列，不改原deploy失败记录。全部本批SSH及队列已关闭。用户随后确认测试域名前缀为`api-test`，本批实际已使用完整`https://api-test.yiyu.pro`，不据简称改正式域名、当前App Auth或DNS。

**本批可签收：两支整合至stao、已推送匹配提交、129测试三消费者及窄入口的关闭式部署、唯一025迁移和健康／未登录边界。** 首次部署验证器误停与其只读协调分开记录，不篡改原失败，不据此重跑已过部署。真实模型生成、真实测试登录、Credits／容量启用及当前正式Auth App的切换均NOT_RUN，不能把HTTP401或健康200写成AI产品可用。

### 54.8 后续最小入口

1. 测试服务器以后跟踪`stao`，不再回到`stao-update`或把恢复分支整树覆盖。维护侧配置只读Git凭据后才可正常远程pull；本批已验证提交由最小bundle导入，不假称自动拉取已打通。
2. 下一批只补**测试AI启用条件**：同一129测试Auth及签名region的新正常会话、该测试栈实际capacity／模型权限／Credits和恢复依赖；复用已合并合同及现有计费链，不重审全部服务、不修正式或调整当前App Auth。缺条件保持关闭，不靠翻一个兼容开关试错，不向测试Auth发送正式token。
3. 条件明确后，单列一次全新真实生成短验；当前旧404／UNKNOWN不重发，禁止自动重试。不重排设备接入、TaskStore、定时、配网或已过QA-5测试。

本批公开Git提交取Service `stao/3e2991c9`；执行工具、原修改／配置／DB保护和完整日志取Service忽略目录`build/qa54-129-stao-20261102/`受限子目录，远端保护取`/opt/aily-stao-deploy-20261102-qa54/private/`，均不入Git。必要执行文档仅本地更新，不借此次Service授权推送其它仓库。

## 55. 测试AI启用与Blinker真实生成接入（2026-11-02）

### 55.1 本批范围与前置事实

用户明确要求开启AI以测试Blinker生成，允许推进129测试栈的必要启用，不代表发布正式Aily、搬正式账号／token或重发§51旧失败。复用§54的stao／025／三服务及现有内测App，不再跑迁移或已过设备矩阵，不自动重试、发码、升级SDK或提交推送。

手机新鲜核对为匹配APK`948d9211…bf9f8`、原账号、China／签名CN且access未过期，7项secure及2个journal store已必要私有保留，未注销／登录、生成或写设备。129新鲜只读检查确认compat／outbox关闭、无已声明兼容模型、capacity尚未配置；原AI／Auth、内部Auth／RedisTCP及Gateway↔Agent链保持。当前App与Agent均仍走正式Auth，不能把正式token发送给测试Auth。

`SERVICE_TOKEN`现值相同，但探针按占位前缀将其标为“未填”；实际`require_strict_service_token`只检查非空及恒时匹配，**启发式不是已证实的认证失败**。不能为本批盲轮换Auth共享token而破坏其它消费者，不能把同值或TCP可达签为真实Credits成功。

### 55.2 最小实施顺序

1. 明确同一测试Auth身份链。已有App Development是路由别名，Gateway及Agent需同时指向`api-test.yiyu.pro`，签名区域仍CN；China／Global及正式Aily不修改。原beta仅一个session槽，正常切区会退出正式会话，已向用户单列选择，不私改header／localStorage注入测试token或恢复已撤销token。
2. 只选择现有真实managed endpoint／已有价格目录的一个兼容候选，核公开能力声明、context和auto／tier选择；私有URL／Key不导出。容量沿已有显式目录与初始化工具，不伪造供应商配额、独立共享关系或以启动自动初始化。只做必要配置，原SDK不变。
3. 同测试Auth账号及Credits齐备后，兼容入口仅UUID名单，启用所需outbox；不顺带改admission=all。先核原outbox／admission／025请求为空及旧49receipt／event指纹，禁止repair／requeue或回填。`EXPECT_SINCE`不是worker过滤器；全局outbox影响未来managed流量，不能假称已有永久逐用户隔离。
4. 最后仅一次新真实UI生成，到完整NDJSON终态及不可交互预览、不Apply、不写设备；沿原有有界设计／工具回合，不增加网络重试，也不将一次UI请求等同于一次供应商调用。测试Auth正常登录及其owner设备／真实Manifest是前提，不能用体验设备或搬正式scope冒充。任何缺项明确停点，不以开关true或health代签生成成功。

### 55.3 Development入口已接通，AI尚待启用

用户明确允许使用Development测试账号。只在129现有权威私有`compose.runtime.json`精确替换Gateway／Agent两处Development origin为`https://api-test.yiyu.pro`，原China／Global与其余环境、mount及端口保持。仅重建这两者，原immutable镜像不变，新ID为Gateway `a4c42fba86de`、Agent `d06759ea9ab6`；另外24容器不变。两服务本地health各200、测试Auth无凭据me401，均各一次。原配置及完整旧日志私有保留，没有Nginx／SDK／数据库修改或模型、登录、发码请求。

最小候选核到现有`glm-5.3-flash`的canonical、800,000 context、Free可选端点及价格声明，但运行配置尚无兼容能力；10月15日旧测试同名模型的指定函数／两轮工具证据只作合同参照，不算本次可用性验证。当前[智谱官方模型文档](https://docs.bigmodel.cn/cn/guide/models/vlm/glm-5.3-flash)明确该系列不能关闭思考，本批不机械复制旧`disable_thinking:true`配置，也不升级SDK或擅改客户端推理参数。

测试Auth中本轮邮箱有唯一active账号，Free；旧Credit投影为正但当时无当前有效正桶，需正常登录后通过既有`credits/me`的自然allowance流程确认，不能人工造余额／升级套餐。仍需原容量技术门槛、显式初始化及兼容白名单后才可发生成，当前开关未改，不提前发送验证码。

### 55.4 测试AI有限启用及运行配置

本节覆盖§55.3的准备时状态。仅129测试AI启用compat和usage outbox，维护开关继续false、Retry仍1，compat只允许本次正常测试账号UUID，不扩大为全用户；原SERVICE_TOKEN不轮换。实际AI容器`37be6eef7fe9`仍用原immutable镜像`5349703c…b3dcb96`，没有build／pull／依赖或SDK升级，源码仍`stao/3e2991c9`。

仅现有`glm-5.3-flash`声明兼容能力，沿原价格／tier／auto链，context保守保留800,000，服务输出上限16,384；按当前官方合同不关闭thinking。三个实际解析凭据域加一个平台共享guard域，均设**平台并发保护1**，不是声称供应商配额为1或多个凭据互相独立。原Redis standalone／master、noeviction和持久化检查后，仅一次显式初始化四域并通过postflight；原49条receipt／usage及空admission／outbox／025请求在启用前完整指纹核对，没有回填、重排或恢复旧请求。

运行目录为`/opt/blinker-ai-pilot-20261102-qa55`，实际AI重建须沿原base叠加`runtime.ai-pilot.override.json`以保留只读容量目录mount，不能只运行base或旧QA54 overlay。原配置及完整日志私有保留；实际加载路由审计排除原loader已跳过的18项及两项已禁用功能，原raw目录不删。供应商Coding产品成本仍为unpriced并保留API参考估算，用户Credits沿真实现有权重，不伪造供应商实际结算价。

候选检查曾分别因未排除既有非运行路由和harness遗漏dict类型guard停止；修正限定验证器后通过。首次apply在优雅停止AI后，psql多语句stdout解析为空而停，尚未写配置或初始化容量。只读确认该状态后，显式续批复用已完成停机，以同连接分条命令补读取、唯一初始化及AI重建；不是重新执行未知写入。原停止／失败与续批证据分开保留。

### 55.5 正常测试登录及独立设备前置

原beta APK仍`948d9211…bf9f8`，本批未重新构建安装。通过原设置页正常切Development并退出正式会话，随后用户仅授权一次发码，真实App一次发码、一次验证码提交完成同邮箱测试登录；没有重复验证码请求、导入正式token或人工写Session。原非session授权及两份完整journal保持；新测试session正常保留，不能把已撤销的旧正式token恢复回去。

登录后的辅助Credits探针误用未发布的IoT路径，收到HTML而停止；实际登录已成功，不因此重做登录。后按原适配文档，仅向同一`api-test.yiyu.pro/api/v1/credits/me`提交一次新测试token的正常GET200，确认真实available余额为正及既有自然allowance；未手改余额、升级套餐或新增公共IoT路由。

真实App仅创建一次独立测试设备`device_e622c490-7b12-4aa9-8be1-43a31867bdce`（AI生成独立ESP32 1102），原入口一次获取新Key。COM11先核实际身份／分区，新鲜保留并ROMverify原20 KiB NVS，原1,437,360 B恢复BIN与现场程序也fresh ROMverify一致；不做无必要8 MiB备份。沿公开CloudSmoke、cloud-beta及原Core3.3.11单队列冷链接一次，再缓存增量绑定本次Key和确认的YYSC网络；只写实际factory程序及隔离空NVS，不写bootloader／分区表／SPIFFS。候选0槽，无WiFiProv／任务图或旧scope迁移，App读到真实Manifest的power和uptimeSeconds两个组件。

### 55.6 唯一真实生成及只读结果协调

原App仅提交一次`POST /api/v1/interface/workflow`，实际HTTP200，完整NDJSON依次`stage → draft → complete`，workflow=`944cb551-e050-4e90-960a-642a0129ca52`、Gateway request=`8d579397e53f576a65620ce3c3b40cf2`。原页面显示约26秒、两个有效组件、无错误且Apply可用；仅查看不可交互预览，**没有Apply、控制／任务命令或第二次生成**。这次只签Development指定账号／原模型／真实Manifest的有限生成成功，不签采用、编辑保存、正式上线或所有用户可用。

初次harness把相同路径的CORS OPTIONS204选成生成响应而断言停止；真实POST200及complete已在原采集内。只读按准确POST requestId协调原证据，结果`PASS_READONLY_COORDINATED`，不重新调用生成、不修改原失败。截图、NDJSON、完整logcat、原账号／授权及journal终核均私有保留；App已回首页，CDP／临时forward释放，USB常亮恢复原值0。

### 55.7 COM11精确恢复及当前交接边界

唯一恢复前先只读保存并ROMverify临时20 KiB NVS（SHA`0e956ae1…68fb281`）；严格CRC核对实际新instance与原不同、临时TaskStore namespace／snapshot均不存在且0槽，原授权／任务未混入测试身份。随后仅恢复并ROMverify原app SHA`f8783fbe…35eaee`及原20 KiB NVS SHA`aad61112…f4179e`，分区表保持并正常boot请求完成；原BoundSnapshot／revision18／一条任务随原NVS逐字恢复。未操作任务、不重放、不自动重试，其他板未动。

Development测试账号和真实测试设备／Manifest保留供后续使用；COM11已回原身份，因此**不能继续把本批独立测试设备描述为仍由该板在线承载**。本批生成成功不依赖保留临时候选常驻。云端逐调用用量／账本收尾及历史指纹结果取§55.8，没有为补日志再发生成。

本批执行工具及证据仅在忽略目录`blinker-app/build/qa55-ai-enable-live-20261102/`、`blinker-library/build/qa55-ai-device-20261102/`、`aily-blinker-services/build/qa54-129-stao-20261102/`的受限子目录，秘密、原NVS、账号快照及完整日志不入Git。本批不提交或推送。

### 55.8 精确逐调用用量、结算及历史保持

首次只读collector误把UI的原workflowId直接过滤`agent_run_id`，因此返回0行。实际stao消费者`openai_request_registry.register_request`使用`openai-run:`加`[已验JWT区域CN, userId, 原workflowId]`规范JSON的SHA256命名空间；App路由别名Development不代替JWT区域。原Agent完整日志已包含本workflow两条200 round，分别为`d4c97541-0172-422a-adb9-b72c79ff1651`与`5df2c4a2-9386-40f7-8f64-1bb2d820911e`；这是既有“批量读取默认配置→最终JSON”的两轮工具流程，**不是网络重试**。

仅按源码公式及两round的实际Idempotency-Key SHA，补一次准确namespaced run的READ ONLY／REPEATABLE READ事务，5秒statement／1秒lock截止，读取后rollback／close。两独立request／logicalCall／admission／receipt／usage／reservation全部一一关联：request=`response_prepared`、admission=`settled`、receipt=`usage_recorded`、usage=`credit_settled`、outbox=`published`、Auth reservation=`settled`；每调用各仅一条settle ledger，676及503 credit micros，合计1179，无本批pending／UNKNOWN。receipt单独仍是usage_recorded，不把该字段独自等同结算；结论来自真实Auth和用量链共同证明。

剔除本批精确run后，原receipt49／usage49／admission0／outbox0四表完整行指纹全部与启用前一致。首次0行查询把本批新增两条留在“非本workflow”集合，得到51及fingerprint false，不代表原49被修改；原报告与输入SHA精确保留，正确关联另立`namespaced-db-correction.result.private.json`（SHA`1fd6ba52…d718891`），不改首轮报告或业务。没有重新生成、HTTP、repair／requeue／reconcile、账本修复或容量重初始化。

原收尾已确认四个容量域healthy、active全0及同Redis run_id，除本批AI外的25个受保护容器精确保持，三目标完整stdout／stderr仅私有留在远端。关联校正不再重采容量、容器或运行日志。全部SSH／CDP／串口／构建队列已关闭，设备恢复结果仍取§55.7。

### 55.9 收口决定及后续最小范围

可关闭：**129测试AI对白名单账号的启用、Development真实登录／独立设备Manifest、一次真实App完整生成预览、两轮用量／结算及COM11精确恢复**。保留当前测试AI、Development配置和正常测试会话；不退回正式token，不把临时设备误写成仍在线。

后续若继续AI产品收口，只单列**生成结果采用／保存的既有CAS及重新进入读取**差异，须先明确是否使用现有真实草稿；本批未Apply，不伪称该链已验。不要为补截图或日志重发生成，也不重跑配网、任务或自动化矩阵。正式AI兼容发布、测试服务器长期Git只读凭据、自然text／数值设备ACK及原同事分工继续独立，不成为追加无边界审计的理由；本批不再部署、刷板、发码、提交或推送。

**下一批§56已核对上述前提：原草稿不再存在，当前Cloud Console也不能直接套用旧page-layout的CAS。** 因而不按此处的“既有CAS”字样盲做成功验收，后续以§56.5纠正后的最小范围为准；本节保留为§55结束时的计划，不改写历史。

## 56. AI采用前置只读核对与Cloud Console CAS差异（2026-11-02）

### 56.1 授权及最小范围

按用户§55.9指令，复用当前beta／Development账号、现有服务和同一真实生成设备，只验原草稿采用／保存、重入读取及CAS；不重新生成、不自动重试、不发验证码、不升级SDK或提交推送。先查真实前置，不将§55完整NDJSON私有证据重新注入App，不改设备能力、固件、NVS、授权、TaskStore、云scope或UNKNOWN。

### 56.2 当前版本、账号与原草稿实际结果

ADB正常，原进程／原已安装APK SHA`948d9211…bf9f8`不变，没有force-stop、reload、构建或安装。当前仍Development、同一测试账号、JWT签名CN且access未过期；相对§55终态原非session授权与完整journal精确保持。本批开始及结束的全部secure项、账号与journal也逐字一致，没有登录或token注入。

先读取缓存页，再仅一次正常进入原路由`/device-manager/device_e622c490-7b12-4aa9-8be1-43a31867bdce/uic/ai`：页面显示“生成界面”和基础“效果预览”，`generated=false`、没有继续调整框，“应用这个界面”禁用。两个部件只是Manifest产生的基础预览，**不是§55的真实生成草稿仍在**。该次唯一实际Console GET返回200，`revision=0`、`layout=null`；同路径OPTIONS204单列，不再把预检当业务响应。

源码生命周期与实测一致：AI结果只在原`DeviceUicPage`实例的`tiles/summary/generated/previousDraft`中；同对象／账号／Manifest的缓存条件下可以保留，但换对象／owner、Manifest变化或销毁会清除。没有按workflow持久化的App草稿恢复入口。不能从当前证据确定上轮离页到本次之间的具体销毁原因，也不能把设计中的非持久内存草稿直接签为丢数据bug；可确定的是**本批已无可采用的真实草稿**。未点生成、微调、应用、预览开关或其他设备控件。

### 56.3 Cloud采用链与旧CAS不是同一消费者

当前必要源码读取结论：

- `device-uic.page.ts::apply`的Cloud分支调用`DeviceConsoleService.save`，实际PUT `/api/v2/devices/:id/console`，body仅`layout/uiConfig`，没有`expectedRevision`。
- Gateway `device-v2/console.js::put`只接受上述两字段，明确“Last successful commit wins”；事务中锁设备／布局、递增版本和权限复核不等于比较客户端旧版本。`revision`是变更序号，不能只因GET返回它就宣称有CAS。
- 当前Cloud路径另有`canEditConsole`／owner变更复核；提交layout时有Manifest fingerprint校验。它们不能代替revision CAS，也不扩大为customPanel-only的完整Manifest围栏证明。
- 旧`/page-layout`确有`expectedRevision`保护，但Cloud采用分支不用它，当前公开beta路由也不能据旧接口测试外推。原非Cloud CAS结果继续保留，不撤销也不代签Cloud。

上述是**当前源码的合同差异**。既有部署记录仅证明Gateway `e209b6…`镜像／`a4c42f…`容器沿用及其他冻结模块，缺少Console模块独立hash或新鲜读回；本批不SSH、不写测试冲突，不假称已经证明部署镜像的CAS运行行为。无草稿条件下不实施正常PUT，更不构造旧revision写入凑样本。

### 56.4 验收状态及收尾

| 项目 | 本批结果 |
| --- | --- |
| 原版本／同账号／授权及journal保留 | 通过；App未重建，当前会话保留 |
| 原AI页返场及Console读取 | 只读通过；GET200／revision0／layout null；无真实生成草稿 |
| 原草稿采用／保存 | NOT_RUN；没有可采用的真实草稿，不重造 |
| 保存后重入读取 | NOT_RUN；不能用本批读取空Console代替保存后验证 |
| Cloud旧revision拒写／CAS | NOT_RUN；当前源码缺该合同，部署实现未独立证明 |
| 生成／验证码／控制／任务／刷板／部署 | 全部未执行 |

原页观察与收尾窗口均HTTP mutation0、接口生成请求0、MQTT控制0，解码完整；没有自动重试或旧UNKNOWN重放。已回首页，原secure／journal与账号终核一致，USB常亮原值0未改，CDP及自有forward释放，无硬件或构建队列。完整前后logcat、响应及截图仅私有保留在App忽略目录`build/qa56-ai-apply-live-20261102/`受限子目录，不加入Git。

收尾harness在终核及`result.json`均已保存后，因console输出使用未定义变量发生本地ReferenceError；没有新的业务请求。保留原源码／失败及其SHA，依据已存终态、账号／journal、零写入网络记录作纯离线协调，结果`READ_ONLY_PREFLIGHT_COMPLETE_WRITES_NOT_RUN`，没有重连手机或重跑收尾。该协调不把NOT_RUN改成通过。

### 56.5 纠正后的最小后续范围

1. 下一批若授权实施，先仅对**实际Cloud Console采用／保存**补明确aggregate revision CAS：App保留Console读取版本并发送，Gateway在原事务／锁及权限复核内比较，旧版本409且不覆盖、不自动重试；保留独立Manifest／owner围栏，不误用`layout.revision`，不扩生成模型或执行链。需要先约定旧客户端缺版本的兼容策略及匹配部署，不能在本批“复用现有App和服务”约束下静默替换两端。
2. 有限离线回归只覆盖新鲜版本成功、旧版本拒写且原内容保持、失败保留草稿、正常重入，以及原Manifest／owner／权限边界；复用既有测试。Console aggregate revision可能在layout与uiConfig同批更新时递增多次，应采用服务器返回的新值，不强求加1。不用旧page-layout通过代替这条链。
3. 真实采用必须等待正常UI中仍存在的真实草稿，或用户另行明确允许一次新生成；若有新授权，安排**同页生成完成后立即采用→返回读取**连续短验，不先回首页再假设内存草稿还在。本批明确不重新生成，也不从旧NDJSON或WebView私自注入草稿，不增加持久草稿新功能。
4. 本批可关闭的是前置核对、差异定位及只读收尾，不是AI采用／CAS已完成。正式环境、原49条计费记录、设备授权／TaskStore／UNKNOWN均不操作；不重跑配网／任务／自动化或追加资源审计，暂不提交推送。

## 57. Cloud Console版本CAS与一次同页采用闭环（2026-11-02）

### 57.1 授权、合同及最小部署范围

用户明确批准§56.5的必要实现、有限回归后匹配App／Gateway部署，以及一次新生成→同页采用／保存→重入读取→旧版本拒写。本批不再要求已不存在的旧内存草稿，不从日志注入草稿、不重新绑定设备或刷COM11；保留原账号、授权、TaskStore、scope、UNKNOWN及完整日志，不发送验证码、不自动重试、不升级SDK或提交推送。

CAS沿现有Console及事务，不新造执行链：PUT必须携带安全非负整数`expectedRevision`，它是**读取到的Console aggregate revision**，不是`layout.revision`。缺／非法版本400 `DEVICE_CONSOLE_REVISION_REQUIRED`；原设备锁、权限／分享grant和owner复核后比较整体版本，旧版本409 `DEVICE_CONSOLE_CONFLICT`且不覆盖。原Manifest校验独立保留；成功采用服务端返回的新版本，不假定只加1。旧无版本客户端明确拒写，GET不受影响，不保留绕过CAS的last-write-wins后门。

App需接齐现有Console保存消费者，保留读时版本、成功后更新；不能保存前自动GET取新版本掩盖旧草稿。冲突保留草稿、禁旧版再次保存，并给明确的显式读取路径；不自动读取／重新生成／重放写入。仅生成采用及其必要共用接口接线，不接管整页或增加草稿持久化功能。

Gateway部署限定129测试宿主、同依赖旧immutable image派生唯一Console叶模块候选；不覆盖远端源码dirty，不重建Auth／Kong／Broker／Agent／Redis／数据库或AI。保留Development `api-test.yiyu.pro`及QA55 AI容量overlay／白名单／outbox，不误用base Compose把AI配置回退。App沿原SDK及单一cloud-beta队列，匹配签名／插件／HTTPS；安装保留应用数据。

### 57.2 已完成前置与待执行顺序

当前同一Development测试账号会话有效、签名CN，原APK仍`948d9211…bf9f8`，7项secure及两份完整journal相对§56终态逐字保持；前后完整logcat及必要账号状态已私有保留，无登录／验证码。当前现场不需要操作开发板。

后端唯一runtime叶文件已完成必要CAS接线，本地6项合同／HTTP回归、原PageLayout检查及TypeScript无输出编译通过；真实PG、App回归、构建／部署和一次生成采用结果**尚待后续续记，不提前签收**。最短顺序是：冻结必要源码→受影响有限App回归与独立PG真实锁／触发器验证→唯一匹配构建及部署→一次原UI生成并立即采用→正常重入GET→同设备显式旧版本API负例409及GET不变→资源／账号终核。旧版本负例是已知成功写入之后的显式验证，不是UNKNOWN重放或自动重试，不为测试制造控制／任务／活动记录。

### 57.3 必要源码、有限回归及唯一App构建

Gateway唯一runtime变更为`device-v2/console.js`（SHA`4a0fefb2…22b524`），复用aggregate revision、原事务／设备锁及两存储触发器；无新迁移或分配器。现有SERIALIZABLE失败仍有原有有界数据库重试，业务CAS的409不重试。新增／匹配Console测试及公开API文档，旧PageLayout解析与CAS不改。独立原PG14.24、候选真实dist运行两文件 **15／15通过、0跳过**，包括聚合与布局版本分离、同版本并发单赢家、旧版本不覆盖／不业务重试、no-op及真实40001边界、原分享编辑／控制权限。仅自有tmpfs PG／client／internal网络，精确ID清理，无业务库连接写入。

App接齐五处现有Console写消费者（AI／自动适配、CustomPanel、PanelEditor含embedded、DeviceComponents可见性、V2布局迁移），共8项必要源文件；aggregate与layout版本分离，读后保留版本，成功使用实际返回值，不在保存前自动GET换版本。共用save设置既有NO_AUTH_REPLAY，冲突保草稿／禁旧版保存；AI原错误区域保留role=alert，并仅在需要时提供“重新读取”显式按钮，不新建页面或改视觉布局。独立审阅发现Uic／Custom旧保存拒绝可能污染新owner的错误／busy，已补同saveEpoch／账号／设备／owner的成功与拒绝终态围栏。

App首轮9个定向spec **171通过／1失败**；唯一失败是新fixture错误要求同一个已由宿主结束的inline实例继续保存，未模拟真实host收尾，不是产品未接新版本。限定修正该例及新增两个迟到拒绝场景，续批 **3通过／171跳过**；原首轮失败保留，不重跑171项、不冒称单次全绿。14项必要源码／测试冻结清单SHA`2693e982…2c7f79c`；本批hunk审阅记录不冒充独立修改前字节备份，原无关dirty保留。

唯一原工具链队列完成prebuild→严格Web/AOT→sync→APK，均exit0、source／native／HEAD／index无漂移。APK为77,894,008 B、SHA`b2468e65…37ff257`；1158项Web资源匹配、原签名`a76639ec…5d5d210`、beta包名、HTTPS及原插件注册保持，WiFiProv原SDK2.4.4及esbuild0.28.2不升级。一次`adb install -r`并读回匹配，不清数据；当前继续保留此App。

### 57.4 Gateway匹配发布与执行端停止记录

只在129由原Gateway immutable image派生Console叶模块镜像`59d49331…ebc271`；实际运行dist及source的console SHA与冻结4a0fef一致，index→console-http→console消费边和Node／pg版本保持，不以本地源码代替镜像证据。只替换权威私有compose的Gateway image，新容器`fff2e128c525`，环境、mount、端口及命令精确保持，必要healthz一次200；发布时其余25容器及AI overlay／名单／outbox保持，原远端dirty源码不覆盖，完整旧日志／停止尾段／新启动日志私有保留。未发布正式Aily／Auth，未升级SDK或重建其余服务。

首次SSH在banner前EOF，远端命令未执行；显式续批保留原intent。首次leaf build将`FROM sha256:…`被BuildKit解析成远程仓库而403，未成功构建、创建PG或停止服务。仅修harness为此前不存在的本机base tag，前后核其仍指向原e209镜像，pull=false／network=none、无RUN或安装；保留失败Dockerfile／日志，显式续接未通过的构建／PG，而非重跑已过业务。独立PG通过后才唯一发布Gateway，没有自动循环或未知写入回放。

### 57.5 两个失败工作流分别保留，不伪造AI草稿

首次原UI调用workflow=`a49d203d-bd94-4a98-ad42-e8dfe759a31d`、Gateway request=`8cf293d84d31c21d799f2dd3e2ee7049`：HTTP200及stage后约49秒失败，App没有生成草稿，Console PUT0。唯一只读关联确认第二模型轮terminal `failed / upstream_error`、Agent `MODEL_REQUEST_FAILED`；不是直接HTTP504，也不是仅凭浏览器ERR_ABORTED推断网络错误。收到流内错误后App取消XHR会产生ERR_ABORTED，不能把它倒置成根因。原轮与全部日志／请求身份保留，不修复或重放旧UNKNOWN。

用户随后明确“服务器问题另一个session已经修复，继续主线”，本会话停止服务器排查，未做该修复或冒称知道供应商细因。按这次显式继续指令，新建独立证据目录及新工作流，未重放上笔：workflow=`2edc153a-517f-4adf-a796-2f79a68c8324`、request=`958b4c09d5ab1c62c167e8a489849b96`，约24秒HTTP200，真实正文仅`stage → error(INTERFACE_AI_INVALID_RESPONSE)`，没有draft／complete或Console PUT。可签的是**服务端拒绝生成内容**，本批未进一步确认是工具参数还是最终proposal校验，也不把它签成App CAS失败。没有放宽校验、注入旧NDJSON、恢复不存在草稿或再发第三次生成。

第二批冷入口harness曾过早检查缓存错误提示而停止，另有OPTIONS204无body触发CDP读取失败；两次均在新生成前、HTTP业务写0。保留各停止记录，限定等待真实离页／入页完成并将无body记录为unavailable，恢复自有forward后才完成原只读前置；不是通过固定等待清业务隔离。服务器后续外部修复造成的AI重启不属于本会话发布，不再宣称跨他人修复期间25容器始终原时序不变。

### 57.6 实际Console CAS真机短验：自动适配输入，非AI采用

为不继续消耗模型调用，已先明确向用户说明替代范围：使用原“自动适配”页面的真实Manifest预览，调用与AI采用相同的`DeviceUicPage.apply → DeviceConsoleService.save → PUT /console`，不将该结果冒称真实AI草稿采用。

| 步骤 | 实际结果 |
| --- | --- |
| 原预览与权限 | 两个原能力字段，预览inert，canEditConsole有效；没有AI草稿注入 |
| 单次原UI采用 | `expectedRevision=0`，PUT200，整体revision=2、layout.revision=1；原触发器真实产生，不固定加1 |
| 采用后进入设备页 | 原导航成功，两个真实绑定组件显示，无开关／任务命令 |
| 离页后重新进入 | 新GET200，整体版本、layout及uiConfig与保存响应完全一致 |
| 显式旧版本负例 | 同设备、同账号、已知成功PUT的旧expectedRevision0，仅一次PUT，409 `DEVICE_CONSOLE_CONFLICT` |
| 拒写后再读取 | GET200，revision仍2，layout及uiConfig不变，没有覆盖或自动重试 |

这条替代窗口新生成请求0、MQTT控制0；两条Console PUT分别为正常UI保存和显式CAS负例，不是UNKNOWN重放。`cas-only-result.json`明确标记`REAL_AUTOMATIC_PREVIEW_NOT_AI`。因此**Cloud Console新鲜保存／重入读取／旧版本拒写已有限通过；本批AI草稿同页采用仍NOT_RUN**，没有拿自动适配、旧PageLayout或空Console GET代签AI结果。

本轮独立测试身份仍未由现场COM11承载，设备页截图中的unreachable与§55已恢复原身份的边界一致；验证的是离线也可保存／读取的云页面资产，不是设备在线、实时数据或控制ACK。本批没有为补在线状态再次刷板。

### 57.7 当前交接及下一步最小范围

匹配新Gateway／App及已保存的测试Console保留，App已回首页，原账号、非session授权及两份完整journal终核保持；原USB常亮0已恢复，CDP／自有forward释放。全程没有刷板、改设备能力、触碰NVS／TaskStore／scope、发送验证码或重放UNKNOWN，所有SSH及编译队列已结束。完整原失败、两个工作流、网络响应、前后logcat、保存／重入／CAS截图与云部署证据仅私有留在忽略目录，不提交推送。

后续**不再重复CAS、配网或任务矩阵**。若继续AI采用，只针对现有2edc工作流的已有Agent日志／校验原因做有限分析，复用同一已部署保存链，不升级SDK、不弱化约束或默默重新生成。需要新的模型调用须另行明确安排；拿到合法真实草稿后，在同页立即采用即可，不再回首页丢内存草稿。旧无版本App现在会明确拒绝Console保存，需匹配更新；本批源码尚未推送，同事Git拉取不应被称作已经包含本批CAS修复。

## 58. 已有AI拒绝日志定位与严格校验脱敏诊断（2026-11-02）

### 58.1 本批范围及现有证据

按用户§57.7继续：仅分析已有日志、确认后最小修正及有限回归；不放宽校验、不重新生成、不重复CAS、不自动重试、发码、升级SDK或提交推送。本批没有部署或构建安装App，不连接手机／开发板，不操作账号、授权、TaskStore、scope、UNKNOWN或业务数据库。保留§57匹配App／Gateway及保存的测试Console。

129仅一次只读提取既有Agent完整日志及实际bundle证明，SSH已关闭。工作流仍为`2edc153a-517f-4adf-a796-2f79a68c8324`，原请求`958b4c09d5ab1c62c167e8a489849b96`；两条带精确workflow关联的模型round均HTTP200：`5dd28780-7810-41ce-8998-fd2d8272013b`及`253cac48-7160-4b72-ad36-c1b4ec4cce58`。约25.2秒后的唯一相邻warning为`MODEL_INVALID_OUTPUT`；**该旧warning本身没有workflow字段，仅能以相邻时间窗关联**，不冒称具备逐字段日志。原App已保存的实际正文为`stage → error(INTERFACE_AI_INVALID_RESPONSE)`，无draft／complete及Console PUT。

Agent为原`d06759…`／image`42eb31…`，本次读取时StartedAt／restart稳定；实际运行是单bundle `dist/index.js` SHA`5f012f74…d0a03e`，与§51冻结交付精确一致。model／workflow／default-config／contract逻辑在该bundle中，不是部署了五份独立runtime文件；对应源hash由原manifest追溯。结果及源证明取Service私有忽略目录`build/qa57-console-cas-20261102/private/agent-validation-2edc.result.private.json`，SHA`643fd1b2…891650`；完整双流日志只保留远端0700目录，无模型、业务HTTP或数据库采样。

### 58.2 确认的停点与无法恢复的信息

实际App请求的power可写boolean、uptimeSeconds只读integer及11项默认配置均符合入口合同；SDK1.1.0提供的toolCall.arguments已是对象，原取值正确，不需要再次JSON.parse。既有流程只有成功执行第一次默认配置工具后才发第二轮，因此结合两条精确round和最终安全reason，可将失败收窄至**第二轮最终文本进入`parseProposal`后被拒**，不是首轮工具选择、工具参数、App CAS或仅HTTP504。

确定的代码缺口是`runWorkflow`将JSON解析、Zod schema、权限／绑定、variant／尺寸、覆盖／extras等所有proposal拒绝统一抹成`MODEL_INVALID_OUTPUT`，HTTP再统一映射INVALID_RESPONSE；原warn又缺workflow。现有日志未保存模型原文／被拒路径，**无法知道这笔具体违反哪条规则，不能据此猜测模型多了options、尺寸错误或提示不兼容，更不能补造历史输出**。本批修复的是此诊断缺口，不宣称已经修正未知的模型输出形态。

### 58.3 最小源码修正：原接受／拒绝结果不变

仅Agent三项现有源文件及一个窄helper：

- `proposal-diagnostic.ts`：固定JSON／schema／字段覆盖／字段类型／绑定／variant／尺寸／extras／强调及未知拒绝分类；语义错误保留原中文内部message，只附安全分类；最终wrapper message仍为`MODEL_INVALID_OUTPUT`，不保留cause／原解析器文本。
- `workflow-contract.ts`：原schema、默认值、每个predicate及语义消息保持，只在原拒绝处附固定类别和schema路径。不清理、补全、强制改写或接受原非法模型结果。
- `workflow.ts`：原catch仍拒绝本次输出，但保留上述安全分类；不增加模型回合、重试、fallback或自动生成。
- `workflow-http.ts`：沿原warn频率／级别输出单行JSON，附workflow及安全分类；**公共NDJSON仍只有原error code，不附诊断、字段值或模型原文**。

诊断最多3项issue、每条最多4段白名单schema path，索引仅0～63，未知段归为固定marker；只记录固定Zod code，不记录unexpected key／value、prompt、tool arguments、JSON SyntaxError.message、Zod全文、账号／token或stack。合法UUID可精确关联，其他原合同合法的ASCII workflow标识仅记录固定marker及SHA256，不收紧原请求合同。自定义Error.name也不原样回显。日志单行上限由有限测试核对≤1024 B。

### 58.4 有限回归结果及原版本边界

新增`test/workflow-proposal.test.ts`及`test:proposal`脚本，复用原Node22.22.3／ts-node ESM、pi-ai1.1.0、Zod4.6.5和TypeScript；不安装或升级依赖，不导入会serve的index。定向队列首跑 **22／22通过、0失败／跳过**，随后原Agent `tsc --noEmit` exit0，未开重复队列。

覆盖合法两字段／一次批量默认工具→完整proposal；JSON、未知root／widget key、required／type、只读权限、variant／尺寸、漏／重复绑定、extras、强调及固定binding改变仍全部拒绝。脱敏哨兵验证未知键值／parser message／自定义错误名／非UUID原标识不进入日志；Hono内存请求仍HTTP200且仅stage／error，原INVALID_RESPONSE不变，新warn正确关联workflow。原SDK由精确mock fetch提供两轮完整SSE，合法流程仍两轮，错误工具或参数仅一轮即拒绝、不重试；**这些是离线fixture，不是真实模型调用、服务器HTTP或历史2edc输出复现**。

原三个已跟踪源文件在本批开始时clean，精确diff仅诊断接线／抛错附带信息；schema、prompt、SDK工具适配和模型选择不变。版本／锁及App、Gateway CAS源不改。完整意图／结果／测试／typecheck日志取Service忽略目录`build/qa58-agent-proposal-20261102/first.*`；无关本地修改保留，未暂存、提交或推送。

### 58.5 收口决定与后续入口

可关闭：**现有证据的最终proposal拒绝定位、错误原因丢失／缺workflow关联的源码最小修复及22项有限离线回归**。不能关闭：历史2edc具体被拒字段、真实模型输出合法性或AI草稿采用；旧日志没有的信息不会因新代码而出现，本批没有重新生成来凑证据。

线上仍为旧bundle，本批未构建／部署诊断候选，不能把源码已修写成运行日志已具新分类。后续如需发布，只单列匹配Agent的必要构建／发布及健康边界，不重建App／Gateway或重跑已过CAS；新的真实模型调用仍须明确安排，沿正常原链取到真实结果才验同页采用。保持严格校验，不以接受非法布局或自动循环作为“修复”。

以上为§58结束时的快照；随后按新授权完成的诊断候选发布及唯一新工作流另见§59，不回写历史证据。

## 59. Agent诊断候选发布与一次真实生成分类（2026-11-02）

### 59.1 冻结输入与最小发布

按用户新授权，仅构建／部署匹配Agent并允许一次新生成：被拒就核对脱敏分类；成功才同页采用／保存及重入读取。本批不重复§57 CAS，不重建／安装App或Gateway，不刷板、不操作TaskStore／scope／授权／UNKNOWN，不发送验证码、不自动重试、不升级SDK或提交推送。

复用已安装beta APK `b2468e65…37ff257`及实际Gateway Console `4a0fefb2…22b524`。Agent四项必要源及原依赖／锁与§58通过输入精确匹配，沿原Rslib1.0.3仅一次bundle构建，1.518秒exit0；复用22项已过回归及原类型检查，不开重复队列。公开制品为61,950 B单bundle，SHA`66d974536d642b056f04cc451ff6958517cb1599780dd3ac737ce266fd06059c`，manifest SHA`4dbc3016…379fb3`；不含私有配置，schema／语义predicate、提示词及模型maxRetries0不变。

只在129测试宿主由原immutable Agent image派生叶镜像，仅COPY该bundle；原User、环境、mount、端口、命令及entrypoint精确保持，Development仍`api-test.yiyu.pro`。首候选因0700私有暂存下的公开JS为root:0600，原非root User在语法检查前EACCES；**当时未停止旧Agent、未改运行配置**。保留首镜像、Dockerfile及失败日志，明确续接仅将公开JS的COPY mode设为0644，不改User或放宽私有配置权限，不重新构建本地bundle。一次SSH续接曾在banner前EOF，远端未执行，原intent保留，无自动循环。

最终镜像`1ccc998d…301ee4d`、CID`e6a95d9610ed`；实际运行bundle的SHA／长度与冻结制品一致，原User下`node --check`通过，Node22.23.3，诊断及原公共错误码标记有效。仅权威compose的Agent image变更，其余25个**发布时当前**消费者前后metadata精确保持，原AI overlay／容量／名单／outbox及Auth／Gateway／Broker／数据库不改。healthz仅一次200、restart0；完整旧日志、停止尾段、新启动日志及配置私有保留，SSH关闭。

### 59.2 新鲜前置与唯一原UI生成

先保留原App账号、7项secure、两份完整journal及logcat，同Development账号／APK核对通过；只唤醒正常已运行App，不退出／登录。首次显式Activity名称未匹配，没有启动业务；读取实际包入口后正常恢复原实例。运行接受记录由真实部署报告、制品manifest及既有Gateway证明推导，未用手填标志代替运行证据。

原AI页真实离页后重入，唯一新GET Console200读到aggregate revision2，两个原Manifest组件、inert预览及生成入口就绪，无生成／写入。随后只点击一次原生成按钮；write-once意图及method=POST关联避免把OPTIONS当生成，不从旧日志注入草稿。

| 证据 | 本次结果 |
| --- | --- |
| 新workflow | `6d626e03-4ee4-4f44-a8cc-e74e02086e14` |
| Gateway request | `236da5fb9de3a6dca4e3e3b14c6f9e62` |
| 原App请求 | POST200，仅一条；约37.53秒后原页面显示“生成结果未通过校验，请重试。”，未点击重试 |
| 原App终态 | generated=false、Apply禁用，保留两个基础预览；不是合法生成草稿 |
| 实际业务写 | Console PUT0、采用0、MQTT command0、其它意外写0 |

客户端收到错误后取消流，CDP正文不可得且记录ERR_ABORTED；不伪造draft／complete／错误事件或仅凭abort推断网络失败。结果记`REJECTED_NOT_ADOPTED`，证据来源是原App错误终态；独立服务端诊断取下节，不将HTTP200等同生成成功。未执行成功分支的采用／保存／重入读取，也不以§57自动预览代签本次AI采用。

### 59.3 同workflow的实际安全分类

仅一次只读提取新Agent完整双流日志及实际runtime证明，精确同workflow在`2026-11-02T08:00:17.845Z`的唯一warning为`MODEL_INVALID_OUTPUT`：

```json
{"reason":"SCHEMA_VIOLATION","issues":[{"code":"invalid_value","path":["widgets",0,"type"]},{"code":"invalid_value","path":["widgets",1,"type"]}]}
```

两轮模型日志分别`c3737755-e8ff-459e-955a-2db11d9aef12`及`c9341d6c-b287-4297-a03b-fe38c4a69327`，均HTTP200；Agent CID／image／bundle不变、restart0。确认本次最终proposal的两个`type`未通过原组件类型枚举，不是本次Console CAS拒写；**未保存实际非法值，不能断定填了哪种值、缺失还是混用了variant，更不能据此回填历史2edc细因**。原类型枚举、字段权限／绑定及其它校验保持，不加容错接受非法布局。

新日志只含workflow、固定分类／schema路径／code，无模型原文、提示词、账号或秘密；完整日志保留0700私有目录，关联读取未调用模型／业务HTTP／数据库或修改配置。服务器SSH已关闭，没有第二次生成。

### 59.4 收尾、证据与下一批最小范围

App正常回首页，实际AI／设备旧页不可见，原非session授权及两份journal逐字保持；自有Network监听、CDP和forward释放，常亮仍原值0，完整前后logcat及网络记录保留。COM11及原固件／NVS／TaskStore／UNKNOWN本批均未操作；保留当前诊断Agent及原App／Gateway，不回滚正常测试会话或已保存Console。

证据位于忽略目录：Service `build/qa58-agent-diagnostic-release-20261102/`的单一公开制品；`build/qa57-console-cas-20261102/private/qa58-agent-mode0644.result.private.json`部署报告SHA`b9b54500…6afa3`、`qa58-workflow-6d626e03.result.private.json`分类报告SHA`83b53b05…36757`；App `build/qa59-ai-diagnostic-live-20261102/private-hcUbH2/`的原UI截图、请求／终态／账号／journal保护及完整日志。私有证据不进入源码交付，不公开凭据。

**本批可关闭：诊断候选匹配构建／发布、健康及真实拒绝的精确安全分类。仍待：合法AI草稿的同页采用／保存及重入读取。** 下一批只基于已知`widgets[*].type`失败路径，离线核对最终输出合同、`allowedComponents`、默认配置工具及提示词中type／variant的语义，明确最小修正并补有限fixture；不得猜造本次模型原文、放宽枚举、强转非法值或自动再生成。没有新修正和明确的一次真实调用安排前，不重复模型／CAS／配网／任务矩阵。

## 60. AI组件类型输出合同离线澄清与有限回归（2026-11-02）

### 60.1 本批授权与同事对接方案的边界

按用户§59续批，仅离线核对并实施必要最小修正、有限回归；不调用真实AI、不重复CAS、不自动重试、发码、升级SDK、部署或提交推送。本批不连接手机／SSH／开发板，不读取或修改业务库、账号、授权、TaskStore、scope、UNKNOWN及原完整日志。已有无关dirty保留。

完整读取用户提供的2026-10-15《智能体模型接口实现与对接方案》（789行）及本地原执行记录。该文明确**Aily负责模型访问、Auth／区域、auto选模及Credits，Blinker Agent负责提示词、默认工具和多轮编排**；是当时待实现约定，不是当前已部署或全场景通过的证明。旧Development IP已由后续测试域名取代，不按旧地址回退或跨环境试调用。

当前模型适配已按原SDK发送用户请求局部凭据、独立轮次ID／幂等键，出站不含model、不加服务不支持的response_format／strict参数；这与最终`widgets.type`属于不同层。§59两轮HTTP200且已进入`parseProposal`的schema错误，不是本批Auth／404／504或Console CAS错误。不能因业务结果非法就改回共享Key、放宽响应校验或改动Aily路由／账本。

### 60.2 为什么以前成功而当前有拒绝

1. 2026-10-10旧记录确有真实模型成功，但为OpenCode Go／DeepSeek及六组合成组件脚本，不是当前App同页采用；`37983b1`后来将生成简化为默认工具＋最终JSON两轮，`9a3b670`再接入Aily用户鉴权、区域及auto。前后提供商与入口不同，不能简单等同一次恒定输入的回归。
2. 组件类型七项枚举起于`6c7ff5a`、严格对象校验起于`743e8ff`，并非本周CAS／诊断新加；`9a3b670`并未改原类型或workflow提示文本。10月15日对接执行记录当时也明确“挑战接口通过”不代表已取得生成／计费证据。
3. 更直接的反证是§55在当前测试链路真实收到draft／complete；后续旧`2edc`内容拒绝仍使用与§51相同的旧Agent bundle `5f012f74…d0a03e`。§59新bundle只接诊断；与§51冻结输入核对，model、layout-guide、default-config-tool等逐字相同，原校验predicate／默认值不变。因此**没有证据证明最近CAS／诊断将原本接受的相同输出改为拒绝**。
4. 已确认的只是§59最终两个type未通过枚举；没有保存实际非法值，不能断定是填了外观名、数据类型、缺失字段或上游换模型。本次查到的是确定可改的输出说明歧义，不能把它冒称已复现历史模型的具体错误。曾成功不等于输出可靠性已稳定，当前内容拒绝也不等于整个AI访问接口不可用。

### 60.3 实际合同核对与最小范围

| 层级 | 唯一含义 | 当前有效示例（说明用，非历史模型原文） |
| --- | --- | --- |
| Manifest `valueType` | 设备数据类型 | boolean／integer |
| Widget `type` | 绑定／控制的功能类别 | switch／value |
| Widget `variant` | 同一类别下的外观 | tile／uptime |

Agent原七类`switch/button/slider/select/input/value/color`与App相同，本例两字段的`allowedComponents`、type→variants、尺寸及默认配置返回与App一致；没有为“修复”改映射或放宽只读字段。本例外的退化数值范围差异不纳入本批。

确定缺口：原system示例将type写为泛称“组件类型”，width又写成并非合法JSON的`2或4或8`；guide主要按variant名介绍，工具参数名components实为variant列表。虽然数据中已有正确的`allowedComponents`及工具返回type／variant，但没有把三层区别及最终type枚举说清楚。最小修正限定三个源文件：

- `layout-guide.ts`直接取**现有`widgetType.options`**生成精确枚举，不复制第二套类型表；说明先从该字段同一allowedComponents对象取type，再从其variants选外观，禁止拿valueType／variant作type或跨对象组合。
- `workflow.ts`仅把伪JSON改为明确必填／可选字段及尺寸说明，保留字段覆盖／唯一绑定、微调原id／type、默认宽高／样式、options继承、不输出设备指令及原两轮调用／诊断。
- `default-config-tool.ts`仅改description：components按variant查询，返回type和variant对应最终同名字段且仍受字段能力限制；**name、JSON参数schema、execute及所有返回值均不变**。

没有给非法type自动纠正、别名映射、默认为value或过滤丢弃字段；原schema、predicate、allowedComponents构造、默认值、model.ts、公共协议、SDK及依赖锁均逐字保持。新增独立type-contract spec，并接入既有`test:proposal`脚本；不新造生成流程／测试服务。

### 60.4 有限回归与首次失败保留

先在原guide运行一个新case，实际缺少`JSON.stringify(widgetType.options)`枚举而断言失败，保留修前日志；这证明输出说明缺口，不冒充复现真实模型随机输出。三源最小修正后，新七项首轮 **5通过／2失败**：通过项覆盖原能力／默认工具值及options不变、合法生成／微调绑定、非法外观名／数据类型／缺失type仍被拒、原variant／只读／绑定围栏。

两项SDK fixture失败为新测试误将原SDK `user.content`的文本块数组当字符串JSON解析，错误被SDK折叠为MODEL_REQUEST_FAILED；已对照实际序列化代码，只修夹具读取和保留mock原断言，没有改生产适配。**仅续跑这两个未过case，2通过；不重跑已过5项或§58旧22项。** 原Agent `tsc --noEmit`随后exit0；不是单次七项全绿。

续验沿原pi-ai1.1.0、精确离线fetch／SSE，无真实网络fallback，验证工具首轮强制一次查询→公开工具结果→第二轮最终文本；原工具type／variant／options精确保留、请求字段子集不变，无model／response_format／新参数。合成非法type仍给出两个安全schema路径、无draft／complete／第三轮；这些合成值只用于验证拒绝合同，**不是§59实际非法值的重建**。

证据取Service忽略目录`build/qa60-agent-type-contract-20261102/`：baseline、修前red、首轮regression及两项sdk-continuation的意图／完整日志／结果分别保留。原type／validator SHA`4f7f4be6…7d060fe`、model `7d26d522…43a3e3`、原两锁及诊断helper均未变；必要源新SHA分别为workflow `128c94dd…e5f614`、layout-guide `a5bee9fb…9a528e`、default-tool `9c5ac187…b16bc`。新测试经既有脚本可持续运行，本批没有再执行旧矩阵。

### 60.5 收口与最短后续

本批完成**接口文档／历史分层、输出说明歧义的最小修正及七项有限覆盖／原类型检查**，不宣称真实模型内容已经稳定合法。云端继续§59诊断bundle `66d97453…6059c`，App／Gateway及原保存Console不变，本批无构建、部署、AI／登录／设备业务请求、暂存提交或推送。

后续若明确授权发布，只需构建／部署这份Agent说明修正候选，保留当前App和Gateway；健康／实际bundle核对后，另行安排唯一一次正常生成。合法时立即同页采用／保存并重入新GET；拒绝时仍读本次安全分类即止，不自动重试、放宽校验或重跑CAS／配网／任务。没有当前部署与真实新结果前，AI采用／保存仍待验。

## 61. 说明修正候选发布与唯一生成的JSON语法拒绝（2026-11-02）

按用户新授权仅构建／发布Agent并真实生成一次；成功才采用／保存／重入，被拒则核对分类即止。冻结§60三项源与分批7项／原类型检查证据，沿原Node22.22.3／Rslib1.0.3仅一次构建，1.062秒exit0，不重跑旧测试、不装依赖／升级。单bundle63,136 B、SHA`e57d505bac6b467cd1ededd41c01b1a5eb4545005fa4bc5da5120e9f6dd55fdb`，manifest SHA`1c1b2f65…066f37`；19项静态标记及原依赖／9个external imports匹配，validator／model／诊断／锁保持，未放宽校验。

129仅由原Agent镜像派生叶镜像`9c1c0447…46bcbb1`，COPY公开JS为0644；原User、env、mount、port、cmd／entrypoint精确保持。实际CID`f5726df5b24c`、bundle hash／长度匹配，原User语法检查通过、healthz一次200、restart0。当前其余25个容器及AI overlay／Development `api-test.yiyu.pro`保持；Gateway当前CID／image仍匹配§57已验报告，因此沿用Console `4a0fefb2…22b524`证明，**并非本批新鲜读模块字节**。完整配置、旧全日志／停止尾段及新启动日志私有保留；SSH关闭，没有其它服务或业务数据库变更。

原App `b2468e65…37ff257`未重建／安装；同Development账号7secure及两份完整journal保留。实际部署报告＋公开manifest＋匹配Gateway证明导出运行门禁，三source hash及bundle双向核对。真实离页后进入原AI页，新GET Console200／aggregate revision2，原两个能力及inert预览就绪，仅读前置无写入。

唯一新workflow=`32e45400-fa3d-48c2-814c-cd1b27ec4402`，Gateway request=`fd8e6eaacd57ad2ea3abce04e84a25ed`；原UI仅一次POST200，约39.41秒，实际完整NDJSON为`stage → error(INTERFACE_AI_INVALID_RESPONSE)`，没有draft／complete。原页面显示校验失败，Apply禁用；采用0／Console PUT0／设备HTTP写0／MQTT命令0／其它意外写0，未点击重试。

随后仅一次只读关联新Agent既有日志：两轮`208ecd2b-49cf-450d-ae85-82aab4e9d5ae`及`bdde766b-acbf-4cc7-9607-dae7aca3df19`均HTTP200；同workflow在`08:28:07.211Z`的安全分类为`MODEL_INVALID_OUTPUT → JSON_SYNTAX`，无schema path／issue code。实际运行CID／bundle保持、restart0；完整双流日志0700保留，SSH已关闭。**本次是进入`parseProposal`的最终文本无法JSON解析，不是上批type枚举错误。** 未保存原最终文本，不能猜测是Markdown包裹、缺标点、截断或其它形态，也不能证明§60提示修改已修复生成可靠性。

App已正常回首页，原授权及两份journal逐字保持，Network／CDP／自有forward释放，常亮仍原值0。COM11／固件／NVS／TaskStore／scope／UNKNOWN均未操作；没有重复CAS、配网、验证码、SDK升级、提交或推送。当前保留已部署Agent及原App／Gateway，不伪造草稿或成功采用。

证据：Service `build/qa61-agent-type-release-20261102/candidate-aCokKJ/`；`build/qa61-agent-release-20261102/private/deploy.result.private.json` SHA`8db4280c…6f4f91`、`qa61-workflow-32e45400.result.private.json` SHA`5438b5e6…ee921d`；App `build/qa61-ai-type-live-20261102/private-VhUMgP/`含账号、journal、前后完整日志、截图、NDJSON及网络终态。私有证据不进入Git交付。

**推进方式纠正：Agent调用流程已通，剩余是最终产物合同可靠性，不应继续重装App／反复真机试提示词。** 公共INVALID_RESPONSE会合并工具、JSON及schema错误；安全分类只证明停点，未保留可重放失败样本则无法进一步确定性修复。下一批应先在Agent最小两字段路径明确有界脱敏失败样本与离线重放合同，定位最终文本／结构边界；再根据当前兼容能力选定输出约束，不盲加现入口不支持的response_format／strict，不改Auth／CAS或放宽语义校验。本批遵照“被拒即止”，不实施下一项、不再生成。

## 62. 双模式生成输出边界重整与离线验证（2026-11-02）

### 62.1 授权及历史设计的关键差异

用户先要求分析旧Agent及参考当前Aily Blockly ABS→ABI，随后明确“有更好方案则直接执行”，并要求保留**现有组件生成、直接HTML生成**两种模式。本批据此实施本地输出边界，不再以猜改提示词后反复真机尝试作为修复；没有追加真实模型、验证码、部署、App安装、CAS或设备操作，原账号、授权、TaskStore、UNKNOWN及完整日志未动。

补核实际Git代码后，确认此前分析遗漏的一项关键历史差异：

- `52a3ca3`曾有匹配→最多三轮设计纠正→渲染／视觉审查；`37983b1`已在10月13日将这套流程删除，不能把它当作交付版本仍在后台使用的兜底。
- 同事交付工作区基线`631b743`已是**默认配置查询＋最终文本两轮**。它的`model.ts`最终一轮显式设置`response_format:{type:'json_object'}`（Responses为`text.format`）；之后仍是直接JSON.parse＋Zod，没有Markdown剥离或通用JSON修复。
- `9a3b670`为兼容Aily已部署子集去掉了该参数。故“schema／提示文本未变”并不足以描述全部历史行为：**输出语法约束确实变弱了**。这与当前JSON语法拒绝的风险吻合，但原失败文本未保存，不能将它签成某次具体错误的唯一原因；JSON模式本来也不能保证type枚举或字段权限。

不直接加回当前未证支持的response_format／strict，不回滚Auth／区域／计费或CAS。采用现接口已支持的命名function工具来提交组件产物，HTML则取消不必要的外层JSON字符串。

### 62.2 ABS参考原则与最终方案

已完整读取`aily-blockly/AGENTS.md`并核对dirty，仅读其现实现，未修改Blockly。参考的是**意图／候选与最终表示分层、按权威定义转换、先校验再提交**，不是宽松修复JSON。当前ABS主链经syntax／precheck／reconciler／native准备及读回，工具明确区分validate和apply；词法失败不猜边界恢复。旧converter utility不等于当前主链。

本项目已有Proposal中间表示及App确定性PageLayout转换，不需要再造页面DSL、复制Blockly编译器或放宽原schema：

| 模式 | 模型侧输出 | 宿主边界 | 原有下游 |
| --- | --- | --- | --- |
| 组件 | 第一轮default_config_tool，第二轮强制submit_layout | 原Proposal schema派生工具描述；原始参数完整解析后走原语义校验 | 原draft／complete→App转换／预览→用户采用→Console CAS |
| HTML | 单次完整原始HTML，不转义成JSON字符串 | 128KiB／非空／NUL／文档首尾检查，由宿主包装{html} | 原mode=custom complete→App校验／沙箱／CSP／能力权限→用户保存 |

组件仍最多两轮，HTML仍一轮；不增加自动模型纠正、循环重试或后台写入。工具schema也不等于提供商绝不会产生非法参数，失败仍明确拒绝，不发布半成品。

### 62.3 最小实现范围

- 原`proposalSchema`仅导出，不改schema／predicate。`proposal-output-tool.ts`用原Zod `toJSONSchema(io=input)`派生`submit_layout`参数，仅去掉版本声明，不复制另一套字段表、不加入strict／response_format；execute仍走原parseProposal及脱敏错误。
- `workflow.ts`仅改最后提交方式及必要提示；原default工具、能力／绑定、时限、NDJSON、微调好草稿和App owner／CAS保持。宿主验证后的对象编码供原消费者继续读取，不消费模型的自由说明文字作组件布局。
- `model-tool-output.ts`按请求／轮次局部累积原始function参数，限制UTF-8字节、唯一index／ID／名称及显式tool_calls结束。名称可正常分片，但最后必须精确匹配；参数只用完整JSON.parse，无SDK partial repair／jsonrepair／正则抽取／猜测补齐。
- 原SSE观察器只增加有界消费者接线，继续验证finish／usage／DONE及4MiB流上限。共享model沿原headers、独立幂等键、原整次截止及maxRetries0；强制最终工具只用于组件，HTML及旧文本入口仍单轮文本。SDK的增量修复对象只用于其内部进度，不成为最终授权事实。
- HTML仅调整输出格式提示，保留全部原主题／能力说明。宿主校验整串完整文档，拒围栏、JSON封套、解释文字、空值和缺尾，不提取片段或修复代码；原App沙箱／权限没有修改，文档首尾检查不冒充HTML语义或安全检查。

### 62.4 有限回归、类型与打包

唯一队列限定六个生成相关spec：受协议变化影响的原两个spec仅更新第二轮工具fixture，另加工具原始参数、组件输出工具、HTML输出和原SDK双模式集成四项spec。**首跑72／72通过，0失败／跳过**；包含原schema／权限／绑定／UTF-8／尺寸围栏、新强制两工具完整流程、分片名称／参数、未知／多call、结束／大小／隔离、原SDK会修复但原始JSON非法的拒绝，以及HTML单双引号／反斜杠／script／options不需外层JSON转义、完整文档、截断／取消拒绝。所有网络为精确离线mock且无fallback，无真实HTML执行／模型调用。

首轮类型检查报一处unknown不能写入SDK JsonObject历史，已只补严格JSON.parse及根对象检查后的原SDK类型声明／断言；未改运行逻辑，保留首次错误。仅续类型检查exit0，不重跑72项。随后一次原Node22.22.3／Rslib1.0.3、build --no-env打包exit0，单ESM bundle约68.5kB；无安装／升级、源码与锁在队列中无漂移。原strict校验与Auth／控制／任务／自动化／CAS链未改；更新既有test:proposal命令以覆盖本批spec，README同步双模式合同。

证据仅在Service忽略目录`build/qa62-agent-output-boundary-20261102/`，first与continuation-1／2的完整意图、结果及日志分别保留。独立只读审阅双模式、raw参数及共享model边界未发现阻塞项；无关dirty不覆盖，没有暂存、提交或推送。

### 62.5 当前收口与剩余一步

本批完成输出边界设计、两模式本地实施、原SDK离线集成、类型检查与打包；**不是已经部署或真实模型验收成功**。云端仍§61 `e57d505b…d55fdb`，App／Gateway不需随该内部改动重建。

下一最小范围是发布匹配Agent后，在Agent最小请求层分别有限验证组件两轮强制提交、HTML单轮原文及下游事件；合法产物才进入App采用／读取。必要真实调用须按新批次明确安排，不因离线通过自动调用，不再重跑配网、账号、任务或CAS。若失败，只记录该模式／阶段的有界安全分类并停止，不将两种模式统统归为“JSON页面生成”。

## 63. 双模式Agent发布、组件采用闭环与HTML输入拒绝（2026-11-02）

### 63.1 匹配发布及保全

按用户新授权部署§62匹配Agent，沿原App／Gateway，组件→HTML各一次原UI短验，成功才同页采用／保存及新GET；任一失败核对分类后停止。本批不放宽校验、不自动重试／发码／升级SDK或提交推送，COM11／原固件／授权／TaskStore／scope／UNKNOWN不操作。

只封装§62已有68,514 B bundle `17c7bcc3e208249e4144bb79b70c48a269aba153b0776a617d12dd3051048d2a`及公开manifest `d8596a14…e0019b`，源码／测试／锁与原打包输入精确匹配，72项通过及原首次类型失败／纯类型续查证据分别保留；没有重复构建、测试、安装或依赖变更。第一次包装审计误要求源码注释保留到bundle，copy前停止；只纠正该审计标记并保留停止记录，不改实际制品。

129仅替换Agent image，当前image `e08b36f8…dace51`／CID `5aaa35b8bec2`，实际bundle及原User语法匹配，healthz一次200、restart0。原env／mount／port／cmd／entrypoint、其余25个当前消费者、AI overlay及Development `api-test.yiyu.pro`精确保持；Gateway当前CID／image仍匹配§57证明，沿用Console `4a0fefb2…22b524`适用性，不冒充本批重新读模块。完整原配置、旧日志／停止尾段及新日志私有保留，SSH关闭。

原beta仍`b2468e65…37ff257`，没有重建或安装。首保全被系统NotificationShade遮挡，document.hidden保护在记录读取前停止；只读定位后收起通知栏正常恢复原App，保留首次停止／logcat，显式续完未完成的保全，不清数据／重启会话。7secure及两份完整journal保留，同Development账号有效，不发验证码。运行门禁由实际报告、公开manifest及匹配Gateway证明推导；原配置页的两个真实模式按钮分别进入各自页面，没有私有controller调用、草稿注入或直接代写API。

### 63.2 组件模式：真实生成与采用已有限通过

新鲜GET Console200／revision2后，只点击一次原组件生成按钮。workflow=`49b57e32-e67c-464c-aa40-75428fe37e3c`，request=`3171f4a716f8a575941785b6b5048ef6`，实际POST200，完整NDJSON为stage／draft／complete，两个真实能力及inert预览合法。服务器对应两轮`91c96088-f2d0-4fe9-9e1a-a715b3109b65`、`7471093b-af7b-4c87-99dd-fb1795d62bd1`均200，精确工作流无错误；新bundle运行下完成原默认查询＋submit_layout链，不以本地fixture代签。

同页只点击一次“应用这个界面”，真实PUT `/console` expectedRevision2返回200、aggregate revision3／layout.revision2，未固定加1或加2。正常导航设备页后离页，再重入新GET200，logicalDeviceId、整体版本、完整layout及uiConfig与保存响应一致，显示两个绑定组件；仅此一条Console配置写，没有负向CAS／重试或设备命令。现场该逻辑身份仍未由已恢复的COM11承载，画面有设备不可达提示；**本批只签页面生成／保存／读取，不签新鲜设备State或控制ACK**。

### 63.3 HTML模式：唯一尝试被前置输入合同拒绝

组件完整通过并回首页／回收后，显式进入原自定义HTML入口，新GET Console200／revision3。只提交一次只读概览需求，workflow=`3e0f0401-6cca-4cfc-bea8-130591c3d8e5`，request=`94c23cd0c4af2e0ed9086e18d30f7ef3`；HTTP400正文是Gateway错误envelope `errorCode=INTERFACE_AI_INVALID_REQUEST`，不是NDJSON中的INVALID_RESPONSE。原页面无新preview、Apply禁用；HTML采用／PUT0，没有重发。

仅对已捕获原request做离线同schema核对，原body23,704 B／catalog21,355 B、23个atoms、两个fields，非本批大小上限问题。两条错误均为`unrecognized_keys`，路径fields[0]和fields[1]，键均是**manifestKind、manifestType**。App `custom-panel-editor.component.ts`的fields()只去掉value后展开整个DeviceUiEndpoint，连名称别名用的两项Manifest元数据一并带出；Agent原custom field为严格白名单且没有这两项，因此输入在模型输出处理之前即不符合同。只在内存副本去掉这两项，原严格schema即通过；没有修改原请求、源码或服务，也没有发送该副本。

服务器一次只读完整日志核对，HTML同workflow无model round／warning，组件两轮如上；现有日志没有逐请求入口访问事件，故不声称单靠“无日志”逐跳证明到达情况。400正文、实际请求、匹配部署schema及原前置校验代码共同支持上述输入拒绝结论；**不能签HTML模型生成、预览、保存或重入通过，也不再猜改HTML输出提示**。

### 63.4 收尾与下一最小范围

两个模式各一条原API请求，组件采用一次／HTML未采用。App已正常回首页，原非session授权及完整journals逐字保持，自有Network／CDP／forward释放，常亮仍原值0；除组件Console配置写外无其它意外HTTP写，MQTT设备命令0。Agent／Gateway及bundle收尾稳定、restart0；服务器SSH关闭，原账号、TaskStore、UNKNOWN和完整日志保留，暂不提交推送。

证据：Service `build/qa63-agent-release-20261102/`的公开制品及私有部署报告SHA`77b722cc…913e74`、双工作流收尾报告SHA`94a61fc0…c0b92c`；App `build/qa63-ai-dual-live-20261102/private-A8onGb/`的原请求、NDJSON／400正文、两模式截图、保存及重入、账号／journal保护、日志，以及custom-input-classification／detail离线分类。首分类仅按error取码得到null的记录保留，后续detail按实际Gateway errorCode取到稳定码，未覆盖旧证据。私有内容不进入Git。

可关闭：**匹配Agent发布、组件AI生成→同页采用／保存→新GET重入读取**。待修：**HTML请求的能力字段投影**。下一批仅在App现fields()边界显式投影原Agent所需的真实能力字段，排除value及本地展示元数据，保留真实id／key／权限；不把Agent字段改为passthrough、不删除严格校验、不改已过组件输出／CAS。补实际模板入参回归后，只安排HTML差异验证，不重复组件生成。本批遵照失败即止，未实施该修正或再调用模型。

## 64. HTML请求字段显式投影与有限回归（2026-11-02）

### 64.1 最小范围与实现

沿§63已捕获请求的确定性输入缺陷，只修App HTML生成出口，不改Agent模型输出、组件链或Console CAS。新增纯函数`custom-panel-generation-fields.ts`，原`fields()`仅薄委托；能力指纹、POST及迟到结果复核继续共用同一投影，不新增生成／保存流程。

显式传递原严格合同的15项：id、key、label、role、valueType、readable、writable、notifies及minimum、maximum、step、maxLength、unit、choices、telemetryMinimumIntervalMs。可选项仅在不为undefined时纳入，保留0／false／空字符串／空数组，不猜测权限或修正非法能力；choices浅复制。原按key去重的首次键顺序／末卡覆盖和title或key回退保持。value、manifestKind／manifestType及未来内部属性不再被展开外传。

owner、账号／设备epoch、独立Manifest指纹及取消／后台生命周期未改；同owner的内部展示属性或读数变化不再误取消，真实权限／约束或原Manifest变化仍退休旧请求。原严格schema和8份Agent生成源、App组件模式及保存／CAS消费者均保持原字节。

### 64.2 有限验证

先在原组件模板＋HttpTestingController新增5项并运行旧实现：**2失败／3通过，另10项未选中**，分别真实捕获额外字段外传与内部metadata误取消，不把它称为真实模型重现。最小修正后仅运行该文件，**15／15首轮通过**，覆盖精确POST字段、全部可选项零／空／假值、未来字段哨兵过滤、原去重，以及权限／约束变化、账号／设备／Manifest隔离、取消、失败和后台返回无自动生成。

另以合成数据离线导入投影与未修改的实际Agent `customWorkflowRequest`：完整15键精确匹配，序列化后的body通过；重新塞回两项Manifest元数据、value或未来属性四种情况仍严格拒绝，非法权限类型仍拒绝。此核对不发网络、不用真实私有payload，不放宽服务器校验。沿现有esbuild0.28.2，不安装／升级依赖；原SDK App `ngc`严格模板／类型检查noEmit通过，非发行包或APK构建。

完整意图、修前失败、修后日志、离线合同及源码冻结结果在App忽略目录`build/qa64-html-field-projection-20261102/`。队列期间源码、HEAD及index不变；修正差异仅HTML编辑器薄调用、新投影及上述5项测试，原其它App dirty、组件／CAS和锁保持。独立只读审阅无必要追加修改。

### 64.3 收口与唯一下一步

**HTML请求字段投影的软件修正已完成，真实HTML生成／采用尚未复验。** 本批未安装／部署、操作手机／设备、发送验证码或模型请求；现装beta仍`b2468e65…37ff257`，云端Agent／Gateway沿§63，没有重新构建服务、配网、Start、重放UNKNOWN、提交或推送。原账号、授权、TaskStore、scope、UNKNOWN及完整日志未操作。

下一最小批仅构建并安装包含此修正的匹配内测App，复用现有Agent／Gateway，做**HTML模式一次**正常生成；成功才同页采用／保存并重入新GET，失败读本次安全分类即止。组件生成及已过CAS不重复，未经新批次安排不自动调用模型。§63原400及更早输出失败保留，不由离线通过倒签真实验收。

## 65. 匹配App安装与HTML生成采用闭环有限收口（2026-11-02）

### 65.1 匹配候选与执行范围

按本批明确授权，只构建／安装§64字段投影修正的内测App，沿现有Development账号、设备、Agent及Gateway，不重跑组件生成或CAS。复用原build／verify脚本，一条队列完成prebuild、production＋cloud-beta Web、官方sync及离线单worker `:app:assembleBeta`。沿原SDK2.4.4／esbuild0.28.2／锁及签名；包名`iot.diandeng.tech.beta`、HTTPS、原生注册表、1158份打包资源与新15字段投影匹配。APK77,894,008 B，SHA`846a6126fa3027b4590c7e520b99bdeec35e518ccbbe6e2db09cd6673056b5e7`。

首次附加AST审计未识别编译器实际展开形态而停在包核对，原构建和原verify均已成功；只修该只读审计并续完未过项，不重复构建／验证。源码、native生成文件、HEAD及index保持原字节，无需恢复EOL。沿§63部署／收口、public bundle及14个本地运行源精确核对Service匹配关系，**本批未SSH、部署服务或取得新鲜进程／健康／轮次日志**，不把旧health与restart冒充本批新证据。

### 65.2 保全、安装与新鲜前置

先完整保留7项secure、两份journal及原logcat，同Development账号有效，不登录或发码。仅一次`adb install -r`成功，安装后调试端口首次尚未就绪，保留`fetch failed`停点；随后仅只读协调已完成的安装，核新APK、同账号与原记录，不二次安装。临时常亮原值保留待收尾恢复。

复用原DOM与HTTP观察器，以新独立批次进入原HTML入口。新GET Console200／aggregate revision3，页面就绪才放行唯一生成；不复用旧preflight／revision，也不复制旧组件通过结果冒充本批执行。真实POST两个字段只有原允许键，power及uptimeSeconds的id、权限原样，value／manifestKind／manifestType及额外属性均未外传；没有截改实际请求。

### 65.3 一次HTML生成、采用及读取

workflow=`1cdc6b12-5270-4454-9713-df9257cdd1dd`，request=`9f7601af492078b1be81156dc6625b09`。原生成按钮仅一次，实际POST200，HTTP完成约50,645ms；完整NDJSON是`stage → complete(mode=custom)`，无error，HTML585 B／SHA`b9cc99f9d37d668559601cc3a86a02e636b33f55143870ebcd049f8fd3853302`。HTML模式不要求组件模式的draft事件；实际结果、inert预览及原只读采用保护均通过，没有修改模型产物或放宽校验。

同页仅采用一次，原Console PUT携带新鲜expectedRevision3，返回200／aggregate revision4。仅更新customPanel和panelMode，HTML／theme与本次complete逐字对应，原普通组件layout及revision2完整保持。正常进入设备页、离页到首页后再进入，新GET200的logicalDeviceId、aggregate、完整layout／uiConfig及HTML与保存响应一致；两只读原子为blinker-status和blinker-uptime，没有点击设备控制或补造实时读数。

### 65.4 收尾、边界与后续

实际workflow POST1、Console PUT1、其它意外写0、MQTT设备命令0且解码完整；该Console配置写不称为设备控制写。7项secure及两份完整journal前后逐字一致。App正常回首页，自有Network／CDP／forward释放，常亮恢复原值0；保留匹配App和已确认的HTML配置。COM11、固件、NVS、TaskStore、scope、授权及UNKNOWN未操作，未发验证码、重放、升级SDK、提交或推送。

本批关闭**HTML请求投影的匹配安装与真实生成→采用／保存→新GET读取**；组件仍取§63，至此两模式最小采用链均有限通过。现测试逻辑身份仍显示设备不可达，因此不签新控制ACK／新鲜State，也不为补这些证据重刷或重配。截图中卡片纵向偏扁在本批仅登记为观感待核，不能据此认定产品缺陷；**后续§66已完成离线核对，用户确认手机正常，没有确认尺寸缺陷，不再排产品渲染修复／重复采样**。本批链路通过仍不外推为所有生成布局视觉通过。

证据仅在App忽略目录`build/qa65-html-projection-live-20261102/`：candidate-WzsdID保留单次构建、原审计停点与续验；private-bZudAJ保留账号／journal、完整原请求／响应、前后logcat、生成／采用／重入截图及acceptance-summary。Service `build/qa65-html-release-20261102/private/runtime-reuse.json`仅是沿用证明，明确noFreshRemoteInspection，不当作新远端日志。私有证据不进入Git；独立只读核验与上述指标一致，无额外模型／设备采样。

## 66. HTML截图观感离线核对与下一批建议（2026-11-02）

### 66.1 尺寸证据与结论

用户明确反馈手机实际显示正确。本批仅读取§65已保存HTML、原PNG及当前匹配源码，不连接手机／服务，不重新生成或重采。

- 585 B HTML与生成complete、保存及重入三处精确一致；没有style／高度／rows／span或transform覆盖，只使用gpio-status及device-uptime库预置，分别4×1与4×2。前者本应比后者矮，不能把不同预置高度当成压缩。
- `src/custom-panel/main.ts`的geometry只落共享网格参数，`_panel-grid.scss`按同一容器宽度单位计算行高，窄屏同步换算；frame的100%高度限定iframe视口。审阅路径没有纵向scale接线，不改原网格／字体／预置来迎合一张截图。
- 四份原PNG均1080×2376；原采集直接将Page.captureScreenshot的base64写为PNG，没有另做拉伸、裁剪或重编码。但原截图门禁只核父页可见及iframe存在，未记录runtime／字体／ResizeObserver稳定后的逐帧几何。因此截图可能早于稳定渲染，**这只是证据限制，不是已证明的截图时机或查看器缩放根因**。

决定为**NO_CONFIRMED_PRODUCT_DEFECT：未确认产品缺陷，用户实机显示正常**。撤出“待修尺寸”队列，不写成“已修复压缩”，也不声称离线还原了手机全部布局。本轮不改产品代码、不新增或重跑框架回归；以后有必要截图时才在采集工具中核稳定几何，不为补日志重采本批。UI／UX技能仅用于比例与证据检查，不引入新设计或替换组件。

离线核对记录在App忽略目录`build/qa66-html-visual-offline-20261102/inspection-result.json`；原HTML、PNG、账号、授权、TaskStore、scope、UNKNOWN及完整日志均未改。没有构建／安装、模型／验证码请求、SDK升级、部署或提交推送。

### 66.2 下一批最小建议：子页面冷启动登录保持

优先处理原报告#40尚未覆盖的指定冷启动差异，**不是再验配网的冷owner上下文，也不重新登录取Key**。目标是已有有效账号直接进入一个只读子页时，不因账号／库存恢复尚未完成误跳登录，同时真正失效会话仍正确阻断。

当前实际入口为`AppComponent.initService → await DataService.init → checkLoginStatus`及AuthService／页面数据恢复；`AuthGuard`在现路由中未作为该入口生效，不能凭旧类名猜测修复或新增另一套鉴权。下一批先用原入口、延迟安全存储／账号恢复和一个子页路由的离线替身核顺序，区分“恢复中、有效、真正失效、网络失败”；不把网络失败直接当未登录，不放宽服务鉴权或DeviceKey的近期认证合同。

确认真实缺口才最小修正并补有限回归；不能复现则记录未复现，不制造Bug。软件完成且仍有必要时，另定一次匹配App／现有会话的只读深链短验；本节只建议范围，**尚未实施，也未授权本轮转入手机测试、登录或部署**。

### 66.3 后备方向，不自动串行启动

1. **消息中心的最小产品闭环**：后续明确选择后，先核权威消息源、权限和当前profile，再做列表／详情／空态／读取失败手动恢复。当前cloud-beta的messages／notifications仍关闭，不因规划擅自开服务或测试推送；息屏、进程退出和iOS属于后续独立范围。
2. **分享的自然条件差异**：已修并验证的收到数量0口径不重测；只在自然存在非零分享、到期或撤权场景时补显示／权限恢复，不新邀请造样本、不复制Admin、不重跑已过双账号控制闭环。若无条件，保持待验。

设备接入、固定四槽、最小自动化、两模式AI采用及CAS已有有限结论冻结；同事整页编辑／特殊平台适配继续分工。Gateway前置await取消窗仍是§49.2未复现候选，不自动借本轮重开模型审计。Wio、Error8、8266、OTA／Bulk、复杂规则及发行CI保持原暂缓／后置。后续一次只选择一项明确范围，不再把历史所有“下一步”并成队列。

## 67. 软件交接与设备接入主线重新分线（2026-11-02）

按用户决定，§66的软件优化整理为[App核心软件优化交接计划](../../blinker-app/docs/APP_CORE_SOFTWARE_OPTIMIZATION_HANDOFF_PLAN.md)，供另一会话独立启动S1；S2／S3不自动串行。它们主要属于App／服务软件，并非全部只改App。文档已明确实际启动链、消息关闭profile、详情已读POST不能冒充只读、自然样本和公共文件协调；三项均NOT_STARTED，未由本会话实施。

本会话接[同板接入身份、发现与附近状态计划](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md)。DA-0仅本地核三端代码及既有结果：WiFiProv BLE／SoftAP与纯BLE／日常Direct不同，BLE→WiFi、保根进入Direct及实例resolve已有机制；普通CREATE不接instance，BLE issue无active enrollment时可选新ID，**manual WiFi-only→首次BLE**是下一优先离线复现候选，不签已发生的真机缺陷。不能凭MAC自动合并旧记录、移任务或把多个旧scope拼接。

附近链已包含授权presence匹配、约15秒TTL、可见需求下至少10秒节拍／2.5秒扫描及原owner隔离；首页Cloud／附近／已认证控制也已分开。当前匹配cloud-beta源码directAccess=false，故必须先明确profile，再做正向附近判断。DA-2继续核过期／账号／授权／后台等差异，DA-3只选未覆盖的扫描交接，DA-4软件与现场前置成立后才做一次单板差异；不重排已过完整配网、BLE长循环、任务／自动化或模型测试。

本批只新增两份执行文档、更新当前入口及旧接入决策的覆盖提示；未改产品代码、运行回归、连接手机／设备／服务器、部署、提交或推送。原账号、授权、TaskStore、scope、UNKNOWN及完整日志保持，无关dirty保留。旧文档中的“SoftAP首次接入仍未完成”已标为被统一入网§26／30覆盖，不作为新队列。两个会话涉及Data／User服务、路由、API或模型等公共文件时先协调，不并发改写。

## 68. DA-1：已知Cloud关联的BLE首装防重复软件收口（2026-11-02）

按[设备接入计划§6](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#6-da-1执行结果已知cloud关联的新ble首装防重复2026-11-02)完成首项离线实施。真实BLE repository＋原合成receipt复现manual Cloud先接入、无BLE enrollment时，旧issue→commit将同instance库存从1条变2条；修前红测保留，不冒称新真机缺陷。

最小决定是不放宽Cloud diy与BLE类型／授权边界：仅新首装查既有持久Cloud关联，命中active Cloud设备返回409 `DEVICE_V2_BLE_CLOUD_IDENTITY_CONFLICT`，不泄漏ID／账号，阻止此次新签发与第二记录。四种已完成Key证明的session状态均适用，挑战未证明不算；原BLE恢复、已签intent／UNKNOWN重读和commit保持。不是自动复用／合并旧记录，也不承诺Cloud与BLE并发全局唯一。

App仅BLE guide补精确409＋code提示，保留返回、隐藏该原因重试，普通错误／RESET不变；离页、账号和显式新操作清理，11语种专用单键。未改公共鉴权／路由或另一软件会话范围，UI／UX指引只用于说明原因及安全恢复，不重设计。

新服务回归12／12、旧BLE signer／intent相邻CLI及App24／24通过，原依赖`ngc --noEmit`通过；App修前5红4绿与服务1→2红测均保留。真实PostgreSQL明确跳过，实际事务／查询计划及是否需instance前导索引是部署前有限欠证，不能用fake DB代签。完整离线意图／日志／结果在Service忽略目录`build/da1-cloud-ble-continuity-20261102/`；源码与锁在测试期间保持，未升级依赖或SDK。

本批未连接手机、设备、数据库或服务器，未部署／刷板／发码／调用模型／提交推送；原账号、授权、TaskStore、scope、UNKNOWN、私有日志及无关dirty保持。下一最小批进入DA-2的附近凭据／生命周期离线差异，先核profile，不调扫描、不强开Direct、不重跑配网／任务／自动化。软件防重复已完成，匹配部署及真机结果尚未签收。

## 69. DA-2：附近证据生命周期与本地凭据更新的软件收口（2026-11-02）

完整范围及证据见[设备接入计划§7](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#7-da-2附近证据过期后台及凭据切换的软件收口2026-11-02)。离线原Service／实际Store复现后台旧nearby、HMAC完成时刻错误续TTL，以及同账号remove／pending／epoch／presence rotation后旧观察未退休；原port也复现bleAccess一次读取永久沿用。账号事件及scope隔离原机制仍有效，不从零重写。

最小修正落在三个原模块：Store成功写入后发仅scope＋ID的无重放进程内通知；BLE Service只使相关附近证据及旧batch失效，后台同步清理／拒入，广播回调时刻决定15秒截止；DeviceUiPort收到本地变更先unknown再重新读本地凭据，以read序号挡旧结果。原扫描节拍／配额、原生关闭未知围栏、密钥排空、实际通道／认证及控制链保持，不因附近失效自动断有效认证会话或发起连接／控制。通知不含秘密、不新建身份系统／后台轮询，销毁／退订回收完整。

原port3红1绿、Service8红2绿后，8文件116／116通过。审阅再发现异步写后读取可变caller ID，改为封存stored ID；定向新增1项修前失败、修后1／1通过，未重跑前10项。最终源码原依赖ngc严格模板／类型检查通过；116项在这一个ID表达式修正前通过，分阶段证据各自保留。首轮Service hoisted mock helper报错未注册，仅等价修夹具后续完该文件，不把工具错误当产品缺陷。

App忽略目录`build/da2-nearby-lifecycle-20261102/`保留意图、全部红／绿／夹具停点、单项归属及aot日志。未部署／安装／刷板、操作账号／授权／TaskStore／scope／UNKNOWN、连接数据库／服务、发验证码或升级SDK；不提交推送，无关dirty保留。UI／UX指引只用于真实状态反馈，未改布局或添加额外提示。

这是有限软件收口，不签当前cloud-beta Direct正向广播（仍默认关闭），也不承诺离线感知远程撤权。下一批按DA-3只选一个缺失的扫描owner交接离线反例，已有配网／控制／任务／自动化不重排；DA-1隔离PG及后续匹配单板短验仍按各自前置执行，不并成新全矩阵。

## 70. DA-3：WiFiProv扫描启动到选择连接的窄交接软件收口（2026-11-02）

按[设备接入计划§8](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#8-da-3wifiprov原扫描启动到选择连接的窄交接2026-11-02)，只补一个原WiFiProv内部scan→select组合：结果已到但start的JS Promise仍pending，原stop未排空start、旧start可再挂12秒计时器，重复stop还会各发原生调用。4项修前全部失败，未将此离线JS时序缺陷冒称历史S3原生GATT根因。

仅改App原page的扫描薄接线：共用停止Promise并纳入原busy，捕获并排空原start后stop确认才选择；start续体核Promise身份／世代／阶段，无旧timer；离页等待pending stop再原close／clear／release，失败保隔离。不改插件、中央扫描器、Direct transport／GATT权限、身份或SDK，不引入固定等待／ready解围栏或重发。UI／UX指引仅用于异步操作期间复用原disabled行为，未改布局。

新4项及原discovery6／lifecycle13／ScanCoordinator13共36／36通过，最终原依赖严格模板／类型编译通过。只验证原select→connectSession边界，connectSession替身截断，不当真实连接／身份／WiFi／云上线／ACK。App忽略目录`build/da3-scan-owner-handoff-20261102/`保存red／green／aot意图、原日志及结果，源码和锁在队列期间保持；原页dirty及DA-1／2源码保留。

本批未安装／部署／刷板、连接手机／设备／服务器、发验证码、操作账号／授权／TaskStore／scope／UNKNOWN、升级依赖或提交推送。下一最小批DA-4先只读核实际版本、串口、配网bearer与原授权，匹配后单独安排扫描→停止／选择→取消短差异，不下发WiFi、不默认解绑／清根或刷板。正向附近另需Direct-v3／presence及对应profile，不能由WiFiProv代签；DA-1隔离PG欠证保持，不再扩成全矩阵。

## 71. DA-4：现场只读核对与匹配条件未满足（2026-11-02）

详见[设备接入计划§9](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#9-da-4现场只读前置核对与匹配门禁2026-11-02)。ADB手机在线，当前用户0扫描／连接／精确位置权限已授、蓝牙开启，base APK手机原位SHA精确为§65 `846a6126…056b5e7`。旧APK公开ZIP／匹配制品证明WiFiProv注册及原SDK存在，但配网页无DA-3停止屏障，不能作为修正包验收。一次只读WebView资源入口超时，没有取得活跃Web OTA哈希；不当产品缺陷或擅自断言运行网页版本，自有forward已移除且列表为空。

USB仅COM11／Espressif VID303A、PID1001在线，未开串口／复位／读Flash或NVS／连配网设备，因此当前bearer、instance及Admin未新鲜核实。历史§55恢复BIN对应SoftAP四槽，不是BLE WiFiProv，也不是当前固件证明；Development临时身份及编译夹具不能替代现场原身份。未为补样本刷板、清根、解绑或重配。

DA-3最终编译冻结的必要源、HTML、扫描器、Direct transport、DA-2服务／凭据、profile和锁逐项仍匹配；没有新源缺陷，不重复回归／构建。忽略目录App `build/da4-access-preflight-20261102/`保存live-inspection超时原结果及preflight-summary。未读取secure／journal或凭据，账号、授权、TaskStore、scope、UNKNOWN及完整请求日志未操作；没有安装／服务器／数据库／发码／Git写入。

本批完成的是**只读前置，不是真机短验**。下一最小批先另行安排一次当前源匹配的cloud-beta构建／同签名保留数据安装，COM11保持；具备实际bearer和原授权时才测扫描→停止／选择→取消，不发WiFi、不重跑完整接入。若仍SoftAP，则BLE短验保留欠证，固件变更需另明确必要范围／NVS授权合同，不自动扩大。Direct正向附近及DA-1隔离PG仍各守原前置。

## 72. DA-4：匹配App安装、运行入口及数据保留确认（2026-11-03）

按[设备接入计划§10](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#10-da-4匹配app单次构建保留数据安装及运行入口确认2026-11-03)完成§71所列的唯一App队列。冻结14项必要源、11语种、原插件／SDK／锁后，复用原prebuild、cloud-beta Web、sync及offline单worker assembleBeta，各一次通过；不重复DA-1～3已过回归／AOT，源码、HEAD及索引全程保持，无关修改保留。

新beta APK为77,894,008 B、SHA256 `0cd3c16e6e702e8facb6f2afedfa4dd8781a5e50e1d7bf41ca771c2cae461564`，原证书、WiFiProv2.4.4、HTTPS／注册表／DEX及1,158项Web资源逐字核验通过。配网页`chunk-CtGHB29k2.js`已含DA-3停止屏障；Direct仍false，未改普通profile。安装前保护原数据并再次核旧APK／账号／日志，**仅一次保留数据`install -r`**，手机新APK哈希精确匹配。

安装后初次CDP入口未就绪、随后Page资源清单缺模块main的两个工具停点均保留；没有重装／重建，也不当产品缺陷或旧OTA结论。仅补一个独立只读VM入口：当前顶层默认上下文已加载的`main-E5UZLH2E.js`，原始UTF-8 SHA `cbc5d901…b8fb7a4`与不可变APK精确一致。无fetch／导航／reload，配网页行为没有实际执行，故**只签运行主入口匹配，不签DA-3无线交接**。

最终同账号／区域、7项安全存储逐字保持，2份原IndexedDB store完整保持；本批原0行、结束0行，不造请求样本。各阶段完整logcat私有保留。原常亮0已恢复，自有CDP／Debugger及forward释放，根现场终核列表为空。证据在App忽略目录`build/da4-matched-app-20261103/`的candidate-cijVzS、phone-private-VKF7xF及installation-summary；build-only报告不改写为安装证明，独立核验通过。

COM11零操作，原NVS／TaskStore／scope／授权／UNKNOWN未动；未部署服务、调用模型、发码、升级SDK、提交或推送。**下一仅复用新App只读核COM11实际bearer／instance及原Admin，条件齐备才测DA-3扫描→停止／选择→取消**；若仍SoftAP则先明确匹配候选与原授权合同，不默认换固件或重配。DA-1隔离PG／Gateway发布及DA-2正向附近仍各守自己的前置，不重排已过的配网、任务、自动化或AI矩阵。

## 73. DA-4：账号／通道前置、有界发现及Admin边界纠偏（2026-11-03）

详见[设备接入计划§11](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#11-da-4当前账号通道前置有界发现及前置条件纠偏2026-11-03)。复用§72已安装App，手机原位APK仍精确匹配、DOM主入口未变，未重建或安装。当前Development，7项安全存储中整个BLE credential前缀记录0项；不推断板上根丢失、App存储损坏或需要重认领。原插件getCapabilities支持两种配网；cached getDeviceInfo正常无选择，手机互联网快照只读，不作隔离解除或Cloud认证证明。USB仅COM11在线，未开串口／ROM、复位或读写NVS；实际bearer／instance／epoch仍无新鲜GetInfo证明。

**修正文档中的前置混用：Admin限制原身份连续性及身份／WiFi写，不是DA-3原扫描／停止／选择／取消的必要条件。** 原page这些方法只走权限、原lease、WiFiProv扫描／Security／PoP及目标WiFi只读扫描；provision／installBlinkerIdentity才进身份写链。Direct=false也不关闭独立WiFiProv工具链。不再为纯扫描反复登录、查Key、建Admin或改账号权益。

因此仅作一次正常工具页发现，蓝牙及BLE所需权限原已满足，未授权／切网络／改前缀。事件started→completed→stopped：完成事件观测约10.044秒、原页面stopped约12.039秒，均为同一App接收时间差而非物理射频测量；设备列表及结果事件均0，随后正常离页。没有选择、GATT连接、GetInfo、身份或WiFi写，未人为延迟原桥接。**只签一次有界发现、停止及离页，不从未命中推断SoftAP、硬件故障或配网失败，也不签pending-start竞态或选择链通过。** 没有确认新产品缺陷，不为产出改扫描器或执行链。

最终7secure／两份完整IndexedDB store逐字保持，原0行／终0行；仅落盘指纹与非秘密摘要，不新建重复账号秘密备份，阶段完整logcat私有保留，自有listener／CDP已移除，根终核forward空。原授权、TaskStore、scope、CAS及UNKNOWN未动。未构建／安装、刷板、发送验证码、升级SDK、部署或提交推送；无关本地修改保留。证据在App忽略目录`build/da4-bearer-admin-20261103/phone-private-4Ig42V/`，独立核对通过。

**下一最小批只准备一份COM11匹配WiFiProv BLE运行候选及切换合同，不再扫描碰运气。** 明确原SDK／四槽Manifest、公钥／profile、Security／可信PoP及配网窗口，保留原NVS的instance、Key、根、TaskStore和scope；不把编译夹具刷板、不清根或把SoftAP当BLE，不为扫描造Admin。真正刷写前只保护受影响NVS，候选及兼容条件齐备再一次扫描→选择→取消，不下发WiFi或扩大任务／自动化测试。DA-1隔离PG和DA-2正向附近仍保留，不同时开第二队列。

## 74. DA-4：BLE四槽运行候选完整链接与必要Direct功能启用（2026-11-03）

完整实现、恢复合同及身份方向矩阵见[设备接入计划§12](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#12-da-4ble四槽运行候选非破坏开窗及必要功能启用2026-11-03)。本批仅离线源码／回归／构建，不操作手机、COM11、服务器、账号、授权、TaskStore、scope、CAS、UNKNOWN或原完整日志；不扫描、不刷板、不发码、不升级SDK、不提交推送，无关dirty保持。

原profile新增显式内部BLE四槽分支，普通0槽及原SoftAP默认不变。原产品有Key＋WiFi时不会自然开配网窗口，因此新增1字节RAM启动意图，在首次attach前显式请求、原start检查后消费一次，沿原WiFiProv／TaskRootOwnerGate；不清网络或根、不造实例。候选每次boot请求原600,000 ms窗口，维护期间taskReady=false；取消不代表板端停止，到期仍原Fault／guarded停机，不新增自动恢复／重试。PoP沿原私有非空Security1材料，不公开秘密，不新增串口命令或Facade API。

有限验证分阶段保留：启动意图1个CTest目标／8行为通过；原profile8项通过；首次真实Facade拒绝后仅补显式BLE admission并新增19子例通过；续编因旧f878 Root配置顺序再拒绝后，复用当前已存在Root修正、新增真实入口BLE／SoftAP两子例通过。**两次失败和两次显式续编均同候选、build目录／缓存，无clean或另开重复队列。** 最终原Core3.3.11／CLI1.5.1、XIAO ESP32S3 default_8MB、jobs1完整链接通过，原BLE scheme、四槽、持久确认、双根及唯一产品实例1消费者核查通过；不是新鲜wire Manifest或真机证据。

原始应用BIN为1,413,296 B，SHA256 `5f9a9637a506f4ab4eff18bf4b27c9a93250b3c5910cf5792c17349dd5734990`；编译器Flash1,413,144 B、静态RAM60,492 B，0x10000／3,342,336 B门限不变。签发输入仍P256 keyId1、公钥SPKI SHA `02879e20…b6d061`，runtimeSigningMatched／formalAdmission／deployable均false；不拿编译通过开放服务器准入。完整链与独立只读核验在Library忽略目录`build/da4-ble-runtime-20261103/`，私有PoP未输出。

用户明确Direct／附近为必要功能后，**cloud-beta源已开启directAccess=true**，其它开关、原权限／active凭据／presence／Manifest／State门禁与扫描频率保持。配置红测1失败／4通过，修后5／5通过，证据在App `build/da4-direct-profile-20261103/`；本批未构建安装，现装§72旧包仍false，不能宣称现场已启用。DA-2软件缺陷已修，但真实授权附近仍需匹配App与自然Direct-v3广播／presence；WiFiProv发现不能代签它。

下一最小批仅匹配App构建及同签名保留数据安装，不重编BLE候选、不默认刷板。后续刷写须新鲜核实际分区／原NVS与任务，保护instance／Key／根／TaskStore，仅写原始app；默认IDE／CLI upload还会覆盖分区等，禁止直接套用。指定f878仅为已验恢复应用，不是当前程序新鲜备份；恢复保留最新NVS，不能回滚后续任务消费／网络。DA-1隔离PG／匹配Gateway发布、正向附近及板端选择连接分别待条件，不展开全矩阵。

## 75. DA-4：Direct启用匹配App安装与实际运行核验（2026-11-03）

完整范围与制品取[设备接入计划§13](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#13-da-4direct启用的匹配app安装与运行核验2026-11-03)。相对§72的2210项App输入只有cloud-beta配置／对应spec两处差异；§74配置5项绿证据与当前精确匹配，不重测。唯一原prebuild／Web／sync／offline单worker assembleBeta全部通过，原源码、外部WiFiProv实际消费者、SDK2.4.4／esbuild0.28.2、HEAD／索引保持，无升级或重复队列。

唯一新beta为77,894,008 B，SHA256 `a9e3e4eb10ee16e5be2532c6c21f3e998522ddf4c5443faad068eb5bc7cac7c1`；同证书`a76639ec…5d5d210`，HTTPS／注册表／1158项APK资源逐字核验通过。实际main静态import图及严格AST核directAccess／automation／tasks=true、其它flags保持false，原host及LAN／bootstrap配置不变；原BluetoothLe注册与两扫描DEX核对通过，未改执行／扫描链。

安装前保全同账号／Development的7secure及两份完整journal，前置再核旧APK／原数据后，**仅一次install-r返回Success**。新进程初次CDP fetch failed的原工具停点保留，不当产品失败、不重装／重建；随后一次显式只读协调成功。既有top-default VM的main-GENRCY5Y.js／SHA`4a7ed159…fe4ef5`与配置chunk-BYAGOBAl.js／SHA`5836f149…fd440d`均精确匹配不可变APK；只读isPluginAvailable确认BluetoothLe／BlinkerBleScan均true，不initialize、扫描或连接。配置不在main本体，两个模块分别核，不用源码true代签手机。

终核7secure逐字一致，2完整store原0行／终0行，无新增自然行；完整阶段logcat及首停点ACL私有保留，不造样本／恢复token／清日志。原常亮恢复，监听／Debugger／Runtime／socket／自有forward回收，根终核forward空；App保留新版本。证据在App忽略目录`build/da4-direct-enabled-app-20261103/`的candidate-VpMtEc、phone-private-VN4nG1及installation-summary，build-only报告仍未改成已安装证明。

本批未操作COM11／NVS／授权／TaskStore／scope／CAS／UNKNOWN，未刷板、部署服务器、登录、发码、升级SDK或提交推送，无关dirty保持。正常启用后的App／presence生命周期未拦截，不把无主动扫描写成零射频；当前本地凭据0只作为存储事实，不签授权附近正向。**下一回到§74已编译BLE候选的板端兼容／原NVS合同和一次最短选择取消差异，不重编App／固件或重跑完整配网；候选刷写须按新批范围，仅app、不清根。** DA-1隔离PG／Gateway发布及真实授权附近分别待条件，不重排已过矩阵。

## 76. DA-4：COM11原应用／实际NVS及维护停驻只读核对（2026-11-03）

按[设备接入计划§14](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#14-da-4com11原程序实际存储及维护停驻的只读核对2026-11-03)完成唯一ROM只读流程。精确USB身份匹配，ESP32-S3／8 MiB／Secure Boot及Flash Encryption disabled，实际表4 KiB读验及MD5、范围／不重叠通过：NVS0x9000／20 KiB、factory0x10000／5 MiB、SPIFFS0x510000／2 MiB。仅保护20 KiB原NVS，读验SHA`aad61112…f4179e`；原应用ROM verify精确匹配f878／1,437,360 B，不做整片或当前主程序重复备份、不匹配不扩大读取。9项返回全0，最后只确认原程序hard-reset请求，不代签重新在线或任务业务恢复。

唯一离线投影复用原CRC／blob／任务元数据链：instance schema1与内CRC有效；AccessStore schema3／260 B／epoch1，Key及结构有效Admin1存在，无Shared。TaskStore BoundSnapshot／schema2／count1／revision18、绑定instance一致；唯一id1为10秒倒计时、disabled／Completed／startSequence1，没有enabled／CountdownArmed／Claimed记录，结构及静态停驻检查通过。scope与rootDigest仅hash，不搬迁或替换；不把CRC／结构通过升级为完整TaskCodec、真实Admin握手、当前服务scope／签发或正式准入。

**新鲜差异：设备presence key缺失、version0。** §75当前App全前缀本地BLE凭据0项是另一事实，功能开关已实际开启但不能代替两端合法材料；不按名称／MAC伪造附近或清根补绑。此缺口不阻塞WiFiProv的PoP扫描／选择差异，不能混为同一功能。

现有BLE候选1,413,296 B同时满足编译3,342,336 B与实factory5,242,880 B、offset0x10000一致，不改分区或放宽门限。新ignored薄适配删掉自动整app备份fallback，只有ROM只读allowlist；阶段日志／20 KiB及安全投影在Library `build/da4-com11-readonly-20261103/`，独立meta／SHA核验通过，无第二次串口读取或重跑测试。App、账号、授权、TaskStore、CAS、UNKNOWN及完整请求日志未操作，原SDK／无关dirty保持，未刷板、扫描、登录、发码、部署、提交推送。

原暂不刷板限制未解除，下一仅在明确允许COM11候选切换后，复用原App／已编BIN／非空PoP，只写原始app并补一次扫描→选择→目标WiFi只读扫描→取消；不写身份或WiFi。写前核最新NVS，变化先保护协调，不能旧快照回滚；结束只恢复f878应用、保留最新授权／任务／网络。真正授权附近及DA-1隔离PG／发布仍单列，不扩大矩阵。

## 77. DA-4：COM11单次BLE选择取消及原存储保持（2026-11-03）

按用户新授权完成[设备接入计划§15](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#15-da-4com11应用切换与一次ble选择取消短验2026-11-03)，本批覆盖§76的暂不刷板及恢复原程序约定：仅写app、结束保留5f9a，不清根、不恢复旧NVS。复用已编BIN和a9e3 App／WiFiProv2.4.4，不重复编译安装或发码，账号／授权／TaskStore／CAS／UNKNOWN及完整日志保护保持。

手机入口全备齐后，首新鲜NVS raw变化在零写入时触发停点；原始新旧件及失败事实保留。完整35项记录仅SDK的phy/cal_data变化，其它身份／接入根／网络／任务完全一致，停车条件保持。独立显式续批重核全key集合／类型／长度及非cal_data值，不放宽整个namespace；以最新raw54a45d…3089e作写后基线。**实际app只写1次**，0x10000的5f9a…734990／1,413,296 B、最新20 KiB NVS、原分区均verify通过才启动；11项return0，首批无write，未回滚校准、bootloader、分区或SPIFFS。

原工具页一条链：startBleScan1／唯一匹配结果1，原start和stop完成后才selectDevice1；connect1／Security1成功，原自动scanWifiNetworks1返回14项且UI14行，不再点刷新或SSID。header返回后stop／clearState回执完成、disconnected、pending0，getDeviceInfo明确No selected device。三个观察listener和桥接包装均恢复，原生命周期沿close确认后的lease释放路径；不通过第二owner扫描或ready／计时器代签关闭。

单App performance时钟观测：扫描调用→select约240.2 ms、connect1,193.3 ms、目标WiFi扫描6,326.8 ms、扫描调用→WiFi列表回执7,777.8 ms、取消原生stop／clear窗口20.9 ms。约6.3秒是目标网络扫描，不是控制／任务写入延迟；不为分布或罕见pending-start竞态重复采样。此只签常规连接取消差异，不是完整WiFi下发／云上线、Direct附近、Admin控制、任务或全owner／全部GATT问题关闭。

终核同Development账号、7secure、2完整journal与原WiFi保存项指纹保持，原journal0行／终0行，完整logcat私有保留；App在/home/tools，临时常亮及监听／CDP／forward已回收。原maskedPopCleared=false保留，独立只读DOM证明页面／字段已卸载，不能把不存在的字段误判为PoP持留；未重跑UI。Library／App忽略目录`build/da4-ble-select-live-20261103/`保留首停、显式续批、实际桥回执／截图／收尾及trial-summary，独立只读终核SHA链通过，无额外硬件／秘密读取。

按要求保留BLE维护候选与最新NVS，**没有为收尾再进ROM或复位读取射频后的NVS**，字节保持只签到启动前。手机取消不结束板上600,000 ms维护窗口；到期Fault／guarded停机合同未再次长等待测试，不称为普通常驻运行／正式准入。无新logical ID／scope、身份／custom／WiFi／任务写、业务重放、SDK升级、提交推送，无关dirty保持。DA-4常规短验有限收口，下一优先DA-1隔离数据库有限回归及匹配部署前核对，不自动发布或迁历史ID；授权附近另列。

## 当前主线指针（2026-11-03）

当前进度统一见[独立注册执行方案§14](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#14-ip-4-新身份-cloud-首装及保根注册差异2026-11-03)，以上历史结果不改。用户允许COM11覆盖、无需旧恢复；测试Auth本账号设备额度10留存。新Cloud首装、两卡独立ID／2→3→2计量、显式恢复、附近不串旧卡及仅删BLE保Cloud／根已有限通过。

f878任务owner内同步Presence写自拒已修正，新BIN e48cffa…28f07及授权保留。最新[§15源码审阅与静态对齐](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#15-必要源码审阅选择器与发布冻结对齐2026-11-03)已关闭三项freeze／选择器差异，架构及App有限95项通过；下一仅在明确交付时冻结匹配WiFiProv旁仓及同提交CI。首次提交前未分类错误、BLE→Cloud及删源writer边界保持，不重采已过矩阵、不自动推送。
