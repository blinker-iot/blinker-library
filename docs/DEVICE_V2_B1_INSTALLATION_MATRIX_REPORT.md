# B1：单 ZIP 安装、默认示例与平台矩阵（2026-09-13）

> **2026-09-22 执行入口：[新进度与后续计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md) §6。** B1/C3 基础矩阵及缓存小包已收口，不包含新增定时/自动化。SDK 来源以 C4 §4 后续官方归档比对结论为准，下文早期待测/来源不确定性保留为历史。

2026-09-14 更新：本页为历史 B1 制品证据。当前执行顺序以 [核心稳定主线](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md) 为准，当前源码的 ZIP/矩阵见 [C3 报告](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md)；不要用下文旧 ZIP 摘要代表当前源码。

当前执行入口：[基础能力路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)。B2b-13 混合授权主路径已完成，
本包不重复改根/解绑或扩网关；只处理 B1 发布边界、安装编译及基本接入证据。

最新 B1b-3 见 §6：COM11 同 ZIP 的默认明文普通 WiFi 登录/控制/重启及租约采样已真机通过，
并恢复静默 QuickStart；R4/Wio 当前硬件接入及干净 SDK/CI 边界仍单列。§4～5 的未部署/未刷板为历史。
下面 §1～3 是已通过的 B1a 安装证据，不与后续真机计数混算。

## 1. 本包修正范围

- 发布器仍只允许 16 个编译单元，漏掉已有且关闭默认宏的 `BlinkerEsp32LocalWebSocket.cpp`，导致
  `--check-only` 直接失败。按实际分层更新为 17 个，并在正式模式 ELF 中禁止未选 LAN/网关类。
  不为绕过校验删除有消费者的 LAN 代码，也不把可选候选默认打开。
  架构检查复用发布器的编译单元合同，删除第二份常量清单，避免再次出现“一边 16、一边 17”的漂移。
- B2b-12 的 HTTP 传输截断重试、MQTT Backoff/socket 错误透传已通过 host/真机，但冻结清单未更新。
  复核实现与前批证据后只更新四个对应摘要为 v37，保留 wire/golden/Key/proof 及其他摘要；没有取消冻结校验。
- 发布范围仍是 5 个根文件＋`src`＋4 个公开示例；新增源文件扩展名边界，构建产物不能随 `src/examples`
  悄悄进入 ZIP。`docs/tests/tools`、私有配置/真机证据和原根目录 `.obj` 不进入发布清单。
- 安装门禁不再只信 SHA marker：核对实际 ZIP 文件内容及额外文件，拒绝工作树源码和头文件路径混入。
  可从既有库缓存离线复制固定依赖，仍查版本/安装路径；这只是维护者加速，不增加用户 Python 前置。
- 修正测试 README 的旧支持矩阵：Wio BLE preview；ESP32 WiFi+BLE 有公开入口，但取决于 IDF NimBLE
  SDK 能力。没有新增 MCU API、凭据槽、产品模式或 App/Service 生产代码。
- 修正 CI 的独立缺口：Wio preview 在新 runner 尚未安装其三个固定依赖就先编译；补显式安装步骤。
  此步骤不绕过正式 ZIP 的独立 sketchbook 安装，也不把 preview 变成正式发布能力；本批未触发远端 CI。

## 2. 验证方法与证据

使用 development profile（HTTP/MQTT 默认明文、占位 host/测试公钥），只验证编译，不连接线上。
同一个 `BlinkerDeviceV2-0.1.0.zip` 安装到全新 sketchbook；实际编译 `examples` 中原文件，无 Sketch 补丁、
私有 `ClientStorage`/composition 参数或源码目录旁路。SDK 与依赖已安装，不在线升级 SDK，也不刷写开发板。
依赖从本机缓存复制并校验版本，未在本批重新下载或逐字节对照上游依赖归档；因此这是本机安装矩阵，
不是“任意原版 SDK/依赖组合均通过”。固定上游包的全新 CI 环境仍须另验，不能用本机缓存结果替代。

维护者检查：release/profile/architecture、Python 包边界回归、安装字节/provenance、编译资源与 map/ELF。
详细产物在 ignored `build/b1-package-20260913/`；不暂存/上传 `.zip`/SDK引用/日志。

