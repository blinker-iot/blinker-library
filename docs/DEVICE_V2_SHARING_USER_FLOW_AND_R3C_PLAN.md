# 单设备分享：用户流程、权限与R3c实施合同

2026-09-11。覆盖设备/App/Service/Broker；延续基础收口路线与Service跨分片§30，不新增设备登录协议或第二条App MQTT连接。

最新入口：§8.14。Q3e定位本机UID冻结；真实Activity重建通知点击、跨Broker接受viewer/进入设备页、撤销及两个Broker精确回执通过。测试grant已撤销、原账号/跨片关闭值恢复；后台持续收信与进程冷启动仍未通过，保留独立兼容性门禁。下一批R4物理LAN撤权，不让通知提醒代替或阻塞授权链，不改设备协议。

## 1. 用户看见的完整流程

**设备页面 → 分享设备 → 选权限 → 发链接/二维码 → 对方登录并确认接受 → 共享设备页面 → owner可改权限/解除，对方可退出。**

| 环节 | 用户操作与反馈 | 后台真实含义 |
|---|---|---|
| 创建邀请 | owner在设备页选择仅查看/可控制；可选限制可控制组件 | 创建带权限快照的短期单次Invitation，不发DeviceKey、不转移所有权 |
| 发出 | 复制分享链接、二维码或系统分享；显示有效期和“待接受” | 链接和二维码携带相同随机邀请proof，不是两种协议；首期24小时、单账号单次领取 |
| 打开/扫描 | 展示设备名、分享者安全展示名、权限、有效期，用户确认“接受分享” | 链接GET/预览绝不消费邀请；登录后再POST接受；聊天链接预取不能抢占 |
| 未登录 | 引导登录，返回同一邀请确认页 | 临时保存邀请而非自动领取；账号变更必须重新确认；不记录到分析/崩溃日志 |
| 接受 | 先显示“设备已添加”；如Broker尚未同步，显示“权限同步中” | 权威grant事务成功；重复请求返回当前grant，不重新增权，不重新配网/绑定 |
| 使用 | 增加“共享设备”标识，进入同一组件控制页面 | 一条账户MQTT路由到device；Manifest描述能力，share ACL裁剪允许的操作，页面不是鉴权依据 |
| 修改权限 | owner在成员列表修改角色/可控制组件并确认 | 新权限版本，旧观察/在途结果失效；本地授权需要退休/重新签发 |
| 解除/退出 | owner“解除此人的分享”；成员“退出设备共享” | 仅该account＋logicalDeviceId撤销；不重置设备，不刷新DeviceKey，不影响owner/其他设备 |
| 撤销未接受邀请 | owner在“待接受”中取消 | Invitation失效；不是删除已有成员，不连带撤销其他邀请 |

一个开放链接被转发时，首个成功确认的合法账号可以领取。因此创建页要提示“仅通过可信渠道发送”；owner能看到已接受成员并立即撤销。另支持规划中的指定账号邀请：只能由指定账号领取，即使链接/通知被转发也不改变目标。不增加默认人工二次审批。邀请过期不导致已经接受的share过期。

## 2. 权限最小模型

| 角色 | 查看Manifest/状态/事件/已授权实时流 | 业务组件控制 | 再分享/Key/配网/解绑/成员管理 |
|---|---|---|---|
| owner | 是 | 是 | 是 |
| viewer | 是 | 否 | 否 |
| operator | 是 | 全部或commandEndpointKeys白名单 | 否 |

“查询权限”第一期是整个设备的可读业务能力，不声称支持逐字段隐藏敏感数据。组件白名单使用稳定的Manifest key而非页面组件ID/数字fieldId；陌生或已删除端点由设备/Broker拒绝，不按同名UI自动扩大权限。全部控制（null）包含设备未来新增的业务端点；明确白名单则不自动加入新端点，界面需提示差别。

viewer→operator、白名单扩大属于增权：在事务中要求owner/recipient active、device active及可用normal route。operator→viewer、白名单缩小和解除/退出是减权：**路由迁移或Broker故障不能阻止用户撤权**；先提交逻辑权限变化，原队列保留未完成传播。相同权限重复PATCH不得递增version或再次退休凭据。邀请权限被owner修改后旧邀请不能重新覆盖当前已接受权限；接受重放只能读当前结果，不再次upsert。

默认新邀请选择viewer，可明确改为operator。服务器现有待接受邀请上限16；App原“9个成员或邀请”是无服务端依据的限制，应删除。活动成员配额以后由服务器统一返回，不把RAM凭据槽个数误作云端分享人数限制。

## 3. 通道与硬件边界

