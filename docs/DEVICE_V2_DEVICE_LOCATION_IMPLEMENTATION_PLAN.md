# Device V2 设备定位实施方案

更新日期：2026-10-29。当前进度：LOC-0／LOC-1软件、LOC-2私有Cloud、LOC-3／LOC-4 App按已记范围有限收口；LOC-5统一Sketch定位输入、有限host及LOC-5b隔离安装／原SDK完整链接完成；LOC-5c 同库交付说明和可选 CI 接线在 §20 收口。
云端接线与原 SDK S3 完整链接取 §10～§11；COM11 非卫星测试输入及 App 页面生命周期结果取 §12～§13。
匹配 beta 已保留，已恢复已验四槽 WiFi BIN 及本批 NVS／授权，临时定位准入／背景均为 `[]`。
网络参考源码准入默认关闭，云端按§16保留匹配Gateway／只读月库及COM11单设备名单；原GNSS／背景仍0/0。
匹配beta已按§17接显式只读入口并完成真实页面短验；不再重复城市样本／构建。§18按用户明确边界，只提供统一设备观测输入，不实现或验收用户GNSS驱动。

## 1 目标与现状

支持有 GNSS 能力的设备提供自己的最近位置，并为没有 GNSS 的联网设备提供可选 IP 粗定位。
复用 BBP、现有认证／队列／Cloud 租约和地图组件，不另建 MQTT 连接、硬件采集线程或定位上传器。

方案起点（不是当前未完成清单；后续实现与结果见§6～§14）：

- Android one-shot 已接系统网络定位和 GNSS，并非仅 GPS；手机位置仍只是用户可修改、确认保存的草稿。
- App／Gateway `/location` 和迁移 057 只支持 `WGS84 + source: manual`，owner-only、独立 revision/CAS。
  它是安装位置，不是设备上报接口，不能让 GNSS／IP 修改它的 revision。
- BBP 已支持 Object，SDK 有 `setEncoded` 和现有 State／Telemetry 载体；没有设备定位语义／codec。
  多 Property 的本地原子提交可能在网络发送时分包，不能据此把独立经纬度拼成正式定位样本。
- Telemetry 已按 `consumer.fields` 筛选，不是聚合数据全量广播。真正的权限缺口是定位字段识别／Open 授权，
  以及普通 State／历史／Direct 的潜在旁路；现有分享 `canRead` 不等于允许读取精确位置。
- H2 当前接纳数值型实时只读属性，使用服务器收件时间；不是定位 Object 或 GNSS 轨迹合同。

本方案不升级 SDK、不扩任务槽，不改变 R5 冻结候选的支持范围；不重复地图／手机定位、BLE 或断电矩阵。
新源码是后续可选能力，不能把 R5 旧 ZIP 的摘要或编译结果当作它的完整验证。

## 2 三类位置与权限

| 位置 | 来源／用途 | 更新及权限 |
| --- | --- | --- |
| 安装位置 | 用户手动设置，或确认手机／设备取点后保存 | 保留当前 owner-only 与独立 CAS，不自动覆盖 |
| 设备最近位置 | 认证设备声明的 GNSS 观测 | 单独 latest 投影；首版 owner-only，保留来源、精度、时间／过期信息 |
| 网络粗位置 | 认证成功设备会话的可信公网来源 IP | 独立粗略参考；不冒充 GPS，不据此合并设备身份或覆盖安装位置 |

设备当前位置是只读观测，拖动地图仅编辑安装位置。GNSS 失效后保留最后有效点并标过期，
不悄悄用 IP 替换成看似精确的新坐标。查看设备位置不要求手机定位权限。
source 是设备声明／采集来源，不是物理真实性证明或新的授权依据。

## 3 最小架构

```text
用户 GNSS 驱动／解析库
          ↓ 新观测、真实采样时刻／年龄
可选 LocationFix + 单快照 + 有界 CBOR codec
          ↓ LOC-1 隐私合同完成后才接线
现有认证 BBP／Cloud 租约 → owner-only latest 投影 → App 现有地图

认证 HTTPS session → 可信来源 IP → 服务端 IP-provider → 独立粗位置
```

### 3.1 SDK 边界

- 平台无关、显式 include；不由 `Blinker.h` 或普通产品默认实例化，不新增普通 `.cpp`，保留 17 TU 布局。
- 无 heap／线程／UART／NMEA 必装依赖，不阻塞等待首个 fix。硬件驱动由 Sketch 或后续可选适配提供。
- 同一观测包含经纬度与质量元数据；不以属性名 `gps`／`lat`／`lon` 猜语义。
- LOC-0 只实现值、校验、编码与单快照，不提供公开 `report()` 便利绑定、Endpoint／Manifest 注册或发送路径。
  等服务端权限门禁完成后才接，不能让“组件可选”代替数据权限隔离。

### 3.2 最近位置／租约

- 后台需求显式 opt-in，初始建议 60 秒；页面请求可为 1～5 秒，受设备 floor 与原有租约限制。
- 后台 latest 消费者与前台需求聚合到同一 Cloud 租约；不伪装 App 会话或借用 archive 名称，也不同时再建 SDK 周期上传器。
- 页面退出／后台撤销前台需求，异常退出靠 TTL 收回高频；没有后台需求则停止网络上报。
  GNSS 模块仍可供用户其他业务使用，不能因退出 App 就擅自停掉硬件。
- 拥塞只保留最近观测、不积累轨迹队列；不把发送成功当云端持久化 ACK。离线轨迹／补传另立阶段。

### 3.3 服务／App／IP 边界

- 定位能力与 endpoint 映射需明确版本合同。首版不进入普通可分享 State、H1/H2 或原始通道；
  若复用已有数据面，必须先验证共享用户不能请求／读取位置，而不只是地图页隐藏。
- latest 保存认证设备身份、会话／序号和服务器 receivedAt，处理迟到点／重启 fencing；不按 MAC 或 IP 当主键。
- App 复用地图只更新 marker；当前位置只读、安装位置编辑分开，不逐点重建地图或强制平移视口。
- IP 优先走已认证 HTTPS session 的可信代理来源，准确限定代理 allowlist／覆盖写入头；
  当前源码仅取 socket peer，不能直接把代理地址当设备公网地址。本批未测云端真实 IP 链。
- IP-provider 放服务端，按需、限额／超时／有界缓存，失败不影响登录、控制和 GNSS；
  不让 MCU 查询公网 IP／携带地图 Key，不信任任意 XFF 或设备自报 IP。
  纯 BLE 子设备只能显示注明来源的网关网络粗位置，不推断为子设备精确位置。

## 4 执行顺序与门禁

| 阶段 | 最小范围 | 完成条件 |
| --- | --- | --- |
| LOC-0，本批已完成有限验证 | SDK 设备 GNSS 值、固定 codec、单快照 | 有效性、边界、时间未知、坏输入、过期／失锁、乱序／回绕、资源上限通过；无网络接线 |
| LOC-1，软件有限验证完成 | 定位能力映射、私有 latest 摄入／查询端口 | 当前 Cloud 来源／租约后置摄入，owner-only；manual CAS 不变；普通 State／Open／历史无定位旁路；真实私有流待 LOC-2 |
| LOC-2，受控 Cloud 实物有限收口 | SDK 薄绑定和现 Cloud 租约消费者 | 软件／原 SDK 完整链接及 §12 显式非卫星注入短差异通过；准入已撤回，非真实 GNSS 采集证明 |
| LOC-3，有限收口 | App 最近位置只读展示 | 模型／观察器／页面回归及 §13 非卫星测试点、后台／离页 Close、返回新租约短验；不是实物 GNSS 驱动验收 |
| LOC-4，a／b有限收口 | 可信IP元数据、本地城市provider／owner API | §14～§15来源、官方固定月库、软件／真实PG临时表通过；尚未部署／验实际设备IP或接App，不改安装位置 |

IP-provider 授权／数据源选择不阻塞 LOC-0～LOC-3。轨迹、地理围栏、离线存储／Bulk、国内手机 SDK、
GNSS UTC 自动授时和分享位置新权限后置。各阶段只补变化项，不重新铺一轮已通过矩阵。

## 5 LOC-0 数据合同

### 5.1 值与单快照

- 本批设备来源固定 GNSS；手机、manual、IP 不通过此设备 codec 冒充 GNSS。
- `LocationFix` 为显式有效的完整观测；默认无效。WGS84 E7 `int32_t`：纬度 ±900000000，经度 ±1800000000。
  `(0,0)` 是合法坐标，不作无 fix sentinel。量化分辨率不代表实际精度。
- 水平精度以 cm 表示，使用显式已知标记；未知不是 0。UTC 使用明确未知／GNSS 声明质量，未知时不携带假 UTC。
  已知 UTC 沿用公开业务时间 2000..2100 范围，但不调用 WallClock 同步，更不改变安全凭据时钟。
- `LocationSample` 同时携带真实 sample age。快照接收本 boot 的采样 tick 和当前 tick，拒绝未来、过旧与乱序点；
  不把收到旧 NMEA 的时间伪装为采样时间。
- 单快照保留最后有效 fix，另行失锁／过期；读取失败不修改输出，新坏观测不能覆盖旧有效点。
  明确失锁由调用方 `invalidate()`，不是用默认无效值覆盖。默认本地新鲜窗口为 30 秒，可显式配置，
  不等于网络上传周期。不持久化、不跨 boot 排序；按序接收的采样 tick 间隔须小于 2^31 ms。
  重启、tick 复位、未测量睡眠或无法确认采样 tick 连续性时显式 clear，不能声称检测不可观测的整圈回绕。
  一旦读取过期／坏 tick，锁存不可读，保留最后有效点；后续 tick 绕回也不能自动复活，只有新成功观测或 clear 能改变状态。

### 5.2 有界编码

复用现有 CBOR Reader／Writer、Result 和 Span。schema 1 为固定整数键 Map，WGS84 由版本确定：

| 键 | 内容 |
| --- | --- |
| 0 | schema version = 1 |
| 1 | device source = GNSS（1） |
| 2／3 | 纬度／经度 E7 |
| 4 | 水平精度 cm，未知为 null |
| 5 | 采样 UTC ms，未知为 null |
| 6 | UTC 质量：unknown（0）／GNSS 声明（1） |
| 7 | 编码时的 sample age ms，限定在本 boot 的半回绕范围内 |

Decoder 固定 pair 数及键顺序，拒绝重复、缺项、未知版本／来源、越界、非 canonical、截断和尾随数据；
先解到临时值，全部成功才替换输出。Encoder 失败不交付部分值／改写调用者输出，使用固定上限的临时缓冲。
codec 只承载有效 GNSS 点；未获取／失锁不编码为 `(0,0)`。上层无样本／过期投影在 LOC-1／2 接线时验证，
不把本批拒绝编码无效点当作已实现网络失锁通知。
定位 payload 大小、对象 RAM 和有限验证结果见 §6，不将它与含 BBP／MQTT／TLS 开销的整包混为一谈。

## 6 本批执行记录

### 6.1 已实现的最小模块

- [LocationFix.h](../src/BlinkerV2/location/LocationFix.h)：WGS84 E7、显式有效性、可空精度／UTC 与质量校验。
- [LocationCodec.h](../src/BlinkerV2/location/LocationCodec.h)：复用原 CBOR 的固定 schema，严格键序与原子输出；
  仅成功后将固定缓冲复制给调用方，不增加传输或普通 ValueType。
- [LocationSnapshot.h](../src/BlinkerV2/location/LocationSnapshot.h)：调用方持有的单快照、新鲜度／顺序校验、失锁保留及显式清理。
  独立审阅发现的旧点整圈回绕复活风险已用过期／坏 tick 锁存修正，并补回归，不加新的时钟框架。

三个头仅显式包含，无 heap、默认单例、线程、驱动、NMEA 依赖、上传定时器或设备持久化。
源码检查确认普通入口／实现 TU 未引用新模块，仍为 17 个普通 `.cpp`。

### 6.2 有限验证与资源结果

| 验证 | 结果／边界 |
| --- | --- |
| 本机定向 host 构建 | 原 MSVC 19.44，Debug `/W4 /WX`，只构建新 `blinker_location_tests` 及其原 portable core 依赖，通过 |
| 定向 CTest | 1/1 目标通过，含 7 组数据／codec／快照合同；未执行整个旧测试集 |
| 编码边界 | 固定零点未知质量 golden 为 17 B；最坏坐标／精度／UTC／age 为 41 B；所有更小缓冲失败且原输出不变 |
| 坏输入 | 全部截断前缀、尾随、重复／乱序键、缺项、非 canonical、类型／坐标／UTC／年龄越界、null与质量不一致均拒绝且不污染输出 |
| 快照 | 失锁保留、旧值／重复拒绝、真实年龄、过期／坏 tick 锁存及后续整圈不复活、合法回绕和显式 reset 通过 |
| 内存 | 本机 `LocationFix=24 B`、`LocationSample=32 B`、`LocationSnapshot=40 B`；测试期间 new/new[] 无新增 |
| 原 MCU 工具链窄检查 | 现有 Xtensa esp-x32 2601 与 ARM GCC 7-2017q4，以 C++11、无异常／RTTI、严格警告通过新三头语法及静态对象上限检查 |
| 原架构门禁 | `tests/architecture/check_device_v2.py` 与 diff-check 通过；没有新增普通 TU 或自动网络入口 |

MCU 项仅为头部语法检查，不是 ESP32／R4／Wio 新 SDK 全量链接、资源峰值或实物 GNSS 采样证明。
41 B 是定位值本身，不含 BBP、MQTT、TLS、认证或传输分片开销；未更改根 CA、签发 ring 或业务时间／安全时钟。

证据留在忽略目录 `build/location-loc0-20261028`：定向 CMake／CTest 产物、LastTest.log 和三头 probe。
可复用的源码回归为 [test_location_main.cpp](../tests/host/test_location_main.cpp)，已接原 host CMake／CTest，
后续 CI 可按原 host-quality 路径执行，不新增另一套测试执行器。

### 6.3 LOC-0 收口时的停点