同一 development profile 两次独立 stage/打包，以及全部编译后的当前源码重建，三份 ZIP 字节一致，373 个文件，SHA256：
`8336a69880861a173a224706c2177293b0105c19e411d04b6dbf799ca923f9c1`。
包边界回归 11/11、现有 Debug host 58/58、release/profile/architecture 检查通过；本批没有另跑 sanitizer 全矩阵。
安装后的字节、provenance、依赖闭包与 ELF 使用本批最终检查器再次只读核对，不依赖早期进程加载的脚本版本。

ESP32-S3 证据来自本机 3.3.11 NimBLE SDK：`CONFIG_BT_NIMBLE_ENABLED=y`、host stack 5120 B、
IPC stack 2048 B，sdkconfig SHA256 `534f3457aaf3ffbc82c45dc21d6b3ac57388c82d0d9a24bd2b8e6cadc3e48d9d`。
这不证明任意 stock SDK 配置等价，也不替代云端 CI 或任务栈真机水位；本批不改 SDK、不放松 SDK 准入。

本机七目标安装编译、实际依赖集合、provenance 和 map/ELF 检查均通过；不是本批真机结果：

| 目标 | SDK | 原公开示例 | program / 静态 RAM（B） | clean 秒 |
| --- | --- | --- | --- | --- |
| ESP32 WiFi | 本机 Arduino 3.3.6 / ESP32 | WiFiQuickStart | 992,767 / 50,904 | 1,390.5 |
| ESP32 云端 smoke | 同上 | CloudSmokeTest | 1,008,303 / 50,880 | 1,780.0 |
| UNO R4 WiFi | Renesas UNO 1.5.3 | WiFiQuickStart | 138,128 / 13,044 | 1,011.4 |
| UNO R4 BLE | 同上；ArduinoBLE 2.0.2 / Crypto 0.4.0 | BleQuickStart | 193,628 / 16,960 | 2,321.3 |
| Wio Terminal WiFi | Seeed SAMD 1.8.5 / rpcWiFi 1.1.0 | WiFiQuickStart | 142,224 / 68,064，见下述 ELF 口径 | 1,814.8 |
| XIAO ESP32-S3 BLE | 本机 Arduino 3.3.11 / IDF NimBLE | BleQuickStart | 692,008 / 34,848 | 329.4 |
| XIAO ESP32-S3 WiFi+BLE | 同上 | WiFiBleQuickStart | 1,281,176 / 55,516 | 735.2 |

静态 RAM 不是设备运行峰值；SDK 动态堆、任务栈、socket/TLS 缓冲和用户数据须在 B1b 真机分别记录。
`--clean` 本机耗时也不是设备握手时延，不据编译缓慢增加 MCU 缓冲或继续合并不相关模块。
Wio Core 1.8.5 的 size recipe 只报告 `.text=141,488 B`，不打印 RAM；补用同工具链 ELF size 得到
`.data=736 B`、`.bss=67,328 B`，所以静态 RAM 为 68,064 B、Flash 载荷为 142,224 B。`.bss` 内的
SDK/RTOS 静态预留同样占 MCU SRAM，不能扣掉后再与其他平台比较，也不把 ELF 调试段算作 Flash。

各组使用 `--jobs 3` 并行推进，各目标仍是独立目录的 `--clean`。ESP32 普通组完成两项后停止重复 R4 任务，
R4 组完成两项后停止已被独立 Wio 组覆盖的任务；早期 SDK 视图的重复尝试同样未计入正式结果。
这些驱动日志中的主动终止不是有效样本的源码编译失败，也不额外计为通过。
以七个目标完成行及最终 `verified-summary.json` 的实际 ZIP/编译/链接复核为准；所有构建进程已收尾。
冷构建耗时较长，尚未据此宣布构建时间预算达标；没有修改 SDK、停止用户其他进程或降低验证条件。

## 3. B1 尚未完成及后续顺序

1. 本机 B1a 七目标矩阵已完成，不重复整批编译。上游固定包的全新 CI、冷构建时间预算仍是独立发布门禁，
   不以本机缓存和较长但成功的编译宣布生产发布完成。
2. 然后是具备真实本机服务配置的默认明文 WiFi 公共示例，注册→Manifest/State→Property/Action→
   普通重启与退出回收；仅改维护者 profile 和用户正常输入的 Key/网络。不能把上批 TLS 配置副本或本批
   `.invalid` development ZIP 的编译称作本次默认明文真机通过。
   `WiFiQuickStart` 验证普通静默接入/Property/Action；`CloudSmokeTest` 另验按租约 `onSample()`，
   它显式开启同步 `Print` 调试，需保持串口读取，不能据此把所有 Print 后端宣称为非阻塞。
