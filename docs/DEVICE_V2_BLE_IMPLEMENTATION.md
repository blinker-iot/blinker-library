# Device v2 BLE 实施方案

> 状态：2026-08-29，4 B BLE wire、Noise enrollment、Method 2、DirectSecureRecord、official BLE
> composition 与 App/Service 链路已实现；ESP32/XIAO 和 UNO R4 真机核心门禁通过。Wio Terminal 在 Seeed
> 官方 RTL `2.1.3` 上已通过 `resetAccess()` 后的同 owner 三端重认领、Direct 恢复、Manifest/State、
> Power/Button、App kill、同 Sketch 重刷与真实重新上电持久恢复。三次空连接 deadline、两次认证 deadline、
> 32 次完整 GATT 重连及 heap 水位门禁已通过；仍保持 opt-in preview，只等待真实 owner DirectSecure 的
> 24 h 长稳门禁。具体证据见 `DEVICE_V2_WIO_BLE_G1D_RELEASE_GATE_REPORT.md`。

## 1. 分层结论

BLE 不再是 ESP32 专属协议实现，而是四层：

~~~text
DeviceRuntime / BBP/2 + CBOR
  └─ BleFrameTransport（BBP/2薄wrapper）
      └─ BleRecordTransport（所有 MCU / record格式共用）
          ├─ logical record 分片/重组
          ├─ 固定发送队列、背压、超时
          └─ session/authentication 元数据
              └─ IBleLink（平台接口）
                  ├─ Esp32BleLink
                  ├─ ArduinoBleLink（UNO R4/Renesas）
                  └─ WioTerminalRpcBleLink（SAMD51 + RTL8720DN RPC）
~~~

接入新 MCU 时，不复制 CBOR、BBP/2、会话或重组代码，只新增一个
`IBleLink` adapter，职责限定为：

- 初始化 BLE peripheral、service、characteristic；
- 报告连接、断开、订阅和安全状态；
- 收发单个 characteristic packet；
- 把协商/已知的最大 packet size 报给通用层。

## 2. GATT 契约

Direct BBP/2模式使用独立收发 characteristic，不再复用单个 FFE1：

| 项目 | UUID | 属性 |
|---|---|---|
| Service | `5f6d0001-3f5b-4e4f-9f4d-626c696e6b32` | primary service |
| App → Device | `5f6d0002-3f5b-4e4f-9f4d-626c696e6b32` | write / write without response |
| Device → App | `5f6d0003-3f5b-4e4f-9f4d-626c696e6b32` | read / notify |

ESP32 和 UNO R4 使用相同 UUID 与 wire bytes。Adapter 可通过 config 替换 UUID，
但正式 App 接入前应冻结默认值，生产设备不应按平台分叉 UUID。

## 3. BLE 分片格式

每个 characteristic packet 是 4 字节分片头加 payload：

| Offset | 大小 | 字段 |
|---:|---:|---|
| 0 | 1 | magic `0xB2` |
| 1 | 1 | 高 4 bit version=`1`；低 bit0 START、bit1 END |
| 2 | 1 | 非零 session-local、direction-local frameId |
| 3 | 1 | fragment index `0..255` |
| 4 | N | 当前mode的logical record bytes，N 必须大于 0 |

START/END 组合对应第二字节 `0x10/0x11/0x12/0x13`。首片 index=0 且至少携带当前record格式的
完整长度header（BBP/2为10 B，local secure record为4 B），接收端从该 header 得到logical总长度；不再在每片重复totalLength
和 offset。首片 Data 长度固定本 frame 的 payload capacity P，offset=`index*P`；所有非终片
必须恰好 P 字节，终片为 1..P 字节且必须刚好结束 frame。

index 不得在 frame 内回绕，一帧最多 256 片。ATT payload=20 时 P=16，BLE logical frame
上限因此精确为 4096 B。MTU 增大时当前 frame 继续用原 P；MTU 降低无法继续原 P 时，
发送端必须换新 frameId、从 index 0 和完整 logical frame 重启。完整 wire 状态机以
`BBP2_NORMATIVE_WIRE_SPEC.md` 第 10 节为准。旧 9 B 原型不兼容，也不保留双格式探测。

