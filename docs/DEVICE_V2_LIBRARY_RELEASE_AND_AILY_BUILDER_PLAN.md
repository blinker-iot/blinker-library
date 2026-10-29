# 设备库发布与 Aily Builder 兼容实施方案

更新日期：2026-10-29。当前收口：R1～R5 原受限候选证据保留，LOC-5b 当前定位源码隔离安装／完整链接已完成；下一候选的说明与可选 CI 接线见 §13。

## 1 目标与范围

将现有单一 Arduino 设备库收口为可安装、可编译、支持边界明确的发行物。先处理构建工具对现有实现文件的兼容，不重写设备执行链，不把实验功能作为普通库发布的无限前置。

本方案是设备库发布准备的当前入口。App 历史页失败恢复暂缓，原记录保留；旧七目标 Arduino CLI 编译证据保留，但不替代当前 Aily Builder 或新发布 profile 的验收。

## 2 现状与发布边界

- 单库布局、Property／Action／Event、控制 ACK／State、授时、Sketch 周期／延时 Timer 和租约实时数据已有实现；H1／H2 沿用 State／Telemetry，由服务器归档。
- 当前普通七目标已有 Arduino CLI 隔离安装和链接证据。Aily 有历史成功记录，但当前版本／SDK／配置组合仍需定向验证。
- 公开 `cloud-beta` profile 仅覆盖 ESP32 普通 WiFi、零任务槽 TLS，不能代表全部模式的云端发行配置。
- 普通持久任务默认零槽；ESP32 四槽按逐设备准入开放，R4 固定维护配置为四槽 Plain 有限支持。Wio 持久任务暂缓、BLE 为预览；不扩大原支持声明。
- 不扩槽、不升级 SDK、不刷板、不部署服务、不重跑 BLE 长循环。提交、推送和正式发布另按用户指令执行；本次允许先推送 `i3w-new-sim` 已有提交再切换。

## 3 执行顺序

| 批次 | 最小实施范围 | 完成条件 |
| --- | --- | --- |
| R1 构建兼容，已完成软件验证 | 在 builder 的 `i3w-builder` 分支合并最新 main，补齐 `.ipp` 依赖闭包和缓存输入 | 条件依赖、递归片段、索引及归档失效回归通过；完整 TypeScript 构建和已有测试通过 |
| R2 代表目标限定收口 | 原冻结包两目标编译／链接，R4 初次依赖差异由 R2b 修正 | 原失败保留；R4 新候选严格门禁通过，ESP32 原完整产物加有界差异验证，不宣称新全量重链 |
| R2b 已完成本批有限修正 | 通用公共入口闭包、上下文缓存、`.inc` 输入和薄 TU 分类 | 113 项软件回归、R4 BLE 完整重链与严格闭包、ESP32 单新增空 TU 差异通过；Wio SDK 完整编译仍单列 |
| R3 边界／材料核对已完成，完整云端配置受限 | 六模式／七目标首发计划与 ESP32 WiFi 公开候选分开冻结，见 §9 | 当前云端仅 P-256 任务 signer；缺 Ed25519 业务签发／BLE 公网入口，不生成假完整 profile |
| R4 当前公开候选限定安装／编译已完成 | `cloud-beta` 精确 ZIP 隔离安装；CLI 相同编译输入复用，Aily 两目标完整链接／严格门禁通过 | 普通代码计划不等于全模式云端发行；工具／SDK／profile 证据分开，见 §10 |
| R5 当前候选本机交付准备完成 | 安装入口、目标依赖／旧库冲突、精确版本／校验值及手动候选 CI | 新 7 项有限回归、YAML 语法及 README-only 差异／隔离安装通过；未触发远端 CI 或正式发布，见 §11 |

R1 不顺带重做 Blockly 积木；编译器支持 C++ 与旧积木包适配 V2 是不同工作。性能、OTA、离线历史／Bulk、新网关协议继续后置。

## 4 R1 实现合同

### 4.1 依赖与编译职责

`.ipp` 是被包含的实现片段，应参与本地 `#include` 递归、宏条件及外部依赖发现，但不是新增独立翻译单元。保持现有 `.c`／`.cpp`／汇编编译输入规则。

同时核对两条路径：

1. `analyzeLibraryIncludes`：公开头／有效源码经本地 `.ipp` 发现外部库依赖；未选择的条件分支不能新增依赖。
2. `computeLibrarySourceFiles`：沿 `.ipp` 继续遍历，识别其包含的 `.cpp`，防止片段与包含它的实现重复编译。

只抽取三处共用的文件匹配规则，不增加新扫描器、依赖引擎或平台专用 Blinker 分支。保留各消费者原排除范围。

### 4.2 缓存合同

- LibraryIndexCache 的快指纹和内容摘要包含 `.ipp`。新增、修改、删除片段必须让对应分析失效；新分析版本拒绝旧合同下的索引。
- ArchiveCloudCacheManager 的源文件摘要包含 `.ipp`，修改片段必须改变相应归档键；保持既有 v2 网络协议、地址和校验，不清空用户全部缓存。
- `.ipp` 不计入独立编译文件数或归档资格统计。无片段库和既有工具链隔离合同保持兼容。

### 4.3 有限回归

使用临时目录、隔离缓存和小型 Arduino 库 fixture，不接设备、业务服务或真实远程缓存：

- 公开头 → 嵌套 `.ipp` → 外部依赖，宏开启／关闭及未引用片段对照。
- `.cpp` → `.ipp` → 被包含 `.cpp`，仅真正翻译单元进入编译列表。
- 索引首次 miss、未改命中，以及仅片段修改／新增／删除后的失效；旧分析版本不得命中。
- 归档实际键随片段内容变化，未变仍稳定；仅存在头／片段不能获得编译源资格。
- 原 package 测试集合与 TypeScript 完整构建，保留合入 main 的容量、多源码及工具链隔离回归。

## 5 本批执行记录

### 5.1 分支与合并

- 原 builder 主目录为 `i3w-new-sim`，跟踪文件干净；无关未跟踪内容不纳入提交或删除。
- `i3w-builder` 从 `0c8c895` 最终快进合入远端最新 `main` 的 `ebc367a83145ebc6cf7862cc0302e4b17230988b`，无冲突；包含归档缓存／Arduino 多源码、`undef` 兼容和开发板菜单校验四项上游提交。
- 初次 Git HTTPS 拉取失败；通过 API 发现本地追踪记录滞后，再以单次命令的 Git protocol v0／HTTP 1.1／curl User-Agent 配置成功 fetch，未改全局或仓库网络配置。
- `i3w-new-sim` 没有独有待推提交，已快进到远端 `06edb3d689758b1684cb5da2d193c67403861e32`；正常 push 返回 `Everything up-to-date`，未强推。最终主工作目录停留在 `i3w-builder`；准备阶段新建的干净临时工作树已移除，原有工作树不变。

### 5.2 最小修正

- 新增 `src/utils/LibraryFilePatterns.ts` 集中依赖扫描与缓存文件匹配规则；两种职责仍分别定义，不扩大独立编译扩展集。
- `DependencyAnalyzer` 同时补齐 `.ipp` 本地依赖闭包和片段递归，条件关闭及未引用实现不带入外部依赖。
- `LibraryIndexCache` 纳入片段指纹，将分析版本提升为 `dependency-directive-tape-v3`；旧 path／source 索引按版本隔离，不删除用户缓存。
- `ArchiveCloudCacheManager` 纳入片段内容摘要，归档键自然改变；原 v2 网络协议、地址、资格阈值和编译源过滤不变。
- 新增 `test/library-ipp.test.ts` 六项回归；另外只修正两处上游测试的 Windows fixture：可执行文件扩展名及跨平台路径断言，未修改对应产品行为。

