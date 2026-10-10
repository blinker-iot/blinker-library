# B2b-11～13：混合设备部署、双通道与授权生命周期真机报告

归属 [owner 实施方案](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md) §19～21。2026-09-12～13；最新结果见 §7。仅本机测试，远端未部署，不代表 B2b 全部验收。

## 1. 部署与保护范围

- 原 Gateway schema 37 备份为 custom archive：2,525,271 B，SHA256 `d68605189e9460e910cf80f0fb91a17107598997741e309624f87dc539af167f`。位于 ignored `build/b2b-rollout-20260912/gateway-before.dump`，包含真实数据，不暂存/推送。
- 完整恢复到独立临时数据库后运行 038，再重跑原迁移器。44 张原有业务表逐行规范化摘要/数量保持；只允许 Presence request 新 epoch 回填，已精确校验；10 条 active 实例视图结果保持，第二遍迁移零写。临时演练库已回收，备份保留；没有通过回滚业务库恢复授权。
- 本机镜像 `blinker-gateway:b2b11-release-20260912` 只覆盖 8 个关联编译模块：migration registry、038、removal、ble-enrollment、direct-access、direct-access-http、repository、presence-keys。实际 schema 38、8 个 SHA 和 healthy 已核对。Broker/Auth/环境开关未修改或重启；不部署 unrelated/stale dist 文件。
- App 十文件 116/116、App/spec/Gateway 类型检查、local 构建和 Android debug 构建通过；APK 保留数据安装，SHA256 `9559e3dff41aa1d248215b10ab73ad629fc0b8671c994b72fabf00ffab229ed1`。原账号和 16 个设备库存保留。Service PG 十一个文件重新运行 65/65、0 skipped（不是重复引用上批结果）。
- 页面、设置确认及状态提示纠正“重配网永不刷新 Key”的旧文案：network-only 保留根；全接入 reset 后更新 Key/本地根但保留 ID/页面。没有新 UI 状态机或登录协议。

## 2. 精确现场

| 角色 | 本批处理 |
| --- | --- |
| COM11 / XIAO ESP32-S3 / MAC `8C:BF:EA:1B:96:14` | 专用测试板；先核对 MAC、备份 system 64 KB 和 app0 3,342,336 B，再仅写 app0。未擦 NVS、未恢复旧 NVS，也未清历史 Hub |
| 当前逻辑设备 | `ble__Z_ZA8SUGgyIiqkfAf3ZBw`，`B2b Temporary Removal COM11`；实例 `4b3efcd46ff96acc1908f1affefda192`、原 epoch `1360873493` |
| 历史 Hub | 独立逻辑身份/存储；本批只读确认当前测试逻辑 ID 无 topology 历史，不按板上曾运行 Hub 就修改/清除历史行 |
| COM3 WiFi 见证 / COM12 LAN 基线 | 不刷写或清授权，库存及服务器 active 保留 |
| 手机 / 网络 | ADB `196f65f9`、原 debug 账号；PC `带翅膀的蜗牛`、手机同子网 `_Wi-Fi5`，设备目标 2.4 GHz；密码不进入文档 |

测试 Sketch 是 ignored 的公开 API `BlinkerWiFiBLE.h` 例程副本，只有 Property/Action、显式串口 s/n/r；不自动 reset、不连续串口上报。使用本机既有 3.3.11 / IDF NimBLE 配置，不宣称任意官方 SDK 配置均已过门禁。TLS 要求真实 CA 校验，采用编译时授时夹具、不跳过验证；B2b-12 查明本批配置实际误用了服务端叶证书作为 CA，不能将检查叶证书的 IP SAN/有效期等同于信任链正确，见 §6。这不是 B3 生产授时实现，也不是默认明文发布矩阵。

## 3. 已取得的真实证据

