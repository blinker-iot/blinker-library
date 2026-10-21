# Device V2 H2：在线周期历史执行方案

更新：2026-10-20。**H2a-1～3 软件完成；H2a-4 必要功能有限真机及保全完成，云端续验见 §8.6。** 同值新样本、离页／关闭 App 后继续归档及真实分享读／禁写／撤权取原本机证据；云端已补真实控制 ACK／新鲜 State、单板断线恢复新段／曲线缺口及停用停止，COM11 已精确恢复原完整 Flash／授权，App 账号／真实 journals／native及连接回收已终核。原 Broker／本地续批分别因 Docker 调整／引擎停止中断，其旧策略仍待原库只读协调，不能由新云端验收冒充迁移恢复或正常停用。部署及支持范围取[独立云端计划](DEVICE_V2_CLOUD_BETA_H2_DEPLOYMENT_PLAN.md)，不搬本机身份、数据或演示私钥。既有软件证据取 §7；L1／L2、H1、R4 固定四槽 Plain 不重开，Wio 暂缓、Error 8 独立待定位，不扩槽、不升级 SDK、不自动提交推送。

## 1 目标与分层

当前采样／上传／保存周期及多字段合并的源码口径见[§9](#9-采样上传与历史保存周期)，固定查询窗口的软件进度取[云端计划D2](DEVICE_V2_CLOUD_BETA_H2_DEPLOYMENT_PLAN.md#d2-固定历史窗口本批最小范围)。不把H2a已验收与新查询页面尚未安装混为同一运行结果。

先让在线 WiFi 设备在用户离开页面后，仍按明确策略留下低频趋势数据。复用设备 `realtime()`／`onSample()`、Cloud Telemetry 聚合、既有逻辑设备身份、鉴权和名称解析，不另建采样执行器，也不把周期样本伪装成 State 来写 H1 或触发自动化。

| 路径 | 需求拥有者 | 保存和时间语义 |
| --- | --- | --- |
| 现有 Live | 可见页面／其他现有消费者 | 有界内存曲线、best-effort；离页释放本页面需求 |
| 现有 H1 | 被 Broker 采纳的 State | 基线／实际变化；相同 Patch 值去重，`serverAcceptedAt` |
| **H2a 在线趋势归档，本阶段** | 持久服务端归档策略 | 独立数值样本表；重复测量值保留，`serverReceivedAt`，无离线补传保证 |
| H2b 增强可靠性，后置 | 独立评审后确定 | 真正采样 UTC／时间质量、有界待确认样本、应用提交 ACK／幂等重试 |
| H3／Bulk，后置 | 独立存储准入 | Flash 离线队列、断电补传、大批次等，不是本阶段依赖 |

H2a 是原[实时与历史计划 §4.2](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md#42-h2-在线周期测量)的**缩小首阶段**，不是完整 H2 的替代完成证明。MQTT 发送成功／PUBACK 不等于历史已提交；H2a 不新增设备持久 ACK。现有 SQLite 为 WAL＋`synchronous=NORMAL`，事务成功可用于进程重启恢复，不作突然掉电绝不丢数据承诺。

## 2 冻结的最小产品合同

### 2.1 策略、能力和生命周期

- 默认关闭，一设备一份策略；仅 owner 能配置，读者按既有设备 read/share-read 权限查趋势。归档属于设备／owner，不伪装成长驻 App session；分享撤权不停止 owner 的归档。
- 首版固定 **60,000 ms**，每策略最多 **4 个数值字段**（类型 1～4）。这是服务端产品上限，不是增加设备租约槽。字段必须是现有实时链路支持的可读、不可写 Property，已声明 `telemetryMinimumIntervalMs`。
- 精确 endpoint key＋Manifest fingerprint 绑定，不保存临时 field ID 或昵称作为身份。能力变化后暂停并待 owner 重新确认，不把同名新能力静默当成原字段。
- 策略 revision／expectedRevision CAS；首次创建 expectedRevision=0，禁用保留 revision 防止 ABA。旧 revision 不得覆盖新配置；策略提交不表示设备已开始采样。
- 策略持久化，进程重启／设备重新 ready 后重新协调；设备断线保留策略、停止旧运行态。设备退休阻止采样、续租、写入及读取；停用只移除归档需求，不能误关其他页面的实时流。
- 页面关闭不移除归档需求；无归档且无页面消费者时，仍按既有 Close／TTL 回收。首期只做直接 Cloud WiFi，不承诺无在线网关的纯 BLE 能连续云归档。

### 2.2 复用一个 Cloud 需求聚合

在 `telemetry.ts` 使用明确的 ArchiveDemand，与页面需求一起计算字段并集和频率；仍只使用同一个 Cloud lease，不为归档占第三个设备槽，不另建 MQTT 连接／高频轮询。

`targetInterval = max(min(各需求请求间隔), max(并集字段 minimumInterval))`。必须补不同字段 floor 的回归，避免设备确认较慢间隔后反复 Close／Open。并集仍受既有字段数、报文及资源上限约束；若影响页面实时频率，明确反馈实际频率或拒绝不兼容需求，不能宣称归档零成本。

H2a-2 采用页面优先的有限策略：归档字段使总数超过现有 32、使数值／布尔 CBOR 保守上界超过已协商帧预算，或使已有页面必须变慢时，仅暂停归档为 `capacity`／`cadence`，不拒绝本来能准入的页面。无法静态证明的变长实时字段，不据此批准新增归档字段；原字段子集不增加报文字节，可继续共用。页面需求解除后再协调恢复归档；不临时增加第二条 Cloud 流。

现有 wire 最大间隔为 60,000 ms；300 s 设备采样不是现成能力。首版不扩 wire；如以后每 5 分钟保存，须明确是每分钟采样后降采样，功耗／网络成本不等同每 5 分钟设备采样。

采集点在 Broker 完成当前 tenant／stream／epoch／租期／新序号／字段集合校验后、逐页面降频／重编号／latest-only 合并之前。归档按自身节拍选样，不能把页面 1 Hz 全量落库。选样用单调时间，不按接收 UTC 的分钟整点补造点；新流／序号回绕／策略变更新建观测段。

### 2.3 样本、去重和失败

- 服务端为每个上游运行代际生成 segment UUID；样本身份为逻辑设备＋segment＋上游 sequence＋精确字段 key。相同值的新测量照常保存；同身份同内容幂等，同身份不同内容拒绝。不以值变化或单独 streamId 去重。
- 保存 fingerprint、policy revision、stream／epoch、上游 sequence、设备 monotonic、确认的有效间隔和服务器接收时间。明确 `timeBasis=serverReceivedAt`、`timeQuality=receipt-only`，不能标为设备采样 UTC；单调时间只解释节拍／连续性。
- canonical CBOR、类型／约束及解码值一致性复用既有 codec；一笔选中样本的字段同事务写入，失败无部分记录。裁剪按完整样本，最多少保留 3 行；幂等仅覆盖仍在保留窗口内的样本身份，不承诺永久重传去重。运行选样游标只在成功后推进。
- 不写 latest State／H1，不触发历史重放自动化，不进入设备 ReliableOutbox。数据库失败不抛断普通实时／控制连接，不无限排队、不补造缺口；运行接线需给出有界失败状态。

### 2.4 保留和查询

- 独立表与独立额度：首版 **30 天且每设备最多 200,000 个字段样本，先到为准**；4 字段每分钟约 172,800 行／30 天。不是沿用 H1 每设备 10,000 行，更不是对所有字段数无限承诺 30 天。上线前仍需按设备总量作部署容量核对。
- 读时过滤过期数据，写时物理裁剪本设备；查询半开区间 `[from,to)`，跨度最多 30 天、每页最多 50 行、响应体有界。ID 用十进制字符串，对数据库 INTEGER 数值排序；游标绑定 tenant／device／key／范围／页大小并冻结 upper ID，隔离并发新增。
- Gateway 沿用现有真实 shard 路由及返回前新鲜权限复核，App 无权指定 tenant／shard。策略配置 owner-only，查询 share-read；不把内部 store 的 tenant 检查当成完整用户授权。
- App 独立标记“在线趋势”，复用名称／别名。原始分页、有界图点先行；离线／缺样断线，不插值制造连续历史。累计量、分桶统计和长期汇总本批不做，不一律求平均。
- 旧 tenant migration v3 不含 H2：只要存在策略（即使停用）或样本，export／旧 ledger cleanup 均必须拒绝，避免静默丢配置／数据。

## 3 模块边界

| 模块 | 本阶段责任 | 不负责 |
| --- | --- | --- |
| Broker periodic-history store | policy CAS、数值样本事务、幂等、保留、分页 | 用户 token、MQTT、采样调度 |
| Broker telemetry archive adapter | policy 协调、单 Cloud 聚合、选样、故障隔离 | App 页面存活、独立设备执行器 |
| Gateway | owner 配置、read 查询、新鲜 ACL、shard | 二次采样、伪造源时间 |
| App | 配置／状态／有限趋势、名称、错误恢复 | Flash、协议重造、假连续曲线 |
| 设备库 | 复用现有采样和租约，不变更 | 新持久队列、槽位扩充、HistoryBatch codec |

## 4 执行顺序与关闭条件

| 批次 | 最小范围 | 关闭条件／状态 |
| --- | --- | --- |
| **H2a-1，本批完成** | 本合同、独立 policy/sample store、数据库初始化、迁移防丢护栏 | SQLite 事务／CAS／去重／范围／保留／重开文件及 H1、迁移有限回归通过；不接自动采样或外部接口 |
| **H2a-2，本批完成** | 显式 ArchiveDemand、上游采集、ready／重启／断线／Manifest／退休协调 | 单 lease、floor、页退出继续、归档关闭不误关页面、失败隔离及旧流 fence 有限回归通过，见 §6 |
| **H2a-3，软件完成** | Gateway owner-only 配置／共享只读查询，App 最小入口 | 权限及撤权 fence、策略确认与样本状态分开、名称／范围／有界曲线／错误恢复有限回归通过，详见 §7；手机实测仍属 H2a-4 |
| H2a-4 | 一台 WiFi 设备有限 App 真机短验 | 必要功能及保全有限通过，分环境证据见 §8；COM11 原固件／授权已恢复，App 最终保全与连接回收通过；不把新云端停用代替原本机未知策略协调 |
| H2b／H3 | 按需求另立范围 | 本计划不自动开启，不用 H2a 宣称可靠离线历史已完成 |

本批不部署 App／服务，不刷板／改授权／网络，不重复 BLE 或断电循环。后续真机沿当前用户网络重新核对，不能沿用旧 IP。账号、授权、UNKNOWN、完整请求日志及无关本地修改保留。

## 5 本批实施记录

### 5.1 实际变更与有限结果

- 新增 Broker [`periodic-history.ts`](../../aily-blinker-services/services/broker/src/periodic-history.ts)，独立策略／样本表，CAS、绑定能力、样本原子提交与去重、保留及范围分页；数据库初始化只建表，**不创建策略、不调用设备或启动归档**。
- 复用旧 tenant migration v3 的统一防丢入口，增加 H2 策略／样本检查。即使只有停用策略，或旧 export ledger 后才出现归档，也拒绝 export／cleanup。
- 新增 [`test-periodic-history.js`](../../aily-blinker-services/services/broker/test-periodic-history.js) 28 项有限回归，接入既有 `test:state-history` 和 `test:device-v2`，保留原脚本中其他本地修改。
- 修正代码审阅发现的两项实现缺陷：选中原 Manifest 中非连续 ID 的字段时，用完整 Manifest 作 codec 校验；200,000 行边界按完整样本裁剪，避免残留半笔样本并错误拒绝正常重传。对应单字段 ID>1、key／ID 反序和三字段跨界回归通过。
- Broker TypeScript 原依赖完整编译及 `--noEmit` 检查通过；本机 Windows 首次 SQLite 运行因缺少 `better-sqlite3` 原生 binding 停在夹具创建，未作为业务失败或成功。未安装／升级依赖，转用现有 Broker 镜像的原生依赖，在 **无网络、只读挂载候选、仅临时 tmpfs 数据库** 的独立容器内执行 H2、H1、实时 session 与 capacity／恢复／ACL／慢消费者回归，全部通过、无跳过。未挂载业务数据库、未启动或变更现有服务。

上述结果是确定性软件和 SQLite 文件重开证据，不是新设备、App 或突然掉电保证。设备源码、SDK、App、账号、授权、UNKNOWN、请求日志及网络配置均未操作，未暂存、提交或推送。

独立窄审确认两项缺陷修正及测试匹配，未发现本批新增阻断。文档本地链接和 diff 空白检查通过；三仓 HEAD／分支／索引不变。终核同时发现 App 首页卡片方向新增五项并行改动，与本批文件不重叠，原有 App 脏条目摘要未变；本批不写入／撤回这些改动，不将其计为 H2 成果。

### 5.2 H2a-1 结束时确定的顺序

当时下一步为 **H2a-2**：只接明确 ArchiveDemand、现有 Cloud 聚合及上游选样；先补不同字段 floor／容量／迟到响应回归，再接 ready、重启、断线、Manifest 变化和退休协调。该批现按 §6 完成，不再重复派发。

H2a-1 结束时 Gateway／App 尚未接，当批不能从 UI 开启趋势；后续入口现按 §7 实现，但尚未部署。各 shard 的新表／迁移保护和运行适配必须匹配后才允许开启策略，旧 Broker 回滚不得静默忽略已有 H2 数据。不扩大为 H2b、H3 或 Bulk，也不回头重开 R4 资源／BLE 审计。

## 6 H2a-2：Cloud 聚合与生命周期有限结果

### 6.1 最小实现

- [`telemetry.ts`](../../aily-blinker-services/services/broker/src/protocol/v2/telemetry.ts) 保留原页面 consumer、身份、限额、降频和 latest-only；增加独立归档需求及采样段。请求间隔与设备确认间隔分开，字段 floor 归一化后不因 Opened 回应循环重开。
- [`periodic-history-runtime.ts`](../../aily-blinker-services/services/broker/src/periodic-history-runtime.ts) 是数据库／真实物理 session 薄适配：选择当前策略与 Manifest，捕获准确 owner；在 due／续租边界以小查询复核 revision、启停、设备 active、Manifest、session 当前性及迁移冻结。落库复用 H2a-1，取得既有 tenant write fence 并 finally 释放。
- 合法上游样本在页面处理前选取；首次立即记录，后续按设备 monotonic 至少 60 s。相同值不去重；新 stream、epoch、序号回绕、policy revision 或 owner 使用新段，不关联旧样本。失败不前移已提交采样游标，不重放旧序号，不积压补传队列。
- 全局 **最多 128 份归档运行态与首次来源故障重试条目合计**，并非设备槽；不全量加载策略表。超过上限返回 `capacity`，不为超限设备保留条目；释放后由后续显式 refresh 或重连重试，不承诺无限自动排队或公平轮转。故障计数饱和到 65,535，不输出原始错误／密钥。

### 6.2 生命周期与故障边界

| 情形 | 现行处理 |
| --- | --- |
| Broker 重启／设备 ready | 从持久策略重建，不保存运行 stream；ready 按 claims／fingerprint 去重，不每帧解码目录或重置采样段 |
| 页面退出／归档停用 | 只移除所属需求；另一需求存在时仍由同一聚合协调 |
| 实际断线 | 清旧上游／运行段，保留策略；新认证 ready 后重新 Open，旧 stream／epoch 不再入库 |
| State 重同步 | 显式 Suspend，保留页面 consumer，先 Close 已知旧 lease，再 ready／Open；不能只删除旧需求导致争槽。只有 Close 可在当前认证但非 fresh 状态发送，Open／Renew 仍要求 fresh；队列跳过失效 job 不丢后续 Close |
| Manifest 变化 | 原 fingerprint 策略失活，不能沿用同名字段；新能力与策略重新确认是 H2a-3 的产品入口 |
| 退休 | deprovision 提交后立即停止引擎需求，不只依赖 socket close 事件；数据库复核亦拒绝旧提交，不删除历史／授权日志 |
| 来源／数据库异常 | 状态 `source-error`／`storage-error`，不抛给 Broker 畸形包处理，不关闭页面／控制连接；首次读取失败也受有界维护重试，恢复后只处理新样本 |
| Open 未确认／迟到回应 | 有限租期结束重新协调；保留精确取消 tombstone，旧 Opened 只能 Close 原 stream／epoch，不能复活旧归档 |

复用既有 5 s sweep，不增加计时器。正常迁移 freeze 路由会 drain 连接，解冻后由重新就绪恢复；不把绕过该流程直接改数据库当成受支持的恢复入口。Close 仍是原 best-effort＋TTL／连接生命周期，不新增可靠关闭保证。

### 6.3 实际验证与未测

新增 [`test-telemetry-archive.js`](../../aily-blinker-services/services/broker/test-telemetry-archive.js) **31 项**和 [`test-periodic-history-runtime.js`](../../aily-blinker-services/services/broker/test-periodic-history-runtime.js) **24 项**，最终匹配编译的 **55/55 通过，无跳过**。包含：真实 SQLite／registry 联合落库、关闭重开文件及新 owner 恢复、同值保留、不污染 State／H1／command ledger、页退出／停用、容量／floor／报文字节预算、写失败／来源失败恢复、Manifest／退休 fence、epoch／wrap、新旧回应隔离，以及实际 `setupBroker` 发布队列的 Open 挂起→失 fresh→跳过 Renew→精确 Close→新 Open 顺序。字节预算例为 17 个整数实时字段的保守上界 252 B 保持原流，新增归档字段后上界 264 B 超过 peer 256 B 时只暂停归档；这不是实际样本字节或性能测量。

H2a-1、H1、原实时 session／capacity／ACL／慢消费者、App relay telemetry、用户实时控制、command admission、local control 和 task carrier 作有限关联回归。扩大清单首轮 **191/193**，两项任务 HTTP→Broker→MQTT 测试因隔离镜像未挂 Gateway 源码首停（`MODULE_NOT_FOUND`）；补充同候选 Gateway 的只读挂载后，仅这两项重跑 **2/2** 通过，未改任务执行链、未据首轮失败伪称业务通过。随后针对审阅修正完成上述 55 项新文件定向终核，不重复已通过长循环。

原依赖 TypeScript 完整编译及 `--noEmit` 通过。所有 SQLite／Aedes 验证均为原镜像、无网络、只读候选源码、临时 tmpfs 数据，无业务数据挂载；Aedes 只捕获发布回调，不运行外部 Broker／App 或硬件。没有部署／刷板／改变账号、授权、UNKNOWN、网络配置和请求日志，没有扩槽、升级 SDK、离线存储或 Bulk；未暂存、提交或推送。

文档链接及 diff 空白检查通过；库／Service 的范围外脏条目摘要保持，原 package 脚本改动在移除本批两处新增测试名后精确匹配。三仓 HEAD／分支／索引不变。App 首页卡片方向在本批期间另有八项并行差异（含既有脏文件继续编辑）；本批没有写入任何 App 文件，未回退或混入其变更。

### 6.4 H2a-2 结束时确定的顺序

继续 **H2a-3**：复用现有 Gateway owner 配置／read 查询／真实 shard 路由和 App 名称／历史显示链路，提供最小在线趋势入口。owner 保存策略后显式调用已有 refresh；策略“已保存”与实际 `waiting`／`active`／暂停／故障状态分开，不能对尚未准入或未出现首样本的设备显示“正在记录”。共享只读查询仍需新鲜 ACL 及返回前撤权复核，配置不开放给共享用户。

该入口的软件完成后再按 H2a-4 备份、部署并做一台 WiFi 设备短验；本批软件通过不代替真机或全部 shard 发布验收。

## 7 H2a-3：Gateway／App 最小入口与软件收口

### 7.1 API 与模块责任

| 用户接口 | 输入和权限 | 结果 |
| --- | --- | --- |
| `GET /api/v2/devices/:id/periodic-history` | `from/to` 必填，`limit≤50`、`key/before` 可选；沿现有设备 read／share-read | H2a-1 原始分页，固定范围／游标，明确接收 UTC 与来源 |
| `GET /api/v2/devices/:id/periodic-history-policy` | read／share-read；无客户端 tenant／shard | 当前数值能力、已保存策略、实际运行状态及 `canConfigure`；读取不打开租约 |
| `PUT /api/v2/devices/:id/periodic-history-policy` | 仅 owner；精确 `{expectedRevision, enabled, manifestFingerprint, keys}` | CAS 成功后协调既有 Cloud 聚合并返回确认；不可自选频率、租户或扩字段数 |

- Broker 新增 [`periodic-history-http.ts`](../../aily-blinker-services/services/broker/src/periodic-history-http.ts) 薄路由；内部均为有 token／精确 shard 的 POST：`periodic-history`、`periodic-history-policy/read` 和 `periodic-history-policy`。复用原 store、迁移 write fence 和 refresh，不新建调度器。读／写／运行接线共用 `isPeriodicHistoryField`，移除三处独立维护的筛选条件。
- Gateway 新增独立 [`periodic-history.js`](../../aily-blinker-services/services/gateway/src/device-v2/periodic-history.js)／[`periodic-history-http.js`](../../aily-blinker-services/services/gateway/src/device-v2/periodic-history-http.js)，接入原用户鉴权、真实 owner／share ACL 和 Broker client。请求前及结果返回前复核版本、关系、目标租户及 shard；即使分享者有控制权，也不能配置 owner 的归档。取消或返回 fence 失败后不重试写入。
- 历史和策略响应均 `private, no-store`，不返回内部 tenant／shard。公共处理最多 16 个进行中请求，body 2 KiB，Broker 内部对应 body 亦 2 KiB；策略响应 32 KiB、历史 256 KiB，并保留原 HTTP 截止／取消。CAS／Manifest 冲突显式 409；不可用不得伪装成功或空历史。
- 运行状态区分 `disabled`、`waiting`、`active`、`capacity`、`cadence`、`unavailable`、`storage-error`、`source-error`、`manifest-changed`；`active` 只表示该段已有提交样本，UI 为“已收到归档样本”，不承诺持续无缺样。GET 只观察；没有首样本不展示“正在记录”。

### 7.2 App 最小产品入口

- 设备设置增加“在线趋势”，独立 [`DeviceTrendsPage`](../../blinker-app/src/app/sections/device/device-trends/device-trends.page.ts)，不改写 H1 状态历史页面、实时租约或自动化执行链。复用账号上下文、名称／别名和原 H1 canonical CBOR 值校验，不增加协议解码器。
- owner 最多选 4 项，只能固定 60 s；分享者只读。Manifest 变更后能停用原精确定义，启用新定义必须核对当前能力。配置确认立即展示，不等待历史刷新；“设置已保存”和“已有归档样本”是两个事实。
- 任何保存未确认都禁止旧版本再次保存，只有新鲜只读查询成功后才能核对并继续，不保存重放队列。刷新开始即禁写；失败可保留原确认用于展示，但不允许以旧 revision 写入。403／404 清除敏感结果。
- 最近 1 h／24 h／7 d 查询、按功能筛选／游标分页；每页 50 行，页面最多 200 个字段样本。接收时间曲线按字段／类型、segment／策略／Manifest 和缺口分段，不插值、不补造离线点；大整数精确文本展示，不转成不精确曲线。最大保留 30 天／200,000 条指字段行，不是每设备无限 30 天。
- 离页、后台和账号变化取消请求／清理数据；无后台查询计时器。离页只回收页面资源，不停用 owner 的持久归档需求。返回前台重新只读查询，不能把旧缓存当新鲜样本。

### 7.3 本批实际验证与首错记录

- Broker 新 [`test-periodic-history-http.js`](../../aily-blinker-services/services/broker/test-periodic-history-http.js) **35 项**。包括原 Fastify token／shard、参数／2 KiB、CAS、Manifest、迁移冻结、退休、真实聚合状态、分页及 H1 隔离；其中 2 项使用真实 Fastify loopback → Gateway `createBrokerClient`／service → SQLite，不 mock Broker 返回，验证 owner／分享只读、无内部路由身份泄漏、真实分页及 CAS 不重试。ACL resolver 为有限替身，不宣称真实双账号验收。
- 最新匹配编译的 Broker **11 文件 231/231**，其中本批 35 项、既有 H2 83 项及 H1／实时／控制／任务关联 113 项，无 skip。使用既有原生依赖镜像、`--network none`、只读源码、仅 `/tmp` tmpfs 的隔离容器；HTTP 仅容器 loopback，无业务数据库挂载，无服务重建。Broker 完整 TypeScript 编译及 `--noEmit` 通过。
- Gateway 新 [`test-device-v2-periodic-history.js`](../../aily-blinker-services/services/gateway/test-device-v2-periodic-history.js) **17 项**＋原 H1 7 项，最终 **24/24**、无 skip；类型检查通过。覆盖 owner-only、read／share、返回前撤权、严格响应、未知错误脱敏、16 pending／body 限制及断开取消。新测试已进入原 `test:device-v2`，Broker 同时接入原历史关联入口，不旁置失联测试。
- App 最终 9 文件 **169/169**（本批 50／关联 119），包括模型、HTTP、页面、生产模板和设置入口；类型检查、原 development 完整 Web／模板构建通过，不是 APK／手机验收。首轮全 App 类型检查遇到并行首页代码引用未齐，未替其修复或回退；其独立工作补齐后重跑全量类型检查与 Web 构建均通过。未改首页文件或依赖／SDK。
- 路由回归揭示原全局错误处理将 body-limit 的 413 改成 500；本模块只在自身路由保留 413／畸形 JSON 400，不扩改其他接口。窄审另修复刷新失败后旧 detail 可能继续写入的问题，并补回归；未删安全 fence 或未知结果分支来缩短代码。

测试原命令（工作目录分别为 Service／App 根；容器须同时只读挂载当前 Broker／Gateway／contracts／device-v2-protocol，沿用 §6 隔离方式）：

```text
node node_modules/typescript/bin/tsc -b services/broker/tsconfig.json --force
node node_modules/typescript/bin/tsc -p services/broker/tsconfig.json --noEmit
node node_modules/typescript/bin/tsc -p services/gateway/tsconfig.json --noEmit
node --test services/gateway/test-device-v2-periodic-history.js services/gateway/test-device-v2-state-history.js
node --test test-periodic-history-http.js test-periodic-history.js test-periodic-history-runtime.js test-telemetry-archive.js test-state-history.js test-telemetry-session.js test-app-telemetry-control.js test-app-relay-telemetry.js test-command-admission.js test-local-control.js test-task-carrier.js
node node_modules/typescript/bin/tsc -p tsconfig.app.json --noEmit
node node_modules/@angular/cli/bin/ng.js build app --configuration development
```

App 定向测试沿原 Angular runner，使用 ignored `build/h2a3-tsconfig.spec.json` 限定上述 9 文件的 spec 编译，不把并行首页测试混计；不修改版本化测试配置或安装依赖。本批没有部署／刷板、创建业务策略、修改账号／授权／UNKNOWN／请求日志或网络，没有暂存／提交／推送。既有无关脏修改和同期首页工作保留，不纳入本阶段成果。

独立只读窄审未发现本批新增阻断，核对了 GET 无开租约、owner 返回 fence、CAS／未知结果无自动重发以及三层严格合同。终核库／Service 范围外脏条目摘要保持，三仓 HEAD／分支／索引不变；App 同期首页另有 16 项差异（含新文件），未写入或回退这些文件，共用路由中的首页改动也保留。两份文档本地链接及改动空白检查通过。

### 7.4 下一批仅 H2a-4

1. 先核对当前网络、运行 Gateway／目标 shard 的实际源码与 schema、App 及现有 WiFi 实时数值能力；备份账号／请求日志、必要数据库和目标固件／授权，不沿用旧 IP／APK 冒充匹配版本。
2. 有限部署匹配 Gateway／目标 Broker／App；由真实 owner 页面选一项数值并开启固定 60 s，确认“已保存 → 等待 → 已收到样本”，保留同值的两次不同采样。不让 UI 缓存代替真实库记录。
3. 离开页面／关闭 App 后继续归档，停用后停止本归档且原控制 ACK／新鲜 State 不回退；一次有限断线／恢复检查新段与缺口，不重做 BLE／整板断电循环或扩为长稳。
4. 分享只读与撤权沿既有真实权限验证；能力变化先用已有软件证据，不为本批扩大新固件产品图。结束停用本批策略，保留 revision／历史／请求日志；不直接删库“恢复为空”，不回滚成会忽略 H2 数据的旧 Broker。固件／授权按实际备份恢复，必要服务保留匹配版本并记录与原配置差异。

未通过的 H2a-4 项逐项保留，不将本次软件通过代替现场、全部 shard、独立同事签收或生产发布。H2b、离线存储、Bulk、新网关、R4／Wio 扩展仍不进入本批。

## 8 H2a-4：部分真机证据、现场中断与有限续批

### 8.1 已完成的真实差异

- 新鲜备份 App 账号／请求日志、COM11 完整 8 MiB、必要数据库／密钥卷及服务配置后，有限部署匹配的 H2 Gateway、Broker shard 1 和 App。沿用原 owner、DeviceKey、身份与授权，不新建设备或调整额度；56 项 Gateway 迁移、原 14 条 UNKNOWN 及旧 ledger 保全核对通过。
- 真实 App 页面选择现有数值能力并保存固定 60 s 策略，revision 1：先显示“设置已保存，等待在线样本”，显式只读刷新后显示“已收到归档样本”。固件使用标明的模拟恒定值 42，不称物理传感器测量。
- SQLite 保留了同一 segment 中序号不同的同值记录。前两个采样间隔的设备 monotonic 均相差 60,000 ms，对应服务器接收差约 59,902／60,008 ms；不以值相同去重，也未按 UTC 整点补点。
- 离开趋势页面后无页面 scope／BLE link，归档继续；随后真正 force-stop App、进程消失后仍出现新记录。该窗口内 latest State 更新时间和 H1 行数不变，H2 未伪装成 State 写入。中断前共保留 4 个真实字段样本。

### 8.2 未完成项与中断后的保护

仅目标 Broker 的停机开始后，用户正在调整本机 Docker，其他服务随后一并停止；本次不再作为“其他服务保持运行的隔离 Broker 重启”证明。已保留完整日志、停机后数据库快照及 SQLite integrity=ok／4 条样本／原策略证据，未直接改 SQL 停用策略、删库或回滚成不认识 H2 的旧 Broker。

COM11 原完整 8 MiB 已独立 ROM 校验并恢复运行，原固件、身份及授权保留；COM9 未操作。App 和服务的已有数据／日志不回滚。局部验收不能关闭恢复后新段／缺口、停用停止、原控制 ACK／新鲜 State 或真实分享只读／撤权。

用户曾提出独立远端环境；只读服务器预检及仓库部署依赖分析不是 H2 真机通过，也不是已部署生产环境。现按用户最新指令回到本地 Docker，远端不在本批操作范围。本机策略仍需通过正常 owner 页面停用收尾，不能把“已恢复固件”误认为“归档策略已关闭”。

### 8.3 本地续批的最小顺序

1. 核对新的“带翅膀的蜗牛”网络、现存 H2 镜像／数据卷及实际 App。新鲜备份配置、日志和账号／journal；只恢复必要测试服务，保留其他项目容器状态，不重建依赖或部署远端。若 App 被并行更新，先核对 H2 相关实际模块，不能直接覆盖同事新版。
2. COM11 仅以原 frozen 候选更新测试 WiFi／profile，服务健康、身份和恢复证据核对后才进入有限刷写；不修改 Key 或授权，不碰其他板。已通过的同值／离页／杀 App 项不再重复。
3. 核对持久策略和历史在实际进程恢复后仍存在；恢复设备后取得新 segment，缺口不补造。此证据明确为实际整组中断后的恢复，不补称原隔离窗口通过。
4. 原控制 ACK／新鲜 State、只读分享／禁止共享用户改策略／撤权拒读分别取证；最终由 owner 正常停用归档并确认停止，保留 revision／历史和完整请求记录。不得重放旧 UNKNOWN。
5. 恢复 COM11 原固件与授权，终核账号／旧请求／服务实际差异并清理本批连接资源。未通过项逐项保留，不扩为长稳、H2b、离线存储或 Bulk，不提交推送。

续批保护已核对：实际 App 被并行工作更新，未回滚／重装。当前 APK 的 H2 model／service／页面及相关鉴权、传输、历史／分享源码与本批候选精确匹配；trends 编译块仅 import 文件名／source map 名变化，归一化字节相同。另有首页卡片、端点并发保护和 UI 权限守卫变更，因此不能称整包相同或只有首页变化，原控制仍须在实际版本短验。新鲜账号／epoch 和原五条任务 journal 精确保持；只恢复因 USB 重连丢失的本批两个 ADB reverse，不重放业务请求。

### 8.4 本地续批已补的恢复与真实 ACL

- 恢复原 H2 Gateway／Broker 镜像和必要服务，仅将 Plain 测试公布地址／18884、17002 绑定改至当前电脑地址；原 Auth 已由外部启动，未另行重启。新鲜备份后核对原策略 revision 1／enabled、4 条样本逐行、56 项迁移、26 个 scope／353 个请求／355 个 attempt 及 14 条 UNKNOWN 保持。无关项目容器不启动、停止或重建。
- 实际 App 经正常只读刷新读到 4 条原样本、同一旧段曲线；恢复后的原固件自然登录并上报另一份 Manifest，页面正确显示“设备能力已变化”，不静默将同名字段视为原归档能力。匹配数值候选仍在编译，恢复后新段及控制／停用尚未据此通过。
- 原 owner 真实 App 创建本轮 viewer 邀请，第二个既有真实账号走正常 Auth refresh、preview 和 accept（`replayed=false`），没有伪造 token 或换掉手机原账号。该账号路由在 shard 0、目标在 shard 1，仅 Gateway 两个既有跨片请求／分享开关临时开启；Broker 0 不启动、Broker 1 及跨片用户 MQTT 开关不改。
- 授权后策略／历史 GET 均 200、4 条历史、`canConfigure=false`；一次精确同策略 PUT 返回 403，随后策略逐字段未变。owner 在真实 App 移除本批成员后，策略／历史 GET 均立即 404，owner 页面 active／pending 成员均为 0，公开访问一直关闭。证明真实双账号 HTTP 权限，不声称另一台手机 UI 或跨片 MQTT 验收。
- 首次 preview 发现目标与接收方已有历史 revoked grant；维护脚本错误预设为 null，在任何 accept intent／写入之前停止。保留首错，改为核对 revoked／viewer／不可编辑的真实前置状态后才接受这次全新邀请；后续成功不是重放旧邀请／UNKNOWN。没有修改生产代码来迁就夹具。

该节为当时的逐项进度，当时整体 H2a-4 仍开；恢复后首个新段、停用停止、原控制 ACK／新鲜 State 及最终固件／服务／账号收尾尚未完成。后续有限续验与收尾结果取 §8.6，不删改本机中断边界。完整私有备份、原首错和逐请求日志留在 ignored build 目录，不进入公开包或 Git。

### 8.5 唯一编译完成、第二次现场中断及云端切换

- 续批只保留原 Arduino 编译队列，原 ESP32 SDK 3.3.11、92 个对象完整链接通过，BIN 947,888 B／静态 RAM 51,392 B。只改变本机 WiFi／profile，不重复完整编译队列、扩大槽位或更新 SDK。该 BIN 严格属于本机 Plain 配置，不能充当云端 HTTPS／MQTTS 候选。
- 用户增加云端部署要求后，先撤销自动烧录准入并保留 guard；为完成原策略／4 条样本恢复，又在全新本地收尾授权与 intent 下核对原完整 Flash／NVS／分区后仅写 COM11 factory。期间没有发送控制、重放 UNKNOWN 或额外串口采样。
- 手机休眠导致调试暂时不可达；用户解锁后核对发现本机 Docker 引擎已停止、原 API 不可达。没有启动或排查用户正在调整的 Docker，不从页面失败推定设备缺陷，也没有再次发起采样／断电循环。候选期间是否增加新样本保留为**未知、待原库只读协调**；新段／离线缺口、正常停用及 ACK／新鲜 State 本轮不宣称通过。
- COM11 原完整 8 MiB 再次精确写回、独立 ROM 全范围校验并恢复运行，SHA256 `7e3616e0d311672e4d13291dc8172180e55f1a6a5ec756e618dbab4ba127ed21`；原固件／身份／授权保留，COM9 未操作。App 原账号／epoch、21 台设备及五条任务 journal 精确保持；没有重装或回灌 App 数据。本批 reverse 与 9339 forward 已回收，其他会话的 forward 保留。
- 本机旧策略最后确认仍 revision 1／enabled。不能将完整 Flash 恢复当作策略停用；原环境日后可用时只读协调未知采样，再由正常 owner 接口停用，保留 revision／历史与日志，不直接 SQL 改写或删库。不要把这笔旧策略搬到云端凑恢复验收。

当时执行入口转为独立云端部署计划：复用已有 Aily 测试服务器的 HTTPS 注册链路，独立 IoT 数据、密钥和新测试身份／设备，待部署完成后只安排匹配新环境的有限控制与 H2 实物验收。后续实际结果取 §8.6；原本机未通过项的证据边界保留，不复制私有授权以省略真实认领。

### 8.6 云端独立环境的有限续验与收尾

- 复用真实测试 Auth 的现有统一账号，独立 IoT 库、密钥及正常 App 新测试设备，不导入本机设备／请求或重放 UNKNOWN。匹配 beta v2 保持独立包及原签名，原 release／debug不覆盖。本批正常邮件请求 HTTP 200但测量时未见倒计时，保留该现象；用户收到本次新验证码后，真实表单登录、`me`、正常创建设备与获取新 Key 均通过。不读取码库或凭据库，不由HTTP 200代称送达。recent-auth 403专用提示仍待最小修正。
- 先新鲜完整备份 COM11 的8,388,608 B、账号／任务和自动化 journals、完整 App native数据，以及云端 PG／SQLite原生备份与完整日志。只在独立保护目录绑定真实测试 profile；432项冻结普通 WiFi 库不改，0槽、原 SDK 3.3.11、唯一单 worker完整链接通过。BIN 1,036,528 B，SHA256 `5ca02e29cee27d08b87eb47abaacbe49069d859b49c9e37e64dbb2c40ce4d01e`；不把本机 Plain BIN冒充云端候选。
- 真实 DeviceKey 经 HTTPS／MQTTS接入，App 沿原 Cloud 命令及同一 Store收取 ACK／新鲜 State。归档前两次 ACK202／224 ms，新鲜 State203／224 ms；单板断线恢复且停用归档后两次 ACK259／144 ms，新鲜 State259／145 ms。本批控制 State revision 2→5；没有造协议帧或用发布返回代替设备确认。该 MCU SDK实际未启用 `MBEDTLS_HAVE_TIME_DATE`，只确认 CA链／hostname，不声称验证了证书有效期，也未关闭 CA／域名验证来通过。
- owner 正常启用固定60 s、实际数值能力的策略。恒定模拟42的两条真实新样本，设备 monotonic间隔60,000 ms、服务器接收间隔59,995 ms；不是物理传感器值，也不因值相同而去重。
- 仅目标板私有一次性 `H2:GAP90` 在明确 intent后断开自身 WiFi并暂停自身 `run`，实测90 s，再交原生命周期恢复。离线期间真实 `unavailable`，原两条历史逐行不变；恢复后新 segment／stream、epoch2→3、sequence重新1，共3条历史。App为2条曲线路径／3点，按新段断开，不跨缺口连线、不补点。上次样本至新样本的132.767 s包含恢复等待，**不是断网时长**；本窗口短于同段180 s空窗阈值，故以真实新段而非时间差单独作断段证据。共用 Broker／服务未重建，既有本机同值／离页／杀 App／真实分享 HTTP项不重复。
- owner 经正常页面停用策略并确认 revision 2；77.829 s后只读读取3条历史逐行不变，原控制仍取得新鲜结果。本批云端新测试设备、停用策略和3条历史保留，不删设备／数据凑恢复。云端前后 PG native／SQLite native备份及健康核对通过；完整请求／原首错／新旧恢复材料私有保留，不进入公开源码或文档。
- COM11 原完整8,388,608 B按本批新鲜备份精确写回，SHA256 `7e3616e0d311672e4d13291dc8172180e55f1a6a5ec756e618dbab4ba127ed21`，全范围独立 ROM验证及 hard reset通过；原身份／授权恢复，未复验原业务上线。恢复原身份后本批云端设备离线符合预期，不当作新环境旧设备迁移。
- App最终完整 native备份4,625,408 B，SHA256 `b968fe65e6c13dd8055cb7a2bc7b49f287512cda54c0d112926c6f8c796b998f`。仅正常停启beta作一致备份，重启后原owner及全部真实任务／自动化 journals逐行全等、APK不变；保留真实新登录session，不还原注销前token。回首页后 device scopes／BLE links均0、回收本批专属 forward，原两包／R4／服务配置不动，最终保全与回收证据已终核。

**收口边界**：H2a-4必要功能及最终保全的有限证据已补齐，但各环境分别记账。原本机策略最后已知仍revision1／enabled、候选期间新记录未知，待原库只读协调及正常owner收尾；这次云端revision2停用不能代替它。10-20前批只读确认本机Docker不可用，未启动／排查／改旧卷。后续[Key提示与登录窄保护](DEVICE_V2_CLOUD_BETA_H2_DEPLOYMENT_PLAN.md#c6-后续key-提示与发码生命周期2026-10-20)源码128项回归和[匹配beta只读差异](DEVICE_V2_CLOUD_BETA_H2_DEPLOYMENT_PLAN.md#c6-匹配-beta-安装与只读-key-提示短验2026-10-20)均已限定完成；新beta保留，原账号／请求／凭据保持，Key单次403及专用提示、不误报owner和无Key／认证重试通过，未重跑H2。独立通知注册请求完成未知与旧现场倒计时根因分别保留，不为此重发验证码或重复点击Key。不扩H2b／离线历史／Bulk、新网关、R4／Wio或长稳，不提交推送，也不把本批有限验收当作独立同事签收或全面生产发布。

## 9 采样、上传与历史保存周期

当前60秒是Broker H2a的**固定归档策略**，不是设备所有消息的默认周期；本次只澄清现有实现，不开放自定义归档频率或修改发送合同。

| 层次 | 谁决定／现行行为 |
| --- | --- |
| 设备能力底线 | [`realtime(ms)`](../src/BlinkerV2/api/Interactions.h)声明该字段最短允许采样周期，默认200ms；声明本身不主动发送，无租约不调用该链的`onSample()` |
| 实时页面需求 | [App](../../blinker-app/src/app/device/v2/device-v2.page.ts)请求`max(1000ms,所选字段floor)`；实际频率以设备确认值为准，不把UI请求当设备承诺 |
| 设备采样／上传 | [Cloud聚合](../../aily-blinker-services/services/broker/src/protocol/v2/telemetry.ts)合并页面及归档需求为同一上游租约；[设备runtime](../src/BlinkerV2/runtime/DeviceRuntime.inc)按实际interval执行所选字段回调，同一采样批次编码到一个TelemetryData包 |
| 历史选样／保存 | [Broker策略](../../aily-blinker-services/services/broker/src/periodic-history.ts)固定`intervalMs=60000`并持久保存；首个合法样本可立即保存，之后按设备monotonic至少60秒选一次，不是把所有上行帧都存历史 |

例如温度`realtime(200)`、湿度`realtime(1000)`，H2选两项：只有归档需求时，设备约60秒采两值、合为一包；打开包含两项的实时页面后，同一上游改为每1秒一个双字段采样包，Broker仍约60秒选一笔归档。离页后仅移除页面需求，归档仍开则回60秒；两类需求均移除后，此链不再主动采样，原State／控制／自动化等独立业务不受影响。不是缓存60次测量再合一包，同一批次回调依序执行也不承诺物理传感器严格同时采集。

- 多字段归档在同一事务中按字段分别保存：两字段一次采样对应两条字段记录，App的200条上限不是200个完整多字段时点；相同读数仍保留，重复样本身份去重。
- H2当前最多4个只读数值字段，不是4个定时任务槽；底层Telemetry另受32字段、协商帧长与工作区容量约束，不能无限合包。若归档加入慢字段会拖慢既有实时需求，则归档进入`cadence`暂停；容量超限也单独暂停归档，不挤掉控制／实时。
- 普通[`property.report()`](../src/BlinkerV2/api/Client.inc)走State脏值合并，不走60秒归档节拍。[默认调度器](../src/BlinkerV2/runtime/OutboundScheduler.h)合并窗口／最短发送间隔均250ms，`run()`到期把多个变化字段组成StatePatch。同字段反复report只保留最新值，例如`a.report(1); b.report(2); a.report(3);`可合为`a=3,b=2`，不保留每次中间态；容量不够分批，背压保留最新脏值。有控制结算时可强制刷新，不承诺ACK都等待250ms。
- ACK、事件、Telemetry和State是不同消息语义，不任意混包；Telemetry仍best-effort、无设备历史补传。H1才按State实际变化记历史，H2保留周期测量的同值样本。设备发送、服务器转发、历史保存三种频率不能混称“上传周期”。

### 9.1 退出实时页后的保护合同

实时采样是**有期限的观看需求**，不是一次打开后永久生效的1秒模式。复用原租约链，不由历史页面另发全局停机指令，不影响控制、State或其他独立业务。

| 场景 | 现有保护与结果 |
| --- | --- |
| 正常离页／后台 | App立即标记不可见、停止续租、清实时样本，并尽力发送Close；Android同时监听pause，不只等待onStop。在途Open晚到也要关闭，不重新开始续租 |
| 最后一个观看者退出，归档仍开 | Broker关闭原快速上游流，按剩余归档需求重开约60秒流。新流首样本可立即产生，之后才按慢周期，不要求退出后静默满60秒 |
| 仍有其他观看者 | 只移除退出者的需求，保留其他有效观看者所需频率；不能由一个用户退出误停所有用户 |
| App被杀／断网，未发Close | Broker发现账户MQTT断开时移除该连接的消费者；漏掉断开事件时按消费者租约到期回收，过期消费者不再收到转发 |
| Close丢失或Broker不可达 | 设备独立按本地单调时钟到期；即使Expired通知发送失败，本地仍关闭且到期租约不能采样。传输／会话断开亦清租约 |
| 页面与归档需求都无 | 不再开启此Telemetry采样流；普通report／ACK／任务等不是页面租约的附属流，不被一并关闭 |

当前App默认租期30秒、约10秒续租，Broker上游设备租期也是30秒；Broker消费者清扫实际每5秒，不能描述成每秒。正常离页不等待到期，但异常情况下不承诺瞬时停发：设备侧明确的时间锚点是**最后成功接受Open／Renew后约30秒**。若Cloud同时漏掉App断线与设备Close，两级租约兜底在正常时钟、传输和调度条件下可保守估为约一分钟加清扫／传输延迟，**不是从点击退出起统一30秒的硬SLA**。已在途控制、运行阻塞和调度延迟也不能被隐藏；原租约／发送边界不因本说明改变。

本批只补一项确定性组合回归：两个真实30秒消费者先后自然到期、不发Renew／Close；首个到期仍保留另一1秒需求，最后到期关闭快速流并转60秒归档，停用归档后关闭慢流，迟到Opened／旧Data／后续清扫均不复活。沿原`test-telemetry-archive.js`一次 **32/32**通过，含原页面主动移除降频、慢字段不拖慢实时等用例；测试使用的现有dist与锁定TypeScript转译的实际消费者源码一致。没有改生产协议、增加计时器或刷板；这是软件补证，不冒称本批真机杀进程／丢Close故障测试。App隐藏／后台、在途Open和Close失败、设备本地到期沿用原有限回归证据。
