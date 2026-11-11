# BLE Direct → 独立 Cloud 卡的有限实施计划

更新：2026-11-04。承接独立接入计划§14.7。BC-1／2见§6～8，BC-3见§9～12，BC-4见§13～17。**BC-5 当前以 §25～26 为准：Key 安装 ACK 与独立 Cloud 卡激活已真机通过；SoftAP 暂停误取消已修并通过离线回归。现有 COM3 固件不支持对已装 Key 的实例发起独立 Wi-Fi-only 恢复，提交与云上线仍未确认。** 原发行不重放，不升级 SDK、不扩槽、不搬 scope，不提交推送。

## 1 已确认的阻点，不是单一UI开关

1. 现保根注册v1只支持Cloud→BLE；源根`cloudCredentialVersion`为正整数，纯BLE无Key不能冒填1或以0／null放宽原校验。原App资格、M2上下文、本地根引用和Gateway持久模型均有方向约束。
2. 原WiFiProv Install能够只加Key、保留Admin／epoch／presence，但GetInfo不能证明TaskStore状态；原Install报文也没有本次intent／根CAS／nonce授权关联。App先完成M2不等于设备实际安装Key时已验证这次授权。
3. 原prepared的120秒过期释放建立在“没有设备写入”上。反向一旦Key已发行／可能下发，继续自动释放会留下设备持Key但云卡／额度已被丢弃的错误状态。
4. 旧enable-cloud把Key加到原BLE ID，违背独立Cloud卡／ID／额度的产品决定；继续禁用它作为新反向路径，不恢复旧代码来绕过上述问题。

## 2 最小阶段与本批范围

| 阶段 | 最小实施 | 当前决定 |
| --- | --- | --- |
| BC-1 | SDK只读区分未绑定、有效空表／有任务、退休标记及未知 | 已完成新增API及定向host，见§6；不提供安装授权 |
| BC-2 | 产品独占owner下取真实事实，在实际保根写Key边界核原根、任务安全及一次操作授权 | BC-2a及BC-2b内部桥／晚检已验；不代签BC-3 intent真实性或BC-4实际接入，不开放网络入口 |
| BC-3 | 方向化根合同、独立Cloud ID／Key及不可丢失的issued／UNKNOWN生命周期 | 内部发行／读取／确认／激活／两卡删除完成，061及062实库通过；App 内部接线消费权威端口，Gateway HTTP 仍未开放 |
| BC-4 | App／WiFiProv显式用途、一次Key安装及一次WiFi下发，分别核ACK与Cloud确认 | 内部候选源码已接显式 App/Android worker 与 ESP32 owner；默认关闭，见§17 |
| BC-5 | 匹配候选最短反向真机差异，核原BLE卡／根及新Cloud卡独立 | §25 已验 Key 安装 ACK、独立 Cloud 卡激活和原 BLE 卡保留；§26 修正选网交接，当前 COM3 不能无重放续写，WiFi 与云上线未确认 |

## 3 BC-1：存储事实与授权分离

复用原TaskSnapshotStore的同一实例、原workspace和TaskSnapshotCodec，不另开存储句柄、不调用TaskEngine初始化、replace、clear或迁移：

| 真实读取 | 观测 | 不允许的推断 |
| --- | --- | --- |
| 原Store合法load返回NotFound | Unbound | 不是本次安装Key许可；仍须原root、独占owner和持久性合同 |
| 有效schema2，包含零任务表 | Bound | 零条任务不等于无scope／root绑定 |
| 有效schema3退休标记 | Retired | 不等于已删除或可重新绑定；不得自动clear |
| 未观测、损坏、容量不足、端口错误、failed handle | Unknown，并保留原错误 | 不假装Empty，不自动重读或新建wrapper解除未知 |

观测没有有效期／令牌，也不上传scope、rootDigest或任务内容。后续调用者必须持产品独占owner，并已确认端口持久性；即使收到Unbound，也不能缓存后在别的线程／会话直接安装Key。发生未知写入后，新建TaskSnapshotStore包装同一脏端口不构成恢复。

本批不修改已有Key写入、observer、WiFiProv endpoint、BBP2或磁盘格式，不增加普通Sketch准入。TaskStore观察不能代替权限、M2、任务退休、写边界fence或新Cloud身份确认。

## 4 后续写入与服务端合同必须同时守住的边界

- 源仅限当前owner／tenant的可信BLE注册，核同instance、epoch、Admin及新鲜授权；MAC、邮箱和公开instance不构成证明。安装前“无Key”与安装后“新目标Key版本”需方向化判别，另定合同域，不能更改v1摘要语义或改写旧UNKNOWN。
- 实际设备写入边界必须核同根和任务安全，并关联本次操作；native callback不读取NVS／等待owner。独占与任务写的顺序须闭合，不能凭此前一次读取放行；未知结果不得借同Key早退／同handle读回产生伪成功。
- 新Key只属于独立Cloud目标ID，复用原vault及目标ID／版本／locator的AAD，不把Key挂回BLE源。保留原Admin／epoch／presence，原BLE保持其根引用，不自动把材料writer移交给新Cloud卡。
- 未发行／未交出Key的准备可过期关闭；Key已发行或安装未知，必须保留同目标、密钥引用、额度和意图，显式只读协调。配额、删除、过期和取消消费者需一起接线，不能先只改prepare／commit。
- 安装ACK、WiFi官方回执、Broker准备和Cloud新鲜上线是不同事实。超时不误报失败、不换Key／ID重试，不把Broker200或App完成M2冒充设备已安装。

## 5 BC-1验收口径

有限host回归须覆盖NotFound、有绑定空表、有任务、退休、损坏／截断、scratch容量不足、底层错误及写失败后禁止同handle读取；连续使用同一输出对象不得留下上一次Unbound。仅编译运行新增目标，不跑旧配网、SDK全矩阵或真机样本。

本页随实际结果追加，不将“读到了存储状态”写成“BLE→Cloud已开放”。产品仍按原具体门禁拒绝反向注册；提交、远端CI及私有golden授权HOLD保持原边界。

## 6 BC-1实施与有限结果

SDK在原`TaskSnapshotStore.h/.inc`新增一字节`TaskStoreBindingObservation`及`observeBinding(output, headerScratch, recordScratch, capacity)`。仅复用原load／Codec，header和records由调用者提供，不新增实例成员、heap、磁盘格式、wire或Key写入接线。每次先重置Unknown及header；NotFound观察成功但不是安装许可，其它失败原code返回；failed Store不会再调用底层load。

新`tests/host/test_task_binding_observation_main.cpp`使用真实Store／Codec和合成内存端口，接原CMake目标`blinker_task_binding_observation_tests`。先缺API编译红，实施后原MSVC Release／`/W4 /WX`定向编译及以下**一个目标**通过，含9组场景；不是99个注册目标全部运行或通过：

```sh
ctest --test-dir build/task-binding-observation-20261103 -C Release -R '^blinker_task_binding_observation_tests$' --output-on-failure
```

覆盖NotFound、有效空表、有记录、退休、损坏／failed handle、未知写后不得读回证明成功、读错误、容量不足，以及同输出对象Unbound→Bound→Retired→Unknown／Unbound→读取失败。所有只读路径核零replace／clear；未知写反例仅fixture受控先写一次，随后观察不得再读端口。实际日志保留在本机忽略构建目录，未操作真实授权、任务或设备。

独立源码审阅通过。未重跑旧host、SDK矩阵、配网／附近／Cloud→BLE或任何真机样本；未部署、迁移、提交或推送。旧DA文档页首已改指实际§14.6，避免“仍缺Admin、附近未验”的过时停点覆盖后续成功。

BC-1当时冻结的下一步已由§7推进；当前唯一后续取§7.5，不重复本节host或把Unbound当持久安装许可。

## 7 BC-2a：内部保根首次Key事务与旧旁路封堵

### 7.1 真实写边界，不是新的网络授权

`IDeviceAccessStore`新增内部`installInitialCloudKeyVerified(candidate, expectedEpoch, expectedAdmin)`，默认Unsupported；具体Store只供可信产品调用，不是Sketch／wire入口，也不因名为Verified就证明已完成M2。先拿专用first-Key fence，再读真实根并比较完整原Admin／epoch（含域、代际、版本、权限、suite、id及secret）；只增加Key，原controllers／epoch／presence保留。已有不同Key拒绝；同Key零写成功也须过fence，不接受同handle尝试值代替持久性。

observer新增独立命名的首次Key借用，默认关闭；原`TaskRootOwnerGate`维持busy锁，Key与Presence借用互斥，原普通写不能重入。实际写／读回失败保原unknown poison；明确无写入的CAS或不同Key拒绝不poison。没有复制根存储、密钥、TaskStore或一般可重入锁。

### 7.2 原产品／存储生命周期薄接线

原TaskModule→Esp32TaskModule→WifiTaskExtension→Esp32ProvisionedTaskExtension沿同一TaskStore／Codec／NVS handle转发，任务snapshot只接受Unbound；Bound（含零表）、Retired、Unknown均拒。首批限Engine尚未初始化，才可借自己的records/workspace，绝不在运行中的Engine背后解码覆盖。

ESP仅将原单次storage open与单次Engine begin分开；后续正常Engine begin仍借同handle，open／读取失败不重开或重读解毒。产品核observer确属同Store，自己取得maintenance owner（含inactive配网阶段），先停止management／使TaskAuthority失效，再读取和提交；全部NVS操作在协作调用中，observer callback仅检查状态／借用，不I/O或等待。

两个窄条件分别限定“产品事务正在进行”和“原Store已返回Unbound后的实际同步commit”。观察回调不能提前借Key口，inactive也不能绕owner重入transport或第二事务；结束撤掉两层窗口。已知零写拒绝只结束本次并标dirty供原生命周期刷新，不自动重试Key；只有原root uncertainty或TaskStore／port failed才保持实例锁闭，stop/start不能清除。

产品预读`loadAdminSelector`发生根读取错误时，尚未准入低层Key事务或打开任务NVS：返回原错误、零Key写入、不自动重读；后续一次新的显式内部调用可重新核真实根。它不同于已准入事务内的根读取／提交故障；后者沿原guard锁闭，不能凭同handle露出Key、再读或stop/start解除。

### 7.3 旧入口兼容与已确认的UNKNOWN别名

- 普通`DeviceAccessStore::replace`不再给“已有Admin但缺Key”的根首次加Key，属刻意收紧，不能绕专用门。三参WiFiProv Install每次核真实epoch／Key，缺Key返回原AccessRequired、读取错误StorageFailure；`installed_`历史标记不能跳过。
- 两参纯Key路径、空根Bootstrap、新Bootstrap提交及已有Key正常刷新保持；报文／版本／status／Endpoint字段和32位16 B门禁未改变。
- 普通replace同Key现先拿ordinary fence再读；同值零写也不能绕poison。另两个真实反例均先红：无Presence根首次Key写unknown后，旧Bootstrap exact-same错误Success；已有`installed_=true`的Endpoint从同handle露出Key时也错误Success。前者仅给same-state零写分支加ordinary fence；后者combined同Key重放沿原replace零写fence确认，不把不同Key变为rotation。没有添加未知写后的自动重发。

### 7.4 有限验证与真实停点

唯一原host构建目录、原MSVC Release／`/W4 /WX`下定向完整编译，新增三目标：

| 目标 | 实际有限范围 |
| --- | --- |
| `blinker_task_initial_cloud_key_tests` | 9组：默认关闭／owner、完整CAS、根保留、同值／不同值、known拒绝、unknown及Bootstrap别名、借用互斥 |
| `blinker_device_key_initial_cloud_gate_tests` | 7组：Admin-only旧Install拒绝、历史installed不绕过、错误／短buffer／错composition、原纯Key／Bootstrap／刷新、UNKNOWN fastpath |
| `blinker_cooperative_initial_cloud_key_product_tests` | 9组：真实内部产品模板、同handle、inactive观察重入、绑定／退休／错误、已begin Engine、observer／owner、CAS与poison、预准入只读失败与显式新调用 |

