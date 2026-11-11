# Device v2 MQTT 实施说明

> 实现批次：MqttFrameTransport + PubSubClientAdapter

## 1. dev_edu 的真实情况

当前 dev_edu 并未接入 ArduinoMqttClient：

| 平台 | dev_edu MQTT 实现 | 来源 |
|---|---|---|
| ESP32 | Adafruit_MQTT_Client | src/modules/mqtt |
| UNO R4/Renesas | PubSubClient | src/modules/pubsubclient |

因此新架构的首个真实适配器选择 PubSubClient，而不是 ArduinoMqttClient：

- UNO R4 的现有行为更容易对照；
- PubSubClient 可同时运行在 ESP32 和 UNO R4；
- 两个平台可以结束使用不同 MQTT 引擎的状态；
- P1-3 复核后冻结 PubSubClient 为当前默认/参考 adapter；ArduinoMqttClient 不进入当前
  依赖、推荐 profile 或实施关键路径。

核心不绑定 PubSubClient。依赖方向是：

~~~text
Blinker Core
  -> MqttFrameTransport
      -> IMqttClient
          -> PubSubClientAdapter
              -> 用户持有的 PubSubClient
                  -> 用户持有的 WiFiClient / WiFiSSLClient
~~~

## 2. MqttFrameTransport

平台无关实现位于：

- src/BlinkerV2/transport/MqttFrameTransport.h
- src/BlinkerV2/transport/MqttFrameTransport.inc（由 `implementation/BlinkerTransport.cpp` 唯一聚合）

职责：

1. 将 SessionCredentials 转换为 MqttConnectOptions；
2. 连接并订阅动态下发的 subscribeTopic；
3. 一个 MQTT binary payload 对应一个完整 BBP/2 frame；
4. 配置时要求 adapter 预检上下行 topic、QoS、最大 frame 与 packet capacity；
5. 发布前校验 frame magic、版本、长度和最大 frame；
6. 接收时只接受当前 subscribeTopic；
7. 若 adapter 能提供 retain metadata，拒绝 retained command frame；
8. 网络失败进入 Backoff，使 BLE/WS transport 可继续调度；
9. 到期后自动重连和重新订阅；
10. 不拥有 MQTT 对象，不分配堆内存。

初始状态机：

~~~text
Stopped
  -> Starting
      -> Online
      -> Backoff --到期--> Starting
      -> Error

Online --断线--> Backoff
任意运行状态 --stop--> Stopped
~~~

认证失败、错误安全模式、错误协议配置进入 Error；临时网络失败进入 Backoff。

## 3. PubSubClient Port

内部 header-only Port：

~~~text
ports/arduino/PubSubMqtt.h
~~~

采用 header-only 是为了按 Product 精确选择，不让 BLE-only 包安装或编译 PubSubClient。它没有独立
`library.properties`/版本，用户安装的是包含该 Port 的 WiFi 产品包。

### 3.1 对象创建

明文：

~~~cpp
WiFiClient network;
PubSubClient mqtt(network);

blinker::PubSubClientAdapter<0> adapter(
    mqtt,
    blinker::MqttSecurity::PlainTcp,
    1152,
    2); // socket timeout seconds
~~~

TLS：

~~~cpp
WiFiClientSecure network; // UNO R4 使用 WiFiSSLClient
PubSubClient mqtt(network);

blinker::PubSubClientAdapter<0> adapter(
    mqtt,
    blinker::MqttSecurity::Tls,
    1152,
    2);
~~~

应用持有所有对象，adapter 不 delete 它们。

### 3.2 安全模式

adapter 构造时必须声明底层 Client 的实际安全模式。登录返回的 SessionCredentials.security 与它不一致时连接失败。

这避免：

- mqtts 登录结果被明文 Client 使用；
- TLS 失败后静默降级到 TCP；
- ESP32 再次把 HTTP/TLS client 和明文 MQTT client 混用。

由于 PubSubClient 不公开底层 Client 的动态类型，adapter 无法自动证明用户传入的枚举是否诚实；官方聚合包在发布构建期选定 `WiFiClient` 或 `WiFiClientSecure`，普通 Sketch 不再创建或注入这组对象。Advanced 用户自行组装时仍必须让 Client 类型、adapter 枚举和 session profile 三者一致。

### 3.3 PubSubClient 限制

