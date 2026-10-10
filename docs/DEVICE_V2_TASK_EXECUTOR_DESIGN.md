# Device V2 本机定时执行器：架构、持久化与实施边界

更新：2026-09-24。由[任务与 BLE 方案](DEVICE_V2_SCHEDULING_AUTOMATION_AND_BLE_PLAN.md)的 T0b 展开；[总主线](DEVICE_V2_NEXT_EXECUTION_PLAN.md)仍是唯一优先级入口。

**结论：第四十五批已关闭T2最小设备源码交接。** 433项清单在仓库外重建出与第四十四批真机候选完全一致的ZIP，隔离安装/完整编译公开入口及来源/链接检查通过，见§10.44。公开API合同§8.17、软件§10.42、实物§10.43保留；[交接说明](DEVICE_V2_TASK_PILOT_HANDOFF.md)区分用户与维护者。第四十七批已完成[A1-1纯规则/判定软件](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_AUTOMATION_IMPLEMENTATION_PLAN.md)，当前下一步取主线§6第20项Broker有界观察，不修改本机引擎。已分享operator、写额度实物/其他平台/生产材料/三端完整构建独立待办，未知根恢复未实现，T2正式整体未关闭。

## 1. 先限定“定时”做什么

- 首版是设备本地执行一个**开发者显式允许调度的属性赋值**，支持倒计时、一次日期、固定偏移的每日/每周。不是一个任务线程，也不是 App 持续推送。
- 创建、修改、删除、启停和倒计时 Start 来自已验证的 owner；一旦落盘，页面退出/网络断开不撤销任务。业务日历仍受现有有效授时范围限制。
- 属性赋值仅表示同一个目标值；回调仍可能有副作用。因此首版不开放任意 Action、开关翻转、累加、任意脚本或通用 JSON 任务。
- 默认不启用；不改变现有 begin/组件用法，不为每条连接复制任务模块。开发者周期回调、onSample、OutboundScheduler 继续各司其职。
- 推荐先用 **4 条任务**做资源探针，再测 8/10 条容量构建；旧版支持 10 条日历任务，4 条不是已决定的功能降级或发布上限。未测整机 RAM/Flash/栈与持久写耗时前不固定最终发布上限。

## 2. 代码审查发现的实际约束

| 当前模块 | 可以复用 | 不能据此推断 |
| --- | --- | --- |
| `core/TaskTiming` | 日历下一时刻、易失单调倒计时 | 已有去重、执行、持久化或权限 |
| `core/WallClock` | 同一 UTC/误差/年龄快照；本批新增 sampleRevision | 数天无源准时运行；业务时钟可作为安全时钟 |
| `interface/IAtomicBlobStore` | 一个 blob 的旧值/新值崩溃一致边界 | 跨多个 blob 事务、一次失败表示完全没写、CRC 是授权 |
| `ports/esp32/Esp32Storage` | NVS blob 与 commit，可配置最大长度 | 任意失败后立即 read-back 就证明数据已掉电持久化 |
| `ports/uno_r4/UnoR4Storage` | WiFiS3 Preferences 上已有双 key/generation/CRC/tombstone | 当前固定 5 个 blob、最大 260 B 自动容纳任务表 |
| `ports/wio_terminal/WioTerminalStorage` | 外部 QSPI 末尾两个 4 KiB 扇区交替、读回校验 | 任务高频执行适合重写含身份/网络凭据的整张快照 |
| `DeviceAccessStore` | PlatformAccess 非零 accessEpoch；Admin/Shared 受约束 | 普通 WiFi-only 已具备 owner scope；任意 permissions=All 都证明 owner |
| `Client` / `Property<T>` | 类型/约束校验、实际用户回调、正常状态上报 | `report()` 能执行硬件，或可伪造 RxContext 当任务授权 |

这些不是要重写现有模块的理由，而是新增任务接线的验收条件。

## 3. 四个职责，不建万能调度框架

```text
App/Service 的任务请求
    ↓ 现有 BBP/2 承载（Cloud / 已授权 Direct）
TaskCommandAdapter：网络解码、owner 验证、有界排队、回执编码
    ↓ 已验证的管理操作
TaskEngine：固定容量、CAS/请求去重、状态机、到期选择、提交预算
    ├─ TaskTiming + WallClockSnapshot：只回答时间
    ├─ TaskStore → IAtomicBlobStore：编码、完整性、原子提交
    └─ TaskTargetAdapter → 现有属性校验/回调/状态入口
```

`runtime/TaskEngine` 是内部纯核心，注入 `ITaskEnvironment` 与 `ITaskTarget`；`api/TaskPropertyTarget`、`runtime/TaskTimeEnvironment` 和 ESP32 Module承接属性/时间/持久端口。当前CommandAdapter、真实owner签发/验真及手动WiFi内部产品已接线并做有限真机；**正式公共任务opt-in/profile仍未发布**，不能再按第五批历史状态说整个适配器未实现。存储使用唯一`runtime/TaskSnapshotStore`，请求幂等/CAS只在Engine实现，适配器不复制事务逻辑。

- Engine 不 include WiFi、MQTT、BLE、NVS 或 App 页面代码；注入时间快照、存储与目标执行能力。
- Store 不决定是否到期、不查询账号；平台端口不理解任务定义。
- CommandAdapter 不自行执行 GPIO，不在 MQTT/BLE 接收回调内写 Flash；先校验/有界排队，由同一个 `run()` 消费。
- TargetAdapter 先按稳定 key 解析端点、验证类型/值/显式允许调度，再调用与普通属性控制共用的实际回调。不能直接改 State 假装执行，不能构造 authenticated 的网络上下文。
- 网络鉴权与本机已安装任务的执行来源分开表达；在有真实调用方时提取窄的共用属性应用函数，保留现有网络权限检查。低层自定义网络回调若没有本机执行合同，则不允许调度该目标。
- 一个设备一个实例，由 WiFi/BLE/WiFi+BLE composition 可选注入；网关子设备未来复用语义，不能现在就让云端和网关同时执行。

建议后续 facade 为独立 opt-in，并把允许调度的 Property 显式传入；具体拼写在最小消费者编译门禁中确定，不先把未实现的 `enableTasks(...)` 写成可运行示例。用户不传 workspace、Flash 对象或 owner proof。

## 4. 数据模型：定义、持久进度、易失状态分开

| 数据 | 主要字段 | 保存位置/生命周期 |
| --- | --- | --- |
| 表头 | schema、表长度、instance/128-bit scope/root 摘要、tableRevision、nextTaskId、最近管理请求 ID/摘要/结果修订、CRC | TaskStore，同一原子快照 |
| 任务定义 | taskId、definitionRevision、enabled、时间类型/参数、稳定 endpoint key、预期值类型、目标值 | TaskStore |
| 持久执行进度 | 最近 consumed UTC occurrence、倒计时 startSequence、最近执行标识与 Claimed/Completed/Failed/Skipped/Interrupted 状态 | 与对应定义在同一快照 |
| 易失执行状态 | 下一截止点、单调倒计时、上一有效时间快照、RAM 跳过高水位、当前 poll/提交标志 | RAM；断电不恢复单调截止点 |
| App 镜像 | 草稿/待确认请求、最近设备确认的定义/结果 | App/可选服务器；不是实际执行真值 |

首版持久目标值建议仅 Boolean、有限整数与有限浮点，复用既有 CBOR 数值/跨端范围校验；Text/Bytes/Object/Array 不先引入一个变长值堆。值是自有字节，不保存临时 ByteView、回调地址、session ID 或 Manifest ID 索引。稳定 key 沿用设备 64 B 上限。

定义修改递增 definitionRevision；删除后不复用 taskId；表修订与 ID 耗尽报错，不回绕。网络 requestId 与设备 taskId 不同，启动倒计时另有单调递增 startSequence；“设置启用”绝不隐式 Start。

持久编码显式读写固定端序、schema、长度和保留位，不 memcpy C++ struct。CRC 覆盖完整表，不提供机密性或抗恶意篡改。磁盘格式与 BBP/2 CBOR 是不同合同，不要求 Flash 直接存整帧或 App JSON。

容量设计预算：4 条编码快照不超过 **1 KiB**（8 条不超过 2 KiB）。第十一批 schema 2 为 `128 B header + 128 B × count + 4 B CRC`，4/8/10 条为 **644/1,156/1,412 B**；相对旧 schema 1 每表固定增加 56 B，不是每任务增加。仍仅一份解码表和不重叠的编码工作区，不常驻第二份解码备份。旧 u32 epoch 格式拒绝加载/自动迁移，早期资源和断电证据只覆盖其当时版本；新版软件/四档整固件静态资源见 §10.10，联网运行堆栈、未来管理接线资源与新格式真机仍待测。

## 5. 持久化：首版一张原子快照，执行写意图

### 5.1 为什么不用多个独立 key 或每秒写剩余时间

4 条低频任务很小。首版选择一个 TaskStore blob，**定义、最近管理回执和执行进度一起提交**，消除“配置已改、执行记录仍旧”的跨 key 事务问题。也不另外创建通用 WAL/数据库或为每个任务分配线程。

平台负责原子替换。构建候选→校验→replace 成功后立即同步内部表头/数量与已提交记录→复查授权/耗时→允许读取/回执。第十一批修正：**已经成功落盘、但随后暂失授权时，不能保留旧表头配新记录**；内部记住已提交状态并进入 AwaitingAuthority，仍不向未授权调用者公开内容或成功回执。真正存储失败/超预算继续锁定实例，不凭 dirty handle 读回恢复。没有第二份整表；候选/槽在 busy 期间原地暂存，读取/重入/执行被阻止。失败/断电只允许旧或新完整快照，不能部分拼接。

倒计时剩余毫秒、每轮 nextDue、普通时间刷新和网络状态**不落盘**。只有管理变化、倒计时 Start、执行 Claim/结果、必要恢复归一化才写。执行期限受写入耗时影响，不能宣称硬实时。

### 5.2 同一管理请求的重传

每次变更携带 requestId、完整操作和 expectedTableRevision；摘要由设备对已验证作用域与操作自算，不信任客户端提交的摘要。持久保存最近一次管理请求结果：

1. 与最近请求相同 ID/摘要：返回原提交修订，不重复修改、写盘或 Start。
2. 同 ID 不同摘要：冲突；不覆盖。
3. 其他请求且 expected 修订不匹配：冲突，要求重新读取。
4. 成功变更时递增 tableRevision，并在同一次原子提交保存回执。

执行 Claim/结果不改变 tableRevision，避免每次定时触发让配置编辑失效；底层存储 generation 是另一计数。只保留最近一条管理回执，不承诺无限历史重放。更旧请求到来时依靠单调 tableRevision 拒绝，客户端查询实际状态；删除/Start 也必须递增表修订。跨通道使用同一 requestId/expected revision，不为 BLE/Cloud 各建一条任务。

`replace()` 报错不保证“完全没写”。暂停任务模块并返回明确的未确认/需要查询状态，不把错误映射成“旧配置保证仍有效”。尤其 NVS 的失败 commit 后，当前 handle 的 load 可能只是未落盘状态，不能凭一次 read-back 执行动作。恢复必须由端口提供持久状态已确定的边界，最保守为重新启动后 load；UI 保留 Pending/Unknown。

### 5.3 到期后的顺序

```text
有效时间 + 当前 owner scope + 启用/目标合法
 → 选择一个 due occurrence
 → 原子保存 Claimed，并推进 consumed occurrence
 → 再检查时钟/迟到、scope 和目标准入
 → 调用一次属性应用入口
 → 原子保存 Completed / Failed / Skipped
```

保证的是：在存储合同成立时，**同一 occurrence 不自动重放**；不保证物理副作用恰好一次。调用前后不同时持有两个异步执行所有者，执行回调须有界且不可递归 poll。默认每轮最多一个执行候选，稳定 taskId 顺序；其他到期项在后续 poll 处理，超过迟到预算则跳过，不突发追赶。

| 掉电/错误位置 | 恢复行为 |
| --- | --- |
| 新配置提交前/中 | 恢复旧或新完整表；手机未收到回执则查询，不伪造成功 |
| Claim 提交前 | 尚未调用；重启按当前时间重新定基线，不补跑过去 occurrence |
| Claim 已持久、回调前/中/后但结果未持久 | 显示 Unknown；持久水位已消费该 occurrence，绝不自动再调回调 |
| 结果已持久 | 保留结果，不重执行 |
| 结果提交返回错误 | 本轮不重试回调；暂停后续任务，待存储恢复核对 |
| 记录损坏/未知 schema | 任务模块禁用并报告存储错误；不能当空表，也不能自动退回旧定义复活删除任务 |

底层“写到一半，选择旧完整值”与“已确认的新值后来损坏”是两种故障。首版不声称可对后者永久保证不复活；支持端口必须审查其回退行为，无法判定时任务模块 fail-closed。两扇区 journal 不是抗回滚安全存储，也不抵御物理修改。

### 5.4 磨损与写入预算

原子一致不等于适合高频写。首版按低频闹钟/倒计时设计：一次执行通常两次持久提交；4 个每日任务约 8 次/日，另加管理修改。不能将秒级采样或 Sketch 每秒回调安装为持久任务。

实现时配置统一持久提交配额（管理与执行都计入）、最小重新 Start 间隔、容量及平台写耗时上限；执行前要能预留 Claim 和结果的预算，不能 Claim 后才发现配额无余量。耗尽时可观察地暂停/跳过，不绕过落盘。具体额度依 Flash 型号/端口擦除放大及实测确定，不能为了给数字而假定擦写寿命。重启不得成为无限写入额度的正常业务机制，反复断电磨损仍属设备物理边界。

第四批采用**冷启动空令牌桶**：容量 `writesPerWindow`，每 `ceil(windowMillis / writesPerWindow)` ms 恢复一个额度。它限制平均速率并允许有界突发，**不是任意滚动窗口绝不超过 N 次**；所有提交尝试均消耗额度，包括初始化、管理、恢复归一化与失败写入。due 必须先有两份额度；额度不足返回 WouldBlock，候选保留，后续超过迟到边界只写 Skipped，不执行回调。未到期轮询没有写入。

每次 boot 从零额度起步，不能靠重启获得免费突发；初始化或恢复可能等待首个额度。Start 另有设备级间隔（包括 boot 后首个 Start），删除重建不绕过；以每轮单调增量递减剩余冷却，避免运行超过一个完整 uint32 tick 回绕后误恢复已过期冷却。`TaskWritePolicy` 不给未实测的发布默认值；平台需同时验证正常交互/同时到期容量、最坏写时延及擦除放大。测试用的快速补充额度仅用于确定性模拟，不能照搬成产品耐久配置；也未声称防止反复物理断电磨损。

## 6. 三个平台如何存

| 平台 | 推荐接线 | 必须补的门禁 |
| --- | --- | --- |
| ESP32 | 第五批内部 `Esp32TaskModule<N>` 已组装独立 `bl_v2_tasks/snapshot` 和现有原子 NVS 端口，容量按表配置，不改凭据 blob | fake NVS 提交/恢复/容量门禁已过；真实提交/断电、整机增量及栈峰值待测；默认产品不实例化 |
| UNO R4 WiFi | 复用既有 Preferences 双 key journal 的机制，新增**独立任务存储视图/命名空间**；先抽出当前实际可复用的 envelope，不复制整套凭据银行 | 不能只增加 blob ID 而忘记 260 B scratch 上限；桥接单次长度/阻塞时间实测，不能拆成无原子性的多次裸写 |
| Wio WiFi | 外部 QSPI 为 tasks 分配独立双扇区区间，复用擦写/校验机制，不挪到 SAMD 固件 Flash、不和身份快照共享高频改写 | 当前末尾 8 KiB 已被身份银行占用；新增区域先核对用户文件系统/分区占用与升级边界，未确认不得自动格式化或擦除 |

第一份真实接线先 ESP32，另两平台先 host 端口测试和编译，不要求为了任务核心改 BLE/WiFi 协议。R4 的长调用和 Wio 双扇区不均衡磨损如不满足预算，暂停该平台任务声明，再评估小型多扇区 journal；不预先给所有平台添加复杂 WAL。

刷写通常保留任务的前提是该存储区未被擦除；完整擦除后不保证恢复。服务镜像只能让 owner 显式重新安装，不能烧录后后台静默恢复。固件更新按稳定 key/type/允许调度标记重新核对；端点失效则暂停，不因其他无关 Manifest 变化删全表。

## 7. 时间、校正与执行游标

日历准入用同一次 WallClock 快照，保持现有最大样本年龄 1 小时、最大误差 5 秒/1,000 ppm 默认预算；1,000 ms 是 loop 迟到宽限，不等于精确到一秒。当前默认策略不能保证 BLE 数天离线闹钟。

本批新增 `WallClockSnapshot.sampleRevision`：每次接受新样本递增，正常 poll/read 与失败的授时不变；clear/过期不重置编号，重新授时继续递增；新 boot 从零开始，溢出拒绝新样本不回绕。它只提供可观察性，不调用执行器、不改鉴权时间、不进入授时 wire。

执行器时间策略拟固定为：

- 无有效时间→WaitingForTime，保持已消费水位；有效样本回来后重新定基线，只安排严格未来 occurrence。
- 连续有效快照且 tick 合法→以先前 armed 截止点判断跨越；不能每轮 `nextTaskUtc(now)` 把刚到期点跳走。
- sampleRevision 变化时，比较新 UTC 与“上次 UTC＋实际单调经过量”。修正绝对值不超过 1,000 ms、前后都满足误差预算时按有界小校正处理；超过则重新定基线，不将校时前跳当成自然到期。cursor 与第四批 Engine 提交后的复查共用 `taskCalendarContinues`，不复制两套校时策略。
- RAM 跳过/消费高水位防止本次启动内回拨再跑；持久 consumed occurrence 防止已 Claim 的任务重启再跑。普通每轮和每次校时不写 Flash；RAM 中跳过但未 Claim 的历史不是永久执行账本。
- Claim 落盘后再次检查当前时间、scope、目标与迟到预算；跨过大校正/超时则记录 Skipped，不因为 Claim 已写就强行动作。
- 同一任务的日期 occurrence 使用定义版本＋UTC 截止点；倒计时使用定义版本＋startSequence。倒计时不受 UTC 校正影响，重启显示 Interrupted；断网不相当于重启。

重启时不以 Flash 的旧 UTC 当当前时间；不恢复旧 monotonic deadline。需要 RTC/长期保持属于独立时间源增强，不应在任务模块里复制第二个永不过期时钟。

## 8. 所有权与管理准入：尚不能跳过的三端接线

普通控制与持久任务管理分开。内部 Engine 接收**由可信适配器确认的 scope**，不把请求 body 的 owner/deviceId 当证明；装入和每次执行都必须与当前 scope 相符。退休/更换所有权先停止执行、再清理存储，不要求两个 blob 的删除是一个事务。

- **BLE**：Method 2/安全 record 证明当前本地通信凭据，不直接等同云端 owner。首版与 Cloud 一样转发服务器签名任务授权；不为任务另存每连接 Admin 身份，不将 `sessionPermissions()==All` 或 accessEpoch 当作管理授权。已有普通 BLE 控制/分享权限不变。
- **WiFi-only/Cloud**：当前 RxContext 只有 transport/session/authenticated，不含终端用户 owner；现有手动 DeviceKey 登录不保证 MCU 已持有上述本地 Admin/epoch。因此不能只加一个 kind 并沿用普通 Control 门禁。
- **两通道统一管理合同**：服务器验证真实账号 owner 后，签发绑定设备挑战、请求 ID、完整管理操作摘要和独立任务 scope 的一次授权；MQTT/BLE 仅承载。操作摘要须包含 expectedRevision/操作类型/全部参数，不能授权后再改写。任务 scope 使用服务生成的非零随机 128-bit 值，不从 accessEpoch、MAC、DeviceKey 或 credentialVersion 推导。Grant 的内部 codec/摘要/验签接缝已实现；scope 建立、单次挑战与三端业务入口**仍未实现/未开放**。
- 混合设备两种入口必须落到同一 scope/tableRevision/requestId；Key 刷新不自动清任务，更换 owner 使旧任务先失效。普通断线不能清表。
- 离线设备无法立即知晓云端移除；以设备实际收到的退休/物理 reset 为停用边界，不能承诺离线瞬时撤销。重认领必须确保新代际不会与历史任务 scope 碰撞；只比较一个可重用的布尔“已绑定”不够。

此处没有增加 MCU 的账号数据库，也没有要求普通 MQTT 全部开启 TLS。新增的是长期本机任务的**管理准入**，不是重做每帧安全。签名 proof 超出现有 frame 预算时，先缩小合同/复用有界控制通道，再讨论传输扩展；不偷偷放大所有 buffers。

### 8.1 第六批源码复核：不能把现有权限数字直接接成 owner

2026-09-22 核对本地三端源码（没有修改或部署 App/Service）：

- `transport/IFrameTransport.h::RxContext` 没有账号或 owner 证明；`authenticated` 只属于该承载。普通 MQTT Control 不足以管理持久任务，TLS 也不会自动补出终端用户身份。
- `security/ControllerHmacSha256Authorizer.inc` 在验证 proof 并调用 `directSessions_.prepare()` 后立即清掉握手槽；`DirectSecureSession` 保留安全 record 材料而非完整 controller 身份。不能在任务模块里回读这个已清掉的槽，或把一个活跃 sessionId 当作 Admin。若复用本地角色准入，应在原有 prepare→commit→cancel 生命周期中保留最小已验证身份，并在撤权/轮换/断开时失效；不要延长 nonce/secret 生命周期或再建一套 BLE 会话表。
- BLE 认领的 `accessEpoch` 由设备产生随机非零 u32；服务器 `ble-enrollment.js::requireFreshClaim` 检查已退休认领和 reset 历史，阻止重新使用。另一条 `direct-access.js` 初始 Admin 记录从 epoch=1 开始，reset 使用 CAS 递增。**它不是全系统统一递增的 owner 序号**；不能把两条路径的相等整数直接作为任务所有权连续性的证明。
- 因而当时 TaskSnapshot schema 1 的 epoch 不能作为生产授权依据；第十一批已由 §8.7 的完整 binding 替代。T0b-2 仍须给出“逻辑设备/物理 instance/当前授权根→任务 scope”的可信映射和不可复用检查；不增加 WiFi-only 假 Admin 凭据来凑 scope，也不把 Key version 当 scope。
- 三端 kind 表不完全相同：App/Broker 已占用 `0x1c～0x1f`、`0x28～0x2d`；MCU 的 `0x34/0x35` 已用于 PresenceKey。只看设备 enum 的空位会冲突。本批不分配任务数字，不宣告 feature；冻结时应连同路由白名单/最大帧一起做三端检查。

### 8.2 T0b-2 的下一项可执行合同

先完成以下小合同，再实现 TaskCommandAdapter；不重新规划整个通信栈：

1. **scope 生命周期表**：明确首次手动 WiFi、首次 BLE、同一混合设备、普通 Key 轮换、Admin 根替换、物理 reset、owner 移除/重认领各自保持或退休的条件。当前可信 scope 在启动任务前就绪；不能先执行旧任务再异步查询所有权。
2. **操作与授权分层**：Create/Replace/Remove/SetEnabled/Start/初始化和读取分别确定准入；读取定义也可能暴露私人作息，不能默认向所有 Control 分享者开放。首版 Cloud/Direct 都采用设备挑战＋服务器签名授权，复用同一 Engine 的业务操作与 CAS/去重；不实施第二套 Direct Admin 管理分支。网络适配器不能自行改变 requestId/expectedRevision。
3. **先测包长再冻结 wire**：最大 64 B key、最坏整数/日期、16 B requestId、设备挑战、签名及 Route/Delivery 包装都计入 Small 的 256 B 帧预算。不能仅以原有 ControllerGrant 的 193 B 或签名 64 B 证明“任务也放得下”。本批测得单包失败，优先采用下述已通过包长实验的“一个暂存操作→挑战→授权引用摘要”方向；未经测量不新增通用分片或放大全设备 buffer。事务/授权/golden 尚未完成，不能视为已实现协议。
4. **队列只有一个业务所有者**：授权后的请求先有界入队，在 `run()` 中执行前重查当前 scope/凭据状态；网络回调不写 Flash。Prepare 暂存与执行队列不各存一份整表；首版最多一笔待授权操作，有速率/占用时限，挑战超时用单调 tick 而非业务 UTC，取消/过期/断开后的迟到签名不能串用到下一个请求；提交成功而 ACK 丢失仍查询/重放原 Engine 回执，不创建新任务或再次 Start。
5. **跨端 golden/拒绝测试**：错误 instance/scope/签名/摘要、Shared/Local、凭据轮换、scope 退休、挑战过期/复用、最大帧、旧请求/版本冲突、掉电后 Unknown、查询信息泄漏。明文 MQTT 的传输回执没有天然防篡改保证；管理回执的验证边界须一并说明，不把签名下行等同于双向安全通道。

这份清单明确了源码差距，**不将 T0b-2 标为完成**。内部资源探针使用 scope=0 并拒绝授权，不能成为绕过这份合同的“示例实现”。

**第六批包长比较（历史证据）**：当时组合三种合法时间形态、64 B UTF-8 key、9 B 整数/浮点值、最大宽度修订/任务 ID、16 B requestId/instance/nonce、64 B 签名占位，比较两个 scope 宽度。结果包含 10 B BBP 头及可选 4 B CRC：

| 候选帧 | u32 scope | 128-bit scope | Small 256 B |
| --- | ---: | ---: | --- |
| 完整操作＋签名一次发送 | 295 B | 307 B | 均不满足 |
| Prepare 操作 | 182 B | 182 B | 满足；尚未授权、不得写 Flash/执行 |
| Grant 引用 requestId＋操作摘要 | 223 B | 235 B | 满足；余量 33/21 B，不代表可任意加字段 |

第十批选定 128-bit scope，`test_task_management_budget_main.cpp` 已删除 u32/单包候选的重复编码分支，改为调用真实 Grant codec；当前回归为 Prepare 182 B、Grant 235 B。仍只借用一个既有二字节 kind 的编码宽度测量外层包络，**不能将任务当 TelemetryData 发送**。Grant 的内部字段/签名域见 §8.6；Prepare 挑战回应、回执、分页读取与完整事务仍待冻结。未经过 App/Service 真实业务接线，不能把 C++/Node golden 标成三端调通。上表不含 MQTT topic/TCP/IP 或 BLE 安全 record/分片开销，后者属于不同层。

### 8.3 真实 NVS 门禁的最小执行边界

资源探针仅用于编译，不能直接烧录后当作持久化验收。硬件批次需单独满足：

第六批用户授权 **COM3 的新增 XIAO ESP32S3**，要求先核对/备份、完成后恢复，COM11/COM12 不动；第七/八批已按此执行，证据见 §10.6/§10.7。以下是持续适用的硬件准入条件，不以授权本身代替实测。

1. 读取并记录专用板的端口、物理身份、分区表和现有固件；取得可读回校验的固件/NVS 恢复材料。COM 号不是身份，未核对前不自动选列表首块板，不动其余见证板。
2. 在明确隔离的测试任务区域中测首次打开、缺表读取、空表/4/8/10 满表、初始化、修改、Start、Claim、结果和恢复归一化。不能用空表写入时延代表满表/空间压力。`nvs_open(..., NVS_READWRITE)` 可能创建命名空间元数据；“没有写任务快照”不等于 SDK 没有分配空间或写任何 Flash。
3. 分别记录模块调用耗时、任务所在 loop 的延迟、堆/最小堆、任务栈水位及 NVS 空间变化。空闲 poll 无快照写、普通联网状态不变；cold/已有 namespace、空间压力等样本分开，不以三次正常写的最大值宣称芯片绝对最坏值。
4. 重启、实际断电要覆盖管理提交前后、Claim 已持久但回调前、回调后但结果提交前等可观察边界。软件错误注入继续测“写后报错”；外部断电不能保证命中任意一条 Flash 指令，报告实际命中窗口，不宣称穷举所有掉电点。
5. 每次恢复先核对完整快照/修订/最近请求及进度，再观察回调计数；持久 Claimed 不自动重放，恢复倒计时 Interrupted。仅 after-reboot 成功读取才作为该次恢复证据，失败 handle 内的 read-back 不算。
6. 测试完成恢复原固件/原授权并读回校验，再验证原有连接。测试数据只按已确认的独立 namespace/分区清理，不调用整片 erase/恢复出厂来掩盖失败。硬件测试策略不得变成公开任务授权的后门。

只有取得这些结果后，才给 `TaskWritePolicy` 选择产品预算；当前编译 fixture 的数值不属于寿命、延迟或发布时间保证。

### 8.4 生命周期顺序与本批落地的最小清理接口

不能把“scope 不同就 retire”当作完整重置方案。`retire()` 是 **RAM 内终止**，掉电不改变旧快照；旧 `accessEpoch` u32 候选已删除，当前完整 binding 也不能单独证明跨路径所有权连续性。以下行为先固定，可信 scope 的签发/消费仍归 §8.2：

| 事件 | 已安装任务 | 生产接线要求 |
| --- | --- | --- |
| 普通启动、重连、App 退出 | 保留定义；倒计时按 Interrupted，日历按有效时间重新定基线 | 当前持久授权来源明确后才启动 Engine；手机未连接不等于 owner 消失 |
| 网络变更、DeviceKey 常规刷新 | 不因 SSID/Key/version 改变而清任务 | Key 不是 task scope；不以清任务修复普通联网故障 |
| Shared 撤销/成员变化 | owner 安装的任务保留 | 拒绝 Shared/Local 管理；共享凭据变化不替换 owner scope |
| 同 owner 的 Admin secret/version 轮换 | 保留须有可信连续性；旧会话/待授权请求失效 | 不只比较权限数字或整数 epoch；根身份变化而无连续性证明时停止，不猜测继承 |
| owner 移除、确认物理重置、更换 owner | 先停止，再持久清任务，之后才能清/替换授权根 | 清理未成功不能报告设备端移除完成；不可达设备不承诺立刻停止 |
| 管理表读取 | owner-only，与创建/修改一致 | 内部 `recordAt()` 不是可直接暴露的网络查询 |

**最小顺序**（第九批可选组合见 §8.5；默认产品/服务器退休事件接线仍待做）：可信退休事件 → 阻止新管理请求/丢弃旧 pending → `engine.retire()` → cooperative 上下文调用 `engine.clearRetired()` → 成功后继续既有授权清理/退休确认。原生线程只投递事件，由唯一产品运行上下文处理；属性回调内可立即 retire，但清理返回 WouldBlock，退出后再做。

本批已实现 `TaskSnapshotStore::clear()` 与 `TaskEngine::clearRetired()`，后者只接受 Retired，成功转为 **Cleared 终态**。重复调用不再触碰存储，同实例不得重新 initialize；重认领需要新 scope 与新生命周期。只清任务 blob，不清凭据、不格式化 NVS、不声称安全擦除 Flash。清理是一次终止事务，不受普通任务写令牌限制；成功后不重写、失败后锁定，不是正常管理的无限重试通道。

存储端 `clear() == NotFound` 仅在**已确认持久不存在**时代表幂等成功；ESP32 端原本就先 commit，再返回 NotFound。失败 handle 不允许读回或再清来猜测结果。Store 必须先成功验证快照/确认缺表或提交有效新表，尚未读取、格式损坏和读/写失败均不能清理；用一个单字节 Unloaded/Ready/Failed 状态替代原失败标志，不增加存储布局或把清理变成自动格式化。

清理成功后重启缺表，不会恢复旧任务；清理失败/完成前断电则旧表或缺表都可能存在，且授权根必须尚未退休，移除仍未完成。生产接线还须明确持久移除事务及启动恢复次序；没有接齐之前，不能承诺“点击移除后无论在哪一步掉电都永不再执行”。主机测试保留该失败边界，不用全片擦除掩盖。

第八批没有新增 controller 身份表、保留握手秘密、分配 task kind 或改变 scope 格式。第九批已实现下述最小可选生命周期；scope/Prepare/Grant 的单笔队列和跨端 golden 仍是 T0b-2 未完成项。

### 8.5 第九批：可选生命周期组合与生产启用边界

`api/TaskProductLifecycle<Module>` 是内部 composition，不是新的 Arduino 用户入口；只依赖既有 `IProductLifecycle`、TaskEngine 和属性适配器，平台只提供 Module 的 `begin()/engine()`。默认 `Product`、WiFi/BLE facade、平台凭据存储不增加任务字段或依赖。当前 ESP32 Module 是实际编译消费者；不能由此推断 R4/Wio 存储已适配。

- **attach/start**：沿用原产品 attach/time/prepare/start；Target 在 attach 时绑定同一个 Client，不再用测试扩展捕获私有 Client。底层启动成功后只尝试一次 Module.begin；普通 reconnect/resetNetwork 不重开任务 handle、不重载或清表。模块的存储必须在底层产品 stop 后仍有效；当前独立 ESP32 NVS handle 满足这一组合条件。
- **run**：先 poll 原通信产品，再检查任务状态，最多执行一个到期任务。通信本轮提供的 scope 退休在动作前生效；没有第二个 run owner/线程。同步 Flash 时延不受传入的 `totalBudgetMicros` 硬限制，仍须通信共存准入。
- **可用性隔离**：任务存储打不开/格式坏不让普通通信一起变砖；内部 `taskError()` 留下原因，Engine 不执行。未成功验证任务存储则 resetAccess 也不能绕过它清根；维修恢复是另一项显式策略，不自动格式化来“修好”。本组合不宣告任务 feature。
- **物理 resetAccess**：先核对原产品是否支持，retire 并停通信；clearRetired 成功后才调用原产品 resetAccess。清任务失败不触碰授权根；清根报错则根可能已变更，锁定本生命周期，不能自动开始/重试并假定回滚。任务清理已成功的缺表状态保持幂等。
- **回调重入**：网络 poll/属性执行栈内请求 reset 时立即 retire，返回 WouldBlock，退出栈后才 stop 底层；不在回调内清任务/根。调用者在 cooperative 上下文显式再次 reset 完成清理，未完成前 begin/resetNetwork 拒绝。原生 SDK 回调仍只投递事件，不能直接调用本组合跨线程操作。
- **再次认领/再次重置**：成功 reset 后原产品可重启配网/认领，但旧 Engine 保持 Cleared。产品启动会废弃旧的“根清理成功”缓存；后续 reset 必须清掉可能新建的授权根，不能用上一次成功回执跳过。新 owner 的任务需要新可信模块生命周期，尚未提供同对象重新启用 API。

**本轮清查到的真实边界**：`PlatformBleOnlyComposition` 和 `Esp32WiFiBleProduct` 清 `DeviceAccessStore`；Hub 先清南向再清自己的 key。包装不会复制或颠倒这些内部顺序。**手动 WiFi Product 当前不提供 AccessReset 能力**，组合不擅自添加，也不把 Sketch DeviceKey 当可擦除的授权根。网络清理保持原语义。

**未完成且不得自动放行**：ControllerControlCoordinator 的 Install/Rotate/Revoke 不经过 Product.resetAccess，不能把每次 Shared 变化都清任务，也不能把同 epoch 的 Admin 替换当连续 owner。须先确定可信 scope/根连续性，并让网络管理的单笔 pending 与已验证退休事件接入唯一执行上下文。当前没有 TaskCommandAdapter/pending，不能声称本组合已经取消了真实网络事务。

当前证据是 **真实 Product → 可选组合 → TaskEngine/ESP32 blob/DeviceAccessStore 的主机故障注入**及真实 WiFi 图的编译链接；底层通信在主机为 fake lifecycle。默认 Arduino 三种产品尚未实例化任务，尚无 BLE/WiFi 生产 owner 的端到端运行证据。无需再写一个通用插件总线；下一包直接推进 §8.2 的 scope/管理授权合同与测试。

### 8.6 第十批：统一签名管理与独立 task scope（2026-09-23）

本节取代早期“Cloud 签名、Direct 保留 Admin 元数据”的双分支建议。**首版 WiFi/BLE 都使用设备挑战＋服务器签名管理授权**；普通属性控制的现有准入不改，不强制明文 MQTT 改 TLS，也不要求 BLE 设备自己联网。BLE 模式由 App 在线取得授权后原样转发。代价是初始化、安装/编辑/启停/删除和读取设备任务都需要 App 可访问服务；离线草稿不生效。已经安装的任务可按现有时间有效性和生命周期规则离线执行，手机不必常驻；这不扩大日历时钟的保持期限。

**任务 scope 的唯一语义（设计确定，持久化接线未完成）**：服务在“逻辑设备＋物理 instance＋本次所有权任期”下产生非零随机 128-bit scope，检查当前及已退休 scope 不复用。两条通道必须查同一映射；它不是账号 ID、MAC、Admin accessEpoch 或 Key 版本的缩写。Key/Controller 凭据轮换是访问材料变化，不自动等于换 owner。

