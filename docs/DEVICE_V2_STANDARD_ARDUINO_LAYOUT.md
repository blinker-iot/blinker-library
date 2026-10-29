# Device V2 单库 Arduino 架构与实施方案（2026-08-19）

> 当前发行入口（2026-10-27）：本文保留单库架构及历史资源证据。公开 ESP32 云端受限候选的
> 配置／安装以[发布方案 R3～R5](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md)及
> [安装说明](DEVICE_V2_ESP32_CLOUD_BETA_INSTALLATION.md)为准；仓库默认 development 头或 GitHub 源码 ZIP 不能替代 profile 化候选。

2026-09-13 B1 修订：当前执行状态见[基础路线](DEVICE_V2_BASELINE_CLOSURE_ROADMAP.md)和
[本批安装矩阵](DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md)。下文历史资源数字不替代当前版本证据。

## 1. 修订结论

Device V2 对 Arduino 用户只发布一个库、一个版本和一个 ZIP：

```text
BlinkerDeviceV2-<version>.zip
```

同一次安装同时提供 ESP32、UNO R4 的普通 WiFi 与普通 BLE 能力。不能再把
`MCU × WiFi/BLE` 组合发布为多个库，也不能要求用户分别安装 Core、Product 或 Port。

用户只做编译期模式选择：

```cpp
#include <BlinkerWiFi.h>
Blinker.begin("DEVICE_KEY", "SSID", "PASSWORD", power);
```

或：

```cpp
#include <BlinkerBLE.h>
Blinker.begin(power);
```

两者共享同一个 `Blinker`、Property/Action/Event、回调、上报和 `run()` API；`begin()` 不再接收
`blinker::WiFi`/`blinker::BLE` mode tag。模式入口是头文件，不是独立安装包。

上一版文档把“Arduino 会递归编译 `src` 下全部 `.cpp`”直接推导为“必须按产品发布多个 ZIP”，结论过度；随后又
把 `dot_a_linkage=true` 当成完整模式隔离，同样不准确。静态归档和 section GC 可以删除未引用实现，却不能阻止
Arduino 依赖扫描器从库内另一模式的 `.cpp` 发现并编译外部库；部分 Core 还会以 `--whole-archive` 提交整个库。
当前修正为：公共 Core 保持 `.cpp` + archive，
依赖 WiFi/BLE SDK 的模式实现改为 selector 按 Sketch 引入的内部 `.ipp`，Product binding 改为对应平台头文件。
是否串入仍必须由 map/ELF 和公开产品 clean compile 证明，不能只看源码结构或类名。

## 2. 标准发布树

仓库根本身就是可被 Arduino IDE“添加 ZIP 库”或复制到 `libraries/` 的完整库，不再生成另一套公开源码树：

```text
blinker-library/
├── library.properties
├── library.json
├── keywords.txt
├── LICENSE
├── README.md
├── src/
│   ├── Blinker.h
│   ├── BlinkerWiFi.h
│   ├── BlinkerBLE.h
│   ├── BlinkerWiFiBLE.h
│   ├── BlinkerV2Advanced.h
│   └── BlinkerV2/
│       ├── api/                 # 组件与 facade
│       ├── core/
│       ├── model/
│       ├── protocol/            # BBP/2、CBOR、BLE framing
│       ├── runtime/
│       ├── transport/
│       ├── provisioning/
│       ├── security/
│       ├── implementation/      # 职责分组的 17 个普通编译单元
│       └── arduino/
│           ├── config/          # 单一官方 host/TLS/key-ring 发布配置
│           ├── internal/        # Product composition，用户不可 include
│           ├── ports/
│           │   ├── arduino/
│           │   ├── esp32/
│           │   ├── uno_r4/
│           │   └── wio_terminal/
│           └── platform/
│               ├── esp32/
│               ├── uno_r4/
│               └── wio_terminal/
├── examples/
│   ├── WiFiQuickStart/
│   ├── BleQuickStart/
│   ├── WiFiBleQuickStart/
│   └── CloudSmokeTest/
├── tests/
├── tools/
└── docs/
```

Arduino 发布 ZIP 白名单只包含根 metadata、`src/`、`examples/`、`README.md` 和 `LICENSE`；`tests/`、
`tools/`、`docs/`、`release/` 不进入用户 ZIP。高级 Core 示例移入 `tests/arduino`，避免 IDE 中出现第二套用户入口。

