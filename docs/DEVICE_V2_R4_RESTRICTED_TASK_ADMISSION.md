# R4 四槽受限产品：准入收口

## 1. 决定与出口

本阶段不再审计任意 Sketch、全部 IRQ 或无限碎片历史。基于任务方案 §11.47～§11.52 的已有证据，接受一个**非安全关键、受维护固定图**的工程支持合同。资源补证、匹配发布构建与一次真实 App 短验均已完成；不再重开资源、BLE、断电或长循环。

**受维护固定 R4 四槽 Plain 产品有限准入通过；通用 R4 Sketch／普通 ZIP 的任务准入继续关闭。** 资源支持评估、构建、运行验收与服务器显式准入仍是不同状态，不能靠删除 INTERNAL 宏跳过。普通单一 Arduino ZIP 保持任务默认 0，不产生另一套需要用户安装的 R4 库 ZIP；只对下表精确固定图及匹配服务设备 ID 准入，不为任意 Sketch 提供资源保证。

§11.52已通过实际生产选择器及本轮真实 grant 的 Ed25519／keyId1／签名／五字段与 scope-instance 绑定、真实 App 查询／保存／一次 Start／后台执行、完成进度及新鲜 State 验收。首次上线阻点为保护层误拒绝 SDK 长度字段前导空格，已最小修复、保留红灯并通过13项有限回归；无诊断产品图程序只增24 B，静态RAM／heap不变。正常任务删除与设备签名在线退休后，十二键／授权及指定原WiFi恢复BIN已恢复；临时服务准入恢复，不把临时测试ID或本机开发验签钥当作生产部署。

Wio 暂缓；实时续租 Error 8 独立待定位，不将两者继续追加为此固定 R4 任务图的开放前置。

## 2. 有限支持合同

| 项目 | 固定范围／接受条件 |
| --- | --- |
| 工具与平台 | 原 Core 1.5.3、原 GCC 7-2017q4、UNO R4 WiFi、NO_USB；只在隔离副本应用已有 Modem／Serial overlay，不改用户安装的 SDK |
| 产品图 | 手动 Plain WiFi、Small 256 B、Ed25519；power 布尔、duty 整数 0..100／step 5、uptimeSeconds 1000 ms 实时采样，短回调，仅原公共 API |
| 任务 | 原四槽 TaskEngine／TaskStore／Codec；写窗口 60000 ms、最多 10 次、最短 Start 30000 ms、commit 策略 1000 ms；不套用 ESP32 的 100 ms 策略，不自动 Start |
| 资源 | MQTT 初始及消费者同为 361 B；单 parser command 32 B、response 1024 B；整次 RPC 1000 ms；唯一 owner／boundary；原 `_sbrk` 上方保留 6144 B 栈工程空间 |
| 运行假设 | 单协作 loop、原 AGT／SCI、正常时间基准；不在 IRQ 分配，不引入 RTOS、DMA、长回调、动态日志、额外 heap 消费者或任意浮点格式化 |
| 故障 | 共享故障锁存后停止服务、禁止后续原生调用；Close 失败保持不可返回 fail-stop。人工整板冷恢复后，以新鲜只读结果协调；不自动解锁、重连重放或补造 ACK |
| OOM | 原 SDK new／string fatal 行为保留，不承诺 ACK、析构／RAII、自动复位或可返回恢复。已经存在的持久意图／UNKNOWN 保留；启动前失败不虚构业务意图 |

这不是硬实时／安全关键设备承诺，也不是任意第三方 Sketch 的资源保证。已有 UART Close、四 IRQ 禁用、停钟／解绑和生产清理证据足以支撑上述停止服务合同；不要求新增逻辑分析仪或所有真实晚 ISR 证明。原 TXI pending、借用指针残值及未测线路波形仍是证据限制，不改写为已证明瞬时物理静默。

## 3. 连续块／碎片补证结果

离线执行 §11.50 已审 ELF 的原 ARM wrapper、malloc／free／realloc、heap cap／sbrk、锁与 memcpy 机器码，未替换函数；临时 Unicorn 2.1.4 仅为测试工具，不进入库依赖或 SDK。

六个有限 RPC 生命周期窗口、149 个事件通过：最大单笔请求 1921 B，已枚举同时请求 5464 B、实际存活 chunk 5556 B；真实 break 高水位 **8476 B／8936 B**。首次完整 network／storage family 后与重复最大窗口结束均恢复同一 **5924 B 连续 free 块**，原 1921 B 再申请及释放后同形。

