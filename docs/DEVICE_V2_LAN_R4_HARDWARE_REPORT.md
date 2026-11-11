# R4 双账号 LAN 撤权真机记录

2026-09-12。范围：普通 ESP32 WiFi 内部 LAN 候选、真实 Android App、当前本机 Gateway/Broker。优先级入口仍为[基础能力收口路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)。

## 1. 结论与边界

实际完成：真实账号邀请/接受 operator 分享 → 手机 LAN 的 Hello、Manifest、State、Property/Action → owner 公开撤销 → 设备精确 Retire ACK → App 原 LAN 会话关闭 → 旧共享账号本地签发被拒 → 同设备重分享恢复。两次撤权期间，共享账号均保留原账户 MQTT Session，未重连。恢复 owner 后本地读/控仍正常。

后续§6已补第二台物理设备在同一共享账号下持续通信的隔离，**R4本工作包真机收口**，不是全发布矩阵完成。本轮没有重新发送旧 grant/record 的无线攻击用例：403 证明重新签发被拒，旧 session.command 拒绝证明已关闭会话不可复用，二者不冒充旧 wire 重放测试。原设备/服务端重放与退休软件门禁继续保留。

## 2. 环境与修正

- 手机 ADB 恢复；电脑在“带翅膀的蜗牛”，手机在该路由器的 Wi-Fi5 SSID，实际同为 192.168.2.0/24。COM12 经读 MAC 核对为原 XIAO ESP32S3（尾号 7C:6C），未碰 COM3/COM11。
- Gateway 仍公布旧网络 172.20.10.3，旧 TLS 证书也只包含旧地址。基于当前运行的 compose/镜像，仅修正测试地址为 192.168.2.76 和代理证书；原测试 CA 对当前 IP/127.0.0.1 校验通过。不导入历史 compose 的旧镜像，不修改数据库/schema、账号或绑定。
- App 仍使用本机 HTTPS/一个用户 MQTT；恢复 ADB reverse 17000、17443、18885。缺少 18885 时的 account retrying 属测试连通性问题，不需要第二条 MQTT。
- LAN 使用 profile 2：明文业务记录＋既有认证/权限/单调租约，不是无鉴权。本轮云登录/MQTT用 TLS，不改变普通 WiFi 默认可明文的合同。固件时间戳仅用于短期测试 CA，不代表 B3 生产授时已实现。
- 烧录工具 stub 在固定读取位置反复停止；ROM `--no-stub` 对相同位置和完整程序区读取成功。保存系统区/程序区备份后，只写 0x10000 应用镜像，写入 hash 校验通过。未清 NVS、刷新 Key 或擦绑定，DeviceKey version 仍为 2。不能由 stub 读取失败推断 Flash 损坏。

## 3. 实际结果

| 检查 | 首次分享 | 撤销后重分享 |
|---|---|---|
| 原 App 认证收件箱、operator 确认、自动进入设备页 | 通过 | 通过 |
| 原页面连接所有者进入 LAN ready | 1,520 ms | 1,348 ms |
| LAN Property / button Action ACK | 97 / 80 ms | 173 / 94 ms |
| buttonPresses 实际 State | 0→1 | 1→2 |
| owner 发起公开撤销至设备精确 Retire ACK | 544 ms | 1,334 ms |
| 退休早于原 300 s 租约到期 | 是 | 是 |
| 两实际 Broker 当前版本/route 回执 | 都已确认 | 都已确认 |
| 共享账号同一 MQTT Session 保持 ready | 是 | 是 |
| 旧共享账号重新申请 local-access | 403 | 403 |

第二轮额外保留原 LAN Session 对象，不读取密钥，确认 state=closed、原生 caller 空闲、后续 command 被拒。数据库两条目标授权均为 revoked / acl_changed，resolution=acked，指向各自有 exact_ack 的控制命令（sequence 2/3）；不是 superseded、TTL 清理或 Broker publish 成功。生产 receiveAck 仍验证当前 DeviceKey/proof、控制会话、sequence 和 MAC 后才持久确认。

