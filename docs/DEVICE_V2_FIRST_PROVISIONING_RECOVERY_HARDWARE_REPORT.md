# C1：ESP32 首次 WiFiProv 配网中断恢复真机报告

2026-09-14。当前执行顺序见[核心稳定版主线](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md#8-下一步固定为-c1-d分配记录退休而非无缝扩展)。本报告记录有限故障窗口，不代表所有接入生命周期或任意网络故障已经通过。

**最新状态：C1-d 已按 §6 完成正常退休/新认领与本机清理失败后的跨进程恢复。§1～§5 保留前批“尚缺退休”的真实发现，不再作为当前待办；下一步为[主线 C2](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md#9-c1-d-完成与-c2-定点验收入口2026-09-14)。**

## 1. 范围与结论

三类真实进程中断已验证：CREATE 成功/context 未保存、context 已保存/pending Admin 未保存、Bootstrap 已确认/首次 WiFi 未发送。恢复沿用精确逻辑 ID，未重复建号；随后原 App 页面 BLE Property/Action/State、原账户 MQTT 控制、MCU 普通重启和 App 冷启动通过。

真机暴露并修正了一个纯软件正常流程看不见的边界：Bootstrap 成功不等于服务器已建立 instance 关联，不能此时删除首次分配 journal；历史 Admin 记录不一定唯一。最终原生 journal 保留精确 ID供原 Admin 协调器验证，不扫描后任取历史身份。

**仍未收口：** 关联较晚时 journal 会保留，目前缺与明确移除/重新认领的退休联动。最终现场确实保留了该记录，没有手动删除伪装通过。下一包固定为 C1-d，不将本报告作为 C1 全包/发布完成声明。

## 2. 环境与隔离

| 项目 | 本次范围 |
| --- | --- |
| 被测板 | COM11 XIAO ESP32S3，物理 MAC `8c:bf:ea:1b:96:14`，GetInfo instance `4b3efcd46ff96acc1908f1affefda192` |
| 身份使用 | MAC/广播名只用于现场选板，实际恢复核对 GetInfo instance、账号、逻辑 ID、epoch、当前 Key context；不以 MAC 作业务 ID |
| 手机/App | 已有 Android 手机，保留 App 数据更新 debug APK；真实原生 Security1、secure-storage 与原页面/API |
| 网络 | 电脑/设备接“带翅膀的蜗牛”，手机同局域网 Wi-Fi5 接入；密码不进入本报告或源码 |
| 见证 | COM3 `device_92184181-cbe6-4503-b152-649c8537060c`、COM12 `device_15e78609-bdb8-482e-90d4-3df3997393b0`；原 MQTT 控制前后通过 |
| SDK/固件 | 现有 Arduino ESP32 3.3.11 自定义 IDF NimBLE profile；不证明任意 stock SDK。现有 WiFiBLE 组合，无新增设备生产代码 |
| 服务 | 使用当前本机 Gateway/Broker；原 DELETE/CREATE/REVEAL/Admin/resolve-instance，SQL 仅只读审计；不部署/迁移或直接改库 |

设备应用区备份后仅写 app0（`0x10000`）并 verify，不改分区、不整片擦除/NVS 恢复。程序 1,369,636 B、静态 RAM 56,108 B；bin 1,369,792 B，SHA-256 `6aa862356eb26c5e78e708da01809a64823a43426d08880f6abfb826a0b7c319`。资源数据不是运行期峰值。

## 3. 故障注入和实际结果

测试在真实操作返回处暂停，然后由 ADB `am force-stop` 结束整个 App 进程；新进程重新进入原配网页面。拦截器仅记录脱敏事件并调用原方法，不伪造 HTTP 成功、设备 ACK、原生持久化或业务状态；不进入发布源码。

| 窗口 | 实际证据 | 结论 |
| --- | --- | --- |
| CREATE 201 后，context bind 前终止 | 重启 request/name 不变，原 Service 返回 200、同 `device_9a0143e4-eebc-4130-8311-7123d55bd61c` | 幂等重放通过 |
| context 已写，首个 pending save 前再次终止 | 第二次重启直接 REVEAL，零 CREATE；原登记、Bootstrap、active、WiFi/MQTT/BLE 完成 | 恢复通过 |
| 中间候选：Bootstrap 后清 journal、WiFi 前终止 | 实际 ACK：operation 3/status 0；GetInfo 有 Key/access、epoch 1；resolve-instance 无结果；历史记录导致 `BLE_DIRECT_PENDING_AMBIGUOUS` | **失败保留，不算通过**；不能以删除历史缓存解决 |
| 最终版：Bootstrap ACK、native active 后、WiFi 前终止 | 原 journal 指向 `device_215c0f9e-b44b-4ef8-8cc9-60a7daecf45c`；服务器无关联/无该设备 session；未指定 ID 的历史查询确实 ambiguous | 前置故障条件成立 |
| 最终版进程重启恢复 | 读取上述精确 ID；原 REVEAL/Admin 幂等确认；零 CREATE、零重复 Bootstrap（仅 GetInfo）；完成 WiFi并自动进入对应控制页 | 恢复通过 |

前两窗是初版 APK `c881d4a70672e5ac90b2497c5f87c4bb7bcb471b529571cb38aeb694ffb62757`；第三窗最终通过 APK 为 `58e745219382b5d450ef00846f1cc36b4f3ae4b51fba60d6f46718d715b93a1a`。最终版保留前两窗分配实现并跑确定性回归，**未把两版真机结果描述成同一 APK 全矩阵**。

最小生产修订为 App 原 allocation store 新增只读 `load()`，页面使用 journal exact ID并延后 best-effort 清理；删除旧“Bootstrap 后立即清 journal”路径。原 `WiFiProvAdmin` 继续唯一负责接入根；Service 首次登记并不自动填 `v2_active_direct_instances`，首次 cloud session 前 resolve 无结果符合现有实现。无新 MCU wire、权限槽或服务 API。

最终软件回归 **9 文件 131/131**，包括 journal 20 项和原页面 33 项，以及原 Admin/Cloud-only/账号保护回归；App/spec TypeScript、五个 TS 文件 ESLint、Angular local build/Capacitor copy/Android `assembleDebug` 通过。收尾再经原 `ng test --configuration=ci --include=…` 复跑同九文件及类型/lint，仍全部通过。不要直接用裸 Vitest CLI 替代 Angular unit-test builder：它不加载项目的 Angular setup/globals，收尾一次误用产生四个套件初始化错误；纠正入口后未修改业务源码。APK 保持 `lanEnabled=false`，沿用既有本机 CA 校验，没有忽略 TLS 校验。中间候选的 127 项软件成功不能替代其失败的真机结果。

## 4. 最终版本控制、重启及清理

以下为现场单次耗时，不是 SLA/分位数；BLE page ready 包含重新建链及状态读取，不能与已连接命令延迟混用。

| 样本 | BLE Property / Action | MQTT Property ACK | 实际 Action 计数 |
| --- | --- | --- | --- |
| 最终窗口恢复后 | 280 / 311 ms | 124 ms | BLE 4→5，MQTT 5→6（本轮 resetAccess 未重启应用计数） |
| MCU 普通重启 | 250 / 270 ms | 150 ms | 重启 0，BLE→1，MQTT→2 |
| App 冷启动 | 272 / 279 ms | 196 ms | BLE 2→3，MQTT 3→4 |

普通 MCU 重启后打开 BLE 页面 ready 3,753 ms，App 冷启动后进入页面 3,476 ms；首次恢复后观察到页面剩余 ready 等待 8,936 ms，没有当作“无缝通信已经实现”。当前包只验恢复正确性，不跑 100 次循环或扩连接抢占。

每次 BLE 页面退出均回收控制连接至 0，nearby 只是可发现状态。MQTT 测试复用 App 用户级会话，在每次测试内保持同一会话；不表示 App 被强制杀进程后仍是同一 MQTT session。最后 COM3/COM12 Property ACK 166/146 ms，Action/State 均通过，原测试 owner 已释放。

最终只读核验：同一新逻辑 ID，Key version 1、Admin epoch 1、原生 `source=wifiprov/state=active`，无 keyRotation；Service 恰有两条该版本登录 session（首次联网＋普通 MCU 重启），当前实例关联精确匹配。库存 16、当前测试名称精确匹配 1 项；首页、控制连接 0、账户 MQTT ready，无测试拦截器。串口 `NETWORK_READY=1`、`LOCAL_READY=1`、`CLOUD_READY=1`、`LIFECYCLE_ERROR=0`。

进入控制页前的唯一清理检查早于 cloud session，**journal 最后仍存在且 ID 精确匹配**。这符合“不能过早丢恢复身份”，不等于自动回收通过。已将明确 owner 移除后的 journal 退休/再次认领列为 C1-d 必须修正项；没有增加等云上线的阻塞等待或后台清缓存定时器。

## 5. 测试数据影响、证据和剩余

为复用唯一专用测试板并构造首次接入，按原 App 设置确认及实际实例/epoch依次退休四个 COM11 逻辑记录：原 `ble_LhR-mYNVmrHJnnavXGVd9A`、原 `device_5f9d3415-8395-4538-bad6-1c2bc5bb28d7`、第一轮 `device_9a0143e4-eebc-4130-8311-7123d55bd61c`、失败候选 `device_55092486-873c-40c9-bb50-629c138b8d46`。其中 HTTP 202 表示云端授权撤销/清理待完成，**不是本地权限清除**；随后另行在已确认的 COM11 执行公开 resetAccess，并用 GetInfo 空根验证。COM3/COM12 未变。

上述旧云端授权不能靠恢复固件备份复活；未恢复旧 NVS 或直接回写数据库。原应用区备份保留，但不把含已退休 Key 的旧 WiFi 固件刷回去。当前 COM11 保留最终 WiFi+BLE 测试固件与在线设备 `C1 WiFiBLE Recovery COM11`。历史 native Admin 记录为复现歧义刻意保留，不批量清除。

本机证据（不作为库发布内容）：

- `blinker-library/build/c1-allocation-live-20260914/`：前两窗、备份/烧录/verify、原见证和第一轮重启。
- `blinker-library/build/c1-network-live-20260914/`：候选第三窗真实失败，含 Bootstrap ACK 与 ambiguous 错误。
- `blinker-library/build/c1-exact-live-20260914/`：最终 `checkpoint.json`、原始页面/控制样本、只读审计、`finished.json`，后者如实记录 journal 保留；`verify.cjs`/`verified.json` 已交叉核对原始证据，明确 `c1Closed=false`，没有把保留记录写成清理通过。
- `blinker-app/build/core-stability-20260914/c1-final.xml`、`build/c1-exact-20260914/`：最终回归/构建。构建时 `apk.json.installed=false` 是构建快照，后续真实安装见设备 evidence 的 `installed.json`。

未暂存、提交、推送；未展开无缝/网关新能力。尚未覆盖 C1-d 退休/重认领、坏密码/AP 故障、Cloud-only Install ACK 丢失、跨安装/清 App 数据/换手机、任意无线丢包、全部 SDK 和 R4/Wio 最终矩阵；这些不从本次有限恢复证据外推。

## 6. C1-d：明确移除、退休重认领及清理失败恢复（2026-09-14）

### 实现和软件验证

增加无 Angular/Capacitor 运行依赖的 `WiFiProvRetirement`，由设置页和首次配网页面共用。管理层新增对既有 owner GET 的严格状态校验，200＋精确 ID＋active/deleted 才是可用结果；没有新 Service API/迁移。存储层新增 scoped 唯一查找 journal，复用原锁和读取/写入；界面不遍历原生存储，未做全局缓存清除。

owner DELETE 200/202 明确云端撤销后，按 journal 精确 ID/instance/source 清理对应本机 WiFiProv Admin，然后删除 journal。若任一步失败，已确认云端删除不会被描述成未删除；提示本机清理待续。账号/页面变化不向新账号执行清理。journal 保留时，下次必须真实 GetInfo 空根、原 owner GET 确认 deleted、用户明确确认后才能幂等重放 DELETE/精确清理并新建；不把 404/超时当删除、不自动旋转 Key、不擦其他 Admin/Presence。

先 5 文件 100/100，补设置页失败/账号切换回归后，完整 **11 文件 172/172**（journal 24、retirement 19、配网页面 34、设置页 16 等）；App/spec 类型、十个 TS 文件 lint 与原 Angular unit-test builder 二次回归通过。debug APK SHA-256 `659a10274e979de270de708dc28a9d695ddf0ee6409ff9385149824870a290c3`、84,741,335 B，保留数据安装，LAN 默认关、既有 CA 校验未绕过。之后唯一类型层整理将协调器依赖由 concrete store type 改为结构接口，不改变已测运行逻辑；再次类型/lint/172 项通过。

设备仍为 §2 的同一 COM11/同 instance，应用固件未重新编译/烧录；资源沿用 §2，**不是本批重新量测的峰值**。COM3/COM12 未改固件/授权；同一原账户 MQTT 见证。

### 两条实际链路

| 阶段 | 真实结果 |
| --- | --- |
| 正常移除前 | 原 `device_215c0f9e-b44b-4ef8-8cc9-60a7daecf45c` 的认证 BLE 页面可控；本机 active Admin/epoch 1、journal exact ID/request 均存在 |
| 原设置移除 | DELETE 202/cloud revoked/local not_confirmed；随后只读 native 确认该 journal/Admin 均消失，见证仍在 |
| 明确 reset 后新认领 | 仅 COM11 `resetAccess` 回执与 GetInfo 空根，原 WiFiProv 新 CREATE 201→`device_e7649632-e7fa-4cad-8d6e-a739db7c0e7b`，新 request 不同于旧 request；一条真实 Bootstrap ACK，无 rotate/enable/reset-admin |
| 故障样本 | 对上述新测试 ID 原设置再 DELETE 202；测试工具仅对其原 store.complete 第一次调用抛本机写错误，不伪造云响应或设备 ACK。真实 native Admin 已删除但 journal 保留，故障钩子立即恢复 |
| 跨进程恢复 | 明确 COM11 reset；真实强停/重启 App；原 owner GET 返回旧 ID deleted，可见“原设备已移除”确认框；用户确认路径→原 DELETE 200 幂等重放→精确 journal 清理→新 CREATE 201→新 Bootstrap ACK/联网 |
| 最终身份 | `device_1c26af38-eed8-48f6-9ee2-d106e22d4d00`，名称 `C1d Retry WiFiBLE COM11`，新 request `wifiprov-7178c5ba9b07eb3a78931c2d4abf450b`；Key version 1、Admin epoch 1/active |

确认框通过真实 Ionic 组件 `confirm` 路径完成，未绕过页面状态/协调器；手机一直使用原测试账号，不是另造用户 token。第二轮证明的是云端已删但本地记录未清的真实恢复状态，不冒称使用了第二手机删除。每次测试只清确认目标，不对无 context 请求做“猜测清除”。

### 控制和普通恢复

两轮每轮都有初始、MCU 普通重启、App 冷启动三次 BLE Property/Action/State 与原账户 MQTT Property/Action/State；按钮计数实际增长，不将本地 UI 乐观更新当设备成功。

| 轮次/样本 | BLE Property / Action | MQTT Property ACK |
| --- | --- | --- |
| 正常退休后新认领 | 277 / 281 ms | 115 ms |
| 同设备 MCU 重启 | 318 / 323 ms | 93 ms |
| 同设备 App 冷启动 | 251 / 306 ms | 70 ms |
| 清理失败恢复后新认领 | 255 / 284 ms | 94 ms |
| 最终设备 MCU 重启 | 325 / 336 ms | 143 ms |
| 最终设备 App 冷启动 | 318 / 309 ms | 141 ms |

最终 MCU 重启后 BLE 页面 ready 4,235 ms、App 冷启动后进入 ready 3,685 ms，均为单次样本而非 SLA。首次恢复后观察剩余 ready 等待 7,498 ms，不宣称无缝实现。COM3/COM12 最后 Property ACK 115/159 ms，Action/State 均通过；所有控制页面退出连接归零，扫描/nearby 与控制连接分别记录。

最终 read-only PG 核验：两个被退休 ID 及其 CREATE journal 均 deleted，旧 session 为 revoked；最终 ID active/version 1，已有首次联网和普通重启两条 session。Service 保留旧 Admin 审计行不等于授权有效；本批不直接删除数据库行。App 首页、MQTT ready、本地控制连接 0、原始测试钩子已释放。串口 `NETWORK_READY/LOCAL_READY/CLOUD_READY=1`，`LIFECYCLE_ERROR=0`。

最终新分配 journal 仍可能因首次关联较晚而保留；这次已真实证明其能随明确移除退休或失败后恢复，不要求先等云上线才进入控制页。没有隐藏轮询/定期批量清缓存。

### 影响、证据与边界

本批原设置退休 `device_215c0f9e-b44b-4ef8-8cc9-60a7daecf45c` 和中间测试 `device_e7649632-e7fa-4cad-8d6e-a739db7c0e7b`，精确清两者的本机 WiFiProv 记录；两次 COM11 resetAccess，旧授权不能通过恢复固件备份简单复活。保留最终设备在线；没有清其他历史 Admin、没有整片擦除或新建服务端绕行合同。未暂存/提交/推送。

证据：`blinker-library/build/c1d-retirement-live-20260914/`（正常退休）、`build/c1d-retry-live-20260914/`（注入一次本机失败与跨进程恢复）、`blinker-app/build/c1d-retirement-20260914/tests.xml`/构建/安装记录。未测坏密码/AP 故障、无 context 的 CREATE 丢失与跨安装删除复合故障、Cloud-only ACK 丢失、清 App 数据/换手机及任意平台故障，继续明确失败且不自动换身份。C1 本次冻结范围可转入 C2，不等于整个 B2/C3/C4 已发布。