LOC-0 按该批范围收口，当时下一批为 LOC-1：先明确定位能力与 endpoint 映射及私有 latest 数据权限，
覆盖设备认证、owner／share、manual CAS、Open 字段请求、State／H1／H2／Direct／原始通道旁路，再实施服务接线。
不先给 SDK 加自动 `report()` 或给 App 地图接普通属性；后台租约与实际定位上报在 LOC-2 验收。

LOC-0 当时没有设备位置网络注册、服务／App 接线或部署；后续 LOC-1 服务端进度以 §7 为准。
当前仍不能称为设备 GNSS 已可在地图使用，模拟样本不等于真实卫星取点。

R5 冻结 ZIP 及原证据原样保留；新 SDK 源码会改变后续候选身份，不能无条件沿用旧包作为新功能证明。
不清授权、TaskStore、UNKNOWN 或日志，不提交推送，不做无必要备份。

## 7 LOC-1 定位映射与私有最近位置（2026-10-28）

### 7.1 本批最小决策

不扩 Manifest wire schema，不按字段名猜语义。现有字段解码严格限定 key 0～11，贸然增加新 key
会牵动 SDK／App 和指纹兼容；本批采用维护者显式、精确的内部绑定：

- Broker `DEVICE_V2_LOCATION_BINDINGS=[]` 默认关闭，最多 128 项、JSON 最多 64 KiB。
- 每项仅 `logicalDeviceId`、64 位小写十六进制 `manifestFingerprint`、`endpointId`、`codec: device-gnss-v1`。
  设备 ID 是认证逻辑身份，不是 MAC／IP；每设备只绑定一个定位 endpoint。
- endpoint 必须是原有只读、可实时采样 Object Property：`kind=0,type=7,access=1/5`，floor 为 100～60000 ms。
  可任意命名；配置重复／多余键、指纹改变、endpoint 类型／权限不符均拒绝，不降级成普通 Object。
- 本批没有把该配置写入运行环境，也未创建私有消费者，因此不会凭配置或 GET 查询自动开启定位上报。
  这不是面向任意 Sketch 的自动能力发现，也不扩大 R5 发行平台／连接模式范围。

实现位于服务仓库 `services/broker/src/protocol/v2/location-policy.ts`；原无绑定设备的 State 读取
保持原路径，不为每次普通状态读取额外查询或解析 Manifest。

### 7.2 摄入与隐私边界

复用现有 Cloud Telemetry 入口；新增 latest 投影 hook 在聚合器完成当前 stream／epoch／Manifest、
租约、字段集合和递增序号检查之后运行。Broker 调用聚合器前先比较实际连接的 claims 对象，
而非仅认证 session ID，旧 socket 不能推进新连接的序号／epoch 或拒绝其采样流。

存储事务再次检查 active 设备、tenant、当前 credential/session、Manifest 与绑定 endpoint；
身份来自认证会话，不从定位 payload 获取。LOC-0 的 17～41 B 严格 CBOR 合同原样复用。
错误只增加有界失败计数，不记录坐标／原始 CBOR，也不回滚已接受的普通实时数据或控制链路。

隐私门禁为数据面限制，不是只隐藏 App 页面：

- StatePage／Patch 在进入状态缓存或通知前拒绝定位 entry，数据库写入再检查一次。
  旧状态读取删除完整 entry（包括 raw CBOR），绑定设备缺 Manifest 时 fail-closed。
- 普通 Telemetry Open／Renew 不允许定位 endpoint，所有者、分享与 Console 通用消费者都不能借此读取。
  owner 地图需求将在 LOC-2 走独立授权的私有需求接口，不能绕回普通 Open。
- H1 原有标量类型 0～5、H2 原有数值只读门槛不接纳定位 Object；不另写轨迹表或扩展历史字段。
- Console、AppRelay 与普通 fanout 的 lane 通知只取 Manifest／State／Patch／Event，
  Telemetry 仍经原 consumer 字段筛选；已检查本次新增字段没有接入这些原始通知。
- 安装位置 `/location`、迁移 057、owner-only 和 revision/CAS 不改；最近位置不能修改其 revision。

本批仅建立 Cloud 服务端门禁。Direct／LAN／网关子设备定位未接线、未验收，
不能把服务端检查外推为已阻断嵌入式本地直连；LOC-2 必须先限定 Cloud-only 绑定。
SDK 的定位 provider 也不得对该 endpoint 调用普通 `setEncoded()`／初始化 State slot；
当前 StatePage 可跳过未初始化值并保持原 cursor／totalFields，已用真实编码帧验证不会阻塞就绪。

### 7.3 最近位置存储与读取合同

Broker 在原 SQLite 初始化路径新增独立 `v2_latest_location`，每设备一行、全局最多 128 行，
不驱逐其他设备，不进入普通 State、H1/H2 或 PostgreSQL 安装位置表。只保存最后有效点，
不保存轨迹；同段检查序号和 stream／epoch／session 组合，收件时钟倒退或样本 age 超过 90 秒不覆盖。
新段只能由已验证的当前 Cloud 租约入口交付，不能由 payload 自认。

新增 owner-only `GET /api/v2/devices/:id/location/latest`，复用 App 原鉴权及 shard 客户端：

- 读前、读后均核对 owner、canRead、有效云凭据、tenant、路由版本／shard 及授权版本；有效分享也不可读。
- 无 query／body、无自动重试，`private, no-store`；最多 16 笔并发，Broker 上游 6 秒、16 KiB 上限并关联取消。
- 返回 `logicalDeviceId`、可空 `capability{codec,endpointId,manifestFingerprint}`、
  `source: device-gnss`、`coordinateSystem: WGS84`、可空 `sample`、`ageMillis` 和 `fresh`。
- `sample` 仅含 E7 经纬度、可空精度 cm、可空采样 UTC ms、UTC 质量、收件时样本 age 和服务器 receivedAt。
  不返回内部 session／segment／stream、raw CBOR、token 或设备 Key。
- 没有点时 sample／age 为 null、fresh=false；有旧点则保留并标过期。
  新鲜度上界为收件后实际年龄加设备报告年龄 ≤90 秒，不把 receivedAt 冒充采样时间。
- 最近收到不等于在线：还须是当前连接、有效会话和新鲜普通 State。
  内存来源 fence 最多 128 项，仅成功摄入后记录当前 claims 对象；断线／State 重同步失效，
  Broker 重启为空。同 token 重连也必须收到本连接的新点才能恢复 fresh，不能“复活”旧点。
- 指纹切换、云凭据换代、设备退休清理相应 latest；不删除请求账本、授权历史或 UNKNOWN。
  latest 是可重新采集的投影，不承诺跨 shard 迁移或离线恢复轨迹。

内部查询仅接受原 shard Bearer 鉴权与单一 tenantId，不能用调用方 account ID 代替设备 owner tenant。
实现分为 Broker `location.ts`／`location-source.ts` 和 Gateway `location-latest.js`／`location-latest-http.js`，
不另建认证、MQTT 连接或地图 provider。

### 7.4 有限软件验证

沿用现有 Node 22.22.3 与已安装依赖；PATH Node 24 的原生 SQLite ABI 不匹配，
本批未换依赖、重建原生库或升级 SDK。仅使用新建临时 SQLite／内存 fixture、mock 和 loopback，
未连接运行数据库／账号服务、发送验证码或制造实际设备记录。

| 项目 | 结果／边界 |
| --- | --- |
| 新定位配置／策略 | 9/9：默认关闭、精确映射、配置有界、重复键拒绝、State 整 entry 剔除、普通 Open 拒绝 |
| 新 codec／latest 存储 | 9/9：SDK golden／41 B 上界、坏输入、age／乱序、跨 tenant／指纹、128 行硬上限 |
| 新 Broker 接线 | 9/9：认证 scope、凭据换代／退休、真实 StatePage／Patch、H1/H2、内部 GET、连接代际／重连、租约后置 hook／错误隔离 |
| 新 Gateway latest | 9/9：owner／分享、读后撤权／路由变化、严格响应／并发上限和真实 loopback 取消，无重试 |
| 受影响的原 Telemetry 回归 | 原 App control＋session 41/41：租约复用、Close／过期、队列／字段筛选及旧回调 fencing |
| 原手动位置有限回归 | 单元／HTTP 11/11；2 项 PostgreSQL 测试未运行，不访问业务库，不重做已通过的迁移矩阵 |
| 原工程检查 | Broker TypeScript 构建、Gateway noEmit、JS 语法及 diff-check 通过 |

新增回归已接 Broker `test:location`／`test:device-v2` 和 Gateway 原 `test:location`，
复用现有构建与 Node test runner；没有另开测试框架或硬件编译队列。
本批不是 App 地图／真实 GNSS 上报验收，也未部署、刷板、打新 ZIP 或提交推送。

### 7.5 下一批唯一入口：LOC-2

按以下顺序补当前真实缺口，不再重复 LOC-0／1 已通过项：

1. ESP32 Cloud-only SDK 薄绑定：复用可选 Snapshot／codec，提供现 Telemetry 采样 provider，
   定位值不落普通 State slot；未知／失锁／过期不得编码假零点。先保留明确的能力与准入边界。
2. 现 Cloud 聚合器加入命名明确的 private latest 需求，维护者显式 opt-in；
   背景低频与 owner 页面前台需求合并到同一物理租约，不借用 App／archive 身份。
   GET latest 本身仍只读，不隐式续租；前台控制需独立 owner 授权与当前路由复核。
3. 有限回归优先验证字段并集／容量、离页撤销、TTL 降频／停止、失锁保留过期和 source fencing，
   再按匹配配置做一次受限 Cloud 真机差异验收，保留原控制 ACK／State。
4. 通过后进入 LOC-3，复用地图作只读 marker 更新和来源／精度／时间／过期展示。
   手动安装位置仍独立编辑和确认，设备上报不自动保存；不要求手机定位权限。

IP 定位、轨迹／地理围栏、离线存储／Bulk、GNSS 授时、分享位置新权限、Direct／LAN／新平台适配继续后置。

## 8 LOC-2 私有 Cloud 需求与 SDK 薄绑定

### 8.1 本批最小合同

沿用当前源码与原 SDK，只做 ESP32 TLS Cloud 候选的软件接线，不连接或部署到服务器，不修改公开发行包身份。
先完成下列接线和定向验证；地图 UI／真机部署仍是后续门禁，不以模拟采样冒充实际 GNSS 验收。

- 背景需求另由 `DEVICE_V2_LOCATION_BACKGROUND_DEVICE_IDS=[]` 显式开启，ID 必须已精确绑定，
  最多 128 项，固定 60000 ms。绑定本身和 GET latest 都不会启用背景采样。
- 前台独立 `POST /api/v2/devices/:id/location/lease`，仅 owner；严格 Open／Renew／Close，
  客户端为每个页面生成新的 UUIDv4 leaseId，固定 30 秒上限，单设备最多一个前台租约。
  Open 请求间隔 1000～5000 ms，并尊重设备 floor；floor 超过 5 秒时拒绝前台高频请求，不谎报已达到频率。
  响应 active 仅代表需求已受理，intervalMs 是已接受的需求间隔，不是设备 Opened ACK、真实采样频率或持久化确认。
- Renew／Close 固定页面 nonce、owner account／tenant、授权版本和路由版本；旧页面不能关闭或续租新页面。
  取消在 Open 前到达时保留短期关闭标记，迟到的同 nonce Open 不得启动采样；不重放或自动重试。
- Gateway 副作用前后复核原 owner 权限／云凭据／路由，取消或后查失败仅尽力关闭该精确 nonce，
  使用独立短截止的一笔清理，不改成另一条 Open／Renew。网络未知结果依靠有界租约，不宣称已收到设备停机 ACK。
- Broker 租约元数据同时使用墙钟和单调时钟期限。采样来源必须是当前物理 Cloud 连接、active
  credential、Manifest 与未冻结 tenant；不接 Hub、虚拟子设备、BLE／LAN。
  支持候选仍固定 ESP32 TLS profile；现 Broker 位于 TLS 终止代理之后，session 表没有 transport_security，
  不能伪称从该表或设备自报字段证明实际加密，也不能把代理后的 socket 当公网 Plain。
  后续部署门禁需核对既有 TLS 入口与冻结 profile，本批不增加新传输安全证明协议或开放 Plain 定位支持。
  owner 撤权后的接口立即拒绝，设备凭据撤销／退休阻断来源；前台高频需求本身最多残留当前 30 秒 TTL，
  不宣称已新增跨进程 owner 权限即时推送机制。

### 8.2 合并、停止与无 fix

后台、前台、普通实时和 H2 仍共用现有物理 Telemetry 租约及控制队列，不新增连接、独立上传器或定位采样线程。
定位是明确的私有需求角色，不伪装 App／archive；普通 App 和 H2 优先，容量／帧预算／floor 不兼容时
只暂停私有需求，不能让原实时或控制失效。

现协议每条聚合流只有一个 interval，因此背景定位 60 秒与普通实时 1 秒共存时，线上整包可以是 1 秒；
不能把“背景需求 60 秒”说成此时定位字段必定只传 60 秒一次。latest 投影仍按私有需求的频率选样。
退出位置页撤销的是它的高频需求，不停止其他合法消费者；无其他需求则 Close，有背景需求则恢复其低频。
正常关闭主动发原 Close，网络异常还需原设备租约到期兜底，不能承诺离线瞬时停发。

SDK 复用 Snapshot／codec 和原 Telemetry 采样器，必须按真实 RxContext 限制 Cloud 来源；
不能只看产品名称或由调用方声明“这是 Cloud”。定位绑定不初始化普通 State slot，绑定后普通 publish／setEncoded
也必须拒绝该 endpoint；绑定前已有 State 值则失败，不静默清除旧状态。

GNSS 无 fix／失锁／过期时不编码假零点、null 或新采样时间。只允许明确绑定的私有字段从本次 Telemetry
样本中缺省，不修改持久租约的字段集合；普通字段仍须完整，多余字段仍拒绝。
这样温度等普通数据继续发送，latest 保留最后有效点并按真实年龄过期，无定位字段不计投影故障。
定位独占且无 fix 时不发空 Data，不扩大 CBOR 协议，也不虚增 sent/drop 或形成热重试。

### 8.3 实际实现与资源

