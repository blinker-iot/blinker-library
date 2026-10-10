# B3：普通设备业务授时实施合同

> **2026-09-22：历史授时合同/证据保留，执行顺序仅见[主线 §6](DEVICE_V2_NEXT_EXECUTION_PLAN.md)。** 新本机定时按[任务合同 §4](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md)实施；当前默认有效样本最长一小时、误差也可提前使其失效，断电无源则未同步。授时通过不等于长期离线日历已完成，不执行下文旧“下一步 B4”。

更新：2026-09-13。执行入口为[基础路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)。普通 WiFi 可选接线、Broker responder 见 §8；
最新 **B3-4c-4b 南向授时/控制与本地 TX 背压修正见 §19**；自身 Hub 台架见 §18，匹配 APK 精确检查点/仅重置网络真机见 §17，WiFiBLE 自身见 §16，南向软件见 §15。
§19.4 已补来源不可用/恢复、活动断开及原固件控制复验，本组南向短闭环收口，**下一步回 B4 单槽接替与通用 LAN**；不能跳过独立剩余边界宣布 B3 全部完成。§14 的“待接线”、§16 的“APK 尚未安装”、§18 的“南向尚未验证”为当时状态。
普通 WiFi + LAN 明文授时真机见 §13；B3-4a BLE 真机、LAN 接线和 R4 安装对照见 §12，基础模块/资源见 §11。
B3-3b 的同步输出/真实短离线/五分钟刷新已通过，证据见 §9。B3-4 后续接线顺序见 §10/11；B3-0/1/2 为基础及历史记录。
不重复 B1b 真机，不改授权、系统 RTC/TLS 时间或 NTP SDK 配置。软件通过不等于设备已自动授时。

## 1. 第一原则与实际消费者

“知道当前日期”与“控制有效期、重试、授权”不是同一能力：

| 当前消费者 | 当前输入 | B3 处理 |
| --- | --- | --- |
| Client/DeviceRuntime 的重试、限流、实时租约 | IClock.monotonicMillis | 保持原单调时间，不吃业务 UTC |
| BleEnrollmentApplication 的 hasTrustedTime | IClock.unixTime | 原安全判定保留，不注入手机业务样本 |
| GatewayAccessDelivery/凭据续期/PermitJoin/Proof/Revocation | IClock.unixTime | 原准入及过期判断保留 |
| GatewayChildSession/RouteBridge | IClock.unixTime | 不因业务校时延长或恢复凭据 |
| 普通 WiFi/BLE 的用户日期查询 | ArduinoClock.unixTime=false | 共用可选 WallClock/facade；WiFi 已真机，BLE 接线与待验边界见 §11 |

现有 ESP32 EdgeHub 的 SNTP 同步标志不是密码学证明；本批既不提高其安全声明，也不替换它。
WallClock 不继承 IClock，不持有平台/网络/Flash 对象。没有每 peer 新槽、第二套组件模型或每条 State 的时间字段。
启用普通 facade 时创建一个静态无堆服务；默认接口指针成本和启用对象预算在 §8 明列。

## 2. B3-1 纯核心合同

实现位置：`src/BlinkerV2/core/WallClock.h/.inc`，复用既有 BlinkerCore 编译单元；普通 WiFi 的可选集成见 §8。

- 输入 `WallClockSample`：UTC 毫秒、样本误差估计毫秒、来源标签。标签只表示来源，不表示已鉴权或比另一来源可信。
  适配器在调用 `synchronize(sample, nowMillis)` **之前**完成授权、时效和来源选择；核心不是报文鉴权器。
- 输出 `read(nowMillis, snapshot)`：成功才提供 UTC/年龄/误差/来源。未同步或过期返回 false，清空旧输出并保留状态区别，
  不用 0 冒充当前时间。纯核心毫秒/快照与 Sketch `Blinker.time()` 的 UTC 秒接口区分，见 §8。
- UTC 范围为 `[2000-01-01, 2100-01-01)`，单位毫秒，拒绝 0、把秒错当毫秒及溢出。
  使用 uint64，不受 32 位 time_t/2038 限制；不内置时区、日历或定时任务。
- 默认最大样本年龄 3,600,000 ms、最大误差估计 5,000 ms、漂移预算 1,000 ppm；配置可收紧，
  这些是业务可用性预算，**不是已校准晶振或可信时间保证**。
  配置范围分别是 1..2^31-1 ms、1..60,000 ms、0..1,000,000 ppm；非法配置不能安装样本。
- 误差估计 = 样本误差 + ceil(样本年龄 × ppm / 1,000,000)，按总年龄计算，不能每次 poll 向上取整累积。
  年龄达到上限、误差超过上限或 UTC 到达上界即过期；过期后新样本才恢复。
- 32 位 tick 用无符号差值，支持一次正常回绕；必须定期 `poll(nowMillis)`，调用间隔小于 2^31 ms。
  大间隔/可检测 tick 回退失效。超过整圈后同余 tick 无法凭 32 位值区分，禁止宣称能自动检测任意休眠；
  产品每次 run 调用 poll，已知 tick 重置/不可计时的深睡眠恢复必须 clear。断电不持久化 UTC 锚点。
- 已选中的新样本可以向前或向后校正业务 UTC，不做暗中钳制或复杂 slew；不改任何 IClock/系统时间。
  无效样本返回精确错误，不能覆盖当前有效样本；clear 回到未同步。
- 无堆、无虚表、不含 Arduino/网络/NTP；目标对象不超过 48 B（实测 ABI 单列），不按组件/连接数量预分配时钟。

## 3. 最小跨端样本交换语义（B3-2 已实现 wire）

只复用原 MQTT/已授权 Direct 或 LAN/网关子设备 BBP 会话，不新增连接，不借组件保留 key 修改系统时钟。
设备主动申请一次样本，回应端在构造响应前取 UTC；设备测量本机往返时间，不让回应端声称自己的网络延迟。

canonical CBOR body 固定为：

```text
TimeRequest  [requestNonce: bstr(16)]
TimeResponse [requestNonce: bstr(16), utcAtReplyMs: uint, sourceUncertaintyMs: uint]
```

没有额外版本字段/设备 ID/来源字符串：BBP 已有版本与路由，来源由本地选中的已授权 peer 决定，不信响应自称来源。
typical body 为 18/30 B（毫秒 UTC 使用 9 B unsigned、误差使用 3 B unsigned）；加 10 B BBP 头为 28/40 B，
启用 CRC 再加 4 B。这是已实现 codec 的字节数，**不是 MQTT/TCP 总流量，也不表示产品已发出授时报文**。
响应 body 随误差整数宽度为 28..30 B。CRC 仅设备通用 frame 支持，现有 App/Broker codec 不支持 CRC，首条云闭环不开启它。

B3-2 三端分配已核对：TimeRequest = **0x22**，TimeResponse = **0x23**，FeatureTimeSync = **bit 24 / 0x01000000**。
bit 19 已用于 DirectReady，20..23 也已占用，不能因为设备枚举没有完整列出便复用。
这是 B3-2 的历史边界；后续普通 WiFi 协商/公开 time API 见 §8，普通 BLE 与 App Direct responder 见 §11。
普通账户 MQTT 的 APP_FEATURES 仍不增加该位；不能将物理设备授时与账户业务路由混为一谈。
授时不能注入普通 App Route/Delivery：没有把这两个 kind 加入原可路由业务集合。

请求 flags=0、sequence 非零；回应仅 IsResponse、sequence 回显请求（设备解析已验证的 CRC frame 时允许额外 HasCrc32c）。
不使用 AckRequired/IdMode/可靠重传。nonce 必须恰好 16 B 且非全零；UTC 范围沿用 §2，来源误差整数 0..60,000 ms。
拒绝负数、浮点、非最短整数、非定长数组、错误长度和尾随字节。请求不携带来源/权限自声明。
以下请求生命周期固定：

1. 每设备最多一个在途请求，通过 IRandom 生成随机 16 B nonce；无安全 RNG 时返回错误，不用 tick/MAC 替代。
   协调器绑定 transportId + sessionId + 非零 generation + 选中来源 + sequence + nonce + 发起 tick。
   sessionId=0 对单会话 transport 可有效，generation 必须由 runtime 在换连接/换 peer 时更新，不来自 wire。
   保留上次公开 nonce 拒绝紧邻重复；跨 boot 的唯一性依赖安全 RNG，不声称保存了所有历史 nonce。
2. 未协商能力、不具权限、不匹配 peer/代际/nonce、超时/重复回应全部不能校时。
   断链/换源/撤权/发送失败由适配器调用 cancel；重启/不可测睡眠另需 clear 业务时钟。
   cancel 不清除有效 UTC holdover；超时重试用新 nonce，不能把旧重传当新 RTT 样本。
   wrong peer/nonce/sequence/flags/坏编码不消费当前合法请求；同 peer 的当前授权/能力丢失则取消。
   匹配的有效响应仅消费一次，即便最终因误差/UTC 预算不能安装，也不能反复提交同一个样本。
3. 首期 RTT 上限 2,000 ms（含边界，2,001 ms 失效）；在实际发送前 begin，任何排队时间都计入该保守预算。
   估计接收时 UTC = 回复 UTC + floor(RTT/2)，
   误差 = 来源误差 + ceil(RTT/2)，通过范围/预算检查后才能安装；不宣称这个对称估计等于真实单向延迟。
4. 响应必须关联设备请求，拒绝未经请求的写时钟。来源没有有效 UTC 则明确失败，不发送 0 或沿用过期时间。
   错误响应关联、发送失败和退避接原运行时，留 B3-3 验证；当前协调器只接收 TimeResponse，其他消息不会写时钟，
   没有错误处理接线时最迟在原 2 秒截止取消，不额外维护重试队列。

## 4. 三端职责和权限

- **设备**：纯核心只接受已处理的样本。单个可选协调器只负责已选 peer 的在途关联/准入/RTT，runtime 转交消息并 poll；
  来源优先级和请求时机/退避属于 B3-3/4 的薄产品策略，不让无网络依赖的协调器枚举或持有 transport。
  不把新的状态塞入每个 EndpointSlot；未启用授时的产品不创建对象/请求、不宣告能力。
- **Service/Broker**：普通设备优先从原认证云连接获得时间；只在有效 device session 上响应其自身请求，
  不把其他 App 的业务 Route 自动当云授时回应。原明文/TLS 策略保持，nonce 不是消息 MAC，
  明文链路仍可能受主动攻击，不能将此样本升级为授权/证书安全时间。云端响应 handler 归原 Broker；
  Gateway 不新增 HTTP 时间接口/数据库表，部署开启前须完成 Broker 身份/nonce/限流门禁。
- **App**：作为设备请求的 responder，在已认证 Direct/LAN 会话且拥有 Control 权限时才参与，
  只有 Observe 的共享用户不能改设备业务时间。使用当前系统 UTC，不要求进页面前校准所有设备；
  没有第二个账户 MQTT、额外后台轮询或 UI 前置。App 时钟也可能错误，来源标签不能绕过权限。
- **网关**：有效子设备 Control 会话上回应同一种样本；只转发自身有效业务时间和已增长的误差预算。
  没有有效业务时钟则失败，不直接把安全 SNTP 标记当子设备“高可信时间”。不为每个子设备建立独立 NTP 客户端。
- 首期偏好有效云端样本；无可用云源时才选择已授权网关/Control 手机；显式平台 RTC/NTP provider 后置。
  薄产品策略每次只选择一个来源交给协调器，不让并行响应竞争覆盖同一个时钟；不能凭枚举数值排序。

## 5. 分包顺序与退出条件

| 批次 | 实施内容 | 验收/状态 |
| --- | --- | --- |
| B3-0 | 消费者审计、业务/安全分离、样本/权限与来源语义 | 本文；不是 wire 已上线 |
| B3-1 | 纯核心与测试、ABI/无堆/安装边界 | 已通过；结果见下节，不是三端自动授时完成 |
| B3-2 | 三端统一 wire/golden + 单在途请求协调器 | 已实现；无授权/跨 peer/代际/nonce/超时/重复/RTT/来源切换软件门禁见 §7；不是 runtime 已接线 |
| B3-3 | 普通 WiFi→云端最小闭环和统一可选 facade | 接线/软件/部署见 §8；真实 COM11 与离线/重启证据逐项记录 |
| B3-4 | 同一核心接 Control 手机 Direct/LAN 与网关回应 | 只改适配器；Observer 拒绝、跨源回拨不影响安全租约；硬件按现场 |

目标仍是统一简单的可选 `Blinker.time(uint64_t& utcSeconds)`；未启用/未同步返回 false、不改变 begin() 参数。
启用方式、端点能力和 facade 存储注入已在 B3-3 一起接线；不支持的 composition 显式拒绝，不伪造可用 API。
R4/Wio 未接硬件、已有 B2 边界继续单列，B3 后回 B4；不展开 History/Bulk、日历任务、网关规模或长期循环。

## 6. B3-0/1 历史证据

已完成 `WallClock.h/.inc`，只在原 `BlinkerCore.cpp` 加一个 include；没有增加编译单元或平台依赖。
原 ArduinoClock/ESP32 SNTP/安全消费者/公开 begin/run/默认产品存储均未改，尚无产品实例化此对象。

验证：

- 原 host 全量 **59/59** 通过，使用既有 MSVC Debug/ASan 配置（非 UBSan）；较上批增加一个 WallClock target。
  新 target 含 7 组断言：未同步/清理、输入范围与拒绝不覆盖、非法配置、年龄/恢复、漂移预算与 poll 频率无关、
  tick 回绕/回退/边界，以及业务回拨/前拨与独立 IClock 不变。包含超过 2038 的 UTC 上界测试。
- 架构/冻结检查通过，发布包测试 **11/11**；这些是本批软件结果，不重复计算 B1 真机门禁。
- freestanding `-std=c++11 -Os -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections`，
  三目标 GCC 严格 warning 编译和 `--gc-sections` 链接通过；R4/Wio 使用 B1 安装缓存的 M4 hard-float 编译器配置。

| 独立探针目标 | WallClock | Snapshot | 启用调用探针 text | 未使用探针 text |
| --- | ---: | ---: | ---: | ---: |
| ESP32-S3 / GCC 14.2 | 40 B | 24 B | 1,212 B | 5 B |
| Cortex-M4 Arduino / GCC 7.2.1 | 40 B | 24 B | 1,384 B | 2 B |
| Cortex-M4 Seeed / GCC 7.2.1 | 40 B | 24 B | 1,384 B | 2 B |

