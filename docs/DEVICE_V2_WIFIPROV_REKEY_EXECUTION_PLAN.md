# WiFiProv 换网与换 Key 执行方案

更新：2026-11-04。修订 [普通入网方案](DEVICE_V2_ONBOARDING_COMPLETION_EXECUTION_PLAN.md) 的产品规则；已完成的旧 Wi-Fi-only 验收仍是历史证据，不代签新规则。

## 1 产品边界

- 首次 WiFiProv 入网创建一张 Cloud 卡并生成 Key。用户从原设备发起重新配网，**保留账号归属、原设备卡和逻辑 ID，换网络并轮换 DeviceKey**；BLE 与 SoftAP 是传输，不另建设备。扫描、选择、连接、取消和只读查询不轮换；禁止因同 MAC、广播名或单独的 instance 合并卡片。
- 普通手工复制／刷新 Key、纯 BLE 认领和 BC-5 保根新增 Cloud 卡均不是本操作，不改它们的 API、准入或历史记录。WiFiProv 安装专用交付使用当前有效会话、owner 和物理证明，不要求退出重登，不调用通用明文 Key reveal；用户主动复制 Key 的交互单独改，不在此处放宽权限。
- 第一批只支持具有既有 Cloud Key＋Admin、同实例同 epoch 的独立 Cloud 测试卡。共享根、任务 authority 或设备 store 尚有绑定／未知操作，先拒绝，不自动迁移、清根或扩范围。保留原设备 ID 不等于旧任务 root 随 Key 无损迁移。

### 1.1 重置和恢复不是普通重配

| 设备还保留什么 | 用户入口与结果 |
| --- | --- |
| 只丢失 Wi-Fi 配置，原 DeviceKey 与 Admin 仍在 | 从原设备卡重新配网。设备在本地配网链路证明原身份，不依赖已联网；保持原逻辑 ID、换网并轮换 Key。 |
| DeviceKey 丢失，但可能仍保留可验证的设备管理根 | 不走普通轮换，也不要求用户输入旧 Key。保留原卡的恢复需要独立的 owner 授权与设备管理根验证合同；在该能力验收前不得静默恢复或跳过旧 Key 校验。 |
| 完整抹除且设备没有可验证的原身份 | 正常添加新设备，走空根首次入网，分配新逻辑 ID 和 Key；旧卡保留，用户可明确移除。相同 MAC、板子、广播名或自报 instance 都不能自动继承原卡、任务、分享或 UNKNOWN。若以后需要保留原卡，应作为独立的替换设备功能设计。 |

设备端重置是否保留身份，取决于实际清除的存储范围，不根据“重置”按钮名判断。App 在身份不匹配时提供清楚的分流提示；正常首次入网或保根重配不要求用户退出重登、复制旧 Key 或输入旧 Key。

ESP32 WiFiBLE 当前分别提供 `Blinker.resetNetwork()`（清 Wi-Fi）和 `Blinker.resetAccess()`（清访问根）。用户只想换路由器时，应走清 Wi-Fi／保身份，不把清访问根当作进入配网的前置动作。硬件 RESET 键普通重启不等同于擦除 Flash 或 NVS。