**460 B 只是未伸展的 break 尾部，不是总 free 或额外安全余量。** storage 生命周期确实造成一次额外 1932 B 伸展，已保留；不能只用申请量总和减 heap 跨度判通过。固定完整 family 后的归一只支持所列生命周期，不宣称整个产品、任意申请历史或 C++ OOM 恢复已模拟。

首轮在执行 ARM 前拒绝原 FSP 的 28 B `.id_code` 段；仅记录该精确非执行段而不映射，任何访问仍失败。第二轮错误要求冷首个 network 窗等于完整 family，所有分配成功但归一断言失败；原 8476 B 高水位与首错保留。第三轮仅增加一个 storage 重复窗、改用完整 family 参考，通过后不再加窗或运行。allocator 直调最深 88 B 不是产品／IRQ 栈证明。

最终报告 SHA256：`b774ce1e9868482515f07f075bad7b148513792641a18a0cda53f9d88a5cd315`。私有目录和逐事件日志保留，不作为公开交付内容。

## 4. 匹配候选的发布接线

采用维护者专用 `tools/prepare_r4_restricted_release.py`，复用原候选 overlay 和公开发布配置解析器，不放宽普通 ZIP／ESP32 task profile 的合同。

- 生成独占隔离目录：原 Core 副本、六份精确 overlay、当前完整库源码投影、唯一 heap wrapper TU 和固定最小 Sketch。Core 另镜像四份原端口合同头以满足其独立 include 路径，不复制框架实现或新增 owner。
- 公开配置只含控制入口、固件版本和验签公钥；拒 TLS、其他 task profile 或可扩展的回调／功能图。
- DeviceKey、SSID 和密码独立输入并生成私有头，不写进公开 manifest、源码仓库或 stdout；不从旧设备、UNKNOWN、账号备份自动提取。
- 所有 SDK／库／Sketch 消费者统一强制配置头，通过标准 ArduinoCLI 原 recipe 构建；不再手工混拼新旧 Modem／Serial archive。
- 准备回执仅为 prepared，`linked`、`formalTaskAdmission`、`hardwareWritten` 均 false；输出或输入变动需重新验证，不以自报摘要代替原输入。

维护者准备及复现命令（输出应放入私有、Git 忽略的独占目录）：

```powershell
python tools/prepare_r4_restricted_release.py --sdk <Core-1.5.3目录> --pubsub <PubSubClient-2.8目录> --crypto <Crypto-0.4.0目录> --compiler <原GCC的bin目录> --builtin-tools <Arduino15/packages/builtin/tools> --profile <公开配置.json> --output <新stage目录>
# 同一组参数再附加 --verify-only；实际验收另加 --credentials <私有三字段JSON>
$r4Plan = Get-Content <stage目录>/release-plan.json -Raw | ConvertFrom-Json
& arduino-cli @($r4Plan.cliArguments)
```

私有 JSON 仅接受 `device_key`、`wifi_ssid`、`wifi_password` 三个字符串，生成 UTF-8 转义头；不允许注入 C++ 宏或回调。未提供时只有占位数据，能验证构建接线，不能用于设备上线。公开配置沿用现有 development schema，不接受其他 task profile、TLS／CA 或私有字段；该阶段不是生产环境发布。

## 5. 有限开放验收清单（§11.52已通过）

1. 用上述配方完成一次匹配构建，核配置及实际依赖、唯一 owner／boundary／Modem／Serial／heap wrapper、361 B 一致性、新 HeapBase／cap 与静态边界。仅身份／Flash 网络常量变化不重开资源审计；实际常驻、回调或 heap 分区变动才是新产品图。
2. 备份后做一次匹配 App 短验：只读查询／新 scope 初始化、一个停用定义保存、一次受控 Start／执行确认及新鲜状态、正常退休和恢复。UNKNOWN 恢复与故障停止复用既有通过证据；不再注入故障或做断电循环。只补此发布图尚未覆盖的差异，新实际写入不得用旧 scope／UNKNOWN。
3. 明确固件版本、匹配签发算法／keyId、公钥、四槽策略及支持合同后，仅对受维护固定图有限开放；普通 Sketch、普通 ZIP 和 Wio 仍不随之开放。正式开放需独立记录决定，不由 prepare 工具修改服务白名单。

完成以上三项即结束此受限准入阶段。OTA、任意回调／TLS／LAN／BLE 组合、自动故障恢复及其他 SDK 兼容另列阶段，不追加为当前出口条件。

