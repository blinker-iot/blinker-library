# B2a 普通 WiFi owner 移除真机记录

2026-09-12。同日后续；归属 [B2 实施合同](DEVICE_V2_OWNER_REMOVAL_IMPLEMENTATION_PLAN.md)。本批完成**普通 WiFi、无分享临时设备的 owner 移除主路径**：真实 Android App → 本机 Gateway/原 Broker → COM11 XIAO ESP32S3。不是 BLE/网关本地解绑，也不是全部发布故障矩阵完成。

## 1. 环境与隔离

- 电脑仍在“带翅膀的蜗牛”，服务地址 192.168.2.76；使用原手机 debug 包、原 owner 账号及原账户 MQTT，没有新账号/伪造令牌/写数据库代替 API。
- COM11 基准 MAC 尾号 96:14，仅用于核对 USB 板；**MAC 不作为逻辑设备 ID**。通过 App 原 HttpClient（含原鉴权拦截器）公开创建 `B2a Temporary Removal COM11`，取得专用新逻辑 ID 和 Key。COM3 原 WiFi 见证与 COM12 LAN 基线的记录/固件/凭据不改。
- 原 COM11 系统区 64 KB、app0 3,342,336 B 均以 ROM read 完整备份。核对原分区与 app0 启动选择后，仅写 0x10000 应用区，写入 hash 校验通过；不擦 NVS/改分区。测试后恢复原 app0 备份，见收尾。
- 本机 Gateway 基于原运行镜像只覆盖 B2a 的 5 个编译模块（index、management、repository、invalidation-worker、removal），原 compose 参数/数据库/三个 Broker 保留，恢复 healthy。跨 shard requests/shares 保持关闭，本轮同 owner/same shard 不需打开。
- 手机保留数据安装修订 debug APK，生成制品使用本机 HTTPS/测试 CA，并保持既有内部 LAN 开启；三个源码 environment 的 LAN 默认仍 false。首次误启动同机 release 包看到登录页；实际 debug 包原账号仍在。随后补齐 debug 网络 CA overlay，云连接 ready；未重新注册、未清手机数据。

WiFiQuickStart 的配置副本只替换 Key/网络/本机地址并使用公开 `Blinker.debug(Serial)`；本机 TLS 注入短期构建时间，不冒充 B3 授时。loop 仍只有 Blinker.run，不新增自动上报生产循环。SDK ESP32 3.3.11，XIAO/default_8MB：程序 1,034,552 B，静态 RAM 51,864 B；不是默认明文/原样 ZIP 验收。

## 2. 真机结果

| 检查 | 结果 |
| --- | --- |
| COM11 新 Key → session → MQTT | active、Broker 实际 online，真实 Manifest/State |
| 删除前 Property / button Action | ACK 190 / 177 ms，power false→true，buttonPresses 0→1 |
| App 原设备设置确认处理器 | 先校验临时名称/owner/精确 ID/保护列表，显示本地未清除警告，再调用原 management DELETE |
| 首次 DELETE | 202，cloudAuthorization=revoked，brokerCleanupPending=true |
| 请求发起 → Broker 持久注销确认 | 217 ms；权威事务提交后 120 ms；一次 attempt |
| App 回设备列表 / 清展示 Key | 428 ms；目标不在库存，保护设备仍在 |
| 重复 DELETE | 200、两项 pending=false，deletedAt 不变；没有新的注销任务/版本递增 |
| 目标云端状态 | PG device deleted、原 session revoked；Broker device revoked、mqtt_online=0 |
| COM11 硬复位后 | 原临时设备仍离线，无新增有效 session，后续观察未重新激活 |
| 旧 locator + 原 MCU instance 的正式 challenge | HTTP 403、wire DeviceRevoked(3106)、AuthorizationRequired(4) |
| 本地权限 | 200/202 始终 not_confirmed；未声称 Flash 本地授权已清除 |

