# Device V2 BLE 模式所有权、广播与切换实施合同

> 状态：2026-08-14 设备侧软件门禁已退出；App 生产扫描器、产品 composition 与真机门禁未退出。
> 2026-08-16 R4b-4b：默认 DIY local wire 已冻结为 Noise NN；NNpsk0 只保留为显式
> Enhanced/Manufactured profile。两档共用一份状态机，连接内禁止协商/降级。广播 8 B 字段已纠正命名为
> `setupSessionLocator`，不能与 Ticket 的 16 B `setupSessionId` 混用。

## 1. 决策

Device V2 在同一时刻只允许一个逻辑端点拥有一个物理 BLE link：

- `Provisioning`：`B2` 分片承载 `B3 + Noise NN/NNpsk0 + EnrollmentTicket v1/WiFiConfig v1`；
- `Direct`：`B2` 分片直接承载 BBP/2；
- 两种模式复用同一组冻结的 128-bit GATT service/RX/TX UUID，不并行启动两个 BLE host，也不按
  payload 首字节 `BK/B3` 猜测协议；
- 模式由严格、版本化的 Service Data 明示。模式切换会停止旧端点、断开旧 central、重新配置并启动
  新端点，App 必须重新扫描和连接；
- WiFi/MQTT 可以与当前 BLE 模式同时运行。禁止的只是同一物理 BLE link 上两个逻辑 callback owner
  同时运行，不是禁止 WiFi + BLE composition。

采用单一 GATT UUID 是刻意的跨平台基线。ArduinoBLE、NimBLE-Arduino 与 Arduino-ESP32 Bluedroid 对运行中
删除/重建 service 的生命周期能力不一致；依赖两套动态 service UUID 会把平台差异重新泄漏到 Core。模式
已经由 Service Data 无歧义表达，切换又要求重连，因此两套 UUID 不增加身份或加密强度。

## 2. 分层与所有权

| 层 | 职责 | 不得承担 |
|---|---|---|
| `ble::ModeProfile` | mode、wire version、capability、8 B setupSessionLocator | 完整 setupSessionId、setup PSK、ticket、设备密钥 |
| `IBleModeLink` | stopped 状态下复制 profile；按 profile 配置广告并启动平台 BLE | 解析 BBP/2、Noise 或业务消息 |
| `ITransportLifecycle` | 通用 `start/stop/poll/state`；供 frame transport 与 secure channel 复用 | BLE 专属 profile 或平台 SDK |
| `IBleModePreparation` | 在 link stopped 且广告启动前准备选中端点；回滚时重新准备 | 常驻保存 raw setup secret |
| `BleModeCoordinator` | 唯一决定 provisioning/direct owner、切换和回滚 | WiFi credential、claim/activation 状态机 |
| `BleRecordTransport` | 选中端点启动时才绑定 packet/session callbacks | 构造时抢占 callback |

`IFrameTransport` 继承通用 `ITransportLifecycle`，没有继承 BLE 专属接口，因此 MQTT/WebSocket 等 frame
transport 不会在类型模型中被错误描述为 BLE endpoint。`BleFrameTransport` 仍只有一个虚表，UNO R4 Small
对象门禁保持 160 B。

`BleRecordTransport` 的所有权规则为：

1. 构造不绑定任何 link callback；
2. `start()` 仅在 `link.state()==Stopped` 时绑定并启动，否则在触碰 callback 前返回 `AlreadyExists`；
3. 平台 `start()` 失败时清 callback 并执行 link `stop()`，保证可配置/可回滚；
4. 只有成功启动的实例可以在 `stop()` 中清 callback 和停止 link；未启动或启动失败实例的 `stop()` 是 no-op。

第 4 条防止“第二个 endpoint 启动失败后又被清理，反而停掉第一个 owner”的隐蔽竞态。

## 3. 广播 wire

冻结 GATT service UUID：`5f6d0001-3f5b-4e4f-9f4d-626c696e6b32`。Primary advertising 放 Flags 与
完整 128-bit service UUID；legacy scan response 精确使用一个 128-bit Service Data AD structure：

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0 | 1 | AD length=`30` |
| 1 | 1 | AD type=`0x21`（Service Data - 128-bit UUID） |
| 2 | 16 | service UUID，Bluetooth little-endian |
| 18 | 1 | advertising format version=`1` |
| 19 | 1 | application mode：`1=Provisioning, 2=Direct` |
| 20 | 1 | selected wire version |
| 21 | 2 | capability bits，little-endian |
| 23 | 8 | setupSessionLocator；Direct 必须全零 |

也就是 Service Data payload 本身固定 13 B：

```text
formatVersion | mode | wireVersion | capabilities(le16) | setupSessionLocator(8)
```

能力位：