三新目标及原Endpoint／Presence两个受影响目标共5／5通过；随后仅追加product第9组并单目标严格重编、执行通过，生产源未再变。旧Store只执行修改到的4函数窄入口`--initial-key-contract-only`通过。新三目标最终为25组，不累计相邻旧场景或BC-1／App结果，不冒称全host矩阵。完整architecture checker通过，未改freeze摘要。

保留缺接口编译红、初始Endpoint错误映码红（底层当时已拒写，不是实际新增Key成功）、两个真实同值别名红。真实ESP模板host consumer曾遇常量if的MSVC C4127，改原析构为等价C++11 tag dispatch，不压产品告警；原host main显式return0，避免重命名复用fixture时压C4715。签名外部适配器全部拒绝，不伪造网络认证；固定单个placement产品图处理原静态属性owner的fixture地址约束，不改配置API。

此为host假NVS／transport的产品与写事务证据，**没有原SDK链接、硬件Flash、真实认证会话或云端联调结论**。未刷板、发码、创建账号／Key、迁移数据库、部署或提交推送；原授权、TaskStore、scope、UNKNOWN及完整日志不动。

### 7.5 下一步仅BC-2b：可信会话与一次操作授权

现有可信上下文只在设备Method2校验成功后的`DirectSecureContext`中出现，须等AuthResult实际排队并commit才是活动会话；prepare、App布尔值、ControlOpen的Control权限或独立controlNonce均不足。下一批先冻结最窄内部producer／consumer合同，再接已commit会话owner：同transport／session、Platform Admin及All权限、instance／epoch／版本／原根关联、双方nonce和本次intent；取消／断连立即失效，原secret不新增长期副本。

当前内部事务接收的expectedAdmin仅用于真实根CAS，不是网络授权票据，尚未接此认证owner。BC-2a不完成整个BC-2，不开放App／Gateway／WiFiProv新用途；后续还须BC-3的独立Cloud目标／Key发行和UNKNOWN额度生命周期，不能继续用原prepared到期释放规则。

本节记录BC-2a当时的停点；内部认证owner已由§8推进，当前唯一后续取§8.5，不重复本节回归。

## 8 BC-2b：已提交Method2与单次注册意图的最小合同

### 8.1 本批范围及授权来源（2026-11-04）

只接设备内部认证producer到BC-2a消费者，不添加App／Gateway注册接口、WiFiProv写用途或新wire解码。默认产品图不自动装入本授权桥；普通控制权限与原Method2报文不变。下述合同先冻结，实施结果另记§8.4，不把方案当运行证据。

1. 授权桥封装原`ControllerHmacSha256Authorizer`，只有它实际验过HMAC后的prepare才能暂存非秘密认证事实；最终AuthResult成功入队、原transport真正commit后才可用。公开`RxContext`、expectedAdmin、prepare状态或ControlOpen均不能生成许可。底层默认不支持已提交binding查询时返回空，保持关闭。
2. 显式首次Cloud注册只接受同transport／session的PlatformAccess、All权限及当前Admin版本／epoch。双方Method2 nonce及派生sessionBinding均绑定；不保存第二份Admin secret或设备Key。原根在消费时重新读取，写边界仍作完整CAS。
3. 一次操作证明采用独立域的真实HMAC-SHA256，精确绑定本机instance、完整认证描述、双方nonce、已提交binding、非零16 B intent及候选Key原字节。intent是服务端独立注册意图的opaque值；源／目标逻辑ID和Key归属由BC-3映射，设备不凭MAC、公开instance或App自称判断归属。本批不沿用v1正Cloud版本摘要。
4. 期限自本机commit起最多120秒，使用单调时钟及回绕安全比较；不靠固定等待或ready快照放行。通过证明后，在进入产品写调用前消耗本会话的执行权；成功、known拒绝、忙、UNKNOWN均不自动恢复或重发。新操作须显式新认证；实际未知写仍受原存储poison约束。
5. 消费者复用原独占owner／TaskStore。专用guard在真实根读取及完整CAS之后、任何同Key成功或实际写之前再次核活跃binding、期限及原Key／Admin关联；此guard不做NVS、等待或重试。取消、断连、stop、reset、换会话均使旧许可失效，调用中取消也不能重入或重置busy来再写。

guard是写准入点，不是可撤销Flash事务：其前取消／过期须零写拒绝；通过后若真实提交中才发生断连或取消，仍保留installer原始成功／失败／UNKNOWN结果，不能在返回时用NotConnected覆盖持久事实。后续执行权仍已消耗，无法确认的写只沿原UNKNOWN只读协调，不自动重发。

设备内部proof合同为186 B定长消息：ASCII `blinker.initial-cloud-key-registration.v1`加NUL，随后依次为instance16、method:u16be=2、suite:u8=1、controllerId16、domain:u8=2、epoch:u32be、credentialVersion:u32be、permissions:u32be、clientNonce16、deviceNonce16、committedBinding16、intent16、Key原字节32；以原Admin secret32计算HMAC-SHA256，输出32 B。没有CBOR、长度前缀或MAC字段；不混用16 B Key locator。这是本批内部合同，不意味着服务端已签发／承认该intent，也不是已发布的网络命令。

### 8.2 不跨radio owner移交授权

现WiFiBLE图无Key时先运行WiFiProv，Direct只在配网引擎明确停止后启动；虽然同固件有两对象，不代表纯BLE根当前就能先完成Method2。原task owner已held的回调也不能重入maintenance。故本批只验内部桥及实际写边界，不改这两项安全条件。

BC-4须显式接首次Cloud用途的PreCloudDirect有限阶段：Cloud／任务不启动，在原Direct上验证并完成一次Key安装，然后关闭Direct并退休全部认证上下文，再交既有WiFiProv下发WiFi。不得把旧会话能力带过radio owner交接；若改为WiFiProv阶段才安装Key，须另定设备一次票据，不能缓存auth布尔值。本批不提前实现该阶段或另起配网链。

### 8.3 有限回归范围

新增授权桥host目标只核真实Method2 HMAC及生产会话状态机；AEAD primitive若沿既有确定性fixture必须单列，不能称真实AES／无线证据。另一个有限消费者目标核guard位于实际根CAS之后、同值也需授权、晚取消／过期零写拒绝及原owner不放宽。只补必要相邻受影响目标，不重跑BC-1／BC-2a全矩阵、正常配网或真机样本；原SDK及私有golden授权HOLD不变。

### 8.4 实施与有限结果（2026-11-04）

新增内部`InitialCloudKeyRegistration`封装原Method2 Authorizer，私有实现Direct控制端，原HMAC算法和AuthResult报文不变；只在自己的authorize调用栈捕获成功prepare的非秘密事实，且Authorized结果返回、精确queued回调及实际transport commit全部成立才激活。原控制器新增默认空binding查询；`BleFrameTransport`仅active、Online且原`records_.hasSession`确认真实连接时返回binding，原生断连尚未送达回调也不能只看旧slot。默认产品图不自动接入此桥。

一次install先复制借入的intent／Key／proof／会话事实为同步栈快照，再读原凭据并验完整HMAC，先消耗执行权才调用原产品。cancel／reset不清调用中busy；新auth与重复queued不能重入或续期旧操作。不保存长期secret／Key，不添加heap、存储句柄、线程或重试；busy仅防原协作owner内重入，不冒称任意线程安全。guard接口经原TaskModule三层转发，在具体Store的真实根CAS后、同值成功或实际写前调用；旧实现默认Unsupported，不能降级到trusted-only入口。

独立审阅确认上述边界。审阅中修正一处真实返回合同缺口：删除installer返回后的liveness覆盖，避免已开始持久提交时发生取消／断连把原成功或UNKNOWN改成NotConnected；该修正有专门host反例，未声称先红后绿或真实设备曾发生。新两个目标沿原CMake、原MSVC Release及`/W4 /WX`完整编译、链接：

| 目标 | 本批实际范围 |
| --- | --- |
| `blinker_initial_cloud_key_registration_tests` | 10组：独立HMAC向量／字段反例、真实M2 prepare与commit、非Admin、错peer／intent／Key／proof、生命周期／原生断连、重新握手／真实Admin rotate、120秒／回绕／重复queued、晚取消／过期／重入、known／UNKNOWN消耗、写准入后结果保留 |
| `blinker_initial_cloud_key_commit_guard_tests` | 6组：真实Store内实际Admin晚检、同Key零写也检查、根读中取消／过期、CAS／unknown围栏、真实产品转发、原Bound／owner前置 |

两新目标共16组，另仅执行被改接口／Store公共实现影响的`blinker_direct_secure_session_tests`及`blinker_task_initial_cloud_key_tests`，最终CTest共4／4通过；不累计旧25组或称全host矩阵。architecture checker通过，未改freeze摘要。唯一构建目录仍为`build/task-binding-observation-20261103`，无另开队列；完整本机输出留原忽略目录`Testing/Temporary/LastTest.log`。

授权测试使用真实Initiator／Authorizer的HMAC、真实`DirectSecureSession`及`BleFrameTransport`，成功路径Rx来自原seal→BLE分片→解封。认证结果queued由fixture受控调用，不称已执行Runtime新命令；原确定性AEAD、Clock／RNG／link／blob为外部测试端口，非真实AES或无线。installer spy只证明生产guard接受，具体Store／产品由第二目标另验，不拼成真实设备端到端结论。另用原Node内建crypto独立计算186 B合成proof常量（无新依赖），与字段变体一同核对；它不是私有服务golden，不解除发行HOLD。

没有原SDK定向编译、制品、App安装、刷板、服务部署、账号认证或验证码调用；授权、TaskStore、scope、UNKNOWN及完整日志未改。现阶段只完成设备内部桥与消费边界，不宣称BLE→Cloud已可用。

### 8.5 下一步仅BC-3：方向化注册与发行生命周期

下一批先冻结BLE源无Key情况下的方向化根承诺、独立Cloud目标ID、intent与Key的绑定及issued／UNKNOWN状态机，再接原注册／vault／配额／删除消费者。设备内部proof证明持原Admin者对本次Key／intent的授权，不证明该intent已被服务端准备、Key已合法发行或账号拥有目标；这些必须由BC-3完成，不能放宽原v1、回到enable-cloud或把已issued记录按原120秒自动释放。

BC-4再把该合同接入显式App用途、PreCloudDirect及原WiFiProv生命周期；旧会话不得跨radio owner移交，已有Key／任务绑定仍按现门禁拒绝。当前不做这些阶段的半开放部署，不重复原扫描／配网／两卡真机样本，暂不提交推送。

## 9 BC-3a：独立Cloud发行账本及保留围栏（2026-11-04）

### 9.1 最小合同与实施边界

完整方向合同取[BLE→Cloud注册v2](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md)。原v1及060仍只接受正Cloud版本；新增严格`access-registration-cloud-v2`及`ble-no-cloud-key-v2`／`cloudKey:{state:'absent'}`，不传0／null绕旧解析。新根版本正uint32，但须与可信源当前Admin完全匹配；源按产品、owner／tenant、逻辑ID及原实例关联核验，不按MAC或单独instance合并。另一个源即使instance相同，也不能据此误合并／拒绝本源。

本批只接内部prepare／metadata read／密文issue／markUnknown／cancel，**不接明文读取、安装确认、commit激活或HTTP**。原因是原Key表FK需已存在逻辑卡，而无Key准备不能伪造060正Cloud根；原Presence reader还假定Cloud是writer。先持久记录不可丢失的发行意图，再在BC-3b一次接准确认、原BLE materialOwner及正式目标，避免提前创造active卡或转移原材料。