1. 新 App 经原账户 GET 读取 Admin，返回 epoch `1360873493`，与 OS secure store 的 active/enrollment 实例、epoch 一致。未读取或输出 raw secret。
2. 手机扫描仅选精确测试广播，再使用 GetInfo 核对实例。广播 `BLINKER_4B3EFCD46FF9`，Security1 原生连接成功；GetInfo flags=6（原根在、无 DeviceKey、支持 Bootstrap），证明本次改烧混合固件保留了原实例和访问根。MAC 仅用于实验室选板，不替代产品认证。
3. 原页面确认“为已有 BLE 设备启用 WiFi”，真实 enable-cloud **1 次、create 0 次、rotate 0 次**；成功后自动进入原逻辑设备页。服务器原 ID/type/page、epoch 保留，云端版本从 null 变 1；没有新设备、没有伪造 BLE receipt。
4. 保留的原 BLE credential 通过混合固件 Direct 认证/控制：ready **3,211 ms**，Property **263 ms**、Action **313 ms**。这是直接调用真实 App BLE 服务的短门禁，不等于自动 BLE-first 页面已通过。
5. COM3 在 Gateway 部署后实际 Manifest/State 与 Property/Action ACK **114/142 ms**；新 APK 安装后再测 **165/117 ms**。各次测试原账户 MQTT 不重连，observer 已释放；APK 安装本身会重启 App，不宣称安装跨进程不断 MQTT。

## 4. 本批发现与尚未通过

- 初始测试固件错误指向 PC 未发布的 7000 端口，实际 Gateway HTTP 映射仅 loopback 17001，设备应走既有 TLS 17000。已修正配置、核对 CA 的 IP SAN/有效期并实际编译/仅 app0 刷写重启；程序 1,368,828 B、静态 RAM 56,108 B，bin SHA256 `e4dd5bb011aae9dd6ee031509a2f71690ce8f03780823544213e02e54eaa4f5c`。网络 ready、WiFi 状态 3/IP `.63`、local ready 均成立，但 cloud ready 仍 0、Starting/error 0；精确逻辑 ID 仍无 challenge/session。端口配置并非全部原因，不能宣布 MQTT 登录或重启云恢复通过。
- 自动进入原页已过，但初次页面停在 cloud lane、旧 Manifest 缓存且 State 不 fresh。已定位并修复两处 `deviceType=ble && cloudEnabled` 将 BLE-origin 原生 WiFi 误判成网关子设备的问题；真实自动页面复测见 §5，不用手动短门禁替代。
- 首次自动点击确认早于 Ionic 完成呈现，测试等待超时；检查原操作后点击同一已呈现按钮，原操作继续成功。没有重发 enable/create/rotate；CDP 超时不是事务取消，工具必须检查在途操作。
- 应用分区备份最初 180 s 到约 80% 超时，未刷写；独立确认进程退出后延长到 360 s 并完整重读成功。该备份工具超时不是 MCU 存储损坏。
- 本批未执行 resetAccess、Admin PATCH 换根、联合 owner 移除或旧凭据拒绝真机。首次 create→pending 窗口仍按 §18 单列，不宣布 B2b 全完成。

原始私有证据集中于 `build/b2b-rollout-20260912/`；前后 COM3 witness 复用原工具，时间戳结果位于 `build/b2b-live-20260912/`。无暂存、提交或推送。

## 5. 跨日补测：自动 BLE 页面及显式拓扑路由

Service 使用原 `listCloudCapableChildren` 按 tenant、每批最多 100 个 ID 查询**全部**已授权目录设备（原先跳过已有 Key 的设备），给 owner/read/resolve/shared enrich 返回 `gatewayRouted` 布尔值。它表示当前有效网关路由，不是授权、占有射频、设备在线或 Direct Ready 证明；自身 Key 与网关拓扑可共存，不能以 Key/类型/MAC 排除拓扑。

App 页面选择器与 BLE 命令/Presence 维护共用一个小纯函数，删除重复错误判断。离线目录仅多保存此非秘密布尔字段，旧目录缺失时保持原保守准入，错误类型拒收；实际 credential/Method 2/会话准入照旧。未添加状态机或改变 BBP/2、Bootstrap、BLE 安全协议。B4 LAN 的 `deviceType !== ble` 产品筛选仍需单列核对，不借此次 BLE 修复宣布所有通道已按能力接入。