## 6. 本批已完成的发布接线核查

维护工具与固定 Sketch 已实现，九个有限准备场景及受影响差异回归通过。使用公开 CI profile／占位凭据，已在同一隔离 stage 由原 Core 1.5.3 和标准 ArduinoCLI recipe 完成实际编译、完整链接及构建后输入验证；不是手工拼接旧 archive，也未刷板或操作本机服务。

| 实际 CI 图 | 结果 |
| --- | --- |
| 程序存储，原 recipe `.text+.data` | 180968 B |
| 常规静态 RAM，`.data+.bss` | 17248 B；另有 `.noinit`28 B，不将名义 heap／stack 计入此数 |
| HeapBase／cap／StackTop | `0x20004380`／`0x20006700`／`0x20007f00` |
| 可伸展 heap 跨度 | 9088 B，不小于补证图的 8936 B；不是新的动态高水位测量 |
| 实际接线 | 唯一 owner／boundary／heap wrapper，原 `_sbrk_r→wrapper→_sbrk`；MQTT 构造为 361 B，无运行 observer |

首轮 include 引号拆分、随后预置源码 include 绕过 CLI 库识别，以及 Core 不继承库头路径的三个构建缺项均保留原日志后有限修正。最后一项仅新增四份原字节合同头，CLI 沿用已成功对象完成剩余编译／链接；没有为此重开资源、BLE 或断电门禁。审计首次误用无命名空间的 Client 符号名，改为实际 `arduino::Client`，复用已完成 nm／反汇编，不重编或篡改原首错。

该批准备回执仍为 prepared；实际链接结果另存，不回填工具的 `formalTaskAdmission=false`。CI ELF SHA256 为 `3ecb9d57d182c61c71be4d7ad15e3cb144518a820322eab3e1a73b878d7f665f`，不向用户交付含占位数据的可上线固件。当时剩余的实际 profile／签发核对及一次 App 短验，已由下节§11.52完成；不重新派发，不回到资源审计。

独立窄终核已通过：1583个准备文件、132条编译配方、四头镜像与实际owner／boundary共享指针、heap调用和361构造均相符。回执 SHA256 为 `d26571980e68d6235346a50437077908b7639d5c4772ec746811a1b8ef213383`。三仓无关修改及索引保持，未提交推送；私有构建／首错／完整日志留本机，不进入公开源码。

## 7. 匹配运行短验与有限开放（§11.52）

实际PREFGET在6 ms误拒绝长度字段`" 36"`已修复：只跳过前导ASCII空格，至少一位数字，原容量／溢出／符号／尾随垃圾限制、整次截止和锁存不变；二进制payload不trim。旧逻辑红灯及3新增＋10必要原回归（13/13）保留，不以新诊断图代替正式图。正式无诊断BIN为`a52d4006ca12994c4107c00ff9775367d0977e06724a324eeb1316c4dc56834f`，程序181008 B（＋24 B），RAM17248 B、heap9088 B及栈预留6144 B不变。

本机实际Ed25519／keyId1／公钥与真实grant完整绑定通过。真实App四槽查询／新scope初始化、停用保存、启用、唯一30秒Start、后台到期progress3及新State revision2／power=true、返回控制页fresh、删除及设备签名在线退休均通过；十二键／授权和指定已验WiFi恢复BIN已恢复，临时新ID已正常移除、服务准入恢复。完整原备份及新旧请求留存，旧14UNKNOWN不变；没有重放、故障或断电循环。

临时设备额度已正常恢复22→原10，active override0／权益revision20／remove审计与outbox及缓存失效均已核对；先前OOM和提交断连先只读协调、保留失败意图，不将例外抹成首次成功。此次资源与运行验收收口不遗留行政临时额度或永久测试准入。

维护者后续可对**同一固定图**，按本文件配方及匹配profile／Ed公钥构建，并对精确设备ID显式准入；仅网络／Key常量变化不重开资源审计。改变常驻、回调、heap分区或支持功能组合才是新产品图。普通用户仍使用同一标准库，不新增R4任务ZIP，不声称普通Sketch已默认支持持久任务。本机测试钥及本批候选不是线上生产发布；Wio和Error8仍独立待办，不再作为这项已通过有限支持的追加前置。