### 5.3 验证结果

| 验证 | 结果 |
| --- | --- |
| 修复前 `.ipp` 六项对照 | 2 通过、4 失败，实际复现漏依赖、重复片段及缓存不失效 |
| 修复后 `.ipp` 六项 | 全通过，包含旧版本双层 guard 与大 `.ipp` 不计入归档资格的对照 |
| 最新 main 合入后，全部跟踪测试加本批测试 | 95 项（含子项）通过，0 失败／跳过；包含上述六项，不重复累加 |
| 完整 TypeScript 构建、新测试静态类型检查、`git diff --check` | 通过 |

验证使用既有 Node 24.19.0、TypeScript 5.9.3 与本地依赖；未安装或升级依赖、未改 package／锁文件。PowerShell 环境未提供 npm 命令，直接调用已有 TypeScript 和 Node 测试入口：

```powershell
node node_modules/typescript/bin/tsc --pretty false
$builderRegressionFiles = @(git ls-files 'test/*.test.js' 'test/*.test.ts') + @('test/library-ipp.test.ts')
node --import tsx --test @builderRegressionFiles
node node_modules/typescript/bin/tsc --noEmit --esModuleInterop --resolveJsonModule --module commonjs --target ES2020 --skipLibCheck test/library-ipp.test.ts
git diff --check
```

测试创建的临时库和缓存均自动清理；不读取真实 SDK、远程归档缓存或设备。原有未跟踪测试不混入本批统计。中间两项 Windows fixture 失败在修正后通过，未跳过或删断言。

### 5.4 下一批

R2 初次冻结编译见 §7，R2b 修正及有限差异收口见 §8。当前下一步进入 R3，先冻结首发支持范围和公开 profile；不重复已通过目标，不将两个代表目标或 Wio 薄 TU fixture 外推为全平台 Aily 兼容。

本批设备库仅更新发布准备文档和入口；未刷板、部署、升级 SDK、提交或推送本批修正。原 App 历史页记录及无关本地修改保持不动。

## 6 相关依据

- [打包与平台矩阵](DEVICE_V2_PACKAGING_CI_AND_PLATFORM_MATRIX.md)
- [发布 profile 合同](DEVICE_V2_RELEASE_PROFILE.md)
- [任务与 Sketch Timer 方案](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md)，普通七目标证据见 §11.23。
- [库使用说明](../README.md)

## 7 R2 两个代表目标隔离编译

### 7.1 固定输入

- builder 为 `i3w-builder` 的 `ebc367a` 加 §5.2 未提交修正；从当前源码定向生成独立 dist，连同实际 Ninja 二进制逐文件记录摘要，不使用旧 dist 或旧产物。
- 设备库为 `c719d29` 当前源码，复用现有发布器和 `tests/release/development_profile.json`，生成同一份 443 文件的内部编译包。普通任务零槽、Plain 编译配置，无真实 DeviceKey／网络凭据；不是正式云端发行包。
- ESP32：`esp32:esp32:esp32`、SDK 3.3.6、`WiFiQuickStart`、PubSubClient 2.8。
- R4：`arduino:renesas_uno:unor4wifi`、SDK 1.5.3、`BleQuickStart`、ArduinoBLE 2.0.2／Crypto 0.4.0。Preferences／WiFiS3 由原 SDK 提供，不手工补 `AES.h` 或旧 preview 宏。
- 工具版本按原 SDK 的 `installed.json` 固定，以 Aily 标准 `name@version` 路径链接到既有工具，不复制、修改或升级 SDK。编译、索引／对象缓存及工具路径均隔离；仅子进程的 USERPROFILE 指向本批缓存目录，真实用户资料和缓存不改。
- 单一顺序队列，ESP32 jobs=1；后续 R4 jobs=2，不重启 ESP32 队列。显式关闭归档缓存和远端获取。全部输入、命令、完整日志与产物保存在忽略目录 `build/library-release-r2-20261027`，冻结 builder 位于 builder 仓的 `build/blinker-release-r2-20261027`。

### 7.2 检查方法与边界

Ninja 只读 `-t compdb`／`-t deps` 用于实际源码、头文件和命令来源检查；`ar t` 核对归档成员，实际 ELF／map 复用既有安装门禁。审计适配层只接受已知的 `BlinkerV2_implementation_` 对象前缀，不修改原始 map，不放宽 transport／平台／任务／可选功能符号要求。

本批 Aily 枚举 16 个 Blinker TU：`BlinkerWioTerminal.cpp` 被原 `isCodeFragment` 启发式排除。该单元在 ESP32／R4 本来不产生实现，不影响本批两目标；但属于声明 Wio Aily 兼容前必须单独处理的缺口，不能将此结果写成 17 TU 全编译或全平台通过。

本次库包另含 122 个 `.inc`，而当前依赖候选与两类源指纹规则只补了 `.ipp`，仍未覆盖 `.inc`。这是实际规则中的发布缺口：片段变化可能未改变对应缓存输入；本批全新缓存不受旧缓存复用影响，但不能据此宣称增量／归档缓存已覆盖全部库实现。列入 R2b，完成本批两个冻结目标后再修，不为此重启当前编译或顺带扩展无消费者的扩展名。

### 7.3 当前进度

以下为 R2 初次冻结结果，原失败保留，后续修正见 §8。两目标当时均以首次新目录编译／完整链接完成，CLI exit=0，没有重开队列、重用旧对象或自动重试；该批严格验收未整体关闭，因为 R4 的最小依赖门禁失败：

| 项目 | ESP32 WiFi | UNO R4 BLE |
| --- | --- | --- |
| 编译／完整链接／BIN | 通过 | 通过 |
| Flash | 实际应用 BIN 1,001,920 B，分区上限 1,310,720 B | SDK size 报告 197,844 B，上限 262,144 B |
| 静态 RAM | 50,920 B | 17,028 B |
| 本机冷编译总耗时 | 3,613.739 s，jobs=1 | 103.054 s，jobs=2 |
| ZIP 字节、实际源码／头路径、归档成员 | 通过，88 条编译命令 | 通过，164 条编译命令 |
| 所选 transport／平台、零槽和可选功能符号 | 通过 | 通过；额外核对 BLE、Noise 与 portable crypto 保留，内部 RPC／heap 候选不保留 |
| 严格依赖闭包 | 通过：Blinker、PubSubClient、WiFi、Network | **失败**：期望 ArduinoBLE、Blinker、Crypto、Preferences、WiFiS3，实际多 PubSubClient |

Flash、RAM 与耗时定义分别保留，不把 ESP32 merged BIN、ELF 文件大小或主机编译时间冒充设备资源测量，也不将两种 jobs 的耗时当工具链性能对比。

内部 ZIP 为 734,167 B、SHA-256 `9b6f29d2e29aa6e74fd66faf3ef1ba457c946e4a857d2c81a41c5348bf885db5`；完整冻结输入清单 SHA-256 `5029dc592d558b0351a4385c6195682a1d9a70c87b9edb3ca5f927a18b19b47c`。逐产物摘要分别保存在两个目标的 `audit` 下，不用文档修改后的工作树冒充冻结源码。

