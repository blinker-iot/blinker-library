# Device V2 Arduino 发布、CI 与平台矩阵

> **2026-08-23 复核：**此前安装 ZIP/ELF 数据仍是历史有效基线，但删减后版本必须重新跑完整矩阵。当前 `src`
> 有 91 个普通 `.cpp`；最新隔离安装已完成，ESP32 WiFi clean compile 在本机 6 分钟仍未结束，说明已有门禁验证了
> 链接闭包，却没有冻结可接受的首次编译成本。新增 P0：收敛到约 8～16 个普通 translation unit，随后再跑 8 个
> 正式目标。详见 `DEVICE_V2_ARCHITECTURE_CLOSURE_AUDIT.md`。

> 修订日期：2026-08-19。本文件替代旧的 per-product ZIP/profile 方案。旧
> `device-v2-packages.json`、official package builder 与 profile verifier 已退役，不再是事实源。

## 1. 冻结结论

Device V2 Arduino 只发布一个库、一个版本和一个 ZIP：

```text
BlinkerDeviceV2-0.1.0.zip
```

同一安装目录同时包含：

- ESP32 普通 WiFi；
- ESP32 普通 BLE；
- ESP32 WiFi+BLE；
- UNO R4 WiFi 普通 WiFi；
- UNO R4 WiFi 普通 BLE；
- Wio Terminal 普通 WiFi；
- Wio Terminal BLE 实验源码（必须显式 opt-in，不属于当前发布能力和正式编译矩阵）。

不能按 MCU、通信模式、Plain/TLS 或资源档位再生成用户安装包。新增 MCU 时仍向同一个库的
`src/BlinkerV2/arduino/platform/<mcu>` 与 `ports/<mcu>` 增加实现。

用户通过头文件选择产品，不在 `begin()` 中重复传模式：

```cpp
#include <BlinkerWiFi.h>
Blinker.begin("DEVICE_KEY", "SSID", "PASSWORD", power);
```

```cpp
#include <BlinkerBLE.h>
Blinker.begin(power);
```

```cpp
#include <BlinkerWiFiBLE.h>  // ESP32 only
Blinker.begin(power);
```

## 2. 发布边界

唯一事实源是标准 Arduino 库根与单一构建器：

```text
library.properties
library.json
keywords.txt
LICENSE
README.md
src/
examples/
tools/build_arduino_release.py
tools/arduino_release_profile.py
```

发布 ZIP 只包含根 metadata、`src` 和四类正式示例：

```text
examples/WiFiQuickStart
examples/BleQuickStart
examples/WiFiBleQuickStart
examples/CloudSmokeTest
```

以下内容永不进入用户 ZIP：

- `docs`、`tests`、`tools`、`.github`；
- internal probe、Host 测试、资源探针；
- 服务器联调配置和测试密钥。

`tools/build_arduino_release.py` 必须拒绝：

- 旧根 `products`/`ports` 源码回流；
- tests/internal 候选回流正式 Product；
- 非四类公开示例；
- metadata、架构或依赖集合漂移；
- Arduino archive 中重复的 `.o` basename。
- staged release 缺少显式 profile；
- production profile 使用 placeholder host、仓库测试 key，或 TLS 未提供 PEM CA。

发布 profile 只在 staged copy 中生成 `OfficialConfig.h`、`Esp32ServerKeys.h` 与 `PortableServerKeys.h`。它不包含 DeviceKey、
WiFi 密码或服务器私钥。Plain/TLS 是 profile 的明确选择，不强制所有 MCU 使用 TLS；详见
`DEVICE_V2_RELEASE_PROFILE.md`。

## 3. 平台/模式编译边界

平台无关 Core 保持普通 `.cpp` 并进入静态归档。依赖外部 Arduino 库或平台 SDK 的模式实现由 selector 经平台
Product header 引入内部 `.ipp`，避免未选模式触发第三方依赖发现。WiFiProv 也必须遵守此规则：公共 `.cpp`
即使最终 section 可被删除，仍会让 Arduino 依赖扫描器把 `Network/WiFi` 带入纯 BLE。`.ipp` 只属于内部，
不是用户入口；最终仍须通过 dependency/ELF 门禁证明没有进入未选产品。