有限启用沿现有配置：Gateway 同时将精确设备ID加入 `DEVICE_V2_TASK_MANAGEMENT_DEVICES` 与 `DEVICE_V2_TASK_ED25519_DEVICE_IDS`，对应Broker加入 `DEVICE_V2_TASK_ALLOWED_DEVICE_IDS`。先核实际签发为Ed25519／匹配keyId及固件公钥，再新鲜查询容量4；不能只开一个列表、按MAC推断身份、重用旧scope／UNKNOWN或删除内部守卫。退出试点先走正常在线退休及云移除，再恢复空任务备份；操作与授权审计不回滚。

## 8. 普通Arduino示例／用户API接入顺序（§11.53）

“四槽有限准入”是固定用户图、原SDK受保护构建、匹配验签材料及服务设备ID准入的交集；“普通默认0槽”是同一标准库在缺少该受维护构建配置时不创建持久任务图。两者不矛盾，也不表示Sketch Timer、实时数据或Cloud历史被关闭。

公开API已有`Blinker.enableTasks(power, duty)`及原`begin／run`；前者自动启用业务授时，只登记任务允许控制的Property，不初始化owner、不创建或Start任务。默认产品返回`UnsupportedFeature`。本阶段不加第二套API、不把组件自动全局注册，也不让用户填写容量、存储、workspace或SDK参数。

### 本批最小实施

1. 将固定图收为唯一`examples/R4TasksQuickStart`参考：普通`BlinkerWiFi.h`、原组件及API、三字段占位凭据；维护生成器直接读取同一`.ino`，私有凭据仍隔离，删除重复Sketch模板。不得把现有带button的WiFiQuickStart直接改成已准入图。
2. 补R4端口内部精确构建合同：原板型／NO_USB、四槽Plain策略、Small256／MQTT361、命令32／响应1024／RPC1000／heap保护。通过原RPC故障查询及原heap wrapper零增量查询形成实际强符号链接依赖；缺SDK overlay、heap TU或wrap配置应拒绝，不提供弱fallback或新owner。它不是安全证明或资源审计，实际编译recipe和最终ELF仍须匹配。
3. 同步单一库打包清单、普通安装编译的证据措辞及README，明确新示例是受维护profile参考，默认未准入时清晰拒绝，不默默以0槽运行。
4. 补默认API／受限配置／冲突配置／缺真实链接保护与共享示例来源的有限回归，并用原SDK完成一次匹配图定向构建／链接。无新资源审计、硬件写入、服务部署、SDK升级或Git推送；新接线产生运行差异时再单独安排一次短验，不重复旧矩阵。

### 普通IDE一键安装仍缺什么

普通Sketch中的宏或新头文件不能自动传播到独立编译的Core、WiFiS3、PubSubClient，也不能给最终链接补`--wrap`；单一库ZIP不会自行修改已安装SDK。上述第一批完成，只代表示例／API与现有受维护构建贯通，不代表任意IDE Sketch已获四槽准入。

§11.53当时剩余的最小交付是可复现、隔离且用户可识别的构建profile安装／选择入口，绑定已有原SDK overlay、全消费者配置及heap TU／链接参数，沿用同一库及同一示例；不能新增每MCU／模式一个库ZIP或要求初学者手工编辑SDK。该交付已由下节§11.54接入，不通过删除INTERNAL守卫、默认容量改4或升级SDK代替。实际常驻／回调／heap分区不变时不重开资源审计。

### 本批已完成的接线与构建

`examples/R4TasksQuickStart`现为唯一固定图；维护生成器读取同一`.ino`，重复模板已删除。示例只调用原`enableTasks／begin／run`，省去重复的显式`enableTime`，三属性及短回调不变；公开凭据必须为三个`REPLACE_*`占位。生成stage不复制旁边的占位头，仍只从独立private目录取凭据，不存在同名头覆盖私有输入。单一库清单纳入这三个文件，不改依赖或原七个普通安装目标。

新增`UnoR4TaskBuildContract.h`集中任务启用图的精确配置，默认关闭图不引用它。`TaskSupport.configure`在构造产品前调用原RPC故障查询，再执行原heap wrapper零增量检查；故障返回`StateConflict`，边界无效返回`NotConfigured`，不分配、不启动UART或增加owner。缺真实SDK／heap强符号链接应失败；宏及链接检查不等同于任意用户图的资源认证。旧架构门禁已改为核对新合同头和调用接线，审阅后仅更新对应冻结项，登录／session／MQTT实现及其摘要不变。