- 分享身份是account＋logicalDeviceId；不是MAC，不复制DeviceKey、MQTT设备密码、owner Direct Admin凭据。
- MQTT：App账户单连接，Broker对目标和endpoint授权；既有BBP/CBOR原样路由，不为分享新增JSON设备通道。
- LAN LocalGrant：当前支持read/control两位能力，没有endpoint白名单。白名单operator不能拿到无范围限制的本地control；可只读LAN、命令走原云端。路由选择必须先看权限是否能在该通道兑现，不能盲目BLE/LAN-first。
- 纯BLE：现有Direct Admin元数据入口是owner专用。云端分享记录不证明成员凭据已安装，更不证明viewer/逐endpoint能在设备端执行。共享长期同一controller secret无法严格区分每个成员的权限/单独撤销。成员BLE直连的有限授权接线属于待补能力；未完成时不得交出owner密钥来让按钮“能用”。
- WiFi+BLE有可用云端时，共享用户可先走可兑现权限的云端链路；不能宣传尚未实测的成员BLE直连支持。
- 分享网关只分享网关本身；分享一个子设备只授权该子设备，借网关转发不等于访问全部子设备。家庭/房间仅是组织关系，单设备分享不自动加入家庭。将来家庭授权与单设备授权叠加时，移除一个来源不能谎称已撤销所有来源，本批不新增家庭ACL。
- 设备离线也可以建立云端分享关系；有normal路由不代表设备在线。离线旧本地授权只能靠原退休交付/ACK、现有到期规则或物理reset处理。界面应分别表示“云端已撤销”和“本地同步待完成”，不得凭HTTP200宣布离线设备已清除。

## 4. 链接/二维码与App生命周期合同

二维码只是分享链接的编码。正式HTTPS入口使用经验证、配置在App白名单中的分享域名，建议邀请proof放fragment，落地页不引入第三方资源/统计且使用no-referrer；不能把proof放在设备ID或MQTT topic中。测试/已安装App可使用独立`diandeng://share/<code>`入口；它不等于HTTPS通用链接已部署，也不能保证聊天App允许直接唤起。

Universal/App Links的域名关联、Android assetlinks、iOS关联域、未安装跳转和内置扫码需要单独验收。不能靠生成一个外网未部署的URL冒称完成。冷/热启动、扫码/粘贴、登录返回应汇入同一个邀请控制器；仅格式解析和只读预览可自动执行，消费必须用户显式确认。

邀请码为已有43位base64url proof；用户无需逐字输入，保留粘贴原码作为兼容入口。App不持久化已接受/撤销的邀请码。并发打开去重、重复点击复用请求，超时保持邀请及操作幂等标识以便重试。服务错误要区分未登录、过期、已被别人领取、已撤销、路由准备中和网络失败，不能全部提示“邀请码无效”。

## 5. Service/Broker一致性修订（R3c）

1. 默认关闭的独立cross-shard分享开关，必须同时满足跨Broker请求链已启用。不开启部署开关，也不搬迁账号来绕过限制。
2. received清单是已授予关系，不因临时missing/migrating route消失；保留active账户/owner/device过滤，route健康和在线信息另表达。清单查到关系不授权新的控制。
3. create/accept/增权要求normal route、active主体和设备；prepare只做准入检查，必要的账户路由分配在事务外，最终accept事务重新检查。
4. accept replay复查owner/device/account与匹配grant；返回当前状态而不是原邀请权限。不因已过期的已接受邀请重新发grant；重复消费不唤醒重复失效任务。
5. update共用权限偏序判断：相同为幂等，无网络依赖；增权受route/跨shard开关约束；减权不依赖broker可达。revoke/leave不依赖normal route/同shard；不允许更改他人的share。
6. 所有变化复用原031/032持久双端失效与027 LocalGrant退休。`realtimeRefreshPending`只表示Broker投递，不代表本地Retire完成；界面不得将二者混为“全设备已同步”。
7. 接受同一device/account的新邀请不能无提示降级或覆盖现有权限；当前已有share时应返回当前已生效关系，改权限由owner成员管理入口完成；已revoked关系可由新邀请重新授予。旧接受记录重放不恢复已撤销关系。

## 6. 本轮执行与后续拆分

| 批次 | 范围 | 验收 |
|---|---|---|
| R3c本轮（软件基线已完成） | Service公开入口一致性/默认关开关，PG与实际两Broker夹具改走原分享handler，App保留mutation身份/pending/重放状态，去无依据9人限制 | 增/减/同权限、跨shard、迁移/停用、重复接受/撤销/退出和原回归通过；不宣称二维码已上线 |
| 邀请体验下一批 | 只读预览、链接/二维码/粘贴、统一接受控制器、登录返回、成功后进设备页、可控组件选择器 | 原生冷/热启动/扫码/未登录/二次点击/错误分类；不能只测URI生成 |
| R4 | 受控部署后双账号真实App/LAN/设备退休及恢复 | 一人失权另一人继续、离线pending诚实展示、控制/只读行为真实执行 |
| B2及后续 | owner绑定/解除、共享BLE有限授权和长期密钥轮换等 | 不以云端ACL成功替代设备端成员权限安装/撤销 |

设备业务固件本轮不新增成员列表；权限状态归服务器、限时本地能力归现有接口，传输与UI不持有第二份权威ACL。方案不会因做分享而把历史BLE共享secret设计未经审查地复制到新通道。

下一批邀请体验细分为：

1. 增加认证后的只读预览接口（候选`POST /api/v2/share-invitations:preview`，proof放body），返回最小设备展示信息、权限/有效期和可接受状态，不分配route、不写grant、不消费；同已领取者可查看当前关系，第三方不能列出成员资料。
2. App统一share-invitation控制器接收已校验的code；二维码、深链、粘贴只负责输入，先预览，登录后再次确认。owner成员列表还需可辨识的安全显示名，不能只展示散列memberRef让用户猜要删除谁；不向成员泄露完整账号或其他受邀人。
3. 冻结真实分享域名配置及关联文件，二维码本地生成；无已部署域名时明确为开发scheme入口，不声称外部聊天链接已通。只读viewer/可控operator/稳定endpoint key选择从原Manifest取值，不实现另一套页面编辑器或ACL。
4. 冷/热启动、未登录→登录返回、接受成功进同一设备控制页、重复打开、邀请过期/取消/被别人领取、弱网重试和解除/退出真机验收。权限同步中保持诚实反馈；不能以强制重连账户MQTT实现同步。