| 目标 | 应进入最终 ELF | 必须排除 |
|---|---|---|
| ESP32 WiFi | DeviceKey HTTP、PubSubClient、ESP32 WiFi/storage | BLE、Noise、UNO R4 |
| ESP32 BLE | NimBLE、BLE frame、Noise、ESP32 crypto | WiFi、HTTP、MQTT、UNO R4 |
| ESP32 WiFi+BLE | SoftAP WiFiProv、DeviceKey/MQTT、NimBLE Direct、Controller method 2 | BLE-WiFiProv/Bluedroid、portable Noise setup、UNO R4 |
| UNO R4 WiFi | DeviceKey HTTP、PubSubClient、UNO WiFi/storage | BLE、Noise、ESP32 |
| UNO R4 BLE | ArduinoBLE、BLE frame、Noise、portable crypto | WiFi、HTTP、MQTT、ESP32 |

Product binding 与 Port 源文件不得同名。Arduino archive 会把不同子目录的对象 basename 扁平化；重复名称可能
覆盖对象并造成链接错误。当前 binding 使用 selector 可达的 `*WiFiProduct.h`/`*BLEProduct.h`，构建器仍持续检查
普通 `.cpp/.c` 的对象 basename。

该做法借鉴 TFT_eSPI/LVGL 的编译期裁剪，但不复制 TFT_eSPI 的安装目录全局 setup：Blinker 由每个 Sketch 的
`BlinkerWiFi.h`、`BlinkerBLE.h` 或 ESP32-only `BlinkerWiFiBLE.h` 选择实现图。与 LVGL 的 `lv_conf.h` 类似，影响所有翻译单元的资源/协议配置仍须
单一且一致，不能让用户 selector 改变 Core ABI。

## 4. 依赖策略

当前 `library.properties` 使用 Arduino 可表达的扁平依赖集合：

- PubSubClient 2.8；
- NimBLE-Arduino 2.5.0；
- ArduinoBLE 2.0.2；
- Crypto 0.4.0；
- Wio Terminal 所需的 Seeed rpcWiFi/rpcBLE/rpcUnified 固定版本。

Library Manager 可解析 `depends`。Arduino CLI 的本地 `lib install --zip-path` 不会自动安装依赖，因此手工添加 ZIP
前仍需准备依赖。依赖优化必须在同一 ZIP 内解决，不得以恢复 per-product 包规避：

1. 以真机/RF/heap 数据决定是否继续缩减 ESP32 BLE 图和外部 NimBLE 依赖；
2. 再评估小型 MQTT Port 或用户对象注入，降低 PubSubClient 强绑定；
3. 保留 UNO R4 官方 ArduinoBLE，确保 portable crypto 只被 BLE Product 链接；
4. 若未来采用预编译归档，也只发布一个 Arduino 库。

UNO R4 BLE 仍可在构建目录看到 WiFiS3，因为 Arduino Renesas 的 `Preferences.h` 通过 `Modem.h` 使用 WiFiS3
后端；它不是 WiFi Product/MQTT 串入。后续只有在独立 NVM 后端能以真机数据证明明显收益时才替换。

## 5. CI 主路径

### 5.1 Host/协议门禁

- Host 单元、golden 与存储故障测试；
- pre-enrollment exact vectors；
- deterministic protocol fuzz；
- architecture/source-boundary 检查。

### 5.2 单一发布门禁

两次独立构建同一 development-profile ZIP 并逐字节比较；CI artifact 名称必须显式包含 `development`，不能作为正式发布。
正式 ZIP 使用同一个构建器与 production profile，profile 是发布配置输入，不是 per-product 源码 manifest。

### 5.3 Arduino 正式矩阵

同一个 staged library 分别 clean compile：

1. ESP32 `WiFiQuickStart`；
2. ESP32 `BleQuickStart`；
3. ESP32 `WiFiBleQuickStart`；
4. ESP32 `CloudSmokeTest`；
5. UNO R4 `WiFiQuickStart` / `BleQuickStart`；
6. Wio Terminal `WiFiQuickStart`。

Core portability和独立 Port 可以有额外测试，但不得生成第二个发布包。
Wio BLE 只允许使用 `BLINKER_ENABLE_WIO_BLE_PREVIEW=1` 的独立内部真机门禁，不能混入上述发布矩阵。

## 6. 本地服务器联调

本机联调统一复用`tools/build_arduino_release.py`及development profile；profile明确局域网host/port、安全选项与公开验签材料。旧字符串替换生成器已于2026-09-24移除：

```powershell
python tools/build_arduino_release.py `
  --profile D:\private-build\development-profile.json `
  --allow-development-profile `
  --stage-dir D:\temp\BlinkerDeviceV2Interop