旧的根 `ports/`、`products/arduino/` 和 `integrations/` 不再承载源码。Port 仍是内部窄接口，但它在物理上属于
同一个 Arduino 库，而不是具有独立 metadata/版本的子库。

## 3. 编译和链接边界

当前 `src` 普通 `.cpp` 已收敛为 **17 个**，全部位于 `implementation/`，由发布器和架构检查同时约束。
第 17 个是已有、默认关闭宏的 ESP32 LAN 实现，B1 修复发布器漏列；不为满足旧的“8～16”估计而合并无关职责。
91 个单元是 2026-08-23 的历史状态，不再是待删清单。编译时间仍须实测，减少单元不能代替 SDK 依赖扫描、
实际安装内容及 ELF 隔离检查；本批安装门禁拒绝构建产物/额外源码和工作树路径混入。

### 3.1 平台边界

ESP32 源文件仅在 `ARDUINO_ARCH_ESP32` 下产生定义；UNO R4 源文件仅在
`ARDUINO_ARCH_RENESAS[_UNO]` 下产生定义。平台 SDK 头文件必须放在对应条件分支内部，其他平台不能解析它。

### 3.2 模式边界

每个平台只保留其真实支持的 Product binding，并由 selector 直接引入：

```text
BlinkerWiFi.h -> <Mcu>WiFiProduct.h -> WiFi Port header -> WiFi Port .ipp
BlinkerBLE.h  -> <Mcu>BLEProduct.h  -> BLE Port header  -> BLE Port .ipp
BlinkerWiFiBLE.h -> Esp32WiFiBLEProduct.h -> BLE WiFiProv（默认，可选 SoftAP）+ WiFi + IDF NimBLE
```

`.ipp` 是库内部的、带 `inline` 定义的选择性实现，不是用户入口，也不是把整个库改成 header-only。协议、状态机、
存储记录和通用 runtime 继续是普通 `.cpp`，由 archive/section GC 裁剪。只有会触发第三方库发现的
WiFi/BLE/MQTT/crypto 适配实现由 selector 引入。禁止恢复同名模式 `.cpp`，否则 Arduino 会在未选择该模式时仍扫描
其 include，并编译不需要的外部库。

公开产品门禁必须分别检查最终 ELF：

- WiFi ELF 不得含 BLE radio、Noise、Controller pairing；
- BLE ELF 不得含 HTTP、MQTT、PubSubClient、WiFi station；
- ESP32 ELF 不得含 UNO R4 Port；
- UNO R4 ELF 不得含 ESP32 Port。

UNO R4 BLE 构建中当前仍会看到 WiFiS3：不是 WiFi Product 或 MQTT 串入，而是 Renesas core 的
`Preferences.h -> Modem.h -> WiFiS3` 存储后端传递依赖。最终图中 PubSubClient 已为 0。若真机/编译时间数据证明
收益足够，再以 RA4M1 独立 NVM 后端替换；不能把该上游依赖误报成 selector 失败。

### 3.3 ESP32 WiFi+BLE 边界

ESP32 WiFiProv Port 与 WiFi+BLE Product 已位于标准 `src`，平台 `.cpp` 由 ESP32 架构宏保护。
当前默认 BLE WiFiProv，可选 SoftAP；配网和 Direct 统一使用 SDK 的 IDF NimBLE，不恢复第二套 Bluedroid/
外部 NimBLE-Arduino Host 或已退役的 portable Noise 配网图。公开 selector 不等于任意 SDK 都含所需配置，
也不等于生产发布。UNO R4 只提供普通 WiFi/普通 BLE；Wio WiFi 公开，Wio BLE 为显式 preview，无混合产品。

### 3.4 TFT_eSPI/LVGL 可借鉴与不照搬之处

TFT_eSPI 同样依赖编译期选择：安装目录中的 `User_Setup_Select.h` 选择一个 setup，使处理器、总线和屏幕驱动在预处理
阶段裁剪。优点是单库和低运行时开销；不足是配置属于全局安装实例，升级可能覆盖，本机多个项目使用不同屏幕时也
容易发生配置漂移。Blinker 采用相同的“编译前只选择一张实现图”原则，但选择放在 Sketch 的
`BlinkerWiFi.h`/`BlinkerBLE.h`，不要求修改库内全局配置。

