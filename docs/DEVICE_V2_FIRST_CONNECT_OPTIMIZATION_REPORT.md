# Device V2 首次进入：有界优化与真机记录

> **2026-10-13现行入口：[BLE 首连分段优化§6～§7](DEVICE_V2_BLE_FIRST_CONNECT_SEGMENT_EXECUTION_PLAN.md#6-com3-同条件短验收与恢复)。** 四入口窗口收尾／四槽背压回归与 COM3 A/B 两窗通过：同为244 B、缓存命中及fresh State，单对可控2531→2168ms，仅B验证原电源／按钮ACK/State。GATT差异发生在HIGH前，不归因窗口；普通默认不变，原App和COM3原固件／授权已恢复，不追加循环。2026-10-12的20Bpeer A/B/B/A及认证中取消仍取[原核心易用性方案§8.4](DEVICE_V2_CORE_USABILITY_OPTIMIZATION_EXECUTION_PLAN.md#84-有界窗口同设备对照和取消)，不与本轮混算。非BLE展示、日期／星期编辑和七目标交付分别保留该方案§8.6～§8.8、§9.4、§10.6～§10.7原边界；同事独立验证仍单列，本文件下方历史实测不改写，不将有限样本当全平台默认或SLA。

历史记录：2026-09-18；当时主线为 `DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md`。
C3 的冻结基本矩阵已于 §23 / C3 报告 §12.6 收口，本文件不重开平台矩阵、不展开 C4。
最新结论见 §6：真实失效 BLE 候选对照已完成，缓存协商已实现并真机验证；§5 保留实施前依据。

现行 Android 测量口径：BluetoothLe 的 connect Promise 包含 GAP 连接、服务发现和 MTU 交换；getServices 读取已发现的 GATT 列表。不能将后者短时间称为完整服务发现，或将 connect 全窗口都归给物理建链。App 记录开始发送至 receiveRecord 返回的 bootstrap 时间包含 native write 和片间等待，也不是设备端计算/发送的独立耗时。2026-10-12 原 669/5 ms 数值保留，正确名称分别为“原生连接准备全流程”和“GATT 合同列表读取”；后续没有单独观测的子阶段保持未观测。

## 1. 本包合同

普通 Cloud-capable 页面复用唯一账户 MQTT，同时准备既有 Direct 候选：Cloud 就绪即可控制，
不串行等待历史 BLE 凭据对应的失败扫描。只有真实 Direct 认证和同步成功才切换至 BLE；
失败/迟到结果不得覆盖可用通道或自动重放 Action。退出页面取消本地候选并释放页面 interest。
纯 BLE、网关子设备、WiFiProv handoff、一次性读取各自保留原合同，不新增后台预连接。

生产改动已在 9 月 16 日完成于 `DeviceUiPort`，当时 456 项回归与匹配 APK 的两个普通
路径通过。本轮补其缺少的真实故障候选对照，不将测试钩子变成生产连接器。

## 2. 现场与证据边界

- 初始 APK SHA256：`6b45f1875d6d3d3587b5fe27f2469b5aaef0f302d0bb6ff51792d7c833caa4b8`；
  最终修正版为 `3c1a2c75c3256bcb915e3eea828745deb1e88fd9242dc52f8849939cafc85ed6`。
  两次均通过手机 `pm path` 和实际文件 hash 核对，保留数据安装，LAN 默认关闭。
- 专用 COM11 S3：逻辑 ID `device_1c26af38-eed8-48f6-9ee2-d106e22d4d00`；
  active BLE 凭据与 cloud/direct 能力均由原 App 读取。MAC 只用于烧录目标防误操作，不做业务身份。
- 备份原应用整个 3,342,336 B 分区及系统/NVS/OTA 元数据；只允许写应用区，不清权限、不旋转 Key。
- 当前 PC/手机为 mostfun2026；PC DHCP 改为 `.1.141`。仅同步本机 Gateway 的广告/监听地址
  与设备专用 HTTP 代理；固定原镜像/数据，不升级服务业务。旧 compose 的 `.2.76` 监听导致一次
  启动失败，按原运行容器的端口映射修正后恢复。测试 TLS 证书补入新 IP，保留原 CA/叶子
  私钥及 localhost/旧 `.133` SAN，不关闭校验。已删除停用的旧 HTTP 测试代理容器，原
  inspect 配置仍留在私有证据中可恢复；没有删除任何卷或业务数据。
- 临时标准 WiFi 示例沿用该设备原 DeviceKey，建立真实 BLE 不可用/Cloud 可用条件。
  完成后恢复原 WiFi+BLE 应用并再次验证授权和控制；不回写旧 NVS 以伪造绑定。
- 证据目录：`build/first-connect-20260918/`；含私有凭据、备份的目录限制本机 ACL，
  不进入发布 ZIP/版本库。可复用测量入口为 `tests/hardware/first_connect_app_gate.mjs`。

测量包装器仅调用原方法、记录时序/结果，不替换回复或 ready 状态、不导出密钥。
所有时间为有限现场样本，不是 SLA、分位数或旧 APK 的 A/B 性能结论。

## 3. 执行记录

| 项目 | 状态 |
| --- | --- |
| C3 基础矩阵 | 已冻结关闭，不重复执行 |
| 当前相关软件回归 | 初查 61/61；取消修正后 8 文件 142/142，App/spec 类型与定点 lint 通过 |
| 原 BLE 基线及分阶段计时 | 已通过，见下表 |
| 真实历史 BLE 候选不可用 / Cloud 可用 | 通过：初始 APK 254 ms；最终修正版 371 ms，不等待 BLE 约 5.3 s 失败 |
| 候选迟到失败不影响 Cloud，精确 Property/Action | 通过：最终版 170/197 ms，计数 2→3；14 个观察样本持续可控，无自动 Action 重放 |
| 提前退出和再次进入，无本地资源遗留 | 发现取消竞态后修正并通过：退出后约 52 ms 结束候选；100 ms 及 6.5 s 检查均无遗留；重进 155 ms |
| 原 WiFi+BLE 固件/授权恢复 | 原应用整分区 SHA256 验证一致，Key version 1、原 BLE active 凭据保留；BLE 控制/退出通过 |

初始 BLE 页面可控 **3,782 ms**，退出 groups/links=0，账户 MQTT 为原 session。

| 阶段 | 单次耗时 |
| --- | ---: |
| 发现 | 163 ms |
| GATT 连接/服务/通知准备 | 1,038 ms |
| Hello + Method 2 + 安全通道认证 | 1,012 ms |
| Manifest/State 同步 | 1,267 ms |

发现不是此样本的主要瓶颈。该设备仍使用旧测试网络配置，BLE 基线中的 Cloud 设备通道
并不可用；账户 MQTT ready 不等于设备 Cloud ready。表中也不把多个嵌套 ensureReady
调用当作多个原生连接。

## 4. 真实失败、最小修正与最终闭环

初始 APK 已证明 Cloud 首次不被 BLE 阻塞，但提前退出样本暴露真实竞态：
`ensureReady` 在等待后台 Presence 扫描退出时尚未登记 `opening`；页面退出时
`disconnect(id)` 找不到这次尝试，之后递归 `ensureReady` 仍会启动扫描，直到约 5 秒超时。
所以早期仅看 groups/原生 links=0 并不足以证明退出完成，必须同时检查 pending opening。
失败样本 `page-early-exit-cancel-*` 与 `page-reopen-close-*` 保留，不改写成通过。

最小修正在原 `DeviceV2BleService`：将等待 Presence 的八行分支移入已经由 `opening`
追踪的 `open()`，等待结束后再检查账号/取消信号；删除未归属的递归等待分支。
沿用原连接槽、generation、取消和共享 promise，不添加状态机、队列或设备 wire。
两个新增测试先失败，再经修正通过，覆盖等待期间退出及同目标共享同一尝试。

现场另见页面在失败后按 1/2.5/5 秒重试；核对为原有三次有界前台策略，不是无限循环。
本包保留该策略，退出会取消；不能把“第一次候选失败”说成“整页以后不再扫描”。

最终修正版同一账户 MQTT/同一设备：

| 门禁 | 结果 |
| --- | --- |
| 首次 Cloud | 371 ms 可控，BLE 候选 5,294 ms 失败；Property 170 ms、Action 197 ms，计数 2→3 |
| 提前退出 | 进入后 216 ms 离开，候选在第 268 ms 拒绝；退出后 100 ms 无 opening/link/session，6.4 s 后仍无迟到工作 |
| 再次进入 | 155 ms 可控；Property 192 ms、Action 113 ms，计数 3→4；退出回收 119 ms |
| 原固件恢复 | 完整应用区 hash `9f6437590254de2a7f6204139bedb8c21eb6ca742ce976c698899d2ff0642eb2` 与读回备份一致，不回写 NVS、不清授权 |
| 恢复后 Direct | 3,231 ms 可控；Property 297 ms、Action 249 ms，普通重启后计数 0→1；退出回收 117 ms |

恢复后 BLE 拆分：发现 166 ms、GATT 854 ms、认证 927 ms、Manifest 718 ms、State 246 ms。
结合原 3,782/3,049 ms 样本，**未宣称纯 BLE 性能已改善**；发现较快，能力同步值得先评估。
没有修改 30 ms 分片间隔或安全认证。COM11 恢复原测试固件/原网络 profile（该旧 profile 的
Cloud 在本轮基线也不可用），本包仅验证恢复后的 BLE，不把它描述成原固件两通道都已重新联网。
COM3/Wio 未刷写。最终 App 首页、原账号/Key version 1/active BLE 凭据，所有本地资源及钩子为 0。

临时标准 WiFi ZIP 经真实安装、字节/来源/依赖/ELF 检查：program 969,207 B、静态 RAM
49,892 B；这只是本次 S3 Plain profile，不外推新增跨平台资源矩阵。Android 构建成功后
构建守护进程继承输出句柄导致工具迟迟未返回，确认该进程已 idle 后仅停止本轮守护进程，
取得成功结果；不是 App/设备连接故障。

`verify.mjs` 检查原失败、修正回归、实际 ACK/计数、延迟顺序、取消、恢复 hash 和收尾状态，
生成 `verified-summary.json`：`firstConnectHardwarePassed=true`、
`lateScanCancellationFixed=true`、`restorePassed=true`。没有提交/暂存/推送。

## 5. 下一小包的优化依据（先实测，再实施）

优先继续拆分 Manifest 与 State，而非全局删掉 30 ms 分片间隔：当前间隔兼顾 R4/Wio
无线协处理器消费时序，不能用单块 ESP32 通过就改掉所有平台。安全认证不能省略。

可评估的最小方向是复用已验证能力缓存：现有 Hello 已带 revision/fingerprint，但 App
解码后尚未向 Direct 会话暴露；设备现有 ManifestAccept 在授权后精确验证 revision/hash，
不要求本连接先下载 Manifest，因此无需新增 wire 消息或权限槽。实施前必须满足：

1. 缓存按账号/服务/逻辑 ID 隔离，并重新 canonical hash 验证，不仅验证 JSON 外形。
2. Hello 中摘要只作候选，不能代替授权；必须先通过 Method 2，再取得真实 ManifestAccept ACK。
3. 不复制缓存的 accepted/fresh 状态；每条新连接仍获取 fresh State 后才可控。
4. 摘要/版本不符、缓存损坏、设备拒绝时，有界回到原 Manifest 下载路径，不重放 Action。
5. 保持 BLE/LAN 共用 Direct session 的模块边界，避免再写一套缓存、传输或连接状态机。

§4 完成时尚未实施该缓存优化（现已完成，见 §6）。真实 Cloud 对照、取消修正和恢复均通过，下一步进入上述缓存
协商的小型实现/回归，再以同设备作对照；首次无缓存仍须正常完整下载。
不因此扩展网关、LAN 发布、100 次循环、服务器维护或全部无缝 SLA。

## 6. Manifest 缓存协商实现与同设备闭环（2026-09-18）

### 6.1 模块边界与降级规则

1. `direct-hello.ts` 保留已有可选 Manifest 摘要；无摘要旧固件仍完整下载。
2. BLE service 通过唯一 client 工厂注入原 `DeviceV2ManifestCache.load`，加载时再次检查
   账号 session context。缓存仍按账号、服务地址、逻辑 ID 隔离；不读取其他设备/账户的 schema。
3. `codec.decodeCachedManifest` 重建 canonical 字段并复用 wire 解码约束；`DeviceV2Store`
   使用原 SHA-256 验证与副本逻辑。完整支持 256 字段、float64 约束、enum、遥测间隔。
4. `DirectDeviceSession` 每连接只试一次候选；摘要相等且 hash 验证成功才省下载。原
   `acceptManifest` 统一缓存/下载的可靠确认；只有实际 ACK 才 accepted，新 State 才 fresh。
5. 摘要不同、缓存缺失/损坏回完整下载；设备 ManifestConflict 有界重取一次；权限错误、
   unsupported、ACK 丢失、连接关闭不会被当作缓存 miss 吞掉。显式 refresh 不复用旧 Hello。

不新增 MCU 协议或 RAM 对象，不引入第二套缓存/通信状态机；不减少 Method 2 或加密 record。
共享会话可以服务 LAN，但本次仅接线 BLE，未修改 LAN/Cloud 的生产缓存策略。
缓存摘要不是鉴权信息，不能因此离线绕过账号隔离/设备确认，也不缓存 accepted/fresh/value。

### 6.2 实物记录与遇到的故障

证据目录 `build/manifest-cache-20260918/`，沿用 COM11 S3 原 WiFi+BLE 固件、原逻辑 ID、
active BLE 凭据和 mostfun2026 环境。没有刷写、复位或清授权，也没有修改服务器配置。
App 账户 MQTT ready 不代表该旧固件的 Cloud 可用；本次只验证其 BLE 数据通道。

初版缓存 APK `743afa2e…` 的三次缓存命中耗时为 Manifest **540/479/481 ms**，
整页 **3,370/3,047/3,098 ms**；一次无缓存为 **752 ms / 3,349 ms**。
每个命中均真实发送 kind 6（ManifestAccept）、16（StateRequest），没有 kind 2
（ManifestRequest）；确认 ACK 时 `accepted=true,fresh=false`，State 完成才 ready。
测量钩子仅代理原方法并记录 kind/长度/时序，不伪造帧、ACK、授权或状态。

人为仅损坏该账号/设备 Manifest 缓存的一个 key，保持原 fingerprint。协议正确识别 hash
不符并下载；但该缓存此前已用于页面预览，旧 PageLayout 校验错误没有随真实能力恢复清除。
该次属性确已生效，按钮尚未发送即被测试停止；保留 `page-corrupt-commands-*` 和
`diagnostic-*` 失败证据，不通过清 error 或重复 Action 使结果变绿。

最小修正为页面在异步布局返回时使用**当前** schema，而非开始时的快照；成功恢复只清理
对应布局校验错误，不掩盖布局 HTTP 失败或其他错误。四条页面回归覆盖恢复、无关错误和迟到结果。

最终 APK SHA256：`3c325440dd9890b9f99d955f92a44c01abe622632ab7822d563f063fb23d6170`，
保留数据安装，手机实际 `base.apk` hash 相符，LAN 默认开关仍关闭。

| 最终 APK / 同设备 | Manifest | GATT / 认证 | fresh State | 页面 ready | 控制及回收 |
| --- | ---: | ---: | ---: | ---: | --- |
| 损坏缓存自动完整下载 | 717 ms | 727 / 986 ms | 299 ms | 3,322 ms | Property 314 ms / Action 263 ms，计数 5→6；回收 120 ms |
| 随后缓存命中 | 481 ms | 780 / 933 ms | 272 ms | 2,958 ms | Property 308 ms / Action 303 ms，计数 6→7；回收 225 ms |

能力协商该对照减少 236 ms（约 33%）；不能将整页差值全部归因于缓存。首次真正无缓存
仍必须下载，缓存命中也还要 BLE 建链/鉴权/ACK/State，未达到秒开或全部无缝通信 SLA。
初版 3,370 ms 的命中比 3,349 ms 无缓存还慢，明确保留这项抖动而不筛掉不利样本。

### 6.3 完成边界与下一步

- 14 文件 **240/240**，App/spec 类型和 scoped lint 通过；包含摘要变化、256 字段、hash
  损坏、账号/服务隔离、ManifestConflict 一次回退、权限拒绝、可靠 ACK 丢失和 digest 中途关闭。
- 最终 APK 的缓存损坏恢复/命中/属性/按钮/快速退出通过；同一账户 MQTT 复用，原权限未变，
  缓存已与测试前内容一致，首页 groups/links/sessions/opening 均为 0。没有依赖手动清 error。
- `verify.mjs` 保留并检查原失败、真实帧顺序/ACK、精确按钮计数和当前 APK/最终缓存；
  `verified-summary.json` 是本包机器校验汇总。没有提交、暂存或推送。
- 摘要改变/设备拒绝/丢 ACK 等是**软件回归**，不是本轮固件故障注入实物测试；无缓存首次
  认领、R4/Wio BLE 性能、LAN/Cloud 缓存接线及生产发布不从当前 S3 结果外推。

本包收口。若继续首次连接优化，仅先测 GATT 与认证的具体往返、MTU 和连接参数，不增加
后台全设备预连接、不去掉权限校验、不全局改变 30 ms 分片间隔；C4 和新功能维持暂停边界。