R4 已实际通过 `PortableCrypto.ipp` 找到隔离的 AES／Curve25519／GCM 头并链接对应实现，没有在 Sketch 补 include。额外 PubSubClient 是依赖入口过扫描，不是 Preferences／WiFiS3 引入 MQTT：Aily 将所有库根公开头作为独立 root，因而额外走了 `Blinker.h → BlinkerWiFi.h → UnoR4WiFiProduct.h → internal/WiFiProduct.h → PubSubMqtt.h`，即便 Sketch 只选 `BlinkerBLE.h`。日志与实际 Ninja 同时记录了 PubSubClient 编译／归档／链接输入。

实际 ELF 未保留 `PubSubClient::`、`ManagedMqttTransport::`、`MqttFrameTransport::` 定义符号；原 R4 SDK 的 whole-archive 使 map 仍列出该成员，不能把 archive 输入当作运行图泄漏，也不能据此放宽严格依赖检查。原失败保存在 `uno-r4-ble/attempt1/audit/strict-library-closure-failure.txt`，补充摘要明确标为 `summary-with-closure-failure.json`，不伪造完整门禁通过。

来源审计的 `compile_commands.json` 是从原始 Ninja compdb 经 Windows `CommandLineToArgvW` 转为 arguments 的审计投影，不是 Aily 原生导出；原 `ninja-compdb.json`、`ninja-deps.txt`、实际 ELF／map 均保留。只接受已知归档成员命名前缀，不改原 map 或原断言。

本批未修改生产源码、SDK、App、服务、账号或设备；未提交／推送。既有无关本地修改保留，已通过的软件回归和实物流程不重跑。

### 7.4 R2b 最小实施范围

本节为已执行范围；当前结果和后续入口以 §8 为准。

1. **优先修通用公共入口闭包**：以 Sketch／依赖实际请求的公开头为入口，同时保留真实待编译 TU 的依赖分析。将入口集合纳入索引缓存签名；补单入口、双入口、真实 TU 依赖和切入口缓存隔离用例。不增加 Blinker／BLE 专用分支，不删除共享库来掩盖误扫描。
2. 在同一文件匹配合同中纳入实际使用的 `.inc`，覆盖依赖递归、两类指纹及嵌套 CPP 片段；片段不作为独立 TU。只补有消费者的扩展名，不改归档 v2 网络协议或清空用户缓存。
3. 用薄 TU fixture 纠正 Wio 的误分类，同时保留显式包含 CPP 的去重保护；Wio 实际编译另按其既有 SDK 验证，不将本批两个目标外推为 Wio 支持。
4. 完成有限软件回归后，优先只复核受影响的 R4 BLE 最小依赖图与完整链接。已通过的 ESP32 冷编译不无条件重复；若变化确实影响其输入，先明确差异验证范围。

## 8 R2b 通用入口与片段兼容收口

### 8.1 最小实现

- `DependencyAnalyzer` 仅从实际请求的公共头和待编译 C／C++ TU 分析；移除“没有入口就扫描全部头”的 fallback。强制库无公开入口时仍分析 TU，纯头库无入口时不凭空引入依赖。
- 每个 session 独立记录库入口，遇到同库第二入口继续分析；递归栈按实际头路径去环并在 finally 清理。未增加 Blinker／BLE 特判，未删除共享的 PubSubClient 来掩盖误扫描。
- 规范化入口集合进入现有 `analysisContext`，复用宏签名。缓存 schema 与分析版本均提升至 4；`sourceFiles` 下移到上下文索引，避免另一个入口／宏上下文覆盖先前的源列表。已测的 null 损坏 entry 按 miss 重建，不抛异常或清空用户缓存，不宣称覆盖所有损坏形态。
- `.inc` 与 `.ipp` 同样参与本地依赖闭包、嵌套 CPP 片段去重及两类源指纹；不成为独立 TU。引用实现片段的薄包装单元保留，原显式包含 CPP 的去重保护不变。
- 多 tab／原生 Sketch 继续使用原有 GCC 依赖路径，不复制执行器，也不把本次有限静态分析宣称为完整 C 预处理器。

### 8.2 有限软件验证

新增真实单 ino `preprocess` fixture，覆盖单入口、双入口、TU 私有依赖、跨库循环及晚到入口、同 Analyzer 的 session 清理、入口与宏缓存隔离、空入口和强制库。片段 fixture 覆盖 `.inc` 改／增／删、归档实际 key、嵌套 CPP 去重和条件薄 TU。

修复前分别复现：入口集合 7 项中 6 失败，片段集合 7 项中 5 失败；额外复现源列表跨上下文串用和损坏缓存 entry 抛错。最终全部跟踪测试加本批三文件 **113 项（含子项）通过、0 失败／跳过**，包含 R1 回归，不重复累加中间集合。完整 TypeScript 构建、三个测试文件静态类型检查和 `git diff --check` 通过。仅使用既有依赖，没有修改 package／锁文件或升级 SDK。

### 8.3 匹配候选与 R4 BLE

复用 §7 的精确设备库包、SDK、工具和占位 Sketch，不重新打包或修改设备源码。最终 builder 冻结于 `aily-builder/build/blinker-release-r2b-20261027/final-dist`，新证据目录为 `build/library-release-r2b-20261027`；`inputs-final.json` SHA-256 为 `55b98b0c02b40a9e06f59c64e19ef7399a8b117385a1da3006623838eb41bdc1`。修正损坏缓存 guard 前的准备／预处理记录保留，不冒充最终冻结输入。

R4 BLE jobs=2，全新输出与隔离缓存，关闭归档缓存和远端获取，CLI exit=0：

- Flash（SDK `.text + .data`）**197,836 B**，静态 RAM（`.data + .bss`）**17,028 B**；实际 BIN **197,844 B**，不混用两种大小定义。总进程耗时 97.643 s，不当作独立性能优化结论。
- 164 条实际编译命令；17 个 Blinker TU 归档成员与安装库一一对应，没有重名或片段独立编译。
- 严格库集合为 ArduinoBLE、BlinkerDeviceV2、Crypto、Preferences、WiFiS3；没有 PubSubClient 路径、编译归档或 MQTT 定义符号。原门禁未放宽。
- 原始 map／ELF、安装字节与源码／头路径、BLE／Noise／portable crypto、零任务槽、无可选功能／跨平台／内部 R4 RPC 与 heap 候选等检查通过。
- ELF SHA-256 `3d6edda3c9b03d8090c5d4a794d7f46985a32216a2f03011aad04842af9c0e45`；BIN SHA-256 `eed4ced760a4fa686f1b4ab3c1b236964c3ad51dea43fe2eeff1f3812010d206`。完整逐项结果在 `uno-r4-ble/audit/summary.json`。

### 8.4 ESP32 有界差异验证

只执行最终候选的预处理，库闭包仍为 BlinkerDeviceV2、PubSubClient、WiFi、Network。与 §7 的实际编译命令逐源对比，无源文件减少，仅新增 `BlinkerWioTerminal.cpp`。

