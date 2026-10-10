# Device V2 Arduino compile gates

`compile_matrix.py` 验证 Core 可移植性、独立 Port 和 internal 候选；它不会生成或模拟用户产品包。正式 Arduino
库的唯一构建器是 `tools/build_arduino_release.py`；`verify_installed_release.py` 将同一个 ZIP 安装到独立
sketchbook，再编译安装目录的四个公开示例。源码树编译/历史成功不能替代 ZIP 安装证据。

## 三类门禁

- `core`：无堆 Client/model/BBP2/runtime 在 ESP32、UNO R4、ESP8266、Wio Terminal/SAMD51、RP2040、
  nRF52840 上编译；这只证明 Core 可移植，不代表平台已有无线 Product。
- `ports`：HTTP、PubSubClient、storage、crypto、BLE security 等窄 Port 独立编译。
- `internal`：尚未发布的网关/LAN候选与故障/资源探针不作为公开示例进入 ZIP；公开 WiFi+BLE 的 SDK/Port
  实现仍在标准 `src` 中，不能把正式 composition 移回测试目录。

正式产品支持范围现为：

| 平台 | 普通 WiFi | 普通 BLE | WiFi+BLE/WiFiProv |
|---|---|---|---|
| ESP32 | 公开 | 公开，需目标 SDK 提供 IDF NimBLE | ESP32 公开入口；BLE WiFiProv 同样需要 IDF NimBLE |
| UNO R4 WiFi | 公开 | 公开 | 暂不提供 |
| Wio Terminal | 公开 | 显式 preview，未作为正式发布能力 | 不支持 |

## 常用命令

```powershell
python tests/arduino/compile_matrix.py --target esp8266-core
python tests/arduino/compile_matrix.py --target wio-terminal-core
python tests/arduino/compile_matrix.py --target esp32-device-key-session
python tests/arduino/compile_matrix.py --target uno-r4-device-key-session
python tests/arduino/compile_matrix.py --group facade
python tests/arduino/compile_matrix.py --group examples
python tests/arduino/compile_matrix.py --target uno-r4-size-probe
```

脚本不会隐式安装 core 或依赖。CI/开发机必须先安装目标声明的固定版本。Small 目标统一传入
`BLINKER_RESOURCE_PROFILE=1`，每个 build 使用独立目录并要求 Arduino 输出 size summary。

通过 `core` 不代表已有 WiFi、BLE、存储或配网适配。RP2040 与 nRF52840 目前仅证明 Core
可移植；ESP8266 另有 cloud Port 门禁，但没有正式 BLE Product。用户发布判断以统一库的七组 compile/ELF
结果为准，而不是 internal probe 数字。

## 安装与验证边界（B1，2026-09-13）

- 用户只安装一个 ZIP，选择头文件决定模式，不按 MCU/传输拆包。这里的 Python 是维护者门禁工具，不是
  Arduino 用户运行 Sketch 的依赖。
- IDE「添加 ZIP 库」/CLI `lib install --zip-path` 不应被描述成已经替用户安装了外部库；用 Library Manager
  安装 `library.properties` 的依赖。本门禁默认在独立 sketchbook 安装选中目标的固定依赖，也可用
  `--dependency-dir <已有 libraries>` 离线复制后严格校验版本；不修改用户原库目录/SDK。
- `--reuse-work-dir` 必须匹配 ZIP SHA256，并重新逐文件校验安装内容、排除新增源码；编译命令不得引用
  工作树的 `.cpp` 或 `-I src`。map/ELF 同时检查未选传输、平台、LAN和网关不进入固件。
- 编译通过不证明默认明文登录/实际业务、断电恢复、全 SDK/全平台发布可靠性；尤其 development profile
  的 `.invalid` host/测试公钥只供编译，不可拿去烧录后要求连接生产环境。
- 默认公开示例的 ELF 还必须排除业务 WallClock/TimeSyncService/Coordinator 和未安装的 TimeSyncResponder。
  显式开启授时的对照可复用 `verify_link_closure(..., time_enabled=True)`；只放行完整业务时钟，不放行网关/LAN 等未选扩展。
  Hub 自身的完整 Product 注入对照见 `tests/internal/Esp32HubTimeCompileProbe`，不替代真实子设备授时门禁。
- ESP32 WiFi 的已安装 profile 开启 HTTP 或 MQTT TLS 时，允许 SDK 附带 `NetworkClientSecure`；明文/纯 BLE/其他平台不能借此增加依赖。
  旧 core 可能把 secure socket 包含在原库中，因此它不是额外用户必装库；其余依赖缺失/新增仍失败。
- 使用哪个 SDK/配置必须写进证据，不只记 Arduino 版本号。本机 XIAO 的 3.3.11 IDF NimBLE 配置与别的
  ESP32 芯片/预编译 SDK 并不等价；不能临时去掉 capability 检查或静默退到另一 BLE 实现。

```powershell
python -m unittest discover -s tests/release -p 'test_*.py'
python tools/build_arduino_release.py --check-only
python tests/arduino/verify_installed_release.py --zip-path <同一个ZIP> --work-dir <新独立目录> --data-dir <目标SDK目录> --target uno-r4-wifi --target uno-r4-ble
```

最新结果/未完成项见 [B1 安装矩阵](../../docs/DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md)。Wio BLE 长稳、B2c、授时和
LAN 产品化仍各自验收，不以扩大内部探针数量替代普通产品门禁。