3. UNO R4/Wio 当前版本真机接入矩阵按实际接入的板补证；本批现场三块 XIAO，不要求为编译而清其授权。
   Wio BLE preview/发布长稳不阻塞其他平台普通产品。
4. B1 基本接入收口后 B3 可选跨平台授时→B4 LAN 产品化。首次分配中断窗口、独立本地授权/B2c/未测
   故障组合保持原门禁，不以本包编译通过宣布整个库或全部 B2 已完成。

## 4. B1b-1：默认明文真机前置修正（2026-09-13）

本批现场只读确认：PC 为“带翅膀的蜗牛”/192.168.2.76，ADB 196f65f9 和 COM3/11/12 在线；
App 原账户 ready、15 个设备，服务健康。未沿用旧 SSID，也未创建新设备、提取用户令牌或清授权。
COM11 上批已空根待配网；COM3/COM12 继续保护，不为本批编译重置它们。

实际阻断在 Service：主入口 `mqtts` 的全局判断拒绝普通设备的 plain session，且 config 仍含
`&& !productionEnvironment`。这与已冻结的普通设备默认明文、可选 TLS 政策不符；不是 Arduino 用户代码错误，
也不应给 Sketch 新加内部配置绕过。正式修订见[Service 共存方案](../../aily-blinker-services/docs/DEVICE_V2_PLAIN_TLS_MQTT_COEXISTENCE.md)。

已完成：

- 删除原唯一 expectedSecurity 判定，按 proof 已绑定的 requestedSecurity 选择已启用的对应 endpoint。
  保留原 challenge/session wire、exact replay、Broker verifier 注册、Key 撤销和会话刷新机制。
- 同一 Gateway 新增默认关闭的独立 plain TCP listener；主/附加 listener 共享接受器与原生产连接生命周期，
  首次/后端建连后复核都使用服务端固定 ingress。App JWT 不可进入附加口，Edge Hub 签发/连接均仍强制 TLS。
- 明文按明确配置允许，不再按 production 标识暗中关闭；生产 Key 管理/KEK 准入未放宽。非法/重叠端口和
  未授权/不完整配置 fail closed。同步 README/deployment，未增加 MCU/App 源码、依赖、RAM 或接口。
- 新专项接入正式 public-endpoint 测试命令；删除测试对间接 mqtt-packet 包的依赖，仅保留短 CONNECT fixture。

验证边界：

- Session/入口专项 **25/25**：主 TLS/主 plain/附加 plain 的 HMAC/原响应重放、Broker 注册与恢复；
  更改 requestedSecurity 不通过 proof；真实 TCP fixture 的两口隔离、禁用/撤销/Edge Hub/App 拒绝及 socket 回收。
  session repository/Broker 为测试依赖，不冒称真实 Broker/MCU/TLS 握手完成。
- 原公开 endpoint、账户 MQTT WebSocket、credential-security 检查通过；Gateway 类型/构建通过。
- 六个既有模块的隔离 PG 回归 **21/21，0 skipped**（含父项，不是 21 种独立故障）：HTTP wiring、repository、
  owner removal、invalidation worker、Direct access、Edge Gateway retirement。每个测试只改其新建隔离 schema，
  由原 helper 清理；真实业务 schema/授权未改。输出位于 ignored `build/b1b-plain-20260913/`。

部署检查另发现：当前 `18884→gateway:8883` 以 `0.0.0.0` 发布，虽称 TLS 主入口，内部仍是裸 TCP。
现有 TLS proxy 实际连接 Docker 内的 `gateway:8883`，不依赖这个公网映射。**下一次部署应删除或仅绑定回环该映射**，
账户 WS 后端同样复核；不能以仅声明 mqtts 等价于完成 TLS。未在本批单独改运行中的网络映射。

下一包顺序固定：保存/备份真实部署→复核差分模块并部署唯一 Gateway、隔离 TLS 后端/开放独立 plain 和受限
challenge/session HTTP→验证旧 TLS/账户恢复→真实 profile 同 ZIP/COM11 公开 QuickStart→CloudSmokeTest 租约采样。
普通重启、退出回收、R4/Wio 当前硬件证据仍待补。本批无部署、刷板、设备注册/删除、App 更新、暂存/提交/推送。