| 场景 | 任务 scope / 旧任务 | 必须先成立的证明 |
| --- | --- | --- |
| 首次手动 WiFi / BLE 认领 | 新 scope，空表；不自动导入历史任务 | 服务验证真实 owner 与已注册物理 instance，设备挑战及当前授权根连续性成立 |
| 同一混合设备从 Cloud 换 BLE | 同一个 scope/tableRevision/requestId | 只是换承载，不以相同 MAC 或相等 epoch 合并身份 |
| 正常重启/短断网，授权根未变 | 保留并装载已确认 scope；无需每次启动在线取票 | 本地持久映射与当前 instance/根绑定一致，格式和表完整；日历仍重新授时 |
| 同 owner 刷新 DeviceKey / 轮换 Admin 根 | 可保持 scope/定义，但根改变期间暂停，待连续性证明后恢复 | 服务签名证明仍是同一逻辑设备/instance/任期；不能只比较旧 epoch，不能先执行再异步确认 |
| 修改 Shared/Local / 临时云 session 续期 | 不改变 owner scope、不清 owner 的任务 | 使用既有分享/凭据规则；不得把临时 MQTT 登录材料当持久根 |
| Sketch 换成另一设备的 Key / instance 改变 / 无法证明根连续 | 旧表不执行、不向新身份公开；不猜测自动迁移 | 必须重新核对 scope；修复失败不能自动格式化或沿用旧数据 |
| 物理 reset / owner 移除 / 新 owner 认领 | 先退休、取消 pending，再按 §8.4/§8.5 清表；新任期用新 scope | 真实退休事件或物理操作；离线云移除仍不能承诺瞬时到达 |

第十一批已将最小元数据放回**同一任务原子快照**，保存 instance/scope/rootDigest，并同步更换 Engine/TimeEnvironment 的 u32 候选接口，详见 §8.7。没有账号表、明文 Key 或每连接 Controller 表；绑定摘要只判本地连续性，不自证 owner。实际持久根的有域摘要输入及初始化/换根签名合同仍待接线，不能以测试 fixture 充当来源。旧 schema 1 拒绝自动迁移，**不得截断/补零扩展 scope 或散列为 u32**；也没有擦除现场任何数据。

**已实现的 Grant v1 内部合同**：`provisioning/TaskManagementGrant.h/.inc`，只有一份固定编码/解码/摘要/验证实现，不新增 SDK 依赖或会话状态。

```text
body = [1, instance:bstr16, taskScope:bstr16, nonce:bstr16,
        requestId:bstr16, operationDigest:bstr32, keyId:u32, algorithm, signature:bstr64]
signedDigest = SHA256(canonical-CBOR([
    "blinker.task-grant.v1", 1, instance, taskScope, nonce,
    requestId, operationDigest, keyId, algorithm]))
```

- 四个 16 B 字段非零；keyId 非零；算法复用 `IServerSignatureVerifier` 的 Ed25519=1、P-256 raw r||s=2。验签公钥由库已有的受信 key ring 选择，不能来自 grant。Ed25519 签摘要本身，P-256 验证该摘要，不能额外再哈希一次。算法字段也在签名域内。
- 只接受有界、确定长度、canonical CBOR 和精确字段宽度；拒绝未知版本/算法、超长/截断/尾随内容。输出借用输入，失败时不留半个 decoded grant；验签错误按原错误返回。Grant body 最大 **176 B**。
- 摘要由 CBOR Writer sink 直接进入原 Sha256；无完整 transcript scratch，无动态分配、长期状态或存储写入。不等于零栈开销，验签 provider 自身仍需资源预算。
- 通用 verifier 只接受**设备自己持有的当前 scope 和 pending binding**，精确比对 instance/scope/nonce/requestId/operationDigest 后调用注入的验签器。空 scope 不表示“允许初始化”。第十二批另设明确的初始化/换根操作子合同（§8.8），复用同一 Grant 验证器；它仍须设备 pending/存储状态检查，不能拿一份签名直接建立 owner。
- verifier 无状态，**不消费 nonce、不判断超时、不直接授权 Engine**；同一份数据重复验签可以仍成功。后续唯一 pending 所有者须检查单调 TTL、通道/生命周期取消、一次消费、根/scope 当前状态和完整操作摘要，在 `run()` 提交前再次复查。异步验签后不能只记一个永久 `verified=true`。
- `operationDigest` 最终必须来自完整 canonical Prepare（含 requestId、CAS、操作和全部参数）的有域摘要；不是 Engine 私有重放摘要（当前 BTM2），不接受请求方另提供未对应暂存操作的 hash。Prepare 的正式 codec/挑战响应/执行回执/分页仍未完成。
- 签名下行不让明文 MQTT 自动变成双向加密/认证通道；读取任务可暴露作息，首版不承诺明文承载上的保密。Installed 的可信回执与查询返回绑定仍是 T0b-2 准入项，不能拿传输 ACK 或未认证 JSON 回执替代。

**接下来按依赖顺序，不新建管理框架**：① 复用 §8.8 根/变更合同，接生命周期已验证根缓存与统一有界单笔 pending（含一般 canonical Prepare、挑战消费及授权读取）；② 三端统一 kind/feature/回执与真实 owner 签发，接现有可选产品生命周期；③ 通信共存资源与 T2 安装闭环。Grant、根/变更和持久化子合同不等于 T0b-2 整体完成。

### 8.7 第十一批：原子授权绑定与可恢复的等待状态（2026-09-23）

`runtime/TaskAuthority.h` 仅包含自有的 64 B 元数据与状态值，无虚拟管理器、线程、协议或平台依赖。`TaskSnapshotHeader.authority`、可信内部 `initializeVerified/mutateVerified/rebindVerified` 和 `TaskTimeEnvironment::AuthorityReader` 统一使用它，删除旧 epoch 入参及适配，不增加兼容整数重载。有效绑定必须有非零 instance16、scope16 和 rootDigest32；完整宽度比较，不允许同前 32 位就命中。

**磁盘 schema 2**（大端整数，magic 仍为 BTS1，schema 字节改为 2）：

| 偏移 | 内容 | 字节 |
| ---: | --- | ---: |
| 0 / 4 / 5 / 6 | magic / schema / reserved=0 / count | 4 / 1 / 1 / 2 |
| 8 / 24 / 40 | deviceInstanceId / taskScope / rootDigest | 16 / 16 / 32 |
| 72 / 76 / 80 / 96 | tableRevision / nextTaskId / requestId / requestDigest | 4 / 4 / 16 / 32 |
| 128 | 原有任务记录，字段布局不变 | count × 128 |
| 表尾 | CRC32，覆盖授权元数据与整表 | 4 |

拒绝未知 schema（包括完整有效的旧空表）、错误长度/CRC/字段；失败不发布半张表、不自动格式化。CRC 不是授权 proof，也不抗离线改写/回滚 Flash。生产初始化还须签名证明，不因字段“看起来合法”就建立当前 owner。

**环境与执行状态分开**：

- 环境 `Pending`（默认、根未确认或无有效绑定）且单调 tick 正常→已加载 Engine 进入 `AwaitingAuthority`，停执行/管理/定义读取，不写 Flash；缺表仍是 Empty，不能偷偷初始化。TimeEnvironment 未配置 tick 时不调用授权 reader；运行中 tick 回退/间隔异常仍按原时钟合同 Fault，不能用 Pending 隐藏。
- 环境 `Current` 且同 instance/scope/root →从等待恢复；重新给日历定基线，不追赶间隙内到期点，保留已有消费/跳过水位；CountdownArmed 经原恢复逻辑持久改 Interrupted，不重新 Start。Claimed 保持 Unknown，不重放回调。
- 环境 `Current`、instance/scope 相同但 root 不同 →仍等待；只有可信层验证连续性后调用 `rebindVerified(expected, replacement)` 原子 CAS 更新同表中的根。拒绝跨 instance/scope 或与实际环境根不符的 replacement；成功保持定义、tableRevision、requestId 和回执，重复已提交换根不再写。
- 环境明确 `Retired` 或可信 Current 映射指向不同 instance/scope →终态 Retired，旧值回来也不复活。只有 RAM 退休不等于持久删除；沿原 retire→clearRetired→清根顺序处理。
- 真正 replace/commit 报错→Fault/Store 锁定，不能当 Pending 等待后自动恢复。冷启动只能读取端口确认的持久结果，旧完整绑定则等待证明，新完整绑定可加载；不靠读回 dirty handle 判断。

`TaskProductLifecycle` 继续轮询 AwaitingAuthority，不重开 NVS 或重建 Engine；通信照常运行，taskError 可观察等待/拒绝原因。内部查询不是网络 owner 验证入口；未来管理适配器仍要保护整表/count/revision/分页输出。没有任务功能默认开关变化。

本地管理去重摘要改为 `BTM2`＋完整 128-bit scope；不将 root 摘要混入操作身份，确保同 owner 换根后原请求仍返回原回执、不再次 Start。scope/root 变化期间不发送成功安装确认，客户端最终必须用已认证查询/原 requestId 核对；即便 store 已成功写入，也不能把 WouldBlock 当成“保证没写”。

**尚未完成的可信输入**：WiFi 产品已有 `DeviceInstanceIdStore` 和 `IDeviceKeySource`，不能用 MQTT 临时 username/password 作为根；BLE 从现有 AccessStore 的 Admin 根确认，不纳入普通 Shared 改动；混合模式需要同一明确的根摘要合同。此批没有计算这些生产摘要、让环境读取器从表头自证 Current、接服务初始化 proof 或把上批 Grant 当成 scope 建立授权。下一批直接补这些来源/签名与 pending 边界，不再复制快照/生命周期框架。

### 8.8 第十二批：实际根来源与初始化/换根合同（2026-09-23）

第十二批的 `provisioning/TaskAuthorityContract.h/.inc` 纳入原责任单元，只实现读取/编码/摘要/验签函数；这些函数仍不写 Flash、不调用 Engine、不请求服务器、不建立凭据表。第十三批在同一模块补可选根缓存，另用唯一 Session 接 Engine，见 §8.9；默认产品和 Arduino API 不变。

**根来源由产品固定，不能由请求/当前承载决定**：

| 产品 | 当前根摘要输入 | 不允许的替代 |
| --- | --- | --- |
| 手动 WiFi | 实际 `IDeviceKeySource` 读取的 DeviceKey；已加载的 DeviceInstanceId | MQTT 临时口令、假 Admin、MAC |
| 纯 BLE | 实际 `IControllerCredentialSource.loadActive` 返回的 PlatformAccess Admin | Shared/Local、错误 ID/epoch、无效凭据 |
| WiFi+BLE | 同时要求 DeviceKey 和 Admin，两条通道共用同一摘要 | 任一根缺失时临时退成单根模式 |

`TaskRootSources` 是借用源/selector，不是证明或长期状态。Admin selector 必须来自已安装/验证生命周期；函数仍复查返回凭据的 domain、epoch、ID、完整权限及格式。权限检查只确定“哈希哪个根”，**不授予任务管理权**。Shared 安装/轮换/撤销不参与摘要。读取可能访问 Flash，只能在启动/根变更等有界管理点执行，不能每轮 `poll()` 做存储 I/O。

根摘要精确为流式 canonical CBOR 的 SHA-256：

```text
rootDigest = SHA256(CBOR([
  "blinker.task-root.v1", 1, instance:bstr16,
  deviceKey:bstr32 | null,
  [suite, domain, accessEpoch, credentialVersion, permissions,
   controllerId:bstr16, adminFingerprint:bstr32] | null
]))
adminFingerprint = SHA256(
  UTF8("blinker/direct-admin/fingerprint/v1") || 0x00 ||
  rawControllerId16 || credentialVersion:u32BE || rawControllerSecret32
)
```

**跨端复核修正**：Service 只保存 `admin_fingerprint`，不持有 raw Admin secret。因此不能直接把 Admin secret 放进需要服务器重建的根 transcript。设备从原存储读取 secret，先算已有约定的指纹；临时 secret/指纹在退出时清理，不新增秘密数据库。DeviceKey 由 Service 现有 Vault 解封后计算；带 Key 的哈希 transcript 不能打日志或发送给 App。

源码依据：App `core/device-v2/ble-direct/crypto.ts::adminFingerprint` 当前写固定版本1；Service `device-v2/direct-access.js` 保存 ID/version/fingerprint，BLE 认领 commit 同步该元数据，`repository.js/vault.js` 可解封 DeviceKey。本合同在版本1与当前 App 一致；**未来大于1的 Admin 轮换须让 App 指纹参数化，并以已验证安装/轮换结果更新 Service 元数据 CAS**。没同步时拒绝签发/保持等待，不沿用旧 fingerprint。当前 registerAdmin 元数据也不是设备持久安装证明，任务 signer 仍须核对认领/receipt 状态。本批只读核对三端，没有修改或部署 App/Service。

**初始化/换根操作子合同**：

```text
change = [1, requestId:bstr16, instance:bstr16, scope:bstr16,
          previousRoot:bstr0|32, currentRoot:bstr32]
operationDigest = SHA256(CBOR(["blinker.task-authority-change.v1", ...change]))
```

- previousRoot 空字节串明确为 Initialize，只能在设备确认 Empty 时暂存；32 B 为 Rebind，旧根/scope 必须来自已验证持久表，新旧根必须不同。错误宽度/零 ID 或根/版本/非 canonical/尾随数据拒绝，无空 scope 通配。
- `verifyTaskAuthorityChangeGrant` 使用设备持有的 expected/nonce、独立观测的当前 instance/root，匹配后复用 **原 Grant v1** 验签。服务生成的 scope 在提议/解码阶段仍不可信，不能拿普通通用 Grant 的空 expected 作为 bootstrap。
- helper **无状态**：不消费 nonce、不检查 TTL/Empty/CAS、不持久写入或恢复执行。下一批统一 pending 所有者负责根/通道变化和超时取消、防重入、提交前再校验，再调用原 initialize/rebind。不得为初始化单独再建一个执行队列。
- body Initialize88 B/Rebind121 B；包含既有 Route/Delivery、10 B BBP头和4 B CRC 后 **147/180 B**，Grant仍235 B，满足256 B预算。测量只借用现有二字节 kind 宽度，未分配任务 kind/宣告 feature，不把任务当 TelemetryData 真发送。
- 换根重试沿用 Engine 精确 CAS；初始化写后 ACK 丢失应以授权查询确认同 binding，不重建空表。服务签发须查实际 owner、instance/任期、旧映射和当前根元数据，不只签 App 自报 root。

本节原下一步已由第十三批推进为 §8.9 的缓存/单笔管理实现。混合根读取仍不是跨 blob 原子事务，必须串行化根变更；内部测试不等于产品根事件或整个生产任务授权已完成。

### 8.9 第十三批：根缓存、统一 pending 与类型化执行接线

**责任与启动顺序**

- `TaskAuthorityCache` 放在原根合同模块，持有 instance/root commitment/scope/state/generation，不复制 DeviceKey/Admin secret。`invalidate()` 在变更实际根之前调用，立即变 Pending；`refresh()` 在 cooperative 管理点从固定源独立读取，provider 返回后代际必须仍相同。读失败或被取消不发布新根。`retire()` 为终态，代际耗尽不回绕复活。
- 启动可先加载 Engine 的完整快照，再独立刷新根，或先读根再加载表。`restore(engine)` 只通过受限的 `authorityBinding()` 取得**已经完整验证**的持久绑定，与实际 instance/root 全长匹配后接受 scope。它不读 Flash、不接受网络传来的 binding；Fault/Retired/Cleared/未加载均拒绝。正常重启无需联网签票，但这不把 CRC 升级为对物理 Flash 篡改/回滚的防护。
- 原 `TaskTimeEnvironment` 可以直接注入 `TaskAuthorityCache::read`；执行器每轮只读缓存。默认 Product 尚未实例化缓存/Session，实际 Admin/Key 变更与断连事件尚须接线。没有新增线程、credential blob、平台库或每连接任务表。

**同一笔管理事务**

`TaskManagementSession` 注入原 Engine、根缓存、`IRandom`、`IServerSignatureVerifier` 和单调 `IClock`。只有一份自有请求及一个挑战槽，Initialize/Rebind/Mutate/Read 共用；无初始化专用队列或网络 payload 的长期借用。TTL 和最小挑战间隔由产品明确提供（均为非零且不大于半个 u32 周期），当前测试参数不是生产默认。

1. Prepare 先核对当前实际根/Engine 状态；初始化要求 Empty，换根要求实际持久旧 root/scope/instance 一致。一般修改复用 Engine 的定义校验。操作完整内容编码摘要后拷入自有请求，不能只保存请求方提供的 hash。
2. 有效请求得到安全随机非零16 B nonce；同一槽未完成/未过期时不允许另一请求覆盖。挑战生成也计入 TTL/冷却；随机源失败或回调使根代际改变则取消。不依赖业务 UTC，也不无限重试随机源。
3. 内部 `channel` 是适配器分配的非零64位本地句柄，必须同时区分 transport session 和路由 peer，并在旧事务有效期内不复用；**不是上行自报账号 ID，也不是 owner 证明**。错误 channel 不消耗别人的 pending；匹配断连必须取消。平台原生回调只投递事件，不能跨线程直接访问 Session。
4. 对应 channel 的一次 Commit 在验签前消费挑战，包括格式错误和失败签名，避免同一挑战无限验签。旧 nonce 不复活。`poll()` 推进单调剩余时间，支持正常短跨越 u32 回绕，反向/过大跳变取消；离线时也必须 poll。`pending()` 是最近一次轮询状态，不是权限检查。
5. 验签返回后再次检查取消、TTL、根代际；provider 中取消/换根再换回同摘要也不能过关。唯一 Session 防重入；不得绕过它从网络调用 `*Verified()`。授权成立后才调用原 initialize/rebind/mutate/recordAt，原 Engine 保持 CAS、Start 限制、写配额、故障锁定与一次执行。
6. 提交后再检查有效性；若 Flash 已成功但根失效/TTL到期，清空对外结果，不假定回滚、不自动重发。一般修改以**新挑战＋原 requestId/完整操作**取得原 Engine 最近回执；初始化/换根若 ACK 丢失，先授权查当前绑定/表状态，不重新建空表。Session 的内部结果不是网络 Installed 证明；第十四批的认证回执见 §8.10，实际接收者的请求关联仍须接线。

`TaskAuthorityCache::selectVerified` 仅供 Session 使用；初始化/换根调用 Engine 所需的临时 Current 在返回后与实际有效 Engine 元数据重新核对。未落盘提议或故障表不能留下 Current；不会读失败 store 来猜测持久化结果。暂时没有把服务器退休事件接入产品，不能据此承诺线上删除立刻清任务。

**已实现的类型化操作摘要（内部 wire 候选，未分配 kind/feature）**

```text
mutation = [1, requestId16, expectedRevision:u32, operation, taskId:u32,
            enabled, definition | null]
definition = [timingKind, utcMillis, durationMillis, minuteOfDay,
              utcOffsetMinutes, weekdays, enabled, key, valueType, scalarValue]
mutationDigest = SHA256(CBOR(["blinker.task-mutation.v1", currentRoot32, mutation]))

read = [1, requestId16, expectedRevision:u32, index:u32]
readDigest = SHA256(CBOR(["blinker.task-read.v1", currentRoot32, read]))
```

operation 依次为 Create=0、Replace=1、Remove=2、SetEnabled=3、Start=4；TaskMutation/Engine 共用验证，不允许非定义操作携带隐藏参数。完整 requestId/CAS/参数及 currentRoot 进入管理摘要，**不使用 Engine 私有 BTM2 重放摘要充当授权摘要**。两个摘要均流式生成。

读取首先用 `index=UINT32_MAX, expectedRevision=0` 授权获取**当前**表元数据，不要求 App 事先知道修订号；此 sentinel 不允许其他 revision。随后单条索引读取必须带精确 tableRevision，不匹配则拒绝并重新获取元数据。避免第一次使用无法查询，也不混合不同版本定义。执行进度会在同一定义 revision 内变化，因此逐条结果不是整表运行状态的原子快照。空表元数据也需 owner Grant。第十四批已补一条记录/表修订与数量的认证编码及挑战绑定；分页消费者的 outstanding 生命周期仍待接线。

第十三批已有修改 Prepare 编码器、两个摘要与类型化管理调用，当时尚无一般网络 decoder 或挑战/结果编码；第十四批进展见 §8.10。命令 kind/feature、真实 Service signer 或 App 新消费者仍未实现，不应把内部 prepare 方法直接暴露为网络入口。包长测试已删除手写 Prepare 的第二套编码，改为调用实际函数；最大修改路由帧182 B、Grant235 B不变。

本节原下一步已推进为 §8.10 的严格 wire/认证回执；当前按第10节接真实消费者与产品事件，不另建管理框架。

### 8.10 第十四批：严格 wire、单条查询与事务绑定回执

**职责收敛。** `TaskManagementContract.h/.inc` 只持有自有 DTO、规范编解码、摘要和 HMAC，不轮询、不写存储、不选择连接或保存密钥。原 Session 内的修改编码/摘要与结果类型移入该模块，仍编入 `BlinkerProvisioningController.cpp`；Session 只负责有界事务状态和 Engine 调用。没有新责任翻译单元、动态分配、第二个 pending 或任务凭据库。

一般 Mutation/Read 的数组和摘要沿用 §8.9，不改变 Grant 或 schema 2。入站先检查长度、canonical CBOR、固定数组长度、版本、数值范围、尾随数据，再复用 Engine/Store 对 timing/key/scalar 的语义校验。拒绝 indefinite/非最短整数、截断、溢出后窄化、全零 ID、非定义操作隐藏参数；key最多64 B、标量最多9 B，仍只支持既有标量类型。解码失败清空输出；成功返回自有数据，不跨异步边界借用 RX 缓冲。

**已实现的内部挑战和成功结果合同；尚未分配公开 BBP kind/feature：**

```text
challenge = [1, instance16, taskScope16, currentRoot32,
             nonce16, requestId16, operationDigest32]

resultUnsigned = [1, keyRole, resultKind, requestId16,
                  tableRevision:u32, count, detail]
resultWire = [1, keyRole, resultKind, requestId16,
              tableRevision:u32, count, detail, proof32]
proof = HMAC-SHA256(actualSecret32,
          CBOR(["blinker.task-result.v1", completeChallenge, resultUnsigned]))

keyRole: DeviceKey=1, Admin=2
resultKind: Authority=0, Mutation=1, Metadata=2, Record=3
detail:
  Authority / Metadata -> null
  Mutation -> [taskId:u32, replayed:bool]
  Record -> [index:u32, [taskId:u32, definitionRevision:u32,
                         consumedUtcMillis:u64, startSequence:u32,
                         progress, definition]]
```

`definition` 复用 §8.9；一条 Record 同时返回定义与本次观察的运行进度。count不超过10，index必须小于count，taskId/definitionRevision/进度由 Snapshot 校验；revision为0时count必须为0。先元数据、后精确修订逐条查询，不能将同一 tableRevision 内变化的运行进度称为整表原子快照。Claimed 在恢复时仍按 Unknown 处理，Completed 只说明回调成功，不证明物理设备状态。

完整挑战固定138 B。结果不重复 nonce/root/scope：验证方已经持有对应 outstanding challenge，其全部字段进入 HMAC；减少包长不缩短认证标签。使用实际编码器的完整路由帧（含 peer/request 封套、BBP10 B、可选CRC4 B）中，挑战197 B，合法64 B key＋最大宽度字段的单条认证记录最大236 B；修改182 B、Grant235 B、初始化147 B/换根180 B不变。结果输入body保守上界197 B，对应路由预算256 B，当前测试最大合法body177 B。此处不含 MQTT topic/TCP/IP、BLE SecureRecord/ATT 分片开销；不能将256 B称为全部链路成本。

**验真方和密钥来源必须明确，不能让对端自己选权限。**

| 场景 | 当前32 B密钥来源/实际验证者 | Service 能得出的结论 |
| --- | --- | --- |
| WiFi DeviceKey 路径 | 设备当前 DeviceKey；持同一密钥的 Service 验证 | 与该密钥及本次完整挑战绑定的设备侧结果；不具备对同密钥持有人不可抵赖性 |
| 纯 BLE Admin 路径 | 已安装 PlatformAccess Admin secret；持密钥的 owner App 验证 | Service 只有已验证 Admin 指纹，不能独立验 HMAC；最多记录经已认证 App 提交的确认 |
| WiFi+BLE 混合产品 | 产品与消费方按真实入口固定角色；根承诺仍包含所需全部根 | 不允许报文自选角色、缺根降级，或把 Admin 指纹充当密钥 |

这复用现有 Controller receipt/commit 的信任边界，不上传 raw Admin secret、不派生另一套持久任务密钥。DeviceKey 回执的 Service 验证结果传回 App 时，必须沿可信确认路径并关联本次请求；不把未经认证的 `verified=true`、普通转发 ACK 或 JSON 字段当成验真。若明文承载不提供该可信路径，则 App 不得宣称独立验证 Installed，实际消费者接线必须明确这一点。HMAC 是完整性/持钥证明，不提供加密；默认明文 MQTT 上任务内容仍可能泄漏，不因此扩大为全局 Cloud Frame Authentication 或改动可选 TLS 策略。

**提交、回执与重试是一笔事务，不是再建状态机。**

- `Session::commit(..., completed)` 仅在 Engine 成功且提交后代际/TTL复核仍通过时，输出设备自有的实际已消费挑战。失败/迟到/错误channel一律清空completed；不复制报文自报上下文。初始化/换根使用实际已接受scope和当前根。
- 编码器只能处理上述成功结果；选择实际当前secret、校验根来源与事件串行性由可信产品负责，codec不读取凭据。编码/发送空间不足不回滚已落盘表；一般修改沿用新挑战＋原requestId和完整操作的去重，初始化/换根走授权查询恢复，不自动重新初始化。
- `verifyTaskManagementResult` 先验证再发布自有结果，没有公开的未验真结果decoder。接口本身无状态；调用方必须核对预期操作→resultKind、record index/修订语义、角色/密钥来源、请求期限，并在成功接受后消费outstanding。不能把“正确旧MAC”当新请求的新鲜结果。
- Challenge decoder 只证明格式正确，不证明设备身份。真实 Service signer 必须从权威 owner/scope/根元数据与完整操作重算摘要，不签客户端自报root/hash；App 指纹版本与服务端已验证元数据必须一致。
- 失败响应尚未冻结/接线；不得以未认证错误替换成功证明，也不能让伪造错误触发擦表、退休或自动重装。下一步以最小实际消费者确定错误合同、公开分发和发送背压，不提前宣告feature。

### 8.11 第十五批：来源绑定适配器与实际产品生命周期

**已实现的窄接口，不是另一个通信栈。** `TaskCommandAdapter` 持有现有 Session/Cache/Engine 及固定实际根来源的引用；Authority/Mutation/Read/Commit 四种内部选择器调用 §8.10 严格 codec。选择器不是新增BBP kind，无默认feature，也不把任务伪装成属性Command。无socket、RX/发送队列、额外线程或持久密钥；只保存一笔来源和递增64位本地channel，耗尽拒绝，不在stop/start时归零。

来源由可信transport分发提供：`transport + generation + session + routed + peerKind + peerId + resultKeyRole`。generation为非零连接代际，重连复用session之前必须变化；route类型也参与匹配，不能仅比较16 B ID。非路由路径要求空peerId及默认peerKind。role由产品路径固定，不是对端请求字段。该来源**仅隔离事务，不证明owner**；Commit仍验证完整一次性Grant，Shared/control位或源账号自报信息不能替代签票。

- 错来源Commit/断连不消费合法pending；匹配断连、整transport同代际断开、stop、TTL到期取消。不同来源不能抢占一笔正在等待签票的操作，沿用Session挑战冷却，无第二套验签重试。
- 接受请求前检查挑战/结果所需TX空间（138/197 B），不足不消费nonce/不写Flash。成功提交后的实际send失败是另一种不确定结果，仍须新挑战查询或原requestId去重，不能自动重装/重开Start。适配器不会保留RX/结果供异步使用。
- `encodeTaskManagementReceipt` 复用重构后的根加载/校验/摘要，每种实际来源只load一次；从同一批临时secret计算完整root并选实际DeviceKey/Admin签HMAC，既不将root当密钥，也不再第二次读取签名secret。临时secret由统一RAII清零，不增凭据表。混合产品始终验两根，缺任一根不降级；原独立golden确保root格式未改变。
- Commit在成功落盘后再取实际回执密钥；取源失败/根不匹配时不发布回执，但不回滚Flash。签回执provider期间发生stop/断连/根代际变化也清空并擦掉已经生成的响应。主逻辑防重入；这种丢回执不允许扩大成Flash失败恢复或自动重放动作。
- `invalidateRoots()` 必须在写实际DeviceKey/Admin之前调用；写入完成后才`refreshRoots()`。固定来源/选择器对象必须比adapter活得久，选择器只能由已验证产品生命周期更新，不能来自请求。刷新与存储表不匹配保持Pending，可用新的同owner Grant显式Rebind；不是新建空表。不同源不是跨blob原子事务，仍依赖同一cooperative上下文串行化。

**已有可选生命周期接线：** `TaskProductLifecycle` 可注入该adapter；实际transport启动、Engine begin后独立刷新/恢复根；transport poll前后推进原Session超时；普通stop立刻取消管理但保留已安装任务；resetAccess先退休cache/Session/Engine，再按原有“任务清理成功→清授权根”顺序执行。task错误不使普通控制无法恢复。可选生命周期32位自身24→28 B；未启用任务的普通Product结构/API不变，不把管理对象加进每个组件或每条连接。默认Arduino产品尚未实例化完整管理组合。

**仍须真实分发接线，不能用host fixture代替：**

1. 当前 `MqttFrameTransport` 的回调sessionId固定为1；已有 `connectionGeneration()` 才能区分重连。`RxContext` 无route源信息，Runtime也不消费Delivery封套。接线必须在可信分发层保留真实peer，或明确Service是唯一云任务管理对端；不从业务payload冒认原账号。generation本身仍不是身份认证，明文MQTT风险沿原合同保留。
2. 先用一个WiFi真实管理事务，协同共享协议包/Broker冻结kind/feature与失败回执。未通过普通Hello/管理路径准入的流量不得进入adapter；失败提示不是可信Installed，不触发擦除/重装。BLE再复用已认证管理session和同一个执行器，不复制一条BLE任务图。
3. adapter调用必须在cooperative管理点进行，避开借用的原生RX回调和属性回调；复用现有有界收包存储，不为管理创建无限任务队列。发送背压、请求关联及真正的断连/代际事件由该分发层处理，本批没有用raw callback绕开这些约束。
4. 平台Key/Admin真实写入前后尚未调用这些根事件；Service真实owner/scope/根签发、App回执验证与新版安装/通信共存仍待完成。DeviceKey由Service验证、Admin由owner App验证的边界不变。公开API/feature继续关闭，内部成功不能标成生产安装成功。

### 8.12 第十六批：物理 WiFi carrier、协商与收发所有权

本节载体合同已同步第二十三批操作扩展；第十六批“剩余实施”段是历史状态，当前进度以§10.22和总主线为准。

三端kind占用核对后预留 `TaskManagement=0x37`、`FeatureTaskManagement=1<<25`。首版云端唯一对端为Service，通过既有设备MQTT会话/topic传递；不接App Route/Delivery、LAN、BLE或虚拟子设备。账号授权在Service签发前完成，MCU只认实际Grant绑定，不从payload推断用户身份。混合产品根仍按固定配置共同验证，不能因为入口是WiFi就绕过Admin根。默认产品未实例化该扩展，Broker目前仅识别、不协商/转发。

外层BBP固定10 B header，request flags=0、response flags=IsResponse；sequence非零且响应原样回显。外层sequence只关联传输，不是管理幂等ID、权限或认证证明。无AckRequired、IdMode、CRC、Route或第二套BBP可靠发送队列；实际管理去重和认证仍在原inner合同内。

```text
body = [1, operation, bstr(exactInnerCbor)]
operation = Authority(0), Mutation(1), Read(2), Commit(3),
            Challenge(4), Result(5), Failure(6), Retire(7), RetirementRead(8)
Failure inner = [originalRequestOperation, code]
code = Malformed(1), Unauthorized(2), Busy(3), Conflict(4),
       Unavailable(5), Capacity(6)
```

0..3及7..8仅Service→设备，4..6仅设备→Service。必须明确分类，不能用`operation>=4`判断响应。outer严格canonical、三个字段、body1..197 B；inner由typed codec校验，旧签名/HMAC transcript不变，新退休摘要与结果细节见§10.22。Failure是**未认证的失败提示**，只能结束/提示本次等待；它不是“未执行/未写Flash”的证明，不驱动擦表、自动初始化/换scope、退休或新ID重发Start。损坏外层、未协商、错误方向/flag/零sequence/收发槽已占用直接不接纳，由Service有界等待收口；不能以不断来的报文延长nonce或outstanding生命周期。

`TaskCloudTransport<Cloud>`只装饰同一Cloud，注入MqttFrameTransport、时钟、adapter和独占workspace；不持有raw key，不创建socket/session/线程。native回调将outer body复制到workspace后返回；cloud.poll返回后才处理。inner输出在同一workspace尾部构造，carrier编码显式支持重叠move，不叠加另一份212 B输出数组。Request/Reply两个阶段互斥，单槽占用期间不收另一帧；这是一个缓冲的所有权状态，不是第二份管理事务。

`IFrameTransport`增加默认空的协议能力/协商通知虚函数，不增加普通实例数据字段。Runtime从安装的物理cloud handler取得唯一允许的任务bit，并过滤配置中自行设置的bit；只有完整Hello已被Runtime接纳、认证云路径、协商bit、对端和本地frame上限均≥212 B才打开该入口。未用raw Hello解析直接标ready。真实session断连、网络离线和重新Hello关闭入口、取消当前Cloud来源；MQTT重连复用session1时还必须匹配真实connectionGeneration。

从RX到发送完成只有1秒绝对缓冲窗口，不取代Session既有挑战TTL。`WouldBlock`只保留exact response bytes重试，不重新验签/提交；其他send失败释放缓冲，已落盘结果保持不确定。到期、stop、重连、根代际改变取消/清空，回调中变化抑制迟到响应；不在borrowed RX或重入poll中清正在解码的内存，退栈后统一清零。

**资源和边界**：Win32对象96 B＋独占workspace212 B；原adapter64 B/cache76 B/Session336 B仍分别计入，不是全部产品RAM。实际物理帧Prepare138 B、Grant191 B、初始化103 B/换根136 B、Challenge153 B、当前最大单条结果192 B，保守接受上限212 B；不含MQTT/topic/TCP/IP/TLS。§8.10的197/236 B是未套本carrier的历史Route估算，不再称为当前WiFi链路包长。禁止用197 B接受上限推断未来Route封套仍必然≤256 B，未来如需路由应重新测整个载体。

**剩余实施**：此批host组合使用真实Runtime、TransportHub、MqttFrameTransport、adapter/Session/Engine，但底层socket/Flash/签名验证器是替身。签发、权威owner/scope/实际根读取及回执消费者未接；真实Platform写Key/Admin前后通知、用户产品composition和通信共存尚未验证。Broker识别但不协商任务，普通App控制继续原路径。下一步按Service计划闭合权威签发和物理carrier，再连接一个手动WiFi产品，不再做另一套adapter。

### 8.13 第三十六批：认证的单次尝试拒绝

只补两个能在**有效Grant验证后、首次持久写前**确定的拒绝原因。复用原Result/HMAC，不把普通Failure或通用WouldBlock升级为可信“未执行”。

```text
resultWire = [1, keyRole, 7, requestId16, tableRevision, count, reason, tag32]
resultKind = Rejected(7)
reason = WriteBudget(1) | StartCooldown(2)
```

- reason为单个CBOR unsigned，不含预计重试时间；StartCooldown只允许Mutation.Start，WriteBudget允许Mutation各操作。tableRevision必须等于原请求expectedRevision，count仍限0..10。其他操作、原因0/未知值、非规范编码均拒绝。
- HMAC仍为原`blinker.task-result.v1`域，绑定完整挑战中的instance/scope/root/requestId/nonce/operationDigest，密钥来源与DeviceKey/Admin角色不变。外层仍为`TaskManagement=0x37 / Result(5)`，无新topic、feature、队列、线程或secret。
- Engine仅通过可选`TaskMutationRejection*`返回明确决策；原Result错误与普通调用者行为不变。已有持久去重先于额度/冷却；通用WouldBlock、目标验证失败、存储错误、根变化均不产生认证拒绝。Session成功表示交换完成，消费者仍须检查kind，不能等同于写成功；nonce已消费，任何后续尝试需新挑战。
- Service在原attempt上保存exact拒绝证明并置`rejected`，释放该attempt的single-flight；**request保持pending及原root/CAS/正文，不能置verified、推进回收floor或改变confirmed_root**。同attempt只有完全相同证明可重复确认。先前unknown记录保留，8attempt/64request上限不变，拒绝不能当TTL可回收的成功写。
- HTTP 200表示认证交换完成，返回`state:"rejected", evidence:"fresh", result:{kind:"rejection",tableRevision,count,reason:"write-budget"|"start-cooldown"}`。不是已执行/永久失败。App严格校验关联/原因/CAS，在成功清日志路径之前显示原因并保留原意图；无自动重试、自动清理或新ID/CAS。只有用户明确恢复才重提原ID，且届时可能首次执行。
- schema043只扩展原attempt状态和grant/result约束，不建新表/新列。必须先备份并完成匹配版本迁移才部署；第三十六批仅隔离schema验证，第三十七批已备份后升级本机业务库至43（非线上）。旧固件Failure继续unknown，旧消费者遇到新kind失败关闭而非误判成功；不将混合版本宣称为可用。

