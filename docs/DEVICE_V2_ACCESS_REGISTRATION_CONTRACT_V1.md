# Device V2 跨产品注册删除与幂等接口合同

更新：2026-11-03。归属：[独立接入注册方案](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md) IP-1。状态：**IP-2／IP-3软件及隔离PostgreSQL7／7结果保留，Cloud→BLE核心两卡有限真机结果取执行方案§14.6。** 本v1仍只支持Cloud→BLE保根注册，不因后续[BLE→Cloud计划](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md)放宽根、prepared期限、配额或UNKNOWN合同；反向尚未开放。

同一实例的Cloud与BLE Direct注册各有卡片、logicalDeviceId和额度；WiFiProv BLE／SoftAP仍是同一Cloud产品的配网通道。首期选择**独立业务注册、显式关联原物理根**，复用原Method 2、额度锁及intent恢复，不安装第二个Admin，不新增设备wire或多根存储。

## 1 本期可实施范围

| 方向与条件 | 冻结决定 |
| --- | --- |
| 空根首次Cloud或BLE Direct | 沿原入口和设备协议；新增可信产品分类，BLE在签发前补统一额度预留 |
| Cloud已有合法Admin，本机持有原secret，固件提供Direct | 通过本页保根intent及新鲜Method 2，创建独立BLE ID；不改DeviceKey／Admin／epoch／TaskStore |
| Cloud无Direct能力或手机缺原Admin secret | 明确能力／凭据门禁；Cloud在线或GET Admin元数据不替代认证，不清根补建 |
| BLE Direct已有根，准备新增Cloud | 新建Cloud ID是目标规则，但v1暂不开放该方向；不得退回旧enable-cloud复用ID |
| 原Cloud重新配网或更换WiFiProv bearer | 沿原Cloud ID与原配网合同，不调用跨产品注册接口 |

BLE→Cloud的设备`Install`及`DeviceAccessStore::replace`确有保留Admin的存储原语，但加Key会改变任务rootDigest；现GetInfo没有TaskStore空态证明，无Cloud scope也不等于设备无绑定任务。本方向返回具体能力门禁，在能证明存储边界或完成原任务生命周期后另补小阶段，不通过擦NVS、迁scope或猜Empty放行。

## 2 现有接口复用清单

| 现有API | 源码实际行为 | 本合同处理 |
| --- | --- | --- |
| `POST /api/v2/devices` | 严格name／deviceType及Idempotency-Key，创建新Cloud ID／Key，已有user／tenant额度锁 | 保留原报文；受控入口写入cloud产品分类，计量复用统一预留口径 |
| `POST /api/v2/devices:resolve-instance` | 原实例解析优先Direct；另有专用Cloud入网确认合同 | 新增明确版本的按产品只读解析，不改原Cloud确认合同 |
| `POST /api/v2/devices:enable-cloud` | 给原Direct ID补Key，没有新卡及新增额度 | 不作为新跨产品入口；新App停止调用此路径新增另一产品。旧用途／旧UNKNOWN保留原恢复合同，不静默改变返回语义 |
| `POST /api/v2/ble-enrollment/intents`及commit／cancel | 原空根grant、回执与Method 2确认；commit才插BLE行 | 保留原安装／恢复协议，补统一额度预留；不能用于保根新增第二Admin |
| `GET/PUT/PATCH /api/v2/devices/{id}/direct-access/admin` | GET／PUT是元数据，PATCH是条件根替换 | 不能作为设备持有secret的证明；跨产品绑定不走PUT复制元数据或PATCH重置 |
| `DELETE /api/v2/devices/{id}` | 目标ID级撤销，稳定deleted重放，localAccess为not_confirmed | 保留URL与权限，增加注册绑定／引用处理，不声称物理根已清除 |

## 3 注册产品与根绑定

### 3.1 产品分类

新增服务端只读投影`registrationProduct: 'cloud' | 'ble_direct' | null`，随原详情／列表返回，创建后不可由普通更新改写。它由受控创建／认领用途设置，不由MAC、deviceType字符串、cloudEnabled或客户端自行声明决定；网关路由及旧BLE加Key不因此被重分类。

迁移仅根据可靠的原创建／认领记录识别旧行。旧双用途、来源矛盾或缺证行保留null，按原ID使用／协调；新的按产品解析不能将它们当成“目标产品不存在”后又建一张。MAC相同不构成迁移或合并理由。