上述 text 包含测试入口和必要 libgcc 辅助，不是 Arduino 整个固件的增量。未使用探针只剩空入口，
无 WallClock 符号；链接没有堆分配引用，data/bss 均为 0（对象由探针放栈上），**不表示已启用对象不用 RAM**。
40 B 是单个对象存储，24 B 是调用者快照；未测产品栈峰值。该模块未接入产品，所以没有默认产品新常驻槽。
这是 ABI/链接验证，不是 R4/Wio 新 Arduino 固件安装或真机授时通过。

证据在 ignored `build/b3-clock-20260913/`（host cmake/ctest 日志、ABI 源/脚本/对象/ELF 和 abi.json）。
只更新三端文档接力入口；没有 App/Service 生产代码变更、刷板、清授权、部署、暂存/提交/推送。

当时下一包为 B3-2，现已推进至下节；不要再审计一次同样的 IClock 或重复 B1b。

## 7. B3-2 实施与验证（2026-09-13）

最小职责分离：

- 核心 WallClock：仍只管业务 UTC/年龄/误差；原私有范围常量提升为 C++ 共享定义，codec 不重复写一套范围。
- protocol/bbp2/TimeSync：复用既有 canonical CBOR Writer/Reader；没有新序列化库。
- runtime/TimeSyncCoordinator：只借用一个 WallClock，保存一个在途请求；无堆/网络/虚表/连接队列/每 peer 槽。
  复用原 BlinkerProtocol/BlinkerRuntime 编译单元，不新增平台文件或 SDK 绑定。
- Broker cbor.ts / App codec.ts：复用各自原 CBOR 原语，只增加独立编解码；不新增 Gateway 服务或 App 页面。

权威 golden 为设备仓 tests/golden/time_sync_golden_vectors.properties；
Broker test-fixtures/ 与 App tests/fixtures/ 保留逐字镜像，保证单仓可测试。
跨端门禁先比对镜像，再以 BLINKER_TIME_SYNC_GOLDEN 指向权威文件跑两端测试，不维护三套不同预期。
新增测试进入原 host CTest、Broker test:device-v2 和 App Vitest。最终结果记录在本批 ignored build/b3-time-sync-20260913/。

最终验证：

- 设备 host **60/60**（MSVC Debug/ASan）通过。新增 target 覆盖三端 golden/所有截断前缀/编码范围、无授权及 Observer、
  跨 peer/连接代际/来源/nonce/sequence/flags、重复和旧响应、撤权/能力消失、来源切换、2 秒边界/tick 回绕、
  RNG 故障/紧邻重复、RTT 奇数舍入以及坏样本不覆盖已同步时钟。
- Broker 类型构建通过；TimeSync 与连接需求/本地控制/账户路由/实时租约/网关命令专项 **136/136**，
  原 Device V2-only 回归通过。匹配 SQLite 原生依赖使用本机 Node 22.22.3，不修改依赖或 lockfile。
- App 协议 **11 文件 / 156 项**通过，app/spec TypeScript 检查通过；未构建/安装新 APK。
  既有使用全局 describe 的测试需 --globals；测试运行参数修正，无关旧测试源码未改。
- 架构/冻结检查、发布包 **11/11**通过；三端镜像一致，Broker/App 均实际读设备仓权威 golden 重跑。

过程异常保留：第一次 Broker 扩展组与其他构建并行时，既有本地 HTTP→MQTT 中止测试在 12 秒预算取消。
该用例独立运行通过；同六文件 --test-concurrency=1 全部 136 项通过。未提高超时或修改生产取消逻辑，
不能把这次软件回归称为压力门禁通过。首次错误使用 Node 24 的原生 ABI 不匹配也已通过切回现成 Node 22 解决。

资源探针（C++11 / -Os / 无 exceptions/RTTI / section GC）：

| 32 位独立目标 | WallClock | 协调器 | 输入 peer 快照 | 启用探针 text | 未使用 text |
| --- | ---: | ---: | ---: | ---: | ---: |
| ESP32-S3 / GCC 14.2 | 40 B | 40 B | 20 B | 3,700 B | 5 B |
| Arduino Cortex-M4 / GCC 7.2.1 | 40 B | 40 B | 20 B | 3,940 B | 2 B |
| Seeed Cortex-M4 / GCC 7.2.1 | 40 B | 40 B | 20 B | 3,940 B | 2 B |

常驻对象合计 80 B，仅启用者持有；peer 是调用者输入快照，不在协调器中重复保存完整结构。
text 包含核心、所需 CBOR、协调器、测试入口和必要 libc/libgcc，不是完整 Arduino 固件增量；
尚未包含 B3-3 调度策略、发送缓冲/调用栈峰值。未使用链接仅空入口、无 blinker 符号；全部无堆引用、data/bss=0
是探针把对象放栈上的结果，不表示启用模块不占 RAM。不是 R4/Wio 新真机授时通过。

下一包固定 **B3-3**，按一条纵向链闭合：

1. 复用原有效设备 MQTT/BBP session，在 Hello 协商后选择云源；只响应物理设备自身请求，限速/断链/撤权边界明确。
2. 普通 WiFi 产品可选注入 WallClock + 协调器 + 薄调度，不增加 begin 参数；实现真正可用的统一 time 查询。
3. 发送成功/失败、Error、超时退避与新连接 generation 接既有 runtime；未安装 responder 的对端不宣告 bit 24。
4. 软件通过后 COM11 短真机：首次授时、读取、离线 holdover/过期、重启未同步→重新授时；保持 Property/Action/采样租约。

然后 B3-4 才接 Control 手机 Direct/LAN 与网关回应；继续不扩 History/Bulk、NTP 安全重构或长循环。
本批没有刷板/清授权/新账号/部署/迁移/暂存/提交/推送；尚无默认自动授时或公开 time API。

## 8. B3-3：普通 WiFi 可选业务授时（2026-09-13）

### 8.1 实现与使用边界

```cpp
void setup() {
    Blinker.enableTime(); // 在原 begin 之前可选开启；应与 begin 一样检查返回值
    Blinker.begin("DEVICE_KEY", "SSID", "PASSWORD", power);
}
void loop() {
    Blinker.run();
    uint64_t utcSeconds;
    if (Blinker.time(utcSeconds)) {
        // 使用 UTC 秒；查询本身不发送数据，不建立连接。
    }
}
```

- 默认关闭，QuickStart 不改；CloudSmokeTest 演示开启/读取。未同步、过期或未启用时返回 false 并清空输出。
  普通 WiFi 的 ESP32/R4/Wio composition 共用同一注入点；本批不是 R4/Wio 新真机通过。
  当时 BLE/WiFiBLE/Hub 尚未接线；§11 已接普通 BLE，WiFiBLE/Hub 开启授时仍显式返回 UnsupportedFeature。
- `ITimeSync` 是 Runtime 借用的窄接口，不让 Runtime 依赖具体服务；`TimeSyncService` 组合原 WallClock/协调器，
  只负责来源代际和调度。普通 facade 显式启用才构造一个静态服务，无堆分配、第二网络连接或新编译单元。
  `TimeSyncPeer` 移到共用接口，消除重复合同；没有每 peer 新建时钟或缓冲。
- 复用有效云 MQTT transport、Hello/ID-ready、原帧 scratch 和序号分配。控制/ACK/同步/采样先运行，授时最后调度。
  能力只对安装了服务的认证 Cloud 通道宣告；来源由绑定的 cloud transport 决定，不相信对方 Hello.role。
  一次在途；首次即请求，成功后 5 分钟刷新，RNG/编码失败、不确定发送失败、远端 Error 或 2 秒超时受 30 秒重试期限约束。
  2026-09-13 §19 修正：仅已编码但被本地 TX 队列以 `WouldBlock` 拒收的请求改为 1 秒有界重试，使用新 nonce/RTT；不增加队列或热重试。
  断线取消在途并递增代际，保留 UTC holdover 和重试期限；反复重连不能绕过退避。停止/重启清空时间。
- Broker 只响应当前、未关闭、未撤销/未过期的物理设备 session；virtual child/普通 App Route 不能冒充云源。
  Hello 与设备取 feature 交集，每会话单调时钟限 5 秒一次，超额静默丢弃；没有 reliable replay/outbox/广播。
  nonce/seq 精确回传，错误也受协商帧容量约束；零 seq 或尚无容量时不编造相关错误。成功样本仍走原下行 MQTT。
- 服务器 `Date.now()` 和固定 1,000 ms 来源误差是**业务估计**，不是 NTP 精度证明；普通明文可被网络攻击者篡改。
  没有更新 `IClock.unixTime`、系统 RTC、证书或授权时间，没有把业务授时变成安全依赖。
  本批不增加 Gateway HTTP/表、App 轮询/页面、连接、自动化/History/Bulk。

### 8.2 资源与软件证据

三种编译器目标（ESP32-S3、Arduino M4、Seeed M4）对 Small/Standard/Large 共九组 ABI：

| 项目 | 测量结果 |
| --- | ---: |
| WallClock / 单在途协调器 | 各 40 B |
| 完整可选 TimeSyncService（含前两者、虚表指针、tick/RNG 借用与调度） | 112 B |
| 普通 BlinkerClass | 12 B，原 8 B |
| Small / Standard / Large DeviceRuntime | 400 / 648 / 1,568 B |
| Small / Standard / Large Device | 1,676 / 3,464 / 11,008 B |

Small 默认产品成本为 Runtime 4 B 借用指针 + facade 4 B 指针，不是“默认零 RAM”。
启用服务才增加 112 B 对象，编译器静态初始化 guard 和调用栈另计；不是对象上限即为产品栈峰值。
首次 32 位编译触发旧 Small 1,672 B 静态断言，已明确修订为 1,676 B；不移除尺寸门禁。
Standard/Large 当前实际尺寸被既有布局吸收，上界为原值 +4 B，实际值以上表为准。
独立 ABI 不冒充三平台 Arduino 运行；完整 ZIP/map/真机结果另列。

- 设备 host **61/61**（原 MSVC Debug/ASan）通过，新增真实 DeviceRuntime + TimeSyncService 组合测试：
  Hello/能力/授权、请求与读取、刷新边界、失败退避、related Error、断链 holdover/过期、停止/重启、旧包与跨代际拒绝。
- Broker 类型构建、原 V2-only 及七文件串行专项 **193/193** 通过。包括新增物理授时 7 项与原路由/需求/本地控制/网关生命周期；
  使用隔离临时 SQLite，未写业务测试账号库。此次扩展组比 B3-2 多选一个已有文件，不把全部差额算成新增测试。
  容量门禁补强后，旧测试曾期待未 Hello 时也回 Error；现按未协商容量 0 应丢弃修正预期，并补有容量的错误/资源边界。
- App 原协议回归 **11 文件/156 项**通过；本批 App 生产代码未改，没有新 APK。
- 架构/登录冻结检查通过；v38 只复核 facade 可选授时和 WiFiProduct attachTime 两个变化摘要。
  DeviceKey、登录 proof/session wire、begin 参数、安全时钟与其余冻结项不变。
- 发布 profile 与安装包边界 **11/11** 通过；正式单 ZIP 独立安装后两个普通 WiFi 示例编译、安装字节、provenance 与链接闭包检查通过。

证据位于 ignored `build/b3-cloud-time-20260913/`，不把本地 IP、Key、数据库备份或配置 Sketch 纳入发布。
B3-2 的 cmake/ctest 同名日志在本轮早期重跑时被更新；B3-2 其他 golden/ABI 记录保留，
本轮最终 61 项设备结果只引用新的 B3-3 目录，避免把覆盖后的日志冒充此前 60 项原始记录。

### 8.3 本机部署范围与待验证项

已逐文件对照运行容器并仅部署 Broker shard 1 的 frame/cbor/lane/session-registry/time-sync 五个模块；
变更全部属于 B3-2/3。部署前以 SQLite backup API 保存一致快照与旧镜像/模块，未迁移数据库。
shard 1 恢复 healthy、模块摘要一致，其他 Broker、Gateway、Auth 未替换；同 shard 的三块测试板会短暂重连。
手机原账户在部署后对 COM11 旧静默固件完成一次 Property（247 ms）与真实 State 变化，退出释放观察。
COM11 应用区备份匹配上一批恢复的 WiFiQuickStart；未清 Flash/NVS/根/授权，COM3/COM12 固件保持。

同一真实 profile ZIP SHA-256 为 `96a55cc1fb78c8e991f1a5a7c17814805b05b8d76187a573e40083851b04351e`。
本机 ESP32-S3/3.3.11、与 B1 同一明确配置的 SDK/开发服务 profile；不是新的上游干净 SDK 准入。

| 正式示例 | 程序占用 | 全局 RAM | 链接检查 |
| --- | ---: | ---: | --- |
| WiFiQuickStart（授时关闭） | 942,612 B | 51,296 B | 无 WallClock/TimeSyncCoordinator/TimeSyncService 符号 |
| CloudSmokeTest（授时开启） | 947,800 B | 51,384 B | 实际链接授时服务；无未选 BLE/LAN/Hub 产品 |

相对 B1 同示例：QuickStart 为 +824 B 程序/+16 B 全局 RAM，CloudSmokeTest 为 +3,892 B/+128 B；
这些是完整固件变化（含 facade/runtime 接线、时间打印、静态 guard/链接对齐及 firmware version 标识），
不是把两种不同组件示例相减得出的“纯时钟成本”。结构层的 8 B 指针不能替代固件实际测得的 +16 B。

COM11 只写应用区 `0x10000`，精确 MAC `8c:bf:ea:1b:96:14`、原设备身份/Key 不变：

- 未打开设备观察前已收到可读 UTC，随后日志按 10 秒递增；没有第二连接、App 校时请求或 NTP 注入。
- 原手机账户 Property ACK **238 ms**，真实 State 变化；本示例无 Action，不能把 B1 Action 记作本次 Smoke 新测试。
- 同一账户 MQTT 采样：**7→暂停仍 7→恢复 14→关闭仍 14**，设备计数与 App 14 样本一致；UTC 同时持续递增。
- 普通重启后新获取可读 UTC，未请求采样时计数持续为 0；原 Property/State 再次通过（**200 ms**）。
  首次串口输出前已同步，启动瞬间 false/清空输出的证据仍是 host，不冒称在串口捕获了未同步瞬间。
- 最终已恢复本批同 ZIP 的**静默、授时关闭 WiFiQuickStart**，Property **187 ms**、Action **215 ms**，真实状态/按钮计数递增。
  COM3 原 TLS 固件再次控制并见 State revision 33→34（133 ms）；COM12 原固件未写，数据库只读确认仍有有效 TLS session。
  所有测试观察/采样句柄已释放；COM11 仍 plain，未新建账号或设备/轮换 Key/清授权，未暂存、提交或推送。

