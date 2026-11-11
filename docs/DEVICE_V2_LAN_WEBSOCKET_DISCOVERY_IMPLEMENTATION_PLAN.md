# Device V2 通用局域网 WebSocket 与发现实施方案

**2026-09-22：执行优先级仅以[新进度与执行计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md) §6 为准。C3/缓存小包已收口；§70.5 ACL/页面证据保留，LAN 仍为关闭默认开关的 internal/preview。下方 B4“当前/下一步”均为历史状态；新增本机任务将来可复用承载，但不自动开启抢占、全平台 LAN 或新 caller。**

当前执行 **B4-1：同 caller/self 的 scope 接替**。匹配服务/APK 正向与取消见 §70.4；**真实 ACL 变化＋延迟退休 ACK、实际页面原 MQTT 控制、有界准备结束、取消和 Observer 恢复已在 §70.5 补齐**。App 删除库存 presence 否决已就绪 LAN 的旧判断，187 项回归/真机通过，测试配置与账号已收尾。下一包定点收口新 caller/换账号、到期/tombstone/容量边界，并复核一次增权后工具 8 秒窗未 ready 的样本，再通用 LAN extension/资源/单 ZIP；不把单次约 3 秒页面就绪视为无缝或 SLA。B3 南向短闭环已结束，不重复授时、分享长循环或百轮。

