# Device V2 独立接入注册与附近状态实施方案

更新：2026-11-04。**Cloud→BLE真机见§14.6，源码／CI见§17～18；反向BC-1／2见§19～21，BC-3内部闭环和062实库见§25，BC-4a 专用回执消费者见§26。** HTTP、默认入口和实际产品无线交接仍关闭，不宣称完整反向入网已可用。尚未提交或触发远端CI，私有golden授权HOLD不变。授权、TaskStore、UNKNOWN和完整日志保留。

产品决定：**同一设备实例分别以BLE Direct、WiFi／Cloud产品注册时，分别建立设备卡、逻辑ID并占用额度；MAC不参与注册归属判断。** WiFiProv BLE与SoftAP只是WiFi设备的配网通道，切换配网通道不另建设备。

本方案取代[原设备接入计划](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md)中“同实例跨产品必须同ID”“Cloud存在即阻止BLE新增”“BLE转Cloud默认复用ID”的产品前提。原DA-1测试只证明旧合同的实现行为，不能作为新规则的通过证据；DA-2附近生命周期、DA-3扫描owner及DA-4已完成的有限无线结果继续保留。

## 1 产品注册与连接变化

“接入产品”指用户明确选择的BLE Direct设备或WiFi／Cloud设备；“连接通道”指配网或运行时使用的BLE、SoftAP、Cloud等传输。只有显式新增独立产品注册才新建卡片，不因每次传输变化创建身份。

| 用户操作 | 逻辑ID与设备卡 | 额度与归属 |
| --- | --- | --- |
| 首次添加BLE Direct产品 | 新建BLE注册A | A占用1台 |
| 同实例首次添加WiFi／Cloud产品 | 新建Cloud注册B，不合并A | B再占用1台；A、B独立管理业务数据 |
| WiFi设备由BLE配网改为SoftAP配网 | 保留原Cloud注册B | 不新增占用 |
| 原设备修改路由器、重连或已有运行通道切换 | 保留该产品原注册 | 不新增占用，也不自动创建另一产品 |
| 重复点击、取消后恢复、进程重启或UNKNOWN协调 | 使用原操作与原注册结果 | 不重复创建或重复扣额度 |
| 同MAC但不同合法设备实例 | 各自连接可区分且认证成立后独立注册 | 注册层不因MAC相同合并、拒绝或互相覆盖 |

同产品的合法重新注册仍走原释放／退役及新认领合同；不将“允许跨产品分卡”解释成可重复认领已有根、绕过他人所有权或无限创建同产品副本。

## 2 身份职责

| 标识或材料 | 职责 | 不允许的用途 |
| --- | --- | --- |
| logicalDeviceId | 服务端生成，标识一条产品注册；权限、额度和业务数据以此归属 | 不由MAC计算，不因instance相同跨产品自动复用 |
| 接入产品类别 | 明确区分BLE Direct与Cloud注册，限定解析、幂等及恢复入口 | 不由客户端随意改值绕过owner、根或额度检查 |
| deviceInstanceId | 设备持久随机实例，用于协议关联、证明核对及排错 | 不单独证明所有权，不作为跨产品库存唯一键 |
| DeviceKey及Direct控制凭据 | 各自原认证链的秘密材料 | 不互相派生，不以公开instance替代认证 |
| MAC、原生deviceId及广播名 | 当前平台的发现结果、连接句柄及临时扫描去重 | 不决定账号归属、注册唯一性、逻辑ID或额度 |

服务端解析必须明确目标产品及原操作上下文。不得继续从同instance的任意一条记录中“优先Direct，否则Cloud”来替用户决定目标卡；同产品存在无法解释的多条有效关联时显式报歧义，不选择最新记录。

不依赖MAC注册，不等于消除无线地址冲突。若同一现场的两个BLE设备实际地址相同，系统扫描去重或GATT寻址仍可能无法区分；此时明确暴露传输问题，不猜测归属，本期不承诺两板可同时发现／连接。

SDK已有随机16字节instance持久化实现，沿用，不改成MAC派生值。instance意外相同但凭据不符时走冲突／认证失败，不自动合并或重置。若设备连同全部秘密被完整克隆，仅凭相同协议材料无法区分两块物理板；此问题不靠卡片ID或修改MAC解决，也不扩为本期防克隆工程。

## 3 设备端授权与任务边界

当前`DeviceAccessStore`保存可选DeviceKey及一套Direct控制根、Admin、epoch和presence；设备不保存账号或logicalDeviceId。任务rootDigest又绑定instance、DeviceKey及当前Admin材料。因此，**两张独立业务卡不表示设备已支持两套独立物理授权根或两份任务资源。**

首期保留现有单物理根，不为分卡重造多根存储。跨产品注册必须明确建立“该注册记录→已证明接入域／物理根”的绑定，不能只是复制另一张卡的Admin元数据。

### 3.1 已有根新增另一产品

1. 核对当前authority、账号权限、实际instance、epoch及有效设备根证明；同邮箱、Cloud owner布尔值或实例查询结果都不够。
2. 按目标产品建立新的logicalDeviceId，复用原额度及事务保护。已有根不再冒充空根首次Bootstrap，不静默Rotate、resetAdmin或覆盖DeviceKey。
3. 对照原认证会话、签发及精确回执，确定是否已有可绑定新注册的证明。现协议不够时，只补原链路中的最小关联证明；未满足前明确提示“已有授权，需完成授权验证”，不能伪称注册完成，也不能仅因已有另一产品记录一概拒绝。
4. 绑定失败或结果未知时保留原操作、CAS及持久意图，进入原只读协调；不另签新版本试探，不新建替代卡。

服务器当前只提供Admin元数据／摘要，不提供可下载的原Admin secret。手机缺少原凭据时不能声称重新登录即可恢复，也不得从DeviceKey派生控制secret。凭据真正遗失的恢复另按显式恢复合同处理，根变化对任务的影响须可见。

### 3.2 删除与资源归属

- 删除、解绑或撤权一张卡，只处理该注册的服务端关系与权限，不默认`eraseAccess`、擦NVS或撤销另一产品。设备侧根级操作若不可独立完成，必须明确影响并走原根／CAS授权，不能把“只删卡”伪装为已独立完成物理撤权。
- 共用物理根意味着设备侧撤权可能耦合；首期不承诺两种注册可独立更换全部底层秘密，更不默认允许不同owner各自认领同一有效根。分享仍走原分享权限链。
- 四槽是设备物理容量，不因两张卡变成八槽；原TaskStore、scope及rootDigest保持。首期不增加BLE任务管理通道，不把Cloud任务自动呈现为另一BLE卡的任务。
- 任务、历史、共享、自动化、控制回执及UNKNOWN均按原logicalDeviceId归属，不复制、不迁scope、不复活旧ID。任何DeviceKey／Admin／epoch变化继续触发原任务权威围栏。

## 4 注册幂等与并发

独立注册需要防的是“同一次操作重复建卡”，不是“同板存在两种产品”。沿用原allocation、intent、CAS及额度锁，不建立按MAC的全局去重表。

- 在现有数据模型中明确、持久化并验证产品类别；可由原类型安全确定的旧行沿原映射，无法确定的记录标为待核对，不凭名称或MAC推断。
- 同产品、同注册意图的重发返回原ID与原结果；新产品是独立意图，分别接受额度检查。取消是否释放占用、未完成卡如何清理，沿原生命周期明确处理，不在UNKNOWN时私自删除卡或返还后又补扣。
- 同产品注册的并发保护覆盖`issue → device receipt → commit`，不能只在issue时预查一次；复用现有持久intent及必要约束。跨产品不加全局instance唯一约束。
- 同时注册不同产品仍须各自证明权限，并由原账号额度锁防止超额。第二产品存在不应触发通用“同实例重复设备”错误；真正的在途同产品认领、他人有效根、防重放及退役围栏继续生效。
- 冻结前已签intent和UNKNOWN按原协议只读协调，不追溯改写用途、换logicalDeviceId或当作新方案成功；新规则从明确的版本／启用边界起生效。

## 5 附近状态与设备卡映射

复用现有presence解析、credential store、扫描owner及`DeviceUiPort`，不增加第二套扫描器。

1. 原始扫描按原生句柄临时去重，建立连接后仍校验协议、instance及认证材料；相同MAC不能直接映射到某张卡或继承其权限。
2. 附近匹配只使用当前authority／账号下有效的presence凭据，并按该凭据明确所属的注册记录发布。不得从instance查出所有卡片后一起标“附近”，也不得把旧卡凭据自动复制给新卡。
3. 首期分卡后的presence归属须在绑定合同中明确；如保留某Cloud卡已支持的Direct能力，也必须有该卡自身合法绑定。共用根下无法区分归属的旧凭据先标待核对，不猜测，不用一个命中自动扩散状态。
4. BLE附近、已认证连接、可控制、Cloud在线继续独立；Cloud卡不能仅因同MAC的BLE广播被标云在线，配网广播不能冒充日常Direct附近。
5. 保留观察时刻TTL、后台清理、账号／凭据变更失效、迟到结果围栏和原扫描预算。不调整扫描频率，不关闭Direct功能，不重测已过生命周期矩阵。
6. 无本地控制凭据、无设备presence、权限不足、扫描受限与未命中分别说明。只修映射不能补出缺失授权，也不能把“没有附近标记”直接诊断为硬件故障。

## 6 必要消费者

| 层 | 现有入口 | 最小调整方向 |
| --- | --- | --- |
| Gateway | `services/gateway/src/device-v2/ble-enrollment.js` | 替换Cloud存在即409的跨产品前提；保留原同产品owner、intent、幂等及回执保护 |
| Gateway | `services/gateway/src/device-v2/repository.js`及management入口 | 实例解析和新增Cloud区分目标产品；不默认返回／扩展原BLE逻辑ID |
| App配网 | `esp32-provision.page.ts`、`wifiprov-allocation.ts`、`wifiprov-admin.ts` | 明确“新增Cloud产品”与“原Cloud重新配网”；保留原身份、CAS、Cloud／Local和隔离链 |
| App纯BLE | BLE enrollment、guide与credential store | 新增另一产品不误报重复；恢复仍回原ID，安全存储保持账号及注册作用域 |
| App附近 | `device-v2-ble.service.ts`、presence与`DeviceUiPort` | 按已证明注册绑定投影，不由MAC／instance扩散 |
| SDK | `DeviceAccessStore`、产品生命周期及任务权威合同 | 保留单物理根与任务围栏；只有原证明链确实缺能力时才做窄扩展 |

