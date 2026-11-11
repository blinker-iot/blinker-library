# Device V2 Arduino 实现单元布局

> 状态：2026-08-23，C2/C3 已完成。本文只冻结编译结构与单 ZIP 编译门禁；公开 API、BBP/2、CBOR、登录合同和 Product 行为未改变。

## 1. 结论

标准 Arduino 库仍只有一个 `src/` 和一个 ZIP。Arduino Builder 可见的普通 C++ 实现已从 69 个 `.cpp` 收口为
16 个职责单元：12 个跨平台单元、1 个 Arduino 通用单元和 3 个平台单元。

```text
src/BlinkerV2/implementation/
├── BlinkerApi.cpp
├── BlinkerControl.cpp
├── BlinkerCore.cpp
├── BlinkerIdentity.cpp
├── BlinkerModel.cpp
├── BlinkerProtocol.cpp
├── BlinkerProvisioningBle.cpp
├── BlinkerProvisioningController.cpp
├── BlinkerProvisioningWifi.cpp
├── BlinkerRuntime.cpp
├── BlinkerSecurity.cpp
├── BlinkerTransport.cpp
├── BlinkerArduino.cpp
├── BlinkerEsp32.cpp
├── BlinkerUnoR4.cpp
└── BlinkerWioTerminal.cpp
```

原实现仍在各职责目录，以 69 个 `.inc` 片段保存。它们不是第二套源码，也不会被 Arduino 单独编译；每个片段只有一个
上层职责单元引用。这样保留按类定位和小范围 review，同时避免 69 次独立编译器启动。

## 2. 边界规则

- 普通 `.cpp` 只能位于 `BlinkerV2/implementation`，文件集合必须精确等于上述 16 个；architecture 和 ZIP builder
  同时 fail closed。
- `.inc` 是跨平台实现片段，不是公开 include。用户和 Product header 不得直接引用。
- 需要按 Sketch 选择、且会触发第三方 SDK 依赖发现的实现继续使用 selector 引入的 `.ipp`；不得塞进公共聚合单元。
- ESP32、UNO R4、Wio 平台持久化/crypto 由三个独立平台单元承载。条件编译仍在片段内部，其他 MCU 不解析对应 SDK。
- 不继续合并为单个巨型 `.cpp`。16 个单元是编译启动成本、并行度、链接/诊断粒度之间的当前边界。
- 匿名命名空间中的通用 helper 使用职责化名称；禁止用宏批量重命名、`#include *.cpp` 或全局 `using namespace` 规避冲突。

## 3. 测试与链接策略

Host CMake 只编译 12 个跨平台职责单元一次，所有行为测试链接同一个静态库；平台 storage adapter 测试可直接把对应
`.inc` 指定为 C++ source，避免为了一个适配器引入完整 Arduino SDK。

Arduino 使用 `dot_a_linkage=true` 和 function/data sections。聚合后不能再用旧的细粒度 `.cpp.o` 名称推断某功能是否进入
固件；门禁改为：

1. archive member 验证职责单元存在；
2. 最终 ELF defined symbols 验证 MQTT/BLE/Noise/WiFiProv 的必需或禁止符号；
3. `libraries.cache` 验证第三方依赖集合；
4. compile database 验证源码来自已安装 ZIP，而不是工作树。

## 4. C2/C3 证据

- Windows MSVC + ASan：Host `25/25`；另行完成 200,000 次 deterministic protocol fuzz。
- architecture、Python syntax、单库 release boundary 通过。
- 单 ZIP：256 个文件，恰好 16 个 `.cpp`、69 个 `.inc`；SHA-256
  `7513cb8a0fa20a73fe8b8cad376039f8f8625e7462eb85a22b5e31eda9645687`。
- 三平台 Core clean compile：

| 目标 | Flash | Global RAM |
|---|---:|---:|
| ESP32 | 314,787 B | 21,768 B |
| UNO R4 WiFi | 72,648 B | 8,532 B |
| Wio Terminal | 51,340 B | core 未输出 RAM 汇总 |

- 从空 sketchbook 安装上述 ZIP 后，八个正式目标全部 clean compile；依赖集合、最终 ELF 必需/禁止符号和安装源码
  provenance 全部通过：

| 安装 ZIP 目标 | Flash | Global RAM |
|---|---:|---:|
| ESP32 WiFi | 993,467 B | 50,192 B |
| ESP32 BLE | 744,975 B | 42,632 B |
| ESP32 WiFi+BLE (`min_spiffs`) | 1,395,135 B | 62,848 B |
| ESP32 CloudSmokeTest | 1,010,891 B | 50,216 B |
| UNO R4 WiFi | 134,408 B | 12,956 B |
| UNO R4 BLE | 178,180 B | 16,608 B |
| Wio Terminal WiFi | 137,456 B | core 未输出 RAM 汇总 |
| Wio Terminal BLE | 169,120 B | core 未输出 RAM 汇总 |

实际依赖闭包保持按模式裁剪：ESP32 WiFi 只使用 PubSubClient/WiFi/Network，ESP32 BLE 只使用 NimBLE；UNO R4
WiFi 使用 PubSubClient/WiFiS3，BLE 使用 ArduinoBLE/Crypto。UNO R4 BLE 中出现的 WiFiS3 来自官方
`Preferences -> Modem` 存储后端，不是 WiFi Product 泄漏。Wio WiFi 与 BLE 分别只进入 rpcWiFi 与 rpcBLE 依赖图。

ESP32 首次全量构建仍需数分钟，说明 69→16 只关闭了编译单元膨胀，并未消除 ESP32 core、第三方依赖和 Product header
模板实例化成本。本次全新隔离目录安装依赖并串行完成八个 clean 目标约 85 分钟，其中 ESP32 WiFi/BLE/WiFi+BLE/
CloudSmoke 的日志墙钟区间约为 10/15/22/10 分钟，UNO R4 与 Wio 单目标约 2～4 分钟。耗时主要来自 ESP32 core、
NimBLE 逐源依赖发现和 Seeed 依赖安装，不是最终固件链接闭包膨胀。门禁现已直接输出 ZIP 安装、依赖安装、逐目标编译和
总耗时，后续不再依赖日志时间估算。

## 5. 下一批 C4

保持单 ZIP 和 16 个职责单元不变，收口正式 control host、plain/TLS profile 与 server key ring 的发布注入，并增加
placeholder fail-closed。普通 Sketch 仍只填写 DeviceKey/SSID/password；用户不得修改库内部配置。C4 不扩展新协议、不恢复
69 个逐类 `.cpp`，也不拆成每 MCU/transport 一个 ZIP。