LVGL 把核心、`lv_conf.h` 功能裁剪和显示/输入驱动回调分离，这与 Blinker 的 Core、Product、Port 分层相近；但
Arduino 必须保证所有翻译单元看到同一份配置宏，否则会出现 ABI/功能不一致。Blinker 因此只允许资源/协议配置存在
一个官方配置事实源，用户 selector 只选择产品，不允许 Sketch 重新定义一组影响 Core ABI 的宏。

## 4. 外部依赖策略

Arduino `library.properties` 没有按架构或模式声明条件依赖的能力。当前统一库阶段先显式声明：

- PubSubClient 2.8：WiFi MQTT；
- ArduinoBLE 2.0.2：UNO R4 BLE；
- Crypto 0.4.0：UNO R4 BLE 的 Noise/Ed25519 fallback；
- Seeed Arduino rpcWiFi/rpcBLE/rpcUnified：Wio Terminal WiFi/BLE Port。

ESP32 BLE **不依赖外部 NimBLE-Arduino**。当前 Port 直接使用 Arduino-ESP32 随 ESP-IDF 提供的 NimBLE C API，
由同一套 SDK controller、Host 和配置完成编译；这也避免与 WiFiProv 引入第二套 BLE Host 或发生符号/生命周期冲突。

Library Manager 安装时可以解析这一个扁平依赖集合；手工“添加 ZIP”是否自动安装依赖取决于 IDE/CLI 路径，发布
验证必须从空 sketchbook 安装并记录行为。即使全部依赖都已安装，最终固件也只能链接当前模式使用的对象。

这不是长期最优依赖面。后续按收益排序收缩：

1. 普通 WiFi 保持内部 PubSubClient 默认后端；高级自定义实现通过既有窄接口接入，不要求初学者传 MQTT 对象，
   不把重写 MQTT 或换 ArduinoMqttClient 当基础能力的前置；
2. ESP32 继续使用 Arduino-ESP32 内置的 ESP-IDF NimBLE，不重新引入外部 NimBLE 包；
3. UNO R4 继续使用官方 ArduinoBLE，Crypto 只在 BLE 产品真正需要的对象中引用；
4. Wio Terminal 的 Seeed RPC 依赖只由 Wio Port 引用，并继续推动平台包提供标准能力；
5. 如 Arduino 生态无法表达条件依赖，再评估预编译架构归档，但不得恢复多 ZIP。

## 5. 官方配置边界

控制面 host、端口、Plain/TLS 选择和固件版本是库发布配置，不是要求初学者在 Sketch 中维护的第二套参数。
ESP32 P-256 与 UNO R4 Ed25519 server key ring 分文件保存，只由对应 BLE Product 引用。当前仓库中的 host 和公钥
是联调占位值，不能作为生产发布凭据；生产发布前必须由 Service 冻结真实 endpoint/key ring 并通过轮换门禁。

DeviceKey、SSID、WiFi password 仍是用户/设备运行参数，不写入官方配置。DeviceKey 登录、BBP/2 wire、组件
合同没有因本次目录和打包修订而改变。

## 6. 实施批次与状态

| 批次 | 内容 | 状态 |
|---|---|---|
| U-ARCH-1 | 冻结单库/单 ZIP、Product selector API 和标准发布树 | 已执行 |
| U-ARCH-2 | 将公开 Product/Port 物理迁入 `src/BlinkerV2/arduino`，公开示例归一 | 已执行 |
| U-ARCH-3 | ESP32 WiFiProv/WiFi+BLE 迁入标准 `src`，删除 P3 并加平台编译边界 | 已执行 |
| U-ARCH-4 | WiFi/BLE/WiFiBLE compile、ELF 图隔离、Flash/RAM/编译时间基线 | B1 本机七目标通过；SDK/资源/时间及发布边界见 B1 报告 |
| U-ARCH-5 | 单一公开 ZIP builder/CI 主路径；物理退役旧按产品发布工具链 | 已执行 |
| U-ARCH-6 | 空 sketchbook 安装单 ZIP 并编译正式示例，删除旧分包发布器与过期文档 | B1 本机安装矩阵已过；固定上游包的干净 CI 仍待验证 |
| U-ARCH-7 | 收敛普通 `.cpp`，记录 clean-build 时间并复验 ELF 闭包 | 单元已收敛为 17 个；当前编译/资源/耗时见 B1，不重新合并全部实现 |

任何一组 clean compile 或 ELF 隔离失败，都在当前批次修正对象边界；不通过增加用户安装包数量规避。