对该新增 TU 使用原已验编译参数、相同库字节／工具及相同内容的 response files 单独编译：成功，nm 无定义符号，`.text/.data/.bss` 均为 0。新增对象 SHA-256 `89f270ab13528d8ca968d3c52b8512232f8a63cfa64de46a5bcfd8a6262266fd`；原完整 ELF 摘要仍为 `1c7ccb2b8419220b5b2d2c2c55a716ff996a08b47ce1d6f537fd9f1d2c31f3db`。

这是“原完整产物＋当前预处理和新增空 TU”的有限差异证据，**没有重新做 ESP32 冷编译或生成新的完整 ELF**。原始命令、差异、独立编译日志和节／符号结果保存在 `esp32-wifi-final/delta`。

### 8.5 收口与下一步

R2b 按本批范围收口，R4 初次严格闭包失败已通过通用修正消除。Wio 薄 TU 误分类已有软件修正与 fixture，并在 ESP32／R4 保留 17 个公共单元；**尚无本候选的 Wio SDK 完整编译证据**，不扩大 Wio Aily 支持声明。

下一步 R3 只冻结首发平台／连接模式、环境、公开 profile 及验签材料；若首发声明包含 Wio 或其他尚未验证的 Aily 目标，在 R4 发行矩阵中补其原 SDK 定向编译，不把硬件、持久任务或实验功能自动并入。既有 ESP32／R4 编译和实物不无条件重复。

本批未刷板、部署、改变账号／授权或提交／推送；原失败、UNKNOWN 和无关本地修改均保留。隔离测试临时目录自动清理，构建证据留在忽略目录。

## 9 R3 首发边界与公开配置冻结

### 9.1 本批决定与最小范围

首发普通库**代码计划范围冻结为六种模式、七个编译目标**；同一单库、同一套 API，不拆 Core／Port 包。
这份代码范围不等于当前云端环境全部可用。当前唯一已配置的公开云端候选仍为
`profiles/cloud-beta.json`：ESP32 手动 WiFi／HTTPS＋MQTTS／持久任务 0 槽。
完整多模式云端候选缺 Ed25519 业务签发及 BLE 公网入口，本批不部署服务补齐，也不编造公钥或放宽 guard。

本批只新增公开 profile 使用／选择说明，修正 README 和旧发布文档中的过时当前表述，
补支持范围、版本一致性及 key ring 首项顺序回归。现有 schema 1、生成器和有效 profile JSON 均无需改写，
不增加运行时功能矩阵，不改设备执行链、用户 API、SDK 或构建工具依赖。

### 9.2 普通库代码首发矩阵

| 平台／模式 | 固定 SDK／目标 | 已有证据及下一步边界 |
| --- | --- | --- |
| ESP32 手动 WiFi | 官方 Arduino-ESP32 3.3.6；`esp32-wifi`、`esp32-cloud-smoke`，两例程不是两个模式 | 原 CLI 七目标基线；Aily WiFi 原完整链接＋R2b 有界差异，不冒称新完整 ELF |
| XIAO ESP32-S3 BLE | 官方 Arduino-ESP32 3.3.11／ESP-IDF NimBLE，`default_8MB`；`esp32-ble` | CLI 与有限实物基线；当前 Aily 候选未验，不外推其他 ESP32 芯片或外部 NimBLE-Arduino |
| XIAO ESP32-S3 WiFi+BLE | 同上；`esp32-wifi-ble` | 既有 BLE WiFiProv／Direct 有限实物；当前公开 profile 未开放，本轮不重测 |
| UNO R4 WiFi | Renesas UNO 1.5.3；`uno-r4-wifi` | 既有普通 WiFi／有限实物；本轮 Aily 未验；不以 Plain 任务准入证明该云端 TLS 图 |
| UNO R4 BLE | Renesas UNO 1.5.3，ArduinoBLE 2.0.2／Crypto 0.4.0；`uno-r4-ble` | R2b Aily 完整链接和严格五库闭包已通过，公开环境仍缺 Ed25519 签发／认领入口 |
| Wio Terminal WiFi | Seeed SAMD 1.8.5；`wio-terminal-wifi` | 原 CLI／有限实物；薄 TU 软件修正已完成，新 Aily 候选尚无 Wio SDK 完整链接 |

这是固定代表板／模式，不是 `architectures=esp32,renesas_uno,samd` 涵盖的任意板型全部验收。
既有 CLI 和两个 Aily 代表目标证据分别保留，不跨工具或跨 profile 继承产物证明。
表中的普通 Plain WiFi 基线为 3.3.6；公开 cloud-beta TLS 的原 D1b 基线实际为 3.3.11，
R4 已依据原冻结输入和编译选项纠正二者混用的说明，见 §10.1；没有升级 SDK 或为文案重编。

### 9.3 支持边界

- 普通持久任务默认 **0 槽**；公开 Sketch Timer 为 RAM 内周期／延时，不是 App 管理的断电持久任务。
- ESP32 四槽仍按维护 profile 与逐设备准入；R4 固定维护图的四槽 Plain 已有有限支持，
  不包含任意 Sketch、TLS／BLE／LAN 任务图。Wio 持久任务继续暂缓，不能为首发改为默认四槽。
- Wio BLE 仍显式预览，长重连／单连接占用 DoS 未关闭，不列入六种普通模式。
  Wio 存储继续遵循既有 owner／预留区合同，不承诺独立 QSPI／文件系统驱动任意共存，也不泛禁外部 SD。
- 授时、控制 ACK／State、租约实时复用已验链路；H1／H2 是 Cloud State／Telemetry 的服务端归档，
  纯 BLE 不自行上传云历史。不承诺重启后无外部授时仍有可信 UTC。
- 普通 LAN、网关、OTA、离线历史／Bulk、复杂自动化和新协议不自动并入本次发行必选项。
  用户外围库／回调的任意资源组合、额外持久任务模式仍需自己的支持合同。

### 9.4 公开环境与验签核对