保留异常：一次关闭串口读取后的重开段出现 productError=21、云端离线→恢复，第一次采样门禁失败，
当时已有 UTC 仍可读。保持串口连续读取后同一固件/账号的 14 样本门禁通过。
现有 `debug(Print&)`/`printDiagnostics(Print&)` 是同步 Print 路径，多个输出明显集中在重新打开串口时出现；
这是日志背压的线索，不是已排除网络/SDK因素的根因定论，也不能称无串口消费压力下稳定通过。
未通过增加 MQTT 超时、重建账号、放宽授时 TTL 或隐藏重试把失败计为成功。

**下一小包仍收口 B3-3 边界**：先对日志输出/串口背压做最小对照，再完成可控的短离线 holdover/恢复；
最大年龄过期和 5 分钟刷新当前有软件边界测试，真实长期龄段不冒称已测。
完成这条边界后再 B3-4 Control 手机 Direct/LAN/网关来源，再 B4；不重复建号、轮换 Key 或百轮。

## 9. B3-3b：日志对照与受控离线（2026-09-13，本批执行）

发现现有 CloudSmokeTest 每 10 秒调用同步 `printDiagnostics(Print&)`，与诊断 §9.6 的使用边界矛盾。
修订顺序固定为：

1. COM11 原应用精确备份，用上一批 periodic Print 固件做有限无串口消费/恢复对照；保留失败，不扩大 ACK/keepalive 超时。
2. 正式 CloudSmokeTest 删除自动周期输出/默认 debug/旧定时变量，仅串口显式 `d` 请求一次诊断与 UTC。
   通用 Print 的同步限制仍存在，不能把显式调用包装成已经非阻塞；不改串口 timeout、不换 USB driver、不加后台日志队列。
3. 同一 ZIP 编译修订示例，并用测试专用 IWifiStation 包装注入单设备短离线。测试接口只在 tests/internal，
   复用原 WiFi 产品/真实网络、ITimeSync 与业务核心；不改正常产品增加故障开关、不停共享 Broker、不清授权。
4. 原手机账户验证控制/采样；测试时间源尚无样本、首次同步、离线 holdover、恢复、正常刷新分别记录。
   只有真实测到的边界才关闭，默认一小时过期不由缩短策略/加速 tick 假冒；原 host 过期边界独立保留。

本批按此顺序执行；结果及边界见下面各节，下一步 B3-4 见 §10，不再重复 B3-3 的 Broker 部署。

### 9.1 同步输出对照（已完成）

COM11 原应用精确备份后，只写应用区；原身份/Key 与 SDK/profile 不变。上一批周期输出固件在持续串口读取时
Property ACK 237 ms；关闭读取后的控制失败，重开日志见 cloud=0/error=21 后恢复；重新建立观察后 ACK 96 ms。
原观察有 120 秒自动关闭上限，超过该期限的调用不算设备网络故障，也不把恢复重试冒充原操作成功。
本机 HWCDC 默认 tx timeout 为 100 ms，其 write 可连续等待 20 个 timeout；多次同步 Print 会叠加等待。
这份 SDK 含本地实现差异，本对照不宣称所有上游版本都有同一缺陷，也没有测得每次 Print 的精确阻塞时间。

修订后的正式 CloudSmokeTest 取消默认 debug、周期诊断和启动时主动 Print；串口显式发送 `d` 才输出一次诊断/UTC，
请求时必须保持读取。没有新队列/线程/timeout 设置，不承诺任意 Print 调用非阻塞。
单 ZIP 安装字节、编译来源与链接闭包通过；ESP32-S3 程序 946,400 B、全局 RAM 51,384 B，
对旧例程减少 1,400 B 程序、RAM 不变；核心库本批未改。
ZIP SHA-256 `b39528d90fc51183225beb26fbdd61d854d8eb38ed89213aaabe523d391515eb`。

修订例程在显式读取后关闭串口 45 秒，原手机账户同一 MQTT 连续控制 ACK 243/157/171 ms，真实 State revision 2→3→4。
随后继续无串口读取完成采样 7→暂停仍 7→恢复 14→关闭仍 14；按需诊断为 active/cloud=1、productError=0、
telemetrySamples=14，UTC 有效。该有限对照收口示例默认使用边界，不代替长稳或 SDK 非阻塞 logger 准入。
证据为 ignored `build/b3-boundary-20260913/`；旧固件失败日志保留，不用新结果覆盖。

### 9.2 单设备真实离线、启动与正常刷新（已完成）

内部 `tests/internal/Esp32BusinessTimeProbe/` 使用 Advanced 入口，复用正式 ZIP 中的原普通 WiFi 产品、
IWifiStation 和 TimeSyncService。测试包装只断开一块板的真实 radio，并在 30 秒后允许原连接状态机恢复；
观察器只记录实际请求/已接受 TimeResponse，没有伪造 UTC、缩短授时年龄或增大 RTT 窗口。它不进入 Arduino 发布包。

COM11 同次启动结果：

- tick 1,839 ms 捕获 UTC=0/valid=0，bootUnsynced=1；首次有效样本在 1,943 ms，之后查询可读。
- 30 秒离线窗口内，六个快照覆盖约 25.2 秒，均 radio=0/cloud=0；UTC 按 tick 递增，requests=samples=1，锚点仍是首次样本。
- tick 62,845 ms 已恢复 radio=1/cloud=1；UTC 未清零，仍只有首次样本，不因重连绕过刷新调度。原手机账户 Property ACK 246 ms。
- 第二个有效样本在 tick 301,961 ms，距首次 **300,018 ms**；requests=samples=2，真实完成默认五分钟刷新。
- 故障窗口内 COM3 原 TLS MQTT 见证 ACK 154 ms，State revision 34→35；同账户 MQTT 保持，未动共享 Broker/其他开发板。

探针完整程序 942,872 B、全局 RAM 51,360 B，含测试包装/计数，不用与正式示例差值推算纯授时成本。
单次 product.run 记录峰值 289,967 μs，包含原连接/登录路径；不是硬实时保证或授时模块耗时。
默认一小时过期仍只有 host 软件证据，R4/Wio 当前真机授时仍待现场；不把本次 ESP32 短门禁扩称全部发布矩阵完成。
可复核记录及断言在本批 ignored 目录：serial 时间快照、holdover-result.json、同账户 App ACK、两个 verified 固件记录。
首次内部探针因误用普通入口被架构检查拒绝，已改 Advanced 并重新编译、验证源码仅三项凭据替换后才烧录；未放宽门禁。

### 9.3 验证收口与现场恢复

本批 host **61/61**（原 MSVC Debug/ASan）、架构/登录冻结、发布 profile 与安装包 **11/11** 通过。
修订正式示例与内部探针均从同一 ZIP 独立安装编译，字节/provenance/链接闭包与仅三项凭据替换检查通过；
未选 BLE/LAN/Hub 未留在 WiFi 链接中。App/Service 本批只有接力文档更新，没有重新部署/安装或修改其生产代码。

COM11 最终已恢复原静默、授时关闭 WiFiQuickStart，应用 SHA-256
`84f124dbedcc9fbe4c3fc290e661b80b52d3ef4e03b59f8f02fabbf19dc8e485`；
恢复后真实手机 Property ACK **130 ms**、Action ACK **186 ms**，State revision 1→2→3，按钮计数 0→1。
所有本批测试观察/采样句柄已关闭；未擦 NVS/Key/授权、未写 COM3/COM12 固件，未暂存/提交/推送。
串口脚本初次因命名 serial.py 遮蔽依赖而失败，修正为 read_port.py 后才计入有读者证据；
配置 Sketch 字节检查也显式改 UTF-8，避免本机 GBK 误解码中文 SSID。失败记录不当作有效测量。

本批短边界已收口，下一包进入 B3-4。默认一小时硬件过期、R4/Wio 当前授时硬件矩阵及平台非阻塞日志能力仍明确单列，
不要求重做百轮、共享 Broker 部署或基础 WiFi 注册。

## 10. B3-4 接线审计与后续顺序

§9 的单设备短离线/恢复已完成；下一包按下面顺序推进，不一次打开三条未实现来源：

1. **先普通 BLE 的 Control 手机来源（§11 已接线，真机待验）。** 审计时 DeviceRuntime 的 timePeer/Hello 只允许 Cloud，
   BleProduct 尚未 attachTime；本批已修订。复用 ITimeSync 和单一时钟，在运行时选择已经认证且具有 Control 的 peer；
   来源来自绑定通道，不信 Hello.role 或来包自声明。无在途/合法 UTC holdover 与失效不能混为一种状态。
2. **App 共用 DirectDeviceSession responder（§11 已实现 BLE 接线）。** BLE/LAN 共用该 BBP 层；审计时 receive 会把未经 pending 匹配的
   TimeRequest 当错误，因此不能只改 Hello feature。现已增加有界、无可靠重试的请求处理，保留原业务 pending/ACK；
   使用同一个发送通道及其容量/取消，不能新增 socket/无限发送队列。回应前取系统 UTC，限流用单调时间。
   direct-hello 的已知位 mask 也须同步更新。**Hello 能力与授权分开**：BLE 的 Hello 在 Method 2 完成之前，
   bit 24 只能表示已安装 responder/接收能力，不能当 Control 证明；实际回应与选源必须等认证完成且具 Control。
   不为获授权限另造第二次 Hello；Observer 即使识别能力位也不能校时。普通账户 APP_FEATURES 不默认扩大。
3. **权限必须传递自完成的授权。** BLE 已校验 challenge.permissions；LAN 当前 LocalAccessClient.open 只返回 void，
   session 没有获授权限视图。接线时通过最小只读授权结果传递，不从 UI 的角色、请求权限或“已连上 socket”推断 Control；
   Observer 不回应，过期/撤权/关闭时取消。设备端仍是最终权限边界，App 拒绝不能替代设备检查。
4. **再复用到 LAN、混合产品与网关。** 保持一设备一业务时钟，云源优先、无可用云源再本地 Control，
   切换取消旧在途而保留有效 holdover；旧来源/代际回应不得覆盖新选择。网关回应须读取自身有效 UTC 与增长后的误差，
   不能直接转用其安全 IClock/SNTP 标志。回应完成前不发布网关/子设备 feature，不增加每子设备时钟或 NTP。
5. 软件必须覆盖 Observer/撤权、未知位/未协商、帧容量/seq/nonce、同时业务 ACK、并行来源和来源切换；
   默认不启用产品的链接/ABI 门禁保持。再按现场做 BLE→LAN 短真机，不重开百轮、History/Bulk 或新登录方案。

本节是原接线清单；已经完成的范围和剩余入口以 §11 为准。共用 responder 已实现不等于 LAN/混合/网关均已安装。
B3-4 完成后回 B4 LAN 产品化，不继续扩展时区/日历/History/Bulk。

## 11. B3-4a：普通 BLE Control 手机业务授时（2026-09-13）

本节保留首批软件接线及当时的未通过记录；后续安装、真机与标准编译补证以 §12 为准，不重复执行已完成步骤。

### 11.1 模块职责与实际接线

- 普通 BLE 的 PlatformBleOnlyComposition 增加同 WiFi 一样的 attachTime 注入，借用原 ArduinoClock/安全 RNG。
  Sketch 仍是 `Blinker.enableTime()` 后调用原 begin，再查询 `Blinker.time(uint64_t&)`；默认 QuickStart 关闭。
  不增加 begin 参数、每 MCU 新 API、每 peer 时钟、Flash UTC 或第二条连接。Wio BLE 仍是 preview。
- DeviceRuntime 只选择已 Hello/ID-ready 且实际具有 Control 的通道；有效云源优先，无云源才用本地 Control。
  本地来源必须绑定已有 transport 授权或 in-band authorization provider。Hello.role、MAC、页面角色不是授权依据。
  `Controller` 是本地控制来源类别，不证明它一定是手机；网关回应尚未安装，不能因此提前声称 Gateway 来源实现。
- ITimeSync.poll 接收当前选中 peer，来源/授权消失时取消旧在途；接收 TimeResponse 前也复核选择，
  防止云源刚上线时旧本地回应抢先覆盖。sessionClosed 只取消精确 transport/session 的在途请求，
  另一连接断开不影响当前请求；取消保留 UTC holdover 和既有重试期限。沿用单服务 generation/随机 nonce，
  不保存新的每 peer 代际表；同 sessionId 重连不能复活旧授权/旧在途。
- App DirectTimeResponder 位于共用 protocol/device-v2 层，BLE 在完成原 Method 2、验证设备 proof/permissions 后安装。
  复用 DirectSecure BBP 通道，回应精确 sequence/nonce，不占用原 App 业务 pending，不另建可靠重试/后台 timer/socket。
  时间取系统 UTC；单调时钟每会话 5 秒至多一答，成功与错误均计额度。系统 UTC 无效时回关联 Error，不发送 0。
- Hello 在 Method 2 之前，bit 24 只表示已安装协议能力；设备选源和 App 回应仍必须有 Control。
  Observer 不校时，不补第二次 Hello。未安装/未协商/协商容量不足 40 B 不发起或回应；旧设备/旧 App 的原控制路径不受影响。
  APP_FEATURES 和普通 App Route 白名单不变；共享 Direct 类默认不安装 responder，LAN 仍未接获授权限与 feature。
- BLE 发帧保留原串行链，仅允许“正在发 + 等待”共 2 项，容量满拒绝新项，不破坏已接受项。
  关闭立即清会话并触发原 native disconnect，不再排在挂住的加密/发送后面；异步加密完成后检查关闭状态，不发迟到帧。
  Native 释放失败仍如实报错，不冒充已释放。Direct receive 返回后也复核关闭，避免迟到回应或状态更新。

所有时间仍是业务 UTC。手机时间可能不准；没有修改 IClock.unixTime、系统 RTC、证书、授权过期或安全租约。
App 固定 1,000 ms 的来源误差是业务估计预算，不是手机时钟校准证明；设备还叠加实测 RTT/2 与 holdover 漂移。

### 11.2 软件与资源门禁

- 设备 host **61/61**（既有 MSVC Debug/ASan）通过。新增到原 Runtime target 的断言覆盖：
  Hello 能力不等于权限、in-band provider 的 Observer/Control（非密码学替身验收）、39 B 帧上限、
  未协商、撤权/断链/同 ID 重连、无关通道断链不误取消、云源优先及旧来源竞争回应；旧刷新/超时/nonce/重启测试保持。
  超时回应被丢弃而不回 Error，不能因可选授时失败制造回应循环或误伤 App 业务 pending。
