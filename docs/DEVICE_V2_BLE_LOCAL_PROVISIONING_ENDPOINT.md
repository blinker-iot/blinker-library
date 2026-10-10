# Device V2 BLE 本地安全端点合同

> 状态：2026-08-17 R4b-4d Core 已完成。旧 Enrollment/1、ClaimProof、永久 localAccessKey 和只适用于
> NNpsk0 的实验 application 已删除；本文只描述当前 `BleLocalProvisioningEndpoint + LocalSetupApplication`。
> durable setup saga 已接入 application handler seam；official BLE-only/WiFi+BLE composition 与真机门禁
> 仍未完成。

## 1. 当前对象图

~~~text
BleModeCoordinator
  -> BleNoiseModePreparation
       -> ordinary NN: no PSK
       -> enhanced NNpsk0: ISetupPskProvider
  -> BleLocalProvisioningEndpoint
       -> BleNoiseProvisioningChannel
       -> LocalSetupApplication
            -> EnrollmentTicketVerifier
            -> LocalSetupSaga : ILocalSetupOperationHandler
                 -> Ownership / Controller / WiFi / Cloud stores
~~~

endpoint 是 `ITransportLifecycle`，只负责 channel/application 绑定、session 隔离、背压和故障收口。它不解析
Ticket、WiFi、Ownership 或 Controller body，也不访问 store。所有大 buffer 由 composition 静态提供。

## 2. 启动和握手

1. composition 生成/恢复当前 8 B setupSessionLocator，并交给 `BleModeCoordinator` 的静态 mode profile。
2. `BleNoiseModePreparation` 先取消旧 channel/session，按 profile arm channel，再调用
   `LocalSetupApplication::beginSession(locator)`；handler 在此时只读判断 fresh/可恢复/Active，并给出目标
   ownership generation。普通 DIY NN 不读取 PSK；显式 Enhanced/Manufactured NNpsk0 才短暂读取与
   locator 绑定的 32 B 高熵 PSK。
3. endpoint `start()` 只接受已 arm channel 和互不重叠的 caller-owned workspace/response storage，随后独占
   channel callback 并启动 BLE link。
4. App 的 HelloRequest 位于 initiator handshake payload；application 生成 HelloResponse，endpoint 通过
   `sendHandshakeResponse()` 放入 responder handshake。
5. 只有 responder handshake 成功进入 Noise TX record 后，endpoint 才读取 final handshake hash 并调用
   `application.secureSessionReady(hash)`。若 TX `WouldBlock`，保留 exact HelloResponse，稍后重试；不得提前
   提交 transcript 或重复调用 application。
6. 后续 Ticket/operation 全部使用 Noise transport record。

## 3. 背压与会话隔离

- channel 只有一个 TX record slot。application 已执行但 send 返回 `WouldBlock` 时，endpoint 保留
  response storage 中的 exact plaintext、payload kind 和长度；`poll()` 先 flush，再接收新 request。
- pending response 期间出现新 application request 是协议错误，进入 recovery；App 必须 request/response
  串行，超时只重发同一 exact envelope。
- `activeSessionId` 必须与 channel event 一致；其他 session 的 connect/disconnect/payload/fault 只计入
  `ignoredForeignSessionEvents`，不能结束当前 session。
- active session 断开、Noise fault、application/response 失败或 pending 冲突都调用
  `application.endSession()`、清零 workspace 并进入显式 recovery。上层停止旧 link，重新 prepare/arm/start，
  不在故障 channel 上静默续跑。
- `stop()` 解绑所有 callback、停止 channel、结束 application session并清零敏感 buffer。

## 4. 固定资源

当前 LocalSetup composition 的 caller-owned 最小容量为：

| storage | 最小值 | 生命周期 |
|---|---:|---|
| operation workspace | 512 B | 每次 application handle；返回后清零 |
| response storage | 273 B | response 成功进入 Noise TX 后清零；背压时保留 |

operation workspace 内部复用为 256 B scratch + 256 B operation-response body；Ticket verification 使用前
216 B。`LocalSetupApplication` 在 UNO R4 32 位 ABI 为 216 B，`LocalSetupSaga` 为 40 B，runtime record/
store 为 240/4 B、durable BSG2 为 248 B；`NoiseNnSession` 272 B，
`BleNoiseProvisioningChannel` 520 B。channel 已内含一份 session，SizeProbe 同时实体化这些数组只是 ABI
回归门禁，不是产品峰值相加公式。

本片 UNO R4 Small SizeProbe 为 52,324 B Flash / 14,668 B global RAM；+284 B 是新增 Saga 三个 ABI
测量数组常驻造成。Host/Windows ASan 36/36、200,000 deterministic fuzz 和 LocalSetup/Noise 三端 exact
golden 通过。实际 product 仍必须测 BLE
stack/heap、ATT MTU、notify 背压、断连、crypto stack、WiFi 共存和 24 h。

## 5. 后续落地状态与边界

R4b-5 曾把 endpoint/application/saga 接入双平台 internal WiFi+BLE 实验图，并完成 GATT、BSG2 storage、
WiFi candidate connect/confirm/recovery 与 Cloud Session lifecycle。当前该 8-operation saga 只保留为
Enhanced/跨平台实验：正式 ESP32 WiFi+BLE 改用 SoftAP WiFiProv 最小 Bootstrap，UNO R4 不提供 WiFi+BLE；
普通 Platform BLE facade 已公开，LocalBLE 仍为 internal，二者真机门禁未完成。后续不得：

- 把旧 `LocalProvisioningCoordinator`、Enrollment/1 或永久 setup key 接回；
- 仅按 Ownership Active 拒绝可能处于 Controller/Network/Cloud 中间态的恢复会话；
- 让 ESP32 facade 拥有业务流程，或为 UNO R4/Renesas 复制状态机；
- 仅因 Core/saga 编译通过就把 endpoint 包装成 official `Blinker.begin(BLE/WiFiBLE)` 产品；
- 在本阶段扩展 HC-05/Bluetooth Classic SPP。

LocalSetup wire、App/Gateway 执行输入和完整发布阻断见
`DEVICE_V2_LOCAL_SETUP_APPLICATION_CONTRACT.md`。
