# Device V2：现有任务与 Sketch 周期／延时收口

更新：2026-10-15。V4／V5既有ESP32四槽复测及恢复保持关闭，取§10.8。R4共享owner／预算取§11.10，最新实物未知写入及待复电／十二键恢复取§11.11，X1未关闭。Wio共享QSPI与四槽产品软件取§11.12；当前§11.13补齐固定16 KiB维护／受限恢复、存储专用探针及主机防重放工具。现场仅COM9 R4、COM11 ESP32，无Wio；本批不操作串口或刷板。下一实物批先保护原固件及精确区域，确认独占，再做存储准入，不能直接跳到任务App。普通通信、实时已通过项不重派，H1只补平台Cloud State差异，不扩槽或重造执行链。

## 1. 目标与边界

本阶段让用户更容易使用现有定时任务，并为 Sketch 提供小型、非阻塞的周期／延时工具。两者不是同一执行器：

| 能力 | 时间与生命周期 | 存储／授权 | 本阶段做什么 |
| --- | --- | --- | --- |
| App 管理的持久任务 | 倒计时用单调时间；日期／星期用有效业务 UTC | 复用现有 snapshot、journal、grant、请求恢复 | 修正输入边界，补设备授时状态观察和使用说明 |
| Sketch 周期／延时 | 本次开机的 `millis()`；由 `loop()` 协作驱动 | 不写 Flash，不依赖 App、网络或授权 | 新增按需实例化的固定容量调度器 |
| 实时数据采样 | 现有授权租约、实际协商频率 | 沿用 Telemetry lease | 不改；不得用周期定时器绕过租约持续上传 |

自定义控制页面完整接入 V2 由同事处理，不修改其编辑器或页面模型。BLE 性能作为下一独立优化；自动化扩展与 H2 周期历史各开小阶段。OTA、设备离线历史／Bulk、复杂规则、新网关协议继续后置。

不扩任务槽、不升级 SDK、不创建第二个任务执行器、不修改三端线协议、不自动开放未准入平台。原账号、授权、请求日志和无关本地修改必须保留；本阶段没有提交／推送或刷板的默认动作。

## 2. 已有能力，不再重复实现

基线：设备库 `c4b9f3a`、App `ffb9b5b`、IoT Service `268a1d1`。这些是本阶段前的 Git 基线，不等于生产配置或本次新增 API 已验收。

- 持久任务已有 Countdown、Once、Weekly、原子快照、执行结果 journal、查询／未确认恢复和显式 Start。
- App 已显示真实容量、任务来源、名称／别名、倒计时重启中断及查询失败恢复。不要新增一套相同管理页面。
- 默认源码任务容量为 0；已准入候选是 ESP32 手动 WiFi 的 4 槽试点，匹配维护者 profile 和 Service allowlist。核心上限 10 不是默认可用容量。
- 纯 BLE、WiFi+BLE、R4/Wio 任务产品准入不因本阶段新工具自动开放。Sketch 定时器则不依赖这些任务准入。
- `enableTime()`／`time()` 已复用原 WiFi／BLE 业务授时；缺少的只是普通 Sketch 对“未授时／过期／来源”的观察入口。

持久任务设计继续以 [执行器设计](DEVICE_V2_TASK_EXECUTOR_DESIGN.md) 为准；实时/H1 以 [实时与历史计划](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md) 为准。

## 3. 本阶段实施顺序

| 步骤 | 最小内容 | 验收要求 | 状态 |
| --- | --- | --- | --- |
| T3-1 | 冻结两类计时边界和后续顺序 | 不扩槽／准入／线协议；旧进度入口指向本页 | 本文已冻结 |
| T3-2 | 现有任务输入保护和授时可观测性 | 毫秒精度倒计时不被浮点误拒；空时区不变成 0；授时快照复用原服务 | 源码／回归完成 |
| T3-3 | 固定容量 Sketch 周期／延时 API | 单调时间、取消句柄、无补发风暴、回调安全、无 heap | 源码／回归完成 |
| T3-4 | 回归、资源和 Arduino 使用验证 | C++11 host、32 位槽位／对象预算、真实 SDK 语法检查；说明结果不冒充完整链接／实物验收 | 有限软件／ABI检查完成 |
| T3-5 | 必要的有限真机短验收 | 先完成匹配 Arduino 固件链接，再测周期／延时、通信推进、取消／重启语义；不重复长 BLE 循环 | COM3／真实 App 短验收及恢复完成，见§7.3 |

T3-2/T3-3 的软件通过不表示普通持久任务已全面产品化。扩大任务准入前仍需逐平台存储预算、断电恢复、可信时间来源和匹配服务授权验收，不在本批绕过。

## 4. Sketch API 合同

采用独立、可选 `<BlinkerTimer.h>` 与 `blinker::Timer<4>`，`Timer` 不持有 `Blinker` 或任何连接。用户在 `loop()` 依次调用 `Blinker.run()` 和 `timers.run()`，与常见协作式 Arduino 工具一致。不往每个全局 `Blinker` 对象加 timer 指针或固定最大槽。

```cpp
#include <Blinker.h>       // 也可以选 BlinkerBLE.h / BlinkerWiFiBLE.h
#include <BlinkerTimer.h>

blinker::Timer<4> timers;
blinker::TimerHandle delayed;

void sampleSensor() { /* 短小非阻塞采集，只更新本地缓存 */ }
void finishPulse() { /* 结束本地输出脉冲 */ }

void setup() {
    // 省略原来的组件注册和 Blinker.begin(...)，它们不变。
    const blinker::TimerHandle sample = timers.setInterval(1000U, sampleSensor);
    delayed = timers.setTimeout(500U, finishPulse);
    if (!sample.valid() || !delayed.valid()) { /* 处理参数错误／容量不足 */ }
}
void loop() {
    Blinker.run();
    timers.run();
}
```

- `setInterval(ms, callback)`／`setTimeout(ms, callback)` 返回句柄；也支持 `callback(void*) + context`，上下文必须存活到取消／执行结束。
- `cancel(handle)`、`active(handle)`、`clear()`、`size()`；取消旧句柄或其他实例的句柄不得影响新任务。
- `handle.valid()`只表示曾成功发出句柄；是否仍在调度以`active(handle)`为准。句柄不能跨对象销毁复用；generation达到`UINT32_MAX`后拒绝新注册，既有任务仍可运行，不回绕重用旧编号。
- 容量由模板明确选择；范围 1～255，每槽固定内存。参数为整数毫秒，合法延时 1～`INT32_MAX`；空回调、满槽或非法延时失败，不覆盖旧任务。
- `run()` 返回本轮实际调用次数。一次性任务在调用前释放；周期任务保留原相位，错过的周期跳过，每槽每轮最多调用一次，不追赶发送。
- 本轮回调中新建的任务留到后续 `run()`；允许取消自身、取消其他槽或清空。嵌套 `run()` 不递归触发。
- 单线程／同一 `loop()` 使用，不是 ISR、RTOS 或硬实时调度器。回调阻塞仍会阻塞通信；必须持续驱动，观察间隔小于 `2^31 ms`。
- 开机／重启重新注册，不恢复上次剩余延时；对象生命周期独立于 `Blinker.end()`、断线或授权变化。深睡／不连续的 tick 不承诺续期，应清空并重新注册。
- 不自动上报状态、不存储业务任务、不创建 App 可编辑任务、不用于凭据过期或跨设备自动化。

便携内核 `SketchTimer<Capacity>` 只注入单调时间函数；Arduino 包装只适配 `millis()`。协议、持久任务、网络与平台不依赖该调度器，可独立测试和移植。

## 5. 现有任务的有限修正

App 输入先检查空值和精度，再转换成共享合同需要的整数毫秒。`1.001 秒`应准确表示 `1001 ms`；真正非毫秒精度不能静默舍入。空时区必须报错，用户明确填写的 `0`合法。服务和设备的最终校验继续复用原合同。

设备新增 `Blinker.timeStatus(WallClockSnapshot&)` 只读取既有业务时钟。返回值表示当前是否有效 UTC；返回 false 时仍可观察 `Unsynchronized`／`Stale`，输出每次初始化，不能沿用上次有效值。`source`只是来源，不是安全保证。该接口不启动第二次授时、不设系统 RTC、不增加 facade 字段。

日期／星期任务必须等真实设备时间有效；App 当前没有收到设备任务时钟状态时继续显示“未知”，不得以手机时间或 MQTT 在线推断就绪。本批不新增目录字段；后续若确需 App 状态联动，单独设计窄的状态合同。

Countdown 与周期／延时不依赖 UTC；持久 Countdown 重启后仍按现有 Interrupted 处理，不自动重新 Start。固件注册和 App owner 初始化是不同动作，`enableTasks()`不能当成授权。

## 6. 验收和后续顺序

软件检查覆盖输入边界、取消／重用／跨实例、时间回绕、错过周期、回调注册／clear／重入、时间未启用／未绑定／过期／恢复。编译使用现有 SDK，不升级；检查新增对象的 32 位内存，既有 `BlinkerClass <= 12 B` 不变。

下一阶段只在本阶段结果清楚后推进：

1. BLE 性能：复用既有 MTU／数据长度协商和 20 B 回退，按阶段计时定位真实瓶颈，不先做长循环。
2. 最小自动化的明确缺口，另开小阶段；不重造规则执行链，不扩复杂条件。
3. H2 周期历史，另行冻结采样频率、保留策略和成本；不搭车实现设备离线存储／Bulk。

自定义控制页面与以上阶段并行由同事维护。新增功能前先处理可复现缺陷；通过的通信／分享／恢复检查不自动重派。

## 7. 本批结果

### 7.1 已实施

- 便携内核`src/BlinkerV2/core/SketchTimer.h`只依赖整数和函数指针；`BlinkerTimer.h`为可选入口，Arduino层单独适配`millis()`。未加外部库、`.cpp`责任单元或全局固定槽。`run()`用饱和generation前沿判断新注册任务，不使用按容量增长的临时栈数组。
- 新增`Blinker.timeStatus()`，只读现有时钟，不增加facade字段；公共回归验证未启用／未绑定、有效、过期、重新授时、清除后状态。既有`time()`和`lastError()`语义不变。
- App仅替换现有任务编辑的两处输入转换：整数毫秒round-trip修正浮点误差；时区空值／非整数拒绝，显式0保留。协议、页面模板、任务槽、执行链和Service不变。
- README和canonical API补最短使用说明；总主线及执行器旧“未来timer”入口改为本文。不修改同事负责的自定义控制页面，也不删除无关本地文件。

### 7.2 软件与资源验证

| 检查 | 实际结果 | 边界 |
| --- | --- | --- |
| Host Release构建＋CTest | 73/73通过，含新timer及公共授时状态回归 | 软件测试，不是硬件定时精度 |
| App任务页面／模型／任务client | 3 suites，92/92通过，新增29个输入边界案例；spec TypeScript通过 | 软件输入检查；真实App结果另见§7.3 |
| ESP32S3 / UNO R4 / Wio现有SDK | 3/3通过当前头文件语法、32位ABI及便携内核严格C++11检查 | 使用现有3.3.11／1.5.3／1.8.5，不升级；仅语法，不是完整固件链接 |
| timer资源 | 32位每槽24 B；1／4／8槽对象36／108／204 B；句柄12 B | 只为实际定义的对象占用；不是持久任务槽 |
| 默认facade资源 | 32位`BlinkerClass`仍12 B | 未使用timer的Sketch不增加timer对象 |
| 架构／profile／包边界 | 通过；单库清单最终419项；发布回归35/35通过 | 本轮不生成／安装新ZIP，不重称七目标全编译 |

复现入口：`tests/host/CMakeLists.txt`及`tests/arduino/sketch_timer_compile_gate.py`（传既有`compile_commands.json`和专用`build`目录）。SDK gate将当前`src`置于旧安装源码之前，保存输入hash、结果与日志；不把旧安装源码当新API通过。Host测试包括回绕、最大延时、无补发、取消、旧token、上下文、清空／重入／后槽新注册；新增API测试使用真实TimeSync编解码和nonce校验的软件链路。

初次架构检查拒绝了Arduino适配出现在入口层，随后将其迁回Arduino层；冻结记录只经审阅更新Facade的只读`timeStatus`变更，其他登录hash不变。新增授时恢复测试曾被重复nonce的测试RNG拒绝，修正fixture后最终73/73通过；没有修改产品nonce校验。中间失败不算通过记录。

上述软件检查阶段未部署或刷板；后续有限实物操作和恢复取§7.3。SDK未升级，也未暂存／提交／推送。App三个既有无关本地文件、库的既有删除与空`partition.csv`保留原样。

### 7.3 COM3 与真实 App 短验收

本机沿用 Service 051 和既有镜像，用真实 App 新建一个专用测试身份；只临时准入该身份的四槽任务，不启用或 Start 持久任务。COM3 核对身份后完成本次新鲜 8 MB 全 Flash 备份及设备校验，App 原 APK、账号与请求日志另行备份。匹配 App 更新未清数据；其他板未刷写或清授权。

| 检查 | 实际结果 | 边界 |
| --- | --- | --- |
| 匹配固件完整链接 | 既有 ESP32 3.3.11，当前 410 个源码输入；BIN 993,456 B，静态 RAM 54,168 B | BIN 小于实物 app0 的 1,310,720 B；编译配置的 3,342,336 B 容量不能代替实物准入 |
| 周期和延时 | 一个 6 秒窗口：250 ms 周期执行 6 次；上下文一次延时仅执行 1 次，首次为 1001 ms | 有限协作式测试，不是硬实时精度承诺 |
| 取消和错过周期 | 提前取消的回调 0 次；400 ms 周期在第 3 次自取消；显式取消后计数不变，最终活动槽 0 | 仅暂停 timer 轮询，通信持续推进；恢复一轮只执行 1 次，没有补发风暴 |
| 通信同时推进 | 窗口内 `power=false` 获真实 ACK 200；设备写计数 1→2，窗口内 loop 推进 56,594 次 | 数值 50 控制在窗口前、75 控制在窗口后通过 ACK／State，不称数值控制也在窗口内 |
| 原控制与状态 | 两次布尔、两次数值控制均有 Broker 真实 ACK；最终 State revision 5、`power=false`／`duty=75` 与设备回调一致 | 不把本地控件变化代替设备回执 |
| 重启语义与授时 | 显式重启后周期／延时计数与活动槽均为 0，未自动续跑；既有云端业务 UTC 恢复有效 | 仍需 Sketch 重新注册；不修改持久 Countdown 的 Interrupted 语义 |
| 毫秒精度任务输入 | 真实 App 保存停用 Countdown，设备回执和读取均为 `durationMillis=1001` | 沿用原任务创建／查询链路，不 Start |
| 空时区与显式 0 | 清空时区后页面拒绝保存，任务 HTTP 请求 0 条且表未变；显式 0 的 Weekly 停用定义获真实 replace 回执并读回 | 不能把空值转换成 0；仍由原协议校验执行 |
| 清理与离页 | 真实 remove 回执及读取确认任务表为空、revision 3；18 个任务请求与 18 个 attempt 全部 verified，无处理中项；离页后页面 group 和本地 BLE 连接资源为 0、任务页面已回收 | 用户 MQTT 常驻不据此关闭；保留完整请求和回执，不回滚数据库，不重放已确认请求 |
| 恢复 | 从本次备份恢复 `0x9000` 起的 1,339,392 B 原 NVS／启动状态／app0 范围，设备 digest 校验并正常复位；原授权恢复 | 未改 bootloader／分区表；这是精确范围恢复，不称恢复后重新读回整片 8 MB |

应用 BIN SHA-256 为 `789147c00fd41e9dc4369bcde47bc296abcf86a60a3cd6249a0b0d1862264dee`。可复用的有限实物探针在 `tests/internal/arduino/Esp32SketchTimerTaskGate/`；内部配置需匹配现有任务 profile，不纳入公开 Arduino 示例。原生产存储命名空间未被候选使用；临时身份、网络与任务存储使用隔离命名空间，随原范围恢复撤回。

Gateway 与 Broker1 的临时任务准入各从 2 项恢复为原 1 项，全部请求与精确回执保留。本批新建测试设备通过正常 App 入口移除，原 19 台设备的 ID／UUID 及账号保留；服务将该身份退役并撤销 Key。只读核对确认 18 条已验证请求、18 个 attempt 和精确回执与恢复后的基线逐项一致，只有目标 scope 的退役状态及对应事务时间戳变化。App 测试后完整数据另行归档并校验，只撤回本批新增 USB 转发，不恢复旧数据覆盖请求日志。

只将必要的三项测试网络配置对齐到本机当前地址并保留，不恢复已不存在的旧地址；服务镜像、SDK、验签材料、其他容器和数据库未升级或回滚。上述测试身份和审计记录不作为生产发布或全部平台准入。

中间失败保留：首次全 Flash 读取因 USB 中断未生成有效备份，核对进程结束后另做新鲜完整读取并校验；滑块右边界坐标未发出 100 控制帧，该动作不计为通过，随后正常位置的 75 控制独立获 ACK。离页夹具在导航约 122 ms 时过早检查旧页面关闭，独立只读复核确认任务页面、页面 group 和本地 BLE 资源均已回收，没有重放导航／业务请求或为通过门禁改动产品生命周期。

T3-5 关闭的是本次 ESP32／真实 App 有限短验收，R4／Wio 的新增 API 仍只取§7.2 的语法与 ABI 结果。默认持久任务容量仍为 0，原四槽试点准入不扩大；本批未生成新交付 ZIP，也未暂存／提交／推送。