U-ARCH-4/6 历史实证如下，资源/哈希/旧 API 描述只说明当时结果；当前入口是 `Blinker.h` 默认 WiFi
（`BlinkerWiFi.h` 同义），当前 ZIP/SDK/资源以 B1 为准：

1. Arduino archive 会把不同子目录的对象 basename 扁平化。Product 与 Port 原来都叫 `Esp32WiFi.cpp` 时，
   Product 对象在 `.a` 中被同名 Port 对象覆盖，最终出现 `lifecycle(...)` 未定义。当前 Product binding 已改成
   selector 可达的 `*WiFiProduct.h`/`*BLEProduct.h`；发布器仍拒绝普通 `.cpp/.c -> .o` basename 冲突。
2. 旧门禁只按 Blinker 类名检查 ELF，漏掉了外部 WiFi/BLE 库被另一模式 `.cpp` 触发编译/链接的问题。模式 Port
   改为 selector 引入的内部 `.ipp` 后，正式安装 ZIP 门禁得到：ESP32 WiFi
   `980,015 B Flash / 49,856 B global RAM`，ESP32 BLE `763,007 B / 43,064 B`；UNO R4 WiFi
   `125,432 B / 12,696 B`，UNO R4 BLE `193,108 B / 16,896 B`。ESP32 BLE 从默认 APP 分区约 96%
   降至约 58%，原“图自身就是 96%”结论撤回。Flash 数值可能受构建目录等工具链输入影响而有几十至百余字节波动，
   RAM 与模式隔离结论不变。
3. 新 map 审计确认 ESP32 WiFi 不再编译 NimBLE，ESP32 BLE 不再编译 Arduino WiFi/PubSubClient；UNO R4 WiFi
   不再编译 ArduinoBLE/Crypto，UNO R4 BLE 不再编译 PubSubClient。R4 BLE 中 WiFiS3 只由 Renesas
   `Preferences -> Modem` 传递进入，并非 Blinker WiFi Product。CI 已加入按目标检查外部依赖路径的回归门禁。
4. 新单一发布器在依赖版本冻结后两次独立构建均生成 305 文件、402,207 B 的
   `BlinkerDeviceV2-0.1.0.zip`，SHA-256 都是
   `7994e0cb2c1252d4b19ea8313d8bc4ab5d5ec435794ec08eb010edaec6de2db3`。自动门禁已将该 ZIP 安装到
   全新 sketchbook，精确安装锁定依赖，并从这个安装目录完成 ESP32 WiFi/BLE、UNO R4 WiFi/BLE 四目标
   `--clean` 编译、实际链接库集合和 `compile_commands.json` 来源审计；Arduino CLI 只识别一个
   `BlinkerDeviceV2 0.1.0`。CLI 本地 `--zip-path` 不会自动安装 `depends`，门禁因此显式安装发布清单中的精确版本；
   Library Manager 可解析，手工 ZIP 路径现阶段仍须先安装依赖。
   旧 per-product/Core builder、manifest、profile verifier 与禁用 CI job 已物理删除；
   本地 interop builder 只产生同一统一库的临时联调副本。
5. selector 修订后的 Host 门禁为 40/40；pre-enrollment exact 校验和 20,000 次 deterministic fuzz 均通过。
6. `library.properties` 已将 Arduino IDE metadata 默认入口冻结为 `BlinkerWiFi.h`；BLE/WiFiBLE 示例显式改用
   对应 selector。`Blinker.h` 只保留共享声明/高级入口，不同时 include 多个 selector，也不把 mode 参数塞回
   `begin()`。

## 7. 完成定义

统一 Arduino 发布只有在以下条件同时满足后才能标记完成：

1. 一个 ZIP 可在 Arduino IDE/CLI 安装，库管理器中只出现一个 Blinker 条目；
2. ESP32 WiFi/BLE/WiFiBLE、UNO R4 WiFi/BLE、Wio WiFi 均从同一个 ZIP 的真实安装目录编译，
   SDK/配置可用独立 sketchbook 隔离但不能改源码/示例；Wio BLE preview 的正式准入另列；
3. 四个公开示例只使用 selector、组件、`begin()`、`run()`，不出现 internal 类型；
4. ELF 确认未选模式不进入 Flash/RAM；
5. `docs/tests/tools/release` 不进入发布 ZIP；
6. 旧 per-product ZIP 只能作为历史/内部测量产物，不能进入 release 清单或用户文档；
7. App/Service、真机和生产 endpoint/key ring 未完成时，库可标记“架构可编译”，不能标记“可生产上线”。