- App **18 文件 223/223**、App/spec/Gateway 类型、local/Android debug 构建通过；PG **65/65、0 skipped 再次重跑**。既有网关子设备在线/离线/取消门禁保留，增加 BLE-origin 原生 WiFi、网关与原生 WiFi 共存、WiFiProv handoff、原命令和缓存投影回归。
- 最终本机镜像 `blinker-gateway:b2b11-routing-20260912` 在前述 8 模块基础上只增加 `device-v2/management.js`；schema 仍 38。实际源码 SHA/healthy 核对通过，Broker/Auth/环境未改。中间私有打包脚本误将 Buffer 序列化成 JSON 导致 Gateway 启动失败：立即恢复上一镜像（不回滚库），修正脚本、对实际产物 `node --check` 后重建并恢复；失败与修正证据均保留，不记作产品协议 bug。
- 最终 APK SHA256 `f8a267f64760c9c275f002cbb09356f434656d5652663361b7d37c7fbd9ca1ac` 保留数据安装。真实 HTTP 刷新得到原设备 `type=ble/cloudEnabled=true/gatewayRouted=false`；**页面自身自动连接** BLE，fresh Manifest/State，ready **3,638 ms**。调用原页面开关/Action handler：**334/350 ms**，power=true、buttonPresses 0→1；不是直接调用 BLE service 绕过页面路由。
- 离开设备页后实际 BLE 从 ready→nearby→stopped，页面已销毁。此处没有精确测量 release 延迟；不能将导航 Promise 返回时间当成底层已断开。
- 后续 COM3 MQTT 见证重跑未 ready，`DeviceV2ReadyWaitError`，没有发送 Property/Action，finally 已关闭 observer；目录显示 COM3 cloudReachable=false，COM12 true。此前两次成功见证仍是历史证据，**不是最终部署后的成功**。保留两块板，不清授权、不重刷来掩盖退化，下一包需同时核对见证离线原因。

下一包只处理当前混合云登录断点及 COM3 见证恢复：逐段区分 HTTP/TLS/返回码、session provider 的等待/退避/错误和网络状态，诊断不得输出 Key/凭据；确认 MQTT/Direct/重启稳定后再 resetAccess→同 ID、新 Key/Admin→旧凭据拒绝/联合 owner 移除。首次 create→pending 窗口继续单列，不重复 038 迁移、enable-cloud 或创建逻辑设备，不恢复旧 NVS。

## 6. B2b-12：信任链对照、恢复错误分类与原见证恢复（2026-09-13）

### 6.1 两条故障分开归因

COM11 的本机配置把 TLS 服务端叶证书作为 CA；实际签发根位于本机旧 TLS 夹具 `build/lan-live-20260909/tls/ca.pem`。OpenSSL 用根验证当前叶证书和 `192.168.2.76` 成功；主机 HTTPS 对照：叶证书作 CA 为 `UNABLE_TO_VERIFY_LEAF_SIGNATURE`，正确根可收到 HTTP。XIAO 仅写 app0 后重复同一组对照：TCP 成功 **7 ms**，叶证书校验失败 **-9984 / 95 ms**，正确根 TLS 成功 **405 ms**、`GET /health` 返回 **404**。404 只证明 TLS 后真实 HTTP 返回，**不是服务健康接口通过，也不是 DeviceKey 登录完成**。没有关闭证书验证或修改服务器证书；MAC 只作刷板保护。

COM3 原 session 已过期、设备仍能响应 IP 探测，没有新 challenge。仅 `read-mac` 引发普通重启（未刷写/擦授权）后，同一 Key version 1 重新登录，原账户 MQTT 上 Property/Action **143/169 ms**；随后自然运行约 8 分钟自动续期，再测 **172/94 ms**，各次短门禁原账户 MQTT 不重连。最终已观察三次自然续期（未升级其固件），收尾见证 **243/203 ms**。现有固件编译时间晚于此前 session 过期修正，不能直接归咎“用了老固件”。**重启后正常续期已过；此前停止重试的现场根因没有被日志捕获，仍不宣称全部故障自动恢复已验收。** COM12 基线保持。