原Core1.5.3／GCC7-2017q4的一次独立prepare→verify→标准CLI完整编译链接→postverify通过；仅公开CI profile与占位凭据，不刷板、不部署、不触碰旧任务或UNKNOWN。实际132个编译TU一致使用强制配置；唯一Modem／Serial／owner／boundary／heap wrapper，实际`setup`到RPC与零增量查询、`_sbrk_r→wrapper→原_sbrk`、RPC1000与MQTT361已窄核。1052份安装SDK输入未变，1584份准备文件绑定通过。

| 新接线CI图 | 结果／边界 |
| --- | --- |
| 程序存储 `.text+.data` | 180912 B，仅当前CI图，不与不同网络／凭据常量的§11.52 BIN作运行性能对比 |
| 常规静态RAM／noinit | 17248／28 B，与原固定图相同 |
| heap跨度／栈工程预留 | 9088／6144 B；只核静态接线一致，不重复动态资源审计 |
| ELF SHA256 | `f552d161e4c7ee54edc940341b1ca3de93330255de0089cbe46bd1c75094f560` |

这是原有限准入图的示例／API接线与静态构建差异证据，不将占位CI固件当已上线固件，也不以此代替新接线的运行验收；§11.52原运行支持边界保持。下一步只围绕受维护profile的普通IDE安装／选择方案及其必要短差异，Wio暂停、Error8独立待定位，不重开资源或长循环。有限回归明细及无关修改保护取任务方案§11.53。

## 9. IDE维护板型接入（§11.54）

已实现`tools/install_r4_arduino_profile.py`，按[安装／选择说明](DEVICE_V2_R4_ARDUINO_PROFILE_INSTALLATION.md)在Sketchbook侧装`blinker:renesas_uno:r4tasks`。原SDK和同一库只读；原heap adapter由维护Core唯一直链，不再需要用户Sketch中的额外TU。安装器复用原overlay／固定配置／公开profile渲染与验证，不新增执行器、owner或资源策略。切回官方UNO R4 WiFi仍默认0槽，用户需要明确选板，USB标识不能替代选择。

11个有限安装场景与原生成器10个场景通过，正常ZIP安装＋带空格路径实际板发现／工具解析／唯一完整链接通过。132TU自动由board recipe配置，没有CLI extra build properties；Core仅原Serial.cpp字节变化、五份纯合同头与一份heap TU新增，六文件overlay全体按原输出绑定（其中Serial.h与原SDK恰好同字节）。程序180912 B、静态RAM17248 B、heap9088 B及栈预留6144 B保持；原1052份SDK输入不变，不重开资源审计。

§11.54当时仅完成Windows本地维护profile的安装／静态构建；§11.55随后已补匹配固件的COM9启动、真实App四槽空表查询和唯一控制ACK／新鲜State，临时空scope正常在线退休后恢复原授权及指定WiFi固件。IDE GUI仍未测，不自动匹配线上服务或开启设备ID准入，公开CI占位BIN不能上线。独立FQBN、可恢复撤销及日志边界取任务方案§11.54～§11.55；普通单库默认0槽、Wio暂停和Error8待定位均不变。

## 10. 发布匹配护栏与现行收口（§11.56～§11.58）

§11.56已关闭安装后更换普通库／公开profile却仍与旧维护板宏／强符号兼容的缺口：发布器生成纯头、维护Config冻结八字摘要并include唯一已核库中的实际头、新任务合同拒绝旧维护板缺绑定；原两种构建工具共用渲染器，不新增执行器、owner、运行RAM或用户配置项。

安装后依赖／工具的手工替换不是这项摘要的覆盖范围；任意Sketch、生产分发及SDK／功能组合仍后置。更新顺序必须先用原匹配工具核验／撤销，再换库、工具、公开profile并重新安装；误更新后先恢复原匹配输入，不强行覆盖旧profile。详细步骤见[安装说明](DEVICE_V2_R4_ARDUINO_PROFILE_INSTALLATION.md#更新顺序与错配恢复1156)。§11.56当时只有有限软件回归及原ARM预处理，没有整固件或实物验收。

随后[§11.57](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#1157-构建缓存清理及新护栏候选的有限运行差异)已补新护栏实际产物的原SDK完整链接、postverify及COM9启动／真实App空Catalog／原power ACK与新鲜State，正常在线退休并恢复十二键、指定固件与临时配置。它不借用§11.55旧BIN代替新护栏运行，也不重复§11.52保存／Start／执行。现行剩余范围取[§11.58](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#1158-有限支持收口后的剩余范围与文档对齐)：本固定图不再有待派发的核心准入门禁；普通默认0槽、GUI／公开分发后置、Wio暂停及Error8待定位保持。