- SDK 新增显式 include 的 `CloudLocationBinding.h`，复用已有 Snapshot／codec；先在同一 Client 声明
  只读实时 Object endpoint，再 attach，不创建上传定时器或 GNSS 驱动。Snapshot 须覆盖绑定生命周期，
  绑定须覆盖 Device／Client 生命周期；不能在 setup 的局部临时对象销毁后继续借用。
- Runtime 的可选采样前处理保持原 sampler 调用兼容，使用真实 RxContext 和 transport capabilities，
  仅接受 Cloud＋Authenticated、非 Local；复用现发送 envelope scratch 剪本，不修改租约 bitmap，
  不额外分配每租约 bitmap 或第二帧。原 `onSample` 接口、EndpointSlot 和 17 TU 布局不变。
- Client 统一拦截私有 endpoint 的普通 publish／setEncoded／StateUpdate；attach 已有 State 值、错误
  descriptor 或不足8对的 Object maxLength 均拒绝。服务端精确映射同步拒绝不容纳固定8键 codec 的约束。
- Broker `location-lease.ts` 只持有有界需求元数据，`location-demand-runtime.ts` 负责实际设备／会话
  loader 与 latest 摄入，原 `telemetry.ts` 负责唯一物理流；没有第二套传输执行器。
- Gateway lease 复用 `location-access.js` 的 owner／路由检查，latest 与 lease 不复制权限规则。
  已修正已知400／403／404／409／429拒绝后的误清理：这些无副作用结果不发送 Close，
  避免重复 Open 的409关闭原合法租约；未知或副作用后检查失败才精确清理一次。
- Console 自动选择排除已绑定定位字段，再请求普通实时，避免被整体拒绝后连普通字段也无法观察。

实际资源并非“可选所以零成本”：通用 Runtime 函数指针＋Client 借用指针在32位共新增8 B，
普通实例也承担该固定开销，静态上限已据实调整，不增加容量。
原 Xtensa 默认 Small probe：`Runtime=404 B, Client=696 B, Device=1692 B, EndpointSlot=8 B`；
可选 `CloudLocationBinding=56 B`（host为72 B），内部定位缓存41 B，无 new/new[]。
这些是静态尺寸／软件证明，不是完整 ESP32 峰值 RAM、任务栈、真实驱动或长时间联网证明。

### 8.4 本批有限验证结果

| 项目 | 结果／边界 |
| --- | --- |
| 新私有聚合 | 10/10：字段并集、App/H2优先、帧／容量／floor隔离、背景60s选样、前台TTL降频／停止、晚Open、失锁混合缺省、生命周期及错误隔离 |
| 新租约与实际loader | 9/9：严格命令、wall／monotonic、128上限、精确nonce／关闭标记、会话来源、真实SQLite latest摄入、内部HTTP只读GET及Console兼容 |
| 新Gateway lease | 10/10：owner／撤权／路由、精确独立清理、不重试／UNKNOWN、已知拒绝保留原租约、请求／并发上限、真实loopback客户端 |
| 新SDK host | 原CMake定向完整链接；CTest `blinker_cloud_location_tests` 1/1，6组通过，含真实Command→ACK／StatePage、原子StateUpdate拒绝、失锁恢复、Local及Cloud\|Local隔离、TTL／Close停止；new/new[]=0 |
| 原工具链窄检查 | 原Xtensa2601，C++11／无异常／RTTI／严格警告，新头object／尺寸及两个受影响TU语法通过；不是MCU全量产品链接 |
| 受影响兼容项 | 原Telemetry archive＋session 52/52；共用owner helper影响的latest 3项、旧hook断言及新增Object约束定向检查通过；未重跑LOC-0／1全套或旧硬件矩阵 |
| 原工程门禁 | Broker TypeScript构建、Gateway noEmit、JS语法、architecture与diff-check通过 |

回归接回原 Broker／Gateway `test:location` 和 host CMake／CTest，没有新测试框架。
SDK证据复用忽略目录 `build/location-loc0-20261028` 的新增 host target、LastTest.log 和 cloud-header-probe；
目录名沿用旧构建树，不代表拿旧结果替代 LOC-2。
服务端只使用临时SQLite／内存fixture／loopback，不访问运行库、不制造实际账号／请求或授权记录。

本批软件接线收口，未部署、刷板、重新构建发行ZIP或提交推送。App源码、账号、授权、UNKNOWN、
请求日志与运行环境保持原样；新服务器访问材料不进入源码、profile、文档或测试证据。

### 8.5 下一步最小停点

1. 只对匹配 LOC-2 绑定的 ESP32 TLS候选做一次原SDK完整链接，冻结本次 provider、Manifest指纹／endpoint和公开profile。
   不复用R5旧ZIP摘要冒称新源码已完整链接，不再跑其他平台或旧矩阵。
2. 获准部署时先核对届时真实服务入口与TLS终止配置，沿用新环境，不沿用旧IP；再做一次有限 Cloud 差异：
   新点→latest、普通实时并存、失锁、前台释放／TTL和背景恢复、原控制ACK／新鲜State。
   本轮“暂不部署”的约束未变，未连接新服务器或操作现场设备。
3. 上述门禁通过后才进入 LOC-3，只接现地图的只读设备marker与私有页租约生命周期，
   不触发手机取点、不改变安装位置CAS，不扩展IP、轨迹、离线存储、Bulk或分享位置权限。

## 9 LOC-2 原 SDK 完整链接与候选冻结（2026-10-28）

### 9.1 本批完成范围

完成 §8.5 第1项，仅一个实际消费定位绑定的 ESP32 TLS 内部探针。不部署、不刷板，
不创建账号、设备、准入或租约，不调用服务器、发送验证码或修改运行配置。
普通公开例程及七目标安装矩阵未扩展，也不把本批称为定位功能已正式发布。

新增内部 Sketch 为 `tests/arduino/CompileCloudLocation`，不是公开 examples：

- 显式复用原 Device／Product／WiFi lifecycle、业务授时和 `cloud-beta.json`；ESP32、TLS、task0、无LAN编译门禁。
- 普通 power 保留控制与初始 State，uptimeSeconds 仍由真实 millis() 按需采样。
- GNSS 只接真实观测／失锁同步输入端口，默认没有 fix；不模拟坐标，不初始化定位 State，
  不创建第二个定时器或上传执行器。Snapshot／Binding／TimeSync覆盖Device生命周期。
- 本次 Key、SSID、密码均为公开占位，不是可直接刷入现场使用的固件；后续私有参数或实际板型变化
  必须产生自己的匹配产物／摘要，不能将这个通用ESP32 BIN刷到XIAO ESP32S3。

本批未改生产 SDK 执行链。新增 Schema／probe、内部安装门禁开关和必要有限检查，保留此前源码及无关本地修改。

### 9.2 唯一编译队列与脚本复用

复用 `tools/build_arduino_release.py` 一次生成新公开profile库ZIP，仍只派生原4个配置／身份头；
然后复用 `tests/arduino/verify_installed_release.py` 在空 sketchbook安装、逐文件核对并完整链接。
没有调用默认ESP32仍为3.3.6的 `compile_matrix.py`，没有拿旧R5包或普通无定位WiFi链接代替新证据。

安装门禁新增显式 `--location-candidate`：只能配单一 `--target esp32-cloud-smoke`，
保留原public TARGETS不变，实际消费外部 `CompileCloudLocation` Sketch。
该开关检查原SDK3.3.11、jobs=1/2和受限public profile，再复用原安装字节、依赖、来源、可选图与平台闭包检查；
另要求真正保留 `CloudLocationBinding::prepare/write`、Client attach及采样预处理符号，不能只include新头。

本次实际输入／结果：

| 项目 | 冻结结果 |
| --- | --- |
| Arduino CLI／SDK | 1.5.1；Arduino-ESP32 3.3.11，实际hardware路径在 `D:/arduino-esp32-3.3.11/data` |
| 实际编译器 | 原 `esp-x32/2601/bin/xtensa-esp32-elf-g++` |
| 唯一板型／连接／任务 | `esp32:esp32:esp32`；手动WiFi／HTTPS+MQTTS；task0 |
| 外部依赖 | 隔离复制并核对 PubSubClient 2.8；WiFi／Network／NetworkClientSecure来自原SDK |
| 编译命令／库源 | 94条实际命令，17个库实现TU均来自本批隔离安装目录，无主仓src头／源码旁路 |
| 新完整链接 | exit=0；编译243.2秒、完整安装门禁248.8秒；没有第二次MCU队列 |
| 程序／静态RAM | CLI报告1,079,860 B／53,040 B；不是运行时heap峰值或任务栈实测 |

CLI输出仍将目标标为 `esp32-cloud-smoke`，必须同时记明 `--location-candidate` 和实际 Sketch，
不能写成普通CloudSmoke例程重复验收、两个公开目标全矩阵或XIAO真机已通过。

### 9.3 权威 Manifest 与精确输入

Sketch与新增host probe共用 `Schema.h`，后者只复用生产 `EndpointRegistry::seal → computeManifestFingerprint → encodeManifest`，
不需要假transport、begin或联网，也未改golden框架来手算／漏掉实时floor。
本次仅构建并执行新 `blinker_cloud_location_manifest_probe`，没有重跑旧host测试。
对其规范Manifest字节做独立SHA-256核对，结果与生产函数一致。

| 字段 | 值 |
| --- | --- |
| revision／字段数 | 1／3，声明顺序为power、uptimeSeconds、position |
| 定位endpoint | id=3、key=position；只读Object、1000ms floor、maxLength=8 |
| codec／坐标系 | device-gnss-v1／WGS84 E7 |
| Manifest fingerprint | `ed2e47c19b210a924dbc991e5842ab08df495a5202780d031f2828485e3f3499` |
| Schema SHA-256 | `5bb8f47664ce720053ccec278989dd113fdc3da745bbe78c7afbc8c4622419a8` |
| Sketch SHA-256 | `9fad0ca2526415f0c8dffbab350e0295074e47702f3685d8dcc851145ffa4470` |
| public profile SHA-256 | `b4ef0cb36a7fdc654b71bfc4b1ab47850c0a58819f6559e028f67bf89b7bcd30` |

以上非秘密合同保存为 `tests/arduino/CompileCloudLocation/candidate.json`，不含logicalDeviceId或真实凭据，
不会自行写入Broker名单。安装门禁对冻结源码／profile摘要、规范Manifest散列及原4个派生头做只读核对，
以后输入有变须重新明确候选，不能静默沿用旧指纹。
Arduino复制的Schema仅增加其自动 `#line` 指令，去除该指令后与共享源完全一致；原始文件摘要因此不混用。

冻结输入检查是在本次唯一队列已启动后补入脚本，已对本批实际安装独立只读补验通过，
没有为补检查再编译。新增发布合同检查4/4通过，先前已通过的release／LOC-0～2用例和硬件矩阵未重复运行。

### 9.4 新包与链接产物身份

仅留在本机忽略目录 `build/location-loc2-link-20261028`，不提交构建产物或私有备份。

| 对象 | 大小／SHA-256 |
| --- | --- |
| 新库ZIP | 447文件、744,125 B；`0da8f1f2cfaa760890cd48c9b23290a2840d486cafecbe2583429838818474c6` |
| 内容身份头 | `db37d40310b153021e514dd5c44295765b43b0ecf77a0dbea9898f34bc55f60e` |
| ELF | 15,267,088 B；`18764685dd35a482990ce21369983ff64d19b0ecea56700e460ae91a19646772` |
| 应用BIN | 1,080,016 B；`0e627fbcf50c027a4e57197e6f002573afbd9f0d2d69c787d9e27e3d77a83a71` |
| map | 15,871,630 B；`abc3cc02685f6b47bcf7bb473af97b0f9bb59b50a028285e736b3115423abe14` |

BIN大小包含镜像头／对齐等，与CLI报告的程序存储用量不同，不将二者当成同一指标。
新ZIP比旧443文件包增加4个可选定位头；内部Sketch／Schema／host probe不被塞进公开examples。
库／固件版本仍为0.1.0／device-v2-0.1.0，以本批精确摘要区分内测候选；R5旧包与摘要继续原样保留为历史证据。

### 9.5 当前停点

§8.5第1项收口。下一步仅在获准部署后核对届时真实TLS入口／域名与新服务器环境，
按实际测试板型和私有参数产生自己的匹配候选，做一次有限Cloud差异，保留原授权和请求日志。
本批没有连接新服务器、发送业务请求、刷板或部署，也没有提交／推送。
Cloud实物门禁前不接LOC-3地图，不借本次通用ESP32完整链接扩大BLE、LAN、网关、R4/Wio、持久任务或IP定位范围。

## 10 LOC-2 部署前离线接线与观测输入门禁

### 10.1 为什么本批没有重复真机／编译

本轮仍要求“暂不部署”。已通过的原SDK链接不重跑，不能借“下一步”擅自部署Gateway／Broker或刷板。
当前没有匹配LOC-2运行环境的有效记录；普通health只表示存储／shard健康，不包含足以证明LOC-2的源码／镜像身份。
本批未连接服务器，不能推测新服务器已经匹配，也不提前进入地图。

只读核对发现两个实际接线缺口，故在允许范围内先修正部署模板：beta Nginx原来只代理安装位置 `/location`，
没有最新点／租约两路；Compose也没有透传定位绑定／背景名单。它们不解决就无法进行预期的公网Cloud短验。

### 10.2 已补的最小接线

服务仓库复用原 beta 编排／Nginx渲染入口，新增：

- `deploy/beta/compose.location.yaml`／`.env.location.example`：只有两个Broker环境变量增量，默认 `[]`；
  base和原overlays不变，不添加服务、端口、挂载、任务签名或新传输连接。
- `check-location-config.cjs`：独立、有界的静态stdin检查，复用生产纯解析。
  无候选只接受关闭状态；显式公开候选文件则必须恰好一个匹配ID，背景名单为该绑定子集。
  核对冻结 ESP32 Cloud／SDK3.3.11／task0／Manifest与endpoint 3，不在生产运行时硬编码候选指纹。
  最大stdin2 MiB、候选16 KiB，仅输出计数／固定错误，不输出设备ID、env或凭据。