- App 共用协议、BLE Direct、LocalAccess、EdgeGateway 回归 **36 文件 / 411 项**通过；app/spec TypeScript 检查与 local Angular 完整 bundle 构建通过。
  覆盖 40 B 响应、单调限流/UTC 回拨、无效 UTC 关联 Error、坏帧、Hello 新位/未知位、业务 ACK 同序号并行、
  Observer 不回应、关闭后的迟到接收、发送容量上限及挂起加密期间关闭取消。没有修改无关页面或测试超时。
  两个既有 Angular API 测试需要 JIT compiler setup；本批用 ignored 配置显式加载 @angular/compiler 后全组重跑通过，
  初次 404 项通过但 2 个 suite 导入失败的运行不记为全量通过。
- 架构/登录冻结、发布 profile 检查与发布包 **11/11** 通过；未新增每平台产品包装或保留旧 poll 接口做兼容分支。
- ESP32-S3 / Arduino M4 / Seeed M4，三档资源配置共 **9 项 ABI**：WallClock 40 B、协调器 40 B、完整服务 112 B；
  facade 12 B，Runtime 为 400/648/1,568 B，Device 为 1,676/3,464/11,008 B，均与 B3-3 相同。
  本次没有新常驻设备字段；此前默认两个指针合计 8 B 的成本仍存在，不能称默认零 RAM。
- 新 development-profile ZIP SHA-256 `6589c5795086b3a386af3800e64540d344e2bc3d406765b6495e4dd0930e311a`。
  ESP32-S3 独立 sketchbook 的正式 BleQuickStart 与仅增加 enableTime 的探针均编译、字节/provenance/链接闭包通过。
  默认：程序 693,072 B / 全局 RAM 34,864 B；开启：696,044 B / 34,984 B，即 **+2,972 B / +120 B**。
  默认 ELF 无 WallClock/协调器/服务实现；开启才保留。120 B 含服务及静态初始化成本，不是运行时栈峰值。
  纯 BLE 链接不拉入 MQTT/WiFi 产品、LAN 或 Hub。

**UNO R4 安装编译未完成，不能列为通过。** 独立 sketchbook 已安装同一 ZIP；首轮完整依赖扫描后进入编译，
但触及测试脚本 15 分钟预算（exit=null，无源码 error）。保留 cold-timeout 原日志。
随后尝试 CLI 的 skip-libraries-discovery，虽能编译 Sketch/core，但实际链接命令缺少 BlinkerDeviceV2.a、ArduinoBLE、Crypto，
出现对应 undefined reference。该构建路径无效，已撤回该参数，不能靠手工补链接冒充标准 Arduino 安装门禁。
最初复制依赖时也已修正 Preferences 是 R4 SDK 自带库，不要求用户另装。
后续 `node build/b3-direct-time-20260913/r4-resume.cjs` 使用标准 CLI、现有同包 sketchbook、30 分钟**本机构建**预算，
默认/开启分别保存真实编译来源与 ELF/map；两项成功后运行 `C:/Python313/python.exe build/b3-direct-time-20260913/verify.py r4`。
本批止于以上未通过记录，没有用调大设备通信超时或先前 R4 二进制充数；当前无遗留编译进程。

证据目录：设备 ignored `build/b3-direct-time-20260913/`，App 同名 ignored build 目录。
本批未刷板、清授权、安装 App、部署 Broker/Service、迁移数据库或提交/推送；当前手机/板上仍是上一批固件。
上述软件/编译结果不是 BLE 真机自动授时已通过。

### 11.3 下一批固定顺序

1. 先补 R4 的标准 CLI 默认/开启对照，不引入新依赖或 skip-discovery 快捷路径；与 ESP32 的匹配 App/纯 BLE
   有限真机准备可并行，不能把未验证的 R4 固件烧录或把该平台标为已过。真机前精确核对板角色与身份、保留恢复固件；
   Control 会话中首次授时与 Property/Action/State 同时工作，退出断链后 holdover、重连旧样本不可复活、Observer 不授时。
   不把全部 WiFi 见证板一起改成 BLE，不为测试重建账户或改变已有安全时钟；仅按真实证据关闭门禁。
2. B3-4b：LocalAccessClient.open 返回最小只读“实际获授权限/协商能力”，共用 responder 从该结果安装，
   而非 UI 请求角色或 socket 可达性。完成 LAN Plain/可选加密、Observer/撤权/关闭及云优先边界，再短真机。
3. B3-4c：WiFiBLE/Hub 复用同一时钟注入；网关回应从自身有效业务快照读取 UTC/增长误差，
   无有效样本则 Error，不从安全 SNTP/IClock 偷取“可信样本”，不加每子设备 NTP 或新会话。
4. 收口以上适配后回 B4 LAN 产品化。默认一小时硬件过期、R4/Wio 当前硬件矩阵及 Wio BLE 发布限制继续明确单列。

## 12. B3-4a 真机补证与 B3-4b LAN 接线（2026-09-13）

本节优先于 §11 的“未安装/未刷板/LAN 尚未接线”。只推进原业务 UTC，不改安全 IClock、账户 MQTT、登录/授权协议。

### 12.1 ESP32-S3 + 真实 Android App

只对 COM11 的应用区操作，刷写前核对 MAC/实例、精确备份原静默 WiFi 固件；COM3/COM12 不刷写。
使用 §11 同一 ZIP 的独立安装源码；测试 Sketch 来自正式 BleQuickStart，仅增加 enableTime、启动未同步断言及串口 `d` 按需状态，
没有循环日志、默认调试输出、NTP 或秘密输出。程序 696,988 B / 全局 RAM 34,992 B；诊断版不是 §11 的纯开启增量基准。
固件 SHA-256 `ebb47a89c252d549c05b509b1cc253f62ac8f34987d018964d5c4ee7b020b9b3`，安装字节、来源与纯 BLE 链接闭包已验。

匹配 B3-4a APK 保留账户/数据更新，SHA-256 `64389f0da5664b999be57f2a30fb7bab515de9da80c8056d73710edebabbcc51`，
实际 APK assets 已核验包含 responder。此 APK 的 BLE 结果不能替代下文后续 LAN 源码的安装/真机验收。
COM11 的 BLE 访问根已在 B2b-13 明确清空，本批未再清根；经真实 App 引导页完成一次新绑定，
得到 `ble_LhR-mYNVmrHJnnavXGVd9A`。只用 MAC 限定台架上的目标，绑定仍验证真实 DeviceInstanceId/Noise/receipt/Method 2，
没有把 MAC 改成生产身份，也没有复活已退休 ID、伪造 grant 或重建账户。

| 门禁 | 本次证据 |
| --- | --- |
| 启动/首次授时 | 连接前 3 次 valid=0、UTC=0；真实 Control 会话后 valid=1。请求/回应为完整 BBP 28/40 B、sequence 一致 |
| 业务共存 | Property ACK 338 ms，button Action 262 ms；State 显示 power=true、buttonPresses=1 |
| 断开后保持 | 主动关闭真实 Direct session 并退出引导页；7 个串口样本跨 30,328 ms，UTC 前进 30 秒，Action 计数保持 1 |
| 同根重连 | 未重新签发 enrollment；从发现完成起至 Direct ready 为 2,652 ms；未提前再授时，Property 324 ms，Action 未重复 |
| 普通重启 | 未擦 NVS；再次连接前 valid=0，原绑定重连后新授时 valid=1；ready 2,915 ms，Property 306 ms，Action 计数 0 |
| 原账户 MQTT | 同一账户 session 保持；COM3 两次 Property ACK 99/142 ms，真实状态从 true 变 false |
| 恢复 | COM11 恢复精确原 WiFi 应用 SHA `84f124dbedcc9fbe4c3fc290e661b80b52d3ef4e03b59f8f02fabbf19dc8e485`；原 Key/ID 登录，Property/Action 135/195 ms |

表中的重连 ready 不含之前的扫描阶段；串口 UTC 是秒级公开 API，不据此声称亚秒校准精度。
台架 CDP 只观测真实协议元数据、调用既有业务接口，授时样本仍来自生产 responder 的手机系统 UTC。
本次退出验证是显式关闭原 Direct 会话/回到设备列表，不替代完整页面路由 UI、杀进程或所有后台状态的验收。
测试结束已关闭 BLE/云端观察 owner、恢复 App 客户端工厂；BLE 测试记录保留，当前板运行原 WiFi 固件，不能将该卡离线当成授时失败。
当前已证明真实 Control 短闭环；**Observer 真机、恶意旧 nonce 真机、默认一小时过期和 R4/Wio 当前硬件授时仍未通过**，
分别沿用软件拒绝证据或待验项，不能将普通重连误写成主动重放攻击已测。
证据：ignored `build/b3-direct-live-20260913/`，`verify-live.cjs` 从 App/串口原始结果校验生成 `verified-live.json`。

### 12.2 B3-4b：LAN 只传递已获授权限，复用 responder

- `LocalAccessClient.open()` 从原 `Promise<void>` 收口为最小只读 `LocalAccessAuthorization { permissions }`；
  数值来自原 HTTP 已验证的 exact grant，在清理临时秘密前复制，**收到并验证 recipient proof、复核取消/期限后**才返回冻结快照。
  不返回 sessionKey/exactGrant，不新增持久凭据、当前权限全局 getter 或第二份授权策略；快照不能用于重开已关闭连接。
  原 HTTP 对 grant 与请求逐字段相符的检查未放宽，不是在服务端新增“请求 Control 时自动降为 Observe”的协议。
- `openLocalDeviceSession()` 仍使用同一个 Plain/Noise 策略、原 WebSocket 和 BBP Hello。由认证结果 + 设备 Hello 的协商能力/容量
  创建共用 DirectTimeResponder，不从页面请求 permissions、角色或 IP/发现可达性推断 Control。
  Hello 宣告已安装 bit 24，实际 Observe 不回应；旧 peer 未宣告仍能同步 Manifest/State/业务。
- 复用两项有界串行 writer 与原请求关联；时间回应不会消费同 sequence 的业务 ACK，不加独立 timer、socket、重试队列或每 peer 时钟。
  关闭/撤权 owner 的迟到读不能发 TimeResponse；Plain 和 Noise 的原 proof、账户代际、到期及秘密清除边界保持。
  设备端云源优先仍由原 DeviceRuntime 执行，本批没有更改设备代码/存储或 Service/Broker。
- 专项 **45/45**；扩展回归 **36 文件 / 418 项**，app/spec TypeScript 与完整 local Angular 构建通过。
  新用例覆盖实际权限而非请求值、proof 完成前不发布、冻结快照、Observe/旧 peer 不回应、40 B/nonce/sequence、
  与同号业务 ACK 并行及取消后的迟到读取。真实 Plain HMAC/golden 原回归保持；mock HTTP/Noise 边界不冒充真实签发/密码学验收。
  全组命令需沿用既有 `--globals` 和 Angular JIT setup；首次漏 globals 导致 10 个旧 suite 未加载的运行不计通过。
  四个本批 App 源码/测试文件 ESLint 通过；设备 host 61/61、架构与 release-profile 检查本批重跑通过，无设备常驻字段增长。

本批手机安装的是上述 BLE 验证 APK；LAN 新接线仅源代码/测试/Angular bundle 验证，**未安装 LAN 修订 APK、未刷 LAN+time 固件或通过 LAN 授时真机**。
原 App LAN 源码开关仍关闭；不能将这次接线称为 B4 全平台 LAN 已产品化。

### 12.3 UNO R4 标准安装编译与下一步

同 §11 ZIP、独立 sketchbook、Renesas UNO 1.5.3，标准 CLI（无 skip-libraries-discovery）已完成正式 BleQuickStart：
程序 194,460 B / 全局 RAM 16,964 B；开启授时的 BleTimeProbe 为 **197,052 B / 17,080 B**，即 **+2,592 B / +116 B**。
两份产物的 ZIP 安装字节、真实编译来源、map/ELF 链接闭包全部通过；默认无 WallClock/协调器/服务实现，开启才保留，
不拉入 WiFi product、MQTT、LAN 或 Hub。ArduinoBLE 所需的 SDK WiFiS3 串口协处理器依赖不是第二个 Blinker WiFi product。
116 B 包含服务和静态初始化成本，不是 BLE 运行时堆/栈峰值。没有刷写 R4，不能替代该平台真机。
证据：`build/b3-direct-time-20260913/verified-r4.json` 及两个 `r4/evidence-*`；旧超时/无效 skip-discovery 日志保留，不能回写成通过。
默认/开启标准 CLI 分别约 21.5/22.6 分钟，本机预算内完成；复用同一 build-path 不代表 CLI 保留了不同 Sketch 的缓存。
验证结果将旧 runner 的 `reusedBuildCache` 元数据纠正为 `sharedBuildPath`，没有修改原编译输出、补链接或扩大 MCU 通信超时。
R4 本轮编译进程已结束，不再把此对照列为下一轮未完成任务。

下一步固定为：B3-4b 匹配 APK/单普通 WiFi LAN 授时短真机 → B3-4c 混合/Hub同一时钟 → B4。
LAN 真机应先保持两个 WiFi 见证与原账户 MQTT，验证有云源时不被手机抢写；在真实有效 LAN grant 内暂时失去设备云源后，
才验证 Control 手机回应、holdover/云源恢复、Observe 不授时及关闭。不能伪造权限、改安全时间或停整个共享 Broker 来制造成功。
来源切换保留现有 5 分钟刷新/30 秒失败重试期限，不意味着断云后立刻增加请求；须记录真实到期窗口，不能将健康 holdover 误报成 LAN 授时失败。
Observer BLE/硬件过期及平台矩阵仍按上方待验项补证，不重开百轮、History/Bulk、额外账户连接或新授时协议。

## 13. B3-4b LAN 授时真机（2026-09-13）

本节替代 §12 中“LAN 修订未安装/未真机”的状态；**仅关闭普通 ESP32 WiFi + LAN Plain 的本批短闭环**，
不等于 B3-4 全部结束、所有加密/平台已验或 B4 产品化完成。

### 13.1 实现和可复现边界

- 新增内部 `tests/internal/Esp32LanTimeProbe` 及操作说明。继承现有平台并借用真实 MQTT Client，
  只包装 `connect/stop` 以暂停该板 MQTT TCP 60 秒、自动恢复；不关闭 WiFi、不停共享 Broker、不创建第二条 TCP。
  使用原 WiFi product、LAN extension、ITimeSync/TimeSyncService 和组件核心，不给正式库增加故障开关、每通道时钟或默认日志。
- `s` 按需输出有界状态；`p/r` 暂停/恢复 MQTT；`c` 只能在暂停且断云时每次启动使用一次，清除测试用**业务时间锚点**。
  `c` 不用于自然刷新门禁，不改安全 IClock/授权/系统 RTC。真实 WiFi/Key 只放 ignored 生成 Sketch。