按产品解析扩展：`POST /api/v2/devices:resolve-instance`，显式头`X-Blinker-Contract: access-registration-v1`，严格body为`{deviceInstanceId, registrationProduct}`。同账号、同产品唯一有效注册返回`{device}`，确无返回`{device:null}`；目标产品内歧义或相关legacy未确定则409。此为关联读取，不证明所有权，不授权新注册。原Cloud确认版本及expectedContext逐字合同保持。

### 3.2 根上下文

本合同`expectedRoot`严格包含：

| 字段 | 类型与比较 |
| --- | --- |
| deviceInstanceId | 非零16 B，canonical base64url |
| accessEpoch | 1至2^32−1整数 |
| controllerId | 非零16 B，canonical base64url |
| credentialVersion | 原Admin版本，当前支持1 |
| adminFingerprint | 非零32 B，canonical base64url；由本机原secret计算，服务端与源记录比较 |
| cloudCredentialVersion | 源Cloud当前Key版本；与服务端权威版本相等，保根过程中不Rotate |

authority、owner和tenant由服务部署及原鉴权确定，不接受body覆盖。根上下文须能与源注册的持久实例关联、当前Admin及当前Key版本对应；仅有一次未证明challenge、公开instance或客户端填入的PUT Admin记录不足以证明整条关联。

新增的注册根绑定保存`logicalDeviceId、registrationProduct、rootBindingId、instance、Admin上下文、bindingRevision、state及原intent关联`等非秘密材料。`rootBindingId`为服务端随机不透明标识，同根的两种产品显式共享此引用；它不是MAC派生身份，不占额度，也不引入全局instance唯一约束。

每条已确认绑定拥有独立持久上下文，不能只以sourceLogicalDeviceId外键或“源卡仍active”为唯一运行依据。删除源卡仅退休该卡绑定；另一卡仍需每次按原认证和epoch验证，绑定快照不成为永久控制授权。同一有效根下，`rootBindingId＋registrationProduct`的在途／有效目标由持久注册槽排他保护，不能仅在prepare时查重后释放锁；intent引用该槽，提交原子转为active。

实现只需新增用途明确的intent／绑定持久记录和原模型的产品投影，不增加第二套设备、控制器或签发执行链；不存原secret，不伪造BLE Bootstrap／ControllerMutationReceipt历史。

### 3.3 有效根消费者与变更

新绑定必须接入原有效Direct实例视图或同一个共享查询口，覆盖`requireFreshClaim`、原实例解析、reset及删除准入；不能只有新resolve能看见它。按产品区分目标，同rootBinding下合法的两产品不按“两个ID”误判歧义；新BLE卡即使源Cloud卡后来删除，仍能被正确解析。同产品的另一在途／有效注册仍须围栏。

DeviceKey、Admin、epoch及presence写路径从IP-2开始共用`rootBindingId＋bindingRevision`锁与CAS。已知的根替换／根退休在权威事务中更新根版本，使该根旧注册绑定进入stale／待核对、未决保根intent进入invalidated并释放无设备写预留；本机根秘密版本变更一次失效所有引用及旧附近证据，不只更新发起操作的卡。presence轮换仍走其原单调版本及精确回执，不与根替换混为同一操作。

存在共享引用而尚未接入上述围栏的旧Rotate、reset或presence写入口明确409 `ACCESS_REGISTRATION_SHARED_ROOT_OPERATION_REQUIRED`，不继续按单logicalID修改。IP-3仅为唯一活跃原Cloud材料owner接入§6.3的presence单写例外，不放宽其它根级写入口，也不关闭Direct功能或普通控制。设备离线发生且服务端尚未知的变化，仍由下一次原认证发现，不承诺即时远端撤权；服务端元数据更新也不等于MCU已写入。

## 4 保根新增注册接口

接口统一`Cache-Control: no-store`、原Auth及owner权限、严格字段、原请求日志和限流；当前v1仅支持源cloud、目标ble_direct。新handler是原管理服务的薄适配，数据库事务不跨设备或网络I/O。

### 4.1 准备

`POST /api/v2/access-registrations/intents`，严格body：