### 7.4 后续最小阶段

后续独立 [BLE 首连分段优化](DEVICE_V2_BLE_FIRST_CONNECT_SEGMENT_EXECUTION_PLAN.md) 的 B1～B4 已限定收口：统一既有窗口收尾、四槽背压回归和 COM3 同条件 A/B／B 控制短验收通过，原固件／授权和普通 App 已恢复。单对 2531→2168 ms 不作为全平台收益，普通默认仍关闭窗口，不追加循环。现有 MTU／数据长度协商、20 B 回退和安全链不变，不升级 SDK、不扩槽或重造执行器；自动化扩展和 H2 继续按§6 后置。

## 8 必要源码与安装链接收口

当前单库已有 419 个成员、410 个运行时源码，新增三个 Timer 头和只读 `timeStatus()`；上一轮七目标 ZIP 为 416 个成员、407 个源码。旧安装结果不能代替新 API 的交付证明。本节补这个缺口，不把已经通过的有限实物阶段重新改成未完成。

| 步骤 | 最小范围 | 状态 |
| --- | --- | --- |
| D1 | 审阅必要差异、修 UTC 默认负零、复跑 App 累计定点回归 | 151/151、两套 TypeScript、四文件 lint 通过 |
| D2 | 当前源码冻结、普通公开 profile 的单一 ZIP、隔离安装七目标完整编译 | 7/7 通过，关闭；所有目标均取本批新安装目录 |
| D3 | 同一安装库中 S3／R4／Wio 的现有 Timer Probe 完整链接与来源检查 | 3/3 通过，关闭；真实回调／timeStatus 符号及原产物摘要核验通过 |
| D4 | 结果与旧进度入口对齐，保留无关修改；撤回内部编译 ZIP | 整体审计通过、ZIP 已撤回，关闭；未提交／推送、部署或刷板 |

D1 发现手机处于 UTC 时 `-getTimezoneOffset()` 产生 `-0`，被共享任务合同拒绝。仅将默认初始化改为 `0 - getTimezoneOffset()`，得到 `+0` 且不把 `NaN` 吞成合法零；显式 `-0` 仍由原合同拒绝。新增两项真实 editor 定义／保存回归先复现失败，再修正通过。七套 App 合计 151 项，其中 BLE 57、任务 94；不累计重复子集。

首次联合测试的两个 resume 保存快照断言因 `structuredClone` 跨 realm 的 typed-array 原型比较失败，值相同但框架不认为深比较相等。只改测试为逐字节比较并检查 buffer 独立，保存先于同步、原 secret／presenceKey 清零和成功／失败顺序断言不变；没有为测试改 BLE 生产链或共享合同。原失败日志与最终 green 分开保留。

D2 沿用 stock ESP32 3.3.6（WiFi／CloudSmoke）、S3 3.3.11（BLE／WiFi+BLE）、Renesas UNO 1.5.3（WiFi／BLE）和 Seeed SAMD 1.8.5（WiFi）。每目标单 worker，从新的隔离 sketchbook 安装同一 ZIP，核对实际 SDK／FQBN、安装字节、编译来源、map／nm、依赖闭包及未选功能剥离。只用既有 SDK 与固定版本依赖，不增加 Wio BLE、R4/Wio WiFi+BLE 或持久任务准入。

D3 复用 `SketchTimerProbe`，只编译链接独立 Timer 及未绑定 facade 的时间快照读取；不启动产品、联网、授时或持久任务。新增窄门禁复用原安装字节／provenance／nm 检查，而不是重造打包器；R4 的 Preferences／WiFiS3 是既有存储实现编译依赖，需与最终链接功能区分。普通七例程编译和三平台新 API Probe 链接分别计数，不冒充全部平台实物验证。

源码以 Git 工作树为准，不造同事交接包。内部 ZIP 只用于安装测试，验收结束已撤回这个可重建文件；忽略目录中的日志、摘要、安装库、SDK 和既有私有备份保留。公开配置继续使用 `.invalid` 占位服务和测试公共验签材料，无现场账号、DeviceKey、网络密钥或私有 Flash；编译通过不使该配置成为可上线包。服务源码与镜像沿用，不重复 App／服务部署、恢复或 BLE 循环；自动化扩展和 H2 仍独立后置。

### 8.1 软件与安装链接收口结果

本批 App 151/151、设备 73/73 host、45/45 发布及门禁回归通过；类型检查、定点 lint、架构、公开 profile 和既有 SDK pin 检查通过。架构检查先拦住上一批内部 `Esp32SketchTimerTaskGate` 对默认 `Blinker.h` 的依赖，只改为等价的显式 `BlinkerWiFi.h`，未增加例外或改变产品、Sketch 业务逻辑；首次失败保留后另冻当前输入。

419 成员、410 源码的内部安装 ZIP 为 698,129 B，SHA-256 `e98bd407d32ae3f624cce4c4ff0751dfd4a205cfd47ca7b26cf684c0943cae4c`；同字节重建、安装字节及七目标的来源／链接剥离检查通过。stock ESP32 WiFi／CloudSmoke、S3 BLE／WiFi+BLE、UNO R4 WiFi／BLE 和 Wio WiFi 共 7/7 通过，D2 关闭。三平台独立 Timer／timeStatus 完整链接共 3/3，真实回调和方法符号保留，产品、UTC、持久任务及未选扩展运行链未混入，D3 关闭。两条协调进程均正常退出，没有重放已通过构建；各目标冷编译耗时是本机编译表现，不是设备性能。

| 安装目标 | 实际 SDK | 程序大小 B（CLI） | 静态 RAM B |
| --- | --- | ---: | ---: |
| ESP32 WiFi | ESP32 3.3.6 | 995,959 | 50,920 |
| ESP32 CloudSmoke | ESP32 3.3.6 | 1,013,447 | 51,024 |
| XIAO S3 BLE | ESP32 3.3.11 | 695,108 | 36,008 |
| XIAO S3 WiFi+BLE | ESP32 3.3.11 | 1,283,992 | 56,660 |
| UNO R4 WiFi | Renesas UNO 1.5.3 | 139,888 | 13,060 |
| UNO R4 BLE | Renesas UNO 1.5.3 | 196,092 | 16,996 |
| Wio WiFi | Seeed SAMD 1.8.5 | 144,240 | 68,080 |

Wio 的 CLI 未提供 RAM 摘要，68,080 B 取实际 ELF 的 `.data=736 B` 与 `.bss=67,344 B`；其他 RAM 数值取 CLI。以上为当前例程的静态编译结果，不是库的独占成本或运行峰值；SDK 分区容量也不能代替实物分区检查。普通任务容量仍为 0，四槽试点准入不变。

最终审计绑定同一 ZIP 与原候选摘要／大小／清单／profile，核对既有组日志摘要及每目标 17 个源码单元的准确集合；437 项源码／门禁输入保持。普通目标产物摘要在最终审计时捕获，不称为与早期产物摘要比较；三项 API 则逐项比对原报告的输入与输出摘要。审计及实际结果经独立只读复核，未增加产品源码改造、升级 SDK 或放宽门禁。最多两个 SDK 组并行、每目标一个 worker；其他链已在运行同一目标时只等待并重验，不覆盖日志或重复构建。

复核入口为忽略目录 `build/t3-ble-installed-20261013/explicit-wifi-probe/` 的各目标日志、`frozen-inputs.json`、三项 `timer-api.json`、`wio-elf-size.json` 与 `installation-acceptance.json`；最终汇总 SHA-256 为 `9629d8bb3bfb6bc27ff9801731e6444ca956dab7fc53a5375b066bbf9040100b`。三端 HEAD／已暂存条目、无关本地修改及四项 App 输入核对保持；服务工作树干净。本批未部署、刷板、提交或推送。整体审计后只删除上述 698,129 B 的内部 ZIP，文件可按冻结源码和公开 profile 重建；没有删除日志、安装库、SDK、授权或备份，D4 关闭。

本节 D1～D4 已按限定范围完成；随后 R4／Wio 独立 Timer 实物结果取§9。下一步可整理必要源码与方案差异，提交／推送需另按用户指令；正式服务配置、同事独立验收及未覆盖的网络／时间来源组合仍分别处理，不自动启动自动化扩展、H2、OTA 或 Bulk。

## 9 UNO R4 和 Wio Timer 真机短验收

本批仅使用 COM4 的 UNO R4 WiFi 和 COM6 的 Wio Terminal；COM11 的 ESP32 不动。复用§8 同一安装库和既有 Renesas UNO 1.5.3／Seeed SAMD 1.8.5，不升级 SDK。新增 `tests/arduino/SketchTimerLiveProbe/` 是有限内部测试，不新增用户示例或运行时实现：开机不自动注册，串口 `s` 每次开机只启动一个六秒观察窗口，`j` 只查询一次。注册后 1500～2250 ms 只暂停 Timer 轮询，2500 ms 起显式取消周期；周期不是持续运行完整六秒。它只调用独立 Timer 和未绑定 facade 的 `timeStatus()`，不启动产品、网络、真实 UTC 或持久任务。

| 检查 | UNO R4 WiFi | Wio Terminal |
| --- | --- | --- |
| 完整链接与应用区 | BIN 55,148 B，程序 CLI 55,140 B，静态 RAM 7,752 B；应用区起点 `0x4000`，上限 245,760 B | BIN 34,772 B，程序 CLI `.text` 34,132 B，ELF `.data+.bss` 3,664 B；应用区起点 `0x4000`，上限 507,904 B |
| 周期与一次延时 | 250 ms 周期在有限窗口执行 6 次；1001 ms 上下文延时首次在 1001 ms 执行，仅一次 | 同一周期执行 7 次；同一延时首次在 1001 ms 执行，仅一次 |
| 容量和取消 | 第五个注册拒绝；旧失效句柄不能取消复用槽；提前取消回调 0 次；400 ms 周期第 3 次自取消 | 相同检查通过 |
| 暂停和恢复 | 只暂停 Timer 轮询，期间串口查询成功；恢复一轮周期仅执行 1 次；窗口内 loop 推进 1,139,519 次 | 相同检查通过；窗口内 loop 推进 2,134,097 次 |
| 窗口结束 | 显式取消后计数不变，最终活动槽 0；同次开机第二个 `s` 拒绝，未用 `clear()` 制造结束结果 | 相同检查通过 |
| 重启和未绑定授时 | 窗口结束后软件复位，槽和计数均为 0，探针未自动重新注册；`timeStatus()` 返回 false／Unsynchronized／None，并清除输入中的旧字段 | 相同检查通过 |
| 原固件恢复 | 全 256 KiB 代码 Flash 与新鲜备份逐字节一致，8 KiB Data Flash 摘要一致，启动区保留 | 全 512 KiB 主 MCU Flash 与新鲜备份逐字节一致，启动区保留 |

周期次数是上述暂停／取消窗口内的观测值；两平台均验证每轮最多执行一次，不从次数差异推导精准频率或任务丢失。没有逐回调时间戳，不从单个样本推导相位、精度或硬实时能力；loop 及串口持续推进也不等于 WiFi／MQTT 并发通过。复位发生在窗口结束后，不能代替运行中重启或断电持久化测试。回绕、跨实例、回调重入等仍取原 host 回归，不外推本次硬件覆盖。

刷入前分别新鲜备份，并第二次从硬件读取核对主 MCU 字节；R4 另核对 Data Flash。两候选刷入后读回匹配，17 个安装实现单元及真实回调／`timeStatus()` 符号校验通过，439 项输入与 419 个安装文件保持。候选使用新的目标构建目录、单 worker、允许既有 SDK cache；未单独测量 cache hit，不称本批全 SDK 冷构建。R4 编译依赖中的 Preferences／WiFiS3 不代表本探针调用存储或无线功能；Timer 本体不使用 heap，不据此宣称整个 SDK 固件无动态分配。

恢复只写各自主 MCU 的 `0x4000` 起应用区；没有写启动区、R4 ESP32 桥接固件／Preferences 或 Wio RTL 固件／外部 QSPI 存储，没有清授权。这些外部授权存储不在主 MCU 备份中，也没有额外读取它们作逐字节证明；主 MCU 全片一致不能冒称全板授权备份／验证。本批没有部署 App／Service、修改网络、扩持久任务槽或准入、暂存／提交／推送。

R4 的首次候选上传报 `Device unsupported`，随后读取确认原代码 Flash 和 Data Flash 完全未变；用户仅拔插 R4 后标准上传及校验通过。恢复原固件时另有一次 `No device found on COM4`，没有进入写入；再次拔插后复用与备份严格相等的原应用片，标准上传及最终完整读回通过。两次失败日志保留，没有盲目循环、改 SDK 上传器、强制整片擦除或升级桥接固件。

证据保留在忽略目录 `build/timer-platform-live-20261013/`：两个 `*-compiled.json`、`*-runtime.json`／原串口日志、`*-reboot.json`、`*-restore.json` 及对应上传／读回日志；私有 Flash 备份不提交。R4 候选 SHA-256 为 `d3ebb75fec3207638c613ea33d943086465389917d38221c841cfe44c6961909`，Wio 为 `c835cbcfe08af5fa935c1ff910f3013906c24a94f8fa7746a482eda8c1651e4f`。本节限定实物项关闭；现有服务授权、默认任务容量 0 和四槽试点边界不变，未覆盖的真实 UTC 来源、网络组合及 Wio BLE 正式发布不据此关闭。

## 10 普通通信与持久任务短验收顺序

当前继续新的 WiFi／BLE 联调，不重复§9独立 Timer 窗口。用户已选择先复测既有 ESP32 四槽持久任务试点；R4／Wio 的任务实现另在普通通信验收完成后推进，不能仅改容量宏绕过平台和服务准入。

| 顺序 | 最小范围 | 通过条件 | 当前状态 |
| --- | --- | --- | --- |
| V1 | 核对当前账号、硬件、服务与网络，按授权保护现场 | 备份／恢复范围明确；App 原 APK／账号与日志保留；不使用旧凭据造测试身份 | 已完成；历史 R4/Wio 保护取§10.2～10.6，新 COM9 的限定保护取§10.7；COM11 全 8 MiB 恢复材料在 V4 刷写前经当前 ROM 校验一致，见§10.8 |
| V2 | R4 普通 BLE 匹配当前源码 | Method 2、20 B peer、Manifest／fresh State、属性／按钮真实 ACK、离页回收；既有 Timer 与业务授时短检查 | 限定短验收与原主固件完整读回恢复通过，关闭，见§10.3；未清授权或重新认领 |
| V3 | R4／Wio 普通 WiFi 匹配当前测试地址 | DeviceKey→session→MQTT、属性／按钮 ACK 和 State；Timer／授时共存及有限复位恢复 | 限定范围关闭：Wio 业务／恢复取§10.5；新 COM9 R4 正常上传、业务、手动 RESET 恢复及收尾取§10.7，不重复旧板诊断 |
| V4 | 既有 ESP32 手动 WiFi 四槽持久任务 | 当前候选完整链接、精确试点准入；定义／查询、有限 Countdown 执行、重启中断、删除及原控制恢复 | 限定范围关闭：6 s Completed 持久化、15 s 运行中重启 Interrupted、期限后不续跑、删除及普通控制通过，见§10.8 |
| V5 | 恢复及结果同步 | 按当前授权保留或恢复固件，保护授权和请求记录，不回滚数据库；区分实际通过与待办 | 限定范围关闭：Wio 原固件已恢复，新 COM9 按授权保留验收固件；COM11 原全 Flash 校验、两项临时准入撤回、App／完整日志归档及资源回收完成。新增平台任务不据此准入 |

R4 普通 BLE 公开支持，Wio BLE 仍为 preview。本轮先测 R4 BLE 和两平台 WiFi；Wio preview 短验与未完成的长期发布门禁分开，不用短验宣称正式发布。R4／Wio 的 BLE peer 当前为 20 B，原生 MTU 增大不等于它们已支持 ESP32 的大包协商。

R4 的身份／网络／授权位于连接模块 Preferences，Wio 位于外部 QSPI；主 MCU 备份不能恢复这些数据。需要改变网络或认领时，必须先获取匹配当前存储版本的外部备份／恢复路径，或经审阅使用隔离存储候选；不能拿恢复主固件代替外部授权恢复。保持默认任务容量 0，App 和服务沿用既有链路，不升级 SDK、不扩槽、不重造鉴权或任务引擎。

本批 R4 的有限授权例外已经用户确认：仅 COM4，在必要时通过真实 App 重新认领并保留新的测试授权，不清 App 历史设备或其他板授权；原主 MCU 固件可恢复，但不能承诺恢复未完整备份的桥接旧绑定。该例外不授权改 Wio 的外部存储或将所有测试设备清空。COM11 则须核对当前身份并用 ROM 验证完整恢复材料，任务测试后恢复原固件／授权；仅持有未校验的历史文件或读取中间文件不能算完成备份。V4 采用现场整片验证与已有完整文件一致的方式，不称为又做了一次新鲜全片读取。

### 10.1 普通测试完成后的平台支持顺序

用户已允许在上述测试完成后推进 R4／Wio 的持久任务、实时数据和历史数据支持。实现按已有能力拆分，不新增三套平台执行链：