- 使用真实 development profile 单 ZIP，独立 sketchbook 标准 CLI 安装编译，ESP32 Arduino 3.3.11；
  库全局打开 internal LocalAccess/LAN Plain。程序 **1,031,300 B**、全局 RAM **56,720 B**，
  此数包含 LAN 和探针，不是仅启用授时的增量，更不是堆/任务栈峰值。
  ZIP 安装字节、编译来源、LAN/业务时钟链接与 APK assets 接线核验通过。
- ZIP SHA-256 `cb908f6ada8f4b4f1c54d1f84a5c581a682f56fa94b03186fdf2d74f45873b62`；
  固件 `fdd268322ededda49440878cd13d80bf4079d3001c93a225d7a435a4acd87d68`；
  APK `42b7027cd8a0d679e5d05ef3c732e77c1caa2bea41a61c53b7a9f27048b1f892`。
  APK 保留账户/数据更新，只在生成的 debug 环境启用 LAN；源码默认 LAN 开关保持关闭。
- App 台架借用生产 LocalAccessService/HTTP exact grant/recipient proof/Hello/DirectDeviceSession，
  只观测报文长度、类型、序号与 nonce 是否相符；不导出或改写 grant/PSK，不伪造 UTC，不回退 Cloud 代替 LAN 命令。
  Observer 仅将 HTTP **请求权限**收窄为 1，响应仍经过原 exact grant 和 proof 校验；不是降格已经签发的 Control 凭据。

只刷 COM11 已核验实例的应用区，保留原 Key/设备 ID/NVS/访问根；COM3/COM12 不刷写。
手机和 MCU 位于同一路由器的 5 GHz/2.4 GHz LAN，电脑地址 192.168.2.76；没有修改防火墙、部署服务或迁移数据库。

### 13.2 真实结果及不能省略的限制

| 门禁 | 证据 |
| --- | --- |
| 启动/原云源 | 启动 526 ms：valid=0/UTC=0；2,068 ms 接受首个 Server 样本，原身份 MQTT 在线 |
| 加入 LAN | 首个样本约 250 秒后取得 Control；鉴权、Manifest/State ready 1,484 ms；未提前增加授时请求 |
| 自然到期切换 | 约 290 秒仅暂停 MQTT，WiFi 保持在线、UTC 正常前进；原样本后 **300,173 ms** 接受 Controller 样本，未清锚点或缩短刷新周期 |
| 实际 wire | TimeRequest **28 B**、TimeResponse **40 B**；sequence=6 相同、nonce 匹配、回应 flags=2；生产 responder 取手机系统 UTC |
| LAN 业务并行 | Property ACK **168 ms**、button Action **84 ms**；真实 State 为 power=true、buttonPresses=1，无 Action 重试 |
| 云端恢复 | 同 Key/ID 恢复 MQTT，没有立即再授时；后续又在本地样本后 **300,019 ms** 接受 Server 样本，仍使用原五分钟间隔 |
| 独立见证 | COM11 暂停期间，COM3 经原账户 MQTT 的 Property ACK **153 ms**、State 真实变化；账户 session 不更换 |
| Observer | 真实权限 1，Manifest/State ready **1,274 ms**；断云并显式清业务锚点后 **12,148 ms** 内 valid=0/UTC=0，requests/samples 不增，WiFi 在线 |
| Observer 控制拒绝 | 单次 Property 写收到 wire **AuthenticationRequired=2**，power/Action 计数不变；没有 TimeRequest/TimeResponse |
| 失去锚点后恢复 | 恢复原 MQTT 后接受新 Server 样本，valid=1；只读连接最终关闭、原生 slot 释放 |
| 关闭/还原 | Control 显式关闭完成 **25 ms**；精确恢复原静默 WiFi 固件，原 Key/ID 的 Property **201 ms**、Action **141 ms**，真实 State 通过 |

**Observer 首次握手失败的记录保留。** 在 Control 连接关闭后立刻请求另一权限作用域，新握手返回 `LOCAL_WS_RECEIVE_FAILED`。
源码检查表明同 caller 但不同权限产生另一 authorization，设备单槽 ledger 保留原授权至到期及 15 秒 challenge 墓碑窗口；
此行为与 LAN 方案 §31.3 的既有范围限制一致。等待原窗口结束后，未清授权/重启/改 Key，Observer 成功。
本次没有采到 MCU 的精确 CapacityExceeded 错误码，不能将上述推断写成该错误码的串口实测。
**不同 scope/新 caller 不能立即替换旧单槽授权，仍是 B4 产品化待处理项**；没有通过扩槽、清 ledger 或放宽 proof 掩盖。

Observer 的零时间来自显式锚点丢失注入，不是自然一小时过期。这里证明真实 Observer 未获选为时间源、不能写控制，
不等于恶意设备主动向 Observer 注入 TimeRequest 的真机测试，也不替代真实分享/ACL 撤销。
Observer 最终原生原因是 `LOCAL_WS_CLOSED`，不是此次主动 close 的耗时样本；不据此新增“精确到期关闭”门禁通过声明。
Control 关闭是借用真实 session 的服务级验收，不是完整页面导航/杀进程 UI 验收。
同 Control/Cloud 同时可选且恰好到期时的云优先仍由 host 覆盖，本次不冒充该竞争场景的硬件证据。

恢复固件 SHA `84f124dbedcc9fbe4c3fc290e661b80b52d3ef4e03b59f8f02fabbf19dc8e485`；
最终 App 工厂已还原、LAN/见证 owner 关闭并移除、原账户 MQTT ready，保留原数据和本批匹配 APK。
证据：ignored `build/b3-lan-time-20260913/`；`verify_live.cjs` 校验真实 App/串口/刷写/恢复原始记录，生成 `verified-live.json`。

本批新增 host 自然“云→本地→云”刷新期限测试，合并重复应答构造；MSVC Debug/ASan **61/61**、架构与 release-profile 检查通过。
探针首编用了错误的 Action 回调名称，改为既有 `onInvoke` 后标准 CLI 重编通过；旧日志保留，不修改公开 API。
host 首次运行缺 ASan DLL 搜索路径；补本进程 PATH 后同一新二进制专项及全组通过，不改通信超时。
App 生产源码沿用 §12 的已验接线，本批重新构建/安装/真机，不重复声明一轮新的 418 项执行。

### 13.3 下一包：B3-4c 按所有权拆分，不把授时塞进 BLE/GATT 驱动

1. **先接混合/Hub 自身。** `Esp32WifiBleProduct` 和 `Esp32EdgeHubProduct` 复用已有 stack 的 tick/RNG/Client，
   通过原 `attachTime` 注入同一个可选服务。默认关闭、begin 形式不变；默认/开启独立安装编译和链接对照。
   此步不自动等于子设备 responder 已安装，不提前宣告后者能力。
2. **再接业务快照读取。** 网关 responder 借用最小只读快照（有效 UTC、当前增长误差），由唯一业务服务提供；
   不调用安全 SNTP/IClock 充当业务源，不暴露或另存一份可写 WallClock，不增加每子设备 NTP/授时请求器。
3. **最后接南向协议分发。** 在唯一 `GatewayChildExecutor` 的 BBP 分发边界处理子设备 TimeRequest，
   复用原安全会话、限速和有界发送；不能仅加在 Cloud RouteBridge 中，否则未来 LAN 等上游会绕过或重复处理。
   回应保留请求 sequence/nonce，不占用普通命令关联；实际 Control/协商/有效样本/会话代际全部满足才回应，
   无样本关联 Error，关闭/撤权后迟到工作不得发送；直到安装完成才宣告子设备授时能力。
4. **软件、资源、短真机后回 B4。** 覆盖同序号业务 ACK、无样本/到期、权限、会话关闭、跨来源与 SDK 默认不链接；
   不扩网关规模/百轮/History/Bulk。B4 再处理通用 LAN 注入/发布默认/平台，以及上文不同 scope 的单槽接替。

加密 LAN 授时真机、Observer BLE/主动旧 nonce、一小时过期、R4/Wio 当前硬件矩阵仍单列，不能由本批 Plain ESP32 结果代替。

## 14. B3-4c-1/2：混合与 Hub 自身、共享快照和回应策略（2026-09-13）

本节接续 §13.3。**自身授时接线、只读快照和 responder 策略核心已实现；南向 executor 尚未接线，不能宣布网关对子设备授时可用。**
不改 WiFi/BLE/组件初始化方式，不新增产品入口、连接、NTP 或安全时钟，不扩网关规模。

### 14.1 本批实现与解耦边界

- `Esp32WifiBleProduct`、`Esp32EdgeHubProduct` 增加原有 `attachTime` 重载，借用各自 stack 的 tick/RNG/Client。
  混合设备仍在原 `begin(...)` 前 `Blinker.enableTime()`；Hub 仍为内部 Product 显式注入，不推出新的公开 facade。
  两个 composition 不新增成员字段。此处只启用 **物理设备自身** Runtime 的协商/请求；Hub 安全 SNTP 不作业务样本。
- 新增最小 `IWallTimeSource::readTime(WallClockSnapshot&)`，`ITimeSync` 单继承该接口；
  `TimeSyncService` 从唯一 WallClock 读取当前 UTC/增长误差，并在读取时推进过期状态。
  原 UTC 秒 helper 从此快照导出，不保留另一套读取实现；移除旧虚拟 `time`，两个内部探针同步改用 `readTime`。
  普通 `Blinker.time(uint64_t&)` API 不变。回应方拿不到 synchronize/reset 或安全 IClock。
- 新增独立 `TimeSyncResponder` 策略，仅借用只读 source，每安全会话可有一个 12 B 限频状态；不持有帧缓存、连接、队列或另一份时钟。
  调用方须提供 **当前真实授权/非零 session 与 generation、已协商 features，以及不大于 negotiated max 的输出 span**。
  必须 Control + FeatureTimeSync；有效 TimeRequest flags=0、非零 sequence、16 B 非零 nonce；至少 40 B 输出预算。
  生成 TimeResponse 原样回显 nonce/sequence，带增长后的 uncertainty；无效/过期样本返回关联 InternalError=8。
  成功和错误共用每会话 5 秒额度，超额静默拒绝；上次 enqueue 失败也不另行重试。支持输入/输出共用原 scratch。
- 把原 `DeviceRuntime.h` 中的 `WireError` 枚举移入协议头 `protocol/bbp2/WireError.h`，保持命名和值不变；
  responder 不再依赖完整 DeviceRuntime，也不复制错误数字。删除原位置的枚举，不留旧实现/兼容层。
- **当前任何产品都没有实例化 responder，也没有为子设备新增 TimeSync Hello feature。**
  核心生成响应不代表发送成功、拥有会话或通过了 Method 2；会话所有者仍负责 send/queue/retire，策略不能替代这些边界。

### 14.2 验证与资源

Host MSVC Debug/ASan 最终全组 **62/62** 通过，包含“一个共享时钟→两个独立额度→子设备协调器”的真实 codec/WallClock 往返测试；
授时/时钟专项 **4/4** 另过。验证 nonce/65535 序号、无权限/无 feature/坏报文不读源、40 B 边界、无样本关联错误、5 秒边界/tick 回绕、
scratch 别名、授权变化重验，以及网关已有漂移误差再叠加子设备 RTT；不能将转授当成零误差新样本。
真实 Runtime 还覆盖读取时年龄/误差增长、未配置清零、reset(false) 保持、过期锁存和 reset(true) 清零；安全 IClock 不受影响。

ESP32-S3 / R4 / Wio 三套编译器 × Small/Standard/Gateway ABI **9 项**：WallClock 40 B、coordinator 40 B、service **112 B**、facade **12 B**，
与上一包一致；独立 responder 为 **12 B**。默认 Device/Runtime 大小仍为 1676/400、3464/648、11008/1568 B。
这不是运行时堆/任务栈峰值；responder 尚无产品实例，不能把 12 B 乘子设备数计成当前已占用。

架构/release-profile 检查通过。WiFiBLE 冻结摘要 v39 经上一包 stage 对照，只新增 attachTime 接线；不变登录 wire/Key/TLS/配网。
共享快照不得读写安全/系统时钟、responder 不依赖 DeviceRuntime 的边界已进入 architecture 检查。
常规 ZIP ELF 检查新增默认排除业务时钟/未选 responder；显式开启对照必须完整链接 service/coordinator/clock。
本批还发现并修正默认依赖表误拒 TLS SDK 子库的问题：仅当已安装 profile 的 ESP32 WiFi 开启 HTTP/MQTT TLS 时，
允许 SDK `NetworkClientSecure`（旧 core 可内嵌）；不允许明文/纯 BLE/其他平台或未知新增库。包检查回归 **13/13**，真实默认 TLS ELF/依赖已复验。
标准单 ZIP 默认/开启 WiFiBLE、Hub **四目标安装编译、来源及 map/ELF 对照全部通过**：
ESP32 Arduino **3.3.11 / IDF NimBLE**、`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`，PubSubClient **2.8**。
使用同一 development **HTTP/MQTT TLS profile** 和原生受保护 NVS Hub，不使用 DualBoard 的弱存储替身。
普通设备的默认明文政策不改变；选择同一 TLS profile 是为了让 Hub 策略实际保留，并使各自默认/开启对照一致。

| 安装编译目标 | 程序 B | 全局 RAM B | 相对本模式默认增加 |
| --- | ---: | ---: | --- |
| WiFiBLE 默认 | 1,368,644 | 56,124 | — |
| WiFiBLE enableTime | 1,371,720 | 56,244 | +3,076 / +120 B |
| 内部 Hub 默认 | 1,456,132 | 75,916 | — |
| 内部 Hub 注入 TimeSyncService | 1,459,124 | 76,028 | +2,992 / +112 B |

两份默认 ELF 均无 WallClock/TimeSyncService/Coordinator；两份开启 ELF 完整链接同一业务服务；**四份均无 TimeSyncResponder**。
Hub 保留真实 Central/Route 产品，WiFiBLE 的未选传输/平台/网关/LAN 闭包通过；均只有 Blinker、Network、NetworkClientSecure、PubSubClient、WiFi。
逐文件核验安装 ZIP 字节和本批核心源文件、排除工作树源/include 泄漏，未跳过标准依赖发现、未手工链接。
每个模式只比较自己默认/开启，不能拿 Hub 与 WiFiBLE 的整体差额当授时成本；全局 RAM 不是运行时堆/任务栈峰值。