```json
{
  "requestId": "<nonzero-16B-base64url>",
  "sourceLogicalDeviceId": "<owned-cloud-id>",
  "targetProduct": "ble_direct",
  "name": "<validated-display-name>",
  "expectedRoot": {
    "deviceInstanceId": "<16B-base64url>",
    "accessEpoch": 1,
    "controllerId": "<16B-base64url>",
    "credentialVersion": 1,
    "adminFingerprint": "<32B-base64url>",
    "cloudCredentialVersion": 1
  }
}
```

App先核本机原凭据与声明能力，再显式准备；不因看到另一产品就自动调用。服务端同事务核源owner／tenant／active、可信产品、原根CAS、目标产品未被认领及统一额度，预留一个目标ID和额度后返回201；同请求精确重放200。

返回data固定包含`intentId、sourceLogicalDeviceId、targetLogicalDeviceId、registrationProduct、rootBindingId、bindingRevision、expectedRoot、clientNonce、expiresAt、state、quotaState、replayed`。intentId与本次requestId一致；clientNonce为服务端随机非零16 B，canonical base64url；bindingRevision为正整数CAS版本。prepared期限为120秒，expiresAt为UTC毫秒。响应不含DeviceKey、Admin secret、presence key或可安装grant；此时未新建active BLE卡。

`targetProduct='cloud'`在写入和预留前返回409能力门禁，不调用旧enable-cloud，不偷偷分配另一Key。真正不存在／不属于当前账号的源记录统一404，不泄漏他人ID或名称。

### 4.2 本次设备认证

原Direct认证薄端口接入本intent的clientNonce，重新运行原Method 2的device proof、app proof及授权确认；不以旧ready会话、GATT已连、presence命中或页面布尔值代替。本机核同账号、同intent、原凭据的controller／epoch／version／权限及已关联instance，后台／账号变化／取消沿原owner生命周期退休，不自动重连或重认证。

现wire已携带clientNonce，logicalDeviceId不在设备认证报文中，因此无需改设备协议或安装第二Admin。新nonce只给这个显式认证用途，不开放可由任意页面注入nonce的公共通用连接能力。

App形成非秘密`confirmationDigest`，编码冻结如下：对下列数组作标准`JSON.stringify`，以无BOM的UTF-8编码计算SHA-256，再转无padding的canonical base64url。`rootTuple`依次为expectedRoot表中六字段；`authTranscriptDigest`为原`directAuthTranscript(context)`字节的SHA-256，使用相同base64url编码。IP-2仅补共享fixture和薄接线，不再自行选择另一编码。

```text
["blinker.access-registration.confirmation.v1", intentId, sourceLogicalDeviceId,
 targetLogicalDeviceId, targetProduct, rootBindingId, bindingRevision,
 rootTuple, clientNonce, authTranscriptDigest]
```

该摘要用于绑定本次操作，不另建密码协议。原secret和完整HMAC／transcript不上传或写公开日志，digest仅供关联与同commit比较。

**信任边界：这是复用原App验证Method 2后向Gateway确认的模型，不是Gateway独立验签硬件。** 服务端无Admin secret，不能从digest验证HMAC或判定设备实际完成认证，只能验证其已知上下文、nonce、状态与幂等，并保存App确认。如果要求抵抗恶意改造客户端伪造确认，需要独立设备证明能力，不宣称本v1已经提供。

### 4.3 提交

`POST /api/v2/access-registrations/intents/{intentId}/commit`，严格body：

```json
{
  "commitId": "<nonzero-16B-base64url>",
  "method2Confirmed": true,
  "clientNonce": "<same-intent-16B-base64url>",
  "confirmationDigest": "<32B-base64url>"
}
```

服务端先识别本intent已有终态及精确重放，再在原user／tenant锁及该intent、根绑定锁内重核源卡仍active、bindingRevision／根上下文未变、本intent仍持有目标注册槽、prepared未过期。原子插入目标BLE逻辑卡、可信产品及确认后的根绑定，quotaState从reserved转consumed，同时保存commitId与请求摘要及committed终态；不得先返回成功再后台插卡。

成功返回200，data包含准备响应的非秘密身份上下文、`state='committed'、quotaState='consumed'、device、replayed`。相同commitId与相同完整提交摘要返回同一目标ID；不同commitId或内容409，不重复创建。源卡已删／根变化则拒绝新提交，不继续从已删除源认领；已有committed结果只读核原终态，不据源卡后来删除改写历史。