`BleFrameTransport` 严格拒绝重复、缺口、乱序、错误 stride、未知 flag/version、超时和
断连残留；只向 Runtime 提交长度一致且通过 BBP/2 frame 校验的完整结果。

### 20-byte ATT golden vector

以下 45-byte BBP/2 frame 使用 `frameId=1`、ATT payload 20 分片。P=16，共 3 片；
每行是一个完整 characteristic packet：

~~~text
B2 11 01 00 42 4B 02 11 00 0A 00 07 00 23 0A 0B 0C 0D 0E 0F
B2 10 01 01 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
B2 12 01 02 20 21 22 23 24 25 26 27 28 29 2A 2B 2C
~~~

ATT payload 244 时同一 frame 是一个 49 B packet：`B2 13 01 00` 后直接跟上述完整
45 B BBP frame。

默认 GATT UUID 已集中在平台无关 `protocol/ble/Gatt.h`，两个 adapter 均引用同一常量，避免后续平台分叉。

## 4. 内存和背压

核心不分配堆。调用者显式提供：

- RX：`maxFrameSize * BLINKER_BLE_MAX_SESSIONS`；
- TX：`maxFrameSize * queueSlots`，slot 数不超过
  `BLINKER_BLE_MAX_TX_FRAMES`；
- packet scratch：至少 14 字节（4 B fragment + 10 B BBP header），通常按目标最大 ATT
  payload 配置。

UNO R4 32 位 ABI 实测 `sizeof(BleFrameTransport)`：Small 160 B、Standard 240 B；该数字
只含 session/queue metadata，不含上述 caller-owned buffers。SizeProbe 同时设置
Small ≤160 B、Standard ≤256 B 的编译门禁。

历史 portable Noise 探针的UNO R4 32位ABI实测为：旧固定 session Small/Standard均272 B，
`BleNoiseProvisioningChannel`为512/592 B。它另需caller-owned `RX 1028 + TX 1028 + plaintext 1008 +
ATT scratch`；Small + ATT20固定合计3,616 B，不含平台crypto provider和上层application workspace。
本批共享 `NoiseNnSession` 需要重新记录32位ELF结果；64位Host为280 B，不能拿Host尺寸替代UNO R4结论。

发送队列复制完整 BBP/2 frame，因此 transport 返回成功后，调用者可以立即复用原
frame buffer。广播在入队前检查所有接收者容量，保证全入队或全不入队；不会只发给
一部分连接。底层返回 `WouldBlock` 时保留 offset，下次 `poll()` 原位继续。

ESP32 Arduino BLE 的 `notify()` 不返回真实 controller 拥塞状态。硬件部署建议把
`maxPacketsPerPoll` 设为 1；若需要可靠的多连接高吞吐，应新增能暴露 notification
完成/credits 的 NimBLE 原生 adapter，而不是在通用协议层加入 `delay()`。

## 5. 会话与安全

每次 BLE 连接产生新的非零 `sessionId`，断开后：

- 清除未完成 RX frame；
- 删除该 session 的待发 frame；
- 清除 encrypted/bonded/authenticated 状态；
- 重连必须重新 HELLO 和认证。

配对、加密、bonding 与“这个 App 用户有权控制设备”不是同一件事。所有 adapter
默认都把新连接标成 unauthenticated；业务代码不得伪造 encrypted/bonded/authenticated。平台若启用原生
链路加密，adapter 必须从当前连接的 stack/controller 事实源读取；缺少事实源时不能声明该纵深能力。
BLE Direct composition 可在普通 GATT 上完成 HELLO/Method 2，但只有显式
`EstablishDirectSecure` authorizer 成功原子建立 DirectSecureRecord 后，已验 AEAD 的 frame 才以
`encrypted=true` 进入 Runtime。COMMAND、ManifestRequest 和 StateRequest 同时要求该安全事实与 session 权限。

`BleFrameTransport::capabilities()` 不声明 `TransportFeatureAuthenticated`：4 B 分片和 BBP/2 frame 校验
都不是认证。静态 capability 不能替代每个连接的 `BleSessionInfo` 安全事实。

当前没有把“看到某个 bonded device”自动等同为当前 central 已授权；ArduinoBLE 2.0.2 的
`BLE.paired()` 是 any-peer 信息，已由架构门禁禁止在 Blinker adapter 中使用。

