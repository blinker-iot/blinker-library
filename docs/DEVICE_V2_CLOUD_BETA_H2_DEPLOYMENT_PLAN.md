# Device V2：独立云端内测与 H2 续验计划

更新：2026-10-21。用户指定将 IoT 部署到云端，复用现有 Aily-service 测试服务器注册链路。**本文件区分部署、健康、真实登录及 MCU 验收，不以其中一项代替全部完成。** 本机 H2 部分证据及两次环境中断取[H2 §8](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md#8-h2a-4部分真机证据现场中断与有限续批)，不重复已通过项。

当前结果取**[D4：现有COM11的ESP32云端四槽TLS有限短验已收口](#d4-esp32云端四槽任务最小准入与部署差异2026-10-20)**：实际P256材料匹配、原SDK链接、匹配beta／服务部署、查询／保存／唯一Start／到期完成及原控制ACK／新鲜State均通过；固件、TaskStore及active云scope保留。当前云实例与beta显式启用tasks＋automation，但任务只有本次COM11单ID准入；基础模板及普通prod/dev默认关闭，普通Arduino公开profile仍0槽。D3a～D3c最小自动化不重跑；R4原四槽Plain与普通0槽TLS差异不能扩大为四槽TLS，Wio／Error8继续后置，不冒称完整生产发行或同事签收。

## 1 本批固定范围

- API `https://iot.yiyu.pro`、WSS `wss://iot.yiyu.pro/mqtt`、MQTTS `mqtt.yiyu.pro:8883`。使用已有可信域名证书，不再依赖 IP 证书。HTTPS、WSS、MQTTS 分别核验，不把 MQTT 域名 443 的默认站点当成 8883 的验收。
- 复用 `https://api-test.yysc.tech` 的真实注册／ALTCHA／登录／me／refresh 链。工作区既有部署记录与公开无凭据 GET 已核对：ALTCHA 200、me 401，标准 TLS 校验通过；尚不等于新账号邮件与真实登录已验收。三个 region 当前明确指向**同一测试身份服务**，不是三个区域部署。
- 独立 Gateway、单 managed Broker／shared shard 0、PostgreSQL 16、Redis 7；不搬本机数据库、账号、授权、请求／UNKNOWN、设备身份、JWT／KEK 或 RFC 演示私钥。Auth 不另建，所有 IoT 凭据独立生成。用户确认统一使用当前测试 Auth：该环境正常注册且可登录的账号（包括 Aily Blockly 用户）均可登录 Blinker，不要求另注册、开通专用 Broker 或来自指定客户端；账号／设备 ACL 和原配额仍独立生效。
- 当前云实例开放注册登录、设备／Key／session、控制／State、名称、普通页面配置、H1／H2、同片非定向分享，以及D3b最小自动化与D4单设备任务精确入口。任务仍受双侧逐设备名单与原权限／设备能力限制。普通 `/console`／`/ui-config` 不等于 Console 链接／公共访问／查看会话；后者与Direct／BLE／LAN、定向邮箱目录、通知、Controller、动态及跨片继续关闭。基础部署模板不因实例启用而改默认值；仍是**受限内测**，不是完整旧本机功能／全部平台的生产发行。
- 不扩设备槽位、不升级 MCU SDK、不扩离线历史／Bulk、不提交推送。不把本机地址 BIN 烧录为云端候选，不保留重复编译队列。

## 2 分层与持久边界

```text
App / 设备
    ├─ HTTPS / WSS ─ Nginx :443 ─ loopback Gateway
    └─ MQTTS ────── Nginx :8883 ─ loopback Gateway
                                      ├─ HTTPS ─ 既有 Aily 测试 Auth
                                      └─ 隔离内部网络 ─ Broker 0 / PG / Redis
```

只有 Gateway 有出站网络；Broker／PG／Redis 无公网端口。HTTP 80 仅 ACME／HTTPS 跳转，移除旧 1883 Plain 入口，内部／admin／health／metrics 不透传。私钥只留服务端私有目录，Broker 仅获得 MQTT JWT 公钥；CloudSession 复用现有 DeviceKey proof，不因未配置 BLE／任务 grant signer 而另造登录协议。

服务源码、配置与职责沿[Service beta 最小模板](../../aily-blinker-services/deploy/beta/README.md)。正常 `/me` 原样同步资格；本测试环境显式 `BROKER_ROUTING_POLICY=shared`，原静态 resolver 为 Free／Pro 分配共享承载，不改 Auth 权益／`eligibleMode`，不以 SQL 造 tenant 或用户。实际路由／`currentMode` 与资格分离；其他环境默认 `entitlement` 保持原行为。H2 初始化仅建表，不自动创建策略。

## 3 执行顺序与当前状态

| 顺序 | 最小范围 | 当前状态 |
| --- | --- | --- |
| C1 | 只读主机／域名／证书／上游 Auth 核对 | 已核；原服务器仅入口，无 IoT 后端，API 502 |
| C2 | 新鲜 Nginx 配置备份、独立目录／密钥、完整源码冻结与传输核验 | 已完成；528 文件逐项一致，无本机数据导入 |
| C3 | 公开编排／精确入口及有限配置回归 | 9/9；实际私有 Compose config、主机 nginx -t 与 reload 通过；80 跳转、内部 404 已核 |
| C4 | 基础镜像固定摘要；唯一串行 Gateway→Broker 构建 | 已完成；两个完整运行镜像构建成功，Node／PG／Redis 均固定摘要。PG 900 s 超时后仅续拉一次，原 intent／日志保留，未重复构建 |
| C5 | 仅本项目启动；迁移／存储／真实路由与 TLS 运行核对 | 四容器 healthy、56 项迁移精确一致、SQLite quick_check／H2 空表、内外端口隔离通过；公网 HTTPS 与 WSS 101 通过。10-20 安全组调整后公网 MQTTS TLS 1.2、证书链及域名验证通过；不等同 MQTT 登录 |
| C6 | 真实账号／新设备、匹配 App／COM11 固件及有限 H2 续验 | 有限短验及保全完成：真实登录／新设备与 Key、HTTPS／MQTTS 接入、原控制 ACK／新鲜 State、60 s 同值归档、单板 90 s 断线恢复新段／曲线断段及正常停用停止；COM11 原完整 8 MiB 精确恢复并独立 ROM 校验，App 最终账号／journal／native 保全及连接回收通过；不代称旧本机策略已收尾 |

初始完整运行源码冻结摘要：`9f161627001ff7d37dfb9572f6116ecd4ebe4370a65e04a0c7f1c5e56cb187b3`。来自必要 workspace、原 lock／Dockerfile，不是仅把 13 个 H2 JS 覆盖到未知底座；528 文件在打包前后及云端解包后 hash 全部核对。新增 beta 公开模板独立冻结，实际私有实例和密钥不进入源码、镜像上下文或 Git。C6 只更新 Gateway，当前差异及镜像见下节，不把初始版本冒充当前 Gateway。

本批构建结果：Gateway `sha256:21cd9611dcdf94668a89b82e1e007d24e1ca2e080dffc7f148f836dcc1ab31e4`；Broker `sha256:4980ebde40acc693a8e8e6c83f71a8b40cddc6d483db790c9b0eedf34b1f6339`。这是构建证据，不是运行健康或设备验收。

### C5 实际运行及有限恢复材料

- 只启动 `blinker-iot-beta` 的 Gateway、Broker、PostgreSQL、Redis，均 healthy；原 stopped `hello-world` 不动，未部署另一套 Auth。实际 Gateway／Broker 镜像与上述构建一致，其他项目未启停。
- 正常启动完成原 56 项迁移，实际 ID 与本批源码精确一致。Broker managed shard 0／storage healthy、SQLite `quick_check=ok`，H2 两表均存在，策略／样本均 0，无默认归档。未创建用户、设备或伪造历史。
- 主机 `ss` 明确核对 7000／18883／8884 只绑定 loopback，3000／5432／6379 没有 host listener，1883 不存在。新 Gateway 实际公布 `mqtts://mqtt.yiyu.pro:8883` 与 `wss://iot.yiyu.pro/mqtt`；不是只检查模板。
- 公网 HTTPS ALTCHA 200、无凭据 me／devices 401，HTTP 308；internal／health／未开放自动化路径 404。WSS 返回真实 101 并校验 MQTT subprotocol／WebSocket accept，不代表已完成用户 MQTT 登录。
- MQTT 服务在服务器 loopback 的 TLS 1.2／证书链／`mqtt.yiyu.pro` 主机名验证成功；服务器 DNS 为目标 IP，INPUT ACCEPT、UFW inactive。服务器公网回连 8883 在 10 s 内未完成；电脑 DNS 被 TUN 改为 `198.18.*`，指定真实 IP 的原始 TCP 探测也存在代理干扰，甚至 1883 出现 TCP connect 成功的假阳性，与服务器实际无该 listener 矛盾。因此电脑 TCP 结果不能证明真实公网监听，8883 TLS reset 也不能直接归罪服务。公网 MQTTS 仍单列待验，不关闭 CA／域名校验或回退 Plain 来凑通过。
- 手机单独只读对照：活动默认网络 WiFi、未观察到 VPN，域名和真实 IP 的 8883 均 5 s 超时，而两者 443 均可达（约 174／185 ms）；未发送 MQTT 凭据。说明问题不只属于电脑 TUN。已请求用户核对云安全组入站 TCP 8883；SSH 内没有云平台安全组管理权限，不假定已修改，也不开放 1883／数据库端口。
- 用户确认云安全组需同事处理，当前停止该路径的重复探测；保持已部署服务与配置，SSH 运维会话已正常关闭。交接仅需核对该实例绑定的云安全组并放行入站 TCP 8883（MQTTS），不改宿主已通过的 TLS 配置。收到完成通知后仅补一次独立网络连通／TLS 差异，再准备 C6 的匹配 App、新账号／设备及云端 profile；不把 WSS 可达视为 MCU 已支持 MQTT over WebSocket。
- 云端保留初始 PG 原生 `pg_dump -Fc`／archive list、SQLite native backup／quick_check、独立 env／密钥及 manifest。首次 Docker archive copy 失败后保留 intent／首错，仅只读转移同一已生成 SQLite 快照，没有重做业务写入；首次 copy 的底层 stderr 未保全，不宣称完整。此为新空环境的同机恢复材料，不等同协调全栈或异机恢复演练。

## 4 部署后的最小验收

1. Gateway database 与 shard 0 healthy／acceptingNew、Broker managed／storage ready；实际 PostgreSQL 迁移 ID 与源码一致、SQLite quick_check 和 H2 表／索引有效。确认新库没有默认策略或伪造样本。
2. HTTPS 注册链路及实际区域／权益正常；WSS／MQTTS 经真实证书与 Gateway，不绕过 Broker ACL。无凭据 API 401／受限路径 404 不等于登录、MQTT 或设备已验收。
3. 新测试账号经正常流程新建设备，复制该环境 DeviceKey；匹配 API 域名、TLS／CA、profile 后才编译／刷 COM11。先核身份／原完整备份／授权，使用单一编译队列和原 SDK；不清 App 旧账号数据来切环境。
4. 新环境完成一次控制 ACK／新鲜 State；H2 新策略 60 s、真实新序号／来源／时间／有界曲线。已有本机同值、离页／关闭 App 与共享 HTTP 证据不重跑；新环境必要的部署匹配差异明确记录。本环境恢复后新段／缺口、停用停止已按下方有限窗口补齐，不补造离线点，不据此改写旧本机中断记录。
5. 测试后通过正常 owner 停用新策略，保留 revision／历史／完整请求日志；恢复 COM11 原完整 8 MiB／授权并独立 ROM 校验，回收本批连接。旧本机策略另在原环境恢复可用时只读协调并停用，不复制／重放旧 UNKNOWN。

### C6 初始接入缺口（处理结果以下方记录为准）

- 10-20 公网差异：真实 IP `42.194.131.140:8883` + SNI `mqtt.yiyu.pro`、强制 TLS 1.2、标准证书链／域名校验成功；HTTPS 与 MQTTS 又分别以同一 E46 自签根单独验证通过。保留原安全组阻断证据，不再以过去的超时判断当前状态；该项只关闭公网 TLS 缺口，不声称账号、MQTT 或 H2 已验收。
- 生产 Key 管理目前始终返回 `DEVICE_KEY_STEP_UP_UNAVAILABLE`：创建接口不返回原始 Key，而 reveal／rotate／enable-cloud 尚无可信 recent-auth 接线。最小修正只接受本 Gateway 代理的真实邮件／密码登录成功，记录短期、区域与访问 token 指纹绑定的服务端证明；仍执行原身份／owner 检查，refresh／me 不建立或续期证明，存储失败关闭 Key 操作。禁止启用生产不安全开关、从数据库取 Key、信任客户端时间／未校验 JWT 的 iat，或将 Key 塞入未经审阅的创建响应。先有限回归并匹配部署，再发真实邮件。
- 本批普通 WiFi 零槽图不启用 BLE／任务。交付 renderer 当前仍要求两类业务签名公钥；MQTT JWT 公钥和 TLS CA 均不能拿来冒充。匹配 profile 必须独立解决真实签名环或明确拒绝不支持功能的最小配置，不能沿用本机 RFC 演示 Key。
- 手机当前匹配 H2 的 debug APK 固定本机 API；“自建服务器”页不会修改 V2 API，地区切换也不是环境切换。手机已有的另一正式包缺少 H1／H2 页面标记，不能充当匹配客户端。先保留这两个包、账号与 journal；后续在隔离构建目录固定云端 API，生成并列 beta 包，不用运行时 JS 覆写或清旧数据来伪装正常切环境。
- 邮箱登录经新 Gateway 复用原 ALTCHA／发码／登录，账号不存在时正常注册。发码 HTTP 200 只代表安排投递，必须由真实收信确认，不读取验证码库或伪造 token。部署健康不替代该项。
- ESP32 原 SDK 3.3.11 的现有产品图已有 HTTPS DeviceKey 与 MQTTS 两条 `WiFiClientSecure` 路径，无需新增 BLE／MQTT 库或升级 SDK。候选必须分别设置 API TLS、MQTT TLS 和正确 CA，取得新环境新 DeviceKey；证书根、DeviceKey 和业务签名公钥不是同一材料。
- API 已只读验证 TLS 1.2 可用，云入口需保留它以适配该 SDK，不能仅开 TLS 1.3。MQTT 使用自身覆盖 `mqtt.yiyu.pro` 的证书链，须在服务就绪后独立验证；不能拿只覆盖 API 的叶证书代用。设备 TLS 握手仍受原 SDK 同步等待边界约束，短验前说明，不冒称性能已验收。

### C6 10-20 前序推进记录

1. **Key 保护已最小接线**：仅可信 Aily 完整邮件／密码登录响应为访问 token 建立服务端 5 min 证明，按环境、region 和 SHA-256 指纹隔离。读取／重复 remember 不续期，refresh／me 不建立证明；无证明为 `403 DEVICE_KEY_RECENT_AUTH_REQUIRED`，依赖不可用仍 `503 DEVICE_KEY_STEP_UP_UNAVAILABLE`。Redis 命令 1 s／最多 16 未决、绝对过期及 logout tombstone／在途读取 fence；不存原 token、不改 owner／创建／rotate 合同，不声称独立 MFA 或依赖故障后跨实例即时注销。原生产不安全开关仍 false。
2. **有限回归**：recent-auth 单元及真实 HTTP 接线 13/13；原 Aily client／identity、WeChat HTTP 通过；Gateway `tsc --noEmit` 通过。新 HTTP 用隔离身份／业务边界测试，不代表云端真实用户登录已通过。
3. **匹配部署**：以原 528 文件归档逐项验证为底，仅覆盖本轮 index／module／两个测试／package 测试命令；531 文件中 526 项原字节不变，依赖与锁不变。新 source `88a1ba0e3fe69f836c9d88e9fa591a1f6393aed4dab2b6bef17d0b9a095599b2`，archive `ce1c999fc88a215e366daf11203d5fa098abe44e8c5a439c3676fd2f77259cfa`。先新鲜 PG 原生备份＋archive list、SQLite native backup＋quick_check、env／密钥／镜像／完整日志，再唯一 Gateway 构建；不新增迁移、不重建 Broker／PG／Redis。当前 Gateway `sha256:7c212ac380de207b4834a3673718f9bacd62c32074402ec25b2c0c2739a173c0`，其余镜像与 C5 一致，四容器实际 healthy。只保留镜像／配置前向小更新；这些原生备份不是协调全栈或异机恢复演练。
4. **独立 App 尚不可用**：`iot.diandeng.tech.beta`／“Blinker 内测”，固定云端 HTTPS API、HTTPS WebView origin、禁明文／本机 CA；唯一隔离 Web→Android 构建及签名／APK 资源一致性通过。APK SHA `52f76f550099aa554988c9be8fd1df133da6df1db002cf41e5c2231186646657`。安装前新鲜备份原 release／debug 的 APK、完整数据与账号／journal，安装后两包逐字节 hash 全等。但 beta 两次正常启动在约 70～80 s 出现 WebView renderer JS heap 约 2.7 GiB 的 `V8 javascript OOM`，继而主进程 SIGTRAP；第二次仅轻量 DOM 读取仍超时，不能将第一次全库读取直接认作原因，也不能把安装成功当页面可用。手机物理 RAM 未耗尽。旧 debug 实际 APK 与新 beta 的所有 `src/app/*` source-map 内容逐字相同，只存在环境／独立空 sandbox 等差异，不能将同事首页修改当根因或盲回滚。
5. **COM11 未动**：私有普通 WiFi stage 已逐项核对，仅派生 TLS／CA、两个 typed-null 空签名环及真实源码身份四头；0 槽且禁止 BLE／WiFiBLE／LAN／EdgeHub。空 verifier 返回 `NotConfigured`，不需要生成未使用的业务签名私钥，也不复用 fixture／MQTT JWT 公钥。凭据仍保留 `#error`，无新云 Key，因此没有编译、串口操作或刷写；原完整 8 MiB／授权及恢复证据保留。

**启动阻点的充分证据及最小修正**：一次明确 intent 的冷启动只取两次暂停栈／heap数字，不执行页面脚本或扫库；首次 heap 约1.19MB，短 resume 后增长118,556,532B，热栈落在 `loadPluginImplementation → internalGetItem → ZoneAwarePromise`。旧真实 APK 有19项 `assets/capacitor.plugins.json`，本批隔离 APK 缺该文件；私有冻结排除了生成注册表，却只运行不会生成它的 `cap copy`。缺少 native Header 后，SecureStorage Native implementation 的 `internalGetItem` 已绑定同一个 Capacitor proxy，fallback 再回到该 proxy，形成无界 Promise 链。证据闭合到本次隔离打包遗漏，不是账号／TLS／首页／依赖或 SDK 升级问题。窗口结束已停止未登录 beta并回收专属forward。仅补正确注册表与 APK／原生来源门禁，再用同一 Web 快照执行一条增量离线 Gradle 队列；不改第三方库／业务页面或重做原构建，不覆写首错 APK。

**修订与真实发码结果**：同一 Web 冻结输入，官方 CLI 生成19类注册表并核对20个原生工程来源、恢复原隔离 Gradle 路径；v1缺表 APK 被新增门禁拒绝。一条37秒离线 native 增量队列产生 v2，APK SHA `758d42bc82fe0b067afce75f7c95018dff0d27d329a57194fe5b345e68e94da6`，实装读回匹配；原两包／依赖／SDK／业务及 Web 资源不改。真实 `/login`、未登录、`SecureStorage.internalGetItem` native Header 就绪；60.25秒 heap 16.90→16.90→11.60→11.61MB，正常GC，无再膨胀。首包／首错保持，非静态检查冒充运行。

随后在真实登录表单输入用户提供的邮箱并仅点击一次发送：ALTCHA及发码业务响应分别200，页面进入60秒倒计时；保留HTTP requestId。首个测量脚本误取同 URL 的 OPTIONS 204 而报失败，原结果保留；只读协调后续业务200＋真实倒计时，不重复发码、不删除错误证据。接口接受不代表邮件已送达或账号已登录，已请用户通过真实邮件输入验证码。没有读验证码库／Auth secret／邮箱或伪造 token。

**真实登录及当前承载阻点**：用户提供真实邮件验证码后，仅通过 beta 正常表单提交一次，未把验证码写入文件。App 自然进入 `/home/device`，token pair／账号存在、云设备0；HTTPS `/me` 200。实际账号是现有 Pro，而不是新 Free 身份，`iot.devices=10`、`iot.dedicated_broker=true`；subscription 的显示字段 `service_tier=shared` 不改变现有 IoT 合同——`database.js:getEligibleBrokerMode` 按该专用权益布尔值选择 dedicated。本环境只配置 shard0/shared、专用分配／Controller关闭，不能将“具备专用资格”偷偷改为shared、修改 Auth 权益或SQL造路由。只读资格核对在任何创建设备表单或POST前拒绝，本批未创建云设备／Key，也不存在需要重放的设备创建请求。新 beta 账号已在私有目录保全，旧两包及COM11未动。

**承载选择已明确（10-20）**：用户不增加专用 Broker，并确认统一使用当前 `api-test.yysc.tech`。由显式部署策略选择共享承载，而非将账号原有 dedicated 权益改为 false。Blockly 正式 `api.yiyu.pro` 等其他 Auth 环境不自动互通，不合并用户库、不跨主机试送现有 token，也不拿地区选择冒充环境切换。

### C6 共享账号与单 Broker 收口

- `BROKER_ROUTING_POLICY` 默认 `entitlement`，本 beta 明确为 `shared`；解析拒绝非法值及 reconciler／动态创建／专用预热冲突，要求 shadow 打开。资格 getter／Auth 数据不改，初次路由与迁移目标只选择健康共享分片；无容量即失败，不暗回退专用。不迁移／替换既存 route/version；启动在任何 allocation 更新前拒绝专用或未知路由。专用启停／重启和运行恢复入口禁入，历史 operation 保留。
- 纯策略／真实配置／路由／管理／原 admission、allocation、runtime、capacity 共22项有限回归；beta Compose／Nginx／普通 TLS session 合同9项；原 recent-auth13项及 Gateway类型检查通过。纯内存双租户覆盖不是云端双账号真机验收，不据此关闭真实设备 ACL／H2。
- 源码以当前 C6 531项冻结为底，仅覆盖11项审阅输入；新538项中527项原字节不变，依赖与锁不改。source `49c029b5a76085c66dcc0e37d20c470616a8ebddd90a98f6810b446221ead02a`，archive `f26bf6d45f52d5c8deae2ad38f4bad3efcdab49e5cab1d97d1b99cd8c5ec711b`。云端新鲜 PG dump／archive list、SQLite native backup／quick_check、配置／密钥／完整日志已保全；只读确认 routes／Controller／迁移均0，未通过 SQL 创建／修改路由。
- 唯一 Gateway 构建已完成；首次 Compose 比较因发布目录的 build context 绝对路径不同而拒绝，核对仅这两路径及策略差异后有限续接。worker启动后运维记账局部变量错误保留原 stderr；读取同一队列真实完成／镜像结果后协调，不另起构建或删除首错。
- **实际部署与账号短验通过**：Gateway `sha256:33d87a0bac01030c4c3e5440fedfe5929e572af434174159c1bbea2931854a03`（取代上节Key修正时的`7c212ac3…`），运行 Compose `/opt/blinker-iot-beta/releases/49c029b5a760/deploy/beta/compose.yaml`；其他三容器镜像不变、四项均healthy。实际 `shared`／reconciler false／shadow true／dynamic false／prewarm0、原不安全Key开关false逐项核对；只重建Gateway，无新Broker／迁移／Auth修改。备份及完整新旧日志留服务端私有`c7-shared-routing`，不是离机恢复演练。
- 真实手机原账号保持登录，HTTPS `/me` 200（916 ms，requestId `7b2d32aeae27fb97dd820c584a0c898b`），原 `iot.dedicated_broker=true`；正常 `/api/v2/account/connection?wire=bbp2&pv=2&transport=websocket` 200（155 ms，requestId `5039fece753be53030c5745dea5d1a10`），`shard0 / route_version1`，无 SQL 造路由。随后调用现有 App 账户客户端 `start()`，idle→ready、会话ready，794 ms，复用原 WSS／CONNACK／SUBACK 接线，不另造第二个 MQTT 客户端。只读账本确认1条 shared route、eligible dedicated1／current shared1／desired shared1、Controller0／迁移0。账号凭据及完整响应仅私有保全，原两包／授权、COM11和COM9未动；该共享承载阶段尚未创建云设备／Key、编译或刷板，后续结果以下节为准。

### C6 新设备接入与 H2 续验：实际有限通过

- 接回后重新核对手机及 COM11 的 USB／ROM 身份，COM9 不操作。COM11 由唯一官方 ROM/no-stub 队列新读完整 8,388,608 B，再独立整盘验证并恢复原程序；本批 SHA256 `7e3616e0d311672e4d13291dc8172180e55f1a6a5ec756e618dbab4ba127ed21`，虽与旧备份相同，恢复来源仍严格取本批新鲜文件。原分区／NVS、授权与首 intent 保留，不套旧备份冒充新读。
- 匹配 beta APK SHA仍为`758d42bc…e94da6`，先新鲜保存账号／任务和自动化 journals、完整 APK及4,595,200 B native数据；只停启beta，原两包不动，owner与journals跨重启保持。云端另取新鲜 PG native dump／archive list、SQLite native backup／quick_check和全日志、env／密钥；四容器原镜像与配置不变，不重启共用Broker。
- 正常退出后仅发出本批一次邮件请求，HTTP200，requestId `9cf5f79a2c1dc548ce216901986a3483`；测量时未见倒计时，保留该现象，不由HTTP状态推定送达。随后用户收到本次新验证码，真实App正常表单登录成功，requestId `18b23c72065420e7a52a9151276acd61`，me／新设备列表也200；未记录验证码或读取码库。随即正常App创建新设备并reveal Key成功，Pro资格保持true、路由shared0，不从DB取Key、rotate或重放旧验证码。本批Key与账号完整材料只留私有目录。
- 432项普通 WiFi 冻结库逐项复核无变化，0槽／空业务签名环／原 SDK 3.3.11不变；新 Key、当前测试网络和既有 HTTPS／MQTTS profile 绑定为一份候选，唯一 `jobs=1` 队列已完整链接通过，BIN 1,036,528 B，SHA256 `5ca02e29cee27d08b87eb47abaacbe49069d859b49c9e37e64dbb2c40ce4d01e`。随后仅 COM11 刷入并完成本批真实 App 验收；生产源码／默认示例／App／服务不为夹具改写或重建。
- 设备真实使用本环境 DeviceKey，经 HTTPS 登录及 MQTTS 接入。原 App Cloud 控制两次 ACK 为202／224 ms，新鲜 State 为203／224 ms；单板断线恢复、正常停用归档后，再次控制 ACK 为259／144 ms，新鲜 State 为259／145 ms。本批控制 State revision 2→5，不能把 MQTT 发布成功或 ACK 单独当作新鲜 State。
- 正常 owner 页面只选实际数值能力并启用固定60 s归档。两个恒定模拟值42的真实新样本，设备 monotonic 相差60,000 ms、服务器接收相差59,995 ms；这不是物理传感器读数，也不是按值变化保存的 H1。
- 仅目标板私有 Sketch 的一次性 `H2:GAP90` 请求断开自身 WiFi、暂停自身 `run`，实测90 s；期间真实运行态为 `unavailable`，原两条历史逐行不变。恢复交原生命周期，出现新 segment／stream，epoch 2→3、新样本 sequence=1，共3条；App 显示2条曲线路径／3个点，不跨段连线或补点。旧样本至恢复样本接收间隔132.767 s包含恢复等待，**不是90 s断网时长**；当前同段空窗阈值180 s，不能仅用时间差证明断段。共用 Broker／服务未作为故障目标、未重建。
- owner 通过正常页面停用并收到策略 revision 2；77.829 s后只读查询的3条样本逐行完全不变，原控制仍取得上述 ACK／新鲜 State。云端新测试设备、停用策略与3条历史均保留，不删库、不删除设备凑空环境。COM11 原完整8,388,608 B已按本批新鲜备份精确写回，全范围独立 ROM 校验及 hard reset 完成，原身份／授权恢复；未复验原业务上线。恢复原身份后本批云端测试设备离线符合预期，不冒充失败或旧身份迁移。
- App 最终完整 native 备份4,625,408 B，SHA256 `b968fe65e6c13dd8055cb7a2bc7b49f287512cda54c0d112926c6f8c796b998f`；仅 beta 正常停启做一致备份，重启后原 owner及全部真实任务／自动化 journals逐行全等，APK不变。保留本次真实新登录session，不还原注销前token；回首页后 device scopes／BLE links均0、本批专属 forward已回收。原 release／debug、R4和服务配置未改。
- 原SDK实际`MBEDTLS_HAVE_TIME_DATE`未启用，普通WiFi无已证实的“先NTP才能HTTPS”死循环；不为本批加NTP／升级SDK或关闭校验。MCU验收仅按实际报告TLS／CA链／hostname，**不声称该SDK验证了证书有效期**。App的recent-auth过期403目前缺专用提示；本批先真实登录后取Key，不把误导文案当owner变化或通过重建／rotate绕过，另保留最小提示修正项。

本环境必要的 H2、原控制短差异及最终固件／App保全已完成，云端前后 PG native／SQLite native 备份及健康核对通过；完整新旧日志、授权与恢复材料私有保留。本机旧策略仍是最后已知 revision 1／enabled、候选期间新记录未知，待原库只读协调，不能由这次云端 revision 2停用代替。后续提示源码修正及安装边界取下节；本次发码200但未见倒计时的现场根因仍待核，真实OTP登录已通过，不为此重发验证码。不重跑已通过窗口、不扩 H2b／离线历史／Bulk，不提交推送。云端细项同时见[H2 §8.6](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md#86-云端独立环境的有限续验与收尾)。

### C6 后续：Key 提示与发码生命周期（2026-10-20）

本批只做已知接入体验缺口，不扩 H2 能力或重新部署服务／刷板；修改前保留 App 相关源码快照，现存 H2 与无关本地修改不回退。

- **错误语义优先于 HTTP 状态**：guide 与 settings 将 `DEVICE_KEY_RECENT_AUTH_REQUIRED` 明确提示为先正常退出／重新登录，再在原设备设置查看或刷新 Key；不再误报“只有设备所有者”。`DEVICE_KEY_STEP_UP_UNAVAILABLE` 单独提示安全校验暂不可用，其他原 owner 拒绝与网络失败保持既有合同。没有放宽 recent-auth、ACL 或服务端证明。
- **保留已建设备**：guide 新状态禁止再次 create／reveal，按钮只复用 `finishKeySetup()` 前往原设备；不自动注销、重发 Key、rotate 或创建第二台。settings 原刷新幂等 ID 及未知结果保持，失败不复制 Key；原次级 reveal 路径删除原始 HTTP 错误日志，复用相同提示。中英文本同步，不为两处判断新增通用框架或身份验证流程。
- **登录窄保护**：源码审阅确认发码未返回时重复点击会给同一 Auth promise 增加多个倒计时续体，以及销毁后晚响应仍可能新建 timer。仅补发送中／销毁护栏、加载完成后的发码前禁入和旧 timer 清理；初设60及每次计时变动显式 `markForCheck()`，复用原 Auth、ALTCHA、Notice 及固定60 s合同，不扩完整缓存离页或网络取消机制。
- **有限验证完成**：guide、真实模板、settings、interceptor四组111项通过；原Auth9项和最终登录8项通过，共128项，未重复已过组。覆盖倒计时60→59→0／冷却禁发、真实模板、503后手动重试、HTTP200业务拒绝、并发单timer、销毁晚响应／加载后禁发及无效邮箱；登录用例使用真实模板／Auth与离线HttpTesting，其余用例使用本地服务替身或HTTP测试后端，不会投递邮件。26个模板翻译Key在中英均可解析，其他翻译内容与修改前一致。首轮登录2通过／4个 `NG0100` 失败保留：本地Angular TestBed默认无Zone，非dirty视图未刷新却执行checkNoChanges；直接异步改countdown缺视图通知。补显式通知后原检查仍开启，无skip／断言弱化。生产App显式启用Zone，故这不是现场单次不显示的充分根因证据。
- **环境边界**：本机 Docker 命名管道仍不可用，未启动／排查 Docker 或操作旧卷，旧 H2 策略仍按最后已知 revision1／enabled、后续记录未知保留待协调；不把云端停用代替原环境收尾。云端共享 Broker、数据库、账号、授权、UNKNOWN 和完整请求日志不变，本批没有发真实验证码或操作手机／开发板，不提交推送。

该软件批次结束时手机仍为原beta v2，源码回归不代称新APK或手机现象已验收；随后按下节用户明确范围完成匹配安装与既有设备只读差异。旧本机策略仅待原Docker自然恢复后核对原镜像／卷，再只读协调和正常owner停用，不启动新环境代签。

### C6 匹配 beta 安装与只读 Key 提示短验（2026-10-20）

本批限定为现有账号／设备的一次正常“查看设备 Key”点击，不发送验证码、重新登录、创建设备或刷新Key，不刷板、不重跑H2／BLE，不部署服务或改云端配置；源码仍取上批128项回归，不重复运行。

- **先新鲜保全**：原beta APK读回摘要为`758d42bc…e94da6`；完整native备份4,626,944 B，SHA256 `217ffa2b65577a8afa25a012814065e05b5057f5776dcd73bfaaf3b33ee53c4a`，tar内容及APK核对通过。首次重启后过早读取到未完成水合的设备version／locator，原断言首错保留；后续仅只读读取，字段自然加载后与原设备上下文一致，owner及全部任务／自动化journals保持，没有重复备份、补写字段或重放业务。
- **唯一匹配构建**：从原1980项冻结输入派生独立目录，仅覆盖guide TS／HTML、settings TS、login TS及中英JSON这6项，覆盖前核对上批before快照与原stage逐字相同。继承原3项模板产物及3项生成原生输入，共1986项；12项依赖摘要、5插件dist、20原生工程及19类注册表前后核同，原stage／v1与v2 APK／manifest不改。唯一串行类型检查→Web→cap copy→原SDK／JDK离线单worker Gradle→签名／包名／逐资源验证通过；1647项Web资产与APK逐项一致，注册表SHA仍`e869f119…b9722`。不重跑模板构建、不安装依赖或升级SDK。
- **实际更新**：仅`iot.diandeng.tech.beta`正常`-r`更新，包签名保持，原release／debug不覆盖；新APK 82,076,245 B，SHA256 `8d6b001dd34d9fe680699c6f1ff881745edf49fc14715d3a9a41254633ad71c4`，安装读回完全一致。仍固定`https://iot.yiyu.pro`、HTTPS WebView、禁明文／本机CA；原生SecureStorage真实Header及原账号／journal／设备上下文启动核对通过。不回灌数据或注销前token。
- **一次真实只读请求**：正常现有owner设备settings中，仅点击可见“查看设备 Key”一次；同JS轮复核目标／owner／epoch／context／按钮状态。实际reveal POST返回403，HTTP约155.4 ms，requestId `d99510fe08e8347c33234f6dcab31f3c`；页面显示“请先退出并重新登录”，不出现所有者误报，Key始终掩码、busy结束，设备version及owner／epoch未变。保留实际页面截图和脱敏逐请求元数据，不读取Key响应正文或凭据。完整账号备份另在私有目录保留，不能把整批备份说成“不读取任何token”。
- **首错与只读协调**：原工具把“零其他POST”当作额外条件，因观察窗中一笔`POST /api/v2/notification-installations`而首停。保留原trace及首错；只读核对原／新三份通知相关源码逐字一致，该请求属于既有独立启动／通知注册链，不是Key或认证重放，不能仅由时距断言是通知首发还是其内部重试。基于同一trace另立协调证明：原Key请求只有一笔，403响应后超过5 s未见Key／认证重试，无设备创建／rotate；没有第二次点击。通知请求在窗口结束前未取得响应，完成状态仍未知，不补发、不宣称通知注册成功或“所有其他POST为0”。
- **最终保全**：新版beta保留，完整native后备份4,627,968 B，SHA256 `08475f96c7929ede6a94ab048ba8c6864057be471cf4a77f0286d8f8acba35f2`。正常重启后owner、全部原任务／自动化journals、设备身份／凭据上下文保持；原两包APK摘要不变，回到首页，device scope／BLE均0，本批专属ADB forward已回收。新旧请求／首错、App日志、截图及恢复材料私有保留；没有清授权、处理旧UNKNOWN或修改服务／其他板。

**本批有限验收收口**：匹配beta安装及settings真实recent-auth403提示、无Key／认证自动重试和保全已通过；构建与安装分别记账，不覆写构建时`installed=false`的原候选证明。Guide的503／近期验证分支取既有软件回归，不冒称本批真机覆盖。现场旧发码倒计时现象、通知注册未确认和旧本机H2策略协调分别保留，不扩大为新功能、完整通知适配或重派H2；未提交推送。

## 5 恢复与支持边界

- 操作前保存实际镜像、配置、证书／公钥指纹及数据。全新环境生成独立 secret；不得凭证书相同或模板通过推定私钥／数据已备份。既有 Nginx ACME 和默认站点保留，配置改变须先比较新鲜 hash，禁止覆盖其他会话部署。
- 新库产生 H2 策略／样本后不回滚为忽略 H2 的旧 Broker；不靠 SQL 删除日志／旧 UNKNOWN 或单库回灌“恢复干净”。一致备份复用现有 PG dump、SQLite native backup 和协调物理备份合同。
- 当前 Gateway Redis adapter 无密码配置；专用 Redis 仅在无 host 端口的隔离内部网络可达，不谎称启用 Redis ACL。宿主 Docker 权限、镜像 root／完整依赖、异机恢复与长期容量／监控仍是受限内测边界，不展开全面资源审计。
- H2a 是低频在线 best-effort，接收时间／receipt-only，不承诺突然掉电绝不丢样本、离线补传或设备持久提交 ACK。新部署健康、新账号登录、手机／MCU 短验、同事独立签收分别记账。

## 6 H2a 收口后的执行顺序（2026-10-20）

目标是让现有核心功能通过正常源码与配置稳定复现，不因 H2a 短验结束自动扩大协议。C6 已通过项不重新排队；下列阶段按顺序推进，各自保留软件、构建与运行的证据边界。

| 顺序 | 最小实施范围 | 完成条件 |
| --- | --- | --- |
| D1（已收口） | 普通云端接入与部署能力对齐 | 单一库公开profile、普通ESP32 WiFi／0槽／HTTPS＋MQTTS、匹配beta构建安装及控制短差异取D1a～D1c；普通页面配置精确公开及真实只读／回收取D1d。各层证据不互代，不扩大产品组合或称完整生产发行 |
| D2（有限范围已收口） | H2a 历史时间窗口查看 | 原查询／cursor、固定UTC窗口、前后翻窗及200条字段上限保持；D2a软件、原工具链匹配构建和D2b既有3条历史只读短验分别通过。后台恢复首请求的renderer trace间隙明确保留，不当作全程网络／后台DOM证明；不重跑采样或将7天窗口称完整曲线 |
| D3（D3a～D3c已收口） | 将已验最小自动化启用到云端 | 部署／beta及两设备真实后台触发、唯一execution、ACK／新鲜State、规则正常退役及固件／授权恢复均完成，详见D3c；不再重跑M5／N5或该云端短验 |
| D4（单设备有限支持已收口） | 将已验ESP32四槽持久任务接入新云环境 | 真实P256／双侧单ID、四槽TLS原SDK链接、匹配beta／服务部署及现有COM11的一次任务闭环通过；匹配固件／Store／scope保留，不重新实现任务／CAS、不放宽公开0槽profile或自动扩名单 |
| 后置 | H2b／H3、离线存储／Bulk及新协议 | 只有明确需要设备采样 UTC／时间质量、持久提交 ACK 和有界重试时才开 H2b；不将它们列作 H2a 的遗留修补 |

### D1 本批冻结范围

- 先核对现有 release renderer、普通 `OfficialConfig.h`、已验 cloud 候选和 App 构建配置。公开材料仅含域名、TLS CA、功能开关；不放 DeviceKey、账号、token、WiFi 密钥或私钥。
- 复用现有单库打包及配置校验。支持未配置业务签名环的普通 WiFi 产品必须显式限制可用图；不能因此将 BLE／WiFiBLE／LAN／网关／任务变成假可用，也不能弱化已有完整 profile 的校验。
- 不覆盖普通库的自建／DIY profile 或把 R4 Plain 四槽与 ESP32 云端 TLS 图混为一个已准入组合；不增加按 MCU／模式分裂的 ZIP。
- App 的公共 API 已为 `https://iot.yiyu.pro`，本批不再重造环境切换。明确正常 beta 构建入口，并对本环境关闭的通知等调用及相关入口做最小接线，不新增动态能力发现框架或完整通知服务。
- 软件验证优先复用现有回归、生成／打包、原有 SDK 的定向验证；不部署服务、不刷板、不发验证码、不创建或刷新 Key，不重复 H2、BLE 或断电循环，不提交推送。
- 原本机归档策略待原环境可用后正常 owner 协调；旧通知注册未知结果、自然登录时的倒计时现象及 Error 8 单独保留，不为本批重放 UNKNOWN 或开启重复采样。

### D1a 配置与软件收口结果（2026-10-20）

- **设备库**：原 renderer 新增显式 `esp32-cloud-wifi-v1`，公开输入为 `profiles/cloud-beta.json`。仅准普通 ESP32 手动 WiFi／0 槽／HTTPS 443＋MQTTS，复用已验 Sectigo E46 根（DER SHA-256 `c90f26f0fb1b4018b22227519b5ca2b53e2ca5b3be5cf18efe1bef47380c5383`）。MQTT host／port 仍由原受信 session 下发，不新增连接。两类空业务签名环使用 typed-null／signature=0，不借 RFC fixture、JWT 公钥或新私钥冒充。
- **图隔离与单库**：生成配置／签名头在 include guard 外重验范围，BLE／WiFiBLE／配网／LAN／网关／任务及非 ESP32 消费者拒绝；先读配置再选择禁用图也拒绝。普通源码默认值、自建 full profile、R4 Plain 维护路径不变；原 development 和显式任务 profile 的三个配置头与旧 renderer 逐字节一致。原单库 ZIP 名称／结构不变，安装门禁只允许本 profile 的 `esp32-wifi`、`esp32-cloud-smoke`，不把它冒称七目标云端准入。
- **App**：环境共用一个小型 `DeploymentCapabilities` 配置，当前云环境关闭通知、消息、任务、自动化、Direct、定向邮箱邀请和 Console 链接；已有 local／local-device／优先级实验保留原功能。复用原服务、菜单和路由 guard，不另造授权／CAS／执行器。通知关闭时只做一次本地 native stop，保留存储凭据与未决清理意图，不发起注册／撤销／权限请求；消息及 BLE 首页发现关闭时不发 HTTP／扫描／原生初始化或读取 BLE 凭据。原账户 MQTT、普通控制、H1／H2、非定向分享入口不因此关闭。
- **正常源码 beta 构建入口**：`pnpm run sync:cloud-beta` 先执行原模板／面板构建和 production＋cloud-beta Web 构建，再运行官方 `cap sync android`。Android `assembleBeta` 为并列 `iot.diandeng.tech.beta`，沿内部 debug signer，不改原 debug／release 包名；HTTPS origin、禁明文／本地 CA，构建前核对 beta 原生配置和 SecureStorage 注册表。不是用 `cap copy` 或私有冻结 stage 冒充源码重建。命令与安装保全边界取 [App README](../../blinker-app/readme.md#本地开发)。
- **有限验证**：设备库新 profile 17 项、原 package 33 项、handoff 11 项、R4 binding 12 项及 restricted release 11 项，共 **84/84**；另原 profile 校验及 443 文件发布边界通过。含真实头文件预处理／typed-null 语法、原发布 CLI、两次单一 ZIP 确定性及安装字节一致；测试临时 ZIP／stage 自动清理。App 定向 Angular **10 文件／168 项**、纯环境 **4 项**及 beta 构建入口 **5 项**通过，应用／测试 TypeScript 检查通过；独立只读审阅未发现阻断问题。
- **测试首错保留**：App 首轮 168 项中 76 项失败，原因是 Angular bundling 下新加的环境 mock 没覆盖相对导入消费者，旧“功能已开放”测试实际落在当前关闭的 cloud profile。删除无效 mock，改为每项显式选择 local／cloud 并恢复原配置后，同一有限集合全过；不是放开生产默认值或隐藏失败。普通服务／硬件／SDK／账号／授权／UNKNOWN及旧请求记录未操作，不提交推送。

本批只关闭 **D1a 配置和软件证据**。未执行新 profile 的完整 SDK 编译或新 beta APK 构建／安装，不把旧 C6 制品与本次源码视为同一新候选；独立签收仍另记。

### D1 步骤收口与下一批

1. **D1b 唯一隔离构建（已完成）**：两目标安装编译、匹配 beta 构建及来源核验取下节；不重派成功队列，不重复七目标／BLE／资源审计。
2. **D1c 必要新产物短差异（核心与保全已通过）**：实际结果取文末。新鲜备份、匹配 beta 安装、普通接入及一次真实控制 ACK／新鲜 State 已核，COM11 完整恢复；关闭功能结论限定实际观察窗口，不泛称全部原生网络零调用。
3. **D1d 普通页面配置缺口（已通过）**：实际Gateway已含两handler；根因是Nginx精确白名单漏项。只更新普通 `/console`／`/ui-config` 路由，保留原ACL、CAS及全部关闭功能。新鲜配置／PG／SQLite／日志备份、实际平滑加载及真实App带授权读取200／后台回收已核；详情取文末。不新造页面协议、迁移、签发图或控制执行链，不重跑已过项。
4. **D2固定历史时间窗口浏览**：软件范围及结果取下节；下一步仅匹配beta构建、先备份再做只读窗口短验，不启用新策略／刷板／全量拉取或同时开放D3／H2b。旧本机策略、通知请求未知结果、发码倒计时及Error8独立保留，不作为D1重测清单。

### D1b 两目标与匹配 beta 的唯一构建（2026-10-20，已完成）

- 开始前只读核对未有 Arduino／Gradle 构建队列；使用一条串行流程：单库生成／空 Sketchbook 安装 → `esp32-wifi` → `esp32-cloud-smoke` → 当前源码 beta。每步保存 intent、退出码和完整日志，失败不自动重派成功阶段；不部署、刷板、安装 App 或提交推送。
- 显式使用原 CLI 1.5.1、原隔离 ESP32 Core **3.3.11**，两个公开目标均为 `esp32:esp32:esp32`、jobs=1；不误用默认 Arduino15 的 3.3.6，不将通用编译称为新 XIAO 真机准入。PubSubClient 2.8 从原本机缓存复制至空 Sketchbook，只有本批 ZIP 和必要依赖参与。
- 单库仍为 443 文件，ZIP SHA-256 `3bc8ebf28db25f7f81aa1491660da643949012d3c00d07a0047e49a43a1303ff`。两个目标均已通过安装字节、实际依赖、链接闭包及来源检查：`esp32-wifi` Flash **1064996 B**／静态 RAM **52880 B**，`esp32-cloud-smoke` **1082880 B**／**52992 B**；分别365.8／351.6 s，整条安装编译门禁722.6 s。每目标94项编译输入、其中17项安装库 TU；实际 Core 来源和原平台摘要一致，无额外配置宏或源码树泄漏。ELF 含原 MQTT／TLS socket，不含检查范围内的 BLE／Noise／任务图。
- App 固定原 Node **22.22.3**、pnpm **11.19.0**、JDK **21.0.12.1**、Android SDK **36**、Gradle **8.14.3**／AGP **8.13.0**。不复用当前原目录由 pnpm 11.25.0 安装的依赖图来冒称 11.19.0 fresh install：独立目录将从原锁离线 frozen 安装，原工作目录／生成文件保持。
- 预检最小修正：源码导出纳入 beta 同步与现有面板打包工具，排除 `public/custom-panel-runtime` 的旧生成资产；面板 packer 的直接 `esbuild` 依赖显式固定为锁中已有 **0.28.2**，仅增 importer，不重新解析或升级其他依赖。修订后导出回归 **12/12**、beta 入口 **6/6**。首次冻结1872文件中误含3项旧面板生成资产，保留其检查记录；正式输入改为1869文件，摘要 `16a5749b24535543ab1cce9592ab9df28306c5f078eab53ea052561a652c148b`，App 尚未开始编译旧输入。准备脚本首次 Windows ESM 路径格式错误也保留，不涉及源码编译或运行重试。
- **App 实际构建**：五插件从当前冻结源码重新生成 dist，仅复用原插件构建工具依赖；App 自身在新目录以 pnpm11.19.0 完成 offline／frozen／ignore-scripts 安装，722包／0下载、明确跳过重新解析，输入锁文件逐字节保持，生成安装锁与源锁的 YAML 对象一致。没有将插件工具依赖复用称为六仓全部 fresh npm install。随后原模板／面板预构建、应用类型检查、production＋cloud-beta Web、官方 sync 通过；原 Gradle Wrapper／SDK 离线 **181 s／440项任务**完成 `assembleBeta`，未改 SDK、依赖版本或预算来凑通过。
- **最终 APK 核验**：73,775,343 B，SHA-256 `ead0d442f269748fbe6b42eb83b1675b6ea8f795d78117e2b5700b55f84332d4`；包名 `iot.diandeng.tech.beta`／“Blinker 内测”，APK v2 签名真实验证通过，既有内部证书 SHA-256 `a76639ec5806bd3b25f7dce0ebfdca775b0ae967b7ddc297ccebdbc8a5d5d210` 不变。最终合并 Manifest 明确 `usesCleartextTraffic=false`、无 networkSecurityConfig 覆盖；打包配置为 HTTPS origin、无 remote server、桥接日志关闭。19个唯一注册类全部在实际 DEX class_def 中定义，20个生成原生工程均位于本批隔离依赖图；五个 file 插件当前 dist／Android源码与安装副本精确一致。
- **资产与来源**：1150项 Web 构建资产逐字节进入 APK，sync 后1152项全部一致，额外仅两个官方空 Cordova占位。1869项冻结输入／工作副本／原仓源码三方摘要一致；快照来自当前工作树，不以 HEAD 冒称全部已提交。独立只读审阅分别关闭两目标与 APK 的产物核对，不代替手机安装、MCU 接入、真实控制或同事签收。
- **限定首错及警告**：两次 Windows批处理包装启动语法失败均在进入 Gradle前，原 intent／日志保留；最终直接由原 JDK 调用同一个原 Gradle Wrapper入口，只执行一次实际 Android构建，未重跑成功的插件／安装／Web／SDK阶段。APK读取器对 AAPT字面 `false` 和 pnpm生成锁的 key排序有两次格式假设失败，分别修正读取和使用真实YAML解析后只读核验，未改产物。Web初始包2.92 MB超过2 MB warning、未超5 MB error，原样式／SDK XML／flatDir／deprecated／strip警告保留，未放宽门禁；字体样式链接未内联的warning对应文件已确认在最终APK中，不冒称UI显示验收。

本批关闭 **D1b 构建与产物核验**；保留单份匹配 APK／单库 ZIP及必要 ELF、map、编译来源和完整日志，本批临时安装／源码／依赖工作目录在保全后清理，不动既有缓存、备份或其他工作树。未使用 ADB／串口／Docker／SSH、未安装／部署／刷板、未处理账号／授权／UNKNOWN或重派H2，未提交推送。下一步仅按D1c的新鲜备份和最小运行差异范围另行执行；不能因构建成功提前关闭D1c，亦不提前进入D2／D3／H2b。

### D1c 匹配 beta 与公开 profile 的有限运行差异（2026-10-20）

本批只验证必要新产物，不部署或重建云端服务，不发验证码／重新登录、不创建或刷新 Key、不重跑 H2／BLE／断电循环，不操作 COM9；无生产源码／SDK修改、Git暂存／提交／推送。既有云端设备和原账号复用，不用新的临时身份代替已有接入。

- **App 先保全再安装**：原 beta `8d6b001d…33ad71c4` 的 APK、新鲜完整 native 数据 **4,628,480 B**（SHA-256 `2d814332c8c941a9c68c9abe2f062d78768564f24bc68c39445fab46d4a12f8b`）、所有 `blinker*` IndexedDB／localStorage／sessionStorage及任务、自动化请求 journals 私有保留。正常停启后原 owner、登录及设备上下文保持，再且仅一次覆盖安装 D1b 的 `ead0d442…f84332d4`，设备端读回 SHA 一致，HTTPS origin／实际原生 SecureStorage 桥接正常；未清 App 数据或回灌旧 token。原 release／debug 包哈希不变。
- **原 SDK 唯一 XIAO 候选**：D1b 同一 ZIP 的 443 文件逐项安装核验，独立 Sketchbook／PubSubClient2.8，原 Core **3.3.11**／CLI1.5.1／`XIAO_ESP32S3:PartitionScheme=default_8MB`／jobs1，约420 s 完整链接及现有闭包检查通过。实际 Flash **1,035,936 B**／静态 RAM **51,976 B**，应用 BIN **1,036,080 B**，SHA-256 `45936b46be39c5bf15297799eccb756981419082e5693eeba85036f1f06620e7`；94编译项仅来自本批库及原依赖／SDK。用既有 Key 和现场 mostfun2026；普通无故障 Sketch 维持原 power／uptimeSeconds Manifest，0槽，不加入 GAP 命令／H2策略操作或额外宏。D1b通用ESP32占位BIN未用于真机。
- **COM11 新鲜恢复点**：核 USB／ROM MAC后重新读取全部 **8,388,608 B**，保持 ROM停机期间独立全量校验，SHA-256仍为 `7e3616e0d311672e4d13291dc8172180e55f1a6a5ec756e618dbab4ba127ed21`；相同摘要不替代本批新读。写前 NVS／现场factory表与本批备份一致，仅写 `0x10000` 应用并核验应用、原表和 NVS，不写编译生成的 OTA表。板端原身份及授权留在完整备份，不删除其他设备。
- **真实接入和唯一控制**：App正常 `/me`、设备目录、账户connection均200，维持单一账户 MQTT；设备使用公开profile的HTTPS DeviceKey和服务下发MQTTS接入。production beta 无 `ng` 调试对象，使用真实设备卡片／开关，旁路只观察 App 自身 WSS 的 MQTT PUBLISH并复用原 codec，不新建客户端或注入命令。唯一 Command sequence **32** 与同 requestId／acknowledgedSequence 的真实 ACK 对上：ACK **247 ms**，新鲜 Patch **234 ms**，power false→true、revision **1→2**，UI同步变更。State先于ACK到达合法，二者分别相关联；不把缓存 Manifest／基线、publish成功或UI乐观值当作控制结果，没有重发。
- **关闭功能的限定证据**：观察器先开启，再正常重载WebView，覆盖启动／后台／返回 **39,464 ms**；另设备进入 **16,746 ms**、一次控制 **614 ms**、离页／后台／返回 **30,456 ms**。按已冻结的关闭能力合同，四个窗口关闭功能HTTP调用均 **0**，无观察上限溢出；原生Ntfy最终持久开关 `enabled=false`、无Ntfy前台服务。此为有限 renderer HTTP窗口加原生停机状态，**不是安装首次启动前置窗口、全部原生网络或长期零调用证明**。请求记录保存URL路径／状态／关联ID／单调时间，不保存MQTT CONNECT凭据；已有持久请求journals全量保全，不把本批trace称为全部历史HTTP流量。
- **真实独立缺口不掩盖**：启动时普通 `/ui-config` 的 OPTIONS404／GET失败；设备页原 `/console` 读取有6组 OPTIONS404／GET失败，页面显示“控制台权限或配置暂不可用”。正常通用控制回退及上述 ACK／State仍可用，但页面配置链尚不健康。`consoleLinks=false`关闭链接／公共访问接口，不等于禁止普通控制页布局读取；最初观察器误将普通 `/console` 归为关闭功能，产生12条分类记录。原始trace及失败逐条不改，依据真实配置合同修正观察工具并保留单独复核结果；观察解码／分类有限回归 **13/13**。这既不是放开生产功能或隐藏失败，也不将404冒称权限不足／已通过。下一批先D1d，不在本批无备份地部署服务或改变ACL。
- **收尾已完成**：离页回首页后保全账号、全部持久请求journals、原生数据及日志；最终 native **4,687,872 B**（SHA-256 `db85a6c66cfc82e4ee25fe3c5fff96bfa26725c23387e14110ff12dc7a5f6d12`），正常重启后原 owner／登录、原任务与自动化journals逐行一致，原两包保持，匹配 beta保留，本批9342 forward已回收。COM11 将**本批新鲜全8MiB**精确写回，再独立全范围ROM校验及hard reset通过，原固件与授权恢复；不宣称原业务重新云上线已复验。云端测试设备暂离线符合恢复旧身份的预期，未删除设备或修改停用H2策略／已有历史；授权、UNKNOWN和云端原完整日志保留。

首错保留：编译前工具沿用旧宏名检查、实际新renderer已用typed常量，修正只读断言后才首次启动唯一编译；首次页面验收选择器误限定英文按钮，未识别实际本地化卡片开关而停止、未发命令，之后仅从原trace／当前DOM只读协调基线，再执行上述唯一正常开关。未靠重放或重跑功能绕过失败。

证据仅存忽略的 `build/d1c-cloud-20261020`（App完整数据、Flash／Key／网络配置私有保护）及被动观察工具目录，不整体交付或提交。**D1c当时的接入／控制核心与保全通过，页面配置缺口仍保留，未宣称整体健康或同事签收。该缺口随后已由下节D1d关闭，不再作为当前待办；原始失败证据保持。**

### D1d 精确公开页面配置与 D1 收口（2026-10-20）

本节解决D1c发现的404，上一节为当时真实证据，不删除或改写成从未失败。**D1a～D1d按普通ESP32 WiFi／0槽／当前beta的受限云端范围收口，下一步D2。** 不扩大到完整生产发行、其他产品组合、D3或同事签收。

- **根因已由实际运行确认**：Gateway仍为 `sha256:33d87a0b…a03`，原四容器healthy；运行 `dist/index.js`／两handler真实存在，统一OPTIONS204与原鉴权接线保持。公开OPTIONS却404且无Gateway CORS头，对应Nginx精确资源枚举漏掉普通`console`和`ui-config`。这不是Key／owner权限、设备离线或任务准入问题；不改App读配置、错误提示或4秒生命周期轮询绕过缺陷。
- **最小源码与部署**：只改`deploy/beta/nginx-api.conf.template`的既有设备资源精确枚举及拒绝注释，加入上述两个完整路径，保留ID字符界限／尾部锚定；`console-link`、`console-public-access`、查看会话、UI agent、任务／自动化／内部等保持拒绝。同步路由回归及部署说明；原Node22有限Nginx **3/3**通过，含新增子路径／尾斜线／相似后缀拒绝。App／设备／运行服务源码和SDK不变，不创建新协议、迁移或执行链。
- **先新鲜保全再加载**：远端私有`/opt/blinker-iot-beta/backups/d1d-page-config-20261020`保留原Nginx配置及完整`nginx -T`、PG native dump与archive list、Broker SQLite native备份与quick_check、完整Gateway／Broker日志及intent／哈希。它们是本机各库原生备份，不冒称协调整栈或离机恢复演练。只原子替换API配置，SHA `9a59c1ab…7f97`→`e3fcec7b…810a`，实际`nginx -t`和graceful reload通过；配置失败路径保留原配置恢复。未重建或重启任何Docker容器；终核全容器ID／镜像／StartedAt／env摘要保持、四个IoT容器healthy，完整`nginx -T`除该API文件外逐字节不变，证书／stream／proxy片段与其他业务配置不改。
- **公网边界有限核对**：共 **13项**，两接口预检204及原Gateway CORS头正常，无token GET均401；Console链接／公共访问、额外子路径、Direct、任务、自动化、内部和UI agent测试路径仍404。不是Nginx单独伪造204或扩大匿名权限；后台原ACL／CAS保持，没有用SQL补授权。
- **真实App只读短验**：先新鲜保存原APK、完整native **4,688,896 B**（SHA `4304e544aa492ed32859334debcd6ff975aa22453d7759c5e1fa74286b3610a2`）、原账号及全部持久任务／自动化journals，再复用同一个D1b beta `ead0d442…f84332d4`，不重建／重装。被动观察在正常重载前开启，原账号自然加载`/ui-config`200；正常卡片进入同一既有云设备，`/console`共6次GET均200，至少两次原4秒节拍读取成功，响应身份／revision／canRead结构与App正常解析匹配，原可见配置错误消失。当前云设备离线符合COM11已恢复旧身份的事实；配置读取不依赖设备在线，不冒称本批新鲜State或硬件控制。
- **回收与保全**：设备页退后台后及正常离页回首页后，各取1秒收敛加 **10秒**观测窗，均无新增Console读取；返回前台后自然恢复读取。关闭能力HTTP调用0，未发Command、写配置／设备／Key／策略或重放UNKNOWN；只允许自然会话refresh。最终完整native **4,689,408 B**（SHA `b954f483d4563905bd2b4e1f1ffc9fb24481f34dd6344d60285db0bd6775dd3d`）、实际日志保全，重启后原owner／登录及全部journals逐行保持，原release／debug包不变。匹配beta保留、首页及本批forward回收；两块板均未打开串口／刷写，不重复控制、H2、BLE或断电。服务端追加完整新日志，SSH会话已退出。

保留工具首错：首次只读探针误查容器`src`而非实际`dist`路径，修正后只读核实；运维脚本字典键笔误被Python解析拒绝，尚未执行任何备份或写入，修正并本地语法核查后才首次建立新鲜备份／唯一apply意图。首错与原始trace保留，未重派业务请求。

证据位于忽略的Service `build/d1d-cloud-20261020`与Library `build/d1d-app-20261020`及上述服务端私有备份，含账号／数据材料，不整体交付或提交。独立源码／操作边界审阅无阻断；不将有限只读验收称为页面编辑或所有共享ACL全矩阵通过。**D1本批无剩余必需阻点；D2尚未实施，D3／H2b、旧本机策略／通知UNKNOWN／发码现象／Error8等既定边界不变。** 未提交推送，无关本地修改保持。

### D2 固定历史窗口：本批最小范围

- 仅改App查询层：原1小时／24小时／7天改为“窗口宽度”，新增明确以UTC输入的结束日期时间、应用、前一窗／后一窗及回到最近窗口。结束时刻秒精度，范围始终为`[from,to)`；相邻窗不重叠，不把7天选择项称为完整曲线。
- 已应用范围与未应用结束时间草稿分离；刷新、字段筛选、分页和后台返回保持同一范围，仅首次进入或显式“回到最近窗口”以当前时间重锚。宽度变更保留结束时间，翻窗保持宽度；非法／空日期、未来结束和早于epoch的窗不发请求、不静默校正。UTC避免本地时区／夏令时歧义。
- 换窗清空旧行和cursor，复用原50条分页、200条总限、固定范围cursor与权限／account／generation／abort栅栏；未知保存仍须完整只读确认后才能解除禁写。沿用原曲线断段、名称、精确大整数和retention说明，不拉全量、不增加缓存／轮询或改服务、采样、协议、SDK。
- 本批先有限纯逻辑、页面／真实模板及类型检查；通过后再单独安排匹配beta构建和仅只读历史窗口短验。不部署、刷板、启用策略、重跑H2采样或提交推送。

60秒含义说明：这是现行Broker H2a持久归档策略，不是设备全局发送周期。设备按租约实际interval和所选字段`realtime(ms)`最短周期执行`onSample()`，同次选中的多字段放同一个TelemetryData包；Broker合并页面／归档的Cloud需求，页面更快时共享上游流，再按设备monotonic间隔至少60秒选一次归档。一次多字段样本以同一事务保存多条字段记录，页面200条按字段记录计，不是200个完整多字段采样点。不积攒60个样本合包，普通State／控制ACK／`report()`不受此60秒固定值限制；不兼容的慢字段归档不能拖慢既有实时需求。

### D2a 软件结果与下一批（2026-10-20）

- 新增一个小型纯函数模块`history-window.ts`，复用原30天查询宽度上限，集中UTC秒精度／严格日历round-trip、未来／epoch／窗口宽度和相邻翻窗计算；不将日期逻辑塞进HTTP服务或扩框架。结束输入是明确标注UTC的`datetime-local`，不使用宿主默认时区解析。
- 趋势页保留三个窗口宽度，增加结束草稿／应用、前一窗／后一窗／回到最近窗口。普通刷新、字段筛选、分页与前台恢复保持已应用`from/to`；后台仍清敏感样本／策略并中止请求，但保留同账号筛选。新进入或显式回到最近窗口才取当前时间，删去旧refresh每次重锚和“最近7天”滑动标签。改变宽度／翻窗以已应用窗口为准，并舍弃未应用结束草稿；输入草稿没有被冒充为结果范围。
- 复用原完整detail＋page只读恢复及generation/account/abort栅栏；失败不移动所选范围、不解除UNKNOWN／CAS禁写，只有完整读成功才重新核对策略。换窗清旧rows／cursor，每页50／最多200字段记录不变；文案明确曲线未必覆盖全窗、retention仍生效，固定范围不等于冻结数据库快照。原曲线断段、名称／精确大整数、read-only share及权限拒绝清理保持。
- 原Node22.22.3下5文件 **75/75**通过：15窗口函数、28页面状态／生命周期、5生产模板、22原DTO／曲线、5原HTTP合同。覆盖UTC／非法日历／未来／epoch、相邻半开边界、固定范围刷新／筛选／分页／后台恢复、无效草稿零请求、网络失败同窗恢复、晚回包隔离、未知保存仍禁写、200条不继续发网。最后补清楚“60秒是服务器选样而非全局上传”的模板文案和未来输入断言后，仅定向终核模板 **5/5**，是75项的受影响子集，不累计成80项；App与测试TypeScript `--noEmit`均通过，独立窄审无阻断。
- 没有设备／服务源码、采样节拍、策略值、API／数据库／SDK／依赖变更；未访问真实API、账号、设备或部署运行版本。普通report约250ms合并及多字段采样说明补入[H2 §9](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md#9-采样上传与历史保存周期)，App来源说明同步避免误解，不将其当新的发送优化。原授权／UNKNOWN／完整请求日志与无关修改保持，未暂存、提交或推送。

**D2a当时仅完成软件，未整体关闭。** 当时安排的D2b匹配构建、账号／日志新鲜保全及既有历史只读短验，现以下节实际结果为准；不将当时未安装的记录反填为已安装，也不自动开启D3／H2b。

### D2b 匹配 beta 与既有历史的只读短验（2026-10-20）

- **唯一构建**：当前App与五插件冻结1871项源码，输入manifest SHA-256 `5bd42bcef7faa84c55095512dba9d3338266756d6eb99330f7a87dff753cd276`。沿原Node22.22.3／pnpm11.19.0／JDK21／SDK36，离线frozen安装、原模板／面板、类型检查、production＋cloud-beta和官方sync通过，唯一Gradle8.14.3队列 **178秒／440任务**完成。APK **73,779,335 B**，SHA-256 `51637371563c1e3ffb37eef48a14adcc716465e15cba1189a22cdf85eaac1df7`；`iot.diandeng.tech.beta`、原signer、HTTPS／禁明文／无信任覆盖、19插件类／18DEX、20原生工程、1150项Web资产字节一致均核对。保留原2.92MB初始包、样式及SDK／原生警告，不升级依赖、放宽预算或另开重复队列。
- **先保全后覆盖安装**：原D1 beta APK、新鲜完整native **4,689,408 B**（SHA `1fd38f2308b88980c8277b95276846fd75d943bf861ef088f7a89e45f093468b`）、原账号和全部`blinker*`持久存储／任务及自动化journals保存在私有目录。一次`install -r`后读回新APK摘要，实际SecureStorage、owner、journals及原release／debug包核对通过；没有清数据、重新登录或回灌旧token。
- **正常UI与真实旧记录**：从原设备卡片进入设备设置的“在线趋势”，沿现有服务读取原3条归档和revision2／disabled策略；不造新记录或启用归档。指定UTC秒精度结束点、24小时半开窗口，3条真实记录保持；只读刷新、字段筛选保持完全相同`from/to`。前一窗空记录／空曲线、后一窗恢复原记录，边界精确相邻；未来结束草稿明确提示、应用禁用且历史GET增量0，原结果范围不被错误改名。6个完整历史GET及对应detail读取均200，响应no-store／范围／原记录ID与真实DOM相符，不以模拟点代替数据。原分页／200条软件门禁复用D2a，不为本批新增197条记录或声称覆盖全部保留期。
- **回收与返回**：原生HOME后1秒收敛＋**10秒**窗口历史GET无新增；正常返回后页面保持同一个UTC范围和字段，3条原数据重新展示。后台WebView不响应Runtime.evaluate，之后连调试发现端点也超时，故该段**未取得后台DOM证据，恢复首个请求亦有renderer trace间隙**。正常恢复后仅只读协调：实际ResourceTiming比前窗多1个已完成的同范围／同字段history请求，真实DOM与已验范围／记录数相符，没有重跑前六项或重复采样。随后正常离页再取1秒收敛＋10秒无history读取，旧页DOM样本／曲线已清空。后台清敏感行的实现保证取D2a软件回归，不把返回数据或ResourceTiming冒称缺失的逐请求响应体／全程网络证明。
- **限定首错不掩盖**：首次工具误选择独立V2内容的设置按钮，但普通页面由外层wrapper持有实际设置入口，未开始历史测试即停止；只读核对正常可见router link后继续。保留原trace及之后后台调试超时，不改产品逻辑来迁就观察工具、不注入独立API客户端。完整观测窗无业务写HTTP／Command或关闭能力调用，返回间隙只按上述证据表述，不泛称全部原生网络均零调用。
- **最终保全**：匹配新beta保留、回首页，全部原owner／登录及持久journals逐行保持，原两包不变。最终完整native **4,727,808 B**（SHA `ca1683861b4565a69e1e5f00eb04a8bd3f3154f5e6c78c19196ad92cd9f927ce`）、实际App日志与完整请求证据私有保留，正常重启再核通过；专属9342 forward已回收。云端原服务、授权、UNKNOWN和既有日志不改，未操作串口／刷板／重控／重跑H2或BLE。

本批附带解决用户对离页后仍1秒上传的疑问：现有App30秒／约10秒续租、Broker按需求重算／5秒清扫、设备30秒本地到期已具备三层保护；只新增“两个消费者无Close先后到期→仅归档60秒→停归档且迟到包不能复活”的确定性组合回归，原归档测试一次 **32/32**通过，生产租约链不变，详见[H2 §9.1](DEVICE_V2_H2_ONLINE_HISTORY_EXECUTION_PLAN.md#91-退出实时页后的保护合同)。这不是本批新增真机故障循环；不能统一宣称从退出起30秒硬停止。

**D2按固定窗口最小产品与上述有限只读范围收口。** D1原范围保持，下一批先明确D3签发／服务准入／设备产品／beta开关的最小匹配范围，再决定有限实施；不直接开放任务、自动化、Direct或新网关。H2b、离线存储／Bulk、旧本机策略协调、通知UNKNOWN／发码现象和Error8仍独立后置。构建候选、实际安装与运行记录分别保存，不覆写构建时`installed=false`的证明；临时工作目录仅在保留产物与审计后清理，账号／Flash备份不删。不提交推送、保留无关本地修改。

证据仅在忽略的`build/d2b-cloud-20261020`及`build/d2b-app-20261020`（后者私有ACL保护），不将账号、完整native备份、日志或构建工作目录当交付源码。独立只读终核未发现阻断，已复核实际APK、逐窗记录及前后native／owner／journals；两个journal当前为空，不冒称非空请求恢复。清理后唯一APK和证据摘要再验保持，仅删除本批临时源码／依赖／构建目录`local-maintenance/d2b-cloud-20261020`，约释放487MB，可由原源码及依赖重建；旧批次、共享缓存、账号与硬件备份均保留。

### D3 最小范围与顺序（2026-10-20）

**自动化和设备持久任务分开准入，先自动化，不同时打开所有beta能力。** 从职责出发，自动化规则在Gateway执行，设备只复用普通State上报／Property控制；持久任务由设备TaskEngine独立保存、调度，另需设备槽位、匹配Grant签发及资源支持。两者没有必须一起开放的依赖。

| 阶段 | 最小工作 | 收口条件 |
| --- | --- | --- |
| D3a（本批已完成） | 默认关闭的自动化部署候选及有限回归 | 基础Compose／已安装beta不变；显式overlay配对Gateway／Broker五用途HMAC，Nginx仅显式选择时开放精确自动化路由。App只补automation-only入口／guard矩阵，运行链和生产开关不改；不部署、刷板或造账号／规则 |
| D3b（已完成） | 实际运行匹配、备份及有限部署 | 原镜像／56迁移保持，双侧私有用途配置已启用，四容器healthy；精确公网401／204、匹配beta安装及真实AppGET200／只读草稿短验完成。不是重新实现自动化，也不等待两板才能关闭部署接线 |
| D3c（已完成） | 仅一次新云环境的两设备触发差异 | COM11 ESP32-S3与COM9 R4普通0槽TLS内部候选，同owner／tenant／normal同shard。真实App新建停用规则、启用后确认false基线，App退后台，一次新State变化取得唯一execution／Command、真实ACK和目标fresh State；正常停用／删除及两板恢复完成。Broker尝试2次、目标回调1次，不称一次投递；其他矩阵仍取原M5、N5及§11.41 |
| 独立后续 | ESP32云端持久任务 | 先核真实P256算法／keyId／公钥、Gateway／Broker逐设备名单，再出有限四槽TLS候选，原SDK定向链接和真实短验另记。不放宽D1公开0槽profile，不借MQTT JWT公钥或R4 Plain结果代签 |

本次源码核对结论与冻结约束：

- 当前自动化缺口是**部署接线**：基础beta两进程均`DEVICE_V2_AUTOMATION_ENABLED=false`、未注入配对用途key、Nginx拒绝公开自动化、App入口关闭。已有[配置解析器](../../aily-blinker-services/packages/contracts/src/automation-config.ts)严格验证五个不同的32字节hex key；复用它而不新增第二套签发／校验框架。采用额外显式Compose overlay，不改基础闭合配置；key只在运维私有env生成、注入，不进入源码、镜像或公开JSON。
- 自动化的五用途HMAC是Gateway／Broker内部消息保护，不是设备P256／Ed25519 Task Grant或MQTT JWT，也不要求开启Direct／BLE／LAN。公网只评审`/api/v2/automations`、`/:id`及`/:id/retire`；方法、owner、CAS、取消／fence、UNKNOWN、限额和限流均沿原handler。`/internal/v2/automation/*`、任务、通知、Console公开链接等继续拒绝。
- 触发源是已接受的**StateSnapshot／StatePatch**，不是H2 TelemetryData或归档记录。源／目标仍须原规则支持的同owner普通DIY能力、单一当前物理连接及Manifest／State新鲜度；不因“服务器能收到包”跳过资格，不把60秒历史节拍变成自动化节拍。原worker合并唤醒和1秒兜底不改。
- D1公开`esp32-cloud-wifi-v1`为production／0槽／空业务签名环，仍保持该限制；普通0槽设备可以参与云自动化。ESP32既有任务carrier可组合原TLS客户端，**代码可组合不代表任务＋TLS已经验收**，D1两目标0槽链接与本机四槽候选不能互代。R4四槽仅固定Plain＋Ed25519有限图，Wio暂缓，本批不扩大其支持、不重开资源审计。
- 云端上次确认仅一台既有离线测试设备，不具备两设备闭环前提；本批不将软件候选写成上线或真机通过。D3b再新鲜核对运行环境，D3c再核真实设备数量／身份／备份，不能复制本机私有身份或同一物理板冒充两个在线参与者。

本批有限软件实施仅在现有部署工具／测试中完成，结果见下节；SDK、协议、设备源码、运行服务及已安装App均保持，不扩槽、不升级、不重放、不提交推送。原授权、UNKNOWN、完整请求日志和无关本地修改继续保留。

### D3a 软件候选结果（2026-10-20）

- **Service只改部署适配**：新增`compose.automation.yaml`及五空项`.env.automation.example`，只通过同一组必填私有用途key配对两服务。合并后仅两进程的enabled和对应用途JSON四项环境值改变，其余镜像、端口、数据、挂载、Auth、任务名单及功能开关与原配置完全一致。基础`compose.yaml`与`.env.example`本批前后SHA分别保持`3f396c431c5d5953f28b630e11b957e6bfeeeac17f3765bfed89eba64a18be77`／`e3573c8c1fc0c6727910a158053e67a0a868676bb766bd3d02db2254c973db33`；这是本批审计证据，不把历史文件SHA硬绑为长期产品测试，避免正常CRLF检出和后续合法维护被误拒。
- **精确可选路由**：原Nginx renderer增加严格可选布尔`automationEnabled`，省略／false保持原三个输出，true才插入集合／单规则／退休的首尾锚定路径。拒字符串／数字假布尔、相似后缀、尾斜线、额外子路径及内部／任务／Direct等未准入路径；原handler继续鉴权、方法、CAS和限流。审阅发现Windows CRLF模板会让原新增插入点失配，已最小修成保留原换行的行匹配并补内存变体；缺标记仍显式失败，不静默输出缺路由候选。
- **复用真正的配置消费者**：Compose只核必填插值，不冒充密钥校验器；测试将实际`docker compose config --format json`结果交给原contracts解析，核五用途配对、shard0／origin，重复、短值、大写、非hex均拒绝。原锁TypeScript5.9.3按实际contracts配置转译`automation-config.ts`与现有dist逐字节一致，未用旧消费者测试新合同，也未写dist或新增鉴权框架。
- **App仅三处测试**：原environment、个人页菜单及deployment guard各增加automation-only组合用例；证明只有唯一自动化入口／路由可入，tasks、Direct、消息等仍拒绝，原生产／beta能力对象保持全false且冻结。不新增环境分支／执行器，不改实际开关、HTTP管理、保存确认、CAS或持久UNKNOWN链。
- **有限验证**：原Node22.22.3首轮Service配置／Nginx／原装配runtime队列 **25/25**；新增换行回归后仅Nginx受影响 **6/6**，去除历史SHA硬断言后仅配置受影响 **4/4**，后两者为子集终核、不叠加为35项。App单条定向队列6文件 **64/64**（environment5、菜单3、guard4、原HTTP5、client29、确认18）和spec TypeScript检查通过；无跳过，不运行全量数据库／MCU或重复部署测试。原有限runtime替身／配置解析不等于真实数据库、物理ACK或两设备签收。
- **审阅与保全**：独立窄审未发现职责、签名材料、槽位或准入混用；保留原HMAC／P256／Ed25519分工。无业务源码、协议、迁移、SDK或依赖升级，不构建镜像／APK，不SSH部署、访问账号API或操作串口／ADB；运行服务、已安装beta、账号、授权、UNKNOWN和完整请求记录均未操作。无关本地修改保持，未暂存、提交或推送。

**D3a当时仅软件收口，未部署；实际启用结果以下节D3b为准。** 本节测试不能反填为当时已上线，也不能代替剩余一次新云触发。H2b／离线历史／Bulk及既定待定位项不扩散。

### D3b 实际云端启用与匹配 App（2026-10-20，已收口）

**纠偏**：本机[M1～M5](DEVICE_V2_NEXT_EXECUTION_PLAN.md#886-第八十六批m5本机新版app与两设备产品验收2026-10-10)、[N5数值动作](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#96-n5-本机数值短验收2026-10-11收口)和原合并唤醒实物均已完成。最近D3a处理的是新云环境此前关闭的配置，不是自动化核心缺失；阶段过细容易误读为重做。本批合并备份、启用、App安装和只读短验一次收口，后续不再派发新的纯校验阶段。

- **不重编服务器**：实际Gateway `33d87a0b…a03`、Broker `4980ebde…6339`已有完整自动化dist及原合并唤醒，56项迁移已齐。新鲜核对后保留原镜像，仅同镜像重建两容器、启用配对五用途HMAC；PostgreSQL／Redis未重启，镜像、其他配置和业务数据保持，未重造执行器或改轮询。
- **先保全再启用**：远端私有`/opt/blinker-iot-beta/backups/d3b-automation-20261020`保留PG原生归档／目录、SQLite native backup／quick_check、配置／授权材料、镜像清单和完整G/B／Nginx日志。五个独立CSPRNG用途key只存私有env；基础env和模板默认仍关闭。备份不是协调整栈／异机恢复演练；原UNKNOWN不重放、不删库或回灌。
- **实际部署与边界**：真实双侧配置解析启用并配对、WARN且诊断关闭；四容器healthy。Nginx仅增加精确自动化块，经`nginx -t`及graceful reload；三类未登录请求401／预检204、14项未开放路径404，原Gateway CORS保持。任务名单／Direct等继续关闭，公网边界不绕过原鉴权。全局Nginx仅本批自动化差异，不改其他站点。
- **匹配beta已安装**：仅现有`environment.cloud-beta.ts`覆盖`automation=true`，普通prod/dev及其他能力保持关闭，不增环境／执行链。入口7项、环境／菜单／guard12项及App／spec类型通过。1871项源码冻结SHA `d97a782159b2c43bad71ac384a68fb4f137da5c8e4631b12cbc2dc2716356788`，原Node22／pnpm11.19／JDK21／SDK36唯一离线队列、440任务／174秒完成。APK **73,776,855 B**、SHA `1b43b7a489135f16f838d9961857558969410d7bba0ba1e9d5eb40f5dea2c265`，原signer、HTTPS、19注册类／20原生工程／18DEX及1152资产字节核对通过；原非阻断警告保持。
- **真实App只读差异**：先保全原APK、账号及完整native **4,727,808 B**（SHA `ea5aa774045d709a3c0903518e50d267b89f803321af6c387ea18e98f0bf0d4e`），一次覆盖安装后读回新APK一致。正常个人页唯一自动化入口→真实列表GET **200／no-store**，仅1次GET、`rules=[]／nextAfter=null`。进入本地新建草稿、使用既有缓存源后，自然提示需要另一台所属普通WiFi设备，保存禁用；正常确认放弃草稿、离页5秒无新自动化请求，回首页。没有规则PUT／POST、Command或其他关闭功能请求；不是伪造规则／HTTP夹具，也不是新云触发验收。
- **保全与最终状态**：账号前后逐字保持，终核读取实际`blinker-automations`的`intents`（新建空store，0请求），全量原存储／授权和日志保全，不仅比较旧helper的空别名。最终native **4,761,600 B**、SHA `4fec807d3f86d86ce2aed7dd3770a958f11744a081c2a684d4e9b67f5a9f9183`，正常停启再核、原release／debug包保持，9342 forward回收。云端最终规则／启用规则／execution／UNKNOWN均0、容器／配置未变、完整追加日志保全，SSH已退出。自动化保持开启、新beta保留，不刷板／重跑H2／BLE或本机M5。

后续运维须显式保留本次opt-in：两份私有env为`/opt/blinker-iot-beta/private/beta.env`、`automation-20261020.env`，Compose为原`releases/49c029b5a760/deploy/beta/compose.yaml`加`operators/d3b-automation-20261020/compose.automation.yaml`。只执行基础Compose会重新关闭自动化；这些路径是运行接线，不把私有文件内容加入源码或Git。

工具首停保留：候选配置的只读子进程曾因输出获取为空而停止，dotenv提示单独记录后只续验证；当时尚未重建容器，没有重复生成key、重做备份或派发业务请求。独立最终窄核无阻断；不把启动装配证据称为已有规则执行。证据分别位于忽略的Service `build/d3b-cloud-20261020`（公开ready／closeout摘要及私有原件）和Library `build/d3b-app-20261020`／`build/d3b-cloud-20261020`，不交付账号或构建目录。唯一APK／manifest／锁／完整日志保留后，仅删除本批可重建临时构建工作目录，释放约487MB；旧批次、账号与硬件备份、共享缓存均未动。

**D3b部署接线关闭，剩余仅D3c一次新云触发。** 当前真实App只有一台既有云设备，本轮只读USB识别为COM11 ESP32-S3＋COM9 UNO R4，未操作两板；不能把尚未匹配当前云profile的另一MCU或同一板的两个身份当两台已准入参与者。待两台支持当前HTTPS／MQTTS profile的设备同时云在线，复用普通0槽链做上述一次差异；不重新做M5／N5、不引入任务签名／四槽候选，不新开资源审计或长循环。无关本地修改、授权、UNKNOWN及日志保留，未暂存、提交或推送。

### D3c 前序材料复用与重复副本清理（2026-10-20）

按用户要求，不再为相同内容反复保留备份。本批逐文件重新核对大小、SHA-256、限定工作区路径及仍存在的canonical后，仅删除 **114个完全重复副本／4,848,022,683 B（约4.85 GB）**，涉及旧APK、整Flash及完整native归档；未删除父目录。结果见忽略的`build/cleanup-duplicates-20261020/result.json`，67个canonical原路径及权限保持，可用于恢复相同副本内容；所有唯一授权、任务／自动化UNKNOWN、完整日志、历史数据和本轮D3b服务配置恢复材料保留，不按日期批量删除。

COM11本轮新读8 MiB与既有D1c canonical全量逐字节及独立ROM核验一致，差异扇区0，复用既有恢复材料；在再次核对canonical后删除本轮新读的 **8,388,608 B重复副本**，只保留`build/d3c-board-20261020/canonical-reuse-verified.json`等小型核验记录。此前各节“备份保留”是当时状态，后续重复内容以本节清理记录为准；唯一不同材料不删，也不为清理另造备份。本节只记录材料管理，**当时尚未完成D3c，最终验收以下节为准，不代表R4公开支持范围开放**；未提交推送。

前序停点（后续已完成，见下节）：COM11复用原云身份及Key，原ESP32 Core3.3.11／单队列完成普通0槽TLS候选链接，仅在factory应用区写入并verify；NVS与分区前后保持canonical一致。串口单次`j`确认`power=0`，当时未触发或启用规则。R4代码核对存在既有普通0槽`WiFiSSLClient`的HTTPS／MQTTS链路，不据此扩大公开ESP32 profile或R4四槽TLS支持。真实App一次创建设备成功（201），Key获取正常返回近期登录要求；当时等待重新登录，R4尚未编译／刷写，没有重复创建或刷新Key。

云端只读基线再次确认原四容器健康、镜像及五用途配置匹配、WARN且诊断关闭；规则／execution／UNKNOWN仍为0，未改服务或数据库、未新增服务器备份。忽略的只读观测工具曾在结果前缀截取及Broker配置导出适配处首停，修正为实际前缀长度／既有导出形态后只续同一基线；不把工具错误计为业务请求或设备故障。证据保存在`build/d3c-app-20261020`、`build/d3c-board-20261020`及Service `build/d3c-cloud-observer-20261020`的私有目录；SSH、串口及本批ADB转发已释放。后续只续R4 Key、匹配候选与一次真实触发，不重复D3b／H2／M5、整包备份或已通过编译。

登录首停保留：旧码仅经正常App表单提交一次，真实登录POST返回400，未建立会话。发码到提交约600秒，超过当前Auth源码300秒有效期；时间证据支持超时判断，不冒称已取得错误正文。未重试旧码或绕过Auth；后续按用户明确要求重新发码，结果见下节。

### D3c 两设备云端自动化与恢复收口（2026-10-20）

**本批完成一次新云环境差异，不重新实现或重验M5／N5。** 复用D3b已安装beta和运行服务，无App／服务重建、配置更改或新整包备份。

- **真实登录和既有设备**：用户明确重新发码后，正常App一次发码／登录成功，原owner及token pair就绪；在已创建的R4设备设置正常查看Key成功。未再次创建设备、未刷新Key、不读取验证码库／数据库Key。App最终原账号、两台设备记录和原APK保持，Key隐藏，回首页；不保存验证码或在文档记录网络密钥。
- **匹配候选**：COM11原Core3.3.11／单队列，普通0槽TLS来源BIN **1,036,304 B**，SHA `70ba3c2e…33b8e1e`，Manifest仍为原power／uptimeSeconds，仅复用既有串口report方式产生真实State。COM9原Core1.5.3唯一链接，普通0槽TLS目标BIN **145,064 B**，SHA `60be7a11…e7be0d8`，复用现有`WiFiSSLClient`／DeviceKey／MQTT，无Task／BLE／Gateway图。一次标准上传后真实networkReady／cloudReady、Active／error0，未重开上传器或资源审计；这是内部运行差异，不扩大公开ESP32 profile或R4四槽TLS支持。
- **真实App与后台触发**：两板正常控制页取得Manifest及false State；正常表单新建停用规则（revision1），启用确认（revision2），只读PG核同watch／session的`rawValue=false`、`latched=false`基线。App原生HOME后确认launcher在前台，再仅发一次来源串口`1`，无复位、无`0`回写或第二次触发。候选明确为`kind=change`、false→true／State revision1→2，不用首次Snapshot或重连冒充变化。
- **唯一执行与设备结果**：规则`84ac686d…f485`仅有execution `9725b09b…9b30`，一次occurrence和一个Command ledger行；PG与Broker均`acknowledged`，sequence5／idempotency／digest对应一致、UNKNOWN为0。R4同session／fingerprint的新鲜State false／revision1→true／revision2；真实串口回调计数0→1。**Broker发送尝试为2次，不是一次网络投递；设备应用回调只执行一次。** 源接受→目标State **648 ms**、→最终PG ACK **694 ms**仅为这一次有限样本，不称claim／publish精分段、长期性能或实物动作完成。
- **App查看与规则收尾**：回前台通过正常详情GET200核唯一最近执行，页面实际记录显示“设备已确认”；正常停用（revision3）并确认删除（revision4，retire200）。活动列表移除、原`blinker-automations/intents`只读确认0，历史不删、不造请求或重放UNKNOWN。最终云端规则历史1条／启用0／execution1／UNKNOWN0，原四容器、镜像、启动时间、env及用途配置保持，WARN且诊断关闭。
- **精确恢复及边界**：COM11复用唯一canonical写回完整8 MiB，独立全片ROM验证SHA `7e3616e0…127ed21`通过，原固件／NVS／文件区／身份及授权精确恢复并恢复运行。COM9既有维护BIN核十二键双读与canonical逐字相同，故存储回写0次、namespace未清；指定已验普通WiFi BIN标准上传成功，`j`匹配原程序、Timer未启动。R4证据为指定BIN上传＋运行及十二键核对，不冒称新鲜主程序备份／全Flash读回或原网络云在线。两板已恢复旧程序，新云设备当前离线符合测试收尾，账号记录与Key保留，不再次创建。
- **材料和连接收口**：原唯一恢复材料、授权、完整请求／串口／有限云日志及UNKNOWN保留；未新增App／主程序／Store整包备份。删除本批两个编译目录的六个可重建core／libraries／sketch中间目录，约 **92.35 MB**，候选BIN／ELF／map逐一hash不变；可由冻结源码和原SDK重建，恢复材料及日志未动。SSH、串口、ADB forward已释放，未提交推送，保留无关本地修改。

忽略目录中的`Service build/d3c-cloud-observer-20261020/d3c-once-independent-proof.json`为独立窄审，`closeout.json`联合核实际App、退役、服务前后及两板恢复；Library `build/d3c-app-20261020`、`d3c-board-20261020`、`d3c-r4-20261020`保留私有原始关联记录，不作为源码交付。薄适配的字段ID选择和Android前台格式首停均发生在业务提交／来源触发前，只续原草稿／只读确认，没有创建第二规则或额外触发。

**D3a～D3c关闭。** 后续设备持久任务单独明确ESP32云端四槽TLS的真实签发环／keyId／公钥、逐设备名单和匹配图，不放宽现有公开0槽profile、不自动开放R4四槽TLS；本批不启动该阶段。H2b、离线历史／Bulk、旧本机策略协调、通知／发码独立项、Wio及Error8继续按原边界后置，不再重复云端最小自动化或BLE／断电长循环。

## D4 ESP32云端四槽任务最小准入与部署差异（2026-10-20）

本节前半为前批未启用候选及实施前基线，**实际部署和验收后的最终状态以本节末尾收口记录为准**，不再将前批空名单／未构建写成当前待办。

### 主线纠偏与实际运行基线

**本次不是再做定时任务。** [任务计划§10.8](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#108-esp32-四槽持久任务及恢复验收)已有ESP32四槽保存、Start、真实到期／重启恢复及原控制证据，§11.30～§11.33又完成确认／刷新解耦、目录缓存、进度新鲜读取及CAS优化。旧本机签发／实物验收可作为实现证据，不能当成新云环境已配置密钥或已验四槽TLS组合。此次最小缺口仅是**签发材料、逐设备名单、匹配固件／App和公网任务入口**。

本批通过既有只读工具核实际运行容器、配置消费者与数据库；四容器健康，原镜像／配置匹配、WARN且诊断关闭，没有签发Grant或设备／scope变更：

| 项目 | 实施前新云环境实际结果 | 结论 |
| --- | --- | --- |
| Gateway业务Grant密钥环／有效Task signer | 空／null | 实际算法、keyId、公钥和公钥指纹**尚不存在**；P256／Ed25519默认数值1只是配置默认，不是加载成功 |
| Gateway任务名单／Ed25519子集 | 均空 | 未准入任何设备，不把已有普通WiFi设备自动开放为任务设备 |
| Broker任务名单 | 空，与Gateway一致 | 原Cloud控制和自动化保持，不绕过设备Hello／carrier检查 |
| Task scopes／requests／attempts／UNKNOWN | 0／0／0／0 | 本批没有旧云Task scope可复用或需重放 |
| Nginx任务路径／beta任务开关／公开Arduino profile | 关闭／false／0槽 | 当前不能宣称云端任务已可用，普通自动化已启用不代表任务已启用 |

现场另有非本批设备记录，均不纳入名单；不因为共享Broker就共享设备权限。本批私有证据仍复用Service忽略目录`build/d3c-cloud-observer-20261020`，`task-admission-1792497448167.json`只读摘要及受限原件保留；未导出私钥、token、DeviceKey或账号凭据，无新备份。旧本机P256／Ed25519公钥、RFC fixture及MQTT JWT均不得充作云端Task材料。

### 冻结的有限支持与最小接线

- **仅现有COM11的普通ESP32-S3手动WiFi逻辑设备**，后续实施前精确匹配实际实例、owner／tenant、active DIY Key和正常路由；完整ID只在运维私有输入核对。双侧只允许同一个显式ID，不用通配或自动纳入全部设备，不动R4及其他云端设备。原Core3.3.11、HTTPS＋MQTTS和现有任务／控制执行链复用。
- **仍为四槽**Countdown／Once／Weekly，沿原ESP32策略：60000 ms写入窗口／10次、Start间隔下限30000 ms、最大提交预算100 ms。现有目录返回实际容量及进度；不移用R4／Wio的1000 ms合同，不扩槽、不改变任务语义或CAS／UNKNOWN处理。
- **计划采用已有P256-SHA256-RAW（wire算法2）、keyId1**，复用ESP32的P256验签环及Gateway原签发消费者，不新增密码框架。必须先为此环境生成独立实际密钥，并核对容器真正加载的算法／keyId、原始公钥及SPKI SHA-256与固件一致；本批**未生成**运行密钥，候选里的1不是签发事实。公钥可交付，私钥只在受限运维目录以只读文件注入，不复用Auth／MQTT JWT／自动化HMAC。
- **部署候选默认不生效**：新增Service `deploy/beta/compose.tasks.yaml`、两空项`.env.tasks.example`和离线checker，复用现有`DEVICE_V2_BLE_P256_*`业务签发配置名，只增加双侧单ID名单及专用只读挂载。基础Compose／env不改；未来启用必须保留已部署的automation overlay，不能用基础启动命令误关自动化。checker不读密钥、不访问Docker engine，也不能替代实际公钥／文件权限／符号链接核对。
- Nginx仅显式`tasksEnabled: true`时增加原handler的`POST／OPTIONS /api/v2/devices/:id/tasks:execute`，精确首尾匹配；`/tasks`、尾斜线、任意子路径、内部及Direct等仍关闭。省略／false的三个输出与原输出逐字节一致，原自动化可独立组合；真正启用前仍须宿主`nginx -t`，静态测试不代表已reload。当前beta不改，不提前打开空入口。

### 前批未启用候选与验证边界

原Node22.22.3的单条有限队列**8/8通过**：实际Compose离线合并及空输入拒绝、双侧唯一名单／挂载校验、原Gateway配置与原P256 signer的内存fixture签发验签、精确路由／方法、automation组合及CRLF。审阅后补充挂载拒绝例，仅重跑受影响**1项**通过，含真实Compose解析的“移除保护／显式true”反例；不是新增第9项或重复整队列。Compose2.35.1会把显式`create_host_path:false`规范化为`bind:{}`，缺整个bind则拒绝，不能把默认可建目录误当保护已生效。内存fixture不是运行密钥，也不证明云端签发已启用。

本批只改部署适配／回归及本节与执行入口，未改Task业务／协议、App、设备源码、SDK或依赖；未构建镜像／APK／MCU、部署、初始化scope、签发真实Grant、创建或刷新Key、操作串口／ADB、重做真机矩阵或新增备份。原账号、授权、UNKNOWN和完整日志保留，无关修改保持，未暂存、提交或推送。**软件候选收口，不等于正式云端任务准入。**

### 前批计划的一次部署与真实短差异（已执行）

合并完成实际密钥／公钥匹配、四槽TLS候选原SDK定向链接、保留自动化的服务／精确入口及匹配beta启用，然后仅对同一COM11逻辑设备做真实App查询容量4、停用保存、一次Start、执行回执／新鲜State和原控制检查。复用有效恢复材料和完整请求日志，不重复旧Countdown／Weekly／断电／BLE／资源矩阵，也不再创建设备、拆出纯核对批次或用“部署健康”代替任务结果。当前公开`esp32-cloud-wifi-v1`仍0槽；内部四槽TLS构建须显式匹配本环境材料，不能松开原0槽图保护或为了工具通过而填入假Ed25519 key。

**持久scope的收尾必须与存储一致。** 公共任务API没有独立retire，删除单条任务或回退名单不等于退休scope；停机／重连也不清TaskStore。复用现有逻辑设备建立scope后，默认保留匹配的任务固件＋TaskStore＋云scope，可删除本批单条任务但不能自动恢复旧空Flash留下active scope。若确需恢复旧Store，只能走正常owner移除整设备／退休并核实际`localTasks=retired_verified`；单纯DELETE成功不证明设备签名退休已确认，而且此操作会删除原设备，**不作为本次默认收尾**。不SQL退休、不重放UNKNOWN、不强行重复Initialize。

D1／D2／D3及原任务核心均不重新排队。R4仍只有原固定四槽Plain有限支持，不因普通0槽TLS已验而开放四槽TLS；Wio、Error8、H2b／离线历史／Bulk和复杂自动化继续按原边界后置。

### D4 实际签发、匹配构建与COM11任务短验收口（2026-10-20）

**D4本批关闭：只有现有COM11获得四槽TLS有限准入。** 没有创建或删除设备、刷新Key、重做任务执行器／CAS、扩槽、升级SDK或重跑M5／H2／BLE／断电矩阵。此前“密钥与名单为空、App未启用”的记录是实施前基线，不再代表当前状态。

- **实际签发与云部署**：服务器独立P256私钥仅存受限运维文件（0600），未导出；实际算法2／keyId1，公钥SPKI SHA-256为`57f69d14769fd9a099c1d4cc11fac64f8389ee975200f9fa65f53e18b58a6025`。Gateway原配置消费者／signer和Broker均只准本次COM11的同一逻辑ID，Ed任务子集为空。保留原automation overlay，同镜像更新G/B；原56迁移、PG／Redis容器和其他业务配置保持。Nginx只加原`tasks:execute`精确路径，`nginx -t`／reload通过，未登录POST401／OPTIONS204／GET405，WARN且诊断关闭。
- **唯一匹配固件**：原Core3.3.11、单作业完整编译534.186秒，94编译单元，BIN **1,077,360 B**、SHA `56953759…49339be7`，静态RAM54,552 B。Task／P256／TLS实际符号与生成头公钥核对通过；未用SDK／namespace overlay。原单库renderer只新增显式development内部`esp32-cloud-task-d4-v1`，固定四槽及60000／10／30000／100原策略，P256单环、Ed typed-null；公开`esp32-cloud-wifi-v1`的0槽／禁图合同保持。4项新profile回归、旧profile检查与公开0槽定向项通过，独立源码窄审无阻断；不是公开四槽ZIP或任意Sketch准入。
- **板身份与保全**：ROM和USB确认COM11 `8c:bf:ea:1b:96:14`。只读分区／20 KiB NVS均与既有canonical一致，`bl_v2_tasks` namespace不存在；未重新读取／保留整片备份或清区。只写`0x10000`应用区并独立verify，原NVS和分区前后保持。启动`begun=1`／容量4／有效时间／cloud active／错误0，power=false、powerWrites=0。原canonical仍保留，但建立新scope后不再将它自动恢复到当前设备；COM9等其他板未操作。
- **匹配App**：仅beta启用原tasks＋automation，其余能力继续关闭，生产／development不变，复用原页面／guard／执行链。工具7项、环境／入口24项、官方Angular guard5项及类型通过。原工具链唯一Gradle4分37秒完成；APK **73,776,843 B**、SHA `7a6f5ce0…3605f21c`，原包名／signer、HTTPS／禁明文、19原生类及1150项Web资产逐字节核对通过。一次覆盖安装读回一致，原owner、token pair及原journal保留；没有新APK／账号整包备份、验证码或Key操作。
- **真实App的一次任务闭环**：首次正常查询得到scope-required409并显示明确初始化提示；确认后设备接受初始化，新鲜空表为0/4。正常表单保存task1：停用10秒Countdown、power=true，列表显示1/4与“等待手动开始”；正常启用后**仅一次Start**。到期只读串口powerWrites **0→1**、power=false→true；App正常新鲜读回“本次执行已完成”，Broker实际State **revision2／power=true**。保留完成定义，不自动再Start，也不拿Start的管理确认冒充到期完成。
- **真实回执／签名与原控制**：Task ledger为scope1 active、request6／attempt6均verified、UNKNOWN0，分别Initialize1、保存1、Enable1、Start1及新鲜Read2；6笔实际Grant均以本批公钥独立验签成功，设备真实receipt及完整关联日志保留。新鲜record Read与已验证回执匹配，task1的tableRevision3／startSequence1／progress3（Completed）；**没有独立推送完成ACK，完成证明是这次Read＋MCU回调＋State2，不是Start接受回执**。保存／Enable／Start／到期读回与State关联当前会话；Initialize在前一连接，**不声称全轮同session**。之后真实App仅一次power关闭，普通Command sequence33／attempt1／ACK200，同当前会话新鲜State **revision3／false**，串口计数 **1→2**；ledger→ACK77 ms只是该次样本，不代表点击至动作时延或长期性能。
- **明确收尾**：COM11保留匹配四槽固件、默认TaskStore、active云scope和一条已完成Countdown；不会自动重启倒计时。没有恢复旧空Flash、退休scope或删除设备。beta与云端三overlay显式启用保持，最终App回首页，原账号／两个设备记录保留；Task和automation实际intents均0，原UNKNOWN／完整日志不删、不重放。云端四容器健康，最小自动化无新增execution；串口、ADB forward及SSH均已释放。无关本地修改保留，未暂存、提交或推送。

运行恢复接线：本环境后续重建必须保留base＋automation＋tasks三份Compose／私有env，不能用旧两文件命令静默退回任务名单。Task运维私有材料位于服务器`/opt/blinker-iot-beta/operators/d4-tasks-20261020`及本批私有key/env，私钥不进入仓库；唯一公钥和实际部署／回执摘要在Service忽略目录`build/d4-cloud-task-20261020`。Library `build/d4-task-firmware-20261020`、`d4-app-20261020`、`d4-runtime-20261020`分别保留冻结输入、构建及实际安装／请求／串口证据，原恢复材料未删除。

工具首停单独保留：首次将确认弹窗拆成两次`am start`触发了既有generation保护，未派发Initialize；改为附着现有进程、同会话确认后才有唯一业务初始化。两个刷新选择器因Ionic把aria转移入shadow button未命中，没有HTTP；改用实际图标按钮只刷新一次。一次只读shell求值因引号停止，无API／设备操作。均不是产品执行失败、重复任务或重放，首错及完整日志保留。

**当前支持是现有单设备的受限内测，不自动推广到全部ESP32。** 普通公开示例仍0槽，R4固定四槽Plain不扩为四槽TLS；Wio／Error8及H2b／Bulk等原后置边界不变。D4已通过项不再排成待实现；后续只按真实缺陷或独立功能验证安排，不再自动增加核对批次或资源审计。

### D4 后续核对与剩余范围（2026-10-21）

**结论：D4限定范围没有剩余必须重做的任务验收。** 本轮交叉核对上述实际收口、Service `build/d4-cloud-task-20261020/closeout.json`／`completion-read-proof.json`及板、App证据，未发现查询、保存、Start、执行结果或原控制尚欠验收。前半节的空密钥／空名单／功能未启用属于实施前历史，不能重新生成待办。本轮新增的实际差异仅为COM11恢复接入后未连上原WiFi；用户确认使用mostfun2026后，只对齐该网络并核对原持久表，没有重做任务执行闭环。

| 范围 | 已有证据与本轮核对 | 执行决定 |
| --- | --- | --- |
| COM11四槽TLS任务闭环 | 实际P256算法2／keyId1、唯一ID准入、原SDK链接、真实App查询／保存／启用／唯一Start、到期Read＋MCU回调＋新鲜State、原控制ACK／State均已完成 | 不再Initialize、保存、启用／Start或重跑执行／断电矩阵；本轮仅必要网络差异构建和应用区更新 |
| 保留的任务状态 | 10-21真实App全新查询仍为容量4、tableRevision3、task1 Completed、definitionRevision1、startSequence1，原10秒／power=true／enabled定义逐项一致 | 原scope直接续用，未初始化或回灌旧空Flash。3笔新查询均200／fresh／verified；不把前批6笔请求总数误写成今日云账本总数，也不冒称新做了PG账本审计 |
| 当前App与账号设备 | 沿用10-21已安装的匹配beta；原账号2台设备及精确COM11逻辑设备保留，进入原设置页的正常任务入口显示“已用1/4条”和“本次执行已完成” | 未回装旧APK、重新登录、创建／刷新Key或部署服务；仅2笔Catalog＋1笔Read，不发任务mutation。离页5秒无新增任务HTTP，原任务／自动化journal哈希及账号保持 |
| 当前硬件与网络 | 最初Ports为空，随后COM11恢复枚举；USB序列号／VID／PID匹配原MAC。原候选为network=0／cloud=0／timeValid=0；更换WiFi参数后Active、network=1／cloud=1／timeValid=1、错误0，powerWrites仍0 | 不操作COM9或其他板，不把目录active当成在线证明；没有人工断电、BLE循环或新任务触发 |
| 公开支持与独立后置项 | 普通公开Arduino profile仍0槽；自动扩大ESP32名单、R4四槽TLS、Wio、Error8及离线历史／Bulk未因D4开放 | 属于独立支持范围或后置工作，不计作D4核心验收缺口；未指定新缺陷／独立验收目标前不自动扩展 |

**网络更新的最小构建与保全：** 仅在受限忽略目录生成新WiFi头与Sketch副本，DeviceKey行不变；原93个编译单元及660项冻结输入未修改。先用原对象、原SDK链接recipe独立复现旧BIN的精确SHA，再只重编1个Sketch并重链；SDK仍3.3.11、P256公钥／四槽／TLS／原写策略及`bl_v2_tasks`不变。新BIN为1,077,360 B、SHA `e8a3c4c0…99adda13`，静态RAM仍54,552 B；不是新图资源准入或公开四槽profile。原D4源码、BIN、ELF及恢复材料保持。

只向`0x10000`写入新应用；为保护已存在的TaskStore，仅新读20 KiB NVS与4 KiB分区作ROM停留期间的完整字节校验，确认任务namespace存在，写后application／NVS／分区verify均通过再恢复启动。未读／备份整片8 MiB、清区、恢复旧canonical或修改身份；启动后WiFi自身存储正常更新不等同于任务表被重置。匹配新网络的固件保留在COM11，原Store及scope以本次真实fresh Read确认延续。

本轮证据在Library忽略目录`build/d4-network-20261021`、`build/d4-runtime-20261020`新增只读观测，以及App `build/account-ux-20261021/d4-readback.result.json`。工具首停为测试保护器误拦CORS OPTIONS，尚无任务HTTP响应，未派发mutation；修正为允许预检后，使用全新查询ID完成3笔只读操作。原首错及完整请求记录保留，没有把工具错误覆盖成业务成功。

未部署服务、调整签发材料／准入／账号额度或清除授权／UNKNOWN。串口、ADB专属forward均释放，App回到“我的”页；未暂存、提交或推送。后续若出现真实任务差异，先以同一逻辑设备的新鲜只读结果定位，禁止用重复Initialize／Start或清Store来“恢复”。