### 9.2 复用实现与实际消费者

Gateway新增`access-registration-cloud-contract/repository/key/lifecycle`薄模块，原Key生成的四次非零规则提为`device-key-material`供旧repo及新发行共用。`vault.js`及算法不改，显式注入原vault；source／target／intent／根／nonce／目标version1／locator的issuanceDigest作staging额外AAD。明文在finally清零，元数据投影不含密文、Key或secret，重放只返回原发行元数据，不再次随机Key。

061只新增`v2_cloud_registration_intents`、索引及有界行trigger，不改060或插active目标／正式Key／共享根。准备身份、目标、root、nonce及期限不可更新；已发行材料不可换Key、清空、TTL取消或删除，UNKNOWN不可回退。该SQL已注册，**本批没有实际执行迁移**；不能把SQL源断言称作约束已在PostgreSQL生效。

原统一quota增加新ledger目标的UNION：prepared未到期才计量，key_issued／install_unknown不看TTL；按目标ID去重，未来转换正式卡不计两份。新retention薄口同时接原root／Presence变更、source删除、BLE恢复的issue／commit和账号删除事务。root/member为空也核ledger；source删前未发行可cancel，已发行只保留未决及原Presence引用。不复制删除worker、Broker执行或任务scope，也不按新目标选举writer。缺061或读取错误不当成空ledger。

审阅发现并修正发行尾窗：coordinator初检TTL后读取原根可能跨120秒，实际seal前须重新取时；跨截止先在原事务持久关闭prepared，再返回410，不生成候选Key。新反例在carrier先红后绿（Admin／instance关联两个停点），证明零额外RNG／密文；未宣称真实PG曾错误持久Key，061原CHECK也会拒绝超期issued_at。

### 9.3 有限结果与未验证项

六个新增纯Node测试文件共40项，包含v2合同3、仓储13、原vault／共用Key生成5、迁移注册／SQL源6、retention9、BLE恢复围栏4；另quota增加2个反例。计数按唯一用例，不累计复跑或原BC-2／App结果。主代理四文件组合27／27通过，随后仅新加独立摘要常量的合同3／3通过；consumer与BLE guard分别已过，不再重跑。真实AES-GCM的staging反关联、密文/目标/nonce保留、提交后丢响应、事务carrier回滚及跨TTL均覆盖。

必要相邻回归：quota31／31（含新2项）、原lifecycle16／16，presence映射／产品准入／BLE continuity／原Direct与Presence脚本等runner43项通过；原schema58升级注册的单一更新用例1／1通过。Direct／Presence老脚本内部明确跳过PostgreSQL，不累计为实库通过。原PG升级测试仅将当前history追加61并保留59／60前缀、before MAX及count60断言，未运行。

Node语法核查及维护runner路径核查通过。新增Gateway `test:cloud-registration-staging`有限脚本，并从原workspace Device V2 runner调用；本批未运行整套workspace、远端CI或构建服务镜像。原依赖／锁／SDK保持，现有dirty按hunk保留。

SQL carrier只模型化结果／回滚，migrator测试仅用假的pg client；不证明SQL语法、隔离／锁或trigger执行。可选真实PG runner只接受显式`TEST_DATABASE_URL`，没有明确隔离连接时不运行、更不fallback业务`DATABASE_URL`。原业务账号、授权、TaskStore、scope、UNKNOWN及完整日志不动；没有数据库连接、服务部署、刷板、发码或提交推送。

### 9.4 唯一下一步：BC-3b先补数据库，再接读取／确认／激活

1. 用独立测试库执行060→061真实迁移、约束／trigger、held额度与事务失败回归；当前缺证保持待验，不因pure测试绿开放。
2. 接原近期认证保护的同intent秘密读取，严格owner；不可从普通已登录、refresh、`/me`或测试放宽开关取得新方向许可。读取只解原Key，不生成替代Key。
3. 冻结真实安装回执／UNKNOWN只读协调及确认摘要，按明确App验证模型接最终commit：原子生成正式Cloud目标／Key，按原Key表AAD重封，独立根／额度转换与原BLE材料归属一起接线。不能直接把staging密文拷到原Key表。
4. 补新Cloud与原BLE各自删除、源已删／账号已删的拒写及原Presence唯一writer，不将未物化目标404解释成未安装／可释放。