## 6. ESP32 NimBLE Port

正式 ESP32 BLE 产品只选择 `ports/esp32/Esp32NimBle.*`；旧 Bluedroid/兼容诊断 adapter 已删除。
`ports/arduino/HardwareRandom.*` 中的 `PlatformHardwareRandom` 通过 `esp_fill_random()` 提供协议 nonce，
不受 Arduino `randomSeed()` 降级影响。NimBLE Port 的安全契约为：

- 官方基线使用普通 GATT bearer，不主动触发 SMP、系统配对或 bonding；RX/TX characteristic 不要求
  native encrypted permission；
- Provisioning 的机密性和完整性由 Noise 负责，Direct 会话由 Method 2 认证并强制进入
  `DirectSecureRecord`；未经 AEAD 验证的业务 frame 不能进入 Runtime；
- 若未来某个平台产品额外启用可靠的 link encryption，`encrypted/bonded` 只能来自当前连接的 controller
  事实，不能由业务代码伪造，也不能替代应用层授权；
- 只有 TX notify 已订阅的物理连接才晋升为 Device V2 bearer session；callback 只写固定字段/队列，
  session 事件和协议处理仍在 `poll()` 执行。

2026-08-28，XIAO ESP32-S3 + Android + 本机 Service 已通过普通 GATT 上的完整真机门禁：
`Provisioning -> Noise -> grant/receipt/commit -> Direct -> Method 2 -> DirectSecureRecord ->
Manifest/State -> Power Command`，协商 ATT MTU 为 256，但 BLE v1 分片仍固定为 20 B；全程没有系统 PIN、
pairing 或 bonding。此前保留 SMP/SC/bonding 的体积数据仅是历史归因基线，不能再作为当前发布构建结果；
当前简化 Port 的资源数字以同一版本重新编译后的门禁结果为准。当时测试专用、现已删除的
`AuthorizedPresencePreviewProbe` clean compile 为 690,475 B Flash、38,564 B global RAM；该历史数字只代表
BLE-only 探针。2026-08-29 正式默认构建不再定义 Presence 预览宏：公开 `BleQuickStart` 为
692,943 B / 38,612 B，`WiFiBleQuickStart` 为 1,283,647 B / 60,036 B（Flash/static RAM）。

2026-09-02 当前原位 profile 刷新实现使用 Arduino-ESP32 `3.3.6` 完成 Android + XIAO 真机复测：Setup
认领后不重启 NimBLE Host/controller 即切到 Direct，重连约 3.6 s，Manifest/State 与设备页 Command/ACK
闭环通过，App kill 后凭据和 State 可恢复。相同源码的正式 `WiFiBleQuickStart` clean compile 为
`1,274,055 B Flash / 54,028 B static RAM`；42 个 Host/ASan 测试与 200,000 次确定性 fuzz 全部通过。
正式 `BleQuickStart` 同批 clean compile 为 `696,579 B Flash / 33,412 B static RAM`。认领后物理 reset
能够恢复 Direct v3 与 Method 2；随后不擦 NVS 重刷该正式示例仍沿用同一逻辑设备，初始 State、Power 与
Action Command/ACK 均通过。Manifest 从 1 个字段扩展到 3 个字段时，App 能提示 2 个新增能力并在用户确认后
将 Layout revision 从 1 更新到 2。由此关闭纯 BLE 的原位模式刷新、认领后 MCU 重启、Sketch 重刷和
Manifest 变化门禁；WiFiProv→Direct、locator 长稳及 WiFi/Direct 共存仍未关闭。

## 7. UNO R4 / ArduinoBLE Port

`ports/arduino/ArduinoBleLink.*` 依赖外部 ArduinoBLE，不把 ArduinoBLE 源码塞进 Blinker
主库：

- 单 central；
- 默认 packet size 20 B；
- 用 `written()`/`subscribed()` 驱动 RX 和通知状态；
- App 可按已验证平台能力显式提高 `maxPacketSize`；
- `ports/arduino/HardwareRandom.*` 的 `PlatformHardwareRandom` 直接调用
  Renesas SCE RNG、检查 FSP 返回值，并在复制后清除 128-bit 临时块。