本批软件证据：Service公开分享/准入与开关20项、原PG/两个Broker组合9项、相关回归77项；Gateway正式清单通过（最后组99项）。App已保留同步反馈、邀请失败同一操作Key重试、接受重入保护及已撤销重放处理；详细测试结果见App LAN文档。尚无本批真机/部署/原生二维码证据，不能把“软件基线完成”写成完整分享产品上线。

## 7. 官方方案参考及取舍

- [米家官方分享入口](https://www.mi.com/global/support/article/KA-07429/)包含设备页分享、长按卡片分享和家庭成员入口；[米家解除分享](https://www.mi.com/global/support/faq/details/KA-491605/)按目标账号移除。借鉴的是设备设置入口与成员管理，不据此推断其BLE内部密钥协议。
- [涂鸦Smart App SDK设备分享](https://developer.tuya.com/cn/docs/app-development/deviceShare?id=Kdwaamirw2x4o)区分分享者/接受者、分享码信息、接受与移除关系，并要求分享码入口配置通用链接/scheme。借鉴邀请预览→接受→关系管理和深链配置，不引入其SDK。
- [涂鸦官方用户说明](https://support.tuya.com/zh/help/_detail/Kfct1sdt6dwjk)区分单设备和家庭分享，并指出该产品的单设备分享没有权限管理。因此我们的viewer/operator/endpoint白名单是针对Blinker需求的设计，不能说照搬涂鸦已有逐组件授权。

参考查询：2026-09-11；具体App版本/设备类型能力可能不同。本文的流程和权限取舍为本项目决定，官方资料仅支持上列公开产品行为。

## 8. 粘贴链接、唯一账号邀请与两栏管理（2026-09-11后续修订）

### 8.1 页面职责

管理页保留两个页签（手机不做窄屏左右硬分栏）：**我分享的 / 分享给我的**。

- 我分享的：按设备分组展示已接受成员、待接受邀请，可改权限、取消邀请、解除成员分享；未分享设备可从此发起。
- 分享给我的：待处理的定向邀请在前，已接收设备在后；未读/待处理角标不混入已接收设备数量。已接受设备仍在原设备首页，管理页不是唯一控制入口。
- 独立邀请确认页只负责一条邀请的设备信息/权限/有效期、接受和取消，不承载所有设备列表。粘贴、未来扫码/通知共用该页，不各写一套accept逻辑。
- “取消/返回”仅离开页面；定向邀请的“拒绝此邀请”才是服务端终态。开放链接接收者不能因取消而作废所有人手中的链接；owner仍能取消邀请。

### 8.2 链接入口（已完成软件接线）

设备页生成`diandeng://share/<code>`；接收者在App“分享给我的→粘贴分享链接”粘贴完整链接或原码，进入预览/确认页。它是App内可解析的便携链接，不依赖外部网页已上线；不冒称微信中点击唤起、HTTPS落地、系统扫码或二维码已验收。严格匹配scheme/host/路径/长度及canonical code，不请求用户粘贴的任意URL，不读取后台剪贴板。

预览必须已登录、只读且不分配route/不增grant。邀请码仅保留当前流程内存，不写地址栏/router参数、磁盘或日志；登录后回同一流程仍需显式确认。账号改变/退出及晚到响应不能把旧账号预览或接受结果显示给新账号。成功领取后刷新原设备目录再进入`/device/:logicalDeviceId`；刷新失败不谎报邀请失效、不重新创建授权，保留重试打开入口。

### 8.3 指定唯一账号邀请（合同；底层接线进展见§8.5）

1. owner选择“邮箱邀请”，填写对方注册邮箱、确认权限（默认viewer）后创建。邮箱只作可变查找别名，昵称不定位，手机号不是必须字段。IdentityDirectory向账号权威服务精确解析为稳定UUID；沿用注册时trim/lower规则，不添加去点、去加号或邮箱商特判。不查询Gateway最近登录缓存，不新建接收者账号。邀请创建后固定UUID，后续邮箱变更或被另一账号使用都不改变既有目标。安全展示名/查找结果二次确认可后续完善，不以新增不可改公开账号字段为前置。
2. 创建原Invitation时固定`recipientAccountId`，将目标纳入幂等digest；接受始终使用登录身份匹配recipient，不能通过可转发的开放proof绕过。target为空才是开放链接。目标账号不能自填另一个recipient来领取。账号注销后同名账号即使被重建也不能领取原accountId的旧邀请。
3. 同事务保存Invitation及通知待发意图，复用系统现有notify能力（App已有`NtfyService`/`capacitor-ntfy`），不另造设备MQTT推送协议。消息投递按invitationId幂等；账号查询/推送网络调用不在SQL事务内。通知仅“收到设备分享邀请”等最小信息及messageId，账号认证后读取消息详情中的invitationId，再读取当前邀请。通知不是授权或事实来源，payload不带DeviceKey/邀请proof/完整账号或手机号。现有ntfy按安装和账号管理凭据，沿用退出清理与旧账号消息隔离。
4. 收件箱按认证accountId分页读取自己pending且未过期邀请；接受复用原route准入/最终事务复核，拒绝幂等终止该定向邀请、不撤已有share。通知丢失/重复/延迟、App杀进程都不丢收件箱；过期或已取消通知打开后以最新服务端状态为准。
5. 防滥用按owner/recipient/IP限速、有界待处理数和重复邀请抑制，不提供邮箱模糊搜索或批量枚举；内部查询只返回accountId/active或null，不返回账号资料，App不能直接访问。暂停/删除主体不能继续发新邀请。用户输入邮箱与内部accountId分开命名，避免把显示别名混进ACL。

审计修正：Auth已有`/api/v1/users/expose/info-by-email`，但其旧查询参数鉴权/资料返回不直接用于分享；§8.6复用底层get_by_email新增最小内部接口。§8.7补齐Gateway站内消息处理器/有界生成worker及App消息动作，但installation处理器与系统推送仍缺。邮箱入口仍明确“系统通知尚未接通，请自行提醒对方”；失败不伪造本地成功或自动降级为开放链接。

执行顺序：本批开放链接粘贴/只读预览/统一确认及管理页语义→定向邀请权威账号解析/收件箱/通知→原生扫码/HTTPS关联/两账号体验→R4真机撤权→B1/B2/B3/B4。硬件协议和设备成员RAM槽本批不变。

### 8.4 本批执行结果与下一项

已实现：Service认证只读preview复用原proof/主体检查；App严格便携链接解析/生成、粘贴入口、一个内存型邀请控制器、独立预览/确认页、登录返回接线、领取后刷新目录进入原设备页。跨账号/晚到响应被隔离，失败可以重核对；设备列表刷新失败不会再次accept。管理页改为“我分享的/分享给我的”，已接收列表仍保留；移除旧直接领取逻辑及无消费者的行按钮样式。定向邀请待处理区在真实收件箱实现后接入，没有假待处理数据。

软件验证：Service隔离PG预览6项＋原分享/准入20项通过，Gateway正式Device V2清单通过（最后组99项）。App专项与ntfy/消息深链/interceptor关联72项＋输入/登录路由2项通过；两端类型检查通过。测试含父项和重叠，不累计为独立场景数。没有原生通知/扫码、APK安装或实体设备证据，开关/部署/绑定/网络保持不变。证据`build/lan-sharing-invitation-20260911/`。

下一项优先补**唯一账号IdentityDirectory合同与当前系统notify消息投递/收件箱接线**，再让指定账号邀请进入同一确认页。已有ntfy客户端不是服务器通知端已完成；账号查询、邀请事务、通知提示各有边界，不把它们塞进设备协议。owner/成员可辨识安全展示名、组件选择器、二维码/原生深链仍为后续体验门禁。

### 8.5 定向邀请、真实收件箱与决策复用（2026-09-11同日下一批）

**本节为此前批次记录，账号前提已由§8.6纠正**：Auth已有可查找的注册邮箱与稳定UUID，无需再增加独立“不可修改公开账号”。邮箱可修改；业务invite_code仍不可挪用。用户已授权修改账号服务。

已实施的软件链：

1. Gateway migration **033**在原邀请表增加`recipient_account_id`、`notification_pending`、`declined_at`及recipient分页索引。recipient为空仍为开放链接；定向目标包含于幂等digest，同Key改目标409。原开放链接digest与proof生成不变。目标不作为新设备属性/Key/租户ACL，不增设备RAM。
2. 定向创建只接`recipientAccount`，不接受客户端`recipientAccountId`。handler先校验owner/设备/role/idempotency，再在事务外调用注入目录。最新合同为可信`{accountId,active:true}`或null（撤销原immutable要求）；具体邮箱适配器、实际index及App接线见§8.6。没有目录/合同错误503，不查Gateway最近登录缓存、不自动降为开放链接。
3. repository创建与`notification_pending=true`同事务提交，定向响应不返回bearer proof或完整目标账号；同设备/目标重复pending409，沿用每设备16条pending，新增每接收者64条未过期pending上限。这是服务端收件箱防滥用预算，不是云端成员数或MCU槽位。上限不代替后续owner/IP/日限速，外部定向入口上线前仍要补齐。
4. 认证`GET /api/v2/share-invitations/received?limit=20&before=<invitationId>`：默认20、最多50、keyset分页；before只能引用本人定向邀请，允许已处理项仍作为分页锚点；排除开放链接、过期/已处理、失活owner/设备。响应`{items,nextCursor}`，只有最小设备信息及原权限/有效期；不含proof、目标账号、通知凭据。查询不建route、不增grant。pending不混成已接收设备或已授权关系。
5. 原`POST :preview/:accept`接受**二选一**`{invitationCode}`或`{invitationId}`；ID仅允许登录的指定接收者使用，开放邀请凭ID不能领取；即使拿到有效code也不能绕过recipient。仍复用原检查、最终事务、route准入、当前grant重放及031/027传播，不复制第二条授权链。
6. `POST :decline`只接`{invitationId}`，仅目标账号可操作；pending→declined，重复返回现态，不修改已有grant。accepted返回409，退出设备应走原leave。临时路由或owner暂停不阻止接收者拒绝；accepted/declined/revoked/expired终止通知待发标记。实体设备退休与此无关。
7. App“分享给我的”已增加真实pending列表、分页/独立错误重试，与已接收设备分开。点条目传ID到同一个内存邀请控制器；接受/拒绝互斥，登录返回、换账号清空及晚到响应隔离沿用。确认页显式拒绝不等于返回；成功接受仍刷新原目录进设备页。替换原code-only状态，不另写通知专属accept；没有新增HTTP客户端、二维码依赖或设备协议。

验证：隔离PG定向门禁 **9/9**（含父项），原分享/准入 **20/20**、Gateway正式Device V2清单通过（最后组99）；实际index HTTP/auth接线夹具 **1/1** 验证真实收件箱/preview/decline以及未装目录503。App专项 **35/35**，包括API9、flow9、列表5、确认4、owner4、链接2、路由2；两端类型检查通过。并发create及accept/decline、转发有效code、ID猜测、账号/游标串用、重复拒绝、route异常、quota与旧开放digest均覆盖。夹具账号/权威目录为隔离测试替身，不能把它写成真实账号服务联调。没有安装APK、修改常驻数据库/部署开关、刷板或操作网络；没有实际推送、暂存/提交/推送代码。

**仍未完成及接入顺序**：

以下Q1～Q3为本节当时的拆分；最新完成范围见§8.7（Q2a已完成，Q2b仍待实施）。关联回归补充：原ntfy15、message深链3、server interceptor28，共 **46/46** 通过；不是新增通知送达证据。

- Q1 已修正为注册邮箱精确解析→稳定UUID，见§8.6；不要求邮箱不可改/永不复用，不让用户填写UUID。安全展示名及真实部署仍需验收。
- Q2 当前系统消息/installation服务实际接通；有界worker消费本表pending意图，原消息中心按invitationId幂等创建messageId，再由原ntfy投递。网络请求不占SQL事务；失败保留pending，成功投递检查点不等于用户收到/接受。worker消费前重新核对pending/未过期/主体有效；无有效安装仍以收件箱为准。**本批仅保存通知意图，没有发送器/重试worker，不能称notify已发送。**
- Q3 启用账号选择/发送入口；认证消息详情新增可验证的邀请ID动作并接`stageDirected`，不能解析通知正文直接授权；测试通知丢失/重复/过期、换号、安装撤销、App杀进程。当前MessageItem尚无已验证的邀请动作字段，不能仅凭ntfy客户端已存在声称消息点击闭环。
- 随后原生链接/扫码/权限选择与R4真实双账号LAN撤权，再B1～B4。账号服务已获用户授权纳入本批，不回头扩R3故障矩阵。

### 8.6 邮箱别名、稳定UUID与真实创建入口（2026-09-11）

本批范围增加已授权的`aily-services/services/auth`，设备协议/固件无变化。原则是“查找目标、保存邀请、投递提醒”三件事分离：**邮箱不是ACL，邀请不是grant，通知不是授权**。

1. Auth新增内部`POST /api/v1/internal/users:resolve-email`，body只有email；复用原get_by_email/write DB，排除deleted/inactive/banned，返回`{status:200,data:{accountId:<UUID>,active:true}}`或data:null。严格X-Service-Token（未配置503/错误401），不接受用户Bearer或query token。无新增账号字段、迁移或邮箱修改限制。Kong两份配置明确阻止该公网路径。
2. Gateway `aily-identity-directory.js`是唯一邮箱权威适配器；index已注入原sharing handler。设置`AILY_AUTH_DIRECTORY_BASE_URL`为私网Auth地址、`AILY_AUTH_SERVICE_TOKEN`匹配其SERVICE_TOKEN；不回退到公共Kong地址/SECRET_KEY/hash query。邮箱仅放POST body，不缓存、不记录；禁止重定向，响应≤4096 B，整个查询含响应体≤8 s，在途≤8，未结束的超时依赖仍占槽。
3. owner设备权限校验后才查目录。固定时间桶：owner 10/min＋100/day，socket peer IP 30/min＋300/day，全局300/min＋3000/day；本地计数上限4096键，加Redis共享计数，Redis不可用则503，不以本地兜底放开跨实例限制。桶切换可能边界突发，不是严格滚动24 h配额。计数对象是查询尝试（不存在的邮箱也计入），与recipient pending64、device pending16是不同预算。反向代理后的IP会合并保守限流，不相信任意X-Forwarded-For；后续部署可信代理策略再独立配置。
4. 目录得出UUID后仍走原create事务/digest/目标约束，不把邮箱写入邀请或grant。新邮箱映射同UUID时同操作可重放；旧邮箱不存在返回404；旧邮箱被另一个UUID使用时，同Idempotency-Key返回409、旧目标不变。用户只有在核对后显式开始新操作才可以邀请新的邮箱持有人。首次解析到提交间邮箱可能变化，但已解析目标固定，绝不在accept时按邮箱重新定位。
5. App一个createInvitation接口兼容两种创建：省略recipientAccount生成原链接；填写规范邮箱创建targeted邀请且禁止携带proof或静默回退。两个可见入口复用默认viewer/可选operator和已有收件箱/确认页。重试Key隔离账号session、设备、权限和邮箱；页面退出/换号后的旧创建响应不展示链接、不写新账号目录。失败按目录不可用/查无账号/限流/重复/幂等冲突提示；邮箱、proof不写日志。
6. 定向邀请成功只提示“邀请已创建，对方可在分享给我的查看；系统通知尚未接通”，不生成假消息。真实notify仍沿用Q2持久意图→messageId→安装/ntfy；Q3剩余消息动作、原生扫码/HTTPS关联/账号切换真机，不重写accept。开放链接与未来二维码同用原proof，不被定向邀请取代。

**账号生命周期边界**：本次权威解析拒绝失活/删除账号，但不是实时撤销所有既有授权的同步器。Auth已有邮箱修改API；软删除后邮箱登录可能恢复原UUID，不应宣称恢复必然获得新UUID。后续部署门禁须核对Auth注销/封禁→Gateway主体失活与邀请终止→既有grant退休，恢复账号不得重放旧邀请恢复权限；归Q2/Q3上线检查与B2，不能凭目录测试宣布该全链已完成。

验证与部署范围见Service§34、App§49、Auth内部邮箱目录文档。先完成通知/真实双账号创建与领取验收，再R4双账号LAN/MCU撤权，随后B1～B4。没有新增设备凭据槽、账户MQTT连接或BBP消息种类；本批不部署常驻环境、不操作真实账户/Flash/网络、不暂存/提交/推送。

### 8.7 Q2a：持久站内消息与统一邀请动作（2026-09-11）

本批完成三个边界明确的工作：邀请生成站内消息、原消息中心HTTP接线、消息详情跳转既有确认页。**通知意图、站内消息、系统推送回执、用户接受是四种不同事实**，不能复用一个成功标记。原邀请/授权事务、账户MQTT和设备协议不变。

1. migration 034新增`v2_user_messages`，以`accountId + sourceType + sourceId`唯一约束幂等，随机messageId不是邀请proof。原邀请增加`notification_message_id`，只表示站内消息检查点；生成消息不清`notification_pending`。未知Gateway登录缓存的有效Auth接收者也能先收到持久记录，不伪造本地账号。
2. 分享来源复用现有有界reconcile worker：每轮20条、并行2条、默认5秒；消费前复核pending、期限、设备/owner及已知接收者状态。消息插入与原邀请检查点同事务，跨worker锁竞争跳过、失败下轮重试，重复执行不重建已读/已删除消息。无推送网络调用或无限任务队列。
3. 通用消息仓库与HTTP不直接决定分享权限。认证消息列表、详情、未读汇总、单条/快照全部已读、删除只作用于当前账号；cursor是本人messageId对应的服务端顺序。快照与同账号插入共用事务锁，避免尚未提交的新消息落入旧“全部已读”范围。已读/删除不接受或拒绝邀请；删除为墓碑，仍可作分页锚点。
4. 仅认证详情可返回严格动作`{type:'share_invitation',invitationId}`；列表及未来推送不携带授权动作/proof/Key。分享来源按当前邀请状态解析动作，已接受/拒绝/撤销或主体失活后为null；消息随邀请到期不可读，不作为长期审计记录。App点击时重新读详情，再`stageDirected`进入原确认页，由用户明确接受/拒绝；不解析正文或任意URL、不另建accept实现。
5. App补请求/账号epoch/销毁守卫，旧账号消息详情与晚到动作不得串入新会话；重复点击只启动一次。消息页显式采用Eager检测以适配现有可变MessageService，修正异步详情到达后按钮未刷新的问题，未引入第二套消息状态框架。
6. Gateway已有注销事务现在原子关闭该账号发出/收到的pending邀请并软删除收到的消息。**这不是Auth封禁/注销→Gateway的完整同步**，更不是全部grant退休验证；恢复账号不能靠消息动作自动恢复权限，该部署闭环仍归Q3/B2。

软件门禁：消息PG **11/11**（含父项；双worker、回滚重试、跨账号、删除幂等、快照并发、注销、有界批次）；现已加入正式`test:sharing`，完整分享组连续两轮 **57/57**，覆盖撤销中的邀请行锁；真实Gateway dispatcher/auth夹具 **1/1**；正式Device V2链通过（最后组 **99/99**）；App消息/分享/ntfy/deep-link/interceptor **113/113**，两端类型检查通过。各组含重叠，不累计计数。软件测试不是通知、APK或MCU真机送达证据。详见Service§35、App§50与ignored `build/lan-sharing-invitation-20260911/`。

**下一项只推进Q2b→Q3→R4，不回头扩R3：**

- Q2b先核对现有ntfy服务部署与安装凭据合同，补认证installation注册/刷新/退出撤销，再用原消息ID投递；每个安装独立保存有界重试与回执。没有有效安装是“仅站内可见”，不是push成功；发送成功也不是用户已收到/接受。不能向公共topic广播邀请或把raw凭据写日志。
- 投递前复核消息未删除/未过期、邀请可操作、安装仍属于当前账号。终态/账号变化不续发；外部网络调用在SQL事务外，断点重试沿用同messageId，手机收到重复提醒也只能走原确认页。`notification_pending`的最终消费规则须随安装投递检查点一起实现，不能在Q2a提前清除。
- Q3在修好Auth/Kong本机环境后验证真实邮箱邀请→站内消息→显式领取，以及禁通知、重复/迟到、换账号、安装撤销、App冷启动/杀进程与账号生命周期。之后R4实体LAN撤权，再B1/B2/B3/B4。历史消息/墓碑清理须制定保留期，不能直接硬删导致重放生成或游标失效；当前无清理worker。

本批未迁移常驻业务库、未部署/安装APK/修改设备Flash或网络、未暂存/提交/推送；系统通知提示仍保留“尚未接通”。设备库只更新主线与测试证据，生产源码不改。

### 8.8 Q2b1安装生命周期完成；下一项为Q2b2（2026-09-11）

已补原App约定的认证安装POST/DELETE、migration035、独立ntfy HTTP适配器、现有vault封装与有界后台worker。每个安装独立用户/只读主题/token，最多8个未退休安装；不是家庭共享凭据或MCU槽位。202只表示持久受理，active才返回密文恢复后的凭据；注销先revoke_pending，实际远端删除后revoked。密码只留服务端，App不持管理员token。近到期POST更新原安装，超时/崩溃/重复/换号按原幂等和lease/state fence恢复。

本机无既有ntfy，本批使用固定官方镜像的临时私有实例验证，不把公共主题当私有服务。真实ntfy＋PG＋实际Gateway注册/撤销接线 **12/12**，包含已建立订阅断开与旧token失效；App修正退出/其它topic通知事件和宽松凭据校验，相关 **46/46**；原分享 **57/57**，正式Device V2链通过，两端类型检查通过。只测试了测试通知载荷透传，**未实现邀请消息的生产投递worker、未向真实手机发送邀请通知**。

详细合同、部署要求、源码依据与下一步见[安装/投递方案](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)。下一批先Q2b2原messageId的每安装投递及回执/取消，再Q3真实双账号/Android、账号生命周期和R4实体LAN；不扩设备协议、不把ntfy已接入等同分享提醒上线。常驻开关未开启，App“系统通知尚未接通”文案保留。本批未部署常驻库/装APK/刷板/改网络或暂存/提交/推送。

### 8.9 Q2b2每安装投递完成；下一项Q3（2026-09-11）

通用消息投递仓库与邀请来源/ntfy适配器分离，复用原worker；migration036持久保存消息fanout/completed/outcome和每安装pending/accepted/cancelled、lease/退避、精确供应商受理ID。只选消息生成前创建且当前有效的安装，最多8个，集合冻结后不补新手机；无安装只站内可见。每轮20消息、并行2、每消息本轮最多一安装；网络不占事务，不拿MCU凭据槽或第二条账户MQTT实现通知。

通知仅固定通用提示＋messageId；发布使用独立只写用户，非管理token。发送前后复核来源/账号/消息/安装，失效取消；服务端受理不等于手机显示，丢检查点可能同ID重发。无缓存避免供应商重连重放陈旧提醒，离线由站内兜底；晚到受理可保留事实但不恢复安装/邀请。全部投递终态后，原来源worker才清notification_pending；已读/删除仍不接受/拒绝邀请。

隔离真实ntfy/PG/实际Gateway接线 **25/25**，包含实时订阅→同messageId认证详情/原邀请preview，未自动授予grant。原分享 **57/57**、默认关闭index **1/1**、App相关 **46/46**、正式Device V2链通过（最后99），类型/Compose通过。测试中出现的HTTP429由退避正确保留意图，仅提高临时实例集中测试预算，不修改常驻配置。

下一项按[通知方案§7](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)：Q3 Auth/Kong/私有HTTPS/Publisher部署→真实双账号/系统通知/明确领取和账号生命周期→R4实体LAN撤权→基础B1～B4。App/MCU生产源码未改、未装APK或修改网络/Flash，无常驻迁移、业务邀请、暂存/提交/推送；不能再把Q2b2记为“未写代码”，也不能把软件通过记成手机上线。

### 8.10 Q3a私有通知基础设施通过；账号部署范围待确认（2026-09-11）

已部署独立持久ntfy＋Nginx只读HTTPS入口，原生产provider的合成安装/实际发布、匿名/跨主题/管理/发布拒绝、45秒保活、容器重启后原token恢复、删除用户断流等10组边界检查通过。Android已安装debug App通过原生HTTP与原CA访问该端点并获得预期匿名403；仅USB映射的TLS可达检查，不是原生系统通知或真实邀请送达。Gateway两开关仍关闭，未改App/MCU生产代码或真实绑定。

Auth源码目录路由尚未被常驻进程加载；Kong源码＋原插件隔离parse成功，但常驻容器缺启用/挂载。Auth数据库credit004到源码head另有5个积分/奖励/订阅权益迁移，启动会自动执行，超出本次分享目录；未擅自迁移或热重载，已请求确认。完整Compose还缺无关服务env，应保持最小服务部署，不伪造账号或改鉴权绕过。

按[通知方案§8](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)继续：明确Auth升级范围→原Auth/Kong接线→Gateway备份迁移/installation再delivery→真实两账号邀请/系统通知/明确领取与退出撤销→R4。不要把“服务已启动”作为修改用户通知成功文案的条件，也不继续扩展通知供应商框架。

### 8.11 Q3b Auth/Kong完成，先部署消费端再迁移Gateway（2026-09-11）

用户已明确允许备份升级本机Auth；5项迁移经隔离恢复预演后实际应用，原手机登录保留。新Auth `/verify`有UUID/user_type后才更新Kong；仅复用原服务定义的extends入口，避免无关env阻断或复制鉴权配置。真实目录解析同一账号、匿名/用户token与身份头冒充拒绝、内部路径外部404均通过；Auth35项/原通知25项回归通过。

Gateway候选镜像构建及真实PG备份→隔离恢复→原迁移runner幂等检查完成，旧40表行数保持。**现场版本30而非32，实际需要31～36**；两个旧Broker还没有当前分享失效消费端模块。下一步按[通知方案§9](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)先做Broker原SQLite备份/兼容更新，再Gateway新备份/vault与31～36、分阶段开通知。通知仍关，尚无双账号邀请/系统显示新证据，不能跳到R4或重开网关高级设计。

### 8.12 Q3c实际部署与Android安装生命周期通过（2026-09-11）

本节替代§8.11的待部署状态。实际静态0/1＋动态2共3个Broker均完成一致性备份、隔离恢复与原迁移幂等检查，再按原配置更新；Controller动态模板同步，保留reservation、派生凭据和卷。Gateway刷新PG/vault备份后实际迁移31～36并健康运行。回填2条placement回执均在shard0，不能冒充双Broker或设备ACK。部署细节与恢复档案见[通知方案§10](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)。

真实手机修订debug APK保留登录数据；原NtfyService注册active、严格HTTPS、3次重启订阅、跨原10秒误报窗口保持连接、精确sequence_id接收及Android通知栏记录均通过。生产revoke接口停止原生订阅/清本地历史、服务端revoked及旧token拒绝，再恢复同账号新安装通过；其他账号待清理记录保留。该门禁没有退出账号或创建分享，不代表换号/离线退出/真实邀请已通过。

App只修重复权限请求，原生依赖只修运行状态/旧worker连接所有权并移除无用引用；不扩页面、设备协议或MQTT连接。测试CA仍通过原debug-only仓外配置注入，release不信任测试CA。Broker285/285、App20＋58、原生JVM5/5、构建/Compose通过。原生修正在独立`capacitor-ntfy`仓库，正式构建需刷新file依赖快照或发布版本，不可漏带。

下一批受控开启delivery前核对真实账号/设备、安装恢复，依次验证邮箱定向和开放邀请→站内消息/系统提醒→认证详情→明确接受/拒绝，补点击路由、冷启动、换号/离线退出及Auth主体同步，再R4实体LAN撤权。当前installation开、delivery和跨shard分享开关仍关；本批未刷板/清绑定、未暂存/提交/推送，不转向100次网关循环。

### 8.13 Q3d真实warm通知点击完成；收敛后台恢复断点

2026-09-11。参见[通知方案§11](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)。原生插件仅提供指向本地历史的单次点击，App复用当前账号/安装检查及原消息页/邀请确认，不新增设备协议或分享授权实现。普通依赖安装/Gradle制品已覆盖原生修改，App84/84、原生8/8与构建通过。

真实两账号完成邮箱定向viewer邀请→生产投递→系统通知点击→认证消息详情→原确认页拒绝；开放链接预览不授予权限，owner撤销后再次预览拒绝。5条本批邀请终态为1拒绝＋4撤销，无待处理来源/消息投递、无接受share；原账号/原页面与通知订阅已恢复。delivery在本机已开启，跨shard分享仍关。

进程终止恢复后FGS存在且曾记录connected，但两次后续邀请accepted而手机未收精确message，实际冷通知点击未通过。它不是已确认的系统限制或加密问题，不靠100次循环掩盖；先核对状态新鲜度/实际socket/网络和系统调度，再补冷启动/接受分享/离线退出/账号主体同步，随后R4与B1～B4。完整观察与私有证据边界归Service§11，不重复扩测试框架。

### 8.14 Q3e真实授权链闭环与通知边界修订

2026-09-11。[通知方案§12](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)给出本机目标UID的freeze=1、目标进程frozen=1、原生线程do_freezer_trap及回前台归0的证据。具体系统策略来源未确认，不增加隐藏保活；原生只补实际流观察，不用持久connected冒充健康。Activity结束后真实通知点击重建页面/认证详情/拒绝通过，但不是冻结中送达或进程死亡冷启动通过。

本机两真实账号在shard1/0，受控开启既有跨片开关，原认证邀请列表→明确接受viewer→自动进入对应设备页通过。owner公开撤销后grant为revoked v2，接收列表移除目标、旧已接受邀请不重新授予权限；两真实Broker均有当前版本/route的精确回执。没有物理LAN连接/Retire ACK，不计R4完成。原账号恢复、测试授权撤销、跨片开关恢复关，保留审计，installation/delivery继续开。

App97/97、原生8/8及构建通过；清理无消费者事件与旧快捷入口注释。通知是提醒，下一批进入**R4最小物理撤权**；后台兼容性、进程冷启动、账号生命周期、QR/组件权限继续单列未完成且约束发布，不能再把后台即时提醒当授权前置。随后B1/B2→B3→B4；不扩网关测试循环，未刷板/清绑定/暂存/提交/推送。

### 8.15 R4 双账号物理 LAN 授权链

2026-09-12。两次真实operator邀请/确认、LAN Property/Action、owner撤销、设备精确Retire ACK、旧连接关闭/签发403及重分享恢复已过；共享账号同一MQTT不重连，owner同设备权限保留。测试分享最终revoked v6，保留审计，原账号/首页/通知恢复。完整记录与未测边界见[集中报告](DEVICE_V2_LAN_R4_HARDWARE_REPORT.md)。

第二物理设备同账号并行隔离仍待补；下一批与B1普通WiFi标准例程见证合并，再回B1/B2。分享撤销不是owner解绑，旧session拒绝不是完整无线旧wire重放测试，通知后台兼容性/QR/逐组件仍约束发布。无新设备协议/UI重构、无暂存/提交/推送。
