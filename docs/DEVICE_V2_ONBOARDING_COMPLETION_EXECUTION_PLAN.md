# Device V2：全流程入网完成判定与结果页执行方案

更新：2026-10-31。当前结果取§30：新的明确单次授权后，BLE非空PoP空根首装真实有限通过，独立新身份／CAS／Admin、Bootstrap精确ACK、唯一WiFi官方成功及本次Cloud新会话／新鲜State成立，并原正常前台自动进入。新BLE NVS／scope及完整日志保留，COM11已精确恢复c051＋73d949完整NVS和原分区，按合同留ROM；原5项记录及新增2项均保留，UNKNOWN不删不重放。匹配App／49项冻结运行输入保持，临时3台仍按原到期保护。BLE与§26的SoftAP两条最小首装成功链均可冻结，不再默认配网或验证码循环；自然UNKNOWN正样本／其他矩阵不冒称完成，无SDK升级、服务部署或提交推送。下文旧“当前／下一步”为阶段历史，不再构成重跑队列。

## 1. 目标与现状

主线是用户通过BLE／SoftAP完成WiFi设备接入，不是新增Direct BLE、任务、自动化或OTA。保留现有WiFiProv、身份分配／Admin、CAS、owner隔离及只读协调链；补产品级“添加完成”判定，不能以列表刷新或导航替代设备可用证据。

已有证据冻结：BLE已有根重配在§39完成一次成功下发、原身份保留、Cloud Presence及新鲜State；SoftAP手动§34云端上线但原本地回执UNKNOWN；自动managed在当前OEM仍冻结。新版UNKNOWN自然正样本恢复仍待测，不制造记录、不重放旧请求。上述结果不重跑。**2026-11-04 规则修订：原设备重新配网还应轮换 DeviceKey。** 上述仅换网络的记录不代签新规则；新接线及验收取[换网与换 Key 执行方案](DEVICE_V2_WIFIPROV_REKEY_EXECUTION_PLAN.md)。

## 2. 官方合同与产品边界

- 原WiFiProv负责发送／应用配置及查询连接状态，本批不新增第二套官方查询器、不升级SDK。
- 官方配网服务正常先响应连接状态再自动停止，并非收到凭据立即关闭SoftAP；信道变化和手机切网仍可能造成查询结果丢失。
- 当前设备适配在CRED_SUCCESS只置状态，后续等待官方END／DEINIT；不盲加2～3秒固定延迟。
- 参考成熟产品将网络配置、WiFi连通、云端绑定／可用分阶段呈现，不假定“发现mDNS即成功”。