源码依据：[实例持久化](../src/BlinkerV2/identity/DeviceInstanceIdStore.inc)、[接入根](../src/BlinkerV2/identity/DeviceAccessStore.h)、[接入根接口](../src/BlinkerV2/interface/IDeviceAccessStore.h)、[任务根摘要](../src/BlinkerV2/provisioning/TaskAuthorityContract.inc)、[Gateway认领](../../aily-blinker-services/services/gateway/src/device-v2/ble-enrollment.js)、[Gateway实例解析](../../aily-blinker-services/services/gateway/src/device-v2/repository.js)、[App凭据作用域](../../blinker-app/src/app/core/device-v2/ble-direct/credential-store.ts)。

## 7 最小执行顺序

| 阶段 | 实施内容 | 完成条件 | 当前状态 |
| --- | --- | --- | --- |
| IP-0 | 固定分卡规则、授权及任务边界，更新主线入口 | 旧同板同ID前提明确失效，历史结果保留 | 本轮完成 |
| IP-1 | 核对两个新增方向及已有根证明，冻结产品类别、同产品幂等、根绑定与删除合同 | 给出字段／原API的最小差异；明确单根下可支持和仍受限路径 | 已冻结，见[接口合同](DEVICE_V2_ACCESS_REGISTRATION_CONTRACT_V1.md)；不代表实现通过 |
| IP-2 | 先补可信产品分类、统一额度／BLE预留，再接Cloud→BLE保根绑定及独立删除 | 新合同核心反例及原相邻有限回归通过；需要事务证据时用独立数据库 | 核心及原App装配通过，真实PG 7／7已补，见§11～13；匹配真机不代签 |
| IP-3 | 接入注册归属及附近投影，处理凭据失效 | 不按MAC去重逻辑设备，不借另一张卡的授权；生命周期有限差异通过 | 软件及相邻生命周期保留；本批Cloud／BLE两卡正向附近及旧卡不串标已验，见§14.6 |
| IP-4 | 仅构建变更实际影响的匹配App／服务／设备候选 | 原SDK定向编译／链接及配置匹配；无设备端变化则不重编固件 | Gateway沿用；两份必要App已匹配安装，8文件owner固件差异原SDK链接／运行已验，不升级SDK或wire |
| IP-5 | 条件齐备后同板最短差异验收 | 两产品ID／额度独立、恢复不新增、附近不串卡，保留授权及UNKNOWN | Cloud→BLE核心短验有限通过；覆盖旧测试身份为用户明确例外，不代表原任务迁移已验；反向及欠项见§14.7 |

IP-1只审阅实际消费者，不重开全部身份、资源或故障审计。优先复用现有存储和协议；若某路径缺设备根证明，写明一个具体缺口及最小补口，不用“待完善安全”无限延长阶段，也不靠删除校验强行通过。

