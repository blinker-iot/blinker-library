# Device V2：名称、附近状态与后续易用性执行计划

> 最新设备源码与交付验证取[核心易用性方案§10.6](DEVICE_V2_CORE_USABILITY_OPTIMIZATION_EXECUTION_PLAN.md#106-当前库背压审阅与单zip隔离编译收口2026-10-12)：最终72个host目标、35项发布回归、四种32位ABI及同一ZIP的七目标隔离安装/编译全部通过，本批有限收口完成；同revision重试、混合lane错误及snapshot/Apply边界有限修正，32位仅增加8 B标量并明确预算，不扩队列/容量。原失败及初次资源拒绝记录保留。379项兼容图U2实物和恢复仍取§10.5，不代替当前407项/N3跨平台运行、生产配置或同事独立签收。N1～N5、定时、实时/H1原限定结果保持；2026-10-13必要源码Git交付取同方案§10.7，不重复BLE循环、不部署或升级SDK、不扩功能。

更新：2026-10-11。**N1～N5各自限定的本机短验收已收口；后续核心稳定与最小内部交付只取§10，不重新派发旧阶段。** N5两设备数值闭环取§9.6，设备整数step源码修正及COM3有限差异取§9.7～§9.8：非法值到设备并被拒绝，真实ACK/State及原控制通过，COM3原固件/授权已恢复。N1/N2取§6，N3取§7，N4取§8。§10顺序1～3已关闭，原单库ZIP七目标取§10.6，README/BLE例程差异及S3/R4 BLE隔离编译取§10.7。**顺序4现改为§10.9的三仓Git源码交付与五插件固定远端提交，接收同事自行编译并实际签收；§10.8本地交接包不再作为交付入口。** 操作取[Git独立验收手册](DEVICE_V2_INDEPENDENT_ACCEPTANCE.md)，不扩大任务、自动化、LAN或网关范围。

> H1服务器标量State历史及真实分享只读/撤权已按[实时与历史方案§9.5～§9.6](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md#96-h1真实分享只读与撤权收口2026-10-11)限定收口，不再派发旧H1部署任务。§10顺序3的公开说明与必要差异已关闭，尚无接收同事本人实录；L1/L2b/L2c原有限结果及COM3恢复保持关闭，普通LAN开关不变。H2、离线存储/Bulk和服务器长期维护不自动启动。

## 1. 目标与基线

用户应能用易懂的设备和功能名称完成控制、定时与最小自动化，看到可信的连接状态。复用现有身份、Manifest、页面、任务和自动化执行链，不重新设计协议或引擎。

最小自动化 M1～M5 已在限定范围完成匹配新版 App、本机 Gateway/schema50、两普通 WiFi 设备的命名、编辑、启用、触发、退出 App 后执行、查询、停用和删除。证据见[产品验收](../../blinker-app/docs/APP_DEVICE_V2_MINIMAL_AUTOMATION_PRODUCT_PLAN.md#8-m5本机新版本验收)及[历史主线 §8.86](DEVICE_V2_NEXT_EXECUTION_PLAN.md#886-第八十六批m5本机新版app与两设备产品验收2026-10-10)。不重新派发 M5，也不把限定验收称为完整自动化产品。

本次修改前的已推送源码基线：

| 仓库 / 分支 | 提交 |
| --- | --- |
| blinker-library / device_v2 | `350f4474b88b8a414028316da478d4b43354e2fe` |
| blinker-app / i3w-dev-v2 | `b26cb91f34cf66a781e88b7a19062cef244f07b0` |
| aily-blinker-services / dev_v2 | `8f0bca23c52e3488f5f698559c782c72dd302fbd` |

源码、现场部署、真机证据和正式发布是四个不同状态。原生插件/外部依赖、正式 profile 和生产准入仍取原交接材料；N1/N2的本机部署见§6，N3只操作§7核对并备份的指定ESP32测试板，不额外刷其他板、清授权或提交推送。无关本地修改保留。

## 2. 队列与边界

| 阶段 | 解决的问题 | 最小交付 | 本阶段不做 |
| --- | --- | --- | --- |
| N1（基本展示收口） | 昵称仍走已停用的 Legacy 保存接口；功能名称在各页面分散、自动化冲突提示不符合设备级限制 | V2 owner 昵称/共享功能名称持久化；统一显示解析；控制/自动化/定时复用；修正文案 | 设备 Manifest/Wire/NVS 改动、新权限系统、页面编辑器重写 |
| N2（有限短验收收口） | 扫描动作被当作附近证据；首页无统一可见期间刷新；一次漏扫抹掉有效证据 | 同一扫描器的前台有界刷新、明确证据有效期和连接阶段、退出/后台回收 | 第二扫描器、每设备保活连接、后台无限扫描、MAC 作为设备身份 |
| N3（限定短验收收口） | BLE BBP 分片的 20 B packet / 4 B header / 30 ms 间隔；MTU 变化不等于协议大包协商 | ESP32：20 B 启动、鉴权后确认双方/平台上限、完整记录边界切换；244 B 与 20 B 各三轮短闭环，故障分支取软件证据 | 直接取消所有等待、未经确认修改当前 frame stride、声称 R4/Wio 已支持大包或长稳已通过 |
| N4（限定短验收收口） | 本机任务入口和可用范围不直观；未确认容量易被误报为0槽 | 既有入口、认证目录真实容量/目标、设备执行/云端管理来源及共用名称；原4槽ESP32的0/4→1/4、失败恢复和显式载入真机通过 | 扩槽/全平台持久任务、新调度器、Sketch interval/timeout、线程/脚本/cron 框架 |
| N5（限定短验收收口） | 最小自动化原先只覆盖有限 source scalar → 一个 bool target | 原A2链增加uint32 Property固定赋值；匹配新版三端及两普通WiFi设备的编辑、50/0真实ACK/State、退出App执行、查询、停用/删除通过；bool/CAS/基线/冷却/unknown保持 | int32/float目标、多条件、多动作、Action、BLE-only/网关自动化、全平台/生产 |

普通 WiFi 的 session HTTP 与 MQTT 已可由维护 profile 选择 plain/TLS；HMAC 身份校验不是 TLS 加密。DIY plain 是明确选项，不需要新造不鉴权登录协议。生产安全增强单独评审；不顺带降低 App HTTPS、Direct 鉴权或已有授权检查。

定时容量不是协议硬编码“四条”：设备默认关闭任务（容量0），维护配置支持1～10，当前有限实物profile为4槽/单目标。N4先显示设备认证目录返回的真实容量与可调度目标，不先扩槽或将普通Manifest可写等同于任务许可。BLE Hello已有frame/reassembly上限协商；N3补的是ATT packet/分片节奏，不重复实现已有frame协商。

## 3. N1：语义、显示和布局分开

| 层 | 权威内容 | 保存位置 | 允许影响 |
| --- | --- | --- | --- |
| Manifest | 稳定 key、kind、type、访问和约束 | 设备/既有能力缓存 | 实际读写与任务/规则准入 |
| 共享显示元数据 | 设备昵称、功能名称、可选布尔状态文本 | Service 逻辑设备记录；App 分账号/服务显示缓存 | 各页面标题/选择项/状态文本 |
| PageLayout | widget、位置、样式、单 widget title | 既有账号页面合同 | 单个控制页面呈现 |

昵称和别名不是身份、权限或新能力。绑定仍为 logicalDeviceId + endpoint key；别名另记录 kind/type，解析时不匹配则回退真实 key。删除/变化的端点不能因别名重新出现；旧别名可留存但忽略。不因整份 fingerprint 改变而丢掉其他未变端点的名称。

### 3.1 最小 Service 合同

`GET /api/v2/devices/:logicalDeviceId/display-metadata` 和 `PUT` 同一路径。复用现有用户身份及 metadata ACL：owner/shared 可读，**只有 active owner 可写**；服务层判断和存储事务中的 owner/tenant/state 条件均需成立。无权限读取不泄露设备记录。

GET/PUT 成功响应：`{status:200,data:{logicalDeviceId,revision,name,capabilities}}`。PUT 请求：

```json
{
  "expectedRevision": 0,
  "name": "客厅灯",
  "capabilities": [
    {"key":"power","kind":0,"type":0,"displayName":"电源开关","trueLabel":"开启","falseLabel":"关闭"}
  ]
}
```

- 迁移 051：在既有 `v2_logical_devices` 增独立 `display_revision`（默认 0）和 `display_capabilities`（默认 `[]`）；`name` 仍为原权威列，不增另一份昵称。
- revision 为 0～2³¹−1；PUT 整份显示元数据 CAS，成功递增。并发失配返回 409，App 提示刷新，不自动覆盖或以新请求偷偷重试。
- name：trim、合法 Unicode、无控制字符、最多 128 UTF-16 单元且 256 UTF-8 B。功能/布尔标签最多 80 UTF-16 单元且 320 UTF-8 B；空输入在 App 表示移除可选别名，线上字段不接受空标签。
- capabilities 最多 256 项、key 唯一且 1～64 UTF-8 B、kind 0～2、type 0～9；trueLabel/falseLabel 仅 type=0。禁止未知字段与非法编号。
- 整体 PUT 正文最多 256 KiB；仅本接口显式使用此限额，其他 HTTP 接口保持原限额。新建设备/认领入口复用名称字符校验，BLE 认领仍保留原更严格的 128 UTF-8 B 上限；正常请求的原创建摘要/幂等键不变，不批量改写历史名称。
- 列表和共享列表同时投影 `displayMetadata`，与 `name` 同一记录生成。GET 默认为当前 name、revision0、空数组。纯 BLE/暂时离线设备不需要设备在线才能改显示名称；客户端仅从已有真实能力生成编辑项。
- 不修改 Manifest、页面 revision、自动化 definitionRevision/digest、任务定义、凭据和 Broker 登录。不得为昵称触发设备重注册或清授权。

### 3.2 App 实施

1. 类型化 HTTP/client 与纯显示 resolver，校验响应 ID、schema/边界和账号 epoch。所有异步回写必须仍属于原服务/账号，切账号后的响应不得回填。
2. 用 V2 保存取代昵称的 Legacy config 请求；服务 name 优先，旧本地 customName 不得覆盖远端更新。目录显示缓存仍按 authority/account/logicalID 隔离，显示缓存不是 ACL/在线证据。
3. 设备设置内提供 owner 的“功能名称”小表单，复用缓存 Manifest；尚未获取能力时提示先打开控制页。昵称弹窗和别名表单均冻结打开/加载时的 revision，冲突后由用户刷新，不在确认时读取新版本再静默覆盖。保留隐藏/未知旧别名不扩大权限。
4. 控制页、自动化选择/列表/详情、定时列表/编辑使用同一解析器，默认 key、开启/关闭。有效 widget.title 可独立覆盖；新默认布局不写 raw-key title，旧 `title===endpointKey` 作为默认占位解析，其他标题保留。
5. 自动化冲突文案已改为“目标设备已有启用的自动化，请先停用相关规则或选择其他设备。”当前服务限制是 target.deviceId，不是 target.key；本批不改此准入政策。

### 3.3 N1 验收

软件门禁覆盖 owner/shared/revoked、CAS 竞争、Unicode/长度/重复 key、非法 boolean 标签、纯 BLE/offline 显示、服务/账号隔离、缓存重载、旧目录兼容、默认与 widget override、kind/type 变化回退。验证只改名称不变能力 fingerprint/规则定义/任务目标，名称在控制/自动化/定时一致。Service 迁移/仓库使用隔离 PostgreSQL 实测；若环境不可用，明确待测，不能把 skip 当通过。

后续真机需部署匹配 Service 051 与新 App，真实设置改名后分别查看三处、重启 App/离线显示、共享只读和冲突提示。本批软件测试不代替上述真机证据。

## 4. N2：附近证据不等于扫描动作

沿用已验证 locator/presence tag 与 logical identity 匹配。MAC、广播名、正在扫描/连接都不是“此逻辑设备在附近”的证明。

- 绿色：已建立可用 Direct 会话；蓝色：在有效窗口内匹配到该设备广播；灰色：无有效证据。扫描/连接/重试是独立忙状态，不凭动作显示蓝色；Cloud 仍独立保留当前已知状态。
- 首页/设备列表可见期间由一个生命周期入口请求有界 presence 刷新：2.5秒窗口，正常启动至少间隔10秒；现有 scan coordinator / budget 负责排他、冷却和连接优先级，native拒绝的冷却从拒绝时起完整等待。不为每张卡片建 scanner 或 BLE 连接。
- 有效匹配沿用15秒TTL；一次未匹配保留尚未过期证据。预算拒绝、连接占用、取消不清除或续期证据；断连/取消连接也不能制造新附近证据。过期回到无证据状态。明确权限撤销或radio关闭清除本地附近显示，但不改变Cloud在线事实，不宣称物理设备离线。
- 路由离开、App 后台、退出账号、权限丢失停止需求并取消本批扫描/定时器；恢复前台重新评估。不承诺后台持续发现或旁路 Android/iOS 生命周期限制。
- 优先复用 native/transport 已有结果回调；若只能批量返回，先准确状态与生命周期，不为缩短一次扫描额外引入长期常驻扫描器。

验收：确定性 fake-time 测试 TTL、单次漏扫、busy/denied/cancel、leave/background/logout、账号切换、重复 lifecycle；回归现有匹配和 dedupe。真机另验首页附近蓝色、进入/控制绿色、退出释放、离开范围过期及 Cloud fallback；不能以模拟扫描结果冒称真机。

## 5. 本批执行与交接

N1/N2批次执行顺序：本文件与旧主线入口更新 → N1 Service/App/文案 → N1 软件门禁 → N2 → 联合回归 → 记录结果与真实待测项。该批设备源码和公开 `Blinker.begin/run` 不需要变更；N3设备内部修改与门禁另取§7，公开API仍不变。

### 5.1 已完成的软件实施

以下为部署前的软件批次快照，不代表当前现场状态；本次实际部署与验收另记§6。

| 项目 | 本批结果 | 证据边界 |
| --- | --- | --- |
| N1 Service | 独立显示元数据API、迁移051、owner写/shared读、独立CAS、目录投影及创建名称校验 | 隔离PostgreSQL16实测；业务库未迁移 |
| N1 App | 昵称、功能名称与布尔文本统一解析；控制/自动化/定时复用；显式widget标题保留；冲突文案修正 | 类型化HTTP、缓存、Angular/Ionic DOM单元测试，不是真手机 |
| N2 App | 独立附近证据、单一前台刷新、TTL、生命周期和账户隔离；只有已验证广播可续期 | 模拟时间/扫描/适配器测试；不声称真机延迟已改善 |
| 联合门禁 | App 39测试文件/489项全部通过，0跳过；spec TypeScript与development Web构建通过；Gateway TypeScript通过 | 2026-10-10冻结源码上的最终联合运行；中间重叠测试数不累加 |
| Service回归 | 上述8文件/13项全部通过，0跳过；另行HTTP正文/自动化合同、判定、管理和worker回归117项通过，0跳过 | 13项中含真实PG仓储/HTTP测试；117项不是新增真机执行次数 |

实施中一并修正：昵称弹窗冻结原revision，避免确认时读取新revision后覆盖他人编辑；定时标签核对任务自身valueType，避免同key类型变化误用别名；显示PUT单独传256KiB正文限额，避免沿用4KiB默认值；单调revision合并后的目录缓存落盘，避免重启退回旧显示名称。显示更新不改授权用`updated_at`、创建摘要或自动化/任务定义。

清理了列表与组件重复发起附近扫描、昵称Legacy保存路径和新默认布局的raw-key标题重复。图标、位置及旧页面编辑器配置仍有消费者，本批保留，不能宣称所有Legacy配置接口已经移除。未新建scanner、原生插件、协议、设备Flash表或执行器；设备源码不变。历史M5证据保留，仅修正当前队列与不准确声明。

临时隔离测试容器已停止并自动删除，确认不在容器列表中。未部署App/Service、迁移业务库、刷板、清授权或暂存/提交/推送；无关本地修改保留。

### 5.2 可复现的软件门禁

以下在App根目录运行（Node需已在PATH）；只运行源码测试/构建，不安装手机包：

```powershell
node node_modules/@angular/cli/bin/ng.js test --configuration=ci --include=src/app/core/device-v2/display-metadata.spec.ts --include=src/app/core/services/device-v2-management.service.spec.ts --include=src/app/core/device-v2/device-catalog-cache.spec.ts --include=src/app/core/services/data.service.spec.ts --include=src/app/sections/device/device-settings/device-settings.spec.ts --include=src/app/device/v2/device-v2.page.spec.ts --include=src/app/device/v2/device-v2.render.spec.ts --include=src/app/sections/scene/automation/automation.spec.ts --include=src/app/sections/scene/automation/automation.dom.spec.ts --include=src/app/sections/device/device-timer/timer.service.spec.ts --include=src/app/sections/device/device-timer/device-timer.pages.spec.ts --include=src/app/core/device-v2/page-layout.spec.ts --include=src/app/core/device-v2/ble-direct/*.spec.ts --include=src/app/core/services/device-v2-ble*.spec.ts --include=src/app/home/components/deviceblock/deviceblock.spec.ts --include=src/app/home/components/tab-device/tab-device.presence.spec.ts --include=src/app/home/components/deviceblock-list/deviceblock-list.spec.ts --include=src/app/core/device-v2/device-ui*.spec.ts --include=src/app/core/bluetooth/scan*.spec.ts --include=src/app/core/services/app-visibility.service.spec.ts
node node_modules/typescript/bin/tsc --project tsconfig.spec.json --noEmit
node node_modules/@angular/cli/bin/ng.js build --configuration=development
```

以下在`aily-blinker-services/services/gateway`运行。`TEST_DATABASE_URL`必须预先指向**专用隔离PostgreSQL测试库**，不得使用业务库；helper创建/回收随机测试schema。未配置而跳过不算PG通过：

```powershell
node --test test-device-v2-display-metadata.js test-device-v2-repository.js test-device-v2-access-grants.js test-device-v2-ble-enrollment.js test-device-v2-page-layout.js test-device-v2-sharing-http.js test-device-v2-sharing-e2e.js test-gateway-wiring.js
node --test test-http-body.js test-device-v2-automation-runtime.js test-device-v2-automation-contract.js test-device-v2-automation-a2-contract.js test-device-v2-automation-typed-admission.js test-device-v2-automation-typed-clients.js test-device-v2-automation-version-policy.js test-device-v2-automation-worker.js test-device-v2-automation-execution-worker.js test-device-v2-automation-management.js
node ../../node_modules/typescript/bin/tsc -p tsconfig.json --noEmit
```

### 5.3 下一批：匹配版本的本机短验收

N1/N2不能用旧M5 APK或旧Service050实物结果关闭。按以下顺序补证，不开启100轮或复杂自动化：

1. 核对实际测试网络、手机、账户与设备身份；备份受影响App状态/本机数据库及当前服务来源。构建匹配新版App/Gateway，按现有本机部署流程应用迁移051；不升级无关Auth/Broker或刷其他板。
2. 用真实设置页改设备昵称、一个功能名称与布尔状态文本；分别查看控制、自动化、定时，并重启App核对目录缓存。现有任务/规则端点和定义不变；没有当前可用任务的设备不为展示伪造任务。
3. 验证显式控件标题优先、类型不匹配回退；在可用账号环境下补共享只读及两个编辑会话的CAS冲突提示。离线验收只查已保存显示，不承诺离线编辑队列。
4. 核对BLE首页有效发现蓝色、可用Direct绿色、退出控制页释放本地连接；后台/返回、权限或radio变化、离开范围TTL过期及Cloud图标回退。保留实际样本与失败，不把正在扫描当附近。
5. 完成后分别记录N1/N2真机通过或剩余缺口，再选择N3或N4的一个有限批次；N5只保留评审入口。首次连接性能没有新真机数据前，不承诺秒开或大包收益。

## 6. 本机匹配版本短验收（2026-10-10）

### 6.1 备份与部署

保留原账户、设备绑定及业务数据。原App APK与停止进程后的完整run-as数据归档已备份，归档目录可读；Gateway PostgreSQL完整dump已备份并在隔离PostgreSQL16中恢复验证。恢复副本从schema50迁到051后，原38张V2表行摘要一致，47台设备新增显示字段均为revision0/空数组。仅更新本机Gateway并应用051；Auth、Broker及其他容器保持原实例/配置，没有恢复业务数据库或降级历史记录。

首次匹配App在独立目录由当前必要源码闭包重建；包名、HTTPS WebView origin、公开测试CA、debug签名、local/local-device profile与原包一致，普通LAN及实验BLE优先开关保持关闭。`install -r`不清数据，手机已安装APK的SHA-256与候选一致：`4ceb45621d15bbf45f6bc2dd937e4896cb17d7ad463f613154e9ac17cdd9aad1`。Gateway镜像为`c77c5de7fbcab53b277a50fde845bc6eb1cacaf816b610f9bcbdfd6a6396c01e`，健康检查与数据库状态正常。

构建边界：冷Docker构建因better-sqlite3预编译下载/缺少node-gyp工具链失败，不能写成全新无缓存构建通过。本机候选复用已实测M5镜像的原Linux依赖；核对依赖声明/锁不变、冻结当前源码与编译产物差异后生成051镜像。独立交接的冷构建保障仍不在本次功能短验收中关闭。

备份、账户材料、网络参数、APK、dump和原始UI/扫描证据仅在被忽略的`build/n1n2-usability-live-20261010`及本机维护目录保存，不作为源码/文档交付内容。

### 6.2 已通过与未覆盖

| 验收项 | 实际结果 | 不得扩大为 |
| --- | --- | --- |
| 真实设置页保存显示元数据 | 既有WiFi+BLE测试设备临时昵称“桌面演示设备”；power显示“电源开关”，布尔文本“已开启/已关闭” | 新身份、新Manifest、权限变化或新规则 |
| 控制及自动化显示 | 控制页真实切换开启/关闭；既有停用规则列表与新建编辑器均解析昵称/目标别名/布尔文案；新建草稿丢弃，没有保存或启用规则 | 重复声称M5执行验收，或BLE自动化已开放 |
| 重启与离线显示缓存 | 重启进程后首页昵称保留；临时移除本机服务ADB通道后重启，昵称/功能别名仍显示，既有授权BLE会话可就绪；随后恢复通道 | 无缓存首次离线接入、离线保存显示元数据；PageLayout请求失败仍显示重试提示 |
| 定时页 | 同一设备昵称正确；未取得可用认证目录且未开放任务管理试点，未创建任务；软件解析回归仍有效 | 设备真实容量为0、定时目标别名/现有任务执行已完成真机验收 |
| 首页附近可信性 | 仅匹配到当前逻辑身份的广播才显示蓝牙在附近；其他正在扫描的候选仍为未知/查找中，17张卡片未重复目标身份 | MAC/名字决定身份，扫描动作直接变蓝，所有平台无重复 |
| Direct与退出释放 | 真实Direct会话ready、控制权限有效；退出控制页后links/sessions归零，重新发现来自新广播 | 已优化首次连接延迟或大包协商 |
| 离开列表后TTL | 离开Home沿用原deadline，不续期；约14.7秒后附近证据到期，期间原生扫描预算记录不变 | 物理移远测试或Cloud在线测试；本轮是停止观察后的自然证据到期 |
| 真实后台/恢复 | Android Home后台12秒，同一App进程、扫描启动预算记录不变；恢复前台后需求重新建立并匹配新广播 | 已收到原生stopScan回执，后台无限发现，100轮/长稳门禁 |

共享只读、两个真实编辑会话的CAS冲突、显式widget标题覆盖、端点类型变化回退仍仅有软件门禁。本轮不伪造共享账户、任务或能力来补真机数字。上述基本短验收通过不等于N1全部扩展验收或N2全平台发布通过。

### 6.3 现场显示缺口与当前下一步

已只读核对：Direct实际ready、`canControl=true`、`syncing=false`，子组件与外层getter均返回“蓝牙已连接”，但真实标题DOM仍显示“正在连接并同步”，用户交互后才刷新；不是Direct等待Cloud。另放弃新建自动化草稿的确认完成后，页面同样可能等待下一次交互才刷新。

修正仅为异步完成后的Angular视图通知：嵌入控制页`requestRender`补`markForCheck`通知祖先；自动化页面将视图切换统一为内部`setView`并通知框架。不新增父子协议，不改`stateLabel`判定、连接、鉴权、规则定义或执行链。

最终联合门禁为**40测试文件/491项全通过，0失败/跳过**，spec TypeScript通过；隔离模板、类型、同profile Web及Android构建通过。另在隔离工作副本移除两处通知，两个DOM用例恰好在最终显示断言失败（标题仍同步中、确认后编辑框仍存在）；逐字节恢复冻结源码后focused11项通过，不累加到491中。

最终修订App SHA-256：`068cf2e636e901b56af6987b42cbdbf36a10eecc85ee77fb87f285aa517fb4b0`，79,939,254 B；原签名/公开CA/HTTPS origin/profile不变，每个编译JS与APK资产哈希一致。安装前再次停止进程备份测试后的APK/数据，随后`install -r`及手机读回哈希匹配。真实页面进入后仅被动等待，不再点击/强制detectChanges；runtime ready与真实DOM均为“蓝牙已连接”。修订包控制再次切换已开启→已关闭；放弃新建草稿只确认一次即返回列表，无第二次点击、无规则保存。

修订包退出控制后会话释放；首页新广播继续匹配附近；再次真实后台12秒的原生扫描启动预算不变、同一进程保留，返回恢复。被动采样中的约500ms只是已就绪后的框架观察等待，**不是从进入页面到BLE连接的性能数据**，不能据此宣称首次连接已降到0.5秒。

### 6.4 业务保全与收尾

临时昵称与别名已通过真实设置页恢复原内容：原设备名恢复、capabilities为空，显示revision由0递增到4，不回滚CAS历史。开关恢复关闭。最终只读审计：47台设备所有非显示字段及凭据、设备`updated_at`不变；5条规则的定义/摘要/启停不变，10条task scope、131条request及原confirmed roots不变；12条执行历史保留，enabled/unresolved均为0。

tenants整行并非零变化：仅`last_active_at`和`updated_at`随正常App活动更新。已从原dump在隔离副本逐字段比对，其余全部字段（包括owner、status、credential_version、profile、plan）一致；不先排除未知差异来制造通过。schema051/健康检查正常，24个非Gateway容器的原实例/镜像/启动时间不变；所有运行ENV语义、端口、挂载与网络配置保持。

未刷板、清授权、恢复业务DB、升级Auth/Broker、暂存/提交/推送或扩展新功能；无关本地修改保留。备份和详细证据留本机忽略目录。已确认前台扫描需求及Direct会话释放，再停止App并移除本批ADB调试通道；未退出账户或清数据。后续联调时需重新建立`adb reverse tcp:17000 tcp:17000`及`adb reverse tcp:18885 tcp:18885`，不能将此测试包称为独立网络交付。

### 6.5 后续执行边界

本次请求的N1基本展示与N2本机短门禁已收口，M5执行链不重开。定时目标别名、真实共享只读/并发CAS、显式widget覆盖/类型回退、真实移远/权限或radio变化的扩展真机验收仍按§6.2保留；无匹配环境时不伪造任务、账户或设备能力凑通过。

用户已确认的N3 ESP32 ATT packet协商与失败回退已按§7完成限定短验收；§8.4补齐N4匹配App有限现场展示验收与恢复。用户随后明确启动N5，当前范围取§9，不重复派发N1～N4。不启动扩槽、复杂自动化、原生扫描框架、冷构建整组维护或100轮测试；未覆盖的发布门禁不混入功能完成声明。

## 7. N3 ESP32 BLE 包长协商

### 7.1 协商与兼容边界

复用现有4 B BLE分片头、DirectSecureRecord和Hello；BBP帧大小/重组大小的原协商不变。Hello新增可选CBOR key12，值为20～244 B的ATT characteristic value上限，包含4 B分片头，不含3 B ATT开销。key10/11仍为已撤回WidgetCatalog的保留值，不复用。新会话没有key12就是原20 B链路；已协商会话中普通Hello省略该字段不会隐式撤销包长，退回20需显式确认20或重新建立会话。不会通过广播、MAC、MTU变化或仅有原生配对状态开启大包。本批不实施链路层DLE或2M PHY，MTU优选值也不是物理吞吐保证。

启动、Noise配对、Method2鉴权及Hello收发均用20 B。App连接后读取当前原生MTU，取`min(MTU−3, 244)`，此时不提高发送上限；读取失败/超时或上限为20时直接保留20。上限大于20时，在Method2通过后通过已建立的DirectSecure会话发送带key12的Hello。设备只在该会话已通过应用层鉴权时接受，取App声明、原生MTU、Port和scratch上限的最小值，并用20 B回复确认。App在发出请求前准备有界接收能力，收到确认后才提高发送上限，避免确认回复后的首个大包与异步验签竞争。旧设备忽略key12，回复不带字段，仍为20 B。

协商候选大于20时，初始Hello暂不声明App授时响应，鉴权后的Hello再打开原授时能力；20 B路径保留原行为。这样复用原Hello响应与现有授时处理，无需另建接收demux或缓存未匹配的握手消息。

首批仅ESP32 peripheral Port增加244 B物理上限及ATT MTU247优选值；实际值仍取SDK当前连接事实。R4/Wio仍20 B，网关central/permit-join relay不扩包，不增加新的广播版本或插件。`Blinker.begin/run`与组件API不变，服务器无需部署修改。

### 7.2 帧边界与失败处理

发送记录在入队时冻结分片上限；鉴权前已排队的AuthResult不能被后续协商放大。接收端从START片取得该记录实际stride，后续非末片必须一致，因此对端下一记录退回20 B不会使旧stride污染新记录。新的frameId可从START替换未完成记录，但相同ID、乱序和越界仍拒绝。

设备收到明确且未消费包的长度拒绝时，保留同一密文记录，换新frameId按20 B从START重分片并锁定本会话20 B；不重新seal、不推进nonce、不重执行业务。`WouldBlock`只表示背压，不作为降级依据。App原生写入失败不一定证明对端未接收，因此关闭会话，并在后续重新鉴权时用20 B；不在未知结果下自动重发Property/Action。

协商失败、断开或重连都不复用上一会话大包状态。App初始20 B间隔仍为30 ms，确认大包后为10 ms，保留串行GATT写入；不同时扩大队列、引入并发写入或取消所有等待。原生MTU读取最多等待250 ms，失败继续原20 B鉴权路径，不阻塞共享GATT队列。未知写入失败后的20 B提示仅在进程内保留5分钟、最多16个carrier条目，用于下一次新鉴权连接，不保存秘密、不持久化、不自动重发业务。

### 7.3 门禁与现场范围

软件门禁覆盖：旧Hello逐字节兼容、key12边界/无效字段、鉴权前不能升级、双端最小上限、入队帧长度冻结、记录中MTU下降、明确长度拒绝后的原密文20 B重发、背压、断开清理和App未知结果不重试。编译复用ESP32 IDF NimBLE，保留R4/Wio原20 B边界。

软件门禁通过后，先备份当前App数据/APK与识别过的ESP32测试板Flash，再部署匹配候选。短验收核对实际MTU与确认包长、Manifest/State及控制、退出释放、重连、20 B对照和失败回退；对同一设备/账号/路径记录耗时及分片数。不进行100次循环、不清授权、不替换Auth/Broker、不改自动化规则，测试后恢复原固件及临时调试通道。软件通过、真机通过和性能收益分别写明。

### 7.4 当前软件与资源结果（2026-10-10）

冻结源码的新目录全量设备host门禁71/71通过；另在clean-first重建下，DeviceV2、DirectSecure、ESP32 NimBLE、BLE enrollment与BLE mode五目标的ASan门禁通过。App联合门禁为45文件/561项通过、0跳过，spec类型检查及隔离源码闭包的Web/Android构建通过。复用现有IDF NimBLE和原生BLE插件，未新增依赖、公开API、UI或Service功能。

实测Xtensa 32位ABI：Small/Standard/Large的BleFrameTransport为320/516/876 B，BleRecordTransport为156/236/364 B，修改前后相同；两种config仍为16/12 B。通过紧凑的每会话包长/flag保存状态，没有为通用runtime或R4/Wio增加大包队列。ESP32 Port专属RX队列增加896 B、共享packet scratch增加224 B，即静态缓冲净增加1120 B；callback/poll局部packet缓冲各增加224 B，不能把它们误算为通用对象ABI或已测动态heap峰值。

已完成当前手机APK/停止进程数据备份，以及指定ESP32完整8 MB Flash的fresh读出、ROM-held可变区域补读和全范围verify。新版App普通更新、不清数据，安装读回SHA-256为`48cce9b49385c3eea5e8844325bbeabe59589d0b938bb14271a4fb79ee936d22`，保留原HTTPS origin、公开CA、签名及local/local-device配置。匹配候选固件仅将备份核对过的application分区作为刷写目标，不清授权或刷其他板；不据此宣称设备正常运行产生的NVS写入为零。

旧固件兼容真机已通过：原生MTU读取后提供244 B包长候选，但旧Hello回复缺省key12，新App保持20 B发送；能力/State同步、真实UI开/关及退出后links/sessions归零均通过。此时RX预备上限244并不表示对端已发送大包，实测notify仍≤20 B。新版固件与20 B对照的结果取§7.5，不将软件故障注入当作真机故障注入。备份、网络/账号材料及构建产物仅保存在忽略的本机目录，不进入交付文档或Git。

### 7.5 ESP32 真机短验收与恢复（2026-10-10）

只操作事先核对并完整备份的COM11 XIAO ESP32S3；COM3及UNO R4见证板不动。新版App、原固件兼容、244 B候选与明确20 B物理上限对照均使用同一手机、账号和逻辑设备，不重新认领或清授权。固件来自同一冻结正式ZIP、安装源码、profile与Sketch；20 B对照仅增加`BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE=20`。所有13个本次设备源码文件均核对工作区、ZIP、安装副本逐字节一致。

SDK3.3.11的安装库编译、链接及产物来源检查通过。一次冷编译中途因电脑休眠/执行超时中断，该运行不计成功；随后在同一冻结源码与安装目录恢复编译并校验输出，不宣称无中断的全新冷编译通过。244 B/20 B两种编译的程序占用分别1,283,592/1,283,448 B，静态RAM分别56,652/55,532 B；RAM差额正好1120 B，不代表实测栈/heap峰值。

每组从真实设备卡片进入，等到Direct可控制，点击开关开/关，执行既有能力/状态刷新，再从页面退出；每组3轮，退出均为0连接/0会话、开关最终关闭。244 B组先执行，20 B组后执行，不是随机交错对照或统计性能承诺。

| 样本项目 | 新固件20 B对照 | 新固件244 B确认 |
| --- | --- | --- |
| 三轮进入至页面可控制 | 2626 / 3329 / 3111 ms | 2459 / 2632 / 2517 ms |
| 六次开关权威回报 | 265～312 ms | 82～121 ms |
| 三次完整能力/状态诊断刷新 | 942 / 979 / 1268 ms | 252 / 253 / 253 ms |
| 每次诊断刷新实际接收notify片数 | 16 | 3 |
| 每次诊断刷新发送片数（由记录长度计算） | 11 | 3 |
| 样本中实际最大notify | 20 B | 141 B |

能力/状态刷新调用既有已鉴权`session.refresh()`，使本会话缓存提示及已验证能力/状态缓存失效后，真实执行ManifestRequest → ManifestAccept → State重读；不修改账号、凭据或页面设置。三条TX记录长度均为37/72/37 B，三条RX均为137/37/49 B。记录大小和操作结构相同，不声称不同nonce/状态revision下密文逐字节相同。该诊断刷新不是未知设备首次进入时间；表中页面就绪是独立真实进入样本。收益来自减少分片与有界节奏调整，BBP正文并未进一步压缩。244 B是双方确认的packet上限，最大实际包141 B只证明已使用超过20 B的包，不证明满244 B载荷、链路层DLE/PHY或持续吞吐。

覆盖与保留边界：旧peer缺省key12真实退回20 B、新20 B上限真实不扩大、新244 B能力/控制与三次短重连均通过。明确未消费的`CapacityExceeded`原密文重分片、MTU中途下降、未知原生写入错误关闭且下次20 B等路径有确定性host/App软件门禁；本轮没有在硬件上人为触发这些SDK错误。长稳、全SDK、R4/Wio大包、动态内存峰值和异常压力仍未验，不启动100轮来替代这些边界。首次进入仍约2.5秒，不能称秒开。

测试后仅将COM11完整application分区从本次fresh校验备份写回并读回verify；NVS及其他分区没有作为写入目标。原固件恢复后同账号Method2复连、真实开/关和退出释放均通过，无需重新认领，最终开关关闭。新版App保留，观察器已卸除，App进程停止，仅移除本批ADB调试通道；未退出账号或清数据。当前网络变更不触发BLE测试板配网，不宣称该网络下MQTT已复测。

N3限定本机短验收至此收口。该批Service051及Auth/Broker保持，未部署服务器、扩展UI/API、刷其他板、清授权或暂存/提交/推送，无关修改保留。当时下一批N4已由§8.4补证，不重复派发；Sketch interval/timeout另行评审，不属于N4，不立即扩持久任务槽、复杂自动化或网关功能。

## 8. N4 既有定时功能展示

### 8.1 容量和来源

本批只改既有App入口和呈现，不修改TaskEngine、Flash存储、任务合同、签发权限或执行链。设备默认任务关闭不等于App查得0槽：当前认证任务目录的合法capacity是1～10，未准入、离线、查询失败或只有历史确认时显示“容量尚未确认”。§6.2原“真实目录返回0槽”已纠正，不能把App初始值当实物结果。

| 信息 | 来源 | 展示规则 |
| --- | --- | --- |
| 总容量、许可目标 | 本次fresh认证TaskCatalog | 不取固定4槽、普通Manifest或缓存；0个许可目标合法但不能新建 |
| 已用数量、当前任务 | 本次完整fresh任务表 | 两项查询成功才显示已用/总量；失败、离开或换账号清除当前证据，不显示假空表 |
| 执行与管理 | 既有本机任务产品 | 设备本地执行，当前通过云端查询/管理，需要设备云端连接；退出App不取消已安装任务 |
| 昵称、目标及布尔文案 | N1共用显示解析 | 同key/kind/type匹配才用别名，不增加目标或权限，类型变化回退原key |
| 日历授时依赖 | 既有once/weekly合同 | 提醒需要有效设备UTC且错过不补跑；当前响应不提供授时就绪或UTC来源，不能制造状态 |

### 8.2 入口与实施顺序

复用设备设置→定时任务→现有列表/编辑器路由。一个纯入口判定同时供菜单和页面使用：预览、分享、停用、纯BLE、网关/子设备及非V2设备展示当前范围限制；普通候选可进入查询，但入口不预判服务白名单、硬件能力或容量。服务暂不可用、未开放试点和云端连接不可用分别提示，不能一律当“设备不支持”。

列表与编辑器共用真实容量、来源和创建限制。容量满时禁用新建，仍可处理既有任务；无目标时解释固件未许可目标，不从Manifest补目标。空表只在认证完整读取后显示；内部修订号作为查询详情，不占主信息。昵称、功能名称和布尔文案复用原解析器，不新增任务name字段或重复的设备显示表。

先补状态与纯展示模型，再更新入口/列表/编辑器，最后联合回归。保留原请求日志、CAS、启用≠Start、重启中止、未知结果不自动重发及明确恢复/停止追踪确认。只读进入不能Initialize或Start；任何名称改动不修改任务定义。编辑器首次查询失败后，重新查询成功应允许显式载入原任务，不能因表单尚未载入误报任务已删除；查询刷新不自动覆盖未保存编辑及冻结的CAS修订。清理重复入口判定及不再准确的默认容量/固定范围文案，不删除仍有消费者的任务恢复模块。

软件门禁覆盖1/4/10容量、未确认与非法0、无目标/满额、刷新失败/账号切换/页面离开清理、旧类型别名回退、真实模板名称/来源呈现与原管理链路。通过后另部署匹配App做有限现场展示验收；软件结果不冒充新真机任务安装/执行。现有服务及设备无合同缺口时不重部署或刷板；不顺带扩槽、复杂自动化、网关、Sketch调度器或SDK维护。

### 8.3 N4 实施与验证（2026-10-10）

App的设备设置和任务服务复用`taskEntryState`，取消两套入口判断；状态层以`capacity=null`表示未知，读取/管理请求开始、失败、离开及换账号清除当前表证据，不删除持久请求日志。列表与编辑器复用`TaskStatus`容量/来源摘要、共用名称解析和`canCreate`限制；满额不妨碍已有任务管理，无许可目标不从普通Manifest补权限。修订号下沉到操作说明，移除误导的默认0槽、失败后空表和无目标数值输入。

补齐编辑器恢复：任务是否存在以新鲜任务表判断，表单是否载入是独立视图状态。首次查询失败后，成功重查允许用户显式载入；未载入不开放保存/删除。后续刷新不自动覆盖未保存字段或CAS修订，原修订过期仍由既有服务拒绝；真实缺失任务和非法旧地址仍阻止编辑。没有新增Effect、自动Initialize/Start、HTTP接口或第二套任务缓存。

专项软件门禁5文件/100项通过，包括服务状态29、纯模型11、页面行为17、真实Angular模板DOM12及设置入口31；这些项目互不累加为真机结果。DOM验证包含实际服务信号异步更新，以及查询恢复后的原生按钮点击自然更新模板；按钮点击后不额外强制视图刷新。原请求精确恢复、暂停追踪确认、账号代际、CAS、启用不等于Start等行为保留。

最终冻结源码的联合门禁**49测试文件/637项通过，0失败/跳过**，包含上述100项及已有任务client/journal、N1名称、N2附近/回收和N3 Direct包长回归；不是100＋637。spec TypeScript、设备页面模板的`tsc --noEmit`/Vite构建及新输出目录的App开发Web构建均通过。未构建或安装N4 Android包，未运行新的MCU任务或服务器集成门禁，不将旧N3设备71项或旧实物累加为本批结果。构建日志保存在本机忽略目录；补齐子模板`node_modules`忽略规则，未修改依赖清单或锁文件。

本软件批次不部署App/Service、不刷板、不清授权、不改变槽位、协议、执行器或存储；N4现场展示验收另取§8.4，不与本节软件结果混记。原执行边界是优先复用现有已准入候选；无候选时保留待验，不自行扩大白名单或刷见证板。

### 8.4 N4 本机有限任务展示验收（2026-10-10，收口）

现场原任务白名单仅指向已删除、已退休的历史试点，不能用它证明当前容量。用户已明确允许使用 **COM3 专用 XIAO ESP32S3**：先完整备份，临时部署原4槽/单目标任务候选，经真实App创建测试设备，只准入该逻辑设备；结束后恢复原固件与授权。其他板不动，不扩槽，不重开M5或N5。

安装前备份原APK和停止进程后的完整App数据归档。匹配N1～N4的新App在隔离目录重建，包名、签名、公开CA与HTTPS origin保持原值，使用`install -r`；安装后首次启动前的归档与安装前**全部成员哈希一致**。真实账号恢复，原3条任务意图记录另建立只读哈希基线；不退出账号、不清数据、不停止追踪原请求。账号、Key、网络参数、Flash/dump、构建产物和原始证据只存被忽略的本机目录。

真实App创建本批临时手动WiFi设备后，先验证未准入/未上线时显示“容量尚未确认”和对应限制，而不是0槽或假空表。本机Service保持051和原镜像；经数据库归档备份、原设备/规则/任务审计基线后，仅临时追加该ID到Gateway及其实际Broker的任务名单，并调整本批plain网络地址/端口及独立device-only HTTP代理。保留旧名单、原TLS/App origin和旧代理，不升级Auth或迁移数据库。数据库备份的完整目录可读；本批没有另做隔离完整恢复演练，不冒称本次备份已通过灾难恢复门禁。

COM3在官方ROM中完成本次fresh完整8 MiB Flash备份及设备校验，实际app0容量为1,310,720 B；任务存储只读审计为Empty，不需要清原授权或任务。标准Arduino ZIP安装编译和相同4槽私有候选编译均通过，程序985,312 B、静态RAM53,936 B，未改变SDK、分区、公开API或协议。仅写app0并核对NVS在写入时未变。备份阶段RAM stub曾在相同偏移失败，分段也未解决；保留失败证据，最终ROM完整读取通过，不将备份传输问题写成产品SDK修复。

真实App在原生产页面通过DOM操作完成以下短验收，不调用页面内部写方法、伪造HTTP响应或写业务SQL代替用户操作：

| 验收 | 真实结果 |
| --- | --- |
| 昵称/名称 | 设置并保存设备昵称、`power`的“电源开关”及“已开启/已关闭”；重新进入设置读取一致，定时目标和值复用这些名称 |
| 真实容量/来源 | 显式初始化后fresh完整目录/表显示0/4；仅创建一条禁用的60秒倒计时、值为false，fresh读取显示1/4和“已停用 · 等待手动开始”，开始按钮禁用；展示设备本地执行、经云端查询管理及云端连接要求 |
| 列表失败恢复 | 对该临时设备的真实任务请求有界阻断，0/4和1/4两种状态均改为容量未确认、禁止新建而非假空表；解除阻断后fresh查询分别恢复原值，未重装任务 |
| 编辑首次失败 | 首次查询失败后恢复到真实1/4；必须明确点击“载入当前任务信息”才开放原表单，目标/布尔文案正确且仍为禁用，不误报任务已删除 |
| 离开回收 | 编辑→列表→设置释放任务视图状态；5,020 ms观察窗内没有新增任务请求，不把任务页回收等同于全App MQTT断开 |
| 删除/退休 | 在线时真实owner从App移除临时设备；服务设备为deleted、scope为retired，设备退休回执已验证，不用先前active审计冒充退休证据 |

留存18个请求/18个attempt重新核对prepare/challenge绑定、Grant签名及设备结果HMAC，全部verified：Initialize、Create、Retire各1，Catalog Read 7、Table Read 8；没有Start、Enable、Replace或Remove操作，所有已留存任务记录的startSequence/progress均为0且enabled=false。有限读取保留窗中的后期目录是1/4；早期0/4取真实App采集及已验Initialize结果，不把后期目录当早期证据。本批只验证展示/管理恢复，**没有启动倒计时，也不新增一次性/每周任务执行结论**。

结束后COM3从本次fresh备份恢复原NVS、otadata及完整app0，并经官方ROM校验整块8 MiB的设备MD5与备份一致，原固件/授权恢复；这是设备整盘校验，不冒称恢复后又逐字节读取了8 MiB。未erase-all或写分区表，其他板不动。Gateway/Broker只恢复原测试准入和网络配置，镜像/schema051不变；本批device-only代理已删除，旧代理与无关容器保留。未回灌数据库：原47台设备、10个scope、规则/执行/路由及原任务请求/attempt均保全，临时设备墓碑及退休请求正常留存。

原账号标识和3条原任务意图的逐条内容哈希、类型化哈希与整体指纹，在开始、任务验收后、删除后完全一致；没有注销、清数据或停止原请求追踪。验收后再停止App形成完整可读私有数据归档，匹配新版APK保留，移除仅本批ADB调试通道。运行期间归档有151项成员变化，其中23项持久文件变化、无持久文件删除；这是运行后的浏览器/应用文件变化，**不声称活动后整个归档字节不变**，账号与原意图保全另由上述逻辑哈希证明。服务请求日志、原始归档和失败证据完整保留在本机忽略目录。

匹配冻结源码的定时模型/页面/模板/服务专项回归4文件69项再次通过，是§8.3的637项子集，不累计。本现场批次未新增产品源码修改；只更新三端方案进度，保留无关本地修改，没有暂存、提交或推送。N4有限本机验收至此收口；未覆盖满4槽、1/10槽实物、其他MCU、分享任务管理、长稳、生产准入和新的调度API，不能据此称定时功能全面发布。N4结束时N5尚未启动；后续用户确认的实施范围取§9。

## 9. N5 单一整数目标

### 9.1 剩余缺口与本批决定

最小自动化已能命名、编辑、启停、查看结果并在退出App后执行；A2源条件已有布尔双方向和四种数值的阈值/回差。N5实施前，目标在App管理合同、Gateway规则验证、签名能力/执行合同和Broker实时准入中均明确只允许bool，不是单纯缺一个数值输入框。

| 剩余内容 | 本批处理 |
| --- | --- |
| 普通属性数值动作 | 只增加uint32 Property固定值赋值，范围0～4,294,967,295；仍单条件、单目标、不同物理WiFi设备 |
| 整数step准入漏洞 | 复核已确认合法大step下既有浮点容差可错放整数；本批只修Broker原Command约束校验的整数精确余数，不增加另一个自动化校验器 |
| 其他整数/浮点、按钮、延时和组合动作 | 后置；不同时扩充scalar集合、Action、多步骤或新调度器 |
| 分享、跨分片、BLE-only和网关规则 | 维持原准入边界，不以普通Command已支持这些设备推断自动化已支持 |
| 发布profile、旧版本交接及新数值实物 | 独立门禁；软件通过后才备份并部署匹配App/Gateway/Broker，验证真实状态和ACK，不借旧bool实物关闭 |

浮点目标暂不开放：除了Float32精确舍入语义，现有通用step校验的极大值/极小step商需要另补有限性门禁，不能借此次uint32验证宣称浮点边界已关闭。

整数问题有独立实证：合法uint32 Manifest的minimum=0、maximum=4,294,967,295、step=1,000,000,000,001时，值1不匹配step，旧Broker却因商小于浮点容差而放行。App精确余数可拒绝，但不能把App校验当服务授权。因此沿原Broker Command codec用BigInt整数差/余数修正type1/2约束，兼顾安全整数端点之差超出JS精确范围的情况；浮点分支和Wire保持。N5原软件/实物阶段未修改设备库的同类旧算法，§9.6仅临时部署普通Property测试固件并恢复；后续设备通用整数step已由§9.7有限修正。旧固件不因Broker或库源码更新而获得修复，N5原数值动作仍必须经过当时已修正的Broker。

优先选择uint32是一个有限切片，不将设备UnsignedInteger的通用64位Wire范围当成自动化支持范围。负数、分数、字符串、NaN/Infinity、负零和超32位值必须拒绝；零合法。布尔仍只接受真正boolean，不能把0/1自动转为开关。

### 9.2 复用与版本边界

条件语义没有改变，保持policyVersion=2和原Watch/runtime；不引入policy3、第二worker或新的存储表。A1仍bool-only；A2目标成为明确的boolean/uint32封闭联合。布尔原签名内部消息不新增字段，保持旧请求字节及原意图；uint32能力选择/回应与执行绑定显式包含valueType和值，由既有definitionDigest、请求签名和bindingDigest覆盖。

这不是对旧同版本客户端的无感兼容：旧严格实现会拒绝uint32分支。包含新类型记录的环境须匹配升级App、Gateway和Broker；不能用旧组合降级读取新规则、清历史或把新意图改写为bool。旧bool规则、原CAS/ID、混合合法请求日志和初始精确重提继续兼容；不改原journal前缀、账号隔离或丢回执恢复流程。

职责保持：App用缓存Manifest提供选择与约束提示；Gateway持久化完整typed意图、实时owner/根/版本审查；Broker以当前就绪Manifest验证Property/type/write和具体值约束，执行前再次复验；最终仍由原Command编码、ledger和ACK投递。缓存和显示别名不扩大权限，服务不能只核类型而忽略具体值的min/max/step。设备保持普通Property回调/report，无规则表、额外MQTT连接或设备库源码改动；真机测试固件操作另取§9.6。

### 9.3 实施顺序和退出条件

1. 冻结boolean/uint32类型和值合同；扩展现有共享签名能力/执行校验，保持A1和bool消息原样，错类型/遗漏绑定字段拒绝。
2. 沿原Gateway rule→capability→持久事务→execution接线；create准入与dispatch复验具体值约束，未知结果、目标设备冲突、禁止链、冷却和旧请求幂等不变。
3. App目标仅允许可写bool或uint32 Property；数值输入与布尔选项分开，切目标清旧输入，非法值不提交。名称解析、原端点编辑、精确CAS、待确认结果与原请求恢复复用，不重写client/http/journal。
4. 运行共享合同、Broker准入/Command、Gateway合同/管理/事务及App生产模板/客户端回归；隔离PG测试与跳过分别记录。检索并修正bool-only现行文案，删除此次确实失用的重复代码，不删除历史数据或仍有消费者的模块。
5. 软件关闭后再做匹配本机部署与两设备数值短闭环。§9.4原软件批不部署、刷板、修改现场规则/授权或Git提交推送；后续真机与恢复结果单独记录在§9.6，不重复累计软件结果。

必测：0和uint32最大值、负数/分数/非有限/字符串/负零/越界；Manifest类型、可写、fingerprint和min/max/step失配；旧bool/A1原字节、源条件四方向、原ID/CAS、未知响应保留、类型和值错配不能confirmed；切换目标、名称展示、页面退出/切账号迟到回写。ACK只表示设备确认，实物效果另取真实State/report。

### 9.4 N5 实施与软件验收（2026-10-11）

本批必要生产变更为App 5文件、Service共享合同/Broker/Gateway 7文件，没有设备源码或BBP/2 Wire格式变化；只扩既有version2管理/内部签名合同的封闭目标分支。未新增业务模块、依赖、数据库迁移、队列、worker、规则执行器或发送器。设备继续普通Property回调/report；`client.ts`、`http.ts`及原`automation-v1:`持久意图前缀不改，旧记录不回写。替换旧bool-only目标判定和不准确的范围文案，保留仍有消费者的恢复模块，不为减少文件数删除有效接口。

App只给可写boolean/uint32 Property提供目标：数值输入无默认值、显式0合法，切类型清旧输入，同fingerprint可信缓存的min/max/整数step前置核对；缓存缺失不猜约束，服务仍作最终核验。列表、详情、编辑与恢复展示数值原值；布尔继续沿N1别名，不把0显示成“关”或非零显示成“开”。原端点/ID/CAS、编辑启停状态、待确认日志及迟到回写隔离保留。

Gateway、共享签名合同和Broker分别绑定完整数值类型/固定值、持久定义与原执行意图；能力回应必须双向匹配目标是否带typed字段，不能让合法签名uint32回应替代bool请求。当前Manifest约束在准入、投递及原帧重传前复验；补齐大整数step的精确BigInt校验。已知不符合step的值在能力或首次dispatch预检被拒绝，无Command序列、ledger、反向授权或publish副作用；授权等待后约束改变仍禁止publish，原未发送记录按既有状态保留，不冒称此时从未分配ledger。float算法未修改；本节原软件阶段未改设备同类算法，后续对齐另取§9.7。

| 最终软件门禁 | 结果 |
| --- | --- |
| App相关联合回归 | 13文件/201项通过，0失败/跳过；含自动化、N1共用名称、既有任务管理/恢复与Manifest缓存，自动化专项70项包含其中，不相加 |
| App类型/构建 | spec TypeScript和新输出目录的开发Web构建通过；未构建/安装本批Android包 |
| 共享合同/签名 | 6文件/84项通过，0失败/跳过；旧bool/A1字节与numeric绑定、原2,048 B正文门限保留 |
| Broker | 8文件/146项通过，0失败/跳过；另原`test-device-v2.js`适配脚本通过，不虚构逐项数量或累加 |
| Gateway确定性门禁 | 9文件/121项通过，0失败/跳过；合同、管理、worker、精确绑定及未知恢复 |
| 最终隔离PostgreSQL | 修正后冻结组合141项通过，0失败/跳过（含1个父测试、140个子测试），原事务/签名worker/accepted-lane/真实Command编码与ledger/ACK fixture联合验证 |

有限预算压力样本中uint32执行签名正文1,997 B，保持原2,048 B上限；PostgreSQL JSONB测量包含极端源条件、escaped key、最大目标值及disabled定义，本组最大定义2,040/2,048 B、runtime 1,762/2,048 B、candidate 3,126/4,096 B、target binding 695/1,024 B。是有限样本，不是所有浮点/Unicode组合的最大证明，也不把服务内部JSON大小说成设备BBP frame大小。public bool→uint32(50)沿原签名HTTP、PG和Command实际解码为数值50；MQTT载体及设备ACK为测试fixture，**不是新实物效果**。

保留失败证据：一次扩大App回归触及240秒工具时限，没有完整结果，不计为通过；缩至本批相关13文件后完整通过。整数修正后一次临时PG启动超时、测试未执行；下一次容器启动和测试合计触及180秒时限，只完成部分子测试，同样不记为完整通过。只核对/清理本批owner标签的空PG/网络，最终用适当的有界预算重跑同一集合，不增加业务场景或循环，不重启/改配置业务服务。日志、来源哈希和构建产物在本机忽略目录，账号/Key/网络信息不写入公共方案。

最终TAP 141项及随机schema清理成功；原runner因临时network清理返回延迟保留failed/cleanup=false，不覆写为成功。后置只读按本批owner标签核对：全部临时container/network已不存在，清理补证另存。空PG无映射端口、独立internal网络、候选源码只读挂载，未使用本机业务数据库。先前两轮PG通过及中间软件子集不累计；所有最终生产源码哈希复核，保留无关修改和空Git index。N5限定软件切片至此收口，环境失败记录和软件通过分开保存。

### 9.5 数值真机门禁设计（§9.6已完成）

最终软件门禁关闭后才进入本机匹配部署；**限定数值真机已由§9.6关闭，正式发布仍独立待验**。先核对当前网络、账号、两块专用板身份与原绑定；备份APK/应用数据、受影响本机服务数据/配置及两板fresh固件/授权。保持同包名、签名、HTTPS origin和原请求日志，不升级Auth或整组服务。App/Gateway及实际参与链路的Broker必须同一typed合同组合：旧严格App遇到含uint32规则的列表会拒绝整页解析，旧Broker也会拒绝numeric selector，不能混装或降级读取新历史。

用普通可写uint32 Property及原回调/report验证命名停用创建→启用/无动作基线→合法变化的ACK和真实数值State→编辑固定值→退出App后执行→重新进入查询→停用/删除不再触发。至少覆盖0及一个实际Manifest许可的非零值；uint32上限的现有软件证据不冒充窄范围硬件值。沿原30秒冷却、基线、目标设备级冲突和unknown保护，不伪造时间、直写业务SQL或重新生成ID凑验收。结束停用/软退役本批新规则，不删除两台原绑定设备；恢复原固件/授权和临时网络配置，保留业务历史及账号请求。

§9.4原软件批没有部署、刷板、清授权或修改现场规则；§9.6现场操作单独记录。原业务PG不用于隔离测试，无关本地修改保留。不用旧M5布尔实物或N4任务实物关闭新数值门禁，不转入多动作、BLE/网关自动化或服务器长期维护；本次未暂存、提交或推送。

### 9.6 N5 本机数值短验收（2026-10-11，收口）

用户授权的匹配部署与两设备短闭环已完成，不借§9.4软件或旧M5布尔实物作证明。先备份原APK/停止进程后的应用归档、Gateway PG及Broker在线SQLite副本，并校验摘要、PG可读目录和SQLite integrity；本批未另做完整隔离PG恢复演练。新版App、Gateway及实际参与的Broker按冻结源码匹配部署，schema保持051，无迁移、Auth升级或整组维护。原用途密钥、公开CA、账号、HTTPS origin与无关服务保持。

两块专用XIAO分别为COM3源、COM11目标，先核MAC/USB身份与分区并fresh读取、设备验证可恢复范围；COM4的R4不动。当前单库ZIP在新目录安装后，普通WiFi及两份测试固件编译通过；SDK仍3.3.11，不改SDK或分区。仅写app0，写入时NVS/分区表未变，复用原DeviceKey及绑定。源用普通`power:boolean`的串口受控采样/report，目标增加`level:uint32`（0～100、step1）的普通Property；目标值没有串口写入旁路，只有原onWrite回调计数/report。没有设备规则表、第二MQTT连接、调度器或设备库源码改动。

手机同包名/签名覆盖安装，设备读回的新APK摘要一致。所有管理写入经真实Android App已渲染DOM控件，未直接调用页面内部方法、管理API或业务SQL代替用户操作；源变化为真实MCU report，未伪造Broker消息、设备ACK或时间。

| 同一规则的真实过程 | 结果 |
| --- | --- |
| 新建与启用 | “N5 数值联动”，固定目标50；revision1停用创建，revision2显式启用。初始false只建基线，无执行 |
| 非零动作 | 真实源0→1，目标收到uint32 50；第1条执行和Broker ledger为ACK200、sequence/wireSequence5、attempt1；真实MCU level=50、回调1及新鲜State交叉确认 |
| 编辑与App退出后执行 | 同ruleId/端点精确CAS到revision3，改名“N5 数值清零”、值0，保持启用并重建基线。源回0；独立记录App force-stop、ADB在线且无PID后再源0→1，第2条ACK200、sequence/wireSequence6、attempt1，真实level=0、回调2及新鲜State确认 |
| 查询 | 重启真实App、按新名称进入正确规则详情，显示已启用、值0和两条“设备已确认”，不依赖App在线执行 |
| 停用与删除 | revision4停用、revision5软删除；各自在原冷却期结束后再次发送真实源0→1。四个源State均由Broker新鲜接收确认，执行/ledger仍2条、目标level=0及回调2；历史详情保留两确认，无启用/编辑入口 |

两条PG execution与原Command ledger的序列、frame SHA-256及ledger摘要逐项匹配；State fingerprint对应当前真实Manifest，更新时间不早于执行。ACK只证明设备协议确认，MCU属性/回调另证明应用软件执行；本批无逐包raw ACK抓包、GPIO实物或物理动作exactly-once结论。一次历史规则只读选择错误被操作器阻止继续写入，改为按新规则名称精确选择；原规则逐行不变另有终态审计，失败证据未删除。

终态schema051、6条规则/14条执行、enabled0/unresolved0；原5条规则/12条执行与fresh基线逐行相同，仅新增本次1条retired规则/2条ACK执行。没有回灌业务库、删除历史、刷新Key、清授权或删除两台原绑定设备。两板从各自本次fresh备份恢复：COM3从0x9000起1,339,392 B、COM11同起点3,371,008 B，官方ROM验证后再次完整读取该范围逐字节相同；未erase-all、写分区表或system分区。不将这个范围称整块8 MiB，也不将恢复字节/启动复位称恢复后新云登录通过。

原Gateway网络环境/端口配置已恢复；只删除本批受限device-only代理及新增ADB通道，无关容器ID/image/StartedAt保持。保留匹配新版App/Gateway/Broker与typed历史，不降级旧严格版本。最终只读运行审计与离线跨证据验证均通过，原账号及3条任务请求的key/content摘要在部署后和验收后精确保留，原自动化意图0条仍0条；已确认新意图按原正常流程移除，不把它误判为日志丢失。结束停止App形成可读数据归档，未注销或清数据。

保留安装归档限定：文件级比较仍为failed，131个成员变化，其中23个关键成员变化，含一个旧LevelDB日志移除。备份后为取原账号/库存曾启动旧App，比较窗口混入旧App活动，不能归因安装，也不声称全部文件字节不变；安全存储XML字节及摘要一致，账号/原请求保全另由上述逻辑哈希证明。未用旧归档覆盖新日志。原失败、备份、请求和执行证据均留在本机忽略目录`build/n5-numeric-live-20261011`；该目录含凭据与私有数据，不整体上传。最终证明取`final-proof-1791680052558.private.json`，不是历史准备快照。

N5有限本机验收至此收口，无本现场批新增生产源码、Git暂存/提交/推送。int32/float目标、Action、多动作、BLE-only/网关、跨账号/分片、全平台、长稳与生产不外推；当时剩余的设备通用整数step随后由§9.7有限对齐，浮点边界仍后置。不自动重开已过门禁或扩展服务器维护。

### 9.7 设备通用整数 step 有限对齐

本批只修改`model/EndpointValue.inc`的既有数值校验。Command、desired-state Apply、设备report/状态存储、PatchBuilder与任务目标仍调用同一个`validateEndpointValue`；不增加通道专用校验器、依赖、缓存、接口或Wire字段，不扩大自动化目标类型。

有约束的整数值及声明的min/max/step沿原合同限制在±(2^53−1)，step必须为正整数；基点仍为minimum，没有minimum时为0。整数step分支先验证value/base/step有限、精确且在安全范围内，分别转为`int64_t`后计算`(value-base) % step`。差值最大2^54−2，仍在int64范围内；不先作double减法，不用浮点容差，不引入动态内存或大整数库。直接调用公共校验入口遇到非法step/base也拒绝，避免转换越界或除零。

Float32/Float64原容差分支、min/max边界、CBOR类型/编码和错误合同保持。无约束整数仍允许完整int64/uint64，不把现有64位Wire缩成32位；带约束整数的原safe-range也不扩大。原容差误放行的非step值改为`ValueOutOfRange`，不自动舍入、改写输入、迁移旧状态/任务或清除存储。清除的是整数走浮点step容差的执行路径，仍有消费者的浮点校验保留。

新增独立host目标`blinker_endpoint_integer_step_tests`并纳入CTest，不继续扩大已有大型`test_main.cpp`。回归使用真实CBOR Writer与公共校验，覆盖巨大合法step、0及邻值、正/负偏移、跨safe端点的精确差、仅maximum/无step、非法step/base、无约束全64位及既有浮点/布尔行为；另用真实EndpointStateStore验证keyed/id非法多字段更新不修改状态arena/已用长度，合法更新正常提交。

| 软件门禁（2026-10-11） | 结果 |
| --- | --- |
| 旧实现红测 | 追加最后10个案例前的73直接值案例/199断言中29失败；不将该旧集合称最终83案例 |
| 最终专项 | 83直接值案例/219断言全部通过；严格MSVC x64与Win32构建/CTest均通过 |
| 严格全量host | 72个CTest目标全部通过，无失败/跳过；包含专项、Runtime/API、任务与golden/fuzz smoke，不和219断言相加 |
| MSVC AddressSanitizer | 公共Runtime/API、整数专项、TaskSnapshotStore、TaskProduct、TaskFacade共5个相关目标通过；Windows不提供UBSan，本批不宣称已跑UBSan |
| Arduino源码可移植编译 | UNO R4 WiFi（Renesas core 1.5.3）及Wio Terminal（Seeed SAMD core 1.8.5）的`CompileV2`均通过；未安装/升级SDK，仅验证源码核心目标，不代替ZIP安装、完整产品或真机验收 |

Arduino编译保留SDK自身告警，未将有告警的编译称为零告警门禁；严格host构建使用`/W4 /WX`。R4核心测试程序为73,192 B Flash、8,536 B静态RAM，Wio为52,604 B Flash；这些是`CompileV2`总体占用，不是本次25行校验的增量或WiFi/BLE产品占用。

本批没有修改App/Service生产源码、部署、刷板、清授权或Git暂存/提交/推送，原现场仍运行§9.6结束保留的版本；库源码修复不等于已安装设备完成升级。浮点极端有限性、复杂自动化及全通道真机验证仍后置，不以软件/编译门禁冒充实物或正式发布，不重新开启数值真机全套。

### 9.8 设备与App整数step短差异验收（2026-10-11，收口）

本批仅操作COM3专用XIAO ESP32-S3，COM11及COM4的R4不刷写。先核USB/ROM身份和实际分区，再fresh读取并由设备验证从`0x9000`起1,339,392 B的NVS/otadata/app0恢复范围，分区表单独备份；不是整块8 MiB备份。原APK和停止进程后的应用数据归档先备份、验证摘要及可读性。正常App身份查询确认原DeviceKey与credentialVersion=2匹配，不刷新Key、清授权、创建新绑定或写业务SQL。

冻结当前库407个src文件，以`BlinkerWiFi.h`和Arduino ESP32 SDK 3.3.11编译修复候选，应用二进制1,031,264 B，适配板上真实app0容量；本批是当前源码编译，不是新ZIP安装门禁。仅写app0并ROM验证，写入时NVS和分区表不变。测试固件只增加普通uint32 Property及原onWrite/report回调计数；串口`j`只读取值和计数，没有串口赋值、ACK或State注入。生产修复仍仅§9.7共享校验入口，本批未增加生产源码、接口、依赖或Wire字段。

沿用§9.6实际安装的匹配App、Gateway及Broker镜像，不重新部署新代码或升级schema051。因当前网络与原服务公布地址不同，只临时调整同镜像Gateway的设备登录地址/受限端口，并建立仅允许既有设备鉴权POST路由的本机代理；其他业务入口404，Broker/Auth及无关容器不动。结束恢复原Gateway配置并删除本批代理，没有删除volume/image或回灌业务数据库。

非法值经**真实已登录App的既有原生LAN诊断composition**发送：signed grant、设备instance核验和PlainHmacSha256 record鉴权均沿原链路，record profile=2，不降低为无鉴权连接。App普通LAN开关仍关闭，未改supported/available判定，也未新增BLE授权。该路径的既有Command编码检查类型/min/max而不前置过滤step，使非法整数实际发往设备，不用Broker拒绝或App自动化表单的前置拒绝替代设备证据。原页面控件另经正常用户Cloud/MQTT链路验收。

两个严格目标均为uint32，maximum=4,294,967,295、step=1,000,000,000,001；`strictZero`的minimum=0，`strictBase`的minimum=1。在此范围分别只有0、1是合法step值，非法邻值1、2正是旧浮点商容差会误放行的样本。旧实现红测取§9.7软件证据，不声称本批另刷旧固件完成硬件红测。

| 实际指令 | 设备协议证据 | 回调和State |
| --- | --- | --- |
| strictZero=0，合法 | 原session parser收到真实ACK6；首条raw RX观测漏测，严格trace记录仍为failed，未重发该逻辑操作 | zeroCallbacks 0→1，值0保持；合法同值写入不要求State revision增加 |
| strictZero=1，非法 | 实际Command10正文`a10301`；设备Error6关联10，正文`a20006010a` | 四属性、四回调计数及State revision全部不变 |
| strictBase=1，合法 | Command11、真实ACK11 | baseCallbacks 0→1，值1保持 |
| strictBase=2，非法 | 实际Command12正文`a10402`；设备Error6关联12，正文`a20006010c` | 四属性、四回调计数及State revision全部不变 |
| power=true/false | Command13/14、真实ACK13/14 | 各增加一次power回调，值与真实State变化一致 |
| level=50/0 | Command15/16、真实ACK15/16 | 各增加一次level回调；同sequence16原frame重传两次、收到两ACK，回调只增加一次 |

诊断结束前经原refresh收到真实StateSnapshot，manifestAccepted/stateFresh均为true、revision5，值为power=false、level=0、strictZero=0、strictBase=1。关闭session后状态标记失效是正常清理，不能拿关闭后的快照替代上述新鲜证据。同序号重传不等于新逻辑操作重发，也不据此宣称物理动作exactly-once。

随后在真实App已渲染设备页面点击原开关开/关，并以原生输入事件将原滑块设为50再归零，不调用页面内部控制方法。四项均有独立MCU回调和页面State支持；最终powerCallbacks/levelCallbacks各为4，strict计数仍各1，State revision9、accepted/fresh及canControl为true，power=false/level=0。此普通UI回归不另宣称逐包raw MQTT ACK抓取；真实raw ACK证据来自上述诊断指令。退回首页后连接组/BLE link/session均归零，诊断已关闭，原账号保持。

保留两项现场限定：新安装的私有观测器未接管已在途的receive调用，导致首合法0的raw RX漏测；修正私有工具并用只读refresh消费旧引用后，剩余七项逐帧证据完整，未改App生产代码或重发首操作。一次显式续接约117秒后返回`LOCAL_WS_RECEIVE_FAILED`，未发业务Command；源码的120秒lease＋15秒tombstone及单槽容量是候选解释，内部ledger未测，不能称已确认根因。超过该边界后仅显式续接一次并完成剩余操作，不清授权、扩槽或自动重发业务指令。

最终COM3从本次fresh备份恢复原NVS/otadata/app0范围：ROM验证通过，再次读取1,339,392 B逐字节相同，分区表也相同，并完成启动复位；未erase-all、写bootloader/分区表/system。**修复候选有限验收通过，现场已恢复原固件，不是所有设备完成升级，也不是恢复后新云登录已验收。** 原安装APK摘要不变，原账号及可读前后应用归档保留，不降级App/服务或回灌旧日志；仅移除本批ADB通道。终态schema051、6规则/14执行、enabled0/unresolved0，fresh基线的规则/执行逐行相同，Broker及无关容器ID/image/StartedAt不变，407个生产src摘要不变。

证据留在本机忽略目录`build/integer-step-live-20261011`，最终跨证据审计为`final-proof-1791685621165.private.json`；目录含固件、账号、Key和网络配置，不整体上传。本批未暂存、提交或推送。仅限定ESP32 uint32 step＋App LAN鉴权诊断／原Cloud页面控制；不外推BLE、R4/Wio实物、全通道、浮点、长稳或生产，不重新派发已收口N1～N5或扩展自动化引擎。

## 10. 核心稳定与最小内部交付

### 10.1 交付目标与功能边界

先交付一套同事可以安装、按说明使用并验收的内部匹配候选，不先制作生产发行版。N1～N5和整数step差异已取得各自限定证据；剩余工作是把当前源码、可安装制品、匹配服务和使用说明对应起来，不重新设计协议、鉴权或执行器。

普通核心仍为一个Arduino库、原WiFi/BLE/WiFi+BLE入口、Manifest/State/ACK、属性/动作控制、附近与连接状态、可选授时及租约采样。App只维持一个用户MQTT连接，Gateway/Broker复用既有路由和Command ledger；设备本地任务、云端自动化与连接/采样职责分开。

定时仍是ESP32普通WiFi、owner、原4槽/单目标的封闭试点，必须固件profile、Gateway和实际Broker三侧准入。自动化仍为同owner/tenant/shard的两普通WiFi设备、原标量条件和一个boolean/uint32固定Property动作。默认创建停用、显式启用、未知结果保留原请求；退出App不停止已确认的设备任务或云端规则。上述范围不是所有平台、设备或账号默认开放。

### 10.2 剩余缺口与不应重做的内容

| 剩余项 | 最小处理 | 关闭条件 |
| --- | --- | --- |
| 交接入口仍将旧schema49/50及APK称为当前 | 更新候选索引、App handoff和Service使用说明；保留旧摘要为历史 | 一个当前匹配入口，不从旧批次自动派发任务 |
| 新设备修复制品及声明矩阵（已关闭） | §10.6已冻结并重建含N3/整数step的单一development ZIP，不覆盖旧包；隔离七目标通过 | 当前ZIP与所选源码/profile对应；不挪用旧包、任务或实物证明 |
| 旧架构门禁20 B约束（已修正） | §10.5检查已收口：物理容量与协议协商分开，保留鉴权前20 B、整record边界和失败回退 | 不再次派发此旧缺口；保留正/负回归，不把上限244 B当默认分片大小 |
| 已测App/服务不等于当前工作树所有改动已测 | 记录实际APK/镜像与各自来源；必要新改动先限定差异再构建 | 新来源/旧现场事实不混写，不给无关修改套用旧验收 |
| 初学者/接收同事缺一份最短安装和准入说明 | 复用四个公开例程，说明库配置由维护者提供、用户只填Key/网络/组件；任务/自动化资格独立列明 | 不要求用户拼composition、运行Python或配置内部用途密钥 |
| 接收方安装与运行记录未独立完成 | 同事在明确的本机测试环境验证所选候选，记录来源、结果和限制 | 接收人实际确认，不能由发送方历史测试代签 |

无需再次实现：N1名称映射、N2有界附近扫描、N3数据长度协商、N4真实容量/来源、N5数值动作，以及设备整数step修正。普通设备session HTTP/MQTT已经允许明确plain配置；DeviceKey challenge-HMAC是身份校验，不是TLS。App查看/复制Key的HTTPS与设备session承载是不同职责，不为DIY便利删除账户或设备鉴权。

当前没有证据要求大规模删除仍有消费者的模块。保持五个App相邻插件闭包、共享校验、任务意图恢复、Direct/Noise及底层ports；只删除确认无消费者的旧路径。架构门禁中过时的“物理上限必为20 B”约束已按§10.5替换为现行分层不变量，BLE安全回归保留，不重复作为新待办。

### 10.3 当前已验收的匹配组合

本表取后续[H1§9.5～§9.6](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md#96-h1真实分享只读与撤权收口2026-10-11)实际组合，替代本节原N5版本推荐；旧N5摘要仍保留于§9.6～§9.8。H1恢复并保留本机测试地址/代理，App专属ADB桥已撤且进程已停止；不是本批再次探测现场或可独立连接任意网络的生产包。使用前仍核实际地址、HTTPS信任及配对下发。

| 部分 | 当前限定依据 | 下一候选注意事项 |
| --- | --- | --- |
| Android App | H1实际APK 79,987,977 B，SHA-256 `34e3499a4db42ed9db33bc6cec5f06fbd444dd51ae3df5ad1a84bcf2dfe56b35` | 同AppID/签名/HTTPS origin；保留原已装私有LAN-enabled overlay，普通源码LAN/P1开关未改；五插件及overlay来源分开交接 |
| Gateway | H1镜像 `sha256:9f3e78a6311d87a93eef981748a476c3e85ec27c79a4d0a9f8e14e2eef9f6e23`，schema051 | 已接H1读ACL与路由，保留原N1/typed uint32合同；不新迁移/整组升级 |
| 参与Broker shard1 | H1数值ID排序修正镜像 `sha256:b895b8a08a31eabd6360109e687f0731b3ebb492878c41f5593b910cf02fb96c` | 用途配置与Gateway配对；输出ID仍为字符串，SQL按INTEGER倒序；不外推shard0/2已部署H1 |
| 设备修复源及内部ZIP | §9.7～§9.8原N3/step限定结果，§10.6原七目标；当前README/BLE实时例程差异取§10.7 | 407源码/profile不变；新旧ZIP摘要不同，BLE例程编译证据单列；未刷当前板或代签联网接收 |

旧严格App/Broker不能读取或准入uint32历史；规则停用/删除不使旧版本重新兼容。回退不能清历史、回灌旧DB或换origin隐藏意图。Git HEAD、库版本号0.1.0和相同schema都不能单独证明配对。

### 10.4 最小执行顺序

| 顺序 | 实施内容 | 范围与退出条件 |
| --- | --- | --- |
| 1 当前入口和门禁收口 | 本节、候选索引、App/Service说明；运行现有架构/打包/profile检查，修正与N3冲突的旧断言 | 不动生产通信、现场数据、SDK或默认开关；失败和通过分别记录 |
| 2 同源内部候选 | 审阅并冻结本批必要设备输入，复用原发布器与source-handoff；App/服务沿用§10.3匹配版本及原来源，不重导无变化的闭包 | 一个用户ZIP；普通七编译目标与显式任务目标分别记录，未测目标不宣称新版通过；无变更的匹配App/服务不重复部署 |
| 3 最短使用与差异验收（已关闭） | 取§10.7及现行QuickStart；沿用已测H1 App/服务，README/BLE例程差异、新源同字节重建及S3/R4 BLE安装编译已通过 | 不重复部署/M5/N1～N5或100轮；保留原账号/请求/授权，不借旧ZIP证明新例程或代签独立接收 |
| 4 Git源码交接（签收待完成） | §10.9三仓当前分支及五插件固定提交；接收同事按Git手册自行构建、核公开配置，完成安装、ACK/State和恢复检查 | 本地交接包不保留；凭据、Flash/账号备份、日志和构建产物不进Git；提交/推送结果另核，发送方测试不代签 |

七个普通安装目标按`tests/arduino/verify_installed_release.py`：ESP32 WiFi/BLE/WiFi+BLE/CloudSmoke、UNO R4 WiFi/BLE、Wio WiFi；任务另显式选择`esp32-wifi-tasks`。一份ZIP不等于同时启用所有扩展；任务＋LAN等未准入组合仍应拒绝。离线编译不需要更换实物板，新的全平台硬件验收不在本次最小交付范围。

后置且不阻塞内部普通核心候选：生产域名/信任根/KMS/release签名、独立冷构建及完整离线SDK闭包、iOS、Wio BLE正式长稳、分享任务、未知根恢复的拒绝边界放宽、浮点/多条件/多动作、网关本地自动化、LAN/P1默认启用、无缝通信新策略、History/Bulk和服务器长期维护。缺这些条件就保持明确preview/封闭试点，不扩大功能声明。

### 10.5 本批执行记录

已完成三端只读缺口核对和当前源码结构检查；现有单库白名单检查为416文件、一个src/四个公开examples，生产源码未发现对旧products/integrations/adapters或tests/build的依赖。当前入口、App handoff、Service说明和发布profile文案已对齐本节，旧制品与限定验收保留为历史，不覆盖原记录。

首次架构检查确实失败：旧断言强制ESP32物理包上限20 B，与N3的物理容量和受保护协议协商分层不符。现沿原检查器改为物理MTU上限、鉴权前20 B起步、完整record边界及CapacityExceeded回退等分层不变量，删除失用的局部变量；新增1个现行正例及4个独立拒绝负例，复用原打包测试文件。此检查是源码结构门禁，不是AST或全部运行行为证明；既有运行回归继续负责对应行为。不修改生产BLE代码或删除安全检查来放行。

| 本批最终检查 | 结果 |
| --- | --- |
| 单库白名单 `--check-only` | 416文件；未生成新ZIP，不将清单通过称为安装/链接通过 |
| 架构与发布profile | 均通过；保留首次旧断言失败，不覆写为成功 |
| 打包回归 | 最终24项全部通过、无跳过；先前19项属于修订前集合，不相加 |
| 现源码限定host重建/CTest | Release重建Runtime/API、DirectSecureSession、ESP32 NimBLE adapter/capacity、整数step五个既有目标，5/5通过；不冒称本批全量72项或真机复测 |

构建准备曾误用不存在的`blinker_esp32_nimble_tests`目标而停止，按CMake实际目标名纠正后才取得上述完整五目标结果，不将首次部分构建算完整通过。顺序1至此关闭，下一批仅进入顺序2：审阅、冻结必要差异并生成含N3和整数step修复的单一内部development ZIP，沿原安装门禁记录实际结果；不因入口整理重部署无变更的匹配App/服务。

本批没有新增运行功能或生产源码改动，未部署、刷板、改变SDK/默认开关、改业务数据或暂存/提交/推送；无关本地修改和私有证据均保留。

### 10.6 N3与整数step单库候选冻结及安装编译（2026-10-11）

本节记录当批顺序2，沿用当时N5匹配App/Gateway/Broker（原§10.3，精确摘要保留于§9.6～§9.8），不重建或部署；不能反向套用后续H1更新表。只读审阅确定必要运行差异为13个N3源文件和`EndpointValue.inc`整数step修正，共14个；没有新增生产代码、协议、依赖版本或默认功能。407个`src`文件整体进入单库白名单，配套回归留在维护仓库，不进入用户ZIP。

N3仍从20 B起步，仅在Direct已鉴权、加密且active后按Hello可选能力、实际ATT MTU和workspace共同协商，旧对端维持20 B。仅明确未发送的CapacityExceeded允许原密文回退，临时背压保留记录，未知发送结果关闭安全会话。整数step复用现有校验：安全整数操作数分别转int64再取余；不增加浮点功能或另一个校验器。上述运行行为取既有回归、N3的§7.4～§7.5及整数step的§9.7～§9.8限定证据，本批编译不替代新硬件测量。

唯一内部Arduino制品为`build/internal-n3-step-zip-20261011/rebuild/artifacts/BlinkerDeviceV2-0.1.0.zip`，692,951 B，SHA-256为`b80f5126ef035dfe5c59f1b8d4615d2b025fa2aca2c15a6d26704d40adc2a43d`，同目录有`SHA256SUMS`。ZIP仅有`BlinkerDeviceV2-0.1.0/`库根、416文件：407源码、4公开例程、5根文件；没有按MCU/模式拆包。除3个profile生成头外，其余413文件与所选工作树字节一致，14个必要修复均未遗漏。

沿原`build_arduino_handoff.py`导出433项设备维护输入，在新目录重建得到与参考ZIP逐字节相同的制品。冻结`manifest.json`摘要为`051ac128c74e6ab1eea3f6c79d9135209558f00c2cba6c50884a1ecb63b0aa55`。该既有交接闭包限定ESP32普通WiFi/PubSubClient源码重建，**不包含完整七目标SDK/CLI，不能称全平台离线工具链包**；本轮七目标使用另列的已安装SDK和固定依赖。

profile沿用N5本机控制地址`192.168.2.76:17002`和对应development公开验签材料，仅固件版本改为`device-v2-internal-n3-step-20261011`；普通Cloud session/MQTT明确plain，任务/LAN默认关闭，无`task_profile`。**此处地址是所选N5配置，不是本批验证过的当前网络可达地址；安装编译成功不等于用户仅填Key即可在任意网络上线。** 不是生产信任根或正式发行，当前运行板未升级。使用前核对实际服务地址、配对下发与信任，缺口不靠删除鉴权或无感改配置解决。

| 本批门禁 | 实际结果 |
| --- | --- |
| 单库边界、架构、发布profile | 均通过；416文件 |
| 完整release工具回归 | 35项通过、零失败/跳过；不是上一批24项相加 |
| 冻结源重建及包审计 | ZIP字节相同、14修复一致；3生成头与所选profile对应 |
| 隔离Arduino安装及七目标 | 7/7通过；三组新建空sketchbook安装同一个ZIP，未跳过目标 |

七目标只编译从该ZIP安装到新建空sketchbook的公开例程，不取工作树源码或旧产物。复用原门禁，安装逐文件、固定依赖、实际编译输入、最终map/ELF与未选功能剥离检查全部通过；`--clean`、jobs2。CLI为1.5.1；下表Core版本又按各目标实际`build.options.json`核对，不以预处理、`.o`或归档生成代替链接通过。固定外部依赖为PubSubClient2.8、ArduinoBLE2.0.2、Crypto0.4.0、Seeed FS2.1.3、mbedTLS3.0.2、rpcUnified2.1.4、rpcWiFi1.1.0；SDK内置依赖沿所列Core，不宣称完整外部工具链随ZIP分发。

| 普通目标／公开例程 | Core／FQBN | 安装编译门禁 | Flash B | 静态RAM B |
| --- | --- | --- | --- | --- |
| ESP32 WiFi／WiFiQuickStart | ESP32 3.3.6／`esp32:esp32:esp32` | 通过 | 995,683 | 50,912 |
| ESP32 CloudSmoke／CloudSmokeTest | ESP32 3.3.6／`esp32:esp32:esp32` | 通过 | 1,013,139 | 51,016 |
| UNO R4 WiFi／WiFiQuickStart | Renesas 1.5.3／`arduino:renesas_uno:unor4wifi` | 通过 | 139,680 | 13,052 |
| UNO R4 BLE／BleQuickStart | Renesas 1.5.3／`arduino:renesas_uno:unor4wifi` | 通过 | 195,596 | 16,964 |
| Wio WiFi／WiFiQuickStart | Seeed SAMD 1.8.5／`Seeeduino:samd:seeed_wio_terminal` | 通过 | 143,936 | SDK未输出 |
| XIAO S3 BLE／BleQuickStart | ESP32 3.3.11／`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB` | 通过 | 694,484 | 35,984 |
| XIAO S3 WiFi+BLE／WiFiBleQuickStart | ESP32 3.3.11／同上 | 通过 | 1,283,720 | 56,652 |

Wio本次CLI只报告程序大小，静态RAM项保留未输出，不填0或挪用历史值。S3在本轮编译前通过原SDK pin检查，官方3.3.11的IPC配置2048 B、Host配置5120 B；这不是本批运行栈余量或峰值测量。

用户ZIP未含docs/tests/tools/build；维护冻结源另含原重建工具及必要公开说明，二者用途不同。包及冻结源未含私有备份、账号/DeviceKey、WiFi密码或构建制品；有限已知秘密和模式扫描未命中，不据此声称通用秘密扫描覆盖所有可能。只交付上述ZIP/摘要与公开说明，不整体分发本机build目录。

官方Arduino入口保持Small的一个BLE session/一个TX槽；自定义多会话/多TX组合未准入，故障回调清队列后的跨peer行为须另做专项验证，不借此次单槽编译扩展承诺。Flash/静态RAM结果不是运行heap、SDK任务栈峰值或长稳证据。任务显式编译目标和全平台实物不在本批声明矩阵中。

隔离冷编译初期遇到宿主可用内存偏低及分页竞争，保留原日志并完成现有门禁；没有停止匹配服务、关闭安全软件、借历史对象或改SDK/FQBN绕过。普通stock/portable/ble三组分别3061.7/3207.8/839.6秒，均退出0；这是本机冷构建耗时，不是设备运行性能。`seven-target-evidence.json`记录每个目标的实际Core/FQBN、编译日志、compilation database、ELF及map摘要，最终审计再次核对冻结433项、manifest和ZIP，全部一致。

本批仅更新本节及候选/主线三份文档，未增加生产源码变更、部署、刷板、操作账号/业务数据或暂存/提交/推送。末尾源状态断言通过：407生产源码、公开例程、Git索引、其他库已跟踪差异以及App/Service的差异/索引/状态摘要与初始相同；无关本地修改保留。**顺序2至此关闭。下一步仅顺序3：最短安装使用说明、实际使用前的地址/信任核对及必要差异，随后顺序4独立接收。** 不重开N1～N5、M5、100轮或复杂新功能；本轮安装编译不代签同事接收或生产发行。

### 10.7 最短使用、匹配来源与例程差异（2026-10-11）

本批只执行顺序3，沿用已验收H1 App/Gateway/Broker，不部署或刷板。现行用户说明取[Arduino/LLM QuickStart](DEVICE_V2_USER_API_AND_LLM_QUICKSTART.md#0-最短安装与接入)：一个ZIP、四个例程、默认`Blinker.h`、BLE/WiFiBLE明确selector、参数/回调/真实状态、诊断及接收模板。已删除该QuickStart中过时的R4a/R4b执行队列，修正“Blinker.h非用户入口”“默认SoftAP”等旧结论；canonical/profile及三端当前索引同步校正，不删除仍有消费者的协议、授权或platform模块。

README原BLE/WiFiBLE短示例漏了`onWrite`，当前绑定会因没有回调而拒绝写入；现仅补回调示例，不改业务API。只读审计另发现§10.6冻结包早于BLE正式例程新增`uptimeSeconds/onSample()`，故旧七目标结果不能覆盖新BLE例程。本批不再次修改该例程，而是为实际差异补隔离安装编译。

新内部ZIP为`build/internal-core-handoff-20261011/artifacts/BlinkerDeviceV2-0.1.0.zip`，693,054 B，SHA-256 `30b9ae2bc3a829d370470a15bb7dd9501edffc4d09599877c8acc182ba9b8c9e`。仍416文件、一个src/四例程；与§10.6原包只差`README.md`和`examples/BleQuickStart/BleQuickStart.ino`，407源码（含3生成头）、其他例程、版本/profile均逐字节相同。旧包及其433项冻结源/七目标日志不覆盖、不改摘要；同名0.1.0不能代替精确来源。新包仍development/普通plain Cloud、无task_profile且普通LAN关闭，不因文档整理扩准入。

本批实际补测XIAO S3 BLE与UNO R4 BLE，均从新ZIP安装到新的空sketchbook，使用原SDK、固定依赖、`--clean --jobs 2`和现有门禁，串行完成。实际编译输入、安装来源、ELF/map与链接闭包均通过，不以仅安装或编译命令退出代替来源校验。

| 新候选实际编译目标 | SDK | Flash | 静态RAM | 结果 |
| --- | --- | --- | --- | --- |
| XIAO ESP32S3 BLE | ESP32 3.3.11 | 694,820 B | 36,000 B | 安装/编译/链接/来源通过；门禁324.2秒 |
| UNO R4 BLE | Renesas 1.5.3 | 195,852 B | 16,988 B | 安装/编译/链接/来源通过；冷构建门禁1,764.7秒 |

其他五目标没有重编，只保留§10.6原编译与本批输入字节等价证明，不称本批又跑7/7；静态资源值不代表运行堆/任务栈峰值或长稳。R4新增实时BLE例程的编译待办至此关闭，**该新版例程的R4实物仍未复测**，旧R4基础实物不能代替。

433项有限维护输入重新冻结，manifest SHA-256 `ea735495c632f5c85e39dc79638c8864c162ccc8e55a7e0eb939504672ea98be`；在独立目录重建出与新ZIP完全相同的字节。此闭包仍只包含原ESP32普通WiFi/PubSubClient维护重建所需源码，不含完整七目标SDK/CLI。公开QuickStart/canonical及本节说明另行交接，不声称它们已进入用户ZIP或该433项闭包。

单库白名单416文件、架构及发布profile检查均通过；打包回归固定输入复跑35/35通过。首轮35项中精确重建测试曾失败（ZIP不等），发生在README修订窗口；失败日志保留，未证实唯一根因，不通过放宽相等检查消除失败。最终冻结输入未再变化，新旧ZIP/profile摘要独立核对通过。脱敏结果取`build/internal-core-handoff-20261011/{final-evidence,finite-compile-evidence}.json`及`release-review-results.md`，完整编译轨迹在同一忽略目录；不分发整个build目录。

H1现行匹配摘要取§10.3；App基础源码manifest与已测私有LAN overlay不是同一配置，不能把普通重建称实测APK。状态历史只限已采纳State，撤权后下一次查询失败清空旧数据，不等于即时push、分享控制或全部shard部署。源码说明与公开profile交接，私有Flash、账号、网络密钥、业务库和整build目录不分发。顺序4仍须接收同事实际签收，发送方测试不能代签。

**顺序3按本节范围关闭，下一步仅顺序4：同事按现行QuickStart接收模板独立安装、短链检查并记录通过/失败/未测。** 本批没有改变生产runtime/API/协议、SDK/依赖或默认开关，没有部署、刷板、操作账号/授权、暂存/提交/推送；保留无关本地修改。缺接收人/现场记录时保持待接收，不用重复打包、长循环或H2/Bulk替代。

### 10.8 三端独立接收材料与空白清单（2026-10-11）

> **历史材料记录，不再作为当前交付方式。** 用户随后改为从三仓Git拉取源码编译，并要求本地交接包不保留；本节目录、归档和摘要只说明当时生成与校验的结果，不是继续分发或保留这些文件的要求。现行接收步骤取§10.9及[Git手册](DEVICE_V2_INDEPENDENT_ACCEPTANCE.md)。本轮交接副本清理与实际提交/推送结果分别核对，不删私有Flash/账号备份或原请求日志。

本批仅准备顺序4材料，不执行接收者的构建、安装或真机测试。操作入口为[独立接收手册](DEVICE_V2_INDEPENDENT_ACCEPTANCE.md)，分发目录为`build/independent-acceptance-20261011`，根README为该手册的精确副本，`ACCEPTANCE.md`所有本人结果保持待填写。优先使用已有匹配本机服务；没有Auth/Kong、有效HTTPS信任、运行密钥或网络条件时记录阻断，不将IoT源码包伪装一键部署。

| 交接材料 | 实际选材及核验 |
| --- | --- |
| 设备 | 原433项冻结维护输入及manifest，原工具复制前后verify通过；唯一Arduino ZIP为§10.7的`30b9ae2…`，profile为`86227f7…`；不新打固件或拆MCU安装包 |
| App | 原H1的1666项基础源码与五插件、manifest `240b11f…`保持；单APK `34e3499…`及签名核对通过。三文件公开CA/信任overlay与LAN-only配置独立列hash，原来源不修改；基础LAN/P1=false与已测APK LAN=true/P1=false仍区分 |
| Service | 已测298项精确冻结输入汇总`d2d12e18e721296ee834ac5e5e0d6362e434de4b949b3e65a225b880a7d84194`，9项必要测试/原代理工具独立列出。原逐文件/汇总和Docker COPY静态检查通过，不新build或部署；schema51及仅参与shard1范围保持 |
| 公开说明/校验 | 根手册、组件说明、空白签收表、总manifest/SHA256SUMS及只读校验器。共2437个payload文件、133,350,670 B；加总manifest/摘要后2439物理文件。App跨仓公共golden作为单独测试材料保留，不能作部署Key |

可分发的**三端交接归档**为`build/independent-acceptance-artifacts-20261011/BlinkerCoreAcceptance-20261011.zip`，105,265,447 B，SHA-256 `17bd4e1d7efeccf3d250d53fb84798eff5ef8de2682a85e2af9f8e1798138df6`；这不是第二个Arduino产品包，IDE只安装包内那个库ZIP。总manifest SHA-256为`953404e7a871395c2757826b5eb16d863730deb4cbf5b0edfeb956281a454f09`。当前目录和新目录解压后逐文件、长度、重复/未知文件校验均通过，归档与源制品摘要一致；完整性不证明发布者身份。

首次总长度汇总把PowerShell有序字典按属性求和得0，已改为逐项长度累加并让校验器拒绝错误总计。初始归档/元数据保留在私有audit目录，不进入最终交接；未修改任何源码/profile/既有制品掩盖错误。校验器8项独立合成正反例通过，包括错hash、缺文件、多文件、越界路径、重复路径、错总长度及错文件数；这是材料工具回归，不是设备业务或接收者验收。

有限文件路径/私钥标记/已知网络秘密及App源码/APK检查未发现需要剔除的运行凭据。实际账号、DeviceKey、WiFi密码、私钥、App数据/Flash/业务库备份、原请求日志、node_modules/dist/SDK缓存、镜像/ELF/map及无关构建产物不随包提供；没有通用秘密扫描全覆盖保证。必要测试向量包含明确的固定假数据，公开CA没有私钥且不能签发新服务证书。App与Service无统一根许可证，材料只供同事内部接收，不把某子包/插件许可扩展为全仓公开再分发许可。

手册冻结了正确的选测范围：标准例程只必验power与单次button/counter，整数控制/step只能选择原已有可写整数试点，不为签收增字段；Core按已测目标区分，Wio旧WiFi基线为SAMD1.8.5，不将S3 3.3.11外推全部ESP32目标。接收者实际记录fresh Manifest/State、关联ACK及效果、离页重入/普通重启恢复、原程序/授权恢复范围；缓存、App乐观值、发布成功和ROM reset不能代替这些结果。

**顺序4只完成材料准备，独立签收未关闭。** 本批未改runtime/API/依赖或默认开关，未部署、安装App、刷板、访问账号/改授权、迁移/回滚业务库、暂存/提交/推送；原工作树修改保留。维护证据位于`build/independent-acceptance-audit-20261011`，不整体分发；下一步由同事回传包外脱敏签收表，实际失败才回对应模块做有限修正，不自动扩H2/Bulk或重复本机门禁代签。

### 10.9 三仓Git源码交付与独立验收（2026-10-11）

按用户最新要求，三端必要源码、正式示例、回归工具及公开说明交付到各自当前分支：设备`device_v2`、App`i3w-dev-v2`、IoT Service`dev_v2`。五个App插件的必要实现已有远端固定提交，保留相邻目录布局和各自锁文件，不把`file:`改为另一套依赖机制。本批具体新提交与推送完成状态由Git操作及最终报告核对；接收同事先记录三仓HEAD和手册列明的五插件提交，再固定这一组来源完成整轮验收。

唯一当前操作入口是[Git拉取、编译与独立短验收手册](DEVICE_V2_INDEPENDENT_ACCEPTANCE.md)。接收者从源码生成一个标准Arduino库目录，必要时自己生成一个ZIP安装，不按MCU/模式拆包；App从原锁构建模板、Web和Android，服务复用现有Gateway/Broker链路。仓库默认设备host及编译fixture仍为占位地址，不能仅凭Git源码可编译就认定能上线：实际host/port、安全模式、公开公钥与服务下发必须匹配。维护者只在stage注入三个配置头，不改库API；运行secret、用户Key/网络参数、签名私钥和账号由验收环境私下配置，不提交到Git。

必要设备差异为N3包长协商/20 B安全回退、共享整数step校验及BLE租约采样正式例程；App/Service包含N1～N5、实时展示和H1状态历史的现有链路及对应回归。本批不增加功能、依赖、wire、SDK或默认开关，不删除仍有消费者的安全/协议/平台模块。无关`ISSUE_TEMPLATE.md`删除、`docs/partition.csv`及其他本地修改不随交付提交；私有备份、账号、原请求日志和与交接无关的构建目录保留。

| 本批源码交付检查 | 实际结果与边界 |
| --- | --- |
| 设备库结构、架构、profile | 416文件白名单及既有架构/profile检查通过；构建工具和测试留在维护仓库，不进入用户库 |
| Arduino打包回归 | 35/35通过，零失败/跳过，133.685秒；不累计旧批次数，也不称新一轮SDK安装编译 |
| 设备host回归 | 复用既有Release构建，协议主集、整数step、Direct secure、ESP32 NimBLE adapter四目标构建及CTest 4/4通过；不替代实物 |
| App编译 | `device-template:build`、`tsc --noEmit -p tsconfig.app.json`、`ng build --configuration local,local-device`均退出0；Web构建38.167秒，不是本批安装APK或手机验收 |
| Service有限回归 | 115通过、1项PG环境相关跳过、零失败；跳过不计通过，不据此宣称数据库集成或生产部署已完成 |

§10.6～§10.8保留历史制品摘要与有限结果，相关本轮交接ZIP、源码副本和无关打包产物不再分发或继续保留；历史摘要不表示仍提供下载。接收者自行构建的制品另记摘要，不能借旧APK/ZIP证明自己构建的是已测字节。原H1已装APK的私有LAN overlay不随Git交付，普通App LAN/P1默认仍关闭。

**顺序4独立签收仍待同事完成。** 手册要求真实App的fresh Manifest/State、属性写入ACK及真实State、单次button/counter、离页重入、普通设备重启后身份/授权恢复；未知Action保留原ID查询，不盲重放。接收者逐项填写通过/失败/未测与脱敏证据，缺环境明确阻断，不用发送方软件回归或历史真机测试代签。本批不重新部署服务、刷板、安装App或清授权，不扩H2、离线存储/Bulk或复杂自动化。