- 原背景解析函数移至 `location-policy.ts` 供运行／离线检查共同使用；原runtime导出保留兼容，
  没有另写一套定位JSON规则，也不改变实际租约或采样行为。
- 原任务checker恢复原样并保持独立，定位task0不被迫开启任务／提供签名材料；
  既有已启用任务、自动化、地图仍须保留各自配置和检查，不能借定位降级或扩大当前实例。
- 原 `render-nginx.cjs` 新增严格布尔 `locationTelemetryEnabled`，默认关闭且三份原输出字节不变。
  true仅放 `/api/v2/devices/:id/location/latest`（GET／OPTIONS）及 `/location/lease`（POST／OPTIONS），
  id限制1～128、完整路径锚定，拒query、禁止代理重试／错误拦截，private/no-store和no-referrer。
  owner鉴权继续由现Gateway前后复核，安装位置CAS、stream、共享proxy、地图与其他公开能力不变。

真实设备ID与填好的env仍放运维私有目录；公开候选元数据来自 §9 冻结输入。
配置检查仅验证声明，不是设备授权、TLS真实证明、发布者签名或已部署证据。
操作说明及保留旧overlays的离线示例在服务仓库 `deploy/beta/README.md`，本批未执行这些Docker命令。

### 10.3 有限验证

| 项目 | 本批结果 |
| --- | --- |
| 新定位Nginx配置回归 | 4/4：严格开关与缺省字节、两精确路由／方法／query、与原能力共存、CRLF／缺marker拒绝 |
| 新定位编排／checker回归 | 9/9：两变量overlay、默认关闭、单候选绑定／子集、版本／指纹拒绝、有界输入／文件、脱敏CLI及原任务独立 |
| 原Broker定向TypeScript构建 | 通过，更新移动后纯解析导出；未重装依赖或升级SDK |
| 独立只读审阅 | 配置默认关闭、身份／名单边界、Nginx精确路径和旧功能兼容通过 |

测试只使用内存已渲染fixture、临时公开JSON及Node子进程，不调用Docker／Nginx／数据库／服务器。
没有重跑LOC-0～2、旧硬件矩阵或开启第二条MCU编译队列；静态测试不等于真实合并Compose或宿主 `nginx -t`。
没有部署、reload、刷板、改运行配置、制造观测记录、发送验证码或提交推送。

### 10.4 不能忽略的观测输入

当前 `CompileCloudLocation` 仅提供 `acceptDeviceGnssObservation(...)`／`deviceGnssFixLost()` 两个同步端口，
没有GNSS驱动或串口解析，也没有其他代码调用它们。默认Snapshot无fix是正确行为，
仅靠上电不能完成“新点→latest”。本批不为补样本自行填入坐标，不新增生产source类型或假卫星证明。

未来一次Cloud短验须明确二选一：真实GNSS数据源，或用户明确允许的限定测试观测。
后者必须在证据中标为“注入、非卫星”，只证明codec／上传／租约／latest，不证明GNSS采集能力。
如果需要修改Sketch接入输入，须记录新源码／实际板型／私有参数产物的匹配身份，不能沿用 §9 旧BIN摘要。

### 10.5 唯一下一步与停止条件

下一步需要有限部署／刷板授权，以及明确的观测来源。届时只核对匹配源码与镜像、实际合并配置／TLS入口，
保留原授权／UNKNOWN／日志，完成一次新增链路的短差异；随后才进入LOC-3地图。

如果下一轮仍要求不部署，则当前主线保持在此门禁，不再用重复编译、审计、模拟记录或新工具替代真机验收，
也不绕过顺序提前扩展地图、IP、离线轨迹、Bulk、分享位置或其他平台。

## 11 LOC-2 云端受限发布与 S3 候选（2026-10-28）

### 11.1 本轮范围与授权

本轮允许必要部署／真机操作；受控观测获准仅用于 COM11，必须标明“注入、非卫星”，不作为 GNSS 定位验收。
用户随后明确现四槽任务也是测试，可覆盖并要求通过 ADB 操作；本轮仍不清 TaskStore／授权／UNKNOWN，
不重放任务、不发送验证码、不刷新 Key，不改其他板或已通过硬件矩阵。

实际新服务器为 `129.204.34.159`，两个域名均解析到它。沿原公开 profile 的 E46 CA 单独验证
`iot.yiyu.pro:443`／`mqtt.yiyu.pro:8883` 成功，主机名验证及 CERT_REQUIRED 保持；
ZeroSSL 中间证书名称变化不等于根 CA 不匹配，未改成裸 IP、系统 CA 泛信任或关闭 TLS。
SSH 首次接触后固定主机公钥；这是 TOFU，不冒称已有独立外部指纹认证。

### 11.2 实际运行基线与最小修正

新环境已迁移为现有 `blinker-iot-beta` 四容器和规范化 `compose.runtime.json`，不能拿仓库 base-only 重建。
只读比对实际运行源码、原 Node22.23.3、依赖及迁移后确认：

- Gateway 干净隔离编译仅有 2 个运行 JS 修改、5 个定位 JS 新增；Broker 为 7 改／5 新，无删除。
  contracts 及其余执行 JS 与旧运行镜像相同；Gateway 216／Broker 85 个源文件与冻结上下文逐一匹配。
- PostgreSQL 已应用 1～57；本次不新增迁移。两个 package.json 只有测试脚本不同，
  name／version／入口及全部依赖语义保持；迁移源归档的原锁文件与本批相同。
- 复用 `freezeBuildContext` 时补齐两个确实被原 build 使用的资源：Gateway skills 复制脚本和 UI agent rslib 配置。
  精确白名单纳入总摘要，拒绝链接文件／父目录，有限回归 2/2 通过；不运行旧维护脚本 main。
- 首次完整镜像构建在 better-sqlite3 原生回退时因缺构建工具停止。Dockerfile 仅 build 阶段补
  python3／make／g++，不带入运行阶段；有限合同回归 1/1 通过，锁文件、依赖与 SDK 未变。
  修正后的同一队列在 Debian 索引传输停滞约 9 分钟后有据取消，没有反复拉取或宣称完整 npm 构建成功。

实际发布采用 **compiled-runtime-delta**：本机原 Node22／原 tsc 的干净输出与冻结源匹配，
在原不可变 Linux 镜像上只复制以上 7／12 个运行 JS 及 12 个对应 Broker map。
没有复制 Windows node_modules、改 native ABI、升级 Node／SDK、夹带旧 dist 或扩依赖；
保留原 Linux 依赖，镜像标签和 RootFS 前缀独立核对。该方式不是完整依赖重建的替代证明。

| 证据 | 本轮值 |
| --- | --- |
| 原锁文件 SHA-256 | `15c78b9dafe4b90b90ff13a155d3f2313dd79c2c194291081351f2dc036de30c` |
| 修正后 367 文件冻结源摘要 | `ec440f030226c604c8bd34e9911dd15b0d6c20e5427ca2179356618338d6231e` |
| 实际 Gateway 镜像 | `sha256:ad97057e7c8f7bd562ef2d3e15b0b69677e03a96f6c38dd184a7c835b6d1c4a8` |
| 实际 Broker 镜像 | `sha256:c65007840cd808cc35619cbcb11d64e597cbdfee9bccd4e131831cb327bb2bc8` |

### 11.3 部署与只读入口核验

保留原 project、规范化配置、env 数组、所有既有用途密钥／挂载／卷／端口和旧镜像。
CAS 核对配置摘要与容器代际，只替换两镜像及 Broker 两个定位配置 `[]`，单次仅重建 Gateway／Broker。
新旧环境在内存中逐项核对，防止 Compose 插值悄悄改变原 Auth 值；不输出秘密值。

结果：四容器健康，PostgreSQL／Redis ID、启动代际、挂载不变，迁移 57 项不变。
实际容器逐文件核验 Gateway 7 文件、Broker 24 文件；定位绑定 0／背景 0，无位置采样需求。

Nginx 沿现渲染器提取两个入口的纯增量，保留现活动记录／地图顺序和其他原文件字节。
第一份运维提取候选误包含地图块，被“必须恰好两个 route”检查在写入前拦截；日志保留。
改为渲染开关前后精确纯插入差异后，候选 SHA 为
`37f9dbf24591d040aadac7b285add71d12edbd06f0155380d49b63eeed663f94`。
原 API CAS、去除增量逐字节还原、真实 `nginx -t` 通过，仅成功 reload 一次；stream／proxy 片段不变。

有限入口核验：

- latest GET、lease POST 未认证均 401，`private, no-store`；未开放通配或重试代理。
- 现真实 beta App／原账号的 latest GET 为 200，`capability=null`、`sample=null`、`fresh=false`、`ageMillis=null`；
  是关闭状态的空投影，不是已取得位置／设备 ACK，也没有为补样本造点。
- 未安装／重建 App；原账号及两类请求 journal 前后相同，保留完整本批请求证据，无验证码／Key 管理调用。

运维证据在本机忽略目录 `build/location-loc2-deploy-20261028` 及服务器受限目录
`/opt/blinker-location-loc2-20261028`；私有配置副本、日志及业务数据不得作为源码交付。

### 11.4 内部 S3 受控输入与完整链接

新增内部 `SerialObservation.h`，不改变冻结 ino／Schema／公开 profile，不进入公开 examples 或生产定位源合同。
只有显式 FIX／LOST 完整合法命令才调用现 Snapshot 端口；96 B 固定行缓冲，wrapper 每轮最多 32 B，
单调序号、16 笔上限、未知回执不重放。本地回执 `InjectedNonSatellite`，不自动生成／续造坐标；
注入点精度及 UTC 保持未知。新 host target 完整链接／运行 3 组通过，另经独立有限审阅。

单一私有队列使用实际 XIAO ESP32S3／default_8MB、原 SDK3.3.11、jobs1、task0／TLS／授时，
复用 447 文件隔离安装与原 PubSubClient；已保留的合法 Key／WiFi 仅进入 ACL 私有头，不放进命令行或日志。
完整链接 623.251 秒通过，程序 1,036,836 B／静态 RAM 52,144 B；17 TU／17 归档成员及 LOC2／时间／task0 闭包通过。
BIN 1,036,992 B，SHA `cd6726c729e9ef8b4ea5bfca022a3f7ccd4cf65c8d74801ee393d1652290daf3`。
Manifest 仍取 §9 指纹，未重跑通用 ESP32 编译或旧矩阵；静态 RAM 不是动态峰值证明。

### 11.5 此前硬件首停（已由 §12 授权解除，不再排队）

以下保留此前停止依据；当前进度及恢复结论以 §12 为准，不再重复全片备份或追溯未知主程序。

COM11 身份核对为 ESP32-S3，安全启动／Flash 加密关闭、8 MiB；未操作 COM9。
ROM 无 stub 全读在 420 秒读取约 77.7% 时达到运维截止，没有生成完整备份；
一次标准 RAM stub 读取在首 4 KiB 后断流。两者都没有 Flash 写入，失败日志不冒称有效备份。

为避免继续长全读，尝试仅用已有已验 canonical、mostfun 四槽 App 与新鲜小 NVS／分区表构造恢复源，
补齐最后擦除扇区后独立 ROM 全 8 MiB verify **不匹配**。因此不能把该组合称为当前精确备份，
也不能据历史烧录记录断言当前运行它；恢复源文件保留但未通过准入。

已停止所有刷写／注入，原主程序未改，已通过 ROM 只读 MAC 命令发出硬复位恢复原启动；
未宣称恢复后已云在线。新服务与入口保留，定位准入仍 `[]`，没有高频需求或测试位置记录。

唯一后续：明确 COM11 的恢复目标。可由用户允许使用已验四槽 WiFi BIN 恢复主程序、保留新鲜 NVS／授权／UNKNOWN，
不再要求追溯当前未知主程序；否则先保留当前程序，不继续刷写。目标明确后复用本批已链接候选，
只做一次无 fix→前台租约→显式测试观测→owner latest→Close／退出回收的差异并收尾，
实际影像／名单／请求均重新核对；不重复构建、验证码、任务操作或旧矩阵，通过前仍不进入 LOC-3。

本批没有提交／推送，没有启用背景采样、IP、轨迹或分享位置。

## 12 LOC-2 COM11 受控 Cloud 短差异收口（2026-10-28）

### 12.1 最小实施与实际分区合同

用户明确允许以推进进度为主，未知原主程序无法恢复也可接受。因此不再重复 §11 的全片读取，
保留新鲜小 NVS／分区表、授权及 UNKNOWN，以已验四槽 WiFi BIN 作为收尾目标；不清任务区或重放旧请求。
沿用 §11 已完整链接的 S3 候选、现 beta App／账号及云端镜像，不重编、安装 App、刷新 Key 或发送验证码。

写前发现现表为 `nvs@0x9000/0x5000 + factory@0x10000/0x500000`，编译表为同址 NVS、
`app0@0x10000/0x330000` 及其他 OTA／FS 分区；整表字节比较在写前拒绝，没有覆盖表。
按实际使用核对后，仅本批采用 NVS＋应用兼容合同：

- 芯片 S3／8 MiB、镜像校验及芯片最低版本匹配，应用起址均 `0x10000`，1,036,992 B 同时放得进两容量。
- 存储依实际表／`nvs` 标签查找，不按编译 CSV 硬编码其他区域；实际链接无 OTA 写入／启动切换、FS 或任务模块。
- 仅单次写精确候选应用 BIN，不写 bootloader、分区表、otadata、FS 或整片；写后应用／NVS／表独立核验通过。
  不将此内部合同扩成 R5 支持声明或“任意分区均兼容”。

实际启动并经认证 Cloud 登录，同一设备的 Manifest 与 §9 三字段指纹匹配；没有换账号／设备身份或 TaskStore scope。
Broker 只临时新增这个 ID 的精确 MF／endpoint3 绑定，背景始终 `[]`。启用／撤销均仅重建 Broker 一次，
带实际 Manifest／配置 CAS／持久意图；Gateway、PG、Redis 代际／挂载／原环境不变。

### 12.2 新增链路的有限结果

本批使用当前真实 beta App 的原用户鉴权调用现 API；不是 LOC-3 地图 UI 或完整页面生命周期验收。
仅一条 FIX 及一条 LOST，各发送一次，串口终态匹配后不重发；其本地回执不冒充 Cloud ACK。