1. 既有 `onSample`／Telemetry lease 已完成 R4 BLE、R4 WiFi 与 Wio WiFi 的有限频率／来源、离页停止／恢复及原控制验收，取§10.3、10.5、10.7。后续平台任务接线只做受影响路径回归，不重新实现或重跑全部实时检查。
2. H1 状态变化历史复用服务器采纳的 Cloud State 和现有鉴权／分页。R4／Wio WiFi 尚需平台的真实变化、不变值去重、重连基线、范围分页、名称及读取权限短验收；无需设备增加历史缓存。仅本地 BLE 的数据不因此自动成为服务器历史。H2、离线存储和 Bulk 仍后置。
3. 持久任务复用 `TaskEngine`／`TaskSnapshotStore`、时间／目标／写策略接口，只新增平台存储和薄组合。先解决共享资源生命周期、644 B 快照的真实读写／原子恢复、RAM／延迟预算以及匹配签发，再开放精确内部试点。不得把小型凭据银行直接扩大成所有产品默认任务存储。

**纠正 R4 存储假设：**现有四槽快照为 `128 + 4×128 + 4 = 644 B`；260 B 是当前凭据银行序列化上限，不是已证明的 SDK NVS 极限。Renesas UNO 1.5.3 的 `Preferences::begin()`通过 modem 设置当前 namespace，后续 get／put／end 不带实例 handle；不同对象不能假设拥有 ESP32 式独立上下文。采用一个共享后端 owner，在现有 `bl_v2` 中使用独立任务键和有界任务 store；身份键、格式及默认凭据 scratch 不变。是否能可靠传输并提交 644 B 必须实测，不能从函数接受 `size_t` 推断。详见§11。

Wio 身份银行占尾两扇区，任务需要单独预留的原子区；区域归属须先核对文件系统／布局，空白扇区不等于可以挪用。现有 QSPI begin／end 也是共享硬件生命周期，不能让任务 store 的关闭禁用仍在使用的身份端口。所有任务存储必须在普通 transport 停止后仍可用，因为 `TaskProductLifecycle::resetAccess()`先停止通信，再 `clearRetired()`；不能因资源借用使退休清除失败。

任务 grant 已支持 Ed25519／P256 的现有合同；便携 verifier 与当前 ESP32 P256 扩展不同，服务签发必须匹配维护者 profile 和公开信任材料，而非由客户端自行选择算法。先推进 R4／Wio **普通 WiFi**任务薄组合，BLE 任务管理另列阶段；不新建任务线协议或第二个签发／执行链。

V2～V5 的限定结果已完成；R4 内部任务源码已接入，实物准入仍待完成，Wio 尚未接线，执行顺序取§11。功能缺陷由本项目修正，正式材料和同事独立验收仍单列。

### 10.2 当前候选与新鲜备份

内部 `PlatformTransportTimerGate` 使用显式产品入口、原 power／buttonPresses／button／1000 ms uptime 实时字段、`enableTime()` 与独立 Timer；开机不启动 Timer，不清授权，不输出网络／账号／密钥。R4 SDK 1.5.3 单 worker 完整链接通过，BIN 200,452 B、应用起点 `0x4000`，SHA-256 `0c8efb4815be78b283f4252b722fbad0a1151e468ea20279a7ee76b6e7f9529d`。CLI 程序 200,444 B、静态 RAM 17,284 B；不把剩余 SRAM 当作已测运行峰值。17 个安装实现单元、417 个冻结输入和 419 个安装文件核验通过；真实 callbacks／timeStatus／UTC 类保留，任务／MQTT／LAN／网关未混入。当前 Gateway 的公开签名与候选 profile 相符。首次依赖发现前工具引用错历史 profile，修正的是忽略目录中的路径，未改变产品或验签门禁。

原 R4 固件没有匹配已保存的 33 个候选应用；仅确认它与此前原始备份相同。其 BLE 兼容基线没有建立会话，未发送控制，退出资源为 0。Wio 原 WiFi 基线未获取 Manifest／fresh State，观察窗口结束并释放 interest，没有发业务命令。这些失败不代表当前源码不可用，也不作为成功验收。

R4 新候选首次进入上传前，测试脚本 `serial.py` 文件名遮蔽 pyserial，导入阶段被拒绝，未碰硬件；已改为 `r4_serial_gate.py`。随后标准上传在握手报 `Device unsupported`，原代码 Flash 与 Data Flash 再读摘要一致。用户仅拔插 R4 后，标准上传和读回成功，实际结果取§10.3；失败日志保留，没有自动连续上传或更改 SDK。

COM11 本次完整读出 8 MiB 后，旧 OTA 工具的几何检查拒绝继续：实际是 `nvs=0x9000/20 KiB`、`factory=0x10000/5 MiB`、`spiffs=0x510000/2 MiB`，不是原 COM3 的 OTA 布局。随后对精确三项布局独立设定只读守卫，ROM 校验整片备份并恢复正常运行；没有写 Flash、改分区、SPIFFS 或授权。后续候选与恢复必须按此实物 factory 范围，不套用旧 app0 假设。该新鲜备份仍只是后续任务测试准备，未新增任务准入或发送 Start。

当前证据在忽略目录 `build/transport-task-matrix-20261013/`；App 与 COM11 全片数据放在受限私有子目录，R4/Wio 主 MCU 备份同样受限。原账号和请求记录不回滚，未知 Action 不重试；串口共存验收必须在 Timer 尚未结束的真实样本中看到控制变化，不能仅用最终 ACK 和计数推断并行。V2 的实际刷入／业务／恢复结果取§10.3，不据此关闭 V3～V5。

### 10.3 R4 BLE 与现有实时授时短验收

用户仅拔插 R4 后，200,452 B 候选标准上传及完整应用读回通过，启动区和 Data Flash 保留。当前 R4 原有 P1 账户凭据匹配成功，直接完成 Method 2；本批没有清授权、重新认领、签发新 grant 或新增设备。上述重新认领授权没有被使用，原程序基线失败不能当作需要清绑定的证据。

| 检查 | 真实结果 | 范围 |
| --- | --- | --- |
| 首次／离页再进／软件复位后进入 | 4,266／3,650／3,901 ms 就绪，均为 BLE、Manifest accepted、fresh State | 有限样本，不承诺全平台秒开或把差值当优化收益 |
| 包长 | 原生链路 packet capacity 239 B；认证后的 peer 确认 20 B | 不是原生 MTU=239 B；R4 未因原生容量变大而错误使用大包 |
| 实际控制 | power ACK 358 ms、button ACK 329 ms；State revision 3、power=true、buttonPresses=1 与 MCU 一致 | 原客户端／页面方法真实发送，不是 UI 本地赋值；Action 只发一次 |
| Timer 与控制共存 | 250 ms 周期 9 次；1001 ms 延时首次 1002 ms、仅一次；提前取消回调 0 次，最终槽 0 | 两项控制变化都在 `started=1 && finished=0` 的 MCU 串口样本中出现；不是只用最终 ACK 推断并行 |
| 既有实时产品 | App 显示“蓝牙直连 · 实际 1 秒/次”，fresh、missed=0；首次曲线观测 156 点，最近到达间隔 996～1018 ms | 复用原 onSample／lease；300 点上限不变，实际到达间隔不等于硬实时采集精度 |
| 离页停止／返回 | groups／links／sessions 均回收；MCU samples=158 在两次相隔约 81.1 秒的观测中不变；返回后新租约采样恢复 | 用户 MQTT 会话保持，不将本地断开等同于账号退出 |
| 真实 App 后台／前台 | Android Home 后 samples=160 在约 44.2 秒的两次观测中不变；前台自动恢复，14 个只读样本均 ready；后续 samples=222、曲线 62 点且 fresh | 未强杀进程，不从延后观测反算自动恢复 SLA |
| 业务授时和复位 | 已连接时 `timeStatus()` 有效、source=Controller；软件复位先观察到时间／Timer／采样计数为 0，再连接后时间及采样恢复，Timer 未自动注册 | 不是 R4 Cloud 授时、无源长期准时或任务掉电验收；原 Sketch 控制值复位为默认 false／0 |

前台返回后第一次立即读曲线早于本地重新就绪，被测试前置拒绝；没有重发控制或造 State，随后只读观测确认真正恢复。首次上传的脚本命名冲突、标准上传握手失败及首次恢复时 `No device found on COM4` 均保留；握手失败没有进入写入。用户仅拔插 R4 后，标准上传恢复原应用，随后完整读回确认 262,144 B 代码 Flash 与本次原备份逐字节一致，8,192 B Data Flash 摘要与原备份一致。启动区未改，未写连接模块固件或清授权；这不等同于对桥接存储做完整备份或恢复。

本轮 V2 已按有限范围关闭。恢复记录为 `r4-restore.json`，对应标准上传及完整读回日志均保留；不重复已经通过的业务测试。下一步按 V3 核对 R4／Wio WiFi 的当前网络与外部存储保护，再做有限通信闭环；V4 使用已备份 COM11 的实物 factory 布局复测既有四槽试点，不提前扩展 R4／Wio 持久任务。

本轮 App 沿用此前已恢复的普通 `f2bc1828…` 候选，未安装新 APK；不据此宣称软件阶段的 UTC 默认 `+0` 修复已部署。前后完整 App 数据归档分别为 7,309,312／7,312,384 B，并校验读回；只撤回本批拥有的 USB 转发，不回滚账号、请求日志或数据。Wio 主固件未刷写，COM11 只完成新鲜全片备份／验证／正常恢复，没有开启任务准入或 Start；V3／V4 仍待办。

### 10.4 WiFi 外部存储保护与网络变更后的验收

V3 先增加有限内部 `PlatformStorageBackupGate`，不改生产存储或 Sketch API。R4 仅双读／恢复 `bl_v2` 的十个库所属原始键，保留 absent、journal 及其他键；Wio 仅双读／恢复固定尾部两扇区的完整 8,192 B，包含旧格式和 padding。两者均要求有效原设备身份、CRC 与已知格式，不清 namespace、不 format、不写连接模块固件；一次恢复意图发出后，结果不明必须停下核对，不能自动重发。恢复主 MCU 之前须先完成外部存储的独立双读确认，主 MCU 备份不冒称全板授权备份。

| 检查 | R4 | Wio |
| --- | --- | --- |
| 维护程序完整链接 | BIN 59,344 B；静态 RAM 10,132 B | BIN 34,804 B；静态 RAM 19,816 B |
| 实物原始存储双读 | 十键编码 978 B，CRC／私有文件读回核验通过 | 固定两扇区 8,192 B，CRC／私有文件读回核验通过 |
| 普通 WiFi 候选完整链接 | BIN 144,536 B；静态 RAM 13,372 B | BIN 149,624 B；ELF `.data+.bss` 68,384 B |
| 业务验收 | 尚未取得 fresh State，未发 power／button 或启动 Timer | 相同；不能因编译和烧录通过而关闭业务项 |

维护程序真实四个头文件的 native 回归 3/3、535 条持续生效断言通过，覆盖 CRC、明确 absent、RPC 失败、旧格式、精确 raw 恢复与失败停止；不代替真实擦写或断电原子性证明。本轮 R4／Wio 外部恢复前后均与原备份相同，属于 no-op 恢复及独立双读，不称为实际 NVS 写删或 QSPI 扇区擦写门禁。普通两候选复用当前 419 个安装成员、410 个运行时源码，仅投影三个已有公开 profile 头；原安装不变，仍使用 Renesas UNO 1.5.3／Seeed SAMD 1.8.5、单 worker、17 个实现单元与原链接闭包检查。不据此扩大普通任务容量或 Wio BLE 发布准入。

R4 原始存储实例经真实 App API 解析到同一已有 BLE 设备，而非名称相似的旧 WiFi 记录。经用户允许，对这个相同逻辑设备正常开启云接入并保留配置，仍为原 19 台设备，没有新建、轮换 Key、清 BLE 授权或用 MAC 替代身份。两板的 Key 均从原 App 管理接口正常 reveal，网络、Key、原始 Flash 和日志仅留受限私有目录。

两候选冻结的是 mostfun2026／旧测试地址；长编译期间电脑和手机改连“带翅膀的蜗牛”，服务仍绑定旧地址，设备页因此未就绪。这轮失败不证明设备库缺陷，也不算 DeviceKey→MQTT／ACK／State 通过。目标页面连接已收回；Wio 外部存储与完整主 MCU 已恢复；R4 外部存储双读与原备份一致，原主固件标准上传后完整 262,144 B 代码 Flash 与 8,192 B Data Flash 读回确认。上传握手失败留日志，用户单板拔插后有限重试，不更改 SDK 上传器或整片擦除。

下一步沿用户确认的“带翅膀的蜗牛”，只调整 Plain MQTT publicHost、18884 和 17002 的测试绑定，保留镜像、数据库、账号、授权和完整请求日志。新阶段独立冻结私有输入并按原源码重编译，不覆盖旧候选／proof；烧录前和进入设备页前再次检查当前 SSID／电脑地址、手机网络、服务绑定／可达性、公开验签材料与候选摘要，变化即停止，不自动切网或重发控制。V3 的两平台有限业务闭环通过后才进入 V4；R4／Wio 持久任务继续按§10.1 后置。

测试地址三项已对齐到当前本机地址，两个目标均 healthy；完整前置配置／日志已私有归档，镜像、源码、公开验签材料、挂载／卷及其他容器保持。Gateway 更新后，旧检查因 Docker 的 `com.docker.compose.replace` 来源标签变化停住；仅确认新标签精确等于原容器 ID 后，独立只读审计归一该 metadata，其他守卫不变。随后只继续尚未执行的 HTTP 绑定，未重放 Gateway 更新或改数据库。新网络准入 8/8、网络范围 5/5、来源标签审阅 4/4 的有限回归分别通过；这些是工具回归，不算设备业务验收。此前 `wifi-task-matrix-20261013` 轮结束后的 App 完整数据 7,325,696 B 已归档并校验，只撤该轮拥有的 USB 转发，不回滚账号／请求记录；当前轮 App 保护与收尾另取§10.5。

匹配“带翅膀的蜗牛”的两目标新候选已在独立私有目录完整链接通过。Wio BIN 149,768 B、CLI 程序 149,024 B、ELF `.data+.bss` 68,384 B，SHA-256 `d5667140df7cc001da51f7e9edd728c5e8fa02021c46fa0f6058a087d9aafd49`；R4 BIN 144,560 B、CLI 程序 144,552 B、静态 RAM 13,372 B，SHA-256 `8b7e3e173d5aaa8af8bb9921c76522caadac997c55ffd23a990e04fed2dd621f`。源码、安装投影、17 个实现单元、SDK、公开验签材料及旧候选输入保持。原官方 cache 可用，未复制旧 Sketch／库对象；不称为冷 SDK 构建或缓存命中测量。V3 业务短闭环仍待完成，旧网络候选不再用于当前网络的成功证明。

### 10.5 当前网络下 Wio WiFi 有限业务与恢复

沿用普通 App `f2bc1828…` 和既有服务，不升级 SDK、部署新 App／Service、轮换 Key 或新建设备。先新鲜双读主 MCU，再用原已验证维护 BIN 双读外部存储；烧录前重新采集同网手机的两端口 TCP 可达记录，并核对候选、公开验签材料与原实物实例。内部测试工具仅复用原上传、存储、App 和 Timer 链，没有新增产品运行时。串口异常时补 `finally` 回收存活测试子进程；关闭 CDP 不等于取消已发请求，未知 Action 仍禁止重放。

| Wio 检查 | 实际结果 | 范围 |
| --- | --- | --- |
| 新鲜保护 | 524,288 B 主 MCU 两次读取一致；外部 8,192 B 双读及私有文件读回一致；App APK 不变、本轮验收前完整数据 7,325,696 B 备份校验 | 不拿旧备份或主 MCU 备份替代外部授权保护 |
| WiFi 接入 | 新候选应用读回一致；真实 App 实际通道 Cloud、Manifest accepted、fresh State | 同一原设备、原 Key；当前实例而非 MAC 匹配 |
| 控制 | power／button 原请求各一次，真实 ACK 233／203 ms；State revision 3、power=true、buttonPresses=1 与 MCU 一致 | 原客户端 Promise 经验证后结算；不宣称物理负载或物理 exactly-once |
| Timer 共存 | 250 ms 周期 9 次、1001 ms 延时在 1002 ms 执行一次；提前取消 0 次，结束槽 0 | 两项控制都在真实 `started=1 && finished=0` 串口样本中出现，未靠最终计数推断并行 |
| 实时产品 | 实际来源 Cloud、协商间隔 1000 ms；短窗口 7 个独立样本平均到达间隔 1000 ms，fresh、missed=0，曲线 109～115 点 | 原租约及 300 点边界；不作为硬实时精度、长稳或达到上限的实测 |
| 离页与返回 | 目标页面资源回收；MCU samples=161 两次相隔约 55.6 秒不变，返回后增至 213 | 用户 MQTT 保持；本轮未另测 Android 后台或强杀 |
| 软件复位 | 首次时间／来源／采样／Timer 全为 0；再次进入后 fresh State=false／0、revision 1，MCU 时间有效、source=Server、samples=1 | Timer 未自动注册、旧控制未重放；不代替断电持久任务验收 |
| 恢复 | 外部 raw 从变化后的摘要恢复到原 8,192 B，独立双读一致；完整主 MCU 524,288 B 与新鲜备份逐字节一致 | 此轮不再是外部 no-op；没有 clear／format、写 RTL 固件或恢复数据库 |

证据保留在忽略目录 `build/wifi-live-current-20261013/`，私有原始 Flash、外部存储、账号及请求日志不提交。原请求／回执及四个相关服务的完整现存日志已归档，未清除、回滚或造请求。Wio 的普通 WiFi 有限业务及恢复关闭，不据此扩大持久任务、Wio BLE 发布、历史／Bulk 或其他网络组合。