IP-1已明确无需新增设备wire：保根绑定复用原App侧Method 2确认，服务端nonce接入原报文，但服务器不能独立验设备secret。当前BLE首次认领缺额度预留，必须在签发前补齐；删除按卡处理并保存仍被另一卡引用的根凭据，不把服务端删除当物理撤权。BLE→Cloud加Key会改变任务根，GetInfo不提供空TaskStore证明，因此v1该方向设明确能力门禁，不退回旧同ID升级。后续按[接口合同§8](DEVICE_V2_ACCESS_REGISTRATION_CONTRACT_V1.md#8-ip-2-的最小实施与验证)执行，不默认部署、刷板或推送。

## 8 有限回归与验收矩阵

| 场景 | 必须得到的结果 |
| --- | --- |
| 同MAC、不同有效instance及凭据 | 在各自可区分且认证成立的连接上独立注册；注册层不按MAC合并／拒绝，底层地址冲突不猜测归属 |
| 同instance先Cloud后BLE Direct | 满足已有根证明后新建BLE ID；Cloud ID、授权及业务数据保持 |
| 同instance先BLE Direct后Cloud | 满足已有根证明后新建Cloud ID，不默认enable-cloud到旧BLE ID |
| 同Cloud设备切换WiFiProv BLE／SoftAP | 仍为原Cloud ID，不新增卡或额度 |
| 同产品重复提交／重连／缓存页或进程恢复 | 原操作、原ID、原扣额；不多发Bootstrap或WiFi |
| 同产品并发、另一产品并发或额度不足 | 同产品原intent正确串行；跨产品独立且不超额；失败不破坏原注册 |
| 他人有效根、凭据缺失、epoch不符或仅有公开instance | 不接管、不清根、不伪造完成，也不泄漏其他账号信息 |
| 已签intent或UNKNOWN | 保留原合同和只读协调，不换ID、重放或补造成功 |
| 删除一张卡及物理根变更 | 另一卡和原任务不被静默删除／迁移；根级影响显式处理 |
| 附近命中及同MAC／instance多卡 | 仅投影到有合法凭据绑定的注册；不等同Cloud在线或控制权限 |
| 后台、账号切换与凭据失效 | 沿既有围栏退休旧证据，迟到结果不回填，不自动重试业务写 |

软件回归先使用原模板、原repository／协议及受控夹具；数据库只补新合同必需的事务边界，不再单独完成旧DA-1“跨产品必须拒绝”的过时发布队列。真机阶段只验证新增差异，不重复已通过的WiFiProv扫描连接取消、完整配网、自动化、定时或BLE长循环。

## 9 当前现场及交付边界

最新运行发布与手机结果取§13；[原接入计划§15](DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md#15-da-4com11应用切换与一次ble选择取消短验2026-11-03)的无线有限结果继续保留。COM11仍保留BLE维护候选与原存储，不是日常常驻Cloud／Direct运行固件；本批只枚举串口，没有打开串口、复位或刷板。匹配App保持Direct开启，但原本机BLE凭据为0，不能以登录、公开instance或服务器Admin摘要补出原secret。

不合并／删除历史双记录，不迁任务scope或业务数据，不清根、不覆盖未知意图，不凭旧备份回滚合法新状态。不升级SDK、不扩槽、不新增纯BLE任务或多根／多租户硬件能力，不调用模型、不发验证码、不自动重发，不处理软件会话的整页编辑器、IP定位或地图扩展。

必要交付仅包含实际修改的源码、有限回归与本方案；私有凭据、原NVS、账号日志、构建产物均不进入公开配置或Git。无关本地修改保留，暂不提交推送。

## 10 IP 2 前半批执行结果

状态：**PRODUCT_CLASSIFICATION_AND_SHARED_QUOTA_OFFLINE_PASSED_NOT_DEPLOYED**。本批只处理已冻结的分类、查询及计量基础，不开新注册窗口、不改旧根或删除历史设备。

### 10.1 最小源码接线

- Gateway新增`registration-product.js`与独立迁移059；列可空，无默认产品。Cloud仅由匹配创建日志识别，BLE需完整commit证据且owner／tenant一致；混合用途、Cloud历史或归属矛盾保留null，不依据MAC、名称、deviceType或在线状态猜归属。原058及历史迁移保留。
- 原Cloud INSERT固定写cloud，BLE首次commit INSERT固定写ble_direct；已有ID恢复不改分类。原仓储映射及公共目录／详情只读取持久分类，不接受客户端传入值覆盖。
- 原resolve-instance增加`access-registration-v1`显式版本，只按目标产品读取两个既有可信关联来源，不再因Direct存在遮住另一Cloud卡；相关未知旧行或目标产品多ID明确409。原legacy和Cloud入网确认版本不变，新增版本严格字段、非零canonical instance、owner围栏及所有早退／OPTIONS禁缓存通过。
- `registration-quota.js`提取原权益读取和归属核对；在原user→tenant锁内，对未deleted逻辑ID与grant_issued／expired BLE意图目标做UNION。现有签发intent就是预留，不另建额度账本；恢复已有目标不双算，原精确重放及commit不新验额，过期未决grant不按TTL释放。
- BLE新签发在原身份／epoch／presence检查后、删除旧pending材料或supersede之前验额。拒绝时不签grant、不清旧未知材料；与Cloud创建共用一个计量口径。没有添加设备写入、业务重试、SDK或依赖升级。

以上仅是两个现有入口的共用计量。将来保根intent真正接入时，其预留也必须加入同一计量模块，不能另设一套额度规则；本批没有假造尚不存在的数据表或签发用途。

### 10.2 反例与有限回归

| 范围 | 实际结果 |
| --- | --- |
| 产品列／来源迁移及原registry | 原缺059的4项红测失败；修后12项队列11通过、1个DDL正则误捕获后续IS NOT NULL，收窄到首DDL后只补该1项通过；不是生产DDL失败 |
| 共用额度薄模块 | 旧logical-only反例6失败／14通过；UNION修正后20／20通过，随后仅补新增5项SQL及异常计数检查，5／5通过 |
| 两个真实repository新用例 | 首轮12项中9个产品／接线失败、2通过，另1个fixture直接持有SQL Buffer导致摘要被原清零误报；只修fixture复制后该项单独通过。生产接线后12／12通过 |
| 清理顺序与删除不了旧意图的保护 | 另追加4项：满额BLE续签保留原expired材料、旧分类不被恢复重写、共享末槽、HTTP403；连同收窄到实际锁参数的原1项，仅跑该5项，全部通过 |
| 按产品仓储／管理查询 | 修前6失败／2通过，修后8／8通过；原Direct优先的legacy读取保留，不把新按产品结果称所有旧入口已改完 |
| 实际HTTP dispatcher | 新合同21个子用例修前19失败／2通过，修后21全过；Node含父用例计22／22。包含原legacy与Cloud确认的隔离，不连接HTTP服务或数据库 |
| 受影响原相邻 | 原BLE signer／intent脚本通过；原Cloud连续性12项及Cloud只读HTTP3子项通过，Node计16／16。旧保护只签旧空根入口不被误放宽，不代签新保根注册 |
| 原依赖静态核查 | Gateway `tsc --noEmit`及必要diff空白检查通过；未安装／更新依赖或改锁文件 |

首轮fixture停点、正则误报及旧保护通过结果均保留，不将它们改写成首轮全绿或新产品已交付。并发用例的内存SQL载体仅模型事务序列化与回滚，不证明PostgreSQL真实锁、DDL、SQL语义或查询计划。原PG检查已明确SKIP，不能计为通过。

证据在Service忽略目录`build/ip2-registration-product-20261103/`及`build/ip2-registration-foundation-20261103/`。部分测试完整输出仅在本轮工具记录，磁盘摘要只列实际存在的结果；不补造raw日志。新回归接入Gateway `test:registration-foundation`，不默认运行整个Device V2历史套件。

### 10.3 下一批最小范围

以下为前半批结束时冻结的后半批范围，已按§11推进，不重新派发已通过项：

1. 先以原共享合同补保根intent／注册槽的离线反例，固定nonce、root CAS、摘要编码与120秒无写入期限；未提交、已提交及响应丢失各守原终态。
2. 实现同一数据库中的Cloud→BLE持久intent／确认绑定，并将预留纳入本批计量；新绑定进入原有效Direct消费者。共享根变更围栏与新绑定删除分支须同时接入，不能只插一条BLE卡放开首装保护。
3. 再接App窄Method 2 nonce及单份根凭据引用、按卡回收；保留原UNKNOWN和所有未决引用，不调用clearAll或eraseAccess。BLE→Cloud的任务区证据门禁保持。
4. 真实事务验收使用隔离数据库；匹配部署和真机仍按IP-4／5，不以当前COM11维护候选补造日常Direct／Admin能力。

当前跨产品注册、共享根删除／附近引用并未整体完成，IP-2不关闭。App、固件、账号、授权、TaskStore、scope和UNKNOWN本批未操作；无关dirty与旧完整日志保留，未部署、刷板或提交推送。

## 11 IP 2 保根注册共享根引用与独立删除

状态：**核心源码与有限离线回归通过，尚未部署；IP-2用户入口未整体收口。** 本批支持Cloud→BLE Direct的保根绑定，不安装第二Admin，不改变DeviceKey、epoch、TaskStore或云scope。BLE→Cloud加Key仍需任务存储边界证明，不能退回旧同ID升级。

### 11.1 服务端核心

- 新增独立合同、仓储和HTTP薄适配，接原Gateway鉴权、限流及日志入口。prepare／read／commit／cancel严格字段且禁缓存，owner／tenant不接受客户端覆盖；不返回秘密或安装grant。
- 迁移060保存物理根引用、逐产品注册槽及保根intent。原user→tenant→root→device锁序下，prepare预留独立目标ID，原槽保护延续到提交；commit重核原根CAS、源卡、nonce及120秒期限后原子创建BLE卡。已提交精确重放先读取原结果，目标删除后不复活。
- 原共用额度计量增加尚有效的只读prepared目标，按ID去重；其超时可释放未写设备的预留。原BLE可安装grant即使expired仍保留未知占用，不复用新TTL规则清理旧UNKNOWN。
- 有效Direct实例视图纳入新绑定。删除只退休目标卡／引用；删源卡取消其未提交目标，保留已提交另一卡；新BLE删除核本合同committed证据，不伪造旧Bootstrap回执。`localAccess=not_confirmed`保持，`rootDisposition`仅表达是否仍被其它注册引用。
- 原Admin元数据／reset、DeviceKey Rotate及presence写接共享根保护；原精确已确认重放不被新写门禁误拦。已知根变化使旧绑定stale、未决intent关闭；共享根未具备安全协调的写入口明确409，不静默覆盖另一卡材料。
- 源卡删后若旧presence仍被物理根使用，保留加密材料及审计，但关闭源卡ACL。剩余BLE卡不能另配一把版本1覆盖它；附近材料归属及投影留到IP-3，不把本批写保护称为附近已可用。

### 11.2 App 根引用及操作协调端口

安全存储新增v7逐卡引用，秘密只存在同authority／账号作用域的一份根记录；新BLE引用不继承源Cloud的presence展示权，不制造首装receipt。旧v5／v6只在显式保根绑定时先写并核根项、再替换源卡引用、最后写目标引用；写入中断不先删原秘密，不迁旧pending／UNKNOWN为active。

原Direct Client增加限定用途的新鲜Method2端口，将本intent服务端nonce放入原auth-init。核原instance、epoch、controller、fingerprint及Cloud版本，不复用ready会话，不新增设备wire。原日常认证继续自行生成nonce。

薄API和协调器提供prepare→本地pending引用→新鲜认证→commit；传输未决只保留原checkpoint，显式GET协调，不自动重发POST或设备配置。确认已落库但本地收尾失败单列`localReferencePending`，不撤回服务端成功。checkpoint最多32条，不为新操作淘汰UNKNOWN；仅权威终态且本地处理完成后可显式清理，不删完整journal。

同账号作用域修改串行，排队开始及破坏性操作前后均核当前scope。generic remove保留本批v7未决引用；权威closed才窄退休目标。删除源卡保留目标所依赖根。已知根失效端口标记整个根stale并通知所有引用；新协调器的invalidated恢复已接入，**其它原权威消费者尚需下一批装配**。旧v5／v6的cancel确认后清理语义不在本批全局改写。

### 11.3 有限验证结果

| 范围 | 结果及边界 |
| --- | --- |
| 服务端合同、仓储、生命周期、真实HTTP dispatcher及Key门禁，连同受影响迁移／原签发准入 | 最终组合96／96通过；使用真实产品模块与内存SQL载体，不连接业务库 |
| 共享额度 | 本批扩展29／29通过；只读prepared期限与旧signed UNKNOWN分别计量，不在最终组合重复运行 |
| 跨端摘要 | 改为合法的公开合成Cloud／BLE ID后，Gateway只补该1项通过；固定JSON数组及摘要与App公开fixture对照，不放宽原ID校验 |
| App核心 | 原store／client／presence-refresh、共享引用、新鲜nonce、新HTTP／协调／checkpoint共6文件71／71通过；不代签页面或无线行为 |
| 静态编译 | Gateway原依赖`tsc --noEmit`通过；App用既有WiFiProv源码路径覆盖的`tsc`及最终Angular `ngc --noEmit`均通过，不安装／升级SDK或依赖 |

保留最初红测、SQL夹具别名／加载时机及摘要fixture的修正记录，不改写成首轮全部成功。Service证据在忽略目录`build/ip2b-access-registration-20261103/`，App证据在`build/ip2b-app-registration-20261103/`；部分完整输出仅在工具记录，磁盘摘要不冒充原始日志。新增Gateway脚本`test:access-registration`供核心回归使用，不默认拉起全量历史套件。

本机未发现原生PostgreSQL工具，**未运行060迁移或真实事务测试**。静态DDL与内存事务载体不能证明PostgreSQL解析、锁竞争或SQL执行。本批不部署、不安装App、不操作手机／COM11或账号，不改SDK、锁文件及现有无线状态，不提交推送。

### 11.4 下一批最小范围

先补IP-2原入口，随后IP-3附近投影；不重做原配网扫描、正常入网、定时或自动化：

1. **先消除原App入口歧义。** 原WiFiProv无产品resolve在Cloud＋BLE共存时明确409，`findForProvisioning`也明确歧义，并非任意选错。复用现有resolve增加显式cloud产品用途；把“是否存在任意接入记录”与“选原Cloud凭据”分开，保原ID／instance／epoch／CAS，不按共享根折叠两张卡。
2. 通过原BLE radio owner／账号／可见性生命周期装配保根添加入口、显式UNKNOWN恢复及独立删除收尾；把原权威根变化消费者接全引用失效端口。没有原Admin secret时保留具体门禁，不以登录补出凭据。
3. IP-3再接注册归属及presence单一写入权／附近投影，不从同MAC或instance扩散附近／在线，不调整扫描节拍。
4. 部署前仅补两组独立PG验证：059→060事务迁移／约束／双产品视图及删源后目标关联；prepare／commit／删除并发的唯一槽、回滚及终态持久性。随后只构建实际受影响候选，条件齐备再做同板最短差异真机。

本批不将旧空根入口的Cloud冲突保护直接移除；新增产品必须走新保根用途。原TaskStore、scope、授权、UNKNOWN和完整日志保持，无关本地修改保留。

## 12 IP 2 原 App 产品入口与生命周期接线

状态：**App软件装配有限通过，未部署；IP-3附近映射及数据库／真机不在本批通过范围。** 继续同实例两产品分卡、分ID、分额度，WiFiProv BLE／SoftAP不分卡。没有新增设备协议、扫描器、物理Admin或任务scope。

### 12.1 原 WiFi 配网明确选择 Cloud

原管理服务增加可选产品参数，发送既有`access-registration-v1`合同；核200、no-store、目标产品及有效Cloud上下文，不降级到legacy实例优先。原WiFiProv页面的Cloud选择全部显式带cloud，空根首次Cloud不存在后再核BLE关联，已有BLE不被当作“全新无记录”创建Cloud。

本机`inspectForProvisioning`只返回同instance是否有任意引用／未确认引用，不替用户挑卡；原preflight再用`findWiFiProv`选择Cloud凭据。合法双卡不会因通用多匹配被误拦，但另一引用pending／轮换UNKNOWN仍阻止写。旧混合记录无法分类时保持明确409，不猜归属。WiFiProv页面和薄端口移除旧同ID开云用途，BLE→Cloud仍保留具体能力门禁。

### 12.2 原 BLE 添加页装配保根用途

新增独立view-state薄模块，原页面只负责展示和账号／可见性生命周期。用户先从本机有效Admin所对应的原Cloud卡中选择源，再通过原扫描器显式选择Direct运行候选，之后才调用保根prepare及新鲜Method2。即使只有一个候选也不自动认领；无presence时仍可显式选择一次，由原认证最终核instance／secret，不按MAC认卡，不遍历设备自动认证。

扫描候选持当前账号／前台epoch及原扫描signal票据；离页、取消、后台或换号后失效。页面保留扫描signal至本次认证结束，旧首装恢复与新保根选择也相互退休，不留下可再次使用的旧候选。原空根首装入口及其owner保护不被放宽。

添加记录只在本机枚举，原结果由用户显式GET读取；UNKNOWN不自动重发、继续或取消。只有本地phase及权威intent均prepared时才显示继续／放弃，继续仍须新显式候选选择。已确认但本地引用未完成单列提示，不冒充可用卡；终态清理不删journal。成功页区别“注册已确认”和原首装的Manifest／State已同步；后续列表刷新失败保留确认，显式完成按钮才再次刷新。

沿用原卡片、按钮及主题token，明确额外一台额度、原WiFi卡保持、取消和错误恢复。新增操作文案覆盖注册的11种locale；非中文新文案暂用英文，不改语言清单。新提示与操作不以颜色或技术错误详情代替文字说明。

### 12.3 删除及已知根变化的实际消费者

- 中央`WiFiProvRetirement`及设备设置接权威删除收尾。新BLE卡没有WiFi allocation，也必须按本ID移除active本机引用；根已stale时不必先展开秘密才能删除。另一卡及根仍有引用时保留，pending／轮换UNKNOWN继续留待协调。
- WiFi UNKNOWN只读恢复依赖非秘密allocation上下文，不依赖active Admin。因此删除保留原UNKNOWN journal，但仍移除当前active凭据，避免已删除卡继续控制。仅发当前卡删除事件；不会全根失效或断另一产品的会话。
- DELETE确认先于本地I/O反馈，本地清理失败仅追加待办，不撤回确认。仍不承诺MCU或其它手机已物理撤权。
- 设备设置及原WiFiProv Key Rotate／Admin reset，仅在精确新版本回执及原上下文匹配后使旧共享根失效；重放旧版本不会失效较新的本机绑定。读取、普通失败或presence命中均不是根变化证据。
- 根失效事件关闭相关引用会话，删卡事件只关闭目标会话。仍复用原store、原账号作用域与原disconnect路径，不把生命周期接入错误地塞进Cloud执行服务。

### 12.4 有限回归与编译

| 范围 | 实际结果 |
| --- | --- |
| 显式产品API、引用检查、写入preflight、管理薄端口 | 新反例7失败／2通过；修后相关49／49通过 |
| 原配网页及产品／根确认差异 | 原四文件110／111，唯一旧DELETE fixture停点修正；最终原page＋port 73／73通过，其它原已通过项不重跑 |
| BleService装配及原附近生命周期 | 新入口11项分批通过，受影响原生命周期11项通过；另补当前卡删除事件1项通过，不算IP-3附近正向 |
| 删除／根生命周期 | store 3项、设置2项先红后绿；相关四文件89／89通过；后补WiFi UNKNOWN保记录、矛盾轮换记录拒绝及单卡断开各1项定向通过 |
| 原 BLE 页真实模板 | 原页＋冲突＋新入口组合32／32通过；随后仅补新差异与文案覆盖，新入口最终10／10通过，与组合8项重叠，不累计为42项 |
| 原依赖编译 | 既有WiFiProv源码路径覆盖下，最终Angular `ngc --noEmit`通过；没有SDK／依赖／锁文件升级 |

保留runner未装JIT／过宽test include、旧fixture缺新端口、mock历史未清、zoneless测试未等待本地Promise及一次模板action错误的停点；已分别修正，不重写成首次全绿。证据摘要在App忽略目录`build/ip2c-{cloud-context,app-registration,lifecycle,entry}-20261103/`；部分原始输出仅在工具记录，不补造raw日志。

本批只读串口枚举见COM11，没有刷板、重配、认证设备或制造真实注册。060尚无真实PG迁移／事务结果，未部署新Gateway或安装App，故未以现有服务404或维护固件冒充产品缺陷。模板测试不证明手机375px／横屏／大字／深浅色的实际视觉效果，留匹配安装短验，不将jsdom截图当实机结果。

### 12.5 下一批最小范围

进入IP-3：先明确共享根presence的单一材料归属与逐卡绑定投影，再接原凭据／附近消费者。新BLE引用当前不自动继承源卡presence；不得为了点亮附近把同MAC／instance的全部卡一起标命中，也不能各装一把互相覆盖的presence。保留原Direct功能及已通过的后台／账号／TTL矩阵，只补真实新增差异。

部署真机前仍仅需§11.4列出的两组隔离PG验证及匹配Gateway迁移，之后核当前账号、COM11实际身份、原Admin和声明Direct运行候选。没有原秘密时保留具体门禁，不重登补造凭据、不清根或重配凑样本。条件齐备后才做两卡ID／额度、恢复不新增、单卡删除及附近不串卡短验。

本批没有更改实际账号、授权、TaskStore、scope、UNKNOWN或日志；没有发码、自动重发、SDK升级、部署或Git提交推送。无关本地修改保留。

## 13 IP 3 共享根附近投影、数据库及匹配发布

状态：**软件、真实隔离PG、Gateway部署及匹配App安装已完成；真机正向两卡仍有具体前置，不列为全部接入收口。** 本批复用原presence、原扫描器、Method 2、鉴权和引用生命周期，无设备wire、SDK、扫描节拍或物理任务容量变化。

### 13.1 单一材料owner与逐卡授权

- 新只读`GET /api/v2/access-registrations/presence/{logicalDeviceId}`沿原App Auth、owner／tenant、限额和no-store，核有效member、完整根CAS及BLE committed证据。只返回最多两项设备已确认current／previous；无材料为空，不allocate。原Cloud材料ID继续作为vault AAD，不复制密文后改属另一卡。
- 仍active的唯一原Cloud材料owner可沿原allocate／install／ACK／confirm维护根内一份材料。BLE别名不能另装一把版本1；源Cloud删除后保留的确认材料只读，不向BLE交接writer，其它共享根写围栏不放宽。
- App安全根只存一份秘密；在原显式prepare捕获`bindingRevision`和六字段`expectedRoot`。GET仅授予本次目标卡presence读取权，不按instance、MAC或根引用遍历授权其它卡。根／绑定变化先退休旧附近，再写安全存储；同版本换bytes、版本倒退、pending冒充active均拒绝。
- 尚未确认安装的材料仅存writer私有candidate，供显式原认证／UNKNOWN协调，不进入附近证据，也不给alias。精确确认后退休candidate；读取较旧confirmed不丢较新UNKNOWN意图。
- 物理HMAC观测和逻辑卡投影分离。多卡必须各有同根、同revision、同材料owner及同版本bytes的有效绑定；未绑定同key或不同根仍歧义拒绝。附近不修改Cloud在线或控制通道。
- 原TTL、观测时间、有界队列、扫描owner、后台／账号取消继续保留。alias只GET；初次最多追加一次已明确命名且本机仍active的原owner GET，不做instance遍历或前台轮询。相同材料／grant重读为no-op，不重复写存储或打断扫描。

初始化边界明确保留：旧根缺CAS上下文返回`CONTEXT_REQUIRED`；无原Admin不能保根Method 2；无confirmed材料且无原source已认证会话不能替设备确认安装，alias返回`SETUP_REQUIRED`、writer保留原`SESSION_NOT_READY`。这些不是关闭Direct，也不靠自动重放配置绕过。

### 13.2 有限回归与真实PostgreSQL

| 范围 | 实际结果 |
| --- | --- |
| Gateway presence／原HTTP dispatcher／共享根生命周期 | 最终组合52／52；原PresenceKey非PG定向通过，不把其当时skip计作数据库通过 |
| App安全根／逐卡grant／candidate／HTTP合同 | 新反例及原引用相邻用例分批通过；保留早期fixture停点及修正，不累加重叠队列 |
| 附近投影及原附近生命周期 | 最终组合28／28（17新增＋11原有）；原匹配／无隐式acquire定向5／5，非累计 |
| 公开Nginx窄入口 | 新6／6、相邻23／23；关闭时128种原配置组合保持原输出 |
| 静态及制品编译 | Gateway原依赖tsc及实际overlay编译通过；App既有WiFiProv源码路径覆盖下最终Angular ngc通过；依赖／锁文件不升级 |
| 隔离真实PG 16.15 | 7／7、0fail／0skip；真实059→060升级、约束／视图、并发prepare／commit／remove、额度回滚、关闭持久性、源删后目标／confirmed材料保留 |

PG使用129既有PG镜像启动独立内网、无主机端口、tmpfs数据的临时容器，全部为合成身份；不复制业务账号或任务。测试前后既有消费者保持，临时容器、网络及临时凭据已删除。新公开脚本`test:access-registration-postgres`仅在显式测试数据库运行；不得将内存SQL载体或skip代签真实数据库。

### 13.3 匹配Gateway有限发布

先保护必要配置、完整旧日志和一次可读回目录的数据库dump，再在129 IoT环境仅以原Gateway镜像叠加18个审阅编译消费者；未全量覆盖远端源码或其它AI／Auth模块。运行image为`sha256:7d29d6e56f4557937ed28c0045204399a7ccea09bb93971bca792d4b43395c5b`，18项运行hash匹配，健康且无重启／OOM。

业务schema仅新增059／060，实际历史1～60完整。Gateway环境变量名称／值、挂载及端口保持；Docker重建只改变环境列表顺序。原Auth／Broker及另外21个容器ID保持且未在发布期间重启；远端`stao`源码HEAD和未提交状态指纹保持，正式服务器未操作。

Nginx仅插入六类审阅窄入口，放在旧宽正则前，保留原配置其它字节；`accessRegistrationEnabled`用于以后相同生成，不能再用缺此开关的模板覆盖。实际`nginx -t`通过、仅reload一次。最终未登录presence／prepare为401及private no-store，未知子路径404；未作真实注册或配置写入。

发布停点照实保留：旧配置缺模板注释时在修改前停下；BuildKit把immutable ID误当仓库名时未动服务，后用核对同ID的本地alias；重建后环境顺序及一次手录容器ID检查停点经只读协调，未二次重建；reload后的立即探针命中旧worker 404，后续原配置只读核对及公网401确认新代生效，没有二次reload、迁移、恢复数据库或业务自动重试。证据在Service忽略目录`build/ip3-release-20261103/`，私有日志／dump不进入Git。

### 13.4 匹配App安装与手机有限差异

仅一次现源码stage→web→Cap sync→原版本离线Gradle构建，1158个web资产、HTTPS、原生插件注册及签名匹配。APK SHA256 `45d74e2b5e18aec266990bc3a7f9870bb9993f738eca6098dc9f31eb024baec3`；包名`iot.diandeng.tech.beta`、WiFiProv 2.4.4不变，实际打包Direct／tasks／automation为true，未关闭需要的功能。

安装前以私有ACL保护账号及完整journal；仅`install -r`一次，不卸载、clear data或导回旧token。安装和短验后同账号／Development保持，7项安全存储逐字节一致、两份原journal store保持（原有行数为0，不能说存在UNKNOWN样本）；运行环境模块源hash与该APK匹配。

原BLE添加路由可达，保根源卡0、checkpoint0，显式本机记录按钮可用，没有凭空显示可选原Admin。一次有界原生观察最终确认`pause → appStateChange(false) → appStateChange(true) → resume`，观察窗注册及其它业务请求均0；只移除新增观察监听，不动产品监听。

保留验证工具停点：此WebView不发document可见性事件，且onPause时`App.getState()`仍可active；最终改核产品原本使用的原生pause／stop／resume，不以JS接收时间冒充物理回收预算。模块资产以实际Debugger source而非只列classic script的资源树核对；未因此重建或重装。当前没有活动扫描／认证资源，所以只证明生命周期到达和无自动业务调用，**不代签实际无线连接释放或正向两卡附近**。App证据在忽略目录`build/ip3-matched-app-20261103/`。

### 13.5 下一批唯一最小范围

不再重复迁移、构建安装、WiFiProv扫描／完整配网或已通过生命周期矩阵。先只核以下具体前置，再决定是否需要一份日常运行候选：

1. 当前账号能否取到与COM11原根一致的本机source Admin。现装手机新鲜枚举为0；登录或DeviceKey读取不能恢复该secret。只能复用仍合法的原凭据转移／恢复入口，不能从旧记录补造、直接读设备秘密或默认清根。
2. COM11原NVS／TaskStore／云scope保持下，是否已有声明支持Cloud＋日常Direct、匹配identity／profile的运行制品。当前5f9a维护候选不满足；本批无设备代码变化，不为软件接线重复烧录编译夹具。
3. 条件齐备后，只做一次Cloud→BLE保根注册：独立ID与两额、原ID／任务不变、UNKNOWN显式GET不新增；再按原writer确认材料，alias GET（必要时加一次owner GET）及一次原2.5秒扫描核逐卡投影。无confirmed材料时先复用原显式认证安装口，不让alias创建材料。
4. 同一批仅补目标卡独立删除后源卡／根／任务保持；若要删原Cloud源须先明确最后writer消失后的只读支持边界，不为凑矩阵破坏现有任务。BLE→Cloud任务区证据门禁继续保留，不退回旧同ID升级。

前置缺失只报告该具体条件，不将整个IP-3判为未实现，也不关闭Direct或重开资源／配网审计。本批未改账号额度、授权、设备NVS、TaskStore／scope或UNKNOWN，未发码、自动重发、升级SDK或Git提交推送；无关本地修改保留。

## 14 IP-4 新身份 Cloud 首装及保根注册差异（2026-11-03）

### 14.1 明确授权与基线更正

用户明确允许仅COM11覆盖原测试身份、不要求恢复原程序／NVS。因此本批不再坚持§13.5的旧Admin恢复前提：核唯一USB及原分区后，仅清空`0x9000+20 KiB`，由设备生成新instance，不由主机指定。保留旧云记录、原日志及已存在UNKNOWN，不搬旧scope。没有重新制作8 MiB备份，也没有操作其它板。

§13手机枚举工具此前误用安全存储前缀，须更正解释：原7项安全存储中实际有3份旧凭据，均不属于本轮Development账号；当前账号可用原Admin确为0。它们未删除、导入当前账号或跨账号复用。该工具错误不等于App产品过滤错误。

测试Auth仅通过既有审计override API将本账号`iot.devices`从1设为10并保留，套餐仍为Free，其它权益不改；不是直改数据库或关闭额度。全程只使用一次用户批准的真实登录验证码，正常登录后保持同一Development账号，无后续发码或换Auth。

### 14.2 已完成的真实 Cloud 首装

复用已编译5f9a维护固件、原非空PoP与Security1，在入口全备齐及认证成功后启动原600,000 ms窗口。真实WiFiProv链完成：一次扫描／选择、目标网络列表、一次创建及Key读取、一次Admin安装、一次Bootstrap精确`[2,3,0]`ACK、一次WiFi下发；Cloud确认后正常自动进入新设备页。

新Cloud逻辑ID为`device_bb5a28d7-be6d-477c-b07b-6e60b128b113`，名称“IP4 同板 Cloud”，新Admin已由真实App保存。随后仅应用区切换为原f878日常Cloud＋Direct四槽BIN，新20 KiB NVS及原分区在启动前逐字节核对保持；没有再次清根、创建身份或重配WiFi。没有在这条新身份上初始化、Start或重放任务；旧任务不搬迁。

完整账号、请求journal、网络与原生事件私有保留在App忽略目录`build/ip4-independent-live-20261103/`指向的ACL目录。原记录不为凑验收而删除。此处只签Cloud首装及新Admin，不代签两卡注册、附近投影或删除。

### 14.3 注册边界与 App 最小修正

首必要App候选SHA256为`bce63be7b3f52fe26ff951d41dc5c6c21e48e1f26f9175dab613e59af6473014`。真实首次prepare收到明确`409 / ACCESS_REGISTRATION_ROOT_OPERATION_PENDING`，并非不确定提交：实际仓库先查同intent重放，该guard在新intent、额度预留及目标member写入之前；事务回滚。原页面错误地把这一明确拒绝记成UNKNOWN。

- 仅begin/prepare阶段的准确Gateway错误status＋code可记严格`rejected`终态；commit同码、超时、其它409／404、相似普通Error仍保留原UNKNOWN语义。rejected允许显式清理，不显示无意义的结果读取，不触发HTTP或无线恢复。
- 对本批旧误分类记录，只做一次有原始409完整回执及同账号／同request GET404佐证的限定维护协调，先完整归档原UNKNOWN和回执，再改成严格rejected；没有重发旧POST。产品未加入“404就清UNKNOWN”规则，其它记录不动。
- 原设备页已分配首次presence，但维护固件没有日常Direct会话，未完成安装确认；不能让BLE alias另分配密钥。显式选中目标后，只有旧v5 WiFiProv源、wire v2、初始version1／同epoch／无previous、精确匹配本机材料时，先沿原source新鲜M2、同步、安装及Receipt确认，精确读回并关闭；然后才prepare及另一条nonce-bound M2。无allocate、旧ready复用或guard放宽。
- 注册commit及本地引用激活后，仍沿唯一Cloud writer完成附近材料确认并单独GET授予alias；初始化失败只显示附近设置待完成，不撤回已确认注册，HTTP-only恢复不写设备。

第二必要App候选SHA256为`46ca8474da7a647b3a368c439bcc395322eab408d541bd55ca5c8b8bddc54bd6`，已一次覆盖安装，账号、非会话安全存储及完整journal保持。两次构建是先后真实缺陷修正，不是同输入重复队列；原WiFiProv 2.4.4、SDK与依赖锁不变。

有限源回归分批记录，不能相加：原精确三文件55／55；严格终态及相邻四文件78／78，另一个own-field反例通过；新增前置差异16／16，旧78不重跑；最终原真实模板12／12及Angular ngc通过。初始化pending与明确rejected有独立文案，沿现有组件，不扩大页面设计。

### 14.4 真实停点与已确认根因

第二候选的有限差异中，原生connect、正确Direct GATT服务、通知和write均成功。定向调试断点确认：M2、Manifest、State已完成，失败请求为`PresenceKeyMutation(0x34)`，等待`PresenceKeyReceipt(0x35)`，设备返回准确相关sequence7的wire error1。不是扫描失败、协商超时或注册提交UNKNOWN；尚无新的prepare／commit。

冻结f878的真实SDK单元离线复现：epoch1／expected0／version1的同一合法报文，无任务owner时安装、Receipt验签和存储读回通过；持原`TaskRootOwnerGate`时返回`WouldBlock(22)`、无Receipt；释放owner后通过。实际链为`TaskProductLifecycle`持owner→transport／Client／BLE队列dispatch→Presence写→同Store再次申请gate；`DeviceRuntime.mapError`默认将WouldBlock映成MalformedMessage1。CBOR／flags合同无差异，不改编码、鉴权或服务端guard。

最小修正限定为已认证同步Presence的显式owner借用：保持busy锁、普通原生写互斥及CAS，写前使management authority失效，写后在原owner边界重新核双根再允许任务处理；不把整个transport移到无锁区，不提供一般可重入锁。未知写入继续poison，同值早退也不得绕过这一保护。冻结复现与有限回归证据在Library忽略目录`build/ip4-presence-frozen-probe-20261103/`；匹配固件及两卡真机剩余项待本修正的定向完整链接后执行，不再发码或重配。

### 14.5 设备侧最小修正及有限回归

本次生产差异限定8文件：两项observer／Store接口、`DeviceAccessStore.h/inc`、原`ControllerControlCoordinator.inc`、`TaskRootOwnerGate.h`、`TaskProductLifecycle.h`及`Esp32ProvisionedTaskExtension.h`。只给原同步Presence路径命名的cooperative入口，非任务profile默认沿普通fence；原bootstrap／rotate／revoke不借用。没有设备wire、CAS、认证、TaskStore格式／槽位或SDK版本变化。

借用要求当前active且已持owner，拒绝第二借用；整个写期间保留busy原子锁。写前立即`setTaskReady(false)`并停止／失效原management，同一次transport poll后面的旧请求不能继续。写后不直接解锁，transport完整返回后才leave／reenter原owner，消费dirty、重读selector及双根authority，再恢复management／engine；抢锁失败跳过后段且仍处理停机收尾。已准入写失败保留uncertain，不能stop/start或用同handle读回解毒。

审阅补齐一个具体反例：原same-value精确重试会在observer前早退。现在只有新cooperative入口的精确重试也必须经过fence；仍不写Store，普通原路径不变，避免未知commit后把同handle未提交视图当作持久成功。

有限host `/W4 /WX`通过：Store／Coordinator新增3组及原Store／Presence合同；owner集成新增5场景、原QA3受影响6场景与NoOwner7场景分别通过，不累加重叠执行次数。保留冻结旧实现先红的WouldBlock反例及新入口缺失停点，不改成首次全绿。

完整架构checker另停在未纳入本修正的既有dirty文件：`BlinkerFacade.h`、`Esp32WiFiBLEProduct.h`的DeviceKey login freeze，以及`BlinkerWiFiBLE.h`选择器合同。没有为本批放宽freeze或混入这些修改，故不称架构全绿。原SDK单候选只复制冻结f878图并叠上述8文件，审阅清单、唯一编译intent及完整日志在`build/ip4-owner-fix-20261103/`；不另开重复队列。

### 14.6 匹配固件及两卡有限真机结果

唯一原SDK 3.3.11单任务编译、完整链接及原消费者审计通过。BIN为`e48cffa2953130d37455ca886c3f6a96071894bd070e8000bb5788e98d128f07`，1,438,176 B，静态RAM 60,116 B，比f878增加8 B；原分区、PoP、profile、两标量和四槽图不变，实际cooperative消费者已链接。没有全量混入当前dirty源码或另构建App／服务。

仅COM11应用`0x10000`写入一次；当前f878先verify，新鲜20 KiB NVS读后verify，写后新BIN／最新NVS／原分区均verify通过才一次启动。没有NVS写入或旧身份恢复，结束保留新候选及新Cloud授权；不再为收尾进ROM复位或读取射频后的NVS，字节保持只签到启动前。

新运行候选实际穿过原停点：Presence Receipt经原App验签并获Gateway confirm200，精确GET为deviceConfirmed／version1。随后第一条新prepare201，但提交前报错，原始分类及精确停点未捕获，不能宣称首尝试全绿；没有commit或新增active卡。原记录保留，经同账号显式GET确认expired／reserved→released并退休本地pending引用后，才由一次新的显式选择创建不同requestId，**不是重发原POST或UNKNOWN**。

后一次完整结果：

- `MTKgWvzkI0ypPePfJJcSRg`一次prepare201、独立nonce M2、一次commit200；目标`ble_bcEvRP7tf6wZaiZVlTW5Vg`与源Cloud不同，页面明确“蓝牙设备卡已注册”、`presenceSetupPending=false`。正常完成进入目标设备页；不把注册确认称作控制ACK。
- 前后正常owner设备GET集合为2→3→3→2；中间同账号显式读取该committed intent200，ID集合不变，无另一次prepare／commit。两产品各一条active逻辑卡，而不是同MAC合并或扣两次同一注册的额度。
- 原自动初始化GET及本次读回证明两卡同materialOwner=源Cloud、同root／revision及已确认version1／digest；source canWrite=true、alias=false。额外一次读alias触发429，未重试或改限额；已有200及本机grant保持，不能把该429擦掉。
- 完成页进入BLE详情时显示“蓝牙已连接”。一次正常后台／前台后回首页，Cloud主aria“云端已同步”且WiFi图标，另有“蓝牙在附近，非当前控制通道”；BLE主aria“蓝牙在附近”。旧`device_e622c490…`仍离线、无附近标记。区分连接与附近，不借此重签全TTL／账号矩阵或云端控制ACK。
- 原设置页一次“解除设备绑定”及一次确认，DELETE200精确目标ID、`registrationProduct=ble_direct`、`rootDisposition=retained_for_other_registration`、`localAccess=not_confirmed`，Broker／refresh待办均false。目标消失，源Cloud及旧卡保留，源附近标记仍在；不发清根或物理撤权，也不宣称其它手机已失权。
- 独立只读终核证明源v7引用和根材料保留，原新Cloud的instance／epoch／Admin id／secret／version／permissions及cloudContext精确不变；本批目标凭据已移除。原6项非会话安全记录逐字节保持，原两份intent journal原为0行且仍0行，不补造“已有UNKNOWN样本”。完整HTTP／原生／logcat私有留存，全部临时监听、Debugger及9479 forward已回收；无活动编译队列。

主证据为App忽略目录所指私有`ip4-final-proof.json`、两卡与删除后截图；Library为`build/ip4-owner-fix-20261103/`链接及有限ROM记录。只生成脱敏摘要，秘密、APK、BIN、私有备份和原始请求不进入Git。本批没有提交推送。

### 14.7 剩余边界与下一批最小范围

1. Cloud→BLE核心差异本批有限收口，不再重跑配网、首装或两卡样本。第一次prepared后的提交前错误保留为未分类；如自然再现，先捕获有界脱敏阶段／原始code，保持prepare、M2、commit和UNKNOWN边界，不因一次后续成功删除原失败，也不为补日志再采样。
2. BLE→Cloud仍受原TaskStore／scope可安全区分门禁约束，不能据本批放开；原Cloud源删除后的唯一writer消失仍按确认材料只读合同，不自动交接写权。没有验原任务迁移、Controller rotate/revoke或其它板／平台。
3. 下一批优先必要源码交付审阅与发布冻结差异：只核本批8文件、App终态／presence接线及对应回归的必要集合，对齐尚未通过的既有freeze／选择器合同；保留无关dirty，不自动提交推送。无需再次认证、刷板或重跑已通过项；新Cloud身份、授权及候选保留供后续使用。

## 15 必要源码审阅、选择器与发布冻结对齐（2026-11-03）

### 15.1 审阅范围与最小修正决定

仅审§14的必要接线及交付检查，不重新部署、构建固件或做真机测试。当前8项owner生产文件与e48c运行候选的审阅stage哈希全部匹配；App本批20项增量加5项安全store／API／matcher基线，当前源码及stage均25／25匹配46ca App。故不重写已验产品行为，不开放BLE→Cloud、任务准入或其它平台。

App增量为5项生产文件（checkpoint、coordinator、BleService、注册guide及HTML）、4份spec（ports、Service registration／presence及原模板）和11个locale；IP-3 credential store／root reference等作为基线保留，不复制第二份秘密、writer或执行链。SDK8文件、两份相邻host测试及原CMake注册仍为§14必要集合。其它dirty修改不自动纳入交付或提交。

三项架构停点分别有已存在的合法来源，不能统称为“启动意图”：

| 停点 | 相对原f878的审阅结论 | 本批动作 |
| --- | --- | --- |
| Facade登录冻结 | DA-4显式内部BLE四槽分支，须两项内部selector；没有公共类成员、begin或登录算法变化 | 只更新对应实现摘要 |
| ESP32 WiFiBLE产品登录冻结 | 一字节RAM启动意图、pre-attach显式request、一次consume及原开窗选择；不清NVS、不产生授权、不自动重开 | 只更新对应实现摘要 |
| WiFiBLE入口选择器 | 已有库profile必须先于Facade准入guard；旧检查器却要求全部Facade-first | 精确允许此入口的`OfficialConfig→Facade`，其它入口维持Facade-first |

本批只修改检查器、冻结记录、有限回归入口及文档。两项登录摘要相对**本批开始时v52**更新，不是说相对Git HEAD只存在两项修改；此前v50～52的dirty记录及审阅说明保留，不撤回或重新baseline。

### 15.2 精确合同与冻结追溯

`product_selector_valid`是供原架构循环及反例共用的纯检查函数：只认三个原产品宏；WiFiBLE仅允许`OfficialConfig.h`、`BlinkerFacade.h`组成的精确前缀，普通WiFi／BLE仍要求Facade第一。选择宏必须先于首consumer，两个其它产品宏都须保持冲突，平台绑定必须在前缀之后；没有允许任意提前依赖或同时开启两种产品。

登录freeze升至v53，仅Facade与ESP32 WiFiBLE产品两项实现摘要改变，剩余22项精确保持。独立审阅在内存反向还原v52后，完整LF哈希仍为原`8ee6a3b5…50df72b`，并非重新接受所有当前文件。原`task_service_confirmation.json`的§23 review、positiveRelease、formalAdmission=false及18项实现／回归摘要保持，只更换所嵌套manifest一项哈希，另追加dependencyReviews保存前后哈希及未变集合摘要。不把这次元数据更新当成重新完成§23数据库／COMMIT验收。

### 15.3 有限验证与公开复跑入口

- 提取原选择器逻辑后，真实WiFiBLE入口及删／移profile、晚定义／漏冲突的反例共7个subcase先红；修正后8个测试方法通过，不将subcase数量冒充业务场景数。
- 冻结追溯3项通过；随后仅新增的CI接线1项通过，未重跑原3项。CI在原architecture之后、syntax／编译之前追加该目录有限unittest；未修改SDK矩阵、安装版本或触发远端CI。
- 完整`tests/architecture/check_device_v2.py`通过，§14.5原三项静态停点已对齐。这不等于新的全目标SDK编译、安装或正式发行完成。
- 新源码未改设备／App运行逻辑，既有host及真机结果只作匹配基线，不重新累计或复采。原权限、UNKNOWN及完整请求日志没有本批写入。

App回归交付另发现原命令依赖忽略目录runner，不能直接当作Git复跑说明。先验证标准`ng test app`的精确include，保留主tsconfig引入无关旧spec类型错误／旧WiFiProv声明的构建停点；没有去修改那些文件。窄tsconfig使原presence和真实模板40项通过，但另两份原spec的相对`vi.mock`被Angular runner明确禁止，故不能冒称四文件都通过。最小公开入口复用原Vitest及Angular runner顺序执行，不改测试语义、全局配置、依赖或锁文件；最终执行结果另记，不使用私有build作为运行前置。

最终公开入口为App的`node tools/test-access-registration.mjs`：原Vitest核心2文件55／55、原Angular presence／模板2文件40／40，四文件95／95、exit0。4个新`tools/test-access-registration*`不含私有路径或账户材料；后来仅补依赖源码存在性检查，syntax／`--preflight`通过，不重跑95项。既有host／真机结果不累计为本批新测试，有限入口通过也不等于主App所有旧spec类型检查通过。

### 15.4 交付依赖与下一步

App原锁已依赖同级`../capacitor-wifiprov`源码。匹配46ca的插件definitions／web仍有既有未提交修改，Android／iOS原实现及配套类也需按已验来源一起冻结；index只有CRLF差异、不是Git语义修改。本批没有擅自提交旁仓、回写打包声明或升级SDK。公开入口消除了对私有build runner的依赖，**未消除匹配插件源码这个真实前置**；preflight只验存在，不能把其成功当版本匹配证明。详情与完整命令取[App交付说明§10](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md#10-ip-4-独立接入的必要源码与有限复跑2026-11-03)。

本批§14.7所指三项静态冻结／选择器差异已关闭，运行源码及已验授权不动。下一步只在明确交付／提交时整理匹配的App＋WiFiProv依赖与设备库必要源码，并以同一提交的CI确认声明目标；不将本批静态检查和旧单候选链接称为新的全目标发行。首次prepared后提交前错误仍待自然条件，BLE→Cloud及删源writer边界保持；不为此再次刷板、登录、发码或制造样本。

## 16 公开回归入口的源码导出闭合（2026-11-03）

### 16.1 最小缺口与修正

沿§15.4只核交付工具，不扩大产品范围。原App `tools/source-handoff.mjs`已导出App的`src`及五个相邻插件的`src/android/src`，却未把上批新增的四个`tools/test-access-registration*`列入文件白名单：完整源码导出后会丢失公开复跑入口，单靠本机95项通过不能消除此缺口。

只在原`APP_FILES`增补这四个文件，继续使用原manifest逐文件摘要、HEAD来源和`verify`，不新增打包器、摘要框架或版本号。四份spec、setup和父tsconfig／Vitest配置沿原树与白名单导出；缺少必需入口时导出失败且不生成成功manifest，沿用原工具的部分目录保留语义，不声称原子回滚目录。

WiFiProv的Android闭包已有原规则承接。有限只读复核的21项（TS三项、`android/src`十五项、package／Gradle／ProGuard三项）与已验46ca的原输入摘要匹配；未增加SDK补丁目录、改变原2.4.4依赖或编辑插件。现工具明示Android源码范围，不能据此签iOS交付、原生构建或整套当前dirty树匹配。

### 16.2 有限回归与清理

新增两项先红：导出确实缺公开入口，删除入口仍能错误生成manifest。白名单修复后执行：

```sh
node --test tools/source-handoff.test.mjs
```

工具自身14／14通过，包含新入口／原生路径逐字节导出及缺失拒绝，以及原锁、私有路径排除、摘要／来源篡改和安全边界反例。未执行安装器、App95项、Java／SDK编译、手机或服务测试；测试仅使用自动清理的临时fixture，没有生成当前工作树交接包或保留新备份。

### 16.3 当前收口与下一步

本批源码导出漏项已关闭，不继续重复本机配网、两卡注册或静态审计。当前运行候选、账号、授权、TaskStore、scope、UNKNOWN及完整日志不动，未提交推送。

后续明确进行Git交付时，须一并审选App、匹配WiFiProv和设备库必要基线，再用同一提交的现有CI确认声明目标。设备侧“8项增量”不是相对HEAD可独立提交的全部闭包，例如既有`TaskProductOwner.h`仍未跟踪；不能只挑8文件就宣称可重建。原工具记录实际输入摘要，但HEAD本身仍不等于包含所有必要未提交源码。当前不为此生成第二份源码快照、升级SDK或另开编译队列。

## 17 三仓必要源码、前置基线与提交后 CI 范围（2026-11-03）

按§16只整理提交前审选，不暂存／提交／推送、不安装或重跑已过测试。单一清单取[IP-4匹配源码提交计划](DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md)，不在各页维护第二套文件表。

- WiFiProv需完整39项功能／回归修改，另9项原Android基线保持；不是只提交definitions／web。Swift为Android专属能力显式拒绝合同，配套静态测试依赖它，不代签iOS功能。
- 设备库为8项owner增量＋14项组合／选择器／生命周期基线＋16项持久确认／续期基线，共38项生产源审选，另带必要回归／冻结及原发布工具闭包。不复制e48c私有stage诊断，不因追求匹配BIN而覆盖当前公开无诊断源。
- App列出新增注册／根引用／presence、原删除／retirement／首页消费者、原配网及Cloud／Local前置、原生Discovery／精确SoftAP网络策略和配套spec。API配置、11locale、大型Service／Settings／配网页均须按hunk审选；本轮没有拼成已验证的隔离提交。旧25项匹配不是完整提交集。
- 跨仓记录各自精确SHA组合；另外四旁仓clean且固定原SHA，锁文件不改。App／插件没有现成远端CI，库公开cloud-beta只签零槽两目标；四槽组合需显式接原受限安装入口，不能误用默认job代签。私有服务golden读取授权仍为发行HOLD，不绕过或代写CI secret。

下一批最小动作是按清单拆出混合hunk／前置层并准备匹配CI接线；明确Git授权后才提交相应组合，随后仅验证该新组合的声明门。已验配网、两卡、附近、删除不重采，不通过账号／设备变更来补源码证据。本批授权、TaskStore、scope、UNKNOWN和完整日志保持，暂不提交推送。

## 18 App混合源码隔离、匹配CI及有限回归（2026-11-03）

实施记录取[提交计划§7](DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md#7-混合源码隔离与匹配-ci-实施2026-11-03)。固定Git基线＋146个显式输入组成一次隔离App候选；只投影注册API及11语言的26＋1注册键，定位API和QA分享计数保留原工作树。Cloud／Local、Settings和配网页依赖为同一接入原子候选，不重构已验执行链或恢复旧跨产品同ID行为。

新增纯投影、组装／路径保护、CI计划、导出接线及库受限workflow合同共31个唯一用例通过。隔离新输入的产品TS以匹配WiFiProv源码解析无emit通过，旧打包声明失败保留；不把类型核查当完整制品构建。链接／TAB／越界输出、二进制保留及Git索引不变均有有限反例。

App手动CI锁三仓精确组合及四旁仓基线，Android显式选择；库四槽SoftAP复用原受限入口并保留`host-quality/package-boundary`依赖。远端未触发，私有golden授权HOLD仍会阻断库候选，不绕行、不写secret、不发布。原SDK／锁文件、生产源／locale、账号、授权、TaskStore、scope、UNKNOWN和完整日志保持，未重复配网、BLE或Start，未暂存／提交／推送。

下一步仅在明确提交授权后形成精确Git组合，再执行对应远端声明门；不使用旧HEAD触发缺新源码的队列，也不为CI补真机样本。

## 19 BLE→Cloud反向缺口：BC-1只读存储事实（2026-11-03）

独立小阶段取[BLE→Cloud有限实施计划](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md)。只读复核确认三个真实阻点：v1源根需已有Cloud版本、旧Install无本次intent／根CAS／nonce关联、原prepared到期释放不适用于可能已发行／下发Key。保留旧enable-cloud禁用，不把同MAC／公开instance或一次M2当写入授权。

本批只补第一道可复用事实：原TaskSnapshotStore／Codec读取的NotFound为Unbound，有效零任务表仍Bound，退休标记单列Retired；错误／未知写句柄为Unknown，不自动重读或包装新句柄解锁。新增API不提供canInstallKey，不改Key写、observer、wire、磁盘格式或准入；scratch、端口持久性及产品独占owner的职责明确。

原CMake／MSVC严格定向编译及新增`blinker_task_binding_observation_tests`通过，含9组场景；未运行其它98个host目标，不累计旧31项或重跑真机。无SDK升级、设备／账号／任务／scope变更，原授权和完整日志保持，未提交推送。旧DA页首和v1合同索引同步到§14.6的现行成功，历史事实不删除。

下一步仅BC-2：将同Store只读事实与实际保根加Key的产品owner／失败poison边界接合，并明确一次操作的设备侧授权关联。BC-1成功不等于反向注册已可用，App和Gateway仍保持原门禁；后续Key已发行／UNKNOWN不得沿原120秒释放，须BC-3配额／生命周期一并实施。

## 20 BC-2a内部首次Key事务与旧入口封堵（2026-11-03）

详细合同与结果取[反向注册计划§7](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#7-bc-2a内部保根首次key事务与旧旁路封堵)。原产品取得独占owner，停止management并失效原TaskAuthority，经同一TaskStore／Codec／handle观测Unbound，才在专用fence中核完整原Admin／epoch并保根加Key。Engine已初始化、Bound零表、Retired、Unknown、错owner／observer均拒绝；known零写拒绝不poison，实际未知写及存储失败持续锁闭，不靠重开handle或stop/start恢复。

旧Install不再允许Admin-only根绕过首次事务；普通replace同值、Bootstrap同根及历史installed同Key的零写早退也须过原fence，修复两个已复现的UNKNOWN伪成功别名。纯Key入口、空根Bootstrap和已有Key刷新保留，原报文／status／Endpoint容量不变。产品预准入根读取失败则原错误返回、零Key写／零任务NVS、不自动重读；仅后续新的显式内部调用可重核，不能混同已准入未知结果。

原MSVC Release／严格告警定向编译：三新增目标最终25组通过，必要相邻Endpoint／Presence及旧Store4函数窄回归通过；architecture checker通过，未改冻结摘要。不把合成NVS／transport或拒绝式签名适配器称作真实认证／硬件证据；没有原SDK链接、刷板、部署、发码、任务或scope变更，未暂存／提交／推送。必要源码独立增量取[提交计划§9](DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md#9-后续bc-2a独立增量2026-11-03)，不回写旧e48c运行候选摘要。

下一步仅BC-2b：冻结并接入实际已commit的Method2会话owner及本次intent授权，核同transport／session、Platform Admin／All权限、instance／epoch／根版本和双方nonce；取消／断连失效。当前expectedAdmin只是CAS输入，不是网络许可。BC-2a不等于整个BC-2完成，App／Gateway／WiFiProv反向入口仍关闭；随后还须BC-3独立Cloud目标、Key发行及UNKNOWN额度生命周期，不开放旧enable-cloud捷径。

## 21 BC-2b真实Method2与单次intent绑定（2026-11-04）

合同、源码及结果取[反向注册计划§8](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#8-bc-2b已提交method2与单次注册意图的最小合同)。新增可选内部授权桥复用原Authorizer、实际Direct commit和当前物理连接binding，精确HMAC绑定instance／原Admin／双方nonce／会话／intent／Key；PlatformAccess All、120秒及同peer边界均明确，公开结构和准备态不构成许可。

一次操作先复制当次栈输入、核当前原根并消耗执行权，再沿原owner／TaskStore调用。实际Store完整CAS后、同Key成功或写前再调guard核活会话；取消／断连／reset不解除busy或恢复旧授权。guard是写准入点，其后原installer成功／失败／UNKNOWN保持，不能由返回时NotConnected覆盖；未知写仍受原poison保护，不重发。

新增两host目标10＋6组、必要相邻DirectSecure／首次Key Store目标共CTest4／4通过，原严格MSVC完整编译／链接及architecture checker通过。独立合成HMAC向量不代替服务私有golden；确定性AEAD／受控queued回调／installer spy与真实Store产品测试分开记录，不称Runtime新wire或真实无线已接。没有SDK链接、App／服务／设备／账号操作，原授权、TaskStore、scope、UNKNOWN和完整日志保持，未暂存／提交／推送。必要源码清单取[提交计划§10](DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md#10-后续bc-2b独立增量2026-11-04)。

下一步仅BC-3：先固定BLE无Key源的方向化根合同、独立Cloud目标／Key／intent及issued／UNKNOWN额度生命周期，再接原注册、vault、删除与过期消费者。BC-4的显式用途和PreCloudDirect→停止Direct→原WiFiProv交接仍后置；当前不把内部桥当已开放BLE→Cloud，不重跑旧扫描、配网或两卡验收。

## 22 BC-3a方向化根、密文发行账本及保留围栏（2026-11-04）

合同取[BLE→Cloud注册v2](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md)，实施及完整结果取[反向计划§9](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#9-bc-3a独立cloud发行账本及保留围栏2026-11-04)。本批明确只接内部准备、密文发行及metadata协调，不提前建active目标或让Cloud接管原BLE Presence。v1／060正Cloud版本与摘要保持，源无Key使用独立判别形状，不伪造0／null／版本1，也不按MAC／单独instance合并注册。

复用原非零Key生成、locator、vault和user→tenant事务，冻结独立目标及intent／root／nonce／Key绑定；061 staging行guard保留已发行材料。统一quota和原root／Presence／BLE恢复／源删／账号删消费者接新retention薄口，root=null也不能绕过；只有未发行prepared可过期释放，issued／UNKNOWN继续持有额度、密文和材料引用。发行前跨TTL尾窗已先红后绿修复，不自动重发。

六新增文件40项＋quota2个新反例及必要相邻回归通过，语法与有限runner接线核查通过；纯carrier／migrator假client不是PostgreSQL证据，061未执行、真实PG待验。未运行SDK／App／设备／服务部署或认证，原账号、授权、TaskStore、scope、UNKNOWN及完整日志保持，未暂存／提交／推送；原私有golden HOLD不变。

下一步仅BC-3b：先补独立数据库的061迁移／约束／held额度回归，再接原近期认证下同intent Key读取、真实安装确认及独立Cloud目标原子激活，同时固定原BLE materialOwner和两卡删除。BC-4无线用途及交接继续后置；本批不称完整BC-3或反向注册已可用。

## 23 BC-3b数据库前置与事务测试准备（2026-11-04）

详见[反向计划§10](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#10-bc-3b前置真实事务用例补齐隔离数据库待确认2026-11-04)。061可选实库runner补第5项真实SERIALIZABLE回滚／COMMIT后调用方丢响应，只核ledger与原事务，不重复配网或旧离线用例。语法及差异检查通过，五项PG均未执行。

本节当时无独立测试URL／native PostgreSQL，Docker及WSL均未运行，未自动启动；当时停点已由用户授权后的§24推进，不再将环境待确认列为当前阻点。原账号、授权、TaskStore、UNKNOWN和日志不动，未部署业务服务或提交推送。

## 24 BC-3b实库门完成及内部Key读取收口（2026-11-04）

完整结果见[反向计划§11](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#11-bc-3b061实库与内部key读取2026-11-04)。用户允许保留129的专用测试数据库，已建立无公网端口、独立卷／内部网络和受限角色的PostgreSQL16.15；本机原Node／锁依赖经SSH执行061的5个子项，全部通过且0skip。每批schema已清理，库保留复用，原22个容器均未重启，不使用业务卷或部署Gateway／App。

同intent、真实近期认证下的内部秘密借用已接原repo／vault及原认证模块，末端晚检、零重试、清零和UNKNOWN保留明确；新增Node31个唯一叶、必要相邻25个叶通过，另Python7项通过。Redis／SQL合成端口不代签真实账号或无线。HTTP未开放、不生成替代Key、不提前插active Cloud卡；下一批仅将安装确认／最终提交、原BLE materialOwner与两卡删除一起闭合，再接BC-4无线消费者，不再重复本批实库或原配网样本。

不能将本批数据库／内部读取通过称作BLE→Cloud已可用。原授权、TaskStore、scope、UNKNOWN、完整日志及私有golden HOLD保持，未部署Gateway／App、刷板、发码、升级SDK或暂存／提交／推送。

## 25 BC-3b内部安装确认、原子激活及两卡删除（2026-11-04）

精确合同取[v2合同§7](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md#7-bc-3b安装确认幂等及独立激活合同)，源码／实际结果取[反向计划§12](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#12-bc-3b确认与激活收口2026-11-04)。只接受App核验原认证通道成功ACK的明确模型，同intent／根／发行nonce／Key与会话确认摘要绑定；不冒称服务端验硬件签名。

原SERIALIZABLE事务一次生成独立Cloud目标、正式AAD重封的原Key、非秘密Admin、060根和两成员，再持久不可变commit；原BLE、Presence及TaskStore／scope不搬。RESULT_UNKNOWN可协调迟到确认，删源／删号／换根及其它原因仍拒；精确提交重放先于当前源／TTL，目标墓碑不复活。062增量保060／061历史及v1解析，原Admin版本保持真实正uint32。

固定BLE材料归属及唯一活writer，两种删除顺序都保留另一卡；末删撤ACL、退休根，保必要密文／审计。审阅中补齐删除前当前root／Admin CAS，以及已激活共享根下原BLE恢复的issue／commit围栏；不把committed继续算held，也不永久禁止原BLE Presence轮换。

新增纯Node36个唯一叶及必要相邻35叶通过。现有专用PostgreSQL仅执行新062文件一次，7子项全过（含父项8pass／0fail／0skip，73.6秒），实际生产commit／正式Key读取／原Presence和removal消费者、事务回滚及丢响应均有实库证据；合成账号／关联／receipt不代签真实认证或无线。after-hook清理本批schema，库保留，不改业务服务或真实设备。

下一批仅BC-4：明确实际认证命令／成功ACK及App验证消费者，再接PreCloudDirect有限阶段与原WiFiProv owner交接；不得重放旧UNKNOWN、回到enable-cloud或Bootstrap补链，不重开已过数据库／资源／配网审计。HTTP及部署仍关闭；原授权、TaskStore、scope、UNKNOWN、完整日志和私有golden HOLD保持，未升级SDK、暂存、提交或推送。

## 26 BC-4a 专用回执消费者与原生退出（2026-11-04）

详见[反向计划§13](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#13-bc-4a专用安装回执与原生停止证据2026-11-04)。独立安装命令复用已提交 Method 2／原 owner CAS，回执只能向同绑定加密发送；App 专用 client 验原 Key/源/intent 与原始 ACK，写后不明不重发。NimBLE 的逻辑 stop 不再可当释放凭据，提供原生停止／任务退出证据供后续 owner 调用，未接默认产品。

设备 host／App 定向、同字节 Gateway 摘要向量以及原 ESP32 3.3.11 XIAO ESP32S3 普通消费者链接通过，不代签实际服务请求或无线交接。下一批仅显式 PreCloudDirect product owner、服务同intent确认及原 WiFiProv WiFi-only 交接；不重做061／062，不绕原 freeze 或私有 golden，不挂 HTTP／页面、不部署、不提交推送。

## 27 BC-4b 同 intent 一次交接；实际 WiFi 接纳暂不开放（2026-11-04）

详见[反向计划§14](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#14-bc-4b显式-direct-交接与-wifi-接纳停点2026-11-04)。候选 Runtime 限原 Hello／Method 2、专用安装和一次 `0x3A`；App 严格核同 intent 服务确认后从原 AEAD 会话提交并退休 GATT。设备只输出一次本地交接事实，原生释放、重新核根及 WiFi 写前准入仍各自验证。重连、过期、取消或 ACK 丢失均不能重开旧写。

原 SDK 的凭据事件发生在 STA 配置生效后，不能用于写前围栏；现有通用配网入口保持冻结。下一批只做候选专用写前接线及匹配原 SDK 完整链接，达标后再排 BC-5，不重跑旧配网或实库。无新公开入口、部署、刷板或提交推送；授权、TaskStore、UNKNOWN及完整日志保留。

## 28 BC-4b 候选写前门禁（2026-11-04）

见[反向计划 §15](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#15-bc-4b候选专用写前门禁2026-11-04)。复用 ESP32 原 SDK 的 Security1 和 WiFiConfig codec，独占候选不启动 stock manager；Set 只暂存、同 session 的 Apply 只排队。监督器在设备原生 BLE 释放后、以及一次真实 journal 写之前复核持久根和原窗口。原默认产品、App 入口及 HTTP 不变。

后续仅接匹配 App 和设备的独立 AP／PoP 与启停顺序，现有 UNKNOWN／Cloud 只读协调不变；没有该真实 owner 不能开始 BC-5。普通设备尺寸上限及冻结断言保持，不升级 SDK、不烧录、不部署、不提交推送。

## 29 BC-4b 同意图 SoftAP 与 Android 窄适配（2026-11-04）

见[反向计划 §16](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#16-bc-4b独立-softap-凭据与候选接收顺序2026-11-04)。设备和 App 已对齐临时派生，设备候选以独占 AP／HTTP／journal／原 STA 顺序接现有门禁，Android 窄适配只认一次 Set／Apply 的带类型回执。未挂默认入口、未烧录、未部署；仍需显式 worker／设备启动调用和最短真机验收，不能用离线回归代替。

## 30 BC-4b 显式 App/worker/ESP32 owner（2026-11-04）

详见[反向计划 §17](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#17-bc-4b显式-worker-与设备-owner-接线2026-11-04)。显式 Android 候选复用同独占 worker 与真实退出证据，固定请求只走其持有的 AP；ESP32 候选复用 `stack.open()`、原 TaskStore owner 和原 STA/Cloud，Key/WiFi 写均受原 setup 窗口。App 把 Apply 结果和匹配退出后的新鲜 Cloud 观察分开。

BC-5 仍需匹配 APK／真实可刷 profile、现有 BLE 根的真实 Unbound 测试板，以及授权的同 intent 服务内部调用入口；不通过合成确认、旧 `enable-cloud`、清根或开默认 HTTP 补证。原授权、TaskStore、UNKNOWN及完整日志保留；本批不部署、烧录、升级或提交推送。

## 31 BC-5 冷预检及离线准备（2026-11-04）

见[反向计划 §18](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#18-bc-5-冷预检构建准入与内部同-intent-入口2026-11-04)。完成固定批准范围的服务进程内入口和无写冷预检、显式 Beta 构建门、独立设备候选配方，以及 Android 保同一 Cookie 修正；未挂 HTTP 或运行真机。当前只有 COM11，无合格 BLE-only／Unbound 源证据；当前 APK 未启用候选。真实内测入口、源板和匹配可刷/可装构建齐备后才做一次短验，不清根、不重放、不部署或推送。