- 发布仅支持 QoS 0；
- 订阅支持 QoS 0/1；
- packet buffer 必须容纳 MQTT 固定头、topic 和完整 BBP/2 frame；
- setBufferSize 会使用 PubSubClient 自己的堆分配；
- PubSubClient 的默认 socket timeout 为 15 s；adapter 默认改为 2 s，也允许构造时显式调整；
- 2.8 callback 不向用户暴露 inbound retain/QoS，因此 metadata 标为 unknown，而不是错误地假定 retain=false；
- 普通函数指针 callback 没有 context 参数。

`poll(budgetMicros)` 是协作预算而不是抢占式截止时间：PubSubClient 的 CONNACK/读包等待受
2 s socket timeout 限制，但 DNS、TCP 和 TLS 建连仍可能受底层 Arduino Client 自身超时控制。
P1-4 必须在 WiFi+BLE 实机上测量最坏暂停；不能仅凭 `poll(1000)` 宣称 MQTT 一定在 1 ms
返回。

`MqttFrameTransport::configure()` 现在会分别以 publish/subscribe topic、QoS 和
`maxFrameSize` 调用 `IMqttClient::validatePacketCapacity()`。PubSubClient adapter 按最坏
5 B MQTT fixed header、2 B topic length、topic、可选 2 B packet id 和完整 BBP/2 frame
计算；任一方向放不下时直接返回 `BufferTooSmall`，不再等到运行期静默丢包。

生产 broker 必须禁止在设备下行 topic 保留 COMMAND。一个 adapter 独占一个 PubSubClient 的 callback；用户不能再覆盖该 callback。

为支持多个 adapter，使用模板 callback slot：

~~~cpp
PubSubClientAdapter<0> cloudA(...);
PubSubClientAdapter<1> cloudB(...);
~~~

同一进程内的活动实例必须使用不同 slot；重复 slot 在 connect 时返回 CapacityExceeded。

### 3.4 为什么不接入 ArduinoMqttClient

本轮基于官方 0.1.8 源码完成静态比较，结论不是“ArduinoMqttClient 不可用”，而是它对
当前 BBP/2 云通道没有净收益：

| 项目 | PubSubClient 2.8 | ArduinoMqttClient 0.1.8 | 当前决策 |
|---|---|---|---|
| 底层网络 | Arduino `Client&` | Arduino `Client&` | 相同 |
| 发布 | QoS 0 | QoS 0/1/2 | BBP/2 当前只需 QoS 0 |
| 订阅 | QoS 0/1 | QoS 0/1/2 | PubSub 已满足 |
| 入站 QoS/retain | callback 不暴露 | 可读取 | broker 必须禁止 retained 下行；暂非换库理由 |
| payload | 单个可调 packet heap buffer | 可流式发送，也有 `String` 与动态 payload/will buffer | 两者均非全无堆 |
| 等待行为 | socket timeout 可调 | QoS publish/subscribe 可同步等 ACK 到 connection timeout | 双无线仍需实测 |
| 项目状态 | 简单、现有两平台已编译 | 官方 metadata 仍标为 BETA | 不新增默认依赖 |