| 范围 | 结果与边界 |
| --- | --- |
| 无 fix／私有能力 | 名单开启后 latest200，能力匹配，sample为空／fresh=false；没有默认坐标或空 Data 造点 |
| 单点本地输入 | FIX seq1与LOST seq2均精确回执 `InjectedNonSatellite/status0/error0`；没有新生产source或GNSS驱动 |
| Cloud 最近点 | 前台 Open200／active，接受1000ms、TTL30000ms；latest200/fresh=true，坐标与显式输入一致，WGS84，UTC／精度均null、timeQuality0 |
| TTL 兜底 | 首次显式Close在Open后41847ms，已经超过TTL，**只算TTL兜底**。最后接受点早于租约到期927ms，后续receivedAt不变，原点年龄到90429ms时fresh=false；不宣称MCU物理零包 |
| 提前 Close | 不再注入点，失锁后以全新页nonce在同进程顺序Open→Close，两HTTP200；服务时间显示78ms、早于30sTTL，active→closed/leaseMs0。仅证明租约API可提前撤销，不证明真点持续采样下78ms物理停包 |
| 普通 State 隔离 | 同一候选State200只含原power=false，无position；本批未发送业务Command或任务请求，不将只读State冒充新增Command ACK验收 |
| 撤销后 | 唯一本批绑定1→0、背景始终0；owner latest200回到capability=null/sample=null/fresh=false，无高频需求残留 |

LOST仅使本地 Snapshot 失效，不按“失锁”伪造新点或马上删云端旧点。旧点按真实年龄过期，
上表 TTL／提前Close分开记录；请求间工具时延不被塞成设备采样／物理停机性能。

### 12.3 收尾与独立终核

先成功撤销本批准入，再恢复已验 mostfun 四槽 WiFi BIN：
SHA `e8a3c4c026ca84af0b2bcbf8142b18c63c2540253d260cfb2d59689199adda13`。
应用、原 NVS／授权及分区表独立 verify通过，已发正常启动复位；**不是未知原程序或原整片8MiB的精确恢复**。
随后真实 owner Manifest200已更新为该已验固件的两字段指纹
`e428a5be2f5da3a875c058a5907bb2e9f9ba5649cba6461fe18d855df5486a0b`，更新时间晚于恢复操作，证明已重新接入 Cloud。
Broker 只读查询确认原 Manifest 生命周期已清除此 ID 的测试最近点，剩余位置行0；没有手工删库／造日志。

原 App 不安装／清数据／切号，账号仍登录；两类完整 intent 记录前后相同，SHA 均
`bf9d4ab2543f573181376b318961a72d431233320a203b324f0d964782f2faa3`。
另对这两个数据库的**全部 object store**做前后深比较，同样未变，完整快照摘要均为
`33c37551cfdbc065f650874990018a6b8d9e6438b1a8caad868e5c5661e64ec4`，取 `full-journal-preservation.json`。
串口／CDP转发已释放；现有云端两镜像、Nginx精确入口保留，健康／未认证拒绝及准入0/0终核通过。
健康工具只证明运行配置，不夹带它无法观测的“未刷板／未注入”硬件结论。

独立只读终核确认本批点相等／时间精度未知、TTL与提前Close分界、普通State隔离、准入撤销及已验BIN恢复的有限结论。
原始请求／串口／恢复证明在忽略目录 `build/location-loc2-com11-hardware-20261028`、
候选在 `build/location-loc2-s3-20261028`；云端准入证据沿 §11 的运维目录。
`final-account-and-serial-summary.json`提供根线程的前后账号／journal与两次本地终态投影，
不把子审阅未读取的会话材料说成其已独立复核；这些私有材料不随源码交付。

### 12.4 唯一下一步

LOC-2 **显式非卫星注入的私有 Cloud 租约／上传／最近点及普通 State 隔离**有限收口。
下一批进入 LOC-3：复用现鉴权、租约和地图组件，只接设备最近位置的只读消费、未知／过期提示及页面资源回收，
与安装位置的手动确认／CAS分开，不自动覆盖、不触发手机定位，不扩IP／轨迹／Bulk、分享位置或其他硬件。
实际页离开／后台触发Close、真点持续采样下的取消及真GNSS输入仍须在相应阶段取得证据，不能借本批API提前关闭代替。

本轮没有改产品执行链、升级 SDK、扩槽、重跑 BLE／H2／断电矩阵或提交推送；只复用已验候选和有限运维接线。

## 13 LOC-3 App 最近位置与页面生命周期（2026-10-29，有限收口）

### 13.1 最小接线

本批只在现设备位置页增加明确的“安装位置／设备定位”只读来源切换，默认仍为安装位置。
仅 owner、显式设备定位页可见时读取最新点并申请已有私有租约；无能力不开租约，无fix不造标记。
设备上报点不经过manual草稿／CAS，不自动存成安装位置，也不启动手机定位。

分工保持三层：严格的latest／lease模型与现鉴权HTTP薄服务；独立可测的有界页面观察器；现地图／页面展示。
初始观察需求及串行GET均5秒，同nonce每10秒续租、服务TTL30秒；失败后停止并需手动刷新，
不将regular poll当失败重试、不用active受理结果冒充设备ACK。后台／离页立即取消请求、清timer及一次独立有界Close；
换账号／撤权清除私有点、禁用新身份做旧清理，失去Close机会则沿服务TTL收回。

沿现地图仅更新marker，设备模式不逐点平移／重建；首次显示或显式来源切换可定位视口。
保留来源／精度未知、采集时间与服务接收时间的区别、旧点／失鲜提示。地图失败门槛与位置读取失败门槛各自保持，
不在后台返回时偷偷重试失败。复用现云端代码／镜像；仅临时变更一个精确名单并同镜像重建Broker，
不重建Gateway、PG、Redis，不修改原授权／UNKNOWN。

顺序：有限模型／HTTP、观察器及现页面／地图差异回归→一次匹配beta构建与必要安装→
有条件时复用COM11已验候选、以明确的非卫星测试输入补实际页面Close／后台／返回与marker差异，结束撤准入并还原已验BIN／NVS。
不重复GNSS采集、MAP-4／手机定位、旧控制／任务矩阵，以下结果仅来自本次页面短差异。

### 13.2 代码与有限软件验证

App沿用现鉴权拦截器，新增严格的latest／lease模型与薄HTTP服务，复用统一路由判断；
有界观察器独立处理串行读取、同nonce续租、一次Close、取消／超时及账号／权限边界，页面不另写请求执行链。
401／403及实际latest／lease隐藏404清除私有点；普通服务失败保留旧点但锁住自动请求，需手动刷新。
安装位置保留原record／草稿／revision路径，设备来源没有编辑／手机取点／保存入口。
现地图仅增加默认保持旧行为的`followPoint`输入；设备模式关闭跟随，后续坐标只更新marker。

| 有限检查 | 实际结果 |
| --- | --- |
| 新模型／HTTP服务 | 45/45通过：严格字段、坐标／精度／UTC未知语义、响应容量、鉴权及取消／截止、无自动重试 |
| 观察器 | 初始15通过；权限／清理增量7通过、14跳过；5秒节拍增量1通过、20跳过；实际隐藏404增量2通过、21跳过。最终23项，不把累计执行次数称为独立用例数，也未重复最终全量运行 |
| 地图／原生命周期／页面组合 | 17＋3＋49项中首轮68通过；一项离页测试仍查已被正确移除的map组件，修正测试断言后仅该项1通过、48跳过，未更改产品行为或重跑整组 |
| 唯一匹配构建 | 现锁文件、原SDK与原生依赖不变；Web构建、cap copy、离线beta完整打包通过，未执行cap sync覆盖用户原生修改 |
| 包身份与资产 | `iot.diandeng.tech.beta`、原签名／HTTPS注册表通过；APK内1153份Web资产逐字节匹配冻结树，2126份源码／锁文件／原dirty原生设置前后不变 |
| 独立源码审阅 | owner-only失权清点、失败后台不重试、manual隔离、取消与marker更新无阻断发现；不代替实物GNSS证据 |

本次APK为76,518,128 B，SHA256
`4f5da211b7ea5e95e44b7b54fe12cd4971a5a69a2b62aee153dca8b070696b66`。
匹配构建证据取App忽略目录`build/location-loc3-app-20261029`；观察器增量来自当次工具输出，未冒称另有独立raw日志或完整23项终审重跑。

### 13.3 唯一 App／COM11 页面短差异

安装前保留当前全部IndexedDB、local／session storage及原APK，原生SecureStorage只读取账号与登录存在布尔，
不返回或比较Token。原调试snapshot依赖生产包不存在的`ng`，产生一次空账号误判；
文件保留并补独立原生账号证据后才安装，不据该工具误判触发重新登录、发码或清数据。
同包同签名覆盖安装一次；原账号、两个intent store完整内容前后相同。唯一变化的localStorage键为该设备Manifest缓存。

沿用COM11同一身份、已验候选及§12应用／NVS兼容合同；本次只新鲜保留并独立核验24 KiB NVS／分区表，
不重复失效全片备份或其他板测试。实际Manifest匹配后仅为本ID／指纹／endpoint3临时启用私有绑定，背景始终0。
手机实际点击来源／刷新、HOME／返回及Back，由页面自己发HTTP；未通过脚本直调Open／Close代替页面验收。

| 页面范围 | 实际证据与边界 |
| --- | --- |
| 默认与未开放能力 | 默认安装位置不申请定位租约；名单关闭时一次latest200后显示“设备未提供定位”，无Open／持续poll，也无假标记 |
| 单点页面消费 | 只发送一笔FIX seq1，明确`InjectedNonSatellite`，本地status0/error0；页面latest200取得相等的E7点与fresh=true，显示marker、WGS84、未知精度／采集时间及独立接收时间 |
| 读取与需求 | 真实Open接受5000ms，与页面5秒GET对齐；两次前台各同nonce续租一次，返回后nonce不同。active仅是需求受理，不冒充设备ACK或实际采样频率 |
| 后台回收 | 第一段Open请求至HOME触发Close请求12430ms、Close200终态12495ms，均早于30秒TTL；Close至新前台Open间32585ms没有latest GET／Renew |
| 返回与离页 | 返回正常读取旧点并新Open；第二段Back触发Close请求13371ms、终态13470ms，均早于TTL；实际路由回设备管理页，后续捕获区间没有定位GET／Renew |
| 地图与说明 | 本机屏幕已核对来源切换、标记、只读布局及天地图署名；26次地图数据响应200，OPTIONS另26次204。该注入点附近底图区目视空白，不据200宣称底图地物已绘制或推断GNSS精度；未重跑MAP-4、手机定位或跨尺寸矩阵 |

采证共7次latest GET、2次Open、2次Renew、2次Close，13个业务响应均200、结束无未完成请求。
另11个CORS OPTIONS无响应体导致CDP取body失败，记录原样保留；它们不是业务HTTP失败，不丢弃后再声称“全采证零异常”。
返回期间latest.receivedAt保持为最后有效点、年龄继续增加，没有把旧点改成新采集；过期／坏时间的软件边界沿§13.2。
本批证明**App发起且服务接受提前Close**，不把latest未更新或TTL兜底说成MCU物理零包、GNSS模块停机或卫星硬件验收。
独立只读审阅已核对完整业务响应、非卫星本地回执、页面文字／截图与恢复digest；
它只从采证文件核出静默间隙，HOME／返回动作由根线程终端顺序关联，云端收尾取根线程实际只读查询，不冒称子审阅独立访问过服务器。

### 13.4 收尾与唯一下一步

先撤本批绑定1→0，再恢复已验四槽WiFi BIN（§12.3同SHA）及本批新鲜NVS；应用、NVS、分区表均独立verify通过，
正常复位后云端Manifest回到两字段已验指纹，latest行由正常Manifest生命周期清为0。
这不是未知原主程序或完整8MiB的精确恢复。原任务区／授权／UNKNOWN未清、未重放、未Start。
云端四服务healthy，两服务原镜像及原地图4500／UTC日、单用户300／UTC日保持；定位绑定／背景均0。
新版beta保留，原生账号仍登录、全部原store及完整intent未变；串口和本批ADB转发释放，私有完整快照／请求／串口／写入证明留忽略目录
`build/location-loc3-com11-20261029`，不随源码交付。

LOC-3按上述有限范围收口。下一批若继续定位功能，只先明确LOC-4可信设备会话来源、代理覆盖头与provider授权／限额／坐标合同，
再决定最小实现；不先选择任意公网IP服务、用IP替换GNSS或自动覆盖安装位置。
真实GNSS驱动／精度、真点持续采样下的物理停包、轨迹／分享／离线Bulk仍单列，不为补证重跑本批。
本轮不升级SDK／依赖、不扩槽、不提交推送，用户原dirty原生配置及其他本地修改保留。

## 14 LOC-4a 可信网络来源与 provider 门禁（2026-10-29，有限收口）

### 14.1 本批最小范围

先完成认证设备的可信来源元数据，不接公网定位服务、App入口或新的SDK上传路径。
沿Gateway现DeviceKey HTTPS session，复用现TLS代理判定；新增精确设备名单及独立TLS代理名单，默认均空。
当前Nginx模板已覆盖`X-Real-IP`和`X-Forwarded-For`为`$remote_addr`，但容器看到的socket peer仍须现场核对，
不得因为其是Docker桥接／内网地址便自动信任，也不把App请求端或Gateway出口当设备IP。

- 直接TLS仅取socket地址并忽略客户端转发头；只有精确列出的TLS终止代理可提供单一、无歧义的`X-Real-IP`及`X-Forwarded-Proto: https`。
  明文直连、重复／逗号头、端口／zone、内网／特殊用途地址无来源，不影响原登录及限流语义。
- 仅首次proof成功、`finalized.replayed === false`的首次finalize获胜者，且本次Broker注册及PG激活均成功后，绑定逻辑ID、tenant、凭据版本、sessionId、shard／route及签发／到期时间。
  exact replay、registration_pending恢复、challenge、失败／UNKNOWN均不按新请求IP更新；缺失来源宁可未知，等待后续正常新proof。