### 6.2 库内最小修正及边界

- `ManagedMqttTransport` 透传 session/MQTT 的 Backoff 原因；MQTT 在线 socket 丢失时记录 `NotConnected`。`WifiCloudLifecycle` 在可恢复 CloudConnecting 阶段保留退避错误，而不是一直 error=0；仍为 Starting，继续轮询共享 Client/BLE，恢复 Online 时清除。状态合同明确不能只凭非零 lastError 判定终止。
- `HttpDeviceKeySessionProvider` 将 HTTP 传输层 `TruncatedInput` 归为原请求退避重试。Arduino HTTP 在 Content-Length/body 或 headers 未收完整时断开会产生此错误，原分类会终止后续登录。这是代码/回归证明的缺陷，**不是对 COM3 现场原因的推定**。
- 完整 HTTP 200 中损坏的 CBOR 仍由 decoder 路径终止；明确鉴权/轮换/撤销仍按原规则失败关闭。未放宽证书、身份校验或幂等性，未新增协议、缓冲区、日志队列、常驻字段或组件 API。
- 主机 **58/58** 通过（现有 Debug/ASAN profile），覆盖 challenge/session 两阶段原请求逐字重试、完整损坏 CBOR 不重试、网络恢复清错及 BLE 不被云端退避停止。测试 double 的 network setter 修为与真实 transport 一样对同值调用幂等，避免把模拟 Backoff 错改 Online。

诊断应用程序大小 1,372,408 B、静态 RAM 56,108 B，后者与 B2b-11 相同；保留探测二进制 SHA256 `57e0e3c00b89edf8c99235067f79e4bde60f4edddbaa559c4033b9ad8a47292b`（bin 1,372,560 B）。错误 CA 下真实状态现在显示 `Starting/NotConnected`、network/local ready，验证诊断透传不再静默。临时 `t`/TLS 探测代码和重复 CA 头已从继续使用的 Sketch 中移除，探测源码/二进制只留 ignored 证据目录。

本批证据：ignored `build/b2b-cloud-20260913/`（原日志/构建/刷写/会话/根证书对照），COM3 原工具结果仍在 `build/b2b-live-20260912/`。没有重新部署 Gateway/Broker/Auth、安装 APK、重复 enable-cloud、创建/轮换 Key、清根、Admin PATCH 或 owner DELETE；不回滚 NVS/业务数据库。实际 DeviceKey/MQTT 恢复以本节后续运行结果为准，不能由上述无凭据 GET 推断。

### 6.3 正确根证书下的双通道与普通重启闭环

最终公开 API Sketch 已移除 TLS 对照函数、`t` 命令和重复 `TestCa.h`，只保留显式 `s` 非秘密状态查询；没有加入自动 reset 或串口连续输出。实际程序 **1,368,248 B**、静态 RAM **56,108 B**，bin 1,368,400 B / SHA256 `c8166041fa643dbd3989e75f6b7e49997c7e123b7c3db42a44fd4be79da936b0`。只写 COM11 app0，保留原 NVS。

| 真机门禁 | 首次修正配置后 | 普通重启后（不刷写） |
| --- | --- | --- |
| 实际 DeviceKey challenge/session | consumed + active，version 1 / shard 1 | 新 challenge/session，仍 version 1 / shard 1 |
| 公共状态 | network/local/cloud ready，Online / error 0 | 同样恢复 |
| 原 App 账户 MQTT Property / Action ACK | **80 / 64 ms** | **138 / 91 ms** |
| 自动 BLE-first 页面 ready | **3,428 ms** | **3,395 ms** |
| 页面 BLE Property / Action ACK | **259 / 282 ms** | **312 / 307 ms** |
| 新鲜数据 | 云端 Manifest/State、power false→true、button 0→1；BLE 页面读取 1 后操作到 2 | 同样从 MCU 运行时计数 0 开始，云端到 1、BLE 到 2 |
| 离页资源 | 后续 inspect 确认 page=null、BLE=stopped | 同样确认；不把导航 Promise 返回时间当物理释放时间 |