Hub 探针首次仅 include 深层头，Arduino 未发现安装库，编译失败；改为根 `BlinkerV2Advanced.h` 后标准重编通过，失败日志保留。
新探针验证完整 Product::begin 注入，不替换已有手工 Client 的 `CompileEsp32EdgeHubInternal` ABI 探针，也不作为公开例程。
ZIP SHA-256：`e43598d48aeca1b7d2c0eb847ec3e9018e666a75eb2d7cf96e254c4caa9b5d88`。
证据：ignored `build/b3-mixed-time-20260913/` 的四份日志/ELF/map、`abi.json`、`host-tests.xml` 和 `verified.json`；
`verify.py` 复验安装字节/源码/编译来源/依赖与链接，记录四份 binary SHA 和 `hardwareTest=false`。
本包为 **编译、软件和 ABI** 门禁；混合/Hub 自身本批授时、子设备授时、受保护 NVS、堆/任务栈均不因此获得新的真机通过声明。

### 14.3 下一包只完成南向接入，不越过授权和默认链接边界

审计确认以下是 **待实现项**，不是仅缺一次长循环：

1. **Method 2 权限快照。** `GatewayChildSession` 当前只保留已认证 credentialVersion，握手后清除 credential；`secure()` 不是 Control 权限。
   应保留本次真实证明匹配的权限，仅在当前 secure、凭据未到期/撤销、attempt 一致时暴露；不能从新下载的 store 推断旧连接权限，
   更不能把 Hello role/feature 当授权。断开、失败、撤销必须清理。范围只在南向会话，不给所有普通设备增加多槽权限缓存。
2. **可选 Hello。** `ChildSessionCoordinator` 的 known/config feature 目前未允许 TimeSync；必须在连接前按实际 responder 安装情况配置，
   协商取交集，未安装保持原 flags。处理中不能修改现有 Hello/伪造对端支持；Hub 自身启用授时不自动宣告子设备 responder。
3. **唯一 executor 分发/有界发送。** 在 `GatewayChildExecutor::onChildFrame` 拦截 TimeRequest，不转成上游业务 Event、不能加到 Cloud RouteBridge。
   保留孩子请求的 seq/nonce，不消费命令关联/Recent/ControllerControl slot。借用现有 scratch，发送前重验会话/权限/feature/大小；
   关闭或授权代际变更后不可提交迟到工作。复用 `sendFrame` 的 exact ciphertext 背压槽，不能重新 seal 重试、另加授时队列或广播。
4. **可选能力与编译闭包同时成立。** 策略代码不得因为默认 Hub 的虚表/模板里直接引用 concrete responder 就被永久链接。
   用最小可选协议扩展边界注入，普通 Executor 不依赖业务时钟实现；启用时共享一个业务服务，不给每个子设备创建 TimeSyncService。
   当前 responder 的 `reset` 仅由拥有者在退役旧会话后调用，不能当作绕过限频或旧工作代际检查的方法。
5. **门禁后回 B4。** 先确认混合/Hub 自身真实授时及业务并行，再补真实南向 Control/Observe、无样本/源过期、同序号普通 ACK、回压 exact bytes、关闭/重连/撤权迟到、默认/启用资源和短真机。
   软件策略通过不等于上述 executor/链路测试已过。随后回通用 LAN 注入/平台/发布默认和 §13 单槽 scope 接替，不扩 History/Bulk、百轮或新协议。

本批未修改 App/Service 生产代码，未部署、刷板或清授权；上一包恢复的硬件基线不动。
各端采用本节入口协作，原 §13.3 整体顺序不变，但 1/2 与 3 的完成状态分开记录。

## 15. B3-4c-3：唯一南向 executor 的可选授时（2026-09-13）

本节接续 §14.3，**Method 2 权限快照、按安装情况协商、唯一 executor 回应已接线并通过软件集成测试**。
这不是 Hub/子设备真机已通过，也不发布新的 Hub facade。普通设备 `begin/enableTime/time` 不变；不新建 MQTT、BLE 连接或授时队列。

### 15.1 模块职责与生命周期

- `GatewayChildSession` 保留本次 Method 2 credential 的 permissions，只有证明匹配且当前 attempt 仍 secure、未到期、无待处理故障才暴露。
  证明中的 permissions 必须与发起方凭据一致；不是从 Hello、UI 或后来更新的 store 推断权限。credential 仍在握手后清除。
  断连/失败清权限；逻辑退役立即关收发，即使 SDK 暂时拒绝取消也不回退为 Secure。迟到的非当前 attempt 报文直接忽略。
- `IGatewayChildRequestHandler` 是一个借用的、同步的内部协议扩展接口，不是新 router 或连接所有者。
  每 executor 只有一个可选指针；模块和 source 的寿命必须覆盖绑定。只有 Stopped/Idle 且无 native attempt 才能安装/移除。
  setter 对当前握手/安全/退役连接返回冲突，不改变现有协商，不重置活跃会话的限频额度。
- `GatewayTimeResponder` 适配原 `TimeSyncResponder`，每 active child 各一份限频状态，所有模块借用同一个 `IWallTimeSource`。
  自身 `TimeSyncService` 与子设备回应可独立启用：仅有 Hub 自身服务不会宣告南向 TimeSync。
  默认 Hub/executor/Cloud bridge 不引用 concrete responder 或 TimeSyncService，实现代码因此可由链接器排除。
- 内部 Hub 的 `setChildRequestHandler(index, handler)` 仅转交对应 executor，不新增成员/SDK依赖；测试探针显式安装两份 module 后注入一个业务服务。
  当前两槽上限不变，不给普通 BLE 设备增加网关权限槽，也不把每个孩子做成新 Client/Clock。
- 连接前把实际 module features 配给 `ChildSessionCoordinator`，仍保留网关必需的 ControllerControl；Hello 与对端取交集。
  默认无 module 不宣告 bit 24，未协商的请求不读取样本。能力位只表示支持，不能代替 Control 权限。

### 15.2 数据路径与发送边界

`子设备 TimeRequest → 当前 DirectSecure.open → 唯一 executor → 可选模块/共享快照 → 同 session.sendFrame → 原 GATT 队列`。

executor 在业务响应/事件路由之前识别 TimeRequest；TimeRequest/TimeResponse 均为 link-local，**无模块也不转成 Cloud/LAN Event**。
保留请求 nonce/sequence，不占普通 pending/recent/ControllerControl 关联；同 sequence 的普通 ACK 仍交给原 owner。
module 只在既有 scratch 的 `min(negotiatedMaxFrame, scratchCapacity)` 中编码，不拿到安全时钟、连接或发送对象。
应答后提交前重验 secure、attempt、permissions、features 与模块绑定，源回调中发生到期/退役也不能提交旧工作。

沿用现有背压语义：

- 发送前已有密文待排队：本次应答不排第二队列、不 seal，按原策略消耗本轮额度，子设备按原重试周期再请求。
- 已 seal 但 GATT enqueue 返回 WouldBlock：原 session 保存这份密文，poll 重送**完全相同字节**，不重新 seal、不给序号制造空洞。
- 不确定的 fatal enqueue：立即封住权限/后续收发，再由 poll 退役原 attempt；已到期的 pending 密文不 flush。
- 成功与关联 Error=8 共用每安全会话 5 秒限频；native 断开完成后或新的 Method 2 成功时 reset，不加延迟响应定时器。
  主动 stop 会先解绑底层回调，不能依赖它必然再报告 disconnected；新握手初始化覆盖此边界，不开放活跃会话重置 API。

业务样本仍带网关快照中增长后的 uncertainty，子设备继续加自身 RTT。业务 UTC 不进入 IClock/NTP/凭据有效期。
当前 `GatewayAccessRecord` 合同只接受既有网关权限集合（包含 Control），**没有“Observer 网关凭据”模式**。
Observer 不回应由通用策略负例覆盖；真实 Method 2 用不同 permissions 的证明验证拒绝，不能为测试擅自放宽持久化合同。

### 15.3 软件、资源及安装门禁

最终 Host Debug/ASan **62/62**、架构/release-profile、包检查 **13/13** 通过。
扩展现有 `test_gateway_child_session_main.cpp`，保留真实 Method 2 authorizer/initiator/HMAC、DirectSecure 帧和 executor；
仅用假的 radio/storage 与既有确定性 primitive provider 注入故障。它不是 AES-GCM SDK/真实无线测试，独立 crypto golden 仍随全组运行。
覆盖：握手前/证明不匹配/过期/退役/故障/attempt 不一致/stop 无权限，双边可选 Hello、关闭时不转发，
同序号普通 ACK、关联无样本错误、5 秒限频、native 断开/完整 stop 后新握手额度恢复，陈旧 record 不消耗新会话序列，
相同密文背压重送、占用单槽时无第二队列/无序号空洞、fatal enqueue、pending 到期与源回调中到期。
去掉该用例原来只拒绝所有操作的 UnusedCrypto，复用共享测试 primitive；失败改为非交互退出，避免 Windows abort 对话框挂住门禁。

三套编译器 ESP32-S3/R4/Wio × Small/Standard/Gateway，旧安装包与当前源 **9 组配对、18 次 ABI 编译**：

| 对象 | 旧 B3-4c-1/2 | 当前 | 解释 |
| --- | ---: | ---: | --- |
| GatewayChildSession | 560 B | 560 B | permissions 占用原有对齐空间 |
| GatewayChildExecutor | 580 B | 584 B | 一份可选模块指针 |
| GattChildContext，Small/Standard/Gateway | 7552/7632/7760 B | 相同 | 该 ABI 下结构体对齐吸收 executor 增量 |
| TimeSyncService / TimeSyncResponder | 112/12 B | 相同 | 共享业务钟/独立策略未增长 |
| GatewayTimeResponder | 未实例化 | 每个 16 B（可选） | 12 B 策略加接口虚表指针 |

普通 facade/Device/Runtime 大小不变。不能由结构体 padding 推断其他 ABI 永远免费，也不能把静态大小当堆/任务栈峰值。
ESP32 Arduino **3.3.11 / IDF NimBLE**，`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`，PubSubClient **2.8**，
同一 HTTP/MQTT TLS development profile + 原生受保护 NVS Hub：最终标准单 ZIP 三目标安装编译、来源/依赖与 ELF **全部通过**。

| 当前完整产品探针 | 程序 B | 全局 RAM B | 相对本批默认增加 |
| --- | ---: | ---: | --- |
| Hub 默认（无自身服务/子设备模块） | 1,456,560 | 75,916 | — |
| Hub 自身 TimeSyncService | 1,459,556 | 76,028 | +2,996 / +112 B |
| Hub 自身服务 + 两份子设备 responder | 1,460,996 | 76,060 | +4,436 / +144 B |

因此南向可选部分相对 **自身授时版**为程序 **+1,440 B**、全局 RAM **+32 B**，与两个 16 B 模块一致。
与 §14 旧默认 Hub 相比，本批默认程序 +428 B / RAM 不变；权限/attempt/错误围栏及可选扩展入口不是“默认零代码成本”。
两份无南向模块 ELF 均无 TimeSyncResponder/GatewayTimeResponder；默认还无 TimeSyncService/Coordinator/WallClock，只有启用版才完整链接。
三个目标保留原 Central/Route 产品；依赖均仅 Blinker、Network、NetworkClientSecure、PubSubClient、WiFi。
逐文件核验 ZIP 安装字节与本批核心源码，compile_commands 排除工作树源码/include 泄漏；未跳过标准依赖发现、未手工链接/改安装源码。
首次三目标通过后，因完整 stop 的新会话额度边界修订，**重新打包、标准 CLI 重装并复编三个目标**；首轮报告保留为 first-pass，不拿它替代最终证据。
最终 ZIP SHA-256：`63cde912e02489c07307788da89799065ab1a027f6326d4140d8181e733b26db`。
证据均在 ignored `build/b3-child-time-20260913/`：host-tests.xml、abi.json、artifacts-final 单 ZIP、安装目录、日志/ELF、verify.py/verified.json；
后者记录三份固件 SHA 与 `hardwareTest=false`。这是实际编译/静态资源门禁，不是受保护 NVS、无线、运行时堆或任务栈峰值门禁。

### 15.4 下一步与本批边界

下一包进入 **B3-4c-4 短真机准入/闭环**，不重新设计模块、重跑百轮或扩 History/Bulk：

1. 确认当前板卡角色、WiFi、App/服务和已恢复固件；使用匹配单 ZIP，先 WiFiBLE/Hub 自身授时与 Property/Action 并行。
2. 同一只读业务源经现有真实 Method 2/DirectSecure 给子设备授时，记录完整帧大小、nonce/sequence/uncertainty，短测断开/源不可用/恢复。
3. **生产 Hub 受保护 NVS/不可逆 eFuse 门禁仍后置。** 没有可报废板时不得烧 eFuse；使用已有明确标记的台架替身所得结果只算台架功能，不替代生产安全门禁，也不放宽发布策略。
4. 软件/安装资源已通过与新真机尚未通过分开报告。随后回 B4 通用 LAN 功能注入/平台/默认发布及 §13 单槽 scope 接替。
   BLE Observer/主动旧 nonce、默认一小时过期、R4/Wio 当前硬件与加密 LAN 独立项目仍保留，不由本批 ABI/host 冒充通过。

本批不改 App/Service 生产源码/路由白名单，不安装 APK、刷板、清授权、部署/迁移，也未暂存/提交/推送。
短真机准备只读核对：2026-09-13 19:12，ADB `196f65f9` 与 COM3/COM11/COM12 在线，Gateway/Broker healthy；
电脑接“带翅膀的蜗牛”，手机接同路由 Wi-Fi5 / 192.168.2.31。未据串口存在推断板卡身份/固件/凭据正确，烧录前仍须精确核对。

## 16. B3-4c-4a：正式 WiFiBLE 图的短真机闭环（2026-09-13）

### 16.1 实现与准入

新增内部 `tests/internal/Esp32WifiBleTimeProbe`，使用 `BlinkerWiFiBLE.h` 实际选择的
`Esp32OfficialWifiBleProduct` + 原 `Product::begin(ITimeSync*)`，不是手动 WiFi 的替代 composition。
当前这个入口读取 WiFiProv 持久化的 DeviceKey/接入根与网络，没有直接 `configure(key, ssid, password)`；
不能把普通 WiFi 的手动图改名就算双模通过。这里没有新增公开 API、队列、连接或生产故障开关。

LAN 和本探针共用 `tests/internal/support/ObservedTime.h`，仅计数真实服务接受的样本；
只按 `s` 查询，输出来源/年龄/误差/控制状态。借用对象先于 Product 构造、晚于其销毁。
修正 Hub compile probe README 的过时“南向未实现”说明；测试代码不进入发布 ZIP。