- 首版仅当前Gateway进程的私有有界缓存：最多128个显式设备、每设备一个会话来源，可用有效期最多签发后10分钟且不超过会话到期。
  不落库、不打原始IP日志、不返回App；重启／跨副本未命中为未知，不伪装成已具备持久或分布式定位服务。
  TTL按访问惰性清理，无后续调用时原字符串可能留到下一次清理或进程退出，不声称10分钟物理擦除；仍有128条硬上限，不另加后台timer。
  内部读取只接受active且精确八字段fence；**它不是鉴权仓库**，未来消费者须先取得当前权威设备／Key／tenant／route／会话状态，I/O后再复核。
  不为本阶段无provider的hint给登录增加额外PG等待；即使撤权与本地记录并发，旧hint也不能凭自身状态获得查询权。
  新会话、版本变化、同时间冲突或时钟倒退不复活旧来源。
- 本批没有坐标、地图请求、分享／历史字段或后台定位消费者，因此不可能覆盖manual revision或GNSS latest。

### 14.2 provider 冻结合同与启用门槛

后续provider仅在owner显式请求、前后权限／会话fence都有效时按需查询；不得放进登录关键路径或自动后台调用。
必须显式提供被查询的设备公网IP；不支持该IP族、无数据、超时、配额或服务失败均返回未知，不能退回服务器自身定位。
来源固定为`network-ip`、城市级粗略参考；保留provider、观测时间／查询时间、坐标系与精度未知语义。
只返回行政区而无已核实坐标的结果不画精确marker，不把城市矩形中心伪装为设备GPS。
GCJ-02不能直接标作WGS84；转换／许可与数据保留合同没有明确前不接现地图。
后续需固定服务地址、整次截止、无自动重试、有界响应与缓存、逐用户／全站配额及并发合并；不接受任意URL或用户自报IP。

现有天地图服务器端Key仅已验证WMTS合同，不能据此推定支持“指定设备IP”的服务器定位。
第三方新Key、获准用途或合法本地数据源尚未落实；本批不发真实IP、不消耗上游额度，也不把纯测试provider算作线上定位通过。
执行顺序为来源／会话最小接线→有限负向／生命周期回归→配置与现登录合同检查。
没有可供真实坐标验收的provider前不部署／刷板，不重复LOC-3／地图／GNSS测试；本批不提交推送。

### 14.3 数据源核查与后续选择

本批查阅公开官方文档，不调用实际定位端点，不携带设备IP／Key。结论不是“天地图不存在IP定位”，
而是尚未核实当前服务器端应用Key查询**指定设备IP**的正式REST／用途／坐标合同，不能把浏览器自身城市定位代用。