MQTT 门禁使用 App 已有账户连接，观测器均在 finally 释放；open/sample/action/inspect 时 stateFresh=true，close 后 false 是主动释放后的正常状态，不是控制失效。BLE 调用真实设备页 handler 和页面自动路由，没有绕过页面直接选 BLE 服务。未测新型无缝切换算法或无线丢包长稳；这些结果仅覆盖本表明确场景。

精确末态：原逻辑 ID `ble__Z_ZA8SUGgyIiqkfAf3ZBw`、实例 `4b3efcd46ff96acc1908f1affefda192`、epoch `1360873493`、Admin version 1 / DeviceKey version 1 均保持；无新 reset/topology 行，原账号仍 16 个设备。COM11 在本批产生 2 个真实 session，COM3/COM12 保持在线。原工具云端结果和最终只读 audit 存在 `build/b2b-rollout-20260912/`，新页面/刷写/会话/编译记录存在本批目录。**B2b-11 的当前云登录断点已关闭、COM3 见证已恢复，不再重复 CA 诊断/enable-cloud/038 部署作为下一步。**

### 6.4 下一包与未通过项

1. 只对当前专用 COM11 做混合设备 resetAccess→同 ID/页面恢复：先验证空根/原实例，再走 prepared→单次 Key 轮换→key-ready→条件 Admin PATCH→Bootstrap/ACK→active；核对旧 Key/session 和旧 Controller 不恢复，新 MQTT/Direct/重启可用。
2. 再测混合 owner 联合移除和另一物理设备见证隔离。云端撤销完成不等于 Flash 已清，本地 reset 的边界按原合同验证；不清历史 Hub、不恢复旧 NVS/数据库。
3. 首次 create→pending 分配窗口、COM3 历史退化未捕获根因及未执行的故障组合继续明确保留；本批没有通过丢包注入证明所有自动恢复场景。
4. 混合主路径完成后回 B1 单 ZIP/平台矩阵→B3 授时→B4 LAN 产品化；BLE-origin WiFi 的 LAN 旧类型筛选仍在 B4，不扩 Hub/S4 或百轮。

本批生产修改仅设备库错误分类/状态透传及状态合同注释；App/Service 沿用 B2b-11 已部署版本，未改生产代码。主机最终 58/58 重跑通过；未暂存、提交或推送。

## 7. B2b-13：同 ID 换根、旧授权拒绝与联合移除（2026-09-13）

本节取代 §6.4 中尚待执行的混合主路径；首次分配窗口及未测故障组合不因此关闭。只处理原专用 COM11，COM3/COM12 保持见证，不刷写/清除两块见证，不清历史 Hub，不恢复 NVS/数据库。网络核对为「带翅膀的蜗牛」，秘密只留本地 ignored 夹具。

### 7.1 同逻辑身份、不同授权版本

公开 WiFi+BLE API 测试 Sketch 只增加显式串口 `r → Blinker.resetAccess()`，不自动清根，不添加连续日志；生产 MCU 源码沿用 B2b-12。只写 COM11 app0：program **1,368,536 B**、静态 RAM **56,108 B**；bin 1,368,688 B，SHA256 `2983bf8ac19300c3baa62ba8c0a8149b65df47c2681c6e11f59268c720bb2889`。SDK 为本机 ESP32 Arduino 3.3.11 / IDF NimBLE 配置，不能据此宣称默认全部 SDK 已通过。