复核依据：[PubSubClient 2.8 API/limits](https://github.com/knolleary/pubsubclient/blob/master/src/PubSubClient.h)、
[PubSubClient receive/timeout implementation](https://github.com/knolleary/pubsubclient/blob/master/src/PubSubClient.cpp)、
[ArduinoMqttClient API/state](https://github.com/arduino-libraries/ArduinoMqttClient/blob/master/src/MqttClient.h)、
[ArduinoMqttClient allocation/ACK waits](https://github.com/arduino-libraries/ArduinoMqttClient/blob/master/src/MqttClient.cpp)、
[ArduinoMqttClient 0.1.8 BETA metadata](https://github.com/arduino-libraries/ArduinoMqttClient/blob/master/library.properties)。

只有出现“设备必须 QoS 1/2 发布”或“设备端必须获知下行 retain/QoS，且服务器/broker 约束
无法替代”这类明确产品需求时，才建立第二 adapter，并重新跑 contract、资源和实机阻塞
对比；不能为了抽象对称而维护两个默认 MQTT 引擎。

## 4. 资源测量

板卡版本：

- ESP32 Arduino Core 3.3.1；
- Arduino UNO R4 Core 1.5.3；
- PubSubClient 2.8。

### 4.1 明文 TCP

| 目标 | 仅 WiFiClient | BlinkerV2 + PubSubClient + Transport | 增量 |
|---|---:|---:|---:|
| UNO R4 Flash | 59,136 B | 63,964 B | 4,828 B |
| UNO R4 静态 RAM | 7,556 B | 8,464 B | 908 B |
| ESP32 Flash | 900,639 B | 908,519 B | 7,880 B |
| ESP32 静态 RAM | 44,648 B | 45,568 B | 920 B |

### 4.2 TLS

| 目标 | 仅 TLS Client | BlinkerV2 + PubSubClient + Transport | 增量 |
|---|---:|---:|---:|
| UNO R4 Flash | 62,056 B | 66,924 B | 4,868 B |
| UNO R4 静态 RAM | 7,572 B | 8,480 B | 908 B |
| ESP32 Flash | 989,967 B | 997,851 B | 7,884 B |
| ESP32 静态 RAM | 45,192 B | 46,112 B | 920 B |

以上是早期 MQTT adapter 独立增量基线，不等同于当前完整 Device V2 总尺寸。静态 RAM 不包含
示例设置的 1152 B PubSubClient runtime packet buffer；该 buffer 会在首次连接时通过
setBufferSize 分配。P1-4 将用当前代码重新测量 WiFi+BLE 同时在线的 heap watermark。

ESP32 总 Flash 较高主要来自 Arduino WiFi/TLS 网络栈；上述早期基线中 Adapter + Transport
的可比较增量约为 7.9 KB。

### 4.3 P1-3 当前编译烟测

| 目标 | 明文 Flash / 静态 RAM | TLS Flash / 静态 RAM |
|---|---:|---:|
| UNO R4 | 64,244 / 8,468 B | 67,196 / 8,484 B |
| ESP32 | 918,063 / 45,576 B | 1,007,423 / 46,120 B |

这些是当前完整编译总量，不可与上表的旧增量直接相减；同样不包含首次连接时分配的
1,152 B PubSubClient packet heap。新增 capacity contract + timeout 配置相对本轮修改前的
UNO R4 已知编译值增加约 280 B Flash / 4 B static RAM。

## 5. 已验证

- FakeMqtt 断线、退避、重连、重新订阅；
- `MqttFrameTransport::start()` 不同步连接，首次连接延后到 `poll()`；
- 显式 network-available gate：离线时不调用 MQTT connect，恢复后才重连；
- `ManagedMqttTransport` 通过通用 `ISessionProvider` 让同一 gate 同时覆盖
  session 获取和 MQTT，BLE 可先启动；v2 与 legacy 不再复制 Managed 层；
- TransportHub 每轮先服务 local/BLE，再服务 cloud-only MQTT，且各优先级内部保持轮转；
- BBP/2 binary publish；
- topic 过滤；
- malformed frame 过滤；
- 已知 retained 消息过滤；
- PubSubClient QoS 限制；
- 上下行 packet buffer 配置期预检与运行期发布上限；
- 2 s 默认 socket timeout 和 0 s 非法配置；
- callback slot 冲突；
- TLS/明文安全模式不匹配；
- MQTT 认证错误映射；
- ESP32 WiFiClient；
- ESP32 WiFiClientSecure；
- UNO R4 WiFiClient；
- UNO R4 WiFiSSLClient。

尚未进行真实broker联网测试，因为需要有效设备凭证、测试账号和实板。当前完成的是协议状态机、adapter contract、编译和链接验证。完整HTTP登录 + TLS MQTT + BLE的真正Small产品探针由Arduino CLI测得：UNO R4 145,296 B Flash / 14,108 B static RAM；ESP32安全完整的compact NimBLE为1,277,391 B / 60,648 B。ESP32正式4 MiB分区保留双OTA并给每个app `0x180000`，候选占81.21%、余18.79%；静态门禁已通过，真实broker/RF/heap门禁未通过。

## 6. 后续

1. Device V2 只保留
   `HttpCloudSessionProvider -> ManagedMqttTransport -> PubSubClient`；旧
   `HttpControlPlane`/JSON/authKey 源码和探针已删除，旧设备兼容由旧固件与服务器旧入口承担；
2. ESP32 cloud-only/BLE-only/dual归因和安全保留的build-wide feature strip已完成；保持compact profile与双OTA产品分区的可复现编译门禁；
3. 完成 PubSubClient + WiFi + BLE 同时在线的峰值 heap、最长 poll/connect 暂停和断线压力门禁；
4. 仅在出现 QoS 1/2 发布或入站 metadata 硬需求时重新评估第二 MQTT adapter；
5. 不再把 Adafruit MQTT 或 PubSubClient 源码复制进主库。

v2 session 的 canonical CBOR、HMAC、短期 credential、刷新和错误合同见
`DEVICE_V2_CLOUD_SESSION_CONTRACT.md`。PubSubClient 仍只是 `IMqttClient`
adapter；它不解析登录响应，也不持有长期设备身份。