```

该目录仍是同一个 ESP32/UNO R4、WiFi/BLE 库。工具不修改仓库中的 `OfficialConfig.h`，不接收 `--platform`。
联调默认 Plain；生产 host、TLS 与 server key ring 由正式发行配置冻结。

## 7. 当前实证与阻断

2026-08-23 C3 从同一个 256 文件安装 ZIP 得到以下 Arduino CLI clean compile/ELF 结果：

| 目标 | Flash | global RAM | 图隔离 |
|---|---:|---:|---|
| ESP32 WiFi | 993,467 B | 50,192 B | PubSubClient/WiFi；不含 BLE/Noise/NimBLE |
| ESP32 BLE | 744,975 B | 42,632 B | NimBLE；不含 DeviceKey/MQTT/HTTP cloud |
| ESP32 WiFi+BLE | 1,395,135 B | 62,848 B | `min_spiffs`；WiFi cloud + NimBLE Direct |
| ESP32 CloudSmokeTest | 1,010,891 B | 50,216 B | 公开 WiFi API；不含 BLE |
| UNO R4 WiFi | 134,408 B | 12,956 B | 不含 ArduinoBLE/Crypto/Noise |
| UNO R4 BLE | 178,180 B | 16,608 B | 不含 PubSubClient/cloud；WiFiS3 仅为 Preferences 后端 |
| Wio Terminal WiFi | 137,456 B | core 未汇总 | 只含 rpcWiFi 侧闭包 |
| Wio Terminal BLE | 169,120 B | core 未汇总 | 只含 rpcBLE/Crypto 侧闭包 |

冻结精确依赖后的 C3 单一 ZIP 为 256 文件，SHA-256：

```text
7513cb8a0fa20a73fe8b8cad376039f8f8625e7462eb85a22b5e31eda9645687
```

正式门禁在隔离 sketchbook 中安装该 ZIP，显式安装发布清单的精确依赖版本，然后只从安装目录完成八个
公开目标的 `--clean` 编译；八组实际链接库集合、唯一 map、最终 ELF defined symbols 与
`compile_commands.json` 源码来源审计均通过，
且 CLI 只识别 `BlinkerDeviceV2 0.1.0`。本地 `--zip-path` 不会自动解析依赖是 Arduino CLI 行为，门禁已显式覆盖，
不再构成安装编译证据缺口。Host 门禁为 40/40，pre-enrollment exact 校验通过，20,000 次 deterministic fuzz 通过。
本批改动后另以 Windows MSVC ASan 重跑 40/40、pre-enrollment exact 与 1,000 次 deterministic fuzz，全部通过；
Windows 不提供 UBSan，Linux CI 仍负责 ASan+UBSan 组合门禁。

这里必须区分编译闭包与链接闭包。Arduino 1.5 library format 会递归编译单库 `src` 下的实现文件；Sketch
选择头文件不会把宏传播到其他 translation unit。因此，在保持“一个标准库、一个 ZIP、公共 Core/Advanced API”
的前提下，不能宣称 WiFi Sketch 完全不编译 BLE/Noise 的 portable `.cpp`。发布门禁冻结的是最终固件闭包：
`dot_a_linkage=true` 先形成库 archive，但不同 Core/产品可能按需抽取，也可能以 `--whole-archive` 提交后再做
section GC。门禁不能只看 archive inclusion table，必须从最终 ELF defined symbols 证明：WiFi 不得保留
BLE/Noise 实现，BLE 不得保留 DeviceKey/MQTT/HTTP cloud，实现中也不得保留其他平台的 storage/crypto。
将全部实现改成入口头文件包含的 `.ipp` 虽可减少首次编译对象，却会显著扩大 ODR、增量构建和维护风险，现阶段不采用。

Aily Builder 当前构建参数与正式 Arduino CLI 门禁不同；它可用于快速发现编译错误，但不得用于发布资源数字或
feature closure 判定。正式 ZIP 准入以 Arduino CLI 的
`libraries.cache`、link map 和安装来源审计为准。

当前不能标记为生产可发布：双账号分享 UI、PageLayout 云端持久化、24 h/容量门禁、生产 WSS/step-up/监控
仍未冻结。单 ZIP、入口级外部依赖闭包和 archive 成员级 feature/platform gating 本批已完成；后续不改变
单库/单 ZIP，也不恢复 `begin(mode, ...)`。