| bit | 含义 |
|---:|---|
| 0 | 4 B fragmented records |
| 1 | ordinary Noise NN |
| 2 | Enrollment Contract v1 |
| 3 | WiFiConfig v1 |
| 4 | Direct BBP/2 |
| 5 | enhanced Noise NNpsk0 |

当前合法 profile 只有：

| 模式 | wire | capabilities | setupSessionLocator |
|---|---:|---:|---|
| Provisioning/不收 WiFi | 1 | `0x0007` | 非零 8 B |
| Provisioning/收 WiFi | 1 | `0x000f` | 非零 8 B |
| Enhanced Provisioning/不收 WiFi | 1 | `0x0025` | 非零 8 B |
| Enhanced Provisioning/收 WiFi | 1 | `0x002d` | 非零 8 B |
| Direct BBP/2 | 2 | `0x0011` | 8 B 全零 |

未知 capability、未知格式版本、mode/wire/capability 不匹配、两种 Noise bit 同时/均未设置及非法 locator
必须拒绝，不能“尽量连接”。
Golden fixture 当前示例为：

```text
Provisioning Service Data: 0101010f008182838485868788
Direct Service Data:       01020211000000000000000000
```

31 B legacy scan response 已满，因此 adapter 不在 scan response 重复放 local name。GAP device name仍可存在，
但 App 不能依赖名字发现、判定模式或身份。`setupSessionLocator` 只是本次 setup window 的 8 B 公开定位值，
不是 Ticket 绑定的 16 B `setupSessionId`、secret、proof 或设备身份；广播中永远不得出现 setup PSK、ticket、
CloudCredential 或 WiFi 密码。

## 4. 切换和回滚

`BleModeCoordinator::switchTo()` 是同步控制面操作，只能由 Arduino `loop()`/产品状态机调用，禁止从 BLE、
Noise 或业务 callback 内重入。算法固定为：

```text
validate target profile
  -> exact active profile: success/no disconnect
  -> stop previous endpoint (disconnect + release callbacks)
  -> configure target advertising profile while link is stopped
  -> prepareBleMode(target, rollback=false)
  -> start target endpoint
  -> success: publish new active mode
  -> failure: stop target, configure old profile,
              prepareBleMode(old, rollback=true), restart old endpoint
  -> rollback also fails: enter Error with no active endpoint
```

目标 profile 在停止旧端点前校验，所以 malformed/unsupported 请求不会造成现有连接中断。相同完整 profile 重放
幂等；Provisioning 的 locator 改变不算相同 profile，必须重启 session，避免旧 setup window 继续可用。

exact profile幂等不等于断连恢复。Noise channel在disconnect、认证失败或协议fault后会清安全状态；产品在callback
中只置recovery flag，再从Arduino `loop()`调用`BleModeCoordinator::restartActive()`。该操作停止当前endpoint、
重新配置同一profile、再次执行preparation并启动广告；prepare/start失败进入`Error`，不得继续广播。禁止在BLE
SDK callback里读取NVS/factory secret或直接重入mode coordinator。

`IBleModePreparation` 是安全回滚必需边界。`BleNoiseProvisioningChannel::stop()` 会 fail-closed 地取消 Noise
session并清状态。普通 NN preparation 只需要 channel + application session，不读取任何 PSK；增强档才注入
`ISetupPskProvider`，按 exact locator 短暂读取 32 B PSK并立即清理临时副本。两档都通过
`ILocalProvisioningSession`重建 application 状态；该层必须另外持有完整 16 B setupSessionId，不能把 locator
当授权 ID。若 re-arm 或 application begin 失败，回滚/restart 必须失败并进入 `Error`。

## 5. 平台 adapter

`arduino-ble`、`esp32-nimble`、`esp32-ble` 都实现 `IBleModeLink`：

- 默认 profile 为 Direct，产品在首次 start 前显式选择实际模式；
- `configureBleProfile()` 只接受 stopped link，校验后按值复制 profile；
- adapter配置中的 service/RX/TX UUID必须等于冻结 UUID，避免 GATT service 与 raw scan response UUID 分裂；
- start 时生成 Service Data，不缓存外部临时 buffer；
- stop 后才允许下一次 profile 配置；当前 portable baseline允许平台完整 deinit/reinit BLE stack；
- 三个 adapter 使用同一 13 B encoder和 exact golden，不各写一套位布局。

ESP32产品首选同一个 NimBLE-Arduino host。`esp32-ble`/Bluedroid保留兼容和诊断用途，不得与 NimBLE package
同时组成同一固件的两个 BLE owner。UNO R4使用外部 ArduinoBLE，不把其源码重新内嵌进根库。

## 6. App 与服务器对接

App扫描顺序固定为：

