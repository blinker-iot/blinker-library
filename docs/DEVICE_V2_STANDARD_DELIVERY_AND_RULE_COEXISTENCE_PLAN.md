# Device V2：标准接入交付与自动化共存收口

## 1. 当前主线与顺序（2026-10-31）

后续顺序已按用户新测试清单转至[测试问题对照与基础能力渐进计划](DEVICE_V2_TEST_REPORT_RECONCILIATION_AND_CORE_PLAN.md)。本文件§10／§11结果冻结；以下“现行／本轮／下一步”保留当批含义，不再次启动已过配网或自动化矩阵。新计划本批仅分析与文档整理，不自动部署或提交推送。

**现行执行取§11：§10真机有限结果冻结，仅补派生单位小修、交付闭包及现行文档入口，不部署或提交推送。** §10的R4当前正式Auth新身份、两设备同目标开／关规则同时启用、真实ACK／新鲜State、停用／退役及后台回收均已有限通过，§6.2收口。保留匹配App／Gateway／原Broker、R4普通WiFi零槽候选及原COM11身份；设备额度10在测试期保留，自动化规则仍4／16。§1～10保留其当时证据，不再默认重做登录／配网／已过矩阵。

BLE／SoftAP的匹配空根首装已分别按[全流程入网§26、§30](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md#30-on-7ible非空pop空根首次接入及精确恢复有限收口2026-10-31)有限通过，证据冻结，不再默认发码、清根或重复配网。本轮只完成下列最小源码收口，不升级SDK，不部署、刷板、提交或推送，不重启发行CI。

| 顺序 | 范围 | 完成条件 |
| --- | --- | --- |
| P1 | 标准Arduino示例与配置交付 | 沿用一个WiFiBLE示例，区分BLE／SoftAP及PoP／热点密码／DeviceKey；私有配置不能进入发行ZIP；有限回归通过。 |
| P2 | 同目标设备的最小自动化共存 | 只开放可证明不矛盾的规则组合，沿用原CAS、锁、观察、执行及UNKNOWN协调；契约、worker和真实PostgreSQL并发／迁移回归通过。 |
| P3 | 类型化默认名称 | 布尔power显示“电源开关”，数值power保留“功率”；显式标题及匹配别名优先，受影响消费者回归通过。 |

P3是独立显示小修，可以与P2并行；不改变协议、Manifest、保存接口或页面布局。优先完成核心可用性，不扩展复杂规则、任务槽位、Wio、OTA或Bulk。

## 2. P1：标准接入与发布边界

复用现有`WiFiBleQuickStart`与`Esp32WiFiProvOptions`，不另造示例或配置框架。公开说明模式选择、PoP用途及私有头的显式引入；没有配置PoP的现有Sketch行为保持不变。发行器只对约定的`PrivateProvisioningConfig.h`做大小写不敏感拒绝，不尝试删除用户文件或通过通用字符串猜测秘密。

**公开cloud-beta仍仅支持ESP32手动WiFi／TLS／0任务槽。** 私有配网候选及匹配P256／非空PoP不等于新增公开Cloud WiFiBLE支持；不发布私有配置、密钥或测试BIN。

已完成：标准接入新增8项、受影响包边界3项，合计11项有限回归通过。示例只改说明，未改变编译接线，不重复已通过的SDK编译或真机配网。发行CI仍保持原HOLD。

## 3. P2：最小共存合同

### 3.1 定义准入

以同租户完整的未退休规则集合为依据，在原用户事务锁内校验，不引入客户端判定器：

- 不同目标属性（设备ID／key不同）可以共存。
- 同一目标属性须匹配目标fingerprint及valueType；相同目标值可以共存。
- 不同目标值仅在源设备ID／key／fingerprint／valueType全部一致，并且触发域可证明互斥时共存：布尔上升沿与下降沿，或数值`above(A)`和`below(B)`且严格`B < A`。阈值边界为包含，回差仅决定重新布防，不能据此宣称触发域互斥。
- A1旧上升沿按原条件归一化。不同源、相同方向、相交数值区间或不匹配的能力继续返回原`AUTOMATION_TARGET_CONFLICT`。

原自环／连锁禁用、4条租户／16条试点上限、能力和所有权验证、CAS及撤权fence不变。App只修正冲突文案，不复制服务器规则引擎。

### 3.2 单笔执行与背压

只移除“一个目标设备只能启用一条规则”的数据库唯一约束，改为非唯一查询索引；用新增058迁移，不改已发行044迁移。**原目标设备级pending唯一约束及prepared／dispatching／outcome_unknown三态保持不变。** 不新增发送队列或并行设备写入。

只有consume事务明确返回`AUTOMATION_TARGET_PENDING`、当前watch有效且未取消／停止时，保留同watch、不ACK；原worker下一节拍重新读取原有序未ACK观察。cursor／冷却／occurrence与执行意图仍原子提交。原5秒新鲜度、4条有界缓冲、溢出／过期重建基线保持，不承诺跨过期补执行、持久FIFO或“最后一个值必定执行”。UNKNOWN继续阻塞，不能因修改、停用规则而重发或绕过。

其它错误、失效epoch、超时、取消、撤权沿用原释放逻辑；不降低轮询间隔。ACK丢失只协调同epoch已提交cursor，不重发设备命令。

终审发现并修正的反序边界：两条规则独立watch可能先提交较新Off、再处理较旧On，单有设备级在途锁仍不足。现在复用原consume事务／owner锁与execution ledger，对同租户、完整目标属性及完整源／credential／session／connection域查询较新或等stateRevision的相反意图。存在时只消费观察cursor／raw／latched，返回`suppressed/superseded`，不新建执行、不增加occurrence或冷却。所有已提交意图状态（含取消／过期／拒绝）均计入，取消不能倒退事件水位；原7天保留期大于5秒观察寿命，不增加新表／队列。UNKNOWN／pending仍先于抑制整行回滚，不跨会话作比较，也不承诺撤回已投递命令或全局latest-wins。

### 3.3 最小验证

先红绿验证上述互斥边界、A1兼容及原图约束，再验证worker仅对精确背压保留watch／不ACK。真实PostgreSQL需覆盖并发启用（互斥两条都成功，冲突只允许一条）、同设备单笔执行、背压cursor不推进、ACK后重评估、UNKNOWN继续阻塞，以及057→058与重复初始化。数据库测试使用独立临时库，不改业务schema、账号、权限或服务配置；skip不计通过。

## 4. P3：名称合同与兼容边界

一个共享解析器按“显式控件标题 → 匹配能力别名 → 类型化默认名称 → 原key”取名。布尔power默认“电源开关”，数值power默认“功率”，未知类型保留power；新自动布局不把默认标题固化为用户标题，后续别名可自然生效。

存量`title=功率`无法可靠区分旧自动生成与用户显式设置，**不猜测迁移、不覆盖用户数据**。用户可以通过现有编辑清除标题恢复动态默认，或显式改名。名称之外发现的布尔power单位W推断另留后续小缺口，不在本批扩展单位合同。

已完成：9个受影响spec共217个唯一测试通过，App类型核通过；无APK、真机或视觉验收。UI技能仅用于术语一致性与别名优先级，没有重设计布局。

## 5. 记录与后续边界

当前P1、P2、P3软件及本节所列数据库差异均有限完成，最终结论取§6。既有账号、授权、UNKNOWN、请求日志和无关本地修改均保留。本批不触碰原配网冻结脚本或历史测试记录，不从静态workflow计划推导运行结果。

发布／部署与一次同目标共存真机短验如需安排，应另按匹配版本执行；本批单元／真实数据库通过不代替已部署或产品真机通过。自然UNKNOWN冷恢复正样本、临时权益正常收尾仍独立保留，不为补样本制造新记录。

## 6. 本批有限收口与唯一下一步

### 6.1 实施与证据

| 项目 | 实际结果与边界 |
| --- | --- |
| P1标准交付 | README／原示例说明及发行器私有配置保护完成；11项通过。只改示例注释，不重跑原SDK或配网；公开cloud-beta边界不变。 |
| P2纯合同／worker | 先红后绿，87项通过；反序查询新增2项通过。新058只替换enabled索引，原044、pending三态唯一索引及轮询节拍不改。 |
| P2真实PG | 云端原PostgreSQL16.15内创建专用临时库；隔离schema／完整迁移后7个子用例通过（Node报告含父测试8／8、0skip），约4.2秒。不是在业务库执行迁移，也没有部署服务。 |
| P3名称 | 原9个受影响spec共217个唯一测试通过，App类型核通过；终审额外保守修正只跑3条差异，红2失败／1通过，绿3通过。共享helper保留任意非空显式title，仅控制页沿用原raw-key默认标题合同；不把此合同推广到所有控件／预览。 |
| App冲突文案 | 同一条原用例先红后绿：从“目标设备已有规则”改为“此目标属性存在可能同时触发的不同动作，请调整触发条件或目标。”只改文案，不增加客户端引擎；其余29条过滤不计本次新增通过。 |

7项PG覆盖：可证明互斥规则并发启用双成功／重叠相反动作一胜一拒及旧revision拒绝；同目标并发consume单一prepared与ACK后同watch重评估；UNKNOWN跨停用仍保留reservation且不重新claim；不同目标key仍设备级单笔在途；实际相反沿反序消费及UNKNOWN优先；正常顺序、等revision、完整身份／会话域隔离及七种历史状态水位；057→058注入失败原子回滚／升级／幂等及原UNKNOWN逐字段保持。

首轮本地经SSH逐query访问的完整旧suite受网络往返影响，240秒截止，若干用例未通过且没有完整完成，**不计验证通过、不据此放宽产品截止时间**。其临时库已清除。随后只在云端既有Node／依赖下的独立`/tmp`源码树运行上述新增差异，避免网络延迟污染事务／receipt预算；前两次环境准备在测试前中止的权限／复制问题也不计通过。最终唯一实际完成的窄PG队列全部通过。没有升级Node、SDK或安装新依赖，没有写业务表或改运行服务镜像／配置。

普通`ng test`最初引用相邻WiFiProv旧dist类型，止于编译，未运行用例；沿用既有源码alias候选tsconfig后才执行文案红绿。没有重建插件、改锁文件或升级依赖解决此环境差异。

### 6.2 唯一后续范围

本批源码／数据库差异冻结，**下一批仅审阅匹配版本后部署必要Gateway（含058）和App，沿用现有Broker及两设备做一次同目标开／关互斥规则短验**：编辑／启用两条、真实ACK／新鲜State、顺序变化、停用／删除及后台回收。若原设备不在线，仅处理该测试前置，不重做BLE／SoftAP首装、资源审计或已有完整矩阵；无自然UNKNOWN就不造记录补样本。

本批未安装APK、刷板、重认证、发验证码、调用设备业务API或提交推送；现有正式服务仍运行原版本，不声称共存已产品真机验收。源码、账号、授权、UNKNOWN及原完整日志保持；私有材料、临时测试archive及`/tmp`候选不作为发行产物。

## 7. 匹配部署、运行数量及两设备前置停点（2026-10-31）

用户明确批准按§6.2部署必要Gateway和内测App，沿用原Broker与设备，保留账号／授权／UNKNOWN／完整日志；不重复配网、不发验证码、不自动重发、不升级SDK、不提交推送。本节取代§6中的旧“暂不部署”，不修改旧证据。

### 7.1 保留与Gateway部署

App安装前新鲜只读保留全部7项SecureStorage、完整IndexedDB journal、原生安全存储XML／logcat和一份当前APK。当前正常session与原授权作为基线，不导回旧token／旧记录，不清应用数据。

云端先保留原Gateway镜像、实际compose／env、完整Gateway及Broker日志，以及**仅当前IoT数据库**的必要pg_dump（362121 B）；不连接或备份统一Auth业务库。私有材料只存本批ACL目录及服务器`/opt/blinker-automation-coexistence-20261031`，不入Git。

基于当时运行的Gateway镜像构建无网络delta，运行闭包仅六文件：`automation-contract`、`automation-repository`、`automation-worker`、新增`automation-execution-order`、migration registry及新增058。原tsc／实际入口完整import与268项运行文件逐字核对通过，保留其它已部署定位／配网等补丁，不安装或升级依赖。

**Gateway唯一一次重建**后健康200，镜像SHA256为`8b353f332618871a05287dca185acd6bc590df5fd3be6e794613bff5e5f5699a`。schema为1～58，原1～57迁移记录逐字保持；enabled_target非UNIQUE，prepared／dispatching／outcome_unknown的pending_target UNIQUE保持。实际compose只变Gateway image，原环境／挂载／端口保持；Broker／PostgreSQL／Redis原容器ID不变，均healthy、restart0。新版本保留运行，不自动回退058或为凑样本写数据库。

### 7.2 匹配内测App与只读收尾

复用原prebuild→production,cloud-beta→sync→offline assembleBeta，**一个队列、一次构建和一次覆盖安装**。原源码alias、WiFiProv SDK2.4.4、依赖锁和签名保持；原证书`a76639ec…a5d5d210`、beta包名、HTTPS、原生注册表与1157份APK／Web资产核对通过。四阶段源码／原生输入／HEAD／index未被构建改写。

已安装APK SHA256：`086d66c15137eba3b4e119ebee4a8f087c6fdcbf4c4920ea1dd98a2af8386aad`。安装后及终核同账号／同session、全部7项secure和完整journal逐字不变，旧UNKNOWN／allocation／Admin没有清除或重放；App继续保留新版，CDP转发全部回收。

真实只读自动化列表GET恰1，200／no-store／响应正文完整；实际页面空列表、无待确认／错误，新建按钮可用。初次观察器过早取Ionic控件可见性停止，后续只读确认原同页按钮已可用；继续同一页、只导航离开一次时又过早断言旧缓存页已隐藏。两次工具停止原样保留，不当作产品已修或已验。最终从已到达首页的自然状态继续，**没有重进自动化页／重发GET／重新构建**；实际旧页隐藏成立，HOME后台5秒、返回首页成立，期间自动化补查0、业务／认证写入0。只证明空列表的离页和首页前后台，不外推已启用规则、活动控制租约或两设备执行回收。

### 7.3 自动化数量：源码和实际运行一致

| 限制 | 当前实际值与语义 |
| --- | --- |
| 每租户可用规则 | **4条未退休定义**；启用、停用均计入，不是每台设备4条。正常删除（retire）后才腾名额，历史保留。 |
| 全库可用规则 | **16条未退休定义**，跨租户合计；原事务锁内检查，不是每租户16条，也不是SQL CHECK。 |
| 包含历史的定义存储 | 每租户64条、全库256条；不是可同时启用数量，不清历史绕过。 |
| 列表分页 | 每页16条；不代表当前用户额度16条。 |
| 发送资源 | 全局最多4笔pending；每目标设备1笔prepared／dispatching／UNKNOWN。不是规则创建数。 |

权威源码：`packages/contracts/src/automation-observation.ts`、Gateway的`automation-management-contract`／完整集合事务校验。App新建按钮与满额提示同步按4条；**没有Free／Pro权益映射或数量环境变量**。当前这是最小试点的固定边界，不是最终套餐化规模设计；本批仅核查，不擅自扩额或改轮询。

实际部署后的Node消费者已读回4／16、64／256及分页16；数据库全库未退休／启用／退休／execution／UNKNOWN均0。当前账号的正常API及对应tenant只读核对为**0／4条使用中**，不是已占满而不能创建。数据库UNKNOWN为0不等于App不存在旧未知意图；App旧UNKNOWN已独立逐条保留。

### 7.4 两设备短验尚未开始：准确停点

本次手机ADB正常、网络为“带翅膀的蜗牛”；USB／PnP仅识别COM11 ESP32，未识别R4／Wio。当前账号3条Cloud记录均离线，分别对应前面COM11配网验收的不同instance，session均已过期；这些不是两台同时可用的实物。没有把三个逻辑ID当成两个设备、没有复活旧scope或创建新身份／规则，也没有操作COM11串口、重置／刷板／配网。

因此下列项目明确**待验**：两条同目标开／关规则编辑及同时启用、源变化触发的真实目标ACK／新鲜State、规则停用／删除，以及存在活动设备链路时的后台回收。当前规则写入／设备控制／验证码／重发均0。先前纯回归／PG通过和本节部署／空态通过不能代替这些实物结果。

**下一步只补第二台同账号已授权、可上线设备的前置**，核对两台确为不同物理身份／匹配Cloud后，复用本节已安装App和运行Gateway／原Broker直接完成§6.2短验；不重复构建、部署、配网矩阵或资源审计。若第二台没有匹配该正式Auth环境的既有身份，应先明确接入范围，不擅自解绑、重用旧授权或发送验证码。此次无产品新修改／SDK升级／提交推送，全部无关本地修改保持。

## 8. R4接回后的身份核对与限定接入前置（2026-10-31）

用户回复“已接入”后，实际枚举到COM9 UNO R4（VID2341／PID1002、USB序列`F412FA6F2F88`），与此前测试板一致；COM11 XIAO ESP32S3仍在。该回复解决物理连接前置，不代表设备身份、运行程序或Cloud已匹配。

沿用§7的App `086d66c1…8386aad`，未构建／安装或重部署。新批ACL目录中先保留当前全部7项secure、完整journal与logcat；与§7终核逐字一致，同账号／同session。随后正常同token一次GET设备列表及一次GET当前自动化，均200、无写入：当前仍只有原3条COM11历史记录，均`cloudReachable=false`，未退休规则0；没有R4归属该账号的记录。

只读历史核对确认：COM9的D3c普通0槽TLS候选曾在`api-test.yysc.tech`旧隔离环境通过；修复方案§33切换`api.yiyu.pro`时采用全新IoT数据卷，明确未复制旧设备／Key／scope，COM9当时未动。不能把旧候选／旧Key移入新域或据此前成功声称当前上线。

本轮仅针对精确USB身份，在115200下做一次8秒／32KiB上限的被动串口读取，**写入串口字节0**，实际读取0B，不能据此判定程序故障或匹配既有fixture。没有发送`j`或其它未知程序指令，没有1200bps touch、reset、刷板、SWD、存储写入或备份循环。原工具含写入／恢复的分支没有运行。

**准确停点：硬件已接回，仍缺当前正式Auth环境内的R4测试身份／凭据。** 两规则共存、实际ACK／State、停用／删除及活动链路回收仍未开始；没有为凑样本创建设备／规则、重放UNKNOWN、发送验证码或提交推送。当前数量仍0／4，§7部署保持。

最小接续需明确允许：通过正常App为COM9建立该环境的独立R4测试身份，必要时只增加所需1台设备创建额度（不改套餐／其它权益，不删除原3台设备或旧UNKNOWN）；沿普通WiFi／TLS／0槽接入，不重跑BLE／SoftAP。取Key仍遵守正常近期认证门槛；先备齐入口再提示同账号近期登录，不抢先发验证码、导回旧token或绕过门禁。用户未明确这项范围前，不做新身份／凭据／固件写入。

## 9. 统一10台测试额度、R4冷准备与原ESP32上线（2026-10-31）

### 9.1 用户授权与正常额度接线

用户已明确“测试需要可以直接开通10个额度，避免反复询问”，承接§8必要R4新测试身份及设备创建前置。后续同范围必要构建／测试及这项额度不再逐次询问；授权不等于允许绕过近期认证、发送未获许可验证码、复制旧Auth身份或删除UNKNOWN。

本批沿正式Auth正常管理员API，先GET原覆盖／Auth me确认原`iot.devices=3`、revision3及原到期，完整保留基线；随后**同原override row正常POST恰1次**，仅设`iot.devices=10`、revision4。按用户此前“测试期间保留，全部完成再恢复”的授权，`expires_at=null`，**无自动到期**，而非仍保留明天旧到期；这是测试权益覆盖，不是改Free套餐。原3台额度／到期／reason均私有保留，全部测试结束后再经正常API统一恢复，不默默忘记该收尾。

Auth me及Gateway me各一次实际读回10／matched，套餐、其它权益与其它override逐字不变；未清缓存、轮询或重发，未登录／刷新／发送验证码，未直写数据库。源码确认revision／outbox／audit同事务合同，**本批未直接采样线上audit／outbox行**，不把源码合同当运行证据。完整请求／响应存服务端仓库忽略目录`build/automation-quota10-20261031`的ACL私有目录，不入Git。

**10台是设备创建额度，不是自动化规则数。** 自动化仍沿§7.3每租户4条未退休／全库16条，任务槽位不变；本批没有擅自扩大这两个资源合同。

### 9.2 R4唯一冷队列及真实App入口

复用原D3c普通R4 Cloud Sketch的`power(bool)`读写／onWrite→report及`j`观察，只在隔离私有sketchbook中薄适配。目标`iot.yiyu.pro:443`／公网TLS、原CA、0槽／空业务签名ring，原Core1.5.3、PubSubClient2.8／Crypto0.4.0不升级；不接Task／BLE／Direct或旧Auth Key。普通MQTT自动化Command／ACK复用现BBP2认证会话，不要求引入Task／Direct签名材料。

**唯一冷编译／完整链接已通过，226.292秒**，BIN145592 B、SHA256 `9f61735fce4521453b5fbb4f6f4e3f20c86ab46d750a689a21927456c3cb6c0e`；TLS Client／CloudSession／MQTT符号核对通过，Task／BLE／Direct／Gateway未链接。此时只用明确占位Key／SSID，证明`placeholder=true, uploadAllowed=false`，**禁止烧录冷BIN**。原SDK、私有profile与依赖摘要已留存，新目录`build/automation-coexistence-r4-20261031`不改公开发行支持边界。

同build目录的真实Key注入／Sketch增量编译＋完整链接入口及原IDE式单次bossac上传、成功后只读观察入口均已备齐并语法核对，**没有运行上传或串口控制**。最终上传仍核精确USB身份与实际非占位参数，保留桥接授权／任务键，不运行旧十二键恢复或重审上传器。

匹配App不再构建／安装，复用原生产DOM的`/guide/key`：实际名称输入与原提交按钮可用，填写本批唯一计划名称，**未提交**。已备一次明确CREATE→原自动reveal的网络记录及结果捕获入口；若任一步UNKNOWN／近期认证拒绝，保留原身份与意图、不重建或自动重试。取Key只使用当前正式Auth正常近期登录，不导回旧token或篡改门禁。App所有旧授权／UNKNOWN／journal此前基线保持。

### 9.3 原COM11源设备正常启动

只读代码确认原c051／73d949恢复合同对应instance`215af06bebe01744006b9348d9a5e40c`、原根授权及Empty TaskStore；已有DeviceKey和active WiFi profile走`startActive`，不发送显式`n`／`resetNetwork`。使用原固定esptool5.3.1、新批USB精确身份guard，仅一次`read-mac / no-reset / hard-reset`读取MAC后启动原Flash，实际匹配MAC`8c:bf:ea:1b:96:14`。

工具Flash写入／NVS写入／配网调用均0，没有刷板、清根或恢复旧快照。设备运行后可能自然更新WiFi／session元数据，因此只称保留身份／授权／Task语义，**不宣称运行后整个NVS逐字不变**。

启动后同当前账号一次正常Cloud设备列表GET200，原设备`device_b60437fc-dfe2-4745-91cc-57efbf6bf024`已`cloudReachable=true`、新lastSeen`1793440570150`；其余两条COM11旧记录仍离线。源设备上线前置已补，不把presence当作控制ACK或新鲜业务State验收。

### 9.4 唯一接续与保留边界

已通过异步提示请求用户在入口齐备后进行一次同账号正常近期登录，**本批发码0／登录0／CREATE0／reveal0／规则写0**。当前停在服务端近期认证前，不在认证后再准备Core编译／页面控件，不反复询问10台额度或已批准的新R4测试身份。

收到正常近期登录后：原App一次新建独立R4身份／reveal→只留ACL私有Key参数→同build增量Sketch／链接→精确COM9正常上传→匹配实际云上线／原控制ACK及State→两条同目标互斥规则共存短验及停用／删除／回收。沿§7已部署Gateway／App和原Broker，不重复部署、配网矩阵、SDK升级或资源审计；不重用旧scope、不自动重发、不提交推送。

## 10. R4正式环境接入及同目标开／关共存真机有限收口（2026-10-31）

### 10.1 近期认证只用于敏感Key读取

用户完成同账号正常重新登录后，Root立即复用已冻结入口：原App实际CREATE201和reveal200各**1次**，没有发送验证码、自动认证重放、重复创建设备或刷新Key。新独立设备为`device_62dfa80f-9f16-4f9b-8573-6f65b8550e44`，当前正式`api.yiyu.pro`环境，不复制旧测试Auth身份／Key／scope。

重新登录原因是**DeviceKey读取采用最近5分钟正常主认证门禁**，不同于普通access token仍可控制／查询的有效期；不是普通会话掉线，也不要求日常设备控制或自动化每5分钟登录。本批未延长／关闭门禁、伪造近期证明或导回旧token。先完成冷编译／入口，再认证后连续获取Key，本次没有在认证窗口内重新编译Core。

### 10.2 匹配增量编译、普通上传与真实设备前置

真实Key和当前WiFi参数只存ACL私有JSON／头；同原build目录一次增量Sketch编译＋完整链接通过（70.127秒、93个对象复用，未clean），原Core1.5.3／SDK／profile／依赖保持。最终BIN145592 B，SHA256`a6ab9d412d8243920f6938252cdeee0613fd387b694c6fc18dbaacf1cad2cf9b`，`placeholder=false`；任务0槽／空业务签名ring，不扩大公开R4支持范围。

精确COM9／原USB序列核对后，原IDE式bossac**单次正常上传返回0**；没有SWD、上传器底层对照或Flash新鲜读回，不冒称bitwise readback。未运行桥接十二键恢复／Task区写入，也没有恢复旧Auth固件。成功匹配上传后仅一次`j`观察，实际fixture匹配、`begun=1/networkReady=1/cloudReady=1/lastError=0/power=false/writes=0`。

源COM11仍原215实例，目标R4实际新实例`12e911eae208c3196c6294e5dbb67568`；两个不同物理实例均有当前Cloud session及Broker权威State，初始`power=false`。正常App各访问控制页一次取得自身Manifest和真实开关，未伪造缓存、未人工控制目标。R4新身份／Key保留，不删除原3台设备；设备额度仍§9的10台。

### 10.3 两条规则同时启用与真实开／关结果

通过原App名称输入／实际ion-select／真实确认按钮建立两条规则，均初始停用：

| 本批规则 | 原条件／动作 | CAS轨迹 |
| --- | --- | --- |
| 共存短验 开1031 | 同源COM11.power上升沿 → 同目标R4.power=true | 新建rev1 → 启用rev2 → 停用rev3 → 正常退役rev4 |
| 共存短验 关1031 | 同源COM11.power下降沿 → 同目标R4.power=false | 新建rev1 → 启用rev2 → 停用rev3 → 正常退役rev4 |

两次创建PUT200的旧工具结果原样保留，仅HTTP确认不冒称当时UI已完成；随后分别原界面只读inspect，确证App已收尾、pending0／无错误。后续使用新固定post-create观察入口，先不可变保存回执，再等真实App无busy／刷新／pending／错误才确认终态；不覆盖原意图或重跑创建。

两条规则在同目标属性上**同时enabled=true／rev2**；云端一次只读确认各自独立watch、真实同源session、基线`rawValue=false/lastSeq1/occurrence0/cooldown0`，上升／下降沿latch分别false／true。目标设备级在途唯一保护没有关闭，未降低轮询。

此后只在原App源设备控制页分别点击开／关各**一次**，目标R4人工控制0次：

| 事件 | 真实源与目标证据（同服务器时域） | 结果 |
| --- | --- | --- |
| 源false→true | 源State rev2 true接受`1793441664120`，源command实际ACK`1793441664177`；目标State rev2 true接受`1793441664950`，目标command ACK`1793441664973` | 开规则execution `85a5e279…f0803` acknowledged，目标seq4／attempt1／status200；关规则只重新布防，未执行。 |
| 源true→false | 源State rev3 false接受`1793441747851`；目标State rev3 false接受`1793441749033`，目标command实际ACK及result digest与PG binding逐字匹配 | 关规则execution `945e1d0c…9a6f6` acknowledged，目标seq5／attempt1／status200；开规则旧执行整行保持。 |

源手动command恰2条、目标自动化command恰2条，均真实ACK／attempt1；自动化execution恰2条，UNKNOWN保持0。每条规则只触发一次，没有重放／连锁／目标人工补发。期间正常Cloud session更新时，原worker按fence重建基线，不重派旧执行。

本次源State→目标新State约**830ms／1182ms**，只是单次短样本，不是性能保证或UI保存耗时；源App DOM settle为411ms／252ms，**不等于设备ACK**，真实完成只取command ledger／State／binding关联。没有用publish、presence、toast或乐观UI代替设备回执。

### 10.4 真实后台回收、返回恢复及准确工具边界

规则仍启用时，在源设备控制页做有限后台观察。首个观察器尝试在Android挂起后读取后台JS，20秒CDP截止；原intent／异常保留，不当作设备或规则失败。该段仅内存buffer的网络事件未能完整落盘，**不声称该失败后台段有完整wire覆盖**；全部业务控制／规则请求、App journal和服务日志另已保留，未补造缺失正文。

只补这项未完成观察：改为原生焦点取证、事件即时JSONL落盘，后台不执行JS；未重做开／关或创建规则。真实HOME后台**12407ms**、焦点确在App外，之后正常返回。复用已安装MQTT parser和App原BBP2 decoder对留存报文作离线解析（无新网络请求／SDK）：

- 本页实时流原实际1000ms，发送Close operation2，收到**同requestId的Closed status2／stream1／leaseMs0／effectiveInterval1000ms**，不是只发送未确认。
- 12秒后台范围内没有Open／Renew／Command，自动化HTTP补查0；返回后只有新的只读Open operation0，没有控制重发。
- 后续真实页面恢复“云端MQTT · 实际1秒/次”及“刚刚更新”，无降级notice／控制错误，power仍false；离页按原生命周期回收。后台样本不外推所有OEM／长期息屏，不重开Error8诊断。

这是本页实时资源回收和原自动化独立运行边界的短验，不新增后台执行器；没有在后台人为制造额外触发或声称测了另一轮离App执行矩阵。

### 10.5 正常停用／删除与完整收尾

两条规则各原按钮停用到rev3，再各一次真实删除确认正常retire到rev4，**不是SQL删除或清历史**。八笔规则管理写（创建2／启用2／停用2／退役2）全部200、响应正文完整，各自CAS和意图留存；App pending0、当前列表0条。

云端唯一final只读确认：未退休0／启用0／退休2、两个worker claim已释放；原两ACK执行逐行等于after-off，原4条source／target命令逐行保持、attempt1、没有新增目标命令／UNKNOWN。两设备逻辑身份active、credentialVersion1与基线逐字一致，最终power均false；原Gateway／Broker／PG／Redis容器、镜像及健康不变，无新部署或诊断开关。

Root最终保留当前正常新session、全部7项secure；原非session授权逐字保持、旧journal／UNKNOWN逐条完全一致，不导回旧会话或清应用数据。完整Key创建／reveal、八笔规则写、源控制、最终后台短验网络记录、原生XML／logcat与本批Gateway／Broker日志均ACL私有保留；前述失败后台段缺口独立如实记录。CDP转发和只读SSH均回收，App回首页。

**有限收口决定：§6.2所需当前两设备、同目标互斥开／关、真实ACK／新鲜State、正常停用／退役与后台回收／返回恢复通过。** 保留新R4正式环境普通WiFi零槽候选／身份、源COM11原身份、匹配App `086d66c1…8386aad`、Gateway `8b353f33…f5699a`／058及原Broker，不恢复旧Auth程序／scope。10台设备额度继续在测试期间保留，自动化仍4／16，当前0／4、历史2条。

本轮无产品源码新修正、SDK升级、App／服务重复构建部署、验证码发送或提交推送。后续只整理必要交付差异及此前已登记的独立小缺口；发行CI原HOLD、自然UNKNOWN正样本／Wio等后置项不借本次通过销项，不默认再跑登录／配网／自动化矩阵。

## 11. §10收口后的最小修正与源码交付索引（2026-10-31）

### 11.1 剩余缺口与本批最小范围

§10已证明当前匹配部署下的最小自动化可用；本批不再测配网、共存、规则触发或后台矩阵。逐项对照代码后，仅以下两项进入本批：

1. **派生单位模型小修**：`interface-draft.ts` 的 V2 `InterfaceTile.unit` 按 key 回退，会把无单位的布尔 `power` 推成 `W`，还会覆盖显式空单位并读取原型键。只在本模块按“显式 `field.unit`（含空串）优先；仅声明 `integer/number` 且词表自身键才推断”的规则修正，保留原七项数值默认词表。
2. **交付说明与入口一致性**：更正 App README 的 Auth 环境及插件先构建要求，补齐三端＋原生插件必要来源索引，将旧计划的“当前／下一步”标为历史，不再派发重复队列。

审阅确认：当前可见预览 `panelCards`、`WidgetBase` 及 workflow 使用原字段的显式单位，保存布局不序列化 `InterfaceTile.unit`；因此第1项是**派生模型默认值纠正，不宣称已复现可见开关带W**。不扩接其它控件／首页／历史，不改协议、Manifest、显示元数据或用户布局，不猜测覆盖存量标题。

### 11.2 必要源码闭包与构建顺序

下表是未来交付时的依赖索引，**不是本批暂存白名单或已推送声明**。当前工作树包含必要的未提交／未跟踪源码；仅 `HEAD`、旧插件 pin、相同版本号或 schema058 都不能证明取得当前完整实现。保留无关修改，不整体添加工作树，也不输出私有交接包。

| 组件 | 必须一起审阅的来源与约束 |
| --- | --- |
| 设备库 | `Esp32WiFiProvOptions.h`、`Esp32WiFiBLEProduct.h`／`Esp32EdgeHubProduct.h` 的现有接线，`WiFiBleQuickStart`及README；发行器 `build_arduino_release.py` 的私有头拒绝与两项新 WiFiProv release 回归，不漏掉未跟踪头／测试。 |
| App | 当前 provisioning 的 transport／owner阶段／allocation／Admin／CAS／Cloud只读协调／completion／UNKNOWN checkpoint 及其受影响服务、页面、原生发现／注册源码与回归；P3名称解析及各实际消费者、本批 `interface-draft.ts/.spec.ts`。新增文件和调用方需作为闭包审阅，不只取本批两文件或 `git diff` 的已跟踪行。 |
| WiFiProv插件 | `src/definitions.ts`／`web.ts`、Android manifest／`WiFiProvPlugin.java`，以及 `ProvisioningCallScope`、`ProvisioningFailure`、`ProvisioningInternetState`、`ProvisioningLifecycleTrace`、`ProvisioningSdkLocalOwner`、`SoftApSystemConnectedWifi`、`SoftApWorkerChannel/Client/Protocol/Service` 的源文件及对应 Java／接口回归；受影响 iOS 兼容声明也须同行。不交付生成的 `dist` 代替源码。 |
| Gateway | `automation-contract/repository/worker.js`、新 `automation-execution-order.js`、`database-migrations.js` 与新迁移 `058-device-v2-automation-coexistence.js`；contract／A2／worker／repository／coexistence／order／migration／typed helper 回归、`test-helpers/automation-coexistence-cases.js` 及 `package.json` 测试入口。原 Cloud确认模块与消费者也不得在另行整理配网源码时漏掉。 |
| 方案及公开配置 | 本计划、入网修复／完成方案、主线索引、App README／源码手册、Service自动化方案及受影响发布说明。公开地址／公钥与实际支持声明同行，私钥、账号／DeviceKey、网络密码、私有profile、日志、备份、APK／BIN／ZIP／stage／构建目录全部排除。 |

现有 Gateway 构建 `src/**/*` 已包含执行排序模块，repository 已实际引用；迁移注册和 `test:automation` 已接入新增测试。设备库现有 CI 的 `tests/release/test_*.py` 自动发现两项 WiFiProv 回归；**无缺失入口，不为此再改 CI 或启动队列**。这项静态核查不替代未来匹配提交的远端 CI／发行，原 HOLD 保持。

App 保留五个相邻 `file:../...` 插件目录。全新拉取或插件源码变化时，在**五个插件各自目录**先运行 `npm ci --ignore-scripts` → `npm run build`；再在 App 根运行 `pnpm install --frozen-lockfile --ignore-scripts`，使用原 `pnpm 11.19.0`／锁文件。WiFiProv 无 `prepare/postinstall`，它的 `types` 指向 `dist`，不能依赖 App prebuild／sync 自动补建。云端 beta 沿原 `pnpm run sync:cloud-beta` → `android/gradlew.bat assembleBeta`，不用 `cap copy` 或手抄旧注册表代替正常 sync。本批仅核查说明，**不重新安装依赖、生成APK或部署**。

### 11.3 环境、回退及支持边界

当前 `iot.yiyu.pro` Gateway 对接统一正式 Auth `https://api.yiyu.pro`，不是旧 `api-test.yysc.tech`；App 仍经 Gateway 原入口，不转移测试 token／设备／Key／scope。旧源码手册和独立验收中的固定提交已明确仅属历史，不把尚未提交的工作树称为可供 Git 拉取的冻结候选。

058将 enabled 目标设备唯一索引改成非唯一查询索引，但**未改变** pending 的 `prepared/dispatching/outcome_unknown` 目标设备级唯一保护；CAS、fence、原事务及只读协调保持。已有安全共存数据不能靠降级镜像／迁移注册表或重建旧 enabled UNIQUE 直接回滚；回退需另核数据与兼容合同，不删规则／执行／UNKNOWN来消除冲突，也不擅自降schema账本。本批无数据库操作。

公开 cloud-beta仍为 **ESP32手动WiFi／TLS／0槽**。内部R4普通TLS／0槽已上线，不意味着公开支持R4 Cloud、WiFiBLE或四槽TLS；既有逐设备任务准入合同不变。测试设备额度10是测试期权益覆盖，不是套餐或自动化上限调整；测试全部结束后的正常API恢复仍在收尾清单，当前规则0／4、历史2条，不删除历史以清零。

### 11.4 本批验证及后续决定

本批在原 Node22.22.3、现有 Angular 测试入口及 WiFiProv 源码 alias 下，仅运行 `interface-draft.spec.ts`：先红测 **13项中3失败／10通过**，准确复现布尔power误推W、显式空单位被覆盖、原型键取到继承对象；最小局部修正后绿测 **13／13通过、无skip**。新增四组参数矩阵覆盖声明类型、无当前值、显式单位／空串、原七项数值默认、原型键和不写回snapshot／布局。后续原配置 `tsc --noEmit` 通过，不用旧插件dist掩盖类型不匹配。红绿日志保存在现有忽略目录 `blinker-app/build/access-ar4g-20261030/interface-unit-{red,green}.log`，不是交付源码。

本批UI技能只用于显式单位优先和类型语义一致性；没有直接适用的单位检索条目，未据此扩展设计或改布局。不将历史217项或§10真机结果累计为本批测试。本批不操作账号、设备、云服务、UNKNOWN、完整请求日志或清理构建缓存；当前安装的 App 仍为§10候选，本次单位模型改动尚未部署，不新增视觉／真机通过声明。必要文件的 `git diff --check` 通过，依赖／锁／SDK不变，未暂存、提交或推送。

后续仅在用户指定交付／新缺陷后推进：必要源码匹配提交与发行CI仍待授权；自然UNKNOWN重启正样本没有新版自然记录时保留待测，不造记录、不重放；Wio、Error8、OTA、离线历史／Bulk、复杂规则及全部模式开放继续后置。不以这些后置项阻塞当前受限核心功能，也不自动开启另一轮审计。