通用 ArduinoBLE Port 不猜测安全状态，也不提供 `setSessionSecurity()`。UNO R4 正式 composition 使用
`requireEncryption=false`：ArduinoBLE 2.0.2 在本次 Android 真机上出现 Security Manager DHKey mismatch，
且公开 API 不能完整、可靠地冻结 per-connection pairing/bond 恢复合同。已删除不再使用的
`UnoR4BleSecurity.*` exact-handle probe，避免让“能读到一次 encryption-complete bit”被误解为产品安全根。
UNO R4 的安全路径是 Noise enrollment + Method 2 + DirectSecureRecord；未来 core 若提供可靠 native
encryption，可作为平台配置叠加，但不得改变 wire/API。

UNO R4 WiFi、Renesas core 1.5.3、ArduinoBLE 2.0.2 编译结果：

| Probe | Flash | 静态 RAM |
|---|---:|---:|
| 通用 ArduinoBLE + SCE random | 91,132 B / 262,144 B | 9,836 B / 32,768 B |
| BLE 产品 Probe（Noise + Method 2 + DirectSecureRecord，`Small`） | 186,572 B / 262,144 B | 16,788 B / 32,768 B |

R4b-5D 后完整 internal WiFi+BLE composition 为 211,332 B Flash / 18,812 B global RAM，静态 singleton
仍为 9,008 B；剩余 50,812 B Flash / 13,956 B RAM。record transport 已自动拒绝未准入包、活动
sessionId 二次宣布与已成立安全位回退，并在漏失 disconnect 时对账关闭 session；这些门禁不增加 static RAM。
UNO R4 纯 BLE 真机认领、Manifest/State、Command/ACK、App kill、固件重刷、持久重连，以及 active central
仍连接时的 `resetAccess()` -> 重认领均已通过。ArduinoBLE Port 在 stop 时显式发送 HCI Disconnect，并只调用
无 timeout 参数的非阻塞 `BLE.poll()`；UNO R4 的 `BLE.poll(timeout)` 可能无界等待，不能用于 stop/restart 路径。
剩余门禁是 24 h 长稳、异常断连/背压及纯 WiFi 的独立回归；UNO R4 不提供 WiFi+BLE composition。

UNO R4 的 Arduino `random()` 初始实现虽然可使用 SCE，但任意一次 `randomSeed()` 都会让后续调用切换到 `rand()`，因此不能作为认证 nonce 来源。当前 adapter 隔离了对 Renesas core 私有 `HW_SCE_*` 符号的版本耦合；升级 core 时只需重新验证该 adapter，不影响协议和 Runtime。

项目旧的 `src/modules/ArduinoBLE` 缺少标准独立库结构，Arduino CLI 不会完整递归
编译其 `local/remote` 源文件。新架构以独立依赖替代它；旧分支可继续保留，最终切换
主库时删除内嵌副本。

### 7.1 Wio Terminal / rpcBLE Port

Wio 的 SAMD51 运行协议、Noise、Method 2 和 DirectSecureRecord，RTL8720DN 只提供普通 GATT bearer。
由于官方 `2.1.3` 的闭源 `btgap.a` 不能可靠恢复 Android RPA identity keys，正式 Blinker Port 不请求系统
pairing/bonding，也不伪造 native `encrypted/bonded`。Noise/Method 2/DirectSecureRecord 提供应用层机密性、
完整性、身份和防重放；该路径现已在官方 RTL `2.1.3` 上通过完整三端真机门禁。

2026-08-27 使用 Seeed GitHub Release 原始三镜像回刷并读取 `RTL_VERSION=2.1.3` 后，完成了两层门禁：

- 已安装路径：既有 credential 的扫描、Method 2、DirectSecureRecord、Manifest/State 与 Power/Button；
- 重认领路径：设备 `resetAccess()` 进入 Provisioning；非 owner 账号的认领被 Service 精确拒绝；owner 账号完成
  `Hello -> Noise NN -> intent/grant -> encrypted relay -> receipt -> commit -> Direct mode -> Method 2 ->
  DirectSecureRecord`，Service 将旧 active intent 标为 superseded，并继续使用同一 logicalDeviceId；