目标卡后来删除时，原intent及相同commit重放仍只关联原目标，device投影反映当前deleted／退役状态，不返还伪造active、不复活卡片或再次占额。保留“当时已确认注册”和“当前已删除”的区别。

### 4.4 查询与取消

- `GET /api/v2/access-registrations/intents/{intentId}`：同账号显式只读，无body；返回原身份上下文、state、quotaState及已提交device，不含秘密，不生成nonce／grant，不推进设备操作。无记录或他人记录404。
- `POST /api/v2/access-registrations/intents/{intentId}/cancel`：严格空body，仅未提交prepared可取消，重复取消返回同终态；committed返回409，不用取消代替删除。
- prepared过期、取消后迟到commit返回410，不因method2Confirmed而复活；同requestId不会重新开120秒窗口或换nonce。创建或提交HTTP结果未知，App仅保留原ID并提供显式GET，不自动重发POST、取消后另建或先显示失败。
- 新intent只有设备认证读，无可在TTL后安装的grant；因此可在同事务锁内将仍prepared的过期项置expired并释放预留。若提交已落库则是committed，不得释放已用额度。App的传输UNKNOWN不是数据库第三种真相，查询返回哪一原终态就按哪一终态显示。

intent状态限定`prepared | committed | cancelled | expired | invalidated`，quotaState限定`reserved | consumed | released`。源删除将其prepared意图置cancelled，迟到提交410；根版本变化置invalidated，查询保留原上下文及原因，提交409 ROOT_CONTEXT_CHANGED。只释放尚未转换的预留，不改旧signed-grant用途。

App本地checkpoint另允许严格`rejected`，不是新增数据库intent状态：仅本次begin的prepare收到准确Gateway HTTP409／`ACCESS_REGISTRATION_ROOT_OPERATION_PENDING`，且仓库合同保证未创建新intent、预留或member时，记录非秘密`prepareRejection={httpStatus:409,code:...}`、`localReferencePending:false`，不得带intent、confirmationDigest或unknownOperation。它只允许显式本地清理，不发GET或无线请求；已有UNKNOWN仍阻止同source另起操作。同码在commit、其它错误、网络中断或仅后续GET404均不能进入此终态。

## 5 幂等与额度

准备唯一作用域为当前authority数据库内的owner＋requestId；请求摘要包含合同版本、tenant、source ID、targetProduct、name及完整expectedRoot，字段用服务端固定顺序规范编码。首次后任何字段不同409；先查原请求再决定是否新占额，不能在额度已满时把自己的重放拒绝为新请求。

所有产品共用原`user → tenant → 操作／根`锁序与一个计量函数：

`使用量 = tenant下state != deleted的逻辑卡数 + 尚未转换为逻辑卡的有效预留目标数`。

预留按targetLogicalDeviceId去重，目标卡已计入便不再算预留；原BLE恢复复用已有逻辑卡时不再占一份额度。Cloud create、BLE首次签发、保根新增都读取此口径；额度不足沿`403 DEVICE_LIMIT_REACHED`，缺少合法权益沿原权益错误，不改套餐或放宽账号额度。

**IP-2已在原BLE首次issue补统一额度预留，commit只转换原目标。** 可实际安装的grant在签发前同事务保留额度，不在设备已安装后才首次验额。原已签grant、expired但安装结果未明的意图不因TTL释放；升级前遗留未决项也按原记录保守纳入计量，不伪造设备回执或因新规则拒绝原只读协调。超额只阻止新分配，不删除已有设备或UNKNOWN。

这与第4节无设备写的prepared过期不同，不能写一个统一TTL清扫器处理所有用途。事务内部沿原有限SERIALIZABLE冲突重试，不新增业务POST或设备报文自动重发。

## 6 删除与本地安全存储

### 6.1 服务端

继续`DELETE /api/v2/devices/{logicalDeviceId}`，原owner权限及稳定终态重放不变。事务只退休目标产品卡、目标绑定及该ID的会话、分享、presence发行关系和业务权限；不按instance或rootBindingId批量删除另一卡。只有目标ID真实拥有的Cloud task scope才走原任务退休，不给BLE卡借Cloud scope。