对照：[小米 IoT 控制端 API](https://iot.mi.com/new/doc/accesses/direct-access/other-platform-access/control-api)公开区分平台 DID 和通信地址；[小米嵌入式 SDK FAQ](https://iot.mi.com/v2/new/doc/embedded-dev/sdk-dev/sdk-faq)说明特定模组的 PID/MAC 登记关系，[小米 IoT 安全基线](https://trust.mi.com/pdf/%E5%B0%8F%E7%B1%B32025%E7%89%A9%E8%81%94%E7%BD%91%E5%AE%89%E5%85%A8%E5%9F%BA%E7%BA%BF6.0.pdf)要求独立密钥或基于秘密导出。不能据此认定某型号重新添加时一定删除旧卡、或只凭 MAC 鉴权；本项目不增加按 MAC 剔除或自动继承的规则。

## 2 最小闭环

| 步骤 | 需要的证据 | 禁止事项 |
| --- | --- | --- |
| 原设备校验 | 显式提交；owner、逻辑 ID、实际 instance、Admin 和原 Key 版本／locator 精确匹配；设备证明持有旧 Key | 不把 MAC、GetInfo 自报或仅登录当授权 |
| 暂存并安装 | Gateway 同一 intent 暂存一把新 Key，用旧 Key 派生的独立密钥加密给设备，App 只转发；设备在现有独占 owner 内 CAS、持久写入并读回 | 不提前吊销旧云凭据，不重发安装、不给 App 明文 |
| 提升云端版本 | Gateway 验证绑定同一 intent 的设备安装回执，CAS 一次推进版本，复用旧会话和 Broker 失效链 | 不接受三字节旧 ACK、页面状态或仅 STA 成功；不再生成 Key |
| 一次 Wi-Fi 提交 | 当前设备／新版本仍匹配；原 WiFiProv 一次 Apply，官方回执与新鲜云状态分别确认 | 不用 Cloud 在线反推 Wi-Fi Apply 或目标 SSID，不自动补发 |

旧 Cloud Key 在持久安装回执被验证前保持有效；不是在 App 点击确认时调用立即吊销的手动 Rotate。安装回执丢失时只读查询原 intent／设备持久 Key 证明，只有精确回执才可显式继续同 intent 的待确认阶段。不能凭到期、App 重启或未上线丢弃可能已装入设备的 Key。Cloud 提升成功但 Wi-Fi 未确认是部分完成，不将旧 Key 重新启用，不重发原提交。两套存储不是一个原子事务，必须保留中间状态。

App 恢复时不能用“曾收到设备安装回执”的非秘密状态代替回执本身，也不能用设备读回证明 Cloud 已提交。先从原 intent 获取精确的新鲜证据；Cloud 提交不明时只承认 Gateway 的持久提交结果。Cloud 上线证据应来自本次 Apply 之后的新会话，不能用 Apply 前已建立会话的新 State 代替。

### 2.1 认证报文边界

旧绑定为 `operationId(16) | serverChallenge(16) | instance(16) | accessEpoch(u32 BE) | oldVersion(u32 BE) | oldLocator(16)`，共 72 B。新绑定再附 `newLocator(16)`，共 88 B，新版本固定为 `oldVersion+1`，拒绝溢出。Gateway 将 nonce 与当前有效会话、原设备和单个 intent 持久关联；设备须在实际 setup owner 下核本机 instance、epoch、原根和设备当前 Key。逻辑 ID 由 Gateway 原行绑定，设备不从 MAC 推导。

独立的 HMAC-SHA256 域（ASCII 加末尾 NUL）分别为 `blinker.wifiprov-rekey.challenge.v1`、`blinker.wifiprov-rekey.wrap.v1`、`blinker.wifiprov-rekey.installed.v1`。设备用旧 Key 对挑战生成原身份的持有证明；旧 Key 派生包装密钥，AES-256-GCM AAD 为 wrap 域与新绑定，12 B IV、32 B 密文＋16 B Tag；**安装回执是新 Key 对完整新绑定的认证码，不是旧 Key 或新 Key 本身。** 固件只有持久写入和精确读回后才生成该回执。重新读取同一已安装新 locator 可为原 intent 生成只读证明，不重写存储或发行材料；不能拿其他设备、其他 nonce 或旧三字节 ACK 升级结果。

安全边界：这证明持有绑定的 Key；持有已复制旧 Key 的一方可以模拟 Key 证明，不等同于独立硬件认证。第一批仅用于受限 owner、真实配网会话和匹配固件的试点，不据此开放默认入口；正式开放另核是否需要设备独有证明及对应的入网锚点。

候选仍走固定 `blinker-config` v2：挑战 `04`（74 B）、安装 `05`（150 B）、只读回执 `06`（90 B）。精确成功回包为 `version | opcode | 0 | operationId(16) | MAC(32)`；3 B 的响应只用于非零失败，状态 `9` 表示持久结果未知。不增加任意 custom，不把这些命令映射成旧 `GetInfo`／Install 的通行状态。

### 2.2 官方 Wi-Fi 写入的前置门禁

现固件收到 `NETWORK_PROV_WIFI_CRED_RECV` 就提交自有网络 journal，旧 `GetInfo` 仅用于配网结束时的 ready 检查。原 Core 3.3.11 SDK 头文件和实际 ESP32-S3 静态库确认：官方 `ApplyConfig` 调用配置写入及连接准备之后，才发 `CRED_RECV`。因此不能靠事件回调拦截提前 Apply；受限候选必须先在真实 `prov-config` 入口、同一有效 setup owner 下拦 `SetConfig`／`ApplyConfig`，拿到本次 Cloud 持久提交的确认才能授权一次；只读状态单独放行。候选相对链接已证实原 SDK `manager.c` 可进入包装函数，包装函数再委托 `network_prov_config_data_handler` 原解析及响应；新薄适配在 typed Set/Apply 回调前加门禁。该证据不是完整链接或运行验收，也不修改已安装 SDK。参考[乐鑫官方处理器](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/network_config.c)。

## 3 执行与验收顺序

1. 先锁定设备 CAS、Gateway proof／包装及 App 非秘密进度状态，做有限回归。现网手动 Rotate、旧配网及 BC-5 不部署替换；上轮普通首装交付的本地未接线代码不作为本操作的证明。
2. 接独立持久 intent：只暂存加密新 Key；挑战可过期，已发行／UNKNOWN 不能自动删除。实际锁定原身份、版本及共享根／任务门禁，验真回执才复用原 Key 提升及 Broker 会话失效。随后接原 WiFiProv 固定命令和 App 同一逻辑 ID 的显式重配；能力不匹配时，新路径不回退到立即 Rotate 或只改网络假成功。先匹配 BLE；SoftAP 保持原 owner 死亡／Cloud／新 Local 同根交接，支持后另验同合同。
3. 原 SDK 完整链接、匹配 App／测试 Gateway 均通过后，优先沿用 COM11 已有测试卡做一次重配，不清 NVS、不重绑定。先只读核无共享／无占用任务和无旧 pending。记录逻辑 ID 不变、Key 版本 `+1`、旧 Key 在安装确认前未失效、一次 Apply 官方回执和新鲜云 State；真机证据不以普通首装、Wi-Fi-only 历史或宿主机夹具代替。失败停在具体阶段，保留授权、日志和 UNKNOWN，不重放或刷新第二次。

## 4 本批结果与下一次验收入口（2026-11-04）

- Gateway：已实现 063 独立暂存及原子提升、旧 Key／手工轮换和任务 scope 围栏。复用独立 PostgreSQL 16 的最终候选 **4 pass／0 skip**，覆盖旧 Key 仍能认证、回滚、COMMIT 后丢回包的只读协调；本地密码学及迁移注册 **11 项通过**。未开放 HTTP、未迁移业务数据库。
- 设备：已实现与 Gateway 相同的证明、解封、读回回执、原 Admin／epoch／locator CAS、固定命令和独立持久读口。定向宿主测试通过；新增官方 Wi-Fi 写前薄适配，反例覆盖未获准／错 session／重复 Set/Apply，定向测试通过。原 Core 3.3.11 ESP32-S3 编译和双向重定位链接通过，未产出可刷产品 BIN。
- App：非秘密 checkpoint 和固定报文适配共 **47 项通过**；原配网读取回归、定向类型／Angular 模板编译通过。未替换页面的原写入链路，也未安装 App。COM11 仅通过现装 App 只读核对了旧卡和 Admin 入口；无 Key、NVS、Wi-Fi 或 TaskStore 写入。未重测普通首装、未部署、未提交推送。
- 下一步接受限 BLE 产品候选：受限 HTTP 绑定有效 owner 会话和本次 intent，实际 native owner 关联 Admin、持久写入预约和独立持久读回；Gateway 提交后提供该 intent 的一次 Wi-Fi 许可。App 在 fresh GetInfo 后将显式重配分流到该链，不支持时不降级到旧 Wi-Fi-only／手动 Rotate；保留 Admin 并 CAS 更新云 Key 版本。沿用固件现有 Apply 成功且配网 owner 退出后才启动 Cloud 的顺序，并避免重启后用旧 Wi-Fi 登录的新会话冒充本次换网。以上接齐后才冻结同批固件、Gateway、APK，在 COM11 一次验收；SoftAP 另验同合同。

## 5 受限 BLE 实际接线（2026-11-04）

### 5.1 相同合同

- Gateway `POST /api/v2/internal/wifiprov-rekey/<operationId>/{prepare,issue,confirm,read}`：默认关闭；私有配置只准一个 Development owner／tenant／逻辑 ID／operationId／物理实例／原 epoch 和版本，通过固定入口与正常 Auth 校验。有效 bearer 的绑定摘要持久化到 064，同一 intent 写操作必须为原会话，不使用登录时点的固定五分钟门槛。只读可用当前同 owner 会话核对，返回非秘密原 binding，不返回盒或许可。
- `08` 向当前真实 Security1 owner 取 16 B nonce；`09` 校验 `oldBinding(72) | AdminId(16) | AdminVersion(u32 BE) | nonce(16) | MAC(32)`，MAC 为原本地 Admin secret 对 ASCII `blinker.wifiprov-rekey.setup-admin.v1` 加 NUL 和前 108 B 的 HMAC。App 只在本机生成，不把 Admin secret 送往 Gateway。`07` 携带 `newBinding(88) | MAC(32)`，由 Gateway 持久提交后一次返回，MAC 为新 Key 对 ASCII `blinker.wifiprov-rekey.wifi-permit.v1` 加 NUL 和 newBinding 的 HMAC；`07` 和 `09` 的精确成功回包为 `version | opcode | 0 | operationId(16)`。本次原 owner 消亡后不得以旧许可开放 Wi-Fi；新 owner 仅可按持久记录只读协调。
- App 复用原设备页的显式提交，只有内部 cloud-beta 构建传入非秘密 `BLINKER_WIFIPROV_REKEY_OPERATION_ID` 才选择试点，限 Android／Development／BLE，且不与 BC-5 候选混用。GetInfo `0x08` 只用于匹配候选：缺失立即停止，不创建暂存、不触发通用 Rotate 或 WiFi-only。原 App 默认、普通首装及其他通道不切换。账号和 BLE owner 校验分开，正常 Wi-Fi 终态断开不抹掉官方回执。

### 5.2 当前边界和最短验收

- Gateway 受限路由已接线，独立 PostgreSQL 16 的 064 最终候选 **4 pass／0 skip**，本地路由／兼容／密码学及迁移 **19 项通过**。未开启远端路由或迁移业务库。
- App 原页、协调器、SecureStorage 单卡 CAS、原 Admin 云版本 metadata CAS 已接线。协调器／报文／HTTP／状态 **69 项**，原页面／安全存储／Admin CAS／环境 **49 项**，匹配 Angular 模板与类型检查通过。旧固件零写；匹配夹具按顺序完成一次官方调用，正常终态关闭本地 owner 仍保留回执。只读重入沿原操作核对，不重新发放 Key、不补发 Wi-Fi；设备官方确认与本次云上线单列。没有因测试编排替换普通页面，也没有把宿主测试当真机。
- 设备端在原 Core 3.3.11 核对真正的同步 Security1 vtable／NimBLE 请求边界，使用当前连接代际而非复用的连接号授权；原 SDK 的三个窄包装入口可重定位。新增 08／09 Admin 验证、07 和原 SDK 一次 Apply 围栏、独立只读 NVS 句柄及预约 journal，定向宿主测试 **7／7**。开发候选未注册到普通 Product。匹配构建仍须让实际 `Esp32WiFiBLEProduct` 装配该 owner、挂设备专用 NVS journal、在回调门禁和根 CAS 下启用能力位。启动 Cloud 前还须以**本次 Apply 成功 + 配网 owner 确认退出 + 持久状态匹配**区分正常换网与重启后用旧网登录，不能仅以 App 时间戳或 `ApplyConsumed` 判成功。
- 上述装配完整链接通过才开放单设备入口、安装匹配 App，并仅用 COM11 作一次实测。若任一步结果不明停在该步并只读核对。全擦后新卡、原卡保留和 SoftAP 的策略不变；本批不刷板、不变更运行服务或推送。

## 6 ESP32-S3 专用产品候选与上线因果验收（2026-11-04）

### 6.1 冻结输入与冷预检

- COM11 继续原保根身份；只改 app0 的计划不包含 NVS、分区、任务或授权清除。内部 sketch `tests/internal/arduino/Esp32WiFiProvRekeyCandidate` 与原 `power`／`uptimeSeconds` Manifest、原 SDK 3.3.11、PubSubClient 2.8、`XIAO_ESP32S3:PartitionScheme=default_8MB`、HTTPS/MQTTS 保持一致。仅在私有构建目录放入旧候选的配网码输入，不嵌入 Wi-Fi 或 DeviceKey；使用 `--jobs 1` 和三个明确的 linker wrap。标准示例／默认发行不选择此 profile。
- 内部 sketch 的串口 `r` 只在首个 `begin` 前申请一次原有的 10 分钟窗口，`b` 进入普通冷启动，`j` 只读输出非秘密状态。无清网络、清根、复位或自动开窗入口；真正的授权仍在产品、native owner、Admin 和云端准入，不由串口等待或字样提供。
- 129 测试机只读已确认原 Gateway／Broker／PostgreSQL／Redis 健康，实际业务库最高迁移 062，新路由及旧 BC-5 默认关闭，直连和测试 HTTPS 均返回 404。隔离库的 064 回归不等于已部署；本批不打开入口、不部署、不更改账号及日志。完成唯一完整链接后再安排同源 Gateway／APK 和单 ID 准入。

### 6.2 App 本次上线证据

受限原页只在这一次前台调用记录官方 Wi-Fi `accepted` 后，才沿用已有结果页做有界只读观察。真实 native 清理结束后，以新版本／locator、同实例和 `WiFi 发出时刻 < 云会话签发时刻 ≤ State 时刻 ≤ 观察时刻` 判定；绝不从旧会话上的新 State 或历史 `accepted` 推出成功。实际 Apply→设备配网 owner 退出→新版本可登录的因果边界由匹配固件保证，时间对照只做防串批检查。Cloud 确认失败保留官方 Wi-Fi 结果，不补发、不自动进设备；成功只启用原显式入口。定向 **84／84** 及 Angular 模板／类型检查通过，未安装。

### 6.3 设备产品装配和运行边界

限定 profile 已把固定命令、真实 Security1 owner 和写前护栏接入原 `Esp32WiFiBLEProduct`／`Esp32WiFiProv`，只在保根的显式 BLE 窗口宣告 `GetInfo` 0x08。窗口内不执行旧 Install／Bootstrap，读取 GetInfo 也不会授予写权限。先验证已有实例、唯一 Admin／原 Key，并用实际 `bl_v2_tasks/snapshot` 的 NVS 只读检查拒绝任何已有任务 scope（即使是 0 条任务）；共享根和服务端 scope 继续由 Gateway 同一事务拒绝，不通过清除本机任务取得准入。

只有精确的一次 Apply、原 SDK 成功、SDK 管理器确实退出、新 Key 可从独立持久句柄读回、新 Wi-Fi profile 的 generation 增加且已确认后，才在专用 `bl_v2_rekey/attempt` 中持久记 `Complete`。冷启动发现中间状态、错实例／根、错 generation 或不完整的持久记录，不启动 Cloud。直接复用相同 Wi-Fi 配置不会增加 generation，不属于本次实际换网验收。设备宿主定向 **7／7**，覆盖终态与错 intent 等反例。

原 SDK 完整编译必须使用现有公开 Cloud profile。首次直接引用原源码树在 TLS 编译前置失败（普通源码是占位配置）；未放宽检查。隔离快照仅替换为原已验候选中逐字节核对的三个生成头 `OfficialConfig.h`、`Esp32ServerKeys.h`、`PortableServerKeys.h`。续编纠正了仅内部 sketch 调用开窗函数的命名空间，未改产品默认、SDK 或其他工作树。

### 6.4 完整链接通过，尚未刷写

唯一完成的固件候选以原 Core 3.3.11 完整链接生成 ELF／map 与 app BIN：**1,389,120 B**，SHA-256 `7afb069f88fcb951d123d593b9718be55c35e43d03b4aa18d880bda1afd7b348`。链接报告 Flash 1,388,968 B／3,342,336 B，静态 RAM 57,956 B。分区 BIN 与原 BLE 候选逐字节一致；实际 map 中 SDK `manager.c` 消费 `__wrap_protocomm_security1` 和 `__wrap_network_prov_config_data_handler`，原 NimBLE 对象消费 `__wrap_protocomm_req_handle`，无 SoftAP scheme。产物只保留于既有私有构建目录，未刷板、未安装 App、未部署、未推送。

下一批只冻结 129 实际运行基线上的 Gateway 063／064 必要消费者和同一 operationId 的 Android 内测 APK；新鲜核原 COM11 实例、Admin、版本／locator、无任务或共享根及无未确认操作。三端匹配后仅刷候选 app0、保留 NVS，配合单 ID 受限窗口做一笔真实换网并记录新版本和新 Cloud 会话。任一结果不明确只读核对；云端版本已提升后，不通过还原旧 NVS 或旧 Key 来伪造回滚。

## 7 三端同批冷预检（2026-11-04）

- 本批只使用一个内测 operationId；保留在三端私有批次清单。手机原 Development 会话、原 owner Cloud 卡、本地全权限 Admin 与 Gateway 新鲜读回相符。卡片版本／access epoch／Admin 版本均为 1；原 App 自动化及任务请求库存在，作了只读摘要基线、不导出 token 或 Key。COM11 ROM 只读核对实例、Admin ID、locator 的摘要与 App／Gateway 全部精确一致；原 `bl_v2_tasks/snapshot`、`bl_v2_rekey/attempt` 均不存在，Wi-Fi 为原确认 generation 1。未开窗口、未清根或刷写。
- **实际 COM11 分区以新鲜 ROM 为准。** 原 `7afb…` 候选虽能完整链接，但它的双 OTA 表不等于这块板的 `factory @0x10000/0x500000`，不直接使用。实读 NVS 区仍是 `0x9000/0x5000`；使用原 SDK 工具生成完全相同的 `factory` 分区表，仅对隔离的匹配候选编译。设备最终验收若具备条件，只允许写原 factory 应用区，不写分区表、NVS 或 bootloader。
- 目标网络须与原已确认 Wi-Fi 配置真实不同，且新网络能达到本测试 Cloud。用户已指定不同的目标网，手机主动扫描已观察到其 2.4 GHz 信号；手机和电脑留在原网。最终仍须在本次配网页由 COM11 扫描命中后才提交，不以手机扫描代替设备可见性。
- 测试 Gateway 在现网 062 编译基线上只冻结 063／064 与配网实际消费者，路由先保持关闭。只有在本次没有发行候选、没有持久提交时，才可以退回不认识 rekey 围栏的旧运行镜像。出现 `key_issued` 或 `committed` 后，关入口、撤销私有准入，但保留理解 064 的 Gateway 及完整日志，不释放或重放原 intent。

### 7.1 匹配产物与已完成的原位核对

- COM11 `factory` 版以原 Core 3.3.11 完整链接。分区表与实读逐字节匹配（3072 B，SHA-256 `cd5fe1dc9f9013a71a7d4a5ce5ec8aab4d1a8f424511ff9e7fdb09543eb58e92`）；应用 BIN **1,389,120 B**，SHA-256 `47d48dcc0d88f1bbf0b4e23ca4aec562c035c2fdffddbe2af0a0b4d93141e64c`。真实 ELF／map 核三处原 SDK 消费者、BLE-only，原双 OTA 候选不覆盖。只读临时分区＋NVS 转储中的含凭据副本已删除，设备原存储未改。
- Android 内测 APK 原签名和包名，匹配单 operationId、原 WiFiProv 2.4.4、无 BC-5 配置；SHA-256 `c811fb48cc7d22af40ab68d8f81b1ff7b1b9ef7d4a32070dbffd1ecf3247c06c`。仅一次原位安装，实机 APK 摘要一致；原账号、卡片、Admin、两类请求日志摘要前后一致。已打开真实重配页确认试点 BLE 入口，未扫描板或下发。构建前一次 pnpm 隔离自检影响主目录依赖链接，已按原锁离线恢复（esbuild 0.28.2），源码和锁不变；最终使用不自动安装依赖的隔离队列。
- Gateway 冻结 10 个必要编译消费者，隔离回归 **19／19**；真实 062 只读核原 owner／tenant、原 Admin、版本／locator 和实例一致，活跃共享、任务、普通待定换 Key、相关注册待定均为 0。部署及准入结果与真机结果分开记录。

### 7.2 闭门部署与测试网络切换

129 的最小 Gateway 已闭门部署并健康（实际新镜像 `7734daf4c2e0e094…`、业务迁移 064、其余 22 容器不变）；Nginx 原配置不变、没有准入变量，精确路由直连及 HTTPS 都是 404。变更前配置、日志和数据库备份已保留且独立校验。第一次本地等待超时只丢失回包，通过远端结果与运行态只读协调得知成功，未重复部署。此时无配网请求发行或提交。

电脑／手机随后切网，旧出网证明作废。用户确认改用当前网络「带翅膀的蜗牛」，手机已看到同 SSID 的 2.4 GHz；已重读同账号、卡片版本 1、原 Admin 和日志摘要，均未变。新来源仅用本次真实详情请求的代理日志关联，继续使用原 operationId；目标仍须由 COM11 本次扫描命中才提交。

### 7.3 一次真机提交：停在 App 请求发送前

- 仅写 COM11 `factory` 应用区一次，独立校验通过；分区表及完整 20 KiB NVS 刷前后逐字节一致。一次原 BLE 窗口内用匹配内测 App 选中真实实例，实际 Security 1 连接成立，COM11 自动扫描返回 14 项并找到指定目标网。Android 原扫描适配只传 SSID／RSSI／安全类型，界面信道为 `—` 不是没扫到；本次目标由只支持 2.4 GHz 的 COM11 实际返回。不因采集脚本要求可选信道而再扫一次。
- 只给当前来源、原 owner／逻辑卡及本次 operationId 开试点。错 operationId 和 Gateway 直连为 404，无 bearer 为 401；实际手机的匹配 `read` 返回 200／`no-store`／`data:null`。真实页面只点一次发送按钮：原实例解析和 Admin 读取均为 200，保留 `prepare-unknown` checkpoint 并回收 App BLE 会话；没有设备安装回执、Wi-Fi Apply、官方 Wi-Fi 回执或新 Cloud State，故本次不算换网通过。COM11 只读 `j` 仍在该窗口，无网络、无 Cloud 和控制写入；窗口自然到期会停配网，不会自动连回原网。本轮不复位、不重开窗口，原 NVS 仍保留。
- 同一真机 WebView 使用 `data:` URL 离线证实：全局调用成功、把原生 `fetch` 作为 Gateway 实例属性调用抛 `TypeError: Illegal invocation`、由全局对象转发调用成功。原单元测试只注入不检查接收者的替身，未覆盖真实默认路径。新增默认接收者回归先复现失败；最小修正仅为默认适配显式通过 `globalThis.fetch(...)` 转发，仍沿用原会话、超时、响应验证、CAS 和 UNKNOWN 保护，定向 **74／74**，绑定匹配 WiFiProv 源码的 App 类型检查通过。不改 SDK、固件、Gateway 鉴权或密钥流程。
- 已恢复原 Nginx 并移除试点私有准入；HTTPS 及直连双 404，同一 064 Gateway 健康。隔离于 App 的只读终核：本 intent 无持久行，原卡 v1／原 Admin 不变，旧 Key 仍有效、无新版本会话或凭据失效事件。保留原 checkpoint 和完整日志，不重开本次旧 intent。下一批复用匹配固件及闭门 Gateway，先为已确认未发行的 checkpoint 接可审计的收尾，再安装匹配修正版 App，以全新显式意图验收精确安装回执、版本 +1、一次 Apply 和新鲜 State。若服务端已发行或结果不明，继续保留 UNKNOWN，只读处理，不按未发行分支收尾。

### 7.4 已确认未发行的旧意图收尾（2026-11-04）

在真实 064 库以只读事务复核旧意图不存在，未发行、未提交，原卡 v1、旧 Key 及 Admin 不变，无新版本会话；旧入口在 HTTPS 与直连均为 404，私有配置已移除。手机同账号、同设备、实例、根及请求日志摘要与原基线一致。只在此闭门且独立可证的情况下，对这台测试手机做一次范围固定的维护：在设备列表离开原重配页，以原 checkpoint 完整字节、原 scope、`prepare-unknown`、无后续阶段及新鲜服务端证据摘要关联，先写原账号下的专用归档并读回，再仅释放活动槽位；复读归档、活动槽位及原 Admin 后确认。旧 UNKNOWN 的原始内容、时间及日志未删除，未重发旧请求、未写数据库。这不是为其他 UNKNOWN 提供通用清理入口。

新意图另有独立作用域及单次提交标记。候选仅包含 §7.3 App 修复，沿用原签名、WiFiProv、闭门 064 Gateway 和 COM11 原应用；不开旧入口、不刷板。手机和电脑仍在目标网络，手机已看到同名 2.4 GHz；设备须由本次实际 BLE owner 自己扫描命中。等待 APK 构建与原位核对后，再开一次板端窗口、一个受限云端入口；结果不明仍停止并只读协调。

### 7.5 全新意图单次 COM11 BLE 换网（2026-11-04）

- 修正版内测 App 唯一隔离队列通过，原 SDK／签名／包名／HTTPS 保持不变、无 BC-5，包内只有新意图及修复后的默认 Fetch 调用；APK SHA-256 `992684e80e749e30d4fd835886a4df799cc7bbd18c76168b3873b62ea310c750`。仅一次原位安装并核对实机摘要，原账号、设备卡、Admin、旧 UNKNOWN 归档及任务／自动化请求日志摘要均不变。COM11 仅普通复位一次，不进 ROM、不刷板，使用原有固件一次开窗，真实 Security 1 会话下由设备找到「带翅膀的蜗牛」。
- 新来源按安装后真实 `no-store` 详情请求关联；只开放新意图，同一手机的受限只读预检返回 200／`data:null`。真实页面一次显式提交，`prepare`、`issue`、`confirm` 各一次 200。App 只从精确 `02 05 00 | operationId | MAC`（51 B）提取安装回执，Gateway 验证绑定同一意图的新 Key MAC 后才提交；未保存或输出回执材料。之后同一 owner 的原 WiFiProv 进度中 `config-sent`、`config-applied`、`success` 各一次；从点击到 `config-sent` 约 3.49 s、从发送到官方连网成功约 7.41 s。持久 checkpoint `wifi-accepted`，未重发配置。
- App 结果页同时显示官方 Wi-Fi 确认与设备上线；新鲜 `no-store` 复读确认同卡、实例和 Admin，Key v1→v2、locator 变更，旧 UNKNOWN 归档及原请求日志仍一致。Gateway／Broker 独立终核、关窗及设备只读状态单列，不用 App 的在线提示代替服务端证据。
- 终态后即关闭试点，恢复原 Nginx，移除新准入和临时 marker，直连／HTTPS 双 404，同一 064 Gateway 保持健康并留存完整开窗日志。板端随后一次只读 `j` 证实 Wi-Fi 就绪、无控制写入，但该瞬时采样为 `Starting / NotConnected`、`cloudReady=0`；不将此前的本次新鲜 State 扩大为持续在线验收。未再次复位、开窗、下发或读取原始存储。
- 关窗后独立 PostgreSQL／Broker 只读核对通过：旧意图仍不存在；新意图 `committed`，同卡 v2、原 Admin 不变，新 Key 生效、旧 Key 已撤销，旧会话 0 活跃／12 撤销，Broker 失效通知已送达。同一实例的新 v2 会话与 Broker 最新 State 的 session 精确一致，服务端时序为持久提交 → 新会话 → State。手机与服务器时钟未校准，不直接比较两端原始毫秒来证明因果；App 必须收到并验证 `confirm` 响应后才调一次原 WiFiProv，服务端则独立验证提交、新会话和 State。本批 BLE 短验通过，不据此扩大默认准入或宣称 SoftAP 已验；无刷板、重放、自动重试或提交推送。