- 当前网络：电脑“带翅膀的蜗牛”/192.168.2.76；手机同路由 Wi-Fi5/192.168.2.31，ADB `196f65f9`。
- COM11 MAC `8c:bf:ea:1b:96:14`，实例 `4b3efcd46ff96acc1908f1affefda192`；精确备份并核对原应用 SHA 后，仅写 `0x10000` 应用区。
- 独立开发 profile 的标准 ZIP 安装、来源、ELF/依赖检查通过：ESP32 Arduino 3.3.11 IDF NimBLE，HTTP/MQTT 明文及开发用普通存储。
  BLE WiFiProv Security1/应用层 DirectSecure 仍按真实实现运行；这些结果不代表 Hub TLS/受保护存储或无加密 BLE。
- 最终探针程序 **1,283,460 B**、全局 RAM **55,612 B**，包含测试观测器，不能当作单独授时模块增量。
  不链接 LAN、GatewayTimeResponder 或南向队列；授时按一个真实服务运行。
- ZIP SHA-256 `299e3bc226b572d7f578f66fb95e076e008cc61dc5ddc49b647360a515d34a59`；
  固件 SHA-256 `f0bcd2c28b3fc1717fadb4ecc0e4b325dbf375da8ff6524db10180ff92d32ee6`。

### 16.2 真机结果

保留原 `B3 BLE Time COM11` / `ble_LhR-mYNVmrHJnnavXGVd9A`，通过真实 App 配网页、Security1、GetInfo 和用户确认流程启用 WiFi。
`enableCloud=1`，`create/rotate/reset=0`；accessEpoch `1352336423` 不变，保留原 BLE 凭据/页面。
配置成功后 App 自动进入原 `/device/<id>`，页面实际 BLE ready。

| 门禁 | 本轮证据 |
| --- | --- |
| 配网前业务时间 | `valid=0, utc=0, bootUnsynced=1`，没有本地假时间 |
| 双模自身云授时 | 原 MQTT 接受一个 Server 样本；BLE 可用时仍 `serverSamples=1, localSamples=0` |
| 页面拥有的 BLE 控制 | 两次 Property **355/383 ms**，button Action **332 ms**，实际计数变为 1 |
| BLE 保持连接时的云控制 | 同 ID 的 MQTT Property **190 ms**、Action **221 ms**；两个入口状态一致、实际计数变为 2 |
| 退出控制页 | BLE 退回扫描/附近，原账户 MQTT 未重连；Cloud Property **248 ms**，业务 UTC 不清零 |
| 普通重启/持久化 | 无再次配网，业务时钟从未同步开始，启动 **2,389 ms** 接受云样本；原 BLE 凭据仍可重连 |
| 重启后 BLE/云控制 | BLE Property **288/369 ms**、Action **299 ms**；关闭该 BLE 后云 Action **188 ms** |
| 恢复原固件 | 原静默 WiFi/原 Key/原 `device_5f9d3415-8395-4538-bad6-1c2bc5bb28d7`，Property **210 ms**、Action **253 ms** |

重连测到 **3,005 ms（不含发现）**；这是功能通过，不是“无缝/低于 1 秒”通过。
观测 UTC 的误差预算约 1.1 秒是协议报告/漂移预算，不是实验室校准精度。
`localReady=1` 仅表示产品本地能力就绪；真正会话证据来自页面 BLE ready、实际 ACK 和 State，不能以该位代替已连接。
两次 boot 都只接受云样本；本轮没有伪造 nonce/UTC/授权，没有据此宣称 BLE 本地源切换、默认一小时过期或子设备授时通过。

COM3 原账户 MQTT 控制见证前后通过；COM12 未刷写。COM11 原应用精确恢复 SHA
`84f124dbedcc9fbe4c3fc290e661b80b52d3ef4e03b59f8f02fabbf19dc8e485`。
最终 App 回首页、测试 owner/会话回收、跟踪 BLE links=0，原账户 MQTT 保持。
本轮启用 WiFi 的原 BLE 测试身份和真实持久配置保留，**没有声称恢复了配网前 NVS**；原手动 WiFi 固件使用原 Sketch Key。

### 16.3 现场缺陷与最小修正

首次配网被 `WIFIPROV_CHECKPOINT_DEVICE_MISMATCH` 阻断，未发生 Key/设备注册写入。
原因是添加入口无逻辑 ID 时，`findWiFiProv(instance)` 取到同实例旧已退役测试身份
`ble__Z_ZA8SUGgyIiqkfAf3ZBw` 的 active checkpoint（epoch `1360873494`），而当前 BLE 根已属另一逻辑 ID。
核对当前账号 resolve 指向本轮原 BLE ID、旧 ID 不在 inventory、旧 DirectAdmin 返回 null 后，
按已授权测试清理范围仅删除这一条失效 App 本地凭据，未备份；未清设备 NVS、当前凭据或其他账号。
随后取得 §16.2 的真实闭环，**该人工清理不能当作 App 自动恢复修复已经真机通过**。

App 生产代码现已小修正：添加入口面对已有接入根时，先用既有只读 resolve API 获取当前账号的逻辑 ID，再精确 `load(id)` 恢复检查点。
显式重新配网保持其 expected ID；空根 pending 的原恢复流程不变；找不到归属、服务失败、账号变更、精确记录 epoch 不符均失败关闭。
不按 MAC 认身份、不吞 mismatch 后新建/rotate、不自动删除未知旧凭据，不改 UI/Service 合同。
回归覆盖同实例旧缓存不被选中、无归属/断网/切账号无副作用、精确 pending epoch 错误仍拒绝；
相关 App **6 文件/72 项**、app/spec 两套类型检查通过。修正 APK **尚未构建安装/真机验证**；手机仍是本轮原包。

### 16.4 状态与下一步

- 本轮：Host **62/62**、架构/profile、包 **13/13**、上述单 ZIP 与 WiFiBLE 真机均通过。
  ignored `build/b3-mixed-live-20260913/` 保存备份、固件、原始 App/串口证据及 `verified-live.json`；不要将私有测试产物打包发布。
- 下一小包：匹配 APK 验证精确检查点选择；再 **B3-4c-4b Hub 自身与真实南向子设备授时**，复用同一业务源/现有 executor，记录实际帧关联、断开、源失效和恢复。
- 不要重复本轮配网升级或重建账号，也不要因 Hub 真机依赖而扩百轮。生产 Hub 受保护 NVS/eFuse 后置，已有明确标记台架可验证功能，但不能代替生产安全准入。
- 之后回 B4 通用 LAN 注入、默认产品化/平台矩阵及单槽 scope 接替；BLE Observer/主动旧 nonce、默认一小时、R4/Wio 当前硬件、加密 LAN 独立门禁仍保留。

本批无 eFuse、全 Flash/设备授权清空、部署/迁移、暂存、提交或推送；App 生产改动仅上述身份选择逻辑，Service 无代码改动。

## 17. B3-4c-4a-r：匹配 APK 与仅重置网络真机（2026-09-13）

### 17.1 实施与边界

完成 §16.3 修正的 local Android 构建和保留数据更新安装；没有继续增加生产 API/身份分支。
安装前保存原 APK；新 APK SHA-256 `c8efcff8ee61790ad3663c8151cc7823cc83c7f8c901836ab223e0fd8917c20d`。
只对生成的测试 www 指定本机 HTTPS 入口/LAN 开关，不修改 App 源码环境或持久化用户令牌。
账户 MQTT 连续性从安装后起算，不能声称安装导致的进程重启也没有重连。

`Esp32WifiBleTimeProbe` 增加编译期可选 `BLINKER_TEST_NETWORK_RESET`：每 boot 最多一个串口 `n` 调用真实 `Product::resetNetwork()`。
默认没有这个入口；不调用 resetAccess，不清 Key、接入根、子设备记录或整片 Flash。`s` 仍只查询。
显式产品网络重置会停/启生命周期并清本次业务锚点，**不等于普通掉线 holdover 失效**；普通掉线语义仍按 §9。
该命令仅在没有进行中配网/Direct 事务时使用；主机写前记录一次意图，超时不得盲目再次发送。

沿用 §16 同一已安装 ZIP 并逐文件核验工作树源码/安装包、编译来源和 ELF 闭包。
标准 Arduino 3.3.11 编译：程序 **1,283,916 B**、全局 RAM **55,612 B**；相对无网络重置观测探针 +456 B 程序、RAM 不变。
固件 SHA-256 `e54456eb9b478683860a879c02bc8dca9d2551ddbd916ce8ff10f1267f77f07a`。
原 COM11 应用先从硬件精确读回核对，再仅写 `0x10000` 应用区。COM3/COM12 未刷写。

### 17.2 真机结果

现场仍为“带翅膀的蜗牛”、电脑 192.168.2.76 与已解锁原手机；原实例/逻辑 ID/epoch 同 §16。
先验证持久身份自动云登录并在 boot **2,526 ms** 接受 Server 样本；一次网络重置返回成功，随后走 App **添加入口**而非显式重新配网上下文。

- Security1 + 真实 GetInfo 返回 flags **7**、epoch **1352336423**，证明 Key 与接入根仍在。
- 原 HTTP resolve 指向 `ble_LhR-mYNVmrHJnnavXGVd9A`；配网期间三次 credential load 均为这个精确 ID。
  对旧 `findWiFiProv` 加只报错的调用探针，实际调用为 **0**；真实 resolve/load/服务响应均未替换。
- 用户保留原页面/控制权确认后，原 enable-cloud **1 次、replayed=true、sameIdentity/sameVersion=true**；
  create/rotate/reset **全为 0**，凭据 secret 的内存摘要前后相同且未导出，epoch 不变。
- WiFi 恢复、接受第二个 Server 样本，`localSamples=0`；自动回原控制页并实际 BLE ready。
- BLE 两次 Property **340/289 ms**、button Action **295 ms**；同时原 MQTT Property **242 ms**、Action **189 ms**，实际 buttonPresses **2**。
- 退出页后 BLE 释放，原 MQTT Property **243 ms**，账户 session 不变；COM3 TLS Property ACK **140 ms**。
- 恢复原静默 WiFi 应用 SHA `84f124dbedcc9fbe4c3fc290e661b80b52d3ef4e03b59f8f02fabbf19dc8e485`，
  原普通 WiFi ID/Key 的 Property **132 ms**、Action **213 ms**、真实 State 通过。App 回首页，测试 owner/跟踪 BLE links 均回收。

本轮没有删除任何凭据或重建身份。§16 被删旧记录没有重新伪造放回：**旧/新缓存共存为软件回归；真实 APK 已验证精确选择分支及不调用模糊查找**，两类证据不混写。
这也不是首次空根注册、跨账号恢复、断电 journal 或全部重新配网失败窗口的验收，不由本轮覆盖它们。

### 17.3 验证与后续

App 同组 **6 文件/72 项**、app/spec 类型检查、完整 local/Android 构建通过；设备 Host **62/62（Debug）**、架构/profile、包 **13/13** 通过。
ignored `build/b3-checkpoint-live-20260913/` 保存 APK/原应用备份、标准编译、串口/App/云见证及 `verified-live.json`；不发布这些私有产物。
原 Hub 双板台架仅增加可选共享业务服务和两个 responder，保留现有 executor、TLS、SNTP、存储边界；其准入/后续结果另记，不拿本节代替子设备真机。

本轮未修改 Service 生产代码、部署、迁移或数据表；没有 eFuse、授权清空、暂存、提交或推送。
下一项 **B3-4c-4b：Hub 自身授时准入 → 原孩子真实 Method 2/DirectSecure 授时/控制 → 短断开与恢复/来源不可用**，随后回 B4。
受保护 NVS/eFuse、BLE Observer/主动旧 nonce、默认一小时、R4/Wio 当前硬件与加密 LAN 仍独立保留。

## 18. B3-4c-4b：Hub 自身台架准入与普通重启（2026-09-13）

### 18.1 最小接线与安装

复用既有 `tests/internal/arduino/Esp32EdgeHubDualBoard`，仅在 `BLINKER_TEST_ENABLE_TIME` 下增加一个共享 `ObservedTime` 和两个 `GatewayTimeResponder`。
它们先于借用者构造；`attachTime`、两个 `setChildRequestHandler` 均成功才 start。未启用时不创建时钟/responder。
新增串口 `u` 只读取有效性、UTC、来源/年龄/误差和真实接受计数；不生成样本或额外请求。
保留其他仍被门禁使用的原台架故障命令，本批不触发它们；没有新建第二套网关 composition、南向队列或公开初始化入口。

本台架沿用已有、明确标记的弱存储替身；**没有把生产 protected-NVS alias 改成普通存储，也没有烧 eFuse**。
TLS、SNTP、安全 IClock、管理订阅、真实会话与业务服务仍为原实现。台架功能通过不代表生产存储或 BLE 无线压力通过。

生成当前开发 profile 的新单 ZIP，标准 CLI 安装/依赖发现、工作树来源隔离、逐文件字节与 ELF 闭包均通过：

- Arduino ESP32 **3.3.11**，同 XIAO ESP32S3 默认 8 MB 分区；程序 **1,466,580 B**、全局 RAM **75,452 B**。
  含原台架诊断/故障代码与弱存储替身，不能与 §15 生产存储探针相减后当授时增量。
- 只链接 Blinker/Network/NetworkClientSecure/PubSubClient/WiFi；实际 Central、Route、TimeSyncService 和两个 responder 闭包保留，无普通 BLE transport/LAN。
- ZIP SHA-256 `6fbc13494ab762faa55f348e6707f8781a3f8f3cb16a78a4ee9c15962ad8a04c`；
  固件 SHA-256 `f0e544c36d23663a67432faaa857c4800bfb682907e56902d21e281e1fe34eb8`。
- 原编译对照 profile 的 `17002` 是 plain 入口；本次 live TLS profile 使用 **17000**，未部署/修改服务器端口。
  使用原受信 CA 验证现有 leaf 的 IP SAN/链，真实 Node TLS 验证通过；CA 自身 SAN 不等于服务端 leaf SAN，不能因 CA 没有当前 IP 就换根或关验证。
  不把仅叶证书直接当 CA 后的验证失败误写成服务器证书错误；没有关闭主机名/证书验证。
- 生成 ignored Sketch 只替换原 test-local profile/network includes，使用当前已确认网络；不把旧私有头带入 ZIP，不重新安装/编译 Hub DeviceKey。

### 18.2 本轮真实通过范围

COM11 只换应用区，读取原 Hub 独立持久身份/Key 与网络记录；原账号 `XIAO Edge Hub E2E` /
`device_4af104c0-4a84-4e85-9009-4c2390c9e5a3` 的真实 App Manifest/State 成功。