- 恢复路径：退出/重入设备页、杀掉并重启 App、重刷同一 Sketch 后均可用持久 ControllerCredential 重新认证；
  重刷后的 App 状态先回到设备代码默认值，再次执行 Power/Button 后从 State 1 更新到 State 3，串口分别收到
  `COMMAND power=1` 与 `ACTION button count=1`。
- 真实重新上电路径：设备直接恢复 `state=4, local=1, error=0`；App 冷启动在首页恢复“蓝牙在附近”，进入设备页
  后无需重新认领即可恢复 DirectSecureRecord、Manifest 1、Layout 1、State 1，并再次完成 Power/Button 双向闭环。
- App 已修正嵌套 `ion-tabs` 返回缓存首页时不重启 BLE presence 扫描的问题：`/home/device` 的
  `NavigationEnd` 复用现有 presence service。Wio 重刷后设备页先正确离线，返回首页可直接恢复“蓝牙在附近”
  并再次进入 Direct，不需要重启 App。

本次重新编译/烧录测试固件为 178,864 B program storage；此前 ELF 资源探针为 178,696 B `.text`、
672 B `.data`、64,452 B `.bss`。

可复现探针 `tests/arduino/WioBleReleaseGate` 为 178,832 B `.text`、672 B `.data`、64,452 B `.bss`。
BLE 初始化并进入 advertising/ready 后，
48 KiB FreeRTOS heap 剩余 17,120 B、历史最低 17,072 B；newlib heap end 到其实际 MSP 限界仍有
109,936 B。以上把 RTOS heap 和普通 `malloc/new` heap 分开记录，不能相加成单一“可用 RAM”；长稳门禁仍需
确认这三个水位不会随反复连接持续下降。

Port 与协议层现在有两个不同 deadline：已连接但未开启 notify 的物理 central 15 s 后断开；notify 已就绪但
未完成 Noise/Method 2 的逻辑 session 15 s 后由 transport 请求物理断开。协议失败和 active secure replay
同样退役连接。Host ASan 已连续执行 128 轮未认证连接并确认唯一 session 槽位可重复使用。

独立 `seeed-ambd-firmware` 分支只保留 FTL/key-storage/bonding 失败的复现环境。当前补丁无法修复 Android RPA
冷恢复，不能以生产 bonding 修复提交 PR。完整应用层门禁已经证明 Blinker 当前正式支持不需要自维护 RTL
镜像，发布依赖冻结为 Seeed 官方 `2.1.3`。若后续能获得修复后的 Realtek host binary/source，原生 link
encryption 可作为可选纵深防御重新验证，但不得改变公开 API、BBP/2 wire 或应用安全基线。

## 8. BLE + Wi-Fi/MQTT 同时工作

二者不是两套业务协议：

~~~text
Esp32NimBleLink / ArduinoBleLink → BleFrameTransport ┐
                                                      ├→ TransportHub → DeviceRuntime
IMqttClient → ManagedMqttTransport ───────────────────┘
~~~

`TransportHub` 分配 `transportId`，Runtime 以 `(transportId, sessionId)` 保存协商
状态。相同数值的 `sessionId` 出现在 BLE 和 MQTT 时也不会串会话。定向响应只回请求
来源；设备主动状态更新可显式 broadcast 到两条 transport。

`IFrameTransport` 的 session connect/disconnect 事件经 Hub 统一补上 transportId。
Runtime 按 session 发送并重试 HELLO；BLE 断开即释放 peer 槽位。因此 BLE link 保持
`Online` 时的多次 central 重连不会继承认证，也不会在默认 8 个 peer 槽位中泄漏。

## 9. App 首次连接流程

~~~text
BLE connect + subscribe（native pairing/encryption 可选）
  → BBP/2 HELLO / HELLO response（设备不会在加密前宣布 session）
  ↔ Authenticate / AuthResult(Continue)
  ← AuthResult(Authorized)
  → ManifestRequest
  ← Manifest（endpoint 能力，不是 UI 坐标/颜色）
  → StateRequest(cursor, observedRevision)
  ← StatePage response（同 revision 分页；基线 key:value，协商后可用短 ID）
  ↔ COMMAND / PATCH / ACK / ERROR（key/ID 双模式）