## 5. B1b-2：受控部署与真实 profile 双示例安装编译（2026-09-13）

本节优先于 §4 的“未部署/下一包”。范围仍是默认普通 WiFi 基础接入，不扩 BLE/网关/百轮或授时。
现场 PC 为“带翅膀的蜗牛”/192.168.2.76，手机位于同网段 192.168.2.31；COM11 精确 MAC
`8c:bf:ea:1b:96:14`。COM3/COM12 为保护见证，未刷板、重置或更改它们的 Key。

### 5.1 唯一 Gateway 与设备专用入口

- 保存运行中镜像/配置/六模块差分，Gateway 数据库备份 2,665,227 B，PGDMP 头和 pg_restore TOC 校验通过。
  本批不迁移 schema（仍 38），未做新的完整恢复演练；不把 TOC 校验当作恢复演练。
- 仅将 Gateway 更新为 `blinker-gateway:b1b-plain-20260913`，六模块运行哈希及健康检查通过。
  Broker/Auth/Postgres/原 TLS proxy 均未重启，不复制第二个 Gateway/worker/状态库。
- 删除原主 MQTT 8883、账户 WS 8884 的宿主机裸 TCP 映射。TLS proxy 仍经 Docker 内网访问它们，
  原 HTTPS/MQTTS/WSS 公共入口不改；原 Gateway HTTP 仅绑定宿主机回环。
- 新 device-plain 为 `192.168.2.76:18884 → gateway:1883`；同网络内受限 HTTP proxy 为
  `192.168.2.76:17002`，只转发精确 POST challenge/session 两个原接口，不新增登录协议。
  复用既有有界 HTTP 转发代码，去掉账户 Authorization/Cookie 与来访转发头，只断言 `http`，
  不伪造 TLS；其他路径/方法拒绝。代理无证书挂载、只读、去 capabilities、内存/连接/请求时限有界。
- 本机实测 HTTPS `/healthz`=200，plain 管理请求=404，畸形 CBOR challenge/session=400（各 9 B），
  主 TLS/WS 裸 TCP 已不对宿主机发布。这里验证的是入口，不是普通设备成功登录。
- 原 App 账户正常恢复；COM3 真实 Manifest/State/Property ACK 部署前 108 ms、后 104 ms，测试观察者已释放。
  Gateway 重启会令连接重建，**不宣称同一 socket 跨部署存活**；账户登录策略和 TLS 设备 Key 未变。

服务验证：本批重跑 session/入口 **25/25**、HTTP/TLS 代理 **6/6**，Gateway 类型/编译通过。
§4 隔离 PG **21/21** 为前置已通过结果，本节未再次重跑。工具真实转发/限时用例包含 TLS 及 plain 不可冒充 TLS；
它们不能替代 MCU 或网络攻击的完整验收。

### 5.2 App 普通建号与真实 ZIP

通过现有 App 普通 Key 建号页面创建 **B1b Plain COM11**，库存 15→16；测试逻辑 ID 为
`device_5f9d3415-8395-4538-bad6-1c2bc5bb28d7`，Key version 1。固定操作检查点防止重试重复建号；
没有伪造用户令牌或直接写业务库。旧 COM11 已退休 ID 不复活。DeviceKey/WiFi/部署凭据只在 ignored 私有测试材料中，
不写入正式示例、文档或发布包。

维护者真实 profile 使用本机可达地址和运行中 Gateway 对应公钥，HTTP/MQTT 均显式 plain；
这是局域网 development 联调 profile，不冒称生产安全配置。生成一个 373 文件 ZIP，独立 `lib install --zip-path`
安装后，仅在 ignored 示例副本填写 Key/SSID/password；没有 internal composition、强制 include 或工作区源码旁路。

ZIP SHA-256：`d2d638df2209ba2b924bebb78c0fa10d1f083f645a93813cf8efa32cb19f9a7d`。
目标 `esp32:esp32:XIAO_ESP32S3`，本机 Arduino 3.3.11 **已明确记录配置的 IDF NimBLE SDK**，不是 stock SDK 兼容性声明。
两次并行冷编译耗时约 759/749 s，仅为本机记录，不设置用户时间保证。