BC-4无线交接及BC-5真机继续后置；本批不开放v1 Cloud方向、不恢复enable-cloud、不迁移旧scope或补造旧UNKNOWN。必要源码清单取[提交计划§11](DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md#11-后续bc-3a服务端与文档增量2026-11-04)。

## 10 BC-3b前置：真实事务用例补齐，隔离数据库待确认（2026-11-04）

按§9.4先核数据库，未越过该门实现秘密读取或激活。本机没有`TEST_DATABASE_URL`／`DATABASE_URL`、可用native PostgreSQL或运行中的Docker／WSL；不为验证而连接业务库、启动Docker Desktop或修改现有服务。已向用户询问129测试机一次性隔离PostgreSQL16或现成可丢弃测试库配置；确认前不连接或启动远端实例。

原可选PG runner的四子项主要使用observer autocommit，尚缺计划要求的真实事务失败证据。本批仅补第5子项：read／issue可注入原db executor，默认observer不变；以原`db.transaction`的SERIALIZABLE／maxRetries0验证事务内issued而独立observer仍prepared，合成失败后回滚全部材料；再在同intent真实COMMIT后模拟调用方丢响应，仅读协调保留原密文／nonce／digest，不再次issue。它验证ledger／vault／生产transaction，不代签新repo完整sourceAuthority或无线。

本节当时仅Node22.22.3语法及空白检查通过，五个PG子项尚未执行；后续实库结论见§11，不再以本节停点覆盖新结果。原授权、TaskStore、UNKNOWN及日志保持，未部署／发码／升级SDK／提交推送。

## 11 BC-3b：061实库与内部Key读取（2026-11-04）

### 11.1 可复用测试库及真实事务结果

用户明确允许在129测试机保留专用测试数据库供后续复用。本批仅新建`blinker-qa-postgres16`、独立卷和内部网络，无宿主／公网端口、不复用业务库或账号；PostgreSQL16.15复用原缓存镜像，内存上限192MiB。专用测试角色无superuser／createdb／createrole／replication／bypassrls。连接配置仅在服务器root所属0600私有文件，不入Git；[服务仓操作说明](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_ISOLATED_POSTGRES_TESTING.md)记录边界和唯一执行入口，不记录密码。

本机Node22.22.3、原锁依赖及当前工作树经SSH隧道只执行一次新061 runner：**5／5子项通过，含父项Node计6pass／0fail／0skip，约16.0秒**。覆盖真实060→061升级、材料约束／trigger、prepared取消／源槽、issued／UNKNOWN跨TTL held额度及UNION、SERIALIZABLE回滚和真实COMMIT后丢响应的只读协调。没有将旧40项或设备测试重复计入。

末检测试角色schema剩余0，专用数据库约8.38MB并保留；原22容器均运行且未重启。原账号、授权、TaskStore、scope、UNKNOWN及完整日志不变，没有业务迁移、Gateway／App部署、刷板或发码。SQL合成库存不能代签真实源授权或无线安装。

复用runner后续安全收口改为单文件单进程、真正2MiB有界输出、180秒截止及未启动隧道的安全清理；新增7个离线Python用例通过，不重跑已通过的PG。本批实库运行发生在该维护修正前，二者不混称同一次远端结果。

### 11.2 内部同intent秘密借用已接，HTTP仍关闭

新增`access-registration-cloud-key-service`只供原repository的可选`keyAccess`使用。真实身份解析器从同token／region给owner与tenant；原recentAuth增加调用期`withVerified`，只复用Redis原期限和注销代际，DB等待后在解密及借用前同步晚检。原check的返回形状、Lua、登录／注销及普通Key入口不变，不用测试放宽开关或本地重新算5分钟。

原repo私有锁口复用原authority／coordinate和projection，核源、Admin CAS、原TTL及`key_issued`无关闭原因；秘密事务显式`maxRetries=0`。源删除／根变化先持久标UNKNOWN再拒绝，已过期issued继续held，不释放材料或重新随机Key。UNKNOWN只可读metadata，新的近期认证也不能再次借用。

秘密按原staging AAD及issuanceDigest解密，重新核locator，只同步借32B Buffer给受信任内部consumer，finally清零。明文、密文和secret均不进返回值；异步consumer、thenable及伪造AbortSignal拒绝，取消取原生内部状态而不是可覆写属性。callback完成不代表设备ACK；借用后取消／过期不覆写已发生事实。未知DB／COMMIT错误只给通用503，不暴露内部信息，也不意味着秘密没有借出，不自动重试。

有限Node回归的唯一新增叶数为31：近期认证lease11、reader12、repository集成8；另只运行受影响的原recentAuth12及原repo13。reader原11项先通过，追加原生signal1项定向通过；不把pattern跳过的旧叶计作未验或累计新通过。真实原Vault用于reader及repo，但Redis回复／身份解析和SQL carrier为明确合成端口；不能称真实账号登录或新增消费者实库并发已验。独立审阅发现工厂函数／对象调用不匹配已在集成前统一，未增加兼容alias或开放旧入口。

本批无App／WiFiProv／设备库生产修改、无线测试或业务服务部署；没有发送验证码、创建实际账号／设备、升级SDK、提交或推送。新维护脚本及源码清单取[提交计划§13](DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md#13-bc-3b实库与内部读取增量2026-11-04)。

### 11.3 唯一下一批：确认、激活与方向化材料归属一起闭合

不再重跑061或重开资源／扫描审计。先冻结安装确认及精确重放合同，明确采用“App核真实认证通道回执”的信任模型；补增量迁移支持`issued／UNKNOWN→committed`及不可变确认摘要，不修改061触发器来假装已有提交能力。

同一有限批次实施原根CAS后的独立Cloud目标激活、staging解密／正式AAD重封、060根／两成员、额度从预留转active去重，以及原BLE固定materialOwner和两卡各自删除。060当前Admin版本固定1而新v2可为正uint32，必须显式支持或明确首批实际版本门，不冒填1。真实确认未知只读协调，旧提交／删除不复活目标、不转移writer。该闭环及有限失败／幂等回归完成前不接HTTP／App；再进入BC-4一次无线交接和BC-5最短真机差异。

## 12 BC-3b确认与激活收口（2026-11-04）

### 12.1 本批冻结范围

精确合同取[v2合同§7](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#7-bc-3b安装确认幂等及独立激活合同)。先实现内部确认／独立目标原子激活、固定BLE材料归属与两卡独立删除；HTTP／App／WiFiProv／wire和部署继续关闭，不把服务端fixture的receipt当真实设备ACK。原授权、TaskStore、scope、UNKNOWN及完整日志保留，不重跑061、扫描／正常配网／自动化或资源审计。

服务端确认精确绑定原owner／tenant／intent／根／发行nonce／Key摘要、同Method2会话和成功ACK摘要；确认及commit两个独立域区分幂等。原窗口结束不取消已发行事实，可协调真正的RESULT_UNKNOWN；删源／删号／换根的未决仍拒。已提交重放先识别原摘要，目标已删只读墓碑，不新激活。062增量而非改060／061；原Admin正uint32不冒填1，原v1方向及解析保持。

本节先冻结实施合同，实际有限回归及未验证项完成后记§12.2，不能把计划视作已通过。

### 12.2 实施、有限回归及062实库结果

原Cloud repository新增内部commit，严格解析确认，并在同SERIALIZABLE事务内物化独立Cloud目标、正式AAD的原Key、非秘密Admin元数据、060共享根／两成员和不可变committed账本。原staging密文和BLE身份／Presence／任务区不搬移，明文在第一个SQL等待前清零；reserve转active仍占独立一台，不二次扣额。已提交同摘要先识别，源已删或TTL结束不重建；目标已删只投影墓碑。

新增`access-registration-cloud-activation`复用原vault，062仅增量扩状态／六确认列、row guard及正uint32 Admin约束。062拒绝用NULL三值逻辑漏放UNKNOWN，也拒绝先洗关闭原因再确认；committed的摘要、根关联及旧发行材料不可变。060／061本体、原v1解析和摘要不改。

原Presence绑定按完整v2committed记录固定BLE materialOwner，旧Cloud→BLE保持原路径。两种删除顺序都沿原removal，活另一卡保留；末删仍撤ACL、退休root，但留必要密文／确认审计，不声称物理擦除或远端撤权已确认。交叉审阅发现两处实际消费者遗漏并限定修正：reverse删除前补完整root／Admin／revision CAS（新反例先红后绿）；已激活后旧BLE恢复issue／commit复用原共享根围栏，不能因退出pending就改掉两卡的Admin／Presence。终态精确重放仍早退，活BLE普通Presence轮换不被永久锁死。

本批新增纯Node唯一36叶通过：确认合同6、062迁移5、原子激活13、Presence／删除9、共享根恢复2、registry新升级单项1。另仅运行受修改路径的必要相邻35叶：旧Presence13／lifecycle16、Key读取2、schema60→当前历史2及schema53／56历史2；定向重跑改过的回滚停点、NULL保护或新删除反例不累计新叶。回滚fixture明确命中六个正式writer，不把同名Admin读取当成INSERT故障。

复用现有专用PostgreSQL16.15，仅运行新的`test-device-v2-cloud-registration-activation-postgres.js`一次，**7／7子项通过；Node含父项共8pass／0fail／0skip，73.6秒**：

1. 真实61→62升级保留原issued／UNKNOWN材料及v1根。
2. 真实生产commit、原正式Key读取器及vault验证同Key重封，原BLE不变、Cloud无复制Presence，计量仍为2。
3. RESULT_UNKNOWN超过原TTL仍可确认；删除／根变化／账号删除及NULL原因不能洗成可确认。
4. 原SERIALIZABLE事务提交前失败回滚全部目标／Key／根／成员，独立observer看不到未提交事实。
5. 真实COMMIT后合成调用方丢响应，精确协调原结果；正常删除目标后不复活、不重随机。
6. 原Presence／removal真实SQL消费者验证两种删除顺序，无writer转移，最后root退休、已删卡不可读取。
7. SQL直接验证commit字段／发行材料不可改、不可删除／回退或预填，以及root唯一性和Admin版本边界。

实库使用合成owner／tenant／BLE关联和App确认向量，不是真账号、硬件签名、Method2或无线ACK验收。测试after-hook已清理本批精确随机schema，专用库保留；没有业务迁移、服务重建或部署。旧061／设备／App／配网／自动化矩阵不重复。原账号、授权、TaskStore、scope、UNKNOWN及完整日志保持，无SDK升级、暂存／提交／推送。

### 12.3 下一批仅BC-4实际消费者，不重开已验后端

内部服务端闭环已补齐，不再重做061／062或资源审计。下一批先冻结显式BLE→Cloud用途的实际认证命令与成功ACK解析，再把BC-2桥接到PreCloudDirect有限阶段及原App注册消费者：原Method2双方nonce／sessionBinding与本intent／Key对应，保根写前仍核真实Unbound／owner／CAS，取消退休一次权限。

Key持久ACK核验后先确认服务端原intent，不用Cloud上线反推安装；明确Direct owner停止后才交原WiFiProv下发一次WiFi，旧会话不跨radio owner。App保留issued／UNKNOWN只读协调和独立两卡引用，不能借旧Install、enable-cloud、Bootstrap或普通成功布尔值补链。设备安装ACK、官方WiFi回执及Cloud可用状态仍分别判定。匹配源码／原SDK定向链接通过后再安排BC-5最短反向真机，不重复Cloud→BLE、扫描、正常首装或自动化。本批未开放这些调用方，也不把新Cloud卡删除后的保根重新注册扩成新功能。

## 13 BC-4a：专用安装回执与原生停止证据（2026-11-04）

### 13.1 本批实现与验证

- [精确 wire／摘要合同](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#8-bc-4专用安装报文与实际回执)：仅候选 BBP/2 `0x38/0x39`、97/46 B 完整帧。C++ 专用 endpoint 复用原 Method 2 已提交 binding、BC-2 HMAC 与晚检安装，不添加普通 Runtime handler；写前验 ACK 容量并预编码，写成功只对同 binding 加密入队，不能回退明文。安装结果与发送结果分开，解封暂存退出回调后擦除。
- App 专用 client 复用原 Method 2、Direct AEAD、exchange 和原停止路径；校验冻结的源 Admin（真实版本）、原 nonce、期限、Key locator／发行摘要，首次原生写前晚检 owner 与原窗口，只有一次发送。专用通道只允许 `0x38` 发、`0x39` 收，通用 Direct 帧不能进入原自动应答。精确 ACK 核对后才给出§7确认材料；尝试发送后不明结果保留 UNKNOWN，不从通用 ACK、网络上线或超时推断安装事实。调用方必须持有独占 client/link 和本次持久 checkpoint；没有新增第二套日志或持久层。
- ESP32 NimBLE 退出保留原唯一 native owner/claim：核 ACL 已退、SDK stop 返回成功、旧 Host 循环返回且旧 task 实际暂停，再由原 SDK 退 task、反初始化。原生结果不明时保持 claim，不能作为交接证据。只对可判定未推进的等待允许原 owner 显式再观察／推进，不自动重新写设备。此证明只覆盖本端 NimBLE，不等于 SDK WiFiProv owner 已经可用，也不把底层 SDK 内部调用描述为独立有界。

有限证据：新增设备 wire 7 组、ESP32 native fake 5 个新增情形、App 9 项和 Gateway 固定向量 1 项通过；必要 DirectSecure／NimBLE Central／App 相邻回归、App 定向类型检查及 architecture 检查通过。App 专用通道的通用帧反例先红后绿，只重测受影响的 6 项、不累计复跑。设备 host 用原 Method 2、分片和内存 Store／确定性 AEAD，App 用实际 Method 2/AEAD 和独立合成 peer；均不是无线或真实 App→Gateway 激活验收。无服务生产源改动、不重跑061／062、不发送真实请求。

原 SDK 定向链接通过：Arduino CLI 1.5.1、ESP32 3.3.11、XIAO ESP32S3 `default_8MB`、上批同一份 PubSubClient 2.8，编译现有 `WiFiBleQuickStart`（Flash 1,288,512 B、静态 RAM 56,684 B）。首次仅缺依赖搜索路径，补原库路径后成功，未安装或升级依赖。该链接验证普通产品实际引用的新 NimBLE 端口，不表示默认产品已挂专用报文或证明真机交接；未烧录。App 源码清单补齐 5 项依赖，原校验器确认 151 个输入路径，未启动隔离交付或远端 CI。

### 13.2 后续仅 BC-4b 产品 owner 与一次 WiFi-only 交接

显式 PreCloudDirect 需在 `stack.open()` 取得真实 instance 后才构造授权桥；仅允许原 Hello、Authenticate、专用 Install 和后续明确的交接操作，不复用会同时启动普通 Direct／Cloud／Task 的 `Client::begin()`。普通配网及冻结产品未挂新入口。原生停止失败或已写而 ACK 丢失时保持隔离和 UNKNOWN，不以重启、等待、广告变化、App close 或已存在 Key 自动进 WiFiProv。

下一批把该 owner 接到实际产品：App 确认同一 intent 后，在仍有效的原加密会话内显式授权一次交接；再各自退休 GATT 与设备 Host，分别验证 App close 与设备原生停止，才进入现有 WiFiProv 的 **WiFi-only** 一次提交。交接命令本批不分配 kind；普通断连不能自动触发。单列旧 WiFiProv 读取里 Admin 版本和 logical ID 的原假设，不以旧 Install／Bootstrap 处理新方向；未确认或错代际时零新写。匹配 App/设备完成后再安排最短真机反向差异。当前无 App 页面／HTTP 挂载、刷板、部署或推送，原授权、TaskStore、UNKNOWN、完整日志及私有 golden HOLD 保持。

## 14 BC-4b：显式 Direct 交接与 WiFi 接纳停点（2026-11-04）

### 14.1 本批完成：候选 Runtime、同 intent 确认及单次交接

- 独占 `PreCloudDirectBinding` 复用原 Hello／Method 2／Direct AEAD；只接受一个 Managed BLE transport 和认证能力。未协商或未认证、普通控制／状态／任务帧、超出协商长度、伪造 Hello 响应都不进入专用 endpoint。保留 App 原有的认证后、专用操作前的加密包长协商；显式停止退休 endpoint 和此 Runtime，不能再启动。不接普通 `Client::begin()` 或冻结产品。
- [精确交接合同](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#81-同意图显式交接仅内部候选) 分配 `0x3A`，设备核原会话、实际 Admin、原 intent／locator 并锁住一次交接；已尝试持久化但失败或 ACK 未入队时，新 Method 2 不会重新获得写权限。允许在已认证并接受命令之后带着一次性本地元组结束 Direct，但元组不代表设备 Host 已停或允许 WiFi 写入。
- App 专用 `submitWifiHandoff` 用内部权威端口提交同 intent 的原回执，严格校对当前 active Cloud 目标、原根和 Key 发行材料；同一加密链路一次发送后关闭 GATT。取消、超期或迟到确认不发；可能已发但结果不明不重试。内部确认端口没有 HTTP／页面挂载，App close 不宣称设备收到指令或 Host 停机。

定向回归覆盖设备 wire 10 组（含本批新增 3 组）、独占 Runtime 两组及相邻授权和 Runtime；App 独立加密 peer 15 项、定向类型检查和源码清单（151 输入路径），设备 architecture／原 freeze 通过。这些是 host 和合成端口结果，不是实际 Gateway 激活、原 SDK 候选完整链接或无线证据。

### 14.2 下一批唯一阻点：写前 WiFi 接纳，不重复认证或数据库回归

核对原 ESP32-S3 3.3.11 `network_provisioning` 头文件与链接归档：`prov-config` 的 `Apply` 先选择 `WIFI_STORAGE_FLASH`、应用 STA 配置，才发送 `NETWORK_PROV_WIFI_CRED_RECV`。旧业务回调无法撤销或拦截原生写；`prov-ctrl` 还能在规定条件下重置状态。原 `Esp32WifiProvAdapter` 会挂通用 `blinker-config`，`HasDeviceKey` 只参与完成判定，不是写前围栏。App 一次调用、一次 journal 写或 `GetInfo` 都不能冒充设备一次接纳。普通 WiFiProv 仍保持不变。

下一批先锁定一个候选专用的**写前**接线：复用原 WiFiProv 安全传输与 WiFi 配置合同，核对能否在 stock Apply 可达前绑定同 intent 的新用途 owner，并在原窗口内消费一次、晚检持久根和 locator；旧 Install／Bootstrap、任意 custom 和状态重置不得对该用途开放。需要实际挂载在 `stack.open()` 后的 ESP32 专用 owner，先退出原生 NimBLE 并验证释放，再以真实 SDK 链接证明所选边界。若原 SDK 无可审查的安全接法，保持入口关闭，单列最小替代方案；不绕过旧 freeze 或把 SDK 事后事件写成写前证据。仅在该门通过后安排 BC-5 短验。

本批未改服务端生产源码、未重新执行实库迁移／配网、未安装 App、部署或刷板；授权、TaskStore、UNKNOWN、完整日志和私有 golden HOLD 保持，无 SDK 升级、暂存／提交／推送。

## 15 BC-4b：候选专用写前门禁（2026-11-04）

### 15.1 已接线的边界

- [写前合同](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#82-候选写前接纳保留-wifiprov-wire不启动-stock-manager)：采用 [Espressif 公开 Protocomm API](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/provisioning/protocomm.html) 和**本机实际 3.3.11** `network_prov_config_data_handler`。新 ESP32-S3 内部 carrier 只注册 `proto-ver`、`prov-session`、`prov-config`，不启动 manager，不启用身份／custom／重置。原 SDK 继续做 Security1 和官方 protobuf 编解码。第一份合法 Set 后封住握手代际，Apply 仅接纳原 native cookie 的一次提交；不按 TCP fd 或 ready 快照代替认证。
- Supervisor 只消费原 `.3A` 的一次元组；ESP32 薄桥复用原 Runtime 停止和原 NimBLE 退 Host 证据。启动 receiver 前及实际 journal 写前都复核原窗口、真实 instance、同一持久根里的 Admin 代际和 Key locator。Set／Apply／Status 从 HTTP 线程只触及锁保护的有界暂存，持久读取与 `WifiCredentialStore` 在原协作 owner 上同步执行；取消、校验失败、写入不明均不重开。没有把 journal 成功显示成 `Connected`。
- 对 Android 原 2.4.4 AAR 的 Set／Apply／GetStatus 做了定向 wire 核对：拒绝嵌入 NUL、越界、额外 targeting 与空/歧义 protobuf。原 AAR 会从空 protobuf 读出默认 `Success/Connected`，不能将 stock 成功回调当权威完成证据；没有放宽当前 App GetInfo 或旧 WiFi-only 门禁。原 SDK 只接受 8–63 B 口令或开放网络；未承诺此路径支持 raw PSK。
- 原 SDK 编译暴露上批专用 Runtime 多占普通 `Device` 存储。专用 binding 改复用互斥的普通状态事务 context，不上调原 1692 B 上限，不改冻结断言或普通配网路径。

有限证据：候选 mailbox／写前 guard／精确 wire、独占 Runtime 和 4 组真实 Method 2 fixture 驱动的 supervisor 回归通过；原 architecture／freeze 检查通过。原 Arduino CLI 1.5.1／ESP32 3.3.11、XIAO ESP32S3 `default_8MB` 完整链接内部探针（Flash 709,304 B，静态 RAM 32,296 B），ELF 包含 `protocomm_httpd_start`、`network_prov_config_data_handler`、原 NimBLE 退休桥和 `WifiCredentialStore::commit`，不含 stock `network_prov_mgr_*`。探针默认不启动网络或触碰持久区，编译不证明物理交接或配网成功。未改 App 或 Gateway 源码，未部署、烧录、升级、提交或推送；授权、TaskStore、UNKNOWN 及日志保留。

### 15.2 仍关闭的产品入口与下一步

当前是内部 transport／门禁／原 journal 薄桥，未在 `stack.open()` 后启动真实产品 owner。下一批只补最短调用路径：把同 intent 的独立临时 AP 口令与 PoP 接入匹配 App 和设备独占 owner，并在无 STA／LAN 时启 listener、确认关闭后再沿原 STA 路径运行。复用既有 UNKNOWN／新鲜 Cloud 只读确认，不将 `Apply` 排队当 WiFi 成功；App 只解析固定带类型回执，不接回旧 GetInfo、Bootstrap 或 arbitrary custom。真实 SoftAP 启停与 Android 回执达标后才进入 BC-5，不重跑 BC-3、普通配网或 App 大矩阵。

## 16 BC-4b：独立 SoftAP 凭据与候选接收顺序（2026-11-04）

- [§8.3 临时凭据合同](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#83-同意图-softap-临时凭据与退出顺序)：从同一原 Admin、实际 Method 2 binding、instance／intent／Key locator 用三个域派生 SSID、WPA2 和 PoP。设备重读原 Admin，启动回调后清 binding；App 仅在原 commit、`.3A`、本地 GATT 释放后向显式回调提供一次，完成、取消或到期清掉临时口令。无原密钥或临时口令写入持久账本，原 BBP/2 不变。
- 设备 `Esp32PreCloudWifiOnlyReceiver` 接已有监督器。只在原生 BLE 已退出、无旧 WiFi driver 和网络接口时启动候选 AP；先设置 WPA2／CCMP、单连接与 RAM 配置再启动，AP-only、DHCP 和 `192.168.4.1` 均正确才挂窄 HTTP。首笔协作 journal 写后按 HTTP→AP/driver/netif 退出，只有实际释放且已提交才允许原 STA 启动一次。原 STA 生命周期继续单独判断联网，未知退出阻断交接。候选不用普通 `softAP()`，避免先启动默认热点再下发保护配置。
- 插件包内新增非公开 Android 适配器，复用锁定 SDK 的 SoftAPTransport/Security1 Session，以实际 AP／绑定网络和原 owner 检查围住一次 Set／Apply，只接受对应有类型回执，不调用 SDK 自动状态流程或旧 GetInfo／Bootstrap／任意 custom。Set 未确认不发送 Apply；Apply 可能送达但回执／路由丢失保持 UNKNOWN，不重发。即便收到 `Apply queued` 也不表示 WiFi 或 Cloud 成功。

定向离线：C++ 固定向量及实际 Method 2 handoff→当前 Admin（2 组），相邻监督器（4 组）及 architecture；App 19 项、定向类型检查和源码选择器（152 路径）；Android 候选 7 项、实际锁定 2.4.4 AAR 的定向 Java 编译及本地插件 Gradle `compileDebugJavaWithJavac` 均通过。未构建 APK，无旧配网矩阵、设备任务或 Gateway 变更。

原 Arduino CLI 1.5.1／ESP32 3.3.11、XIAO ESP32S3 `default_8MB` 的内部探针完整链接通过：Flash 1,138,720 B，静态 RAM 48,036 B。确认 ELF 包含候选 AP attach／release、原生 NimBLE 退休、窄 HTTP/codec、原 journal 与 `WifiConnectionLifecycle::start`；未链接 manager init/start，原 Arduino STA 带来的历史 manager deinit 事件处理不由候选启动。探针默认不启用、不烧录；未升级 SDK、部署或提交推送，授权、TaskStore、UNKNOWN 和完整日志保持。

### 16.1 最短剩余调用路径

尚无 App 页面、插件 IPC/公开命令或设备默认启动入口。下一批只挂**显式**完整候选：App 的临时回调进入独占 Android worker（证明精确 AP 网络，保留原 Cloud/Local 隔离及退出证据），设备在现有 `stack.open()` 后挂专用 Runtime、原 TaskStore owner 和本 receiver，不替换普通 WiFiProv。每次 receiver poll 前由原协作 owner 推进监督器，原 setup 窗口到期先取消和清理，再决定是否允许任何操作。先冻结两端候选并核对单次启动与恢复条件，再在 BC-5 短验分别记录官方 Apply、STA 和新鲜 Cloud。若离线或现场不能核实 worker/设备释放，保持关闭，不把独立 adapter／链接或 GATT 关闭当作已验整链。

## 17 BC-4b：显式 worker 与设备 owner 接线（2026-11-04）

- **App/Android**：只读冷预检 capability／精确权限／旧网络占用，进 Local 前重验。新 `blinker-initial-cloud` 用途沿同一 `candidateId` 关联独占 worker 的创建、连接、一次 WiFi 提交和关闭；候选用系统 `WifiNetworkSpecifier` 申请精确 AP，避开原锁定 AAR 中会打印 WPA2 口令的连接重载。SDK Session／Security1 保持，版本探测和固定提交均绑定精确 `Network`，不回落默认路由，不开放其它命令。`initialCloudWifiCandidate` 默认关闭，无页面或普通配网入口变更。只有本候选的 Binder 死亡，或确知该候选没有启动，才释放 Cloud fence；不以当前无 worker 或等待代替证据。
- **设备**：仅显式 ESP32-S3 四槽 TLS 内部构建选择 `Esp32PreCloudWifiProduct`。先查原物理 instance、原 BLE Admin、授权 Presence、无 Key；`stack.open()` 后接原受限 Runtime 和原 TaskStore 安装器，不伪造 Unbound，不清任务／scope。原 setup/Method 2 截止穿到实际 Key 写前；已进入原生退出时不再轮询 Direct。原 BLE／HTTP／AP 均释放且 journal 已确认后才 attach 和启动原 STA/Cloud lifecycle。候选不启用普通 BLE 控制或任务管理，不代签默认产品。
- **结果**：显式 App composition 先核安装回执与不可变发行目标，再关联退出证明和 native submit 的时间下界；只复用一次原 owner-only `provisioning-cloud-v1` 查询，按目标 Key／instance／新会话判定。保留原 Apply queued／UNKNOWN，不借 Cloud online 回填官方回执；旧会话、换账号和关闭未确认均不能冒认成功。细节取[合同 §8.4](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#84-显式调用方local-退出与-cloud-确认)。

有限回归：App 当前 carrier／observation 18 项、匹配兄弟插件源码的定向 TypeScript 和源码清单（156 项）通过；相邻安装合同 19 项在本批组合初测通过，未重跑旧配网矩阵。设备原持久存储／监督器 5 组及 architecture/freeze 通过，其中新增反例在实际写前跨原 setup 边界，核无 Key 或 ACK。Cloud 观察只读审阅核实原 Gateway 按目标 ID 查询、核当前 owner/tenant/Key locator 和实际 Broker session 的 instance，不借实例映射选旧 BLE 卡。

原 Arduino CLI 1.5.1／ESP32 3.3.11／PubSubClient 2.8，XIAO ESP32S3 `default_8MB` 显式产品探针完整链接通过：Flash 1,384,128 B、静态 RAM 60,620 B。ELF 包含实际产品 start/poll/cleanup、四槽原 guarded installer／`TaskSnapshotStore::observeBinding`、setup-bound Method 2、Protocomm 与原 `WifiConnectionLifecycle::start`，不包含 stock manager init/start/register。该探针用不可达 `.invalid`／合成公钥，默认不启动、未烧录，不表示真实 profile、发行或物理交接已验。

Android 最终版本使用原锁定 WiFiProv 2.4.4，离线 Gradle 插件 Java 编译通过。定向 adapter／bound HTTP 8 项、worker 2 项、Client 退出证明 1 项、Plugin 路由 1 项，插件 TypeScript 与 Web/iOS 平台源 13 项均通过；退出测试为 host 合同，不代替 Android 真机 Binder／WiFi 验收。旧 SDK 含口令日志的重载不进入候选，未安装 APK、部署、烧录、升级、提交或推送。原授权、TaskStore、UNKNOWN 和日志保持。

### 17.1 BC-5 准备门（尚未执行）

1. 用相同源码准备显式测试 APK 配置和真实设备候选：本轮探针只用于链接，服务地址／密钥夹具不可当作可烧录配置。只选原 BLE 身份／当前 Admin／Presence、真实 Unbound 的独立测试目标，不清根或挪旧 scope。核本机服务版本与 061／062、实际 broker／profile 和签发材料。
2. App 目前通过注入的权威端口消费原 `prepare/issue/withIssuedKey/commit` 合同，Gateway 尚无对应公开 HTTP 路由。BC-5 要先具备已批准的同账号、同 intent、可保近期认证的实际内部入口；不借旧 `enable-cloud`、通用 Key reveal 或合成确认替代。正常 Cloud 只读观察已可复用，不为真机开默认接口。
3. 冷预检通过才做一次新 intent。分别保留 Key 持久 ACK、匹配 Android worker 退出、带类型 Apply 或 UNKNOWN、设备 STA、Broker/新鲜 Cloud State，随后核两个独立卡和原根。不能发写即停在相应阶段，不重放 `.3A` 或 WiFi，不重跑已通过矩阵。

## 18 BC-5 冷预检、构建准入与内部同 intent 入口（2026-11-04）

- **服务端**：新增进程内 `createCloudRegistrationAcceptance`，默认不实例化、不挂 HTTP。构造时固定批准的账号／tenant／区域、原 BLE 源、根、一个新 intent 和绝对期限；每次操作从实际身份解析端口验原 owner，不能从 body 冒认。冷预检复用原 source CAS、pending intent 和 quota，在原锁下读而不创建 intent、不调额度、不生成／解密 Key。准备、发行和借用仍经原近期认证；发行和借用在实际 Key 边界晚检。同步借用进程内一次，返回只含原元数据，原 Key 归零；回调异常和进程重建不能重用旧借出入口。晚到精确确认和 UNKNOWN 沿原 repository，不从临时批准过期推断设备未写。新增 6 项、必要相邻 21 项 Node 回归通过（实际 repository/vault/lease＋合成端口，不是本轮线上 Auth、Redis、PostgreSQL 或无线验收）。
- **设备**：新增[独立候选及原渲染器配方](DEVICE_V2_BC5_DEVICE_CANDIDATE.md)。内部 sketch 拒绝无正式形状 profile 或 `.invalid` 探针；上电不开放，另行批准后仅一次显式 USB 开窗。原发布白名单／profile 干验和 architecture 通过，未用历史 profile 构建可刷固件。只读 USB 仅见 COM11；[最近可证状态](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#146-匹配固件及两卡有限真机结果)保留 Cloud 卡／Key、BLE 卡已删除，不是可推定的 BLE-only／真实 Unbound。未开串口、读写 NVS、复位或刷板。
- **App**：新增 `BLINKER_BC5_CANDIDATE=1` 明示 Beta 打包门，普通 Beta/Debug/Release 不会采用错配或残留候选 assets。3 项离线测试、Gradle 正反例、160 项源码输入通过，未同步／构建 APK。只读核实当前安装内测 APK 未开启开关、无新原生命令；已安装 file 依赖的生成声明仍旧，待真实调用方就绪后按原锁重建。手机权限和定位满足冷条件，实际验收需解锁并在内测前台，不先发码。
- **Android**：`proto-ver` 探测与 Security1 沿用同一绑定 `Network`、HTTP carrier 和 Cookie，避免 SDK keep-alive 不重发 `Set-Cookie` 时误拒同一设备。仅候选路径修改；无 Cookie 重发的定向 fixture 和相邻 worker 共 3 项、原 SDK 2.4.4 Gradle 编译通过。这是 host/编译证据，不是现场已复现或 Binder/WiFi 真机证据。

### 18.1 实际写入前仍需什么

1. **合格独立源**：提供原 BLE 身份、当前 Admin／Presence、无 Key 及真实 TaskStore Unbound 的测试板；不能清 COM11 原根或任务凑样本。
2. **当前真实测试环境及调用入口**：本机 Docker engine 不在线，本批未连远端、未验证现网 061／062 或发布配置。先核已批准的实际服务迁移、TLS/Broker 和签发公钥，给内测 App 接获准的同 intent 私有调用方，保持原近期认证及 Key 保密。进程内门禁已实现，但不是网络路由或手机可直接访问的服务；未用假确认或旧 Key reveal 填空。
3. 条件齐备后才冻结本批可刷 profile、重建匹配锁定插件及显式 Beta APK，并做一次新 intent 的 §17.1 短验。现在尚未安装 APK、刷板、发验证码、发出配网或发行请求、运行迁移、部署或提交推送；原授权、TaskStore、UNKNOWN 及完整日志保留。

## 19 BC-5 受限 App↔Gateway 传输及真实前置核对（2026-11-04）

- Gateway 候选只接固定 `POST /api/v2/internal/bc5/<批准的 intent>/<action>` 和同一路径的窄 CORS preflight，未显式配置时在普通路由／通用 CORS 前返回 404。私有配置固定账号／tenant／区域、原 BLE 源与根、一个 intent、批准截止、明确的 TLS 代理地址和仅代理可注入的高熵入口标记。真实身份、近期认证和原 repository 仍是发行／借用权威；错误代理／标记／intent 无权触发认证或借 Key。Redis 只使用该 intent 专用限流窗口，不占普通账号／租户预算，Redis 不可用则拒绝；响应不缓存、不记录 token、正文或 Key。
- `/key` 只接受本次 App 的临时 X25519 公钥。用原 `withIssuedKey` 同步借用同一 Key，立即用 X25519/HKDF-SHA256 和 AES-256-GCM 封给该接收方，AAD 绑定原 intent 和双方公钥；只在原事务完成后传密文。坏公钥在借用前拒绝，事务未知不出密文，不替换 Key、不重借。App 私有 HTTPS client 检当前账号／区域、不可变目标／根／nonce 和精确响应，解封后核原 Key locator／发行摘要并清借用缓冲；无默认 URL、认证刷新重放或普通 reveal/enable-cloud 回退。实际 App 页面／radio caller 和持久写前 checkpoint 尚未挂；不能以提前标记 `RESULT_UNKNOWN` 或 v1 checkpoint 冒充本方向的写前意图。
- Gateway 定向 10 项通过（6 项原进程内合同＋4 项回环 HTTP、合成 TLS 代理证据与独立 WebCrypto 解封）；App 独立 Node 加密对端 5 项、定向 TypeScript 和源码选择器 162 路径通过。源码／本机结果不代签真实 TLS 入口、Auth、现场 Redis/PG、签发公钥或运行版本。

实际前置：USB 仍只识别 COM11；没有新证据推翻保留 Cloud Key 且 BLE 卡已移除的最近状态，不清根、不读写设备存储、不刷板。本机 Docker engine 未运行；Development 只切原 Auth 区域，现有公用 `iot.yiyu.pro` 不是已批准的私有 BC-5 入口。未启用真实私有配置，未连接远端账号、库或设备，**现网 061／062、Broker、TLS 与签发匹配未核实**，故不构建可刷/可装制品、不部署、不开默认 HTTP、不发出真实发行或配网请求。

后续最小范围：确认合格 BLE-only／无 Key／TaskStore 真实 Unbound 源板和明确的私有测试 TLS 环境，再按[私有入口配方](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_BC5_PRIVATE_ACCEPTANCE.md)核配置与真实 Auth。将专用 App 调用和可恢复非秘密 checkpoint 接入后，冻结匹配 APK／固件，只做一次 §17.1 短验。授权、TaskStore、UNKNOWN及完整日志保留；不重放、不提交推送。

## 20 BC-5：原生写前检查点与冷启动只读协调（2026-11-04）

- **事实分开**：受限客户端在拿到明确的 `key_issued` 元数据后，先用原生原子比较写入占用一次本地尝试，再借 Key；原 Method 2 认证完成后，在实际 `.38` 写入前原子推进 `armedAt`。两次都等待同步落盘确认，写失败、同 intent 并发、账号／owner 变化、取消或最短截止到达，均不能发设备写入。`armedAt` 只表示写前准备完成，不表示已经发送、设备已安装或产生 `RESULT_UNKNOWN`。原服务提交仅接受本次 live 安装返回的确认摘要和原 `commitId`；不把 ACK、原生退出、WiFi Apply 和 Cloud 可用性合并。
- **隔离持久化**：原 SecureStorage Android 实现使用异步 `SharedPreferences.apply()`，未作为写入门禁。显式 HTTPS Beta 才注册新原生桥；普通 Beta、Debug、Release 不开放。原生用进程内比较与检查 `commit()` 结果，失败后不信内存缓存继续写；每账号/环境 32 条、全文件 128 条、单值 4096 个 UTF-16 单元，保留未确认项、不按年龄淘汰。冷预检先核专用存储可读且原 intent 无记录，否则不发服务请求。检查点只含原 intent、冻结发行元数据、`commitId` 和本地时点，不含明文 Key、Admin secret、token、原 ACK／session binding 或临时 WiFi 凭据；不触碰旧 v1 或请求日志。Beta 的云备份／设备迁移仅排除该专用文件。
- **恢复**：重启后只按原 TLS origin／区域／租户／账号／intent 显式读取，并核对冻结目标、原根、nonce 和发行材料；不恢复写权限，不借 Key，不重发 `.38`、`.3A` 或 WiFi，不自动调用 `/mark-unknown`。`key_issued`／`install_unknown` 仍为未确认；准确匹配的 active `committed` 只表示服务端激活，不表示原 ACK 重新取得或设备在线。404、错代际、已删除目标或入口停用保留记录，不据此判失败或清理。App 只发 read；服务原 `coordinate()` 仍可按既有合同维护过期／根变化状态。本批仅覆盖已明确发行之后的安装检查点，不补造 prepare／issue 回执丢失的记录。

本批没有挂正常 App 页面、默认 HTTP、账号或物理 radio 入口。现场只识别 COM11，本机 Docker Linux 引擎仍不可用；未核实真实私有入口、服务配置或合格源板，未部署、构建 APK／固件、刷板、清根、发码、重放或提交推送。后续在真实前置齐备后，将此显式候选接到隔离验收页并复用 §17 的 handoff／Cloud 观察，冻结一次匹配制品并完成一次短验；不替换普通配网链路。

有限验证：App 两个定向套件 30 项、作用域 TypeScript、Profile／匹配 CI 计划 11 项和 169 条源码选择通过；原 SDK 36／现有 Capacitor 和 WiFiProv 缓存依赖定向 Java 编译、原生 checkpoint JUnit 6 项及 Beta 两个 XML 资源离线编译通过。覆盖持久确认延迟、原子争用、存储失败、账号和截止晚检、冷读不重发及发行/终态错配。原生 JUnit 用独立内存存储模拟失败和重开，不是手机进程冷杀／持久盘验证；未生成 APK 或请求真机。

## 21 BC-5：隔离验收入口与 COM3 现场预检（2026-11-04）

- **App 执行入口已接上**：显式 Beta 用 `BLINKER_BC5_CANDIDATE=1` 和绝对路径 `BLINKER_BC5_ADMISSION_FILE` 打包固定单 intent 的非秘密配置；原生注册表、生成资产与 Gradle 请求必须一致。默认 Beta 和正式包不可进入 `/tools/bc5-acceptance`。该页只接受本次目标 Wi-Fi，不能填写或从 URL 切换网关；明确分开“开始一次”和“只读查询”。复用原 BLE 独占 owner、当前账号/区域、原 Presence 选择、签名回执、WiFiProv 和匹配 worker 关闭后的新鲜 Cloud 查询。离页／后台停止，未知不重发。普通 Web 网关与候选 origin 必须一致；若不是普通 Beta 的既有 HTTPS origin，显式候选的 Web 构建通过 Angular `--define BLINKER_BC5_GATEWAY_ORIGIN=<JSON 字符串>` 指向同一已批准地址，默认构建不传。
- **板已到位**：COM3 的实际 ESP32-S3 与 8 MiB Flash 通过 ROM 只读核对，且与 COM11 区分；读取实际分区后，仅解析原 20 KiB NVS，见无 DeviceKey、1 个 Admin、有 Presence，原任务无快照／0 条。原解析器的 `Empty` 是结构证据，最终仍由候选运行时确认 `Unbound`；是否与当前 App 的原 BLE 卡和 Admin 相符待服务／App 侧核对。不重认领或清根凑样本。标准 BLE QuickStart 原 CLI 1.5.1／ESP32 3.3.11、8 MiB 分区完整链接（Flash 699,392 B，静态 RAM 36,032 B），默认仓库验签材料只作编译证据，未烧录。COM3 只读探测结束恢复正常启动；COM11 未动。
- **部署不要求新域名或新 Broker**：可沿用现有 HTTPS 主机和匹配的 MQTT 服务；只隔离临时 BC-5 路径、访问范围及单 intent。直连 `https://129.204.34.159` 当前证书不匹配，带 `iot.yiyu.pro` 主机名直达 129 的 HTTPS 证书校验通过。裸 IP 不是产品禁令，配对应 IP 证书才可用；不关 TLS 校验、不把真实登录凭据走公网明文。对 129 SSH 只读核对：现有 Gateway／Broker 健康，实际运行数据库迁移只到 `060`，没有 BC-5 配置；Gateway P256 签发材料可读、keyId `1`，无 Ed25519 签发配置，公钥指纹已只读取得，尚未用于匹配固件。未修改远端服务或数据库。

验证：三组定向 App 回归 47 项、Angular 模板／TypeScript、25 项 Profile 和 CI 计划及 174 条源码选择通过；新原生桥按当前 SDK 36 定向 Java 编译通过。没有 APK、部署、Flash 写入或 Git 推送。**停止追加离线子阶段**：下一次只确认现有 129 IoT 测试实例是否可在保留原数据下补 061／062 和匹配 Gateway、核同账号 BLE 卡及实际签发公钥，随后在现有 HTTPS 下临时开放单 intent、一次构建部署、一次 COM3 短验；原来的已通过矩阵不重跑。

## 22 BC-5：129 匹配部署及 COM3 独立测试认领（2026-11-04）

- 129 先核验 Gateway／Nginx 原配置、完整旧 Gateway 日志及 PostgreSQL custom dump（`pg_restore --list` 可读）。在原运行镜像叠加 22 个逐文件核对的 Gateway 编译消费者；原编译器、32 项定向回归通过。已部署，仅 Gateway 重建，迁移历史实际完整至 `062`（本批 061／062），新实例健康。其它 22 个容器、现有 Broker、Auth、数据卷、Gateway 原环境/挂载/端口及 Nginx 不变。直接和 HTTPS 两个 BC-5 未批准路径均验证 404，没有公网默认开放或业务重放。
- 原 COM3 实例/原 Admin 指纹与 129 的现役、历史认领和当前 Admin 三处匹配均为 0；当前手机在 Development，存在原会话，但无可复用 BLE-only Admin 卡。经用户授权，后续仅 COM3 作为可覆盖测试板走正常空根 BLE 认领，不做旧板备份；COM11、旧云端记录及 App 原日志不动。原 1.25 MiB app 槽装不下本批 1,388,272 B BC-5 候选，先用同原 SDK 的 8 MiB BLE 首装镜像选择新分区，认领完成后只换新 app 槽，保留刚建立的 NVS／授权。
- 实际 P256 `keyId=1` 与公开 profile 的公钥指纹匹配；固件原 CA 对 `iot.yiyu.pro:443` 与 `mqtt.yiyu.pro:8883` 两条 129 TLS 都通过。BC-5 运行固件原 SDK 链接及实际产品/无 stock manager 符号通过。普通 BLE 认领仅需限测 HTTPS 路径，不另起 Broker，手机网络源经已知 HTTPS 无鉴权探针单列核验。
- COM3 已按授权只清原 20 KiB NVS 并写入/校验 8 MiB 首装镜像，不备份或恢复被授权覆盖的旧板数据。真实内测 App 在 Development 下完成一次原生 BLE 认领、关闭原 owner；目录与实际 129 记录均为 active、本人拥有、BLE-only、无 Cloud Key 历史，服务实际签发算法 2／keyId 1，Presence 已确认。重新只读核实实例、Admin、Presence 与板内一致后，仅将 app0 换成 BC-5 运行 BIN，读回校验成功；**新 NVS／授权不变**。第一次校验器的 Presence 文本编码不一致在写入前停止，按同一 Base64URL 合同修正后才写 app0；没有重复 BLE 认领。临时普通认领路由已恢复，HTTPS 再次 404。

测试服务器常规入口仍可供同事使用。此处关闭的是固定账号／源板／单 intent 的运维验收入口；它不是多人功能入口。后续真实验收通过再按已有正常账号、设备授权和明确准入开放产品路径，不让同事使用固定测试 intent 或运维标记。

### 22.1 匹配内测安装及实际签发门禁

Web 复用已通过的设备模板，面板构建以锁定兄弟 WiFiProv 源码消除旧安装声明错配；未重跑设备模板。现有 Android SDK／WiFiProv 2.4.4、原签名、HTTPS 及 APK 中 1,163 项匹配 Web 资产核验通过，已 `adb install -r`，同一 Development 账号及授权在位。实机已打开专用页，原生准入范围与新 COM3 源卡一致，尚未点击开始。

129 仅对本次单 intent／手机实际公网来源启用入口，同一候选 Gateway 镜像、Broker 及其它 22 个容器不变。实际 CORS 204、正确受限路径未登录 401、错 intent／直连 404；未跳过 TLS 或修改默认产品路由。真实当前 App token 仅在手机计算摘要，129 Redis 对其近期认证 `PTTL=-2`（未建立该会话证明）；无原验证码、token、Key 或 WiFi 凭据出现在证据中。等待同账号正常近期登录，期间未开启 COM3 的物理窗口、未下发 Key 或 WiFi。

### 22.2 真机一次结果、最小修复及收尾

- 用户正常同账号重新登录，已核新的近期认证证明确实生效。仅 COM3 一次 USB 显式开窗（`BC5_OWNER_OPEN_RESULT=0`），真实 App 单次开始：`preflight/prepare/issue/key` 四项均返回 HTTP 200，CORS OPTIONS 204；然后停在 BLE 安装，无 `commit`、无 WiFi 提交、无 Cloud 上线。真机 GATT 连接/发现成功，收到两段通知后释放；没有自动重试。
- App 原生检查点存在、`armedAt` 为空；COM3 精确只读复核新实例、Admin、Presence 不变，仍无 DeviceKey 和 TaskStore 快照。已确定本次停在设备安装写入前；129 原 intent 真实状态为 `key_issued`（有发行、无 commit、无目标卡），按合同保留，不取消、不改库、不重借。临时 Nginx／Gateway 准入已撤，公网及直连均恢复 404；保留 061／062、新 BLE 卡、App 账号、请求日志和原发行记录，COM11／Broker／其它 22 个容器不动。
- 定位到 BC-5 专用设备 Hello 仅 `0x20` 和 Method 2，旧 App 却要求普通 Direct 的 Manifest／端点／可靠／状态能力。用实际候选 Hello 换掉原普通模拟报文后，稳定在同一阶段报 `DIRECT_DEVICE_HELLO_INVALID`，没有借现场日志冒称已捕获该内部异常。已修复为仅 BC-5 使用严格认证专用 Hello，覆盖首次和加密 ATT 协商，普通 Direct／LAN 不放宽；35 项定向回归和 TypeScript 通过。当前手机仍安装本次真机所用 APK，修复版尚未重建或上板复测；同一固件可复用。

后续只处理这个已知停点：先按现合同处置旧已发行记录（不直接释放或重放；若需全新独立测试源，明确独立准入），再构建修复 App 做一次差异。不要重新部署数据库、重复普通 BLE 首装以外的已过链路，或关闭实际设备验签。5 分钟证明只约束 DeviceKey 敏感操作，不是登录有效期；对内测用户优先设计 App 内同账号即时验证，而不是让用户反复退出登录或全局取消 Key 门禁。

## 23 BC-5：独立新身份的 Hello 修复差异验收（2026-11-04）

- 129 原 intent 用 `BEGIN READ ONLY`／`SELECT`／`ROLLBACK` 精确核对：schema 062，窗口已过期，仍 `key_issued`，无提交、无目标云卡，同 BLE 源占用 1。未重新开放旧入口，不重借、不取消、不直接改库释放。
- 经用户授权，只对 COM3 读回并核验原完整 20 KiB NVS，在受保护目录保留副本；旧板根与原 App 卡匹配。随后仅临时清同一 NVS、写入既有 BLE 首装 app0。通过真实 App 在同 Development 账号一次正常认领了**不同物理实例与根、独立逻辑 ID**，旧 App 卡与原服务记录保持。真实 App `完成` 后原生连接为 0，读板确认新实例、Admin、Presence 一致，设备无 Key／任务快照，再仅写回既有 BC-5 app0 并校验；无固件重编，无 COM11 操作。临时认领路由已撤。
- 只重建修复版主 Web 和匹配 APK，模板／面板按旧哈希复用；APK 内专用 `precloud-controller`、1,163 项资产、原签名及 WiFiProv 2.4.4 通过检查，已覆盖安装并保留 App 数据。129 沿用 schema 062、同一 Gateway 镜像、Broker 和原测试域名，临时仅新源／新 intent／手机实际来源可达；CORS 204、未认证 401、错 intent／直连 404，其他 22 个容器未变。只待同账号近期认证后开启一次 COM3 窗口。
- 窗口收尾：手机只读核对仍为原会话，本轮没有新的近期登录确认。新 intent 真实数据库只读查询为不存在；未开启 COM3 物理窗口、未发新 Key、未发送 WiFi，安装 ACK／WiFi／云上线均未复测。已撤临时范围与入口标记，公网和直连恢复 404，其他 22 个容器不变。保留新 COM3 BLE 身份、现有固件、原 NVS 私有备份、已安装修复 App 及全部历史；后续只更新单次准入及匹配包的准入配置，既有 Web、独立身份、固件和数据库不重做，同账号近期验证后直接完成一次短验。

## 24 BC-5：复用 COM3／Web 的一次写前预检（2026-11-04）

- 只读核对 COM3 仍为 §23 新实例及原 Admin／Presence，未安装 Key，未初始化任务；不重编固件、不重刷、不重复认领。129 先只读核 §23 原准入已关闭且无 intent 记录，再签入新单 intent，沿用 Gateway／Broker 和 schema 062；CORS 204、未认证 401、错 intent 与直连 404。沿用同一套 Hello 修复 Web 和锁定 WiFiProv，只更新匹配准入配置、覆盖安装，保留同账号和 App 数据。
- COM3 显式开窗一次，真实 App 首次只走到 `preflight`，HTTP 403；129 对该会话的近期证明在这次预检时已过期。没有 `prepare`、`issue`、`key` 或后续请求；新 intent 精确只读查询为 0 行，App 原生安装 checkpoint 不存在。不能据此判定 Hello 修复已通过、配网失败或设备已上线。旧 `key_issued` 保留，不重借、不直接释放。
- 未再次发起请求。已撤销仅本轮的 Nginx 路由与 Gateway 准入，公网及直连再次确认 404，其余 22 个容器、schema 062 和数据库保留。COM3 未再复位或刷写，开窗按设备原时限自然结束；保留当前 BLE 身份、私有 NVS 备份、App 账号及完整历史。下一批优先按 §24.1 去掉加云对登录时点的依赖，再复用现有 COM3／Web 做一笔全新差异验收；不重复认领或发旧请求。

### 24.1 后续产品项：取消加云对「登录后 5 分钟」的固定限制

按用户要求单列：正常内测加云不应依赖在登录后 5 分钟内完成操作。拟将固定登录时间门槛替换成与当前有效登录、所有者、物理设备和单个 intent 绑定的操作级授权；遇到会话或风险变化时在当前流程验证，不要求用户退出重登。保留 DeviceKey 加密交付、一次借用、根 CAS、旧 UNKNOWN 禁止重放。先区分加云内部交付与明文 Key 查看／轮换的风险，不能因去除固定时间限制而开放通用凭据读取。本轮真机仍沿现有门禁，不临时旁路或部署此项。

## 25 BC-5：用当前会话授权替代固定登录时间（2026-11-04）

- 只改私有加云组合。以固定批准的 owner／tenant／region／BLE 源物理根／新 intent 建立授权；`preflight` 不写入，只在同一进程记住已核验 bearer 的摘要。`prepare → issue → key` 各阶段重新向现有 Auth 验证、不允许缓存回退，拒绝同账号换会话接管、乱序、重复借用、请求取消及本次窗口过期。原根、设备与额度仍由数据库锁内 CAS 校验，原准备、发行和借用期限不延长；设备的 Method 2 安装确认合同不变。会话重新认证不能重发旧操作；已发行结果仍允许当前同一 owner 按原 intent 协调或提交精确回执。明文 Key 读取和其他入口的认证不变。
- 保留 HTTPS 传输、一次临时接收公钥、同步借用后清零及不返回明文。原锁定 Gateway 工具链完整编译；74 项定向回归通过，覆盖无近期标记依赖、跨会话／撤销会话、根 CAS、过期和取消、一次借用及旧 UNKNOWN 协调。仅四个编译消费者改变，129 原配置和日志留存后只更新 Gateway；schema 062 无迁移，其他 22 个容器不动，默认入口公网和直连仍为 404。
- 真机沿用 COM3 原固件／新鲜只读核对 Admin、Presence 和空任务区，无认领或刷写。App 仅原生重打包匹配新 intent（原签名、WiFiProv 2.4.4、1,163 项 Web 资产匹配），覆盖安装后 bearer 摘要与安装前相同，服务器对该摘要的近期标记为缺失（`PTTL=-2`）。已核当前手机来源，临时路由仅匹配本 intent；未认证 401、OPTIONS 204、错误 intent 和直连 404。物理窗口开启一次，没有要求重新登录。
- 真实 App 单次开始，`preflight → prepare → issue → key → commit` 各一次且均 200。App 按原 Method 2 验证设备持久 Key 安装 ACK，原生非秘密 checkpoint 已标记；数据库确认 `committed`、新 Cloud 卡 active、源 BLE 卡仍 active 且 BLE-only，无该源待决发行。进入 `wifi` 后约 434 ms，App 停在 `unconfirmed`；手机实际前台为 OEM 的 `NetworkRequestDialogActivity`。没有正式 WiFi Apply 回执或 Cloud 观察请求，独立只读查询无匹配的 active TLS 设备会话。不能把卡激活算成 WiFi 成功或设备在线，也不根据此推断 WiFi 写入一定未发生。
- 有界 Android 时序补证：系统创建选网 Activity 后约 1 ms 请求暂停 Beta，Beta 的首个实际 stop 晚约 82 s，不在 App 快速终态区间。现场只有一个符合候选形状的热点选择项，未选择或确认。证据指向系统选网遮挡期间的交接，不等同于已定位具体取消分支；没有可证的原生 Submit/Apply 调用。
- 已关闭本轮 Nginx/Gateway 临时范围并删除本轮标记，公网及直连均恢复 404；新版 Gateway 留在测试环境，其他 22 个容器不变。保留 COM3 现有 Key、原授权、已激活目标、原始请求和旧 issued/UNKNOWN，不重发 Key 或 WiFi、不回写 NVS、不提交推送。下一批只定位 BC-5 专用 SoftAP owner 与系统授权遮挡的前后台交接，复用既有 WiFiProv 生命周期证据；现有已提交 intent 不用于重跑，另行决定保根、WiFi-only 的恢复入口。

## 26 BC-5：系统选网遮挡、原连接继续和恢复边界（2026-11-04）

- **定位**：§25 Android 系统先启动 OEM 选网界面，约 1 ms 后发出 Beta pause，真实 stop 晚约 82 s；App 在 Wi-Fi 阶段约 434 ms 即落入未确认。`AppVisibilityService` 在 pause 即发出 `active=false`；BC-5 原来直接取消整个 BLE 注册操作，连带撤销已开始的隔离 SoftAP owner。原生 WiFiProv 原本只在真实 stop 强制回收。日志和代码可定位这个误取消路径，但没有本次 native Submit/Apply 记录，不能反推“设备肯定没收到”。
- **最小修复**：只给 BC-5 在 GATT 已释放、匹配隔离 owner 已创建且唯一 `connect` 已发起之后，复用普通配网的 `WiFiProvSoftApAcquisition` 保留同一连接。遮挡中只等待原结果，不提交、不恢复 Cloud；前台回来也不会发送。确认相同 native owner、SSID、安全模式和账户后，由用户明确点「确认并发送 Wi-Fi 配置」；现有固定 native Submit 在 Wi-Fi 写前重查实际绑定 Network、版本和 Security1，候选不新增通用 `getDeviceInfo`。原截止、一次写、匹配 Binder 死亡才释放 Cloud 围栏不变。真实 stop、离页、换账号、截止及普通 BLE 资源回收不放宽。
- **恢复准入**：原连接的明确继续只发生在同一次尚活跃的操作，且此前未提交。已终止的 intent／热点凭据／App checkpoint 都不能重启写链。COM3 现固件是专用首次接入版本，Key 已安装后 `checkBleOnlyRoot` 拒绝开窗；原 Wi-Fi handoff 最长 120 秒，App 临时凭据已销毁，checkpoint 只读，ELF 无普通 WiFiProv manager 启动入口。要独立恢复，必须有匹配固件支持对已有根发起新的、用户显式授权的 Wi-Fi-only 操作，重新认证当前 Admin、物理 instance／epoch／根及已激活 Cloud 目标；先权威协调旧 Apply，不能凭无回执或未上线认定未发。旧结果仍未知时不补发原请求。本批不刷板、不重装 Key，因此不具备真机续写条件；已保留原授权、Cloud 卡、UNKNOWN 和完整日志。
- **验收边界**：普通 BLE 绑定，以及刷普通 WiFi 固件后走常规 App 的 WiFiProv BLE／SoftAP 添加，都是常规产品验收，走原页面，不应使用此内部工具。BC-5 另验的是不清原 BLE 根，在同一物理设备上新增独立 Cloud 卡。内部页用于在默认入口尚未开放时约束一笔试验，不是对用户的新入口，也不代替普通绑定。后续若进入产品交付，复用普通设备添加／设置的显式操作，不再扩展验收页为第二套产品流程。
- **验证**：最终当前源码定向 7 组、113 项通过，含实际 BLE 服务生命周期接线、原普通配网、BC-5 同 owner 恢复及背景／换账号／取消反例；Angular 模板与 TypeScript 核查通过。只改 App 交接和最小提示，不改 WiFiProv 原生、Gateway 或固件。本轮未安装 APK、开窗、触发 Wi-Fi、部署或提交推送；真机 Wi-Fi 提交与上线不销项。

## 27 普通 App／COM11 真机功能复验（2026-11-04）

- 把普通入口和 BC-5 专项分开执行。复用当前已安装内测 App、Development 会话和 COM11「IP4 同板 Cloud」，从真实首页打开原控制页。该页实际走 BLE，测试开关由关到开；确认页面收到开状态后，用一次相反方向的普通操作恢复原关状态，回读约 180 ms。MQTT 旁路未观察到该设备控制命令，后续 UI 确认为 BLE，故不把此结果写成 Cloud 控制或独立原始 ACK。离页再进入，页面实际显示「云端已连接」，回读原关状态及新鲜运行时长；区分两条通道。
- 使用原「添加设备」入口进入普通 BLE 添加页，控件可用，没有点击会自动认领的按钮。另走普通 Wi-Fi 添加进入现有 WiFiProv，执行一次 BLE 扫描／停止（无配网候选，不把日常 Direct 当配网广播）；切换到 SoftAP 页后退出，无热点连接、身份写或 Wi-Fi 下发。返回首页，自有 ADB 转发已释放；本轮没有刷板、登录、服务变更或 Git 提交推送。
- 首装不是未执行的基础功能：[普通入网执行文档](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md) §26（手动 SoftAP）和 §30（BLE）已有正常 App 真机成功记录。本轮不把上述功能复验写成新设备首装或 §26 新修复已装机。若要对当前版本重验完整首装，应使用明确可初始化的匹配普通 WiFiProv 测试板走原 App，一次验证连接→配网回执→云上线；不再让 BC-5 专项门槛挡住普通测试，也不覆盖 COM3 已激活身份或重放旧操作。BC-5 的 Wi-Fi-only 恢复和上线仍按 §26 独立保留。

## 28 普通 WiFiProv BLE 空根首装差异（2026-11-04）

- 用户允许选一块测试板初始化、不恢复；只选 COM11，保留 COM3 已激活身份和 BC-5 历史。复用 §14.2 已真机的普通 WiFiProv BLE Security1／非空 PoP 固件 `5f9a…4990`、原 ESP32 SDK 3.3.11 和现有内测 App，不另构建或部署服务。核实际 COM11 USB、ROM 安全状态、原 `e48c…8f07` 应用及原分区后，仅写入应用区及明确获准初始化的 20 KiB NVS；两处已读回校验，分区不变，暂留 ROM，未开启十分钟配网窗口。旧 COM11 云端记录、App 授权和历史日志不删，不搬旧 scope。
- 当前真实 Development 会话只读 `/api/v1/auth/me` 与设备列表均 200，账号匹配，5 台可见设备、权益 10 台。普通首装仍走原创建／DeviceKey 明文读取合同，当前近期认证标记缺失；不挪用 §25 只对加云安装生效的授权。现阶段未创建身份、读取明文 Key、发送验证码或下发 Wi-Fi。待同账号近期认证并核匹配入口后只启动一次窗口，按原正常页面连续验 Security1、Bootstrap 精确 ACK、一次官方 Wi-Fi 提交和新鲜云上线。
- **真机结果：通过。** 用户手动同账号登录，真实 Redis 近期证明核验有效，未代发验证码。随后仅 COM11 启动一次，从原「添加设备 → Wi-Fi → ESP32 配网 → BLE」进入：一次发现与选择，实际 Security1 连接约 1.34 s，GetInfo 24 B／空根 epoch 0。设备创建 201、Key 读取 200、Admin 201 各一次，精确 Bootstrap ACK `[2,3,0]` 约 136 ms；一次官方 WiFiProv 提交返回 `success:true`（约 7.61 s）。App 自身的只读云观察前三次未确认，随后拿到新鲜 online 证据；单独回读真实 App 已自动进入本次新设备的控制页，不把页面导航当作上线证据。
- 本轮没有第二次写入、自动重发、恢复固件或修改 COM3。COM11 保留新测试身份和普通 WiFiProv 固件；原 App 历史、旧云端记录及 BC-5 UNKNOWN 保留。验收监听和 ADB 转发已释放，脱敏阶段记录保存在忽略目录。未部署、升级 SDK、提交或推送；本次通过仅覆盖普通 BLE WiFiProv 首装，不替代 BC-5 或重复通过的 SoftAP 首装。

### 28.1 普通绑定不应要求退出重登

已确认固定 5 分钟不是 BLE、SoftAP 或账号登录有效期。普通 WiFiProv 新建分配与既有根接入当前复用 `revealDeviceKeyV2`（公开明文 Key 的敏感接口），因而把明文导出策略带入正常配网。产品应分开处理：有效会话可完成普通绑定；新设备密钥只允许一次安装专用交付，与本次账号／分配意图／目标版本和物理会话绑定，继续沿现有 Security1／PoP、根 CAS、安装 ACK 和 UNKNOWN 只读协调；不得变成任意已有设备 Key 读取。保留用户显式查看明文的原鉴权，不全局改 TTL。先做普通新建分配的最小差异，不重造 WiFiProv、已有根重配或身份链。§25 私有 BC-5 不能直接当作普通入口，尤其不能用 App 自报的设备身份替代可信校验。