~~~

用户拖拽生成的页面 schema 仍保存在 App/服务器，组件绑定 endpoint key。Manifest
用于校验能力和生成可选的默认页面，不覆盖用户的页面布局。CBOR body 只存在于发送
frame buffer、接收/reassembly buffer 或显式离线队列中，不由 codec 隐式持久化。

## 10. Provisioning / Direct 显式模式所有权

当前不再允许把`BleNoiseProvisioningChannel`和`BleFrameTransport`同时绑定一个`IBleLink`，也不读取首包
`BK/B3` magic后分流。`BleModeCoordinator`、`IBleModeLink`、`ITransportLifecycle`和
`IBleModePreparation`组成唯一切换路径：构造不抢callback，只有成功start的endpoint能stop物理link；切换前
校验完整profile，相同profile幂等，失败时重配旧profile并重新prepare/start，回滚失败进入Error。

两种模式复用`5f6d0001-3f5b-4e4f-9f4d-626c696e6b32`物理GATT service。模式不靠UUID猜测，而由scan response
中的13 B Service Data严格表达format/mode/wire/capability/setupSessionId。切换会断开central，App必须重新
扫描和连接；这是ArduinoBLE/NimBLE/Bluedroid共同可实现的portable baseline。完整wire、状态机、App行为和
资源结果见`DEVICE_V2_BLE_MODE_OWNERSHIP_AND_ADVERTISING.md`。

## 11. 已知限制与下一步

- 设备侧 4 B wire 与 C++ vectors 已冻结；App/服务器 BLE gateway 必须在 P0-4 复用同一
  exact vectors，不能移植旧 9 B 原型；
- Authenticate/AuthResult 信封、显式 DirectSecure bootstrap policy、per-session permission 与失败锁定已实现；
  Method 2 ControllerCredential-HMAC-SHA256 的 selector/version/permission、双 nonce、双向 proof 和
  C++/TypeScript/Node exact golden vectors 已冻结；
- 旧 Broker 的 16-hex authKey 和实验性 Method 1 不再作为 Direct BLE 身份凭据，Method 1 生产源码已删除；
  Ownership-domain ControllerCredential 的 grant/receipt/commit、轮换与撤销 control-plane 已冻结；
  R4b-6A～6C 已完成 LocalBLE exact 安装、Noise application、共享 lifecycle，以及 ESP32/UNO R4 四个
  Platform BLE/LocalBLE internal product graph；
- 云端共享/访客仍需后续签名 capability method；
- ESP32 NimBLE正式候选当前单 central；仍需实机确认scan response、模式切换/回滚、notify背压、WiFi共存和24 h稳定性；
- UNO R4 默认 20 B，吞吐低于协商大 MTU 的 ESP32；核心真机链路已通过，仍需长稳/恶意断连门禁；
- Wio 官方 RTL `2.1.3` 已通过同 owner 三端重认领、App kill、同 Sketch 重刷和 Direct 数据面；自维护 RTL
  不再是应用安全发布依赖。readiness/authentication deadline、32 次重连和 heap 水位已通过，只剩
  24 h authenticated DirectSecure 长稳后才能移出 opt-in preview；
- legacy scan response已被128-bit Service Data占满，App不得依赖广播名；旧JSON/TLV scanner必须与v2 scanner隔离；
- `IBleModePreparation`边界已完成：普通NN不需要PSK provider；增强档才允许短暂装载PSK/re-arm，协调器
  不得常驻保存secret；
- COMMAND幂等、可靠PATCH重试和P0-4三端共享golden均已实现。官方 internal WiFi/BLE/WiFi+BLE
  composition 已完成软件/资源门禁，App production codec/session 已消费同一 vectors。统一 facade 的发布
  继续等待目标平台长稳、动态内存峰值和异常恢复门禁；native pairing/exact link encryption 不是通用发布前置条件。

鉴权状态、Manifest 指纹与短 ID 的完整契约见 `DEVICE_V2_MANIFEST_ID_AUTH_CONTRACT.md`；Controller Method 2 见 `DEVICE_V2_CONTROLLER_AUTH_METHOD.md`；COMMAND/PATCH 重试见 `DEVICE_V2_RELIABLE_DELIVERY.md`。