| 路径 | 已核实边界／当前决定 |
| --- | --- |
| [高德IP服务](https://lbs.amap.com/api/webservice/guide/api/ipconfig) | 官方需另申请Web服务Key，国内IPv4、行政区／城市矩形；现Key不是该服务Key，不选定或调用 |
| [腾讯IP服务](https://lbs.qq.com/webservice_v1/guide-ip.html) | 官方明确市级非精确，另需腾讯Key；省略ip会使用请求端IP，服务器调用必须显式给设备IP。坐标合同须核对，不能直接宣称WGS84 |
| [ip2region](https://github.com/lionsoul2014/ip2region) | 本地IPv4／IPv6城市查询，不外发设备IP；默认Country／Province／City／ISP／ISO无经纬度。可作为下一批优先评估的城市文字方案，须固定数据版本及许可，不额外做城市地理编码／虚构marker |
| [GeoLite](https://dev.maxmind.com/geoip/geolite2-free-geolocation-data/) | 另需账号／license key及数据更新维护；官方明确是粗略区域而非具体家庭地址。当前无已确认数据库，不自动下载或接入 |

来源筛选参考[IANA IPv4](https://www.iana.org/assignments/iana-ipv4-special-registry/)及[IANA IPv6](https://www.iana.org/assignments/iana-ipv6-special-registry/)，
实现为本阶段保守允许子集，不冒充完整公网可达性判断：特殊用途即使某子段可全局转发也不用于定位；
IPv6仅2000::/3且拒文档／协议／过渡块，mapped IPv4先规范化。代理信任参考[Nginx realip合同](https://nginx.org/en/docs/http/ngx_http_realip_module.html)，
不会默认修改系统`set_real_ip_from`或信任任意多跳链。

### 14.4 最小实现与实际结果

Gateway新增`network-location-source.js`（严格配置／来源）、`network-location-sources.js`（私有有界hint），
现HTTP仅给session独立传`networkSource`，原`context.ip`及限流保持socket peer；challenge不解析来源。
现SessionService只在上述首次激活获胜路径同步调用本地recorder，传八字段投影而非含password／proof的原session或request。
resolver／recorder同步失败只使可选来源未知，不改变exact CBOR、重试类别、注册或激活结果。
现index只在非空精确设备名单时实例化；不添加provider调用、公共路由、数据库迁移、Broker／App／SDK接线或新的网关进程。

| 有限验证 | 结果与边界 |
| --- | --- |
| 来源／配置 | 独立新10项通过；畸形rawHeaders保护后仅受影响3项增量通过。覆盖TLS直连、精确代理、伪造／重复头、mapped地址、特殊用途／内网和名单上限 |
| 私有hint＋认证／HTTP接线 | 新20/20通过，覆盖先注册后激活后记录、失败隔离、并发finalize loser、pending/replay不改来源、八字段fence、容量／到期／双时钟倒退、未知新来源锁旧hint、原限流与CBOR不变 |
| 原session差异 | primary TLS／primary Plain／additional Plain及原HTTP CBOR adapter共4/4通过，复用现`ts-node/register/transpile-only`。初次未加载现TS适配而找不到interface-workflow，未据该工具启动失败判断产品认证故障；未改依赖或把未执行项记通过 |
| Gateway类型核查 | 原TypeScript `tsc -p tsconfig.json --noEmit`通过，无安装／锁文件修改；本机没有npm命令路径时直接用现node解析已安装tsc，没有更新依赖 |
| 独立只读审阅 | 原认证接线及私有hint合同无阻断发现；特别核实repository并发finalize的`replayed:true`必须被排除，以及惰性TTL不冒称物理删除 |

本批没有生成或安装App、部署服务、刷板、查询实际设备来源、修改云端配置／授权／TaskStore或日志；
不拿fixture公网字面值和私有cache测试冒充真实IP定位或地图验收。三端原有本地修改保留，未提交推送。

### 14.5 唯一下一步

LOC-4a可信来源元数据的软件合同有限收口，**LOC-4整体尚未完成，当前用户仍没有IP定位入口**。
下一批先确定可分发／维护的数据源，优先评估本地城市文字、无坐标与无外发IP方案，再实施owner-only按需查询；
若改选外部服务，则先提供对应Key／授权、坐标系及独立预算，不沿用天地图WMTS凭据。
届时需补当前PG设备／Key／tenant／route／active session权威读取、前后fence与错误隔离，再决定是否启用及部署；
不先公开私有hint、不以调用者传`state: active`代替鉴权、不做城市中心精确marker，不重复LOC-3或开启新后台采样。

## 15 LOC-4b 本地城市文字与 owner 按需查询（2026-10-29，有限收口）

### 15.1 数据源决策与本批范围

选择[DB-IP Lite官方固定月版](https://db-ip.com/db/download/ip-to-city-lite)作为首个可选数据源：
官方明确数据为CC BY 4.0、允许应用使用且展示结果须链接署名DB-IP；月版覆盖／精度有限，不当设备GPS。
GitHub参考[sapics/ip-location-db](https://github.com/sapics/ip-location-db)的数据来源／许可分类，而不是把其中不同来源都套同一许可证，
更不跟随rolling latest自动换数据。优先官方MMDB，避免镜像自有字段布局差异。
[ip2region](https://github.com/lionsoul2014/ip2region)仍是中文城市、IPv4／IPv6及有界引擎参考；其软件许可明确，但本批未核到内置原始数据单独权利说明，
不据此断言禁止使用，也不据代码许可保证内置数据可任意分发。

复用现Node／Gateway，新增一个精确版本、无运行时依赖的[mmdb-lib](https://github.com/runk/mmdb-lib) reader，不自行写MMDB/XDB解析器。
不升级任何已有依赖或MCU SDK；锁文件只允许新增该reader及Gateway直接依赖。主工作树node_modules不重装。
数据作为可选的只读运维输入，不打包数据库到设备库／App／Git，不在登录或请求路径下载／自动更新。

- 默认没有manifest，功能关闭；只有既有精确设备名单＋可信代理＋显式数据manifest时才能查询。数据缺失／校验失败只关闭本功能。
- 固定月份、官方源URL、文件大小、SHA256与许可；启动时有界异步读入，最多128MiB，验证实际metadata及版本来源。
  仅一个共享只读reader，请求期全内存，不把async外观的同步磁盘IO放进登录／控制路径。
- 首版只投影国家码／名称、省、市和数据版本／署名；即使库有lat/lon也主动忽略，不新增地图marker或坐标转换。
- API固定`GET /api/v2/devices/:id/location/network`，无任意IP／URL／session参数、无请求体或query，owner-only、no-store。
  先现owner授权，再取私有hint的无IP fence；按这个server-owned sessionId单条PG只读查询当前设备／Key／tenant／route／active session，
  匹配后才内部取IP。查询完成重新核owner、PG及缓存fence，变化就丢弃结果，不跨provider持锁。
- 复用现用户鉴权及账号限流，有界并发、取消后不交付；未知来源／无命中正常表示未知，服务故障不伪装成定位成功。
  PG信号检查不冒称真正取消数据库查询，不为此改全局DB池或认证链。

本批先完成只读adapter／owner API、负向／fence回归和匹配类型核查；不重复LOC-3、不接App新入口、后台定位或分享。
实际月库和云端代理未核验前不宣称真实设备IP定位通过。必要部署与真机测试须在软件／数据准入通过后单独记录，不提交推送。

### 15.2 必要实现与默认关闭接线

- `network-location-provider.js`：固定本地manifest、精确官方月份／URL／CC-BY许可、普通文件及解压大小／SHA校验后构造现成reader；
  一个共享只读Buffer，启动异步有界读取、不拼无界chunks；请求期只在内存解码，**即使库中有经纬度也只投影四个文字字段**。
  核实际type、format、IP族、树大小及精确月份metadata，93日有效期于启动及查询复核，没有热更新／下载／新timer。
- `network-location-authority.js`：按私有hint中的server-owned sessionId一条只读PG查询，显式九列且核owner、tenant、Key、设备、session及route；
  不select凭据／原IP，不凭PG“最新session”推断来源。前后owner与PG／缓存fence复核后才能交付。
- `network-location.js`＋`network-location-http.js`：固定GET `/location/network`，现鉴权／账号限流、严格无query／body、no-store，
  缺能力／过期来源／无数据为unknown，provider不可用503；取消／撤权／版本变化不交付迟到结果，不自动重试。
  DTO固定`source: network-ip`，region四文本字段／观测时间／查询时间／数据版本及DB-IP署名，坐标和坐标系均null。
- 现GNSS latest与新城市GET抽出同一个`location-read-http.js`薄层；GNSS原16并发、无新增限流／截止的默认保持，
  新城市独立4slot／5秒HTTP截止。未结束的PG工作仍占slot，防止超时后无限叠加；不将信号检查称为取消PG或抢占同步decode。
- 在原Gateway装配处注入同一来源cache；partial init也不另造cache。数据manifest空或设备名单空默认不加载月库，
  现Nginx渲染新增独立`networkLocationEnabled`开关，默认输出逐字不变，不借已启用GNSS开关开放新API。

现成reader新增精确`mmdb-lib@3.0.3`，MIT、无生产依赖，NPM SRI已锁定；没有升级旧包或MCU SDK。
NPM lock-only初次清掉12项旧Linux包的libc元数据，已用patch恢复并逐项比较：**旧包entries全等，只有该reader和Gateway直接依赖新增**。
主node_modules未重装，仅隔离目录安装reader；原pnpm锁／App／SDK源码未改。测试复用既有node:test、TypeScript及Nginx渲染脚本。

### 15.3 官方月库与有限结果

一次下载固定官方2026-10 gzip，无重试／重定向；压缩60,193,321 B、解压126,998,165 B，均低于128MiB。
发布页SHA1 `df17bd24390108ed1d24dcb7718dc60f6b9abd4f`匹配**解压MMDB**，不是gzip摘要；本地MMDB SHA256为
`9e250f02722d1ad1780f88192f0e908af285478a33e76b88b349c9181ae9d0f5`。
实际metadata为`DBIP-City-Lite`、MMDB2／IPv6树／28-bit record，buildUTC `2026-10-01T01:41:21Z`，不是猜类型后声称匹配。
数据及manifest只在忽略的`data/network-location-loc4b`，下载／许可／metadata证据在`build/network-location-loc4b-data-20261029`；不进Git或App／设备库。

| 有限检查 | 实际结果／边界 |
| --- | --- |
| 新PG authority单测 | 最终7项有限通过：初轮6通过，一项fixture缺省参数修正后仅该项1通过；无产品改动，不冒称全量终审重跑 |
| 新query服务 | 原Node22.22.3最终18/18通过，含owner／share、前后fence、scope原地变异、取消与错误脱敏；此前Node24初版17项不重复累计为独立用例 |
| 本地provider | 新9/9通过，覆盖路径／大小／hash先于reader、坏metadata／月份、运行中过期、只读共享buffer、无匹配及不输出坐标 |
| 新HTTP／Nginx入口 | 新7/7通过，含独立flag、关闭逐字保持、GET/no-query/no-body、账号限流、错误脱敏、取消回收、5秒超时且未结束工作占4slot |
| 抽取薄层差异 | 仅原GNSS latest两个HTTP受影响用例＋新私有无IP fence一项，3/3通过；没重跑LOC-3或整套旧矩阵 |
| 官方真实月库 | 运维检查通过，固定公开示例IPv4／IPv6均可本地解析四文本字段，coordinates=null；没有查真实设备IP，不是城市准确度／卫星／地图验收 |
| 真实PostgreSQL | 复用现SSH脚本，在PG16.15独立连接BEGIN、search_path仅pg_temp/pg_catalog、5张TEMP表，8/8合成owner／tenant／Key／route／session／expiry案例通过，最终ROLLBACK并断连。未访问或修改业务行、迁移、账号或授权 |
| 匹配编译／审阅 | 原Node22.22.3下Gateway noEmit及隔离目录完整emit通过；独立只读审阅无阻断发现。reader实际包默认noCache，不引入无界解码cache |

数据库临时表实测与本地完整编译不等于已部署Gateway／真实用户查询。没有镜像构建／重建容器、服务配置修改、App安装、设备登录／重置或刷写，
账号、授权、TaskStore／UNKNOWN和完整旧日志保留。没有更新Key、发送验证码、外发设备IP或提交推送。
本批新增只读运维入口`deploy/beta/check-network-location-dataset.cjs`，可复用其纯离线校验，不让运行进程下载或自动更换月库。

### 15.4 尚未覆盖与唯一下一步

LOC-4b按上述软件／数据／隔离PG范围收口，**LOC-4用户闭环仍未完成**。
下一批先核云端剩余内存／只读数据挂载、实际TLS终止peer、精确设备名单及独立Nginx开关，再只部署匹配Gateway和月库，
复用一个既有设备的新正常认证（不重放）验证owner城市文字、错误／撤权与原登录不受影响；不必为本功能改MCU SDK或反复刷板。
通过后再给App增加显式“网络参考”只读入口，按需单次查询、来源／观测时间／未知态及DB-IP链接署名；不画城市中心marker、不覆盖manual或GNSS。

来源是**认证时的网络出口**而非实时GPS／当前MQTT在线证明；原hint最长10分钟，重启或超期没有来源时必须unknown，
不因owner打开页面而强迫设备重新认证、延长旧hint、自动回退服务器出口或新增后台采集。
长期在线设备的来源可用性需以后单列设计，不在这轮暗改认证频率。DB-IP免费月版的准确度／覆盖不保证，真实生产RSS也须部署前核，
128MiB文件上限不是进程RSS或抗任意恶意MMDB的保证；只接操作员批准的官方hash产物，拒用户上传库。

## 16 LOC-4c 匹配云端候选与单设备只读短验（2026-10-29，有限收口）

沿§15已通过的完整emit、官方月库和PG临时表证据，不另开编译／安装队列。
先核实际运行代际、内存及loopback HTTP路径，再仅叠加Gateway的12个JS差异与单个纯JS reader；
原Linux／native基底、Key、签发、PG／Broker／Redis及原私有环境不变，不全量npmci或迁移业务数据。
运行前私有保留必要配置、原Gateway日志及App全store／账号／请求记录，不做重复Flash或整库备份。

本批最小执行：

1. 精确当前image／config SHA／Nginx文件及资源前置；只信任同上游TCP路径实际观测到的一个peer，不自动信任Docker／内网范围。
2. 以当前immutable Gateway为基底离线叠文件，逐文件／package元数据验证；月库固定官方版本和SHA，只读挂载，临时隔离容器先验证provider可读。
3. 只重建Gateway一次，新增的仅该image、三项network配置及一个只读数据mount，CAS及剥离增量后的完整配置比较通过才应用。
4. Nginx仅新增精确owner城市GET入口，不重渲染／覆盖其他虚拟host、TLS、stream或proxy配置；检查再reload，不自动重做未知结果。
5. 复用真实beta账号及COM11旧身份；必要时只正常复位一次产生新proof，不刷板、不重放旧请求、修改Key／TaskStore或重跑控制矩阵。
   查询网络参考／原State或Manifest及必要的无凭据／坏输入负向；不为做撤权样本而删设备／改实际所有权，真实撤权边界与原PG／软件证据分别记账。

通过后保留匹配Gateway与只读月库，网络来源名单**只保留COM11一项**用于后续App阶段；这不是所有设备正式开放。
原GNSS定位／背景名单仍0/0，不借此新增后台采集；只读网络查询不触发设备认证或自动延长10分钟hint。
未完成项和失败／未知如实保留，不将无UI的运维HTTP调用说成App“网络参考”交互已验。本批不提交推送。

### 16.1 实际部署与短验结果

| 范围 | 本批证据与边界 |
| --- | --- |
| 运行前置 | 原四容器健康；宿主可用内存约1.27GB，Gateway限额512MiB。一次同后端路径零HTTP载荷TCP观测确认唯一peer，未信任整段Docker／内网地址；随后真实新认证与查询验证实际来源链路 |
| 匹配候选 | 复用原Node22隔离emit，不重开编译。固定Linux基底叠加12个Gateway JS及mmdb-lib3.0.3的19个文件；离线构建／逐文件、模块、package唯一新增依赖、原image Config及底层layers检查通过。不是完整npm依赖镜像重建或新的库发行验收 |
| 数据 | 云端一次下载固定官方2026-10产物，解压126,998,165 B；发布SHA1及§15 SHA256匹配，manifest固定。仅数据目录只读挂载，不放App／SDK／Git；隔离容器无网络、无任何IP lookup地加载通过 |
| 有限应用 | 只重建Gateway一次，唯一变更是image、三项network配置及一个只读mount。剥离增量后完整Compose相等；Broker／PG／Redis的容器身份、原env／mount、57项迁移不变；旧Gateway完整日志含停机尾部已保留 |
| Nginx | 仅新增一个精确GET／OPTIONS block，剥离后原文件逐字节相等，其他已加载文件／proxy／stream不变。初次预检因错误要求API文件属于root而停在写入前；只读核实原属主1000:1000、0600及内容SHA后，以独立reviewed attempt保留原属性，nginx -t及一次reload通过。没有自动重做未知结果或更换原属主 |
| 现设备新来源 | COM11正常复位一次，确认仍为既有ESP32S3；未刷写、清授权、变更Key／Manifest／TaskStore或创建身份。新Gateway启动后的正常新proof已生成来源；既有owner通过真实beta WebView的原native账号作单次GET，HTTP200、source=network-ip、status=available、有城市文字，耗时约600ms |
| DTO／隐私 | 观测时间晚于新Gateway启动，查询时来源年龄462,677ms，小于10分钟；数据版本2026-10、DB-IP署名／链接／许可匹配，coordinates与coordinateSystem均null，无原IP或session字段。是认证时出口的粗参考，不证明城市准确度、当前在线或GPS位置 |
| 入口负向 | 无登录GET401、额外query400、错误方法405，均private/no-store；无自动重试。未为做撤权样本修改真实owner／Key或删除设备；实际双账号撤权不计本批通过，相关权限／并发fence以§15软件及独立PG临时表证据保留 |
| 原只读与保留 | 原State、Manifest GET各一次HTTP200，约203／219ms；不是控制ACK或新鲜State矩阵复测。相同APK／原账号、全部原store及完整intent journal前后核对，journal不变；调试forward已释放，未发送验证码或刷新用户token |
| 实际资源 | Gateway实际Node RSS约229,650,432 B；收尾cgroup current179,130,368 B、peak194,756,608 B、limit536,870,912 B。隔离probe RSS186,408,960 B另记，不冒充运行Gateway的heap；以上仅有限快照，不是容量／长稳承诺 |

本批image为`sha256:d0fa1ccc28d8779950e14d3d7947930dba43f32b4d9fb7c0320c7e17368365c2`，
source graph SHA256为`d8345b35bc666f718a4d22096d05444dc87306b05f31f9d8ac8fe76f2782bfce`；
数据库及数据manifest摘要取§15和私有运维证据，不把镜像／公开摘要当成新的发行签名。

收尾四服务健康，按本批计划**保留匹配Gateway／固定月库／COM11一个network ID／一个可信peer**，GNSS绑定及背景名单仍0/0。
App／设备固件未改或安装，无需固件恢复；授权、UNKNOWN、原任务与云scope保留，未改轮询／SDK／槽位或提交推送。
私有证据在Service忽略目录`build/location-loc2-deploy-20261028`、Library忽略目录`build/network-location-loc4c-20261029`，
云端本批operation目录留必要配置／新旧Gateway完整日志及阶段状态；不将这些私有文件作为交付源码。

### 16.2 唯一下一批

接入App显式“网络参考”按需只读入口，复用原鉴权及设备位置组件；展示四文本字段、来源、观测时间／未知态及DB-IP回链署名。
不画城市中心marker、不覆盖安装位置或设备GNSS，不添加自动查询／重试／刷新认证或后台采集；仍沿单设备受限名单。
优先补App有限回归，再沿本批已保留的服务做一次最小交互短验，不重复镜像构建、数据下载、GNSS／地图／设备控制矩阵。
来源过期／重启缺失正常显示未知；长期在线来源刷新、真实双账号撤权与GNSS硬件精度分别留账，不以扩展认证频率来补本批样本。

## 17 LOC-4d App 网络参考入口（2026-10-29，有限收口）

本批只接§16已部署的owner GET，不改Gateway／Broker／设备、数据集或认证频率，不新建身份／租约。
沿用现位置页、Angular HttpClient／鉴权拦截器及账户可见性生命周期；仅增加严格有界DTO和GET薄服务。

- 来源选择增加“网络参考”；选择不请求，只有显式按钮单次查询。查询中防重入，回前台不自动查询／重试。
- 文字区展示国家／省／城市、来源、观测／查询时间及固定DB-IP链接署名；未知、失败、未登录和权限变化分别处理。
- 不画marker、不调用手机定位、不以IP生成坐标，不改安装位置CAS或设备GNSS样本。
- 切换来源、后台、离页、销毁、账户／owner变化取消请求、清空网络结果并拒绝迟到交付；保留原安装位置／GNSS链路。
- 先有限模型／实际拦截器HTTP／模板生命周期回归，再复用原脚本作一次匹配beta构建和必要短交互验收。
  账号／完整journal保留，不为有城市样本强迫认证或刷板；若来源已过期，真实未知态不冒充成功定位。

使用UI/UX检查指导显式加载反馈、可触达按钮和状态可读性，沿用既有token／图标／布局；不重做地图或新建页面设计系统。
旧LOC-4c的WebView运维GET不替代本批页面验收；未完成的构建／安装／交互须分别记录，不提交推送。

### 17.1 实现与有限回归

App仅增加`device-network-location.ts`有界、精确DTO及`device-v2-network-location.service.ts` GET薄层；
复用原HttpClient／Bearer／请求标识与`GATEWAY_NO_AUTH_REPLAY`，15秒截止、取消、不刷新token或重放。
`api.config.ts`仅增精确`LOCATION_NETWORK`及无query／hash的合法ID匹配，不放宽其他位置路径。
解析拒绝额外字段、坐标／session、任意署名链接、错设备、错时间和超10分钟来源；未知允许无来源或有来源但未命中城市。
固定DB-IP链接配`noopener noreferrer`，不用服务器文本生成HTML或跳转URL。

页面沿用原位置source选择／owner与账户fence，仅加第三个来源和独立一次性AbortController，未新造poller／watch／缓存。
网络数据从不进入`point`、手机定位、安装位置CAS或GNSS租约；前后台回收复用现可见性订阅，迟到结果另核账号、ID和请求owner。

唯一有限Angular队列：5文件134/134通过（DTO41、实际鉴权HTTP13、原GNSS HTTP12、位置页65、原地图生命周期3）。
位置页65含本批16项，覆盖选择不GET／并发防重、未知态、固定署名、503／429／服务404手动恢复、401／403禁写禁重放、
源切换／后台／离页／销毁取消及账号／epoch／owner变化时服务即使忽略abort也不交付迟到数据。
原49项位置页和地图／GNSS HTTP用于验证实际受影响consumer，不把它们累加成新的设备或BLE循环。
独立只读审阅无阻断发现；差异空白检查通过，没有升级依赖／SDK或扩任务槽。

### 17.2 匹配构建、安装及真实页面

复用原LOC-3构建／资产验证脚本，仅给忽略目录output增加显式受限参数；原证据和queue lock不覆盖，不复制整工作树或重装依赖。
Node22.22.3、pnpm11.19.0、esbuild0.28.2、JDK21.0.12.1保持；一次`build:cloud-beta`→`cap copy`（非sync）→原Gradle离线beta完整打包通过。
1153份Web资产与APK逐字节匹配，`iot.diandeng.tech.beta`、HTTPS／禁明文、19项原生注册表及原签名均通过；
冻结源码／锁文件／原dirty原生设置构建前后逐字节不变。APK SHA256为
`f17fd31ff0e99fa5d9d04aa72f3208f35f7809ca6eaa3700c971cfd14fd815f2`。

先保留原账号／全IDB／local与session／完整journal，再一次`adb install -r -t`覆盖内测包成功；未清App数据、主动登录／刷token或发送验证码。
使用原owner、COM11既有身份及§16服务，**通过真实位置页DOM点击**而非手工fetch：

| 页面短验 | 实际结果 |
| --- | --- |
| 进入与选择 | 正常进入位置页后选“网络参考”，network GET为0，查询按钮可用，地图关闭；未请求手机定位 |
| 显式读取 | 点击按钮一次，仅1次业务GET返回200，约191ms；另有正常CORS OPTIONS204，不能误计成第二次查询或重试 |
| 展示 | 真实城市四文本、认证时出口来源、观测／查询时间及DB-IP署名链接进入模板；截图查看文字不叠地图、不画城市marker。时间按手机系统本地时区展示，不将手机时区作为设备授时证明 |
| 后台／返回 | HOME后回原页，网络结果及署名节点清空、按钮回“查询网络参考”且可用；第二次DOM核对请求数仍仅1 GET＋1 OPTIONS，没有自动补查 |
| 离页／保留 | 正常返回设备页，捕获结束无未完成请求。新版APK匹配、原账号／原store均在、完整intent journal前后不变，自建ADB forward已释放 |

本批没有为获得available而复位或重认证COM11：原固件正常产生的可信来源可用，页面成功不等于GPS／城市精度或长期在线可用性。
真实unknown／错误／取消／撤权竞争以有限模板和HTTP回归覆盖，未在云端制造撤权、断网或新身份样本。
截图只核当前手机竖屏，不冒称完成375px／横屏／大字和暗色的全视觉矩阵；语义token、至少48px触控区与可访问状态沿UI/UX检查落实。

匹配beta保留；服务、月库、设备固件、TaskStore／云scope／授权及UNKNOWN未改，无需固件恢复。
构建证据在App忽略目录`build/location-loc4d-app-20261029`；私有快照／脱敏请求元信息和截图在Library忽略目录
`build/network-location-loc4d-20261029`。未提交推送，不将私有记录或APK作为源码交付。

### 17.3 剩余边界与后续顺序

网络城市参考的owner按需入口至此有限收口，不再因同一环境刷新而重做§16～17或重建候选。
后续按§18用户确认的职责边界执行：GPS／其他设备的驱动、协议及解析由用户负责，SDK仅提供统一观测接口，不以缺某个模块阻断API交付。
长期在线来源刷新／多副本、月库换月运维、真实双账号撤权、完整视觉矩阵分别列为后续独立项，不隐式开放所有设备、改认证节拍或扩成轨迹／分享定位。

## 18 LOC-5 统一 Sketch 定位输入（2026-10-29，有限收口）

用户明确：真实观测来自用户的GPS或其他设备，库不负责其驱动／NMEA／串口／第三方解析，只提供`Blinker.xxx(...)`统一接口。
因此取消§17.3原先“先确认具体模块”的前置，不新增任何硬件驱动依赖，也不为本批测试刷板／伪造卫星精度。

最小实现：显式caller-owned `BLINKER_LOCATION(position)`随原`Blinker.begin(..., position)`声明只读定位能力；
`Blinker.location(position, latitude, longitude[, sampledAtMillis])`接WGS84角度，
或`Blinker.location(position, LocationFix[, sampledAtMillis])`保留E7／精度cm／UTC质量。
`locationLost(position)`抑制失锁后的新采样，`clearLocation(position)`另清本boot的样本顺序，均不写云端记录。
默认没有有效点，非法输入不覆盖旧点；0,0可为真实有效观测，不能拿它表示失锁。

复用原`LocationSnapshot`／`CloudLocationBinding`／私有Cloud租约与State guard；不造上传器、timer、线程或队列，不自动授时。
仅持有显式Location对象的Sketch增加其固定内存，默认Facade／Client／Device不加字段。绑定同Client幂等，跨Client拒绝，失败可按原begin流程恢复。
输入与`Blinker.run()`在同一Sketch上下文串行调用；采样tick与已绑定Client同一boot单调时钟，不从GPS UTC或外部设备tick推本地年龄。

验证顺序：新接口host有限回归及原codec／Cloud绑定／三种facade多TU的受影响consumer→复用现facade编译脚本，
仅用原ESP32 SDK作新接口对象编译／ABI与ODR token核查。对象编译不冒充新的安装ZIP、完整MCU链接或物理测量。
首版上传仍取原ESP32 Cloud受限绑定，不扩BLE／LAN／R4／Wio或分享／历史支持；本批不部署、发码、清授权或提交推送。

### 18.1 实现与结果

使用说明集中在[统一输入API](DEVICE_V2_LOCATION_INPUT_API.md)，README只保留入口及支持边界。
新`location/Location.h`提供静态不可复制Spec、显式不可复制／移动的Location和两个声明宏；
Facade新增四种输入及Lost／Clear薄方法，Client仅增forward／friend，不增加本批默认成员、时钟或全局定位对象。
输入读取已绑定Client同一IClock，不直接读外设UTC；便捷经纬度先拒NaN／无穷／越界，再取整E7，精度／UTC保持未知。
高级输入复用原`validateLocationFix`和单快照校验，不在普通State保存坐标。新对象同Client重复bind成功、跨Client拒绝、失败attach清指针可重试。
失锁／清理、过期／乱序、无租约／租约关闭均沿原路径；不会因为调用输入API就上传或自动授时。

| 有限验证 | 实际证据／边界 |
| --- | --- |
| 新接口host | 4组通过：真实facade＋fake Cloud Hello／租约及原codec解码，含0,0、边界、正负E7取整、NaN／Inf、精度未知／已知0、UTC不授时、年龄／乱序／回绕／失锁／清理、不同Client及失败恢复、原report和私有State guard。测试采样不是卫星数据 |
| 无堆／尺寸 | 新host用例覆盖期间new／new[]计数不增；64位host facade24 B、可选Location128 B，原布局断言不变。原ESP32对象的`position`符号104 B，facade32位12 B断言通过；不是全产品RAM峰值或实际设备运行预算证明 |
| 受影响consumer | 原Location、Cloud绑定及WiFi／BLE／WiFiBLE三个facade多TU目标与新目标，指定6目标编译／链接及CTest6/6通过；未运行其他矩阵／BLE或断电循环。首次编译仅因测试全局变量名与既有模板参数重名而/WX失败，重命名fixture后通过，未降低警告或修改产品模板 |
| 原SDK对象 | 复用`facade_api_compile_gate.py --location-input`及原ESP32 3.3.11真实编译命令，current-src-first；含六个方法调用的driver-free probe通过，临时Spec负向按deleted拒绝；core／三模式TU的facade tokens完全相同。未升级SDK，不把object检查称为完整MCU链接／安装ZIP验证 |
| 原候选合同 | 原`test_location_candidate.py`4/4通过，旧三字段候选、Manifest及公开目标矩阵未变；没有改旧候选摘要、串口注入器或将其冒充新facade验收 |
| 审阅 | 独立只读审阅无阻断发现；生命周期、同clock、旧fix原子保留、私有链路、无默认成员增长及文档本地接受／有限平台边界匹配 |

driver-free编译参考在`tests/arduino/CompileLocationInput`：不自造观测、不调用模拟输入、loop只推进原`Blinker.run()`，用户自行接解析器。
旧`CompileCloudLocation`的两个内部端口仍被冻结候选和受控观察测试引用，故保留，不把它们当“未使用旧实现”删除。
验证复用现`build/location-loc0-20261028`的host树，新的SDK／日志证据在忽略目录`build/location-loc5-input-20261029`。
本批无App／Service源码修改、安装、部署、设备操作、账号／授权／TaskStore修改或Git提交推送，保留全部无关本地修改。

### 18.2 后续最小范围

用户不需要提供GPS模块型号才能使用统一接口；具体驱动、解析、源真实性和物理精度不再作为SDK接口交付门禁。
该剩余步骤已由§19完成：新driver-free消费者接入当前源码的隔离安装与原SDK单目标完整链接，核包内新头和实际链接输入，
不覆盖旧R5候选摘要／artifact，不重跑App／服务／地图，也不为本接口烧录带假观测的固件。
实际Cloud启用仍需匹配Manifest及既有精确准入；不借此扩大平台、协议、后台采集或自动授时。

## 19 LOC-5b 隔离安装与原SDK单目标完整链接（2026-10-29，有限收口）

仅补§18.2尚缺的当前源码安装／完整链接，不重复host、App、服务、设备或真实GNSS流程。
复用原单库打包和`verify_installed_release.py`，增加与旧LOC-2候选互斥的独立`--location-input`消费者：
只选择原`esp32-cloud-smoke`的板型／库闭包，换为driver-free `CompileLocationInput`并关闭不需要的业务授时。
公开七目标、旧候选／R5摘要和官方SDK版本均不变。

先冻结本批打包源码／公开profile／probe，生成唯一新内部ZIP，安装到新空sketchbook；仅复制已验PubSubClient2.8缓存，复用原ESP32 3.3.11 SDK，不复制SDK或全工作树。
逐字节核ZIP／安装树／当前冻结头文件，编译命令不得引用仓库src或旧安装库；单worker完整编译／链接，核原WiFi TLS闭包、无任务及无新业务时钟。
通过probe内部六个volatile函数指针保留所有输入adapter，loop仅读取、不调用，拒绝“只include头就通过链接”；不为符号检查更改产品优化或强留本应内联的短方法。
保存新阶段、原始日志及包／ELF摘要，不覆盖旧输出、不自动重跑失败或另开队列；没有硬件操作或新观测，本批不提交推送。

### 19.1 必要接线与有限回归

`verify_installed_release.py`只增加独立`--location-input`：与原`--location-candidate`互斥，限定单一`esp32-cloud-smoke`基板型、
原SDK3.3.11、jobs1／2、已有公开Cloud零任务profile；该内部consumer的`time_enabled=False`，不误借原cloud-smoke的授时验证。
原`TARGETS`、五个公开示例及旧LOC-2候选不变，没有增加对外平台或安装第二套Blinker库。

复用`configured_library_files`冻结全部437项当前源码／渲染profile与必要输入；旧ZIP、缺／变新头、profile／probe变化均拒绝。
安装前及完整编译后核ZIP／安装树逐字节、冻结源码不变；原provenance校验仍拒仓库src或其-I路径代替安装库。
链接要求六个具备正确签名的probe wrapper及原Cloud prepare／write、Client attach／采样preparer路径，
不强制短inline方法有独立ELF symbol、不修改产品优化或加noinline。新7项门禁回归与旧候选4项合同回归均通过。
独立只读审阅无阻断发现；没有重跑§18 host、App／服务器或硬件矩阵。

### 19.2 唯一实际队列与产物核对

仅执行一次本批队列，打包约2.7秒、安装及完整链接约373.9秒，无失败重跑／第二队列。
新空sketchbook复用原ESP32 3.3.11 SDK；依赖只离线复制已验PubSubClient2.8的41个文件约0.11MiB，未复制SDK、旧Blinker或整个工作树。
原SDK data保持原版本，user／下载及显式`--build-path`输出位于本批目录，使用`--clean`；没有生成另设的cache目录，不据此声称独立core cache已生效。
不写系统Arduino15或执行依赖／SDK升级；实际库来源与完整编译命令由下方provenance检查核对。

| 项目 | 实际结果 |
| --- | --- |
| 内部ZIP | 当前源码按原`profiles/cloud-beta.json`生成一个内部ZIP，746,690 B；安装树与ZIP及437项冻结源码／profile一致 |
| 完整编译／链接 | `esp32:esp32:esp32`／原SDK3.3.11／jobs1、`CompileLocationInput`，编译尺寸及最终ELF均生成，完整门禁通过；不是只编译头或局部object |
| 来源 | 17个Blinker源码单元来自本批安装库、仓库src编译单元0，原-I泄漏检查通过；实际库仅BlinkerDeviceV2、PubSubClient、Network、WiFi及TLS所需NetworkClientSecure |
| 功能闭包 | 六种输入wrapper与Cloud四条路径实际保留；业务TimeSync／WallClock和任务闭包未启用，BLE／LAN／网关等原禁用扩展未泄漏 |
| 尺寸 | Flash 1,065,200／1,310,720 B（81%）；静态RAM 52,976／327,680 B（16%）。ELF文件15,281,700 B含调试信息，不是Flash占用，也不是动态heap／栈峰值或运行预算证明 |
| 输入与保留 | 本批打包源码、公开profile、probe及门禁脚本前后摘要一致；旧R5／LOC-2产物未覆盖，原dirty修改保留；没有设备、账号、授权或TaskStore操作 |

内部ZIP SHA256：`8361fc3cd80c24c4f105414bd65aaf2481e84f14d2d46ab6938fab3e1884fc0e`。
最终ELF SHA256：`86fc5710160274c704754e5b52e18e7d616783fbef3521366c32002875484897`。
公开profile SHA256：`b4ef0cb36a7fdc654b71bfc4b1ab47850c0a58819f6559e028f67bf89b7bcd30`，未为定位改配置。
原始日志、冻结清单、安装证据与只读ELF摘要在忽略目录`build/location-loc5b-install-20261029`；不是对外发布或同事签收产物。

### 19.3 收口与剩余边界

统一输入API的实现、有限行为回归、原SDK隔离安装及完整链接至此收口，不再以具体GPS硬件或解析库作为SDK门禁。
没有提供模拟点、运行新固件或声称卫星精度；用户模块／外设接入由用户完成。Cloud上线仍需原精确Manifest／owner准入，未扩成所有设备默认定位。
后续若准备发行，先将这些必要源码／使用说明及可选消费者门禁纳入同一库的下一份候选范围，再决定发布；不重打本批候选补样本，
不自动增加新平台／驱动／历史／定位分享或触发部署／推送。原R5冻结归档不因本次内部验证而更名或替换。

## 20 LOC-5c 同库交付说明与可选 CI（2026-10-29，有限收口）

本批完成 §19.3 的交付准备，具体维护入口为[库发布方案 §13](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#13-loc-5c-下一候选说明与可选-ci-接线2026-10-29有限收口)。
不改产品实现或公开 profile，不新增 GPS 驱动／依赖、支持平台、传输链或默认任务槽。

README 随同一库提供独立最小示例、输入顺序／boot-local tick、年龄、Lost／Clear、本地接受不是 ACK 及原 Manifest／owner 准入说明。
源码仓库中的进阶 API 文档不作为 ZIP 安装者的必要前置，不为塞入内部计划而扩大打包白名单。

现有手动受限候选 CI 新增默认关闭的 `verify_location_input`：原打包一次、校验和与两个普通目标不变；
勾选后对同一 ZIP 使用原 `--location-input` 消费者，在独立空 sketchbook 复用原 SDK／已装 PubSubClient，单 worker 完整链接。
可选步骤在 artifact 上传之前，失败不上传；该输入不打开运行定位准入、不产生观测，不冒充物理 GNSS 验收。
原质量 CI 已统一运行 release 合同回归，无需另一套调度器或新依赖。

受影响的交付回归 9/9、本机 YAML 结构检查及独立只读审阅完成；没有重跑 §18／§19、重打 ZIP、部署、刷板或触发远端 CI。
§19 的 `8361fc3c…fc0e` 仍对应补充本批 README 前的历史包；未来候选以实际检出提交和同批校验文件为准，不覆盖旧摘要／产物。
原账号、授权、UNKNOWN、完整日志、服务及设备均未操作，无关本地修改保留。

**至此本地定位接口与交付准备收口，不再排相同验证队列。** 剩余仅是另行授权后的源码提交／推送、匹配提交的手动 CI 与发行决定；
尚未执行远端 CI 或公开发布，不把本机通过替代它们。真实 GNSS 驱动由用户负责，其他后置功能不自动启动。