2026-09-13 最新 B3-4c-4b：[南向真机与背压修正 §19](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#19-b3-4c-4b-南向真机与本地发送背压修正2026-09-13)已通过真实授时/控制/State和自然断开恢复。
仅本地 TX WouldBlock 改为 1 秒有界重试，wire/权限与 LAN 默认不变。手机接回后已补活动断开、无源真实 Error/控制及自动授时恢复，原固件控制复验与测试资源清理也通过，**现在回 B4**；不扩长循环。
B4 仍须处理 §31.3 单槽 scope 接替、通用 LAN 注入/声明平台/资源与安装矩阵。旧 GATT 周期让出仍有窗口边缘控制失败，
不得以授时通过替代无缝通信验收；持续连接/显式 Direct 让出需按共享连接方案独立审计，不能直接取消后永久占用单连接子设备。
下方带日期的“当前”为历史进度，不是本批重新开工范围。

2026-09-13 当前 **B3-4c-3**：南向可选模块、真实 Method 2 权限快照、Hello 与唯一 executor 回应已接线并通过软件门禁。
下一步 **B3-4c-4 短真机准入/闭环→B4**，不再重复实现南向策略；不扩连接/队列、不改变 LAN 默认。
本批无刷板/部署，生产 Hub 受保护存储门禁仍后置；资源及明确未验项以[授时 §15](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#15-b3-4c-3唯一南向-executor-的可选授时2026-09-13)为准。§31.3 单槽 scope 接替仍待 B4。

2026-09-13 当前 B3-4c-1/2：混合/Hub 自身可选业务时钟、共享只读快照和 responder 策略已实现，**南向尚未接线/宣告能力**。
下一步按[授时 §14](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#14-b3-4c-12混合与-hub-自身共享快照和回应策略2026-09-13)补 Method 2 权限快照、可选协商、唯一 executor 分发及队列/退役围栏，再短真机后回 B4。
本批不修改 LAN 注入/发布默认，不用安全 IClock 转授 UTC；下方 §31.3 单槽 scope 接替仍待 B4。软件/资源结果以 §14 为准，未刷板。

2026-09-13 B3-4b 真机：匹配 APK/单 ZIP 普通 ESP32 WiFi + LAN Plain 已验；仅暂停 MQTT TCP，原五分钟到期经 LAN 完成 28/40 B 授时，Property/Action 168/84 ms，原账户 MQTT/独立设备见证不变。
真实 Observer 在显式业务锚点丢失后不成为授时源、写控制被拒；没有清授权或停 Broker。COM11 已恢复原静默 WiFi 固件/Key/ID 并复验。
§31.3 的**改权限/scope 单槽边界仍存在**：原授权/15 秒保留窗口结束前新握手失败，等待后成功；不算即时接替/无缝已过，B4 必须单列处理。
本批不改源码 LAN 默认、不做 B4 全平台产品化。下一步 B3-4c 混合/Hub 自身→只读业务快照→唯一南向 executor 回应，再回 B4。
具体结果、失败记录、资源和未验项以[授时 §13](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#13-b3-4b-lan-授时真机2026-09-13)及[基础路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)为准；下方日期条目为历史。

2026-09-12 同日 B2a 更新：[owner 移除真机](DEVICE_V2_OWNER_REMOVAL_HARDWARE_REPORT.md)已完成专用普通 WiFi 删除主路径与第二物理设备隔离，LAN 本地清除仍独立返回 not_confirmed。此批不扩 LAN 产品组合、不新增 BBP 消息、不改变源码 LAN 开关；下一步按基础路线继续 B1/B2b。下方 R4 状态不因此重新打开。

2026-09-12；当前入口[基础收口路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)与[真机报告§6](DEVICE_V2_LAN_R4_HARDWARE_REPORT.md#6-第二物理设备隔离与b1普通wifi初步见证)。R4已补第二物理设备同账号并行隔离：LAN撤权/设备exact ACK/原连接关闭期间，另一普通WiFi设备持续真实读控，原账户MQTT不重连。进入B1/B2→B3→B4，不再扩R4循环或Hub/S4。LAN仍是ESP32 WiFi内部候选，源码开关关闭；不是全平台产品化。以下带旧日期的“下一步”为历史记录。

**当前决策：LAN 默认明文 BBP/2，按需开启加密；鉴权与数据加密分离，禁止静默降级。** §44已修首次账户/合法分享需求的路由准入、竞态和错误分类，10项软件及13项隔离PG runner结果通过。本机Gateway已更新；shard0旧镜像导致新账号MQTT未ready，对齐已验证的shard1镜像后真实手机新账号/原账号均ready，板未动。当前新账号route0、目标route1，正式接受分享确实返回跨分片409，无grant、邀请已撤销；与§43缺路由误报不同。下一批先冻结并补跨分片转发前置合同，再真实ACL→Retire ACK→提前关闭/恢复，不能迁移测试账号绕过。源码LAN关闭，P3未整体通过，不宣称无缝、不跑100轮、不扩网关协议。

## 1. 结论与范围

1. WebSocket 是具备 IP 网络能力的设备与 App 之间的局域网数据通道，不是 BLE 网关专属能力，也不是第二条账户 MQTT。
2. 普通 WiFi、WiFi+BLE、WiFi+BLE 网关应复用同一 LAN 模块。网关只额外注入目标授权与子设备转发；普通产品不链接网关目录、GatewayAccess 或 GATT Central。
3. 首选 mDNS/DNS-SD 发现服务，WebSocket 承载二进制 BBP/2。发现、安全会话、业务路由分别实现；不用发现报文承载 Manifest、State 或权限。
4. App 账户 MQTT 常驻；页面按需获取本地连接的使用权，最后一个本地使用者退出才关闭该连接。网关上多 child 复用一条 App→Hub LAN 会话，不为每个 child 创建 MQTT/WebSocket。
5. §10.109 只修订文档；§10.110 开始实现前置代码，保留鉴权失效安全屏障、不烧录、不清绑定。之后按 §7/9 补会话分派和共享执行，再实现最小纵向链路，不先跑 100 次交替。

本文的“普通 WiFi/direct WiFi”指设备接入路由器后由 App 直接访问其 IP，不指 Wi-Fi Alliance 的 Wi-Fi Direct/P2P 技术。WiFiProv 配网 SoftAP/BLE 与正常 LAN 数据服务也是两种职责。

## 2. 现有代码事实与必须先处理的边界

下表为 §10.109 审查快照；其中 lifecycle、清理/注册和容量断言的修正见 §9，不能将历史问题误记为仍原样存在。

| 已核对位置 | 当前事实 | 对本次实施的约束 |
| --- | --- | --- |
| `transport/IFrameTransport.h`、`TransportHub.h`、`api/Client.h` | 已支持二进制、多 transport、逐 session 来源/回包及共享设备状态 | LAN 实现现有传输接口；不新建第二套 Client/组件注册/StateArena |
| `arduino/internal/WiFiProduct.h` | DeviceKey WiFi stack 组装站点连接、HTTP、PubSubClient/MQTT；平台提供出站 Client 对象 | 传入 TCP Client 不能代替服务端 listen/accept；只增加必要的监听/发现平台适配 |
| `arduino/platform/esp32/Esp32WiFiBLEProduct.h` | DirectBleExtension 已 attach 到 cloud 使用的同一个 Client | LAN 可采用同样的组合方式，不能复制整个 WiFi Product；扩展名并不要求产生一套用户 API |
| `runtime/WifiCloudLifecycle.h` | Cloud 进入 `TransportState::Error` 时调用 `client_->end()`、`wifi_.stop()`；WiFi+BLE 外层也把 cloud Fault 上提为产品 Fault | 这是终态错误路径，不能概括成每次 MQTT 重连都会停止本地；但 LAN 接入前必须拆故障域，不能因 broker/登录路径故障停止整个 Client |
| `core/ResourceProfile.h` | SMALL 为 2 transport/2 peer/frame 256 B；STANDARD 为 3/4/1024 B | MQTT+BLE+LAN 需要 3 transport；不能为加一个槽直接全库切 STANDARD。宏影响跨编译单元 ABI，不能只在 Sketch 定义 |
| `api/Client.h` | 当前只有一个 authorization provider 和一个 controller control endpoint | 需要按 transport/session 分派现有鉴权；LAN 注册不能覆盖 BLE 鉴权，`authenticated=true` 不能代替业务权限检查 |
| `arduino/internal/ArduinoRuntime.h` | 通用 ArduinoClock 的 `unixTime()` 返回 false | 普通 WiFi 不能直接照搬 Hub 的可信时间假设；有限授权必须有接收方可验证的期限 |
| `runtime/GatewayChildRouteBridge.h` | Bridge 仍同时持有 Cloud authority、唯一 child 回调、南向序号与在途关联 | LAN 不能再构造一个 Bridge；抽取唯一 child 执行/需求模块后由不同上游共享 |

App 也不是从零开始：已有 `capacitor-mdns`、局域网发现/OTA 工具，但未接入 V2 业务通道。旧 `device.service.ts` 中仍有 `ws://IP:81`、JSON connected 握手与旧发现逻辑，不能据此宣称 V2 LAN 已存在，更不能直接恢复这些调用。

## 3. 组合、接口与生命周期

### 3.1 一个网络能力，两个可选业务目的地

```text
WiFi station / IP interface                  App
         │                                   │
         ├── Cloud MQTT adapter ───────── 账户 MQTT（经 Broker）
         │
         └── LAN extension ─────────────── 本地 WebSocket
               ├── discovery publisher       │
               ├── bounded listener/WS transport
               ├── session authentication + authorization
               └── target dispatcher
                    ├── self → 同一个设备 Client / Manifest / State
                    └── child（仅网关注入）→ 唯一 child executor → BLE / 未来其他南向

WiFi+BLE 的 Direct BLE transport ───────── 同一个设备 Client
```

以上名称是设计职责，不是已经新增的类。实现时优先在现有接口上组合，不为每一行再造一个抽象工厂或公共头文件。

- **平台端口：**提供 IP 可用性/网络代际、服务端监听/accept/close 和发现注册能力。可用模板或小型 port 包装 SDK，SDK 类型留在 Arduino platform/ports 内部。
- **传输：**WebSocket adapter 实现 `IFrameTransport`，只处理帧、session、流控与断开；不认识 DeviceKey、组件、child 目录或 App 页面。
- **LAN 编排：**拥有有界连接/握手资源，引用平台、现有鉴权与业务接收端；不自行连接 WiFi、请求 MQTT 凭据或开启另一套 `Blinker.run()`。
- **安全/权限：**认证本地 peer 后签发运行时会话上下文，业务访问逐目标检查。复用密码原语和审查过的实现，不把 SDK/TLS 类型传到组件层。
- **网关路由：**复用通用 LAN 前端；只注入 self/child 目标分派及代理授权。原 `GatewayCloudMux` 是 MQTT management/route/permit 分流，不能改名当通用 LAN router。
- **执行结果：**ACK 回原 caller；序号、请求关联、未知结果与去重归业务执行模块。`RxContext` 是当前回调来源，不能保存其引用作长期授权；`SendTarget` 也只是路由地址。

用户仍使用 `BlinkerWiFi.h` / `BlinkerWiFiBLE.h`、组件、`Blinker.begin()`、`Blinker.run()`。不要求传 listener、工作区或 child executor，不增加 LAN 产品 ZIP。三种产品都以支持同一 LAN 能力为目标；是否默认启用及具体容量在资源门禁后确定，不将暂未验证的平台宣布支持。

未启用 LAN 时应不实例化其工作区、避免拉入相关实现与依赖；运行时 `enabled=false` 本身不保证减少 Flash/静态 RAM。公开可选配置只保留统一入口，具体签名在首个实际 composition 验证后冻结，不在文档虚构可编译 API。

### 3.2 故障归属

| 事件 | 应退休的内容 | 应保留的内容 |
| --- | --- | --- |
| MQTT 断开/Cloud session 错误 | 该 Cloud 会话、投影与需求 | IP 网络、其他仍合法的 LAN/BLE 会话；网关仍有其他需求的 child 执行 |
| WiFi IP 丢失/切换接口 | 旧接口上的 LAN socket、地址候选、Cloud socket | 设备组件状态；与该 IP 无关且仍合法的 BLE 会话 |
| 单个 LAN caller 断开/过期 | 该 session/授权/需求/待发送窗口 | 其他 caller、账户 MQTT 和兄弟 child |
| 收到有效的 Key 代际/ACL 撤销 | 所有被该代际或权限撤销覆盖的会话和需求 | 不受影响的身份/权限；不能借“故障隔离”保留已知失效权利 |
| SDK Host reset / 用户整体 stop 或恢复出厂 | 相应真实物理资源及受影响 authority | 不假装是单 session 故障；按现有重置合同处理 |

Cloud、LAN、BLE 状态分别报告；产品 Fault 只表示确实无法继续服务的产品级问题。监听成功、mDNS 被发现、TCP 连接、鉴权成功、BBP 协商/State Ready 是不同状态。

## 4. 发现：mDNS/DNS-SD 为主，不自建一套广播协议

DNS-SD 用 PTR 枚举服务、SRV 给出主机与端口、TXT 给少量元数据；mDNS 是其在本地链路上无需配置 DNS 服务器的常见承载。[RFC 6763](https://www.rfc-editor.org/rfc/rfc6763.html)、[RFC 6762](https://www.rfc-editor.org/rfc/rfc6762.html)。因此这里不是“mDNS 或 DNS-SD 二选一”。

### 4.1 服务发布草案

- 建议 V2 使用 `_blinker-v2._tcp.local.`，与旧 JSON `_blinker._tcp` 区分。这是项目内待冻结名称，不宣称已完成 IANA 注册。
- 端口来自 SRV，不硬编码 `81`；WebSocket 路径可用 `/bbp2`，在 TXT 中给出版本/路径/支持的安全 profile 提示。字段与端口默认值经互操作测试后统一冻结。
- TXT 设置小预算，目标不超过约 200 B；不放 Manifest、页面 JSON、状态、子设备全集、Key、grant 或 secret。TXT 预算独立于设备 BBP frame 预算。
- 设备实例发布一个服务，WiFi+BLE 不发布两个“设备”；网关只发布自身 LAN 入口，不为无 IP 的每个 BLE child 伪造一台 LAN 设备。
- 服务显示名可为 `Blinker-短标识`；与 BLE 名称均可供人识别，但逻辑设备名称仍取账户设备资料。重名、IP、MAC、mDNS instance name 都不是逻辑身份。
- 可用不含秘密的随机实例 locator 帮 App 筛候选；它只是提示，必须在认证中绑定真实 instance/Key 代际。稳定 locator 有可跟踪性，首个 profile 冻结前明确隐私取舍；不能把 BLE PresenceKey/DeviceKey 填入 TXT，不能用 MAC 替代。

发布跟随实际 LAN listener 与 IP 生命周期。设备未获 IP 不发布正常 LAN 服务；DHCP/接口变更更新地址并拒绝旧会话代际。mDNS responder 若与 OTA/Web UI 共用，借用方只管理自己的 service，不能随意 `MDNS.end()` 关闭全局 responder；平台没有局部注销 API 时须解决所有权再宣称支持共存。

### 4.2 App 消费与回退

1. 前台设备列表用共享发现服务维护候选，不为每个页面单独启动扫描，不为列表中全部设备维持控制 socket。
2. 记录 service instance、地址列表、端口、TXT 与本地网络代际；只把候选关联到已知账户身份，不自动创建设备或改绑。
3. 页面有需求时，尝试目标候选并验证服务端身份/会话权限；成功后协商 Manifest/State，再成为可选控制路径。陌生或伪造同名服务不能继承缓存权限。
4. 发现过期/removed 更新“局域网可发现”，不单凭发现缓存超时拆掉仍在交换认证数据的连接；网络真实变化则失效旧 endpoint/callback。
5. mDNS 不可用时，允许在有界时间内重试本网络缓存地址，或由用户提供地址；每次仍认证。失败保留 Cloud 路径，不做全网段端口扫描。

mDNS 默认本地链路有效，访客隔离、不同 VLAN、禁组播可能阻断。自定义 UDP 广播也不能解决 AP 隔离；目前不同时实现第二套 UDP discovery、SSDP 或云端内网地址扫描。未来跨子网由网络管理员的 DNS-SD/mDNS reflector 配置或显式地址解决，不能承诺“同 SSID 必定可直连”。

### 4.3 平台和 App 的实际接入点

| 平台 | 可用基础与建议 | 未验证事项 |
| --- | --- | --- |
| ESP32 | 当前 Arduino 3.3.11 有 NetworkServer 与 ESPmDNS；port 管理监听和服务注册 | Cloud+BLE+LAN 的堆/任务栈、共用 responder 和多个客户端；不能把 API 存在当实测 |
| UNO R4 WiFi | WiFiS3 有 WiFiServer，WiFiUDP 有 `beginMulticast`；评估 ArduinoMDNS/通用 UDP responder 的小适配 | 模组 socket 总配额、组播收发、缓冲与掉线行为；不是只需 include 就完成 |
| Wio Terminal | rpcWiFi/WiFiServer 与官方 rpcmDNS，优先评估已有 RTL RPC 能力 | 官方固件的服务注册/注销和并发资源，SAM D51 与 RTL 两端都要测；不自动新增私有固件要求 |

来源：[ESP mDNS](https://docs.espressif.com/projects/esp-protocols/mdns/docs/latest/en/index.html)、[WiFiS3 Server](https://raw.githubusercontent.com/arduino/ArduinoCore-renesas/main/libraries/WiFiS3/src/WiFiServer.h)、[WiFiS3 UDP](https://raw.githubusercontent.com/arduino/ArduinoCore-renesas/main/libraries/WiFiS3/src/WiFiUdp.cpp)、[ArduinoMDNS](https://github.com/arduino-libraries/ArduinoMDNS)、[Seeed rpcmDNS](https://github.com/Seeed-Studio/Seeed_Arduino_rpcmDNS)。这些是候选能力依据，不是本库三平台 LAN 编译/真机通过证据。

App 现有 `capacitor-mdns`：Android 使用 **JmDNS 3.6.3 + multicast lock**，iOS 使用 NetService；不是已接入 Android NsdManager。先在现有接口外统一 watch 所有权、网络代际和去重，按类型 unwatch；不使用全局 close 破坏 OTA/发现工具。原接口不暴露 network handle/TTL，不能假定已有这些能力。

- Android 原生 [NSD](https://developer.android.com/develop/connectivity/wifi/use-nsd) 可作为后续后端替换候选，保持上层发现接口，不将插件重写列为第一条业务消息的前置工程。
- 当前 App targetSdk=36。按官方规则，targetSdk 37 起的 Android 17 广泛局域网访问需要适配 `ACCESS_LOCAL_NETWORK`；当前版本及 Android 16 opt-in 测试须分别验证，不能无条件要求旧系统请求不存在的权限。[Android 官方说明](https://developer.android.com/privacy-and-security/local-network-permission)。
- iOS 已有本地网络用途说明，但 NSBonjourServices 尚无建议的 V2 类型，需要补精确服务类型及真机权限测试。直接 UDP multicast/broadcast 还涉及 multicast entitlement，不能将其当“更省事的免权限回退”。[Apple TN3179](https://developer.apple.com/documentation/technotes/tn3179-understanding-local-network-privacy)。
- 现有 `network.service.ts` 主要是 connected 布尔状态，需区分局域网链路、接口代际、外网及 MQTT。手机“WiFi 无 Internet”仍可能有 LAN；不能通过全进程绑定 WiFi 把本可经蜂窝使用的账户 MQTT 一并切断。

## 5. WebSocket 承载与安全

### 5.1 帧、依赖与资源

首个业务 profile：一个 WebSocket binary message 对应一个 BBP/2 帧或安全记录；不做 Base64/JSON 外套，不另写局域网组件协议。网关目标/结果归属使用有界、可验证的路由封装，原 BBP body 不重定义；self 与 child 的具体封装在 S3b/c 统一 golden 后冻结，不能把 MQTT topic 原样当 LAN 权限。

适配器必须处理 TCP 分段/合并和 WebSocket continuation、ping/pong/close；限制 HTTP Upgrade 长度、重组总长度、未认证握手数/时限、每 poll 工作量和待发送窗口。首版不启用 permessage-deflate。WebSocket 心跳不是订阅需求或鉴权续期，TCP/MQTT ACK 不是 Action 执行 ACK。[RFC 6455](https://www.rfc-editor.org/rfc/rfc6455.html)。

优先评估一个跨平台现成引擎的薄适配：**Links2004/arduinoWebSockets** README 列出 ESP32、UNO R4 与 Wio rpcWiFi。必须额外审查其动态分配、最大 message、慢客户端写阻塞与 continuation 处理；其 README 明确 continuation 需应用处理、WebSocketsServer 不原生提供 WSS，不能据名称误认为已有 TLS server。[项目说明](https://github.com/Links2004/arduinoWebSockets)。

这只是候选依赖，不是本批新增依赖或资源准入。若不能提供有界行为，先缩小/修正 adapter 或换引擎；不要直接引入整套 ESPAsyncWebServer、管理网页、文件系统、JSON API，也不要自行发明一个不完整的 WebSocket 协议栈。已有出站 WiFiSSLClient 不能证明平台支持入站 TLS server。

资源必须分开计量：transport/peer 表、每连接网络缓冲、安全状态、共享握手 scratch、最大帧/重组、mDNS、SDK/模组 socket、握手时栈峰值。不同传输不重复分配整套组件状态。现有 DirectSecure record 固定开销为 24 B，但这不是完整握手 RAM，也不意味着 LAN wire 已可直接复用。

普通设备先以 1 个本地 caller 测量，网关先以至多 2 个 caller/2 child 作为容量评估目标；数量不是承诺。MQTT、DeviceKey HTTP、DNS、mDNS、LAN、OTA 等要核对同时使用的 socket 配额，不能只看 MCU 剩余 RAM。资源满返回明确 Busy，不驱逐合法会话、不分配无限队列。

### 5.2 同网段不是鉴权，TLS 与局域网安全是独立决策

**§29覆盖此前“首发只准入安全记录”的决策：参考普通设备MQTT，LAN业务默认明文，用户按需开启加密。** 启用LAN与选择加密是两件事：当前LAN仍未发布、源码开关关闭；未来默认明文不代表现在自动开启监听器。

明文使用`ws`承载原BBP/2，保留身份、权限、期限、流控和业务ACK；加密优先复用现有`ws + Noise/LocalSecureRecord`可选模块，不为“加密开关”同时引入WSS、证书管理和第二套握手。两种模式共享Client、组件、Manifest/State与页面生命周期。明文不提供业务内容的保密性，也不承诺逐帧防篡改；在有恶意同网段节点的网络中不能视为安全控制，发布说明必须明确这一点。TCP/WS校验、BBP序号与Manifest fingerprint不能替代密码学完整性。

连接建立不等于鉴权：不能删除Noise后直接设`authenticated=true`，不能明文发送永久DeviceKey、账户Bearer、Controller/Gateway secret。优先评估复用当前有限Grant/挑战及HMAC原语进行短期会话证明、不启用逐帧加密的最小合同；具体模式绑定/证明字节须按§29.4先冻结并交叉测试。该合同不是当前`local-access/1`的隐式变体，HMAC鉴权成本也不能说成零。

普通设备MQTT默认明文/可选TLS不变；当前Edge Hub云端密钥交付的MQTTS要求、App获取账户/短期秘密的HTTPS要求、已实现BLE安全模式不在此次自动降级范围。若后续调整这些路径，必须分别审查传输内容。要求加密的端点在超时、证书/握手失败或资源不足时明确失败，不自动重连明文，也不能仅凭不可信mDNS TXT改变安全策略。

App复用已有受限原生WebSocket端口，保持当前网络/地址/大小/期限约束；不因允许LAN明文而扩大WebView mixed-content或全局cleartext例外。平台不支持某种模式时明确返回不支持。加密模块按需链接，关加密不应仍为Noise/X25519/GCM预留专属工作区；资源差异需实测，不能从“已关闭运行时开关”推导Flash/RAM已减少。

### 5.3 通用本地权限，网关只是目标扩展

- 基础权限绑定：LAN endpoint 的真实 instance/Key 代际、caller 会话证明、访问目标与操作权限、有效期限。普通 WiFi 目标为 self；网关可另授权 exact child/instance/epoch/topology。被分享 A 不等于获得 Hub 自身控制权或 B 的权限。
- `ControllerGrant` 是单设备凭据 Install/Rotate/Revoke 的签名变更合同，没有代理目标集/有限期限，不能改名代用。Gateway→Child secret 与长期 DeviceKey 不发给 App 充当 LAN 登录密码；不为每个家庭用户再存一份 MCU 权限表。
- 初版保持 S3 既有边界：**Cloud 在线完成首次会话授权，已认证会话有限离线继续**。暂不新增冷启动完全离线认证。普通 WiFi/LAN 的首发体验也必须说明这一点，不能说断网首次打开 App 必定能用。
- 设备生成 fresh challenge，在线签发的授权必须绑定该 challenge、接收方 boot/session 和调用方证明；接收方从自身挑战开始计单调时钟并限制响应窗口与绝对累计时长。旧授权重投、App 重连不能让期限从零重算。签名正确而缺少可信时间/新鲜挑战，不足以证明尚未过期。具体算法/字段由 S3b 冻结，处理 millis 回绕和重启。
- 在线收到有效撤权立即停止相应需求/转发；断网不能即时知道新撤权，只保证不超过原期限。重新联网先复核再续期，重启丢失 RAM 授权时 fail closed；不借网络故障绕过 Key 刷新。
- 鉴权未完成只开放有界握手与必要非秘密 Hello，不开放 Manifest/State/control；记录未认证占位超时、连接/验证速率及慢客户端上限，避免补 LAN 后再引入单连接占用 DoS。

Service 的实现应是同一会话授权服务 + self/child 权限解析，不各做一套 WiFi token 与 Gateway token 服务；传输、鉴权与后端发行接口仍解耦。

## 6. App 路径选择、设备状态与无缝边界

- 一个账户 MQTT 始终由账户连接服务拥有，不由设备页面创建/销毁。本地 LAN 与 Direct BLE 是可选路径，不改变 Broker 用户路由模型。
- 普通 WiFi 页：有效 LAN Ready 后优先本地，否则 Cloud；已有健康路径继续服务后台候选握手。WiFi+BLE 延续此前 BLE 优先意图，但只有认证/同步 Ready 的 BLE 才可用于控制；广播附近不触发先断 Cloud/LAN，已选健康路径采用稳定策略避免反复切换。
- 网关 child 页：LAN→Hub 或 Cloud→Hub 都复用同一 child 南向安全连接。选择代理路径不抢手机→child Direct；显式维护 Direct 仍需 exact 物理让出。纯 BLE 设备没有 IP，不因此获得 WebSocket。
- 页面退出立即释放自己的本地使用权；没有其他调用者时关闭该 socket。其他页面、家庭用户、自动化仍有需求时不能被清理；App 关闭 Hub LAN 不代表 Hub 必须拆 child。
- 切换是“新路径先 Ready，仅后续请求走新路径”，不是同一个 Action 两边同时发送。原在途请求等原结果或呈现 Unknown；只有业务 ledger 已确认的重试规则可以重试，不让 selector 擅自复制 Action。
- Manifest/State 缓存依 logical instance/schema/revision 合并，不按 MAC/IP/路由复制页面；某 caller 已接受 schema 不代表其他 caller 已完成协商。
- 发现、路由和状态分层：`LAN detected`、`LAN authenticated/Ready`、`BLE nearby`、`Cloud reachable` 独立记录，再投影到已有图标。网关 mDNS 可见不能让其所有 child 显示“蓝牙在附近”或 Ready。

**无缝功能仍在 S4，体验验收在 S5；S3c 只是本地路径首次可用。** 有限重叠窗口用于已存在的 Cloud 与 LAN 接入，不需要第二条账户 MQTT、不要求子设备双 BLE ACL。Cloud↔LAN 暖切换目标为不重建 child 南向连接、无固定 7～16 s 交替空档；ACK P95≤300 ms 是待测目标，不是当前性能。突然断电/接口消失有检测与重建成本，单列冷恢复。

## 7. 修正后的分批实施与清理

下表保留阶段编号和历史改动归属；**当前执行依赖以§29.4的默认明文修订为准**。S3b已经实现的加密路径保留为可选模块，不再阻塞基础明文路径；多目标/满槽无空档续期仍不排在普通WiFi首条LAN消息之前。基础权限与有限资源屏障不能通过省略加密一并取消。

| 顺序 | 内容与改动范围 | 退出证据 |
| --- | --- | --- |
| L0 / S3a-0 本批完成 | 核对三类产品、发现基础与安全/资源边界；本文件及三端主线同步 | 文档与源码事实对应，不冒充功能完成 |
| S3a-1a 已实现，见§9 | 普通WiFi/WiFi+BLE非鉴权路径故障隔离；逐peer完整退休；注册冲突/回滚保护；2/2→3/3独立容量ABI测量 | Host/ASan、架构检查；保留AuthenticationRequired关闭，不把注册保护当分派完成；默认容量不变 |
| S3a-1b 已实现，见§10 | transport/auth/control原子绑定；Runtime逐路径协商/分派/退休；BLE-only和WiFi+BLE正式接线；三槽与默认配置ABI | Host三通道安全分派和生命周期回归；真实core统一配置。软件通道假体不是LAN wire/真机；默认容量未扩大 |
| S3a-1c 已完成 | 从现有Bridge抽取唯一child executor/需求，由Cloud adapter实际消费；固定Cloud/Local来源位 | 不复制child回调、序号、在途关联；Cloud退休只影响自身投影/需求，旧Direct承诺仍履行。EdgeHub生命周期与持续策略未改 |
| S3b-1（完成） | 冻结一个安全 profile，通用 self 授权 + exact-child 扩展、挑战/期限/撤权 golden 与设备验证核心 | 普通WiFi不依赖GatewayAccess；仅分享A拒绝self/B；过期/重启/重放/篡改负测；无UTC假设偷渡 |
| S3b-2a（完成，§13） | 三端共用golden、Service签发核心/HTTP边界、App消费及过期修正 | 软件验证通过；没有真实repository或生产API接线，不冒充LAN可用 |
| S3b-2b1（完成，§14） | 持久authority/幂等期限、真实ACL/Key/拓扑事务、精确撤销outbox与容量/清理接口 | 真实PG并发/进程恢复/撤销通过；无生产schema迁移或设备送达证据 |
| S3b-2b2a（完成，§15） | 认证authority同步/精确退休/ACK wire与接收核心，挑战代际覆盖在途旧grant | 软件验证保留B既有lease；其他pending须新挑战，exact重传不重复打断；未接真实投递 |
| S3b-2b2b（部分完成） | 持久投递/Service/Broker已接；§20新增设备可选组合层、有限Hello/ACK、首次同步恢复与到期维护；剩后续控制进展/恢复和真机闭环 | 见§19退出条件和§20差额；不能将已完成部分反复列作新任务，也不能将载体accepted当作设备ACK |
| S3b-3（拆分验收） | 安全socket载体与正式App网络策略随S3c-1一起验证；同socket多目标/满槽无空档续期随S3c-2、S4完成 | 第一条self链路仍须完整鉴权/到期/撤权；不靠local-cleartext包，不提前宣称多目标或无缝 |
| S3c-1 | ESP32 普通 WiFi 的共用 LAN 模块、mDNS、App V2 discovery/session；真实 Manifest/State/属性与 Action/ACK | 同一设备 Cloud+LAN 有界并存；未授权发现不能控制，保留一条账户 MQTT；不以测试 JSON server 代替正式链路 |
| S3c-2 | 同模块接 WiFi+BLE 与 Hub；Hub LAN 控制 A、Cloud 保持 B，再同 child 双 caller | 三 transport 准入、共享 child 序号/执行/背压，关一个 caller 不关其他；没有第二套网关 LAN 实现 |
| S3c-P / S3d | UNO R4 WiFi、Wio WiFi adapter 与编译/真机；有限离线、网络改变、系统权限、Cloud/局部故障；原 S2 管理交错/SDK reset 资源收口 | 每平台单独记录支持/未验；LAN→Cloud 故障域、期限/撤权、最小资源证据。R4 不顺带新增 WiFi+BLE |
| S4 | 新路径 Ready 后切未来请求；网关采用合法需求并集 + 最后 owner 离开后的有界 idle，移除旧固定占用/让出默认策略 | 兑现已发 Direct 承诺，不双发未知 Action；Cloud↔LAN 不拆仍有需求的 child |
| S5 | 1Hub+2child、普通 WiFi/WiFi+BLE 与已接入平台分层体验门禁 | 暖/冷分开、统计样本与 P95、容量/故障隔离；发布长稳单独进行，不恢复旧 100 次交替作为方案验收 |

表中批次可以进一步小步提交；S3a 的网络与 child 两处改动分别回归，不做一次“全库重写”。S3b/普通 WiFi 的工作不依赖新增 BLE 子设备绑定。ESP32 可先完成 S4 工程真机，但不能据此标记 R4/Wio LAN 发布通过。

清理与替换同批进行，不先删正常链路后堆候选：

1. App 审计调用者后移除旧 `ws://IP:81` JSON 握手、旧 LAN client 字典及废弃 Zeroconf 接线；保留页面/编辑器和仍使用的 OTA/发现工具。新业务只通过 V2 session/channel。
2. WiFi lifecycle 替换成功后删除“Cloud Fault 停整个产品”的旧接线，而不是保留两个默认入口。整体 `stop()` 和确实的全局重置语义不删除。
3. child executor 抽取后删 Bridge 内重复的南向状态拥有权；不复制 GATT session、ClientStorage、序号或无限队列。
4. 设备侧仍是标准单 Arduino 库；实现放现有 `src/BlinkerV2/{transport,runtime,security,arduino/...}` 对应层，不重建 products/integrations 源码树。不为候选引擎先增正式依赖。
5. 发布检查新增“不启用 LAN 不拉入 LAN 工作区/依赖”、全编译单元容量一致、三模式共用实现；不只检查示例能否编译。

## 8. §10.109 分析批次结果（历史）

本批只完成分析及文档修订，未改三端生产源码/插件、依赖、MCU 固件或凭据；未运行构建/真机/性能测试，未暂存/提交/推送。

尚需先冻结：安全握手与域、grant 编码/期限上限、self/child LAN 封装、发现字段/隐私、每平台容量与默认启用策略。已确定的架构不再反复讨论：**LAN 是通用网络能力；mDNS 负责发现；WebSocket 负责承载；鉴权与业务独立；网关只增加子设备目标与南向执行。**

## 9. S3a-1a：路径故障、会话退休、注册保护与容量实测（§10.110）

### 9.1 已实现及安全修正

- `WifiCloudLifecycle` 不再因普通 Cloud Error 或 station Fault 结束整个 Client；分别报告 `CloudFault/NetworkFault` 和产品 `Degraded`，继续轮询其他通道。不增加对象成员或另一套网络连接器。普通 WiFi 与 ESP32 WiFi+BLE 已通过原 composition 消费此改动。
- **不能无条件保留所有登录失败下的本地会话。** 源码确认 `DeviceKeyInvalid/DeviceKeyRotated/CredentialRevoked/SessionRevoked/DeviceRevoked` 均映射成 `AuthenticationRequired`。现阶段仍以 `AccessFault` 关闭原 Client/网络，不自动重试旧Key、不假定本地权利独立。S3b 保留原 wire 细分类并绑定本地授权 scope 后才能细化退休范围，不需发明第二套服务器错误码。
- `Degraded` 只代表路径故障且产品仍运行，不保证替代 peer Ready；诊断输出增加对应名称。整体 stop/启动失败回滚和现有 Access reset 不变。
- Runtime 将 native disconnect 与 transport 非 Online 的 peer 清理汇合：逐 `(transportId,sessionId)` 通知鉴权与 controller control endpoint，再清对应 peer。修复“无断开回调时未退休控制窗口”的遗漏，移除重复清理代码；兄弟通道状态、需求与发送仍保留。
- 单一鉴权/control注册现在拒绝不同指针覆盖，同指针幂等、停止后可显式注销。BLE-only/WiFi+BLE attach 失败只回滚本次已取得的对象，不因 AlreadyExists 删除已有 transport/provider。这是当前单入口的安全保护，**不是多通道鉴权分派已实现**。
- 登录/composition freeze 经源码审查升至 v31，仅重算两个实际修改文件；DeviceKey wire、HTTP proof、MQTT登录与默认配置均未改。

### 9.2 容量实测

使用独立 Win32 MSVC 构建树，core 与测试同编译参数；不是仅在测试头文件重新定义宏。真实 Client/Runtime/TransportHub 验证容量边界、逐transport Hello/poll 和停止，未模拟 LAN 安全握手。

| 32位ABI，SMALL且frame=256 B | transport/peer 2/2 | transport/peer 3/3 |
| --- | ---: | ---: |
| `Client` | 668 B | 748 B |
| `DeviceRuntime` | 404 B | 472 B |
| `TransportHub` | 56 B | 68 B |
| `Device`（同一SMALL配置，仅槽数不同） | 1,664 B | 1,744 B |
| 固定24-field `ClientStorage<24,256,192,32>` | 824 B | 824 B |
| `WifiCloudLifecycle` | 20 B | 20 B |

Client 已包含 Runtime/TransportHub，Device 已包含 Client，表格各行**不能相加**。三槽结构增量为 80 B，不是完整 WebSocket/mDNS/加密/SDK socket 的成本；仍欠真实 MCU 资源门禁。

原 Device “默认profile”断言遗漏transport/peer条件，会把自定义3/3误判为默认2/2超标；已限定精确默认容量，原默认上限不放宽，并在三槽ABI测试中加1744 B独立上限。生产 SMALL仍2/2，未切STANDARD、未新增用户ZIP或依赖。

### 9.3 验证与后续

Host/ASan 53项通过（含确定性协议fuzz smoke）；覆盖双向路径故障后另一通道继续发送、相同sessionId的transport隔离、原生关闭/无回调关闭、不重复退休、鉴权拒绝后MQTT不再publish/connect、注册/回滚冲突、显式stop及恢复。容量测试复现方法见 `tests/host/README.md`。

架构检查、修改源码 diff whitespace 与单Arduino库发布清单检查通过（347文件，未生成发布ZIP）。Arduino平台编译结果在 GATT §10.110 统一记录；这些都不等于真机/无缝门禁。没有新增生产LAN/鉴权wire，也没有改App/Service生产代码、Key/Access/拓扑或烧录、暂存、提交、推送。

下一批 S3a-1b 的按通道鉴权/control分派与 S3a-1c 共享child执行仍未完成。EdgeHub自己的错误/南向生命周期、BLE自身Fault是否只退休本地路径、MQTT底层阻塞耗时也需分别审查，不因普通WiFi修正就宣称三类产品完全隔离或无缝。

## 10. S3a-1b：每通道绑定、协商与控制分派（§10.111）

### 10.1 实施合同与清理

本批将§9的单入口防覆盖替换为真正的按通道绑定，不保留第二套全局fallback：

1. `TransportHub`在原有槽位中按值保存两个非拥有指针（`TransportAccess`）：authorization与controller control。与transport一次添加、一次移除；移除前方槽位时，transport、访问服务和接收context同步前移。注册仍按对象身份，不用MAC/IP/sessionId充当注册所有者。
2. Runtime不再拥有全局provider/control指针。Hello能力/认证方法、交集协商、`establishesSecureTransport`、Authenticate、控制窗口/ControllerMutation/PresenceKeyMutation、queued/closed回调全部查询本transport，sessionId原样传入。无绑定不回退其他路径；未知方法在进入provider之前拒绝。
3. controller入口必须与authorization一起绑定；局部授权服务若错误接在非Local transport，Runtime在启动IO前拒绝。普通MQTT Hello不再错误声明BLE认证方法、ControllerControl或PresenceKeyControl；这是通道能力声明纠正，不改变BBP消息编码、DeviceKey协议或云端认证方式。
4. 删除Client/Runtime的`setAuthorizationProvider`和`setControllerControlEndpoint`。内部扩展通过`addTransport(transport, lifecycle, TransportAccess(&auth, &control))`接入，结束后`removeTransport(transport)`；用户`Blinker.begin()`及组件API不变。无需独立dispatcher、全局单例注册表或动态队列。
5. BLE-only lifecycle直接完成原子注册，析构在停止后移除自身binding；BleOnlyRadio删除重复的client指针、注册/析构回滚。ESP32 WiFi+BLE同样用单次绑定，detach只移除成功取得的transport。Client和Hub运行期间都拒绝注册/移除，必须先停止，防止活跃peer持有的索引被重新分配；故障退休不等于移除注册。
6. 全量启动/停止按不同provider重置一次；通道断开只通知该transport/session，不reset兄弟provider。指针为非拥有关系，transport/provider/control必须存活到解绑，方法表与安全能力在注册期间保持稳定；支持多通道context的provider才可被显式共享，不提供运行期热销毁。

上一批模糊`AuthenticationRequired`的AccessFault安全关闭仍保留；本批不是新增LAN权限，也不使BLE凭据自动适用于LAN。`TransportAccess`只是分派边界，S3b仍需安全域、有限授权及重放/期限合同。

### 10.2 回归与资源

Host/ASan 53项通过，新增三通道用例使用真实Runtime/Hub和假传输/假provider：Cloud不泄漏本地能力、两Local不同认证方法及安全建立能力、同sessionId独立权限、控制/PresenceKey分派、未知session/方法拒绝、非native故障和native断开、剩余通道继续控制、重新编号/复用无残留权限、启动失败回滚、BLE析构解绑和显式共享provider的单次reset。

旧Hub测试允许运行时remove，已改为先验证拒绝，再stop/remove；不放宽Client原有约束。原双传输业务路由测试保留，新安全测试承担逐provider/control退休断言，不再给Cloud假装挂一个Local control endpoint。

32位MSVC Release实测（每个构建树core与测试参数一致）：

| 配置 | Client | Runtime | TransportHub | Device | Device静态门禁 |
| --- | ---: | ---: | ---: | ---: | ---: |
| SMALL，2 transport/2 peer，256 B帧 | 676 B | 396 B | 72 B | 1,672 B | 1,672 B |
| SMALL，3/3，256 B帧 | 764 B | 464 B | 92 B | 1,760 B | 1,760 B |
| STANDARD，3/4，1024 B帧 | 944 B | 644 B | 92 B | 3,460 B | 3,464 B |
| LARGE，4/8，4096 B帧 | 1,888 B | 1,564 B | 116 B | 11,004 B | 11,008 B |

相对§9，默认SMALL增加8 B，三槽增加16 B；本批同SMALL从2/2到3/3差88 B。增量来自每槽两指针替代全局两指针，以及运行锁/对齐；固定24-field存储824 B、WiFi lifecycle20 B不变。Device已含Client/Runtime/Hub，表格不可相加。显式更新旧默认静态上限，不把功能增加藏在“默认大小未变”中；容量/帧/schema默认未扩大。STANDARD/LARGE门禁仍保留原有4 B余量。

真实LAN安全状态、socket、mDNS、工作区、堆/栈仍未计入；这些不是MCU资源或密码实现验收。复现方法见`tests/host/README.md`，Arduino各目标结果统一在GATT§10.111记录。SizeProbe删除重复Device数值断言，改由Device.h统一维护精确默认门禁，大小符号仍保留。架构/源码whitespace与单库发布清单检查通过（347文件，未新增源码文件或依赖），freeze经显式审查升v32。

补充交叉编译证据：ESP32 WiFi/BLE/WiFi+BLE、R4 WiFi/BLE、Wio WiFi六个产品目标，以及R4 SizeProbe通过。R4 ELF符号确认Device1672/Client676/Runtime396/固定存储824/Wifi lifecycle20/BLE lifecycle48 B；SDK仍有依赖警告，不冒充零告警、真实运行或无缝体验通过。

### 10.3 后续顺序

下一批S3a-1c只围绕唯一child执行/需求与Cloud adapter拆分，保持既有南向连接和协议状态单一所有者；完成后进入S3b，再S3c实际LAN模块。当前三通道假体不标记V2 WebSocket/Discovery已接入，原S3c/S4/S5及安全边界不变。
App/Service本批仅文档同步，不改生产代码/协议/数据库或部署；没有真机、烧录、Key/Access/绑定操作，未暂存/提交/推送。

## 11. S3a-1c：唯一 child executor 与 Cloud adapter（§10.112）

每个活动child现在由`BasicGatewayChildExecutor`唯一拥有南向回调、连接、BBP/2序号、单在途/最近重放、管理事务和
512 B共享改写workspace；`GatewayChildRouteBridge`已成为真正的Cloud adapter，只保存Cloud准入、需求租约、投影与
Cloud correlation/envelope。Cloud downlink实际提交到executor，response按owner返回后恢复Cloud sequence。

executor以固定Cloud/Local需求位表达并集，不建立队列或动态表。Cloud route/reset只释放自己的需求和pending/recent，
不清Local位、child安全会话或序号；native disconnect才清南向会话状态。revoke/renew与management直接消费executor，
不再绕过Cloud桥。新增双owner测试确认第二caller忙时被拒且不消耗序号、ACK只回原caller、重放复用原sequence。

原Bridge的一份512 B workspace迁入executor，没有复制frame buffer。ESP32 3.3.11同配置实测活动context
7,512→7,552 B（+40 B/child），两个活动槽整机BSS+88 B；Host/ASan53、架构、单库清单和EdgeHub交叉编译通过。
本批未实现LAN transport/auth/discovery或多observer事实缓存，也未改wire/公开API/SDK依赖。下一批S3b先完成一种
安全profile、通用self有限授权与child scope；S3c再纵向接mDNS+WebSocket。S4前仍不能宣称无缝。

## 12. S3b-1：有限本地授权与单一安全 profile（§10.113）

本批只冻结并实现与载体无关的授权核心，不提前绑定WebSocket、mDNS或平台SDK。唯一首发profile为
`Noise_NNpsk0_25519_AESGCM_SHA256`；复用现有无动态分配Noise实现，但使用独立
`BLINKER-LAN-ACCESS-V1` prologue，禁止与BLE setup复用安全域。

设备每次连接先发canonical CBOR challenge，包含recipient instance、DeviceKey version、随机receiver session、
随机challenge、authority revision、15 s响应窗口、300 s最大授权期和profile。Service返回canonical CBOR grant，
同时绑定caller session、self或exact-child目标、权限、拓扑代际和期限。grant不携带DeviceKey、ControllerCredential、
Gateway secret或Noise PSK；认证码通过当前DeviceKey派生的HMAC key对18项canonical transcript流式计算，不创建完整
grant scratch。Noise PSK再由DeviceKey与认证码HKDF派生，App只能从已认证HTTPS响应取得并只存RAM。

设备不依赖UTC：首次接受`authorizationId`时以接收方`millis()`锚定deadline；相同授权重连只返回剩余时间，不能续命。
challenge一次性使用；认证成功后即使scope过期或容量已满也消费challenge。DeviceKey version或authority revision提升会
撤销旧lease，回滚被拒；设备重启后RAM authority为空，旧wire不能通过新挑战。候选为2个授权目标槽（不是2个caller的任意目标集），self目标必须为空logical id且
epoch/version为0；child目标必须精确匹配logical id、instance、accessEpoch和topologyVersion。

产品只持有一个`LocalAccessAuthority<2>`，每个载体连接只持轻量`LocalAccessChallengeSession<2>`；self与Hub child
通过`ILocalAccessTargetPolicy`注入目标校验，避免复制密码、lease和执行器。32位ABI实测：lease 144 B、admission 168 B、
2槽authority 324 B、每连接challenge session 48 B；grant golden为167 B。ESP32、UNO R4、Wio Terminal SizeProbe均通过，
UNO R4整份探针静态RAM 15,660 B/32 KiB，不能把平台/既有库总量冒充本模块增量。

Host golden/重放/篡改/过期/回绕/重启/Key换代/ACL撤销/exact-child/容量负测及fuzz已加入。尚未完成的是S3b-2
Service签发与App HTTPS消费、S3b-3将授权接入实际socket会话；因此本批不宣称LAN可用或无缝切路完成。

## 13. S3b-2a：跨端有限授权核心与边界修正（§10.114）

Service已增加独立LocalAccess codec、派生/验签及可注入签发核心/HTTP handler；App只增加无UI的严格解码与HTTPS消费类。
普通WiFi self与Hub child使用同一协议和签发入口，不新增Gateway专属LAN密码/登录图。App从BLE wire抽出通用Base64URL实现，
原调用点以re-export兼容；LAN不依赖BLE协议层。移除Host内重复golden，MCU/Service/App直接消费同一公开fixture。

已修正两个安全边界：组件级受限分享不能兑换成whole-target Control；过期lease首次拒绝不再立即清槽，
保留15秒tombstone挡住第二条尚有效的旧challenge响应。tombstone占用原槽，不增144 B的lease结构；
但满槽新授权可能暂时Busy，S3b-3须补续期替换/防重放容量门禁，**不能把这段安全隔离变成正常业务强制等待窗口**。

当前全局authorityRevision提升会撤掉recipient全部旧lease，不保证只退A而保留B；精确ACL/child退役仍需独立绑定授权与需求。
一caller访问两child消耗两授权槽，两caller各访问两child需要四授权槽或明确有界策略，不能把socket容量与授权容量混为一谈。
同一WS连接内多目标授权增补/撤销尚未实现；不因每grant有PSK就为每child开一条socket。

下一批先补真实Service持久事务和精确撤权，执行合同见
[Service LAN实施计划](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)。
现有device更新时刻不是可信LAN authority revision，纯内存issuer ledger也不能跨服务重启维持原期限；因此本批handler有实现但未注册。

验证：Host/ASan54项、20,000 fuzz、Win32 ABI不变；Service协议25项和签发/HTTP12项；App19文件204项，生产/测试TS及Gateway TS通过；
设备架构与354文件单库检查通过。本批未部署/烧录/改凭据/暂存/提交/推送，未测真实PG/手机/LAN，S3c/S4/S5未完成。

## 14. S3b-2b1：持久授权与撤销事务（§10.115）

Service新增migration 027与独立repository，复用既有DeviceKey vault及目标ACL，不复制密码/权限/路由模块。
只有新鲜已验证session激活才建立LAN continuity；同instance重登不升代，新instance更新当前身份并退旧权限，不绑定MAC。
authority revision饱和时停用LAN、不回绕且不阻断MQTT登录；设备改名不改变权限代际。

固定deadline、幂等request/grant、不可变scope、临时Key清零与跨Node进程恢复已验证。真实PG测试还捕获了仅行锁配合
混合事务隔离的漏撤竞态，改为SERIALIZABLE/SSI；只在签发callback之前有界重试，签发后的提交失败不返回材料。
局部ACL/解绑写精确authorization退休与outbox，不升整个Hub代际；Key/recipient停用才做整体退休。

服务器每recipient64个活动授权、每授权64个request，提供过保留期的有界清理接口；与MCU两目标槽无关。
联合现有登录/分享/网关访问/退休和LocalAccess签发HTTP测试30项通过、0跳过，Gateway TS通过；未部署、改原数据、
刷板或运行App/MCU。本批详细表结构、保留期限和复现见[Service计划](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)§3/6。

下一批S3b-2b2须补认证revision同步及exact退休投递/ACK，并阻止“先收到撤销、后收到尚未安装的旧grant”复活。
数据库outbox不是设备已撤权；局部撤A也不能破坏B/另一caller。接收方有界退休记忆、投递重试/恢复与清理调度通过后才开HTTP。
S3b-3另补满槽续期和同WS多目标，不以15秒tombstone重建正常强制通信空档；S3c真实LAN、S4持续切路和S5体验未完成。

## 15. S3b-2b2a：认证控制接收与在途授权失效（§10.116）

`protocol/local/Control`负责固定wire，`LocalAccessSecurity`复用流式密码基础，`LocalAccessControlReceiver`只拥有设备级控制会话/序号和
最新exact消息指纹；`LocalAccessAuthority`仍唯一持有lease/权限/挑战代际。模块均不依赖MQTT、WebSocket、mDNS或平台SDK。
Hello/Command/ACK使用独立HMAC域；Enable/Disable同步权威版本，Retire只针对authorizationId。ACK完整绑定原消息，不能把命令反射成ACK。
controlSession属于本次设备运行，不随MQTT重连重建；Hello恢复不清本地lease，也不自动证明某项outbox已送达。

精确退休采用“删该lease + 使先前pending挑战失效”，不维护撤销ID表。尚未安装的延迟旧grant同样拒绝；B已建立权限和原到期时间不变，
B尚在握手则新取挑战。重复同消息只回相同ACK，不再失效新挑战。腾出的槽可以立即接**新的在线授权ID**；不能让server重新签旧已撤ID。
这不是满槽过期续期问题的完整解法：自然过期tombstone仍保留15秒，S3b-3另补无空档续期；控制消息序号/挑战代际均禁止回绕。

Host/ASan54、20,000 fuzz、Service协议26项及Gateway TS、架构/357文件单库检查通过。Win32 authority328/challenge52/control receiver84 B，
原lease144/admission168 B；两挑战配置模块ABI+96 B，不宣称整机增量或物理连续通信通过。
ESP32/UNO R4/Wio SizeProbe显式实例化receiver全部方法并交叉编译通过，三种ELF确认328/52/84 B；UNO R4整份探针RAM15,752 B。
既有SDK警告仍在，未烧录或测试运行时栈/真实控制面。
协议精确定义与后续持久发送顺序见[Service计划§7](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)。

下一批2b仍需持久单在途command/序号恢复、exact ACK落库、Broker和product入口、业务转发/需求对同一authority的消费及清理调度。
接收器成功ACK不是Action撤回/物理断链证据；目前无生产HTTP/WS/mDNS，未部署或真机测试，App生产代码未改。

## 16. S3b-2b2b1：持久发送准备与精确ACK恢复（§10.117）

Service用独立repository持有单recipient checkpoint和不可变命令日志，事务提交后才返回发送字节；同一pending永不改序号/内容。
Hello补当前云登录proofSessionId认证上下文，旧Hello不能回切启动nonce；不改登录字段，MCU对象ABI不变，不增加MQTT连接。
新boot替换pending保留abandoned记录、outbox不算完成；只有exact Retire ACK才记送达，全局屏障ACK的覆盖单独留superseded证据。
同步/退休恢复前暂停新签发，B既有lease不改；统一保留期限和有界清理不删除pending或被退休引用的完成记录。

真实PG联合34项、Host/ASan54及20,000 fuzz、协议26、Gateway TS、Win32 ABI/架构/单库清单通过；本批不重跑三MCU交叉编译/真机。
下一批2b2b2仍须持久重试预算/退避、Broker和product消费、主动到期释放及实际控制面故障门禁，不能现在注册HTTPS或称无缝完成。
之后S3b-3无空档满槽续期/同socket多目标、S3c共用WS/mDNS不变；完整合同、证据和待办以[Service计划§8](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)为准。

## 17. S3b-2b2b2：有界投递与清理核心（§10.118）

Service已实现先提交attempt再发布、每当前proof最多8次、退避/超时、迟到结果隔离、预算耗尽保留pending，以及每分钟两类32条清理。
复用共享scheduler，不建立另一条MQTT/内存重试队列；只有exact ACK完成状态，新鲜登录+Hello可重开预算，旧Hello和进程重启不能。
`claimNext`拒绝外层事务，测试第二条PG连接证实发布时已提交且未持原锁。共享worker还补批次截断和Infinity间隔保护。

真实PG联合47项、后台任务16项及Gateway TS通过；发布仍为注入测试载体，真实carrier必须在abort后结束I/O。
下一步Broker/产品接线并自动处理预算恢复，禁止把“预算耗尽”变成要求初学用户重置/改Key；再主动到期/有限离线与真机控制面门禁。
HTTPS/WS/mDNS及无缝仍未完成；本批MCU/App源码不变，无部署/刷板。唯一详细策略和剩余步骤见[Service计划§9](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)。

## 18. S3b-2b2b2：Service/Broker真实载体接线（§10.119）

原物理MQTT数据topic承载0x36控制frame，独立模块封装有界I/O；不新增MQTT，不复制Hub-only管理图或PG重试账本。
已接内部双向HTTP/可信proof和唯一start/stop，在线QoS0发送、签名ACK才完成；取消/迟到和容量有限。
PG联合50项、Broker专项7项及既有165项回归通过。MCU只有enum占位，App不改，实际网络为loopback模拟器，无部署或刷板。
设备Host/ASan54项及20,000确定性fuzz、架构/357文件发布清单通过，不重计历史交叉编译/整机资源证据。
下一步WiFi/Hub产品控制队列、登录Hello/预算自动恢复/主动到期与真机门禁；S3b-3/WS/mDNS/LAN签发尚未完成。
完整当前顺序与证据边界见[Service计划§10](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_LOCAL_ACCESS_IMPLEMENTATION_PLAN.md)。

## 19. 主线复核：先收口控制闭环，再做首条LAN业务链路（当前执行入口）

### 19.1 是否偏离

本节代码事实为§10.119结束时的复核快照；后续产品接线见§20，恢复补齐见§21，不能将以下“尚未消费”当作当前仍未接线。

目标未改变，设备接收/ACK、云Hello和自动恢复是必要前置，不应因为最近批次集中于控制面就将它们判为无用工作。
当前源码中`LocalAccessControlReceiver`已有独立验证核心，`Frame.h`只有0x36载体枚举；普通WiFi产品尚未消费该控制入口。
Service/Broker的loopback测试不能替代设备应用后的ACK。跳过此步就开放LAN，会缺少权威同步/撤权及恢复保证。

需要纠正的是**进度粒度与后续依赖**：§7原先将部分完成的大项仍写为“下一批”，且把安全socket、网关多目标和满槽续期捆成了首条LAN业务之前的大前置。
`src`中仍无V2 WebSocket listener/transport；App目前仅有LocalAccess协议/API消费模块，旧`ws://IP:81` JSON入口不能替代V2 LAN。
已有安全核心保留，不重新设计认证wire、不新增第二套控制框架；文档现在纠正，代码仍先完成正在进行的必要闭环。

### 19.2 正在进行的这一批：完成边界

以下是一组收口任务，不再将每个测试扩展为新的架构阶段：

1. **一个设备控制模块。** 普通WiFi、WiFi+BLE、Hub复用原MQTT入口；0x36回调只做有界收包，主循环验证/应用并回exact ACK。
   不抢占MQTT单回调，不复制Hub管理图；未启用LAN的产品不默认承担授权表/队列资源。产品入口须处理队列满、重复包、连接/Key代际与停止清理。
2. **Hello与自动恢复。** 当前登录提供proof，boot控制会话不因MQTT短断重建；明确首次Hello、丢失重试、重连再同步、如何判断设备端同步进展。
   预算耗尽后的新proof由库限频自动取得；没有对端进展不能用无限Hello定时器补洞，不能要求用户重置/换Key。
   先消费现有登录刷新能力；若发现现有wire缺少必要的进展证明，只修这一明确缺口，不扩展通用管理协议。
3. **同一authority的生命周期。** 主循环主动处理到期；覆盖无流量、millis回绕、Key/实例换代，普通网络短断不延长也不无故清空其他合法lease。
   业务/需求/物理socket的实际消费在LAN纵向链路再验收，此处Host通过不冒充实际断流。
4. **一次有故障注入的真实控制闭环。** 真PG→Broker→MCU→Broker→PG验证Enable/Disable/Retire的exact ACK与checkpoint。
   覆盖ACK丢失后同frame重传、重复命令不重复退休、MQTT断开/设备重启、旧proof/Key拒绝和自动恢复；精确隔离先有软件负测。
   无LAN socket时只标“控制闭环通过”，不得标“App LAN可用/撤权即时断流/无缝已完成”。不要求100次交替或24小时长稳作为此步退出条件。

### 19.3 收口后直接进入的主线

| 顺序 | 交付 | 必须看到的证据 |
| --- | --- | --- |
| 先：S3b-2b2b剩余 | §19.2设备控制与恢复闭环 | 真实设备应用ACK，不是HTTP accepted或模拟器；资源有界、停止无残留 |
| 然后：S3b-3基础 + S3c-1 | ESP32普通WiFi，1个caller/self，mDNS→安全WS→HTTPS授权/Noise→BBP Hello/Manifest/State→属性与Action/ACK | 正式App网络配置可连接；Cloud保持原MQTT；未授权拒绝、有限离线到期/在线撤权实际停止；明确记录RAM/栈/socket成本 |
| 再：S3c-2 + S4 | 同模块接WiFi+BLE/Hub；多目标/同child多caller、续期与未来请求路径选择 | 共用唯一child executor；不复制Action；暖切换不拆仍有需求的南向连接；再评估“无缝” |
| 分平台及发布 | R4/Wio LAN、平台权限/网络故障、容量与长稳 | 各平台独立验收；一个ESP32纵向通过不代表全平台发布 |

第一条LAN链路不是无鉴权测试server：未经完整认证不开放业务，生产HTTPS签发仍须满足authority同步/撤销就绪条件。
正式网络配置下的socket引擎、mDNS和缓冲上限可在控制收口期间审查，但不以候选依赖评估替代完成§19.2，也不为了测试修改全局cleartext策略。
普通WiFi单caller首测可以到期后重新建立会话，必须如实标注不连续；满槽无空档续期是无缝验收条件，不是验证第一条安全消息的前提。

本次仅复核源码事实并修正文档顺序；未修改生产代码、未新增测试通过记录、未部署或刷板。§18实现证据不重计为本次结果。

## 20. 设备控制组合层、proof与到期维护（GATT §10.120）

### 20.1 本批实现

- `HttpDeviceKeySessionProvider`保留成功登录响应的16 B `sessionId`，通过`proofSessionId()`提供只读视图；HTTP scratch清理不再丢失它。
  正在刷新时仍对应旧成功凭据，成功后一起换代；失败/stop清空。没有解析MQTT密码、改变登录wire或增加第二个登录服务。
- 可选`LocalAccessCloudTransport`包装原`ManagedMqttTransport`：非0x36帧仍交给原Runtime，Hub side-topic mux不变；不另创建MQTT、组件状态、child执行器或SDK任务。
  唯一112 B inbox在回调只复制，主循环才读取Key、验证/应用并编码ACK；按credential generation和物理connection generation丢弃跨代待处理输入。
  满槽不覆盖、不排无限队列。ACK发送失败不撤销已经应用的命令，由Service原账本重传同frame、接收器重建exact ACK，不重复退休。
- 同一boot控制nonce跨MQTT重连/同Key新proof保持，Key代际或产品stop才重置。普通断网仍轮询authority到期；等待首次登录不反复清authority/耗尽challenge generation。
- 每次云连接/成功凭据代际最多4次Hello，计划时刻0/1/3/7秒；本地发送失败同样消耗次数。收到合法Command后停Hello重试。
  **应用本地Command不等于Service已收到ACK，也不等于LAN Ready。** 首次240秒仍没有应用authority时，通过现有`requestRefresh()`自动取得新proof；尝试间隔至少300秒，不重置/改Key/擦Flash。
  240秒是设备bootstrap等待上限，不声称设备能由时间推断Service的8次预算已经用尽。已配置且正常空闲的authority不会因此周期性重登。
- `LocalAccessLeaseLedger::poll()`主动标记到期并在原15秒tombstone窗口后回收；已观察的到期不会因millis回绕复活。
  `LocalAccessChallengeSession::poll()`同样提供无流量过期维护，后续LAN session必须接入自己的poll。32位时钟仍要求主循环在一次计数周期内运行，不承诺无时钟记录下跨49天停机可恢复旧lease。
- 普通WiFi、ESP32 WiFi+BLE、Hub从同一`DeviceKeyWifiStack::CloudTransport`组装，不再把产品lifecycle硬编码为Managed类型。
  **仅内部全库构建开关`BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1`启用**；没有新增用户begin参数，也没有开启WS/mDNS。
  当前组合测量为1槽、self target；Hub共用载体不代表child本地授权已接入。未开该开关不分配authority/inbox、不链接该组合层。

### 20.2 验证与资源边界

Host/ASan 54项及20,000确定性fuzz通过；新增用真实C++登录provider/ManagedMQTT/控制接收器和假HTTP/MQTT端口的组合测试。
覆盖成功proof保留/换代/清理、SDK回调不读Key/不publish、满inbox、坏MAC/方向/flags、重复ACK、发布失败、普通数据透传、断网清队列/保留boot、Key换代、首次自动刷新及正常空闲不重登。
这些不是实际HTTP/Broker/MCU互通证据。32位Win32实测：provider296 B（原280 B，+16 B）；单槽wrapper452 B（已包含authority、receiver及inbox，不能再次相加）；lease144/双槽authority328/challenge52/receiver84 B不变。
wrapper的临时输出帧为122 B，密码函数内部栈与SDK socket成本另测；不把对象ABI当作整机栈/堆准入。
架构/单Arduino库清单通过（358文件），登录/composition freeze经审查升v33；跨MCU编译结果见本节追加记录。

### 20.3 尚未收口：后续投递进展与自动恢复

设备已配置后，“没有新的Command”可能是正常空闲，也可能是后续Retire/Disable全丢且Service预算耗尽；4次Hello全部丢失后的新proof同步也有同类问题。
当前内部HTTP只把Hello accepted返回Broker，设备看不到；因此**首次bootstrap自动恢复不能记为所有耗尽场景自动恢复完成**。

下一批优先补这一处最小、可验证的进展/恢复合同：区分Service已确认的checkpoint、待送工作和需要新proof；绑定当前proof/boot及新鲜请求证明，防止旧“已同步”响应掩盖后来撤权。
复用当前0x36载体、登录刷新接口和Service唯一账本，先核对能否只复用既有恢复能力；确实缺失时才增最小协议字段/消息，不新建通用管理框架。
**不能直接调用现有`revokeSession()`强迫重登**：migration027会同时disable本地authority并推进revision，可能把原本只应精确撤A的恢复扩大到B。
也不能靠永久Hello定时器、每隔几分钟强制所有设备重登、放宽PG预算或要求用户重置来绕过。

该缺口的软件/真实PG恢复用例通过后，再进行§19.2的真实PG→Broker→MCU闭环；随后直接进入普通WiFi LAN纵向链路。
本批没有改Service/Broker/App生产源码、部署/烧录或设备绑定；没有注册App LAN签发、完成完整自动恢复或宣称无缝通信。未暂存/提交/推送。

### 20.4 Arduino实际交叉编译（2026-09-09）

以下均为本批最终源码的成功编译，不是刷板/联网/运行时堆栈门禁。启用项全库定义`BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1`。

| 目标/例程 | Core | Flash / B | 静态RAM / B |
| --- | --- | ---: | ---: |
| XIAO ESP32-S3普通WiFi / CompilePublicWiFi | ESP32 3.3.11 | 946,088 | 51,680 |
| XIAO ESP32-S3 WiFi+BLE / WiFiBleQuickStart | ESP32 3.3.11 | 1,286,384 | 55,972 |
| XIAO ESP32-S3 Hub / CompileEsp32EdgeHubInternal | ESP32 3.3.11 | 1,456,320 | 75,700 |
| UNO R4 WiFi / CompilePublicWiFi / Small / 启用 | Renesas UNO 1.5.3 | 141,992 | 13,448 |
| UNO R4 WiFi / 同一例程与profile / 关闭 | Renesas UNO 1.5.3 | 137,328 | 12,992 |
| Wio Terminal WiFi / CompilePublicWiFi | Seeeduino SAMD 1.8.5 | 145,160 | 68,472 |

R4同配置增量为4,664 B Flash、456 B静态RAM（452 B wrapper + 4 B self target policy）；两个构建均已包含provider新增16 B proof。
关闭构建的ELF没有`LocalAccessCloudTransport`/`LocalAccessControlReceiver`/`LocalAccessAuthority`符号；启用构建有34条相关符号，证实不是只编译头文件却未链接控制路径。
Wio CLI未报告RAM，表中取实际ELF的`.data=736`与`.bss=67,736`相加；不包含运行时分配、SDK任务栈或安全握手峰值。

ESP32使用本机已安装的3.3.11 **IDF NimBLE SDK配置**（`D:/codes/.arduino-cli/esp32-sdk-3311-ab.yaml`），不是宣称所有默认/库存SDK均支持此组合；未安装或修改SDK。
CLI为1.5.1，ESP32 FQBN为`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`。普通WiFi和Hub构建产物分别位于`build/local-control-wifi-3311`、`build/local-control-3311`，WiFi+BLE位于`build/local-control-wifible-3311`。
R4/Wio启用产物位于`build/local-control-products/<target>-local-control`，R4关闭对照位于`build/local-control-disabled-r4`；R4两个构建均定义`BLINKER_RESOURCE_PROFILE=1`。
`tests/arduino/compile_matrix.py --list --group local-control`列出五个可选控制组合；该组继承既有平台Core准入版本，不将本次ESP32 3.3.11显式构建偷换为默认矩阵版本变更。
SDK/第三方库已有编译警告仍在；本批未通过复制或修改这些外部库来掩盖警告。

## 21. 新鲜进展回执与后续耗尽恢复（GATT §10.121）

### 21.1 最小职责与合同

§20.3所述缺口需要对端进展证明；仅设备收到Command、MQTT仍连接、HTTP accepted均不够。新增独立`ProgressView`，
不把查询nonce塞入authorizationId，不新增通用管理协议、MQTT连接、持久outbox、Flash记录、App UI或用户begin配置。
原Hello=3/Command=4/ACK=5合同保持，接收核心和Service仍支持其既有使用者；**可选产品现在用带回执的Query完成Hello式对齐，不再并行运行旧Hello/bootstrap定时器。**

0x36外层仍为10 B header、flags=sequence=0；Query/Status为exact canonical CBOR数组：

```text
[1, kind, recipientInstance(16 B), keyVersion, controlSession(16 B),
 requestNonce(16 B), acknowledgedSequence, syncedRevision, state, authenticator(32 B)]
kind: Query=6 / Status=7
state: Query=0 / Synchronized=1 / Pending=2 / Refresh=3
```

Query中的sequence/revision是设备已应用值；Status是PG**已收到exact ACK**的checkpoint，可能落后于设备。两者序号/修订均为uint32，
body最大104 B，仍使用原112 B inbox/122 B外层上限。独立HMAC域`BLINKER-LAN-ACCESS-PROGRESS-MAC/1`；
按既有DeviceKey+instance+keyVersion方式派生密钥，签名数组前9项，并追加当前登录的16 B proof作为固定suffix，两个方向都必须绑定。
设备还验证当前boot、单在途nonce、10秒本地期限和checkpoint不超前；Synchronized必须与设备当前sequence/revision一致。
回执一次消费，超时后失效；旧proof/nonce/方向/Key或MAC不能重放成当前同步事实。

### 21.2 调度、恢复与隔离

| 条件 | 行为及上限 |
| --- | --- |
| 初次成功登录/物理重连/新proof | 新随机nonce，原boot保持；最多在0/1/3/7秒发送4次Query，本地发送失败也计数 |
| 有效Status或10秒期限到达 | 结束这一轮，清nonce；Query/Status不产生协议ACK，也不更新PG投递次数 |
| 健康空闲 | 距上一轮开始至少60秒才发新nonce；正常每分钟一问一答，不重复登录 |
| Pending且checkpoint前进 | 记录进展；仅反复Pending且checkpoint不变不延长进展等待时间 |
| 有效Refresh | 原proof的8次预算及最后ACK窗口耗尽；库自动调用已有requestRefresh，收到新登录结果后以原boot/已应用sequence再对齐 |
| 回执全丢或checkpoint持续不前进240秒 | 同样允许requestRefresh兜底，覆盖首次同步和已配置后的失败；不是声称由本地时钟推断PG已经耗尽 |
| 任意自动刷新尝试 | 独立最短300秒间隔，跨成功proof与物理重连保留；刷新在途不并行开新查询轮，命令/ACK仍可处理 |

这是低频控制健康查询，不是不断重发Hello/重开命令预算。计时从主循环观测，调度迟滞不会补发一串积压轮次。
PG每个命令/每个proof原8次预算、累计计数、exact命令字节和唯一发送worker不变。Query与Hello共用`receiveCheckpoint`，
检查当前云身份并对齐receiver proof，但不会把设备上报的已应用sequence当ACK；新proof只使下一次`claimNext`有资格按原规则重新领取。
Service持锁读取authority/checkpoint/待送工作及退休记录，只有确无待处理工作且全局状态已对齐才返回Synchronized。
同一旧Query即使再次到达Service，也读取当前事实；设备不接受已消费的旧Status。Synchronized是有期限请求的快照，不是对未来撤权的承诺，更不是LAN授权。

进展Status直接随已有内部HTTP响应返回Broker，再投递原在线MQTT下行；不增加Service发Status的持久队列。
仍遵守原上行16总在途/每连接1个/4条每秒、下行8总在途/每连接1个和3秒write期限；Status与Command竞争既有有界槽，满则丢失，由各自已有重试处理。
HTTP迟到响应必须仍匹配捕获的**同一个物理client**，不能交给复用logicalId/proof的新socket。下行成功仍不代表设备已应用。

恢复**不调用revokeSession、不disable全局authority、不改变revision或清其他lease**。精确退休A在新proof下重传原字节、设备回原ACK；
B的状态、权限和期限不因传输恢复被扩大撤销。终态租户/Key失效、累计uint32/控制序号用尽仍按原失败关闭边界处理，不承诺自动刷新能修复权限撤销或坏数据库。

### 21.3 验证与资源

- 真实PostgreSQL及HTTP/worker联合31项通过：包括Query建receiver、旧proof拒绝、8次预算及末次ACK窗、反复查询不补预算、
  设备已应用但ACK丢失仍未完成、新proof第9次累计投递保持exact bytes、A退休/B不受影响、Key更新拒绝旧请求。
  测试使用临时runner容器、源码只读挂载和唯一隔离schema；未替换运行中的Gateway/Broker或修改账号/绑定。
- 共享协议27项与Broker专项8项联合35项通过；Broker专项及原App/网关/连接需求联合173项通过，Device V2-only脚本、两端与协议TypeScript通过。
  新增真实Aedes/TCP MQTT模拟器的Query→Status同socket返回、HTTP回执字段/长度检查和物理连接替换隔离；不是实际MCU回执。
- 设备Host/ASan54项及20,000确定性fuzz通过；共享golden覆盖Query/Sync/Refresh、proof/nonce/方向、截断/非canonical/越界；
  产品组合测试覆盖回执消费/过期/重放、同步后静默恢复及独立刷新限频。完整结果以最终测试记录为准，不将编译代替执行。
- Win32：单槽wrapper由452增至488 B（+36 B），provider296 B、receiver84 B、lease144 B、双槽authority328 B、challenge52 B不变。
  新nonce为16 B，未扩大原inbox；默认未开内部扩展仍不实例化此wrapper。
- 最终源码实际编译：ESP32-S3普通WiFi（同§20.4本机3.3.11 IDF NimBLE配置）Flash948,404 B/静态RAM51,720 B；
  UNO R4 WiFi Small（1.5.3）Flash144,120 B/静态RAM13,484 B。相对§20.4同目标增加2,316/2,128 B Flash及40/36 B静态RAM。
  本批未重编WiFi+BLE/Hub/Wio，§20.4五组合是前批历史证据；未测实际安全握手/SDK任务栈峰值。
- 架构与单Arduino库360文件清单通过；原登录wire与v33 freeze不变，没有机械重置哈希。

### 21.4 下一步与现场约束

软件恢复缺口已补，下一批直接执行§19.2第4项真实PG→Broker→MCU控制闭环；至少覆盖Enable、Disable、精确Retire、ACK丢失重传、
新proof恢复、短断/重启与旧Key拒绝，再进入普通WiFi单caller/self安全WS/mDNS→BBP业务。不要继续拆分为新的协议准备阶段或先跑100次交替。
当前仍无生产LAN listener/自动发现/公开签发，也未宣称实时撤权断流、BLE/LAN路径无缝或网关多目标续期完成。
用户本轮已确认WiFi为`mostfun2026`并私下提供密码；密码不写入本文或公开示例。只读检测到COM3/COM11/COM12，尚未读取设备固件/角色。
真机前将新参数用于本地联调配置，并重新核对开发板端口/角色和服务地址，**不得沿用历史网络参数或误刷子设备**。
本批未刷板/部署/重置设备、未暂存/提交/推送；App只同步本节边界，不改源码/UI。

## 22. 真实控制闭环与启动Query竞态修正（GATT §10.122）

### 22.1 现场、部署与职责边界

2026-09-09，实际使用`mostfun2026`，PC `192.168.1.133`、Hub DHCP `192.168.1.135`。密码只写ignored本地配置。
确认COM11为Hub（8C:BF:EA:1B:96:14），COM12为child A，COM3已经是child B，**不是备用WiFi板**。
只烧录COM11的`Esp32EdgeHubDualBoard`应用区0x10000，保留NVS/分区表/Key/A/B绑定；未改两块child固件。
联调对象是原DeviceKey WiFi stack在Hub中的self控制载体，不是普通WiFi产品或已建立的LAN业务会话。
内部探针仍显式使用测试用弱存储；不能据此宣布生产加密NVS或网关发布门禁完成。

本机Gateway/Broker shard 1更新至当前代码，migration027/028/029首次在实际开发库执行；原PG数据库、应用区、容器配置/镜像均已备份。
更新原TLS代理的IP SAN证书与服务发布地址，未降级TLS或修改Windows防火墙。证书仅为七日有效的本地联调证书，不可发布。
所有网络密码、私钥、环境快照、原始备份和联调输出在ignored `build/local-control-live-20260909/`，不进入源码发布或暂存。
`compose.json`只管理本批Gateway、shard 1、TLS代理；不得使用`--remove-orphans`误删其他开发服务。`rollback.json`保留原部署参数，旧TLS容器另行停放供回退。

### 22.2 真机发现并修正的问题

`LocalAccessCloudTransport.poll()`独立于Runtime的BBP Hello发送；首个Query可能先到。此前Broker的`current()`把
`helloComplete=false`也作为`SESSION_STALE`，publish鉴权将正常启动竞态判成严重违规，断开MQTT并触发设备级退避。
真机日志出现`LOCAL_CONTROL_SESSION_STALE`，不是NimBLE、WiFi密码、证书或新增MQTT连接的问题。

修正仅在Broker：对真实、当前、未过期/未撤销的设备MQTT身份允许**形状合法的Query**在Hello前经过publish检查；
载体先计原每连接4/s预算，然后直接丢弃，不relay Service、不分配HTTP等待槽、不发Status/Command、不设置Hello完成。
Runtime正常Hello完成后，设备现有0/1/3/7秒重试即可继续。旧Key/无效身份/App/虚拟child、其他方向及下行准入仍按原规则拒绝。
没有添加固定延时、第二套队列或让设备transport依赖Broker内部会话状态。

同时修复内部Hub探针过期诊断调用：`bridge.controllerControlPending()`改为直接读取`executor.controllerControlPending()`。
队列事实只归Executor所有，不复活旧Bridge转发接口。

### 22.3 实测结果（不是模拟器ACK）

| 项目 | 实际证据 | 结论/范围 |
| --- | --- | --- |
| 初次同步 | Query建立PG receiver，Enable得到MCU exact ACK | 通过；无需Manifest或App页面 |
| Disable→Enable | revision 1→2→3，sequence 1→2→3；命令均单次ACK入账 | 通过；仅LAN authority，不改原MQTT/child绑定 |
| Retire与ACK提交丢失 | 精确命令id 5、sequence 4；首次已收到ACK但阻断其事务提交，PG仍pending | 通过；测试不是伪造ACK，也不以HTTP accepted完成命令 |
| 原proof耗尽 | attempts=8、total=8，exact command hash保持`1a077acd83331bec` | 通过；查询没有重置预算或当成ACK |
| 自动新proof恢复 | 新proof激活早于本轮Broker重启；boot hash仍`706c9ebc3fb68491`，revision仍3，原命令继续 | 通过；未手动requestRefresh、改Key、重新绑定或重置MCU来解锁预算 |
| 最终exact ACK | 第12次累计投递（新proof第4次）完成，耗时259,339ms；六次被阻断ACK与最终ACK的MD5一致 | 通过；这个耗时是人为持续故障测试，不是正常业务切换时延 |
| 独立scope | 被退休测试scope为revoked/resolution=acked；另一scope仍active且revoked_at为空 | 只证明精确控制/服务隔离；两个scope未安装为MCU LAN caller，不是实际WS断流测试 |
| Broker重启/物理重连 | 在原命令仍pending时重启shard 1，随后原boot、命令、累计投递继续并ACK | 通过；不把过程中新proof引起的合法每proof预算重开误记为重启重置 |
| MCU重启 | 新boot、新proof；从sequence 1重建revision 3 Enable，单次ACK | 通过；Key版本仍2，A/B拓扑仍Active5 |
| 旧Key版本下行 | 真实Broker拒绝version 1控制包`LOCAL_CONTROL_SESSION_STALE` | 通过Broker边界；未真正刷新Hub Key，不能声称完成旧Key登录/旧MQTT密码失效全链路 |

ACK故障在**本地PG提交点**注入，最长220秒、只匹配本测试recipient；事务失败也会使内部HTTP返回错误并出现既有限流/熔断影响，
不是精确“只丢一个无线包”的测量。故障到时自动失效，完成后已删除测试trigger/function/table，保留正常TTL管理的测试scope和签名命令审计记录。
测试入口为Service `tools/device-v2-local-control-live-gate.js`，仅允许development+显式recipient；支持status、enable/disable、retire、
有界drop-acks、reject-key-version、cleanup。不注入生产开关，不暴露Key，不新增App API；下一轮可直接复用。

最终Broker专项9项及关联回归174项通过。最初一次TCP测试因12秒测试期限取消，单独复跑和完整174项均通过；未放宽测试期限。
最终ESP32-S3 Hub实际编译：Flash 1,467,952 B，静态RAM 75,820 B，使用本机3.3.11/IDF NimBLE配置。
一次最终重启后空闲观测：heap最低152,940 B、loop栈水位3,364 B、ipc0 1,252 B。
本轮没有建立子设备BLE业务连接（nimble_host未启动），不能拿此空闲水位证明历史并发BLE栈/DoS或长稳门禁已通过。

### 22.4 后续执行，不再重复协议准备

1. 控制载体主路径和自动恢复已有真机依据；保留本轮回归，后续做最小普通WiFi self纵向链路，不再拆新的控制协议阶段。
2. 按§19.3接入可注入的LAN listener/discovery、单caller安全WS与现有LocalAccess/Noise/BBP Runtime；App仅使用原账户MQTT与按页LAN连接，设备不复制Client或组件状态。
3. 第一条LAN真机验收必须补普通WiFi产品、实际caller授权/撤销断流，以及**隔离测试身份**刷新Key后旧登录/旧MQTT凭据拒绝。
   不旋转当前Hub Key来破坏A/B拓扑，不将本轮Broker版本检查偷换为该项通过。三块现用板都已有角色，复用前明确备份/恢复与角色安排。
4. WiFi+BLE/Hub多目标、满槽无空档续期与“无缝”仍在后续扩展；本轮未新增WS listener、mDNS或公开LAN签发，未改App源码/UI。

本批无暂存、提交或推送；保留用户原有工作区和暂存区。实际证据文件包括`disable-ack.json`、`enable-ack.json`、
`retry-mid.json`、`after-broker-restart.json`、`recovered.json`、`ack-drop-evidence.log`、`after-mcu-reboot.json`与`serial-final.log`。

## 23. 单self安全消息链路与Runtime权限接入（GATT §10.123）

### 23.1 本批已实现的纵向切片

`LocalAccessFrameTransport<LeaseCapacity>`实现现有`IFrameTransport`，注入产品原有authority、clock/random/crypto与
`ILocalMessageLink`。没有新Client、组件表、设备状态、MQTT连接、协议控制队列或SDK任务。当前只接受self；即使误注入允许child的
target policy，也在调用authority前拒绝child grant，不能把child报文交给self Runtime。Hub多目标必须另接唯一executor，而非删除这条检查。

一个原生连接对应一个递增、非零的临时sessionId；它不是MAC/设备身份。网络换代丢弃原RX/TX，先退旧Runtime peer，再处理新连接。
消息均为WS binary message（这里冻结消息边界，实际RFC6455端口未实现）：

1. 设备发送原canonical CBOR Challenge（≤96 B），连接起算15秒总准入期限，不因分片/新报文续命。
2. App对候选instance与预期设备进行匹配，经原HTTPS API取得exact Grant+临时PSK；只relay canonical Grant（≤272 B），wire不携带PSK。
3. App发送52 B LocalSecureRecord InitiatorHandshake，设备返回52 B ResponderHandshake；Noise NNpsk0、LAN独立prologue、空握手payload。
   不新增Grant ACK、BBP认证方法或BLE Method 2交换。App的`open()`只在校验Responder之后完成，随后由原BBP Session主动发送Hello。
4. 后续每个LocalSecureRecord Transport内只有一个BBP/2 frame；默认frame上限400 B，加16 B tag/4 B header共420 B。
   设备参数可更小，BBP仍取双方能力交集；没有提高SMALL默认256 B或修改全库帧上限。

设备每poll最多一次消息读取/一次消息发送；一个待发exact record、3秒写出期限。队列满在加密前返回WouldBlock，不消耗nonce；
原字节重试，不重加密。未知写入失败/坏tag/重放/乱序关闭此连接，不能把不确定的Action改投MQTT。端口自身也必须有有界写入期限。
权限到期、Key/revision换代或精确退休在允许端口继续flush前检查，清临时PSK/Noise状态/明文和待发缓冲；故障断开回调从poll派发，避免send重入Runtime。
关闭普通socket不清产品lease，重连必须新Challenge/Grant，原authorizationId只用剩余期限。退休其他scope不会全局断开已建立的独立会话。

### 23.2 修正已认证传输的权限语义

原Runtime将本地`authenticated=true`等同于所有业务权限；LAN Observe-only不能沿用这一假设。
新增`IFrameTransport::sessionPermissions(sessionId)`只提供**已认证会话的上限，不自行授予权限**；本地入站以及主动发送路径都重新检查。
默认返回原all上限以保持既有Cloud/BLE行为，LAN覆盖为共享authority当前仍有效的Observe/Control，未知/失效session返回0。
不新增RxContext/PeerSession字段、不增加运行时表，不把握手后的本地授权复制成另一份永久权限。旧BBP显式鉴权仍由原provider处理，LAN不冒用其方法编号。

### 23.3 App最小客户端与端口职责

`core/device-v2/local-access/client.ts`复用现有HTTPS消费、Noise与LocalSecureRecord codec，不复制密码实现。页面/账号/网络代际由外部owner持有，
退出时abort/close；晚到HTTPS material和密码结果再次清理，不复活旧会话。每方向只允许一个在途操作，可同时一发一收；不建无限Promise队列。
发送失败在nonce已消费后关闭，不自动重发业务；idle也有有限期限，客户端保守从签发请求前起算，设备剩余lease仍是权威。
本批客户端未挂进页面、mDNS或account transport，MQTT仍只有原账户连接。测试中的mock native port不是移动端原生WS实现；
`receiveBounded()`的接收后长度检查**不能替代**原生端口在分配/入队前的硬上限。

### 23.4 资源、测试与监听器准入纠正

- Win32实测单self transport为448 B（已含Noise、challenge和一个会话上下文），另有400 B明文RX、420 B exact TX；不内置这些buffer。
  原生端口还需一份≤420 B wire RX；authority复用产品对象，不能重复计入，SDK socket/mDNS/密码栈另测。32位探针上限464 B为ABI对齐余量。
- 新Host用真实C++authority/Noise状态机/transport/Runtime，测试密码原语不是AES/X25519；覆盖只读/控制权限、Hello/Manifest/Command/ACK/主动Patch、
  背压exact重试、replay/坏MAC/错PSK/错安全域、期限、Key/撤权、跨socket代际、回调时机和重连不能续命。抽取原Noise测试假体供两组共用，删除原位置重复代码。
- Host/ASan完整55项及20,000确定性fuzz通过；App LocalAccess与原BLE/BBP关联5文件72项通过，App与spec TypeScript、两新增文件ESLint通过。
  独立运行原BLE spec时需`vitest --globals`（其原文件使用全局测试API）；首次未加该参数的加载失败不算功能回归通过。
- Arduino SizeProbe增加完整模板实例化与ABI符号，跨平台编译结果单独追加；不把对象sizeof/编译当作真实socket/密码栈或RAM峰值门禁。

本机Links2004 WebSockets 2.7.1存在HTTP行`readStringUntil`、按输入payload分配内存和循环等待读写。上游当前源码也可核对
[server读取](https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSocketsServer.cpp)与
[frame分配/读写](https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSockets.cpp)。
因此不能只接一个callback包装就声称满足本计划。减小frame宏不等于HTTP头总量/时间/SDK写入已有限；本批没有把它加入依赖、fork或发布ZIP。

### 23.5 下一批直接完成实际端口接线

1. 落实`ILocalMessageLink`实际监听器（先ESP32普通WiFi）：HTTP头总量/准入时间、mask与continuation、ping/pong/close、binary-only、
   长度前置检查、单消息背压、部分TCP写入/3秒期限、网络代际失效。先评估平台已有引擎能否做到有界注入；若必须改候选库，明确最小改动与维护代价，
   不原样包上阻塞库，也不为绕过它无审查地手写完整HTTP/WS协议栈。
2. 同批接`_blinker-v2._tcp`、SRV端口/`/bbp2`与有界TXT；service只属于本模块，不能全局`MDNS.end()`破坏其他功能；不新增每child广播。
3. App接受限原生binary WS端口与既有按页owner；设备以扩展形式注册到同一个WiFi Client。Service把已测试的HTTPS handler/事务依赖接正式路由并设准入门禁。
   不启用全局cleartext/mixed-content放行；不复制account MQTT或自动重试不确定Action。
4. 隔离测试身份跑普通WiFi MQTT+LAN双路径、实际caller授权/到期/撤销断流、真实Key刷新旧登录拒绝，再扩展WiFi+BLE/Hub共享目标。
   三块现用板仍分别是Hub/A/B，复用前先明确备份/恢复角色。当前WiFi仍按用户最后确认的mostfun2026，密码只入ignored配置。

本批未烧录/部署/重置设备/旋转Key、未暂存/提交/推送。§22的真实控制面证据保留；本批软件消息链路不能代替实际WS、mDNS、移动端网络准入或“无缝”验收。

### 23.6 最终交叉编译与复现

`tests/arduino/SizeProbe`显式实例化整个`LocalAccessFrameTransport<1>`，而非只编译sizeof声明。
两目标均使用全编译单元`BLINKER_RESOURCE_PROFILE=1`、库路径指向当前src；没有安装新SDK/外部库或改全局配置。

| 目标 | 本机Core/编译配置 | Probe Flash / 静态RAM | ELF传输对象符号大小 |
| --- | --- | --- | --- |
| UNO R4 WiFi | renesas_uno 1.5.3 | 52,440 / 16,216 B | 448 B |
| XIAO ESP32-S3 | 本机ESP32 3.3.11配置、default_8MB | 272,017 / 30,528 B | 448 B |

这是包含多种ABI探针数组的测试Sketch，不是运行中的WiFi+LAN产品内存，也不包含一个真实WS listener的成本。
Noise272/lease144/admission168/双槽authority328/challenge52/control receiver84 B在两份ELF中保持；transport已包含其中一份Noise/challenge，不可重复相加。
构建目录分别是ignored `build/lan-self-size-r4`、`build/lan-self-size-esp32`；`build.options.json`保留实际编译参数。
Host复现`python tests/host/run_sanitized_tests.py --build-dir build/s3b-host-asan --iterations 20000`；
App用`vitest run .../local-access .../ble-direct/ble-direct.spec.ts .../ble-direct/client.spec.ts .../protocol/device-v2/session.spec.ts --globals`。
最后一次55项ASan/fuzz及72项App回归覆盖当前最终实现，含原生代际替换、后台定时器挂起后的主动期限复查与native close异常清理。
架构和单Arduino库清单检查通过；登录freeze不变，未重置哈希。无真机/线上发布通过声明。

## 24. ESP32实际监听器、mDNS与普通WiFi扩展（GATT §10.124）

### 24.1 分层与本批交付边界

设备已从假消息端口推进至可编译的SDK实际实现，但“已实现端口”不等于“已完成真机网络验收”：

| 层 | 实现与唯一职责 |
| --- | --- |
| 发现 | `Esp32LocalDiscovery`只注册/删除本模块的DNS-SD service；不读Key、不做鉴权/连接/路由 |
| 原生消息端口 | `Esp32LocalWebSocket : ILocalMessageLink`负责STA IPv4接入、固定mailbox、物理代际和写出期限 |
| 安全传输 | 复用§23 `LocalAccessFrameTransport<1>`，注入原产品authority/clock/random/crypto；仍仅self |
| 产品接线 | `DeviceKeyWifiProduct<Platform, Extension>`默认空扩展；ESP32 LAN扩展只向原Client注册一个Managed transport |

没有第二个Client、组件表、MQTT、凭据存储或业务调度器。Cloud保持先注册/先poll，以便同一tick先处理撤权，再允许LAN写出。
`Client.begin/end`继续拥有transport生命周期，扩展不另建start/stop/poll编排图。未引入Links2004、AsyncTCP或私有SDK fork。
网络SDK留在ESP32 port，新增独立编译单元只承载可选LAN端口；同一个Arduino库ZIP，不新增产品安装包。

### 24.2 SDK复用与补齐的边界

本机Core 3.3.11已开启`CONFIG_HTTPD_WS_SUPPORT`。使用ESP-IDF HTTPD解析升级和解码WS；不手写HTTP解析器、SHA1握手或mask解码。
审查[HTTPD WS源码](https://github.com/espressif/esp-idf/blob/v5.5.2/components/esp_http_server/src/httpd_ws.c)
可见SDK发送接口不能代替调用方的完整短写处理；本端口不从SDK worker直接发送业务WS数据。

- 单caller、`max_open_sockets=1`、关闭LRU抢占；只接受本机STA IPv4目的地址，拒绝AP/其他网卡、非`/bbp2`、普通HTTP GET冒充升级和带body的请求。
- HTTP完整接收字节上限1,024 B，accept起算3秒；拒绝升级前提前夹带的WS read-ahead，防止绕过后续header检查。SDK原有有界URI/header parser仍负责HTTP语法。
- 小型`WebSocketIngressGuard`仅验证流中的帧头和长度，不分配/复制payload、不解mask；拒绝RSV、无mask、text/保留opcode、非canonical长度、64位长度及超过420 B的消息。
  64位长度在SDK转换为32位`size_t`前拒绝。支持有界binary continuation、ping/pong/close，校验close code/UTF-8；Ping不能延长未完成消息的3秒期限。
- SDK任务只填固定RX mailbox；一次完整消息加正在读取的固定scratch，慢消费者只在SDK任务内有限等待，不在主循环阻塞读socket。
  产品借用的RX副本只由产品loop清理，SDK关闭/网络回调不能清零正在解密的内存。
- `send()`只复制一个待发record；loop每poll至多一次非阻塞TCP写，保留精确offset，3秒超时关闭。发送方本身只编码最大4 B的固定server帧头。
  一份有界control回复可在当前frame完成后写出；不拼接/交错半帧，不分配无界队列，不从后台任务绕过authority继续flush。
- 网络断开、重新获得IP（包括IP未变）都立即使sessionId失效，丢旧RX/TX；fd由HTTPD最终关闭，避免另一线程误关闭重用fd。
  session计数跨stop/start不复用，耗尽关闭准入；单SDK对象的互斥只包固定复制/非阻塞系统调用，不持锁等待网络。

HTTPD拥有一个6,144 B任务栈；不是“无SDK任务/零后台线程”的方案。`poll`不注册mDNS、不等待TCP完成；初始化/显式stop仍调用SDK生命周期接口，
不宣称硬实时或已验证所有线程调度。mock SDK测试覆盖状态机，但不能替代真实HTTPD解析器、lwIP短写/任务并发、手机系统网络权限和DoS实测。

### 24.3 mDNS合同与所有权

- `_blinker-v2._tcp`，SRV提供端口；候选默认8920、路径`/bbp2`。App必须使用SRV，不能写死81或8920；SDK内部控制UDP端口独立，默认32769。
- TXT仅`v=2`、`path=/bbp2`、`security=noise`、32字符hex随机`locator`（128 bit），合计71 B（含各项长度字节）。无MAC/DeviceKey/PresenceKey/PSK/Manifest。
  locator在listener重新start时更新，只筛候选；随后Challenge与授权中的真实instance绑定才是身份事实。
- 复用已存在的mDNS hostname，不重命名；首次初始化没有hostname时才设置本模块的随机名称。审查并测试了SDK `mdns_hostname_get`在这时返回`ESP_ERR_INVALID_STATE`的正常分支。
- 只按本模块instance/service/proto删除自身service，不调用`MDNS.end()`、`mdns_free()`或全量remove；共享mDNS引擎由SDK/整机持有，不以stop局部feature为由停掉其他功能。
- service注册是启动阶段的有限阻塞操作；SDK维护IP变化，主loop不每次重试注册。SDK可在它启用的网络接口发布候选；**STA-only限制由socket实际目的地址检查保证**，不把mDNS可见误当可连接/已授权。

### 24.4 构建、冻结与资源修正

候选需全编译单元同时设置`BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1`和`BLINKER_INTERNAL_LAN_WEBSOCKET=1`，不能只在Sketch写宏。
用户原`#include <BlinkerWiFi.h>`、`Blinker.begin(key, ssid, password, properties...)`不变，默认不分配LAN/控制队列或启动相关SDK任务。
编译probe在`tests/internal/arduino/Esp32WiFiLanCompile`，无真实Key/网络参数；省略两个gate可编译同一Sketch作WiFi-only对照。
本批默认SMALL仍256 B业务frame，secure transport实际缓冲256+276 B；原生420 B上限兼容较大的grant，不把SMALL全局帧上限改成400。

需要纠正§23的预算使用方式：SDK worker端口不仅有“一份420 B RX”，还需稳定borrowed RX420 B、TX424 B、control127 B及同步状态。
SDK接收scratch420 B在上述6,144 B任务栈内，不可再次当独立静态缓冲累加。mDNS本机配置另有4,096 B任务栈；HTTP parser/socket/lwIP/密码堆峰值需实测。
核心transport448 B仍复用，authority不重复建立。此ESP32后端**不能按相同对象/任务模型直接搬到32 KB SRAM的R4**；后续R4/Wio端口可复用消息接口/安全层，独立选择有界网络后端。

冻结清单从v33显式修订为v34：仅共同WiFi product增加extension接线、ESP32条件typedef两项hash变化。
Key编码、登录HTTP字段、proof保存、credential刷新、ManagedMQTT、AuthRequired fail-closed、公开facade及golden向量hash均未改变。
架构检查把可选LAN独立编译单元加入准确清单（16→17）；没有放宽hash算法或忽略冻结错误。发布依赖清单不变。

Host/ASan已通过57项和20,000确定性fuzz；新增两组分别覆盖任意分块帧头/聚合长度，以及实际端口源码上的mock SDK生命周期、短写、背压、跨网络代际、权限关闭后不flush、
HTTP期限/上限/read-ahead、mDNS首启/复用/失败清理、close合法性和稳定RX。SDK假体不是HTTPD/lwIP模拟器，不声称跨线程/无线互通实测通过。

同一compile-only Sketch的最终交叉编译（默认SMALL，无真实Key/WiFi信息）：

| 目标/配置 | Flash | 静态RAM | ignored构建目录 |
| --- | --- | --- | --- |
| ESP32-S3 Core3.3.11，关闭LAN与控制扩展 | 938,520 B | 51,232 B | `build/lan-wifi-baseline-esp32` |
| 同目标，开启两个候选gate | 1,033,188 B | 56,752 B | `build/lan-wifi-port-esp32` |
| UNO R4 WiFi Core1.5.3，关闭候选gate | 135,664 B | 12,996 B | `build/lan-wifi-disabled-r4` |

ESP32合计差值94,668 B Flash/5,520 B静态RAM，包含控制扩展、密码与SDK链接变化，不是“一个WS端口的sizeof”。
动态HTTPD/mDNS任务栈与socket/密码堆峰值不在此差值中。基线ELF检查没有`Esp32LocalWebSocket`、`Esp32LocalDiscovery`、`httpd_start`、`mdns_init`或`GatewayGattPool`符号。
R4只证明原WiFi编译路径没有引入ESP32 LAN依赖，不宣称R4已实现LAN后端。未安装/更换SDK，未刷写设备。

### 24.5 下一批与真实验收顺序

1. 接Service已有HTTPS handler工厂至正式认证/限流/事务门禁路由，单self；保留原控制同步ACK/ready/精确退休，不另创签发协议。
2. App实现受限native binary WS与mDNS候选消费，复用§23客户端及既有页面/账户/网络owner；限制局域网目的地址、端口/路径和消息大小，禁止HTTP重定向绕过目的校验。
   不开放全局cleartext/mixed-content，不增加账户MQTT，不把不确定Action自动重发到另一条路径。
3. 三块现有板仍为Hub/A/B；先明确隔离普通WiFi测试身份、备份/恢复与暂用角色，再刷候选固件，完成真正App→WS→Noise→BBP Hello→Command/ACK/State，以及MQTT并行交互。
   同次记录HTTPD/mDNS/loop栈水位、heap最低点/最大连续块、短写/慢包/多扫描方/单caller占用、网络换代/重连和实际caller撤销/期限断流。
4. 隔离身份真实刷新DeviceKey后旧登录/旧MQTT凭据拒绝；不能拿Hub版本字段检查替代。上述通过后才扩展WiFi+BLE/Hub多目标，或声明具备无缝体验。

本批不修改App/Service源码、不部署/烧录/重置/清绑定/旋转Key，不暂存/提交/推送。原有Hub/A/B和§22真机控制证据不变。

## 25. 公开签发与Android有界原生消息端口（GATT§10.125）

### 25.1 本批已完成

- Service `POST /api/v2/devices/:recipient/local-access` 已接正式路由；仅允许 `targetLogicalDeviceId=""` 的 self scope。
  使用原账户身份、共享账户/tenant/IP限流、1024 B严格请求、原事务ACL/Key vault/ready和精确撤权；响应仍为原exact Grant + 临时PSK，所有结果no-store。
  `local-control-service`只额外暴露已有仓库的`withIssuance`端口；不复制授权仓库，不新建worker、定时器或MQTT。child签发核心保留用于后续网关，不经本公开入口开放。
- Android `lan/LocalWebSocketSession`负责单物理socket/有界消息，`LocalWebSocketDraft`负责限制现有RFC6455实现；
  `LocalDestination`负责纯地址规则，`BlinkerLocalWebSocketPlugin`负责Android网络/前后台/桥接生命周期。四者不持有设备身份、Noise、页面状态或MQTT。
- App `local-access/native-link.ts`实现原`LocalAccessMessageLink`；`discovery.ts`消费已安装的`capacitor-mdns`，未新造发现引擎。
  后续直接注入§23的`LocalAccessClient`，再交原BBP Session发Hello。**本批尚未挂接DeviceService/页面owner，不能把新增端口等同于App已能LAN控制。**

### 25.2 准入、资源与退出规则

1. 第一条路径限定Android、当前默认WiFi网络、同链路私有IPv4字面量、端口8920、固定`/bbp2`。
   mDNS仍读取SRV，但目前只准入设备已用的8920；其他端口、IPv6、跨子网、Ethernet/VPN后续显式扩展，不绕过校验。
   拒绝域名/DNS重绑定、公网/loopback、本机/网络/广播地址。socket先绑定确切Android Network再connect；网络换代/链路变化/后台立即退役。
2. 复用[Java-WebSocket 1.6.0](https://github.com/TooTallNate/Java-WebSocket/tree/v1.6.0)，不用WebView混合内容例外。
   HTTP响应头1024 B；拒绝重定向、扩展/压缩、text、RSV、服务端mask、非canonical长度和64位超大长度；单帧/重组总量420 B、最多16片。
   真实负测发现上游解码后的`Framedata`不保留原始mask标记，因此入口直接检查原始头；不拷贝载荷、不再实现一套重组/编解码。
3. 单待发、单待收、单未读mailbox；SDK reader等消费者最多3秒，不能无限积累JS事件。
   单独允许一个待发Pong，不因正常business TX正在排队就关闭连接；第二个未处理Ping不无限排队。
   不完整帧/分片3秒、发送3秒、open15秒、原生会话总上限300秒；idle完整消息间隙不误算成分片超时。
   send成功仅是实际socket写入完成，不代表BBP ACK；不确定send、超时、owner取消都关原socket，绝不自动重发Action。
4. mDNS只看`resolved`的当前4项TXT，最多8个地址/记录、16个候选/10秒scan；同plugin唯一scan owner，退出只unwatch本类型和移除本监听。
   不调用全局`close/removeAllListeners`。locator/IP/服务名都不是设备身份；候选必须经过预期instance校验、HTTPS授权、Noise及BBP Hello才可成为业务路由。
5. 原Android Manifest已存在`usesCleartextTraffic=true`（历史全局设置），**本批未新增/扩大它，也未打开mixed-content**。
   新native端口独立限制目的地址；不能宣称整个App已取消历史全局例外，后续须结合现有HTTP开发服务单独收口。

### 25.3 软件证据与真实测试边界

- Service签发/载体/worker专项24项通过；正式路由、公开签发组合和隔离PostgreSQL共28项通过。
  其中真实HTTP入口在控制ACK前返回`503 LOCAL_ACCESS_SYNC_REQUIRED`，ACK后签发已入库Grant；原请求重放字节一致。
  TLS直连/可信精确代理/伪造和重复标头、账户身份、限流、child拒绝、事务失败/no-store均有测试。
- App LocalAccess/BLE/BBP/connection-scope关联232项通过；App/spec TypeScript和新增文件lint通过。
- Android编译及全部JVM单测16项通过（含8项新增LAN测试）；真实loopback socket验证二进制读写、合法分片、header拆包、超长/掩码/text/片数/聚合负测、302不跟随、握手取消、慢包/慢消费者回收。
  **这是JVM与Android编译证据，不是手机WiFi Network回调、真实mDNS或MCU无线验收。**
- 测试仅用临时容器、只读源码挂载和隔离PG schema，结束由fixture清理。没有替换运行中的服务/安装APK/烧录/重置/清身份/刷新Key；没有提交/推送/暂存。
  设备协议/产品源码本批未改，§24的SDK/Flash/RAM数据未重新测量，不能用App测试代替设备资源门禁。

### 25.4 下一批顺序（取代§24.5中已完成的端口任务）

1. 先处理**本地HTTPS可信终止**：现有`tools/device-v2-local-tls-proxy.js`只是TLS→TCP透传，保留客户端标头，不能配置成可信HTTP代理。
   使用真正覆盖/去重`X-Forwarded-Proto`的HTTP TLS终止与受限backend，并验证伪造标头/非TLS入口拒绝；不影响现有MQTTS透传。
   Gateway默认`DEVICE_V2_LOCAL_ACCESS_ENABLED=false`，`DEVICE_V2_LOCAL_ACCESS_TLS_PROXIES`为空；只接受实际TLS socket或明确配置的TLS终止peer，不能写`isSecure=()=>true`。
2. 把mDNS→native link→原LocalAccessClient→原BBP Session接到现有页面/账户/网络owner，并保持账户MQTT常驻。
   LAN建立期间原云路由照常可用；仅在身份/Noise/BBP Ready后升级路由，不等待局域网握手才允许云端发送，不跨路由重试结果不明的Action。
   需要scope关闭、账号切换、退后台/网络变化和迟到完成的集成测试；不新建第三套连接/业务状态机，不先改页面编辑器。
3. 核对现有Hub/A/B角色后，明确隔离普通WiFi身份及备份/恢复，再部署/刷候选固件；真实App完成Hello→Manifest→Command/ACK/State与MQTT并行。
   原三端主线仍在普通WiFi self，不先拿Hub多child充当首条LAN体验。
4. 同轮测真实caller撤销/期限断流、换网重连、HTTPD/mDNS/loop栈与heap/socket峰值、慢包/单caller占用；隔离身份执行实际Key刷新和旧登录/MQTT拒绝。
   通过后才扩展WiFi+BLE/Hub、其他MCU/平台，或讨论无缝发布；不以100次循环替代这些闭环。

## 26. 可信HTTPS与共享直连BBP核心（GATT§10.126）

### 26.1 必须纠正的会话边界

§25的“原BBP Session”表述过于笼统：App账户`DeviceV2Session`以Route/Delivery与Broker交换消息，不能直接把其channel换成LAN socket。
普通设备Runtime接收的是self BBP帧。此次将原`ble-direct/session.ts`中的Manifest/State、Property/Action、ACK精确重传、通知和退出处理
提取为`protocol/device-v2/direct-session.ts`；BLE只保留Controller/PresenceKey三个专属操作的薄子类，没有复制第二份业务循环或缓存实现。
`direct-hello.ts`共用canonical codec，但鉴权路径显式区分：BLE仍要求Method 2；LAN只在原LocalAccess Grant+Noise已成功后选择transport模式。
不是BLE认证失败后降级，也不以局域网地址或发现记录直接授予权限。

### 26.2 本批完成

- Service开发TLS工具的HTTPS部分改成Node内建HTTPS→固定HTTP backend，解析HTTP后删除来客Forwarded/X-Forwarded-*/X-Real-IP与hop头，写入唯一`X-Forwarded-Proto: https`。
  固定目标/Host、只收origin-form，拒绝CONNECT/WS升级；不支持通用代理或SSRF目的地址。MQTTS仍独立作二进制TLS载体，原账户MQTT WS端口不变。
  HTTP头8 KiB、64连接、单socket一请求、总请求20秒、TLS握手5秒；请求/响应流式转发，取消/超时销毁upstream，不重新编码业务内容。
  这只是开发工具源码；尚未替换实际TLS容器、配置可信peer或部署Android认可的证书。
- App `local-access/session.ts`组合原安全客户端与共享`DirectDeviceSession`：安全open→加密Hello→verified Manifest→fresh State后才返回候选session。
  App传输Hello精确体为`a600010181020218c303190190041901900901`（400 B、单可靠接收窗口、无Method 2），设备Host权限测试也消费这组字节。
  按peer协商取`min(400, peer frame, peer reassembly)`，普通SMALL的256 B不能被400 B传输上限覆盖；Hello/同步准备最多15秒。
  单业务请求与通知ACK最多共2个待发，底层仍单发单收；相同命令仅按原可靠规则同序号/同body重传，不确定写关闭，不建新session/跨路由重放Action。
- 原生link→LocalAccessClient→frame channel逐层保留同一个关闭结果；取消立即中止IO并清密码，await close等待native回收。
  native关闭失败必须保留给owner，不能吞掉失败后调用“已释放”监听；迟到回调仍不能复活旧连接或关闭新id。
- 未新增账户MQTT、SQL迁移、公开用户API、UI或网关协议；未删除其他尚在使用的BLE功能。

### 26.3 本批证据与边界

- App相关24个测试文件、250项通过，App/spec TypeScript及本批13个TS文件lint通过。
  新增LAN测试为假安全字节端口上的真实BBP codec/Manifest校验/共享session，不是Android无线或实际Noise端到端；原Noise/消息端口测试另行覆盖。
- Service原签发/公开接口/控制载体/worker24项通过；新增4项真实TLS/socket测试通过：证书链/主机名校验、伪造与重复scheme清除、固定backend、禁止隧道、总期限/取消、MQTTS精确字节与回收。
  测试显式信任临时测试CA，未关闭证书验证。运行`npm run test:local-tls-proxy`需要OpenSSL（可用`OPENSSL_BIN`指定）；测试不读取开发/生产私钥。
- 设备本批只更新Host输入fixture，产品/SDK源码不改；`blinker_local_access_transport_tests` Debug+ASan通过（含Observe/Control/组合权限三种Runtime路径）。
  首次启动因进程PATH未含VS Debug/ASan运行库返回`0xc0000135`；只补当前测试进程PATH后通过，未关闭ASan或修改系统环境。
  §24的SDK体积/栈数据未重新测量，Android/JVM16项为§25已有证据，未重新安装APK。
- 没有部署、数据库变更、刷板/清身份/改Key/拓扑、暂存/提交/推送。当前Hub/A/B角色保持不动。

### 26.4 下一批可直接执行的主线

本节为§26完成时的任务快照；第1项已由§27完成源码接线，当前剩余顺序见§27.4。

1. 将短时mDNS候选→native link→`LocalAccessClient`→`openLocalDeviceSession`挂到**现有DeviceUiPort页面/账户/网络owner**。
   instance只取当前账户可信记录；各await后核对scope/账户代际。LAN准备期间既有账户MQTT与cloud命令照常工作；返回verified-ready候选后才能升级首选路由。
   退最后页面owner、切账户、后台、换网均关闭本地；回收失败不标释放、不并发开第二条native连接。不复制第三套连接管理、命令队列/业务cache，不改编辑器。
2. 本机部署本批HTTPS工具时先核对backend网络隔离与实际proxy精确地址；不能沿用当前公开backend并直接信任所有loopback/private IP。
   安装/使用手机认可且主机名正确的开发证书，检查真实HTTPS路由/no-store/伪造头负测后才打开self签发flag。源码测试通过不等于这些部署条件已经完成。
3. 只读核对Hub/A/B后，明确隔离普通WiFi身份、应用/NVS备份与恢复方案；再ADB+MCU验收Hello/Manifest/Property及Action/ACK/State，并验证MQTT常驻并行。
4. 真实caller撤销/期限断流、换网、Key刷新旧登录/MQTT拒绝、HTTPD/mDNS/loop栈及heap/socket峰值和慢包/占用负测仍为发布条件。
   第一条self闭环之前不扩WiFi+BLE/Hub多目标/其他MCU，也不恢复100轮交替测试。

## 27. 原页面owner与可信self身份接线（GATT§10.127）

### 27.1 修正缺失的身份来源，不增加身份数据库

实际App设备目录没有可直接用于LAN握手的可信`deviceInstanceId`；不能将mDNS TXT、MAC或收到的Challenge自行认作目标。
复用同一`/api/v2/devices/:id/local-access`路径，GET返回`local-access-context/1`：logicalDeviceId、16 B instance的canonical base64url、permissions与maximumLifetimeMillis。
它只查询原ACL、当前Key版本、authority和exact ACK ready屏障，既不解密DeviceKey也不生成授权行/PSK；仍要求HTTPS、账户认证、原限流与no-store。
当前账户无Observe权限则拒绝；整机operator为3，viewer及组件限定operator只为1。后续POST签发重新检查实时ACL，GET不是长期授权。
App先获得此预期instance，再发现候选并比较Challenge；仅“签发之前发现instance不同”可继续下一候选，授权/Noise/业务失败不自动换设备重试。

### 27.2 模块与现有业务的接合

- `LocalAccessConnector`只组合身份/发现/link/self session端口；与Angular、页面、MQTT解耦。整个App一个native LAN槽，不建每设备socket池或新重试调度器。
  10秒发现期间收到候选即可开始连接，不等扫描结束；最多16个候选、尝试4个不同目标，总准备期限30秒。
- `DeviceV2LanService`是小型Android组合层：捕获账户代际和原NetworkService的网络代际，向下传唯一AbortSignal；不持久化IP/PSK/新设备目录。
  原NetworkService保留offline语义，增加WiFi证据/代际；即使connected一直为true，新的WiFi事件也退休旧LAN。
- 原`DeviceUiPort.ConnectionGroup`持有一份LAN准备结果。页面打开时云端立即准备，与LAN并行；只在Grant/Noise、Hello、verified Manifest和fresh State完成后选LAN。
  退出最后页面、切账户、后台、删除设备关闭owner；多页面共享当前连接，普通列表/一次性读取不触发LAN扫描。失败/到期退回既有cloud候选，不另建账户MQTT。
- Property/Action沿原command接口发送，LAN结果不明直接报错，绝不捕获后向cloud重发。Observe-only或组件限定分享的写请求在发送前就选cloud ACL。
  State/Event仍经原UI port投影，只有当前有效owner的已验证Manifest变化写入原账户缓存；无编辑器/组件API改动。
- 原生socket或mDNS清理失败不声称已释放、不允许重开；普通发现错误且清理成功允许未来页面重试。错误关闭只退休本地路径，不关MQTT。

### 27.3 准入边界与验证

首个切片仍是Android+HTTPS账户+当前WiFi+cloud-enabled self，BLE-only与edge-hub不准入；`deviceV2LanEnabled`在三个环境均为false。
`diy`目录类型不足以区分普通WiFi与WiFi+BLE，所以门禁只安排隔离普通WiFi身份；不据此宣称其他产品实测通过。
当前一个页面周期只尝试一次LAN准备，最长300秒后退cloud，未实现满槽无空档续期；Telemetry继续走原账户云端租约，未新增LAN实时数据协议。
只读现场确认ADB、COM3/COM11/COM12和Docker在线，但没有刷板、清身份、刷新Key、改拓扑或部署服务；现用Hub/A/B保持不动。
最新用户网络为“带翅膀的蜗牛”，密码不写文档/公开示例；后续临时真机配置不得沿用此前mostfun2026参数。

本批Service签发/HTTP专项17项、隔离真实PG+public+Gateway接线30项通过，无skip；GET含ACK前503、ACK后可信instance、实时ACL/撤销、重登录失同步、无Key解密/无授权行。
App项目`ng test`最终51个文件/507项全部通过，无skip/未处理错误；App/spec TypeScript与变更TS/lint通过。
首轮裸Node缺JIT/浏览器环境、首轮Angular多fork超时及relative mock不兼容已修正重跑，详细配置见[App计划§6](../../blinker-app/docs/APP_DEVICE_V2_LAN_ACCESS_IMPLEMENTATION_PLAN.md)。这些都是软件/隔离数据库证据，不是无线实测。

### 27.4 下一批直接执行

1. **先部署准入，不再扩协议。** 按Service§17隔离Gateway backend，更新HTTP-aware TLS代理；只信任实际终止peer的精确地址，配置手机认可且SAN正确的证书。
   当前`0.0.0.0:17001`公开HTTP映射未隔离，旧TLS容器未更新，不能直接打开公开签发。部署前备份当前compose/runtime配置，保留原Broker/MQTTS职责。
2. 验证实际HTTPS GET/POST、no-store、伪造转发头/明文拒绝；条件满足后仅本地测试配置开启Service/App LAN，构建并安装APK。
3. 只读核对三块板的角色、MAC与固件，再备份选定板的应用/NVS并安排恢复；用隔离普通WiFi身份验证mDNS→身份→Grant/Noise→Hello/Manifest→Property+Action/ACK/State，账户MQTT保持在线。
4. 再测退出/后台/换网、真实撤权/到期断流、Key刷新旧登录拒绝及任务栈/heap/socket/慢包预算。先收口单self真实业务，之后才扩WiFi+BLE/Hub child与S4暖切换；100次交替循环仍不恢复。

## 28. 本机部署、真实手机准入与首轮WiFi测试（GATT§10.128）

### 28.1 本批完成与真实接线遗漏修复

1. 先保存原容器配置、compose回滚文件和PostgreSQL完整dump。仅重建本地Gateway/开发TLS代理，原Broker/Controller/Redis/PG及Hub/A/B服务器拓扑不变。
   原`0.0.0.0:17001` HTTP映射收口为宿主`127.0.0.1:17001`负测入口；代理直接通过Docker DNS访问`gateway:7000`。
   只信任代理的固定地址`172.19.0.250`，不信任private网段。重建时旧代理地址曾被Gateway占用，已按实际peer纠正；不能照抄历史容器IP。
2. 开发代理HTTP职责不变；MQTTS和可选账户WSS共享一个有界TLS字节工厂，删除旧MQTTS专用工厂名，不增加Broker或第二条账户MQTT。
   本次App HTTPS/WSS经USB reverse的17000/18885；MCU走真实WiFi上的HTTPS/MQTTS。**只有App到MCU的8920走真实无线LAN，无8920端口转发。**
3. 以原开发CA签发SAN含本机实际地址的短期leaf，未关闭链/主机名验证。Android通过仓外`BLINKER_DEBUG_NETWORK_CONFIG_DIR`提供debug-only manifest/resources；release不包含此信任入口。
   App源码三个环境仍默认关闭LAN；本次构建临时开启local后已经恢复源码默认值。安装的debug APK保留本机门禁配置，不作为发布包。
4. 手机实测暴露`local-access`不在原`isGatewayUrl()`白名单，GET/POST漏带Bearer而返回401。
   增加统一`API.DEVICE_V2.LOCAL_ACCESS(id)`并加入原精确鉴权路径；GET/POST复用原拦截器/刷新/账户代际，不在LAN实现第二套登录。
   回归同时验证外部主机、局域网WS目标不能收到账户Bearer。
5. 原ESP32端只用`sockaddr_in`接收本地地址；SDK启用IPv6时HTTPD实际使用双栈listener，IPv4来客可能呈现IPv4-mapped IPv6。
   改用`sockaddr_storage`，只规范化AF_INET或严格v4-mapped地址后与当前STA IPv4比较；原生IPv6、AP地址、截断地址、无STA IP继续拒绝。
   此为SDK适配修复，不是开放IPv6产品功能，也不涉及Noise/身份策略。参考[Espressif HTTPD监听器源码](https://github.com/espressif/esp-idf/blob/v5.5.4/components/esp_http_server/src/httpd_main.c#L325)。

### 28.2 真机证据、备份与恢复

- 最新SSID为“带翅膀的蜗牛”；手机连接该路由5 GHz SSID后缀`_Wi-Fi5`，PC与手机同属192.168.2.x，MCU用用户提供的2.4 GHz配置。密码仅入ignored门禁配置。
- 新建独立普通WiFi记录`LAN Self 20260909`，不是复用/刷新Hub或原子设备Key。测试Sketch仅调用公开`BlinkerWiFi.h`、begin/run、Property/Action；内部开关/CA/凭据通过仓外header注入所有编译单元。
- COM12基准MAC核实为`98:3d:ae:ee:7c:6c`。stub整盘读取两次在约332 KB中断，改ROM读取完成8 MiB备份；SHA-256为`4b5f649f806585ece78e137b62686d2f075426cdf93ffb1543536bbcde8b4a6b`。
  校验原分区表与新固件一致后，仅临时清NVS/写OTA metadata与app0；结束已恢复`0x9000..0x33ffff`原始3,371,008 B并通过esptool写后hash校验，bootloader/app1/其他分区未改。
  独立测试身份的NVS/OTA另行保存，后续可复用，不必每批新建设备或反复清原子设备。原Hub/另一子设备未刷写；未删除服务器设备/改原拓扑。
- 手机真实HTTPS：未认证401、直连明文即使伪造scheme仍403；修复鉴权后未上线设备GET为409 authority未建立，上线同步后GET200且`private, no-store`；非法POST400。**未获得成功签发POST证据。**
- MCU实际WiFi→DeviceKey session→MQTTS→云端Hello/Manifest/State成功，App能得到power=false、buttonPresses=0和button Action能力。
  地址修正前原生WS约65 ms内失败；修正后到已发现192.168.2.54:8920的原生WS打开成功，观察到114/251 ms，不能据两次样本宣称延迟分位或无缝。
- WS之后首个安全Challenge receive等待15秒被关闭；没有进入LAN-ready，没有发送Property/Action，没有用云端控制冒充LAN结果。
  页面生命周期内App账户MQTT对象未替换、state保持ready；MCU MQTT出现一次重连（约5.4秒恢复），后续必须核对与端口/控制生命周期的关系，不能宣称设备双通路稳定。
- 所有证据、恢复镜像、配置与DB备份位于ignored `build/lan-live-20260909`。增加有界`tests/hardware/lan_self_app_gate.mjs`：只使用原page port、记录检查点、120秒释放、发送前锁定命令次数，不重试不确定Action，不修改UI布局。

### 28.3 回归及资源边界

App拦截器/LAN共7文件95项通过，App TypeScript和变更文件lint通过；APK Debug构建、Release manifest任务和Android/JVM16项通过。
首次测试因尚未更新统一URL的mock有2项失败，更新后95项重跑全过；507项是§27已有证据，本批未重跑完整套件。
Service开发TLS真实socket5项通过，新增真实WSS二进制往返；不是新增协议或生产TLS发布门禁。
ESP32端口mock SDK补双栈默认、IPv4/v4-mapped成功与原生IPv6/AP/截断/无IP拒绝；Debug和ASan通过，SDK3.3.11真编译通过。
XIAO S3门禁固件Flash 1,128,476 B、静态RAM 57,384 B；运行中观察free heap约193～234 KB、loop栈余量3472 B，尚非所有任务/压力峰值。
内部Sketch用构建时刻为短期证书测试提供时钟初值，不是生产可信时间方案；不得将测试CA、凭据或忽略目录打包发布。

### 28.4 当批下一步（已由§29修订）

1. 复用隔离self身份和备份恢复流程，不清Hub/A/B、不扩产品；先确认MCU MQTT与authority ready，再打开页面。复位后提前开页会触发已有30秒准备期限，不能混作握手性能。
2. 在SDK Upgrade→link.sessionId→LocalAccessFrameTransport.poll/open→Challenge入队→loop刷新→Android receive之间增加仅阶段/长度的有界诊断，定位首个Challenge未到达；同时核对MCU MQTT重连原因。
   不先提高15秒期限、不放开鉴权、不增加第二套队列或第二条账户MQTT；不要把WS101当作安全/业务ready。
3. 修复后完成同一次真实mDNS/HTTPS Grant/Noise/Hello/verified Manifest/fresh State，以及Property、单次Action、ACK/State证据；验证App账户MQTT同一session与设备MQTT并行稳定。
4. 此后才做退出/后台/换网、caller撤销/到期/Key刷新旧登录、HTTPD及loop栈/heap/socket和慢包负测。S4暖切换、Hub多目标和100轮仍不提前。

## 29. 默认明文修订与可选加密路径收尾（GATT§10.129）

### 29.1 为什么修订，而不是继续增加安全前置

用户明确第一阶段优先Arduino易用性、多MCU兼容及资源预算，可参考WiFi MQTT“默认非加密、必要时开启加密”。从本节起，LAN基础路径不强制Noise；§12～28的安全协议及证据保留为**已有可选实现/历史阶段**，不能继续按这些章节的旧“唯一下一步”扩散。
目标保持一套业务核心、两种可选记录处理模式。传输负责WS、鉴权负责caller/目标/期限、记录层决定是否加密；不是两套产品或两个ZIP。默认模式也不绕开App账户ACL、Key代际与本地授权，不能把“同网段”当身份。
当前代码仍只有加密LAN候选，**本批没有声称已经实现默认明文**；新的线格式必须可明确区分、模式受可信策略与会话证明约束，不通过嗅探首字节在同一已认证会话自动切换。

### 29.2 已定位并收尾的SDK问题

1. 实际Arduino ESP32 3.3.11捆绑IDF为`v5.5.5 b774170ff46`。其HTTPD完成WS Upgrade后不再调用原GET业务handler；原port等待GET创建session，因而无首个Challenge。改为在product poll及SDK收帧入口读取公开`httpd_ws_get_fd_info()`并幂等提交会话，同时兼容旧SDK GET回调。
   原始请求仍需验证：复用SDK已有`http_parser`增量检查精确`/bbp2`、GET、Upgrade、无body/chunked、无提前混入WS帧，保留1024 B/3秒HTTP预算。不另写HTTP codec，不依赖SDK私有结构。见[实际SDK源码](https://github.com/espressif/esp-idf/blob/b774170ff46/components/esp_http_server/src/httpd_uri.c#L332)。
2. 安全路径随后到达Grant接受与Noise请求验证，却在空明文GCM响应返回`MBEDTLS_ERR_GCM_BAD_INPUT(-20)`；IDF AES后端要求长度为0时I/O指针也非空。仅在ESP32密码port用一个局部哑字节适配空输入/输出，长度仍0、AAD/nonce/tag/算法不变。没有修改SDK或引入私有加密库。见[IDF GCM实现](https://github.com/espressif/esp-idf/blob/b774170ff46/components/mbedtls/port/aes/esp_aes_gcm.c#L472)。
3. 内部门禁Sketch改为USB `Serial.setTxTimeoutMs(0)`，无人读取日志时允许丢诊断，不能阻塞loop数秒；不改变公开库Print语义。临时SDK/密码trace已移除，保留独立测试用空GCM已知向量，公开零Key/nonce仅用于离线自测，绝不用于会话。

### 29.3 真实证据与未完成项

- Host端口Debug与ASan通过，补充SDK不调用GET/旧GET幂等、请求路径/方法/body/早发负测。mock中的HTTP parser只是回调合同替身，不冒充真实SDK parser；另用真实MCU原始TCP/WS门禁9/9通过，三个有效请求收到首个binary包约244/322/188 ms，负测未升级。该门禁验证socket行为，不单独证明包就是合法Challenge。
- 最终Arduino 3.3.11门禁固件Flash **1,129,564 B**、静态RAM **57,432 B**；ESP32真实密码provider通过空payload、空/非空AAD的加解密向量及坏tag拒绝，日志`emptyGcm=1`。运行观察free heap约195 KB、loop栈3472 B，不是所有任务/握手峰值。HTTP parser引入约40 B静态RAM；**这不是加密的资源成本**，尚无同条件Plain/Encrypted对照编译，不能声称加密导致OOM或去掉能省多少。
- 真实手机mDNS→原生WS→65 B Challenge→169 B Grant→Noise双向52 B消息→Hello/verified Manifest/fresh State完成，原页面port切换LAN；本次打开到LAN Ready约3.8秒，包含发现/云端授权/同步，不是单次控制延迟分位。App账户MQTT保持同一session且ready，没有增加第二MQTT。
- `power=true`经LAN发送、获得ACK并更新State，MCU记录`gate power=1`。测试helper原将`BLINKER_ACTION(button)`当整型Action传1，但该宏为无参Action/CBOR null，App编码阶段拒绝、没有发出Action；helper已改为null，**Action/ACK/计数仍待下一次真机验证**，没有以云端调用或重试冒充通过。无需为测试输入错误改生产codec。
- 页面关闭完成并保留账户MQTT；未测快速重开、真实撤权/到期、Key刷新、后台/换网、所有任务栈与长时间双通路稳定。当前单槽授权ledger的重开容量/期限需与后续模式一并验收，不能提前声称无缝。
- 本轮开始备份COM12当时NVS/OTA，结束恢复该份元数据加此前完整备份中的原app0，`0x9000..0x33ffff`共3,371,008 B，esptool写后hash通过。恢复文件SHA-256 `98730477b74f3556dec77de04470eb66c544724882536e9de6176f514499f558`。COM11 Hub/COM3另一子设备、原服务器拓扑/Key未改；隔离测试身份保留。证据在ignored `build/lan-challenge-20260909`，不打包发布。

### 29.4 修订后的固定顺序（进度见§30）

| 顺序 | 实施范围 | 退出条件 |
| --- | --- | --- |
| P0：最小合同 | 默认明文、可选加密；冻结可信模式能力/策略、caller证明与模式绑定。优先复用有限Grant和HMAC挑战证明，双方不明文发送长期Key/短期PSK；不再增加所有者记录、另一套权限库或手工配置步骤 | 三端golden与拒绝错误模式/实例/重放/过期；明确只有握手认证、明文帧无保密/完整性保证的威胁边界。模式变化不偷换旧`local-access/1`含义 |
| P1：设备普通WiFi | 在现有LAN extension注入记录/会话处理；Plain直接承载BBP，Encrypted复用现有Noise记录。同一Client/收发预算/权限检查，只有加密模式链接专属密码状态 | 默认/加密/禁用LAN三配置编译、Host/ASan；普通WiFi不链接Hub；实测静态RAM/Flash、握手heap/stack/socket，不能只去掉调用仍保留工作区 |
| P2：App/Service接合 | 原HTTPS授权服务显式返回获准模式；原页面/账户/网络owner选择对应byte session，共用DirectDeviceSession、缓存/队列/ACK。mDNS只给提示，失败不自动降级 | Plain与Encrypted互操作/拒绝交叉、ACL/撤销/期限，唯一账户MQTT持续可用；不改页面编辑器，不给每设备加MQTT |
| P3：首个默认明文闭环 | 隔离普通WiFi真机Hello/Manifest/Property及无参Action/ACK/State，退出/快速重开/Cloud并行与已知权限退休；采用相同脚本另验可选加密 | 区分真实经过的路径/未知命令结果；账户会话不重建、资源回收有证据。再扩WiFi+BLE、Hub共享child以及S4暖切换，不恢复100次交替 |

P0与P1可在同一批完成，但必须先定义线上字节与验证边界，不能只关闭现有加密开关当作Plain已实现。普通MQTT/HTTPS登录现状、BLE安全、Hub秘密交付保持原约束；LAN选择明文不自动解除它们。明文的首发风险说明和加密能力/资源支持矩阵是发布条件，增强加密压力门禁不再阻塞基础明文功能开发。

## 30. P0合同与P1共用设备核心（GATT§10.130）

### 30.1 本批最小合同

现有Challenge/Grant已有`securityProfile`且Grant MAC覆盖该字段，不新增重复mode字段、第二种CBOR布局或版本嗅探。外层wire version仍1；profile 1的Noise字节/域不变，新增profile **2 = PlainHmacSha256**，表示只有握手HMAC鉴权、其后BBP业务明文，不表示逐帧认证。未知profile拒绝。当前接收端每条socket只能使用构造时选定的一种profile，Challenge固定声明它，Grant必须精确匹配；不允许对失败的加密握手自动重试Plain。

仍由现有授权服务检查账户ACL/当前recipient instance/Key/authority、target和期限并计算原Grant MAC。会话材料统一概念为32 B session key：profile 1使用原Noise PSK派生域；profile 2使用`HKDF-SHA256(IKM=DeviceKey, salt=Grant.authenticator, info="BLINKER-LAN-ACCESS-PLAIN-PROOF/1", L=32)`。App通过HTTPS获得profile 2材料，DeviceKey、原始session key均不出现在LAN消息；不同profile不可互相派生或复用密钥。

Plain握手在原Challenge/Grant后增加两个固定canonical CBOR证明，各37 B：`[1,8,bstr32]`为caller，`[1,9,bstr32]`为recipient。证明为`HMAC-SHA256(sessionKey, ASCII(roleDomain) || Grant.authenticator)`；roleDomain分别为`BLINKER-LAN-PLAIN-CALLER/1`与`BLINKER-LAN-PLAIN-RECIPIENT/1`，不含字符串结束符。Grant MAC已绑定双方会话标识、fresh challenge、实例/Key版本、权限、期限和profile；方向域防止简单反射，验证使用常量时间比较。双方证明完成后清临时密钥/证明，下一条消息才是原始BBP/2 Hello，不加逐帧包装。原15秒挑战/握手及300秒最大授权期限不变，失效/撤销先于队列发送检查。

这是项目的**基础访问控制合同，不是经第三方审计的通用安全握手**。明文模式不提供保密性、逐帧完整性、抗主动中继/会话接管或恶意链路重放保证；旧Grant/证明不能建立新挑战会话，不代表攻击者不能篡改已建立连接。危险执行器/不可信LAN应要求profile 1。不把BBP ACK、WS mask、序号或Manifest hash描述为安全保护。

### 30.2 分层与本批范围

传输保留唯一`LocalAccessFrameTransport`，引用可注入的Plain/Noise记录会话；队列、挑战、authority/ledger、权限、期限、断开回调和Runtime接入只维护一份。Plain会话不包含Noise/X25519/GCM状态，不创建第二个Client或更多凭据槽。Grant ledger的scope补profile比较，拒绝同authorization跨profile改变含义。

三端codec和同一golden先支持新profile/证明；现有HTTP `local-access/1 + noisePsk`及现有App Noise client必须继续明确拒绝profile 2，不能因为底层codec放宽而隐式签发/消费Plain。P2再将原接口扩为显式profile的`local-access/2 + sessionKey`，复用原仓库与事务，配合可信模式能力/策略接线。**本批不打开HTTP Plain签发，不部署/刷板，不声称App Plain可用。**

普通WiFi内部LAN产品将默认选择Plain，显式编译期选择Encrypted；LAN功能总开关仍关闭。选择发生在库内部组合层，不能把listener/record/workspace暴露给Arduino用户，公开易用入口与三端完整发布留到P2/P3。下一批不回去重复实现已通过的Cloud控制/ACK层，也不扩网关多目标或100轮。

### 30.3 已完成的软件边界与后续准入

- `LocalAccessPlainSession`只保留双方预计算证明，完成/失败清除；记录无额外头，复用原BBP。`LocalAccessNoiseSession`包装已有Noise/LocalSecureRecord，不复制密码实现。删去共用transport内嵌Noise及专属workspace假设，普通Plain组合不引用ESP32专属crypto provider。
- mDNS的`security=plain/noise`跟随构建选择，不再硬编码Noise；此字段仅用于发现提示。可信能力/策略尚待P2从已认证服务返回，不允许依据mDNS签发或降级。
- Runtime区分握手鉴权与业务加密。明文本地输入必须来自声明`TransportFeatureAuthenticated`且给出当前session权限的transport；单独伪造`RxContext.authenticated`不够。原不可信local/BLE通道继续拒绝，密钥/Controller等敏感变更仍要求加密，Cloud规则不变。
- 原有限权限/授权ledger/挑战不复制；同授权的scope增加profile，合法签名也不能跨模式借用。握手证明非法/反射、旧挑战重放、跨profile、过期/撤销/Key代际、预认证业务帧及有界队列均有Host回归。
- 三端共享`local_access_golden_vectors.properties`：现有profile 1向量不变；新增profile 2 Challenge、Grant、MAC、派生key及双向证明。固定fixture为65 B Challenge、167 B Grant、37 B/方向证明；Grant随字段取值变化，不是恒定167 B。
- 本批App只增加codec/注入式HMAC证明和旧入口拒绝测试，Service只增加同一协议包及旧签发入口防误用。没有改UI、部署、刷新Key、数据库结构或设备固件；真实模式切换尚未接通。

算法定义参考[HMAC RFC 2104](https://www.rfc-editor.org/rfc/rfc2104)与[HKDF RFC 5869](https://www.rfc-editor.org/rfc/rfc5869)。这些是底层原语标准，不代表本项目Plain握手经过安全审计。

#### 同条件编译与资源证据

同一公开`examples/WiFiQuickStart`、占位凭据、Arduino ESP32 **3.3.11 / IDF 5.5.5**、`esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`；只变化内部LAN/authority和记录模式开关。未注入真机CA、业务Key或密码自测代码。

| 构建 | Flash（B） | 静态RAM（B） | 最终ELF的NoiseNnSession函数 |
| --- | ---: | ---: | ---: |
| LAN关闭 | 941,340 | 51,280 | 0 |
| LAN Plain | 1,027,612 | 56,600 | 0 |
| LAN Noise | 1,036,544 | 56,832 | 25 |

Plain相对Noise少**8,932 B Flash / 232 B静态RAM**；Plain相对LAN关闭多86,272 B / 5,320 B，包含HTTPD/mDNS、authority及共享产品等整条扩展，不仅是记录层。当前不能据此认定加密导致RAM不足，也不能从静态量推断握手峰值。最终ELF已定义函数符号检查确认Plain无Noise、三者均无匹配的Gateway child/GATT执行函数；不能用map的丢弃段或源码文件存在与否判断实际链接成本。

Host-64实测：Plain会话65 B、Noise会话280 B、共用transport248 B；**不能把248 B单独与旧内嵌Noise对象比较**。Win32 Release实测：65 B / 272 B / 172 B，旧内嵌transport448 B，新transport+Noise为444 B（外部对象/对齐另计）。Win32的lease144 B、admission168 B、authority2为328 B、challenge52 B；profile进入既有padding，未新增凭据槽。SizeProbe原464 B门限改为检查transport+Noise之和，防止通过拆对象虚假降低预算。

本批验证：Host全核心及LocalAccess/双模式transport/ESP32端口四组ASan通过；Win32 LocalAccess与双模式transport通过；App **6文件70项**、App/spec类型与变更TS lint通过；Service协议包**28项**及签发/public HTTP **18项**通过；架构及三仓whitespace检查通过。完整三仓全部测试和所有MCU SizeProbe本批未重跑。首轮Runtime解耦曾暴露不可信local的authenticated标志可误放行，补显式transport能力准入后全核心和ASan回归通过，未压制旧测试。

编译/测试日志位于ignored `build/lan-plain-20260909`；编译使用`--jobs 2`限制本机并行内存压力。私有`Esp32LanSelfGate`现仅在Noise模式运行空GCM自测，Plain不再被该测试强制链接密码provider；模式日志明确profile，跳过不报测试通过。该门禁Sketch修改后尚未重刷，旧§29复现需显式设置`BLINKER_INTERNAL_LAN_ENCRYPTED=1`。

**进度：P0字节/负测完成，可信模式策略接线归P2待完成；P1核心/静态对照完成，MCU运行时heap/各任务栈/socket与其他平台资源仍待P3及后续平台准入。** 没有把Win32替代MCU ABI、把静态RAM替代动态峰值，也没有宣称默认Plain已发布或真机可用。

### 30.4 当批后续：P2接线，再P3实测（P2结果见§31）

1. **先冻结可信模式准入。** 沿用原经过DeviceKey证明、精确ACK确认的self authority及HTTPS身份上下文，增加最少必要的模式能力/策略绑定；版本/实例/Key变化时旧声明失效。不要仅信任Challenge/mDNS自报，不新建另一张权限表或凭据槽。
2. **原接口演进，不建立第二套授权服务。** 在原HTTPS GET/POST接入显式`local-access/2 + sessionKey`；profile 1/2共享ACL、事务、限流、期限与撤权。新App统一消费sessionKey，最终移除被完整替代的内部`noisePsk`命名/兼容壳；未完成接线前保留旧profile 1严格合同，不能提前删除有效功能。
3. **只注入byte session。** 原App connection/page/account/network owner共用Plain/Noise状态机边界，继续使用DirectDeviceSession、Manifest/State缓存和command/ACK队列。加密要求失败关闭，保持一条账户MQTT；退出/重开、期限回收与单槽容量先用软件覆盖，不靠增加slot隐藏问题。
4. **普通WiFi隔离身份先测。** 确认网络与板角色，沿用备份恢复流程；真实验证默认Plain Hello/Manifest/State、Property和无参Action的ACK/计数、退出/重开、Cloud并行、撤权/到期/Key刷新和heap/任务栈/socket。用同一脚本另验Noise。历史加密成功不能抵扣本轮Plain；App/Service尚未接线前不烧录默认Plain固件。
5. **首个self链路通过之后**再推广WiFi+BLE、Hub共享child与S4暖切换。其他MCU实际编译/资源矩阵及首发风险说明仍是发布条件；不恢复100轮交替，不扩编辑器、History/Bulk或网关协议。

## 31. P2可信模式、统一客户端及有限授权重开（GATT§10.131）

### 31.1 先解决可信模式来源，不复制登录或权限系统

Challenge与mDNS尚未鉴权，不能据此决定签发哪一种材料。复用已存在、绑定当前DeviceKey登录proof的签名Progress Query声明接收端模式；Query原本就能建立与Hello相同的控制checkpoint，不为这项能力新增Hello格式或另一条MQTT。

- **只演进Progress为v2**：`[2,kind,instance:b16,keyVersion,controlSession:b16,requestNonce:b16,sequence,revision,state,securityProfile,MAC:b32]`。签名为canonical前10项数组加当前proofSession的16 B后缀，沿用原独立Progress HMAC域；version和profile都被覆盖。最大105 B，仍在112 B控制body/122 B载体预算内。
- 保留既有v1 Progress codec/golden，仅代表Noise；新设备统一发v2 Query并只接受匹配version/profile/nonce的Status。Hello/Command/ACK、Challenge/Grant的原布局不变；普通DeviceKey登录协议不变。这不是恢复Legacy JSON。
- 模式在设备组合层编译期选择。Cloud控制声明与LAN记录profile通过静态断言绑定，不增加运行时模式状态、凭据槽或用户初始化参数。
- Service migration **030**只给原`v2_local_control_receivers`增加可空`security_profile`列。通过当前Key/proof验证的Query才可写入；Hello/ACK本身不声明模式。GET/POST仍需原精确ACK ready、当前实例/Key/权限和退休屏障，模式为空不能签发。
- 同一个controlSession即使带合法MAC也不能改profile；更换boot session仍须新proven login，并清空旧模式等待重新声明。原proof重放不能把模式改回去；同boot重登录复用模式但必须通过原新proof/checkpoint屏障。
- Broker仍有界转发exact控制字节，只增加Query/Status的version/profile关联检查；它不解封Key、不签发Grant、不成为第二个权限来源。

### 31.2 一个HTTP合同、一个连接生命周期

原URL的GET现返回`local-access-context/2`，包含可信`securityProfile`和实例/权限/期限。POST原六项外增加显式`contract:'local-access/2'`；响应为`{contract, exactGrant, sessionKey, validForMillis}`。旧HTTP v1或缺contract请求明确拒绝，不保留永久的Noise专用接口；旧`deriveLocalAccessNoisePsk`包装及生产调用已移除，Noise原派生域和字节不变。

Service在解封DeviceKey和预留授权前检查Challenge模式与持久声明一致，签发器再检查事务快照。Plain/Noise共用原ACL、serializable事务、限流、幂等、期限和撤权。HTTPS/no-store交付临时sessionKey的要求不变；Plain不会发送原始Key，也不改变BLE或Hub秘密交付约束。

App增加小型`LocalAccessRecords`策略，Plain负责37 B双向证明及raw BBP，Noise包装已有Noise/LocalSecureRecord。唯一`LocalAccessClient`继续拥有native link、单发单收、有界帧、取消/迟到清理、绝对截止时间与终态错误；DirectDeviceSession/Manifest/State/command/ACK及页面owner不复制。mDNS接受plain/noise提示但只输出候选地址，不输出获准模式；匹配实例的错误profile直接失败，不能自动降级。

Plain只完成握手鉴权，**业务不加密、不逐帧认证，不能防主动中继/连接注入或接管**。敏感本地变更仍要求加密。没有以新增HMAC证明声称业务帧具备完整性保护；选Noise需当前设备明确声明，不能由App单方面改变接收端模式。公开模式配置入口/支持矩阵仍待实测后冻结。

### 31.3 快速重开不增加授权槽，也不延长期限

原App每次开页生成新的caller ID，会导致服务器生成另一份authorization，并可能撞上设备唯一ledger槽。现由账户组合层在同一account/authority/sessionEpoch内复用一个内存中的公开16 B caller ID；每次握手仍有新receiver挑战、request ID和Grant。没有缓存Key/IP或持久socket，也不增加第二MQTT。

同caller、同scope的原服务器授权可以复用，`expiresAt`不变。新响应`validForMillis`在事务提交后以该授权的真实剩余时间计算；App从HTTP请求开始的单调时刻扣除耗时后设置截止时间，不能重新给满300秒。设备仍是最终期限/权限检查者；HTTP剩余时间不替代Grant或设备ledger。

**范围限制：**同进程同账户快速重开已有软件覆盖；杀App后新caller、切换账户、改变权限/scope仍可能在旧单槽授权退休/过期前拒绝新本地握手。此问题尚未真机闭环，不靠扩槽或自动放宽权限掩盖。退出页面关闭本地连接不等于立即销毁服务器有限授权；Cloud继续按原路线使用，未知Action不得跨通道自动重发。

### 31.4 本批验证与资源

| 验证 | 结果与范围 |
| --- | --- |
| 三端共享协议golden | 新增v2 Plain Query/Status；Service协议包29项通过，原v1向量不变 |
| Service | 签发/HTTP/public/控制worker与carrier 27项通过；真实PostgreSQL 25项通过（独立随机schema、自动清理，不改业务表） |
| Broker | TypeScript build与控制载体9项通过，含错误响应profile拒绝 |
| App | 7文件88项、App/spec TypeScript及LAN变更eslint通过；双模式、迟到清理、错误模式、账户代际、重开caller与剩余期限覆盖 |
| 设备Host | 全核心、HTTP DeviceKey/MQTT、LocalAccess、双模式传输、ESP32 WS端口五组ASan通过；新增声明测试再跑LocalAccess ASan通过 |
| 32位Host | Win32 Release LocalAccess及HTTP DeviceKey/MQTT通过；control receiver 84 B、provider 296 B、cloud wrapper 488 B，与此前一致 |
| ESP32同条件SDK构建 | 同§30公开WiFiQuickStart/Arduino 3.3.11：Plain Flash **1,027,812 B** / RAM **56,600 B**；Noise **1,036,736 B** / **56,832 B** |

本批两模式静态RAM均未增加；相对§30 Flash各增加200/192 B。Plain/Noise差8,924 B Flash及232 B静态RAM，仍不能据此认定加密是RAM瓶颈。LAN关闭构建和全部MCU矩阵本批未重跑，不能把此前结果称为本批新测；握手峰值heap、全部任务栈与socket仍待真机。

`WiFiProduct.h`仅因CloudTransport模板绑定profile而更新登录冻结清单hash，经审查登录body/协议没有变化，未关闭freeze门禁。编译日志与测试证据在ignored `build/lan-p2-20260909`；未改UI/native、安装APK、刷板、部署Service/Broker或向实际业务库应用030，也未暂存/提交/推送。

### 31.5 下一批P3固定顺序

1. **协调更新测试环境。** 先核对PC/手机网络、ADB与板角色、原备份和隔离self身份。当前最近确认WiFi为“带翅膀的蜗牛”，密码仅在ignored运行配置使用，不入例程/日志/文档。确认后更新协议包、Service/Broker及migration030和同合同App；部署前备份开发数据库/配置。旧HTTP v1 App不能与新服务混用，旧授权scope摘要变化不得被当成可无条件复用。
2. **备份后同步隔离门禁固件，先验证控制面。** 备份选定板的应用/NVS/OTA后才烧录，保留Hub/A/B拓扑和Key。核对当前proof签名的v2 Query、profile声明、精确Enable ACK以及HTTPS context/2；不能用手改DB模式、跳过证书检查或打开额外权限模拟ready。旧软件在隔离测试中恢复需要整体匹配，不能静默降级。
3. **普通WiFi默认Plain最小闭环。** 实测Challenge/Proof→Hello/verified Manifest/fresh State→Property与无参Action(null)→ACK/执行计数；用实际lane和MCU日志证明，不用Cloud成功代替LAN。
4. **有限生命周期/资源。** 页面退出/快速重开验证同authorization原截止时间、原账户MQTT session常驻；测真实到期、撤权、Key刷新与迟到回包。分别记录heap低水位、任务栈及socket回收；新caller/账户切换的单槽行为单独验，不提前承诺无缝。
5. **同脚本回归Noise后**再将已验证的self模块注入WiFi+BLE与Hub共享child，推进S4暖切换和多child；不恢复100次交替、不扩History/Bulk或页面编辑器。P2源码接线已完成，P1动态资源与P3物理闭环未通过之前，不宣布LAN正式发布或无缝切换完成。

## 32. P3 Plain真机闭环与ACK受阻问题（GATT§10.132）

### 32.1 本机配套更新与操作边界

本轮网络为“带翅膀的蜗牛”，PC私网地址192.168.2.76、MCU192.168.2.54；手机已解锁、ADB在线。继续复用原隔离self逻辑设备，不创建新设备、不刷新Key或修改原Hub/A/B绑定。COM12的MAC与既有备份匹配，只用于确认本次刷写目标，不用于应用身份或发现授权。

更新本机Gateway和该测试账户所在broker-shard-1镜像中的实际dist/协议包，应用migration030；两容器健康。协议包、HTTP handler与030文件的SHA-256与本地构建逐项一致。保留原受限HTTPS终止、可信proxy地址与USB reverse测试云端路径；手机→MCU实际走WiFi WebSocket。未修改其他broker shard或原TLS代理配置。

App本地调试配置临时开启LAN并使用原HTTPS127.0.0.1:17000，构建、安装debug APK，保留登录态。编译后源码配置已恢复LAN=false；release不信任测试CA。设备使用Arduino3.3.11/IDF5.5.5的默认Plain内部组合，保留原测试Cloud HTTPS/MQTTS；此次LAN Plain不等于取消其他秘密交付约束。

**备份纠正必须保留记录：**初版脚本误用`aily-services-postgres-1`容器的默认`postgres`库，实际Gateway连接`aily-iot-dev-postgres-1 / aily_gateway`（DNS为172.19.0.3）。030应用后才发现并补存正确快照，因此本批没有真正的Gateway迁移前快照；不能把错误目标dump或补存快照称为迁移前恢复点。030仅增加可空模式列，没有删除/回写绑定数据。本批不执行数据库回滚；原错误dump的用途已明确标注。后续部署先由运行中DATABASE_URL、DNS/容器与`current_database()`三者核对目标，再验证dump包含预期v2表/迁移记录，不能再用默认POSTGRES_DB猜测。

### 32.2 已取得的真实证据

- 新App/Service配套后、MCU尚未声明模式时，真实GET返回503 `LOCAL_ACCESS_PROFILE_REQUIRED`；默认Plain门禁启动后，通过当前proof签名Query和精确Enable ACK，数据库模式变为2，GET返回200 `local-access-context/2`及`private, no-store`。没有手改模式或跳过证书验证模拟ready。
- 共5次有界开页均取得LAN Ready及verified Manifest/fresh State。最终一次记录65 B Challenge→169 B Grant→37 B caller proof/37 B recipient proof→BBP Hello/Manifest/Accept/State；未调用Noise记录，未发原始sessionKey到LAN。
- 3组实际Property/无参Action控制均收到LAN ACK，MCU及State计数依次为1、2、3，没有因可靠重传重复执行Action。门禁在每次发送前检查LAN与Control权限，避免Observe-only时真实走Cloud却被统计为LAN。
- 首次`power=false→true` ACK约1248 ms；后两次保持true、不产生状态变化时约177/112 ms。三个button ACK约1241/1236/1094 ms。**仅为此次样本，不是分位或性能通过。** 上轮误用整数Action的测试输入已用null完成真实验证。
- 首次开页到LAN Ready约4491 ms，快速重开约3516 ms，其余约3.5～4.0秒；包含发现/HTTPS/同步/清理，不能当控制延迟。最后一次StatePage已到达后仍约2秒才公布LAN Ready，需单独量测发现停止/owner清理路径，不能未经分段验证归因WiFi或删除清理屏障。
- 关闭、快速重开后服务器复用同一个authorization ID，`expires_at`保持不变；不是不断占用新槽。原授权到期附近再次开页，只短暂使用LAN后回到Cloud，没有从重开时再给满300秒；可观察到接收结束和路由回收。手机与服务器墙钟未对齐，**不能将两个Date.now直接相减声称精确到期误差，也未独立证明App定时器与设备到期检查各自的拒发行为**。
- 全程门禁保留同一账户MQTT session基准，Cloud持续ready；退出LAN不重建账户MQTT。未通过接口绕过真实LAN发命令，也没有把不确定Action换路重发。

门禁首两次open在CDP返回对象阶段报错，但App内操作继续完成；检查已有checkpoint后继续，没有盲目重新打开或重发命令。helper现将限定字段快照先JSON序列化再返回，后续open/inspect/control/close实测正常；通用CDP错误只允许输出源码坐标，不输出任意异常/HTTP内容。此修正不修改App业务实现。

### 32.3 ACK迟滞的根因与下一批最小修正

真实Plain帧记录：Command序号6发送后约43 ms收到Patch；初次ACK未到。约1009 ms后App重发**同一序号6**，再收到ACK。已有可靠重放记录保证按钮只执行一次，故不是Action重复执行或加密慢。对应源码链路：

`DeviceRuntime::handleCommand → stateSettlementHandler(nullptr) → Client::flushPendingState → fanOutCurrentPatch → 唯一transport槽占用 → sendAck返回WouldBlock → 等App一秒重传 → cached ACK`。

`handleCommand`当前显式依赖“发送满时由peer重试”，安全去重基本正确，但在正常带状态回报的交互中成为固定延迟。**本批定位、未修改这段生产Runtime；P3不能因此标完成。** 下一批按以下顺序，优先公共语义，不能只为ESP32或Plain打补丁：

1. 增加真实单发送槽/有背压的Host回归，区分“业务已执行”“ACK待送达”“状态通知待发送”。先复现状态变化→ACK初次受阻，覆盖不变值、Action、同帧重传与多通道，不能使用永远send-success的mock掩盖。
2. 优先复用现有可靠接收窗口/结果元数据，记录有限的待补发响应，在transport取得进展后由公共Runtime尝试发送；不重新执行回调，不靠App缩短重试计时，不为每模式增加整帧队列或第二业务核心。具体字段/调度在回归约束下实现并测ABI，不能先宣布零RAM成本。
3. 响应必须保留原命令关联与执行结果/状态版本，目标仍是原transport/session；会话销毁、鉴权/Key代际变化或授权失效后不得向替代session发送。待响应容量/每poll工作量有界，长期拥塞不产生无限缓冲；保留现有同帧重放作为兜底，不做跨lane未知命令重放。
4. 保留向caller及其他合法peer的State传播，不能用“排除caller通知”或丢Patch换取低延迟。Host/ASan、双模式transport、Cloud以及资源检查后，再用同一Plain脚本证明状态变化命令无需等1秒重传、计数仍一次。
5. 然后分段验证发现清理至Ready的额外约2秒、真实撤权/Key刷新和设备侧过期拒发；同脚本Noise后才推广WiFi+BLE/Hub/S4，不恢复100次交替。

### 32.4 资源观察与恢复

诊断门禁Flash **1,124,508 B**、静态RAM **58,144 B**，含原测试CA/Cloud配置及新增**960 B**静态任务快照；不是§31公开例程的净LAN成本。13个任务快照完整取得：loop最低余量3392 B、httpd3340 B、ipc0/ipc1为1284/1276 B、mdns1652 B、wifi4468 B；IDLE0/1为232/352 B，仅记录SDK idle任务观察，不按应用任务门限一概判通过。

观察到空闲heap约194.4 KB、连接时约193.9 KB、最长连续空闲块143,348 B；整个测试过程最低空闲heap为**144,576 B**，包含Cloud/TLS与多次会话活动，不能归因单次Plain握手或作为Plain/Noise净差。任务快照不是连续采样，尚未完成所有运行阶段/其他平台/长时间资源准入。

结束已关闭门禁页面与trace hook，保留账户MQTT。COM12恢复本轮从ROM读取的`0x9000..0x33ffff`应用/NVS/OTA共3,371,008 B，SHA-256 `4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`，写后hash校验通过；隔离测试NVS/OTA另存供下次复用。COM11与COM3未刷写，原拓扑/Key不改。新测试服务和调试APK保留用于后续，不是生产发布。

证据在ignored `build/lan-p3-20260910`。本批Gateway/Broker TypeScript构建、Android debug构建、SDK编译、架构检查和门禁JS语法检查通过；未把§31的全套Host/App/PG测试计作本批重跑。未暂存/提交/推送。**当前主线：P3基础Plain业务证据已取得，先修受阻ACK，再完成其余生命周期与切换性能门禁。**

## 33. P3公共Runtime补发ACK与Plain延迟复测（GATT§10.133）

### 33.1 最小公共实现

已把§32中“已执行、仅因发送槽满而等peer重传”的正常路径改为有限的本地补发：

- `RequestReplayResponse::AckPending`复用现有去重结果枚举，不新增帧缓冲、计时器、Client或平台分支。Command/Event/Patch成功后的ACK与重复请求的cached ACK共用`sendRequestAck`。
- 仅`WouldBlock`保留待发状态；transport完成poll、确认Online后，自动补发每transport每poll最多尝试一次，早于自动Hello/outbox/telemetry。成功入队后不自动重发，永久发送错误保留原peer同帧重放兜底。
- 目标取原peer的transport/session，不保留RxContext指针；发送前重新检查Hello及当前Control权限。权限丢失取消自动补发，会话关闭/transport离线退休清除记录。窗口覆盖同时覆盖待发标志，容量不超过原可靠接收窗口；不会把旧结果发送给替代session。
- ACK body沿用原协议：Command/Event不额外添加revision；Patch保存**执行时**的revision，即使其他状态之后变化也不改成发送时revision。原fingerprint/sequence冲突检测、同帧去重与caller/其他合法peer的状态通知不变。
- 本批只补成功执行的可靠请求ACK；Error、Hello、Manifest等未加入新的通用自动响应队列。长期拥塞仍受原窗口及会话寿命限制，不保证无限缓存，也不做跨通道未知Action重放。

删除原Command/Event处理中“可靠发送失败一律只等peer重试”的正常路径，保留重放作为丢帧/拥塞兜底。用户API、BBP编码、App的1000 ms重试间隔与服务器合同没有变化。

### 33.2 回归与资源

新增真实`Client`状态settlement配合单TX槽的Host回归：拒绝的发送不能覆盖已接受帧，只有transport.poll推进才释放槽。旧Runtime下出现5项预期失败（受阻ACK未补发），修正后全组通过。覆盖状态变化/不变、null Action、同帧在待发及已发后的去重、其他transport状态通知、有限尝试、窗口替换、撤权、session替换、无disconnect回调的离线退休、永久错误、Event、session=0的transport级响应，以及Patch延迟发送仍返回原revision。

本批重跑并通过：Win32核心Host；ASan核心、Telemetry、LocalAccess合同、Plain/Noise transport、HTTP DeviceKey/MQTT、ESP32 WebSocket共6组；架构与diff检查。不是App/Service全量测试，也未声称真机Noise通过。

32位ABI前后相同：Small（三transport/三peer）Runtime464 B、Client764 B、Device1760 B；Standard Runtime644 B、Client944 B；Gateway Runtime1564 B、Client1888 B。没有新增静态RAM。相同私有配置/SDK3.3.11诊断固件Flash从1,124,508 B到**1,124,828 B（+320 B）**，静态RAM保持**58,144 B**，含960 B测试任务快照，不能冒充公开最小例程大小。

### 33.3 本轮真实手机—设备结果

保持§32已部署的Service/Broker及已签入debug App不变；通过真实App page port、Android native WebSocket与COM12隔离self执行，不绕到Cloud代发、不修改UI或重新安装APK。网络仍为“带翅膀的蜗牛”，PC192.168.2.76、MCU192.168.2.54。

| 本轮控制组 | power ACK | button ACK | buttonPresses | 相同Command重传 |
| --- | ---: | ---: | ---: | --- |
| 1，power false→true | 163 ms | 147 ms | 1 | 无 |
| 2，power保持true | 185 ms | 130 ms | 2 | 无 |
| 3，power保持true | 120 ms | 149 ms | 3 | 无 |

每组均记录verified Manifest/fresh State、LAN Control权限与发送/ACK后仍为LAN；每个Command序号5/6各只发送一次。状态变化时先收到Patch，随后即收到ACK，不再等一秒重传。计数1→2→3，原账户MQTT全程同一session且ready。3组是短程样本，不是分位性能、长稳或所有平台准入。

开页到LAN Ready分别3791/3537/3514 ms；另一次只测握手与清理、不发控制的开页为3521 ms。阶段hook不输出payload/secret，手机同一墙钟内测得：session建立（含Plain证明、Hello/Manifest/State）344 ms；session返回后1 ms触发scan abort；**scan abort→scan promise返回2031 ms**；之后5 ms公布LAN Ready。因此额外约2秒已定位在扫描退休阶段，不是控制ACK或握手本身。尚未分别给native unwatch和listener.remove打点，不能把2031 ms全算作某个Java函数的独立测量。

静态源码进一步指出：connector在成功同步后`scanning.abort(); await scan`；discovery finally等待unwatch和listener移除；当前安装的capacitor-mdns Android最后一个browser释放JmDNS lease时同步`JmDNS.close()`。这是下一批需要核验的调用链，**本批未修改插件/清理屏障**。

### 33.4 恢复与下一批准入

本轮13任务快照完整：loop3392 B、httpd3296 B、ipc0/1为1284/1276 B；观测heap连接193900 B、关闭后194416 B、全程最低183260 B、最大连续空闲块143348 B。样本比§32短、无相同Cloud重登录负载，不得据此宣称heap优化。idle水位232/352 B仍仅作SDK观察，未开展长稳。

COM12刷写前重新ROM备份`0x9000..0x33ffff`，大小3,371,008 B、SHA-256仍为`4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`；结束保存隔离NVS后恢复该备份，写后hash校验通过。App gate/trace hook已关闭，COM11/COM3及原绑定未改；未部署服务、迁移数据或刷新Key。证据为ignored `build/lan-p3-20260910/ack-*`及本轮时间戳App/trace JSON；本批未暂存/提交/推送。

下一批顺序冻结如下：

1. 先完善扫描退休阶段测量与ownership回归：已鉴权WebSocket的就绪、浏览器停止接收事件、native最终资源退休应分别定义。评估持有原owner的后台扫描退休，正常控制不等待共享JmDNS关闭；但页面关闭/换网/账户切换必须汇合退休结果，清理失败仍禁止第二native open。不能直接删除await、复用旧IP/鉴权材料或永久占用mDNS来掩盖。
2. 小范围验证重新开页、退出/取消、cleanup失败、网络/账号代际和原MQTT并行，然后再次手机分段测量；只有实际延迟和正确性同时成立才记发现/Ready优化通过。
3. 精确授权到期（分别验证App和设备）、真实ACL撤销/Key刷新、新caller单槽恢复仍未通过；Host撤权测试不能替代真机/服务闭环。
4. 同脚本Noise及之后的WiFi+BLE/Hub准入仍待执行。保持P3优先，不开展100次交替或扩展新网关协议。

## 34. P3扫描退休与LAN就绪解耦：软件完成，真机待环境恢复（GATT§10.134）

### 34.1 责任边界和实现

发现提供的是不可信地址候选，不是已认证连接的授权依据。保留原context、设备Challenge实例校验、Grant与Plain/Noise、Hello/verified Manifest/fresh State，只有这些检查完成、owner仍有效且session仍ready时才公布就绪；mDNS退休不再位于成功返回的关键路径。

修正仅位于App `local-access/connector.ts`，不扩BBP、Record策略、native WebSocket或MQTT接口：

- connector对现有byte link组合一个幂等关闭屏障，send/receive原样委托。成功认证后将扫描退休归入该link的生命周期，后台停止扫描；未匹配候选的关闭只关该候选，允许原扫描继续寻找其他实例。
- `session.close()`立即启动原native close，同时等待扫描退休。使用allSettled，任一关闭失败也必须等另一个资源处理结束；只在完整成功关闭后释放connector槽。扫描清理失败会使close失败、槽保持占用；确认已清理的普通扫描错误允许资源退休后重试。
- 后台scan错误触发原lifetime abort，关闭现有lane；不忽略rejection、不另建连接。页面/网络/账户取消仍通过既有owner立即终止native IO，旧结果不能成为新代际Ready。
- 特别补上“byte link已开始close，但BBP receive-loop尚未把session标成closed”的竞态：此时禁止转交扫描owner，走完整失败清理。否则一个已按旧边界开始关闭的link可能绕过新屏障。
- 删除原成功路径的`scanning.abort(); await scan`串行等待，改为成功后后台退休；失败准备路径也合并等待native与scan，避免一个先失败就遗漏另一个清理。没有修改插件/JmDNS、没有永久扫描、IP/Grant缓存、额外socket或重试调度器。

必须区分：页面scope的`closed`表示需求已取消，**不是native资源退休证明**；connector槽和`session.close()`才是本批物理关闭边界。清理未结束时再次open仍返回BUSY，当前没有新增等待队列或自动重开。原账户Cloud可继续服务，但这不等于任意快速重开都能立即恢复LAN，也不称整体无缝完成。

### 34.2 验证

新增延迟扫描清理回归在旧实现中失败（就绪结果仍undefined），修正后通过；connector共14项，覆盖Ready不等scan、close合并、取消立即关native、迟到scan错误、清理失败保留槽、native失败仍等scan、account/network迟到结果、已关闭link不能转移owner，以及原候选失配/授权错误不自动重试等。

本轮正式Angular入口运行**8文件106项通过**：LocalAccess协议/Client/Session/native link/discovery/connector、LAN service、DeviceUi LAN。App与spec TypeScript、connector/source test eslint、diff检查通过。成功构建local web及debug APK并安装，App停留于`/home/device`，既有登录态保留。

本批没有设备源代码/协议变化，未重跑§33的MCU编译/Host/ASan或Service历史测试。硬件准备复用§33已修ACK的同一Plain门禁binary，SHA-256 `8034b158636c5008d31790d886cfd81cdebea875569bbdc48775e97da51d6648`，不把这次刷写称为重新编译或新资源测量。App的新理论时延收益尚未取得真机数字；§33的344/2031 ms仍是旧App实测。

### 34.3 现场阻碍与恢复

手机196f65f9、COM3/11/12均在线；COM12的MAC仍98:3d:ae:ee:7c:6c，WiFi已连、IP192.168.2.54，PC192.168.2.76。原生链路准备没有改变原Hub/A/B绑定或Key。

本机Docker引擎未启动，受限HTTPS/MQTTS代理端口未监听。尝试启动原Docker Desktop后，后台日志明确报`initializing Inference manager ... dockerInference ... The file cannot be accessed by the system`，引擎退出；App Cloud为retrying，设备为starting。因此**没有执行/通过本轮真实Grant、LAN控制或性能门禁**，不能降级明文HTTP、手造授权或把离线失败视为协议缺陷。已请求在错误窗口Quit后重开，禁止恢复出厂设置；没有删除Docker IPC/数据盘、迁移数据库或重建容器。

刷写前COM12重新ROM备份`0x9000..0x33ffff`，3,371,008 B，SHA-256仍为`4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`。结束恢复该原应用/NVS/OTA并通过写后hash；COM11/COM3未刷写。旧debug APK另有`scan-App-before.apk`备份，新APK保留供复测。local源码临时配置已恢复LAN=false及原URL；只对debug使用原私有CA overlay，release不信任测试CA。

证据为ignored `build/lan-p3-20260910/scan-*`。已准备有界`scan-live-gate.mjs`：就绪时立刻检查scan是否仍在退休，然后仅发送一组已锁定的Property/Action；另有close模式等待精确session关闭，避免用page scope取消冒充资源释放。失败不自动重发Action，脚本尚未取得本批真实结果。未暂存/提交/推送。

### 34.4 下一批执行顺序

1. 先恢复原Docker测试环境，核对原Gateway/Broker/HTTPS代理健康、App账户Cloud ready和隔离设备登录，不创建新设备、不改Key/ACL。不能把“Desktop进程存在”当引擎和服务可用。
2. 复用本轮新APK及§33同一门禁固件，记录发现、握手/同步、Ready、scan退出；要求**scan尚未退出时真实Property/Action完成ACK**。另外执行退休期间取消、精确session.close汇合、屏障完成后的重开与同账户MQTT并行；不得把软件回归代替这批物理证据。
3. 通过后再做精确App/设备到期、真实ACL撤销/Key刷新、新caller单槽及Noise。P3仍优先，不转向100轮、Hub/S4或新协议。

## 35. P3扫描后台退休、关闭重开与App到期的真机闭环（GATT§10.135）

### 35.1 环境与实际范围

用户重启Docker后，实际引擎、原Gateway、三个Broker及HTTPS/MQTTS/WSS代理均可用；既有debug App登录态、账户MQTT ready和原隔离身份的HTTP context/2返回200、profile2、private/no-store。没有重建环境、重新迁移030、创建新设备、改Key/ACL或手造授权。Service§23的备份用途纠正仍适用。

手机196f65f9与COM12（98:3d:ae:ee:7c:6c）完成真实原页面连接端口→Android native WS→MCU链路。复用§34已安装APK和§33同一ACK修复binary（SHA-256 `8034b158636c5008d31790d886cfd81cdebea875569bbdc48775e97da51d6648`）；没有新产品代码、插件、协议、队列或用户API改动。本批实施重点是验收上一批生命周期修正，而非继续增加抽象层。

### 35.2 Ready、控制和资源退休分别验收

| 顺序/场景 | 开页→LAN Ready | session准备 | session返回→Ready | scan abort→退出 | power/button ACK |
| --- | ---: | ---: | ---: | ---: | --- |
| 1，改变power并调用button | 1660 ms | 313 ms | 5 ms | 2032 ms | 135 / 87 ms |
| 2，Ready后立即取消 | 1437 ms | 317 ms | 4 ms | 2031 ms | 不发业务命令 |
| 3，屏障完成后重开/控制 | 1603 ms | 365 ms | 4 ms | 2029 ms | 38 / 136 ms |

两组真实控制均在scan尚未返回时完成，完成后距离扫描最终退休还分别有1744/1797 ms。每组经过Hello、verified Manifest、fresh State和Control权限检查；Property与null Action各只发送一次Command（序号5/6），没有1秒重传，buttonPresses从1到2，无重复执行。第二组power未变化，因此38 ms不是状态变更的性能代表。账户MQTT全程同一session且ready，控制发送和ACK后均为LAN，不是Cloud代发。

取消场景：page.close时扫描未退休且connector槽仍占用；6 ms后开始native close，18 ms后native close返回；精确session.close在2001 ms后才完成，并在scan返回后释放槽。trace内可出现多个幂等close调用记录，不能据此计成多条socket。随后第三次重开成功。这证明“立即终止数据IO”和“汇合资源退休”分离正确，不是仅提前显示就绪或删除清理等待。

§33旧App的Ready为3514～3791 ms；本批同一固件下，已认证session的Ready不再等待约2秒扫描退休。以上仅短程分段样本，不是P95、长期稳定性或全产品无缝声明。仍保留清理期间二次open的BUSY屏障；其失败/取消/身份代际软件回归已覆盖，不能宣称任意快速重开立即回LAN。

### 35.3 App期限回收：不代替设备独立期限门禁

第四次只开页、不发送业务命令，LAN Ready为1425 ms。真实服务器仍使用同一authorization ID和原expires_at；重开没有续期。通过debug只读引用观察精确LocalAccessClient的单调时钟、expiresAt、close和session退休；仅把测试页面watchdog延至剩余期限后30秒，**未改实际授权、生产timer、时钟、native或路由策略**。

开始观测时剩余145343.4 ms。页面仍打开、无先行client错误时，client在自身期限后3.4 ms开始close；期限后37.0 ms完整session退休，槽为未占用，路由回Cloud，原账户MQTT仍是同一ready session。对这个已关闭LAN session执行只读refresh立即被拒绝，新增发送数为0。所有期限差值只使用同一手机单调时钟，不与服务器/PC墙钟相减。

结论只覆盖**App侧到期主动回收和已过期session不再发送**。由于App先关闭连接，不能据此判定MCU能独立拒绝过期Grant/业务。额外尝试到期后新Cloud控制时，测试页面watchdog已关闭需求，发送前准入拒绝；checkpoint没有命令起始记录，未重试、未计为Cloud业务ACK通过。该独立项保留到下一批。

### 35.4 回归、恢复与下一批

正式Angular入口重跑8文件106项全部通过；设备架构检查及三仓diff检查通过。本批没有重新编译设备/重跑Host、ASan、资源ABI或Service历史测试。相同诊断binary的短程采样：loop栈最低3392 B、httpd3344 B、ipc0/1为1284/1276 B；观测minHeap179040 B。不是新固件资源收益或长稳准入。

COM12先ROM备份`0x9000..0x33ffff`，3,371,008 B，SHA-256 `4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`；结束另存隔离NVS/OTA后，恢复该原始区域并通过写后hash校验。COM3/11未刷写，原绑定不变。App gate、trace及临时close hook已关闭/恢复；新debug APK、原服务保留。源码LAN开关仍关闭，没有暂存/提交/推送。

ignored证据：`build/lan-p3-20260910/scan-live-summary.json`及其关联三组JSON/trace、`scan-expiry-verify-*`、`scan-live-regression.log`、`scan-live-restore.log`。私有测试脚本仅观察生命周期和锁定一次业务发送，不进入库发布依赖。

下一批顺序收口：

1. 精确验证MCU独立到期拒绝（App主动close不能作为替代），并用**新的用户命令**验证到期回Cloud后的业务ACK；不跨lane重放不确定Action。
2. 原隔离身份上做真实ACL撤销/Key刷新与新caller单槽恢复。需记录Service→设备同步/拒绝，不用Host结果或数据库手改代替链路。
3. 同脚本Noise，再评估WiFi+BLE/Hub产品准入。普通self P3未关闭前，不转100轮、S4、多子设备新协议或UI扩展。

## 36. P3接收端独立到期、新caller与Cloud新命令（GATT§10.136）

### 36.1 测试如何排除App替代设备执行期限

核对发现Android `LocalWebSocketSession`也有300秒最大连接期限。因此仅停用JS的300秒timer、等待断开，仍无法证明MCU独立守期限。没有修改该原生保护或设备代码，而是经**原HTTPS正式签发合同**请求30秒和15秒Grant（合同允许1000～300000 ms），不手造Grant、不改服务期限/DB/设备时钟。

前两组仅在隔离debug会话中清除LocalAccessClient的主动到期timer；逐操作expiresAt检查、设备权限和Android原生最大期限全部保留。先实际完成Hello、verified Manifest、fresh State和一次LAN只读refresh，再静候底层断开；页面需求未退出。第三组保留正常App主动timer作对照。所有hook在结束后恢复，不进入生产源码或发布配置。

| 场景 | 实际签名期限 | App主动timer | Grant发送返回→native关闭观察 | 新Cloud power ACK |
| --- | ---: | --- | ---: | ---: |
| peer30，原caller | 30000 ms | 仅本测试停用 | 30066 ms | 165.7 ms，false |
| peer15，新随机caller | 15000 ms | 仅本测试停用 | 15439 ms | 158.6 ms，true |
| app30，正常App对照 | 30000 ms | 保留 | 29913 ms | 158.8 ms，false |

前两组首次LocalAccessClient.close均已有底层失败，页面未取消，且远早于原生300秒限制；与30/15秒Grant对应的接收端会话回收成立。第三组首次close没有先行client错误，在App自身单调期限后3.7 ms发生、42.1 ms完整退休，与主动timer对照一致。

表内网络间隔来自同一手机时间轴，起点是**发送返回，而不是MCU接收/准入时刻**。未在MCU逐poll记录单调时钟，不将66/439 ms解释为设备定时器本身的误差，更不作为硬实时上界。已经验证的是既有认证会话到期主动断开；未单独验证“仍打开的过期socket收到业务帧”或重新签发过期Grant，不能与Host边界测试混称真机结果。

### 36.2 单槽和Cloud业务恢复

三组均在精确session.close完成后确认connector槽释放；对原LAN session的refresh被拒绝，新增LAN发送数0。随后在原页面需求仍有效时，经原DeviceUi发送**新的**power命令，全部通过原账户MQTT收到ACK，状态依次false/true/false。它不是失败LAN命令的跨路重放，没有重发Action；本轮没有调用button。Cloud始终是同一账户session。

peer15由正式签发接口取得与原caller不同的随机caller作用域。在前一期限及15秒challenge墓碑保留窗口已结束后接入成功，未清除设备记录/刷新Key/重启固件来释放槽。此项只覆盖**到期后的新caller复用**，不代替ACL撤销/Key刷新后的新caller恢复、另一真实账户分享或并发caller门禁。

### 36.3 软件与恢复

本批重建并运行3组x64 MSVC/AddressSanitizer门禁：LocalAccess lease/control、LocalAccess transport/Runtime、ESP32 WS端口mock，全部通过；Service签发和public handler两文件19项通过，设备架构检查通过。最初Host进程因缺失ASan/Debug CRT运行时未能加载，在**当前进程PATH**补入已安装工具链目录后正式运行通过，没有安装系统组件或把未启动当测试失败。未重跑前批App106项/完整6组ASan/MCU交叉编译，不新增资源ABI测量。

沿原Docker环境、既有App/隔离身份及§33相同binary执行，无生产代码、native插件、协议、服务部署或迁移变更。COM12先ROM备份原`0x9000..0x33ffff`，大小3371008 B、SHA-256仍为`4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`；结束另存隔离NVS/OTA并恢复原区域，写后hash通过。COM3/11和原绑定未改，所有gate/trace/会话hook已清理；未暂存/提交/推送。

ignored证据位于`build/lan-p3-20260910/expiry-peer-summary.json`、关联`peer-expiry-*.json`/trace、`expiry-host-results.log`、`expiry-service-results.log`及`expiry-peer-restore.log`。私有工具只为故障隔离和有界测量，不构成用户API。

下一批仍是Plain真实ACL撤销/Key刷新及同步、拒绝和恢复，再复测Noise；不重复已通过的普通到期轮次凑数量。首页预发现按§37设计，不以增加连接池或放松鉴权换取Ready数字。

## 37. 首页前台预发现与短期候选目录：设计决定（实施进度见§40）

### 37.1 为什么开页仍需时间

当前connector在开页后先取可信context，再启动mDNS。§35的约1.5秒包含发现、建连和鉴权同步，不是全由扫描造成。本轮新增只读分段hook：

| 本轮场景 | 开页→scan | scan→首候选 | native WS open | session鉴权/同步 | scan后台退休 |
| --- | ---: | ---: | ---: | ---: | ---: |
| peer30 | 165 ms | 882 ms | 302 ms | 681 ms | 2026 ms |
| peer15 | 149 ms | 968 ms | 206 ms | 842 ms | 2022 ms |
| app30 | 203 ms | 865 ms | 266 ms | 702 ms | 2032 ms |

三组开页Ready约2.035/2.171/2.041秒，比§35样本慢；实际握手/同步有波动，不能把旧1.5秒当固定性能。预发现有机会移走约0.9秒的首候选等待，但不能保证去掉TCP/WS建立、HTTPS授权、Hello/Manifest/State，也不预先承诺亚秒结果。

### 37.2 采用的职责划分

1. **App登录后在前台设备列表/home提前发现**，复用一份mDNS browser和有界内存候选目录；不是每个设备/页面各扫描一份，也不是永久后台扫描。账户MQTT仍按原生命周期常驻。
2. 目录只存公开候选：service/locator、host/port、单调observedAt、network/account generation；总候选上限先保持16。不保存Grant、sessionKey、Control权限或新的持久IP数据库。
3. 打开设备页优先使用同网代、未过期候选；没有候选则订阅该目录的有界发现。每次仍需可信context、设备exact Challenge和实例校验、正式Grant及BBP同步；候选命中不等于Ready。未知结果不能跨路重放。
4. 退出页关闭自己的native连接/订阅；如果首页/其他前台消费者仍持有目录，不能由页面调用unwatch关闭全局browser。目录拥有browser最终退休；最后一个发现使用者退出、退后台、换网或注销时按职责停止/清空，并汇合清理。native失败和发现失败各守自己的资源边界，不能把一个旧owner的全局清理传播给新owner。
5. 当前插件的重复watch会替换watch；现有LocalLanDiscovery则是独占scan。**不能直接让首页和connector同时各调一次scan/watch。** 实施时必须先统一扫描所有权和订阅释放合同，再注入connector，保留原失败清理/代际回归。

### 37.3 必须保留的限制

- 现有MdnsService不暴露记录TTL；短期候选有效期需用App单调时钟定义并测试，不能声称自动继承DNS TTL。收到removed、换网/账户代际、前台重新进入时及时失效；缓存未命中/过期的有界刷新由唯一目录协调，不能另外起第二扫描。具体TTL和刷新节奏在实现批冻结，不先增加定时轮询池。
- 当前locator是设备启动生成的随机公开提示，不是MAC、逻辑deviceId或BLE PresenceKey。HTTP context/2未包含locator绑定，首页仅见一个服务时，**不能直接把某个账户设备标成“LAN附近”**。初版预发现可以先只加速候选选择；若复用此前已认证的短期关联，也必须受账户/网代/期限约束，真正建连仍重验实例。
- discovery可供普通WiFi/WiFi+BLE/Hub复用，但产品准入和self/child路由仍按现有阶段。不要为预发现增加网关专属协议、提前连接所有设备、预取长期授权或第二账户MQTT。

### 37.4 实施批次

先完成§36剩余Plain撤权/Key闭环；随后在self发现优化子批中：先候选目录及前台/订阅生命周期测试，再注入原connector，再做真机冷/热发现对照，Noise仍走同一连接与鉴权合同。至少覆盖多页面共享、缓存命中/过期、IP复用、设备重启换locator、退后台、换网、账户切换、清理失败和取消后迟到结果；验证页退出真的关native、后台不持续扫描、Cloud不重连。

本轮只完成分析/分段测量与执行顺序，**没有实现首页扫描、候选缓存或修改UI**。不能把这份设计当成已启用的预连接能力。

## 38. Plain真实Key刷新、撤权边界与恢复（GATT§10.137）

### 38.1 两种撤权不能用同一个ACK门禁代替

| 变更 | 接收端失效依据 | 验收依据 |
| --- | --- | --- |
| 分享ACL缩权/撤销，设备Key不变 | 原签名Retire清退指定authorization | 正式分享API、durable outbox、设备exact Retire ACK、旧会话关闭、重新分享后新授权 |
| DeviceKey刷新 | Broker撤销旧session；设备重登录取得明确鉴权拒绝，清空本地authority并进入AccessFault | 正式rotate、旧Key/session被撤销、未到期LAN实际关闭；新Key登录及新Enable ACK后恢复 |

**纠正此前“Key刷新也必须由旧控制面Retire ACK立即关闭”的表述。** Key更新会使旧proof和签名上下文失效，不能继续把它当成有效下发通道。rotate响应中的`disconnected:true`是云端处理结果，不是设备清退ACK，也不等于本地连接已同步关闭。普通MQTT断连不是撤权事实，不应为了缩短数字而终止所有有限LAN授权。

当前能力是在线时传播后失效、离线时由原有限期限兜底，不能承诺服务端点击后的零延迟撤权。接收端收不到权威变更时，既有LAN最多保留原剩余期限（当前上限300秒），不会续期；本批未模拟“刷新时设备云端完全不可达”，该最坏情形仍不能称为已完成真机测试。

### 38.2 已取得的真实旧Key失效证据

沿原Docker、已登录Android和COM12隔离身份，旧Key版本1先通过真实Plain/Hello/verified Manifest/fresh State及只读refresh，再调用正式`device-key:rotate`，一次幂等请求升至版本2，HTTP200/disconnected=true。没有手改DB、伪造Grant或刷新原Hub/A/B身份。

- 同手机单调时钟：rotate请求到返回156.8 ms；返回后5859.8 ms精确LAN session退休，页面未退出，授权还剩284094.5 ms。旧session再refresh拒绝，新增LAN发送0；connector槽释放，原账户MQTT保持同一ready session。
- MCU未重新烧录/重置就进入`state=fault`、`productError=23`（AuthenticationRequired），network/local/cloud均0。与现有明确鉴权拒绝后`clearCredentials → authority.clear / Client.end`路径一致；不是App300秒timer或native最大期限关闭。
- 只读DB核对：旧Key revoked、新Key active；17条旧版本session全部revoked。authority版本2/revision3/disabled，实例/proof清空；旧receiver仍为版本1且synced_enabled=true。**不能只读receiver布尔值宣称就绪**，当前scope必须核对Key/proof/authority和ACK屏障。新context实际返回409 AUTHORITY_CHANGED。
- 13条旧授权已撤销且retirement仍未决；不能将其写成Retire ACK通过。新Key重新登录后的新Enable ACK才可按既有合同将旧代际退休记为superseded，而非假造delivered/acked。

本轮约5.86秒只是一个在线样本，包含重连/鉴权/接收端调度；默认MQTT重连参数为5秒，但未逐阶段测量，不能把差值全归为该参数或当性能上界。没有在这个传播窗口注入业务帧，不声明窗口内每个时点的拒绝行为。

### 38.3 回归、恢复与下一批

补强`testLocalCloudControlComposition`：新Key先用编码签名的Enable/ACK fixture使authority启用，再注入明确DeviceKeyRotated响应，验证清空身份、旧签名Enable不能复活、连续poll不反复重试被拒Key；普通refresh仍不清退授权。复用原fixture，无新增生产接口、队列或运行时状态。4组x64 Host/ASan（公共Runtime/生命周期、LocalAccess lease/control、LocalAccess transport、HTTP DeviceKey composition）通过；签发/public两文件19项通过，架构检查通过。

另外以临时容器、只读源码挂载运行3文件真实PostgreSQL/worker/carrier测试，**33项全部通过、无skip**；使用既有测试helper创建/删除随机`test_local_access_*` schema，不改业务schema/原身份，不启动第二套服务，临时容器已退出移除。数据库不暴露宿主端口，使用原Docker内部网络，没有为测试新增端口或放宽防火墙。这组fixture覆盖不能代替第二真实账户真机分享。

**新Key恢复已通过**：保留本轮隔离NVS/OTA，只刷写含版本2新Key的应用；设备以同一逻辑身份登录，版本2 session active，authority revision4/enabled，新receiver版本2/sequence1/revision4/profile2与exact Enable ACK同步。13条旧退休记录全部变为superseded、delivered仍false，符合跨Key代际合同；GET context/2恢复200。

手机随后重新完成LAN鉴权/Hello/Manifest/State和只读refresh，新的power=true走LAN、ACK233 ms；主动关闭精确LAN session、页面仍有效后，新的power=false走原Cloud、ACK108.3 ms。账户MQTT同一session、无Action或跨路重放，connector槽释放；MCU日志active、network/local/cloud均1，power依次1/0。此项没有发起另一真实caller/账户，不能与分享恢复混称。

SDK3.3.11重新编译成功，Flash1124828 B、静态RAM58144 B（含960 B诊断workspace），与旧门禁相同，无新增生产资源；binary1124976 B，SHA-256 `9f0bf36cb531b6d4caf0bb81306d69d6d5d5ac22718f917ab18289a6671ee21a`。只改变私有Key/构建时间，未重建App/修改生产固件接口。§33旧binary仅可作历史证据，**不能再用于此隔离身份登录**。

隔离Key已实际刷新且保持版本2，旧Key不可恢复使用；新Key只留在ignored私有响应/配置和隔离fixture，不写文档或日志。COM12先ROM备份原`0x9000..0x33ffff`3371008 B（SHA-256仍`4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`），结束另存版本2隔离NVS/OTA并恢复该原区域；其他板和原Hub/A/B绑定未改。App测试hook/页面已清理，源LAN开关仍关闭，未暂存/提交/推送。

证据位于ignored `build/lan-p3-20260910/key-peer-*`；后续隔离门禁使用更新后的`GateConfig.h`、隔离fixture、同目录`key-peer-after-nvs-ota.bin`及上述新binary。恢复原板角色不等于回滚服务器隔离Key；不重复rotate来重试不确定请求。

下一批仍需第二真实账户通过正式分享API进行ACL撤销/重新分享：只撤销目标授权，不误杀owner；断开必须发生在期限前并有对应Retire ACK。没有找到可直接复用的第二账户登录材料，本批不伪造用户token或直接插入分享行冒充真机通过。完成Plain剩余项后沿原合同Noise，再按§37实施单一前台候选目录；不扩网关协议。

## 39. P3 Noise真实控制、篡改/重放拒绝及到期（GATT§10.138）

第二真实账号材料尚未确认，ACL撤销/恢复仍未通过，不反复搜索或用模拟账户宣布完成。Noise record模式与真实分享主体是两个独立维度：本批在不改变签发/权限/原账户MQTT合同的前提下，前移同一隔离self身份的Noise验证；不是跳过ACL发布门禁或转向Hub/S4。

执行范围：独立私有Noise配置与构建目录，保留§38 Plain版本2配置/binary；COM12先备份原区域再刷写测试模式，结束恢复。用原App生产端口验证可信profile1声明、exact Grant、Noise与Hello/Manifest/State、Property/Action ACK、精确关闭/重开、有限期限及Cloud新命令。硬件结果与软件回归分开记录，未完成项不得先勾选；不刷新Key、不手改模式/ACL数据库、不安装第二连接栈。

软件准备：原8文件106项App回归先通过；在既有client fixture新增发送/接收两个取消后迟到crypto结果测试，验证缓冲清零、native仅关闭一次、Grant/握手后不再发送业务记录，完整8文件108项再次通过。复用原代码，没有新增生产状态/缓冲。5组Host/ASan（核心/生命周期、LocalAccess lease/control、双模式transport、HTTP DeviceKey composition、Noise golden）与Service签发/public19项通过；不把§38的PG33项当本批重跑。

真机计划通过正式接口请求90秒有限Grant，复用同caller/scope做有界重开，不改变生产默认300秒，也不因重开续期。分别只在只读refresh记录上注入一次密文tag篡改/已用密文重放，检查提前退休和零旧会话续发；末轮仅在隔离debug会话停JS主动timer，保留逐操作、原生和MCU保护，以排除App代替接收端执行期限。所有测试hook结束恢复，不进入发布构建。

### 39.1 五个场景的实际结果

沿原服务、手机196f65f9和COM12隔离身份，保持Key版本2。新Noise固件通过现有proven login/v2 Query声明profile1及exact Enable ACK，receiver version2/revision4/sequence1/profile1；HTTPS context/2实返200/profile1/private-no-store，未手改DB模式。原生WS端口观测65 B Challenge、169 B Grant、双方各52 B Noise handshake，后续均为LocalSecureRecord(type3)，没有raw BBP。实际Hello/verified Manifest/fresh State成功，不以Cloud ACK冒充LAN。

| 场景顺序 | 开页→LAN Ready | 已取得的结果 |
| --- | ---: | --- |
| 控制 | 1826 ms | power=true ACK314 ms；button(null) ACK305 ms，计数0→1 |
| 篡改 | 1929 ms | 只读refresh的一条密文tag翻转1 bit，拒绝并关连接；旧会话再读0发送 |
| 重放 | 2281 ms | 一次正常只读refresh成功后重放其中已用33 B密文，拒绝并关连接；旧会话再读0发送 |
| 关闭 | 1381 ms | 页面取消，精确session.close汇合1743.8 ms，槽释放后可重开 |
| 到期 | 1482 ms | 重开后仍只有原剩余约56.5秒，接收端独立退休，新Cloud power=false ACK168.1 ms |

前两项负测各仅注入一次，页面未退出且授权分别仍剩约68.2/63.2秒；不重试Action、不降级Plain、不自动跨路重放。按同手机trace，异常记录send开始→native close开始分别183/38 ms；注入→精确session退休则为1636/1607.2 ms，因为必须等待仍在退休的mDNS扫描。**后者不是加密校验耗时**；关闭场景native开始→返回仅14 ms，其余主要是扫描屏障。没有为了改善数字删除该屏障或增加连接槽。

五次开页沿同一90秒authorization ID和原expires_at，重新建立Noise计数器不续期。末轮只停JS主动timer，原生300秒限制不在此次约56.5秒剩余窗口内触发；旧会话在App保守单调截止时间后318.3 ms精确退休、再读0发送，页面仍有效，原账户MQTT同一ready session，新Cloud命令ACK168.1 ms。该差值包含授权发送/接收和事件处理，不是MCU期限精度；此次验证既有会话到期回收，未注入“过期Grant”或过期业务帧。全轮唯一Action仍为第一次button，最终计数1。

### 39.2 资源、软件与恢复边界

- SDK3.3.11诊断门禁：Flash1131664 B、静态RAM58376 B（含960 B任务快照workspace），比§38 Plain增加6836/232 B。Noise额外带空GCM正确性自检，因此该Flash差不是最小产品/纯算法净增量。独立Noise binary1131808 B，SHA-256 `7612bf693e4fbcee1f72c5929b2fbbff439520d2716e4b084abccf03134b1204`；Plain配置/binary未覆盖。
- 短程采样loop栈最低3392 B、httpd3344 B、ipc0/1为1276/1284 B，minHeap182424 B、最大连续块143348 B；会话退休后idle heap约194200 B。包含Cloud/TLS及多次活动，不可用作单次握手峰值、Plain/Noise净heap差或长稳无泄漏结论。`local=1`代表本地服务可用，不等于仍有App连接；精确会话已关闭且connector槽空。
- App 8文件108项、spec类型/lint、5组Host/ASan和Service签发/public19项、架构检查通过。新增仅两个App清理回归；未增加生产API、缓冲、协议或连接栈，未重建APK/部署服务/迁移业务DB。§38的PG33项不计作本批重跑。
- COM12先ROM备份原区域3371008 B，SHA-256仍`4774677557f7c77eab7964a87c7142dbfef2ec731d80dcc81e276ce6d672b3fd`；结束另存Noise隔离NVS/OTA并恢复原区域，写后hash通过。COM3/11和原绑定未动，隔离Key保持2；App测试page/注入/计时/trace hook清理，源码LAN仍关闭，未暂存/提交/推送。

证据为ignored `build/lan-p3-20260910/noise-summary.json`及关联`noise-*`。复测Noise使用`NoiseGateConfig.h`、`noise-firmware`、`noise-after-nvs-ota.bin`；普通Plain ACL门禁仍用§38的版本2材料。最后认证的服务器profile为1；恢复原板角色不等于把该隔离身份的服务器声明强制写回2，下一次Plain启动必须用新的合法proof/Query/ACK重声明。

### 39.3 下一步与未通过项

优先补第二真实账户正式分享→撤销→设备exact Retire ACK/提前关连接→重新分享恢复，同时确认owner没有被错误撤权。真实账号材料仍待确认，软件fixture不作为替代。本批没有进行Noise Key再次刷新、不同账户/并发caller、其他MCU或网关多child真机门禁。

若账号条件尚未具备，可先推进§37候选目录及共享生命周期的独立软件测试，不必重复已通过的过期轮次等待；必须保留ACL未通过标记，不以Ready优化宣布P3完成。首页预发现尚未启用，WiFi+BLE/Hub/S4、全产品无缝及正式发布仍不在本批结论内。

## 40. 前台共享候选目录、接线和缓存失效边界（GATT§10.139）

本批实施§37独立发现子任务；真实第二账户材料仍待确认，不代替ACL门禁。设备协议、产品组合、MQTT、签发及页面布局均不改，只替换App发现层的独占使用方式。

### 40.1 初版策略与职责

| 层 | 本批唯一职责 |
| --- | --- |
| `LocalLanDiscovery.browse` | 原capacitor-mdns端口：监听当前type/domain的resolved/removed、校验公开提示、精确unwatch/移除自己的listener；保留原生所有权失败闭锁 |
| `LocalLanDirectory` | 单一共享扫描轮次、16个候选/最多16读者、15秒单调有效期、service替换/removed失效、最后读者退休屏障 |
| `DeviceV2LanService` | 一个账户/网络/前台代际一个目录；前台预热、退后台/换网/注销清空；新代际等待旧browser清理 |
| 原connector/native link | 仍只占一个业务socket；开页消费候选，真实context/Challenge/instance/Grant/BBP就绪后交给原页面owner |

具体冻结如下：

1. LAN开关、Android原生能力、可信HTTPS、已登录和WiFi均满足时，进入前台（含首页）先启动**一个最多10秒的预热轮次**，不依赖每个页面组件重复start/stop。原服务常驻监听代际，不常驻扫描，不获取设备context/Grant，不连接WS/MQTT。
2. 浏览器与正在开页的消费者共享；已有轮次的新读者只享用该轮剩余扫描时间。候选缓存按最近resolved的单调观察时间保留**严格小于15秒**，最多16项/实例名记录；IP/locator变化替换旧服务记录，removed无需旧TXT即可移除。locator/服务名依然不是logicalId；不展示新的“LAN附近”在线标记。
3. 10秒轮次结束不自动重扫。缓存由前台代际继续持有，过期项消费前剔除；之后用户开页按需启动新一轮，先给未过期候选，再收新发现。选择该边界是因为插件不提供DNS TTL、也不保证反复发出未变化的resolved事件；不能把15秒当DNS TTL，不能承诺在首页停留任意长时间后仍热命中。
4. 单个页面释放只移除自己的读者；其他消费者仍存在时，不全局unwatch。最后读者必须等待原生unwatch/listener清理；新轮次不得覆盖仍退休的watch。退后台/注销/换网/销毁立即关旧目录并清空提示，快速多代际变化也汇合全部旧清理；旧回调不能污染新目录。清理未确认则闭锁后续扫描，普通扫描失败且清理成功可在新的需求下重试。
5. 已认证socket不依赖后来其他消费者的公开发现结果。页面仍关闭自己精确native/session；它实际持有的发现清理责任仍须兑现。没有为减数字删除清理屏障或新建socket池。

### 40.2 缓存地址失效不能导致盲目重发

候选可能在15秒内下线、换IP或被另一设备复用。复用IP仍需原实例校验；只有`LOCAL_ACCESS_DEVICE_MISMATCH`可在签发前换候选。另补一个窄边界：native open的指定不可达/目的地拒绝/建连期限错误，在**尚未交出byte link、没有发送Grant/业务且close成功之后**，才带`LOCAL_ACCESS_CANDIDATE_UNAVAILABLE`标记；connector仅在`ports.link`阶段消费它。保留总4候选/30秒准备预算，没有缩短既有15秒native open期限。

取消、平台BUSY/不支持、清理失败和普通未标记错误均不作为继续依据；即使相同标记出现在session/Grant/Noise/BBP阶段也不可重试。并发取消不能遮盖cleanup失败而释放槽。此项属于建连前的候选选择，不是业务重试或跨路重放；死地址仍可能消耗原建连预算，不能宣称所有缓存命中都更快。

### 40.3 本批验证、清理和未通过项

已用正式Angular入口通过**15文件201项**：目录共享/期限/容量/removed/替换/取消、迟到回调/失败清理、账户/网络/前后台/销毁代际，真实connector+目录组合的缓存错误实例继续校验、原生建连清理标记及上限，以及原LAN/Direct/网关路由、需求和前后台/网络回归。App/spec TypeScript、改动文件eslint及设备架构检查通过。属于软件/原生mock验证，不是手机多播或WS真机。

旧独占scan的10秒/seen去重策略已移至目录，不保留第二个发现实现；raw browser只负责已安装插件适配。没有修改node_modules、Android插件、设备源码或Service。三个环境LAN仍false，未重建/安装APK、未刷板、未创建/刷新身份/Key/ACL、未暂存/提交/推送。证据：ignored `build/lan-p3-20260910/directory-*.log`；§39的Host/Service/真机数字不计本批重跑。

**下一批：**先构建隔离debug包，沿同一普通WiFi身份做手机首页预热→开页控制→关闭、冷/热/过期发现对照，确认只有一个watch与原账户MQTT，分别测候选等待、建连、鉴权/同步；再测退后台/换网的native清理与恢复。当前10秒为JS轮次预算，不是Android暂停/JS挂起后的硬实时扫描上限；现有mDNS插件只见destroy清理，没有pause钩子，必须真机核实，必要时在插件维护仓收口原生生命周期，不能偷偷修改node_modules。尚未承诺Ready缩短多少。

并行于上述独立条件，账号具备后补真实第二账户分享→撤销→Retire exact ACK/期限前关闭→重新分享及owner隔离。该项仍未通过，P3、WiFi+BLE/Hub/S4和全产品无缝未完成；不扩新网关协议或100次循环。

## 41. Android预扫描生命周期真机与网络对照边界（GATT§10.140）

### 41.1 仅合并推测性预热，不延迟页面需求

真实Android resume先触发foreground，约21 ms后才触发network generation。§40立即预热会先watch、随即因新代际unwatch，后者实测约2017 ms，清理后才第二次watch。原所有权屏障正确，但这次推测性启动没有必要，不能通过删屏障或并发watch掩盖。

只在`DeviceV2LanService`对新scope的**预热延后250 ms**，相邻代际立即失效并取消旧timer；显式`open()`取消尚未开始的timer并直接走原connector，不额外等待250 ms。正在运行的watch仍按原目录共享/精确退休，没有第二套发现策略、轮询、socket、Grant或MQTT连接。新增resume/network合并和立即开页两项回归。

安装修订debug APK后，手机正常首页恢复的观测是network事件后约251.3～252 ms启动**一次watch**，不再经历那次约2秒无效退休。另一个完整无设备轮次在watch开始后10002.4 ms发起unwatch，2034.7 ms后返回；这说明10秒是开始退休的JS预算，**不是native总占用10秒**。

真实HOME退后台时，当时活动watch发起精确unwatch，2022.6 ms返回；最终scope=false、watch/listener均0、connector未占用，原账户MQTT仍同一ready session。再回前台正常单轮，最终清理同样为0。只验证了本机正常生命周期事件已交付的情况，不能当作JS被挂起、杀进程或所有Android设备的原生硬上限；插件pause钩子问题仍保留。首页无候选时未获取context/Grant、未开设备WS。

### 41.2 mostfun2026：关联成功不等于云端/LAN业务可达

按用户新网络制作ignored `WarmGateConfig.h`和独立`warm-firmware`，不覆盖§38 Plain/§39 Noise配置。只修改网络、当前本机地址及诊断；Key维持2，使用原隔离身份，不新增绑定/ACL。

- 先出现SDK reason201（NO_AP_FOUND），用户确认天线。手机扫描同SSID同时存在2437 MHz与5200 MHz，不能将其直接认定为仅5 GHz网络。XIAO ESP32S3只支持2.4 GHz，参见[Seeed官方WiFi说明](https://wiki.seeedstudio.com/xiao_esp32s3_wifi_usage/)；reason201也不是密码错误的证明。
- 在内部Sketch增加断连reason及SDK配置**相等性**诊断，不打印SSID/密码/Key。重新烧入后，实际ssidMatches=1、credentialMatches=1、channel=6、pinnedBssid=0，WiFi已连接并取得192.168.1.144。未改连接策略，因此不把“再烧录后连上”写成根因修复。
- 设备随后持续network/local=1、cloud=0、state=starting；手机到PC的17000/18883 TCP成功，到MCU的8920 TCP失败（No route to host），PC邻居记录对应正确MAC但Unreachable。这些是采样事实，尚不能归因于路由器、RF、驱动或产品代码中的某一层；需要明确2.4 GHz网络对照。
- 手机经原ADB reverse的HTTPS/账户MQTT正常。直接fetch PC地址的TLS失败另有debug CA仅允许127.0.0.1/旧192.168.2.76的限制，**不能拿它证明PC TCP端口被防火墙阻断**。未关闭TLS校验、扩大CA信任或变更防火墙。

本机Gateway公开地址已改192.168.1.140，TLS代理换为原CA签发、含新IP的短期leaf，只重建这两个现有容器；未迁移业务DB、更新身份或部署第二套服务。App继续走127.0.0.1的精确debug CA域及ADB reverse。切网后需重新核对PC地址/证书/固件配置，不能只更换SSID。

**尚未执行**首页热候选→开页的Ready/真实控制、冷/热/过期对照和新网络恢复；没有把旧服务器profile1/context200或账户MQTT ready当成此Plain设备已经登录。下一次必须正常proven login/Query/Enable ACK同步profile2，再签发并测分段时延。不重试Action、不放宽鉴权、不以旧批次控制证据代替本批。

### 41.3 验证、恢复与继续入口

App15文件203项、App/spec TypeScript、改动文件eslint和设备架构检查通过。SDK3.3.11最终诊断构建Flash1125928 B、静态RAM58144 B（含960 B任务workspace），binary1126080 B、SHA-256 `a6da86f24396529d77e255c335bcab0f53221f358858f1ea3c801ae6b9f9a87a`。额外WiFi配置快照/事件日志只在内部Sketch，不是生产资源净增或长期稳定性数据。未重跑Host/ASan、Service/PG历史门禁。

本批COM12原区域3371008 B的备份SHA-256为`a50aabd4fcf3011d901cfa7619b002414eda344f159d3bb19b0f939512c548a6`，与旧备份差在存储/OTA区域，原应用区相同；**不能恢复旧整区备份覆盖本批开始前的存储**。结束保存`warm-after-nvs-ota.bin`28672 B后，用本批原备份恢复0x9000..0x33ffff，写后hash通过。COM3/11及原绑定未动，Key仍2；App测试hook/观察订阅已恢复、无活动watch/listener/session，安装包保留本批debug实现，三个源码环境LAN=false。未暂存/提交/推送。

证据在ignored `build/lan-p3-20260910/warm-*`（私有网络/Key/容器环境文件不提交、不全文输出）。下一批先取得用户确认的2.4 GHz对照网络，检查真实association→服务可达→登录/ACK，再沿原单身份做冷热/过期控制；若仍失败，拆出最小WiFi数据路径诊断而非继续扩大LAN协议。真实双账号ACL撤销/恢复、原生挂起边界、Hub/S4仍分别未完成。

### 41.4 YYSC对照与候选回退的诊断陷阱

同批用户已切YYSC并再次确认密码。PC192.168.50.13、手机192.168.50.106，手机使用2462 MHz（2.4 GHz/信道11）；手机到PC的17000/18883 TCP均成功，原CA签发的新leaf和固件CA指纹/签名验证通过。只再次更新本机Gateway公开地址和TLS代理挂载，App仍用原ADB reverse/精确信任域。

独立`YyscGateConfig.h`/`yysc-firmware`构建成功：Flash1125928 B、静态RAM58144 B、binary1126080 B，SHA-256 `17c802a50fdd9a27f2c4bc0896292ce899fe5a868d5291aa130835395855fa0b`。刷写前另存`yysc-before-region.bin`，3371008 B、SHA-256与§41.3原备份相同；测试沿用隔离存储/Key2，不擦除绑定。

初始SDK配置SSID/密码与YYSC一致，WiFi认证阶段出现reason2、未获IP。后续快照却出现SSID/密码均不再匹配：既有`WifiConnectionLifecycle`在新候选30秒失败后回退已确认旧配置。因此后续断连日志不能全部继续记作YYSC，不能通过无限等待这个回退状态来评估新网。reason2（AUTH_EXPIRE）可表示本地认证超时，也可由AP返回同码，不能仅凭它确定报错来源或认定密码错误，参见[Espressif原因码说明](https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32s3/api-guides/wifi.html#wi-fi-reason-code)。

这揭示了需独立复核的易用性边界：事务式BLE换网的候选回退与Sketch显式指定网络的重试预期是否应采用相同策略。暂不在网络根因未定时修改共享状态机；先用ignored的纯官方WiFi API程序只连接YYSC、输出该目标的信号/信道及双向TCP探测，排除存储回退/MQTT/LAN的干扰，再决定是否需要最小策略修正。该对照不使用DeviceKey、不读写Blinker绑定，不作为LAN Ready/控制通过证据。

**最小对照已实际执行**：同一COM12/SDK3.3.11，程序仅包含官方WiFi API，persistent=false、autoReconnect=false、WPA2最低安全门槛；扫描后只调用一次`WiFi.begin(YYSC, password)`，没有Blinker、TLS、MQTT、mDNS或产品回退。扫描8项、目标1项，信道11、RSSI−55 dBm、auth=7（WPA2/WPA3 transition）；随后两次reason2、WL_DISCONNECTED、无IP。没有进入TCP主动/被动验证分支。它证明此认证失败不要求Blinker上层参与，但**不能进一步证明SDK有bug、WPA3不兼容、网络故障或硬件损坏**；接收信号良好也不等于双向射频正常。

最小构建Flash858424 B、静态RAM45656 B，binary858576 B，SHA-256 `3f922e6a7b607fc3cbe5143e03c15161709fe389112ebf44a1bf48aaa8a5342f`；仅ignored `YyscWifiProbe`，不是新增公开库/示例。另存产品测试及最小对照结束时各自的NVS/OTA快照后，COM12已恢复`yysc-before-region.bin`原整区并写后hash通过，其他板与绑定不变；App观察hook已经清理，账户MQTT ready。三端diff检查通过，没有提交/推送或阶段性暂存。

**继续入口：**先由用户确认可用的独立2.4 GHz/WPA2-PSK网络（手机还看到了YYSC-iot，但不擅自假定其访问权限/密码），用同一最小程序比较；如仍失败再交叉另一块板或SDK版本。基础WiFi验证成功后才重装已保存的YYSC产品门禁、确认真正登录/Enable ACK，再执行已准备的冷/热/过期脚本。手动模式候选回退语义列为独立复核项，不因本次底层失败就删除BLE事务的回退保护。

## 42. i3water网络对照、手动网络意图与冷热LAN闭环（GATT§10.141）

用户已提供i3water，现场手机确认2.4 GHz/WPA2，PC172.20.10.3、手机172.20.10.2；本批按同一COM12/SDK官方WiFi-only对照→原Plain产品登录/授权就绪→冷/热/过期LAN闭环的顺序完成。私有配置、leaf和现有本机Gateway地址同步更新，原CA/Key2/身份不变；不把账户MQTT ready或最小TCP成功当LAN业务闭环。

同时实施§41.4确认的独立语义修订，不以此解释纯SDK的认证失败：

1. 用户在Sketch显式指定网络时，配置失败后按原退避预算重试**同一候选**；不能自动选旧SSID，也不能首轮失败后永久停在CandidateRejected。保留原单份workspace/原状态机，不在loop重写Flash。
2. BLE/WiFiProv事务仍默认候选失败回退已确认配置，若没有可回退项则等待新配网。不改变存储格式或将未连接网络伪标为confirmed；连接且durable confirm成功后才允许云端使用。
3. 失败策略由产品composition在生命周期启动前注入，运行中不能切换。普通WiFi产品的ESP32/UNO R4/Wio复用同一选择，手动Edge Hub采用相同语义，WiFiProv图保留默认；不增加用户参数或复制MCU状态机。
4. 补首次候选、已有旧网络、start失败、超时/时钟回绕、durable确认失败、运行中拒绝改策略和新配置代际测试；原事务回退测试必须继续通过。修订是否经过真机/编译/Host门禁分别记录，不提前标完成。

### 42.1 先验证基础网络，再验证产品

同一COM12（MAC仅用于确认本次烧录目标，不作业务身份）及Arduino ESP32 3.3.11，先保留隔离身份NVS运行完全不含Blinker的官方WiFi-only程序。i3water实际为2.4 GHz/信道6/WPA2，设备RSSI约−30 dBm、取得172.20.10.4；MCU→PC17000及手机→MCU8920的TCP对照通过。与YYSC相比，AP/信号强度/认证组合都变化，**不能据此认定WPA3或SDK就是此前失败根因**。

随后只刷产品app，运行原Plain LAN composition与本批网络策略修订；未刷新Key或清除绑定。串口显示约2496 ms网络可用、5540 ms进入active，network/local/cloud均1且无productError。正式HTTPS context返回200/profile2/private-no-store，与设备profile2一致，不再使用之前Noise的旧profile1判定。没有修改SQL、伪造receiver ACK或扩大授权。

本机仍为原Gateway/Broker/数据库。仅将Gateway公开地址及原CA签发的TLS代理短期leaf更新为172.20.10.3，重建这两个现有容器。App保持原ADB reverse HTTPS/账户MQTT及debug信任域，没有放宽证书检查；本批不含Service生产代码/迁移、App源码/APK改动。

### 42.2 三次真实开页与一次业务控制

沿§41已经安装的debug APK，通过真实DeviceUiPort页面owner及正式context/授权/native WS/BBP路径验证，观察hook不替换通信实现；不是布局按钮点击测试。下表各一个样本，同一手机单调时钟，不能当p95或最大延迟：

| 场景 | 前置事实 | 页面到LAN Ready | 候选交付 | native open | session建立 | 完整close等待 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 冷开页 | 无缓存、无watch、connector空闲 | 3657 ms | 923 ms | 1139 ms | 1066 ms | 1860 ms |
| 首页预发现后开页 | 回前台真实预扫描、候选新鲜 | 969 ms | 0.5 ms | 324 ms | 295 ms | 51 ms |
| 缓存过期后开页 | 原候选年龄15018 ms、无watch | 1574 ms | 850 ms | 78 ms | 243 ms | 1851 ms |

总Ready还包含可信context/授权和调度，表内阶段不要求相加等于总数。冷/热的native及session也不同，不能把全部约2.69秒差值都归因于预发现；能够明确证明的是热候选交付从本轮扫描等待变为约0.5 ms，且本样本页面Ready不足1秒。不是跳过鉴权/Manifest/State或预连设备实现。

三次均取得verified Manifest与fresh State。只在热开页发送一次`power=true`及一次`button(null)`，ACK为249/374 ms，发送和ACK时路由均为LAN；过期后重开页实读power=true/buttonPresses=1，未跨路重放Action。账户MQTT始终原ready session、watch峰值1。

热开页退出时精确设备连接/connector已关闭，但首页预热仍持有原单轮watch（2个listener），这是共享owner仍有需求，不是连接泄漏；轮次结束自然退休。冷/过期退出完整close等待含约1.85秒原生发现清理，不应误称为socket或密码处理耗时。最终stop核验connector空闲、round/watch/listener为0，所有观察hook/订阅恢复，无连续后台扫描。

### 42.3 网络意图修订与资源门禁

只新增内部`WifiCandidateFailurePolicy`并复用原RetryBackoff，原`retryConfirmedLater`改名`scheduleRetry`以覆盖候选，移除已确认网络失败路径重复的station.stop。默认Rollback不变；普通DeviceKey WiFi与手动Hub composition启动前注入Retry，配网Hub仍Rollback。运行中拒绝改策略。同步station.start失败但已进入RetryBackoff时，start返回“异步启动已受理”，避免外层将可恢复状态误判为begin终态错误；lastError仍保留，连接且durable confirm成功前云端仍不可用。

本批6组x64 Host/ASan、Win32 lifecycle、架构检查通过。生命周期由4项原测试扩为12项，包含3次连续重试/不写Flash、旧网不被误选、首轮同步失败仍可恢复、时钟回绕、新配置代际、持久确认失败、配网回滚和策略约束。重试联网不写Flash不等于禁止必要的durable confirm写入。生命周期x64为64 B、Win32为44 B；本次策略利用布局填充，未扩大已测对象。没有新增独立MCU状态机、用户API、协议或持久格式。

SDK3.3.11普通WiFi Plain诊断产品编译/真机通过：Flash1125988 B、静态RAM58144 B；比此前诊断产品Flash增加60 B、RAM相同。业务后设备仍active，heap193980/minHeap182852/largest143348 B、loop最低3396 B、ipc0最低1284 B。它是普通WiFi测试，不证明Hub/NimBLE任务预算。UNO R4/Wio复用修改后的公共product，但本批未做其编译/硬件回归。仅对受影响WiFiProduct冻结哈希作v35审查更新，登录wire/provider/golden合同不变。

另补同SDK的`CompileEsp32EdgeHubInternal`完整构建通过（Flash1451040 B、静态RAM75244 B），覆盖修改后的Hub composition及WiFiProv分支的编译准入；未烧入Hub或据此声明其LAN/GATT硬件验证完成。证据为`i3water-hub-compile.log`。

### 42.4 恢复、证据与下一步

观察已清理；COM12保存隔离测试NVS/OTA后，恢复**本批**`i3water-before-region.bin`（0x9000起3371008 B），写后hash验证通过；未擦整Flash/分区表，COM3/COM11未动。原备份SHA-256为`a50aabd4fcf3011d901cfa7619b002414eda344f159d3bb19b0f939512c548a6`。隔离逻辑设备Key仍2、服务端最后合法profile2；恢复原板固件不意味着服务端模式自动反转。

证据均在ignored `build/lan-p3-20260910/`：`i3water-bare-online.log`、`i3water-product-online.log`、三个`i3water-*-flow-*.json`、`warm-i3water-stop-*.json`、`i3water-after-control.log`、`i3water-host-results-final.log`、`i3water-win32-results-final.log`、`i3water-architecture-final.log`、`i3water-restore.log`。此前无final后缀的初跑测试日志包含已修正的fixture初始化失败，不作通过证据。固件bin SHA-256为`ed7de697b12e41f6fb6362ac0b2adb3e8d23c2731f2b6e1664102f0a398e0d08`。最终三端按各仓库换行配置运行diff --check均通过，保留已有无关改动。密钥/密码/私有配置不入文档或发布包；未暂存/提交/推送。

执行顺序不因单次亚秒热开页扩大：下一批优先完成真实第二账户分享→撤销→durable Retire/exact ACK→到期前会话关闭→重新分享恢复，并确认owner未被误伤；没有第二真实主体不能用owner fixture替代。可独立补Android JS挂起/进程终止时native发现资源的有限生命周期。上述P3余项完成后再进入WiFi+BLE/Hub共享LAN与子设备（S4），不继续用普通冷/热循环或新增协议填充进度。全产品无缝通信、原路故障切换和多child压力仍未宣称完成。

## 43. 原生扫描有限生命周期及第二测试主体（GATT§10.142）

本批先复核第二真实账号条件；用户确认可沿现有测试号流程在手机切换，必须保留当前账号恢复方式，不伪造用户token/直接插入分享记录。账号准备不要求重新擦板/刷新Key。

独立发现边界采用最小修订：在独立`capacitor-mdns`源码仓为Android watch增加可选timeoutMs与stopOnPause，LAN显式选择10秒及退后台停止，默认其他浏览/发布保持原行为。原生期限和退后台回收不依赖JS计时；仍用原串行执行器，所有定时清退精确匹配原session，旧deadline不能关闭替换后的watch，pause不关闭publisher或未选择该选项的其他watch。不会直接编辑node_modules、不为发现新增账户协议或连接池。

JS目录继续持有10秒预算、15秒候选TTL及精确unwatch/监听器清理屏障；原生到期仅回收原生watch，不代替JS owner清理，不在resume自动重开。先证实当前缺口，再验证定时/退后台/旧回调及进程终止资源边界。期限表示开始退休，JmDNS/socket最终close仍有原生耗时；不承诺OS暂停整个进程时按墙钟执行或把正常HOME当JS挂起证据。完成后分别记录软件、原生/真机及恢复事实，不提前判P3通过。

### 43.1 最小实现及实测边界

Android复用原串行执行器，改为可调度的单线程执行器；取消的期限任务即时移出队列。期限以原生单调时钟从调用准入计时，队列中已过期的请求不能替换现有watch；定时回调按session对象精确退休，pause代际阻止旧调用在resume后迟到启动。仅选择stopOnPause的watch退后台停止，publisher及其他watch保持原有生命周期。没有新增扫描线程、App连接池或业务协议。

App只在原`LocalLanDiscovery`传入`timeoutMs: 10000, stopOnPause: true`，不增加第二个扫描器或修改页面UI。共享目录仍负责JS期限/候选/owner，原生没有terminal JS事件；暂停JS时资源已关闭不等于JS状态同步完成。iOS未实施这两个选项，不将Android结果泛化为跨平台完成。

真实手机（196f65f9）使用独立测试service type，确保与生产LAN watch隔离；基线与修订版都先确认持有组播锁，再通过CDP实际触发`Debugger.paused`，不是仅按HOME推断JS暂停：

| 场景（各一次） | 验证事实 |
| --- | --- |
| 旧版watch请求3秒期限，JS暂停约8.6秒 | 同PID仍持有组播锁；显式unwatch后释放，复现缺口 |
| 修订版3秒期限，JS持续暂停 | 约8.6秒检查时同PID组播锁已释放 |
| 修订版30秒期限，先暂停JS再HOME | 约6.1秒检查时同PID锁已释放，早于期限，验证原生pause路径 |
| 修订版30秒期限，force-stop App | 约4.4秒检查时锁已释放，随后重开App |

以上是离散检查时刻，不是精确释放耗时/p95；期限或pause发起退休，最终关闭仍受串行队列与JmDNS影响，不保证整个进程被OS暂停时仍按墙钟运行。原生独立Instrumentation另4项通过（13.365秒）：期限资源归零、旧deadline不关闭替换watch、pause不误伤其他browser/publisher、队列中过期请求不替换现有watch；测试断言同时覆盖池/锁引用及取消任务清理。

插件TS/Rollup、App local build、App/spec TypeScript、改动文件lint及14文件197项关联回归通过；设备架构检查通过。Android debug APK及测试APK构建通过。Gradle connected runner因Google UTP依赖下载TLS失败未跑通，改用同一生成测试APK的标准ADB instrumentation取得4项结果，未禁用TLS校验。没有重跑设备Host/Service PG或新MCU业务闭环，不套用§42耗时为新APK性能结果。

### 43.2 第二主体已准备，发现首次路由准入缺口

按用户授权，通过本机认证服务既有admin CRUD创建一个普通测试用户，再走真实密码登录endpoint取得实际令牌。该步骤是测试数据准备，**不是公共注册/SMS/admin HTTP权限门禁通过**；未伪造JWT、手工插入分享/租户路由，未修改认证服务代码。使用App原DataService/用户信息加载进行账号切换，不声称密码输入UI流程经过点击测试。

新账号`/auth/me`200且主体匹配；无分享时目标local-access403 `LOCAL_ACCESS_FORBIDDEN`，原账号恢复后context200/profile2、账户MQTTready、connector空闲。新号无设备时账户连接未自动启动是按需行为；显式调用正式账户start后，`/api/v2/account/connection`503 `DEPENDENCY_UNAVAILABLE`。

同一真实owner建立一条邀请201，新主体接受409 `DEVICE_V2_CROSS_SHARD_SHARE_UNSUPPORTED`。只读数据库核对：receiver tenant active但route/shard均null，目标route normal/shard1，二者间无grant。因此证据是**缺路由被误分类为跨分片**，不是已经验证两个不同shard不能互通。账户connection当前只读`resolveTenantRoute(...createIfMissing:false)`；分享准入把缺路由、非normal和实际跨分片合并到同一409。现有allocator又按`needsBrokerNow`懒分配，不能假设补一次reconcile就一定解决共享用户首次接入。

本次邀请已通过正式DELETE撤销（200/state=revoked），未产生已接受分享或Retire。原账号恢复成功，新普通测试账号保留供下批使用；恢复材料仅在ignored私有目录，文档/源码不含密码、JWT、邀请code。没有为了通过门禁创建虚假设备、SQL赋shard或改变owner Key/ACL。

### 43.3 恢复与下一批固定顺序

手机曾自动回YYSC，已按用户确认重新接i3water；最终手机172.20.10.2、原账户MQTTready/context200/profile2、connector空闲、无原生组播锁。只卸载本批生成的`tech.diandeng.capacitor.mdns.test`测试包，保留修订后的Blinker debug APK和用户数据；测试APK仍可重新安装。COM3/11/12本批未烧录/清除/重置。三个源码环境LAN仍false，debug生成资产只启用隔离门禁并保留原HTTPS域/CA约束，未扩大信任。未暂存/提交/推送。

证据：ignored `build/lan-native-lifetime-20260910/`下`baseline-1789016867919.json`、`deadline-1789017501953.json`、`pause-1789017621855.json`、`kill-1789017672670.json`、`native-instrumentation.log`、`app-regression.log`、`app-gradle-final.log`、`sharing-route.json`及`switch-owner-1789017990222.json`；同目录早期失败诊断不作通过证据。独立`capacitor-mdns`仓的源码/测试是这批必要修改，不以node_modules副本代替依赖版本交付。

1. **先修Service首次路由需求与分享准入。** 复用既有allocator的容量、资格、持久预留、幂等及迁移屏障，明确首次账户连接/合法接受分享怎样构成Broker需求；无设备用户也能接收分享，不要求先建假设备。缺/忙路由与真正跨shard分开报错，不把无路由503当永久跨分片。并发接受、取消/过期邀请、容量不足、已有不同shard、重试不产生重复grant均须软件/隔离PG验证；只给真正校验过的操作准入，不因任意输入分配昂贵资源。
2. **再真实双主体闭环。** 实际账号MQTTready、正式分享成功后，重新确认COM12角色/隔离固件及同网再刷必要app；在未到期LAN会话中正式撤权→durable Retire/outbox→设备exact ACK→提前关闭→重新分享恢复，owner不能被误伤。不用到期、Key刷新、伪造ACK或手工SQL替代。
3. 两项完成再推进Hub共享LAN/S4。若涉及实际跨shard传递或独享Broker策略，先单独冻结其边界，不为本地单shard门禁顺带增加跨分片协议。P3仍未整体通过，当前不启动100次循环或宣称全产品无缝。

## 44. 首次路由准入、并发分享及真实账户MQTT恢复（GATT§10.143）

### 44.1 完成的最小修改

- 新增小型`tenant-route-admission.js`，统一管理创建设备、账户connection、分享接受的路由准备；只注入既有allocator/route/access接口，不负责选shard或建立MQTT。已有normal route直接复用，busy route不会当缺失重建，更不会为分享隐式迁移。账户签发仍经过认证、参数、限流、租户、路由及健康检查；WebSocket未配置在分配前拒绝。
- 原allocator增加内部显式需求参数，首次合法请求不需要先造一台云设备。后台被动轮次与新请求并发时，合并到一个后续有需求的轮次，8个调用实测只提交一次分配；原持久reservation/执行锁/资格/退避继续使用。已接受且设备仍active的分享计入持久`needsBrokerNow`，不伪造owned-device数量或添加持久需求表。
- 分享按“相同预校验→事务外准入→原SERIALIZABLE事务重新校验并授权”执行。邀请证明、账号/tenant、owner不能自领、撤销/到期、目标active和现有route都先检查；最终锁内再次检查，不把分配完成当授权。取消/过期、失败重试和accepted replay不重复grant；锁等待后重新检查期限。移除抛错后必回滚的过期状态写入。
- 缺/忙route返回`DEVICE_V2_ROUTE_NOT_READY`，容量准备返回`BROKER_ALLOCATION_PENDING`503，真正不同normal shard仍`DEVICE_V2_CROSS_SHARD_SHARE_UNSUPPORTED`409。原同shard限制没有被放宽。静态fallback沿用原route resolver，但首次upsert增加createOnly并保留原PG advisory/serializable锁，另一请求不能覆盖首个已提交route；显式迁移/upsert默认语义不变，无schema迁移。

### 44.2 软件与隔离PG验证

10项Node runner结果通过（6项独立admission + allocator policy/runtime、sharing HTTP、credential-security各一文件），覆盖容量失败、busy route、访问退避、static/active模式及显式需求合并。13项隔离PostgreSQL runner结果通过：9个新分享/首次route子场景、其父测试及access-grants、sharing-e2e、gateway-wiring各一文件；不是13次MCU测试。

PG新测试实际使用原active allocator、持久reservation/execution lease和分享事务，健康shard选择/远端capacity/cache是测试端口。另两个HTTP生产接线用例去掉“预先SQL写receiver route”的fixture，分别从无路由账户connection和无设备receiver接受分享开始，在原static模式完成。包括分配失败保留邀请、重传只一个version、分配期间取消/过期、busy与真跨shard分类、并发接受和两种候选首次route不互相覆盖。Gateway TypeScript构建、设备架构检查通过；本批不重跑App197项、设备Host或MCU控制。

早期PG运行容器继承production环境而缺测试KEK，2项接线失败；修正仅测试容器为test/development后重跑通过，未放宽生产KEK检查。runtime首轮新用例复用前序迁移fixture的reservation而失败，改为独立数据库fixture后通过，不作生产容量bug证据。新测试加入Gateway `test:broker-allocation`；PG需要真实测试数据库，否则明确skip，不把skip记通过。

### 44.3 本机部署及手机真实结果

仅更新现有Gateway为`aily-iot-dev-gateway:lan-route-20260910`，保留业务PG/Redis、CA/HTTPS、原账户和设备Key。新账号正式connection由503变200，真实allocator赋予normal route0。此时App仍报`MQTT channel closed`；核对运行文件发现shard0仍是旧`latest`，shard1是`lan-p3-20260910`，BBP能力不一致。

使用SQLite在线backup先保存shard0的`broker-v2.db`487424 B，SHA-256 `a2a8114a346e754e964b9bb59a638c6fbe9bff785a62804cc8763bc3557b734b`；再只对齐shard0到shard1已用的联调镜像，**保留SHARD_ID=0、原配置/volume**，不是把两个逻辑shard合并。shard2未改，不能据此称整个可分配池都已对齐。初次对齐脚本发现Gateway专用compose没有broker0，准入检查停止且未修改容器；改用broker0实际compose再执行成功。

对齐后同一手机/同一新主体正式MQTT及账户BBP进入ready、ME200；未分享目标context403 `LOCAL_ACCESS_FORBIDDEN`，connector空闲。owner恢复ME200、MQTTready、context200/profile2；无App源码/UI/APK或MCU改动。它证明本轮新账号登录/握手恢复，不等于目标设备控制或所有MQTT稳定性门禁完成。

再次正式创建一条邀请201、接受409；PG证实receiver正常shard0、目标正常shard1、无grant。这次是真正跨分片，而非§43的null route误报。邀请已正式DELETE撤销（200/revoked），未产生receiver LocalGrant或Retire；不能声称已跑过设备撤权ACK。新账号及合法route0保留，未SQL迁移到shard1、未复制测试身份/造假设备。

### 44.4 下一批顺序与不能再隐藏的前置条件

“同一个账号可以接收指定设备分享”不能长期依赖allocator偶然把两个账号分到同一shard。下一批先按Service LocalAccess§34.3梳理**账户身份/设备ACL与物理shard路由分离**：手机仍一个账户MQTT，目标设备仍自己的MQTT，跨shard由服务内部有界转发承接，不增手机第二MQTT、不搬设备或重写DeviceKey。这里只冻结方向/门禁，不提前删除同shard安全限制。

先查清现有Route/Delivery、command ledger、subscription/ACK、撤权invalidation/local-control outbox的跨shard接线；补源/目标权限、route version与单次执行/replay、目标迁移/离线、双边撤权的合同和隔离PG/Broker测试，再用当前真实route0/route1账号做分享/撤销。有效分配池镜像也应先同版验证，不能把旧shard故障记成App或设备问题。上述前置满足后，才恢复COM12隔离产品做LAN未到期会话→durable Retire/exact ACK→提前关闭→重新分享/owner隔离，最后Hub/S4。

证据在ignored `build/lan-route-admission-20260910/`的`postgres.log`、`deploy.log`、`align-broker.log`、受限分享检查点及SQLite备份；真实账号ready/恢复分别在上一证据目录的`switch-receiver-1789020062696.json`和`switch-owner-1789020151409.json`。令牌/密码/邀请code/完整compose仅私有文件，不入源码/文档。手机保持i3water/原账号，板未烧录/擦除/重置；源码LAN仍关闭，未暂存/提交/推送，P3未整体通过。

## 45. 跨分片R0与协议字节口径（GATT§10.144）

后续权威步骤落在 `D:/codes/aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md`：R0权限/placement、R1目标runtime与binary exchange、R2订阅回流、R3双端撤权、R4原双账号真机P3。不是另开BLE网关/History/Bulk分支；App保持一条账户MQTT，LAN/BLE仍独立通道，设备API/协议不改。

本批R0已完成软件基线：单一ACL查询分别校验source/target normal route、活动tenant和源routeVersion；内部只读查询回传两端权威routeVersion，不是执行票据。修复缺route被Number(null)误当shard0、原本地路径未校验source placement及非canonical shard header。现有local dispatch/share仍禁止真实cross-shard；无route的BLE/LAN metadata授权没有被云placement要求替代。

8项本地runner、Broker client4项、隔离PG20项与Broker/Gateway TS构建通过；PG含真实SQL关系与HTTP认证，不是两Broker转发或MCU exact ACK门禁。首次测试错将幂等same-route upsert当作版本递增，已改为实际transitioning→normal再通过；没有为测试改变原幂等契约。证据 `build/lan-cross-shard-20260910/postgres.log`。

生产codec测得8路语义灯JSON222 B→设备BBP33 B/App Delivery59 B；单power Command13 B/App Route57 B。长文本与路由/安全/链路开销另列，详见Service `DEVICE_V2_WIRE_SIZE_AND_ROUTING_REVIEW.md`。旧外层Python基准残留9 B BLE头纠正为4 B，其404/174/94 B仍是历史组件样式模型，不冒充当前真机消息。

本批未部署、更换账户/Key或操作开发板，没有新的App/MCU测量，源码LAN开关保持关闭。未暂存/提交/推送。下一批从R1开始，分享/结果回流/撤权完整通过前，不解除cross-shard限制，也不把权限查询判为P3通过。

## 46. R1a/b本地复用与字节传输底座（GATT§10.145）

本批只改Broker源码和相关测试/文档。Manifest/State/命令结果已解耦为无Aedes/数据库依赖的target模块，并接回原lane，删除旧内联函数；原command ledger与wire不变。账户claims保留JWT认证的shard/routeVersion和每连接sessionGeneration，补断开前不启动排队命令、执行后授权/目标/gate替换拒绝成功ACK。

共用bounded HTTP字节传输替换原Gateway JSON client的手写socket/timer逻辑；支持原JSON codec和raw Buffer、容量/绝对期限/取消/close-open代际，不复制转发协议或新增App MQTT。192项定向测试及原Broker Device V2、Telemetry、限流、TS与wire基准通过；PG、App与MCU本批未测。

R1整体仍未完成：原源Broker的peer解析只查本地表；下一批R1c先接权威16 B peer→唯一逻辑设备/路由解析，再有界signed binary envelope、目的端fresh授权与远端session生命周期，两隔离Broker验证后续R2/R3。share inventory/修改/退出中隐藏的same-shard SQL也记入R3，不能只解除accept检查。权威计划见Service跨分片方案§7；完成后仍回到原真实双主体LAN P3，再Hub/S4。

证据`build/lan-cross-shard-r1-20260910/`；本批无部署、账户操作、烧录或重置，网络沿用户最后确认i3water但未新做现场探测。App/MCU源码及LAN开关未变，未暂存/提交/推送，不宣称真机或跨Broker已经完成。

## 47. R1c权威peer/目的端fence端口与内部重放修正（GATT§10.146）

Service已完成canonical peer共享、源端权威两个主键候选查询、目的端logical ID+peer/两routeVersion/tenant/ACL的fresh复核，以及小型共用HTTP路由和Broker client。候选有歧义或被另一个逻辑设备替换时拒绝，不用MAC也不复制远端设备表。原内部HMAC两处replay边界测试先失败后修正，容量满不能驱逐有效nonce，保留期覆盖未来timestamp有效窗。

198项Broker定向、identity2项、HMAC4项、隔离PG及真实Gateway HTTP27项、原协议/签名/Telemetry/限流及TS/wire门禁通过。当前只是查询端口，源MQTT远端选择、signed binary执行转交、active source session和目的端remote gate未接通，R1/P3仍未通过。下一批合同见Service跨分片方案§9，需一起处理3s查询期限与3750ms设备命令等待窗口的差异；不新增终端协议或第二App MQTT。

证据`build/lan-cross-shard-r1c-20260910/`；无App/MCU源码、部署、APK、账号、Key或硬件变更，未暂存/提交/推送。R1c请求/响应→R2通知→R3双端撤权→原两主体真实LAN P3→Hub/S4顺序不变。

## 48. R1c请求/响应软件闭环，转入R2（GATT§10.147）

服务端实际signed binary relay、active source proof和remote target context已接入受控lane，Manifest/Accept/State请求及Command复用原target executor与唯一持久账本。源库不复制目标设备；App仍单账户MQTT，设备BBP/CBOR与LAN/BLE wire不变。共同HTTP/HMAC已归一，取消不会提前释放尚在执行的命令槽或迁移锁。

隔离两Broker/独立SQLite/生产HTTP入口及签名/分页/生命周期/容量共21项通过；真实PG/HTTP27项和Broker定向199项、Device V2/Telemetry/限流/TS/wire回归通过。补正命令错误缓存遮住晚ACK结果、轻微钟差误拒绝。该闭环使用fixture ACL、设备ACK及初始认证，**不是手机/MCU四端联调**；实际SQL授权另有PG门禁。证据`build/lan-cross-shard-relay-20260910/`。

下一批以Service跨分片方案§10.5为唯一顺序：R2通知sink与State/Presence→有界interest及Telemetry/ConnectionDemand→R3双端撤权/received inventory/Retire exact ACK→原真实LAN P3→Hub/S4。请求开关默认false，分享限制未解除，LAN开关未变。本批无App/MCU源码、部署/账户/硬件操作，未暂存/提交/推送。

## 49. R2a本地通知底座完成，下一批远端Presence（GATT§10.148）

按Service跨分片方案§11推进，非扩散新的MCU/LAN协议。Broker已抽取并实际复用target通知producer与可注入sink，移除旧发布器和无界Presence链；有界顺序准备、出队分配sequence、分页原子准入、旧ACL/会话隔离与慢消费者处理已补测。新增21项，Broker定向220项及R1两Broker21项、原协议/Telemetry/限流/签名/TS/wire回归通过；本批无PG重跑/部署/硬件操作。

修正了“Manifest协商＝页面长期订阅”的隐含前提：现有Presence只有会话级Subscribe；State/Event应先接明确owner，App尚不接受unsolicited Manifest。下一批先R2b远端Presence watch/lease/反向投递，再R2c其余通知与原租约；R3撤权/分享完整后才恢复原真实LAN P3，之后Hub/S4。不是跨shard推送或真机门禁已通过，分享仍关闭。App/MCU源码和开关未变，未暂存/提交/推送；证据`build/lan-cross-shard-notifications-20260910/`。

## 50. R2b远端Presence最小软件闭环与下一步边界（GATT§10.149）

Service跨分片方案§12为最新状态：会话级Presence watch及30 s租约/15 s续期、exact取消、反向原字节通知已接通。复用原target context与R2a队列，不增加App MQTT、设备字段或LAN/BLE wire；首次响应发布后才交接通知，序号防止旧快照/排队结果回滚在线状态。

Windows/Linux各40项relay/生命周期/隔离两Broker、Broker定向220项及原协议/Telemetry/限流/签名/TS/wire回归通过。未重跑PG，fixture不能替代真实账号/MCU；证据`build/lan-cross-shard-presence-20260910/`。当前watch续期失败会使原账户重连，仍影响其他目标，故下批R2c-1先做App单目标失效恢复与实际owner，然后其余R2→R3→原双主体LAN P3→Hub/S4。不是完整R2或无缝通信已完成，跨shard分享保持关闭。

本批没有App/MCU源码或部署/硬件变更，未暂存/提交/推送。现场网络已按用户最新通知改为“带翅膀的蜗牛”，后续真机不得沿用旧i3water；密码不写入方案或发布文件，测试前核对手机、电脑及设备同网。本批隔离软件结果不依赖现场WiFi，不代表新网络已验证。

## 51. R2c-1a目标级Presence恢复，下一批实际owner（GATT§10.150）

Service/App已替换§50的整账户重连策略：协商后的失效通知绑定单目标订阅ID，App保持一条账户MQTT、标记unknown并有限重新订阅，其他目标不受牵连；显式新页面等待可仅重试耗尽目标。不改MCU、LAN/BLE wire、设备Key或存储。权威结果及边界见Service跨分片方案§13；这不是零延迟无缝切换，目标故障探测仍受原续期与HTTP预算限制。

Windows/Linux各44项relay、Broker220项、App headless34 suite/425项及类型/构建/原wire回归通过；PG、App完整浏览器/APK、真实账号/MCU本批未测，12个旧浏览器suite未算通过。下一批R2c-1b明确State/Event实际页面/业务owner，再其余R2→R3→原真实LAN P3→Hub/S4，分享仍关闭。未部署/刷板/改账号、暂存/提交/推送；现场WiFi沿用户最新“带翅膀的蜗牛”，未新做现场探测。


## 52. R2c-1b页面owner基础完成，下一步远端State/Event（GATT§10.151）

Service跨分片方案§14为最新执行入口。App持久Cloud页面已显式获取/续期/释放StateInterest；一次性读及首页Presence不建立这个owner。普通WiFi观察不唤醒子设备，Cloud子设备继续单独持有原ConnectionDemand。单owner页面scope已改为多owner集合，Cloud观察结束不关闭已就绪LAN。

Broker本地State/Event增加连续观察代际与有限租约筛选，旧队列不能在释放后重新订阅时复活；跨分片控制已走原R1签名通道。App旧snapshot跨观察间隙不能恢复fresh，复用原一次StateConflict重读。MCU/API/LAN/BLE业务wire、源码及发布开关未改，没有另一条账户MQTT。

Windows/Linux各47项relay、Broker225项、App34 suite/436项及类型/构建/原协议回归通过；证据build/lan-cross-shard-state-interest-20260910。未做完整浏览器/APK、PG真实分享或MCU联调，未部署/刷板/改账号、暂存/提交/推送。**远端主动State/Event及Manifest变化仍待R2c-2**，之后原租约跨shard→R3→原真实LAN P3→Hub/S4；分享继续关闭，不把订阅控制受理当无缝通信已完成。

## 53. R2c-2a模型变化与恢复基础，仍未进入真机P3（GATT§10.152）

按Service跨分片方案§15先修前置缺口：设备换模型后旧页面仅被服务器跳过而没有通知。Broker完整Manifest校验产生统一事件，普通设备和网关虚拟子设备共享通知入口，删除重复fanout条件树；App/Broker新增ManifestChanged hint并复用原SHA-256/Accept ACK/State交换。

App恢复附着原StateInterest观察组：最多3次，立即及失败后1/3 s，实际并发≤4；续期/成功不补预算，退出不重新调度，旧页/hash/ACK和观察周期隔离。命令不自动重演，不新增账户MQTT、MCU/LAN/BLE字段或设备API。通知body73～77 B是低频模型变化开销，不是每条State消息的新开销。

新增App13项/Broker15项软件测试；完整回归见ignored build/lan-cross-shard-manifest-change-20260910。未做PG真实分享、浏览器/APK或MCU联调，源码LAN/分享开关不变，无部署、账号/Key/Flash操作或暂存/提交/推送。

尚不能宣称可靠被动推送：初次hint的ACL查询失败可能让App暂不知模型变化；下一批**R2c-2b持续source proof＋观察丢失/有限重获＋远端State/Event/Manifest回流**需一起闭环，再原租约跨shard→R3→原真实双主体LAN P3→Hub/S4。不跑100轮BLE切换、不扩设备协议；真机仍先确认用户最新“带翅膀的蜗牛”同网。

## 54. R2c-2b1源端持续观察证明与只读投影（GATT§10.153）

Service跨分片方案§16已完成原StateInterest源端owner接线、独立`state-current`签名复核及目的端只读context投影。提前release、并发重复、旧失败回包与新续期隔离均复用原管理器/replay；不增加设备协议、存储槽或App MQTT，不复制设备模型到源Broker。源证明通过不等于设备在线或远端数据已推送。

Windows/Linux各67项签名relay/双Broker、Broker240项、App35 suite/449项及构建/原回归通过，证据`build/lan-cross-shard-state-proof-20260910/`。本批App/MCU生产代码未改，无PG真分享、APK/MCU实测、部署/账号/刷板或暂存/提交/推送。

唯一下一步R2c-2b2：首次同步与通知交接、远端State/Event/Manifest回流、观察丢失及有限恢复；然后原业务租约跨shard→R3→原真实双主体LAN P3→Hub/S4。不以软件证明代替真机通过，分享与源码LAN开关不变；后续现场仍按“带翅膀的蜗牛”核对同网。

## 55. R2c-2b2a共享观察及失效恢复（GATT§10.154）

App同设备多caller已收敛为一个网络StateInterest；本地引用、原owner引擎、三次/四任务恢复预算分工明确，退出最后caller才释放。Broker/App协商bit23后复用原Status表达exact观察stale/denied，本地模型通知ACL异常与State准备缺口不再静默留下旧fresh。恢复仍完整校验模型/State，不重演Command，不关闭已就绪LAN或其他target。

设备MCU/API、LAN/BLE业务wire、设备存储与源码LAN开关均未改。新增App12项/Broker本地9项/源投影1项；最终App461项、Broker249项、Windows/Linux各68项relay及App↔Broker实际代码互通2项通过，证据`build/lan-cross-shard-state-recovery-20260911/`，完整合同见Service跨分片§17。无部署/账号/Key/硬件操作或暂存/提交/推送，未作PG真分享或真机测试。

下一步R2c-2b2b远端首次响应与通知交接、State/Event/Manifest回流及远端context/序号故障，之后原业务租约→R3→原真实LAN P3→Hub/S4。App恢复基础不代表远端可靠观察已完成；分享仍关闭，后续现场仍先核对用户最新“带翅膀的蜗牛”同网。

## 56. R2c-2b2b1同步响应发布栅栏（GATT§10.155）

复用原R1槽、签名envelope、目标replay checkpoint和App失效恢复，完成账户MQTT发布成功后的内部确认。只有连续兴趣、当前Manifest ACK及State末页已发布，才满足目的只读ready；不按“已生成HTTP响应”提前准入。保持5.5 s总预算/原操作配额，非协作取消持有实际槽；稳态兴趣续期不新增首次确认流量。

本批新14项源/目标、2项两独立Broker HTTP及1项16并发签名池门禁；完整回归App461/Broker249/Windows与Linux各85项relay/App↔Broker2项通过。证据ignored `build/lan-cross-shard-publication-20260911/`；完整约束见Service跨分片§18。设备API/MCU RAM/Flash、LAN/BLE/MQTT设备wire、App源码未改；未部署、刷板、改账号/Key/WiFi或暂存/提交/推送。

下一步R2c-2b2b2窗口有界数据、远端State/Event/Manifest与静默丢失/序号缺口，再原租约→R3→真实LAN P3→Hub/S4。窗口数据尚未补齐，不能将ready或发布确认视为无缝通信完成；分享及源码LAN继续关闭。

## 57. R2c-2b2b2有界窗口及远端通知（GATT§10.156）

目的原Target新增可选stream，accepted之前打开窗口；State只保留dirty并由目的唯一存储补一致快照，Event有界保存/顺序交付，超限及不能证明连续性时明确stale。原lane生产者同时服务本地和远端，原账户sink发布后才确认通知；未新增App MQTT、MCU协议或Manifest数据库副本。

源端校验exact观察/发布baseline/sequence；窗口期限5.5 s不可因重复pause延长，静默context重启由App原10 s续期发现并有限恢复。真实App软件测试确认模型刷新、缺口恢复、原定时器和同账户另一target隔离；不是每次切路需要等待10 s，也不是立即撤权证明。R3设备Retire/exact ACK仍未完成。

Windows/Linux各105项relay、App461项、Broker249项、实际App远端互通6项/本地2项及原构建/协议回归通过；ignored证据`build/lan-cross-shard-state-stream-20260911/`，完整资源/权限/异常边界见Service跨分片§19。MCU/API/设备wire、App源码及现场未改，未部署/刷板/改Key/WiFi/暂存/提交/推送。

下一步R2c-3a原ConnectionDemand→DirectPriority/DirectReady、Telemetry跨shard；R2总验收仍含多target拥塞/非协作取消/迁移，之后R3→原真实LAN P3→Hub/S4。账户底层故障仍可关闭共享socket，分享/源码LAN继续关闭，不宣称无缝通信发布。

## 58. R2c-3a远端连接需求与唯一目的调度（GATT§10.157）

ConnectionDemand已走原账户MQTT/signed relay，到目的Broker与本地需求共用唯一活动汇总，原Hub执行驱动不变。源只保存有界取消/回执关联，不唤醒源端Hub。StateInterest仍独立表示观察，accepted不代表BLE child Ready或State fresh；设备API/MCU资源/协议不扩展。

修正一个exchange结束误伤整个target的耦合，同时保持真正ACL拒绝回收；release可抢先于源lookup/目的授权，旧回执不可复活owner。本地与remote源记录合计16/session，目的本地/远端共同计入32/target及256全局活动配额。cancel丢失沿原30 s需求TTL收尾，不冒充立即撤权/物理停止ACK。

新增Broker软件12项、双独立Broker真实签名HTTP子项3项、实际App核心内存互通3项；后者验证原10 s续期发现目的context丢失，旧owner关闭、不自动新acquire。完整合同和最终门禁见[Service跨分片§20](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md)，证据ignored build/lan-cross-shard-demand-20260911。

此批原“下一步远端Direct”已由§59纠正。分享/源码LAN关闭；无App生产源码、MCU、部署、账号/Key/WiFi或暂存/提交/推送变更。

## 59. R2c-3b范围纠正及Telemetry控制收口（GATT§10.158）

确认DirectPriority/DirectReady的owner-only DirectAdmin要求与按tenant placement共同决定：合法两端处于同一shard，普通云端operator分享不能提升为Direct射频权限。保留原本地Hub Yielded/Ready，在远端Target分配/鉴权/Hub工作前明确拒绝；不创建不可达的候选成功路径。它不改变LAN/BLE LocalGrant合同。

原本地Telemetry控制已接抽出的中性target入口，移除旧内嵌解码/回包辅助实现。旧Open/Renew缓存现在验证exact活跃消费者、epoch/deadline及当前Manifest gate；到期、Close/重开、模型变化不能重放成功或自动重新采样。仍使用原唯一采样汇总，未增加MCU wire/API/存储或App连接。

完整合同、测试及下一步见[Service跨分片§21](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md)：Broker262、Windows/Linux relay各121、隔离PG31、App461/类型检查和既有11项App互通通过，证据ignored `build/lan-cross-shard-direct-scope-20260911/`。下一步R2c-3c-2实际远端消费者/Status/Data接线；控制模块抽取不算跨分片Telemetry完成，之后仍是R2组合门禁→R3→真实LAN P3→Hub/S4。

MCU/App生产源码、LAN/BLE wire/源码开关和现场不变，无部署、手机/刷板/Key/WiFi或暂存/提交/推送。分享仍关闭，本批没有真机或无缝通信完成声明。

## 60. R2c-3c-2a消费者身份与旧数据生命周期（GATT§10.159）

Telemetry改为注入实际session/Target端口对象，不依赖MQTT clientId反查。原本地Broker注册/claims、目标控制、消费者清理和通知sink已接同一实现，移除旧全局App回调和旧入口；目的采样仍只有原汇总，单设备32/全局1024配额共用。没有第二MQTT、远端假client或MCU API/资源/协议变化。

租约裁剪到认证session期限，过短拒绝；排队样本绑定exact消费者/epoch，Close/expiry/重开后在原发布前跳过。旧终止状态不能清掉新epoch样本，同ID新登录不能接收旧claims回调；独立适配器同步入队异常不阻塞其他消费者。已发网络字节不可撤回，中性端口fixture不等于远端Telemetry已接通。

Broker278、Linux Telemetry专项23、双平台relay各121、App461/类型及原11项互通和构建回归通过；完整合同与证据见[Service跨分片§22](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md)，ignored build/lan-telemetry-consumer-20260911。下一步R2c-3c-2b远端签名租约/Status/Data发布顺序及原生命周期，之后R2组合→R3→真实LAN P3→Hub/S4。无MCU/App生产源码、部署、账号、网络、Key或暂存/提交/推送操作。

## 61. Telemetry发布交接前置，不提前开放远端（GATT§10.160）

原消费者附5 s单调交接窗口、单条最新样本及exact响应发布确认；Close/超时/gate替换、旧终止通知/runtime换代与权威模型变更不能放行旧数据。源relay选择前统一校验Telemetry能力/方向。完整实现、测试口径及唯一下一步见[Service跨分片§23](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#23-r2c-3c-2b-1发布交接前置与源准入gatt10160)。

本地生产及软件门禁通过，远端2b-2端口/签名Status/Data/持续证明和两Broker闭环仍未完成；继续R2→R3→真实LAN P3→Hub/S4，不扩MCU协议/API或新业务。无本批设备/App生产源码、部署、手机/网络/Key或暂存/提交/推送操作，分享/源码LAN仍关闭。

## 62. 原Telemetry跨分片软件闭环与R2总验收入口（GATT§10.161）

原消费者/设备采样汇总服务本地及远端Target；Status/Data经签名内部回流和原账户publisher交付，使用原5 s交接窗口与exact stream/Open证明，不增加App MQTT、MCU字段或模型副本。内部绝对租约deadline不发给App，重复响应不续租；Close/旧epoch/错stream证明、慢发布与早到终止状态已有回归。

正式Broker428、Windows/Linux relay各138、Linux Telemetry52、App461/类型检查和新增原Session Telemetry互通2项、原11项互通通过；完整验证口径/资源边界见[Service跨分片§24](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#24-r2c-3c-2b-2原telemetry跨分片小闭环gatt10161)。这不是APK/无线真机或新部署；无设备/App生产源码、Key/Flash或WiFi操作，用户最新现场SSID为mostfun2026。

下一步只做R2多target混合池容量/非协作依赖、丢cancel/响应/确认与Broker重启/有限租约回收、原App恢复；再R3双端主动失效/share/Retire exact ACK，最后真实LAN P3→Hub/S4。数据TTL和下次fresh拒绝不能代替立即撤权，无缝通信仍待真机验收，分享/源码LAN不开放。

## 63. R2d-1实际槽、HTTP断开和有限恢复（GATT§10.162）

服务端修复取消导致实际槽提前释放、源Telemetry关联无统一总量/旧placement占槽，以及HTTP socket reset迟到error导致worker退出。原有界HTTP/消费者/账户MQTT继续复用，未增加设备wire、Arduino API或App连接。新增混合池、1024源关联、丢cancel、发布确认缺失、runtime重建及实际App恢复测试；完整证据/数字见[Service跨分片§25](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#25-r2d-1实际资源所有权http断开与第一组组合门禁gatt10162)。

实际App通过原10 s续期发现失效后停流，显式refresh/重开可恢复；这不是无感自动恢复或APK/无线门禁。两worker内runtime重建不是kill进程/MCU断电；下一步R2d-2真实多target通知生产者与账户sink拥塞/隔离、进程故障和退出矩阵，之后R3→真实LAN P3→Hub/S4。分享/源码LAN保持关闭，现场SSID继续mostfun2026；无本批固件/部署/真实账号/Key/网络操作或提交/推送。

## 64. R2软件基线收口，下一步双端主动失效（GATT§10.163）

真实业务生产者/账户sink验证多target、多个账户的隔离和共享上限；Event有序或明确失效，Telemetry保留最新值。一个target准备暂停可隔离，同账户publisher阻塞不能绕开另一target抢发，另一健康账户可继续。已补真正Broker进程退出/重开SQLite、旧session/epoch失效、Open/published已应用但HTTP返回丢失及原Action结果不重复执行的门禁；软件退出矩阵/数字见[Service跨分片§26](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#26-r2d-2多目标混合生产者实际进程故障与软件退出矩阵gatt10163)。

进入R3a：原失效接口目前只覆盖本地lane，需扩到relay源/目的及Presence的原所有者；随后可靠双端投递与持久LocalGrant Retire exact ACK，统一received/share/leave，才恢复原双主体真实LAN P3。TTL和进程空内存表不是立即撤权或设备停止证明。R2不继续滚动扩协议/100轮测试，发布规模峰值/无线性能另行验收。分享/源码LAN保持关闭，无本批MCU/App生产源码、部署/真实账号/Key/Flash/网络/暂存/提交/推送操作，现场SSID仍mostfun2026。

## 65. R3a收口：撤权生命周期不再遗漏远端（GATT§10.164）

原Broker失效endpoint已接source/target/Presence，再清原lane/无效待发；按account＋logicalDevice＋grant版本隔离，旧cancel不删新授权，在途实际工作未结束不释放原槽。单独目的失效清远端Telemetry/活动需求，保留本地消费者；源失效即停止旧证明/排队数据，best-effort cancel丢失不能冒称目的已完成。Command仍唯一账本，已执行动作不能撤回。

Broker470、Linux relay179及App461/类型、原互通15项通过，非PG真实share事务、APK/手机/MCU测试。App收到State不等于目的确认publication；首次交接时模型变化允许明确失效重同步，稳态模型刷新仍保留原观察。完整实现与下一步见[跨分片§27](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#27-r3a原撤权入口双端生命周期和版本隔离gatt10164)。

下一步唯一入口R3b：事务内Broker双端投递意图、当前placement独立确认/失败和重启恢复；LocalGrant继续原退休表/worker的exact设备ACK，不混用完成条件。随后R3c统一received/share/leave再原真实LAN P3→Hub/S4；不是继续扩R2或100轮。未改设备API/wire、App生产源码、账号/Key/Flash/网络，未部署/暂存/提交/推送；开关仍关，现场SSID继续mostfun2026。

## 66. R3b-1可靠投递软件基线及下一步组合验证（GATT§10.165）

原share事务现在持久记录最新subject/grant版本和source/target placement，handler只读取`realtimeRefreshPending`并唤醒原有界worker。单端失败独立重试，同shard合并实际HTTP，旧claim/route/version不能确认新任务；旧单次单shard路径移除，不改设备wire/API或另增MQTT。原LocalGrant退休同事务存在，Broker确认不会替代exact设备ACK。

隔离PG专项runner60项、完整Gateway device-v2清单、Broker470/Linux relay179和App461/类型/15互通通过；完整范围见[Service跨分片§28](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#28-r3b-1事务持久投递双端确认与设备退休解耦gatt10165)。当前PG使用HTTP/设备协议fixture，不是实际两Broker与PG/MCU一次闭环。同版invalidate不能证明旧placement drain，迁移工作结算仍沿原屏障，永久退役端收尾待验证。

下一步R3b-2真实PG share→两个实际Broker失效→原设备Retire协议ACK组合故障门禁，复核迁移/主体退出；再R3c统一received/accept/update/revoke/leave，最后受控恢复真实双账号LAN P3→Hub/S4。未进行部署/无线测试/账号或Key/Flash变更，开关仍关闭，SSID仍mostfun2026；不把完成回归解释为无缝已实现，不跑100轮。

## 67. LAN完成度与普通设备基础收口（GATT§10.166）

R3本批首次将真实PG、两个实际Broker及原Retire协议ACK夹具串联，7项/相关85项回归通过；不是MCU无线ACK或APK测试。后续只剩R3组合/主体收尾、R3c分享一致性两类，再恢复原真实LAN P3。细节见[Service跨分片§29](../../aily-blinker-services/docs/DEVICE_V2_CROSS_SHARD_ROUTING_IMPLEMENTATION_PLAN.md#29-r3b首组真实pg两broker退休ack串联与基础主线收敛)。

WebSocket核心、安全会话/默认明文可选加密、发现和页面/账户owner已有实现，不代表全部产品可用：普通ESP32 WiFi是内部extension，WiFi+BLE/Hub尚无相同LAN产品接线，R4/Wio没有对应正式listen/discovery端口，App环境仍关闭。R4后按[基础收口路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)优先接入/绑定解除/授时，再完成LAN组合与安装/资源/真机矩阵；不直接转Hub/S4、100轮或额外网关协议。owner解绑与普通设备授时缺口不再隐含算作已完成。无本批部署/板/手机/账号/网络操作。

## 68. R4 双账号最小物理闭环通过

2026-09-12。真实 Android＋COM12 XIAO ESP32S3、两账号/Broker 的两轮 operator 分享、LAN Hello/Manifest/State/Property/Action、撤销/设备精确退休 ACK/原会话关闭、403与重分享恢复通过。账户 MQTT 在撤权中不重连，owner原权限保留；正常页面连接所有者关闭后，实际原生回收29 ms。完整数据、环境修正、制品和边界见[本轮报告](DEVICE_V2_LAN_R4_HARDWARE_REPORT.md)，不重复复制协议。

这次已经是物理 MCU ACK，不是旧§67的协议夹具；但第二物理设备并行隔离尚未补测，旧 grant/record 无线重放也没有本轮证据。R4尾项与B1普通WiFi标准示例见证合并执行，然后B1/B2→B3→B4。当前仍是ESP32 WiFi内部LAN候选，不能据此宣称其他组合/MCU已产品化。源码LAN开关保持关，已安装debug制品单独开启；跨片开关收尾恢复关，通知原配置保留。无暂存/提交/推送。

## 69. R4第二设备隔离收口，回到普通设备

2026-09-12同日后续。COM12撤权至物理Retire ACK1,950 ms，原LAN关闭；COM3普通WiFi示例配置副本持续13次Property ACK/真实State，同账户MQTT不重连，另一设备grant不变；重启恢复亦通过。§68的第二设备待测项已关闭，旧wire无线攻击仍非本批范围。完整数据/测试配置限制集中在[报告§6](DEVICE_V2_LAN_R4_HARDWARE_REPORT.md#6-第二物理设备隔离与b1普通wifi初步见证)，不再复制协议或重跑长循环。

联调修正仅开启Gateway及两个Broker原跨片开关；原只开Gateway导致新设备Manifest路由被拒，非设备协议缺陷。测试后全部恢复原开关/owner，两个测试share已撤销，设备保留，无生产代码/源码LAN默认值变化。下一步B1默认示例/安装矩阵和B2 owner解绑；B3/B4按原顺序。

## 70. B4-1 同 caller/self scope 接替（2026-09-13）

### 70.1 冻结边界与执行顺序

授权槽保存有限权限/截止时间，不是 WebSocket 连接槽。关闭 TCP 不能清掉授权防重放事实；同授权重开不能续满五分钟。
当前 Service 可以给同一 caller 的新 scope 签出另一份授权，设备单槽正确拒绝，却使正常权限切换等待旧期限。
不能在设备按“最近一次 Grant”覆盖旧槽，也不能用全局 revision 变化踢掉其它 caller/目标。

本包复用现有 POST、retirement outbox、唯一 control worker 与精确 Retire ACK，不增加 wire、表、队列、授权槽或 Sketch API：

1. 原 account + tenant + recipient + caller + self target 范围内，新请求 scope 与仍可能占槽的旧授权不同时，先重新检查当前 ACL/实例/Key/模式/同步事实；旧 HTTP 幂等请求不成为接替请求。
2. 同一事务精确标旧授权 revoked 并写原 outbox；**提交后**返回 `409 LOCAL_ACCESS_HANDOVER_REQUIRED`，不解密 Key、不签 Grant。不能抛异常回滚退休后却告诉 App 正在接替。
3. 原 worker 送 Retire。实际 ACK 前，签发和只读 context 仍受 `LOCAL_ACCESS_SYNC_REQUIRED` 围栏；不能将入队、发送成功或 Hello 当已退休。
4. App 仅对上述明确的签发前结果，等待原生链路关闭，有限查询 context，ACK 就绪后对同候选建立一次新握手，生成新 request/challenge；原总 30 秒连接预算不延长。
5. 不重试已经得到 Grant 后的握手/BBP/业务失败；未知 Action 不重发，取消/换账户/网络/清理失败立即结束本地流程。已提交退休继续由原 worker 完成，取消页面不恢复旧 Grant。云端原账户 MQTT 不受这个准备过程控制。

本包仅自动接替**同 caller 的 self scope**。新 caller（杀进程、另一手机）、换账号、网关跨目标容量竞争不推断为可抢占：后续需显式让出/接替合同和身份边界；不得退休另一账号或同账号其他 caller。
同 scope 仍复用原 authorization/deadline；自然到期的同 scope 重新申请及其 15 秒 tombstone 边界独立保留。
旧授权纳入退休的保守范围为 `server expiresAt + lifetimeMillis + 2 × challengeWindow`：设备可能直到服务端最后一次 fresh Grant 才**首次**建槽，此时 MCU 仍从首次接受起计完整租期，再有 tombstone；不能只加两个挑战窗口或仅看服务器未到期。
App 的 `validForMillis` 仍从原服务器截止时间扣减，不能替代 MCU 首次建槽事实；服务端到期、App 关闭和设备槽回收必须分开。

执行：Service 事务/真实隔离 PG→App 有界准备恢复/取消测试→设备单槽真实鉴权控制回归→匹配制品短真机。未完成匹配部署/APK 前不称用户端接替已可用。
随后通用 LAN extension 产品接线/资源/单 ZIP；共享 GATT 10/6 秒公平策略与无缝通信按独立合同收口，不在本包改动。

### 70.2 本批实现与验证

Service `local-access-repository` 沿用原 serializable 事务和锁序：新 scope 没有可复用授权时，原 account/tenant/recipient/caller/self 的不同 scope 精确退休，返回提交结果后才抛 handover；旧 request replay 不进入该分支。
无需 migration/新接口/全局 revision，也不改变既有授权的原始期限。其他用户/caller/recipient 的 active 授权及期限保持；既有全 recipient 待退休签发围栏仍短暂阻止新签发，不等于其他已建立授权被撤销。

App API 仅把真实 HTTP 的 `409 HANDOVER_REQUIRED`、`503 SYNC_REQUIRED` 映射为轻量准备阶段错误，错误返回也检查账户/取消。
独立错误类型不依赖 Angular，connector 不导入 HTTP 实现。首次只读 context 遇到待退休也可有限等待；已打开的链路只在 handover 无 Grant 结果后关闭再开一次。
每段最多 8 次 context 查询、相邻失败间隔 1 秒，整个连接仍受原 30 秒 timer 和单调绝对截止双重约束；无设备发送队列、MQTT或每设备连接池。
重试对同候选执行，实例/安全模式变化拒绝；未知业务不重发，原生清理失败仍占住唯一槽，不假释放。

设备生产实现不变。仅将原 challenge 测试 helper 参数化，复用现有鉴权 Retire，新增真正 `Capacity=1` 的 Plain/Noise：旧槽未退休拒新授权、ACK 缓冲不足不消费控制、旧挑战失效、重复 Retire 精确 ACK 不破坏新挑战、Observer 不获 Control、重开不续期。

| 本批软件检查 | 结果与范围 |
| --- | --- |
| Service 原公开 HTTP＋隔离 PostgreSQL＋控制仓库/worker/carrier | 55/55，含父项；真实 409→503→精确签名协议 ACK→200、并发重放、其他主体/设备隔离、过期偏移边界及退休回滚；不是 MCU ACK |
| App LAN 及原组合关联 | 8 文件 147/147；包含待 ACK、取消/账户变更、清理失败、身份变化、最多一次重握手、非准备错误不重试、总预算和后台 timer 未执行边界 |
| 类型/静态检查 | Gateway TypeScript、App/spec TypeScript、变更文件 eslint 通过 |
| 设备 Host | 单槽测试重编译通过；完整已有 Debug 62/62、架构检查通过；不当跨平台固件重编译或 RAM 实测 |

证据在 ignored `build/b4-scope-20260913/`。本轮未部署常驻服务/迁移业务库、安装 APK、刷板、清授权、暂存/提交/推送；隔离 PG fixture 自动清理自己的随机 schema。
编译测试 helper 初次遇到旧 helper 固定两槽及枚举名称错误，已改为共用模板/实际模式名后重编译通过，未为通过测试增加产品容量。

### 70.3 下一包与未关闭项

本节为软件包完成时的顺序；第 1、2 项及第 3 项的提交后取消/原云通道可用/再次 LAN 恢复已在 §70.4 完成。真实 ACL 变化＋延迟 ACK、页面原 MQTT 控制及取消已在 §70.5 补齐。当前继续第 4 项的竞争边界与第 5 项产品化，不重复本批故障轮次；各短样本不构成就绪延迟 SLA。

1. 先备份当前本机 Gateway 模块，再匹配部署本次唯一 Service 改动、构建/保留数据安装 App；使用现有 COM12 LAN 固件/原身份，只做短闭环。
2. 同 caller 合法 Observer 授权关闭后，正式连接申请 Control，核对精确 Retire 物理 ACK→新 challenge/request→Property/Action/State；另一设备原账户 MQTT 控制作隔离见证，结束回收 owner。
3. 另测 ACL 已变更但 ACK 尚未到达时的准备等待、取消/失败退回原云路线。不得用协议夹具 ACK、清 NVS、全局 Disable 或伪造签发材料代替。
4. 新 caller/换账号的显式接替、自然到期同 scope 的 tombstone、跨目标容量竞争仍未完成，后续要明确竞争/离线可达策略；本批不能标 §31.3 全部完成。
5. 再继续通用 LAN 注入/平台/默认发布矩阵；并列审查共享 GATT 周期退役的连续性，不宣告无缝、不跑百轮代替设计修正。

### 70.4 B4-1 匹配制品短真机与 HTTP 边界修正（2026-09-13）

本机只备份/替换 Gateway `local-access-repository` 编译模块并重启该容器，运行文件 SHA-256 核对一致；未迁移业务数据库或部署其余脏工作树。APK 备份后 `install -r`，保留登录/数据。复用 COM12 原 LAN Self 固件/实例/Key，COM3 原 WiFi 设备作独立云见证；没有刷板、清 NVS、旋转 Key、改 SDK/槽数或增加连接。网络沿用“带翅膀的蜗牛”，手机 Wi-Fi5 与电脑/设备同 LAN。

**首轮失败必须保留**：合法 Observer 成功，设备真实拒绝写入；申请 Control 时 Service 已提交精确退休，MCU ACK 也到达，但 App 报 `GatewayHttpError` 后结束。生产 `ServerInterceptor` 会归一化原始 `HttpErrorResponse`，§70.2 的 API 单测只模拟原始类型，漏掉真实组合边界。

仅修正 `HttpLocalAccessApi.reject`：识别原始 HTTP 或既有 `GatewayHttpError` 的**准确状态码＋错误码**，再转换为独立准备错误；账户/取消检查依旧先行。connector 不依赖 HTTP/Angular 拦截器，不复制恢复器，不将网络错误/任意同名对象变成重试信号。新增经过真实 interceptor 的 HttpClient→API 回归（仅 backend 为夹具）、错误状态/伪对象/换账户/取消负测。8 文件 **149/149**、App/spec TypeScript、两变更文件 eslint 与 debug 构建通过。

修正 APK 再次保留数据安装后的短闭环：

| 门禁 | 实际结果 |
| --- | --- |
| 合法只读准备 | 测试端仅申请 owner 权限的真实子集，不改 Grant/证明；Observer ready 567 ms，写入收到真实 BBP Error code 2，无成功 ACK |
| 同 caller Observer→Control | 唯一正式 connector 收到 handover，关闭旧 native，实际经历 pending context，再同候选新 request/挑战；首失败尝试仅收 65 B Challenge，没有发送 Grant/业务 |
| 精确退休因果顺序 | PG 实际 outbox→物理 MCU exact ACK 664 ms；新授权 `issued_at` 在旧 `delivered_at` 之后，提前约五分钟退休，不靠自然到期；跨端时钟有偏移，仅以同一 Service 时钟证明 ACK→签发顺序 |
| 完整就绪与控制 | 从本次 open 到 Hello/Manifest/State ready **3,025 ms**；Property 两次 ACK 154/133 ms，Action 一次 212 ms；State 确认真→假与按钮计数 +1 |
| 提交后取消 | 下一次请求只读，在真实 handover 结果到达时触发 owner 取消；仅一次握手且未发送 Grant/业务，无自动第二次握手；后台原退休仍获物理 ACK，699 ms |
| 取消后原云通道仍可用 | 用 App 原账户唯一 MQTT 对同一 COM12 两次控制，ACK 206/196 ms，真实 State 真→假；这是明确选择现有云 API 的门禁，不冒充页面自动回退 |
| 再次 LAN 恢复 | 原 caller 再申请 Control，单次握手 **1,423 ms** ready；ACK 162/98/89 ms，真实 State 和按钮再 +1；不重新绑定/清授权 |
| 独立设备/资源 | COM3 原 MQTT ACK 171/211 ms，真实 State 改变；修正 APK 单轮内账户 session 不变。三次就绪连接 native 关闭 13/12/20 ms；测试 owner/订阅已回收，首页、LAN 未占用、BLE 连接 0、云 ready |

证据：ignored `build/b4-scope-20260913/verified-live.json`（逐项断言）、`live-*`、只读 `audit-*`、`app-fixed.xml` 及 `fixed/` APK/构建日志。最终 APK SHA-256 `e8c06d5db51815205a237935f3732dd5234642258db067dcd14763341edb3b6c`；部署模块 SHA-256 `3671d3152c680669d60623804fbf0385254711c7f7ea873588ebb1325ef2a6ae`。首个失败 APK/日志保留诊断，不作为可用制品。旧安装前后 session 不做不变声明，只证明修正 APK 的本轮 MQTT 未重连。

**本节完成时的下一包是实际 ACL＋延迟 ACK/页面原云门禁，现由 §70.5 补齐。** §70.4 本身的 scope 变化不是分享 ACL 变化，未测另一账户或页面自动回云；Observer 请求使用现有内部注入端口限定权限，Control 接替逻辑和真实签发/握手不作替换。
新 caller/换账号、自然到期同 scope tombstone、网关多目标容量、Noise/其它 MCU 和通用 LAN 发布矩阵仍未关闭。3,025 ms 是单次完整准备实测，不是普通命令延迟、统计 SLA 或无缝声明；旧 GATT 10/6 秒问题独立保留。没有暂存/提交/推送、开放生产默认或把长循环当设计修正。

### 70.5 B4-1 实际 ACL、延迟退休与页面原 MQTT 闭环（2026-09-13）

**本包通过的范围**：真实两账号定向分享、公开 PATCH 缩为仅 `power` 的 operator、COM12 原物理设备 exact Retire ACK；Android 已安装匹配 APK，通过真实设备页面处理器执行控制。不是显式调用另一个云端测试入口代替页面，也没有额外 MQTT 会话、模拟 Grant/ACK、刷板或清 NVS。手机/电脑/设备仍在“带翅膀的蜗牛”同 LAN。

本次发现并修正两个不同层次的问题：

- **App 页面旧判断**：共享设备库存 `cloudReachable=null` 时，即使所选 LAN 已鉴权且 State 新鲜，控件仍被禁用。删除库存 presence 对操作的直接否决，`canControl` 改消费 `DeviceUiPort` 已按所选通道提供的 ready＋fresh snapshot，保留同步、页面可见、App 活动与 scope 未取消条件。云端离线/丢失 presence/旧 Manifest 仍由原 UI port 的 fresh 判断拦截；实际写入仍走原 LAN 权限或云端逐组件 ACL，不把“控件可点”当授权。
- **本机部署遗漏**：最初只开启 Gateway 分享/跨片，两个 Broker relay 仍关闭，真实 StateInterest 返回 rejected，页面报 `All promises were rejected`。这是 §69 已记录的配置组合，不是 BBP/2 或 MCU 登录故障。备份两个 Broker 的确切运行入口后，仅临时开启既有跨片开关；未重建镜像、覆盖其它运行模块或改生产默认。以后双账号跨片 HIL 的前置检查必须包含 Gateway **和源/目标 Broker**，先证实正常共享云控，再注入 LAN 故障。

故障只作用于本机 Gateway 原 LocalControl worker 的 COM12 publish 回调：有限 30 秒（取消对照 15 秒）失败窗口，原 outbox/claim/退避继续运行。普通业务 MQTT 不拦截，不伪造发送成功或退休 ACK；恢复由原 worker 真实发送、设备真实应答。它测的是投递暂不可用，不是损坏无线包或直接篡改设备权限。

| 真机项目 | 本批证据 |
| --- | --- |
| ACL 缩权后的实际页面 | 页面 **3,191 ms** 云端 ready；LAN context 同时 pending。页面经原账户 MQTT 两次 Property ACK **141/137 ms**，State 真→假 |
| 有界准备失败不带走页面 | 仅 8 次 context 查询，**7,826 ms** 结束本地准备，没有新 LAN 握手/Grant/业务发送；native 已释放，页面 group 仍在，云控继续可用 |
| 逐组件 ACL | 被移出允许列表的 `button` 经云返回 Route Error **6**，真实按钮计数保持 4；不是本地 target-unavailable 的假拒绝 |
| 实际退休 | 3 次 publish failure 后第 4 次真实投递成功，退休提交→物理 exact ACK **46,021 ms**，远早于旧授权自然到期；两个 Broker 的 ACL version 14 均已独立确认。46 秒是注入失败叠加原退避，不是每次连接必须等待的时间 |
| ACK 后显式重进 | 仅 Observer LAN 权限 **1**，完整 LAN ready **1,435 ms**；读取可用，`power` 在发送本地业务前选择既有云端，ACK **234 ms**；`button` 仍被云端拒绝，不因 LAN ready 越权 |
| 真实 pending 时退出 | 另一次实际 ACL 变化，首个 pending 后退出，**27 ms** 回收；仅 1 次 context、无新 session/业务，随后 2.5 秒无迟到查询/握手；取消不回滚退休，原 worker 后续物理 ACK **21,134 ms** |
| 原权限恢复与收尾 | 测试 share 仅撤销本轮接受的一项，所有实际 retirement 均 exact ACK、两 Broker 最新失效投递均完成；恢复原 owner 后 LAN 两次 ACK **83/182 ms**，State 真→假。首页、0 page group、native 未占用、BLE links 0、云 ready |

上述成功共享阶段原账户 session 对象保持，观测期间 cloud 状态保持 ready；安装、切换账号、启停 Broker 的前后不做“从未重连”的声明。源代码只改页面 ready 判据及回归，不新增 composition、队列、重试器或 SDK 依赖。10 文件 **187/187**，App/spec TypeScript、三个变更文件 eslint、debug 构建和保留数据安装通过。

证据：ignored `build/b4-acl-20260913/verified-live.json`（由 `verify.cjs` 对原始记录断言）、`page-*`、`server-audit-*`、`fault-evidence.jsonl`、`app-final.xml`、`runtime-restored.json`。APK SHA-256 `296fe07607241be04a9ce5b482c06840adac5a3579e6678b3c07b0d2befb80f8`。Gateway/Broker 运行入口已精确还原，临时故障文件已移除；§70.4 的匹配 `local-access-repository` 模块 SHA 保持不变。修正 APK 保留，无暂存/提交/推送。

**不隐去失败样本/安全边界**：首次只开 Gateway 的共享云控失败已保留；另一次增权立即重进在测试工具 8 秒观察窗内尚未 cloud ready，工具随后主动退出。这不等于产品 30 秒预算耗尽，也不能将成功的 3,191 ms 宣称为所有 ACL 切换上界。该增权窗口的 StateInterest/Manifest/presence 时序应随下一批边界检查定点复核，不增加无界重试。候选 LAN 会话曾一次 receive-failed，也不计为成功恢复样本。

设备在实际 Retire 到达前仍可能执行未到期旧 Grant；这属于有限离线授权窗口。本批没有在延迟期间保持旧会话做恶意写入攻击，不声称云端撤权瞬间就远程抹除离线设备权限。正常 App 不复用旧 Grant 越过 pending，云端缩权立即按当前 ACL 处理，两者不能混为一层。

**下一包执行顺序**：先收窄核对新 caller/换账号的显式接替、自然到期同 scope/tombstone 与容量竞争，禁止静默抢占其它 caller、全局 revision 清槽或扩大 MCU 容量；同时仅定点复核上述增权就绪样本，不重跑分享/注册长循环。边界合同与有界测试确定后，继续通用 LAN extension 的 WiFi/WiFi+BLE/Hub 接线、声明平台资源与单 ZIP 门禁。Noise/其它 MCU、旧共享 GATT 10/6 秒连续通信仍独立未完成，不转去新增网关协议或宣称无缝完成。

## 71. 核心稳定 C2：增权与同 caller 自然到期恢复已收口（2026-09-14）

**§70.5 上述“下一包”已被[核心稳定主线 §10](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md#10-c2-已知恢复断点收口下一步-c32026-09-14)取代。当前进入 C3 基础矩阵；新 caller 抢占、通用 LAN 接线、更多网关功能仍后置。**

本次只修 Service 现有签发事务：未过期同 scope 复用原授权；同 caller/self 的原 scope 改变或租约自然到期，在 MCU 仍可能持有原授权/tombstone 的保守区间内，一律复用既有精确退休 outbox。到期审计原因为 `caller_lease_expired`，不新增表或 wire。首次请求只提交退休并返回原 HANDOVER；原 worker 真实 ACK 后才允许新 request/challenge 签发。账号/tenant/recipient/caller/self 过滤不放宽；未过期其他 caller 和设备不清槽。不能因服务端截止到达就直接假定 MCU 有空槽。

隔离 PG 先复现旧实现同 scope 到期漏退休，修正后五文件 **56/56**；App 原 builder 九文件 **163/163**，未改 App/MCU 生产源码；既有设备 Debug host **62/62**。真实 COM12 同固件：operator 仅 power→全权限后云端 ready **1,961 ms**、页面 **1,986 ms**、LAN **2,269 ms**，同一账户 MQTT 不变，随后 Property/Action 通过。旧 8 秒测试观察失败不再是已知未收口断点，但有限成功样本不是 SLA。

同 caller/同 scope 使用原协议真正申请 15 秒短租约，两次自然到期退休 ACK **421/1,065 ms**；新授权严格晚于物理 ACK（同 Service 审计时钟）。第一轮验证旧 session 拒绝发送且没有新增帧、云控继续，并退出 pending 后显式重入；第二轮全程不退出页面，原页面恢复新 LAN **3,106 ms**，不替换账户 MQTT、不重复 Action。首个 handover 尝试只收到 Challenge，无 Grant/业务发送；后一次使用新的请求标识。

结束时真实测试分享撤销、两 Broker 失效版本 22 已确认，owner/三块板原 MQTT 见证正常，页面连接/原生占用/测试观察器清零。三份临时 Gateway/Broker 入口逐字节恢复；当前进程 LAN 开关恢复 false，原 APK 与三块板固件均未替换。只保留修正后的 Service 仓库模块；不是新的正式部署配置。制品哈希、证据交叉验证和失败样本见主线 §10。

未测边界仍明确：持续 Telemetry 采样真机、其他 MCU/Noise 真机、任意网络丢包/跨安装竞争、新 caller 自动接替、全部平台 LAN 和零等待切换。本批旧 LAN 停帧＋host 待发队列到期停止不能替代 C3 的逐平台短采样停止检查。
