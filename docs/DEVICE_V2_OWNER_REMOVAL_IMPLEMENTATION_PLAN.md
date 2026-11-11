# Device V2 owner 移除：云端撤销与本地清除分离

**最新 C1-d（2026-09-14）：原 owner 移除已接精确 allocation journal/对应 WiFiProv Admin 的本机退休；云端成功与本机写失败分开提示。正常退休/重新认领、故障后进程重启/空根/服务确认/显式同意/幂等重试两条真机链已通过，172/172 回归。手机记录清理仍不等于 MCU reset；只对已确认 COM11 分别执行现场 resetAccess。下一步已推进[主线 C2 §9](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md#9-c1-d-完成与-c2-定点验收入口2026-09-14)，详细见[报告 §6](DEVICE_V2_FIRST_PROVISIONING_RECOVERY_HARDWARE_REPORT.md#6-c1-d明确移除退休重认领及清理失败恢复2026-09-14)。下段“退休尚未接”为前批历史，不再重复执行。**

**当前执行入口：[核心稳定版 C1～C4](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md)。2026-09-14：首次 CREATE→context、context→pending、Bootstrap→WiFi 三个进程中断窗口已真机，最终 App 131/131 及匹配 APK 控制/重启通过，见[真机报告](DEVICE_V2_FIRST_PROVISIONING_RECOVERY_HARDWARE_REPORT.md)。新 journal 必须保留到首次 instance 关联确认，不能 Bootstrap 后即清除；现场关联较晚仍有精确恢复记录。原 owner 移除尚未接其退休，下一步固定 C1-d：明确云端移除→精确本地 journal 退休→现场 reset→重新认领，不能自动清历史授权或重放 deleted CREATE。本页 §21 旧混合移除通过不代表这个新增记录也已闭合；B2c/无缝扩展后置。**

2026-09-12。归属[基础收口路线 B2](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)。**B2b-6 普通 App pending 恢复入口及进程重启后的真实恢复已通过**，见 §13；复用原 intent/认证/commit，未新增协议。此前 **B2b-5 纯 BLE 同账号释放→本地 resetAccess→新认领→旧 Controller 拒绝→新授权重启恢复**见 §12；不是整个 B2 完成。后续 **D1b 首个显式零等待输出及短真机闭环已通过**，见 §14；普通 Print 仍不承诺非阻塞，不默认启用测试连续日志。此前 [B2a 真机主路径](DEVICE_V2_OWNER_REMOVAL_HARDWARE_REPORT.md)已通过临时 WiFi 删除和旧 Key 拒绝；下一步返回 WiFi+BLE 联合退休/B1，网关本地解绑、新所有者/带分享/故障真机组合未整体完成。

## 1. 先区分用户操作

当前入口为 **§20（B2b-12，2026-09-13）**：[混合真机报告 §6](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md#6-b2b-12信任链对照恢复错误分类与原见证恢复2026-09-13)。COM11 云登录断点已定位为错误 CA 配置并修正，真实 MQTT/自动 BLE 页面及普通重启均通过；COM3 普通重启恢复并自然续期，历史退化根因仍保留诊断边界。**下一步为混合设备同 ID 换根/联合移除，不重复 CA 排查、迁移部署或 enable-cloud。** 首次 create→pending 窗口仍未关闭；此前各节的“下一步”为历史，不据本次正常通信宣布 B2b 全完成。

| 操作 | 改变什么 | 不能声称什么 |
| --- | --- | --- |
| 成员退出 / owner 撤销分享 | 精确 account + device 的权限，复用 R3 投递 | 不删除 owner 设备、不清其他成员/设备 |
| owner 移除设备 | 撤销该逻辑设备的云端授权、Key/session、邀请及分享；保留删除记录供重试/审计 | HTTP 成功不是设备 Flash 已擦除 |
| 重新配网 | 改 SSID/密码，保留逻辑设备和页面 | 不自动转移所有权 |
| 刷新 DeviceKey | 新版本取代旧凭据、旧会话失效 | 不是删除设备；不是仅更新 App 文本 |
| 物理 resetAccess / 再次认领 | 清设备支持的本地授权根；重新建立授权 | 仅清本地不能作废仍写在 Sketch 的云端 Key |

原共享 LocalGrant 的 Retire 只撤对应授权，不等于 owner Admin 根、Presence、全部本地凭据和网关委托都已清除。MAC 不作为恢复旧所有权的依据。

## 2. 最小职责划分

1. Gateway `device-v2/removal.js` 只处理 owner/device 权威事务，不操作 MQTT、BLE、Flash。复用 user → tenant → device 的锁序和 SERIALIZABLE 重试。
2. 同一事务撤销 Key、challenge、session、未完成 Key 轮换、创建幂等记录、待领取邀请和 active 分享，标记逻辑设备 `deleted`。先撤分享，再改设备主体，避免原触发器重复递增 share 版本。
3. 有云端凭据的普通 WiFi 复用原 `v2_broker_invalidations` 的 `device_revoked`，不新增队列。原 worker 使 Broker 凭据失效并执行 deprovision，成功才确认；失败/进程重启仍持久重试。终态注销覆盖全部会话，包括 credentialVersion 已到 uint32 上限的情况。纯 BLE 无设备 MQTT，不制造设备注销任务；B2b-2 仅新增现有 enrollment 表状态/索引迁移 037。
4. 原 share/App-access 触发器与 worker 清理 source/target Broker；原 LocalAccess 触发器停止云端继续签发本地权限。**停止签发不等于已下发 MCU 撤销。**
5. 管理层提交后只唤醒原 worker，并清该设备的 Gateway 连接；不等待网络响应持有数据库锁。唤醒失败不回滚已提交的权威撤销，也不另开 MQTT 连接。
6. App 校验回执的设备 ID、HTTP/body 状态和 pending 字段一致性，清页面中已展示的 Key，回设备列表并刷新。移除旧 `/user/device/remove` 调用及无消费者服务方法/常量；不恢复 Legacy JSON 连接或删除路径。

Broker 完整注销已确认后，可以把不高于该终态版本的旧 Key 轮换投递标为覆盖完成，避免它们永远重试 `DEVICE_NOT_ACTIVE`；仅入队、仅发布或 Broker 失败时禁止覆盖。原轮换请求仍是 revoked，不恢复成能揭示新 Key 的成功请求。

## 3. 源码与本机部署边界

`DELETE /api/v2/devices/:logicalDeviceId`，账户 Bearer，owner-only，空 body。删除天然按逻辑设备幂等，不另建删除 ticket；重复 DELETE 是同一 tombstone 的状态查询，不再次递增版本/创建投递。外部或不存在设备统一 404。匹配的成功/错误响应均 `Cache-Control: no-store`。

源码支持普通 WiFi 云端设备、§9 满足一致性条件的纯 BLE，以及 §15 的普通 WiFi+BLE 联合退休。后者本批为软件门禁，尚未部署/真机；纯 BLE 真机已见 §12～13。普通 WiFi 可有 LAN，所有路径都**未确认本地清除**。下列情况在事务修改前拒绝：

- Edge Hub 或作为 hub/child 的任意网关拓扑（包括历史终态）；非纯 BLE 但没有云端版本；纯 BLE 但有 cloud Key/session/invalidation 历史；BLE 类型或存在 enrollment 历史却没有一致的 active installed intent/Admin：409 `DEVICE_V2_REMOVAL_LOCAL_LIFECYCLE_REQUIRED`。不能仅因含 Direct 元数据或 `device_type=ble` 就拒绝一个已有合法云端版本的混合设备。
- 已存在的 tenant route 不是 normal：409 `DEVICE_V2_REMOVAL_ROUTE_CHANGING`。迁移过程中完整退休另做门禁，不假装当前 worker 覆盖了全部旧 placement。

202 示例（200 的结构相同，仅两项 pending 都为 false）：

```json
{
  "status": 202,
  "data": {
    "logicalDeviceId": "device_example",
    "state": "deleted",
    "deletedAt": 1789171200000,
    "cloudAuthorization": "revoked",
    "brokerCleanupPending": true,
    "realtimeRefreshPending": false,
    "localAccess": "not_confirmed"
  }
}
```

- `cloudAuthorization=revoked`：权威事务已提交，旧 Key/session 不可再取得有效云端授权；已有连接是否完成清理看 pending。
- `brokerCleanupPending`：已有终态 Broker 注销投递是否均确认；普通 WiFi 没有投递记录不能伪报完成。纯 BLE 经准入确认没有设备 MQTT，返回 false；分享带来的 App 投递单独计算。未注册 Broker 的普通 WiFi 沿用原 worker 的“无远端设备可清理”判定。
- `realtimeRefreshPending`：该设备原 App-access 投递仍待确认，涉及分享路由，不是 App UI 已刷新或离线手机已清缓存。
- 任一 pending 为 true 返回 202，否则 200。普通 WiFi 第一次通常是 202；无分享待办的纯 BLE 可以直接 200。重复请求保持 `deletedAt`，直到收尾完成。
- **200 和 202 都返回 `localAccess=not_confirmed`**，不得翻译为“本地已解绑/可以直接转交”。服务端停用 LocalControl authority 后，不能假设仍可用它下发已认证本地擦除；在线退休顺序须在 B2b 独立闭合。
- 网络错误意味着结果未确认，应刷新/重复同一 DELETE，不能自动创建替代设备、恢复旧权限或改 Key。

当前 App 提示云端撤销及通信清理状态，不增加轮询框架或本地凭据删除假回执。普通 WiFi 若要重新使用，需创建新逻辑设备取得新 Key；旧 Key 即使仍在 Sketch 中也不能恢复已删除逻辑设备。

## 4. 分批路径，不扩散协议

| 批次 | 工作与退出条件 | 本批状态 |
| --- | --- | --- |
| B2a | 普通 WiFi owner 删除事务、原 Broker/share 收尾、App 如实消费；旧 Key/session/create replay 拒绝、另一设备隔离、重启重试 | 软件及无分享临时 WiFi 真机主路径通过；带分享/故障重启组合不冒称完成 |
| B2b | BLE / WiFi+BLE 本地授权根退休、在线 receipt / 离线物理 reset 引导、再次认领；清楚区分云端撤销与本地证明 | 纯 BLE 同账号生命周期及普通 pending 恢复真机通过；WiFi+BLE 联合退休软件通过，接入登记缺口及混合真机见 §15；未扩在线远程清根 |
| B2c | hub/child 按角色解除委托与拓扑引用、在途执行/多 child 隔离、迁移中的完整收尾 | 未实施，不等同于普通 WiFi 多发一次 DELETE |

B2b-1 已删除 `ble-enrollment.js::commit` 恢复分支无条件写 active 的实现，并统一检查 issue/replay/commit 的当前权威。历史 intent/receipt 不能复活 tombstone；具体见 §6。B2b-2/3 已补 §9 的显式历史释放和新上下文准入，但软件结果不是本地擦除证据，不引入旧 OwnershipRecord。

B2a 和 B2b 未新增 BBP 消息、存储槽或 Arduino API；不代表当前已有远程 Admin 根清除协议。专用临时 WiFi 和纯 BLE 主路径已执行；下一批按最新 §15 收口混合设备接入与退休。不要删除 COM12 LAN 基线或 COM3 隔离见证来凑测试；不把模拟 receipt 当 MCU ACK。

B1 默认明文/单 ZIP/三平台当前版本矩阵仍需补，B3 统一授时、B4 LAN 产品化尚未开始本批实施。R4 已收口，不追加百轮循环、后台通知保活或 Hub/S4 扩展。

## 5. 本批验证与证据

本机隔离 PostgreSQL schema：删除专项与原 invalidation worker 共 16 项通过；原 Gateway 公开 HTTP 接线测试通过（含 404、409、202 → 200、no-store）；相关 repository、LocalAccess、持久 App-access、sharing E2E 41 项通过。父/子项及关联测试不累计为独立故障数量。Broker/Auth 使用受控网络夹具，数据库真实；不是物理设备/手机测试。

App 删除回执/设置页、用户服务、鉴权拦截器及分享服务专项 64 项通过（5 个文件）；App/Gateway 类型检查通过。证据位于 ignored `build/b2-removal-20260912/`；删除用例已纳入 Gateway 正式 `test:device-v2` 清单。无业务库迁移、现场账号/设备删除、Flash/网络变更、APK 安装、暂存/提交/推送。

## 6. B2b-1：旧认领材料不能重新授予权利

问题已由隔离 PostgreSQL 负向测试复现，而非仅推测：原 active intent 重放可返回 PresenceKey；原 completed commit 在检查当前设备之前返回成功；pending recovery commit 可把 deleted/revoked logical row 写回 active。事务内还缺少账号停用后的准入检查，未完成的首次认领不能仅依靠 HTTP 入口早先取得的身份。

现有 repository 内复用两个小检查，不引入新的服务、队列、状态表或协议：

- issue/commit 在 SERIALIZABLE 事务中先取得 `user → tenant` advisory lock，再锁 tenant 行确认 owner 匹配且 active，与账号/设备移除协调；行锁也防止等待期间沿用旧快照。
- intent 返回、active commit 重放、新 recovery 签发及 pending/expired commit 均检查 logical row 的 owner/tenant/type/state；先检查，再读取 PresenceKey、签发或修改。已安装历史却缺失 logical row 不能作为首次创建处理。
- 真正首次认领可以在 commit 时创建缺失的 logical row；正常 active recovery 只更新必要时间戳，不再写 `state='active'`。
- 账号不可用：403 `DEVICE_V2_BLE_ENROLLMENT_OWNER_INACTIVE`；设备不再 active / 已安装记录对应的 logical row 缺失：410 `DEVICE_V2_BLE_ENROLLMENT_DEVICE_RETIRED`；上下文冲突仍 409。响应 no-store，不返回 grant/PresenceKey/成功 data。
- **grant 的首次安装期限与已安装凭据的寿命不同**：active 设备及账号下，过期 grant 对应的合法安装回执仍可完成断电恢复；相同 completed commit 重放不新增写入。不能把所有过期 commit 一刀切拒绝。
- cancel 仍只清理 pending 事务，不把它改为整机解绑。服务端未持有 Admin secret，receipt 字段匹配与 `method2Confirmed` 不等于服务端独立验证设备 HMAC。

App 的 enroll/resume 在 commit 失败后关闭当前 Direct，不进入 State 同步、不把 pending 改 active、不自动重新签发或删除持久恢复记录；临时密钥副本归零。新增两类明确失败提示，不改连接/凭据架构，也不以普通 403 推断蓝牙权限缺失。保留记录只是为后续现场处置，不表示它仍有有效云端权利。

## 7. 设备本地清除的实际能力

| 当前操作 | 实际边界 |
| --- | --- |
| 普通 Controller revoke / LocalGrant Retire | 撤特定委托；`DeviceAccessStore::revokeVerified` 明确拒绝撤 Admin，不能冒充整机解绑 |
| `DeviceAccessStore::clearAll()` | 清 controller/presence；若存在 DeviceKey 则保留它，不等于整根清除 |
| `Blinker.resetAccess()`（支持该能力的产品） | Product 先停 lifecycle，再调用 eraseAccess 并按原状态重启；纯 BLE 与 ESP32 WiFi+BLE 复用同一访问根存储，不另建 owner 槽 |
| `DeviceAccessStore::eraseAccess()` | 清统一 blob，包括其中 DeviceKey、epoch、Admin/Shared/Gateway、Presence；读回必须 NotFound 才报成功，失败向上传递 |

设备身份和 WiFi 配置在独立存储中，不由 eraseAccess 清除；保留 SSID 不代表清 Key 后仍能云端登录。Sketch 中的长期 Key 仍可能再次被代码读入，必须由服务器撤旧 Key。存储逻辑不可读不等于芯片底层取证意义的安全擦除；平台掉电/重刷/持久化仍需真机门禁。

本批 host 补测 BLE-only 和含 DeviceKey 的三槽访问根：清除返回失败、假成功但读回仍存在、重复清除、重建 store 后所有旧字段不可读、新 epoch/Admin/Presence 安装、旧三个 controller 不恢复。没有改设备生产实现或新增 RAM/Flash 常驻字段。

## 8. B2b-1 当时的下一步及证据界限（后续以 §9～10 为准）

1. **B2b-2 先完成单设备最小退出路径**：优先基于现有物理 resetAccess，明确云端 tombstone、旧 enrollment 历史终态、仍待本地确认三个事实。公开 DELETE 在路径闭合前继续 409；“我已重置”按钮、广播出现、HTTP 成功都不能作为擦除证明。若需在线远程清根，必须单独定义绑定 instance/epoch/operation 的整根清除与可恢复回执，不能绕过 Admin 撤销保护复用普通 revoke；不先铺新框架。
2. **再完成 B2b-3 同物理设备重新认领**：旧逻辑设备不复活；显式释放历史映射后，新逻辑身份与新 Hello/setup/epoch/controller 上下文独立建立，复用原 intent/Noise/receipt/Method 2。不能仅凭相同 MAC、旧 receipt、缺失 row 或主动删除历史记录跳过所有权检查。授权根清除前已签发 grant 的剩余安装窗口及迟到 receipt 必须纳入检验。
3. **随后短真机闭环**：专用 BLE 设备在线/离线退出→本地 reset→重新认领→旧 controller 不可连接；另一设备/原账户 MQTT 保持。再验证 WiFi+BLE 既有 DeviceKey 与本地根一起退休，最后才 B2c 网关角色/多 child，不回到百轮重连扩项。

B2b-1 使用真实隔离 PostgreSQL，未改现场业务账号/设备；新增退休用例纳入正式 Gateway `test:device-v2`。关联原 BLE 注册、普通 WiFi 移除、invalidation、Direct、Edge retirement 回归与 App/host 软件门禁记录在 ignored `build/b2b-enrollment-retirement-20260912/`。本轮未部署 Gateway、安装 APK、刷板、改网络、暂存/提交/推送；部署仍是 B2a 版本。新软件通过不能宣称本轮物理 BLE 解绑、旧 grant 无线攻击、掉电或新所有者认领已完成。

最终计数：BLE enrollment 两文件 Node runner **12/12**（含父项/脚本级项，退休子用例 10 项）、关联 Service **18/18**，均无 skip；App client/API/引导页/实际 HTTP interceptor 四文件 **54/54**；DeviceAccessStore Debug/ASan **1/1**、App/Gateway 类型检查通过。App 使用原 `GatewayHttpError` 归一化合同，403/410 不触发刷新登录或自动重放。详细复现、源码 hash 及测试边界见 ignored `verification.md`；测试数量不相加冒充真机故障覆盖。

## 9. B2b-2 本批冻结：先释放云端归属，物理 reset 后另行认领

第一性边界修订：owner 有权放弃服务器归属，即使设备离线；这与设备擦除是两项操作，不能互相冒充。最小路径不要求增加远程清根命令，也不把客户端声明的“已重置”当安全证据。本批只开放**有一致 active BLE enrollment/Admin、没有云端凭据、没有任何网关拓扑**的纯 BLE 设备；WiFi+BLE、曾启用 cloud、hub/child 和损坏/缺失的注册链仍 409。

1. 复用 owner DELETE：同一事务撤分享/邀请、写 logical tombstone，将该 logical 下 enrollment 历史改为显式 `retired`，清 pending 和 current/previous PresenceKey；不能硬删历史或借用 `superseded` 冒充释放。migration 037 仅追加 `retired` 枚举及 DeviceInstanceId 历史索引，不增加队列、MCU 字段或 wire。
2. 纯 BLE 没有设备 MQTT，因此不伪造 credentialVersion、不入设备 Broker 注销队列；原账户分享失效投递仍照常运行。回执的 `localAccess` 始终 `not_confirmed`，200 只表示服务端待办完成。保留旧 Direct 元数据/历史用于检查，不继续向旧逻辑设备签发权限。
3. 旧 issue/commit/cancel 均拒绝 retired 记录。释放后同一个 DeviceInstanceId 可建立**新 logicalDeviceId**，但不得复用 retired 历史的 setup session、transcript、epoch、controller 或 secret/fingerprint。它只是签发前的旧上下文排除，**不是服务器证明物理 reset**；不得把 active/损坏的历史当作已释放。
4. 设备端原 enrollment 在仍有访问根时拒绝启动；实际 resetAccess 后新 Hello/Noise 上下文、grant 校验、安装、receipt 与 Method 2 才完成新认领。旧逻辑页面不自动转移给新账号，MAC 不参与授权。Education/Nearby 不提升为硬件身份防克隆认证；Service 仍不独立验证设备 HMAC。
5. App 复用原移除入口，明确提示先云端释放、转交/重新添加前现场重置；不删整个账户存储、不自动重置板、不声称在线擦根成功。enroll/resume 的 commit 失败仍保留 pending。**仅在一次新 Provisioning 中，服务器明确拒绝旧 pending 为 410 DEVICE_RETIRED，且新 Hello epoch 与旧 pending 不同**时，清理这一条旧 pending 以继续添加；相同 epoch、网络失败、其他 403/410/503 均保留并报错。该条件不是物理擦除证明，后续仍须完整新 grant/receipt/Method 2。

软件门禁应同时覆盖回滚、两次 DELETE、在途 recovery、旧 exact receipt、同/异 owner 新认领、第二设备隔离、分享投递、无虚构设备 Broker 工作，以及设备 reset 后旧 grant 拒绝。软件通过后再部署和专用 BLE 真机；本节不是已有物理验证结论。

## 10. B2b-2/3 前批软件结果与当时下一步

- 真实隔离 PostgreSQL enrollment 两文件 **18/18**（退休子用例 16 项，计数含父项/脚本项）：原子回滚、重复 DELETE、迟到 recovery 并发、两个 owner 竞争、新 owner/同 owner 新逻辑身份、旧上下文及 receipt 拒绝、另一设备隔离、分享投递、cloud 历史及 pending/detached 拓扑拒绝均通过。不是 18 种真机故障。
- 原普通 WiFi removal/invalidation/Direct/Edge retirement 回归 **18/18**；生产 HTTP dispatcher 新增纯 BLE DELETE 200/no-store/重复回执与旧 commit 410，原接线脚本通过。Broker/Auth/receipt 使用夹具；未连接手机或 MCU。
- App client/API/引导页/interceptor/设置页五文件 **75/75**，包括纯 BLE 无 Key 的 200 处理、owner 移除与成员退出的不同确认提示。App 生产/测试及 Gateway 类型检查通过。没有另建存储管理层、重试框架、连接或 UI 流程。
- 设备 Debug/ASan enrollment 与访问根两目标 **2/2**。同一 application 在 eraseAccess 后创建新 window，仍在 TTL 内的旧 exact grant 拒绝且无写入，新 context/grant 可安装。signature verifier 是 host 夹具；不是无线签名攻击、掉电或 Flash 取证门禁。设备生产代码未变，没有增加常驻资源。

证据在 ignored `build/b2b-release-20260912/`。本批无业务库迁移/部署、APK 安装、刷板、真实账号移除、网络变更或暂存/提交/推送；现场仍是 B2a。

下一批先核对现场/备份本机 Gateway 数据并受控应用 037 与对应代码，更新 debug App；再选专用纯 BLE 测试身份，走现有 UI 移除→设备未 reset 时拒绝新安装→物理 resetAccess→新 logical ID 认领→旧 controller 拒绝→新控制成功，保留另一设备与原账户 MQTT。这是短闭环，不开始百轮测试。通过后才处理 WiFi+BLE 的 Key 与访问根联合退休；B2c 网关仍单列。

明确余项：账号整体注销造成的旧 BLE 映射退休/补偿不在本次 owner DELETE 路径内，不能自动把 deleted/缺失 row 当释放；App 既有 active credential 孤立缓存清理尚未统一，不能声称云端删除已擦所有手机副本。远程清 Admin 根、网关拓扑释放和硬件身份防克隆均未新增；不得以这些后续扩项替代本次纯 BLE 真机门禁。

## 11. B2b-4 本机部署及非破坏真机准备（2026-09-12）

1. 本机 Gateway 数据库从 schema 36 备份为 custom archive（2,471,230 B），校验 PGDMP/pg_restore 目录及 SHA-256；原镜像、compose 参数与旧 APK 留 ignored 检查点。随后只覆盖原镜像的 removal、ble-enrollment、migration registry、037 四个编译模块，部署 `blinker-gateway:b2b-release-20260912`。真实业务库已应用 037、retired 约束存在，Gateway healthy。不修改 Broker/账号服务，不开 cross-shard flags、不删除现场业务记录。
2. App local 构建/Android debug 构建通过并 `adb install -r` 保留数据更新；仅生成制品采用本机 HTTPS/内部 LAN 与原 debug CA，源码环境默认值不变。原 owner 仍在、15 个设备库存保留。安装会重启 App，不能声称 MQTT 跨安装不重连。
3. 安装后 COM3 原 WiFi 见证：实际 Manifest/State、Property ACK 156 ms、button Action ACK 184 ms，buttonPresses 到 4；这一段验证内同一账户 MQTT ready，随后释放观察。COM3 与 COM12 在实际 Broker 仍 active/online，前批已删除临时设备仍 deleted/revoked。
4. 手机原设置页展示“确认移除设备 / 移除设备”和“仅重启或移除云端记录不代表本地权限已清除”；仅预览并取消，未调用删除。原 owner-removal 真机工具去掉过时“确认解除”匹配，增加严格 `ble_`＋`B2b Temporary Removal ` 目标组合，仍要求原 owner/精确名称/保护列表。工具单测与 Cloud witness **10/10**，没有新生产 API。
5. 新 `tests/arduino/BleLifecycleGate` 只使用公开 BLE/组件/debug/resetAccess，启动不自动擦授权，串口 `r` 才明确清根；`s` 只查状态。ESP32 3.3.11 / XIAO ESP32S3 编译通过：程序 694,620 B、静态 RAM 34,848 B；这是测试 Sketch 构建，不是全部平台矩阵或已刷板。唯一公开 examples 目录不变。

当前硬件保护边界：COM3=WiFi 见证，COM12=LAN 基线；COM11 曾恢复原网关固件，不假设为空白。已询问用户是否允许 COM11 改作专用纯 BLE 板并清本地授权，未确认前不刷板/清根。确认后仍先核对实例对应的旧云端/网关历史；如被既有拓扑占用，不直接改库释放，使用经确认的空闲测试身份或另走 B2c。

**本批未完成纯 BLE 移除→物理 reset→新认领→旧 Controller 拒绝。** 下一步从硬件目标确认继续，不重复部署/创建账号、不追加长循环。固件应用区备份/可恢复刷写与本地凭据不可回滚须分开处理；禁止回灌旧 NVS 假装恢复安全状态。网关镜像回退也不是数据库权威回滚。

证据 ignored `build/b2b-live-20260912/`，含私有数据库/配置/旧 APK，不得暂存推送。无本批暂存、提交或推送。手机仍 debug owner/home、电脑“带翅膀的蜗牛”、手机同子网的 `_Wi-Fi5`，未修改无线网络。

## 12. B2b-5 纯 BLE 授权生命周期及串口对照真机（2026-09-12）

用户明确允许测试清授权后，只将 COM11 XIAO ESP32S3 改作专用 `BleLifecycleGate`。先备份系统区 65,536 B 和应用区 3,342,336 B，核对芯片、分区、启动元数据与固件 hash，再仅写 0x10000 应用区；`r` 才调用公开 `Blinker.resetAccess()`，不刷分区、不回灌旧 NVS、不直接 SQL 修改权威数据。首次应用区 ROM 读取超时，第二次完整读取 218 s；部分失败结果不作备份验收。

身份边界：COM11 普通 WiFi/BLE 的 `bl_v2_id` 与旧 EdgeHub 的 `bl_eh_id` 是不同持久命名空间。读取 CRC 合法实例记录并核对 Gateway：普通实例无 BLE 历史/拓扑，旧 Hub 历史单独保留，不据 MAC 清理网关映射。无线地址只定位测试板，生产 enrollment 的 Hello 实例必须精确匹配；本板 BLE 地址为基准 MAC +1，已纠正私有测试工具白名单。**没有验证跨产品角色切换清除全部历史命名空间，也不把本次当成 B2c。**

| 真机步骤 | 本批实际结果 |
| --- | --- |
| 原引导页新认领 | 使用生产 Hello/Noise、grant relay、exact receipt、Method 2、DirectSecure、commit/持久凭据及 Manifest/State；最终关闭串口的修订固件完整完成 7,911 ms |
| 原设置页 owner 移除 | 精确临时名称/逻辑 ID/owner/保护列表，原确认 handler；200、deleted、cloudAuthorization=revoked、localAccess=not_confirmed。仅操作本轮三个 BLE 临时逻辑身份 |
| 退休历史重放 | 第一轮原请求再次 issue、原 intent/commit/receipt 再 commit 均真实 HTTP 410 DEVICE_V2_BLE_ENROLLMENT_DEVICE_RETIRED；没有新签发/激活旧记录 |
| 尚未本地 reset | 已云端删除的旧 Controller 仍能真实 Direct 控制，2,844 ms；验证云端释放不等于本地擦除，设备此时不是新的 Provisioning 候选 |
| 公开本地 reset | 串口 `r` 明确 ACCESS_RESET_OK；原实例不变，后续注册为新逻辑 ID、新 setup/epoch/controller，旧逻辑身份不复活 |
| 新授权可用 | 最终固件 Property ACK 241 ms、Action ACK 267 ms；3 个字段的 Manifest/State 正常，buttonPresses=1，无不确定 Action 人为重试 |
| 旧 Controller 被拒 | 实际 GATT Hello 后收到精确 BBP/2 AuthResult(seq=2)、Method=2、Status=Rejected，无 secret payload；1,475 ms。不是仅把扫描失败、超时或通用错误当撤权证据 |
| 新授权重启恢复 | 不擦存储的 MCU 重启后，新 Controller ready 2,554 ms、Property ACK 241 ms；旧 Controller 再次精确 Rejected（1,463 ms），串口 LOCAL_READY=1、计数重启归零 |
| 另一设备/账户隔离 | 移除期间 COM3 24 次 Property ACK 全通过（56～189 ms），有 State 更新且原账户 MQTT 未重连；最终 COM3/COM12 均真实 Broker active/online |
| 数据库收尾 | 三个被移除身份 deleted、intent retired、pending/current/previous Presence 表无记录；最终身份 active、1 条 Presence、0 pending。四个 BLE 身份均无 cloud credential version、设备 session 或伪造的 Broker 注销任务；只读核查 |

### 12.1 发现并修正的测试阻塞，不扩 BLE 协议

前两次原测试固件在 receipt 已正确收到、Direct 首次 Hello 等回复时发生 10 s timeout；其中后一次 trace 明确定位到这一阶段。pending 没有被伪装为 active。用原 `BleDirectClient.resume`、真实 GATT 与原 HTTP commit 完成同 intent 恢复（私有 CDP 驱动核心客户端，**不是普通用户界面恢复入口已完成**）。

控制对照：同一旧固件保持串口持续读取时，完整 enrollment 7,774 ms；不读取时曾出现 6,259/6,274 ms Property/Action。本机 HWCDC 的写入在 USB backpressure 下会阻塞，Info 状态转换的多次 Print 也可能累积超过 BLE deadline。`BleLifecycleGate` 现移除开机 `Blinker.debug(Serial)` 及业务回调里的连续串口打印，计数仅由显式 `s` 查询。修订后不保持串口读取，完整 enrollment、控制和重启都通过。没有增加延时、重试、后台日志任务或改变 NimBLE/BBP 合同。

修订构建：本机 ESP32 Arduino 3.3.11 / XIAO ESP32S3，程序 **693,124 B**、静态 RAM **34,848 B**，bin 693,280 B，SHA-256 `fda3ed99697dfda108f7446a3701d266fac97de67b1c41c175e973a71bdb047b`。较原测试程序减少 1,496 B，静态 RAM 不变；不是未修改上游 SDK 或单 ZIP 多平台发布矩阵证明。测试工具回归 **10/10**。本批没有改三端生产通信代码，Service/App 沿用 §11 本机部署；没有再次安装 APK 或迁移业务库。

### 12.2 下一工作包与未覆盖边界

1. **小范围 B2b-6 收尾优先**：普通 BLE 引导页目前只发现 Provisioning；已安装、App pending 的 Direct 恢复没有普通用户入口。复用既有 credential store、授权 Presence 匹配、核心 resume/幂等 commit，补明确恢复操作与账户隔离；不复制握手、不静默重新认领、不替换为自动 reset。清理已退休 active credential 缓存须与云端释放/本地擦除区别处理。
2. **D1 串口背压单列修正**：通用 `debug(Print&)` 仍直接同步调用 Print，当前不能保证任意 sink 非阻塞；§9.3/9.4 的独立日志预算是要求，不是已实现事实。先冻结可检测容量/丢弃语义及最小 sink 适配，保留默认 logs off，不为此新增通用 ring buffer 或后台 task。常规无日志 BLE 主路径不被这一可选能力无限阻塞。
3. 再进入 B2b 混合 WiFi+BLE 的 Key/访问根联合退休，随后 B1 剩余平台矩阵/B3/B4；B2c 网关、长循环、History/Bulk 不在本批扩展。

仍未验收：真正另一账号重新认领、带分享/在途控制的整机移除、断电途中清根/Flash 取证擦除、手机备份恢复、旧无线 grant 全攻击矩阵、R4/Wio 本版生命周期。ACK 证明设备应用拒绝了旧授权，不证明服务器能远程确认整机物理擦除。

现场保留 COM11 的新 BLE 授权与静默测试固件；未恢复旧网关应用或旧 NVS。COM3/COM12 不清授权，原账号/home 保留。应用/系统备份、私有恢复上下文、CDP metadata trace、HTTP 回执、数据库只读审计在 ignored `build/b2b-lifecycle-20260912/`；不得提交凭据/备份。三个临时 BLE 云端身份已退休，原授权不回滚，继续使用新认领身份。本批无暂存、提交、推送。

## 13. B2b-6 普通 App 绑定恢复入口（2026-09-12）

### 13.1 一条恢复链，发现方式注入

普通引导页新增“继续未完成的绑定”：进入页面仅列当前 authority/account secure store 的 pending enrollment；不扫描、不自动重发 issue、不默认选择另一块同名板。用户明确点击后，新 GATT link 用该 pending 的授权 PresenceKey 定位，再走原 `BleDirectClient.resume` → Method 2 → 原 intent/commit/receipt → active → Manifest/State。

`DeviceV2BleService.resume(id, signal)` 是普通手机入口；原网关 `resumeUsing(link,id)` 保留 relay 的 `waitForMode`。二者共用 `resumeEnrollmentWithLink` 的凭据检查、清理、账号校验、认证提交和 Manifest 缓存，不复制握手。新手机 link 没有“上次目标”，不能强行调用其 `waitForMode`，这也是前批私有工具需单独定位的原因。MAC/名称不参与恢复授权。

引导页复用同一个完成结果处理函数；页面离开/切账号 abort 恢复，晚到 session 关闭，不写入继任页面。修正原 `fail(operation, error)` 未检查 operation 的缺口，以及完成阶段 await 库存/关闭后仍可能跳旧设备的问题。Ionic 缓存页面重入不显示已关闭的 ready/已取消的 busy。退休/超时/网络失败仍保留 checkpoint 并显示原错误，不自动新认领或删除凭据；读取 pending 失败不伪装为“没有记录”。中英文文案及既有卡片排版一并接线。

页面取消不是服务器事务回滚承诺：已发出的 commit 可以在页面退出后完成；原 account-scoped client 的幂等/存储规则保持，不能为了 UI 取消去撤权。此处软件覆盖账户切换和晚到结果，未将其扩大为所有移动端后台故障证明。

### 13.2 软件与真机证据

- Angular/Vitest **52/52**，5 文件：普通恢复 9、服务已有门禁 14、引导页 15、核心 client 13、native release 1。包含授权 Presence 匹配、缺权限不扫描、load/scan 期间切账号、scan 取消、退休不重试、网关共用路径、明确点击、late success/failure、finish 跨账号隔离。App/spec TypeScript 及 local/Android debug 构建通过。
- 备份原 debug APK 后保留数据更新；只在生成资产使用原本机 HTTPS/内部 LAN/debug CA，源码默认配置未改。Android 构建 helper 的 Windows 命令引用错误已改为直接 PowerShell 调用；不归因于产品代码、不安装新依赖。
- 经用户授权，原 App 设置页移除 §12 最终 COM11 临时 BLE 身份，真实 200/not_confirmed，然后仅对 COM11 调用 `resetAccess()`；COM3/COM12、原 Hub 命名空间保留。未重刷 MCU/修改 SDK。
- 真机故障注入**仅在原客户端第一次调用 commit、实际 HTTP 发送前主动抛错一次**。此前真实 Noise/授权安装/receipt/Method 2 已完成，App 为持久 pending 且本地 session 关闭；不能称为真实无线丢包或服务器 commit 成功后响应丢失测试。
- ADB 强制停止并重启 debug App（不清数据），普通 BLE 引导页从 secure store 重新列出该 pending。实际 DOM 按钮点击恢复 **3,146 ms**，**同逻辑 ID、同 intent、0 次 issue、1 次真实 commit**；state=active、Manifest 3 个字段、待完成列表移除此项。未使用前批私有 core.resume 旁路。
- 紧随恢复的 Property ACK **250 ms**、Action ACK **267 ms**，State `buttonPresses=1`；点击原完成入口进入该设备控制页。随后 COM3 WiFi Property **130 ms**、Action **194 ms**，原观察段账户 MQTT 保持；最终 Broker COM3/COM12 均 active/online。安装和强停 App 会重连 MQTT，不能宣称跨进程保持同一连接。
- 最终仅排版调整的 debug 包再次保留数据安装后，重新进引导页为 idle、pending=[]，原账号/home/cloud ready 保留。最终 APK SHA-256 `a7475a4053285d4ff1a789cb5cb8bed506f9795eda4545d61617caf7c8d1f529`。数据库只读核查新 intent active、1 条 Presence、无 pending Presence/设备 MQTT session/伪造注销任务。

私有证据 `build/b2b-recovery-20260912/` 含 tests.log、verification.json、源码 hash、APK/旧包、pending/恢复回执与截图；复用核查脚本的时间戳证据仍在原 b2b-lifecycle/b2-removal-live/b2b-live 目录。不提交这些制品/恢复上下文。本批 App 生产代码有改动，设备/服务器生产代码无改动；没有迁移业务库或部署 Gateway，没有暂存/提交/推送。

### 13.3 下一步与不冒称完成的项目

普通手机 pending 恢复入口不再是主线阻塞；下一小包为 **D1b 输出背压准入/适配及短门禁**，按[诊断 §9.5](DEVICE_V2_DIAGNOSTICS_OUTBOUND_TELEMETRY_PLAN.md#95-d1b-最小背压实施边界2026-09-12尚未实施)执行。随后回到混合 WiFi+BLE 的 Key/访问根联合退休与 B1 平台矩阵，再 B3/B4，不先扩 Hub/S4/长循环。

已退休 active credential 孤立缓存清理、账号整体注销补偿、另一账号转交、commit 成功后回包丢失/Flash 掉电等发布矩阵仍单列。恢复完成不意味着这些全部完成，也不意味着任意 Print 已不阻塞。现场 COM11 保留本轮新授权/静默测试固件；上一轮临时云端身份已退休、旧访问根已清，不回灌旧 NVS 恢复授权。

## 14. D1b 前置小包完成，返回混合退休（2026-09-12）

完整输出合同、软件与资源/真实背压结果统一在[诊断 §9.6](DEVICE_V2_DIAGNOSTICS_OUTBOUND_TELEMETRY_PLAN.md#96-d1b-首个平台闭环单次输出显式能力与真实背压2026-09-12)，不在移除协议复制一套日志规范。57/57 host；原 COM11 授权、无再次认领，在同串口句柄暂停/恢复读取时通过原 App Method 2/Hello/Manifest/State/Property/Action；最终恢复本批核心静默固件，原逻辑设备保持，COM3 WiFi 见证通过。没有本批清授权/重置身份/服务部署。

下一实施包先审核普通 ESP32 WiFi+BLE 当前 Key/session、BLE enrollment、Direct/Presence 根及 owner 移除路由如何关联，再以一个混合逻辑设备联合退休，不把纯 BLE 删除路径或 WiFi-only Key 撤销简单拼接。云端完成与 MCU 本地清除继续分开；离线时拒绝继续签发、保留待处理证据，不能把 HTTP 200 当 Flash 擦除。用户已允许测试时清授权，仍先核对专用目标，不动 COM3/COM12 基线或历史 Hub 关系，也不为了该包先扩 Hub 解绑。

## 15. B2b-7 混合身份联合退休软件收口（2026-09-12）

### 15.1 先按实际权威数据分流，不按设备名或 ID 前缀猜能力

| 实际接入链 | 服务端权威形态 | 本批退休规则 |
| --- | --- | --- |
| 普通 WiFi DeviceKey | 云端版本非空，可没有 Direct | 原 Key/session/分享撤销及持久 Broker 注销 |
| WiFiProv 新建 WiFi+BLE | 普通云端逻辑 ID；设备 Bootstrap 同时装 Key/Admin；不创建纯 BLE intent | 有无已登记 Direct 均按同一云端事务退休；清该 ID 已存在的 Presence，不伪造 BLE receipt |
| 已认领 BLE 后启用 WiFi | `ensureDeviceKeyForInstance` 保留原 `ble_` ID、`device_type=ble`、intent/Admin，只新增云端版本/Key | 保持 installed intent/Admin 一致性检查；同时退休 BLE 历史与云端凭据，并创建真实 Broker 注销任务 |
| 纯 BLE | `device_type=ble` 且云端版本为空，无云端凭据历史 | 原纯 BLE 路径，不凭空创建 MQTT 注销任务 |
| 任意网关 hub/child 历史 | topology 或 edge-hub | 仍 409，必须另走 B2c；新增 WiFi 不会自动释放历史拓扑 |

### 15.2 一个事务、原 worker、原回执

`removal.js` 移除“凡 Direct/enrollment 或 BLE 类型一律拒绝云端退休”的过时门禁。保留 user→tenant→device 锁序、SERIALIZABLE、一致 installed intent 校验及 topology/route 准入。共同事务按精确逻辑 ID 退休所有旧 intent（含在途 recovery）、删除 pending/current/previous Presence 材料，并撤 Key/challenge/session/创建与轮换请求/邀请/分享，最后写 deleted。Direct 非秘密元数据和 receipt 历史保留供审计；读取、再次登记、分配/揭示 Presence、issue/commit 仍先检查 active 权威，不可借幂等重放复活旧身份。

有云端版本就复用原持久 Broker invalidation，与 ID 前缀或 BLE 类型无关。没有新 API、迁移、消息类型、worker 或 Arduino 参数；重复 DELETE 不新增版本或注销工作。失败整体回滚。原 200/202、`localAccess=not_confirmed` 不变；旧设备离线本地根和手机安全缓存并不被数据库删除自动擦除。

设备端审核确认 ESP32 WiFi+BLE 的 Key、accessEpoch、三类 Controller 和 Presence 共用 `DeviceAccessStore`；公开 resetAccess 先停产品、eraseAccess 读回确认、再依原状态启动。resetNetwork 只清网络。新增 host 断言覆盖混合根清除后重新写入**新 Key＋新 Admin**，重建 store 后新 Key 正确、旧 Key/旧三个 Controller 在旧/新 epoch 下均不能恢复；纯 BLE 重认领仍不制造 Key。生产设备实现无需再造 OwnershipRecord/槽位，RAM/Flash 常驻布局不变。

### 15.3 软件证据与未完成事实

本机隔离 PostgreSQL 十个测试文件 **59/59、0 skipped**，覆盖删除/回滚/并发幂等/Key 与 session 拒绝/Presence replay/跨设备隔离/真实 `ensureDeviceKeyForInstance` 升级后的删除/在途 recovery 退休/历史网关阻断，及原 repository/session/worker/公开 dispatcher 回归。计数含父项和脚本级项，不宣称 59 种独立真机故障。设备 host **57/57**（复用已建核心，重编本次存储测试）；PG receipt 为明确模拟输入，不是 MCU 安装证明。App 配网/管理服务/设置页三文件 **26/26**，Gateway TypeScript 检查通过。

本批不部署本机 Gateway、不迁移业务库、不刷板、不清现场授权；COM3/COM12 基线及 COM11 当前纯 BLE 身份未被本批操作。证据在 ignored `build/b2b-mixed-20260912/`。无暂存、提交、推送。本节覆盖旧 §3/4 的混合设备全部 409、纯 BLE 真机待验收等过时当前描述，历史批次结果仍保留。

### 15.4 下一小包：配网登记与混合真机，不继续扩退休框架

1. **先补齐 WiFiProv Admin 元数据接线**：本次全局核对 `esp32-provision.page.ts`，新建链已生成 Admin、执行 101 B Bootstrap 并保存在 App secure store，但没有调用既有 `PUT direct-access/admin`；Service API 存在不等于 App 已使用。纯 BLE 的 enrollment commit 会登记 Admin，不能拿它的通过结果覆盖 WiFiProv。现有 API 只允许首次注册/精确重放，不能在 reset 后简单重复调用并期待替换旧 Admin。后续先明确首次登记、Bootstrap 失败恢复、同设备重配网是否保留根、reset 后新根的冲突处理，再最小接线；不上传 raw Admin secret、不无条件覆盖元数据，不复制一套 BLE enrollment。
2. **随后受控部署及专用混合真机**：备份当前 Gateway 后仅部署本批模块；精确核对可清授权的 COM11 普通实例，保留独立 Hub 命名空间。测试 BLE→启用 WiFi 保留同逻辑 ID，MQTT 与 Direct 控制、owner DELETE、旧 Key/旧认领拒绝、物理 reset 后新身份/新 Key/新 Controller、重启与另一设备隔离。WiFiProv 新建链需独立覆盖；前一路径通过不能替代后一路径。
3. **回 B1→B3→B4**：混合短闭环通过后补单 ZIP/当前平台矩阵，再授时和 LAN 产品化。另一账号转交/带分享/掉电/账号整体注销补偿仍独立列出，不扩大为本批已完成；不因这次新增混合准入开始 B2c/Hub/S4/百轮循环。

## 16. B2b-8 WiFiProv 首次 Admin 登记与检查点恢复（2026-09-12）

### 16.1 已实施：一个协调器，原 wire 和原访问根

App `core/device-v2/provisioning/wifiprov-admin.ts` 负责首次安装的顺序与恢复，注入安全存储、管理 API、配网 transport 和账号上下文检查；页面只负责网络/选择/导航。它不复制 Noise/纯 BLE enrollment，不新增 MCU API、消息或访问槽。Admin fingerprint 移入原 `BleDirectCrypto`，纯 BLE 和 WiFiProv 共用同一 transcript。

顺序冻结为：取得云端 Key context → **安全保存 pending Admin** → PUT 登记 controllerId/version/fingerprint → 原 101 B exact Bootstrap → 严格 3 B Bootstrap Success → 安全保存 active → WiFi 信息 → 原设备页。Service 仍不接收 raw Admin secret；页面不再持有 Admin secret 字段。payload、加载的 controllerSecret 和 fingerprint 临时字节数组在结束/失败时归零，不声称 JavaScript/插件内部不可变副本具备取证级擦除。

复用原账号＋authority 隔离的 secure credential store，仅加可选 `cloudContext={credentialVersion,locator}`，**不保存 DeviceKey 副本**；恢复由 owner 原 reveal 接口取 Key，并精确检查逻辑 ID/version/locator。WiFiProv pending 不出现在纯 BLE“继续绑定”列表，不携带假 intent/receipt。版本 5 的旧 active 数据仍可读；旧 App 不认识新增 pending 语义，不把降级 APK 当作恢复方案。

| 情况 | 实际行为 |
| --- | --- |
| pending 安全保存失败 | 不登记 Admin、不向 MCU 写 Bootstrap；不退回明文存储 |
| 登记 HTTP 失败或响应不确定 | 保留同一个 pending；重试原 metadata，不生成第二套 Admin |
| Bootstrap ACK 丢失 | 按实例/epoch 与 Key context 恢复，重放完全相同 101 B Bootstrap；**不退成 Key-only Install** |
| Bootstrap 成功、active 保存失败 | 仍保留原 pending；原设备端 exact replay 合同解决重试 |
| active 已保存、WiFi 尚未完成 | 重建页面可恢复同 ID/根，登记精确重放后继续网络步骤；不重写可能已有 Presence/Shared 的访问根 |
| Key 刷新/撤销，实例/epoch 不匹配，账号或页面事务已变 | 停止后续写入/激活/导航，不自动重新创建身份、不回滚其他账户授权 |
| 原页面记着成功，但 MCU 又被 reset | 再读 GetInfo，拒绝复用旧成功状态；不直接下发 WiFi |
| 空根从添加入口开始 | 只搜索 pending 安装；不把旧 active 检查点重新写入已清根设备。后续仍通过服务端 resolve-instance 核对当前所有权 |

原 device-settings 重新配网和 BLE→启用 WiFi 的保留根路径不改协议。旧 WiFiProv active（无 cloudContext）的补登记/迁移没有自动放开；原 source=enrollment 不重新执行首次 WiFiProv Admin 登记。

### 16.2 查询与新根替换边界

在原 `/api/v2/devices/{id}/direct-access/admin` 增加 **GET**，复用 `getOwned`：账户鉴权，200/no-store，`data.record` 为非秘密 owner active metadata 或 null。不存在、非 owner、非 active 不泄露别人的 Admin；读取失败不伪装成 null。原 PUT 三字段合同、首次 201/精确重放 200/冲突 409 不变。App 的账户 Gateway URL 识别同步接线，校验 no-store、目标/epoch/版本/controller 与 replay/status。

**尚未完成的功能必须明确：** 已清根但原逻辑设备还有服务端 Admin 时，目前预检在进一步轮换 Key、覆盖本机检查点或写 MCU 之前停止，并提示新授权替换未开放。已有 active 检查点也不能被当成 pending 自动复活旧根。这个保护消除了原流程“先刷新 Key 再撞 Admin 冲突”的已知路径，**不是 reset 后保留页面重配已经实现**；并发 GET→rotate 间的另一写入仍须由随后原 PUT 冲突校验兜底，GET 不是事务锁。

下一小包必须为“保留原逻辑 ID/页面、安装新 Admin”补**显式条件更新**，而不是删除 Direct 行或放开任意覆盖：冻结预期旧 controller/epoch、当前云端版本、候选新根与操作重放关系；覆盖首次 WiFiProv 与 BLE→WiFi 两类来源，保留实例映射。原 Presence/Shared/在途授权、旧 BLE intent 及网关引用如何退休须在同一权威边界处理；有网关历史仍不借此绕过 B2c。不能伪造纯 BLE receipt，也不能仅将 Key 版本当成本地清根证明。该条件更新本批未新增 API/迁移/候选表。

### 16.3 验证与下一步

- App **10 文件 92/92**：新协调器、实际页面接线、secure store、metadata 客户端、原 WiFiProv wire/native adapter、纯 BLE client/恢复和设置页回归；覆盖检查点顺序、准确重放、保存失败、上下文冲突、重建页面及切账号。测试初期的 typed-array realm 差异已修在 jsdom 夹具，不放宽生产二进制验证。
- Service 原十文件 PostgreSQL 门禁 **59/59、0 skipped**；补 GET owner/非 owner/删除后/no-store/不泄露 secret 或 fingerprint 的覆盖。计数含脚本/父项，不是 59 种真机故障。
- App、spec、Gateway TypeScript 与 App local 完整构建通过。设备生产代码/固件未改，本批不新增设备常驻 RAM/Flash。
- **未部署 Gateway、未安装 APK、未刷板或清现场授权**。软件中断/页面重建是可复现模拟，不是真实 ADB 进程死亡或无线丢包证据；首次 create/rotate 成功但 pending 尚未保存的窗口，也未被本检查点覆盖。多条同实例检查点仍明确报歧义，不自动删除旧账号材料。

证据在 ignored `build/b2b-wifiprov-admin-20260912/`，含软件输出、source hashes；无暂存、提交、推送。下一步优先收口上述新根条件更新和分配到检查点之间的恢复边界，再备份/受控部署，执行专用 WiFi+BLE 的首次接入与联合退休短真机；保持 COM3/COM12 和历史 Hub 不被顺带清除。随后 B1→B3→B4，不扩百轮/Hub/S4。

## 17. B2b-9 新根条件替换、实例连续性及旧授权隔离（2026-09-12）

### 17.1 本批软件边界：服务器事务及 App 管理适配器，不是页面全链完成

原 Direct Admin endpoint 新增 **PATCH**，PUT 仍只首次登记/精确重放。PATCH 使用 owner 账户鉴权，明确提供操作 ID、设备实例、预期旧 controller/epoch、当前已轮换云端版本、新 controller/version/fingerprint；新 accessEpoch 为旧值 + 1，不允许溢出或沿用旧 controller/fingerprint。严格八字段见 [Service API](../../aily-blinker-services/docs/API.md#direct-admin-条件替换b2b-9源码未部署)。不接收 raw Admin secret、不新增 MCU wire/槽位、不自动改 DeviceKey。

`resetAdmin` 沿用既有 repository/错误边界，user→tenant→device 锁序和 SERIALIZABLE 三次重试；校验 active owner/tenant/device、正常 route、当前 active rotated Key，以及旧根的精确条件。任何历史 topology（含 detached）或 edge-hub 仍拒绝；纯 BLE 必须先按原路径启用云端并轮换 Key，不能拿 WiFiProv 替换覆盖其纯 BLE enrollment 合同。

新迁移 **038** 只增加非秘密操作日志/当前 Direct 实例视图及 Presence 请求 epoch 约束。日志绑定整个规范化请求摘要，保留新 controller/fingerprint/epoch、实例及云端版本；禁止修改既有日志。精确重试只在账户仍 active、Key 版本仍 current、新根仍匹配时返回 200；首次事务 201，修改请求/旧根/旧 Key/被更新的操作 409。重放不重复删 Presence 或刷新分享版本。不存在/非 owner/已删除为 404，无秘密回包。

**这些检查不是物理擦除证明。** 实例来自历史已完成 session 或当前 Direct 认领/替换连续性，不使用 MAC 认证，不限制普通 DeviceKey 的可移植登录。GET、PATCH、Key 版本均不能证明 Flash 已清；3 B Bootstrap ACK 仍不是 Service 可验证的 BLE enrollment receipt。后续 App 必须经受控 WiFiProv GetInfo 核对空根，并在本机安全持久化候选 Admin 后才 PATCH/Bootstrap。当前 App 仅新增严格管理客户端及共用 PUT/PATCH 响应验证；页面仍保留 B2b-8 的阻断，不会调用半完成流程。

### 17.2 同一事务中保留什么、退休什么

| 对象 | 条件替换行为 |
| --- | --- |
| logical ID、设备名、账号页面、账号分享角色/组件范围 | 保留；重配网不等于 owner 删除或解除分享 |
| 旧 active/pending/历史 BLE intent | 同 logical ID 全部 retired；旧 receipt/issue/commit 不可恢复；历史 receipt 不伪造、不改写 |
| BLE→WiFi 的当前实例映射 | 当前 Direct 实例视图接替 retired intent，resolve/enable 仍返回原 ID，不等待新 MQTT session |
| 首次 WiFiProv 来源的映射 | 当前 Direct 连续性优先，原 owner 下无 Direct 时才回退已完成 session；缺失或歧义明确拒绝 |
| 原 pending/current/previous Presence 及轮换请求 | 移除精确目标的旧材料；保留幂等请求历史并绑定原 accessEpoch |
| 已签发 LAN 授权 | 调用原持久 retirement 机制，禁止继续用旧授权；禁用旧 LAN authority，待新 session 重新建立 |
| 账号分享 | 不删除关系；递增既有版本、复用原双端失效队列，使旧观察/授权过期；有效分享可重新授权 |
| MCU 上的 Shared/Gateway 槽 | 不宣称远程擦除；必须由物理 reset 清根。有网关历史不进入本事务 |
| DeviceKey/Broker 注销 | 原 rotate/worker 负责；本 PATCH 不再制造第二次 Key 轮换、假 Broker ACK 或新 worker |

新根仍 active 时，纯 BLE fresh claim 被当前实例视图阻断（包含同 owner）；不能因旧 intent 已 retired 再创建第二个设备。随后真正 owner DELETE 会按混合路径撤 Key/根并释放映射。释放后也不能复用日志里的旧 epoch/controller/fingerprint 重新认领。原 BLE→WiFi 的旧 `device_type=ble` 不改名为 diy 来绕过一致性检查；移除路由明确接受与当前 Direct 根完全匹配的替换日志。

测试发现并修正 **Presence 版本别名**：换根后 version 1 可能再次出现，单独按 `result_version` 重放会错误返回新根材料。请求新增 accessEpoch，回放必须与当前材料的 epoch 相等；迁移只从仍存在且同版本的 key 回填，无法确定的历史请求保留 null 并拒绝重放，不能猜成当前根。该修正是本包新根退休的必要依赖，不新增发现协议。

### 17.3 下一包的固定顺序：先检查点、后打开页面、再真机

1. **安全准备在外部副作用之前**：复用原账号隔离 secure store 和 `WiFiProvAdmin`，持久化一次替换操作、原 ID/实例/预期旧根、Key 轮换幂等 ID、候选新 Admin 与阶段；随后才调用 rotate。不能在重试时生成新的 rotate ID/新 Admin，也不建立另一套全局事务引擎或把 secret 放页面/localStorage。
2. **完成轮换及登记**：按原 rotate/worker 确認完成，保存精确新 Key context，再调用 PATCH。Key 不长期保存副本；重建进程通过同幂等轮换和 owner reveal 恢复。当前 PATCH 仅校验当前 rotated Key，不将它的 201 等同于 Broker 已强踢旧会话。
3. **原 wire 安装及激活**：复用原 exact 101 B Bootstrap 和严格 3 B ACK；ACK 不确定时重放同一候选，不降级 Key-only Install。只在 ACK 后写 active，再网络/页面。server CAS 成功但未写 MCU 时保留 pending；不回滚服务器到旧根，也不丢弃候选 secret。
4. **软件故障门禁后才解除页面保护**：覆盖 prepared 保存失败、rotate 已成功但回包丢失、保存新 context 失败、PATCH 成功回包丢失、Bootstrap/active 保存失败、切账号及 GetInfo 发生变化；无实例证据和旧 active 无 cloudContext 的情况明确提示，不猜测绑定。
5. **备份/受控部署和专用混合真机**：迁移 038 与 Direct/Presence/实例解析/retirement 模块一起部署；不只拷贝一个 handler。真实 WiFiProv 首次链、BLE→WiFi 保留 ID、重置后保留页面换根、MQTT/Direct 控制、联合移除与见证设备隔离。完成后回 B1→B3→B4，不转入 Hub/S4/百轮。

### 17.4 验证范围

App 十文件 **95/95**，生产/测试 TypeScript、Gateway TypeScript 和 App local 完整构建通过。仅管理适配器和公共响应校验发生 App 生产变更；WiFiProv 页面/协调器仍沿用 B2b-8 的保护，未改 MCU 生产代码。

本包 PostgreSQL 十一个测试文件 **65/65、0 skipped**，包含原十文件回归与新条件替换测试；真实 repository 的 Key/session 状态转换，协议字节/receipt 为明确测试夹具，不是真机证据。覆盖回滚、并发 exact replay/竞争候选、账户/Key/epoch/实例歧义门禁、页面/分享策略保留、LocalGrant 退休、Presence 旧请求拒绝、BLE→WiFi 后旧 receipt/fresh claim 拒绝、随后 owner 删除及另一设备隔离。

App/类型/构建结果见本包 ignored `build/b2b-admin-reset-20260912/verification.json`，原始日志与源码 SHA 在同目录。本包**未部署 Gateway、未迁移业务库、未安装 APK、未刷板或清现场授权**；只在自动隔离测试 schema 上运行迁移，测试结束回收该 schema。没有暂存、提交、推送；没有删除用户源码文件或旧现场数据。未通过 App 页面替换和混合真机，不将 B2b 主路径标成全部完成。

## 18. B2b-10 Key 轮换前安全检查点与页面恢复接线（2026-09-12）

### 18.1 已实现：原协调器、原 secure store、原设备 wire

`WiFiProvAdmin.reconfigure()` 承接已有云端设备的重新配网：GetInfo 空根 → 查询原 Admin/核对本机旧记录与实例 → owner reveal 核对当前 Key context → 安全保存 **prepared** → 原 rotate → 安全保存 **key-ready** → owner reveal 验证新 context → 原 PATCH（有旧 Admin）或 PUT（尚无 Admin）→ 再读 GetInfo → exact 101 B Bootstrap → 严格 3 B ACK → active → 网络步骤。

同一个协调器继续负责首次 install 和 resume，共用 candidate、fingerprint、注册/Bootstrap/secret 清理，未再建状态机框架。页面只收集网络/选择与导航，移除“已有 Admin 一律阻断”的临时方法；不持有候选 secret 或复制八字段 PATCH 组包。网络-only reset 仍保留原访问根，Cloud-only 产品继续原 Install，不强行走 Direct Bootstrap。

单个既有账户＋authority 隔离 credential 记录新增非秘密 `keyRotation`：`phase`、16 B `operationId`、`previousContext`、可选旧 controller/epoch。轮换幂等 ID 从已保存 operation 派生，格式 `wifiprov-{operationId}`；没有第二个需要同步的持久 ID。候选 Admin secret 仍只在这个 OS secure record 中，**不持久保存 DeviceKey**；序列化明确提取已知元数据，即便误展开 reveal 响应也不会保存其中的 Key。

prepared 的 cloudContext 仍是原版本，key-ready 必须恰为原版本 + 1 且 locator 改变，候选 accessEpoch 是旧根 + 1（没有旧根时为 1）。带 journal 的记录写 **schema 6**；当前 reader 同时读原 schema 5。旧 reader 必须拒绝 schema 6，不能把 prepared 错当成原版本 Key 已就绪的首次安装。ACK 后保存 active 并移除 journal，仍可写普通 schema 5。新 schema 不进入纯 BLE pending 列表，不伪造 intent/receipt，不继承旧 Presence。

### 18.2 故障结果与恢复合同

| 中断点 | 持久状态与后续行为 |
| --- | --- |
| prepared 保存失败 | 原记录保留；未 rotate/PATCH/Bootstrap |
| rotate 失败、已提交但回包丢失、Broker disconnect pending | 保留 prepared；新页面/协调器再次使用同一轮换 ID，不重复分配 Admin；未拿到成功轮换响应不 PATCH/Bootstrap |
| key-ready 保存失败 | prepared 可重放原 rotate，拿回同一版本；不先 PATCH 或写 MCU |
| PATCH 失败或响应丢失 | 保留 key-ready；原 operation、预期旧根、新根指纹和版本精确重放；不再次 rotate、不回滚成旧 Admin |
| Bootstrap ACK 丢失 | 保留 key-ready，校验实例/epoch 后重放同一 101 B；不降级 Key-only Install |
| active 保存失败 | 原 pending 仍可按 exact replay 恢复；没有 HTTP 即激活 |
| active 后网络失败 | 复用同 ID/新 epoch，不重复轮换或写访问根；恢复前核对云端 Key context |
| 切账号/页面事务改变/迟到返回 | 后续保存、PATCH、设备写入、导航停止；加载的临时 secret 清理；已保存检查点留在原账号空间 |
| Key 被其他操作刷新、旧根/实例/epoch 不一致或再次读 GetInfo 发现变化 | 停止，不“修正”为另一版本继续、不创建第二设备、不无条件覆盖记录 |

原已激活记录遇到空根时只让显式新操作接管，不重新安装旧 Admin。新操作安全保存后，本机这条记录为 pending，不能用旧 secret 继续 Direct。BLE enrollment 来源且**已有云端 Key**的混合设备可用同一方法换根，保留 ID、以新 WiFiProv 凭据替换本机旧 receipt/Presence；没有云端版本的纯 BLE 仍应先完成原保留根 enable-cloud 路径，未新增“纯 BLE 清根后直接猜 Key”入口。旧 WiFiProv active 没有 cloudContext 的自动迁移仍拒绝，明确提示恢复记录。

修正关联 bug：`resume()` 返回真实 accessEpoch，页面统一 `rememberIdentity()`；从空根安装 epoch 2/8 后不会错误记成 1，后续 WiFi 重试不会误报根变化。页面删除重复的字节比较函数，使用原 wire helper；`registerExisting` 收为协调器私有实现。

### 18.3 软件证据与真实边界

本批 App 十文件 **116/116**、App/spec/Gateway TypeScript 与完整 local build 通过；最终结果、源码 SHA 位于 ignored `build/b2b-wifiprov-rotation-20260912/verification.json`。覆盖 secure store 重建、版本/阶段/账号隔离、元数据字段过滤、同操作响应丢失/保存失败、Broker pending、PATCH/ACK 精确重试、页面重建、真实新 epoch、BLE→WiFi 来源及原首次/Direct/设置回归。

这些是确定性的内存/OS 插件 mock 和真实 Angular 页面类测试，**不是 ADB 进程死亡、射频丢包或 Flash 擦除证据**。Service/MCU 生产代码本批未改；Service 沿用 B2b-9 的 PG 65/65，本批不重复冒称已运行。没有部署 Gateway/迁移业务库/安装 APK/刷板/清授权，没有暂存/提交/推送。

### 18.4 剩余项和下一包：进入部署前门禁，不回头扩协议

1. **部署前演练及专用混合真机**：先对 037→038 做存量升级演练/备份，关联发布 Gateway，再安装匹配 APK；核对现场精确目标和最新网络。优先已有云端混合设备的 reset→同 ID/页面、新 Key/Admin→MQTT/Direct→重启/联合移除，以及 COM3/COM12 见证隔离。不只拷贝 handler，不清历史 Hub，不把源码接线当已在手机运行。
2. **首次 create 之前的分配检查点仍缺**：新 logical ID 尚未分配时，当前首次 create/reveal→pending 窗口未被本轮“已有 ID 的 prepared”覆盖。不能宣称任意首次配网杀进程都不重复创建；后续以最小账号/实例作用域的分配恢复记录解决，不把占位 ID 伪装成真实凭据，也不另起泛用事务引擎。Cloud-only Install 的跨进程轮换恢复未在本包扩展。
3. **并发与遗失材料边界**：服务端 CAS 继续最终判定当前根/Key；rotate 前 reveal 只是新鲜度检查，不是跨请求锁。另一客户端并发刷新可能使本操作结果不再是预期下一版，此时保留错误检查点、停止安装，不自动发起新一轮。安全存储被删除、未知旧 active、跨安装降级和 pending 操作明确取消仍需单列处理，不能从 fingerprint 反推 secret。
4. 混合主路径真机后回 **B1 单 ZIP/平台矩阵→B3 授时→B4 LAN 产品化**。首次接入故障余项保持发布门禁，B2b 不宣布全完成；不转向 Hub/S4、工厂方案或百轮测试。

## 19. B2b-11 受控部署与混合身份真实接入（2026-09-12）

详细证据与精确现场见 [混合生命周期真机报告](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)。本机 schema 38 / 8 个关联模块 / healthy 已核对，Broker/Auth 未重启；配套 APK 保留原账号数据安装。实际备份完整恢复演练比对 44 张原表、10 个实例映射，原迁移器第二遍零写；不恢复业务库来回退授权。App 116/116、Service PG 65/65（本批重跑）、硬件工具 12/12 通过。

COM11 从普通 BLE 应用改烧公开 WiFi+BLE 产品测试副本，仅 app0、保留 NVS/实例/原根。App WiFiProv 原生 Security1 与 GetInfo 成功；只 enable-cloud 一次，create/rotate 均零，原 BLE logical ID/type/页面和 epoch 保留，云端版本由 null 变 1，自动进入原设备页。Direct 原根认证/Property/Action 短门禁通过；COM3 前后真实 MQTT 控制见证及 COM12 状态保留。详细剩余及环境修正按真机报告更新，不将仅云端分配版本等同于实际 MQTT 已在线。

清理与修正保持局部：旧配网/设置“不会刷新 Key”文案按 network-only/full reset 区分；硬件 Cloud witness 允许合法 BLE-origin logical ID，不再把 `ble_` 前缀当作永不能连接 MQTT。仍使用原账户 MQTT/观察租约/命令方法，补合法/非法 ID 和同会话工具回归；不新增通信封装。

跨日续测已修复 App 将 BLE-origin 原生 WiFi 误判为网关子设备：Service 原目录投影新增 `gatewayRouted`，App 两处共用路由判断并保留离线非秘密标记；有 Key 不代表无拓扑。App 223/223、PG 65/65 再次通过，本机仅追加 management 编译模块并保留数据更新 APK。真实页面自动 BLE ready 3,638 ms、开关/按钮 334/350 ms，离页后物理连接退出；具体构建/回滚打包失误见报告 §5。

尚未通过：修正 TLS 17000 的测试固件重启后网络/local ready，但 cloudReady 0、精确设备无 challenge/session；最新 COM3 见证也未 ready、未发命令，不能引用此前成功代替。下一包先定位两条云端断点并恢复见证，再 reset/PATCH/旧凭据拒绝/联合移除。不重复配网分配或通过清根增加变量。首次分配窗口继续保留；之后 B1→B3→B4，不重开网关/S4/百轮。本批无 MCU 生产代码/设备 wire 改动、无暂存/提交/推送。

## 20. B2b-12 云登录恢复与换根前双通道基线（2026-09-13）

详见 [混合真机报告 §6](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)。上一批误将叶证书作为 CA；主机及同一 XIAO 的 TCP/叶证书/根证书对照确认后，仅修正测试 CA 并写 app0，原 Key/实例/本地根不动。真实 MQTT Property/Action 80/64 ms、自动 BLE 页面 259/282 ms；普通重启后分别 138/91 ms、312/307 ms，fresh Manifest/State 和离页回收通过。COM3 普通重启后同版本重新登录，连续三次正常自动续期，最终见证 243/203 ms；此前离线的完整原因未捕获，不以新代码替代现场证据。

库内修正保持最小：透传可恢复 Backoff 原因、记录 MQTT socket 丢失、HTTP 响应传输截断按原事务重试；完整损坏 CBOR/鉴权拒绝仍失败关闭。Starting 非零 lastError 不再被误解成永久故障，云端退避时 BLE 继续轮询。无新 wire、凭据槽、运行时字段或 UI；主机 58/58、实际 RAM 56,108 B。App/Service 本批生产代码和已部署版本未变；无清根/轮换/PATCH/联合移除/暂存/提交/推送。

**下一步只进入混合授权生命周期门禁**：精确 COM11 空根→原实例/ID/页面恢复→prepared/单次 rotate/key-ready→条件 Admin PATCH/Bootstrap ACK→旧授权拒绝/新 MQTT 与 Direct/普通重启；随后联合 owner 移除，并持续保留 COM3/COM12 见证。云端撤销和本地 Flash 清除分别验收。首次分配窗口及未测故障组合继续单列；完成此主路径后 B1→B3→B4，不再回头重复部署/配网分配或扩 Hub/S4。

## 21. B2b-13 同 ID 换根与混合联合移除真机收口（2026-09-13）

完整证据见 [混合真机报告 §7](DEVICE_V2_MIXED_LIFECYCLE_HARDWARE_REPORT.md)。精确 COM11 resetAccess 后，真实 WiFiProv 页面只 rotate 1 次、条件 PATCH 1 次、无 create/enable/register；原实例/ID/页面保持，Key 1→2、epoch +1。旧 Controller 精确 Method 2 Rejected、旧 MQTT 强断及未到期重连 CONNACK 5、旧 Key 403/DeviceKeyRotated 均通过。新 MQTT/自动 BLE 页面和普通重启控制通过。

真实 owner 设置页 202→worker 回执→显式幂等 DELETE 200，目标全部云 session/Key 退休，Key 2 新登录 403/DeviceRevoked；COM3 同账户 24 次控制成功、COM12 保持。Direct 审计记录保留，active 实例映射退休，owner metadata GET 是 200/null，不是必须 410 或硬删审计行。卡片 16→15，页面显示的 Key 清空。

**云端撤销、本地停服、本地擦除是三个事实**：当前混合产品终止性鉴权错误会停共享 Client/WiFi/Direct，不代表 eraseAccess；不能把扫描不到当作认证 wire 拒绝。最后另行显式 resetAccess 后，真实 Security1 GetInfo 证实同实例 flags=2/epoch=0。服务端仍 `localAccess=not_confirmed`，无设备擦除证明不能改成已确认。独立本地授权与 cloud key/session/device 失败分类留 B2c，不在本包放宽旧安全策略。

真实旧 APK 首次扫描争用已修：原协调器提供外部 SDK 排他交接，等待物理扫描排空；原配网页面持有/释放、排空迟到 start，关闭失败继续隔离。无新增 MCU/Service wire、存储槽或扫描状态机。App 135/135、类型/构建、新 APK 真实扫描交接/退出回收通过，host 58/58；Service 本批未重新部署/改代码。COM11 已空根待配网，COM3/COM12 未清授权，临时 App secret 已清零；不恢复删除的原 ID。

**下一包 B1 单 ZIP/默认示例/平台接入矩阵→B3→B4**。首次分配窗口、未测无线/杀进程组合、另一账号/带分享/B2c、Wio 长稳和历史 COM3 问题保持明确未完，不把本次主路径完成写成全部 B2/发布完成。未暂存/提交/推送。