参考：[乐鑫停止合同](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32/api-reference/provisioning/wifi_provisioning.html#when-how-to-stop-the-provisioning-service)、[官方SoftAP信道已知问题](https://github.com/espressif/esp-idf-provisioning-android/releases/tag/app-2.0.2)、[涂鸦云激活](https://developer.tuya.com/en/docs/app-development/active_wifi_channel?id=Kdwxq4a3u289h)。这些是参考，不将其内部算法或超时值冒称本项目事实。

## 3. 单一操作与双事实合同

一次用户发送操作：连接设备 → 核对身份 → **下发一次WiFi** → 确认设备可用 → 展示结果。身份核对仍沿原fresh owner／instance／epoch／Cloud context CAS，不能借本批跳过Admin门禁。

| 事实 | 值／来源 | 禁止推断 |
| --- | --- | --- |
| 官方本地回执 | confirmed／rejected／unknown；原WiFiProv终态 | Cloud上线不能把旧UNKNOWN改成官方成功；通道断开不是认证失败。 |
| Cloud可用状态 | checking／online／unconfirmed／unavailable／blocked；原owner-only只读观察 | 页面ready、列表缓存、手机互联网恢复都不是设备上线证据。 |
| 产品结论 | ready／wifi-connected／failed／unconfirmed／checking | 超时不能推断设备一定失败；已确认WiFi不能因Cloud暂不可用降为WiFi失败。 |

调用WiFi前固定公开attempt快照：账号／authority／sessionEpoch、设备ID、instance、凭据版本与locator、原始attemptedAt；不包含密码、PoP、DeviceKey或controllerSecret。不从旧allocation／日志补造attempt，不延长旧时间。

Cloud确认沿用原接口，必须当前owner／instance／CAS一致、本次提交后的新鲜会话及其State一致。误配、旧会话、过期／时间异常及权限变化禁入。Cloud成功只更新设备可用事实，不改官方回执、旧UNKNOWN或旧allocation。

## 4. ON-1：独立完成器与有界Cloud观察

### 4.1 模块职责

新纯模块只接收官方回执和只读端口，负责结果归并、整次截止、取消和资源回收；不依赖Angular、原生SDK、存储或身份写端口。原单次协调器抽通用观察能力，UNKNOWN兼容类继续保留`localReceipt=unknown`与显式one-shot合同。

Page薄绑定负责原owner真实关闭、Cloud网络确认、原管理接口调用和账号／页面代次检查。共享现有验证逻辑，不复制一套CAS或响应解析器。

### 4.2 观察不是写入重试

原SDK的配网整次60秒截止不变。新增确认阶段初值 **30秒整次预算，包含退出owner、确认手机网络和只读请求**；不是2～3秒后默认成功，也不等待设备10分钟开放窗口耗尽。

同一次仍在前台的用户操作，可在确认阶段起点0／2／5／10／20秒最多进行 **5次串行只读观察**：仅收到合法`unconfirmed`才推进下一观测点。慢请求越过的时点直接跳过，不排队补发，不并发／catchup。到期立即结束观察，不等待不合作的Promise返回；晚结果不发布。

HTTP错误、unavailable、401／403／404／409、CAS／instance变化、close未确认均立即停止，不自动重试读取。用户后续点击“再次检查状态”才开新只读窗口，仍沿用原attemptedAt；不重发WiFi、Bootstrap、Key或Admin操作。

本段是新增产品完成流程的明确观察合同；不把原UNKNOWN恢复器暗改为轮询器。App重启恢复、返回前台、账号恢复仍不自动发起Cloud读取。LAN不接本批，也不阻塞Cloud。

## 5. ON-2：Page薄接线与生命周期

1. 只有原WiFi调用确实即将调用时才生成本次内存attempt；前置身份失败仍为前置失败，不冒称WiFi写入未知。
2. 官方confirmed：保留成功事实，退出owner→确认Cloud→结果页，不自动100%完成／getAllInfo→导航。
3. 官方unknown：保留原UNKNOWN和既有持久记录；同次前台流程可进入只读确认，结果online时主结论ready，但官方回执仍unknown。
4. 明确认证失败／找不到AP或明确negative结果：显示失败原因，不继续Cloud观察。通用SoftAP错误不能捏造具体原因；原生原因进一步结构化另列，不扩大本批Java接线。
5. 每次Cloud读取前后检查同账号、authority、页面generation、原identity CAS；先取得匹配owner真实死亡证明再解除Cloud隔离，不以等待或ready代替。
6. 后台／离页／销毁／换号取消观察，移除timer／listener，在途读取可中止、晚结果丢弃；返回只展示事实、不自动查或发送。
7. 新完成状态存在即禁普通配置写入口。进入设备为用户显式导航，只在已确认Cloud可用时开放；UNKNOWN导航不完成旧allocation、不触发Direct handoff。**本批所有导航均不自动complete allocation**：旧UNKNOWN可能尚无metadata，不能按设备ID误清。原正常retirement和明确清理合同不改；现存有界容量未扩大，后续若收口成功意图，仅可凭本次确切requestId另定消费合同。

### 5.1 UNKNOWN保留范围

沿用原typed首装的真实UNKNOWN存储、24小时TTL、CAS、精确清理和恢复入口，不新增存储schema，不改旧时间。本次普通BLE也有内存attempt，可只读确认；若没有匹配的真实allocation，不伪造durable记录，明确提示其本次状态核对的边界，不能声称已完成普通BLE跨进程持久恢复。

## 6. ON-3：复用现有页面的结果区域

不另建路由／组件库，复用status-card、既有主题token、按钮及安全区。进度来自真实阶段，不用计时器伪造百分比；诊断细节渐进展开。

| 证据 | 主结果 | 主要操作 |
| --- | --- | --- |
| confirmed或unknown＋可信Cloud online | 设备已上线／添加完成 | 进入设备；unknown只在详情注明“通过云端确认，原配网回执未收到”。 |
| confirmed＋Cloud未就绪／不可查 | 已连接WiFi，云端尚未就绪 | 再次检查状态，不显示WiFi失败。 |
| 明确官方拒绝 | WiFi连接失败及已知原因 | 给出检查网络／密码指引；本页不自动重发。 |
| unknown＋到期仍无可信证据 | 暂未能确认设备是否联网 | 再次检查状态；不默认要求解绑／清根。 |
| owner退出／账号／权限／CAS禁入 | 无法继续确认，说明所需操作 | 保持隔离／原记录，不展示错误的成功入口。 |

结果同时显示“WiFi回执”和“Cloud确认”两项摘要；UNKNOWN不盖过可信上线主标题。使用可读文字、`role=status`／`aria-live`／busy状态；动态错误可被辅助技术感知，禁用按钮不能触发操作。触控目标至少48px，窄屏／横屏／大字／减少动态效果沿现有布局兼容。

## 7. 本批有限验证与禁止范围

- 纯模块：完整结果映射，30秒／5次上限，串行与跳过时点，异常停止，取消／不合作晚结果，账号／CAS失效，资源释放；无写端口。
- 原读协调：原UNKNOWN one-shot、时间／session／State／instance／CAS及owner门禁继续有效。
- 真实Page：官方confirmed不会立即导航；unknown＋Cloudready不改回执／日志；owner真实关闭前零读取；后台／换号不发布晚结果、返回不自动查；超时不失败／不重发；显式进入保留原清理差异。
- 真实模板：checking／ready／partial／failed／unknown／blocked，主标题、诊断摘要、手动检查与进入门禁；无结果页重发控件，触控与语义检查。
- 原工具链类型与模板编译，定向受影响回归；不重复无线、断电、地图、任务或自动化矩阵。

所有验证使用边界fixture或原模板，不能冒称真实App安装、设备WiFi、Cloud链路或iOS通过。本批不连接云端／手机／串口，不改账号、授权、UNKNOWN、请求记录或网络；不发验证码、不升级SDK、不部署或提交推送。

## 8. 后续顺序

| 顺序 | 范围 | 门禁 |
| --- | --- | --- |
| ON-1～3 | 本批软件实施及有限回归 | 本文合同冻结后执行；结果更新于§9。 |
| ON-4 | 匹配App必要部署和最短差异真机 | §15已完成匹配安装及COM11一次正常前台入网→Cloud确认→自动进入，正常路径有限收口。保守恢复、自然异常及受限平台仍按单列待验，不重复正常配网或§39无线矩阵。 |
| ON-5 | LAN按能力可选接入 | 有LAN能力／同局域网／Grant认证及新鲜State后才可判局域网可用；mDNS只是发现hint，不强制新增固件服务。 |

自动SoftAP OEM入口、普通BLE UNKNOWN跨进程存储、原生明确失败原因进一步结构化均独立记录，不借本批扩大支持范围。正式开启这些路径前另定最小实施范围。

## 9. 实施结果

### 9.1 软件实施：ON-1～3有限通过

| 范围 | 实际结果 |
| --- | --- |
| 独立完成器 | 新`wifiprov-completion.ts`负责双事实归并、30秒绝对monotonic截止、5个串行观察时点及取消。只有合法unconfirmed推进；慢响应越过时点直接跳过，错误不自动重试，无写／存储端口。 |
| 复用原观察链 | 原只读验证抽为`WiFiProvCloudObservation`；UNKNOWN兼容类继续one-shot且`localReceipt=unknown`。原owner真实关闭、网络隔离、账号／instance／Cloud CAS、新会话及State时间合同不变。 |
| Page薄接线 | 实际WiFi调用前捕获本次公开attempt，成功／未知进入同一确认阶段，不自动导航；typed提交前拒绝不制造UNKNOWN。官方已成功后即使观察构造／Cloud／清理出错也不降级。普通BLE未知只用本次内存，不伪造durable。 |
| 生命周期与导航 | 后台、离页、换号取消，晚结果不发布，返回不自动查。结果操作也遵守全局Cloud fence；用户显式进入时防双击、confirmed重新核owner／CAS，刷新失败不撤回已确认结果。所有allocation／UNKNOWN保留，不自动complete；正常retirement不变。 |
| 结果区域 | 复用原status-card／主题token；ready主标题为设备已上线，unknown只在折叠详情保留。明确区分已连WiFi、Cloud未就绪、明确拒绝及未能确认；不再有第二套UNKNOWN结果主标题／重发控件。移除废弃`wifiCloudMessage`消费者，保留原恢复列表和精确清理入口。按钮48px／busy与live语义、文本换行及减少动态样式已接。 |

明确原因只接受原native精确的AUTH_FAILED／NETWORK_NOT_FOUND或明确false结果；DEVICE_DISCONNECTED、通用SoftAP失败／传输超时仍为unknown，未臆造密码错误。本批未改原生SDK／Java，也未扩展LAN或后台自动观察。

### 9.2 有限验证及证据

**10个选定文件、209个不同用例分批通过**：新纯完成器32、Page集成15、真实结果模板8；关联旧单次观察13、Page42、首装22、恢复17、生命周期11、Cloud／Local阶段22、引导模板27。不是把多次运行数量相加。

首次队列208例中202通过、6失败，原日志保留；针对旧成功即导航／阶段预期、弱模板fixture及非法观察应blocked的差异修正后，仅重跑4个受影响文件106例，全过（含新增第三方Local fence案例）。原身份精确报文、owner死亡、CAS、前后台、UNKNOWN及无重发断言保留，没有删除测试或mock整个完成器。最终App与选定spec的`tsc --noEmit`通过；Angular真实模板／样式定向编译与相关diff检查通过。

App忽略目录`build/onboarding-completion-20261030`保存两阶段完整日志、有限配置、最终源码SHA及去重汇总`verification-summary.json`。首轮测试工具曾提示setup文件未纳入类型程序，但仍正常参与测试打包；本批已将既有setup明确纳入有限类型配置并定向核查，不为该配置提示重跑功能矩阵。结果不冒称完整全库类型／原生发行编译。

本批只读审阅并修改本地源码／测试／方案，未访问手机、串口、云端业务或账号存储；原账号、授权、UNKNOWN、完整真实日志及无关本地修改未触及。未修改锁文件／依赖、升级SDK、构建APK、部署、发验证码、提交或推送。

### 9.3 待验而非已完成

- §9软件批当时尚未安装匹配App／真机；后续一次真实差异见§11，不能把软件通过或部分真机通过写成ON-4整体完成。
- 真实手机375px／横屏、大字、暗色对比／系统辅助技术仍需匹配App短验；本批真实模板测试验证DOM语义与动作门禁，不把JSDOM当像素布局引擎。
- LAN按ON-5后置，首发固件不因本批强制增加LAN服务。
- 普通BLE UNKNOWN跨进程持久恢复、原生SoftAP失败原因结构化、OEM自动入口仍保留原边界；不为凑样本重置设备或重放旧UNKNOWN。

## 10. ON-4a：匹配候选离线准备与最短差异清单（2026-10-30）

### 10.1 本轮范围裁定

用户继续下一步，同时明确**暂不部署**。因此本轮只做ON-4的离线准备，不安装App、不改云配置／镜像、不连接设备或账号，不跳到LAN或扩展功能。ON-1～3的209项冻结证据沿用，不重复测试队列；ON-4真机仍未通过。

先独立只读核对：§9汇总冻结的完成器、Cloud观察器、Page TS／HTML／SCSS五份源码SHA全部匹配；原SDK2.4.4 JAR、原生LocalOwner及esbuild0.28.2未变，Cloud接口仍保留no-store／禁止鉴权重放及CAS合同，LAN仍关闭。无源码缺口阻断本批候选，故没有新增生产代码或扩展身份链。

### 10.2 唯一匹配构建与产物核对

复用§39已审构建／产物检查工具，固定原脚本SHA，仅内存派生本批目录；构建前置替换为五份源码冻结核对，不读取／伪造phone-before账号快照。没有复制第二套公开构建工具，也没有借候选构建执行安装／ADB／Cloud调用。

| 阶段 | 本轮实际结果 |
| --- | --- |
| 原模板／custom-panel prebuild | 20.972秒，通过。 |
| 匹配cloud-beta Web | 29.862秒，通过。 |
| 原Capacitor同步 | 9.845秒，通过；仅标准生成产物，既有机械换行归一按原工具恢复。 |
| 原SDK离线Beta APK | 143.280秒，通过；唯一队列，未自动重试。 |
| 源码与Git保留 | 每阶段App源码、HEAD、Git index精确不变，保留无关本地修改，锁文件／依赖未改。 |

唯一标准输出：App `android/app/build/outputs/apk/beta/app-beta.apk`，**77,893,852 B**；SHA-256 **`aff1b90083cb17700cfb8f6bc7f2feabdbcb1225c009439924976ec5868298f9`**。包名`iot.diandeng.tech.beta`、标签“Blinker 内测”，实际manifest版本`1.0-beta`／code1。原签名`a76639ec…5d210`、原SDK2.4.4及native class／源码SHA均与§39一致；不把版本名称当候选唯一标识，以完整SHA为准。

APK HTTPS origin／无cleartext、关闭bridge明文日志、原生插件注册表及**1156份Web资产逐字匹配**均核对通过。实际Page chunk为`chunk-DutqjwL4.js`／SHA`ea044bf4…9bf192`；跨chunk核对10项稳定完成判定／Cloud合同／结果文案消费者，并以AST核编译后的30秒预算及0／2／5／10／20秒五个观察时点，不只凭源码函数名认定已打包。

限定产物边界扫描没有发现私有备份／build／spec／env路径、私钥块或JWT字面量；不是完整安全审计，也不读取真实账号日志来做匹配。冻结源码、候选哈希及原生输入一致性证明位于App忽略目录`build/onboarding-on4-prep-20261030`。只保留标准APK，不另打交接ZIP／复制整库；后续若该标准输出被其他构建覆盖，必须重新核SHA，不能沿用本批证明安装别的包。

### 10.3 ON-4后续最短真机差异矩阵：当批待验冻结，实际结果见§11

| 项目 | 前置及唯一操作 | 必要证据与停止条件 |
| --- | --- | --- |
| 统一成功结果 | 获准安装后，核匹配APK、同账号／COM11原instance与Admin及当前网络；只做一次明确的新WiFi操作。 | 官方confirmed与本次新鲜Cloud证据分别记录；owner退出前零Cloud读。结果页不自动导航，显式一次“进入设备”，allocation／UNKNOWN不清理。回执未知、权限或CAS冲突即停写，不自动重发。 |
| 部分完成／确认超时 | 仅利用上述操作自然出现的合法unconfirmed；或下一批另定受控只读故障范围。 | 已确认WiFi始终保留，最多30秒／5次串行读。**断网／HTTP错误应立即unavailable，不能拿来销项“30秒未就绪超时”。** 无自然条件就标待测，不再重配凑样本。 |
| 后台取消／手动恢复 | 仅在同一真实确认仍进行时后台一次再返回。 | 取消观察，晚结果不升级，原confirmed不降级；返回零自动读／写，显式一次“再次检查状态”沿用原attemptedAt。若正向操作已结束，不重发WiFi制造窗口。 |
| UNKNOWN＋Cloud上线 | 仅新版真实操作自然产生的UNKNOWN，或合法的新版持久记录。 | 主标题设备已上线，详情官方unknown；无Bootstrap／WiFi／Key／Admin写、无complete。没有样本即NOT_RUN，不从§34旧日志或无metadata allocation补造。 |
| 结果显示与收尾 | 顺带检查上述真实结果页，不另开入网循环。 | 截图、375px／横屏／大字、暗色和辅助技术；折叠详情、48px按钮、busy／禁用和无重发控件。视觉不能替代Cloud新鲜度证据。 |

### 10.4 已有证据沿用与明确不做

§39的BLE必要发现／连接、设备WiFi列表／GetInfo、7629ms唯一成功WiFi回执、23ms回收、身份／授权保留、Presence转在线及52ms精确新State回应继续沿用；不重跑144个实时样本、控制、BLE长循环或断电矩阵。原SoftAP UNKNOWN、OEM自动managed冻结及原Error5仍独立保留，不能借本批候选改写。

本轮没有新鲜读取手机安装状态或账号，不把“候选已构建”写成“新App已在手机运行”；没有设备／云端操作，也不声称当前网络名称已确认。账号、授权、UNKNOWN、完整真实日志及服务配置未触及。未安装／部署、发送验证码、升级SDK、提交或推送。

**下一步仍是ON-4获准安装后的最短差异真机，不是重复离线回归或开始LAN。** 安装前重新核对候选SHA、账号与现场网络；出现近期认证门禁就停止，不发验证码或以旧token绕过。

## 11. ON-4b：一次真实差异及导航缺陷（2026-10-30）

### 11.1 本轮范围与匹配前置

用户允许安装§10的已构建候选，沿用现有云服务、COM11固件和已存在身份。**本轮没有重建App、刷板、解绑、创建身份或重新签发Admin。** 单次安装APK SHA为`aff1b90083cb17700cfb8f6bc7f2feabdbcb1225c009439924976ec5868298f9`；同包名／原签名，安装后及终核时的完整账号session、3项secure存储和两类完整journal均与安装前逐字哈希一致，原allocation／UNKNOWN保留。

- 新鲜网络：电脑为“带翅膀的蜗牛”／192.168.2.76，手机为“带翅膀的蜗牛_Wi-Fi5”／192.168.2.162；手机实际2462 MHz，即2.4 GHz，不能按SSID尾部判断频段。COM11实际WiFi列表含该网络后才填写；密码仅在内存使用，未写方案或公开日志。
- COM11核对MAC、原BLE应用SHA`103cdde6…3a1b4`、instance、epoch1和单Admin；App现有active Admin的同账号／instance／controller／epoch／Cloud CAS与设备及两次owner-only GET匹配，响应`private,no-store`。不调用Key reveal、注册Admin或重置根。
- 原固件的显式`n`仅清网络、打开一次原10分钟BLE配网窗口；不清身份、TaskStore或授权。仅保留新鲜分区／20 KiB NVS及既有App必要资料，不另做8 MiB全片备份或更换固件。
- 首次WebView调试连接超时，尚未发生业务操作；先保留原生安全存储、IndexedDB及完整logcat，再普通重启App一次。确认新PID／调试端点后读取成功，未清数据、重新登录或发送验证码。不是网络／业务的自动重试。

### 11.2 唯一实际操作及结果

通过真实原页面与WiFiProv选择COM11，核Security1、GetInfo及已有Admin，只进行**一次新的普通WiFi下发**。观察器不填造上下文或结果，拒绝Bootstrap／typed首装写及第二次WiFi调用；本轮只验证差异，不重跑BLE发现／控制／实时样本矩阵。

| 项目 | 实际证据及结论 |
| --- | --- |
| 官方回执 | 唯一`provision`实际`success=true`，**7638 ms**；不把Cloud上线补写成官方回执。 |
| owner回收与Cloud时序 | 原`clearState`**24 ms**完成后才发本轮确认合同POST，手机同一时钟上相差约23 ms；不是用ready快照／固定等待解除隔离。两次确认均带`provisioning-cloud-v1`合同。 |
| 后台停止 | 首次确认HTTP200／约185 ms，合法`unconfirmed`；同次确认尚未结束时按真实HOME，结果明确“状态检查已停止”，busy结束。不是HTTP断网错误，也不宣称已耗尽30秒预算。 |
| 返回与显式恢复 | 返回保留官方confirmed，检查按钮恢复；**零自动完成确认请求、零WiFi／身份重发**。随后仅显式点一次“再次检查状态”。返回期间原账号连接生命周期有一次`GET /api/v2/account/connection`200及MQTT重连／Presence订阅，不错误描述为“全App零HTTP／零读取”。 |
| Cloud真实确认 | 显式读HTTP200／约180 ms，`online`；两次POST正文完全相同，仍为原attempt／instance／CAS。新`issuedAt ≥ attemptedAt`、`stateUpdatedAt ≥ issuedAt`且`observedAt ≥ stateUpdatedAt`，符合新会话／新鲜State合同，不凭列表在线或旧State判成功。 |
| 统一结果与非自动导航 | 主标题“设备已上线”，详情分别显示官方已连接／本次云上线已确认，默认折叠；仍留在结果页，只有一个“进入设备”按钮，没有重发控件。实际320 CSS px竖屏无横向溢出，按钮高48 px；折叠／展开真实截图保留。 |
| 进入设备 | **失败，ON-4不能整体关闭。** 官方confirmed及Cloud ready、后台返回后显式点一次进入，路由未变，未发生owner context GET；页面没有显示错误。根因见§11.3，不进行第二次点击或重配绕过。 |

后台操作由第一条真实Cloud确认请求触发，未拦截、延迟或伪造响应。首次合法unconfirmed后仍有剩余确认阶段，后台取消覆盖的是本次确认阶段／后续观察，不冒称已验证“正在传输的HTTP被中止”。ADB回前台的首次命令误用了beta相对Activity类名，系统拒绝且未启动任何Activity；核实际完整类名后单次bring-to-front成功，原始错误保留，不算App登录或配网失败。

本BLE短验的回收证据为原接口`clearState`真实完成及后续无隐藏续写，不代替SoftAP独立owner进程死亡或故意清理失败的既有合同／验收。原始WS流的离线只读核对也未发现Command／任务／遥测写；只有离开结果页回首页后，才发生原首页的Manifest／State新鲜读取。

### 11.3 新发现的最小修正范围：仅导航signal与可见反馈

只读源码核对和真实Page快照一致：后台`suspendPage()`已abort原`cloudNetworkAbort`；手动Cloud检查正确使用新signal，因此可恢复ready。但`navigateProvisionedDevice(signal)`仍调用无参`assertProvisioningCloudNetwork()`，错误复用了旧的已abort signal。其catch只写旧`statusMessage`后return，新完成卡却读取`completionActionMessage`，表现为点击无响应、失败原因不可见。

**本轮按验收范围保留匹配候选，未改生产源码、未另建／安装第二个APK。** 下一批最小修正合同为：

1. 导航网络检查传入本次导航signal，不重建全局controller或绕过真实Cloud fence；仍重新核owner／CAS。
2. 清理／网络／列表失败通过现有完成卡反馈，保留official confirmed／Cloud ready及原意图，不默默return、不自动重发。
3. 补“后台取消→返回→显式检查ready→显式进入”的有限回归，另保留真实新导航取消、第三方Local fence和CAS变化禁导航；不重跑209项或无线矩阵。

优先软件最小修正及有限回归。后续若安装新候选导致本次内存attempt消失，不得从日志／旧allocation注入一个成功结果或再冒充本次验收；是否需要新的最短真机差异单独明确，不为补样本循环配网。LAN继续按能力后置。

### 11.4 终核、证据及仍待覆盖

结果页退出回首页，原native扫描停止／`clearState`完成，观察器原样恢复nativePromise并删除监听、binding及9479转发；无隔离WiFiProv子进程。完整App账号、secure和请求journal终核一致；COM11终核原BLE应用／分区未变，instance与根授权精确一致、TaskStore摘要一致。**WiFi目标为本轮正常写入的新网络，不把NVS整片称为字节未变，也不把旧网络写回。** 云服务／配置均未改，其他板未操作；无发码、Key／Admin写、自动重发、SDK升级、提交或推送。

App忽略目录`build/onboarding-on4-live-20261030`保存完整前／安装后／终核账号私有记录、完整logcat、原APK、真实请求／响应和截图、只读Page快照、`acceptance-summary.json`及独立审计。设备库同批忽略目录保存新鲜NVS／分区与校验日志，私有目录有访问限制。五份生产源码仍匹配§9冻结SHA；209项旧软件证据沿用，未重跑凑数。定位及界面核对参考UI/UX规则，仅用于显式反馈、48px按钮和窄屏检查，未引入新样式或页面。

以下为**NOT_RUN而非通过**：自然30秒未就绪超时、新版真实UNKNOWN＋Cloud上线／跨进程恢复、375px／横屏／暗色／大字／辅助技术专项、LAN及SoftAP OEM入口。本轮旧UNKNOWN完整保留，BLE这次成功不改写原SoftAP回执。当前优先处理§11.3的实际导航缺陷，不扩展身份链或重开资源／任务审计。

## 12. ON-4c：后台返回导航的最小软件修正（2026-10-30）

### 12.1 实施范围及保持的边界

按用户要求，只修§11.3，不操作手机、COM11、云服务或真实账号／存储，不构建安装新APK、不重复配网，不添加LAN。既有App／固件／UNKNOWN和完整请求日志保持；手机仍是§11验收的`aff1b900…8298f9`候选，**本节软件通过不等于手机缺陷已消失或ON-4整体完成**。

生产改动仅在现有`Esp32ProvisionPage.navigateProvisionedDevice`内：

- 原网络检查传入本次显式导航的`signal`，不复用后台已取消的`cloudNetworkAbort`，也不通过重建旧controller恢复已退休流程。
- 清理失败、网络／隔离未确认通过现有`completionActionMessage`显示在结果卡；保留official confirmed／Cloud ready，不再只写不可见的legacy状态字段。列表不可用仍给现有提示；已有合法设备缓存时，列表刷新失败不撤销可进入结果。
- 路由明确返回`false`与reject均走已有可见反馈；缺失或不匹配的allocation／attempt以原context错误终止，不再静默return。CAS／账号检查、重复点击保护及原意图不清理合同不变。
- 原全局Local fence在native网络检查、上下文读取、列表读取三个等待边界重新同步核对；若新owner在等待期间进入，停止后续全局列表读取或Direct／导航并给可见提示。复用原lease和global fence，无新增RPC、轮询、固定等待或新的协调器。
- 网络、context或列表等待时离页取消后，晚结果不触发Direct／导航，也不发布迟到错误。未把“取消旧操作”改成“自动重试新操作”。

不改生产HTML／SCSS，不新增提示组件或另一套结果模型。UI/UX检查用于确认当前完成卡内可读、带`role=status`的错误反馈，不改变成功标题、按钮或诊断折叠规则。

### 12.2 有限回归与证据

最终只运行两个直接受影响文件：**Page链路30项、真实生产模板9项，共39个不同用例通过，其中16项为本批新增**。覆盖真实Page的“确认中取消→返回无自动确认→显式检查ready→显式进入”；没有mock掉网络检查或Cloud观察器来掩盖旧signal问题。另覆盖三阶段取消晚结果、清理／网络失败可见、原CAS变化、账号变化、三个等待边界的第三方Local fence、列表缓存有／无、路由false／reject及UNKNOWN不清理／不重发。

App及选定spec的`tsc --noEmit`通过；Angular真实模板／样式随上述定向测试编译通过。原依赖、`package.json`／锁文件、生产模板／样式、HEAD和Git index逐字核对不变，保留无关本地修改。不是全库／APK／原生或真机验收，也不把209项历史软件证据重复计数。

首轮类型检查的三处错误均在新增测试夹具（mock签名／private字段访问），修正后保留原失败日志；首轮功能28项Page通过，模板新增例因OnPush夹具未mark触发NG0100，补`markForCheck()`后最终两文件全过。未关闭开发模式检查、放宽断言或修改生产模板来让测试通过；中间失败与最终结果分开保留，不自动重开失败队列。

App忽略目录`build/onboarding-on4-nav-20261030`保存冻结输入哈希、三轮类型／测试日志及最终`verified.json`。最终Page源码SHA为`ee2e331dc02f812059af2a794adbf388fbac4dcdc84130a946fc82d85e332aa7`。独立只读审阅未发现本批新增阻断；本次不提交、推送或升级SDK。

### 12.3 正常成功后是否自动进入：当批建议，实施更新见§13

**产品建议：正常前台的一次新配网，在真实可用确认和进入权限均通过后，自动进入设备通常更顺手，不需要人为停留数秒再跳。** 配网完成判定与跳转策略是两层：可以复用同一进入设备薄链，不把WiFi回执当成Cloud可用，也不通过跳转完成／删除allocation或UNKNOWN。

查阅[小米官方米家接入说明](https://ics-resources.xiaomi.com/ics-resources/articles/60bf175420a523e5ed34b3f3.html)，WiFi路径在通信成功后还有选择房间步骤，其他连接类型也按实际引导推进；该资料不能证明所有设备／版本都无条件直跳控制页。用户观察到的米家自动推进可以作为体验参考，而非本项目跳过可用性／权限核对的依据。

| 场景 | §12当批建议（实施更新见§13） |
| --- | --- |
| 当前前台、用户本次发起的正常配网 | official confirmed＋Cloud本次新鲜ready、原owner真实退出、同账号／instance／CAS且无新Local fence时，复用原进入链自动进入一次，保留轻量成功提示及日志。不另造定时跳转器或配置重发。 |
| 后台／离页后返回、持久记录恢复 | 不抢焦点自动跳页；沿用显式只读检查与进入，不恢复旧操作的自动continuation。 |
| official UNKNOWN但Cloud已证实可用 | 设备仍应显示ready，不能说未上线；建议保留显式进入与回执详情，原UNKNOWN继续保留。这是当前保守的恢复交互建议，不把UNKNOWN等同于控制权限不足。 |
| Cloud未确认／错误、明确WiFi失败或身份／隔离门禁未过 | 保留相应结果及合法手动操作，不自动进入、不误报成功／失败、不重发配置。 |

§12当批只修已验缺陷，自动进入当时尚未实现；后续用户已明确采用，实际软件接线及有限结果见§13。保留这一阶段区别，不把建议、源码实现和真机验收混为完成。

### 12.4 下一步最小范围

§11.3的软件缺陷已修，手机仍保留旧候选；等待明确允许后才构建／安装一个匹配新候选，并安排可获得真实结果的最短导航差异。不得导入旧日志／allocation伪造本次成功、恢复退休continuation或为了补样本重复配网；无合适自然结果则保留真机待验。LAN、自然超时／新UNKNOWN样本及其他屏幕专项仍保持原独立待测范围，不借本次改名为已完成。

## 13. ON-4d：正常前台完全确认后一次自动进入（2026-10-30）

### 13.1 已实施的最小行为合同

用户明确允许按§12.3接入自动进入。本批仅修改现有Page的`provision()`尾部及相关测试，不部署、构建APK或操作真机／账号／服务；不增加LAN、SDK、配置重发或可恢复自动队列。

| 结果／来源 | 进入行为 |
| --- | --- |
| 本次真实新配网、持续前台、官方confirmed＋本次Cloud新鲜ready | 清除原配网busy后仅有一次自动进入机会；仍通过原导航的Cloud网络、owner／CAS、账号及隔离检查。 |
| 初次确认超时／错误后，用户显式检查得到ready | 保留“进入设备”，不自动跳转。 |
| 途中后台／离页／取消后返回 | 原generation永久退休，返回不恢复自动continuation；显式只读及进入仍可用。晚回执／晚Cloud／晚导航结果不自动跳。 |
| 官方UNKNOWN但Cloud已证实可用 | 仍显示ready并保留显式进入，原UNKNOWN不改写、不complete／删除。 |
| 自动进入被门禁拦住或导航失败 | 留在原结果，沿用可见反馈与合法手动入口；不自动重新查询、重试导航或重发WiFi。 |

### 13.2 接线方式及一次性保证

复用`openProvisionedDevice()`和§12原导航链，不增加另一套导航／Cloud检查，也不在通用`confirmCompletion()`、页面初始化或前台返回时触发。`provision()`局部捕获本次真正WiFi调用的公开attempt对象；仅`acceptedReceipt=confirmed`、同一attempt引用、原generation仍current且结果ready时，尾部调用原入口。

该尾部在原`finally`清除`provisioningGeneration`之后执行，避免被自身busy挡住；**位于WiFi的try/catch之外**，导航false／reject不会撤回已确认回执或制造UNKNOWN。一次方法调用的尾部天然只经过一次，不设可重新arm的token、timer、后台任务或持久标志；手动检查不经过该尾部，因此失败／返回不自动重试。原`completionOpening`在第一个await前同步互斥自动／人工点击；人工点击先赢时不会额外发context请求或导航。

生产HTML／SCSS未改，成功及失败沿用现有结果卡／`role=status`提示。按UI/UX规则保留失败后的显式可恢复入口，不使用固定等待、强制焦点变化或新loading组件；没有新增成功toast或宣称新的视觉／辅助技术专项已验。

### 13.3 有限回归及保持检查

**两个相关文件54个不同用例一次通过：Page45、真实生产模板9，其中新增15项自动进入差异。** 原相关导航用例按新策略使用真实“初次Cloud读失败→显式检查ready”的共享夹具准备手动入口，不注入私有skip-auto标志或mock掉自动逻辑。正常路径用例现验证慢清理／Cloud确认前零导航、完全确认后一次自动进入。

新增覆盖：typed端口成功不回退generic WiFi、false／reject不自动重试且确认保留、自动／人工竞争、ready收尾取消或attempt替换、真实可见性后台后晚native成功、自动导航network／context／list三阶段取消、原CAS／账号／owner门禁及自动网络检查失败后的显式进入。旧UNKNOWN、超时、显式恢复、取消／门禁及模板错误提示断言继续通过，未删除弱化；typed用例仅证明Page薄接线，不能代替原生／SoftAP／真机验收。

App及有限spec的`tsc --noEmit`通过，Angular真实模板／样式定向编译通过。复用§12有限验证脚本，固定其SHA后只内存派生本批路径；唯一顺序队列，无重复队列或失败重试。没有重跑209项、原生／SDK编译或无线矩阵，不把旧39项与本次54项相加计通过数。

App忽略目录`build/onboarding-auto-entry-20261030`保留输入哈希、完整类型／测试日志和`verified.json`；最终Page源码SHA为`23e275612a138d5af3f59fa392b47f832daf9630cc2797c6deca925b46efc2d1`。`package.json`、锁文件、生产模板／样式、原模板测试、HEAD及Git index核对未变，保留无关本地修改。独立只读审阅未发现本批新增阻断。

### 13.4 当前停点与后续最小差异

自动进入已在源码实现，**尚未构建／安装匹配APK或验修后真机，ON-4整体仍开放**。本轮未读取手机当前安装状态，最后一次实际验收是§11的`aff1b900…8298f9`；不能称手机已运行§12／13修正。账号、授权、UNKNOWN、完整既有请求日志及服务配置未触及；未发送验证码、自动重发配置、升级SDK、提交或推送。

后续获准后仅准备一个包含§12修复与本节策略的匹配候选；真实新结果可用时补正常自动进入／保守恢复入口的最短差异。没有合适结果就保留待测，不导入旧结果、恢复退休continuation或为补样本重复配网。LAN、自然确认超时／新版UNKNOWN样本及原视觉专项继续独立后置。

## 14. ON-4e：匹配候选完成，限定真机等待手机解锁（2026-10-30）

### 14.1 本轮实际范围与授权

用户继续收口，并已明确允许安装一个包含§12／13修正的匹配内测App及仅COM11一次必要的新配网。原暂不部署限制在此限定范围内解除，**不需要再询问安装／本次配网许可**；不刷板、解绑、清根或重放UNKNOWN，不发送验证码，不升级SDK，不修改云端服务／配置，不提交推送，LAN后置。

本轮先完成候选，获准后只读核对现场；手机ADB在线但仍锁屏，已请用户解锁并进入“Blinker 内测”。截至本节，**未安装、未打开新的10分钟网络窗口、未扫描／连接／发送配置，也未完成修后真机验收**。不能把许可、已构建或新鲜硬件读取写成入网成功。

### 14.2 唯一构建及实际产物核验

沿用§13冻结的54项回归和类型结果，全部当前源码／受保护依赖与证明SHA一致，不再运行已过测试。复用§10／§12原构建及产物工具，固定原脚本SHA，只派生本批路径及新源码冻结；不读取／伪造手机快照当离线构建前置，不另建构建队列。

| 阶段 | 本批唯一实际结果 |
| --- | --- |
| 原prebuild | 21.961秒，exit0。 |
| cloud-beta Web | 30.237秒，exit0。 |
| 原Capacitor同步 | 10.350秒，exit0。 |
| 原SDK离线beta APK | 135.889秒，exit0。 |

四阶段源码、HEAD、Git index均精确保持；原同步机械换行按既有工具恢复，未改依赖或锁文件。标准唯一输出仍为App `android/app/build/outputs/apk/beta/app-beta.apk`，**77,893,852 B**，SHA-256 **`95c7ada28da67f1ac34320916a75ac06d67800f33a6c959531afe6b5c87a73bc`**。包名`iot.diandeng.tech.beta`、原签名`a76639ec…5d210`；Page为`chunk-C29qMJL22.js`／SHA`b357e92f…109dab`。版本名称不作为候选唯一标识。

实际APK的1156份公开资产与Web／原生资产逐字核对通过；HTTPS、关闭cleartext和bridge明文日志、WiFiProv原生注册表均保持。SDK2.4.4 JAR及native source／class SHA与已验候选一致。限定资产扫描未见私有备份路径／私钥块／JWT字面量；不扩大为完整安全审计。

对实际压缩JS作AST核验：唯一自动进入位于清busy的finally之后且在WiFi catch外，confirmed／ready、同attempt引用及current generation条件在；导航转发本次signal，三个现有隔离门禁仍在，显式检查／通用确认／挂起不触发自动进入。原Cloud合同、结果文案及30秒／五时点常量也已打包。

静态检查器最初未兼容相邻minified节点及无插值模板字面量，分别修正为非重叠边界和`isStringLiteralLike`后通过；原检查错误及校正说明保留。没有为检查器问题修改生产源码、放宽业务条件、重建APK或重跑功能矩阵。独立只读AST审阅确认上述guard实际存在。

### 14.3 已完成的现场前置及保留

- 电脑“带翅膀的蜗牛”实际5GHz；手机“带翅膀的蜗牛_Wi-Fi5”实际2462 MHz／2.4GHz、192.168.2.162。SSID名称不用于猜频段；真正下发前仍须由COM11实际扫描确认该目标可见，密码只在内存读取。
- 仅COM11被核对：原MAC `8c:bf:ea:1b:96:14`、原BLE应用SHA`103cdde6…3a1b4`、instance及epoch1／单Admin匹配。新鲜分区、20 KiB NVS读回及校验日志已私有保留；没有8 MiB全片备份／刷写，也未打开新配网窗口。
- 在安装前只读保留手机完整原生secure SharedPreferences、IndexedDB／请求数据库及完整logcat，私有目录有访问控制。secure XML为3108 B，SHA`40f0961e…cc0c0`，与上轮已验存储匹配；此为原生存储保留，**不是本轮已通过App读取当前账号和Cloud Admin的新鲜验证**。
- 账号前台快照／owner-only详情及Admin读、匹配安装、唯一新配网和结果／导航仍待解锁；未用旧token／旧记录替代新鲜前置，也未因锁屏重启App或解锁安全设置。

App忽略目录`build/onboarding-on4-close-20261030`保存完整唯一构建日志、`artifact-verified.json`、`completion-verified.json`、`flow-verified.json`及访问受限的原生账号／journal保留；设备库`build/onboarding-on4-close-live-20261030`保存硬件身份／分区／NVS证据。旧候选证明、原账号／授权、UNKNOWN及完整日志均未覆盖。

### 14.4 解锁后的唯一接续

无需重建、重新申请安装许可或重读已经核验的固件。按顺序完成：

1. 用户解锁并进入现有内测App后，保留前台账号／完整journal，核同账号、COM11 instance、现有Admin及Cloud CAS；只读失败或近期登录门禁即停，不发码／绕过。
2. 仅安装上述SHA候选一次，核账号／授权及journal保持；不修改服务。
3. 原页面／观察器就绪后，才打开COM11原10分钟网络窗口。仅一次新的WiFi操作，记录官方回执→真实owner回收→本次Cloud新会话／State→自动导航一次；不手动注入成功、不点击进入设备制造自动导航证据。
4. 原页面自然离页回收，保留本批完整日志并终核授权／UNKNOWN，记录通过与未覆盖。无法自然获得的超时／新版UNKNOWN不补造、不为样本重配；不借一次正常自动进入宣称所有SoftAP／OEM／LAN／后台异常矩阵通过。

当前只是等待手机解锁的外部前置；候选、硬件前置和唯一操作边界已固定，不扩展新的源码／审计批次。

## 15. ON-4f：匹配安装及正常自动进入真机有限收口（2026-10-31）

### 15.1 解锁后的唯一接续与实际安装

用户已解锁并自行取消锁屏密码；本轮没有更改任何系统安全／锁屏设置。ADB及COM11仍匹配，电脑网络和手机实际2.4GHz网络与§14一致。通过当前App读取同一账号、完整secure和journal，owner-only详情及Admin GET均200／`private,no-store`，同instance／controller／epoch1／Cloud CAS匹配，未使用旧token替代新鲜前置，没有reveal、Key刷新、Admin写、解绑或新身份创建。

仅安装§14的**`95c7ada28da67f1ac34320916a75ac06d67800f33a6c959531afe6b5c87a73bc`**候选一次。安装前、安装后、终核的账号、完整session、3项secure、完整任务／自动化journal及原生secure XML均一致。保留旧allocation／UNKNOWN，不complete／删除；没有重新构建、重复安装、刷固件、清NVS或重跑54／209项软件测试。

安装运维薄包装最初将proof文件URL当普通路径，尚未进入ADB安装即ENOENT；校正时又由意图文件重名守卫提前停止。两次前置停止均保留记录，随后分开包装意图和`device-install`意图，只发生一次实际ADB安装。本批修的是忽略目录内工具路径／守卫，不改业务源码或候选，不是自动业务重试。

### 15.2 一次真实正常前台入网结果

原页面及有界观察器就绪后，仅COM11打开一次原10分钟网络窗口；仅清网络、不清根，未重新读取／刷写已核验应用。设备实际扫描返回“带翅膀的蜗牛_Wi-Fi5”后才填写，以现有BLE Security1／GetInfo／Admin身份链执行**唯一新WiFi操作**。观察器拒绝Bootstrap、首装typed写、任意custom及第二次WiFi；本批不手动进入／检查、不注入后台或修改成功结果。

| 段落 | 本次真实结果 |
| --- | --- |
| GetInfo／身份 | 唯一2B GetInfo，原instance、flags7、epoch1匹配；无身份安装或Key／Admin写。 |
| 官方WiFi回执 | 唯一`provision`实际`success=true`，**7647 ms**。这是官方回执段，不是完整入网总耗时。 |
| owner回收→Cloud | 唯一`clearState`**25 ms**完成后约**16 ms**才发首次Cloud确认（同手机时钟）。不以ready快照或等待解除隔离。 |
| 有界Cloud确认 | 四次均200、正文／instance／CAS一致；相对时点约**0／1978／4977／9971 ms**，依次三次合法`unconfirmed`及一次`online`。属于原计划观察，不是错误重试；不重发WiFi。 |
| 新鲜证据 | 当前新`issuedAt ≥ attemptedAt`，`stateUpdatedAt ≥ issuedAt`且`observedAt ≥ State`；新会话在本次WiFi调用后约13.8秒，State随后约1.9秒。Cloud可用与官方回执独立成立。 |
| 自动进入 | `online`响应结束后约**23 ms**发原owner context GET，实际200；随后**目标原设备路由事件恰好一次**。操作日志没有open／check动作，非人工点击或注入成功；自动进入不是额外固定倒计时。 |
| 页面与离页 | 确认中保持禁用检查按钮／busy，之后自动进入原设备页，截图显示“云端已连接”。未发送控制命令，随后回首页并回收观察器。 |

原页面自然产生的Manifest／State及实时租约管理属于进入设备后的既有读取。独立离线WS解析无Command、任务写或Error，全部MQTT分片尾完整；37条TelemetryData只作旁证，不是额外实时性能循环。离页最终Close已发送，但捕获结束前未取得该笔Closed ACK，**不宣称最终租约关闭或后台回收全矩阵已确认**，也不补发Close／重开页面凑回执。

### 15.3 终核及证据

- App最后停在首页；原nativePromise、监听及binding恢复／释放，9479调试转发为空，无隔离WiFiProv子进程。
- COM11终核原BLE应用、分区不变，instance／根授权精确一致、TaskStore摘要一致；正常WiFi网络写入保留，不把NVS整片说成字节未变，不恢复旧网络或未知固件。
- 账号／授权／UNKNOWN和完整journal逐字核对保持；服务和配置未改，其他板未动。未发送验证码、升级SDK、提交或推送。
- 唯一候选和原固件继续保留。App `build/onboarding-on4-close-20261030`保存`live-accepted.json`、`independent-live-audit.json`、私有目录内完整请求／响应、logcat、账号前后记录及两张截图；设备库同批live目录保存根／任务／分区／应用终核。独立只读审阅确认本批事实，无需追加业务请求。

UI/UX技能仅用于检查真实确认阶段的busy／禁用和原设备页状态反馈，未改布局或扩大视觉验收；截图不是Cloud新鲜度或唯一导航的替代证据。

### 15.4 收口决定与剩余边界：不再默认重复配网

**本批“已有根BLE、正常前台新配网→官方确认→本次Cloud确认→自动进入一次”已真机有限收口，可冻结此实现。** §12导航修复与§13策略已在实际匹配App运行；不是只做静态检查，也不把仅有软件通过写成全部组合已验。当前正常路径没有本次发现的阻断，不再以补样本为由自动开启同路径配网／源码审计队列。

以下独立保留，**不是本次通过项，也不据此误报设备联网失败**：

- 新版App的“真实后台恢复→显式检查→显式进入”正向、自然30秒未就绪、新版UNKNOWN自然记录及跨进程恢复：本轮未制造样本；保留§11已验事实和§12／13有限回归，不冒称本次正常自动进入覆盖它们。
- 手动SoftAP首次接入、已有根手动写、OEM自动managed入口：继续原各自支持范围／受限状态；本次BLE不替它们销项，旧SoftAP UNKNOWN不改写。
- 最终Telemetry Closed ACK、其他视觉／辅助技术专项、LAN按能力接入：保持原独立范围，不能借此要求重复正常配网或新增协议。

后续仅针对具体未覆盖的受限路径或新真实缺陷安排最小批次；若无自然样本，不重放旧请求、不注入旧结果、不将“所有模式全完备”作为无限期审计目标。保留正常实现及匹配候选，暂不提交推送。

## 16. ON-4g：已有根手动 SoftAP 的 WiFi-only 窄接入（2026-10-31）

### 16.1 最小范围及授权合同

当前 COM11 已有完整根和匹配 active Admin。旧 `system-connected` owner 永久只读；`blinker-first-install` 仅支持空根，不能为已有根复用 Bootstrap。该手机的 OEM managed 自动入口继续冻结。本批只补原 WiFiProv 内的**保留身份、仅修改网络**通道，不清根、不重新分配身份、不获取 Key，不用重复首装作为重配替代。

固定支持边界：

- 新独立 create-only 用途 `blinker-wifi-reconfigure`、独立 capability `softApSystemConnectedWifiReconfigure` 和固定 `provisionBlinkerWifiOnly`。已有只读 owner 不可升级；Web／iOS 不支持即明确拒绝，不退回 managed 或 generic 写入。
- 设备须处于用户显式开启的物理配网窗口；使用设备原约定的非空 PoP，原生逐次核**实际 Security1**，拒绝安全降级和 `no_pop`。这是物理窗口＋PoP 的**网络配置授权**，不是 Cloud Admin 的设备端密码学证明。普通无 PoP 固件继续不支持，不引入新挑战协议或升级 SDK。
- 同 owner 首次固定 GetInfo 必须为完整根（flags 7、非零 instance、epoch>0），锁定 instance／epoch。只保留原只读扫描及固定 GetInfo；禁止 Bootstrap、Install、reset、任意 custom 和 generic provision。新窄方法参数必须精确匹配，SDK 前消费一次 WiFi 机会。
- App 仅从原设备重新配网入口开始，读取新鲜 owner-only Cloud 上下文，复用 `inspectExistingNetwork` 核 active 全权限 Admin、无 pending／rotation、原 logicalId／CAS、controllerId、instance／epoch。没有 Key reveal、Admin 注册或身份写。
- 原 Local owner 实际死亡后显式进入 Cloud；回 Local 必须创建不同 owner 并重新读同 instance／epoch／完整根。前后台、账号、CAS、网络绑定和原整次截止继续生效，不以固定等待或 ready 解除隔离。

**与修复方案 §28 的关系：** §28 对身份安装／轮换／恢复旧未决事务的禁重放合同不变。本批单独授权新的、无身份副作用的 WiFi-only 操作：完整根和当前 Cloud／Admin 均匹配、没有 pending Admin／rotation时，可以保留历史 allocation／UNKNOWN 而不加载、恢复、清除或完成它们；不能把新联网事实补写成旧 RPC 成功。该分界与 §39 已有 BLE network-only 一致，不是给旧 UNKNOWN 重试许可证。

### 16.2 实施及有限验证范围

复用原 SDK、独立 worker、父进程 owner 关联、Cloud／Local 阶段及统一结果页。成功终态沿原 Cloud 确认／一次前台导航；提交后的不确定结果仅保留并显式只读协调，不重发。新 WiFi-only 不借旧 allocation 保存新 checkpoint：新的 UNKNOWN 本轮仅内存保留，旧持久记录精确保留；不虚构新版持久恢复验收。

软件验证限于：新用途／安全与精确报文、实际扫描互斥、单次写入、同 owner 关联、迟到回调／官方成功后 AP 消失、Cloud 死亡屏障与新 Local 根比较、CAS／Admin 失配、后台禁续写和旧 UNKNOWN 不触及；加原首装受影响回归及原 SDK 定向 Java 编译。结果另记，不重复 BLE 已通过矩阵。

本批匹配 App 安装和 COM11 临时切换 SoftAP 固件／一次保根短验已单独向用户确认，未收到确认前仅软件实现与验证，不操作设备或安装 App。若后续授权，保留根／Key／任务区／UNKNOWN及账号日志，仅验证一次新的 WiFi-only，不重放 §34 请求；结果如实区分官方回执和 Cloud 实际可用性。当前不宣称 SoftAP 全链路已收口。

### 16.3 软件结果与下一步

**本批窄路径软件接线完成，尚未构建／安装新 APK 或进行无线真机验收。** 原插件新增 purpose／capability／固定 API；新 App 端口只读完整根、锁定 owner／instance／epoch后单次提交，WiFi 参数和 owner 关联的小型共用校验由原首装端口共同复用，原首装合同未放宽。页面新增明确“保留身份重新配网”入口和配网码标签，复用原扫描、结果页、Cloud 确认及正常前台一次进入，不新增身份执行链。

独立审阅发现并修正一个实际前置阻点：用户切到无互联网的设备 AP 后，原 `startOver→preparePage` 会再次读取 Cloud owner context。现在仅在显式新 WiFi-only 入口复用**先前同账号／authority／CAS**的上下文用于初始 Local 读取；真实后台返回不自动读／重连，但允许用户显式切换 SoftAP 并创建新 owner。实际写前仍要求旧 owner 死亡、显式互联网阶段的新鲜 resolve／Admin／CAS，以及新 Local 同根核对。无旧上下文、换号或关闭失败不适用该只读准备路径。

有限证据：

- 新薄端口16项、受共用校验影响的原首装端口15项通过；插件平台合同12项通过（iOS仅静态源码合同，不声称 Swift 构建／真机）。
- 真实 Page 15项通过，包含扫描互斥、两 owner／Cloud 死亡屏障、无互联网 AP 的后台返回、账号／CAS／根变化、Admin 缺失、单次 UNKNOWN 和旧记录完全不触及；真实模板11项通过，其中新增2项核入口隔离、标签／说明及缺能力不展示。受影响原首装 Page22项、原完成页45项通过；共93个不同页面／模板用例，分批运行不重复累计。
- Native共25个不同JUnit方法通过：新 owner10、Protocol4、Bridge6、受影响首装4及只读1。原SDK **2.4.4**的单一离线 `:capacitor-wifiprov:compileDebugJavaWithJavac` 成功（1m57s），三个Java消费者与仓库源码核同，未生成 APK。
- App、回归及插件 TypeScript noEmit 通过；必要diff检查通过。初次夹具类型、缺失 Direct handoff 替身和模板变更标记导致的失败日志保留，定向修正后只补失败／新增集合，不改产品截止或放宽断言。最终Page源码摘要 `c4bf4779…dff1be3c`；App忽略目录 `build/onboarding-softap-reconfigure-20261031` 保存页面日志、临时类型配置与 `verification.json`，native host输入在插件 `.build` 对应run中。

UI/UX技能用于明确操作用途、配网码标签／错误关联和48px按钮约束；仅完成实际模板软件检查，手机横竖屏／字体／视觉专项不冒称已验。本批无服务部署、App安装、刷板、网络变更、账号／授权／UNKNOWN操作、验证码、自动重试器、SDK升级或提交推送；无关本地修改保留。

**唯一接续：** 若用户确认本批硬件范围，使用匹配新 APK 与既有非空 PoP SoftAP 候选，仅COM11保根执行一次新的 WiFi-only短验：原实例／Admin核对→原扫描选择WiFi→真实Cloud／Local交接→一次WiFi→官方回执／新鲜Cloud结果页及成功收尾；结束核根／任务摘要、账号／UNKNOWN／完整日志和资源释放。先前已通过的BLE、只读扫描、首次Bootstrap矩阵不再重跑；无确认则软件到此冻结，SoftAP整体保持待真机，不循环清根或重新创建身份。

## 17. ON-4h：匹配构建及 COM11 保根 SoftAP 短验（2026-10-31，有限收口）

用户已明确本批测试必需的构建及限定操作直接推进，不再重复询问。本批范围沿§16：匹配内测 App 一次构建／安装，仅COM11应用区临时切换既有非空 PoP SoftAP 候选，保持原根、Key、任务区及旧 UNKNOWN，不清根、解绑或新建设备，不发验证码；其他板与服务不动。完成后只恢复原应用区，保留正常网络配置，不回滚整个 NVS。

前置只读核对：现场仅COM11 XIAO ESP32S3，MAC／instance／epoch1／单Admin与原记录一致，TaskStore Empty；原BLE应用 `103cdde6…b63a1b4`核同。新鲜保留4KiB分区表、20KiB NVS及根／任务摘要，未做无必要8MiB全备份。手机真实2.4GHz（2462MHz）仍在“带翅膀的蜗牛_Wi-Fi5”，电脑在同路由“带翅膀的蜗牛”。App旧APK `95c7ada2…7a73bc`；账号完整session、3项secure、完整task／automation journal已私有保留。原owner detail／Admin均200且no-store，本地active全权限Admin与同账号、controller、instance、epoch、Cloud CAS匹配，无Key reveal或数据写入。

唯一顺序构建、匹配安装及本批真机短验均已完成，既有SDK／依赖／锁文件未升级。COM11原10分钟窗口仅在App就绪后开启一次，原§34 UNKNOWN未改写。以下为本批真实差异结果，不重复此前BLE／空根首装或只读矩阵。

### 17.1 匹配产物与一次真实流程

内测APK SHA256为 `8ec2d4025ba5b04db926ef3f329aff4d9d90a6b015b725f526a090ec83c6d62c`；实际包名 `iot.diandeng.tech.beta`、原签名、HTTPS、1156份Web资产及新Java／JS接口核同，SDK仍2.4.4。唯一prebuild→Web→sync→离线APK队列通过，生产源码／HEAD／index保持；本批只执行一次实际ADB覆盖安装，不清App数据。安装工具的意图文件重名在进入ADB前拒绝，保留该失败记录、修正操作者脚本后才开始这一次实际安装，不作为业务重试。

| 环节 | 本批真实证据 |
| --- | --- |
| 原入口与扫描 | 原设备重新配网页→显式保留身份SoftAP用途；一个UI连接动作，原SDK首次连接238ms。实际扫描3859ms返回15行，点击真实扫描中的目标SSID并输入密码，未伪造网络列表。 |
| Cloud／Local | 首个完整根GetInfo94ms，flags7／epoch1／原instance一致。旧owner的clearState74ms确认后才显式切互联网；fresh resolve327ms、Admin108ms均200。随后重连精确AP，创建不同owner，SDK连接197ms、固定GetInfo425ms，再核同instance／epoch／完整根；没有以SSID、等待或ready替代死亡回执。 |
| 唯一WiFi下发 | 仅一次UI发送和一次 `provisionBlinkerWifiOnly`，锁定第二owner及原根，**8199ms返回官方success=true**，继而clearState103ms完成。Bootstrap／Install／generic provision／reset及身份HTTP写均未发生，旧UNKNOWN不重放。 |
| Cloud确认与恢复 | 手机切网期间首个本次Cloud读取真实 `ERR_INTERNET_DISCONNECTED`，页面保留“已连接WiFi、云端暂不可用”，不误报配网失败、不再发配置。手机自然回互联网后，仅一次显式“再次检查状态”，实际200／private,no-store，确认原instance／CAS的新session和新State（均在本次WiFi之后）。原普通阶段resolve未观测到Cache-Control，不冒称全部HTTP均no-store。 |
| 结果与进入 | 结果页显示“设备已上线”；用户式显式“进入设备”一次后，目标原设备路由恰一次，设备页显示“云端已连接”。**本批是显式恢复／进入，不是自动进入验收**；正常前台自动进入仍取§15原有限证据。没有新增控制／任务操作来补样本。 |

两次系统接设备AP均为2412MHz；Cloud回网时手机使用同路由5220MHz，SSID命名不作为频段或退出证明。Local重入前一次系统网络观察处于切换瞬间，不作为已连接结论；实际SDK后续在自己的精确SSID／绑定／Security1／PoP检查下完成连接。此次未为该观察重复连接或放宽安全门禁。

### 17.2 保留、收尾与收口边界

- COM11仅应用区临时切为既有SoftAP候选 `c05195d7…b96eff`；结束恢复原BLE应用 `103cdde6…b63a1b4`并读回校验。原instance记录、根／Key整blob、epoch1／Admin1、空TaskStore及分区表全部精确核同；合法WiFi配置保留，不宣称正常配网后的整片NVS字节不变，也不回滚NVS。
- App账号完整session、3项secure（含原授权／allocation／旧UNKNOWN）及task／automation完整journal在before／installed／final逐字摘要一致。匹配新版App保留，最终停首页。原生worker不存在，原nativePromise已还原、观察器／binding／9479转发已回收；CDP移除binding后残留的同名JS工具stub亦显式清理，未修改产品生命周期。
- 原始请求元数据、目标Cloud响应、完整journal／账号快照与logcat保留在ACL私有目录；不公开Token／PoP／网络密钥。事件原文件273100B、SHA256 `d35de3e1…ff7187`，独立离线审阅与root最终账号／设备／资源核查一致。App批次下的 `live-accepted.json`、`independent-live-audit.json`、`cleanup-verified.json` 和candidate证明为公开脱敏摘要；设备证据位于 `build/onboarding-softap-wifi-only-live-20261031`。
- UI/UX检查限本机320px竖屏真实布局、配网入口／等待／可恢复结果与两张成功截图：无横向溢出，相关按钮／详情48px；未扩大到横屏、暗色、大字体或辅助技术矩阵。未改服务、其他设备、账号额度或权限；无验证码、SDK升级、自动重发或Git提交推送。

**本批“已有根、非空PoP、Android手动SoftAP：扫描选网→Cloud／Local保根交接→官方确认→一次显式Cloud恢复→进入原设备”已真机有限收口。** 新软件与匹配App可冻结，不再默认重跑本路径。原OEM managed自动入口、空根首装的旧UNKNOWN、新版自然持久UNKNOWN正向及其他后台／权限组合保留各自证据边界；本次不把它们整体销项，也不据此自动开始新配网循环。LAN继续后置。

## 18. ON-4i：OEM 自动 SoftAP 入口的最小差异修正（2026-10-31，连接／扫描有限收口）

### 18.1 已有证据与本批范围

按修复方案§20～22的原日志重新核对：固定WiFiProv 2.4.4有参入口确实被调用，60秒内没有SDK专用onAvailable／proto-ver，旧请求关闭后worker正常死亡。已接受同AP“继续使用”不等于应用专用请求完成；普通AP agent／IP不能补造SDK成功。现有证据仍只把失败限定在Android指定网络获取，不能归因为设备HTTP、PoP、NSC或身份链。

独立核[固定SDK源码](https://raw.githubusercontent.com/espressif/esp-idf-provisioning-android/lib-2.4.4/provisioning/src/main/java/com/espressif/provisioning/ESPDevice.java)和[Android官方请求合同](https://developer.android.com/develop/connectivity/wifi/wifi-bootstrap)：继续使用原SDK申请精确网络、真实onAvailable后再绑定／握手。无证据要求移回主进程、追加Activity Context或扩大预算；不实施猜测性band改写。普通保存的AP和该App的请求授权是不同记录，不据SSID推断授权。

本次发现两个应分别处理的条件：

1. **已证产品匹配缺口：** 当前Page仅手动写用途透传PoP，普通managed create／connect／重入忽略输入；当前COM11固件使用非空原厂PoP，即使系统连接恢复，后续安全会话／扫描也可能失败。先补可选PoP的薄参数快照，不把它写成此前onAvailable缺失的根因。
2. **已证操作混杂、未证因果：** 旧AR4f工具会点击已经“正在连接”的同名行并立即am start Main。这可能打断OEM picker的自然提交，但旧buffer另有更早callback缺失，不能提前定为唯一原因。本批工具不再点Connecting／Connected行、不强拉Main、不用WiFi CLI预连；只按原系统精确选择／确认自然完成。现场另发现本测试AP的普通保存项，仅允许在原请求前移除这一个测试AP项以消除autojoin混杂，保留路由器和全部设备账号授权；该前提改变也不等于根因确认。

### 18.2 最小实施与有限验证合同

- App保留一个PoP输入，普通managed按设备要求选填，手动首次／保根写仍必填；非空值不裁空格、有界UTF8 64B／禁NUL。create、connect及原阶段重入使用同一捕获值，空值省略以保留无PoP固件兼容；不持久化／记录PoP、不增加managed purpose或写许可，复用原清理生命周期。
- 原WiFiProv有参调用、被动失网观察、独立worker、60秒native／90秒acquisition、true stop关闭、UNKNOWN与death-only Cloud隔离均不改；不自动fallback、不增加连接重试或配置重发。
- 先以有限回归复现PoP缺失并修复、核共享字段与required边界，再唯一顺序构建匹配beta。真机仅COM11保根临时应用区切换，**一次managed连接→系统自然授权→SDK真实结果／同owner新鲜读取→实际WiFi扫描→离页回收**；不点击发送，不进入身份安装或WiFi下发，不重复§17手动闭环。
- 保留账户／授权／UNKNOWN／完整journal和原始私有日志，系统picker工具须先纯测试并对原60秒请求设负向截止；控件点击／日志pending／普通网络观察均不算成功证据。若仍失败，只按本次首个分歧给出明确待定位点，不加预算或无边界重连。
- 必要构建及限定测试按用户本批授权直接执行，结束恢复原COM11应用及核根／任务，保留匹配App；不升级SDK、不部署服务、不提交推送。实施前不以方案或软件回归销项，以下§18.3～18.5记实际结果。

### 18.3 软件修正与唯一匹配构建

普通managed入口的PoP遗漏先由两项真实Page失败测试复现，再完成最小修正：提取单一`readWiFiProvPoP`校验，原首装／保根必填包装保留各自错误码；普通managed可省略，但显式输入须原样捕获并贯穿create／connect／阶段重入。原手动链与managed共用一个输入，终态／取消／离页沿原生命周期清除；旧异步finally不得清掉新generation的数据。未改原生SDK调用、owner／Cloud隔离、权限和截止时间，未新增重试或managed写用途。

19个不同有限用例通过：Page9、共享校验6、真实模板3、受影响UNKNOWN终态1；跳过项不计通过。覆盖PoP缺失、异步前快照／重入、空值省略、UTF8／NUL边界、原手动必填和清理。App／spec类型检查及必要diff检查通过。最初夹具编译错误和两项产品失败日志均保留，不把夹具失败说成产品根因，也不重复运行已通过的全矩阵。

唯一prebuild→Web→sync→离线APK顺序队列完成（约25.8／37.9／9.8／97.1秒），仅安装一次。APK SHA256 **`c739e98384174796698d0abeaedcce2a4e95df48c128092ee2cb22aae3931700`**，包名`iot.diandeng.tech.beta`、原签名、HTTPS和1156份Web资产核同，日志／生命周期诊断关闭。原生owner源码／class、WiFiProv **2.4.4** JAR、ESP32 Core **3.3.11**及依赖锁不变，无服务部署或SDK升级。

UI/UX检查只影响共享字段的标签／错误关联、用途说明与既有48px控件，不另造页面。真机320px竖屏无横向溢出；横屏、字体、暗色和辅助技术专项不据此销项。

### 18.4 一次OEM managed连接与真实扫描

现场PGEM10／OPlus和COM11身份／原Admin核同。仅移除这个测试AP的旧普通保存项，全部路由器保存项保留；未用CLI预连设备热点。App就绪后仅COM11临时切既有非空PoP SoftAP应用，原10分钟网络窗口只开启一次。本批只有连接及读取，不点击发送，不触及固定GetInfo、Bootstrap、身份、WiFi配置或任务写。

| 环节 | 本次真实差异证据 |
| --- | --- |
| create／连接 | 唯一UI连接、唯一managed create及原有参SDK connect。新owner前后一致，Security1；connect **6776ms**返回`connected=true`。原生同worker日志核SDK fullEntry／精确onAvailable／proto-ver各1，无无参入口或unavailable。 |
| 系统授权 | 系统picker只选择一次精确AP行，点击前没有Connecting／Connected状态；随后自然返回App。没有强拉Main、BACK、预连或重复点“正在连接”，不以普通WiFi IP代替SDK回调。 |
| 前台继续 | 系统遮挡返回后沿原acquisition显式“继续连接设置”一次；不是自动续写或后台放行。相同owner新鲜`getDeviceInfo` **27ms**，随后实际SDK扫描 **3987ms**，原页面展示**15个真实网络**。 |
| 离页 | 一次原首页导航触发`clearState`且收到成功；最终原生隔离worker不存在、观察器／binding／9479转发回收。手机已自然恢复原路由，无额外选网。 |

独立离线审阅确认上述调用、阶段及owner证据；原生identity／custom／WiFi／reset／固定GetInfo调用和观察器拒绝计数均为0。页面扫描截图只作展示旁证，不替代同owner回调／原日志。没有云端完成确认、进入设备、控制或新的UNKNOWN操作。

**因果边界：** 本批同时修复PoP透传、移除本测试AP旧保存项，并去掉旧工具强拉App／重复点Connecting行的操作混杂。因此可确认“当前匹配条件下OEM自动连接／扫描已可用”，但不能单独认定旧onAvailable缺失由哪一个条件唯一造成；PoP修复也不被追溯写成此前网络获取超时的根因。不再为拆分归因重复采样或改SDK。

### 18.5 终核、证据与下一步边界

本轮`n`开窗清了设备网络，且没有随后配置写入，因此与§17合法新网络保留不同：结束已先新鲜比对原instance／根／Key／TaskStore，再恢复**原103c应用及本批原20KiB NVS**；ROM核应用／NVS精确字节和未改的分区表通过。正常启动后的独立终核亦确认原五个网络逻辑键、instance整记录、根／Key整blob、epoch1／Admin1及空TaskStore精确保持，串口已释放。没有借恢复操作更换或清除授权；保护状态不匹配即停止的守卫未触发。

App before／installed／final的账号完整session、3项secure（含原授权和旧allocation／UNKNOWN）及完整task／automation journal摘要精确一致。匹配新版App保留并停首页，无隐式身份／网络写、发码或服务变更；原日志、完整请求、前后账号记录及截图均保存在ACL私有目录，不公开Token、PoP或网络密钥。软件／候选／实机摘要位于App `build/oem-softap-entry-20261031`，其中`live-accepted.json`统一核源码／锁仍冻结、唯一候选、独立审阅、离页及恢复事实；事件原文45941B、SHA256 `35bf7e66…d4d7a2`。设备终核位于库 `build/oem-softap-entry-live-20261031`；忽略目录不作为必要源码提交。

**收口决定：本批仅关闭“当前Android OEM、匹配PoP及系统自然授权条件下，自动SoftAP连接→安全会话→真实扫描→离页回收”入口缺口。** 既有BLE正常入网取§15、手动保根SoftAP闭环取§17，已过项不再重跑。本次未发WiFi、未做完整managed身份／Cloud入网，也未验证旧保存AP／其他OEM的全组合；旧UNKNOWN、自然持久UNKNOWN正向及LAN仍维持各自边界，不补造记录或自动重发配置。后续若要补managed完整入网，只安排一次有明确业务必要的新操作，复用现有身份／CAS／只读协调，不重建执行链。本批暂不提交推送。

## 19. ON-4j：OEM 自动 SoftAP 保根完整入网短验（2026-10-31，有限收口）

### 19.1 本批最小范围与新发现的前置缺口

用户明确接续§18补一次完整managed入网，不重复BLE、手动SoftAP或连接／扫描专项，不清根、解绑或新建设备。现场仍仅COM11，原103c应用、instance、epoch1／Admin1、完整根／Key、空TaskStore及20KiB NVS与§18终核一致；现有账号及原授权／旧UNKNOWN保留。Cloud owner detail与Admin只读均200／no-store，同账号／instance／controller／CAS匹配，无需Key reveal、近期登录或验证码。

开始写前核对发现一个既有接线缺口：`installBlinkerIdentity`的已有根network-only快路仅覆盖BLE及手动WiFi-only；普通managed会落入旧allocation／UNKNOWN的禁重放前置，或走不必要的Key读取。不能通过删除旧记录、重做Bootstrap或只放宽测试guard绕过。因此本批必要最小源码范围是：**显式重配＋已隔离SDK-managed SoftAP**复用原`inspectExistingNetwork`快路；所有该SoftAP都先调用原`prepareCloud`，维持旧owner实际死亡、显式Cloud、fresh resolve／Admin／CAS、新Local不同owner和新鲜同instance／epoch／完整根检查。纯手动只读、空根首装和旧UNKNOWN恢复不获写许可，不新增协议或native能力。

先以真实Page夹具复现门禁、最小修正并补受影响有限回归；仅因该实际缺口构建／安装一个新匹配App，复用原WiFiProv2.4.4及已编译c051固件，不升级SDK或重复无差异构建。其他已通过项不重跑。

### 19.2 一次写入与证据合同

- 原App保留账号／授权、完整journal与原始日志，只读读取原设备重配上下文。COM11仅临时切应用，App就绪后只开启一次原10分钟网络窗口；不清根或任务，不从旧记录生成新UNKNOWN。
- 第一个SDK-managed owner自然系统选精确AP→原安全会话／扫描；这些是到达本次新WiFi操作的必要前置，不另开专项循环。只允许固定`blinker-config`两字节GetInfo（`02 01`），禁止任意custom、Bootstrap、typed身份写、reset及Key／Admin写。
- 原owner实际关闭后用户式显式Cloud交接，fresh resolve／Admin／CAS全部匹配；回Local创建不同owner、原SDK-managed申请网络、新GetInfo比较同根后，**仅一次原`provision`发送本次新WiFi**。两次连接属于原Cloud／Local执行链所需不同owner，不是失败重试；各保留原60／90秒预算，不以SSID／IP／ready或等待解除隔离。
- 官方回执与设备实际Cloud可用性分别记录。先确认owner死亡，再沿原有界Cloud观察核本次新session／State；自然前台满足条件则自动进入一次；若切网读失败则保留确认，最多一次原显式只读检查／进入，不重发配置，不为自动进入凑样本。
- 真机失败即停在本次首个真实分歧，保留UNKNOWN和完整证据；不重用旧请求。结束核根／Key／任务、账号／UNKNOWN与资源回收。若新WiFi确已合法写入，只恢复原应用并保留新网络；若没有配置写入，再恢复本轮开窗清除的原网络，不用整片回滚掩盖未知结果。
- 暂不服务部署、发码、SDK升级、提交或推送；LAN仍后置。实际结果另记，不把本节合同当作通过证明。

### 19.3 最小修正及匹配候选

已由真实Page有限夹具复现旧managed保根路径不能进入原Cloud关闭阶段，再仅修两处产品分支：带原隔离stage的SDK-managed显式重配纳入已有根快路；该SoftAP统一先走`prepareCloud`。继续原Admin／CAS、只读resolve、不同owner重入与新鲜同根检查，不动WiFiProv原生接口、首装和手动只读授权。旧allocation／UNKNOWN不加载续写、不完成、不删除；没有Key读取或Admin登记。

本批13个不同有限用例、App／spec类型检查及diff检查通过：新增managed10项（含新owner根变化、CAS、Admin缺失、后台、换账号、缺隔离stage与旧UNKNOWN零写），受影响只读／首装边界3项。未重跑先前手动／BLE完整矩阵；最初夹具类型错误及真实行为失败日志保留。自然picker只对本次两owner顺序补9项工具差异回归，不能混称产品或真机通过。

唯一prebuild／Web／sync／离线APK队列完成，约26.9／39.9／13.4／103.0秒。匹配APK SHA256 **`7f5eb340adfcca2b4509ef4e7596c1bc29500236af3226c985ca497d7a9e2fc4`**，只覆盖安装一次、不清数据；包名、原签名、HTTPS、1156份Web资产和真实原生注册表核同。WiFiProv2.4.4、native owner源码／class及SDK JAR、原Core3.3.11和依赖锁保持，未部署或重建服务。前批PoP修正保留，未重复构建旧候选。

### 19.4 一次新WiFi完整短验的实际结果

本次原10分钟窗口只开启一次；普通系统保存列表没有该测试AP项，未再删除任何路由器配置或预连AP。以下两次连接是原Cloud／Local交接所需的**不同owner**，均自然系统授权、无强拉App／重复点Connecting、无失败重连：

| 环节 | 本次真实结果 |
| --- | --- |
| 初始Local | 原SDK-managed连接**8423ms**；同owner安全会话／扫描**4109ms**返回13个真实网络，原页面选择目标SSID。 |
| 原根与Cloud | 固定2B GetInfo **91ms**核flags7／epoch1／原instance；clearState **67ms**完成后才显式Cloud步骤。新鲜resolve及Admin成功、同CAS，没有reveal／身份写／旧UNKNOWN恢复。 |
| 新Local | 创建不同owner，managed连接**8197ms**；第二次固定GetInfo **591ms**核同instance／epoch／完整根后才继续原下发。两次SDK fullEntry／精确onAvailable／proto-ver均各有实际记录，无无参入口。 |
| 唯一新WiFi | 单次UI发送、单次原`provision`，**7528ms**返回官方`success=true`；随后clearState **74ms**成功。无Bootstrap／Install／typed WiFi／reset，未刷新Key。 |
| Cloud实际确认 | 首次本次完成读取遇到自然`ERR_INTERNET_DISCONNECTED`，页面保留“WiFi已确认、云端尚未就绪”；手机自然回原路由后只点一次“再次检查状态”，POST200（约265ms）。实际响应核原instance、expected／returned CAS和本次新会话，State在该session签发后1820ms更新；结果页显示“设备已上线”，不是把官方成功直接当Cloud在线。 |
| 成功进入 | 本轮是一次显式只读恢复后再点一次“进入设备”，目标原设备路由恰一次；截图显示“云端已连接”。**这是显式恢复／进入，不是自动导航验收**；不为补自动进入样本重复配网。之后回首页，未发送控制或任务命令。 |

第一个Cloud错误没有撤回已确认的WiFi结果，没有自动重试或配置重发；本批没有产生新UNKNOWN。旧UNKNOWN仍按原证据保留，不能由本次新成功请求销掉。SDK完成／Cloud新鲜可用与路由进入分别留证，不依赖SSID／IP／ready快照互相替代。

### 19.5 恢复与收口边界

已恢复原103c BLE应用并ROM校验；保留本次合法的新WiFi网络NVS，未执行restore-baseline或任何NVS写回。终核原instance整记录、根／Key整blob、epoch1／单Admin、空TaskStore及分区与本批before精确一致；当前五网络逻辑键与应用恢复前一致。正常启动、串口释放，其他板未动。

App账号完整session、3项secure（含授权／旧allocation／UNKNOWN）及完整task／automation journal在before／installed／final精确一致。匹配App保留，最终首页；原WiFiProv worker不存在、nativePromise还原、观察器／binding／9479转发及临时系统XML已回收。原始请求、响应、系统／SDK日志及前后账号记录在ACL私有目录保留；脱敏摘要位于App `build/oem-softap-full-20261031`，设备证据位于库 `build/oem-softap-full-live-20261031`。事件原文307162B、SHA256 `7cd70682…3f26d8`；不公开PoP、网络密钥、账号token或Key，不提交忽略目录产物。

独立离线审阅`independent-live-audit.json`及统一验收`live-accepted.json`均已通过，源码／锁仍冻结，物理恢复与账号终核分别交叉核对。普通阶段resolve／Admin保留实际HTTP200、页面原准入检查通过，正文未单独捕获；预检Admin／CAS及完成确认的expected／returned CAS已独立核对，不冒称每份普通HTTP正文均复核。CDP本轮没有WebSocket事件，不将其写成“全部WS控制／任务流量为零”；操作者没有发送控制／任务，完整journal及本批native／HTTP有界证据保留，进入页后的原实时链不另作新性能／最终Closed ACK验收。

**本批有限收口**：仅限**当前OEM、Security1／匹配PoP、已有根及当前账号Admin的自动SoftAP保根新WiFi→官方成功→一次显式Cloud恢复→进入原设备**；不是空根首装、全OEM或所有后台／取消组合通过，也不是全自动免交接入网。正常BLE与手动保根SoftAP取先前冻结结果，不再重复。LAN与自然持久UNKNOWN样本仍单列，不补造；本批无验证码、SDK升级、自动重发、Git提交或推送。后续仅处理新真实缺陷或明确未开放的产品路径，不默认重跑本次成功操作。

## 20. ON-6：接入剩余清单与明确WiFi失败分类（2026-10-31，软件有限收口）

### 20.1 §19后的真实剩余，不重组已过队列

主线仍是WiFi设备通过BLE／SoftAP入网；不是日常Direct BLE、任务、自动化或OTA。当前已有根正常路径已有限可用，不能继续沿历史“managed冻结／SoftAP未完成”字句安排重复测试，也不据单手机成功宣布所有平台完备。

| 分类 | 范围／事实 | 后续处理 |
| --- | --- | --- |
| 已有限收口，冻结 | §15 BLE官方确认→本次Cloud／State→正常前台自动进入；§17手动保根SoftAP完整闭环及显式恢复；§18／19当前OEM managed连接／扫描及保根完整入网 | 不再重复正常配网，不清根重造同一成功样本。 |
| 软件已修，兼容矩阵不泛化 | 扫描取消、扫描／发送互斥、离页回收、首次账号恢复／冷重配上下文、PoP、managed network-only快路 | 报告#13／29／30／31／35／36／49／51不能一概写仍故障，也不能一概跨机型销项；仅对新真实失败按实际版本／固件追踪。 |
| 已证小缺口，本批实施 | 原SDK明确`AUTH_FAILED／NETWORK_NOT_FOUND`在隔离SoftAP丢失；typed进一步变UNKNOWN；BLE仅英文message且null可异常；App只按message识别 | 统一稳定、非秘密错误码，沿原owner／ticket／IPC到既有结果页。仅改原因传递，不改执行链。 |
| 待自然样本，不是已证缺陷 | 新版真实UNKNOWN的跨重启／同账号显式核对／精确清理；自然30秒未确认；新版真正后台后的正向恢复；最终实时Closed ACK | 保留软件与既有证据边界；没有匹配新记录就NOT_RUN，不从旧allocation／日志补造，不制造UNKNOWN补表。 |
| 核心后续验收，尚未闭合 | 空根BLE／managed首装完整验收；手动空根首装§34有真实UNKNOWN及云上线，未取得官方成功 | 失败分类软件收口后，先核一套不覆盖当前根授权／任务的新鲜首装前置，再明确一次必要新操作；无近期登录或安全测试身份则只准备方案，不发码、绕过Key门禁或清原根。不能用保根重配成功代替首次接入，也不能改写§34旧回执。 |
| 明确后置／未开放 | 新OEM／旧保存AP组合；LAN完成判定；iOS／低版本／多机型全矩阵；ESP8266、同镜像双路、配网任务组合、分区／OTA | 分别按产品准入另开最小阶段，不能借本批两码分类扩大支持；不是当前正常保根链的已证故障。 |

当前未发现另一个有明确证据的未修保根正常路径阻断。**新设备首次接入仍是核心剩余验收，不永久后置**；只是本批不破坏当前授权或无近期认证就强行首装。跨平台／自然样本缺口保留，但不以“所有场景全通过”为无限期审计目标；定位历史§34回执UNKNOWN的根因不属于本批，两码新合同不能追溯改写旧结果。

### 20.2 本批最小合同

固定WiFiProv **2.4.4** 本地SDK JAR实际枚举为`AUTH_FAILED／NETWORK_NOT_FOUND／DEVICE_DISCONNECTED／UNKNOWN`；本批按该固定消费者接线，不升级SDK、不解析异常字符串或根据Cloud在线猜原因。

| 原SDK回调事实 | 稳定码／结果 | 禁止推断 |
| --- | --- | --- |
| `AUTH_FAILED` | `WIFIPROV_WIFI_AUTH_FAILED`，官方明确拒绝 | 只能说密码或认证失败，不能断言一定是密码错误。 |
| `NETWORK_NOT_FOUND` | `WIFIPROV_WIFI_NETWORK_NOT_FOUND`，官方明确拒绝 | 给名称／频段／信号检查指引，不猜硬件损坏。 |
| `DEVICE_DISCONNECTED／UNKNOWN／null`及传输／会话／apply异常、超时、提交后取消 | 沿现有UNKNOWN／隔离合同 | 不能洗成明确失败，不因后来的明确回调覆盖先前UNKNOWN。 |

- 原生只增加一个小型共享SDK枚举映射，接原BLE／隔离SoftAP generic／typed回调；实际当前owner／operation／ticket、先取消／超时／失联及单终态检查保留。typed明确拒绝仍须原Security1／PoP合同，WiFi机会已消费，不重置或重发。
- 原IPC仅在`provision／provisionBlinkerWifi／provisionBlinkerWifiOnly`三种方法白名单接受两个稳定码；custom、Bootstrap、旧nonce／request及迟到结果不能借码降级UNKNOWN。原进程死亡及Cloud释放合同不变，不以等待／ready／SSID解隔离。
- App复用一个小纯分类器：新code优先，UNKNOWN或其他明确code不可被英文message覆盖；旧精确英文仅兼容无code或原BLE`WIFIPROV_DEVICE_REJECTED`。原typed薄端口已经保留普通明确错误，仅补必要回归，不另造端口／存储／状态机。
- 原完成器／结果页复用“WiFi连接失败＋原因”，有恢复指引但没有本页重发入口；明确拒绝后只做原owner清理，不查询Cloud、不生成UNKNOWN或持久checkpoint。断线／超时仍走原UNKNOWN只读确认；保留已有账号、CAS、授权及历史记录。
- 本批先红后绿：实际SDK回调／IPC／public bridge、typed、Page及必要模板有限回归，补原SDK单一插件Java定向编译和App类型核查。不构建APK、不部署、不操作ADB／COM11或账号，不故意发错密码制造真机记录，不重复§15～19。

UI/UX规则仅核错误反馈明确、原因不武断及沿用原状态宣告／恢复指引；不改页面布局、颜色、控件或动画，不把有限模板回归说成真机视觉／多机型通过。实际结果另记，暂不提交推送。

### 20.3 实施及有限验证结果

已完成本批两码合同，复用原WiFiProv及身份／完成执行链：

| 范围 | 本次实际结果 |
| --- | --- |
| 原生SDK／IPC | 共享`ProvisioningFailure`仅映射两明确enum，null安全；BLE／SoftAP generic／typed接同一分类。typed仍核原Security1／PoP、current ticket和已消费的一次机会；Channel仅三种WiFi provision允许新码，custom／Bootstrap／reset／read不可借用。原IPC、deadline、关闭及Cloud fence未改。 |
| App分类／结果 | 小纯模块供Page与Stage共用，删除Page内旧message-only函数。稳定code优先，UNKNOWN／其他明确code不能被旧文案洗成失败；有限兼容旧BLE。保留原失败标题、认证／AP检查指引及状态宣告，没有新重发按钮或Cloud执行链。 |
| 原端口与存储 | 原两个typed生产端口不必修改：当前请求的普通明确错误原样保留，retired／abort／close-unknown仍UNKNOWN。原Admin／CAS、managed保根快路、PoP、HTML／SCSS、依赖锁及所有UNKNOWN／存储合同保持。 |
| 原生有限回归 | 两个关键JUnit红分别实际复现LocalOwner丢enum和BLE统一旧拒绝码，修正后**15个不同方法**通过（10新增＋5受影响），覆盖generic、typed首装／保根、BLE public bridge及IPC。每个方法内枚举／安全变体不重复计数。 |
| App有限回归 | 两个关键Page红复现稳定code被误判UNKNOWN，修正后**29项不同用例**通过：纯分类6、真实Page10、Stage3、typed首装5、typed保根5；108个跳过项不计通过。明确拒绝零Cloud／导航／UNKNOWN持久化／重发，普通未知与取消仍受原合同保护。App／spec noEmit及必要diff检查通过。 |
| 原SDK真实编译 | 单一离线`:capacitor-wifiprov:compileDebugJavaWithJavac`成功，约**62.5秒**，17项任务中该编译实际执行；原SDK2.4.4、Gradle8.14.3、max-workers1不变。沿既有source.gradle明确绑定当前插件工作树，不误用App pnpm中缺新helper的旧副本；不是APK或无线验收。 |

初次worker host运行参数拼写错误的编译失败日志保留，校正后只补该未通过组，不重跑前12个已绿方法；原SDK编译的flatDir／CRLF提示未通过升级依赖消除。源码、必要测试及runner摘要冻结在插件`.build/onboarding-rejections-20261031/{source-freeze,green,compile}.json`，App证据在`build/onboarding-rejections-20261031/app-scope-verified.json`；既有私有日志不动，不将忽略目录产物当源码交付。

独立有限源码审阅无阻断，`independent-review.json`离线核对15份App及12份native冻结输入一致，未再执行测试、编译或设备请求。UI/UX规则只用于保留清楚且不武断的失败／恢复指引，未新增视觉设计或矩阵验收。

本批**没有APK构建／安装、服务部署、ADB、串口、账号／授权／UNKNOWN操作或新的真实请求**，不会改变§19已安装App／设备的行为。没有发送验证码、升级SDK、自动重发或提交推送。软件／host证明不能写成匹配手机运行或真实官方失败回执已经验收；原无线成功事实继续只引用§15～19。

### 20.4 收口后的最小顺序

1. **本批软件到此冻结**：不再扩错误枚举／诊断框架，不重建全套测试；必要独立源码终审和冻结摘要用于交接。
2. 若安排下一批运行验收，必须先构建并安装同时包含新Java及新App分类器的一个匹配APK；仅补官方明确拒绝→原结果页／清理的必要差异，不重跑正常BLE／手动／managed完整入网，不靠仅Web更新声称原生分类已生效。实际无明确拒绝样本就保留待验，不将通用UNKNOWN改作认证失败。
3. **核心剩余回到新设备首次接入前置**：梳理空根匹配固件／PoP、独立安全测试身份及近期登录是否齐备，再限定一次必要首装；不默认清当前COM11原根、覆盖既有授权／任务、重用旧scope或发验证码绕门禁。保根成功与首装成功分别跟踪。
4. 持久UNKNOWN三项正向和自然后台／超时等保持自然样本门禁，不阻塞已可用正常路径；LAN、其他OEM、平台及扩展内容继续后置。

## 21. ON-6b：匹配App与一次明确WiFi拒绝差异（2026-10-31，有限收口）

### 21.1 唯一运行范围

按用户授权，本批只运行§20已冻结实现，不重跑29项App／15方法native软件矩阵，不修改产品代码。先保留完整账号／授权／journal及请求／SDK日志，唯一构建安装同时包含共享`ProvisioningFailure`、原生消费者和App分类器的beta；固定原SDK2.4.4、原签名与HTTPS，明确编译当前相邻插件源码，避免pnpm旧Java副本。仅COM11，原身份／Admin／CAS、根／Key及任务不变，无服务部署、Key读取／刷新或验证码。

使用已验SDK-managed保根两owner执行链，仅作为到达本次失败写入的必要前置：原安全会话及实际扫描→2B GetInfo→旧owner真实死亡→原Cloud身份准入→不同owner同根核对→**一笔新WiFi配置**。为避免对真实路由器做错误密码尝试，优先在真实扫描确认不存在后，通过原`#wifi-ssid`手输固定非真实SSID `BLINKER_TEST_ABSENT_6D692032`和非秘密测试口令；“记住WiFi”必须未启用，不改变原保存偏好。系统自然选AP，不预连、强拉App或重复Connecting行；不放宽60／90秒预算。

本次期望实际官方`WIFIPROV_WIFI_NETWORK_NOT_FOUND`，再核原页面“设备未能连接WiFi＋未找到目标网络”检查指引、明确拒绝而非UNKNOWN、没有结果页重发／进入／再次检查入口，以及原owner清理。**必要的身份resolve／Admin读取与`provisioning-cloud-v1`完成查询分开计数**；明确拒绝后完成查询须为0。若实际只得到UNKNOWN／超时或其他事实，如实停止并保留，不延长SDK截止、不重发，不拦截Cloud请求后伪称0，也不改写历史UNKNOWN。

### 21.2 设备与账号收尾

设备在收到凭据时就可能将不存在的测试SSID持久化，不能沿§19“保留合法新网络”直接结束。本批仅保留必要的新鲜20KiB NVS／分区／原根任务摘要及已验恢复应用，不做8MiB全备份。只有匹配本次单WiFi调用的官方明确拒绝、零身份写、无新UNKNOWN且fresh根／Key／任务仍与before精确一致，才批准**测试清理恢复本批原20KiB NVS与原103c应用**；ROM核字节、正常启动后核原网络／授权，不通过发送正确WiFi做第二笔业务恢复。若UNKNOWN或保护状态漂移则停止，不覆盖NVS掩盖未知意图。

App账号／授权、旧allocation／UNKNOWN及完整journal逐字摘要终核，真实日志不删除／回滚；匹配新版App保留并回首页，核worker、listener／工具binding／调试转发回收。截图仅核本机失败提示与操作门禁，不扩大为全视觉／权限／平台或全部失败类型通过。原始秘密仅留ACL私有目录，不入Git；暂不提交推送。实际结果另记。

### 21.3 匹配构建、安装与唯一失败结果

唯一prebuild／Web／sync／离线beta队列通过（约32.6／40.3／13.7／97.0秒），没有重跑§20已通过矩阵。匹配APK SHA256 **`e6c9b55983636a35cbbbb6674cd54c088975674ef1a3f0628395ac888d0e8554`**，实际beta包名、原签名、HTTPS及1156份Web资产核同；新共享映射、owner／channel和BLE实际消费者class，以及DEX／Page两码和原UNKNOWN／cleanup路径都已入包核对。原source.gradle直接绑定当前插件源码，未消费pnpm旧Java；SDK仍2.4.4、JAR摘要不变。只覆盖安装一次，不清App数据／重登录／发码；原账号及3项secure／完整journal摘要保持。

COM11新鲜audit核原103c应用、20KiB NVS、根／Key／epoch1／单Admin／空TaskStore及分区与§19终核一致，Cloud预检owner／Admin均200且no-store，同账号／instance／controller／CAS。仅临时写原c051应用，原10分钟窗口只开一次；没有清根、解绑、创建身份或Key读取。

| 环节 | 本批真实结果 |
| --- | --- |
| 必要Local前置 | 两个不同managed owner均原SDK精确连接及自然系统返回；原2B GetInfo分别核flags7／epoch1／同instance。实际首次扫描15行，不含测试SSID；通过原输入框手填，未选扫描中真实路由器，“记住WiFi”为false。 |
| 原身份准入 | 首owner真实关闭后仅原resolve／Admin读取，均200；按原Cloud／Local阶段重入不同owner并核同根。无Key／Admin／Bootstrap／Install／typed写。 |
| 唯一WiFi终态 | 第二owner单次原`provision`，实际**7512ms**返回 **`WIFIPROV_WIFI_NETWORK_NOT_FOUND`**；不是工具抛出／注入结果，不是通用UNKNOWN，也没有第二笔配置。原60秒截止未放宽。 |
| 原结果页 | 主标题“设备未能连接WiFi”，原因“未找到目标WiFi，请检查名称、频段及信号”，并提示不会自动重发；详情是“设备明确拒绝”。无进入／再次检查／重发按钮，busy=false。 |
| 完成Cloud与清理 | `provisioning-cloud-v1`完成查询**0**，WiFi下发后resolve**0**，check／open／目标设备导航均0。第二owner拒绝后的clearState约**67ms**完成；正常回首页，无新UNKNOWN、持久checkpoint或旧记录消费。 |

SDK原日志核两次fullEntry／精确onAvailable／proto-ver及worker终止，但没有额外打印一条NF枚举原始日志；理由证据是**真实同owner原生拒绝code＋匹配编译映射的唯一SDK回调来源**，不编造第二份日志。本批公开观察只采同owner UUID／唯一方法／原call-error时间，没有额外采native数字requestId；内部IPC仍按原nonce／id／ticket检查，但不能将它冒称额外外采证据。身份准入只读请求不计为完成Cloud查询。

### 21.4 精确恢复、保留与收口

真实NF、唯一请求和零身份写／无UNKNOWN的guard核对后，仅做测试清理恢复，不发送正确WiFi：原103c应用和本批原20KiB NVS **`01664771…220a9`** 均ROM逐字校验通过；正常启动后final NVS仍精确同before，五个网络逻辑键、instance整记录、根／Key整blob、epoch1／单Admin、空TaskStore及分区全部一致。串口已释放，没有第二次开窗、业务重发或其他板操作。

App before／installed／final完整session、3项secure（含原授权、allocation／UNKNOWN）、完整task／automation journal均精确一致；保留匹配新版App并停首页。原配网worker不存在，nativePromise已还原、观察器／binding／9479转发及本批临时系统XML回收。原始日志、完整请求／响应和账号记录留在ACL私有目录，不删／回滚历史记录，不公开PoP、token、Key或网络密钥。

独立离线审阅及统一验收均有限通过，App `build/onboarding-rejections-live-20261031/{independent-live-audit,cleanup-verified,live-accepted}.json`保存脱敏摘要，原事件105202B／SHA256 `f30ffdd7…90e28a`；设备guard／ROM／终核证据在库 `build/onboarding-rejections-live-20261031`。没有新增产品源码、服务部署、验证码、SDK升级、提交或推送。UI/UX规则只用于检查本机竖屏原因／指引与操作门禁，未改布局，不冒称多机型／字体／暗色矩阵通过。

**本批关闭匹配Android App的generic managed SoftAP单次NETWORK_NOT_FOUND运行缺口。** AUTH_FAILED、BLE／typed的明确拒绝仍只有§20软件／host证据，不为补矩阵重复失败操作；本批不产生UNKNOWN，也不据此声称其持久恢复或历史根因已完成。下一批回到§20.4的新设备首次接入前置：先核非破坏的空根／测试身份／近期认证条件，再限定必要首装，不默认清当前根、重用旧scope、发码或重跑成功路径。

## 22. ON-7a：空根首次接入的最小范围与前置核对（2026-10-31）

### 22.1 本批目标与边界

只解决“**一个真正的新设备，经现有WiFiProv及身份链完成一次首次接入**”的剩余验收，不重新测试已有根的BLE／手动SoftAP／当前OEM managed成功链。先核硬件、固件、非空PoP、独立身份、额度及近期认证；具备条件才进入一次写入，条件不足只完成非破坏准备，不通过清COM11原根、删除原设备或复用旧scope制造首装。

首装与保根重配必须分别判定：首装的固定GetInfo应为完整空根、`flags=2／epoch=0`，随后新Cloud身份与CAS、Admin、Bootstrap；现有COM11的`flags=7／epoch=1`属于保根重配。发现任意旧allocation、Admin或Cloud归属即停止该首装路径，不按“重试”清理或覆盖。§34旧UNKNOWN及其原instance仍保留，不能换名称后当新身份。

### 22.2 已核前置及当前阻点

| 前置 | 本批事实 | 决定 |
| --- | --- | --- |
| 安全硬件 | 只读USB枚举仅COM11 XIAO ESP32S3。§21终核已恢复原103c应用、20KiB NVS及原根／Key／Admin／任务；本批未打开串口或改变存储。 | 当前没有可直接使用的独立空根板。旧“empty-NVS”脚本会写原默认`0x9000 + 20KiB`，不是并行隔离，不运行。 |
| 可复用固件 | 现有SoftAP `c05195d7…b96eff`产物哈希有效，沿原Core3.3.11，私有非空PoP输入匹配，固定Bootstrap已实际链接，0任务槽且未嵌入Key／WiFi。无另一个已验证的并行测试NVS区。 | 优先把这一条显式手动SoftAP空根路径作为后续最小候选；换板后仍须核板型／分区、生成的全新instance及实际空根，不能把旧COM11身份搬过去。 |
| BLE候选边界 | 原`103cdde6…ca1b4`是BLE恢复应用，未带上述私有PoP配置。 | 不能仅凭旧恢复BIN声称非空PoP首装已匹配；BLE空根正向仍待匹配候选，不在本批另开编译队列或重跑扫描／连接。 |
| App消费者 | 手机现装beta SHA256 `e6c9b559…0e8554`与§21一致。现有首装／原allocation／Admin／CAS及显式手动首装消费者可复用。 | 不重建、不安装、不新增页面或执行链。必须从普通添加入口进入，不能携带原设备`reconfigure／logicalDeviceId`上下文。 |
| 账号与额度 | 本批一次`GET /api/v1/auth/me` **200／553ms**，一次`GET /api/v2/devices` **200／114ms**；同一原账号，`free`、`iot.devices=1`、非deleted的owner设备去重计数**1**，可用**0**，权益revision1。未包含分享设备数。 | 当前没有独立Cloud测试身份的创建空间；不试探CREATE、不删除原设备腾额度、不改账号权益。若继续，先取得1台有效额度，正常权益或另行批准的限定调整均不能改旧设备／scope。 |
| 近期认证 | 实际Gateway代码对Key管理检查可信primary login产生的**5分钟**标记；refresh、`/me`及“已登录”不续期。当前没有调用Key reveal，也没有执行登录或独立状态探测。 | 仅标**未新鲜验证**，不能凭JWT年龄断言过期或凭`/me=200`断言满足。硬件／固件／额度及工具全部就绪后，再安排已有密码的近期登录；本批不发码、不绕过门禁。 |

当前首装有**硬件、额度、近期认证三个前置**，不是已复现的WiFiProv失败。普通Arduino代码及SDK不需为绕开这些前置而重构；不新增存储分区、Auth接口或另一套认领流程。

补充离线身份核对：c051不包含旧COM11的instance／controller／Key硬编码。原`DeviceInstanceIdStore.loadOrCreate`在instance缺失时以硬件随机数生成并持久化，已有记录则保留；“空root”本身不等于“全新instance”。真正空NVS、同型号及相容分区的新板，身份方面无需重编，但必须另建绑定该板真实USB／MAC的薄guard，不能运行旧COM11操作脚本或写入旧NVS。现有App冻结及私有PoP输入匹配不等于未到场的新板已通过GetInfo／Security1安全会话验收。

特别注意原执行次序：**CREATE与allocation CAS绑定可能先成功，随后reveal才检查近期认证**。因此不能“先创建一个看看”；若后续真实执行已创建并在Key门禁停止，应保留该次request／allocation和原CAS，不再CREATE第二个、不刷新Key、不将其旧记录改造成UNKNOWN。

### 22.3 前置齐备后唯一最小真机步骤

本批没有启动10分钟窗口。后续只选一条准备齐备的路径，优先复用上述显式手动SoftAP候选，不把空根身份与另一个未验OEM自动连接组合同时引入。

1. **窗口外准备**：接入独立专用空根同型号板，核实际身份／分区与匹配候选、私有非空PoP；保留当前COM11。核账号有1台可用额度及Cloud网络可达；不从旧UNKNOWN获取instance／scope或删除原分配。不得用恢复旧NVS或“空根标志注入”伪装新设备。
2. **近期认证就绪后再开窗**：通过正常App沿用同账号已有密码登录，立即进入本次有限窗口；不自动发验证码或反复登录。近期认证仍由原Gateway实际Key门禁判断，不关闭校验、不读出Key后输出或另存公开脚本。
3. **Cloud／Local及新身份**：普通添加入口读取本次owner的固定GetInfo，核全新非零instance、完整空根／epoch0、supportsBootstrap。原Cloud阶段按`createNew → CREATE → allocation CAS绑定 → reveal → pending Admin／register`执行；未知、冲突或拒绝就停止，保留意图，不自动重新分配。
4. **一次性写入**：按原Cloud／Local owner死亡屏障交接，不凭ready／等待／SSID解除隔离；新owner重新核同instance／epoch0／空根及实际Security1／非空PoP。沿原薄端口提交固定Bootstrap，只有精确ACK和原CAS成立后才允许**一次WiFi下发**；任意custom／旧根手动写继续不开放。
5. **完成判定与收尾**：官方回执与Cloud可用事实分别记录；只读确认须同新身份／CAS、本次新会话及新鲜State。正常前台完全确认才自动进入一次；后台／UNKNOWN保留显式只读入口。超时不能写成明确失败，UNKNOWN不补发Bootstrap或WiFi。保存完整日志和旧记录，新身份后续保留或正常移除另按本批收尾决定，不删除原COM11设备。

首装最小通过条件是新身份分配／CAS／Admin、新owner同空根、Bootstrap精确ACK、唯一WiFi及Cloud新会话／State可关联；官方成功回执有无单列，不用Cloud成功改写UNKNOWN。只补新路径，不重复§15～21，不扩为错误／多机型／断电全矩阵。原根手动写、安全隔离及超时合同均不放宽。

### 22.4 本批实际推进与停点

已完成只读硬件／候选／源码消费者及账号额度核对，未发现需要先改产品源码才能完成这条首装的证据。本批使用现有小型账号／journal快照，不复制APK、不做Flash备份；一次两接口GET以现有token、no-store及10秒整次截止执行，非200即停，无刷新token／登录／业务重试。请求／响应及完整logcat仅保留ACL私有目录，公开摘要无token、PoP、Key或WiFi密钥。

App before／final的同账号、完整session、3项secure（含原Admin及allocation／UNKNOWN）及完整task／automation journal逐字摘要一致，匹配APK未变化；CDP／9479转发已回收。脱敏摘要位于App忽略目录`build/onboarding-first-install-preflight-20261031/{account-read,accepted}.json`。这些只证明本批非破坏预检与原记录保持，**不证明近期认证或首次接入真机已通过**。

独立只读复核确认：`/me／devices=200`不代表近期认证成立，也不能证明已过期；剩余额度0不应试探CREATE。候选身份生成／保留及PoP边界已离线复核，没有因此增加API调用、编译或串口操作。必要文档差异／格式检查通过，不重复软件或无线已过项。

**当前停点：新空根首次接入NOT_RUN，等待独立安全硬件及1台有效创建额度，全部准备好后再完成近期认证。** 本批未创建身份、查询／刷新Key、打开串口、清NVS／刷板、发送配置或验证码，未运行服务部署、SDK升级、重复构建／无线测试、提交或推送。COM11原根不动，旧scope及UNKNOWN不消费；不因缺前置转去重复保根成功、造UNKNOWN或继续无边界审计。

### 22.5 用户允许复用COM11：同板临时空根的最小接续

用户说明暂无另一块ESP32，并允许按测试需要使用COM11。**这解除“必须等待另一块板”的硬件前置，不等于删除旧Cloud设备／UNKNOWN、复用旧scope，或修改共享正式Auth权益的授权。** 同板使用原默认NVS是一次明确的临时破坏性切换，不称作并行隔离；§22.1～4的“不默认清根”是此前预检边界，后续仅按本节批准范围推进。

最小顺序：

1. 先准备有效创建额度、现有App／c051／私有PoP及单次工具，再安排近期登录；非破坏前置未齐前不擦设备，不提前CREATE或reveal试探。
2. 只新鲜读取并ROM精确校验原`0x9000 + 0x5000`的20KiB NVS，核原instance／根／Key／任务；核103c恢复应用与现有程序匹配即可，不做8MiB全备份或SDK编译。原始字节仅存ACL私有目录。
3. 用新批次薄guard临时部署既有c051和FF的20KiB，核同COM11真实USB／MAC、原分区不变；不运行已消费的旧`flash-empty`，不改bootloader或其他板。原库自然生成新instance，实际GetInfo须核不同于原实例、epoch0、完整空根及supportsBootstrap，不注入空根标志或复制旧instance。
4. 仍按§22.3的一次首装原链，正常创建独立Cloud身份／allocation，保留原Admin／UNKNOWN及旧云设备。明确ACK之后WiFi机会只有一次；超时／UNKNOWN保留真实意图，不重放或自动重新创建。
5. 收尾先保留本批新身份的完整NVS及请求／CAS／UNKNOWN，再决定切回原身份或保留匹配测试状态。切回原20KiB／103c只是本地身份切换，不代表新Cloud scope已撤销或UNKNOWN已协调，不能用恢复覆盖其证据。旧零身份写的`restore-baseline／restore-rejected-wifi`保护不适用于这次首装，不伪造批准。只允许正常处理本批新测试身份，不删除旧设备挤额度。

额度已离线核清：旧本机`quota.cjs`指向旧Auth容器且有旧账号／数值guard，不能用于当前`api.yiyu.pro`；Gateway只认Auth权益快照，没有合法IoT侧旁路。正常Auth已具备管理员`entitlement:write`的`POST /api/v1/admin/entitlements/override`，可对单账号`iot.devices`设置带原因／到期时间的临时2台覆盖，沿原revision、审计及outbox；这不改Free／Pro套餐或Blockly其他权益。**须先取得正式Auth单账号限定权益变更的明确批准及真实管理权限，不直接写库或伪造管理员。** 没有该条件就停在清NVS之前，不把硬件测试授权外推到共享正式账号服务。

本次补充只做源码／既有工具合同审阅、USB／ADB元数据核对及文档更新，没有账号额度／设备存储／原生会话或服务修改，不重复GET、构建或已通过测试。首装仍NOT_RUN。下一项外部决定收窄为：是否允许按既有审计管理接口仅为当前账号临时增加1台设备额度；落实后再准备本次新鲜NVS保护与近期登录，继续一次有限首装。

## 23. ON-7b：同板首装准备、限定额度及实际认证门槛（2026-10-31）

### 23.1 批准范围与实际推进

用户已明确批准当前账号设备额度临时2台、验收后恢复；必要时可放宽10台。本批采用**2台**，不扩大到10、不改套餐或其他权益；COM11允许临时清根，但仅在原NVS保留、新instance／独立Cloud身份及近期认证齐备后执行一次首装。保留旧云设备、原Admin、scope及UNKNOWN，不重发配置、不发验证码、不升级SDK或提交推送。

| 前置 | 本批实际证据／状态 |
| --- | --- |
| 匹配App／固件 | 手机beta仍e6c9、现有c051及私有非空PoP可复用，没有新构建／安装或SDK变更。已准备新批次薄guard，不执行旧已消费intent。 |
| App原记录 | 新鲜before快照保留同账号、完整session、3项secure及完整task／automation journal／logcat，无APK备份。最终核对仍保持原记录；没有新allocation／Admin或身份写。 |
| COM11必要保护 | 只操作COM11，核真实USB／MAC、原103c应用及分区。完整20KiB NVS新鲜读取并ROM精确校验，SHA256仍`01664771…220a9`；原instance、epoch1／根／Key／Admin及Empty TaskStore与§21相同。原103c应用ROM比较通过，恢复其正常运行；没有清空NVS、写候选或启动新10分钟窗口，没有8MiB全备份。 |
| 合法额度管理 | 当前真实用户已有`admin:dashboard`及`entitlements:read/write`；Auth权限依赖明确兼容单复数，不伪造管理员。正常管理员接口GET前态200、无任何override，单次POST临时`iot.devices=2` 200，再GET精确核同新id／值／原因／期限200。写前完整权益／套餐及原覆盖留存，未直写数据库、运行旧local quota工具或改IoT快照。 |
| 权益真实生效 | 单次直接Auth `GET /api/v1/admin/entitlements/user/{uuid}`及`GET /api/v1/auth/me`均200，解析值**2／source=override**、revision**2**；套餐和其余全部权益与本批写前一致。临时覆盖到期时间为**2026-11-01 10:06:25（Asia/Shanghai）**，验收后仍应沿正常DELETE仅撤本批覆盖、保留审计；到期是兜底，不伪称已主动恢复。 |
| 实际近期认证 | 对本次当前token摘要对应的单个Redis key只读`GET／PTTL／TIME`，固定host key、实际Gateway环境一致；结果**MISSING／PTTL=-2**。未scan、写Redis、remember、reveal、refresh、登录或发码。这是缺少近期认证的实际证据，不再只是§22“未核实”，普通session未因此退出。 |

### 23.2 两个不同的门槛：权益缓存与近期认证

额度POST已提交且GET核2，但随后唯一Gateway `/auth/me`仍在38ms返回旧**1／revision1**，原apply工具因此停止，失败记录及已提交id完整保留；**没有重发POST或自动回滚**。之后仅新增上述两次不同来源的只读协调，确认正式Auth确实为2／revision2且他项不变，没有把旧Gateway快照误写成额度修改失败。

源码核对：Gateway `/auth/me → loadAilyIdentity`未强刷，AilyClient按token指纹缓存，默认TTL **300000ms**；HTTP no-store不绕该进程Map。当前独立Auth切换配置原本关闭权益事件消费，因此不能假定外部管理覆盖立即使Gateway缓存失效。这个差异与已有旧快照一致，**不改轮询／清缓存／重启服务或另加force接口**。文档及收尾准备期间自然超过原读后301000ms，才手动执行**一次**独立只读Gateway `/me`协调，结果**200／264ms、额度2／revision2**，套餐／其他权益仍同前。保存`gateway-coordination.json`，不覆盖旧apply失败、不重发POST或自动轮询；现在两层权益均已实际看见2台，但真实CREATE仍未执行，不能称其已验收。

权益缓存的5分钟与DeviceKey近期认证的5分钟不是一回事：等权益缓存到期**不会**建立近期认证marker。即便后续Gateway看见2台，当前MISSING仍禁止首装擦除／CREATE／reveal。不能用管理权限、等待、ready、修改配置或写标记替代真实primary login。

### 23.3 修正文档假设与唯一接续

当前实际`sections/login/login.html`只提供邮箱验证码及GitHub／微信入口，`login()`沿`loginWithEmailCode`；没有可操作的密码登录UI。旧`AuthService.login()`是legacy GET接口，不能当现成正式primary-login通道。**本方案之前多处“用已有密码重新登录”只是未经实际模板核对的前置假设，现由本节纠正，不据此新增密码登录功能或退出当前账号。** 也不能重复使用旧验证码、导回旧token或用refresh凑近期认证。

下一步收窄为一次真实同账号primary login：若存在该账号已绑定且可用的正常第三方登录，可由用户明确完成；否则需用户另行允许一次正常邮箱验证码登录。本轮“不发送验证码”继续生效，不代点发送、不自动重试、不为测试临时放宽安全门禁。**先保留当前登录，不擅自退出后才索要验证码。** 近期登录前先准备齐单次observer、当前网络、薄guard及独立测试名称；登录完成后另保留同账号新session基线，合法session更新与原授权／UNKNOWN保持分开核，不能为过旧快照断言恢复旧token或删除新记录。

条件齐备才沿§22.5的新批次guard操作：必要时新鲜核原NVS，临时FF20KiB＋c051、自然全新instance；普通添加/createNew及原CAS／Admin链，新owner核空根、精确Bootstrap ACK后仅一次WiFi，再分别记录官方回执及Cloud新会话／State。任何未确认意图单列，不以恢复原NVS宣称已协调；收尾保存新测试NVS／日志并按原批准处理本批新身份，旧设备／scope／UNKNOWN不动。首装后的新增allocation／Admin只能属于本次新instance，不能要求全部secure总hash不变后删除合法新增记录。

工具及证据分别在三端忽略目录`build/onboarding-first-install-sameboard-20261031`：App局部快照／完整日志、设备20KiB／ROM审计、Service正常权益请求／响应及精确marker观察。首次marker工具在**连接SSH前**因本地默认GBK读UTF-8失败，修正编码后仅执行一次实际远程只读观察；原错误／intent保留，不伪称第一次远程失败或重复采样。有限语法检查通过；没有产品源码／依赖改动、新部署、SDK升级或Git操作。

独立只读终审已重算before／final完整secure、journal及原生XML摘要一致，20KiB原字节及ROM日志匹配；正常额度POST恰1、DELETE0、active记录与本次id／值／期限一致，直接Auth及随后Gateway两层协调均2／revision2，套餐／其他权益保持。原临时覆盖未恢复、marker仍MISSING、首装NOT_RUN均按真实结果记录，不以工具准备、缓存到期或NVS保护冒称设备业务验收。

**本批停点：准备已推进，真实首装NOT_RUN，临时额度2台已留存并带到期保护。** 原COM11应用、NVS／授权／任务及App旧UNKNOWN保持，只有正式Auth的已批准单账号覆盖发生变更；尚未清根、创建新身份、Bootstrap、下发WiFi或发送验证码。继续需要正常近期认证，不能把本批资源准备写成首次接入验收通过。

## 24. ON-7c：授权的一次真实发码，等待用户验证码（2026-10-31）

用户已明确解除本次单次发码限制，唯一目标为原账号`121024123@qq.com`，只允许一次，不自动重发。先准备真实登录页薄工具、首装observer与板侧OTP证据分支，沿原App／WiFiProv，不新增产品代码、构建或SDK。当前原audit仍在30分钟内；若用户回复延迟超过期限，仅按新`renew-audit`保留原证据后最小新鲜核，不重复全套测试。

实际通过原路由进入`/login`，**没有logout**；前台原发送按钮可用。由原输入控件填写固定邮箱、原按钮触发ALTCHA，真实网络记录为**challenge GET 1、email/code POST 1、email/login 0**，HTTP及原响应状态均200。本次请求开始`1793413305972`，接受证据时间`1793413308582`；单次`wx`意图及完整请求／响应仅在App本批ACL私有`login-*`目录，摘要`send-result.json`。不获取邮箱密码、不读取验证码库、不用旧验证码、不直接fetch绕原链；若后续登录被拒绝／未知就停止，不能据此再发一次。

手机当前5GHz互联网连接仅用于Cloud，ESP32目标WiFi须用后续真实设备扫描确认，不照搬手机SSID；原COM11应用／NVS未改，没有开启首装窗口。现装e6c9／c051／PoP继续复用，旧账号、Admin、allocation／UNKNOWN及完整journal不清除。板侧仅新增本次真实单次OTP形状的有限guard，语法及14项纯内存合同核通过，不等于登录或硬件已验。

**当前唯一接续：等待用户提供本次验证码。** 收到后通过原登录控件仅提交一次，必须核原UserService完成同账号UUID／新session持久化，旧非session secure及journal保持；不能仅凭HTTP200造`sameAccount`、恢复旧token或填零发码证明。真实登录／发码证据齐备后，才生成短时ready并按§22.5／23完成一次COM11全新instance／独立身份首装。当前尚未登录、擦除、CREATE／Bootstrap／WiFi、部署或提交推送。

## 25. ON-7d：登录事实分离与同板空根候选就绪（2026-10-31）

### 25.1 原单次验证码登录与用户手动登录

用户提供验证码后，工具仅通过原App登录控件提交一次，真实`email/login` **400／AUTH_VERIFICATION_FAILED**；不重试、不重发。发码开始至实际登录请求约651秒，本地Auth验证码实现默认300秒，与过期相符，但实际拒绝未细分原因，不能断言线上唯一根因。旧响应／完整日志及单次意图保留，没有把失败HTTP写成成功登录证据。

用户随后明确“已手动重新登录”。原拒绝快照工具期间遇到合法session变化，旧session精确断言未通过，失败记录照留，不倒回旧token。改做独立手动登录只读基线：实际resolved同账号、新token不同于原token，旧非session secure、原Admin／UNKNOWN及全部journal逐字保持，匹配e6c9。用户手动登录的具体方式／额外发码次数没有被工具观察，**不冒称本批总OTP次数1或原失败login200**。

精确读取这个**新token**对应的Redis单键，真实结果`NUMERIC_NOT_EXPIRED`，观察时间`1793414224994`、数值deadline **`1793414397776`**、当时剩余172786ms。只读GET／PTTL／TIME，不设置标记；原Gateway实际fence／近期门禁仍生效。正常两次Cloud只读前置进一步核同账号、额度2／owner设备数1。用户手动确认＋实际新session／token摘要＋真实marker截止构成独立manual proof，不借用旧OTP成功证明，不虚构登录HTTP200。

### 25.2 COM11本次唯一空根写入

原audit超过30分钟后，仅补一次有限`renew-audit`：原分区／103c／20KiB读回核验、原根及任务保持，旧备份不覆盖。临时写入前又保留真正preempty原20KiB及摘要；当前marker与用户同板授权、quota2和新手动proof均通过原guard后，**仅写一次c051应用＋FF20KiB**，应用、空NVS及原分区ROM精确核验通过。

本次候选启动时间 **`1793414355323`**，原10分钟配网截止 **`1793414955323`**；不主动扩大窗口、不反复烧录。只读串口`j`确认SoftAP begun／待入网、network/cloud未就绪、powerWrites0，无reset网络请求。系统实际扫描自然新AP **`BLINKER_215AF06BEBE0`**，MAC属于本COM11 SoftAP、2412MHz；不是从原NVS复制的新身份。

当前完整32hex instance尚未读到，不能由12hex AP名补造。c051的串口j不输出full instance，故仅测试observer采用窄发现分支：自然AP前缀入场，首个原固定GetInfo实际24B、v2／op1／status0、flags2／epoch0、非零／非旧instance及前缀匹配后锁定full值，随后所有owner重入／Bootstrap只认该值，未锁定禁写。不读Flash重启候选，也不改产品SDK或身份生成。有限纯内存15项合同及派生语法核通过，非真机GetInfo／Bootstrap证据。

原配网目标已在系统扫描看到2412MHz的“带翅膀的蜗牛”，Windows匹配凭据私有可读；手机当前5GHz仅用于Cloud，不将手机频率误作设备可用证据。新runtime记录真实AP名、目标profile及窗口截止，不含推测full instance／PoP／密码。原20KiB、根／Key／Admin／任务及103c恢复目标均留ACL私有目录；它们此刻是**原身份恢复材料**，不是仍在运行的旧根。App原授权／UNKNOWN未删除，旧云设备／scope不变。

### 25.3 当前认证停点与直接接续

烧录／ROM核验耗时后，虽然固件已就绪，原手动登录的Key近期deadline已自然过去；该5分钟与设备10分钟窗口独立。没有为了抢窗口或探测权限进行CREATE／reveal，也没有GETInfo、Bootstrap或WiFi下发。已明确请用户在当前Cloud网络**再手动登录同一账号**；Root不额外发码，不使用旧验证码、不延长认证或写Redis，不以ready／等待解禁。

收到这次登录确认后，只核新的同账号session／原非session secure及journal保持、实际**当前新token**marker，再直接原页面首装，**不重复烧录**。必须给新的手动登录独立文件／单次意图，保留前次成功marker及失败验证码记录；不以覆盖旧proof伪装同一登录。若设备原10分钟窗口已结束，先报告该前提，不能静默重烧、reset或循环开窗。只要没有配置／身份写，就没有本批WiFi UNKNOWN可补造或重放。

三端本批忽略目录继续保留完整事件／请求／响应、20KiB原字节、ROM日志及新runtime；无服务／SDK或产品源码更新、无新APK、提交或推送。**当前只完成同板候选部署，首次接入业务仍NOT_RUN**：新Cloud身份0、Bootstrap0、WiFi0，原授权／UNKNOWN及完整日志保留。

### 25.4 再次登录报告后的实际核对与有界暂停

用户再次确认“已再次登录并保持手机解锁”后，新的只读快照实际resolved为原账号，但完整serialized session（含access／refresh token）与上次手动登录基线**逐字相同**。原旧非session secure／Admin／UNKNOWN及全部journal仍精确保持。快照工具因此停止，不能为过断言改token／删旧记录，也不能仅凭回到首页就签发新primary-login证明。

首次后续marker工具在密码等待处被主动取消，**尚未连接SSH**，未查询／修改服务器；随后独立一次当前token精确GET／PTTL／TIME得到 **MISSING／PTTL=-2**，观察时间`1793414898413`。这次结果证明本次内测包当前token没有有效近期标记，普通会话仍可保持；不能只根据token未变化推断用户没操作或账号错误。当前Auth源码的正常email登录会生成随机jti的新token，未发现复用旧token的正常实现；这是源码边界，不代替本次用户实际请求证据。

当前前台包名实际为`iot.diandeng.tech.beta`。既有本地CDP日志未覆盖用户随后手动操作，App无通用持久HTTP日志，原生logging关闭；不能事后补造其成功登录200。限定Nginx已有访问日志读取未获得可关联的新login记录，保留读取范围及“不能证明其他日志外无请求”的边界，不将空结果当用户操作错误或服务根因。离线另发现当前登录失败分支只隐藏loading、无明确错误反馈，**是需跟踪的体验缺口，不是已证的本次手动失败根因**；本批不另开UI／Auth构建部署。

原设备窗口在`1793414955323`自然结束；没有为了抢窗口调用CREATE／reveal或任何配置写入，也不把认证前置失效称为WiFi失败。只读`capture-new`精确保存候选当前完整20KiB、分区和c051匹配，实际新instance为 **`215af06bebe01744006b9348d9a5e40c`**，NVS SHA256 **`941571f5…6fef4ce3`**，无root／Key，TaskStore Empty。此full值来源于实际持久记录，不是从AP名补造；后续首个固定GetInfo仍必须验证相同完整新instance、空根／epoch0，不跳过WiFiProv身份核对。

capture后**COM11留在bootloader**，没有自动reset／开第二个窗口，原20KiB及103c恢复材料继续保留。新状态也保留，使下一批在有效认证就绪后可沿原`resume-new`显式恢复运行、只开一次新窗口，无需再清根／刷板；不据此声称原Cloud scope已撤、旧UNKNOWN已协调。当前没有新Cloud scope或新配置UNKNOWN可恢复／补造。完整最新logcat、原生安全存储、快照及服务只读诊断证据留在本批ACL私有目录，工具转发回收。

**唯一下一步：停止重复手动登录猜测，先在同一内测包完整观察一次真实正常登录及当前token的近期标记。** 若需要代发新验证码，必须用户另行明确允许一次；不沿旧已消费发码意图重发，不再次使用旧码、不退出清掉原授权。认证真实通过后先显式恢复已部署同一候选，再按一次首装原链完成新身份／Bootstrap／WiFi／Cloud确认。当前候选安全暂停、业务NOT_RUN，无新登录POST／验证码、SDK／服务变更或提交推送；临时2台覆盖仍按§23期限及验收后撤销合同保留。

### 25.5 新授权login-2：一次发码已接受，等待用户新码

用户另行明确允许本次仅向原邮箱发送一次验证码，并要求在当前内测包全程记录正常登录后显式恢复候选、不再刷板。新`login-2-directory.json`及`attempt-2-directory.json`分别记录认证与入网的独立ACL目录，旧失败／manual记录、已消费intent及原NVS不覆盖。新恢复guard先核真实本次登录response／同账号新session／当前token marker及quota，再核已捕获的新NVS、c051与分区，才显式hard-reset开启一次新窗口；不使用过期旧runtime时间，不伪造旧成功登录证明。

工具准备／语法核完成后，真实普通`/login`未logout，原发送按钮可用。根只通过原App控件执行一次：**challenge GET 1、email/code POST 1、email/login 0**，认证请求origin实际为 **`https://iot.yiyu.pro`**，HTTP及响应状态均200。请求开始`1793415927144`，接受摘要`1793415929455`，独立`send-result.json`及完整网络证据私有保存；未自动重发、直接fetch绕认证或读取验证码库。

**当前等待用户提供本次新验证码**，不消费之前失败的旧码，不重复执行已发送intent。收到后TTY非回显用于原登录控件一次，核真正新token／resolved UUID与旧非session secure及journal保持，并精确读当前token近期marker；本地至多8秒只读等待原会话持久化不是Auth重试。成功后才显式恢复COM11同一新instance215af06bebe01744006b9348d9a5e40c，原GetInfo／CAS／owner死亡／Bootstrap精确ACK后唯一WiFi合同不变。当前没有新登录POST、恢复设备、配置下发、刷板、SDK升级、部署或提交推送。

### 25.6 login-2真实通过，局部连接／扫描及安全收尾（完整首装未完成）

用户提供本次新验证码后，仅原登录控件一次`email/login`，实际origin为IoT Gateway，HTTP／响应状态**200**。正常持久化的新access／refresh token与这份真实响应逐字匹配，resolved UUID为原账号，旧non-session secure（含Admin及allocation／UNKNOWN）与全部journal同原。当前token精确marker读取真实`NUMERIC_NOT_EXPIRED`，当时剩余274524ms、deadline`1793416376049`；根采用更保守的按钮提交时间`1793416075246＋300000`作为操作截止。**本轮认证链已实证通过**，不能再沿§25.4写成未成功登录或Auth根因未定位。

新token正常只读前置核额度2／owner设备1，然后原`resume-new`先精确比对captured新NVS、c051和分区，再仅hard-reset一次，**write-flash为0**。真实启动`1793416125323`、本次10分钟截止`1793416725323`，新runtime消费该证明，instance仍`215af06bebe01744006b9348d9a5e40c`；未复制旧scope或修改窗口时间。

本批实际中断来自**测试工具准备**，不是SDK／设备／服务器缺陷：第一次observer未分配持久终端，stdin EOF结束，所有native／请求／业务计数0；显式纠正后原路由goto与SoftAP点击被同批提交，页面尚未挂载就触发控件不可用，仍native0，只有两笔图标GET。旧文件与intent保留，严格核零业务后才使用独立输出的显式纠正入口，没有重发任何设备配置。后续先实际DOM核可用、逐条操作，页面正常完成挂载。**这些纠正及根操作耗时耗尽了已验证的5分钟Key窗口，责任在验收工具／操作节奏，不能归因用户再次登录失败。** 没有通过延长TTL／写Redis或试探CREATE绕过已知过期。

| 实际执行 | 有限结果 |
| --- | --- |
| 系统网络 | 一次明确加入新AP，实际SSID及2412MHz核对；不是把系统命令提交当连接或隔离释放证明。 |
| 原Page局部会话 | purpose实际`blinker-first-install`，非空私有PoP。`createDevice`340ms、`connect`233ms，同一个真实owner；原权限／capability消费者不变。这里createDevice只是SDK局部对象，**不是创建Cloud设备**。 |
| 设备扫描 | 原`scanWifiNetworks`3999ms，页面实际15个SSID，含“带翅膀的蜗牛”目标2.4GHz；未填写／发送网络配置。 |
| 尚未执行 | 固定GetInfo **0**、Cloud CREATE／reveal／Admin写 **0**、Bootstrap **0**、WiFi下发 **0**、Cloud完成查询 **0**；full instance此前来源于实际NVS，不冒称Page GetInfo已验收。 |
| 回收 | 设备窗口到期后observer结束；根仅原Page离开，真实`clearState` **68ms** resolved，stopScan19ms、网络状态读取15ms，没有直接重试cleanup。手机随后真实恢复“带翅膀的蜗牛”、App首页、WiFiProv worker **0**，CDP转发回收。 |

两笔`account/connection`只读DNS失败发生于Local owner创建**之前**、手机已在AP阶段；保留网络证据但不称active Local owner并发违规，不计为身份写或配置重试。没有人为制造UNKNOWN，也没有消费旧UNKNOWN。

安全暂停先只读比对整20KiB与首次captured `941571f5…6fef4ce3`，ROM报告digest mismatch，guard立即停止，**未写回／恢复旧NVS**。随后在已留ROM、不reset／不开窗的条件下仅新鲜read＋verify20KiB，得到`fa4e4ac5…9a5cc11f0`；严格NVS CRC、完整instance及其持久record摘要、root=null、TaskStore Empty均与原captured一致。整区变化来源没有另做根因归因，不把其当配置UNKNOWN；本批SDK实际调用已确认没有身份／网络写。旧原NVS `01664771…220a9`、初次空根captured9415及新鲜fa4e全部保留。**下次恢复须采用最新实际fa4e快照，不可继续套旧9415整区guard或覆盖它以过校验。** COM11当前留bootloader，无第二次reset／新窗口／刷写。

末次真实快照确认：成功login-2 session、全部3项secure、原授权／UNKNOWN及所有journal与成功登录后基线精确一致；完整最新logcat、原生XML及请求／响应私有保留。独立离线审阅确认本轮认证通过、局部owner／扫描通过及未执行项，未再次采设备／服务。两个旧operator失败、原apply缓存差异及先前验证码失败不回滚／删除；所有产物继续Git忽略，没有产品源码、依赖、SDK、服务部署或提交推送。

**工具就绪修正已有限落地**：新增只在测试忽略目录的`prepare-page.mjs`，用原路由及MutationObserver等待真实Page和可用SoftAP控件（8秒截止、无固定sleep），只允许能力／状态及原生命周期cleanup；连接、扫描启动、身份HTTP及配置写全部阻止，不以UI ready解隔离。根已在同手机**实际运行一次116ms挂载通过**，仅`getCapabilities`，原session／授权／UNKNOWN逐字同前，没有连接或身份API；观察hook／转发回收后正常回首页。不修改旧已消费observer或源日志，不新建产品功能／SDK。

**当前最小接续**：下次发码前先完成页面预热和持久终端准备，确认真实控件就绪后才启动新的短认证窗口；逐条执行页面动作，不再将goto与下一控件点击盲批处理。下次仅连续完成`GetInfo → Cloud独立身份／CAS／Admin → 新owner同空根 → Bootstrap精确ACK → 唯一WiFi → 官方回执与Cloud新会话／State`，不再重复认证排查、烧录或连接性能矩阵。任何新的发码仍需明确授权，不能将本次允许1次外推为自动再发。当前登录与局部链有证据，**完整空根首装仍未关闭**，临时2台按原合同保持到期及验收后恢复。

### 25.7 一次认证前的执行入口已预检并冻结

按用户要求先备齐入口，本批只准备下一次受控尝试，不提前消耗近期认证。直接、分模块的测试薄入口复用现有App、WiFiProv和身份链，全部位于三端忽略目录；移除重复的未使用预热工具，没有产品源码、SDK、服务或固件变更。

实际整链冷预检在认证前发现本地`capturedFile`变量错误，已停止并保留原半成品意图；修正后仅显式补完该本地准备，不重复已通过的板侧预检。最新fa4e NVS、空根／空任务、原恢复材料及候选／分区的**本地文件**核对通过，不冒称本轮物理读回。真实手机预热140ms内得到Page／SoftAP控件，仅调用能力读取，未连接／扫描或写身份；正常登录页准备及预检通过，APK仍e6c9，同账号、旧授权／UNKNOWN及journal保持，无logout／发码。持久TTY探针实际通过，未消费业务意图。

15个必要运行文件已生成冻结摘要并实际复核一致；独立只读审阅确认文件名、证明字段和消费者匹配。唯一热入口`workflow-3.ps1 begin`必须使用持久TTY，顺序为：原控件一次登录 → 新session快照 → 当前token精确marker → 正常只读账号／额度核对 → 显式恢复原候选 → runtime → 原Page观察器。非零即停，无重试或跨阶段继续；发码、网络切换、收尾及只读新NVS捕获入口也已备齐。认证后不再改脚本、构建或启动其他调试器，避免竞争同一转发端口；不再运行已消费的整套prepare。

**当前停点：入口准备完成，尚未发新码或新登录，完整首装仍未关闭。** 本批没有打开板侧端口、reset／刷写、创建Cloud身份、GetInfo、Bootstrap或WiFi下发，也没有部署、SDK升级、提交推送。COM11继续留ROM，新旧NVS／授权／UNKNOWN／完整日志保留，临时2台仍按§23合同处理。下一步只需取得本次向原邮箱发送一次新码的明确授权，再沿冻结入口连续补齐§25.6所列剩余主链；不沿旧已消费授权自动再发。

## 26. ON-7e：冻结入口连续执行，SoftAP空根首次接入有限收口（2026-10-31）

用户明确允许本次原邮箱单次验证码并提供新码。仅通过原内测App控件，实际`challenge GET 1／code POST 1／login POST 1`，发码及登录HTTP／body均200，origin为原IoT Gateway；无logout、Auth直接旁路、refresh或自动重发。新session实际同账号持久化，旧3项secure及全部journal保持；当前token精确单键marker真实未过期。正常账号／设备只读前置确认额度2／已拥有1台后，沿已冻结单TTY入口连续执行，不再改脚本或构建。

板侧只核COM11、原分区、c051及最新fa4e实际字节后hard-reset一次，**write-flash为0**；真实启动`1793420884370`，10分钟截止`1793421484370`，未改窗口。Key操作使用更保守的原登录提交`1793420851202＋300000`截止，没有用ready或等待解除认证／owner隔离。

| 原链路 | 本次真实结果 |
| --- | --- |
| 首个Local owner | 原Security1／非空PoP首装用途，GetInfo为完整24B、v2/op1/status0、flags2／epoch0，完整新instance为`215af06bebe01744006b9348d9a5e40c`。原扫描得到实际目标2.4GHz；不是由AP前缀补造身份。 |
| Cloud交接与独立身份 | 首owner真实`clearState`84ms完成后，正常切互联网并显式继续；CREATE201、reveal200、Admin PUT201各**1次**，原allocation／CAS绑定独立新设备`device_b60437fc-dfe2-4745-91cc-57efbf6bf024`，不复用旧scope。 |
| 第二个Local owner | 原Page显式重连精确AP，owner与首个不同；再次GetInfo同完整instance／epoch0／空根。不是旧owner、ready快照或固定等待授权写入。 |
| 精确ACK与一次WiFi | 新owner的Bootstrap **1次**，返回精确`[2,3,0]` ACK，**72ms**；其后typed WiFi **1次／7556ms／success=true**，没有generic provision、任意custom、WiFi-only或配置写重试。真实`clearState`75ms完成。 |
| 官方回执与Cloud分开判断 | 官方明确WiFi成功。手机尚在Local网络时首次Cloud确认DNS不可用，页面保留“WiFi已确认、Cloud暂不可用”，不误报UNKNOWN／失败、不重发配置。切回互联网并确认实际网络可用后，仅显式“再次检查”1次，200／observation=online、同新instance；新会话签发`1793421033216`及新鲜State`1793421035170`均晚于WiFi成功`1793421027826`，Cloud观察`1793421072156`。 |
| 结果与导航 | 真实成功页显示“设备已上线，WiFi连接和云端上线均已确认”；显式“进入设备”**1次**，实际路由匹配新设备。因为经历暂不可用及人工检查，**不称本轮自动进入已测**，正常前台自动进入仍沿原已验结果冻结。 |

完整记录的resolve-instance共3笔，分别是首装前读取、Local DNS失败、显式最终确认，不是3次配置写。独立只读终审核CREATE／reveal／Admin、Bootstrap／ACK及WiFi各1，配置重发0；不能把SDK局部`createDevice`2次混作Cloud创建2台。

### 26.1 原记录保留、必要捕获与支持边界

原Page退出回首页，实际owner关闭已完成，WiFiProv worker **0**，手机恢复原互联网，CDP转发回收。终核同本次成功session，原3项secure逐字不变、journal深比较保持；只新增本次同账号／同新instance的allocation和Admin **2项**，没有为保持旧总hash而删除合法新授权。旧UNKNOWN未删除、未协调或重放；完整请求／响应、原生日志、成功页及设备页截图均在ACL私有忽略目录，未将凭据／验证码／网络口令写入方案文档。

沿冻结capture入口只读取并精确verify当前完整20KiB NVS，原分区／c051匹配，新SHA256为`73d94901ec0ff205f5ac172f70a6e086f7060b0f7eaa4ae4a57b282d71728a51`，同instance且root现已安装、TaskStore Empty。旧原016、941、fa4e及本次新字节全部保留；**不恢复旧NVS覆盖新授权，不改TaskStore，不清根、不解绑，未刷固件**。捕获后COM11留ROM，不自行再开窗口；此时的offline是受控收尾停机，不能倒写为入网失败。

**决定：§22.3所定义的一条真正新设备首次接入最小链，现已由显式手动SoftAP路径真实完成，可有限收口并冻结。** 不重复认证／烧录／同路径性能或断电循环。BLE已有根重配、手动及OEM已有根成功分别沿原结果；本批不扩称BLE空根／所有OEM组合、自然UNKNOWN持久恢复、长期在线或GNSS验收通过。

剩余仅独立处理：①保留本次匹配测试身份／授权／云scope，不删除旧设备挤额度；临时2台覆盖仍带§23到期保护，**尚未主动撤销**，按原正常接口收尾，不能称已恢复；②若发行范围要求BLE空根首装，先准备真正匹配非空PoP的独立BLE候选，再安排一次必要差异，不把当前无匹配PoP的103c恢复BIN当首装候选；③没有自然新版UNKNOWN记录则保留待测，不造记录。下次执行从这些具体剩余项选最小批次，不再沿§25历史停点重新发码或重跑SoftAP。无SDK升级、服务部署、产品源码变更、提交或推送。

## 27. ON-7f：BLE非空PoP空根首装候选与最小差异范围（2026-10-31）

### 27.1 本批目标与真实缺口

用户选择§26的BLE空根候选准备。本批先完成匹配固件及真实App入口，不发验证码、不提前清COM11当前根或创建第三个Cloud身份；SoftAP成功链冻结，不重跑。BLE仍为WiFiProv的配网传输，不转成纯BLE设备或Direct控制，不引入新身份协议。

源码核对发现**非空PoP的真实App接线缺口**：原PoP字段只在SoftAP模板分支；`selectBleDevice`只传id／security，`connectSession`也只为SoftAP带PoP。原插件的select／connect本来都支持PoP并调用原SDK设置，因此只造BLE固件或用测试脚本私调native不能完成真实产品接入。最小修正为共享现有PoP字段、同一字节校验和原样传参，复用原遮蔽／显示、错误提示、忙态及生命周期清空；保留未设置PoP的旧设备兼容，不持久化、日志输出或按设备公开标识推导PoP。插件／SDK不变，不新建第二套连接器。

### 27.2 冻结最小实施与验收范围

1. **固件候选**：复用§26的c051同源stage、公开Cloud profile、同一私有非空PoP及依赖，只显式选择BLE bearer。原ESP32 Core3.3.11、Security1、IDF NimBLE及任务0槽不变；无嵌入WiFi／Key，不改分区。新ACL忽略目录独立保存配置／摘要／日志，唯一队列jobs1完成定向编译和完整链接，实际尺寸必须落入原app0，不能改分区绕容量。
2. **App有限修正**：原页面在BLE选择前能填写同一PoP；选择和连接使用同一经校验值。invalid在native调用前拒绝，异步／离页不引入新的自动重试；空值旧设备仍按原合同。只补受影响选择／连接、原模板、错误及清空回归和类型核查，不重跑已过配网矩阵。
3. **BLE原执行链**：BLE不占用手机WiFi，保留原`sendCustomData(blinker-config)` GetInfo → 原allocation／CAS／Admin → 原generic Bootstrap精确ACK → 官方`WiFiProv.provision`一次 → 原Cloud确认／结果页。**不照搬SoftAP的typed首装接口、两owner切网或系统热点授权**；需要一个新鲜BLE会话和全过程同instance／账号／generation检查。
4. **真机一次短差异**：匹配App／固件／PoP及所有操作入口先就绪，再完成一次正常近期认证；只验证非空PoP BLE连接、自然新instance完整空根GetInfo、独立Cloud身份／CAS／Admin、精确Bootstrap ACK、一次WiFi、官方回执与本次新会话／新鲜State、结果及资源回收。明确回执拒绝／UNKNOWN分别保留，不为补样本故意失败、不自动重发。扫描仅为选中真实新设备所必需，不另开性能或断电循环。

### 27.3 真机写入前置与当前推进

COM11现在持有§26成功的新根及Cloud scope，最新完整20KiB为`73d94901…71728a51`，已经只读保存并精确verify，停ROM。直接换BLE应用**不是空根首装**，不能复用215实例／旧scope；也不把“准备候选”当作已执行清根。本批先保留当前应用、该NVS及全部原授权／UNKNOWN。需要同板切换时，必须先确认本次限定新fixture／恢复目标和合法创建额度，再按真实自然新instance执行，不能删除旧设备挤额度或覆盖未确认意图。

§26末次已知账号2台额度／2台已拥有，没有第三个合法创建空位；临时权益带原到期保护，不假定缓存或时间自动提供空位。创建前仅通过正常Auth权益合同核对、按既有明确授权作必要限定调整；不直写数据库、不复用旧身份、不在编译期间发送验证码或耗用近期认证。本批先并行准备唯一候选编译及App有限修正，实际结果取§27.4～5，尚无本批BLE真机首装结论。

### 27.4 App最小PoP修正及有限回归

已共享原`provisioningPoP`字段到BLE／SoftAP，不新增第二份状态、CSS或存储。BLE选择前复用原校验，局部不可变快照原样传`selectDevice`与`connectSession`；异步选择期间字段改变不改变已选连接的PoP，空值保持原无PoP调用形状，invalid在停止扫描／native选择之前拒绝。连接开始遮蔽，忙态禁用，离页沿原清除／取消，不新增重试。身份／Bootstrap／WiFi／Cloud完成链及插件不变。

本批使用UI技能仅约束可见标签、关联字段错误、秘密遮蔽及禁用语义，未做页面重新设计。模板只把原PoP块共享一次，BLE有简短按固件配置说明，SoftAP原手动必填／自动选填和原调用合同保留。

有限验证如实分阶段记录：默认测试入口解析`node_modules`旧插件声明，在业务断言前缺类型；没有改依赖或锁文件。改用已有相邻插件源码alias和本批定向tsconfig后，修前24项**8红／16绿**，红点对应BLE字段缺失／PoP漏传／invalid未阻断；最小修正后，同一匹配入口**24／24通过**，含新增BLE合同9项、新增真实模板2项及原模板13项。原Angular真实模板编译、匹配配置`tsc --noEmit`与改动文件`git diff --check`均通过，不把默认入口失败冒称产品业务失败。

本次必要源码仅Page TS／HTML、BLE PoP新spec及现有真实模板spec的有限增量，已有无关本地修改保持；没有插件／依赖／锁文件、账号／设备／云配置变更。**尚未构建或安装本次App**，现装e6c9仍不含这份PoP接线；未进行手机多尺寸／暗色视觉、实际BLE Security1或空根真机验收。后续必须匹配打包安装后再认证，不用CDP私调PoP绕真实UI，也不套旧SoftAP observer／已消费意图。

### 27.5 BLE候选原SDK完整链接通过

唯一CLI／jobs1队列正常exit0，约14分钟，未重启或另开队列。当前库437个src文件与已验c051基线实际一致，沿同一stage／PubSubClient2.8／公开profile／私有PoP，只改bearer宏及非秘密诊断标识；没有本批设备库产品源码或SDK修改。

| 项目 | 实际产物／消费者证据 |
| --- | --- |
| 目标 | XIAO ESP32S3，原Core **3.3.11**，`PartitionScheme=default_8MB`，原SDK无overlay／build_opt注入。 |
| 匹配BLE应用 | SHA256 **`1e6009013d55890892160a8c7380c6e7f04551c9e0fbb984ae12da8edb1d356f`**；BIN **1,372,240B**，编译flash1,372,084B，静态RAM57,228B，完整ELF／map生成。根终核重新计算BIN摘要与实际文件大小一致。 |
| 固定容量 | 编译app0 **3,342,336B @0x10000**，BIN可容纳；NVS **20KiB @0x9000**不变，未改分区。真实写入前仍须匹配物理板分区，编译CSV不代替实物准入。 |
| 实际链接 | ELF含`network_prov_scheme_ble`、无SoftAP scheme，原IDF NimBLE消费者存在；实际依赖命中PoP头及原产品／公开profile／验签头，未改TLS／P256 keyId1。 |
| PoP及任务边界 | 私有非空PoP头实际消费且与c051输入一致，不按公开标识推导；任务仍0槽且无TaskStore消费者，不扩持久任务。 |

准备、完整编译日志、ELF／map／消费者审计及`candidate.json`保留于库忽略目录`build/onboarding-ble-first-install-20261031`的新ACL私有子目录。**含该测试PoP的头文件、BIN和产物只用于本批受控验收，不进入公开ZIP、发行CI或Git**。原SDK Security1配置和真实PoP消费者已静态核对，不冒称实际Security1会话、空根、GATT或Cloud已真机通过。

### 27.6 当前收束与唯一下一批

本批完成**匹配BLE候选＋真实App PoP接线修正＋有限回归**，独立只读终审未发现新阻断；原插件实际返回`transport=ble`，新传参分支可命中，SoftAP原owner／用途／写入边界不变。没有ADB／串口／刷板／NVS写入、账号或额度变更、验证码／Cloud请求、服务部署、SDK升级、提交推送，§26完整日志和新旧授权保持。

**下一批先匹配打包并安装一次内测App，完成BLE首装入口的冷预检，再安排近期认证。** 复用既有构建、快照、安装、只读核对及身份协调脚本；新的BLE观察只配置原generic custom／provision路径，不复制SoftAP专用typed工具。保留此时全部secure／journal（包括§26新增allocation／Admin），不是只保留更早3项；先准备合法创建空位和COM11限定新fixture／原73d949状态的保留与恢复合同，不复用215 scope、不删除旧设备挤额度、不在认证后再修工具。所有入口／候选／权限齐备后才允许一次新认证与一条必要BLE首装短差异；没有新发码授权不发送，任一写入UNKNOWN停止并留原记录、不自动重发。

## 28. ON-7g：匹配App安装、合法额度与BLE首装入口冷预检收口（2026-10-31）

### 28.1 实际匹配安装与账号保留

先在原beta保留当前正常session、全部**5项secure**、完整IndexedDB journal／请求日志和原生logcat；其中仅1项是session，其余4项包含旧授权和§26新增allocation／Admin。原e6c9 APK仅保留一份必要恢复副本，未全量备份App或设备Flash。未logout、refresh、清应用数据或触发验证码。

复用既有cloud-beta预构建、Angular相邻插件源码alias、Capacitor同步及原Gradle定向命令，**唯一队列**完成prebuild／web／sync／APK，全部exit0，无重试或SDK下载／升级。各阶段App源码、插件输入、HEAD／index、依赖锁均保持；没有新产品源码修改。必要产物验证包括实际aapt包名、apksigner原证书、HTTPS／cleartext=false、原生插件注册表、SDK2.4.4摘要、1156份打包资源与本次生成资源逐字匹配，以及真实共享PoP页面代码。

匹配APK SHA256为 **`49e68eed7afc051cbdd857f9cb3ec8f31d472e09d38044577808243f7e29f8e6`**。正常`adb install -r`一次成功，无清数据；装机摘要匹配，旧证书`a76639ec…a5d5d210`保持。安装后及本批最终两次必要快照均同账号／同session、全部5项secure逐字一致、journal深比较一致，旧UNKNOWN不删不重放。完整安装、请求／原生日志及原生安全存储XML只留ACL私有忽略目录，不入Git。

### 28.2 合法创建额度：只补必要的第三台

用户此前已允许测试所需可增加至10台，本次要求备齐合法额度；本批按最小范围仅用原正式Auth审计接口将目标账号`iot.devices`由**2更新为3**，不扩至10、不改Free套餐或Blockly／其他权益。先使用本批真实保留的当前token，正常3笔GET确认唯一原活跃row／原到期、直接Auth额度2／revision2、现有设备2台。

根独立核对前态后，正常POST **恰1次**，同一个原override row原地更新至3，原到期仍为 **2026-11-01 10:06:25（Asia/Shanghai）**；没有DELETE旧2、不延长期限、直写数据库或修改IoT快照。真实POST／前后GET及直接Auth me均200，直接Auth解析3／revision3，套餐、其余权益和其余override逐字保持。正常API的事务revision／outbox／审计合同已源码核对，**本批未另查询远端审计表，不冒称运行审计行采样通过**。

安装与工具准备期间自然超过原Gateway缓存期后，根仅一次正常读回，Gateway已见**3台／matched=true**，未轮询、清缓存、重发POST或重建服务；当前仍只拥有2台，尚未CREATE第三台。原§23只认2及旧reason的restore应继续拒绝，不能强改旧guard：后续若回2需按正常POST恢复保留的原前态，最终回套餐默认才正常DELETE本批覆盖，均不代表删除设备／scope／UNKNOWN。额度仍带上述原到期保护，**尚未主动恢复**。

### 28.3 真实冷UI及执行合同

匹配安装后仅运行一次BLE冷UI：实际APK核对通过，原配网页挂载、BLE模式、单一PoP字段／可见标签／默认遮蔽及扫描按钮可用；原Page离开回首页。原生只出现`getCapabilities／stopBleScan／clearState`，**没有开始扫描、选择、连接、身份设备API或认证请求**；全部5项secure／journal前后相同，CDP转发回收。DOM ready只证明入口展示，不解除任何owner／账号／近期门禁，不是真实Security1或首装通过。

原登录页随后真实prepare／preflight通过，未logout，发送／登录控件可用、已装49e匹配、全部原授权及journal保持，**发码0／登录0**。单TTY控制层探针通过，无意图或runtime消费；原BLE观察器TTY探针也已通过，不重复探针或冷UI。

| 入口／合同 | 本批状态及边界 |
| --- | --- |
| 匹配候选 | 沿§27的1e600901 BLE BIN、原SDK3.3.11、私有非空PoP、任务0、固定分区，无新编译或刷写。 |
| fixture冷合同 | 本地既有文件／严格NVS CRC及摘要核通过，27项纯合同通过；恢复目标明确为c051＋**73d949完整20KiB**及215原成功身份，不套旧016／941／fa4或103c。初次本地reader错取signer schema在任何设备操作前停止，已改读权威manifest并保留说明。 |
| 物理执行薄口 | 统一七份hash引用核实际新APK／安装／全记录、新primary／token、精确marker、原Gateway额度与原2台身份及独立明确批准；41项纯合同通过。本批**未运行**prepare-fixture／resume／capture／restore，未打开串口。未来只写指定BLE app＋FF20KiB并精确verify，真实一次启动／10分钟窗口；不能用冷布尔或等待解锁。 |
| BLE原UI观察器 | 最终23项纯合同、语法及7项本地入口冻结核通过；复用单一物理ready校验，不另造Auth合同，不用SoftAP typed／两owner切网。真实UI scan后显式选唯一设备，完整新instance只能首个固定GetInfo实读，先拒旧6d／215再允许原Cloud／CAS／Admin、精确Bootstrap ACK及官方WiFi各1。 |
| 认证／只读额度入口 | 仅本地7正向／20拒绝及表达式／Python内外AST核通过，Root另实际运行无发码的页面prepare／preflight；未来仅新明确单次批准才能发码，TTY新码不入argv／stdout。当前token精确marker单键只读及登录后同新token Gateway me/devices入口已备，不在本批执行。 |
| 只读收尾与恢复 | 17项纯保留合同及语法通过，未运行live收尾。原成功session／全部旧记录保留，只允许真实新instance的allocation／Admin最多2条；缺真实instance或响应正文缺失不得签capture许可。先保留新NVS／CAS／scope／UNKNOWN和完整日志，再按独立批准精确恢复c051＋73d949；恢复不撤销新Cloud scope或协调UNKNOWN。 |

**身份不预造**：物理FF核验和真实boot窗口与首个GetInfo后的完整instance绑定分开；不从广播12hex补造32hex，也不为了拿instance额外进ROM或反复开窗。所有浏览器操作经原可见控件，不私调Page或native写接口绕过PoP产品入口。

### 28.4 当前停点与唯一连续执行顺序

Root已备齐单一`workflow.ps1`及真实证据装配入口，独立只读核对七refs、consumer字段、CLI参数、window／runtime及capture／restore合同一致。**49项运行文件与必要静态证据已冻结并实际复核一致**；这只是准备证明，不是近期认证或物理写许可。代码均在忽略目录，不改旧已消费工具、冻结文件／日志或SDK；本批未提交推送。

唯一后续顺序：新的单次发码及COM11限定空根fixture执行须明确批准 → 原App一次正常登录／新session → 精确当前token marker → 同新token正常Gateway额度／原2台核对 → 七refs真实ready → 指定COM11物理身份／原分区／c051／73d949核对 → 一次BLE新fixture／真实窗口 → 原Page首装 → 只读留存及批准后的精确恢复。认证后不改脚本、不构建、不查控件、不另开CDP；非零立即停，无业务重发。发生新的UNKNOWN保留真实意图，不复用旧scope或旧验证码。

**本批已完成匹配安装、合法额度及全部入口冷准备；BLE首装尚未运行。** COM11仍是§26的原成功根／NVS、留ROM，本批串口0／清根0／固件写0／发码0／新登录0／CREATE0／Bootstrap0／WiFi0；只发生已批准的App更新和正常Auth限定额度更新，没有服务部署或SDK升级。下一步等待本次单次认证及限定fixture的明确执行授权，不能将本地seal或旧OTP授权当作已获新许可。

## 29. ON-7h：已授权单次发码，唯一登录被拒后停在设备操作前（2026-10-31）

用户明确允许本次原邮箱发码一次、认证后COM11限定新fixture及结束恢复原c051／73d949。调用前发现§28的登录页预检已过去约98分钟，超过原30分钟时效；先保留旧preflight三份证据及整体seal到ACL归档，再用**不变的原入口**实际只读新鲜核对。没有改时间、源码、baseline或prepare记录；新旧49项seal只有真实`preflight-result.json`一项摘要变化，其余48项一致。该刷新仅用于过期读取前置，不重放任何认证或设备写。

原App正常ALTCHA／发码链实际HTTP／body200，`challenge1／code POST1／login0`，origin为`https://iot.yiyu.pro`，提交`1793430353233`。单TTY已就绪，用户提供新码后只提交一次原登录按钮，提交`1793430907903`；真实HTTP／body **400**，顶层`errorCode=AUTH_VERIFICATION_FAILED`，未生成成功登录证明。两次提交相隔**554670ms（9分14.67秒）**，与已核默认5分钟验证码过期相符，但拒绝未细分过期／错误，不能断言线上唯一原因或改Auth时限绕过。

原workflow按非零立即停止，未运行snapshot成功分支、marker／新token quota、ready／fixture／窗口或BLE observer；**发码1／登录1，重发0，COM11端口／清根／固件写／开窗／CREATE／Bootstrap／WiFi均0**。没有为了执行用户“结束恢复”而无意义地再写原设备，因本轮尚未改动它。当前匹配App和BLE候选继续保留，不重跑SoftAP、构建或无线矩阵。

失败后仅独立只读保留当前快照、完整原生logcat／XML与原请求响应：同账号／原session未变、全部5项secure逐字同login-before，journal深比较保持，旧授权／UNKNOWN未删除或重放。CDP回收，原49项冻结输入复核保持；未修改冻结运行脚本。当前正常权益仍沿§28的3台／原到期保护，不修改套餐、服务或SDK，无提交推送。

**下一步需要新的明确单次发码授权，当前授权次数已消费，不能自动再发或复用本次码。** 保留这次拒绝证据与意图，下一次只在用户准备好接收并及时提供新码后另建明确的新认证尝试；原冻结源码／候选可复用，不把本次400改成成功，不延长近期门禁或清根抢窗口。BLE空根首装仍未验收，不能将“已发送”“窗口已准备”写成设备成功或失败。

## 30. ON-7i：BLE非空PoP空根首次接入及精确恢复有限收口（2026-10-31）

### 30.1 新授权与一次真实成功认证

用户另行允许再发一次，收到新码后明确沿§28连续完成首装与恢复。先保留§29拒绝的整个auth目录／原指针、已消费单次授权／执行计划及seal，使用**原不变源码**新建独立认证目录；不重用旧验证码或消费旧意图。新旧49项中全部运行源码摘要一致，只有本次新认证的真实元数据／路径变化，没有重建App、固件或修改SDK。旧拒绝及只读保留记录不覆盖。

本次正常原App链发码提交`1793431304754`，HTTP／body200；新码只经非回显TTY与原登录控件提交一次，提交`1793431345306`，HTTP／body200，间隔约**40.55秒**。**本次尝试**code POST1／login POST1，之前已明确授权的失败次数照留，不把全历史写成仅一次。实际同账号新session正常持久化，原4项non-session secure／全部journal保持，未logout、refresh或导回旧token。当前新token单键只读marker真实未过期，同token原Gateway me／devices确认额度3／原设备2；真实七refs才允许后续物理写。

### 30.2 COM11唯一新fixture与原BLE身份链

原物理入口实际核COM11 USB／MAC、原分区、c051及73d949完整20KiB后，仅一次命令写匹配BLE应用和FF20KiB，各项精确verify，再显式启动一次。真实`bootAt=1793431425156`、原10分钟截止`1793432025156`，不扩大窗口；不预造新instance，不复用215／6d身份或旧Cloud scope。旧固件／NVS／授权和既有UNKNOWN完整保留。

| 必要差异 | 本次真实结果 |
| --- | --- |
| PoP与安全连接 | 原页面共享PoP先填后扫描，实际BLE select／connect各1，匹配非空PoP均传递，原Security1会话连接约**1602ms**；没有SoftAP切网、独立进程或typed首装接口。扫描只用于本次真实选择，不重做扫描性能矩阵。 |
| 真实空根 | 唯一原generic GetInfo完整24B、v2/op1/status0、flags2／epoch0；实际新instance **`884c8c46bdf8ddd9b16e1367359b38a2`**，不同于原两实例，并与真实BLE名称前缀一致，不由名称补造。 |
| 独立身份／CAS | 新设备 **`device_fb7bb81d-a43d-42a3-85bc-3b8e612e3a15`**；原CREATE201、Key reveal200、Admin PUT201各**1次**，原allocation／CAS及pending→active Admin链完整，没有旧scope复用、清授权或直接数据库操作。 |
| 配置写与官方回执 | 原generic Bootstrap **1次**，精确`[2,3,0]` ACK约**158ms**；其后原官方`provision` **1次／7564ms／success=true**，没有typed SoftAP、任意custom、Key-only或自动重发。 |
| Cloud实际可用 | 原计划3次unconfirmed后第4次200／online；同完整新instance，新会话签发 **`1793431538060`**、新鲜State **`1793431540207`** 均晚于唯一WiFi成功 **`1793431532760`**，观察 **`1793431543061`**。加上首装前读取，resolve共5笔，不是配置写重发。 |
| 结果、导航与回收 | 正常前台确认后原路由**自动进入一次**新设备页，人工check／open均0；截图可见云端已连接。原`clearState`约**20ms**实际完成；quit走原Page返回首页，连接释放、hook／CDP回收，不直接重试cleanup。 |

### 30.3 完整日志的bodyless补证，不改写原false记录

原只读finish核原5项secure及成功session逐字保持、journal深比较保持，仅新增当前instance的allocation／Admin2项。但其初始`completeRequestLogsPreserved=false／capturePermitted=false`，原因是16个`responseUnavailable`，因此当时没有执行恢复或改写该记录。

独立只读分类证明：这16笔**全部是OPTIONS、204、type=Preflight、initiator=preflight、encodedDataLength=0**，本来无响应正文；原工具对不存在正文调用CDP而报错，不是身份／业务回执丢失。已完成的**24笔非OPTIONS设备业务响应24／24正文同requestId真实留存**，实际应有正文缺口0，包含CREATE／reveal／Admin及Cloud确认；原request／response／headers事件也完整。仅另加读取现有文件的分类补证，保留原false及失败记录、无伪造正文／新请求／重采样／冻结源码修改。

根据严格补证重新做独立capture批准，仍使用原物理入口。首次批准的Windows路径混合分隔符在`sameRef`前置被拒，尚无capture意图或端口操作；该错误及批准原样保留，另建canonical真实路径批准后才实际捕获。未放宽路径比较、清理／授权门禁，未重发任何设备业务请求。此处是验收工具语义／元数据问题，不作为BLE／SDK失败，也不触发产品链重跑。

### 30.4 新状态留存与原状态精确恢复

真实capture读取并verify本次完整20KiB，新NVS SHA256 **`c93354dca6a759121932afb13171a70d17abce283ec6d323136885dadf3b9a10`**，同实际新instance，root已安装、TaskStore Empty；新CAS／Cloud scope／UNKNOWN及请求日志先保留，捕获后留ROM，不再开窗。

之后沿原restore入口，先核当前BLE应用及最新新NVS字节，按独立批准仅恢复原c051应用＋73d949完整20KiB；原分区不写，仅核对。应用、NVS及分区实际ROM日志全部`digest matched`，恢复结果`1793432027320`，**oldProtectedStateExact=true**；默认按合同留bootloader，与本次写入前的停机状态一致，不追加业务重启／断电循环。

恢复后再次只读终核：同成功账号／session，当前全部**7项secure**（原5＋新增2）和journal与业务收尾逐字保持，保存新原生XML／完整logcat；新旧授权／UNKNOWN不删不重放，未正常移除新Cloud设备，也不声称本地恢复撤销了新scope或协调UNKNOWN。CDP转发回收，49项冻结运行输入再次实际核一致；App继续保留49e新版。当前临时额度仍3及§23原到期保护，**未主动恢复额度或改套餐**。

工具`workflow plan`仍只是冻结时的静态准备说明，不读取最终ledger，其中`NoCodeSent／NoFixtureExecuted`不得作为运行后事实；本批真实次数及恢复状态只取本节与原始auth／native／物理证明。此处不修改冻结脚本或重跑链路。

**收口决定：本批BLE非空PoP空根首装成功链与§26手动SoftAP空根成功链均真实有限通过，可冻结，不再默认发码／清根／配网循环。** 只限该匹配Android App、COM11／候选／PoP和既有Cloud；不扩称全OEM、所有错误组合或自然UNKNOWN冷恢复正样本已验。当前任务未升级SDK、部署服务、提交或推送，所有无关本地修改保持；后续只处理明确的新缺陷、必要交付或临时权益正常收尾，不拿已通过链路重复凑样本。
