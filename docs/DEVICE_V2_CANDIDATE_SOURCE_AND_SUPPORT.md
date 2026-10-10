# Device V2：S2 候选源码、依赖与支持边界

M1～M5最新结果：名称与编辑已由匹配新版App、Gateway迁移050及两普通WiFi设备完成本机验收，版本与边界取[主线§8.86](DEVICE_V2_NEXT_EXECUTION_PLAN.md#886-第八十六批m5本机新版app与两设备产品验收2026-10-10)和[App交接§9](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md#9-第86批m5命名与编辑的当前本机候选)。下文第83／84批的来源、摘要和升级边界保留为历史；其中“当前”仅指当批状态，§3.1旧APK／schema49组合不是M5的匹配版本，不能用于读取新命名历史或无感回退。

更新：2026-10-10，第83批交付索引收口。**当前匹配选择只取§3.1；升级、暂停和回退只取§7.5。** 第82批A2软件＋两普通WiFi有限实物已收口，本机保留匹配A2 App/Gateway/Broker、试点停用/历史保留，原板恢复、临时入口撤除完成。旧A1 App/旧P1包不能作为保留typed历史环境的替代App。正式profile/签名、独立接收与缺环境项仍待办；本批只核既有字节及操作合同，不新建合成包、重建/部署或改变默认开关，详见[主线§8.83](DEVICE_V2_NEXT_EXECUTION_PLAN.md#883-第八十三批匹配组件索引与升级回退交付收口2026-10-10)。以下第81～76批增量均是历史，不把旧“下一步”当当前指令。

最新用户确认与结果（2026-10-10）：**本机真实App最小自动化用户闭环已通过，取[主线§8.84](DEVICE_V2_NEXT_EXECUTION_PLAN.md#884-第八十四批本机真实app最小自动化用户闭环2026-10-10)。** 复用本索引匹配组合，经真实Android控件完成新建/启停/两设备联动/退出后执行/查询/退役；新增维护DOM回归，不新建制品或扩大平台声明。§7.6内部交接保留为后续，不是继续开发的前置；正式profile/签名/生产发布暂停。该交接节“不复开入口”只限定当时静态交接，本轮已按用户授权临时恢复受限入口并收尾，不变更正式默认。

第81批（2026-10-09）历史增量：第38项A2-3公开合同与App软件收口，正式runtime源码配对支持A1/A2，未组装端口默认A1。最终Service315节点、App40项、合同对照59项及隔离PG134节点各自零失败/跳过；App/spec类型与严格模板检查通过，详见[主线§8.81](DEVICE_V2_NEXT_EXECUTION_PLAN.md#881-第八十一批a2公开合同app与原意图恢复软件收口2026-10-09)。**当批未部署、未安装APK/操作MCU，新源码不能继承旧289项/镜像/S4实物证据**；其后A2-4由第82批完成限定实物。原依赖/协议/业务schema不变，无第二执行器或发送器。页面当前创建/启停/退役/恢复，已有条件编辑器不在本包；原准入状态保留历史，不当作新runtime现状。

第80批（2026-10-09）历史增量：第37项/A2-2内部底座关闭，完整V2两worker/五用途签名/PG/Command组合、并发恢复及公开版本边界通过；23文件软件309节点及隔离PG125节点分别零失败/跳过，详见[主线§8.80](DEVICE_V2_NEXT_EXECUTION_PLAN.md#880-第八十批a2完整v2软件组合并发恢复与公共版本收口2026-10-09)。**当批私有typed准入只用于隔离候选，production/公开A2关闭；新Service源未纳入冻结289项/镜像或S4**。原镜像/业务schema不替换，测试schema残留0、临时容器已移除；旧来源和验收声明保留，不将软件fixture当MQTT/MCU实物或混版本无感部署。其后公开/实物取第81～82批，不因软件检查重建全部候选。

第76批（2026-10-09）历史增量。**P1新目录构建来源的公开CA变体完成Android/XIAO有限差异实物；普通配置仍关闭。** 当批原无CA包、旧S4/P1不覆盖，手机原APK/数据已恢复，运行时/插件/锁未改；不代签接收。合同取[共同方案§3.4](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#34-p1候选冻结与使用边界2026-10-09)，详细取[主线§8.76](DEVICE_V2_NEXT_EXECUTION_PLAN.md#876-第七十六批新目录p1公开ca变体与有限实物闭合2026-10-09)。第82批之后手机保留匹配A2 APK，不能沿用这里的原包恢复声明；iOS/Wio及生产/T2限制不变。

第71批（2026-10-09）历史准备记录：新R4已独立备份并完成冻结ZIP标准BLE安装编译，但上传握手失败；原Flash完整读回一致，当批BLE兼容性用例未开始。手机已恢复原APK/原授权/缓存，账户MQTT恢复ready，但WiFi见证设备离线，不算Cloud设备控制通过。后续R4/Cloud补证见第72～73批，不覆盖该失败；详见[共同方案§7.7](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#77-第71批r4兼容性准备及烧录阻断2026-10-09)。

第72批（2026-10-09）补充：隔离profile纠正打包测试公钥笔误后，R4正式认领、缓存命中/缺失、一次Property/Action、真实认证取消/只读重进取得有限实物证据；首次错误公钥认领/恢复扫描失败及一次未到认证的GATT失败保留，不称连续无失败或稳定提速。窗口内健康账户MQTT保持同一会话，Cloud设备前后控制仍待测。原App恢复/数据保留；COM4仅拔插并核对身份后，R4原应用245,760 B已恢复/读回匹配，未刷bootloader/桥接或清授权，未宣称NVS全恢复，见[共同方案§7.8](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#78-第72批r4有限实物公钥纠正及验收边界2026-10-09)。打包工具/fixture修正而原433项冻结输入、ZIP及S4原样保存；**旧普通WiFi交接包内Ed25519测试公钥为误编码值，不得继续作为当前服务BLE认领包**。新BLE测试变体不是正式profile或S4替换，后继候选须单独列出摘要/验收，P1默认仍关闭。

## 1. 三种身份分开

| 身份 | 含义 |
| --- | --- |
| Git HEAD | 已提交来源；不能代表外部profile、插件所选工作树字节、工具链或当前实际部署。当前三端HEAD取主线§2，不沿用历史“未上传”状态 |
| 冻结源码＋锁/profile＋摘要 | 指定输入可校验；不含运行secret/完整外部工具链 |
| 实际安装/部署制品 | 只适用其真实验收范围；新构建不能冒充原已测制品 |

App保持一个账户MQTT，设备复用原会话/BBP/2；没有新增连接/发送器。用户仍安装**一个Arduino库ZIP**，不按MCU/模式拆包。Python/Node操作只供维护者，初学者不需要执行源码交接。

## 2. 同源材料

以下是历史S2/S3冻结材料，不是最新Git/已部署状态；当前组件选择取§3.1，不能直接把旧设备ZIP当当前BLE认领包。local-maintenance不是Arduino安装产品。

| 模块 | 冻结物/结果 | 边界 |
| --- | --- | --- |
| Arduino普通WiFi | D:/codes/local-maintenance/core-arduino-source-20261008，433文件；core-arduino-rebuilt-20261008/artifacts/BlinkerDeviceV2-0.1.0.zip | 新目录与第62批实测ZIP逐字节一致，无需因工具变化重刷相同字节 |
| App＋5插件（第63批历史） | D:/codes/local-maintenance/core-app-source-20261008，1,639文件；core-app-rebuilt-20261008 | 完整Android构建成功，但npm锁版本与已测工作区不匹配；仅保留诊断，不准入/不安装 |
| App＋5插件（第64批） | D:/codes/local-maintenance/core-app-source-s2b-complete-20261008，1,642文件；core-app-rebuilt-s2b-complete-20261008 | 已测版本/frozen install/16步Android/391项相关回归通过；无现场测试CA、未安装，不冒充实物候选通过 |
| Gateway/Broker | D:/codes/local-maintenance/core-service-source-20261008/context，289文件及manifest | 与第62批来源逐文件一致、含迁移001～049；核对镜像ID/source标签，本批不重建/部署 |

设备ZIP SHA：`3ea54b5269459a7b4841ab41f3e43a1801b1e059aa9da349bedf1853c4d3abfc`，690,345 B。profile=development/automation-a1-lab-20261008，仍为第62批本机服务地址、明文Cloud session/MQTT及显式plain storage；**无task_profile**，不能测试T2。地址/公开build-test验签材料不是生产信任根，无用户Key/WiFi密码。

第62批手机**实际安装**APK SHA：`ee6a0786509d156b901f2e2c5bfb511b2b21c73a570706d096b314bcfd7d9653`，有独立测试CA overlay。本批新目录普通构建不继承其私有路径、不安装/冒充原包。App source manifest记录六仓HEAD及每文件SHA，包含必要未提交源码。

Service输入SHA：`544e431558e842a4ab9b6f5e56c738686f0edf6efc7b0b3cf871c0e894819635`。已测Gateway镜像`sha256:59a2ba7a9ac5b36a09a8e2b699af9d37920652d9feb55681b47c1fde93559ca0`、Broker`sha256:e0f5a2c024e0fee9b892c5381bb44021b43fb13c5350493730eb600777c52446`。固定Node base为`public.ecr.aws/docker/library/node:20-bookworm-slim@sha256:2cf067cfed83d5ea958367df9f966191a942351a2df77d6f0193e162b5febfc0`。这不是OCI归档或镜像层逐字节重建证明。

运行.env、5用途配对secret、TLS私钥、业务库备份、手机账户/授权及现场fixture不进入交接包；不得分发第62批整个私有目录。Auth/Kong/Controller/Redis不是这289文件的闭包。

## 3. 支持与准入

| 能力 | 定位/入口 | 限制 |
| --- | --- | --- |
| 普通WiFi | C3冻结核心；Blinker.h、Key→session→MQTT | ESP32/UNO R4/Wio各既有基线；默认明文、显式TLS可选；非所有型号/SDK |
| BLE Direct | C3冻结核心；BlinkerBLE.h、认领/Method2/安全record | XIAO S3/UNO R4既有实测；不删鉴权、不外推Wio正式BLE |
| ESP32 WiFi+BLE | C3冻结核心；BlinkerWiFiBLE.h、App WiFiProv | 同一逻辑身份/名字；未提供手动混合重载，R4无混合模式 |
| T2任务 | 封闭owner试点；development task_profile＋显式enableTasks＋DEVICE_V2_TASK_ALLOWED_DEVICE_IDS | 4槽/单power/XIAO S3普通WiFi；本次普通ZIP未准入。operator同片/写额度实物、磨损仍缺 |
| A1自动化 | 有限同owner/同片；Gateway/Broker配对DEVICE_V2_AUTOMATION_ENABLED及5用途配置；默认关闭、缺配置拒绝 | bool rising→fixed bool，两普通WiFi；4次ACK/回调/有限恢复不等于生产/长稳 |
| A2条件 | 第38项公开六字段write/closed-union/App＋完整V2底座及匹配本机两S3下降沿/uint32阈值有限实物已过；原观察/PG/Command复用 | §3.1匹配组、本机shard1/同owner；试点已停用，非完整条件/全平台/生产准入，不包含在旧289项/S4中；旧A1不能无感回退 |
| P1 BLE参数 | 显式Android实验；S3/R4有限控制/取消及健康Cloud/BLE并行只读已测；新目录构建＋匹配CA变体Android/XIAO有限实物闭合，普通构建关闭，不在原S4 App中 | iOS实物/Wio preview、独立接收待办；无CA原包保留，有限样本/中断/冷却限制不变，不称正式支持/稳定提速或扩P2 |
| LAN WebSocket | internal preview；App deviceV2LanEnabled默认false及匹配LocalAccess | ESP32 self既有证据，不是全部WiFi产品已接线 |
| Hub/子设备 | 可选模块preview | 不扩多child/新南向；生产秘密保护门禁未补齐 |
| Wio BLE | 暂不正式发布 | 原长重连/单连接占用等限制保留 |
| 分享/通知 | 条件开放；匹配Auth/ACL、requests/shares、通知各开关 | 页面存在不等于环境启用；双账号验收不能由打包替代 |

源码存在、编译、部署、启用、验收不是同一个状态。不改现场开关伪造默认状态；第82批本机Gateway/Broker shard1为匹配A1/A2，两条规则均停用、8条历史执行保留。这个上次收尾快照不冒充本批重新查过现场或所有实例已升级。

### 3.1 当前匹配组件索引（第83批收口）

维护者必须明确选哪一组字节，而非笼统说“最新版”。当前普通A2本机联调匹配组如下；**这是既有组件索引，不是新合成S4包或正式发布**。本批实际重核ZIP/stage、App source/APK与Service context；实物及安装/部署结果仅引用第82批，不重跑或代签独立接收。单库ZIP仍只有一个，不能按MCU/模式拆安装产品。

| 组件 | 精确来源/摘要 | 当前可以声明与限制 |
| --- | --- | --- |
| 普通设备单库ZIP | [第82批ZIP](../build/automation-a2-live-20261010/artifacts/BlinkerDeviceV2-0.1.0.zip)，690,345 B；SHA `09900bfd355165bee5e063fea2b187df5014dab6e0cfc647752dbbd49814bbd7`；[公开profile](../build/automation-a2-live-20261010/profile.json) SHA `daaf515fd67122a15260520c2c867ceb9779c886c4c30f991971e58940aeb9db` | 416文件与stage同字节、标准安装ESP32 WiFi clean编译与两XIAO fixture有限实物；相对第73公钥修正ZIP仅OfficialConfig.h变化，415文件同字节。没有本批独立device source-bundle/重建证据，不能称新版七目标/T2/S4全闭包 |
| App＋5插件冻结源 | [core-app-source-a2-20261010/manifest.json](../../local-maintenance/core-app-source-a2-20261010/manifest.json)，1,646项；SHA `9fa45279444874820350083a81cc2953d06753179476684752c73be66c73140c` | 包内原verify重新通过，新目录原锁/插件/模板/Web/types/sync及Android已构建。所选插件README/tests字节含当时未提交输入，不能拿Git HEAD代替manifest；iOS/完整外部SDK离线闭包不在内 |
| 匹配普通Android debug APK | [第82批APK](../../local-maintenance/core-app-rebuilt-a2-20261010/source/blinker-app/android/app/build/outputs/apk/debug/app-debug.apk)，79,959,579 B；SHA `66b47aaa416085561759be7fdd0358f8dcd775cd10b4a5a5899fa8c101c42e5d` | 第82批实际安装/有限实物/读回；local,local-device，P1=false/LAN=false、原HTTPS origin。仅USB本机HTTPS公开CA变体，非生产签名/任意网络一键APK；当前保留此包，不用旧05071或旧P1覆盖 |
| A2 Gateway/Broker冻结输入 | [context](../build/automation-a2-live-20261010/context)＋[inputs.json](../build/automation-a2-live-20261010/inputs.json)，291项；记录聚合摘要 `15477d2be5775f085b54ad44fc7c6c8e97f97dcca6ec9a72d13a287da5e0fab8` | 各文件重新比对完整一致，含Dockerfile/npm锁/迁移001～049；第82批隔离恢复/匹配镜像/有限实物。不是完整Auth/Kong/Controller/运行secret闭包；构建不等于部署 |
| 匹配Service镜像身份 | Gateway `sha256:1bd321f51654243da56e6a783ff5fbaf50971701e3cd9421a3f62691a1777b28`；Broker `sha256:57501d068e588bea3995491ebcc3b24206ccc15c5a166ae3b666497d65d7b681` | 本机镜像ID及第82批部署证据，不是registry拉取地址、OCI分发归档或位级重建承诺；原5用途配置及schema49配对，只有参与链路的shard1通过有限实物 |

设备profile仍指向历史测试`192.168.1.142:17002`、明文session/MQTT及显式plain storage，且无task_profile；**第82批结束已移除该临时入口，不能直接复制Key就声称这组字节现在能上线**。使用前维护者须明确新的可达服务/profile与配对下发，再按实际差异准入；本索引不自动恢复监听、改网络或把测试公钥作为生产根。

App公共debug overlay只单独选原[AndroidManifest.xml](../build/automation-a1-live-20261008/android/AndroidManifest.xml)、[network XML](../build/automation-a1-live-20261008/android/res/xml/blinker_local_network.xml)、[CA PEM](../build/automation-a1-live-20261008/android/res/raw/blinker_local_ca.pem)，不得复制其父build目录。CA证书指纹`27:29:CD:BF:78:1F:DA:55:13:0D:B2:CC:16:E6:79:9E:78:17:4B:5D:45:90:60:FC:12:29:36:FD:4B:A8:A5:61`，有效至2027-01-06 04:34:19 UTC；匹配实验leaf原记录有效至2026-11-07 04:34:19 UTC。APK接入`https://127.0.0.1:17000`、账户WSS `wss://127.0.0.1:18885/mqtt`需操作者审阅的USB reverse和有效证书；本批不重新验网/证书链，接收方不能拿旧validTo替代实际连接/验链。

分发只选上述明确组件/源码manifest/公开overlay及脱敏说明，逐项审核；**禁止整体分发automation-a2-live或其他现场build**，其中有私有固件fixture、Flash/手机数据/数据库备份和日志。SHA是完整性而非发布者签名；ZIP/stage结构排除现场私有文件是白名单核对，不是通用秘密扫描。当前普通匹配组保持P1关闭；如需A2＋P1须从匹配源另行评审差异，不安装旧P1包伪称已组合验收。

以下第74～76批组件选择保留为历史比较，不是当前保留A2历史环境的App推荐：

| 组件 | 当前可使用范围 | 不能混同 |
| --- | --- | --- |
| 原S4整包，§5.4 | 历史完整性/普通核心验收依据；其中App `05071e…7606`默认不含P1 | 旧设备ZIP `3ea54b…abfc`含错误Ed25519测试公钥，**不能直接用于当前服务BLE认领**；README保留原样，不当最新操作说明 |
| 公钥修正设备组件，§5.5 | 第73批普通development WiFi组件 `1e05de…23c1`；433项输入及原新目录重建证据保留 | 是旧实验地址，不代表当前网络可达、完整BLE/七目标新版门禁或T2/生产profile；第82组另列，不覆写旧包 |
| 独立P1 App候选，§6及App交接 | 1,646项输入；开启包 `1a5b64…861c`，同源关闭对照 `2dba09…a988`；仅匹配USB HTTPS测试环境 | 不包含于原S4、不替换其App/接收记录；原工作区构建不等于交接工具可新目录重建P1 |
| 第75批P1独立构建证据 | 新1,646项快照 `2918f2…dd5c`，新APK `f54b78…4209`；只改维护工具/测试，16步独立构建及41项相关回归通过 | 普通P1仍关闭；无测试CA/未安装，不等于上一行旧APK同字节、实物通过或完整S4交付 |
| 第76批匹配公开CA实物变体 | 同一新快照/隔离work，另输出APK `cebd57…2b85`；Android/XIAO已绑定缓存、一次Property/Action、退出、认证取消/只读重进通过，手机原包已恢复 | 明确USB HTTPS测试CA，非生产/独立接收/全平台P1；不覆盖上一行无CA原包或旧S4/P1，不称DEX/整APK同字节 |

第74批当时八项manifest/APK/ZIP摘要一致，原S4全部2,376项完整性和两个App快照原verify通过；当时1,646项App/五插件输入与P1冻结源匹配。P1相对原S4 App快照**新增4项、修改9项、删除0项**，五插件及依赖锁未变；逐项差异在`build/p1-candidate-closeout-20261009/audit.json`，不是另一个发布manifest。原交接工具当时对三种P1配置均安装前拒绝，无工作目录/依赖下载；不倒写第75批后的工具准入或当前A2源码。

上段为第74批原快照事实。第75批新快照在[core-app-source-p1-clean-20261009](../../local-maintenance/core-app-source-p1-clean-20261009/manifest.json)，manifest SHA `2918f27d044404dda2726f052c99737edc0f2e122fb937723c22e63e5433dd5c`；相对原P1仅`tools/source-handoff.mjs`及其测试不同，运行时/五插件/锁不变。包内工具完成新目录16步构建，新[无CA APK](../../local-maintenance/core-app-rebuilt-p1-clean-20261009/source/blinker-app/android/app/build/outputs/apk/debug/app-debug.apk)79,949,462 B，SHA `f54b7881d08c3d4eaa1dbe7656d5ac74e159f1c1b55f6cd9126ff35410b34209`。实际依赖/冻结锁、插件dist/声明TS/Android main及HTTPS/P1导出核对通过；12项工具/新目录41项相关回归0skip。原八项制品未变，**该无CA包没有安装、不覆写**；详细构建取[主线§8.75](DEVICE_V2_NEXT_EXECUTION_PLAN.md#875-第七十五批p1显式维护入口及新目录android构建2026-10-09)，不倒写原S4/P1清单。

第76批[公开CA变体APK](../../local-maintenance/core-app-p1-test-ca-20261009/app-build/outputs/apk/debug/app-debug.apk)79,951,127 B，SHA `cebd57ecc8f497a7df1bc77c3d5a4b31d4ece3866832fc6d6dba4df090c22b85`。显式复用冻结公开overlay，实连验链/端点、原签名/HTTPS origin、源码字节、Web/native及编译App Java核对通过；资源生成的项目DEX不同，不称全DEX一致。Android/XIAO新包控制/退出/认证取消/只读恢复通过，17项测量工具0skip，原3条任务intent/空自动化journal摘要不变；原App读回恢复、账户ready、零连接/观测，无MCU/服务器修改。证据和有限时延取[主线§8.76](DEVICE_V2_NEXT_EXECUTION_PLAN.md#876-第七十六批新目录p1公开ca变体与有限实物闭合2026-10-09)。这只补新构建来源的发送方实物链，不替独立接收或原其他门禁。

后继合成交付如果需要，先明确选用源码/profile、网络/公开信任材料及开关，再新建manifest/制品、按实际差异验收。**现有单独组件校验不能替合成包验收。** 缺iOS/Wio P1环境不阻塞关闭P1的普通候选，但原平台准入/T2/A1限制依然有效；接收方记录、默认启用和生产/Git/部署权限分开。

## 4. 重建入口与外部责任

设备沿用tools/build_arduino_handoff.py export/verify/rebuild，仅扩为普通或显式T2的development profile，生产仍拒绝。旧task-pilot manifest兼容验证，新包kind为wifi-source。导出前重建并比参考ZIP；profile漂移/依赖版本不符拒绝，不替换实测参考。PubSubClient2.8源码/许可证随包；Python3.11+、CLI1.5.1、stockESP32 3.3.11及编译工具外置。本包只闭合ESP32 WiFi依赖，不称R4/Wio/BLE外部SDK全打包。

App按[交接说明](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md)。五个file依赖保持不变，无私有registry/monorepo；JDK/SDK、Gradle/Maven传递依赖和iOS尚非完整交付。SHA检查非发布者签名，分发前仍检查源码误写凭据。

Service context具备Dockerfile/npm锁/迁移，可用记录base构建Gateway/Broker；npm/base可用性、运行secret与部署批准独立。schema049账本不能盲回灌043“回退”；暂停优先配对关试点并保留数据，见[Service§27](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#27-a1-5匹配测试部署真实账号app与实物准备2026-10-08)。

## 5. 第63批验收与实际差异（历史）

- 已过App闭包8自测、Arduino交接11＋原打包19自测、普通WiFi同ZIP独立重建、Service289输入/迁移/两镜像来源标签。App新目录16步骤（插件/依赖、模板、类型、完整local Web、Capacitor sync、Android debug）全部成功；Android冷构建14分11秒/439构建任务，不称439测试。
- 新目录经原Angular test builder的5文件**61/61、0skip**通过，范围为自动化client/http/page、持久意图与鉴权拦截。先直接调用Vitest因缺Angular启动/globals而5个suite失败/0测试，随后改用原ng test入口；不改业务测试以迁就错误入口、不累计前批120/776。
- 首轮APK：78,433,858 B，SHA `2afc53c0dcf30c0405672a75839219f5bf17aff983a8572da1b7ad4bb6147245`。只证明此锁可构建；无测试CA继承，未安装，不能替代上一批已测包。

逐个比对发现实际已测工作区与npm锁有**20项直接依赖/开发依赖版本差异**（完整列表为本批build下tested-direct-differences.json）。关键项：

| 依赖 | 已测工作区 | 本次npm锁重建 |
| --- | --- | --- |
| Angular运行时 | 22.1.3 | 22.1.1 |
| community BLE | 8.3.0 | 8.2.0 |
| Ionic Angular | 8.8.19 | 8.8.17 |
| Angular CLI/build | 22.1.5 | 22.1.3 |
| Vitest | 4.1.11 | 4.1.10 |

这不是同一制品，只写package.json/lock“匹配”不能检出实际版本漂移。所选MQTT/RxJS/Capacitor核心版本一致，五个自有插件全部dist逐字节一致且file链接均指向隔离目录；仍不能据此跳过第三方BLE/框架差异。

原锁全精确版本对齐300秒超时；仅20项对齐明确peer冲突；独立目录全精确版本解析180秒超时。均保留失败，未使用force/legacy-peer-deps，没有更改原package.json、package-lock或node_modules；单包registry查询成功，不能笼统归为离线。独立解析目录仅诊断，不是有效候选。

### 5.1 第64批S2b依赖合同与最终源码重建

- App/模板唯一安装入口为pnpm11.19.0 frozen lock，五独立插件各自npm10.9.8 ci并显式build。Node22.22.3、JDK21/SDK36、Gradle8.14.3/AGP8.13.0仍沿原基线；未升级/降级框架或改通信协议。原App npm锁备份为`D:/codes/local-maintenance/core-pnpm-probe-20261008/app-package-lock-before-s2b.json`，SHA `797e63d98691c8cd609962e4c14e73e7f07bd029e42454973a068b3b63de01dd`，移除的是过期锁，不是用户会话/源码；五插件锁及原node_modules不动。
- 初步只补模板importer，隔离tsc暴露@ionic/core缺声明。随后精确声明原App直接Core8.8.17和Buffer6.0.3；Angular组件内部Core8.8.19原组合保留，不把不一致隐藏成“自动升级”。旧lock既有包/peer snapshot条目均保持，只追加Core8.8.17条目与必要importer；旧ntfy Windows `file:..\\capacitor-ntfy`与规范化相邻路径解析相同，不是更换来源。
- 第二次完整构建已过，但扩大测试发现授时golden漏带：31套件中1套件加载失败/其余386项通过。补白名单，公开镜像与设备canonical文件SHA均为`24ab1864fc7dc96025dcbb756b43610255716417362d49f8c66fa19b7e475c3f`。失败原样保留；不改断言、设置外部fixture环境变量或冒充整组通过。
- 最终source manifest SHA `b31393b1c74cb90b805b34902d40dfe2199288ac4bc1b721c868fa6db88efe71`，schema2、六仓1,642输入；**16步骤全部通过**。安装前后四份依赖输入字节不变；实际69项直接版本与原工作区逐项相同，原20项漂移已消除；模板penpal7.0.6/typescript6.0.3/vite8.1.5。新锁SHA `fee8ffd2d11f9204e2bda87ba3518809ce6201bf192c19916678c129eb8404ca`。
- 五插件dist13/13/14/13/13个文件与原仓库构建结果一致，JS/type/map及Android main/build.gradle与原App实际安装一致；旧ntfy安装只缺docgen docs.json，新包补此元数据，不能笼统称全部旧安装目录字节相同。file安装结果只指向新隔离工作目录，无借用原dist/node_modules。
- **9/9工具自测、2/2真实锁失配对照、31文件391/391相关App测试、0skip**。原Angular test builder覆盖BLE认证/取消/退出、MQTT、协议/Manifest缓存/授时、自动化/鉴权/持久意图和Ionic动画。包含先前61项，不累计为452；也不宣称全部App/LAN预览测试闭包已齐。Android4分17秒/439项是构建任务，不是测试。
- 最终APK **79,946,806 B**，SHA `96efef6287b780f6b9234cce824ad6ea15aa09a78f6855803e94467c3545268c`，路径为最终工作目录`source/blinker-app/android/app/build/outputs/apk/debug/app-debug.apk`。没有继承现场debug CA、没有安装，不是原已测APK的逐字节复现，也不是正式release。
- 原始结果在最终目录`rebuild.json`及`D:/codes/blinker-library/build/core-candidate-s2b-20261008/`的`dependency-audit-final.json`、`negative-locks-final.json`、`app-regression-final.json/log`。首次未补依赖/fixture的目录与日志保留；源快照冻结不改，不把重试结果写回旧失败记录。设备同ZIP/Service289输入及镜像来源复用第63批，没有本批重新刷板/部署证据。

### 5.2 第65批S3：显式测试信任、origin修正及有限实物

- 当前电脑/手机YYSC、主机192.168.50.13；ADB与三串口在线。只更新debug App，没有写MCU、清授权、部署Service、迁移业务库或执行Git暂存/提交/推送。原APK和私有账号数据备份留在本机，不加入源码包或Arduino ZIP。
- 显式采用第62批公开测试CA，SHA-256 fingerprint为`27:29:CD:BF:78:1F:DA:55:13:0D:B2:CC:16:E6:79:9E:78:17:4B:5D:45:90:60:FC:12:29:36:FD:4B:A8:A5:61`，有效至2027-01-06。原生overlay只含manifest、限定127.0.0.1/192.168.50.13的网络配置及CA公证书；现场TLS链/地址验证通过，未关闭验证或加入私钥，release不采用此overlay。
- 首个HTTP诊断APK：79,948,471 B，SHA `650dabdc93ddcb97e1a8de9f35b563c8fa0d8272aef42eb2ccd95d10b63fe52a`。独立输出构建4分21秒/439构建任务，1,580个Web资产与第64批普通APK相同，CA字节/更新签名通过；原普通APK、报告和冻结源码未覆盖。安装后原账号及Native BLE授权仍在，完成冷缓存/命中/显式删单条缓存后三轮BLE控制：ready3378/3316/3331ms、按钮计数0→1→2→3，属性/动作返回及真实State相符；退出219～231ms、早退218ms后观察6.4秒连接仍0，原账户MQTT保持同一会话。**这些仅为HTTP诊断包证据，不自动继承到后续HTTPS包。**
- S3发现第64批工具自动设置BLINKER_LOCAL_CLEARTEXT，导致WebView从原HTTPS变为HTTP。账号凭据没丢，但localStorage缓存与IndexedDB写入意图按origin隔离；新origin中的pending=0不证明原意图不存在。只移除维护工具自动设置，保持默认HTTPS；增加环境隔离回归。没有改Capacitor运行时开关/协议、复制journal或清数据。恢复旧HTTPS APK后旧Manifest缓存重新可见，原A1规则仍停用/pending0；未执行A1写入。
- 修正后冻结源码`D:/codes/local-maintenance/core-app-source-s3-https-20261008`仍1,642输入，manifest SHA `f486e00ab1c4be87a7ef7da88a004ed34b254651359bc2cad8efd3c105ee019b`。与第64批只差维护工具及其测试两文件，App业务/五插件源码不变；69项实际直接版本及模板版本零差异。冻结包10/10工具回归通过，复用第64批391项业务测试，不谎称本轮重跑391项。
- 新目录`core-app-rebuilt-s3-https-20261008`完成15步安装/模板/类型/Web/sync，再明确采用公开CA构建Android，共16步骤；原生4分42秒/439构建任务。最终HTTPS APK **79,948,479 B**，SHA `05071e7160333c68f1fb612f26c78cbe8059c352415402ed2de119457bbc7606`，本机备份名`app-debug-https-test-ca.apk`。更新签名/CA原字节验证通过，保留原APK及账号数据，已安装并读回摘要核对；源快照不改，安装事实取独立https-install.json而不是改写构建时installed=false。
- 首轮“全Web资产原字节相同”检查明确拒绝，未跳过：1,580资产中1,574内容相同（含改名但内容相同的map）；4份JS经**精确构建目录、三个chunk引用/hash及debugId注释**替换后完全相同，1份map仅位置/debugId变化、原source content相同，1份Capacitor配置单独验证HTTPS/cleartext=false。根因为SCSS调试注释嵌入新工作路径导致hash传递，不改业务代码追求伪字节复现。初始差异清单保留，不能写“最终全部资产逐字节相同”。
- **最终HTTPS候选实物复测通过**：原账号/Native授权及旧HTTPS Manifest缓存均可见；命中无ManifestRequest，显式备份并移除该一条缓存后重新发ManifestRequest，均经设备Manifest ACK再取fresh State。ready分别3107/3677ms，属性返回323/287ms、按钮返回300/302ms，设备计数3→4→5且State revision递增；没有跨路径自动Action重发。最后仅恢复power=false并确认State，不伪装计数回退。退出126/123ms回收；早退222ms后观察6.4秒，group/link/session/opening均0、无晚到连接，账户MQTT保持原同一会话。
- 最终HTTPS包ME200、原A1规则可读/停用、pending0；账户MQTT ready。但原测试设备Cloud presence均offline，匹配包的有限Cloud等待仍失败为DeviceV2ReadyWaitError，观察已关闭，**未发送Cloud控制或自动重放Action**。历史固件profile含不同旧服务地址，但本轮未读出设备当前网络/登录配置，尚不能把离线唯一归因于地址。下一步先定点核对WiFi/服务地址/Key版本，不改服务架构或盲目重刷所有板。
- 证据目录`D:/codes/blinker-library/build/core-candidate-s3-20261008`含profile、build/install/apk-audit、逐轮gate结果及修订后构建/输入核对报告；该目录也含私有数据备份，**不得整体分发**。Service原规则revision5/disabled、4条已ACK历史/0未确认保持，未重建或启用规则。

### 5.3 第66批S3：普通Cloud控制、重启恢复与收尾

- 现场电脑/手机YYSC，主机192.168.50.13；现有App账户MQTT ready，COM3对应普通设备active、DeviceKey版本2。只通过原App管理接口查看原Key，未创建设备、刷新Key或清授权。原恢复镜像app0/NVS均含旧SSID且不含YYSC/当前Key原文，证明其不是本轮相同明文参数，不证明当前有效启动/加密存储的唯一根因；本轮不重构协议或服务。
- 仅操作已授权COM3、MAC `8c:bf:ea:8e:f1:2c`。先新鲜读取64KB系统区，再与历史静态字节重构整8MB并在设备校验全部范围，备份SHA `5725d8073b4913451aaddf7c53beedc9d30a043345e5a8c2c82c64f9118e625e`；备份不是直接信任过期NVS。COM11/12没有打开串口或刷写。
- 同一§2普通ZIP的416项文件与实际编译库逐字节一致；433项输入清单包含维护工具/依赖，不与416个Arduino ZIP文件混称。标准`WiFiQuickStart`仅替换私有Key/SSID/密码，仍公开Property/Action、`Blinker.h`和同一`begin/run`；无任务profile、未链接新安全/自动化模块。SDK stock3.3.11，Flash **942,976 B**、静态RAM **51,296 B**；应用bin **943,120 B**，SHA `6fcd43c381e2e88b3d32e2e3b0cd09910c3b2d46aefb72c7db12a30b3fd57bc0`，仅写入并校验原app0起始0x10000。
- 继续使用第65批实际安装HTTPS APK，设备读回SHA仍`05071e7160333c68f1fb612f26c78cbe8059c352415402ed2de119457bbc7606`，未重装、清数据或更改origin/CA。真实Cloud首次ready **227 ms**、Property ACK **287 ms**、按钮ACK **1104 ms**；设备State确认power=true、按钮 **0→1**、revision **1→2→3**。最后power=false/revision4；ACK与实际State分别验收，非幂等按钮仅一次，不自动重试。
- 普通复位后的第一次观察仍收到旧revision4/计数1，**不算新启动ready证据**，该窗口未发控制。新revision1/计数0相对首次观察 **37,375 ms**、相对复位工具完成 **38,582 ms**到达；无需清授权/重配网或额外复位即恢复。此后重新有限观察，Property ACK **182 ms**、State到revision2；最后power=false/revision3，计数保持0且按钮没有重放。功能恢复通过，延迟原因未分阶段测量，不归因于某个SDK/MQTT环节、不宣称无缝或低延迟；这是已知待优化，不开100轮掩盖它。
- 恢复原固件/NVS/授权只写`[0x9000,0x150000)`，**1,339,392 B**，SHA `3b1f96098254da193bc52b5fe5d7f681c54f7cd62f14ca9bbe8fa06df466d416`。保持ROM直到整范围device verify通过后启动；bootloader/分区/其他应用不写。恢复原配置后COM3 Cloud再次offline如实记录，不能把候选成功冒充旧镜像在线。原账号/同一个账户MQTT仍ready，有限观察全部abort/release、group/BLE link/session均0，临时debug gate引用移除；其他两板不动。
- 原A1数据库READ ONLY核对：revision5/disabled、4条ACK/0未确认保持。没有新建/启用规则、部署/迁移、修改生产代码、暂存/提交/推送。普通控制产生原command ledger记录属于正常链路，不称整个业务库零写入；第65批10工具/第64批391业务测试复用，未重跑/累计。
- 证据`D:/codes/blinker-library/build/core-candidate-cloud-20261008`：`backup.json`、`installed-library-audit.json`、`candidate.json`、各轮Cloud gate、`restore-verified.json`、`final-audit.json`、`cleanup-verified.json`。该目录含完整Flash/设备Key/私有网络参数，**禁止整体分发**。首轮ZIP审计因误假定根目录无版本号被拒绝，改成实际`BlinkerDeviceV2-0.1.0/`后416项全部相同；不放宽内容核对。第65批离线失败/旧制品不覆写为成功。

### 5.4 第67批S4：干净交接目录与离线验收

- 干净目录为`D:/codes/local-maintenance/core-candidate-handoff-20261008`，入口[README](../../local-maintenance/core-candidate-handoff-20261008/README.md)。Arduino433、App＋五插件1,642、Service289项源码输入与§2/§5既有冻结清单逐字节相同；单库ZIP和已测HTTPS debug APK摘要不变，没有另导出不匹配工作树或重建APK。原组件清单及三文件公开CA overlay保留；仅新增总清单/只读校验、操作说明、空白接收表和白名单脱敏历史摘要。
- 总清单固定**2,376个文件（不含manifest.json自身）**。由发送方另行提供的manifest SHA-256为`80c35bdd1b8cfa8dd73dc92644a7559ca86ec8ffd980753e54aeea40616850bb`；接收方先从可信渠道核对，再执行可信脚本。清单摘要不是发布者签名，也不能证明真实接收验收或生产准入。
- 复制后直接从新目录运行原Arduino/App verify通过；Service289项context与原source摘要、原清单匹配。普通ZIP在独立`core-candidate-s4-arduino-rebuilt-20261008`工作目录重建，与§2已测ZIP逐字节一致。没有重新下载SDK、编译/刷板、重建Android或部署服务，原10工具/391业务回归和S3实物证据仅复用。
- 总清单只读校验**8/8、0skip**：有效包不改文件，错误外部摘要、修改/缺失/多余文件、越界路径、无效内容摘要和不支持schema均拒绝。夹具在外部build目录，不污染交接物。已知现场DeviceKey/WiFi密码两项分别按UTF-8/UTF-16LE检查全部交接文件及ZIP/APK的3,158条解压文件，零命中；这不是通用秘密扫描器。公证书单独核对fingerprint/期限和无私钥，现场证据整目录未复制。
- 当前电脑网络**mostfun2026、192.168.1.136**；冻结development profile仍为历史`192.168.50.13:17002`，未测本轮网络下可达性。更换SSID不会自动迁移服务/信任配置；需要新profile时新建变体、核对监听/下发/信任并只做必要差异门禁，不能覆盖历史候选或旧失败证据。
- 本批原件/冻结输出校验前后不变，未操作手机/开发板、清授权、刷新Key、改服务/业务库、暂存/提交/推送。只作有限内部交接；接收方独立验收、生产profile/签名/信任根、远端最小CI、iOS、原缺环境及长稳仍未通过。外部本批记录为`build/core-candidate-s4-20261008/assembly.json`、`checks.json`，不放入Arduino ZIP。

### 5.5 第73批公开公钥修正组件交接及有限并行短链

- 新维护组件目录[core-arduino-ed25519-fixed-20261009](../../local-maintenance/core-arduino-ed25519-fixed-20261009/source-bundle/manifest.json)，433输入；manifest SHA `eb3fa720bd2eb811f76b7413d5ba503fc02aa2731cae24cf0c54846b3ee1b6e0`。从新目录原工具verify及重建通过，与修正参考ZIP逐字节一致；11项交接工具测试0skip。原S4整包不覆盖、不改成“已接收”。
- 用户安装仍是一个[Arduino ZIP](../../local-maintenance/core-arduino-ed25519-fixed-20261009/artifacts/BlinkerDeviceV2-0.1.0.zip)，690,345B、SHA `1e05deba4f1965d28db7f128706cc64d39343d826587cea8e0028ff191b423c1`；相对旧ZIP仅`src/BlinkerV2/arduino/config/PortableServerKeys.h`公开Ed25519笔误修正。source-bundle相对旧433输入另有profile、development fixture、profile校验工具和任务交接文档四项不同，不混同用户ZIP差异。development验签材料不是秘密，也不能作为production信任根。
- **仅普通development WiFi组件交接范围**，不称完整BLE/跨MCU新版安装门禁；保留原实验网络`192.168.50.13:17002`，不是当前网络一键部署包。当前现场独立overlay为`192.168.1.146:17002`/新版本标记，只在私有build用于COM3；不分发overlay、现场Key/密码/Flash。已知COM3 Key的明文/UTF-16LE检查零命中，不是通用秘密扫描。Python/SDK仍只需维护者使用，Arduino用户安装ZIP/写Sketch即可。
- 本轮临时网络下，416项安装源与独立stage一致、公开WiFiQuickStart真机上线，程序942,976B/静态RAM51,296B；三次Cloud新StateRequest/StatePage及COM11的既有BLE链路保持同一原App账户MQTT。没有Property/Action、本轮无需重测C3；不能以该overlay成功宣称本段旧地址组件包可达或所有模式准入。1,646项App/P1源输入摘要零差异，原APK/板卡均已恢复，详情取[共同方案§7.9](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#79-第73批健康cloudble只读短链及恢复2026-10-09)。
- Gateway回滚时发现旧地址端口无法bind：最终镜像/环境/原卷恢复且健康，只移除此前实际未生效的明文发布映射；不是完全相同的HostConfig、服务升级或ACL变化。临时无状态代理已删，持久卷不动。公开组件包不含这些私有运行配置，证据`build/p1-cloud-compatibility-20261009/audit.json`不得连同整个build目录分发。

## 6. 交接剩余与当前执行顺序

- **第27项S3完成；第28项S4发送方干净目录与离线校验完成，接收验收待实录**：接收方复制§7.4模板到包外、核对实际网络/profile并独立安装/短链验收。没有接收记录时保持待验收，不再重复生成检查批次或代签；不重测已过Cloud/BLE、刷全部板或重启服务。初始HTTP包只留诊断，不用跨origin空journal证明恢复；38.6秒复位窗口保留为待优化，不扩写为无缝通信/生产准入。
- 仍未通过/不承诺：生产profile/签名信任根、全SDK离线字节闭包、iOS、远端最小CI闭包、生产部署/长稳及上表缺环境实物。
- 五插件运行时未变，不因打包清授权/重配网；A1仅核对原停用记录/只读恢复，不重新建规则凑次数。无变化MCU/Service不反复刷写/部署；构建问题修构建层，不改通信协议。
- 第68批用户明确允许先回到既有P1，有限实验取[共同方案§7.4](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md#74-第68批p1控制兼容取消与短窗口实物2026-10-08)；只修硬件测量工具/测试，无App运行时、插件、MCU或服务变动，不重新导出/覆盖本交接目录。当批下一步是主线第30项有界可选策略候选，现已完成；不能把P1实验当S4独立接收通过。
- 第69批完成该有界App候选、217项相关软件回归及显式开启APK有限Android/S3门禁；另存1,646项输入和新APK，不替换§5.4。测试后手机已恢复原匹配APK及缓存/授权；普通配置仍false，收益未定案。第68批“下一步”成为历史，现按主线第31项小规模同候选A/B判断，不用更大循环或新架构替代。Docker仅按用户追加请求清理无引用历史Gateway/Broker镜像，现用/Compose镜像、容器/卷保留；不算服务升级/重新部署。
- 第70批完成有限配对收益判断：同输入关闭包`2dba09450cda1bfd0bd960d524a11af0585d30f6d1944c41bd1f4d2a93eca988`与第69批开启包；6有效样本GATT后中位配对减少271ms（16.3%），整页中位2733→2631ms。两次预热中断/最后四次安装新增35秒冷却完整保留，不称严格连续实验或正式准入；11项分析工具自测、原输入/实际参数/控制和恢复核验通过。手机再次恢复原APK及授权/缓存，普通配置仍false。第31项转为已完成的有限收益判断，下一项32仅最小兼容性，R4/Wio/iOS/健康Cloud和S4接收不冒充完成；不改冻结目录、生产源码或部署。

第74～76批已完成第33～35项，P1新目录公开CA变体已有有限实物、无CA原包仍未安装；iOS/Wio与独立接收保持待办，普通配置仍false。第82批A2限定实物及第83批匹配索引见§3.1/§7.5；旧历史“下一步”不重复执行。§5.5仅公钥修正组件；没有生成合成新S4包，也不把当前A2来源/实物证据覆写旧S4，独立接收第28项仍待实录。

第67批仅离线交接整理；第66批仅定点写入并恢复COM3，未清授权或修改其他板，第65批不操作板卡的历史事实保持。第73批用户许可COM3临时profile后恢复，COM11/COM4不刷写，网络回滚差异明确记录。未暂存/提交/推送、不清开发者原文件；维护者源码/测试日志不放用户Arduino ZIP。

## 7. S4最小交接操作说明

本节是内部候选的操作合同，不代表生产发布或另一位接收方已独立验收。当前选定组件取§3.1，第82批A2结果取主线§8.82，升级/回退取§7.5；§5及原S4只留对应历史。失败诊断包不得替代候选。

### 7.1 Arduino用户只安装一个库

1. 维护者先提供匹配服务地址/公开信任材料的**单个**`BlinkerDeviceV2-0.1.0.zip`。当前普通A2测试组件取§3.1的第82ZIP/hash，但其临时入口已关闭；§5.5/§2旧ZIP保留原历史。先明确可达服务/profile，不能让用户安装后自行拼内部composition，也不以Git占位地址或已修正公钥等同正式上线。
2. Arduino IDE“项目→加载库→添加.ZIP库”安装；也可解压为Arduino用户库目录下的一个`BlinkerDeviceV2`目录，根部须有`library.properties`、`src`、`examples`，不能重复嵌套。不要同时保留旧Blinker库或另装按模式拆分的同名包。
3. 安装对应开发板Core及`library.properties`声明的依赖。当前SDK/版本基线取§4；外部SDK未全部包含在ZIP中，Arduino用户不运行Python/Node维护工具。
4. 从“文件→示例→BlinkerDeviceV2”选模式，仅编译一种入口。组件全局/长期存活，显式传入`begin()`；`loop()`只调用`Blinker.run()`，采样使用`onSample()`而非不断上报。

| 场景 | 标准例程/头文件 | 用户配置与上线 |
| --- | --- | --- |
| 手动WiFi | `WiFiQuickStart` / `Blinker.h` | 在App创建普通设备、复制现有DeviceKey，填入示例Key/SSID/密码；`begin(key, ssid, password, components...)` |
| 纯BLE | `BleQuickStart` / `BlinkerBLE.h` | `begin(components...)`，App正常发现/认领；不以MAC决定身份，不把WiFi Key硬塞进BLE初始化 |
| ESP32 WiFi+BLE | `WiFiBleQuickStart` / `BlinkerWiFiBLE.h` | `begin(components...)`，App BLE WiFiProv配网并下发接入材料；配网与Direct安全会话不同，同一逻辑设备/名字 |
| 诊断/采样/授时 | `CloudSmokeTest` / `Blinker.h` | 仍是普通WiFi，额外展示可选业务授时、租约采样与按需诊断，不是第五种连接模式 |

接口/简例取[库README](../README.md)，不向用户暴露composition、workspace、MQTT对象、存储槽位或`official_detail`。无手动混合模式重载；UNO R4/Wio不按ESP32混合模式套用。

### 7.2 绑定、Key和重置不是一个操作

| 操作 | 实际影响/用户动作 | 不能声称 |
| --- | --- | --- |
| 普通重启或保留存储的重新烧录 | 沿用现有身份/授权与可用网络配置；手动WiFi仍应保留正确Sketch参数 | 重启等于重新绑定或刷新Key |
| 手动WiFi丢失Sketch/完全擦除Flash | 从原设备页面重新查看、粘贴当前DeviceKey和网络配置；不必仅为丢失代码新建设备 | 当前Key是数分钟过期的一次性Ticket |
| App刷新DeviceKey | 用同一设备的新版本Key更新手动Sketch；旧Key取得的Cloud登录材料也应失效 | 旧固件仍可永久凭缓存登录，或刷新就是换DeviceId |
| 仅重新配网 | 对支持模式使用`resetNetwork()`并检查返回，清保存的WiFi信息、重新进入配网；保留身份/Key/Direct授权 | 清网络等于解除云端绑定/分享或抹除身份 |
| 本地授权维护重置 | 对支持模式使用`resetAccess()`并检查返回，失效本地访问材料；按App正式认领流程恢复 | 所有模式均支持、同时清WiFi、必然删除BLE bond或云端设备 |
| 云端owner移除设备 | 走正常owner删除；在线本地退休回执与云端撤权分别核对，离线清理保持未知 | 云端成功就证明离线MCU已清理所有本地状态 |
| 被分享用户退出/owner撤销分享 | 只取消该用户对此设备的权限，owner与其他分享不受牵连 | 退出分享等于owner删除或MCU整机重置 |

库不自动给所有板设置统一物理重置按钮；产品Sketch可把支持的维护API接到显式按钮/确认流程。普通WiFi若配置写在Sketch里，修改网络首先修改Sketch，不假定支持WiFiProv。擦除Flash、清授权不是连接失败的第一步；任务试点的未知根/退休限制仍取[任务交接](DEVICE_V2_TASK_PILOT_HANDOFF.md)。

### 7.3 最短排障顺序与维护边界

- 依次确认：选对板/入口及服务profile → 实际2.4 GHz WiFi与IP/服务可达 → DeviceKey版本 → session/MQTT连接 → Manifest接受/fresh State → 控制ACK与真实状态。App账户MQTT ready不等于设备在线；恢复旧测试固件也可能恢复旧网络，不能立即归因于协议故障。
- 默认日志静默；必要时在`Serial.begin()`后调用`Blinker.debug(Serial)`，或像CloudSmokeTest一样按需`Blinker.printDiagnostics(Serial)`。保持串口读取，避免循环同步输出阻塞`run()`；不要记录Key、WiFi密码、bearer或完整接入报文。
- 默认普通设备Cloud明文，显式TLS可选；这不等于BLE授权/安全record可以跳过。BLE附近≠已认证连接，Manifest描述能力≠App页面布局；App继续使用一个账户MQTT。
- App重建取[源码交接](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md)，保持HTTPS origin与明确测试CA；Service迁移/配对配置取[自动化部署边界](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md#27-a1-5匹配测试部署真实账号app与实物准备2026-10-08)。维护者只交付白名单源码/公开profile/摘要；整个现场build目录含私钥、设备Key或数据备份，禁止整体分发。
- 接收方需另记录自己的安装/短链结果；本文不代签接收验收。生产profile/发布签名、远端CI、iOS、缺环境实物及长稳仍取§3/§6，不为补齐文档而重新部署整组服务、扩网关或重测已过BLE。

### 7.4 实际交接目录与接收步骤

1. 原[干净目录说明](../../local-maintenance/core-candidate-handoff-20261008/README.md)和§5.4总清单仍可只读校验，但它是历史A1包、不含P1；旧设备ZIP不能用于当前服务BLE认领，旧App不能读取保留A2历史。当前匹配组件只按§3.1选，不改原manifest、不把混用组件合称已接收的新包。Arduino用户仅安装明确选定的一个ZIP，不安装整个交接包，无需Python/Node。
2. 维护者在新工作目录运行包内原工具重建；不要在冻结源目录安装依赖或写配置。App维持HTTPS origin，debug测试CA只限定原实验端点；Service仅构建源码，不含完整运行环境/账号/用途密钥。不是任意网络一键部署。
3. 将[ACCEPTANCE模板](../../local-maintenance/core-candidate-handoff-20261008/ACCEPTANCE.md)复制到包外填写，明确实际profile、板型/依赖和网络可达，再记录安装/短链、ACK与真实State、退出/恢复及已知失败。缺环境填未测，非幂等Action不自动重放，不返回Key/密码/私钥/Flash/业务库。
4. 接收方实际网络、可达服务与信任配置须重新核对，不能拿发送方历史结果或相同SSID替代联网验收；当前设备profile的临时入口已撤除。本批没有重新探测现场网络。接收人、实际环境与验收记录未取得前，保持S4待接收状态；生产发布仍另行准入，不自动扩新功能或重跑历史门禁。

### 7.5 匹配候选升级、暂停与回退

此处提供有限维护操作合同，不新增安装器、runtime管理器、发布开关或第二套状态。适用当前同owner/tenant/shard普通WiFi封闭试点；生产多客户端维护窗口、正式profile/签名必须另获准入。**规则停用、试点开关关闭、业务历史兼容、设备恢复、App回退是不同事项。**

#### 升级顺序与准入

1. 先选§3.1完整匹配组，核来源/hash、实际设备profile/地址与服务下发/信任是否一致；不要用`0.1.0`同名文件、Git提交或镜像tag代替精确制品。当前本机A2组合已经通过且保留，不为说明再次部署。原设备BBP/2 wire没有因A2而变，只有库profile差异，不要求给MCU增加规则执行器。
2. 升级前由真实账号停止试点并暂停新建/启用/编辑操作，确认enabled0、prepared/dispatching/outcome_unknown0及App原pending状态。未决存在就先按原ID查询/恢复，不切镜像、删日志或改ID解锁。单操作者测试可以显式停止管理操作；生产有并发客户端而没有维护期写入隔离时，不能宣称可安全滚动升级，也不在本文凭空承诺已有维护锁。
3. 保留Gateway PG、参与Broker SQLite/原配置及App APK/必要私有数据，设备确需刷写时再按精确身份/范围备份。服务间5用途secret分别配对，不放App/设备ZIP；备份先隔离恢复核对，不回灌现场覆盖新账本。schema49与A2源码版本不是同一概念，本次无新迁移不等于任意旧应用可读新JSONB。
4. 新App解析A1/A2，允许先更新匹配App而保持原origin/签名及账号日志；在服务全部匹配前不得创建/启用A2，不向旧服务降级重发。随后仅替换参与链路的Gateway/Broker为匹配源码，配对原试点准入/5用途配置；其他分片要承接规则时也必须有相同能力/版本配置，不能把只升级shard1写成全站升级。只读核健康、schema/账户/设备能力/规则历史；版本/type/digest错配明确拒绝，不能吞错误猜已支持。
5. 匹配准入后才允许显式新建disabled A2规则、核原ID/CAS/pending，再启用/取得新基线；初值触发区和重新连接不补动作。变更到新服务/profile时只做实际差异短链，ACK与fresh State/回调分开验收，Action不盲重放。原A2-4的4条ACK是发送方有限证据，不代签接收验收或以100轮代替准入。

#### 暂停优先，不能把停用当降级兼容

- 业务暂停优先在匹配组合下正常停用规则、核无未决，然后按原合同停止观察/执行准入；保留rule/revision/typed定义及execution/ledger/unknown历史。停用只阻止后续新准入，不能召回已发包或把结果未知变成失败；解除授权也不自动清历史。
- 第82批实际旧APK字节/decoder拒绝停用A2规则；旧A1镜像读取non-retired graph也不兼容。旧P1冻结源同样只有A1规则合同，不能作为当前A2 App替代。**没有明确兼容性证明时保留匹配A2 App/Gateway/Broker＋停用状态，不单独回退旧包。** 不给旧实例滤掉typed行绕过共享冲突/额度，不删除A2/unknown或整库覆盖伪造可回退。
- 退役是正常保留历史的业务操作，但不能承诺让原A1回退安全：旧public retired列表仍可能返回typed定义，App/历史execution/账本/读取路径仍需完整版本核验。真正降级须按实际读写范围证明兼容且保留晚到结果与账本，不是“数据库备份在就能恢复”。原A1镜像/APK只作为受控备份/隔离对照，当前没有无感降级承诺。
- 固件/NVS/授权恢复按每块板已验证范围独立进行，不代表App/服务器回退。第82批原板恢复后旧fixture缓存不再fresh；临时代理/端口及ADB reverse均已移除，重新使用测试环境必须重新核可达地址与有效信任，不清Key解决地址错误。

#### 交付剩余与停止条件

- 现在可交付的是明确白名单组件/冻结源/公开profile与上述操作说明，不是无需配置的一键生产产品。正式控制域名/端口/服务下发、公钥/CA信任、App生产签名、功能开关及发布批准未给定，保持待办；不生成伪正式包或令初学者自己修内部实现。
- S4第28项由真实同事在包外填写接收人、实际网络/profile/板型/安装/ACK与fresh State/恢复结果。可复用旧空白[ACCEPTANCE模板](../../local-maintenance/core-candidate-handoff-20261008/ACCEPTANCE.md)的字段，但更新成§3.1所选组件/hash/范围，不把旧manifest/签名或发送方结果冒充新包验收。缺设备/正式配置明确未测；当前A2设备源独立交接重建尚无新证据，旧433文件包不冒称覆盖新profile。
- 已过C3/Manifest/A2软件及有限实物不再扩成新任务包。只有实际输入变化、接收失败或明确产品需求才回到相应模块修正/差异验收；iOS/Wio/P1、T2 operator/写寿命、网关/LAN生产化等保持原独立边界，不为完成索引开启服务器维护。

### 7.6 内部候选交接验收（用户已确认）

本轮只交接§3.1所列的匹配内部候选，不制作正式上线包、不复开已撤除的测试入口、不启用P1/LAN/T2或扩大平台声明。以下是供同事执行的清单，**尚无接收人实录，不是验收通过记录**。

1. **材料选择。** 按§3.1提供单个设备ZIP与公开profile、App冻结源/manifest及匹配APK、必要的三份公开debug overlay、Service context/inputs和本说明。镜像ID仅供核对，不当作可下载镜像；运行secret由接收方按既有合同安全配置，不通过候选包提供。禁止整体复制现场build或发出Key、私钥、Flash、手机数据和业务库备份。
2. **来源与安装验收。** 接收人记录实际收到的路径/摘要、系统与板型/Core/依赖版本；用包内原校验工具核来源，在冻结目录外操作。Arduino只安装一个ZIP并编译其标准示例；App/Service如需重建，按各自原交接入口和外部工具链合同执行。设备第82组没有独立source-bundle闭包，不拿旧433项的重建证据替新profile；没有环境的步骤明确未测。维护者校验工具不是Arduino初学者的使用前提。
3. **联网短链单独准入。** 只有接收方已有明确可达的匹配内部服务、有效公开信任及合法测试账号/设备时，才记录DeviceKey→session→MQTT→Manifest→fresh State、一次属性与按钮的ACK/真实结果、退出和普通重启恢复。A2交接需另核匹配App/Gateway/参与Broker以及原规则/未决历史，按§7.5操作；下降沿/数值用例仅限原A2-4范围，不自动扩权限或平台矩阵。非幂等按钮不盲重试，旧缓存不能代替fresh State。当前17002已撤除、APK依赖USB本机测试配置，因此无匹配环境时本项记未测，不调整源码/网络凑通过，也不要求现在提供生产域名。
4. **回传与收口。** 接收人在冻结包外填写§7.4模板，明确所选组件/hash、实际环境、逐项通过/失败/未测、ACK与State、恢复范围及最终确认。原始敏感日志本地私有留存，回传前脱敏。来源/安装可独立确认，但第28项联网接收未测时不整体关闭；实际失败才形成对应模块的最小修正任务，不重复全量构建、刷板或已过门禁。

当前状态：发送方材料与有限测试证据已具备；用户已选内部交接；接收人、接收方式及其实际验收记录尚未取得。这里没有代发材料、部署、安装、清授权或Git提交/推送。