2026-10-27 只读核对运行 Gateway，未用账号接口、验证码或设备请求；两端口只做一次 TLS 握手。
公开结果与完整公钥见[发布配置合同§5](DEVICE_V2_RELEASE_PROFILE.md#5-r3-公开环境核对2026-10-27)：

- `iot.yiyu.pro:443` 和 `mqtt.yiyu.pro:8883` 均通过 `cloud-beta.json` 的 E46 根 CA 校验；
  MQTT 地址仍由认证 CloudSession 返回，不添加第二个硬编码配置源。
- 业务签发仅 P-256／算法值 2／keyId 1，SPKI SHA-256
  `57f69d14769fd9a099c1d4cc11fac64f8389ee975200f9fa65f53e18b58a6025`，与 D4 既有公开值一致。
- 未配置 Ed25519 业务签发；现有任务准入 1 台、Ed25519 子集 0；公网未开放 BLE／Direct 认领，
  LocalAccess 及 Plain 关闭。P-256 私钥仅在容器内用于派生公开值，未导出或改写。
- ESP32 BLE 使用 P-256 65 B SEC1；R4 BLE 使用 Ed25519 32 B。各组 `serverSigningKeys[0].keyId`
  是认领请求值，必须与实际可签发 key 对齐；CA、JWT／Auth 材料不是业务 ring。
- 因此保留受限 profile 双空 ring，不把已有任务 signer 当成 BLE 已开放的证明，不伪造完整 profile。

### 9.5 必要发布配置与有限回归

新增 [profiles/README.md](../profiles/README.md)，作为配置选择、固定版本／SDK、TLS、ring 及阻点的入口；
README 的“所有生产配置尚缺”改为区分当前受限云端 profile 与完整多模式发行。
旧打包资源矩阵顶部标注历史范围，保留其原有数字与失败证据，不重写历史。

新增 `tests/release/test_release_support_freeze.py`，仅做离线范围验证：

1. 六模式／七普通 target 的 FQBN／例程与现有安装验证器一致，无 Wio BLE 或任务目标混入。
2. 当前仅一个公开 profile，固件／库两份元数据版本一致；复用既有 parser 和安装范围检查，候选仍仅两个 ESP32 WiFi 目标、双空 ring、无任务 profile。
3. 两种算法的轮换 ring 保留首项和显式顺序，不能按数字 id 重排；development fixture 仅作回归，不进入公开 profile。

`python -B tests/release/test_release_support_freeze.py -v`：**3 项通过，0 失败／跳过**，
`git diff --check` 通过。另行只读复核了设备与服务的算法枚举值、平台首项消费者及生成器顺序；
独立审阅未发现范围误报或新增配置体系。此前通过的 113 项 builder 测试、SDK 编译和真机流程未重跑。

### 9.6 收口与后续

R3 可收口的是**首发代码边界冻结、真实公开材料核对及受限候选配置说明**，不是六模式已经可以云端交付。
R4 优先对现有公开 ESP32 WiFi 候选核对精确包安装／编译证据；先检查已有证据能否按相同输入复用，
只有输入确实变化或缺证的部分才安排定向验证。全模式候选等实际业务签发／入口合同满足后另行生成，
不让缺 Ed25519 的问题变成重复 SDK 冷编译或资源审计。

本批未刷板、部署、改变账号／授权／TaskStore／scope，未提交或推送；UNKNOWN、完整业务日志与无关本地修改保留。

## 10 R4 当前公开候选安装与编译

### 10.1 输入核对与最小范围

本批仅验 `cloud-beta.json` 的两个公开目标：`esp32-wifi`、`esp32-cloud-smoke`。
首发代码计划中的其余模式未获得新的公开签发／入口合同，不编译假配置或扩大云端声明。

先核对 D1b 原证据：其两目标实际使用官方 Arduino-ESP32 **3.3.11**，
不是普通 Plain WiFi／R2 canary 的 3.3.6。R3 的公开 profile 说明误套普通基线，本批已修正；
沿用各自原 SDK，不升级、不换 SDK 来迁就证据，也不将 3.3.11 结果冒称 3.3.6 通过。

原 D1b ZIP 与当前发布器派生输入均为 443 个文件，无新增／删除，唯一变化为 README：
源码、例程、元数据、三配置头及 `LibraryBuildIdentity.h` 字节完全一致。
因此 CLI 原两目标完整链接可按 README-only 差异复用；本批仍对新精确 ZIP 做全新隔离安装和逐文件校验。
Aily 的 R2／R2b 是 development／Plain 代表目标，不能替代公开 TLS profile；本批只补这两个缺证目标。

### 10.2 单一候选与队列合同

证据目录：`build/library-release-r4-20261027`。使用现有发布器生成一个普通库 ZIP，
不改源码默认头，不填真实 DeviceKey／SSID／密码，公开例程仍保留占位符。

- Arduino CLI 1.5.1 将新 ZIP 安装到本批空 sketchbook；`verify_installed_bytes` 逐文件核对通过。
  只复制已固定的 PubSubClient 2.8 依赖，不下载／升级依赖。
- 复用最终 R2b 冻结 builder；每个冻结 JS 和当前源码摘要在准备阶段复核，未重新编译 builder。
- Aily 使用 D1b 原 SDK 3.3.11 和 `installed.json` 声明的原工具版本，标准 `name@version` junction 仅指向已安装工具，不修改 SDK。
- 唯一串行队列，jobs=2；本批隔离缓存，开启原有本地归档生成／恢复、关闭远程读取。
  只允许工具按原 key／manifest 正常命中，不跨 SDK 搬用 R2 core、不伪造 Ninja 状态或降低校验。
- 两目标分别保留命令、日志、实际 map／ELF／BIN 和审计结果。后目标若正常命中前目标缓存，明确记录，
  不冒称两次独立冷编译；失败后不自动新开重试队列。

### 10.3 CLI 原完整产物复用

可复现审计为 `audit-cli-reuse.py`／`cli-reuse.json`，结果 `passed=true`、`newCompileOrLink=false`。
原 12 项设备证据的大小及 SHA 均匹配 D1b 保留清单；原 profile 摘要与当前相同，
SDK 3.3.11 的 `platform.txt` 摘要也匹配原基线。
两原 ELF 重新执行现有严格库／链接门禁，补核实际 TLS connect、setCACert、handshake／x509_verify 符号及两个配置 CA。
这只是读取原产物和核对相同编译输入，不声称当前 ZIP 又产生了新 CLI ELF，也不是 Aily 证明。

| CLI 既有目标 | SDK Flash 报告 | 静态 RAM | 实际应用 BIN |
| --- | --- | --- | --- |
| esp32-wifi | 1,064,996 B | 52,880 B | 1,065,152 B |
| esp32-cloud-smoke | 1,082,880 B | 52,992 B | 1,083,024 B |

原 ZIP SHA-256 `3bc8ebf28db25f7f81aa1491660da643949012d3c00d07a0047e49a43a1303ff`；
profile SHA-256 `b4ef0cb36a7fdc654b71bfc4b1ab47850c0a58819f6559e028f67bf89b7bcd30`。
新 ZIP 摘要与 Aily 结果见下文；不以旧摘要代替新包摘要。

### 10.4 Aily 公开 TLS 两目标结果

唯一串行队列两目标 exit=0，无自动重试；原 SDK 3.3.11、最终 R2b builder 和同一个精确安装库未变。
两目标本地 Core 归档均未命中，实际各自编译／归档并完整链接；不将启用缓存等同于命中，
不跨 SDK 复用、不伪造 Ninja 状态。远程缓存关闭，未发出获取请求。

`audit-aily.py` 仅读取已完成的实际 Ninja 图／deps、归档、map、ELF 与 BIN，
审计投影写到各目标 `audit`，未改原 build 或 `compile_commands`。两目标均通过：

- 94 条实际编译命令，17 个当前库 TU 和 17 个归档成员完整匹配；无片段独立编译或重名成员。
- 库闭包严格为 BlinkerDeviceV2、PubSubClient、WiFi、Network、NetworkClientSecure；没有为 TLS 放宽其他库。
- 323 个实际记录的 Blinker 头路径、源码路径、安装字节和冻结输入匹配。
- 现有 WiFi／CloudSmoke 授时、零任务和无跨平台／实验链路的符号门禁通过。
- 补核 `NetworkClientSecure::connect`、`setCACert`、`start_ssl_client`、`mbedtls_ssl_handshake`
  实际保留；profile 的 TLS 与 CA 从精确安装字节核对，不以名称或仅 archive 输入证明功能保留。

| Aily 目标 | 实际应用 BIN | 静态 RAM 原始节和 | 整次进程耗时 |
| --- | --- | --- | --- |
| esp32-wifi | 1,065,360 B | 52,877 B | 253.881 s |
| esp32-cloud-smoke | 1,083,248 B | 52,989 B | 248.114 s |

这里 RAM 为 ELF `.dram0.data + .dram0.bss` 原始和；构建日志的 globals 还计入 3 B `.noinit`，分别为 52,880／52,992 B。
应用 BIN 不是 merged BIN 或 ELF 文件大小，不与 §10.3 的 CLI Flash 报告混用。
主机编译耗时不是设备运行性能／峰值资源证据，CLI 与 Aily 的产物不要求字节相同。

两份新 Aily ELF SHA-256：

- WiFiQuickStart：`6b8e851b6fa205ac5fe1608f6aef8f6b0d2ecac1c31f466490471c6262c2f50b`。
- CloudSmokeTest：`85bc400ccee485267b51a1a9a3e165fed4b496dbabd8d92e4fcbd0cb1e29d0b8`。

其余 map／BIN／归档长度及摘要分别保留在两目标的 `audit/summary.json`；两个审计均明确是安装／编译／链接，
不冒称云端运行、实物可靠性或新的控制 ACK／State 验收。

### 10.5 精确候选冻结

- 单一库 ZIP：443 文件，735,990 B，SHA-256
  `281717d0f9ce7b3eb6bc442844f186bc43d45596174266769c5c0d71205ea396`。
- 冻结 `inputs.json` SHA-256：`af00b012f2c23c236bbd1d18b8f2e9263b561e1373db799ceb3241de85415a9e`。
  包含安装库／依赖、公开 profile、最终 builder、原 SDK 元数据及实际工具版本。
- 当前内容身份头摘要与 D1b 一致：`05297506b491085c8cd474bc2faf92c1e0544d92cc63a7b152153ffef7d05448`。
- 独立只读复核安装字节、当前 builder／冻结 dist、SDK 与原 CLI 证据复用边界；本批 `git diff --check` 通过。
  既有 builder 113 项回归、R3 三项回归和已验硬件／服务流程未重复执行。

### 10.6 收口与下一步

R4 在**当前公开 ESP32 WiFi／TLS／零槽候选**范围收口：精确新 ZIP 安装通过，CLI 原完整产物按相同编译输入复用，
Aily 两个缺证目标新完整链接及严格审计通过。没有把 R2 Plain、旧 ZIP 摘要或旧 SDK 产物冒充本批结果。
无需为本批改设备执行链、配置生成器、SDK 或依赖版本。

下一步 R5 只整理该受限候选的安装入口、版本／校验值、依赖／旧库冲突提示及匹配发行 CI；
先复用已有脚本，缺的才补，不再新开相同编译队列。完整多模式云端候选仍缺 Ed25519 业务签发和实际认领入口，
不能把本批两个目标收口表述为六模式已经全部可云端发布。

本批无部署、刷板、账号／授权／TaskStore／scope 更改，无验证码、业务请求或提交推送。
UNKNOWN、完整日志和无关本地修改保留；构建产物仅在本机忽略目录，不纳入源码发布。

## 11 R5 当前受限候选交付准备

### 11.1 最小实施范围

只完善当前 ESP32 手动 WiFi／HTTPS＋MQTTS／普通持久任务 0 槽候选的用户安装与维护发行入口。
库／固件版本保持 `0.1.0`／`device-v2-0.1.0`，沿用原 SDK 3.3.11、相同 public profile 和设备源码。
没有改设备执行链、用户 API、六项公共 metadata 依赖或构建生成器，也不把其他模式顺带并入本候选。

### 11.2 安装、依赖与旧库冲突

- README 在首个示例之前提示当前云端包的受限范围；安装步骤随 ZIP 一起提供，不依赖用户能打开仓库文档。
  另提供[中文安装说明](DEVICE_V2_ESP32_CLOUD_BETA_INSTALLATION.md)，覆盖 IDE／CLI、校验、原板型与两个例程。
- 明确安装的是经 `cloud-beta.json` 配置的单一 ZIP，不是 GitHub 源码 ZIP、裸克隆默认头、旧 Core／Port 分包或私有联调 stage。
- 当前实际外部依赖仅 PubSubClient 2.8；WiFi／Network／NetworkClientSecure 来自原 ESP32 3.3.11 SDK。
  不需要外部 NimBLE-Arduino、ArduinoBLE、Crypto 或 Seeed RPC 库，不用同名第三方网络库替换 SDK 实现。
- 统一库 metadata 仍是六项扁平依赖。Library Manager 能解析整套声明，不能承诺自动按 Sketch 只装一项；
  已验 CLI 的本地 ZIP 不自动解析 `depends`。当前安装说明显式列目标依赖，不为规避提示改变公共库 metadata。
- 旧 Blinker／V1、重复 V2 或含 `Blinker.h` 的副本移到 sketchbook `libraries` **之外**保留；
  仅在里面改名仍可能冲突。核对详细输出的 Used／Not used 路径，不批量删除库、Sketch、凭据或清除设备 Flash／授权。
- 用户只在自己的 Sketch 填对应环境 DeviceKey、SSID、密码；不修改 SDK、服务 host、CA 或业务 ring，
  正常持续推进 `Blinker.run()`。安装／编译不需要真实账号、验证码或硬件连接。

### 11.3 匹配候选 CI

新增 `.github/workflows/device-v2-cloud-beta.yml`，没有重造打包／安装执行链：

1. 仅 `workflow_dispatch`，`contents: read`，单个 Ubuntu 24.04 job；不自动部署、刷板、创建 tag 或发布 GitHub Release。
2. 固定已用 Arduino CLI 1.5.1 和原官方 ESP32 3.3.11，只使用独立 SDK data／downloads／work 目录。
   未升级 action 版本，沿用仓库已有 checkout／setup CLI／upload 动作。
3. 现有 `build_arduino_release.py` 用 public profile **构建一次**，不传 development override；
   校验其生成的 `SHA256SUMS` 后，调用原 `verify_installed_release.py`，显式只选
   `esp32-wifi` 和 `esp32-cloud-smoke`，jobs=2，使用原目标依赖与严格符号／来源门禁。
4. 仅前面步骤成功后上传一个 ZIP 和它的 `SHA256SUMS`，artifact 名带提交 SHA；不上传整个 workspace、SDK、私有备份或账号材料。

原 `device-v2-ci.yml` 保留 development／多平台质量职责，仅补上 `profiles/**` 和新 workflow 的两组路径过滤器。
它不作为当前受限云端候选的发行证明。新候选工作流只覆盖 Arduino CLI；Aily 的匹配冻结输入及完整链接证据仍取 R4，
不下载未包含 R1／R2b 修正的远端 HEAD 或声称任意已发行 builder 均已兼容。

本机配置检查**不等于 GitHub CI 已运行**。本批不提交／推送，远端工作流与 Aily 修复的可获取版本仍需后续明确安排；
不为补这个状态偷偷推送或触发 CI。每次 CI 对所检出的提交生成自己的摘要，不承诺与本机未提交工作树或不同检出字节跨主机相同。

### 11.4 有限回归与差异验证

- 新增 `tests/release/test_cloud_beta_delivery.py`：**7 项通过、0 失败／跳过**。覆盖手动只读权限、
  固定 SDK／两目标、单次 public profile 打包与校验顺序、成功后限定 artifact、原质量门禁的 profile 触发范围、
  随包安装说明及原归档器按精确 ZIP 生成摘要。
- 复用本机已有 js-yaml 解析两个 workflow，语法与新工作流的手动触发／只读权限／单 job 检查通过，未安装新依赖。
  有限文本合同及 YAML 检查不能替代远端实际编译成功。
- 现有发布器构建一次 R5 候选，Arduino CLI 在本批空 sketchbook 安装成功；
  `verify_installed_bytes` 对安装目录及 stage 逐文件核对通过，未启动 SDK 编译。
- `build/library-release-r5-20261027/audit-delivery.py`／`delivery-audit.json` 记录与 R4 精确包的差异：
  同为 443 文件，无新增／删除，**仅 README.md 改变，其余 442 文件字节一致**。
  profile、全部源码、例程、metadata 和四个派生头不变，内容身份头也不变。
- 读取 R4 两目标审计清单并核对原 ELF／map／应用 BIN 大小与 SHA；保留 CLI 原完整产物的既有复用链。
  所以本批可复用 R4 编译证据，不重复 SDK 队列；明确 `newCompileOrLink=false`、`ciExecuted=false`。

本批 `git diff --check` 通过；原 R1／R2b／R3 测试、已通过的固件编译和实物流程未重复。

### 11.5 精确候选与校验值

候选目录为本机忽略的 `build/library-release-r5-20261027`，未生成新的私有备份或将产物加入 Git。

| 项目 | 当前值 |
| --- | --- |
| 文件名／库版本 | `BlinkerDeviceV2-0.1.0.zip`／`0.1.0` |
| 固件标签／产品 profile | `device-v2-0.1.0`／`esp32-cloud-wifi-v1` |
| 文件数／ZIP 大小 | 443／737,390 B |
| ZIP SHA-256 | `d815614a47e5f1029fa37e9647685118de1dd1b25dee7fa95fb3a68b7225ce1b` |
| profile SHA-256 | `b4ef0cb36a7fdc654b71bfc4b1ab47850c0a58819f6559e028f67bf89b7bcd30` |
| 内容身份头 SHA-256 | `05297506b491085c8cd474bc2faf92c1e0544d92cc63a7b152153ffef7d05448` |

配套 `SHA256SUMS` 来自原发布器，并已核对对应本精确 ZIP；不要把摘要写回随包 README 形成自引用。
R4 的 735,990 B／`281717d0…05ea396` 仍是原编译证据对应的历史包，不是 README 更新后的新包摘要。
相同版本可以有不同内测候选字节，安装时核对同批文件；摘要不是签名，可信来源仍需保证。

### 11.6 收口与正式发行前的剩余项

R5 的**本机安装交付准备与 CI 配置**按当前范围收口：安装说明、依赖／冲突提示、版本／校验值和手动候选 CI 已落地，
本机有限回归及 README-only 新包校验通过。不新增产品功能、不扩大平台或任务范围。

正式发行之前仍须后续授权并记录：必要源码与文档提交／推送、远端匹配 CI 实际运行、候选复核和发布决定；
Aily 自动化若接入远端构建，还需可获取且明确固定的 R1／R2b 修复版本，不能拿尚未同步的远端版本冒充本机冻结候选。
完整多模式云端 profile 的业务签发／入口阻点仍按 R3 保留，不是重跑当前两目标的理由。

本批没有升级 SDK、部署、刷板、发送验证码、业务请求、改变账号／授权／TaskStore／scope 或提交推送。
UNKNOWN、完整业务日志和无关本地修改保留；原服务与设备不受本批操作影响。

## 12 可选设备定位的现行边界（2026-10-29）

定位实施与真实结果独立维护于[设备定位方案](DEVICE_V2_DEVICE_LOCATION_IMPLEMENTATION_PLAN.md)。
LOC-0／1 的数据与 owner-only latest、LOC-2 私有 Cloud 租约及受控非卫星上传、LOC-3 地图最近位置／页面生命周期，
以及 LOC-4 owner 按需网络城市参考已按该方案 §10～§17 有限收口；不能继续按旧 §9 停点重复排测试。
网络参考是服务端数据源和 App 入口，不是设备库的新依赖，不把城市参考当成设备 GPS 精度或安装位置。

LOC-5 按用户确认只提供统一 `Blinker.location(...)` 输入，不接管用户 GPS／其他设备的驱动或解析。
显式 `Location` 复用单快照和原私有 Cloud 绑定／租约；本地接受不等于发送／ACK，不自动授时、启动上传或写安装位置。
§18 的有限行为回归与原 SDK 对象证据、§19 的新 ZIP 隔离安装及完整链接已完成，
后续不再要求具体 GPS 型号或另开相同编译／实物队列。

库的普通支持范围仍为原 ESP32 手动 WiFi／TLS／默认 0 槽；定位是需精确 Manifest／owner 准入的可选能力，
未开放 BLE／LAN／R4／Wio 的位置上传、定位分享／历史或默认背景采集。
R5 与 LOC-5b 的精确包、摘要和编译证据原样冻结；新增源码／README 不能沿用旧 ZIP 摘要声称是同一候选。

## 13 LOC-5c 下一候选说明与可选 CI 接线（2026-10-29，有限收口）

### 13.1 最小交付范围

下一候选仍由现有发布器生成一个配置化 Arduino 库，不拆定位分包、不新增 GPS 依赖或 SDK 版本。
必要内容是已验的统一输入／原私有 Cloud 接线、相应 host 与安装消费者回归、同一库 README 和维护门禁。
公开 `profiles/cloud-beta.json`、默认两个受限目标及原七目标矩阵不变；App／Service 的网络参考与地图实现不混入设备库 ZIP。

README 随包提供可独立使用的最小 Sketch、纬度在前／同 boot 采样 tick、30 秒默认年龄、失锁／清理及本地接受语义。
用户不需要能打开仓库文档才能接入。进阶精度／UTC 说明仍在源码仓库的
`docs/DEVICE_V2_LOCATION_INPUT_API.md`；不把含内部计划引用的整份文档强行加入打包白名单。
保持 caller-owned 生命周期和单上下文调用，不引入第二条上传器、自动采样或新的默认成员。

### 13.2 同一 ZIP 的可选门禁

原手动云端候选 workflow 增加 boolean `verify_location_input`，默认 `false`：

1. 沿用单次 public profile 打包、`SHA256SUMS` 检查及原 `esp32-wifi`／`esp32-cloud-smoke` 两目标。
2. 勾选后，用**同一个 ZIP**调用原 `verify_installed_release.py --location-input`，只换为已验的 driver-free 单目标消费者。
   使用新的安装 work／sketchbook、原 SDK data／downloads 和前步已装 PubSubClient 源码缓存，jobs=1；不复用旧编译产物，不另装 SDK 或第二次打包。
3. 可选步骤实际成功后，才可声明该提交的定位输入已通过远端安装／完整链接；默认两目标成功不能代替它。
   选项本身不打开运行设备准入，不提供模拟点或卫星精度证据。
4. 仍仅 `workflow_dispatch`／`contents: read`，所有门禁成功后只上传同批 ZIP／校验文件；无账号材料、远程部署、tag 或 Release 发布。

对应回归沿用原质量 workflow 的 `tests/release/test_*.py` 入口，不另造执行器或引入 YAML 解析的运行依赖。
CI 覆盖 Arduino CLI；既有 Aily 冻结输入证据保持独立，不据此宣称任意远端 builder 版本均已兼容。
本节是 LOC-5c 本地准备时的状态；获准执行实际交付后的同提交 CI 入口修正与结果见 §14。

### 13.3 有限验证及证据保留

- 本批只运行受影响的交付合同回归：`test_cloud_beta_delivery.py` 9/9 通过，包含可选开关、同包／同 SDK、独立安装目录、上传顺序和随包最小用法。
  原 LOC-5b 的 7＋4 项门禁、host 与完整链接不重复执行。
- 本机 YAML 结构检查核手动触发、boolean 默认关闭、唯一 job／SDK 安装／打包和可选步骤条件；独立只读审阅不代替远端 CI。
- 本批没有重新生成候选、启动 SDK 队列、安装 App、操作设备或部署服务；完整授权、UNKNOWN、业务日志及无关本地修改保持。
  测试仅创建并自动清理极小的临时校验 fixture，不是新的发行 ZIP。
- README 的补充会改变下一次候选字节。§11.5 的 R5 摘要和定位方案 §19 的 LOC-5b 摘要仍只标识原历史包；
  下一次实际打包按对应检出提交生成自己的 `SHA256SUMS`，不手填摘要、不覆盖历史产物。

### 13.4 本地收口与下一步

统一定位 API 的实现、隔离安装／原 SDK 完整链接及本批交付准备均已按限定范围完成，**不再为同一功能排新的本地编译或真机循环**。
剩余是实际发行流程，而不是新的设备功能缺口：后续另获提交／推送授权后整理必要源码和文档；
在匹配提交上手动运行原候选 CI 并勾选定位门禁，核实际结果与同批摘要，再作发行决定。
若交付 Aily 用户，还须固定可获取的匹配 builder 修复版本，不能以本机证据替代远端可用性。

本批没有提交／推送或触发远端 CI，不把配置完成写成远端通过，也不自动回到暂缓的 App H1／H2 恢复问题。

## 14 必要源码交付、同提交远端 CI 与发行确认（2026-10-29）

用户已授权本批交付必要源码并执行匹配远端 CI；不等于发布任意平台正式版、npm 包或更新默认分支。
仍使用同一库／原公开 profile／原 SDK／已有执行脚本，不重跑本地链接、App／服务或实物流程。

### 14.1 固定可获取的 Builder 修复

`ailyProject/aily-builder` 的 `i3w-builder` 已提交并普通快进推送至
[`e18855124a03e40ec10f47912e62db113dbf2716`](https://github.com/ailyProject/aily-builder/commit/e18855124a03e40ec10f47912e62db113dbf2716)。
只纳入 4 个源码文件与 5 个必要回归：`.ipp/.inc` 扫描／缓存指纹、公共入口与宏上下文隔离、薄 TU 保留及 Windows fixture。
原 40 项 builder 源码与 R2b／R4 冻结清单一致，保留原 113 项软件和完整链接证据，不为了推送再编译。
既有 4 个上游 main 修复在当前分支正常交付；未切换或改写 main、sim 工作树，不纳入其他未跟踪文件、dist、SDK 或日志。
版本与锁文件不变，未发布 npm；使用者须固定上述提交，不能只凭 `1.2.17` 判断已包含修复。

### 14.2 设备库交付与有限 CI 接线

设备库仅纳入已验 LOC 源码、对应 host／安装消费者回归、两个 CI 文件及必要安装／API／发布方案，精确白名单交付。
不纳入混有 App 失败恢复的进度修改、`docs/partition.csv`、账号／网络密钥、私有备份或 build／stage／产物；原样保留无关本地修改。
服务和 App 本批无新增交付，不把设备库候选准备扩成三端部署。

已只读确认 GitHub 默认分支是 `master`，其中没有新候选工作流。
[GitHub 手动运行合同](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow)要求该工作流位于默认分支，
因此不以反复 dispatch 或改 `master` 绕过此限制。最小修正为保留手动入口并增加同名 boolean 的 `workflow_call`，
让现有 `device_v2` push 质量 workflow 在 host／package 门禁通过后，以 `./.github/workflows/device-v2-cloud-beta.yml`
调用同提交版本并传 `verify_location_input: true`。[本地 reusable 引用](https://docs.github.com/en/actions/how-tos/reuse-automations/reuse-workflows)固定为调用者同一提交。

调用仅限 `push`＋`refs/heads/device_v2`，继承 `contents: read`，不传 secrets，不在 PR 或其他分支自动打候选。
原单次 public profile 打包、同 ZIP 的两个普通目标与可选定位安装／完整链接、成功后限定上传均复用。
本批受影响交付回归 10/10 与两个 YAML 结构检查通过，不重新运行旧 host／SDK／硬件队列。
远端质量矩阵与受限候选的结论分别记录：不拿旧开发 profile 的 7 目标结果代替新的 TLS／定位候选。

### 14.3 发行确认门槛

仅当本次源码提交的实际 run 中 host／package 和受限候选均成功，且同批 ZIP／`SHA256SUMS` 核对通过，
才能确认该提交的**内部 ESP32 手动 WiFi TLS／默认 0 槽、定位输入可选**候选可交付。
定位仍需原 Manifest／owner 精确准入，不因发行打开设备名单；现有任务／授权／UNKNOWN／完整业务日志均不操作。
R4／Wio、多模式云端、物理 GNSS、定位分享／历史及公开稳定发行继续不包含。
实际 run、提交和校验结果在执行完成后补入本节；未取得结果前不把 push 成功写成 CI／发行通过。

### 14.4 首次远端执行与解析阻点

首批 40 文件已交付至 `device_v2` 的 `0171921884ac038ff448ace608be4a12b3907034`，仅三项排除路径留在原工作树。
Git HTTPS 连接先卡住、随后有界传输报告无法连接；没有强推或重写历史。
改用已有仓库凭据通过 GitHub Git API 逐个核对 blob、tree 和完整 commit SHA，再以 `force: false` 更新原分支；
服务器提交与本机提交完全一致，没有把 API 生成的另一份提交冒充原 SHA。凭据只在内存，未写日志或制品。

同 SHA 的[质量 run](https://github.com/blinker-iot/blinker-library/actions/runs/40821113612)和
[候选解析记录](https://github.com/blinker-iot/blinker-library/actions/runs/40821115037)在 runner job 创建前失败，不能算作软件／编译通过。
静态核查发现新候选及原质量 workflow 在 job 级 `env` 使用 `runner.temp`，违反
[GitHub 上下文作用域](https://docs.github.com/en/actions/reference/workflows-and-actions/contexts#context-availability)。
本地 YAML 语法检查没有覆盖表达式作用域；最小修正把这几项目录初始化移到 SDK 安装前的运行步骤，
从运行时 `RUNNER_TEMP` 写入本 job 的 `GITHUB_ENV`，保留原目录隔离和 SDK，不改产品源码。
新增作用域／运行步骤初始化顺序保护后，受影响交付回归 11/11 与 YAML 结构核查通过；
仅对修正提交继续一次匹配 CI，不重跑本地 SDK 或改默认分支。

Builder 的本机冻结记录包含 CRLF，Git 标准 blob 是 LF；规范化文本与原冻结源码一致，
不宣称不同平台检出的原始文件字节摘要完全相同。远端可获取性由固定提交确认，编译支持边界仍按原 R4 证据。
