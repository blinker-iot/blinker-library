# Device V2 核心稳定版：C0～C4 历史进度与证据

入口更新：2026-09-22。范围：设备库、App、Service。**后续执行优先级只维护在[新进度与执行计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md) §6；新增 BLE 优化/定时/自动化见[共同合同](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md)。本文件保留 9 月 18 日前 C0～C4/§25 的证据，不继续追加批次，也不按历史“下一步”自动扩项。**

**截至 §25：C3 冻结基础矩阵已收口；真实失效 BLE 候选对照已完成，Manifest 缓存协商及损坏缓存恢复已实现并过同设备真机。9 月 21 日必要代码/插件已推送，但依赖及发布配置仍有缺口。C3 不覆盖用户定时/自动化；新 P1/T0～T2/A1 独立实施，不重开 C3 或恢复 C4 整组维护。这不是生产发布或无缝 SLA 已完成。**

## 1. 本阶段交付目标

用户用一个 Arduino ZIP 和标准示例即可接入，完成注册/绑定、能力读取、属性和按钮控制、状态上报、断线/重启恢复、必要的解绑与重配网。先修已有断点，再统一版本与发布配置；不以全部无缝通信完成作为基础版本前提。

首期声明范围：ESP32 WiFi/BLE/WiFi+BLE、UNO R4 WiFi/BLE、Wio Terminal WiFi。Wio BLE、Edge Hub/子设备网关、未正式接线的 LAN 维持 preview/internal，不删除仍有消费者的实现，也不默认开放。ESP32 BLE/WiFiProv 所需 SDK 配置必须明确，不宣称任意 stock SDK 都可用。普通 WiFi 默认明文、可选 TLS 的既有政策不变。

基本恢复（退出可取消、重启可继续、断网能重连、权限拒绝准确）属于当前范围；新 caller 自动抢占、多手机零等待切换、所有产品的 LAN 扩展属于后续。已暴露的越权/丢状态等缺陷不能因 preview 被忽略，修复与扩大功能须分开。

## 2. 当前状态总表

| 项目 | 已有证据 | 核心稳定版剩余 |
| --- | --- | --- |
| 库/API/安装 B1 | 单 ZIP、四个标准示例；七目标基础编译、六种实物模式；HTTP 维护补丁再过 S3/R4/Wio WiFi 安装/依赖/ELF，静态 RAM 均不增加 | 干净 SDK/CI、生产 profile 与发布归档属于 C4；不声称维护补丁后又作七目标冷编译，R4 上传握手现象独立保留 |
| WiFi/BBP/2 | DeviceKey 登录、MQTT、Manifest/State、Property/Action、ACK；三平台基础恢复；S3 TLS 服务组恢复、Wio 标准示例续签期间停服/自然恢复已真机 | 不外推所有故障；普通 MCU reset/测试服务组恢复不等于整板断电或宿主 USB 故障，不新写协议 |
| 普通 BLE | ESP32 绑定/恢复/旧授权拒绝近期通过；当前 R4 新认领、标准固件/MCU/App 三轮控制与精确 State 已过 | 不外推任意 SDK、长稳或授时全矩阵；首次延迟后置，Wio preview 不挡住其他平台 |
| ESP32 WiFi+BLE | WiFiProv、双通道、同 ID 重配网；三个首次分配中断窗口、journal 精确退休及清理失败重进/新认领已真机 | 最终版本平台回归；不把有限故障样本外推为任意故障恢复 |
| 绑定/解绑 B2 | 普通 WiFi、纯 BLE、混合主路径、首次分配恢复和明确移除→重新认领通过；云端撤销/手机记录清理/MCU reset 分别确认 | 带分享/另一账号按发布路径补证，不顺带扩网关 B2c |
| 授时 B3 | WiFi、普通 BLE、Plain LAN、混合与 Hub 南向短闭环通过 | 最终声明平台的必要回归、已有授权/来源边界；不重复接线 |
| 日志/限流/采样 | 基础日志/限流；同 ZIP S3 CloudSmoke 已验证前后台/退出停发、自然到期与 Broker 恢复（§12） | 默认静默；通用 Print 不承诺非阻塞；不把发送计数等价为采集回调次数，不外推 R4/Wio |
| 共享云控与 LAN B4-1 | 实际 ACL、设备 exact Retire ACK、页面原 MQTT 控制、有限 pending/取消/Observer 恢复；C2 增权与同 caller 自然到期恢复已通过 | 最终版本回归及一致的部署开关；LAN 暂不扩所有组合，不承诺无缝 SLA |
| 发布配置 | 已有本机联调配置/临时开关及测试制品 | 三端版本与开关可复现；共享跨片必须 Gateway＋源/目标 Broker 一致；不能靠临时钩子作为部署方式 |