R4 同批主 MCU 262,144 B 和 Data Flash 8,192 B 已新鲜读取并第二次核验。维护候选标准上传在握手阶段报 `Device unsupported`，尚未进入写入；只读复核确认整主 Flash、启动区与 Data Flash 都与原备份一致。待用户仅拔插 COM4 后，再采集新鲜复核并有限重试。R4 业务及整体 V3／V5 尚未关闭，后续四槽 ESP32 试点仍按 V4，不因 Wio 通过而提前扩展。

本轮等待 R4 物理拔插时，App 已安全收尾：验收前／后完整数据分别为 7,325,696／7,331,328 B，均从手机读取并校验文件；APK 未更新，未回滚账号、授权或请求记录，只撤本轮拥有的 USB 转发。四个相关服务的完整现存日志再次私有归档，容器日志未清除。没有使用旧轮或同大小文件替代当前结果。后续 R4 需重新建立匹配 App 的有限观察并采集新鲜复核，不重放原控制。

下一步仅续办 R4 V3：确认 COM4 原物理身份，重新只读核对代码／启动区／Data Flash，再按原标准上传器有限重试；维护程序校验后先双读外部键，重新核对当前网络与服务，再刷现有 WiFi 候选。通过真实控制、Timer／实时及恢复后才进入 V4，不把当前编译通过或 Wio 通过替代 R4 实物结果。等待期间 R4 原程序与存储未变，Wio 已完整恢复，不留运行中的业务测试。

### 10.6 R4 主程序覆盖授权与剩余验收

2026-10-14 用户确认 R4 曾另烧程序，并明确允许“不需要备份，直接覆盖”。本轮不再要求完整备份当前主程序，也不恢复昨晚的主程序。上述§10.5末尾的原程序恢复／未变状态是上一轮结果；当前仅保留其历史证据，不作为新程序的回滚依据。

| 本轮范围 | 执行要求 |
| --- | --- |
| 主 MCU | 仅 COM4、原硬件 UID，原 SDK 标准上传器写应用区；保留通过验收的新候选，不覆写旧备份／失败记录 |
| 上传边界 | 沿用现有保护记录；用户后续要求停止底层诊断，按手动进入下载模式直接标准上传，不再要求 SWD 全读回；以实际上传结果和真实业务区分通过，不能补造读回成功 |
| 连接模块 | 维护程序先双读十个库所属 raw 键；继续保护设备身份、网络和 BLE 授权，不清 namespace、不轮换 Key、不升级桥接固件 |
| 业务 | 同一已有逻辑设备和 App 账号；实际 Cloud 控制 ACK/State、Timer／授时共存、实时与离页恢复、有限软件复位 |
| 收尾 | 保留账号和请求日志、回收本轮观察资源；不得将“主程序允许覆盖”扩大为删除授权或数据库回滚 |

当前电脑和手机仍在“带翅膀的蜗牛”，服务地址未变。Docker 重启后，仅恢复未自启的原 PlainHTTP 测试代理；Gateway、Broker、数据库和镜像不更新。App 匹配 APK 未更新，本轮新的完整数据 7,331,328 B 已独立读取并校验。

两次完整主 MCU 只读读取遇到 SWD 传输中断，均没有完整成功记录；其共同前缀一致、应用区与旧备份不同，与用户另烧程序的说明一致。本轮没有使用这些部分文件放行上传。剩余顺序为维护程序／外部保护 → 已编译 WiFi 候选 → 有限业务 → 存储保护与 App 收尾；整体 V3 通过后才进入 V4，不重复 Wio 业务、不扩槽或升级 SDK。

#### 上传入口核对

启动区 16 KiB 与 Data Flash 8 KiB 的窄守卫已完整读取、验证。维护程序标准上传仍在握手报 `No device found on COM4`；随后实际读取并验证 59,344 B 应用范围，维护候选未匹配，启动区和 Data Flash 与本轮守卫相同。这里没有完整旧主程序备份，也没有成功候选或业务结果。

用户指出 Arduino IDE 可正常上传。已对照本机工具：脚本 CLI 1.5.1、IDE 2.3.6 自带 CLI 1.2.0；两侧 Renesas Core 1.5.3、`boards.txt`／`platform.txt`、bossac 1.9.1-arduino5、serial-discovery 1.5.2 均同字节，无 SDK 本地覆盖。`--port`／`--fqbn`／`--input-file` 参数有效；IDE CLI 的无写入展开也得到原官方 bossac 命令。串口独占打开成功，没有串口被占用的证据。

用户提供的 IDE 成功命令仍是原 bossac `-d --port=COM4 -U -e -w <BIN> -R`，Core 1.5.3 不变。仅换 IDE 自带 CLI、保留便携 SDK 的第一次对照失败；随后使用 IDE 自带 CLI 和 IDE 原 Arduino15 目录，维护程序上传和 59,344 B 应用读回成功。不能把这个结果归因为 CLI 版本或目录单一因素。