1. 显式 resetAccess 成功，Security1 下 GetInfo：实例仍 `4b3efcd46ff96acc1908f1affefda192`，flags=2（支持 Bootstrap，无 Key/根），epoch=0。
2. 真实重新配网页面复用 `ble__Z_ZA8SUGgyIiqkfAf3ZBw`：**1 次 rotate、1 次条件 Admin PATCH、0 create/register/enable**；prepared/key-ready/Bootstrap/active 使用原实现。DeviceKey **1→2**，访问 epoch **1360873493→1360873494**、Admin version 仍 1；库存仍 16，成功后进入原控制页。
3. 旧 Controller 通过原 Direct 客户端发起认证，捕获精确 Method 2 / seq 2 Rejected wire。不是把发现超时或断开当作鉴权拒绝。
4. 使用原 owner 正常 reveal 的旧 Key，在 MCU 已停云之后通过真实 challenge/session 得到 version 1 的短期 MQTT 登录，作为有界失效见证。轮换时连接被 Broker 断开；**未到期**重连得到 **CONNACK 5**；旧 Key 的新 challenge 得到 **403 / DeviceKeyRotated(3101) / AuthorizationRequired(4)**。没有伪造令牌/绕过 TLS，亦没有与在线 MCU 抢同一 clientId。

| 新授权下真实控制 | 换根后 | 普通重启后（不刷写/不清授权） |
| --- | --- | --- |
| 自动 BLE-first 页面 ready | 4,073 ms | 3,382 ms |
| BLE Property / Action | 275 / 295 ms | 366 / 386 ms |
| 原 App 账户 MQTT Property / Action | 214 / 240 ms | 89 / 147 ms |
| Manifest / State | fresh；按钮计数随真实控制递增 | fresh；MCU 运行时计数归零后重新递增 |

控制均使用真实页面 handler 或原账户 MQTT/观察租约；观察结束后主动释放，close 后 fresh=false 不算故障。普通重启后仍用新根/Key 2，无第二次 rotate/PATCH/新建。

### 7.2 联合 owner 移除、云端退休与物理清除分别验收

- 原 App 设置页确认删除，首次 **202**：`cloudAuthorization=revoked`、`localAccess=not_confirmed`；卡片消失、显示 Key 清空。COM3 在删除前后 **24 次顺序 Property** 全部 ACK、原账户 MQTT session 不变；COM12 保持在线。不是为每个设备另建 App MQTT。
- Gateway 的 rotation/revocation 工作均 delivered；最后一项 device_revoked 的真实 Broker 回执距删除提交约 **120 ms**。目标 session 的 version 1/2 全部 revoked，Broker 目标 `revoked / mqtt_online=0`、版本增至退休界限 3；两块见证仍 active/online。
- 针对刚退休的 **Key 2 locator** 发新 challenge：**403 / DeviceRevoked(3106) / AuthorizationRequired(4)**。在已知 202 完成和 worker 回执后，显式再次调用原删除接口检验幂等性，返回 **200 / brokerCleanupPending=false**，deletedAt/退休版本不再次改变；不是失败后盲目重发 DELETE。
- `v2_active_direct_instances` 不再包含目标。Direct metadata/receipt 保留作审计，不应要求删除审计行；真实 owner metadata GET 返回 **200 / record=null**，不再用于准入。测试夹具曾把“审计行仍在”或“GET 没抛异常”误判为失败，核对正式合同后修正断言；没有为迁就夹具删除服务端数据。
- **混合产品现有鉴权边界必须明确**：`WifiCloudLifecycle` 收到终止性的 `AuthenticationRequired` 会停共享 Client/WiFi，实测 `Fault / error 23`、network/local/cloud 均不 ready。因此本批云端删除后的再次 Direct 发现未进入认证，**不算旧 Controller 的 wire 拒绝，也不支持“云端删除后 BLE 仍然可控”的结论**；§7.1 的旧根拒绝是此前独立的真实认证证据。这个失败关闭策略并未执行 Flash 擦除；网络退避继续本地轮询与授权失败是两种不同边界，本批不放宽它。更细的独立本地授权策略留 B2c，不临时改协议。
- 随后对已删除的精确 COM11 **另行显式 resetAccess**，独立检查点确认 `ACCESS_RESET_OK`。新版 App 经真实 WiFiProv Security1 再读 GetInfo：同实例、flags=2、epoch=0。由此确认最终空根/待配网；没有重新分配身份。云端 HTTP 结果仍诚实保持 `localAccess=not_confirmed`，因为没有向服务器提交设备擦除证明。

