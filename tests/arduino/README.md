# Device V2 Arduino compile gates

`compile_matrix.py` 验证 Core 可移植性、独立 Port 和 internal 候选；它不会生成或模拟用户产品包。正式 Arduino
库的唯一构建器是 `tools/build_arduino_release.py`；`verify_installed_release.py` 将同一个 ZIP 安装到独立
sketchbook，再按七目标矩阵编译安装目录的四个普通通信示例。源码树编译/历史成功不能替代 ZIP 安装证据。

同一ZIP另含`R4TasksQuickStart`受维护四槽Plain参考。它在无匹配profile时明确编译拒绝，
不属于上述普通七目标；其原Core／Modem保护／heap与全消费者配置由
`tools/prepare_r4_restricted_release.py`单独构建并核最终ELF。该矩阵不据此增加一个普通R4任务目标，
也不将“普通R4默认0槽”误写成“受维护固定图未获有限准入”。IDE维护profile安装工具另取
`tools/install_r4_arduino_profile.py`及[安装说明](../../docs/DEVICE_V2_R4_ARDUINO_PROFILE_INSTALLATION.md)，
侧装`blinker:renesas_uno:r4tasks`，不改原SDK／库；只安装库或在Sketch写宏不等于补齐接线。

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

普通安装矩阵的范围及历史结果见 [B1 安装矩阵](../../docs/DEVICE_V2_B1_INSTALLATION_MATRIX_REPORT.md)。Wio BLE 长稳、B2c、授时和
LAN 产品化仍各自验收，不以扩大内部探针数量替代普通产品门禁，也不追加为已收口R4固定任务图的前置条件。

## R4固定任务图：按受影响范围选择窄入口（§11.58）

以下是后续发生对应代码变化时的维护入口，不是每批必跑清单；本次仅同步文档，未重跑：

```powershell
python -m unittest discover -s tests/release -p 'test_r4_arduino_profile.py'
python -m unittest discover -s tests/release -p 'test_r4_profile_binding.py'
python -m unittest discover -s tests/release -p 'test_r4_restricted_release.py'
```

- `test_r4_arduino_profile.py`：安装／核验／撤销及升级fixture，不执行真实SDK、编译器或硬件。
- `test_r4_profile_binding.py`：公开身份fixture与`OriginalCompilerBinding`的原ARM三语言预处理，二者证据分开。该类当前使用脚本内固定的本机原GCC路径；缺工具会`SkipTest`。通用discover退出0但含skip，不代表取得新ARM证据，不将fixture通过算成原工具通过；跨机路径便携性另列维护范围，不改SDK或静默换版本。
- `test_r4_restricted_release.py`：维护者生成器的准备合同fixture，不代替真实链接、签发选择或设备运行。

原有安装／发布护栏软件证据复用§11.54／§11.56，新护栏实际链接及必要实物差异复用§11.57；
现行范围取[§11.58](../../docs/DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#1158-有限支持收口后的剩余范围与文档对齐)。
普通七目标与受维护固定图不混算；本机通过不扩大任意Sketch、IDE GUI、公开分发、其他SDK或Wio的支持。
