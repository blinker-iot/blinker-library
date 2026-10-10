# Device V2 控制面与运行时

> 2026-08-15 修订：Device V2 只保留 canonical CBOR/HMAC 控制面；旧
> `LoginContract`、`LegacyLoginJson`、`HttpControlPlane` 已从 V2 源码和探针删除。
> 旧设备兼容由旧固件与服务器旧 endpoint 独立承担，新设备不双栈、不 fallback。

## 1. 分层边界

```text
Active CloudCredential
          |
          v
HttpCloudSessionProvider -- device-cloud-session/2 over TLS + canonical CBOR
          |
          v short-term SessionCredentials + generation
ManagedMqttTransport
          |
          v
MqttFrameTransport -> TransportHub -> DeviceRuntime -> BBP/2
```

`ISessionProvider` 只表达短期 MQTT credential 生命周期；`ManagedMqttTransport` 不知道
CloudCredential secret、HTTP 或 CBOR；`MqttFrameTransport` 只处理 MQTT connect/subscribe 和完整
BBP/2 binary frame。PubSubClient 位于 adapter 层，可以被其他 `IMqttClient` 实现替换。

Device V2 的 session、Manifest、State、Command、BLE 和 MQTT 数据面都禁止 JSON/CBOR 猜测或
v2/legacy fallback。页面布局不在 cloud-session body 中，也不由 runtime 保存。

## 2. Cloud session

`HttpCloudSessionProvider` 从 Active CloudCredential 构造 HMAC proof，经
`POST /api/v2/device-sessions` 换取受 broker/topic/TTL 限制的短期 MQTT credential。完整 wire、
幂等、错误、buffer 和三端职责见 `DEVICE_V2_CLOUD_SESSION_CONTRACT.md`。

长期 secret 只在 HMAC 计算期间载入 RAM，并在 HTTP transaction 开始前清零。caller 提供互不重叠的
320 B request、640 B response 和 518 B短期 credential arena。网络 gate 为 false 时不得启动
DNS/TCP/TLS；未知网络结果重试原 requestId，只有明确 `RequestExpired/NewRequest` 才换新 ID。

核心 `IHttpClient` 是单请求、可轮询接口。Arduino adapter 只负责 HTTP 字节流，不解释业务；拒绝
明文降级、redirect、chunked、重复 Content-Length、header 注入和越界 body。底层 SDK 的
DNS/TCP/TLS 仍可能阻塞，所以双无线产品必须以真机最大 loop latency 为准。

## 3. Managed MQTT

Managed 层负责：

1. network gate 同时控制 session HTTP 和 MQTT；
2. provider generation 变化后原子替换短期凭据并重连；
3. MQTT authentication failure 请求一次新 generation；
4. refresh/backoff 时，在旧 credential 未过期的前提下维持连接；
5. provider 终止失败或 credential 过期时关闭 MQTT；
6. 通过 `lastError()` 向产品层保留真实 provider/binding/MQTT 错误。

credential view 只在下一次成功 generation 或 `stop()` 前有效。当前模型为单 Arduino loop 协作调度；
跨 RTOS task 时必须另加队列或双 arena，不能并发读取正在刷新的 view。

## 4. Runtime 与传输所有权

`DeviceRuntime` 实现 HELLO/ID gate、Manifest 分页、streaming fingerprint、State
snapshot 分页、COMMAND/PATCH 校验、ACK/ERROR、replay window 和可靠 outbox。它不读取登录
响应，也不拥有页面坐标、颜色或布局。

产品组合中，云 transport 由 `Client/TransportHub` 以 `Managed` 生命周期管理；Direct BLE 以
`External` 注册，只参与路由和发送，由 `BleModeCoordinator` 负责启停/轮询。完整顺序见
`DEVICE_V2_PRODUCT_ORCHESTRATOR.md`。

## 5. App 页面边界

```text
App/服务器页面资产：拖拽/Agent -> widget slot -> Property/Action/Event binding
设备 Manifest：Property/Action/Event、类型、access 与约束
DeviceRuntime：校验并路由 BBP/2，不保存页面布局
```

默认组件建议由 App/服务器根据 Manifest 与产品模板生成；设备 descriptor 不包含 widget 或页面。
cloud session 也不携带 Manifest body。App/服务器按 Manifest fingerprint/revision 缓存和分页获取
设备能力，再把用户布局绑定到稳定 endpoint key。

## 6. 发布阻断

设备参考实现和主机测试已完成，但正式产品仍需 App/Gateway/Broker codec、proof/幂等、TTL/topic
ACL、撤销 E2E，以及 ESP32/UNO R4 真机断网、response loss、并发 BLE、heap/stack 和24小时 soak。
HTTP 200只表示短期凭据已签发；必须在 MQTT CONNECT 与 BBP/2 HELLO/ID gate 后才发布在线状态。
