# blinker-library 功能改进与性能优化建议

生成日期：2026-07-02  
分析对象：`D:\Git\diandeng\blinker-library`

## 1. 结论摘要

`blinker-library` 当前版本已经从早期“大而全”的 Arduino IoT SDK 收敛为更轻的跨板卡 SDK：核心入口围绕 `BLINKER_BLE`、`BLINKER_WIFI/BLINKER_MQTT`、`BLINKER_HTTP` 三类模式展开，其中 `BLINKER_WIFI` 会被转为 `BLINKER_MQTT`；当前 README 也明确移除了 `BLINKER_PRO`、蜂窝/AT、Bridge、天气、短信/微信、语音助手、Mesh、ESP32-CAM、OTA、Push、APConfig 等能力。

这次对照 GitHub 上较成熟的 IoT SDK 后，建议不要简单恢复所有旧功能，而是优先补齐“现代 IoT 产品最小闭环”：

1. 安全 OTA：带版本检查、签名/哈希、进度回报、失败回滚。
2. 配网与网络管理：BLE/AP 配网、多 WiFi 凭据、按键信号、重置与诊断。
3. 设备状态模型：把 widget 数据上升为 `desired/reported` 或属性模型。
4. 云端命令/任务：RPC、Direct Method、Jobs 风格的异步命令与幂等执行。
5. 传输层抽象：MQTT/HTTP/BLE/LAN 共享同一业务 API，而非业务逻辑散落在适配器中。
6. 性能基线：把 heap、flash、消息延迟、重连耗时和示例编译纳入 CI。

性能方面，当前最大风险集中在动态内存和阻塞式流程：`malloc/free`、`String` 反复拼接、固定 20 字节 BLE 分片、HTTP 每次新建请求、固定间隔重连、`run()` 路径中的 `delay()`，都会在长时间运行、弱网、低内存板卡上放大碎片化和卡顿风险。优化重点应放在“可复用缓冲区 + 非阻塞状态机 + 批量/合并上报 + 指数退避 + 可观测指标”。

## 2. 本地代码基线

| 观察项 | 当前实现证据 | 风险或机会 |
| --- | --- | --- |
| 模式入口清晰但绑定较硬 | `src/Blinker.h` 中 `BLINKER_WIFI` 会 `#undef` 后转为 `BLINKER_MQTT`，HTTP 独立，BLE 分 ESP32 BLE 与串口 BLE。 | 外部 API 简单，但业务能力容易和传输模式耦合。建议保留简单入口，同时内部拆出传输接口。 |
| 默认消息尺寸较小 | `src/Blinker/BlinkerConfig.h`：WiFi/MQTT/HTTP 默认 `BLINKER_MAX_SEND_SIZE=1024`，非原生 WiFi 为 128。 | 适合小消息，但 OTA、批量遥测、属性同步需要分片/流式处理，不能强塞进普通 `print()`。 |
| 协议层频繁动态分配 | `src/Blinker/BlinkerProtocol.h` 在 `checkFormat()` 中 `malloc(BLINKER_MAX_SEND_SIZE)`，发送后 `free()`。 | 高频 widget 上报会产生 heap 碎片。建议改为实例级固定缓冲区或可配置池。 |
| HTTP 路径大量拼接 `String` | `src/Adapters/BlinkerHTTP.h` 与 `src/Blinker/BlinkerApi.h` 中多处 `String msg/url_iot/payload`、`http.getString()`、`http.begin(url_iot)`。 | TLS/HTTP 请求成本高，payload 复制多。建议流式 JSON、复用 client、批量上报与超时/重试策略。 |
| MQTT 有 ping 保活 | `BLINKER_MQTT_CONNECT_TIMESLOT=5000UL`、`BLINKER_MQTT_PING_TIMEOUT=30000UL`；`BlinkerMQTT::available()` 中会定期 `ping()`。 | 固定间隔重连不够抗雪崩；缺少离线队列、指数退避、失败原因指标。 |
| TLS 校验需要强化 | 多处有 `setInsecure()` 或 `http.begin(url_iot, ca); TODO` 的历史痕迹。 | 对产品级 SDK，这是 P0。需要 CA bundle、证书轮换策略、可配置 pinning、失败诊断。 |
| BLE 分片保守 | ESP32 BLE 发送按 20 字节分片并 `delay(5)`；接收端动态分配 `BLEBuf`，旧 BLE 接收缺少和 `BlinkerBleFi` 一样清晰的边界保护。 | 兼容性好但吞吐低。建议协商 MTU、流控、环形缓冲区、边界检查。 |
| CI 偏旧且失败不可靠 | `appveyor.yml` 使用 Arduino 1.8.9、ESP32 core 1.0.4；`test/build.sh` 遇到单个 sketch 编译失败只打印，不立刻退出失败。 | 需要现代矩阵：Arduino CLI、PlatformIO、ESP32 core 2/3、UNO R4、RP2040、库大小预算。 |