独立Node手写CBOR/HMAC与C++/Service共享fixture交叉验证两角色×两原因。普通小revision回执58 B、最大uint32 revision为62 B，含5 B carrier和10 B BBP头为**77 B**；不含MQTT/topic/TCP/TLS。沿用inner197 B、carrier202 B、frame212 B上限，不因此放宽任何工作区。新增字段为瞬时结果枚举，不增加每任务槽或Engine/Session持久状态。

### 8.14 第三十八批：有限试点准入与未确认根边界

本节于第三十九批修订未确认根规则，第四十批补连续未知根真实拒绝证据，第四十一批修复并实测跨根退休，第四十二批补首次根从未确认的差异实物。集中维护当前可声明范围；不扩大默认开关，也不等于T2整体关闭或正式发布。协议仍取前述合同，实施优先级只取总主线§6。

| 项目 | 当前准入/证据 | 不可外推 |
| --- | --- | --- |
| 设备与用户 | 显式设备ID白名单、普通WiFi DeviceKey-only、真实owner；XIAO ESP32S3 stock3.3.11，4任务槽/1个boolean power目标有限实测 | 全部ESP32 SDK、R4/Wio任务、BLE/混合Admin根、网关任务；4槽不是最终公开容量 |
| 执行 | 倒计时、一次日期、固定偏移星期任务；安装不隐式Start，退出页面后本机执行，重启倒计时不续跑，失时跨到期不补放 | 任意cron、动作链、DST、物理副作用exactly-once、长期离线准时 |
| 权限 | Service逐阶段owner检查，普通分享control不授予长期任务管理；真实非共享账号403已测，viewer/operator全阶段隔离软件回归 | 已分享operator的真实账号/MCU门禁仍缺；当前两测试账号分属shard1/0且跨片分享未开放 |
| Key/原意图 | 正常单次换Key、App未决Start/同步丢确认；首次/连续Service未知根后换Key拒绝/保留日志真机；普通控制独立；两种窗口当前根认证终止旧表真机 | 未知根恢复、清日志后补做、保存旧Key；所有根/通道组合的退休实物覆盖 |
| 拒绝与资源 | Start冷却认证拒绝及同ID恢复真机；写额度认证拒绝软件；请求30次/分钟、64条账本、每请求8attempt保持 | 写额度真机、最大32目标目录、10任务、长期磨损/无限管理历史、拒绝即证明历史从未执行 |
| 交付 | 本机封闭试点候选，匹配schema043；公开opt-in/profile、单库安装和COM3实物已过；第四十五批最小设备源码独立重建/安装编译闭合 | SDK/CLI须预装；正式域名/签发材料、三端全新checkout闭包、生产部署；Broker删除清理仍独立待确认 |

**当前前置拒绝（第三十八、三十九批）**：Service在已有owner/session/scope锁内，一次读取该scope已签发的AuthorityChange（原64请求上限内，EXISTS过滤attempt避免重复）。复用规范codec解码持久化`previousRoot/currentRoot`，不读取旧secret或按时间排序。存在以下任一条件时，Initialize/Read/catalog/Mutation返回409 `TASK_AUTHORITY_RECOVERY_UNSUPPORTED`：

1. 从未确认过根，已有签发变更的目标根不是当前Key派生根。
2. 已确认根为A，已签发从A到B的变更，但当前Key派生根已不是B。B是否真正安装尚不可证实，不能猜A或B来构造下一次同步。

相同旧requestId也受限制；拒绝前不写新request/attempt、签发Grant或发任务MQTT，事务回滚保持历史。只prepared未签发不算变更证据，过期/unknown attempt已有的Grant仍算；不能用新prepared覆盖旧签发。已确认根由认证Authority/Read回执更新，不以原request是否仍pending判定。

| 最后确认根 | 中间记录 | 当前Key根 | 当前允许的路径 |
| --- | --- | --- | --- |
| 无 | Initialize A已签发、未确认 | A | 原同根重试/新鲜Read恢复 |
| 无 | Initialize A已签发、未确认 | B | 明确拒绝不支持的恢复 |
| A | A→B仅prepared | C | 显式构造A→C；旧B请求不改写 |
| A | A→B已签发、未确认 | B | 新鲜Read可确认B；不自动同步 |
| A | A→B已签发、未确认 | C | 明确拒绝，不尝试猜A→C或B→C |
| B | 原A→B仍pending，但Read已确认B | C | 显式B→C；旧pending不能误阻断 |

Authority历史不回收；Read历史按原策略回收后，已持久确认的根仍有效，因此最后一行不能依赖残留Read记录。该规则复用上一批HTTP错误/App日志，没有新增字段、表或恢复队列。

这是**拒绝不支持的恢复窗口**，不是恢复算法：不猜旧根、不把签发等同MCU安装、不发送空previousRoot来重新初始化、不清任务或保存旧secret。首次和连续未知换根目前均为“明确拒绝、保留证据”，不是“已恢复”；已确认根的正常多次同步、签发当前根后新鲜Read继续有效。原有未知窗口仍是试点限制，不能因此宣称正式全场景可用。

App复用原错误/日志状态：显示限制，禁止该页面当下的初始化、同步和原请求重试；重进只做查询，不写新意图、不清旧日志。安全Key刷新仍可进行；正常owner删除/退休入口不被本检查禁止，是否真的完成MCU退休仍必须有认证证明。文案不能要求用户保留已泄漏Key或擦Flash作为隐式恢复手段。

**第四十批修正**：上述Service退休绕过普通管理的前置限制，不代表MCU必能退休。真实A→B未确认→C后，`prepareRetirement`仍要求`stored == request.authority == current root`，B与C不同就拒绝挑战；commit也重查完整绑定。实物仅云撤销成功，`localTasks=not_confirmed`，退休request pending、无Grant/Result且设备无终止写入。不能把scope数据库state=retired写成设备清理完成。

**第四十一、四十二批覆盖上述退休缺口**：终止授权/历史表绑定按§8.15分离，两类未知根有限实物已通过，见§10.40～10.41；保留第四十批失败证据，不把终止能力扩成恢复。第四十三至四十五批已补用户入口/profile、安装/实物和最小源码交接；下一步主线§6第18项只设计A1。已分享operator缺同片环境，不迁账号/改ACL/开跨片；最大目录、写额度真机、其他平台及正式交付仍未准入，不重复刷板或宣告T2正式整体关闭。

### 8.15 第四十一批：终止认证根与存储绑定分离

终止同一ownership tenure不要求先恢复其可执行根。Retire/RetirementRead的现有wire `authority.rootDigest`继续表示**当前独立加载的认证根**；instance/scope必须与存储任务表或终止标记相同，不能由App自报owner代替Service授权。

- Prepare验证request等于当前根加目标scope，检查存储binding.sameOwner；`expected_`仍固定完整旧存储绑定，不覆盖为新根。签名operationDigest包含当前根，challenge及回执也用当前根；`generation_`锁定其整个生命周期。
- Commit消费nonce、验证Grant后，重查generation/TTL/channel、当前instance/scope及**存储完整绑定仍等于expected_**；仅调用原Engine `retireVerified(expected_, requestId)`。不修改Engine的旧绑定核对，不调用Rebind/selectVerified，不恢复执行或触发业务回调。
- 原终止标记仍保存原存储绑定/原退休ID；重启后只有当前根的新Grant可只读查询，同IDRetire幂等、不重写。旧Key请求/旧Grant、跨instance/scope、prepare或验证中换根、标记冲突与脏存储均拒绝，不能用不可信失败响应声明清理完成。
- 复用既有单待处理槽、digest和generation；不新增字段、secret、migration、wire、RAM槽或后台恢复器。设备普通Read/Mutation/Initialize的权限及Service未知根前置拒绝保持不变。
- 本批按上述合同先补设备Session/真实codec与MQTT编排回归，再用上一批窗口复测原owner删除。软件、实物和制品测量分别记证，不因合同修订提前宣称真机通过。

### 8.16 第四十二批：有限profile与普通用户入口的退出条件

本节只收口现有普通WiFi任务，不扩展通道/任务种类。以下各项不能混为“定时已完成”：

| 层次 | 当前事实/退出条件 | 处理方式 |
| --- | --- | --- |
| 功能与故障边界 | 倒计时/日期/星期、持久化/授时、Start保护、单次/连续换根和终止均有前批有限证据；本批首次Initialize未知根拒绝/终止已通过 | 不新增长循环或重跑整个矩阵；实物结果见§10.41，不代表未知根能恢复 |
| 已分享账号权限 | 真实非共享账号和隔离viewer/operator矩阵已过；两个现有测试账号仍为shard1/0，跨片分享未启用 | 缺受支持同片账号时明确待测，不改账号分片/ACL、不为该门禁开启跨片；不是任务管理授权已开放 |
| Arduino对外启用 | 第四十二批曾需内部宏/`configureTargets`；第四十三批迁移为§8.17的`enableTasks`，原内部旁路删除 | 软件/编译、第四十四批安装/COM3实物通过；不复制Engine/Session、不暴露Flash/workspace/Grant/MQTT |
| 维护者profile | stock3.3.11、4槽/单power目标、60秒10写/冷启动空额度/30秒Start/100ms提交预算为受控实测配置；签发材料仍为测试profile | 容量、资源policy与信任配置由维护者明确提供；用户启用API不能静默选择生产默认，普通可写属性不自动成为调度目标 |
| 组合开关 | 任务默认未启用；任务＋LocalAccess明确拒绝，单独打开LAN又要求LocalAccess，二者同时打开也被拒绝 | 六种真实编译器语法检查均符合现有合同；不需要新增同义guard或重写composition，不将该结果外推为任务＋LAN支持 |
| 正式交付 | 用户opt-in、development profile、安装/实物及最小源码交接已过；非production，最大目录/写额度真机/长期耐久/其他平台未准入 | 封闭owner试点与正式发布分别记录；默认开关保持，下一包仅A1设计，不因交接完成自动部署云执行器或启动S2整体发布 |

**此项边界已于第四十三批按下节实现**：开发者只选择允许调度的现有Property与简单启用方式，维护者负责已验证容量/policy/信任材料。API/目标校验与平台存储/通信继续分离，首个实现仍仅普通ESP32 WiFi，API不以MCU命名，也不承诺其他模式支持。未经资源检查的容量默认值不擅自新增，开发试点与正式教程分开。已分享operator是独立缺环境项，不用重复刷板代替。

### 8.17 第四十三批：最小任务启用入口与维护者profile

任务表/权限仍归原Engine/Session/Service；本批只落实§8.16的调用边界，不创建第二套模型、时钟、凭据或MQTT连接。

```cpp
// 使用维护者提供的、已明确准入任务的开发候选库。
#define BLINKER_ENABLE_TASKS 1
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
void onPower(bool value) { power.report(value); }
void setup() {
    power.onWrite(onPower);
    if (!Blinker.enableTasks(power)) return;
    if (!Blinker.begin("DEVICE_KEY", "SSID", "PASSWORD", power)) return;
}
void loop() { Blinker.run(); }
```

- **编译开关与允许目标不是同一权限**：前者只编入可选任务组合，未定义/为0不链接任务图；后者显式选目标、启用既有业务UTC。均不调用Initialize、创建/Start任务、读取App owner声明或绕过Service白名单。未启用产品调用方法返回UnsupportedFeature；显式开启不支持的BLE/混合/R4/Wio组合编译拒绝，LAN/LocalAccess仍沿既有guard拒绝。
- **一次、静态、有限**：`enableTasks`在首次begin之前调用一次，只接受Property引用，编译期至少1个/至多32个；这不是32目标真机准入。配置按数量保留固定绑定，不按全库最大目标预留；无堆分配、无普通Property/全局facade字段增长。同类型参数首次无效后可以改为有效目标；成功后其他参数包/重复调用/另一个product不得覆盖绑定，end不解锁。Property及Spec仍须全局或长生命周期，不能把setup局部属性交给静态会话。
- **校验分层复用**：配置阶段拒绝重复key、只读、文本/bytes、遥测及坏描述符；不探测执行用户回调。Product完成schema准备后、启动transport/任务存储前，用原目标目录验证实际Client归属、begin成员和写回调，漏配不会打开任务NVS。准入之后的存储/未知根故障仍不阻断普通网络控制与原认证终止。描述校验只维护在TaskPropertyTarget，不在facade复制一套。
- **实现接缝**：facade通过轻量`TaskSupport<Profile>`调用产品已有extension，默认实现不分配任务状态；ESP32 specialization复用原单实例图。原借用数组的底层target接口保留给独立核心测试/组合，普通Sketch不使用。删除生产`configureTargets`旁路和旧`BLINKER_INTERNAL_WIFI_TASKS`/四项旧policy宏，两个probe迁移公开enableTasks；live gate仅底层诊断仍读取内部taskError，不是用户启用所需API。
- **维护者配置，不转嫁用户**：原Arduino打包profile可选增加`"task_profile":"esp32-wifi-t2-lab-v1"`，只允许development；缺省仍无任务配置，未知ID/非字符串/production均拒绝。现阶段只映射已明确的4槽、60秒10写、30秒Start、100ms提交预算以及原profile签发材料，不接受任意新policy。写入OfficialConfig的宏由TaskConfig校验；资源探针的8/10槽仍只测链接，不通过此profile开放。维护者生成一个候选库，不要求最终Arduino用户安装Python或修改内部文件。
- **边界**：保留四个正式普通示例，不将有测试Key/Flash计数器的live gate作为发布示例。`tests/arduino/CompileTaskWiFi`是无内部类型的编译消费者；README明确该入口仍为封闭候选。准入材料、服务白名单、资源与实物检查缺一不可，不能因新方法存在而把普通源码ZIP称为正式任务产品。

## 9. poll、动作和回执

一次 `run()` 顺序：现有网络/时间 poll → 处理有限管理操作（退休/停用优先）→ 获取一个时间快照 → 最多一个任务 Claim/执行/结果提交 → 普通状态发送。Flash 调用也在预算中，真实最坏时延必须测试。

所有权变更通知、任务管理与执行在同一 cooperative 上下文串行；中断/原生线程只发事件，不直接修改任务表。属性回调内再次管理或 poll 直接 WouldBlock，不能重入执行。不让用户回调操作持久表对象。

设备只显示最近执行结果，不保留无限历史。Completed 表示回调成功返回；传感器/继电器物理反馈是另外的状态。回调报告值、失败及无变化，沿现有上报/限流路径处理，不为任务建立第二条 MQTT 连接。

App Draft/Pending/Installed 与设备 Disabled/WaitingForTime/Armed/Claimed/Completed/Interrupted/Unknown/Error 分开；提交 ACK、执行结果、物理状态不是同一种成功。分享控制者暂不允许安装长期任务，不只靠 UI 隐藏。

## 10. 实施顺序与本批进度

| 批次 | 工作 | 退出条件/状态 |
| --- | --- | --- |
| T0b-1 | 本文：架构、单快照取舍、掉电/时间边界；增加时间样本 revision 可观察性 | 本批文档/时钟标识与回归完成；不是 T0 全完成 |
| T1b-1 | 纯 TaskCalendarCursor，fake/真实 WallClock；不执行属性或写存储 | 第三批单任务软件门禁通过；第四批 Engine 已补多任务顺序 |
| T1b-2a | TaskSnapshotCodec/Store＋内存 fake store 故障注入 | 第三批候选格式/golden/4、8、10 条资源和存储失败锁定通过；不等于幂等事务或执行器已完成 |
| T1b-2b | 在 TaskEngine 中落实管理 CAS/最近回执去重、Start、Claim/结果转换和写入配额 | **第四批纯核心通过**：17 组行为＋4/8/10 容量；内部 trusted seam，不是网络管理准入完成 |
| T1b-3 | 可选 TaskEngine＋实际属性入口、ESP32 存储；R4/Wio 端口评估 | **软件、S3 四档链接、COM3 隔离 NVS/一个物理断电窗口通过，现场字节已恢复**；退休清理底座通过，正式生命周期/通信共存/其他平台未完成 |
| T0b-2/T2-1 | 冻结 owner scope/管理 proof、kind/feature/分页等三端 wire，并接 CommandAdapter | 无新 wire 时软件核心可先测；无权限/持久化/执行时严禁公开宣告能力 |
| T2-2 | 类型化 App 服务替换旧 timing JSON、设备实物闭环 | 安装→退出/短离线→执行→查询；重启/撤权/容量/掉电；WiFi 和 BLE 分别验收 |

首轮不强求同时实现全表事务、RTC、多平台磨损日志、任意值或高级 cron。文档里留为待测的约束不能在实现时默默删除。旧页面/JSON 发送必须在类型化消费者切换后删除，不因名字含 timer 就删掉采样/重连定时器。

