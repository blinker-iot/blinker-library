# Device V2 WiFi + BLE 产品准入（P1-4）

> **归档（2026-08-16）**：旧 `WifiBleProductProbe` 已删除并退出 CI。本文保留的 transport、
> 资源和真机准入原则仍可参考，但所有旧 product size、Enrollment/1 和编排器结论都不是当前发布证据。

> 2026-08-15 变更提示：本文只保留 transport/resource 探针结论。正式 Ownership、Cloud、Controller
> 接入顺序和统一 public profile 以 `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 为准。

> 历史状态：静态组合与P1-4a归因门禁曾完成；真实硬件并发/24 h 门禁未完成  
> 基线日期：2026-08-14  
> 历史探针：`tests/arduino/WifiBleProductProbe/WifiBleProductProbe.ino`（已删除）

## 1. 当前判定

- PubSubClient 继续作为默认/参考 MQTT adapter；P1-4 不接入 ArduinoMqttClient。
- 一个 `Client`、一个 Registry/StateStore 同时挂本地 BLE session 与云端 session；不复制协议和状态仓。
- 云端产品路径必须是 `HTTP login -> SessionCredentials -> MQTT`，control 与 MQTT 分别采用产品冻结的 Plain/TLS profile；默认 Plain，TLS 是可选安全档，不能用测试固定凭证冒充产品组合或在失败后静默降级。
- ESP32 默认候选改为独立的 NimBLE-Arduino 2.5.0 adapter。旧 Bluedroid adapter只保留兼容/诊断；其完整组合放不进默认双 OTA app slot。
- UNO R4 完整组合的 static RAM 已低于 16 KiB，但 WiFiS3/ArduinoBLE 共用 connectivity modem，未过硬件并发前仍是实验能力。
- ESP32 NimBLE已补齐真实链路加密闭环；Small + compact peripheral profile配合保留双OTA的1.5 MiB app slot占81.21%、余18.79%，静态余量门禁通过。硬件并发未通过前仍不标记正式支持。

## 2. 冻结的运行结构

~~~text
                         one Client / Runtime / StateStore
                                      |
                         TransportHub (local first)
                         /                          \
          BleFrameTransport                  ManagedMqttTransport
                  |                         /                    \
        platform IBleLink          HttpControlPlane        MqttFrameTransport
                                            |                       |
                                  Arduino Client HTTP       PubSubClientAdapter
                                            |                       |
                                  TCP or TLS Client       TCP or TLS Client
~~~

关键规则：

1. `Client::begin()` 先让 BLE/local transport 可用；HTTP/MQTT `start()`只校验并暂存请求，不启动DNS/TCP/TLS，连接动作延后到`poll()`。
2. `ManagedMqttTransport::setNetworkAvailable(false)` 同时门控 HTTP 登录与 MQTT；离线启动不得尝试 DNS/TCP/TLS。
3. `TransportHub` 每轮先 poll local transport，再 poll cloud-only transport；同一优先级内部仍轮转，避免 MQTT 重连长期饿死 BLE。
4. `poll(budgetMicros)` 仍是协作预算，不是抢占式 deadline。底层 `Client::connect()` 可能超出预算，必须由探针记录真实阻塞。
5. ESP32/R4 都使用两个 composition-owned Client：一个用于控制面登录，一个用于 MQTT。每个 Client 的 Plain/TLS 类型在发布构建期确定；刷新登录时不能破坏已建立的数据连接。
6. 每条 transport/session 独立 HELLO、授权、sequence、ManifestAccept 与 ID gate；只共享权威状态和 revision。
7. ESP32 在 App 已订阅 TX 后才宣布 BLE bearer session；应用访问仍需 Noise/Method 2 建立
   DirectSecureRecord。native encrypted/bonded 若存在只能作为附加连接事实，不能替代 Controller 应用授权。

## 3. 静态组合结果

| 目标 | 完整组合 | Flash | Static RAM | 判定 |
|---|---|---:|---:|---|
| ESP32 core 3.3.1 / Standard / historical NimBLE + default slot | HTTP login + TLS MQTT + NimBLE 2.5.0 + BBP/2 | 1,299,355 / 1,310,720 B | 62,448 / 327,680 B | 历史 SMP/bonding 归因基线；不能代表当前 Port |
| ESP32 core 3.3.1 / Small / historical compact NimBLE + 1.5 MiB双OTA槽 | 同上，删除未用central/observer | 1,277,391 / 1,572,864 B | 60,648 / 327,680 B | 历史构建；当前 dual 必须重测 |
| XIAO ESP32-S3 / current `BleQuickStart` | durable Presence state-selected Direct + Noise/Method 2/DirectSecure | 692,943 / 3,342,336 B | 38,612 / 327,680 B | 2026-08-29 clean compile；无 Presence 构建开关 |
| XIAO ESP32-S3 / current `WiFiBleQuickStart` | WiFiProv + Device V2 cloud + ESP-IDF NimBLE Direct | 1,283,647 / 3,342,336 B | 60,036 / 327,680 B | 2026-08-29 clean compile；真机 Cloud+Direct 共存门禁已通过 |
| UNO R4 WiFi core 1.5.3 / `BleQuickStart` | durable Presence state-selected Direct + ArduinoBLE 2.0.2 | 192,500 / 262,144 B | 16,976 / 32,768 B | 2026-08-29 clean compile；BLE 真机门禁已通过 |
| Wio Terminal / `BleQuickStart` | durable Presence state-selected Direct + rpcBLE | 183,728 / 507,904 B | 工具链不报告 | 2026-08-29 clean compile；平台仍为显式 preview |
| ESP32 Bluedroid 对照 | 直接凭证 TLS MQTT + Bluedroid + BBP/2 | 1,777,971 / 1,310,720 B | 65,240 / 327,680 B | app slot 超限 467,251 B |

`PubSubClientAdapter` 的 1,152 B packet buffer 在首次连接时分配，不计入 static RAM。TLS、BLE
controller/host、任务栈和底层库heap也不在static表内，因此R4的18,660 B linker余量和ESP32
的267,032 B local-variable余量都不能直接当成运行时最低可用内存。

历史同源Standard-profile build-wide归因为cloud-only 1,043,535/49,280 B、BLE-only 663,667/38,840 B、dual
1,298,971/62,440 B；共享基线408,231 B。compact feature strip保留peripheral、broadcaster、
SMP、LE Secure Connections和bonding；这些数字是历史归因结果。当前官方普通 GATT Port 已删除未使用的
SMP/bonding 生产路径，必须以重新编译结果更新发布表。正式分区
把app0/app1各设为`0x180000`、SPIFFS设为`0x0e0000`，没有采用取消OTA的Huge APP。

R4-Small 登录边界当前显式预算：192 B request target、512 B response、256 B credential arena；
HTTP adapter 为 64 B host、384 B request header、96 B header line。256 B arena 已由 Host test 锁定为
能容纳 16-byte device/user/token/owner、7-byte product 与 125-byte normalized MQTT host 的精确上限。

## 4. 暂定产品门禁

这些是工程发布门禁，不是 BBP/2 wire 规范；产品预算变化必须显式评审：

### 4.1 编译/链接

- ESP32正式4 MiB分区的两个OTA app slot各为1,572,864 B；至少保留10%即产品基线不超过1,415,577 B。Arduino CLI当前低于门禁138,186 B。不得用Huge APP隐式取消OTA；若产品需要超过917,504 B SPIFFS，必须重新评审分区/外部存储。
- UNO R4 完整产品 `.data + .bss <= 16 KiB`；当前14,108 B通过，距16 KiB门禁尚有2,276 B，物理SRAM静态余量18,660 B。
- 资源表必须同时保存 empty、core、MQTT/login、BLE、dual-transport 五个构建和 map；不能用 adapter-only 数字代替产品数字。
- 所有 profile 宏必须是 build-wide 参数；禁止只在 sketch 翻译单元定义造成 ABI 不一致。

### 4.2 运行/并发

- UNO R4 24 h 后实测 combined heap/stack headroom仍至少 8 KiB；ESP32 两次 TLS handshake和 BLE session 后 free heap最低值暂定至少 80 KiB。
- 正常在线 BLE command/ACK p99 不超过 250 ms；WiFi/MQTT 断线重连期间最长本地响应不超过 1 s，且 BLE session 不因网络重连被动断开。
- 反复执行 WiFi drop、MQTT auth refresh、BLE connect/disconnect和 32-patch burst；非故障注入阶段 fragment drop/corruption 必须为零。
- 队列满必须返回 `WouldBlock` 并计数；不得静默覆盖、丢字段或跨 session 重复执行 COMMAND。
- 先过 24 h admission，再做 72 h release soak；必须固定 UNO R4 connectivity firmware版本并记录。

## 5. 探针操作

默认构建不联网。烧录后通过 115200 串口：

- `s`：先启动 BLE，再关联 WiFi；网络可用后才开始 HTTP 登录/MQTT；
- `p`：发送一次状态变更；
- `q`：连续 32 次变更，制造 BLE/MQTT backpressure；
- `w`：断开 WiFi并触发重连；
- `m`：立即输出 loop/poll/network 阻塞、heap/stack、BLE 队列和 drop 指标。

硬件记录至少包含：core/库/firmware版本、FQBN/分区/profile、供电方式、AP信道、BLE central
型号、broker延迟，以及每 5 s `METRIC` 行。WiFi 密码、authKey和 CA 不提交仓库。

## 6. aily-builder 使用边界

本地 1.3.0 能明显加快本轮迭代，但必须满足：

1. ESP32 显式传 `--sdk-path .../esp32/3.3.1`，否则多 core 安装可能产生非确定选择；
2. NimBLE 2.5.0 增加 build macro `NIMBLE_CPP_ARDUINO_STRING_AVAILABLE=1`，绕过 builder依赖分析器不支持的 parenthesized `__has_include`；官方依赖源码不修改；
3. 真实`BLINKER_RESOURCE_PROFILE=1`与compact profile的七个NimBLE宏必须用`--build-macros`逐个传入；当前工具把依赖分析与prebuild hook并行执行，日志虽显示复制sketch `build_opt.h`，最终响应文件仍可能被置为0 B；
4. 工具会生成/合并custom `partitions.csv`，但size输出仍使用FQBN default 1,310,720 B上限；门禁必须读取实际app slot 1,572,864 B；
5. R4干净测量必须使用全新build path和`--jobs 1 --no-archive-cloud-cache`；本机四并发曾生成全零头`core.a`，增量复用已再次复现遗留`core/main.cpp.o`导致重复`main`；
6. Arduino CLI/正式 CI 保留为独立发布验证，aily 不能成为唯一编译 oracle。

## 7. 下一子阶段

1. Arduino CLI已独立复现ESP32 Small/compact/custom双OTA并按ELF+有效分区通过门禁；UNO R4独立构建结果记录在主线进度中。正式CI继续固化这两条命令。
2. P1-4b等待ESP32与UNO R4实板运行本文件4.2的admission matrix；未完成前不决定Local WebSocket。
3. 无硬件期间继续P2的软件门禁、平台编译矩阵、打包边界和可审查checkpoint；App/服务器问题只写入`APP_SERVER_BBP2_REFACTOR_INPUT.md`，本阶段不修改两侧代码。

## 8. 真实station adapter软件门禁更新（2026-08-15）

ESP32与UNO R4 product probe已不再直接调用`WiFi.begin/status/disconnect`，而是分别注入独立`IWifiStation` adapter。
最终Arduino CLI clean结果：ESP32 1,288,083 B Flash / 60,720 B static，按实际1.5 MiB app slot余
284,781 B（18.11%）；UNO R4 148,400 B / 14,208 B，扣除static后仅余18,560 B给heap/stack。

该结果只关闭“平台adapter可与现有BLE/MQTT/HTTP对象图链接”的软件门禁。probe仍为legacy authKey资源探针，
`device-cloud-session/2`、正式v2 orchestrator及本文件4.2的真实AP/掉电/backpressure/loop latency/24 h门禁均未关闭。
UNO R4还必须专测WiFi协处理器无响应时单次同步事务的最长阻塞；ESP32必须记录netif未启动时`begin()`的约1 s有界等待。