1. 以冻结 service UUID做粗筛；
2. 等待并读取完整 Service Data（它可能位于 scan response）；
3. 严格解码13 B profile，拒绝缺失、截断、未知格式/capability或不一致组合；
4. setup只连接 `Provisioning`，用 locator 关联当前扫描窗口；完整 16 B setupSessionId 必须在受保护 payload 中取得；
5. Direct设备只进入BBP/2连接路径；不得通过读取首包再猜测Noise/BBP；
6. 设备完成activation并切到Direct后，旧连接断开是正常事件；App重新扫描，观察Direct profile后再连接；
7. 不按MAC、名字或缓存的peripheral对象断言身份；OS扫描缓存必须以本轮Service Data刷新结果为准。

服务器/Gateway不参与BLE advertising、GATT切换或Noise解密，也不接收setup PSK。Gateway签发绑定完整
16 B setupSessionId与握手 hash 的 EnrollmentTicket；它不把8 B locator、BLE MAC或adapter类型当设备身份。

旧JSON/BLE设备应留在独立legacy scanner/session边界。新service UUID命中的设备必须遵守本合同，不能在同一
连接中回退JSON/TLV；否则 malformed v2设备会意外降级到弱路径。

## 7. 资源和已通过门禁

2026-08-14当前结果：

| 项目 | 结果 |
|---|---:|
| 32-bit `ModeProfile` | 12 B |
| 32-bit `BleModeCoordinator`（含preparer） | 40 B gate |
| 32-bit Small `BleFrameTransport` | 160 B |
| 32-bit Small `BleNoiseProvisioningChannel` | 520 B（当前 shared NN/NNpsk0） |
| UNO R4 SizeProbe Small | 52,112 B Flash / 11,964 B static SRAM |
| UNO R4 + ArduinoBLE mode adapter | 90,392 B / 9,740 B |
| Wio Terminal Core Small | 83,436 B Flash |
| ESP32 NimBLE mode adapter | 615,159 B / 35,688 B |
| ESP32 Bluedroid mode adapter | 1,114,387 B / 40,464 B |

Host `11/11` CTest、MSVC ASan、20,000 deterministic fuzz、architecture、9-package boundary和三语言
golden均通过；Enrollment fixture从21项扩为25项，其中新增Provisioning/Direct Service Data及完整scan response。

上表其余编译/测试项是 2026-08-14 的历史切片。2026-08-16 R4b-4b 当前门禁已更新为 Host 33/33、
14-package boundary；UNO R4 32 位 `NoiseNnSession` / channel / preparation 分别为 272/520/16 B，
Small SizeProbe（故意重复实体化嵌套对象）为 52,268 B Flash / 14,168 B static RAM。

## 8. 未退出项

- 暂无真机：实际Android/iOS是否稳定取得scan response、OS缓存刷新、NimBLE/ArduinoBLE断开重启、notify背压和
  WiFi共存仍须实测；
- `IBleModePreparation`边界已完成，但正式产品preparer、ESP32原子`IWifiCredentialSink`和完整activation状态机
  composition尚未连接；
- UNO R4/Renesas portable Noise crypto provider 已通过 SDK 编译；32 KiB 下真机峰值 stack/heap watermark 尚未退出；
- 完整ESP32 `cloud + provisioning + Direct NimBLE + identity/store`仍须验证1.5 MiB双OTA至少10%余量；
- App生产scanner/session尚未实现13 B strict decoder和切换重连；服务器生产Enrollment/receipt事务也未完成；
- extended advertising可以未来作为性能扩展，但不得改变这13 B语义或让legacy scan response路径失效。

因此本批结论是“设备侧BLE模式所有权和advertising合同已通过软件门禁”，不是“完整安全配网产品已可发布”。
下一设备批进入ESP32 `IWifiCredentialSink`的掉电原子实现和产品composition；真机到位后补模式切换、断电、
backpressure、24 h与secret泄漏扫描。

## 9. secure Provisioning endpoint补充（2026-08-14）

第8节所述“正式产品preparer与application composition未连接”已部分退出：

- `BleNoiseModePreparation`负责start/restart前arm Noise并开始local session；
- `BleLocalProvisioningEndpoint`现在是`BleModeCoordinator`的Provisioning lifecycle endpoint，负责把Noise
  handshake/transport送入`ILocalProvisioningApplication`并把exact response送回；
- active disconnect/fault后endpoint进入Error/recovery并停止link，产品loop调用`restartActive()`后才恢复同profile广告；
- foreign session capacity/fault不会清当前active application session；
- TX背压不会重复执行application或推进Noise nonce，pending期间的新request按协议错误关闭session。

因此正式composition应向`BleModeCoordinator`传入`BleLocalProvisioningEndpoint`，不能继续传裸
`BleNoiseProvisioningChannel`。完整ACK、buffer和claim handoff合同见
`DEVICE_V2_BLE_LOCAL_PROVISIONING_ENDPOINT.md`。Wi-Fi connection/TLS activation、Active后切Direct与真机门禁仍未完成。