## 3. 外部方案参考

公开指标为 2026-07-02 通过 GitHub API 查询到的近似值，主要用于判断生态影响，不作为功能优先级的唯一依据。

| 方案 | 当前公开指标 | 可借鉴点 |
| --- | ---: | --- |
| [ESP-IDF](https://github.com/espressif/esp-idf) | 约 18.5k stars | ESP-MQTT 支持 MQTT 5.0，TLS 示例默认使用证书 bundle；OTA 示例覆盖 HTTPS 下载、写入 OTA 分区、重启和回滚；版本支持策略清晰。参考：ESP-MQTT 文档、OTA README。 |
| [Blynk Library / Blynk.Edgent](https://github.com/Blynk-Technologies/blynk-library) | 约 4.0k stars | Blynk Library 覆盖大量硬件和 App widget 体验；Edgent 把安全云连接、动态凭据配网、OTA、网络管理和按键交互打包成产品化流程。参考：Blynk README、Blynk.Edgent 文档。 |
| [Mongoose OS](https://github.com/cesanta/mongoose-os) | 约 2.7k stars | 强调固件框架、云连接、OTA 与配置。适合作为“SDK 不只是 API，还包含设备生命周期”的参考。 |
| [Particle Device OS](https://github.com/particle-iot/device-os) | 约 1.1k stars | 设备 OS 化、云端 OTA、网络抽象和产品发布节奏值得借鉴，但与 Arduino 库定位差异较大，不建议直接复制架构。 |
| [AWS IoT Device SDK Embedded C](https://github.com/aws/aws-iot-device-sdk-embedded-C) | 约 1.1k stars | 模块化库包括 MQTT、HTTP、JSON、Device Shadow、Jobs、Device Defender、Fleet Provisioning；coreMQTT 使用用户实现的 transport 层，并提供指数退避工具。 |
| [Azure IoT SDK C](https://github.com/Azure/azure-iot-sdk-c) | 约 0.6k stars | 明确区分 MQTT/AMQP/HTTPS 能力；Device Twin、Direct Methods、云到设备消息、认证方式表述完整。 |
| [ThingsBoard Client SDK](https://github.com/thingsboard/thingsboard-client-sdk) | 约 0.2k stars | MQTT/HTTP 客户端以接口注入，不强绑定 Arduino；同一业务 API 可跑在 Arduino 或 ESP-IDF；大量容量通过模板参数显式控制。 |
| [ArduinoIoTCloud](https://github.com/arduino-libraries/ArduinoIoTCloud) | 约 0.14k stars | `Thing`/`Property` 模型简单，`ArduinoCloud.update()` 主循环体验好，属性读写权限和 on-change callback 对 Arduino 用户友好。 |

关键外部证据：

- Blynk Library 强调“连接硬件到云、App 控制、规模化管理”，并覆盖 400+ 硬件；Edgent 明确包含安全连接、动态配网、OTA 和状态交互示例。  
  来源：[Blynk Library README](https://github.com/Blynk-Technologies/blynk-library)、[Blynk.Edgent overview](https://docs.blynk.io/en/blynk.edgent/overview)
- ThingsBoard SDK 声明可通过任意 `IMQTT_Client`/`IHTTP_Client` 实现接入，不直接绑定具体传输库，并支持 Arduino/ESP-IDF 集成。  
  来源：[ThingsBoard Client SDK README](https://github.com/thingsboard/thingsboard-client-sdk)
- AWS Embedded C SDK 将 MQTT、HTTP、JSON、Shadow、Jobs、Defender、Fleet Provisioning 拆成可移植模块，并提供 backoffAlgorithm。  
  来源：[AWS IoT Device SDK Embedded C README](https://github.com/aws/aws-iot-device-sdk-embedded-C)
- ESP-IDF 的 MQTT 文档强调 TLS 证书 bundle 和 MQTT 5 示例；OTA 示例覆盖 HTTPS 下载、写入 flash、重启、回滚和版本防循环。  
  来源：[ESP-MQTT 文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/mqtt.html)、[ESP-IDF OTA README](https://github.com/espressif/esp-idf/blob/master/examples/system/ota/README.md)
- Azure IoT SDK 文档把设备孪生、Direct Methods、云到设备消息和认证方式放在能力矩阵里。  
  来源：[Azure IoT SDK C README](https://github.com/Azure/azure-iot-sdk-c)
- ArduinoIoTCloud 用 `Thing` 和 `Property` 建模，属性可声明 `READ`、`WRITE`、`READWRITE`，并通过 `ArduinoCloud.update()` 保持主循环简洁。  
  来源：[ArduinoIoTCloud README](https://github.com/arduino-libraries/ArduinoIoTCloud)

## 4. 功能改进建议

### P0：安全连接与证书治理

当前代码中仍能看到 `setInsecure()`、CA TODO、硬编码 host/port 的痕迹。建议先建立一套最小安全基线：

| 建议 | 实施方式 |
| --- | --- |
| 默认启用服务器证书校验 | ESP32 优先使用证书 bundle 或根证书；UNO R4/RP2040 根据底层 WiFi/TLS 能力提供等价实现；`BLINKER_WITHOUT_SSL` 只允许显式开发模式。 |
| 证书轮换策略 | 提供多 CA slot 或 CA bundle，文档写明证书过期/替换流程；避免单一过期证书导致大规模离线。 |
| 连接失败可诊断 | 暴露 `Blinker.lastError()`、TLS/HTTP/MQTT 错误码、broker、RSSI、heap、上次断线时间。 |
| 敏感数据保护 | auth/token 不在普通 debug 日志中明文输出；示例中使用 `secrets.h` 或占位宏。 |

### P0：恢复“现代化 OTA”，但不要恢复旧版 PRO OTA 形态

README 已声明 OTA 被移除，但从竞品看 OTA 已经是产品化 IoT SDK 的基础能力。建议重新设计为独立模块，而不是恢复旧 `BlinkerOTA` 的所有历史耦合。

建议 API：

```cpp
BlinkerOTA.begin({
  .currentVersion = "1.2.0",
  .channel = "stable",
  .onProgress = onOtaProgress,
  .onResult = onOtaResult
});
```

能力边界：

| 层级 | 建议 |
| --- | --- |
| ESP32 | HTTPS OTA + 双 OTA 分区 + SHA256/签名校验 + 回滚确认；失败后保留旧固件。 |
| UNO R4 / Pico W | 先定义 OTA capability flag；只有底层 flash/bootloader 支持时启用；否则仅保留版本检查和升级通知。 |
| 云端协议 | OTA 作为 Job/Task，不走普通 widget `print()`；包含 `jobId`、版本、大小、hash、url、状态、进度、错误码。 |
| 体验 | App/云端能看到 queued/downloading/verifying/rebooting/succeeded/failed。 |

优先级理由：OTA 能直接降低用户维护成本，也能让后续安全修复真正落地。

### P0：传输层抽象与重连状态机

当前 `BlinkerMQTT`、`BlinkerHTTP`、`BlinkerBLE` 都继承 `BlinkerStream`，但业务请求仍大量散落在 API 和适配器中。建议引入更明确的内部接口：

```cpp
class BlinkerTransport {
public:
  virtual bool begin(const BlinkerCredentials& cred) = 0;
  virtual bool loop() = 0;
  virtual bool connected() const = 0;
  virtual int publish(const BlinkerMessage& msg) = 0;
  virtual int receive(BlinkerMessage& out) = 0;
  virtual BlinkerError lastError() const = 0;
};
```

落地策略：

1. 先把 `BlinkerProtocol` 对 `conn->print/available/lastRead` 的依赖包一层适配器，不破坏现有外部 API。
2. MQTT/HTTP/BLE/LAN 都实现统一收发语义。
3. 重连统一采用状态机：`DISCONNECTED -> WIFI_CONNECTING -> AUTHENTICATING -> CLOUD_CONNECTING -> SYNCING -> ONLINE`。
4. 重试使用指数退避 + jitter；保留最大退避与手动立即重连。

参考 AWS 的 `backoffAlgorithm` 设计，不建议多个设备在同一固定 5 秒周期同时重连。

### P1：引入设备状态模型，降低 widget 协议耦合

Blinker 的优势是 App widget 体验，但现代 IoT SDK 普遍会把 UI 控件之下的设备状态抽象出来。建议新增 `BlinkerProperty<T>` / `BlinkerThing` 风格 API：

```cpp
BlinkerProperty<bool> power("power", READWRITE);
BlinkerProperty<float> temperature("temperature", READ);

void setup() {
  Blinker.addProperty(power, onPowerChange);
  Blinker.addProperty(temperature);
}

void loop() {
  Blinker.run();
  temperature.report(readTemp(), ON_CHANGE);
}
```

收益：

- 对接 Azure Device Twin / AWS Shadow / ArduinoIoTCloud Property 的心智模型。
- Widget 仍可作为 UI 绑定层，但核心数据不再只以 widget key/value 存在。
- 支持离线缓存、变更合并、权限控制、版本号和冲突处理。
- 后续语音助手、自动化、Bridge 可通过属性模型恢复，而不是每个集成各写一套协议。

### P1：云端命令、RPC 与 Jobs

建议新增一层“云到设备命令”：

```cpp
Blinker.onCommand("restart", [](const BlinkerCommand& cmd) {
  cmd.accept();
  scheduleRestart();
  cmd.done();
});
```

功能点：

- 命令带 `id`、`method`、`params`、`timeout`、`ttl`。
- 支持同步响应和异步响应。
- 支持幂等：同一 `id` 重复下发不重复执行。
- OTA、配置迁移、采样频率变更、校准、重启都走 Jobs/Command，而不是挤进普通消息。

参考 Azure Direct Methods、AWS Jobs、ThingsBoard RPC。

### P1：配网与网络管理产品化

当前已有 ESPTouch、ESPTouch V2、bleFi、WiFiMulti 示例/入口。建议收敛成统一网络管理器：

| 能力 | 建议 |
| --- | --- |
| 配网入口 | `BlinkerProvisioning.begin(BLE | AP | SmartConfig)`，示例中保持一眼可用。 |
| 多 WiFi | 支持最多 N 个 WiFi 凭据，记录上次成功、RSSI、失败次数，优先连接可靠网络。 |
| 存储 | ESP32 使用 NVS；其他平台使用可插拔 storage backend。存储结构带 version。 |
| 用户交互 | 统一按键：短按状态、双击进入配网、长按清除配置。 |
| 诊断 | 上报网络失败原因、信号强度、IP、DNS、证书错误。 |

这基本对应 Blynk.Edgent 中 Blynk.Inject + Network Manager 的价值，但应保持 blinker 现有 API 的轻量感。

### P1：LAN 本地控制作为云断连时的保底路径

`BLINKER_PROTOCOL_HTTP_SERVER`、mDNS、WebSocket server 的历史形态说明本库天然适合保留本地控制。建议将 LAN 控制定义为正式能力：

- 云端在线时同步 token/权限；离线时只允许同局域网内经授权客户端控制。
- mDNS 广播只暴露必要信息，如 device id、capability、protocol version。
- LAN 消息和云消息共用 property/command 模型，避免双协议分叉。
- 给 App/上位机提供发现和连接示例。

### P2：可观测性与设备诊断

建议新增 `BlinkerDiagnostics`：

| 指标 | 用途 |
| --- | --- |
| heap/free/fragmentation | 发现长期运行碎片化。 |
| RSSI、IP、DNS、NTP 状态 | 定位网络问题。 |
| MQTT/HTTP/BLE 错误码 | 定位协议问题。 |
| 上报队列长度、丢弃计数 | 判断上报积压和丢弃情况。 |
| reboot reason、firmware version、build id | OTA 和现场排障。 |

这些指标可以低频上报，也可在 debug 模式下本地打印。

### P2：旧能力恢复策略

不建议一次性恢复所有已移除能力。建议按“可复用底座”排序：

1. 先恢复 Push/Event，但用统一 command/event 模型，不恢复旧 SMS/WeChat 专用 API。
2. Bridge 先作为 property/command 转发器，不恢复复杂旧网关。
3. 语音助手不要先写 AliGenie/DuerOS/MIOT 独立封装，而是导出标准属性 schema，再由云端适配。
4. Weather/AQI/GPS 属于应用数据服务，不应进入核心 SDK；可做扩展包。
5. 蜂窝/AT 模块只有在真实用户需求明确时才恢复，且必须走统一 transport 接口。

## 5. 性能优化建议

### 5.1 动态内存与碎片化

问题点：

- `BlinkerProtocol::checkFormat()` 每次自动格式化都 `malloc(BLINKER_MAX_SEND_SIZE)`，发送后 `free()`。
- MQTT/HTTP 初始化时大量 `malloc` 凭据、topic、host 字符串。
- MQTT/BLE 接收缓冲在消息到达时动态分配。
- `String`、`substring()`、`http.getString()` 会产生额外 heap 分配。

建议：

| 优先级 | 优化 |
| --- | --- |
| P0 | 把 `_sendBuf` 改为实例级固定数组或可配置静态缓冲；发送后清零，不释放。 |
| P0 | BLE 接收必须边界检查，超过 `BLINKER_MAX_READ_SIZE - 1` 时丢弃并报错。 |
| P1 | MQTT/HTTP 凭据用固定上限结构体或 `std::array<char, N>`；初始化失败要安全释放。 |
| P1 | 对必须使用 `String` 的路径调用 `reserve()`，减少多次扩容。 |
| P2 | 提供 `BLINKER_USE_STATIC_BUFFERS`、`BLINKER_RX_BUFFER_SIZE`、`BLINKER_TX_BUFFER_SIZE` 配置。 |

建议验收：

- ESP32 连续 24 小时每秒上报 1 次，heap 最低水位稳定，碎片化不持续恶化。
- BLE 连续收发 10,000 条 64 字节消息，无越界、无泄漏。
- 低内存板卡以 128/256/512/1024 buffer 四档编译通过。

### 5.2 JSON 序列化与解析

当前 vendored ArduinoJson 为 7.4.3，代码多处使用 `JsonDocument`。ArduinoJson v7 简化了 capacity，但在 MCU 上仍要控制解析规模和复制次数。

建议：

- 对云端响应使用 `DeserializationOption::Filter`，只解析 `message/detail/data` 等必要字段。
- 对 MQTT/HTTP 入口设置 nesting limit 和最大 payload。
- 避免 `payload = http.getString()` 后再 `deserializeJson()` 的双份内存；优先支持从 stream 解析。
- 对简单响应先做状态码/字段快速检查，再进入完整 JSON。
- 为常见上报路径提供直接写入 `Print`/transport 的序列化函数，避免先拼成 `String`。

### 5.3 MQTT 连接与上报

现状已有 keepalive，但策略偏固定。

建议：

1. 固定 5 秒重连改为指数退避 + jitter：1s、2s、4s、8s、16s、最大 60s，并带随机抖动。
2. 上报队列分级：状态变更优先，周期遥测可合并，debug 日志可丢弃。
3. 支持“最后值合并”：同一 property 在离线期间只保留最新值。
4. 支持 QoS 或应用层 ack：关键命令/OTA 不能只依赖普通 publish。
5. 断线后不要在 `run()` 中长时间阻塞；把 WiFi、NTP、MQTT、同步拆成阶段。

### 5.4 HTTP 请求成本

现状多处每次业务请求都新建 URL、`HTTPClient`、payload，并读取完整响应字符串。

建议：

- 业务请求合并：config/data/event/heartbeat 统一走一个 `BlinkerHttpClient`，集中处理 headers、timeout、错误码。
- 支持 keep-alive 或连接复用，至少避免重复构造相同 header/string。
- 上报端点支持批量 telemetry：多条 sensor 数据合并一次 POST。
- 对低频业务加 ETag/If-None-Match 或版本号，避免重复拉取 sharers/timer/config。
- 所有 HTTP 调用都要有明确 timeout，不允许长期卡住主 loop。

### 5.5 BLE 吞吐与稳定性

当前 ESP32 BLE 固定 20 字节分片，兼容老设备但吞吐低。

建议：

- 优先协商 MTU，根据 MTU 动态分片；保留 20 字节 fallback。
- 根据 MTU 和确认结果动态分片，减少固定 `delay(5)` 带来的吞吐损耗。
- 接收侧使用环形缓冲区和帧状态机，支持 `\n`、长度前缀或 JSON 完整性判断。
- bleFi 和普通 BLE 共用分片/解析工具，避免两套行为。

### 5.6 编译体积与模块裁剪

建议：

- Core、Widgets、Cloud Storage、OTA、Provisioning、LAN、Diagnostics 拆为功能宏或子模块。
- BLE-only 模式不要拉入 HTTP/MQTT/WebSocket/云存储代码。
- HTTP-only 模式不要拉入 MQTT topic 和 WebSocket server。
- 大量固定字符串继续使用 `BLINKER_F`/PROGMEM，但敏感信息不打印。
- CI 记录每个示例的 flash/RAM 增量，设置预算阈值。

## 6. 工程化与 CI 建议

当前 `test/build.sh` 会打印单个 sketch 编译失败，但不会立即让 CI 失败。建议先修这个，否则后续优化没有防线。

建议矩阵：

| 维度 | 建议 |
| --- | --- |
| 构建工具 | Arduino CLI + PlatformIO；保留 Arduino IDE 兼容性验证。 |
| 板卡 | ESP32 core 2.x/3.x、Arduino UNO R4 WiFi、RP2040/Pico W、AVR BLE 串口模式。 |
| 示例 | Hello、WiFi/MQTT、HTTP、BLE、bleFi、CloudData、Timer、Widgets、未来 OTA。 |
| 静态检查 | `arduino-lint`、拼写、license、第三方库版本清单。 |
| 尺寸预算 | 每个示例输出 flash/RAM，超过阈值失败或警告。 |
| 长稳测试 | ESP32 硬件或模拟环境跑 24h 上报/断网/重连脚本。 |

`test/build.sh` 至少应在失败时累计 `failed=1` 并以非零退出码结束。

## 7. 分阶段路线图

### 0-2 周：打地基

- 修 CI 失败退出，迁移到 Arduino CLI。
- 加 `docs/capability-matrix.md`，把当前支持/移除能力写清楚。
- 安全基线：禁用默认 `setInsecure()`，补 CA 配置文档和示例。
- 修 BLE 接收边界，给 `_sendBuf` 做静态/复用改造。
- 增加 heap/size benchmark sketch。

### 1-2 个月：提升核心体验

- 引入内部 `BlinkerTransport` 接口，先兼容现有 API。
- MQTT/HTTP 统一重连退避、错误码、队列和批量上报。
- 新增 `BlinkerProperty<T>`，让 Widget 绑定到属性模型。
- 网络管理器统一 ESPTouch、bleFi、WiFiMulti。
- 建立 ESP32/UNO R4/Pico W 示例编译矩阵。

### 3-6 个月：产品化闭环

- 发布 OTA 模块：ESP32 先行，支持进度、校验、回滚、状态上报。
- 新增 Command/Job API，OTA、配置迁移、远程重启走统一任务协议。
- LAN 本地控制正式化，云断连时可受控使用。
- Diagnostics 上报与本地 debug 命令。
- 基于属性模型恢复必要的 Push/Event/Bridge 能力。

## 8. 不建议优先做的事

- 不建议把所有历史删除文件直接恢复，会重新带回 AT、蜂窝、PRO、语音助手等维护负担。
- 不建议把 OTA 做成普通 HTTP 下载示例就结束；没有签名、版本、回滚和状态回报的 OTA 对产品风险很高。
- 不建议继续扩大核心头文件的宏分支；应先拆内部接口。
- 不建议为了追求兼容性继续默认跳过 TLS 校验。
- 不建议把 Weather/AQI/SMS/WeChat 这类平台服务放回核心 SDK；更适合云端或扩展包。

## 9. 可直接拆分的 Issue Backlog

| Issue | 类型 | 优先级 |
| --- | --- | --- |
| `test/build.sh` 编译失败应返回非零退出码 | CI | P0 |
| 为 ESP32/UNO R4/Pico W 建立 Arduino CLI 构建矩阵 | CI | P0 |
| 移除默认 insecure TLS，新增 CA/cert bundle 配置文档 | Security | P0 |
| `BlinkerProtocol` 发送缓冲改为复用缓冲区 | Performance | P0 |
| BLE 接收增加边界检查和超长帧错误 | Performance | P0 |
| MQTT 重连改为指数退避 + jitter | Reliability | P0 |
| 新增 `BlinkerTransport` 内部接口 | Architecture | P1 |
| 新增 `BlinkerProperty<T>` 属性模型 MVP | Feature | P1 |
| 统一 ESPTouch/bleFi/WiFiMulti 为 Network Manager | Feature | P1 |
| 设计 OTA Job 协议和 ESP32 OTA 原型 | Feature | P1 |
| HTTP 请求集中到 `BlinkerHttpClient` 并减少 `String` 复制 | Performance | P1 |
| 增加 heap/flash/latency benchmark examples | CI/Perf | P1 |
| LAN 本地控制 capability 与 mDNS schema | Feature | P2 |
| Diagnostics API：heap/RSSI/error/reboot reason | Observability | P2 |

## 10. 推荐北极星指标

- Hello WiFi 示例从开机到云在线的 P50/P95 时间。
- MQTT 断网 5 分钟后恢复在线的 P50/P95 时间。
- 24 小时连续上报后的最低 heap 与碎片化变化。
- BLE 64 字节消息往返延迟与丢包率。
- OTA 成功率、平均耗时、失败回滚率。
- 每个示例 flash/RAM 大小。
- 用户需要写的最小样板代码行数。

这些指标比单纯“支持多少功能”更能指导 SDK 长期演进。
