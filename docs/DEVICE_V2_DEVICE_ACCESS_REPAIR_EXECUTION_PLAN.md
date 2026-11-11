# Device V2：基础设备接入修复方案

更新：2026-10-29。范围取当前 `i3w-dev-v2` 源码与已安装内测版，不按报告中的历史状态直接销项。
本方案接续核心稳定工作；[NEXT-1／2](DEVICE_V2_CORE_REMAINING_EXECUTION_PLAN.md)的历史页恢复已有限收口，不重跑。

最新结果取[全流程入网方案§30](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#30-on-7ible非空pop空根首次接入及精确恢复有限收口2026-10-31)：COM11 BLE非空PoP／Security1空根首装原链真实通过，GetInfo自然新instance、独立Cloud/CAS/Admin、精确Bootstrap ACK、唯一WiFi官方成功和Cloud新会话／State确认后自动进入；结束精确保留新NVS／scope并恢复原c051＋73d949，ROM摘要及原分区匹配，按合同留bootloader。App原5项和新增2项secure／UNKNOWN／完整日志保持；204预检无正文已严格另存补证，旧false记录不改写，冻结代码未动／无重发。BLE与SoftAP最小成功链均收口，不再重跑；自然UNKNOWN正样本及其他矩阵不扩称完成，临时额度仍3带原到期，无SDK升级、部署或推送。

最新停点取[全流程入网方案§29](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#29-on-7h已授权单次发码唯一登录被拒后停在设备操作前2026-10-31)：本次正常发码200后唯一登录400／AUTH_VERIFICATION_FAILED，相隔9分14.67秒；与过期相符但未细分原因，未重试或自动再发。原账号／5项secure／UNKNOWN／journal及完整失败日志保留，冻结源码未改，任何COM11操作均未发生，不为收尾反复写恢复。当前仅待新的明确单次认证授权，BLE空根未验收；App／候选及原期限3台额度保持，不重复SoftAP、升级SDK或推送。

最新接续取[全流程入网方案§28](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#28-on-7g匹配app安装合法额度与ble首装入口冷预检收口2026-10-31)：新beta49e一次匹配构建安装，原账号／全部5项secure／UNKNOWN及完整journal保持，BLE共享PoP真实冷UI与登录控件／TTY均就绪；合法额度正常API限定到3且Gateway读回，原期限及他项保持。候选、七refs保护、COM11新fixture／恢复和只读收尾入口49项已冻，无串口／清根／刷写／认证／身份写。下一步需本次单次认证及明确限定fixture执行许可后连续BLE首装，不重复SoftAP、修改认证中工具、升级SDK或提交推送。

最新接续取[全流程入网方案§27.4～6](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#274-app最小pop修正及有限回归)：非空PoP BLE候选已原SDK单队列完整链接，固定分区可容纳；App共享原PoP字段并修BLE选择／连接漏传，24／24和类型核通过，原插件／generic身份链不变。尚未打包安装该App或开始BLE空根真机；下一批先匹配App及入口冷预检，确认合法创建额度／限定新fixture后才一次认证。COM11原73d949根及scope、新旧授权／UNKNOWN和完整日志保留，未操作手机／串口、清根、发码或创建身份，不重复SoftAP、升级SDK或提交推送。

最新结果取[全流程入网方案§26](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#26-on-7e冻结入口连续执行softap空根首次接入有限收口2026-10-31)：本次正常登录后，COM11无刷写连续完成真正空根GetInfo、新Cloud身份／CAS／Admin、新owner同instance、Bootstrap精确ACK、唯一WiFi成功及本次Cloud新会话／State确认，显式进入新设备。原账号／3项secure／授权／UNKNOWN／journal保持，仅新增本次2项授权；实际回收worker0，新NVS已只读精确保留且root已安装，COM11留ROM。显式手动SoftAP首装最小链有限收口，不再重复；临时额度未主动撤、BLE空根匹配候选和自然UNKNOWN样本单列，不冒称全矩阵关闭，无SDK／服务变更或提交推送。以下历史冻结。

最新接续取[全流程入网方案§25.7](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#257-一次认证前的执行入口已预检并冻结)：真实页面／登录控件和持久TTY均预检通过，15个运行文件已冻结并复核；单TTY认证后直接显式resume同一候选并补剩余主链，不再修脚本。未发新码／新登录、操作串口、reset／刷写或执行业务写；COM11仍留ROM，原NVS／授权／UNKNOWN／完整日志保持。完整首装仍未关闭，下一步仅本次单次发码授权及冻结热路径，旧授权不外推；以下历史冻结。

最新接续取[全流程入网方案§25.6](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#256-login-2真实通过局部连接扫描及安全收尾完整首装未完成)：本次登录／同账号新token／真实近期marker已明确通过，无刷写恢复及局部owner连接／扫描、清理也通过；因测试工具EOF／DOM挂载纠正耗时使Key窗口过期，不能继续归因Auth待定位或设备失败。GetInfo／Cloud创建／Bootstrap／WiFi仍0，完整首装未关闭。原NVS／授权／UNKNOWN保持，新鲜新NVS fa4e与旧9415整区不同但身份／空根／空任务同，已只读保留且板留ROM，不覆盖旧存储／开窗。下一批先发码前就绪工具，再连续跑剩余主链；新发码需另行允许，不再烧录或重复已过矩阵，未升级SDK／部署／推送。以下历史冻结。

最新接续取[全流程入网方案§25.5](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#255-新授权login-2一次发码已接受等待用户新码)：用户另行允许一次发码，当前内测包login-2真实经Gateway的原ALTCHA／email-code链200，challenge1／code1／login0，等待新码且不再发送。旧失败／manual／UNKNOWN／NVS保留；已备齐显式resume同一候选与原Page首装工具，不重刷、不提前开窗，登录及真实marker成立后才操作COM11，未升级SDK或推送。下段停点历史冻结。

最新接续取[全流程入网方案§25.4](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#254-再次登录报告后的实际核对与有界暂停)：再次手动登录报告后，内测实际session未变化、当前token真实marker缺失，不能签新近期认证证明；有限既有日志不能回溯归因，不猜用户错误、不继续循环要求登录。原10分钟已结束，未GETInfo／CREATE／Bootstrap／WiFi。只读保留新20KiB／完整新instance且COM11停bootloader，不静默reset，原NVS／103c及App授权／UNKNOWN保持。后续先在同一内测包取得可关联正常登录；如需新码须另行允许一次，再显式resume原候选，不重刷、不重发、不升级SDK或推送。以下停点为历史冻结。

最新接续取[全流程入网方案§24](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#24-on-7c授权的一次真实发码等待用户验证码2026-10-31)：用户已允许本次固定原邮箱单次验证码，真实App原ALTCHA／发码链200，challenge1／code1／login0，等待用户本次验证码，不再发送。没有退出账号／改NVS／创建身份／新构建或推送；收到后先正常同账号登录及新session／旧授权／UNKNOWN核对，再按§22.5启动一次首装。下段§23为发码前冻结事实，不再询问已批准权限。

当前接续取[全流程入网方案§23](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#23-on-7b同板首装准备限定额度及实际认证门槛2026-10-31)：用户批准COM11同板及临时2台，正式Auth原审计接口已写入并直接解析确认2／revision2、套餐他项不变，带24小时到期及验收后精确撤销合同；Gateway旧缓存1先保留失败记录，自然过缓存期后仅一次显式GET已确认2／revision2，不重发或清缓存。原20KiB NVS新鲜保护／ROM验证，103c及原根／Key／任务保持；App账号／3secure／UNKNOWN／完整日志终核不变。实际近期marker MISSING，且App无密码登录UI，不能沿旧“已有密码登录”假设、refresh或写标记绕门禁；本轮禁发码，先保持当前会话、不清NVS或创建新身份。现有App／固件复用，首装NOT_RUN，无新构建／部署／SDK升级或推送；下一步待正常同账号近期登录，若代发一次验证码需另行明确允许，旧§15～21不重跑。

最近无线事实：2026-10-31 OEM managed已有根完整入网已在[全流程入网方案§19](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#19-on-4joem-自动-softap-保根完整入网短验2026-10-31有限收口)有限通过：最小补managed network-only快路及原Cloud死亡屏障，原SDK不变；两不同owner同根交接后唯一新WiFi7528ms官方成功，切网首次Cloud不可用后只读显式确认本次session／State，再进入原设备一次。不是自动导航或全OEM矩阵；旧§20～22失败证据保留，不倒写单一根因。原应用已恢复，合法新WiFi保留，授权／任务及账号／旧UNKNOWN保持；已过BLE、手动SoftAP及自动连接／扫描不重复。

## 1. 目标与冻结边界

先让已有设备接入流程可正确发现、取消、串行操作、离页释放及恢复上下文，再定位真实 S3 GATT 失败。
复用中央 scanner、WiFiProv SDK、身份 journal、账户 fence 和原管理 API，不重造配网／认领执行链。
正常停止不是故障；未知关闭不是已释放；离页不是授权重置；缓存目录不是新鲜密钥上下文或访问授权。

本批不扩到同镜像 BLE／SoftAP 动态切换、ESP8266、任务准入、分区／OTA、MAC 合并身份或 TLS 降级。
不清授权、不重建现有身份、不重放 UNKNOWN，不发验证码、不升级 SDK、不部署服务或提交推送。
已有无关源码和前批 H1／H2、位置等修改保留；原始账号／网络凭据和日志只在忽略目录，不入 Git。

## 2. 已有复查事实

本机复查证据在 App 的忽略目录 `build/access-triage-20261029`；不是所有报告项的真机通过证明。

| 报告项 | 当前事实 | 证据层级 |
| --- | --- | --- |
| #31 | 正常停止扫描实际显示 `BLE_SCAN_CANCELLED`；owner 主动 stop 的取消被当成失败向上抛 | 真机复现＋代码链 |
| #36 | 冷进入已有 owner 重新配网页实际提示上下文缺失；缓存不保存 credentialVersion／locator，页面同步要求二者且不等待读取 | 真机复现＋代码链 |
| #30 | canProvision 不排除 WiFi 扫描；扫描方法不排除发送，原 SDK 请求可并发 | 静态确认；尚无连接会话中的实物差异 |
| #13／35／51 | 只在销毁／成功跳转释放，缺缓存离页与后台合同；本轮正常返回实际销毁并释放，无 BUSY | 缺口与未复现边界分开 |
| #33 | App 入热点后才请求云身份；插件和原 SDK 均可能绑定整个进程，影响并行 Cloud 请求 | 静态跨层冲突；本轮未切热点复测 |
| #29／49／60 | 当前错误不能区分原生 GATT／服务发现等失败；现场两次扫描未见 BLINKER_ 候选，未尝试连接 | 待匹配候选，根因未定 |
| #32／39 | 现有 allocation／Admin journal 和同实例复用已有实现；本轮 9 项相关软件回归通过，不等于新增实物断连／重载验收 | 保留既有链路，不先重写 |

当前 beta 的 directAccess 关闭，添加向导受守卫保护，但配网工具路由／菜单仍存在。
这是入口与部署合同需另行核齐的差异；原生声明支持不等于云端身份／签发已开放，不直接打开总开关。
“本机没有该设备的蓝牙授权”指 Direct 设备凭据，不等同于 Android 蓝牙权限。

## 3. 执行顺序

| 编号 | 最小范围 | 完成条件 | 状态 |
| --- | --- | --- | --- |
| AR-1 | 主动取消分类、扫描／发送互斥、页面离开／后台的有序清理及迟到结果隔离 | 新增有限回归先复现；预期取消不报错，真实故障仍上报；未知 close 保持 fence，重新进入不复用失效 session | 软件有限通过，原 SDK Java 编译通过；尚未安装／真机差异 |
| AR-2 | 重新配网的新鲜 owner 上下文读取 | 冷目录也可按精确 ID 读取；先 loading，成功才允许操作；离页／切号取消或丢弃，失败手动恢复，不刷新整个首页 | 软件有限通过，见 §9；匹配部署／真机另批 |
| AR-3 | SoftAP 本地请求与 Cloud 的网络隔离 | 覆盖 SDK 自身绑定，不靠等待／ready；原插件、身份链和 UNKNOWN 保持 | 原生与App阶段软件接线有限通过；§17真机发现系统网络授权界面被后台保护抢先取消，完整首次接入未通过 |
| AR-4 | 匹配 S3 配网候选及原生阶段日志，短差异验收 | 精确芯片／产品入口／profile／分区与实际广播匹配；一次连接逐段定位，不误连同名旧板 | 软件稳定后再选真机批次 |

AR-1 与 AR-2 均已有限软件收口；AR-2 详情响应 no-store、首次鉴权恢复与重配意图保护已实施，取 §9 最新结果。
当前接续结果取 §17：匹配内测App已安装，两种配网已分别实际执行但全链路均未通过；只有经过验证的隔离owner按显式阶段接入Cloud，§10旧主进程／缺元数据的门禁保留。不能把局部连接通过、软件回归或编译通过代替首次入网。
未部署 Gateway；已安装范围、实际拒绝与精确恢复取 §17，不将其概括为整表修复。
后续项若需要新的传输接线、部署或匹配固件，明确停点，不声称整表完成。

## 4. AR-1 最小实现合同

### 4.1 原生取消

复用 `WiFiProvDiscovery` 与同一个 `AndroidBleScanner`，按本 owner／run 区分预期 stop 与异常取消。
只有本次主动 stop 对应的 CANCELLED 归正常结束；原 DEADLINE 仍正常。
意外 CANCELLED、BACKGROUND、CLOSE_FAILED／关闭未知继续保留分类，不能统一吞掉或把错误改为成功。
旧回调不影响新 run；不修改扫描节流预算，不增加第二个 scanner。

### 4.2 页面互斥与生命周期

BLE 发现可停止后转交同一 SDK，但扫描、选择／连接、WiFi 扫描和身份／WiFi 写入不允许交叉发命令。
函数入口与原按钮两处均设门槛，不能只禁用 UI；WiFi 扫描期间不能发送，发送期间不能刷新 WiFi 或更换设备。
SDK 接管期间保留独占租约；不能为了消除 BUSY 而在每次 stop 时无条件释放它。

缓存离页、后台和销毁都应先使本轮操作失效，停止计时器，再排空晚启动、关闭原生会话及监听，确认关闭后释放 owner。
异步权限、扫描、选择、连接及 WiFi 列表的晚回包不得重新打开页面／写入状态；离页后不得继续调用后续身份或 WiFi 写入步骤。
关闭失败保持独占 fence 并显示需要手动恢复，不自动重试；普通返回前台不自动扫描／重连／重发。
重新进入／用户明确重试时才重新准备当前页；不清持久身份 journal、原授权、UNKNOWN 或制造新的 CREATE。

### 4.3 有限验证

- 原生：本 owner stop、自然 deadline、意外取消、后台、关闭失败及旧回调隔离；先红灯后绿灯。
- 页面：真实模板验证 WiFi 扫描时发送禁用，发送时扫描禁用；函数直接调用也不得绕过互斥。
- 生命周期：权限／SDK 晚启动、缓存离页、后台、销毁、正常重入、晚连接／晚列表及关闭失败 fence。
- 只重核受影响身份 journal／同实例恢复，不重跑任务、历史、自动化、BLE 长循环或新设备创建。

## 5. AR-2 上下文与授权合同

复用现有精确设备详情读取，只取 logicalDeviceId／credentialVersion／locator 及必要 active／owner 条件。
校验 no-store、返回 ID、字段边界、账户／authority；不 reveal／rotate Key，不把上下文写入普通目录缓存。
读取需有界且可取消；错误保持可理解的手动恢复入口，不靠自动循环或全量 getAllInfo 补齐。
页面仍在发送前校验真实设备实例，既有 CAS、credentialVersion、Admin 与重配语义不降低。

已核定的最小接线：

- 使用现有 owner-only `GET /api/v2/devices/:id` 详情路由，不新增 reveal／rotate／签发接口；服务层增加薄读取方法，复用现有 allocation 字段校验。
- 现有详情 GET 未设置 no-store；需仅补该响应的 `Cache-Control: private, no-store` 及有限后端回归。不靠请求端随意加头或 fetch cache 选项绕过 XHR／CORS 合同。
- 冷页可能先于首次账户恢复创建；只允许初始未登录态等待一次正常身份恢复。真实 HTTP 失败后不得自动重试；读取采用 15 秒整次截止、取消和账户／authority／页面代次隔离。
- “正在重配”取路由意图，独立于上下文是否已加载。loading／失败均不得降为新建流程或触发 CREATE；不覆盖未确认的 Admin／rotation journal、requestId 或 CAS。

## 6. AR-3／4 的先决条件

原 Espressif Android 2.4.4 的 `ESPDevice.connectWiFiDevice()` 自身也会选择 WiFi 并绑定进程；SoftAPTransport 使用普通 URL 连接。
仅删插件 bindProcessToNetwork 不是网络隔离，短暂全进程 bind 仍会污染并发 Cloud；不借“修复”引入反射改 private 字段、热补丁 SDK 或未维护的复制实现。
应先给出能覆盖实际 transport 的最小方案及失败／取消恢复；若只能分阶段切网，必须明确与无缝并行的区别及用户操作要求。

原 SDK 没有公开的 Network／SocketFactory／Transport 注入点；本批不复制整个传输栈。
后续可先评估显式分阶段：热点本地 GetInfo 完成→排空并关闭本地会话→确认原互联网恢复→复用原云身份 journal→重入同一热点并再次校验实例／epoch／root→继续原安装与 WiFi 请求。
这需要两次入热点及相应系统确认，不是一次连接的并行通信；不提前 CREATE、不重放 UNKNOWN，也不把 unbind 等同于互联网已恢复。

S3 诊断按广播→GATT→服务／特征／描述符→版本／安全会话→GetInfo→云身份→WiFi→Cloud 分层。
Direct 的 GATT 重新发现补丁不等于 WiFiProv SDK 已具备相同处理；无匹配广播时不按“第一个同名设备”连接。
阶段日志必须可关闭、有界且不含 PoP、Key、secret、SSID／密码或完整业务载荷；未到云端前不归因 signer／CA。

另有原生连接剩余边界：当前插件 clearState 将 pendingConnectCall 置空而未明确拒绝；SDK 连接事件没有本次原生 session 标识。
AR-1 的 JS Promise／监听代次隔离不证明匿名 SDK 晚事件已关联到正确连接，也不证明 clearState 返回等于全部物理 GATT 已断开。
此项与 AR-4 一并限定复查，不误写为本批已解决，且不得通过提前释放 owner 或再次连接掩盖。

## 7. 交付与收口

每项分别记录实现、有限回归、原 SDK 编译和匹配安装／真机结果，不互相替代。
先保持软件差异可审阅；必要真机批次沿用当前账号／服务，安装前保留完整请求日志，不做无关整机／Flash 备份。
设备固件、任务 scope、云签发与服务准入不因 App 修复自动修改。缺匹配配网候选时保留 AR-4 待测，不靠清授权推进。
完成选定批次即停，不自动扩到报告全部功能；不提交推送。

## 8. AR-1 实施与有限结果（2026-10-29）

### 8.1 必要源码

App 原生 `WiFiProvDiscovery` 仅增加同一 scanner 的薄测试端口与 owner stop 状态；预期取消归正常，意外取消、BACKGROUND、CLOSE_FAILED 保留，晚 run 不能释放新 owner。
配网页复用 `AppVisibilityService`、原独占租约和原身份执行链：函数与真实模板双层互斥，缓存离页／后台使本代失效并有序排空，关闭未知继续 fence。
普通回前台不扫描／连接／重发；显式重新开始或缓存路由重入只准备当前监听，不自动开始业务。
账户／authority 和页面代次共同校验；特别补上身份安装完成后的代次检查，防止离页再进入时旧 Promise 继续发送 WiFi。
原持久 journal／UNKNOWN 不清理，原配网布局与按钮保留；此次 UI 规范仅用于禁用／恢复语义，不改视觉样式。

### 8.2 红绿与编译证据

| 范围 | 红灯复现 | 最终结果／边界 |
| --- | --- | --- |
| 原生 owner 取消分类 | 8 项中 3 失败、5 保护项通过 | 8／8 通过；host JUnit，使用原 Android SDK 头，不是 Android 运行时实测 |
| 页面生命周期／函数互斥 | 最初 8 项全部失败；另补的跨重入旧身份写入 1 项实际失败 | 最终 11 项通过，覆盖后台、缓存离页、晚权限／连接／列表／身份及失败 close 手动恢复 |
| 真实生产模板 | WiFi 扫描／写入互斥 2 项失败 | 修复后原模板禁用门槛通过；不是复制模板或强制恢复状态 |
| 受影响 App 有限回归 | 复用原身份恢复／owner 测试，未重开任务等长队列 | 最终 4 文件 57 通过、7 跳过；包含上述页面／模板及相关原有测试，不累加多轮数量 |
| 原 SDK Android Java 编译 | 不改 SDK／依赖／锁文件 | 单次 `:app:compileBetaJavaWithJavac --offline --no-daemon --max-workers=1` 成功，1 分 7 秒，292 tasks（2 executed）；不等于 APK 打包、覆盖安装或真机连接通过 |

本批复现和验证日志取 App 忽略目录 `build/access-repair-20261029`；唯一 Java 编译完成结果另在本机 Gradle daemon 日志，未重开重复队列。
未安装新版 App、未操作开发板／云服务或更改授权；当前手机仍是修复前已安装 beta，不能据此宣布报告 #30／31／35／49 全部真机销项。
与此前 2130 文件哈希基线比对，除本批明确修改的配网页／测试／原生适配外无其他文件变化，依赖与锁文件保持；原无关修改、凭据／完整请求日志保留。未提交推送。

### 8.3 当批停点（后由 §9 接续）

先执行 AR-2：新鲜 owner 详情薄读取、no-store、冷登录恢复与重配意图保持，补有限红绿回归；不安装服务不代表已修远端运行版本。
再单独选择匹配 App 安装及 AR-1／2 的短差异验收，或先决策 AR-3 分阶段切网合同。
原生连接 pending-call／session 关联仍单列；没有匹配 S3 配网广播前不做碰运气连接、清授权或宣称 GATT 根因已定位。

## 9. AR-2 冷重配上下文的软件收口（2026-10-29）

### 9.1 实施合同与文件边界

1. App `DeviceV2ManagementService.readDeviceContextV2` 复用原 owner 详情 GET／Bearer／错误码，只投影 logicalDeviceId、credentialVersion、locator。严格验证 HTTP／body 200、no-store、精确 ID、active／cloudEnabled 与原 allocationContext 字段边界；不查完整目录、不 reveal／rotate Key、不存目录缓存。
2. `waitForWiFiProvAccount` 是独立薄辅助，复用现有账户 epoch／authDataChanged 语义，不引用 room 页面模块。只对首次缺身份等待正常恢复：初始空 session 最多前进一代，已有 token 补 UUID 只能同代；无 auth 的残留 profile 不算登录。完成／取消立即清监听，不轮询、不写账户、不发码。
3. 配网页构造时固定原 route 重配意图，独立于上下文是否已加载；新增 context 等待态及真实模板 aria-busy，读取成功前函数与按钮均禁止进入配网。首次恢复与单次 GET 共用 15 秒整次截止；离页／后台／切号取消，旧响应再校验 generation／authority／account 后才可接受。
4. 后台／缓存返回不自动重读、扫描或发送。仅“重新开始”明确操作才读取一次；原 CAS／rotation requestId 保持，新详情与已保留 context 不一致则停止，不以新版本覆盖旧 UNKNOWN 意图。真实设备实例核对、原 Admin／allocation journal 与已确认身份复用链保持。
5. Gateway 仅对精确详情 GET 设置 `Cache-Control: private, no-store`，在 region／鉴权早退前生效，成功及 401／404／400／503 均不可缓存。原 owner-only 仍由实际 management 校验；非 owner 与不存在维持相同 404，list／CREATE／DELETE 行为不变。

页面复用原布局／状态卡／手动恢复按钮；本次 UI 规范仅影响读取等待、禁用和恢复语义，不变更样式、地图或其他功能。
新取消检查不依赖较新 `AbortSignal.throwIfAborted`，使用 aborted 状态及原事件，避免旧 WebView 缺方法阻断本次读取；不升级 SDK 或改全局 polyfill。

### 9.2 有限红绿结果

- 修复前页面 12 项中 11 项失败、普通首次配网保护 1 项通过；真实旧页面复现重配意图丢失、依赖目录、无新鲜读取、无初始恢复／取消及 CAS fence。新薄读取首项也确认尚无 API。
- App 主有限队列 7 文件：90 通过，另 6 项旧扫描测试因夹具尚未提供构造器所需 route 失败；只补夹具后该 6 项通过，不放宽生产路由合同。
- 终审额外移除 signal 的新方法模拟旧 WebView，页面／薄服务 2 项先红灯后绿灯。最后只跑受改动的兼容／取消／截止及扫描夹具 13 项，全部通过；不重开主队列、原生 Java、H1／H2 或 BLE 长循环。
- 本批选定的 7 文件共 98 个不同用例分批通过（含上述保护项，不累计重复执行数，不称单次全量 98 项）；11 个服务旧方法用例、11 个无关 guide 用例未选。Angular 测试构建／类型检查通过，未构建 APK。
- Gateway 新增 no-store 套件先红灯后绿灯；最终与既有相关会话／授权回归的有限组合 11／11 通过，原 JS 语法与 diff 检查通过。测试调用真实 dispatcher／owner 校验，使用隔离仓储，不监听端口、不连业务库或云端。
- 另一次未筛选既有 session 文件出现 1 项旧 inventory 断言漏 displayMetadata（24／25），与此次 header 补丁无关，原记录保留，未为了绿灯修改或宣称完整服务器总测通过。

App 源码与此前 2130 文件哈希基线对照，除 AR-1 与本批明确的配网页／测试／薄服务外无其他文件变化；依赖／锁文件不变，服务器原有定位等脏改保留。
未触碰真实账号、授权、UNKNOWN、完整请求日志、设备固件或任务 scope；未发送验证码、自动重试、部署、提交或推送。

### 9.3 运行停点与下一批

本轮只关闭 AR-2 必要源码与有限软件回归，**不关闭真机接入报告**。当前运行 Gateway 可能尚无新 no-store，旧 App 也尚无新冷重配读取；后续需要匹配部署才可做运行验收，不能单装 App 后把响应不匹配当作新增设备故障。
若选择运行短差异，只验证原设备冷进入→首次恢复／单详情读取→标题及按钮恢复、失败→后台／缓存返回不重试→手动恢复；保留原账号／日志，不创建第二个设备、不旋转 Key、不刷板。
SoftAP 分阶段回网按 AR-3 单独实施，首批安全前置取 §10；匿名原生连接晚事件／S3 GATT 按 AR-4 单列，不以 AR-2 通过代替。

## 10. AR-3a：SoftAP 身份 Cloud 安全前置（2026-10-29）

### 10.1 最小范围与新增事实

本批先修“已知热点路径仍继续发身份 HTTP／准备意图”，**不是完成 SoftAP 云端配网**。
原身份链的 Cloud 需求不止 CREATE／Reveal：Admin.finish 先注册或重置 Admin，rotation 才随后 GetInfo→Bootstrap；只先取 Key 再重连不能覆盖整条链。

本地精确 Android SDK 2.4.4 runtime 核查还确认：

- SDK disconnectDevice 会清 session、unbind 并注销 SDK 自己的 WiFi callback，但吞掉注销异常，没有关闭结果。
- 插件另有一层热点 specifier callback，也吞掉注销异常；两层 onAvailable 均缺少关闭代次检查，存在晚回调重新 bind 路径。
- clearState／disconnect 的 resolve 不能作为“已确认恢复互联网”，也不证明物理连接关闭或 RPC 全部取消。
- pendingConnectCall 在 disconnect 时未明确终止，clearState 仅置空；能力查询／SDK worker 也没有完整取消合同。当前没有现成的 releaseNoInternetWifi API。

因此本批不做隐式 unbind、自动重新入热点、HTTP 探活／重试、反射修 SDK 或另一套身份实现；先关闭不具备网络前提的请求入口。

### 10.2 必要源码与保护合同

新增 `wifiprov-management-port.ts`，只在原管理服务外包一层显式方法端口：实例解析、CREATE／Reveal／Rotate、启用云、Admin 查询／注册／重置、退休查询／删除共 10 项。
工厂捕获本次实际 selectedDevice.transport，不读可变 UI 选择；每次调用前后复用账户／generation fence，BLE 转发原 this／参数／结果一次，原错误直接返回，不增加重试。
SoftAP 抛明确 `SOFTAP_CLOUD_NETWORK_REQUIRED`，未知传输拒绝，不向真实服务发请求。Admin／Allocation／Retirement 和原 page helpers 复用同一端口，不复制 CAS 或身份流程。

页面在只读 GetInfo 与已确认身份的 WiFi-only 快路之后、进入任何身份协调之前再作前置检查。
这同时避免“已有 Bootstrap allocation、尚缺 Admin”先写新 pending 记录，或先计算新的 rotation requestId 才被 HTTP 门禁拦住。
原 journal、CAS、UNKNOWN、已准备的 allocation 不因阻断而移除／改写；不同实例及接入根变化仍由原校验拒绝。

实际 SoftAP connect 一经尝试，当前页保留网络未确认标记；更换 UI 传输、丢失 selectedDevice、startOver 或 clearState resolve 不清除此标记。
AR-2 详情读取在账户恢复前、发 GET 前及接受结果前均检查此边界；首次未连接的正常 owner 详情读取不受影响。
成功后的 getAllInfo 与检查点清理 resolve 同样受保护。已确认的本地身份仍可原路发送 WiFi，成功后保持 success，只提示手机回网后从设备列表查看；不自动查云端、导航或清检查点。
沿用原状态卡、返回和手动恢复入口，仅完善明确错误文案，无布局／样式重构。

**支持边界：** 以上标记只保护当前配网页与已捕获事务，不是跨页面／全 App 的进程网络隔离，也不是原生互联网就绪证明。
本批所有尚无回网确认的 SoftAP 身份 Cloud 路径均安全停止，不能据此声称 Android／iOS SoftAP 可完成首次接入。单纯从本页返回也不等于互联网已经恢复。

### 10.3 有限软件证据

- 页面原链先跑 8 项：7 项实际红灯、BLE Key-only 原链保护 1 项通过。复现未经门禁的 Cloud 请求、传输切换旁路、成功后自动刷新及 clearState 误放行。
- 初次测试队列因新测试的 6 处索引签名访问触发类型错误而停止，已修夹具；这不计为业务红灯。
- 首轮接线后 4 文件 81／81 通过：端口 26、页面 8、受影响原身份恢复 34、AR-2 13。
- 终审另加“Bootstrap allocation 不先写新 pending Admin”一项，实际先红灯（save 被调用一次），再将 preflight 前移；最终只重跑受影响页面与原身份恢复两文件，43／43 通过。
- 本批四文件共 82 个不同选定用例分别通过，不累计重复轮数、不称一次全量 82；Angular 测试构建／类型检查通过。未重开 Java／APK、Gateway、H2、BLE 长循环或真机队列。

与原 2130 文件哈希基线核对，除 AR-1／2 已有差异和本批明确的配网页／测试外无额外变化。
未改 SDK、插件、服务、依赖或锁文件；未操作手机／开发板、账号、真实授权、UNKNOWN 或完整请求日志；未部署、发码、提交或推送。

### 10.4 AR-3b 最小接续顺序（原分批合同，进度取 §11）

1. 先修原生 attempt／stage fence：SoftAP callback 与 UI runnable 不再访问可变新 selectedDevice；关闭时 settle 原 pending connect、拒绝迟到事件，区分无法取消的 SDK worker 与本地终态。
2. 复用 disconnect 保留热点选择，排空本地 RPC，清理插件与 SDK 两层 callback 后再最后 unbind；失败／未知保留禁入，不把 clearState 当回执。
3. 加纯只读、布尔有界的网络观测：进程未绑定＋默认网络 INTERNET／VALIDATED；不返回 SSID／BSSID／IP、不 HTTP 轮询。它仅是当时网络前提，不能承诺 SDK 再无晚 rebind 或目标服务可达。
4. 接原 Cloud／Local 双薄适配：连续 Cloud 操作复用同一回网阶段；返回本地前由用户明确继续，重连原热点并复读相同 instance／epoch／根状态。WiFi provision 同样经过 Local 门槛，原 Admin／Allocation／Retirement 不复制。
5. 切网等待期间保留事务忙态，正常 disconnected 不重置为 idle；离页／后台／取消终止本轮，不自动回网、重连或重放 UNKNOWN。原阶段只有未执行的下一步可继续，未知写入仍走既有协调。

暂不接原生 Network.openConnection Cloud 旁路：它依赖第二条可用网络，还需独立 HTTP 白名单、TLS、截止／取消／单终态以及现有 Bearer／region／requestId／journal 接线，且不能解决其余 App Cloud／MQTT 的进程绑定，不是本批更薄的替代。
下一批按以上原生最小合同推进；在完整接线与匹配真机差异之前，AR-3 与报告 #33 保持未完成，AR-4 亦未销项。

## 11. AR-3b-1：原生退出终态与网络只读观测（2026-10-29）

### 11.1 本批最小实现

仅修改 `capacitor-wifiprov` 原端口及其必要声明／说明，不改 SDK 或另造身份执行链。

- `ProvisioningCallScope` 是不依赖 Android 的薄 owner／epoch／单笔 ticket 状态。主插件统一 main Handler actor 序列化选设备、connect、RPC 结果及关闭；不持锁调用 SDK／Capacitor。
- connect、WiFi 扫描、provision、custom endpoint、reset 的本地结果用同一活 ticket；终态只消费一次，关闭先失效 ticket／owner，再通知旧调用。旧结果与 progress 不再更新新状态，晚选择扫描不能替换新 owner。
- 取消读取返回 `WIFIPROV_OPERATION_CANCELLED`；在途写入与不透明 custom endpoint 取消／超时返回 `PROVISIONING_RESULT_UNKNOWN`，不表示设备未执行。不重发、不清应用 journal／授权，也不把 UNKNOWN 变成失败后重新初始化。
- 插件自有 SoftAP NetworkCallback 捕获原 device、ticket、epoch 与 callback 身份，进入 actor 后重核才 bind／调用 SDK，不再由异步 UI runnable 读取可变 selectedDevice；失效回调不 bind、不连接、不发新状态。
- 关闭先 fence，清理 SDK 及插件的 callback 后最后 unbind；SDK 抛异常也继续插件清理。忽略的异常和 bind false 不再被当作成功。关闭正在连接／RPC、既有 UNKNOWN、SoftAP 已进入 SDK 或清理失败均保持 quarantine，并拒绝 `WIFIPROV_CLOSE_UNCONFIRMED`，不靠 clearState 解禁。主插件另保留进程内单向标记，重建 Capacitor Bridge 也不能忘记旧 UNKNOWN；不以新插件实例作为原 SDK 退出证明。
- connect／扫描／custom／reset 的本地异步等待截止为 20 秒，provision 为 60 秒，扫描关闭为 5 秒；这些是 actor 上的有限等待，不是对 SDK 同步调用或 worker 的硬抢占。超时只终结本地调用并隔离，不自动重试。

`getInternetState()` 仅 Android 提供，输出 processUnbound／internet／validated／ready 四布尔；ready 只是前三者的与。
一次读当前绑定、默认网络与 INTERNET／VALIDATED；异常／未知按 false 返回。不访问 HTTP、不返回 SSID／BSSID／IP、不轮询、不改变网络、不清 quarantine。
TS 声明与 Web unavailable 薄实现已补；iOS 原实现保持，未声明该平台已支持或通过新观测。

### 11.2 未扩大承诺的边界

SDK 2.4.4 的全局 DeviceConnectionEvent 没有来源 ID；版本／能力只是缓存，不能用于补造事件归属。
本批证明的是自有闭包 ticket 隔离，以及取消／超时／未知／SoftAP 退出后禁止在同一不明 scope 上新开连接；**不是给 SDK 匿名事件增加了 session 身份**。
为避免把普通 BLE 多次进入降为“每次必须重启进程”，正常已完成、无在途的 BLE 关闭／重连保留旧能力；其旧匿名事件窗口仍属 AR-4 未证明范围，不作已修承诺。

SDK 自己已经排队的回调／worker 无公开 drain 或取消回执，仍可能晚 bind。
即使 getInternetState.ready=true，也不能证明所有 SDK 工作已停止、目标 Gateway 可达或永久不会再 bind；不得据此解除 AR-3a 门禁或 quarantine。
因此 **AR-3b 的 Cloud／Local 切换、用户明确继续及同设备复读尚未接入，完整 SoftAP 配网依旧不可用**。
本批安全拒绝可能要求结束当前原生进程后再重新准备，不能在 App 中自动重开、自动“修复网络”或重放 UNKNOWN。

### 11.3 有限验证与实际消费者

- 原 JDK／JUnit 主机合同 23／23：CallScope 15 项覆盖 owner、旧 ticket、重复终态、单笔互斥、取消读／未知写、回调重入／抛异常、quarantine 不解禁；网络 DTO 8 项覆盖全部真值组合及四个不可变布尔字段。
- 插件原 TypeScript `tsc --noEmit` 通过；未安装依赖、改版本或生成 dist。主机测试没有运行 Android Handler、ConnectivityManager、真实 EventBus 或 SDK worker，**不冒称 Android 运行时验证通过**。
- 原 SDK 源码定向 Java 编译通过。使用忽略目录 `.build/ar3b-20261029/source.gradle` 临时将 Gradle 的 WiFiProv 消费者指向当前插件源码，不覆盖 App 的 dirty settings，不把 node_modules 的旧 hardlink 副本算新候选。
- 首次 `:app:compileBetaJavaWithJavac --offline --no-daemon --max-workers=1` 成功（1 分 15 秒、15 tasks executed）。终审修正“已有 quarantine 仍可能 close resolve”后只对修改源码定向重核，同目标成功（43 秒、3 tasks executed），不是并行／重复未改队列。
- 最后补进程内标记后，仅重编实际变更的 `:capacitor-wifiprov:compileDebugJavaWithJavac`，11 秒成功（17 tasks、仅 1 executed）。未再编 App／开第二个队列；主机合同和 TS 检查未因只改桥接私有状态而无谓重复。
- 原编译有 flatDir、SDK XML 版本提示；首轮 App Java 有 EventBus ThreadMode.MAIN 注解类不可见警告，编译成功，不借此升级 SDK 或依赖。未做 APK／打包／签名／安装、真机切网或固件验证。

App 产品源码、其账号／授权／UNKNOWN／完整日志保持原状；服务、设备、任务 scope 和云签发均未操作。原 SDK／Gradle／包及锁文件不变；无提交推送。

### 11.4 当批停点（后由 §12 接续）

先为上述 **Android 桥接运行差异** 设计并补有限验收：关闭正在 connect／读／潜在写时各一次终态、迟到自有回调不生效、清理失败不报成功、重建桥接不忘 quarantine，以及四布尔观测。不得制造真实身份写入来凑 UNKNOWN；本地不透明端点的取消分类与实际设备是否执行分开。
若仍限制不部署／不安装，可先做桥接测试夹具与取消合同回归；不要重跑 AR-1／2 软件矩阵或绕过 AR-3a。
在决定恢复 Cloud／Local 分阶段接线前，须明确未知 SDK 关闭的有限支持策略；不以固定等待、缓存 metadata 或一次 ready 快照消除不确定性。正常 BLE 匿名事件按 AR-4 单列，完整 SoftAP 与报告 #33 继续未关闭。

## 12. AR-3b-2：实际桥接主机回归与有限缺陷修正（2026-10-29）

### 12.1 不安装的验证方法

复用当前 `WiFiProvPlugin.java`、CallScope、InternetState、BleDiscovery 原生产源码，新增 `capacitor-wifiprov/test/bridge-host` 独立夹具和运行脚本；不复制插件的 owner、取消、UNKNOWN 或单终态逻辑。

- 仅外部边界替身：Android Handler／Looper 用可推进 FIFO 虚拟时间，不立即执行 post、不 sleep；网络状态／错误与 SDK 回调由测试显式触发。
- Capacitor PluginCall 记录每一次 resolve／reject，不自行去重。SDK listeners 与已注销网络 callback 仍保留供迟到注入，不在替身中假装已经取消。
- 原缓存 SDK 2.4.4 提供真实事件、常量、监听接口和 WiFiAccessPoint；Capacitor 注解／permission enum 取原 App 依赖源码，EventBus 注解取原缓存。仅 SDK 设备／管理器和桥接／平台对象用薄替身，不运行其 worker／无线栈。
- Runner 不安装依赖、不改 SDK／锁文件，不改或替换生产源码。每轮只在忽略目录 `.build/bridge-host/run-*` 生成独立源码／classes及输入 SHA，避免旧 class 复用；可用 `--method` 只跑指定差异。
- 原生产源码编译与替身运行分开：另由已有 Gradle source override 核对原 SDK 的真实消费者，不以夹具的 API 子集证明 ABI 或 Android 运行行为。

完整运行方式、范围与排除项见插件 `test/bridge-host/README.md`。QR 文本解析、权限弹窗、真实 EventBus 派发、Android 多线程、GATT、SDK worker、切网及设备实际执行均未模拟或宣称通过。

### 12.2 两处实际复现与最小修正

1. **销毁后排队进度泄漏**：真实 provision 回调先排队，再调用 handleOnDestroy；关闭任务尚未执行时，原 wifiConfigSent／Applied 只检查 ticket，仍多发 3 次进度。新增反例先红灯，再在原两个 progress 闭包补 destroyed／closing 条件，不改变取消合同、WiFi ACK 或原写入意图。
2. **连接元数据异常使 connect 悬挂**：原 CONNECTED 先退休 ticket，后读取元数据；getter 异常时旧 call 没有终态，截止任务也不能再完成它。新增反例先红灯（reject 为 0）。现先确认活 ticket、构建元数据，成功后再退休；异常走原 failConnect／quarantine。failConnect 先返回拒绝终态，状态事件中的诊断元数据异常只省略 device，不再阻断终态。

元数据没有被用于配对匿名 SDK 事件；scope、CAS、UNKNOWN、授权及已有请求身份未重置。未引入自动重试、轮询、新的 SDK 或身份执行链。

### 12.3 有限结果

- 两个新缺陷分别实际红灯后转绿；主机桥接整组 **22／22 通过**。
- 覆盖：connect／只读扫描取消，custom／provision／reset 在途 UNKNOWN，晚成功／失败／进度、SoftAP 迟到 callback、SDK 清理抛错后继续 unbind、unbind false／unregister 错误、20／60／5 秒边界、busy／owner、选择扫描迟到、普通已完成 BLE 重连、桥接重建保留 quarantine、四布尔映射及观测不改网络。
- 最终收紧元数据用例为“不吞 RuntimeException”，并验证 runner 新输出目录后，仅定向复验两项：各 1／1 通过；不累计为 24 项，不重跑前批纯 Scope／DTO 23 项或 App 软件矩阵。
- 仅对实际变更的插件重编原 SDK `:capacitor-wifiprov:compileDebugJavaWithJavac --offline --no-daemon --max-workers=1`，**13 秒成功，17 tasks 中仅 1 executed**。未重编 App、生成 APK、安装或改现有服务。
- 主机 javac 有原 annotation-jvm 缺 Kotlin 注解元数据的 9 条警告，非编译／断言错误；原 SDK 编译仍有 flatDir／SDK XML 提示，不为清警告升级依赖。

这是“真实 Java 桥接代码＋确定性外部边界”的合同证据，**不是实际 Android Handler／EventBus／网络或 SDK 取消／物理断开验收**。
App 产品源码、原账号、授权、UNKNOWN、完整日志、设备固件与任务区均未操作；无部署、验证码、自动重试、提交或推送，保留原无关本地修改。

### 12.4 当批停点：先明确支持策略（接续结果取 §13）

§11.4 的无安装夹具已完成，不再把同一取消／观测矩阵重复排队。下一步先明确 Cloud／Local 接线的受限支持合同：

- 当前公开 SDK 无法证明旧 worker／匿名事件已完全排空；SoftAP 退出保持 quarantine。不能仅看到 ready 或等固定时间就解除门禁后重连。
- 若继续限制不安装／不部署，只做接线方案与准入决策，不虚构真实 Android 证据或为补样本制造写入 UNKNOWN。
- 如要恢复完整 SoftAP 首次接入，必须先选定可验证的原生网络／SDK 所有权策略，并明确需要的维护范围；不是把当前安全拒绝删除或复制一套身份流程。本批不擅自变更 SDK、平台支持或发布范围。

AR-3a 继续关闭 Cloud 身份路径；正常已完成 BLE 的匿名旧事件窗口仍按 AR-4 单列。报告 #33 和完整 SoftAP 依旧未关闭，当前没有新的真机或长循环队列。

## 13. AR-3c：Cloud／Local 受限支持与跨页门禁（2026-10-29）

### 13.1 本批先冻结的支持合同

原 SDK 不提供旧 SoftAP worker 排空证明，本批不实现可反向切换的 handoff，也不以固定等待、clearState 成功或 getInternetState.ready 解禁。

| 路径 | 本批决定 |
| --- | --- |
| BLE 配网的 Cloud＋Local 身份链 | 复用原串行 owner、账户／代次、CAS 与 journal；不新增执行器 |
| SoftAP 的已确认身份 WiFi-only 重试 | 仅保留本页已有 deviceKey 为空的 confirmed allocation，且本地 GetInfo 核对 instance、根与必要 accessEpoch 后继续；不是一般冷重配或重载恢复支持 |
| SoftAP 首次认领、冷重配、Admin／Bootstrap／rotation、重载后 pending／UNKNOWN | 仍拒绝需要 Cloud 的下一步，不提前创建替代身份／写入意图，不自动断连重连或重放 |
| BLE UNKNOWN 的云端只读协调 | 原生 Local quarantine 不自动等于 Cloud 网络污染；保持既有原请求协调，不因本批扩大禁入 |

一次 SoftAP 尝试后，本进程中的配网 Cloud 门禁保持关闭；单纯新建页面、Bridge 或读取到 ready 均不能解除。重新启动原生进程也不是未知写入已确认，原请求／授权与 UNKNOWN 仍须沿原合同协调。
正常已完成 BLE 的匿名 SDK 旧事件仍是 AR-4，完整 SoftAP 与报告 #33 不因此销项。iOS 不新增原生网络证明，不扩大平台支持。

### 13.2 最小接线与执行范围

已确认页面字段 softApNetworkUnconfirmed 会随页面重建遗忘，而 owner 详情 GET 在初始化插件之前发生；原生随后拒绝 Local 不能阻止此前 HTTP。

1. Android 增加只读 getProvisioningNetworkState，仅返回 softApAttempted 布尔。进程静态单向标记在 connectSoftAp 入口、任何 requestNetwork／bind／SDK 网络操作之前置真；不进入 Local quarantine 判断、不增加清除 API，不返回网络标识。
2. App 沿原配网 Cloud 门禁接入该读取：身份操作预检（在准备意图之前）、management 每笔 HTTP、冷重配详情及成功后刷新均先读取。页面已置真的标记不可由 false 覆盖；Android 缺方法、失败或非法响应均安全停止，不退回 ready。新页面通过原生进程标记找回禁入原因。
3. 原账户／authority／页面代次在异步读取后、HTTP 前及接受回包时复核；不让新的异步边界使已离页调用继续。BLE 的 UNKNOWN 不产生 SoftAP 标记，不改变原只读协调。
4. 只做新门禁差异回归、实际桥接的新增布尔合同与原 SDK 定向编译；AR-2 仅核异步门禁实际影响的页面读取，不重开原完整矩阵。§12 的 22 项、AR-1、H2 或 BLE／断电循环不重复。不部署、安装、操作账号／设备、发码、升级 SDK 或提交推送。

接线复查另发现：原 openProvisionedDevice 在 Cloud 门禁前关闭原生会话，SoftAP 的预期未确认 close 会把已有 WiFi success 降为 error。本批仅保护本代／账户有效且设备已经明确确认的 success，保留 closeFailed／未释放 owner，不刷新、不清 journal 或重发；未确认 WiFi 结果不吞异常。异步门禁失败后同样先复核账户／代次再更新提示。

该读取不是原子 Cloud reservation，也不证明全 App HTTP／MQTT、其他插件或任意并发页面已隔离。支持前提仍是已有单配网页／单 owner／busy 串行合同；需要真正双向切换或并行时另选可验证的原生网络所有权策略，不静默复制 SDK 或身份管线。

### 13.3 实施结果

最小接线已实施，不改原身份执行器、授权、CAS、UNKNOWN 或重配目标。

- 插件增加单一布尔观测和进程 SoftAP 标记；`getInternetState` 原四布尔合同不变，不新增解禁 API。`processQuarantined` 与网络标记分开，BLE 未知写入不凭空变成 SoftAP 网络污染。
- App 新增独立 `wifiprov-network-state` 薄读取：缺方法／拒绝／非法响应／取消／3 秒整次截止均拒绝一次，无 ready 回退、网络请求或重试；这是拒绝截止，不是等待后放行。生产页面接入原门禁，各异步边界后再核账户／代次，读到 false 不能覆盖等待期间新出现的本地 true。
- 新页面读取原生 true 后不发 owner 详情 HTTP；已有本地已确认身份的 WiFi-only 快路不引入 Cloud。已确认 WiFi 的 close 未确认保持 success／result 与 closeFailed，原 journal／owner 不释放、不刷新／导航；未确认 WiFi 不吞异常。网络错误提示也不能覆盖切号／离页 fence。

有限证据分层记录：

1. App 首次选定 AR-3c 10 项：8 项实际失败、2 项已有保护通过，复现页面遗忘原生标记、坏状态仍 Cloud、异步前后账户边界缺口；未将它计为真机 HTTP／网络证据。接线后 3 文件 **50／50** 通过（management 26、读取 5、页面 19）。
2. 终审新增“WiFi success 被清理失败降为 error”反例实际红灯后修复；最终仅跑受改动的 AR-3c／已确认结果和 AR-2 页面，**26 通过／10 未选**。收尾仅核未确认 close 不吞异常及原已确认本地 WiFi 快路，**2 通过／21 未选**。这些轮次有重复，不累加为总用例数或称全量测试。
3. 新增原生主机合同 **6 项分批通过**：初始 false 只读、仅选择热点不置位、网络回调／SDK 进入之前已 true、同步 requestNetwork 失败不清、close／Bridge 重建／ready 不清，以及 BLE UNKNOWN 仍 false。首轮 5 项因缺 API 失败，明确不冒称旧业务故障复现；未重跑 §12 的 22 项。
4. 插件原 TS noEmit 通过；实际消费者继续用原 Gradle source override，唯一 `:capacitor-wifiprov:compileDebugJavaWithJavac --offline --no-daemon --max-workers=1` **BUILD SUCCESSFUL，约 1 分钟，17 tasks 中仅 1 executed**。原 flatDir／SDK XML 提示保留，不升级依赖消警告。App 有限测试构建／模板与类型检查通过，未构建或安装 APK。

原 App node_modules 的旧声明没有新方法，本批通过窄只读端口按实际方法存在性校验，不手工修改 node_modules／锁文件；后续安装必须包含匹配插件原生实现，不能仅更新 Web 包。Android 缺新原生方法时只停止，不静默退回不受保护的旧链；iOS 不调用新增观测，未新增平台验收承诺。

这些是生产页面／桥接加外部替身及原 SDK 编译的有限软件证据，不是 Android 真机切网、GATT 或 SDK 排空证明。原账号、授权、UNKNOWN、完整请求日志、云服务与设备均未操作；无部署／安装、验证码、自动重试、SDK 变化、提交或推送，无关本地修改保留。

### 13.4 下一停点

本批必要源码／合同回归结束，不再重开相同门禁或退出矩阵。仍不部署／安装时，不宣称已修现网或新增真机证据；之后若授权运行短差异，先安装包含插件的匹配 App，再按 AR-1／2 和 AR-4 的原范围核对，不扩账号、任务或固件支持。
完整 SoftAP 首次接入需独立选定能覆盖 SDK 自身网络绑定的受维护策略（如明确所有权的隔离进程或可注入传输）；当前未实施，不通过删门禁、固定等待或 ready 快照替代。保留报告 #33、AR-4 及 UNKNOWN 原恢复语义。

## 14. AR-3d：配网主线复核与完整 SoftAP 最小实施决定（2026-10-29）

### 14.1 主线与名词纠偏

本次 BLE／SoftAP 是 **ESP32 WiFiProv 配网 bearer**，不是设备类别：BLE 通过 provisioning GATT，SoftAP 通过设备临时热点；二者复用标准 WiFi scan/config/status 和同一 `blinker-config`。
目标仍是 `GetInfo → 原实例／云身份核验 → 必要的 Install/Bootstrap → WiFi candidate → 设备以 DeviceKey 登录 Cloud → 退出 WiFiProv → 可选日常 Direct BLE`。BLE-only 认领、运行期 Direct／MQTT／LAN 和网关子设备协议不属于本批，普通手动 WiFi 的 Key／SSID 接口不改，R4／Wio 不扩成 WiFiProv 产品。

设备实际 selector 为 `src/BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h`：默认 BLE，要求底层 IDF-NimBLE；不满足时编译拒绝，**不会自动切 SoftAP**。SoftAP 需显式 `BLINKER_ESP32_PROVISIONING_SOFTAP`，同一镜像动态双路仍不在范围。
接入旧决策中的 2026-08-22“SoftAP 默认”和优先计划 §3.1“以后才评估默认 BLE”已陈旧，本批纠正执行描述，历史实测／未测事实不覆写。当前 App 的安全停止不是该设备 bearer 已完成或准入。

### 14.2 两种方案的实际改动与维护成本

核查对象固定为本机原 SDK 2.4.4 runtime（SHA256 `0199ef4493ea5468d0c5b8e3a7905b836f5610a5cee2fa773bd9f15c7ae971ef`）及该 tag 官方源码，不以新版本 API 作假设。
ESPDevice 固定创建 SoftAPTransport，transport／security／session 为 private；公开 Session 构造器也不能接回其 private 扫描／配置链。原 SDK 多个 callback 自行 bindProcessToNetwork，SoftAPTransport 用 URL.openConnection 且没有公开 drain／cancel；因此仅删插件 bind 或替换一行 URL 均不足。

| 比较项 | SoftAP 专用独立进程 | 可注入 Network／Transport |
| --- | --- | --- |
| SDK | 保留现有 2.4.4 二进制，SoftAP SDK 对象全部迁子进程 | 当前无注入 API；需维护同版本源码 fork，非简单插件参数 |
| 必要改动 | 非导出 Service、单 owner IPC、远端死亡确认、桥接路由、前台生命周期与分阶段薄适配 | ESPDevice 构造／网络获取／断开，SoftAPTransport 连接工厂、整次截止／取消／worker 生命周期，Gradle 唯一替换 |
| 故障边界 | 该代 Binder 死亡可界定旧进程不再运行；IPC 死亡仍保留在途写 UNKNOWN | 可按连接取消，但必须由受维护实现证明晚回包、worker 排空和无进程 bind |
| 长期成本 | 多一个短生命周期进程、IPC DTO／状态和应用初始化审计；无需跟踪 SDK 内部补丁 | 无额外进程，按 Network 选路更直接；持续承担上游 diff、license／来源、唯一依赖和 BLE 回归 |
| 不解决的问题 | 不凭空增加可用互联网，不证明设备写入未执行 | 同样不提供第二条物理网络，不替代身份／UNKNOWN 协调 |

**本轮选定最小实施方向：Android SoftAP 专用独立进程。** 理由是复用当前 SDK 和身份链、不引入 SDK fork；它不是几行开关修复，也不是 Android `isolatedProcess=true`。若未来明确接受同版本 SDK 分支维护，再评估注入方案，不在本批同时建设两条链。
**配网仍统一由现有 `capacitor-wifiprov` 插件提供。** 独立进程只是该插件的 Android 内部实现：Service／IPC／SDK owner 放在插件模块与插件 Manifest，App 不另建配网插件、不直接调用 ESP SDK／Binder／热点 HTTP，原 `WiFiProv` JS 接口和 Blinker 身份 coordinator 保留。iOS 仍沿插件原端口单列，不借 Android 方案改写。
这里是网络／生命周期隔离，不是不同 UID 的凭据安全沙箱；不使用 `isolatedProcess=true` 来绕过应用权限要求。
Android 官方支持组件 `android:process`；process bind 仅作用当前进程，且官方更推荐 per-Network sockets。该事实支持两方案的边界，不代表已完成本产品验证。

### 14.3 最小运行结构与首次接入路径

- 主进程：原 App 身份 coordinator／management／账户／CAS／journal／UNKNOWN 和 BLE 路径不迁移；插件内部只持远端 SoftAP 代理，App 继续调用原 `WiFiProv`，不直接管理远端 Binder。
- 子进程：由 `capacitor-wifiprov/android` 声明的非导出、显式绑定 `:wifiprov` Service，使用应用自身权限；持有 SoftAP 的 manager／device／Session／EventBus／worker 和全部热点 callback，不把 SoftAP SDK 对象同时留在主进程。不新增常驻前台服务／自动重连任务。
- IPC：只转发现有 local 命令和结果／进度，session nonce＋owner epoch＋requestId 关联，方法／端点白名单、有界载荷、单笔请求、整次截止和一次终态；不支持任意 URL、文件、Cloud 令牌或通用 RPC 执行。复用 `ProvisioningCallScope`，不复制身份 saga；敏感入参不记录或落盘。
- 退出：close ACK／stopSelf／unbind／onServiceDisconnected 不作为旧 worker 停止证明；等待匹配远端 Binder 的真实死亡，超时保持未确认，不自动替换 owner。写请求进入远端后未获设备确认即保留原 UNKNOWN，进程死亡也不将它改成“设备未执行”。服务或主进程恢复不重放队列；主进程死亡应终结本代子进程。
- 网络：若热点期间存在可用蜂窝／并发默认网络，Cloud 可沿主进程原 HTTP；没有互联网时，最小兼容路径是 `Local 读取 → 结束子进程并确认死亡 → 用户明确恢复互联网 → 原 Cloud 步骤 → 用户明确重入热点 → 新子进程复核同一 instance／epoch／root → 原 Local 下一步`。连续 Cloud 操作复用同阶段，不强制每笔都切网；无人工确认不自动重新连接，无真实 HTTP 成功不宣称 Cloud 完成。
- 接线点：原 `CapacitorEsp32ProvisioningTransport` 已是可注入的 custom endpoint 薄端口，`protectedWiFiProvManagement` 已统一 Cloud 入口。后续只让这两个边界接受已验证的隔离 owner／阶段，不把串行身份逻辑重写一遍。每次 Local 重入先核实例和根，不因界面选项或名字相同继续写。

第一批新模式限定 Android 10／API29+ 和既有 ESP32 SoftAP Security1 profile；BLE 旧平台范围不变，iOS、API26～28 的完整 SoftAP、动态双 bearer 不随之开放。仍保留当前 legacy 主进程的单向 taint：新 worker 不能清除先前已污染的主进程标记。后续只有真实实现了隔离 owner 的新路径能使用自己的证据，不调用 clear/reset 解禁旧路径。
SDK 原日志存在 cookie／旧连接分支敏感日志，不能宣称整个二进制日志已脱敏；首批不导出原 SDK 全量日志，只取自有阶段／错误码，保留原业务请求日志，日志抑制需实际核查后才宣称完成。

### 14.4 本批先实施的必要修正与后续顺序

除纠正历史默认描述，还核到与所选路径直接相关的权限缺口：插件 FINE_LOCATION 仅声明到 API30；API31／32 只申请蓝牙，并把 location 固定报 granted。配网不能依赖用户曾在地图页授权。

本批仅修此前置：

1. FINE／COARSE 声明到 API32，Android12 的精确定位与粗略定位同批请求；按明确 transport 申请 BLE／SoftAP 真正需要的权限，回调沿原请求 transport，不读取可变选择。无参调用保留组合申请兼容。
2. App 在扫描／创建前传固定 BLE／SoftAP，必需权限未 granted 即停止；原生 SoftAP 网络入口再检查实际权限，缺权限在 requestNetwork／bind／taint 前返回明确未发送，不变为 UNKNOWN。
3. API33+ 的本地热点连接沿既有 neverForLocation 的 Nearby WiFi，不为此扩大到精确定位；设备自身扫描 WiFi 不等于手机调用 WifiManager.startScan。API31／32 系统定位服务仍是运行前提，授权不等于连接成功。
4. 仅跑新增权限差异与原 SDK 定向编译；不操作手机／设备／账号、不发码、不部署、不升级 SDK 或提交推送。当前 SoftAP Cloud 门禁不解除。

接续固定为 **权限前置 → SDK Local owner 薄提取与子进程／IPC 实际接线 → 原两端口阶段适配 → 匹配安装后一次首次接入短验**。不先添加未消费的 Service／IPC 骨架，不重复既有拒绝矩阵，也不再无边界审计选型。

### 14.5 证据与状态

方案、三份旧文档的执行口径纠偏与最小权限修正已完成：

- 插件 request／check／permission callback 均用同一原请求 transport／OS 策略；API31／32 SoftAP 不再硬报 location granted，FINE／COARSE alias 成对声明，请求结果真实返回。不需要的字段沿原 granted 约定，不冒称对应 OS 授权。省略 transport 保留组合兼容，非法值不发权限请求。
- 原生 connect 在创建 ticket／调用 SDK／requestNetwork 之前复核 SoftAP 实际权限；缺失明确返回 WIFIPROV_PERMISSION_REQUIRED，本地已知未发送，未产生原生 SoftAP taint 或 UNKNOWN，不自动弹窗／重试。
- App 扫描与热点入口分别向原 WiFiProv.requestPermissions 传固定 ble／softap，而非读取可能变化的 UI selector；prompt、prompt-with-rationale 或缺字段均不继续。原 discovery 测试的空权限替身改为真实完整 DTO，不放宽生产门槛。原暂停／离页／owner／CAS 与身份链保持。
- 原生新增 7 项主机差异：首次 6 实际红灯／1 组合兼容保护通过，修后 7 项分别 1／1 通过；覆盖 API30／31／32／33／35 的所需 alias、回调不随选择变化、非法参数、32 仅粗略定位／33 缺 Nearby 的网络前拒绝、33 不依赖 Bluetooth／FINE，以及 annotation／Manifest 真实声明。未重跑旧 28 项退出／标记矩阵。
- App 新增 8 项先 6 红／2 保护绿；修后与实际受影响的 discovery 6、lifecycle 11 同一有限队列 **25／25 通过**，Angular 测试构建／类型检查通过，不累计红绿轮次。页面测试使用外部 native／SDK 边界替身，不冒称权限弹窗或手机连接通过。
- 插件原 TS noEmit 通过；沿已有 source override 的唯一原 SDK `:capacitor-wifiprov:compileDebugJavaWithJavac --offline --no-daemon --max-workers=1` **46 秒 BUILD SUCCESSFUL，17 tasks 中 1 executed**。原 flatDir／SDK XML 警告保留，无依赖／锁文件／SDK 变化。未构建／安装 APK、部署或操作真实账号／设备。

§14 当批快照：完整独立进程、IPC、阶段适配及 SoftAP 首次接入尚未实现／验收；后续原生候选接线结果取 §15。App Cloud 门禁仍关闭，不将设计／权限回归当成可用配网功能。原授权、UNKNOWN及完整日志不动，无验证码、自动重试、提交推送，无关本地修改保留。
原接续为插件内部 SDK Local owner 薄提取与远端生命周期，不重开方案比较或权限矩阵；当前进度和下一停点统一取 §15，不重复派发本段历史任务。

### 14.6 参考

- 原 SDK [ESPDevice 2.4.4](https://github.com/espressif/esp-idf-provisioning-android/blob/lib-2.4.4/provisioning/src/main/java/com/espressif/provisioning/ESPDevice.java)、[SoftAPTransport 2.4.4](https://github.com/espressif/esp-idf-provisioning-android/blob/lib-2.4.4/provisioning/src/main/java/com/espressif/provisioning/transport/SoftAPTransport.java)。
- Android [进程与线程](https://developer.android.com/guide/components/processes-and-threads)、[Service manifest 属性](https://developer.android.com/guide/topics/manifest/service-element)、[bindProcessToNetwork](https://developer.android.com/reference/android/net/ConnectivityManager#bindProcessToNetwork(android.net.Network))、[Network.openConnection](https://developer.android.com/reference/android/net/Network#openConnection(java.net.URL))。
- Android [本地 WiFi Request API](https://developer.android.com/develop/connectivity/wifi/wifi-bootstrap)、[WiFi 权限](https://developer.android.com/develop/connectivity/wifi/wifi-permissions)、[Android12 定位请求](https://developer.android.com/develop/sensors-and-location/location/permissions/runtime#approximate-request)。

## 15. AR-3e：插件 Local owner、独立进程及 IPC 候选接线（2026-10-29）

### 15.1 已实施范围

仅在原 `capacitor-wifiprov` 实施，不另建插件、不改 App 身份链、服务端、设备端 wire 或 SDK2.4.4。

| 模块 | 实际职责 |
| --- | --- |
| `ProvisioningSdkLocalOwner` | 子进程唯一 SDK 实例；标准连接／扫描／WiFi 配置／custom／reset 的薄适配，复用 CallScope、原结果 DTO 与 UNKNOWN 分类，不持账号／Cloud／journal |
| `SoftApWorkerService` | 非导出 `:wifiprov` Service；核对进程名、父子 PID、UID／版本／nonce／replyTo，独立控制 HandlerThread 管 IPC 与终结；SDK 构造及执行留给另一个 main actor |
| `SoftApWorkerProtocol` | 插件内部 DTO 白名单及上限，不是新设备协议：请求 8 KiB、响应 64 KiB、custom 1024 B，仅 `blinker-config`，禁止任意 URL／文件／通用执行 |
| `SoftApWorkerChannel` | 单笔 ticket、序号及 nonce 关联、终态／取消／截止；明确 SDK 拒绝不误报 UNKNOWN，未知写不转成“未执行” |
| `SoftApWorkerClient` | 主进程显式 bind／HELLO／Messenger 与对应 Binder 死亡，跨 Bridge 的单 owner 预留；无自动重绑命令／重放，空 Binder 或未启动与真实死亡分别处理 |
| 原 `WiFiProvPlugin` | 原 JS API 不变；SoftAP 七种 Local 命令转交远端，BLE 沿原链。QR 同一路由，不在主进程创建 SoftAP SDK device／执行 SoftAP 网络绑定 |

删除已失去消费者的主插件 SoftAP NetworkCallback／显式 bind／cleanup 分支，不保留失败后退回主进程的第二条路径。原 SDK 的 BLE 清理内部行为未修改，不能扩大为“主进程从此没有任何 SDK unbind”。
新候选仅 Android API29+／SoftAP Security1（SoftAP 缺省为1，BLE 缺省不改）；API26～28、Security0／2 不在此路径支持范围。一个子进程只选一个 SDK 实例，关闭后须明确重新 create/select；不把 BLE 已完成重连改成强制重建。当前主进程 legacy 标记仍无清除 API，查询不代表远端已死或准许 Cloud。

### 15.2 生命周期与故障合同

- Service 控制线程不执行 SDK；SDK 初始化、匿名 EventBus 及网络回调均归子进程 Local owner 的 main actor。一生仅一次 create／connect，原匿名事件没有获得新 session 身份，而是不能跨实例复用该子进程。网络 callback 在进入 SDK 前复核原 ticket／owner，重复 onAvailable 不重复 bind／connect。
- 命令帧带固定 nonce 和单调正 requestId；重复／旧命令不重放。RESULT 对应原请求，EVENT 使用同 session 的 request=0，进度由 owner 活 ticket 筛选；事件不消费命令终态。payload／元数据先构造有界 DTO 再退休 ticket。
- 首次 create 的 **绑定＋HELLO＋create 共用20秒整次预算**，不是两段各20秒。后续 Local 为原20秒／provision60秒；这些仍是 actor 等待截止，不冒称 SDK 同步调用可被线程抢占。
- close 先失效 pending：读取消、潜在写／opaque custom 保留 UNKNOWN；取消、超时、父进程死亡及晚回包不自动补发。不搬运或清除主进程原持久意图、账号与授权。
- close ACK、onServiceDisconnected、解绑、ready 或5秒截止均不释放 owner。仅对应 Binder 死亡后接受退出；5秒只拒绝 close 并保留 fence，晚死亡不二次 settle 原调用。系统返回空 Binder／完全未送远端命令时的“已知未启动”单独回收，不冒称发生了 Binder 死亡。
- Service 在正确专用进程内只终结自己，不向外部 PID 发 kill；CLOSE／父 Binder 死亡由控制线程处理，即使 SDK main actor 卡住也不等待它清理。实际是否成功死亡仍由客户端 Binder 通知确认，不以调用 kill 本身代替回执。
- HELLO 已送而回执丢失也可请求 CLOSE；关闭期间晚绑定只 HELLO→CLOSE，不执行原 create。SDK 明确的 scan／provision 拒绝保留原错误，其他未确认写继续 UNKNOWN。

### 15.3 有限回归与编译证据

所有以下主机测试都运行实际生产类，只有各自明确的外部平台／SDK 边界替身；无手机、网络、业务记录或设备写入。

| 有限集合 | 实际结果及排除项 |
| --- | --- |
| 内部协议 | 真实 JSON 与实际 Protocol，11项通过；不是 Android JSON／Parcel／Binder 实测 |
| Channel | 18个不同 JUnit 方法分批通过；“取消重入死亡后遗留close timer”实际先红，修后仅原项复验；SDK明确拒绝反例先在SCAN分支红，修后同方法SCAN／PROVISION两分支绿，不补造另一红灯 |
| Client | 14个不同方法分批通过；外部 Context／Messenger／Binder 只记录和显式投递。丢HELLO ACK、公开create整次预算各实际先红后定向绿；不把虚拟死亡当真进程证明 |
| SDK Local owner | 10个不同方法分批通过，覆盖旧回调、权限、单 owner、重复网络、DTO／超限／元数据、deadline和UNKNOWN。跨包订阅反射实际先红，类改public后仅原项绿；没有用setAccessible掩盖，也不冒称真实EventBus派发通过 |
| 主插件路由 | 新路由9项＋BLE reconnect／UNKNOWN2项分批通过，证明主插件不自己建SoftAP SDK／显式bind、原接口转发及外部旧事件隔离；终审在真实JSON迁移及主插件清理后，仅补核受影响的BLE connect取消／read取消／销毁进度／metadata／busy5项，分别通过，不重开旧28项矩阵 |

旧 bridge-host 的 map-only JSON 替身已删除，采用真实 `org.json:json:20240303`。该唯一新增测试输入仅在忽略 `.build/worker-host-deps`，78,332 B，发布 SHA1 核对并固定 SHA256 `3cf6cd6892e32e2b4c1c39e0f52f5248a2f5b37646fdfbb79a66b46b618414ed`；未加入 Gradle/npm/SDK 依赖。新 runner 每轮新目录／输入 SHA，不复用旧 class、不下载或自动重试。
旧主进程 SoftAP 网络测试随该路径删除／迁移，不用失效断言凑通过；历史 §11～14 仍描述旧实现。Client 测试中的 Service 仅是 Intent 名称标记，主插件测试中的 Client 仅是全记录远端边界，均不能代替真实 Service／Binder。

首轮原 SDK Java 编译发现 Client final actor 初始化顺序错误，已修，属于编译错误而非业务反例。真实JVM JSON与Android JSONException继承不同的host编译错误也已用兼容源代码修正，不改写生产源码来迎合替身。
匹配插件源码的原 SDK 定向编译通过；整次预算修正后仅重编实际变更插件，最终 `:capacitor-wifiprov:compileDebugJavaWithJavac --offline --no-daemon --max-workers=1` **12秒 BUILD SUCCESSFUL，17 tasks 中1 executed**。首轮编译错误／中途通过不累计为最终轮数。原 flatDir／SDK XML 警告保留；本批未编 App Java／APK、签名或安装。
仅因新增组件另做 `:app:processBetaMainManifest`，14秒成功、67 tasks 中3 executed；实际合并文件核对 AppID `iot.diandeng.tech.beta`，Service `:wifiprov`、`exported=false`，无 `isolatedProcess=true`。这仅是合并配置证据，不是进程启动证明。

### 15.4 当前停点与下一批

**本批原生候选接线完成，完整 SoftAP 首次接入仍未完成。** App 业务源码／身份链本批未改，原 SoftAP Cloud 门禁保持；本机手机／服务／设备并未运行本候选。无部署、验证码、自动重试、SDK 升级、提交或推送，授权、UNKNOWN、完整日志与无关修改保留。
下一批只接原 Cloud management／Local transport 两个薄端口的隔离 owner 与显式阶段，不重写身份 saga，也不再比较方案或重复本批矩阵。必须保留 legacy taint，捕获匹配原生 owner 关联并在阶段切换后核对同一 instance／epoch／root；不能凭 capabilities、ready 或“存在worker”解除隔离。
原生与阶段接线均完成、且允许安装后，再做一次匹配 App 的首次配网短验，包括无可用默认互联网时的显式切换；不把主机测试、成功Manifest合并或SDK编译代替此验收。AR-4及报告#33仍未关闭。

## 16. AR-3f：原 Cloud／Local 薄端口与显式阶段（2026-10-29）

### 16.1 先核对历史，避免把旧可用范围改成另一条主线

用户补充曾可正常配网、怀疑 UI 更新回归。本批先核本地 Git，不以文档缺证否定用户曾成功：

- `a7c4ab1` UI6 前后的配网页 TS blob 都是 `7160b78aaec8e888db51f9443d9b50ce96f1882d`，GuidePage 也相同；当次只改 HTML／SCSS／ARIA，未把 CREATE／Reveal 改到热点之后。`0d9091e` 的配网页 TS 仅移除视觉 HeroCard。
- `28df962` 已是 connectSoftAp → create/connect → provision → GetInfo → Cloud resolve/CREATE/Reveal；未找到“最近 UI 才把 Cloud 从入热点前移到入热点后”的证据。`0b25326` 仅发 WiFi，`22f60ac` 明示“Wi-Fi已连接；Blinker设备身份尚未写入”，不能把这类成功替代完整 V2 首次接入。
- `3c1ed1b` 的部署 directAccess 门禁确实会隐藏 WiFi 向导／重新配网入口，是“入口消失／不能点”的候选原因；不是“已经进入热点后 HTTP 0”的证据。当前不擅自打开部署能力或改服务器准入，后续入口问题单列。
- 已有 P6-B Android BLE 配网证据保留；本批只接 SoftAP 的网络交替，BLE、手动 Key、Direct 及原身份 coordinator 不重写。

### 16.2 最小接线合同

1. 原插件在成功 create／connect／getDeviceInfo 的主原生端附 `localOwner={kind:'isolated-softap',version:1,session}`。仅来自已验证 HELLO 的活 Client，不采用子响应自报、capabilities、ready 或目录缓存；关闭／死亡后不提供。无新 JS 方法，App 缺元数据按原禁入停止，不回退旧主进程 SoftAP。
2. App 新增无 Angular／存储依赖的薄阶段控制，只包装现有 management 的实际 Cloud work 和原 Local transport。Local→Cloud 先等匹配 native close 结果，再等用户“已恢复互联网，继续”；连续 Cloud 复用阶段，每笔仍沿原鉴权／账户／CAS。失败停止，不自动重试 HTTP。
3. Cloud→Local 必须用户明确继续，再用原 WiFiProv create/connect 打开新 owner；原始热点选择固定，重入先 raw GetInfo 核同一 instance、accessEpoch、root flags 及 Bootstrap capability，成功才发送原下一步，不重新运行身份链或重放未知写。
4. 仅匹配的3B成功ACK且本轮有效时推进期望摘要：Install置 hasKey，其他不变；Bootstrap置 hasKey/hasAccess，并取该101B请求 offset45 的大端 epoch。只留非秘密 effect，不另存 Key/Admin/payload。非成功、未知或坏ACK不推进。
5. **证据上限：** 现有24B GetInfo不含 Key locator／fingerprint、controllerId／secret。这里核对的是已有 wire 可观察的根摘要，不宣称精确证明 Key/Admin 原始字节；保留原 Security1 物理窗口信任上限、服务端 CAS 和 Admin 检查点。不能为“核根”自动重写 Bootstrap，或暗中扩协议。
6. 阶段等待仅提供继续／取消，不轮询网络；离页、后台、切号／authority／generation变化取消本轮。关闭 Promise 去重，迟到 create／close／GetInfo 不继续 Cloud 或 Local；原账号、凭据、journal、UNKNOWN不清。实际发送后的潜在 Local 写取消仍 UNKNOWN，不能被普通取消错误掩盖。

### 16.3 验证与停点

先做缺元数据、旧 owner、死亡未确认、用户未继续、根变化、ACK后推进、取消／迟到及原 identity/CAS 的有限软件回归，再核匹配原生源码编译；不发验证码、不升级SDK、不安装、部署、提交或推送。UI复用原配网页／状态卡，按本批技能约束保留明确继续／取消和忙态，不做视觉重构。
本批源码接线及有限回归完成：

- 插件主端生成受验证的 `localOwner`，剥除子响应自报字段；create/connect/getDeviceInfo之外不附带。新增Client3／主桥3共6项主机差异分别通过，插件TS noEmit通过；不把被动metadata槽当真实Binder证据。
- `WiFiProvSoftApStages`是无Angular／存储依赖的阶段模块，只保留owner与可观察摘要。Page把它接到实际management执行、custom传输、WiFi扫描／下发和最后列表读取；不是只在HTTP前检查一次。首次已选热点参数固定，关闭Promise复用，重连先raw GetInfo，再把原下一笔工作交还原身份链。
- 页面复用状态卡、原按钮和配色，只加显式继续／取消；等待时不冒称正在发送，原表单禁入，最终WiFi确认不因云端阶段取消而撤回。BLE不创建阶段owner，保留原分配／安装链及列表失败时的缓存回退。
- 单一有限Angular队列先 **7文件129／129通过**；终核发现原“明确扫描失败后手填SSID”被阶段失败态禁用，限定修正后仅重验受改动阶段37＋真实Page10，**47／47通过**，其余已过文件不重复。全批共133个不同用例（新增52、受影响旧保护81），不相加重复轮次。首轮仅spec索引访问、对象字面量Security类型推断导致编译失败，修正后通过，不将其写成业务红灯。测试实际运行原Page／Allocation／阶段类，native／HTTP／持久化边界为替身；模板测试不发权限或网络请求。
- 仅Local实际操作、owner仍为Local时，读的`WIFIPROV_SCAN_FAILED`和写的`WIFIPROV_PROVISION_FAILED`原样交给页面，保留原手填／明确改密重试，不自动再次调用。已核原生这两码只完成ticket而不quarantine；Cloud、重连、custom、UNKNOWN及取消不套用例外，未知扫描结果也不再误提示可以手填后发送。该问题为终核发现并补有限回归，不补造修前实测红灯。
- 本轮匹配原SDK插件Java定向编译 **49秒 BUILD SUCCESSFUL，17 tasks中1 executed**；沿原source override、offline、单worker，无SDK或依赖／锁文件变化，保留原flatDir／SDK XML警告。未重跑上一批协议／Channel／Service矩阵，不把增量编译当APK安装或真机证明。

**本段软件批停点：AR-3f接线完成，当批未安装或真机。** 当批未部署服务、刷板、发验证码或操作真实身份／授权／journal，无自动重试、SDK升级、提交推送，无关本地修改保留。后续§16.4匹配安装与双路径真机结果现统一取§17，不再重复派发本段旧停点。

### 16.4 必须分别完成 BLE／SoftAP 真机收口

用户明确：BLE 与 SoftAP 都是原 WiFiProv 的配网方式，修复不能以其中一条可用替代另一条。本轮源码／有限回归不等于实机通过；后续匹配安装批次两项均必测，不绕过原插件、不改成 Direct BLE 控制链。

| 独立真机项 | 最小验收范围 | 当前证据 |
| --- | --- | --- |
| BLE 配网 | 匹配配网广播／选择、安全会话、WiFi扫描结束后下发、原身份接入与云上线；正常选择不误报取消、离页后不遗留扫描或继续写入 | §17连接／扫描／GetInfo通过；已有根缺当前App管理员凭据，身份及后续未通过；离页回收、返回不自动重连已核 |
| SoftAP 配网 | 匹配热点／安全会话、原GetInfo／Cloud身份操作／Local写入全程；实际远端关闭后显式继续、重新进入原热点核同一摘要、WiFi下发与云上线、取消后手动恢复且UNKNOWN不重放 | §18已修pause-only过早取消，true stop关闭真机通过；系统“不可上网／继续使用”授权未接受，连接及后续仍未通过，不能据20s RPC截止判SDK缺陷 |

安装前核对同一源码的原生APK与插件、SDK2.4.4及设备配网固件，现场先辨认测试板与已有身份，不为两种方式重复创建设备。每项记录真实APK／固件来源、请求关联、结果与未覆盖范围；权限、网络、原生连接失败分别保留实际原因。若网络或固件不支持某组合就明确待测，不写“整体已修复”。保留账号、授权、原CAS／UNKNOWN与完整日志；不发验证码、不自动重试、不升级SDK、不做长循环。

## 17. AR-4a：匹配安装与两种配网的实际短验（2026-10-29）

### 17.1 本批范围与构建

用户授权按§16.4构建／安装匹配App、核配网固件、分别验收两条路径。本批没有修改产品源码或授权策略，仅必要的内部候选／观测脚本与文档；不把既有根不匹配通过清授权绕过。

- 手机PGEM10／Android原beta保持同一账号。覆盖安装前保存原APK、SecureStorage、全部现存IndexedDB stores及logcat；原安全存储2条、automation/task intents各0条，原日志不清。
- App临时tsconfig映射到当前插件`src/index.ts`，原SDK native构建用既有`source.gradle`指向插件源码，不依赖pnpm快照是否含新增Java。实际APK检查旧包无而新包有`ProvisioningSdkLocalOwner`与四个worker类，原生registry、`:wifiprov`／`exported=false`、页面handoff和owner parser均核齐。未打开directAccess部署总门禁；原工具路由进入。
- 原构建链Web→sync→`:app:assembleBeta --offline --no-daemon --max-workers=1`通过，原SDK2.4.4不变；Android构建1m54s／441 tasks中113 executed。保留原体积、flatDir、SDK XML、FontAwesome路径等警告，不扩大成构建失败。既有pnpm脚本触发5个本地file依赖缓存刷新，无下载／版本升级；锁文件只有工具排序变化，已用patch还原，esbuild仍0.28.2，不能称完全未碰依赖缓存。
- APK **76,518,206 B**，SHA256 `bb9333bf479c5fb7ad4fb64923f64cc800116e1c59948acfdf76e9a5ef2bcc00`；同原debug证书`a76639ec…5d5d210`，`iot.diandeng.tech.beta`／HTTPS／cleartext=false／bridge logging=none。`adb install -r`成功，安装后与最终读回同SHA；账号、session、安全存储与原完整intents逐项哈希不变。
- 仅COM11（实际MAC尾`1b:96:14`），COM9不动。新鲜读NVS20KiB、独立ROM verify，核真实factory `0x10000+0x500000`、NVS `0x9000+0x5000`，未烧默认双OTA分区、bootloader或merged。原D4主程序由ROM比对确认SHA`e8a3c4c…adda13`，不再做8MiB长备份。
- 当前库源码冻结为内部WiFiBLE候选，保持D4的power／uptimeSeconds=42，task0、无TaskStore消费者，无嵌入Key／WiFi；不解除公开WiFi-only发行guard。沿原Core3.3.11／PubSub2.8／IDF-NimBLE及既有公开HTTPS／MQTTS／D4 P256公钥（keyId1，SPKI `57f69d14…58a6025`），不把历史公钥比对冒充本轮新Bootstrap验证。唯一队列串行BLE385s、SoftAP370s通过完整链接／实际consumer核查：1,372,128 B／1,396,288 B，静态RAM57,228 B／56,844 B。两次均只写应用并ROM verify，写前后NVS及原表精确不变。

### 17.2 BLE实际结果：本地通道通过，原根凭据阻断身份

1. 通过候选的显式串口`n`调用原`Blinker.resetNetwork()`，返回accepted1，只清WiFi；进入Provisioning、error0。不调用resetAccess、任务写／Start或命名空间替换。初始沿原保存网络启动曾见Fault／AuthenticationRequired23，未逐段归因，不冒充新的云端拒绝根因或云上线结论。
2. 真实App扫描精确`BLINKER_4B3EFCD46FF9`并选择；stop/select正常完成，无`BLE_SCAN_CANCELLED`误报。原SDK connect **1,235ms**成功；WiFi扫描**6,147ms**返回且含mostfun2026，扫描结束后才允许发送。
3. 通过原输入事件填写既定WiFi，点击一次发送。实际custom仅GetInfo（153ms），返回同实例、epoch1、flags7；云端两次不同原消费者的resolve-instance均200（670／66ms），不是失败自动重试。
4. 当前设备有已有access root，而当前beta仅有session／通知安装存储，没有该设备管理员凭据；原`enableCloudForExistingBleDevice`分支明确提示“本机缺少该设备的BLE控制凭据”。**这是现有身份／测试前提拒绝，不等于已证明WiFiProv协议或SDK连接缺陷。** 未运行Install／Bootstrap／WiFi provision，未创建、Reveal、Rotate或修改管理员；不以解除旧授权凑通过。
5. 离页clearState20ms成功；手动返回空闲页，无自动扫描／重连／发送；最后回首页。此次安全回收与页面恢复成立，不冒充另一轮完整GATT恢复或首次认领成功。报告#49在本匹配候选未复现，不能外推所有S3／公司包已销项。

### 17.3 SoftAP实际结果：系统授权界面被过早取消

- 初次：create350ms返回新`isolated-softap` owner；connect后约65ms，页面因visibility=false发stop／clear，connect在**72ms**收到`WIFIPROV_OPERATION_CANCELLED`，clear29ms完成。没有到SDK安全会话、扫描、GetInfo或Cloud管理。
- 独立系统证据：ActivityTaskManager明确记录系统UID1000发起`com.android.settings.wifi.action.NETWORK_REQUEST`，组件`com.oplus.wirelesssettings/Settings$NetworkRequestDialogActivity`。不是推断SDK自开完整设置；源码和真实缓存SDK2.4.4无该startActivity链。
- 已确认原因链：`AppVisibilityService`把native pause与appStateChange(false/onStop)压成同一active=false；Page263对false立即suspend→generation失效／abort／clearState。该保护在授权遮挡时先终止原连接。**尚无实际onPause／onStop双事件关联探针，不能据当前记录断言OEM只pause、未stop。** 后续修正必须先保留这个区分，不能忽略全部后台。
- 按取消恢复范围仅做一次明确对照：在系统界面手动连接同一开放AP（SSID、BSSID尾与COM11匹配，获得192.168.4.2），返回App后点重新开始并重新连接。新owner不同，仍弹系统NetworkRequest界面，connect **65ms**取消／clear46ms完成；不再循环。手动预连热点不是可用的免授权解决方案。
- 两次最终进程表均无`:wifiprov`，legacy main标记仍false。仅证明实际启动后的取消／关闭，不证明已绑定网络后的完整Cloud回网、重入摘要或首次接入成功。手动预连无互联网AP时，原App的`/account/connection`前台只读检查出现DNS／断网，恢复mostfun2026后200；它不是配网management请求，不能宣称全App无HTTP，也不据此解除隔离。
- 某次系统页仍在前台，观测器的只读CDP state到期；在执行后续重开操作前退出，保留失败记录。后来明确拉回App并新开观测器才执行上述唯一恢复对照；不把CDP超时当产品协议超时，也未在它之后自动重发。

### 17.4 收尾与结论

- COM11恢复原已精确匹配D4应用及完整20KiB NVS，ROM verify程序、NVS、原表均通过后硬复位；实例、access root和TaskStore快照在每次写前均严格核同原值。任务快照仍为原1条／revision3／SHA`d1773564…a741ac4`，未操作scope或UNKNOWN。未宣称原8MiB每个尾字节全量恢复，也未以原程序读回代替新一次云登录。
- 手机恢复mostfun2026，匹配新App保留首页；原账号、安全存储及完整intents再次精确一致。诊断binding／wrapper／CDP转发均回收，原logcat不清，临时系统XML已移除，本轮留有其私有副本。无OTP、测试重试循环、SDK升级、服务部署或Git提交推送，旧UNKNOWN与无关本地修改保留。
- 固件首次prewrite停在离线NVS审计的跨VM对象原型比较（结构相同），尚无写入intent／Flash写；修正本批工具的JSON投影后，仅显式复核原读取与MAC再继续单次写入。不补造为硬件故障或产品红灯。
- 证据仅在忽略目录：App `build/access-ar4-20261029`（安装／完整账户、request/native日志、最终独立audit）；库同名目录（冻结候选、串行编译、原始NVS、write intents和精确恢复）。其中含私有身份／网络与授权材料，不能整体交付或推Git。

**本批两条真机已执行，但§16.4全链路未通过：BLE停在既有根凭据前提，SoftAP停在系统授权／生命周期。WiFi下发、身份安装及新云上线均不能销项。** 不重跑已通过的连接／编译来累计“整体通过”。

### 17.5 下一批最小范围（先修真实阻点，不重开架构）

1. 先为原生pause、stop、resume及当前同owner connect做可关联、脱敏、有界的生命周期证据，补真实模板／有限回归；仅针对系统Local网络获取的前台往返设计最小接线，不改全局active语义。不允许后台HTTP／custom／WiFi写、不以ready／等待／曾有worker解禁，Home／离页／换号／截止仍关闭。若OEM实际stop，不能用“仅pause例外”冒充支持。
2. 并行只读核现有根与当前App凭据的正规恢复条件，区分“原设备WiFi-only重配”与“安装／重置Direct管理员”；保留owner／instance／CAS和原根，不直接删除管理员门禁或从NVS导出秘密伪造App凭据。当前不是空根first claim样本；若必须临时新身份／清根或近期登录，先明确批准范围，不发验证码、不复活旧scope。
3. 上述合同及修正通过后，只补未通过的SoftAP连接与两条身份→WiFi→云上线；复用本次匹配固件／账号／日志，不重编未变SDK或重跑BLE长循环。完成前AR-4和报告#33仍开放。

## 18. AR-4b：同owner连接的授权遮挡与后台边界（2026-10-29）

### 18.1 先证据、后决定范围

本批沿§17.5，不处理旧根管理员恢复，不清授权／TaskStore，不发验证码。先新鲜核COM11原D4应用、NVS20KiB及原表，复用§17已链接的同SDK SoftAP候选；无新的SDK编译队列。App账号／完整intents先保存，仅原生诊断APK先行，不提前改变Page取消行为。

新增默认关闭`plugins.WiFiProv.lifecycleDiagnostics`，WARN tag `WiFiProvLifecycle`，每owner最多16条／每Bridge64条。仅13个白名单关联字段，不含SSID、密码、Key、账户或载荷。snapshot仅诊断，closing／dead不恢复ownership、不作为Cloud放行。新增6项有限host通过，不累计原矩阵；真实APK编译／子进程及现场结果另记。

原页面probe真实8条关联证据：同session、requestId3／connect／pending／hello已验证→系统UID1000 NetworkRequestDialogActivity→native pause（距connect38ms、距系统界面启动2ms）→Page close-request→匹配dead close-result→14s后native resume。期间**没有stop**，事件数未触上限；该区间可采用pause-only窄合同，不泛化所有OEM。详细uptime／wall关联与原始截图、请求日志仅保留忽略目录`build/access-ar4b-20261029`。

### 18.2 最小实施合同

- 全局`AppVisibilityService.active`原语义不改，只增加独立的只读reason／sequence事件口；pause→stop即使同为false也不能被active去重吞掉。
- 仅用户已发起、主端已验证的同一isolated owner、唯一尚在pending的Local connect可暂存pause；调用前固定账户、authority与generation。不声称应用可以仅凭phase证明当前遮挡一定是系统弹窗。
- `current()`仍严格要求前台。暂停期不得扫描、GetInfo／custom、HTTP或WiFi写；原connect最多调用一次，结果只暂存。真正stop／Home、离页、切号、取消及截止仍走原close，迟到结果不能复活owner。
- 曾pause的连接在返回前台后必须明确继续，才调用原getDeviceInfo新鲜核同一owner／session／transport／Security1，之后交回原扫描或重入GetInfo链；不是以connected／ready解除Cloud隔离。Cloud仍须匹配close与显式阶段，重入仍核原instance／epoch／root摘要。
- 初次及Cloud→Local重入复用同一acquisition薄模块；整次JS上界只拒绝／关闭、不解锁，原native20s预算不变，不自动重试。候选UI复用原状态卡与继续／取消，等待不冒称正在发送。
- 真正native stop独立于JS触发原关闭链；与首个JS清理共享同一close，额外公开等待者仍BUSY。仅原death／已知未启动证明才释放；失败保留owner，stop不自动重试。stop时即刻标记该代，拒绝未发送SoftAP调用；resume仅恢复前台标记，不复活该代。pause不触发这项原生关闭。
- 网络选择本批需真实点选，而非仅给input写值；验证SSID、选中标记、空值／扫描禁发，截图不展示密码。仅有限对齐原SSID字节透传与busy时偏好按钮，不扩BSSID策略或重写SDK扫描。

### 18.3 实施与有限软件结果

- `AppVisibilityService`保留原2个native监听与active语义，只增加reason／sequence薄口。核真实Capacitor8.5：`BridgeActivity.onResume→fireStatusChange(true)`没有状态去重，已足以处理pause-only恢复，未增加重复resume监听。独立stop事件不因active已false而丢失。
- 独立`WiFiProvSoftApAcquisition`只持有同owner单次connect／有界用户继续，原Page和重入共用；失败走原release fence。暂停期间到达失败只保存有效context的错误展示，前台恢复才render，不会卡在connecting，不清UNKNOWN／journal、不解close。
- 默认关闭的诊断白名单模块与原Client／Channel只读关联口不影响owner校验。true stop增加Plugin内共享close薄协调，不修改SDK／分配器／身份链，不以snapshot代替退出。
- 网络选择有限修正：SSID下发保持原始字符串，不再trim合法首尾空格；仍用trim拒绝纯空白。扫描／写入时禁用记住网络及密码显隐，函数守卫也阻止busy偏好写入。SDK原扫描已按SSID聚合，不据无BSSID／channel扩造新扫描链。
- 唯一首轮App队列7文件84／84通过；补暂停失败展示后仅重跑受影响Page文件9／9通过，**合计85个不同用例**，不把重跑8项累计。含真实模板的显式继续／取消、网络选中标记、空SSID／扫描禁发、原stage／CAS边界、账户／owner／超时及迟到结果。
- 原生先通过新诊断host6项；随后onStop／共享close相关8项通过（5项新增、2项受影响诊断、1项原UNKNOWN回归）。这是确定性边界夹具，不冒充Android物理验收；原SDK2.4.4匹配APK另行构建成功。
- 原锁文件／SDK未升级；Web只构建一次，原工具sync后匹配诊断APK构建1m24s（441 tasks，27 executed）。沿用§17冻结SoftAP固件，没有重复设备编译队列。原有样式budget、FontAwesome路径、Gradle／flatDir警告保留，不扩成本批失败。

### 18.4 真实短验：生命周期修正成立，系统授权仍缺一步

本批仅COM11，同实例／原根／原TaskStore。首个观测脚本在路由动画未结束时点选不可用控件而退出，尚无create／connect，不计配网失败或重试。重新显式进入原页后共4个新owner，完整事件35条，各owner8～10条，均未触诊断上限；没有WiFi provision／custom／身份／Key／任务写入。

| 短验 | 实际结果与边界 |
| --- | --- |
| 系统NetworkRequest遮挡 | 每次匹配owner的pending connect→native pause，**没有再出现§17的65～72ms Page取消**；前3次均保持到原父RPC约20s截止再关闭（connect→close20,028／20,038／20,031ms）。不是用等待解除隔离。 |
| 首次系统点选与手动恢复对照 | 系统不是预期的标准“连接”按钮，而是Oppo完整WLAN列表。脚本只点精确AP，后因未找到标准按钮停止，未任意点坐标。手机确有同板AP／192.168.4.2，但截图另有“不可上网，是否继续使用”原生弹窗，**未接受继续使用**；CDP DOM看不到这层确认。一次明确手动重开／提早返回Main仍未取得native连接结果。不能把物理WiFi已关联冒充此worker的指定NetworkRequest获准，亦不能归为SDK缺陷。 |
| Home对照 | ADB Home在现场未实际切离Main／未产生stop，第三个owner仍到20s；该操作**不算真正后台通过**，也不是新的成功连接样本。后改用完整系统设置，只补未覆盖stop分支。 |
| 真正后台 | 第4个owner在pending connect时进入完整系统设置，**以stop为起点，1ms后close-request、68ms后匹配dead close-result**（close→dead67ms）。未等待JS或RPC20s才关闭；返回Main仅错误／手动重新开始，未自动create／connect／scan。无`:wifiprov`残留。 |
| 失败返回与WiFi选择 | 失败后错误和手动入口可见、不再connecting卡死；若connect在pause期间成功返回后的显式继续／新鲜同owner读取仅有软件证据，因本轮从未native connect成功而未进入。**App目标WiFi扫描列表的实际点选仍待测**，不得拿系统WLAN点AP或单元测试代替。BLE原扫描名单含mostfun2026的§17证据不重复；本批没有重跑BLE。 |

截图采用原始ADB全屏PNG，已逐张人工查看：系统WLAN列表、无互联网继续使用确认及底层Page错误状态。`run-fixed-local/system-9.private.png`清楚展示未接受的确认；`system-after-fix-1.png`展示同一系统阻点。截图不是成功配网／网络点选证明，不修改图片凑通过。原App全局`/account/connection`前台检查在无互联网AP下仍产生DNS失败，完整原日志保留；本批没有改这条全局链，不能宣称全App没有HTTP或没有其他既有轮询。

### 18.5 恢复、私有证据与本批停点

- COM11原D4应用SHA`e8a3c4c…adda13`、原完整20KiB NVS及原分区表逐项ROM verify恢复并硬复位；access root／epoch1、原TaskStore1条／revision3精确不变。COM9未动，无Key刷新、管理员安装／清除、创建云设备或scope操作。
- 诊断关闭仅做一次原生资产重打包（Web不重建），20s／4 tasks executed。最终同签名beta SHA256 **`381363febf6d3f3bfcd4cc3db5fb1cbac19864c1504fd2aad3eee5204ee7f48e`**，76,518,206B，实际APK配置diagnostics=false／HTTPS／cleartext=false，已覆盖安装并从手机独立核同SHA。安装后立即发起的首次只读CDP连接遇WebView socket尚未就绪而退出、转发已回收；稍后明确执行最终只读终核成功，不是产品请求自动重试。
- 最终同账号／session、安全存储2条及全部现存automation/task intents逐项哈希不变；旧UNKNOWN不动。手机已回mostfun2026／App首页；worker、CDP binding／wrapper／forward及临时系统XML均回收，logcat不清。
- 本批原始请求／native／系统事件／图片／完整账户日志、写入意图与恢复核查只留忽略目录`build/access-ar4b-20261029`。含私有材料，不整体交付或入Git。未部署服务、发送验证码、自动重试业务、升级SDK、提交推送；无关本地修改保留。

**本批收口：pause-only最小修正、真正stop物理关闭与失败返回保护有限通过；SoftAP连接／目标WiFi实际点选及身份→WiFi→云上线仍开放。** 不把85个软件用例或手机WiFi的IP替代这些实机结果。

### 18.6 下一批唯一最小顺序

1. 先处理截图已证的系统“无互联网／继续使用”确认：明确识别并由用户动作／原系统控件接受，保留同owner connect截止及pause／stop合同。只做一次有明确系统授权前提的SoftAP短验，不再盲重连、延长deadline或预连热点凑通过；不忽略真正后台。
2. 若接受后仍到20s，先核既有日志。原父Channel在IPC前起20s，早于child owner；`WIFIPROV_RPC_TIMEOUT`不等于SDK自身超时。必要时仅补默认关闭／每owner≤16条的child阶段证据，区分精确requestNetwork回调→owner／epoch接纳→bind→SDK connect→proto-ver／EventBus终态，不能以SSID／IP／ready替代。原SDK另有泛WiFi回调及HTTP无readTimeout是待核边界，**不是本轮已证根因**，先定位首处分歧再改。
3. 实际connect成功后才补App网络列表点选、SSID／选中标记、密码隐藏／草稿、空值与扫描禁发及离页取消截图。原根管理员前提继续按§17.5第2项正规协调，不删门禁／导出秘密／清授权；完整身份→WiFi→云上线、AR-4／报告#33仍未销项。

## 19. AR-4c：一次系统确认短验及回归官方用法核对（2026-10-29）

### 19.1 本次有限结果，不再重复试连

沿§18.6保留同App Web及原SDK2.4.4、原Core3.3.11冻结候选；仅原生诊断资产重打包1m3s／4 tasks executed，无新的Web／固件编译队列。新鲜小范围NVS／表和ROM应用比对确认原身份、root及TaskStore一致；没有8MiB重复备份。

准备时唯一串口`n`在启动后很早到达：STA正在连接，原SDK provisioning start设置空WiFi配置失败，整次`resetNetwork`返回0／InternalError25。不能把返回失败解释为持久清除未发生。ROM只读核原root／TaskStore精确不变、NVS有变化后，仅做一次冷启动，不重发`n`；只读诊断得到`state=provisioning`／error0。源码顺序也吻合clear已成功、紧接着restart失败。该设备热重置竞争保留为独立缺陷，未改SDK、未重放未知动作、未清授权或任务区。

真实App只有**一次create／connect**，原生同owner8条事件：pause保持原请求，父RPC在20,008ms返回`WIFIPROV_RPC_TIMEOUT`，随后匹配dead退出。系统观察只点精确热点一次，7次只读UI快照中未出现可接受的“继续使用”提示，无标准连接按钮；未自动重连、未GetInfo／custom／WiFi下发或Cloud身份写入。**§18截图中的无互联网确认是欠证系统交互，不足以认定为本次20s截止的唯一原因。** 不追加循环验证；用户提出应回到WiFiProv标准用法，后续先按下节评估，不直接沿旧“只补弹窗”假设继续。

结束已精确恢复COM11原D4应用、完整20KiB NVS及原表，ROM verify通过；COM9未动。覆盖恢复先前同签名、diagnostics=false的匹配APK（SHA`381363fe…ee7f48e`），无需再次构建；最终账号／session／安全存储与全部intents哈希不变，手机mostfun2026／App首页，无worker／forward／wrapper／遗留系统确认。完整证据仅在忽略目录`build/access-ar4c-20261029`。无发码、服务部署、SDK升级或Git提交推送。

### 19.2 官方合同与当前实现的区别（只读分析）

已核官方[使用说明](https://github.com/espressif/esp-idf-provisioning-android#using-provisioning-library)、固定[lib-2.4.4 ESPDevice源码](https://github.com/espressif/esp-idf-provisioning-android/blob/lib-2.4.4/provisioning/src/main/java/com/espressif/provisioning/ESPDevice.java)和[官方手动SoftAP示例](https://github.com/espressif/esp-idf-provisioning-android/blob/lib-2.4.4/app/src/main/java/com/espressif/ui/activities/ProvisionLanding.kt)。

- 第一层发现：手机找待配网设备，SDK有BLE搜索与WiFi AP搜索。用户选定后由`ESPDevice`连接对应BLE／SoftAP。QR与手工建引用也是支持方式，不是协议错误。
- 第二层扫描：会话连接后，`ESPDevice.scanNetworks`要求**嵌入式设备**扫描目标路由器；选择SSID／密码后`ESPDevice.provision`负责安全会话、凭据发送、应用及结果。这不是手机第一层的AP发现。
- SoftAP有两种合法接线：`connectWiFiDevice(ssid,password)`含Android网络请求和绑定；无参`connectWiFiDevice()`直接开始已连接网络上的设备能力通信。官方`ProvisionLanding`也有系统WiFi设置返回后调用无参接口的模式，不能把系统页面本身判为偏离。[Android也要求此类网络请求由用户确认](https://developer.android.com/develop/connectivity/wifi/wifi-bootstrap)。
- 当前BLE选择后仍调用SDK `connectBLEDevice`，后续扫描和下发也仍用SDK，没有重写配网协议。**SoftAP连接获取由我们自建requestNetwork／bind，再调用SDK无参接口**；SDK无参接口又注册自己的WiFi回调。它不是SDK完整连接入口的直接薄桥接，是否必须保留这些额外接管应先与官方最小链路对照，不能仅以当前架构内部测试通过来证明合理。
- 当前App SoftAP只有手填热点入口，尚未接SDK AP搜索列表；系统弹出的WLAN列表不等于App完成了待配网AP发现产品入口。Blinker绑定／Key／管理员与CAS是另外的业务合同，WiFiProv只提供custom endpoint承载，不应混为SDK的网络连接职责。

### 19.3 收敛后的建议，不自动开始新架构

先在现有插件／隔离worker内对照官方完整连接API，最小范围只到“选待配网设备→SDK连接事件→设备WiFi扫描”，不接云身份写入、任务或新功能，也不再扩大IPC／诊断。按开放／加密热点及当前SDK实际支持核对，不能只改一行调用就宣称兼容或已修；同步检查原SDK日志、取消与真实退出边界。若官方完整入口可用，再最小删除重复的网络获取编排；必要的pause／stop、owner／CAS／UNKNOWN、Cloud隔离保护不可直接移除。官方手动模式只作为明确支持路径，而非不断预连／返回重试的默认流程。

本节是用户最新问题的只读分析与前批收尾；未据此实施产品重构或开始新的真机对照。AR-4／#33仍开放。

## 20. AR-4d：官方完整SoftAP连接入口的有限对照（2026-10-29）

### 20.1 本批最小合同与软件修正

用户现授权按§19继续。本批仅原插件／同一隔离worker内“SDK完整连接→设备WiFi扫描”，不接云身份、Key、TaskStore／任务或WiFi下发，不新增自动重试、不升级SDK、不提交推送；既有owner、20s父子截止、pause／true stop、CAS／UNKNOWN和Binder death释放合同不改。

LocalOwner改为一次调用原SDK2.4.4 `connectWiFiDevice(name, softApPassword)`（开放AP为非null空串）。删除自有WifiNetworkSpecifier／requestNetwork获取、onAvailable主动绑定及转调无参入口；配网协议、会话和扫描仍由SDK执行。原SDK完整入口的onLost只打印日志，故保留独立的**被动失网观察**：不请求网络、不绑定、不在available上执行操作，只在同owner／epoch和SDK绑定网络匹配时隔离；观察失败亦failclosed。SDK disconnect不是退出证明，仍须匹配worker死亡。

原共享fixture曾把有参入口转调无参，现两者独立计数／参数快照，不伪造绑定、网络授权或成功事件。17项实际Owner有限host通过，含开放／加密参数原样透传、单次SDK调用、权限、同步失败、20s、被动异网／匹配失网、迟到事件及原UNKNOWN／scan／60s provisioning；不当作Android实机。未重跑不受影响的App／BLE长矩阵。

本次仍是**内部待验候选**，不宣布生产等价替换：SDK会在Log.d打印AP参数，本批真机只用无密码测试AP，SDK原日志保留私有，不向公开诊断转发；加密AP仅host参数测试。SDK已有最多3次只读proto-ver尝试，“一次connect”不代表一次HTTP。CONNECTED之后仍须实际设备scan成功，不能以IP／ready／事件名称独自算闭环通过。

后续只复用已核固件／Web完成一次匹配原生构建和短验，真机结果与恢复另记；暂未据软件通过写成连接已修复。

### 20.2 精确AP明文策略及最终负向保护

在首个完整SDK入口对照仍截止后，核实另一独立配置缺口：beta原来全局禁止明文，而固定SDK的SoftAPTransport以HTTP访问`192.168.4.1`。参照[Android网络安全配置](https://developer.android.com/privacy-and-security/security-config)，仅beta新增NSC：base明文false、单一精确host `192.168.4.1`允许、includeSubdomains=false，无额外CA／debug overrides，Cloud／Capacitor HTTPS不改。这个例外覆盖**beta全进程及该host全部端口／路径**，不是只允许worker／80端口／proto-ver，亦不提供设备认证或隔离释放证明。SDK未到HTTP前，这一缺口不能被称作本次20s的根因。

App原native／配置守卫2项和新增NSC2项分别有限通过；新范围断言另含10个反例，不把反例计成10项业务测试，也未重跑全部11项。实际APK合并manifest及编译XML由aapt2复核：usesCleartextTraffic仍false、NSC精确host／默认拒绝／无CA扩大，不能仅看源码断言已打包。

独立审阅另补实际Owner负向前置：SDK CONNECTED若读取绑定为空或异常，以`WIFIPROV_RESPONSE_UNCONFIRMED`单次拒绝并隔离；非空只用于被动失网关联，仍不能取代同pending事件，更不能当退出证明。新增1项含null／异常及迟到事件、info／scan／custom／provision禁入的合同，最终**18／18 Owner host通过**（含原17，不累加）；最终源码原SDK定向Java编译48s／17 tasks中1 executed通过。它发生在下述实机之后，**未重新打APK／安装或追加实机循环**，不把最终补丁的host／Java结果冒称真机通过。

### 20.3 一次入口对照与一次配置差异短验（跨日至2026-10-30）

本批无Web／固件重复编译队列。先仅原生入口改动构建1m25s（441 tasks／8 executed）、同签名覆盖安装；实际class调用核查为有参SDK入口1处、自有requestNetwork0、无参入口0，被动observer保留。随后只因上述NSC缺口做一次配置差异构建28s（441 tasks／14 executed）、安装并核APK SHA`7c78c4e9…c1e28fa`／77,893,852 B；未改SDK／依赖／锁。前一APK SHA`e46876d9…7a67`／76,518,206 B作为阶段制品保留，不混称同一安装包。

COM11复用§17冻结SoftAP固件，没有重编。新鲜NVS基线与此前完全同SHA；为避开§19已证的热reset竞争，本次冷启动前复用已验WiFi清除快照，严格核仅`bl_v2_net/wifi_0、wifi_1、wifi_sel`及`nvs.net80211/sta.apinfo、sta.chan`五个逻辑键变化，其余类型／payload、实例、access root及TaskStore精确不变；ROM verify后冷启动只读得到Provisioning／error0。**未重发串口n、未清授权或任务。** 配置差异前仅按同MAC冷复位重开已过期的配网窗口，不再次写NVS。

| 有限对照 | 实际结果 |
| --- | --- |
| SDK完整入口、原beta明文策略 | 一次create／connect，pause不提前取消，父RPC约20s拒绝并确认worker死亡；原页面无connected／设备网络列表。早期logcat使用含冒号tag过滤，未取得可靠SDK内部阶段；其零计数及稍后已滚动buffer**不能作为未发生回调的证据**。 |
| 精确AP NSC配置差异 | 一次新create／connect，真实系统NetworkRequest界面点选精确AP一次；7次有界只读UI快照未观察到可接受的“继续使用”或标准连接按钮。按beta UID直接采集原日志，证实SDK有参入口进入；在本次采集窗口内无其exact onAvailable、proto-ver start、unavailable或cleartext拒绝日志。当前首个未完成阶段是**Android指定AP网络获取**，尚不能断言OEM／用户授权／SDK／被动观察中的具体根因。 |
| 截止与回收 | 配置差异同owner8条生命周期：connect请求→父截止恰20,000ms，close-request后14ms取得匹配dead；resume发生在dead之后。两次JS connect均20,013ms返回`WIFIPROV_RPC_TIMEOUT`。无scan／GetInfo／custom／WiFi下发、身份／Key／Cloud写或任务动作，未因返回或ready自动重连。 |

手机关联AP不等于指定worker的NetworkRequest获准；本批没有SDK connect成功，**目标路由器列表点选、SSID／密码草稿、WiFi发送和云上线均未进入**。系统WLAN里的AP选择不是App目标网络选择。原全局account/connection前台检查在无互联网AP下仍有DNS失败，完整日志保留；不把它称为配网identity写入或本批自动重试器，也不宣称全App完全无HTTP。

独立只读时序核对进一步缩小了停点：SDK entry后5ms系统WifiNetworkFactory已收到本次exact specifier请求，20ms启动Oplus NetworkRequestDialogActivity；约2,093ms出现“未注册network request matches callback”的系统记录，**该callback是系统选择UI的匹配回调，不是SDK onAvailable**。约5,100ms精确AP点击只使原行显示“不安全”，七份系统快照均无该行连接中／已连接或继续使用确认，不能把tap认作已完成onUserSelection。父20,000ms先拒绝，之后约42ms才有系统“App released active request, cancelling”与releaseAsUnfulfillable；所以后者是截止关闭后的收尾，不能倒因为用户取消导致超时。它支持先查系统选网UI回调接入，但不足以认定其具体原因；不支持归因为密码、HTTP／NSC、设备端HTTP或bind失败。

**截图补充，不能将上述窗口内缺失外推为弹窗不存在：** 观察脚本在结束时显式拉回Main；其后原ADB截图`run-policy/system-7.private.png`实际可见“不可上网，是否继续使用”原生弹窗，下层已是20s失败页，未点击接受。七份XML只覆盖此前系统选网页，未覆盖这一稍后展示。当前还不能证明弹窗何时创建／是否更早已在另一窗口，亦不能据迟到截图把旧已死owner恢复为有效。后续应先核这次选择→返回Main→系统确认的交接可见性，而不是沿“没有弹窗”假设继续；不得把测试脚本留在系统页至接近截止的观察方式当成功授权流程。

### 20.4 收尾、支持边界及唯一下一步

- COM11已恢复原D4应用`e8a3c4c…adda13`、本批新鲜完整20KiB NVS及原表，逐项ROM verify后复位；root／epoch1／原TaskStore1条revision3精确不变。COM9不动，无scope／UNKNOWN重放或授权清除；不是8MiB逐字节全片恢复或新云登录验收。
- 手机mostfun2026／首页，复用已验同签名diagnostics=false的原APK`381363fe…ee7f48e`覆盖恢复，无再次构建；账号／完整session／安全存储2条及所有现存intent逐项哈希不变。worker、CDP转发／binding／wrapper与本批系统XML均回收，原logcat不清。候选软件／精确NSC源码保留待验，不能将恢复的旧安装包称为已部署最终新修正。
- 私有证据仅在App及库忽略目录`build/access-ar4d-20261029`，包括完整请求／SDK原日志、截图、NVS／写入意图／恢复及独立final audit；SDK日志可能含AP参数，不整体交付或推Git。无验证码、服务部署、自动重试器、SDK升级、提交推送，无关本地修改保留。

**AR-4／SoftAP完整接入仍开放，不再追加同配置连接。** 下一批先核本次系统选网UI匹配回调接入／选择提交、返回Main与无互联网确认的交接可见性，区分“系统收到了request”“展示／点击AP”与“提交选择后建立网络”；沿已有关联时序定位首处分歧后才做最小修正。缺证据则限定补该边界，不扩IPC／身份框架、不加固定等待或放宽后台。只有SDK连接及真实设备scan通过，才补原App目标WiFi选择／取消截图，并回到既有根正规恢复与身份→WiFi→云上线。BLE §17已过连接／扫描不重复，但原根凭据阻点仍不得以删门禁凑通过。

## 21. AR-4e：系统交接时序与有界用户授权预算修正（2026-10-30）

### 21.1 先定位，不移动SDK或新建连接框架

本批按§20.4继续。独立核固定SDK2.4.4及Android请求合同，未找到必须用Activity Context的规定；当前Service并非isolatedProcess，仍同包／UID／权限，实际exact请求已被系统接收并启动选网UI。将SDK搬回Main会同时改变进程网络绑定及Cloud隔离，既无根因证据也不是最小修正，不实施。系统选择UI的特权matches callback不能由普通App仿造注册；不加私有系统API或NETWORK_SETTINGS权限。

先只做一次操作交接差异：复用既有SoftAP固件、原Web及相同NSC，匹配最后的null绑定保护构建／安装一次原生beta（37s、441 tasks中6 executed，SHA`e377b99e…8944d2`／77,893,852 B，同原签名）。仍原SDK2.4.4、Core3.3.11，无新设备固件编译队列、依赖或锁升级。原APK仅同卷hard link引用；新鲜小范围NVS／表及原D4程序ROM核齐，没有8MiB重复备份。

原系统精确AP点选后即显式切回Main，而非留在系统列表至接近截止；全程只一次create／connect，无自动重连。操作脚本只准精确系统AP和同AP无网确认，3项工具测试通过，**只是维护工具回归，不是App产品测试**。未点发送、未scan／GetInfo／custom／安装身份、刷新Key或操作TaskStore／云scope。

### 21.2 实际结果与首处分歧

本次同owner8条生命周期，Main在connect后**4,698ms**即恢复且attached；未发生true stop，仍在20,001ms父截止拒绝，关闭后23ms确认dead。SDK原UID日志有full-connect入口，窗口内仍无exact onAvailable或proto-ver。故§20提出的“上次返回过晚”不足以解释本次失败，不据此改SDK／IPC或忽略后台。

| 相对本次connect | 既有系统buffer／请求证据 |
| --- | --- |
| +15ms／+2,162ms | 系统收到应用exact请求3542；随后报告选择UI matches callback未注册。仍不是SDK NetworkCallback缺失的证明。 |
| +4,921ms／+5,035ms／+5,281ms | exact AP成为network123、DHCP ACK并成为普通默认网络；不等于应用的本地请求3542被满足。 |
| +15,134～17,574ms | network123 DNS探测超时；有无互联网评估，但不能只凭时序认定唯一因果。 |
| +20,003ms／+20,065ms | 原连接拒绝，随后系统释放该exact请求且worker死亡；没有在旧owner上接受迟到结果。 |
| 约+25,080～25,203ms | 同轮OEM评估valid=false、showDialog invalid=true/type3，HTTP／HTTPS探测DNS失败；之后独立XML确有同AP“继续使用”。与约25.1s确认时序吻合，晚于本次20s连接预算。 |

五份操作中XML只观察到约+14.1s，直到20s仍有约5.9s窗口未覆盖；这部分不能由XML断言“无弹窗”。提示延迟的依据是上述另存的系统buffer，不是末次截图倒推。确认节点可被原精确匹配函数识别，无问号／换行／package过滤漏检；未点已过期owner上的确认来凑通过。

另核[Android agent匹配合同](https://android.googlesource.com/platform/packages/modules/Wifi/+/c6e2a6083b342a9196091f17ab5276cac8f4d2b3/framework/java/android/net/wifi/WifiNetworkAgentSpecifier.java)：本次普通网络agent `mMatchLocalOnlySpecifiers=false、band=1`，SDK请求`band=-1`；该分支按band匹配而非仅SSID，所以现有agent不能满足此请求。UID1000／10050的差异不是单独决定性依据。**没有接受系统确认，尚不能断言确认后agent不会重建／变化，也未确定UI callback为何未注册。** 不把“延长预算”宣称成此匹配问题已修。

### 21.3 必要最小修正：区分用户授权预算与普通RPC

20s连接在系统确认约25.1s之前终止，属于已证的预算不相容；但只调时间不能替代成功回调。本批只修这一前置，不新增自动重试、发现／身份协调器或OEM私有适配：

| 范围 | 固定有界预算及合同 |
| --- | --- |
| isolated SoftAP connect | **60s**，仅改原`SoftApWorkerProtocol.timeoutMs("connect")`；父Channel与child Owner共用这一值，不另设独立可漂移常量。SDK真实结果才能完成，时间流逝不产生connected。 |
| 普通read／scan／custom／reset、provision | 原20s／60s不变，BLE原连接20s不变；启动20s与关闭5s不改。写入UNKNOWN及死亡释放合同不改。 |
| App整次SoftAP acquisition | **90s单一整次截止**，覆盖native connect60s及显式继续／新鲜同owner读取余量；pause／resume／continue不重置，不是每阶段再加90s。native较早失败立即沿原对象失败，不刻意等90s。 |
| 允许与退出条件 | 原pause-only仅暂存结果；true stop／Home／离页／换号立即原路径关闭，返回前台本身不继续。原显式continue、上下文／前台检查和fresh owner读取保留；SDK成功／IP／ready均不替代匹配worker死亡释放Cloud。 |

有限回归：App原acquisition 24／24通过，含约25.2s延迟后仍须真实native结果和显式continue、时间／返回不自动读、新的90s整次边界、45s内取消立即及原UNKNOWN／账户／owner保护；native定向16／16（Protocol1、Owner7、Channel5、Client3），覆盖20s／59.999s不能过早拒绝、60s单终态隔离／迟到禁入、25s显式成功与立即取消、其余RPC及startup／close预算不变。不是重跑全部native／BLE矩阵，亦不与§20原18项累计。原SDK最终Java定向编译14s／17 tasks中1 executed通过，保持原工具链和已知Gradle警告；未再打APK或安装预算修正版。

### 21.4 现场恢复与唯一后续范围

本次实机发生在上述60／90s修正**之前**，不能称新预算已安装／真机通过；新修正目前仅源码和有限软件候选。实机完毕已恢复COM11原D4应用、完整20KiB NVS及原表，ROM逐项verify并复位；原root／epoch1及TaskStore1条revision3不变。手机恢复原diagnostics=false APK`381363fe…ee7f48e`／mostfun2026／首页，同账号／完整session／安全存储及全部现存intents精确不变。COM9未动，无清授权、UNKNOWN重放、真实验证码、服务部署、SDK升级或Git提交推送；完整私有日志／图像／请求及恢复记录仅在忽略目录`build/access-ar4e-20261030`，无关本地修改保留。

**唯一下一步：构建／安装60／90s匹配候选，仅做一次同owner系统确认→SDK exact onAvailable／能力响应→显式继续／实际设备WiFi扫描的短差异。** 操作观察须覆盖原有界请求窗口而非再次在14s提前停止；匹配已拒绝／关闭／死亡则停止，不接受迟到确认复活旧请求。仅原系统控件的用户动作，不预连后自动重试，不以等待／ready解除隔离、不忽略真实后台。不重复BLE、身份／WiFi写入／云上线、长循环；若系统确认后仍无应用请求回调，则停止该自动入口对照，按官方手动连接模式另明确最小兼容方案，**不再继续加预算或扩大IPC审计**。AR-4／#33仍未销项。

## 22. AR-4f：60／90s匹配候选短验与自动入口冻结（2026-10-30）

### 22.1 匹配构建与唯一实机范围

严格执行§21.4，未再改产品源码、依赖、SDK或授权策略。仅一次Web生产／cloud-beta构建（17.713s）、原工具sync、原SDK单worker离线APK构建（33s／441 tasks中26 executed）；复用既有COM11 SoftAP固件，无设备重编队列。APK SHA **`84fa3cb72d6c05b8c5efc4f4d19d1ab2cfe7af63b8298b0fb5d2cac388aac88c`**，77,893,852 B，同原签名`a76639ec…5d5d210`、beta包名与HTTPS，安装成功。原样保留样式budget、FontAwesome路径及Gradle警告，不为本批扩大处理。

打包核对不只读取源码：实际source.gradle覆盖的编译消费者`SoftApWorkerProtocol.class`经javap核connect／provision60s、其他20s；APK内唯一acquisition JS资产与www同SHA，定位该类run初始deadline及setTimeout共用90s。实际NSC资源仍默认拒绝、只放行精确192.168.4.1、无CA扩大。两个维护核查错误（先误读pnpm静态目录、后误将finally清空覆盖初始预算表达式）均在发起connect前停止；修正工具再只读核齐，没有重复构建／安装或设备连接。Java消费者检查不冒称通用DEX验证器。

账号、完整session／安全存储及全部现存intents先保留；原APK同卷hard link复用，不重复复制大包。COM11原D4应用、原表及新鲜20KiB NVS经ROM核对，原root／epoch1／TaskStore1条revision3相同；仅复用已严格核五个WiFi键差异的冷配置，不重发热resetNetwork，不清授权或任务。COM9不动。

操作工具覆盖同一个原60s请求窗口，不再固定五次读取；每次原生控件动作前重读同session／request日志并核设备时钟，拒绝终态／换代／超时；这些只是负向护栏，日志末条pending不充当原子活性或连接证明。9／9纯工具测试通过，不算产品／真机测试数；§21的24／16软件结果冻结、不重复累计。

### 22.2 实际结论：确认已接受，但专用SDK请求仍未完成

一次真实Page create／connect、一个新隔离owner，无true stop，无第二次连接或原请求重放。

| 环节 | 本批证据及边界 |
| --- | --- |
| 系统交接 | connect后约2.9s已回Main。约26.5s点同AP原生“继续使用”；独立设备系统日志另在约26.4s记录positive button及dialog dismiss，确证按钮动作执行。电脑与设备时钟不直接精确相减。 |
| 确认后的请求 | 约32.6s与51.6s系统仍报告network request matches callback未注册；原专用请求3593持续active直到App截止后释放。确认按钮动作不是SDK专用请求已获准／已满足的证明。 |
| SDK阶段 | worker原UID日志有full-connect入口，无exact onAvailable／proto-ver／能力响应；没有getDeviceInfo／scan／custom／WiFi下发、身份／Key／Cloud写入或任务动作。实际设备网络列表仍未进入，不能销项。 |
| 有界失败与关闭 | native单一时间轴connect后**60,002ms**拒绝，close-request后**19ms**确认dead；JS返回`WIFIPROV_RPC_TIMEOUT`，约60,016ms。App90s总保护没有伪造成功，也没有等待90s才处理较早native失败。 |

本批system buffer没有完整的WifiNetworkAgentSpecifier字段，**不得借用§21的agent字段断言本次接受后一定未变化**。当前只确认系统正向动作已执行，而应用专用请求及SDK连接没有完成；选择UI callback为何未注册仍未定位，不能称所有Android／所有SoftAP都失败。

SDK原UID采集在五分钟工具上限以exit1结束；文件13,320 B，未触4MB上限，实际覆盖本次create→connect→pause／resume→拒绝→close→dead全部8条事件，最后为实际dead。该工具退出不算产品根因或本次60s窗口欠证，不另开重复采集。另存既有系统buffer、完整HTTP／native日志、原窗口XML和实际失败页截图`run-budget/system-8.private.png`，未修改图片凑成功。

**当前PGEM10/Oplus组合下自动SoftAP入口未通过，自动入口对照到此冻结。** 不再增加预算，不围绕同一请求做重复循环，不用SSID／IP／ready／系统确认取代SDK结果；BLE既有连接／扫描不重跑。AR-4与报告#33仍开放。

### 22.3 现场收尾

- COM11原D4程序`e8a3c4c…adda13`、本批新鲜完整20KiB NVS及原表逐项ROM verify恢复并复位；身份／root／epoch1／TaskStore1条revision3精确不变。不是8MiB逐字节全片恢复，也不冒称新一次云登录。
- 手机恢复mostfun2026／首页及原diagnostics=false APK`381363fe…ee7f48e`，实际安装SHA核同；账号、完整session、安全存储和所有现存intents哈希不变，UNKNOWN不动。worker、CDP转发／wrapper／binding、采集器及本批系统XML均回收，原logcat不清。60／90候选本批已装并测过，**目前现场已恢复旧APK**，不要混称。
- 私有证据在App／库忽略目录`build/access-ar4f-20261030`，含账号／授权／NVS及完整原日志，不能整体交付或推Git。没有验证码、服务部署、SDK升级、自动重试器、Git提交推送；无关本地修改保留。本批仅必要维护工具与文档，不把验收失败直接重写成新的产品功能。

### 22.4 下一软件包：官方手动连接的只读最小兼容合同

这是下一步**待实现**合同，不等于手动模式已经可用或完整SoftAP首次入网已经通过。继续复用原WiFiProv、原SDK2.4.4和同一隔离worker，不能回Main绑定网络／重造协议／加入OEM特权接口。

1. **显式选择，不自动fallback。** 计划在现有`ConnectOptions`增加`softApConnectionMode?: 'sdk-managed' | 'system-connected'`，缺省保留原有参SDK入口；仅显式system-connected才调用SDK无参入口一次。同一owner首次connect即固定方式，失败后必须先死亡、再新generation／新owner，不能在旧请求里偷偷切换／重发。新增小能力标识，旧插件不识别时前置拒绝，不能静默忽略枚举。期望SSID仍复用原createDevice.name，不增加任意URL／绑定参数。
2. **系统连接是用户操作，不是成功证明。** 用户先在系统WLAN连指定AP、返回后明确继续，再创建新worker；进入完整设置若触发true stop，仍原样取消旧页，不把旧acquisition／stages复活。IP／ready／勾选／返回本身不授权协议操作。
3. **SSID与绑定只作负向校验。** SDK无参入口本身仍注册泛WiFi回调／可能绑定任意WiFi，并非纯粹attach已确认网络。原worker必须先新鲜观察期望WiFi／可关联网络，未知、隐藏、非WiFi、不符均拒绝；SDK CONNECTED时再核实际绑定非空及同一网络／SSID，变化则关闭，保留现有null guard。即使一致仍须真实SDK事件及设备scan，不把SSID当设备身份或Cloud释放证明。
4. **权限最小对齐。** 原插件Fine／Coarse声明仅到API32，但当前App已由main manifest移除maxSdkVersion，不能误报本App33+一定未声明。下一包须显式核system-connected的实际Fine Location授权及系统定位服务，而不是沿现有“非必需location显示granted”当真实授权；独立插件消费者的33+声明边界也应明确。拒绝／SSID不可见时failclosed，不增加模糊定位或其他SDK。
5. **第一包只读。** 仅SDK连接／元数据／设备WiFi扫描和原工具页最小显式入口；system-connected暂不允许custom／身份／Key／reset／WiFi写。60／90、真正后台／取消／失网、迟到禁入、死亡释放及UNKNOWN保护不改。复用Protocol、Owner、Channel、acquisition及现有日志，不新增协调框架。
6. **完整Cloud／Local手动重入另列后续合同。** 当前true stop会abort整个stages，不能为了去系统设置而放宽后台或恢复旧await-local；完整写入必须在确认公开handoff意图／旧owner死亡后，以新generation及原instance／epoch／root新鲜核对另行设计，UNKNOWN仅只读。该后续不并入首个只读兼容包，也不冒称已解决原根凭据问题。

最小回归只覆盖枚举／旧能力拒绝、真实权限、SSID未知／不符、绑定为空／换网、SDK单入口、只读禁写及取消／超时／迟到结果；原SDK定向编译后再决定一次匹配只读实机。**下批先实现这个软件包，不再派发60s自动入口、BLE、身份写入或资源审计循环。**

## 23. AR-4g：显式手动连接的只读软件兼容（2026-10-30）

### 23.1 本批实现与受限合同

按§22.4实施，不另建配网插件、SDK transport或Cloud协调器。**本批仅软件及原SDK定向Java编译；没有安装APK、刷板、真实配网或操作账号／授权／UNKNOWN，不能称手动实机通过。** §22的自动入口失败结论冻结，未再试连或增加60／90s预算。

| 模块 | 最小接线与拒绝边界 |
| --- | --- |
| 能力与模式 | `softApSystemConnectedReadOnly === true`才开放，Android API31+；旧／false／非布尔能力前置拒绝。ConnectOptions及权限options使用`softApConnectionMode: 'system-connected'`，缺省仍sdk-managed。只限SoftAP，非法枚举／BLE混用拒绝；iOS／Web明确不支持，不静默降级。 |
| 权限 | 手动模式请求并核实际Fine／Coarse，API33+另需Nearby WiFi；worker再次核权限及Location服务。原插件位置声明不再maxSdk32，当前App原tools:remove保持；managed仍只请求原传输／OS所需aliases。不把非必要字段显示granted当真实SSID观察权限。 |
| 当前网络观察 | 新薄helper `SoftApSystemConnectedWifi`不申请、不绑定、不扫描手机网络；只接受唯一可观察WiFi且它是当前default／active，拒绝VPN、secondary／多WiFi、未知／异常capabilities。读前后active一致，SSID按Android带引号UTF8格式精确比较createDevice.name，不trim合法SSID、不接受unknown／裸hex。权限／服务／网络不明均failclosed，不返回网络标识或OEM异常原文。 |
| worker／SDK | 首次connect固定方式；手动前置失败也隔离，必须真实死亡后新owner，不在旧owner里转managed／重发。仅显式手动调用原SDK无参入口一次；SDK CONNECTED时保持非空binding保护，再新鲜核原Network／SSID和实际绑定一致。操作前、网络事件及扫描回调再次观察，已观察到的变化隔离且不交付其结果。 |
| 只读权威 | 手动worker的custom、reset、provision直接RPC也在SDK前拒绝。父Channel保留明确只读错误，不将确定拒绝误改为UNKNOWN；其余不确定写仍保持UNKNOWN，post-SDK RESPONSE_UNCONFIRMED不加入安全拒绝白名单。只开放连接、原metadata及设备WiFi扫描。 |
| App薄入口 | 原工具页新增显式“已连接热点，只读查看”，旧能力／操作中禁用；回到App本身不调用。复用原startOver／release，页面代次先使旧回调失效，旧worker关闭确认后才捕获新上下文、create新worker；代次失效不充当死亡证明。点击时账号与SSID先快照，关闭等待／权限等待期间换号、换服务或离页均不得转用新上下文继续。重新配网路由不开放此只读入口。 |
| 页面只读 | 手动不创建Cloud／Local stages；禁止选择网络、记住草稿、身份／管理端口／custom薄端口及发送。结果页只显示设备扫描列表和刷新，隐藏身份名、目标SSID、密码、记住与发送控件。失败后退休原会话，须显式重新开始；权限／SSID／绑定失败提供对应提示，不覆盖UNKNOWN。 |

原暂停例外、true stop立即关闭、失网／换号／离页取消、迟到结果禁入及死亡释放合同不变。完整系统设置若导致stop，旧页仍取消；返回后先点原“重新开始”，再点击只读入口，不能复活旧stages。UI技能仅用于明确只读／显式动作、禁用原因、48px次级按钮与原主题样式，不新增页面／自动系统跳转或新的用户授权动作。

### 23.2 网络安全证据的准确边界

依据[WifiInfo.getSSID](https://developer.android.com/reference/android/net/wifi/WifiInfo#getSSID())及[WifiManager.getConnectionInfo](https://developer.android.com/reference/android/net/wifi/WifiManager#getConnectionInfo())，SSID可能因权限而隐藏，同App并发STA也可能使观察落到secondary；因此采用上述单WiFi／default保守合同，不假定一次SSID快照就是身份或原子锁。

原SDK无参入口仍有异步泛WiFi回调、绑定及HTTP。此次未修改SDK／注入transport，**不宣称在SDK内部“绑定→发HTTP”之间建立了原子网络锁，也不宣称绝不会在未观察到的瞬间换网上发出只读协议请求**。新鲜前后观察用于排除已证不匹配／丢弃其结果；本包无身份、Key、WiFi写入且不把观察结果转换成写权限。SSID不证明设备身份，SDK结果不证明Cloud隔离已解除。

### 23.3 有限回归、实际修正及构建

- 原生host：新增手动Owner／helper **9项**，受影响managed **6项**；真实桥接 **11项不同用例**，Protocol **2项**，Channel **3项**通过，共31项不同host合同，不当作OS／手机／网络执行。另Web实际运行2项、Swift不支持路径源码静态1项单列，未宣称iOS编译／真机。插件TS noEmit通过。
- App：手动Page **13项**、原权限 **8项**、受影响acquisition **9项**、原模板新增 **2项**，分批32项不同用例通过；后增的扫描失败退休、等待退休时换号及错误态入口提示只定向运行，不重复累计。模板的原20项筛选跳过是本批不重复执行，不填成通过；API／native边界均为测试替身，不算真机或375px／横屏／系统大字视觉验收。
- 初次App源路径映射使Angular测试bundle内联插件，`vi.mock`未生效，旧用例也在准备阶段失败；仅testing buildTarget显式externalize `capacitor-wifiprov`，与测试runner同源alias对齐，未改生产构建／锁或依赖版本。随后暴露的五条权限fixture缺旧账号上下文、两条仍推进30s的旧断言分别补齐真实测试前提／已批准90s合同，不修改产品授权或预算来凑通过。原失败日志保留，修正后只重跑受影响项。
- 新回归实际修正三处App边界：手动不再透传旧AP密码；手动扫描失败走原suspend／release而非允许刷新旧owner；点击账号在等待旧worker退休前先快照，返回后不能静默改用新账号。UNKNOWN对象、CAS、原授权／journal消费者不改。
- App最终源声明TypeScript noEmit通过；原SDK2.4.4实际源码override的Java定向编译 **56s／17 tasks中1 executed，BUILD SUCCESSFUL**，最终helper／Owner／Bridge消费者已编译，保留原getConnectionInfo弃用及Gradle警告。没有升级SDK、完整APK构建／安装或设备固件编译。

本批日志分别在App／插件忽略目录`build/access-ar4g-20261030`，原真机私有日志／授权材料未删改；没有ADB／串口现场操作、服务部署、真实验证码、自动重试或Git提交推送，无关本地修改保持。新增helper均有实际消费者，未新增无用兼容框架，也不为清理移除现有UNKNOWN／身份／旧BLE分支。

### 23.4 唯一下一步：匹配手动只读短验

先构建匹配当前TS／Java、能力标识与API31+权限声明的beta，核原SDK2.4.4、NSC精确AP及包名／签名；保留原账号与完整请求日志。仅COM11复用既有配网候选，不重编设备、不清授权或TaskStore；按需核原小范围恢复材料，不做无必要全片备份。

用户在系统WLAN连接指定热点并接受无网确认，返回后显式重新开始／只读查看；新owner只调用SDK无参入口。核能力／真实权限、同SSID／原网络及非空绑定、实际SDK能力响应／设备扫描；网络选择与身份／WiFi发送必须不可用。只补一次真实离页／后台回收，结果迟到不继续；未成功即记录首停，不自动retry／切回managed或再扩观察范围。

结束恢复原App、原固件及明确范围NVS／授权，保留UNKNOWN和完整日志；仍不接Cloud／Local手动重入、身份／WiFi写入、BLE循环或SDK升级，不提交推送。**只读候选通过也不能关闭AR-4完整入网或报告#33；下一阶段写入合同另行确认。**

## 24. AR-4h：匹配手动只读候选真机短验（2026-10-30）

### 24.1 本批范围及匹配材料

按§23.4，仅COM11 XIAO ESP32S3复用既有SoftAP候选，COM9及其他板不动；没有重编设备、操作服务、发验证码、身份安装、WiFi配置下发或任务动作。产品源码及SDK未再修改，§23的软件回归冻结，不重复累计为本批通过数。

唯一Web生产／cloud-beta构建17.118s、原sync工具、原SDK离线APK构建1m22s（441 tasks中29 executed），同原签名覆盖安装一次。实际安装APK SHA **`bcc43394a4961dc78f0e4e6403f01ece669d944af68e986a8be96e07354b0de7`**，77,893,852 B；包名beta、HTTPS、原SDK2.4.4及实际源码override消费者一致。核对编译class及APK内能力／无参入口／只读禁写标记、实际手动模板资产和www同SHA；原NSC默认拒绝、只放行精确192.168.4.1，无额外CA。原60s native／90s App总截止不变，诊断仅本批私有配置开启。

原App APK采用既有文件同卷hard link保留，不重复大包备份。COM11原D4程序`e8a3c4c0…adda13`经ROM核同，原表及新鲜20KiB NVS另存校验。新鲜NVS SHA **`9b6d4ac1e56b90876a09ffe04c0ec55a6563375c3ca4f2ce8688fc04c50f259a`**，不能用旧批次NVS替代恢复基线。旧冷配置除五个WiFi逻辑键外还含旧PHY校准，最初严格审计拒绝，未写入；离线按相同跨度完整复制本次三个PHY原记录及CRC／索引，再经解析、五键白名单和独立摘要核对，派生冷配置`b37ca6e9…ae761`。其余逻辑键、实例／root／epoch1／controller及TaskStore一条revision3保持一致，未扩大白名单或重发热reset。只写已核应用及冷配置，写后ROM verify，不作8MiB备份循环。

### 24.2 真机结果与准确边界

手机PGEM10／API36，本次实际user0 Fine／Coarse／Nearby均已授权，系统定位开启；没有撤销权限或测试拒绝弹窗。不能把另一用户profile的拒绝记录混入本次，也不以插件兼容字段代替真实SSID权限。系统WLAN实际连接指定COM11热点，取得192.168.4.2；此批未再出现需点击的“继续使用”对话框，没有冒称本批重新验过该按钮。

| 短验 | 实际结果 |
| --- | --- |
| 错误SSID | 手机仍在普通WiFi时，原Page一次显式手动connect，**26ms**返回`WIFIPROV_SYSTEM_WIFI_UNVERIFIED`；无设备扫描／身份／WiFi写入，原owner关闭确认dead。未切换managed或重试原请求。 |
| 正确热点及绑定 | 系统连接指定AP后返回App，点原“重新开始”，再显式只读查看；新owner与负向不同。实际SDK日志只有一次无参入口、泛WiFi available及proto-ver，无有参managed入口；connect **243ms**成功。结合已核实际消费者，可确认本次通过原Network／SSID及非空同网络binding检查；不是SSID证明设备身份或原子换网锁。 |
| 真实设备扫描 | 同owner一次`scanWifiNetworks` **3768ms**返回，页面显示**10个网络**，全部disabled、无selected；显示“手动只读连接／设备扫描结果（只读）”。身份、目标SSID、密码与发送控件均不存在；native写类调用为0，无身份／Key API写入。未再次点刷新制造样本。 |
| 真实后台及返回 | 连接并扫描后按Home。原native时轴pause→close-request **8ms**，close-request→dead **99ms**，随后真实stop；返回记录仍dead／未attached，Page提示手动重新开始，connect／scan仍各1，没有自动重连。不是等待、ready或清UI解禁。 |
| 离页范围 | 后续通过原路由进入首页，Page无连接／列表／手动入口，采集及owner已回收。**本次活连接先由后台退休，未独立再连一次测活连接离页；不重复连接凑矩阵，也不据此重复销项。** |

初次维护观察器在路由尚未渲染时点选控件失败，记录保留；当时create／connect均0。重新打开观察器后才执行上述一次负向，不计为配网自动重试或产品失败。恢复APK后首次只读快照遇到WebView尚未就绪，未取得账号文件、没有产品调用；WebView已出现后重新读取成功，不重装或重连。

日志仍见既有全局`/api/v2/account/connection` GET／OPTIONS，在无网热点下DNS失败（约20.2／22.6s），另一次于返回后仍在采集窗口内pending。**“无自动重试”仅指本次配网链路，无第二connect／scan／写请求；不得宣称全App无Cloud HTTP或这些全局查询已隔离。** 没有借此放开Cloud／Local写门禁或修改全局查询节拍；完整手动入网仍须另行明确Cloud交接。

### 24.3 恢复及保留证据

- COM11原D4应用、新鲜完整20KiB NVS及原表分别ROM verify后复位；恢复前投影确认实例、root／epoch1、授权及TaskStore1条revision3精确不变。NVS恢复校验是复位前的逐字节证明，不冒称复位后网络运行不再产生任何NVS更新，亦不称8MiB全片恢复或新一轮Cloud登录验收。
- 原diagnostics=false APK **`381363fe…ee7f48e`**已覆盖恢复且核实际安装SHA；手机回mostfun2026／首页。before、installed、final三份证明的账号、完整session、安全存储及所有现存intents哈希一致，UNKNOWN不改、不重放，原请求／HTTP／native／logcat完整保留。
- 实际成功页、后台返回页及系统截图保存在App忽略目录`build/access-ar4h-20261030/run-system-connected`；正向原图`system-7.private.png`。对应库私有材料位于库同名忽略目录，含原NVS等秘密，不能整体交付或推Git。采集器、CDP wrapper／binding／转发、worker及本批系统XML均回收，未清logcat或删除业务记录。
- 本批只新增必要维护工具与本节文档，无Git提交推送、SDK升级、服务部署、自动重试器或无关修改清理。两次有意区分前提的显式连接不合并成“失败自动fallback”。

### 24.4 当前结论与后续边界

**Android当前组合的system-connected手动只读连接／扫描及真实后台回收有限通过。** §22自动SDK入口失败继续冻结，BLE不重跑；本结论不关闭AR-4完整首次入网、旧根管理员凭据前提或报告#33。

下阶段如继续完整SoftAP接入，先只明确系统设置往返后的公开handoff意图、旧owner真实死亡、新generation及同instance／epoch／root新鲜核对、Cloud身份链恢复和UNKNOWN只读协调；另外列明上述全局账号查询在Local阶段的处理边界。复用原WiFiProv／身份链，不从只读成功直接开启custom／身份／WiFi写权限，不以等待或ready解禁。本批不擅自启动该实现或新增写入真机测试。

## 25. AR-4i：完整SoftAP交接合同与最小安全接线（2026-10-30）

### 25.1 先冻结支持边界与本批范围

完整目标仍是原WiFiProv连接→原设备GetInfo→Cloud身份步骤→新Local会话复核→身份／WiFi下发→原设备确认→Cloud上线核对，不新造身份执行器。**本批先实现交接安全前置，不把手动只读连接直接升级为完整手动配网。** 当前system-connected在App及原生均禁止custom／身份／WiFi写，SDK元数据不是设备GetInfo；24B设备GetInfo仅含instance、epoch与Key／root状态标志，没有root／controller指纹，不能宣称同epoch即同一密码学根。既有根的管理员凭据及已确认ACK仍由原授权链处理，缺凭据不绕过，不用`WiFiProvAdmin.resume()`冒充UNKNOWN只读查询（它可能轮换／注册／Bootstrap）。

| 交接 | 固定合同 |
| --- | --- |
| Local→Cloud | 正常身份链原GetInfo建立基线；只有匹配owner的原clearState真实死亡完成后，才能进入显式Cloud等待。用户继续仅表达意图；ready、系统SSID／IP、等待时间及legacy marker=false均不能生成死亡或写权限。 |
| 系统设置／真实后台 | 无例外保活：仍取消旧generation、所有旧await与owner。仅当已完成死亡且正等待显式Cloud／Local切换、无在途RPC时，保留单次、内存内、不可从序列化数据导入的公开回执。回执不含Key／Admin secret／密码／payload／Promise／回调；不保存旧continuation。离页、销毁、换号／服务不保留回执。 |
| 显式新一轮 | 返回不自动恢复；原“重新开始”建立新generation／账号上下文，连接仍须显式触发。原同instance、epoch、Key／root存在状态及能力标志必须由新owner的真实GetInfo重新核对，已确认云端logicalId／credentialVersion／locator仍按原CAS保护；不符即在新身份API／意图／写入前停止。该公开状态比较不是root指纹认证。 |
| UNKNOWN | 在途写、关闭未知、失败或取消后的旧阶段不发行可续写回执；保留原journal／requestId／UNKNOWN，只读协调，不自动重发CREATE、轮换、Bootstrap或WiFi。回执本身不是写许可证。 |
| 全局账号连接查询 | `/account/connection`唯一生产HTTP provider在DeviceV2Service，失败由原account-client退避重连，非新增轮询。SoftAP create前同步取得独占Local租约，暂停原client及凭据刷新／重连，取消该provider在途HTTP并拒绝迟到结果。只有该租约对应原生关闭完成才释放；释放不start、不补发被抑制的工作，不reset账号store／presence或UNKNOWN。 |

全局边界本批只覆盖上述凭据查询及其原client，不冒称取消全部App HTTP。HTTP取消阻止该订阅后续401重放，但已经启动的共享Auth refresh可能仍由其他订阅继续，不能称全局Auth请求已撤销；不改Auth刷新策略、验证码或其他功能。既有主进程SoftAP sticky危险标记不能被本批租约释放覆盖。手动只读模式的两层禁写和§22自动入口冻结保持不变。

实施最小范围：复用原Stages导出死亡后的公开回执，Page真实后台取消并在显式新轮GetInfo前校验；共享小型Local租约接Page与原账号client/provider。仅补这三处的有限回归及类型检查，不部署、刷板、发码、升级SDK或提交推送；实际结果见本节后续记录。

### 25.2 最小接线实施与有限结果

1. **Stages回执。** 在原阶段适配器增加`captureHandoff()`与单次消费校验。仅当前owner已真实关闭、当前仍等待用户切换时发行，失败／关闭未确认／在途操作均不发行；回执全部字段冻结、instance转固定hex，拒绝全零instance及access-state／epoch不一致。模块内WeakSet记录发行与消费，不接收JSON／复制对象，不跨重载持久化。后台仅允许context-only校验后读取这份公开状态；真实请求、continue与Cloud操作仍使用原严格前台校验。
2. **Page显式新轮。** 真正后台前保留合格回执、账号／authority／sessionEpoch及已有确认的云端CAS；旧generation、stages、Promise、请求与owner一律取消。回执不保留Key等秘密，已有临时allocation中的Key清空并丢弃对象，原持久意图及rotation／UNKNOWN引用不清。返回不做任何恢复调用；原“重新开始”只建立新一轮，不能消费回执。下一次用户显式连接／发送时，新owner真实GetInfo与原公开状态、账号及CAS匹配才消费；失配在新的身份API、prepare或写入前取消并关闭。原分配／授权执行链未替换，也不调用Admin.resume当作只读协调。
3. **全局连接查询。** 新薄`WiFiProvCloudBoundary`不认识SDK、UI或网络ready，仅管理单owner Local租约。Page在create前同步enter，原DeviceV2Service暂停account-client并取消在途凭据HTTP；所有start／presence需求／刷新／重连入口受原client同一suspended门禁约束。缓存及presence目标保留，已有session按原close语义退役，不reset store或UNKNOWN。对应Local关闭后只解除门禁，不start、补发或改退避节拍。
4. **复核修正三处边界。** 租约只接受本页create结果中已验证的隔离owner，缺元数据／create未知不能拿通用clearState结果释放。关闭后另用独立有界只读信号核native sticky危险标记；true／未知或本页已有危险标记仍禁入，false只是附加排除条件，不替代匹配死亡。无本页租约且另一页持有边界时，旧页清理不得调用clearState干扰对方。原client在ready／retrying同步通知之后及timer回调内再核generation、suspended与timer自身token，避免消费者同步进入Local后创建残留定时器，并在释放后自动触发旧GET；退避与expiry数值未变。

唯一有限队列首组**5文件60／60**通过；复核修正后仅定向**4文件15／15**通过（45项筛选跳过，不算通过），合并为**67个不同用例**，不是75项。覆盖回执17项、Local边界3项、原client暂停／HTTP取消、原Page新上下文与CAS／根状态失配、legacy禁入／跨页清理及受影响的手动只读／pause-stop路径。新增UNKNOWN引用保护用例仅证明不清既有引用，不冒称新的真实UNKNOWN协调成功或密码学同根证明。原阶段正常执行链、原native只读禁写及物理死亡实现未改，主机替身不充当OS／无线栈真机结果。

匹配插件源码声明的App TypeScript noEmit最终通过。首次私有检查配置未设置rootDir，入口落在配置目录之外而失败；只修维护配置后通过，原失败日志保留，没有改生产类型／协议来掩盖问题。必要diff检查无空白错误，原LF／CRLF提示保留。日志在App忽略目录`build/access-ar4i-20261030`；本批无APK构建／安装、native／设备重编、ADB／串口、服务部署、账号变更、验证码、SDK／依赖／锁版本升级或Git提交推送，无关本地修改保留。

### 25.3 当前停点与唯一接续范围

**交接安全前置及全局账号查询隔离已软件有限通过；完整手动SoftAP入网仍未开放。** 普通managed原身份链继续受原门禁保护；§22本机自动入口失败不重跑，§24只读真机结果不重复验收。现场App、COM11固件／TaskStore／授权及原日志未操作。

若继续下一批，先在现有WiFiProv内定义手动模式读取固定`blinker-config/GetInfo`的窄能力／精确报文合同，不开放任意custom、不把SDK元数据当设备身份；保留不确定结果的原UNKNOWN分类。通过后再单独明确新owner的显式写入阶段和已有根授权条件，复用原分配／Admin／CAS／意图库。不能从本批回执或公开instance／epoch／flags比较直接推出写许可。上述接续尚未实现／部署，不把AR-4完整入网或报告#33关闭。

## 26. AR-4j：手动模式固定GetInfo窄读取（2026-10-30）

### 26.1 精确能力与报文合同

复用原WiFiProv、SDK2.4.4、隔离owner及设备已有协议，不新造传输或身份执行链。新增可选能力`softApSystemConnectedConfigInfoRead`；仅匹配Android API31+且已有手动只读能力时接受，缺失／false／非布尔均不支持本读取，不降级为任意custom。Web／iOS明确false并拒绝，不能无声改用其他通道；已有旧版只读扫描兼容行为保留，不冒称它已读设备身份。

| 项目 | 唯一允许值及失败边界 |
| --- | --- |
| 公开API | `WiFiProv.readBlinkerConfigInfo()`，无参数；Bridge／IPC拒绝额外字段，不能让调用方提供endpoint、payload、encoding或owner。仅当前已连接的system-connected隔离owner可发起，无BLE／managed／Main进程fallback。 |
| SDK请求 | 原`sendDataToCustomEndPoint`一次，固定端点`blinker-config`、固定两字节`02 01`（Base64表示`AgE=`，不是调用输入）。原20s整次RPC截止、实际权限／原Network／精确SSID／非空同网络binding及回调校验不变。原`sendCustomData`即使传入同样GetInfo也继续拒绝。 |
| 成功响应 | 恰好24B：`02 01 00 flags instance[16] epoch[4,BE]`；仅flags低3位，instance非全零，hasAccessState与epoch>0一致。含义复用原GetInfo decoder，不含Key／controller secret，不是密码学root指纹或写许可。 |
| 明确设备拒绝 | 恰好3B：`02 01 status`，status仅1～8；沿原decoder保留设备拒绝，不把StorageFailure等当空身份成功。仍不进入分配／Admin或写入链。 |
| 返回及关联 | 仅canonical Base64与主进程根据当前匹配live Binder注入的`localOwner`；不信任child自带owner。App再次核原session、当前账号／代次及精确报文，原待核交接只作不消费的比较，读取不会恢复旧continuation或消费写前回执。 |
| 不确定结果 | 复用原opaque SDK的保守UNKNOWN分类：新方法在IPC的potential-write分类中保留，仅用于取消／未知语义，不授予语义写权限。SDK失败、超时、取消、换网、畸形／未知响应仍UNKNOWN／隔离，不加自动重试或扩大安全拒绝白名单。 |

源核对：设备endpoint的GetInfo只load Key／epoch并清临时Key，不replace、Bootstrap或写持久身份；但ESP32 WiFiProv适配器在已有Key的成功GetInfo后会设置RAM `keyInstalled_`重配门闩。因此不声称查询完全无副作用，App与原生仍必须禁止任意custom、身份、provision及reset，不能用该门闩或GetInfo成功当写授权。

本批最小实施为窄原生入口／能力、原SDK固定请求与严格响应、跨平台显式拒绝、App只读helper及手动连接后的可协商读取；只补有限回归及原SDK定向编译。无UI新页面、服务／设备写入、部署、验证码、SDK升级或Git提交推送；实现及结果见后续记录。

### 26.2 实施、实际修正与有限结果

- **原生窄入口。** Bridge只接受空options，API31+、当前live隔离owner才转发，无Main／BLE fallback。Protocol仅新增固定方法名及空字段合同，原20s与opaque潜在写UNKNOWN语义保留；Owner只对该方法例外放行，必须manual／已连接且通过原网络与binding检查。SDK固定发送`blinker-config`／`02 01`一次，回调仅复制3B或24B再交原actor，避免持有SDK可变数组；任何其他长度／畸形响应进入原UNKNOWN／隔离。返回仅Base64，parent去除child owner后由当前匹配live Binder注入；stop／换代／缺标签的迟到结果不成功交付。原Channel安全拒绝白名单未扩大，原任意custom／身份／WiFi／reset门禁保持。
- **App薄读取。** 新独立helper先核新能力、方法存在、已连接的隔离owner及当前账号／代次；仅调用一次无参方法，复用原canonical Base64函数与GetInfo decoder，返回后再核同session及24／3B精确合同。明确设备status保持原BlinkerConfigError，原生UNKNOWN对象原样抛出，无custom降级／重发。只有新能力严格true才在手动connect之后、scan之前执行；旧能力缺失继续既有只读scan，不伪称已读取设备上下文。原待核handoff仅比较公开状态及账号／CAS，不消费回执，不创建Cloud／Local身份阶段、不进入Admin或写入链。
- **回归实际修正。** 固定读取使旧connect可能跨后台悬挂，审阅发现原`finally`无条件清manualPreparing可能覆盖新一轮准备标记。已用每次connect独立preparation token限定两个清理点；真实Page用例将新run停在原clearState，再交付旧读取，核新busy仍保持，最后仅新run扫描。没有依赖等待、ready、额外重连或扩大后台例外解决。
- **有限回归。** 原生host **14个不同合同方法**：Owner5、Bridge6（含受影响能力核查）、Protocol2、Channel1，通过；其中循环故障样本不重复计数。App分批**30个不同用例**通过：固定读取helper22、Page新增7、handoff非消费1；末批受影响两项复验不累加。覆盖无参／能力拒绝、24／3B响应、canonical／零instance／epoch／flags／owner失配、原UNKNOWN、迟到／stop、新旧代busy及原两层禁写。另Web实际运行1、TS声明静态1、Swift源码静态1通过，**不冒称iOS编译或任何真机**。
- **检查与原SDK编译。** 插件与匹配源码声明的App TypeScript noEmit均通过。App初次测试fixture漏填PlatformCapabilities既有必填字段而编译失败，仅补齐fixture后通过，未放宽产品类型或门禁；失败日志保留。原SDK2.4.4定向Java编译一次，**40s／17 tasks中1 executed，BUILD SUCCESSFUL**；实际源码override目录的Bridge／Owner／Protocol class已更新，javap核到新固定方法及响应校验消费者，不误用pnpm缓存声明或称APK已打包。保留原flatDir、SDK XML及Gradle警告，不升级工具链。

日志在App忽略目录`build/access-ar4j-20261030`及插件`.build/ar4j-20261030`，测试沿用既有JDK／Android36类型包、真实JSON与SDK2.4.4缓存输入，不下载或升级依赖。必要diff无空白错误，原LF／CRLF提示保留；无APK构建／安装、ADB／串口、设备／服务部署、发码、账号／授权／TaskStore／UNKNOWN或原请求日志改动，无Git提交推送，无关本地修改保持。

### 26.3 当前停点与唯一下一步

**固定GetInfo窄读取及软件／原SDK编译有限通过，尚未安装或实机验收。** 当前已部署App和COM11均未变，不把源码新能力混称现场已启用；§24只读连接／扫描和§25软件交接结果冻结，不再为本功能重跑旧矩阵。完整手动Cloud／Local身份、已有根授权及WiFi写入仍未开放，AR-4与报告#33不关闭。

下一步先做一次匹配内测App与既有COM11配网候选的新增固定读取短验：核新cap、原owner标签、实际24B上下文或自然明确拒绝，确认原custom／身份／WiFi仍不可用及退出回收；不为补样本故意损坏存储、不重跑BLE／完整旧扫描循环。实机通过后再单独明确显式写入阶段及已有根的授权合同，复用原身份／CAS／UNKNOWN链，不以此次公开信息读取或RAM门闩授予写许可。本批不自动开始该部署或硬件队列。

## 27. AR-4k：匹配固定GetInfo的COM11只读短验（2026-10-30）

### 27.1 范围、匹配构建与现场保护

本批按§26.3只验新增窄读取，不重跑BLE、自动SoftAP或完整入网。保留原App账号、完整session、安全存储、全部现存请求意图库及logcat；原APK引用经SHA核同，无清数据／登出／发码。仅COM11复用既有SoftAP候选，冻结源码及BIN **`d43b5db8…02ef168`** 核同；COM9不动，原SDK2.4.4及Core3.3.11不升级。

唯一Web构建 **15.514s**、原同步脚本及唯一匹配beta原生构建 **43s／441 tasks（27 executed）**通过，覆盖安装一次。候选APK **`99b0c201…59684e2`**，原签名证书匹配；核实际源码override的Bridge／Owner／Protocol消费者、DEX新方法／能力、打包Page无参读取及新能力协商，不误用pnpm缓存。APK网络安全仍默认禁明文、仅精确AP地址例外，无额外CA；20s固定读取、原60／90s预算不变。本批无产品源码修正或服务部署。

### 27.2 配网候选启动前的实际停点

维护前已核原D4应用BIN **`e8a3c4c0…9adda13`**；新鲜完整20KiB NVS **`9b6d4ac1…50f259a`**和原分区表单独保留，原instance、root／epoch1、一个controller及TaskStore一条revision3固定为恢复基线，不覆盖为后续运行快照。

原D4短暂运行后仅`phy/cal_data`更新，导致第一次旧冷启动profile的前置digest核查拒绝，**未进入profile写入**。独立逻辑差异核查确认身份／授权／任务不变；保留新校准值的离线派生又因blob由2块变1块拒绝，亦未刷写。随后仅一次既有串口`n`正常网络重置，返回`accepted=0`／`sta is connecting, cannot set config`，未重发；该热重置限制单列，未冒称本批已修。

为完成限定读取，最终显式恢复上述初始NVS并ROM verify，再写已审计、仅五个WiFi键不同的既有冷启动profile **`b37ca6e9…43ae761`**并verify后复位；不改授权／TaskStore，不重新编译或重刷候选应用。保留原PHY运行快照但不声称最终profile保留该新校准值。此后系统确实连接设备AP，**本次只有一次显式manual connect**；上述维护停点不是App自动fallback／重连，也未重测系统“继续使用”授权矩阵。

### 27.3 新增能力、固定读取、禁止写入与回收

| 本次实际证据 | 有限结论 |
| --- | --- |
| 原生新cap严格true；create一次376ms、connect一次226ms，SDK走无参连接。 | 新匹配能力及隔离owner生效，不是旧能力缺失时的兼容scan。权限仅已授权路径，不重复拒绝权限矩阵。 |
| 无参`readBlinkerConfigInfo`一次 **472ms**；canonical Base64解码24B，头`02 01 00`、flags7、epoch1。 | instance／epoch与原NVS一致；create、connect和结果注入的owner完全一致。公开状态不是root指纹认证、写许可或完整身份接入。 |
| 原Page随后依赖扫描一次3398ms，十行全部禁用；无身份、目标SSID、密码或发送入口。 | 保留原只读流程，无再次刷新／选择网络／WiFi下发。 |
| 唯一无害探针：经通用`sendCustomData`请求相同固定GetInfo，26ms返回`WIFIPROV_SYSTEM_CONNECTED_READ_ONLY`。 | 即使报文无害，任意custom通道仍拒绝；不能写成custom调用为零。没有发送有效身份或WiFi写探针；这些禁写由实际打包消费者／既有host门禁及本次UI共同证明，不伪称实机写后拒绝。provision、reset及身份API均未调用。 |
| Home一次：pause→close **13ms**、close→dead **78ms**，随后stop；resume仍dead／未attached。 | 返回显示已断开，无第二create／connect／固定读取／scan。随后离页到首页无连接资源；未独立重连再测活连接离页，不累加为第二项无线关闭证明。 |

固定读取的原生trace request4沿既有脱敏allowlist将method留空；用CDP方法／字节、同owner、逐请求时序及实际消费者交叉关联，不能声称原trace已输出新方法名。UID采集器达到既定五分钟上限后退出并保留完整日志／阶段结果，未为采样重开队列，不是产品RPC超时。

HTTP日志保留：系统已连无网AP、尚未进入Local之前的两组`/account/connection` GET／OPTIONS约20.2s DNS失败；**Local持有期间未见新的该路径GET，离页释放后首页需求另起一组**，终态在采集窗口之外。只证明本次Local边界，不宣称全App零HTTP、所有请求均有终态或全局Auth已隔离。无验证码、新身份分配、自动重试器或旧UNKNOWN重放。

### 27.4 恢复、结果与唯一后续边界

- COM11恢复前核实例／授权／TaskStore投影不变；原D4应用、**原始完整20KiB NVS**及原表逐项ROM verify后复位。恢复范围不是8MiB全片，不把复位前字节校验说成复位后PHY／网络永不更新。
- 原diagnostics=false APK **`381363fe…ee7f48e`**已覆盖恢复并核实际安装SHA；手机回mostfun2026／首页。before、installed、final证明账号、完整session、安全存储及所有现存intents哈希精确一致。授权、TaskStore、UNKNOWN及全部原请求日志保留，未制造请求记录。
- 采集器、CDP wrapper／binding／本批转发、worker及系统XML均回收；最终只见原App主进程，无隔离worker。截图、完整HTTP／native／logcat、原NVS及维护失败记录保存在App／库忽略目录`build/access-ar4k-20261030`，含私有材料，不整体交付或推Git。正向系统截图为App目录`run-system-connected/system-10.private.png`，后台返回为`step-12.private.png`。

**§26.3新增固定GetInfo／同owner／只读禁写／真实后台回收短验有限通过，原现场已恢复。** 不关闭AR-4完整首次入网或报告#33。下一批先明确新owner显式身份／WiFi写阶段、已有根管理员凭据及Cloud交接合同，再实施必要最小接线；复用原身份／CAS／UNKNOWN链，不以本次GetInfo或RAM门闩授予写权限，不以等待／ready解除隔离。本批不自动开始写入验收、清根或新一轮测试；无Git提交推送，无关本地修改保留。

## 28. AR-4l：显式写入与已有根授权合同（2026-10-30）

### 28.1 支持边界与本批最小实施决定

`system-connected`现有owner永久保持只读，不能原地升级。完整手动接入须另有原生可协商的显式配网用途，并建立新owner；本批不凭cap／GetInfo／SSID／ready发行写许可证，不增加任意custom或以Install验证身份。GetInfo的instance／epoch／flags只作上下文比较，RAM `keyInstalled_`不是管理员授权。

| 阶段／身份 | 固定合同 |
| --- | --- |
| 新事务与新owner | 用户显式开始，旧Local owner实际死亡后才能进Cloud；账号／authority／sessionEpoch、原logicalId／credentialVersion／locator均重新核对。回Local须新owner、新鲜GetInfo及同instance／epoch／状态比较；同一owner、旧回调、后台／换号或CAS变化均停止。公开handoff不持秘密，也不能恢复旧await或生成写权限。 |
| 空根首次接入 | 仅支持新鲜空根、无任何已有allocation／pending Admin／rotation的全新事务，复用原allocation及Admin链；先原持久意图再外部副作用，设备精确Bootstrap ACK后才允许同owner下发一次WiFi。未知CREATE／Bootstrap／WiFi结果一律保留，不因空根或用户再次连接重放。旧managed／BLE恢复语义不由本批改写。 |
| 已有完整根 | 仅讨论保根WiFi重配，禁止CREATE、轮换、reset、Install及用Bootstrap当通用鉴权。先只读核当前账号原安全存储中的active全权限Admin（无keyRotation）、同instance／epoch、Cloud同logicalId／CAS及当前Admin controllerId／credentialVersion。缺凭据、共享控制者、pending／旧UNKNOWN或失配先停止。该核查仍不是设备端密码学同根证明，不能单独开启新手动写路径。 |
| 已有根实际授权 | 原设备端没有Admin challenge／MAC授权操作。Bootstrap只接受精确相同单Admin集合，分享扩展后可Conflict，不能当只读恢复或一般管理员认证。后续窄写能力至少须核原SDK**实际建立**的Security1及约定非空PoP，再明确物理配网授权合同；SDK可能根据proto-ver的no_pop／sec_ver改安全类型，create DTO的security1不等于实际会话证明。不支持此合同的设备继续只读，不静默降级或清根。 |
| Cloud／Local及UNKNOWN | 原死亡回执＋显式继续控制阶段；互联网可达只表示能发HTTP，不能替代owner死亡或解除sticky隔离。Local期间原全局账号查询保持暂停。Cloud只读结果也须原账号／CAS门禁；返回不自动重连／续写。任何在途写取消或不确定结果保留原意图／requestId／UNKNOWN，只读协调，不用resume推进旧注册／轮换／Bootstrap。 |

**本批最小接线：** 在原`WiFiProvAdmin`增加独立只读已有根检查，复用原安全存储、owner-only Cloud查询及CAS校验，不register／rotate／Bootstrap或修改凭据。原Page的隔离SoftAP身份入口先核已有未决allocation／Admin／rotation；已有完整根改用上述只读检查而不是可能重放的`resume()`，空根新事务仍用原执行器。仅在已有隔离阶段且支持Bootstrap的SoftAP接本合同，BLE及未接此合同的旧恢复入口不改造；手动只读双门禁不放开。补真实Page及独立协调器有限回归、匹配源码类型检查，不构建／部署APK或操作设备。本批结果及后续原生窄写接线停点另记，不把此安全前置说成完整手动入网完成。

### 28.2 最小接线与有限回归结果

1. **已有根只读核对。** 原`WiFiProvAdmin.inspectExisting()`只load指定logicalId的本机凭据、读当前Cloud Admin元数据及按原方法核Key CAS。严格匹配完整物理上下文、active／无rotation、全权限、controller版本／非零秘密／instance／epoch，以及Cloud logicalId／controllerId／epoch／version。输入在await前复制，换号／迟到结果失败，成功或失败均清零取得的秘密缓冲；只返回无秘密身份上下文。没有register、rotate、reset、Bootstrap、save或调用resume，不能称设备根认证。
2. **未决意图先停。** 新小型preflight在Cloud写前检查原allocation与本账号所有相关instance凭据，任何旧record、pending／rotation、歧义／存储异常、已知凭据对应空根或不完整根均停止。新`findForProvisioning`复用原扫描器，涵盖enrollment与旧WiFiProv active但缺Cloud元数据的记录；原`findWiFiProv`／BLE恢复筛选不变，原记录不删，跨账号变化失败并清理临时秘密。
3. **修正真实重放缺口。** 只做preflight后调用旧prepare存在竞态，旧prepare又会复用旧CREATE。已在原锁内增加`prepareNew`的“存在即拒绝”，`createNew`缺此能力即拒绝，不降级旧create；仍复用原allocation响应／CAS／Key校验。该锁仅保护原allocation库，不冒称跨凭据库或跨手机原子性。原BLE／旧恢复的幂等create保持，不把旧恢复测试当作新合同的重放许可。
4. **原Page接线。** 限已有隔离SoftAP阶段且支持Bootstrap：已确认的同轮身份保持原WiFi-only快路；未确认的RAM allocation与持久意图先停。已有完整根先原Cloud解析／CAS再inspectExisting，不走resume／清理旧记录；空根仅全新身份用createNew和原Admin首次Bootstrap，ACK后原WiFi步骤照旧。空根对应旧Cloud身份／重配上下文不在此最小合同内，停止而不擅自轮换。现有managed路径的物理授权前提未被这些只读事实替换；手动通道仍原生／App两层禁写。新核对用到的Admin／Key reveal／instance读取设置原`GATEWAY_NO_AUTH_REPLAY`，不在401时自动刷新并重发。

有限回归合计 **32个不同用例通过**：只读Admin10、preflight10、新凭据查找3、fresh-only分配3、真实Page5、HTTP no-auth-replay标记1。覆盖旧UNKNOWN／pending／rotation保留、未知CREATE不重发、旧记录在prepare前插入、缺实现不fallback、legacy／enrollment／跨账号／歧义、精确CAS及同阶段新owner、原Bootstrap ACK先于WiFi。向量循环及定向复验不累加，跳过项不算通过；Page外部SDK／HTTP替身不等同无线真机或新手动写能力验收。

初始直接Vitest队列的Angular服务JIT环境不匹配、Admin测试名筛选未命中及新Bootstrap夹具的WebCrypto／清零后原数组记录问题均保留原失败／跳过日志。改用项目原Angular test builder，仅补漏测／失败项；夹具只记报文公开操作号，并等待明确测试状态，不改生产等待／截止。零Key测试改为保留原decoder的`DEVICE_V2_BASE64URL_INVALID`，未放宽产品验证。随后只定向两项通过；App与选定回归TypeScript noEmit通过，必要diff无空白错误，原LF／CRLF警告保留。

日志在App忽略目录`build/access-ar4l-20261030`。本批无原生／设备／服务代码变更，无SDK／依赖／锁文件升级，无APK构建安装、ADB／刷板、验证码、自动重试器、清授权／删除意图或提交推送；原现场与完整日志保留，无关修改不动。

### 28.3 当前停点与唯一接续

**显式写入合同、只读管理员核对与新事务禁重放安全前置已软件有限通过；新手动写路径仍未实现／开放。** 不能把原managed Page的替身测试说成system-connected已能写身份／WiFi，或把active Admin＋Cloud记录匹配说成密码学同根证明。

下一批只接原WiFiProv内的空根首次接入窄能力：独立显式用途、新owner，原SDK实际Security1／约定非空PoP核对，固定101B Bootstrap及精确ACK后同owner的一次原SDK WiFi下发；通用custom／Install／reset继续拒绝，取消／换网／后台／UNKNOWN即退休。沿本批原allocation／Admin／CAS／阶段及原意图实现，不另造身份链。已有根的新手动写入仍须明确设备根／物理配网授权支持合同，当前保持关闭；不在本批添加新设备协议或清根绕过。AR-4完整入网、旧自动SoftAP失败与报告#33仍开放，不自动转入部署或真机队列。

## 29. AR-4m：空根首次接入的原生窄写能力（2026-10-30）

### 29.1 固定API与最小状态合同

复用原WiFiProv、SDK2.4.4、单worker owner／actor／截止／Binder死亡及UNKNOWN链，不在worker内加入账号、Cloud token、分配器或恢复队列。本批只实现原生能力与跨平台明确拒绝，不打开App现有只读入口或开展部署／真机。

| 入口／阶段 | 允许值与失败边界 |
| --- | --- |
| 创建用途 | 独立可选cap `softApSystemConnectedFirstInstall`，仅Android31+。仅直接`createDevice`接受`softApPurpose: 'blinker-first-install'`，需SoftAP、Security1及约定非空PoP，在启动worker前校验；用途固定该新owner整个生命周期，不能通过connect、QR、只读结果或后续参数升级旧owner。旧owner须按原死亡机制退休。 |
| 连接 | 该用途只允许原`system-connected`无参SDK连接，核实际权限／唯一精确SSID／原Network与非空binding；不能fallback到managed或BLE。普通无此用途的system-connected owner仍永久只读，通用custom／provision／reset均拒绝。 |
| 实际会话与空根 | 原无参固定GetInfo的真实响应建立同owner上下文；24B成功须flags恰为2、非零instance及epoch0。SDK实际SecurityType须仍为Security1，实际PoP须与create时约定的非空值完全一致，no_pop或安全降级不准入；连接DTO／ready／等待不证明上述事实。 |
| 固定Bootstrap | 新`bootstrapBlinkerAccess({ deviceInstanceIdHex, base64 })`仅接受精确非零32位小写hex instance及canonical Base64的101B原Bootstrap：`02 03`、合法非零43字符Key、epoch恰1、非零16B controllerId、controller版本恰1、非零32B secret。与该owner新鲜空根instance一致才可发；固定端点blinker-config，无调用方endpoint／编码／owner字段。有效尝试在进SDK前即消耗，读／扫描／明确拒绝均不能重新武装。 |
| ACK及WiFi | 仅精确3B `02 03 00`可武装同owner的新`provisionBlinkerWifi({ssid,password?})`一次；原3B明确status1～8仍保留设备拒绝，不授权WiFi。使用原SDK provision及原SSID／密码边界，进SDK前消耗WiFi机会。Bootstrap20s、WiFi60s整次截止不变；成功也不允许第二次下发。 |
| 关联与退出 | 两新方法的结果仅由parent当前live Binder注入localOwner，拒绝child伪造／缺owner／stop后迟到成功。每次调用、回调及进度核原网络与实际安全状态；SDK失败、畸形／不确定响应、换网、后台或截止沿原UNKNOWN／隔离／真实死亡，不重发、不扩大安全拒绝白名单。秘密报文仅有界临时持有，不记录到公开诊断，不在SDK尚可能异步使用前提前清零。 |

已有根仍不在本能力支持范围；公开GetInfo、Cloud元数据、active本机凭据或RAM门闩均不能改变此限制。原BLE及managed API不改造成另一条身份链，Web／iOS cap为false且新方法明确拒绝，无静默降级。有限host替身／类型／原SDK编译与设备无线／实际安全会话／完整入网验收严格分开；本批结果在后续小节记录。

### 29.2 原生最小接线与有限结果

- **用途固定在create。** Android直接create在worker启动前严格核用途／API31／SoftAP／显式Security1／非空PoP及字段边界；QR（含payload）、select及connect不接受该用途参数。原worker一生一个device／purpose，现有canSelect、死亡回执和停止门禁不变。新用途只能走原无参system-connected连接，不会从原只读owner自动升级，也不fallback到BLE、managed或主进程。
- **设备事实与实际安全。** 原固定GetInfo仅在匹配owner的真实SDK响应后记录首次物理上下文；第一次不是空根或自然设备拒绝，后续读空根也不能武装。真实SDK SecurityType、原约定PoP及no_pop能力每次检查，SDK降级、PoP漂移、失网／权限／binding变化均停止。源核原SDK的initSession可能按sec_ver／no_pop改变设置；本批核的是固定读取后的实际状态，不是DTO宣称。SDK custom／provision若需再建session仍用该单次连接的缓存version／capabilities；未另造安全实现或延长预算。
- **两条窄命令。** 原IPC严格校验固定101B／epoch1／controller版本1及非零字段，零化仅用于校验的临时解码数组；原Owner固定调用blinker-config，只有精确3B成功ACK才能武装一次新WiFi方法。合法Bootstrap／WiFi尝试进SDK前消耗；明确status1～8保留原报文但退休首装owner，不重试。通用custom、Install、reset及generic provision继续拒绝；原SDK provision回调复用，仅新增方法名／一次性门禁及相同60s预算，失败／迟到结果不恢复机会。
- **关联与秘密生命周期。** parent移除child owner字段后只注入当前live Binder，stop／换owner／缺owner时不能交付成功，原Channel安全拒绝白名单未扩大。审阅发现初稿在timeout／cancel清理时会提前改写SDK仍可能持有的101B请求，已修为：终态前不修改；真实SDK回调（含迟到回调）才清零，尚未回调时由隔离进程死亡回收这块有界内存。迟到回调只回收，不接受ACK或调用WiFi。没有把超时当SDK取消证明、等待当死亡或ready当授权。
- **跨平台及App边界。** TS增加独立用途／cap和两种窄结果，Web运行时及Swift源码明确false／拒绝，Swift QR内夹带用途也拒绝。现有App只读入口、原身份／Cloud分配／CAS／意图执行链不改，不因新cap自动发写请求；README明确本次原生支持范围与秘密不可公开记录。

唯一原生有限队列 **23个不同host方法通过**：真实Owner11、Bridge6、Protocol3、Channel3；另外**3项跨平台合同通过**（Web实际运行1、TS声明静态1、Swift源码静态1）。向量和定向复验不累计，不冒称Swift编译／iOS／OS进程死亡或无线真机证明。覆盖精确请求／ACK、空根与已有根区分、用途不可升级、实际安全漂移、单次下发、畸形／明确拒绝／未知／deadline、晚回调、保留原数组至SDK终态及parent owner关联。未重复BLE／历史真机矩阵；原§28的32项结果冻结。

插件与匹配源码声明的App TypeScript noEmit通过；仅受最后Web公共方法复用／Swift QR防降级修正影响的两项再定向通过，不重复累计。原SDK **2.4.4**单次离线定向Java编译 **51s／17 tasks（1 executed）成功**；源码override指向当前插件而非pnpm缓存，javap核实际Bridge／Owner／Protocol class含新cap／窄方法、实际SecurityType／PoP／no_pop与严格协议消费者，SDK JAR SHA **`0199ef44…ae971ef`**前后不变。不是APK完整构建或安装；原flatDir／SDK XML／Gradle弃用和LF／CRLF警告保留，未升级工具链或依赖。

必要日志、输入／源码／class校验记录在插件忽略目录`.build/ar4m-20261030`，不覆盖先前验收日志。本批无ADB、串口、刷板、服务部署、App安装、真实验证码、自动重试器、账号／授权／TaskStore／UNKNOWN或原请求日志变更，无Git提交推送；无关本地修改保留。

### 29.3 当前停点与唯一接续

**空根首次接入原生窄能力、有限回归及原SDK定向编译已通过；尚未接入App完整手动首装，也未真机验收。** 现有system-connected只读入口仍只读，已有根手动写继续关闭；不据host替身或编译结果关闭AR-4完整入网／报告#33。

下一批只接原App的显式空根首装薄端口：先明确固件约定非空PoP的输入来源及匹配候选，再复用原账号／fresh-only allocation／Admin／CAS／持久意图和死亡支持的Cloud／Local阶段。新用途必须新owner，固定GetInfo及每次结果核当前账号／generation／instance／owner，将原Bootstrap编码映射到新窄方法、ACK后才调用一次新WiFi方法；不能回退通用custom或provision，不能用resume重放旧UNKNOWN。先软件有限回归，通过后才另定一次匹配安装／COM11短验；本批不自动部署或开启硬件队列。

## 30. AR-4n：PoP匹配与App空根首装薄端口（2026-10-30）

### 30.1 PoP来源、候选与本批最小范围

PoP只能来自设备固件／设备提供者约定的配网码，由用户在现有SoftAP页显式输入；它不是热点密码、DeviceKey、云账号密码或验证码。App不从SSID／MAC／公开instance推导，不预填通用值、不发HTTP索取、不保存到URL／localStorage／账号或日志；仅本轮内存持有，离页／真实后台／取消／结束清除。新入口独立于“只读查看”，旧只读owner不可升级，cap缺失或PoP无效时不启动原生或Cloud流程。

已核普通WiFiBLE／EdgeHub产品及冻结COM11候选都固定传`nullptr`，**旧候选与非空PoP首装不匹配**。底层原Esp32WifiProvConfig已有proofOfPossession／1～64B校验／Security1传入；本批只补默认仍为空的可选编译期PoP注入点供两个产品复用，不改普通签名、不内置实际配网码。未来内部候选从私有配置显式注入，保留原Core3.3.11、固定endpoint v2／Bootstrap101B、taskCapacity0及原公开TLS profile；不改冻结候选、不生成或刷写固件。改变PoP不能清根，首次新鲜GetInfo仍须flags2／epoch0／非零instance，既有根继续拒绝。

最小配置为在产品／库include之前、或一致的构建私有配置中定义`BLINKER_V2_WIFI_PROV_POP`，值须是1～64B非空、静态生命周期文本；不把真实值写入公开示例、源码提交、构建输出或验收截图。两个产品通过同一薄配置取值，BLE／SoftAP使用同一固件约定；普通未配置产品仍为`nullptr`，显式空字符串交给原adapter拒绝，不静默降级。App本批只为新的手动SoftAP首装提供PoP输入，不顺带扩大BLE入口或宣称旧镜像已经支持。

App仅新增窄transport适配：将原GetInfo／Bootstrap编码与原provision调用映射到§29固定API，严格核当前账号／代次／同owner及精确ACK，拒绝Install／任意custom／generic provision及降级。复用原Cloud／Local阶段、fresh-only allocation、Admin与CAS／持久意图，不新建身份协调器或重放恢复引擎。Cloud前必须旧owner真实死亡；回Local新owner新鲜读取匹配后才发Bootstrap，一次ACK后同owner一次WiFi。空根检查失败先停在Cloud写之前。

真实后台仍取消旧generation／await／owner，不因新增入口保活；仅原公开handoff可保留，秘密不随回执保存。若网络切换必须离开App，返回不能自动续写或重放已保留的pending／UNKNOWN；本批不把这种需要只读协调的情况说成完整入网已验。只做软件有限回归和匹配声明类型检查，不部署、安装、发码、刷板或升级SDK。

### 30.2 最小接线与有限回归结果

- **显式入口与短期PoP。** 原SoftAP页增加渐进展开的“新设备首次接入”，仅协商到完整Android窄能力且非重配时展示；沿现有样式提供遮罩、显隐、就近校验和标签，区分PoP与热点密码。严格按UTF-8字节校验，不裁剪或派生秘密。用户点击后先按原机制退休旧owner／建立新generation，重新读取的cap再次核对后才申请权限／create；新用途与PoP仅放在本轮spec，不通过connect升级旧owner。离页、真正后台、取消、连接／写失败及WiFi成功均清除Page持有值；不是JS字符串可确定性清零或实际手机视觉验收证明。
- **薄transport，不重造身份链。** 新`WiFiProvFirstInstallPort`只映射固定GetInfo、原101B Bootstrap及typed WiFi到§29原生窄方法。复用旧读取的严格报文／同owner校验，3B明确设备拒绝仍交原decoder；Bootstrap经原encoder复核，精确`02 03 00`才武装同owner一次WiFi，有效尝试进原生前即消耗。畸形／错owner／错误操作／已有根／换号／退休／迟到ACK不能武装，未知对象保留，不转generic custom／Install／provision或自动重试。原始字节临时副本在调用终态回收；原生SDK请求生命周期仍沿§29，不提前改写SDK异步持有的数组。
- **原Cloud／Local链保留。** 首次原GetInfo确认空根后，继续原preflight、fresh-only allocation、Admin、CAS及持久意图；Cloud写之前必须原owner死亡并显式继续。回Local重新create同用途的新owner、新鲜读取同instance／epoch／根，原stages核对后才发Bootstrap。终审补上显式首装缺隔离owner即在connect前拒绝，不能走旧managed兼容fallback；未改变旧BLE／managed路径。旧allocation、UNKNOWN或已有根均不绕过；成功后最终云核对仍是独立显式阶段，取消不撤回已确认WiFi或清理未决记录。

软件有限结果：**App共32个不同用例通过**（新薄端口15、真实Page／阶段／身份链11、真实模板2、受读取重构影响的原明确拒绝／UNKNOWN兼容4）；库侧**6项通过**（配置／消费者合同3、实际无依赖头文件C++编译3）。覆盖新cap刷新失效、缺owner先停、旧只读owner不可升级、Cloud前死亡、fresh-only意图、已有根前置拒绝、精确ACK／一次WiFi、晚ACK和真正stop后不自动恢复。HTTP／原生／存储边界用替身，不把这些用例计作无线、安全会话、OS死亡或完整入网成功；各次跳过项、向量及定向复验不累计。

初次类型检查的typed-array夹具／字面量收窄及严格索引访问错误、真实模板的`spellcheck`属性绑定错误，以及后续Angular对两种provision返回联合类型的推断失败均保留原日志。分别修正夹具类型、属性写法与原`ProvisionResult`泛型后，仅补失败／新增项；最终App及选定回归TypeScript noEmit通过。未放宽schema、结果合同或使用`any`绕过产品类型。必要diff无空白错误，原LF／CRLF及测试setup未列入TS程序的builder警告保留。

日志保留于App忽略目录`build/access-ar4n-20261030`。本批不重复§29已通过的原生／原SDK Java队列；库侧头文件编译不等于两个完整Arduino产品链接。没有选择真实PoP、生成固件／APK、部署、安装、ADB／串口／刷板、发码、SDK／依赖／锁文件升级或提交推送；账号、授权、TaskStore、UNKNOWN、完整原请求日志和无关本地修改未动。

### 30.3 当前停点与唯一接续

**PoP来源／匹配条件已明确，App空根首装薄端口及软件有限回归完成；完整手动首次入网仍待匹配构建与真机。** 已有根手动写、普通只读owner升级、任意custom／Install／reset继续关闭；AR-4整体及报告#33不关闭。旧COM11空PoP候选不能代替新非空PoP候选，不把既往只读真机结果拼接成写入验收。

下一批只准备约定非空PoP的匹配SoftAP候选与含§29插件／本批App的安装包，沿原SDK进行必要定向构建后再约一次COM11短验：新用途／实际Security1、空根GetInfo、新owner／同实例根核对、精确Bootstrap ACK→一次WiFi及云上线；现有授权／任务若需变更先明确有限范围，保留原意图和日志。遇真正后台或未决写，立即按原UNKNOWN只读协调边界停止，不为跑通而忽略stop、延长等待、重试写入或清根。该实际系统切网支持边界只在首次停点补证，不重开已通过的BLE／只读矩阵；本批不自动开始上述部署或硬件队列。

## 31. AR-4o：匹配非空PoP候选与COM11空根短验（2026-10-30）

### 31.1 本批范围及新鲜现场基线

仅COM11的XIAO ESP32S3（MAC末尾`1b:96:14`），COM9及其他板不操作。新鲜只读核查原应用与D4恢复BIN **`e8a3c4c…9adda13`**实际一致，分区表 **`5f451391…d37e7a`**不变；完整20KiB NVS **`9b6d4ac1…50f259a`**已读回／ROM verify，仍为原实例、epoch1／一管理员和一条revision3任务，不能冒充空根。

用户随后明确允许**本测试板直接覆盖、结束不必恢复**。因此本批允许新候选及独立空NVS初始化新测试实例，不将旧root切成“获准手动重配”，不重放旧任务／UNKNOWN或复用旧云scope。保留上述已校验的原固件引用、NVS、原授权／任务快照和App全部现存记录，但不承诺测试后原授权仍在板上生效；本批不额外清理旧云设备或App历史。无必要的全8MiB备份不重做，分区表不写。

App原账号、安全存储及全部请求journal先留存。匹配App沿原Node／pnpm／SDK2.4.4、cloud-beta和单队列；固件只编译SoftAP／Core3.3.11／四槽功能关闭的0槽候选。PoP由本批私有生成输入提供（18随机字节的24字符文本），以私有头在产品include前注入，不放命令行、公开源码或日志。原TLS／P256公开profile复用，不新增服务部署或SDK覆盖。实际构建／运行结果与未通过停点记后续小节，不用本节计划充当验收。

保持登录不等于具有DeviceKey近期读取权限：当前Gateway将近期认证单独保存约5分钟，JWT有效期不能替代该门禁。没有独立公开状态端点，本批只读远端必要条件查询因没有现成认证SSH会话未执行，结果为未核实而不是过期。实际正常身份链若收到近期认证／step-up拒绝，停在首个结果，保留新意图，不发码、不伪造认证标记、不重发或绕过；是否已登录由App原session另核。

### 31.2 匹配构建及安装结果

App首轮默认Web构建因pnpm包的旧`dist`声明缺少新API停止，产品源码／锁文件未变；纠正为原批次使用的当前插件TS源码映射，同时原生Gradle显式指向同一插件工作树，再执行必要构建。匹配Web **16.972s**、原sync **6.577s**及原SDK2.4.4离线APK **122.809s**通过。库侧仅一份SoftAP队列，Core **3.3.11**、CLI1.5.1、PubSub2.8、jobs1，完整编译／链接 **384.301s**；应用 **1,396,416B**、静态RAM **56,844B**，BIN **`c05195d7…36b96eff`**，当前src快照、原profile／P256、私有PoP头的实际消费者、单SoftAP方案及无TaskStore均核同，原分区可容纳，不升级SDK或写分区表。

APK **`46f4c994…2403b33`**已同签名覆盖安装一次，beta包名／HTTPS／原生注册、打包DEX窄接口及全部1156份同步Web资产核同，实际Java class核原SecurityType／PoP／no_pop消费者；diagnostics=false，不另造trace版再构建。产物核查器初稿把Capacitor生成的cordova文件误作www原文件、把枚举名误写为Security1，已只修核查器为精确同步资产比对及实际`SecurityType.SECURITY_1`，不改产品或重编APK。原失败记录保留。

仅COM11应用与已批准空20KiB NVS写入并逐项ROM verify，原分区表不变；启动新实例、无root／任务快照，未内置DeviceKey或目标WiFi。原应用引用和原20KiB授权／TaskStore快照保留但不恢复；不能宣称旧授权仍在板上有效。当前测试实例不复用旧scope。

### 31.3 真实短验：有限通过及实际停点

| 本批实际观测 | 有限结论 |
| --- | --- |
| 系统连正确新AP并明确接受“继续使用”；原Page显式首装create **341ms**、无参SDK连接 **178ms**、真实设备扫描 **3889ms**，有11个SSID及可选目标网络。 | 非空PoP匹配候选可连接并扫描。只走原WiFiProv，不是Direct或任意custom；未重复BLE矩阵。 |
| 同create／connect owner的固定GetInfo **120ms**，精确24B成功，flags2／epoch0，新instance与新鲜NVS一致。 | 首次空根与同owner有限通过；原生实际Security1／PoP门禁已进入成功读取路径，编译声明不是单独证据。 |
| 原Cloud阶段先clearState **60ms**完成；随后显式继续一次。 | 保留原owner死亡／Cloud边界；没有等待／ready解禁。 |
| 手机仍在设备AP，首个`devices:resolve-instance`语义只读POST及关联OPTIONS均`ERR_NAME_NOT_RESOLVED`，HTTP0；没有CREATE／Admin／Key reveal。 | 本次切网前提未完成，不是账号或近期认证拒绝，更不是身份写成功。Page停止，没有自动重发。操作者随后选择互联网网络，没有把该旧请求重放。 |
| 一次明确纠正后重新开始：第二个create **384ms**产生不同owner；connect **794ms**返回`WIFIPROV_WORKER_UNCONFIRMED`，随后clearState **45ms**，无任何Cloud请求／写入。 | 实际没有连接到新Local。系统采样仍为mostfun2026；该“已提交连接命令”不等于实际连接，不据其放行。 |
| 结合最终串口和时间线：第二次已越过产品原**10分钟配网窗口**，最终state5／error21（NotConnected），源码在到期时end并进入该故障。 | 配网窗口结束是第二次不能继续的重要现场前提；不能把该次unconfirmed当成PoP或原SDK缺陷。未延长窗口、重启第三轮或盲重连。 |

**Bootstrap调用0、ACK未到达、新窄WiFi调用0、generic provision／custom／reset调用0，云上线未完成。** 不把“点击发送”的Page身份阶段计作已经下发WiFi。第二owner虽不同于第一owner，但尚未成功回Local及再次读取，不能宣称Bootstrap前的新owner全合同已验。

最初采集器因非TTY输入EOF在无操作时退出，随后用交互采集；目标密码读取器只识别英文netsh字段而在填表前停止，修为兼容现有中文“关键内容”，保留前段证据并继续**同一个已连接owner**，没有重复SDK connect或发送。上述工具停点不算产品失败，也不遮盖首次Cloud操作前没有先完成系统切网的操作前提问题。秘密只进原表单／原生临时参数，不输出PoP、密码或Bootstrap原文。

### 31.4 收尾与最小接续

匹配App保留，原账号／完整session／安全存储和所有原请求journal在before、installed、final哈希精确一致；**当前App仍已登录**，本批没有到DeviceKey reveal，不能推断近期认证失效。原授权／UNKNOWN／历史设备不删，不新增云身份或恢复旧scope。COM11按用户新决定保留新SoftAP候选和新空根测试实例，配网窗口已结束；不恢复原任务。最终手机回mostfun2026／首页，采集、包装器、9479转发及worker均回收，仅原App主进程；本批临时USB常亮已关闭。

App／库忽略目录`build/access-ar4o-20261030`保留原账号／NVS、匹配产物、完整已采集HTTP／native／logcat、截图、所有失败及阶段日志；不整体推Git或公开私有PoP。无服务部署、验证码、自动重试器、SDK／依赖／锁文件升级、提交推送，产品源码无本批新增修改，无关本地修改保留。

**本批匹配构建与首次非空PoP读取有限通过，完整首次入网未通过，AR-4／#33继续开放。** 下一批只处理已明确的实测前提：在原10分钟窗口内先安排好Cloud阶段可访问互联网、Local阶段实际连接精确设备AP，再按真实owner死亡／显式继续门禁推进。系统网络选择可用性、默认路由与“提交命令”分开核对；真正stop仍取消，不加宽后台、不延长等待或复用ready／旧owner。先核实际网络与当前空根，无已保存写意图才允许全新事务；若Cloud返回近期认证或UNKNOWN立即停止，保留原记录，不发码或重放。未经上述前提核清，不继续新一轮Bootstrap／WiFi测试，也不重编已通过产物或重跑BLE／断电矩阵。

## 32. AR-4p：实际切网前提与现有候选短验接续（2026-10-30）

### 32.1 固定显式顺序，不以观测解禁

复用§31同一APK／固件，不构建、安装、刷写或清NVS。开始前保留当前账号、安全存储、完整journal及已有日志；核COM11身份、候选和现存根／未决写，不重放旧意图。只在全部必要前提可用后重启COM11进入原10分钟窗口，其他板不动。

Local阶段须新鲜系统关联证据：精确设备SSID、Supplicant COMPLETED、IP与唯一、非VPN的默认WiFi网络；“命令已提交”、扫描列表、旧WifiInfo、等待或ready均不能替代。随后原SDK连接及同owner固定GetInfo才证明设备会话，系统关联观测不授予Bootstrap或WiFi许可。Cloud阶段必须先原owner真实死亡／隔离释放，再由实际互联网默认网络及正常TLS／API结果确认前提；不能用VALIDATED清除死亡未知或调用旧await。

固定顺序为：精确AP→原Page显式首装connect一次（自动scan）→填表／send一次（自动GetInfo）→死亡／await-cloud→手机实际互联网→continue①→原Cloud身份链／await-local→手机实际回同AP→continue②（自动新owner、connect及GetInfo）→精确Bootstrap ACK→同owner一次typed WiFi→死亡／await-cloud→实际互联网→continue③／原finalCloud。不得另发GetInfo、第二次按钮connect／send或直接调用窄写补步骤。真正stop仍取消原代次；若切系统Settings触发stop，不能强续旧continue。采集器期限不是固件剩余窗口，切网／worker重建不续期。

### 32.2 窗口启动前的实际Cloud预检

已核实际安装APK仍为 **`46f4c994…2403b33`**，未重建或重装；本批只为原APK建立硬链接而非重复复制备份。手机实际SSID mostfun2026，默认网络134为同一WiFi、INTERNET／VALIDATED／NOT_VPN，当前session／账号仍与§31一致。before完整安全存储及journal已保存，COM11窗口尚未启动。

为避免在取Key权限未知时创建孤立身份，仅对已有owner COM11云设备做一次fresh DETAIL预检（不是新测试实例的分配，也不将旧Key用于新实例）。手机WebView从https://localhost正常CORS OPTIONS取得204，随后单次GET到达https://iot.yiyu.pro并收到**HTTP504，8708ms**，requestId **`14ada0bb346a74b1a37e64e11be2bc79`**。因此DNS／TLS及互联网可达与§31设备AP上的DNS失败不同，当前停点是服务端上游超时；不能据此认定账号过期、近期认证拒绝、PoP或SDK失败。由于fresh上下文未取得，Key reveal为0，CREATE／Admin／Bootstrap／WiFi均为0，不换设备试探或重发GET。

本批控制器及已采集网络证据位于App忽略目录`build/access-ar4p-20261030`。只读云依赖定位另记实际结果；未因该504重启COM11、消耗配网窗口、扩大后台、改等待、切换owner或启动硬件循环。

### 32.3 迁移部署只读诊断与实际阻点

用户明确要求检查迁移后的`129.204.34.159`，并确认继续使用原统一测试Auth `https://api-test.yysc.tech`，不切到正式账号库。通过已固定host key的SSH开展有界只读检查，凭据只在隐藏输入／内存中使用，不落盘、不打印；没有部署、重启、修改Nginx／Compose／防火墙或数据库操作。

| 实际检查 | 结果与边界 |
| --- | --- |
| Gateway、Broker、PostgreSQL、Redis四个IoT容器 | 均运行且healthy，restartCount0；未发现并行部署作业。新主机现存部署目录及监听中没有单独运行的统一Auth服务，不把IoT迁移等同Auth已迁入。 |
| Nginx及IoT入口 | `nginx -T`通过；IoT域名解析到新IP，本机Gateway `127.0.0.1:7000/healthz`返回200。实际设备API能够收到Gateway的结构化响应，不是Nginx不能连Gateway。公开`/healthz`返回404、8884未升级连接返回426各按实际路由语义记录，不当作API宕机。 |
| Gateway→Broker与Docker出网 | 实际分片地址是`broker:3000`，`/internal/health`返回200；容器默认路由走非internal的egress网络`172.19.0.1`，主机OUTPUT ACCEPT，普通国内HTTPS连接可达。初稿探针误用4000端口的拒绝、把delimited SHARD_ENDPOINTS误作JSON的脚本失败均保留并纠正，不能据此声称Broker或产品故障。Cloudflare目标不可达也不作为“全站无法出网”证据。 |
| 统一Auth目标 | Gateway实际`AILY_API_BASE_URL`及内部Auth base均为`https://api-test.yysc.tech`，服务器／容器解析为`42.193.22.41`。宿主HTTPS连接超时，容器请求6秒截止；该IP443及旧8180的TCP检查均超时。只证明从新IoT服务器到该Auth目标不可达，尚不能区分Auth服务未启动、目标侧安全组／白名单或网络路径故障，不能擅自判定DNS该改到新IoT IP。 |
| 严格设备读取的明确错误 | 一次独立诊断`GET /api/v2/devices`返回 **504／`AILY_TIMEOUT`／`Aily request timed out`，8451ms**，requestId **`5bc65fead06818dfb7aad0e08bcbc636`**。与原fresh DETAIL约8.7s停点及Gateway原8s Aily调用截止一致；直接失败依赖定位到统一Auth，不是DeviceKey、Bootstrap、PoP或SDK。 |
| 登录态与缓存 | 单次`GET /api/v1/auth/me`返回200但耗时8443ms；源码允许该路径在依赖失败时使用有界旧身份／权益快照，DeviceV2使用更严格的新鲜身份读取。因此该200不证明Auth已恢复，不能据其放开写入或擅自要求重新登录。本次没有取Key／发码／刷新token，也没有跨环境试送token。 |

手机对Auth域名的无凭据直接fetch在11ms返回TypeError，未取得HTTP响应及有效传输证据，不拿它判断Auth主机存活或网络防火墙原因。目标请求ID在本批有界Gateway日志中未匹配，故不声称已有完整逐跳trace；明确`AILY_TIMEOUT`、实际运行配置及双位置依赖可达性结果分别作为证据。

**当前直接原因已定位为统一测试Auth上游超时，底层Auth目标的监听／安全组／网络路径仍须其维护侧核对。** 保持原Auth域名和账号库，最小后续是确认`api-test.yysc.tech`的预期目标是否仍为`42.193.22.41`，若正确则检查该Auth服务443监听、目标侧安全组／防火墙及是否允许新IoT服务器IP。没有该Auth主机或域名管理权限时，不把IoT重启、改成明文8180、换正式Auth、延长超时或放宽缓存当作修复；本批只诊断，不实施这些改动。

SSH、CDP及本批转发均已结束，当前App账号、完整session、安全存储和journal前后哈希一致；COM11／COM9未重启、刷写或开启新窗口，原UNKNOWN未动。Service忽略目录`build/access-ar4p-20261030`保留部署只读证据，App同名目录保留请求证据及原记录。未构建／重装、服务变更、验证码、自动重试器、SDK升级或提交推送；无关本地修改保留。Auth链路恢复且严格读取正常之前，不启动COM11首装短验。

## 33. 正式 Auth 隔离切换（2026-10-30）

### 33.1 新决定与最小隔离

用户在§32诊断后明确改用 `https://api.yiyu.pro`，批准保留原测试卷、使用全新PG／Redis／Broker存储。该决定替代此前“继续测试Auth”，不把原测试Auth超时称为已修复。操作仅在固定host key的IoT主机 `129.204.34.159`；正式Auth仅调用公开HTTPS，不管理其主机、账号库或密钥。

新鲜只读检查原PG账号／租户／设备 **3／3／8**、Redis20键。实际表没有Auth issuer分区，`AILY_IOT_ENVIRONMENT`不能替代数据库隔离，因此不在原库只改URL、不按邮箱合并UUID，不复制旧账号、设备、Key、scope或UNKNOWN。

| 项目 | 实际结果 |
| --- | --- |
| 新运行域 | project `blinker-iot-cn-20261030`，双侧environment `yiyu-iot-cn-auth-20261030`；新独立backend／egress网络、新 `blinker-iot-cn-20261030_{postgres,redis,broker}-data` 三卷。切流前精确核新卷／容器／网络均不存在，运行后核实际Source。 |
| 原运行域 | `blinker-iot-beta`四容器正常停止并保留，原三卷、配置、密钥、完整原日志及数据保留。未删卷／清库／导入旧数据，也未执行 `down -v`。 |
| 镜像和入口 | 复用原实际Gateway `d0fa1ccc…365c2`、Broker `c6500784…b2bc8`、PG `721873c3…080ea`、Redis `858f009f…b3499`，不build／pull／升级。保留 `iot.yiyu.pro`／`mqtt.yiyu.pro`、原Nginx／TLS及7000／8884／18883 loopback upstream，无新公开端口。 |
| Auth接线 | China／Global／Development／internal四项显式统一 `https://api.yiyu.pro`，无测试fallback；不声称独立Global服务已验。不复制正式Auth secret、内部目录token或旧权益事件来源。 |
| 用途材料 | 新PG密码、MQTT Ed25519对、双向内部token、KEK／ID、分享secret、migration HMAC、五用途自动化HMAC分别生成并配对。材料只在私有运维目录，不进App／Git。MQTT无独立kid配置，不混同Task／BLE keyId。 |
| 支持边界 | 自动化功能维持双侧开启、规则库为空。清旧任务／定位逐设备名单，移除旧P256挂载并清BLE／Task signer路径；Direct、跨分片、通知及Auth事件仍关闭。新设备任务／Direct须另核签发材料，不自动继承旧试点准入。 |
| 地图 | 暂 `MAPS_ENABLED=false`，保留 **4500／UTC日**及原用户／分钟限制和上游私有Key，防止新Redis重新赠送同一上游Key的当日预算。未清旧计数、提高额度或自动安排启用；再启用前须明确安全预算承接或新日可用额度。 |

新四容器均 **healthy、restartCount0**，新PG账号／租户／设备 **0／0／0**。现有公开HTTPS入口单次检查：不带凭据的`/api/v1/auth/me` **401／51ms**；`/api/v1/auth/altcha/challenge` **200／306ms**。挑战不是发验证码，健康不是实际账号／权益验收。

编排停点已保留：Docker不存在错误文本差异、旧日志输出保护两次均在停止原服务前结束；改为成功清单的精确不存在断言和日志私有直写。新存储启动后，Compose按需建网使Gateway专用egress尚未创建；只读确认三新服务健康、Gateway未创建后，仅补新域egress并继续Gateway。没有重建新存储、重试业务请求或回落测试Auth。

### 33.2 App隔离与验收边界

复用§31同一已装beta `46f4c994…2403b33`，未构建或安装。切流前私有保全session、安全存储、全部journal及完整请求／logcat；在仍为测试Auth的入口使用原退出控件正常退出，核`/login`及session不存在。残留旧通知凭据、authority＋owner显示缓存封存后移出活动存储，不清整个App数据、不删请求journal；两个原journal逐行保留（本机当时各0行）。旧token只在私有归档，不导入新域。

首次冷启动发现两个Manifest显示缓存仍在持久存储，而会话／通知凭据均不存在、账号目录已清；保留失败快照，只补精确旧owner显示缓存清理，不将内存删除当持久化完成。最终 **06:40:56 UTC冷启动核查通过**：停在`/login`、原生安全存储无活动测试凭据、旧owner缓存不存在、全部journal与切前逐行一致；没有重新构建／安装、清App数据或登录正式账号。CDP及本批ADB转发已回收。

本批未向正式Auth试送旧测试token、代登账号、发验证码、自动注册或创建设备；COM11／COM9均未操作，未reveal／Bootstrap／下发WiFi／重跑H2。正式Auth实际账号、套餐／IoT权益、近期认证及新设备入网尚待正常登录后验证。AR-4／报告#33仍开放。

### 33.3 最小接续与恢复

下一步在当前beta登录页用正式Auth已有账号正常登录，优先已有密码，不自动发码或导入旧凭据。核新鲜身份／额度及近期认证后，再接续§32.1冻结的COM11空根入网，正常分配新身份，不复用旧测试DeviceKey／scope。不重复构建、BLE或地图矩阵，不绕近期认证门禁。

若需回退，先停新域、保留其卷／密钥／日志／未知意图，再整体回接原容器及原卷；不能只换Auth URL而沿用另一域存储。App旧归档仅能在原测试运行域恢复后协调使用，不在线导回正式域。本批未回退，启动命令提交不单独等同健康恢复。

Service忽略目录`build/auth-production-selection-20261030`、远端私有运维目录及App `build/auth-production-switch-20261030`保留完整证据／失败和旧身份归档，不整体交付。无SDK／依赖／锁文件升级、Git提交推送，无关修改保留。

## 34. AR-4q：首装前置补缺与原窗口短验（2026-10-30）

### 34.1 本批唯一范围

复用§31同一beta APK `46f4c994…2403b33`、COM11 SoftAP固件 `c05195d7…36b96eff`及原Core3.3.11，不重复构建／安装／刷写／清NVS。USB串号核同COM11 XIAO `8c:bf:ea:1b:96:14`；COM9不操作。目标网络仍mostfun2026。沿§32.1原死亡及显式继续序列，在一次原10分钟窗口核新owner、Bootstrap精确ACK、同owner一次typed WiFi、云上线；不以系统网络观察／ready／等待解除隔离，不放宽真正stop。

启动前原App在登录页、无活动凭据；账号及完整journal／日志已保全，两个原journal各0行。用户随后正常登录正式Auth账号；原APK、同账号当前安全存储及journal再次保全，无代码发码／刷新／注册或测试token导入。普通登录与5分钟Key近期认证是两个合同，窗口只在必要前提齐备后启动。

### 34.2 先修正实际首装入口缺口

原App空根身份链在CREATE／reveal后、Bootstrap之前必须 `registerDirectAdminV2`。已只读确认新域Nginx未公开该精确路径，实际Gateway原Admin handler则不受LAN开关或BLE／Task signer控制；它保留owner、新鲜身份、active设备、controller／fingerprint及事务校验。此时未启动COM11窗口或创建半途设备，不以开放整套Direct／恢复旧signer替代正确接线。

复用原Nginx渲染器增加严格可选 `provisioningAdminEnabled`，默认／false逐字保持原输出；true只允许 `/api/v2/devices/{规范1～128B ID}/direct-access/admin` 的PUT／OPTIONS，无query、缓存或上游重试。GET／PATCH／reset、任意custom／签发及其他Direct仍关闭。新增6项、原关联24项共 **30个不同有限用例通过，无跳过**；没有Gateway运行源码、镜像、SDK或模板默认修改。

现网原配置与完整差异已私有保留，只插入该段、`nginx -t`通过并平滑reload，其他原路由／TLS／upstream逐字保留。安装前针对旧注释／多个server拒绝块的安全匹配停点均发生在写配置前，未隐藏或自动循环部署。reload刚提交时首组无凭据检查仍404；随后只读核运行配置、reload记录和实际接收IP，回环OPTIONS **204**、公网GET **405**，证明新worker已生效。不能拿首次404推断Gateway handler不存在，也不把reload命令提交当生效证据。新服务／账号／设备数据未因此改动。

本机及云端必要记录位于三端忽略目录 `build/access-ar4q-20261030` 和远端同批私有目录；不打印PoP、WiFi密码、token或Bootstrap正文。COM11原窗口及实际业务短验结果在后续小节单独记录，软件／路由检查不计为Bootstrap或云上线通过。

### 34.3 窗口启动前停点（已由§34.4接续）

用户正式账号已登录，原journal与登录前逐行一致；但准备期间5分钟近期认证已过期。通过当前token摘要对应的IoT Redis精确键只读PTTL得到 **-2**，未发新挑战／验证码、refresh或伪造标记；普通session仍存在，不能把它写成“账号未登录”。已请用户在前置齐备后用同账号已有密码正常重新登录，之后立即接续，不提前CREATE留下半途分配。

**本批COM11配网窗口未启动，真实CREATE／reveal／Admin写／Bootstrap／WiFi下发均0，尚无云上线验收。** 准备好的单次reset／观察工具只做语法核查，未运行硬件动作；没有烧录、NVS读写、清授权、重放UNKNOWN或新构建。最近一次成功App快照为正式账号已登录、原journal不变；末次只读终核因WebView调试连接超时未取得新快照，不能宣称另有终核成功，也未为恢复调试而重启App、恢复旧token或发码。已保留成功快照与完整已有日志，本批CDP／SSH及转发均结束，无自动重试器、提交或推送。下一步只等待正常近期登录后启动已准备的一次原10分钟窗口，不再重做本节已通过的软件／Nginx检查。

### 34.4 一次原窗口实测：云上线通过，WiFi回执UNKNOWN保留

用户再次用同一正式账号正常登录并保持解锁后，立即接续，不重复软件／路由／构建检查。**北京时间2026-10-30 15:35:35.320**只对COM11完成一次原复位，原10分钟窗口不延长；没有烧录、维护写NVS、清根、网络reset或操作COM9。App及固件沿用§31同一产物。

| 原链步骤 | 实际结果 |
| --- | --- |
| 首个Local | 手机精确AP、COMPLETED、192.168.4.2、非VPN默认WiFi已记录。原页面首装create380ms、connect275ms、扫描3928ms；GetInfo95ms，24B成功、flags2／epoch0。原owner真实关闭69ms后，实际互联网网络及显式continue进入Cloud，非等待／ready解禁。 |
| Cloud身份 | 原`resolve-instance`200、唯一CREATE201、reveal200、Admin PUT201；各语义请求仅1次。新逻辑设备`device_54bb065d-9299-4852-bfdc-2a001aa744d1`，未复用测试域Key／scope或重放旧意图。 |
| 新Local owner | 实际回同一精确AP后显式continue，新create372ms／connect222ms，owner从`c5d55d69…50ad`变为`12acac96…834b`。新GetInfo553ms，仍同instance `6d692032857f461a8bdc3ad6db8d13fc`、flags2／epoch0；未把旧owner升级或用网络快照授权写入。 |
| Bootstrap | **唯一1次**，新owner且与其create／connect一致；86ms返回精确 **3B `[02,03,00]`**，原decoder接受。此ACK不代表WiFi操作已经成功。 |
| typed WiFi | ACK后34ms，原`provisionBlinkerWifi` **仅调用1次**；7675ms返回 **`PROVISIONING_RESULT_UNKNOWN`**，无成功回执。没有第二次下发、generic provision／custom、重新开始或自动重连；随后原clearState72ms获结束回执。WiFi错误本身未携owner，关联依据是唯一新owner、原固定port／worker及期间无owner更换，不能声称错误另有显式owner证明。 |
| 只读云上线核对 | 未continue／重发旧操作。Gateway实际同instance新TLS会话active、credentialVersion1；Broker该设备`mqtt_online=1`、session未撤销、Manifest revision1／2字段及State revision1，State与该session一致。会话在窗口后 **272753ms**产生，新State在 **277519ms（约4分38秒）**到达，均在原10分钟内。独立串口在启动后379342ms为state4／error0／networkReady1／cloudReady1，未network reset。 |

**新owner、Bootstrap精确ACK、一次typed WiFi调用及实际云上线已取得本批新鲜证据；App完整成功收尾仍未通过。** 设备实际收到并使用网络配置，不意味着能把此前UNKNOWN RPC改写成成功；下发的设备端exactly-once也不能只由1次App调用推定。保留本批未知结果和原记录，不伪造ACK、不改ledger、不用服务器在线状态抹掉未知回执。

原观察器已恢复被包装的nativePromise、关闭binding及ADB转发；原worker已结束，最终仅App主进程。手机回mostfun2026、COMPLETED／VALIDATED；不强推旧continue或修改sticky隔离。最终App新鲜登录的**完整session逐字一致**，原两个journal逐行不变；新增Admin凭据active及原allocation记录保留，安全存储1→3项是本批正常身份安装，不删除云设备／授权或恢复旧NVS。COM11保留匹配固件与实际新绑定，云端scope不做人工回灌。

私有App事件、完整HTTP／native／logcat、错误页截图、串口及远端原日志／PG／Broker只读证据均保留。`result-audit.json`独立核上述次数、owner、ACK、同instance／session、时间界及session／journal。旧preflight摘要中的`boardWindowStarted=false`为遗留默认值、`cloudRequests=0`只表示快照工具未发HTTP；已修正工具并以本批reset／事件／服务端证据在审计结果中显式纠正，不据旧标签声称未运行。无App／固件／镜像重建、验证码、SDK升级、提交或推送，无关修改保留。

### 34.5 唯一剩余：WiFi UNKNOWN与已上线的收尾区分

7675ms不是原60秒整次RPC截止。现源码的SDK config／apply／provision失败、网络丢失、后置绑定检查均可能映射UNKNOWN；原SDK2.4.4的应用后2秒／5秒状态查询与该量级相容，但现证据尚不足锁定具体回调。手机日志时区为America/Los_Angeles，不能直接按北京时间筛日志或将未命中当无回调；业务关联以Unix时间／同instance／session为准，未修改手机时区。

下一批只定位这条已发生的确认分支并设计最小收尾：保留已确认的Bootstrap／Admin及WiFi UNKNOWN，通过原owner只读云状态协调分别展示“设备已上线”和“本地回执未知”，不重发配置、不自动重建／重试，也不将历史RPC改成功。若现日志缺原生失败分支，只补可关闭、有界、脱敏的SDK回调／网络失效分类；先有限回归，再决定是否确有必要另做一轮单次差异验收。**不再重复本批已过的新owner／ACK／云上线、BLE或长循环，不重新部署Auth或扩大Direct。** AR-4／报告#33整体仍开放，当前不宣称UI完整入网已收口。

## 35. AR-4r：官方回执合同、适配层竞争与只读协调（2026-10-30）

### 35.1 WiFiProv关联与证据边界

回执涉及原WiFiProv／Espressif配网状态链，但UNKNOWN不是设备明确拒绝，也不能直接归因于ESP SDK缺陷。官方参考：

- [ESP-IDF 5.5停止配网合同](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/provisioning/wifi_provisioning.html#when-how-to-stop-the-provisioning-service)：`set_config`／`apply_config`之后查询`get_status`；默认成功状态响应后自动撤除配网服务，无状态查询则30秒后停止。手动停机需给响应传输保留清理延迟，不等于在App增加固定等待或凭ready放行。
- [原Android SDK lib-2.4.4](https://github.com/espressif/esp-idf-provisioning-android/blob/lib-2.4.4/provisioning/src/main/java/com/espressif/provisioning/ESPDevice.java)：应用配置后查询设备状态，明确成功、设备拒绝与传输失败是不同回调；切网断链可能无法判断设备最终结果。本批没有替换SDK或重造协议。
- [匹配Arduino Core 3.3.11](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/WiFiProv/src/WiFiProv.cpp)实际使用`network_prov_*`；本机原SDK头文件的默认停止合同与上述语义一致，不能把旧`wifi_prov_*`名称直接套进设备修改。

设备库只读核对：`Esp32WiFiProv.ipp`的CRED_SUCCESS仅记成功，END／DEINIT分别记录生命周期；普通产品等Complete后才切active。未发现拿到IP或CRED_SUCCESS就提前强停服务的本库分支，因此本批不改设备停机逻辑。

**已复现的是适配层的确定竞争，不是已证明§34.4真实UNKNOWN的唯一根因。** 原回调日志未保留足够分支，不能仅由7675ms、云上线或新host用例反推当时SDK已回成功；原UNKNOWN及所有原始日志继续保留。

### 35.2 原生最小修正

仅对空根首次接入的typed `provisionBlinkerWifi`接受原SDK成功终态：仍核同live owner、同request／ticket、原Security1／非空原PoP及无`no_pop`，不再用成功回调排队后AP是否仍存在的快照否定该终态。此时只完成本次调用／清其deadline，**不关闭owner、不释放NetworkRequest／Cloud租约，也不授权下一次写入**。

先发生onLost、取消／关闭、超时或安全上下文漂移仍失败／UNKNOWN；旧／重复回调不能翻转终态。Bootstrap、只读、非终态progress及generic provision保留原绑定检查，没有通用放宽或新增等待。

### 35.3 只读协调与页面收口

复用原`POST /api/v2/devices:resolve-instance`，增加显式`X-Blinker-Contract: provisioning-cloud-v1`窄读取变体；旧合同不变，无新路由、迁移、Broker功能或准入分配。请求只含原16B instance及已确认设备CAS；owner-only、所有响应no-store、15秒有界取消、不认证重放。只读既有租户路由、Broker在线／Manifest／State，再由State.sessionId关联PG当前有效会话的instance、凭据、租户、分片和routeVersion，结束再次核owner／CAS。不返回sessionId、token、Key或签发材料；证据不足返回未确认／不可用，不能用普通在线布尔值替代。

App将“原WiFi回执未知”与“观察到匹配云上线”分别展示，沿用原样式、语义状态提示及显式按钮，不提供重发／重新开始。每次用户操作只读取一次；顺序必须是**原匹配owner真实死亡→保留sticky隔离并核Cloud前提→新鲜只读查询**，死亡未知或全局Local租约仍占用时不发Cloud请求。ready、时间及前台恢复均不解除隔离。

只有instance／账号／authority／sessionEpoch／CAS一致，且云会话不早于本次WiFi调用、State不早于该会话时，才展示“设备已在云端上线”；这是关联观测，不是原子socket Ready或原RPC成功。用户随后可主动进入设备页，不走Direct handoff、不完成／删除allocation或UNKNOWN、不伪造ACK、不恢复旧阶段。

提交原WiFi后若后台／离页，保留本次不含密码、PoP或Key的公开checkpoint，取消并退休原代次；晚成功不升级，返回不连接／查询／重发，只能手动读。账号切换仅丢弃该页面的旧账号checkpoint，持久凭据、原日志和意图不清除。已接受精确WiFi成功后立即消费pending checkpoint，后续Cloud等待或导航不能反将其降为UNKNOWN。当前新增checkpoint仅保留当前页面生命周期，不新增冷启动导入／重放机制。

### 35.4 有限验证与未验证范围

| 范围 | 本批结果 |
| --- | --- |
| 原生适配 | 先红：SDK成功已排队／AP撤除被误判；修正后9个不同定向host方法通过，含先断链／先关闭、安全漂移、60秒budget、重复终态及generic旧门禁。原2.4.4材料校验值不变，非APK编译／无线证明。 |
| Gateway | 8项observer及3项HTTP叶子用例通过；旧resolve、owner／CAS／会话关联、无分配、早退no-store覆盖。无真实HTTP或数据库写入。 |
| App读取 | 15项真实HttpTestingController／原拦截器有限用例通过，覆盖401不重放、15秒截止、取消／晚结果、精确上下文和响应合同。 |
| App协调／页面 | 13项协调器、8项Page及1项原模板新增用例分批通过；另保留原晚ACK用例通过，未重复全套。WiFi在途后台／离页入口丢失、已确认成功被降级两处先红后绿；模板初测失败为zoneless夹具直接改值未通知检测，改用markForCheck后通过，未关闭Angular检查。 |
| 类型／审阅 | App及选定回归最终noEmit通过；新增源文件／有限diff检查通过。独立审阅核实际死亡、sticky、账号／CAS、取消和晚结果边界。 |

§35当时裸Node导入`src/index.js`找不到`./interface-workflow`，HTTP回归仅显式隔离这个未使用边界，若触及即失败，未擅自补模块。**当时不能把有限dispatcher测试称完整Gateway启动／部署通过。** §36已核实它是现存已跟踪的TypeScript文件，由原构建生成JS；前稿将源码直接消费错误称作“模块缺失”不准确，以§36.1为准。模板当时仅核语义与禁用，不称手机视觉／暗色或真机短验通过。

证据保留：插件`.build/ar4r-20261030`，App `build/access-ar4r-20261030`及`build/access-ar4r-cloud-client-20261030`，Service `build/access-ar4r-cloud-observation-20261030`；初次失败、定向修正和绿结果均保留，不进入源码交付材料。

### 35.5 当前唯一接续

**本批软件有限通过，未构建／安装APK、部署服务、操作COM11／账号／授权或执行真实业务请求。** 既有App／固件／云运行实现不因本地修正自动改变，§34.4历史UNKNOWN仍未知。未发送验证码、自动重试、升级SDK、提交或推送，无关本地修改保留；AR-4／报告#33整体仍开放。

下一批只先核Gateway完整消费者及缺失模块的交付归属，再决定必要的匹配候选构建／部署与一次UNKNOWN只读协调差异验收；无需重建身份、重发Bootstrap／WiFi或重复已过云上线／BLE／断电矩阵。若需要验证原生成功竞争修正，另以明确单次差异范围进行，不用既有UNKNOWN伪造新的成功，也不为消除未知无限采样。

## 36. AR-4s：匹配部署与一次UNKNOWN只读协调短验（2026-10-30）

### 36.1 完整消费者归属及最小部署范围

`services/gateway/src/interface-workflow.ts`是已跟踪、已合入的xwd源码，不是丢失文件。Gateway原`tsc`／`copy-interface-skills`生成`dist/interface-workflow.js`，正式package／Docker入口均消费`dist/index.js`；不能让裸Node消费混合TS源码后另补同名替身。本批没有修改workflow、公用源码、锁文件或SDK。

原workspace脚本仅构建实际contracts依赖及Gateway，分别12.0秒／9.2秒；源码树265文件SHA **`f16acbf7…4e3abb`**，构建前后不变。真实编译入口导入workflow及新observer通过，无Module替身、无startGateway，禁止网络／监听且尝试数0。静态246消费者核查发现本机未安装按需`mmdb-lib@3.0.3`，不安装或升级；新鲜远端核原运行镜像已有该依赖。四个本地旧dist孤儿产物既无源码也无消费者，不带入镜像、不删除用户本地文件。

与实际云端完整树比较，仅部署原tsc的 **3文件**：`index.js`、`device-v2/management.js`、新增`device-v2/provisioning-cloud-observation.js`。包含§35接线及已审AR-2设备GET全部早退no-store的5行必要配套；没有编译后手改、任务／定位新功能、迁移或依赖变更。单一归档SHA **`09c6eea1…7a4dc9`**；以固定原镜像为基底，仅FROM／LABEL／COPY，无pull／安装或本机Docker。

### 36.2 保全、部署与匹配产物

| 项目 | 新鲜证据及结果 |
| --- | --- |
| 原现场 | COM11仍是§34原身份／固件，COM9不动；本批没有串口打开、复位、刷写、NVS或设备配置。App原APK `46f4c994…2403b33`，同一正式Auth账号；完整session、3项安全存储、完整IDB／localStorage／logcat及已有私有HTTP／native记录保全。原两个任务／自动化journal各0行，不造记录。 |
| 云端保全 | 固定主机host key，仅操作现有`blinker-iot-cn-20261030`环境；原Compose、四容器inspect及完整Gateway／Broker日志私有保全，原镜像保留。PG只读核现有active设备／凭据1及instance，不创建设备、不调整额度或准入。 |
| Gateway匹配 | 新镜像 **`413c3279…8f802b`**；隔离无网络／只读image检查266文件runtime／contracts完整树SHA **`442a8424…731c9f`**及真实入口导入通过，原依赖不变。仅Compose的Gateway image字段替换，单次`up --no-deps --no-build --pull never --wait`，实际healthy、`/healthz`200。Broker／PG／Redis原容器ID、镜像、卷及健康不变；Auth仍`api.yiyu.pro`，MAPS关闭，原端口／挂载／全部环境变量相同。 |
| App匹配 | 唯一原工具链队列：模板21.8秒、Web22.4秒、sync9.9秒、离线Beta APK156.7秒，均成功且源码／HEAD／index不变；没有重复构建。APK **`3cf481ea…d7e079`**，同原签名 **`a76639ec…5d210`**，包名`iot.diandeng.tech.beta`、HTTPS／无cleartext、原生注册表及1156份Web资产逐字匹配。实际消费相邻插件新Java／TS源码，SDK仍2.4.4、校验值不变，编译class核到typed终态修正。 |
| 安装／终核 | 单次`adb install -r`成功，不清数据／切号／发码。安装后与最终快照的完整session、3项secure及完整两journal逐字相同；新Gateway／App保留运行，不恢复旧版本，原产物仍可回退。 |

远端私有保全在`/opt/blinker-access-ar4s-20261030`，本机Service／App各自忽略目录`build/access-ar4s-20261030`；部署只修改必要运行产物，不把凭据、网络参数或运维归档加入Git。

### 36.3 实际短验：一次显式查询，UNKNOWN不改写

安装必然重启App，而§35公开checkpoint仅在Page内存；旧allocation v1只存requestId／名称／CAS，没有WiFi提交与UNKNOWN时间，不能凭allocation伪造未知回执或声称有自然恢复入口。

本批执行前已明确限定为**已验证历史证据受控内存导入的真实只读链路短验**：离线核§34唯一typed调用／UNKNOWN、精确Bootstrap ACK、两个不同owner、同instance及当前原allocation／controller／账号／CAS逐项一致，再只导入原call时间和不含Key／PoP／密码的公开checkpoint。通过精确匹配APK的真实lazy Page与真实协调器，点击原“核对云状态”按钮 **仅1次**；未替换reader、服务、HTTP或返回结果，未写SecureStorage／journal。包装仅记录并禁止任何原生／HTTP写入或第二次请求，不用旧执行器的connect／send／continue。

| 核对项 | 实际结果 |
| --- | --- |
| 真实请求 | `POST /api/v2/devices:resolve-instance`仅1次，`provisioning-cloud-v1`，HTTP **200**；关联OPTIONS204。requestId **`5217e40d38b09f2075fdc950b45e91fd`**，响应`private, no-store`；约 **2.1秒**。CDP responseReceived2118.286ms与loadingFinished2111.954ms有约6ms反序，原值保留，不伪算负分段或据此另开性能优化。 |
| 关联证据 | 返回同COM11 instance、原logicalDeviceId／credentialVersion／locator；当前有效session `issuedAt=1793350873235`不早于原WiFi call `1793345994256`，关联State `1793350875371`，本次观测`1793351356311`。这是新鲜查询取得当前会话关联的最近State；该State距观测约8分1秒，**不是本次短验新产生的State，也不是原WiFi RPC成功证明**。 |
| 原页面结果 | 实际显示“Wi-Fi 配置回执未知”和“设备已在云端上线；原本地配网回执仍未知”，保留UNKNOWN、result为空、无成功收尾／意图complete。没有发送配置、首装或重新开始控件；只读结束后显示进入设备与再次核对，未再次点击或进入控制页。界面按原状态语义／显式动作检查，单手机竖屏截图可读、不溢出；不称暗色、多机型或375px矩阵完成。 |
| 原生及业务次数 | WiFiProv仅capability／sticky读取、监听及stop／clear回收；**create／connect／scan／Bootstrap／typed或generic WiFi／custom均0**。CREATE／reveal／Admin／控制命令均0，未延长或启动配网窗口，未触发旧阶段／Auth／Direct恢复；查询无第二次提交。 |
| 回收与保留 | 正常回首页，真实Page已active=false／destroyed=true，临时公开checkpoint清理；原native包装、断点、Fetch拦截、observer／binding／对象组均按脚本完成移除，工具exit0。最终仅App主进程，9479及本批转发为空。旧events／audit哈希、账号／授权／完整journal保持；未重放UNKNOWN或抹掉历史原记录。 |

Gateway部署可能导致设备正常重新建立Cloud会话，实际本次关联会话产生于部署后；这不是重新配网或重发业务请求。运维before／final仅保存按创建时间最早的10条会话样本且样本一致，**不能把该有界样本称整个会话表没有新增**；以本次实际HTTP关联的当前会话为准。容器最终四者healthy／restartCount0，除Gateway外均未重建。

独立离线终审与`final-audit.json`核上述唯一请求、精确关联、禁止写入、前后四份账号记录、历史哈希及Page销毁；保留全部本批请求／native／截图及原始时序。仅200成功路径，不新增401禁认证重放、故障取消或原生终态竞争的真机结论；这些有限软件证据仍取§35，不重复制造失败样本或启动新WiFi调用。

### 36.4 当前结论与下一批最小缺口

**匹配Gateway／内测App部署及一次UNKNOWN真实只读链路有限通过。** 本批不把“已上线”改成原RPC成功，不将历史证据导入称自然冷启动恢复，AR-4／报告#33整体不关闭。

唯一明确产品缺口是：UNKNOWN发生后安装／冷启动缺少可自然恢复的只读入口。下一批先设计并实现最小、非秘密、有界的“WiFi实际已提交／原时间／instance／CAS／账号来源”持久checkpoint，复用现有allocation与账号生命周期；仅同账号、显式恢复只读，不导入历史私有日志、不提供自动重发／新身份／旧阶段继续，明确完成／切号／撤权／过期清理语义，再补有限回归。不能仅凭旧allocation推断已提交或补造既往UNKNOWN；本批原记录继续保留。已有根手动写及整体验收仍维持原支持边界，不借本批开放。

本批无验证码、自动业务重试、SDK／依赖升级、提交或推送，无关本地修改保留；COM11身份、固件、授权与原UNKNOWN保持，不重复BLE／H2／断电或已过首装矩阵。

## 37. AR-4t：同账号UNKNOWN持久只读恢复（2026-10-30）

### 37.1 主线与本批边界

当前仍是基础设备接入：修复配网完成／异常收尾，不是自动化、任务、地图或新的通信协议。§34已有真实一次WiFi调用和云上线，§35补回执竞争／只读协调，§36完成匹配部署与真实只读链；本批仅补“App重启后找不到原UNKNOWN核对入口”。不重新解绑或创建身份，不重发Bootstrap／WiFi，不重开已通过矩阵；不部署、构建APK、操作设备或提交推送。

### 37.2 冻结持久化、过期与清理合同

- 仅新版typed WiFi已实际invoke后进入真实UNKNOWN（包括该inflight因真正后台／离页取消），才保存checkpoint。普通allocation、Bootstrap ACK、公开GetInfo、旧日志或旧版本记录均不能补造UNKNOWN；精确成功不生成该记录。**本批不新增写前事务，实际调用至UNKNOWN成功持久化之间突发杀进程仍可能丢失恢复提示；不能宣称跨原生／存储原子提交或零丢失。**
- 复用原authority＋accountId＋16B instance键域及allocation锁，在`record.wifiUnknown`保存exact `{version:1,status:'unknown',attemptedAt,expiresAt}`；requestId、名称、CAS沿用原record，instance及账号沿用原封套。无DeviceKey、controllerSecret、PoP、WiFi参数、token、旧owner／lease／ready／sessionEpoch。
- TTL固定为原提交时间起24小时，读取、失败或再次核对不续期；时钟倒退至提交时间之前则禁读，过期同样禁读。保留原metadata及日志供核对，不初始化自动删除、不驱逐旧意图。恢复时捕获当前登录epoch，每次await前后仍核账号与代次，不能把旧epoch当冷启动凭据。
- 本地枚举总keys最多2048，同账号allocation最多32，带checkpoint条目最多8，单记录有界并独立校验新字段；超限、损坏、未知字段或不支持原生存储均fail-closed，不投影覆盖或删除旧字节。旧v1无新字段只跳过，零写入、零迁补。
- 新增`saveUnknown／listUnknown／clearUnknown`薄存储能力；save需原record与CAS一致，相同提交时间幂等不续期，不覆盖另一笔attempt。手动移除恢复提示须精确requestId／CAS／原时间，仅去掉新metadata，原allocation继续承担禁写fence；正常设备移除仍走原retirement／complete，不另造解绑实现。
- 切号／退出只清页面条目及取消旧读，不删除旧账号隔离记录。404／NOT_CURRENT／CAS变化禁用该条恢复，不能用新CAS续旧请求；401／暂不可用保留并等待显式操作。Cloud online仅是关联观测，**不清除或改写原UNKNOWN，不complete allocation、不启动Direct或旧阶段**。

### 37.3 最小接线与有限验证范围

首次账号恢复后仅列本地公开条目，不自动Cloud查询；用户显式选择“核对云状态”才复用§35原协调器、真实owner清理／sticky、当前owner／CAS与原提交时间门禁。过期／时钟异常条目不读取Cloud，可明确移除恢复提示；读取失败返回前台不重试。后台／离页继续退休旧代次；持久化失败明确提示不能承诺重启恢复，不重发也不自动重试保存。

只补新checkpoint存储、真实UNKNOWN保存接线、同账号显式只读入口及对应有限回归，界面复用现有样式／状态区域。测试覆盖旧记录零迁补、原时间／CAS、重启／切号／过期、精确清理、晚结果隔离及无自动请求／写重放；实施结果续记本节，不以软件回归替代真机。

### 37.4 实施结果与有限回归

本批按冻结合同实施，**软件有限通过，未部署或实机验收**：

| 范围 | 实施与结果 |
| --- | --- |
| 纯合同／存储 | 新`wifiprov-unknown-checkpoint.ts`独立负责exact元数据与24h期限；原allocation仅新增可选`wifiUnknown`，沿用原SecureStorage键域、锁与scope复核，无第二套身份／执行链。新增36个不同有限用例分批通过：旧记录零写、实际新metadata跨store读取、幂等不续期、CAS／时间冲突、账号隔离／并发、过期／回拨、2048／32／8及4KiB边界、损坏不修补、精确移除、可变调用者快照、正常retirement及无Web存储fallback。 |
| 真实UNKNOWN保存接线 | 仅原first-install typed操作实际调用后进入UNKNOWN或被真正后台／离页取消，才调用`saveUnknown`；保留原提交时间、instance及CAS。5秒整次存储观察截止，保存未确认只提示可能无法重启恢复，不自动再存／重发；离页仍可完成原账号的这笔公开记录保存，销毁不删除durable。精确WiFi成功在最终Cloud等待前消费pending，不生成UNKNOWN。3项真实Page／原typed port有限用例通过，无真实无线或存储写入。 |
| 冷启动恢复页面 | 初次账号恢复→同账号本地枚举→显示公开条目，初始化／回前台无Cloud查询或重试；当前epoch只用于本次读，旧owner／lease／网络许可不导入。用户选中具体条目后才走原协调器，实际清理／sticky及账号／CAS门禁不变；HTTP前后均重新读取这笔本地记录，过期、CAS变化或在途清理时不发布online。404／409禁读保留记录，401／暂不可用仅手动恢复。17个不同新用例分批通过（原15项＋单独2项截止／在途清理），无真实HTTP。 |
| 原模板与类型 | 两项真实模板用例通过，覆盖显式按钮、过期禁用、清理仅提示及保存未确认反馈；未重跑旧UI矩阵。App及选定回归最终noEmit、必要diff检查通过。UI复用原语义状态区域、样式和控件，无新组件库／CSS重构。 |

共 **58个不同新增有限用例分批通过**，不是完整跨端集成测试。新增Page层已将初始加载代次固定：后台取消后的晚本地结果不能借新generation重新初始化插件；选中UNKNOWN后普通配网操作继续禁入。旧v1没有metadata仍无恢复提示；§34／§36原UNKNOWN不被追补，已有设备／授权／完整请求日志未触及。

有限证据位于App忽略目录`build/access-ar4t-20261030`：store、Page、两项新增边界、原typed接线与模板日志及类型结果分别保留。测试只替代原生、存储与HTTP边界，使用真实Page／原typed port／协调器；不称已有新APK、真实重启恢复、手机视觉或iOS验收。未构建发行包、连接手机／串口、访问云端、解绑、发送验证码、自动业务重试、升级SDK或提交推送；无关本地修改保留。

### 37.5 收口与下一步约束

当前主线仍是 **BLE／SoftAP发现连接→原身份与WiFi下发→云上线及异常收尾**。本批只补最后一段的持久只读入口，不能把AR-4／报告#33整套入网写成完成，也不转向自动化／任务扩展。软件修改到此冻结，避免再为同一UNKNOWN无限加诊断或循环复测。

后续如安排匹配App短验，只验证本合同的新记录跨正常重启可见、同账号显式只读及清理；无新版真实UNKNOWN时如实保留待测，不从旧allocation／私有日志补造，也不为凑样本反复解绑／清根／重发Bootstrap或WiFi。是否需要一次新受控入网由下一批实际缺口决定，本批无需解绑。其后回到基础设备接入剩余清单按BLE／SoftAP分别收口，不把历史已通过项重组成新队列。

## 38. AR-4u：匹配App安装、真实样本门禁及回到配网清单（2026-10-30）

### 38.1 先核真实记录，不生成测试UNKNOWN

本批沿用现有云服务及COM11身份／固件，不打开串口、不复位或刷板，不解绑、创建身份、刷新Key或下发Bootstrap／WiFi。只保全App账号／授权／完整日志、安装§37匹配候选，再按真实记录决定验收范围。

新鲜before快照：已安装§36 APK **`3cf481ea…d7e079`**；同一账号、完整session及3项secure保全，两份原任务／自动化journal各0行；完整localStorage、IDB、logcat及既有私有请求证据保留。当前只有 **1条原allocation、新`wifiUnknown`为0**。§37此前未部署，旧记录没有真实新metadata，不能从§34日志、原allocation或内存fixture追补。故本批执行前已将正向三项标为`NOT_RUN_NO_NEW_REAL_RECORD`，不制造样本。

### 38.2 唯一匹配构建与安装

复用§36四个已审运维脚本，固定原脚本SHA，仅内存派生新批目录／文件URL及路径，记录原／派生SHA；没有复制第二套执行器或修改公共源码／锁文件。唯一队列：模板 **20.945秒**、Web **20.117秒**、sync **7.620秒**、原离线SDK Beta APK **79.251秒**，均通过且源码／HEAD／Git index不变。未重新运行§37已过58项回归或其他平台矩阵。

实际APK **`ab7ab17e…7bd47c`**，同签名 **`a76639ec…5d210`**；包名`iot.diandeng.tech.beta`、HTTPS／无cleartext、关闭bridge明文日志、原生注册表及 **1156份Web资产**逐字匹配。仍用原Node／JDK／SDK2.4.4及相邻插件源码；native class与§36一致，无SDK或依赖升级。额外跨chunk核新save／list／clear／recover／dismiss和metadata消费者，并静态核实际产物`expiresAt = attemptedAt + 86400000`，不能只凭源码存在称新功能已打包。

单次`adb install -r`成功，不清数据；新版保持安装。紧接安装的首个本地CDP快照遇到`UND_ERR_SOCKET`，发生于WebView目标列表读取之前，未开始账号快照或业务请求。保留该停止记录后，独立核新PID的调试socket已出现，另做一次明确的`installed-confirmed`只读终核；未重复安装、构建、登录或发码，工具无自动重试器。该完整session／secure／journal与before逐字一致。

### 38.3 一次普通冷启动的空态结果及待测项

只对App执行一次进程终止／重新打开（`force-stop`／`start -W`），PID **32504→1908**，不清App数据、存储、缓存或绑定。新进程确认为同一匹配beta；页面经原路由／真实组件初始化，无私有上下文、reader、checkpoint或结果注入。

| 项目 | 实际结果 |
| --- | --- |
| 真实本地恢复加载 | 同账号配网页完成原本地枚举，loading结束、error为空、恢复条目0，`wifiReceiptUnknown=false／cloud=idle`。旧allocation没有被补写metadata；冷启动前／页中／离页后三份原secure逐字一致。 |
| 无自动业务操作 | 目标页观察窗内`resolve-instance`请求0，设备API mutation0；native仅原capability／监听／存储读取及退出回收。create／connect／scan／Bootstrap／typed或generic WiFi／custom／reset及SecureStorage写入均未发生。未点击重读或其他已过按钮，不声称整个App启动零HTTP。 |
| 回收与保留 | 正常回首页，原Page为`active=false／destroyed=false`（Ionic缓存页，不冒称销毁）；原native函数引用精确恢复，Fetch／监听／binding／对象组及本批转发回收。最终仅App主进程、无配网worker；最终session、3项secure及完整两journal仍与before相同，原UNKNOWN／日志不改写。 |
| 正向持久记录重启恢复 | **待测／NOT_RUN：没有新版真实记录。** 空列表不能代替真实UNKNOWN跨进程恢复。 |
| 同账号显式Cloud核对 | **待测／NOT_RUN：没有可选的新版真实记录。** 不复用§36历史证据导入工具，也不发只读请求凑样本。 |
| 精确清理新metadata | **待测／NOT_RUN：没有可清理的新记录。** 原allocation、授权、任务／自动化journal及云scope不删除。 |

本机App忽略目录`build/access-ar4u-20261030`保留原APK回退副本、所有前后快照／日志、原／派生工具校验、产物证明、一次重启及冷页观察、截图和停止记录。没有额外服务部署／健康采样、设备操作、验证码、自动业务重试、SDK升级、提交或推送；无关本地修改保留。本批结论仅为**匹配安装及冷启动空态有限通过，三项正向验收仍待真实样本**。

### 38.4 BLE／SoftAP是否均已完整复测：明确答案与剩余清单

**两条均有真机证据，但不能说均已完整入网通过。** BLE WiFiProv是WiFi设备的配网通道，不是普通Direct BLE控制；以下不混用这两类证据。

| 路径 | 已冻结的实际证据 | 最小剩余／约束 |
| --- | --- | --- |
| BLE WiFiProv（§17） | APK `bb9333bf…bcc00`、原SDK2.4.4／Core3.3.11：发现／选择、connect1235ms、设备WiFi列表6147ms、GetInfo153ms及离页回收通过。当时已有根缺当前App Admin，身份／WiFi写入0。 | **完整身份→WiFi下发→云上线未取得该路径成功结论。** 下一批先核当前COM11／App Admin是否匹配及BLE配网固件来源，再只补未完成段；§17是当时缺凭据，不能据此认定§34新绑定后仍缺，或先解绑／清授权。 |
| SoftAP显式手动（§24／27／34） | 连接／WiFi列表／固定GetInfo及后台回收已有证据；§34原窗口进一步取得新owner、同instance、精确Bootstrap ACK、唯一WiFi调用和实际云上线。 | WiFi本地终态为UNKNOWN，完整App成功收尾未关；§35适配修正在匹配App内，但尚无新的无线成功终态。**不重跑已过写入／云上线矩阵，不把online观测改成旧RPC成功。** |
| SoftAP自动managed（§22） | PGEM10／Oplus实际系统授权、截止及worker死亡已核，但SDK指定网络仍未连接成功。 | 此OEM自动入口继续冻结；不能用手动结果替代，也不加宽后台、延长等待或用ready解隔离。无新差异证据前不重复循环。 |
| UNKNOWN恢复（§36～38） | 真实只读链已有限通过，持久软件58项通过，新匹配App及空态冷启动已验。 | 上述3项正向保留待测，不再为同一提示制造UNKNOWN或独立开启新首装回环；原授权／完整日志继续保留。 |

**下一批主线优先回到BLE完整入网缺口的前置核对与最小补段方案**，不是继续扩展UNKNOWN、自动化、任务、OTA或新网关。当前App／服务／身份先保持，BLE与SoftAP的缺口分别跟踪；如下一批确需新WiFi写入或匹配固件切换，应明确本批次的新操作范围，不能沿用本批“不重发”的许可直接执行。AR-4／报告#33整体仍开放。

## 39. AR-4v：现有身份的BLE WiFi-only补段（2026-10-30）

### 39.1 最小范围与前置发现

本批只补BLE WiFiProv已有根的身份核对→一次新WiFi下发→新云会话／State→成功收尾，不重跑§17扫描、连接性能或SoftAP矩阵。§34后本机已有同instance、epoch1、权限15的active Admin；§17缺本机授权的旧前提不再成立，不默认解绑／清根／新建设备。

源码发现BLE显式重配仍先调用`admin.resume(active)`，产生不必要的Key reveal与Admin重新登记；原`inspectExisting`也含reveal，不能冒称纯只读。先限定补齐显式BLE重配的只读准入：新鲜owner context、instance／Cloud CAS、本机active无rotation且完整权限、公开Admin一致。通过后仅记本次内存WiFi-only身份，不恢复旧allocation、不reveal／rotate／register／Bootstrap；成功也不消费§34原allocation。pending、账号或epoch／CAS不符继续禁入。

### 39.2 固件、状态保留与验收合同

复用原Core3.3.11已完整链接BLE候选`103cdde6…a1b4`（1,372,128 B，0槽，原Security1），不重新编译固件／升级SDK。COM11新鲜核对USB MAC、分区、当前应用与NVS；仅写应用区，不改分区／bootloader，完整保留instance、DeviceKey、Admin、TaskStore及原UNKNOWN证据。已有WiFi时原产品不会打开配网窗口，可通过候选既有显式`resetNetwork`仅清网络以进入原10分钟窗口；此操作不是解绑，不调用access reset，前后核对根／instance及任务摘要，所清网络由本次唯一WiFi下发重新写入。

App账号／授权及完整journal／日志先保留；必要的匹配App修正只构建安装一次。云端先核owner只读Admin入口，若公开代理仍阻断该GET，须记录实际停点并仅修正必要只读入口，不能绕过owner／CAS或改用Key reveal。WiFi若返回UNKNOWN即停止，保留新旧记录，不重发。只有真实成功回执、新云会话／新鲜State及原页面成功收尾均有证据才能关闭本批；原SoftAP UNKNOWN与其自然重启正样本仍独立待测。

### 39.3 最小修正、构建与前置实核

App新增`inspectExistingNetwork`，复用原严格事实校验；原isolated `inspectExisting`仍保留其Key核验合同。Page仅显式BLE完整root重配先行resolve／CAS及Admin核验，设置本次内存WiFi-only标记；不读旧allocation，不调用resume／reveal／register／Bootstrap，成功不清原记录。4个受影响spec **104项通过**，App／spec两套noEmit通过；不是重复无线矩阵。

新鲜App快照确认原账号、session、3项secure及两份完整journal保全。当前COM11 USB MAC、instance与epoch1／单Admin吻合；当前SoftAP应用与既有BIN逐字比较通过。20 KiB NVS严格CRC解析及精确读回，原分区factory容量5 MiB；只写BLE应用`103cdde6…a1b4`并验证，写前后NVS逐字不变。随后通过候选既有`n`明确执行一次`resetNetwork()`，仅清WiFi、开启原10分钟窗口，不清Key／root／TaskStore。没有解绑、分配新身份或恢复首装前旧NVS。

当前owner Detail GET为200／no-store，但Admin GET实际405。仅修正现有可选精确路由的方法guard为GET／PUT／OPTIONS，Gateway原owner鉴权不变；query、PATCH／reset禁入、no-store及禁止上游重试保持。相关renderer **30项有限回归通过**。云端只改该guard并reload一次，不重建容器／镜像；部署即时合并终核曾断言停止，保留失败证据、不重复写配置，后续独立只读终核确认：无token GET／PUT均401，OPTIONS204，PATCH405，reset404；同原owner Admin GET200且controller／epoch匹配。四容器仍同ID／镜像、healthy、restart0。

唯一匹配App队列：模板13.153秒、Web18.817秒、sync6.839秒、原SDK APK60.067秒；源码／HEAD／index全阶段不变。APK **`b2aec069…34c660`**，原签名`a76639ec…5d210`、HTTPS、1156份资产逐字匹配；实际Page chunk核新只读方法及内存标记。原生class／SDK2.4.4与§38一致，固件Core3.3.11不变。一次`install -r`保留数据；本地包装器首次因自身intent同名在ADB前停止，修正工具命名后才执行这唯一安装，不是重复安装或业务重试。

### 39.4 一次BLE保留身份入网短验：通过范围

只执行原重新配网页的一次必要发现／选择／连接前置，没有另开扫描、连接性能测试或重试。原设备WiFi列表正常显示`mostfun2026`，经真实表单只下发这一个目标网络；使用原Page／WiFiProv调用，未注入身份、回执、上下文或云端结果。

| 本次未完成段 | 实际结果 |
| --- | --- |
| 身份接入／保留 | 唯一GetInfo请求2 B，返回24 B、status0／flags7，instance仍`6d692032…13fc`／epoch1；resolve-instance及Admin GET200，原ID、Cloud CAS、同controller核验通过。此分支是**核对原身份，不是重新安装身份**。 |
| 一次新WiFi下发 | 原generic `WiFiProv.provision` **1次，7629ms后success=true**；非typed首装、无Bootstrap／Install。新操作不是重放§34 UNKNOWN。 |
| 云上线与新鲜State | 原设备页显示云端已连接；原WS按方向／requestId流式重组，尾部0字节，214份完整MQTT／BBP路由帧。原设备Presence false→true；本次StateRequest seq7／新requestId在 **52ms** 后收到同peer／同requestId StatePage，revision1、power=false，证明新鲜Cloud读而非缓存。随后同peer实时stream144个样本形成。用户wire不含设备cloudSessionId／SessionReady，不能把App WS ID冒称设备新sessionId；云可达由新Presence＋精确State回应证明。 |
| 成功收尾 | WiFi成功后原`clearState` **23ms完成**，App进入同一设备ID；最后正常回首页。未请求Key reveal／rotate、Admin PUT／reset、CREATE或Bootstrap；唯一非GET业务HTTP是只读resolve-instance POST。 |
| 身份／记录终核 | 最终NVS根记录摘要、instance记录、TaskStore摘要及分区与新鲜before精确一致，仅WiFi按本次预期更新。App session／3项secure及两份journal逐字同before；旧allocation／原UNKNOWN仍保留，没有新增`wifiUnknown`。无配网worker，本批binding／native包装与转发回收；普通App MQTT常驻不当成配网泄漏。 |

App忽略目录`build/access-ar4v-20261030`、库同名批次及服务同名批次保存前后私有快照、完整请求／原生日志、唯一构建／安装、应用及NVS验证、截图、只读流审计与运维停止记录。保留当前匹配App、BLE候选和原授权／云scope，不回滚到首装前身份；云端必要只读入口保留。无验证码、自动业务重试、SDK升级、提交或推送；无关本地修改保留。

冻结流审计输入SHA`a7857d99…d4d4f`，`audit-readonly.json`保留计数与关联。附带发现：设备页第一笔TelemetryControl曾返回Error5／UnknownEndpoint，下一Open获Status并建立上述stream；**不是待定位Error8，也不能写全程零错误**。本批没有为此改租约／轮询、重发WiFi或扩大实时验收，保留原日志供单独必要差异分析。另原页面仍有“身份已写入”旧文案，在本次已有根分支应理解为核对；后续若统一文案仅做小修，不据此再跑本次无线矩阵。

### 39.5 收口与剩余

后续执行以[全流程入网完成方案](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md)为准：ON-1～3软件有限通过，ON-4待匹配App短验；本节以下是AR-4v验收结束时的冻结结论，不另开重复BLE队列。

**BLE WiFiProv已有根、匹配Admin的保留身份重配缺失段本批有限通过。** 这不是空根BLE首装、所有设备组合或整个AR-4的无限外推。§38.4的BLE行由本批结果接续，不再回到扫描／连接循环。

SoftAP显式手动仍保留§34原UNKNOWN，不能用本次BLE成功改写；自动managed在该OEM仍冻结。新版UNKNOWN的自然正样本恢复仍无记录，不造样本。下一批回到SoftAP剩余成功终态与原身份安全支持边界，先核是否有**新的、必要且不重放旧UNKNOWN**的操作条件，再安排最短差异验收，不默认清根／解绑；不再扩展同一UNKNOWN提示或重跑BLE。