恢复 owner 后 Manifest/State、Property/Action 通过，buttonPresses 2→3；该轮 Property ACK 为 1,052 ms、Action 为 76 ms。样本少且有慢样本，**不据此宣称 P95 或全部控制均小于 100 ms**。独立正常退出复测记录原生资源实际释放 29 ms，账户 MQTT 仍 ready。

没有自动重放不确定 Action；两个 accepted 测试邀请保留审计，最终共享关系 revoked v6。设备授权 revision 5、已确认 sequence 3。正常退出不会立即删除尚未到期的 owner 授权账目，它是原单调租约合同，不应为清理测试硬删证据。

## 4. 工程改动与回归

本批生产设备/App/Service 不增协议、连接、队列或权限系统。只改 `tests/hardware/lan_self_app_gate.mjs`：每次页面捕获自己的账户 Session 基线；保留无密钥的原 Session 引用；通过原 subscribeClosed 观察原生释放，避免把 caller scope.closed 误计为 socket 已关闭。另一个页面仍持有共享连接时，不强制关闭；生产生命周期合同不变。

- Arduino ESP32 3.3.11、XIAO ESP32S3/default_8MB：程序 1,126,004 B，静态 RAM 58,144 B。这是含诊断工作区的内部门禁，不是 WiFiQuickStart 资源数据。
- 实体 SDK WebSocket 入站 **9/9**：合法 challenge、空 body；拒绝 query/body/chunked/错误方法/超长头/提前 frame；拒绝后再次连接正常。
- App **68/68**：LAN 路由12、connector18、session16、client22。host authority/transport 两个可执行门禁重建并通过（MSVC ASan；仅补运行时 PATH，没有关闭 sanitizer）。
- Android debug 构建、保留数据安装通过；源码三个 LAN 默认开关仍关闭，只在生成的本机 debug 制品开启。

证据在 ignored `build/lan-r4-20260912/`：真实 API/DB 观察、App 时间线、编译/备份/烧录及运行配置检查点。私有账号、DeviceKey、WiFi密码、CA私钥与 Flash 备份不得提交。

制品 SHA-256：

- 固件：`37E6359AE3C881C639F97DA24790C63FBF5DD9EAC282A69F25C895A92A822D5C`。
- 已安装 debug APK：`68CC75A5227726F082F111BE8D7FF0960B3D7581206DACCD5C416CB00A7C075C`。

收尾：原 owner、`/home/device`、MQTT ready、原生通知 connected、本地 caller 空闲；跨 shard 请求/分享开关恢复关闭，notification installation/delivery保留原开启状态；修正后的地址/证书保留。无暂存、提交或推送。

## 5. 前批安排（§6已执行第1项）

1. 第二测试板使用普通 WiFi 标准示例作为持续通信见证，与本轮 LAN 目标同账户运行；撤销 LAN 目标时确认另一设备仍能读/控，补 R4 物理隔离尾项，同时开始 B1 的 ESP32 普通接入基线。不能把“owner仍可控同一设备”当“另一设备未受影响”。
2. 然后按 B1/B2 回到三平台基础接入与 owner 解绑。已再次核对 App DELETE 对应的 Gateway 设备详情当前仍只允许 GET；分享撤销不等于 B2 已实现。
3. B3 授时、B4 LAN 产品入口/其他 MCU 按原顺序；不转向 Hub/S4、通知保活或100次循环。后台通知、QR/逐组件、旧 wire 无线重放及全平台发布门禁继续单列。

## 6. 第二物理设备隔离与B1普通WiFi初步见证

2026-09-12，同日后续。COM12保留上一批LAN固件/逻辑设备；COM3经MAC核对为另一块XIAO ESP32S3（尾号F1:2C），COM11未动。两个账号通过原App/公开接口创建和接受两条operator分享；没有额外账户MQTT、伪造用户令牌或SQL写grant。

### 6.1 普通WiFi示例与重启

COM3使用标准`examples/WiFiQuickStart`的配置副本：公开Blinker.h/Property/Action/begin/run，loop仅Blinker.run，没有LAN、BLE、网关或额外用户发送循环。owner公开创建设备/reveal取得长期DeviceKey，仅存ignored私有配置。为了连接本机TLS代理，副本注入测试CA/地址/网络与短期构建时间；**不是原样ZIP、默认明文或B3生产授时验收**，标准示例源码未改。