| 公开示例 | 编译 Flash | 静态 RAM | 镜像文件大小 | 编译/来源/链接检查 |
| --- | ---: | ---: | ---: | --- |
| WiFiQuickStart | 941,788 B | 51,280 B | 941,936 B | 通过 |
| CloudSmokeTest | 943,908 B | 51,256 B | 944,064 B | 通过 |

RAM 为静态链接统计，**不是运行峰值/任务栈余量**；镜像文件大小包含封装，不能混同编译 Flash 统计。
两个构建都只编译链接 BlinkerDeviceV2、PubSubClient、WiFi、Network；安装字节、ZIP/Sketch 哈希、来源隔离和
map/ELF 未选 LAN/BLE/网关闭包检查通过。固件 SHA-256：

- QuickStart：`ba0eddb602725e1f3edda2934c3a6febfcdd5d37a6e9112f277517e87fcf0cd1`。
- CloudSmokeTest：`c137aeefc57e9d56339d5c69a013bf824407d46ab7ca03954d37e92993224f3a`。

COM11 当前应用区已只读备份并与上批实际烧录镜像哈希相同；读取期间发生普通复位，**没有写 Flash/分区/NVS 或清授权**。
尚未烧录上述两个镜像，因此不记录它们的登录、控制、采样或重启通过。

### 5.3 当前阻断与续接点

电脑本机可访问新入口，但 ADB 手机访问 `192.168.2.76:17002` TCP 超时；现有 Windows 测试规则未包含
17002/18884，执行器非管理员。已请求管理员仅放行本地子网，**仍需放行后实际复验**，不能仅凭规则缺失断言
所有问题都来自防火墙，也不通过代理隧道/内部 API 绕过设备应使用的真实 LAN。

```powershell
New-NetFirewallRule -DisplayName "Blinker B1b Plain Gate" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 17002,18884 -RemoteAddress LocalSubnet -Profile Any
```

下一步复用现有部署/建号/已校验 ZIP，不再冷编译或创建第二个设备：

1. 确认两口真实 LAN 可达和 App 原账户 ready；重新核对 COM11 MAC，只写应用区，保留分区及 NVS。
2. 静默 QuickStart：普通 DeviceKey session/MQTT、Hello/Manifest/State、Property/Action、普通重启与观察者回收。
3. 同包 CloudSmokeTest：保持串口读取，用生产 `onSample()` 租约启动→页面暂停/恢复→关闭，
   联合设备采样计数核实停止发送，不能只看 App 不再显示就声称 MCU 停发。
4. 恢复静默 QuickStart，记录当前固件/资源/结果；R4/Wio 新硬件证据仍单列，再按基础路线 B3→B4。

证据在 ignored `build/b1b-plain-20260913/`（含敏感私有材料，不整体暂存）。本批仅修改受限代理工具/测试及文档，
未改 MCU/App 生产代码；无 App 安装、设备 Flash 写入、暂存/提交/推送。

## 6. B1b-3：默认明文公开示例与租约采样真机闭环（2026-09-13）

### 6.1 边界与结果

用户已添加本地子网防火墙规则，复核 Enabled/Inbound/Allow。手机真实 LAN 请求 :17002 返回预期的
404；:18884 的无凭据 MQTT CONNECT 返回 `20 02 00 05`（拒绝准入），不是端口超时。
沿用 §5 已部署服务、同 ZIP/编译固件/专用 App 设备，不重复部署、迁移、建号、轮换 Key 或冷编译。

仅对精确 COM11 `8c:bf:ea:1b:96:14` 执行三次应用区写入：QuickStart→CloudSmokeTest→恢复 QuickStart，
每次先核对物理 MAC；没有重写分区表或擦除 NVS，没有执行清除绑定/授权，也没有操作 COM3/COM12 的 Flash。
固件正常保存网络配置/读取实例仍按原实现运行；“保留 NVS”不表示运行期间完全不写存储。两种固件 hash 与 §5 一致。

