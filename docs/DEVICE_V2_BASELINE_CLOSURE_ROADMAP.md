# Device V2 基础能力收口路线：R3余项、接入、绑定、授时与LAN

**2026-09-22 执行优先级只维护在[新进度与执行计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md) §6。C1～C3 冻结范围和缓存优化均已收口；新增用户任务不是 C3 已验收内容，见[共同合同](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md)。本页保留历史证据，不按下方历史“下一包”重做 C1/C2/C3、开启抢占/网关/LAN 扩展或恢复 C4 整组维护。**

更新：2026-09-14。范围：`blinker-library`、`aily-blinker-services`、`blinker-app`。本页是基础能力与历史批次索引，详细协议不复制到这里；R3测试细节继续归Service跨分片方案。当前执行顺序以[核心稳定版唯一入口](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md)为准，下方历史批次的“下一步”不再作为任务队列。

## 1. 结论与主线边界

最新 **B4-1 实际 ACL/页面闭环（2026-09-13，优先于下方历史批次）**：[LAN §70.5](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#705-b4-1-实际-acl延迟退休与页面原-mqtt-闭环2026-09-13)。真实分享缩权、延迟物理退休 ACK、页面原 MQTT 控制 141/137 ms、LAN 8 次有界等待后页面保持、Observer 恢复/逐组件拒绝、首个 pending 后取消均已验证。App 删除以库存在线提示否决已就绪 LAN 的旧判断，统一使用所选通道 ready/fresh 与原生命周期围栏；187/187、类型/lint/匹配 APK 通过。首轮 Broker 跨片未开启导致的失败已明确隔离，测试后 Gateway/两 Broker 原入口和开关还原、测试 share 撤销/退休结算、原 owner 恢复，未刷板/清 NVS/提交。
**本工作包收口的是实际 ACL 的有限准备和原云路线，不是全部 B4 或无缝 SLA。** 下一步定点处理新 caller/换账号显式接替、自然到期/tombstone/容量边界，并复核一次增权立即重进在工具 8 秒观察窗内尚未 ready 的样本；之后继续通用 LAN extension/平台资源/单 ZIP。不同调用者不能自动抢占，延迟 ACK 前旧 Grant 的有限离线权限不能表述为已即时消失；不重复扩分享、授时或百轮测试。

最新 **B4-1 同 caller/self scope 接替（2026-09-13）**：[LAN §70](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#70-b4-1-同-callerself-scope-接替2026-09-13)。
Service 精确退休提交后返回签发前 handover；App 关闭旧链路、有限等物理 ACK 同步就绪，再一次新握手，复用原 POST/outbox/worker。设备生产代码/槽数不变，单槽 Plain/Noise Host 边界通过。
隔离 PG/HTTP 55/55、Host 62/62/架构已过。**本机单模块/匹配 APK 的 Plain 短闭环已过**（LAN §70.4）：真机暴露 HTTP 拦截器把错误归一化后漏识别，已在 API 边界修正并补真实拦截器回归，App 149/149、类型/eslint/构建通过。精确退休 ACK 664 ms，完整接替 ready 3,025 ms，Property/Action/真实 State、提交后取消、原云通道控制与再次 LAN 恢复通过，测试 owner 已回收、原固件/NVS 不变。该批次后续的真实 ACL/页面门禁现已由 §70.5 补齐；当前顺序见上方最新入口。其它 caller/设备不自动抢占，旧 10/6 秒 GATT 连续性问题仍独立保留，不宣告无缝。
以下 B3 为已完成的短闭环历史，不重复授时/注册或百轮；本包不重刷 MCU、清授权或新增 wire/表/队列。

最新 **B3-4c-4b 南向（2026-09-13）**：[授时 §19](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#19-b3-4c-4b-南向真机与本地发送背压修正2026-09-13)。
原 Hub/COM12 普通 BLE 子设备的真实 Method 2、授时关联、Property/Action/State 与自然断开 holdover/恢复已过。
真机发现本地 TX `WouldBlock` 与远端失败共用 30 秒退避会错过短 BLE 窗口，已拆为本地 1 秒有界重试；无新增队列/时钟/用户 API，Host 62/62、包 13/13、匹配 ZIP 编译和修正真机通过。
**本组南向短闭环已收口**：手机接回后活动会话 `d=0`、holdover/控制恢复、45 秒 provider 无源时真实 Error/控制及自动授时恢复均通过。COM11/COM12 已再次恢复精确原 WiFi 应用并完成 Property/Action/State 复验；授权/NVS 保留、测试 owner 已回收。USB 间隔账户 MQTT 曾重连，补验内部使用原账号当前唯一 session，不冒称跨失联不变。
**下一包回 B4**：先 LAN §31.3 单槽 scope/caller 的退休/接替合同与有界回归，再通用 extension/声明平台/资源及标准 ZIP/短真机；不重做已过授时、注册或百轮。
旧 GATT 10 秒占用/6 秒让出仍会造成窗口边缘控制失败，本轮未改，不能宣告无缝；B4 通用 LAN/单槽 scope 接替与共享 executor 持续通信需明确分别验收。
下方 §18 的“南向未过”为历史；生产 protected-NVS/eFuse、Observer BLE/旧 nonce、默认一小时、其它平台与加密 LAN 仍独立保留。

最新 **B3-4c-4b 自身台架（2026-09-13）**：[Hub 授时/普通重启 §18](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#18-b3-4c-4bhub-自身台架准入与普通重启2026-09-13)。
原 Hub 双板台架可选共享业务服务/两个 responder、新 TLS profile 标准 ZIP 安装编译及自身真实 TLS 登录/管理订阅/授时通过；重启观察到 UTC 清零后重新授时，原账号 App Manifest/State 保持。
COM11 原普通 WiFi 固件/Key/ID 已恢复，Property/Action 和 COM3 TLS 见证通过；App 测试 owner/本地连接回收，新修正版 APK 保留。
**真实孩子会话未建立，南向授时未通过；弱存储台架不能替代生产 protected-NVS/eFuse 门禁。** 下一包仅补原 COM12 孩子 Method 2/Time/控制/断开恢复，再 B4，不再重复检查点或新增产品图。

最新 **B3-4c-4a-r（2026-09-13）**：[匹配 APK/仅重置网络真机 §17](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#17-b3-4c-4a-r匹配-apk-与仅重置网络真机2026-09-13)。
修正包已保留数据安装；真实添加入口 resolve→精确 load、模糊查找 0 次、Key 幂等重用/原 epoch 与 BLE 凭据保持、自动回原页及 BLE/MQTT 并行控制通过。
只清 WiFi profile 一次，不清授权/重建设备；COM11 原普通 WiFi 固件/ID/Key 已恢复控制，原账户 MQTT 保持。App 72 项/类型与构建、Host 62/62、包 13/13 通过。
旧缓存共存由软件覆盖，没有伪造已退役凭据充当真机证据。**下一步 B3-4c-4b Hub/子设备短真机 → B4**；下方“修正 APK 未安装”为历史。

最新 **B3-4c-4a（2026-09-13）**：[WiFiBLE 短真机 §16](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#16-b3-4c-4a正式-wifible-图的短真机闭环2026-09-13)。
正式 WiFiProv/WiFiBLE 图保留原 BLE ID/凭据启用 WiFi，云授时、页面 BLE 与 MQTT Property/Action 并行、退出页和普通重启通过；原 COM11 固件/Key/ID 控制已恢复，App 原账户 MQTT 未重连。
新增内部探针与 LAN 共用观测器，Host 62/62、包 13/13、标准 ZIP 安装编译通过。
现场旧退役 WiFiProv 检查点误选已定位，精确清理一条测试残留后完成上述真机；App 已修正为先 resolve 当前逻辑身份再读检查点，72 项/类型检查通过，但该修正 APK 尚未安装验证。
**下一步：匹配 APK 的检查点回归 → B3-4c-4b Hub/子设备短真机 → B4**。不把本轮混合设备自身授时冒充南向、加密 LAN、全平台或生产存储通过。
下方“B3-4c-4 尚未任何真机”的记录是历史；其未完成边界继续保留。

最新 **B3-4c-3（2026-09-13）**：[南向可选模块与软件边界 §15](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#15-b3-4c-3唯一南向-executor-的可选授时2026-09-13)。
已接 Method 2 真实权限快照、按模块安装协商 Hello、唯一 executor 的 link-local 回应和 exact-ciphertext 背压/退役围栏。
默认不安装模块；两个孩子各 16 B 可选 responder，共享一个 112 B 业务服务，不增加队列/连接/公开初始化入口。
Host 62/62、架构/profile、包 13/13、九组新旧 ABI、最终标准 ZIP 三目标安装编译/ELF 已过。
ESP32 3.3.11 同 TLS profile，两份南向模块相对自身授时版仅增加程序 1,440 B / 全局 RAM 32 B，默认不链接时钟/应答；完整限制见 §15。
**下一步为 B3-4c-4 短真机准入/闭环，再回 B4**；本批没有新真机通过声明，不烧 eFuse，也不因台架成功放宽生产 Hub 存储策略。
下方 B3-4c-1/2 的“待接南向”为历史，不应重复实施；LAN 默认产品化/单槽 scope 接替等余项仍保留。

此前 **B3-4c-1/2（2026-09-13）**：[自身时钟、共享快照与南向边界 §14](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#14-b3-4c-12混合与-hub-自身共享快照和回应策略2026-09-13)。
已实现 WiFiBLE/Hub 自身 attachTime、最小只读业务快照、独立 12 B responder 策略，复用一个 112 B 服务，不增加 composition 成员/连接/公开初始化形式。
Host 62/62、授时专项 4/4、包检查 13/13、九项 ABI、架构/profile 及四目标单 ZIP 默认/开启安装编译/链接均已过。
ESP32 3.3.11 同 TLS profile 的自身授时增量：WiFiBLE 程序/RAM +3,076/+120 B，Hub +2,992/+112 B；默认不链接业务时钟，四版均无子设备 responder。
**南向未接入、未宣告子设备授时**：下一包优先补真实 Method 2 权限快照→按实际安装协商 Hello→唯一 executor 回应及旧会话/背压围栏→资源/短真机。
当前安全会话标志不等于 Control；默认产品不得因引用 concrete responder 而强制链接时钟，不把软件回应测试冒称网关子设备授时已通。
完成后回 B4 通用 LAN/平台/默认发布和下文单槽 scope 接替；本包不动硬件/App/Service 生产运行环境，不重做 §13 已过闭环。

此前 **B3-4b LAN 明文真机（2026-09-13）**：[授时真机与后续顺序 §13](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#13-b3-4b-lan-授时真机2026-09-13)。
匹配 APK/独立 ZIP 已安装验证；只暂停 COM11 MQTT TCP，WiFi/UTC 保持，原定 **300,173 ms** 到期经 LAN 授时，完整 BBP **28/40 B**。
Control ready 1,484 ms，Property/Action 168/84 ms；恢复云源仍按五分钟间隔。真实 Observer 明确清业务锚点后 12,148 ms 不授时，写控制被拒；
原账户 MQTT/COM3 断云期间见证保持。COM11 已精确恢复原静默 WiFi，原 Key/ID 控制复验通过，测试 owner/工厂已回收。
新增跨来源刷新回归并合并重复构造，host 61/61、架构/profile 通过；无生产故障开关、清 NVS、部署/迁移或新连接。
**限制不能遗漏：**同 caller 改权限产生新 scope，原单槽授权到期/15 秒保留窗口前仍可能拒绝新握手；首次 Observer 失败保留，等待后成功。
此项继续留 B4，不能宣称即时接替/无缝。源码 LAN 默认仍关闭，Plain 结果不替代加密/全平台产品化。
下一步 **B3-4c：混合/Hub 自身 attachTime → 只读业务快照 → 唯一南向 executor responder → 软件/资源/短真机 → B4**。
原 §12 的 BLE Control 真机与 R4 默认/开启标准 ZIP 对照已过，不重复。Observer BLE、主动旧 nonce、默认一小时过期、R4/Wio 当前硬件仍单列。

此前 **B3-4a（2026-09-13）**：[普通 BLE Control 手机授时与剩余顺序 §11](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#11-b3-4a普通-ble-control-手机业务授时2026-09-13)。
已接纯 BLE 的可选同一时钟、设备云源优先/本地 Control 选择及精确会话取消；App 共用 Direct responder 经 Method 2 权限验证后安装。
Hello 能力不当权限；BLE 发帧最多 2 项、关闭立即取消原生链路并拒绝迟到异步写入。不新增连接/页面/每 peer 时钟。
host 61/61、App 36 文件/411 项与类型检查通过；九项 ABI 服务仍 112 B，ESP32-S3 同 ZIP 开启 BLE 授时增量为程序 2,972 B / 全局 RAM 120 B。
安装编译/资源明细统一记于 §11；R4 冷构建超时及跳过扫描的无效链接保留为未通过，须用标准 CLI 补完。
未安装本批 App/固件，不能称 BLE 真机授时通过。下一步补 R4 门禁、准备匹配 App/纯 BLE 短闭环，
再 B3-4b LAN 实际获授权限传递，随后 B3-4c 混合/网关回应，完成后回 B4。
此前 B3-3b 的同步日志对照、真实 30 秒离线及 300,018 ms 刷新已通过；默认一小时硬件过期仍单列，不重开百轮或共享 Broker 部署。

此前 **B3-3（2026-09-13）**：[业务授时实现与验证 §8](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#8-b3-3普通-wifi-可选业务授时2026-09-13)。
普通 WiFi 已通过 ITimeSync 注入原 Runtime，公开 `enableTime()` + `time(uint64_t&)`，默认关闭、begin 不变；
原 MQTT Hello/有效物理 session responder、5 分钟刷新/30 秒重试、断线保留业务 UTC、重启清零已接线。
host 61/61、Broker 扩展 193/193 与 V2-only、App 原协议 156/156、架构/冻结及包边界通过；
三平台 ABI 的启用服务 112 B，Small 默认新增 8 B 指针，不能称默认零 RAM。Broker shard 1 五模块已备份后部署，
未迁移/清授权/重建账号，其他服务不动；COM11 原固件在部署后控制/State 通过。
同 ZIP 两示例安装/编译/链接已通过；COM11 原身份授时、Property、14 样本启停/恢复与普通重启已过，未清授权。
最终恢复同包静默 QuickStart 并通过 Property/Action，COM3 TLS State 见证通过；App 生产代码/安装包未变。
**当时的下一小包为 B3-3b；当前进度以上方入口和授时 §9 为准，随后 B3-4，再 B4。**
一次串口重开段云端重连导致首轮采样失败，连续读取后的复测通过；不能把复测解释为该资源边界已解决。
Direct/LAN/网关授时 responder 尚未接线；R4/Wio 当前硬件与离线过期实测不能由 host/ABI 代替，不扩长循环或新登录协议。

此前 **B3-2（2026-09-13）**：[业务授时合同与实施记录 §7](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#7-b3-2-实施与验证2026-09-13)。
三端 codec/golden 与设备单在途协调器完成，0x22/0x23 + bit 24；典型完整 BBP 请求/响应 28/40 B。
原 CBOR/编译单元复用；peer/代际/nonce/sequence/权限/RTT 关联和超时/重复拒绝，不改安全 IClock。
host 60/60、Broker 专项 136/136 及原 V2-only、App 156/156、类型、架构/冻结及发布包 11/11 通过。
三种 32 位 ABI 均为 WallClock 40 B + 协调器 40 B，未启用链接无模块符号；不是新产品/MCU 实测。
**下一步 B3-3：原 WiFi MQTT 会话云回应/Hello→可选产品调度与 time 查询→COM11 短闭环；随后 B3-4 Direct/LAN/网关来源**。
当前仅 codec 可用，默认 Hello 不宣告授时、没有自动 responder/公开 time API；没有刷板/清授权/部署/提交，不重复 B1b。

此前 **B1b-3（2026-09-13）**：[B1 报告 §6](DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md#6-b1b-3默认明文公开示例与租约采样真机闭环2026-09-13)。用户放行后，手机真实 LAN HTTP/MQTT 准入已验；
COM11 同 ZIP 静默 QuickStart 登录/Manifest/State/Property/Action、普通重启及恢复正常固件均通过。
同包 CloudSmokeTest 的 onSample 租约启停/恢复共 14 样本、关闭后设备计数保持 14；重启后无租约计数保持 0，仍可控制。
服务端只读证据确认同设备/Key/实例使用 plain session，COM3/COM12 保持 TLS；COM3 最终真实 State 变化/控制通过。
只写 COM11 应用区，未清分区/NVS/授权；未部署服务、迁移/建号、改 MCU/App 生产源码或提交推送。
**B1b 的 ESP32-S3 默认普通 WiFi 本工作包已过，不等于全平台 B1 完成**：R4/Wio 当前硬件接入、干净 SDK/CI 等继续单列。
下一步 **B3-0 冻结最小授时合同/接口与来源边界→B3 实现/短闭环→B4**；不重复本批烧录/建号或扩百轮。
本页 §4 同时修正业务 UTC 不能直接接管已有安全 IClock；首次分配窗口/B2c 等既有边界仍未冒称通过。

此前 **B1b-2（2026-09-13）**：[B1 报告 §5](DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md#5-b1b-2受控部署与真实-profile-双示例安装编译2026-09-13)。已备份并只更新唯一 Gateway，
移除 TLS/账户 WS 后端裸 TCP 的宿主机映射；独立 device-plain MQTT 和受限 challenge/session HTTP 已在本机部署。
原 App 账户恢复、COM3 TLS Property ACK 前后 108/104 ms；本机 HTTPS/HTTP 路径隔离已验。
真实 profile 同 ZIP 独立安装，公开 QuickStart/CloudSmokeTest 均编译、来源及 map/ELF 检查通过，
只链接 Blinker/PubSubClient/WiFi/Network。App 正常创建一个专用 COM11 测试设备；旧退休 ID 不复活。
**未刷 COM11、未通过默认明文 MCU 闭环**：手机到新 17002 端口超时，现有防火墙未包含 17002/18884，
待管理员放行 LocalSubnet 后复验再刷已校验固件。COM11 仅精确 MAC 核对/应用区读备份及普通复位，COM3/COM12 固件/Key 保留。
**下一步不重复部署/建号/冷编译：确认 LAN 新端口→COM11 QuickStart 登录/控制/重启→CloudSmokeTest 租约采样/退出回收**。
R4/Wio 当前真机、上游干净 CI 与 B2 独立边界继续单列；之后 B3→B4，无暂存/提交/推送。

此前 **B1b-1（2026-09-13）**：[B1 报告 §4](DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md#4-b1b-1默认明文真机前置修正2026-09-13)、
[Service 明文/TLS 共存](../../aily-blinker-services/docs/DEVICE_V2_PLAIN_TLS_MQTT_COEXISTENCE.md)。
发现并修复两条旧政策：全局 mqtts 拒绝普通设备 plain session；production 环境无条件关闭已明确允许的 plain。
同一 Gateway 可选独立 device-only plain listener，复用原 CONNECT/复核/转发，App 仍原账户 MQTT、Edge Hub 仍 TLS；
不改设备 API/BBP/存储。软件专项 **25/25**、隔离 PG **21/21**、公开 endpoint/WS、凭据安全与 Gateway 类型/构建通过。
**本批未部署/开端口/刷板/清授权**，不冒称普通明文真机通过。实际部署还发现旧 TLS 后端 TCP 对 LAN 暴露，
下一包须一并隔离该入口（代理实际走 Docker 内网），不能将新 plain 端口接到旧 TLS listener。
**下一步：备份/复核单 Gateway 部署及受限设备 HTTP→同一真实 profile ZIP→COM11 静默 QuickStart 与采样/普通重启**。
COM3/COM12 保留；B1a 不重复，B3/B4 与未完成的 B2 边界仍按原顺序。

最新 **B1a（2026-09-13）**：[单 ZIP 安装矩阵](DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md)。当前已回到 B1，
修正发布单元清单/四个已复核的登录冻结摘要，补安装字节、工作树源码隔离及未选 LAN/网关链接检查；
当前源码三次 ZIP 重建一致，包边界 11/11、host 58/58 通过；**七目标本机安装编译、依赖集合、provenance 和
map/ELF 全部通过**。修正 Wio 的 size 口径、过期架构/API 文档及 CI preview 依赖准备；无 MCU/App/Service
生产源码修改。SDK/依赖来自本机缓存，ESP32-S3 为明确记录配置的 IDF NimBLE SDK；上游干净 CI 和构建时间
预算仍单列，不宣称所有默认 SDK 或生产环境已通过。
**B1a 是安装编译，B1b 才是当前默认明文公开示例真机及 R4/Wio 平台接入补证**；不拿 development profile
占位包或历史 TLS 副本冒充 B1b。**下一步先 B1b 默认明文公开示例的登录/Manifest/State/Property/Action、
采样与普通重启短真机闭环**，平台硬件按现场补证；之后仍 B3→B4，不扩解绑故障矩阵、网关或百轮。
本批无刷板/清授权、暂存/提交/推送。

最新 **B2b-13（2026-09-13）**，优先于下列历史：[混合报告 §7](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)、[owner §21](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md)。精确 COM11 **同 ID reset→1 次 Key 轮换/条件 Admin PATCH→旧 Controller/MQTT/Key 拒绝→新双通道与重启→联合 owner 删除 202/200** 已过；COM3 同账号 24 次控制、COM12 保持。最后明确物理 resetAccess，GetInfo 同实例/空根，COM11 现待配网，原测试 ID 已退休。App 修复配网与 Presence/Direct 扫描所有权交接，135/135、类型/构建及新 APK 真机交接/回收通过，host 58/58。**下一步回 B1 单 ZIP/默认公开示例与平台当前版本矩阵，再 B3→B4**；不重复部署/清根/换根或扩百轮。首次分配窗口、B2c 独立本地授权/另一账号与未测故障仍单列；混合鉴权失败停本地≠擦 Flash，Service metadata 审计保留≠可继续准入。无暂存/提交/推送。

最新 B2b-12（2026-09-13，优先于下列历史）：[混合真机报告 §6](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)、[owner §20](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md)。**COM11 错误 CA 已用真机对照定位并修正，同一 ID/Key/根下 MQTT、自动 BLE 页面及普通重启已过；COM3 已恢复并连续自然续期，最终另一设备 MQTT 见证通过。** 库内仅补 HTTP 传输截断原事务重试与退避错误透传，58/58 host；无新 wire/状态槽，RAM 56,108 B。COM3 历史退化根因未捕获，不宣称全部故障自动恢复完成。**下一步：精确 COM11 的 reset→同 ID、新 Key/Admin 条件恢复→旧授权拒绝→联合 owner 移除，再 B1→B3→B4。** 不重复 CA 排查、038 部署或 enable-cloud；首次分配窗口、B4 LAN 旧类型筛选继续单列，无暂存/提交/推送。

跨日续测（2026-09-13，优先于下列历史摘要）：[B2b-11 报告 §5](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md#5-跨日补测自动-ble-页面及显式拓扑路由)。已修复 BLE-origin 启用自身 WiFi 后被误判为网关子设备，改用服务端有效拓扑 `gatewayRouted`；App 223/223、PG 65/65，匹配本机 Gateway/APK 已部署。自动页面 BLE ready 3,638 ms、Property/Action 334/350 ms，退出后连接回收。**下一步是混合设备 cloudReady=0/无 challenge-session 与最新 COM3 MQTT 见证离线诊断，不再重复部署 038、enable-cloud 或改根。** 两条云端路径恢复后再混合 reset/PATCH/联合移除→B1→B3→B4；首次分配窗口和 LAN 旧类型筛选继续保留，未宣布全部基础能力完成。

最新 B2b-11（2026-09-12）：[混合设备部署/真机报告](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)。**037→038 存量备份/完整恢复演练、8 个关联模块本机部署和匹配 APK 安装已完成，不重复作为下一步**。44 张原表数据保持；App 116/116、PG 65/65（重跑）、工具 12/12。COM11 已仅应用改烧混合产品，原实例/epoch 保留，真实 enable-cloud 一次且不新建设备/不轮换 Key，自动进入原设备页；原根 Direct 控制及 COM3 前后 MQTT 见证已过。实际云端控制、自动 BLE-first 及换根/联合移除按报告逐项补证；首次 create→pending 窗口仍未关闭。剩余混合短门禁之后 B1→B3→B4，不扩网关/百轮；以下各批为历史。

最新 B2b-10（2026-09-12）：[owner §18](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#18-b2b-10-key-轮换前安全检查点与页面恢复接线2026-09-12)。**已有设备重新配网的新根路径已在原页面/协调器接线**：先安全保存 prepared，再用固定幂等 ID 轮换 Key，保存 key-ready，PATCH/PUT→复查 GetInfo→原 Bootstrap/ACK→active。页面重建不再重新分配 Admin/轮换 ID，网络重试保留真实新 epoch；删除临时阻断方法和重复字节比较。App 十文件 **116/116**、App/spec/Gateway 类型及完整 local 构建通过；本批 Service/MCU 生产代码未改，未部署/迁移业务库/安装/刷板/清授权。下一包先 037→038 存量升级演练/备份、关联部署，再专用混合真机；**首次 create→pending 的分配窗口仍未关闭**，不宣称 B2b 全完成。随后 B1→B3→B4；以下各批入口为历史。

最新 B2b-9（2026-09-12）：[owner §17](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#17-b2b-9-新根条件替换实例连续性及旧授权隔离2026-09-12)。**新 Admin 条件替换的服务器事务和 App 管理适配器已实现，页面尚未开放**。migration 038 保存非秘密操作/实例连续性；BLE→WiFi 旧 intent 退休后仍保留原 ID，旧 receipt/fresh claim 不复活。保留页面/账号分享策略，退休原本地材料、刷新原失效队列；修复 Presence version 1 跨 root 重放别名。PG 65/65、App 95/95、类型及 local 构建通过。下一步固定为：安全检查点前移到 rotate 之前→原协调器接 PATCH/Bootstrap/ACK 恢复→解除页面保护→备份/受控部署混合真机→B1/B3/B4。未部署/迁移业务库/安装/刷板/清授权；无 MCU 常驻资源增长。以下各批“下一步”为历史，不据 API 完成就宣布端到端完成。

最新 B2b-8（2026-09-12）：[owner 移除 §16](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#16-b2b-8-wifiprov-首次-admin-登记与检查点恢复2026-09-12)。WiFiProv **pending 安全保存→原 Admin PUT→exact Bootstrap→active→网络**已软件接线；页面不持有 Admin secret，重建协调器/页面复用检查点，ACK 不确定不退成 Key-only Install。原 endpoint 增加 owner metadata GET，新根冲突在继续刷新 Key 前阻断；**reset 后保留原 ID 替换新 Admin 尚未实现**，下一包先做条件更新/来源映射与检查点窗口收口，再受控部署混合真机。App 92/92、Service 59/59、类型及 local 构建通过；未部署/安装/刷板/清授权，无 MCU 协议或资源增长。以下“App 未调用 Admin 登记”为历史；旧 active 无检查点迁移、分配→pending 窗口等边界不冒称完成。

最新 B2b-7（2026-09-12）：[owner 移除 §15](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#15-b2b-7-混合身份联合退休软件收口2026-09-12)。混合身份联合退休软件已通过：WiFiProv 云端身份不要求纯 BLE intent；BLE 启用 WiFi 后保留原 ID/type，按实际云端版本同时撤 Key/session 与 BLE 历史/Presence。PG **59/59**、host **57/57**、App **26/26**，复用原事务/worker，无新协议/存储槽；网关历史仍 409。本批未部署或真机。发现 WiFiProv 新建链尚未调用既有 Direct Admin 登记 API，且该 API 不支持重置后直接覆盖旧根；**下一小包先闭合这条登记/恢复链，再专用 WiFi+BLE 真机，随后 B1→B3→B4**。不能将纯 BLE、已有元数据的测试夹具或本机 Direct 控制当作该缺口已完成。以下各批“下一步”为历史。

最新 D1b（2026-09-12）：[诊断 §9.6](DEVICE_V2_DIAGNOSTICS_OUTBOUND_TELEMETRY_PLAN.md#96-d1b-首个平台闭环单次输出显式能力与真实背压2026-09-12)。单次输出/拒绝计数/重入保护、共用有界 formatter、显式 IDF USB 零等待适配已实现；57/57 host、ESP32-S3 同句柄“25 s 暂停读取→28 s 恢复读取”真实 BLE ACK 通过，输出段最大 179 μs。普通 Print 仍同步，R4/Wio/Arduino HWCDC 不冒称已安全适配。COM11 已恢复本批核心的静默固件且保留授权，COM3 WiFi 见证通过；无本批 App/Service 改动。**D1b 首个平台小包已闭环，下一步回混合 WiFi+BLE 的 Key/访问根联合退休与 B1 平台矩阵，再 B3/B4；不继续扩日志、网关或长循环。** 以下各批“下一步”是历史。

最新 B2b-6（2026-09-12）：[owner 移除 §13](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#13-b2b-6-普通-app-绑定恢复入口2026-09-12)。普通 App 已提供明确 pending 恢复入口；手机/网关复用同一 resume 完成路径，补离页、切账号和晚到结果隔离。52 项回归、构建及真实 COM11 门禁通过：提交前故障→App 强停/重启→原页面按钮恢复 3,146 ms，原逻辑 ID/intent、0 issue/1 commit，Property/Action 250/267 ms。**普通 pending UI 不再待补；下一小包 D1b 日志背压准入，再混合 WiFi+BLE 联合退休/B1矩阵。** 不将本次明确故障注入扩称全部无线/事务故障通过；以下 B2b-5/4 的“下一步”是历史。

最新 B2b-5 真机进展（2026-09-12）：[owner 移除 §12](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#12-b2b-5-纯-ble-授权生命周期及串口对照真机2026-09-12)。用户确认可清授权后，COM11 专用纯 BLE 完成同账号云端删除、旧 API 410、本地未清时仍可控制、resetAccess 后新身份/新 epoch、旧 Controller 精确 Method 2 Rejected、新控制及重启恢复；COM3 24 次控制与原账户 MQTT 保持，COM12 在线。删除测试 Sketch 的无条件串口日志后，关闭串口 enrollment 7.91 s、Property/Action 241/267 ms。**本工作包主路径通过，不代表全部 B2/发布组合通过。下一步先补普通 App pending 恢复入口及明确可选 Print 日志背压边界，再混合 WiFi+BLE 联合退休/B1矩阵，不扩网关或长循环。** §11 未获清授权许可/未刷板的描述是此前历史，不再是当前阻塞。

最新 B2b-4 部署进展（2026-09-12）：[owner 移除 §11](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md#11-b2b-4-本机部署及非破坏真机准备2026-09-12)。已备份本机 Gateway 库并部署 037/四个编译模块，debug App 保留数据更新；原账号/库存及 COM3 WiFi Property/Action 真机回归通过，COM12 在线。纯 BLE lifecycle 测试 Sketch 编译通过，工具去掉旧按钮匹配并保持精确保护。**尚未刷板/清授权或通过纯 BLE 移除→reset→新认领；下一步先确认 COM11 原网关授权可清或提供空闲板，再专用 BLE 短闭环。** 不重复部署、扩网关/授时或长循环；§9～10 软件证据不冒充物理擦除。

此前 B2a 真机进展（2026-09-12）：[owner 移除真机报告](DEVICE_V2_OWNER_REMOVAL_HARDWARE_REPORT.md)。COM11 专用普通 WiFi 设备已完成真实 App 删除 202→Broker 注销→200、旧 Key 精确 403/DeviceRevoked、MCU 复位不重新上线；COM3 持续 24 次 Property ACK、原账户 MQTT 不重连，COM12 基线保留。此主路径通过，不等于本地根已清除或全部发布组合通过。B1 矩阵余项并列，不扩 Hub/S4。

前批 B2 软件基线：[owner 移除实施合同](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md)。已修普通 WiFi owner DELETE 的 405 断点，提交云端撤销事务并复用持久 Broker/share 收尾，App 区分 202 清理中、200 云端完成与始终独立的本地未确认。删除/worker 16 项、公开 HTTP 接线、相关 PG 回归 41 项及 App 64 项通过；这些是前批软件结果，后续部署/真机见上段。新的纯 BLE 软件路径见最新入口；混合与网关仍在改数据前明确 409。

最新 R4 进展（2026-09-12，同日后续）：[双账号 LAN 真机报告§6](DEVICE_V2_LAN_R4_HARDWARE_REPORT.md#6-第二物理设备隔离与b1普通wifi初步见证)。COM12撤权至exact Retire ACK为1,950 ms，原LAN关闭；同一共享账号中的COM3普通WiFi设备持续完成13次Property ACK和真实State更新，其中撤权结束后5次，原账户MQTT不重连。结合前批重分享/owner恢复，**R4本工作包已收口**，不代表全部无线攻击/发布矩阵完成。COM3的WiFiQuickStart配置副本还通过首次接入、Manifest/State、Property/Action、重启恢复；B1仍缺默认明文/单ZIP及其他平台当前版本矩阵。下一步B1/B2→B3→B4，不再追加R4循环、网关或通知保活。

分享体验记录：见[分享方案§8.14](DEVICE_V2_SHARING_USER_FLOW_AND_R3C_PLAN.md)及[通知方案§12](../../aily-blinker-services/docs/NOTIFICATION_INSTALLATIONS_AND_DELIVERY_PLAN.md)。Q3e定位本机应用UID冻结；Activity重建通知点击、真实双账号接受viewer分享/自动进入设备页、撤销及实际Broker0/1精确失效回执通过。原账号恢复，测试grant已撤销，跨shard开关恢复关闭，installation/delivery保持开启。后台持续收信/进程死亡冷点击仍未通过；通知只是提醒，不再把系统后台兼容性当核心授权链前置。后续R4已执行；其余通知/账号生命周期/QR门禁独立保留，不冒称全部完成。

R3是**账户跨Broker路由中的分享撤权收口**，不是Arduino库全部重构，也不是BLE网关子设备协议。它是原双账号LAN P3实测被跨shard分享409阻断后补的必要依赖。目前不需要重写设备协议或再加账户MQTT。

R3/R4完成后应优先回到普通设备基础能力，**不再自动转向Hub/S4扩展**。WiFi/BLE接入核对可以现在开始；解绑/授时不能因网关高级能力、100次轮换或Wio 24 h发布门禁无限延期。已批准的复杂网关工作保留文档，普通设备先形成能安装、注册、使用、退出和恢复的完整闭环。

按可验收工作包，而非百分比或承诺轮次数量计算：**R3a～R3c软件基线、最小邀请链、R4及B2a普通WiFi移除主路径已收口；当前继续B1矩阵/B2b与B2c边界→B3→B4**。邀请体验的QR/组件选择及后台通知余项独立保留，详见[单设备分享流程](DEVICE_V2_SHARING_USER_FLOW_AND_R3C_PLAN.md)。不代表全部设备安全生命周期已验收。B1/B4是复核与补齐已有实现，不是重新写两套通信库。硬件/共享网络异常可能增加复测，不给没有依据的完成天数。

| 工作包 | 本次核对后的状态 | 退出条件 |
|---|---|---|
| R3a 双端主动失效 | 已完成软件基线 | 原source/target/Presence及本地lane、旧版本/在途操作隔离已有回归 |
| R3b 事务投递与组合故障 | 分享路径软件基线已收口，详见Service§30 | PG主体变更→持久双端投递、在途Action/连续通知/另一账户、原迁移实际工作排空、LocalGrant退休均已补测；完整移动App/设备及迁移部署验收不在本批 |
| R3c 分享入口一致性 | 软件基线已收口，Service§31；部署开关仍关 | 增权保留active主体/设备和normal双端route；库存与减权不受临时路由故障阻断；公开接口/双Broker门禁通过 |
| 邀请体验 | 真实warm/Activity重建点击、接受viewer/进入设备页、撤销与双Broker回执已过；本机后台UID冻结已定位，未解决 | 发布前补后台支持边界/进程冷启动/账号生命周期、组件选择/QR；通知不作为核心授权链前置 |
| R4 原双账号LAN P3 | 本工作包真机收口 | 已含第二物理设备同账号持续读/控隔离、设备exact ACK、原账户MQTT不重连；旧wire无线攻击及全部平台发布另列，不继续循环扩项 |
| B1 接入与统一API | B1a 同 ZIP 七目标编译/ELF 已过；B1b-3 真实 profile 同 ZIP 的 COM11 默认普通 WiFi 登录/控制/重启及租约采样已真机通过；上游干净 CI/时间预算、R4/Wio 当前真机单列 | 三平台普通WiFi、ESP32/R4纯BLE、ESP32 WiFi+BLE；默认示例注册/Manifest/State/Property/Action/重启恢复；Wio BLE保持preview单列 |
| B2 绑定/解除绑定/恢复 | B2a 普通 WiFi、B2b-5 纯 BLE、B2b-6 pending 恢复已过；B2b-13 混合同 ID 换根/旧授权拒绝/双通道重启/联合删除及显式空根已真机通过；首次分配窗口、B2c/故障组合未全完成 | 区分owner解除、成员退出、重配网、本地reset；云端失效、本地停服与Flash擦除分别举证；不把主路径通过扩称另一账号/带分享/全部故障完成 |
| B3 授时最小能力 | WiFi、BLE、Plain LAN、混合和 Hub 南向短真机均已过，最新见授时 §19 | 保留一个可选业务时钟及独立安全时间；只补最终声明平台/已知边界，不重复产品接线 |
| B4 LAN产品化 | 普通ESP32 WiFi有内部候选，非全部平台完成 | R4后完成组合复用、普通产品入口/资源/安装矩阵；未实现的平台明确不支持，不能凭框架声称已支持 |

执行顺序（Q3e修订）：**已验证最小邀请授权链→R4最小撤权闭环→B1/B2优先→B3→B4产品化→再评估Hub/S4与其它扩展**。通知后台兼容性/账号生命周期及分享体验余项保持独立发布门禁，不能当作通过或无限阻塞设备基本功能。基础审计/文档可与软件门禁交叉进行，但同一批不同时新增解绑协议、TimeSync、LAN平台后端。发现安全或实际功能断点立即修正；不为凑轮次继续扩故障矩阵。

迁移范围收敛：R3必须保证migrating/旧route/旧回执/仍在执行的工作不能被错误放行；**永久退役Broker的自动清理优化**可留运维后续，只要未确认任务保持pending且不宣称撤权完成。不以网络失败、TTL或空内存代替证明，不把完整弹性迁移平台当普通设备首次上线的前置。

## 2. WiFi、BLE及WiFi+BLE核对事实

现有公开入口是同一Arduino ZIP内的`Blinker.h`（默认WiFi，`BlinkerWiFi.h`同义）、`BlinkerBLE.h`、`BlinkerWiFiBLE.h`；Property/Action/Event、begin/run/report共用。标准示例在唯一`examples`目录：WiFiQuickStart、BleQuickStart、WiFiBleQuickStart、CloudSmokeTest。不要重新引入按MCU/传输分别安装ZIP，或让用户管理composition/slot/frame。

- 普通WiFi：`arduino/internal/WiFiProduct.h`的DeviceKey stack由ESP32/R4/Wio平台绑定复用；手动网络和长期DeviceKey→challenge/session→MQTT。代码存在且有历史真机证据，本批未重新刷三种板，不能将历史通过写成本次最新版本通过。
- 纯BLE：统一BleProduct/BBP/Direct安全，ESP32和R4有核心真机证据；Wio BLE源码存在但仍显式preview。最新Wio发布报告已有30 min工程长稳/32次GATT门禁，24 h认证长稳未完成；不阻断其他两平台普通产品。
- ESP32 WiFi+BLE：当前公开begin只收组件，配网由App驱动；默认BLE WiFiProv，可选SoftAP。不是所有WiFi产品都自动有BLE，也不是当前已经提供带Key/SSID的WiFi+BLE公开重载。NimBLE/WiFiProv、network-only重配网和BLE-first/cloud fallback已有历史证据，当前SDK、分区、heap与双通道要按最后有效固件复核。
- R4本阶段仍只提供普通WiFi和普通BLE，不新增WiFi+BLE组合；Wio的模组固件/存储/长稳限制单列，不能强行用ESP32 SDK实现覆盖。

B1以当前4个标准示例为准，记录源码/ZIP版本、SDK/板型、资源、水位、登录/订阅/Manifest/State、Property与Action执行ACK、掉线/重启恢复。移除无消费者候选要先核对发布脚本、测试和产品闭包，不能只按目录名删除。

## 3. 绑定与解除绑定：真实缺口及最小合同

**已修并完成普通WiFi真机主路径的断点**：App `device-settings.ts::removeOwnedDevice`→`device-v2-management.service.ts::deleteDeviceV2` 的 `DELETE /api/v2/devices/:id` 原来返回 405。B2a 已接入 Service owner 权威事务与原持久 worker，并补公开 HTTP 与真实 App/MCU 202→200 验证；细节见[owner 移除合同](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md)。B2b-5 已完成纯 BLE 同账号释放/物理 reset/新认领/旧授权拒绝及重启主路径；B2b-13 已完成混合同 ID 换根、旧授权拒绝、双通道重启和联合移除主路径。首次分配窗口、网关/另一账号及完整发布组合仍未完成。成员的 leaveShare 是另一条已实现路径，不能替代 owner 解除。

设备的`Blinker.resetAccess()`只在支持产品清本地授权根，保留DeviceInstanceId/网络等；`resetNetwork()`只清网络。它们均不是服务器所有权转移协议。普通WiFi把长期Key写在Sketch时，擦本地存储后该Key可能仍来自Sketch，必须依靠服务器作废旧Key，而不是仅删NVS。

B2 按以下边界分批实现，不增加旧 OwnershipRecord/saga：

1. **成员退出/owner撤销分享**：只撤匹配account＋device权利，复用R3投递、原BLE共享凭据轮换及LocalGrant Retire。owner与其他设备保留。
2. **owner解除/删除设备**：服务器原子标记该逻辑设备不再授权、作废DeviceKey/session和相关share/direct/presence/gateway引用，复用已有持久撤销机制；返回云端生效与本地待处理的不同事实。不是立即硬删所有记录让重试失去证据，也不是仅前端移除卡片。
3. **离线BLE/LAN设备**：服务器不能凭HTTP删除隔空擦掉旧本地权限。需要已认证连接下交付原控制撤销/对应ACK，或用户现场物理reset；前端不得将“云端移除”显示成“本地已清除”。附近原owner与新owner重新认领、旧controller无法恢复必须测。
4. **重新配网**仅改SSID/密码，保留逻辑设备和页面；**刷新Key**明确轮换凭据；**重新认领/转交**才改变所有权。MAC不是逻辑身份，不凭相同MAC恢复旧账户权限。

优先顺序：协议状态/接口合同→Service事务与持久收尾→App入口消费→设备必要的已有控制接线→在线/离线/恢复真机。owner 可以独立放弃云端归属；纯 BLE 没有设备 MQTT 且无分享待办时 DELETE 可以直接 200，但始终 `localAccess=not_confirmed`，不得掩盖尚未发生的本地清除。本轮软件收尾后先短真机，不继续扩软件循环矩阵。

## 4. 授时：WiFi与BLE应共用什么

### 4.1 当前已接可选业务授时，不替换安全时钟

`IClock`已有`monotonicMillis()`与`unixTime()`分离。普通WiFi stack默认、纯BLE product都使用`ArduinoRuntime.h::ArduinoClock`，其中`unixTime()`固定返回false。`Client::setMonotonicClock()`是调度/限流用时钟，不等于用户已经能取年月日。

普通产品已有可选 `enableTime()` / `time(uint64_t&)`；WiFi、BLE、Plain LAN、混合和 Hub 南向短闭环见业务授时 §8～19。业务服务不直接替换上述安全 `IClock`。ESP32 EdgeHub 的 SNTP 安全时间准入仍独立，不能把普通手机/业务时间或 NTP 完成称为密码学认证时间。

### 4.2 B3最小设计与当前状态

最新实施及唯一细节合同见[业务授时方案](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md)。B3 核心、wire、产品接线及上述短闭环已实现；下方设计原则继续有效，当前只做核心稳定版必要回归。

一个可选、与传输无关的业务 WallClock，持有 UTC 锚点、单调时间锚点和同步质量/过期状态。平台只提供 tick/可选 RTC，WiFi/NTP、App 和网关只是不同时间样本来源，不分别写一套日期/租约逻辑。业务时区在显示层转换，设备保存 UTC；无有效样本时显式未同步，不把 0 当当前时间。

**B1b-3 源码复核后的 B3-0 修正：不能将上述业务 WallClock 直接替换所有产品的 `IClock`。**
`GatewayChildSession::connect()`、`GatewayChildRouteBridge::admittedAccessState()` 已使用 `IClock::unixTime()`
检查网关凭据有效期；此前“继续实现 IClock”的宽泛表述会让手机/业务校时意外影响安全准入。
B3 首包复用 tick 输入，但业务 UTC 查询/样本更新与已有安全时钟注入分离，默认不改安全消费者。
若以后复用同一计时算法，必须显式区分来源准入和安全用途，不能只凭 `synchronized=true` 升格为安全时间。
上述 API 与关联通道已经实现；不再重做最小合同或给每个平台引入一套 NTP 库。当前未覆盖的平台/边界仍据最终发布范围补证。

| 设备/通道 | 初期授时来源 | 无连接时 |
|---|---|---|
| 普通WiFi/WiFi+BLE | 优先复用服务器已鉴权通道的一次时间响应；有合适UDP/SDK时NTP可作可选provider，不强迫所有平台依赖NTP库 | 本次开机用单调时钟外推，超出允许年龄/误差标stale |
| 纯BLE、手机直连 | App在已认证BBP会话后提供UTC样本，带本次请求关联，设备记录往返时间；不是广播携带时间或建立BLE连接就自动授时 | 断连后同上；无RTC断电后未同步，不假设重启知道日期 |
| BLE子设备经网关 | 网关以自身有效时间经已授权子设备会话提供同一TimeSync样本 | 网关时间无效不冒充同步成功；不要求手机一直在附近 |
| 有电池RTC的板 | 可选RTC provider启动提供候选样本，再校正 | 区分RTC来源和最近校正质量，不强制所有设备具备RTC |

既有 TimeSync request/response、来源权限、关联/回放隔离和可选 capability 继续沿冻结合同；不借 Property 写保留字段冒充系统时钟，也不把时间字段加入每条 State。公开 API 和例程以当前源码及授时方案为准。

**安全租约、重试、发送限流继续使用单调时间；不能被手机时间回拨续命。** 手机/明文NTP的业务样本不授予新的权限、不延长LocalGrant/设备Key有效性，系统证书/签名安全时钟的需求独立处理。需测32位millis回绕、漂移/回拨、重复响应、离线/重启、授时源优先级及RAM/Flash；不在本批扩定时任务、日历、History/Bulk。

## 5. WebSocket：已有核心，但不能称为完善

已实现通用`ILocalMessageLink`、有界LocalAccess transport、明文/可选加密会话、鉴权/BBP Hello/Manifest/State及队列/ACK恢复；ESP32的`Esp32LocalWebSocket`复用SDK HTTPD并有独立mDNS发现。App已有独立discovery/directory/connector/session与前台预热、账户/网络代际清理，不需要增加第二条MQTT。

当前边界：

- `Esp32WiFiProduct.h`在`BLINKER_INTERNAL_LAN_WEBSOCKET`下才注入`Esp32WifiLanExtension`。这是普通ESP32 WiFi、单本地caller、400 B设备profile的内部候选；监听/发现不代表鉴权ready。
- 当前`Esp32WiFiBLEProduct.h`/`Esp32EdgeHubProduct.h`尚未看到该正式LAN extension接线；R4/Wio WiFi产品只有普通WiFi stack，未提供对应LAN listen/discovery实现。接口可扩展不等于所有产品都已连接它。
- App三个environment的`deviceV2LanEnabled`仍false；默认明文指将来启用LAN时的记录模式，不表示当前所有App自动开启LAN。
- 原双账号分享撤权/设备ACK/页面提前关闭、重分享恢复、第二设备隔离已在R4通过。网络切换/全平台与规模发布矩阵仍需独立核对；不能用这组结果宣称所有路径无缝，也不能用单个ready时长推出体验通过。

B4继续采用extension注入，ordinary WiFi/WiFi+BLE/Hub共享Client、组件和状态；网关只额外注入子设备路由。R4/Wio新增平台listen/发现适配前先做socket/动态内存/背压准入，不能为了复用强制引入ESP32 HTTPD。为每个声明支持的产品补同一ZIP安装编译与真机矩阵；未完成的组合明确preview/unsupported，不硬开宏。

## 6. 本批实际推进与证据边界

新增正式Gateway `test:app-access-delivery`，依赖现有构建产物及隔离PG；抽取复用原Broker子进程夹具，删除原HTTP测试内的重复进程管理函数。新用例实际运行PG权威ACL/placement、生产投递worker、原签名中继、两个Broker原HTTP/runtime和各自SQLite；App登录和物理设备仍是fixture，未连接现场手机/板。

首组7项（含父项）通过：真实路由/Manifest/Presence、事务回滚、一端失败/源取消丢失/worker恢复、新分享与旧失效、应用后结果丢失重投、实际目标进程终止/重启，以及migrating/freeze拒绝。原LocalControl同PG构建签发/退休，两个实际Broker成功后仍待Retire；错MAC拒绝，exact ACK后才恢复本地签发，owner授权保留。ACK在原repository入口由协议夹具送入，不是MCU无线ACK，也不冒称本批测完LocalControl完整MQTT运输链。

相关回归85项（含父项）通过：原双Broker HTTP场景、PG持久投递、投递worker和LocalAccess签发/退休。以上是本批实际运行范围，不把历史Broker/App全量测试计为本批通过，也不将父项与子项视为独立的85种故障。

证据：ignored `build/lan-r3-baseline-closure-20260911/`，运行说明见该目录`README.md`。复用了已编译源码；无常驻部署/业务库迁移、真实账号/设备删除、Key/Flash/网络操作、暂存/提交/推送。现场最后SSID为mostfun2026，真机前重新核对设备/手机同网。

## 7. R3b主体生命周期收口（后续批次，同日）

Service migration 032将tenant状态/设备状态改变关联到现有active share的version递增，复用031双端投递和027本地退休；没有新建队列、消息类型或主体权限系统。停用期间fresh权限拒绝；恢复后仍需新代际观察/本地授权，不复活旧会话。账号状态/last-active重复写入不触发新版本，已撤销share不反复重投；原账号注销事务和回滚均已覆盖。ownedDevice查询补owner tenant active/一致性，阻断停用owner继续创建新分享。

真实PG＋两个Broker串联**9/9**、持久投递PG专项**14/14**、相关回归**86/86**通过；Gateway正式`test:device-v2`顺序清单通过，最后并行组**99/99**；类型检查通过。计数含父项且相互重叠，不累计。证据`build/lan-r3-subject-lifecycle-20260911/`。

组合中另一账号在同一个真实Source Broker上有独立原App lane，停用读者时Action在途、取消丢失/目标失败，State/Event/Telemetry只向仍授权的账号转发，原本地采样保留；恢复后实际再次收到数据，重传Action未增加物理执行次数。第二逻辑设备的隔离由真实PG专项与已有Broker目标隔离门禁覆盖，**不是本批两块物理设备同时运行**。原migration admission/OperationWork/wait函数实测等待已派发工作，coordinator的freeze→verify→route CAS→cleanup顺序已代码核对，未实际搬迁常驻Broker。设备ACK仍为协议夹具，不替代R4。

这里完成的是**分享主体生命周期**，不是owner全部连接/资源删除，也不是共享BLE长期凭据已轮换或离线设备已擦除；这些仍在B2与原退休合同中验收。保持公开跨shard分享与App LAN关闭，下一批R3c必须先让公开入口使用相同权威事务，再替换串联测试中的跨shard SQL更新夹具。避免继续扩充一个包办所有协议/设备的巨大测试框架。

## 8. R3c软件基线收口与分享体验修订（同日后续）

按[分享流程合同](DEVICE_V2_SHARING_USER_FLOW_AND_R3C_PLAN.md)完成公开received/accept/update/revoke/leave一致性：关系库存不被路由故障隐藏，正向增权保留严格准入，撤销/缩权/本人退出不受故障阻断；相同PATCH和接受重放不制造新权限。原PG/两Broker组合通过真实公开分享HTTP重新授权，删除跨shard直接更新grant表的夹具。App保留pending和当前权限状态，默认viewer、去伪9人限制、邀请重试幂等与接受防重入；不是增加一套设备成员列表或更改Arduino API。

新增/相关验证20项公开分享与准入、9项真实双Broker组合、77项相关回归；Gateway正式清单通过（最后组99项）；App46项专项/关联测试及两端类型检查通过。计数含父项和重叠不累计，物理MCU/App登录仍无本批证据。记录在`build/lan-r3c-sharing-20260911/`及Service§31/App LAN§46。

R3软件基线至此完成；尚需1个邀请体验包→R4双账号真实LAN/设备退休→B1/B2→B3→B4。链接/二维码/预览/登录返回、成员安全展示名和逐组件选择器尚未实现；纯BLE成员有限授权和owner解绑仍不能宣称可用。开关保持关闭，无本批部署/暂存/提交/推送，不重刷设备。