新BLE卡没有旧BLE enrollment安装intent，现removal的旧安装历史准入会拒绝它，必须新增窄分支：核可信ble_direct产品、本合同committed记录及原active／已知失效的注册根绑定、owner权限和原Cloud／topology冲突边界后，允许该目标ID的注册级退役。不能伪造旧receipt，也不因根已stale而阻止用户删除自己的注册卡；物理撤权仍不确认。

删除保留原`localAccess='not_confirmed'`、Broker清理及任务结果语义；补充`registrationProduct`与`rootDisposition='retained_for_other_registration' | 'not_confirmed'`投影。共用根还有有效绑定时不发清根命令，不删另一个绑定依赖的根／presence材料；最后一张卡删除也不能仅凭SQL结果声称设备物理权限清除。

源删除与跨产品提交使用同锁序：源删除先提交则新commit拒绝；新commit先提交则后续删除仅影响源ref，目标保留原已确认绑定。pending目标不借源卡退役继续建卡，committed目标不因源卡后续删除自动退役。

### 6.2 App

单份原根secret存储于当前authority／账号作用域的安全存储；每张注册卡保存显式rootCredentialRef及自身用途／状态。`load(logicalDeviceId)`通过该卡引用解析原凭据，不复制secret来假装两套授权，也不再以findByInstance随意选卡。

旧v5／v6记录只在原凭据成功解析且显式绑定成立时迁移为新可识别引用格式；不把旧pending或UNKNOWN改成active。迁移采用“先写并核安全根项，再写卡引用，最后清旧副本”，中断仍保留原秘密，恢复只协调原迁移，不回滚原intent。新格式需旧读取器明确失败，不把root引用当内联secret解析。

删除目标卡只移除该卡的有效引用及本次已终结、可清理的临时checkpoint；原UNKNOWN、旧已签intent和完整journal／审计记录不因删卡抹除，只标该注册已删除、禁止新写，仍按原只读协调及retention合同保留。其它active或未决引用仍在时保留根秘密。引用清理失败继续显示已确认服务端删除并保留本机清理待办，不撤回删除确认，不静默删其它卡。所有引用为零且原未决记录不再依赖它时才可清本机秘密；这也不是其它手机或MCU物理撤权证明。

不得调用SDK`clearAll`／`eraseAccess`实现单卡删除，亦不能撤销Admin来模拟独立卡撤权。附近材料同样按root绑定的唯一版本／写入权协调，不能给两张卡各分配一把物理presence后互相覆盖。IP-3软件投影按下节执行；材料读取、设备初始化和两卡真机通过仍是不同结果，不关闭既有Direct功能。

### 6.3 IP 3 附近材料只读与唯一写入方

`GET /api/v2/access-registrations/presence/{logicalDeviceId}`复用原App Auth、owner／tenant隔离及请求限额，成功和错误均为`private, no-store`。不接受query或body，不从客户端覆盖账号、根、材料owner或版本；GET不allocate、rotate、sync或向设备发送数据。

成功为`{status:200,data:...}`，data严格包含：

| 字段 | 合同 |
| --- | --- |
| contract、logicalDeviceId | 固定`access-registration-presence-v1`及本次独立逻辑卡ID |
| rootBindingId、bindingRevision、expectedRoot | 当前有效根、CAS及§3.2六字段上下文，与当前Admin和注册证据一致 |
| materialOwnerLogicalDeviceId | 根内唯一原Cloud产品槽的材料owner，不按MAC／instance另选一张卡 |
| canWritePresence | 仅目标就是仍active的原Cloud材料owner时为true，不代表writer已交接或拥有额外控制权限 |
| keys | 最多两项，严格为`{state:'current'\|'previous',accessEpoch,version,key,keyDigest}`；key为16 B、keyDigest为32 B canonical base64url |

只读取原presence记录中`device_confirmed_at`非空的材料；无确认材料时为`keys:[]`，新current未确认时可只返回已确认previous。仍按材料原logical ID及原vault AAD解密，不复制密文冒充目标卡上下文，不把密钥写入日志。

目标逻辑卡、member及root必须active，owner／tenant／产品／Admin／根CAS一致；BLE目标另需匹配当前根的committed保根证据。不存在、已删除或不属于当前账号的目标统一404 `ACCESS_REGISTRATION_NOT_FOUND`；根变化、未确认绑定或材料归属歧义返回409 `ROOT_CONTEXT_CHANGED`，不回退MAC匹配或新建版本1。源Cloud正常删除后，存活BLE卡仍可只读已保留的确认材料；被删源卡自身读取404，不据源删除向BLE移交writer。