证据索引：[B1 安装矩阵](DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md)、[混合生命周期](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)、[授时 §19](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#19-b3-4c-4b-南向真机与本地发送背压修正2026-09-13)、[LAN §70.5](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#705-b4-1-实际-acl延迟退休与页面原-mqtt-闭环2026-09-13)。历史通过不等于当前所有平台/任意 SDK 全部通过，不给无依据完成百分比。

## 3. 固定执行顺序及退出条件

| 包 | 工作与退出条件 | 状态 |
| --- | --- | --- |
| C0 范围/文档收口 | 一个当前表；旧入口链接本页；消除已过 B3 仍写未接线的冲突 | 已完成本批文档收口 |
| C1 首次配网恢复 | 同请求幂等分配、精确检查点恢复；新增记录与正常移除/重认领闭合 | 冻结范围已通过：三个进程中断窗口＋C1-d 正常/本机清理失败两条真机链；172/172、类型/lint/APK 已过。明确排除项见 §9 |
| C2 已知恢复断点 | 定点核对增权就绪与普通 App 重启/到期；保持原 MQTT、明确错误/有限等待，不引入多 caller 抢占协议 | 已按 §10 收口：增权、自然到期/原页面恢复、精确退休 ACK 真机；普通重启/退出沿用 C1 证据 |
| C3 最终基础矩阵 | 同一实现与可追溯 profile/ZIP/依赖版本；ESP32 受影响回归、R4 WiFi/BLE、Wio WiFi；接入→控制/状态→断线/重启→退出→必要解绑重配，分别记录资源 | **冻结范围已完成，见 §23 / C3 §12.6**：既有七目标/六种模式＋HTTP 小补丁三 WiFi 目标补编译，已定位并闭合续签停服故障；不是全部可选 API、长稳或生产发布 |
| C4 可复现部署/候选发布 | 三端匹配版本/迁移/功能开关及安装方式；干净 SDK/CI、必要源码/示例清单与 preview 边界；普通路径不依赖内部 probe | **服务器后续稳定维护暂停，见 §19**；已完成项/未验收风险保留，不以整组升级工具完成作为本轮普通 WiFi/BLE 真机前置 |

不把普通 WiFi 多做一条 DELETE 等价为网关拓扑解除。若本阶段不发布网关，其连接窗口、拓扑/多 child 隔离及生产 protected-NVS/eFuse 继续单列，不能删掉风险说明后放行。

## 4. C1 最小实施合同

已核对并修正的断点：原 `Esp32ProvisionPage.createDeviceIdentity()` 以页面内存 `allocationIdempotencyKey` 发 CREATE→REVEAL，然后 `WiFiProvAdmin.install()` 才存 pending Admin。崩溃在 CREATE 成功与 pending 之间，会遗失请求身份；Service 原 `createDevice` 已按 owner＋idempotency key 持久重放并拒绝已删除设备。

本包只补 App 的小型首次分配记录：

1. 从已连接 WiFiProv 的 GetInfo 取实例；MAC/广播名不充当业务身份。记录按 authority＋account＋instance 隔离。
2. CREATE 前保存稳定随机 requestId 与规范化原请求体；无持久化成功就无网络写入。重进使用同请求体/幂等键，不因改显示名改请求。
3. 成功返回的逻辑 ID/Key 上下文先落盘，再执行 REVEAL 和原 pending Admin/Bootstrap；不落盘 DeviceKey、WiFi 密码或另存 Admin secret。
4. CREATE 结果未知用同请求重放；有上下文后直接 REVEAL，不自动旋转 Key。当前版本/locator 不一致或已删除，停止且保留恢复证据，不能新建绕过。
5. 原 pending/active Admin 路径继续唯一负责实际接入根。**不能在 Bootstrap ACK 后立即删除分配记录**：它可能早于首次 WiFi/云端 instance 关联。保留精确逻辑 ID 至原 resolve-instance 确认同一关联；进入控制页前仅 best-effort 清理，查不到/清理失败保留记录、不阻塞已配置设备。不为清理等待云端或增加轮询。账号/关闭检查在每个异步边界；并发准备拒绝，不新增无限队列。
6. 只处理首次 WiFiProv 分配；普通手动建号、Cloud-only 已有 Key 轮换恢复、Cloud-only Install 成功但 ACK 丢失后的跨进程恢复、清 App 数据/换手机丢本地记录、跨安装并发或旧 APK 降级不冒称由本包解决。不新增 Service migration/MCU wire/权限槽/Sketch 参数。
7. 明确 owner DELETE 成功后，只退休本账号/authority、该逻辑 ID 唯一匹配的 allocation journal，以及与之同 instance/source 的 WiFiProv Admin。本机写失败与云端成功分开显示；保留 journal 供下一次明确恢复。仅当 GetInfo 为空根、原 owner GET 明确返回同 ID `deleted`、用户确认，才重放原 DELETE 并重试精确清理后新建。404、离线、未绑定 context、当前账号不符均不得自动换请求；原同 ID 重配网不走此新认领分支。

验收：持久化失败零 CREATE；CREATE 响应丢失、结果落盘失败、REVEAL 失败后重建，逻辑 ID 不重复；请求体固定；账号/服务地址/实例隔离；当前 Key 变化拒绝；迟到结果不下发设备；原 pending 恢复/网络重配/混合旧根保护不退化。先软件，后匹配 APK＋已确认测试板短真机，不使用伪造 ACK 充当物理完成。

## 5. 明确后置

无缝/多 caller 抢占、新 LAN 平台/全组合、多子设备网关扩展、ESP-NOW/Zigbee/BLE Mesh、History/Bulk/高频图表、页面编辑器增强、Wio BLE 长稳与网关生产存储。本阶段既不继续展开，也不删除有消费者的实现；无用重复分支可在作用范围和回归确认后移除。每包更新本页状态，避免再次以新的“下一步”覆盖核心目标。

## 6. C0/C1 本批实施结果与下一步（2026-09-14）

**本节保留最初软件批次的历史证据；当前执行以 §3、§11 及 C3 报告为准。下文“确认后删除 journal”“未安装/真机”已由真机发现和后续修订覆盖，不是当前操作指令。**

职责保持三层：页面负责生命周期及原配网交互；`WiFiProvAllocation` 负责 CREATE/REVEAL 顺序与公开上下文核对；`CapacitorWiFiProvAllocationStore` 负责现有 OS secure storage 上的账号/authority/实例隔离。原 `WiFiProvAdmin` 仍唯一负责 pending/Bootstrap/receipt/active，不创建第二套安装事务。只在 journal 存储操作期间拒绝并发，不在网络等待期间持锁或新增重试队列。

新 journal version 1 只包含固定请求 ID、原 name 和成功后的逻辑 ID/版本/locator；`deviceType=diy` 是固定请求合同。页内首次分配幂等 ID 已被持久记录取代，旧内存字段收窄并改名为 Cloud-only `rotationIdempotencyKey`，不混用两种操作。Bootstrap/resume 确认后先记录页内已安装状态再删除 journal，删除失败重试只清记录/继续网络，不再重复安装。

本批确定性回归 **8 文件 116/116**，App/spec TypeScript、五个变更 TS 文件 eslint、Angular local build、Capacitor copy、Android `assembleDebug` 均通过。新增 journal 单测 18 项；页面增加响应丢失、pending 写失败、原 pending 恢复和清理失败场景。覆盖存储先于 CREATE、同请求重放、恢复沿用原逻辑 ID、原请求体冻结、Key context 改变拒绝、账号/实例隔离和迟到结果阻断。

证据目录：`blinker-app/build/core-stability-20260914/`（ignored）中的 `app.xml`、构建日志及 `apk.json`。匹配 APK SHA-256：`c881d4a70672e5ac90b2497c5f87c4bb7bcb471b529571cb38aeb694ffb62757`。它是本机 debug 测试制品、保持 `lanEnabled=false`，不是 C4 发布包。**未安装 APK，未操作 ADB/开发板，未新增 Service/设备生产代码或部署/迁移，未暂存/提交/推送。** 本批未重跑 Service PG 或 MCU 真机，不将历史结果混入 116 项。

**下一步仍是 C1 短真机，不跳到无缝扩展：**

1. 只读核对手机/三块板的实际身份、现有固件/根状态及当前网络；仅选已确认的专用空根 WiFiProv 测试板，保留见证板和既有授权，不因旧端口记录自动 reset。
2. 保留数据安装匹配 APK；在真实 CREATE 已提交、pending 尚未保存的边界中断配网页面/进程。通过正式 API 和设备 GetInfo 核验重进沿用同一 requestId/逻辑 ID，而非再建设备；不能伪造业务回执代替。
3. 继续原 Admin 登记/精确 Bootstrap、WiFi 登录/MQTT、自动 BLE 页面 Property/Action/State；短普通重启和网络重试后不重复分配，确认 journal 清理。临时诊断中断点用后撤回，不进入发布逻辑。
4. C1 通过后进入 C2：只复核已知增权 ready 样本、普通 App 重启/退出及到期恢复。随后 C3 同版本平台矩阵与 C4 配置收口；不会以新 caller/跨平台 LAN 接线代替核心稳定性验收。

上述软件回归采用真实页面/协调器和存储插件 mock；**尚不证明 OS 杀进程、无线丢包或跨安装丢资料可恢复**。若真机暴露新的基础断点，先修同一职责模块并补回归，再继续矩阵，不增加新的通道协议。

## 7. C1 三个真实中断窗口与双通道恢复（2026-09-14）

已保留数据安装 App，通过原页面/原 API/真实 WiFiProv Security1 操作专用 COM11 XIAO；COM3/COM12 作为原 MQTT 控制见证，不清授权、不改固件。详见[首次配网恢复真机报告](DEVICE_V2_FIRST_PROVISIONING_RECOVERY_HARDWARE_REPORT.md)。

- CREATE 返回 201、context 未保存即强制终止 App：重启用原请求返回 200、同 ID；随后 context 已保存、pending Admin 未写再次终止：再次重启不发 CREATE，完成 Bootstrap/WiFi/双通道控制。这两窗采用初版 APK，最终版保留同分配路径并经软件回归，不写成最终 APK 重跑了所有窗口。
- Bootstrap 已真实 ACK、原生 active Admin 已保存、WiFi 尚未发送时终止：发现 Service 尚无首次 session/instance 关联，且本机确有多个历史 WiFiProv 检查点。最初“无关联就找唯一历史记录”的候选真实失败，不能按 MAC/名字或任取一个解决。
- 最终版改为保留 journal 并传入精确 ID，由原 Admin 协调器继续核对 instance/epoch/当前 Key context/fingerprint。最后一窗真机恢复 **零 CREATE、零重复 Bootstrap**；WiFi/MQTT 上线并自动进入 BLE 控制页，Property/Action/State 通过。普通 MCU 重启和 App 冷启动各复测一次，COM3/COM12 控制均通过。

最终 **9 文件 131/131**、App/spec 类型、五文件 lint、Angular/Android debug 构建通过；APK `58e745219382b5d450ef00846f1cc36b4f3ae4b51fba60d6f46718d715b93a1a` 已安装。`lanEnabled=false`，无 Service 生产代码/部署/迁移或新 MCU wire。本批固件由现有库/profile 构建，未改设备生产实现；当前只是本机测试制品，不是统一发布包。

**真实结束状态：** 最终设备唯一且可控制，active Admin/epoch 1/Key version 1；页面已退到首页，本地控制连接 0、账户 MQTT ready、测试拦截器已释放。进入页面时服务器关联尚未就绪，journal 因此保留；之后只读核实服务器已关联同一 ID，没有用测试工具删除记录来冒充自动回收通过。

## 8. 下一步固定为 C1-d：分配记录退休，而非无缝扩展

**本节为历史发现和验收计划，已按 §9 完成冻结范围；C2 也已按 §10 收口，当前执行以 §3、§11 及 C3 报告为准。不再次按本节删设备或 reset。**

本批生命周期审查发现：原 owner 移除只确认服务器删除，未接本次新增 allocation journal 的本地退休。保留记录虽然不影响已配置设备的普通控制，但同实例后续 reset→重新认领可能回到已删除的 CREATE 请求，形成可用性断点。因此 **C1 还不能整体关闭，也不进入候选发布**。这是已识别的实现尾项，不以“测试缓存”忽略。

下一批按以下顺序收口：

1. 先补确定性回归：关联未就绪保留；同一设备关联确认后清理；明确 owner 移除后的精确退休；删除响应未知/账号切换/不同实例/不同逻辑 ID 不误清；删除发生在另一安装时须原服务确认与用户明确操作，不能把 404/离线/解析失败当重建许可。
2. 在原应用生命周期协调层接线，存储层只做 scoped 精确读写；HTTP 管理客户端保持传输职责，页面不直接遍历 secure-storage。不新增定时清缓存、批量删历史 Admin、第二套配网事务或自动换 Key。未完成 CREATE 与已完成并主动移除必须区分。
3. 同一专用 COM11 用原设置移除→明确 resetAccess→原首次配网重新认领，验证生成新的请求/ID而不重放已删除请求；保留原新建中断恢复/同 ID 重配网保护。涉及删除只作用于当时确认的测试 ID，COM3/COM12 继续见证。
4. C1-d 软件＋必要真机通过后进入 C2 剩余的增权 ready/自然到期定点测试，再做 C3 同版本 ESP32/R4/Wio 基础矩阵与 C4 部署配置。未测坏密码/AP 故障、Cloud-only Install ACK 丢失、清 App 数据/换手机、全部平台和 SDK 不纳入本批通过。

本批未暂存、提交或推送；用户已有工作区变更保留。为构造首次配网，按确认的物理实例退休了 COM11 的四个历史/临时逻辑设备并显式 resetAccess，最终保留新测试设备在线；删除边界及不可简单回滚说明见真机报告。

## 9. C1-d 完成与 C2 定点验收入口（2026-09-14）

**本节 C2 计划已按 §10 完成冻结范围；当前下一步为 C3。不得再次据下方历史入口删设备、配网或扩新 caller。**

新增的 `WiFiProvRetirement` 是无 Angular/Capacitor 运行依赖的小型流程协调器：接收存储/凭据/管理接口及账号检查，复用原 DELETE/GET/原生存储，不增加服务事务、协议、定时清理或自动换 Key。`CapacitorWiFiProvAllocationStore.findDevice()` 只读当前账号/authority 的 journal，唯一匹配后仍经 exact complete 删除；存储原并发拒绝共用一个实现。原设置页和原配网页面共用退休流程；HTTP 管理客户端不读取本地存储。

清理先删与 journal 精确匹配的本机 WiFiProv Admin，后删 journal；中途失败保留 journal，之后即使 Admin 已不存在也可重试。不是批量清凭据，更不是 MCU reset。无 context 的未完成 CREATE 保留原请求；不能把服务查不到设备或账号切换解释为自动重新建号的许可。

**验收已完成：**

- **11 文件 172/172**、App/spec TypeScript、十个变更 TS 文件 ESLint、Angular/Android debug 构建通过，APK `659a10274e979de270de708dc28a9d695ddf0ee6409ff9385149824870a290c3` 已保留数据安装，LAN 默认仍关。
- COM11 原设置 DELETE 202→真实 native journal/Admin 均消失→明确 `resetAccess` 空根→新 request/新逻辑 ID→原 WiFiProv Bootstrap→WiFi/MQTT/自动 BLE 页面完成。第二轮只注入一次本机 journal 删除失败：真实 DELETE 后 Admin 已清但 journal 保留；App 强停/重启后由原 GET `deleted`、可见确认框、原 DELETE 200 精确重放完成清理，再新建/Bootstrap/双通道控制。
- 两轮各有初始、普通 MCU 重启、App 冷启动三次 BLE Property/Action 与 MQTT 控制/State；退出连接 0，COM3/COM12 前后控制通过。最终专用设备名 `C1d Retry WiFiBLE COM11`，ID `device_1c26af38-eed8-48f6-9ee2-d106e22d4d00`，Key version 1/Admin epoch 1；旧两个测试 ID 云端 deleted、本机精确退休。过程/资源和未测边界见[真机报告 §6](DEVICE_V2_FIRST_PROVISIONING_RECOVERY_HARDWARE_REPORT.md#6-c1-d明确移除退休重认领及清理失败恢复2026-09-14)。

这关闭的是本次已识别的首次分配恢复/明确退休缺口，不是所有 owner 生命周期组合。删除发生在另一安装的状态分支已有确定性回归，本次真机用“真实云端删除＋本机写失败＋进程重启”到达同样状态，**未另用第二手机操作删除**。CREATE 成功但 context 丢失且同时又在另一安装删除、App 清数据/换手机、Cloud-only ACK 丢失、任意无线故障仍不得自动重建，保留明确错误；不从本批外推通过。

**下一批 C2 的顺序和停止条件：**

1. 先核对原共享测试 fixture/账号、当前权限版本、原 Gateway＋两端 Broker 开关及已安装 APK；只有实际目标与账号确认后才操作。不要直接运行历史 `fault-preload`/部署脚本或再次清 COM11 授权。当前普通生产默认 LAN 仍关，不因测试永久启用。
2. 对 [LAN §70.5](DEVICE_V2_LAN_WEBSOCKET_DISCOVERY_IMPLEMENTATION_PLAN.md#705-b4-1-实际-acl延迟退休与页面原-mqtt-闭环2026-09-13) 的“增权后 8 秒观察窗口内未就绪”样本，记录原权限失效→StateInterest/Manifest/presence→页面可控时间；按产品原有有界预算等待，不能靠扩大超时、自动重发未知 Action 或重建每设备 App MQTT 达成通过。
3. 同一 caller/scope 自然到期：核验过期后停止转发/采样，原通道和原申请/退休流程能有限恢复或准确拒绝；未过期的其他设备/账号不能被清槽。先软件再一组必要真机，不进入新 caller 抢占、容量扩张、跨平台 LAN/网关接线或 100 轮循环。
4. 两点通过后转 C3 同版本平台矩阵和 C4 配置。若暴露核心断点，只修改对应职责层并补定点回归；不再用专项历史“下一步”替代本表。

本批仅 App 生产源码和三端说明更新，未改设备生产源码/Service 实现、未部署/迁移、未暂存/提交/推送。测试确有正常 API 删除/创建及两次 COM11 `resetAccess`，不是只读；设备应用固件未重刷，见证板未重置。

## 10. C2 已知恢复断点收口；下一步 C3（2026-09-14）

### 10.1 修正与模块边界

先用隔离 PostgreSQL 复现：原仓库只退休同 caller 的不同 scope；相同 scope 在服务器自然到期后直接签新授权，而 MCU 单槽仍可能保留旧授权/防重放 tombstone，导致新连接容量拒绝。服务器到期时间不是 MCU 最后一次首次接纳时间，不能删掉设备 tombstone、扩大容量或重连续期来规避。

仅修改 Service `local-access-repository` 原无可复用授权分支：未过期同 scope 仍复用原 ID/原截止；到期或变化的 self 意图复用同一个原子退休 outbox，增加准确的 `caller_lease_expired` 审计原因。过滤仍限定 recipient/self/account/tenant/caller，保留 `expiresAt + lifetimeMillis + 2 × challengeWindow` 的保守上界。返回原 `HANDOVER_REQUIRED`，原 worker exact ACK 后才能重新申请；既有请求过期仍拒绝。无新 API、表迁移、定时器、MCU 槽、App 重试器或业务 Action 重放。

Service **56/56，0 skip**（五个文件，计数含父项）；包括并发、零提前取 Key、ACK 校验、未过期其他 caller/账号/设备隔离及自然到期上界。App 原 builder **9 文件 163/163** 本地接入/组合回归；Gateway TypeScript 构建通过。设备未改源码，现有 Debug host **62/62**，未在本批重新编译全部 MCU/SDK；第一次运行缺少本机 Debug/ASan DLL 搜索路径，补齐进程 PATH 后重跑通过，不是设备运行缺陷。

### 10.2 必要真机结果

复用原真实 owner/receiver 账号、COM12 LAN self 与原固件/绑定。Gateway 与两个相关 Broker 的原跨片开关均经备份临时开启，无 publish 故障注入。手机沿用已安装 APK `659a10274e979de270de708dc28a9d695ddf0ee6409ff9385149824870a290c3`，只在当前进程启用原 LAN 配置对象，不改鉴权条件或源码默认。

| 项目 | 实际结果及边界 |
| --- | --- |
| 真实分享增权 | 定向分享 operator 仅 power → 全端点；PATCH 后原账户 MQTT **1,961 ms** ready，页面可控 **1,986 ms**，新 LAN Control **2,269 ms** ready。原 MQTT 会话未替换，属性/按钮随后 ACK **169/91 ms** |
| 到期后旧通道 | 真正请求并签发 **15,000 ms** 租约，不修改时钟/Grant/证明；旧 session 关闭后显式发送拒绝，记录数 **15→15**，没有旧通道新包；页面原 MQTT 属性 ACK **221 ms** |
| 同 caller 到期退休 | 两次真实 `caller_lease_expired` outbox→物理 ACK **421/1,065 ms**；PG 证明新授权签发晚于 exact ACK。首个 handover 尝试仅收 Challenge、无 Grant/业务发送；之后新 request、原 caller |
| 不退出页面的到期恢复 | 原页面自己重新准备，旧 LAN 关闭后 **3,106 ms** 新 LAN ready，原 MQTT 不变；属性/按钮 ACK **160/113 ms**，无工具代替页面重进或自动重复 Action |
| 精确取消/退出 | 第一条到期链在原页面已经发起 handover 后显式退出，再正常进入恢复；第二条不干预页面。最终连接组/占用/BLE link 均 0，测试观察器已移除 |

时间是本机有限样本，不是 P95/SLA，不称无缝切换完成。App 与 Service 时钟有偏移，耗时只在各自时钟内计算；ACK→签发顺序使用同 Service 审计。最初旧测试工具在 PATCH 返回后误把尚未失效的旧 Observer 页判作新 ready，过早发出的 Property 收到 `TARGET_UNREACHABLE`，按钮未发送；记录保留。修正的是测试观察条件，之后按新 context/新 ready 观测，没有加大产品超时或自动重放失败命令。

采样边界：本批真机证明过期旧 LAN 不再承载帧、原云控继续；MCU 原 Plain/Noise host 回归证明过期/退休丢弃待发队列并通知断开。**未另运行持续 Telemetry 采样真机**，不能把它写成所有平台到期停采样都通过；C3 保留对应短采样检查。

### 10.3 收尾、制品与下一步

临时 share 已通过原 DELETE 撤销，两个 Broker 失效版本均 **22**、pending=false；相关退休全部 exact ACK。恢复原 owner，COM3/COM11/COM12 原 MQTT Property/Action/State 均通过；未刷固件、未 reset、未清 NVS/绑定。Gateway/Broker 三份入口逐字节恢复，仅保留已修正仓库模块（SHA256 `c89a220bbbf19a0578ea45268b4318d0dd9e7204c39407f5f5e98b3afe53bd97`）；App `lanEnabled=false`，首页 MQTT ready，无测试 owner/钩子。测试分享不保留；原设备及页面保留，可通过正常新分享再次授权。

证据为 ignored `build/c2-recovery-20260914/`：`pg-before.log`/`pg-after.log`、`app.xml`、`host.xml`、`page-*`、`expiry.json`（取消后重入）、`expiry-auto.json`（原页面自动恢复）、`audit-*`、`runtime-*`、`witness-*`、`final.json`。`verify.cjs` 交叉核验生成 `verified.json`，不是只读取脚本自报通过。私有账号/邀请仅在本机 ignored 目录，不进入源码/ZIP。

**当前顺序冻结为 C3→C4，不继续 LAN 专项扩展：**

1. 先只读记录三端源码/构建摘要、当前 SDK/profile/依赖和功能开关，生成同一个标准 Arduino ZIP；执行原独立 sketchbook 安装编译与正式例程，不要求用户单独安装平台产品包。不把旧 B1 的 ZIP hash 当新制品。
2. 先以现有 ESP32S3 完成受影响的 WiFi/BLE/WiFi+BLE 短回归；保留见证板，仅操作每次确认的测试角色。R4 WiFi/BLE、Wio WiFi 换板后按同制品补齐，不把别名含 R4 的 ESP32 见证算作 R4 硬件证据。
3. 每项记录接入/Manifest/Property/Action/State、普通掉线与重启、退出、短按需采样停止及必要解绑/重配的证据和内存边界；C1/C2 已过的深故障不无理由再重复。Wio BLE/网关/LAN 全平台仍 preview/internal。
4. C4 收口三端匹配部署配置，尤其共享跨片的 Gateway＋源/目标 Broker 一致性；**本批临时开启后恢复不等于默认部署已提供跨片分享**。干净 SDK/CI、生产地址/信任材料、必要源码/例程清单与发布边界另验，不能靠运行入口补丁发布。

本批生产修改仅 Service 原仓库及对应测试；App/MCU 生产源码未变，无迁移、未提交/推送/暂存。上述新增文档状态优先于专项文档较早的“下一步”。

## 11. C3 当前制品、安装门禁与正式示例（2026-09-14）

详细结果与剩余检查只在 [C3 最终基础矩阵报告](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md) 维护。本批仍属核心稳定主线；不是重新启动 LAN/网关/无缝切换专项。

最终 development ZIP 为 388 个文件，两次独立打包一致。纠正 README 的 WiFi+BLE 授时旧描述；修正 ZIP 门禁把已启用授时的 CloudSmoke 误当未启用的测试缺口，包边界回归 15/15。没有用删除现有功能、修改协议或放宽链接闭包来换取通过。

COM11 已读回原应用并确认身份，随后只写应用分区，运行包内未改动的 WiFiBleQuickStart；保留 NVS、网络、逻辑 ID 和授权。MQTT、自动 BLE 选择、Manifest/属性/按钮/状态与退出回收均已有当前包的真机证据；普通重启后的 BLE 恢复及后续云控可用。两次早期云控观察必须保留：一次重连窗口中的 Property 未成功且未发 Action；一次以重启前缓存按钮次数作基准导致测试断言失败，实际 Property/Action 有 ACK 且新状态更新。**云端缓存可读不等于设备已恢复就绪**。随后保持原 MQTT/状态订阅、不发送任何命令的被动复测，复位工具结束后约 2,143 ms 自动收到新 revision 1 / 按钮 0，证明普通恢复不依赖控制操作。未调整生产超时、加 Action 自动重试或把有限样本当无缝 SLA；短采样仍须补证。

首批安装编译完成 4/7，其余因 C 盘余量不足主动停止/未启动。缓存处理后使用同一 ZIP、新证据目录、D 盘临时/core cache、顺序单 job 完成 CloudSmoke、R4 BLE、Wio WiFi，当前 **7/7 安装编译与实际依赖/来源/map/ELF 全通过**。CloudSmoke 的真实授时闭包通过；本轮源码重新打包仍同摘要。原资源中断保留，不计作固件缺陷。当前 host 重建后 62/62、包边界 15/15、云控工具 8/8、架构/profile 检查再次通过；R4/Wio 没有本轮实物证据，不等价为真机全过。

本轮在专用 COM11 真正刷入同 ZIP 独立 BleQuickStart，完成初次控制、普通 MCU 重启、App 冷启动和退出回收；已只恢复原 WiFiBleQuickStart 应用并校验，NVS/逻辑 ID/Key/授权不变。不是仅测混合产品的 BLE 通道。用户暂时无法换 R4/Wio，因此实物项保留待测，COM3/COM12 仍是 ESP32S3 见证，不重复请求换板。

既有 Docker/ADB 恢复后，两块 TLS 见证初次未自动就绪；串口观察出现 USB reset 启动记录后才恢复，不能判为无干预通过。之后单独重启原 TLS proxy，不操作串口/MCU，两块原 MQTT 控制通过。原失败与有限对照分开保存，待在最终独立 WiFi 制品上排查整套环境/USB 边界，不先改协议、加重试或宣称解决，见 C3 报告 §7。

本节检查点的下一步为 ESP32 独立 WiFi/短采样及恢复对照；其最新进度已在 §12 更新。七目标编译已关闭，不再按旧“中断恢复”重跑全部目标。C4 预审明确了静态/动态 Broker 开关注入，以及 CI 尚未提供 BLE 准入 SDK 的缺口，详见 C3 报告 §5；它们不靠临时入口补丁解决。沿用 C1/C2 真实绑定和权限深故障证据，不重复清授权或扩产品入口。本恢复批没有生产源码修改或 App/Service 配置部署，仅重启既有本机服务；无暂存/提交/推送。

## 12. C3 独立 WiFi / 短采样已补证；收口范围不扩展（2026-09-14）

最新事实见 [C3 报告 §8](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md)。同 ZIP、实际 S3、仅替换 DeviceKey/SSID/password 用户输入的 WiFiQuickStart 与 CloudSmokeTest 已编译/校验/刷入正式示例；不以混合产品或泛 ESP32 ELF 代替，无设备/Service 生产改动、清 NVS、重新绑定或刷新 Key。

真机发现并修正 App 核心就绪缺陷：历史 BLE 凭据/nearby/候选重试不能替换已可用 Cloud。原选择器仅在 Direct Ready 后切换；原页面连接组保留旧 ready scope，候选成功才退休、失败保留、取消同时回收。去掉提前选 BLE、提前关 scope、无条件用 syncing 禁用控制的旧行为。151/151 相关回归、TypeScript、定点 ESLint、真实 APK 构建/保留数据安装及页面复测通过；不新增协议、每设备 MQTT、连接容量、重试器或 Action 重放，不展开无缝通信专项。

S3 实物已覆盖独立 WiFi 接入/Manifest/Property/Action/State、普通重启、持续可控和退出；CloudSmoke 原页面覆盖 1 秒按需流、真实 HOME 停止、前台恢复、退出停止、暂停真实 Broker 后自然到期和原页自动恢复。MCU samples/dropped/leaseExpired 与 App/Broker 顺序联合核验；串口连续读取、未 reset/改时钟。暂停造成正常 MQTT 重连，不称 Session 从未变化；没有额外回调计数，不将发送次数写成 `onSample` 次数。

最终 COM11 已恢复原 WiFi+BLE 应用并核验摘要，新 APK 下 BLE 和 MQTT 再次通过；两块未改固件的 TLS 见证也通过。原 owner/网络/授权保留，首页连接和钩子归零，LAN 仍默认关闭。本批没有暂存/提交/推送。

**剩余执行顺序：**

1. C3 只补报告 §7.3 整套服务/USB 恢复边界的有限可观测对照：本批单 Broker 暂停恢复已过，但不能替代先前整套环境失败。固定制品、记录是否 USB reset，不能靠打印日志/重刷/无限等待达成通过，不做 100 次循环。
2. R4 WiFi/BLE、Wio WiFi 仍待实物；用户暂时不能换板，保留待测，不反复请求、不把 S3 别名或七目标编译代替真机。
3. 硬件受限时可推进独立的 C4 配置/CI 静态检查与定点修正，但不宣布 C3 通过或正式发布。优先原静态/动态部署的跨片开关一致性、生产配置与本地制品隔离、干净 SDK/CI 准入；不以新 LAN/网关/分享/编辑器功能绕过发布缺项。

首次进入存在历史 Direct 凭据时仍走原 5 秒有界探测，实测 ready 约 5.6～5.8 秒；本轮关闭的是“可用后被候选探测破坏”，不是首次延迟 SLA。该体验边界明确保留，不能悄悄加协议或删除用户凭据来缩短表面时间。

## 13. R4/Wio 暂缓，先 C4 可复现配置（2026-09-15）

当前优先级为 C4，不展开无缝通信。上述 5.6～5.8 秒发生在保留历史 BLE 凭据但已刷为独立 WiFi 的设备；不能称为所有 BLE 首次进入耗时。真正独立 BLE 的同包有限样本约 3.2～3.4 秒，混合产品约 3.4～3.8 秒。先保留准确阶段测量和有界失败，不擅自删授权、增重试或新增双通道竞速。若再次发现可用通道被破坏、错误就绪或不可取消，则作为基础缺陷优先修复。

本批完成三个互不增加协议复杂度的配置修正，具体证据/剩余步骤只在 [C4 可复现部署执行表](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md) 维护：

1. Service 复用原解析/Controller 模板，统一注入 Gateway、静态 Broker 和未来动态 Broker 的 requests 开关，shares 只归 Gateway；默认 false。Compose 默认/四组合回归、Controller 25/25 通过；已有动态容器必须另行受控对齐，未部署。
2. App 增加构建期 `local-device` 覆盖，明确 Android 本机 HTTPS `17000`，不再改生成的 JS 或运行中换 authority；浏览器/生产配置及 LAN 默认不变。相关 111/111、类型/lint 和 Angular 实际构建通过；本轮未生成/安装新 APK，不将旧 APK 证据当作新构建通过。
3. 官方 S3 3.3.11 Core/SDK 溯源通过，消除“需要私有准入 SDK”的误解。同一个 ZIP 的 CI 分成原 stock WiFi/跨平台五目标＋官方 3.3.11 BLE 两目标，SDK 摘要检查只在维护门禁运行，用户不需 Python。包边界 17/17；干净远端 CI 尚未运行，不改变 MCU 协议/源码或声称所有 SDK 可用。

下一步先按新构建入口生成并验证匹配 APK，盘点/备份现有静态与动态分片后完成原开关受控部署及真实双账号验证，再补 C3 整套环境恢复的有限对照。网络已变为 `mostfun2026`，本机 IP 也已变化，旧固件测试 profile 的 IP/SSID 不再视作有效基线：必须核验电脑/手机/测试板地址及服务可达后再做故障注入，不把换网后的未连接判为自动恢复缺陷。不清 NVS/绑定、COM3/COM12 继续保留见证；R4/Wio 待用户后续可换板时补证。

本批未部署服务/迁移数据库、未刷板/改 WiFi/清授权、未改手机 APK、未暂存/提交/推送。C3/C4 均未整体关闭，生产信任材料与实物缺项不能被配置回归替代。

## 14. C4-B 新 APK 通过；下一步先补动态分片维护流程（2026-09-15）

按新构建入口生成并保留数据安装 APK `d1ceb65ca1c75fbdd90053d5c3b48ec6767dcead39021befcfe249b2d3ec621c`，
手机实际摘要吻合；326 个打包 JS 未二次修改、debug CA/HTTPS/签名及 release 合并清单隔离通过。
原账号目录/账户 MQTT 可用，新包首次进入和 App 真冷启动两轮 COM11 BLE Property/Action/State
均通过（ready 3.23/3.53 秒，ACK 266～299 ms），退出连接/占用/钩子归零。未刷板/清绑定；
这不是换网后设备 MQTT 或跨片分享通过。过程与中断保留见 [C4 §6](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md#6-c4-b-新-apk-闭环与-c4-a-升级前置纠正2026-09-15)。

**纠正上批的直接部署顺序：** 已有动态 Broker 2 带 reservation，当前匹配旧模板；候选模板
在实际只读预检中报 `SHARD_TEMPLATE_MISMATCH`。Controller 现无原地保 reservation 升级 API，
新增默认 false 字段也改变指纹，不能直接 `compose up`、改 label 或 remove/create 解决。
新增预检命令复用原配置/校验，Controller＋预检 32/32、Compose 四组合通过；容器/镜像未变。

下一包先实现并验证 **C4-A 单机维护窗口升级** 的明确计划、检查点、备份、保留 reservation/卷
及失败回退；不扩零停机/HA，不新增设备协议或 App 接口。通过隔离失败门禁后再部署现有测试栈、
执行双账号跨片/撤权。C3 网络重新核验及整套环境恢复随后补，R4/Wio 仍暂缓；干净 CI/生产
profile 仍待完成。不要重复已经通过的新 APK/BLE 样本来绕开当前升级缺项。

## 15. C4-A 内核/隔离失败门禁已过；现有栈操作器待完成（2026-09-15）

已实现独立维护计划、持久意图日志/锁、同 reservation/凭据/数据卷的候选替换、数据回滚及
最终确认；不扩 Controller HTTP 或设备/App API。54/54 单测和 Compose 四组合通过；真实
隔离 Docker 的“候选不健康→回滚原容器/SQLite”和“创建响应丢失→续跑→最终确认→原
reservation replay”均通过，21 个既有容器状态/启动时间不变。结果与真实失败修正见
[C4 §7](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md#7-c4-a-维护内核及隔离-docker-门禁2026-09-15)。

新增只读基线证据：Gateway local-access-repository.js 当前运行字节不同于原镜像，但与当前
源码按原配置编译的输出一致。不能把旧镜像当作有效行为的完整回滚基线；本轮没有回滚该文件
或再次热复制 JS。下一步先固化该已有修正及对应制品，再补现有单机栈操作器/真实 PG 和全部
SQLite 备份/全流量写入隔离。只停 Gateway/Controller 是分配隔离，不能保证 Broker 数据停止
变化；最终确认后恢复入口前必须完成整组模板/保留关系验证，不声称已经原子升级全部分片。

C4-A 尚未部署现有栈或完成真实跨片/撤权，C3 整栈恢复、R4/Wio 实物、干净 CI/生产 profile
继续保留。当前先完成核心稳定可用，不恢复 100 次循环、并行握手或其他扩展功能。未刷板、
清授权、暂存、提交或推送。

## 16. C4-A 制品/整栈隔离完成，下一包连接真实备份与整组升级（2026-09-15）

以固定的 240 个源码输入构建 Gateway/Broker/Controller；有效 Gateway 修正已纳入 recovery
镜像，179 个代码文件与当前运行树逐一匹配。候选/原栈/回滚镜像保存到已完整校验的
linux/amd64 归档，尚未做干净引擎导入/启动。Compose 增加可选 CONTROLLER_IMAGE，默认不变。

独立整栈维护隔离已实现：同项目跨目录锁、明确入口 ID、入口/控制/写入者按序关闭、PG/Redis
不操作、原实例与 restart policy 按就绪顺序复原。现场 TLS/明文代理归属不同的假设已纠正，
只读 v2 计划覆盖 9 个实例，没有关闭现有服务。72/72、Compose 矩阵及隔离 Docker 生命周期门禁
通过，原 Broker 回滚/续跑门禁复测通过。详细证据/失败与边界见
[C4 §8](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md#8-c4-a-制品与整栈维护隔离2026-09-15)。

**下一包必须整合真实备份和整组升级，而不是再扩维护框架：**

1. 受保护的配置、实际 PG/每个 SQLite 备份/校验，先确认迁移/cleanup 空闲；复用既有 SQLite 工具。
2. 在隔离真数据库中连接静态服务/动态分片替换、失败恢复、原 reservation 和新实例开放入口。
   当前 restoreOriginal 只允许原 ID，不能忽略其拒绝来开放新版本。
3. 通过后维护升级当前测试栈，执行真实跨片/撤权，再补 C3 环境恢复；C4-A 未关闭。

设备/App 协议和源码未增加功能，未部署现有服务/清授权/刷板/提交推送。R4/Wio 实物、干净
CI/生产 profile 继续保留；仍优先核心稳定，不转向无缝通信或新协议形态。

## 17. C4-A 真数据库备份/恢复门禁完成，下一包整合整组替换（2026-09-15）

主线仍是核心稳定 C4-A，不是无缝通信。本批将原整栈 fence 与具体数据备份端口接通；
86/86 软件回归、Compose 矩阵和独立 Docker 真数据库门禁通过。PG 使用实际 Gateway
38 个迁移/46 张表，3 个 Broker 使用实际 SQLite schema；验证 PG 行内容/序列/完整 schema
及 SQLite 恢复、未 checkpoint WAL、迁移/冻结拒绝、坏摘要/错计划拒绝和原入口状态复原。
Windows 备份目录按当前用户＋SYSTEM 限制 ACL，配置和环境秘密不进入公开日志。

没有替换旧 SQLite 工具或增加三端运行时依赖。修正了只读 WAL 需要临时共享内存、PG
逻辑恢复不保留删列物理序号、CHECK 表达式规范化等实测假设，清除重复的局部 schema
指纹逻辑。过程/边界/证据见 [C4 §9](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md#9-c4-a-真实数据库备份与恢复门禁2026-09-15)。

**下一包：静态 Gateway/Controller/Broker＋动态分片整组替换、整组失败恢复和新实例准入。**
先在隔离数据库中接入现有分片事务和数据回执；原 ID 的 restoreOriginal 仍不能放行新 ID。
组回滚需要同时核验 PG、所有 SQLite、配置及 reservation/派生凭据，并明确 Redis 缓存处理；
新实例启动时还须控制直接发布的 MQTT/HTTP 端口，不能把“代理仍关闭”视为全部入口已关闭。
实际配置文件/JWT/部署 Secret 的覆盖清单与镜像干净导入仍需完成，再进入现有栈维护窗口、
真实跨片/撤权及 C3 环境恢复。现阶段不加 HA、100 次循环或新设备功能。

本批只操作随机隔离 fixture，22 个既有容器保持原状态。**现有栈的实际数据未备份/升级，
整组数据回写与跨片业务尚未验收**；C4-A 不关闭。测试容器/卷已精确清理，私有备份和失败证据
保留在 D 盘。未改 App/设备源码、刷板、清授权、暂存、提交或推送；R4/Wio 实物继续暂缓。

## 18. C4-A 原实例多库回滚与中断续跑通过（2026-09-15）

本批完成恢复链路的三个具体缺项，不增加常驻服务或设备 API：

1. 在写入者关闭、备份/恢复回执与计划严格绑定的前提下，向隔离原 PG 和全部 3 个 SQLite
   回写已验证的备份；候选模拟新增表/改写记录均撤回，再校验完整 schema、行内容和序列。
2. PG 或首个 SQLite 已恢复、但完成回执未写入时，保持所有业务写入者停止；续跑整个恢复
   并重新校验后才允许重开。只认最新一次恢复完成，不以旧成功标记放过后来的失败。
3. 真实 Redis 只失效 Gateway 路由投影及版本，保留限流计数和其他 key；Gateway/Broker
   开启持久 MQTT Session 时明确拒绝当前恢复路径。没有增加全库 flush 或暗含 Secret 回滚。

结构保持职责分离：整栈 fence 管入口/原实例，分片事务管替换，数据端口管备份恢复。
先捕获整栈原状态并停写，再生成分片计划，现有 rollback 就会保持原 Broker 停止；不加
“延迟启动”配置。候选提前发布宿主端口（含 loopback）会被拒绝，但不把无 published port
当作已完全隔离同 Docker 网络的流量。详见 [C4 §10](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md#10-c4-a-原实例多库回滚中断续跑与候选端口准入2026-09-15)。

软件 **102/102**、Compose 矩阵通过；独立 Docker **12 项门禁**通过（46 PG 表、38 个迁移、
3 SQLite、真实 Redis）。最终样本中 21 个既有容器 ID/镜像/运行状态/启动时间不变；精确
清理本轮测试容器/卷，D 盘私有备份/失败证据保留。不是设备 100 次循环或实际 MQTT 联调。

**下一包仍是 C4-A 整组替换与新实例准入：** 接入静态服务和动态分片的明确替换回执、
原 reservation/派生凭据核对、直接端口与 Docker 网络准入，再做实际候选业务启动后的
整组失败恢复。当前数据恢复只接受原实例 ID、原配置未变；不能据此放行新实例或宣称
配置/密钥已恢复。实际 Secret 清单、镜像干净导入、现有栈升级与双账号跨片/撤权仍待验收。
随后补 C3 整套环境恢复；R4/Wio 实物继续暂缓，不转入无缝通信扩展。

本批未操作现有业务数据库/部署、App/设备源码或开发板，未清授权、暂存、提交、推送。

## 19. 收窄优先级：暂停服务器维护，回到 R4 基础真机（2026-09-15）

按用户最新决定，覆盖 §18 及 C4 专项中旧“下一包”顺序。C4 整组替换/新实例准入、
部署恢复操作器继续保留未完成记录，但**本阶段暂停实施**，不继续扩大服务器维护细节。
无需开启跨片分享即可验证同账号普通 WiFi/BLE 的核心接入；不临时改开关/改运行 JS 绕过。

当前按 COM9 为新接入 UNO R4 WiFi 推进，Wio 继续暂缓。BLE 首次进入延迟只记录，保留
体验待优化，不加新的扫描/并行握手/自动抢占协议。三个当前执行步骤：

1. 核对 USB 板卡、原固件/存储及实际网络/服务可达性；用同 C3 ZIP 和已安装的 Renesas
   1.5.3，先运行普通 WiFi 标准示例。仅替换用户 DeviceKey/SSID/password，网络 authority
   若变化则如实单列 profile 差异；不清授权、不改 ESP32 桥接固件或擅自擦存储。
2. 实际 App/用户 MQTT 获取 Manifest/State，Property、button Action 各一次并核对状态，
   再做有限普通重启/重新登录/退出回收。只修复复现的基础断点，不扩服务器维护方案。
3. 同一 R4 再验证独立 BLE 示例、已有绑定/必要的正常认领、控制/状态与普通恢复；可行时
   补现有授时/采样短检查，不因覆盖更多条目而使用内部 composition 替代标准接口。

本轮 WiFi/BLE 分开运行，不为 UNO R4 增加 WiFi+BLE。保护其他板的固件及绑定；若缺少
有效恢复镜像/身份映射，应明确记录而非把旧 ESP32 别名算作 R4。真机结果写 C3 报告，
未测项目保留，不以编译通过填充实物矩阵。现有 C4 产物保留，不为“清理”删除仍有用途的代码。

当前检查点见 [C3 §9](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md#9-com9-uno-r4恢复基础真机与本机换网2026-09-15)：
mostfun2026 的必要本机地址调整已完成，Gateway 有效业务代码保持相同。R4 标准 WiFi 的
实际登录/MQTT、Manifest/State、Property/Action、普通 MCU reset、App 杀进程后恢复和退出回收
已通过；独立读回应用摘要一致。用户允许只重置 R4 BLE 后，已发送一次公开 resetAccess 指令；
串口回执未知但随后真实 Provisioning / App 重新认领及自动进入原控制页通过，旧记录没有删除。
用户拔插后，原标准 BLE 固件已恢复并完整读回确认；标准 BLE 的控制、MCU 重启及 App
冷启动三轮也已通过。COM9 最终为标准 BLE 与新授权，无维护程序/本地会话遗留，不需再次
resetAccess 或拔插。原 WiFi 三轮不重复执行。Wio、BLE 首次延迟与 C4 服务器维护继续按本节
暂缓；C3 整体未完成，下一步只核对必要剩余补证，不转回服务器扩展或无缝专项。多次烧录需
拔插的现象独立保留，尚不能归因为库协议缺陷；整板断电/整套环境恢复不能以 RA reset 代替。

## 20. COM6 Wio 已接入：补普通 WiFi 矩阵（2026-09-16）

用户已将 R4 换成 COM6 Wio Terminal，覆盖 §19 的“Wio 暂缓”。本批只推进已声明支持的
普通 WiFi，Wio BLE 仍 preview/后置；C4 服务器后续维护、BLE 首次进入优化继续暂停。
COM3/COM11 见证板不动，不把先前的 R4 USB 握手现象推定为 Wio 缺陷。

执行顺序保持有界：确认实物与备份 → 原 Key / 标准 WiFiQuickStart 安装编译和独立读回 →
真实 App 的 Manifest/Property/Action/State → 普通 MCU reset 和 App 冷启动 → 退出资源回收。
不清外置 QSPI 授权/网络记录，不升级 RTL 固件；若确有基础断点，先定位所在模块，不添加
新的轮询、协议或内部示例绕过。当前串口触发的是 MCU reset，不伪称整板断电测试。

Docker 已恢复；mostfun2026 下电脑地址从 `.139` 变为 `.133`，只调整既有 Gateway/明文入口
的公告和宿主绑定，保持有效业务代码及卷不变。沿用正常 owner 查看得到的原 Wio Key，
不旋转/新建。同 C3 源码按测试地址生成 ZIP，profile 差异和实测结果写入
[C3 §10](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md#10-com6-wio-terminal普通-wifi-实物补证2026-09-16)。

**本批完成：** 标准示例编译/来源/依赖、CLI 烧录与独立应用读回通过；program 143,040 B、
静态 RAM 68,072 B。真实 App 完成登录/MQTT、三字段 Manifest/State、Property/Action、
普通 MCU reset 和 App 冷启动三轮，退出资源归零。初轮 USB/账户 MQTT 恢复导致会话改变，
冷启动前置也曾因丢失 ADB reverse 失败，均在报告保留；不声称全程无中断或首次连接 SLA。
最终 Wio 运行标准 WiFi 与原 Key，未改三端生产源码、清授权或升级 RTL，无提交/推送。

下一步只补 C3 报告 §7.3 整套环境恢复的有限对照及声明范围核对；先明确当前测试 profile，
不把旧网络见证板未连接判为设备缺陷。整板长断电、跨平台 Time/Telemetry、运行峰值 RAM
与正式部署/干净 CI 仍单列未测，不以本轮基本控制通过替代。Wio BLE、BLE 首次延迟与 C4
维护扩展继续后置，不启动 100 次循环或新的无缝通信功能。

## 21. C3 关闭清单与首次进入优化的顺序（2026-09-16）

本节覆盖旧章节中的“下一包”。按用户要求优先核心稳定，**先收口 C3，再定点优化首次进入**；
不是重新展开平台矩阵或无缝通信项目。可复现部署/服务器维护仍暂停，不能为完成 C3 反过来
要求先完成 C4 整组替换工具。

已完成的安装编译为七目标，基础实物为 ESP32S3 WiFi/BLE/WiFi+BLE、UNO R4 WiFi/BLE、
Wio WiFi 六种模式；ESP32 CloudSmoke 短采样另有证据。C1/C2 冻结范围继续沿用已通过记录。
本批按原 C3 profile 从当前源码重新生成完整 388 文件，**仅 README 的官方 SDK 说明改变，
全部编译输入相同**；R4/Wio 实物包另有已声明的地址 profile 差异，不声称所有包 SHA 相同。
实际手机 APK 也与 C4-B 验收包一致。详情及完整摘要见
[C3 §11](DEVICE_V2_C3_FINAL_BASIC_MATRIX_REPORT.md#11-c3-关闭前审计与有界剩余项2026-09-16)。

**当前关闭前的功能尾项只有已知 §7.3 恢复问题的有限对照及结论**：历史 TLS 见证在整套环境/
USB 恢复后没有自然上线，后来出现 MCU reset 才恢复，不能以已过的单 Broker/代理重启替代。
本轮只读检查发现当前测试证书仍覆盖旧 `.2.76`，不覆盖电脑现址 `.1.133`，旧 TLS 见证也
离线；这只是今天的前置不满足，**不是对历史失败的根因结论**。下一步先修正最小测试地址/
证书及匹配固件基线，再做一次有界故障—自然恢复—精确控制/退出闭环；观察期间不打开串口、
复位或发业务命令诱导恢复。若失败，定位对应模块，不加无限重试、不延长窗口掩盖问题。

C3 关闭只表示冻结的基本接入/控制/有限恢复矩阵通过，不表示生产发布、全部可选 API 或
任意环境故障均已验收。干净 CI/正式地址与信任材料属于 C4；Wio BLE、整板长断电、跨平台
Time/Telemetry、运行峰值 RAM、100 次循环与多通道无缝 SLA 仍明确未测/后置，不偷偷算作通过。
R4 偶发上传握手需 USB 拔插也保留为工具链/桥接待定位限制，不以更改库协议绕过。

首次进入优化另列为下一有界包：当前 Cloud-capable 设备只要有历史 BLE 凭据，就可能先等
最多 5 秒 BLE 再接 Cloud；这与纯 BLE 自身约 3～6 秒的鉴权/能力读取不是同一个问题。
先在原页面连接组内消除失败候选对可用 Cloud 的串行阻塞，保留唯一账户 MQTT、真实 BLE
ready 才选路及退出取消，不能复制一套连接器或自动重放 Action。纯 BLE、配网 handoff 与
网关子设备分别保留原合同；测试和分阶段测量见 C3 §11.4，不先通过砍安全握手/超时“提速”。

本批包边界回归 **17/17**、架构与 profile 检查通过。未修改生产源码、刷板、重启服务、清授权、
暂存或提交推送；只读现场仍为 Wio cloud reachable、App 首页/账户 ready、本地连接与钩子 0。
**C3 保持未关闭**，本地证据 `build/c3-closeout-20260916/` 保留，不能把本节计划当真机结果。

## 22. 本批恢复对照与新增 Wio 尾项（2026-09-16）

本节保留本轮定位过程；最终完成状态与当前下一步见 §23。

本节覆盖 §21 的执行状态。COM3 S3 已用当前测试地址与验证开启的标准 TLS WiFi 示例完成
九个既有 IoT 测试容器停止/恢复：停服 38.414 秒，服务启动后 28.006 秒自然恢复，期间没有
MCU reset/串口干预/业务命令；随后属性和按钮准确通过，按钮计数 1→2、退出资源归零。
原容器 ID/镜像/卷保留，其他七个容器未动，没有展开 C4 升级或维护功能。

**C3 仍未整体关闭：随后补查 Wio，发现其 cloud lastSeen 停在停服时，未取得 fresh State，
尚未向 Wio 发送控制命令。** 单个 S3 恢复通过不足以排除这个新发现；本地汇总曾提前写成
complete，现已纠正。优先保留现场、定位 Wio 恢复层次，再做同条件有限复测，不用复位后
在线、缓存 Manifest 或延长观察时间替代自然恢复。详见 C3 报告 §12。

App 的下一小包已完成代码与离线回归：普通 Cloud-capable 页面同时准备原账户 Cloud 与
已有 Direct 候选，可用 Cloud 不再串行等待失败 BLE；未新增 MQTT 连接、选路器、Action
重放或后台扫描。仅改原 DeviceUiPort 和配套测试；456 个 Vitest 回归、类型检查与定点 lint
通过。匹配 APK 的 S3 Cloud ready 620 ms、另一 S3 自动 BLE ready 3,649 ms，控制/退出通过。
这不是“纯 BLE 已提速”的证据；历史 BLE 凭据但设备 BLE 不可用的真实对照仍需单列补测。
当前回到 C3 的 Wio 恢复尾项，暂不继续扩大首次连接优化、Wio BLE 或服务器维护。

尾项已定相位复现并完成最小软件修正，见 C3 §12.4：停服跨过 session 续签点时，代理的
chunked 空 502/504 被小型 HTTP 适配器判成不可重试的 UnsupportedFeature。设备只对
临时 HTTP 状态的不支持 framing 进入原退避，代理空响应补 Content-Length: 0；成功/
鉴权错误的严格处理不变，无新增 RAM 缓冲。host 62/62、代理 10/10 通过。当前正在补新
ZIP 的受影响 WiFi 编译和标准 Wio 同相位复测，完成前仍不关闭 C3。SDK 默认无限 TCP
等待保留为独立风险，本次没有据此添加专用 wrapper 或更换依赖。

标准 Wio 的修正后同相位门禁已通过，见 C3 §12.5：服务恢复后 32.794 秒取得 fresh ready，
没有串口/复位介入，按钮计数连续、控制及退出回收通过。新应用仅增加 32 B、静态 RAM
不变。R4 受影响 WiFi 路径也补编译通过；目前只待 ESP32 补编译汇总，不再展开新的 C3
故障测试。完成该软件检查后冻结基本矩阵，转到首次进入小包的 BLE 候选不可用实物对照。

## 23. C3 已收口；当前只推进首次进入小包（2026-09-16）

最终机器可校验汇总 `build/c3-closeout-20260916/final-summary.json` 已通过，
**c3Complete=true（冻结基本矩阵，不是生产发布）**。既有七目标/六种基础模式证据保留，
本次 HTTP 单头文件修正后，S3 TLS、R4 WiFi、Wio WiFi 均经重新安装、逐字节/依赖/ELF
补编译验证；三平台静态 RAM 不变。Wio 的续签点定相位故障、精确控制、计数连续和退出
回收已用标准固件完成，失败样本及因果对照未删除。详见 C3 §12.4～12.6。

当前下一包固定为首次进入，禁止再按 §19～22 的历史计划重复清授权或重新展开 C4：

1. 已完成：原 DeviceUiPort 并行准备可用 Cloud 与 Direct 候选，复用唯一账户 MQTT/
   页面连接组；取消/迟到/失败顺序回归 456/456，匹配 APK 的 Cloud 和自动 BLE 短真机通过。
2. 下一项：在已确认、可恢复的专用 S3 上作真实“历史 BLE 凭据仍在、BLE 不可用、Cloud
   可用”对照。不得伪造凭据/ready、按 MAC 合并业务身份或重放 Action；记录首次 Cloud
   ready、迟到候选、页面退出的资源回收。若需临时切换固件，先备份、保留授权并最终恢复。
3. 然后再决定是否优化纯 BLE：先分解发现/GATT/安全握手/Manifest/State 时间，只修最大
   可解释的延迟项。当前 3.649 秒样本不等于已经提速；不预连接所有设备、不删除鉴权步骤，
   不增加第二套 MQTT/扫描器/选路器或追求未冻结的无缝 SLA。

Wio BLE、网关/LAN 扩展、100 次循环、全部 Time/Telemetry 组合、整板长断电、运行峰值
RAM、干净 CI/正式 profile/三端发布仍保持原后置边界。C4 服务器维护继续暂停。本批没有
暂存、提交或推送；没有清任何设备授权，COM6 已是标准 WiFi 固件，手机回首页、本地资源 0。

## 24. 首次进入对照及取消修正已通过（2026-09-18）

C3 冻结结论不变，不再依据历史章节重复清授权/重跑全矩阵。本批完成 §23 的真实对照：
COM11 S3 保留原 active BLE 凭据、原逻辑 ID/DeviceKey，临时标准 WiFi-only 固件使 BLE
确实不可用、Cloud 可用；最终 App **371 ms** 可控，先于 BLE 候选 **5,294 ms** 的失败。
Property/Action/精确计数、迟到失败保持 Cloud、提前退出和重进通过；唯一账户 MQTT 复用。

真机发现并修复一处取消竞态：等待后台 Presence 扫描结束的 Direct 尝试此前尚未登记
opening，页面退出会漏取消，之后仍启动扫描。现将这段等待移入原 BLE service 的已追踪
open 生命周期，删除未归属的递归等待；没有新连接器/状态机/设备 wire。两条回归先红后绿，
8 文件 **142/142**、App/spec 类型与 lint 通过，匹配 APK 保留数据安装并核对实际 hash。
快速退出后约 52 ms 候选结束，100 ms 和 6.5 秒检查无 pending/native 资源；重进 155 ms。
原有三次有界前台重试保留，不能误称整页只扫描一次。

原 COM11 应用整分区已恢复并验证 SHA256，BLE 认证/控制/状态及退出再次通过；Key version 1、
active 凭据不变，没有擦 NVS 或清授权。COM3/Wio 未刷写。仅更新本机测试网络地址/同 CA
证书和监听，原服务镜像/数据保留，不展开服务器业务维护。详见
[首次进入实测与下一包](DEVICE_V2_FIRST_CONNECT_OPTIMIZATION_REPORT.md)。

下一步固定为纯 BLE 的小型能力缓存协商：三个现场样本约 3.05～3.78 秒，发现约 0.16 秒，
GATT/认证各约 0.8～1.0 秒，Manifest 约 0.67～0.72 秒、State 约 0.24 秒。
先复用原账号/逻辑 ID 缓存，canonical hash 验证、真实认证后 ManifestAccept ACK，仍读 fresh
State；变化/损坏有界回退完整下载。现有 wire 已支持，不新增协议/权限槽，不直接信任本地
缓存为 ready，不删鉴权或全局 30 ms 分片间隔。首次无缓存照常下载。

本轮不宣称纯 BLE 已提速，不覆盖 Wio BLE/新网关/LAN 发布/100 次循环/全部无缝 SLA。
证据 `build/first-connect-20260918/verified-summary.json` 已机器核验；未暂存、提交或推送。

## 25. Manifest 缓存协商及页面恢复闭环（2026-09-18）

§24 的下一小包已实现，不再把“真实失效 BLE 候选对照”列为待办；该项仍以 371 ms Cloud
先于 5,294 ms BLE 失败的原始证据为准。C3 冻结基础矩阵不变。

本包仅在 App 既有 BLE 入口注入账号/服务/逻辑 ID 隔离的 Manifest 候选，协议逻辑放在
BLE/LAN 共用 `DirectDeviceSession`。Hello 暴露已有 revision/fingerprint；认证之后才加载
候选，复用原 codec/Store 重新 canonical 编码、校验字段及 SHA-256，再发送原
ManifestAccept 并等待设备真实 ACK，最后照常读取 fresh State。下载/缓存共用一个接受函数。
无缓存、摘要变化或损坏走原下载；ManifestConflict 只回退一次；权限/传输/ACK 超时不当成
缓存失效吞掉。每连接候选只消费一次，显式刷新强制完整路径。没有新增设备 wire/凭据槽/
扫描器/选路器，也没有改全平台 BLE 分片间隔；LAN/Cloud 的生产缓存接线没有因此扩项。

损坏缓存实物测试发现旧预览布局错误在能力恢复后仍残留。仅修当前页面恢复边界：异步布局
结果用当前 schema 校验，成功后只清除对应的旧布局校验错误，不清除权限/连接/布局网络错误。
原失败样本保留；当时属性已 ACK，测试在按钮前停止，未重试 Action。

最终同 COM11 原固件/原授权、同 App APK：

- 损坏缓存：ManifestRequest → ManifestAccept ACK → fresh State，Manifest 717 ms，
  ready 3,322 ms；属性/按钮计数 5→6、页面错误恢复、退出回收通过。
- 缓存修复后：无 ManifestRequest，仍有 ManifestAccept ACK → fresh State；Manifest
  481 ms，ready 2,958 ms；属性/按钮计数 6→7、退出回收通过。该能力协商样本减少 236 ms，
  不能把整页 364 ms 差值全部归因于缓存，GATT/认证和无线调度仍有波动。
- 完整实现回归 14 文件 240/240，App/spec 类型及 scoped lint 通过；匹配 APK 真机缓存命中/
  缺失/损坏恢复/快速退出通过。设备拒绝、固件摘要变化、丢 ACK、账号切换和异步 digest
  取消为软件回归，本轮没有伪装成相应设备故障真机证据。

证据与边界见 [首次进入记录 §6](DEVICE_V2_FIRST_CONNECT_OPTIMIZATION_REPORT.md#6-manifest-缓存协商实现与同设备闭环2026-09-18)。
没有刷写 MCU、清授权、变更服务器业务或提交/暂存/推送。缓存以真实下载自动修复，手机回首页。
本包可以关闭；**首次从未缓存的连接仍须完整下载，BLE 全流程仍约 3 秒，不承诺秒开**。
若再优化，先拆分 GATT 服务发现/MTU/连接参数和认证往返，不凭总耗时删除安全或固定等待。
C4 持续暂停；Wio BLE、网关/LAN 新组合、100 次循环等后置项不因本包通过自动开启。