官方配置同时接受 USB PID1002／006D；本机精确 UID／COM4 保持 PID1002，不以 PID 判断 boot 就绪。WiFi 自动 touch 后出现版本为空／`Device unsupported`；用户双击 RESET 后确认 L 灯持续呼吸，但一次无写 Flash 的 bossac 信息查询仍失败。将主机串口设为 DTR／RTS 开启、115200→230400 并等待后，同一个原 bossac 信息查询返回真实 `Arduino Bootloader (SAM-BA extended) 2.0 [Arduino:IKXYZ]`。这一对照同时改变控制线和速率，不单独认定 DTR 或缓存为根因，也不改桥接固件。[官方 USB 桥接](https://github.com/arduino/uno-r4-wifi-usb-bridge/blob/main/UNOR4USBBridge/UNOR4USBBridge.ino)、[BOSSA Windows 串口实现](https://github.com/arduino/BOSSA/blob/37600d1/src/WinSerialPort.cpp)。当前 BOSSA 的实际高档为 230400，其打印的“921600”不作为真实速率测量。

握手后的 WiFi 候选通过原网络／来源校验并产生一次真实上传意图；CLI 启动后 60 秒超时，私有上传日志为空，不能据此推定成功或未写入。防重放检查拒绝重复该意图。随后 144,560 B 应用范围完整读取并验证，前 59,344 B 仍与维护程序相同，WiFi 候选未匹配、启动区相同；同一轮外部十键再次双读，与原 978 B 快照相同。没有发送 power／button／Timer 业务请求。

#### Data Flash 读取保护修正

旧窄守卫的 8,192 B 为全零，新读回为全 `0xff`。Renesas 官方规定访问 RA4M1 Data Flash 前须设置 `DFLCTL.DFLEN=1`，并等待最长 10 µs；寄存器 `0x407EC090` 复位默认关闭。旧检查未记录该状态，因此旧摘要不能作为物理 Data Flash 已保留或被清除的证明。[Renesas 技术更新](https://www.renesas.com/en/document/tcu/errata-users-manual-hardware-accessing-data-flash-without-using-device-driver)、[FSP 4.0.0 实现](https://github.com/renesas/fsp/blob/v4.0.0/ra/fsp/src/r_flash_lp/r_flash_lp.c)。本文件旧阶段的 Data Flash 同摘要仅保留为当时读视图结果，不据此扩大物理存储保护声明；旧证据不改写。

本轮已建立独立当前基线：实际读取 DFLCTL=0，只使能读访问后观察到 1，再完整双读／校验 8,192 B；启动区与原有效守卫相同。首次 1 MHz SWD 读取中断、未生成成功基线；临时降到 100 kHz 后完整核验通过。这里只改易失读访问寄存器，不擦写 Data Flash、启动区或连接模块，也不把旧全零文件写回硬件。当前有效基线不补证之前的历史物理存储状态。

#### 当前业务验收入口

用户明确要求停止上述底层对照，暂按这块板需要手动复位处理。到此冻结现有结果，不再查询 bootloader、进行 SWD／Data Flash 读回、复测上传器或升级 SDK。仅确认用户已手动进入下载模式，使用与 IDE 同一原 bossac 和已编译的 WiFi BIN 正常上传；实际成功返回后，由串口运行标记、同一设备 App 的真实 Manifest／fresh State、ACK 和实时数据确认功能，不把它称为 Flash 逐字节验证。

顺序为 WiFi 登录／MQTT → 原 power／button 各一次 ACK 与 State → Timer／通信共存 → 实时来源和间隔 → 离页停止／返回恢复 → 业务复位恢复及收尾。未知 Action 不重发，账号和请求日志继续保留；不清授权，不恢复用户已经允许覆盖的旧主程序。V3 业务关闭后才推进 V4 既有 ESP32 四槽试点；Wio 已通过项不重跑。

### 10.7 新 R4／mostfun2026 普通 WiFi 收口（2026-10-14）

用户更换了 R4，并确认当前网络为 mostfun2026。重新枚举到 COM9、USB 序列 `F412FA6F2F88`，不同于§10.6的旧 COM4；旧板的 Key、物理实例、备份和读回证明不用于新板。本轮按用户要求停止底层对照，只正常上传并用实际运行和 App 业务验证，不做 SWD／Data Flash／启动器信息查询或升级 SDK。

当前电脑／手机同网，本机测试地址对齐到 `192.168.1.156`：仅 Plain MQTT publicHost、18884 与 PlainHTTP 17002 的绑定调整；原镜像、公开验签材料、持久挂载、其他服务、账号和数据库不变。真实 App 正常创建“R4 WiFi 验收 1014”并 reveal Key 各一次，不复用旧 BLE 设备身份、不轮换旧 Key、不伪造会话或直接写数据库。本轮未独立核验设备的 16 B 物理实例，明确保留未知，不能从 USB／MAC 推导或补造验证；此项不阻断上述真实 Cloud 功能闭环。

| 检查 | 实际结果 | 范围 |
| --- | --- | --- |
| 匹配链接与上传 | Renesas UNO 1.5.3；BIN 144,536 B，CLI 程序 144,528 B、静态 RAM 13,372 B；原官方 bossac 正常上传返回 0 | 当前源码、419 个安装成员、17 个实现单元及既有闭包核验；仅投影三个 profile 头，不修改生产配置。没有 Flash 读回，不称逐字节校验或峰值 RAM |
| WiFi 接入 | 新 DeviceKey 对应设备真实 Cloud 就绪；Manifest accepted、fresh State；首次进入页就绪 468 ms | 已在线后打开页的单次结果，不是设备开机到 MQTT 的耗时或 BLE 优化结论 |
| 控制 | power／button 各一次；原生产 ACK 校验 Promise 在 251／301 ms 结算；revision 3、power=true、buttonPresses=1 与 MCU 一致 | 未本地赋值造 State，未知 Action 不重发，不宣称物理 exactly-once |
| Timer 共存 | 周期 250 ms、执行 9 次；1001 ms 延时实际 1021 ms、执行一次；取消回调 0 次、最终槽 0 | 14 个真实串口样本，两项控制变化均出现在 `started=1 && finished=0`；不以最终 ACK 推断并行，不承诺硬实时精度 |
| 实时产品 | 实际来源 Cloud，协商 1000 ms；短窗 7 个独立样本平均到达间隔 1013 ms，fresh、missed=0，曲线 55→61 点 | 原 onSample／lease 和 300 点边界不变；未测上限或长稳 |
| 离页／返回 | 目标页面资源回收，用户 MQTT 会话保持；MCU samples=147 在相隔 20.08 s 的两次观测中不变；返回后 5.07 s 内 149→154 | 返回页就绪 145 ms；本轮不另测 Android 强杀、多消费者或 BLE 循环 |
| 手动 RESET 恢复 | loops 11730→642；Timer／控制计数归零，未自动注册或重发；重新进入取得 fresh revision 1、false／0，时间有效 source=Server，采样 2→7 | 用户仅按一次 RESET。首个重启后观测已重新授时，未捕获未同步窗口，不称断电持久任务通过 |
| 收尾 | 最终离页释放；App APK 未更新，前／后完整数据 7,331,328／7,354,368 B 归档并核验；四个相关服务完整现存日志归档；仅撤本轮拥有的 USB 转发 | 不清账号／授权／请求，不回滚数据库；新 COM9 保留验收固件与测试设备，不声称恢复未知原固件或完整桥接授权 |

证据留在忽略目录 `build/r4-wifi-business-20261014/`、`build/r4-wifi-resume-20261014/` 的受限私有子目录，以及服务网络／日志归档；Key、WiFi 密钥、App 数据和请求正文不提交。BIN SHA-256 为 `15c1dde88ed291452f36a3edef1471c64207c9eb4235552a61432b1adf693c82`。未改设备／App／Service 生产源码，保留无关工作树和 Git index；本批不暂存、提交或推送。

至此 V3 两平台有限普通 WiFi 范围关闭。新板只完成本节 WiFi 业务，不把旧板 BLE 结果冒称新板 BLE 实测。下一步仅 V4：按 COM11 的真实 factory 布局核对新鲜恢复材料，编译既有四槽任务试点，精准准入后完成定义／查询、一次有限 Countdown、运行中重启中断、删除及原控制恢复，最后恢复 COM11 原固件／授权并收尾。

V4 准备时不能原样重跑历史工具：旧 `build-device.cjs` 固定 COM3 的 `default_8MB/app0`、旧测试地址／设备证明和旧 include 审计；旧任务 UI 工具禁止 Start，不能充当 Countdown 完整验收。实际复测已按 COM11 的 `factory=0x10000/5 MiB`、现网／真实设备／profile 做最小参数对齐，结果取§10.8；不重写 TaskEngine、不扩槽、不刷分区或升级 SDK。

### 10.8 ESP32 四槽持久任务及恢复验收

2026-10-14，沿用普通 App `f2bc1828…`、Service 051 和当前镜像；真实 App 正常创建并 reveal 一个专用测试设备各一次。COM11 身份、factory 布局和完整 8 MiB 恢复文件重新经官方 ROM 校验一致；本轮未再次读取整片作新鲜备份。候选只写 factory 应用区，启动器／分区表／SPIFFS 不写。临时准入只增加这个逻辑设备，Gateway 与其真实路由 Broker1 使用原镜像，其他服务不升级。

| 检查 | 真实结果 | 范围 |
| --- | --- | --- |
| 完整链接／刷入 | 原 SDK 3.3.11、当前 410 源码输入；BIN 993,456 B，静态 RAM 54,168 B；实物 factory 起点 65,536、容量 5 MiB，应用及原 NVS／分区校验通过 | BIN SHA-256 `f9fffc8cc9b7aed9bc343d594a26cf6cc79cc7a2a0872dae569c18a26d3f74c6`；不刷编译生成的分区表，不称峰值 RAM |
| 初始化／目录 | 真实 owner 初始化、容量 4；power 布尔和 duty 0～100／step 5 目标可查询 | 匹配内部 profile 与精确 allowlist，不开放普通默认任务 |
| 保存和首次执行 | 启用的 6 s 定义保存／读回后仍未 Start；唯一显式 Start 获 Armed，MCU power=true、写回调 1 次 | 创建／编辑不自动启动；有限布尔 Countdown，不称全部日期／星期或槽满门禁 |
| 完成持久化 | 新 fresh Read 真实读取 Completed=3、definitionRevision=1、startSequence=1、tableRevision=2 | 查询前发生观察器引起的疑似重启，恢复读证明完成状态仍保留；不重放旧 Start／失败查询 |
| 编辑／运行中重启 | 同一任务改为 15 s，definitionRevision=2；第二次唯一 Start 后受控复位，fresh Read 为 Interrupted=6／tableRevision=4 | Replace 重置 startSequence，第二定义 Start 为 1，不误称全局序列 2 |
| 到期后行为 | 超过原 15 s 到期点再次 fresh Read 仍为 Interrupted；MCU power=false、写回调 0 次 | 重启不续跑、不隐式 Start；正常软件复位，不冒充切断写入期间的掉电原子性门禁 |
| 删除／原控制 | remove 后目录 count=0、revision=5；原 App power false→true 的生产 ACK Promise 在 207 ms 成功，fresh State revision=2／MCU 写回调 1 次 | 仅一次原布尔控制，不新增数值／Action 验收声明 |
| 收尾／恢复 | 离页后本地 group／BLE 资源及缓存任务页为 0；恢复原 NVS 20 KiB 与 factory 5 MiB 后，整个 8 MiB ROM 校验与原文件一致，正常复位 | 原固件／授权已恢复；字节一致不代替原固件旧网络配置的上线证明 |

首次执行后的一个 Read 返回 `TASK_OUTCOME_UNKNOWN`，没有当作 Completed 或自动重发。串口观察器当时默认启用 DTR／RTS，并在两个关闭点后均约 3 s 出现新 Cloud session、回调计数归零，高度符合测试观察造成重启；没有以此修改 SDK、任务存储或服务。将忽略目录观察器改为打开前关闭控制线后，新正常 readonly 查询取得真实 Completed；原失败 HTTP、prepare、时间线与日志保留，不补造旧成功记录。

当前目标 ledger 为 25 个 requests／25 个 attempts，全部 verified：初始化 1、写操作 5、读取 19。原初始化／Create／Start1 的精确回执与故障时点逐字节一致。过期 Read（含上述 Unknown）已由现有保留策略自然清理，旧失败证据留在私有归档；不宣称它们仍存在于当前 ledger，不回滚数据库或补写请求。

Gateway／Broker1 的两项 allowlist 均恢复原 1 项，目标容器 healthy，镜像／公开信任／网络绑定／持久挂载及其他容器合同保持；所有已捕获写回执保留。App 前／后完整数据 7,354,368／7,406,592 B 均读回校验，APK 未更新，不回滚数据覆盖账号／请求；仅撤本轮拥有的 USB 转发。目标临时测试设备及审计保留，不删除历史设备。完整服务现存日志及失败证据在忽略的受限 `build/esp32-task-v4-20261014/`；私有 Flash、Key、网络密钥和请求正文不提交。

本轮只更新内部测试工具和本文／进度入口，三端生产源码、SDK、Git HEAD／index 及无关工作树不变；未暂存、提交或推送。V4／V5 按本节有限范围关闭，下一步为平台适配，不重新派发已通过普通通信或长 BLE 循环。

## 11 R4 与 Wio 新能力适配

先完成普通 WiFi 持久任务，实时与 H1 只补实际差异；保持 `enableTasks()`、App 任务入口、四槽合同与现有执行语义。普通默认容量仍为 0，未启用任务的产品不能承担任务 workspace／scratch 成本。不复制 `WiFiProduct`、`TaskEngine` 或鉴权链，不新增平台专用用户 API。

| 顺序 | 最小内容 | 关闭条件 | 状态 |
| --- | --- | --- | --- |
| X0 平台契约 | 核对共享存储 owner、644 B 原子快照、停止后退休清除、verifier／profile 和预算 | 下述边界已冻结；参数在平台测量后准入，不直接沿用 ESP32 提交时限 | 共享契约／ABI、独立 store 耗时及有限分配边界已测；完整产品资源待补 |
| X1 R4 WiFi 持久任务 | 共用连接模块后端，独立双键任务 store，复用任务模块／Cloud carrier；匹配便携 verifier 和服务 profile | host 与真实 SDK 完整链接、默认无任务大缓冲；644 B 读写／重启／故障恢复、凭据保护、App 创建／Start／Interrupted／删除和原控制短验收 | §11.5／§11.7有限闭环保持；Facade一致性已修正，§11.11真实未知写后的整板复电、十二键／固件恢复及新owner产品差异仍待验，不开放默认产品 |
| X2 Wio WiFi 持久任务 | 共用原生 QSPI 端口，核准独立预留区的原子双扇区 store；复用 X1 的共享模块和合同 | 区域归属／备份明确、读回／断点／重启恢复、身份银行不变、匹配 App 任务及原控制短验收 | §11.12共享owner、任务视图、薄WiFi组合、模型及最终普通／四槽隔离完整链接通过；区域归属与实物仍待验，默认不开放 |
| X3 平台 H1／实时差异 | R4／Wio 的 Cloud State 变化、去重、重连基线、范围分页、名称及现有读取 ACL；回归任务接线影响的实时租约 | 原 Cloud ACK／State 与历史记录相符，离页停止／返回恢复未退步 | 可与相应平台任务准备并行，不重跑无关已通过项 |

### 11.1 一个存储后端 owner 与独立任务视图

R4 保留 `bl_v2` 和既有身份／网络／授权键，任务使用独立键 `ts0`／`ts1`；已检查当前源码没有命名冲突，实物仍需检查是否有外来数据。后端唯一拥有 Preferences begin／end，身份 store 与任务 `IAtomicBlobStore` 只持有有界视图；不创建会互相关闭 namespace 的第二个 Preferences。新视图复用既有 journal 的长度、magic／generation／CRC、tombstone、恢复和精确读回，不另写一套算法；凭据银行不扩大为 644 B，也不增加默认全局大缓冲。该单 owner 约束不承诺兼容第三方代码同时保有多个 Preferences namespace。

任务视图的存活范围必须覆盖 transport 已停止、`clearRetired()`尚未完成的阶段；一般 stop／重连不清任务。现有普通 WiFi／BLE stop 本身并不直接关闭 bank，这是新增共享消费者的生命周期约束，不是已证实的普通停机缺陷。用已有任务生命周期测试验证 begin／stop／retire／clear，并覆盖身份与任务交替访问、第二消费者初始化失败／析构；存储 fault 不能阻断普通控制，也不能伪装清除成功。R4 实测 bridge RPC 单次 payload、644 B 往返／提交时间及耗用后再设内部写策略，不从 API 签名或剩余静态 SRAM 推导通过。

Wio 从既有存储实现提取小型共享 QSPI owner／读写原语，不复制身份银行实现；身份占用的尾 8 KiB 及旧格式保持。任务候选可评估相邻的两扇区，但必须先核准文件系统与固定预留布局，取得该精确范围的备份；不自动占用空白区域、不格式化用户文件系统。任务与身份不同 store 共享端口，关闭一个视图不得使另一个失效。以原子区实际擦写／延迟和寿命预算决定准入，不机械套用 ESP32 的 100 ms。

### 11.2 薄组合与匹配授权

按实际消费者提取 ESP32 模块中不依赖平台的任务 workspace／runtime 组装，注入平台 store、时钟、目标和 grant verifier；平台只提供这些端口及按需 WiFi extension。继续复用 `TaskProductLifecycle`、`TaskCloudTransport` 与原 App／服务管理链，禁止新增第二个 MQTT 客户端或平台专用任务协议。

Ed25519／P256 都属于既有任务 grant 合同。选择由维护者可信平台 profile 与服务 signer 决定，客户端不得指定 keyId／算法来降低验证；若需要便携算法，只补现有签发链的可信 profile 选择及对应回归，不扩 wire 或改普通设备安全策略。先完成 host／32 位 RAM、两个 store 相互作用和 SDK 完整链接，再进行精确试点；只在实测通过后更新平台声明。

纯 BLE 持久任务管理、R4 WiFi+BLE、Wio BLE 正式发布、H2 周期历史、离线存储／Bulk、OTA、复杂自动化和新网关协议继续独立后置。平台差异回归不以新阶段名重开已经关闭的业务项。

### 11.3 R4 内部任务源码及有限软件验证

本节保留 workspace 迁移前的首轮软件基线；其中 15,820 B 静态 RAM、8 B task view 和 1328 B 局部写 scratch 不再代表当前任务候选。当前数值及实际存储结果取§11.5，不能用旧 BIN 放行新源码。

任务公共组装提取为 `TaskModule<Capacity>` 和 `WifiTaskExtension<Stack, Port>`：表／workspace、authority／grant session、Cloud carrier 和生命周期各只有一套。ESP32 保留原 NVS namespace、P256 verifier、begin失败不重试及原类型入口；R4 薄 port 借用平台唯一 bank，使用签名专用 Ed25519 provider。原 Direct 的完整 crypto provider 委托同一签名校验，未复制 canonical 检查，也不为 WiFi 任务引入 Noise／X25519／AES-GCM 对象。

R4 在默认容量 0 之外仍需维护者内部 `BLINKER_INTERNAL_UNO_R4_TASKS` 门禁。本次编译参数为四槽、60 s／10写、Start间隔30 s、提交上限1000 ms，**只是编译候选参数，不是实测准入策略**；公开 profile 工具不因此新增 R4 发布 profile。Sketch 的 `enableTasks()`／`begin()`／`run()`签名不变。

存储修正同时补齐一个原 API 的失败边界：SDK `Preferences::getBytesLength()`／`getType()`把 RPC 失败折为 0／PT_INVALID。bank 仅复用原 SDK 的 PREFTYPE／PREFLEN 请求，显式检查 RPC 成功及严格数字回复；读取失败不能当作空表或将不可读新槽退回旧版本，CRC损坏则保留原恢复语义。未知写入由原 `TaskSnapshotStore` fault latch 停止，不创建另一执行器或自动重试。旧十键、BRJ2格式及 legacy ID兼容保持。

Service只补必要的可信签发选择：`DEVICE_V2_TASK_ED25519_DEVICE_IDS` 为 `DEVICE_V2_TASK_MANAGEMENT_DEVICES` 的无重复子集；名单非空而缺少 Ed key 则启动拒绝。repository在原 owner／Key／root／nonce检查后按持久 scope 的逻辑设备 ID 选择固定 signer；已签发 grant返回原精确字节，不重签。客户端不能指定算法／keyId，未列设备、Ed-only环境和 Controller默认行为不变；Broker沿用原准入及exact relay，无迁移或新线协议。

| 验证 | 实际结果 | 边界 |
| --- | --- | --- |
| Host／ASan | 75/75通过，含共享模块、R4完整644 B／双键、身份交替、CRC／tombstone、元信息／读回失败、旧存储及原任务生命周期回归 | 当前未启用 fuzz smoke；假介质不代替实际NVS掉电恢复 |
| 发布工具／架构 | 45项回归、架构门禁通过；实现单元仍17个，运行时文件415个 | 审阅后仅更新Facade／R4产品两项冻结摘要，登录wire及其他冻结项不变；未生成或交付新ZIP |
| ESP32 SDK API | 原3.3.11编译参数、当前src优先，15个语法／诊断案例通过 | 包括无policy、BLE／WiFi+BLE、未准入R4／Wio拒绝；不是新的ESP32完整链接或实物复测 |
| R4任务完整链接 | 原Renesas UNO1.5.3，BIN177,384 B／程序177,376 B，静态RAM15,820 B；真实任务store／carrier／Ed25519符号保留 | 只编译，不上传；物理应用限245,760 B，静态余量不能代替运行峰值 |
| R4普通WiFi完整链接 | 同一当前源码，标准WiFiQuickStart BIN140,656 B／程序140,648 B、静态RAM13,060 B；未链接TaskEngine／TaskCloudTransport／Ed25519 | 两Sketch不同，不将差值全算作任务成本；默认产品仍无任务对象 |
| 真实32位ABI | Facade12 B；原／当前bank84／84 B；task blob view8 B；共享module四槽1560 B、R4module1568 B、task extension2456 B | 真实SDK编译器对象大小，不是运行高水位；task读scratch664 B，写／clear局部scratch1328 B仍须纳入栈预算 |
| Service定点回归 | 26项通过、2项PG集成明确skip；新增9项中真实Ed／P256签验、原Controller、绑定／拒绝和exact grant复用通过；Gateway类型检查通过 | 使用生成测试key／假DB；未连接业务数据库、构建镜像或部署 |

候选BIN摘要为 `4c6885a3e6158fcd16d71c7a3decdfd0020415b9d4c857f8bb1143b69308272e`；普通WiFi为 `7066e89d1113d3593c75777e94e6a587271d2132284ddb0d22281a5fafb49b6a`。当前源码／SDK来源、依赖、符号及输入一致性记录在忽略的 `build/r4-task-x1-20261014/`；旧安装Blinker没有混入。SDK缓存可用，不称全SDK冷构建。

中间工具失败保留：ASan运行初次缺DLL导致进程尚未执行，配置仅子进程PATH后真实75项通过；CMake include路径及host stub不安全字符串调用已修正，未放宽 `/W4 /WX`。任务链接后的首个来源检查拒绝重复的同一SDK路径，后续独立审计按唯一真实路径及Windows大小写核验当前源码／新头／符号；原checker留存，没有重编或伪造原成功记录。这些都不作为产品通过次数。

本批未部署App／Service、改本机env／数据库、刷板、清授权、提交或推送；App及无关本地修改保留。X1软件部分通过，整体未关闭。

### 11.4 下一批R4实物准入顺序

步骤1／2已按§11.5有限执行，现行待办取§11.6，不重复首写或已通过普通通信。主程序的保护方式以当前板的明确授权为准；正常上传不构成主 Flash／Data Flash逐字节读回证明。

1. 枚举当前R4并核对身份／网络，明确可覆盖范围与恢复目标；未获覆盖授权时须先具备可靠的主程序恢复材料。本轮用户允许 COM9 直接覆盖，仍要求外部存储保护涵盖旧十键及 `ts0`／`ts1` 的实际raw／absent状态。旧十键快照不能称已覆盖新任务键，不借用其他板的恢复材料或清整个namespace。
2. 用小型存储探针测实际644 B payload／664 B journal往返、提交／读回耗时及确认提交后的重启恢复；同步测有限堆／栈观测。1328 B scratch 已移入可选 task view，完整 caller／SDK／IRQ与共享carrier的1 s待处理窗口仍需在真实产品测量，不以静态剩余RAM或1000 ms编译值直接放行。
3. 冻结匹配的R4内部policy与公开验签材料，备份原App／服务配置；只准入该逻辑设备及实际Broker，加入Ed签发子集。复用现有App，不新增管理页面，不全局切换Controller／ESP32签发算法。
4. 真实App验证容量／来源／目标、保存不自动Start、唯一显式Countdown、Completed持久化、运行中重启Interrupted／到期不续跑、删除和普通ACK／State。再对受影响的实时租约做必要差异回归，结束按授权恢复原固件、全部库所属键和临时准入，保留完整请求记录。

原顺序为X1实物准入后推进X2；用户暂不能拔插时，允许按§11.12并行完成X2的软件接入与编译，不据此关闭X1。X3的Cloud历史短验可与匹配平台准备并行。Wio软件候选不等于实物支持声明，也不自动打开纯BLE任务管理。

### 11.5 R4 任务 workspace 与独立存储短验

本轮仅 COM9、USB UID `F412FA6F2F88`，不操作 COM6 Wio／COM11 ESP32。用户明确允许覆盖主程序，并允许必要时不恢复 WiFi；本轮仍在结束时恢复此前已验收的 WiFi BIN，未新鲜备份主程序、进行 SWD／上传器对照、读取 Data Flash 或更新连接模块固件。

Core1.5.3 名义主栈预留为1024 B，实际 ELF 的 `__StackTop=0x20007f00`、`__StackLimit=__HeapLimit=0x20007b00`；`_sbrk`按固定HeapLimit扩堆、main关闭SPMON，不能把局部数组侵入未用堆空间当作SDK已有保护。四槽 task view 改为自有两块664 B workspace，只在启用任务时分配，不增加默认bank、使用heap或共享全局scratch。真实32位ABI：bank84 B、Facade12 B不变；view1336 B、R4module2896 B、extension3784 B。view自身编译栈帧为load48／replace64／clear48 B，不是完整调用链峰值。

维护 gate 增加可选十二键 `R4K2` 格式，旧十键 `R4K1` 默认路径保持；只保护库所属键，不清namespace。十二键维护大缓冲放在独立维护程序workspace中，不进入产品源码。初始1004 B raw／absent快照独立双读相同、受限私有文件读回一致，两任务键均absent。存储探针仅加载既有身份，无网络、执行器、自动写入或自动Start。

| 验证 | 实际结果 | 范围 |
| --- | --- | --- |
| 当前任务完整链接 | Core1.5.3；BIN177,328 B／程序177,320 B、静态RAM17,148 B；保留真实store／TaskEngine／carrier／Ed25519符号 | BIN `3ed017c6099e33c4a5455ba08dceae26b71eafb58b0c6b2e31012a297307b87d`；仅编译，占位凭据不上传；policy仍未准入 |
| 实物存储候选 | 最终probe BIN73,552 B、静态RAM11,628 B；标准上传成功，实际PING／OPEN及GNU malloc/realloc/free wrapper链接确认 | BIN `541e34b4b7163eb13d2d7bd077a50baea15bd75f6e3919c981a9cee42bd97bcd`；没有Flash读回，不是完整WiFi任务图 |
| 首写／完整快照 | 一次replace644 B，实际journal664 B；提交157,028 µs、load86,794 µs；四条disabled／未Start记录，精确fixture与摘要匹配 | 首笔实际落ts1；原探针错误的ts0假设导致后置检查失败，日志与锁保留，不将整个旧命令改写为成功，也不重放该笔 |
| 真实断电恢复 | 用户仅拔插COM9后，OPEN／READ取得相同644 B phase1摘要；load86,749 µs | 已确认提交后的全板断电复插，不是写入途中断电原子性或故障注入证明 |
| 第二笔／软件重启 | 修正后的probe先读回phase1才继续；replace234,544 µs，load163,314 µs，双664 B槽；软件复位后load163,478 µs、phase2摘要相同 | 两笔总写次数2，不重写phase1；没有执行真实用户任务 |
| 清除墓碑 | 第三笔clear195,014 µs；一20 B墓碑＋一旧664 B槽，当前及软件复位后均NotFound，后者103,839 µs | 墓碑不等于raw键删除；旧表不会因重启重新成为active；总共三笔journal写 |
| 有限资源观测 | 单／双槽读取与写入6～13次真实SDK malloc调用；观测点最低 `MSP−brk=18,012 B`，invalidBreakSamples=0；操作后mallinfo used44 B、free1960 B、arena2004 B | wrapper仅记录分配边界，不涂栈、不扫描内存；mallinfo free不是全SRAM余量，probe结果不替代完整产品／IRQ峰值 |
| 身份／授权恢复 | 操作前后五个logical blob匹配；结束经十二键维护恢复到原1004 B快照，独立双读逐字节一致，包括原task raw absent状态 | 不清授权／namespace，不把五个logical hash当十二键raw保护；不恢复数据库 |
| 软件回归 | 更新目标后当前75/75 host／ASan、实际store／codec两方向槽序回归及架构检查通过；工具侧5项CRC／截断／身份守卫通过 | 未开启fuzz smoke；默认bank／Facade ABI不变，未重复BLE长循环或全平台发布编译 |
| 收尾 | 标准上传恢复known WiFi BIN `15c1dde8…`；只读j实际begun／timeEnabled／timeValid=1、source=Server、activeTimers=0；App APK不变、fresh数据tar7,406,592 B读回校验，完整IoT服务日志保留 | 恢复的是已有验收材料，不称当前主程序新鲜备份或Flash逐字节恢复；未重新发App控制／Start，不开放R4任务 |

探针的槽位断言已改为共享 `TaskSlotShape`，只检查阶段对应的存在／长度集合，保留完整fixture的真实codec校验；实际store的正反槽序、重开继续、三次put与clear恢复均补host回归。生产journal选择算法未改。SDK的128 B限制是AT头，不是payload；664 B跨模块实际往返已证实。

独立probe设置 `modem.timeout(1000)`，当前普通产品仍沿用SDK默认10秒；Core只提供全局timeout setter。因此这批不全局修改WiFi／MQTT RPC策略，也不将234 ms的健康提交或编译policy1000 ms当故障上界。下一批须核准完整产品的RPC baseline、实际grant／提交与carrier待处理窗口；超时／未知状态沿原fault latch及ledger处理，不能自动重放。

原工具失败保留：未参与维护编译的task头在并行修正中改变，原全src守卫拒绝；独立审计核实维护未消费该头后放行原已完成artifact，未重跑编译。probe深层header未被Arduino自动选库，之后采用既有Advanced入口；中间普通入口候选在架构门禁拒绝后停止且未上传。App包信息原全字节守卫失败，独立核实仅6个临时dump对象标识、安装／权限不变后补完尚未执行的日志归档，未重复账号capture或覆盖原失败。

固件候选／十二键／账号／日志与测量留在忽略的 `build/r4-task-x1-resource-20261014/`，敏感内容仅入受限私有子目录；没有当前主Flash原始备份。没有App／Service部署、环境／DB变更、SDK升级、Git提交或推送；仅完成存储子门禁和workspace修正，X1整体未关闭。

### 11.6 下一批完整 R4 产品与 App 准入

1. 不重跑本轮三笔journal或普通通信。枚举现板、核对现网与十二键保护；以最新源码生成真实R4内部候选，不使用compile-only占位BIN。预算检查覆盖完整WiFi＋MQTT／Ed25519 grant／TaskEngine调用链、UART/IRQ与SDK transient分配，并明确超时与1 s carrier窗口的正常／故障语义。
2. 保留当前App与账号，通过原App/API使用同一真实测试设备、取得Key／scope／路由；匹配公开Ed材料，冻结实测internal policy。仅构建Gateway新增签发选择所需的最小运行闭包，精确加入该设备的Ed子集与实际Broker准入；旧Controller／ESP32默认算法及其他服务不变，原配置／日志先备份。
3. 真实App检查容量4、来源／名称、保存不自动Start、一次有限Countdown、Completed、运行中重启Interrupted／到期不续跑、删除及原ACK／State；再只回归受影响的实时租约。原请求／未知状态保留、不重放Start，结束撤临时准入并按确认范围恢复，才关闭X1。

Wio任务store／预留区继续X2，平台H1继续X3；本轮不因存储探针通过自动开放这些功能。

### 11.7 完整 R4 四槽候选与真实 App 有限闭环

2026-10-14，继续同一 COM9／USB UID `F412FA6F2F88`，沿用 mostfun2026、普通 App `f2bc1828…`、Service 051 和同一个正常注册的 R4 测试设备。不操作 COM6 Wio／COM11 ESP32，不清账号或授权，不扩槽、升级 SDK、重造执行器或重跑 BLE 长循环。

新增内部 `UnoR4WiFiTaskGate` 只使用原 `enableTime()`／`enableTasks(power,duty)`／`begin()`、真实属性回调和 `onSample()`；不在启动时初始化或 Start。验签 wrapper 原样调用 Crypto 一次，不改变结果；分配观测及串口格式化均为测试能力，不进入普通产品 API。

| 检查 | 真实结果 | 声明边界 |
| --- | --- | --- |
| 完整链接／上传 | 原 Renesas UNO 1.5.3；最终 BIN182,132 B、程序182,124 B、静态 RAM17,292 B；17实现单元，96个C++编译单元统一 `BLINKER_PRODUCT_WIFI=1`；标准上传返回0 | 最终 BIN SHA-256 `31cdc7f3195df74d44ff06124b906ce9f0a8ac6dc9a67f58af2df2c7e130c8d1`；35个Core C／ASM不消费Facade。未做主Flash读回，不称峰值RAM或普通发布方式已修复 |
| 匹配服务／公开信任 | Gateway仅新增签发选择所需四个dist文件，其他172个dist文件与原镜像一致；真实Ed keyId1与固件公开材料一致；只将该设备加入Gateway管理／Ed子集及实际Broker1准入 | Controller／ESP32默认算法不变，无迁移、DB回滚或新协议；候选及原镜像各自保留。相同scope的旧grant精确复用，不由App选择算法 |
| 初始化／目录 | 真实App显式初始化一次；容量4、云端查询／设备本地执行来源、当前设备名、power(bool)／duty(uint)目标均可读 | 固件opt-in不等于owner授权；初次进入仅提示初始化，不自动创建scope／Start |
| 保存／完成 | 启用的10秒定义保存后progress0、startSequence0；唯一显式Start取得Running1，MCU power=true、回调1次；fresh Read为Completed3、revision2 | enabled=true与已经Start是两件事；不写成停用定义，不将管理ACK冒充物理输出恰好一次 |
| 编辑／重启中断 | 同任务改为30秒、definitionRevision2，保存后仍Idle；第二次唯一Start后受控软件复位，fresh Read为Interrupted6、revision4；越过原期限后再读仍Interrupted，MCU power=false、写回调0 | Replace重置Start序列，第二定义Start仍为1；不续跑、不自动重发，软件复位不是写入中断电门禁 |
| 删除／原控制 | remove后revision5、count0；原App真实toggle／range各一次：power=true、duty=5，原生产ACK Promise在474／166 ms结算，fresh State与MCU各1次写回调一致 | 不发Action，不赋值伪造State，不扩展数值类型或任务容量 |
| 实时差异 | 实际Cloud来源、协商1000 ms；两个短窗各7个独立样本，平均到达间隔1004／1007 ms、fresh／missed0、曲线推进；离页后MCU samples46在两个独立观测中不变，返回后恢复到65 | 原租约与onSample链；用户MQTT会话保留，仅有限离页／返回差异，不重开所有通道／长稳矩阵 |
| 请求／恢复 | 6次真实HTTP mutation：initialize1、create1、replace1、Start2、remove1；目标ledger20个requests／20个attempts全部verified（初始化1、写5、读14）；原已捕获写请求及精确回执保留 | 未确认mutation不重放；不写数据库或伪造请求。scope为16 B bytea，须按字节值比较，不能用JS对象引用相等过滤 |
| 存储／收尾 | 十二键新鲜1004 B raw／absent基线独立双读；结束恢复全部十二键并再次独立双读match；标准上传恢复known普通WiFi，实际begun／Server授时有效、Timer未自动注册。两项服务准入及原Gateway镜像恢复，相关服务healthy | 用户允许覆盖主程序，未新鲜备份主Flash；恢复known BIN不称逐字节主Flash恢复。原task raw absent状态恢复，不清namespace或升级桥接固件 |
| App／软件回归 | 原APK前后hash一致；完整账号／请求数据前后归档7,406,592／7,366,656 B并读回核验；撤本轮USB转发。最终内部UI工具20项回归、设备架构门禁通过；合并后Gateway四入口语法及任务测试26pass／2PG skip | 数据大小不是逐请求保留证明，须结合实际HTTP／ledger；未部署新App，不以假DB测试代替PG集成或合并后完整服务部署 |

#### 资源与尚未关闭的阻塞边界

已捕获样本的签名调用数与通过数一致：复位前样本所见最长287,501 µs，验签入口最小 `MSP−brk=9,044 B`；分配观测点最低8,036 B、invalid=0。真实编译可见Ed callee为1,364 B，完整可见调用链约3,060 B（计Core启动帧约3,084 B），均不是完整IRQ／全部路径高水位。仅本轮实验判断采用 `9044−1364−1024−512=6144 B>0`，后两项为未观测IRQ／SDK和实验余量；入口已扣过外层caller，不能再次减完整调用链。

健康任务往返期间曾观测最大loop约544 ms；受控业务复位后的连接恢复又实际出现10,193,310 µs的同步loop等待。SDK全局默认10秒未改，`run(1000)`不能中断同步RPC。Task carrier的1秒年龄检查和engine的1000 ms提交检查都是返回前后判据，不是SDK硬截止时间；晚提交可能已经持久化，必须保持现有Fault／Unknown，不自动重放。不能用健康234 ms存储写入、静态余量或本次签名成功次数冻结正式故障上界。

#### 编译一致性与并行合并

首个产品候选发现：Sketch通过公开入口选择WiFi，而库实现单元没有相同产品宏，`BlinkerClass`的条件成员和`SelectedProfile`绑定不同，存在正式ODR风险。首BIN未用于任务写入；本轮仅将内部候选所有C++单元显式对齐后重新链接、上传并开始业务。**普通Arduino用户不应被要求设置全库宏**，因此这不等于公开库修复；须按下一节收口，不把全局编译flag传播为新使用要求。

服务器工作树在验收期间有独立UI合并，随后由外部完成 `origin/xwd` 合并，HEAD由`268a1d12…`变为`b1b27799…`。中途两项UU及被暂时覆盖的Ed入口均未由本批处理；最终Ed配置／profile／repository接线重新可见，定点26pass／2skip通过。真机运行的是**合并前冻结的四文件镜像**，不是合并后完整源码的新镜像，不将其当合并后部署证明；保留合并和其他无关改动。

恢复时只启动现网地址上已经存在但被Docker重启停下的HTTP入口，未更改其镜像／配置／绑定；其他容器合同保持。Compose新镜像label的原审计拒绝只读核对后完成，未重放部署。内部UI工具对共享header／Ionic路由过渡／确认框背景模型的误判均保留原失败和源码摘要，最小修正后只执行原来尚未发出的操作，第一／第二Start各一次。收尾首次统计误用Buffer对象引用，所得0条结论无效；独立按16 B bytea校正到上述20条，原摘要保留，不重发查询或mutation补造结果。

完整证据位于忽略的 `build/r4-task-x1-product-20261014/`；Key、网络密钥、App账号、Flash／桥接raw、DB备份及请求正文只留受限私有目录，不提交。库／App既有源码、HEAD／index和无关本地修改保留；服务器并行合并不回撤，不宣称其HEAD／index未变。本批未暂存、提交或推送。

### 11.8 现行下一批顺序

X1的**固定候选业务与恢复子门禁通过，整体及正式产品准入未关闭**。不再重放本轮两个Countdown或存储首写，也不重新派发普通通信／BLE长循环。

1. 公开Facade多TU源码和有限软件门禁已按§11.9收口：保留唯一全局Blinker与12 B状态，三个公开入口的dependent helper选择原factory／TaskSupport；R4 Print绑定同一Core实体。不要求全库产品宏。三个模式的跨TU共享实例、错误调用回归及R4内部任务完整链接通过；普通三平台全部新原生组合的安装／真机结果不由本节替代。下一批不再重开本轮软件项。
2. R4共享owner／整笔RPC预算与故障恢复策略的软件部分按§11.10完成，原SDK真实对象及任务完整链接通过。剩余仅有限实物差异：正常任务／原控制、一次受控RPC失败或回应未知、全板复电后fresh查询及原存储保护。采用匹配构建和当前授权，不重放未知mutation或前两次Countdown，不开展上传器底层对照。1000 ms响应等待／调用准入不等于SDK UART、carrier或普通联网硬截止，正式X1仍不因软件回归自动关闭。
3. 保留合并后Ed接线，按新服务HEAD重新冻结最小运行闭包及任务定点回归；只有匹配构建／准入才补一次必要差异验收，不能从旧镜像直接签收新源码。正式配置仍不自动开放R4任务。
4. 完成上述共享边界后进入X2 Wio：核准现有QSPI预留区、原子双扇区／凭据保护和同一四槽合同，再做有限任务闭环。X3仅补任务接线影响的实时与平台H1变化／去重／分页／ACL，不重复已通过实时，也不扩展H2／离线历史／Bulk。

### 11.9 App xwd 合并与公开 Facade 编译一致性

2026-10-14，按要求将App当前 `i3w-dev-v2` 合入 `origin/xwd` 的 `9ea8e90…`，本地merge提交为`9c630fb…`，父提交为原`ffb9b5b…`与本次xwd。17项冲突按功能合成，保留同事的组件、原子面板和自定义界面；原BLE／LAN租约、账户与连接generation、实际通道、新鲜度、名称CAS、任务真实准入、自动化和H1没有整体取一方覆盖。原6项tracked本地修改及1份未跟踪文档先独立私有备份，合并后恢复到未暂存状态并核对内容；没有把它们纳入merge提交或推送。

新页面的queued控制仅在前一条真实确认成功后消费，结果未知不自动续发；离页、换设备或换连接后退休旧队列。新组件需要的短趋势限定为当前页有界内存（32字段×240点），不写localStorage、不跨授权／能力代际，不代替H1或开启离线历史。恢复既有空整数／显式0和字段别名；历史页仍使用真实服务器记录，不恢复旧静态假数据。统一pnpm锁文件，只补xwd新增拖拽和三种字体依赖，不保留冲突中的第二套npm锁。标准构建脚本另以`57da16a…`提交两个package文件的pnpm调用对齐，原七项本地改动仍未暂存／提交，未推送。

| 门禁 | 结果 | 范围 |
| --- | --- | --- |
| App语法／模板 | TypeScript无错误；自定义面板构建／pack及生产AOT构建通过，补齐新模板IonSpinner声明 | 生产初始包2.76 MB，超过2 MB警告、低于5 MB错误限；未放宽预算。没有安装新APK或部署服务 |
| App有限回归 | Angular 68文件／1015项通过，覆盖V2、任务、自动化、UI配置、BLE/LAN、历史与来源；原本地修改恢复后另5文件／170项通过；独立页面3文件69项通过 | 软件结果，不代替新自定义界面全量手机签收。queued／账户切换和短趋势上限回归已加入 |
| 标准构建入口 | 根项目与device-template脚本统一调用已声明的pnpm；embedded-load通过；带完整prebuild的标准生产构建通过 | 避免pnpm脚本再次要求独立npm可执行程序；无SDK升级／新增执行链。产物只入忽略目录，未同步Android或安装APK |
| Facade／ABI | `BlinkerClass`成员与模板定义不按产品宏变化，稳定`EntryProfile`在三个入口选择调用点；唯一全局实例、32位12 B和现有begin形态不变 | 依赖`fieldAt()`与helper.make的替换约束，错误产品／空schema／字符串抢本地重载直接拒绝；不新增全局注册或组件系统 |
| 多TU／host | WiFi／BLE／WiFiBLE三模式共享实例、report/run/end／默认tasks关闭及错误调用回归；最终Release＋ASan78/78 | WiFiBLE host使用mock原生lifecycle，仅验证真实公开入口／链接合同，不冒称ESP32原生栈实测 |
| R4 API／Print | 原Core1.5.3，18项真实对象编译通过；bare core与三个选择的class token同一摘要；Arduino.h前／后两种包含顺序均绑定`arduino::Print` | 不为API头增加Arduino SDK include；ESP32／Wio仍用原global Print实体。BLE／WiFiBLE对象案例mock原生绑定，不扩大R4 WiFiBLE支持 |
| ESP32 API | 原3.3.11的既有任务API15/15，编译命令不带全库PRODUCT宏 | 有限SDK门禁，不是新S3完整原生组合／真机复测 |
| R4完整任务链接 | 无全库PRODUCT宏、当前源投影／17实现单元、原Core1.5.3；BIN182,100 B、程序182,092 B、静态RAM17,292 B | BIN SHA-256 `99ff0bb975a1ab995523b8a553da4594d10eded37a8ebd2bade40684abe6aec4`；保留实际TaskEngine／store／carrier／Ed符号，只编译不上传，不作为RPC时限准入 |

R4的原Print前向声明在bare实现单元会创建global实体，而Arduino.h先含时解析为Core的`arduino::Print`；现已显式统一Renesas实体，不改变debug调用方式或增加对象。源码／源投影／Sketch输入前后摘要一致；仅审阅并更新Facade freeze，登录、凭据、MQTT和其余冻结项保持。

原存储host测试假设SSO回复意味着整个RPC mock无分配，MSVC Debug容器代理使原全局计数断言失败；未修改存储或放宽断言。使用与此前相同的Release＋ASan配置实际78/78通过，Debug失败保留、不算通过。Core GCC7的`-fsyntax-only`对旧头／当前Property声明均触发ICE；当前门禁用普通`-c`完成对象编译，保留对照，不升级SDK或宣称该命令完成固件链接。完整R4链接独立取上表。

App合并的回归与产物留在忽略的`build/merge-xwd-20261014/`，私有备份保持访问保护；设备端有限软件／完整链接留在相应`build/`门禁目录。没有刷板、清授权、改服务配置或重复BLE循环，设备库本批改动不提交／推送。下一批仅取§11.8第2项R4同步RPC预算；匹配新App／Service的部署另有独立门禁，不把旧真机结果套到本次新源码。

### 11.10 R4 共享 modem 与任务 RPC 预算

本批只收口R4任务存储的同步RPC边界。沿用Core1.5.3、四槽、原journal／TaskSnapshotStore／TaskEngine／管理会话和服务ledger，普通产品任务准入仍关闭。

共享owner位于R4端口，管理唯一SDK modem、Preferences namespace和临时RPC预算；WiFi站点、HTTP、MQTT及ArduinoBLE HCI检查同一个owner，存储操作进行中不接管串口，失步后包括stop／析构清理在内不再发送RPC。网络和BLE guard只注入R4端口，不给其他MCU的共同实现增加hook或状态。库激活期间不允许Sketch直接操作Serial2、modem.timeout或另开Preferences namespace；高级自定义composition须承担同一所有权合同。baseline明确采用SDK的MODEM_TIMEOUT，不声称保存了SDK不存在getter的外部原超时。

每次task blob load／replace／clear采用policy.maximumCommitMillis作为整笔操作预算（当前候选1000 ms），包括启动读取、退役和清除。每条显式AT RPC发送前重算剩余时间，设置其响应等待timeout；完成后复核截止时间。端口直接复用SDK modem的Preferences命令，删除重复PREFLEN调用及有无条件PREFEND析构的Preferences对象，不改SDK或journal字节。

| 情形 | 端口处理 | 执行与恢复 |
| --- | --- | --- |
| 全部已完成回应有效，但下一调用前预算耗尽 | 不发送下一RPC；写尚未开始时可证明本操作没有put | 原store／engine故障锁，不自动重试mutation；时间到期本身不冒充介质损坏 |
| 元数据失败、回应格式异常、调用返回后越过截止时间 | modem隔离，不继续读取或发网络命令 | 不能把失败解释为NotFound；保留原表和请求记录 |
| put已发出，但返回失败或验证未完成 | 结果未知；不立刻readback、回滚或重发put | 不发成功receipt，不执行目标；服务ledger维持unknown，不能把Busy视为安全重试证明 |
| 正常put、完整精确读回且未超时 | 按原journal确认提交 | 原任务流程及精确请求去重不变 |

恢复要求整板断电上电，或明确同时复位连接模块并重新建立主MCU／owner生命周期，随后正常初始化并fresh load，再以新会话／授权查询旧请求。仅复位连接模块不能解除同一主MCU内的static owner隔离；end／begin、清空输入、仅RA4 RESET或重新设置timeout均不是已证明的协议恢复，owner没有原地unlock。已提交定义可被读取；Claimed／Running仍按原中断恢复，不重放Start。

1000 ms仅是端口响应等待和下一调用准入预算，不是硬实时保证。Core的UART::write在buf_read开始计时前无截止时间地等待TX，SDK也不暴露Error／Timeout细分；故false一律按串口可能失步隔离。put头已承诺长度后必须补完payload，其响应等待重新取剩余时间（到期则0），返回后仍判未知，不把补完误称成功或可取消。carrier的1000 ms还包含签名、排队和发送，独立存储预算不等于整个carrier、Blinker.run或普通WiFi重连上界。SDK响应std::string没有接收长度硬上限；644／664 B工作区有界不等于SDK异常接收RAM有界。正式准入仍需有限真实故障／恢复差异证明，不用健康耗时替代。

隔离触发限于库端口可识别的存储RPC失败／格式异常／迟到响应。普通Native WiFi／HCI复合方法仍由原SDK执行，其返回值不提供内部每一笔RPC的失步状态，本批guard不宣称截获了所有SDK超时。新增未来native回调若在poll中访问存储，须核对回调返回后的原生HCI调用；目前接收路径在回调后退出，不把该合同推广到任意第三方回调。

实施顺序：先改R4 owner／端口和网络隔离，再补剩余预算、写前停止、写后未知、重入、millis回绕、late返回、启动／退休／clear及重启后fresh load的有限回归；最后执行原SDK编译与资源差异检查。不部署App／服务，不清授权，不重复BLE循环。

#### 已完成的软件边界

共享owner没有heap、任务线程、第二执行器或原地unlock。task view直接承接原policy的maximumCommitMillis；默认凭据bank采用SDK baseline作为整笔预算，不永久降低WiFi／MQTT超时。原十二键／journal版本、generation、CRC、tombstone和任务codec不变；删除旧Preferences对象及SDK getBytes的重复长度查询。

Release＋ASan最终79/79通过，包括原任务迟提交、fault latch、exact-replay和回复重发。R4新增有限case覆盖namespace争用、同bank重入、初始化失败不发PREFBEGIN、实际剩余timeout及baseline恢复、回绕、读取late、写头耗尽、写入已应用但返回false／late、禁止后续RPC、clear／load故障和模拟整模块重启后fresh load。正常Client原生虚函数嵌套不自锁；两个网络客户端及BLE link在active／quarantine时均不调用native，站点poll／stop／重连也不绕过隔离。

测试媒体重启是软件模型，不是实物桥接复位证明。原“整个RPC无heap”断言已纠正为固定task workspace构造／析构无分配；真实SDK式664 B回复字符串如实计入mock heap，不放宽生产容量或SDK预算。首轮mock parser触发MSVC安全函数警告被WX拒绝，改为有界key及严格数字解析后重建通过，未放宽编译警告。源码架构门禁通过，新增防回退检查约束R4存储owner和网络／HCI guard。

#### 原 SDK 编译与剩余真机范围

| 门禁 | 结果 | 边界 |
| --- | --- | --- |
| 完整内部任务链接 | Core1.5.3、17实现单元、无全库PRODUCT宏；BIN183,164 B、程序183,156 B、静态RAM17,336 B | 仅公开placeholder配置，不投影真实Key／网络密码或控制地址；BIN SHA-256 `ae295b27f36b548017424143ee385c37fd42c6150ab316bb3e32a304c2cca52b`，未上传 |
| Plain／TLS／BLE对象 | 真实GCC普通对象编译均通过，BLE采用既有ArduinoBLE2.0.2；418源文件和安装投影前后摘要一致 | 不是三种固件的真机通信或硬截止证明；不升级SDK／依赖，不复制旧Sketch／库对象 |
| 32位资源 | owner32 B、bank80 B（原84 B）、task view1340 B、module2904 B、extension3792 B、Facade12 B | view新增4 B预算，module／extension因对齐较原各增加8 B；静态RAM比§11.9多44 B，非完整IRQ／动态峰值 |
| 零字段原生包装 | WiFi native／guard均32 B，SSL均48 B，ArduinoBleLink／guard均240 B | Plain／TLS平台180／212 B，BLE平台344 B；普通产品没有默认任务workspace |

本批软件、架构和编译证据留在忽略的`build/r4-rpc-budget-20261014/`，没有账号／授权／网络密钥输入，没有刷板、清授权、App／服务部署、SDK升级、暂存、提交或推送。现有不相关修改及用户删除保留。

下一批仅补上表不能替代的短差异：先核对现板和保护十二键／App请求记录，采用匹配的R4四槽候选；检查一次正常任务及原ACK／State；一次受控存储RPC失败或未知回应后验证无后续RPC／无成功receipt／无第二put；用户整板断电复插后fresh load和新授权查询，不重放Start。结束核对原身份／授权／task raw及日志，按确认范围恢复。若要使用合并后服务，先完成§11.8第3项最小匹配运行闭包，不能拿旧运行镜像为新源码签收。X1整体和普通任务准入未关闭；Wio X2继续在该共享边界后，不扩槽、SDK、BLE循环、H2或Bulk。

### 11.11 R4 有限故障验收前置条件

2026-10-14现场COM9／UID `F412FA6F2F88`与原授权板一致，手机ADB在线、匹配APK `f2bc1828…`不变，电脑位于mostfun2026／192.168.1.156。本轮沿用schema051及已验收冻结服务，不把合并后Gateway源码或052／053迁移暗中部署进RPC差异测试。

App账号／请求数据7,366,656 B、原APK、三端脏工作树、数据库与完整现有服务日志已新鲜保护。正常上传维护程序返回0，十二键1004 B raw／absent独立双读相同，任务两键均absent；保留实例和BLE授权。用户此前允许覆盖主程序，仍不将known WiFi恢复BIN称为本轮主Flash新鲜备份。备份归档列表查询曾出现输入管道EOF但子进程返回0；仅协调原已保存dump的只读完整列表，不重复备份或改数据库。

#### 存储与产品验收分离

旧R4服务scope仍active，20个requests／20个attempts全部verified，而任务介质已按上轮收尾恢复为空。同root的新Initialize会被`AUTHORITY_ALREADY_CURRENT`拒绝，旧verified请求只返回历史确认，不会重新初始化设备。没有保留可精确恢复的删除后任务raw，不能根据数据库重造快照、重放Start或改scope状态作为验收。此测试恢复前置条件须单独处理，不归因于新RPC预算。

产品闭环采用正常App建立临时WiFi逻辑身份与新的任务scope，保留原设备、Key、scope和请求记录。当前账号通过正常getAllInfo刷新为21台active owned设备、0台shared，额度10。用户已允许仅该本机账号临时调至22，经正常App创建后恢复10；实际调整／创建另据审计，不自动删除旧设备或改旧scope。新逻辑身份保留同一immutable instance与BLE授权，登录及任务scope合同须先核对，不能靠清身份解决冲突。

独立子门禁复用原停用四槽codec fixture及共享blob bank，不创建TaskEngine、任务授权或服务器receipt。串口显式arm下一次任务PUT，观察器将该次真实SDK回应窗缩至0 ms，再完整调用原passthrough一次；真实bool与耗时原样返回。成功不得强制改成失败，未确认写不得重复。故障后只做RAM状态和一次显式native网络guard检查，再由用户整板断电复插后fresh load协调实际介质结果。它证明真实SDK失败隔离／持久化恢复，不证明生产1000 ms硬上界、物理输出恰好一次或App／ledger闭环。

#### 测试工具收口

故障观察器仅在`BLINKER_INTERNAL_RPC_FAULT_GATE=1`编入内部候选，默认关闭对象无wrapper／observer符号；一次arm本身不发RPC或写入。计数限于GNU wrap覆盖的外部SDK引用，不声称是完整UART监听。原SDK对象可见测试wrapper栈帧164／172／32 B须另计，不借用旧生产峰值测量。

旧存储探针删除全局modem.timeout和失败后继续metadata的旁路，metadata纳入共享owner的整笔预算，失败即隔离。原fixture、slot-order和首写合同保留，本轮不重跑旧两笔健康首写／clear流程。新增观察器模型和既有Release＋ASan80/80、真实Core1.5.3对象及架构门禁通过；这些软件结果不关闭尚未执行的实物门禁。私有保护及候选留在忽略的`build/r4-rpc-live-20261014/`，不提交账号、Key、网络密钥或备份。

#### 真实 SDK 故障隔离结果

独立候选以原Core1.5.3完整链接：BIN81,464 B／程序81,456 B、静态RAM11,708 B，17实现单元，无全库PRODUCT宏。418个当前源码及安装投影、探针投影均匹配冻结摘要；BIN SHA-256为`ad3b4e9577c3517c531152e4da46ddb9561ba9e7b8f21449723289830696ddae`。标准上传返回0，主Flash没有读回。

| 检查 | 实际结果 | 范围 |
| --- | --- | --- |
| 写前状态 | OPEN成功，fresh READ为NotFound，两个task raw键absent；owner健康、计数为0笔task PUT | 仅既有身份读取，没有自动初始化／Start |
| 一次真实未确认写 | 显式arm后仅一次REPLACE1，完整664 B journal payload发送一次；SDK原bool为false，replace返回WouldBlock，耗时71,139 µs；原passthrough58,462 µs | 观察器只缩短回应窗，没有伪造bool；不是生产1000 ms故障上界 |
| 未知结果隔离 | outcome=WriteUncertain、quarantine=1、networkAllowed=0、writesLocked=1；PUT／payload／injection各1 | 失败分支没有立即readback、回滚或第二次put |
| 主循环与后续调用 | 两次RAM观察loopCounter18,534→40,338；write／write_nowait／passthrough为97／1／1且各自不变。显式Client／Station连接和stop检查前后总计数99不变；随后READ仅返回StateConflict | 真实生产网络guard的有限检查；不是所有SDK复合调用或完整UART计数 |
| 复电及收尾 | 用户暂不在电脑旁，COM9保留无执行器的安全隔离probe；整板断电／fresh介质读取及十二键／known WiFi恢复待测 | 不以软件复位代替断电，不在隔离态发送维护RPC，不重放REPLACE1 |
| 账号与原记录保护 | 原16个scope／250个request／253个attempt全字段原文精确不变；原R4为1／20／20。创建前读取阶段12个IndexedDB文件SHA不变，阶段完整日志及原文保留 | 创建前App tar7,366,656→7,367,168 B；创建后最终7,382,528 B，仅作备份元数据，不把正常创建后的文件变化说成逐字节不变 |
| 正常App身份准备 | 真实App确认额度22后，create／reveal各1次，保留新WiFi逻辑身份与Key；Auth立即恢复10，App经正常缓存刷新最终也为10 | 新设备session／task scope均0、instance未知、仍离线；注册不等于上线或任务初始化 |
| 当前产品编译 | 原Core1.5.3完整链接：BIN184,164 B／程序184,156 B、静态RAM17,372 B；418源、17实现单元、7个真实wrapper，新Key／网络／私有投影前后hash一致，无全库PRODUCT宏 | BIN SHA-256 `38d0507e113761fe57894af0ffb037241676b4f8122b9e514788ed699e317a60`；仅编译，不上板、不签收完整运行资源或产品闭环 |

#### 产品材料与恢复前置条件

固定schema051登录不以instance全局绑定cloud设备；任务scope按logicalDeviceId唯一。新手动WiFi Key可以保留既有instance／BLE record，仍须在App明确选择新逻辑设备，不通过instance反查选设备。旧active Direct条目会优先匹配，不能把旧设备记录迁到新scope。私有注册材料明确`physicalInstanceVerified=false`，新scope只在后续真实设备上线、正常授权初始化后成立。

额度采用真实Auth per-user override，不改Gateway snapshot或App缓存。首次维护发现操作者归因口径不合适，未创建新设备，先精确恢复10并保留原审计；后续独立阶段使用`granted_by=NULL`和明确local-maintenance来源，复用原用户锁／revision／outbox，再经正常App创建。两阶段结束均无active override，revision／outbox4保留，不回滚或删除审计。创建后的App曾仍显示缓存22，真实Auth为10；只读等待正常约5分钟缓存更新后确认App为10，原失败／协调记录不改造为成功，也不重复调额或创建。

产品编译原后置checker要求七条wrapper `.su`，实际GCC7只产生五条，故原失败保留。现有ELF独立复核七个强符号；五条自身栈16／24／16／32／24 B，两个varargs的完整prologue分别`4+20+144=168 B`和`4+24+148=176 B`，outgoing空间已包含，不能重复加计。联合静态自身证据已核对，不重编、不改BIN／源码或伪造SU；`compiled.json`保持`linkPassed=true`、整体`passed=false`及`hardwareAdmissionGranted=false`。它不能替代callee／IRQ／SDK动态峰值或真机准入。

最终APK摘要不变、账号与完整请求数据及五个相关容器日志已归档。原任务ledger逐行不变，新逻辑设备保留为下一批匹配目标；本批没有初始化、Start、目标控制、服务镜像部署或SDK升级。App已停止并撤本批三项USB转发，不登出／清数据。COM9仍为安全隔离probe，尚未恢复known WiFi或十二键，其他板未动。三端HEAD／暂存区及无关本地修改保留，没有提交或推送。

下一批只按以下顺序，不重派两个Countdown或普通通信：

1. 用户能操作COM9时仅整板断电复插；先确认RAM隔离／arm状态重新建立，再只读协调那一笔实际介质结果，SDK false不等于未落盘，不重放REPLACE1。
2. 正常维护程序恢复全部十二键raw到本批1004 B基线，并独立双读确认task键原absent。不能直接把synthetic fixture交给新TaskEngine：非空介质必须拒绝新Initialize，新逻辑ID不能绕过该条件。操作工具已阻止缺复电证明的再次上传及未恢复十二键的产品上传。
3. 核对现网，冻结新逻辑设备的最小服务准入／Ed签发子集与实际Broker；仅使用已核准匹配闭包，合并后整组部署不作为本轮附带操作。再审本批产品构建与有限资源范围，才标准上传。
4. 正常App仅补当前owner固件的一次停用定义保存及原ACK／State；其后显式arm一次停用定义Replace的真实未确认回应，检查无成功receipt／无第二put／无目标执行，整板复电后新会话fresh查询协调结果。结束恢复十二键及确认的固件，保留请求；Wio的软件准备可按§11.12并行，不替代这些实物步骤。

### 11.12 Wio共享QSPI与四槽软件接入

2026-10-14用户暂不能拔插，明确继续Wio内容。本批仅推进X2软件：不操作COM9、不协调那笔未知R4写入、不刷Wio或其他板、不部署App／服务、不改数据库、账号额度或测试网络。沿用Seeed SAMD 1.8.5和现有依赖，不升级SDK；当前Wio仍未获持久任务硬件准入。

#### 最小接线与存储边界

从原身份银行提取一个`WioTerminalQspiOwner`和原生读写端口，寄存器操作只保留在`WioTerminalQspi.inc`；删除银行内部重复的原生驱动。身份与任务各持独立lease，整笔事务持有Operation，防止底层`yield()`期间的协作重入；结束一个视图不关闭另一个。不是线程／IRQ互斥器，第三方直接操作同一QSPI也不会被该owner拦截。

| 区域（4 MiB设备） | 当前合同 | 准入 |
| --- | --- | --- |
| `[0x3fe000,0x400000)`尾8 KiB | 原身份BWB2双扇区及generation／CRC，572／508／488 B格式归一化保持 | 本批不改地址、格式或扩张身份银行 |
| `[0x3fc000,0x3fe000)`相邻8 KiB | 可选四槽任务双扇区；644 B原任务codec＋20 B外层BWT2 journal | 必须先明确整区独占和精确raw备份；空白、编译通过或宏设为1均不构成物理归属证明 |

Seeed的[官方读写示例](https://wiki.seeedstudio.com/Wio-Terminal-FS-ReadWrite/#readingwriting-the-flash-using-qspi)通过SFUD访问同一外部Flash，[原生QSPI实现](https://github.com/Seeed-Studio/Seeed_Arduino_SFUD/blob/master/src/Arduino_QSPI_Hal.cpp)直接配置外设。因此本候选不能推断它与外部Flash文件系统已有分区隔离：维护profile须排除绕过owner的SFUD／Flash FS并行使用。地址不重叠只是必要条件，不能阻止另一个driver重配或关闭QSPI；若以后共存，须先统一生命周期和整笔事务，再证明固定布局不覆盖这16 KiB并独立准入。不能格式化文件系统以取得准入，也不改用片内FlashStorage。

任务视图默认`regionReserved=false`，未准入时不调用native begin／read／erase／program。读写只落入任务区；更新只擦inactive sector，256 B逐页写，确认后精确读回；清除写更高generation的墓碑。当前格式CRC中断可从完整peer恢复；双坏记录、相同generation、不支持schema或超当前容量明确报错，不把损坏／不兼容表当Empty或擦掉再初始化。不可识别的部分擦除现场也保留失败，待精确备份及维护协调，模型不宣称覆盖全部掉电波形。

native false、超过整笔预算或读回不一致后锁定共享owner，当前生命周期不继续读回、发下一页、重复写或通过end／begin解锁；任务blob再由既有TaskSnapshotStore／TaskEngine处理故障，不制造成功receipt。独立RTL WiFi不因QSPI锁定被主动断开，但SDK内部等待仍会占主MCU：原100 ms instruction／1 s ready／10 s erase及cache／AHB调用保留，**当前预算仅控制调用准入和返回后迟到判定，不能保证硬截止**。1000 ms仅为内部编译候选，须实测后冻结策略。

产品只增加一个可选Wio task port，复用`TaskModule`、`WifiTaskExtension`、TaskEngine／Cloud carrier及Ed25519签名专用provider；不引入第二个MQTT或完整Direct crypto对象。普通容量0仍无任务workspace；四槽候选须同时满足内部平台准入、区域保留及写policy，限普通手动Plain WiFi。BLE／WiFi+BLE／LAN／TLS组合不在本批准入，用户API仍为原`enableTasks()`／`begin()`／`run()`。

#### 软件验证与资源

- 新增owner与任务介质模型，覆盖独立lease、协作重入、跨区拒绝、false／late锁定、页写／墓碑、有效peer恢复、版本／容量拒绝，以及任务访问前后身份区原字节和三种BWB2长度不变。补充TaskStorage读取／写后读回失败：即使输出已有完整字节也拒绝接受，后续无新增IO；模型fresh owner仅读取实际模拟介质，不重放写入，不提供生产解锁接口或代替真实断电。
- 同时有限纠正既有身份银行双CRC损坏被折成NotFound的缺陷：`newest()`区分全擦空与存在记录但无有效peer，后者返回InvalidEncoding；原`DeviceInstanceIdStore::loadOrCreate()`不再随机建新身份，replace／clear也拒绝改写。先用真实store入口模型复现，再修正并通过；原空表创建、单好peer恢复、格式及正常选择语义保持，没有新错误处理链或新协议。
- Release＋ASan整体82/82、发布工具45/45及架构／冻结检查通过，`/W4 /WX`不放宽。MSVC链接仍提示ASan符号诊断建议，不影响实际82个测试执行，不将建议消除计为修复。首轮失败为模型把原身份572 B native调用错当单页，已仅将任务区域限定为单页，未为测试重写原身份算法；失败不计产品通过。
- 真实SDK首轮9项配置／ABI和3项API对象通过；schema／容量及身份双损坏修正后，最终default／task4／native对象与双目标隔离完整链接通过。compile-only fixture保留公开入口供链接，`setup()`／`loop()`不打开Flash、联网、Initialize或Start，不能烧录作为运行验收程序。
- 真实32位ABI：owner36 B，身份bank68 B、平台192 B、Facade12 B；默认产品3512 B。共享owner全局新增48 B，不能写成普通产品零RAM增加。可选四槽task view1344 B（含两个664 B长存活缓冲）、module2912 B、extension3800 B，产品7312 B；没有新增默认大型缓冲，不用静态余量替代运行峰值。

最终两目标使用同一Sketch、同一423源／17实现单元与原SDK1.8.5；差别仅为四槽内部admission／region／policy宏。维护profile及密钥未改，源码／Sketch／安装投影前后逐文件冻结核对。SDK与构建缓存允许使用，CLI因新安装路径重编了多项对象，不称冷SDK构建或全部缓存命中；修正前`full/`证据保留，不作为当前源码通过。最终结果留在忽略的`build/wio-task-product-audit-20261014/final-revision/`，不生成或推送交付包。

| 最终完整链接 | BIN | 程序`.text` | `.data`＋`.bss` | 范围 |
| --- | ---: | ---: | ---: | --- |
| 普通任务关闭 | 148,856 B | 148,112 B | 736＋67,512＝68,248 B | nm确认无TaskEngine／TaskSnapshotStore／TaskCloudTransport／Ed25519验证图 |
| 内部四槽 | 185,752 B | 184,992 B | 752＋71,520＝72,272 B | 实际store／engine／carrier／签名provider保留；不是运行峰值或硬件准入 |

同Sketch四槽相对关闭档BIN增加36,896 B、静态RAM增加4,024 B。其中3,800 B是可选extension，余224 B经真实nm／map核对为Crypto RNG singleton160 B、两任务binding及owner／guard32 B、可信key-ring12 B和20 B初始化项／对齐；不是全算成任务缓冲。普通共享owner新增48 B另取上述ABI证据，不能用两个不同历史Sketch相减推导普通RAM退步。RNG构造仅初始化字段，不调用begin／EEPROM／QSPI；没有为此新增RNG初始化或另一套crypto。所有静态结果均不能替代native等待、caller／IRQ、SDK动态峰值和实际擦写测量。

最终普通／四槽BIN SHA-256分别为`8de445df7610dae160b0cba96aeefc654e23a274043a0bc8db6d537abacbf279`／`12d46e83351fe3845c556edbacf5f239aff6bc283293cff7c156a59b9fcf2948`，输入摘要为`b538e46afe103b6e4ebcc436ece09d59eba822e03e6465b7ccf0925dc720f20c`。本批只关闭X2软件准备，实物、运行资源及平台正式支持仍独立待办。

资源只读解析工具的两次适配失败保留：最终布局中4 B额外BSS对齐落入TimeSync对象而非产品，修正归因口径后按实际map／symbols核对224 B；不改源码／BIN、不重编或删旧失败，也不将解析失败算成产品编译失败。

#### 实物顺序（维护与探针准备取§11.13）

1. 用户能操作设备后，先核对Wio身份、JEDEC容量与是否存在外部Flash FS；独立保护上述精确16 KiB及原固件／授权，并确认任务8 KiB独占。不能确认则保持任务关闭，不做试写。
2. 先存储探针，不建TaskEngine：一笔644 B写入／读回、停止身份视图后任务读取／墓碑、整板复电和有限未知结果协调；原身份8 KiB逐字节不变，记录擦写／整笔耗时及资源。原源码的1 s carrier等待不能由静态剩余RAM证明足够。
3. 准入策略和region证据明确后，再准备仅该逻辑设备的匹配任务scope／Ed signer，使用原App完成保存不Start、显式Countdown、Completed／Interrupted、删除和原ACK／State；实时与H1只补受影响差异，保留完整请求日志，按确认范围恢复。

R4仍按§11.11等待真实整板复电及恢复；软件并行不改变未知写入恢复顺序。暂不扩槽、BLE持久任务、H2、离线历史／Bulk或整组服务维护，不暂存、提交或推送本批变更。

### 11.13 Wio固定区域维护与存储探针准备

2026-10-15本批完成固定区域维护及单启动存储探针的软件准备，不关闭全部复电准备或实物准入。串口枚举只有COM9 R4与COM11 ESP32S3，没有Wio；未打开串口、操作ADB、调整网络、部署App／服务、改数据库或刷板。R4那笔未知PUT、十二键保护和待整板复电顺序保持§11.11，不用软件reset、另一维护程序或Wio模型替代。

#### 为什么先补维护，再试写

原Wio维护只保护身份尾8 KiB，且复制原生QSPI driver；不能用于证明新的任务区能精确恢复。本批删除内部维护工具的复制驱动，`WioRawQspi.h`只作兼容薄入口，`WioOwnedQspi.h`注入并复用生产共享owner、独立region lease及整笔剩余预算。生产`src`、四槽codec、journal、用户API和普通产品默认配置均未修改。

| 内部维护模式 | 精确raw范围 | 写权限与守卫 |
| --- | --- | --- |
| 0，默认 | 原身份尾8 KiB | 保留原身份恢复合同；不增加任务区权限 |
| 1 | 任务＋身份固定16 KiB | 仅读；任务8 KiB不解码、不归一化、不认领 |
| 2 | 同一16 KiB | 仅任务两扇区；原备份SHA匹配、身份8 KiB原字节相同、显式区域独占才允许恢复 |

读出全FF、取得软件lease或配置`regionReserved=1`都不证明物理独占。仍须核对原固件没有SFUD／Flash FS旁路及布局覆盖；维护不提供任意地址、整片擦除、格式化、创建新身份或RTL固件操作。任务`clear()`是墓碑，不是恢复原全FF；收尾必须精确恢复原任务raw，并独立两读整个16 KiB与原备份逐字节相同。

所有64 hex配置在编译期检查长度／字符。模式1／2的63 B公开`p`元数据包含mode、预期布局、主MCU UID和source ID，不打开QSPI、不输出秘密。UID顺序与本机原Seeed1.8.5的USB serial实现核对；source ID必须连同冻结输入、匹配BIN及上传证据核对，不是安全启动或签名证明。真实native begin只验证容量字节／4 MiB边界，当前不导出完整JEDEC manufacturer／type，不能报告已测得三字节JEDEC。

#### 单笔探针，不启动产品

新`tests/internal/arduino/WioTerminalTaskStorageProbe`只构造既有owner／身份bank／四槽blob／Codec，不构造TaskEngine、网络、MQTT、grant或receipt。开机只初始化Serial，不自动打开Flash、写入、认领或reset。显式有限命令按以下顺序：

```text
OPEN_CAPTURE → REGION_ATTEST → READ(NotFound)
             → REPLACE1 → READ → IDENTITY_END → READ
             → CLEAR_FIXTURE → READ(NotFound)
```

`OPEN_CAPTURE`两次完整流式SHA核对原16 KiB备份，再仅加载既有身份，不loadOrCreate。`REGION_ATTEST`要求操作者明确独占，额外预检任务区全FF；已有任务或外国数据保留，不擦掉再试。`REPLACE1`只提交一笔四条停用／Idle的644 B已知fixture，使用原664 B journal，真实load与全字节比较并报告SHA；不新增decoder或任务执行链。`IDENTITY_END`停止身份view后任务lease仍可读取；所有阶段验证身份8 KiB原SHA不变。clear仅证明原墓碑语义，raw恢复另走受限维护。

replace与clear各自一次性消耗意图，执行前进入失败保留态。异常、迟到、未知或校验不一致后不再native I/O、补写、cleanup或end／begin解锁；协作yield重入也不能发第二操作或让失败状态恢复成成功。健康结束释放自己的lease，不影响另一个view。预算只控制调用准入及返回后迟到判定，不声称SDK cache／AHB／擦除等待已具硬截止。

冷启动会回到Fresh；写入后完整16 KiB不再等于原baseline，且任务区不再全FF，因此不能重新走`OPEN_CAPTURE→REGION_ATTEST`做fresh load。mode1冷后raw双读只证明介质字节存续，不能证明生产TaskStore冷恢复，更不能证明产品任务恢复。禁止把baseline改成写后current SHA来绕过原备份绑定。下一小批须补独立、只读的冷恢复候选，联合绑定原身份8 KiB摘要、原备份、明确UID、冷后raw及原写入意图，只调用既有TaskStore begin／load和Codec核验，禁用replace／clear／执行器；不重造恢复链。

#### 主机协调，不自动恢复

`tests/hardware/wio_storage_gate.py`只驱动已经核对、烧录完成的夹具，不上传或reset。按明确端口＋实际USB UID＋公开source＋mode／布局匹配；固定位置解析CRC帧或有限JSON，拒绝错prefix、重复字段、超长、尾随数据及部分回复，不扫描magic、清空输入、自动换板或重发。

原16 KiB只以exclusive新建文件保存在忽略的私有`build`路径，独立读回SHA，不覆盖原文件。每个UID有固定session claim，换目录不解锁；每条probe命令及任务raw恢复都在发送前fsync意图。未知保留pending；恢复意图一旦发送永久消耗，不因丢ACK重新发`r`。用户真实整板复电后，显式`reconcile`只做新鲜双读、私有存档及身份／原raw对比，不能重放写入或以读取成功自动重开probe。

若完整raw已等于原备份，可只读确认恢复并关闭本session；若仍不同且从未尝试raw恢复，只有独占、真实复电确认及匹配mode2才可进行唯一一次受限恢复。尝试恢复后仍不同或身份变动，保留原备份、意图与读回现场，停止并人工协调。capture两读失败且original／state／`.next`均不存在时，仅同目录、同source、显式整板复电后允许重新只读捕获；已有残留文件则停止人工协调，不覆盖或删除claim伪造新会话。source全零只作编译placeholder，不可操作硬件。

#### 验证与待办

- 当前Release＋ASan、`/W4 /WX`整体86/86通过；新增探针和三种维护mode模型，覆盖原8 KiB恢复不变、区域拒绝、身份不写、完整剩余预算、重入及失败锁定。既有MSVC ASan诊断符号建议保留，未放宽警告。
- 主机31项离线回归、既有45项发布回归及架构检查通过；离线测试不导入pyserial、不打开USB。架构新增约束维护不得复制原生驱动、probe不得引入开机IO／网络／第二执行器。独立审阅发现capture中断后claim保留但无法继续仅读，已有限补同session／空文件／真实复电守卫，不扩展擦写权限。
- 原Seeed1.8.5、CLI1.5.1、jobs1：四项正向对象及十项精确负向守卫14/14，维护mode2＋probe完整隔离链接2/2通过。423生产源与§11.12最终版逐字节相同，每目标17实现来自本次安装投影，无全库PRODUCT宏；不重编已通过的普通／任务产品，不升级SDK。

| 本批完整链接 | BIN | `.text` | `.data`＋`.bss` | 边界 |
| --- | ---: | ---: | ---: | --- |
| maintenance mode2 | 39,116 B | 38,492 B | 624＋35,652＝36,276 B | 含两个16 KiB维护缓冲；只恢复任务区，不进入普通产品 |
| storage-only probe | 50,540 B | 49,892 B | 640＋6,812＝7,452 B | 真实encode／644 B比较／load／replace／clear；无产品或任务执行器 |

实际32位owner36 B、维护helper24 B、controller8 B、probe port3920 B；静态值不能代替运行栈／IRQ／原生等待峰值。mode1对象无擦写引用；两完整图均无产品、TaskEngine／Cloud carrier、设备登录或Ed验签。BIN只使用公开source编译tag、全零baseline及编译专用region声明，**不得直接烧录作为匹配运行候选**；真实原备份与独占证据取得后须重新冻结、编译及核对。单启动probe不称decoder、cold TaskStore恢复或实物通过。

固定输入摘要`0f423405cc66a20ed50c0b552fd26442f1560b6d019144a081d2e73c5a731a92`；维护／probe BIN SHA分别为`d39db75ba9a8830dd4898a72e539bfbe6934214d0bb23f5af31e55196d8072a6`／`844bb3754b354d3537c9f9ae7a037f58fa9070cae16ff4b7664a359e4877eb06`。汇总与实际对象／ELF／日志留在忽略的`build/wio-storage-tools-20261015/`，原Sdk依赖缓存可复用，不声称SDK全冷构建。probe安装只机械投影相邻helper include，原头及映射摘要均保护，未另造驱动。

首轮四对象本身编译成功，工具错误要求未调用的decoder符号而停；首次完整链接参数多余转义又在Sketch阶段失败。两次失败材料保留，有限修正忽略的验证工具后独立attempt2完成上述门禁，未改生产或冻结夹具来绕检查。

下一批只按以下顺序：

1. 先补上述只读冷恢复候选及有限模型／原SDK验证，仍无写权限；当前单启动探针保持冻结，不把raw读取当fresh load。无Wio时可先完成该软件缺口，不去操作R4。
2. Wio接入后核对实际USB UID、原固件／区域／授权及SFUD／FS独占依据。先准备原主固件恢复材料，再用匹配mode1保护精确16 KiB；失败则停止，不试写。
3. 用原raw SHA冻结匹配单启动probe、只读冷恢复与mode2恢复BIN，核对编译／上传材料，再执行单笔写入、整板断电fresh load及原raw精确恢复；身份8 KiB逐字节不变。记录整笔和擦写耗时／资源，不以静态剩余RAM证明1 s carrier足够。
4. 只在存储与预算实物准入后接入原App／scope／Ed签发，补保存不Start、显式Countdown、Completed／Interrupted、删除及ACK／State。实时与H1只补受影响差异；纯BLE不等同云历史。

X2区域归属、真实擦写、整板复电、运行峰值、App短闭环及正式平台支持仍待办。X1待R4复电；不扩槽、BLE持久任务、H2、离线历史／Bulk或整组服务维护。本批没有暂存、提交或推送，已有无关修改与删除保留。