原`GET /api/v2/devices/{id}/presence-key`及`POST .../presence-key:allocate|rotate|sync|confirm`保留原报文、单调版本和精确回执。共享根写只允许唯一、仍active的原Cloud材料owner走原链路；BLE alias写仍返回409 `ACCESS_REGISTRATION_SHARED_ROOT_OPERATION_REQUIRED`。无活跃原Cloud writer时只读，不自动换材料owner、补分配或重放写入。

App先核响应ID、合同、禁缓存、摘要和epoch，再核本机rootRef、revision及完整expectedRoot。只有本次目标卡取得显式presence绑定；单份确认材料留在安全root项，不把同instance或rootRef的所有卡自动授权。同版本不同材料、回退或stale根被拒绝。必要时只对响应明确指定、且本机已有同根active凭据的材料owner额外GET一次，由其独立接口验权；不遍历根找卡，也不把网络轮询加进扫描或前后台返回循环。

原writer未确认安装材料只存一笔`presenceWriteCandidate`，仅该writer可见，可供显式原Method2连接定位及原意图协调；它不是设备已安装或已确认的证据，不能进入alias材料或任何附近扫描候选。确认后才窄更新root材料并清对应candidate，不自动重发未知安装。

附近仍由原Direct HMAC、观察时刻及TTL判定。同一观测可分别投影给多个有效卡，但每张卡须有显式绑定，且root ID、revision、材料owner及物理key上下文一致；仅同MAC、instance、secret或rootRef仍不足，多根／无绑定命中保持歧义。附近不等于Cloud在线、已认证连接或可控制；沿原扫描owner、账号／后台围栏和64条观察上界，不调节拍或新增扫描器。

初始化条件不具备时保持具体门禁：本机缺原Admin为`LOCAL_ADMIN_CREDENTIAL_REQUIRED`，旧根缺登记CAS为`ACCESS_REGISTRATION_PRESENCE_CONTEXT_REQUIRED`；无确认材料的alias为`ACCESS_REGISTRATION_PRESENCE_SETUP_REQUIRED`；原source无已认证ready会话时仍为`BLE_DIRECT_SESSION_NOT_READY`。这些App条件不会把GET `keys:[]`改成配网失败，不以重新登录补造secret、猜MAC、清根或自动install解除。

真实隔离PostgreSQL 16.15已完成059→060迁移、约束／视图、并发prepare／commit／remove、回滚及关闭持久性、源删后BLE关联保留和confirmed材料／alias门禁回归：7／7通过、0skip，均为独立合成身份。它不代签业务库迁移、匹配部署安装或两卡真机；当前运行状态统一取执行方案§13，不在本合同重复累积发布记录。

## 7 错误与生命周期

| 分类 | HTTP与处理合同 |
| --- | --- |
| 无Auth／不属于当前账号 | 原401／404；不跨authority找凭据，不泄漏他人注册 |
| 额度不足 | 403 DEVICE_LIMIT_REACHED；原已成功请求重放先协调，不重复预留 |
| 无本机Admin | App显式LOCAL_ADMIN_CREDENTIAL_REQUIRED，不提交确认，不从服务器元数据补secret |
| 无Direct或v1未支持的加Key方向 | 409 ACCESS_REGISTRATION_CAPABILITY_REQUIRED；指出缺Direct或任务区证据，不误报“同实例禁止新增” |
| 源根变化／同产品已存在／幂等冲突 | 409及窄code ROOT_CONTEXT_CHANGED／PRODUCT_ALREADY_REGISTERED／IDEMPOTENCY_CONFLICT；不自动重置或换ID |
| legacy无法判定或目标内多条关联 | 409 ACCESS_REGISTRATION_CONTEXT_AMBIGUOUS；原ID恢复继续，不猜归属 |
| prepared过期／已取消 | 410 ACCESS_REGISTRATION_CLOSED；不复活或自动开新intent |
| 网络中断／超时／提交响应丢失 | 保留原意图及UNKNOWN，显示未确认；仅用户显式只读GET恢复 |