Arduino ESP32 3.3.11、XIAO/default_8MB构建：程序1,033,204 B，静态RAM51,864 B，镜像1,033,360 B；依赖解析仅Blinker/Network/NetworkClientSecure/PubSubClient/WiFi。保留板上原较小分区表，核对镜像小于原app0的1,310,720 B后，只写0x10000并验证hash。系统区及ROM读出的程序范围已备份；失败stub产生的局部备份不作为恢复依据。不擦NVS/分区表/Key，未把本轮记成8 MB分区安装通过。

首次Manifest/State与Property/Action通过；COM3一次RTS硬复位后再次自动登录，状态回到power=false/buttonPresses=0，Property ACK185 ms、Action119 ms，真实buttonPresses更新为1。只读账目显示三个登录session记录使用同一个DeviceInstanceId和Key version1；session记录数不是重启次数或同时在线MQTT数。收尾owner仍可读/控，未重新复制Key或认领。

### 6.2 双设备并行隔离

使用同一已登录App的原DeviceV2Service观察COM3；LAN目标仍通过原DeviceUi页面连接端口。后台Property见证每次ACK后间隔1 s，最多24次/总期限120 s；不重放不确定Action。观察器只记录测试字段power/buttonPresses，独立持有State interest，不建立第二个MQTT或native连接。

| 检查 | 结果 |
|---|---|
| COM12 LAN Property / Action ACK | 129 / 205 ms，发送和ACK后均为LAN |
| owner撤销至COM12 exact Retire ACK | 1,950 ms，早于原300 s到期 |
| 两个实际Broker当前grant/route失效回执 | 分别确认 |
| 原LAN Session / 后续command / native caller | closed / 拒绝 / 空闲 |
| COM3 Property持续见证 | 13次ACK；撤权发起前完成5次，撤权完成后开始并完成5次 |
| COM3 Property ACK样本 | 最小117、中位129、最大271 ms，不声称P95 |
| COM3 Action | ACK159 ms，真实计数更新为2 |
| COM3 State观察 | 14条观察（含初始/Action），power实际true/false交替 |
| 原共享账号MQTT | 同一Session保持ready，无重连 |
| COM3 share version | 撤销COM12时仍active v1，未被波及 |

### 6.3 部署前置修正及收尾

初次观察收到wire code6，尚未读取Manifest/发送控制。定位为**联调配置遗漏**：Gateway开了跨shard requests/shares，但两个Broker未开requests，源Broker落回本地查找新设备。补齐两端Broker同名开关后原App立即取得真实Manifest/State，不修改授权规则、设备协议或增加连接。Broker重建引起的MQTT重连发生在隔离计时开始前，不能计成撤权中的重连。后续受控跨片部署必须预检Gateway和两端Broker，不能只开Gateway。

新增`cloud_witness_app_gate.mjs`复用原CDP工具；仅测试层增加观察所有权/有界采样/Action锁存。5项隔离生命周期单测通过：等待interest accepted、deadline/丢观察释放、不确定Action不重试、账户替换拒绝、采样上限/关闭迟到任务。最终版本再完成owner物理读/控/close；本批没有App/Service生产源码、UI或设备API改动，也不重复计入前批68项回归。

最终测试share分别revoked v8/v2；owner、/home/device、MQTT ready、通知connected、LAN caller空闲。Gateway和两个Broker跨片测试开关恢复原关闭值，原镜像/数据保留；COM3保留普通WiFi测试固件与owner设备页面，COM12原LAN固件不变。未删除审计/设备记录，未暂存、提交或推送。

证据：ignored `build/lan-r4-isolation-20260912/`；账号/DeviceKey/网络/CA私钥/Flash备份不提交。固件SHA-256：`44AF94FC65679EADE9F095F2071F765AB24C6063BAF28BC88D4BCB7380E79A7E`。

下一步：B1默认明文/单ZIP和其余平台矩阵，与B2 owner解除合同核对并行准备；然后B3授时→B4 LAN产品化。R4不再追加循环，普通WiFi样例通过不代表owner DELETE、BLE解绑或全平台已完成。