| 门禁 | 本批实测 |
| --- | --- |
| 静默 QuickStart 首次接入 | Hello/Manifest/State ready；Property ACK 187/347 ms，Action ACK 192 ms，buttonPresses 0→1 |
| QuickStart 普通重启 | 同设备、Key、实例恢复；状态重新初始化，Property 169 ms，Action 141 ms |
| CloudSmokeTest | 新 Manifest 被接受；实时租约样本与设备计数联合通过，详见下节 |
| CloudSmokeTest 普通重启 | 设备 uptime 1,862 ms 时 cloud=1；Property 237 ms；无租约三个诊断点均为 0 |
| 恢复静默 QuickStart | 原 Manifest 恢复，Property 135 ms、Action 190 ms；最终 power=true、buttonPresses=1 |
| 观察者退出 | 四个有界控制窗口均 release，stateFresh=false；没有留下测试 State/Telemetry 观察者 |
| 原 TLS COM3 见证 | 最终 Property ACK 131/93/163 ms，真实 State true→false→true；Key/固件保持 |

所有命令均由已登录手机 App 的生产 DeviceV2 服务，经原账户 MQTT→Gateway/Broker→物理 MCU 发送；
没有用设备 MQTT 凭据替代账户、另开测试用户 MQTT 或在 SQL 中伪造 ACK。每个有界门禁中的账户 session 对象保持，
不据此推断两个门禁之间或跨此前部署的 socket 从未重建。CDP 调用的是生产服务，不是本批重新操作所有 UI 按钮。

只读数据库事务记录五次 COM11 登录均为 `mqtt_security=plain`、Key version 1、同实例
`4b3efcd46ff96acc1908f1affefda192`；保护 COM3/COM12 的近期记录仍为 TLS（version 1/2）。
未导出 MQTT 口令/Key/用户令牌。session 行是登录证据，不单独代表实时在线；在线和控制另由上表举证。

QuickStart Manifest 为 `b4194d759d911b76fc56ef755ca4d6ef66e76b87d1f892007e8ee357d9d04a55`；
CloudSmokeTest 为 `e428a5be2f5da3a875c058a5907bb2e9f9ba5649cba6461fe18d855df5486a0b`。
同一逻辑设备更换这两个公开固件后均取得当前 Manifest/State，不靠删除页面或重新注册掩盖版本变化。

### 6.2 onSample 的 App/MCU 联合证据

通过现有 `openTelemetry(['uptimeSeconds'], 1000, {leaseMs:15000})`、`setVisible()` 和 `close()`，
串口持续读取公开示例的同步 Print 诊断：

1. State ready 后不申请实时租约，等待 11 s，设备计数 0。
2. 申请租约、采样 6.5 s，App 收到 7 个递增样本。
3. setVisible(false)，等待 11 s，无新增 App 样本；对应设备诊断计数为 7。
4. setVisible(true)，再采样 6.5 s，新增 7 个样本，共 14。
5. close，App 11 s 无新增；设备后续三个约 10 s 间隔诊断点保持 14，telemetryDropped=0。
6. 普通重启，三个约 10 s 间隔诊断点均为 0、cloud=1，同时可下发 Property。

联合自动检查通过，非仅凭 App 过滤后不显示数据判定 MCU 停发。没有改变示例的 onSample/run 模型，
没有添加 loop 无节制发送。此处是主动暂停/关闭及重启验证，**不是租约自然到期、手机杀进程或真实页面后台门禁**；
`leaseExpired=0` 符合本批主动关闭，不能拿它证明自然回收。没有对手机/电脑时钟校准，不据诊断时间戳声称毫秒级停发延迟。

### 6.3 续接点

本工作包的 **ESP32-S3 默认明文普通 WiFi 基础链已通过**，不是整个 B1/全部平台发布完成。
COM11 当前保留已校验静默 QuickStart；手机回到原账户设备列表，测试兴趣已释放。无生产代码问题被本批复测证实，
不为凑改动新加协议/依赖或改写既有实现；本批仅补 ignored 证据工具与正式文档，无暂存/提交/推送。

下一包 B3-0 先冻结普通产品的最小授时合同/查询接口与来源边界，再进入核心实现/短闭环，之后 B4。
源码复核发现网关授权有效期已消费 `IClock::unixTime()`，故业务 UTC 不能直接替换所有现有安全时钟；
修订在[基础路线 §4](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md#4-授时wifi与ble应共用什么)。
R4/Wio 当前硬件、干净 SDK/CI、首次分配窗口/B2c 等按原边界单列；不重复 COM11 烧录或展开百轮。

本批证据：ignored `build/b1b-plain-20260913/hardware-summary.json`，对应 close/telemetry/serial/session 与
flashed 文件。该目录同时含前批私有 Key/配置和应用备份，不整体暂存/公开；资源数字继续引用 §5，未重新冷编译。