以上ACCESS_REGISTRATION类code已接入候选的窄handler／仓储，运行版本以执行方案§13为准。后台、离页、取消及账号切换沿原owner／AbortSignal／关闭确认围栏；原App入口已装配，新协调器不因切回前台自动查询或认证。

## 8 IP 2 的最小实施与验证

以下保留IP-2实施顺序；当前核心、App及IP-3结果取执行方案§11～13，不重跑整套旧场景：

1. 固定可信产品分类与按产品解析，保留原Cloud-confirm版本；对旧同ID升级调用做明确新旧入口分流，不悄悄改老响应。
2. 提取共用计量／预留薄模块，补BLE签发前额度及原未决意图计量，保持旧恢复用途。
3. 实现本合同Cloud→BLE保根intent与提交，App接原Method 2指定nonce及root凭据引用，不改设备wire。
4. 将新绑定接入原Direct关联消费者、共享根写入围栏及目标ID删除／引用回收，保留localAccess未确认及目标scope边界。随后IP-3再接附近映射；匹配部署／真机仍在IP-4／5。

有限用例须覆盖：同产品并发只一目标、跨产品两卡计两额、重放不双算、BLE签发前拒超额、grant过期UNKNOWN保留、新只读intent过期迟到拒绝、提交后回包丢失GET协调、源删除两种顺序、目标删除后原提交不复活、新绑定被原关联消费者识别、共享根变更与未接线写入口围栏、根CAS变化及全引用失效、缺原secret、无Direct、legacy歧义、重复删除、目标task scope不串用、删除不抹UNKNOWN及迁移中断不丢secret。新绑定测试不得伪造Bootstrap回执，不重复已过配网／Start／自动化。

## 9 IP 1 冻结时的源码核对结果

| 已核消费者 | 本批结论 |
| --- | --- |
| [Gateway管理路由](../../aily-blinker-services/services/gateway/src/index.js) handleDeviceV2ManagementApi | 原严格请求、Cloud-confirm版本及近期Key认证保持；新API需独立窄handler |
| [Cloud仓储](../../aily-blinker-services/services/gateway/src/device-v2/repository.js) createDevice／resolveOwnedDeviceByInstance／ensureDeviceKeyForInstance | 原创建可复用；原跨产品优先解析及同ID加Key不等于新注册 |
| [BLE认领](../../aily-blinker-services/services/gateway/src/device-v2/ble-enrollment.js) issueIntent／commit | 原commit不独立验证Admin secret；尚无额度预留，不能只删除跨产品409 |
| [Direct元数据](../../aily-blinker-services/services/gateway/src/device-v2/direct-access.js) registerAdmin／resetAdmin | 元数据CAS不是设备secret证明，不作为新绑定认证口 |
| [删除仓储](../../aily-blinker-services/services/gateway/src/device-v2/removal.js) removeOwned | 已按目标ID退休；共享根新增关系不能破坏此边界 |
| [任务仓储](../../aily-blinker-services/services/gateway/src/device-v2/task-management-repository.js) beginRetirement | scope按目标logicalDeviceId，不能因同instance迁给另一卡 |
| [App原认证](../../blinker-app/src/app/core/device-v2/ble-direct/client.ts) authenticate | 原双nonce及HMAC可复用，logicalID仅用于App会话归属；需窄nonce输入 |
| [App原凭据](../../blinker-app/src/app/core/device-v2/ble-direct/credential-store.ts) load／findByInstance／remove | 当前按卡存secret，多instance匹配会歧义，需显式根引用 |
| [SDK Key安装](../src/BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.inc) install | 原Install可保Admin加Key，但3 B ACK不绑定新注册，且不证明TaskStore为空 |
| [SDK根存储](../src/BlinkerV2/identity/DeviceAccessStore.inc) replace／installVerified／clearAll | 单Admin；新注册不再安装根，单卡删除不清根 |
| [任务权威](../src/BlinkerV2/provisioning/TaskAuthorityContract.inc) rootDigest | Key／Admin改变仍触发原围栏，不能承诺任意加Key后任务无损 |

IP-1仅完成必要消费者的只读核对、合同选择及文档一致性检查；没有运行产品回归、连接数据库／手机／设备或服务器，没有改产品源码、锁文件、原SDK或账号授权。未知意图、TaskStore及完整日志未操作，无关本地修改保持，暂不提交推送。