| 项目 | 实际结果 |
| --- | --- |
| 网络与管理 | IP 192.168.2.63，networkReady=1、managementSubscribed=1、managementError=0 |
| Hub 自身授时 | requests=1 / samples=1 / serverSamples=1，source=Server，误差随年龄增长，无替代 UTC |
| 普通重启 | 明确观察到 valid=0/utc=0/requests=0，随后恢复 valid=1/requests=1/serverSamples=1；没有再次配网或新 Key |
| 重启前后 App | 原 Hub Manifest/State 两次通过，原账户 MQTT 不重建，观察租约已释放 |
| 子设备 | 当前 childState=0、secure=0、executionWanted=0；**尚未进行 TimeRequest/TimeResponse 南向真机** |
| 资源观测 | idle Hub 样本 heapMin 最低 153,888 B、loopStackMin=3,700 B、ipc0StackMin 最低 1,252 B；不是忙载峰值或原 SDK 风险已修复 |

`nimbleStackMin=0` 在本轮表示未找到已启动的该任务，不能当作“栈只剩 0 B”或“无需 NimBLE 内存”。
本台架只定义只读 `hubReady`，App 获得有效但空值的初始 State；没有伪称执行了 Hub Property/Action，也没有伪造子设备回应。
本轮未增加 MAC 身份规则、重建拓扑、清子设备授权或修改 Broker 白名单。COM3/COM12 未刷写。
最终恢复 COM11 原普通 WiFi 应用/Key/ID，SHA 与 §17 原备份一致；Property **182 ms**、Action **198 ms** 与真实 State 通过。
COM3 原 TLS 见证两次 Property **129/100 ms**、State 实际变化通过；App 回首页、测试 owner 回收、BLE links=0，原账户 MQTT 保持。
新 App 修正版保留安装，未回装旧检查点选择逻辑。设备本轮使用/确认的网络记录保留；不宣称还原了历史 NVS 字节。

### 18.3 下一包与证据边界

1. 复用本次已验 Hub ZIP/固件，不再次改变产品图。精确核对并备份 COM12 当前应用，使用普通 BLE 设备能力 + 可选授时，保留原接入根；COM3 留作云见证。
2. 先核对原孩子实例/逻辑身份与已有拓扑，再由真实 App 业务需求驱动 Hub 原 Method 2/DirectSecure；不按串口/MAC 猜身份，也不因记录不符自动新建或清授权。
3. 同一实际会话验证授时请求/响应关联、源误差、Property/Action、普通断开/重连及源不可用/恢复。只为观测增加内部有界计数，不增加生产接口、连接或队列。
4. 当前 `DeviceRuntime` 把已鉴权 local 路径记为 `Controller`；它包含网关控制端，不证明是手机。
   `Gateway` 枚举不是已经验证的具体身份标签，不能根据 Hello.role 改标或增强信任；真实南向事实必须由会话/物理链路证据确认。
5. 子设备短闭环后回 B4 通用 LAN 注入/默认产品化与单槽 scope 接替；独立安全/平台/长期过期门禁仍保留，不扩成百轮。

本次 ignored `build/b3-hub-live-20260913/` 保存匹配单 ZIP、安装/编译、TLS 准入、串口和 App 证据。
`verified.json` 为编译验证；台架自身真机不能据此将 `protectedStorageTest` 或南向项目改成通过。
`verified-live.json` 分别记录自身授时/普通重启/原固件恢复通过，以及 `southboundTime=false/protectedStorageTest=false/bleStackLoadTest=false`。
本批未部署/迁移/清授权/烧 eFuse，未暂存、提交或推送。

## 19. B3-4c-4b 南向真机与本地发送背压修正（2026-09-13）

### 19.1 真实前置与失败，不能省略

沿用原账号、Hub 与 COM12 BLE 孩子的持久身份，真实账户 HTTP 核对既有拓扑 Active、版本 4、credentialVersion 185、
accessEpoch 494518246，凭据未过期。手机没有该孩子 Direct 缓存，但本轮使用 Hub 独立凭据，不因此重新注册或清根。
COM11/COM12 原应用均精确读回匹配 SHA/实际分区容量后，只换应用区；COM3 保留云见证。

新增 `tests/internal/Esp32BleTimeProbe` 仍是 `BlinkerBLE.h` 的普通 `Esp32PlatformBleProduct`，不是另一套“网关子设备固件协议”。
由原账户 MQTT 的 connection demand 唤醒 Hub；另持有 state interest 获取后续 State。两者职责不同，不创建第二条 MQTT。
已真实通过 Method 2/DirectSecure、Manifest/State、Property/Action：早期 722/504/524 ms；完整 State 观察组 428/518/532 ms，
物理孩子计数实际增长。手机 BLE links 始终为 0，不能将此标成手机直连授时。

首轮时间请求编码成功但样本为 0。应答端观测请求为 0，子设备观测编码后同一毫秒 `reset(false)`，实际未入队；
现有软件也复现本地 `WouldBlock` 后需 30 秒才重试。短 BLE 会话中 ACK/bootstrap 先占用发送槽，统一长退避可反复错过窗口。
不把“requests 编码计数”写成已发送包数，也不把序号增长写成无线包风暴。
另一个独立现象是现有 GATT 公平策略 10 秒占用/6 秒让出，实际 Secure 后约 7～8 秒会退役；
该策略未在本批改变，控制/授时短闭环成功也不代表已经无缝常驻。

### 19.2 最小修正与证据口径

Runtime 在请求成功编码、但本地 transport 拒绝 enqueue 时通知可选 `ITimeSync::sendRejected(error)`。
业务服务取消这次 nonce/RTT；**只有 `WouldBlock` 且确有 prepared request** 把下一次尝试设为 1 秒。
每次重新检查协商/Control/会话、生成新 nonce，保留有效 UTC；其他错误、RNG/编码失败、远端 Error/超时仍为原 30 秒，成功仍为五分钟。
不新增数据成员、定时任务、TX 队列、连接或用户 Sketch API，也不修改 Hub 的 TLS、安全时钟和生产存储 alias。

共享内部观测器仅新增可选有界 trace。`s`/`t` 分开输出，修正首版非阻塞 USB 大行截断；请求编码、入队拒绝、真实接受样本分别计数。
Hub `y` 只看真实 handler 结果；可选 `v` 注入 45 秒**子设备时间 provider 不可用**，保持 Hub 自身真实时钟和控制链路。
该注入不是等待默认一小时自然过期，也不伪造 UTC、Error、凭据或权限。

新增软件用例先失败后通过，覆盖 999/1000 ms、持续背压无逐 loop RNG、旧未发送请求响应拒绝、无 prepared request 不缩短五分钟、
非背压/RNG WouldBlock 不走快重试、撤权后不发。完整 Debug Host 62/62、架构/profile、包 13/13 通过。
新增重连不退还背压窗口及 32 位 tick 跨回绕边界也通过。匹配真机与未过边界分别如下。

### 19.3 修正固件真实通过与仍然失败的范围

子设备匹配新 ZIP SHA `b079c8cd2ee2c5a36eda269f5e61cd30e73f947041d4ec3205324fda7b0717bf`，
Arduino ESP32 3.3.11 标准 CLI 安装编译、逐文件/source provenance 与 ELF 闭包通过。
程序 **694,788 B**、全局 RAM **35,016 B**；固件 SHA `f6b58a29a42e2830e512a265aba939571431a10515ceeae1875c2c0131a64c9d`。
这是含观测器的普通 BLE 测试 Sketch 总量；不将不同 trace 版本相减当纯生产授时增量。
只链接 Blinker 库，保留 PlatformBleOnly/BleFrameTransport/Time，无 MQTT/Route/LAN/网关 responder。
Hub 沿用本轮**修正前已验证**的真实应答端二进制 `fb263388474592f9a43d8f7fcacbb932281b598463f5c19dbccdb2c2efff9c15`；
程序 1,467,424 B / RAM 75,516 B，含内部故障/provider 和 handler 观测。没有修改该已验 Hub 来配合新子设备伪造回应。
两端并非同一 ZIP：这个兼容事实表明修正没有改变 BBP/Method 2/DirectSecure wire；也不把旧 Hub 当当前新源码编译证明。

| 项目 | 本轮实际结果 |
| --- | --- |
| 背压原因 | 新探针明确 `sendError=22/WouldBlock`，3 次本地拒收后第 4 个编码请求成功；不是无线收到 4 包 |
| 真实授时 | Hub handler permissions=3、request/reply=1；孩子 samples=1/local=1/server=0，bootUnsynced=1 |
| 关联/大小 | sequence=9774、nonce 对应，TimeRequest body 18 B/完整 BBP 28 B，TimeResponse 完整 BBP 40 B；不含 DirectSecure/分片/ATT |
| 来源/误差 | 源报 1,119 ms 不确定度；孩子已验后的样本随本地年龄继续增长；source=Controller 是实际 local 绑定路径标签，不另信任 Hello.role |
| 同会话控制 | Property 506/513 ms、button 540 ms；实际 State/物理计数=1 |
| 自然短断开/恢复 | 旧公平策略造成会话退役后 UTC 保持有效，samples 仍为 1、不借重连刷新五分钟；恢复 Manifest/State 后 Property 437/491 ms、button 512 ms，计数=2 |
| 独立云见证 | 原 COM3 TLS Property ACK 259 ms；后续 110 ms 的相同值写入只算 ACK，不冒充 State 改变；单个原账户 MQTT、无手机 BLE 直连 |

失败保留：切换窗口边缘的一组控制仅首个 Property ACK=774 ms，后一个控制失败，未自动重试 Action。
紧接着的手动 `d` 返回 19/NotFound，说明当时没有可断开的活动链路，**不计手动断开门禁通过**。
随后表中的控制恢复是真实新操作，不是把失败操作改写为成功。现有“10 秒占用/6 秒让出”仍可产生感知迟滞与边缘失败，
继续留网关连接策略/B4 联合后续；未通过增加超时、假响应或多跑循环掩盖。

准备下一次手动断开时 ADB 列表变空、9222 转发消失，板仍在 COM3/COM11/COM12。
此前 child gate 已明确 close，原 COM3 witness 有 120 秒截止；未再次创建或注入账号连接，未发送 `v`。
当时源不可用/关联 Error/自动恢复、手动断开注入尚未通过。用户随后接回手机，已补验，结果见 §19.4；此前失败和 USB 中断仍保留，不因复测成功抹去。
ignored `build/b3-southbound-live-20260913/` 保留失败/成功串口、App 投影、备份/编译与红→绿测试证据。

### 19.4 ADB 恢复后的短门禁、原应用复验与下一包

手机接回后，原登录身份/数据仍在；USB 丢失使 17000 HTTPS 与 18885 账户 WSS reverse 消失，恢复原端口后 App 自动重连。
**USB 间隔前后 MQTT session 已变化**，不再声称跨失联全过程 session 不变；先检查并释放已截止 test owner，再以原账号当前唯一 session 建立新观察。
未重新登录、安装 APK、创建额外 MQTT 或手机 BLE 链路。先验证两块原 WiFi 应用 Property/Action/State，再复用 §19.3 已验二进制，仅写应用区。

| 补验 | 实际结果 |
| --- | --- |
| 活动会话普通断开 | `d` 明确返回 0；断开前已接受样本，断开/恢复后 samples=1，UTC 有效、年龄继续增长；不重置五分钟刷新 |
| 断开后控制 | Property 536/626 ms、button 628 ms；App State 与物理 presses=1。ready=19,567 ms 包含首次 Hub 重启上线等待，不当 BLE 稳态连接耗时 |
| 真实来源不可用 | 关闭需求后普通重启孩子，只清易失业务锚点；`v` 仅暂停 Hub 子设备时间 provider 45 秒，Hub 自身 UTC/serverSamples 始终有效 |
| 无源处理 | 孩子 valid=0/samples=0，接受相关 sequence=6854 的真实 Error；Hub handler 输出完整 BBP 17 B。错误接受由真实 service 的 peer/sequence 校验决定，不用观测器构造回应 |
| 无源时控制 | Property 524/606 ms、button 576 ms；物理 presses=1。App 当次按钮 ACK 后尚未收到该 State，后续真实重连/快照才显示 1，不能称全程即时 State |
| 自动授时恢复 | 无额外 reset/手动校时；孩子开机 56,574 ms 接受样本，request/response sequence=7473、nonce 对应，完整 BBP 28/40 B、源误差 1,147 ms；errors=1/rejected=3/编码 requests=5、实际样本仅 1 |
| 恢复后控制 | Property 524/631 ms、button 682 ms；State/物理 presses=2、UTC 继续 holdover。此观察全程原账户当前 session 相同、手机 BLE links=0 |
| 独立云见证 | 来源暂停期间 COM3 同账户真实 Property 改变/State，ACK 199 ms；先前同值 110 ms 只算 ACK |

来源暂停是内部 provider 故障注入，不是默认一小时自然到期；Error 后仍按原 30 秒请求退避，未被本地 WouldBlock 的 1 秒分支缩短。
原拓扑仍 Active/version=4、credentialVersion=185、accessEpoch=494518246，HubStored/childInstalled/gatewayProven 保持。
测试 owner/兴趣/计时器已释放、test-only 全局引用已删除；两块板再次恢复已核对的原应用：

- COM11 原普通 WiFi 应用 SHA `84f124dbedcc9fbe4c3fc290e661b80b52d3ef4e03b59f8f02fabbf19dc8e485`。
- COM12 原 LAN WiFi 应用 SHA `37e6359ae3c881c639f97da24790c63fbf5dd9eac282a69f25c895a92a822d5c`。
- 写入校验通过，COM3 未刷；没有清 Flash/NVS、Key、接入根、网络记录、拓扑或权限。
- **最终原 WiFi 应用控制/State 已复验**：COM11 Property 155/254 ms、button 160 ms；COM12 Property 1,782/148 ms、button 125 ms，二者实际计数均 1。保留 COM12 首次长尾，不将功能通过表述为所有控制都低延迟。
- `verified-live.json` 明确逐项记录已过/未过，不设置笼统的全部完成。未暂存、提交、推送、部署服务或重新安装 APK。

**B3-4c-4b 的本组南向短闭环收口，下一包回 B4**：先复核 LAN §31.3 单槽不同 scope/caller 的退休与接替合同，沿用真实 ACK/权限/截止时间，不扩槽或延权；
随后通用 LAN extension 接入 WiFi/WiFiBLE/Hub、声明平台/资源和标准 ZIP 门禁，再匹配短真机。默认关闭/未支持的平台不能硬开宏称完成。
共享网关持续需求却周期退役的控制边缘失败独立保留，先联合审计唯一 executor、Direct 显式让出、单连接孩子离线可达性，再实施最小策略；不是取消公平窗口后永久占用。
不得重做已过注册/授时、自动重发未知 Action 或扩成 100 次循环。
当前新源码的完整跨平台/默认关闭重编译、默认一小时自然过期、Observer BLE/主动旧 nonce、受保护 NVS/eFuse等独立发布门禁仍保留。