删除在 2026-09-12 17:13:25（本机时间）执行。本批旧 Key 检查经过原生产 CBOR codec 和正式合同头；第一次测试请求漏 `X-Blinker-Contract` 得到 400，属于工具错误，不计为凭据撤销通过；补齐后按协议断言精确 403/3106/4，通过。没有直接伪造 SessionCredential 或旧 MQTT CONNECT，也没有把 server revoke 记录冒充设备签名 ACK。

## 3. 第二物理设备与原账户隔离

COM3 使用同一 App 账户 MQTT 的原 DeviceV2Service，有界观察 State，连续 Property ACK 后间隔 1 s；不新增账户连接。一次最多 24 次、总期限 120 s；Action 仅尝试一次，不自动重放未知结果。

- 24 次 Property ACK：删除发起前已完成 9 次，1 次与删除窗口交叠，App 删除完成后发起并完成 14 次。
- Property ACK 样本最小 55、中位（上中位）223、最大 333 ms；样本少，不宣称 P95 或全路径无缝。
- 25 条 State 观察，power 实际交替；随后 button Action ACK 166 ms，计数 2→3。
- 所有样本同一个原账户 MQTT Session，始终 ready；COM3/COM12 均保持 active/online，没有被临时设备删除波及。

## 4. 工程改动与余项

新增 `tests/hardware/owner_removal_app_gate.mjs`：只操作明确命名的新临时设备，调用原页面确认处理器/管理服务，核对回执及库存；保留操作 checkpoint，失败/超时不自动再次删除。所有 service 拦截都在 finally 恢复；不读取账户/设备秘密到输出。

复用 `cloud_witness_app_gate.mjs` 并修复测试工具的“关闭后仍不能切换到另一设备”限制：只有已 abort 的旧观察可被新的 open 替换，不能关闭/写入另一个仍在使用的设备。删除单测 3 项＋见证单测 6 项共 **9/9**。App local 构建、Android debug 构建/安装、Gateway 构建通过；上一批 App64/PG41/删除16 的软件结果是前批证据，不重复计成本批运行。

下一步进入 **B2b 的本地退休/再次认领前置审计与最小接线**，优先阻止旧 BLE intent/receipt 恢复已 deleted 的逻辑设备，再区分 online root retirement、离线 physical reset、共享成员退出；接口在证据齐全前仍明确 409。B1 默认明文/单 ZIP/其余板矩阵继续保留；B3/B4、不确定 Action 的无线重放、带分享 owner 删除/真实 Broker 故障重启等发布组合不能由本批一台普通 WiFi 通过替代。不转向 Hub/S4 或百轮测试。

## 5. 证据与收尾

Ignored `build/b2-removal-live-20260912/`：summary、App 时序、只读 PG/Broker 观察、编译/备份/烧录/恢复日志及原部署检查点。包含私有 DeviceKey、网络参数、CA/Flash 备份；不得暂存或推送。

- 测试固件 SHA-256：`0ce6b14af4305a944770eab8fae3f00350bb48b8c707a4fb7e63ece757494c23`。
- 安装的 debug APK SHA-256：`8dc2c27c706f001e9014c34a2abc42d1e7d4c4b3aed5e512968b9bf1b279f5ce`。
- COM11 原 app0 备份 SHA-256：`cb9664a034863ce062b85be3105ead5422d9955fa6acc7002403485eb098423c`。

临时云端设备已终态移除，旧 Key 不可恢复；tombstone/审计保留，若以后使用需新建设备/新 Key。本批未删除原 COM3/COM12 或原 COM11 对应云端记录。COM11 原 app0 完整恢复并通过写入 hash 校验，不回写 NVS 旧快照，避免回滚安全计数；本轮未重新验收原网关业务。最后手机为原 owner、/home/device、MQTT ready、LAN caller 空闲；COM3/COM12 active/online。没有本批暂存、提交或推送。