以上批次表保留原始阶段划分，不是当前待办清单。普通WiFi权威签发、carrier、产品生命周期及App闭环已有后续证据；不可继续把它们当成未接线重做。当前准入取§8.14，唯一下一步取[总主线§6](DEVICE_V2_NEXT_EXECUTION_PLAN.md#6-新执行队列)。BLE/混合根任务仍未准入；不新增任务凭据库或重跑无差异的COM3/C3门禁。

### 10.1 第二批已执行与证据

- 新增 `WallClockSnapshot.sampleRevision` 和实例计数；不改授时 wire、默认精度、安全时钟或现有同步频率。`clear()` 不清计数，失败样本不增加计数，计数耗尽不回绕。
- 扩展 `test_wall_clock_main.cpp` 第八组：相同/前后调整样本、普通 poll、失败输入、clear/过期/恢复与新对象。耗尽分支是显式检查，本轮未用数十亿次调用作穷举。
- Win32 Release 严格全量构建和 CTest **62/62** 通过（13.38 s）；WallClock 与 TaskTiming 的 x64 ASan 测试通过。Win32 实测 `WallClock=40 B`、快照 `24 B`；不是整个执行器内存。
- ESP32-S3 编译器 C++11 严格编译核心和 WallClock 测试翻译单元通过；未链接完整固件、未烧录。架构检查与 `git diff --check` 通过。
- 补充 `IAtomicBlobStore` 的成功/失败注释，明确报错不等于回滚；未改变任何平台 Flash 布局或存储实现。
- 本批不改 App/Service，不清授权、不操作硬件，不删除尚有消费者的旧 timing 页面。没有把拟议 TaskEngine/TaskStore 或签名管理接口宣告为已实现。

### 10.2 第三批：游标与存储底座（2026-09-22）

**模块与实际行为**

- `core/TaskCalendarCursor`：32 B 单任务运行状态，不复制 TaskTiming。调用方在同一个定义生命周期内传相同定义；修改、停用/重新启用时 reset 并传入该定义的持久 consumed 水位。输出 Due 只消费 RAM 候选，**不是允许执行的授权/持久 Claim**。
- 默认沿用 WallClock 的 1 小时年龄、5 秒不确定性门禁；校正小于等于 1,000 ms 连续处理，超过则重新定基线。无修订却改变 UTC、修订回退、非法 tick 间隔也重新定基线；失效时间清 armed 截止点但保留水位。启动/恢复只安排严格未来事件。
- `runtime/TaskSnapshotStore`：只提供完整快照编解码/原子端口调用，复用现有 CRC32、UTF-8、CBOR 和属性标量校验；数字有限且整数在跨端安全范围。没有第二套值解析器、动态内存、平台宏、网络处理或 Flash 布局改动。
- Decode 先完整验证再发布，坏的后半条目不能留下可用前缀。已删除的空表仍保留 tableRevision/nextTaskId/最近管理回执材料，不调用底层 clear。第三批这里只保存材料，当时尚未实现 CAS/重复请求识别；第四批由 Engine 接入（§10.3），不改变 Store 职责。
- 后端错误或损坏记录锁定当前 Store 实例；不能立即读回后继续执行，也没有自动重试/解锁。重新构造必须以真实重启或端口明确恢复持久状态为前提，不是换个 C++ 对象就获得持久性保证。

**第三批历史候选磁盘布局（schema 1；第十一批已由 §8.7 的 schema 2 替代，不再接受写入/读取；不是 BBP/2 管理 wire）**

| 区域/偏移 | 格式 |
| --- | --- |
| header 0～7 | magic `BTS1`、schema 1、零保留字节、u16 count；当前软件探针最多 10 条，不是公开产品上限 |
| header 8/12/16 | u32 accessEpoch / tableRevision / nextTaskId |
| header 20～67 | requestId 16 B、requestDigest 32 B；表达最近成功管理操作，结果修订即 tableRevision |
| header 68～71 | 必须为零 |
| record 0/4/8/16 | u32 taskId、u32 definitionRevision、u64 consumedUtc、u32 startSequence |
| record 20～23 | 时间类型、启用标志、执行进度、预期 ValueType，各 1 B |
| record 24/32/36/38 | u64 once UTC、u32 duration、u16 minuteOfDay、i16 fixed offset |
| record 40～43 | weekday mask、keyLength、valueLength、零保留字节 |
| record 44～107 / 108～116 | 64 B key 区（长度界定，无 NUL 要求）、9 B 自有单标量 CBOR 区；未使用尾部须零 |
| record 117～127 / footer | 零保留 11 B；全表末尾 4 B IEEE CRC32（不提供认证） |

多字节整数统一大端，不复制 C++ padding；CBOR 标量遵循现有库格式。记录按 taskId 严格递增，ID 必须小于 nextTaskId，非活跃时间字段为零；时间类型值为 Countdown=0/UtcOnce=1/Weekly=2。进度为 None=0、CountdownArmed=1、Claimed=2、Completed=3、Failed=4、Skipped=5、Interrupted=6；启动读到 Claimed 应由 Engine 呈现 Unknown，启动读到 CountdownArmed 应归一化 Interrupted，Store 不擅自改写。

accessEpoch 是内部候选字段，**其非零不证明 owner**。最终 scope 仍须与可信授权和不可重用的所有权代际接线；T0b-2 如要求不同布局必须升 schema/更新 golden。未知 schema 返回错误，不能当空表自动覆盖。首版尚无量产持久数据，因此不加入历史迁移分支。

**验证和资源边界**

- 游标六组：边界迟到、校正 ±1/±1,000/±1,001 ms、前跳后回拨不复活、失效/恢复、持久水位恢复、uint32 tick 回绕/异常、真实 WallClock 及 10 个同刻独立游标。未测试完整 Engine 的调度优先级。
- 快照七组：独立 Python `struct.pack`＋`zlib.crc32` 的 204 B 固定 golden；4/8/10 条完整往返；332 B 双记录逐位翻转 **2,656** 项、全部短截断；重算 CRC 后的无效字段/尾部/重复 ID；64 B key；所有支持的标量、NaN/Infinity/非最短整数拒绝；日历 occurrence、负时区、倒计时进度和空表保留。
- Fake store 区分提交前失败、提交后报错、仅当前 handle 可见但尚未持久；锁定后不再调用后端。模拟 powerCycle 后得到旧或新完整状态；已持久 Claim 交给 cursor 后不能再次到期。没有调用真实用户回调，也没有以此冒充真实 Flash 掉电测试。
- Win32：cursor 32 B；解码 header 60 B、record 128 B、Store 对象 16 B。仅“解码表＋编码区＋Store＋每条 calendar cursor”小计 4/8/10 条为 **1,304/2,456/3,032 B**，另计 Engine、平台缓存和栈；未启用组合无这些实例。不能把该小计当最终整机预算。
- 本轮严格跨编译发现 `DeviceRuntime::timePeer()` 原有 enum/unsigned 条件表达式告警，补显式 uint32 转换；不改变授时权限或选路行为。
- 验证：Win32 全量严格构建、CTest **64/64**（14.18 s）；x64 ASan 新增两组通过（未启用 LSan，不冒充 UBSan）；ESP32-S3 3.3.11 工具链对 Core/Runtime 及新增两份测试翻译单元 C++11 严格编译通过；架构与差异检查通过。跨编译只生成 `.o`，不是完整固件链接或真机门禁。
- 本轮不改 App/Service，不烧录、不清授权、不提交推送。保留仍有消费者的旧 timing 页面，按 T2 后续替换。

### 10.3 第四批：纯 TaskEngine（2026-09-22）

**三个注入边界与收敛**

- [TaskEngine](../src/BlinkerV2/runtime/TaskEngine.h)只消费 TaskSnapshotStore、ITaskEnvironment 和 ITaskTarget；没有 WiFi/BLE/MQTT/NVS 引用、动态分配或每任务线程。时间计算仍归 core，编码/持久性仍归 Store。
- `ITaskEnvironment::observe()` 一次交付同一采样 tick 的业务 UTC 快照、单调 tick 及当前已验证 accessEpoch；避免分别读取时间和 tick 造成假跳变。scope 必须来自已验证生命周期，不是请求 body 的非零整数。`initializeVerified/mutateVerified` 是内部可信入口，不是已完成的网络 proof 校验。
- `ITaskTarget::validate()` 无副作用，检查目标/类型/约束/显式 opt-in；`apply()` 同步有界，WouldBlock 当失败，不异步重试。fake target 不证明现有 Property 已能这样接线。
- 将时间连续性判断提取到已有 cursor 模块供 Engine 复用；记录准入复用现有 codec validator，标量及摘要复用既有 CBOR/属性校验/SHA-256。没有新增同职责事务层或旧 JSON 动作路径。

**实际管理和恢复语义**

- Create/Replace/Remove/SetEnabled/Start 统一 CAS；内部流式 SHA-256 对 canonical CBOR `BTM1 + verified scope + expected revision + operation + taskId + enabled + definition/null` 求摘要。requestId 独立匹配，摘要不能由客户端指定。这是内部重传指纹，**不是签名，也未冻结为网络 wire**。
- 同最近 ID/摘要返回原 tableRevision/taskId，重启和执行结果更新后仍可重放回执，不再 Start；相同 ID 不同操作冲突。只保留最近一次，旧请求按 stale revision 拒绝；请求方不能改 expectedRevision 后仍称它是原请求重传。
- 删除后保留空表、nextTaskId 和回执，不 clear；taskId 不复用，版本/序号耗尽拒绝。替换建立新 definitionRevision 并清该定义的执行进度。禁用停止倒计时；启用不 Start；对正在运行的任务重复设置 enabled=true 不重置截止点。
- 在已有表恢复前核对当前 scope，错误/更换则 Retired，旧 scope 重新出现也不自动解除。缺表只返回 Empty/NotFound，显式初始化须走可信 owner 入口且消耗额度；未知格式/损坏不自动格式化。
- boot 读到 CountdownArmed 时先呈现 Interrupted，进入 Recovering；有一个写额度后把所有恢复倒计时一次性归一化，不改变表修订/最近回执。恢复期间仅相同已提交请求可以取回执，新变更等待。恢复写失败同样锁定，绝不靠旧 Start 重传续跑。
- 持久 Claimed 未有最终结果时保持该码供查询层映射 Unknown，不再执行该 occurrence；后续独立周历 occurrence 可正常执行，不把一次 Unknown 扩大成永久停用。

**执行与故障边界**

- 每次 poll 至多处理一个候选，稳定 taskId 顺序；失效目标写 Failed 并在 RAM 阻止反复尝试，其他目标继续。显式管理修复后重新验证。无效业务 UTC 暂停日历而不影响单调倒计时。
- Claim 前与提交成功后都重新观察时间/scope 并验证目标。提交期间超出迟到预算、时间失效/大幅校正则 Skipped；scope 变化或明确 retire 立即终止。不因 Flash 同步调用已返回就沿用旧授权。
- 写入/解码失败或超过配置提交预算后状态 Fault，不再读取暂存表或继续执行。回调成功但结果写失败只报告执行已调用、结果未知；无自动重试，无物理 exactly-once 承诺。
- 管理、查询和 poll 使用同一重入保护；回调内重入返回 WouldBlock。retire 可在回调中终止后续旧 scope 写入。线程/中断必须转投同一 cooperative 上下文，不可并发调用这些对象。

**验证和资源**

- [执行器测试](../tests/host/test_task_engine_main.cpp)：17 组行为，包括管理回执冲突/重传、Start 不重复、禁用/启用、删除保留其他截止点、全局 Start 间隔、防同刻漏任务、提交前失败/提交后报错/脏 handle、Claim/结果各阶段、恢复归一化失败、落盘后 scope/时间/目标复查、回调重入/退休、额度不足及迟到跳过、UTC 无源、tick 回绕与计数耗尽。fake apply 解码实际 durable blob，验证调用前确有 Claim。
- 容量 4/8/10 分别创建并执行全部任务。Win32 Release `TaskEngine=124 B`（已含 60 B header）、`TaskRuntimeSlot=48 B`、`TaskSnapshotStore=16 B`、record=128 B；按 Engine＋Store＋record 表＋runtime 槽＋编码工作区小计为 **1,432 / 2,648 / 3,256 B**。不重复计 header，也不包含调用方的请求/回执、平台存储缓存、应用属性及调用栈；不是 R4/Wio 整机测量。
- 全量 Win32 严格构建及 CTest **65/65**（最终回归 26.43 s）；x64 ASan 时钟/时间/游标/快照/Engine **5/5**（1.96 s），未启用 LSan/UBSan。ESP32-S3 stock 3.3.11 工具链 C++11 `-Os -Wall -Wextra -Wpedantic -Werror` 编译 Core、Runtime 和 Engine 测试 `.o` 通过，未链接/烧录完整固件。
- ESP32-S3 `-fstack-usage` 单函数静态帧：mutateVerified **496 B**、poll **224 B**、runOne **80 B**；它们不是调用链峰值，还要叠加 SHA/编码、存储和用户回调。源文件/工具链路径与结果保留在 `build/task-timing-20260922`，不当作设备运行水位。
- 架构/差异检查通过。本批未改平台 Flash 布局、App/Service、公共 begin/API 或业务协议；没有真机掉电/整机耗时/Flash 寿命结论。旧 timing 页面仍有消费者，T2 完成类型化接线后再删除。

### 10.4 第五批：属性、时间和 ESP32 内部组合（2026-09-22）

**新增的最小接线**

1. `api/TaskPropertyTarget`：调用方显式列出 `TaskPropertyBinding(Property<T>&)`，绑定列表和 Property 生命周期覆盖 Target。只允许同一个已 prepare 的 Client、稳定 key 对应的真实 descriptor、可写标量和当前合法值；不存在/未 opt-in/只读/类型或范围不符/整型窄化越界均拒绝。Text/Bytes/容器/Action 不在首版范围。
2. `Property<T>::writeValue`：合并原 normal/checked 两条重复 thunk，统一类型化解码/校验与实际用户回调。网络入口保留原授权检查；任务只通过内部 Target 调用同一应用函数，**不构造 authenticated RxContext，不调用 report 冒充动作**。回调愿意报告状态时复用原 report/限流；不报告时 State 不被任务模块擅自改变。checked 失败沿原 Result 返回，Engine 不自动重试。
3. Target 每次检查真实 Client/spec/回调函数及 context。低层 `onCommand` 覆盖后拒绝本机调度，不能绕过自定义网络处理器的业务检查；重新注册正常 Property handler 后才可重新验证。列表重复目标拒绝，执行/验证重入返回 WouldBlock。没有给每个 Property/EndpointSlot 增加调度标志或第二个回调槽。
4. `TimeSyncService::observeTime(snapshot, observedMillis)` 一次读取配置的 tick，并在该 tick 读取原 WallClock。成功仅表示有单调源，不等于 UTC 可用。`TaskTimeEnvironment` 再读取当前可信 scope；未配置时钟/无 scope 均 fail-closed，而 UTC 未同步/过期不阻止已启动倒计时。scope reader 必须有界、无副作用，只读已验证生命周期缓存，不能每轮读 Flash/请求服务器，也不能返回请求中的 owner。
5. `arduino/platform/esp32/Esp32TaskModule<N>` 只组装原 NVS 端口、固定表/槽/workspace、Store 和 Engine；环境/目标仍注入。固定 namespace/key/容量，传入 NVS 配置只保留 partition/encryption 策略，不能误把 task 写到 credential key。构造不打开 NVS，begin 不初始化/格式化整个 NVS，不自动写空表，不重试潜在脏 handle，也不自动执行任何网络授权动作。

**授权与生命周期边界不变**

- Module 没有挂进默认 WiFi/BLE/WiFi+BLE 产品，没有新公开 facade、feature 或 kind，也不在网络回调里写 Flash。应用仅引入普通库不会创建任务对象/命名空间；Flash 链接增量尚须整固件验证，不能因此宣称二进制零成本。
- Host fixture 的固定 epoch 只用于模拟可信生命周期，不是生产 scope 来源。WiFi-only 的 owner 建立、BLE 管理角色、解绑/物理 reset 的 retire/存储清理顺序仍须 T0b-2 和实际产品接线；Key 刷新不能被误当作 owner 变更。
- Module 的 begin 错误必须终止该次接线；Engine/Store 错误仍保持锁定。C++ 对象重建不证明 Flash 已持久恢复；测试通过 fake powerCycle 重建工作状态，实际端口恢复/断电仍待硬件验证。

**本批门禁与真实资源边界**

- 新增 `test_task_product_main.cpp`，使用真实 Device/Product 绑定、真实 Property 回调、真实 TimeSyncService 及实际 `Esp32NvsAtomicBlobStore` 实现，仅 NVS C API/时钟/作用域来源为主机 stub。涵盖无副作用 validate、report/不 report、checked 拒绝、整数窄化/范围/步长、只读/未注册/类型/重复/低层覆盖、目标重入、同 tick UTC 观察和过期后继续提供单调源。
- ESP32 Module 4/8/10 条分别保存满表再恢复；倒计时通过同一 Engine→真实 Property→report 链。默认构造不打开存储、缺表不自动写、100 次空闲 poll 不写 Flash；网络/身份命名空间保持原字节。提交前失败/提交后报错、dirty handle、容量不足、开库失败/加密准入拒绝和 scope 退休均不自动继续动作；Claim 未确定成功不调用用户函数。
- Win32 `sizeof(Esp32TaskModule<4/8/10>)` 为 **1,512 / 2,728 / 3,336 B**，已含 NVS 端口对象、Engine、Store、表、槽和编码区；另计 binding **12 B/允许属性**、Target **20 B**、TaskTimeEnvironment、现有 TimeSyncService、管理请求/回执、SDK NVS 缓存及栈。这是主机 32 位 ABI，不冒充芯片 RAM/Flash 最终增量。
- 全量 Win32 严格构建/CTest **66/66**（最终 10.74 s）；ASan API/授时运行时/Engine/任务组合 **4/4**（0.55 s），未启用 LSan/UBSan。架构与差异检查通过。
- ESP32-S3 stock 3.3.11 工具链 C++11 `-Os -Wall -Wextra -Wpedantic -Werror`：API/Runtime 严格编译通过；实际 SDK 的 `nvs.h`/`esp_err.h`/sdkconfig 下，NVS 端口和新测试翻译单元编译通过，**不是链接整个固件或运行 MCU 测试**。静态单函数栈帧 `TaskPropertyTarget::dispatch=208 B`、`TimeSyncService::observeTime=32 B`，不是调用链峰值。
- 本批未烧录、清授权、写真实 NVS 或调整网络；未修改 App/Service、未提交推送。删除的是 Property 内已被共用函数取代的重复回调实现；旧 App timing 页面仍有消费者，继续等 T2 替换。

### 10.5 第六批：整固件探针、管理预算与硬件准入（2026-09-22）

本批不添加生产任务接口。新增 `tests/arduino/Esp32TaskResourceProbe` 和 `task_resource_gate.py`，使用同一真实 WiFi Product/TimeSyncService 图编译未启用及 4/8/10 容量；容量只改变 sketch 中的模板参数，不改变库/SDK ABI。通过既有 product extension 捕获已绑定 Client，没有为探针公开 Property.bind 或复制产品组装。

- 探针默认 inert，不自动联网、初始化任务或写 NVS；scope=0 明确拒绝管理授权。它是链接测量工具，**不能直接烧录作为功能验收**。用户示例仍不出现内部 module/storage/workspace。
- runner 仅编译，记录源码摘要、工具版本、ELF/bin 摘要与符号；要求禁用档不保留任务图、启用档真正链接管理/执行/Property/Store 方法及实际模块静态对象。缺少尺寸/符号或编译期间源码变更即失败，不把被链接器移除的空壳当资源优化。复用同一构建缓存，逐档保存制品，不反复全量重编译 SDK。
- 初次探针暴露了 Arduino 深路径头依赖发现、Arduino Client 同名及 Property 私有绑定的测试接法错误；修正探针并复用现有扩展接口，未为让测试通过而放宽生产 API。失败日志保留在本地 build 证据中。
- `test_task_management_budget_main.cpp` 使用真实 CBOR/Route/Delivery codec，并先用 `validateTaskTiming` 检查时间形态。其结果与取舍见 §8.2；没有发送消息、分配 kind 或新增 TaskCommandAdapter，64 B 签名只是长度占位，不是有效 proof。
- 本轮复核了 App/Broker kind 占用、Admin 身份生命周期和不同 accessEpoch 来源，形成 §8.1～§8.3 的明确后续合同。没有新增 owner 数据库、第二条 MQTT、公共分片协议或其他无线网关路径。
- 最终软件回归：Win32 Release **67/67**（18.02 s），任务 Engine/Product/包长实验 x64 ASan **3/3**（0.74 s，未启用 LSan/UBSan）；资源证据解析 **3/3**；单 Arduino 库结构检查 **400 files**、发布工具 **17/17**。这些都是本地结果，不声称未同步测试源码的远端 CI 已闭合。
- 用户同意下一批使用 COM3；本批仅看到 COM3/11/12 端口存在，尚未核对芯片身份、备份或操作串口，没有刷写/真实 NVS/断电/运行堆栈证据。COM11/COM12 保持不动；App/Service、网络和 Git 提交/推送未操作。

**完整固件静态资源：四档通过**

本机 ESP32 Arduino core 3.3.11，Xtensa GCC 14.2.0，FQBN `esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`；CLI nightly-20250331。使用同一源码与普通 WiFi＋授时基线，没有烧录或执行探针。

| 任务容量 | Arduino Flash | 静态 RAM | Flash 增量 | 静态 RAM 增量 | 实际 Module 符号大小 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 未启用 | 941,156 B | 51,296 B | — | — | 不存在 |
| 4 | 952,604 B | 52,920 B | 11,448 B | 1,624 B | 1,512 B |
| 8 | 952,608 B | 54,136 B | 11,452 B | 2,840 B | 2,728 B |
| 10 | 952,620 B | 54,744 B | 11,464 B | 3,448 B | 3,336 B |

- 增量以同源未启用档为基线，不和不同 profile/SDK 固件相减。未启用档无 TaskEngine/Store/Target 等任务图符号；启用档保留 begin/initialize/mutate/poll/recordAt、Target.dispatch、Store.replace，避免链接器删掉业务后获得虚假小值。
- Module 的实际芯片对象大小与第五批 Win32 测量一致；表/运行槽/编码区固定分配，空表和满表的这部分静态成本相同。总静态增量另含 Target/绑定/环境以及探针局部 static 初始化 guard 等；**不是全部生产管理接线后的最终资源**。
- 此表不含运行时 NVS 堆缓存、网络堆峰值、调用链/用户回调栈、未来 CommandAdapter 或实际 Flash 提交耗时；不代表 R4/Wio 可用，也不确定发布容量或写入预算。Arduino 输出的“剩余 local variables 空间”不是实测空闲堆。
- 可复现入口：`python tests/arduino/task_resource_gate.py --config-file <已配置 core 3.3.11 的 YAML> --build-root build/<新证据目录>`。完成目录拒绝覆盖；本轮证据为 `build/task-firmware-final-20260922/resource-report.json`，各 capacity 目录保留 compile.log、ELF/bin、nm 符号及 SDK 配置，包含源码/制品 SHA-256。仅本地保留，不进入用户 ZIP。

真实 NVS、可信 owner/管理 wire 及产品生命周期仍是独立未完成门禁，不因静态资源可接受而提前宣告任务能力。

### 10.6 第七批：COM3 隔离 NVS 与复位恢复（2026-09-22）

**本节保留第七批结束检查点：当时 COM3 停在 Claimed/回调前 hold，物理断电和恢复未完成。第八批已收尾，结果见 §10.7；下列等待描述是历史记录，不是当前现场状态。**

#### 备份与操作范围

- 核对 USB/ROM 芯片身份为原 COM3 XIAO ESP32-S3（USB serial 末尾 `F1:2C`）、8 MB Flash，Secure Boot/Flash Encryption 均关闭；COM11/COM12 未打开串口、复位或刷写。本批不操作 App、服务、网络或凭据轮换。
- 高速 stub 读取在约 418 KB 中断，未写入 Flash；保留失败日志，改用 ROM 读取完成全部 **8,388,608 B**，再用 `verify-flash` 对设备全范围验证摘要一致。备份 SHA-256：`9b483572178dd1d7aa3ed499b7f465e39a936b4429c8b9a2c509d31a2aad363a`。
- 物理 Flash 是 8 MB，但原分区布局只有前 4 MB：NVS `0x9000/0x5000`、OTA `0xe000/0x2000`、app0 `0x10000/0x140000`、app1 `0x150000/0x140000`，OTA sequence=1。不能直接按本轮编译 FQBN 的 8 MB 默认分区覆盖原布局。
- 仅向当前 app0 的 `0x10000` 写入测试应用，**未改 bootloader/分区表/OTA 选择**，写后 hash 通过；测试 binary 322,464 B，小于实际 app0 1,310,720 B。额外提取本轮 NVS＋OTA＋app0 恢复区域 `0x9000..0x14ffff`，1,339,392 B，SHA-256 `3c974d5059230488e8fd26fea83c800d73618183293d9930b822404664dd0861`。
- 本地恢复材料在 `build/task-nvs-20260922/original-full-flash-rom.bin` 和 `original-nvs-ota-app0.bin`。含原有授权/配置，只保留本地，不纳入源码/用户 ZIP；恢复必须用本轮备份，不拿历史副本覆盖当前授权。

#### 最小测试接线，不给正式库加后门

- 新增 `tests/arduino/Esp32TaskNvsGate`：真实 TaskEngine → TaskPropertyTarget → 同一个 Property.onWrite/report，以及真实 Esp32NvsAtomicBlobStore；只有 `IAtomicBlobStore` 外包一层测试计时/窗口观察。没有生产源码改动、测试宏或网络管理入口。
- 为避免触碰生产 `bl_v2_tasks`，测试使用独立 `bl_task_gate/snapshot`；不开放 Esp32TaskModule 的命名空间配置来迁就测试。**本批是相同 Engine/Target/原 NVS 端口的硬件证据，不是完整 WiFi/BLE product/公开任务组合验收**。
- fixture 固定 10 条 RAM 表，分别填入 4/8/10 条实际记录；不把三种占用表长说成三档运行 RAM 实测，编译容量结果仍取 §10.5。只用真实 millis 的倒计时，不联网、不造 UTC、不控制 GPIO。固定 test scope 仅用于这个无网络隔离程序，不能成为生产 owner 来源。
- 默认启动只打开隔离存储并读取，缺表保持 Empty，不自动创建任务/执行/格式化 NVS。`clear` 仅可删除成功解码且属于本 fixture 的 snapshot key，随后重启；未知格式/其他 scope 不清。启动可能新增 namespace 元数据，不等于 SDK 零写入。
- 新增 `tests/hardware/task_nvs_gate.py`：显式端口＋USB serial 核对、保护 COM11/COM12、25 秒有界等待、命令白名单、完整记录/返回码/预期进度检查；只保存固定字段的测试事件，不收集任意串口原文。不会烧录、选择其他设备、擦 Flash，且不把 reset 标成断电。Python 仅是维护者工具，普通 Arduino 用户无此依赖。

#### 已通过的真实门禁

| 场景 | 实际结果 |
| --- | --- |
| 缺表及显式初始化 | 第一次 inspect 为 Empty/count=0/writes=0；显式 fill 才写 76 B 空表 |
| 4/8/10 条填满 | 实际快照分别 588/1,100/1,356 B；逐条完整读取、ID/版本/进度恢复成功 |
| 正常倒计时 | 每次 Start 只推进一次序号；Claim 写入后调用真实 Property 回调，再写 Completed；不是 report 代替回调 |
| 变更 | 4 条表 Replace＋停用/启用后再执行成功，definitionRevision 增长，其他记录保持 |
| 重启及原请求重传 | 三种表长 ESP.restart 后仍为 Completed；原 Start ID/操作/CAS 重放返回原回执，0 次新写、0 次回调 |
| 空闲和表满 | 各表 100 次 idle poll 不增加写入/回调；10 条表新增拒绝，修订与写次数不变 |
| Armed 窗口复位 | 10 条表中已启动倒计时变为 Interrupted；最近 Start 重放不恢复运行；一次恢复写后 Ready |
| Claim 前窗口复位 | 尚未写 Claim，原 Armed 同样归一化 Interrupted，0 次回调 |
| 回调后/结果前复位 | 窗口前真实回调 1 次；重启为 Claimed（用户语义 Unknown），原请求重传/恢复/idle 不再调用、不补写成功 |
| Claim 后/回调前物理断电 | 已到 hold，48 次累计写入中最新 Claim 成功，窗口 callbacks=0；待用户实际拔插后读取 |

前三个窗口使用串口下载模式＋硬复位退出 hold，**没有切断供电**。即便 SDK resetReason 显示上电类值，也不能代替用户实际断电证据。读回是重启后新建端口/Engine，不是失败 handle 内读回。

#### 初步资源与时延：不能把平均值当上界

当前 40 份命令记录均无驱动校验失败，合计 **48 次成功 snapshot replace**；不含 namespace 元数据、clear 或 NVS 内部物理写次数。单次端口 `set_blob＋commit` **1,390～54,707 µs**；4 次完整执行 poll（Claim＋回调＋结果）**53,155～64,013 µs**，后者含测试日志开销。

- 常见单次写约 2～8 ms，但出现 46～55 ms 长尾；未独立追踪 NVS GC/擦除，因此不把长尾原因写成已证实。没有强制填满分区/长时间磨损测试，不是芯片绝对最坏值。
- 本隔离无网络固件空闲堆约 **353,108 B**，观测最小堆 **347,732 B**；loop task 栈最小剩余 **5,108 B**。这些包括 fixture/SDK 开销，不是启用 WiFi/BLE 后的余量或全调用链保证；未测 ipc0/蓝牙任务。
- NVS `used_entries` 从缺表后的 109 变为 4 条130、8 条147、10 条154～155；这是 SDK 逻辑条目统计，不是 Flash 写入寿命计数。
- **产品预算仍不冻结**：不能把 synchronous NVS commit 塞进“1 ms run() 必然返回”的承诺，也不能用无限调大提交预算掩盖通信长停顿。下一步在既有 cooperative 管理消费点做有限次数通信共存测量；若不可接受，再评审异步存储端口/分阶段提交，不预先加每任务线程或另一个执行器。测试的 1,000 ms 提交限额只是防误判测量配置，绝非生产默认。

软件回归 **67/67**（23.32 s），新驱动离线测试 **4/4**，架构检查通过；已把驱动离线测试加入既有 CI 清单，但未推送、不声称远端 CI。隔离固件 core 3.3.11 完整编译通过：Arduino Flash 322,309 B、静态 RAM 27,256 B（应用 bin 有额外头/对齐，为 322,464 B）。测试制品 SHA-256 `4bf26aea48c4b30fc4744c99031a2445bb5733e40332f781f690e100c7ca7091`。与 §10.5 的 WiFi 图不同，不能拿两者相减冒充模块增量。

**第七批安排的下一检查点（第八批已执行）**：拔插后核对身份/快照/不重放，再恢复原 NVS/OTA/app0 并校验。实测及仍未验证的原固件在线状态见 §10.7。

### 10.7 第八批：实际断电恢复、现场还原与退休清理（2026-09-22）

- 用户确认 COM3 拔插，读取前未再次复位/刷写。USB serial 与基线一致；inspect/replay/recover/idle 均通过：count=10、revision=15、第一条 definition=1/start=5/progress=Claimed，callbacks=0/writes=0。共 44 份命令证据，snapshot replace 仍为 48 次。断电持续时间未用仪器测量，只为 Claim 后/回调前窗口补物理证据。
- 校验原完整备份与切片大小/SHA-256 后，ROM 写回 `0x9000` 起 1,339,392 B（19.8 s，写后摘要通过）。保持下载模式，对 **全部 8,388,608 B** 再验证与原完整备份摘要一致，然后 hard reset。未整片 erase、未操作 eFuse，COM11/COM12 不动。
- 原 NVS/授权/固件字节恢复也移除了测试新增 namespace；备份继续保存在本机，未上传。两次有界被动串口读取均 0 B，未向原固件发送 fixture 命令。因此“恢复字节并复位”通过，**原应用运行/联网没有新证据**，不以无 panic 日志冒充上线。
- 原始证据和恢复日志：`build/task-nvs-20260922/physical-cut-and-restoration.json`、`events/`、`restore-original.log`、`verify-restored-full.log`、`boot-restored.log`。明确区分用户断电确认、设备观察、文件校验与应用在线。
- 按 §8.4 补内部持久清理，没有改变 schema 1、记录/模块布局或 wire。新增 Engine 行为组覆盖 ready/未开始拒绝清理、成功/重复清理、同实例不能复活、重启缺表、提交前/后/脏 handle 故障；回调内清理 WouldBlock。真实 ESP32 端口＋host NVS stub 验证命名空间隔离、空 key 的 commit、失败后不重试及网络/授权 blob 不变。
- 清理失败时旧任务表可能重启后仍存在，这是“不得继续清授权/报告退休完成”的原因。**正式产品移除事务尚未接入**；本批是底层接口/故障测试，不是生产解绑真机门禁。测试 fixture 已删除直接绕过 Engine 清 key 的旧路径。
- Win32 全量构建后补重建受影响目标，最终 CTest **67/67**（24.28 s）；任务相关 x64 ASan **3/3**（0.78 s），首次执行缺 DLL，补工具链进程 PATH 后通过。驱动离线 **4/4**、发布工具 **17/17**、架构检查通过。Win32 Engine 仍为 124 B、Module 4/8/10 条仍为 1,512/2,728/3,336 B；不当作联网运行峰值。
- 新增清理固件仅做编译，不再刷已恢复 COM3；旧硬件证据始终对应 §10.6 固件摘要，不混为新实现真机通过。
- stock ESP32-S3 3.3.11 最终完整编译/链接通过，ELF 确认包含 `TaskEngine::clearRetired` 和 `TaskSnapshotStore::clear`。同一隔离 fixture 的 Arduino Flash **322,777 B**（比 §10.6 增加 468 B）、静态 RAM **27,256 B**（不变）；bin 322,928 B，SHA-256 `48909599d1418a949cfcc03876f5e7368a9f266020ad347310761620fefa0559`。制品在 `build/task-retirement-20260922/compile/`，日志在 `build/task-nvs-20260922/retirement-compile-final.log`。这不是四档联网产品资源重新验收，也不是新清理路径的物理断电门禁。

### 10.8 第九批：可选产品生命周期与回归（2026-09-22）

- 按 §8.5 新增一份可选组合，既有 Product 的 attach/start/run/stop/reset 委托到它；不修改 Product、IProductLifecycle、Arduino begin、模式选择或平台 AccessStore。其 Module.begin 只尝试一次；失效或清理后的旧 Engine 不通过断线重连自动复活。
- `TaskPropertyTarget` 允许在产品 attach 阶段单次绑定 Client，拒绝跨 Client 重绑，保留已有直接绑定构造。默认产品无新增字段，Win32 Target 仍为 **20 B**，可选组合 **24 B**，Module 4/8/10 条仍为 **1,512/2,728/3,336 B**。
- 资源探针改用真实通用组合，删除 `ProbeExtension`、全局 `attachedClient`、局部静态 `taskModule()` 与 loop 中重复的 Engine.poll 路径；默认 inert、scope=0、不自动写任务不变。资源 runner 同时核对新的组合/clearRetired/Store.clear 链，禁用档不得残留任务符号；历史资源制品不覆盖。
- 主机新增五组生命周期测试：启动/时间转交/run/普通 stop/restart/网络 reset；任务清理和授权根各自提交前/后报错；网络与属性回调请求退休；未绑定/不支持/损坏表/开库失败/无 scope；通信本轮更新 scope 后不执行旧任务。含重新启动认领后再次 reset 的根清理，以及失败实例不重试/不恢复运行。
- 故障测试调用 **真实 Product、TaskEngine、ESP32 NVS 端口、DeviceAccessStore.eraseAccess**；通信及 NVS SDK 是主机 fake，不能写成 BLE/WiFi 真机解绑。NVS fake 修正为仅提交所属 namespace，避免一次任务 commit 意外帮另一个 dirty 根提交而让测试失真。未改生产 NVS 行为或布局。
- 验证：Win32 全量严格构建后重建最终受影响目标，CTest **67/67**（10.34 s）；最终 x64 ASan 任务组合/执行器/Store **3/3**（0.63 s，无 UBSan/LSan）；资源解析 **3/3**、NVS 驱动离线 **4/4**、发布工具 **17/17**、架构及差异检查通过。新增行为扩展原测试可执行程序，不靠新增测试编号计算功能完成度。
- 本批仅枚举 COM3/11/12，USB serial 均与原记录一致；未打开串口、重置、烧录或清授权。上批整片恢复证据继续有效，但本批没有新增原固件在线/通信共存证据。App/Service、网络和 Git index 未改，未提交/推送。

**本批完整固件资源重测：四档全部通过**。同一 stock ESP32-S3 core 3.3.11、默认 8 MB 分区、WiFi＋授时图，无烧录；本表取同源未启用档作基线，不混用 §10.5 的旧 probe。

| 容量 | Arduino Flash | 静态 RAM | Flash 增量 | 静态 RAM 增量 | Module 符号大小 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 未启用 | 941,152 B | 51,304 B | — | — | 不存在 |
| 4 | 952,800 B | 52,888 B | 11,648 B | 1,584 B | 1,512 B |
| 8 | 952,804 B | 54,104 B | 11,652 B | 2,800 B | 2,728 B |
| 10 | 952,808 B | 54,712 B | 11,656 B | 3,408 B | 3,336 B |

启用档确认实际链接组合与 Engine/Store 清理、管理、执行路径；禁用档无任务符号。清除 probe 捕获/局部静态 guard 后，本次总静态增量比 §10.5 的对应差值少 **40 B**，这是探针组装收敛的结果，不宣称所有用户固件都省同样字节。仍未包含未来管理适配器、SDK 运行堆/栈峰值及通信共存写时延。编译无 warning/error；解析/符号/编译期间源摘要一致性全部通过。

证据：`build/task-lifecycle-20260922/resource-report.json`，含四档源码/ELF/bin 摘要；各 capacity 目录保留 compile.log、symbols.txt、sdkconfig 与制品。重跑使用新证据目录，旧物理测试和原备份不覆盖。

**剩余主线不扩散**：先完成 §8.2 的可信 scope、根连续性、Prepare/Grant/单笔 pending/查询授权，再接真实服务退休事件与产品 opt-in。当前清理顺序不是跨 blob 原子事务，也没有消除“确认成功前掉电，旧根与旧表仍可能存在”的边界。通信共存与生产写预算仍未测；不重开 COM3 同一断电窗口，不转服务器维护或网关扩展。

### 10.9 第十批：Grant 合同、流式摘要与拒绝门禁（2026-09-23）

- 新增一对 `TaskManagementGrant.h/.inc` 并纳入原 `BlinkerProvisioningController.cpp`；不增加 Arduino 翻译单元、公共 begin 重载、平台宏、NVS key、第二套会话表或网络入口。尚未接 Engine，不把旧 schema 1 当生产 scope。
- 包长测试改用实际 Grant codec，删掉 u32/单包候选的手写重复编码；保留三种合法时间形态及 Route/Delivery 的最坏 Prepare 182 B、Grant **235 B** 门禁（含 10 B BBP 头与 4 B CRC）。没有发送真实 task/TelemetryData 消息。
- 共享向量覆盖 Ed25519/P-256，C++ 核对 canonical 编码/流式 digest、每一个截断点、逐字节扰动、五种 binding 错误、零/缺字段、错误长度/版本/算法/key、非 canonical/尾随内容、provider 失败和输出不残留。C++ 使用明确标注的验签边界 double；独立 Node oracle 用实际 crypto 验证同一向量并逐字节改动 transcript/signature 拒绝，**不是 Service/App 实现已经接通**。CI 增加该 oracle。
- Win32 /W4 /WX 全量构建＋CTest **68/68**（21.62 s）；x64 ASan 相关 **5/5**（1.56 s），无 UBSan/LSan；独立 Node oracle、架构、资源解析 **3/3**、NVS 驱动离线 **4/4**、发布工具 **17/17** 通过。单 Arduino 库边界检查 **403 files** 通过（不包含 tests/docs）。
- stock 3.3.11 工具链中的 Xtensa GCC 14.2 对实际 `BlinkerProvisioningController.cpp` 做 C++11/Os、无 exceptions/rtti、严格 warnings 的独立交叉编译通过。仅 `.o` 编译，**不是整固件链接/真机验签/资源矩阵复测**。静态单函数栈帧：decode 192 B、digest 192 B、verify 160 B，不含调用链/crypto provider，不能当总栈峰值。工件：`build/task-grant-20260923/`。
- 未开串口、烧录、清授权或重做 COM3 已完成窗口；未修改 App/Service、网络、Git index，未提交/推送。剩余步骤以 §8.6 最后一段和总主线为准。

### 10.10 第十一批：持久绑定、等待恢复与换根门禁（2026-09-23）

**实现及收敛**

- 新增独立 `TaskAuthority.h`，仅包含自有 instance16/scope16/rootDigest32 元数据和 Pending/Current/Retired 观察；没有凭据、服务器请求、SDK、动态内存或新增会话表。Engine、Store、时间适配器和可选组合共同消费它，删除任务核心及测试消费者的 u32 epoch/ScopeReader 入口，不保留两套管理分支。
- 同一原子 blob 的 schema 2 保存完整 binding；header 编码128 B，record 仍128 B，CRC4 B，空/4/8/10 条分别 **132/644/1,156/1,412 B**。版本检查显式拒绝有效旧 schema 1，不伪造128-bit scope、不迁移、不清库；CRC 仍不是认证或防回滚。
- Pending/同 owner 根不匹配进入 AwaitingAuthority，拒绝记录读取/变更/执行；精确绑定恢复后重建游标，间隙不补跑日历、倒计时 Interrupted、已有 Claimed 不重放。明确 Retired/不同实例或 scope 进入终态；普通通信继续。未配置 AuthorityReader 默认 Pending，不能回读任务表头冒充当前授权。
- 内部 `rebindVerified(expected,replacement)` 只接受同 instance/scope 的精确 CAS，原子更新同一表头，不改变定义、管理修订或最近请求回执。重复已提交换根不再写；本地重放摘要改为 `BTM2 + scope16 + 操作`，不含可轮换 root，避免换根破坏已经提交的 Start 去重。该入口要求外部先验证签名根连续性，**本批没有实现该证明或允许任意 Grant 换根**。
- 修正提交后权限变化的一致性：成功 replace 先发布对应的内部 header/count，再观察授权；记录与运行槽的移动/替换保持对应。不增加整表回滚副本。可恢复暂停不暴露未确认回执；后续用同请求核对。存储报错保持 Fault；即使同时 Pending，超过提交耗时预算仍 Fault，不能当普通等待恢复。

**软件与编译证据**

- 更新快照独立 golden 为260 B，独立 Node 大端编码/位运算 CRC 产生固定向量；不是用被测 codec 自证。覆盖逐位扰动/截断、三段零 binding、旧版本拒绝及4/8/10容量。
- 新增 boot/运行/提交/Claim/回调/结果窗口的 Pending、恢复及明确退休测试；覆盖五种变更提交后失权的内部数量/修订/去重一致性、授权间隙日历不补跑、初始化失败不毒化空实例、同 owner 根 CAS/重复换根/错根、存储提交前/后/脏 handle 故障及重启恢复。另补换根提交后再失权、精确 CAS 重试、原 Start 在换根后及重启后重传不再启动/写入；组合测试验证等待态继续轮询但不重开 NVS。都是 host 故障注入，不冒充物理断电。
- Win32 /W4 /WX 全量构建后，补充专项测试再重建受影响目标，最终 CTest **68/68（6.56 s）**；x64 ASan Engine/Store/Product/Grant/管理预算 **5/5（0.99 s）**，未启用 UBSan/LSan。独立 Ed25519/P-256 oracle、架构检查、资源解析 **3/3**、NVS 驱动 **4/4**、发布工具 **17/17**、单 Arduino 库边界 **404 files** 通过。
- Win32 ABI：header120 B、record128 B、Engine184 B、运行槽48 B；含 Engine/Store/表/槽/编码区的4/8/10条为 **1,548/2,764/3,372 B**，另计平台。ESP32 Module 的 host 实例为 **1,632/2,848/3,456 B**（各增加120 B）；Target20 B、可选组合24 B不变。这些数字不是运行堆/栈峰值。
- stock ESP32-S3 3.3.11 / Xtensa GCC14.2 的最终 Runtime C++11/Os/无 exceptions/rtti/严格 warnings 交叉编译通过。单函数静态栈帧 initialize/rebind256 B、mutate576 B、poll288 B；不含调用链、crypto、SDK/NVS 或用户回调。工件为 `build/task-authority-20260923/BlinkerRuntime.o/.su`。
- schema 2 隔离 `Esp32TaskNvsGate` 最终完整编译通过：Arduino Flash **324,481 B**、静态 RAM **27,368 B**；bin SHA-256 `46cb9ff2e0856d0c42103b7fbc86238920246b6a3a072d8d6324383383899036`。日志 `build/task-authority-20260923/nvs-compile-final.log`。**没有烧录**；旧 schema 1 的断电/写入时延不能直接算作新格式验收。

**本批四档同源 WiFi 产品静态资源复测全部通过**。stock ESP32-S3 core 3.3.11 / Xtensa GCC14.2，FQBN `esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`；同一 WiFi＋授时图，不是运行/联网验收。

| 容量 | Arduino Flash | 静态 RAM | Flash 增量 | 静态 RAM 增量 | Module 符号大小 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 未启用 | 941,160 B | 51,304 B | — | — | 不存在 |
| 4 | 954,720 B | 53,008 B | 13,560 B | 1,704 B | 1,632 B |
| 8 | 954,724 B | 54,224 B | 13,564 B | 2,920 B | 2,848 B |
| 10 | 954,724 B | 54,832 B | 13,564 B | 3,528 B | 3,456 B |

禁用档没有任务图符号；启用档除原执行/读写/清理/组合方法外，额外要求实际链接 `rebindVerified`，防止未测新路径造成虚假小值。Module 芯片对象大小与本批 Win32 ABI 测量一致，比旧布局固定增加120 B，不随条数增加；总增量同时含探针 Target/环境/绑定。没有扩大每条运行槽、复制解码表或默认 facade。

四档编译无 warning/error，源码摘要在整轮编译前后相同，符号/资源解析通过。证据 `build/task-authority-resources-20260923/resource-report.json` 记录工具/源码/ELF/bin SHA-256，各 capacity 子目录保留日志、符号、SDK 配置与制品；它们不进入 Arduino ZIP。本表不含运行时 NVS 缓存、调用链峰值、未来签名管理适配器与联网提交时延，不作为 R4/Wio 结果，也不沿用 §10.8 的旧值。

**下一步与现场边界**

先确定实际 DeviceKey/BLE Admin 根的域分离摘要及可信映射来源、初始化/根连续性签名合同，再接既有 Grant 的 Prepare/有界单笔 pending/nonce 消费和查询权限；复用当前 binding/Store/Engine，不另建 owner 数据库。不以临时 MQTT 登录口令作为根，不以 Shared 凭据变化退休 owner 任务。真实根证明、生产移除事件与产品 opt-in、三端 wire/页面、联网共存写时延及 schema 2 硬件验证仍待完成。

本批未打开串口、刷写、复位、擦授权或改变网络，COM3/11/12 均未操作；未修改 App/Service、未暂存/提交/推送。没有新增正式任务 feature/kind/facade，也未删除仍有消费者的旧 App timing 页面。

### 10.11 第十二批：根来源、初始化换根与跨端校验（2026-09-23）

- 一对 `TaskAuthorityContract.h/.inc` 复用 IDeviceKeySource、IControllerCredentialSource、Sha256/CBOR 与原 Grant verifier；读源、操作 codec/摘要/验证无状态，未新增平台宏、存储 key、线程、签名封套或初始化队列。根摘要不与某条 Cloud/BLE session 绑定；schema 2/Engine/默认 facade 未改。
- 主机测试实际使用 **ConfiguredDeviceKeySource、DeviceAccessStore 和其原始 codec/角色约束**，仅 blob 后端为内存 fake。覆盖三种根组合、Shared 安装/轮换/撤销不改摘要、DeviceKey/Admin 变化改摘要、缺任一根不降级、实例变化、存储失败，以及返回错误 domain/ID/epoch/权限/版本/算法/secret 的 source 拒绝。未写真实 NVS。
- 初始化88 B/换根121 B golden、摘要和 Grant 绑定验证复用原测试 executable；覆盖所有截断、逐字节改动、短输出/非 canonical/尾随、零/错误长度/无变化根、错误实例/当前根/nonce/签名 provider 失败，失败不发布半个结果。Route/Delivery 完整预算147/180 B，既有 Grant235 B、一般 Prepare候选182 B不变。
- 独立 Node oracle 从相同 dummy 根材料重建指纹/根/操作摘要；新增两份真实 Ed25519 初始化/换根签名并逐字节修改操作 transcript 后拒绝。原 Ed25519/P-256 Grant 向量继续验证。C++ 注入的 verifier 仍是边界 double，不能将其单独说成 MCU 实际密码库验签通过；Node 不是部署好的 signer。
- 跨端只读审查纠正 Admin raw secret 不能由 Service 获取这一假设，实际采用 App/Service 已有指纹。版本1兼容及后续更高版本/已验证根元数据同步要求见 §8.8；没有为了让测试通过新增服务器秘密存储。
- 最终 Win32 /W4 /WX 全量构建/CTest **68/68（20.22 s）**；x64 ASan Grant/预算/Engine/Product **4/4（1.15 s）**，未启用 UBSan/LSan；独立 Node oracle、架构、单库边界 **406 files**、发布工具 **17/17** 通过。
- stock ESP32-S3 3.3.11 / Xtensa GCC14.2，对实际 `BlinkerProvisioningController.cpp` 以 C++11/Os、无 exceptions/rtti、严格 warnings 交叉编译通过；工件 `build/task-authority-contract-20260923/BlinkerProvisioningController.o/.su`。静态单函数帧 rootDigest304 B、Admin指纹192 B、changeDigest192 B、change verifier128 B；不含调用链、凭据读源、NVS/密码实现，**不是总栈峰值或整固件资源复测**。§10.10 的四档结果只对应上批源码；本批未重跑已过 NVS/刷写/断电窗口。
- 未打开串口、烧录、复位、清授权或改网络；未修改 App/Service、Git index，未提交/推送。没有删除尚有消费者的旧 App timing 页面，也没有宣告任务 feature/kind/API。当前唯一下一步为 §8.8 最后一段及总主线 §6。

### 10.12 第十三批：根缓存、单笔管理与软件闭环（2026-09-23）

- 原根合同模块新增可选缓存；新增 `TaskManagementSession.h/.inc`，仍纳入 `BlinkerProvisioningController.cpp`，不增加责任翻译单元。Engine 只增加安全的内部持久绑定读取，并将原无副作用修改校验共享给管理入口；存储 schema/布局、原 BTM2 最近回执和执行路径不变。
- 实际软件连接覆盖：安全源读取→独立根/已验证表匹配→Prepare→单次 Grant→Engine 原子提交→页面管理连接退出→倒计时执行→授权读取结果。含新挑战/同 requestId 的 Start 去重，签名前后或提交后根变化、过期、断连、重入、错误通道、错误 nonce/格式、随机失败/全零、时钟反向/正常回绕和存储写后失败。
- 专门复核两个可用性窗口：① 仅元数据查询不要求先知道 tableRevision，单条定义查询仍用精确 CAS；② Flash 已成功后 TTL 到期，输出清空、持久修订保留，以新挑战查/重试原 requestId，不重新启动。仅清管理连接不退休已安装任务。
- 新测试 `test_task_management_session_main.cpp`；既有 Grant 测试扩展修改/读取 canonical golden，Node 独立构造 transcript/摘要与原真实 Ed25519/P-256 验签回归。Session 内的 provider 是**摘要精确匹配替身**，不是 MCU 密码运算/远端 owner 鉴权证据。Flash/时间/目标均为主机 fixture，没有冒充现场 NVS 或物理断电。
- Win32 `/W4 /WX` 全量构建、CTest **69/69** 通过；x64 ASan 的 Session/Grant/预算/Engine/Product **5/5** 通过。架构检查、408文件单 Arduino 库白名单、发布工具 **17/17** 通过。当前摘要向量共享同一 properties 文件，未新增另一套签名封套/服务依赖。
- Win32 实测 `TaskAuthorityCache=76 B`、`TaskManagementSession=336 B`，合计412 B，不随任务数量/连接数倍增；32位门禁上界分别80/352 B。默认产品没有实例化它们；此处不是整机运行 RAM/Flash，也不替代先前0/4/8/10链接资源探针。Challenge/Result 是调用方临时输出，密码 provider/调用链栈必须在产品接线后另测。
- stock S3 3.3.11 的 C++11 `-Os -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti` 严格编译实际 ProvisioningController/Runtime 两责任单元通过。`.o/.su` 保留在 `build/task-management-session-20260923/`；Session Commit 自身静态栈帧368 B、修改/读取摘要各208 B，**不包括被调用 Engine/存储/验签的叠加峰值**。没有编译/上传整固件，不宣称硬件共存或实时预算已通过。
- 删除包长测试中无真实消费者的手写候选 Prepare，改用实际编码器，合法定义最大路由帧仍182 B，Grant235 B，Initialize147 B/Rebind180 B；不因预算通过分配协议能力。
- 默认 facade/普通通信及 App/Service 未改；不打开串口、不刷板、不清授权、不调整网络，不暂存/提交/推送。§8.9 明确网络 decoder、可信回执、产品根/断连事件、真实签发与三端闭环仍待完成；下一步先完成该 wire 准入，再接现有唯一 Session，不再重复核心引擎/缓存/队列。

### 10.13 第十四批：严格 wire、认证回执与跨语言门禁（2026-09-23）

- 实现范围见 §8.10：无状态 `TaskManagementContract` 分离原 Session 的 codec/摘要责任，共享定义读写和语义校验；Session 仅增成功提交的可选completed输出与结果种类/index，不增持久状态。未改存储schema、Grant wire、默认facade、普通通信或现有平台依赖。
- 既有 Grant/Session/Budget 测试覆盖 Create/Replace/Remove/SetEnabled/Start 与 Read，所有截断前缀、尾随/indefinite/非规范整数、u32越界、零requestId、隐藏定义、输入缓冲失效、输出空间不足；认证结果覆盖两个角色×四类结果、逐字节篡改、错误根/nonce/request/操作/密钥/角色和失败输出清空。
- 软件事务链增加实际 codec→Prepare→Challenge→Grant→Engine→completed→HMAC结果→验证；提交后TX不足不撤销持久修订，迟到重复不重用completed。Session验签provider仍为精确摘要替身；Node oracle独立执行真实Ed25519/P-256及HMAC，不能将两类证据合并为MCU三端验签通过。
- 在原 `task_management_grant_vectors.properties` 增完整挑战及8组认证结果，Node独立重建canonical transcript，用真实HMAC和篡改对照核验。结果golden为合成布局/认证向量，不是线上授权记录；不增加第二套签名封套或新测试服务。
- Win32 Release `/W4 /WX` 全量构建与CTest **69/69**（20.65 s），x64 ASan Session/Grant/Budget/Engine/Product **5/5**（0.90 s）通过。独立Node oracle、架构检查、410文件单库发布白名单、发布工具 **17/17** 通过。挑战/认证单条结果最大路由帧 **197/236 B**；现有修改/Grant/初始化/换根预算不变。
- Win32 `TaskAuthorityCache=76 B`、`TaskManagementSession=336 B` 不变，尚未默认实例化。stock S3 3.3.11 实际 `BlinkerProvisioningController.cpp` 的C++11/Os/严格warnings、无exceptions/rtti交叉编译通过，工件为 `build/task-management-wire-20260923/BlinkerProvisioningController.o/.su`。静态自身栈帧：HMAC transcript272 B、结果编码128 B、结果验证336 B、修改/读取/挑战解码240/128/208 B、Session commit384 B；**不是嵌套调用链总峰值**，也不代表整固件RAM/Flash或联网共存门禁完成。
- 本批没有打开串口、刷写/清授权、改网络、改App/Service或暂存/提交/推送。真实adapter/channel、根事件、签发、结果消费者/错误回执及新版三端安装/共存仍未完成；下一批接最小实际入口，不重复编解码/缓存/执行器或扩服务器维护。

### 10.14 第十五批：来源适配、实际根回执与生命周期门禁（2026-09-23）

- 新增 `TaskCommandAdapter.h/.inc`，仍编入原ProvisioningController责任单元；一般网络DTO→严格解码→唯一Session→Engine→实际密钥回执已接。重构原根读取为共享load/validate/hash，原golden/root/wire/schema不变，不复制一套签名或凭据表。
- 原69个测试目标内扩展adapter安装/元数据/单条读取、RX复用、7类来源差异（连接代际、session、transport、peerId、peerKind、角色、有无路由）、重复Commit、同/错代际transport断开、停止重启、超时、TX不足、非法请求/占槽、签回执时取消/换根/退休/重入、写后失败及回执源失败；同owner换根仍保留原表并走新Grant。
- WiFi-only/BLE-only/混合源使用真实DeviceAccessStore编码/加载（内存blob）分别生成并验证回执，错误角色/根摘要与缺源拒绝；不把指纹当secret。生命周期测试实际实例化可选模板与adapter，覆盖poll期限、stop取消、reset先退休cache再清任务/根；硬件层和签名provider仍为测试替身，不是远端服务授权证据。
- Win32 Release `/W4 /WX` 全量构建和CTest **69/69**（37.12 s）、x64 ASan Session/Grant/Budget/Engine/Product **5/5**（1.31 s）通过。独立Node真实签名/HMAC/root golden、架构检查、412文件单库发布边界、发布工具 **17/17** 通过。Challenge197 B、单条认证结果236 B的路由帧预算未改变。
- Win32 adapter **64 B**、原cache **76 B**/Session **336 B**；可选生命周期 **28 B**（原24 B），普通Product保持原结构。stock S3 3.3.11实际责任单元C++11/Os/严格warnings交叉编译通过，工件 `build/task-command-adapter-20260923/`。自身静态栈帧：adapter receive368 B、实际凭据回执208 B、共享rootDigest240 B；不含相互调用/Engine/存储/密码provider叠加峰值，不冒充任务运行总资源。
- 因可选生命周期增加管理接缝，重新执行同源0/4/8/10完整固件链接、符号及源码摘要门禁，全部通过。Arduino CLI 1.5.1、stock S3 3.3.11，报告 `build/task-command-resources-20260923/resource-report.json`；旧报告保留。禁用档Flash **941,160 B**、静态RAM **51,304 B**，与§10.10数值相同；扩展符号门禁同时排查根缓存/管理/适配器/codec残留，而非只查Engine。

| 任务容量 | Flash（比本批禁用档增量） | 静态RAM（比本批禁用档增量） | 原任务Module自身 |
| --- | --- | --- | --- |
| 4 | 958,108 B（+16,948 B） | 53,016 B（+1,712 B） | 1,632 B |
| 8 | 958,112 B（+16,952 B） | 54,232 B（+2,928 B） | 2,848 B |
| 10 | 958,112 B（+16,952 B） | 54,840 B（+3,536 B） | 3,456 B |

相对§10.10各启用档静态RAM增加8 B（含全局布局/对齐）、Flash增加3,388 B；不是默认设备成本。探针仍是原WiFi/time＋可选执行器组合，management指针为null，**没有实例化完整adapter/cache/Session或真实签发者**；其可达管理接缝代码仍有链接成本，上表不能冒充完整生产管理组合的资源/运行峰值。没有上传或开始NVS/联网实物测试。

- 本批未操作串口/开发板/网络，未改App/Service或Git index，未提交/推送；实际BBP/平台根写入通知、远端签发/消费及新版三端安装/通信共存仍待接线。下一步见 §8.11，不重复核心底座。

### 10.15 第十六批：真实 WiFi 分发类与共享 carrier 门禁（2026-09-23）

- 在原Task Session测试目标中，串接真实`DeviceRuntime → TransportHub → TaskCloudTransport → MqttFrameTransport`，另显式实例化真实`ManagedMqttTransport`模板。覆盖Hello前/未协商/预算不足拒绝，Initialize→Mutation→认证结果，原生RX污染后仍正确工作，Flash/验签/签回执均不处于socket回调内。
- 回执WouldBlock多轮只提交一次；MQTT实际重连generation递增、旧Grant拒绝；收包到cooperative处理之间stop/换根/过期不落盘。错误方向、flag、sequence和坏inner拒绝；Flash提交期间断连仍保留已落盘事实但不发布回执，退栈擦除workspace；未送出的challenge绝对超时不复活nonce。普通Hello原Runtime仍工作、没有第二个MQTT连接。
- MCU和Service共享carrier失败字节`83010643820302`（Commit/Unauthorized）；两侧长短bstr边界、截断、越界及canonical拒绝通过。包长测试保留历史Route估算，但另标实际physical carrier数值，不把它们混为同一分片或路由方案。
- Win32严格最终构建/CTest **69/69，2.32 s**；x64 ASan相关 **6/6，0.81 s**（任务5项＋受影响Runtime授时1项）。未运行UBSan/LSan。Node真实Ed25519/P256/结果HMAC oracle通过；Session集成的验签器仍是精确摘要替身，不冒充生产签发。
- Service共享协议包typecheck/build及测试 **32/32**；BrokerTypeScript构建和TimeSync/App target/LocalControl相关 **26/26，3.29 s**。新增physical/virtual拒绝和Route/Delivery白名单测试。最初使用Node24与现有SQLite ABI不匹配，改用已安装Node22.22.3；没有重装依赖。并行编译压力下既有HTTP测试曾超时，同源串行复测通过，未延长其超时或删门禁。
- stock S3 3.3.11工具链严格C++11编译`BlinkerProtocol/Runtime/ProvisioningController`和含真实ManagedMqtt模板的测试翻译单元；本地工件`build/task-cloud-20260923/`。`.su`显示新transport自身poll帧96 B、receive80 B、Hello通知32 B；**不是嵌套调用链峰值或MCU运行水位**。
- 单Arduino结构 **413 files**、发布工具 **17/17**、资源解析 **3/3**、架构和diff检查通过；资源工具的禁用符号识别扩展到TaskCloudTransport。未重跑四档整固件链接：§10.14的资源仅代表该批旧源，不可当本批vtable/transport增量。默认产品无新增实例数据，接口虚函数对Flash的实际影响待完整产品组合链接一起量化。空闲轮询不重复擦同一空workspace，实际释放后才清零。
- 未做真实签发/数据库迁移、Broker任务启用、App页面变更、设备烧录、网络变更或Git暂存/推送。当前下一步以主线§6/Service执行计划为准。

### 10.16 第十七批：Service合同与真实密码学对照（2026-09-23）

- 设备源码、存储/版本和212 B载体预算未改。Service共享包新增typed inner合同，沿用§8中的完整字段和digest域；MCU golden原样复制到Service测试工件并比较，不重新生成已有向量。
- `task-contract.js`负责字段和密码学协议，`task-operation-binding.js`负责同步的完整意图/challenge及回执语义核对；仓库事务、owner/tenant/current instance/proof、scope分配和deadline消费不放进codec。Service固定验DeviceKey回执；Admin role的codec验证能力不代表Service持有Admin secret。
- 共享CBOR组合补齐任务需要的负整数、bool/null和有限32/64位浮点，保留宽度/负零、拒绝非有限值和半精度/tag/非最短整数。Task字段仍有限范围，不把通用CBOR接受范围等同于任务可签范围。
- Controller与Task共用`server-grant-signer.js`；删除原Controller内部重复密钥环/算法代码，保留原接口/错误类，额外拒绝public KeyObject误配私钥。Task拒绝调用输入选key，仅用维护配置默认profile。真实Ed25519/P256签名与验证、P256 low-S及double-hash拒绝通过。
- 共享包41/41覆盖原协议、MCU固定向量、所有成功结果逐字节篡改、所有截断前缀、错根/nonce/request/role、数值/时间边界、记录进度与完整页。Gateway任务4/4，连同旧Controller/BLE/renewal软件回归共9/9测试单元通过；Broker相关26/26保持物理/virtual不协商及App Route拒绝。共享包、Gateway和Broker构建通过。MCU/Service fixture归一换行后全文一致，独立MCU Node oracle通过。
- 原Gateway测试中的PostgreSQL子门禁提示skip，未运行数据库测试；没有owner鉴权/持久scope通过证据，没有迁移/部署/新HTTP入口、App/真机或新版联网资源门禁。没有操作设备、网络或Git index。S-T1仅a子项完成，下一步严格按Service§6接b子项，不能宣告生产任务管理已闭环。

### 10.17 第十八批：Service持久scope与签发/结果提交边界（2026-09-23）

- 设备合同、212 B预算、Engine/Store/Adapter/CloudTransport均不改；Service只增加独立scope、完整请求和有期限attempt三表及窄repository。scope不是DeviceKey，不复用LocalAccess.enabled或普通Command ACK，只有真实HMAC及完整操作语义通过才确认结果。
- SERIALIZABLE短事务重新核对owner/tenant/current key/指定active session/正常route，原vault临时Key重算根且finally清零；同步真实签名后仍须COMMIT成功才返回。网络不在事务中。禁止自动重试签名、外层事务嵌套提前发布、客户端提供scope/根替代权威、分享operator管理任务及混合根误当WiFi-only。
- 原ID完整意图持久化，attempt绑定proof与nonce；丢回执不产生新ID Start，fresh Read可收敛已执行Initialize/Rebind。Key刷新需新会话显式换根；移除/转移同事务退休scope，不声称离线MCU已清任务。封闭测试上限64请求/scope、8尝试/request及15秒窗口；前两者是公开前需替换为可回收保留合同的限制，不是发布额度。
- 新增真实PG16/16子项：权限、LAN独立、challenge篡改、真实签名/HMAC、完整意图/CAS、丢回执、换根/实例、撤权/退休回滚、延迟COMMIT失败、并发与容量等。相关Gateway合计85/85无skip，共享协议41/41、Broker26/26；三个构建通过。测试端用实际测试secret生成回执，不是MCU新任务真机门禁。
- migration039仅在一次性容器隔离schema运行，未升级业务库；未改App/设备源码、硬件、网络或Git index。下一步与未闭合项仅由[Service§7](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#7-第十八批持久权威原id账本与提交边界)和总主线§6维护，不再复制新协议。

### 10.18 第十九批：物理Broker交换与连接代际（2026-09-23）

设备源码/任务wire/资源预算不变。Broker新增默认空白名单的TaskCarrier，复用原MQTT客户端与BoundedExchange；Service broker-client提供上下文及单次交换端口，不包含owner、签名/HMAC成功判断。登录proof与物理connectionId分开：同session重连不能接续旧连接的Prepare/Commit。HTTP取消、单调3秒期限及有界槽位覆盖未结束的写回调，不能让晚到回调破坏新连接。

正式Broker bs2认证、Hello、HTTP路由和实际TCP MQTT上下行通过；新传输8项/Hello3项，连同旧相关回归114/114、0 skip，Broker/Gateway构建和Device V2-only脚本通过。任务inner是传输测试正文，不是新MCU执行/HMAC证据，未运行新PostgreSQL集成或真机。没有新增持久队列、依赖、App/设备改动、部署或Git操作。

第十九批时的下一步为S-T2b；第二十批结果见下节，当前唯一优先级按总主线，不复制新的授权或任务协议。

### 10.19 第二十批：Service内部管理集成收口（2026-09-23）

设备wire/模块/212 B预算不变。Service通过共享owner/route预检查得到Broker目标，持久attempt新增物理connectionId；同proof重连不复用旧签票。Prepare→Challenge→签发→Commit→HMAC消费已用真实PG/签名与正式HTTP/MQTT串通；网络均在SQL外，逐阶段复核权威和代际。同ID并发不发送第二笔，未知结果保留原ID/意图与期限，不自动重开倒计时。

最终任务41/41（19项仓库＋20项网络子门禁与两个父项）、Broker相关114/114通过，Gateway/Broker构建通过。含签发前后取消/撤权/重连、丢回执、坏证明、延迟提交失败、存量migration040和局部锁超时。独立协议测试端不是真实C++ Engine/Flash，未刷板、清授权、更新常驻库或改App。唯一详细合同及验证范围见[Service§9](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#9-第二十批s-t2b有界管理调用与原id恢复)。

下一步D-T3只接ESP32手动WiFi可选产品：复用原模块、原Key/公钥源、同一属性目标和授时；接实际根变化/退休通知，测关闭/开启资源与普通通信共存。保持默认关闭及现有Arduino简洁API；不新增混合根/网关任务支持，账本回收与T2真机仍待完成。

### 10.20 第二十一批：D-T3a手动WiFi可选产品（2026-09-23）

#### 实际接线与边界

`Esp32WiFiTaskExtension`是现有WiFi产品的可选对象组装：借用Stack的DeviceKey/instance/RNG/clock/MQTT，持有一份Esp32TaskModule、AuthorityCache、Session/Adapter、212 B载体工作区和原TaskProductLifecycle。使用既有P256公开验签配置，没有新协议、第二个MQTT或后台执行线程。公开入口仍是`<BlinkerWiFi.h>`和原`begin/run/组件`；内部probe的allowlist/宏不是已发布用户API。

`BasicWifiCloudLifecycle`只增加选择一个carrier的attach接缝，仍控制原网络/云连接；默认extension返回原对象，没有额外实例字段。LAN extension仅适配相同窄接口。任务环境借用`enableTime()`的ITimeSync，通过coherent observation一次获取UTC/单调时刻；无服务或未实现该可选方法时fail closed，不另建时钟。属性白名单配置必须早于attach，仍调用原属性校验及回调。

内部`BLINKER_INTERNAL_WIFI_TASKS`默认未定义/0；非零仅ESP32手动WiFi试点，存储policy四项必须显式给出。与LocalAccess/LAN组合编译拒绝，未向BLE/混合/Hub扩展。probe用60秒写窗口、2次写额度、60秒最小启动和100 ms提交预算仅保留测量代码，**没有将其当作实测耐久/时延准入或生产默认值**。验签表仍为仓库build/test公开材料；真实签发须使用匹配的隔离profile，不能发布该测试信任配置。

#### 手动Key生命周期纠正

1. 该产品的ConfiguredDeviceKeySource只从Sketch配置一次；原Key虽在RAM被解码，但产品没有运行时写新Key/清Key的用户接口，也没有从任务模块再保存一份secret。
2. begin先完成原instance/Key配置，再由任务adapter加载真实根并核对已持久表。同Key、同instance重启可恢复；普通stop/reconnect只取消pending，不清任务。业务UTC尚未恢复时日历仍等待授时，倒计时重启保持Interrupted规则。
3. 换Key重烧且保留NVS，旧表进入AwaitingAuthority，需要同owner的显式签名Rebind；不自动Initialize、不擦表。host另覆盖底层Key清空后拒绝；**这不等于手动产品已经提供清Key入口**。
4. 服务器刷新Key/删除owner时，不可达设备并未获知事实。普通明文MQTT错误、登录AuthenticationRequired或掉线不能作为可信退休证据；不得为“立即清理”而允许伪造错误擦表。当前任务离线延续边界保留，不宣称旧SketchKey已被本地禁用。
5. D-T3b先补可信退休与持久拒绝/恢复的最小合同：同所有者换Key和所有权终止分开，任务停止→落盘/清理→确认顺序及掉电窗口必须可验；尚无生产退休载体。原物理resetAccess包装可复用，但手动WiFi底层并不支持清Sketch常量，不能新增一个假reset承诺。定稿后接一个实际消费者，不引入通用事件总线或新账号库。

#### 验证记录

- x64 MSVC/ASan主机回归70/70；Win32严格构建/回归69/69（不含x64的fuzz smoke），32位WifiLifecycle仍20 B。新增同实际ConfiguredDeviceKeySource恢复/换根/缺根拒绝、晚绑定同一业务时钟、白名单冻结、WiFi只注册一个carrier/启动停止/重连。最初调试/ASan DLL不在PATH导致进程无法加载，修正本次测试进程PATH后重跑全绿，未改系统安装。任务session测试验签仍有精确摘要替身，不冒充ESP32真实签名运行。
- 资源解析4/4、Arduino发布工具17/17、架构检查通过。登录冻结清单v40只审阅更新WiFiLifecycle/WiFiProduct/ESP32选择器三项；Key、HTTP登录、proof、MQTT引擎和golden未变，不删除冻结检查。
- 兼容编译：UNO R4 WiFi、core 1.5.3，原CompilePublicWiFi通过（Flash 141,728 B、静态RAM 13,120 B）；ESP32原Plain LAN候选通过（Flash 1,026,864 B、静态RAM 56,568 B）。R4日志有SDK/外部库原有warnings，非warnings-free声明；LAN只验证原组合兼容，不代表任务＋LAN组合准入。无R4/Wio任务支持或新版LAN真机证据。
- stock ESP32 Arduino 3.3.11、CLI 1.5.1、XIAO ESP32S3 default_8MB，原资源工具新增`--product`模式，复用0/4/8/10流程；不另外复制一套门禁。关闭构建不得含任务符号；开启必须链接Engine、Store、Session、Adapter、P256 Grant验证、CloudTransport和真实产品实例。新函数静态实例使用weak-object符号，解析器补支持并用回归覆盖，不再将合法实例误报为缺失。所有probe仅编译，未烧录或接入真实网络。

| 任务槽 | 整固件Flash B | 静态RAM B | 相对关闭Flash B | 相对关闭RAM B | WiFi产品实例 B |
| --- | ---: | ---: | ---: | ---: | ---: |
| 关闭 | 943,684 | 51,368 | 0 | 0 | 3,784 |
| 4 | 980,188 | 53,936 | 36,504 | 2,568 | 6,312 |
| 8 | 980,180 | 55,152 | 36,496 | 3,784 | 7,528 |
| 10 | 980,224 | 55,760 | 36,540 | 4,392 | 8,136 |

实例大小不等于整库RAM；启用增量还含allowlist/其他静态对象。表容量每加1条，当前对象/静态RAM增加304 B；Flash的几十字节差异来自模板/链接布局，不能说8槽比4槽算法更节省。无任务符号只证明关闭不保留任务模块，并非对历史其他probe的绝对字节零增量承诺。这里是首次完整产品组合的新基线，不能与§10.14的旧源module-only基线直接相减归因。

复现：`tests/arduino/task_resource_gate.py --product --arduino-cli <cli> --config-file <stock-3.3.11-yaml> --build-root <本库build下新目录>`。本批报告为`build/task-wifi-product-matrix-20260923/resource-report.json`，各档保存ELF/bin、SHA-256、sdkconfig、build options、symbol表和编译日志，编译前后源码digest一致。普通R4和LAN工件在`build/task-wifi-product-20260923/{r4,lan}`。工件不是发布包，不含现场Key/WiFi密码；堆/栈运行峰值、NVS时延/磨损、真实签发及掉电退休恢复仍未测，不能从这些数字推定生产容量。

唯一下一步：D-T3b上述退休合同与专用COM3真实签发/普通控制＋授时＋NVS提交共存；先确认测试板身份、原固件/授权备份和当前网络。维护policy须依据测量准入，缺条件时保留待测；不回头重跑无关C3、长循环或扩大SDK维护。然后才是账本回收与T2公开入口/安装执行查询；当前D-T3/T2不可标完成。

### 10.21 第二十二批：D-T3b-1持久退休标记与最小恢复边界（2026-09-23）

#### 从状态事实出发，不复制框架

这里“退休”指**永久停用当前任务scope**，不是断网、授时暂不可用、撤销某个分享用户或轮换同owner的DeviceKey。RAM中的`retire()`只保证当前进程不再执行；直接删除blob会使重启后的“已退休”与“未初始化”不可区分。因此在原TaskSnapshot/Store内保留终态，不另建凭据表、NVS key、常驻outbox或任务线程。

| 事件 | 本地任务处理 | 当前范围 |
| --- | --- | --- |
| 原Key、原instance普通重启 | 独立读根后恢复同scope；日历待授时、倒计时保持Interrupted规则 | 既有实现 |
| 同owner换Key且保留NVS | AwaitingAuthority；鲜活签名Rebind精确换根，不丢任务定义/去重 | 既有内部管理链路；真实换Key共存待测 |
| 收到可信的当前scope终止 | 先停执行→原子终态→确认已知持久结果 | 本批仅内部Engine接缝，无线上消费者 |
| 登录失败、MQTT错误、掉线 | 不当作可信退休，不擦任务/密钥 | 保持原规则 |
| 用户明确物理重置/清除 | `retire→clearRetired→底层授权清理`；每步确认成功 | 保留原可信维护消费者；手动WiFi不能擦掉Sketch常量 |
| 云端已删owner，但设备不可达 | 云端拒绝旧访问；设备持久退休仍为pending/unknown | 不承诺无法送达的“立即本地停任务” |

#### 存储与核心接缝：已实现

1. 活动任务仍写原**schema 2**，编码/golden不变。**schema 3仅用于退休标记**：同128 B表头＋4 B CRC，无记录；保留instance/scope/root、tableRevision和nextTaskId，最近请求ID/摘要改为退休意图。总计132 B，未扩编码工作区。旧schema 2固件拒绝未知版本，而不是把标记读成空表恢复；不自动迁移/格式化。CRC依旧不是防恶意回滚机制。
2. `retireVerified(expected,requestId)`只供调用者已经验证新鲜授权的生命周期使用。精确核对存储binding、非零16 B ID，先进入Retired，再一次原子replace；本批没有任何网络接收路径直接调用它。重复同binding/ID且摘要一致只返回已知结果，不再写；不同ID不覆盖既有终态。普通编辑额度耗尽不能阻止一次可信终止，失败则锁定Store，成功则未来只读，因此不形成无限免额度写入入口。
3. 本地幂等摘要为SHA-256(CBOR `["BTR1", bstr(instance16), bstr(scope16), bstr(root32), bstr(requestId16)]`)；它只是本地意图指纹，**不是已经冻结的网络签名/回执协议**，也不授予调用权限。`retirementRecord()`仅暴露已验证且已知持久的内部元数据；不能把它直接作为无认证网络响应。
4. 成功后清空RAM任务记录/运行槽；重启加载schema 3立即Retired，早于scope恢复、任务归一化和动作执行。同旧Key或新Key不能自动Initialize/Rebind。普通管理在退休后全部拒绝，退休前已准备/签发的Read、Mutation、Rebind也不能复活任务。
5. 写入返回失败时，RAM已经停用，但不得声称标记落盘；不读回dirty handle猜测成功，不立即重试或先删旧表。新运行时正常load后，若实际提交了标记可恢复已知终态；若写前失败保留旧表，则仍是旧表，须重新完成可信退休。离线旧表存在时原离线恢复政策没有凭空改变，不能冒充失败后重启也必然停用。
6. 物理`clearRetired()`仍有reset/维护消费者，并非废代码；不自动用于在线退休。擦除全部Flash、恢复旧快照、重新烧录主动修改的固件，不在当前软件标记的抗回滚保证内。未修改原WiFi/Access命名空间或保存raw secret。

#### 第二十二批时的下一步D-T3b-2（合同/设备与Service编排现分别见§10.22/§10.23）

只扩展当前可选TaskManagement模块，复用已有Server签名、公钥源、单pending/单调TTL、原MQTT carrier及结果证明；不新增账号系统、通用事件总线或第二条MQTT。先完成以下事实约束，再分配命令字段和接线：

1. **来源与目标**：Service持久意图区分同owner换根与所有权终止；退休只能命中确切instance/scope/root及原requestId，不接受App自报根、MAC或普通错误作为依据。目标由已验证历史绑定与当前连接身份共同确认；签名须绑定设备新挑战、操作和意图，不能重放历史管理Grant当退休票。
2. **退休与连接撤权的顺序**：现有普通execute要求active owner/scope，Broker也会拒绝已撤Key/session，因此“先删所有凭据，再复用普通execute清任务”不可行。不得放宽普通权限检查或保留旧Key控制权。需明确受控退休交付窗口/最小恢复通道与数据库意图的事务边界；失联时云撤权不等待无限网络，标记本地退休待确认，不谎报完成。此项要先审查现有删除/Key刷新消费者，只实现一个手动WiFi路径。
3. **确认和丢回执**：网络成功只能来自设备已知持久标记的、绑定新鲜challenge的已认证结果；失败、超时不证明未提交。不重复创建退休ID。新会话查询/原意图重试应能辨别已完成；已删除旧Key后如何验证结果必须与上一项一起解决，不能继续调用只认当前Key的普通结果验签器假装有效。
4. **重新使用**：终态不是永久报废设备，也不是自动空表。物理清除可另行授权；远程新owner恢复必须另有新授权、确切终态CAS和新scope，不得借Rebind跨owner。首版只选择能验证完整的恢复方式，未实现时明确要求维护重置；不预先承诺任意离线迁移。
5. **验证**：独立跨端向量、过期/重放/错scope/旧连接拒绝、停止先于写入、写前/后失败、丢结果后新鲜查询、旧Key/新Key重启及当前Key轮换的非退休对照。Service/Broker权限仍保持默认关闭和维护白名单。合同/软件通过后，才在授权COM3做真实签发＋普通控制/授时/NVS共存，记录堆栈、写时延与恢复证据；没有硬件证据就保留待测。

上述事项完成后再收口账本保留/回收与T2用户安装执行查询；不扩BLE/混合/网关任务或A1。现有wire操作号、Grant和公开Arduino API本批均未改变，**D-T3b-1不等于D-T3/T2完成**。

#### 本批验证

- x64 MSVC ASan全量**70/70**、Win32严格全量**69/69**通过；新增终态独立golden、逐字节破坏/截断拒绝、写前/写后失败、精确重试、编辑额度耗尽、重入、重启及退休前管理上下文拒绝。换根用例独立加载真正不同的新根并断言StateConflict，避免被同根输入校验先挡住造成假覆盖。Session签名provider仍是精确摘要替身，未冒充本批真实网络验签。
- 实际Esp32TaskModule/端口使用host假NVS覆盖终态commit成功、commit前失败、commit成功但返回失败，以及WiFi/Access命名空间见证；Product加载终态后普通transport生命周期与属性report继续工作，stop/start不再写任务或清授权。**这不是实际MQTT控制、NVS时延或真机断电证据**。Engine 32位ABI由184到188 B、header由120到124 B，slot仍48 B；4/8/10条Engine/Store/表/槽/工作区合计1,552/2,768/3,376 B，module实例1,632/2,848/3,456 B（后者受对象对齐影响未增加）。不开任务不分配这些对象。
- 架构检查、发布工具17/17、资源解析4/4通过。资源probe复用原0/4/8/10门禁，module模式额外要求实际链接退休/终态读取，product模式不冒称已有在线退休消费者。stock ESP32 Arduino 3.3.11、CLI 1.5.1、XIAO ESP32S3 default_8MB四档编译/符号检查通过，编译前后源码摘要一致。§10.20产品表是上一批源码历史基线，不能与本批module-only图直接相减归因。
- 本批不刷板、不清授权、不改App/Service业务源码、数据库或网络，不暂存/提交/推送；只同步设备及Service实施文档。清理TaskManagementGrant中过时的“尚无BBP kind”注释，保留仍被使用的物理清除路径。

| 任务槽 | module-only整固件Flash B | 静态RAM B | 相对关闭Flash B | 相对关闭RAM B | Module实例 B |
| --- | ---: | ---: | ---: | ---: | ---: |
| 关闭 | 941,388 | 51,304 | 0 | 0 | 0 |
| 4 | 959,548 | 53,016 | 18,160 | 1,712 | 1,632 |
| 8 | 959,552 | 54,232 | 18,164 | 2,928 | 2,848 |
| 10 | 959,552 | 54,840 | 18,164 | 3,536 | 3,456 |

该图保留Engine、Store、同一WiFi/time基础产品、属性目标、生命周期及内部退休路径，**不包括完整Session/Grant/carrier在线任务产品**。关闭档无任务符号；逐槽静态RAM仍增加304 B。报告`build/task-retirement-core-matrix-20260923/resource-report.json`与各档ELF/bin、SHA-256、sdkconfig、symbols、compile.log留在本地build；复用`tests/arduino/task_resource_gate.py`（本批不传`--product`）。不是SDK版本准入扩张，也不是运行堆栈或真实NVS磨损/时延结论。

### 10.22 第二十三批：D-T3b-2a签名退休与新鲜终态查询（2026-09-23）

#### 首版的投递和恢复选择

审查现有Service `removal.js`、任务repository和migration039后确认：删除事务会撤Key/session并退休云端scope，普通execute与Broker都会拒绝继续管理。因此不能“删除后再复用普通execute补发”，也不为后台清理保留旧Key控制权。

首版选择**同一有效owner/当前Key/session窗口中，有界退休尝试→原云撤权**。Service须先记录唯一退休意图并冻结同scope普通管理；网络等待不持有SQL事务。设备离线、超时、回执未验真或崩溃恢复无法取得新鲜证据时，云撤权仍需继续，本地状态只记Unknown，不声称MCU已经停止。云端`scope.retired`仅表示服务拒绝旧scope，不等于MCU已持久退休。

此批尚未实现上述Service编排。后续只补现有repository/删除消费者，不新增撤权后登录、特殊MQTT、持久旧secret或通用恢复服务。当前Key撤销后不能继续查询旧设备；同ownerKey刷新仍走Rebind。新owner首版须显式物理/维护重置任务与相应授权，再用新授权初始化新scope；不自动清旧表、不凭新Key跨owner换根。远程新owner恢复后置。

#### 精确合同与设备接线：已实现

```text
Retire=7 / RetirementRead=8 Prepare:
  [1, bstr(requestId16), bstr(instance16), bstr(scope16), bstr(root32)]
operationDigest = SHA256(CBOR [domain, exactPrepareArray])
domain = "blinker.task-retire.v1" 或 "blinker.task-retirement-read.v1"
成功结果沿原八字段/HMAC封套：resultKind=4 (Retired), count=0,
  detail=bstr(originalRetirementId16)
```

- requestId、instance、scope、root均固定长度非零；严格canonical、完整消费。摘要第二项是原数组，不是bstr或仅其散列。Grant仍是原签名封套，绑定设备新nonce、instance/scope、请求及完整操作摘要；内部BTR1持久幂等摘要不变，不能代替网络签名。
- 沿原Prepare→Challenge→签发→Commit→认证Result，复用单pending、冷却、单调TTL、来源/连接代际及根变化取消。`TaskCommand`不再复制一组数字，直接沿用唯一wire枚举。Retire/RetirementRead仍是请求，不能作为上行响应混入。
- Session从Engine已验证binding/终态和独立当前根两侧核对完全相等，Grant通过且新鲜才调用`retireVerified`。已知同ID终态重试不再写；不同ID不得替换原退休意图。Query只读已知持久标记，采用独立query requestId，回执同时返回原退休ID；查询失败不是“任务仍活动/未提交”的认证证据。
- 退休后清可执行scope，不销毁独立根缓存，以便同Key重启后的鲜活查询；普通Read/Mutation/Initialize/Rebind仍拒绝，查询不能复活任务。根更换/清除、旧连接、过期、坏签名或Store dirty均拒绝；不靠失败后的立即read-back制造落盘证明。
- `task-contract.js`与设备codec共享固定向量；Service纯绑定函数重算整个操作并限制Retired结果语义。Retire结果ID必须等于原操作ID；Query的原退休ID由下一批repository与持久意图再核对。**纯合同不提供owner授权；现有execute/数据库operation约束仍只允许0..2，当前用户尚不能通过Service发起退休。**
- Broker及Gateway传输helper接纳7/8，仅沿原维护白名单、physical/current session/Key/version/Hello/topic及内部token/shard边界，不加App/虚拟设备Route权限。成功传输不代表已验任务证明。

| 消息 | Inner B | 含原carrier与10 B BBP头 B |
| --- | ---: | ---: |
| 退休/终态查询Prepare | 87 | 102 |
| Challenge | 138 | 153 |
| 最大Grant | 176 | 191 |
| 最大Retired结果（u32最大revision） | 78 | 93 |

上表不含MQTT/topic/TCP/IP/TLS；212 B保守总上限、197 B inner缓冲和普通查询页预算不变。仅给临时结果增加16 B原退休ID，不增加常驻pending或Flash命名空间；堆栈运行峰值仍待真机测量。

#### 验证与剩余

- 独立golden扩展退休/查询摘要和两种HMAC角色的终态结果；共享TypeScript全量41/41、Gateway真实Ed25519/P256及操作绑定5/5通过。固定向量不是现场授权记录；设备Session测试的验签provider仍是摘要替身，不能把独立真实密码学与host Flash组合成MCU已验签的结论。
- Broker载体10/10，含两组真实HTTP→正式Broker路由→MQTT双向交换（旧0/3、新7/8）、Failure/取消/撤权/错方向拒绝。用测试专有SQLite和本机端口，无现场服务变更。修正测试中同一时钟重复mock造成的残留，动态测试端口通过原内存shard registry注册/还原，不改生产超时或网络配置；Node22匹配已有SQLite ABI，无重装依赖。
- 设备增加错instance/scope/root、查询签票冒充退休拒绝、过期/断连/根变化/坏签名/提交失败、同ID不再写、重启查询、新Key拒绝，以及真实Runtime→CloudTransport→MqttFrameTransport组合中的丢结果/重连Hello/新鲜查询。底层socket、Flash和验签替身边界明确；没有刷板或清授权。
- x64 MSVC/ASan全量70/70（23.76 s）、Win32 Release严格构建/全量69/69（32.11 s）通过。Win32 cache=76 B、Session=336 B、adapter=64 B、carrier=96 B＋212 B工作区，未增加常驻实例大小。架构检查、资源解析4/4、Arduino发布工具17/17、独立Node真实签名/摘要/HMAC oracle通过；设备/Service fixture文件SHA-256逐字节一致。
- 本批无App修改、数据库迁移、owner删除消费者接线、服务部署或Git暂存/提交/推送；尚未完成D-T3/T2。

stock ESP32 Arduino 3.3.11、CLI 1.5.1、XIAO ESP32S3 default_8MB，复用`tests/arduino/task_resource_gate.py --product`完成当前源码0/4/8/10整固件编译及符号门禁。关闭档无任务符号；开启档实际链接Engine退休/终态读取、Session/Grant/carrier与原WiFi产品，不仅是module-only探针。

| 任务槽 | 整固件Flash B | 静态RAM B | 相对本批关闭Flash B | 相对本批关闭RAM B | WiFi产品实例 B |
| --- | ---: | ---: | ---: | ---: | ---: |
| 关闭 | 943,680 | 51,368 | 0 | 0 | 3,784 |
| 4 | 983,012 | 53,936 | 39,332 | 2,568 | 6,312 |
| 8 | 983,000 | 55,152 | 39,320 | 3,784 | 7,528 |
| 10 | 983,044 | 55,760 | 39,364 | 4,392 | 8,136 |

相对§10.20相同产品各档，开启档Flash增加约2.8 KB，静态RAM/产品实例大小未增加；关闭档Flash的4 B差异不作算法收益解读。记录槽仍每条304 B静态增量，包含表/运行槽/编码区，不是只有48 B运行槽。当前结果不与上一批module-only图混算，也不等于动态堆/栈峰值或NVS时延、寿命准入。

本地证据：`build/task-retirement-product-matrix-20260923/resource-report.json`及各档ELF/bin/sdkconfig/symbols/compile.log；编译前后源码摘要一致：`b957b3a832933b0a233a86539f7e655f1bd51218897b8e4af3e27a4141f44b0e`。未生成对外发布包，未使用现场DeviceKey/WiFi密码或烧录探针。

**第二十三批时的下一步为D-T3b-2b，已由下节推进；当前唯一下一步按总主线§6执行。** 不再扩恢复协议、BLE/网关任务或公开UI。账本公开前保留回收与T2安装/退出/执行/查询仍保留。

### 10.23 第二十四批：D-T3b-2b持久退休与owner删除消费者（2026-09-23）

设备源码、132 B终态快照及212 B载体不变；本批接Service真正消费者，不再重复底层引擎或协议。原三表通过migration041增加冻结/设备已验终态事实与操作7/8；原退休ID、完整Prepare、challenge/回执继续在原request/attempt持久化。退休时冻结普通管理，Query核对原退休ID，不能仅凭HMAC正确就确认另一笔退休。

原owner删除入口在SQL外做一次有界尝试，再走原云端撤权；网络失败、丢回执和未知不能阻止撤权，也不能写成设备已停。`localTasks`与原`localAccess`分开，只有持久认证结果才显示`retired_verified`；普通无任务scope不附加字段。仍只有当前owner/Key/session窗口可精确重试/查询，撤权后无例外；同owner正常换Key保留显式Rebind，已retiring不能通过换根复活。

实施细节、默认关闭的双端维护配置、64＋2/8封闭容量及测试清单只维护在[Service§11](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#11-第二十四批d-t3b-2b持久退休意图与删除前编排)。本批三包构建与177/177相关回归通过，包含真实隔离PG升级/事务与正式HTTP/MQTT、实际签名/独立HMAC回执；不等于MCU验签/Flash写入证明。没有升级常驻库、改App/设备源码、刷板/清授权或Git操作；上批产品资源不重测也不冒称本批动态共存结果。

**第二十四批时的下一步D-T3b-3，实施结果见下节。** 只在专用COM3的4槽WiFi产品中测真实管理、普通属性/Action/授时、NVS与重连、退休/Query/重启不复活，COM11/COM12不动。当前后续按总主线§6，不将历史步骤重复作为下一批任务。

### 10.24 第二十五批：D-T3b-3真实WiFi产品与NVS共存（2026-09-23）

#### 范围与工具边界

只操作专用COM3 XIAO ESP32S3（USB身份`8C:BF:EA:8E:F1:2C`）；COM11/COM12未操作。使用本轮用户指定mostfun2026，设备实际联网，PC地址`192.168.1.146`。ADB未在线，因此这是**真实Service/Broker/MCU封闭内部链路，不是App/真实账号/页面验收**。未部署现有服务、迁移常驻业务库或重写任务协议/生产composition。

- 新增`tests/arduino/Esp32WifiTaskLiveGate`与小型串口driver。沿原4槽产品、显式属性白名单、`enableTime`、DeviceKey登录及单一MQTT连接；真实NVS函数的linker wrapper仅计数/计时，不替代存储。`r`只软件重启，`m`只清测量计数；不开机清授权、不自动擦表。
- Service内部工具生成ignored目录中的私有配置，使用一次性PG隔离schema、真实repository/签名/session HTTP/Broker。复用现有build/test P256材料，不是release key。管理指令只从本机stdin进入，没有新增公开任务HTTP入口、用户token或App伪登录。
- 提取原`task-mqtt-peer`中的Broker部署为共享test helper，删除重复组装；模拟MCU oracle与真机runner各自独立消费它。原软件门禁也改走真实Broker注册HTTP而不是直接插SQLite session。
- 工具的60秒/16次写入、最小Start 1秒、最大commit 1秒仅供本轮有限测量，不是生产寿命策略；产品仍要求显式policy且默认关闭。

#### 实际闭环与观测

| 门禁 | 结果与边界 |
| --- | --- |
| 登录/基础通信 | 实际DeviceKey challenge/session、MQTT订阅/Hello、3字段Manifest、属性和button Action ACK、State与UTC通过；管理沿同一物理MQTT |
| 安装/执行 | Initialize→Create→Start，3秒倒计时实际回调；power从false变true，认证Read为Done、startSequence=1；不是仅看服务端verified |
| UTC持久恢复 | 另建UtcOnce，软件重启后重新登录/授时；到期回调1次、State变true，认证查询的consumedUtcMillis等于原目标时间；已完成倒计时未重执行 |
| 退休/丢结果 | 将倒计时改为60秒并Start；真实Commit结果到达工具后丢弃一次，Service为TASK_OUTCOME_UNKNOWN，随后普通管理为TASK_SCOPE_RETIRING |
| 重启/原ID恢复 | 重启后任务不复活；沿原持久退休ID、新challenge确认Retired，独立Query返回相同原ID；两次证明期间task snapshot新增写入为0 |
| 到期后/普通控制 | 经过原60秒期限，自动属性回调仍0；button仍能控制，随后普通power写入产生1次回调与fresh State；退休只停本scope任务 |
| 删除/拒绝 | 原management.remove返回cloudAuthorization=revoked、localTasks=retired_verified；后续管理为TASK_FORBIDDEN。未运行清理worker，brokerCleanupPending=true，**不称旧Broker socket已被强制撤销** |

两次软件重启均出现短暂`TASK_CARRIER_OFFLINE`：重启前TCP未正常FIN，新session已在线但旧socket尚未keepalive超时，物理候选不唯一，管理按原合同拒绝；旧socket退出后恢复。普通控制/UTC仍可工作。此错误名包含“不唯一”而不只“真实离线”，不能用它断言设备没联网；本批未改成挑最新连接或放宽权限。该边界需在T2提示/有界重试中保留，不自动重放Start。

#### 有限资源测量，不能替代生产准入

stock Arduino ESP32 **3.3.11**、本机CLI **1.5.2-rc.1**、`XIAO_ESP32S3/default_8MB`、4槽、3组件、测量wrapper。完整链接Flash **988,604 B**，静态RAM **54,000 B**；测试固件bin为988,752 B。与其他profile/组件数的探针不能直接相减归因。

| 测项 | 本批观测 |
| --- | --- |
| `nvs_set_blob(snapshot)`最大调用耗时 | **46.305 ms**；既有样本还出现41.112 ms，不把GC停顿隐藏为commit耗时 |
| `nvs_commit`最大调用耗时 | **13 µs**；不是完整snapshot持久化耗时 |
| `Blinker.run()`最大调用耗时 | **298.324 ms**；包含协议、验签、存储等，不等于纯Flash时间，也不是整个loop间隔/网络最坏延迟 |
| 堆 | 该完整流程启动以来最小空闲 **246,724 B**；各采样时约254.7～255.0 KB空闲 |
| loop task high-water | 最低读数 **4,176**；仅该任务，不证明ipc0/NimBLE等SDK任务栈准入 |
| 普通控制ACK | 本次7笔已确认控制约 **64～302 ms**；串行交错验证，未做并发洪泛或延迟SLA |
| 退休重启/查询写入 | 重启后的snapshot计数持续 **0**；all-NVS commit=1来自普通启动路径，不能混作任务重复写入 |

退休加载后`taskError=18(StateConflict)`表示该scope不可重新启动任务；WiFi产品仍begun、timeValid，Query可以验证终态，不能把这个状态解释为整台设备故障。当前测量不足以宣称NVS寿命、GC最坏耗时、长期栈/堆稳定或所有开发板准入；没有新物理断电测试，不将前一批隔离断电证据混入此表。

#### 失败记录、验证及复现资料

- 初版工具漏传Broker HTTP `requestTimeoutMs`，默认计时约1 ms导致session注册超时；已在共享test helper显式传4000 ms，并由真实注册路径回归覆盖。没有直接激活数据库行或绕过DeviceKey登录。
- 本次运行的隔离logical ID为裸UUID，造成App presence fanout格式告警；任务/直接管理控制链路不经过App路由，以上真实回执与NVS证据保留，但**不验收App presence/账户路由**。工具源码已改成标准`device_<uuid>`并增加共享codec的启动断言；此次完整运行不冒称已经用修订ID重跑。
- 一次测试指令误用Create携带非零taskId，严格codec在发送前拒绝；改为已有Replace语义后通过，没有为了工具输入放宽协议。所有失败保留在原始记录。
- 最终Service任务密码学/仓库/网络组合回归 **61/61、0 skip**；此前单独32/32包含于该组合，不叠加成93。Node22匹配本机SQLite ABI；新工具语法、串口driver编译检查及库/Service diff检查通过。
- 本地证据根：`build/task-product-live-20260923`。`live5/service-events.jsonl`保存原请求意图与认证结果，`serial/serial-*.json`只保存结构化指标；`task-regression.log`为61项结果。`live*`早期配置与完整Flash包含私有凭据，不进入Git/发布ZIP。运行方法见`tests/hardware/README.md`。
- 新鲜8MB原板备份SHA-256：`eb8ddcf8d8eb052f9c6c95424a5e94f586ff8661a7c143fa1c83ef95c84a1ae9`。试验前已ROM全量校验；退出时整盘恢复并做独立全8MB校验，复位后的原固件联网状态另行核实，不由恢复校验推断线上ready。
- 测试bin SHA-256：`4312deffc629262359771071b9552c898f0fbfa1bd3b6b2b51ee88954d5637ae`；最终服务事件SHA-256：`ad8403554d764fd25ac9b7056b93841c9aadcfe68b56902025deb6f96e3769b5`。源码没有因观测而修改生产状态机。
- 恢复后独立`verify-flash`确认全8,388,608 B digest一致并硬复位；`restore-verify.log` SHA-256为`ea6d463df5ccaf61c0390cf3d0820ee22ef10be1becf6a0061c3c2d4218c1955`。三块USB身份重新核对通过；没有把原固件在新网络的云端ready列为已测。stdin runner、专用临时数据库容器及17002/18887/63352监听已关闭，原常驻服务未动，私有备份仍本地保留。无Git暂存/提交/推送。

第二十五批安排的账本/policy前置已由下节完成软件收口，当前唯一下一步按总主线§6接T2。这里的有限测量不因新policy而改写，也不升级为长期可靠性/全平台证据。

### 10.25 第二十六批：T2试点写入policy与账本前置（2026-09-23）

#### 职责和清理

设备生产`src`、wire、TaskEngine及普通Arduino API不变。删除内部真机fixture重复的宽松16次/60秒、Start间隔1秒、commit阈值1秒配置，改为`tests/arduino/Esp32WifiTaskLiveGate/TaskPilotPolicy.h`，host Engine测试直接包含该文件；Service生成工具复制同一头文件到私有sketch。没有在产品头中偷偷增加默认值，也不覆盖上一批已保存的实验制品。

| 显式策略 | 封闭T2值 | 精确含义 |
| --- | --- | --- |
| window / credits | 60秒 / 10 | token bucket容量10，每6秒补1次写入；不是任意滚动60秒最多10写 |
| 冷启动 | 空桶 | 不因重启补满；最早6秒才有一次Initialize额度 |
| minimumStart | 30秒 | 全局Start冷却，含冷启动等待；不是倒计时定义必须≥30秒 |
| maximumCommit | 100ms | 完成写调用后发现超限则Fault并停止后续执行；不是可抢占Flash的超时器 |

四个任务同刻到期需要8次持久写入（Claim＋完成），只有已补足额度且未被管理修改消耗时才准入这一突发。余下2次只是满桶时的余量，不是为执行另设保留池。管理洪泛仍可能耗尽额度、超过迟到窗口后Skip；不得因此补跑/自动重启倒计时。配置阶段应分开操作并等待明确结果；这是一套有限实验负载约束，不是面向初学者的最终API默认或可无限编辑的产品策略。

100ms参考上一批有限样本max46.305ms留余量，**不意味着SDK/NVS最大耗时已被证明**；host用47ms模拟正常提交、101ms模拟超限。Flash可能已成功持久化但返回Fault，客户端仍须按未知结果/原ID恢复。30秒冷却只抑制Start洪泛，不代替Flash寿命测量。2写/60秒的compile-only资源probe仍是独立测量配置，不能当作本次可执行四槽试点策略。

#### Service账本合同

精确规则只维护在[Service§13](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#13-第二十六批有界历史回收与t2试点准入)：Read的最后attempt期限后60秒可回收；verified Mutation在24小时后先提升持久`mutation_replay_floor`再回收；unknown写入/权威意图/退休证明保留。仍用原三表、64/8上限和短事务，不新增队列/worker。旧Start无论换不换ID，只要仍带旧CAS均在发送前拒绝；UI不能自动替它改CAS或换ID。门槛不是MCU当前版本，必须新鲜Read确认状态。

#### 软件验证与未测范围

- Win32严格全量重新构建，**69/69**通过，包括同一policy的空桶/5999→6000ms边界、30秒Start、精确重试不写、重启不送额度、四任务同步到期8次写及超预算Fault不执行；47ms模拟提交下四任务完成，非物理时延证据。
- x64 ASan重建后的TaskEngine目标**1/1**通过。首次启动缺MSVC ASan运行时DLL（0xc0000135），仅给测试进程补正确工具链PATH后通过；未修改系统配置或代码绕过检查。
- Gateway任务组合**71/71、0 skip**，含迁移/回收/原CAS拒绝/未知写保留/COMMIT回滚，以及正式HTTP/MQTT链路阻止回收后的旧Start。Gateway TypeScript检查通过；独立回执端不是真实MCU。
- 未刷写开发板、清授权、迁移现有业务库或修改App；未暂存/提交/推送。新policy整固件编译及真机安装/执行准入留在同一T2闭环，不能复用上一批宽松policy的硬件数据宣称已通过。

本批结束时的下一步为owner HTTP和原App任务页面；HTTP/客户端已由下一批推进，当前优先级统一见总主线§6。保留默认关闭、规范设备ID、备份/恢复及见证板边界，不扩其他平台、混合根、BLE/网关任务、自动化或服务器全局清理。

### 10.26 第二十七批：T2a管理入口与页面接线边界（2026-09-23）

复用唯一管理服务增加类型化owner HTTP；App将原ID/完整意图保存在注入的账号隔离存储中，未知结果只恢复原请求。严格区分新鲜认证结果、历史确认和真实执行状态；查询不隐式清理未知写。实现和测试统一见[Service§14](../../aily-blinker-services/docs/SERVICE_DEVICE_V2_TASK_MANAGEMENT_IMPLEMENTATION_PLAN.md#14-第二十七批t2a-owner-http与app原意图恢复底座)及[App定时接线说明](../../blinker-app/src/app/sections/device/device-timer/readme.md)，不在三端再各写一套wire合同。

页面前置缺口：`TaskPropertyTarget`的开发者白名单尚无对外枚举，普通Manifest的writable只证明普通控制权限，不能当作允许持久调度。下一批先在既有有界能力/管理通道中补真实支持、key/type及容量，维持212 B预算/分页，再接原页面。不能为抢接UI而接受任意JSON动作、自动转换旧loop或由服务器复制一份目标表。

本批设备生产src/wire没有变化，不重复声明host/Arduino/真机门禁。Service任务77/77、共享协议42/42、App相关Angular50/50及完整spec类型检查已过；其中独立设备回执端不是真MCU。原TimerService尚未替换、页面未启用，新policy/COM3用户闭环仍待T2c。HTTP默认关闭，不部署现场业务库、不刷板、不清授权或操作Git index。

### 10.27 第二十八批：T2b-1真实目标目录与同一Read通道（2026-09-23）

#### 为什么是目录，而不是第二套Manifest

普通可写属性不是调度许可。目录只描述`TaskPropertyTarget`的显式绑定名单及实际任务槽容量，不从页面控件、Service配置或普通Manifest推断。`ITaskTarget::describe`为可选能力，默认Unsupported；普通控制/旧目标适配器不强制实现。当前白名单要求真实注册、可写、非遥测的标量Property及实际回调，重复key/无效绑定拒绝；查询不调用用户回调，也不触发任务。

目录仅含key/type；名称、范围、步进等继续用已验证Manifest，MCU在实际任务校验/执行时仍检查原约束。它不是一份可转授的权限凭据。当前试点只实现WiFi/DeviceKey管理；Admin的codec向量不等于BLE/混合产品已接线。

#### 冻结wire：原Read操作2增加严格可选选择器

| 请求/结果 | exact inner CBOR / detail | 边界 |
| --- | --- | --- |
| 原任务表Read | `[1,requestId16,expectedRevision,index]` | 原4项字节不变；metadata的index=UINT32_MAX |
| 目标目录metadata | `[1,requestId16,0,UINT32_MAX,h'']` | 第5项存在才是目录，不与表查询混用 |
| 目标目录page | `[1,requestId16,0,index,fingerprint32]` | index 0..31；fingerprint为上次metadata的非零32 B值 |
| 认证结果kind=5 Catalog | `[capacity,total,fingerprint32]` | capacity=1..10，total=0..32 |
| 认证结果kind=6 Target | `[capacity,total,fingerprint32,index,key,valueType]` | key≤64 B；type=Boolean/Int/UInt/Float32/Float64 |

结果仍使用原完整认证header、HMAC和challenge绑定。header的tableRevision/count属于**任务表**，不是目录修订/目标数；不能要求读取目录期间任务表完全不变。Read摘要仍为`blinker.task-read.v1`域，但对完整4项或5项正文分别摘要；Service核对结果kind、index和fingerprint，普通表Metadata不能冒充目录。Operation编号、carrier0x37、feature bit25、Grant和Broker路由均不新增。

目录fingerprint是流式canonical CBOR SHA-256：`["blinker.task-targets.v1",capacity,[[key,type],...]]`，按固件绑定顺序，不构造完整scratch。页提交时重新计算并比较；配置/回调变化后旧hash拒绝，App不混接新旧页。该hash不覆盖属性范围等Manifest字段，页面仍需使用对应已验证Manifest。旧固件拒绝5项Read；试点没有单独目录feature位，失败只能显示不可用，不能当空名单或退回所有可写属性。

**认证没有因“只读”旁路。** metadata和page均走原Prepare→Challenge→Grant→Commit→认证Result，要求已初始化且Current的任务authority。首次启用需用户显式Initialize；`DeviceTaskClient.catalog()`绝不自动初始化。元数据/每页各占一次原只读请求，按§10.25/Service§13回收。32目标＋1目录metadata＋最多10任务/1表metadata可放在64条空闲账本内；已有未知写占用仍可能满额，明确返回额度错误，不为目录扩账本或清历史。

#### 模块收敛与有界资源

- 目录仍由Engine/现有target接口提供；`TaskPropertyBinding`复用原函数指针作无副作用的可用性检查，不给每个Property加标志、全局注册器或额外指针。常量descriptor不复制成运行时数组。
- Session复用原单笔请求空闲定义区保存32 B选择器，绝不持有网络RX指针；Win32 pending Session仍336 B。新增owned结果为临时读回数据：`TaskReadPrepare`60 B、`TaskManagementResult`288 B（Win32 ABI），不是每任务常驻增长，也不是实测栈峰值。
- 流式hash、分页和现有buffer完成查询，无堆分配、Flash提交、新线程或新凭据；最大认证目录metadata完整BBP帧114 B，最大Target帧183 B（64 B key/index31/32目标/10槽），小于既有212 B上限。Read预留上界59 B，实际最大编码56 B。
- Service仅扩原codec/HTTP投影/结果绑定；账本、迁移、签名器及Broker交换复用。App合并只读请求辅助函数，目录查询不覆盖未决写；对页数/类型/重复key/指纹一致性、账号sessionEpoch/取消严格检查，不新增缓存、后台刷新或自动重试。

#### 软件证据与剩余门禁

- 最终Win32全量69/69及架构检查通过；开发中任务子集8/8不重复累计。独立Node最小CBOR oracle生成C++与Service共用的9项目录向量，覆盖摘要和DeviceKey/Admin认证结果，生产codec逐字节匹配。拒绝截断、篡改、错误选择器、未授权、变更目录，且回调/Flash计数不增加。
- Service任务79/79、0 skip；共享协议44/44，Gateway/共享包构建、生产路由接线1/1通过。真实隔离PG＋HTTP＋Broker/MQTT＋独立设备oracle覆盖目录metadata/分页、错hash拒绝，并保留原Start执行计数1；不是MCU或真实Auth/App账号证据。
- App完整spec TypeScript检查、Angular相关52/52（任务12、管理9、拦截器31）。目录不清unknown写；历史结果/重复目标/混合指纹/取消拒绝。测试存储仍为注入MapStorage，不代表真实App重启持久化已过。
- ESP32整固件四档编译/链接4/4通过，资源表如下；本批不刷板、不清授权、不部署业务服务/迁移数据库、不暂存或推送。一次性PG临时盘已停止并自动移除，仅测试数据被丢弃，既有Docker数据未动。

stock Arduino-ESP32 3.3.11、XIAO ESP32S3/default_8MB，同一真实手动WiFi产品图（无刷写）：

| 任务容量 | Flash B | 静态RAM B | product实例 B | 相对本轮0槽：Flash/RAM B |
| --- | ---: | ---: | ---: | ---: |
| 0 | 943,688 | 51,368 | 3,784 | 0 / 0 |
| 4 | 984,808 | 53,936 | 6,312 | 41,120 / 2,568 |
| 8 | 984,800 | 55,152 | 7,528 | 41,112 / 3,784 |
| 10 | 984,840 | 55,760 | 8,136 | 41,152 / 4,392 |

相对上一份`task-retirement-product-matrix-20260923`整固件，启用档Flash增加1,796/1,800/1,796 B；四档静态RAM及product大小均未增长。0槽Flash差8 B，不宣称精确零Flash变化；其符号表仍无任务代码/状态。启用档符号表确认`TaskEngine::describeTargets`与`TaskPropertyTarget::describe`真实链接，非只编译无人调用代码。临时结果结构变大仍可能影响栈峰值，编译数字不能替代运行测量。

证据：[资源报告](../build/task-catalog-product-matrix-20260923/resource-report.json)，同目录各capacity有compile.log/symbols.txt及ELF/hash。源摘要`6943d2752b8dbdf67f51ec2a641784d161ff4592865a33c55b74f8630422546d`；维护者复现入口`tests/arduino/task_resource_gate.py --product --arduino-cli <cli> --config-file <stock-3.3.11-config> --build-root <new-directory>`。不含启动、heap/stack峰值、实际Flash写入或其他MCU准入证据。

**唯一下一步T2b-2**：原任务页面显式启用→认证目录/Manifest→同修订表查询→typed操作/未知意图恢复；保留页面结构，删除被实际替换的旧JSON和延时猜成功。之后T2c真实owner/App/COM3闭环验收新policy与本目录。目录软件完成不等于页面完成、固件已发布或T2已收口，不推进A1、BLE任务或通用能力平台。

### 10.28 第二十九批：T2b-2原任务页面与未决意图处理（2026-09-23）

本批只接App消费者，不再扩设备执行器或Service。原路由/列表/编辑器主题保留；原TimerService改成页面状态/取消适配器，注入根级唯一DeviceTaskClient/HttpTaskApi。完整交互与证据见[App任务readme](../../blinker-app/src/app/sections/device/device-timer/readme.md)。

- 目录是目标权限名单，Manifest缓存仅提供同key/type的范围/单位提示，不证明是最新固件约束；实际验收与执行依旧由MCU检查。无缓存不推断switch/toggle动作，也不要求为编辑建立第二条实时连接。共享taskDefinition校验复用到表单，不复制数值/协议解析器。
- 列表先目录再metadata/同revision记录，用真实taskId导航；编辑保留加载时CAS，刷新不自动覆盖用户草稿或CAS，明确“重新载入”才换基线。创建不Start，enable不恢复；Start/初始化/删除/恢复原请求均有独立确认。
- 当批WebView localStorage写原ID/完整意图先于HTTP；存储失败不发。**此持久屏障假设已在第三十批真实force-stop中失败，由§10.29的strict IndexedDB取代，保留本条仅作历史。** 任务回执认证后再读，读失败不假空表，historical只作原请求历史确认。页面离开/销毁/账号代际变化取消，root客户端协调Ionic保留的页面；旧弹窗也校验页面代际。token原账号续期不是另一次Initialize。
- 只有新鲜完整表读取后，用户明确确认才可停止本地原意图追踪；compare完整原记录后删除本地项，不删除Service请求、取消设备任务或证明未执行。未知记录不会自动清除、换ID/CAS再Start。无核对/损坏存储保持拒绝；实际WebView进程重启、卸载/清数据边界另列。
- 删除旧JSON timing/loop/countdown发送、160ms先开始后暂停、900ms假刷新、数组索引身份、20条假上限、默认动作和旧循环UI/样式；设置页删除旧data.timer徽标。保留主路由和页面用途，不重写全App编辑器。
- 确认原Broker每连接每秒4交换，每管理请求通常2交换；HttpTaskApi补550ms可取消HTTP间隔，响应仍只靠真实回执。不扩大Broker/64请求账本；大目录连续全量刷新仍可能额度耗尽，明确失败，不通过重连绕过。不在本批另做目录缓存/性能工程。

验证：完整spec TypeScript、Angular相关90/90（客户端16、页面服务9、页面/模型9、管理9、拦截器31、设置16）、真实整应用AOT构建通过。修正原拦截器GatewayHttpError.code与原HTTP error.errorCode形状差异，测试使用规范化错误；含分页一致性/重复ID、原意图跨页面恢复、取消、存储失败、无隐式Start、表单CAS及节流。构建产物在App `build/task-pages-20260923`，测试配置不是生产APK/视觉/ADB验收。未重新运行或累计前批MCU/Service结果；未改设备/Service生产代码、wire、业务数据库或默认开关，未刷板/清授权/部署/暂存推送。

**下一步只做T2c**：准备匹配真实账号/任务服务受控准入→核对专用COM3备份/现场网络→App启用/安装/Start确认→退出→设备执行→重进查询；覆盖原ID未知恢复、真实App进程、设备重启/有效授时与新policy，结束恢复。COM11/COM12不动；不将旧policy内部MCU实测或本批模拟HTTP当作已完成这一门禁。

### 10.29 第三十批：T2c-1真实owner/App/COM3及失败修正（2026-09-23）

#### 范围与真实前置

- 仅COM3专用XIAO ESP32S3（现场MAC `8C:BF:EA:8E:F1:2C`作操作保护，不作业务身份）；COM11/COM12不操作。由真实App既有owner登录，在原创建页面生成一个专用WiFi设备，复用DeviceKey登录、唯一MQTT和原授时。没有伪造用户令牌、直写用户数据库或拿oracle代替MCU。
- Gateway库先pg_dump并验证归档目录，核对schema38后应用既有039～042；只部署本机Gateway和该设备所属Broker shard 1。双侧维护名单只含本轮专用ID，其他Broker、Auth/Kong/Redis未整体升级；私钥/Key/密码只留受限的ignored现场目录，不进入源码或文档。
- 编译stock ESP32 3.3.11真实4槽WiFi产品，单目标power(bool)，共享试点policy为容量10、每6秒补1写、冷启动空桶、Start间隔30秒、提交故障阈值100ms。沿用现有TaskPilotPolicy，不增加宽松真机配置或普通Arduino入口。

#### 已过与发现的真实失败

| 门禁 | 真实证据与限定 |
| --- | --- |
| owner鉴权、初始化和目录 | 最初tasks:execute未被App原管理URL谓词识别，报AUTH_TOKEN_MISSING；修正精确路径后真实用户链路通过。首次仅提示启用，明确确认Initialize后目录为4槽、一个power(bool)，不把所有可写Manifest属性放行 |
| 安装、退出、执行、查询 | 原页面动作/确认弹窗创建20秒倒计时，安装不自动Start；单独Start确认后离开到首页，activeDeviceLinks=0并force-stop。到期设备回调增加1；重进认证查询为Done |
| localStorage失败对照 | 在真实Start认证响应进入App、尚未交给客户端结算处人为阻断，然后立即force-stop；原localStorage适配器的未决意图丢失。保留失败证据，没有伪造恢复记录或把已执行结果冒充原ID恢复 |
| IndexedDB原意图恢复 | 新版本在同一受控边界阻断/真实force-stop；重启后原ID与完整Start意图仍在。用户明确恢复原请求得到historical，服务器attempts=1，表修订/Start序号不增加，MCU本次回调只增加1；不是射频故障或物理断电证明 |
| 设备重启中止 | 再次Start后在到期前软件重启COM3；45秒后超过20秒期限，回调仍0，认证表progress=Aborted，未自行续跑。只覆盖软件重启，不替代T1真实断电窗口 |
| 最终异步显示 | 发现组件逻辑ready而OnPush页面未更新；改为一个signal状态驱动原访问器，新增真实父/子模板测试。最终APK已安装核对SHA，切网后真实显示TASK_UNAVAILABLE；有任务的成功列表视觉仍待同网复核，不能拿错误页面作为完整成功UI证据 |

#### 最小修正，不再扩一套实现

1. `isDeviceKeyManagementUrl`仅增加原任务资源路径；拦截器回归覆盖外域/相似路径不带令牌、401刷新保留原ID/正文。没有绕过鉴权或另建HTTP客户端。
2. 原TaskJournalStorage变为可异步端口；唯一IndexedDbTaskJournal等待strict事务oncomplete后发HTTP，删除原localStorage适配器且无静默回退。读写/清理失败及旧WebView不支持strict均有界报错；锁在异步读之前取得，跨await复核账号/取消/完整意图。账号隔离、不保存Key、无自动retry/新Start/持久队列。strict语义见[IndexedDB事务](https://developer.mozilla.org/en-US/docs/Web/API/IDBDatabase/transaction)，不声称能抵抗卸载/清数据/存储驱逐/磁盘损坏或多窗口竞争。
3. TimerService维持原页面生命周期接口，用单一signal状态通知列表和相同引用的状态子组件，不注入视图ChangeDetectorRef，不添加全局轮询或延迟猜显示完成。页面取消及旧弹窗代际保护仍保留。

#### 构建、测量与证据

- 最终相关Angular **75/75、0 skip**：客户端18、日志5、页面服务10、页面/模型9、拦截器33；完整spec TypeScript、local/local-device AOT、Capacitor与Android Gradle构建通过。新增异步锁/存储失败、commit前不发、清理失败保留、事务超时及父/子模板响应式测试。前批90项范围不同，不累计或伪称本轮全部重跑。
- 最终APK SHA-256：`50afbadf156218339832067da77a139e3d4f66d97a3e5ae55a5cb34b9c0eeb33`；核对手机安装文件与构建制品一致。原ID恢复/设备重启门禁由此前IDB版完成，最终版增加显示修正，成功UI还需补证。
- COM3编译Flash **990,424 B**、静态RAM **54,000 B**。样本snapshot最大 **46.783ms**、Blinker.run调用最大 **303.201ms**、最小空闲heap **243,224 B**、loop任务栈余量最低 **3,932 B**。后两项不是整个SDK的所有任务资源；最大调用不能直接归因于NVS，100ms提交故障阈值不是硬实时上限。没有完成全部policy边界、32目标/10任务或长期寿命验收。
- 可复现源码仍为tests/arduino/Esp32WifiTaskLiveGate及TaskPilotPolicy、tests/hardware/task_product_gate.py。受限现场目录`build/task-owner-live-20260923`保留App分步结果、软件日志、构建、备份/部署清单和错误截图；私有配置/完整Flash/数据库归档不得上传。CDP操作原App组件与HTTP链，设备端始终为真实产品。

#### 现场收尾与下一步

- 用户收尾时将网络由mostfun2026改为“带翅膀的蜗牛”；电脑192.168.2.76，手机在同路由5GHz的192.168.2.31。COM3测试fixture仍是旧网/旧IP，因此不追加成功控制结论；下一批须重读现场并更新**私有测试**配置，不能沿用192.168.1.146映射。
- 仅新建专用设备经原owner删除API移除：202、cloudAuthorization=revoked、brokerCleanupPending=true、localTasks=not_confirmed。App清单已无该设备且activeDeviceLinks=0；没有删除旧设备或其他账号授权。离线未获得退休证明，保留待清理/未知语义，不将云删除写成设备任务已退休。
- 当前完整8MB备份：先读取本轮系统/NVS区，与已知完整镜像合并，再与设备**整8MB校验一致**后才允许测试写入；不是仅凭旧文件当作最新备份。测试结束写回该完整备份并整区verify-flash成功，SHA-256 `47e2c983163808c25006ea16cc5a9c99b09595b9a19e5b478c1c69ac19763221`，原固件/NVS授权恢复。此校验不冒充原固件在新WiFi已上线。
- 本轮临时`blinker-task-owner-plain-http`容器已停止移除，没有数据卷；数据库/Flash备份保留。本机Gateway/Broker候选镜像和schema42保留，名单目标已删除，未回滚业务数据库、未全局开启任务。原生成映射仍待新网配置，Broker清理待完成不扩展为整组维护。本批未改设备/Service生产源码、wire或默认发布开关，未暂存/提交/推送。
- **唯一下一步T2c-2**：先最终成功页面、一次日期/星期及待授时/恢复、必要普通控制共存；再有限policy/owner/Key变化门禁。先备份/结束恢复COM3，COM11/COM12不动。不重开已过的退出执行/原ID进程恢复，不转A1、BLE/混合任务或服务器维护。

### 10.30 第三十一批：日历真机与重复查询收缩（2026-09-23）

#### 现场范围与已通过项

仅专用COM3 XIAO ESP32S3、真实App原owner、新建一个测试设备，COM11/COM12不动。最新WiFi“带翅膀的蜗牛”，PC192.168.2.76；手机同路由5GHz192.168.2.31。恢复材料在本轮再次核对整8MB后才刷测试程序。设备源码/4槽单power目标/共享policy未改，只在ignored生成配置中换新设备Key、SSID及控制地址。

| 门禁 | 本轮结果与边界 |
| --- | --- |
| 成功页面 | 第三十批响应式APK真实显示2项任务/容量4、稳定ID、日期和固定时区；刷新后显示各自本次完成。最终新APK也核对了成功列表，不再留“只验证错误页面”的缺口 |
| 一次日期 | 原App编辑器创建，UTC到期值与最终consumedUtcMillis均为1790165984281；设备power=true、回调1，认证progress=Done。手机时区当时UTC-7，未擅改手机；UTC转换不是按电脑UTC+8猜测 |
| 固定偏移星期 | 原编辑器保存周三bit2、05:21、偏移-420分钟；对应UTC occurrence=1790166060000。设备随后power=false、累计回调2；App账号MQTT真实State同步、认证表Done。首版固定偏移不声称自动夏令时 |
| 重启与授时 | 在两任务到期前软件重启：实际捕获wifi=6/timeValid=0/callbacks=0，随后WiFi与UTC恢复，并在到期前同步；原定义保留且按期执行。**没有覆盖无来源持续跨过到期再恢复**，不把这一有限样本写成全部失时语义已过 |
| 完成后再重启 | 两项消费水位/Done保留；再次恢复UTC后回调0、task snapshot零新增写，不重放已消费日期/星期 occurrence。不证明下一周已经实测，也不是物理断电 |
| 普通控制共存 | 在同一个真实账号MQTT观察会话中按钮ACK约107ms、计数1；属性ACK约299ms、真实State符合，账号session未变化。硬件累计回调3=两次任务＋一次普通属性，无新增用户/设备MQTT连接 |
| 编辑替换/删除/新建 | 替换保持taskId2且definitionRevision增加；删除后新建ID3/4，不复用1/2；两个新倒计时均progress0/startSequence0，未因保存自动执行 |

#### 真实失败与最小修正

旧App流程连续创建时返回TASK_RATE_LIMITED；不是任务未保存：设备snapshot及随后认证查询证实已安装，失败出在后续查询。现有HTTP任务路由复用checkAuthRateShared，默认账号/IP各30次/分钟；编辑器写后先全目录/整表读取，再导航回列表完整读一遍，重复消耗预算。

- 只在`TimerService.write`增加内部`refreshAfter`选择（默认true）；编辑器保存/删除传false，收到当前账号/页面的认证确认才导航。原页保持ready=false，不把旧表当新表；目标列表仍走原fresh目录/同revision读取。Start/enable/原请求恢复留本页时仍默认读回。
- 未决/取消/换账号仍保留原意图且不导航；未删除写前日志、回执认证、CAS或目录检查。没有新缓存、请求合并协议、队列、连接或时间猜成功；没有放宽全局认证/Broker限额。
- 实际记录HTTP操作：2任务替换＋返回列表为`replace,catalog,catalog,read,read,read`，共6次（原代码路径需11）；删除后剩1项为5次（原路径需9）。不包含进入编辑页的读取。预算窗口恢复后连续创建两项也通过。未外推32目标/10任务或任意频率管理；鉴权共用预算和64账本仍是准入限制。

#### 软件、制品与测量

- 最终相关Angular **78/78、0 skip**：客户端18、日志5、页面服务12、页面/模型10、拦截器33；完整spec TypeScript、local/local-device AOT、Capacitor/Gradle构建通过。新增唯一读回、未知写保留、日期/固定偏移编辑往返与删除确认后导航测试；历史75/90不累计。
- 仅App两个生产文件（TimerService、TimingEdit）及两个测试变更。APK `afe10f7d1ebbb51ba6ece4781ba6429cfb86bca60535bca702830c97b0ccfdf3` 已与手机安装文件对照一致，并真机验证修改/删除/连续新建及最终页面；未把旧APK的操作当成新APK证据。
- COM3重新编译：Flash990,424 B、静态RAM54,000 B。有限样本snapshot最大46.333ms，loop调用最大505.114ms，最小空闲heap237,272 B、loop栈余量最低3,916 B。loop最大值包含联网等等待，不等于NVS时延；本轮不能承诺100ms硬实时、所有SDK栈或长期Flash寿命。
- 私有现场目录`build/task-calendar-live-20260923`保留分步App结果、串口结构化数据、只记录操作名/确认的请求轨迹、APK、截图和`verified-evidence.json`。复核脚本对日历UTC/周位图、前后UTC观察、消费后不重放、6/5次读取及两次新建作实际断言；不是额外一组伪装成硬件的mock测试。Key/密码/数据库/Flash备份不得上传。

#### 收尾与剩余

- 本机只沿原候选镜像调整Gateway/Broker shard 1单设备名单及新IP，schema42备份但无迁移；Auth/其他Broker不改。新增临时DeviceAuth-only代理`blinker-task-calendar-plain-http`已移除，无数据卷；旧服务和DB/Flash备份保留。
- 新设备经原owner正常删除：202、cloudAuthorization=revoked、**localTasks=retired_verified**、brokerCleanupPending=true、realtimeRefreshPending=false。任务确有认证退休证明，但Broker清理仍独立待确认；App清单移除、activeDeviceLinks=0，不删除其他记录或直写数据库。
- 首次完整恢复写入的内置hash通过，启动固件后的独立全区校验出现差异。该工具原顺序会在二次校验前运行固件，可能受到NVS开机写入影响；保留失败日志，不宣称已定位到某个NVS字段。仅修正私有测试工具为下载模式下写回→独立全8MB校验→再启动，第二次恢复及独立校验已通过。`restore-verified.json`记录8,388,608 B、备份SHA-256 `47e2c983163808c25006ea16cc5a9c99b09595b9a19e5b478c1c69ac19763221`；原固件/NVS已恢复，不等于原固件在本轮新WiFi已上线。
- 当前仅完成上述有限门禁，**T2尚未整体关闭**。下一批按总主线§6补来源失效跨到期/恢复不补放、冷启动/Start冷却/满表/四任务同到期、Key/非owner拒绝；不重跑成功页面、重开其他通道或进行服务器整体维护。正式任务opt-in/profile与最大目录预算仍不宣称已发布。未暂存/提交/推送。

### 10.31 第三十二批：失时跨到期、满表与Start边界（2026-09-23）

#### 场景、结果和失败边界

仅专用COM3（8C:BF:EA:8E:F1:2C）、真实App原owner新建一条测试记录；COM11/COM12不操作。网络仍“带翅膀的蜗牛”，PC192.168.2.76、手机192.168.2.31。既有内部4槽/单power/policy源码原样编译，只更新ignored配置中的新Key。DeviceKey由原App创建/复制链取得，后续有限管理操作由原App DeviceTaskClient/HTTP/账号鉴权执行，不提取token、不直写数据库、不调用MCU trusted接口。

| 门禁 | 本轮有限证据 |
| --- | --- |
| 失时跨到期 | 安装UtcOnce=1790168596355；暂停仅本轮使用的DeviceAuth代理后软件重启。串口实际捕获到期前与到期后约31秒均timeValid=0、callbacks=0。恢复代理后UTC有效，旧定义仍在、callbacks=0、snapshot写入0，没有补执行；不等同于RTC老化/所有时间来源失效或物理断电 |
| 恢复后仍可执行 | 将原任务改为未来时间，再创建3项同刻任务；UTC occurrence=1790168801624。实际回调4次、4条认证progress=Completed、各自consumedUtcMillis相同。snapshot由4增至12，符合4次Claim＋4次结果写入；不是4目标或10槽准入 |
| 满4槽 | 第5次Create返回TASK_OUTCOME_UNKNOWN且原ID留存；设备snapshot仍4，认证表仍4项、tableRevision=5，无新增/覆盖任务。结合既有host CapacityExceeded分支证据验证保护；HTTP响应本身**不含认证具体拒绝原因**，不能宣称用户得到了确定的“容量已满”回执 |
| 运行中Start冷却 | 改2项为2秒倒计时；task1 Start确认约2.4秒后尝试task2，后者没有启动，task2 startSequence=0，表修订8。冷却超过30秒后显式重提相同requestId/CAS得到fresh确认，task2 startSequence=1、Completed，累计回调6=4日历＋2倒计时。无自动重试或更换请求号 |
| 冷启动尝试 | 第一次重启后约8秒在metadata阶段失败，未保存具体HTTP原因，不记通过；第二次直接使用重启前认证revision=9，在约9秒返回TASK_UNAVAILABLE/503，MCU未写入/执行。仍不能归因为引擎额度或启动冷却；后续认证表不变并经原UI明确停止本地追踪。**冷启动6秒额度与启动30秒冷却仍未真机关闭** |

满表的未知意图按原规则先fresh表核对，再用原确认弹窗停止本地追踪；不删除Service未知账本。运行中被限的Start则在窗口结束后显式恢复原请求。二者不能合成“收到任何失败都新发一次”。

#### 仅修必要的App表达

1. `TaskProgress::None`在失时后仍可能存在，不能显示为必定还会执行。原模型按任务类型显示倒计时“等待手动开始”、日历“尚无执行记录”；不凭手机Date.now推断MCU已跳过/完成，不加状态/时钟/刷新线程。
2. 实测证明“恢复原请求”可能在限制解除后**首次执行**。原确认文案“不开始一笔新的倒计时”容易被当成只读查询，改为明确“使用原请求号和参数重新提交；如果原操作尚未执行，这次可能执行”。TASK_OUTCOME_UNKNOWN同步说明先查询、恢复并非只读；未放宽任何鉴权/回执验证、CAS或未知日志。
3. 只改App模型、原TaskStatus确认组件及页面测试；设备/Service生产源码、wire/schema不变。未为了区分忙/满新增Failure格式；可验证拒绝结果的最小合同保留正式准入前处理，未认证失败不能直接清除原意图。

#### 测试、资源与收尾

- 设备重新构建后x64 ASan完整 **70/70、0 skip**（含fuzz smoke）；不是历史Win32的69项相加。首跑因未配置本机ASan/DebugCRT DLL路径报0xc0000135，修正该进程PATH后重建重跑，无业务代码规避。App最终相关 **79/79、0 skip**：客户端18、日志5、页面服务12、页面/模型11、拦截器33；完整spec类型检查、AOT/Capacitor/Gradle通过。
- 原4槽固件Flash990,424 B、静态RAM54,000 B；含收尾样本snapshot最大49.742ms、loop最大399.015ms、minHeap229,912 B、loop栈余量3,916 B。未测所有SDK栈、磨损寿命或最大规模；loop值不能直接归因于NVS。
- 最终APK SHA-256 `4e7f3d743cd4db65458ea02a938cbc2e01ffee6a7984de5a32063c81f11451b3`已安装并核对手机文件。最终版成功列表DOM确认“尚无执行记录”，原恢复弹窗实际显示“这次可能执行”，取消后原意图相同；已看截图核对未知请求面板。真实未决请求后立即查询还遇到TASK_BUSY，窗口结束后显式查询恢复；不自动绕过持久单飞锁。前一个仅改进度文案APK `4a2f19d93c7442da040bdafa90f98417ed0e33cbeb6f8d8e14f197daf29ac1a5`仅作中间制品，不冒充最终证据。
- `build/task-boundary-live-20260923`受限目录保留分步原请求、认证表、串口结构化指标、备份和`verified-evidence.json`。独立断言检查失时/恢复、4任务进度/写次数、Start窗口与相同原ID；冷启动明确输出inconclusive。不上传设备Key、网络密码、整Flash或数据库备份。
- 本机沿原候选镜像仅换单设备名单，Gateway库schema42先备份无迁移；不更新Auth等服务或Service生产源码。专用设备经原owner DELETE返回202、cloudAuthorization=revoked、localTasks=retired_verified，App清单无该设备且activeDeviceLinks=0；brokerCleanupPending=true/localAccess=not_confirmed仍不伪报完成。临时`blinker-task-boundary-plain-http`已停止移除，无数据卷；备份保留，双侧名单仅指向已删除的测试设备，代码默认开关不变。COM3在测试前重新验证完整8MB备份，收尾保持下载模式写回→独立整区verify通过→再启动；备份SHA-256为`47e2c983163808c25006ea16cc5a9c99b09595b9a19e5b478c1c69ac19763221`。原固件/NVS恢复不等于其旧网络配置已连新WiFi。`verified-evidence-final.json`另确认退休、取消恢复不丢意图和整区恢复。无Git暂存/提交/推送。
- 唯一下一步见主线§6：冷启动归因/有限补证、Key/非owner与试点拒绝结果准入。失时、满表、同刻执行和运行中Start冷却不再作为未完成项反复重测；不启动A1、BLE/混合/网关任务或整组维护。

### 10.32 第三十三批：冷启动连接归因与Key刷新恢复（2026-09-23）

#### 修正范围

- Broker原唯一连接选择把无候选和多候选合并，且在选择层过滤未完成Hello的socket。现在注册表只枚举未过期的物理候选；任务carrier负责唯一性、Hello/订阅和会话有效性，旧`getUniquePhysicalV2Connection`入口移除。多候选含未协商的第二连接也拒绝，不取最新、不强制断开其他socket。
- Gateway在owner/profile检查后仅映射固定连接错误：`TASK_DEVICE_OFFLINE / TASK_CONNECTION_NOT_READY / TASK_CONNECTION_CHANGED / TASK_CONNECTION_AMBIGUOUS`。非owner不能借此探测在线信息。已取得持久attempt后的错误仍包为`TASK_OUTCOME_UNKNOWN`，保留原请求/租约；一个离线重试也不能否定此前可能执行过的请求。App只增加准确提示，不加重试器/队列。
- 真机发现新Key上线后原任务根尚未同步，目录Read会变成未知结果，页面无法启用同步。任务repository在已确认旧根与当前根不同时先返回`TASK_AUTHORITY_REQUIRED`，不签发不可用Read或消耗账本；由原UI明确Initialize/Rebind。若已存在对此新根**明确签发过**的Rebind，仍允许认证Read核对丢失回执；仅prepared意图不够。未确认首次Initialize的原恢复路径保留。
- 没有改设备核心、wire/schema、密钥合同、写额度/30秒冷却或普通Arduino API。私有fixture仍4槽/单power、Flash990,424 B/静态RAM54,000 B。

#### 实物结果（仅COM3，COM11/COM12未动）

1. 正常owner创建专用记录，App显式初始化、安装2秒倒计时。第一轮冷启动在约23.6秒取得认证metadata后Start返回未知；后续认证表仍revision1/startSequence0，串口snapshot0/callback0。超过30秒后原ID重提获得fresh确认，Done/startSequence1，3次快照写入、1次回调。
2. 最终候选再次软件重启：约4.9/12.9秒返回`TASK_CONNECTION_AMBIGUOUS`，约23.1秒认证Read可用，23.9秒Start进入管理后未知；串口snapshot0/callback0。持久15秒attempt租约内的立即查询为TASK_BUSY，未绕过。约66秒显式原ID重提通过，startSequence从1到2，最终Done、3次快照写入、仅1次回调。结合核心保护分支支持30秒冷启动Start门禁；**Failure自身仍不是认证的WouldBlock回执**。
3. 因此本轮复现的早期503属于连接唯一性准入，不是Flash预算。不能倒推所有历史503均为同一原因；0～6秒云通道不可用，不能宣称真机覆盖了首个写额度时刻，也不引入测试旁路来假装覆盖。
4. 经App设置页原确认弹窗刷新Key至version2，旧任务云连接失效；旧Key软件重启后约42秒仍不能访问任务云通道，无任务写入或回调。新Key写入同一fixture、仅重刷application保留NVS后能上线，但任务根保持待同步。修正后的原页面显示`TASK_AUTHORITY_REQUIRED/canInitialize=true`；用户确认同步后原revision2、taskId1、定义、startSequence1和Done记录均保留，未重放，随后最终冷启动中的显式Start正常执行。
5. 这只覆盖**无未决写**的正常Key刷新。换Key前已有未知Start/Initialize时，原单槽journal与同步入口如何协调仍须下一批审查；不擅自删除旧意图或宣称所有Key恢复已完善。非owner软件拒绝已覆盖，第二真实账号/分享者硬件门禁未执行。

#### 测试与现场

- 最终Broker11/11、Gateway81/81、App82/82，均0skip；Gateway采用隔离PG schema与真实Broker/HTTP/MQTT、独立设备密码学替身，不计为实物。新增根变化Read准入和丢Rebind回执Read恢复、非owner不查连接、已入账错误保持未知及原ID保留测试。最初误用裸Vitest缺Angular初始化，以及并行构建导致worker/15秒测试组超时，均保留记录；按正确Angular入口及有界复跑通过，没有放宽业务超时。
- 最终APK `8d48276fa0cb1637f58a771e8413fbae6ffac63b48aa21a1cdbe4a0b53356ede`，AOT/Capacitor/Gradle通过、ADB安装并核对手机SHA；成功列表显示Done，未决/同步提示由原页面状态负责。完整spec类型检查通过；设备只编译/实物，不把上一批ASan70项累计为本批重跑。
- 网络“带翅膀的蜗牛”，本机192.168.2.76；仅Gateway/Broker shard1单ID白名单和任务候选镜像`task-startup-20260923`，schema42先备份无迁移，其他服务不整体更新。测试PG只新建/清理独立随机schema；临时回环PG代理不暴露数据库到LAN。
- 本地证据在ignored受限目录`build/task-startup-live-20260923`，包含原请求/认证表、冷启动时间点、Key前后材料、串口指标、DB和完整Flash备份；secret/整Flash不进入Git。软件重启不是物理断电。
- 原owner删除本轮专用设备返回202、cloudAuthorization=revoked、localTasks=retired_verified，App清单已移除且activeDeviceLinks=0；brokerCleanupPending=true仍独立待确认。COM3恢复及临时代理最终清理见本批收尾记录，不以云删除代替MCU恢复。
- 收尾已完成：保持ROM下载模式恢复整8,388,608 B，独立校验后再启动；备份/恢复SHA均为`47e2c983163808c25006ea16cc5a9c99b09595b9a19e5b478c1c69ac19763221`。仅本批DeviceAuth代理及回环PG代理移除，没有删除备份/业务卷；候选双侧名单只指向已删除测试设备。`verified-evidence.json`独立核对冷启动/原ID/新根保留表、最终Done、退休、整区恢复与手机APK SHA。原固件恢复不意味着原SSID配置已在线；无Git暂存/提交/推送。
- 最终Start样本snapshot最大46.550ms、loop最大304.683ms；此前新Key联网/待同步期间采到loop1.027s，未归因到NVS，也不以snapshot小于100ms宣称主循环实时保证。此批没有新增性能优化或长期寿命结论。

#### 下一批最小合同边界

仅补真实非owner、未决写叠加Key变化恢复，以及认证拒绝结果的受限准入。若复用原Result/HMAC结构表达拒绝，必须绑定既有完整challenge/原ID/digest/root，鉴权前Failure仍只是提示；区分“本attempt未应用”与“原request以前是否执行未知”，不可见到签名拒绝就清journal。暂态拒绝也不能沿现有acceptResult成功路径把request永久标verified，使稍后原ID恢复只得到historical。先落准确语义和故障测试再实施，不另造凭据/队列/错误协议，不转A1。

### 10.33 第三十四批：未决写与换Key的有界恢复（2026-09-23）

本批是T2恢复客户端的软件收口，不是新的调度/传输功能或COM3真机门禁。沿用现有HTTP initialize与设备Rebind，MCU源码、wire、Service生产代码、schema42和部署均未修改。

#### 已实施的最小模型

问题是循环依赖：旧写未确认→单槽禁止initialize→新Key任务根未同步→无法认证Read→不能核对后停止旧追踪。直接清槽会丢意图，放开所有写会形成第二个任务队列，均不采用。

- 一个account/API authority/device的IndexedDB键，保存**一个原业务意图＋最多一个显式授权同步意图**；上限2048字符不变，不存Key、root、Grant。普通路径继续读写旧TaskRequest格式；需要恢复时才原子保存`task-recovery/1`（pending、authority），该格式自身表示原意图held。authority可为null：同步已确认，但原意图仍需核对。
- 用户明确同步后，先strict提交“保留原请求、暂停其重提、保存独立initialize请求”，再发HTTP。不是两个独立键/两个提交。同步丢回包只能显式恢复它的原ID；不生成第三项。同步fresh/historical成功只清同步槽，**不清原意图、不判断原操作是否执行、不自动重发Start**。
- held跨页面/进程恢复保留；普通retry/新业务写由底层client拒绝，不只禁用按钮。Read/catalog不清两项；完整新鲜表成功后，用户才能明确停止本地追踪，比较完整两项状态防旧弹窗。停止追踪不撤销Service账本/MCU任务，不产生新操作。
- 根级client继续负责设备级互斥和账号隔离；页面服务只协调生命周期/显示，存储端口只负责原子持久化。没有新worker、MQTT连接、通用队列或自动重试。提交前取消、换账号、保存/清理失败都保留可恢复记录。

#### 验证与不可外推的边界

- App相关92/92：client25、IndexedDB5、页面服务16、页面/模型13、拦截器33；完整spec类型检查及local/local-device AOT通过。新增覆盖冻结、独立同步ID、丢回包恢复、提交屏障/并发/取消/失败、账号隔离、损坏记录、旧弹窗及双记录核对。不把内存API替身当真实设备根迁移。
- Gateway82/82、0skip：隔离PG schema＋真实HTTP/MQTT密码学替身。新增实际DeviceKey repository轮换，覆盖旧写仅prepared或已issued；Rebind后原请求正文/root/state保留、旧attempt转unknown，旧ID不能改绑新root，非owner不获签名，原scope可查询。不是MCU或真实第二账号证据；只改Service测试，无新迁移/部署。
- 未刷板/安装APK/切换手机账号，COM3/11/12均不动。临时回环PG代理已移除，无业务卷/备份删除。新日志`build/task-startup-live-20260923/gateway-tests-1790177726846.log`复用旧工具但属于本批结果。无Git暂存/提交/推送。
- 下一真机目标限于**已确认旧任务根＋单次换Key＋旧业务写未决**。首次Initialize未确认便换Key不能仅靠App双记录判通过：Service以confirmed_root选择previousRoot，MCU已初始化而Service未确认时，空previousRoot可能被拒绝。该窗口保留专项缺口。
- **同步未确认时再次换Key**不替换同步ID/扩日志。若当前根无法新鲜Read，会有REQUEST_CONFLICT/authority恢复限制；不能删日志或旧授权事实“解锁”。连续换根需要独立的有界证据合同。安全Key刷新不能被任务日志禁止，产品需提示该恢复限制。
- 仍不保证多窗口并发、卸载/存储驱逐、无限账本及最大32目标管理。30次/分钟、64请求/8attempt、15秒租约和4槽profile不变。

#### 认证拒绝：语义确定，尚未新增wire种类

Failure仍没有认证具体原因，保持OUTCOME_UNKNOWN。不能简单签名后沿`acceptResult`成功分支入库：它会同时把attempt和request设verified，暂态拒绝后原ID将永远只返回historical。

后续最小实施合同：

1. 仅覆盖**Grant验证且上下文有效、mutation第一次持久写之前**的明确拒绝；先限定写额度/Start冷却。不得将通用ErrorCode/WouldBlock直接映射成未写入，它也用于忙/恢复等阶段。
2. 复用Result/HMAC与完整challenge的instance/scope/root/nonce/requestId/digest绑定；先冻结跨端编码fixture/预算再增加独立拒绝分支，本批不占kind值。nonce仍一次消费，不增secret/重试队列。
3. “已验本attempt拒绝”单独终止并保存证据；request仍历史未知，不标成功、不抬回放水位、不参与成功写回收。所需schema/状态机变更需显式迁移，不偷塞进verified。
4. App显示本次可验证限制但保留原意图。窗口过后不自动重发、不保证下次成功；仍只允许显式原ID/CAS恢复，或新鲜查询后停止本地追踪。本次拒绝不能消除旧attempt的未知历史。
5. 可能已提交的存储错误、超时、回包丢失、认证前失败和上下文变化继续unknown。测试“第一次可能已应用→第二次拒绝”、拒绝后原ID恢复、篡改/旧nonce/旧根/掉电，再有限真机，不同时扩全部错误码。

**下一批**：先真实App/COM3验证单次换根＋旧未决业务写、实际第二账号/分享管理拒绝，沿原备份/恢复纪律；初始根未确认和连续换根保留未准入。认证拒绝按上述合同单独实施，不把软件通过记为T2完成，不转A1。

### 10.34 第三十五批：App未决写、单次换Key和同步丢确认的真实闭环（2026-09-24）

本批只验证§10.33的限定恢复模型，并修正一处App提示。没有新增队列、协议、凭据、设备核心或Service生产代码。证据位于私有忽略目录`build/task-key-pending-live-20260923/`；目录沿开始日期命名，测试跨日完成。凭据、原Flash和数据库备份不进Git。

#### 现场与可归因故障

- 电脑、手机、COM3使用“带翅膀的蜗牛”，本机服务192.168.2.76；原owner通过真实App创建专用普通WiFi设备。COM3身份8C:BF:EA:8E:F1:2C，COM11/COM12不动。先核对并备份原固件/NVS，独立验证完整8,388,608 B后才写测试程序。
- 原ESP32 3.3.11、4槽、单power(bool)测试profile；10写credits、1/6秒补充、冷启动空额度、全局Start 30秒及100ms快照预算均不变。Key1/Key2构建均Flash990,424 B、静态RAM54,000 B，只换私有Key配置、不擦NVS。
- 受控阻断位于原HttpTaskApi返回真实fresh认证结果之后、DeviceTaskClient结算journal之前；随后真实ADB force-stop。分别阻断一次旧Start和一次授权同步，不伪造成功响应、不重签意图。**Service已经verified，未知仅在App；不是射频丢包或Service只prepared/issued的真机证据。** 后两种仍由§10.33隔离软件回归覆盖。

#### 已通过的连续链路

1. 原页面明确启用→创建20秒倒计时→单独Start。请求`TDcMUZohjfdpVpPNgH8Tww`确认被阻断后重启App，原ID/正文保留；新鲜表revision2/startSequence1/Done，MCU callbacks1、snapshots5。
2. 原设置页明确刷新Key，credentialVersion1→2，旧连接无法继续任务云通信。烧录相同固件的新Key配置后，目录要求AUTHORITY_REQUIRED；旧请求不丢，可显式同步，但不能旧Start重提或新业务写。
3. 显式同步ID`LAp7di6veLIFPO5kiFbB6w`，阻断确认并再次杀进程。重进能新鲜读取原Done，原Start＋独立同步两项原子记录及held完整保留。实际页面截图核对“恢复原授权同步”和冻结说明，不把有可读表当成可新写。
4. 明确恢复同步原ID获得historical，只清authority，不清旧Start、不解held。Key2启动后的MCU在超过20秒的多次采样中callbacks=0、snapshots=1（仅根同步），没有旧Start自动重放。
5. 最终APK升级并重启后旧held日志仍在。新鲜表核对后，用户明确“已核对，停止追踪”，仅清本地记录；随后另行Start新请求`2eVeNvtDGMKuhQ447bkoIQ`。离页后任务执行，重进revision3/startSequence2/Done，callbacks1、snapshots4（根/Start/Claim/Done）。
6. 只读服务账本核对：Create、旧Start、新Start共3条mutation，各1attempt且verified；旧Start仍属旧根，新Start属新根。没有为恢复生成另一旧Start，不能以确认同步替代旧操作结果。

最后样本snapshot最大5.138ms、loop最大295.576ms、minHeap242,228 B、stackFree3,932 B；仅该4槽样本，不推导最大目录、长稳、寿命或实时保证。刷固件引发软件重启，本批无物理断电门禁。

#### 实际第二账号与未通过项

- 复用已有真实测试账号，通过本机Auth正常password login签发令牌，实际App切换并由Gateway `/auth/me`确认另一userId。未伪造JWT、写用户/授权数据库或改Auth。测试Gateway没有暴露login路由，因此使用本机Auth正常入口；不是手机登录UI测试。
- 非共享用户对同一设备catalog、initialize、Start均403 `TASK_FORBIDDEN`，不是页面隐藏按钮的替身验证。随后owner正常发出operator邀请，接收者accept返回409 `DEVICE_V2_CROSS_SHARD_SHARE_UNSUPPORTED`。**已分享operator管理拒绝仍待测**，不把“未分享用户被拒绝”替代已分享权限，也不越过当前跨片部署限制。
- 失败邀请已用正常接口撤销并核对state=revoked；finally恢复原owner及通知安装。下一次只在受支持的同片分享环境补该门禁，不启动跨片维护。
- 首次Initialize未确认便换Key、同步未确认再次换Key仍未准入；不扩大恢复日志，也不猜previousRoot。认证拒绝实施合同仍在§10.33，本批Failure语义不变。

#### 最小修正、验证与收尾

- 双记录页面暴露原成功提示歧义：同步historical不应显示为“原请求已有历史确认”。仅原TimerService依据本次是否授权恢复，分别显示“授权同步已有历史确认/设备已确认授权同步；原操作仍需核对，不会自动重提”；普通写提示不变，无新增状态机。fresh/historical测试同步覆盖。
- 最终App相关92/92、0skip：client25、IndexedDB5、页面服务16、页面/模型13、拦截器33；完整spec类型检查、local/local-device AOT、Capacitor/Android构建通过。本批未重跑Gateway/Broker/host套件，不累计历史数字。
- 双故障链候选APK SHA-256为`4fd00e17cb813efa2f12112ffad6d7f3b7970ec08d8bef87ae0316fc72856ca6`；提示修订后的最终APK为`526c131bacc0d9d1c76b6d5ca32bad0f5c8aa3332491c68bdb0c2b510082f804`，安装文件独立核对一致。最终APK实测held持久保持、明确停止追踪及新Start，没有声称在最终包重新跑完整双故障链。
- Service沿用`task-startup-20260923`镜像，只换Gateway/Broker shard1专用ID名单，schema42先pg_dump不迁移；其他服务/默认开关不改。原owner正常删除专用设备返回202、cloudAuthorization=revoked、localTasks=retired_verified；App清单已无记录、activeDeviceLinks=0。brokerCleanupPending=true与localAccess=not_confirmed仍原样记录，不扩大结论。
- 专用DeviceAuth-only代理已移除、备份保留。COM3在ROM模式写回整8MB，独立验证后再启动；备份/恢复SHA均`67e4ea39f33e72603a014ac45a1509c696c77b5af9a61c2e585b8cc65b829b6b`。与前批不同是本批开始前原固件合法启动改变系统区，不复用旧哈希冒充现场备份。原配置恢复不等于旧SSID下已联网。
- `verified-evidence.json`已逐项核对journal/两次恢复、未重放采样、新Start/账本、真实403/分享限制、删除退休、APK及整区恢复。未暂存/提交/推送，T2未整体关闭。

**下一批**：按§10.33认证拒绝最小合同先做跨端fixture/状态迁移与故障回归，再受限真机；同片分享条件具备时补operator。已过单次换Key组合不重复，连续换根和初始根窗口保留准入限制，不转A1或服务器整体维护。

### 10.35 第三十六批：认证拒绝三端实现与软件收口（2026-09-24）

沿§10.33冻结的边界完成§8.13，不扩通用错误系统。设备Engine只标记两个写前拒绝点，原Session/codec/adapter负责认证结果；Service共享解码器、操作绑定和原账本完成验真/提交，Broker仍只承载；App只扩严格响应解析和原错误提示，未另建恢复状态机/调度器。删除了把这些特定拒绝一律当未知的分支行为，未知存储/网络结果仍保留。

本批验证（不与历史批次累计）：

| 检查 | 本批结果与范围 |
| --- | --- |
| 设备x64 ASan | 69/69通过；本次构建未启用额外fuzz-smoke，不写作70项 |
| 独立协议/预算 | 4个两端相同fixture；逐字节篡改、截断、旧挑战/角色/原因拒绝；最大拒绝帧77 B |
| Node共享协议 | 全部45/45（含任务定向14项），Protocol/Gateway TypeScript构建通过 |
| Gateway / Broker | 88/88、11/11，均0 skip；隔离PostgreSQL＋真实Broker/HTTP/MQTT，MCU为密码学替身 |
| App | 95/95、0 skip：客户端27、日志5、页面服务17、页面/模型13、拦截器33；完整spec类型检查、testing AOT通过 |
| ESP32 stock 3.3.11 | XIAO ESP32-S3任务测试组合编译通过，Flash 990,524 B、静态RAM 54,000 B；不是运行堆/栈峰值或正式最小示例成本 |
| 结构/发布检查 | 现有architecture与release-profile检查通过；未生成新的正式ZIP/APK |

关键回归：前attempt未知→新attempt认证拒绝→原ID随后确认；拒绝前后无额外Flash写、nonce不可重用、已持久成功优先去重；伪Grant/通用WouldBlock/根变化/存储失败不产生拒绝证明。数据库042形状存量迁移保留历史，认证拒绝提交失败回滚，证明约束、重启后恢复及24小时后未知历史不被成功回收均覆盖。HTTP与App不把200当业务成功，提示后日志仍在，手动Read不清请求、不自动再次Start。

**边界与下一步**：没有烧录设备、安装APK、迁移常驻业务库或更新常驻服务；没有暂存/提交/推送。临时PG代理已回收，COM3保持上一批恢复的原固件/授权，COM11/COM12不动。下一批仅备份后准备匹配的本机schema043/Gateway/App/COM3候选，用有限样本确认拒绝提示、设备写计数不变和原ID显式恢复，再恢复现场；不得把替身或编译当真机。已有运行中Start保护可作为优先样本，启动首6秒无可用云通道仍不是Flash额度真机证据，不为抢窗口加入公开旁路。已分享operator只在受支持的同片环境补测；首次根未确认/连续换根、最大目录及正式profile限制保持，T2尚未整体关闭。

### 10.36 第三十七批：认证拒绝、杀进程和原ID恢复真机（2026-09-24）

本批复用第三十六批实现，无生产源码/协议/限额修订，无新增后台任务、队列或公开API。先做匹配候选，再验证真实owner/App/Service/COM3，未重跑日期/容量/Key组合或扩A1。私有证据在忽略目录`build/task-rejection-live-20260924/`，`verified-evidence.json`已独立断言下述关联及现场恢复。

#### 候选与现场

- 电脑“带翅膀的蜗牛”192.168.2.76，手机该路由器的`带翅膀的蜗牛_Wi-Fi5`网络192.168.2.31；COM3使用原2.4 GHz SSID。ADB真实owner/App创建本轮专用普通WiFi设备，测试不提取令牌、不直写用户或ACL数据。
- COM3身份8C:BF:EA:8E:F1:2C，写测试固件前刷新系统区备份并独立比对整8MB；COM11/COM12不动。SDK stock3.3.11、4槽/单power目标、10 credits/6秒补充/30秒Start冷却/100ms快照预算保持。新固件Flash990,524 B、静态RAM54,000 B。
- Gateway先pg_dump并核对备份，再由原迁移器从42升43；仅本机Gateway镜像`task-rejection-20260924`、shard1 Broker的单设备白名单。Broker镜像仍`task-startup-20260923`；Auth和其他服务不升级，无线上发布。
- App local/local-device AOT、Capacitor和Android构建通过，APK SHA-256 `7f2d61662ff3ff324f4fa174d7b66148b41b0753f3276cda1110f9f8a6b3e965`与实际安装包一致。Gradle仍有SDK XML版本工具警告，构建成功，不据此声称工具链警告已消除。未将上一批软件测试数再次累计。

#### 真实连续闭环

| 阶段 | App/Service证据 | MCU证据 |
| --- | --- | --- |
| 显式启用、创建5秒倒计时 | 原任务页面/确认弹窗，真实容量4和power目录，创建不Start | revision1、startSequence0，快照2、回调0 |
| 首次Start并执行 | fresh确认，认证查询Done | revision2、startSequence1，快照5、回调1 |
| 冷却内再次Start | `TASK_ATTEMPT_START_COOLDOWN`及真实提示；request pending、唯一attempt rejected、exact回执58 B | 快照5→5、回调1→1，没有再次Start/写入 |
| force-stop并重开 | 原ID/CAS/正文保留，canWrite=false；新鲜Read不清意图，账本没有新增attempt | 仍revision2、Done、快照5、回调1，未自动执行 |
| 明确恢复原请求 | 同ID新增一个verified attempt，原rejected证明保留；成功才清App日志 | revision3、startSequence2、Done，快照8、回调2，仅多执行一次 |

拒绝/恢复的同一requestId为`zmScva25WuZw7aTon2dU2w`；三条mutation总共为Create、首次Start、被拒绝后恢复的Start，attempt数分别1/1/2。没有生成新ID/CAS或根据HTTP200提前记成功。使用真实页面组件和确认弹窗，不是替身响应；本批是App进程中断，不是设备物理断电。

普通通信继续同一App账号MQTT：button ACK211ms、实际buttonPresses=1；power真实true→false控制两次ACK139/173ms，State revision3→4、设备power=0；加上前一次同值控制，共3次普通属性回调。最终总callbacks5=任务2＋普通属性3，snapshots仍8，普通控制没有额外任务持久写。观察结束释放需求，未增加设备/账号MQTT连接，不冒充完整控制页面UI验收。

本次样本maxSnapshot46.590ms、maxLoop255.504ms、minHeap242,412 B、stackFree3,932 B；仅有限4槽样本，不是峰值上界、寿命或长期实时保证。启动云通道未就绪时App先正确显示DEVICE_OFFLINE，随后SCOPE_REQUIRED；未把它算作写额度拒绝。**WriteBudget仍只有§10.35软件/编码证据，本批没有可归因的真实额度耗尽样本。**

#### 收尾和下一步

- owner正常DELETE得到202、cloudAuthorization=revoked、localTasks=retired_verified；brokerCleanupPending=true、localAccess=not_confirmed如实保留，不扩清理维护。专用记录从App消失，回首页、activeDeviceLinks=0。临时DeviceAuth-only代理已移除，无业务卷删除。
- COM3写回原整8MB，在ROM模式完成独立verify再启动；备份/恢复SHA-256均`e5a8b2bf5320a7217d7fd35f55793d34ea61c1420c0eb00ad64a82cc849e4af9`。原固件合法启动会修改系统区，故与上一批哈希不同，不能复用旧值冒充本轮恢复。设备/数据库备份保留；原配置恢复不等于已经验证其原网络业务。
- 本机匹配Gateway、schema43及App候选保留，白名单仍只含已删除测试ID，不扩大试点；不自动降级数据库或清历史。没有Git暂存/提交/推送。
- 下一步仅T2有限profile/权限收口：区分当前owner普通WiFi4槽封闭试点与正式发布，补受支持同片环境的已分享operator拒绝；首次根未确认/连续换根、写额度真机、最大目录仍未准入。缺同片环境保留缺口，不开跨片维护；不重复已过拒绝/恢复，也不提前转A1。

### 10.37 第三十八批：有限准入、首次未知根拒绝及权限回归（2026-09-24）

- 执行范围取总主线§6：冻结§8.14封闭试点，不重跑第三十七批真机、不扩大跨片维护。真实两账户只读复核仍在shard1/0，故已分享operator真机保持待测，不迁账号或改ACL。
- Service在已有仓储/事务内，首次根未确认但旧Initialize已有签发记录、Key变化时，前置409 `TASK_AUTHORITY_RECOVERY_UNSUPPORTED`。HTTP明确透传有限错误，内部细节仍隐藏；无新request/attempt/Grant/任务MQTT。App沿现有错误/原子日志停用无效初始化、同步及重试，不保存第二套恢复状态。修订原冲突提示，避免要求用户为任务保留不安全Key。
- 最终Gateway **93/93、0skip**，覆盖实际隔离PG账本、真实密码学、HTTP/MQTT测试替身；新增首次签发坏/丢证明＋两次换Key＋仓储重建、只prepared不误拒、退休/查询独立，以及viewer/operator逐阶段owner-only。服务编排确认无任务交换/签发、失败释放容量；HTTP错误与隐私差异回归 **7/7**，随后完整93项也含最终HTTP。共享协议/Gateway编译通过；没有用替身代替MCU或真实已分享账号。
- App相关 **96/96、0skip**、完整spec TypeScript、testing AOT通过。新增单原请求和双记录在错误/重进时保留，且不会经按钮发起无效同步/重试。没有APK构建/安装或本批ADB/MCU结果。
- 首轮新增测试缺Broker失效确认，被`DEVICE_V2_ROTATION_PENDING`正确拒绝；补隔离替身确认后，又发现旧迁移fixture不能在已有retiring行之后还原旧约束。将退休检查独立到迁移测试之后，未删除断言、修改生产轮换保护或改变业务库；最终整组通过。保留失败日志，不将中间结果算通过。
- 无设备源码、BBP/存储格式、schema迁移或资源上限变更；无部署/刷板/清授权、暂存/提交/推送。临时PG代理与测试schema正常回收。现场仍为第三十七批Gateway/schema43/App；本批候选未部署。设备Flash及备份未操作。
- 清除本节旧“当前仍待接WiFi carrier/App”的过期指引，历史证据保留；当前准入统一取§8.14，执行顺序取总主线§6。下一步只核对已确认根后的连续未确认换根窗口；权限真机等待支持环境，写额度真机/最大目录等未准入，不自动转A1。

### 10.38 第三十九批：连续未知根前置拒绝与正常历史对照（2026-09-24）

- 核对实际设备`prepareAuthority`：新root必须匹配当前凭据，previousRoot必须匹配安装表，不能由服务器按“最后签发”猜测。Service复用原持久化AuthorityChange/codec，将首次根拒绝和当前根Read检查合并成一次有界读取；依据最后确认根的出边识别未知中间根，不增加字段、根缓存或恢复队列。
- 软件覆盖已确认A→签发B未确认→当前C及再次变更、坏证明、原attempt issued/expired unknown＋更新prepared、仓储重建、五种管理入口拒绝且账本/签发保持；仅prepared B允许A→C。独立对照先丢Initialize/Rebind原回执再以Read确认A/B，旧请求仍pending且Read历史按原策略回收后仍允许B→C和后续已确认同步。owner退休/查询不因未知中间根被阻断。
- 最终Gateway **97/97、0skip**，实际隔离PG、密码学、HTTP/MQTT测试替身；日志`build/task-startup-live-20260923/gateway-tests-1790209263873.log`。首轮回收对照少推进attempt截止窗口，修正模拟时钟至`expires_at + 60,001 ms`，保留原策略/断言后完整复跑通过；不将中间失败当通过。
- App **97/97、0skip**、完整spec TypeScript和testing AOT通过，Gateway编译通过。App仅将旧错误文字扩展到初始化/同步，新增旧业务意图＋未知同步被拒后不丢日志/不重提的回归。没有新wire或设备src变更，因此不重复算前批MCU/资源测试。
- 无部署/业务迁移、APK安装、ADB/刷板/清授权或Git暂存提交推送；临时PG代理/schema按原测试清理，未删除业务卷。未重新验证账号分片或申报新权限真机证据，沿用第三十八批待测条件。当前运行候选仍是第三十七批。
- 当前规则集中维护§8.14，旧批次事实不回写。首次/连续未知根是“明确不支持并保留证据”，不是恢复已完成。下一步按总主线§6第12项仅做匹配候选差异真机：Service尚未验收真实结果的窗口必须独立于App丢确认；缺可靠注入则保留待测，不写数据库制造通过。不转A1或服务器维护。

### 10.39 第四十批：Service真正未知根的差异真机与跨根退休缺口（2026-09-24）

范围：真实已登录owner、Android手机196f65f9、COM3专用XIAO ESP32S3（8c:bf:ea:8e:f1:2c）。电脑192.168.2.76、手机192.168.2.31、网络“带翅膀的蜗牛”；COM11/12未操作。先核对身份与原8MB，再由真实App创建本轮单设备，不复用已删除ID、不清其他授权。私有证据`build/task-root-continuity-live-20260924/verified-evidence.json`独立断言下面的关联和恢复状态。

**匹配候选与故障边界**：App local/local-device AOT、Capacitor/Android构建、安装SHA核对；Gateway编译并在原镜像中仅替换Gateway/共享协议，Broker不换实现，仅更新单设备白名单。本机schema43完整备份，不迁移/升级Auth。临时Node preload仅作为容器挂载，在原`exchangeTaskCarrier`收到真实MQTT结果之后、返回给Service之前，对精确deviceId/shard1/Authority结果截断一次；不进生产源码/候选镜像。无arm、错误设备/分片/操作/kind、过期和单次消费7项检查通过。原Result/Grant私存，之后用生产codec和当时Key独立验证HMAC，不调用acceptResult补记账。

| 实物阶段 | 设备/结果 | Service及App事实 |
| --- | --- | --- |
| A已确认 | 4槽/1目标power；安装5秒倒计时，revision1、startSequence0，不执行Start | 正常Initialize、目录/表Read/Create全部确认 |
| 显式A→B，结果在Service前丢失 | 58 B认证结果kind0/revision1/count1；MCU快照1、callbacks0、taskError0 | 172 B Grant已存；request pending、attempt issued、exact_result空、confirmed_root仍A；App保留原Initialize ID |
| 再换C并实际杀进程重开 | C固件上线、授时正常；taskError22、任务快照0、callbacks0 | 首次/再次查询均TASK_AUTHORITY_RECOVERY_UNSUPPORTED；原日志不变，初始化/重试/新写关闭；原三条历史完整相同，无新attempt |
| 普通控制 | button计数1、power=true、callbacks1，任务快照仍0 | 原账号MQTT不换session；button ACK224ms、power ACK242ms，fresh State一致，观察退出释放 |
| 正常owner删除 | 无退休快照写入 | HTTP202、cloudAuthorization=revoked、列表移除；localTasks=not_confirmed、localAccess=not_confirmed、brokerCleanupPending=true，不能声称实物清理通过 |

**退休问题及下一步**：设备`TaskManagementSession::prepareRetirement`要求`stored == request.authority`且`current root == stored`；commit再检查相同关系。此时存储任务根为B、当前Key为C，Service发出当前C的Retire在挑战前被拒。账本新增唯一operation7、无Grant/Result，删除后attempt变unknown，scope变retired只是服务端状态。Engine自身有独立可信终止写入口，但当前Session没有合法跨根授权路径。此前仓储/模拟MCU的“退休独立”只证明Service没有错误复用普通管理拒绝，不能覆盖这个实物窗口。

下一批按§8.14/总主线§6第13项先审终止授权绑定，再做最小Session/Engine及恢复查询回归；不简单删除root比较、不在退休前伪造Rebind，不新增旧Key库或任意恢复。普通管理未知根拒绝仍保持。该缺口阻止宣称T2整体关闭；已分享operator、首次未知根拒绝实物、写额度实物、最大目录/其他平台与正式profile仍不外推。

**制品与现场恢复**：

- ESP32 stock3.3.11三次私有Key配置编译/仅应用烧录/读回均过；同一测试源码，Flash990,524 B、静态RAM54,000 B。不等于运行峰值或新增设备功能。
- 实际安装APK SHA-256 `e47ba4caa5f8e91ffb27b145e966d4886f8ff9acfe9b2e9b234a58a1f1e91225`；Gateway `blinker-gateway:task-root-continuity-20260924`、ID `sha256:d277be56d707d5fbbee537c44cd031af9df2c9a0f0f11a18b2b85b60286c6ddc`。Broker仍`task-startup-20260923`，Gateway/Broker健康检查通过。
- Gateway备份3,293,661 B，SHA-256 `17bcb39429132f81cfafb9d0bdf1ad3334279b6a85fc46f9c99f46e1cd42510e`；schema43不变，业务卷与备份不删。
- COM3原8MB备份/最终恢复 SHA-256 `592e6fb8741acc9502fd1a8fd61a0685c17f624948760a836ce18a6feba393ee`。先新读系统区、整8MB设备verify通过才用备份；最终整区写回并独立verify。这个恢复操作清除了本轮固件/NVS实验状态，不能冒充Retire协议成功。
- 临时preload的NODE_OPTIONS和挂载均撤除；专用DeviceAuth-only代理已删除，配置/证据/备份保留可重建。App首页、activeDeviceLinks=0；三块板/ADB均在线，其他两板未烧录、复位或清授权。仅保留匹配本机候选与已删除ID的试点名单，无公开开放或生产部署。
- 本批无三端生产源码、wire/schema变化，未重复累计前批97/97等软件测试数；只新增私有测试工具及同步文档，无Git暂存/提交/推送。

### 10.40 第四十一批：跨未同步根的认证终止（2026-09-24）

**最小实现**：按§8.15仅调整Session的Retire/RetirementRead双绑定检查。当前根用于认证、原完整存储绑定用于定位待终止对象；不修改Engine/Store、不预先Rebind、不恢复旧任务。公开API、wire/schema、Server/App生产代码与默认开关均不变。清理测试中已过期的“尚无在线退休命令”说明，同根/跨根Cloud场景共用原测试夹具，没有新建第二套Session或网关模块。

**本批软件回归**：

- 最终重建x64 ASan **69/69、0skip**，架构检查通过；Win32 Session/Budget **2/2**。新增覆盖真实codec/adapter/Engine、A→B存储后刷新C、错误instance/scope/旧根、普通Read/Mutation仍拒绝、一次终止写及重复不重写、D根重新加载后新Grant查询旧终止标记；八种故障含TTL/断链、验证前/中/写后换根、坏签名、Flash已写后报错、另一Session合法改变存储绑定。存储失败后只在新句柄中查询持久标记，旧脏句柄不自愈；软件重建不冒充物理掉电。
- 沿原Runtime/TransportHub/MQTT carrier覆盖同根与跨根Retire丢回执→断链→Hello→新鲜终止查询，原退休ID保留且无额外Flash写/业务回调。签名边界和socket/存储为测试替身，不能冒充密码学真机证据。
- Broker **11/11**、Gateway **97/97**，均0skip；复用原隔离schema/测试代理并按fixture清理。第一次与重编译并行时Broker两项超时取消，未改超时或逻辑；串行复测全部通过，原失败日志保留。无业务数据库伪造、账号分片或ACL修改。
- Win32 `Session=336 B`、cache76 B、adapter64 B、Cloud carrier96 B＋212 B workspace均不增加；Retire/Query最坏物理帧102 B、终止结果93 B上限不变。首个stock ESP32 3.3.11固件Flash **990,464 B**（比上一批少60 B）、静态RAM **54,000 B**不变；不把此静态数据等同运行峰值/其他MCU准入。

**有限实物结果**：同一真实owner/Android App、专用COM3 XIAO ESP32S3（8c:bf:ea:8e:f1:2c），网络“带翅膀的蜗牛”；先完整核对原8MB备份再写应用，COM11/COM12不动。App/Gateway/Broker沿第四十批匹配制品，不重建或更改生产实现；schema43先备份不迁移，仅改单设备名单并临时挂载单次故障钩子。

| 阶段 | 核实结果 |
| --- | --- |
| A确认并创建任务 | 4槽/单目标power、5秒倒计时、revision1/startSequence0；不执行Start，快照2/回调0 |
| B同步结果在Service验收前丢失 | MCU快照1/回调0；58 B实际结果独立HMAC通过，revision1/count1；Service仍issued、结果空、确认A；保留原同步ID |
| C固件上线及App force-stop恢复 | 首次/再次查询均RECOVERY_UNSUPPORTED，初始化/重试/新写关闭，原journal逐项不变；三条服务历史完全一致，MCU零任务写/零自动回调 |
| 原账号普通MQTT | button ACK170ms、power ACK204ms，fresh State、buttonPresses1/callbacks1一致；任务快照仍0；观察退出释放 |
| owner正常删除 | HTTP202、cloudAuthorization=revoked、**localTasks=retired_verified**；唯一Retire request/attempt均verified，172 B Grant＋74 B Result；App清单移除、活动设备连接0 |
| 独立终止核验 | Result kind4/revision1/count0、原退休ID对应；C Key HMAC通过，A/B Key均拒绝；MCU只新增1次快照、业务回调/按钮计数不再变化。confirmed_root仍A，没有预先Rebind或补记B成功 |

删除沿原合同将B的issued attempt置unknown，原request仍pending，原root/Grant/空Result保持；这是正常关闭未决尝试，不等同签发/安装成功。`brokerCleanupPending=true`、`localAccess=not_confirmed`仍独立保留，不能将本机任务终止成功外推为全部撤权清理已完成。

**制品/边界**：三份私有Key固件均Flash990,464 B/静态RAM54,000 B，应用烧录与独立读回校验通过。App实际安装SHA-256仍`e47ba4caa5f8e91ffb27b145e966d4886f8ff9acfe9b2e9b234a58a1f1e91225`；本批只核对和实际操作，未重复宣称App97项为本轮新测。Gateway仍`task-root-continuity-20260924`，Broker仍`task-startup-20260923`。本轮数据库备份3,418,283 B、SHA-256 `4e4a748f8da0cc97575cb432753db7a49fc7ac2bcaf66d808dcd8c6aed34f0d0`，保留可恢复。

**现场恢复已完成**：NODE_OPTIONS/故障挂载已撤除，临时DeviceAuth及回环PG代理均移除，原配置/数据库备份保留；Gateway/Broker健康检查通过。COM3整8MB原固件/NVS恢复并独立verify，SHA-256 `592e6fb8741acc9502fd1a8fd61a0685c17f624948760a836ce18a6feba393ee`与先前新验证的完整备份一致。ADB/COM3/11/12均在线，见证板未动。仅本批新建测试记录通过正常owner接口删除，未删除其他设备、业务卷或备份；板恢复与协议退休是分别验证的两件事。

最终独立断言已通过并保存在私有`build/task-cross-root-retirement-live-20260924/verified-evidence.json`，含MCU/数据库/制品/原日志/旧Key拒绝及恢复关联。无Git暂存/提交/推送。本次未新增物理断电/长期运行、其他平台、首次未知Initialize或已分享operator实物证据；未知根恢复仍未实现，T2未整体关闭。

### 10.41 第四十二批：首次未知根差异实物与用户入口准入核对（2026-09-24）

**范围**：复用第四十一批设备Session及已有App/Service候选，只补“从未确认任何任务根”的差异，不创建/启动倒计时。真实owner、Android手机196f65f9、专用COM3 XIAO ESP32S3（8c:bf:ea:8e:f1:2c）；电脑/MCU使用“带翅膀的蜗牛”，手机为该现场Wi-Fi5接入，192.168.2.31与电脑192.168.2.76实际可达。COM11/COM12不操作。先新读系统区并整8MB校验备份，再由App正常创建专用测试设备。

故障仍只在真实Broker回传后、Service验收前，临时单次挂载钩子限定deviceId/shard1/Initialize Authority Result，7项选择器检查通过。ADB中断发生在初始化之前，首个arm已撤销且只读账本确认请求0；手机恢复后才重新arm并显式初始化，未将连接失败算作协议测试通过。全过程不直接写业务数据库或调用acceptResult补证。

| 阶段 | MCU/认证结果 | Service与App核对 |
| --- | --- | --- |
| 首次Initialize A落盘、回执截断 | 58 B真实结果，kind0/revision0/count0，独立HMAC通过；快照1、回调0 | request pending、唯一attempt issued、Grant172 B/Result空；confirmed_root为空，App原ID保留 |
| 原设置页刷新B，真实App force-stop重启并刷B固件 | WiFi及授时恢复，taskError22；新启动任务快照0、回调0 | 首次/再次查询均RECOVERY_UNSUPPORTED，初始化/重试/新写关闭；原journal完全一致，四份服务快照相同，仍只有原Initialize，无新attempt/Mutation |
| 普通MQTT控制 | buttonPresses1、power=true/callbacks1，任务快照仍0 | 原账号session不变；button ACK224ms、power ACK251ms，fresh State一致，退出观察后释放 |
| 正常owner删除 | 74 B Retired结果kind4/revision0/count0，原退休ID对应；B Key验证通过、A Key失败；只新增1次终止快照，无额外回调 | HTTP202、cloudAuthorization=revoked、localTasks=retired_verified；唯一Retire request/attempt verified，列表移除、活动设备连接0 |

Service的`confirmed_root`全程为空：没有把首次Initialize补记为成功，没有Rebind或恢复旧任务。正常删除将原issued attempt关闭为unknown，原request仍pending、root/Grant/空Result保留；不是“全部历史没有任何状态变化”。`brokerCleanupPending=true`、`localAccess=not_confirmed`独立保留，任务退休不冒充全部清理完成。

**组合与交付审查**：

- stock3.3.11实际编译器对原产品入口执行六组`-fsyntax-only`检查：任务原配置、LocalAccess=0、LAN=0接受；LocalAccess=1、LAN=1、两者=1各由现有显式guard拒绝。LAN单开必须依赖LocalAccess，而任务试点排除该组合；无需新建同义guard或重写组合选择器。这六项不是六份链接固件或六项真机门禁。
- 两个现有真实账号只读确认仍为shard1/0，状态正常，跨片开关未开；已分享operator实物继续待测，未改账号分片/ACL/分享开关或创建新账号。
- 当前实物Sketch仍通过`official_detail::esp32WifiProduct().extension().configureTargets`配置允许调度的属性，普通Arduino用户入口尚未完成。按§8.16将最小opt-in/属性白名单与维护者容量/policy/信任profile分开，继续复用现有模块；本批未仓促公开测试宏或增加新通道。
- 架构检查、Arduino发布profile检查通过；关键设备Session/其回归、Service仓储、App TimerService源码哈希与本轮前置记录一致。未将前批69/11/97项回归重复算成本批新测，也未宣称所有生产文件无既有未提交修改。

**制品与恢复**：两份仅私有Key不同的固件均Flash **990,464 B**、静态RAM **54,000 B**，应用烧录/独立读回验证通过；只是本profile有限资源证据。实际安装APK SHA-256仍为`e47ba4caa5f8e91ffb27b145e966d4886f8ff9acfe9b2e9b234a58a1f1e91225`，未新建APK。Gateway仍`blinker-gateway:task-root-continuity-20260924`（镜像ID`sha256:d277be56d707d5fbbee537c44cd031af9df2c9a0f0f11a18b2b85b60286c6ddc`），Broker仍`task-startup-20260923`；只更新试点ID。schema43未迁移，事前数据库备份3,306,150 B、SHA-256 `4540b9b9f934545672e9bd571621c06e0213d35d24651af91208c52df0deeb23`保留。

COM3最终整8MB原固件/NVS写回并独立verify，SHA-256 `592e6fb8741acc9502fd1a8fd61a0685c17f624948760a836ce18a6feba393ee`与本轮新验证备份一致。临时NODE_OPTIONS/挂载与专用DeviceAuth-only代理已移除，Gateway/Broker健康；仅删除本轮专用测试设备，其他设备/业务卷/备份不删，COM11/COM12未动。退役证明在恢复原Flash前取得，两者不是同一证据。

独立汇总断言全部通过：私有`build/task-first-root-live-20260924/verified-evidence.json`，包括原journal/账本、58/74 B密码学证明、真实控制、六种组合、制品和恢复关联。无Git暂存/提交/推送，无新物理断电/耐久/其他MCU证据。唯一下一步为总主线§6第15项用户入口/profile；T2整体、未知根恢复和正式发布仍未完成。

### 10.42 第四十三批：公开任务opt-in、profile与资源验证（2026-09-24）

完成§8.17实现、两个内部probe迁移、README/canonical API及三端进度同步。移除旧`configureTargets`组合旁路及五个内部选择/policy宏，保留原Engine、Session、TaskPropertyTarget、TaskProductLifecycle、WiFi stack/carrier和持久格式。核心目标校验复用，facade只有模板方法、没有新实例字段；不要把已有授权/认证模块误判为“不必要代码”删除。

**实际验证**：

- x64 ASan全量 **70/70、0skip**；Win32 facade/product/session/budget **4/4**。新增只读/文本/重复key、首次失败后同类型正确配置、不同产品不得覆盖静态绑定、begin后/end后拒绝重配、自动授时、遗漏begin成员/写回调时NVS与网络不启动。原管理生命周期夹具改走真实Property＋Product准备，而不是以空target绕过新增准入；原认证/恢复断言保留。首轮因ASan DLL未在进程PATH而未能执行，日志保留；补正确工具链PATH后重建/完整通过，不放宽断言/超时。
- 实际stock3.3.11编译器 **15组语法准入**通过：默认聚合头、关闭/开启、缺profile/缺policy/非法policy、重复目标（编译允许但运行时拒绝）、空参数、Action、33目标、BLE/WiFiBLE、R4/Wio选择器以及LAN/LocalAccess组合。R4/Wio项只证明选择器拒绝，不冒充对应SDK完整编译或真机支持。
- 架构检查、发布profile检查、打包测试 **17/17**通过。原登录freeze仅对新增facade模板/产品选择接缝两文件审查更新，未改登录provider、DeviceKey/session/wire或begin签名；未移除freeze门禁。打包器不接受生产任务profile，普通profile仍不编入任务。
- 开发候选ZIP与stage逐文件字节核对通过；无内部类型的`CompileTaskWiFi`实际语法检查使用该候选生成的profile通过，不靠Sketch填policy。最终ZIP SHA-256 `c7c41c4c5e760140c6d39738d9a1b581a7c693d9c7c16b0feb7043818cccc2f0`。这不是IDE安装后完整编译/刷板门禁，也不是生产制品。

**实际链接资源**：同一SDK、同一WiFi/授时图，编译开关只改变可选任务组合；`task_resource_gate.py --product`核对完整Engine/管理符号确实链接、关闭版本没有任务符号，源码摘要前后相同。

| 任务槽 | Flash B | 静态RAM B | product对象 B |
| --- | ---: | ---: | ---: |
| 关闭 | 943,680 | 51,368 | 3,784 |
| 4 | 985,100 | 53,936 | 6,312 |
| 8 | 985,084 | 55,152 | 7,528 |
| 10 | 985,128 | 55,760 | 8,136 |

关闭项与既有`task-retirement-product-matrix-20260923`资源数一致；各开启档静态RAM/product对象大小亦不增长，普通Win32 facade仍12 B。4槽相对关闭增量为41,420 B Flash/2,568 B静态RAM，包含整个任务图，**不是本批API单独成本**；不与live gate的54,000 B（包含真机计数/诊断）混比。8/10只是资源测量，4槽/单目标准入不自动扩大。

证据：`build/task-facade-20260924/`（ASan、Win32、15项语法、候选profile/ZIP）及`build/task-facade-product-matrix-20260924/resource-report.json`。本轮无ADB操作、烧录/清授权、服务部署/数据库迁移或Git暂存/提交/推送；App/Service生产实现不变，仅同步文档。下一步按主线§6第16项做候选安装及新入口的最小COM3闭环，不能把前批旧入口实物算作本批新入口完成。共享operator、写额度实物、未知根恢复、其他平台和正式发布仍未完成。

### 10.43 第四十四批：已安装候选的公开入口与真实App闭环（2026-09-24）

本批只验收主线§6第16项，不改生产API/Engine/Session、App代码或Service协议；新增[封闭试点交接说明](DEVICE_V2_TASK_PILOT_HANDOFF.md)，避免让普通Arduino用户承担profile/Python/内部接线。

#### 候选安装与构建

- 原单库打包器以development `esp32-wifi-t2-lab-v1`生成匹配本机地址/测试签名公钥的ZIP，SHA-256为`7a3a839353bc3e1225b6bdaacfac2ec0d5b9d3b4e5a63c5dfa6b152ad6151dcd`。无DeviceKey/SSID/密码进入候选库，私有凭据只在忽略目录Sketch配置。
- 使用Arduino CLI 1.5.1、stock ESP32 3.3.11、PubSubClient 2.8。在空sketchbook以`lib install --zip-path`实际安装，未加`--library`指向工作区。安装后逐文件与ZIP一致；三个完整编译产物均验证实际使用安装目录，未引用仓库src/header。
- 普通ESP32正式WiFi示例（未启用任务）：971,832 B Flash / 52,288 B静态RAM；任务Engine/Session/Target符号不在最终链接内。
- XIAO ESP32S3 `CompileTaskWiFi`：984,948 B / 53,936 B；COM3实际`TaskPublicLive`：987,328 B / 53,976 B。两者均只有公开用户接线，任务/授时与原MQTT符号存在，无BLE/LAN/网关泄漏。普通ESP32与S3配置不同，**不能跨行相减作为开关增量**；不重复前批资源矩阵或外推峰值/耐久。
- 实测Sketch额外提供公开Property/Action及只读串口回调计数，无内部taskError、policy、NVS wrapper或隐藏配置入口；实际固件SHA-256为`313bb95c5c53b295d6467f500557a783b6891a6fb173e77817d18b4d2e188881`，COM3烧录及独立verify通过。

#### 真实用户/设备行为

1. 手机、电脑在“带翅膀的蜗牛”同路由网段。核对COM3 MAC `8c:bf:ea:8e:f1:2c`，先新读系统区并对整8MB基线做完整verify；不是仅凭旧文件存在就继续烧录。COM11/COM12未操作。
2. 沿用原已安装App（摘要核对），真实owner通过原页面创建专用设备；本机schema43先备份、不迁移。同一Gateway/Broker镜像仅更新本测试设备白名单，既有DeviceAuth代理临时映射本地地址，无新故障钩子或服务器实现。
3. 公开`enableTasks(power)`自动启用授时，MCU `begun=1/timeValid=1`。首次页面SCOPE_REQUIRED，显式初始化后得到4槽/单power认证目录。安装20秒倒计时后startSequence0、progress0、设备回调0，未偷偷Start。
4. 明确Start后startSequence1/progress1；到期前退出页面（activeDeviceLinks0）并真实force-stop App。App尚未重启时，串口记录power1、callbacks1、presses0；重启/重进同taskId认证查询progress3（Completed），原请求日志为空且无新Start。
5. 同一账号MQTT普通Action/Property ACK192/92ms；buttonPresses1、属性回调总数2，State与实际计数一致。属性请求为同值幂等写，不把它描述成第二次状态改变；观察连接关闭/释放通过。
6. 原owner删除返回202，cloudAuthorization=revoked、localTasks=retired_verified、列表移除/activeDeviceLinks0。12条真实结果用当前DeviceKey及原challenge独立HMAC核验，含一次Initialize、一次Create、一次Start及74 B Retired证明（count0），没有新建ID重复提交。Read保留策略可回收旧查询，不把12条误当全部历史调用次数。
7. `brokerCleanupPending=true`、`localAccess=not_confirmed`仍是独立未确认项，任务退休不替代它们。没有为门禁写业务DB、改账号分片/ACL、开启跨片，未把MCU软件回调当作外接执行器物理动作证明。

#### 收尾与范围

- COM3整8MB原固件/授权已恢复并独立verify，摘要`592e6fb8741acc9502fd1a8fd61a0685c17f624948760a836ce18a6feba393ee`；本机Gateway/Broker原镜像及环境变量逐项对比一致、健康，临时代理已删除，备份保留。测试设备按正常流程删除，历史证据不抹除。
- 串口观察脚本最初因本地`serial.py`命名遮蔽pyserial而未运行，改名`observe.py`后通过；失败时未发串口命令。只是忽略目录夹具问题，无产品修正或降级检查。
- 集中独立断言：`build/task-public-entry-live-20260924/verified-evidence.json`；安装/链接：`verified-build.json`；只读认证回执：`proof-after-task.json`、`proof-retired.json`。无新增APK构建、生产代码、软件回归计数或Git暂存/提交/推送。
- 本批补齐新入口安装/有限实物，不重复已过故障全集。下一步仅主线§6第17项最小可重建交接闭包；已分享operator缺同片条件，写额度实物/最大目录/其他平台/生产材料/未知根恢复仍未准入，T2正式整体不宣布关闭，不扩大维护或直接实现A1。

### 10.44 第四十五批：最小源码快照独立重建与安装（2026-09-24）

只完成主线§6第17项。用户仍安装一个Arduino库ZIP；新增维护者工具不进入该ZIP、不改变API/Engine/Session或三端协议，也不要求普通Arduino用户安装Python。

- `build_arduino_handoff.py`复用唯一发布器/profile，导出原库白名单、必要维护脚本、公开任务消费者、自测及PubSubClient 2.8源码/许可证。导出前原源码＋profile必须重建出参考ZIP；接收方拒绝缺失/修改/额外文件及路径越界，新输出目录不覆盖已有内容。哈希清单只保完整性，不是发布者签名。
- 独立目录`D:/codes/local-maintenance/task-handoff-20260924`，清单433项，manifest SHA-256：`4f3d50cc02cb4709156186f965b670e69f6e9ccff34f558a04c75ba3b56bb173`。此目录是冻结快照；工作文档后续更新不回写清单。
- 用快照自身脚本从快照源码重建，ZIP SHA-256为`7a3a839353bc3e1225b6bdaacfac2ec0d5b9d3b4e5a63c5dfa6b152ad6151dcd`，与§10.43实物候选逐字节一致。产物位于`D:/codes/local-maintenance/task-rebuilt-20260924/artifacts/`。
- 原安装门禁新增显式`esp32-wifi-tasks`目标，普通七目标默认矩阵不变。使用快照内脚本/例程/PubSubClient及外部CLI1.5.1、stock3.3.11，在新的sketchbook安装并完整编译：**984,948 B Flash / 53,936 B静态RAM**。安装门禁503.2秒通过；不是复用已编译固件或工作区`--library`。
- 库字节与ZIP一致、PubSubClient逐文件一致；编译源/include不含原仓库源码和个人Arduino目录，只使用隔离库/快照Sketch/预装SDK。任务/授时符号存在，BLE/LAN/网关等非目标图不带入；实际依赖只有BlinkerDeviceV2/Network/WiFi/PubSubClient。`build/task-handoff-20260924/verified-evidence.json`记录独立复核及摘要。
- 新交接自测9/9在原目录和导出目录各通过，不累计为18种；打包19/19、架构/profile检查通过。未重复累计前批70项核心测试，也未声称本批新硬件实测或固件ELF逐字节一致。
- 旧联调生成器依赖已失效的配置文本替换，且重复发布职责；已退出现役入口，保留`build/task-handoff-20260924/retired/build_device_v2_interop_package.py.retired`可恢复备份。当前文档指向原`build_arduino_release.py --profile`，历史实物结果不改写。
- 两现有测试账号只读核对仍为shard0/1，已分享operator实物继续待测；无账号/ACL变化、硬件/ADB操作、迁移部署、App/Service生产代码或Git暂存/提交/推送。

**退出边界**：最小设备源码交接已完成；SDK/CLI仅列预装版本、没有随包或全文件锁定，完整CI/三端独立构建/生产材料未完成。封闭owner试点不是正式T2整体准入。下一步只做主线§6第18项A1最小设计和原模块复核，不新增云执行器，不将缺环境项抹除。

## 11. dev_edu 与开源参考取舍

审查日期：2026-09-22。旧库依据本地 `origin/dev_edu` 的 `aa1fd8c3c32eec737714961ea3bb86bd085956b2`，未切换/修改旧分支；外部项目依据下列官方文档及源码，本轮没有引入依赖。外部主分支链接可能变化，实际复制源码前需另行固定版本、检查许可证和目标板资源。

### 11.1 旧实现应该作为业务基线，不是直接移植的底座

旧库已有倒计时、双阶段循环（两段时长/动作与循环次数）、按星期/时刻执行，以及 EEPROM 保存/启动加载；并非只有 ESP32，`BlinkerTimer` 已有 Renesas Ticker 适配。但业务接线受 WiFi/Widget 条件编译约束，不能由此推断旧 BLE 路径已经支持同样任务管理。来源：[BlinkerTimer](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Widgets/BlinkerTimer.h)、[BlinkerApi](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Blinker/BlinkerApi.h)。

值得保留的设计：

- 用户配置后由设备本地执行，不依赖 App 一直打开；但仍须有可用时间来源。
- 定时回调主要置触发标志，主循环 `checkTimer()` 消费并执行动作。新核心直接采用有界 cooperative `poll()`，无需每条任务占一个硬件定时器。
- 星期位图、一天内分钟数、启用/重复状态可以紧凑表达；多个同一时刻任务也必须分别处理。
- 普通控制与定时控制共用实际动作路径。V2 复用属性校验/回调，不复活旧 `parse(action, true)` JSON 执行入口。

静态源码确认的问题，不能随移植带回来：

| 旧实现 | 风险/新方案取舍 |
| --- | --- |
| `BlinkerTimingTimer::freshTimer()` 再次 malloc 后直接覆盖 `actionData`，类也没有释放该指针的析构函数 | 重复编辑/删除存在泄漏；新表采用固定容量、自有标量值，不保存动态 JSON 动作字符串。见[源码](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Widgets/BlinkerTimingTimer.h#L54) |
| `_cdAction` 等固定数组的长度检查使用 `length > capacity`，随后 `strcpy` | 恰好等于容量时没有给 NUL 留空间；新 codec 必须校验精确字节长度。见[检查入口](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Blinker/BlinkerApi.h#L1780) |
| `loadCountdown()` 恢复运行态时按 `_cdTime1` 总时长启动，而不是根据已保存 `_cdTime2` 还原剩余期限 | 不是可靠的断电倒计时续跑；新首版明确重启 Interrupted，需用户重新 Start，不偷偷重开完整倒计时。见[加载逻辑](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Blinker/BlinkerApi.h#L1156) |
| 倒计时先保存停用再 parse；日历任务先 parse 再经后续逻辑停用/保存 | 执行与持久消费顺序不统一，后者存在动作后掉电重复窗口；新方案统一 Claim→回调→结果，不能据此承诺物理副作用 exactly-once。见[checkTimer](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Blinker/BlinkerApi.h#L2355) |
| 固定 EEPROM 偏移、任务代码中的 put/commit，缺少本方案要求的持久提交失败回执和执行进度合同 | 不能把调用 commit 当成整个业务事务成功；存储通过独立 TaskStore/平台原子端口接线，不直接复制偏移 |

容量也不能只比较记录字节：旧 `BLINKER_TIMING_TIMER_SIZE=10`，单个日历记录是 4 B 配置加 60 B 动作区；这不包含完整运行时成本，也不具备本方案的稳定目标 key、owner scope 和持久执行进度。新格式较大的部分须说明用途并实测，不能笼统宣称新方案一定更省 RAM/Flash。来源：[旧容量定义](https://github.com/blinker-iot/blinker-library/blob/aa1fd8c3c32eec737714961ea3bb86bd085956b2/src/Blinker/BlinkerConfig.h#L1546)。

首版与旧功能的差异必须列入验收：双阶段循环尚未纳入、动作收窄为显式允许的标量属性赋值、倒计时重启策略改变、最大容量尚待测。不得将首版标为“旧定时功能已全部替代”。复用 App 星期选择器时显式转换星期编号/位图，不直接复制旧二进制布局。

### 11.2 开源方案分别解决哪一层

| 参考 | 值得借鉴 | 本项目不直接照搬的部分 |
| --- | --- | --- |
| [BlynkTimer](https://docs.blynk.io/en/blynk-library-firmware-api/blynk-timer)、[源码](https://github.com/Blynk-Technologies/blynk-library/blob/master/src/utility/BlynkTimer.cpp) | `setInterval`/`run` 易理解；loop 阻塞漏掉多个周期后只回调一次，避免突发追赶 | 这是程序内周期回调，不等于可持久化的用户日历任务；采样层可以参考，不能拿它代替 TaskStore/权限/掉电语义 |
| [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)、[调度源码](https://github.com/arkhipenko/TaskScheduler/blob/master/src/TaskScheduler.h) | cooperative 调度，启停/有限次数，明确区分周期与不追赶策略；平台依赖少 | 不为少量用户任务引入整个多任务框架；它本身不是本项目的持久任务/所有权协议 |
| [Tasmota Timers](https://tasmota.github.io/docs/Timers/)、[执行源码](https://github.com/arendst/Tasmota/blob/development/tasmota/tasmota_xdrv_driver/xdrv_09_timers.ino) | 更接近当前需求：用户配置时间、星期、重复、输出与动作，由设备本地执行；有限任务表 | 借鉴业务模型，不复制整个固件、JSON 命令解析或固定输出编号；首版不加日出日落、随机窗口、toggle/脚本 |
| [ESPHome Time](https://esphome.io/components/time/)、[触发源码](https://github.com/esphome/esphome/blob/dev/esphome/components/time/automation.cpp) | 时间来源与触发器分离；先检查时间有效性；明确校时/时区边界 | 源码对一定范围内漏过的秒会补查触发，不能原样套用本方案的迟到跳过策略；也不引入完整 cron/YAML 工具链 |
| [TimeAlarms](https://github.com/PaulStoffregen/TimeAlarms)、[源码](https://github.com/PaulStoffregen/TimeAlarms/blob/master/TimeAlarms.cpp) | daily/weekly/once 的简单调用模型 | 它的 interval timer 也基于 `now()`；我们的倒计时须依赖单调时钟，不能随 UTC 校正漂移；未解决持久存储/管理授权 |
| [littlefs](https://github.com/littlefs-project/littlefs)、[存储设计](https://github.com/littlefs-project/littlefs/blob/master/DESIGN.md) | 掉电一致性、提交边界与磨损均衡要一起设计；底层 sync 必须真正落盘 | 它是存储后端，不是定时器；首版优先现有原子 blob，不为 4 条测试任务强制所有 MCU 引入文件系统，更不能挂载失败就自动格式化 |

以上是设计参考，不代表已在 ESP32/R4/Wio 对这些库完成资源、耐久性或功能对比实测。

### 11.3 本项目最终取舍及不扩散边界

1. **开发者回调与用户任务分开**：周期采样继续使用现有 onSample/运行循环；如果未来确需 Sketch Timer API，再单独评估薄适配，不要求持久化函数指针或每秒写 Flash。
2. **旧 Blinker/Tasmota 提供业务基线，轻量调度库提供调度思路**：保留 App 配置后本机执行；核心以固定容量表和有界 poll 实现，不按每条任务创建线程/硬件 timer。
3. **持久化复用现有端口，不能省略业务状态**：一个原子快照保存定义、必要去重与执行进度；不复制旧 EEPROM 布局，不引入第二套凭据/账号库，不承诺重启后自动补跑。
4. **测试优先覆盖旧实现暴露的边界**：连续编辑/删除无泄漏、最大长度、同刻多任务、校时前后跳、执行前后掉电、重启倒计时、容量 4/8/10。旧 JSON 路径只在消费者切换后删除。

参考核对不改变第 10 节的优先级；游标和快照底座已推进，当前下一步以该节及总主线为准。没有新增外部定时依赖，也不因开源项目功能丰富而扩展协议。