清理后的目标原 ID 已退休、原授权不恢复，后续要重新配网/认领；当前卡片库存 15。只清专用设备访问记录，不做全 Flash 擦除或恢复历史凭据。App 中为拒绝门禁临时保存的旧/新 secret 副本在安装新 APK 前已清零；真实 Key/session 证据只在 ignored 私有目录，不暂存/上传。

### 7.3 配网扫描交接修复与匹配 APK 真机

首次旧 APK 配网扫描出现 `BLE_SCAN_BUSY`，待原扫描结束后手工重新扫描才完成 §7.1。根因不是 MCU 广播缺失：WiFiProv 原生扫描与首页/Direct 共用 Android 单一扫描器，但页面没有参与 JS 协调器。该次手工重试不能当作修复后自动交接通过。

最小修正仅 App `ScanCoordinator.acquireExclusive()` 与原配网页面：取消并等待当前物理 start/stop 排空，交给外部 SDK；持有期间拒绝新 Presence/Direct 观察，页面重复扫描复用该租约。SDK 的迟到 start 必须结束后才能 stop/clear/release，退出/权限等待期间不再迟到启动；物理 close 失败保留隔离，不让不明扫描与后继重叠。没有第二个 scanner、SDK 专属重试队列或新连接协议；去掉页面原来无统一所有权的扫描入口行为。

App **12 文件 135/135**，其中扫描协调器 13 项、页面扫描 6 项；App/spec 类型检查、local 构建、Capacitor copy 和 Android assembleDebug 通过。MCU host Debug/ASAN 本批重跑 **58/58**；Service 生产代码/部署/schema 不改，PG 65/65 属于前批，不冒充本批重跑。

保留账号数据安装新 APK：84,741,177 B，SHA256 `8b4d174422670430c794c3614bb98918e343b4aa81d9818e0ef87a28f2c49657`；原 APK SHA `f8a267f64760c9c275f002cbb09356f434656d5652663361b7d37c7fbd9ca1ac` 已备份。同一手机真实 nativePromise 时间线：原 BlinkerBleScan start 成功→原 stop 成功→WiFiProv start 成功，旧观察收到 `BLE_DIRECT_SCAN_CANCELLED`，发现精确 `BLINKER_4B3EFCD46FF9 / android:8C:BF:EA:1B:96:15`。再经 Security1 读空根；离页 stop/clear 完成后，后继 BlinkerBleScan 才启动并按预算正常结束。后继没有匹配 Direct 设备的 timeout 是有意不选择设备的资源回收门禁，不是 Direct 通信成功证明。

最后 COM3 MQTT Property/Action **201/262 ms**，COM12 在线；手机回设备列表，配网会话和临时观察释放，COM11 留在待配网状态。详细本机证据位于 ignored `build/b2b-rotation-20260913/`，云端/见证复用工具的输出在 `build/b2b-rollout-20260912/` 和 `build/b2b-live-20260912/`。未暂存/提交/推送。

### 7.4 下一步及明确未通过项

**B2b 混合主路径短门禁已完成，下一包回 B1 单 ZIP/默认公开示例/平台编译与当前版本接入矩阵，然后 B3 授时→B4 LAN。** 不再次重复 reset/换根/联合删除或开始百轮。

首次 create→pending 分配窗口、真实任意时点杀进程/丢包、B2c 另一账号/带分享/转交及独立本地授权边界仍未全部闭环；COM3 历史退化原因未被捕获，Wio BLE 发布长稳、BLE-origin LAN 旧类型筛选各保留原门禁。本批没有扩大为全 SDK/全平台发布通过。
